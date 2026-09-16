/**
 * Atalho "Limpar bico" para a barra de atalhos do Pro UI
 *
 * Aquece o bico a 180 C, faz home e sobe o Z para 150 mm, deixando
 * espaco para limpeza do bico. Usa M109 para so terminar a fila quando
 * a temperatura for atingida.
 *
 * Acrescimo local ao firmware Professional (mriscoc); licenca LGPLv3.
 */
#pragma once

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(DWIN_LCD_PROUI) && HAS_HOTEND
  void tbCleanNozzle();
#endif
