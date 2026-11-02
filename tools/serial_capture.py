"""Reinicia la badge por USB (RTS) y muestra N segundos de su log serial.

Sirve en Windows, macOS y Linux. Necesita pyserial: ya viene con PlatformIO
(usa el Python de PlatformIO) o instálalo con `pip install pyserial`.

  python tools/serial_capture.py            # 10 s, puerto autodetectado
  python tools/serial_capture.py 60         # 60 s
  python tools/serial_capture.py 30 --port COM4
  python tools/serial_capture.py 30 --port /dev/ttyACM0

La badge usa el USB nativo del ESP32-S3 (VID 303A de Espressif); si hay varias
placas Espressif conectadas, indica cuál con --port.
"""
import argparse
import sys
import time

import serial
from serial.tools import list_ports

ESPRESSIF_VID = 0x303A


def find_port():
    ports = [p for p in list_ports.comports() if p.vid == ESPRESSIF_VID]
    if not ports:
        sys.exit("No encontré ninguna placa Espressif (VID 303A). ¿Está conectada con un cable de datos? "
                 "Puedes indicar el puerto con --port.")
    if len(ports) > 1:
        names = ", ".join(p.device for p in ports)
        sys.exit(f"Hay varias placas Espressif ({names}). Elige una con --port.")
    return ports[0].device


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("seconds", nargs="?", type=float, default=10, help="segundos a capturar (10)")
    ap.add_argument("--port", help="puerto serie (por defecto: autodetectar)")
    args = ap.parse_args()

    port = args.port or find_port()
    print(f"[serial_capture] puerto {port}, {args.seconds:g} s", file=sys.stderr)

    try:
        s = serial.Serial(port, 115200, timeout=0.2)
    except serial.SerialException as e:
        sys.exit(f"No pude abrir {port}: {e}\n"
                 "¿Existe ese puerto? ¿Lo tiene abierto otro programa (monitor serie de VS Code, etc.)? "
                 "En Linux, ¿tu usuario está en el grupo dialout/uucp?")
    # Pulso de reset: en el USB-Serial/JTAG del S3, RTS controla el reset del chip.
    s.dtr = False
    s.rts = True
    time.sleep(0.1)
    s.rts = False

    end = time.time() + args.seconds
    buf = b""
    while time.time() < end:
        try:
            buf += s.read(4096)
        except serial.SerialException:
            # Al reiniciar, el puerto USB desaparece un momento; reabrir.
            s.close()
            time.sleep(0.5)
            try:
                s = serial.Serial(port, 115200, timeout=0.2)
            except serial.SerialException:
                pass
    s.close()
    sys.stdout.write(buf.decode("utf-8", errors="replace"))


if __name__ == "__main__":
    main()
