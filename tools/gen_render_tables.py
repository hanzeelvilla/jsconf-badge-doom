"""Genera las tablas precalculadas del renderer de GBADoom para otra resolución/FOV.

GBADoom trae en source/tables.c tablas calculadas para 120 columnas x 128 filas
de vista, FOV de 90 grados y projectiony 128. Este script reproduce los cálculos
de Doom (R_InitTextureMapping, R_ExecuteSetViewSize, R_InitLightTables) con la
misma aritmética de punto fijo 16.16, para poder generarlas con otros valores.

Uso:
  python -I tools/gen_render_tables.py validate <tables.c>
      Regenera las tablas de GBADoom (120x128, 90 grados, py 128) y las compara
      una por una contra las de tables.c. Tienen que salir idénticas.
  python -I tools/gen_render_tables.py generate <tables.c> <salida.c> W H FOV PY
      W = columnas, H = filas de vista, FOV en grados, PY = projectiony.
"""
import math
import re
import sys

FRACBITS = 16
FRACUNIT = 1 << FRACBITS
FINEANGLES = 8192
ANGLETOFINESHIFT = 19
ANG90 = 0x40000000
LIGHTLEVELS = 16
NUMCOLORMAPS = 32
NUMLIGHTSCALE = 160
MAXLIGHTZ = 128
LIGHTZSHIFT = 20


def trunc_div(a, b):
    """División entera que trunca hacia cero, como en C."""
    q = abs(a) // abs(b)
    return q if (a >= 0) == (b >= 0) else -q


def fixed_mul(a, b):
    return (a * b) >> FRACBITS  # C: (int64)a*b >> 16 (desplazamiento aritmético)


def fixed_div(a, b):
    if (abs(a) >> 14) >= abs(b):
        return -0x80000000 if (a ^ b) < 0 else 0x7FFFFFFF
    return trunc_div(a << FRACBITS, b)


def parse_array(src, name):
    """Lee los números de 'const <tipo> name[...] = { ... };' de tables.c."""
    m = re.search(r"\b" + re.escape(name) + r"\s*(\[[^\]]*\])+\s*=\s*\{", src)
    if not m:
        raise SystemExit(f"no encontré {name} en tables.c")
    end = src.index("};", m.end())
    body = re.sub(r"//[^\n]*|/\*.*?\*/", "", src[m.end():end], flags=re.S)
    return [int(t, 0) for t in re.findall(r"-?0[xX][0-9a-fA-F]+|-?\d+", body)]


def u32(v):
    return v & 0xFFFFFFFF


def make_tables(finetangent, finesine, width, viewheight, fov_deg, projectiony_int):
    finecosine = finesine[FINEANGLES // 4:]
    centerx = width // 2
    centerxfrac = centerx << FRACBITS
    fieldofview = 2 * round(fov_deg / 360 * FINEANGLES / 2)  # en ángulos finos, par

    # R_InitTextureMapping: ángulo de vista -> columna, y columna -> ángulo.
    focallength = fixed_div(centerxfrac, finetangent[FINEANGLES // 4 + fieldofview // 2])
    viewangletox = []
    for i in range(FINEANGLES // 2):
        t = finetangent[i]
        if t > FRACUNIT * 2:
            t = -1
        elif t < -FRACUNIT * 2:
            t = width + 1
        else:
            t = fixed_mul(t, focallength)
            t = (centerxfrac - t + FRACUNIT - 1) >> FRACBITS
            t = max(-1, min(width + 1, t))
        viewangletox.append(t)
    # GBADoom recorta -1 / width+1 ANTES de construir xtoviewangle (vanilla lo
    # hace después); solo cambia la última entrada, xtoviewangle[width].
    viewangletox = [0 if t == -1 else width if t == width + 1 else t for t in viewangletox]
    xtoviewangle = []
    for x in range(width + 1):
        i = 0
        while viewangletox[i] > x:
            i += 1
        xtoviewangle.append(u32((i << ANGLETOFINESHIFT) - ANG90))

    # R_ExecuteSetViewSize: pendiente por fila (pisos/techos) y corrección por columna.
    projectiony = projectiony_int << FRACBITS
    yslope = []
    for i in range(viewheight):
        dy = ((i - viewheight // 2) << FRACBITS) + FRACUNIT // 2
        # GBADoom redondea al más cercano (calculado en punto flotante).
        yslope.append(int(math.floor(projectiony * FRACUNIT / abs(dy) + 0.5)))
    distscale = []
    for x in range(width):
        cosadj = abs(finecosine[xtoviewangle[x] >> ANGLETOFINESHIFT])
        distscale.append(fixed_div(FRACUNIT, cosadj))

    # Luz por distancia (ver comentarios en tables.c).
    scalelight, zlight = [], []
    for i in range(LIGHTLEVELS):
        startmap = (LIGHTLEVELS - 1 - i) * 2 * NUMCOLORMAPS // LIGHTLEVELS
        row = []
        for k in range(NUMLIGHTSCALE):
            j = min(k * 40 // projectiony_int, 47)
            row.append(max(0, min(NUMCOLORMAPS - 1, startmap - j // 2)))
        scalelight.append(row)
        row = []
        for j in range(MAXLIGHTZ):
            scale = fixed_div(160 * FRACUNIT, (j + 1) << LIGHTZSHIFT) >> 12
            row.append(max(0, min(NUMCOLORMAPS - 1, startmap - scale // 2)))
        zlight.append(row)

    # Distancia focal exacta (sin cuantizar por la tabla de tangentes):
    # projection (escala X de sprites) e iprojection (paso de pisos/techos).
    # Con 90 grados dan centerxfrac y 65536/centerx, como en GBADoom.
    half = math.pi * (fieldofview / 2) / (FINEANGLES / 2)
    focal_exact = centerx / math.tan(half)
    projection = int(round(focal_exact * FRACUNIT))
    iprojection = int(FRACUNIT / focal_exact)
    return {
        "projection": projection,
        "viewangletox": viewangletox,
        "xtoviewangle": xtoviewangle,
        "yslope": yslope,
        "distscale": distscale,
        "scalelight": [v for r in scalelight for v in r],
        "zlight": [v for r in zlight for v in r],
        "clipangle": xtoviewangle[0],
        "focallength": focallength,
        "iprojection": iprojection,
        "fieldofview": fieldofview,
    }


def load(path):
    src = open(path, encoding="latin-1").read()
    return src, parse_array(src, "finetangent"), parse_array(src, "finesine")


def validate(path):
    src, ft, fs = load(path)
    t = make_tables(ft, fs, 120, 128, 90, 128)
    ok = True
    for name in ["viewangletox", "xtoviewangle", "yslope", "distscale", "scalelight", "zlight"]:
        ref = parse_array(src, name)
        ref = [u32(v) for v in ref] if name == "xtoviewangle" else ref
        got = t[name]
        diffs = [i for i in range(max(len(ref), len(got))) if i >= len(ref) or i >= len(got) or ref[i] != got[i]]
        status = "IDENTICA" if not diffs else f"{len(diffs)} DIFERENCIAS (primera en {diffs[0]})"
        print(f"  {name:13s} {len(got):5d} valores: {status}")
        if diffs:
            ok = False
            i = diffs[0]
            print(f"      ref={ref[i] if i < len(ref) else None} generado={got[i] if i < len(got) else None}")
    print(f"  clipangle     {t['clipangle']} (GBADoom: 537395200) "
          f"{'IDENTICO' if t['clipangle'] == 537395200 else 'DIFERENTE'}")
    print(f"  iprojection   {t['iprojection']} (GBADoom: 1092) "
          f"{'IDENTICO' if t['iprojection'] == 1092 else 'DIFERENTE'}")
    print(f"  projection    {t['projection']} (GBADoom: centerxfrac = {60 << FRACBITS}) "
          f"{'IDENTICO' if t['projection'] == 60 << FRACBITS else 'DIFERENTE'}")
    return (ok and t["clipangle"] == 537395200 and t["iprojection"] == 1092
            and t["projection"] == 60 << FRACBITS)


def c_array(ctype, name, dims, values, per_line=12):
    lines = []
    for i in range(0, len(values), per_line):
        lines.append("    " + ",".join(str(v) for v in values[i:i + per_line]) + ",")
    return f"const {ctype} {name}{dims} =\n{{\n" + "\n".join(lines) + "\n};\n"


def generate(path, out, width, viewheight, fov, py):
    _, ft, fs = load(path)
    t = make_tables(ft, fs, width, viewheight, fov, py)
    focal_cols = t["focallength"] / FRACUNIT
    vfov = 2 * math.degrees(math.atan((viewheight / 2) / py))
    with open(out, "w", encoding="utf-8", newline="\n") as f:
        f.write("// ESP32: GENERADO por tools/gen_render_tables.py, no editar a mano.\n")
        f.write(f"// Vista {width} columnas x {viewheight} filas, FOV horizontal {fov} grados\n")
        f.write(f"// (FIELDOFVIEW {t['fieldofview']} angulos finos), projectiony {py}.\n")
        f.write(f"// Distancia focal {focal_cols:.2f} columnas; FOV vertical ~{vfov:.1f} grados.\n")
        f.write("// Reemplaza a las tablas de source/tables.c (calculadas para 120x128, 90 grados).\n")
        f.write("#ifdef ESP_PLATFORM\n\n#include \"tables.h\"\n\n")
        f.write(c_array("int", "viewangletox", "[4096]", t["viewangletox"]) + "\n")
        f.write(c_array("angle_t", "xtoviewangle", f"[{width + 1}]", t["xtoviewangle"], 8) + "\n")
        f.write(c_array("fixed_t", "yslope", f"[{viewheight}]", t["yslope"], 10) + "\n")
        f.write(c_array("fixed_t", "distscale", f"[{width}]", t["distscale"], 10) + "\n")
        f.write(c_array("unsigned char", "scalelight", f"[{LIGHTLEVELS}][{NUMLIGHTSCALE}]", t["scalelight"], 32) + "\n")
        f.write(c_array("unsigned char", "zlight", f"[{LIGHTLEVELS}][{MAXLIGHTZ}]", t["zlight"], 32) + "\n")
        f.write("#endif\n")
    hdr = out[:-2] + ".h"
    with open(hdr, "w", encoding="utf-8", newline="\n") as f:
        f.write("// ESP32: GENERADO por tools/gen_render_tables.py, no editar a mano.\n")
        f.write("// Constantes del renderer que van con render_tables_esp32.c.\n")
        f.write("#ifndef RENDER_TABLES_ESP32_H\n#define RENDER_TABLES_ESP32_H\n\n")
        f.write(f"#define ESP32_VIEW_WIDTH   {width}\n")
        f.write(f"#define ESP32_VIEW_HEIGHT  {viewheight}\n")
        f.write(f"#define ESP32_FIELDOFVIEW  {t['fieldofview']}   // {fov} grados\n")
        f.write(f"#define ESP32_PROJECTIONY  {py}\n")
        f.write(f"#define ESP32_PROJECTION   {t['projection']}   // distancia focal X, 16.16 ({focal_cols:.2f} cols)\n")
        f.write(f"#define ESP32_IPROJECTION  {t['iprojection']}\n")
        f.write(f"#define ESP32_CLIPANGLE    {t['clipangle']}u  // xtoviewangle[0]\n")
        f.write("\n#endif\n")
    print(f"Escrito {out} y {hdr}")
    print(f"  projection {t['projection']} ({t['projection'] / FRACUNIT:.3f})")
    print(f"  FIELDOFVIEW {t['fieldofview']} | focal {focal_cols:.3f} cols ({t['focallength']} fijo)")
    print(f"  clipangle {t['clipangle']} | iprojection {t['iprojection']} | FOV vertical ~{vfov:.1f} grados")


if __name__ == "__main__":
    if len(sys.argv) >= 3 and sys.argv[1] == "validate":
        sys.exit(0 if validate(sys.argv[2]) else 1)
    elif len(sys.argv) == 8 and sys.argv[1] == "generate":
        generate(sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5]), float(sys.argv[6]), int(sys.argv[7]))
    else:
        print(__doc__)
        sys.exit(2)
