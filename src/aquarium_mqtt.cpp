#include "../include/aquarium_logic.h"
#include <ArduinoJson.h>
#include <PubSubClient.h>

WiFiClient espClient;
PubSubClient mqttClient(espClient);

extern AppConfig cfg;

void AquariumLogic::initMQTT() {
  if (cfg.mqttServer.length() > 0 && cfg.mqttServer != "0.0.0.0") {
    mqttClient.setBufferSize(1024);
    mqttClient.setServer(cfg.mqttServer.c_str(), cfg.mqttPort);
  }
}

void AquariumLogic::publishHomeAssistantDiscovery() {
  Serial.println("[MQTT] Publishing Home Assistant Discovery for Aquarium...");
  
  JsonDocument devDoc;
  devDoc["identifiers"][0] = "esp32_aquarium_touch";
  devDoc["name"] = "Aquarium Touch Panel";
  devDoc["model"] = "1.0";
  devDoc["manufacturer"] = "Custom";
  
  struct SensorConfig {
      const char* id;
      const char* name;
      const char* devClass;
      const char* unit;
      const char* jsonKey;
  };
  
  SensorConfig sensors[] = {
      {"temp", "Water Temperature", "temperature", "°C", "temperature"},
      {"ph", "Water pH", nullptr, "pH", "ph"},
      {"lux", "Ambient Light", "illuminance", "lx", "lightLux"},
      {"light_relay", "Light Relay", nullptr, nullptr, "lightOn"},
      {"ip", "IP Address", nullptr, nullptr, "ip"}
  };
  
  for (int i = 0; i < sizeof(sensors)/sizeof(sensors[0]); i++) {
      JsonDocument doc;
      doc["name"] = sensors[i].name;
      doc["state_topic"] = "aquarium/data";
      
      String valueTemplate = "{{ value_json.";
      valueTemplate += sensors[i].jsonKey;
      valueTemplate += " }}";
      doc["value_template"] = valueTemplate;
      
      if (sensors[i].devClass != nullptr) doc["device_class"] = sensors[i].devClass;
      if (sensors[i].unit != nullptr) doc["unit_of_measurement"] = sensors[i].unit;
      
      doc["unique_id"] = String("aq_") + sensors[i].id;
      doc["device"] = devDoc;
      
      String payload;
      serializeJson(doc, payload);
      
      String topic = String("homeassistant/sensor/aquarium_") + sensors[i].id + "/config";
      mqttClient.publish(topic.c_str(), payload.c_str(), true); // Retained
      delay(10);
  }
}

void AquariumLogic::reconnectMQTT() {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  String mServer = cfg.mqttServer;
  String mUser = cfg.mqttUser;
  String mPass = cfg.mqttPassword;
  xSemaphoreGive(g_configMutex);

  if (mServer.length() == 0 || mServer == "0.0.0.0") return;
  
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
      } else {
          Serial.print("failed, rc=");
          Serial.print(mqttClient.state());
          Serial.println(" try again in 5 seconds");
      }
  }
}

void AquariumLogic::updateMQTT() {
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  String mServer = cfg.mqttServer;
  xSemaphoreGive(g_configMutex);

  if (WiFi.status() != WL_CONNECTED || mServer.length() == 0 || mServer == "0.0.0.0") return;
  
  if (!mqttClient.connected()) {
      static unsigned long lastReconnect = 0;
      if (millis() - lastReconnect > 5000) {
          lastReconnect = millis();
          reconnectMQTT();
      }
  } else {
      mqttClient.loop();
      
      if (millis() - m_lastMqttPublish > 2000) {
          m_lastMqttPublish = millis();
          
          JsonDocument doc;
          doc["temperature"] = m_currentTemp;
          doc["ph"] = m_currentPh;
          doc["lightLux"] = m_currentLdrValue;
          doc["lightOn"] = m_lightOn ? "ON" : "OFF";
          doc["ip"] = WiFi.localIP().toString();
          
          String payload;
          serializeJson(doc, payload);
          mqttClient.publish("aquarium/data", payload.c_str());
      }
  }
}
