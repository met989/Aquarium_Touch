#include "../include/aquarium_ui.h"
#include "../include/language_manager.h"
#include "ff.h"

#define GFX ((m_activeTab == TAB_SETTINGS) ? (TFT_eSPI *)m_tft : (TFT_eSPI *)m_sprite)
extern AppConfig cfg;
extern bool writeWholeConfigFileSafe();

#ifndef FM_ANY
#define FM_ANY 0
#endif
#ifndef FF_MAX_SS
#define FF_MAX_SS 512
#endif
typedef unsigned char BYTE;

void AquariumUI::drawSettingsTab() {
  drawSettingsSubScreen(m_settingsSubScreen);
}

void AquariumUI::drawSettingsMenu() {
  char titleBuf[32];
  snprintf(titleBuf, sizeof(titleBuf), "%s (Pagina %d/2)",
           langManager.getText("TITLE_SETTINGS", "SETTINGS"),
           m_settingsMenuPage + 1);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(titleBuf, 160, 0, 2);

  if (m_settingsMenuPage == 1) {
    const char *items_page2[] = {
        langManager.getText("MENU_SCREEN", "9. Screen")};
    int btnWidth = 145;
    int btnHeight = 40;
    int startY = 24;
    int gapY = 42;

    for (int i = 0; i < 1; i++) {
      int col = i % 2;
      int row = i / 2;
      int x = 10 + col * 155;
      int y = startY + row * gapY;

      GFX->fillRoundRect(x, y, btnWidth, btnHeight, 8, COLOR_CARD_BORDER);
      GFX->drawRoundRect(x, y, btnWidth, btnHeight, 8, COLOR_CYAN_GLOW);
      GFX->setTextColor(TFT_WHITE);
      GFX->setTextDatum(MC_DATUM);
      GFX->drawString(items_page2[i], x + btnWidth / 2, y + btnHeight / 2, 2);
    }
    return;
  }

  const char *items[] = {
      langManager.getText("MENU_DATETIME", "1. Date & Time"),
      langManager.getText("MENU_TEMP_TARGET", "2. Target Temp"),
      langManager.getText("MENU_RELAY_STATE", "3. Relay State"),
      langManager.getText("MENU_WIFI_MGMT", "4. Wi-Fi"),
      langManager.getText("MENU_SYS_VERSION", "5. System Info"),
      langManager.getText("MENU_LANGUAGE", "6. Language"),
      langManager.getText("MENU_ENERGY_SAVING", "7. Energy Saving"),
      langManager.getText("MENU_FACTORY_RESET", "8. Factory Reset")};

  int btnWidth = 145;
  int btnHeight = 40;
  int startY = 24;
  int gapY = 42;

  for (int i = 0; i < 8; i++) {
    int col = i % 2;
    int row = i / 2;
    int x = 10 + col * 155;
    int y = startY + row * gapY;

    GFX->fillRoundRect(x, y, btnWidth, btnHeight, 8, COLOR_CARD_BORDER);
    GFX->drawRoundRect(x, y, btnWidth, btnHeight, 8, COLOR_CYAN_GLOW);
    GFX->setTextColor(TFT_WHITE);
    GFX->setTextDatum(MC_DATUM);
    GFX->drawString(items[i], x + btnWidth / 2, y + btnHeight / 2, 2);
  }
}

void AquariumUI::drawSettingsSubScreen(int sub) {
  if (sub == 0) {
    drawSettingsMenu();
  } else if (sub == 1) { // Date & Time
    // Consistent Enlarged Back Button
    drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"),
                    COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

    // Title (Shifted down)
    GFX->setTextColor(COLOR_CYAN_GLOW);
    GFX->setTextDatum(TC_DATUM);
    String titleStr =
        String(langManager.getText("TITLE_DATETIME", "DATE & TIME")) +
        " [ UTC: " + cfg.timezone + " ]";
    GFX->drawString(titleStr.c_str(), 190, 10, 2);

    // Date Time Card (Huge - Shifted down)
    int h, m, s;
    aquarium.getTime(h, m, s);
    char clockStr[32], dateStr[16];
    snprintf(clockStr, sizeof(clockStr), "%02d:%02d:%02d", h, m, s);
    aquarium.getFormattedDate(dateStr, sizeof(dateStr));
    String combined = String(dateStr) + " - " + String(clockStr);

    GFX->fillRoundRect(10, 48, 300, 40, 8, COLOR_CYAN_GLOW);
    GFX->setTextColor(COLOR_BG_OCEAN);
    GFX->drawString(combined, 160, 57, 4);

    // Date Format Toggle Button (Larger: Y: 104, Height: 44)
    GFX->fillRoundRect(10, 104, 300, 44, 8, COLOR_CARD_BORDER);
    GFX->drawRoundRect(10, 104, 300, 44, 8, COLOR_CYAN_GLOW);
    GFX->setTextColor(TFT_WHITE);

    String fmtLabel =
        String(langManager.getText("LABEL_DATE_FMT", "DATE FORMAT:")) + " ";
    const AquariumConfig &acfg = aquarium.getConfig();
    if (acfg.dateFormat == DATE_FORMAT_DDMMYYYY)
      fmtLabel += "GG/MM/AAAA";
    else if (acfg.dateFormat == DATE_FORMAT_MMDDYYYY)
      fmtLabel += "MM/GG/AAAA";
    else
      fmtLabel += "AAAA/MM/GG";
    GFX->drawString(fmtLabel, 160, 118, 2);

    // NTP Sync Button (Larger: Y: 164, Height: 52)
    GFX->fillRoundRect(10, 164, 145, 52, 8, COLOR_CARD_BORDER);
    GFX->drawRoundRect(10, 164, 145, 52, 8, COLOR_CYAN_GLOW);
    GFX->drawString("SYNC NTP", 82, 182, 2);

    // Timezone Buttons (Larger: Y: 164, Height: 52)
    GFX->fillRoundRect(165, 164, 65, 52, 8, COLOR_CARD_BORDER);
    GFX->drawRoundRect(165, 164, 65, 52, 8, COLOR_CYAN_GLOW);
    GFX->drawString("-1H", 199, 182, 2);

    GFX->fillRoundRect(240, 164, 70, 52, 8, COLOR_CARD_BORDER);
    GFX->drawRoundRect(240, 164, 70, 52, 8, COLOR_CYAN_GLOW);
    GFX->drawString("+1H", 277, 182, 2);
  } else {
    // Altre subscreen
    if (sub == 2)
      drawSubScreenTempTarget();
    if (sub == 3)
      drawSubScreenRelay();
    if (sub == 4)
      drawSubScreenWifi();
    if (sub == 5)
      drawSubScreenVersion();
    if (sub == 6)
      drawSubScreenLanguage();
    if (sub == 7)
      drawSubScreenEnergySaving();
    if (sub == 8)
      drawSubScreenFactoryReset();
    if (sub == 9)
      drawSubScreenScreen();
  }
}

void AquariumUI::drawSubScreenTempTarget() {
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_EMERALD_GREEN, COLOR_BG_OCEAN);
  GFX->drawString(
      langManager.getText("TITLE_TEMP_TARGET", "OPTIMAL MIN & MAX SETTING"),
      190, 10, 2);

  const AquariumConfig &cfg = aquarium.getConfig();

  // Card 1: MINIMA OTTIMALE (X: 10, Y: 40, W: 300, H: 80)
  drawGlassCard(10, 40, 300, 80, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_MIN_TARGET", "OPTIMAL MIN:"), 18,
                  50, 2);

  char minStr[16];
  snprintf(minStr, sizeof(minStr), "%.1f C", cfg.targetTempMin);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
  GFX->drawString(minStr, 18, 71, 4);

  // Enlarged Buttons (Height: 52, Y: 54)
  drawTouchButton(130, 54, 40, 52, "-1.0", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(174, 54, 40, 52, "-0.1", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(218, 54, 40, 52, "+0.1", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(262, 54, 40, 52, "+1.0", COLOR_CARD_BORDER, TFT_WHITE);

  // Card 2: MASSIMA OTTIMALE (X: 10, Y: 132, W: 300, H: 80)
  drawGlassCard(10, 132, 300, 80, COLOR_CORAL_RED);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_MAX_TARGET", "OPTIMAL MAX:"), 18,
                  142, 2);

  char maxStr[16];
  snprintf(maxStr, sizeof(maxStr), "%.1f C", cfg.targetTempMax);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_CORAL_RED, COLOR_CARD_BG);
  GFX->drawString(maxStr, 18, 163, 4);

  // Enlarged Buttons (Height: 52, Y: 146)
  drawTouchButton(130, 146, 40, 52, "-1.0", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(174, 146, 40, 52, "-0.1", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(218, 146, 40, 52, "+0.1", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(262, 146, 40, 52, "+1.0", COLOR_CARD_BORDER, TFT_WHITE);
}

void AquariumUI::drawSubScreenRelay() {
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_BG_OCEAN);
  GFX->drawString(
      langManager.getText("TITLE_RELAY_STATE", "LIGHT RELAY STATUS (GPIO 17)"),
      190, 10, 2);

  bool lightOn = aquarium.isLightOn();
  bool inv = aquarium.isRelayInverted();
  uint16_t releCol = lightOn ? COLOR_GOLD_ACCENT : COLOR_TEXT_MUTED;

  // Relay Info Card (X: 10, Y: 40, W: 300, H: 70)
  drawGlassCard(10, 40, 300, 70, releCol);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(
      langManager.getText("LABEL_PIN_POLARITY", "PIN: GPIO 17 | POLARITY:"), 18,
      46, 1);
  GFX->setTextColor(inv ? COLOR_CORAL_RED : COLOR_CYAN_GLOW, COLOR_CARD_BG);
  GFX->drawString(
      inv ? langManager.getText("LABEL_ACTIVE_LOW", "ACTIVE LOW (INVERTED)")
          : langManager.getText("LABEL_ACTIVE_HIGH", "ACTIVE HIGH (NORMAL)"),
      165, 46, 1);

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(releCol, COLOR_CARD_BG);
  GFX->drawString(lightOn ? langManager.getText("LABEL_RELAY_ACTIVE",
                                                "RELAY STATUS: ACTIVE")
                          : langManager.getText("LABEL_RELAY_INACTIVE",
                                                "RELAY STATUS: INACTIVE"),
                  160, 83, 2);

  // Button 1: Commuta Relè Adesso (X: 10, Y: 122, W: 300, H: 44)
  drawTouchButton(10, 122, 300, 44, lightOn ? langManager.getText("BTN_DEACTIVATE_RELAY", "DEACTIVATE RELAY NOW") : langManager.getText("BTN_ACTIVATE_RELAY", "ACTIVATE RELAY NOW"), COLOR_CARD_BG, releCol, releCol);

  // Button 2: Inverti Logica High/Low (X: 10, Y: 178, W: 300, H: 44)
  drawTouchButton(
      10, 178, 300, 44,
      inv ? langManager.getText("BTN_RESTORE_HIGH", "RESTORE ACTIVE HIGH LOGIC")
          : langManager.getText("BTN_INVERT_LOW", "INVERT LOGIC (ACTIVE LOW)"),
      COLOR_CARD_BORDER, COLOR_CYAN_GLOW);
}

void AquariumUI::drawSubScreenWifi() {
  if (m_wifiShowKeyboard) {
    drawWifiKeyboard();
    return;
  }

  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_EMERALD_GREEN, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("MENU_WIFI_MGMT", "WI-FI (STATUS/NET)"),
                  190, 10, 2);

  bool conn = aquarium.isWifiConnected();
  uint16_t statusCol = conn ? COLOR_EMERALD_GREEN : COLOR_CORAL_RED;

  // WiFi Status Card (Shifted down Y: 38, Height: 44)
  drawGlassCard(6, 38, 308, 44, statusCol);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_STATUS", "STATUS:"), 14, 42, 2);
  GFX->setTextColor(statusCol, COLOR_CARD_BG);
  GFX->drawString(conn
                      ? langManager.getText("MSG_CONNECTED", "CONNECTED")
                      : langManager.getText("MSG_DISCONNECTED", "DISCONNECTED"),
                  66, 42, 2);

  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString("IP:", 160, 42, 2);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString(aquarium.getWifiIP().c_str(), 180, 42, 2);

  // Scan Button [ SCANSIONE RETI WI-FI ] (Shifted down Y: 90, Height: 38)
  drawTouchButton(6, 90, 308, 38,
                  langManager.getText("BTN_SCAN", "SCAN WI-FI NETWORKS"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

  WifiConnectState connState = aquarium.getWifiConnectState();
  if (connState != WIFI_CONN_IDLE) {
    drawGlassCard(6, 136, 308, 94, COLOR_CYAN_GLOW);
    GFX->setTextDatum(MC_DATUM);

    if (connState == WIFI_CONN_CONNECTING) {
      GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
      GFX->drawString(
          langManager.getText("MSG_CONNECTING_NOW", "Connessione in corso..."),
          160, 176, 2);
      GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
      GFX->drawString(aquarium.getWifiConnectSSID(), 160, 204, 2);
    } else if (connState == WIFI_CONN_SUCCESS) {
      GFX->setTextColor(TFT_GREEN, COLOR_CARD_BG);
      String msg =
          String(langManager.getText("MSG_CONNECTED_TO", "CONNESSO AL WIFI")) +
          " " + aquarium.getWifiConnectSSID();
      GFX->drawString(msg, 160, 183, 2);
    } else {
      GFX->setTextColor(TFT_RED, COLOR_CARD_BG);
      GFX->drawString(
          langManager.getText("MSG_CONNECT_FAILED", "ERRORE DI CONNESSIONE"),
          160, 176, 2);
      GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
      GFX->drawString(aquarium.getWifiConnectSSID(), 160, 204, 2);
    }
    return;
  }

  // Scanned Networks List (Shifted down Y: 136..234)
  if (aquarium.isWifiScanning()) {
    drawGlassCard(6, 136, 308, 94, COLOR_CYAN_GLOW);
    GFX->setTextDatum(MC_DATUM);
    GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
    GFX->drawString(
        langManager.getText("MSG_SCANNING", "PLEASE WAIT... Scanning networks"),
        160, 176, 2);
    GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_SEARCHING_SIGNALS",
                                        "Searching Wi-Fi signals..."),
                    160, 204, 1);
    return;
  }

  int netCount = aquarium.getWifiNetworkCount();
  if (netCount <= 0) {
    drawGlassCard(6, 136, 308, 94, COLOR_CARD_BORDER);
    GFX->setTextDatum(MC_DATUM);
    GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    GFX->drawString(aquarium.getWifiScanStatus().c_str(), 160, 176, 2);
    GFX->drawString(langManager.getText("MSG_PRESS_SCAN",
                                        "Press [SCAN WI-FI NETWORKS] above"),
                    160, 204, 1);
    return;
  }

  int totalPages = (netCount + 2) / 3;
  if (m_wifiListPage < 0)
    m_wifiListPage = 0;
  if (m_wifiListPage >= totalPages)
    m_wifiListPage = totalPages - 1;

  int cardWidth = (totalPages > 1) ? 254 : 308;

  // Display Up to 3 Network Cards (Shifted down Y: 136, 172, 208, Height: 32)
  int startIdx = m_wifiListPage * 3;
  for (int i = 0; i < 3; i++) {
    int idx = startIdx + i;
    int cardY = 136 + i * 36;

    if (idx < netCount) {
      WifiNetworkItem net = aquarium.getWifiNetwork(idx);
      uint16_t borderCol = net.open ? COLOR_EMERALD_GREEN : COLOR_CYAN_GLOW;
      drawGlassCard(6, cardY, cardWidth, 32, borderCol);

      GFX->setTextDatum(TL_DATUM);
      GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
      String ssidDisplay = String(idx + 1) + ". " + net.ssid;
      if (ssidDisplay.length() > 24)
        ssidDisplay = ssidDisplay.substring(0, 24) + "..";
      GFX->drawString(ssidDisplay.c_str(), 14, cardY + 8, 2);

      GFX->setTextDatum(TR_DATUM);
      GFX->setTextColor(net.open ? COLOR_EMERALD_GREEN : COLOR_CYAN_GLOW,
                        COLOR_CARD_BG);

      String secInfo = String(net.rssi) + "dB";
      int textW = GFX->textWidth(secInfo, 2);
      GFX->drawString(secInfo.c_str(), cardWidth - 6, cardY + 8, 2);

      if (!net.open) {
        // Disegna un piccolo lucchetto dorato alla sinistra del testo dB
        int px = cardWidth - 6 - textW - 14;
        int py = cardY + 8;
        GFX->drawRoundRect(px + 2, py, 6, 6, 2, COLOR_GOLD_ACCENT);  // gancio
        GFX->fillRoundRect(px, py + 4, 10, 8, 1, COLOR_GOLD_ACCENT); // corpo
        GFX->drawLine(px + 5, py + 6, px + 5, py + 9, TFT_BLACK); // buco chiave
      } else {
        // Rete aperta
        GFX->drawString(langManager.getText("MSG_OPEN_NET", "OPEN"),
                        cardWidth - 6 - textW - 4, cardY + 8, 2);
      }
    }
  }

  // Right Side Pagination Buttons (^ and v) (Shifted down Y: 136, 184, Height:
  // 44)
  if (totalPages > 1) {
    uint16_t upCol = (m_wifiListPage > 0) ? COLOR_CYAN_GLOW : COLOR_CARD_BORDER;
    uint16_t upTextCol =
        (m_wifiListPage > 0) ? COLOR_GOLD_ACCENT : COLOR_TEXT_MUTED;
    drawTouchButton(266, 136, 48, 44, "^", COLOR_CARD_BG, upTextCol, upCol);

    uint16_t downCol =
        (m_wifiListPage < totalPages - 1) ? COLOR_CYAN_GLOW : COLOR_CARD_BORDER;
    uint16_t downTextCol = (m_wifiListPage < totalPages - 1) ? COLOR_GOLD_ACCENT
                                                             : COLOR_TEXT_MUTED;
    drawTouchButton(266, 184, 48, 44, "v", COLOR_CARD_BG, downTextCol, downCol);
  }
}

void AquariumUI::drawSubScreenLanguage() {
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(
      langManager.getText("MENU_LANGUAGE", "SELECT INTERFACE LANGUAGE"), 190,
      10, 2);

  int langCount = langManager.getAvailableLanguageCount();
  String currentFile = langManager.getActiveLanguageFile();

  // Enlarged Language Cards (Height: 38, Spacing: 44, Y starts at 40)
  for (int i = 0; i < 4; i++) {
    int cardY = 40 + i * 44;
    if (i < langCount) {
      LangItem item = langManager.getAvailableLanguage(i);
      bool isSelected =
          (currentFile == item.filename || currentFile.endsWith(item.filename));
      uint16_t borderCol = isSelected ? COLOR_GOLD_ACCENT : COLOR_CYAN_GLOW;
      drawGlassCard(6, cardY, 308, 38, borderCol);

      GFX->setTextDatum(TL_DATUM);
      GFX->setTextColor(isSelected ? COLOR_GOLD_ACCENT : TFT_WHITE,
                        COLOR_CARD_BG);
      String label = String(i + 1) + ". " + item.name;
      GFX->drawString(label.c_str(), 16, cardY + 11, 2);

      if (isSelected) {
        GFX->setTextDatum(TR_DATUM);
        GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
        GFX->drawString("<-", 300, cardY + 11, 2);
      }
    }
  }
}

void AquariumUI::drawSubScreenVersion() {
  // Consistent Enlarged Back Button
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(
      langManager.getText("TITLE_SYS_VERSION", "SYSTEM INFORMATION"), 190, 10,
      2);

  if (m_isUpdating) {
    drawGlassCard(10, 40, 300, 172, COLOR_CARD_BORDER);
    GFX->setTextDatum(MC_DATUM);
    GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
    GFX->drawString("Aggiornamento in corso...", 160, 100, 2);
    GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    GFX->drawString("Non spegnere il dispositivo", 160, 140, 2);
    return;
  }

  if (m_isCheckingUpdate) {
    drawGlassCard(10, 40, 300, 172, COLOR_CARD_BORDER);
    GFX->setTextDatum(MC_DATUM);
    GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
    GFX->drawString("Controllo in corso...", 160, 100, 2);
    return;
  }

  if (m_showUpdatePrompt) {
    drawGlassCard(10, 40, 300, 172, COLOR_CARD_BORDER);
    GFX->setTextDatum(MC_DATUM);
    GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
    GFX->drawString("Vuoi aggiornare alla versione:", 160, 70, 2);
    GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
    GFX->drawString(m_latestVersion, 160, 100, 4);

    drawTouchButton(30, 140, 110, 40, "SI", COLOR_EMERALD_GREEN, TFT_WHITE,
                    COLOR_EMERALD_GREEN);
    drawTouchButton(180, 140, 110, 40, "NO", COLOR_CORAL_RED, TFT_WHITE,
                    COLOR_CORAL_RED);
    return;
  }

  // Enlarged Version Info Card (X: 10, Y: 40, W: 300, H: 130)
  drawGlassCard(10, 40, 300, 130, COLOR_CARD_BORDER);

  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_SYS_NAME", "FIRMWARE:"), 20, 50,
                  2);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString("AQUARIUM OS", 105, 50, 2);

  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_OS_VER", "VERSION:"), 20, 80, 2);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
  GFX->drawString(AQUARIUM_OS_VERSION, 105, 80, 4);

  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_FRAMEWORK", "FRAMEWORK:"), 20, 115,
                  2);
  GFX->setTextColor(COLOR_EMERALD_GREEN, COLOR_CARD_BG);
  GFX->drawString("ESP32 Arduino", 105, 115, 2);

  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_HARDWARE", "HARDWARE:"), 20, 140,
                  2);
  GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
  GFX->drawString("ESP32-DEV 240MHz", 105, 140, 2);

  bool hasUpdate =
      (m_latestVersion != "" && m_latestVersion != String(AQUARIUM_OS_VERSION));
  if (hasUpdate) {
    drawTouchButton(10, 176, 300, 36, "AGGIORNA VERSIONE", COLOR_CARD_BG,
                    TFT_WHITE, COLOR_CYAN_GLOW);
  } else {
    drawTouchButton(10, 176, 300, 36, "VERIFICA AGGIORNAMENTI", COLOR_CARD_BG,
                    TFT_WHITE, COLOR_GOLD_ACCENT);
  }
}

void AquariumUI::drawSubScreenEnergySaving() {
  // --- Screensaver time table (seconds): 0=MAI, 30, 60, 120, 180, 240, 300,
  // 360...3600 ---
  static const uint16_t SS_TIMES[] = {0,   30,   60,   120,  180, 240,
                                      300, 360,  420,  480,  540, 600,
                                      900, 1200, 1800, 2700, 3600};
  static const int SS_COUNT = sizeof(SS_TIMES) / sizeof(SS_TIMES[0]);

  // Sync idx with current config value
  uint16_t curVal = aquarium.getConfig().screensaverTime;
  for (int i = 0; i < SS_COUNT; i++) {
    if (SS_TIMES[i] == curVal) {
      m_screensaverIdx = i;
      break;
    }
  }

  // --- Header ---
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("TITLE_ENERGY_SAVING", "ENERGY SAVING"),
                  195, 10, 2);

  // --- Glass Card (Enlarged: Y: 40, Height 120) ---
  drawGlassCard(10, 40, 300, 120, COLOR_CYAN_GLOW);

  // Label
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_SCREENSAVER", "SCREEN OFF AFTER:"),
                  22, 52, 2);

  // Current value label
  char valBuf[20];
  if (SS_TIMES[m_screensaverIdx] == 0) {
    snprintf(valBuf, sizeof(valBuf), "%s",
             langManager.getText("LABEL_NEVER", "NEVER"));
  } else if (SS_TIMES[m_screensaverIdx] < 60) {
    snprintf(valBuf, sizeof(valBuf), "%d%s", SS_TIMES[m_screensaverIdx],
             langManager.getText("LABEL_SEC_SUFFIX", " sec"));
  } else {
    snprintf(valBuf, sizeof(valBuf), "%d%s", SS_TIMES[m_screensaverIdx] / 60,
             langManager.getText("LABEL_MIN_SUFFIX", " min"));
  }

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString(valBuf, 160, 92, 4);

  // --- Prev / Next buttons (Enlarged: Height: 44, Width: 70, Y: 110) ---
  drawTouchButton(16, 110, 70, 44, "< ", COLOR_CARD_BG, COLOR_GOLD_ACCENT,
                  COLOR_CARD_BORDER);
  drawTouchButton(234, 110, 70, 44, " >", COLOR_CARD_BG, COLOR_GOLD_ACCENT,
                  COLOR_CARD_BORDER);

  // --- Info text (Spaced out) ---
  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("LABEL_SCREENSAVER_LINE1",
                                      "LCD backlight off = energy saving"),
                  160, 180, 1);
  GFX->drawString(
      langManager.getText("LABEL_SCREENSAVER_LINE2", "Touch screen to wake"),
      160, 200, 1);
}

void AquariumUI::drawSubScreenFactoryReset() {
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("TITLE_FACTORY_RESET", "FACTORY_RESET"),
                  190, 10, 2);

  // Card Background (Enlarged: X: 10, Y: 40, W: 300, H: 150)
  drawGlassCard(10, 40, 300, 150, COLOR_CORAL_RED);

  GFX->setTextDatum(MC_DATUM);

  if (m_resetStep == 0) {
    GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
    GFX->drawString(
        langManager.getText("MSG_RESET_CONFIRM1",
                            "Do you want to restore factory settings?"),
        160, 76, 2);
  } else {
    GFX->setTextColor(COLOR_CORAL_RED, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_RESET_CONFIRM2",
                                        "ALL SETTINGS WILL BE DELETED!"),
                    160, 76, 2);
  }

  // Draw Yes/No buttons (Enlarged: Height: 50, Y: 128)
  drawTouchButton(30, 128, 110, 50, langManager.getText("BTN_YES", "YES"),
                  COLOR_CARD_BG, COLOR_EMERALD_GREEN, COLOR_EMERALD_GREEN);
  drawTouchButton(180, 128, 110, 50, langManager.getText("BTN_NO", "NO"),
                  COLOR_CARD_BG, COLOR_CORAL_RED, COLOR_CORAL_RED);
}

void AquariumUI::handleSettingsTouch(int touchX, int touchY) {
  int sy = touchY;
  const AquariumConfig &cfg = aquarium.getConfig();
      // Se siamo in un Sottomenu, gestisci il tasto Back [ < MENU ] (Top Left touch X: 0..110, sy: 0..45)
      if (m_settingsSubScreen > 0) {
        if (touchX <= 110 && sy <= 45) {
          if (m_settingsSubScreen == 4 && m_wifiShowKeyboard) {
            m_wifiShowKeyboard = false;
            m_settingsNeedsRedraw = true;
            return;
          }
          m_settingsSubScreen = 0;
          m_settingsNeedsRedraw = true;
          return;
        }
      }

      if (m_settingsSubScreen == 0 && m_settingsMenuPage == 0) {
        // Tocco Menu Principale (2 colonne x 4 righe)
        int btnWidth = 145;
        int btnHeight = 40;
        int startY = 24;
        int gapY = 42;
        
        for (int i = 0; i < 8; i++) {
            int col = i % 2;
            int row = i / 2;
            int x = 10 + col * 155;
            int y = startY + row * gapY;
            if (touchX >= x && touchX <= x + btnWidth && sy >= y && sy <= y + btnHeight) {
                m_settingsSubScreen = i + 1;
                m_resetStep = 0;
                m_settingsNeedsRedraw = true;
                return;
            }
        }
      } else if (m_settingsSubScreen == 0 && m_settingsMenuPage == 1) {
          int btnWidth = 145;
          int btnHeight = 40;
          int startY = 24;
          int gapY = 42;
          
          for (int i = 0; i < 2; i++) {
              int col = i % 2;
              int row = i / 2;
              int x = 10 + col * 155;
              int y = startY + row * gapY;
              if (touchX >= x && touchX <= x + btnWidth && sy >= y && sy <= y + btnHeight) {
                  m_settingsSubScreen = 9 + i; // i=0 -> 9, i=1 -> 10
                  m_settingsNeedsRedraw = true;
                  return;
              }
          }
      } else if (m_settingsSubScreen == 1) {
        // Sub 1: Data & Ora (Updated for enlarged Y: 104..148 and 164..216)
        // Date Format Toggle Button (X: 10..310, sy: 104..148)
        if (touchX >= 10 && touchX <= 310 && sy >= 104 && sy <= 148) {
          aquarium.setDateFormat((cfg.dateFormat + 1) % 3);
          m_settingsNeedsRedraw = true;
        }
        // NTP Sync Button (X: 10..155, sy: 164..216)
        else if (touchX >= 10 && touchX <= 155 && sy >= 164 && sy <= 216) {
          aquarium.syncNTP();
          m_settingsNeedsRedraw = true;
        }
        // Timezone -1H (X: 165..230, sy: 164..216)
        else if (touchX >= 165 && touchX <= 230 && sy >= 164 && sy <= 216) {
          int tz = ::cfg.timezone.toInt() - 1;
          ::cfg.timezone = (tz > 0 ? "+" : "") + String(tz);
          extern bool writeWholeConfigFileSafe();
          writeWholeConfigFileSafe();
          aquarium.syncNTP();
          m_settingsNeedsRedraw = true;
        }
        // Timezone +1H (X: 240..310, sy: 164..216)
        else if (touchX >= 240 && touchX <= 310 && sy >= 164 && sy <= 216) {
          int tz = ::cfg.timezone.toInt() + 1;
          ::cfg.timezone = (tz > 0 ? "+" : "") + String(tz);
          extern bool writeWholeConfigFileSafe();
          writeWholeConfigFileSafe();
          aquarium.syncNTP();
          m_settingsNeedsRedraw = true;
        }
      } else if (m_settingsSubScreen == 2) {
        // Sub 2: Soglie Temperatura Target (Minima & Massima Ottimale - Enlarged layout)
        // Minima Ottimale Buttons (sy: 54..106)
        if (sy >= 54 && sy <= 106) {
          if (touchX >= 130 && touchX <= 170) aquarium.setTargetTemp(cfg.targetTempMin - 1.0f, cfg.targetTempMax);
          if (touchX >= 174 && touchX <= 214) aquarium.setTargetTemp(cfg.targetTempMin - 0.1f, cfg.targetTempMax);
          if (touchX >= 218 && touchX <= 258) aquarium.setTargetTemp(cfg.targetTempMin + 0.1f, cfg.targetTempMax);
          if (touchX >= 262 && touchX <= 302) aquarium.setTargetTemp(cfg.targetTempMin + 1.0f, cfg.targetTempMax);
          m_settingsNeedsRedraw = true;
        }
        // Massima Ottimale Buttons (sy: 146..198)
        else if (sy >= 146 && sy <= 198) {
          if (touchX >= 130 && touchX <= 170) aquarium.setTargetTemp(cfg.targetTempMin, cfg.targetTempMax - 1.0f);
          if (touchX >= 174 && touchX <= 214) aquarium.setTargetTemp(cfg.targetTempMin, cfg.targetTempMax - 0.1f);
          if (touchX >= 218 && touchX <= 258) aquarium.setTargetTemp(cfg.targetTempMin, cfg.targetTempMax + 0.1f);
          if (touchX >= 262 && touchX <= 302) aquarium.setTargetTemp(cfg.targetTempMin, cfg.targetTempMax + 1.0f);
          m_settingsNeedsRedraw = true;
        }
      } else if (m_settingsSubScreen == 3) {
        // Sub 3: Stato Relè (Enlarged layout)
        // Button 1: Commuta Relè Adesso (X: 10..310, sy: 122..166)
        if (touchX >= 10 && touchX <= 310 && sy >= 122 && sy <= 166) {
          aquarium.toggleLight();
          m_settingsNeedsRedraw = true;
        }
        // Button 2: Inverti Logica High/Low (X: 10..310, sy: 178..222)
        if (touchX >= 10 && touchX <= 310 && sy >= 178 && sy <= 222) {
          aquarium.toggleRelayInverted();
          m_settingsNeedsRedraw = true;
        }
      } else if (m_settingsSubScreen == 4) {
        // Sub 4: Wi-Fi Setup

        if (aquarium.getWifiConnectState() != WIFI_CONN_IDLE) {
           if (aquarium.getWifiConnectState() != WIFI_CONN_CONNECTING) {
             aquarium.resetWifiConnectState();
             m_settingsNeedsRedraw = true;
           }
           return;
        }

        if (m_wifiShowKeyboard) {
          // Top Left Back Button [ < NET ] (X: 0..94, sy: 0..36)
          if (touchX <= 94 && sy <= 36) {
            m_wifiShowKeyboard = false;
            m_settingsNeedsRedraw = true;
            return;
          }

          // Mask Toggle Button [ MOSTRA / NASCONDI ] (X: 260..318, sy: 38..68)
          if (touchX >= 260 && sy >= 38 && sy <= 68) {
            m_wifiHidePassword = !m_wifiHidePassword;
            m_settingsNeedsRedraw = true;
            return;
          }

          // Keypad Matrix Rows (Updated for enlarged spacing: Y: 72, 108, 144, Height: 32)
          if (sy >= 72 && sy <= 104) {
            int kIdx = (touchX - 4) / 31;
            if (kIdx >= 0 && kIdx < 10) {
              const char* r1_low = "qwertyuiop";
              const char* r1_upp = "QWERTYUIOP";
              const char* r1_sym = "1234567890";
              char c = (m_kbLayoutMode == 0) ? r1_low[kIdx] : (m_kbLayoutMode == 1) ? r1_upp[kIdx] : r1_sym[kIdx];
              if (m_wifiTypedPassword.length() < 32) m_wifiTypedPassword += c;
            }
          } else if (sy >= 108 && sy <= 140) {
            int kIdx = (touchX - 19) / 31;
            if (kIdx >= 0 && kIdx < 9) {
              const char* r2_low = "asdfghjkl";
              const char* r2_upp = "ASDFGHJKL";
              const char* r2_sym = "!@#$%&*+-";
              char c = (m_kbLayoutMode == 0) ? r2_low[kIdx] : (m_kbLayoutMode == 1) ? r2_upp[kIdx] : r2_sym[kIdx];
              if (m_wifiTypedPassword.length() < 32) m_wifiTypedPassword += c;
            }
          } else if (sy >= 144 && sy <= 176) {
            if (touchX <= 44) {
              if (m_kbLayoutMode == 0) m_kbLayoutMode = 1;
              else if (m_kbLayoutMode == 1) m_kbLayoutMode = 0;
            } else if (touchX >= 260) {
              if (m_wifiTypedPassword.length() > 0) {
                m_wifiTypedPassword.remove(m_wifiTypedPassword.length() - 1);
              }
            } else {
              int kIdx = (touchX - 48) / 30;
              if (kIdx >= 0 && kIdx < 7) {
                const char* r3_low = "zxcvbnm";
                const char* r3_upp = "ZXCVBNM";
                const char* r3_sym = "=._:/?!";
                char c = (m_kbLayoutMode == 0) ? r3_low[kIdx] : (m_kbLayoutMode == 1) ? r3_upp[kIdx] : r3_sym[kIdx];
                if (m_wifiTypedPassword.length() < 32) m_wifiTypedPassword += c;
              }
            }
          } else if (sy >= 184 && sy <= 228) {
            if (touchX <= 70) {
              m_kbLayoutMode = (m_kbLayoutMode == 2) ? 0 : 2;
            } else if (touchX >= 71 && touchX <= 172) {
              if (m_wifiTypedPassword.length() < 32) m_wifiTypedPassword += " ";
            } else if (touchX >= 173) {
              aquarium.connectWifiSSID(m_wifiSelectedSSID, m_wifiTypedPassword);
              m_wifiShowKeyboard = false;
            }
          }
          m_settingsNeedsRedraw = true;
          return;
        }

        // WiFi Main Screen Touches (Updated for enlarged layout: Scan Y: 90..128)
        // Scan Button (sy: 90..128)
        if (sy >= 90 && sy <= 128) {
          m_wifiListPage = 0;
          aquarium.startAsyncWifiScan();
          m_settingsNeedsRedraw = true;
          return;
        }

        int netCount = aquarium.getWifiNetworkCount();
        int totalPages = (netCount + 2) / 3;

        // Up/Down Scroll Buttons (touchX >= 260 && totalPages > 1 - Updated for Y: 136..180 and 184..228)
        if (touchX >= 260 && totalPages > 1) {
          if (sy >= 136 && sy <= 180) {
            if (m_wifiListPage > 0) m_wifiListPage--;
          } else if (sy >= 184 && sy <= 228) {
            if (m_wifiListPage < totalPages - 1) m_wifiListPage++;
          }
          m_settingsNeedsRedraw = true;
          return;
        }

        // Tapping Scanned Network Items (sy: 136..244, touchX < 260)
        if (sy >= 136 && sy <= 244) {
          int slot = (sy - 136) / 36;
          if (slot >= 0 && slot < 3) {
            int netIdx = m_wifiListPage * 3 + slot;
            if (netIdx >= 0 && netIdx < netCount) {
              WifiNetworkItem net = aquarium.getWifiNetwork(netIdx);
              m_wifiSelectedSSID = net.ssid;
              m_wifiTypedPassword = "";
              if (net.open) {
                aquarium.connectWifiSSID(net.ssid, "");
              } else {
                m_wifiShowKeyboard = true;
              }
              m_settingsNeedsRedraw = true;
            }
          }
        }
      } else if (m_settingsSubScreen == 5) {
        if (m_isUpdating) return; // Block touches during update
        // Sub 5: System Info (Update Version)
        if (m_showUpdatePrompt) {
          // YES (X: 30..140, Y: 140..180)
          if (touchX >= 30 && touchX <= 140 && sy >= 140 && sy <= 180) {
            m_showUpdatePrompt = false;
            m_isUpdating = true;
            m_settingsNeedsRedraw = true;
            drawSettingsSubScreen(5); // Force draw update loading immediately
            aquarium.performOTAUpdate(m_latestVersionUrl);
            m_isUpdating = false;
          }
          // NO (X: 180..290, Y: 140..180)
          else if (touchX >= 180 && touchX <= 290 && sy >= 140 && sy <= 180) {
            m_showUpdatePrompt = false;
            m_settingsNeedsRedraw = true;
          }
        } else {
          // Update/Verify Button (X: 10..310, Y: 176..212)
          if (touchX >= 10 && touchX <= 310 && sy >= 176 && sy <= 212) {
            if (m_latestVersion != "" && m_latestVersion != String(AQUARIUM_OS_VERSION)) {
              m_showUpdatePrompt = true;
              m_settingsNeedsRedraw = true;
            } else {
              m_isCheckingUpdate = true;
              m_settingsNeedsRedraw = true;
              drawSettingsSubScreen(5); // Draw "Controllo in corso..."
              
              String ver, url;
              if (aquarium.checkGitHubForUpdate(ver, url)) {
                setLatestVersion(ver, url);
              } else {
                setLatestVersion(AQUARIUM_OS_VERSION, "");
              }
            }
          }
        }
      } else if (m_settingsSubScreen == 6) {
        // Sub 6: Language Selector (Updated for Y: 40 + i * 44)
        if (sy >= 40) {
          int idx = (sy - 40) / 44;
          if (idx >= 0 && idx < langManager.getAvailableLanguageCount()) {
            LangItem item = langManager.getAvailableLanguage(idx);
            aquarium.setLanguageFile(item.filename);
            delay(300);
            ESP.restart();
          }
        }
      } else if (m_settingsSubScreen == 7) {
        // Sub 7: Energy Saving (Updated for Y: 110..154 and larger buttons)
        static const uint16_t SS_TIMES[] = { 0, 30, 60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 900, 1200, 1800, 2700, 3600 };
        static const int      SS_COUNT   = sizeof(SS_TIMES) / sizeof(SS_TIMES[0]);
        if (sy >= 110 && sy <= 154) {
          // "<" Prev button (x: 16..86)
          if (touchX >= 16 && touchX <= 86) {
            if (m_screensaverIdx > 0) m_screensaverIdx--;
            aquarium.setScreensaverTime(SS_TIMES[m_screensaverIdx]);
            m_settingsNeedsRedraw = true;
          }
          // ">" Next button (x: 234..304)
          else if (touchX >= 234 && touchX <= 304) {
            if (m_screensaverIdx < SS_COUNT - 1) m_screensaverIdx++;
            aquarium.setScreensaverTime(SS_TIMES[m_screensaverIdx]);
            m_settingsNeedsRedraw = true;
          }
        }
      } else if (m_settingsSubScreen == 8) {
        // Sub 8: Factory Reset (Updated for enlarged Y: 128..178)
        if (sy >= 128 && sy <= 178) {
          // YES button (X: 30..140)
          if (touchX >= 30 && touchX <= 140) {
            if (m_resetStep == 0) {
              m_resetStep = 1;
              m_settingsNeedsRedraw = true;
            } else {
              GFX->fillScreen(COLOR_OCEAN_DEPTH);
              GFX->setTextColor(COLOR_CORAL_RED);
              GFX->setTextDatum(MC_DATUM);
              GFX->drawString("FORMATTING SD CARD...", 160, 70, 2);
              GFX->drawString("PLEASE WAIT...", 160, 102, 2);
              
              BYTE* work = (BYTE*)malloc(sizeof(BYTE) * FF_MAX_SS);
              if (work) {
                f_mkfs("0:", FM_ANY, 0, work, sizeof(BYTE) * FF_MAX_SS);
                free(work);
              }
              delay(1000);
              ESP.restart();
            }
          }
          // NO button (X: 180..290)
          else if (touchX >= 180 && touchX <= 290) {
            m_settingsSubScreen = 0;
            m_resetStep = 0;
            m_settingsNeedsRedraw = true;
          }
        }
      } else if (m_settingsSubScreen == 9) {
        // Sub 9: Hardware Mapping
        if (touchX >= 160 && touchX <= 200) { // Minus
          if (sy >= 46 && sy <= 78) ::cfg.mcpPinLight = max(-1, (int)::cfg.mcpPinLight - 1);
          if (sy >= 86 && sy <= 118) ::cfg.mcpPinWaterLevel = max(-1, (int)::cfg.mcpPinWaterLevel - 1);
          m_settingsNeedsRedraw = true;
          extern bool writeWholeConfigFileSafe();
          writeWholeConfigFileSafe();
        } else if (touchX >= 260 && touchX <= 300) { // Plus
          if (sy >= 46 && sy <= 78) ::cfg.mcpPinLight = min(15, (int)::cfg.mcpPinLight + 1);
          if (sy >= 86 && sy <= 118) ::cfg.mcpPinWaterLevel = min(15, (int)::cfg.mcpPinWaterLevel + 1);
          m_settingsNeedsRedraw = true;
          extern bool writeWholeConfigFileSafe();
          writeWholeConfigFileSafe();
        }
        // Scan Button: (10, 204, 100, 32)
        if (touchX >= 10 && touchX <= 110 && sy >= 204 && sy <= 236) {
          m_settingsNeedsRedraw = true; // Triggers redraw which calls scanI2C()
        }
      } else if (m_settingsSubScreen == 10) {
        // Sub 10: Schermo & Auto-Dimming
        // Toggle Button (X: 10..310, Y: 140..190)
        if (touchX >= 10 && touchX <= 310 && sy >= 140 && sy <= 190) {
          aquarium.setAutoDimming(!cfg.autoDimming);
          m_settingsNeedsRedraw = true;
        }
      }



}


void AquariumUI::drawSubScreenScreen() {
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< MENU"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  
  GFX->setTextColor(COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->drawString(langManager.getText("TITLE_SCREEN", "SCREEN"), 190, 10, 2);
  
  // Real-time Lux/mV display
  GFX->fillRoundRect(10, 60, 300, 60, 8, COLOR_CARD_BORDER);
  GFX->drawRoundRect(10, 60, 300, 60, 8, COLOR_CYAN_GLOW);
  
  GFX->setTextColor(TFT_WHITE);
  GFX->setTextDatum(MC_DATUM);
  String luxText = String(langManager.getText("LABEL_BRIGHTNESS", "Brightness:")) + " " + String(aquarium.getLdrValue()) + " mV";
  GFX->drawString(luxText, 160, 90, 4);

  // Auto-Dimming Toggle Button
  const AquariumConfig& cfg2 = aquarium.getConfig();
  uint16_t btnColor = cfg2.autoDimming ? COLOR_EMERALD_GREEN : COLOR_CARD_BORDER;
  uint16_t txtColor = cfg2.autoDimming ? TFT_BLACK : TFT_WHITE;
  
  String toggleTxt = cfg2.autoDimming ? langManager.getText("BTN_AUTODIM_ON", "Auto-Dimming: ON") : langManager.getText("BTN_AUTODIM_OFF", "Auto-Dimming: OFF");
  drawTouchButton(10, 140, 300, 50, toggleTxt.c_str(), btnColor, txtColor, COLOR_CYAN_GLOW);
}
