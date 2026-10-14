/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_menu.h"

void Gallery_ClearShots(void);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Chat_HideWindows(void);
void *memset(void *dst, s32 c, u32 n);
void Gallery_InitPrizes(void);
void Gallery_ResetWork(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern MenuWork gMenuWork;
extern M2C_UNK Gallery_DrawShooting;

void Gallery_Open(void) {
    gTownMenuOpen = 1;
    gTownMode = 0x2F;
    memset(&gMenuWork, 0, 0xF00);
    gFieldControl = 0;
    Chat_HideWindows();
    LobbyMenu_Init();
    LobbyMenu_SetDraw(&Gallery_DrawShooting, 0, 0, 0);
    Gallery_ClearShots();
    Gallery_ResetWork();
    gMenuWork.prize.played = 0;
    Gallery_InitPrizes();
    gLobby->connected = 0;
}
