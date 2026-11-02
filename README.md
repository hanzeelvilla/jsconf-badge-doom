# DOOM en la badge de JSConf México 2026

El episodio 1 de **DOOM** (shareware) corriendo en la badge digital de **JSConf México 2026**, diseñada por [Electronic Cats](https://github.com/ElectronicCats/jscon-2026-firmware): un ESP32-S3 **sin PSRAM**, pantalla de 428×142 y cuatro botones. Usa el motor [GBADoom](https://github.com/doomhack/GBADoom), pensado para los 256 KB de una Game Boy Advance.

- Pantalla completa de **428×142**, campo de visión de 110° y un HUD mínimo (vida, armadura, munición, llaves).
- **26–35 cuadros por segundo** en el primer nivel.
- Se juega con los **4 botones** de la badge.

> [!WARNING]
> ## Esto NO es una app para tu badge: es otro firmware que REEMPLAZA al original
>
> - **Reemplaza por completo el firmware que trae la badge** (el de Electronic Cats, basado en Meshtastic). No convive con él, no es un parche y no se puede "abrir" desde él.
> - **Vas a perder los datos que la badge tiene guardados**: tu nombre, los mensajes, los ajustes de radio y **tu colección de ajolotes** (los que desbloqueaste al encontrarte con otras personas, que no se pueden recuperar si se pierden). Tu ajolote propio sí vuelve, porque sale de la dirección MAC de la placa.
> - **Volver al firmware original NO devuelve esos datos.** Reinstalar el firmware oficial deja una badge funcional, pero sin tu historial, salvo que hayas hecho un respaldo.
> - **Antes de flashear, haz el respaldo completo de la flash** ([paso 4](#4-respalda-tu-badge-no-te-lo-saltes)). Es lo único que te permite dejar la badge exactamente como estaba.
> - **Se puede volver atrás**: el modo de descarga está en la ROM del chip (se activa con BOOT + RESET) y este proyecto no lo toca ni quema eFuses. Ver [Restaurar el firmware original](#restaurar-el-firmware-original).
> - Es un proyecto **no oficial**: no tiene relación con Electronic Cats, JSConf ni id Software. Úsalo bajo tu propio riesgo.

## Estado y límites

Funciona de principio a fin: pantalla de título, demo, menú, nueva partida, moverse, disparar, abrir puertas y pasar de nivel.

- Solo el **episodio 1 shareware** (`doom1.wad`), que es de libre distribución. Si tienes el WAD de Doom completo, en principio sirve el mismo procedimiento, pero no se ha probado.
- **Sin sonido ni música** (la badge no tiene audio).
- **No se guardan las partidas.**
- El **automapa** no tiene botón asignado.

**Probado en:** Windows 11, con PlatformIO y la plataforma [pioarduino](https://github.com/pioarduino/platform-espressif32) 55.03.311, sobre una badge con el firmware oficial v0.2.0.
**Linux y macOS: sin probar.** Las instrucciones para esos sistemas siguen la documentación oficial de PlatformIO, esptool y la badge, pero nadie las ha ejecutado todavía. Si algo falla, [abre un issue](https://github.com/hanzeelvilla/jsconf-badge-doom/issues) con tu sistema operativo y el error: ayuda a que sea cierto para todos.

## Lo que necesitas

| | Windows 10/11 | macOS | Linux |
|---|---|---|---|
| Git | [git-scm.com](https://git-scm.com/) | `xcode-select --install` | `sudo apt install git` (o el de tu distro) |
| Python 3 | [python.org](https://www.python.org/downloads/) (marca *Add to PATH*) | `brew install python` | `sudo apt install python3 python3-venv` |
| Compilador de C para Qt (solo si convertirás el WAD tú mismo) | no hace falta | Xcode Command Line Tools | `sudo apt install build-essential` |
| Permisos del puerto USB | nada | nada | tu usuario en el grupo `dialout` (`uucp` en Arch), y [reglas udev de PlatformIO](https://docs.platformio.org/en/latest/core/installation/udev-rules.html) |

Además:
- Un **cable USB-C de datos** (los que solo cargan no sirven; es la causa más común de "no aparece el puerto").
- Unos **5 GB libres** y buena conexión: la primera compilación descarga el compilador y las librerías de ESP32 (ver [paso 7](#7-compila-y-flashea-el-firmware)).
- En Linux, si ModemManager está instalado, puede "adueñarse" del puerto de la badge y estorbar al flashear. Si te pasa: `sudo systemctl stop ModemManager`.

## Paso a paso

Los comandos sirven en **bash/zsh** (Linux y macOS) y en **PowerShell** (Windows) salvo donde se indica. Donde dice `<PUERTO>`, pon el de tu badge (paso 3).

> **En Windows usa PowerShell o cmd, no Git Bash.** Git Bash es un entorno MSys/MinGW y las herramientas de ESP-IDF que PlatformIO instala se niegan a correr ahí (`MSys/Mingw is not supported`). Git sí lo puedes usar desde cualquiera de los dos.

### 1. Descarga el proyecto

```sh
git clone https://github.com/hanzeelvilla/jsconf-badge-doom.git
cd jsconf-badge-doom
```

### 2. Instala PlatformIO y esptool

Se instalan en un entorno virtual para no ensuciar tu Python (y porque Linux moderno no deja hacer `pip install` global). **esptool tiene que ser la versión 5 o mayor**: el que traen algunas distribuciones es el 4.x y falla con esta placa.

| | Crear el entorno | Activarlo (cada vez que abras una terminal nueva) |
|---|---|---|
| Linux / macOS | `python3 -m venv ~/badge-venv` | `source ~/badge-venv/bin/activate` |
| Windows (PowerShell) | `python -m venv $HOME\badge-venv` | `& $HOME\badge-venv\Scripts\Activate.ps1` |
| Windows (cmd) | `python -m venv %USERPROFILE%\badge-venv` | `%USERPROFILE%\badge-venv\Scripts\activate.bat` |

Con el entorno activado:

```sh
pip install platformio esptool
pio --version
esptool version
```

> En PowerShell, si `Activate.ps1` se queja de que "la ejecución de scripts está deshabilitada", corre una vez `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned`.

### 3. Conecta la badge y encuentra su puerto

Conéctala por USB y ejecuta:

```sh
pio device list
```

La badge aparece con `Hardware ID: USB VID:PID=303A:1001`. Su puerto se llama distinto según el sistema:

| Sistema | Cómo se ve |
|---|---|
| Windows | `COM4` (el número varía) |
| macOS | `/dev/cu.usbmodem1101` |
| Linux | `/dev/ttyACM0` |

Para no confundirte, en los siguientes comandos reemplaza `<PUERTO>` por el tuyo.

Si no aparece: prueba otro cable (que sea de datos), otro puerto USB, o pon la badge en **modo de descarga** manteniendo **BOOT**, pulsando y soltando **RESET**, y soltando **BOOT** (ver [Solución de problemas](#solución-de-problemas)).

### 4. Respalda tu badge (no te lo saltes)

Este paso lee los 8 MB de la flash y los guarda en un archivo. **No escribe nada en la badge.** Es tu única forma de recuperar exactamente lo que tenía (incluida tu colección de ajolotes).

```sh
esptool --chip esp32s3 --port <PUERTO> --no-stub read-flash 0 0x800000 respaldo-badge.bin
```

- **Tarda unos 10 minutos** (se midió: 576 segundos). La barra avanza despacio; es normal, no se ha colgado.
- Por qué `--no-stub`: con el modo rápido por defecto, `esptool` se atora a los pocos cientos de KB con `A fatal error occurred: Packet content transfer stopped` (pasa con el USB nativo de esta placa). El modo `--no-stub` es más lento pero confiable.
- Comprueba que el archivo mida **8,388,608 bytes**:
  - Linux/macOS: `ls -l respaldo-badge.bin`
  - PowerShell: `(Get-Item respaldo-badge.bin).Length`
- **Guárdalo en un lugar seguro, fuera de esta carpeta.** Git lo ignora a propósito (`*.bin`), pero cópialo a otro lado.

Para dejar la badge **exactamente como estaba** más adelante:

```sh
esptool --chip esp32s3 --port <PUERTO> write-flash 0x0 respaldo-badge.bin
```

### 5. Consigue `doom1.wad`

`doom1.wad` es el **shareware v1.9** (episodio 1): id Software permite distribuirlo sin modificar, pero este repositorio **no lo incluye**. Cualquier copia sirve si cumple **ambos** valores:

| Tamaño | MD5 |
|---|---|
| 4,196,020 bytes | `f0cefca49926d00903cf57551d901abe` |

Una fuente estable es el paquete `doom-wad-shareware` de Debian (el WAD viene dentro). En una carpeta temporal:

```sh
curl.exe -L -o doom.deb https://deb.debian.org/debian/pool/non-free/d/doom-wad-shareware/doom-wad-shareware_1.9.fixed-5_all.deb
```

(`curl.exe` con la extensión sirve en todos lados; en PowerShell, `curl` a secas es otro comando.)

> Si ese archivo ya no existe porque Debian publicó otra versión, mira el [directorio del paquete](https://deb.debian.org/debian/pool/non-free/d/doom-wad-shareware/) y usa el `.deb` que haya. En Debian/Ubuntu también puedes instalarlo con `sudo apt install doom-wad-shareware` (queda en `/usr/share/games/doom/doom1.wad`).

Un `.deb` es un archivo `ar` que contiene `data.tar.xz`. Extrae el WAD:

| Sistema | Comandos |
|---|---|
| Windows / macOS | `tar -xf doom.deb data.tar.xz` y luego `tar -xf data.tar.xz ./usr/share/games/doom/doom1.wad` |
| Linux | `ar x doom.deb data.tar.xz` y luego `tar -xf data.tar.xz ./usr/share/games/doom/doom1.wad` |

> En Windows usa **PowerShell o cmd**, que traen el `tar` de Windows (capaz de abrir `.deb`). En Git Bash el `tar` es el de GNU y no entiende ese formato.

Mueve el resultado a la carpeta `wad/` del proyecto (créala si no existe) como `wad/doom1.wad` y **verifica su MD5**:

| Sistema | Comando |
|---|---|
| Linux | `md5sum wad/doom1.wad` |
| macOS | `md5 -r wad/doom1.wad` |
| Windows (PowerShell) | `Get-FileHash -Algorithm MD5 wad\doom1.wad` |

### 6. Convierte el WAD para GBADoom

GBADoom no lee `doom1.wad` tal cual: necesita una versión reorganizada para caber en poca memoria (texturas ya compuestas, imágenes reescaladas, paleta convertida). Lo hace una herramienta del propio GBADoom, **GbaWadUtil**. Hay que usar la **misma versión del motor que lleva este proyecto**; por eso se descarga fijada a un commit, no la última:

```sh
git init tools/GBADoom
git -C tools/GBADoom remote add origin https://github.com/doomhack/GBADoom.git
git -C tools/GBADoom fetch --depth 1 origin 7763f5f9adf18f83a59093bc1ab786fe2c36591f
git -C tools/GBADoom checkout FETCH_HEAD
```

Luego, según tu sistema:

#### Windows

El repositorio de GBADoom ya trae `GbaWadUtil.exe` compilado:

```powershell
cd tools\GBADoom\GbaWadUtil
.\GbaWadUtil.exe -in ..\..\..\wad\doom1.wad -out ..\..\..\wad\doom1gba.wad
cd ..\..\..
```

Se ejecuta desde su carpeta porque busca ahí sus datos (`data\gbadoom.wad`). Windows puede mostrar un aviso de SmartScreen: es un `.exe` sin firmar de un tercero. Si prefieres no ejecutarlo, compílalo tú con las instrucciones de macOS/Linux de abajo (en Windows hace falta Qt 6 con MinGW).

#### macOS y Linux (sin probar)

GbaWadUtil solo trae `.exe` para Windows, pero su código es portable (usa Qt 6). Hay que compilarlo:

1. Instala Qt 6 con los módulos *Widgets* y *SVG*:
   - Debian/Ubuntu: `sudo apt install qt6-base-dev qt6-svg-dev build-essential`
   - Fedora: `sudo dnf install qt6-qtbase-devel qt6-qtsvg-devel gcc-c++ make`
   - macOS: `brew install qt` (el `qmake` queda en `$(brew --prefix qt)/bin/qmake`)
2. Compila en modo release (el resultado queda en `tools/GBADoom/GbaWadUtil/`, junto a su carpeta `data/`):
   ```sh
   cd tools/GBADoom/tools/GbaWadUtil
   qmake6 GbaWadUtil.pro CONFIG+=release      # en macOS: $(brew --prefix qt)/bin/qmake
   make -j4
   ```
3. Conviértelo, desde la carpeta donde quedó el programa:
   ```sh
   cd ../../GbaWadUtil
   ./GbaWadUtil -in ../../../wad/doom1.wad -out ../../../wad/doom1gba.wad
   cd ../../..
   ```

**Alternativas** si no logras compilarlo: ejecutar el `.exe` con [Wine](https://www.winehq.org/), o hacer la conversión en cualquier PC con Windows y copiar `doom1gba.wad` a tu `wad/`.

#### Verifica la conversión (todos los sistemas)

```sh
python3 tools/check_wad.py wad/doom1gba.wad      # en Windows: python tools\check_wad.py wad\doom1gba.wad
```

Debe terminar con `OK: el WAD está bien convertido para este firmware.` (4,301,004 bytes, 953 lumps, `GBAVER = 15`). Si dice que falta algo o que la versión no coincide, la conversión no sirve: revisa que usaste el commit fijado.

### 7. Compila y flashea el firmware

Con la badge conectada y el entorno del paso 2 activo:

```sh
pio run -t upload
```

- **La primera vez tarda mucho**: descarga el compilador y las librerías de ESP32 (unos **4.7 GB** en disco) y en Windows tardó unos **13 minutos**. Las siguientes veces tarda alrededor de un minuto o menos.
- PlatformIO detecta el puerto solo. Si tienes varias placas conectadas, agrega `--upload-port <PUERTO>`.
- Esto escribe el *bootloader*, la tabla de particiones y el firmware: **desde aquí la badge ya no tiene su firmware original.**
- **Windows:** cierra VS Code (y la pestaña *PIO Home*) durante la primera compilación; si no, pueden bloquear archivos y la instalación falla (ver [Solución de problemas](#solución-de-problemas)).

### 8. Flashea el WAD

El WAD va en su propia partición de flash (`wad`, en la dirección `0x210000`):

```sh
esptool --chip esp32s3 --port <PUERTO> write-flash 0x210000 wad/doom1gba.wad
```

Tarda unos 20 segundos. Solo hay que repetirlo si cambias el WAD; volver a flashear el firmware no lo borra.

### 9. Juega

Al terminar de flashear, `esptool` reinicia la badge solo. Si no arrancó, pulsa **RESET** o desconecta y vuelve a conectar el cable. Verás la pantalla de título de DOOM y, tras unos segundos, la demo; con **Select** o **Back** abres el menú.

Si en lugar de eso ves un mensaje de error en pantalla, ve a [Solución de problemas](#solución-de-problemas).

## Cómo jugar

Los 4 botones cambian de función según dónde estés.

**Título o demo:** **Select** o **Back** abren el menú.

**Menú:**

| Botón | Acción |
|---|---|
| Up / Down | Moverse entre opciones |
| Select | Confirmar |
| Back | Regresar al menú anterior (en el principal, lo cierra) |

Para empezar: **New Game** → episodio → dificultad, confirmando cada uno con **Select**.

**Jugando:**

| Botón | Acción |
|---|---|
| Up | Avanzar |
| Back | Girar a la izquierda |
| Down | Girar a la derecha |
| Select | Disparar |
| Back + Down juntos | Usar / abrir puertas |
| Back + Select juntos | Abrir el menú |

Se pueden combinar: por ejemplo, Up + Back para avanzar mientras giras.

## Restaurar el firmware original

Con la badge conectada y el entorno del paso 2 activo. Hay dos caminos:

**A. Desde tu respaldo** (recupera todo exactamente como estaba, incluida tu colección):

```sh
esptool --chip esp32s3 --port <PUERTO> write-flash 0x0 respaldo-badge.bin
```

**B. Con el firmware oficial limpio** (queda una badge funcional, **sin tus datos**):

1. Descarga `badge_jscon2026-factory.bin` de los [releases oficiales](https://github.com/ElectronicCats/jscon-2026-firmware/releases).
2. Bórrala y escribe el firmware:
   ```sh
   esptool --chip esp32s3 --port <PUERTO> erase-flash
   esptool --chip esp32s3 --port <PUERTO> write-flash 0x0 badge_jscon2026-factory.bin
   ```
   (El README oficial indica que, **sin** el `erase-flash`, el firmware conserva el nombre y la colección que hubiera en la memoria. Este proyecto no usa esa zona con el WAD estándar, así que *puede* que tus datos sigan ahí, pero **no hay ninguna garantía**: no cuentes con ello.)

Si la badge no responde en ningún momento, usa el modo de descarga: mantén **BOOT**, pulsa y suelta **RESET**, suelta **BOOT**, y repite el comando.

## Solución de problemas

| Síntoma | Qué hacer |
|---|---|
| `pio device list` no muestra la badge | Cable de solo carga (cámbialo), otro puerto USB, o modo de descarga (BOOT + RESET). En Linux, revisa los permisos (`dialout`) y que ModemManager no la haya tomado. |
| `Permission denied` / `Access is denied` al abrir el puerto | Otro programa lo tiene abierto (monitor serie, VS Code) o, en Linux, tu usuario no está en `dialout`/`uucp`. |
| `esptool` falla con `FileNotFoundError ... stub_flasher_32s3.json` | Tu `esptool` es 4.x. Instala la versión 5 en el entorno virtual (paso 2). |
| `read-flash` termina con `Packet content transfer stopped` | Usa `--no-stub` (paso 4). |
| La pantalla muestra **"No hay WAD en la particion 'wad'"** | Falta el [paso 8](#8-flashea-el-wad) (o el WAD no se escribió en `0x210000`). |
| La pantalla muestra **`IWAD version N, expected 15`** | El WAD se convirtió con otra versión de GBADoom. Rehazlo con el commit fijado del [paso 6](#6-convierte-el-wad-para-gbadoom). |
| Pantalla negra y nada más | Revisa el log: `python tools/serial_capture.py 20` (necesita `pyserial`; viene con `esptool`). Si imprime `Z_Init` y luego se reinicia, repórtalo. |
| `Z_Malloc: failed on allocation` | Se acabó la memoria del motor. Reporta el nivel donde pasó. |
| **Windows:** `ERROR: MSys/Mingw is not supported` al compilar | Estás en Git Bash. Usa PowerShell o cmd. |
| **Windows:** la primera compilación falla con `Failed to install Python dependencies` o `Acceso denegado` | VS Code o *PIO Home* tenían abiertos archivos del entorno de PlatformIO. Ciérralos, y no aceptes "actualizar PlatformIO Core" desde VS Code. Vuelve a correr `pio run`. |
| **Windows:** `ModuleNotFoundError: No module named 'urllib3'` tras una compilación interrumpida | El entorno de PlatformIO quedó a medias: bórralo (`%USERPROFILE%\.platformio\penv`) y vuelve a correr `pio run`. |

Para ver lo que imprime el juego: `pio device monitor`, o `python tools/serial_capture.py 30`, que además reinicia la badge y muestra los primeros 30 segundos (útil para ver el arranque). El juego **nunca se detiene** esperando a que alguien lea el puerto serie.

## Cómo está hecho

- **Motor:** GBADoom (un port de prBoom a la Game Boy Advance) en `lib/gbadoom/`, tal cual está en upstream salvo unos pocos cambios marcados con `ESP32:`. El commit «Vendorizar GBADoom @ 7763f5f (sin cambios)» es el original; `git diff` contra él muestra todo lo que se le tocó.
- **Datos sin copiar a RAM:** el WAD vive en una partición de flash y se lee con `esp_partition_mmap`, que lo hace aparecer como memoria de solo lectura (la MMU del ESP32-S3 se encarga). Por eso 4 MB de WAD caben en un chip con ~300 KB de RAM.
- **Memoria de Doom:** unos 160 KB para su zona, el resto para el framebuffer (60 KB), pila y sistema.
- **Pantalla completa:** el motor dibuja 214 columnas de 2 píxeles (= 428) × 142 filas, sin barra de estado. Las tablas de proyección del renderer se generan con `tools/gen_render_tables.py`, que reproduce exactamente las de GBADoom para su resolución original (así se sabe que las nuevas son correctas).
- **Pantalla:** NV3007 con [Arduino_GFX](https://github.com/moononournation/Arduino_GFX) a 80 MHz de SPI; el frame de 8 bits con paleta se convierte a RGB565 por bloques de 8 líneas.

### Particiones

| Partición | Dirección | Tamaño | Uso |
|---|---|---|---|
| nvs | `0x9000` | 20 KB | Almacenamiento clave/valor de ESP-IDF |
| otadata | `0xe000` | 8 KB | Selección de aplicación (lo escribe Arduino) |
| factory | `0x10000` | 2 MB | Firmware |
| wad | `0x210000` | 5.9 MB | `doom1gba.wad` |
| coredump | `0x7F0000` | 64 KB | Volcado si el firmware se cae |

### Hardware

| Recurso | Valor |
|---|---|
| MCU | ESP32-S3-WROOM-1-N8, dos núcleos Xtensa LX7 a 240 MHz |
| Memoria | 512 KB de SRAM (sin PSRAM), 8 MB de flash |
| Pantalla | ER-TFT2.79-1 con controlador NV3007, 428×142, SPI |
| Pines de la pantalla | MOSI 11, SCLK 12, DC 9, RST 10, CS 21, luz de fondo 2 |
| Botones | Up IO40, Down IO43, Select IO18, Back IO01 (a GND, con pull-up interno) |
| USB | Nativo del ESP32-S3 (USB-Serial/JTAG), sin chip adaptador |
| Audio | No tiene |

### Estructura del repositorio

```
src/                  arranque, pantalla, botones y WAD mapeado de flash
src/port/             capa de plataforma de GBADoom para esta badge
lib/gbadoom/          motor GBADoom (GPL-2.0); lo nuestro en source/esp32/
tools/                gen_render_tables.py, check_wad.py, serial_capture.py
partitions_doom.csv   tabla de particiones
platformio.ini        configuración de compilación
COPYING               licencia GPL-2.0
```

## Contribuir y reportar problemas

Abre un [issue](https://github.com/hanzeelvilla/jsconf-badge-doom/issues) con: tu sistema operativo, la salida de `pio --version` y `esptool version`, el comando que corriste y el error completo (o el log de `python tools/serial_capture.py 30`). Se agradece mucho que alguien con **Linux o macOS** confirme o corrija los pasos marcados como sin probar.

## Créditos y licencias

- **[GBADoom](https://github.com/doomhack/GBADoom)** de doomhack, basado en **prBoom** y en el código fuente de **DOOM** de id Software: GPL-2.0.
- **DOOM** es marca de id Software / ZeniMax. El `doom1.wad` shareware es de id Software y no se incluye en este repositorio.
- **La badge** es de [Electronic Cats](https://github.com/ElectronicCats/jscon-2026-firmware) para [JSConf México 2026](https://www.jsconf.mx/).
- [pioarduino](https://github.com/pioarduino/platform-espressif32), [Arduino-ESP32](https://github.com/espressif/arduino-esp32), [Arduino_GFX](https://github.com/moononournation/Arduino_GFX) y [esptool](https://github.com/espressif/esptool).
- [nRF52840Doom](https://github.com/next-hack/nRF52840Doom) de next-hack, como referencia de Doom en muy poca memoria.

Como este repositorio incluye el motor de GBADoom, **todo el proyecto se distribuye bajo la GPL-2.0** (ver [`COPYING`](COPYING)).
