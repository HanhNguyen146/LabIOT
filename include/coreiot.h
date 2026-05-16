#ifndef __COREIOT_H__
#define __COREIOT_H__

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "global.h"

// MQTT callback — nhận message từ server
void callback(char* topic, byte* payload, unsigned int length);

// Các task — spawn tự động bởi coreiot_init(), không cần gọi trực tiếp
void task_wifi_check     (void *pvParameters);
void task_mqtt_connect   (void *pvParameters);
void task_send_telemetry (void *pvParameters);
void task_send_attributes(void *pvParameters);
void task_mqtt_loop      (void *pvParameters);

// Gọi hàm này trong setup() thay cho xTaskCreate(coreiot_task, ...)
void coreiot_init();

#endif