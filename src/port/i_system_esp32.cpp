// Capa de plataforma de GBADoom para la badge (ESP32-S3).
//
// GBADoom aísla todo lo que depende de la máquina detrás de las funciones de
// i_system_e32.h (el nombre viene de su primer port, a Psion EPOC32). La GBA
// las implementa en i_system_gba.cpp y el build de PC con Qt en
// i_system_e32.cpp; esta es nuestra versión.
#include <Arduino.h>
#include <esp_timer.h>
#include <stdarg.h>
#include <stdio.h>

extern "C" {
#include "doomtype.h"
#include "i_system_e32.h"
}

#include "buttons.h"
#include "display.h"
#include "wad.h"

// --- Framebuffer ---
// El motor dibuja 214 "columnas" lógicas x 142 filas (SCREENWIDTH/HEIGHT en
// doomdef.h). Cada columna 3D ocupa un unsigned short con el mismo índice de
// color en sus dos bytes (en la GBA así se escriben 2 pixeles a la vez en
// VRAM). Visto como bytes es un frame de 428x142 indexado: la pantalla
// completa. La GBA tiene dos páginas; aquí basta con una: 60,776 bytes.
static const int DOOM_COLS = 214;
static const int DOOM_ROWS = 142;
static unsigned short backbuffer[DOOM_COLS * DOOM_ROWS];

// La paleta llega en RGB15 de la GBA (5 bits por canal, rojo en los bits
// bajos). La pantalla quiere RGB565 (rojo en los bits altos, verde con 6).
static uint16_t lut565[256];

static uint32_t statFrames = 0, statBlitUs = 0, statLastMs = 0, statLastFrameUs = 0, statFrameUs = 0;

extern "C" {

// Uso de la zona de memoria de Doom (lib/gbadoom/source/z_zone.c).
int Z_UsedBytes_esp32(void);
int Z_PeakBytes_esp32(void);

// lprintf (lib/gbadoom/source/lprintf.c) escribe aquí.
void I_LogWrite_esp32(const char *s, unsigned int len) { Serial.write((const uint8_t *)s, len); }

// Para el código C del port (lib/gbadoom/source/esp32/): estado de botones
// y el WAD mapeado, que viven en módulos C++.
unsigned int I_ButtonsHeld_esp32(void) { return buttonsUpdate().held; }

const unsigned char *I_WadData_esp32(void) { return wadData(); }
unsigned int I_WadMappedSize_esp32(void) { return (unsigned int)wadMappedSize(); }

void I_InitScreen_e32() {}

void I_CreateBackBuffer_e32() { memset(backbuffer, 0, sizeof(backbuffer)); }

int I_GetVideoWidth_e32() { return DOOM_COLS; }
int I_GetVideoHeight_e32() { return DOOM_ROWS; }

unsigned short *I_GetBackBuffer() { return backbuffer; }
unsigned short *I_GetFrontBuffer() { return backbuffer; }

void I_SetPallete_e32(const unsigned short *pal) {
  for (int i = 0; i < 256; i++) {
    unsigned c = pal[i];
    unsigned r = c & 31, g = (c >> 5) & 31, b = (c >> 10) & 31;
    // Verde de 5 a 6 bits repitiendo su bit alto, como hace el hardware.
    lut565[i] = (uint16_t)((r << 11) | (((g << 1) | (g >> 4)) << 5) | b);
  }
}

void I_FinishUpdate_e32(const byte *srcBuffer, const unsigned short *pallete, const unsigned int width,
                        const unsigned int height) {
  (void)pallete;
  (void)width;
  (void)height;
  displayBlitDoom(srcBuffer, lut565);

  // Estadísticas cada 5 s: fps, tiempo de blit y tiempo total por frame.
  uint32_t now = micros();
  if (statLastFrameUs) statFrameUs += now - statLastFrameUs;
  statLastFrameUs = now;
  statFrames++;
  statBlitUs += displayLastBlitUs();
  if (millis() - statLastMs >= 5000) {
    if (statLastMs)
      Serial.printf("[stats] fps: %.1f | frame: %.1f ms | blit: %.1f ms | zona: %d KB (pico %d KB) | heap: %u\n",
                    statFrames * 1000.0f / (millis() - statLastMs), statFrameUs / 1000.0f / statFrames,
                    statBlitUs / 1000.0f / statFrames, Z_UsedBytes_esp32() / 1024, Z_PeakBytes_esp32() / 1024,
                    ESP.getFreeHeap());
    statFrames = statBlitUs = statFrameUs = 0;
    statLastMs = millis();
  }
}

// Implementado en lib/gbadoom/source/esp32/i_port_esp32.c (necesita _g).
void I_PollInput_esp32(void);
void I_ProcessKeyEvents() { I_PollInput_esp32(); }

int I_GetTime_e32(void) { return (int)(esp_timer_get_time() * 35 / 1000000); }

// I_Error puede llamarse desde cualquier parte del motor (por ejemplo, sin
// memoria). Muestra el mensaje por serial y en pantalla y detiene la tarea.
void I_Error(const char *error, ...) {
  static char msg[256];
  va_list v;
  va_start(v, error);
  vsnprintf(msg, sizeof(msg), error, v);
  va_end(v);
  Serial.printf("\n*** I_Error: %s\n", msg);
  Serial.printf("*** heap libre: %u, bloque max: %u\n", ESP.getFreeHeap(), ESP.getMaxAllocHeap());
  displayShowText(msg);
  for (;;) delay(1000);
}

void I_Quit_e32() {}

}  // extern "C"
