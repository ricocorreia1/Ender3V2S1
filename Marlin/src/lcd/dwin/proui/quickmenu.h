/**
 * Menu rapido "Atalhos" (pressao longa no botao) para o Pro UI
 *
 * Segurar o botao ~0,7 s em qualquer menu ou na tela inicial abre uma lista
 * com os mesmos itens da barra de atalhos; "Voltar" (ou outra pressao longa)
 * devolve para a tela onde se estava. Bloqueado durante impressao.
 *
 * Acrescimo local ao firmware Professional (mriscoc); licenca LGPLv3.
 */
#pragma once

#include "../../../inc/MarlinConfigPre.h"

#if HAS_TOOLBAR
  void gotoQuickMenu();
  void quickMenuLongPress();   // chamar quando encoderLongPress estiver pendente
#endif
