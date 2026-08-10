#include "aquarium_logic.h"
#include "language_manager.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <SD.h>
#include <WiFi.h>


AquariumLogic aquarium;

static OneWire oneWire(TEMP_SENSOR_PIN);
static DallasTemperature sensors(&oneWire);

AquariumLogic::AquariumLogic() {}

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
  return cfg.wifiSsid;
}

void AquariumLogic::connectWifiSSID(const String& ssid, const String& password) {
  cfg.wifiSsid = ssid;
  cfg.wifiPassword = password;

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);
  if (password.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());
  } else {
    WiFi.begin(ssid.c_str());
  }
  m_wifiScanStatus = String(langManager.getText("MSG_CONNECTING", "Connecting to ")) + ssid + "...";
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
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  digitalWrite(LIGHT_RELAY_PIN, LOW);

  sensors.begin();
  int deviceCount = sensors.getDeviceCount();
  if (deviceCount > 0) {
    m_sensorConnected = true;
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
  } else {
    m_sensorConnected = false;
  }

  loadConfigSD();
  evaluateSchedule();
  applyLightHardware();
}

void AquariumLogic::update() {
  uint32_t now = millis();

  // Automatic NTP Sync when Wi-Fi is connected (on connect & every 30 minutes)
  static bool wasConnected = false;
  static uint32_t lastNtpSync = 0;
  bool isConn = (WiFi.status() == WL_CONNECTED);
  if (isConn && (!wasConnected || (now - lastNtpSync >= 1800000))) {
    lastNtpSync = now;
    syncNTP();
  }
  wasConnected = isConn;

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
    float rawT = sensors.getTempCByIndex(0);
    sensors.requestTemperatures(); // request next conversion

    if (rawT > -50.0f && rawT < 85.0f) {
      float filtered = rawT + m_config.tempOffset;
      // Exponential moving average filter for smooth reading
      m_currentTemp = m_currentTemp * 0.7f + filtered * 0.3f;
      if (m_currentTemp < m_minTemp) m_minTemp = m_currentTemp;
      if (m_currentTemp > m_maxTemp) m_maxTemp = m_currentTemp;
      return;
    }
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
  digitalWrite(LIGHT_RELAY_PIN, pinState ? HIGH : LOW);
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
  applyLightHardware();
  saveConfigSD();
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
  struct tm timeinfo;
  if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo, 0)) {
    h = timeinfo.tm_hour;
    m = timeinfo.tm_min;
    s = timeinfo.tm_sec;
  } else {
    uint32_t secToday = m_uptimeSeconds % 86400;
    h = secToday / 3600;
    m = (secToday % 3600) / 60;
    s = secToday % 60;
  }
}

void AquariumLogic::getDate(int& day, int& month, int& year) const {
  struct tm timeinfo;
  if (WiFi.status() == WL_CONNECTED && getLocalTime(&timeinfo, 0)) {
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
  return writeWholeConfigFileSafe();
}



