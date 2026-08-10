#ifndef AQUARIUM_UI_H
#define AQUARIUM_UI_H

#include <Arduino.h>
#include <TFT_eSPI.h>
#include "config.h"

enum UiTab {
  TAB_DASHBOARD = 0,
  TAB_LIGHT = 1,
  TAB_SCHEDULE = 2,
  TAB_SETTINGS = 3
};

struct WaterBubble {
  float x, y;
  float radius;
  float speed;
  float wobble;
  float wobbleSpeed;
};

struct SwimmingFish {
  float x, y;
  float speed;
  int dir; // 1 = right, -1 = left
  float tailPhase;
  uint16_t color;
};

class AquariumUI {
public:
  AquariumUI();
  void init(TFT_eSPI* tft);
  void update();
  void handleTouch(int touchX, int touchY);

  UiTab getActiveTab() const { return m_activeTab; }
  void setTab(UiTab tab);

  
private:
  void initAnimations();
  void updateAnimations();

  // Screen Rendering
  void drawOceanBackground();
  void drawWaterWaves();
  void drawSwimmingFish();
  void drawNavBar(bool force = false);


  // Tab Content Screens (rendered inside 320x172 m_sprite)
  void drawDashboardTab();
  void drawLightTab();
  void drawScheduleTab();

  // Settings Menu & Sub-Screens
  void drawSettingsTab();
  void drawSettingsMenu();
  void drawSettingsSubScreen(int sub);
  void drawSubScreenTempTarget();
  void drawSubScreenRelay();
  void drawSubScreenWifi();
  void drawSubScreenVersion();
  void drawSubScreenLanguage();
  void drawSubScreenEnergySaving();
  void drawSubScreenFactoryReset();
  void drawWifiKeyboard();


  // Custom Widgets
  void drawCircularTempGauge(int cx, int cy, int radius, float temp, float minT, float maxT);
  void drawGlassCard(int x, int y, int w, int h, uint16_t borderColor = 0x2C18, uint16_t bgCol = 0x11EC);
  void drawTouchButton(int x, int y, int w, int h, const char* label, uint16_t bgCol, uint16_t textCol, uint16_t borderCol = 0x07FF);

  TFT_eSPI* m_tft = nullptr;
  TFT_eSprite* m_sprite = nullptr;

  UiTab m_activeTab = TAB_DASHBOARD;
  int m_settingsSubScreen = 0; 
  int m_settingsMenuPage = 0;  // 0-indexed page for Settings Menu (4 items per page)
  bool m_settingsNeedsRedraw = true;
  int m_lastDrawTab = -1;
  int m_lastDrawSec = -1;

  // WiFi & On-Screen Keyboard State
  int m_wifiSelectedNet = -1;
  int m_wifiListPage = 0;
  bool m_wifiShowKeyboard = false;
  String m_wifiSelectedSSID = "";
  String m_wifiTypedPassword = "";
  bool m_wifiHidePassword = true;
  int m_kbLayoutMode = 0; // 0 = abc (lower), 1 = ABC (upper), 2 = 123/sym (numbers)
  int m_resetStep = 0;
  int m_screensaverIdx = 0; // index into screensaver time table



  WaterBubble m_bubbles[UI_BUBBLE_COUNT];
  SwimmingFish m_fish[UI_FISH_COUNT];


  float m_wavePhase = 0.0f;
  uint32_t m_lastAnimTime = 0;
  uint32_t m_lastTouchTime = 0;
  uint8_t m_lastWifiScanCounter = 0;

  // Temperature Interpolation for Smooth Needle
  float m_animatedTemp = 25.4f;
};

extern AquariumUI aquariumUI;

#endif // AQUARIUM_UI_H
