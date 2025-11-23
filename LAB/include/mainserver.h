#ifndef ___MAIN_SERVER__
#define ___MAIN_SERVER__
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h> 
#include "global.h"

#define LED1_PIN 48
#define LED2_PIN 6

#define BOOT_PIN 0

// extern WebServer server;

//extern bool isAPMode;


extern bool led1_state;
extern bool led2_state;

String mainPage();
String settingsPage();

void startAP();
void setupServer();
void connectToWiFi();

void main_server_task(void *pvParameters);

#endif
