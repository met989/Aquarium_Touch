#include "aquarium_ui.h"

#define GFX ((m_activeTab == TAB_SETTINGS) ? (TFT_eSPI*)m_tft : (TFT_eSPI*)m_sprite)


#define GFX ((m_activeTab == TAB_SETTINGS) ? (TFT_eSPI*)m_tft : (TFT_eSPI*)m_sprite)

#include "aquarium_logic.h"
#include "language_manager.h"
#include "ff.h"
#include <math.h>

extern AppConfig cfg;
extern bool writeWholeConfigFileSafe();

AquariumUI aquariumUI;

AquariumUI::AquariumUI() {}

void AquariumUI::init(TFT_eSPI* tft) {
  m_tft = tft;
  m_tft->fillScreen(COLOR_OCEAN_DEPTH);

  // Allocate 320x172 Sprite (Proven ~110 KB size for ESP32 heap allocation)
  m_sprite = new TFT_eSprite(m_tft);
  GFX->setFreeFont(nullptr);
  m_sprite->setColorDepth(16);

  if (!m_sprite->createSprite(UI_SPRITE_WIDTH, UI_SPRITE_HEIGHT)) {
    m_sprite->createSprite(320, 160);
  }

  initAnimations();

  // Force initial draw of Navbar
  drawNavBar(true);
}

void AquariumUI::setTab(UiTab tab) {
  if (m_activeTab != tab) {
    m_activeTab = tab;
    m_settingsSubScreen = 0;      // Reset subscreen when tab changes
    m_settingsMenuPage = 0;        // Reset settings menu page when tab changes
    m_settingsNeedsRedraw = true;  // Force settings redraw
    drawNavBar(true);
  }
}

void AquariumUI::initAnimations() {
  // 1. Water Bubbles Initialization inside Sprite space (Y: 0 to 172)
  for (int i = 0; i < UI_BUBBLE_COUNT; i++) {
    m_bubbles[i].x = random(10, 310);
    m_bubbles[i].y = random(10, 165);
    m_bubbles[i].radius = random(2, 5);
    m_bubbles[i].speed = 0.5f + (float)random(10, 35) / 20.0f;
    m_bubbles[i].wobble = random(0, 100) / 10.0f;
    m_bubbles[i].wobbleSpeed = 0.04f + (float)random(10, 30) / 500.0f;
  }

  // 2. Swimming Fish Initialization
  uint16_t fishColors[4] = { COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW, COLOR_EMERALD_GREEN, COLOR_CORAL_RED };
  for (int i = 0; i < UI_FISH_COUNT; i++) {
    m_fish[i].x = (float)random(-20, 320);
    m_fish[i].y = (float)random(40, 140);
    m_fish[i].speed = 0.5f + (float)random(2, 10) / 10.0f;
    m_fish[i].dir = (i % 2 == 0) ? 1 : -1;
    m_fish[i].tailPhase = (float)i * 1.5f;
    m_fish[i].color = fishColors[i % 4];
  }

}

void AquariumUI::updateAnimations() {
  // Animations run on ALL tabs EXCEPT TAB_SETTINGS to conserve CPU and SPI resources in menus
  if (m_activeTab == TAB_SETTINGS) {
    m_animatedTemp = aquarium.getTemperature();
    return;
  }

  uint32_t now = millis();
  if (now - m_lastAnimTime < UI_ANIM_UPDATE_INTERVAL_MS) return; // Parameterized FPS animation cycle
  m_lastAnimTime = now;

  // Wave phase animation
  m_wavePhase += 0.08f;

  // Smooth Temperature interpolation for Dashboard gauge
  float targetT = aquarium.getTemperature();
  m_animatedTemp = m_animatedTemp * 0.9f + targetT * 0.1f;

  // Update Bubbles
  for (int i = 0; i < UI_BUBBLE_COUNT; i++) {
    m_bubbles[i].y -= m_bubbles[i].speed;
    m_bubbles[i].wobble += m_bubbles[i].wobbleSpeed;
    m_bubbles[i].x += sinf(m_bubbles[i].wobble) * 0.35f;

    if (m_bubbles[i].y < 2) {
      m_bubbles[i].y = 168;
      m_bubbles[i].x = random(10, 310);
    }
  }

  // Update Swimming Fish
  for (int i = 0; i < UI_FISH_COUNT; i++) {
    m_fish[i].x += m_fish[i].speed * (float)m_fish[i].dir;
    m_fish[i].tailPhase += 0.25f;

    if (m_fish[i].dir > 0 && m_fish[i].x > 335) {
      m_fish[i].x = -15;
      m_fish[i].y = random(40, 130);
    } else if (m_fish[i].dir < 0 && m_fish[i].x < -15) {
      m_fish[i].x = 335;
      m_fish[i].y = random(40, 130);
    }
  }
}

void AquariumUI::update() {
  aquarium.update();

  // 1. Update Animations (skips calculations if on SETTINGS)
  updateAnimations();

  // 2. Render Main Content into Sprite (Y: 14..185 => Sprite Height 172)
  if (m_sprite && m_sprite->created()) {

    if (m_activeTab != TAB_SETTINGS) {
      drawOceanBackground();
      drawSwimmingFish();
      drawWaterWaves();
    }

    switch (m_activeTab) {
      case TAB_DASHBOARD: drawDashboardTab(); break;
      case TAB_LIGHT:     drawLightTab();     break;
      case TAB_SCHEDULE:  drawScheduleTab();  break;
      case TAB_SETTINGS:  
        if (m_settingsNeedsRedraw) {
          m_settingsNeedsRedraw = false;
          if (m_settingsSubScreen > 0) {
            m_tft->fillRect(0, 0, 320, 240, COLOR_BG_OCEAN);
            m_tft->fillRect(0, 160, 320, 80, COLOR_OCEAN_DEPTH);
          } else {
            m_tft->fillRect(0, 0, 320, UI_NAVBAR_Y, COLOR_BG_OCEAN);
            m_tft->fillRect(0, 120, 320, UI_NAVBAR_Y - 120, COLOR_OCEAN_DEPTH);
          }
          drawSettingsTab(); 
          if (m_settingsSubScreen == 0) {
            drawNavBar(true);
          }
        }
        break;
    }

    if (m_activeTab != TAB_SETTINGS) {
      // Fill top margin space
      if (UI_SPRITE_Y_OFFSET > 0) {
        m_tft->fillRect(0, 0, 320, UI_SPRITE_Y_OFFSET, COLOR_BG_OCEAN);
      }

      // Push Double-Buffered Content Sprite
      m_sprite->pushSprite(0, UI_SPRITE_Y_OFFSET);

      // Seamlessly fill ocean transition gap between sprite bottom and navbar top (190)
      if (UI_NAVBAR_Y > UI_SPRITE_Y_OFFSET + UI_SPRITE_HEIGHT) {
        m_tft->fillRect(0, UI_SPRITE_Y_OFFSET + UI_SPRITE_HEIGHT, 320, UI_NAVBAR_Y - (UI_SPRITE_Y_OFFSET + UI_SPRITE_HEIGHT), COLOR_OCEAN_DEPTH);
      }
    }
  }

  // 3. Update Navbar (only redrawn when tab changes or forced)
  drawNavBar(false);
}




void AquariumUI::drawOceanBackground() {
  m_sprite->fillSprite(COLOR_BG_OCEAN);

  // Abyssal Gradient at bottom of sprite
  GFX->fillRect(0, 120, 320, 52, COLOR_OCEAN_DEPTH);

  // Draw Rising Water Bubbles on all tabs EXCEPT Settings
  if (m_activeTab != TAB_SETTINGS) {
    for (int i = 0; i < UI_BUBBLE_COUNT; i++) {
      int bx = (int)m_bubbles[i].x;
      int by = (int)m_bubbles[i].y;
      int r = (int)m_bubbles[i].radius;

      if (by >= 0 && by <= UI_SPRITE_HEIGHT) {
        GFX->drawCircle(bx, by, r, COLOR_CARD_BORDER);
        GFX->drawPixel(bx - 1, by - 1, COLOR_CYAN_GLOW);
      }
    }
  }
}

void AquariumUI::drawWaterWaves() {
  if (m_activeTab == TAB_SETTINGS) return;

  // Oscillating Sine Wave surface effect at top of sprite (Y = 0)
  for (int x = 0; x < 320; x += 2) {
    int wy = (int)(sinf((float)x * 0.04f + m_wavePhase) * 2.5f) + 1;
    if (wy >= 0 && wy < UI_SPRITE_HEIGHT) {
      GFX->drawPixel(x, wy, COLOR_CYAN_GLOW);
      GFX->drawPixel(x + 1, wy + 1, COLOR_WATER_TOP);
    }
  }
}

void AquariumUI::drawSwimmingFish() {
  if (m_activeTab == TAB_SETTINGS) return;


  for (int i = 0; i < UI_FISH_COUNT; i++) {
    int fx = (int)m_fish[i].x;
    int fy = (int)m_fish[i].y;
    int d = m_fish[i].dir;
    uint16_t col = m_fish[i].color;

    // Fish Body
    GFX->fillCircle(fx, fy, 4, col);
    GFX->fillCircle(fx + d * 3, fy, 3, col);
    GFX->fillCircle(fx + d * 6, fy, 2, col);

    // Fish Wiggling Tail
    int tailWiggle = (int)(sinf(m_fish[i].tailPhase) * 3.0f);
    int tx = fx - d * 7;
    GFX->drawLine(tx, fy, tx - d * 4, fy - 3 + tailWiggle, col);
    GFX->drawLine(tx, fy, tx - d * 4, fy + 3 + tailWiggle, col);

    // Eye
    GFX->drawPixel(fx + d * 5, fy - 1, TFT_BLACK);
  }
}

void AquariumUI::drawNavBar(bool force) {
  if (m_activeTab == TAB_SETTINGS) {
      if (m_settingsSubScreen > 0) return; // Navbar is hidden in subscreens
      if (!force && m_lastDrawTab == (int)m_activeTab) return;
      m_lastDrawTab = (int)m_activeTab;

      // Clear Navbar area for Settings custom buttons
      m_tft->fillRect(0, UI_NAVBAR_Y, 320, UI_NAVBAR_HEIGHT, COLOR_OCEAN_DEPTH);
      m_tft->drawFastHLine(0, UI_NAVBAR_Y, 320, COLOR_CARD_BORDER);

      if (m_settingsSubScreen == 0) {
          // Main Menu Navbar
          // Back Button
          m_tft->fillRoundRect(10, UI_NAVBAR_Y + 5, 140, 40, 8, COLOR_CARD_BORDER);
          m_tft->drawRoundRect(10, UI_NAVBAR_Y + 5, 140, 40, 8, COLOR_CYAN_GLOW);
          m_tft->setTextColor(TFT_WHITE);
          m_tft->setTextDatum(MC_DATUM);
          m_tft->drawString(langManager.getText("BTN_BACK", "INDIETRO"), 80, UI_NAVBAR_Y + 25, 2);

          // Next Page Button
          m_tft->fillRoundRect(170, UI_NAVBAR_Y + 5, 140, 40, 8, COLOR_CARD_BORDER);
          m_tft->drawRoundRect(170, UI_NAVBAR_Y + 5, 140, 40, 8, COLOR_CYAN_GLOW);
          if (m_settingsMenuPage == 0) {
              m_tft->drawString(">>", 240, UI_NAVBAR_Y + 25, 2);
          } else {
              m_tft->drawString("<<", 240, UI_NAVBAR_Y + 25, 2);
          }
      }
      return;
  }

  if (!force && m_lastDrawTab == (int)m_activeTab) return; // Only update on tab change!
  m_lastDrawTab = (int)m_activeTab;

  m_tft->fillRect(0, UI_NAVBAR_Y, 320, UI_NAVBAR_HEIGHT, COLOR_NAV_BG);
  m_tft->drawFastHLine(0, UI_NAVBAR_Y, 320, COLOR_CARD_BORDER);

  const char* tabs[4] = {
    langManager.getText("TAB_DASH", "HOME"),
    langManager.getText("TAB_LIGHT", "LIGHT"),
    langManager.getText("TAB_TIMER", "TIMER"),
    langManager.getText("TAB_SETT", "SETT")
  };

  uint16_t colors[4] = {COLOR_CYAN_GLOW, COLOR_GOLD_ACCENT, COLOR_EMERALD_GREEN, COLOR_CYAN_GLOW};

  for (int i = 0; i < 4; i++) {
    int x = i * 80;
    bool selected = (m_activeTab == (UiTab)i);

    if (selected) {
      m_tft->fillRoundRect(x + 4, UI_NAVBAR_Y + 4, 72, UI_NAVBAR_HEIGHT - 8, 8, COLOR_CARD_BG);
      m_tft->drawRoundRect(x + 4, UI_NAVBAR_Y + 4, 72, UI_NAVBAR_HEIGHT - 8, 8, colors[i]);
      m_tft->fillRect(x + 20, UI_NAVBAR_Y + UI_NAVBAR_HEIGHT - 6, 40, 3, colors[i]); // Active Neon Indicator
      m_tft->setTextColor(colors[i], COLOR_CARD_BG);
    } else {
      m_tft->setTextColor(COLOR_TEXT_MUTED, COLOR_NAV_BG);
    }

    m_tft->setTextDatum(MC_DATUM);
    m_tft->drawString(tabs[i], x + 40, UI_NAVBAR_Y + UI_NAVBAR_HEIGHT / 2 - 1, 2);
  }
}

// ------------------------------------------------------------------------------
// TAB 0: DASHBOARD (Rendered in Sprite relative coordinates)
// ------------------------------------------------------------------------------
void AquariumUI::drawDashboardTab() {
  // Left: Circular Temperature Gauge (Center X: 90, Y: 86 inside sprite)
  drawCircularTempGauge(90, 86, 52, m_animatedTemp, aquarium.getConfig().targetTempMin, aquarium.getConfig().targetTempMax);

  // Status Badge below gauge
  TempStatus status = aquarium.getTempStatus();
  uint16_t statusCol = COLOR_EMERALD_GREEN;
  const char* statusTxt = langManager.getText("MSG_OPTIMAL", "OPTIMAL");
  if (status == TEMP_TOO_COLD) { statusCol = COLOR_CYAN_GLOW; statusTxt = langManager.getText("MSG_COLD", "COLD"); }
  if (status == TEMP_TOO_HOT)  { statusCol = COLOR_CORAL_RED;  statusTxt = langManager.getText("MSG_HOT", "HOT"); }

  GFX->fillRoundRect(30, 144, 120, 22, 11, statusCol);
  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(TFT_BLACK, statusCol);
  GFX->drawString(statusTxt, 90, 155, 2);

  // Right Top: Live Date & Time Card (X: 176, Y: 8, W: 138, H: 48)
  drawGlassCard(176, 8, 138, 48, COLOR_CYAN_GLOW);

  int h, m, s;
  aquarium.getTime(h, m, s);

  char timeBuf[12], dateBuf[16];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", h, m, s);
  aquarium.getFormattedDate(dateBuf, sizeof(dateBuf));

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
  GFX->drawString(timeBuf, 245, 20, 2);

  GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
  GFX->drawString(dateBuf, 245, 38, 2);


  // Right Middle: Light Card (X: 180, Y: 60, W: 130, H: 52)
  bool lightOn = aquarium.isLightOn();
  uint16_t lightBorder = lightOn ? COLOR_GOLD_ACCENT : COLOR_CARD_BORDER;
  drawGlassCard(180, 60, 130, 52, lightBorder);

  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("TITLE_LIGHT_CONTROL", "AQUARIUM LIGHT"), 188, 64, 1);

  GFX->setTextDatum(MC_DATUM);
  if (lightOn) {
    int animRay = (int)(sinf(m_wavePhase * 2.0f) * 2.0f);
    GFX->fillCircle(206, 92, 8 + animRay, COLOR_GOLD_ACCENT);
    GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_LIGHT_ON", "ON"), 258, 92, 2);
  } else {
    GFX->drawCircle(206, 92, 8, COLOR_MOON_BLUE);
    GFX->fillCircle(204, 90, 6, COLOR_CARD_BG);
    GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_LIGHT_OFF", "OFF"), 258, 92, 2);
  }

  // Right Bottom: Min/Max Stats Card (X: 180, Y: 116, W: 130, H: 50)
  drawGlassCard(180, 116, 130, 50, COLOR_CARD_BORDER);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString("TEMP MIN / MAX", 188, 120, 1);

  char minBuf[16], maxBuf[16];
  snprintf(minBuf, sizeof(minBuf), "Min: %.1f C", aquarium.getMinTemp());
  snprintf(maxBuf, sizeof(maxBuf), "Max: %.1f C", aquarium.getMaxTemp());

  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
  GFX->drawString(minBuf, 188, 134, 1);
  GFX->setTextColor(COLOR_CORAL_RED, COLOR_CARD_BG);
  GFX->drawString(maxBuf, 188, 148, 1);
}



// ------------------------------------------------------------------------------
// TAB 1: LIGHT CONTROL
// ------------------------------------------------------------------------------
void AquariumUI::drawLightTab() {
  bool lightOn = aquarium.isLightOn();
  uint16_t cardBorder = lightOn ? COLOR_GOLD_ACCENT : COLOR_CARD_BORDER;
  uint16_t statusCol = lightOn ? COLOR_GOLD_ACCENT : COLOR_TEXT_MUTED;
  const char* statusTxt = lightOn ? langManager.getText("MSG_LIGHT_ON", "LIGHT ON (ON)") : langManager.getText("MSG_LIGHT_OFF", "LIGHT OFF (OFF)");

  // Main Light Status Card (X: 10, Y: 8, W: 300, H: 86)
  drawGlassCard(10, 8, 300, 86, cardBorder);

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(statusCol, COLOR_CARD_BG);
  GFX->drawString(statusTxt, 160, 32, 4);

  // Big Touch Toggle Button inside Card
  drawTouchButton(50, 52, 220, 34, lightOn ? langManager.getText("LABEL_TURN_OFF_LIGHT", "TURN OFF LIGHT") : langManager.getText("LABEL_TURN_ON_LIGHT", "TURN ON LIGHT"), COLOR_CARD_BG, statusCol, cardBorder);

  // Mode Selection Card (X: 10, Y: 102, W: 300, H: 62)
  bool autoMode = aquarium.isAutoSchedule();
  drawGlassCard(10, 102, 300, 62, autoMode ? COLOR_EMERALD_GREEN : COLOR_GOLD_ACCENT);

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_MODE", "MODE:"), 70, 133, 2);

  drawTouchButton(140, 114, 160, 38, autoMode ? langManager.getText("MSG_AUTO", "AUTO (TIMER)") : langManager.getText("MSG_MANUAL", "MANUAL"), COLOR_CARD_BG, autoMode ? COLOR_EMERALD_GREEN : COLOR_GOLD_ACCENT);
}


// ------------------------------------------------------------------------------
// TAB 2: TIMER & LIGHT SCHEDULE
// ------------------------------------------------------------------------------
void AquariumUI::drawScheduleTab() {
  const AquariumConfig& cfg = aquarium.getConfig();

  // Title
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("TITLE_TIMER_SCHEDULE", "DAILY TIMER SCHEDULE"), 160, 6, 2);

  // ON Schedule Card (X: 10, Y: 24, W: 300, H: 65)
  drawGlassCard(10, 24, 300, 65, COLOR_GOLD_ACCENT);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_ON_TIME", "TURN ON"), 18, 30, 2);

  char onBuf[12];
  snprintf(onBuf, sizeof(onBuf), "%02d : %02d", cfg.lightOnHour, cfg.lightOnMin);
  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString(onBuf, 150, 60, 4);

  // Touch Buttons for ON Hour [-H] [+H]
  drawTouchButton(210, 40, 42, 34, "-H", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(258, 40, 42, 34, "+H", COLOR_CARD_BORDER, TFT_WHITE);

  // OFF Schedule Card (X: 10, Y: 96, W: 300, H: 65)
  drawGlassCard(10, 96, 300, 65, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_OFF_TIME", "TURN OFF"), 18, 102, 2);

  char offBuf[12];
  snprintf(offBuf, sizeof(offBuf), "%02d : %02d", cfg.lightOffHour, cfg.lightOffMin);
  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString(offBuf, 150, 132, 4);

  // Touch Buttons for OFF Hour [-H] [+H]
  drawTouchButton(210, 112, 42, 34, "-H", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(258, 112, 42, 34, "+H", COLOR_CARD_BORDER, TFT_WHITE);
}

// ------------------------------------------------------------------------------
// TAB 3: SETTINGS / MENU IMPOSTAZIONI DISPOSITIVO (Gear Icon)
// ------------------------------------------------------------------------------
void AquariumUI::drawSettingsTab() {
  drawSettingsSubScreen(m_settingsSubScreen);
}

void AquariumUI::drawSettingsMenu() {
      char titleBuf[32];
      snprintf(titleBuf, sizeof(titleBuf), "%s (Pagina %d/2)", langManager.getText("TITLE_SETTINGS", "SETTINGS"), m_settingsMenuPage + 1);
      GFX->setTextDatum(TC_DATUM);
      GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
      GFX->drawString(titleBuf, 160, 0, 2);

      if (m_settingsMenuPage == 1) {
          GFX->setTextDatum(MC_DATUM);
          GFX->setTextColor(COLOR_TEXT_MUTED);
          GFX->drawString("Questa pagina e' ancora vuota", 160, 90, 2);
          return;
      }

      const char* items[] = {
        langManager.getText("MENU_DATETIME", "1. Date & Time"), 
        langManager.getText("MENU_TEMP_TARGET", "2. Target Temp"), 
        langManager.getText("MENU_RELAY_STATE", "3. Relay State"), 
        langManager.getText("MENU_WIFI_MGMT", "4. Wi-Fi"), 
        langManager.getText("MENU_SYS_VERSION", "5. System Info"), 
        langManager.getText("MENU_LANGUAGE", "6. Language"), 
        langManager.getText("MENU_ENERGY_SAVING", "7. Energy Saving"), 
        langManager.getText("MENU_FACTORY_RESET", "8. Factory Reset")
    };

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
        GFX->drawString(items[i], x + btnWidth/2, y + btnHeight/2, 2);
    }
}

void AquariumUI::drawSettingsSubScreen(int sub) {
    if (sub == 0) {
        drawSettingsMenu();
    } else if (sub == 1) { // Date & Time
        // Consistent Enlarged Back Button
        drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
        
        // Title (Shifted down)
        GFX->setTextColor(COLOR_CYAN_GLOW);
        GFX->setTextDatum(TC_DATUM);
        String titleStr = String(langManager.getText("TITLE_DATETIME", "DATE & TIME")) + " [ TZ: " + cfg.timezone + " ]";
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
        GFX->drawString(combined, 160, 68, 4);
        
        // Date Format Toggle Button (Larger: Y: 104, Height: 44)
        GFX->fillRoundRect(10, 104, 300, 44, 8, COLOR_CARD_BORDER);
        GFX->drawRoundRect(10, 104, 300, 44, 8, COLOR_CYAN_GLOW);
        GFX->setTextColor(TFT_WHITE);
        
        String fmtLabel = String(langManager.getText("LABEL_DATE_FMT", "DATE FORMAT:")) + " ";
        const AquariumConfig& acfg = aquarium.getConfig();
        if (acfg.dateFormat == DATE_FORMAT_DDMMYYYY) fmtLabel += "GG/MM/AAAA";
        else if (acfg.dateFormat == DATE_FORMAT_MMDDYYYY) fmtLabel += "MM/GG/AAAA";
        else fmtLabel += "AAAA/MM/GG";
        GFX->drawString(fmtLabel, 160, 126, 2);
        
        // NTP Sync Button (Larger: Y: 164, Height: 52)
        GFX->fillRoundRect(10, 164, 145, 52, 8, COLOR_CARD_BORDER);
        GFX->drawRoundRect(10, 164, 145, 52, 8, COLOR_CYAN_GLOW);
        GFX->drawString("SYNC NTP", 82, 190, 2);
        
        // Timezone Buttons (Larger: Y: 164, Height: 52)
        GFX->fillRoundRect(165, 164, 65, 52, 8, COLOR_CARD_BORDER);
        GFX->drawRoundRect(165, 164, 65, 52, 8, COLOR_CYAN_GLOW);
        GFX->drawString("-1H TZ", 197, 190, 2);
        
        GFX->fillRoundRect(240, 164, 70, 52, 8, COLOR_CARD_BORDER);
        GFX->drawRoundRect(240, 164, 70, 52, 8, COLOR_CYAN_GLOW);
        GFX->drawString("+1H TZ", 275, 190, 2);
    } else {
        // Altre subscreen
        if (sub == 2) drawSubScreenTempTarget();
        if (sub == 3) drawSubScreenRelay();
        if (sub == 4) drawSubScreenWifi();
        if (sub == 5) drawSubScreenVersion();
        if (sub == 6) drawSubScreenLanguage();
        if (sub == 7) drawSubScreenEnergySaving();
        if (sub == 8) drawSubScreenFactoryReset();
    }
}

void AquariumUI::drawSubScreenTempTarget() {
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_EMERALD_GREEN, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("TITLE_TEMP_TARGET", "OPTIMAL MIN & MAX SETTING"), 190, 10, 2);

  const AquariumConfig& cfg = aquarium.getConfig();

  // Card 1: MINIMA OTTIMALE (X: 10, Y: 40, W: 300, H: 80)
  drawGlassCard(10, 40, 300, 80, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_MIN_TARGET", "OPTIMAL MIN:"), 18, 50, 2);

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
  GFX->drawString(langManager.getText("LABEL_MAX_TARGET", "OPTIMAL MAX:"), 18, 142, 2);

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
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("TITLE_RELAY_STATE", "LIGHT RELAY STATUS (GPIO 17)"), 190, 10, 2);

  bool lightOn = aquarium.isLightOn();
  bool inv = aquarium.isRelayInverted();
  uint16_t releCol = lightOn ? COLOR_GOLD_ACCENT : COLOR_TEXT_MUTED;

  // Relay Info Card (X: 10, Y: 40, W: 300, H: 70)
  drawGlassCard(10, 40, 300, 70, releCol);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_PIN_POLARITY", "PIN: GPIO 17 | POLARITY:"), 18, 46, 1);
  GFX->setTextColor(inv ? COLOR_CORAL_RED : COLOR_CYAN_GLOW, COLOR_CARD_BG);
  GFX->drawString(inv ? langManager.getText("LABEL_ACTIVE_LOW", "ACTIVE LOW (INVERTED)") : langManager.getText("LABEL_ACTIVE_HIGH", "ACTIVE HIGH (NORMAL)"), 165, 46, 1);

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(releCol, COLOR_CARD_BG);
  GFX->drawString(lightOn ? langManager.getText("LABEL_RELAY_ACTIVE", "RELAY STATUS: ACTIVE") : langManager.getText("LABEL_RELAY_INACTIVE", "RELAY STATUS: INACTIVE"), 160, 83, 2);

  // Button 1: Commuta Relè Adesso (X: 10, Y: 122, W: 300, H: 44)
  drawTouchButton(10, 122, 300, 44, lightOn ? langManager.getText("BTN_DEACTIVATE_RELAY", "DEACTIVATE RELAY NOW") : langManager.getText("BTN_ACTIVATE_RELAY", "ACTIVATE RELAY NOW"), COLOR_CARD_BG, releCol, releCol);

  // Button 2: Inverti Logica High/Low (X: 10, Y: 178, W: 300, H: 44)
  drawTouchButton(10, 178, 300, 44, inv ? langManager.getText("BTN_RESTORE_HIGH", "RESTORE ACTIVE HIGH LOGIC") : langManager.getText("BTN_INVERT_LOW", "INVERT LOGIC (ACTIVE LOW)"), COLOR_CARD_BORDER, COLOR_CYAN_GLOW);
}
void AquariumUI::drawWifiKeyboard() {
  // 1. Top Header: Consistent Enlarged Back Button + Selected SSID
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_NETWORK", "< NET"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  String headerStr = "SSID: " + m_wifiSelectedSSID;
  if (headerStr.length() > 18) headerStr = headerStr.substring(0, 18) + "..";
  GFX->drawString(headerStr.c_str(), 96, 10, 2);

  // 2. Password Display Box & Mask Toggle (Shifted down: Y: 38, Height: 30)
  drawGlassCard(6, 38, 256, 30, COLOR_CYAN_GLOW);

  GFX->setTextDatum(TL_DATUM);
  String displayPass = "";
  if (m_wifiHidePassword) {
    for (size_t i = 0; i < m_wifiTypedPassword.length(); i++) displayPass += "*";
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
  drawTouchButton(266, 38, 48, 30, m_wifiHidePassword ? langManager.getText("BTN_SHOW", "SHOW") : langManager.getText("BTN_HIDE", "HIDE"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

  // 3. Keypad Matrix (Shifted Y spacing: Y: 72, 108, 144, Height: 32)
  const char* row1_lower[10] = {"q","w","e","r","t","y","u","i","o","p"};
  const char* row1_upper[10] = {"Q","W","E","R","T","Y","U","I","O","P"};
  const char* row1_symb[10]  = {"1","2","3","4","5","6","7","8","9","0"};

  const char* row2_lower[9]  = {"a","s","d","f","g","h","j","k","l"};
  const char* row2_upper[9]  = {"A","S","D","F","G","H","J","K","L"};
  const char* row2_symb[9]   = {"!","@","#","$","%","&","*","-","+"};

  const char* row3_lower[7]  = {"z","x","c","v","b","n","m"};
  const char* row3_upper[7]  = {"Z","X","C","V","B","N","M"};
  const char* row3_symb[7]   = {"=",".","_",":","/",";","?"};

  int y1 = 72, y2 = 108, y3 = 144;

  // Row 1 (10 keys)
  for (int i = 0; i < 10; i++) {
    const char* k = (m_kbLayoutMode == 0) ? row1_lower[i] : (m_kbLayoutMode == 1) ? row1_upper[i] : row1_symb[i];
    drawTouchButton(4 + i * 31, y1, 29, 32, k, COLOR_CARD_BG, TFT_WHITE, COLOR_CARD_BORDER);
  }

  // Row 2 (9 keys)
  for (int i = 0; i < 9; i++) {
    const char* k = (m_kbLayoutMode == 0) ? row2_lower[i] : (m_kbLayoutMode == 1) ? row2_upper[i] : row2_symb[i];
    drawTouchButton(19 + i * 31, y2, 29, 32, k, COLOR_CARD_BG, TFT_WHITE, COLOR_CARD_BORDER);
  }

  // Row 3: Shift / Mode Key (Shift button enlarged)
  const char* shiftLabel = (m_kbLayoutMode == 0) ? "abc" : (m_kbLayoutMode == 1) ? "ABC" : "123";
  drawTouchButton(4, y3, 40, 32, shiftLabel, COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

  // Row 3 Middle (7 keys)
  for (int i = 0; i < 7; i++) {
    const char* k = (m_kbLayoutMode == 0) ? row3_lower[i] : (m_kbLayoutMode == 1) ? row3_upper[i] : row3_symb[i];
    drawTouchButton(48 + i * 30, y3, 28, 32, k, COLOR_CARD_BG, TFT_WHITE, COLOR_CARD_BORDER);
  }

  // Delete Backspace Button (Enlarged)
  drawTouchButton(262, y3, 54, 32, langManager.getText("BTN_DEL", "DEL"), COLOR_CARD_BG, COLOR_CORAL_RED, COLOR_CORAL_RED);

  // 4. Bottom Control Bar (Larger: Y: 184, Height: 44)
  drawTouchButton(4, 184, 65, 44, (m_kbLayoutMode == 2) ? "ABC" : "123", COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  drawTouchButton(73, 184, 105, 44, langManager.getText("BTN_SPACE", "SPACE"), COLOR_CARD_BG, TFT_WHITE, COLOR_CARD_BORDER);
  drawTouchButton(182, 184, 134, 44, langManager.getText("BTN_CONNECT", "CONNECT"), COLOR_EMERALD_GREEN, TFT_BLACK, COLOR_EMERALD_GREEN);
}


void AquariumUI::drawSubScreenWifi() {
  if (m_wifiShowKeyboard) {
    drawWifiKeyboard();
    return;
  }

  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_EMERALD_GREEN, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("MENU_WIFI_MGMT", "WI-FI (STATUS/NET)"), 190, 10, 2);

  bool conn = aquarium.isWifiConnected();
  uint16_t statusCol = conn ? COLOR_EMERALD_GREEN : COLOR_CORAL_RED;

  // WiFi Status Card (Shifted down Y: 38, Height: 44)
  drawGlassCard(6, 38, 308, 44, statusCol);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_STATUS", "STATUS:"), 14, 42, 1);
  GFX->setTextColor(statusCol, COLOR_CARD_BG);
  GFX->drawString(conn ? langManager.getText("MSG_CONNECTED", "CONNECTED") : langManager.getText("MSG_DISCONNECTED", "DISCONNECTED"), 58, 42, 1);

  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString("IP:", 160, 42, 1);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString(aquarium.getWifiIP().c_str(), 180, 42, 1);

  // Scan Button [ SCANSIONE RETI WI-FI ] (Shifted down Y: 90, Height: 38)
  drawTouchButton(6, 90, 308, 38, langManager.getText("BTN_SCAN", "SCAN WI-FI NETWORKS"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);

  // Scanned Networks List (Shifted down Y: 136..234)
  if (m_pendingWifiScan || aquarium.isWifiScanning()) {
    drawGlassCard(6, 136, 308, 94, COLOR_CYAN_GLOW);
    GFX->setTextDatum(MC_DATUM);
    GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_SCANNING", "PLEASE WAIT... Scanning networks"), 160, 176, 2);
    GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_SEARCHING_SIGNALS", "Searching Wi-Fi signals..."), 160, 204, 1);

    if (m_pendingWifiScan) {
      m_pendingWifiScan = false;
    }
    return;
  }


  int netCount = aquarium.getWifiNetworkCount();
  if (netCount <= 0) {
    drawGlassCard(6, 136, 308, 94, COLOR_CARD_BORDER);
    GFX->setTextDatum(MC_DATUM);
    GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    GFX->drawString(aquarium.getWifiScanStatus().c_str(), 160, 176, 2);
    GFX->drawString(langManager.getText("MSG_PRESS_SCAN", "Press [SCAN WI-FI NETWORKS] above"), 160, 204, 1);
    return;
  }

  int totalPages = (netCount + 2) / 3;
  if (m_wifiListPage < 0) m_wifiListPage = 0;
  if (m_wifiListPage >= totalPages) m_wifiListPage = totalPages - 1;

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
      if (ssidDisplay.length() > 16) ssidDisplay = ssidDisplay.substring(0, 16) + "..";
      GFX->drawString(ssidDisplay.c_str(), 14, cardY + 8, 2);

      GFX->setTextDatum(TR_DATUM);
      GFX->setTextColor(net.open ? COLOR_EMERALD_GREEN : COLOR_CYAN_GLOW, COLOR_CARD_BG);
      String secInfo = net.open ? langManager.getText("MSG_OPEN_NET", "OPEN") : langManager.getText("MSG_PROT_NET", "SECURED");
      secInfo += " (" + String(net.rssi) + "dB)";
      GFX->drawString(secInfo.c_str(), cardWidth - 6, cardY + 8, 2);
    }
  }

  // Right Side Pagination Buttons (^ and v) (Shifted down Y: 136, 184, Height: 44)
  if (totalPages > 1) {
    uint16_t upCol = (m_wifiListPage > 0) ? COLOR_CYAN_GLOW : COLOR_CARD_BORDER;
    uint16_t upTextCol = (m_wifiListPage > 0) ? COLOR_GOLD_ACCENT : COLOR_TEXT_MUTED;
    drawTouchButton(266, 136, 48, 44, "^", COLOR_CARD_BG, upTextCol, upCol);

    uint16_t downCol = (m_wifiListPage < totalPages - 1) ? COLOR_CYAN_GLOW : COLOR_CARD_BORDER;
    uint16_t downTextCol = (m_wifiListPage < totalPages - 1) ? COLOR_GOLD_ACCENT : COLOR_TEXT_MUTED;
    drawTouchButton(266, 184, 48, 44, "v", COLOR_CARD_BG, downTextCol, downCol);
  }
}

void AquariumUI::drawSubScreenLanguage() {
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("MENU_LANGUAGE", "SELECT INTERFACE LANGUAGE"), 190, 10, 2);

  int langCount = langManager.getAvailableLanguageCount();
  String currentFile = langManager.getActiveLanguageFile();

  // Enlarged Language Cards (Height: 38, Spacing: 44, Y starts at 40)
  for (int i = 0; i < 4; i++) {
    int cardY = 40 + i * 44;
    if (i < langCount) {
      LangItem item = langManager.getAvailableLanguage(i);
      bool isSelected = (currentFile == item.filename || currentFile.endsWith(item.filename));
      uint16_t borderCol = isSelected ? COLOR_GOLD_ACCENT : COLOR_CYAN_GLOW;
      drawGlassCard(6, cardY, 308, 38, borderCol);

      GFX->setTextDatum(TL_DATUM);
      GFX->setTextColor(isSelected ? COLOR_GOLD_ACCENT : TFT_WHITE, COLOR_CARD_BG);
      String label = String(i + 1) + ". " + item.name;
      if (isSelected) label += "  [" + String(langManager.getText("LABEL_ACTIVE_LANG", "ACTIVE")) + "]";
      GFX->drawString(label.c_str(), 16, cardY + 11, 2);

      if (isSelected) {
        GFX->setTextDatum(TR_DATUM);
        GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
        GFX->drawString("v", 300, cardY + 11, 2);
      }
    }
  }
}




void AquariumUI::drawSubScreenVersion() {
  // Consistent Enlarged Back Button
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("TITLE_SYS_VERSION", "SYSTEM INFORMATION"), 190, 10, 2);

  // Enlarged Version Info Card (X: 10, Y: 40, W: 300, H: 172)
  drawGlassCard(10, 40, 300, 172, COLOR_CARD_BORDER);

  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_SYS_NAME", "FIRMWARE:"), 20, 56, 2);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString("AQUARIUM MASTER CONTROLLER", 105, 56, 2);

  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_OS_VER", "VERSION:"), 20, 90, 2);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_CARD_BG);
  GFX->drawString(AQUARIUM_OS_VERSION, 105, 90, 4);

  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_FRAMEWORK", "FRAMEWORK:"), 20, 132, 2);
  GFX->setTextColor(COLOR_EMERALD_GREEN, COLOR_CARD_BG);
  GFX->drawString("ESP32 Arduino / PlatformIO", 105, 132, 2);

  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_HARDWARE", "HARDWARE:"), 20, 164, 2);
  GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
  GFX->drawString("ESP32-DEV 240MHz (4MB Flash)", 105, 164, 2);
}

void AquariumUI::drawSubScreenEnergySaving() {
  // --- Screensaver time table (seconds): 0=MAI, 30, 60, 120, 180, 240, 300, 360...3600 ---
  static const uint16_t SS_TIMES[]   = { 0, 30, 60, 120, 180, 240, 300, 360, 420, 480, 540, 600, 900, 1200, 1800, 2700, 3600 };
  static const int      SS_COUNT     = sizeof(SS_TIMES) / sizeof(SS_TIMES[0]);

  // Sync idx with current config value
  uint16_t curVal = aquarium.getConfig().screensaverTime;
  for (int i = 0; i < SS_COUNT; i++) {
    if (SS_TIMES[i] == curVal) { m_screensaverIdx = i; break; }
  }

  // --- Header ---
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("TITLE_ENERGY_SAVING", "ENERGY SAVING"), 195, 10, 2);

  // --- Glass Card (Enlarged: Y: 40, Height 120) ---
  drawGlassCard(10, 40, 300, 120, COLOR_CYAN_GLOW);

  // Label
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_SCREENSAVER", "SCREEN OFF AFTER:"), 22, 52, 2);

  // Current value label
  char valBuf[20];
  if (SS_TIMES[m_screensaverIdx] == 0) {
    snprintf(valBuf, sizeof(valBuf), "%s", langManager.getText("LABEL_NEVER", "NEVER"));
  } else if (SS_TIMES[m_screensaverIdx] < 60) {
    snprintf(valBuf, sizeof(valBuf), "%d%s", SS_TIMES[m_screensaverIdx], langManager.getText("LABEL_SEC_SUFFIX", " sec"));
  } else {
    snprintf(valBuf, sizeof(valBuf), "%d%s", SS_TIMES[m_screensaverIdx] / 60, langManager.getText("LABEL_MIN_SUFFIX", " min"));
  }

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString(valBuf, 160, 92, 4);

  // --- Prev / Next buttons (Enlarged: Height: 44, Width: 70, Y: 110) ---
  drawTouchButton(16, 110, 70, 44, "< ", COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CARD_BORDER);
  drawTouchButton(234, 110, 70, 44, " >", COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CARD_BORDER);

  // --- Info text (Spaced out) ---
  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("LABEL_SCREENSAVER_LINE1", "LCD backlight off = energy saving"), 160, 180, 1);
  GFX->drawString(langManager.getText("LABEL_SCREENSAVER_LINE2", "Touch screen to wake"), 160, 200, 1);
}

void AquariumUI::drawSubScreenFactoryReset() {
  drawTouchButton(6, 2, 84, 32, langManager.getText("BTN_BACK", "< HOME"), COLOR_CARD_BG, COLOR_GOLD_ACCENT, COLOR_CYAN_GLOW);
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(langManager.getText("TITLE_FACTORY_RESET", "FACTORY_RESET"), 190, 10, 2);

  // Card Background (Enlarged: X: 10, Y: 40, W: 300, H: 150)
  drawGlassCard(10, 40, 300, 150, COLOR_CORAL_RED);

  GFX->setTextDatum(MC_DATUM);

  if (m_resetStep == 0) {
    GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_RESET_CONFIRM1", "Do you want to restore factory settings?"), 160, 76, 2);
  } else {
    GFX->setTextColor(COLOR_CORAL_RED, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_RESET_CONFIRM2", "ALL SETTINGS WILL BE DELETED!"), 160, 76, 2);
  }

  // Draw Yes/No buttons (Enlarged: Height: 50, Y: 128)
  drawTouchButton(30, 128, 110, 50, langManager.getText("BTN_YES", "YES"), COLOR_CARD_BG, COLOR_EMERALD_GREEN, COLOR_EMERALD_GREEN);
  drawTouchButton(180, 128, 110, 50, langManager.getText("BTN_NO", "NO"), COLOR_CARD_BG, COLOR_CORAL_RED, COLOR_CORAL_RED);
}


// ------------------------------------------------------------------------------
// WIDGET HELPERS
// ------------------------------------------------------------------------------
void AquariumUI::drawCircularTempGauge(int cx, int cy, int r, float temp, float minT, float maxT) {
  // Outer Ambient Ring
  GFX->drawCircle(cx, cy, r, COLOR_CARD_BORDER);
  GFX->drawCircle(cx, cy, r - 1, COLOR_CARD_BORDER);

  // Radial Ticks (Angle range: -210 deg to +30 deg)
  float norm = (temp - 20.0f) / (32.0f - 20.0f);
  norm = constrain(norm, 0.0f, 1.0f);

  uint16_t gaugeColor = COLOR_EMERALD_GREEN;
  if (temp < minT) gaugeColor = COLOR_CYAN_GLOW;
  if (temp > maxT) gaugeColor = COLOR_CORAL_RED;

  float endAngle = -210.0f + norm * 240.0f;
  for (float a = -210.0f; a <= 30.0f; a += 10.0f) {
    float rad = a * 0.0174533f;
    int x1 = cx + (int)(cosf(rad) * (r - 9));
    int y1 = cy + (int)(sinf(rad) * (r - 9));
    int x2 = cx + (int)(cosf(rad) * (r - 2));
    int y2 = cy + (int)(sinf(rad) * (r - 2));

    uint16_t tickCol = (a <= endAngle) ? gaugeColor : COLOR_CARD_BORDER;
    GFX->drawLine(x1, y1, x2, y2, tickCol);
  }

  // Inner Center
  GFX->fillCircle(cx, cy, r - 15, COLOR_BG_OCEAN);
  GFX->drawCircle(cx, cy, r - 15, COLOR_CARD_BORDER);

  // Digital Temperature Value
  char tempStr[10];
  snprintf(tempStr, sizeof(tempStr), "%.1f", temp);
  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(TFT_WHITE, COLOR_BG_OCEAN);
  GFX->drawString(tempStr, cx - 4, cy - 6, 4);

  GFX->setTextColor(gaugeColor, COLOR_BG_OCEAN);
  GFX->drawString("°C", cx + 32, cy - 10, 2);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_BG_OCEAN);
  GFX->drawString("ACQUA", cx, cy + 18, 1);
}

void AquariumUI::drawGlassCard(int x, int y, int w, int h, uint16_t borderColor, uint16_t bgCol) {
  GFX->fillRoundRect(x, y, w, h, 10, bgCol);
  GFX->drawRoundRect(x, y, w, h, 10, borderColor);
}

void AquariumUI::drawTouchButton(int x, int y, int w, int h, const char* label, uint16_t bgCol, uint16_t textCol, uint16_t borderCol) {
  GFX->fillRoundRect(x, y, w, h, 8, bgCol);
  GFX->drawRoundRect(x, y, w, h, 8, borderCol);
  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(textCol, bgCol);
  GFX->drawString(label, x + w / 2, y + h / 2, 2);
}

// ------------------------------------------------------------------------------
// TOUCH EVENT ROUTING
// ------------------------------------------------------------------------------
void AquariumUI::handleTouch(int touchX, int touchY) {
  uint32_t now = millis();
  if (now - m_lastTouchTime < UI_TOUCH_DEBOUNCE_MS) return; // Parameterized touch debounce
  m_lastTouchTime = now;

  // 1. Navigation Bar Touch (Y >= UI_NAVBAR_Y)
  // ONLY intercept if the Navbar is actually drawn (not hidden by a settings subscreen)
  if (touchY >= UI_NAVBAR_Y && !(m_activeTab == TAB_SETTINGS && m_settingsSubScreen > 0)) {
    if (m_activeTab == TAB_SETTINGS) {
        if (m_settingsSubScreen == 0) { // Main settings menu
              if (touchX <= 160) {
                  setTab(TAB_DASHBOARD);
              } else {
                  // >> -> vai alla pagina 2 (o torna alla 1)
                  m_settingsMenuPage = m_settingsMenuPage == 0 ? 1 : 0;
                  m_settingsNeedsRedraw = true;
                  drawNavBar(true);
              }
        }
        return; 
    }
    int tabIdx = touchX / 80;
    if (tabIdx >= 0 && tabIdx < 4) {
      setTab((UiTab)tabIdx);
    }
    return;
  }

  // 2. Active Tab Touches (Relative Y inside sprite or TFT)
  int yOff = (m_activeTab == TAB_SETTINGS) ? 0 : UI_SPRITE_Y_OFFSET;
  int sy = touchY - yOff;
  const AquariumConfig& cfg = aquarium.getConfig();



  switch (m_activeTab) {
    case TAB_DASHBOARD:
      // Touch Light Card (Middle Right: X: 180..310, sy: 60..112)
      if (touchX >= 180 && touchX <= 310 && sy >= 60 && sy <= 112) {
        aquarium.toggleLight();
      }
      break;



    case TAB_LIGHT:
      if (touchX >= 50 && touchX <= 270 && sy >= 12 && sy <= 98) {
        aquarium.toggleLight();
      }
      if (touchX >= 40 && touchX <= 280 && sy >= 108 && sy <= 158) {
        aquarium.setAutoSchedule(!aquarium.isAutoSchedule());
      }
      break;

    case TAB_SCHEDULE:
      if (touchX >= 210 && touchX <= 252 && sy >= 40 && sy <= 74) {
        uint8_t newH = (cfg.lightOnHour + 23) % 24;
        aquarium.setScheduleOn(newH, cfg.lightOnMin);
      }
      if (touchX >= 258 && touchX <= 300 && sy >= 40 && sy <= 74) {
        uint8_t newH = (cfg.lightOnHour + 1) % 24;
        aquarium.setScheduleOn(newH, cfg.lightOnMin);
      }
      if (touchX >= 210 && touchX <= 252 && sy >= 112 && sy <= 146) {
        uint8_t newH = (cfg.lightOffHour + 23) % 24;
        aquarium.setScheduleOff(newH, cfg.lightOffMin);
      }
      if (touchX >= 258 && touchX <= 300 && sy >= 112 && sy <= 146) {
        uint8_t newH = (cfg.lightOffHour + 1) % 24;
        aquarium.setScheduleOff(newH, cfg.lightOffMin);
      }
      break;

    case TAB_SETTINGS:
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
        // Sub 4: WiFi Management & Touch Keyboard Input
        if (m_wifiShowKeyboard) {
          // Top Left Back Button [ < NET ] (X: 0..94, sy: 0..36)
          if (touchX <= 94 && sy <= 36) {
            m_wifiShowKeyboard = false;
            return;
          }

          // Mask Toggle Button [ MOSTRA / NASCONDI ] (X: 260..318, sy: 38..68)
          if (touchX >= 260 && sy >= 38 && sy <= 68) {
            m_wifiHidePassword = !m_wifiHidePassword;
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
          return;
        }

        // WiFi Main Screen Touches (Updated for enlarged layout: Scan Y: 90..128)
        // Scan Button (sy: 90..128)
        if (sy >= 90 && sy <= 128) {
          m_pendingWifiScan = true;
          m_wifiListPage = 0;
          aquarium.startAsyncWifiScan();
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
          }
        }
      }



      break;
  }
}



// Fin
