/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void CamReq_Cancel(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

void PriceInfo_Close(void) {
    gTownMode = 0;
    gTownMenuOpen = 0;
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
}
