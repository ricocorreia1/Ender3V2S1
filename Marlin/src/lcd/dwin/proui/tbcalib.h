/**
 * Atalho "Preparar calibracao" para a barra de atalhos do Pro UI
 *
 * Abre um popup com tres linhas ("Bico", "Mesa", "Iniciar"): girar
 * navega, clicar em Bico/Mesa entra em modo edicao (girar ajusta 5 em
 * 5 graus, clique aplica), clicar em Iniciar comeca a aquecer bico e
 * mesa nos valores atuais. Serve para preparar a maquina para a
 * nivelacao manual de malha, onde as temperaturas influenciam.
 *
 * Os ultimos valores usados sao lembrados enquanto a impressora estiver
 * ligada (nao persiste em EEPROM).
 *
 * Acrescimo local ao firmware Professional (mriscoc); licenca LGPLv3.
 */
#pragma once

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(DWIN_LCD_PROUI)
  #if HAS_HOTEND && HAS_HEATED_BED
    void tbPrepareCalib();
  #endif
#endif
