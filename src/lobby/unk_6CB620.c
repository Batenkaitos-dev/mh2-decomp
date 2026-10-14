/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"

void CamFocus_End(void);
void CamFocus_SetAside(s8 arg0);
void LobbySe_Play(s32);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

void DojoMenu_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_SetAside(0);
    CamFocus_End();
    LobbySe_Play(4);
    gFieldControl = 1;
}

s32 func_006CB680(void) {
    return 1;
}
