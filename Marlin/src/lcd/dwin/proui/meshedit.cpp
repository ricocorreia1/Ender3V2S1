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
  const uint16_t c = on ? (me_editing ? hmiData.colorHighlight : hmiData.colorText) : hmiData.colorBackground;
  dwinDrawRectangle(0, c, cx - d, cy - d, cx + d, cy + d);
  dwinDrawRectangle(0, c, cx - d - 1, cy - d - 1, cx + d + 1, cy + d + 1);
  if (!on) me_drawGridAt(x, y, d + 1);   // recompoe as linhas que a moldura cobria
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
    ui.status_printf(0, F("Z=%s  gire ajusta, clique grava"), dtostrf(me_z, 1, 2, me_buf));
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
      me_drawPointFrame(x, y, true);
      LCD_MESSAGE_F("Ponto gravado. Salvar = guarda na EEPROM");
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

#endif // DWIN_LCD_PROUI && MESH_BED_LEVELING
