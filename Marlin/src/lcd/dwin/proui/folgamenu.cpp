/**
 * Menu "Folga do eixo Z" — veja folgamenu.h
 */

#include "../../../inc/MarlinConfigPre.h"
#include "folgamenu.h"

#if HAS_FOLGA_MENU

#include "../../../MarlinCore.h"
#include "../../../feature/backlash.h"
#include "../../marlinui.h"
#include "dwin.h"
#include "dwinui.h"
#include "menus.h"

static Menu *folgaMenu = nullptr;

// copias editaveis: o menu edita estas variaveis e o "apply" repassa para o Marlin
static float   fg_z   = 0;     // mm
static int16_t fg_pct = 100;   // %

static void applyFolgaZ()   { backlash.set_distance_mm(Z_AXIS, fg_z); }
static void applyFolgaPct() { backlash.set_correction(fg_pct / 100.0f); }

static void setFolgaZ()   { setPFloatOnClick(0, 2, 2, applyFolgaZ); }
static void setFolgaPct() { setPIntOnClick(0, 100, applyFolgaPct); }

void drawFolgaMenu() {
  fg_z   = backlash.get_distance_mm(Z_AXIS);
  fg_pct = int16_t(backlash.get_correction() * 100.0f + 0.5f);
  if (notCurrentMenu(folgaMenu)) {
    BACK_ITEM(drawAdvancedSettingsMenu);
    EDIT_ITEM_F(ICON_StepZ, "Folga Z (mm)", onDrawPFloat2Menu, setFolgaZ, &fg_z);
    EDIT_ITEM_F(ICON_MoveZ, "Correcao (%)", onDrawPIntMenu, setFolgaPct, &fg_pct);
    #if ENABLED(EEPROM_SETTINGS)
      MENU_ITEM(ICON_WriteEEPROM, MSG_STORE_EEPROM, onDrawMenuItem, writeEeprom);
    #endif
  }
  SET_MENU_F(folgaMenu, "Folga do eixo Z");
}

#endif // HAS_FOLGA_MENU
