#ifndef __SENSORS__
#define __SENSORS__
#include "LiquidCrystal_I2C.h"
#include "DHT20.h"
#include <Adafruit_NeoPixel.h>
#include "globals.h"

#define LIGHT_SENSOR_PIN 1   // cảm biến ánh sáng nối vào A0
#define NEOPIXEL_PIN     6      // D3 thực tế là GPIO4
#define NEOPIXEL_COUNT   4      // Số lượng đèn RGB NeoPixel
#define LIGHT_THRESHOLD  1000 // ngưỡng sáng (tuỳ môi trường)
#define INDICATOR_LED 2   // LED báo NeoPixel đang bật (LED tích hợp trên Yolo Uno)
bool sensor_get_latest(SensorData_t* out);
void TaskLEDControl(void *pvParameters);
void temp_humi_monitor(void *pvParameters);
void light_sensor(void *pvParameters);
void neopixel_task(void *pvParameters);
void tiny_ml_task(void *pvParameters);
void lcd_task(void *pvParameters);

#endif