// Paso 2: test de pantalla.
// Dibuja un patrón animado en un framebuffer de 8 bits (como Doom), lo convierte
// a RGB565 con una paleta y lo envía por SPI en bloques de líneas. Mide fps.
#include <Arduino.h>
#include <Arduino_GFX_Library.h>

#include "buttons.h"
#include "wad.h"

// Índices de paleta reservados para los recuadros de los botones.
// El arcoíris solo usa 0..247, así que nunca pisa estos colores.
static const uint8_t PAL_RAINBOW_N = 248;
static const uint8_t PAL_BTN_OFF = 248;   // gris oscuro: botón suelto
static const uint8_t PAL_BTN_ON = 249;    // 249..252: un color por botón

// --- Pines de la pantalla (ver README) ---
#define TFT_MOSI 11
#define TFT_SCLK 12
#define TFT_DC 9
#define TFT_RST 10
#define TFT_CS 21
#define TFT_BL 2

// Frecuencia del reloj SPI. Probaremos 40 y 80 MHz.
#ifndef TFT_SPI_HZ
#define TFT_SPI_HZ 40000000
#endif

static const int W = 428;  // ancho lógico tras la rotación 1
static const int H = 142;
static const int CHUNK_LINES = 8;  // líneas convertidas por bloque

static Arduino_DataBus *bus =
    new Arduino_ESP32SPI(TFT_DC, TFT_CS, TFT_SCLK, TFT_MOSI, GFX_NOT_DEFINED, HSPI);
// Panel físico 142x428; rotación 1 lo deja horizontal (428x142).
// Offsets de columna 12/14 porque el controlador tiene más RAM que pixeles visibles.
static Arduino_GFX *gfx = new Arduino_NV3007(bus, TFT_RST, 1 /* rotation */, false /* IPS */,
                                             142, 428, 12, 0, 14, 0,
                                             nv3007_279_init_operations,
                                             sizeof(nv3007_279_init_operations));

static uint8_t *fb;                  // framebuffer de 8 bits: 1 byte por pixel
static uint16_t palette[256];        // índice de color -> RGB565
static uint16_t lineBuf[W * CHUNK_LINES];

static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Envía el framebuffer completo a la pantalla, convirtiendo por bloques.
static void pushFrame() {
  for (int y = 0; y < H; y += CHUNK_LINES) {
    int lines = min(CHUNK_LINES, H - y);
    const uint8_t *src = fb + y * W;
    for (int i = 0; i < W * lines; i++) lineBuf[i] = palette[src[i]];
    gfx->draw16bitRGBBitmap(0, y, lineBuf, W, lines);
  }
}

// Prueba del WAD: mapearlo, listar lumps, medir lectura y cargar PLAYPAL.
static void wadTest() {
  if (!wadBegin()) {
    Serial.println("WAD: se usa la paleta arcoíris de prueba");
    return;
  }
  int n = wadNumLumps();
  Serial.println("Primeros 12 lumps:");
  for (int i = 0; i < 12 && i < n; i++) {
    const WadLump *l = wadLump(i);
    Serial.printf("  %4d  %-8.8s  %7ld bytes  @ %ld\n", i, l->name, (long)l->size, (long)l->filepos);
  }
  const char *wanted[] = {"PLAYPAL", "COLORMAP", "E1M1", "TITLEPIC", "E1M9", "E2M1"};
  for (const char *name : wanted) {
    int i = wadFind(name);
    if (i >= 0) Serial.printf("  %-8s -> lump %d, %ld bytes\n", name, i, (long)wadLump(i)->size);
    else Serial.printf("  %-8s -> no está (normal en shareware si es E2+)\n", name);
  }

  // --- Velocidad de lectura ---
  // Secuencial: copiar 1 MB por bloques de 4 KB (como cargar un lump grande).
  static uint8_t tmp[4096];
  const uint8_t *src = wadData();
  const size_t total = 1024 * 1024;
  uint32_t t0 = micros();
  for (size_t off = 0; off < total; off += sizeof(tmp)) memcpy(tmp, src + off, sizeof(tmp));
  uint32_t seqUs = micros() - t0;
  // Repetir sobre los mismos 32 KB: ya están en la caché (lectura "caliente").
  t0 = micros();
  for (int rep = 0; rep < 32; rep++)
    for (size_t off = 0; off < 32768; off += sizeof(tmp)) memcpy(tmp, src + off, sizeof(tmp));
  uint32_t hotUs = micros() - t0;
  // Aleatoria: 20000 lecturas de 4 bytes en offsets dispersos por todo el WAD
  // (parecido a cómo el renderer lee columnas de texturas).
  uint32_t seed = 12345, sum = 0;
  t0 = micros();
  for (int i = 0; i < 20000; i++) {
    seed = seed * 1103515245u + 12345u;
    size_t off = (seed >> 4) % (4196020 - 4);
    sum += src[off] | (src[off + 1] << 8) | (src[off + 2] << 16) | (src[off + 3] << 24);
  }
  uint32_t rndUs = micros() - t0;
  Serial.printf("Lectura secuencial (1 MB frío): %.2f MB/s\n", 1.0f * total / seqUs);
  Serial.printf("Lectura en caché (1 MB caliente): %.2f MB/s\n", 32.0f * 32768 / hotUs);
  Serial.printf("Lectura aleatoria: %.2f us por acceso (checksum %lu)\n", rndUs / 20000.0f,
                (unsigned long)sum);

  // --- Paleta real de Doom ---
  // PLAYPAL tiene 14 paletas de 256 colores RGB (768 bytes cada una). La 0 es la normal;
  // las demás son los tintes rojos (daño), dorados (recoger items) y verde (traje).
  int pp = wadFind("PLAYPAL");
  if (pp >= 0) {
    const uint8_t *rgb = wadLumpData(pp);
    for (int i = 0; i < PAL_RAINBOW_N; i++) palette[i] = rgb565(rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2]);
    Serial.println("Paleta: PLAYPAL del WAD cargada (colores 0..247)");
  }
  Serial.printf("Heap libre tras mapear el WAD: %u\n", ESP.getFreeHeap());
}

// 1 = si nadie lee el puerto USB, descartar el texto en vez de bloquear.
#ifndef SERIAL_NONBLOCKING
#define SERIAL_NONBLOCKING 1
#endif

void setup() {
  Serial.begin(115200);
#if SERIAL_NONBLOCKING
  Serial.setTxTimeoutMs(0);
#endif
  delay(1500);  // dar tiempo a que el monitor serial se conecte por USB
  Serial.println("\n== jsconf-badge-doom: test de pantalla ==");

  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);  // encender backlight

  if (!gfx->begin(TFT_SPI_HZ)) Serial.println("ERROR: gfx->begin() fallo");
  Serial.printf("Pantalla %dx%d, SPI %d MHz\n", gfx->width(), gfx->height(), TFT_SPI_HZ / 1000000);

  // Barras de referencia para verificar colores y bordes (2 s).
  const uint16_t bars[] = {RGB565_RED, RGB565_GREEN, RGB565_BLUE, RGB565_WHITE,
                           RGB565_BLACK, RGB565_YELLOW, RGB565_CYAN, RGB565_MAGENTA};
  for (int i = 0; i < 8; i++) gfx->fillRect(i * W / 8, 0, W / 8 + 1, H, bars[i]);
  gfx->drawRect(0, 0, W, H, RGB565_WHITE);  // borde: debe verse completo en los 4 lados
  delay(2000);

  // Paleta de prueba: arcoíris de 256 colores.
  for (int i = 0; i < 256; i++) {
    float h = i / 256.0f * 6.0f;
    int s = (int)h;
    float f = h - s;
    uint8_t q = 255 * (1 - f), t = 255 * f;
    uint8_t r, g, b;
    switch (s) {
      case 0: r = 255; g = t; b = 0; break;
      case 1: r = q; g = 255; b = 0; break;
      case 2: r = 0; g = 255; b = t; break;
      case 3: r = 0; g = q; b = 255; break;
      case 4: r = t; g = 0; b = 255; break;
      default: r = 255; g = 0; b = q; break;
    }
    palette[i] = rgb565(r, g, b);
  }
  // Colores de los botones: UP rojo, DOWN verde, SELECT azul, BACK amarillo.
  palette[PAL_BTN_OFF] = rgb565(40, 40, 40);
  palette[PAL_BTN_ON + BTN_UP] = rgb565(255, 0, 0);
  palette[PAL_BTN_ON + BTN_DOWN] = rgb565(0, 255, 0);
  palette[PAL_BTN_ON + BTN_SELECT] = rgb565(0, 80, 255);
  palette[PAL_BTN_ON + BTN_BACK] = rgb565(255, 255, 0);

  buttonsBegin();
  wadTest();

  fb = (uint8_t *)malloc(W * H);
  Serial.printf("Framebuffer: %d bytes, heap libre: %u, bloque max: %u\n", W * H,
                ESP.getFreeHeap(), ESP.getMaxAllocHeap());
}

void loop() {
  static uint32_t frame = 0, frames = 0, tRender = 0, tPush = 0, lastReport = millis();
  static uint32_t lastLoop = micros(), worstFrame = 0, lastPrintUs = 0;

  // Tiempo total entre una vuelta y la siguiente (incluye printf y todo lo demás).
  uint32_t now = micros();
  worstFrame = max(worstFrame, now - lastLoop);
  lastLoop = now;

  uint32_t t0 = micros();
  for (int y = 0; y < H; y++)
    for (int x = 0; x < W; x++) fb[y * W + x] = (x + y + frame * 3) % PAL_RAINBOW_N;

  // Botones: leer, reportar cambios y dibujar un recuadro por botón.
  ButtonState bs = buttonsUpdate();
  for (uint8_t b = 0; b < BTN_COUNT; b++) {
    if (bs.pressed & BTN_MASK(b))
      Serial.printf("[%lu ms] %s presionado (held=0x%X)\n", millis(), buttonName(b), bs.held);
    if (bs.released & BTN_MASK(b))
      Serial.printf("[%lu ms] %s soltado (rebotes filtrados total: %lu)\n", millis(),
                    buttonName(b), buttonsBouncesFiltered());

    const int size = 40, gap = 12;
    int x0 = (W - (BTN_COUNT * size + (BTN_COUNT - 1) * gap)) / 2 + b * (size + gap);
    int y0 = (H - size) / 2;
    uint8_t c = (bs.held & BTN_MASK(b)) ? PAL_BTN_ON + b : PAL_BTN_OFF;
    for (int y = y0; y < y0 + size; y++) memset(fb + y * W + x0, c, size);
  }
  uint32_t t1 = micros();
  pushFrame();
  uint32_t t2 = micros();

  tRender += t1 - t0;
  tPush += t2 - t1;
  frame++;
  frames++;

  if (millis() - lastReport >= 1000) {
    uint32_t p0 = micros();
    Serial.printf("fps: %u | render: %.2f ms | push: %.2f ms | peor frame: %.1f ms | printf previo: %.1f ms | heap: %u\n",
                  frames, tRender / 1000.0f / frames, tPush / 1000.0f / frames,
                  worstFrame / 1000.0f, lastPrintUs / 1000.0f, ESP.getFreeHeap());
    lastPrintUs = micros() - p0;
    frames = tRender = tPush = worstFrame = 0;
    lastReport = millis();
    lastLoop = micros();  // no contar el printf como "frame lento"
  }
}
