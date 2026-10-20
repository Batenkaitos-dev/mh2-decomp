/*
 * The screen texture sent to GS memory and the GS
 * registers to draw with it; loading IOP modules with retries; whole-file
 * reads and writes on cdrom0: (the writes serve the debug log, acrout.txt).
 */
#include "common.h"
#include "dma.h"
#include "texture.h"

extern TexRec gScreenTex;
extern s32 gScreenTexState;
extern s32 gScreenTexVram;
extern DmaQueue gDmaQueue;
extern s32 gRenderFlags;

/* A GS register (A+D mode: data, then the register number). */
typedef struct GsAdReg {
    u64 data;
    u64 reg;
} GsAdReg;

/* The GS registers for drawing with the screen texture (sent by
 * Prim_SendPacket, which fills in the DMA tag's VIF codes). */
typedef struct ScreenTexPacket {
    u64 dmaTag[2];      /* 0x00 END, 7 quadwords */
    u64 gifTag[2];      /* 0x10 6 registers, A+D */
    GsAdReg texa;       /* 0x20 */
    GsAdReg tex1;       /* 0x30 TEX1_1 */
    GsAdReg tex0;       /* 0x40 TEX0_1 */
    GsAdReg clamp;      /* 0x50 CLAMP_1 */
    GsAdReg miptbp1;    /* 0x60 MIPTBP1_1 */
    GsAdReg miptbp2;    /* 0x70 MIPTBP2_1 */
} ScreenTexPacket;

extern ScreenTexPacket gScreenTexPacket;

extern char D_001C7C30[];   /* "Can't load module %s" */
extern char D_001C7C48[];   /* "cdrom0:" */
extern char D_001C7C50[];   /* ";1" */

void *Packet_Alloc(u32 size, s32 align);
u128 *Dma_BuildLoadImage(u128 *pkt, u32 irq, u32 src, u32 size, s16 dbp, s16 dbw, s16 psm, s16 dx,
                         s16 dy, s16 w, s16 h);
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *q);
s32 Prim_SendPacket(u32 *pkt);
s32 FlushCache(s32 op);
s32 Dma_WaitIdle(void);
s32 sceSifLoadModule(const char *path, s32 argSize, const char *args);
s32 printf(const char *fmt, ...);
char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
u32 strlen(const char *s);
char *strupr(char *s);
void func_00161EC0(void);
void func_00161ED8(void);
s32 sceOpen(const char *path, s32 flags);
s32 sceClose(s32 fd);
s32 sceRead(s32 fd, void *buf, s32 size);
s32 sceWrite(s32 fd, const void *buf, s32 size);
s32 sceLseek(s32 fd, s32 offset, s32 whence);

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
    DmaQueue_AddChain(0, ((u32)packet & 0x0FFFFFFF) | 0x10000000, (u32 *)last, &gDmaQueue);
}

/*
 * Fills gScreenTexPacket from gScreenTex (TEXA alpha 1.0, TEX1 0, TEX0
 * with its GS address, buffer width, pixel format and size, MIPTBP1 and
 * MIPTBP2 0) and sends it with Prim_SendPacket. CLAMP is 0 (repeat) when
 * gRenderFlags has neither of bits 0x60000, else a region clamp to the
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
    *texaData = (u64)0x80 << 32;   /* TA1 0x80: alpha 1.0 */
    /* TBP0, TBW, PSM, TW, TH; TCC 1 (RGBA) */
    *tex0Data = (u64)tex->vram | ((u64)bufWidth << 14) | ((u64)tex->psm << 20) | ((u64)tex->tw << 26) |
            ((u64)tex->th << 30) | ((u64)1 << 34);
    switch (flags & 0x60000) {
    case 0:
        *clampData = 0;
        break;
    case 0x20000:
    default:
        /* REGION_CLAMP on both axes, 0..width, 0..height */
        *clampData = 2 | (2 << 2) | ((u64)tex->width << 14) | ((u64)tex->height << 34);
        break;
    }
    *miptbp1Data = 0;
    *miptbp2Data = 0;
    Prim_SendPacket((u32 *) &gScreenTexPacket);
}

/* Takes the screen texture's GS block and sends the 512x448 32-bit image
 * pixels (alpha set to 0x80 = 1.0 unless it already is) there; 0 when it
 * does not fit. */
s32 ScreenTex_Reserve(u32 *pixels)
{
    s32 i;

    gScreenTexVram = Vram_AllocBlock(0xE0000) & 0xFFFF;
    if (*(volatile s32 *)&gScreenTexVram == 0) {
        return 0;
    }
    if ((*pixels & 0xFF000000) != 0x80000000) {
        FlushCache(0);
        Dma_WaitIdle();
        for (i = 0; i < 0x38000; i += 8) {
            *pixels = (*pixels & 0xFFFFFF) | 0x80000000;
            pixels++;
            *pixels = (*pixels & 0xFFFFFF) | 0x80000000;
            pixels++;
            *pixels = (*pixels & 0xFFFFFF) | 0x80000000;
            pixels++;
            *pixels = (*pixels & 0xFFFFFF) | 0x80000000;
            pixels++;
            *pixels = (*pixels & 0xFFFFFF) | 0x80000000;
            pixels++;
            *pixels = (*pixels & 0xFFFFFF) | 0x80000000;
            pixels++;
            *pixels = (*pixels & 0xFFFFFF) | 0x80000000;
            pixels++;
            *pixels = (*pixels & 0xFFFFFF) | 0x80000000;
            pixels++;
        }
    }
    gScreenTexState = 1;
    if (ScreenTex_Upload() != 0) {
        gScreenTexState = 2;
        return 1;
    }
    gScreenTexState = 0;
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
    printf(D_001C7C30, path);
}

/* Reads the file cdrom0:NAME;1 (upper-cased) into buf; 0 when it cannot
 * be opened. */
s32 File_Read(const char *name, void *buf, s32 size)
{
    char path[0x800];
    char *nameStart;
    s32 fileDesc;

    strcpy(path, D_001C7C48);
    nameStart = path + strlen(path);
    strcat(path, name);
    strupr(nameStart);
    strcat(path, D_001C7C50);
    func_00161EC0();
    fileDesc = sceOpen(path, 1);
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
    char path[0x800];
    char *nameStart;
    s32 fileDesc;

    strcpy(path, D_001C7C48);
    nameStart = path + strlen(path);
    strcat(path, name);
    strupr(nameStart);
    strcat(path, D_001C7C50);
    func_00161EC0();
    if ((fileDesc = sceOpen(path, 0x602)) < 0) {
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
    char path[0x800];
    char *nameStart;
    s32 fileDesc;

    strcpy(path, D_001C7C48);
    nameStart = path + strlen(path);
    strcat(path, name);
    strupr(nameStart);
    strcat(path, D_001C7C50);
    func_00161EC0();
    if ((fileDesc = sceOpen(path, 2)) < 0) {
        func_00161ED8();
        return 0;
    }
    sceLseek(fileDesc, 0, 2);
    sceWrite(fileDesc, buf, size);
    sceClose(fileDesc);
    func_00161ED8();
    return 1;
}

/* The size of the file, 0 when it cannot be opened. */
s32 File_GetSize(const char *name)
{
    char path[0x800];
    char *nameStart;
    s32 fileDesc;
    s32 size;

    strcpy(path, D_001C7C48);
    nameStart = path + strlen(path);
    strcat(path, name);
    strupr(nameStart);
    strcat(path, D_001C7C50);
    func_00161EC0();
    if ((fileDesc = sceOpen(path, 1)) < 0) {
        func_00161ED8();
        return 0;
    }
    size = sceLseek(fileDesc, 0, 2);
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
char D_001C7C30[0x18] = {
#include "main/001C7C30.inc"
};
char D_001C7C48[0x8] = {
#include "main/001C7C48.inc"
};
char D_001C7C50[0x10] = {
#include "main/001C7C50.inc"
};
u32 gPadConfigDefault[12] = {
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
