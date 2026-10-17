#include "common.h"
#include "scene.h"
#include "lobby_vobj.h"
#include "area.h"

extern s32 gTownMode;
extern s32 gTownMenuOpen;
u16 LobbyPad_Get(s32);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamFocus_End(void);
void lobby_entry(void)
{
    if (LobbyPad_Get(0) & 0x40) {
        gTownMode = 0;
        gTownMenuOpen = 0;
        LobbyMenu_SetDraw(0, 0, 0, 0);
        CamFocus_End();
    }
}

s32 LobbyWorld_GetSeason(void);
s32 Game_IsOnline(void);
s32 Wanted_IsSet(s16 *v);
s8 TownLists_Count(void)
{
    TownQuestLists *t = &gTownQuestLists;
    s32 i = LobbyWorld_GetSeason();
    s8 j;
    s8 n = 0;

    if (gScene.area != AREA_HUB_0x88) {
        for (j = 0; j < 3; j++) {
            if (Wanted_IsSet(t->a[i][j].v) == 1) {
                n++;
            }
        }
    }
    if (gScene.area != AREA_HUB_0x86 && Game_IsOnline() != 0) {
        for (j = 0; j < 3; j++) {
            if (Wanted_IsSet(t->b[i][j].v) == 1) {
                n++;
            }
        }
    }
    return n;
}

s32 Wanted_IsSet(s16 *p)
{
    if (p[0] == 0 || p[1] == 0 || p[2] == 0) {
        return 0;
    }
    return 1;
}
