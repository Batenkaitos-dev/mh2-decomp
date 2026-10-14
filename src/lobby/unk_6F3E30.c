/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_menu.h"

void LobbySe_Play(s32);
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void *memset(void *dst, s32 c, u32 n);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern MenuWork gMenuWork;
extern M2C_UNK HouseAccess_Draw;

void HouseAccess_Open(void) {
    gTownMode = 0x2B;
    gTownMenuOpen = 1;
    memset(&gMenuWork, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, &HouseAccess_Draw, 0, 0);
    WinSlide_Step(&gMenuWork.win, 0x280, 0x19A);
    LobbySe_Play(8);
}
