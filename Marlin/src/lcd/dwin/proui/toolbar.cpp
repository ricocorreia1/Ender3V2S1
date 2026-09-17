/**
 * toolBar for PRO UI
 * Author: Miguel A. Risco-Castillo (MRISCOC)
 * version: 3.1.1
 * Date: 2023/09/12
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "../../../inc/MarlinConfig.h"

#if ALL(DWIN_LCD_PROUI, HAS_TOOLBAR)

#include "../../marlinui.h"
#include "dwin.h"
#include "toolbar.h"
#include "toolbar_def.h"
// os indices 13/14/15/16/17 do DEF_TBOPT (Aquecer bico / Aquecer mesa / Limpar bico / Preparar calibracao / Ir para tela inicial) dependem desta contagem
static_assert(COUNT(TBItemA) == 18, "TBItemA mudou de tamanho: ajuste DEF_TBOPT em proui_ex.h");
#include "menus.h"

const TBItem_t *TBItem;
ToolBar toolBar;

uint8_t ToolBar::OptCount() {
  return COUNT(TBItemA);
}

void onDrawTBItem(int8_t pos, int8_t line) {
  UNUSED(line);   // usamos pos: posicao horizontal fixa, ignora scroll (topline)
  const bool focused = (checkkey == ID_Menu);
  const int8_t sel = toolBar.selected;
  // Ricardo: layout compacto (sem shift horizontal); a legenda do selecionado
  // eh desenhada na area de status (embaixo dos icones). Isso deixa a barra
  // comportar ate 10 icones sem cortar o texto.
  const uint16_t iconsW = uint16_t(B_XPOS) * toolBar.count;
  const uint8_t xoff = (iconsW < DWIN_WIDTH) ? (DWIN_WIDTH - iconsW) / 2 : 0;
  const uint8_t xp = xoff + pos * B_XPOS;
  dwinDrawBox(1, hmiData.colorBackground, xp - 2, TBYPOS, B_XPOS, TBHEIGHT);
  if (focused && (pos == sel)) {
    dwinDrawBox(1, COLOR_BG_WINDOW, xp - 2, TBYPOS, B_XPOS, TBHEIGHT);
    // Ricardo: setar como mensagem de status faz o desenho periodico do LCD
    // exibir a legenda (ela persiste ate expirar ou ser substituida — antes,
    // desenhando direto em STATUS_Y, o refresh imediato apagava a legenda).
    ui.set_status(getMenuItem(pos)->caption);
  }
  DWINUI::drawIcon(getMenuItem(pos)->icon, xp, B_YPOS);
};

void drawToolBar() {
  if (notCurrentMenu(&toolBar)) {
    for (uint8_t i = 0; i < TBMaxOpt; ++i) {
      TBGetItem(PRO_data.TBopt[i]);
      if (TBItem->icon) MENU_ITEM_F(TBItem->icon, TBItem->caption, onDrawTBItem, TBItem->onClick);
    }
    toolBar.onExit = &exitToolBar;
    toolBar.count = menuItemCount;
    currentMenu = &toolBar;
  }
  // Ricardo: forca topline=0. Como count pode ser > TROWS, a lib base entende
  // que o "menu" esta rolavel e pode desenhar UI de scroll (faixa preta no topo).
  // Zerando a topline mantemos a lib no estado "sem scroll".
  toolBar.topline = 0;
  // Ricardo: NAO chamamos toolBar.draw() aqui — a Menu::draw() da lib limita
  // o desenho a TROWS=6 linhas, o que deixaria de fora atalhos alem do 6o.
  // Desenhamos todos os icones manualmente. A parte de click/scroll continua
  // funcionando porque currentMenu = &toolBar.
  dwinDrawRectangle(1, hmiData.colorBackground, 0, TBYPOS, DWIN_WIDTH, TBYPOS + TBHEIGHT);
  for (uint8_t i = 0; i < toolBar.count; ++i) onDrawTBItem(i, i);
}

void updateTBSetupItem(int8_t pos, uint8_t val) {
  TBGetItem(val);
  getMenuItem(pos)->icon = TBItem->icon ?: ICON_Info;
  getMenuItem(pos)->caption = TBItem->caption;
}

void drawTBSetupItem(bool focused) {
  const uint8_t line = currentMenu->line();
  const uint16_t ypos = MYPOS(line) + 1;
  DWINUI::drawBox(1, focused ? COLOR_BG_BLACK : hmiData.colorBackground, { 15, ypos, DWIN_WIDTH - 15, MLINE - 1 });
  onDrawMenuItem(currentMenu->selected, line);
  if (focused) DWINUI::drawChar(VALX + 24, MBASE(line), 18);
}

void TBGetItem(uint8_t item) {
  const uint8_t N = toolBar.OptCount() - 1;
  if (WITHIN(item, 1, N))
    TBItem = &TBItemA[item];
  else
    TBItem = &TBItemA[0];
}

#endif // ALL(DWIN_LCD_PROUI, HAS_TOOLBAR)