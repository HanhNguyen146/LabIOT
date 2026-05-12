#ifndef SHARED_DATA_H
#define SHARED_DATA_H

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/queue.h>

enum LcdDisplayState {
    LCD_NORMAL,
    LCD_WARNING,
    LCD_CRITICAL,
    LCD_ERROR
};

// === LED CONTROL ENUMS ===
enum LedCommandType {
    LED_CMD_NONE,
    LED_CMD_ON,
    LED_CMD_OFF,
    LED_CMD_AUTO  // Return to FSM control
};

enum NeoCommandType {
    NEO_CMD_NONE,
    NEO_CMD_CUSTOM_COLOR,  // Set custom color
    NEO_CMD_AUTO           // Return to humidity-based control
};

// === SERVER CONTROL STRUCTURES ===
struct LedServerCommand {
    LedCommandType type;
    uint32_t timestamp;  // For timeout detection
};

struct NeoServerCommand {
    NeoCommandType type;
    uint8_t r, g, b;     // Custom color if type == NEO_CMD_CUSTOM_COLOR
    uint32_t timestamp;  // For timeout detection
};

// struct TinyMLInputSample {
//     float temperature;
//     float humidity;
//     uint32_t tick;
// };

struct SensorData {
    float temperature;
    float humidity;

    uint32_t lastSensorUpdateTick;
    LcdDisplayState currentLcdState; 

    // --- THÊM: CÁC NGƯỠNG CÀI ĐẶT ĐỘNG (DYNAMIC THRESHOLDS) ---
    float tempWarning;   // Ngưỡng cảnh báo nhiệt độ
    float tempCritical;  // Ngưỡng nguy hiểm nhiệt độ
    float humDry;        // Ngưỡng độ ẩm khô hanh
    float humDamp;       // Ngưỡng độ ẩm ẩm ướt
    float humCritical;   // Ngưỡng độ ẩm gây ngạt

    // --- MANUAL LED OVERRIDE (từ RPC/Dashboard) ---
    bool manualLedOverride;      // Cờ cho biết có override từ RPC không
    bool manualLedState;         // Trạng thái LED khi override (ON/OFF)
    uint32_t lastManualLedTick;  // Timestamp lần cuối RPC gửi lệnh (timeout 10s)
    float lastInferenceScore;

    // === SERVER CONTROL VÀ PRIORITY SEMAPHORE ===
    LedServerCommand ledCommand;
    NeoServerCommand neoCommand;
    SemaphoreHandle_t ledCommandSemaphore;   // Signal LED task khi có lệnh từ server
    SemaphoreHandle_t neoCommandSemaphore;   // Signal NeoPixel task khi có lệnh từ server
    const uint32_t SERVER_CMD_TIMEOUT_MS = 30000;  // 30s timeout cho lệnh server

    SemaphoreHandle_t dataMutex; 
    SemaphoreHandle_t i2cMutex;
    SemaphoreHandle_t tempWarningSemaphore; 
    SemaphoreHandle_t lcdUpdateSemaphore; 
    // QueueHandle_t tinymlInputQueue;
};

#endif
