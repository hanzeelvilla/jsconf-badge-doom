"""Comprueba que un WAD haya sido convertido bien por GbaWadUtil para este proyecto.

Solo usa la biblioteca estándar, así que sirve igual en Windows, macOS y Linux.

  python3 tools/check_wad.py wad/doom1gba.wad

Sale con código 0 si todo está bien y 1 si algo falla (y dice qué).

Valores esperados para el shareware v1.9 convertido con el GBADoom fijado en
este repo (commit 7763f5f): 4,301,004 bytes, 953 lumps, GBAVER = 15. Si cambias
de WAD (por ejemplo el de Doom completo), usa --any para saltarte los valores
exactos y comprobar solo la estructura.
"""
import struct
import sys

EXPECT_SIZE = 4301004
EXPECT_LUMPS = 953
EXPECT_VERSION = 15  # DOOM_IWAD_VERSION en lib/gbadoom/include/doom_iwad.h


def check(path, strict):
    errors = []
    data = open(path, "rb").read()
    if len(data) < 12:
        return ["el archivo es demasiado pequeño para ser un WAD"]
    if data[:4] != b"IWAD":
        return [f"no empieza con 'IWAD' sino con {data[:4]!r}. ¿Es el WAD original sin convertir "
                "o el archivo equivocado?"]
    numlumps, dirofs = struct.unpack("<ii", data[4:12])
    if numlumps <= 0 or dirofs < 12 or dirofs + numlumps * 16 > len(data):
        return [f"directorio inválido (lumps={numlumps}, offset={dirofs}, tamaño={len(data)})"]

    lumps = {}
    unaligned = 0
    for i in range(numlumps):
        pos, size, raw = struct.unpack("<ii8s", data[dirofs + i * 16:dirofs + i * 16 + 16])
        name = raw.split(b"\0")[0].decode("latin1")
        lumps[name] = (pos, size)
        if size and pos % 4:
            unaligned += 1

    print(f"  {path}: {len(data):,} bytes, {numlumps} lumps")
    if "GBAVER" not in lumps:
        errors.append("falta el lump GBAVER: este WAD no pasó por GbaWadUtil (es el WAD original)")
    else:
        pos, size = lumps["GBAVER"]
        version = struct.unpack("<i", data[pos:pos + 4])[0]
        print(f"  GBAVER = {version}")
        if version != EXPECT_VERSION:
            errors.append(f"GBAVER = {version}, pero este firmware espera {EXPECT_VERSION}. "
                          "Reconvierte con el GBADoom del commit fijado en el README.")
    for needed in ("GBAPAL", "COLPOOL", "TEXCOLS", "GBASTATE", "TITLEPIC"):
        if needed not in lumps:
            errors.append(f"falta el lump {needed}")
    if unaligned:
        errors.append(f"{unaligned} lumps no están alineados a 4 bytes (el ESP32-S3 los necesita así)")
    if strict:
        if len(data) != EXPECT_SIZE:
            errors.append(f"mide {len(data):,} bytes; el shareware convertido debería medir {EXPECT_SIZE:,}")
        if numlumps != EXPECT_LUMPS:
            errors.append(f"tiene {numlumps} lumps; se esperaban {EXPECT_LUMPS}")
    return errors


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if len(args) != 1:
        print(__doc__)
        return 2
    try:
        errors = check(args[0], strict="--any" not in sys.argv)
    except FileNotFoundError:
        print(f"No existe {args[0]}")
        return 1
    if errors:
        print("MAL:")
        for e in errors:
            print(f"  - {e}")
        return 1
    print("OK: el WAD está bien convertido para este firmware.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
