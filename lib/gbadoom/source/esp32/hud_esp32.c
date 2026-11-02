// ESP32: HUD mínimo para jugar a pantalla completa (no es de GBADoom upstream).
//
// Sin barra de estado, la vista 3D ocupa las 142 filas. Encima se dibujan:
//   abajo a la izquierda:  vida (números grandes) y armadura (números chicos)
//   abajo a la derecha:    munición del arma actual (grandes) y llaves
// con los mismos gráficos que carga la barra de estado (st_stuff.c:
// _g->tallnum, _g->shortnum, _g->keys), sin escalar.
#include "doomdef.h"
#include "doomstat.h"
#include "d_player.h"
#include "global_data.h"
#include "info.h"
#include "r_defs.h"
#include "v_video.h"

#define HUD_MARGIN 3

// Dibuja num alineado a la derecha terminando en x (exclusivo); devuelve
// la x donde empezó.
static int HUD_DrawNumRight(int x, int y, int num, const patch_t* const* digits)
{
    if(num < 0)
        num = 0;
    const int w = digits[0]->width;
    do
    {
        x -= w;
        V_DrawPatchNoScale(x, y, digits[num % 10]);
        num /= 10;
    } while(num);
    return x;
}

void HU_DrawMiniHud_esp32(void)
{
    const player_t* p = &_g->player;
    if(!_g->tallnum[0] || !_g->shortnum[0])
        return;

    V_Set2DFullscreen();

    const int tallH = _g->tallnum[0]->height;
    const int tallW = _g->tallnum[0]->width;
    const int shortH = _g->shortnum[0]->height;
    const int y = SCREENHEIGHT - tallH - HUD_MARGIN;

    // Vida (3 dígitos) y, a su derecha, la armadura con números chicos.
    int x = HUD_MARGIN + 3 * tallW;
    HUD_DrawNumRight(x, y, p->health, _g->tallnum);
    HUD_DrawNumRight(x + 4 + 3 * _g->shortnum[0]->width, y + tallH - shortH, p->armorpoints, _g->shortnum);

    // Munición del arma lista (el puño y la motosierra no usan).
    const ammotype_t ammo = weaponinfo[p->readyweapon].ammo;
    int right = SCREENWIDTH * 2 - HUD_MARGIN;
    if(ammo != am_noammo)
        right = HUD_DrawNumRight(right, y, p->ammo[ammo], _g->tallnum) - 4;

    // Llaves que tiene el jugador, en columna a la izquierda de la munición.
    int ky = y;
    for(int i = 0; i < NUMCARDS; i++)
    {
        if(p->cards[i] && _g->keys[i])
        {
            V_DrawPatchNoScale(right - _g->keys[i]->width, ky, _g->keys[i]);
            ky += _g->keys[i]->height + 1;
        }
    }

    V_Set2DCentered();
}
