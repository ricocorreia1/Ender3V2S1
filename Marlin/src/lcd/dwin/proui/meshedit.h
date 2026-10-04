/**
 * Editor interativo de malha (Manual Mesh) para o Pro UI
 *
 * Tela "Editar malha": mostra a grade colorida da malha, um cursor que anda
 * pelos pontos girando o botao; ao clicar num ponto o bico vai ate ele na
 * altura guardada, girar sobe/desce o bico (0,01 mm) e clicar de novo grava
 * o valor. Depois dos pontos o cursor cai em "Salvar" (EEPROM) e "Sair".
 *
 * Este arquivo e' um acrescimo local ao firmware Professional (mriscoc);
 * licenca LGPLv3 como o restante do Pro UI.
 */
#pragma once

#include "../../../inc/MarlinConfigPre.h"

// Vale para a malha manual (MESH_BED_LEVELING) e para a malha da sonda
// (AUTO_BED_LEVELING_BILINEAR - BLTouch/CRTouch): as duas guardam os pontos em
// bedlevel.z_values. UBL fica de fora (tem editor proprio).
#if ENABLED(DWIN_LCD_PROUI) && ANY(MESH_BED_LEVELING, AUTO_BED_LEVELING_BILINEAR)
  #define HAS_MESH_EDITOR 1
  void gotoMeshEdit();
  // Chamado no long-press quando o editor de malha esta aberto.
  // Abre um sub-popup para "auto-preencher" o ponto atual usando os vizinhos
  // (esquerda / baixo) + um offset parametrizavel. Devolve true se tratou.
  bool meshEditLongPress();
#endif
