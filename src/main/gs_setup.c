/*
 * The GS's set-up and the frame's clears.
 *
 * Gs_SetupDisplay places the two frame buffers and the Z buffer in GS
 * memory, sets the display mode, describes the frame buffer as an image
 * (gFrameImage), and builds the two draw environments (gDrawEnv, one per
 * frame buffer: FRAME, ZBUF, XYOFFSET, DTHE and COLCLAMP) and the clear
 * packet (gClearPacket). Gs_ShowFrame sends a frame buffer's environment
 * and points the display registers at it; Gs_ClearFrame fills the frame
 * with the background colour (gSysBgColor) and puts the draw registers
 * back; Gs_ClearColor fills it with a given colour. Gs_ClearVram clears
 * all of GS memory at boot.
 *
 * Debug_Printf, at the end, writes the debug output file.
 */
#include "common.h"
#include "dma.h"
#include "syswork.h"
#include "image.h"
#include "hw_regs.h"
#include "prim.h"
#include "texture.h"

#include "ee_stdarg.h"
#include "render_state.h"
#include "gs_setup.h"
#include "display.h"
#include "mem_util.h"
#include "packet_tim2.h"
#include "libgraph.h"
#include "libkernl.h"

/* SCISSOR's register value. */
#define GS_SCISSOR(x0, x1, y0, y1) \
    ((u64)(x0) | ((u64)(x1) << 16) | ((u64)(y0) << 32) | ((u64)(y1) << 48))

/*
 * The clear packet (gClearPacket, 320 bytes): a DMA END tag with the VIF
 * words that send the rest DIRECT to the GIF, then 18 A+D writes: a sprite
 * of the background colour over the frame (FBA_1, SCISSOR_1, TEST_1, PRIM,
 * RGBAQ, XYZ2 twice), then the draw registers put back. Sys_InitGs fills the
 * constant words, Gs_ClearFrame the frame and colour ones before sending it.
 */
typedef struct ClearPacket {
    u64 dmaTag;                 /* 0x000: an END tag of 19 quadwords (DmaTag_End) */
    u32 vifFlush;               /* 0x008: VIF FLUSHA */
    u32 vifDirect;              /* 0x00C: VIF DIRECTHL of 19 quadwords */
    u64 gifTag;                 /* 0x010: 18 loops, EOP */
    u64 gifRegs;                /* 0x018: GIF_REG_AD */
    GsAd fba;                   /* 0x020: FBA_1 0 */
    GsAd scissor;               /* 0x030: SCISSOR_1: the display */
    GsAd test;                  /* 0x040: TEST_1: 0x30003, depth test off */
    GsAd prim;                  /* 0x050: PRIM: GS_PRIM_SPRITE */
    GsAd rgbaq;                 /* 0x060: RGBAQ: gSysBgColor */
    GsAd xyz[2];                /* 0x070: XYZ2: the two corners */
    GsAd scissor1;              /* 0x090: SCISSOR_1 put back (gGsScissor1) */
    GsAd scissor2;              /* 0x0A0: SCISSOR_2 (gGsScissor2) */
    GsAd test1;                 /* 0x0B0: TEST_1 (gGsTest1) */
    GsAd test2;                 /* 0x0C0: TEST_2 (gGsTest2) */
    GsAd zbuf1;                 /* 0x0D0: ZBUF_1 (gGsZbuf1) */
    GsAd zbuf2;                 /* 0x0E0: ZBUF_2 (gGsZbuf2) */
    GsAd fba1;                  /* 0x0F0: FBA_1 (gGsFba) */
    GsAd fba2;                  /* 0x100: FBA_2 (gGsFba) */
    GsAd pabe;                  /* 0x110: PABE 0 */
    GsAd prmodecont;            /* 0x120: PRMODECONT 1 */
    GsAd dimx;                  /* 0x130: DIMX: the dither matrix */
} ClearPacket;
typedef char ClearPacket_size_check[(sizeof(ClearPacket) == 320) ? 1 : -1];

extern DmaQueue gDmaQueue;
extern u32 gSysBgColor;      /* the background colour, 0xAARRGGBB */
extern s32 gSysOffsetX;      /* drawing offset x (pixels) */
extern s32 gSysOffsetY;      /* drawing offset y */
extern s32 gDispHeight;      /* display height */
extern s32 gDispWidth;      /* display width */
extern u64 gGsTest1;
extern u64 gGsTest2;
extern u64 gGsZbuf1;
extern u64 gGsZbuf2;
extern u64 gGsScissor1;
extern u64 gGsScissor2;
extern u32 gGsFba;
extern Image gFrameImage;      /* the frame buffer as an image */
extern ClearPacket gClearPacket;
extern s32 gDebugFileFirst;      /* 1 until the debug file has been created */
extern u64 gClearColorPacket[];

extern u64 gDrawEnv[2][20];     /* the two frame buffers' draw environments */
extern s32 gSysFrameMode;
extern s32 gSysFbWidth;
extern s32 gSysFbLines;
extern s32 gSysMagH;
extern s32 gSysFbDepth;
extern u32 gSysFbPsm;
extern u32 gSysFbPageW;
extern u32 gSysFbPageH;
extern u32 gSysFbBlock0;
extern u32 gSysFbBlock1;
extern s32 gSysZDepth;
extern u32 gSysZPsm;
extern u32 gSysZPageW;
extern u32 gSysZPageH;
extern u32 gSysZBlock;
extern f32 gSysZMax;
extern s32 gSysField;
extern s32 gSysCurBuf;
extern s32 gSysScrollX;
extern s32 gSysScrollY;
extern s32 gSysDispX;
extern s32 gSysDispY;
extern s32 gSysDispAdjX;
extern s32 gSysDispAdjY;
extern struct SysWork gSysWork;
void Mem_CopyQwords(void *src, void *dst, s32 qwc);
void Mem_ClearQwords(void *dst, s32 qwc);
s32 sceDmaSend(DmaChan *chan, void *tag);
s32 sceDmaSync(DmaChan *chan, s32 mode, s32 timeout);
s32 vsprintf(char *buf, const char *fmt, va_list ap);
u32 strlen(const char *s);
s32 File_Write(const char *name, void *buf, s32 size);
s32 File_Append(const char *name, void *buf, s32 size);

/* GS memory seen as 32-bit pixels: GS_VRAM_W by GS_VRAM_W. Gs_ClearVram
 * clears it CLEAR_ROWS rows at a time. */
#define GS_VRAM_W 1024
#define CLEAR_ROWS 64
#define CLEAR_BYTES (GS_VRAM_W * CLEAR_ROWS * 4)

/* Clears all of GS memory, CLEAR_ROWS rows at a time. */
void Gs_ClearVram(void)
{
    u8 buf[96];
    void *memHandle;
    void *zeroPixels;
    u32 i;
    s32 y;

    i = 0;
    memHandle = Mem_Alloc(CLEAR_BYTES, 0);
    zeroPixels = Mem_GetAddr(memHandle);
    Mem_ClearQwords(zeroPixels, CLEAR_BYTES / 16);
    y = 0;
    do {
        sceGsSetDefLoadImage(buf, 0, GS_VRAM_W / 64, GS_PSMCT32, 0, y, GS_VRAM_W, CLEAR_ROWS);
        FlushCache(0);
        sceGsExecLoadImage(buf, zeroPixels);
        sceGsSyncPath(0, 0);
        i++;
        y += CLEAR_ROWS;
    } while (i < GS_VRAM_W / CLEAR_ROWS);
    Mem_Free(memHandle);
}

/* The display setup: frame and Z buffers in GS memory, the display
 * registers, the frame buffer's pixel format (gFrameImage, an Image), the two
 * draw environments (one per frame buffer; the first addressed by name, the
 * second through a pointer), the clear packet, and the VBlank callback.
 * The game calls it once, as (4, 4, FRAME_MODE_FULL, TV_MODE_NTSC, 1).
 * fbDepth and zDepth are bytes per pixel; narrow 1 makes the frame 512
 * pixels wide instead of 640. sceGsResetGraph is passed 2 for NTSC and
 * 3 for PAL, then 1 for field rendering and 0 for full frames. */
#pragma optimization_level 4
/* Level 4. The clear packet's stores follow the game's order: the store
 * order decides where the compiler schedules the two stores of 71
 * (GS_REG_TEST_1, also the VBlank callback's argument) and the two
 * stores of 1. gSysWork's fields must be
 * struct members (they are reloaded after the stores through the
 * environment pointer), and `case TV_MODE_NTSC: default:` keeps the default body from
 * reusing the compare constants. */
void Gs_SetupDisplay(s32 fbDepth, s32 zDepth, s32 frameMode, s32 tvMode, s32 narrow)
{
    s32 frameWidth;
    s32 frameHeight;
    s32 lines;
    u32 size;
    u64 *firstEnv;
    u64 *secondEnv;
    u64 fbw;

    gSysWork.tvMode = tvMode;
    gSysWork.frameMode = frameMode;
    gSysWork.fbDepth = fbDepth;
    gSysWork.zDepth = zDepth;
    gSysWork.dispAdjX = 0;
    gSysWork.dispAdjY = 0;
    if (narrow != 1) {
        gSysWork.magH = 3;
        frameWidth = 640;
    } else {
        gSysWork.magH = 4;
        frameWidth = 512;
    }
    if (frameMode == FRAME_MODE_FIELD) {
        switch (tvMode) {
        case TV_MODE_NTSC:
        default:
            sceGsResetGraph(0, 1, 2, 1);
            frameHeight = 224;
            lines = 448;
            gSysWork.dispX = 636;
            gSysWork.dispY = 50;
            break;
        case TV_MODE_PAL_NTSC_HEIGHT:
            sceGsResetGraph(0, 1, 3, 1);
            frameHeight = 224;
            lines = 448;
            gSysWork.dispX = 656;
            gSysWork.dispY = 98;
            break;
        case TV_MODE_PAL:
            sceGsResetGraph(0, 1, 3, 1);
            frameHeight = 256;
            lines = 512;
            gSysWork.dispX = 656;
            gSysWork.dispY = 72;
            break;
        }
    } else {
        switch (tvMode) {
        case TV_MODE_NTSC:
        default:
            sceGsResetGraph(0, 1, 2, 0);
            gSysWork.dispX = 636;
            frameHeight = 448;
            lines = 448;
            gSysWork.dispY = 50;
            break;
        case TV_MODE_PAL_NTSC_HEIGHT:
            sceGsResetGraph(0, 1, 3, 0);
            gSysWork.dispX = 656;
            frameHeight = 448;
            lines = 448;
            gSysWork.dispY = 98;
            break;
        case TV_MODE_PAL:
            sceGsResetGraph(0, 1, 3, 0);
            gSysWork.dispX = 656;
            frameHeight = 512;
            lines = 512;
            gSysWork.dispY = 72;
            break;
        }
    }
    gSysWork.fbLines = lines;
    gDispWidth = frameWidth;
    gDispHeight = frameHeight;
    gSysWork.fbWidth = frameWidth;
    switch (gSysWork.fbDepth) {
    case 2:
        gSysWork.fbPsm = GS_PSMCT16;
        gSysWork.fbPageW = 64;
        gSysWork.fbPageH = 64;
        break;
    case 3:
        gSysWork.fbPsm = GS_PSMCT24;
        gSysWork.fbPageW = 64;
        gSysWork.fbPageH = 32;
        break;
    default:
    case 4:
        gSysWork.fbPsm = GS_PSMCT32;
        gSysWork.fbPageW = 64;
        gSysWork.fbPageH = 32;
        break;
    }
    size = fbDepth * (gSysWork.fbPageH * (((frameHeight + gSysWork.fbPageH - 1) / gSysWork.fbPageH) *
                                    (gSysWork.fbPageW * ((frameWidth + gSysWork.fbPageW - 1) / gSysWork.fbPageW))));
    gSysWork.fbBlock[0] = Vram_AllocBlock(size) & 0xFFFF;
    gSysWork.fbBlock[1] = Vram_AllocBlock(size) & 0xFFFF;
    switch (gSysWork.zDepth) {
    case 2:
        if (gSysWork.fbDepth == 2) {
            gSysWork.zPsm = GS_PSMZ16;
        } else {
            gSysWork.zPsm = GS_PSMZ16S;
        }
        gSysWork.zPageW = 64;
        gSysWork.zPageH = 64;
        gSysWork.zMax = 65535.0f;
        break;
    case 3:
        gSysWork.zPsm = GS_PSMZ24;
        gSysWork.zPageW = 64;
        gSysWork.zPageH = 32;
        gSysWork.zMax = 16777000.0f;
        break;
    default:
    case 4:
        if (gSysWork.fbDepth == 2) {
            gSysWork.fbPsm = GS_PSMCT16S;
        }
        gSysWork.zPsm = GS_PSMZ32;
        gSysWork.zPageW = 64;
        gSysWork.zPageH = 32;
        gSysWork.zMax = 16777000.0f;
        break;
    }
    gSysWork.zBlock = Vram_AllocBlock(zDepth * (gSysWork.zPageW * ((frameWidth + gSysWork.zPageW - 1) / gSysWork.zPageW) *
                                           (gSysWork.zPageH * ((frameHeight + gSysWork.zPageH - 1) / gSysWork.zPageH)))) & 0xFFFF;
    gSysWork.scrollX = 0;
    gSysWork.scrollY = 0;
    gFrameImage.width = frameWidth;
    gSysWork.offsetX = GS_COORD_RANGE / 2 - (frameWidth >> 1);
    gFrameImage.height = frameHeight;
    gSysWork.offsetY = GS_COORD_RANGE / 2 - (frameHeight >> 1);
    gFrameImage.pixels = 0;
    gFrameImage.flags = 0;
    gFrameImage.bpp = gSysWork.fbDepth;
    gFrameImage.pitch = frameWidth * gSysWork.fbDepth;
    switch (gSysWork.fbDepth) {
    case 2:
        IMAGE_RGBA5551(&gFrameImage);
        IMAGE_GS_ORDER_16(&gFrameImage);
        break;
    case 3:
        IMAGE_RGB888(&gFrameImage);
        IMAGE_GS_ORDER_32(&gFrameImage);
        break;
    case 4:
        IMAGE_RGBA8888(&gFrameImage);
        IMAGE_GS_ORDER_32(&gFrameImage);
        break;
    }
    Gs_ResetState();
    gSysWork.curBuf = 0;
    gSysWork.field = 1;
    firstEnv = gDrawEnv[0];
    secondEnv = gDrawEnv[1];
    DmaTag_End((u128 *)firstEnv, 9, 0, 0);
    DmaTag_End((u128 *)secondEnv, 9, 0, 0);
    gDrawEnv[0][2] = 0x1000000000008008;
    gDrawEnv[0][3] = GIF_REG_AD;
    secondEnv[2] = 0x1000000000008008;
    secondEnv[3] = GIF_REG_AD;
    fbw = (u64)(gDispWidth / 64) << 16;
    gDrawEnv[0][6] = gDrawEnv[0][4] = ((u64)gSysWork.fbPsm << 24) | ((u64)(gSysWork.fbBlock[1] >> 5) | fbw) | ((u64)1 << 32);
    gDrawEnv[0][5] = GS_REG_FRAME_1;
    gDrawEnv[0][7] = GS_REG_FRAME_2;
    secondEnv[4] = ((u64)gSysWork.fbPsm << 24) | ((u64)(gSysWork.fbBlock[0] >> 5) | fbw) | ((u64)1 << 32);
    secondEnv[6] = ((u64)gSysWork.fbPsm << 24) | ((u64)(gSysWork.fbBlock[0] >> 5) | fbw) | ((u64)1 << 32);
    secondEnv[5] = GS_REG_FRAME_1;
    secondEnv[7] = GS_REG_FRAME_2;
    gDrawEnv[0][9] = GS_REG_ZBUF_1;
    gDrawEnv[0][11] = GS_REG_ZBUF_2;
    gDrawEnv[0][10] = gDrawEnv[0][8] = (u64)(gSysWork.zBlock >> 5) | ((u64)gSysWork.zPsm << 24);
    secondEnv[8] = (u64)(gSysWork.zBlock >> 5) | ((u64)gSysWork.zPsm << 24);
    secondEnv[10] = (u64)(gSysWork.zBlock >> 5) | ((u64)gSysWork.zPsm << 24);
    secondEnv[9] = GS_REG_ZBUF_1;
    secondEnv[11] = GS_REG_ZBUF_2;
    gDrawEnv[0][13] = GS_REG_XYOFFSET_1;
    gDrawEnv[0][15] = GS_REG_XYOFFSET_2;
    gDrawEnv[0][14] = gDrawEnv[0][12] = (u64)(gSysWork.offsetX << 4) | ((u64)(gSysWork.offsetY << 4) << 32);
    secondEnv[12] = (u64)(gSysWork.offsetX << 4) | ((u64)(gSysWork.offsetY << 4) << 32);
    secondEnv[14] = (u64)(gSysWork.offsetX << 4) | ((u64)(gSysWork.offsetY << 4) << 32);
    secondEnv[13] = GS_REG_XYOFFSET_1;
    secondEnv[15] = GS_REG_XYOFFSET_2;
    if (gSysWork.fbDepth == 2) {
        firstEnv[16] = 1;
        secondEnv[16] = 1;
    } else {
        firstEnv[16] = 0;
        secondEnv[16] = 0;
    }
    firstEnv[17] = GS_REG_DTHE;
    secondEnv[17] = GS_REG_DTHE;
    firstEnv[18] = 1;
    secondEnv[18] = 1;
    firstEnv[19] = GS_REG_COLCLAMP;
    secondEnv[19] = GS_REG_COLCLAMP;
    DmaTag_End((u128 *) &gClearPacket, 19, 1, 0);
    gClearPacket.fba.data = 0;
    gClearPacket.vifFlush = VIF_FLUSHA;
    gClearPacket.scissor.data = 0;
    gClearPacket.vifDirect = VIF_DIRECTHL | 19;
    gClearPacket.prim.reg = GS_REG_PRIM;
    gClearPacket.rgbaq.data = 0;
    gClearPacket.xyz[0].data = 0;
    gClearPacket.gifTag = 0x1000000000008012;
    gClearPacket.gifRegs = GIF_REG_AD;
    gClearPacket.fba.reg = GS_REG_FBA_1;
    gClearPacket.fba1.reg = GS_REG_FBA_1;
    gClearPacket.scissor.reg = GS_REG_SCISSOR_1;
    gClearPacket.test.data = 0x30003;
    gClearPacket.scissor1.reg = GS_REG_SCISSOR_1;
    gClearPacket.prim.data = GS_PRIM_SPRITE;
    gClearPacket.test.reg = GS_REG_TEST_1;
    gClearPacket.test1.reg = GS_REG_TEST_1;
    gClearPacket.rgbaq.reg = GS_REG_RGBAQ;
    gClearPacket.xyz[0].reg = GS_REG_XYZ2;
    gClearPacket.scissor2.reg = GS_REG_SCISSOR_2;
    gClearPacket.test2.reg = GS_REG_TEST_2;
    gClearPacket.zbuf1.reg = GS_REG_ZBUF_1;
    gClearPacket.zbuf2.reg = GS_REG_ZBUF_2;
    gClearPacket.fba2.reg = GS_REG_FBA_2;
    gClearPacket.pabe.reg = GS_REG_PABE;
    gClearPacket.prmodecont.data = 1;
    gClearPacket.prmodecont.reg = GS_REG_PRMODECONT;
    gClearPacket.xyz[1].reg = GS_REG_XYZ2;
    gClearPacket.xyz[1].data = 0;
    gClearPacket.dimx.data = 0x6071243571603524;
    gClearPacket.dimx.reg = GS_REG_DIMX;
    gClearPacket.scissor1.data = 0;
    gClearPacket.scissor2.data = 0;
    gClearPacket.test1.data = 0;
    gClearPacket.test2.data = 0;
    gClearPacket.zbuf1.data = 0;
    gClearPacket.zbuf2.data = 0;
    gClearPacket.fba1.data = 0;
    gClearPacket.fba2.data = 0;
    gClearPacket.pabe.data = 0;
    sceGsSyncVCallback(VBlankHandler, 71);
}
#pragma optimization_level reset

/* DISPFB's and DISPLAY's register values. */
#define GS_SET_DISPFB(fbp, fbw, psm) ((u64)(fbp) | ((u64)(fbw) << 9) | ((u64)(psm) << 15))
#define GS_SET_DISPLAY(dx, dy, magh, dw, dh) \
    ((u64)(dx) | ((u64)(dy) << 12) | ((u64)(magh) << 23) | ((u64)(dw) << 32) | ((u64)(dh) << 44))

/*
 * Shows frame buffer buf (0 or 1): sends its draw environment
 * (gDrawEnv[buf], with the field's drawing offset when the frames are
 * interlaced) and waits for the DMA, then sets the display registers
 * (PMODE, SMODE2, DISPFB, DISPLAY) for it. boot is 0 from the VBlank
 * handler (hence iFlushCache), 1 at boot.
 */
void Gs_ShowFrame(s32 buf, s32 boot)
{
    u64 *env = gDrawEnv[buf];
    s32 displayX;
    s32 displayY;

    if (gSysWork.frameMode != FRAME_MODE_FULL) {
        if (gSysWork.field == 1) {
            env[12] = (u64)(gSysWork.offsetX << 4) | ((u64)(gSysWork.offsetY << 4) << 32);
            env[14] = (u64)(gSysWork.offsetX << 4) | ((u64)(gSysWork.offsetY << 4) << 32);
        } else {
            env[12] = (u64)(gSysWork.offsetX << 4) | ((u64)((gSysWork.offsetY << 4) + 8) << 32);
            env[14] = (u64)(gSysWork.offsetX << 4) | ((u64)((gSysWork.offsetY << 4) + 8) << 32);
        }
    }
    if (boot == 0) {
        iFlushCache(0);
    } else {
        FlushCache(0);
    }
    Dma_WaitIdle();
    sceDmaSend(gDmaChan[2], env);
    while (sceDmaSync(gDmaChan[2], 1, 0) != 0) {
    }
    displayX = gSysWork.dispX + gSysWork.dispAdjX * gSysWork.magH;
    displayY = gSysWork.dispY + gSysWork.dispAdjY;
    if (gSysWork.frameMode == FRAME_MODE_FIELD) {
        HW_WR64(GS_SMODE2, 3);
        HW_WR64(GS_PMODE, 0x66);
        if (buf == 0) {
            HW_WR64(GS_DISPFB2, GS_SET_DISPFB(gSysWork.fbBlock[0] >> 5, gDispWidth / 64, gSysWork.fbPsm));
        } else {
            HW_WR64(GS_DISPFB2, GS_SET_DISPFB(gSysWork.fbBlock[1] >> 5, gDispWidth / 64, gSysWork.fbPsm));
        }
        HW_WR64(GS_DISPLAY2, GS_SET_DISPLAY(displayX, displayY, gSysWork.magH, 0x9FF, gSysWork.fbLines - 1));
    } else if (gSysWork.oneCircuit == 0) {
        HW_WR64(GS_SMODE2, 1);
        HW_WR64(GS_PMODE, 0x7F27);
        if (buf == 0) {
            HW_WR64(GS_DISPFB1, GS_SET_DISPFB(gSysWork.fbBlock[0] >> 5, gDispWidth / 64, gSysWork.fbPsm));
            HW_WR64(GS_DISPFB2, GS_SET_DISPFB(gSysWork.fbBlock[0] >> 5, gDispWidth / 64, gSysWork.fbPsm));
        } else {
            HW_WR64(GS_DISPFB1, GS_SET_DISPFB(gSysWork.fbBlock[1] >> 5, gDispWidth / 64, gSysWork.fbPsm));
            HW_WR64(GS_DISPFB2, GS_SET_DISPFB(gSysWork.fbBlock[1] >> 5, gDispWidth / 64, gSysWork.fbPsm));
        }
        HW_WR64(GS_DISPLAY1, GS_SET_DISPLAY(displayX, displayY + 1, gSysWork.magH, 0x9FF, gSysWork.fbLines - 1));
        HW_WR64(GS_DISPLAY2, GS_SET_DISPLAY(displayX, displayY, gSysWork.magH, 0x9FF, gSysWork.fbLines - 1));
    } else {
        HW_WR64(GS_SMODE2, 1);
        HW_WR64(GS_PMODE, 0x66);
        if (buf == 0) {
            HW_WR64(GS_DISPFB2, GS_SET_DISPFB(gSysWork.fbBlock[0] >> 5, gDispWidth / 64, gSysWork.fbPsm));
        } else {
            HW_WR64(GS_DISPFB2, GS_SET_DISPFB(gSysWork.fbBlock[1] >> 5, gDispWidth / 64, gSysWork.fbPsm));
        }
        HW_WR64(GS_DISPLAY2, GS_SET_DISPLAY(displayX, displayY, gSysWork.magH, 0x9FF, gSysWork.fbLines - 1));
    }
    HW_WR64(GS_EXTWRITE, 0);
}

/*
 * Clears the frame with the background colour: a sprite over the whole
 * display with the scissor opened and the depth test off, then the render
 * state (scissor, test, zbuf, fba ...) put back from its current values.
 * The offsets are read from gSysWork's fields into locals, y first, as
 * the display setup reads them: so read, the compiler gives the values
 * the game's registers; read through the plain gSysOffsetX/Y externs,
 * the height and the x offset trade registers.
 */
void Gs_ClearFrame(void)
{
    u128 *packet;
    s32 drawOffsetY;
    s32 drawOffsetX;

    gClearPacket.scissor.data = GS_SCISSOR(0, gDispWidth - 1, 0, gDispHeight - 1);
    gClearPacket.rgbaq.data = GS_RGBAQ_ARGB(gSysBgColor, 1);
    drawOffsetY = gSysWork.offsetY;
    drawOffsetX = gSysWork.offsetX;
    gClearPacket.xyz[0].data = GS_XYZ(drawOffsetX << 4, drawOffsetY << 4, 0);
    gClearPacket.xyz[1].data = GS_XYZ((drawOffsetX + gDispWidth) << 4, (drawOffsetY + gDispHeight) << 4, 0);
    gClearPacket.scissor1.data = gGsScissor1;
    gClearPacket.scissor2.data = gGsScissor2;
    gClearPacket.test1.data = gGsTest1;
    gClearPacket.test2.data = gGsTest2;
    gClearPacket.zbuf1.data = gGsZbuf1;
    gClearPacket.zbuf2.data = gGsZbuf2;
    gClearPacket.fba2.data = gClearPacket.fba1.data = gGsFba;
    packet = Packet_Alloc(sizeof(ClearPacket), 16);
    Mem_CopyQwords(&gClearPacket, packet, 20);
    DmaQueue_AddChain(0, (u32)packet & 0x0FFFFFFF, (u32 *)packet, &gDmaQueue);
}

/* printf to the debug file acrout.txt on the host (cdrom0:, so it fails on
 * a disc): the file is created with a header by the first call, then each
 * message is appended with a CR LF. */
s32 Debug_Printf(const char *fmt, ...)
{
    char buf[2048];
    va_list args;
    char *tail;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
    tail = buf + strlen(buf);
    *tail++ = '\r';
    *tail++ = '\n';
    *tail = '\0';
    if (gDebugFileFirst) {
        File_Write("acrout.txt", "Debug Message Output for PS2\r\n", strlen("Debug Message Output for PS2\r\n"));
        gDebugFileFirst = 0;
    }
    File_Append("acrout.txt", buf, strlen(buf));
    return 1;
}

/* Clears the frame with the given colour (0xRRGGBB); mode 0 with the depth
 * test set to pass and write (0x3200F), mode 1 without (0x30003). */
s32 Gs_ClearColor(u8 mode, u32 color)
{
    switch (mode) {
    case 0:
        gClearColorPacket[4] = 0x3200F;
        break;
    case 1:
        gClearColorPacket[4] = 0x30003;
        break;
    default:
        return 0;
    }
    gClearColorPacket[8] = GS_RGBAQ((color >> 16) & 0xFF, (color >> 8) & 0xFF, color & 0xFF, 0, 1);
    gClearColorPacket[10] = GS_XYZ(gSysOffsetX << 4, gSysOffsetY << 4, 0);
    gClearColorPacket[12] = GS_XYZ((gSysOffsetX + gDispWidth) << 4, (gSysOffsetY + gDispHeight) << 4, 0);
    gClearColorPacket[14] = gGsTest1;
    Prim_SendPacket((u32 *)gClearColorPacket);
    return 0;
}

/* This file's initialised data. */
s32 gDebugFileFirst = {
#include "main/001BAA10.inc"
};
u64 gClearColorPacket[16] = {
#include "main/001BAA20.inc"
};
