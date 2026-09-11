#ifndef AQUARIUM_SERVER_H
#define AQUARIUM_SERVER_H

#include <Arduino.h>
#include <WebServer.h>

class AquariumServer {
public:
  AquariumServer();
  void init();
  void update();
  bool isStarted() const { return m_started; }
  void handleClient();

private:
  void setupRoutes();
  void handleRoot();
  void handleApiStatus();
  void handleApiToggleLight();
  void handleApiToggleAuto();
  void handleApiSetSchedule();
  void handleApiToggleRelayInvert();
  void handleApiSetTempSettings();
  void handleApiSetTimeSettings();
  void handleApiNtpSync();
  void handleApiSetRelaySettings();
  void handleApiSetWifiSettings();
  void handleApiSetNetworkSettings();
  void handleApiSetMqttSettings();
  void handleApiSetSystemSettings();
  void handleApiSetScreensaver();
  void handleApiSetLanguage();
  void handleApiSetHardwareSettings();
  void handleApiScanI2C();
  void handleApiConfigRawGet();
  void handleApiConfigRawPost();


  WebServer m_server;
  bool m_started;
};

extern AquariumServer aquariumServer;

#endif // AQUARIUM_SERVER_H
