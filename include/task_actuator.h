#ifndef TASK_ACTUATOR_H
#define TASK_ACTUATOR_H

#include <Arduino.h>
#include "shared_data.h"

#define NEO_PIN 45
#define LED_COUNT 1

void TaskLEDControl(void *pvParameters);
void neo_blinky(void *pvParameters);

// === SERVER CONTROL FUNCTIONS ===
void sendLedCommand(SensorData* data, LedCommandType cmd);
void sendNeoCommand(SensorData* data, NeoCommandType cmd, uint8_t r = 0, uint8_t g = 0, uint8_t b = 0);

#endif
