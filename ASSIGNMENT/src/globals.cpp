#include "globals.h"

// KHÔNG còn glob_temperature, glob_humidity ở đây nữa

QueueHandle_t xSensorDataQueue = nullptr;
SemaphoreHandle_t xSemLedUpdate = nullptr;
SemaphoreHandle_t xSemNeoUpdate = nullptr;

SemaphoreHandle_t xLcdUpdateSem = nullptr;

// Các flag cũ (nếu vẫn cần cho Web/MQTT…)
volatile bool force_on = false;
volatile bool neopixel_on = false;

volatile bool led_on          = false;
volatile bool led_force       = false;
volatile bool led_force_state = false;

volatile bool neo_force       = false;
volatile bool neo_force_state = false;
