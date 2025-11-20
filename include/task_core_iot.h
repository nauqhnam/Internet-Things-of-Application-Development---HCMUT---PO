#ifndef __TASK_CORE_IOT_H__
#define __TASK_CORE_IOT_H__

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <global.h>
extern bool isWifiConnected;
extern SemaphoreHandle_t xBinarySemaphoreInternet;

void coreiot_task(void *pvParameters);
#endif
