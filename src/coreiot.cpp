#define LED_PIN 48
#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12

#include "coreiot.h"

// ─── Cấu hình server ──────────────────────────────────────────────────────────
constexpr char THINGSBOARD_SERVER[]  = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT  = 1883U;
constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

// ─── Interval các task ────────────────────────────────────────────────────────
constexpr uint32_t TELEMETRY_INTERVAL_MS  = 2500U;
constexpr uint32_t ATTRIBUTE_INTERVAL_MS  = 15000U;
constexpr uint32_t WIFI_CHECK_INTERVAL_MS = 5000U;
constexpr uint32_t MQTT_CHECK_INTERVAL_MS = 5000U;

// ─── MQTT client ──────────────────────────────────────────────────────────────
WiFiClient   wifiClient;
PubSubClient mqttClient(wifiClient);

// ─── Mutex bảo vệ mqttClient dùng chung giữa nhiều task ──────────────────────
static SemaphoreHandle_t xMqttMutex = NULL;

// ─────────────────────────────────────────────────────────────────────────────
// MQTT callback — nhận message từ server (RPC / command)
// ─────────────────────────────────────────────────────────────────────────────
void callback(char* topic, byte* payload, unsigned int length) {
    String message;
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("[MQTT] Message [");
    Serial.print(topic);
    Serial.print("]: ");
    Serial.println(message);
}

// ─────────────────────────────────────────────────────────────────────────────
// TASK 1 — Kiểm tra WiFi & reconnect
// ─────────────────────────────────────────────────────────────────────────────
void task_wifi_check(void *pvParameters) {
    for (;;) {
        if (WiFi.status() != WL_CONNECTED) {
            isWifiConnected = false;
            Serial.println("[WiFi] Disconnected — reconnecting...");

            WiFi.disconnect();
            WiFi.begin(wifi_ssid.c_str(), wifi_password.c_str());

            uint8_t retry = 0;
            while (WiFi.status() != WL_CONNECTED && retry < 20) {
                vTaskDelay(500 / portTICK_PERIOD_MS);
                Serial.print(".");
                retry++;
            }

            if (WiFi.status() == WL_CONNECTED) {
                isWifiConnected = true;
                Serial.println("\n[WiFi] Reconnected! IP: " + WiFi.localIP().toString());
            } else {
                Serial.println("\n[WiFi] Reconnect failed, retry later...");
            }
        } else {
            isWifiConnected = true;
        }

        vTaskDelay(WIFI_CHECK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TASK 2 — Kiểm tra kết nối CoreIOT, reconnect & subscribe lại
// ─────────────────────────────────────────────────────────────────────────────
void task_mqtt_connect(void *pvParameters) {
    for (;;) {
        // Chờ WiFi sẵn sàng trước
        if (!isWifiConnected) {
            vTaskDelay(MQTT_CHECK_INTERVAL_MS / portTICK_PERIOD_MS);
            continue;
        }

        if (!mqttClient.connected()) {
            Serial.println("[MQTT] Disconnected — reconnecting...");

            // Dùng MAC làm Client ID để tránh trùng
            String clientId = "ESP32-" + WiFi.macAddress();

            // Ưu tiên token từ file config, fallback về hardcode
            String token = CORE_IOT_TOKEN.isEmpty()
                           ? "zhbZ4lmpduD3T8h64M2H"
                           : CORE_IOT_TOKEN;

            if (xSemaphoreTake(xMqttMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
                bool ok = mqttClient.connect(clientId.c_str(), token.c_str(), NULL);
                xSemaphoreGive(xMqttMutex);

                if (ok) {
                    Serial.println("[MQTT] Connected!");
                    // Subscribe lại sau mỗi lần reconnect
                    mqttClient.subscribe("v1/devices/me/rpc/request/+");
                    Serial.println("[MQTT] Subscribed to RPC topic");
                } else {
                    Serial.print("[MQTT] Connect failed, rc=");
                    Serial.println(mqttClient.state());
                }
            }
        }

        vTaskDelay(MQTT_CHECK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TASK 3 — Gửi dữ liệu telemetry 
// ─────────────────────────────────────────────────────────────────────────────
void task_send_telemetry(void *pvParameters) {
    for (;;) {
        vTaskDelay(TELEMETRY_INTERVAL_MS / portTICK_PERIOD_MS);

        if (!isWifiConnected || !mqttClient.connected()) {
            Serial.println("[Telemetry] Skip — not connected");
            continue;
        }

        if (isnan(glob_temperature) || isnan(glob_humidity)) {
            Serial.println("[Telemetry] Skip — invalid sensor data");
            continue;
        }

        char payload[128];
        snprintf(payload, sizeof(payload),
            "{\"temperature\":%.2f,\"humidity\":%.2f}",
            glob_temperature, glob_humidity);

        Serial.print("[Telemetry] Publishing: ");
        Serial.println(payload);

        if (xSemaphoreTake(xMqttMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
            bool ok = mqttClient.publish("v1/devices/me/telemetry", payload);
            xSemaphoreGive(xMqttMutex);
            Serial.println(ok ? "[Telemetry] OK!" : "[Telemetry] FAILED!");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TASK 4 — Gửi dữ liệu attributes 
// ─────────────────────────────────────────────────────────────────────────────
void task_send_attributes(void *pvParameters) {
    for (;;) {
        vTaskDelay(ATTRIBUTE_INTERVAL_MS / portTICK_PERIOD_MS);

        if (!isWifiConnected || !mqttClient.connected()) {
            Serial.println("[Attributes] Skip — not connected");
            continue;
        }

        char attrPayload[256];
        snprintf(attrPayload, sizeof(attrPayload),
            "{\"rssi\":%d,\"channel\":%d,\"localIp\":\"%s\",\"ssid\":\"%s\",\"macAddress\":\"%s\"}",
            WiFi.RSSI(),
            WiFi.channel(),
            WiFi.localIP().toString().c_str(),
            WiFi.SSID().c_str(),
            WiFi.macAddress().c_str());

        Serial.print("[Attributes] Publishing: ");
        Serial.println(attrPayload);

        if (xSemaphoreTake(xMqttMutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
            bool ok = mqttClient.publish("v1/devices/me/attributes", attrPayload);
            xSemaphoreGive(xMqttMutex);
            Serial.println(ok ? "[Attributes] OK!" : "[Attributes] FAILED!");
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// TASK 5 — mqttClient.loop()
// ─────────────────────────────────────────────────────────────────────────────
void task_mqtt_loop(void *pvParameters) {
    for (;;) {
        if (isWifiConnected && mqttClient.connected()) {
            if (xSemaphoreTake(xMqttMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
                mqttClient.loop();
                xSemaphoreGive(xMqttMutex);
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// coreiot_init()
// ─────────────────────────────────────────────────────────────────────────────
void coreiot_init() {
    if (xMqttMutex == NULL) {
        xMqttMutex = xSemaphoreCreateMutex();
    }

    // 2. Setup MQTT
    mqttClient.setServer(THINGSBOARD_SERVER, THINGSBOARD_PORT);
    mqttClient.setCallback(callback);
}