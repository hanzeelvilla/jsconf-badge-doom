# CLAUDE.md

Doom (motor GBADoom) corriendo en la badge de JSConf MX 2026: ESP32-S3 sin PSRAM, pantalla NV3007 de 428x142 y 4 botones, compilado con PlatformIO. Es un firmware **independiente**: reemplaza al firmware original (Meshtastic) de la badge. Para usarlo, ver `README.md`.

## Forma de trabajar
- Explicar en español, a fondo y con estilo didáctico: qué se hace, por qué y qué se espera ver. Quien trabaja aquí suele estar aprendiendo cómo se porta Doom a un ESP32.
- **Avisar antes de cada flasheo** a la badge y decir qué se escribe y en qué dirección.
- Pasos pequeños, verificados en el hardware real. Si algo no está verificado, decirlo.
- No inventar datos del hardware: los pines y las particiones están en `README.md`.
- `README.md` es solo documentación para quien usa el proyecto. Las notas de trabajo van en `NOTES.md`, que es una bitácora **local** (está en `.gitignore`; si no existe, créala).

## Estructura
- `src/`: arranque (`main.cpp`), pantalla (`display.*`), botones (`buttons.*`), WAD mapeado de flash (`wad.*`) y la capa de plataforma de GBADoom (`port/i_system_esp32.cpp`).
- `lib/gbadoom/`: motor GBADoom vendorizado (commit `7763f5f9adf18f83a59093bc1ab786fe2c36591f`, GPL-2.0). Lo nuestro dentro del motor está en `source/esp32/` y en las secciones marcadas con `ESP32:` / `#ifdef ESP_PLATFORM`. El commit «Vendorizar GBADoom @ 7763f5f (sin cambios)» es el motor tal cual; `git diff <ese commit> HEAD -- lib/gbadoom` muestra todo lo que se le cambió.
- `tools/`: `gen_render_tables.py` (tablas del renderer), `check_wad.py` (valida el WAD convertido), `serial_capture.py` (log serie con reinicio).
- `partitions_doom.csv`: particiones (app de 2 MB en `0x10000`, WAD en `0x210000`).

## Comandos (iguales en Windows, macOS y Linux)
- Compilar: `pio run`
- Flashear el firmware: `pio run -t upload` (el puerto se detecta solo; con varias placas, `--upload-port <PUERTO>`)
- Flashear el WAD: `esptool --chip esp32s3 --port <PUERTO> write-flash 0x210000 wad/doom1gba.wad`
- Validar el WAD convertido: `python tools/check_wad.py wad/doom1gba.wad`
- Log serie reiniciando la badge: `python tools/serial_capture.py 30` (usa el Python de PlatformIO o uno con `pyserial`)
- Tablas del renderer, si cambian resolución o FOV: `python tools/gen_render_tables.py validate lib/gbadoom/source/tables.c` y luego `generate lib/gbadoom/source/tables.c lib/gbadoom/source/esp32/render_tables_esp32.c 214 142 110 160` (mover el `.h` generado a `lib/gbadoom/include/`)

## Convenciones
- Todo cambio al motor se marca con `ESP32:` (y `#ifdef ESP_PLATFORM` si el código original debe seguir intacto), para poder comparar contra upstream.
- Los WAD y los volcados de flash (`*.wad`, `*.bin`) nunca se commitean.
- El proyecto es GPL-2.0 (ver `COPYING`) por incluir GBADoom.

## Cosas que ya costaron tiempo
- **`Serial.setTxTimeoutMs(0)`** justo después de `Serial.begin()`: con el USB nativo, si nadie lee el puerto, `printf` se bloquea y el juego se congela. No imprimir por serie en el bucle de juego sin esto.
- **`lprintf` del motor solo entiende `%s` y `%d`** (no `%u`, `%x`, `%f`).
- **`esptool read-flash` con el stub se atora** (`Packet content transfer stopped`) en ciertos bloques de la flash por el USB nativo del S3. Para respaldos usar `--no-stub` (más lento, pero confiable).
- **Primer build en Windows**: no tener VS Code / PIO Home abiertos mientras PlatformIO instala pioarduino (bloquean archivos de su entorno Python) y no aceptar actualizar PlatformIO Core desde VS Code.
- **El WAD debe corresponder al motor**: `GBAVER` (15) tiene que coincidir con `DOOM_IWAD_VERSION` de `lib/gbadoom/include/doom_iwad.h`.
