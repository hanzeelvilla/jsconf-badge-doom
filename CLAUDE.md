# CLAUDE.md

Proyecto: correr DOOM en la badge de JSConf MX 2026 (ESP32-S3 sin PSRAM, pantalla NV3007 428x142, 4 botones) con PlatformIO.

## Forma de trabajar
- **Explicar todo en español, a fondo y con estilo didáctico.** Es la primera vez del usuario portando Doom a un ESP32: antes y después de cada paso explica qué se hace, por qué, qué significa cada comando, flag o concepto (particiones, mmap, DMA, caché de flash, zone memory, etc.) y qué se espera ver.
- **Avisar antes de cada flasheo** a la badge y decir qué se va a escribir y en qué dirección.
- Trabajar por pasos pequeños verificables en el hardware real; no avanzar sobre algo que no se ha probado.
- No inventar datos del hardware: los pines y hallazgos están en `README.md` / `NOTES.md`. Si algo no está verificado, decirlo.

## Documentación
- `README.md`: **solo documentación del proyecto** (qué es, hardware, firmware original, tecnologías/repos/licencias, cómo compilar/flashear, cómo jugar, estado). Nada de bitácora.
- `NOTES.md`: notas de sesión, decisiones (con su porqué), mediciones, hallazgos y pendientes. Actualizarlo al cerrar cada paso.

## Alcance
- Firmware **independiente**: no se convive con Meshtastic ni se reutiliza código del firmware original (solo se consulta como referencia de hardware).
- Motor: GBADoom (GPL-2). WAD `doom1.wad` en partición propia leído con `esp_partition_mmap`. El WAD nunca se commitea.

## Comandos
- PlatformIO: `pio` está en `%USERPROFILE%\.platformio\penv\Scripts\pio.exe` (no está en PATH).
- Compilar: `pio run`
- Flashear: `pio run -t upload --upload-port COM4`
- Monitor: `pio device monitor -p COM4`
- Flashear WAD: `%USERPROFILE%\.platformio\penv\Scripts\esptool.exe --chip esp32s3 -p COM4 write-flash 0x210000 wad/doom1.wad`
- Capturar log serial (reinicia la badge): `%USERPROFILE%\.platformio\penv\Scripts\python.exe -I tools/serial_capture.py COM4 <segundos>`
- No usar VS Code / PIO Home mientras la CLI instala paquetes; no aceptar actualizaciones de PlatformIO Core desde VS Code (ver NOTES.md).
- La badge está en COM4 (USB nativo, VID 303A). Recuperación: BOOT + RESET.
