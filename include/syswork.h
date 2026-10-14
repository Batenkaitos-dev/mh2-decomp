#ifndef SYSWORK_H
#define SYSWORK_H

#include "common.h"
#include "dma.h"

/*
 * 0x218C70: the core's system work, 0x470 bytes cleared at boot by
 * Sys_InitWork (display settings, DMA channels, frame buffers ...). Most
 * code reaches its fields directly by address (gSysFrameMode ..., gDmaChan);
 * the C still does, since a struct member is scheduled differently from a
 * plain extern (docs/mwcc-matching.md, "An instruction or constant load in
 * another place"). The display fields are set by the display setup,
 * Gs_SetupDisplay(fbDepth, zDepth, frameMode, tvMode, narrow), which the game
 * calls once as (4, 4, 1, 0, 1): a 32-bit 512x448 NTSC frame, 32-bit Z.
 *
 * After it (not cleared with it; gSysHeap is at 0x219110) the frame buffer
 * description that the drawing code reads:
 *   0x219140 0 (the frame buffer's base), 0x219144 width, 0x219148 height,
 *   0x21914C width * fbDepth (bytes per line), 0x219154 fbDepth,
 *   0x219158..0x219184 the pixel format's channel sizes and shifts
 *   (5/5/5/1 for 16-bit, 8/8/8 for 24-bit, 8/8/8/8 for 32-bit),
 *   0x219190 the display height and 0x219198 its width in pixels (the 2D
 *   primitives scale their 640-wide coordinates to it).
 */
typedef struct SysWork {
    /* 0x000 */ s32 tvMode;             /* 0 NTSC, 1 PAL at NTSC height, 2 PAL 256 lines */
    /* 0x004 */ s32 frameMode;          /* 0: field rendering (y halved), else full frames */
    /* 0x008 */ s32 unk008;
    /* 0x00C */ s32 fbWidth;            /* 0x280, or 0x200 when narrow */
    /* 0x010 */ s32 fbLines;            /* 0x1C0 (NTSC) or 0x200 (PAL full) */
    /* 0x014 */ u32 magH;               /* 3 (640 wide) or 4 (512 wide) */
    /* 0x018 */ s32 fbDepth;            /* bytes per pixel: 2, 3 or 4 */
    /* 0x01C */ u32 fbPsm;              /* GS PSMCT32 0, PSMCT24 1, PSMCT16 2, PSMCT16S 0xA */
    /* 0x020 */ u32 fbPageW;            /* the PSM's page, 0x40 x 0x20 or 0x40 x 0x40 */
    /* 0x024 */ u32 fbPageH;
    /* 0x028 */ u32 fbBlock[2];         /* the two frame buffers (Vram_AllocBlock) */
    /* 0x030 */ s32 zDepth;             /* bytes per Z value: 2, 3 or 4 */
    /* 0x034 */ u32 zPsm;               /* GS PSMZ32 0x30, PSMZ24 0x31, PSMZ16 0x32, PSMZ16S 0x3A */
    /* 0x038 */ u32 zPageW;
    /* 0x03C */ u32 zPageH;
    /* 0x040 */ u32 zBlock;             /* the Z buffer */
    /* 0x044 */ f32 zMax;               /* 65535.0 (16-bit Z) or 16777000.0 */
    /* 0x048 */ s32 texBlock;           /* next free GS block for textures */
    /* 0x04C */ s32 field;              /* sceGsSyncV's result at the last flip */
    /* 0x050 */ s32 curBuf;             /* frame buffer shown (0/1), flipped by the VBlank handler */
    /* 0x054 */ s32 flipAt;             /* VBlank count at which the pending flip happens */
    /* 0x058 */ s32 nextFlipAt;
    /* 0x05C */ s32 vblanks;            /* VBlanks counted by VBlankHandler */
    /* 0x060 */ s32 flipPending;
    /* 0x064 */ u8 unk064[0x3A0 - 0x064];
    /* 0x3A0 */ u32 bgColor;            /* 0xAARRGGBB, the clear colour (Gs_SetBgColor) */
    /* 0x3A4 */ s32 offsetX;            /* drawing offset: 0x800 minus half the width */
    /* 0x3A8 */ s32 offsetY;            /* 0x800 minus half the height */
    /* 0x3AC */ s32 scrollX;            /* subtracted from every 2D x (Gs_ScreenX) */
    /* 0x3B0 */ s32 scrollY;            /* and y (Gs_ScreenY) */
    /* 0x3B4 */ s32 dispX;              /* DISPLAY's DX: 0x27C (NTSC) or 0x290 (PAL) */
    /* 0x3B8 */ s32 dispY;              /* DY: 0x32, 0x62 or 0x48 */
    /* 0x3BC */ s32 unk3BC;             /* cleared by the display setup */
    /* 0x3C0 */ s32 unk3C0;
    /* 0x3C4 */ DmaChan *dmaChan[10];   /* sceDmaGetChan(0..9), = gDmaChan */
    /* 0x3EC */ s32 vu1Prog;            /* the VU1 program called last (ModelEntry.prog): Model_Draw queues a CALL of a model's program when it differs */
    /* 0x3F0 */ u32 vu1Size;            /* (progInfo[3] - progInfo[2]) >> 3 of the model drawn, set before its packet builder (progInfo[4]) runs */
    /* 0x3F4 */ s32 noDraw;             /* the 2D primitives return at once when set */
    /* 0x3F8 */ u8 unk3F8[0x414 - 0x3F8];
    /* 0x414 */ void *buf[2];           /* two 0xA0000 blocks (Packet_InitBuffers) */
    /* 0x41C */ u8 unk41C[0x420 - 0x41C];
    /* The last GS register values the render state sent (unk_185460.c),
     * per drawing context: TEST, ZBUF, SCISSOR (and its rectangle). */
    /* 0x420 */ u64 gsTest[2];          /* gGsTest1, gGsTest2 */
    /* 0x430 */ u64 gsZbuf[2];          /* gGsZbuf1, gGsZbuf2 */
    /* 0x440 */ u64 gsScissor1;         /* gGsScissor1 */
    /* 0x448 */ s32 scissor1[4];        /* x0, y0, x1, y1 (gScissor1X0 ...) */
    /* 0x458 */ u64 gsScissor2;
    /* 0x460 */ s32 scissor2[4];
} SysWork;

extern SysWork gSysWork;

#endif /* SYSWORK_H */
