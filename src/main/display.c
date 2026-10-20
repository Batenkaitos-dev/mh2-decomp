/*
 * Display and GS setup at boot (InitDisplay), the VBlank handler
 */
#include "common.h"
#include "dma.h"
#include "syswork.h"
#include "hw_regs.h"
#include "heap.h"
#include "ram_areas.h"

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

void Mem_Fill(void *dst, u8 c, s32 n);
void Vram_Init(void);
s32 LinearHeap_Init(LinearHeap *h, void *start, u32 size, s32 align);
void *SysHeap_Alloc(u32 size);
void *SysHeap_AllocTop(u32 size);
void free(void *p);
void Sys_InitHeap(void *base, s32 size, s32 align);
void Iop_Init(void);
void sceVpu0Reset(void);
s32 Sys_InitCard(void);
s32 sceGsResetPath(void);
void sceDmaReset(s32 mode);
DmaChan *sceDmaGetChan(s32 ch);
void DmaQueue_Init(DmaQueue *q, u32 capacity, s32 (*handler)(s32));
s32 DmaQueue_Handler(s32 ch);
void Packet_InitBuffers(void);
s32 Sys_Init(void);
void Gs_ClearVram(void);
void Gs_SetupDisplay(s32, s32, s32, s32, s32);
void Gs_ShowFrame(s32, s32);
s32 Pad_Init(void);
s32 sceGsSyncV(s32);
void Display_AdxVint(void);

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
    *(s32 *)&T1_MODE = 0x80;
    *(s32 *)&T1_COUNT = 0;
    return 1;
}

/* Clears the system work and sets up the memory: a linear heap over
 * SYS_HEAP_AREA (15 MB), and a block heap in 14 MB taken from it. */
s32 Sys_InitWork(void)
{
    void *heap;

    Mem_Fill(&gSysWork, 0, 0x470);
    Vram_Init();
    LinearHeap_Init((LinearHeap *) gSysHeap, SYS_HEAP_AREA, SYS_HEAP_AREA_SIZE, 0x40);
    gSysHeapSize = 0xE00000;
    heap = SysHeap_AllocTop(gSysHeapSize);
    gSysHeapBase = heap;
    Sys_InitHeap(heap, gSysHeapSize, 0x40);
    D_00219060 = 0;
    gSysAllocHook = SysHeap_Alloc;
    gSysFreeHook = free;
    D_00219138 = 100;
    D_0021905C = -1;
    gRenderFlags2 = 0x202;
    gTexCount = 0;
    gRenderFlags = 0x138B032;
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
    DmaQueue_Init(&gDmaQueue, 0x1000, DmaQueue_Handler);
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
