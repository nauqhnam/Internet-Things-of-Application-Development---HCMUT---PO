#include "sensors.h"
#include "globals.h"

DHT20 dht20;
LiquidCrystal_I2C lcd(0x21,16,2);
Adafruit_NeoPixel rgb(NEOPIXEL_COUNT, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// Ngưỡng
#define TEMP_COOL_MAX   25.0f
#define TEMP_WARM_MAX   30.0f
#define HUMI_DRY_MAX    45.0f
#define HUMI_NORMAL_MAX 60.0f


void TaskLEDControl(void *pvParameters) {
  pinMode(GPIO_NUM_48, OUTPUT);

  const TickType_t baseTick = pdMS_TO_TICKS(100);
  TickType_t last = xTaskGetTickCount();

  int  ticksPerToggle = 10;
  bool ledState       = false;

  while (1) {
    // ===== Cập nhật mode theo nhiệt độ =====
    if (xSemLedUpdate != NULL &&
        xSemaphoreTake(xSemLedUpdate, pdMS_TO_TICKS(5)) == pdTRUE) {

      SensorData_t data;
      if (xSensorDataQueue != NULL &&
          xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {

        float t = data.temperature;

        if (t < TEMP_COOL_MAX)      ticksPerToggle = 15;
        else if (t < TEMP_WARM_MAX) ticksPerToggle = 5;
        else                        ticksPerToggle = 1;

        Serial.print("[LED] New T="); Serial.print(t);
        Serial.print(" => ticksPerToggle="); Serial.println(ticksPerToggle);
      }
    }

    // ===== Force từ web =====
    if (led_force) {
      digitalWrite(GPIO_NUM_48, led_force_state ? HIGH : LOW);
      led_on = led_force_state;
      vTaskDelayUntil(&last, baseTick);
      continue;
    }

    // ===== Blink LED =====
    static int tickCount = 0;
    tickCount++;

    if (tickCount >= ticksPerToggle) {
      tickCount = 0;
      ledState = !ledState;
      digitalWrite(GPIO_NUM_48, ledState ? HIGH : LOW);
      led_on = ledState;
    }

    vTaskDelayUntil(&last, baseTick);
  }
}




void temp_humi_monitor(void *pvParameters){
    Wire.begin(11, 12);
    dht20.begin();
    lcd.init();
    lcd.backlight();

    while (1){
        dht20.read();
        float temperature = dht20.getTemperature();
        float humidity    = dht20.getHumidity();

        if (isnan(temperature) || isnan(humidity)) {
            Serial.println("Failed to read from DHT sensor!");
            temperature = humidity = -1;
        }

        // ====== Đưa dữ liệu vào queue (KHÔNG dùng global nữa) ======
        if (xSensorDataQueue != NULL) {
            SensorData_t data;
            data.temperature = temperature;
            data.humidity    = humidity;
            xQueueOverwrite(xSensorDataQueue, &data);
        }

        // ====== Phát semaphore cho Task1 (LED) + Task2 (NeoPixel) ======
        if (xSemLedUpdate != NULL)   xSemaphoreGive(xSemLedUpdate);
        if (xSemNeoUpdate != NULL)   xSemaphoreGive(xSemNeoUpdate);


        // ====== Phát semaphore cho LCD (Task LCD riêng) ======
        if (xLcdUpdateSem != NULL) {
            xSemaphoreGive(xLcdUpdateSem);
        }

        // (LCD không vẽ ở đây nữa)
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}
void lcd_task(void *pvParameters) {
    char lineBuf[32];

    while (1) {
        // Chờ DHT task báo có dữ liệu mới
        if (xLcdUpdateSem != NULL &&
            xSemaphoreTake(xLcdUpdateSem, portMAX_DELAY) == pdTRUE) {

            SensorData_t data;
            float temperature = NAN;
            float humidity    = NAN;

            if (xSensorDataQueue != NULL &&
                xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {
                temperature = data.temperature;
                humidity    = data.humidity;
            }

            // ====== Tính trạng thái NORMAL / WARNING / CRITICAL ======
            const char* stateStr = "NORMAL";

            if (isnan(temperature) || isnan(humidity)) {
                stateStr = "CRITICAL";   // lỗi đọc sensor => coi như nguy hiểm
            } else if ((temperature >= 20 && temperature <= 30) &&
                       (humidity    >= 40 && humidity    <= 60)) {
                stateStr = "NORMAL";
            } else if ((temperature >= 15 && temperature <= 35) &&
                       (humidity    >= 30 && humidity    <= 70)) {
                stateStr = "WARNING";
            } else {
                stateStr = "CRITICAL";
            }

            // ====== Vẽ LCD ======
            lcd.clear();
            lcd.setCursor(0, 0);
            snprintf(lineBuf, sizeof(lineBuf),
                     "T:%4.1fC H:%4.1f%%", temperature, humidity);
            lcd.print(lineBuf);

            lcd.setCursor(0, 1);
            lcd.print("State: ");
            lcd.print(stateStr);

            Serial.print("[LCD] T="); Serial.print(temperature);
            Serial.print(" H="); Serial.print(humidity);
            Serial.print("  LCD State="); Serial.println(stateStr);
        }
    }
}


void neopixel_task(void *pvParameters) {
  rgb.begin();
  rgb.clear();
  rgb.show();
  pinMode(INDICATOR_LED, OUTPUT);

  while (1) {
    // Chờ có mẫu mới (semaphore)
    if (xSemNeoUpdate != NULL &&
        xSemaphoreTake(xSemNeoUpdate , portMAX_DELAY) == pdTRUE) {

      // ===== OVERRIDE TỪ WEB/MQTT: neo_force =====
      if (neo_force) {
        if (neo_force_state) {
          // Bật NeoPixel màu trắng dịu
          uint32_t color = rgb.Color(50, 50, 50);
          for (int i = 0; i < NEOPIXEL_COUNT; i++) {
            rgb.setPixelColor(i, color);
          }
          rgb.show();
          neopixel_on = true;
          digitalWrite(INDICATOR_LED, HIGH);
        } else {
          // Tắt NeoPixel
          rgb.clear();
          rgb.show();
          neopixel_on = false;
          digitalWrite(INDICATOR_LED, LOW);
        }

        Serial.printf("[NEO] Forced from WEB/MQTT -> %s\n",
                      neo_force_state ? "ON" : "OFF");
        continue;   // bỏ qua chế độ theo độ ẩm
      }

      // ===== CHẾ ĐỘ TỰ ĐỘNG THEO ĐỘ ẨM =====
      SensorData_t data;
      if (xSensorDataQueue != NULL &&
          xQueuePeek(xSensorDataQueue, &data, 0) == pdTRUE) {

        float h = data.humidity;
        uint32_t color;

        if (h < HUMI_DRY_MAX) {
          // Khô -> Xanh dương
          color = rgb.Color(0, 0, 50);
        } else if (h < HUMI_NORMAL_MAX) {
          // Bình thường -> Xanh lá
          color = rgb.Color(0, 50, 0);
        } else {
          // Ẩm cao -> Đỏ
          color = rgb.Color(50, 0, 0);
        }

        for (int i = 0; i < NEOPIXEL_COUNT; i++) {
          rgb.setPixelColor(i, color);
        }
        rgb.show();

        neopixel_on = true;
        digitalWrite(INDICATOR_LED, HIGH);

        Serial.print("[NEO] H="); Serial.print(h);
        Serial.print(" -> color level (0:dry,1:normal,2:wet) = ");
        if (h < HUMI_DRY_MAX) Serial.println(0);
        else if (h < HUMI_NORMAL_MAX) Serial.println(1);
        else Serial.println(2);
      }
    }
  }
}


bool sensor_get_latest(SensorData_t* out) {
  if (xSensorDataQueue == NULL) return false;
  if (xQueuePeek(xSensorDataQueue, out, 0) == pdTRUE) {
    return true;
  }
  return false;
}





