#include "display.h"

#include <Arduino.h>
#include <Arduino_GFX_Library.h>

// --- Pines de la pantalla (ver README) ---
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_DC 9
#define TFT_RST 10
#define TFT_CS 21
#define TFT_BL 2

#ifndef TFT_SPI_HZ
#define TFT_SPI_HZ 80000000
#endif

static const int SCREEN_W = 428;
static const int SCREEN_H = 142;

// Frame de GBADoom: 214 columnas de 2 pixeles x 142 filas = la pantalla
// completa, así que se copia 1:1 sin escalar.
static const int SRC_W = SCREEN_W;
static const int SRC_H = SCREEN_H;

static const int CHUNK_LINES = 8;

static Arduino_DataBus *bus =
    new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED, HSPI);
static Arduino_GFX *gfx = new Arduino_NV3007(bus, TFT_RST, 1 /* rotation */, false /* IPS */,
                                             142, 428, 12, 0, 14, 0,
                                             nv3007_279_init_operations,
                                             sizeof(nv3007_279_init_operations));

// Bloque de 8 líneas ya convertidas a RGB565 (alineado a 4 para escribir de
// a 2 pixeles con una sola escritura de 32 bits).
static uint16_t lineBuf[SRC_W * CHUNK_LINES] __attribute__((aligned(4)));
static uint32_t lastBlitUs = 0;

void displayBegin(void) {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  if (!gfx->begin(TFT_SPI_HZ)) Serial.println("ERROR: gfx->begin() fallo");
  gfx->fillScreen(RGB565_BLACK);
}

void displayBlitDoom(const uint8_t *fb, const uint16_t *lut565) {
  uint32_t t0 = micros();
  for (int y = 0; y < SRC_H; y += CHUNK_LINES) {
    int lines = min(CHUNK_LINES, SRC_H - y);
    const uint8_t *src = fb + y * SRC_W;
    uint32_t *out = (uint32_t *)lineBuf;
    // 2 pixeles por vuelta: el de la izquierda va en la mitad baja (el
    // ESP32-S3 es little-endian, así que queda primero en memoria).
    for (int i = 0; i < SRC_W * lines; i += 2)
      *out++ = lut565[src[i]] | ((uint32_t)lut565[src[i + 1]] << 16);
    gfx->draw16bitRGBBitmap(0, y, lineBuf, SRC_W, lines);
  }
  lastBlitUs = micros() - t0;
}

void displayShowText(const char *text) {
  gfx->fillScreen(RGB565_BLACK);
  gfx->setTextColor(RGB565_RED);
  gfx->setTextSize(1);
  gfx->setTextWrap(true);
  gfx->setCursor(2, 2);
  gfx->print(text);
}

uint32_t displayLastBlitUs(void) { return lastBlitUs; }
