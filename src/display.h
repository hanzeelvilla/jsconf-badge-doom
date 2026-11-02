// Pantalla NV3007 de la badge (428x142 en horizontal) con Arduino_GFX.
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Inicializa el bus SPI, el controlador y el backlight; deja la pantalla en negro.
void displayBegin(void);

// Dibuja un frame de Doom: 428x142 bytes (índices de paleta), del tamaño de
// la pantalla. lut565 son los 256 colores ya convertidos a RGB565.
void displayBlitDoom(const uint8_t *fb428x142, const uint16_t *lut565);

// Texto simple en pantalla (para errores). Limpia la pantalla antes.
void displayShowText(const char *text);

// Microsegundos que tardó el último displayBlitDoom.
uint32_t displayLastBlitUs(void);

#ifdef __cplusplus
}
#endif
