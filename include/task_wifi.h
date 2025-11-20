#ifndef __TASK_WIFI_H__
#define __TASK_WIFI_H__

#include <WiFi.h>
#include <task_check_info.h>
#include <task_webserver.h>



void startAP();
void startSTA();
bool Wifi_reconnect();

#define AP_SSID "ESP32-SETUP"
#define AP_PASS "12345678"

#endif
