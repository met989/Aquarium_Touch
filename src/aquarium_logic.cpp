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
#include <ESP_Mail_Client.h>
#include <Wire.h>

OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);

AquariumLogic aquarium;

AquariumLogic::AquariumLogic() {
  // Constructor
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

  WiFi.setHostname("aquarium-touch");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);
  m_wifiAutoRetries = 0;

  if (ssid.length() == 0 || ssid == "SSID_WIFI") {
    m_wifiScanStatus = String(langManager.getText("MSG_WIFI_NO_SSID"));
    m_wifiConnectState = WIFI_CONN_IDLE;
    return;
  }

  if (cfg.wifiStaticEnabled && cfg.wifiIpStatic.length() > 0 && cfg.wifiIpStatic != "0.0.0.0") {
    IPAddress localIP, gateway, subnet, dns1, dns2;
    localIP.fromString(cfg.wifiIpStatic);
    gateway.fromString(cfg.wifiGateway);
    subnet.fromString(cfg.wifiSubnet);
    if (cfg.wifiDns1.length() > 0) dns1.fromString(cfg.wifiDns1);
    if (cfg.wifiDns2.length() > 0) dns2.fromString(cfg.wifiDns2);
    WiFi.config(localIP, gateway, subnet, dns1, dns2);
  }

  if (password.length() > 0) {
    WiFi.begin(ssid.c_str(), password.c_str());
  } else {
    WiFi.begin(ssid.c_str());
  }
  m_wifiScanStatus = String(langManager.getText("MSG_WIFI_CONNECTING")) + String(m_wifiAutoRetries + 1) + "/3)";
  m_wifiConnectState = WIFI_CONN_CONNECTING;
  m_wifiConnectStartTime = millis();
  saveConfigSD();
}

void AquariumLogic::startWifiConnection() {
  WiFi.setHostname("aquarium-touch");
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(false);

  if (cfg.wifiStaticEnabled && cfg.wifiIpStatic.length() > 0 && cfg.wifiIpStatic != "0.0.0.0") {
    IPAddress localIP, gateway, subnet, dns1, dns2;
    localIP.fromString(cfg.wifiIpStatic);
    gateway.fromString(cfg.wifiGateway);
    subnet.fromString(cfg.wifiSubnet);
    if (cfg.wifiDns1.length() > 0) dns1.fromString(cfg.wifiDns1);
    if (cfg.wifiDns2.length() > 0) dns2.fromString(cfg.wifiDns2);
    WiFi.config(localIP, gateway, subnet, dns1, dns2);
  }

  if (cfg.wifiPassword.length() > 0) {
    WiFi.begin(cfg.wifiSsid.c_str(), cfg.wifiPassword.c_str());
  } else {
    WiFi.begin(cfg.wifiSsid.c_str());
  }
  
  m_wifiScanStatus = "Tentativo di connessione wifi... (" + String(m_wifiAutoRetries + 1) + "/3)";
  m_wifiConnectState = WIFI_CONN_CONNECTING;
  m_wifiConnectStartTime = millis();
}

void AquariumLogic::disconnectWifi() {
  WiFi.disconnect();
  m_wifiScanStatus = langManager.getText("MSG_DISCONNECTED", "DISCONNECTED");
}

void AquariumLogic::startAsyncWifiScan() {
  if (m_wifiScanning) return;

  if (m_wifiConnectState != WIFI_CONN_IDLE && m_wifiConnectState != WIFI_CONN_SUCCESS) {
      WiFi.disconnect();
      m_wifiConnectState = WIFI_CONN_IDLE;
  }

  m_wifiScanStatus = langManager.getText("MSG_SCANNING_BG", "Scanning...");
  m_wifiScanning = true;
  
  WiFi.scanDelete();
  
  WiFi.scanNetworks(true, true); // ASYNC SCAN
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
  // Disabilita la riconnessione automatica per gestirla noi e non freezare
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("aquarium-touch");
  WiFi.setAutoReconnect(false);

  // Configurazione Pin Diretti
  digitalWrite(LIGHT_RELAY_PIN, m_config.relayInverted ? HIGH : LOW);
  pinMode(LIGHT_RELAY_PIN, OUTPUT);
  m_lightOn = false;
  
  // Init I2C & MCP23017
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  if (m_mcp.begin_I2C()) {
    m_mcpReady = true;
    if (cfg.mcpPinWaterLevel >= 0) {
      m_mcp.pinMode(cfg.mcpPinWaterLevel, INPUT_PULLUP);
    }
    // Set other pins as needed
  } else {
    m_mcpReady = false;
  }
  
  pinMode(PH_SENSOR_PIN, ANALOG);
  // TEMP_SENSOR_PIN (22) sarà gestito dalla libreria DS18B20/OneWire

  // 3. Inizializzazione Sensore Temperatura (DS18B20 su 1-Wire)
  sensors.begin();
  int deviceCount = sensors.getDeviceCount();
  if (deviceCount > 0) {
    m_sensorConnected = true;
    sensors.setWaitForConversion(false);
    sensors.requestTemperatures();
  } else {
    m_sensorConnected = false;
  }
  // 4. LDR and Backlight PWM setup
  pinMode(LDR_PIN, ANALOG);
  analogSetAttenuation(ADC_0db); // Massima sensibilità a bassa tensione
  
  ledcSetup(0, 5000, 8); // Canale 0, 5000 Hz, risoluzione 8 bit
  ledcAttachPin(TFT_BL, 0);
  updateBacklight(); // Imposta la luminosità iniziale

  loadConfigSD();
  evaluateSchedule();
  applyLightHardware();
  initMQTT();

  // 4. Time Update
  m_lastTimeUpdate = millis();
}

String AquariumLogic::scanI2C() const {
  String result = "";
  int nDevices = 0;
  for(byte address = 1; address < 127; address++ ) {
    Wire.requestFrom(address, (uint8_t)1, (uint8_t)true);
    bool ok = (Wire.available() > 0);
    while (Wire.available()) {
        Wire.read();
    }
    
    if (ok) {
      if (nDevices > 0) result += ", ";
      result += "0x";
      if (address < 16) result += "0";
      result += String(address, HEX);
      nDevices++;
    }
    delay(1);
  }
  if (nDevices == 0) {
    result = "No I2C device found";
  }
  return result;
}

void AquariumLogic::update() {
  uint32_t now = millis();
  
  static uint32_t lastPrint = 0;
  if (now - lastPrint >= 5000) {
      lastPrint = now;
      Serial.printf("AquariumLogic::update() - WiFi Status: %d, IP: %s\n", WiFi.status(), WiFi.localIP().toString().c_str());
      checkAlarmsAndNotify();
  }

  // Automatic NTP Sync when Wi-Fi is connected (on first connect & every 24 hours)
  static bool hasSynced = false;
  static uint32_t lastNtpSync = 0;
  bool isConn = (WiFi.status() == WL_CONNECTED);
  if (isConn && now >= 30000 && (!hasSynced || (now - lastNtpSync >= 86400000))) {
    lastNtpSync = now;
    hasSynced = true;
    syncNTP();
  }

  // Clock Ticking (1s increment)
  if (now - m_lastTimeUpdate >= 1000) {
    uint32_t elapsedSec = (now - m_lastTimeUpdate) / 1000;
    m_lastTimeUpdate = now;
    
    struct tm timeinfo;
    if (getLocalTime(&timeinfo, 0) && timeinfo.tm_year > 120) { // Year > 2020 (1900 + 120)
      m_uptimeSeconds = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    } else {
      m_uptimeSeconds += elapsedSec;
    }
    
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
    if (n >= 0) {
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
    } else if (n == WIFI_SCAN_FAILED) {
      m_wifiScanStatus = "Scan Failed";
      m_wifiScanning = false;
    }
  }

  // WiFi Connection state machine
  if (m_wifiConnectState == WIFI_CONN_CONNECTING) {
    wl_status_t st = WiFi.status();
    if (st == WL_CONNECTED) {
      m_wifiConnectState = WIFI_CONN_SUCCESS;
      m_wifiConnectResultTime = millis();
      m_wifiScanStatus = String(langManager.getText("MSG_WIFI_CONNECTED_TO")) + cfg.wifiSsid;
    } else if (millis() - m_wifiConnectStartTime > 15000 || st == WL_CONNECT_FAILED || st == WL_NO_SSID_AVAIL) {
      m_wifiAutoRetries++;
      m_wifiScanStatus = String(langManager.getText("MSG_WIFI_FAIL_ATTEMPT")) + String(m_wifiAutoRetries) + "/3)";
      WiFi.disconnect();
      
      if (m_wifiAutoRetries >= 3) {
        m_wifiConnectState = WIFI_CONN_WAIT_LONG;
        m_wifiWaitStartTime = millis();
        m_wifiScanStatus = String(langManager.getText("MSG_WIFI_FAIL_RETRY"));
      } else {
        m_wifiConnectState = WIFI_CONN_WAIT_SHORT;
        m_wifiWaitStartTime = millis();
      }
    }
  } else if (m_wifiConnectState == WIFI_CONN_WAIT_SHORT) {
    if (millis() - m_wifiWaitStartTime >= 30000) { // 30 sec
      startWifiConnection();
    }
  } else if (m_wifiConnectState == WIFI_CONN_WAIT_LONG) {
    if (millis() - m_wifiWaitStartTime >= 3600000) { // 1 hour
      m_wifiAutoRetries = 0;
      startWifiConnection();
    }
  } else if (m_wifiConnectState == WIFI_CONN_SUCCESS) {
    if (millis() - m_wifiConnectResultTime > 5000) {
      m_wifiConnectState = WIFI_CONN_IDLE;
    }
  } else if (m_wifiConnectState == WIFI_CONN_IDLE) {
    if (WiFi.status() != WL_CONNECTED && cfg.wifiSsid.length() > 0 && cfg.wifiSsid != "SSID_WIFI") {
      m_wifiAutoRetries = 0;
      startWifiConnection();
    }
  }

  updateMQTT();
}


void AquariumLogic::readSensor() {
  if (!m_sensorConnected) {
    sensors.begin();
    if (sensors.getDeviceCount() > 0) {
      m_sensorConnected = true;
      sensors.setWaitForConversion(false);
      sensors.requestTemperatures();
    }
  }

  if (m_sensorConnected) {
    float rawT = sensors.getTempCByIndex(0);
    sensors.requestTemperatures(); // request next conversion

    // 85.0f is power-on default, -127.0f is error
    if (rawT == -127.0f) {
      m_sensorConnected = false; // Sensor disconnected
    } else if (rawT != 85.0f) {
      float filtered = rawT + m_config.tempOffset;
      if (m_currentTemp == 0.0f) {
        m_currentTemp = filtered; // first valid read
      } else {
        m_currentTemp = m_currentTemp * 0.7f + filtered * 0.3f;
      }
      if (m_currentTemp < m_minTemp) m_minTemp = m_currentTemp;
      if (m_currentTemp > m_maxTemp) m_maxTemp = m_currentTemp;
    }
  }

  // Leggi pH ogni secondo (evita spam)
  if (millis() - m_lastPhReadTime > 1000) {
    m_lastPhReadTime = millis();
    int rawPh = analogRead(PH_SENSOR_PIN);
    float voltage = (rawPh / 4095.0f) * 3.3f;
    
    // Formula placeholder (lineare). L'utente fornirà i valori di calibrazione.
    // Esempio generico: pH = 3.5 * voltage + offset
    float calculatedPh = (3.5f * voltage) + m_config.phOffset; 
    
    // Media mobile semplice
    m_currentPh = (m_currentPh == 7.0f) ? calculatedPh : (m_currentPh * 0.8f + calculatedPh * 0.2f);
  }
}

bool AquariumLogic::isWaterLevelOk() {
  if (m_mcpReady && cfg.mcpPinWaterLevel >= 0) {
    // Assume LOW = acqua assente (contatto aperto con pull-up), dipenderà dal cablaggio
    // Supponiamo che quando il livello è OK il galleggiante chiuda a massa (LOW).
    return m_mcp.digitalRead(cfg.mcpPinWaterLevel) == LOW;
  }
  return true; // Se MCP non c'è, diciamo che è tutto ok
}

void AquariumLogic::checkAlarmsAndNotify() {
  if (!cfg.emailEnabled || WiFi.status() != WL_CONNECTED) return;
  
  TempStatus currentTempStatus = m_sensorConnected ? getTempStatus() : TEMP_OPTIMAL;
  float phHyst = 0.2f;
  int currentPhStatus = m_lastPhStatus;
  if (m_lastPhStatus == 1) {
    if (m_currentPh >= m_config.targetPhMin + phHyst) currentPhStatus = 0;
  } else if (m_lastPhStatus == 2) {
    if (m_currentPh <= m_config.targetPhMax - phHyst) currentPhStatus = 0;
  } else {
    if (m_currentPh <= m_config.targetPhMin - phHyst) currentPhStatus = 1;
    if (m_currentPh >= m_config.targetPhMax + phHyst) currentPhStatus = 2;
  }
  
  bool currentWaterLevelOk = isWaterLevelOk();
  
  String subject = "";
  String body = "";
  char buf[256];
  
  if (currentTempStatus != m_lastTempStatus) {
    if (currentTempStatus == TEMP_TOO_HOT) {
      subject = langManager.getText("ALARM_TEMP_HIGH_SUB");
      snprintf(buf, sizeof(buf), langManager.getText("ALARM_TEMP_HIGH_BODY"), m_currentTemp, m_config.targetTempMax);
      body = String(buf);
    } else if (currentTempStatus == TEMP_TOO_COLD) {
      subject = langManager.getText("ALARM_TEMP_LOW_SUB");
      snprintf(buf, sizeof(buf), langManager.getText("ALARM_TEMP_LOW_BODY"), m_currentTemp, m_config.targetTempMin);
      body = String(buf);
    } else {
      subject = langManager.getText("INFO_TEMP_OK_SUB");
      snprintf(buf, sizeof(buf), langManager.getText("INFO_TEMP_OK_BODY"), m_currentTemp);
      body = String(buf);
    }
    m_lastTempStatus = currentTempStatus;
    sendEmail(subject, body);
  }
  
  if (currentPhStatus != m_lastPhStatus) {
    if (currentPhStatus == 2) {
      subject = langManager.getText("ALARM_PH_HIGH_SUB");
      snprintf(buf, sizeof(buf), langManager.getText("ALARM_PH_HIGH_BODY"), m_currentPh, m_config.targetPhMax);
      body = String(buf);
    } else if (currentPhStatus == 1) {
      subject = langManager.getText("ALARM_PH_LOW_SUB");
      snprintf(buf, sizeof(buf), langManager.getText("ALARM_PH_LOW_BODY"), m_currentPh, m_config.targetPhMin);
      body = String(buf);
    } else {
      subject = langManager.getText("INFO_PH_OK_SUB");
      snprintf(buf, sizeof(buf), langManager.getText("INFO_PH_OK_BODY"), m_currentPh);
      body = String(buf);
    }
    m_lastPhStatus = currentPhStatus;
    sendEmail(subject, body);
  }
  
  if (currentWaterLevelOk != m_lastWaterLevelOk) {
    if (!currentWaterLevelOk) {
      subject = langManager.getText("ALARM_WATER_LOW_SUB");
      body = langManager.getText("ALARM_WATER_LOW_BODY");
    } else {
      subject = langManager.getText("INFO_WATER_OK_SUB");
      body = langManager.getText("INFO_WATER_OK_BODY");
    }
    m_lastWaterLevelOk = currentWaterLevelOk;
    sendEmail(subject, body);
  }
}

bool AquariumLogic::sendEmail(const String& subject, const String& body) {
  if (!cfg.emailEnabled || cfg.smtpHost.isEmpty() || cfg.smtpUser.isEmpty()) return false;
  
  SMTPSession smtp;
  smtp.debug(0);
  
  Session_Config config;
  config.server.host_name = cfg.smtpHost;
  config.server.port = cfg.smtpPort;
  config.login.email = cfg.smtpUser;
  config.login.password = cfg.smtpPassword;
  
  // Set SSL configuration
  // For ESP Mail Client 3.x, you just avoid setting SSL if not needed, or set ports correctly
  // We can just rely on the port to define SSL behavior (465 = SSL, 25/587 = No/STARTTLS)
  // Or we can set nothing since the library handles it natively.

  
  SMTP_Message message;
  message.sender.name = "Aquarium OS Touch";
  message.sender.email = cfg.emailSender.isEmpty() ? cfg.smtpUser : cfg.emailSender;
  message.subject = subject;
  
  // Parse recipients
  String rec = cfg.emailRecipients;
  int commaIndex = -1;
  do {
    commaIndex = rec.indexOf(',');
    String r = (commaIndex != -1) ? rec.substring(0, commaIndex) : rec;
    r.trim();
    if (!r.isEmpty()) message.addRecipient(r, r);
    rec = rec.substring(commaIndex + 1);
  } while (commaIndex != -1);
  
  message.text.content = body;
  
  if (!smtp.connect(&config)) {
    Serial.println("[SMTP] Connection error");
    return false;
  }
  
  if (!MailClient.sendMail(&smtp, &message)) {
    Serial.println("[SMTP] Error sending Email: " + smtp.errorReason());
    return false;
  }
  
  Serial.println("[SMTP] Email successfully sent!");
  return true;
}

TempStatus AquariumLogic::getTempStatus() const {
  float hyst = 0.2f; // Isteresi per evitare spam di notifiche
  
  if (m_lastTempStatus == TEMP_TOO_COLD) {
    if (m_currentTemp >= m_config.targetTempMin + hyst) return TEMP_OPTIMAL;
    return TEMP_TOO_COLD;
  } else if (m_lastTempStatus == TEMP_TOO_HOT) {
    if (m_currentTemp <= m_config.targetTempMax - hyst) return TEMP_OPTIMAL;
    return TEMP_TOO_HOT;
  } else {
    if (m_currentTemp <= m_config.targetTempMin - hyst) return TEMP_TOO_COLD;
    if (m_currentTemp >= m_config.targetTempMax + hyst) return TEMP_TOO_HOT;
    return TEMP_OPTIMAL;
  }
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
    m_ntpStatus = String(langManager.getText("MSG_NTP_WIFI_OFF"));
    return false;
  }
  if (cfg.ntpServer1.length() == 0) {
    m_ntpStatus = String(langManager.getText("MSG_NTP_NO_SERVER"));
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
    m_ntpStatus = String(langManager.getText("MSG_NTP_OK"));
    evaluateSchedule();
    return true;
  } else {
    m_ntpStatus = String(langManager.getText("MSG_NTP_WAIT"));
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
  static int last_day = 30, last_month = 7, last_year = 2026;
  struct tm timeinfo;
  
  if (getLocalTime(&timeinfo, 0)) {
    // Aggiorna la cache solo se riusciamo a leggere senza bloccare l'UI
    last_day = timeinfo.tm_mday;
    last_month = timeinfo.tm_mon + 1;
    last_year = timeinfo.tm_year + 1900;
  }
  
  // Restituisce l'ultima data valida letta (evita lo sfarfallio se il mutex NTP è occupato)
  day = last_day;
  month = last_month;
  year = last_year;
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

void AquariumLogic::setTargetPh(float minPh, float maxPh) {
  minPh = roundf(minPh * 10.0f) / 10.0f;
  maxPh = roundf(maxPh * 10.0f) / 10.0f;
  if (minPh < 0.0f) minPh = 0.0f;
  if (maxPh > 14.0f) maxPh = 14.0f;
  if (minPh > maxPh - 0.2f) minPh = maxPh - 0.2f;
  m_config.targetPhMin = minPh;
  m_config.targetPhMax = maxPh;
  saveConfigSD();
}

void AquariumLogic::setPhOffset(float offset) {
  m_config.phOffset = offset;
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
    else if (key.equalsIgnoreCase("target_ph_min")) m_config.targetPhMin  = val.toFloat();
    else if (key.equalsIgnoreCase("target_ph_max")) m_config.targetPhMax  = val.toFloat();
    else if (key.equalsIgnoreCase("ph_offset"))    m_config.phOffset      = val.toFloat();
    else if (key.equalsIgnoreCase("relay_inv"))    m_config.relayInverted = (val == "1" || val.equalsIgnoreCase("true"));
    else if (key.equalsIgnoreCase("date_format"))  m_config.dateFormat    = val.toInt() % 3;
    else if (key.equalsIgnoreCase("screensaver_t")) m_config.screensaverTime = (uint16_t)constrain(val.toInt(), 0, 3600);
    else if (key.equalsIgnoreCase("format_hour"))    cfg.formatHour = val.toInt();
    else if (key.equalsIgnoreCase("wifi_static_en")) cfg.wifiStaticEnabled = (val == "1" || val.equalsIgnoreCase("true"));
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
