/*
 * The loading screen: the animated picture drawn while the game loads.
 *
 * Loading_Open loads the screen's file of four 8-bit images, uploads them
 * and their palettes to GS memory, and builds every packet the screen
 * draws with, twice (one set per packet buffer), in a work area just
 * below the memory areas (a LoadingScreen, then its packets). Each frame
 * Loading_Draw sends them: a background scrolling right, a sprite that
 * floats up and down between the screen's edges, five strips drifting
 * left, ten letters bobbing on a wave, and the caption the loaders set
 * (gLoadingMessage, through Loading_SetMessage). Loading_Close gives the
 * GS memory back.
 *
 * The packets are DMA chains of GIF tags: Gs_UploadImagePacket uploads
 * an image, Tex_SelectPacket selects a texture, Loading_WriteSprite
 * draws a sprite. Screen positions are in a 640-wide space, scaled to
 * the display's width.
 */
#include "common.h"
#include "em.h"
#include "dma.h"
#include "hw_regs.h"
#include "texture.h"
#include "render_state.h"
#include "syswork.h"
#include "card.h"
#include "prim.h"
#include "text.h"
#include "loading_screen.h"
#include "display.h"
#include "engine_math.h"
#include "file_load.h"
#include "font.h"
#include "gs_setup.h"
#include "rand.h"
#include "libc.h"
#include "libgraph.h"
#include "libkernl.h"
#pragma optimization_level 4

#define LOADING_IMAGES 4        /* the images of the screen's file */
#define LOADING_SPRITES_MAX 32  /* the sprites the work holds */
#define LOADING_WAVE_STEPS 12   /* the letters' wave (gLoadingBobOffsets) */

/* The loading screen's work (gLoadingScreen, 1244 bytes at gMemAreas -
 * 0x1800); its packets follow it. */
typedef struct LoadingScreen {
    u8 unk00[12];                          /* no use but the clear */
    s32 state;                              /* 0x0C: 1 once set up */
    u8 *base;                               /* 0x10 */
    s32 size;                               /* 0x14: the bytes used from base */
    u8 *img[LOADING_IMAGES];                /* 0x18: the 8-bit images */
    u8 *clut[LOADING_IMAGES];               /* 0x28: their palettes */
    u32 tbp[LOADING_IMAGES];                /* 0x38: the images' GS addresses */
    u32 cbp[LOADING_IMAGES];                /* 0x48: the palettes' GS addresses */
    u64 *chain;                             /* 0x58: the upload chain */
    u64 *sel[2][LOADING_IMAGES];            /* 0x5C: the texture select packets, per buffer */
    u64 *spr[2][16];                        /* 0x7C: the sprite packets, per buffer */
    u8 unkFC[308 - 252];                 /* no use but the clear */
    s32 buf;                                /* 0x134: the packet buffer drawn next (sel, spr) */
    u32 vram;                               /* 0x138: the first free GS block */
    u8 unk13C[320 - 316];               /* no use but the clear */
    LoadingSprite rect[LOADING_SPRITES_MAX]; /* 0x140: the sprites as drawn */
    s16 floatUp;                            /* 0x4C0: the floating sprite moves up (1) or down (0) */
    f32 x;                                  /* 0x4C4: the floating sprite's position */
    f32 y;                                  /* 0x4C8 */
    f32 stripScroll;                        /* 0x4CC: how far the strips have drifted left */
    s32 elapsed;                            /* 0x4D0: the frames passed, summed */
    f32 driftX;                             /* 0x4D4: added to each move of the floating sprite */
    f32 driftY;                             /* 0x4D8: the same, vertically; 0 makes a move off the */
                                            /* screen's height fail rather than clamp */
} LoadingScreen;

/* prototypes */
extern LoadingScreen *gLoadingScreen;
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
 * Writes the DMA packet that uploads a width x height image at src to GS
 * memory (dbp, dbw and dpsm of BITBLTBUF, dx and dy of TRXPOS) and returns
 * the packet's end. The data goes by reference after the packet's own GIF
 * tags. nibbles is the size of a pixel in 4-bit units, by the format's low
 * bits: 8 for 32-bit, 6 for 24-bit, 4 for 16-bit, 2 for 8-bit, 1 for 4-bit
 * (8 for any other). The 32-bit test repeats the default: the compiler drops
 * its store but keeps the branch, which leaves the delay slot after the
 * 24-bit test empty, as in the game.
 */
static u64 *Gs_UploadImagePacket(u64 *pkt, u32 src, u32 dbp, u32 dbw, u32 dpsm, u32 dx, u32 dy, u32 width, u32 height)
{
    s32 nibbles;
    u32 fmt;
    u32 totalNibbles;
    s32 qwc;
    u32 *word;

    fmt = dpsm & 7;
    nibbles = 8;
    if (fmt == 0) {
        nibbles = 8;
    }
    if (fmt == 1) {
        nibbles = 6;
    }
    if (fmt == 2) {
        nibbles = 4;
    }
    if (fmt == 3) {
        nibbles = 2;
    }
    if (fmt == 4) {
        nibbles = 1;
    }
    totalNibbles = nibbles * (width * height);
    qwc = totalNibbles / 32;
    if (totalNibbles % 32) {
        qwc++;
    }
    word = (u32 *)pkt;
    *word++ = DMA_TAG_CNT | 7;
    *word++ = 0;
    pkt = (u64 *)word;
    word = (u32 *)pkt;
    *word++ = 0;
    *word++ = 0;
    pkt = (u64 *)word;
    *pkt++ = 5 | ((u64)0x10000000 << 32); /* GIF tag: five A+D writes */
    *pkt++ = GIF_REG_AD;
    *pkt++ = 0;
    *pkt++ = GS_REG_TEXFLUSH;
    *pkt++ = ((u64)dpsm << 56) | (((u64)dbp << 32) | ((u64)dbw << 48));
    *pkt++ = GS_REG_BITBLTBUF;
    *pkt++ = ((u64)dx << 32) | ((u64)dy << 48);
    *pkt++ = GS_REG_TRXPOS;
    *pkt++ = (u64)width | ((u64)height << 32);
    *pkt++ = GS_REG_TRXREG;
    *pkt++ = 0;
    *pkt++ = GS_REG_TRXDIR;
    /* GIF tag: qwc quadwords of image data, the end of the packet */
    *pkt++ = (s64)qwc | (0x8000 | ((u64)0x08000000 << 32));
    *pkt++ = 0;
    word = (u32 *)pkt;
    *word++ = qwc | DMA_TAG_REF; /* the image data, by reference */
    *word++ = src;
    pkt = (u64 *)word;
    word = (u32 *)pkt;
    *word++ = 0;
    *word++ = qwc | VIF_DIRECTHL; /* a VIF code, not read on the GIF channel */
    pkt = (u64 *)word;
    return pkt;
}

extern LoadingImage gLoadingImageSizes[];

/*
 * Writes the DMA packet that selects image image of the loading screen as
 * the texture (TEX0 with its GS address, palette and size, rounded up to
 * powers of two; CLAMP, ALPHA, TEST and TEX1 with it) and returns the
 * packet's end.
 */
static u64 *Tex_SelectPacket(u64 *pkt, s32 image)
{
    u32 *word;
    s32 size;
    u32 tbp;
    u32 cbp;
    s32 logW;
    s32 width;
    s32 logH;
    s32 rest;
    s32 height;

    tbp = gLoadingScreen->tbp[image];
    width = gLoadingImageSizes[image].w;
    height = gLoadingImageSizes[image].h;
    cbp = gLoadingScreen->cbp[image];
    logW = 0;
    for (size = width; size > 1; logW++) {
        rest = size % 2;
        size /= 2;
    }
    if (rest) {
        logW++;
    }
    logH = 0;
    for (; height > 1; logH++) {
        rest = height % 2;
        height /= 2;
    }
    if (rest) {
        logH++;
    }
    word = (u32 *)pkt;
    *word++ = DMA_TAG_END | 6;
    *word++ = 0;
    *word++ = 0;
    *word++ = 0;
    pkt = (u64 *)word;
    *pkt++ = GIF_TAG_EOP | 5 | GIF_TAG_NREG(1); /* GIF tag: five A+D writes, the end */
    *pkt++ = GIF_REG_AD;
    *pkt++ = 0;
    *pkt++ = GS_REG_CLAMP_1; /* repeat */
    *pkt++ = 68; /* (source - frame) * source alpha + frame */
    *pkt++ = GS_REG_ALPHA_1;
    /* the alpha test passes alpha other than 0, the depth test the nearer pixel */
    *pkt++ = GS_TEST_ATE | (GS_ATST_NOTEQUAL << 1) | GS_TEST_ZTE | (GS_ZTST_GREATER << 17);
    *pkt++ = GS_REG_TEST_1;
    *pkt++ = (u64)tbp | ((s64)(width / 64) << 14) | ((u64)GS_PSMT8 << 20) | ((s64)logW << 26) |
           ((s64)logH << 30) | ((u64)12 << 32) | ((u64)cbp << 37) | ((u64)0x20000000 << 32);
    *pkt++ = GS_REG_TEX0_1;
    *pkt++ = TEX1_MMAG_LINEAR | TEX1_MMIN(TEX1_MIN_LINEAR);
    *pkt++ = GS_REG_TEX1_1;
    return pkt;
}


/*
 * Writes the DMA packet that draws sprite at depth depth and returns the
 * packet's end. x and width are scaled from 640 to the display's width; y
 * and height are halved when gSysFrameMode is FRAME_MODE_FIELD; the
 * picture is centred in the GS's 4096-wide drawing space, in its 1/16 pixel units. left and top
 * stay locals of their own: the game computes the corner first and keeps it
 * for the far corner.
 */
static u64 *Loading_WriteSprite(u64 *pkt, LoadingSprite *sprite, u32 depth)
{
    f32 scale;
    s32 originX;
    s32 originY;
    s32 height;
    s32 x;
    s32 y;
    s32 width;
    s32 texU;
    s32 texV;
    s32 texEndU;
    s32 texEndV;
    u32 *word;
    s32 left;
    s32 top;

    originX = (4096 - gDispWidth) / 2 * 16;
    originY = (4096 - gDispHeight) / 2 * 16;
    scale = (f32)gDispWidth / 640.0f;
    x = 16.0f * (sprite->x * scale);
    width = (u32)(16.0f * (sprite->w * scale));
    y = 16.0f * sprite->y;
    height = 16.0f * sprite->h;
    if (gSysFrameMode == FRAME_MODE_FIELD) {
        y >>= 1;
        height >>= 1;
    }
    texU = 16.0f * sprite->u;
    texV = 16.0f * sprite->v;
    texEndU = texU + (sprite->tw - 1) * 16;
    texEndV = texV + (sprite->th - 1) * 16;
    left = originX + x;
    top = originY + y;
    word = (u32 *)pkt;
    *word++ = DMA_TAG_END | 4;
    *word++ = 0;
    *word++ = 0;
    *word++ = 0;
    pkt = (u64 *)word;
    /* GIF tag: one loop of a list of six registers, the end */
    *pkt++ = 0x8001 | ((u64)0x64000000 << 32);
    *pkt++ = GS_REG_PRIM | (GS_REG_RGBAQ << 4) | (GS_REG_UV << 8) | (GS_REG_XYZ2 << 12) |
             (GS_REG_UV << 16) | (GS_REG_XYZ2 << 20);
    *pkt++ = 342; /* a sprite, textured, blended, with texel coordinates */
    *pkt++ = 0x80404040; /* grey, alpha 128 */
    *pkt++ = (s64)(texU + 8) | ((s64)(texV + 8) << 16);
    *pkt++ = ((s64)left | ((s64)top << 16)) | ((u64)depth << 32);
    *pkt++ = (s64)(texEndU + 8) | ((s64)(texEndV + 8) << 16);
    *pkt++ = ((s64)(left + width) | ((s64)(top + height) << 16)) | ((u64)depth << 32);
    return pkt;
}


extern DmaChan *gDmaChan[10];
s32 sceDmaSend(DmaChan *chan, u32 tag);
extern u8 *gLoadBuf;
extern s32 gMemAreas;
extern s32 gLoadingState;
extern s32 gLoadingArg;
extern s16 gTexFileIds8;
extern LoadingSprite gLoadingSprites[];
extern s16 gLoadingStartPos[][2];

/*
 * Opens the loading screen, unless it is open already. Sets up the work
 * under the memory areas, loads the screen's file (gTexFileIds8) into
 * gLoadBuf and uploads its images and palettes to GS memory after the
 * system's textures, with each palette's alpha halved for the GS (255
 * becomes 128). Builds, for each packet buffer, the texture select packets
 * and the packets of the sprites of gLoadingSprites, which it copies to the
 * work. Then sends the upload chain on the GIF's DMA channel, and picks the
 * floating sprite's start among gLoadingStartPos by the time of day
 * (gLobbyDayFrac): the middle of the day draws from all six, else the first
 * three or, late, the last three. unusedBuf is not read; arg is kept in
 * gLoadingArg.
 *
 * One counter, i, serves the image, palette and select loops and then holds
 * the random index: with the one variable, the compiler gives out registers
 * as the game's code has them. The file's entries are read as u32 and cast
 * to s32 for the offsets: with the cast, the compiler loads the image's size
 * before it loads img[i] again, as the game does.
 */
void Loading_Open(u32 unusedBuf, s32 arg)
{
    u8 *file;
    u32 vramBlock;
    s32 k;
    u32 *entry;
    u32 *imageSize;
    u32 imageW;
    u32 imageH;
    u64 *pkt;
    u32 *color;
    LoadingScreen *screen;
    u32 rgba;
    s32 imageCount;
    s32 i;
    u8 *base;
    LoadingSprite *sprite;
    s32 spriteCount;
    u8 alpha;

    gLoadingScreenOn = 1;
    base = (u8 *)gMemAreas - 6144;
    if (gLoadingScreen != NULL && gLoadingScreen->state == 1) {
        return;
    }
    gLoadingArg = arg;
    gLoadingScreen = (LoadingScreen *)base;
    gLoadingState = -1;
    screen = gLoadingScreen;
    memset(screen, 0, sizeof(LoadingScreen));
    screen->base = base;
    screen->vram = gSysTexBlock + 768;
    file = gLoadBuf;
    LoadCompressedFile(file, gTexFileIds8);
    imageCount = ((s32 *)file)[1];
    vramBlock = screen->vram;
    pkt = (u64 *)(((u32)(base + sizeof(LoadingScreen)) + 63) & ~0x3F);
    entry = (u32 *)(file + 8);
    imageSize = (u32 *)gLoadingImageSizes;
    screen->chain = pkt;
    for (i = 0; i < imageCount; i++) {
        screen->img[i] = file + (s32)entry[0];
        screen->clut[i] = screen->img[i] + (s32)entry[1] - 1024;
        entry += 2;
        screen->img[i] += 64;
        screen->tbp[i] = vramBlock;
        imageW = *imageSize++;
        imageH = *imageSize++;
        vramBlock += *imageSize++;
        pkt = Gs_UploadImagePacket(pkt, (u32)screen->img[i], screen->tbp[i], imageW >> 6, GS_PSMT8, 0, 0, imageW, imageH);
    }
    for (i = 0; i < imageCount; i++) {
        screen->cbp[i] = vramBlock;
        color = (u32 *)screen->clut[i];
        for (k = 0; k < 256; k++) {
            rgba = *color;
            alpha = (rgba & 0xFF000000) >> 24;
            if (alpha == 255) {
                alpha = 128;
            } else if (alpha != 0) {
                alpha >>= 1;
                if (alpha == 0) {
                    alpha = 1;
                }
            }
            *color = (rgba & 0xFFFFFF) | (alpha << 24);
            color++;
        }
        pkt = Gs_UploadImagePacket(pkt, (u32)screen->clut[i], screen->cbp[i], 1, GS_PSMCT32, 0, 0, 16, 16);
        vramBlock += 4;
    }
    {
        u32 *word = (u32 *)pkt;
        *word++ = DMA_TAG_END;
        *word++ = 0;
        pkt = (u64 *)word;
    }
    {
        u32 *word = (u32 *)pkt;
        *word++ = 0;
        *word++ = 0;
        pkt = (u64 *)word;
    }
    for (k = 0; k < 2; k++) {
        for (i = 0; i < 3; i++) {
            screen->sel[k][i] = (u64 *)(((u32)pkt + 15) & ~0xF);
            pkt = Tex_SelectPacket(screen->sel[k][i], i);
        }
        spriteCount = 0;
        sprite = gLoadingSprites;
        if (gLoadingSprites[0].tex != -1) {
            do {
                screen->rect[spriteCount] = *sprite;
                screen->spr[k][spriteCount] = (u64 *)(((u32)pkt + 15) & ~0xF);
                pkt = Loading_WriteSprite(screen->spr[k][spriteCount], sprite, -16);
                sprite++;
                spriteCount++;
            } while (sprite->tex != -1);
        }
        screen->rect[spriteCount].tex = -1;
    }
    screen->size = (u8 *)pkt - screen->base;
    Dma_WaitIdle();
    Sys_FlipFrame(0);
    Sys_FlipFrame(0);
    VramSlot_FreeAll();
    gDmaChan[2]->chcr.tte = 0;
    FlushCache(0);
    Dma_WaitIdle();
    sceDmaSend(gDmaChan[2], (u32)screen->chain);
    sceGsSyncPath(0, 0);
    if (gLobbyDayFrac >= 0.4f && gLobbyDayFrac <= 0.6f) {
        i = Rand_Next(1) % 6;
    } else {
        i = Rand_Next(1) % 3;
        if (gLobbyDayFrac > 0.6f) {
            i += 3;
        }
    }
    screen->x = gLoadingStartPos[i][0];
    screen->y = gLoadingStartPos[i][1];
    screen->floatUp = i % 2;
    screen->state = 1;
    screen->stripScroll = 560.0f * gLobbyDayFrac;
}

/*
 * Closes the loading screen if it is open: waits for the DMA, flips twice
 * so that no frame still draws from it, and frees the GS memory.
 */
void Loading_Close(void)
{
    if (gLoadingScreen != NULL) {
        gLoadingScreenOn = 0;
        gFlipCountdown = 2;
        Dma_WaitIdle();
        Sys_FlipFrame(0);
        Sys_FlipFrame(0);
        VramSlot_FreeAll();
        gLoadingScreen = NULL;
    }
}

/* Clears the floating sprite's drift. */
static void Loading_ClearDrift(void)
{
    LoadingScreen *screen = gLoadingScreen;

    screen->driftX = 0;
    screen->driftY = 0;
}

/*
 * Moves the floating sprite by (dx, dy) plus its drift, which it clears
 * first, so the vertical test always takes its first branch. x is kept in
 * 64..576 for its 48-pixel width. Returns 1, and doesn't move it vertically,
 * when y would leave 64..384; else 0.
 */
s32 Loading_MoveSprite(f32 dx, f32 dy)
{
    LoadingScreen *screen;
    f32 x;
    f32 y;

    screen = gLoadingScreen;
    Loading_ClearDrift();
    x = screen->x;
    x += dx;
    x += screen->driftX;
    if (x < 64.0f) {
        x = 64.0f;
    }
    if (x + 48.0f > 576.0f) {
        x = 576.0f;
    }
    screen->x = x;
    y = screen->y;
    y += dy;
    y += screen->driftY;
    if (screen->driftY == 0.0f) {
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
    screen->y = y;
    return 0;
}

/* Counts the screen's frames and steps the letters' wave every second frame. */
void Loading_Animate(void)
{
    gLoadingFrame += 1;
    if (gLoadingFrame & 1) {
        gLoadingAnimFrame += 1;
    }
    if (gLoadingAnimFrame > LOADING_WAVE_STEPS - 1) {
        gLoadingAnimFrame -= LOADING_WAVE_STEPS;
    }
}

extern s32 gLoadingScrollX;
extern char *gLoadingMessage;
extern f32 gLoadingBobOffsets[];
s32 Str_Length(char *s);

/*
 * Draws the loading screen, once a frame while it is open (frames: the frames
 * passed). The background scrolls right with gLoadingScrollX and wraps
 * every 1248 pixels; the floating sprite (sprite 3) faces right on the
 * screen's right half and is mirrored on its left half; the five strips
 * drift left with stripScroll; the ten letters bob, each one step further
 * on the wave (gLoadingBobOffsets), with a gap of a step after the third.
 * Then switches the packet buffer, moves the floating sprite up or down,
 * turning it round at the edges, and draws gLoadingMessage, if any, centred.
 */
void Loading_Draw(s32 frames)
{
    LoadingScreen *screen = gLoadingScreen;
    LoadingSprite *sprite;
    LoadingSprite *def;
    u64 **firstSel;
    s32 buf;
    s32 i;
    s32 wave;
    s32 depth;
    f32 dx;
    char *text;

    if (screen != NULL && screen->state != 0) {
        buf = screen->buf;
        Gs_SetState(RS_ZWRITE, 1);
        Gs_ClearColor(0, 0);
        Gs_SetState(RS_ZWRITE, 0);
        for (i = 0; i < 3; i++) {
            Tex_SelectPacket(screen->sel[buf][i], i);
        }
        gLoadingScrollX += 1;
        if (gLoadingScrollX >= 1248) {
            gLoadingScrollX -= 1248;
        }
        def = gLoadingSprites;
        sprite = screen->rect;
        Prim_SendPacket((u32 *)screen->sel[buf][1]);
        for (i = 0; i < 3; i++, sprite++, def++) {
            sprite->x = def->x;
            sprite->x += (f32)gLoadingScrollX;
            if (!(sprite->x < 640.0f)) {
                sprite->x -= 1248.0f;
            }
            if (sprite->x < 640.0f && !(416.0f + sprite->x < 0.0f)) {
                Loading_WriteSprite(screen->spr[buf][i], sprite, -16);
                Prim_SendPacket((u32 *)screen->spr[buf][i]);
            }
        }
        sprite = &screen->rect[3];
        def = &gLoadingSprites[3];
        sprite->x = screen->x;
        sprite->y = screen->y;
        if (sprite->x < 288.0f) {
            sprite->u = def->u + (f32)def->tw - 1.0f;
            sprite->tw = -def->tw;
        } else {
            sprite->u = def->u;
            sprite->tw = def->tw;
        }
        /* sel[buf] as a byte offset: indexing the array adds the two
         * registers the other way round */
        firstSel = (u64 **)((u8 *)screen->sel + buf * 16);
        Prim_SendPacket((u32 *)*firstSel);
        Loading_WriteSprite(screen->spr[buf][3], sprite, -16);
        Prim_SendPacket((u32 *)screen->spr[buf][3]);
        Gs_SetState(RS_ZWRITE, 1);
        screen->elapsed += frames;
        Prim_SendPacket((u32 *)screen->sel[buf][2]);
        if (screen->stripScroll < 640.0f) {
            screen->stripScroll += 0.05f;
        }
        sprite = &screen->rect[4];
        def = &gLoadingSprites[4];
        depth = -16;
        for (i = 0; i < 5; i++) {
            *sprite = *def;
            sprite->x -= screen->stripScroll;
            if (!(sprite->x + (f32)sprite->w < 0.0f) && sprite->x <= 640.0f) {
                Loading_WriteSprite(screen->spr[buf][4 + i], sprite, depth);
                Prim_SendPacket((u32 *)screen->spr[buf][4 + i]);
                depth--;
            }
            sprite++;
            def++;
        }
        Gs_ClearColor(0, 0);
        Gs_SetState(RS_ZWRITE, 0);
        Prim_SendPacket((u32 *)*firstSel);
        Prim_SendPacket((u32 *)screen->spr[buf][9]);
        Loading_Animate();
        wave = gLoadingAnimFrame;
        sprite = &screen->rect[10];
        def = &gLoadingSprites[10];
        for (i = 0; i < 10; i++, sprite++, def++) {
            sprite->x = def->x;
            sprite->y = (s32)(def->y + 4.0f * gLoadingBobOffsets[wave]);
            Loading_WriteSprite(screen->spr[buf][10 + i], sprite, -16);
            Prim_SendPacket((u32 *)screen->spr[buf][10 + i]);
            if (i == 2) {
                wave += 2;
            } else {
                wave++;
            }
            if (wave > LOADING_WAVE_STEPS - 1) {
                wave -= LOADING_WAVE_STEPS;
            }
        }
        screen->buf ^= 1;
        dx = 0.3f * Math_Sin(6.2831855f * (180.0f * screen->floatUp) / 360.0f);
        if (Loading_MoveSprite(dx, 0.3f * Math_Cos(6.2831855f * (180.0f * screen->floatUp) / 360.0f)) != 0) {
            screen->floatUp ^= 1;
        }
        if (gLoadingMessage != NULL) {
            Text_Reset();
            Font_SetSpacing(24, 24);
            text = gLoadingMessage;
            Text_DrawShadow((640 - Str_Length(text) * 12) / 2, 200, 1, 0, text);
            Text_DrawPage(0);
        }
        Gs_SetState(RS_ZWRITE, 0);
    }
}
