/*
 * Movie player: the game's layer over CRI Sofdec (MWSFD), which plays the
 * movies in AFS02.
 *
 * Movie_* are the entry points the other modules call: Movie_Play(id) looks
 * the movie up in gMovieDescs and starts it, Movie_Update() runs once per
 * frame, Movie_GetStatus() reports the Sofdec status (MOVIE_STATUS_*).
 * MovieSfd_* drive the Sofdec handle: the player is created on first use in
 * an 8 MB block from the game's heap (MovieWork at its start, then
 * Sofdec's work area and the frame's DMA chain), and each decoded frame is
 * uploaded to VRAM by DMA and drawn as a sprite.
 */
#include "common.h"
#include "syssettings.h"
#include "dma.h"
#include "hw_regs.h"
#include "movie.h"
#include "prim.h"
#include "stream.h"
#include "display.h"
#include "mem_util.h"
#include "libc.h"
#include "libgraph.h"
#include "libkernl.h"

#define MOVIE_HEAP_SIZE 8388608         /* the heap block: 8 MB */
#define MOVIE_SFD_WORK_SIZE 8298048     /* Sofdec's work area in it */
#define MOVIE_CHAIN_SIZE 90112          /* the frame upload chain after it */
#define MOVIE_PACKET_SIZE 192           /* each of the two sprite packets after that */
#define MOVIE_AFS_PARTITION 2           /* the movies are files of AFS02 */
#define MOVIE_VOLUME_MIN (-960)         /* in 0.1 dB: -96 dB */

extern s32 gSysFbWidth; /* main: gSysWork.fbWidth */

/* A Sofdec handle: an object whose first word points to its method table. */
typedef struct MwPly MwPly;

typedef struct MwPlyIf {
    void *unk00[5];
    void (*destroy)(MwPly *ply);            /* 0x14 */
    void *unk18;
    void (*stop)(MwPly *ply);               /* 0x1C */
    s32 (*getStat)(MwPly *ply);             /* 0x20 */
    void *unk24[2];
    void (*setOutVol)(MwPly *ply, s32 vol); /* 0x2C: in 0.1 dB */
} MwPlyIf;

struct MwPly {
    MwPlyIf *vtbl;
};

/* Sofdec creation parameters (48 bytes). */
typedef struct MwsfdCrePrm {
    s32 ftype;          /* 0x00 */
    s32 maxBps;         /* 0x04: bits per second */
    s32 maxWidth;       /* 0x08 */
    s32 maxHeight;      /* 0x0C */
    s32 nfrmPoolWk;     /* 0x10 */
    s32 unk14;
    u32 work;           /* 0x18: work area */
    u32 workSize;       /* 0x1C */
    s32 unk20;
    s32 unk24[3];
} MwsfdCrePrm;

/* A decoded frame, as Sofdec's get-frame call returns it (136 bytes). */
typedef struct MwsfdFrmInf {
    u32 bufadr;         /* 0x00: pixels, or 0 when no frame is ready */
    s32 unk04;
    s32 width;          /* 0x08 */
    s32 height;         /* 0x0C */
    s32 unk10[4];
    s32 frameNo;        /* 0x20: the frame's number (gMovieFrame) */
    s32 unk24;
    s64 unk28[12];
} MwsfdFrmInf;

typedef struct MoviePlayer {
    MwPly *ply;             /* 0x00 */
    MwsfdCrePrm cprm;       /* 0x04 */
    u8 unk34[4];
    MwsfdFrmInf frame;      /* 0x38: the last frame drawn */
    u8 status;              /* 0xC0: Sofdec status, MOVIE_STATUS_* */
    u8 unkC1[11];
    u16 width;              /* 0xCC: picture size */
    u16 height;             /* 0xCE */
    u16 dispW;              /* 0xD0: sprite rectangle on screen */
    u16 dispH;              /* 0xD2 */
    u16 dispX;              /* 0xD4 */
    u16 dispY;              /* 0xD6 */
    u8 unkD8;
    u8 unkD9;
    u8 unkDA[4];
    u16 waitFrames;         /* 0xDE: frames spent preparing */
    u8 unkE0[8];
} MoviePlayer;

/* The start of the 8 MB heap block. */
typedef struct MovieWork {
    u32 work;           /* 0x00: Sofdec work area (64-byte aligned) */
    u32 workSize;       /* 0x04 */
    u16 width;          /* 0x08 */
    u16 height;         /* 0x0A */
    u32 chain;          /* 0x0C: frame upload DMA chain, after the work area */
    u32 packet[2];      /* 0x10: two sprite packets, after the chain */
    u32 tbp;            /* 0x18: VRAM address of the frame */
    u32 packetIndex;    /* 0x1C: packet used next */
    u32 blocks;         /* 0x20: the first block packet of the chain */
    u32 chainSize;      /* 0x24 */
} MovieWork;

extern s32 gFlipCountdown;       /* boot ELF */
extern DmaChan *gDmaChan[10];    /* boot ELF */
extern s32 gMovieTexWidth;
extern s32 gMovieTexHeight;
extern s32 gMovieFrame;
extern u32 gMovieFramePixels;
extern u32 gMovieTexTbp;
extern s32 gMovieTexBytes;
extern MovieWork *gMovieWork;
extern void *gMovieHeapHandle;
extern MovieWork *gMovieHeapBlock;
extern MoviePlayer *gMoviePlayer;
extern MoviePlayer gMoviePlayerData;
extern MovieDesc gMovieDescs[];

MwPly *mwPlyCreateSofdec(MwsfdCrePrm *cprm);
void mwPlyStartAfs(MwPly *ply, s32 partition, s32 entry);
void mwPlySetFrmSync(MwPly *ply, s32 unk);
void mwPlyGetCurFrm(MwPly *ply, MwsfdFrmInf *frame);   /* get the current frame */
void mwPlyRelCurFrm(MwPly *ply);                       /* release the current frame */
u32 mwPlyCalcWorkCprmSfd(MwsfdCrePrm *cprm);                 /* work area size */
void mwPlyGetPlyInf(MwPly *ply, void *buf);
u16 Vram_AllocBlock(u32 size);                          /* VRAM: allocate */
/* unit's own declaration: declared as this file's callers use it; with the definition's void (u32 addr), Movie_FreeFrame compiles differently */
void Vram_FreeBlock(u16 tbp);                          /* VRAM: free */
s32 sceDmaSend(DmaChan *chan, u32 tag);

/* Flips three frames, then allocates VRAM for a width x height RGBA frame
 * (height rounded up to 32 lines). */
void Movie_AllocFrame(s32 width, s32 height)
{
    s32 i;
    s32 alignedHeight;

    for (i = 0; i < 3; i++) {
        gFlipCountdown = 2;
        Sys_FlipFrame(0);
    }
    alignedHeight = height;
    if (height % 32 != 0) {
        alignedHeight = (height / 32 + 1) * 32;
    }
    gMovieTexBytes = width * alignedHeight * 4;
    gMovieTexTbp = (u16)Vram_AllocBlock(gMovieTexBytes);
    gMovieTexWidth = width;
    gMovieTexHeight = height;
}

/* Waits for the frame DMA, flips three frames and frees the frame VRAM. */
void Movie_FreeFrame(void)
{
    s32 i;

    if (gMovieTexTbp != 0) {
        Dma_WaitIdle();
        for (i = 0; i < 3; i++) {
            gFlipCountdown = 2;
            Sys_FlipFrame(0);
        }
        if (gMovieTexTbp != 0) {
            Vram_FreeBlock(gMovieTexTbp);
        }
        gMovieTexTbp = 0;
        gMovieTexBytes = 0;
    }
}

/* A GIF packet cursor, written as 32-bit or 64-bit words. */
typedef union MoviePkt {
    u32 *w;
    u64 *d;
} MoviePkt;

#define PKT_W(pk, a, b) (*(pk).w++ = (a), *(pk).w++ = (b))
#define PKT_D(pk, a, b) (*(pk).d++ = (a), *(pk).d++ = (b))

/* Builds the DMA chain that uploads a width x height frame at buf to VRAM
 * tbp (psm) in 16x16 blocks: a TEXFLUSH/BITBLTBUF/TRXREG header, then per
 * block a TRXPOS/TRXDIR and an image transfer referencing buf (64
 * qwords: 16 by 16 pixels). gMovieWork->blocks gets the first block's
 * address. Returns the end. The GIF tags: three A+D qwords, then one per
 * block, then an image of 64 qwords. */
u32 Movie_BuildUploadChain(u32 chain, void *buf, s32 tbp, s32 psm, s32 width, s32 height)
{
    MoviePkt pkt;
    s32 i;
    s32 j;
    s32 bufWidth;

    pkt.w = (u32 *)chain;
    bufWidth = gMovieTexWidth / 64;
    PKT_W(pkt, DMA_TAG_CNT | 4, 0);
    PKT_W(pkt, 0, 0);
    PKT_D(pkt, GIF_TAG_NREG(1) | GIF_TAG_EOP | 3, GIF_REG_AD);
    PKT_D(pkt, 0, GS_REG_TEXFLUSH);
    PKT_D(pkt, ((u64)psm << 56) | (((u64)tbp << 32) | ((u64)bufWidth << 48)), GS_REG_BITBLTBUF);
    PKT_D(pkt, 0x10 | ((u64)16 << 32), GS_REG_TRXREG);
    gMovieWork->blocks = (u32)pkt.w;
    for (i = 0; i < width / 16; i++) {
        for (j = 0; j < height / 16; j++) {
            PKT_W(pkt, DMA_TAG_CNT | 4, 0);
            PKT_W(pkt, 0, 0);
            PKT_D(pkt, GIF_TAG_NREG(1) | 2, GIF_REG_AD);
            PKT_D(pkt, ((u64)(i * 16) << 32) | ((u64)(j * 16) << 48), GS_REG_TRXPOS);
            PKT_D(pkt, 0, GS_REG_TRXDIR);
            PKT_D(pkt, 0x0800000000008040, 0);
            PKT_W(pkt, DMA_TAG_REF | 0x40, (u32)buf);
            PKT_W(pkt, 0, 0x51000040); /* VIF DIRECTHL, 64 qwords */
        }
    }
    PKT_W(pkt, DMA_TAG_END, 0);
    PKT_W(pkt, 0, 0);
    return (u32)pkt.w;
}
#undef PKT_W
#undef PKT_D

/* Points the 96-byte packets of the DMA chain (their word at +0x54) at the image's
 * 16x16 blocks of 1024 bytes, row by row, skipping skip pixels after each row. */
void Movie_SetupBlockPackets(u32 chain, u32 *pixels, s32 width, s32 height, s32 skip)
{
    s32 i;
    s32 j;
    u32 *word = (u32 *)chain;

    for (i = 0; i < width / 16; i++) {
        for (j = 0; j < height / 16; j++) {
            word += 21;
            *word++ = (u32)pixels;
            word += 2;
            pixels += 256;
        }
        pixels += skip;
    }
}

/* Writes at packet the sprite packet that draws the frame texture at (x,
 * y), width x height, depth z: the alpha, clamp, texture flush, TEX0 and
 * TEX1 registers, then a sprite. The texture's size is rounded up to powers
 * of two (a height between 256 and 512 to 512). Returns the packet's end. */
u32 MovieSfd_FramePacket(u32 packet, s32 x, s32 y, s32 z, s32 width, s32 height)
{
    s32 texWidthLog2;
    s32 ofs;
    s32 size;
    s32 odd;
    s32 drawHeight;
    s32 texHeightLog2;
    s32 i;
    u32 *tagWord;
    u64 *reg;

    i = 0;
    size = gMovieTexWidth;
    while (size > 1) {
        odd = size % 2;
        size = size / 2;
        i++;
    }
    /* odd is never set when the width is below 2: the game's code reads it
     * unset there too */
    if (odd != 0) {
        i++;
    }
    texWidthLog2 = i;
    for (i = 0, size = 1; i < texWidthLog2; i++) {
        size <<= 1;
    }
    if (size < gMovieTexWidth) {
        texWidthLog2++;
    }
    size = gMovieTexHeight;
    texHeightLog2 = 0;
    if (size > 256 && size < 512) {
        size = 512;
    }
    while (size > 1) {
        odd = size % 2;
        size = size / 2;
        texHeightLog2++;
    }
    if (odd != 0) {
        texHeightLog2++;
    }
    drawHeight = gMovieTexHeight;
    if (drawHeight > 448) {
        drawHeight = 448;
    }
    tagWord = (u32 *)packet;
    *tagWord++ = DMA_TAG_END | DMA_TAG_IRQ | 0xA;
    *tagWord++ = 0;
    *tagWord++ = 0;
    *tagWord++ = VIF_DIRECTHL | 10; /* 10 qwords */
    reg = (u64 *)tagWord;
    *reg++ = GIF_TAG_NREG(1) | 5; /* five A+D qwords */
    *reg++ = GIF_REG_AD;
    *reg++ = 68;
    *reg++ = GS_REG_ALPHA_1;
    *reg++ = 5;
    *reg++ = GS_REG_CLAMP_1;
    *reg++ = 0;
    *reg++ = GS_REG_TEXFLUSH;
    *reg++ = (u64)gMovieWork->tbp | ((u64)(gMovieTexWidth / 64) << 14) | ((u64)texWidthLog2 << 26)
        | ((u64)texHeightLog2 << 30) | ((u64)1 << 35);
    *reg++ = GS_REG_TEX0_1;
    *reg++ = TEX1_MMAG_LINEAR | TEX1_MMIN(TEX1_MIN_LINEAR);
    *reg++ = GS_REG_TEX1_1;
    *reg++ = 0x6400000000008001; /* the sprite: PRIM, RGBAQ, UV, XYZ2, UV, XYZ2 */
    *reg++ = 0x535310;
    ofs = ((4096 - gSysFbWidth) / 2) * 16;
    *reg++ = 278;
    *reg++ = 0x3F80000080808080;
    *reg++ = 0x80008;
    *reg++ = (u64)(ofs + x * 16) | ((u64)(y * 16 + 29184) << 16) | ((u64)z << 32);
    *reg++ = (u64)((gMovieTexWidth - 1) * 16 + 8) | ((u64)((drawHeight - 1) * 16 + 8) << 16);
    *reg++ = (u64)(ofs + (x + width) * 16) | ((u64)((y + height) * 16 + 29184) << 16) | ((u64)z << 32);
    return (u32)reg;
}

/* Builds the frame upload chain and the two sprite packets. */
void MovieSfd_BuildPackets(void)
{
    u8 buf[1024];
    s32 height;

    gMovieWork->tbp = gMovieTexTbp;
    memset(buf, 0, sizeof(buf));
    gMovieWork->packet[0] = gMovieWork->chain + gMovieWork->chainSize;
    gMovieWork->packet[1] = gMovieWork->chain + gMovieWork->chainSize + MOVIE_PACKET_SIZE;
    height = (gMovieTexHeight > 448) ? 448 : gMovieTexHeight;
    Movie_BuildUploadChain(gMovieWork->chain, buf, gMovieWork->tbp, 0, gMovieTexWidth, height);
    MovieSfd_FramePacket(gMovieWork->packet[0], gMoviePlayer->dispX, gMoviePlayer->dispY, 1,
                  gMoviePlayer->dispW, gMoviePlayer->dispH);
    MovieSfd_FramePacket(gMovieWork->packet[1], gMoviePlayer->dispX, gMoviePlayer->dispY, 1,
                  gMoviePlayer->dispW, gMoviePlayer->dispH);
}

/*
 * Uploads a decoded frame to VRAM (unless setupOnly) and draws the sprite
 * packets, alternating between the two.
 */
void MovieSfd_Draw(u32 *pixels, s32 width, s32 height, s32 setupOnly)
{
    s32 skip;
    u32 packet;

    if (gMovieWork->tbp != gMovieTexTbp) {
        MovieSfd_BuildPackets();
    }
    if (setupOnly == 1) {
        return;
    }
    if (pixels != NULL) {
        skip = 0;
        if (gMovieTexHeight == 480) {
            skip = 256;
        }
        if (gMovieTexHeight == 512) {
            skip = 512;
        }
        pixels += skip;
        gMovieFramePixels = (u32)pixels;
        if (height > 448) {
            height = 448;
        }
        Movie_SetupBlockPackets(gMovieWork->blocks, pixels, width, height, skip * 2);
        gDmaChan[2]->chcr.tte = 0;
        FlushCache(0);
        Dma_WaitIdle();
        sceDmaSend(gDmaChan[2], gMovieWork->chain);
        sceGsSyncPath(0, 0);
    }
    if (gMovieFramePixels != 0) {
        if (gMovieWork->packetIndex != 0) {
            packet = gMovieWork->packet[0];
        } else {
            packet = gMovieWork->packet[1];
        }
        Dma_WaitIdle();
        Prim_SendPacket((u32 *)packet);
        gMovieWork->packetIndex ^= 1;
    }
}

/* Rebuilds the packets, then uploads and draws a decoded frame. */
void MovieSfd_DrawFrame(u32 *pixels, s32 width, s32 height)
{
    MovieSfd_BuildPackets();
    MovieSfd_Draw(pixels, width, height, 0);
}

/* Sets the player's picture size, rebuilding the packets when it changes,
 * and sets up the drawing without uploading. */
void MovieSfd_SetFrameSize(u32 *pixels, s32 frameWidth, s32 frameHeight)
{
    u16 *curWidth = &gMoviePlayer->width;

    if (frameWidth != *curWidth || frameHeight != gMoviePlayer->height) {
        *curWidth = frameWidth;
        gMoviePlayer->height = frameHeight;
        MovieSfd_BuildPackets();
    }
    MovieSfd_Draw(pixels, frameWidth, frameHeight, 1);
}

/* Takes the 8 MB heap block (allocated once) and lays out MovieWork at
 * its start: Sofdec's work area 64-byte aligned after it, then the upload
 * chain. gMovieWork stays NULL when the block can't be had. */
void MovieSfd_AllocWork(s32 frameWidth, s32 frameHeight)
{
    gMovieWork = NULL;
    Mem_Compact();
    if (gMovieHeapHandle == NULL) {
        gMovieHeapHandle = Mem_Alloc(MOVIE_HEAP_SIZE, 1);
        if (gMovieHeapHandle == NULL) {
            return;
        }
    }
    gMovieHeapBlock = Mem_GetAddr(gMovieHeapHandle);
    gMovieWork = gMovieHeapBlock;
    memset(gMovieWork, 0, MOVIE_HEAP_SIZE);
    gMovieWork->work = ((u32)gMovieWork + 107) & ~0x3F;
    gMovieWork->chainSize = MOVIE_CHAIN_SIZE;
    gMovieWork->workSize = MOVIE_SFD_WORK_SIZE;
    gMovieWork->chain = gMovieWork->work + gMovieWork->workSize;
    gMovieWork->width = frameWidth;
    gMovieWork->height = frameHeight;
}

/* Frees the frame's VRAM and the heap block. */
void MovieSfd_FreeWork(void)
{
    Movie_FreeFrame();
    if (gMovieHeapHandle != NULL) {
        Mem_Free(gMovieHeapHandle);
        Mem_Compact();
    }
    gMovieWork = NULL;
    gMovieHeapHandle = NULL;
    gMovieHeapBlock = NULL;
}

/*
 * Creates the Sofdec handle in the heap block and sets up the frame
 * packets. The parameter is untyped (void *) and desc its typed copy,
 * declared last: the compiler numbers that copy as a local, which gives
 * desc and frameHeight/cprm the registers the game's code uses (a
 * MovieDesc * parameter swaps them).
 */
void MovieSfd_Create(void *arg)
{
    u16 frameWidth;
    u16 frameHeight;
    MwsfdCrePrm *cprm;
    MovieDesc *desc = (MovieDesc *)arg;

    frameWidth = desc->width;
    frameHeight = desc->height;

    gMovieFrame = 0;
    MovieSfd_AllocWork(frameWidth, frameHeight);
    if (gMovieHeapBlock == NULL) {
        return;
    }
    gMoviePlayer = &gMoviePlayerData;
    gMoviePlayer->width = frameWidth;
    gMoviePlayer->height = frameHeight;
    switch (desc->mode) {
    case MOVIE_MODE_FULL:
        gMoviePlayer->dispW = 512;
        gMoviePlayer->dispH = 448;
        gMoviePlayer->dispX = 0;
        gMoviePlayer->dispY = 0;
        break;
    case MOVIE_MODE_SMALL:
        gMoviePlayer->dispW = 256;
        gMoviePlayer->dispH = 224;
        break;
    case MOVIE_MODE_WIDE:
        gMoviePlayer->dispW = 512;
        gMoviePlayer->dispH = 432;
        gMoviePlayer->dispX = 0;
        gMoviePlayer->dispY = 8;
        break;
    }
    cprm = &gMoviePlayer->cprm;
    memset(cprm, 0, sizeof(MwsfdCrePrm));
    cprm->unk20 = 17;
    cprm->ftype = desc->ftype;
    cprm->nfrmPoolWk = 2;
    cprm->maxBps = 1000.0f * (s32)desc->maxKbps;
    cprm->maxWidth = gMovieWork->width;
    cprm->maxHeight = gMovieWork->height;
    cprm->unk14 = 2;
    cprm->workSize = mwPlyCalcWorkCprmSfd(cprm);
    if (cprm->workSize >= gMovieWork->workSize) {
        MovieSfd_FreeWork();
        return;
    }
    cprm->work = gMovieWork->work;
    Movie_AllocFrame(gMovieWork->width, gMovieWork->height);
    if (gMovieHeapHandle == NULL) {
        MovieSfd_FreeWork();
        return;
    }
    gMovieFramePixels = 0;
    gMovieWork->tbp = 0;
    MovieSfd_SetFrameSize(NULL, gMoviePlayer->width, gMoviePlayer->height);
    gMoviePlayer->ply = mwPlyCreateSofdec(cprm);
    if (gMoviePlayer->ply == NULL) {
        MovieSfd_FreeWork();
        return;
    }
    mwPlySetFrmSync(gMoviePlayer->ply, 0);
    gMoviePlayer->unkD9 = 0;
    gMoviePlayer->waitFrames = 0;
}

/* Starts a movie: creates the player if there is none, sets its volume
 * from the music volume option, and starts the movie's file. */
void MovieSfd_Start(MovieDesc *desc)
{
    MwPly *ply;
    s32 vol;

    gMoviePlayer = &gMoviePlayerData;
    ply = gMoviePlayer->ply;
    if (ply == NULL) {
        MovieSfd_Create(desc);
        ply = gMoviePlayer->ply;
        if (ply == NULL) {
            return;
        }
    }
    vol = gVolumeDb[gVolumeMax[gSysSettings.musicVolume]];
    if (vol < MOVIE_VOLUME_MIN) {
        vol = MOVIE_VOLUME_MIN;
    }
    ply->vtbl->setOutVol(ply, vol);
    mwPlyStartAfs(gMoviePlayer->ply, MOVIE_AFS_PARTITION, desc->entry);
}

/* Asks the player, if any, to stop. */
void MovieSfd_RequestStop(void)
{
    MwPly *ply;

    gMoviePlayer = &gMoviePlayerData;
    ply = gMoviePlayer->ply;
    if (ply != NULL) {
        ply->vtbl->stop(ply);
    }
}

/* Stops and destroys the player, then frees the frame and the heap
 * block. */
void MovieSfd_Stop(void)
{
    MwPly *ply;

    gMoviePlayer = &gMoviePlayerData;
    ply = gMoviePlayer->ply;
    if (ply != NULL) {
        ply->vtbl->stop(ply);
        ply = gMoviePlayer->ply;
        ply->vtbl->destroy(ply);
        gMoviePlayer->ply = NULL;
    }
    Movie_FreeFrame();
    MovieSfd_FreeWork();
}

/* Once per frame: reads the player's status. Preparing too long stops it;
 * playing draws the current frame and keeps it; finished asks it to stop;
 * an error stops it. */
void MovieSfd_Update(void)
{
    MwsfdFrmInf frame;
    u8 buf[32];

    if (gMoviePlayer != NULL && gMoviePlayer->ply != NULL) {
        mwPlyGetPlyInf(gMoviePlayer->ply, buf);
        gMoviePlayer->status = gMoviePlayer->ply->vtbl->getStat(gMoviePlayer->ply);
        switch (gMoviePlayer->status) {
        case MOVIE_STATUS_STOPPED:
            break;
        case MOVIE_STATUS_PREPARING:
            gMoviePlayer->waitFrames++;
            if (gMoviePlayer->waitFrames >= MOVIE_PREPARE_FRAMES) {
                MovieSfd_Stop();
            }
            break;
        case MOVIE_STATUS_PLAYING:
            mwPlyGetCurFrm(gMoviePlayer->ply, &frame);
            MovieSfd_DrawFrame((u32 *)frame.bufadr, gMoviePlayer->width, gMoviePlayer->height);
            if (frame.bufadr != 0) {
                gMoviePlayer->width = frame.width;
                gMoviePlayer->height = frame.height;
                gMovieFrame = frame.frameNo;
                gMoviePlayer->frame = frame;
                mwPlyRelCurFrm(gMoviePlayer->ply);
            }
            break;
        case MOVIE_STATUS_FINISHED:
            MovieSfd_RequestStop();
            break;
        case MOVIE_STATUS_ERROR:
            MovieSfd_Stop();
            break;
        }
    }
}

/* The player's status, MOVIE_STATUS_STOPPED when there is none. */
s32 MovieSfd_GetStatus(void)
{
    if (gMoviePlayer == NULL) {
        return MOVIE_STATUS_STOPPED;
    }
    if (gMoviePlayer->ply == NULL) {
        return MOVIE_STATUS_STOPPED;
    }
    return gMoviePlayer->status;
}

/* Called before Movie_Play; empty in this build. */
void Movie_PrePlay(void)
{
}

/* Starts movie id of gMovieDescs; returns 0. */
s32 Movie_Play(s32 id)
{
    MovieSfd_Start(&gMovieDescs[id]);
    return 0;
}

/* Called after Movie_Play with (0, 0); empty in this build, so every movie is full screen. */
void Movie_SetPosition(s32 x, s32 y)
{
}

void Movie_RequestStop(void)
{
    MovieSfd_RequestStop();
}

void Movie_Stop(void)
{
    MovieSfd_Stop();
}

s32 Movie_GetStatus(void)
{
    return MovieSfd_GetStatus();
}

/* Runs the player once per frame; returns 0. */
s32 Movie_Update(void)
{
    MovieSfd_Update();
    return 0;
}
