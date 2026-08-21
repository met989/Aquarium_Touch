#include "../include/aquarium_logic.h"
#include "../include/language_manager.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>


#include <Wire.h>
#include <Adafruit_MCP23X17.h>
// #include <DS2482.h> // Da decommentare quando arriva l'hardware
// #include <DallasTemperature.h>

Adafruit_MCP23X17 mcp;
bool mcp_connected = false;

// Predisposizione per il bridge 1-Wire I2C
// DS2482 ds(0);
// DallasTemperature sensors(&ds);

AquariumLogic aquarium;

AquariumLogic::AquariumLogic() {
  // Configurazione base I2C
}

bool AquariumLogic::isWifiConnected() const {
  return (WiFi.status() == WL_CONNECTED);
}

String AquariumLogic::getWifiIP() const {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.localIP().toString();
  }
  return "0.0.0.0";
}

int AquariumLogic::getWifiRSSI() const {
  if (WiFi.status() == WL_CONNECTED) {
    return WiFi.RSSI();
  }
  return 0;
}

extern AppConfig cfg;

String AquariumLogic::getWifiConnectSSID() const {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  String s = m_wifiConnectSSID;
  xSemaphoreGive(g_configMutex);
  return s;
}

void AquariumLogic::connectWifiSSID(const String& ssid, const String& password) {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  cfg.wifiSsid = ssid;
  cfg.wifiPassword = password;
  m_wifiConnectSSID = ssid;
  xSemaphoreGive(g_configMutex);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  if (password.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());
  } else {
    WiFi.begin(ssid.c_str());
  }
  m_wifiScanStatus = String(langManager.getText("MSG_CONNECTING ", "Connecting to ")) + ssid + "...";
  m_wifiConnectState = WIFI_CONN_CONNECTING;
  m_wifiConnectStartTime = millis();
  saveConfigSD();
}

void AquariumLogic::disconnectWifi() {
  WiFi.disconnect(true);
  m_wifiScanStatus = langManager.getText("MSG_DISCONNECTED", "DISCONNECTED");
}

void AquariumLogic::startAsyncWifiScan() {
  if (m_wifiScanning) return;
  m_wifiScanStatus = langManager.getText("MSG_SCANNING_BG", "Scanning...");
  m_wifiScanning = true;
  WiFi.scanDelete(); // Ensure old results are cleared
  WiFi.scanNetworks(true, true);
}

void AquariumLogic::scanWifi() {
  startAsyncWifiScan();
}

WifiNetworkItem AquariumLogic::getWifiNetwork(int idx) const {
  if (idx >= 0 && idx < m_wifiNetworkCount) {
    return m_scannedNetworks[idx];
  }
  return { "", 0, false };
}

void AquariumLogic::init() {
  // Il relè verrà mappato in futuro su altri pin/espansioni.

  // 1. Inizializzazione Bus I2C
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  
  // 2. Inizializzazione MCP23017 (Relè)
  if (!mcp.begin_I2C(0x20)) {
    Serial.println("Errore: MCP23017 non trovato sul bus I2C!");
    mcp_connected = false;
  } else {
    Serial.println("MCP23017 Inizializzato con successo.");
    mcp_connected = true;
    
    // Configura i pin dinamici (se definiti)
    if (cfg.mcpPinLight >= 0 && cfg.mcpPinLight < 16) {
      mcp.pinMode(cfg.mcpPinLight, OUTPUT);
    }
    if (cfg.mcpPinWaterLevel >= 0 && cfg.mcpPinWaterLevel < 16) {
      mcp.pinMode(cfg.mcpPinWaterLevel, INPUT_PULLUP);
    }
  }

  // 3. Inizializzazione Sensore Temperatura (DS2482)
  // TODO: Da abilitare quando presente il DS2482
  /*
  sensors.begin();
  int deviceCount = sensors.getDeviceCount();
  if (deviceCount > 0) {
    m_sensorConnected = true;
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
  } else {
    m_sensorConnected = false;
  }
  */
  // 4. LDR and Backlight PWM setup
  pinMode(LDR_PIN, ANALOG);
  analogSetAttenuation(ADC_0db); // Massima sensibilità a bassa tensione
  
  ledcSetup(0, 5000, 8); // Canale 0, 5000 Hz, risoluzione 8 bit
  ledcAttachPin(TFT_BL, 0);
  updateBacklight(); // Imposta la luminosità iniziale

  m_sensorConnected = false; // Fallback simulation finché non c'è l'hardware


  loadConfigSD();
  evaluateSchedule();
  applyLightHardware();

  // 4. Time Update
  m_lastTimeUpdate = millis();
}

String AquariumLogic::scanI2C() const {
  String result = "";
  byte error, address;
  int nDevices = 0;
  for(address = 1; address < 127; address++ ) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      if (nDevices > 0) result += ", ";
      result += "0x";
      if (address < 16) result += "0";
      result += String(address, HEX);
      nDevices++;
    }
  }
  if (nDevices == 0) {
    result = "Nessun dispositivo I2C trovato";
  }
  return result;
}

void AquariumLogic::update() {
  uint32_t now = millis();
  
  static uint32_t lastPrint = 0;
  if (now - lastPrint >= 5000) {
      lastPrint = now;
      Serial.println("AquariumLogic::update() is running!");
  }

  // Automatic NTP Sync when Wi-Fi is connected (on first connect & every 24 hours)
  static bool hasSynced = false;
  static uint32_t lastNtpSync = 0;
  bool isConn = (WiFi.status() == WL_CONNECTED);
  if (isConn && (!hasSynced || (now - lastNtpSync >= 86400000))) {
    lastNtpSync = now;
    hasSynced = true;
    syncNTP();
  }

  // Clock Ticking (1s increment)
  if (now - m_lastTimeUpdate >= 1000) {
    uint32_t elapsedSec = (now - m_lastTimeUpdate) / 1000;
    m_lastTimeUpdate = now;
    m_uptimeSeconds += elapsedSec;
    evaluateSchedule();
  }

  // Sensor Sampling (every 2s)
  if (now - m_lastSensorRead >= 2000) {
    m_lastSensorRead = now;
    readSensor();
  }

  // LDR Update (every 1s)
  static uint32_t lastLdrRead = 0;
  if (now - lastLdrRead >= 1000) {
    lastLdrRead = now;
    m_currentLdrValue = analogReadMilliVolts(LDR_PIN);
    Serial.printf("LDR mV: %d\n", m_currentLdrValue);
    updateBacklight();
  }

  // WiFi Async Scan Polling
  if (m_wifiScanning) {
    int n = WiFi.scanComplete();
    if (n == WIFI_SCAN_FAILED) {
      m_wifiScanStatus = "Scan Failed";
      m_wifiScanning = false;
      m_wifiScanCounter++;
    } else if (n >= 0) {
      m_wifiNetworkCount = (n > 12) ? 12 : n;
      if (n == 0) {
        m_wifiScanStatus = langManager.getText("MSG_NO_NETWORKS", "No networks found");
      } else {
        for (int i = 0; i < m_wifiNetworkCount; i++) {
          m_scannedNetworks[i].ssid = WiFi.SSID(i);
          m_scannedNetworks[i].rssi = WiFi.RSSI(i);
          m_scannedNetworks[i].open = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN);
        }
        m_wifiScanStatus = String(n) + " " + langManager.getText("MSG_NETWORKS_FOUND", "networks found");
      }
      WiFi.scanDelete();
      m_wifiScanning = false;
      m_wifiScanCounter++;
    }
  }

  // WiFi Connection state machine
  if (m_wifiConnectState == WIFI_CONN_CONNECTING) {
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) {
      m_wifiConnectState = WIFI_CONN_SUCCESS;
      m_wifiConnectResultTime = millis();
    } else if (millis() - m_wifiConnectStartTime > 15000 || st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) {
      m_wifiConnectState = WIFI_CONN_FAILED;
      m_wifiConnectResultTime = millis();
    }
  } else if (m_wifiConnectState == WIFI_CONN_SUCCESS || m_wifiConnectState == WIFI_CONN_FAILED) {
    if (millis() - m_wifiConnectResultTime > 5000) {
      m_wifiConnectState = WIFI_CONN_IDLE;
    }
  }
}


void AquariumLogic::readSensor() {
  if (m_sensorConnected) {
    // TODO: Lettura I2C dal DS2482
    /*
    float rawT = sensors.getTempCByIndex(0);
    sensors.requestTemperatures(); // request next conversion

    if (rawT > -50.0f && rawT < 85.0f) {
      float filtered = rawT + m_config.tempOffset;
      m_currentTemp = m_currentTemp * 0.7f + filtered * 0.3f;
      if (m_currentTemp < m_minTemp) m_minTemp = m_currentTemp;
      if (m_currentTemp > m_maxTemp) m_maxTemp = m_currentTemp;
      return;
    }
    */
  }

  // Fallback Simulation if hardware sensor is absent/disconnected
  simulateSensor();
}

void AquariumLogic::simulateSensor() {
  // Realistic smooth sinusoidal aquarium temperature around 25.5°C
  float timeInHours = (float)(m_uptimeSeconds % 86400) / 3600.0f;
  float wave = sinf(timeInHours * 0.261799f); // 24h period
  float noise = ((float)(rand() % 100) - 50.0f) * 0.002f;

  float simT = 25.4f + wave * 0.8f + noise + m_config.tempOffset;
  m_currentTemp = m_currentTemp * 0.85f + simT * 0.15f;

  if (m_currentTemp < m_minTemp) m_minTemp = m_currentTemp;
  if (m_currentTemp > m_maxTemp) m_maxTemp = m_currentTemp;
}

TempStatus AquariumLogic::getTempStatus() const {
  if (m_currentTemp < m_config.targetTempMin) return TEMP_TOO_COLD;
  if (m_currentTemp > m_config.targetTempMax) return TEMP_TOO_HOT;
  return TEMP_OPTIMAL;
}

void AquariumLogic::evaluateSchedule() {
  if (m_manualOverride || !m_config.autoSchedule) return;

  int h, m, s;
  getTime(h, m, s);
  int currentMins = h * 60 + m;
  int onMins = m_config.lightOnHour * 60 + m_config.lightOnMin;
  int offMins = m_config.lightOffHour * 60 + m_config.lightOffMin;

  bool shouldBeOn = false;
  if (onMins < offMins) {
    shouldBeOn = (currentMins >= onMins && currentMins < offMins);
  } else {
    shouldBeOn = (currentMins >= onMins || currentMins < offMins);
  }

  if (m_lightOn != shouldBeOn) {
    m_lightOn = shouldBeOn;
    applyLightHardware();
  }
}
void AquariumLogic::applyLightHardware() {
  bool pinState = m_config.relayInverted ? !m_lightOn : m_lightOn;
  
  if (mcp_connected && cfg.mcpPinLight >= 0 && cfg.mcpPinLight < 16) {
    mcp.digitalWrite(cfg.mcpPinLight, pinState ? HIGH : LOW);
  }
}

void AquariumLogic::setLightManual(bool on) {
  m_manualOverride = true;
  m_lightOn = on;
  applyLightHardware();
}

void AquariumLogic::toggleLight() {
  setLightManual(!m_lightOn);
}

void AquariumLogic::toggleRelayInverted() {
  m_config.relayInverted = !m_config.relayInverted;
  saveConfigSD();
  applyLightHardware();
}

void AquariumLogic::setAutoDimming(bool enable) {
  m_config.autoDimming = enable;
  saveConfigSD();
  updateBacklight();
}

extern bool g_screenIsOff;

void AquariumLogic::updateBacklight() {
  if (g_screenIsOff) return; // Non accendere se lo screen saver è attivo

  if (m_config.autoDimming) {
    // 75mV (massima luminosità ambientale) -> PWM 255
    // 1000mV (buio) -> PWM 10
    int pwmValue = map(m_currentLdrValue, 75, 1000, 255, 10);
    if (pwmValue < 10) pwmValue = 10;
    if (pwmValue > 255) pwmValue = 255;
    ledcWrite(0, pwmValue);
  } else {
    ledcWrite(0, 255); // Massima luminosità
  }
}

void AquariumLogic::setAutoSchedule(bool enable) {
  m_config.autoSchedule = enable;
  m_manualOverride = false;
  evaluateSchedule();
  saveConfigSD();
}

bool AquariumLogic::syncNTP() {
  if (WiFi.status() != WL_CONNECTED) {
    m_ntpStatus = "NTP: Wi-Fi non connesso";
    return false;
  }
  if (cfg.ntpServer1.length() == 0) {
    m_ntpStatus = "NTP: Server non impostato";
    return false;
  }

  int tzHours = cfg.timezone.toInt();
  long gmtOffset_sec = (long)tzHours * 3600;

  const char* s1 = cfg.ntpServer1.c_str();
  const char* s2 = cfg.ntpServer2.length() > 0 ? cfg.ntpServer2.c_str() : nullptr;

  configTime(gmtOffset_sec, 0, s1, s2);

  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 3000)) {
    m_uptimeSeconds = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    m_ntpStatus = "NTP: Sincronizzato OK";
    evaluateSchedule();
    return true;
  } else {
    m_ntpStatus = "NTP: In attesa risposta...";
    return false;
  }
}

void AquariumLogic::getTime(int& h, int& m, int& s) const {
  uint32_t secToday = m_uptimeSeconds % 86400;
  h = secToday / 3600;
  m = (secToday % 3600) / 60;
  s = secToday % 60;
}

void AquariumLogic::getDate(int& day, int& month, int& year) const {
  // Simplified date tracking based on last sync
  struct tm timeinfo;
  if (getLocalTime(&timeinfo, 0)) {
    day = timeinfo.tm_mday;
    month = timeinfo.tm_mon + 1;
    year = timeinfo.tm_year + 1900;
  } else {
    uint32_t daysElapsed = m_uptimeSeconds / 86400;
    day = 30 + (int)daysElapsed;
    month = 7;
    year = 2026;
    while (day > 31) {
      day -= 31;
      month++;
      if (month > 12) {
        month = 1;
        year++;
      }
    }
  }
}



void AquariumLogic::getFormattedDate(char* outBuf, size_t maxLen) const {
  int day, month, year;
  getDate(day, month, year);

  switch (m_config.dateFormat) {
    case DATE_FORMAT_MMDDYYYY:
      snprintf(outBuf, maxLen, "%02d/%02d/%04d", month, day, year);
      break;
    case DATE_FORMAT_YYYYMMDD:
      snprintf(outBuf, maxLen, "%04d/%02d/%02d", year, month, day);
      break;
    case DATE_FORMAT_DDMMYYYY:
    default:
      snprintf(outBuf, maxLen, "%02d/%02d/%04d", day, month, year);
      break;
  }
}

void AquariumLogic::setDateFormat(uint8_t format) {
  m_config.dateFormat = format % 3;
  saveConfigSD();
}

void AquariumLogic::setTime(int h, int m) {
  h = constrain(h, 0, 23);
  m = constrain(m, 0, 59);
  uint32_t daySec = h * 3600 + m * 60;
  m_uptimeSeconds = daySec;
  m_manualOverride = false;
  evaluateSchedule();
  saveConfigSD();
}


void AquariumLogic::setScheduleOn(uint8_t h, uint8_t m) {
  m_config.lightOnHour = h % 24;
  m_config.lightOnMin = m % 60;
  m_manualOverride = false;
  evaluateSchedule();
  saveConfigSD();
}

void AquariumLogic::setScheduleOff(uint8_t h, uint8_t m) {
  m_config.lightOffHour = h % 24;
  m_config.lightOffMin = m % 60;
  m_manualOverride = false;
  evaluateSchedule();
  saveConfigSD();
}

void AquariumLogic::setTargetTemp(float minT, float maxT) {
  minT = roundf(minT * 10.0f) / 10.0f;
  maxT = roundf(maxT * 10.0f) / 10.0f;
  if (minT < 15.0f) minT = 15.0f;
  if (maxT > 35.0f) maxT = 35.0f;
  if (minT > maxT - 0.5f) minT = maxT - 0.5f;
  m_config.targetTempMin = minT;
  m_config.targetTempMax = maxT;
  saveConfigSD();
}

void AquariumLogic::setTempOffset(float offset) {
  m_config.tempOffset = offset;
  saveConfigSD();
}

bool AquariumLogic::loadConfigSD() {
  if (!SD.exists(CONFIG_PATH)) return false;
  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (!f) return false;

  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() == 0 || line.startsWith("#")) continue;

    int eq = line.indexOf('=');
    if (eq < 0) continue;
    String key = line.substring(0, eq); key.trim();
    String val = line.substring(eq + 1); val.trim();

    if      (key.equalsIgnoreCase("light_on_h"))   m_config.lightOnHour   = val.toInt();
    else if (key.equalsIgnoreCase("light_on_m"))   m_config.lightOnMin    = val.toInt();
    else if (key.equalsIgnoreCase("light_off_h"))  m_config.lightOffHour  = val.toInt();
    else if (key.equalsIgnoreCase("light_off_m"))  m_config.lightOffMin   = val.toInt();
    else if (key.equalsIgnoreCase("auto_sched"))   m_config.autoSchedule  = (val == "1" || val.equalsIgnoreCase("true"));
    else if (key.equalsIgnoreCase("target_min_t")) m_config.targetTempMin = val.toFloat();
    else if (key.equalsIgnoreCase("target_max_t")) m_config.targetTempMax = val.toFloat();
    else if (key.equalsIgnoreCase("temp_offset"))  m_config.tempOffset    = val.toFloat();
    else if (key.equalsIgnoreCase("relay_inv"))    m_config.relayInverted = (val == "1" || val.equalsIgnoreCase("true"));
    else if (key.equalsIgnoreCase("date_format"))  m_config.dateFormat    = val.toInt() % 3;
    else if (key.equalsIgnoreCase("screensaver_t")) m_config.screensaverTime = (uint16_t)constrain(val.toInt(), 0, 3600);
    else if (key.equalsIgnoreCase("mcp_pin_light"))  cfg.mcpPinLight = (int8_t)val.toInt();
    else if (key.equalsIgnoreCase("mcp_pin_level"))  cfg.mcpPinWaterLevel = (int8_t)val.toInt();
    else if (key.equalsIgnoreCase("lang_file")) {
      String cleanVal = val;
      while (cleanVal.startsWith("\"") && cleanVal.endsWith("\"") && cleanVal.length() >= 2) {
        cleanVal = cleanVal.substring(1, cleanVal.length() - 1);
      }
      langManager.loadLanguage(cleanVal);
      String pureFile = langManager.getActiveLanguageFile();
      snprintf(m_config.langFile, sizeof(m_config.langFile), "%s", pureFile.c_str());
    }
  }
  f.close();
  applyLightHardware();
  return true;
}

void AquariumLogic::setLanguageFile(const String& langPath) {
  langManager.loadLanguage(langPath);
  String pureFile = langManager.getActiveLanguageFile();
  snprintf(m_config.langFile, sizeof(m_config.langFile), "%s", pureFile.c_str());
  saveConfigSD();
}

void AquariumLogic::setScreensaverTime(uint16_t seconds) {
  m_config.screensaverTime = seconds;
  saveConfigSD();
}


bool AquariumLogic::saveConfigSD() {
  g_saveConfigNeeded = true;
  return true;
}

bool AquariumLogic::checkGitHubForUpdate(String& outVersion, String& outUrl) {
  if (WiFi.status() != WL_CONNECTED) return false;

  WiFiClientSecure client;
  client.setInsecure(); // Ignore cert validation for simplicity

  HTTPClient http;
  if (http.begin(client, "https://api.github.com/repos/met989/Aquarium_Touch/releases/latest")) {
    int httpCode = http.GET();
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      if (!error) {
        if (doc["tag_name"].is<String>()) {
          outVersion = doc["tag_name"].as<String>();
          JsonArray assets = doc["assets"].as<JsonArray>();
          for (JsonVariant v : assets) {
            String name = v["name"].as<String>();
            if (name == "firmware.bin") {
              outUrl = v["browser_download_url"].as<String>();
              http.end();
              return true;
            }
          }
        }
      }
    }
    http.end();
  }
  return false;
}

void AquariumLogic::performOTAUpdate(const String& url) {
  if (WiFi.status() != WL_CONNECTED) return;
  
  WiFiClientSecure client;
  client.setInsecure();
  
  httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  t_httpUpdate_return ret = httpUpdate.update(client, url);
  switch (ret) {
    case HTTP_UPDATE_FAILED:
      Serial.printf("HTTP_UPDATE_FAILED Error (%d): %s\n", httpUpdate.getLastError(), httpUpdate.getLastErrorString().c_str());
      break;
    case HTTP_UPDATE_NO_UPDATES:
      Serial.println("HTTP_UPDATE_NO_UPDATES");
      break;
    case HTTP_UPDATE_OK:
      Serial.println("HTTP_UPDATE_OK");
      ESP.restart();
      break;
  }
}
