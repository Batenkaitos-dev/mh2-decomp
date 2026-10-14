#include "common.h"
#include "em.h"
#include "dma.h"
#pragma optimization_level 4

/* prototypes */
s32 Dma_WaitIdle(void);
s32 VramSlot_FreeAll(void);
s32 Sys_FlipFrame(s32 unused0);
extern s32 gLoadScrWork;
extern s32 gLoadingAnimFrame;
extern s32 gLoadingFrame;
extern s32 gSysFrameMode; /* main */
extern s32 gSysTexBlock; /* main: gSysWork.texBlock */
extern s32 gLoadingScreenOn;
extern s32 gDispHeight; /* main: display height */
extern s32 gDispWidth; /* main: display width */
extern s32 gFlipCountdown; /* main */
/* end of prototypes */

/*
 * Writes the packet that uploads a w x h image from src to the GS (BITBLTBUF dbp, dbw,
 * dpsm; TRXPOS dx, dy; TRXREG; TRXDIR 0; then an IMAGE GIF tag and a DMA ref of the
 * data); returns the end of the packet. bits is the pixel size in 4-bit units, one test
 * per format (dpsm & 7: 0 = 32-bit, 1 = 24, 2 = 16, 3 = 8, 4 = 4), 8 for any other. The
 * format 0 test repeats the default: MWCC drops its store but keeps the block boundary,
 * so bits = 8 stays before the fmt == 1 test and that branch slot stays empty, as in
 * the game (without it, bits = 8 goes into the slot).
 */
static u64 *Gs_UploadImagePacket(u64 *p, u32 src, u32 dbp, u32 dbw, u32 dpsm, u32 dx, u32 dy, u32 w, u32 h)
{
    s32 bits;
    u32 fmt;
    u32 n;
    s32 qwc;
    u32 *q;

    fmt = dpsm & 7;
    bits = 8;
    if (fmt == 0) {
        bits = 8;
    }
    if (fmt == 1) {
        bits = 6;
    }
    if (fmt == 2) {
        bits = 4;
    }
    if (fmt == 3) {
        bits = 2;
    }
    if (fmt == 4) {
        bits = 1;
    }
    n = bits * (w * h);
    qwc = n / 32;
    if (n % 32) {
        qwc++;
    }
    q = (u32 *)p;
    *q++ = 0x10000007;
    *q++ = 0;
    p = (u64 *)q;
    q = (u32 *)p;
    *q++ = 0;
    *q++ = 0;
    p = (u64 *)q;
    *p++ = 5 | ((u64)0x10000000 << 32);
    *p++ = 0xE;
    *p++ = 0;
    *p++ = 0x3F;
    *p++ = ((u64)dpsm << 56) | (((u64)dbp << 32) | ((u64)dbw << 48));
    *p++ = 0x50;
    *p++ = ((u64)dx << 32) | ((u64)dy << 48);
    *p++ = 0x51;
    *p++ = (u64)w | ((u64)h << 32);
    *p++ = 0x52;
    *p++ = 0;
    *p++ = 0x53;
    *p++ = (s64)qwc | (0x8000 | ((u64)0x08000000 << 32));
    *p++ = 0;
    q = (u32 *)p;
    *q++ = qwc | 0x30000000;
    *q++ = src;
    p = (u64 *)q;
    q = (u32 *)p;
    *q++ = 0;
    *q++ = qwc | 0x51000000;
    p = (u64 *)q;
    return p;
}

/* Texture sizes (0xC bytes each). */
typedef struct TexSize {
    s32 w;
    s32 h;
    s32 unk8;
} TexSize;
extern TexSize gLoadingImageSizes[];

/*
 * Writes the packet that selects texture idx of *gLoadScrWork (TEX0: its base +0x38[idx],
 * CLUT +0x48[idx], PSMT8, log2 of the size of gLoadingImageSizes[idx] rounded up; with CLAMP,
 * ALPHA, TEST and TEX1); returns the end of the packet.
 */
static u64 *Tex_SelectPacket(u64 *p, s32 idx)
{
    u32 *q;
    s32 n;
    u32 tbp;
    u32 cbp;
    s32 lw;
    s32 w;
    s32 lh;
    s32 r;
    s32 h;

    tbp = ((u32 *)((u8 *)gLoadScrWork + 0x38))[idx];
    w = gLoadingImageSizes[idx].w;
    h = gLoadingImageSizes[idx].h;
    cbp = ((u32 *)((u8 *)gLoadScrWork + 0x48))[idx];
    lw = 0;
    for (n = w; n > 1; lw++) {
        r = n % 2;
        n /= 2;
    }
    if (r) {
        lw++;
    }
    lh = 0;
    for (; h > 1; lh++) {
        r = h % 2;
        h /= 2;
    }
    if (r) {
        lh++;
    }
    q = (u32 *)p;
    *q++ = 0x70000006;
    *q++ = 0;
    *q++ = 0;
    *q++ = 0;
    p = (u64 *)q;
    *p++ = 0x8005 | ((u64)0x10000000 << 32);
    *p++ = 0xE;
    *p++ = 0;
    *p++ = 0x8;
    *p++ = 0x44;
    *p++ = 0x42;
    *p++ = 0x7000F;
    *p++ = 0x47;
    *p++ = (u64)tbp | ((s64)(w / 64) << 14) | ((u64)0x13 << 20) | ((s64)lw << 26) |
           ((s64)lh << 30) | ((u64)0xC << 32) | ((u64)cbp << 37) | ((u64)0x20000000 << 32);
    *p++ = 0x6;
    *p++ = 0x60;
    *p++ = 0x14;
    return p;
}

/* A screen sprite for Loading_WriteSprite: position and size in 640-wide screen units, texel
 * origin and size. */
typedef struct ScrSprite {
    s32 unk00;
    f32 x;              /* 0x04 */
    f32 y;              /* 0x08 */
    s16 w;              /* 0x0C */
    s16 h;              /* 0x0E */
    f32 u;              /* 0x10 */
    f32 v;              /* 0x14 */
    s16 tw;             /* 0x18 */
    s16 th;             /* 0x1A */
} ScrSprite;

/*
 * Writes the GIF packet of screen sprite s at depth z (PRIM sprite, grey RGBAQ, UV and
 * XYZ2 for both corners): x and w scaled from 640 to the display width (0x219198), y and h
 * halved while not interlaced (0x218C74), centred in the 4096-wide space. The corner
 * sx, sy is a local of its own (the game computes it first and keeps it for the far
 * corner), and each XYZ word is GS_XYZ's x | y << 16 before z << 32 is or-ed in.
 */
static u64 *Loading_WriteSprite(u64 *p, ScrSprite *s, u32 z)
{
    f32 sc;
    s32 ox;
    s32 oy;
    s32 h;
    s32 x;
    s32 y;
    s32 w;
    s32 u;
    s32 v;
    s32 tw;
    s32 th;
    u32 *q;
    s32 sx;
    s32 sy;

    ox = (0x1000 - gDispWidth) / 2 * 16;
    oy = (0x1000 - gDispHeight) / 2 * 16;
    sc = (f32)gDispWidth / 640.0f;
    x = 16.0f * (s->x * sc);
    w = (u32)(16.0f * (s->w * sc));
    y = 16.0f * s->y;
    h = 16.0f * s->h;
    if (gSysFrameMode == 0) {
        y >>= 1;
        h >>= 1;
    }
    u = 16.0f * s->u;
    v = 16.0f * s->v;
    tw = u + (s->tw - 1) * 16;
    th = v + (s->th - 1) * 16;
    sx = ox + x;
    sy = oy + y;
    q = (u32 *)p;
    *q++ = 0x70000004;
    *q++ = 0;
    *q++ = 0;
    *q++ = 0;
    p = (u64 *)q;
    *p++ = 0x8001 | ((u64)0x64000000 << 32);
    *p++ = 0x535310;
    *p++ = 0x156;
    *p++ = 0x80404040;
    *p++ = (s64)(u + 8) | ((s64)(v + 8) << 16);
    *p++ = ((s64)sx | ((s64)sy << 16)) | ((u64)z << 32);
    *p++ = (s64)(tw + 8) | ((s64)(th + 8) << 16);
    *p++ = ((s64)(sx + w) | ((s64)(sy + h) << 16)) | ((u64)z << 32);
    return p;
}

/* The work at gLoadScrWork (0x4DC bytes, at gMemAreas - 0x1800). */
typedef struct ScrWork {
    u8 unk00[0xC];
    s32 state;                  /* 0x0C: 1 once set up */
    u8 *base;                   /* 0x10 */
    s32 size;                   /* 0x14: the bytes used from base */
    u8 *img[4];                 /* 0x18: the 8-bit images */
    u8 *clut[4];                /* 0x28: their palettes */
    u32 tbp[4];                 /* 0x38 */
    u32 cbp[4];                 /* 0x48 */
    u64 *chain;                 /* 0x58: the upload packet */
    u64 *sel[2][4];             /* 0x5C: the texture select packets, per buffer */
    u64 *spr[2][16];            /* 0x7C: the sprite packets, per buffer */
    u8 unkFC[0x134 - 0xFC];
    s32 buf;                    /* 0x134: the packet buffer drawn next (sel, spr) */
    u32 vram;                   /* 0x138: the first free GS block */
    u8 unk13C[0x140 - 0x13C];
    ScrSprite rect[32];         /* 0x140: the sprites, unk00 -1 ends */
    s16 unk4C0;                 /* 0x4C0 */
    f32 x;                      /* 0x4C4 */
    f32 y;                      /* 0x4C8 */
    f32 unk4CC;                 /* 0x4CC */
    u8 unk4D0[0x4DC - 0x4D0];
} ScrWork;

extern DmaChan *gDmaChan[10];
void FlushCache(s32 op);
s32 sceDmaSend(DmaChan *chan, u32 tag);
s32 sceGsSyncPath(s32 mode, u16 timeout);
u16 Rand_Next(s32 stream);
s32 LoadCompressedFile(u8 *dst, s32 id);
void *memset(void *p, s32 c, u32 n);
extern u8 *gLoadBuf;
extern s32 gMemAreas;
extern s32 gLoadingState;
extern s32 gLoadingArg;
extern s16 gTexFileIds8;
extern ScrSprite gLoadingSprites[];
extern s16 gLoadingStartPos[][2];
extern f32 gLobbyDayFrac;

/*
 * Sets up the screen work at gLoadScrWork (unless already set up): loads file gTexFileIds8 into
 * gLoadBuf, uploads its 8-bit images (sizes gLoadingImageSizes) and their palettes (alpha halved,
 * 0xFF -> 0x80) to the GS from +0x138 on, builds per buffer the texture select packets and
 * the sprite packets of gLoadingSprites (copied to +0x140), sends the upload chain on DMA channel
 * 2, and picks a random start position from gLoadingStartPos by gLobbyDayFrac (below 0.4, 0.4..0.6,
 * above). One counter, i, serves the image, palette and select loops and then holds the random
 * index: its later webs are numbered after the loop walkers MWCC creates, as the game numbers
 * them. The file's entries are read as u32 and cast to s32 for the offsets: the conversion is
 * what makes MWCC load the size before the reloaded img[i] (any sign cast does it, operand
 * orders don't).
 */
void Loading_Open(u32 a, s32 b)
{
    u8 *buf;
    u32 dbp;
    s32 k;
    u32 *ent;
    u32 *ts;
    u32 tw;
    u32 th;
    u64 *p;
    u32 *c;
    ScrWork *w;
    u32 v;
    s32 n;
    s32 i;
    u8 *base;
    ScrSprite *t;
    s32 m;
    u8 al;

    gLoadingScreenOn = 1;
    base = (u8 *)gMemAreas - 0x1800;
    if (gLoadScrWork != 0 && ((ScrWork *)gLoadScrWork)->state == 1) {
        return;
    }
    gLoadingArg = b;
    gLoadScrWork = (s32)base;
    gLoadingState = -1;
    w = (ScrWork *)gLoadScrWork;
    memset(w, 0, sizeof(ScrWork));
    w->base = base;
    w->vram = gSysTexBlock + 0x300;
    buf = gLoadBuf;
    LoadCompressedFile(buf, gTexFileIds8);
    n = ((s32 *)buf)[1];
    dbp = w->vram;
    p = (u64 *)(((u32)(base + sizeof(ScrWork)) + 0x3F) & ~0x3F);
    ent = (u32 *)(buf + 8);
    ts = (u32 *)gLoadingImageSizes;
    w->chain = p;
    for (i = 0; i < n; i++) {
        w->img[i] = buf + (s32)ent[0];
        w->clut[i] = w->img[i] + (s32)ent[1] - 0x400;
        ent += 2;
        w->img[i] += 0x40;
        w->tbp[i] = dbp;
        tw = *ts++;
        th = *ts++;
        dbp += *ts++;
        p = Gs_UploadImagePacket(p, (u32)w->img[i], w->tbp[i], tw >> 6, 0x13, 0, 0, tw, th);
    }
    for (i = 0; i < n; i++) {
        w->cbp[i] = dbp;
        c = (u32 *)w->clut[i];
        for (k = 0; k < 256; k++) {
            v = *c;
            al = (v & 0xFF000000) >> 24;
            if (al == 0xFF) {
                al = 0x80;
            } else if (al != 0) {
                al >>= 1;
                if (al == 0) {
                    al = 1;
                }
            }
            *c = (v & 0xFFFFFF) | (al << 24);
            c++;
        }
        p = Gs_UploadImagePacket(p, (u32)w->clut[i], w->cbp[i], 1, 0, 0, 0, 16, 16);
        dbp += 4;
    }
    {
        u32 *q = (u32 *)p;
        *q++ = 0x70000000;
        *q++ = 0;
        p = (u64 *)q;
    }
    {
        u32 *q = (u32 *)p;
        *q++ = 0;
        *q++ = 0;
        p = (u64 *)q;
    }
    for (k = 0; k < 2; k++) {
        for (i = 0; i < 3; i++) {
            w->sel[k][i] = (u64 *)(((u32)p + 0xF) & ~0xF);
            p = Tex_SelectPacket(w->sel[k][i], i);
        }
        m = 0;
        t = gLoadingSprites;
        if (gLoadingSprites[0].unk00 != -1) {
            do {
                w->rect[m] = *t;
                w->spr[k][m] = (u64 *)(((u32)p + 0xF) & ~0xF);
                p = Loading_WriteSprite(w->spr[k][m], t, -0x10);
                t++;
                m++;
            } while (t->unk00 != -1);
        }
        w->rect[m].unk00 = -1;
    }
    w->size = (u8 *)p - w->base;
    Dma_WaitIdle();
    Sys_FlipFrame(0);
    Sys_FlipFrame(0);
    VramSlot_FreeAll();
    gDmaChan[2]->chcr.tte = 0;
    FlushCache(0);
    Dma_WaitIdle();
    sceDmaSend(gDmaChan[2], (u32)w->chain);
    sceGsSyncPath(0, 0);
    if (gLobbyDayFrac >= 0.4f && gLobbyDayFrac <= 0.6f) {
        i = Rand_Next(1) % 6;
    } else {
        i = Rand_Next(1) % 3;
        if (gLobbyDayFrac > 0.6f) {
            i += 3;
        }
    }
    w->x = gLoadingStartPos[i][0];
    w->y = gLoadingStartPos[i][1];
    w->unk4C0 = i % 2;
    w->state = 1;
    w->unk4CC = 560.0f * gLobbyDayFrac;
}

void Loading_Close(void)
{
    if (gLoadScrWork != 0) {
        gLoadingScreenOn = 0;
        gFlipCountdown = 2;
        Dma_WaitIdle();
        Sys_FlipFrame(0);
        Sys_FlipFrame(0);
        VramSlot_FreeAll();
        gLoadScrWork = 0;
    }
}

static void Loading_ClearDrift(void)
{
    u8 *p = (u8 *)gLoadScrWork;

    *(s32 *)(p + 0x4D4) = 0;
    *(s32 *)(p + 0x4D8) = 0;
}

/*
 * Moves the pointer of *gLoadScrWork by (dx, dy) plus +0x4D4, +0x4D8 (which Loading_ClearDrift
 * has just cleared, so the vertical test below always takes the first branch): x (+0x4C4) is kept in 64..576 (right edge + 48), y (+0x4C8)
 * in 64..384; without a vertical step a y out of range is refused (returns 1).
 */
s32 Loading_MoveSprite(f32 dx, f32 dy)
{
    u8 *p;
    f32 x;
    f32 y;

    p = (u8 *)gLoadScrWork;
    Loading_ClearDrift();
    x = *(f32 *)(p + 0x4C4);
    x += dx;
    x += *(f32 *)(p + 0x4D4);
    if (x < 64.0f) {
        x = 64.0f;
    }
    if (x + 48.0f > 576.0f) {
        x = 576.0f;
    }
    *(f32 *)(p + 0x4C4) = x;
    y = *(f32 *)(p + 0x4C8);
    y += dy;
    y += *(f32 *)(p + 0x4D8);
    if (*(f32 *)(p + 0x4D8) == 0.0f) {
        if (y < 64.0f) {
            return 1;
        }
        if (y + 48.0f > 384.0f) {
            return 1;
        }
    } else {
        if (y < 64.0f) {
            y = 64.0f;
        }
        if (y + 48.0f > 384.0f) {
            y = 384.0f;
        }
    }
    *(f32 *)(p + 0x4C8) = y;
    return 0;
}

void Loading_Animate(void)
{
    gLoadingFrame += 1;
    if (gLoadingFrame & 1) {
        gLoadingAnimFrame += 1;
    }
    if (gLoadingAnimFrame > 11) {
        gLoadingAnimFrame -= 0xC;
    }
}

extern s32 gLoadingScrollX;
extern char *gLoadingMessage;
extern f32 gLoadingBobOffsets[];
s32 Gs_ClearColor(u8 mode, u32 color);
s32 Gs_SetState(u8 reg, u32 v);
s32 Prim_SendPacket(u32 *p);
f32 Math_Sin(f32 x);
f32 Math_Cos(f32 x);
void Text_Reset(void);
void Font_SetSpacing(u8 a, u8 b);
s32 Str_Length(char *s);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Text_DrawPage(s32 layer);

/* Draws the loading screen: the scrolling background (rect[0..2], gLoadingScrollX pixels on, a
 * 1248-pixel period), the pointer (rect[3], mirrored on the left half), five strips drifting
 * left by +0x4CC, the ten bobbing letters (rect[10..19], gLoadingBobOffsets by gLoadingAnimFrame), then
 * flips the packet buffer, moves the pointer along a sine and prints gLoadingMessage if set.
 */
void Loading_Draw(s32 dt)
{
    ScrWork *w = (ScrWork *)gLoadScrWork;
    ScrSprite *r;
    ScrSprite *src;
    u64 **first;
    s32 buf;
    s32 i;
    s32 k;
    s32 z;
    f32 dx;
    char *s;

    if (w != NULL && w->state != 0) {
        buf = w->buf;
        Gs_SetState(0x6C, 1);
        Gs_ClearColor(0, 0);
        Gs_SetState(0x6C, 0);
        for (i = 0; i < 3; i++) {
            Tex_SelectPacket(w->sel[buf][i], i);
        }
        gLoadingScrollX += 1;
        if (gLoadingScrollX >= 1248) {
            gLoadingScrollX -= 1248;
        }
        src = gLoadingSprites;
        r = w->rect;
        Prim_SendPacket((u32 *)w->sel[buf][1]);
        for (i = 0; i < 3; i++, r++, src++) {
            r->x = src->x;
            r->x += (f32)gLoadingScrollX;
            if (!(r->x < 640.0f)) {
                r->x -= 1248.0f;
            }
            if (r->x < 640.0f && !(416.0f + r->x < 0.0f)) {
                Loading_WriteSprite(w->spr[buf][i], r, -0x10);
                Prim_SendPacket((u32 *)w->spr[buf][i]);
            }
        }
        r = &w->rect[3];
        src = &gLoadingSprites[3];
        r->x = w->x;
        r->y = w->y;
        if (r->x < 288.0f) {
            r->u = src->u + (f32)src->tw - 1.0f;
            r->tw = -src->tw;
        } else {
            r->u = src->u;
            r->tw = src->tw;
        }
        first = (u64 **)((u8 *)w->sel + buf * 16);
        Prim_SendPacket((u32 *)*first);
        Loading_WriteSprite(w->spr[buf][3], r, -0x10);
        Prim_SendPacket((u32 *)w->spr[buf][3]);
        Gs_SetState(0x6C, 1);
        *(s32 *)w->unk4D0 += dt;
        Prim_SendPacket((u32 *)w->sel[buf][2]);
        if (w->unk4CC < 640.0f) {
            w->unk4CC += 0.05f;
        }
        r = &w->rect[4];
        src = &gLoadingSprites[4];
        z = -0x10;
        for (i = 0; i < 5; i++) {
            *r = *src;
            r->x -= w->unk4CC;
            if (!(r->x + (f32)r->w < 0.0f) && r->x <= 640.0f) {
                Loading_WriteSprite(w->spr[buf][4 + i], r, z);
                Prim_SendPacket((u32 *)w->spr[buf][4 + i]);
                z--;
            }
            r++;
            src++;
        }
        Gs_ClearColor(0, 0);
        Gs_SetState(0x6C, 0);
        Prim_SendPacket((u32 *)*first);
        Prim_SendPacket((u32 *)w->spr[buf][9]);
        Loading_Animate();
        k = gLoadingAnimFrame;
        r = &w->rect[10];
        src = &gLoadingSprites[10];
        for (i = 0; i < 10; i++, r++, src++) {
            r->x = src->x;
            r->y = (s32)(src->y + 4.0f * gLoadingBobOffsets[k]);
            Loading_WriteSprite(w->spr[buf][10 + i], r, -0x10);
            Prim_SendPacket((u32 *)w->spr[buf][10 + i]);
            if (i == 2) {
                k += 2;
            } else {
                k++;
            }
            if (k > 11) {
                k -= 12;
            }
        }
        w->buf ^= 1;
        dx = 0.3f * Math_Sin(6.2831855f * (180.0f * w->unk4C0) / 360.0f);
        if (Loading_MoveSprite(dx, 0.3f * Math_Cos(6.2831855f * (180.0f * w->unk4C0) / 360.0f)) != 0) {
            w->unk4C0 ^= 1;
        }
        if (gLoadingMessage != NULL) {
            Text_Reset();
            Font_SetSpacing(0x18, 0x18);
            s = gLoadingMessage;
            Text_DrawShadow((0x280 - Str_Length(s) * 12) / 2, 0xC8, 1, 0, s);
            Text_DrawPage(0);
        }
        Gs_SetState(0x6C, 0);
    }
}
