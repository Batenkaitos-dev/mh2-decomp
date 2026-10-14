/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

extern s32 gSysDispAdjX;
extern s32 gSysDispAdjY;
extern s32 gSysFlipPending;
extern s32 gFlipCountdown;

void AdxWrap_VintHook(s32);

void Display_SetOffset(s32 x, s32 y) {
    gSysDispAdjX = x;
    gSysDispAdjY = y;
}

void Display_AdxVint(void)
{
    AdxWrap_VintHook(0);
}

void Sys_FlipTick(void) {
    if (gFlipCountdown == 0) {
        gSysFlipPending = 1;
        return;
    }
    gFlipCountdown -= 1;
}
