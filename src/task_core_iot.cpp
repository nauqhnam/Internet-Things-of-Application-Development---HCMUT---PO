#include <WiFi.h>
#include <PubSubClient.h>
#include "task_core_iot.h"



WiFiClient espClient;
PubSubClient mqtt(espClient);

// ====== hàm connect MQTT ======
bool mqttReconnect() {
  if (mqtt.connected()) return true;

  if (CORE_IOT_SERVER.isEmpty() || CORE_IOT_TOKEN.isEmpty() || CORE_IOT_PORT.isEmpty()) {
    Serial.println("[MQTT] Missing CORE_IOT config, skip.");
    return false;
  }

  Serial.printf("[MQTT] Connecting to %s:%s ...\n",
                CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.c_str());

  // clientId: tuỳ, username = token, password = ""
  if (mqtt.connect("ESP32_COREIOT_CLIENT",
                   CORE_IOT_TOKEN.c_str(),
                   "")) {
    Serial.println("[MQTT] Connected.");
    mqtt.subscribe("v1/devices/me/rpc/request/+"); 
    return true;
  } else {
    Serial.printf("[MQTT] Connect fail, rc=%d\n", mqtt.state());
    return false;
  }
}

// Callback nếu muốn xử lý RPC từ CoreIOT
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String body;
  body.reserve(length);
  for (unsigned i = 0; i < length; i++) body += (char)payload[i];

  Serial.printf("[RPC] %s\n", body.c_str());

  
  // ví dụ: bật/tắt led1_state, led2_state
}

// ====== coreiot_task chính ======
void coreiot_task(void *pvParameters)
{
  // Đợi WiFi ready từ task WiFi 
  Serial.println("[CoreIOT] Waiting for Internet...");
  xSemaphoreTake(xBinarySemaphoreInternet, portMAX_DELAY);
  Serial.println("[CoreIOT] Internet ready, start MQTT");

  // set server 1 lần
  mqtt.setServer(CORE_IOT_SERVER.c_str(), CORE_IOT_PORT.toInt());
  mqtt.setCallback(mqttCallback);

  const TickType_t period = pdMS_TO_TICKS(10000); // 10s
  TickType_t lastWake = xTaskGetTickCount();

  for (;;) {
    if (!isWifiConnected) {
      vTaskDelay(pdMS_TO_TICKS(1000));
      continue;
    }

    if (!mqttReconnect()) {
      vTaskDelay(pdMS_TO_TICKS(3000));
      continue;
    }

    mqtt.loop();  // xử lý incoming messages

   
    char buf[128];
    snprintf(buf, sizeof(buf),
             "{\"temperature\":%.2f,\"humidity\":%.2f}",
             glob_temperature, glob_humidity);

    mqtt.publish("v1/devices/me/telemetry", buf, true);
    Serial.printf("[MQTT] %s\n", buf);

    vTaskDelayUntil(&lastWake, period);
  }
}
