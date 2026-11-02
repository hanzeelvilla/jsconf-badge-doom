# jsconf-badge-doom

DOOM corriendo en la badge digital de **JSConf México 2026** (diseñada por Electronic Cats).

Es un firmware independiente: reemplaza por completo al firmware original de la badge (basado en Meshtastic). Se puede volver al original en cualquier momento (ver [Restaurar el firmware original](#restaurar-el-firmware-original)).

> **Estado:** **Doom es jugable.** El episodio 1 shareware corre con GBADoom: título, demo, menú y partida (moverse, disparar, abrir puertas), a 27–35 fps en E1M1. No tiene sonido porque la badge no tiene audio.
> Las notas de trabajo, decisiones y mediciones están en [NOTES.md](NOTES.md).

## Hardware

| Recurso           | Valor                                                               |
| ----------------- | ------------------------------------------------------------------- |
| MCU               | ESP32-S3-WROOM-1-N8, dual core Xtensa LX7 a 240 MHz                 |
| RAM               | 512 KB SRAM, sin PSRAM                                              |
| Flash             | 8 MB                                                                |
| Pantalla          | ER-TFT2.79-1, controlador NV3007, 428x142 (horizontal), SPI         |
| Pines de pantalla | MOSI 11, SCLK 12, DC 9, RST 10, CS 21, backlight 2                  |
| Botones           | Up IO40, Down IO43, Select IO18, Back IO01 (a GND, pull-up interno) |
| Audio             | No tiene                                                            |
| USB               | Nativo (USB-Serial/JTAG del ESP32-S3)                               |

## Tecnologías y proyectos usados

| Qué                                                                         | Para qué                                                | Licencia                       |
| --------------------------------------------------------------------------- | ------------------------------------------------------- | ------------------------------ |
| [PlatformIO](https://platformio.org/)                                       | Sistema de build y flasheo                              | Apache-2.0                     |
| [Arduino-ESP32](https://github.com/espressif/arduino-esp32) (sobre ESP-IDF) | Framework                                               | LGPL-2.1 / Apache-2.0          |
| [Arduino_GFX](https://github.com/moononournation/Arduino_GFX) v1.6.5        | Driver de la pantalla NV3007                            | BSD                            |
| [GBADoom](https://github.com/doomhack/GBADoom) @ `7763f5f`                  | Motor de Doom de bajo consumo de RAM (basado en prBoom) | GPL-2.0                        |
| GbaWadUtil (incluido en GBADoom)                                            | Convierte `doom1.wad` al formato que lee GBADoom        | GPL-2.0                        |
| `doom1.wad` shareware (id Software)                                         | Datos del juego (episodio 1)                            | Shareware, libre de distribuir |

Referencias:

- Firmware original de la badge: https://github.com/ElectronicCats/jscon-2026-firmware
- [nRF52840Doom](https://github.com/next-hack/nRF52840Doom): Doom en 256 KB de RAM, usado como referencia de optimizaciones.

## Requisitos

- [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation/index.html) (CLI) o la extensión de VS Code.
- Cable USB-C de datos.
- `doom1.wad` (shareware).

## Compilar y flashear

```sh
# Compilar
pio run

# Flashear el firmware (ajusta el puerto)
pio run -t upload --upload-port COM4

# Ver el log serial
pio device monitor -p COM4
```

La plataforma es [pioarduino](https://github.com/pioarduino/platform-espressif32) 55.03.311 (Arduino-ESP32 3.3.x / ESP-IDF 5.5). PlatformIO la descarga sola en el primer build, que tarda unos 13 minutos. Cierra VS Code / PIO Home durante ese primer build (ver `NOTES.md`).

### Obtener el WAD

`doom1.wad` es el shareware v1.9 (episodio 1, libre de distribuir). Una fuente estable es el paquete de Debian [`doom-wad-shareware`](https://packages.debian.org/sid/doom-wad-shareware); el WAD está dentro, en `usr/share/games/doom/doom1.wad`. Antes de usarlo, verifica que sea el correcto:

|        | Valor esperado                     |
| ------ | ---------------------------------- |
| Tamaño | 4,196,020 bytes                    |
| MD5    | `f0cefca49926d00903cf57551d901abe` |

Colócalo en `wad/doom1.wad`. Git lo ignora.

### Convertir el WAD para GBADoom

GBADoom no lee `doom1.wad` directamente. `GbaWadUtil` lo convierte a su formato: precompone las texturas en columnas, reescala las imágenes de pantalla completa a 240x160, convierte la paleta y aplica las tablas de DeHackEd. GBADoom trae `GbaWadUtil.exe` ya compilado para Windows, en la carpeta [`GbaWadUtil/`](https://github.com/doomhack/GBADoom/tree/master/GbaWadUtil) de su repo. Copia esa carpeta completa a `tools/GbaWadUtil/` (git la ignora) y corre:

```sh
cd tools/GbaWadUtil
./GbaWadUtil.exe -in ../../wad/doom1.wad -out ../../wad/doom1gba.wad
```

Se ejecuta desde su carpeta porque busca ahí `data/gbadoom.wad`. No se pasa `-gus` (banco de instrumentos de música), porque no hay audio. El resultado debe medir 4,301,004 bytes, tener 953 lumps y traer `GBAVER` = 15.

### Flashear el WAD

El WAD convertido va en su propia partición de flash (`wad`, en `0x210000`; ver `partitions_doom.csv`). Se usa el `esptool` que instala pioarduino:

```sh
%USERPROFILE%\.platformio\penv\Scripts\esptool.exe --chip esp32s3 -p COM4 write-flash 0x210000 wad/doom1gba.wad
```

Tarda unos 20 s. Solo hace falta repetirlo si cambias el WAD o la tabla de particiones; flashear el firmware no lo borra.

### Particiones

| Partición | Offset   | Tamaño | Uso                                                     |
| --------- | -------- | ------ | ------------------------------------------------------- |
| nvs       | 0x9000   | 20 KB  | Almacenamiento clave/valor de ESP-IDF                   |
| otadata   | 0xe000   | 8 KB   | Selección de app (Arduino escribe aquí `boot_app0.bin`) |
| factory   | 0x10000  | 2 MB   | Firmware                                                |
| wad       | 0x210000 | 5.9 MB | `doom1gba.wad`, leído con `esp_partition_mmap`          |
| coredump  | 0x7F0000 | 64 KB  | Volcado si el firmware se cae                           |

### Si la badge no responde

Mantén **BOOT**, pulsa y suelta **RESET**, suelta **BOOT**. La badge entra en modo descarga y se puede volver a flashear.

## Cómo jugar

Los 4 botones cambian de función según dónde estés.

**Título / demo:** **Select** o **Back** abren el menú.

**Menú:**

| Botón     | Acción                                                  |
| --------- | ------------------------------------------------------- |
| Up / Down | Moverse entre opciones                                  |
| Select    | Confirmar                                               |
| Back      | Regresar al menú anterior (en el principal, lo cierra) |

Para empezar: **New Game** → episodio → dificultad, confirmando cada uno con **Select**.

**Jugando:**

| Botón              | Acción                  |
| ------------------ | ----------------------- |
| Up                 | Avanzar                 |
| Back               | Girar a la izquierda    |
| Down               | Girar a la derecha      |
| Select             | Disparar                |
| Back + Down juntos | Usar / abrir puertas    |
| Back + Select      | Abrir el menú           |

Se pueden combinar: por ejemplo, Up + Back para avanzar mientras giras.

## Restaurar el firmware original

1. Descarga `badge_jscon2026-factory.bin` de los [releases oficiales](https://github.com/ElectronicCats/jscon-2026-firmware/releases).
2. Flashéalo en la dirección 0:

```sh
%USERPROFILE%\.platformio\penv\Scripts\esptool.exe --chip esp32s3 -p COM4 write-flash 0x0 badge_jscon2026-factory.bin
```

## Licencia

Este proyecto incluye el motor de GBADoom/prBoom (`lib/gbadoom/`), así que se distribuye bajo la **GPL-2.0** (ver [`COPYING`](COPYING)). Los cambios al motor están marcados con `ESP32:`. Los datos de `doom1.wad` son de id Software y no se incluyen en este repositorio.
