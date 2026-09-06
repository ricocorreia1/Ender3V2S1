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
#pragma once

#include "../../../inc/MarlinConfig.h"

//
// dwin/common/dwin_api.h
//
// Included by: dwin/*/dwin_lcd.h
//

#if ENABLED(DWIN_MARLINUI_LANDSCAPE)
  #define DWIN_WIDTH  480
  #define DWIN_HEIGHT 272
#else
  #define DWIN_WIDTH  272
  #define DWIN_HEIGHT 480
#endif

#define RECEIVED_NO_DATA         0x00
#define RECEIVED_SHAKE_HAND_ACK  0x01

#define FHONE                    0xAA

#define DWIN_SCROLL_UP   2
#define DWIN_SCROLL_DOWN 3

// Make sure dwinSendBuf is large enough to hold the largest string plus draw command and tail.
// Assume the narrowest (6 pixel) font and 2-byte gb2312-encoded characters.
extern uint8_t dwinSendBuf[11 + DWIN_WIDTH / 6 * 2];
extern uint8_t dwinBufTail[4];
extern uint8_t databuf[26];

inline void dwinByte(size_t &i, const uint16_t bval) {
  dwinSendBuf[++i] = bval;
}

inline void dwinWord(size_t &i, const uint16_t wval) {
  dwinSendBuf[++i] = wval >> 8;
  dwinSendBuf[++i] = wval & 0xFF;
}

inline void dwinLong(size_t &i, const uint32_t lval) {
  dwinSendBuf[++i] = (lval >> 24) & 0xFF;
  dwinSendBuf[++i] = (lval >> 16) & 0xFF;
  dwinSendBuf[++i] = (lval >>  8) & 0xFF;
  dwinSendBuf[++i] = lval & 0xFF;
}

// Send the data in the buffer plus the packet tail
void dwinSend(size_t &i);

/**
 * PT-BR: letras acentuadas na tela DWIN.
 *
 * A tela nao entende UTF-8: ela desenha um byte = um caractere, buscando o
 * bitmap numa tabela ASCII de 128 posicoes. As acentuadas foram desenhadas
 * em CODIGOS LIVRES dessa tabela (arquivo de fontes 0T5UIC1.HZK, gerado por
 * fonte_ptbr.py). Aqui esta a unica traducao necessaria: todo texto que vai
 * para o display passa por dwinText, entao e' aqui que a sequencia UTF-8
 * (0xC3 + byte) vira o codigo do glifo.
 *
 * O que nao tem glifo proprio cai para a letra sem acento — nunca lixo na
 * tela, mesmo em nome de arquivo do cartao com acento estranho.
 */
// Textos que vem da parte fechada do ProUI (libproui.a) ja' traduzidos.
// Definida em dwin_api.cpp; usada aqui na copia E na medida do texto, para
// que a centralizacao use o tamanho da frase em portugues.
const char* dwinPtbrTexto(const char * const s);

inline char dwinGlifoLatino(const uint8_t b) {
  switch (b) {
    case 0xA3: return 28;      // a til
    case 0xB5: return 29;      // o til
    case 0xA7: return 30;      // c cedilha
    case 0xA1: return 31;      // a agudo
    case 0xA9: return 0x60;    // e agudo   (ocupa o lugar da crase)
    case 0xAA: return 0x5E;    // e circunflexo (^)
    case 0xAD: return 0x7D;    // i agudo   (})
    case 0xB3: return 0x26;    // o agudo   (&)
    case 0xA2: return 0x3B;    // a circunflexo (;)
    case 0xBA: return 0x22;    // u agudo   (")
    case 0xB4: return 0x27;    // o circunflexo (')
    // sem glifo proprio: usa a letra sem acento
    case 0xA0: case 0xA4: case 0xA5: case 0xA6: return 'a';
    case 0xA8: case 0xAB: case 0xAC: return 'e';
    case 0xAE: case 0xAF: return 'i';
    case 0xB1: return 'n';
    case 0xB2: case 0xB6: case 0xB8: return 'o';
    case 0xB9: case 0xBB: case 0xBC: return 'u';
    case 0xBD: return 'y';
    case 0x80: case 0x81: case 0x82: case 0x83: case 0x84: case 0x85: return 'A';
    case 0x87: return 'C';
    case 0x88: case 0x89: case 0x8A: case 0x8B: return 'E';
    case 0x8C: case 0x8D: case 0x8E: case 0x8F: return 'I';
    case 0x91: return 'N';
    case 0x92: case 0x93: case 0x94: case 0x95: case 0x96: return 'O';
    case 0x99: case 0x9A: case 0x9B: case 0x9C: return 'U';
    default:   return '?';
  }
}

// Copia convertendo UTF-8 -> codigos da tela. Devolve quantos bytes gravou.
inline size_t dwinCopiaTexto(uint8_t * const dst, const char *s0, const size_t cap, const uint16_t rlimit) {
  const char *s = dwinPtbrTexto(s0);
  size_t n = 0;
  while (*s && n < cap && n < rlimit) {
    const uint8_t c = (uint8_t)*s++;
    const uint8_t nx = (uint8_t)*s;                 // proximo byte (0 se acabou)
    const bool cont = (nx >= 0x80 && nx <= 0xBF);   // byte de continuacao UTF-8
    if (c == 0xC3 && cont)      { dst[n++] = (uint8_t)dwinGlifoLatino(nx); s++; }
    else if (c == 0xC2 && cont) s++;                // grau, nao-quebra etc: descarta
    else if (c < 0x80)          dst[n++] = c;       // ASCII normal
    // Nome de arquivo vem do cartao em Latin-1 (1 byte por acentuada, sem o
    // 0xC3 na frente): 0xE7=c, 0xE3=a til... O byte menos 0x40 e' exatamente
    // o que dwinGlifoLatino ja entende. Sem isto o acento do nome virava lixo.
    else if (c >= 0xC0)         dst[n++] = (uint8_t)dwinGlifoLatino((uint8_t)(c - 0x40));
    // 0x80-0xBF soltos: ignorados (evitam lixo na tela)
  }
  return n;
}

// Comprimento do texto EM CARACTERES DE TELA (para centralizar e avancar o
// cursor): uma acentuada ocupa 2 bytes em UTF-8 mas 1 posicao no display.
inline size_t dwinTextLen(const char *s0) {
  const char *s = dwinPtbrTexto(s0);
  size_t n = 0;
  while (s && *s) {
    const uint8_t c = (uint8_t)*s++;
    const uint8_t nx = (uint8_t)*s;
    const bool cont = (nx >= 0x80 && nx <= 0xBF);
    if ((c == 0xC3 || c == 0xC2) && cont) { s++; if (c == 0xC3) n++; }
    else if (c < 0x80) n++;
    else if (c >= 0xC0) n++;    // acentuada Latin-1 do nome do arquivo: 1 posicao
  }
  return n;
}
inline size_t dwinTextLen(FSTR_P s) { return dwinTextLen(FTOP(s)); }

inline void dwinText(size_t &i, const char * const string, uint16_t rlimit=0xFFFF) {
  if (!string) return;
  const size_t len = dwinCopiaTexto(&dwinSendBuf[i+1], string, sizeof(dwinSendBuf) - i, rlimit);
  i += len;
}

inline void dwinText(size_t &i, FSTR_P string, uint16_t rlimit=0xFFFF) {
  if (!string) return;
  const size_t len = dwinCopiaTexto(&dwinSendBuf[i+1], FTOP(string), sizeof(dwinSendBuf) - i, rlimit);
  i += len;
}

/*-------------------------------------- System variable function --------------------------------------*/

// Handshake (1: Success, 0: Fail)
bool dwinHandshake();

// DWIN startup
void dwinStartup();

#if HAS_LCD_BRIGHTNESS
  // Set the backlight brightness
  //  brightness: (0x00-0xFF)
  void dwinLCDBrightness(const uint8_t brightness);
#endif

// Set screen display direction
//  dir: 0=0°, 1=90°, 2=180°, 3=270°
void dwinFrameSetDir(uint8_t dir);

// Update display
void dwinUpdateLCD();

/*---------------------------------------- Drawing functions ----------------------------------------*/

// Clear screen
//  color: Clear screen color
void dwinFrameClear(const uint16_t color);

// Draw a point
//  color: point color
//  width: point width   0x01-0x0F
//  height: point height 0x01-0x0F
//  x,y: upper left point
#if ENABLED(TJC_DISPLAY)
  void dwinDrawBox(uint8_t mode, uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t xSize, uint16_t ySize);
  inline void dwinDrawPoint(uint16_t color, uint8_t width, uint8_t height, uint16_t x, uint16_t y) {
    dwinDrawBox(1, color, x, y, 1, 1);
  }
#else
  void dwinDrawPoint(uint16_t color, uint8_t width, uint8_t height, uint16_t x, uint16_t y);
#endif

// Draw a map of multiple points using minimal amount of point drawing commands
//  color: point color
//  point_width: point width   0x01-0x0F
//  point_height: point height 0x01-0x0F
//  x,y: upper left point
//  map_columns: columns in theh point map. each column is a byte in the map and contains 8 points
//  map_rows: rows in the point map
//  map: point bitmap. 2D array of points, 1 bit per point
#if DISABLED(TJC_DISPLAY)
  void dwinDrawPointMap(
    const uint16_t color,
    const uint8_t point_width, const uint8_t point_height,
    const uint16_t x, const uint16_t y,
    const uint16_t map_columns, const uint16_t map_rows,
    const uint8_t *map_data
  );
#endif

// Draw a line
//  color: Line segment color
//  xStart/yStart: Start point
//  xEnd/yEnd: End point
void dwinDrawLine(uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);

// Draw a Horizontal line
//  color: Line segment color
//  xStart/yStart: Start point
//  xLength: Line Length
inline void dwinDrawHLine(uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t xLength) {
  dwinDrawLine(color, xStart, yStart, xStart + xLength - 1, yStart);
}

// Draw a Vertical line
//  color: Line segment color
//  xStart/yStart: Start point
//  yLength: Line Length
inline void dwinDrawVLine(uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t yLength) {
  dwinDrawLine(color, xStart, yStart, xStart, yStart + yLength - 1);
}

// Draw a rectangle
//  mode: 0=frame, 1=fill, 2=XOR fill
//  color: Rectangle color
//  xStart/yStart: upper left point
//  xEnd/yEnd: lower right point
void dwinDrawRectangle(uint8_t mode, uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);

// Draw a box
//  mode: 0=frame, 1=fill, 2=XOR fill
//  color: Rectangle color
//  xStart/yStart: upper left point
//  xSize/ySize: box size
inline void dwinDrawBox(uint8_t mode, uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t xSize, uint16_t ySize) {
  dwinDrawRectangle(mode, color, xStart, yStart, xStart + xSize - 1, yStart + ySize - 1);
}

// Move a screen area
//  mode: 0, circle shift; 1, translation
//  dir: 0=left, 1=right, 2=up, 3=down
//  dis: Distance
//  color: Fill color
//  xStart/yStart: upper left point
//  xEnd/yEnd: bottom right point
void dwinFrameAreaMove(uint8_t mode, uint8_t dir, uint16_t dis,
                         uint16_t color, uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd);

/*---------------------------------------- Text related functions ----------------------------------------*/

// Draw a string
//  bShow: true=display background color; false=don't display background color
//  size: Font size
//  color: Character color
//  bColor: Background color
//  x/y: Upper-left coordinate of the string
//  *string: The string
//  rlimit: For draw less chars than string length use rlimit
void dwinDrawString(bool bShow, uint8_t size, uint16_t color, uint16_t bColor, uint16_t x, uint16_t y, const char * const string, uint16_t rlimit=0xFFFF);

inline void dwinDrawString(bool bShow, uint8_t size, uint16_t color, uint16_t bColor, uint16_t x, uint16_t y, FSTR_P const ftitle) {
  #ifdef __AVR__
    char ctitle[strlen_P(FTOP(ftitle)) + 1];
    strcpy_P(ctitle, FTOP(ftitle));
    dwinDrawString(bShow, size, color, bColor, x, y, ctitle);
  #else
    dwinDrawString(bShow, size, color, bColor, x, y, FTOP(ftitle));
  #endif
}

#ifndef DWIN_LCD_PROUI
// Draw a positive integer
//  bShow: true=display background color; false=don't display background color
//  zeroFill: true=zero fill; false=no zero fill
//  zeroMode: 1=leading 0 displayed as 0; 0=leading 0 displayed as a space
//  size: Font size
//  color: Character color
//  bColor: Background color
//  iNum: Number of digits
//  x/y: Upper-left coordinate
//  value: Integer value
void dwinDrawIntValue(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                          uint16_t bColor, uint8_t iNum, uint16_t x, uint16_t y, uint32_t value);

// Draw a floating point number
//  bShow: true=display background color; false=don't display background color
//  zeroFill: true=zero fill; false=no zero fill
//  zeroMode: 1=leading 0 displayed as 0; 0=leading 0 displayed as a space
//  size: Font size
//  color: Character color
//  bColor: Background color
//  iNum: Number of whole digits
//  fNum: Number of decimal digits
//  x/y: Upper-left point
//  value: Float value
void dwinDrawFloatValue(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                            uint16_t bColor, uint8_t iNum, uint8_t fNum, uint16_t x, uint16_t y, int32_t value);

// Draw a floating point number
//  value: positive unscaled float value
void dwinDrawFloatValue(uint8_t bShow, bool zeroFill, uint8_t zeroMode, uint8_t size, uint16_t color,
                            uint16_t bColor, uint8_t iNum, uint8_t fNum, uint16_t x, uint16_t y, float value);
#endif

/*---------------------------------------- Picture related functions ----------------------------------------*/

// Draw JPG and cached in #0 virtual display area
//  id: Picture ID
void dwinJPGShowAndCache(const uint8_t id);

// Draw an Icon
//  libID: Icon library ID
//  picID: Icon ID
//  x/y: Upper-left point
void dwinIconShow(uint8_t libID, uint8_t picID, uint16_t x, uint16_t y);

// Draw an Icon
//  IBD: The icon background display: 0=Background filtering is not displayed, 1=Background display \\When setting the background filtering not to display, the background must be pure black
//  BIR: Background image restoration: 0=Background image is not restored, 1=Automatically use virtual display area image for background restoration
//  BFI: Background filtering strength: 0=normal, 1=enhanced, (only valid when the icon background display=0)
//  libID: Icon library ID
//  picID: Icon ID
//  x/y: Upper-left point
void dwinIconShow(bool IBD, bool BIR, bool BFI, uint8_t libID, uint8_t picID, uint16_t x, uint16_t y);

// Draw an Icon from SRAM
//  IBD: The icon background display: 0=Background filtering is not displayed, 1=Background display \\When setting the background filtering not to display, the background must be pure black
//  BIR: Background image restoration: 0=Background image is not restored, 1=Automatically use virtual display area image for background restoration
//  BFI: Background filtering strength: 0=normal, 1=enhanced, (only valid when the icon background display=0)
//  x/y: Upper-left point
//  addr: SRAM address
void dwinIconShow(bool IBD, bool BIR, bool BFI, uint16_t x, uint16_t y, uint16_t addr);

// Unzip the JPG picture to a virtual display area
//  n: Cache index
//  id: Picture ID
void dwinJPGCacheToN(uint8_t n, uint8_t id);

// Unzip the JPG picture to virtual display area #1
//  id: Picture ID
inline void dwinJPGCacheTo1(uint8_t id) { dwinJPGCacheToN(1, id); }

// Animate a series of icons
//  animID: Animation ID  up to 16
//  animate: animation on or off
//  libID: Icon library ID
//  picIDs: Icon starting ID
//  picIDe: Icon ending ID
//  x/y: Upper-left point
//  interval: Display time interval, unit 10mS
void dwinIconAnimation(uint8_t animID, bool animate, uint8_t libID, uint8_t picIDs, uint8_t picIDe, uint16_t x, uint16_t y, uint16_t interval);

// Animation Control
//  state: 16 bits, each bit is the state of an animation id
void dwinIconAnimationControl(uint16_t state);
