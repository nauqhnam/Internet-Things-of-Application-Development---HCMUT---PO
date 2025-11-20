#ifndef __NEO_BLINKY__
#define __NEO_BLINKY__
#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include "global.h"


// #include "LiquidCrystal_I2C.h"
#include "DHT20.h"

void temp_humi_monitor(void *pvParameters);


#define NEO_PIN 45
#define LED_COUNT 1 

#define LED_GPIO 48
void led_blinky(void *pvParameters);

void neo_blinky(void *pvParameters);


#endif
