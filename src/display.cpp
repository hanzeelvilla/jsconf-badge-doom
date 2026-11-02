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

// Frame de GBADoom y su tamaño en pantalla (misma proporción 3:2).
static const int SRC_W = 240;
static const int SRC_H = 160;
static const int DST_W = 213;
static const int DST_H = 142;
static const int DST_X = (SCREEN_W - DST_W) / 2;  // 107: barras negras a los lados

static const int CHUNK_LINES = 8;

static Arduino_DataBus *bus =
    new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED, HSPI);
static Arduino_GFX *gfx = new Arduino_NV3007(bus, TFT_RST, 1 /* rotation */, false /* IPS */,
                                             142, 428, 12, 0, 14, 0,
                                             nv3007_279_init_operations,
                                             sizeof(nv3007_279_init_operations));

// Tablas de escalado por "vecino más cercano": para cada pixel de destino,
// qué pixel de origen copiar. Se calculan una vez para no dividir por pixel.
static uint8_t xmap[DST_W];
static uint8_t ymap[DST_H];
static uint16_t lineBuf[DST_W * CHUNK_LINES];
static uint32_t lastBlitUs = 0;

void displayBegin(void) {
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);
  if (!gfx->begin(TFT_SPI_HZ)) Serial.println("ERROR: gfx->begin() fallo");
  gfx->fillScreen(RGB565_BLACK);
  for (int x = 0; x < DST_W; x++) xmap[x] = (uint8_t)(x * SRC_W / DST_W);
  for (int y = 0; y < DST_H; y++) ymap[y] = (uint8_t)(y * SRC_H / DST_H);
}

void displayBlitDoom(const uint8_t *fb, const uint16_t *lut565) {
  uint32_t t0 = micros();
  for (int y = 0; y < DST_H; y += CHUNK_LINES) {
    int lines = min(CHUNK_LINES, DST_H - y);
    uint16_t *out = lineBuf;
    for (int l = 0; l < lines; l++) {
      const uint8_t *src = fb + ymap[y + l] * SRC_W;
      for (int x = 0; x < DST_W; x++) *out++ = lut565[src[xmap[x]]];
    }
    gfx->draw16bitRGBBitmap(DST_X, y, lineBuf, DST_W, lines);
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
