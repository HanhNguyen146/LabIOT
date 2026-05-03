#include <Arduino.h>
#include <Wire.h>
#include "shared_data.h"
#include "task_sensor.h"
#include "task_actuator.h"
#include "task_lcd.h"
#include "tinyml.h"
#include "task_webserver.h"
#include "coreiot.h"
#include "task_check_info.h"
#include "settingWifiAp.h"
#include "task_toogle_boot.h"
#include "system_config.h"
boolean isWifiConnected = false;

// Khởi tạo các biến cấu hình Cloud/WiFi
String WIFI_SSID = "";
String WIFI_PASS = "";
String CORE_IOT_TOKEN = "68PBxgP1uYZWp1Wk4zXE";
String CORE_IOT_SERVER = "app.coreiot.io";
String CORE_IOT_PORT = "1883";
String LOCAL_MQTT_BROKER_IP = "";
String LOCAL_MQTT_BROKER_PORT = "1883";

// Khởi tạo các Handle của FreeRTOS bằng NULL
EventGroupHandle_t xSystemEventGroup = NULL;
SemaphoreHandle_t xMutexCloudConfig = NULL;
SemaphoreHandle_t xMutexWifiConfig = NULL;

void setup()
{
    Serial.begin(115200);
    delay(5000); // Đợi Serial ổn định


    // 1. CẤP PHÁT BỘ NHỚ CHO CÁC ĐỐI TƯỢNG ĐỒNG BỘ FREERTOS
    xSystemEventGroup = xEventGroupCreate();
    xMutexCloudConfig = xSemaphoreCreateMutex();
    xMutexWifiConfig = xSemaphoreCreateMutex();

    // 2. Kiểm tra an toàn: Nếu cấp phát thất bại thì dừng hệ thống
    if (xSystemEventGroup == NULL || xMutexCloudConfig == NULL || xMutexWifiConfig == NULL) {
        Serial.println("LỖI NGHIÊM TRỌNG: Không thể cấp phát tài nguyên FreeRTOS!");
        while(1); // Dừng hệ thống tại đây
    }


    Wire.begin(GPIO_NUM_11, GPIO_NUM_12); 

    const bool hasSavedInfo = check_info_File(false);
    bool hasWifiCredential = false;
    String bootSsid;
    String bootPass;
    WifiGetCredentials(bootSsid, bootPass);
    hasWifiCredential = bootSsid.length() > 0;

    if (hasSavedInfo && hasWifiCredential) {
        Serial.println("[BOOT] Found saved STA credentials, trying WiFi...");
        InitWifi();
    } else {
        Serial.println("[BOOT] No STA credentials saved yet. Cloud publish is waiting for WiFi config.");
    }

    // 1. Cấp phát vùng nhớ cho Struct
    SensorData *sharedData = new SensorData();
    sharedData->temperature = 0.0f;
    sharedData->humidity = 0.0f;
    sharedData->lastSensorUpdateTick = xTaskGetTickCount();
    sharedData->currentLcdState = LCD_NORMAL;
    
    // Ngưỡng cấu hình động
    sharedData->tempWarning = 25.0f;
    sharedData->tempCritical = 35.0f;
    sharedData->humDry = 40.0f;
    sharedData->humDamp = 70.0f;
    sharedData->humCritical = 85.0f;
    // ---

    // --- KHỞI TẠO MANUAL LED OVERRIDE ---
    sharedData->manualLedOverride = false;
    sharedData->manualLedState = false;
    sharedData->lastManualLedTick = 0;
    sharedData->lastInferenceScore = 0.0f;
    // ---

    // === KHỞI TẠO SERVER CONTROL COMMAND ===
    sharedData->ledCommand.type = LED_CMD_NONE;
    sharedData->ledCommand.timestamp = 0;
    sharedData->neoCommand.type = NEO_CMD_NONE;
    sharedData->neoCommand.r = 0;
    sharedData->neoCommand.g = 0;
    sharedData->neoCommand.b = 0;
    sharedData->neoCommand.timestamp = 0;
    // ===

    sharedData->dataMutex = xSemaphoreCreateMutex();
    sharedData->i2cMutex = xSemaphoreCreateMutex();
    sharedData->tempWarningSemaphore = xSemaphoreCreateBinary();
    sharedData->lcdUpdateSemaphore = xSemaphoreCreateBinary();
    sharedData->ledCommandSemaphore = xSemaphoreCreateBinary();    // NEW: Server priority for LED
    sharedData->neoCommandSemaphore = xSemaphoreCreateBinary();    // NEW: Server priority for NEO
    // sharedData->tinymlInputQueue = xQueueCreate(1, sizeof(TinyMLInputSample));

    if (sharedData->dataMutex != NULL && sharedData->i2cMutex != NULL && 
        sharedData->ledCommandSemaphore != NULL && sharedData->neoCommandSemaphore != NULL) {
        xTaskCreate(Task_Toogle_BOOT, "Task_Toogle_BOOT", 4096, NULL, 2, NULL);
        // Task Sensor (Ưu tiên cao nhất để không lỡ nhịp dữ liệu)
        xTaskCreate(TaskSensor, "Sensor_Task", 4096, (void*)sharedData, 4, NULL);
        
        // Các Task điều khiển phần cứng (Ưu tiên trung bình) - LƯU HANDLE
        TaskHandle_t ledTaskHandle = NULL;
        TaskHandle_t neoTaskHandle = NULL;
        xTaskCreate(TaskLEDControl, "LED_Task", 2048, (void*)sharedData, 3, &ledTaskHandle);
        xTaskCreate(neo_blinky, "NeoPixel_Task", 2048, (void*)sharedData, 3, &neoTaskHandle);
        xTaskCreate(TaskLCD, "LCD_Task", 4096, (void*)sharedData, 3, NULL);
        
        // --- THÊM TASK 5: TINYML ANOMALY DETECTION ---
        // Cấp phát 8192 bytes RAM (TensorFlow ngốn khá nhiều RAM)
        // Mức ưu tiên thấp nhất (Priority 1) vì AI chạy tốn CPU, không được làm nghẽn các Task khác
        xTaskCreate(tiny_ml_task, "TinyML_Task", 8192, (void*)sharedData, 1, NULL);
        
        Serial.println("Tao Task thanh cong! He thong dang chay...");
    }

    InitWebServer((void *)sharedData);
    xTaskCreate(coreiot_task, "CoreIOT_Task", 8192, (void*)sharedData, 2, NULL);
}

void loop() {
    vTaskDelete(NULL); 
}
