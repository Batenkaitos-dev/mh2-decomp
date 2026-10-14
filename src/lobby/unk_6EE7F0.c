/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void *memset(void *dst, s32 c, u32 n);
void AreaSearch_Draw(u8 *o);
void AreaSearch_Draw2(u8 *a0);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
extern M2C_UNK gMenuWork;

void AreaSearch_Open(void) {
    memset(&gMenuWork, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, AreaSearch_Draw, AreaSearch_Draw2, 0);
}

void AreaSearch_Close(void) {
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
}
