/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_menu.h"

void LobbySe_Play(s32);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void CamReq_VillageBoard(void);
void *memset(void *dst, s32 c, u32 n);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern MenuWork gMenuWork;
extern s8 gMenuInputLock;
extern M2C_UNK VillageBoard_Draw;

void VillageBoard_Open(void) {
    gTownMenuOpen = 1;
    gTownMode = 0x4A;
    memset(&gMenuWork, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, &VillageBoard_Draw, 0, 0);
    gMenuInputLock = 6;
    WinSlide_Step(&gMenuWork.win, 0x280, 0x11C);
    CamReq_VillageBoard();
    LobbySe_Play(0x2E);
}
