#include "../include/aquarium_ui.h"
#include "../include/language_manager.h"
#include <math.h>

#define GFX ((m_activeTab == TAB_SETTINGS) ? (TFT_eSPI *)m_tft : (TFT_eSPI *)m_sprite)

// ------------------------------------------------------------------------------
// TAB 0: DASHBOARD
// ------------------------------------------------------------------------------
void AquariumUI::drawDashboardTab() {
  // Left: Circular Temperature Gauge (Center X: 90, Y: 86 inside sprite)
  drawCircularTempGauge(90, 86, 52, m_animatedTemp,
                        aquarium.getConfig().targetTempMin,
                        aquarium.getConfig().targetTempMax);

  // Status Badge below gauge
  TempStatus status = aquarium.getTempStatus();
  uint16_t statusCol = COLOR_EMERALD_GREEN;
  const char *statusTxt = langManager.getText("MSG_OPTIMAL", "OPTIMAL");
  if (status == TEMP_TOO_COLD) {
    statusCol = COLOR_CYAN_GLOW;
    statusTxt = langManager.getText("MSG_COLD", "COLD");
  }
  if (status == TEMP_TOO_HOT) {
    statusCol = COLOR_CORAL_RED;
    statusTxt = langManager.getText("MSG_HOT", "HOT");
  }

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
  GFX->drawString(langManager.getText("TITLE_LIGHT_CONTROL", "LIGHT"), 240, 64, 1);

  GFX->setTextDatum(MC_DATUM);
  if (lightOn) {
    int animRay = (int)(sinf(m_wavePhase * 2.0f) * 2.0f);
    GFX->fillCircle(196, 88, 8 + animRay, COLOR_GOLD_ACCENT);
    GFX->setTextColor(COLOR_GOLD_ACCENT, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_LIGHT_ON", "ON"), 258, 90, 2);
  } else {
    GFX->drawCircle(196, 88, 8, COLOR_MOON_BLUE);
    GFX->fillCircle(194, 88, 6, COLOR_CARD_BG);
    GFX->setTextColor(COLOR_TEXT_MUTED, COLOR_CARD_BG);
    GFX->drawString(langManager.getText("MSG_LIGHT_OFF", "OFF"), 258, 90, 2);
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

void AquariumUI::handleDashboardTouch(int touchX, int touchY) {
  int sy = touchY - UI_SPRITE_Y_OFFSET;
  // Touch Light Card (Middle Right: X: 180..310, sy: 60..112)
  if (touchX >= 180 && touchX <= 310 && sy >= 60 && sy <= 112) {
    aquarium.toggleLight();
  }
}
