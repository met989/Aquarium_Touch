#include "../include/aquarium_logic.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

extern AppConfig cfg;

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  if (String(topic) == "aquarium/command/light") {
    if (message == "ON") {
      aquarium.setLightManual(true);
    } else if (message == "OFF") {
      aquarium.setLightManual(false);
    } else if (message == "TOGGLE") {
      aquarium.toggleLight();
    }
  } else if (String(topic) == "aquarium/command/reboot") {
    if (message == "REBOOT") {
      ESP.restart();
    }
  }
}

struct SensorConfig {
    const char* id;
    const char* name;
    const char* devClass;
    const char* unit;
    const char* jsonKey;
};

const SensorConfig sensors[] = {
    {"temp", "Water Temperature", "temperature", "°C", "temperature"},
    {"ph", "Water pH", nullptr, "pH", "ph"},
    {"lux", "Ambient Light", "illuminance", "lx", "lightLux"},
    {"light_relay", "Light Relay", nullptr, nullptr, "lightOn"},
    {"ip", "IP Address", nullptr, nullptr, "ip"},
    {"light_mode", "Light Mode", nullptr, nullptr, "lightMode"},
    {"light_on_time", "Light On Time", nullptr, nullptr, "lightOnTime"},
    {"light_off_time", "Light Off Time", nullptr, nullptr, "lightOffTime"},
    {"relay_polarity", "Relay Polarity", nullptr, nullptr, "relayPolarity"},
    {"timezone", "Timezone", nullptr, nullptr, "timezone"},
    {"hour_format", "Hour Format", nullptr, nullptr, "hourFormat"},
    {"date_format", "Date Format", nullptr, nullptr, "dateFormat"},
    {"temp_target_min", "Temp Target Min", "temperature", "°C", "tempTargetMin"},
    {"temp_target_max", "Temp Target Max", "temperature", "°C", "tempTargetMax"},
    {"temp_offset", "Temp Offset", "temperature", "°C", "tempOffset"},
    {"ph_target_min", "pH Target Min", nullptr, "pH", "phTargetMin"},
    {"ph_target_max", "pH Target Max", nullptr, "pH", "phTargetMax"},
    {"ph_offset", "pH Offset", nullptr, "pH", "phOffset"},
    {"wifi_ssid", "Wi-Fi SSID", nullptr, nullptr, "wifiSSID"},
    {"ip_mode", "IP Mode", nullptr, nullptr, "ipMode"},
    {"subnet", "Subnet Mask", nullptr, nullptr, "subnet"},
    {"gateway", "Gateway", nullptr, nullptr, "gateway"},
    {"dns1", "DNS 1", nullptr, nullptr, "dns1"},
    {"dns2", "DNS 2", nullptr, nullptr, "dns2"},
    {"mqtt_enabled", "MQTT Enabled", nullptr, nullptr, "mqttEnabled"},
    {"mqtt_server", "MQTT Server", nullptr, nullptr, "mqttServer"},
    {"mqtt_port", "MQTT Port", nullptr, nullptr, "mqttPort"},
    {"mqtt_user", "MQTT User", nullptr, nullptr, "mqttUser"},
    {"email_enabled", "Email Enabled", nullptr, nullptr, "emailEnabled"},
    {"smtp_host", "SMTP Host", nullptr, nullptr, "smtpHost"},
    {"smtp_user", "SMTP User", nullptr, nullptr, "smtpUser"},
    {"smtp_port", "SMTP Port", nullptr, nullptr, "smtpPort"},
    {"smtp_ssl", "SMTP SSL", nullptr, nullptr, "smtpSsl"},
    {"smtp_sender", "SMTP Sender", nullptr, nullptr, "smtpSender"},
    {"smtp_recipient", "SMTP Recipient", nullptr, nullptr, "smtpRecipient"},
    {"language", "Language", nullptr, nullptr, "language"},
    {"debug_serial", "Debug Serial", nullptr, nullptr, "debugSerial"},
    {"screen_rotation", "Screen Rotation", nullptr, nullptr, "screenRotation"},
    {"screensaver_time", "Screensaver Time", nullptr, "s", "screensaverTime"}
};

int g_discoveryIndex = -1;
unsigned long g_lastDiscoveryPublish = 0;

void AquariumLogic::initMQTT() {
  if (cfg.mqttServer.length() > 0 && cfg.mqttServer != "0.0.0.0") {
    mqttClient.setBufferSize(2048);
    mqttClient.setServer(cfg.mqttServer.c_str(), cfg.mqttPort);
    mqttClient.setCallback(mqttCallback);
  }
}

bool AquariumLogic::isMqttConnected() const {
  return mqttClient.connected();
}

int AquariumLogic::getMqttState() const {
  return mqttClient.state();
}

void AquariumLogic::publishHomeAssistantDiscovery() {
  Serial.println("[MQTT] Starting background Home Assistant Discovery...");
  g_discoveryIndex = 0;
}

void AquariumLogic::reconnectMQTT() {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  String mServer = cfg.mqttServer;
  String mUser = cfg.mqttUser;
  String mPass = cfg.mqttPassword;
  xSemaphoreGive(g_configMutex);

  if (mServer.length() == 0 || mServer == "0.0.0.0") return;
  
  // Update the server in case it was changed via Web UI without rebooting
  mqttClient.setServer(mServer.c_str(), 1883);
  
  if (!mqttClient.connected()) {
      Serial.print("[MQTT] Attempting connection to ");
      Serial.print(mServer);
      Serial.print("...");
      
      String clientId = "AquariumTouch-";
      clientId += String(random(0xffff), HEX);
      
      bool success = false;
      if (mUser.length() > 0 && mPass.length() > 0) {
          success = mqttClient.connect(clientId.c_str(), mUser.c_str(), mPass.c_str());
      } else {
          success = mqttClient.connect(clientId.c_str());
      }
      
      if (success) {
          Serial.println("connected");
          publishHomeAssistantDiscovery();
          mqttClient.subscribe("aquarium/command/light");
          mqttClient.subscribe("aquarium/command/reboot");
      } else {
          Serial.print("failed, rc=");
          Serial.print(mqttClient.state());
          Serial.println(" try again in 5 seconds");
      }
  }
}

void AquariumLogic::updateMQTT() {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  bool mEnabled = cfg.mqttEnabled;
  String mServer = cfg.mqttServer;
  xSemaphoreGive(g_configMutex);

  if (!mEnabled || WiFi.status() != WL_CONNECTED || mServer.length() == 0 || mServer == "0.0.0.0") return;
  
  if (!mqttClient.connected()) {
      if (m_mqttConnectAttempts < 3) {
          static unsigned long lastReconnect = 0;
          if (millis() - lastReconnect > 30000) {
              lastReconnect = millis();
              reconnectMQTT();
              if (!mqttClient.connected()) {
                  m_mqttConnectAttempts++;
                  Serial.printf("[MQTT] Connection failed (rc=%d). Auto-retry %d/3\n", mqttClient.state(), m_mqttConnectAttempts);
              } else {
                  m_mqttConnectAttempts = 0;
              }
          }
      }
  } else {
      m_mqttConnectAttempts = 0;
      mqttClient.loop();
      
      if (g_discoveryIndex >= 0 && g_discoveryIndex < sizeof(sensors)/sizeof(sensors[0])) {
          if (millis() - g_lastDiscoveryPublish > 100) {
              g_lastDiscoveryPublish = millis();
              
              JsonDocument devDoc;
              devDoc["identifiers"][0] = "esp32_aquarium_touch";
              devDoc["name"] = "Aquarium Touch Panel";
              devDoc["model"] = "1.0";
              devDoc["manufacturer"] = "Custom";
              
              JsonDocument doc;
              doc["name"] = sensors[g_discoveryIndex].name;
              doc["state_topic"] = "aquarium/data";
              
              String valueTemplate = "{{ value_json.";
              valueTemplate += sensors[g_discoveryIndex].jsonKey;
              valueTemplate += " }}";
              doc["value_template"] = valueTemplate;
              
              if (sensors[g_discoveryIndex].devClass != nullptr) doc["device_class"] = sensors[g_discoveryIndex].devClass;
              if (sensors[g_discoveryIndex].unit != nullptr) doc["unit_of_measurement"] = sensors[g_discoveryIndex].unit;
              
              doc["unique_id"] = String("aq_") + sensors[g_discoveryIndex].id;
              doc["device"] = devDoc;
              
              String payload;
              serializeJson(doc, payload);
              
              String topic = String("homeassistant/sensor/aquarium_") + sensors[g_discoveryIndex].id + "/config";
              mqttClient.publish(topic.c_str(), payload.c_str(), true); // Retained
              
              g_discoveryIndex++;
              if (g_discoveryIndex >= sizeof(sensors)/sizeof(sensors[0])) {
                  Serial.println("[MQTT] Home Assistant Discovery complete.");
              }
          }
      } else if (millis() - m_lastMqttPublish > 2000) {
          m_lastMqttPublish = millis();
          
          JsonDocument doc;
          doc["temperature"] = m_currentTemp;
          doc["ph"] = m_currentPh;
          doc["lightLux"] = m_currentLdrValue;
          doc["lightOn"] = m_lightOn ? "ON" : "OFF";
          
          // Luce
          doc["lightMode"] = m_config.autoSchedule ? "AUTO" : "MANUAL";
          doc["lightOnTime"] = String(m_config.lightOnHour) + ":" + (m_config.lightOnMin < 10 ? "0" : "") + String(m_config.lightOnMin);
          doc["lightOffTime"] = String(m_config.lightOffHour) + ":" + (m_config.lightOffMin < 10 ? "0" : "") + String(m_config.lightOffMin);
          doc["relayPolarity"] = m_config.relayInverted ? "LOW" : "HIGH";
          
          // Tempo e Data
          doc["timezone"] = cfg.timezone;
          doc["hourFormat"] = cfg.formatHour;
          doc["dateFormat"] = m_config.dateFormat;
          
          // Sensori e Calibrazione
          doc["tempTargetMin"] = m_config.targetTempMin;
          doc["tempTargetMax"] = m_config.targetTempMax;
          doc["tempOffset"] = m_config.tempOffset;
          doc["phTargetMin"] = m_config.targetPhMin;
          doc["phTargetMax"] = m_config.targetPhMax;
          doc["phOffset"] = m_config.phOffset;
          
          // Rete
          doc["wifiSSID"] = cfg.wifiSsid;
          doc["ipMode"] = cfg.wifiStaticEnabled ? "STATIC" : "DHCP";
          doc["ip"] = WiFi.localIP().toString();
          doc["subnet"] = cfg.wifiSubnet;
          doc["gateway"] = cfg.wifiGateway;
          doc["dns1"] = cfg.wifiDns1;
          doc["dns2"] = cfg.wifiDns2;
          
          // MQTT
          doc["mqttEnabled"] = cfg.mqttEnabled;
          doc["mqttServer"] = cfg.mqttServer;
          doc["mqttPort"] = cfg.mqttPort;
          doc["mqttUser"] = cfg.mqttUser;
          
          // Email
          doc["emailEnabled"] = cfg.emailEnabled;
          doc["smtpHost"] = cfg.smtpHost;
          doc["smtpUser"] = cfg.smtpUser;
          doc["smtpPort"] = cfg.smtpPort;
          doc["smtpSsl"] = cfg.smtpSsl;
          doc["smtpSender"] = cfg.emailSender;
          doc["smtpRecipient"] = cfg.emailRecipients;
          
          // Sistema
          doc["language"] = m_config.langFile;
          doc["debugSerial"] = cfg.debug;
          doc["screenRotation"] = cfg.screenMode;
          doc["screensaverTime"] = m_config.screensaverTime;
          
          String payload;
          serializeJson(doc, payload);
          mqttClient.publish("aquarium/data", payload.c_str());
      }
  }
}
