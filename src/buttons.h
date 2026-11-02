// Lectura de los 4 botones de la badge con debounce.
//
// Cada botón conecta su GPIO a GND al presionarse. Con el pull-up interno
// activado, el pin lee 1 (HIGH) en reposo y 0 (LOW) presionado: "activo en bajo".
#pragma once
#include <stdint.h>

enum Button : uint8_t {
  BTN_UP = 0,      // IO40
  BTN_DOWN = 1,    // IO43
  BTN_SELECT = 2,  // IO18
  BTN_BACK = 3,    // IO01
  BTN_COUNT = 4,
};

// Bit de cada botón dentro de las máscaras (1 << BTN_x).
#define BTN_MASK(b) (1u << (b))

struct ButtonState {
  uint8_t held;      // botones presionados ahora (estado ya filtrado)
  uint8_t pressed;   // flanco: se presionaron en esta llamada
  uint8_t released;  // flanco: se soltaron en esta llamada
};

void buttonsBegin();
// Llamar seguido (cada frame o más). Devuelve el estado filtrado.
ButtonState buttonsUpdate();
// Cambios crudos del pin que el debounce descartó (para diagnóstico).
uint32_t buttonsBouncesFiltered();
const char *buttonName(uint8_t b);
