#ifndef __WEBSERVER_H__
#define __WEBSERVER_H__

#include <Arduino.h>

void web_server_init(const char* default_ssid, const char* default_pass);

void task_web_server(void* pvParameters);

#endif
