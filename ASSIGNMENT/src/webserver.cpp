#include "webserver.h"
#include <WiFi.h>
#include <ESP32WebServer.h>
#include <Preferences.h>
#include "globals.h"
#include "sensors.h"

// Web server HTTP port 80
static ESP32WebServer server(80);
static Preferences prefs;

// Thông tin WiFi STA lưu trong NVS
static String sta_ssid = "";
static String sta_pass = "";

// ======================= NVS: LƯU / ĐỌC WI-FI =======================

void loadCredentials() {
  prefs.begin("wifi", true);
  sta_ssid = prefs.getString("ssid", "");
  sta_pass = prefs.getString("pass", "");
  prefs.end();

  Serial.println("[WEB] Loaded STA WiFi:");
  Serial.println("  SSID = " + sta_ssid);
}

void saveCredentials(const String& ssid, const String& pass) {
  prefs.begin("wifi", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();

  Serial.println("[WEB] Saved STA WiFi:");
  Serial.println("  SSID = " + ssid);
}

// ======================= KẾT NỐI STA =======================

void tryConnectSTA() {
  if (sta_ssid == "") {
    Serial.println("[WEB] No STA SSID stored, skip STA connect");
    return;
  }

  Serial.println("[WEB] Connecting STA WiFi...");
  WiFi.begin(sta_ssid.c_str(), sta_pass.c_str());

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WEB] STA connected, IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WEB] STA connect failed");
  }
}

// ======================= API JSON CHO AJAX =======================
//
// Trả về:
//  {
//    "temperature": 27.3 or null,
//    "humidity":  61.2 or null,
//    "led_on": true/false,
//    "neo_on": true/false,
//    "sta_connected": true/false
//  }

void handleApiState() {
  SensorData_t data;
  float t = NAN, h = NAN;
  if (sensor_get_latest(&data)) {
    t = data.temperature;
    h = data.humidity;
  }

  bool staConn = (WiFi.status() == WL_CONNECTED);

  String payload = "{";

  // temperature
  if (isnan(t)) {
    payload += "\"temperature\":null";
  } else {
    payload += "\"temperature\":" + String(t, 1);
  }

  // humidity
  if (isnan(h)) {
    payload += ",\"humidity\":null";
  } else {
    payload += ",\"humidity\":" + String(h, 1);
  }

  // trạng thái thiết bị
  payload += ",\"led_on\":";  payload += led_on ? "true" : "false";
  payload += ",\"neo_on\":";  payload += neopixel_on ? "true" : "false";

  // WiFi STA
  payload += ",\"sta_connected\":"; payload += staConn ? "true" : "false";

  payload += "}";

  server.send(200, "application/json", payload);
}

// ======================= API CONTROL 2 THIẾT BỊ =======================
//
// /api/control?dev=led&on=1   -> ép LED ON
// /api/control?dev=led&on=0   -> ép LED OFF
// /api/control?dev=neo&on=1   -> ép NeoPixel ON (màu trắng)
// /api/control?dev=neo&on=0   -> ép NeoPixel OFF
//
// Ghi vào global flag:
//   led_force, led_force_state
//   neo_force, neo_force_state
// Các task LED / NeoPixel sẽ đọc các flag này để override hành vi tự động.

void handleApiControl() {
  String dev = server.arg("dev");
  bool on    = (server.arg("on") == "1");

  if (dev == "led") {
    led_force       = true;
    led_force_state = on;
    led_on          = on;
    Serial.printf("[WEB] Control LED -> %s\n", on ? "ON" : "OFF");

  } else if (dev == "neo") {
    neo_force       = true;
    neo_force_state = on;
    neopixel_on     = on;
    Serial.printf("[WEB] Control NeoPixel -> %s\n", on ? "ON" : "OFF");
  }

  // (Nếu sau này muốn thêm nút AUTO, ta chỉ cần dev=led_auto/neo_auto -> led_force=false/neo_force=false)

  server.send(200, "application/json", "{\"status\":\"ok\"}");
}

// ======================= TRANG CHÍNH =======================

void handleRoot() {
  String html =
    "<!DOCTYPE html><html><head>"
    "<meta charset='utf-8'>"
    "<title>Yolo Uno Portal</title>"
    "<style>"
    "body{font-family:sans-serif;background:#111;color:#eee;margin:0;padding:16px;}"
    "h2{color:#0ff;}"
    ".card{background:#222;border-radius:10px;padding:16px;margin-bottom:16px;}"
    "label{display:block;margin-top:8px;}"
    "input{width:260px;padding:4px;margin-top:2px;}"
    "button,input[type=submit]{padding:6px 12px;margin-top:10px;margin-right:4px;}"
    ".ok{color:#0f0;}.bad{color:#f33;}"
    "</style>"
    "<script>"
    "function updateState(){"
      "fetch('/api/state').then(r=>r.json()).then(d=>{"
        "if(d.temperature!==null)"
          "document.getElementById('temp').innerText = d.temperature.toFixed(1);"
        "else document.getElementById('temp').innerText='--';"
        "if(d.humidity!==null)"
          "document.getElementById('humi').innerText = d.humidity.toFixed(1);"
        "else document.getElementById('humi').innerText='--';"
        "document.getElementById('led_state').innerText = d.led_on ? 'ON' : 'OFF';"
        "document.getElementById('neo_state').innerText = d.neo_on ? 'ON' : 'OFF';"
        "document.getElementById('sta_status').innerText = d.sta_connected ? 'CONNECTED' : 'DISCONNECTED';"
        "document.getElementById('sta_status').className = d.sta_connected ? 'ok' : 'bad';"
      "}).catch(e=>{console.log(e);});"
    "}"
    "function ctrl(dev,on){"
      "fetch('/api/control?dev='+dev+'&on='+(on?1:0)).then(()=>setTimeout(updateState,300));"
    "}"
    "window.onload=function(){updateState();setInterval(updateState,5000);};"
    "</script>"
    "</head><body>";

  html += "<h2>Yolo Uno Portal</h2>";

  // CARD WiFi
  html += "<div class='card'>";
  html += "<h3>Trạng thái WiFi</h3>";
  html += "<p><b>AP:</b> YoloUno-Setup (IP: 192.168.4.1)</p>";
  html += "<p><b>STA SSID hiện tại:</b> " +
          (sta_ssid == "" ? String("(chưa cấu hình)") : sta_ssid) + "</p>";
  html += "<p><b>STA status:</b> <span id='sta_status'>UNKNOWN</span></p>";
  html += "</div>";

  // CARD DHT20
  html += "<div class='card'>";
  html += "<h3>DHT20</h3>";
  html += "<p>Nhiệt độ: <span id='temp'>--</span> &deg;C</p>";
  html += "<p>Độ ẩm: <span id='humi'>--</span> %</p>";
  html += "</div>";

  // CARD CONTROL THIẾT BỊ
  html += "<div class='card'>";
  html += "<h3>Điều khiển thiết bị</h3>";

  html += "<p><b>LED chỉ báo (GPIO48)</b><br>";
  html += "Trạng thái: <span id='led_state'>--</span><br>";
  html += "<button onclick=\"ctrl('led',1)\">LED ON</button>";
  html += "<button onclick=\"ctrl('led',0)\">LED OFF</button></p>";

  html += "<p><b>NeoPixel (Grow-light)</b><br>";
  html += "Trạng thái: <span id='neo_state'>--</span><br>";
  html += "<button onclick=\"ctrl('neo',1)\">Neo ON</button>";
  html += "<button onclick=\"ctrl('neo',0)\">Neo OFF</button></p>";

  html += "</div>";

  // CARD CẤU HÌNH WIFI
  html += "<div class='card'>";
  html += "<h3>Cấu hình WiFi có Internet (STA)</h3>";
  html += "<form method='POST' action='/wifi'>"
          "<label>SSID (tối đa 32 ký tự):"
          "<input name='ssid' maxlength='32' value='" + sta_ssid + "'></label>"
          "<label>Password (tối đa 64 ký tự):"
          "<input name='pass' maxlength='64' type='password'></label>"
          "<input type='submit' value='Lưu & Kết nối'>"
          "</form>";
  html += "<p style='font-size:12px;opacity:0.8'>"
          "Sau khi lưu, ESP sẽ dùng mạng này để ra Internet (MQTT). "
          "Bạn vẫn kết nối với YoloUno-Setup để truy cập portal."
          "</p>";
  html += "</div>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ======================= XỬ LÝ LƯU WI-FI =======================

void handleWifiPost() {
  String ssid = server.arg("ssid");
  String pass = server.arg("pass");

  saveCredentials(ssid, pass);
  sta_ssid = ssid;
  sta_pass = pass;

  // thử connect luôn
  tryConnectSTA();

  String html =
    "<!DOCTYPE html><html><head>"
    "<meta charset='utf-8'>"
    "<meta http-equiv='refresh' content='3;url=/' />"
    "<title>WiFi saved</title>"
    "</head><body>";

  html += "<h2>Đã lưu cấu hình WiFi</h2>";
  html += "<p>SSID mới: <b>" + ssid + "</b></p>";
  html += "<p>Trạng thái hiện tại: <b>" +
          String(WiFi.status() == WL_CONNECTED ? "CONNECTED" : "DISCONNECTED") +
          "</b></p>";
  html += "<p>Trang sẽ tự quay về giao diện chính sau 3 giây...</p>";
  html += "<p>Nếu không tự quay, bấm vào đây: <a href='/'>Về trang chính</a></p>";

  html += "</body></html>";

  server.send(200, "text/html", html);
}

// ======================= TASK WEBSERVER =======================

void task_web_server(void* pvParameters) {
  Serial.println("[WEB] Init AP+STA portal...");

  WiFi.persistent(false);         // tránh ghi flash tự động
  WiFi.mode(WIFI_AP_STA);
  delay(200);

  bool ap_ok = WiFi.softAP("YoloUno-Setup", "12345678");
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("[WEB] softAP result = ");
  Serial.println(ap_ok ? "OK" : "FAIL");
  Serial.print("[WEB] AP IP: ");
  Serial.println(apIP);

  loadCredentials();
  tryConnectSTA();

  server.on("/",           HTTP_GET,  handleRoot);
  server.on("/wifi",       HTTP_POST, handleWifiPost);
  server.on("/api/state",  HTTP_GET,  handleApiState);
  server.on("/api/control",HTTP_GET,  handleApiControl);

  server.begin();
  Serial.println("[WEB] HTTP server started on AP (80)");

  while (true) {
    server.handleClient();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void web_server_init(const char* ssid, const char* pass) {
  // không dùng
}
