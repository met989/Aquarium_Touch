#ifndef AQUARIUM_LOGIC_H
#define AQUARIUM_LOGIC_H

#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_MCP23X17.h>
#include "config.h"

enum WifiConnectState {
  WIFI_CONN_IDLE,
  WIFI_CONN_CONNECTING,
  WIFI_CONN_SUCCESS,
  WIFI_CONN_FAILED,
  WIFI_CONN_WAIT_SHORT,
  WIFI_CONN_WAIT_LONG
};

enum TempStatus {
  TEMP_OPTIMAL = 0,
  TEMP_TOO_COLD = 1,
  TEMP_TOO_HOT = 2
};

struct AquariumConfig {
  uint8_t lightOnHour   = 10;
  uint8_t lightOnMin    = 0;
  uint8_t lightOffHour  = 18;
  uint8_t lightOffMin   = 0;
  bool autoSchedule     = true;
  float targetTempMin   = DEFAULT_TEMP_MIN;
  float targetTempMax   = DEFAULT_TEMP_MAX;
  float tempOffset      = 0.0f;
  bool relayInverted    = false; // false = Active HIGH, true = Active LOW
  uint8_t dateFormat    = DEFAULT_DATE_FORMAT; // 0: DD/MM/YYYY, 1: MM/DD/YYYY, 2: YYYY/MM/DD
  char langFile[64]     = DEFAULT_LANGUAGE_FILE;
  uint16_t screensaverTime = 0; // seconds; 0 = disabled (MAI)
  bool autoDimming      = false;
};

struct WifiNetworkItem {
  String ssid;
  int rssi;
  bool open;
};

class AquariumLogic {
public:
  AquariumLogic();
  void init();
  void update();

  // Temperature Methods
  float getTemperature() const { return m_currentTemp; }
  float getPhValue() const { return m_currentPh; }
  bool isWaterLevelOk();
  float getMinTemp() const { return m_minTemp; }
  float getMaxTemp() const { return m_maxTemp; }
  bool isSensorConnected() const { return m_sensorConnected; }
  TempStatus getTempStatus() const;

  // Light & Relay  // Getters
  bool isLightOn() const { return m_lightOn; }
  bool isAutoSchedule() const { return m_config.autoSchedule; }
  bool isRelayInverted() const { return m_config.relayInverted; }
  void setLightManual(bool on);
  void toggleLight();
  void toggleRelayInverted();
  void setAutoSchedule(bool enable);

  // Screen & LDR
  uint16_t getLdrValue() const { return m_currentLdrValue; }
  void setAutoDimming(bool enable);
  void updateBacklight();

  // Settings
  AquariumConfig& getConfig() { return m_config; }

  // OTA Update
  bool checkGitHubForUpdate(String& outVersion, String& outUrl);
  void performOTAUpdate(const String& url);

  // MQTT
  void initMQTT();
  void updateMQTT();
  void reconnectMQTT();
  void publishHomeAssistantDiscovery();
  void resetMqttRetries() { m_mqttConnectAttempts = 0; }
  int getMqttConnectAttempts() const { return m_mqttConnectAttempts; }

  // WiFi Management Methods
  bool isWifiConnected() const;
  String getWifiIP() const;
  int getWifiRSSI() const;
  void connectWifi();
  void connectWifiSSID(const String& ssid, const String& password);
  void disconnectWifi();
  void scanWifi();
  void startAsyncWifiScan();

  bool isWifiScanning() const { return m_wifiScanning; }
  int getWifiNetworkCount() const { return m_wifiNetworkCount; }
  WifiNetworkItem getWifiNetwork(int idx) const;
  String getWifiScanStatus() const { return m_wifiScanStatus; }
  uint8_t getWifiScanCounter() const { return m_wifiScanCounter; }
  WifiConnectState getWifiConnectState() const { return m_wifiConnectState; }
  void resetWifiConnectState() { m_wifiConnectState = WIFI_CONN_IDLE; }
  String getWifiConnectSSID() const;

  // I2C Scanner
  String scanI2C() const;



  // Time & Clock Methods
  void getTime(int& h, int& m, int& s) const;
  void getDate(int& day, int& month, int& year) const;
  void getFormattedDate(char* outBuf, size_t maxLen) const;
  void setTime(int h, int m);
  void setDateFormat(uint8_t format);
  void setLanguageFile(const String& langPath);
  bool syncNTP();
  String getNtpStatus() const { return m_ntpStatus; }





  // Schedule Configuration
  const AquariumConfig& getConfig() const { return m_config; }
  void setScheduleOn(uint8_t h, uint8_t m);
  void setScheduleOff(uint8_t h, uint8_t m);
  void setTargetTemp(float minT, float maxT);
  void setTempOffset(float offset);
  void setScreensaverTime(uint16_t seconds);

  // SD Config Serialization
  bool loadConfigSD();
  bool saveConfigSD();

private:
  void readSensor();
  void evaluateSchedule();
  void applyLightHardware();
  void startWifiConnection();

  AquariumConfig m_config;
  float m_currentTemp = 0.0f;
  float m_minTemp = 100.0f;
  float m_maxTemp = -100.0f;
  bool m_sensorConnected = false;
  
  float m_currentPh = 7.0f;
  uint32_t m_lastPhReadTime = 0;

  Adafruit_MCP23X17 m_mcp;
  bool m_mcpReady = false;

  bool m_lightOn = false;
  bool m_manualOverride = false;

  uint16_t m_currentLdrValue = 0;
  uint32_t m_lastSensorRead = 0;
  uint32_t m_lastTimeUpdate = 0;
  uint32_t m_lastMqttPublish = 0;
  uint32_t m_uptimeSeconds = 32400; // Simulated start at 09:00:00 AM
  String m_ntpStatus = "NTP: Inattivo";
  String m_wifiScanStatus = "Premi Scansiona";

  bool m_wifiScanning = false;
  uint8_t m_wifiScanCounter = 0;
  int m_wifiNetworkCount = 0;
  WifiNetworkItem m_scannedNetworks[12];

  WifiConnectState m_wifiConnectState = WIFI_CONN_IDLE;
  String m_wifiConnectSSID;
  uint32_t m_wifiConnectStartTime = 0;
  uint32_t m_wifiConnectResultTime = 0;
  uint32_t m_wifiWaitStartTime = 0;
  uint8_t m_wifiAutoRetries = 0;
  uint8_t m_mqttConnectAttempts = 0;
};



bool writeWholeConfigFileSafe();
bool loadConfigFromSD(bool& created, bool& updated);

extern AquariumLogic aquarium;

#endif // AQUARIUM_LOGIC_H

