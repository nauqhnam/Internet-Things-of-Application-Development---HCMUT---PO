#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

#include "globals.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "tinyml.h"
#include "webserver.h"

// ===== MQTT config =====
#define MQTT_HOST     "app.coreiot.io"
#define MQTT_PORT     1883
#define ACCESS_TOKEN  "73DIozIx4Af8lqecwzvP"

WiFiClient espClient;
PubSubClient mqtt(espClient);

// ===== MQTT reconnect =====
void mqttReconnect() {
  if (mqtt.connected()) return;

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[MQTT] WiFi (STA) chưa kết nối, bỏ qua reconnect");
    return;
  }

  Serial.print("[MQTT] Connecting ... ");
  if (mqtt.connect("ESP32_COREIOT_CLIENT", ACCESS_TOKEN, "")) {
    Serial.println("OK");
    mqtt.subscribe("v1/devices/me/rpc/request/+");
  } else {
    Serial.printf("fail rc=%d\n", mqtt.state());
  }
}

// ===== MQTT callback (RPC từ CoreIoT) =====
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String body;
  for (unsigned i = 0; i < length; i++) {
    body += (char)payload[i];
  }

  Serial.printf("[RPC] topic=%s body=%s\n", topic, body.c_str());

  // Lấy requestId từ topic: v1/devices/me/rpc/request/<id>
  String t = String(topic);
  int lastSlash = t.lastIndexOf('/');
  String reqId = (lastSlash >= 0) ? t.substring(lastSlash + 1) : "";
  String respTopic = "v1/devices/me/rpc/response/" + reqId;

  // ====== LED CONTROL ======
  if (body.indexOf("\"method\":\"led_force_on\"") >= 0) {
    led_force       = true;
    led_force_state = true;
    Serial.println("[RPC] led_force_on");

  } else if (body.indexOf("\"method\":\"led_force_off\"") >= 0) {
    led_force       = true;
    led_force_state = false;
    Serial.println("[RPC] led_force_off");

  } else if (body.indexOf("\"method\":\"Led_Default\"") >= 0) {
    led_force = false;   // quay về mode default (chớp 1s)
    Serial.println("[RPC] Led_Default");

  // ====== NEO PIXEL CONTROL ======
  } else if (body.indexOf("\"method\":\"neo_force_on\"") >= 0) {
    neo_force       = true;
    neo_force_state = true;
    Serial.println("[RPC] neo_force_on");

  } else if (body.indexOf("\"method\":\"neo_force_off\"") >= 0) {
    neo_force       = true;
    neo_force_state = false;
    Serial.println("[RPC] neo_force_off");

  } else if (body.indexOf("\"method\":\"Neo_Default\"") >= 0) {
    neo_force = false;   // quay về mode default theo cảm biến
    Serial.println("[RPC] Neo_Default");

  // ====== STATUS CHECK: widget "Led" hỏi trạng thái ======
  } else if (body.indexOf("\"method\":\"Led\"") >= 0) {
    // trả về true/false đơn giản, dashboard parse bằng "return data ? true : false;"
    String resp = led_on ? "true" : "false";
    mqtt.publish(respTopic.c_str(), resp.c_str());
    Serial.printf("[RPC] Led status -> %s\n", resp.c_str());

  // ====== STATUS CHECK: widget "Neo_pixel" ======
  } else if (body.indexOf("\"method\":\"Neo_pixel\"") >= 0) {
    String resp = neopixel_on ? "true" : "false";
    mqtt.publish(respTopic.c_str(), resp.c_str());
    Serial.printf("[RPC] Neo_pixel status -> %s\n", resp.c_str());

  // ====== (tuỳ chọn) giữ lại method cũ setState nếu bạn còn dùng ======
  } else if (body.indexOf("\"method\":\"setState\"") >= 0) {
    if (body.indexOf("\"params\":true") >= 0) {
      neo_force       = true;
      neo_force_state = true;
      Serial.println("[RPC] setState -> force ON");
    } else if (body.indexOf("\"params\":false") >= 0) {
      neo_force       = false;  // quay về default
      neo_force_state = false;
      Serial.println("[RPC] setState -> default");
    }
  }
}

// ===== Task gửi dữ liệu MQTT =====
void task_mqtt_publish(void* pv) {
  const TickType_t period = pdMS_TO_TICKS(1000);
  TickType_t last = xTaskGetTickCount();

  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[MQTT] Chờ WiFi STA (vào YoloUno-Setup để cấu hình)...");
    } else {
      mqttReconnect();
      mqtt.loop();

      bool forceOnTelemetry = (neo_force && neo_force_state);

      // ===== LẤY DỮ LIỆU CẢM BIẾN TỪ QUEUE, KHÔNG DÙNG glob_ NỮA =====
      SensorData_t data;
      float t = NAN;
      float h = NAN;

      if (sensor_get_latest(&data)) {
        t = data.temperature;
        h = data.humidity;
      }

      String payload = "{";

      // Nếu chưa có dữ liệu thì gửi null cho rõ ràng
      if (isnan(t)) {
        payload += "\"temperature\":null";
      } else {
        payload += "\"temperature\":" + String(t, 1);
      }

      if (isnan(h)) {
        payload += ",\"humidity\":null";
      } else {
        payload += ",\"humidity\":" + String(h, 1);
      }

      payload += ",\"force_on\":";    payload += forceOnTelemetry ? "true" : "false";
      payload += ",\"neopixel_on\":"; payload += neopixel_on ? "true" : "false";
      payload += ",\"led_on\":";      payload += led_on ? "true" : "false";
      payload += "}";

      mqtt.publish("v1/devices/me/telemetry", payload.c_str());
      Serial.printf("[MQTT] %s\n", payload.c_str());
    }

    vTaskDelayUntil(&last, period);
  }
}

void setup() {
  Serial.begin(115200);
  delay(200);

  // ====== TẠO QUEUE VÀ SEMAPHORE CHO SENSOR ======
  xSensorDataQueue = xQueueCreate(1, sizeof(SensorData_t));
  xSemLedUpdate = xSemaphoreCreateBinary();
  xSemNeoUpdate = xSemaphoreCreateBinary();

  xLcdUpdateSem    = xSemaphoreCreateBinary();        // cho LCD

  if (xSensorDataQueue == NULL || xSemLedUpdate == NULL || xSemNeoUpdate == NULL || xLcdUpdateSem == NULL) {
    Serial.println("[RTOS] Failed to create queue/semaphores!");
    while (1) { delay(1000); }
  }

// ====== TẠO TASK ======
xTaskCreate(TaskLEDControl,    "LED Control", 2048, NULL, 2, NULL);
xTaskCreate(temp_humi_monitor, "DHT20",       4096, NULL, 2, NULL);  
xTaskCreate(lcd_task,          "LCD",         4096, NULL, 2, NULL);  
xTaskCreate(neopixel_task,     "NEO",         4096, NULL, 2, NULL);
xTaskCreate(tiny_ml_task,      "TINY_ML",     2048, NULL, 2, NULL);

  // Webserver: AP+STA + portal
  web_server_init("", "");
  xTaskCreate(task_web_server, "WEB_SERVER", 8192, NULL, 1, NULL);

  // MQTT
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(mqttCallback);
  xTaskCreate(task_mqtt_publish, "MQTT", 4096, NULL, 2, NULL);
}

void loop() {}
