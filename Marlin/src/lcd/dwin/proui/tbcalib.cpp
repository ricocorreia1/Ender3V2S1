/**
 * Atalho "Preparar calibracao" — veja tbcalib.h
 */

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(DWIN_LCD_PROUI)

#include "../../../MarlinCore.h"
#include "../../../module/temperature.h"
#include "../../marlinui.h"
#include "dwin.h"
#include "dwinui.h"
#include "dwin_popup.h"
#include "tbcalib.h"

#if HAS_HOTEND && HAS_HEATED_BED

#define CAL_STEP      5      // graus por clique do encoder
#define CAL_FASTSTEP 10      // graus por clique girando rapido
#define CAL_FAST_MS  120     // intervalo entre cliques para considerar "rapido"

enum : uint8_t { CAL_ROW_HOT = 0, CAL_ROW_BED = 1, CAL_ROW_GO = 2, CAL_ROW_EXIT = 3, CAL_N_ROWS = 4 };

// valores lembrados enquanto a impressora esta ligada
static int16_t cal_hot = PREHEAT_1_TEMP_HOTEND;
static int16_t cal_bed = PREHEAT_1_TEMP_BED;

static int16_t cal_hot_max, cal_bed_max;
static uint8_t cal_row;
static bool    cal_edit;
static millis_t cal_last;

static void cal_drawRow(uint8_t row) {
  const uint16_t y = 115 + row * 40;
  const bool selected = (row == cal_row);
  const uint16_t bg = selected ? (cal_edit ? COLOR_SELECT : COLOR_BG_BLUE)
                               : hmiData.colorPopupBg;
  // Limpa o fundo da linha (dentro da area do popup: x 15..257)
  dwinDrawRectangle(1, bg, 15, y, 257, y + 34);
  char s[24];
  if (row == CAL_ROW_HOT) {
    DWINUI::drawIcon(ICON_SetEndTemp, 22, y + 4);
    sprintf_P(s, PSTR("Bico:  %3i C"), int(cal_hot));
    DWINUI::drawString(font12x24, hmiData.colorPopupTxt, bg, 60, y + 6, s);
  }
  else if (row == CAL_ROW_BED) {
    DWINUI::drawIcon(ICON_SetBedTemp, 22, y + 4);
    sprintf_P(s, PSTR("Mesa:  %3i C"), int(cal_bed));
    DWINUI::drawString(font12x24, hmiData.colorPopupTxt, bg, 60, y + 6, s);
  }
  else {
    // Iniciar / Sair — texto centralizado, sem icone
    const char *label = (row == CAL_ROW_GO) ? "Iniciar" : "Sair";
    DWINUI::drawCenteredString(true, font12x24, hmiData.colorPopupTxt, bg, 14, 258, y + 6, label);
  }
}

static void cal_drawAll() {
  // Ricardo: desenha popup do zero — NAO usa dwinDrawPopup para nao ter o icone
  // grande no topo nem o texto no meio (y=210) que estava sobreposto as linhas.
  DWINUI::clearMainArea();
  drawPopupBkgd();
  DWINUI::drawCenteredString(font12x24, hmiData.colorPopupTxt, 75, F("Preparar calibracao"));
  for (uint8_t i = 0; i < CAL_N_ROWS; ++i) cal_drawRow(i);
  DWINUI::drawCenteredString(false, font8x16, hmiData.colorPopupTxt, hmiData.colorPopupBg,
                             14, 258, 305, "gire=navegar  clique=OK");
  dwinUpdateLCD();
}

static void cal_change(const bool ccw) {
  if (cal_edit) {
    const millis_t now = millis();
    const int16_t step = (now - cal_last < CAL_FAST_MS) ? CAL_FASTSTEP : CAL_STEP;
    cal_last = now;
    int16_t &v    = (cal_row == CAL_ROW_HOT) ? cal_hot : cal_bed;
    const int16_t vmax = (cal_row == CAL_ROW_HOT) ? cal_hot_max : cal_bed_max;
    v += ccw ? -step : step;
    v -= v % CAL_STEP;
    LIMIT(v, 0, vmax);
    cal_drawRow(cal_row);
  }
  else {
    const uint8_t old = cal_row;
    if (ccw) cal_row = cal_row ? cal_row - 1 : CAL_N_ROWS - 1;
    else     cal_row = (cal_row + 1 < CAL_N_ROWS) ? cal_row + 1 : 0;
    cal_drawRow(old);
    cal_drawRow(cal_row);
  }
  dwinUpdateLCD();
}

static void cal_click() {
  if (cal_edit) {
    cal_edit = false;
    cal_drawRow(cal_row);
    dwinUpdateLCD();
    marlin.wait_start();      // popup permanece; re-arma o wait para o proximo click
    return;
  }
  if (cal_row == CAL_ROW_GO) {
    thermalManager.setTargetHotend(cal_hot, 0);
    thermalManager.setTargetBed(cal_bed);
    ui.status_printf(0, F("Calib: bico %i / mesa %i C"), int(cal_hot), int(cal_bed));
    hmiReturnScreen();        // sai do popup — nao precisa re-armar
    return;
  }
  if (cal_row == CAL_ROW_EXIT) {
    LCD_MESSAGE_F("Preparo cancelado");
    hmiReturnScreen();        // sai sem alterar temperaturas
    return;
  }
  cal_edit = true;
  cal_last = 0;
  cal_drawRow(cal_row);
  dwinUpdateLCD();
  marlin.wait_start();        // popup permanece; re-arma o wait para o proximo click
}

void tbPrepareCalib() {
  cal_hot_max = thermalManager.hotend_max_target(0);
  cal_bed_max = BED_MAX_TARGET;
  LIMIT(cal_hot, 0, cal_hot_max);
  LIMIT(cal_bed, 0, cal_bed_max);
  cal_row  = 0;
  cal_edit = false;
  cal_last = 0;
  gotoPopup(cal_drawAll, cal_click, cal_change);
}

#endif // HAS_HOTEND && HAS_HEATED_BED

#endif // DWIN_LCD_PROUI
