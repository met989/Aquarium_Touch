#include "../include/aquarium_ui.h"
#include "../include/language_manager.h"

#define GFX ((m_activeTab == TAB_SETTINGS) ? (TFT_eSPI *)m_tft : (TFT_eSPI *)m_sprite)

// ------------------------------------------------------------------------------
// TAB 1: LIGHT CONTROL
// ------------------------------------------------------------------------------
void AquariumUI::drawLightTab() {
  bool lightOn = aquarium.isLightOn();
  uint16_t cardBorder = lightOn ? COLOR_GOLD_ACCENT : COLOR_CARD_BORDER;
  uint16_t statusCol = lightOn ? COLOR_GOLD_ACCENT : COLOR_TEXT_MUTED;
  const char *statusTxt =
      lightOn ? langManager.getText("MSG_LIGHT_ON", "LIGHT ON (ON)")
              : langManager.getText("MSG_LIGHT_OFF", "LIGHT OFF (OFF)");

  // Main Light Status Card (X: 10, Y: 8, W: 300, H: 86)
  drawGlassCard(10, 8, 300, 86, cardBorder);

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(statusCol, COLOR_CARD_BG);
  GFX->drawString(statusTxt, 160, 32, 4);

  // Big Touch Toggle Button inside Card
  drawTouchButton(50, 52, 220, 34, lightOn ? langManager.getText("LABEL_TURN_OFF_LIGHT", "TURN OFF LIGHT") : langManager.getText("LABEL_TURN_ON_LIGHT", "TURN ON LIGHT"), COLOR_CARD_BG, statusCol, cardBorder);

  // Mode Selection Card (X: 10, Y: 102, W: 300, H: 62)
  bool autoMode = aquarium.isAutoSchedule();
  drawGlassCard(10, 102, 300, 62,
                autoMode ? COLOR_EMERALD_GREEN : COLOR_GOLD_ACCENT);

  GFX->setTextDatum(MC_DATUM);
  GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
  GFX->drawString(langManager.getText("LABEL_MODE", "MODE:"), 70, 133, 2);

  drawTouchButton(140, 114, 160, 38,
                  autoMode ? langManager.getText("MSG_AUTO", "AUTO (TIMER)")
                           : langManager.getText("MSG_MANUAL", "MANUAL"),
                  COLOR_CARD_BG,
                  autoMode ? COLOR_EMERALD_GREEN : COLOR_GOLD_ACCENT);
}

void AquariumUI::handleLightTouch(int touchX, int touchY) {
  int sy = touchY - UI_SPRITE_Y_OFFSET;
  if (touchX >= 50 && touchX <= 270 && sy >= 12 && sy <= 98) {
    aquarium.toggleLight();
  }
  if (touchX >= 40 && touchX <= 280 && sy >= 108 && sy <= 158) {
    aquarium.setAutoSchedule(!aquarium.isAutoSchedule());
  }
}
