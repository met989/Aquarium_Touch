#include "../include/aquarium_logic.h"
#include "../include/aquarium_server.h"
#include "../include/config.h"
#include <Arduino.h>
#include <SD.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <esp_system.h>
#include <vector>
#include <AnimatedGIF.h>
TFT_eSPI tft = TFT_eSPI();

SPIClass *touchSPI = nullptr;
XPT2046_Touchscreen touch(TOUCH_CS, TOUCH_IRQ);

// AppConfig and TouchCal are defined in config.h

AppConfig cfg;
uint16_t physW = 320, physH = 240;
unsigned long lastTouchLog = 0;

void backlightOn() {
  aquarium.updateBacklight();
}

void backlightOff() {
  ledcWrite(0, 0); // Spenge fisicamente il ledc
}

bool g_screenIsOff = false;
unsigned long g_lastActivityMs = 0;

void showMessage(const String &title, const String &msg,
                 uint16_t bg = TFT_BLACK, uint16_t fg = TFT_WHITE) {
  tft.fillScreen(bg);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(fg, bg);
  tft.drawString(title, 10, 10, 4);
  tft.drawString(msg, 10, 55, 2);
}

String stripQuotes(String v) {
  v.trim();
  if (v.length() >= 2 && v.startsWith("\"") && v.endsWith("\""))
    v = v.substring(1, v.length() - 1);
  return v;
}

void setDefaults() {
  cfg.screenMode = 3;
  cfg.wifiSsid = "SSID_WIFI";
  cfg.wifiPassword = "PASSWORD_WIFI";
  cfg.wifiIpStatic = "0.0.0.0";
  cfg.wifiSubnet = "0.0.0.0";
  cfg.wifiGateway = "0.0.0.0";
  cfg.wifiDns1 = "0.0.0.0";
  cfg.wifiDns2 = "0.0.0.0";
  cfg.weatherApiKey = "APIKEY_METEO";
  cfg.weatherCity = "Italia";
  cfg.ntpServer1 = DEFAULT_NTP_SERVER1;
  cfg.ntpServer2 = DEFAULT_NTP_SERVER2;
  cfg.timezone = DEFAULT_TIMEZONE;

  for (int i = 0; i < 4; i++)
    cfg.touch[i] = {0, 0, 0, 0};
  cfg.mqttServer = "0.0.0.0";
  cfg.mqttPort = 1883;
  cfg.mqttEnabled = false;
  cfg.mqttUser = "utente";
  cfg.mqttPassword = "pass";
  cfg.formatHour = 24;
  cfg.debug = false;
}

bool parseConfigLine(const String &rawLine) {
  String line = rawLine;
  line.trim();
  if (line.length() == 0 || line.startsWith("#"))
    return true;
  int eq = line.indexOf('=');
  if (eq < 0)
    return false;
  String key = line.substring(0, eq);
  key.trim();
  String val = line.substring(eq + 1);
  val.trim();

  if (key.equalsIgnoreCase("screen_mode"))
    cfg.screenMode = val.toInt();
  else if (key.equalsIgnoreCase("wifi_ssid"))
    cfg.wifiSsid = stripQuotes(val);
  else if (key.equalsIgnoreCase("wifi_password"))
    cfg.wifiPassword = stripQuotes(val);
  else if (key.equalsIgnoreCase("wifi_ip_static"))
    cfg.wifiIpStatic = stripQuotes(val);
  else if (key.equalsIgnoreCase("wifi_subnet"))
    cfg.wifiSubnet = stripQuotes(val);
  else if (key.equalsIgnoreCase("wifi_gateway"))
    cfg.wifiGateway = stripQuotes(val);
  else if (key.equalsIgnoreCase("wifi_dns1"))
    cfg.wifiDns1 = stripQuotes(val);
  else if (key.equalsIgnoreCase("wifi_dns2"))
    cfg.wifiDns2 = stripQuotes(val);
  else if (key.equalsIgnoreCase("weather_api_key"))
    cfg.weatherApiKey = stripQuotes(val);
  else if (key.equalsIgnoreCase("weather_city"))
    cfg.weatherCity = stripQuotes(val);
  else if (key.equalsIgnoreCase("ntp_server1"))
    cfg.ntpServer1 = stripQuotes(val);
  else if (key.equalsIgnoreCase("ntp_server2"))
    cfg.ntpServer2 = stripQuotes(val);
  else if (key.equalsIgnoreCase("timezone")) {
    String tzVal = stripQuotes(val);
    if (tzVal.startsWith("CET") || tzVal.length() == 0 ||
        (!tzVal.startsWith("+") && !tzVal.startsWith("-") &&
         !isDigit(tzVal.charAt(0)))) {
      tzVal = DEFAULT_TIMEZONE;
    }
    cfg.timezone = tzVal;
  }

  else if (key.equalsIgnoreCase("touch_min_x0"))
    cfg.touch[0].minX = val.toInt();
  else if (key.equalsIgnoreCase("touch_max_x0"))
    cfg.touch[0].maxX = val.toInt();
  else if (key.equalsIgnoreCase("touch_min_y0"))
    cfg.touch[0].minY = val.toInt();
  else if (key.equalsIgnoreCase("touch_max_y0"))
    cfg.touch[0].maxY = val.toInt();
  else if (key.equalsIgnoreCase("touch_min_x1"))
    cfg.touch[1].minX = val.toInt();
  else if (key.equalsIgnoreCase("touch_max_x1"))
    cfg.touch[1].maxX = val.toInt();
  else if (key.equalsIgnoreCase("touch_min_y1"))
    cfg.touch[1].minY = val.toInt();
  else if (key.equalsIgnoreCase("touch_max_y1"))
    cfg.touch[1].maxY = val.toInt();
  else if (key.equalsIgnoreCase("touch_min_x2"))
    cfg.touch[2].minX = val.toInt();
  else if (key.equalsIgnoreCase("touch_max_x2"))
    cfg.touch[2].maxX = val.toInt();
  else if (key.equalsIgnoreCase("touch_min_y2"))
    cfg.touch[2].minY = val.toInt();
  else if (key.equalsIgnoreCase("touch_max_y2"))
    cfg.touch[2].maxY = val.toInt();
  else if (key.equalsIgnoreCase("touch_min_x3"))
    cfg.touch[3].minX = val.toInt();
  else if (key.equalsIgnoreCase("touch_max_x3"))
    cfg.touch[3].maxX = val.toInt();
  else if (key.equalsIgnoreCase("touch_min_y3"))
    cfg.touch[3].minY = val.toInt();
  else if (key.equalsIgnoreCase("touch_max_y3"))
    cfg.touch[3].maxY = val.toInt();
  else if (key.equalsIgnoreCase("mqtt_server"))
    cfg.mqttServer = stripQuotes(val);
  else if (key.equalsIgnoreCase("mqtt_enabled"))
    cfg.mqttEnabled = (val == "1" || val.equalsIgnoreCase("true"));
  else if (key.equalsIgnoreCase("mqtt_port"))
    cfg.mqttPort = val.toInt();
  else if (key.equalsIgnoreCase("mqtt_user"))
    cfg.mqttUser = stripQuotes(val);
  else if (key.equalsIgnoreCase("mqtt_password"))
    cfg.mqttPassword = stripQuotes(val);
  else if (key.equalsIgnoreCase("format_hour"))
    cfg.formatHour = val.toInt();
  else if (key.equalsIgnoreCase("debug"))
    cfg.debug = (val.equalsIgnoreCase("true") || val == "1");
  else if (key.equalsIgnoreCase("light_on_h"))
    aquarium.setScheduleOn(val.toInt(), aquarium.getConfig().lightOnMin);
  else if (key.equalsIgnoreCase("light_on_m"))
    aquarium.setScheduleOn(aquarium.getConfig().lightOnHour, val.toInt());
  else if (key.equalsIgnoreCase("light_off_h"))
    aquarium.setScheduleOff(val.toInt(), aquarium.getConfig().lightOffMin);
  else if (key.equalsIgnoreCase("light_off_m"))
    aquarium.setScheduleOff(aquarium.getConfig().lightOffHour, val.toInt());
  else if (key.equalsIgnoreCase("auto_sched"))
    aquarium.setAutoSchedule(val == "1" || val.equalsIgnoreCase("true"));
  else if (key.equalsIgnoreCase("target_min_t"))
    aquarium.setTargetTemp(val.toFloat(), aquarium.getConfig().targetTempMax);
  else if (key.equalsIgnoreCase("target_max_t"))
    aquarium.setTargetTemp(aquarium.getConfig().targetTempMin, val.toFloat());
  else if (key.equalsIgnoreCase("temp_offset"))
    aquarium.setTempOffset(val.toFloat());
  else if (key.equalsIgnoreCase("relay_inv")) {
    if ((val == "1" || val.equalsIgnoreCase("true")) !=
        aquarium.isRelayInverted())
      aquarium.toggleRelayInverted();
  } else if (key.equalsIgnoreCase("date_format"))
    aquarium.setDateFormat(val.toInt() % 3);
  else if (key.equalsIgnoreCase("lang_file"))
    aquarium.setLanguageFile(stripQuotes(val));
  else if (key.equalsIgnoreCase("screensaver_t"))
    aquarium.setScreensaverTime((uint16_t)constrain(val.toInt(), 0, 3600));
  else if (key.equalsIgnoreCase("auto_dimming"))
    aquarium.setAutoDimming(val == "1" || val.equalsIgnoreCase("true"));

  else
    DBG_PRINTF("[CFG] Chiave ignorata: %s\n", key.c_str());
  return true;
}

String buildConfigText() {
  String out;
  out.reserve(2000);
  out += "# Config file\n";
  out += "screen_mode=" + String(cfg.screenMode) + "\n";
  out += "wifi_ssid=\"" + cfg.wifiSsid + "\"\n";
  out += "wifi_password=\"" + cfg.wifiPassword + "\"\n";
  out += "wifi_static_en=" + String(cfg.wifiStaticEnabled ? "1" : "0") + "\n";
  out += "wifi_ip_static=" + cfg.wifiIpStatic + "\n";
  out += "wifi_subnet=" + cfg.wifiSubnet + "\n";
  out += "wifi_gateway=" + cfg.wifiGateway + "\n";
  out += "wifi_dns1=" + cfg.wifiDns1 + "\n";
  out += "wifi_dns2=" + cfg.wifiDns2 + "\n";
  out += "weather_api_key=\"" + cfg.weatherApiKey + "\"\n";
  out += "weather_city=\"" + cfg.weatherCity + "\"\n";
  out += "ntp_server1=\"" + cfg.ntpServer1 + "\"\n";
  out += "ntp_server2=\"" + cfg.ntpServer2 + "\"\n";
  String tzVal = cfg.timezone;
  if (tzVal.startsWith("CET") || tzVal.length() == 0 ||
      (!tzVal.startsWith("+") && !tzVal.startsWith("-") &&
       !isDigit(tzVal.charAt(0)))) {
    tzVal = DEFAULT_TIMEZONE;
  }
  cfg.timezone = tzVal;
  out += "timezone=\"" + cfg.timezone + "\"\n";

  for (int i = 0; i < 4; i++) {
    out += "touch_min_x" + String(i) + "=" + String(cfg.touch[i].minX) + "\n";
    out += "touch_max_x" + String(i) + "=" + String(cfg.touch[i].maxX) + "\n";
    out += "touch_min_y" + String(i) + "=" + String(cfg.touch[i].minY) + "\n";
    out += "touch_max_y" + String(i) + "=" + String(cfg.touch[i].maxY) + "\n";
  }
  out += "mqtt_enabled=" + String(cfg.mqttEnabled ? "1" : "0") + "\n";
  out += "mqtt_server=\"" + cfg.mqttServer + "\"\n";
  out += "mqtt_port=" + String(cfg.mqttPort) + "\n";
  out += "mqtt_user=\"" + cfg.mqttUser + "\"\n";
  out += "mqtt_password=\"" + cfg.mqttPassword + "\"\n";
  out += "format_hour=" + String(cfg.formatHour) + "\n";
  out += "debug=" + String(cfg.debug ? "true" : "false") + "\n";

  const AquariumConfig &aq = aquarium.getConfig();
  out += "light_on_h=" + String(aq.lightOnHour) + "\n";
  out += "light_on_m=" + String(aq.lightOnMin) + "\n";
  out += "light_off_h=" + String(aq.lightOffHour) + "\n";
  out += "light_off_m=" + String(aq.lightOffMin) + "\n";
  out += "auto_sched=" + String(aq.autoSchedule ? "1" : "0") + "\n";
  out += "target_min_t=" + String(aq.targetTempMin, 1) + "\n";
  out += "target_max_t=" + String(aq.targetTempMax, 1) + "\n";
  out += "temp_offset=" + String(aq.tempOffset, 1) + "\n";
  out += "relay_inv=" + String(aq.relayInverted ? "1" : "0") + "\n";
  out += "date_format=" + String(aq.dateFormat) + "\n";
  String cleanLang = String(aq.langFile);
  while (cleanLang.startsWith("\"") && cleanLang.endsWith("\"") &&
         cleanLang.length() >= 2) {
    cleanLang = cleanLang.substring(1, cleanLang.length() - 1);
  }
  if (cleanLang.startsWith("/languages/"))
    cleanLang = cleanLang.substring(11);
  else if (cleanLang.startsWith("/"))
    cleanLang = cleanLang.substring(1);
  if (cleanLang.length() == 0)
    cleanLang = "english.lng";

  out += "lang_file=\"" + cleanLang + "\"\n";
  out += "screensaver_t=" + String(aquarium.getConfig().screensaverTime) + "\n";
  out += "auto_dimming=" + String(aquarium.getConfig().autoDimming ? "1" : "0") + "\n";
  return out;
}

bool g_loadingConfig = false;
SemaphoreHandle_t g_configMutex = NULL;
volatile bool g_saveConfigNeeded = false;

bool writeWholeConfigFileSafe() {
  if (g_loadingConfig)
    return true;
  String content = buildConfigText();
  DBG_PRINTF("[FILE] Scrittura %u byte\n", (unsigned)content.length());

  if (SD.exists(CONFIG_TMP_PATH))
    SD.remove(CONFIG_TMP_PATH);
  delay(30);

  File tmp = SD.open(CONFIG_TMP_PATH, FILE_WRITE);
  if (!tmp) {
    DBG_PRINTLN("[FILE] ERRORE: impossibile aprire /config.tmp");
    return false;
  }

  size_t written = tmp.print(content);
  tmp.flush();
  tmp.close();

  if (written != content.length()) {
    DBG_PRINTLN("[FILE] Scrittura incompleta");
    return false;
  }

  if (SD.exists(CONFIG_BAK_PATH))
    SD.remove(CONFIG_BAK_PATH);
  delay(20);
  if (SD.exists(CONFIG_PATH)) {
    SD.rename(CONFIG_PATH, CONFIG_BAK_PATH);
    delay(20);
  }
  if (!SD.rename(CONFIG_TMP_PATH, CONFIG_PATH)) {
    DBG_PRINTLN("[FILE] ERRORE rename config.tmp->config.cfg");
    return false;
  }
  if (SD.exists(CONFIG_BAK_PATH))
    SD.remove(CONFIG_BAK_PATH);
  DBG_PRINTLN("[FILE] Salvato /config.cfg OK");
  return true;
}

bool loadConfigFromSD(bool &created, bool &updated) {
  created = false;
  updated = false;
  if (!SD.exists(CONFIG_PATH)) {
    created = true;
    return writeWholeConfigFileSafe();
  }

  g_loadingConfig = true;
  File f = SD.open(CONFIG_PATH, FILE_READ);
  if (!f) {
    g_loadingConfig = false;
    return false;
  }

  std::vector<String> lines;
  while (f.available()) {
    lines.push_back(f.readStringUntil('\n'));
  }
  f.close();

  for (const String &line : lines) {
    parseConfigLine(line);
  }

  g_loadingConfig = false;
  return true;
}



void applyScreenMode() {
  static const uint8_t rotTable[4] = {2, 1, 0, 3};
  static const uint8_t madctlTable[4] = {0xE0, 0x40, 0x20, 0x80};
  uint8_t mode = (cfg.screenMode > 3) ? 1 : cfg.screenMode;
  tft.setRotation(rotTable[mode]);
  tft.writecommand(0x36);
  tft.writedata(madctlTable[mode]);
  if (mode == 0 || mode == 2) {
    physW = 240;
    physH = 320;
    tft.setWindow(0, 0, 240, 320);
  } else {
    physW = 320;
    physH = 240;
    tft.setWindow(0, 0, 320, 240);
  }
}

void printSDInfo() {
  if (!cfg.debug)
    return;
  uint8_t t = SD.cardType();
  Serial.printf("[SD] cardType=%u\n", t);
  if (t == CARD_NONE)
    return;
  Serial.printf("[SD] %llu MB total=%llu used=%llu\n",
                SD.cardSize() / (1024ULL * 1024ULL), SD.totalBytes(),
                SD.usedBytes());
}

void testSDWrite() {
  if (!cfg.debug)
    return;
  const char *testPath = "/test.txt";
  DBG_PRINTF("[SD] Test writing %s\n", testPath);
  if (SD.exists(testPath))
    SD.remove(testPath);
  File f = SD.open(testPath, FILE_WRITE);
  if (!f) {
    DBG_PRINTLN("[SD] test FAILED open");
    return;
  }
  size_t written = f.println("test");
  f.flush();
  f.close();
  if (written == 0) {
    DBG_PRINTLN("[SD] test write FAILED");
    return;
  }
  if (!SD.exists(testPath)) {
    DBG_PRINTLN("[SD] test FAILED check existence");
    return;
  }
  if (SD.remove(testPath))
    DBG_PRINTLN("[SD] Test OK, file removed");
  else
    DBG_PRINTLN("[SD] test OK, but file removal failed");
}

void listRootFiles() {
  if (!cfg.debug)
    return;
  File root = SD.open("/");
  if (!root)
    return;
  DBG_PRINTLN("[SD] Root:");
  File file = root.openNextFile();
  while (file) {
    Serial.printf("  - %s (%u byte)\n", file.name(), (unsigned)file.size());
    file.close();
    file = root.openNextFile();
  }
  root.close();
}

bool initSD() {
  DBG_PRINTF("[SD] init CS=%d\n", SD_CS);

  if (!SD.begin(SD_CS)) {
    showMessage("SD ERROR", "SD card not readable", TFT_RED, TFT_WHITE);
    return false;
  }
  printSDInfo();
  testSDWrite();
  listRootFiles();
  return true;
}

bool touchCalibrationAvailableForMode(int mode) {
  if (mode < 0 || mode > 3)
    return false;
  return cfg.touch[mode].minX != 0 || cfg.touch[mode].maxX != 0;
}

void mapTouchFromConfig(const TS_Point &p, int &x, int &y) {
  int mode = cfg.screenMode;
  if (mode < 0 || mode > 3)
    mode = 3;
  if (!touchCalibrationAvailableForMode(mode)) {
    x = -1;
    y = -1;
    return;
  }
  TouchCal tc = cfg.touch[mode];
  x = map(p.x, tc.minX, tc.maxX, 0, physW - 1);
  y = map(p.y, tc.minY, tc.maxY, 0, physH - 1);
  x = constrain(x, 0, physW - 1);
  y = constrain(y, 0, physH - 1);
}

bool readTouchPointStable(int &rx, int &ry) {
  uint32_t start = millis();
  long sx = 0, sy = 0;
  int n = 0;
  while (millis() - start < 900) {
    if (touch.touched()) {
      TS_Point p = touch.getPoint();
      sx += p.x;
      sy += p.y;
      n++;
    }
    delay(15);
  }
  if (n < 5)
    return false;
  rx = sx / n;
  ry = sy / n;
  return true;
}

void drawCalibrationArrow(int corner, uint16_t color) {
  int base[7][2] = {
    {4, 4},
    {4, 34},
    {14, 24},
    {34, 44},
    {44, 34},
    {24, 14},
    {34, 4}
  };
  
  int pts[7][2];
  for (int i = 0; i < 7; i++) {
    int x = base[i][0];
    int y = base[i][1];
    
    if (corner == 1 || corner == 3) {
      x = physW - x;
    }
    if (corner == 2 || corner == 3) {
      y = physH - y;
    }
    
    pts[i][0] = x;
    pts[i][1] = y;
  }
  
  for (int dx = -2; dx <= 2; dx++) {
    for (int dy = -2; dy <= 2; dy++) {
      if (dx*dx + dy*dy <= 5) { // brush radius ~2.2 for thick lines
        for (int i = 0; i < 7; i++) {
          int next = (i + 1) % 7;
          tft.drawLine(pts[i][0] + dx, pts[i][1] + dy, pts[next][0] + dx, pts[next][1] + dy, color);
        }
      }
    }
  }
}

bool calibrateTouchCurrentRotation() {
  int mode = cfg.screenMode;
  if (mode < 0 || mode > 3)
    mode = 3;
  int margin = 20;
  int ptsX[4] = {margin, physW - margin, margin, physW - margin};
  int ptsY[4] = {margin, margin, physH - margin, physH - margin};
  long rawX[4], rawY[4];

  showMessage("CALIBRATION", "Touch calibration starting...", TFT_BLACK,
              TFT_WHITE);
  delay(800);

  for (int i = 0; i < 4; i++) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString("CALIBRATION TOUCH", physW / 2, 18, 2);
    tft.drawString((String(i + 1) + "/4").c_str(), physW / 2, 38, 2);
    drawCalibrationArrow(i, TFT_YELLOW);

    while (touch.touched())
      delay(10);

    int rx, ry;
    bool ok = false;
    unsigned long ws = millis();
    unsigned long lastPrint = 0;
    while (millis() - ws < 15000) {
      if (millis() - lastPrint > 1000) {
        Serial.printf("[Touch Debug] irq_pin_36=%d touched=%d\n",
                      digitalRead(36), touch.touched());
        lastPrint = millis();
      }
      if (touch.touched()) {
        delay(80);
        ok = readTouchPointStable(rx, ry);
        while (touch.touched())
          delay(10);
        break;
      }
      delay(10);
    }

    if (!ok) {
      showMessage("CALIBRATION", "Timeout point " + String(i + 1), TFT_RED,
                  TFT_WHITE);
      delay(1500);
      return false;
    }

    rawX[i] = rx;
    rawY[i] = ry;
    tft.fillCircle(ptsX[i], ptsY[i], 8, TFT_GREEN);
    delay(350);
  }

  long minX = (rawX[0] + rawX[2]) / 2;
  long maxX = (rawX[1] + rawX[3]) / 2;
  long minY = (rawY[0] + rawY[1]) / 2;
  long maxY = (rawY[2] + rawY[3]) / 2;

  cfg.touch[mode].minX = minX;
  cfg.touch[mode].maxX = maxX;
  cfg.touch[mode].minY = minY;
  cfg.touch[mode].maxY = maxY;

  DBG_PRINTLN("---- CALIBRATION RESULT ----");
  DBG_PRINTF("mode=%d\n", mode);
  DBG_PRINTF("rawX: %ld %ld %ld %ld\n", rawX[0], rawX[1], rawX[2], rawX[3]);
  DBG_PRINTF("rawY: %ld %ld %ld %ld\n", rawY[0], rawY[1], rawY[2], rawY[3]);
  DBG_PRINTF("minX=%ld maxX=%ld minY=%ld maxY=%ld\n", minX, maxX, minY, maxY);
  DBG_PRINTLN("------------------------------");
  return true;
}

bool bootHeldFor3Seconds() {
  pinMode(BOOT_BTN, INPUT_PULLUP);
  if (digitalRead(BOOT_BTN) != LOW)
    return false;
  unsigned long start = millis();
  showMessage("BOOT", "Press and hold to calibrate...", TFT_DARKGREY,
              TFT_WHITE);
  while (digitalRead(BOOT_BTN) == LOW) {
    if (millis() - start >= BOOT_HOLD_MS)
      return true;
    delay(20);
  }
  return false;
}

void showCountdownSave(int secondi) {
  tft.fillScreen(TFT_DARKGREEN);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.drawString("CALIBRATION OK", physW / 2, 25, 4);
  tft.drawString("Saving and rebooting...", physW / 2, 80, 2);
  tft.drawString("Waiting...", physW / 2, 110, 4);
}

void drawTouchLiveScreen() {
  tft.fillScreen(TFT_DARKGREEN);
  tft.drawRect(0, 0, physW, physH, TFT_WHITE);
  tft.drawFastHLine(0, physH / 2, physW, TFT_WHITE);
  tft.drawFastVLine(physW / 2, 0, physH, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
  tft.drawString("TOUCH LIVE TEST", physW / 2, 18, 4);
  tft.drawString("Waiting 5 seconds for reboot", physW / 2, 70, 2);
  tft.drawString("or press Reset and Button for recalibration", physW / 2, 90, 2);
}

void drawSummary(const char *stateMsg) {
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("CONFIG SD", 10, 10, 4);
  tft.drawString(stateMsg, 10, 40, 2);
  xSemaphoreTake(g_configMutex, portMAX_DELAY);
  tft.drawString("mode=" + String(cfg.screenMode), 10, 65, 2);
  tft.drawString("ssid=" + cfg.wifiSsid, 10, 90, 2);
  tft.drawString("city=" + cfg.weatherCity, 10, 115, 2);
  tft.drawString("mqtt=" + cfg.mqttServer + ":" + String(cfg.mqttPort), 10, 140, 2);
  tft.drawString("ora=" + String(cfg.formatHour) + "h", 10, 165, 2);
  xSemaphoreGive(g_configMutex);
  tft.drawRect(0, 0, physW - 1, physH - 1, TFT_GREEN);
}

#include "../include/aquarium_logic.h"
#include "../include/aquarium_ui.h"
#include "../include/language_manager.h"

AnimatedGIF gif;
int gifOffsetX = 0;
int gifOffsetY = 0;

void *GIFOpenFile(const char *fname, int32_t *pSize) {
  File* f = new File(SD.open(fname));
  if (*f) {
    *pSize = f->size();
    return (void *)f;
  }
  delete f;
  return NULL;
}

void GIFCloseFile(void *pHandle) {
  File *f = static_cast<File *>(pHandle);
  if (f != NULL) {
    f->close();
    delete f;
  }
}

int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  int32_t iBytesRead = iLen;
  File *f = static_cast<File *>(pFile->fHandle);
  if ((pFile->iSize - pFile->iPos) < iLen)
    iBytesRead = pFile->iSize - pFile->iPos;
  if (iBytesRead <= 0)
    return 0;
  iBytesRead = (int32_t)f->read(pBuf, iBytesRead);
  pFile->iPos = f->position();
  return iBytesRead;
}

int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  File *f = static_cast<File *>(pFile->fHandle);
  f->seek(iPosition);
  pFile->iPos = (int32_t)f->position();
  return pFile->iPos;
}

void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s;
  uint16_t *usPalette, usTemp[320];
  int x, y, iWidth;

  iWidth = pDraw->iWidth;
  if (iWidth + pDraw->iX + gifOffsetX > physW)
    iWidth = physW - pDraw->iX - gifOffsetX;

  usPalette = pDraw->pPalette;
  y = pDraw->iY + pDraw->y + gifOffsetY;

  if (y >= physH || pDraw->iX + gifOffsetX >= physW || iWidth < 1)
    return;

  s = pDraw->pPixels;
  if (pDraw->ucHasTransparency) {
    uint8_t c, ucTransparent = pDraw->ucTransparent;
    int x = 0;
    int iCount = 0;
    int xStart = 0;
    
    tft.setSwapBytes(true);
    while (x < iWidth) {
      c = *s++;
      if (c != ucTransparent) {
        if (iCount == 0) xStart = x;
        usTemp[iCount++] = usPalette[c];
      } else {
        if (iCount > 0) {
          tft.pushImage(pDraw->iX + gifOffsetX + xStart, y, iCount, 1, usTemp);
          iCount = 0;
        }
      }
      x++;
    }
    if (iCount > 0) {
      tft.pushImage(pDraw->iX + gifOffsetX + xStart, y, iCount, 1, usTemp);
    }
    tft.setSwapBytes(false);
  } else {
    for (x = 0; x < iWidth; x++) usTemp[x] = usPalette[*s++];
    tft.setSwapBytes(true);
    tft.pushImage(pDraw->iX + gifOffsetX, y, iWidth, 1, usTemp);
    tft.setSwapBytes(false);
  }
}

// Funzione di utilità per pilotare il LED RGB di stato
// I pin del CYD per il LED RGB sono solitamente Attivi Bassi (LOW = Acceso)
void setSystemLedState(bool r, bool g, bool b) {
  digitalWrite(LED_RED_PIN, r ? LOW : HIGH);
  digitalWrite(LED_GREEN_PIN, g ? LOW : HIGH);
  digitalWrite(LED_BLUE_PIN, b ? LOW : HIGH);
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  delay(1000);
  Serial.println("\n\n=== BOOTING ===");
  g_configMutex = xSemaphoreCreateMutex();
  
  // Inizializza LED RGB
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  // Accende il LED Blu (Stato: Caricamento all'avvio)
  setSystemLedState(false, false, true);
  
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  backlightOn();
  tft.begin();
  setDefaults();
  applyScreenMode();
  if (!initSD()) {
    setSystemLedState(true, false, false); // Rosso = Errore bloccante
    showMessage("SD ERROR", "Error SD memory!", TFT_RED, TFT_WHITE);
    while (true)
      delay(1000);
  }

  langManager.init();
  bool created = false, updated = false;
  loadConfigFromSD(created, updated);
  applyScreenMode();

  gif.begin(LITTLE_ENDIAN_PIXELS);
  if (gif.open("/boot.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
    tft.fillScreen(TFT_BLACK);
    GIFINFO gi;
    gif.getInfo(&gi);
    gifOffsetX = (physW - gif.getCanvasWidth()) / 2;
    gifOffsetY = (physH - gif.getCanvasHeight()) / 2;
    
    uint32_t start_time = millis();
    pinMode(BOOT_BTN, INPUT_PULLUP);
    while (millis() - start_time < 5000) {
      // Se l'utente preme il tasto Boot durante l'animazione, la salta!
      if (digitalRead(BOOT_BTN) == LOW) {
        break;
      }
      if (!gif.playFrame(true, NULL)) {
        gif.reset();
      }
    }
    gif.close();
  } else {
    showMessage("AQUARIUM OS", "Starting system...", TFT_BLACK, COLOR_CYAN_GLOW);
    delay(1500); // Allow time to read if no gif
  }

  touchSPI = new SPIClass(HSPI);
  touchSPI->begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  touch.begin(*touchSPI);

  bool forceCalibration = !touchCalibrationAvailableForMode(cfg.screenMode);
  if (forceCalibration || bootHeldFor3Seconds()) {
    if (calibrateTouchCurrentRotation()) {
      writeWholeConfigFileSafe();
      drawTouchLiveScreen();
      
      unsigned long lastTouchTime = millis();
      while (millis() - lastTouchTime < 5000) {
        if (touch.touched()) {
          lastTouchTime = millis();
          TS_Point p = touch.getPoint();
          int tx = -1, ty = -1;
          mapTouchFromConfig(p, tx, ty);
          if (tx >= 0 && ty >= 0) {
            tft.fillCircle(tx, ty, 3, TFT_YELLOW);
          }
        }
        delay(10);
      }
      
      ESP.restart();
    }
  }

  // Initialize Aquarium Hardware & UI Engine
  aquarium.init();
  aquariumUI.init(&tft);

  // Auto connect WiFi on boot if saved credentials exist
  if (cfg.wifiSsid.length() > 0 && cfg.wifiSsid != "SSID_WIFI") {
    aquarium.connectWifiSSID(cfg.wifiSsid, cfg.wifiPassword);
  }
  
  // Setup completato, sistema pronto: Accende il LED Verde
  setSystemLedState(false, true, false);
}

void loop() {
  // --- Screensaver Wake Logic ---
  if (touch.touched()) {
    if (g_screenIsOff) {
      // Wake the screen: turn backlight on and consume this touch event
      backlightOn();
      g_screenIsOff = false;
      g_lastActivityMs = millis();
      // Wait for finger release so we don't trigger the button underneath
      while (touch.touched()) {
        delay(10);
      }
      return;
    }

    // Screen is on: normal touch handling
    g_lastActivityMs = millis();
    TS_Point p = touch.getPoint();
    int x = -1, y = -1;
    mapTouchFromConfig(p, x, y);
    if (x >= 0 && y >= 0) {
      aquariumUI.handleTouch(x, y);
    }
  }

  // --- Screensaver Inactivity Check ---
  uint16_t ssTime = aquarium.getConfig().screensaverTime;
  if (!g_screenIsOff && ssTime > 0) {
    if ((millis() - g_lastActivityMs) >= (unsigned long)ssTime * 1000UL) {
      backlightOff();
      g_screenIsOff = true;
    }
  }

  if (!g_screenIsOff) {
    aquariumUI.update();
  }
  aquarium.update();

  aquariumServer.update();
  
  if (g_saveConfigNeeded) {
    writeWholeConfigFileSafe();
    g_saveConfigNeeded = false;
  }
  
  delay(5);
}