/**
 * Menu "Folga do eixo Z" (compensacao de folga / backlash) para o Pro UI
 *
 * Avancado -> Folga do eixo Z:
 *   Folga Z (mm)     - distancia que o Z anda "em falso" ao inverter o sentido
 *   Correcao (%)     - quanto dessa folga o firmware compensa (100 = toda)
 *   Salvar           - grava na EEPROM (mesmo que M425 + M500)
 *
 * Acrescimo local ao firmware Professional (mriscoc); licenca LGPLv3.
 */
#pragma once

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(DWIN_LCD_PROUI) && ENABLED(BACKLASH_GCODE)
  #define HAS_FOLGA_MENU 1
  void drawFolgaMenu();
#endif
