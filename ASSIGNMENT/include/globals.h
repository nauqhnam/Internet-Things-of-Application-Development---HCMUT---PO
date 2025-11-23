#ifndef __GLOBALS_H__
#define __GLOBALS_H__

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

// ================== CẤU TRÚC DỮ LIỆU CẢM BIẾN ==================
typedef struct {
  float temperature;
  float humidity;
} SensorData_t;

// ================== RTOS OBJECTS DÙNG CHUNG ==================
// Queue chứa giá trị cảm biến mới nhất (size = 1, luôn overwrite)
extern QueueHandle_t xSensorDataQueue;

// Counting semaphore cho Task1 (LED) + Task2 (NeoPixel)
// Mỗi lần có mẫu mới: DHT task give 2 lần -> 2 consumer cùng được đánh thức
extern SemaphoreHandle_t xSemLedUpdate;
extern SemaphoreHandle_t xSemNeoUpdate;

// Binary semaphore cho Task3 (LCD): mỗi mẫu mới give 1 lần
extern SemaphoreHandle_t xLcdUpdateSem;

// (Nếu bạn vẫn muốn giữ mấy flag này cho web/MQTT thì tạm giữ)
// Nhưng với Task 1–3, mình KHÔNG cần mấy global này nữa.
extern volatile bool force_on;
extern volatile bool neopixel_on;

// LED
extern volatile bool led_on;
extern volatile bool led_force;
extern volatile bool led_force_state;

// NeoPixel
extern volatile bool neo_force;
extern volatile bool neo_force_state;

#endif
