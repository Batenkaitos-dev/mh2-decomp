/*
 * Display and GS setup at boot (InitDisplay), the VBlank handler,
 * Sys_EnableGameVSync, which turns on the game's VSync counter that the
 * handler advances, and the end of a frame (Sys_FlipFrame).
 */
#include "common.h"
#include "dma.h"
#include "syswork.h"
#include "hw_regs.h"
#include "heap.h"
#include "ram_areas.h"
#include "render_state.h"
#include "pad.h"
#include "texture.h"
#include "display.h"
#include "display_util.h"
#include "gs_setup.h"
#include "iop.h"
#include "mem_util.h"
#include "packet_tim2.h"
#include "sys_main.h"
#include "libc.h"
#include "libdma.h"
#include "libgraph.h"
#include "libvu0.h"

extern DmaQueue gDmaQueue;
extern u8 gSysHeap[];         /* a LinearHeap over SYS_HEAP_AREA */
extern s32 D_0021905C;
extern s32 D_00219060;
extern void *gSysHeapBase;
extern volatile s32 gSysHeapSize;  /* size of the block heap */
extern s32 gRenderFlags;
extern s32 gRenderFlags2;
extern s32 D_00219138;
extern s32 gTexCount;
extern s32 gPalCount;
extern s32 gModelCount;
extern s32 gFlipCountdown;
extern void *gSysFreeHook;
extern void *gSysAllocHook;
extern s32 gSysField;
extern s32 gSysCurBuf;
extern s32 gSysFlipAt;
extern s32 gSysNextFlipAt;
extern s32 gSysVblanks;
extern s32 gSysFlipPending;
extern s32 gGameVSyncOn;
extern s32 gSysExiting;
extern s32 gVSyncCount;

DmaChan *sceDmaGetChan(s32 ch);
void func_00162040(void);     /* CRI (ADX): a server call */
extern s32 gSysFlipCount;

/* Turns on the game's VSync counter, which the VBlank handler
 * (VBlankHandler) advances while it is on. */
void Sys_EnableGameVSync(void)
{
    gGameVSyncOn = 1;
}

s32 InitDisplay(s32 w, s32 h)
{
    s32 field;

    if (Sys_Init() == 0) {
        return 0;
    }
    Packet_InitBuffers();
    gSysFlipAt = 0;
    gSysNextFlipAt = 0;
    gSysVblanks = 0;
    Gs_ClearVram();
    Gs_SetupDisplay(4, 4, 1, 0, 1);
    Gs_ShowFrame(0, 1);
    Pad_Init();
    do {
        field = sceGsSyncV(0);
        gSysField = field;
    } while (field != 0);
    *(s32 *)&T1_MODE = 128;
    *(s32 *)&T1_COUNT = 0;
    return 1;
}

/* Clears the system work and sets up the memory: a linear heap over
 * SYS_HEAP_AREA (15 MB), and a block heap in 14 MB taken from it. */
s32 Sys_InitWork(void)
{
    void *heap;

    Mem_Fill(&gSysWork, 0, 1136);
    Vram_Init();
    LinearHeap_Init((LinearHeap *) gSysHeap, SYS_HEAP_AREA, SYS_HEAP_AREA_SIZE, 64);
    gSysHeapSize = 0xE00000;
    heap = SysHeap_AllocTop(gSysHeapSize);
    gSysHeapBase = heap;
    Sys_InitHeap(heap, gSysHeapSize, 64);
    D_00219060 = 0;
    gSysAllocHook = SysHeap_Alloc;
    gSysFreeHook = free;
    D_00219138 = 100;
    D_0021905C = -1;
    gRenderFlags2 = 514;
    gTexCount = 0;
    /* alpha blending on, the alpha test passing all, depth writes and the
     * nearer-or-equal depth test */
    gRenderFlags = RF_BLEND_ON | RF_ATEST(RS_CMP_ALWAYS) | RF_ZWRITE | RF_ZTEST(RS_CMP_LEQUAL)
        | RS_BLEND_ALPHA;
    gPalCount = 0;
    gModelCount = 0;
    gFlipCountdown = 0;
    return 1;
}

s32 Sys_Init(void)
{
    s32 i;

    Iop_Init();
    if (Sys_InitWork() == 0) {
        return 0;
    }
    sceGsResetPath();
    sceDmaReset(1);
    for (i = 0; i < 10; i++) {
        gSysWork.dmaChan[i] = sceDmaGetChan(i);
    }
    sceVpu0Reset();
    gDmaQueue.chan = 1;
    DmaQueue_Init(&gDmaQueue, 4096, DmaQueue_Handler);
    Sys_InitCard();
    return 1;
}

extern s32 gSubVSyncCount;
void VBlankHandler(void)
{
    gSysVblanks++;
    gVSyncCount++;
    if (gGameVSyncOn != 0) {
        gSubVSyncCount++;
    }
    if (gSysVblanks > gSysFlipAt && gSysFlipPending == 1) {
        gSysFlipPending = 0;
        gSysFlipAt = gSysNextFlipAt;
        gSysCurBuf ^= 1;
        Gs_ShowFrame(gSysCurBuf, 0);
    }
    if (gSysExiting == 0) {
        Display_AdxVint();
    }
    __asm__ volatile ("sync.l\n\tei");
}

/*
 * The end of a frame: the display is flipped (Sys_WaitFlip), then the
 * next frame starts: timer 0 counts the frame's H-blanks from 0, the
 * frame's DMA chain is sent and the packet buffer and the GS frame are
 * cleared. Returns 1; the argument is not used (the callers pass 0).
 */
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
