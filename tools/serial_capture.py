"""Reinicia la badge por USB (RTS) y captura N segundos del log serial."""
import sys
import time

import serial

port = sys.argv[1] if len(sys.argv) > 1 else "COM4"
seconds = float(sys.argv[2]) if len(sys.argv) > 2 else 10

s = serial.Serial(port, 115200, timeout=0.2)
# Pulso de reset: en el USB-Serial/JTAG del S3, RTS controla el reset del chip.
s.dtr = False
s.rts = True
time.sleep(0.1)
s.rts = False

end = time.time() + seconds
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
