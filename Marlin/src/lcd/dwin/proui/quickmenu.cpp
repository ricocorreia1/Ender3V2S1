/**
 * Menu rapido "Atalhos" (pressao longa no botao) — veja quickmenu.h
 */

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TOOLBAR

#include "../../../MarlinCore.h"
#include "../../marlinui.h"
#include "dwin.h"
#include "dwinui.h"
#include "dwin_popup.h"
#include "menus.h"
#include "toolbar.h"
#include "quickmenu.h"

#define QM_MAX 20                 // Voltar + ate 19 atalhos (cobre todo TBItemA)

static uint8_t qm_count;          // linhas em uso
static uint8_t qm_sel;            // indice absoluto do selecionado
static uint8_t qm_top;            // primeiro item visivel (paginacao)
static uint8_t qm_opt[QM_MAX];    // indice em TBItemA de cada linha (0 = Voltar)

static void qm_drawItem(uint8_t idx, uint8_t line) {
  if (idx == 0) drawMenuItem(line, ICON_Back, GET_TEXT_F(MSG_BUTTON_BACK));
  else { TBGetItem(qm_opt[idx]); drawMenuItem(line, TBItem->icon, TBItem->caption); }
}

static void qm_draw() {
  title.draw(F("Atalhos"));
  DWINUI::clearMainArea();
  const uint8_t last = _MIN(qm_top + TROWS, qm_count);
  for (uint8_t i = qm_top; i < last; ++i) qm_drawItem(i, i - qm_top);
  drawMenuCursor(qm_sel - qm_top);
  LCD_MESSAGE_F("Gire p/ escolher, clique p/ executar");
  dwinUpdateLCD();
}

static void qm_change(const bool ccw) {
  const uint8_t old_top = qm_top;
  eraseMenuCursor(qm_sel - qm_top);
  if (ccw) qm_sel = qm_sel ? qm_sel - 1 : qm_count - 1;
  else     qm_sel = (qm_sel + 1 < qm_count) ? qm_sel + 1 : 0;
  // ajusta a "janela" para manter o selecionado visivel
  if (qm_sel < qm_top) qm_top = qm_sel;
  else if (qm_sel >= qm_top + TROWS) qm_top = qm_sel - TROWS + 1;
  if (qm_top != old_top) qm_draw();          // rolou pagina — redesenha tudo
  else                   drawMenuCursor(qm_sel - qm_top);
}

static void qm_click() {
  if (qm_sel == 0) { hmiReturnScreen(); return; }
  TBGetItem(qm_opt[qm_sel]);
  void (*fn)() = TBItem->onClick;
  // volta para a tela de origem ANTES de executar: assim o item se comporta
  // exatamente como se tivesse sido clicado na barra (popups, homing etc.
  // voltam para o menu em que voce estava)
  hmiReturnScreen();
  if (fn) fn();
}

void gotoQuickMenu() {
  qm_count = 1; qm_sel = 0; qm_top = 0; qm_opt[0] = 0;
  // Ricardo: mostra todos os itens disponiveis em TBItemA (nao so os 5 da
  // barra do home). Assim o usuario tem acesso rapido a todas as acoes.
  const uint8_t N = toolBar.OptCount();
  for (uint8_t i = 1; i < N && qm_count < QM_MAX; ++i) {
    TBGetItem(i);
    if (TBItem->icon) qm_opt[qm_count++] = i;
  }
  gotoPopup(qm_draw, qm_click, qm_change);
}

// Chamado por dwinHandleScreen() quando ha uma pressao longa pendente
void quickMenuLongPress() {
  if (checkkey == ID_Popup && popupDraw == qm_draw) { hmiReturnScreen(); return; }   // ja aberto: fecha
  if (isPrinting()) return;                                                            // nunca durante impressao
  if (checkkey != ID_Menu && checkkey != ID_MainMenu) return;                          // so em menus / tela inicial
  gotoQuickMenu();
}

#endif // HAS_TOOLBAR
