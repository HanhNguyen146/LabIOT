#include "tinyml.h"
#include "shared_data.h"

// === TinyML globals ===
namespace
{
    tflite::ErrorReporter *error_reporter = nullptr;
    const tflite::Model *model = nullptr;
    tflite::MicroInterpreter *interpreter = nullptr;
    TfLiteTensor *input = nullptr;
    TfLiteTensor *output = nullptr;

    constexpr int kTensorArenaSize = 8 * 1024;
    uint8_t tensor_arena[kTensorArenaSize];
}

// === setup ===
void setupTinyML()
{
    Serial.println("TensorFlow Lite Init...");

    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    model = tflite::GetModel(TinyML_model);

    if (model->version() != TFLITE_SCHEMA_VERSION)
    {
        error_reporter->Report("Model version mismatch!");
        return;
    }

    static tflite::AllOpsResolver resolver;

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);

    interpreter = &static_interpreter;

    if (interpreter->AllocateTensors() != kTfLiteOk)
    {
        error_reporter->Report("AllocateTensors() failed");
        return;
    }

    input = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("TinyML model loaded successfully!");
}

// === RTOS TASK ===
void tiny_ml_task(void *pvParameters)
{
    SensorData *data = (SensorData *)pvParameters;

    setupTinyML();

    while (1)
    {
        float temp = 0.0f;
        float hum = 0.0f;
        uint32_t lastUpdate = 0;

        if (xSemaphoreTake(data->dataMutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            temp = data->temperature;
            hum = data->humidity;
            lastUpdate = data->lastSensorUpdateTick;
            xSemaphoreGive(data->dataMutex);
        }

        // vẫn giữ logic an toàn sensor
        if ((xTaskGetTickCount() - lastUpdate) < pdMS_TO_TICKS(5000))
        {
            float norm_temp = temp / 40.0f;
            float norm_humi = hum / 100.0f;

            input->data.f[0] = norm_temp;
            input->data.f[1] = norm_humi;

            TfLiteStatus status = interpreter->Invoke();

            if (status != kTfLiteOk)
            {
                Serial.println("Invoke failed");
            }
            else
            {
                float last_inference = output->data.f[0];

                // 🔥 GIỮ NGUYÊN FULL SERIAL OUTPUT NHƯ BẢN CŨ
                Serial.printf(
                    "[TinyML] Input (scaled): T=%.2f, H=%.2f → Output=%.3f\n",
                    norm_temp, norm_humi, last_inference);

                if (last_inference > 0.8)
                    Serial.println("AI dự đoán: Nguy hiểm!");
                else if (last_inference > 0.6)
                    Serial.println("AI dự đoán: Cảm giác khó chịu");
                else
                    Serial.println("AI dự đoán: Bình thường");
            }
        }

        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}
// === TASK TEST / EVALUATION ===
void evaluation_task(void *pvParameters)
{
    SensorData *data = (SensorData *)pvParameters;

    setupTinyML();
    Serial.println("ESP32 Tester Ready. Waiting for commands");

    while (1)
    {
        if (Serial.available() > 0)
        {
            String inputStr = Serial.readStringUntil('\n');

            float temp, humi;

            if (sscanf(inputStr.c_str(), "%f,%f", &temp, &humi) == 2)
            {
                // === Normalize giống training ===
                float temp_scaled = temp / 40.0f;
                float humi_scaled = humi / 100.0f;

                input->data.f[0] = temp_scaled;
                input->data.f[1] = humi_scaled;

                if (interpreter->Invoke() == kTfLiteOk)
                {
                    float score = output->data.f[0];

                    // === LOG GIỐNG BẢN CŨ ===
                    Serial.printf(
                        "[EVAL TinyML] T=%.2f H=%.2f → Score=%.3f\n",
                        temp_scaled, humi_scaled, score);

                    if (score > 0.8f)
                    {
                        Serial.println("AI dự đoán: Nguy hiểm!");
                    }
                    else if (score > 0.6f)
                    {
                        Serial.println("AI dự đoán: Cảm giác khó chịu");
                    }
                    else
                    {
                        Serial.println("AI dự đoán: Bình thường");
                    }

                    // === (OPTION) cập nhật vào system nếu cần ===
                    if (xSemaphoreTake(data->dataMutex, pdMS_TO_TICKS(10)) == pdTRUE)
                    {
                        data->lastInferenceScore = score;   // nếu bạn có field này
                        xSemaphoreGive(data->dataMutex);
                    }
                }
                else
                {
                    Serial.println("Invoke failed");
                }
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}