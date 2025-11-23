#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "dht_anomaly_model.h"
#include "globals.h"
#include "sensors.h"
#include "tinyml.h"

namespace
{
    tflite::ErrorReporter* error_reporter = nullptr;
    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;

    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;

    constexpr int kTensorArenaSize = 8 * 1024;
    static uint8_t tensor_arena[kTensorArenaSize];
}

bool tinyml_init()
{
    // 1) Tạo MicroErrorReporter đúng chuẩn thư viện
    static tflite::MicroErrorReporter micro_error_reporter;
    error_reporter = &micro_error_reporter;

    // 2) Load model từ mảng trong dht_anomaly_model.h
    model = tflite::GetModel(dht_anomaly_model_tflite);

    // KHÔNG dùng TFLITE_SCHEMA_VERSION nữa để khỏi cần version.h

    // 3) Resolver & interpreter
    static tflite::AllOpsResolver resolver;

    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize, error_reporter);
    interpreter = &static_interpreter;

    // 4) Allocate tensors
    if (interpreter->AllocateTensors() != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
        return false;
    }

    input  = interpreter->input(0);
    output = interpreter->output(0);

    Serial.println("[TinyML] Initialized OK");
    return true;
}

// Hàm infer 1 lần
static float tinyml_predict(float temp, float hum)
{
    if (!interpreter) return NAN;

    input->data.f[0] = temp;
    input->data.f[1] = hum;

    if (interpreter->Invoke() != kTfLiteOk) {
        TF_LITE_REPORT_ERROR(error_reporter, "Invoke() failed");
        return NAN;
    }

    return output->data.f[0];
}

// Task cho xTaskCreate
void tiny_ml_task(void* pvParameters)
{
    (void)pvParameters;

    if (!tinyml_init()) {
        Serial.println("[TinyML] Init FAILED");
        vTaskDelete(NULL);
    }

   while (true) {
    SensorData_t data;
    if (sensor_get_latest(&data)) {
        float t = data.temperature;
        float h = data.humidity;

        float score = tinyml_predict(t, h);

        Serial.print("[TinyML] T="); Serial.print(t);
        Serial.print(" H="); Serial.print(h);
        Serial.print(" -> score=");
        Serial.println(score);
    } else {
        Serial.println("[TinyML] No sensor data yet");
    }

    vTaskDelay(pdMS_TO_TICKS(3000));
}
}
