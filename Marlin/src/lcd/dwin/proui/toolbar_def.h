/**
 * toolBar for PRO UI
 * Author: Miguel A. Risco-Castillo (MRISCOC)
 * version: 2.1.1
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

#pragma once

#include "../../../inc/MarlinConfig.h"

#include "dwin.h"
#include "toolbar.h"
#include "tbheat.h"
#include "tbclean.h"
#include "tbcalib.h"

#define TB_ITEM(I,L,V...) {I, GET_TEXT_F(L), V}
#define TB_ITEM_F(I,L,V...) {I, F(L), V}

const TBItem_t TBItemA[] = {
  TB_ITEM(0, MSG_OPTION_DISABLED, nullptr),
  TB_ITEM(ICON_Homing, MSG_AUTO_HOME, autoHome),
  #if HAS_BED_PROBE
    #if HAS_TRAMMING_WIZARD
      TB_ITEM(ICON_BedTramming, MSG_TRAMMING_WIZARD, runTrammingWizard),
    #endif
    TB_ITEM(ICON_SetZOffset, MSG_PROBE_WIZARD, drawZOffsetWizMenu),
    TB_ITEM(ICON_Level, MSG_AUTO_MESH, autoLev),
  #else
    TB_ITEM(ICON_MoveZ0, MSG_HOME_Z_AND_DISABLE, homeZandDisable),
  #endif
  TB_ITEM(ICON_CloseMotor, MSG_DISABLE_STEPPERS, disableMotors),
  TB_ITEM(ICON_Cool, MSG_COOLDOWN, doCoolDown),
  #if HAS_PREHEAT
    #define _TBPREHEAT(N) TB_ITEM(ICON_Preheat##N, MSG_PREHEAT_##N, DoPreheat##N),
    REPEAT_1(PREHEAT_COUNT, _TBPREHEAT)
  #endif
  #if HAS_LCD_BRIGHTNESS
    TB_ITEM(ICON_Brightness, MSG_BRIGHTNESS_OFF, turnOffBacklight),
  #endif
  TB_ITEM(ICON_Reboot, MSG_RESET_PRINTER, rebootPrinter),
  TB_ITEM(ICON_WriteEEPROM, MSG_STORE_EEPROM, writeEeprom),
  TB_ITEM(ICON_Park, MSG_FILAMENT_PARK_ENABLED, parkHead),
  // --- acrescimos (Ricardo): aquecer so o bico / so a mesa, com popup de temperatura ---
  #if HAS_HOTEND
    TB_ITEM_F(ICON_SetEndTemp, "Aquecer bico", tbHeatHotend),
  #endif
  #if HAS_HEATED_BED
    TB_ITEM_F(ICON_SetBedTemp, "Aquecer mesa", tbHeatBed),
  #endif
  // --- acrescimo (Ricardo): rotina rapida de limpeza do bico ---
  #if HAS_HOTEND
    TB_ITEM_F(ICON_HotendTemp, "Limpar bico", tbCleanNozzle),
  #endif
  // --- acrescimo (Ricardo): preparar temperaturas para calibrar malha manual ---
  #if HAS_HOTEND && HAS_HEATED_BED
    TB_ITEM_F(ICON_ManualMesh, "Preparar calibracao", tbPrepareCalib),
  #endif
  // --- acrescimo (Ricardo): voltar rapido para a tela inicial ---
  TB_ITEM_F(ICON_Back, "Ir para tela inicial", gotoMainMenu),
};
