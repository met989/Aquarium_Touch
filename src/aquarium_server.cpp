#include "../include/aquarium_server.h"
#include "../include/aquarium_logic.h"
#include "../include/language_manager.h"
#include "../include/config.h"
#include <WiFi.h>
#include <SD.h>

AquariumServer aquariumServer;

extern bool writeWholeConfigFileSafe();

#include "../include/embedded_web.h"
#include "../include/embedded_languages.h"
#include <ElegantOTA.h>


AquariumServer::AquariumServer() : m_server(80), m_started(false) {}

void AquariumServer::init() {
  if (m_started) return;
  setupRoutes();
  m_server.begin();
  ElegantOTA.begin(&m_server);
  m_started = true;
  DBG_PRINTLN("[WEB] Web Server started on 80");

  // Avvia il task WebServer sul Core 0 (stesso core del WiFi) per non bloccare la UI (Core 1)
  // The web server will be handled in the update() method on Core 1
  // to avoid cross-core network deadlocks with MQTT and WiFi scanning.
}

void AquariumServer::update() {
  if (!m_started) {
    if (WiFi.status() == WL_CONNECTED) {
      init();
    }
  } else {
    if (WiFi.status() == WL_CONNECTED) {
      handleClient();
    }
  }
}

void AquariumServer::handleClient() {
  m_server.handleClient();
  ElegantOTA.loop();
}

void AquariumServer::setupRoutes() {
  m_server.on("/",                         HTTP_GET,  [this]() { handleRoot(); });
  m_server.on("/api/status",               HTTP_GET,  [this]() { handleApiStatus(); });
  m_server.on("/api/lang",                 HTTP_GET,  [this]() { 
    DBG_PRINTF("[WEB] GET /api/lang Free Heap: %u\n", ESP.getFreeHeap());
    uint32_t t = millis();
    m_server.sendHeader("Content-Encoding", "gzip");
    m_server.send_P(200, PSTR("application/json"), (const char*)langManager.getActiveLang()->json_gz, langManager.getActiveLang()->json_gz_len);
    DBG_PRINTF("[WEB] Sent /api/lang in %lu ms\n", millis() - t);
  });
  m_server.on("/api/light/toggle",         HTTP_POST, [this]() { handleApiToggleLight(); });
  m_server.on("/api/auto/toggle",          HTTP_POST, [this]() { handleApiToggleAuto(); });
  m_server.on("/api/schedule",             HTTP_POST, [this]() { handleApiSetSchedule(); });
  m_server.on("/api/settings/temp",        HTTP_POST, [this]() { handleApiSetTempSettings(); });
  m_server.on("/api/settings/time",        HTTP_POST, [this]() { handleApiSetTimeSettings(); });
  m_server.on("/api/ntp/sync",             HTTP_POST, [this]() { handleApiNtpSync(); });
  m_server.on("/api/settings/relay",       HTTP_POST, [this]() { handleApiSetRelaySettings(); });
  m_server.on("/api/settings/wifi",        HTTP_POST, [this]() { handleApiSetWifiSettings(); });
  m_server.on("/api/settings/network",     HTTP_POST, [this]() { handleApiSetNetworkSettings(); });
  m_server.on("/api/settings/mqtt",        HTTP_POST, [this]() { handleApiSetMqttSettings(); });
  m_server.on("/api/settings/system",      HTTP_POST, [this]() { handleApiSetSystemSettings(); });
  m_server.on("/api/settings/screensaver", HTTP_POST, [this]() { handleApiSetScreensaver(); });
  m_server.on("/api/settings/language",    HTTP_POST, [this]() { handleApiSetLanguage(); });
  m_server.on("/api/settings/hardware",    HTTP_POST, [this]() { handleApiSetHardwareSettings(); });
  m_server.on("/api/scan_i2c",             HTTP_GET,  [this]() { handleApiScanI2C(); });
  m_server.on("/api/config/raw",           HTTP_GET,  [this]() { handleApiConfigRawGet(); });
  m_server.on("/api/config/raw",           HTTP_POST, [this]() { handleApiConfigRawPost(); });
  m_server.on("/api/reboot",               HTTP_POST, [this]() { 
    m_server.send(200, "application/json", "{\"status\":\"rebooting\"}"); 
    delay(500); 
    ESP.restart(); 
  });
}

// AppConfig is defined in config.h (included via aquarium_logic.h)
extern AppConfig cfg;

void AquariumServer::handleRoot() {
  DBG_PRINTF("[WEB] GET / (handleRoot) Free Heap: %u\n", ESP.getFreeHeap());
  uint32_t t = millis();
  
  m_server.sendHeader("Content-Encoding", "gzip");
  m_server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  m_server.sendHeader("Pragma", "no-cache");
  m_server.sendHeader("Expires", "-1");
  m_server.send_P(200, PSTR("text/html; charset=utf-8"), (const char*)INDEX_HTML_GZ, INDEX_HTML_GZ_LEN);
  
  DBG_PRINTF("[WEB] Sent INDEX_HTML_GZ in %lu ms\n", millis() - t);
}

// Sanitize a String for JSON: escape backslash and double-quote
static String jStr(const String& s) {
  String out = s;
  out.replace("\\", "\\\\");
  out.replace("\"", "\\\"");
  out.replace("\n", "");
  out.replace("\r", "");
  return out;
}

// Convert float to JSON — emits null for NaN/Inf to keep JSON valid
static String jFloat(float v, int decimals = 1) {
  if (isnan(v) || isinf(v)) return "null";
  return String(v, decimals);
}

void AquariumServer::handleApiStatus() {
  DBG_PRINTLN("[WEB] GET /api/status");
  uint32_t t = millis();
  const AquariumConfig& aq = aquarium.getConfig();

  int h, m, s;
  aquarium.getTime(h, m, s);
  char timeBuf[12];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", h, m, s);

  char dateBuf[16];
  int day, mon, year;
  aquarium.getDate(day, mon, year);
  snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", day, mon, year);

  String tempStatusStr = (aquarium.getTempStatus() == TEMP_OPTIMAL)
    ? langManager.getText("MSG_OPTIMAL", "OPTIMAL")
    : (aquarium.getTempStatus() == TEMP_TOO_COLD
       ? langManager.getText("MSG_COLD", "TOO COLD")
       : langManager.getText("MSG_HOT",  "TOO HOT"));

  String lightStatusStr = aquarium.isLightOn()
    ? langManager.getText("MSG_LIGHT_ON",  "LIGHT ON")
    : langManager.getText("MSG_LIGHT_OFF", "LIGHT OFF");

  String json;
  json.reserve(2048);
  json += "{";
  json += "\"temp\":";             json += jFloat(aquarium.getTemperature()); json += ",";
  json += "\"temp_min\":";         json += jFloat(aq.targetTempMin); json += ",";
  json += "\"temp_max\":";         json += jFloat(aq.targetTempMax); json += ",";
  json += "\"temp_offset\":";      json += jFloat(aq.tempOffset); json += ",";
  json += "\"temp_status\":\"";    json += jStr(tempStatusStr);  json += "\",";
  json += "\"light_on\":";         json += String(aquarium.isLightOn() ? "true" : "false"); json += ",";
  json += "\"light_status\":\"";   json += jStr(lightStatusStr); json += "\",";
  json += "\"auto_sched\":";       json += String(aquarium.isAutoSchedule() ? "true" : "false"); json += ",";
  json += "\"light_on_h\":";       json += String(aq.lightOnHour); json += ",";
  json += "\"light_on_m\":";       json += String(aq.lightOnMin);  json += ",";
  json += "\"light_off_h\":";      json += String(aq.lightOffHour); json += ",";
  json += "\"light_off_m\":";      json += String(aq.lightOffMin);  json += ",";
  json += "\"relay_inv\":";        json += String(aquarium.isRelayInverted() ? "true" : "false"); json += ",";
  json += "\"date_format\":";      json += String(aq.dateFormat); json += ",";
  json += "\"screensaver_t\":";    json += String(aq.screensaverTime); json += ",";
  json += "\"timezone\":\"";       json += jStr(cfg.timezone);    json += "\",";
  json += "\"format_hour\":";      json += String(cfg.formatHour); json += ",";
  json += "\"ntp_server1\":\"";    json += jStr(cfg.ntpServer1);  json += "\",";
  json += "\"ntp_server2\":\"";    json += jStr(cfg.ntpServer2);  json += "\",";
  json += "\"wifi_ssid\":\"";      json += jStr(cfg.wifiSsid);    json += "\",";
  json += "\"wifi_static_en\":";   json += String(cfg.wifiStaticEnabled ? "true" : "false"); json += ",";
  json += "\"wifi_ip_static\":\""; json += jStr(cfg.wifiIpStatic); json += "\",";
  json += "\"wifi_subnet\":\"";    json += jStr(cfg.wifiSubnet);   json += "\",";
  json += "\"wifi_gateway\":\"";   json += jStr(cfg.wifiGateway);  json += "\",";
  json += "\"wifi_dns1\":\"";      json += jStr(cfg.wifiDns1);     json += "\",";
  json += "\"wifi_dns2\":\"";      json += jStr(cfg.wifiDns2);     json += "\",";
  json += "\"mqtt_server\":\"";    json += jStr(cfg.mqttServer);   json += "\",";
  json += "\"mqtt_port\":";        json += String(cfg.mqttPort); json += ",";
  json += "\"mqtt_user\":\"";      json += jStr(cfg.mqttUser);     json += "\",";
  json += "\"debug\":";            json += String(cfg.debug ? "true" : "false"); json += ",";
  json += "\"screen_mode\":";      json += String(cfg.screenMode); json += ",";
  json += "\"lang_file\":\"";      json += jStr(langManager.getActiveLanguageFile()); json += "\",";
  json += "\"lang_name\":\"";      json += jStr(langManager.getActiveLanguageName()); json += "\",";
  json += "\"time\":\"";           json += String(timeBuf); json += "\",";
  json += "\"date\":\"";           json += String(dateBuf); json += "\",";
  json += "\"ip\":\"";             json += WiFi.localIP().toString(); json += "\",";
  json += "\"version\":\"";        json += jStr(String(AQUARIUM_OS_VERSION)); json += "\",";
  json += "\"mqtt_enabled\":";     json += String(cfg.mqttEnabled ? "true" : "false"); json += ",";
  json += "\"mqtt_error\":";       json += String(aquarium.getMqttConnectAttempts() >= 3 ? "true" : "false");
  json += "}";

  DBG_PRINTF("[WEB] /api/status JSON len=%d generated in %lu ms\n", json.length(), millis() - t);
  m_server.send(200, "application/json", json);
}


void AquariumServer::handleApiToggleLight() {
  aquarium.toggleLight();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiToggleAuto() {
  aquarium.setAutoSchedule(!aquarium.isAutoSchedule());
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetSchedule() {
  if (m_server.hasArg("on_h") && m_server.hasArg("on_m") && m_server.hasArg("off_h") && m_server.hasArg("off_m")) {
    aquarium.setScheduleOn(m_server.arg("on_h").toInt(), m_server.arg("on_m").toInt());
    aquarium.setScheduleOff(m_server.arg("off_h").toInt(), m_server.arg("off_m").toInt());
    m_server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiSetTempSettings() {
  if (m_server.hasArg("min_t") && m_server.hasArg("max_t") && m_server.hasArg("offset")) {
    aquarium.setTargetTemp(m_server.arg("min_t").toFloat(), m_server.arg("max_t").toFloat());
    aquarium.setTempOffset(m_server.arg("offset").toFloat());
    m_server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiSetTimeSettings() {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  if (m_server.hasArg("timezone"))    { String tz = m_server.arg("timezone"); tz.trim(); if (tz.length() > 0) cfg.timezone = tz; }
  if (m_server.hasArg("format_hour")) { cfg.formatHour = m_server.arg("format_hour").toInt(); }
  if (m_server.hasArg("date_format")) { aquarium.setDateFormat(m_server.arg("date_format").toInt()); }
  if (m_server.hasArg("ntp1"))        { cfg.ntpServer1 = m_server.arg("ntp1"); }
  if (m_server.hasArg("ntp2"))        { cfg.ntpServer2 = m_server.arg("ntp2"); }
  xSemaphoreGive(g_configMutex);
  aquarium.syncNTP();
  g_saveConfigNeeded = true;
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiNtpSync() {
  aquarium.syncNTP();
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetRelaySettings() {
  if (m_server.hasArg("relay_inv")) {
    bool inv = (m_server.arg("relay_inv") == "1" || m_server.arg("relay_inv").equalsIgnoreCase("true"));
    if (inv != aquarium.isRelayInverted()) aquarium.toggleRelayInverted();
  }
  if (m_server.hasArg("on_h") && m_server.hasArg("on_m") && m_server.hasArg("off_h") && m_server.hasArg("off_m")) {
    aquarium.setScheduleOn(m_server.arg("on_h").toInt(), m_server.arg("on_m").toInt());
    aquarium.setScheduleOff(m_server.arg("off_h").toInt(), m_server.arg("off_m").toInt());
  }
  g_saveConfigNeeded = true;
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetWifiSettings() {
  if (m_server.hasArg("ssid")) {
    aquarium.connectWifiSSID(m_server.arg("ssid"), m_server.hasArg("pass") ? m_server.arg("pass") : "");
    m_server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiSetNetworkSettings() {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  if (m_server.hasArg("enabled")) {
    cfg.wifiStaticEnabled = (m_server.arg("enabled") == "1" || m_server.arg("enabled").equalsIgnoreCase("true"));
  }
  if (m_server.hasArg("ip")) cfg.wifiIpStatic = m_server.arg("ip");
  if (m_server.hasArg("subnet")) cfg.wifiSubnet = m_server.arg("subnet");
  if (m_server.hasArg("gateway")) cfg.wifiGateway = m_server.arg("gateway");
  if (m_server.hasArg("dns1")) cfg.wifiDns1 = m_server.arg("dns1");
  if (m_server.hasArg("dns2")) cfg.wifiDns2 = m_server.arg("dns2");
  xSemaphoreGive(g_configMutex);
  g_saveConfigNeeded = true;
  
  // Applica le nuove impostazioni di rete se connesso
  aquarium.connectWifiSSID(cfg.wifiSsid, cfg.wifiPassword);
  
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetMqttSettings() {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  if (m_server.hasArg("enabled")) {
    cfg.mqttEnabled = (m_server.arg("enabled") == "1" || m_server.arg("enabled") == "true");
  }
  if (m_server.hasArg("server")) cfg.mqttServer = m_server.arg("server");
  if (m_server.hasArg("port"))   cfg.mqttPort   = m_server.arg("port").toInt();
  if (m_server.hasArg("user"))   cfg.mqttUser   = m_server.arg("user");
  if (m_server.hasArg("pass") && m_server.arg("pass").length() > 0)
    cfg.mqttPassword = m_server.arg("pass");
  xSemaphoreGive(g_configMutex);
  aquarium.resetMqttRetries();
  g_saveConfigNeeded = true;
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiSetSystemSettings() {
  if (m_server.hasArg("debug"))       cfg.debug      = (m_server.arg("debug") == "true");
  if (m_server.hasArg("screen_mode")) cfg.screenMode = (uint8_t)m_server.arg("screen_mode").toInt();
  writeWholeConfigFileSafe(); // Salva subito
  m_server.send(200, "application/json", "{\"status\":\"rebooting\"}");
  delay(300);
  ESP.restart();
}

void AquariumServer::handleApiSetScreensaver() {
  if (m_server.hasArg("screensaver_t")) {
    uint16_t val = (uint16_t)constrain(m_server.arg("screensaver_t").toInt(), 0, 3600);
    aquarium.setScreensaverTime(val);
    m_server.send(200, "application/json", "{\"status\":\"ok\"}");
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiSetHardwareSettings() {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  xSemaphoreGive(g_configMutex);
  
  g_saveConfigNeeded = true;
  m_server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void AquariumServer::handleApiScanI2C() {
  String result = aquarium.scanI2C();
  String json = "{\"result\":\"" + jStr(result) + "\"}";
  m_server.send(200, "application/json", json);
}

void AquariumServer::handleApiSetLanguage() {
  if (m_server.hasArg("lang_file")) {
    aquarium.setLanguageFile(m_server.arg("lang_file"));
    m_server.send(200, "application/json", "{\"status\":\"rebooting\"}");
    delay(300);
    ESP.restart();
  } else {
    m_server.send(400, "application/json", "{\"error\":\"missing args\"}");
  }
}

void AquariumServer::handleApiConfigRawGet() {
  if (!SD.exists(CONFIG_PATH)) { m_server.send(404, "text/plain", "# Error: config.cfg not found"); return; }
  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (!f) { m_server.send(500, "text/plain", "# Error reading SD"); return; }
  m_server.send(200, "text/plain", f.readString());
  f.close();
}

void AquariumServer::handleApiConfigRawPost() {
  if (!m_server.hasArg("plain")) { m_server.send(400, "text/plain", "Empty body"); return; }
  String newContent = m_server.arg("plain");
  if (SD.exists(CONFIG_TMP_PATH)) SD.remove(CONFIG_TMP_PATH);
  File tmp = SD.open(CONFIG_TMP_PATH, FILE_WRITE);
  if (!tmp) { m_server.send(500, "text/plain", "Error opening tmp"); return; }
  tmp.print(newContent); tmp.flush(); tmp.close();
  if (SD.exists(CONFIG_BAK_PATH)) SD.remove(CONFIG_BAK_PATH);
  if (SD.exists(CONFIG_PATH)) SD.rename(CONFIG_PATH, CONFIG_BAK_PATH);
  if (SD.rename(CONFIG_TMP_PATH, CONFIG_PATH)) {
    if (SD.exists(CONFIG_BAK_PATH)) SD.remove(CONFIG_BAK_PATH);
    bool created = false, updated = false;
    loadConfigFromSD(created, updated);
    m_server.send(200, "text/plain", "OK");
  } else {
    m_server.send(500, "text/plain", "Error writing config.cfg");
  }
}
