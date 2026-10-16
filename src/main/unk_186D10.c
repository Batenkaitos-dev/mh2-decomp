/*
 * Display helpers (main 0x186D10-0x186D80): the display offset, the CRI
 * libraries' VBlank call, and the flip countdown. A frame's flip is
 * requested by Sys_FlipTick (gSysFlipPending), and VBlankHandler
 * (display.c) shows the new buffer at the next VBlank after gSysFlipAt.
 */
#include "common.h"

extern s32 gSysDispAdjX;
extern s32 gSysDispAdjY;
extern s32 gSysFlipPending;
extern s32 gFlipCountdown;

void AdxWrap_VintHook(s32);

void Display_SetOffset(s32 x, s32 y)
{
    gSysDispAdjX = x;
    gSysDispAdjY = y;
}

/* VBlankHandler's call into the CRI libraries. */
void Display_AdxVint(void)
{
    AdxWrap_VintHook(0);
}

/* Called once a frame by Sys_WaitFlip: requests the flip, unless
 * gFlipCountdown holds it, in which case the countdown goes down by one
 * and this frame is not shown. */
void Sys_FlipTick(void)
{
    if (gFlipCountdown == 0) {
        gSysFlipPending = 1;
        return;
    }
    gFlipCountdown -= 1;
}
