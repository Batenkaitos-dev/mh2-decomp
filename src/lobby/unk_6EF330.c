/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern s8 gMenuInputLock;
extern s8 gLobbyLevel1Sel;

void AreaBoard_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    gMenuInputLock = 3;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gLobbyLevel1Sel = 0;
}
