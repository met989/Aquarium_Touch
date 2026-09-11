#include "../include/aquarium_ui.h"
#include "../include/language_manager.h"

#define GFX ((m_activeTab == TAB_SETTINGS) ? (TFT_eSPI *)m_tft : (TFT_eSPI *)m_sprite)

// ------------------------------------------------------------------------------
// TAB 2: TIMER & LIGHT SCHEDULE
// ------------------------------------------------------------------------------
void AquariumUI::drawScheduleTab() {
  const AquariumConfig &cfg = aquarium.getConfig();

  // Title
  GFX->setTextDatum(TC_DATUM);
  GFX->setTextColor(COLOR_CYAN_GLOW, COLOR_BG_OCEAN);
  GFX->drawString(
      langManager.getText("TITLE_TIMER_SCHEDULE", "DAILY TIMER SCHEDULE"), 160,
      6, 2);

  // ON Schedule Card (X: 10, Y: 24, W: 300, H: 65)
  drawGlassCard(10, 24, 300, 65, COLOR_GOLD_ACCENT);
  GFX->setTextDatum(TL_DATUM);
  GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_ON_TIME", "TURN ON"), 18, 30, 2);

  char onBuf[12];
  snprintf(onBuf, sizeof(onBuf), "%02d : %02d", cfg.lightOnHour,
           cfg.lightOnMin);
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
  GFX->drawString(langManager.getText("LABEL_OFF_TIME", "TURN OFF"), 18, 102,
                  2);

  char offBuf[12];
  snprintf(offBuf, sizeof(offBuf), "%02d : %02d", cfg.lightOffHour,
           cfg.lightOffMin);
  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(TFT_WHITE, COLOR_CARD_BG);
  GFX->drawString(offBuf, 150, 132, 4);

  // Touch Buttons for OFF Hour [-H] [+H]
  drawTouchButton(210, 112, 42, 34, "-H", COLOR_CARD_BORDER, TFT_WHITE);
  drawTouchButton(258, 112, 42, 34, "+H", COLOR_CARD_BORDER, TFT_WHITE);
}

void AquariumUI::handleScheduleTouch(int touchX, int touchY) {
  const AquariumConfig &cfg = aquarium.getConfig();
  int sy = touchY - UI_SPRITE_Y_OFFSET;
  
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
}
