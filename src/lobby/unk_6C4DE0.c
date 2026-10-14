/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void CamFocus_End(void);
void CamReq_Cancel(void);
void StageModels_Free(void);
void Chat_SetColor(s16);
void LobbySe_Play(s32);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

void Workshop_Close(void) {
    gTownMenuOpen = 0;
    gTownMode = 0;
    CamFocus_End();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamReq_Cancel();
    StageModels_Free();
    Chat_SetColor(-1);
    LobbySe_Play(4);
}
