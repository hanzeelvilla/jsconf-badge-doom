// ESP32: parte en C del port a la badge (no es de GBADoom upstream).
// Vive dentro de la librería para compilarse con las mismas opciones que el
// motor (-std=gnu2x) y poder incluir global_data.h. El resto del port está
// en src/port/i_system_esp32.cpp.
#include "doomdef.h"
#include "doomtype.h"
#include "d_event.h"
#include "d_main.h"
#include "doom_iwad.h"
#include "global_data.h"
#include "lprintf.h"
#include "w_wad.h"

// De src/port/i_system_esp32.cpp
unsigned int I_ButtonsHeld_esp32(void);
const unsigned char* I_WadData_esp32(void);
unsigned int I_WadMappedSize_esp32(void);

// Bits de I_ButtonsHeld_esp32() (src/buttons.h).
#define B_UP     0x1
#define B_DOWN   0x2
#define B_SELECT 0x4
#define B_BACK   0x8

// --------------------------------------------------------------------------
// WAD: reemplaza a source/doom_iwad.c. En vez de leer un archivo a RAM,
// doom_iwad apunta directo a la partición de flash mapeada con mmap.
// --------------------------------------------------------------------------
const unsigned char* doom_iwad = NULL;
unsigned int doom_iwad_len = 0;

void I_LoadIwad(const char* path)
{
    (void)path;
    const unsigned char* data = I_WadData_esp32();
    if(!data)
        I_Error("No hay WAD en la particion 'wad'. Flashea doom1gba.wad en 0x210000.");

    // El largo real del WAD: hasta el final del directorio o del lump más
    // lejano, lo que termine después. La partición mide más que el archivo.
    const int numlumps = *(const int*)(data + 4);
    const int infotableofs = *(const int*)(data + 8);
    unsigned int len = (unsigned int)infotableofs + (unsigned int)numlumps * 16;
    const int* dir = (const int*)(data + infotableofs);
    for(int i = 0; i < numlumps; i++)
    {
        unsigned int end = (unsigned int)dir[i * 4] + (unsigned int)dir[i * 4 + 1];
        if(end > len)
            len = end;
    }
    if(len > I_WadMappedSize_esp32())
        I_Error("El WAD (%d bytes) no cabe en la particion mapeada.", (int)len);

    doom_iwad = data;
    doom_iwad_len = len;
    lprintf("I_LoadIwad: WAD en flash, %d lumps, %d bytes", numlumps, (int)len);
}

// --------------------------------------------------------------------------
// Sonido: la badge no tiene audio. i_audio.c llama a esto al iniciar.
// --------------------------------------------------------------------------
void I_StartAudioDevice(void) {}

// --------------------------------------------------------------------------
// Controles: 4 botones -> teclas de GBADoom (KEYD_*, doomdef.h).
//
// Tres contextos:
//   Título/demo (no hay menú y no estamos jugando una partida propia):
//     Select o Back = abrir el menú (START). En GBADoom solo START abre el
//     menú (m_menu.c), y la demo corre dentro de un nivel (GS_LEVEL), así que
//     se distingue con _g->demoplayback.
//   Menú:
//     Up/Down = navegar, Select = confirmar (A), Back = atrás (B: en GBADoom
//     regresa al menú anterior, y en el principal lo cierra)
//   Jugando:
//     Up = avanzar, Back = girar izq., Down = girar der., Select = disparar (B)
//     Back+Down = usar/abrir puertas (A), Back+Select = menú (START)
//
// Cada vez se calcula el conjunto de teclas que "deberían" estar presionadas
// y se mandan eventos solo por las que cambiaron. Al cambiar de contexto
// (entrar o salir del menú) se ignoran los botones hasta soltarlos todos, para
// que el mismo botón que abrió el menú no haga otra cosa dentro de él.
// --------------------------------------------------------------------------
static unsigned int keysDown = 0;   // bit (1 << KEYD_x) por tecla presionada
static int lastContext = -1;        // CTX_* (definidos abajo)
static bool waitRelease = false;

enum { CTX_TITLE = 0, CTX_MENU = 1, CTX_GAME = 2 };

static const char* const keyNames[] = {"?", "A", "B", "L", "R", "UP", "DOWN", "LEFT", "RIGHT", "START", "SELECT"};

static unsigned int MapButtons(unsigned int held, int ctx)
{
    unsigned int k = 0;
    if(ctx == CTX_TITLE)
    {
        if(held & (B_SELECT | B_BACK)) k |= 1u << KEYD_START;
        return k;
    }
    if(ctx == CTX_MENU)
    {
        if(held & B_UP)     k |= 1u << KEYD_UP;
        if(held & B_DOWN)   k |= 1u << KEYD_DOWN;
        if(held & B_SELECT) k |= 1u << KEYD_A;
        if(held & B_BACK)   k |= 1u << KEYD_B;
        return k;
    }

    if((held & B_BACK) && (held & B_SELECT))
    {
        k |= 1u << KEYD_START;
        held &= ~(B_BACK | B_SELECT);
    }
    if((held & B_BACK) && (held & B_DOWN))
    {
        k |= 1u << KEYD_A;
        held &= ~(B_BACK | B_DOWN);
    }
    if(held & B_UP)     k |= 1u << KEYD_UP;
    if(held & B_BACK)   k |= 1u << KEYD_LEFT;
    if(held & B_DOWN)   k |= 1u << KEYD_RIGHT;
    if(held & B_SELECT) k |= 1u << KEYD_B;
    return k;
}

void I_PollInput_esp32(void)
{
    const unsigned int held = I_ButtonsHeld_esp32();
    int ctx;
    if(_g->menuactive)
        ctx = CTX_MENU;
    else if(_g->gamestate != GS_LEVEL || _g->demoplayback)
        ctx = CTX_TITLE;
    else
        ctx = CTX_GAME;

    if(ctx != lastContext)
    {
        lastContext = ctx;
        waitRelease = (held != 0);
    }
    if(waitRelease && held == 0)
        waitRelease = false;

    const unsigned int want = waitRelease ? 0 : MapButtons(held, ctx);
    const unsigned int changed = want ^ keysDown;

    for(int key = 1; key <= KEYD_SELECT; key++)
    {
        if(!(changed & (1u << key)))
            continue;
        event_t ev;
        ev.type = (want & (1u << key)) ? ev_keydown : ev_keyup;
        ev.data1 = key;
        ev.data2 = ev.data3 = 0;
        D_PostEvent(&ev);
        // Log de diagnóstico: qué tecla de GBADoom se mandó y en qué contexto.
        lprintf("[input] ctx=%s %s %s", ctx == CTX_TITLE ? "titulo" : ctx == CTX_MENU ? "menu" : "juego",
                keyNames[key], ev.type == ev_keydown ? "down" : "up");
    }
    keysDown = want;
}
