print("Hello Core IOT")
import paho.mqtt.client as mqttclient
import time
import json

BROKER_ADDRESS = "app.coreiot.io"
PORT = 1883
ACCESS_TOKEN = "8JFElEYTWYZzJzFMbk6m"
ACCESS_USERNAME = "device_lab1"

def subscribed(client, userdata, mid, granted_qos):
    print("Subscribed...")


def recv_message(client, userdata, message):
    print("Received: ", message.payload.decode("utf-8"))
    temp_data = {'value': True}
    try:
        jsonobj = json.loads(message.payload)
        if jsonobj['method'] == "setValue":
            temp_data['value'] = jsonobj['params']
            client.publish('esp/telemetry', json.dumps(temp_data), 1)
    except:
        pass


def connected(client, usedata, flags, rc):
    if rc == 0:
        print("Connected successfully!!")
        client.subscribe("esp/telemetry")
    else:
        print("Connection is failed")


client = mqttclient.Client("Sensor C1")
client.username_pw_set(ACCESS_TOKEN)

client.on_connect = connected
client.connect(BROKER_ADDRESS, 1883)
client.loop_start()

client.on_subscribe = subscribed
client.on_message = recv_message

temp = 30
humi = 50
light_intesity = 100
counter = 0
#HCMUT
long = 106.65789107082472
lat = 10.772175109674038

#H6
long = 106.80633605864662
lat = 10.880018410410052


while True:
    collect_data = {'temperature': temp, 'humidity': humi,
                    'light':light_intesity,
                    'long': long, 'lat': lat}
    temp += 1
    humi += 1
    light_intesity += 1
    client.publish('#define LED_PIN 48
#define SDA_PIN GPIO_NUM_11
#define SCL_PIN GPIO_NUM_12

#include <WiFi.h>
#include <PubSubClient.h>
#include "DHT20.h"
#include "Wire.h"

constexpr char WIFI_SSID[] = "DOAN-HOI CSE";
constexpr char WIFI_PASSWORD[] = "Doanhoilanha";

constexpr char TOKEN[] = "8JFElEYTWYZzJzFMbk6m";

constexpr char THINGSBOARD_SERVER[] = "app.coreiot.io";
constexpr uint16_t THINGSBOARD_PORT = 1883U;

constexpr uint32_t SERIAL_DEBUG_BAUD = 115200U;

constexpr char LED_STATE_ATTR[] = "ledState";

volatile bool attributesChanged = false;
volatile bool ledState = false;

constexpr int16_t telemetrySendInterval = 10000U;
uint32_t previousDataSend;

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

DHT20 dht20;

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  String message;
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println(message);
}

void InitWiFi() {
  Serial.println("Connecting to AP ...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to AP");
}

const bool reconnect() {
  const wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    return true;
  }
  InitWiFi();
  return true;
}

void connectMQTT() {
  while (!mqttClient.connected()) {
    Serial.println("Connecting MQTT...");
    if (mqttClient.connect("ESP32_Device", TOKEN, "")) {
      Serial.println("MQTT Connected!");
      mqttClient.subscribe("v1/devices/me/rpc/request/+");
    } else {
      Serial.print("Failed, rc=");
      Serial.println(mqttClient.state());
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(SERIAL_DEBUG_BAUD);
  pinMode(LED_PIN, OUTPUT);
  delay(1000);
  InitWiFi();

  mqttClient.setServer(THINGSBOARD_SERVER, THINGSBOARD_PORT);
  mqttClient.setCallback(callback);

  Wire.begin(SDA_PIN, SCL_PIN);
  delay(100);
  dht20.begin();
}

void loop() {
  delay(10);

  if (!reconnect()) return;

  if (!mqttClient.connected()) {
    connectMQTT();
  }
  mqttClient.loop();

  if (millis() - previousDataSend > telemetrySendInterval) {
    previousDataSend = millis();

    dht20.read();
    float temperature = dht20.getTemperature();
    float humidity = dht20.getHumidity();

    if (isnan(temperature) || isnan(humidity)) {
      Serial.println("Failed to read from DHT20 sensor!");
    } else {
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print(" °C, Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      // Gửi telemetry giống Python publish
      char payload[128];
      snprintf(payload, sizeof(payload),
        "{\"temperature\":%.2f,\"humidity\":%.2f}",
        temperature, humidity);

      Serial.print("Publishing: ");
      Serial.println(payload);

      bool ok = mqttClient.publish("v1/devices/me/telemetry", payload);
      Serial.println(ok ? "Publish OK!" : "Publish FAILED!");
    }

    // Gửi attributes
    char attrPayload[256];
    snprintf(attrPayload, sizeof(attrPayload),
      "{\"rssi\":%d,\"channel\":%d,\"localIp\":\"%s\",\"ssid\":\"%s\",\"macAddress\":\"%s\"}",
      WiFi.RSSI(), WiFi.channel(),
      WiFi.localIP().toString().c_str(),
      WiFi.SSID().c_str(),
      WiFi.macAddress().c_str());
    mqttClient.publish("v1/devices/me/attributes", attrPayload);
  }
}', json.dumps(collect_data), 1)
    time.sleep(5)