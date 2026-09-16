/**
 * Atalhos "Aquecer bico" / "Aquecer mesa" para a barra de atalhos do Pro UI
 *
 * Ao acionar, abre um popup com a temperatura: girar ajusta (5 em 5 graus,
 * 10 em 10 girando rapido), clicar aplica. 0 = desligar o aquecedor.
 *
 * Acrescimo local ao firmware Professional (mriscoc); licenca LGPLv3.
 */
#pragma once

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(DWIN_LCD_PROUI)
  #if HAS_HOTEND
    void tbHeatHotend();
  #endif
  #if HAS_HEATED_BED
    void tbHeatBed();
  #endif
#endif
