#include "../include/aquarium_ui.h"
#include "../include/language_manager.h"

#define GFX ((m_activeTab == TAB_SETTINGS) ? (TFT_eSPI *)m_tft : (TFT_eSPI *)m_sprite)

// ------------------------------------------------------------------------------
// WIFI KEYBOARD
// ------------------------------------------------------------------------------
void AquariumUI::drawWifiKeyboard() {
  // 1. Top Header: Consistent Enlarged Back Button + Selected SSID
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_NETWORK", "< NET"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  String headerStr = "SSID: " + m_wifiSelectedSSID;
  if (headerStr.length() > 10)
    headerStr = headerStr.substring(0, 10) + "..";
  GFX->drawString(headerStr.c_str(), 96, 10, 2);

  // Top Left Back Button (Already drawn in caller? If not, draw it here)
  drawTouchButton(2, 2, 80, 32, langManager.getText("BTN_BACK", "< NET"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

  // 2. Password Display Box & Mask Toggle (Shifted down: Y: 38, Height: 30)
  drawGlassCard(6, 38, 256, 30, COLOR_CYAN_GLOW);

  GFX->setTextDatum(TL_DATUM);
  String displayPass = "";
  if (m_wifiHidePassword) {
    for (size_t i = 0; i < m_wifiTypedPassword.length(); i++)
      displayPass += "*";
  } else {
    displayPass = m_wifiTypedPassword;
  }
  if (displayPass.length() == 0) {
    GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    displayPass = langManager.getText("MSG_ENTER_PASS", "Enter password...");
  } else {
    GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  }
  GFX->drawString(displayPass.c_str(), 12, 45, 2);

  // Mask Toggle Button (Shifted down: Y: 38)
  drawTouchButton(266, 38, 48, 30,
                  m_wifiHidePassword ? langManager.getText("BTN_SHOW", "SHOW")
                                     : langManager.getText("BTN_HIDE", "HIDE"),
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

  // 3. Keypad Matrix (Shifted Y spacing: Y: 72, 108, 144, Height: 32)
  const char *row1_lower[10] = {"q", "w", "e", "r", "t",
                                "y", "u", "i", "o", "p"};
  const char *row1_upper[10] = {"Q", "W", "E", "R", "T",
                                "Y", "U", "I", "O", "P"};
  const char *row1_symb[10] = {"1", "2", "3", "4", "5",
                               "6", "7", "8", "9", "0"};

  const char *row2_lower[9] = {"a", "s", "d", "f", "g", "h", "j", "k", "l"};
  const char *row2_upper[9] = {"A", "S", "D", "F", "G", "H", "J", "K", "L"};
  const char *row2_symb[9] = {"!", "@", "#", "$", "%", "&", "*", "-", "+"};

  const char *row3_lower[7] = {"z", "x", "c", "v", "b", "n", "m"};
  const char *row3_upper[7] = {"Z", "X", "C", "V", "B", "N", "M"};
  const char *row3_symb[7] = {"=", ".", "_", ":", "/", ";", "?"};

  int y1 = 72, y2 = 108, y3 = 144;

  // Row 1 (10 keys)
  for (int i = 0; i < 10; i++) {
    const char *k = (m_kbLayoutMode == 0)   ? row1_lower[i]
                    : (m_kbLayoutMode == 1) ? row1_upper[i]
                                            : row1_symb[i];
    drawTouchButton(4 + i * 31, y1, 29, 32, k, COLOR_CARD_BG, TFT_WHITE,
                    COLOR_CARD_BORDER);
  }

  // Row 2 (9 keys)
  for (int i = 0; i < 9; i++) {
    const char *k = (m_kbLayoutMode == 0)   ? row2_lower[i]
                    : (m_kbLayoutMode == 1) ? row2_upper[i]
                                            : row2_symb[i];
    drawTouchButton(19 + i * 31, y2, 29, 32, k, COLOR_CARD_BG, TFT_WHITE,
                    COLOR_CARD_BORDER);
  }

  // Row 3: Shift / Mode Key (Shift button enlarged)
  const char *shiftLabel = (m_kbLayoutMode == 0)   ? "abc"
                           : (m_kbLayoutMode == 1) ? "ABC"
                                                   : "123";
  drawTouchButton(4, y3, 40, 32, shiftLabel, COLOR_CARD_BG, COLOR_GOLD_ACCENT,
                  COLOR_CYAN_GLOW);

  // Row 3 Middle (7 keys)
  for (int i = 0; i < 7; i++) {
    const char *k = (m_kbLayoutMode == 0)   ? row3_lower[i]
                    : (m_kbLayoutMode == 1) ? row3_upper[i]
                                            : row3_symb[i];
    drawTouchButton(48 + i * 30, y3, 28, 32, k, COLOR_CARD_BG, TFT_WHITE,
                    COLOR_CARD_BORDER);
  }

  // Delete Backspace Button (Enlarged)
  drawTouchButton(262, y3, 54, 32, langManager.getText("BTN_DEL", "DEL"),
                  COLOR_CARD_BG, COLOR_CORAL_RED, COLOR_CORAL_RED);

  // 4. Bottom Control Bar (Larger: Y: 184, Height: 44)
  drawTouchButton(4, 184, 65, 44, (m_kbLayoutMode == 2) ? "ABC" : "123",
                  COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  drawTouchButton(73, 184, 105, 44, langManager.getText("BTN_SPACE", "SPACE"),
                  COLOR_CARD_BG, TFT_WHITE, COLOR_CARD_BORDER);
  drawTouchButton(182, 184, 134, 44,
                  langManager.getText("BTN_CONNECT", "CONNECT"),
                  COLOR_EMERALD_GREEN, TFT_BLACK, COLOR_EMERALD_GREEN);
}

void AquariumUI::handleKeyboardTouch(int touchX, int touchY) {
  int sy = touchY; // Keyboard is on settings tab, y offset is 0

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

  // Keypad Matrix Rows (Updated for enlarged spacing: Y: 72, 108, 144,
  // Height: 32)
  if (sy >= 72 && sy <= 104) {
    int kIdx = (touchX - 4) / 31;
    if (kIdx >= 0 && kIdx < 10) {
      const char *r1_low = "qwertyuiop";
      const char *r1_upp = "QWERTYUIOP";
      const char *r1_sym = "1234567890";
      char c = (m_kbLayoutMode == 0)   ? r1_low[kIdx]
               : (m_kbLayoutMode == 1) ? r1_upp[kIdx]
                                       : r1_sym[kIdx];
      if (m_wifiTypedPassword.length() < 32)
        m_wifiTypedPassword += c;
    }
  } else if (sy >= 108 && sy <= 140) {
    int kIdx = (touchX - 19) / 31;
    if (kIdx >= 0 && kIdx < 9) {
      const char *r2_low = "asdfghjkl";
      const char *r2_upp = "ASDFGHJKL";
      const char *r2_sym = "!@#$%&*+-";
      char c = (m_kbLayoutMode == 0)   ? r2_low[kIdx]
               : (m_kbLayoutMode == 1) ? r2_upp[kIdx]
                                       : r2_sym[kIdx];
      if (m_wifiTypedPassword.length() < 32)
        m_wifiTypedPassword += c;
    }
  } else if (sy >= 144 && sy <= 176) {
    if (touchX <= 44) {
      if (m_kbLayoutMode == 0)
        m_kbLayoutMode = 1;
      else if (m_kbLayoutMode == 1)
        m_kbLayoutMode = 0;
    } else if (touchX >= 260) {
      if (m_wifiTypedPassword.length() > 0) {
        m_wifiTypedPassword.remove(m_wifiTypedPassword.length() - 1);
      }
    } else {
      int kIdx = (touchX - 48) / 30;
      if (kIdx >= 0 && kIdx < 7) {
        const char *r3_low = "zxcvbnm";
        const char *r3_upp = "ZXCVBNM";
        const char *r3_sym = "=._:/;?";
        char c = (m_kbLayoutMode == 0)   ? r3_low[kIdx]
                 : (m_kbLayoutMode == 1) ? r3_upp[kIdx]
                                         : r3_sym[kIdx];
        if (m_wifiTypedPassword.length() < 32)
          m_wifiTypedPassword += c;
      }
    }
  } else if (sy >= 184 && sy <= 228) {
    if (touchX <= 69) {
      m_kbLayoutMode = (m_kbLayoutMode == 2) ? 0 : 2;
    } else if (touchX >= 73 && touchX <= 178) {
      if (m_wifiTypedPassword.length() < 32)
        m_wifiTypedPassword += " ";
    } else if (touchX >= 182) {
      ::cfg.wifiSsid = m_wifiSelectedSSID;
      ::cfg.wifiPassword = m_wifiTypedPassword;
      extern bool writeWholeConfigFileSafe();
      writeWholeConfigFileSafe();
      aquarium.connectWifiSSID(m_wifiSelectedSSID, m_wifiTypedPassword);
      m_wifiShowKeyboard = false;
    }
  }
  m_settingsNeedsRedraw = true;
}
