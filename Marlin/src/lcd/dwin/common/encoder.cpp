/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2021 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

/*****************************************************************************
 * @file     lcd/dwin/common/encoder.cpp
 * @brief    Rotary encoder functions
 *****************************************************************************/

#include "../../../inc/MarlinConfigPre.h"

#if HAS_DWIN_E3V2

#include "encoder.h"
#include "../../buttons.h"

#include "../../../MarlinCore.h"
#include "../../marlinui.h"
#include "../../../HAL/shared/Delay.h"

#if HAS_SOUND
  #include "../../../libs/buzzer.h"
#endif

#include <stdlib.h>

EncoderRate encoderRate;
bool encoderLongPress = false;

// TODO: Replace with ui.quick_feedback
void Encoder_tick() {
  #if HAS_CHIRP
    #if ENABLED(SPEAKER)
      if (ui.sound_on) BUZZ(LCD_FEEDBACK_FREQUENCY_DURATION_MS, LCD_FEEDBACK_FREQUENCY_HZ);
    #elif HAS_BEEPER
      if (ui.sound_on) buzzer.click(10);
    #endif
  #endif
}

// Analyze encoder value and return state
EncoderState encoderReceiveAnalyze() {
  const millis_t now = millis();
  static int8_t temp_diff = 0; // Cleared on each full step, as configured

  EncoderState temp_diffState = ENCODER_DIFF_NO;

  // ---- Botao: clique ao SOLTAR; segurar >= ENCODER_LONG_MS = pressao longa (Ricardo) ----
  // Antes: cada 300 ms pressionado gerava um novo clique (rajada de bips).
  static bool     enc_pressed   = false;   // estado visto na ultima leitura
  static bool     enc_long_done = false;   // pressao longa ja disparada nesta apertada
  static millis_t enc_press_ms  = 0;       // instante em que apertou
  static millis_t enc_seen_ms   = 0;       // ultima vez que vimos o botao apertado
  static millis_t enc_release_ms = 0;      // instante em que soltou (debounce)
  if (BUTTON_PRESSED(ENC)) {
    if (!enc_pressed) {
      if (PENDING(now, enc_release_ms + 60)) return ENCODER_DIFF_NO;   // rebote logo apos soltar
      enc_pressed = true; enc_long_done = false; enc_press_ms = now;
    }
    enc_seen_ms = now;
    if (!enc_long_done && ELAPSED(now, enc_press_ms + ENCODER_LONG_MS)) {
      enc_long_done = true;
      Encoder_tick();
      TERN_(HAS_BACKLIGHT_TIMEOUT, ui.refresh_backlight_timeout());
      if (!ui.backlight) { ui.refresh_brightness(); return ENCODER_DIFF_NO; }
      encoderLongPress = true;               // consumido em dwinHandleScreen()
    }
    return ENCODER_DIFF_NO;
  }
  else if (enc_pressed) {                    // acabou de soltar
    enc_pressed = false;
    enc_release_ms = now;
    if (enc_long_done) return ENCODER_DIFF_NO;                  // ja tratado como pressao longa
    if (PENDING(now, enc_press_ms + 25)) return ENCODER_DIFF_NO; // rebote ao apertar
    if (ELAPSED(now, enc_seen_ms + 150)) return ENCODER_DIFF_NO; // ficamos sem ler (ex.: homing): descarta
    Encoder_tick();
    TERN_(HAS_BACKLIGHT_TIMEOUT, ui.refresh_backlight_timeout());
    if (!ui.backlight) { ui.refresh_brightness(); return ENCODER_DIFF_NO; }
    const bool was_waiting = marlin.wait_for_user;
    marlin.user_resume();
    return was_waiting ? ENCODER_DIFF_NO : ENCODER_DIFF_ENTER;
  }

  temp_diff += ui.get_encoder_delta();

  const int8_t abs_diff = ABS(temp_diff);
  if (abs_diff >= ENCODER_PULSES_PER_STEP) {
    temp_diffState = temp_diff > 0
      ? TERN(REVERSE_ENCODER_DIRECTION, ENCODER_DIFF_CCW, ENCODER_DIFF_CW)
      : TERN(REVERSE_ENCODER_DIRECTION, ENCODER_DIFF_CW,  ENCODER_DIFF_CCW);

    int32_t encoder_multiplier = 1;

    #if ENABLED(ENCODER_RATE_MULTIPLIER)

      const millis_t ms = millis();

      // Encoder rate multiplier
      if (encoderRate.enabled) {
        // Note that the rate is always calculated between two passes through the
        // loop and that the abs of the temp_diff value is tracked.
        const float encoderStepRate = ((float(abs_diff) / float(ENCODER_PULSES_PER_STEP)) * 1000.0f) / float(ms - encoderRate.lastEncoderTime);
        encoderRate.lastEncoderTime = ms;
        if (ENCODER_100X_STEPS_PER_SEC > 0 && encoderStepRate >= ENCODER_100X_STEPS_PER_SEC)
          encoder_multiplier = 100;
        else if (ENCODER_10X_STEPS_PER_SEC > 0 && encoderStepRate >= ENCODER_10X_STEPS_PER_SEC)
          encoder_multiplier = 10;
        else if (ENCODER_5X_STEPS_PER_SEC > 0 && encoderStepRate >= ENCODER_5X_STEPS_PER_SEC)
          encoder_multiplier = 5;
      }

    #endif

    encoderRate.encoderMoveValue = abs_diff * encoder_multiplier / (ENCODER_PULSES_PER_STEP);

    temp_diff = 0;
  }

  if (temp_diffState != ENCODER_DIFF_NO) {
    TERN_(HAS_BACKLIGHT_TIMEOUT, ui.refresh_backlight_timeout());
    if (!ui.backlight) ui.refresh_brightness();
  }

  return temp_diffState;
}

EncoderState get_encoder_menuitems() { return TERN(SMOOTH_ENCODER_MENUITEMS, get_encoder_state(), encoderReceiveAnalyze()); }

#if PIN_EXISTS(LCD_LED)

  // Take the low 24 valid bits  24Bit: G7 G6 G5 G4 G3 G2 G1 G0 R7 R6 R5 R4 R3 R2 R1 R0 B7 B6 B5 B4 B3 B2 B1 B0
  uint16_t LED_DataArray[LED_NUM];

  // LED light operation
  void LED_Action() {
    LED_Control(RGB_SCALE_WARM_WHITE, 0x0F);
    delay(30);
    LED_Control(RGB_SCALE_WARM_WHITE, 0x00);
  }

  // LED initialization
  void LED_Configuration() {
    SET_OUTPUT(LCD_LED_PIN);
  }

  // LED write data
  void LED_WriteData() {
    for (uint8_t tempCounter_LED = 0; tempCounter_LED < LED_NUM; tempCounter_LED++) {
      for (uint8_t tempCounter_Bit = 0; tempCounter_Bit < 24; tempCounter_Bit++) {
        if (LED_DataArray[tempCounter_LED] & (0x800000 >> tempCounter_Bit)) {
          LED_DATA_HIGH;
          DELAY_NS(300);
          LED_DATA_LOW;
          DELAY_NS(200);
        }
        else {
          LED_DATA_HIGH;
          LED_DATA_LOW;
          DELAY_NS(200);
        }
      }
    }
  }

  // LED control
  //  RGB_Scale: RGB color ratio
  //  luminance: brightness (0~0xFF)
  void LED_Control(const uint8_t RGB_Scale, const uint8_t luminance) {
    for (uint8_t i = 0; i < LED_NUM; i++) {
      LED_DataArray[i] = 0;
      switch (RGB_Scale) {
        case RGB_SCALE_R10_G7_B5: LED_DataArray[i] = (luminance * 10/10) << 8 | (luminance * 7/10) << 16 | luminance * 5/10; break;
        case RGB_SCALE_R10_G7_B4: LED_DataArray[i] = (luminance * 10/10) << 8 | (luminance * 7/10) << 16 | luminance * 4/10; break;
        case RGB_SCALE_R10_G8_B7: LED_DataArray[i] = (luminance * 10/10) << 8 | (luminance * 8/10) << 16 | luminance * 7/10; break;
      }
    }
    LED_WriteData();
  }

  // LED gradient control
  //  RGB_Scale: RGB color ratio
  //  luminance: brightness (0~0xFF)
  //  change_Time: gradient time (ms)
  void LED_GraduallyControl(const uint8_t RGB_Scale, const uint8_t luminance, const uint16_t change_Interval) {
    struct { uint8_t g, r, b; } led_data[LED_NUM];
    for (uint8_t i = 0; i < LED_NUM; i++) {
      switch (RGB_Scale) {
        case RGB_SCALE_R10_G7_B5:
          led_data[i] = { luminance * 7/10, luminance * 10/10, luminance * 5/10 };
          break;
        case RGB_SCALE_R10_G7_B4:
          led_data[i] = { luminance * 7/10, luminance * 10/10, luminance * 4/10 };
          break;
        case RGB_SCALE_R10_G8_B7:
          led_data[i] = { luminance * 8/10, luminance * 10/10, luminance * 7/10 };
          break;
      }
    }

    struct { bool g, r, b; } led_flag;
    for (uint8_t i = 0; i < LED_NUM; i++) {
      led_flag = { false, false, false };
      while (1) {
        const uint8_t g = uint8_t(LED_DataArray[i] >> 16),
                      r = uint8_t(LED_DataArray[i] >> 8),
                      b = uint8_t(LED_DataArray[i]);
        if (g == led_data[i].g) led_flag.g = true;
        else LED_DataArray[i] += (g > led_data[i].g) ? -_BV32(16) : _BV32(16);
        if (r == led_data[i].r) led_flag.r = true;
        else LED_DataArray[i] += (r > led_data[i].r) ? -_BV32(8) : _BV32(8);
        if (b == led_data[i].b) led_flag.b = true;
        else LED_DataArray[i] += (b > led_data[i].b) ? -_BV32(0) : _BV32(0);

        LED_WriteData();
        if (led_flag.g && led_flag.r && led_flag.b) break;
        delay(change_Interval);
      }
    }
  }

#endif // LCD_LED

#endif // HAS_DWIN_E3V2
