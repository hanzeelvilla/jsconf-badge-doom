#include "wad.h"

#include <Arduino.h>
#include <esp_partition.h>
#include <strings.h>

static const uint8_t *base = nullptr;
static size_t mappedSize = 0;
static const WadHeader *header = nullptr;
static const WadLump *directory = nullptr;
static esp_partition_mmap_handle_t mmapHandle;

bool wadBegin() {
  // Subtipo 0x40 y nombre "wad", tal como están en partitions_doom.csv.
  const esp_partition_t *part = esp_partition_find_first(
      ESP_PARTITION_TYPE_DATA, (esp_partition_subtype_t)0x40, "wad");
  if (!part) {
    Serial.println("WAD: no se encontró la partición 'wad'");
    return false;
  }
  Serial.printf("WAD: partición en 0x%06lx, %lu KB\n", (unsigned long)part->address,
                (unsigned long)(part->size / 1024));

  const void *ptr = nullptr;
  esp_err_t err = esp_partition_mmap(part, 0, part->size, ESP_PARTITION_MMAP_DATA, &ptr,
                                     &mmapHandle);
  if (err != ESP_OK) {
    Serial.printf("WAD: esp_partition_mmap falló: %s\n", esp_err_to_name(err));
    return false;
  }
  base = (const uint8_t *)ptr;
  mappedSize = part->size;
  Serial.printf("WAD: mapeado en la dirección %p\n", ptr);

  header = (const WadHeader *)base;
  if (memcmp(header->id, "IWAD", 4) != 0 && memcmp(header->id, "PWAD", 4) != 0) {
    Serial.printf("WAD: cabecera inválida (%02x %02x %02x %02x). ¿Se flasheó el WAD?\n",
                  base[0], base[1], base[2], base[3]);
    base = nullptr;
    return false;
  }
  size_t dirEnd = (size_t)header->infotableofs + (size_t)header->numlumps * sizeof(WadLump);
  if (header->numlumps <= 0 || dirEnd > mappedSize) {
    Serial.println("WAD: directorio fuera de la partición");
    base = nullptr;
    return false;
  }
  directory = (const WadLump *)(base + header->infotableofs);
  Serial.printf("WAD: %.4s, %ld lumps, directorio en offset %ld\n", header->id,
                (long)header->numlumps, (long)header->infotableofs);
  return true;
}

const uint8_t *wadData() { return base; }
size_t wadMappedSize() { return mappedSize; }
int wadNumLumps() { return base ? header->numlumps : 0; }
const WadLump *wadLump(int i) { return (base && i >= 0 && i < header->numlumps) ? &directory[i] : nullptr; }

int wadFind(const char *name) {
  if (!base) return -1;
  // Doom busca desde el final: si un PWAD repite un nombre, gana el último.
  for (int i = header->numlumps - 1; i >= 0; i--)
    if (strncasecmp(directory[i].name, name, 8) == 0) return i;
  return -1;
}

const uint8_t *wadLumpData(int i) {
  const WadLump *l = wadLump(i);
  return l ? base + l->filepos : nullptr;
}
