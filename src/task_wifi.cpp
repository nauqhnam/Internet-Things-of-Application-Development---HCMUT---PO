#include "task_wifi.h"
#include "global.h"

extern SemaphoreHandle_t xBinarySemaphoreInternet;
extern String WIFI_SSID;
extern String WIFI_PASS;
extern bool isWifiConnected;
extern bool isAPMode;

static bool semaphoreGiven = false;

void startAP() {
    Serial.println(F("[WiFi] -> startAP()"));

    WiFi.mode(WIFI_AP);
    WiFi.softAP(AP_SSID,AP_PASS);

    IPAddress ip = WiFi.softAPIP();
    Serial.print(F("[WiFi] AP IP: "));
    Serial.println(ip);

    isAPMode        = true;
    isWifiConnected = false;
    connecting      = 0;
}

void startSTA() {
    Serial.println(F("[WiFi] -> startSTA()"));

    if (WIFI_SSID.isEmpty()) {
        Serial.println(F("[WiFi] Không có WIFI_SSID, bỏ qua startSTA."));
        return; 
    }

    Serial.print(F("[WiFi] Đang kết nối tới SSID: "));
    Serial.println(WIFI_SSID);

    WiFi.mode(WIFI_STA);

    if (WIFI_PASS.isEmpty()) {
        Serial.println(F("[WiFi] WiFi không mật khẩu."));
        WiFi.begin(WIFI_SSID.c_str());
    } else {
        WiFi.begin(WIFI_SSID.c_str(), WIFI_PASS.c_str());
    }

    connecting = 1;
    unsigned long t0 = millis();
    const unsigned long timeoutMs = 15000; // 15s

    while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) {
        Serial.print(F("."));
        delay(500);
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("[WiFi] KẾT NỐI THÀNH CÔNG!"));
        Serial.print(F("[WiFi] IP STA: "));
        Serial.println(WiFi.localIP());

        isWifiConnected = true;
        isAPMode        = false;
        connecting      = 0;

        if (xBinarySemaphoreInternet && !semaphoreGiven) {
            xSemaphoreGive(xBinarySemaphoreInternet);
            semaphoreGiven = true;
            Serial.println(F("[WiFi] Đã give xBinarySemaphoreInternet"));
        }
    } else {
        Serial.println(F("[WiFi] KẾT NỐI THẤT BẠI. Vẫn giữ AP nếu có."));
        isWifiConnected = false;
        connecting      = 0;
    }
}

bool Wifi_reconnect() {
    wl_status_t st = WiFi.status();

    // debug trạng thái thô
    Serial.print(F("[WiFi] Wifi_reconnect() status="));
    Serial.println((int)st);

    // Nếu đang có kết nối STA
    if (st == WL_CONNECTED) {
        if (!isWifiConnected) {
            // mới vừa có kết nối
            Serial.println(F("[WiFi] Vừa kết nối lại thành công."));
            isWifiConnected = true;
            isAPMode        = (WiFi.getMode() & WIFI_AP) != 0;
            connecting      = 0;

            if (xBinarySemaphoreInternet && !semaphoreGiven) {
                xSemaphoreGive(xBinarySemaphoreInternet);
                semaphoreGiven = true;
                Serial.println(F("[WiFi] (reconnect) Đã give xBinarySemaphoreInternet"));
            }
        }
        return true;
    }

    // Mất kết nối / chưa kết nối
    if (isWifiConnected) {
        Serial.println(F("[WiFi] MẤT KẾT NỐI!"));
        isWifiConnected = false;
    }

    // Chưa kết nối, chưa thử -> thử STA nếu có cấu hình
    if (!connecting && !WIFI_SSID.isEmpty()) {
        startSTA();
    }

    return false;
}
