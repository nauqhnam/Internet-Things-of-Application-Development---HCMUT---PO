#include "global.h"
float glob_temperature = 0;
float glob_humidity = 0;

String CORE_IOT_SERVER = "app.coreiot.io";
String CORE_IOT_PORT   = "1883";
String CORE_IOT_TOKEN  = "EYUvxEKglNiCRqOXX2Jb"; 

String ssid;
String password;

String WIFI_SSID ;
String WIFI_PASS ;
boolean isWifiConnected = false;

bool isAPMode = true;
bool connecting = false;


SemaphoreHandle_t xBinarySemaphoreInternet = xSemaphoreCreateBinary();
