/**
 * Atalhos "Aquecer bico" / "Aquecer mesa" — veja tbheat.h
 */

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(DWIN_LCD_PROUI)

#include "../../../MarlinCore.h"
#include "../../../module/temperature.h"
#include "../../marlinui.h"
#include "dwin.h"
#include "dwinui.h"
#include "dwin_popup.h"
#include "tbheat.h"

#define TH_STEP      5      // graus por clique do encoder
#define TH_FASTSTEP 10      // graus por clique girando rapido
#define TH_FAST_MS  120     // intervalo entre cliques considerado "rapido"

static bool     th_bed;                 // false = bico, true = mesa
static int16_t  th_val, th_max;
static uint8_t  th_icon;
static millis_t th_last;
#if HAS_HOTEND
  static int16_t th_lastHotend = PREHEAT_1_TEMP_HOTEND;   // lembra o ultimo valor usado
#endif
#if HAS_HEATED_BED
  static int16_t th_lastBed = PREHEAT_1_TEMP_BED;
#endif

static void th_drawValue() {
  char s[12];
  if (th_val > 0) sprintf_P(s, PSTR("  %3i C  "), th_val);
  else            strcpy_P(s, PSTR("Desligar"));
  DWINUI::drawCenteredString(true, font20x40, hmiData.colorPopupTxt, hmiData.colorPopupBg, 14, 258, 232, s);
}

static void th_draw() {
  dwinDrawPopup(th_icon, th_bed ? F("Aquecer mesa") : F("Aquecer bico"), F(""));
  th_drawValue();
  DWINUI::drawCenteredString(false, font8x16, hmiData.colorPopupTxt, hmiData.colorPopupBg, 14, 258, 290, "gire = ajusta   clique = aplica");
  dwinUpdateLCD();
}

static void th_change(const bool ccw) {
  const millis_t now = millis();
  const int16_t step = (now - th_last < TH_FAST_MS) ? TH_FASTSTEP : TH_STEP;
  th_last = now;
  th_val += ccw ? -step : step;
  th_val -= th_val % TH_STEP;             // mantem multiplo de 5
  LIMIT(th_val, 0, th_max);
  th_drawValue();
}

static void th_click() {
  #if HAS_HEATED_BED
    if (th_bed) {
      thermalManager.setTargetBed(th_val);
      if (th_val > 0) th_lastBed = th_val;
      if (th_val > 0) ui.status_printf(0, F("Mesa: aquecendo para %i C"), int(th_val));
      else            LCD_MESSAGE_F("Mesa desligada");
    }
  #endif
  #if HAS_HOTEND
    if (!th_bed) {
      thermalManager.setTargetHotend(th_val, 0);
      if (th_val > 0) th_lastHotend = th_val;
      if (th_val > 0) ui.status_printf(0, F("Bico: aquecendo para %i C"), int(th_val));
      else            LCD_MESSAGE_F("Bico desligado");
    }
  #endif
  hmiReturnScreen();
}

#if HAS_HOTEND
  void tbHeatHotend() {
    th_bed  = false;
    th_icon = ICON_SetEndTemp;
    th_max  = thermalManager.hotend_max_target(0);
    const int16_t tgt = thermalManager.degTargetHotend(0);
    th_val  = tgt > 0 ? tgt : th_lastHotend;
    th_last = 0;
    gotoPopup(th_draw, th_click, th_change);
  }
#endif

#if HAS_HEATED_BED
  void tbHeatBed() {
    th_bed  = true;
    th_icon = ICON_SetBedTemp;
    th_max  = BED_MAX_TARGET;
    const int16_t tgt = thermalManager.degTargetBed();
    th_val  = tgt > 0 ? tgt : th_lastBed;
    th_last = 0;
    gotoPopup(th_draw, th_click, th_change);
  }
#endif

#endif // DWIN_LCD_PROUI
