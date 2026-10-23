/*
 * The screen texture, the IOP module loader, and whole-file access on the
 * CD/DVD drive.
 *
 * The screen texture (gScreenTex) describes the frame buffer as a
 * texture: ScreenTex_Init sets it up, ScreenTex_Capture reads the frame
 * shown back into its EE buffer, and ScreenTex_Upload sends that copy to
 * GS memory so that a later frame can draw it. ScreenTex_Send and ScreenTex_SetRegs send the screen texture (the frame
 * captured into the EE, gScreenTex) to its GS block and set the GS's
 * texture registers to draw with it; ScreenTex_Reserve takes the block
 * for a capture made elsewhere. Iop_LoadModule loads an IOP module with
 * retries. File_Read, File_Write, File_Append and File_GetSize open
 * cdrom0:NAME;1 (NAME upper-cased) for one whole-file access; the writes
 * serve the debug log (Debug_Printf).
 */
#include "common.h"
#include "dma.h"
#include "texture.h"
#include "render_state.h"
#include "hw_regs.h"
#include "prim.h"
#include "padio.h"
#include "packet_tim2.h"

extern TexRec gScreenTex;
extern s32 gScreenTexState;
extern s32 gScreenTexVram;
extern DmaQueue gDmaQueue;
extern s32 gRenderFlags;

/* The GS registers for drawing with the screen texture (sent by
 * Prim_SendPacket, which fills in the DMA tag's VIF codes). */
typedef struct ScreenTexPacket {
    u64 dmaTag[2];      /* 0x00 END, 7 quadwords */
    u64 gifTag[2];      /* 0x10 6 registers, A+D */
    GsAd texa;       /* 0x20 */
    GsAd tex1;       /* 0x30 TEX1_1 */
    GsAd tex0;       /* 0x40 TEX0_1 */
    GsAd clamp;      /* 0x50 CLAMP_1 */
    GsAd miptbp1;    /* 0x60 MIPTBP1_1 */
    GsAd miptbp2;    /* 0x70 MIPTBP2_1 */
} ScreenTexPacket;

extern ScreenTexPacket gScreenTexPacket;

extern char gIopLoadErrorFmt[];   /* the message when a module does not load */
extern char gCdromPrefix[];       /* the CD/DVD drive's device name */
extern char gCdromSuffix[];       /* the ISO 9660 version suffix */

u128 *Dma_BuildLoadImage(u128 *pkt, u32 irq, u32 src, u32 size, s16 dbp, s16 dbw, s16 psm, s16 dx,
                         s16 dy, s16 w, s16 h);
s32 FlushCache(s32 op);
s32 sceSifLoadModule(const char *path, s32 argSize, const char *args);
s32 printf(const char *fmt, ...);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
u32 strlen(const char *s);
char *strupr(char *s);
/* Two calls into CRI's ADX library that the file functions make before
 * each open and after each close. */
void func_00161EC0(void);
void func_00161ED8(void);
s32 sceOpen(const char *path, s32 flags);
s32 sceClose(s32 fd);
s32 sceRead(s32 fd, void *buf, s32 size);
s32 sceWrite(s32 fd, const void *buf, s32 size);
s32 sceLseek(s32 fd, s32 offset, s32 whence);
extern s32 gDispHeight;      /* display height */
extern s32 gDispWidth;      /* display width */
extern s32 gFlipCountdown;
extern s32 gSysCurBuf;      /* frame buffer shown */
extern s32 gSysFbBlock0;      /* frame buffers (GS blocks) */
extern s32 gSysFbBlock1;
extern s32 gSysFbDepth;
extern s32 gSysFbPsm;      /* display pixel format */
extern s32 gSysNoDraw;

void Dma_QueueStoreImage(void *dst, u32 size, s16 sbp, s16 sbw, s16 psm, s16 sx, s16 sy, s16 w, s16 h,
                         u32 wait);

/* sceOpen's flags and sceLseek's origin, as the file functions pass them. */
#define IO_READ 1
#define IO_WRITE 2
#define IO_CREATE 0x200
#define IO_TRUNCATE 0x400
#define IO_SEEK_END 2

/* The screen capture's size: a 512 x 448 frame of 32-bit pixels. */
#define CAPTURE_W 512
#define CAPTURE_H 448

/* A 32-bit pixel's alpha (128 is 1.0 on the GS) and colour. */
#define PIXEL_ALPHA_MASK 0xFF000000
#define PIXEL_ALPHA_ONE 0x80000000
#define PIXEL_RGB_MASK 0xFFFFFF

/* Describes the frame buffer as a texture whose pixels go to buf. */
void ScreenTex_Init(void *buf)
{
    gScreenTex.used = 0;
    gScreenTex.resident = 0;
    gScreenTex.swizzled = 0;
    gScreenTex.mode = TEX_MODE_ON_DEMAND;
    gScreenTex.levels = 1;
    gScreenTex.lockFlag = 0;
    gScreenTex.flags = 0;
    gScreenTex.locked = NULL;
    gScreenTex.psm = gSysFbPsm;
    gScreenTex.fmt = gSysFbDepth;
    gScreenTex.width = gDispWidth;
    gScreenTex.height = gDispHeight;
    gScreenTex.tw = Tex_Log2(gScreenTex.width);
    gScreenTex.th = Tex_Log2(gScreenTex.height);
    gScreenTex.blocks = Tex_VramBlocks(&gScreenTex);
    gScreenTex.align = TEX_ALIGN_PAGE;
    gScreenTex.size = Tex_Size(gScreenTex.psm, gScreenTex.width, gScreenTex.height, gScreenTex.levels);
    gScreenTex.mem = buf;
}

/* Reads the frame buffer shown (fromTex: the screen texture's GS copy) back
 * into the screen texture's buffer; fromTex 1 then drops the GS copy. */
s32 ScreenTex_Capture(s32 fromTex)
{
    TexRec *screenTex;
    s16 srcBlock;

    if (gSysNoDraw != 0) {
        return 0;
    }
    if (gFlipCountdown != 0) {
        return 0;
    }
    Dma_WaitIdle();
    screenTex = &gScreenTex;
    if (fromTex == 0) {
        if (gSysCurBuf == 0) {
            srcBlock = gSysFbBlock0;
        } else {
            srcBlock = gSysFbBlock1;
        }
    } else {
        srcBlock = screenTex->vram;
    }
    Dma_QueueStoreImage(screenTex->mem, screenTex->size, srcBlock, screenTex->width / 64, screenTex->psm, 0, 0, screenTex->width, screenTex->height, 1);
    Dma_WaitIdle();
    if (fromTex == 1) {
        ScreenTex_Release();
    }
    return 1;
}

/* Sends the screen texture to GS memory (into its reserved block, or a
 * gap); 0 when there is no room. */
s32 ScreenTex_Upload(void)
{
    TexRec *screenTex = &gScreenTex;
    VramSlot *gapSlot;
    u128 *endPacket;

    if (gScreenTexState == SCREEN_TEX_RESIDENT) {
        ScreenTex_SetRegs();
        return 1;
    }
    if (gScreenTexState == SCREEN_TEX_RESERVED) {
        screenTex->vram = gScreenTexVram;
        screenTex->resident = 1;
    } else {
        gapSlot = VramSlot_FindGap(screenTex->blocks, screenTex->align);
        if ((s32)gapSlot < 0) {
            return 0;
        }
        if (VramSlot_Assign(gapSlot, screenTex) == 0) {
            return 0;
        }
    }
    ScreenTex_Send(screenTex);
    endPacket = Packet_Alloc(Dma_EndPacketSize(0), 16);
    Dma_BuildEndPacket(endPacket, 1);
    DmaQueue_AddChain(0, (u32)endPacket & 0x0FFFFFFF, (u32 *)endPacket, &gDmaQueue);
    ScreenTex_SetRegs();
    return 1;
}

/*
 * Frees the GS block reserved for the screen texture (gScreenTexVram, when
 * set) and sets gScreenTexState to SCREEN_TEX_NONE.
 */
void ScreenTex_Release(void)
{
    if (gScreenTexVram != 0) {
        Vram_FreeBlock(gScreenTexVram & 0xFFFF);
    }
    gScreenTexState = SCREEN_TEX_NONE;
}

/*
 * Queues a DMA load of tex's 32-bit pixels (tex->mem, width x height) to
 * its GS address tex->vram, in a packet taken from Packet_Alloc.
 */
void ScreenTex_Send(TexRec *tex)
{
    u128 *packet;
    u32 src;
    s32 texWidth;
    s32 texHeight;
    u32 size;
    u128 *last;
    s32 vram;

    texWidth = tex->width;
    texHeight = tex->height;
    src = (u32)tex->mem;
    size = texWidth * texHeight * 4;
    packet = Packet_Alloc(Dma_ImagePacketSize(size), 16);
    vram = tex->vram;
    last = Dma_BuildLoadImage(packet, 1, src, size, vram, texWidth / 64, tex->psm, 0, 0, texWidth, texHeight);
    DmaQueue_AddChain(0, ((u32)packet & 0x0FFFFFFF) | DMA_TAG_CNT, (u32 *)last, &gDmaQueue);
}

/*
 * Fills gScreenTexPacket from gScreenTex (TEXA alpha 1.0, TEX1 0, TEX0
 * with its GS address, buffer width, pixel format and size, MIPTBP1 and
 * MIPTBP2 0) and sends it with Prim_SendPacket. CLAMP is 0 (repeat) when
 * gRenderFlags has no RF_TEX_CLAMP_MASK bit, else a region clamp to the
 * texture's width and height.
 */
void ScreenTex_SetRegs(void)
{
    s32 flags = gRenderFlags;
    TexRec *tex = &gScreenTex;
    u64 *texaData = &gScreenTexPacket.texa.data;
    u64 *tex0Data = &gScreenTexPacket.tex0.data;
    u64 *clampData = &gScreenTexPacket.clamp.data;
    u64 *miptbp1Data = &gScreenTexPacket.miptbp1.data;
    u64 *miptbp2Data = &gScreenTexPacket.miptbp2.data;
    s16 bufWidth;

    gScreenTexPacket.tex1.data = 0;
    bufWidth = gScreenTex.width / 64;
    if (bufWidth == 0) {
        bufWidth = 1;
    }
    *texaData = (u64)128 << 32;   /* TA1 0x80: alpha 1.0 */
    /* TBP0, TBW, PSM, TW, TH; TCC 1 (RGBA) */
    *tex0Data = (u64)tex->vram | ((u64)bufWidth << 14) | ((u64)tex->psm << 20) | ((u64)tex->tw << 26) |
            ((u64)tex->th << 30) | ((u64)1 << 34);
    switch (flags & RF_TEX_CLAMP_MASK) {
    case 0:
        *clampData = 0;
        break;
    case RF_TEX_CLAMP:
    default:
        /* REGION_CLAMP on both axes, 0..width, 0..height */
        *clampData = 2 | (2 << 2) | ((u64)tex->width << 14) | ((u64)tex->height << 34);
        break;
    }
    *miptbp1Data = 0;
    *miptbp2Data = 0;
    Prim_SendPacket((u32 *) &gScreenTexPacket);
}

/* Takes the screen texture's GS block and sends the CAPTURE_W x CAPTURE_H
 * 32-bit image pixels there, their alpha first set to 128 (1.0) unless
 * the first pixel's already is; gScreenTexState follows (SCREEN_TEX_*).
 * Returns 1, or 0 when the block or the upload does not fit. */
s32 ScreenTex_Reserve(u32 *pixels)
{
    s32 i;

    gScreenTexVram = Vram_AllocBlock(CAPTURE_W * CAPTURE_H * 4) & 0xFFFF;
    /* Read back through a volatile pointer, as the game's code reloads it. */
    if (*(volatile s32 *)&gScreenTexVram == 0) {
        return 0;
    }
    if ((*pixels & PIXEL_ALPHA_MASK) != PIXEL_ALPHA_ONE) {
        FlushCache(0);
        Dma_WaitIdle();
        /* Eight pixels a pass, as the game's loop is written. */
        for (i = 0; i < CAPTURE_W * CAPTURE_H; i += 8) {
            *pixels = (*pixels & PIXEL_RGB_MASK) | PIXEL_ALPHA_ONE;
            pixels++;
            *pixels = (*pixels & PIXEL_RGB_MASK) | PIXEL_ALPHA_ONE;
            pixels++;
            *pixels = (*pixels & PIXEL_RGB_MASK) | PIXEL_ALPHA_ONE;
            pixels++;
            *pixels = (*pixels & PIXEL_RGB_MASK) | PIXEL_ALPHA_ONE;
            pixels++;
            *pixels = (*pixels & PIXEL_RGB_MASK) | PIXEL_ALPHA_ONE;
            pixels++;
            *pixels = (*pixels & PIXEL_RGB_MASK) | PIXEL_ALPHA_ONE;
            pixels++;
            *pixels = (*pixels & PIXEL_RGB_MASK) | PIXEL_ALPHA_ONE;
            pixels++;
            *pixels = (*pixels & PIXEL_RGB_MASK) | PIXEL_ALPHA_ONE;
            pixels++;
        }
    }
    gScreenTexState = SCREEN_TEX_RESERVED;
    if (ScreenTex_Upload() != 0) {
        gScreenTexState = SCREEN_TEX_RESIDENT;
        return 1;
    }
    gScreenTexState = SCREEN_TEX_NONE;
    return 0;
}

/* Loads an IOP module: until it works (retries 0) or up to retries times. */
void Iop_LoadModule(const char *path, s32 argSize, const char *args, s32 retries)
{
    s32 i;

    if (retries == 0) {
        while (sceSifLoadModule(path, argSize, args) < 0) {
        }
        return;
    }
    for (i = 0; i < retries; i++) {
        if (sceSifLoadModule(path, argSize, args) > 0) {
            return;
        }
    }
    printf(gIopLoadErrorFmt, path);
}

/* Reads the file cdrom0:NAME;1 (upper-cased) into buf; 0 when it cannot
 * be opened. */
s32 File_Read(const char *name, void *buf, s32 size)
{
    char path[2048];
    char *nameStart;
    s32 fileDesc;

    strcpy(path, gCdromPrefix);
    nameStart = path + strlen(path);
    strcat(path, name);
    strupr(nameStart);
    strcat(path, gCdromSuffix);
    func_00161EC0();
    fileDesc = sceOpen(path, IO_READ);
    if (fileDesc < 0) {
        func_00161ED8();
        return 0;
    }
    sceRead(fileDesc, buf, size);
    sceClose(fileDesc);
    func_00161ED8();
    return 1;
}

/*
 * Creates or truncates cdrom0:NAME;1 (NAME upper-cased) and writes size
 * bytes of buf to it. Returns 1, or 0 when the file cannot be opened.
 * Debug_Printf uses it to start the debug log.
 */
s32 File_Write(const char *name, const void *buf, s32 size)
{
    char path[2048];
    char *nameStart;
    s32 fileDesc;

    strcpy(path, gCdromPrefix);
    nameStart = path + strlen(path);
    strcat(path, name);
    strupr(nameStart);
    strcat(path, gCdromSuffix);
    func_00161EC0();
    if ((fileDesc = sceOpen(path, IO_WRITE | IO_CREATE | IO_TRUNCATE)) < 0) {
        func_00161ED8();
        return 0;
    }
    sceWrite(fileDesc, buf, size);
    sceClose(fileDesc);
    func_00161ED8();
    return 1;
}

/*
 * Writes size bytes of buf at the end of cdrom0:NAME;1 (NAME
 * upper-cased). Returns 1, or 0 when the file cannot be opened.
 * Debug_Printf uses it for each message.
 */
s32 File_Append(const char *name, const void *buf, s32 size)
{
    char path[2048];
    char *nameStart;
    s32 fileDesc;

    strcpy(path, gCdromPrefix);
    nameStart = path + strlen(path);
    strcat(path, name);
    strupr(nameStart);
    strcat(path, gCdromSuffix);
    func_00161EC0();
    if ((fileDesc = sceOpen(path, IO_WRITE)) < 0) {
        func_00161ED8();
        return 0;
    }
    sceLseek(fileDesc, 0, IO_SEEK_END);
    sceWrite(fileDesc, buf, size);
    sceClose(fileDesc);
    func_00161ED8();
    return 1;
}

/* The size of the file, 0 when it cannot be opened. */
s32 File_GetSize(const char *name)
{
    char path[2048];
    char *nameStart;
    s32 fileDesc;
    s32 size;

    strcpy(path, gCdromPrefix);
    nameStart = path + strlen(path);
    strcat(path, name);
    strupr(nameStart);
    strcat(path, gCdromSuffix);
    func_00161EC0();
    if ((fileDesc = sceOpen(path, IO_READ)) < 0) {
        func_00161ED8();
        return 0;
    }
    size = sceLseek(fileDesc, 0, IO_SEEK_END);
    sceClose(fileDesc);
    func_00161ED8();
    return size;
}

/* This file's initialised data. */
ScreenTexPacket gScreenTexPacket = {
#include "main/001BA990.inc"
};

/* This file's read-only data. */
#pragma rodata on
char gIopLoadErrorFmt[24] = {
#include "main/001C7C30.inc"
};
char gCdromPrefix[8] = {
#include "main/001C7C48.inc"
};
char gCdromSuffix[16] = {
#include "main/001C7C50.inc"
};
PadConfig gPadConfigDefault = {
#include "main/001C7C60.inc"
};
u32 gPadBit[24] = {
#include "main/001C7C90.inc"
};
#pragma explicit_zero_data on
u32 D_001C7CF0[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
#pragma rodata off
