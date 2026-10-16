/*
 * Sys_FlipFrame (main 0x1809C0): the end of a frame. The display is
 * flipped (Sys_WaitFlip), then the next frame starts: timer 0 counts the
 * frame's H-blanks from 0, the frame's DMA chain is sent and the packet
 * buffer and the GS frame are cleared.
 */
#include "common.h"
#include "hw_regs.h"

void SendFrameDmaChain(void);
void func_00162040(void);     /* CRI (ADX): a server call */
s32 Dma_WaitIdle(void);
void Packet_Reset(void);
void Sys_WaitFlip(void);
void SubMain_EndFrame(void);
void Gs_ClearFrame(void);
extern s32 gSysVblanks;
/* written here and by Sys_InitWork only: no code reads them */
extern s32 D_0021905C;
extern s32 D_00219060;
extern s32 gSysFlipCount;

/* Returns 1; the argument is not used (the callers pass 0). */
s32 Sys_FlipFrame(s32 unused)
{
    SubMain_EndFrame();
    Dma_WaitIdle();
    Sys_WaitFlip();
    *(s32 *)&T0_MODE = T_MODE_CUE | T_MODE_CLKS_HBLANK;
    *(s32 *)&T0_COUNT = 0;
    gSysVblanks = 0;
    gSysFlipCount += 1;
    SendFrameDmaChain();
    Packet_Reset();
    Gs_ClearFrame();
    D_00219060 = 0;
    D_0021905C = -1;
    func_00162040();
    return 1;
}
