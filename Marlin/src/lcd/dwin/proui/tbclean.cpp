/**
 * Atalho "Limpar bico" — veja tbclean.h
 */

#include "../../../inc/MarlinConfigPre.h"

#if ENABLED(DWIN_LCD_PROUI) && HAS_HOTEND

#include "../../../gcode/queue.h"
#include "../../marlinui.h"
#include "tbclean.h"

void tbCleanNozzle() {
  LCD_MESSAGE_F("Limpeza: aquecendo bico a 180 C");
  // M104: comeca a aquecer sem bloquear.
  // G28:  home nos tres eixos.
  // G0 Z150: sobe o bico para dar espaco de limpeza.
  // M109: so libera a fila quando o bico atingir 180 C.
  queue.inject(F("M104 S180\nG28\nG0 Z150 F1200\nM109 S180"));
}

#endif // DWIN_LCD_PROUI && HAS_HOTEND
