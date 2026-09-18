/**
 * Editor interativo de malha (Manual Mesh) para o Pro UI
 * Veja meshedit.h
 */

#include "../../../inc/MarlinConfigPre.h"

#if ALL(DWIN_LCD_PROUI, MESH_BED_LEVELING)

#include "../../../MarlinCore.h"
#include "../../../core/types.h"
#include "../../../module/motion.h"
#include "../../../module/planner.h"
#include "../../../gcode/gcode.h"
#include "../../../feature/bedlevel/bedlevel.h"
#include "../../marlinui.h"
#include "dwin.h"
#include "dwinui.h"
#include "dwin_popup.h"
#include "meshedit.h"

// ---- Geometria da grade (mesma do visualizador original) --------------------
#define ME_MARGIN 25                                   // margem lateral
#define ME_RMIN   5                                    // raio minimo do ponto
#define ME_ZMIN  -20                                   // raio minimo em z=-0,20
#define ME_ZMAX   20                                   // raio maximo em z= 0,20
#define ME_WIDTH  (DWIN_WIDTH - 2 * ME_MARGIN)
#define ME_BTN_Y  310                                  // linha dos botoes
#define ME_STEP   0.01f                                // passo do Z ao girar
#define ME_ZLIM   2.0f                                 // limite |Z| do ponto
#ifndef Z_CLEARANCE_BETWEEN_PROBES
  #define Z_CLEARANCE_BETWEEN_PROBES 5
#endif

#if ENABLED(TJC_DISPLAY)
  #define ME_FONT font8x16
#else
  #define ME_FONT font6x12
#endif

static uint8_t me_nx, me_ny;        // tamanho da malha em uso
static uint8_t me_rmax;             // raio maximo do ponto
static uint8_t me_cur;              // cursor: 0..N-1 pontos, N = Salvar, N+1 = Sair
static bool    me_editing;          // true = girar ajusta o Z do ponto
static bool    me_busy;             // movendo o bico: ignora o botao
static bool    me_moved;            // ja moveu o bico ate algum ponto
static float   me_z;                // valor em edicao
static bool    me_levelWas;         // estado do nivelamento ao entrar
static char    me_buf[8];

#define ME_NPTS  (me_nx * me_ny)
#define ME_SAVE  (ME_NPTS)
#define ME_EXIT  (ME_NPTS + 1)
#define ME_ITEMS (ME_NPTS + 2)

static inline uint16_t me_px(const uint8_t x) { return ME_MARGIN + x * ME_WIDTH / (me_nx - 1); }
static inline uint16_t me_py(const uint8_t y) { return 30 + DWIN_WIDTH - ME_MARGIN - y * ME_WIDTH / (me_ny - 1); }
static inline uint8_t  me_r(const int16_t v)  { return (v - ME_ZMIN) * (me_rmax - ME_RMIN) / (ME_ZMAX - ME_ZMIN) + ME_RMIN; }
static inline uint8_t  me_cx() { return me_cur % me_nx; }
static inline uint8_t  me_cy() { return me_cur / me_nx; }

// ---- Desenho -----------------------------------------------------------------

// Refaz os trechos das linhas da grade que passam pelo ponto (x,y), num raio d
static void me_drawGridAt(const uint8_t x, const uint8_t y, const uint16_t d) {
  const uint16_t cx = me_px(x), cy = me_py(y);
  // na tela, y da malha cresce para CIMA (py diminui)
  if (y < me_ny - 1) dwinDrawVLine(hmiData.colorSplitLine, cx, cy - d, d);      // para cima
  if (y > 0)         dwinDrawVLine(hmiData.colorSplitLine, cx, cy, d + 1);      // para baixo
  if (x > 0)         dwinDrawHLine(hmiData.colorSplitLine, cx - d, cy, d);      // esquerda
  if (x < me_nx - 1) dwinDrawHLine(hmiData.colorSplitLine, cx, cy, d + 1);      // direita
}

// Desenha um ponto (bolinha colorida + valor); clear = apaga o anterior antes
static void me_drawPoint(const uint8_t x, const uint8_t y, const float z, const bool clear=true) {
  const uint16_t cx = me_px(x), cy = me_py(y);
  const uint8_t  fs = DWINUI::fontWidth(ME_FONT);
  if (clear) {
    dwinDrawRectangle(1, hmiData.colorBackground, cx - me_rmax - 1, cy - me_rmax - 1, cx + me_rmax + 1, cy + me_rmax + 1);
    me_drawGridAt(x, y, me_rmax + 1);
  }
  if (isnan(z)) return;
  const int16_t v = int16_t(LROUND(z * 100));
  const uint16_t color = DWINUI::rainbowInt(v, ME_ZMIN, ME_ZMAX);
  DWINUI::drawFillCircle(color, cx, cy, me_r(_MAX(_MIN(v, ME_ZMAX), ME_ZMIN)));
  if (v == 0) dwinDrawString(false, ME_FONT, DWINUI::textColor, DWINUI::backColor, cx - 2 * fs - 1, cy - fs, "0.00");
  else        DWINUI::drawSignedFloat(ME_FONT, 1, 2, cx - 3 * fs, cy - fs, z);
}

// Moldura do cursor em volta de um ponto (on = desenha, off = apaga)
static void me_drawPointFrame(const uint8_t x, const uint8_t y, const bool on) {
  const uint16_t cx = me_px(x), cy = me_py(y), d = me_rmax + 3;
  // Ricardo: em edicao usa AMARELO SOLIDO — deixa obvio que girar altera o Z.
  // Fora de edicao, moldura branca discreta indica so posicao do cursor.
  const uint16_t c = on ? (me_editing ? COLOR_YELLOW : hmiData.colorText) : hmiData.colorBackground;
  dwinDrawRectangle(0, c, cx - d, cy - d, cx + d, cy + d);
  dwinDrawRectangle(0, c, cx - d - 1, cy - d - 1, cx + d + 1, cy + d + 1);
  // Em edicao, terceira linha externa reforca o alerta visual
  if (on && me_editing)
    dwinDrawRectangle(0, c, cx - d - 2, cy - d - 2, cx + d + 2, cy + d + 2);
  if (!on) me_drawGridAt(x, y, d + 2);   // recompoe as linhas que a moldura cobria
}

// Moldura dos botoes (0 = Salvar, 1 = Sair)
static void me_drawBtnFrame(const uint8_t idx, const bool on) {
  const uint16_t c = on ? hmiData.colorHighlight : hmiData.colorBackground;
  const uint16_t x1 = idx ? 145 : 25, x2 = idx ? 246 : 126;
  dwinDrawRectangle(0, c, x1, ME_BTN_Y - 1, x2, ME_BTN_Y + 38);
  dwinDrawRectangle(0, c, x1 - 1, ME_BTN_Y - 2, x2 + 1, ME_BTN_Y + 39);
}

static void me_drawCursor(const bool on) {
  if (me_cur < ME_NPTS)       me_drawPointFrame(me_cx(), me_cy(), on);
  else if (me_cur == ME_SAVE) me_drawBtnFrame(0, on);
  else                        me_drawBtnFrame(1, on);
}

static void me_status() {
  if (me_editing)
    ui.status_printf(0, F("* EDITANDO * Z=%s  gire ajusta, clique grava"), dtostrf(me_z, 1, 2, me_buf));
  else if (me_cur < ME_NPTS)
    ui.status_printf(0, F("Ponto %i,%i  Z=%s  clique edita"), int(me_cx()), int(me_cy()), dtostrf(bedlevel.z_values[me_cx()][me_cy()], 1, 2, me_buf));
  else if (me_cur == ME_SAVE)
    LCD_MESSAGE_F("Salvar a malha na EEPROM");
  else
    LCD_MESSAGE_F("Sair do editor de malha");
}

// Tela completa (chamada pelo sistema de popup, inclusive em redesenhos)
static void me_drawAll() {
  title.draw(F("Editar malha"));
  DWINUI::clearMainArea();
  dwinDrawRectangle(0, hmiData.colorSplitLine, me_px(0), me_py(me_ny - 1), me_px(me_nx - 1), me_py(0));
  for (uint8_t x = 1; x < me_nx - 1; ++x) dwinDrawVLine(hmiData.colorSplitLine, me_px(x), me_py(me_ny - 1), ME_WIDTH);
  for (uint8_t y = 1; y < me_ny - 1; ++y) dwinDrawHLine(hmiData.colorSplitLine, me_px(0), me_py(y), ME_WIDTH);
  for (uint8_t y = 0; y < me_ny; ++y) {
    hal.watchdog_refresh();
    for (uint8_t x = 0; x < me_nx; ++x) me_drawPoint(x, y, bedlevel.z_values[x][y], false);
  }
  DWINUI::drawButton(F("Salvar"), 26, ME_BTN_Y);
  DWINUI::drawButton(F("Sair"), 146, ME_BTN_Y);
  me_drawCursor(true);
  me_status();
  dwinUpdateLCD();
}

// ---- Movimento -----------------------------------------------------------------

// Leva o bico ate o ponto (x,y) na altura z, passando por um Z de seguranca
static void me_moveTo(const uint8_t x, const uint8_t y, const float z) {
  me_busy = true;
  LCD_MESSAGE_F("Movendo ate o ponto...");
  dwinUpdateLCD();
  gcode.process_subcommands_now(TS(F("G0 F300 Z"), p_float_t(Z_CLEARANCE_BETWEEN_PROBES, 3)));
  gcode.process_subcommands_now(TS(F("G42 F4000 I"), x, F(" J"), y));
  planner.synchronize();
  current_position.z = z;
  planner.buffer_line(current_position, homing_feedrate(Z_AXIS), active_extruder);
  planner.synchronize();
  me_moved = true;
  me_busy = false;
}

static void me_liveZ() {
  if (!planner.is_full()) {
    planner.synchronize();
    current_position.z = me_z;
    planner.buffer_line(current_position, manual_feedrate_mm_s[Z_AXIS]);
  }
}

// ---- Eventos ---------------------------------------------------------------------

// Girou o botao (ccw = sentido anti-horario)
static void me_onChange(const bool ccw) {
  if (me_busy) return;
  if (me_editing) {
    me_z += ccw ? -ME_STEP : ME_STEP;
    LIMIT(me_z, -ME_ZLIM, ME_ZLIM);
    me_liveZ();
    me_drawPoint(me_cx(), me_cy(), me_z);
    me_drawPointFrame(me_cx(), me_cy(), true);
  }
  else {
    me_drawCursor(false);
    me_cur = ccw ? (me_cur == 0 ? ME_ITEMS - 1 : me_cur - 1) : (me_cur + 1 >= ME_ITEMS ? 0 : me_cur + 1);
    me_drawCursor(true);
  }
  me_status();
}

static void me_leave() {
  if (me_moved) gcode.process_subcommands_now(TS(F("G0 F300 Z"), p_float_t(Z_CLEARANCE_BETWEEN_PROBES, 3)));
  gcode.process_subcommands_now(F("M211 S1"));    // religa os fins de curso por software
  set_bed_leveling_enabled(me_levelWas);
  hmiReturnScreen();
}

// Clicou o botao
static void me_onClick() {
  marlin.wait_start();                            // continua nesta tela (e evita reentrada durante o movimento)
  if (me_busy) return;
  if (me_cur < ME_NPTS) {
    const uint8_t x = me_cx(), y = me_cy();
    if (!me_editing) {
      me_editing = true;
      me_z = isnan(bedlevel.z_values[x][y]) ? 0 : bedlevel.z_values[x][y];
      me_drawPointFrame(x, y, true);
      me_moveTo(x, y, me_z);
      me_status();
    }
    else {
      bedlevel.z_values[x][y] = me_z;             // grava so este ponto
      me_editing = false;
      me_drawPoint(x, y, me_z);
      me_drawPointFrame(x, y, false);             // apaga a moldura amarela
      // Ricardo: apos gravar, avanca automaticamente para o proximo ponto
      // (facilita calibrar linha por linha sem ter que girar depois do click).
      if (me_cur + 1 < ME_NPTS) me_cur++;
      me_drawCursor(true);
      LCD_MESSAGE_F("Gravado. Girando vai para o proximo, clique edita");
    }
    dwinUpdateLCD();
  }
  else if (me_cur == ME_SAVE) {
    saveMesh();
    LCD_MESSAGE_F("Malha salva na EEPROM");
    dwinUpdateLCD();
  }
  else me_leave();
}

void gotoMeshEdit() {
  if (!leveling_is_valid()) { LCD_MESSAGE(MSG_UBL_MESH_INVALID); return; }
  me_nx = GRID_MAX_POINTS_X;
  me_ny = GRID_MAX_POINTS_Y;
  me_rmax = _MIN(ME_MARGIN - 2, ME_WIDTH / (2 * (me_nx - 1))) - 3;   // -3: espaco para a moldura do cursor
  me_cur = 0;
  me_editing = me_busy = me_moved = false;
  // faz o home ANTES de abrir a tela: o popup de homing devolveria para o menu
  gcode.process_subcommands_now(F("G28O"));
  me_levelWas = planner.leveling_active;
  set_bed_leveling_enabled(false);                // Z "cru" enquanto editamos
  gcode.process_subcommands_now(F("M211 S0"));    // permite Z negativo, como no nivelamento manual
  gotoPopup(me_drawAll, me_onClick, me_onChange);
}

// ---- Sub-popup: auto-preencher ponto atual --------------------------------
// Regra da base:
//   y == 0                : base = vizinho esquerda           (canto (0,0) => sem base)
//   y >  0 && x == 0      : base = vizinho de baixo
//   y >  0 && x >  0      : base = media(esquerda, baixo)
// Valor final gravado: base + offset. O offset e' lembrado enquanto a
// impressora estiver ligada (nao persiste em EEPROM).

enum : uint8_t { AF_ROW_BASE = 0, AF_ROW_OFF = 1, AF_ROW_GO = 2, AF_ROW_EXIT = 3, AF_N_ROWS = 4 };

static float   af_offset = 0.10f;   // lembrado entre chamadas
static float   af_base;             // calculado ao entrar
static bool    af_hasBase;
static uint8_t af_row;
static bool    af_edit;
static char    af_buf[8];

static void af_calcBase() {
  const uint8_t x = me_cx(), y = me_cy();
  af_hasBase = true;
  if (y == 0) {
    if (x == 0) { af_hasBase = false; af_base = 0; return; }
    af_base = bedlevel.z_values[x - 1][y];
  }
  else if (x == 0) {
    af_base = bedlevel.z_values[x][y - 1];
  }
  else {
    af_base = 0.5f * (bedlevel.z_values[x - 1][y] + bedlevel.z_values[x][y - 1]);
  }
  if (isnan(af_base)) { af_hasBase = false; af_base = 0; }
}

static void af_drawRow(uint8_t row) {
  const uint16_t y = 115 + row * 40;
  const bool selected = (row == af_row);
  const bool canGo = af_hasBase;
  const uint16_t bg = selected ? (af_edit ? COLOR_SELECT : COLOR_BG_BLUE)
                               : hmiData.colorPopupBg;
  dwinDrawRectangle(1, bg, 15, y, 257, y + 34);
  char s[24];
  if (row == AF_ROW_BASE) {
    if (canGo) sprintf_P(s, PSTR("Base:  %s mm"), dtostrf(af_base, 1, 2, af_buf));
    else       strcpy_P(s, PSTR("  sem base  "));
    DWINUI::drawCenteredString(true, font12x24, hmiData.colorPopupTxt, bg, 14, 258, y + 6, s);
  }
  else if (row == AF_ROW_OFF) {
    sprintf_P(s, PSTR("Offset: %s"), dtostrf(af_offset, 1, 2, af_buf));
    DWINUI::drawCenteredString(true, font12x24, hmiData.colorPopupTxt, bg, 14, 258, y + 6, s);
  }
  else {
    const char *label = (row == AF_ROW_GO) ? (canGo ? "Aplicar" : "(sem base)") : "Sair";
    DWINUI::drawCenteredString(true, font12x24, hmiData.colorPopupTxt, bg, 14, 258, y + 6, label);
  }
}

static void af_drawAll() {
  DWINUI::clearMainArea();
  drawPopupBkgd();
  char title[28];
  sprintf_P(title, PSTR("Auto-preencher (%i,%i)"), int(me_cx()), int(me_cy()));
  DWINUI::drawCenteredString(font12x24, hmiData.colorPopupTxt, 75, title);
  for (uint8_t i = 0; i < AF_N_ROWS; ++i) af_drawRow(i);
  DWINUI::drawCenteredString(false, font8x16, hmiData.colorPopupTxt, hmiData.colorPopupBg,
                             14, 258, 305, "gire=navegar  clique=OK");
  dwinUpdateLCD();
}

static void af_change(const bool ccw) {
  if (af_edit && af_row == AF_ROW_OFF) {
    af_offset += ccw ? -0.01f : 0.01f;
    LIMIT(af_offset, -ME_ZLIM, ME_ZLIM);
    af_drawRow(AF_ROW_OFF);
    dwinUpdateLCD();
    return;
  }
  const uint8_t old = af_row;
  if (ccw) af_row = af_row ? af_row - 1 : AF_N_ROWS - 1;
  else     af_row = (af_row + 1 < AF_N_ROWS) ? af_row + 1 : 0;
  af_drawRow(old);
  af_drawRow(af_row);
  dwinUpdateLCD();
}

static void af_click() {
  if (af_edit) {
    af_edit = false;
    af_drawRow(af_row);
    dwinUpdateLCD();
    marlin.wait_start();
    return;
  }
  if (af_row == AF_ROW_OFF) {
    af_edit = true;
    af_drawRow(af_row);
    dwinUpdateLCD();
    marlin.wait_start();
    return;
  }
  if (af_row == AF_ROW_GO) {
    if (!af_hasBase) { marlin.wait_start(); return; }        // (0,0) sem base — ignora
    const uint8_t x = me_cx(), y = me_cy();
    float v = af_base + af_offset;
    LIMIT(v, -ME_ZLIM, ME_ZLIM);
    bedlevel.z_values[x][y] = v;
    // Ja entra em modo edicao — bico vai ate o ponto na altura v e girar
    // continua ajustando (0,01 mm). Clique grava e sai do modo edicao.
    me_z = v;
    me_editing = true;
    gotoPopup(me_drawAll, me_onClick, me_onChange);
    me_moveTo(x, y, me_z);
    me_status();
    dwinUpdateLCD();
    return;
  }
  // AF_ROW_EXIT
  gotoPopup(me_drawAll, me_onClick, me_onChange);
}

bool meshEditLongPress() {
  // So aceita quando o popup ativo e' o editor de malha e nao estamos com o
  // bico em movimento nem editando um ponto em tempo real.
  if (checkkey != ID_Popup) return false;
  if (popupDraw != me_drawAll) return false;
  if (me_busy || me_editing) return false;
  af_calcBase();
  af_row = af_hasBase ? AF_ROW_OFF : AF_ROW_EXIT;   // foco util por padrao
  af_edit = false;
  gotoPopup(af_drawAll, af_click, af_change);
  return true;
}

#endif // DWIN_LCD_PROUI && MESH_BED_LEVELING
