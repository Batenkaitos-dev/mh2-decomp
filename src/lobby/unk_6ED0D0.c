/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "lobby_menu.h"

s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void *memset(void *dst, s32 c, u32 n);
extern MenuWork gMenuWork;
extern M2C_UNK IdSearch_Draw;
extern M2C_UNK IdSearch_Draw2;

void IdSearch_Open(void) {
    memset(&gMenuWork, 0, 0xF00);
    gFieldControl = 0;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, &IdSearch_Draw, &IdSearch_Draw2, 0);
    WinSlide_Step(&gMenuWork.win, 0x280, 0x190);
}
