
#include "task_check_info.h"

void Load_info_File()
{
  File file = LittleFS.open("/info.dat", "r");
  if (!file)
  {
    return;
  }
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, file);
  if (error)
  {
    Serial.print(F("deserializeJson() failed: "));
  }
  else
  {
    WIFI_SSID = doc["WIFI_SSID"].as<String>();
    WIFI_PASS = doc["WIFI_PASS"].as<String>();
    CORE_IOT_TOKEN = strdup(doc["CORE_IOT_TOKEN"]);
    CORE_IOT_SERVER = strdup(doc["CORE_IOT_SERVER"]);
    CORE_IOT_PORT = strdup(doc["CORE_IOT_PORT"]);
  }
  file.close();
}

void Delete_info_File()
{
  if (LittleFS.exists("/info.dat"))
  {
    LittleFS.remove("/info.dat");
  }
  ESP.restart();
}

void Save_info_File(String WIFI_SSID, String WIFI_PASS, String CORE_IOT_TOKEN, String CORE_IOT_SERVER, String CORE_IOT_PORT)
{
  Serial.println(WIFI_SSID);
  Serial.println(WIFI_PASS);

  JsonDocument doc;
  doc["WIFI_SSID"] = WIFI_SSID;
  doc["WIFI_PASS"] = WIFI_PASS;
  doc["CORE_IOT_TOKEN"] = CORE_IOT_TOKEN;
  doc["CORE_IOT_SERVER"] = CORE_IOT_SERVER;
  doc["CORE_IOT_PORT"] = CORE_IOT_PORT;

  File configFile = LittleFS.open("/info.dat", "w");
  if (configFile)
  {
    serializeJson(doc, configFile);
    configFile.close();
  }
  else
  {
    Serial.println("Unable to save the configuration.");
  }
  ESP.restart();
};

// TRONG src/task_check_info.cpp (Hàm check_info_File)
bool check_info_File(bool isLoopCheck)
{
    if (!isLoopCheck) // Chạy trong setup()
    {
        // Khởi tạo LittleFS an toàn: thử mount (false), nếu lỗi thì format (true)
        if (!LittleFS.begin(false)) {
            Serial.println("⚠️ [FS] LittleFS mount failed, trying to format...");
            if (!LittleFS.begin(true)) {
                Serial.println("❌ [FS] LittleFS Mount Failed after format!");
                return false;
            }
        }
        Load_info_File();
    }
    
    // Quyết định chế độ dựa trên cấu hình đã load
    if (WIFI_SSID.isEmpty())
    {
        if (!isLoopCheck)
        {
            
            isAPMode = true; 
        }
        return false;
    }
    
    if (!isLoopCheck) {
        isAPMode = false; // Có config -> mặc định là STA mode
    }

    return true; // Có cấu hình
}
