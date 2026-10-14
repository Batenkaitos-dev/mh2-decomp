/*
 * Movie player: the game's layer over CRI Sofdec (MWSFD), which plays the
 * movies in AFS02.
 *
 * Movie_* are the entry points the other modules call: Movie_Play(id) looks
 * the movie up in gMovieDescs and starts it, Movie_Update() runs once per
 * frame, Movie_GetStatus() reports the Sofdec status (0 stopped, 1 preparing,
 * 2 playing, 3 finished, 4 error). MovieSfd_* drive the Sofdec handle: the
 * player is created on first use in an 8 MB block from the game's heap, and
 * each decoded frame is uploaded to VRAM by DMA and drawn as a sprite.
 */
#include "common.h"
#include "syssettings.h"
#include "dma.h"

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

/* Sofdec creation parameters (0x30 bytes). */
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

/* A decoded frame, as Sofdec's get-frame call returns it (0x88 bytes). */
typedef struct MwsfdFrmInf {
    u32 bufadr;         /* 0x00: pixels, or 0 when no frame is ready */
    s32 unk04;
    s32 width;          /* 0x08 */
    s32 height;         /* 0x0C */
    s32 unk10[4];
    s32 unk20;
    s32 unk24;
    s64 unk28[12];
} MwsfdFrmInf;

/* One movie: an entry of gMovieDescs (16 bytes). */
typedef struct MovieDesc {
    s32 entry;          /* 0x0: file in AFS02 */
    s32 ftype;          /* 0x4 */
    u16 maxKbps;        /* 0x8 */
    u16 width;          /* 0xA */
    u16 height;         /* 0xC */
    u16 mode;           /* 0xE: 0 = 512x448, 1 = 256x224, 2 = 512x432, 8 lines down */
} MovieDesc;

typedef struct MoviePlayer {
    MwPly *ply;             /* 0x00 */
    MwsfdCrePrm cprm;       /* 0x04 */
    u8 unk34[4];
    MwsfdFrmInf frame;      /* 0x38: the last frame drawn */
    u8 status;              /* 0xC0: Sofdec status */
    u8 unkC1[0xB];
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
extern u8 gVolumeMax[];       /* option value -> volume step */
extern s16 gVolumeDb[];      /* volume step -> 0.1 dB */

void *memset(void *dst, s32 c, u32 n);
MwPly *mwPlyCreateSofdec(MwsfdCrePrm *cprm);
void mwPlyStartAfs(MwPly *ply, s32 partition, s32 entry);
void mwPlySetFrmSync(MwPly *ply, s32 unk);
void mwPlyGetCurFrm(MwPly *ply, MwsfdFrmInf *frame);   /* get the current frame */
void mwPlyRelCurFrm(MwPly *ply);                       /* release the current frame */
u32 mwPlyCalcWorkCprmSfd(MwsfdCrePrm *cprm);                 /* work area size */
void mwPlyGetPlyInf(MwPly *ply, void *buf);
void *Mem_Alloc(u32 size, s32 unk);               /* heap: allocate a block */
void Mem_Free(void *handle);                     /* heap: free */
void *Mem_GetAddr(void *handle);               /* heap: address of a block */
void Mem_Compact(void);
s32 Dma_WaitIdle(void);                            /* waits for the frame DMA */
u16 Vram_AllocBlock(u32 size);                          /* VRAM: allocate */
/* unit's own declaration: with the definition's void (u32 addr) (main/texture.c), Movie_FreeFrame: 3 of 32 words differ */
void Vram_FreeBlock(u16 tbp);                          /* VRAM: free */
s32 Sys_FlipFrame(s32 unused0);                          /* flips a frame */
s32 Prim_SendPacket(u32 *pkt);                       /* draws a packet */
void FlushCache(s32 op);
s32 sceDmaSend(DmaChan *chan, u32 tag);
s32 sceGsSyncPath(s32 mode, u16 timeout);

void Movie_AllocFrame(s32 w, s32 h);
void Movie_FreeFrame(void);
u32 Movie_BuildUploadChain(u32 chain, void *buf, s32 tbp, s32 psm, s32 w, s32 h);
void Movie_SetupBlockPackets(u32 chain, u32 *pixels, s32 w, s32 h, s32 skip);
u32 MovieSfd_FramePacket(u32 packet, s32 x, s32 y, s32 unk, s32 w, s32 h);
void MovieSfd_Draw(u32 *pixels, s32 w, s32 h, s32 setupOnly);
void MovieSfd_Create(void *arg);

/* Flips three frames, then allocates VRAM for a w x h RGBA frame (height
 * rounded up to 32 lines). */
void Movie_AllocFrame(s32 w, s32 h)
{
    s32 i;
    s32 h32;

    for (i = 0; i < 3; i++) {
        gFlipCountdown = 2;
        Sys_FlipFrame(0);
    }
    h32 = h;
    if (h % 32 != 0) {
        h32 = (h / 32 + 1) * 32;
    }
    gMovieTexBytes = w * h32 * 4;
    gMovieTexTbp = (u16)Vram_AllocBlock(gMovieTexBytes);
    gMovieTexWidth = w;
    gMovieTexHeight = h;
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

/* Builds the DMA chain that uploads a w x h frame at buf to VRAM tbp (psm) in 16x16 blocks:
 * a TEXFLUSH/BITBLTBUF/TRXREG header, then per block a TRXPOS/TRXDIR and an image transfer
 * referencing buf. gMovieWork->blocks gets the first block's address. Returns the end. */
u32 Movie_BuildUploadChain(u32 chain, void *buf, s32 tbp, s32 psm, s32 w, s32 h)
{
    MoviePkt pk;
    s32 i;
    s32 j;
    s32 bw;

    pk.w = (u32 *)chain;
    bw = gMovieTexWidth / 64;
    PKT_W(pk, 0x10000004, 0);
    PKT_W(pk, 0, 0);
    PKT_D(pk, 0x1000000000008003, 0xE);
    PKT_D(pk, 0, 0x3F);
    PKT_D(pk, ((u64)psm << 56) | (((u64)tbp << 32) | ((u64)bw << 48)), 0x50);
    PKT_D(pk, 0x10 | ((u64)0x10 << 32), 0x52);
    gMovieWork->blocks = (u32)pk.w;
    for (i = 0; i < w / 16; i++) {
        for (j = 0; j < h / 16; j++) {
            PKT_W(pk, 0x10000004, 0);
            PKT_W(pk, 0, 0);
            PKT_D(pk, 0x1000000000000002, 0xE);
            PKT_D(pk, ((u64)(i * 16) << 32) | ((u64)(j * 16) << 48), 0x51);
            PKT_D(pk, 0, 0x53);
            PKT_D(pk, 0x0800000000008040, 0);
            PKT_W(pk, 0x30000040, (u32)buf);
            PKT_W(pk, 0, 0x51000040);
        }
    }
    PKT_W(pk, 0x70000000, 0);
    PKT_W(pk, 0, 0);
    return (u32)pk.w;
}
#undef PKT_W
#undef PKT_D

/* Points the 0x60-byte packets of the DMA chain (their word at +0x54) at the image's
 * 16x16 blocks of 0x400 bytes, row by row, skipping skip pixels after each row. */
void Movie_SetupBlockPackets(u32 chain, u32 *pixels, s32 w, s32 h, s32 skip)
{
    s32 i;
    s32 j;
    u32 *p = (u32 *)chain;

    for (i = 0; i < w / 16; i++) {
        for (j = 0; j < h / 16; j++) {
            p += 21;
            *p++ = (u32)pixels;
            p += 2;
            pixels += 0x100;
        }
        pixels += skip;
    }
}

/* MovieSfd_FramePacket: the sprite packet that draws the frame texture at (x, y), w x h, depth z. */
u32 MovieSfd_FramePacket(u32 packet, s32 x, s32 y, s32 z, s32 w, s32 h)
{
    s32 tw;
    s32 ofs;
    s32 v;
    s32 r;
    s32 th2;
    s32 th;
    s32 i;
    u32 *q;
    u64 *p;

    i = 0;
    v = gMovieTexWidth;
    while (v > 1) {
        r = v % 2;
        v = v / 2;
        i++;
    }
    /* r is never set when the width is below 2: the game reads t6 unset there too */
    if (r != 0) {
        i++;
    }
    tw = i;
    for (i = 0, v = 1; i < tw; i++) {
        v <<= 1;
    }
    if (v < gMovieTexWidth) {
        tw++;
    }
    v = gMovieTexHeight;
    th = 0;
    if (v > 0x100 && v < 0x200) {
        v = 0x200;
    }
    while (v > 1) {
        r = v % 2;
        v = v / 2;
        th++;
    }
    if (r != 0) {
        th++;
    }
    th2 = gMovieTexHeight;
    if (th2 > 0x1C0) {
        th2 = 0x1C0;
    }
    q = (u32 *)packet;
    *q++ = 0xF000000A;
    *q++ = 0;
    *q++ = 0;
    *q++ = 0x5100000A;
    p = (u64 *)q;
    *p++ = 0x1000000000000005;
    *p++ = 0xE;
    *p++ = 0x44;
    *p++ = 0x42;
    *p++ = 5;
    *p++ = 8;
    *p++ = 0;
    *p++ = 0x3F;
    *p++ = (u64)gMovieWork->tbp | ((u64)(gMovieTexWidth / 64) << 14) | ((u64)tw << 26)
        | ((u64)th << 30) | ((u64)1 << 35);
    *p++ = 6;
    *p++ = 0x60;
    *p++ = 0x14;
    *p++ = 0x6400000000008001;
    *p++ = 0x535310;
    ofs = ((0x1000 - gSysFbWidth) / 2) * 16;
    *p++ = 0x116;
    *p++ = 0x3F80000080808080;
    *p++ = 0x80008;
    *p++ = (u64)(ofs + x * 16) | ((u64)(y * 16 + 0x7200) << 16) | ((u64)z << 32);
    *p++ = (u64)((gMovieTexWidth - 1) * 16 + 8) | ((u64)((th2 - 1) * 16 + 8) << 16);
    *p++ = (u64)(ofs + (x + w) * 16) | ((u64)((y + h) * 16 + 0x7200) << 16) | ((u64)z << 32);
    return (u32)p;
}

/* Builds the frame upload chain and the two sprite packets. */
void MovieSfd_BuildPackets(void)
{
    u8 buf[0x400];
    s32 h;

    gMovieWork->tbp = gMovieTexTbp;
    memset(buf, 0, sizeof(buf));
    gMovieWork->packet[0] = gMovieWork->chain + gMovieWork->chainSize;
    gMovieWork->packet[1] = gMovieWork->chain + gMovieWork->chainSize + 0xC0;
    h = (gMovieTexHeight > 448) ? 448 : gMovieTexHeight;
    Movie_BuildUploadChain(gMovieWork->chain, buf, gMovieWork->tbp, 0, gMovieTexWidth, h);
    MovieSfd_FramePacket(gMovieWork->packet[0], gMoviePlayer->dispX, gMoviePlayer->dispY, 1,
                  gMoviePlayer->dispW, gMoviePlayer->dispH);
    MovieSfd_FramePacket(gMovieWork->packet[1], gMoviePlayer->dispX, gMoviePlayer->dispY, 1,
                  gMoviePlayer->dispW, gMoviePlayer->dispH);
}

/*
 * Uploads a decoded frame to VRAM (unless setupOnly) and draws the sprite
 * packets, alternating between the two.
 */
void MovieSfd_Draw(u32 *pixels, s32 w, s32 h, s32 setupOnly)
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
        if (h > 448) {
            h = 448;
        }
        Movie_SetupBlockPackets(gMovieWork->blocks, pixels, w, h, skip * 2);
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

void MovieSfd_DrawFrame(u32 *pixels, s32 w, s32 h)
{
    MovieSfd_BuildPackets();
    MovieSfd_Draw(pixels, w, h, 0);
}

void MovieSfd_SetFrameSize(u32 *pixels, s32 w, s32 h)
{
    u16 *width = &gMoviePlayer->width;

    if (w != *width || h != gMoviePlayer->height) {
        *width = w;
        gMoviePlayer->height = h;
        MovieSfd_BuildPackets();
    }
    MovieSfd_Draw(pixels, w, h, 1);
}

void MovieSfd_AllocWork(s32 w, s32 h)
{
    gMovieWork = NULL;
    Mem_Compact();
    if (gMovieHeapHandle == NULL) {
        gMovieHeapHandle = Mem_Alloc(0x800000, 1);
        if (gMovieHeapHandle == NULL) {
            return;
        }
    }
    gMovieHeapBlock = Mem_GetAddr(gMovieHeapHandle);
    gMovieWork = gMovieHeapBlock;
    memset(gMovieWork, 0, 0x800000);
    gMovieWork->work = ((u32)gMovieWork + 0x6B) & ~0x3F;
    gMovieWork->chainSize = 0x16000;
    gMovieWork->workSize = 0x7E9E40;
    gMovieWork->chain = gMovieWork->work + gMovieWork->workSize;
    gMovieWork->width = w;
    gMovieWork->height = h;
}

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
 * declared last: MWCC numbers that copy as a local, which puts desc in s0
 * and h/cprm in s1 as in the game (a MovieDesc * parameter swaps them).
 */
void MovieSfd_Create(void *arg)
{
    u16 w;
    u16 h;
    MwsfdCrePrm *cprm;
    MovieDesc *desc = (MovieDesc *)arg;

    w = desc->width;
    h = desc->height;

    gMovieFrame = 0;
    MovieSfd_AllocWork(w, h);
    if (gMovieHeapBlock == NULL) {
        return;
    }
    gMoviePlayer = &gMoviePlayerData;
    gMoviePlayer->width = w;
    gMoviePlayer->height = h;
    switch (desc->mode) {
    case 0:
        gMoviePlayer->dispW = 512;
        gMoviePlayer->dispH = 448;
        gMoviePlayer->dispX = 0;
        gMoviePlayer->dispY = 0;
        break;
    case 1:
        gMoviePlayer->dispW = 256;
        gMoviePlayer->dispH = 224;
        break;
    case 2:
        gMoviePlayer->dispW = 512;
        gMoviePlayer->dispH = 432;
        gMoviePlayer->dispX = 0;
        gMoviePlayer->dispY = 8;
        break;
    }
    cprm = &gMoviePlayer->cprm;
    memset(cprm, 0, sizeof(MwsfdCrePrm));
    cprm->unk20 = 0x11;
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
    if (vol < -960) {
        vol = -960;
    }
    ply->vtbl->setOutVol(ply, vol);
    mwPlyStartAfs(gMoviePlayer->ply, 2, desc->entry);
}

void MovieSfd_RequestStop(void)
{
    MwPly *ply;

    gMoviePlayer = &gMoviePlayerData;
    ply = gMoviePlayer->ply;
    if (ply != NULL) {
        ply->vtbl->stop(ply);
    }
}

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

void MovieSfd_Update(void)
{
    MwsfdFrmInf frame;
    u8 buf[0x20];

    if (gMoviePlayer != NULL && gMoviePlayer->ply != NULL) {
        mwPlyGetPlyInf(gMoviePlayer->ply, buf);
        gMoviePlayer->status = gMoviePlayer->ply->vtbl->getStat(gMoviePlayer->ply);
        switch (gMoviePlayer->status) {
        case 0:
            break;
        case 1:
            gMoviePlayer->waitFrames++;
            if (gMoviePlayer->waitFrames >= 300) {
                MovieSfd_Stop();
            }
            break;
        case 2:
            mwPlyGetCurFrm(gMoviePlayer->ply, &frame);
            MovieSfd_DrawFrame((u32 *)frame.bufadr, gMoviePlayer->width, gMoviePlayer->height);
            if (frame.bufadr != 0) {
                gMoviePlayer->width = frame.width;
                gMoviePlayer->height = frame.height;
                gMovieFrame = frame.unk20;
                gMoviePlayer->frame = frame;
                mwPlyRelCurFrm(gMoviePlayer->ply);
            }
            break;
        case 3:
            MovieSfd_RequestStop();
            break;
        case 4:
            MovieSfd_Stop();
            break;
        }
    }
}

s32 MovieSfd_GetStatus(void)
{
    if (gMoviePlayer == NULL) {
        return 0;
    }
    if (gMoviePlayer->ply == NULL) {
        return 0;
    }
    return gMoviePlayer->status;
}

/* Called before Movie_Play; empty in this build. */
void Movie_PrePlay(void)
{
}

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

s32 Movie_Update(void)
{
    MovieSfd_Update();
    return 0;
}
