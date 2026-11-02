// DOOM (GBADoom) en la badge JSConf MX 2026.
//
// Arduino arranca la tarea "loopTask" de FreeRTOS en el core 1, que llama a
// setup() y luego a loop() para siempre. Aquí setup() prepara el hardware y
// entra a doom_main(), que nunca regresa: el loop de Doom corre dentro de
// esta misma tarea. Así no se gasta RAM en el stack de una tarea extra.
#include <Arduino.h>

#include "buttons.h"
#include "display.h"
#include "wad.h"

// Stack de la tarea loopTask (por defecto 8 KB). En la GBA el stack medido
// de Doom no pasa de ~5 KB; 16 KB deja margen para el printf y Arduino_GFX.
SET_LOOP_TASK_STACK_SIZE(16 * 1024);

extern "C" int doom_main(int argc, const char *const *argv);

static void logHeap(const char *where) {
  Serial.printf("[heap] %-14s libre: %6u | bloque max: %6u\n", where, ESP.getFreeHeap(),
                ESP.getMaxAllocHeap());
}

void setup() {
  Serial.begin(115200);
  // Si nadie lee el puerto USB, descartar el texto en vez de bloquear
  // (ver NOTES.md: "Pausas en la animación sin monitor serial").
  Serial.setTxTimeoutMs(0);
  delay(1500);
  Serial.println("\n== jsconf-badge-doom ==");
  logHeap("inicio");

  displayBegin();
  buttonsBegin();
  logHeap("display+botones");

  if (!wadBegin()) {
    displayShowText("No hay WAD en la particion 'wad'.\nFlashea wad/doom1gba.wad en 0x210000.");
    for (;;) delay(1000);
  }
  logHeap("WAD mapeado");

  static const char *const argv[] = {"doom", nullptr};
  doom_main(1, argv);
}

void loop() {
  // doom_main() no regresa; si lo hiciera, no hay nada más que hacer.
  delay(1000);
}
