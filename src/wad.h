// Acceso al WAD de Doom guardado en la partición de flash "wad".
//
// En vez de copiar el WAD (4 MB) a RAM (que no tenemos), se "mapea" la
// partición: la MMU del ESP32-S3 hace que la flash aparezca como memoria de
// solo lectura en el espacio de direcciones de datos. Leer un puntero ahí hace
// que el hardware traiga los bytes de la flash por la caché, de forma
// transparente. Así el WAD se lee como si fuera un arreglo en memoria.
#pragma once
#include <stddef.h>
#include <stdint.h>

// Formato del WAD (little-endian, igual que el ESP32-S3).
struct WadHeader {
  char id[4];             // "IWAD" (juego completo) o "PWAD" (parche/mod)
  int32_t numlumps;       // cuántas entradas tiene el directorio
  int32_t infotableofs;   // offset del directorio dentro del archivo
};

struct WadLump {
  int32_t filepos;        // offset de los datos del lump
  int32_t size;           // tamaño en bytes
  char name[8];           // nombre, relleno con '\0' (no necesariamente terminado)
};

// Mapea la partición y valida la cabecera. Devuelve false si no hay WAD válido.
bool wadBegin();
const uint8_t *wadData();         // inicio del WAD mapeado (nullptr si no hay)
size_t wadMappedSize();
int wadNumLumps();
const WadLump *wadLump(int i);
// Busca un lump por nombre (sin distinguir mayúsculas). -1 si no existe.
int wadFind(const char *name);
const uint8_t *wadLumpData(int i);
