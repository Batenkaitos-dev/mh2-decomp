/*
 * Display helpers: the display offset, the CRI libraries' VBlank call,
 * the flip countdown and the end-of-frame wait. A frame's flip is
 * requested by Sys_FlipTick (gSysFlipPending) from Sys_WaitFlip, and
 * VBlankHandler shows the new buffer at the next VBlank after gSysFlipAt.
 */
#include "common.h"
#include "hw_regs.h"
#include "cri_setup.h"
#include "sound.h"
#include "display_util.h"

extern s32 gSysDispAdjX;
extern s32 gSysDispAdjY;
extern s32 gSysFlipPending;
extern s32 gFlipCountdown;
extern s32 gSysField;
extern s32 gSysFlipAt;
extern s32 gSysVblanks;

s32 func_00162040(void);
void func_00162188(void);

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

/*
 * The end-of-frame wait, called by Sys_FlipFrame. Runs Sound_Update and
 * the CRI (ADX) server call. When the frame ended in time (gSysVblanks at
 * most gSysFlipAt) it stores the GS CSR's FIELD bit in gSysField, requests
 * the flip (Sys_FlipTick) and keeps calling a CRI library routine until
 * gSysFlipPending is cleared at the VBlank. A late frame requests the flip
 * and makes that call once, without waiting.
 */
void Sys_WaitFlip(void)
{
    Sound_Update();
    func_00162040();
    if (gSysVblanks <= gSysFlipAt) {
        gSysField = (s32) ((s64) ((((u64) GS_CSR >> 13) & 1) << 32) >> 32);
        Sys_FlipTick();
        do {
            func_00162188();
        } while (gSysFlipPending != 0);
        return;
    }
    Sys_FlipTick();
    func_00162188();
}
