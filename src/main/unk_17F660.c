/*
 * main 0x17f660-0x17fe20: the screen texture sent to GS memory and the GS
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

/* Sends the pixels of t (32-bit) to its GS address. */
void ScreenTex_Send(TexRec *t)
{
    u128 *p;
    u32 src;
    s32 w;
    s32 h;
    u32 size;
    u128 *last;
    s32 vram;

    w = t->width;
    h = t->height;
    src = (u32)t->mem;
    size = w * h * 4;
    p = Packet_Alloc(Dma_ImagePacketSize(size), 16);
    vram = t->vram;
    last = Dma_BuildLoadImage(p, 1, src, size, vram, w / 64, t->psm, 0, 0, w, h);
    DmaQueue_AddChain(0, ((u32)p & 0x0FFFFFFF) | 0x10000000, (u32 *)last, &gDmaQueue);
}

/* Sets the texture registers for drawing with the screen texture. */
void ScreenTex_SetRegs(void)
{
    s32 flags = gRenderFlags;
    TexRec *t = &gScreenTex;
    u64 *texa = &gScreenTexPacket.texa.data;
    u64 *tex0 = &gScreenTexPacket.tex0.data;
    u64 *clamp = &gScreenTexPacket.clamp.data;
    u64 *mip1 = &gScreenTexPacket.miptbp1.data;
    u64 *mip2 = &gScreenTexPacket.miptbp2.data;
    s16 bw;

    gScreenTexPacket.tex1.data = 0;
    bw = gScreenTex.width / 64;
    if (bw == 0) {
        bw = 1;
    }
    *texa = (u64)0x80 << 32;   /* TA1 0x80: alpha 1.0 */
    /* TBP0, TBW, PSM, TW, TH; TCC 1 (RGBA) */
    *tex0 = (u64)t->vram | ((u64)bw << 14) | ((u64)t->psm << 20) | ((u64)t->tw << 26) |
            ((u64)t->th << 30) | ((u64)1 << 34);
    switch (flags & 0x60000) {
    case 0:
        *clamp = 0;
        break;
    case 0x20000:
    default:
        /* REGION_CLAMP on both axes, 0..width, 0..height */
        *clamp = 2 | (2 << 2) | ((u64)t->width << 14) | ((u64)t->height << 34);
        break;
    }
    *mip1 = 0;
    *mip2 = 0;
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
    char *p;
    s32 fd;

    strcpy(path, D_001C7C48);
    p = path + strlen(path);
    strcat(path, name);
    strupr(p);
    strcat(path, D_001C7C50);
    func_00161EC0();
    fd = sceOpen(path, 1);
    if (fd < 0) {
        func_00161ED8();
        return 0;
    }
    sceRead(fd, buf, size);
    sceClose(fd);
    func_00161ED8();
    return 1;
}

/* Creates (or truncates) the file and writes buf to it. */
s32 File_Write(const char *name, const void *buf, s32 size)
{
    char path[0x800];
    char *p;
    s32 fd;

    strcpy(path, D_001C7C48);
    p = path + strlen(path);
    strcat(path, name);
    strupr(p);
    strcat(path, D_001C7C50);
    func_00161EC0();
    if ((fd = sceOpen(path, 0x602)) < 0) {
        func_00161ED8();
        return 0;
    }
    sceWrite(fd, buf, size);
    sceClose(fd);
    func_00161ED8();
    return 1;
}

/* Appends buf to the file. */
s32 File_Append(const char *name, const void *buf, s32 size)
{
    char path[0x800];
    char *p;
    s32 fd;

    strcpy(path, D_001C7C48);
    p = path + strlen(path);
    strcat(path, name);
    strupr(p);
    strcat(path, D_001C7C50);
    func_00161EC0();
    if ((fd = sceOpen(path, 2)) < 0) {
        func_00161ED8();
        return 0;
    }
    sceLseek(fd, 0, 2);
    sceWrite(fd, buf, size);
    sceClose(fd);
    func_00161ED8();
    return 1;
}

/* The size of the file, 0 when it cannot be opened. */
s32 File_GetSize(const char *name)
{
    char path[0x800];
    char *p;
    s32 fd;
    s32 size;

    strcpy(path, D_001C7C48);
    p = path + strlen(path);
    strcat(path, name);
    strupr(p);
    strcat(path, D_001C7C50);
    func_00161EC0();
    if ((fd = sceOpen(path, 1)) < 0) {
        func_00161ED8();
        return 0;
    }
    size = sceLseek(fd, 0, 2);
    sceClose(fd);
    func_00161ED8();
    return size;
}

/* unk_17F660's .data (0x1BA990-0x1BAA10) */
ScreenTexPacket gScreenTexPacket = {
#include "main/001BA990.inc"
};

/* unk_17F660's .rodata (0x1C7C30-0x1C7CF8) */
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
