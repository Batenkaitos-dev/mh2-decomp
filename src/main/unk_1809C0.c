/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "hw_regs.h"

void SendFrameDmaChain(void);
void func_00162040(void);     /* CRI (ADX): a server call */
s32 Dma_WaitIdle(void);
void Packet_Reset(void);
void Sys_WaitFlip(void);
void SubMain_EndFrame(void);
void Gs_ClearFrame(void);
extern s32 gSysVblanks;
extern s32 D_0021905C;
extern s32 D_00219060;
extern s32 gSysFlipCount;

s32 Sys_FlipFrame(s32 unused0) {
    SubMain_EndFrame();
    Dma_WaitIdle();
    Sys_WaitFlip();
    *(s32 *)&T0_MODE = 0x83;
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
