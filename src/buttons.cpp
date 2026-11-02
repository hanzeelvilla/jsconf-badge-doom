#include "buttons.h"

#include <Arduino.h>

static const uint8_t PINS[BTN_COUNT] = {40, 43, 18, 1};
static const char *NAMES[BTN_COUNT] = {"UP", "DOWN", "SELECT", "BACK"};

// Un cambio del pin solo se acepta si se mantiene estable este tiempo.
// Los contactos mecánicos "rebotan" (abren y cierran varias veces) durante
// unos pocos milisegundos al presionar o soltar.
static const uint32_t DEBOUNCE_MS = 10;

static bool rawLast[BTN_COUNT];      // última lectura cruda (true = presionado)
static uint32_t rawChangedAt[BTN_COUNT];
static uint8_t stable = 0;           // máscara de estado filtrado
static uint32_t bounces = 0;
static uint8_t rawEdgesSinceStable[BTN_COUNT];

void buttonsBegin() {
  // Todos empiezan como "sueltos". No leemos el pin aquí: justo después de
  // activar el pull-up, el pin todavía no llega a 3.3 V (la resistencia
  // interna de ~45 kOhm tarda en cargar la capacitancia de la pista) y una
  // lectura inmediata puede dar 0 falso. Si un botón sí está presionado al
  // arrancar, el debounce de buttonsUpdate() lo detecta 10 ms después.
  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    pinMode(PINS[i], INPUT_PULLUP);
    rawLast[i] = false;
    rawChangedAt[i] = millis();
  }
  stable = 0;
}

ButtonState buttonsUpdate() {
  uint32_t now = millis();
  uint8_t before = stable;

  for (uint8_t i = 0; i < BTN_COUNT; i++) {
    bool raw = digitalRead(PINS[i]) == LOW;
    if (raw != rawLast[i]) {
      rawLast[i] = raw;
      rawChangedAt[i] = now;
      rawEdgesSinceStable[i]++;
    }
    bool isStable = (stable & BTN_MASK(i)) != 0;
    if (raw != isStable && now - rawChangedAt[i] >= DEBOUNCE_MS) {
      if (raw) stable |= BTN_MASK(i);
      else stable &= ~BTN_MASK(i);
      // Un cambio limpio produce 1 flanco crudo; los demás fueron rebotes.
      if (rawEdgesSinceStable[i] > 1) bounces += rawEdgesSinceStable[i] - 1;
      rawEdgesSinceStable[i] = 0;
    }
  }

  ButtonState s;
  s.held = stable;
  s.pressed = stable & ~before;
  s.released = before & ~stable;
  return s;
}

uint32_t buttonsBouncesFiltered() { return bounces; }

const char *buttonName(uint8_t b) { return b < BTN_COUNT ? NAMES[b] : "?"; }
