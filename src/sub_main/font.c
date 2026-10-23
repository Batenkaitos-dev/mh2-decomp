/*
 * The text drawing: the glyph cache (FontCache, gFontCache), set up from a
 * font's descriptor with its glyph bits (Font_Init); the text runs a frame
 * queues on seven pages (TextRun: Font_Printf and the pen, size, colour and
 * spacing setters), drawn page by page as sprites from the glyphs cached
 * in the pages' textures (Font_DrawPage, Font_CacheGlyph,
 * Font_UploadGlyphs); the Shift-JIS helpers (Sjis_*); and the colour pulse
 * of highlighted text (Text_UpdatePulse). The data: the font descriptors,
 * the colours, the cache's two images, the ASCII to Shift-JIS table and
 * the digits.
 *
 * Everything from Font_ClearSlots to Font_CreateHandles has to be one unit:
 * Font_CreateHandles uses v1 for its temporaries because the static
 * Font_ClearSlots leaves v0 alone.
 */

#include "common.h"
#include "syswork.h"
#include "syssettings.h"
#include "texture.h"
#include "render_state.h"
#include "hw_regs.h"
#include "text.h"
#include "font.h"
#include "engine_math.h"
#include "file_load.h"
#include "packet_tim2.h"

#pragma optimization_level 4

/* The glyph cache's sizes. */
#define FONT_PAGES 7                /* the text pages, drawn in order (a run's page comes from its size: Font_SelectPage) */
#define FONT_PAGE_RUNS 128          /* text runs per page (2560 bytes of TextRun each) */
#define FONT_TEXT_SIZE 32768        /* bytes of the runs' strings, all pages together */
#define FONT_GLYPH_CODES 7808       /* glyph indices (Font_GlyphIndex): FontCache.slot's length */
#define FONT_SLOT_NONE 0xFFFF       /* FontCache.slot: the glyph is not cached */
#define FONT_PAL_SIZE 64            /* bytes per palette in FontCache.palPixels */
#define FONT_FILE_ID 5906           /* the compressed file of the glyph bits (LoadCompressedFile) */
/* The glyph cache of the text system (gFontCache), at the start of
 * gFontArena. The fields the comments call "copied" come from the font's
 * FontDesc at Font_Init and nothing reads them. */
typedef struct FontCache {
    s32 font;                   /* 0x0000: the index in gFontDescs */
    s32 unk04;                  /* 0x0004: copied (FontDesc.unk00) */
    s32 width;                  /* 0x0008: glyph pixels */
    s32 height;                 /* 0x000C */
    s32 unk10;                  /* 0x0010: copied (FontDesc.unk0C) */
    s32 unk14;                  /* 0x0014: copied (FontDesc.unk10) */
    s32 unk18;                  /* 0x0018: copied (FontDesc.unk14) */
    u32 glyphs;                 /* 0x001C: glyphs of the font */
    s32 glyphBytes;             /* 0x0020: bytes per glyph at bits */
    s32 (*palInfo)[4];          /* 0x0024: per palette, 4 words for Font_SetPalette */
    s32 palCount;               /* 0x0028 */
    s32 texCount;               /* 0x002C: pages */
    s32 unk30;                  /* 0x0030: copied (FontDesc.unk30) */
    s32 unk34;                  /* 0x0034: copied (FontDesc.unk34) */
    s32 unk38;                  /* 0x0038: copied (FontDesc.unk38) */
    s32 arenaUsed;              /* 0x003C: bytes of gFontArena in use, to the end of text; nothing reads it */
    s32 loaded;                 /* 0x0040: bits is loaded */
    s32 defAdvance;             /* 0x0044: the font's advance (TextObj_Reset) */
    s32 defCharH;               /* 0x0048: the font cell height (TextObj_Reset) */
    u8 *bits;                   /* 0x004C: the font glyphs, 2 bits a pixel */
    s16 runX;                   /* 0x0050: the position of new runs (Font_SetPos) */
    s16 runY;                   /* 0x0052 */
    f32 size;                   /* 0x0054: of new runs (10.0 by default) */
    s16 x;                      /* 0x0058: the pen */
    s16 y;                      /* 0x005A */
    u8 advance;                 /* 0x005C: of new runs (Font_SetSpacing) */
    u8 charH;                   /* 0x005D: the glyph height of new runs (Font_SetSpacing) */
    u8 lineGapOn;               /* 0x005E: Text_PrintCodes adds lineGap to each new line (Text_PrintfStyled) */
    u8 lineGap;                 /* 0x005F: pixels */
    u8 *palPixels;              /* 0x0060: the palettes, FONT_PAL_SIZE bytes each */
    s32 runCount[FONT_PAGES];   /* 0x0064: per page, its text runs */
    s32 runPrev[FONT_PAGES];    /* 0x0080: runCount of the frame before */
    TextRun *runs[FONT_PAGES];  /* 0x009C: per page, FONT_PAGE_RUNS runs */
    u32 textLen;                /* 0x00B8: bytes used in text (at most FONT_TEXT_SIZE) */
    char *text;                 /* 0x00BC: the runs' strings */
    s32 halfWidth;              /* 0x00C0: one-byte characters advance half a glyph (else 2/3) */
    s32 runPage;                /* 0x00C4: the page of new runs (Font_SelectPage) */
    s32 page;                   /* 0x00C8: the page being filled */
    s32 pageDone[FONT_PAGES];   /* 0x00CC: per page, drawn this frame */
    s32 max;                    /* 0x00E8: slots */
    s32 perPage;                /* 0x00EC: slots per page (texture) */
    f32 scale;                  /* 0x00F0: the display width / 640 */
    s32 clutNew;                /* 0x00F4: of new runs */
    s32 palNew;                 /* 0x00F8: of new runs */
    s32 glyphMode;              /* 0x00FC: new runs take glyph indices, not a string */
    s32 unk100;                 /* 0x0100: set to -1 by Font_BeginPage; nothing reads it */
    s16 pageFirst[FONT_PAGES];  /* 0x0104: per page, its first slot */
    s16 pageEnd[FONT_PAGES];    /* 0x0112: per page, the slot after its last */
    u16 *pageGlyphs[FONT_PAGES]; /* 0x0120: per page, the glyph of each slot */
    u16 slot[FONT_GLYPH_CODES]; /* 0x013C: per glyph (Font_GlyphIndex), its slot or FONT_SLOT_NONE */
    s32 used;                   /* 0x3E3C: slots in use */
    u32 *tex;                   /* 0x3E40: per page, a texture handle */
    u32 *pal;                   /* 0x3E44: palette handles */
    s32 pulse;                  /* 0x3E48: frames of the colour pulse (Text_UpdatePulse) */
    s32 clut;                   /* 0x3E4C: for runs without a palette of the cache */
    u8 *pktBase;                /* 0x3E50 */
    u8 *pkt;                    /* 0x3E54: the next packet */
    s32 sprites;                /* 0x3E58 */
    u32 key;                    /* 0x3E5C: texture | palette << 16 of the packets */
    u32 keySet;                 /* 0x3E60: also the TEX1 filter bits */
} FontCache;

extern FontCache *gFontCache;
/* prototypes */

extern s32 gDispWidth; /* main: display width (divided by 640 for the font scale) */
extern s32 gSysFrameMode; /* main */
static void Font_ClearSlots(void);
void * memset(void *p, s32 c, u32 n);
extern s32 gFontRunCount;
/* This file's read-only data. */
#pragma rodata on
char gSjisAsciiTable_0[272] = {
#include "sub_main/0046A200.inc"
};
#pragma rodata off

/* A font of gFontDescs (60 bytes). Font_Init copies it into the cache;
 * the unk fields are copied there and nothing reads them. */
typedef struct FontDesc {
    s32 unk00;
    s32 width;
    s32 height;
    s32 unk0C;
    s32 unk10;
    s32 unk14;
    s32 (*palInfo)[4];          /* per palette, its four colours (Font_SetPalette) */
    s32 palCount;
    s32 texCount;               /* pages of the cache (textures of 256 x 256) */
    s32 glyphs;                 /* glyphs of the font */
    s32 glyphBytes;             /* bytes per glyph, 2 bits a pixel */
    s32 bitsSize;               /* bytes of the glyph bits */
    s32 unk30;
    s32 unk34;
    s32 unk38;
} FontDesc;

extern FontDesc gFontDescs[];
extern u8 gFontArena[];
s32 strlen(char *s);

/* Sets up the glyph cache (gFontCache) in gFontArena: the font descriptor's fields, the page
 * glyph lists, the texture and palette tables, the glyph bits and the text runs; loads the
 * glyph bits (FONT_FILE_ID) once. The bits' size is rounded up in two statements: in one,
 * every temporary takes another register. */
void Font_Init(s32 unused0)
{
    FontDesc *desc;
    s32 bits;
    s32 i;
    u8 *base;
    u8 *cursor;

    base = gFontArena;

    cursor = (u8 *)(((u32)base + 63) & ~0x3F);
    gFontCache = (FontCache *)cursor;
    memset(gFontCache, 0, sizeof(FontCache));
    cursor += 16000;
    gFontCache->font = 0;
    desc = gFontDescs;
    desc += gFontCache->font;
    gFontCache->unk04 = desc->unk00;
    gFontCache->width = desc->width;
    gFontCache->height = desc->height;
    gFontCache->unk10 = desc->unk0C;
    gFontCache->unk14 = desc->unk10;
    gFontCache->unk18 = desc->unk14;
    gFontCache->glyphs = desc->glyphs;
    gFontCache->glyphBytes = desc->glyphBytes;
    gFontCache->palInfo = (void *)desc->palInfo;
    gFontCache->palCount = desc->palCount;
    gFontCache->texCount = desc->texCount;
    gFontCache->unk30 = desc->unk30;
    gFontCache->unk34 = desc->unk34;
    gFontCache->unk38 = desc->unk38;
    bits = desc->bitsSize + 63;
    bits &= ~0x3F;
    gFontCache->perPage = (256U / gFontCache->width) * (256U / gFontCache->height);
    gFontCache->max = gFontCache->perPage * gFontCache->texCount;
    for (i = 0; i < FONT_PAGES; i++) {
        gFontCache->pageGlyphs[i] = (u16 *)cursor;
        cursor += gFontCache->max * 2;
    }
    cursor = (u8 *)(((u32)cursor + 63) & ~0x3F);
    gFontCache->tex = (u32 *)cursor;
    cursor += gFontCache->texCount * 4;
    gFontCache->pal = (u32 *)cursor;
    cursor += gFontCache->texCount * 4;
    cursor = (u8 *)(((u32)cursor + 63) & ~0x3F);
    gFontCache->palPixels = cursor;
    cursor += ((gFontCache->palCount << 6) + 63) & ~0x3F;
    gFontCache->bits = cursor;
    cursor += bits;
    gFontCache->runs[0] = (TextRun *)cursor;
    gFontCache->runs[1] = (TextRun *)(cursor + 2560);
    gFontCache->runs[2] = (TextRun *)(cursor + 5120);
    gFontCache->runs[3] = (TextRun *)(cursor + 7680);
    gFontCache->runs[4] = (TextRun *)(cursor + 10240);
    gFontCache->runs[5] = (TextRun *)(cursor + 12800);
    gFontCache->runs[6] = (TextRun *)(cursor + 15360);
    gFontCache->text = (char *)(cursor + 17920);
    gFontCache->defAdvance = gFontCache->width;
    gFontCache->defCharH = gFontCache->height;
    gFontCache->arenaUsed = (s32)cursor + 17920 + FONT_TEXT_SIZE - (s32)base;
    Font_CreateHandles();
    if (gFontCache->loaded == 0) {
        LoadCompressedFile(gFontCache->bits, FONT_FILE_ID);
        gFontCache->loaded = 1;
    }
    gFontCache->scale = (f32)gDispWidth / 640.0f;
    Font_ResetState();
}

static void Font_SelectPage(void);

/* Resets the glyph cache's text state: every page's runs (as Font_ClearPage), the position,
 * the size (10.0), the font's advance and height, the text buffer; then the page of new
 * runs (Font_SelectPage). */
void TextObj_Reset(void)
{
    s32 i;

    for (i = 0; i < FONT_PAGES; i++) {
        gFontCache->runCount[i] = 0;
        gFontCache->runPrev[i] = 0;
        gFontCache->pageDone[i] = 0;
    }
    gFontCache->runX = 0;
    gFontCache->runY = 0;
    gFontCache->size = 10.0f;
    gFontCache->advance = gFontCache->defAdvance;
    gFontCache->charH = gFontCache->defCharH;
    gFontCache->textLen = 0;
    gFontCache->halfWidth = 1;
    Font_SelectPage();
}

/* Clears page `page`'s runs, this frame's and the last, and its drawn flag. */
void Font_ClearPage(s32 page)
{
    gFontCache->runCount[page] = 0;
    gFontCache->runPrev[page] = 0;
    gFontCache->pageDone[page] = 0;
}

/* Empties the glyph cache: every glyph's slot (FONT_SLOT_NONE) and every page's range. */
static void Font_ClearSlots(void)
{
    s32 i;
    u16 *slotEntry;

    gFontCache->used = 0;
    slotEntry = gFontCache->slot;
    for (i = 0; i < FONT_GLYPH_CODES; i++) {
        *slotEntry++ = FONT_SLOT_NONE;
    }
    for (i = 0; i < FONT_PAGES; i++) {
        gFontCache->pageFirst[i] = 0;
        gFontCache->pageEnd[i] = 0;
        gFontCache->pageDone[i] = 0;
    }
}

/* Empties the glyph cache and clears the count of runs that found their page full. */
void Font_FlushSlots(void)
{
    Font_ClearSlots();
    gFontRunCount = 0;
}

/* Resets the text state (TextObj_Reset), one-byte characters half wide. */
void Font_ResetState(void)
{
    TextObj_Reset();
    Font_SetHalfWidth(1);
}

/* Sets the four colours of palette slot `slot`: in gFontCache's table and in the locked palette. */
void Font_SetPalette(s32 slot, u32 handle, u32 colorA, u32 colorB, u32 colorC, u32 colorD)
{
    u32 colors[4];
    Image img;
    u32 *cacheDst;
    u32 *paletteDst;
    s32 i;

    if (handle == 0) {
        return;
    }
    colors[0] = colorA;
    colors[1] = colorB;
    colors[2] = colorC;
    colors[3] = colorD;
    flLockPalette(NULL, handle, &img, 2);
    cacheDst = (u32 *)(gFontCache->palPixels + slot * FONT_PAL_SIZE);
    paletteDst = (u32 *)img.pixels;
    for (i = 0; i < 4; i++) {
        *cacheDst++ = colors[i];
        *paletteDst++ = colors[i];
    }
    flUnlockPalette(handle);
}

/* The advance and the line height of new runs, in pixels. */
void Font_SetSpacing(u8 penStep, u8 lineStep)
{
    gFontCache->advance = (s8) penStep;
    gFontCache->charH = (s8) lineStep;
}

/* The page of the next text run, from the glyph size (size / 1000 of the 7 pages). */
static void Font_SelectPage(void)
{
    s32 page;

    page = (s32) (7.0f * (gFontCache->size / 1000.0f));
    if (page < 0) {
        page = 0;
    }
    if (page >= FONT_PAGES) {
        page = FONT_PAGES - 1;
    }
    gFontCache->runPage = page;
}

/* The next text run's position; also picks its page. */
void Font_SetPos(s16 x, s16 y)
{
    gFontCache->runX = x;
    gFontCache->runY = y;
    Font_SelectPage();
}

/* Static: Font_NewRun keeps a2 across the call. */
static void Font_NextPage(s32 unused0, s32 unused1, s32 unused2)
{
    s32 page;

    page = gFontCache->runPage + 1;
    if (page < 0) {
        page = 0;
    }
    if (page >= FONT_PAGES) {
        page = FONT_PAGES - 1;
    }
    gFontCache->runPage = page;
}

/* The glyph size of new runs; it also picks their page (Font_SelectPage). */
void Font_SetSize(f32 size)
{
    gFontCache->size = size;
    Font_SelectPage();
}

/* The colour of new runs, 0xRRGGBB, with palette mode 0: kept in the GS's order (red in
 * the low byte) with alpha 128, which the GS reads as opaque. */
void Font_SetColor(s32 color)
{
    Font_SetPal(0);
    gFontCache->clutNew = (s32) ((color & 0xFF00) | (((color & 0xFF) << 16) | ((u32) (color & 0xFF0000) >> 16)) | 0x80000000);
}

/* As Font_SetColor, without setting the palette mode. */
static void Font_SetColorRaw(s32 rgb)
{
    gFontCache->clutNew = (s32) ((rgb & 0xFF00) | (((rgb & 0xFF) << 16) | ((u32) (rgb & 0xFF0000) >> 16)) | 0x80000000);
}

extern u32 gTextPulseColor;
extern u32 gFontColors[];

/*
 * Sets the text colour: -1 the pulsing one (gTextPulseColor), -2 and -3 palette modes 2
 * and 3 with colour 0, 0-15 that colour of gFontColors in mode 0. Font_SetColorRaw is
 * static (only this file calls it): MWCC then knows it leaves v0 alone and keeps v0 out
 * of the index, as in the game.
 */
void Font_SetColorIndex(s16 index)
{
    s16 mode;

    if (index >= 16) {
        return;
    }
    switch (index) {
    case -1:
        Font_SetPal(0);
        Font_SetColor(gTextPulseColor);
        return;
    case -2:
        mode = 2;
        index = 0;
        break;
    case -3:
        mode = 3;
        index = 0;
        break;
    default:
        if (index >= 16) {
            return;
        }
        mode = 0;
        break;
    }
    Font_SetPal(mode);
    Font_SetColorRaw(gFontColors[index]);
}

/* The palette mode of new runs. */
void Font_SetPal(s32 palette)
{
    gFontCache->palNew = palette;
}

s32 Font_GetPal(void)
{
    return gFontCache->palNew;
}

/* The colour of new runs, 0xAARRGGBB (Font_SetColor's order). */
u32 Font_GetColor(void)
{
    s32 clutNew;

    clutNew = gFontCache->clutNew;
    return ((clutNew & 0xFF) << 16) | ((clutNew & 0xFF00FF00) | ((u32) (clutNew & 0xFF0000) >> 16));
}

/* One-byte characters advance half a glyph; the argument is ignored. */
void Font_SetHalfWidth(s32 unused)
{
    gFontCache->halfWidth = 1;
}

/* A free text run on the page of new runs. While that page is full, the run moves to the
 * next page (Font_NextPage) and gFontRunCount counts it; NULL when the last page is full.
 * The caller counts the run in runCount. Font_NextPage's arguments keep the page's
 * address and the counts in the registers the game uses. */
TextRun *Font_NewRun(void)
{
    s32 *page;
    s32 runIndex;
    s32 oldPage;

    for (;;) {
        page = &gFontCache->runPage;
        runIndex = gFontCache->runCount[*page];
        if (runIndex < FONT_PAGE_RUNS) {
            break;
        }
        gFontRunCount++;
        oldPage = *page;
        Font_NextPage((s32)page, runIndex, oldPage);
        if (oldPage == gFontCache->runPage) {
            return NULL;
        }
    }
    return &gFontCache->runs[*page][runIndex];
}

/* Getters and setters of the glyph cache's fields; strlen. */
s32 Font_GetPage(void)
{
    return gFontCache->runPage;
}

s16 Font_GetX(void)
{
    return gFontCache->runX;
}

s16 Font_GetY(void)
{
    return gFontCache->runY;
}

s16 Font_GetAdvance(void)
{
    return gFontCache->advance;
}

s16 Font_GetCharH(void)
{
    return gFontCache->charH;
}

s32 Font_GetHalfWidth(void)
{
    return gFontCache->halfWidth;
}

s32 Font_IsLineGapOn(void)
{
    return gFontCache->lineGapOn;
}

s32 Font_GetLineGap(void)
{
    return (s16)gFontCache->lineGap;
}

void Font_SetLineGapOn(u8 value)
{
    gFontCache->lineGapOn = value;
}

void Font_SetLineGap(s32 value)
{
    gFontCache->lineGap = value;
}

s32 Str_Length(char *str)
{
    return strlen(str);
}

#include "ee_stdarg.h"
s32 vsprintf(char *buf, const char *fmt, va_list ap);

/* Adds a text run with the cache's current position, size, colours and
 * palette: a printf into the text buffer, or (glyph mode) a copy of the
 * glyph indices up to the negative one that ends them. */
void Font_Printf(char *fmt, ...)
{
    TextRun *run;
    va_list args;
    s16 *src;
    s16 *dst;

    run = Font_NewRun();
    if (run != NULL) {
        gFontCache->runCount[gFontCache->runPage]++;
        run->x = gFontCache->runX;
        run->y = gFontCache->runY;
        run->size = gFontCache->size;
        run->advance = gFontCache->advance;
        run->charH = gFontCache->charH;
        run->clut = gFontCache->clutNew;
        run->pal = gFontCache->palNew;
        run->isGlyphs = gFontCache->glyphMode;
        if (gFontCache->glyphMode == 0) {
            run->glyphs = (s16 *)(gFontCache->text + gFontCache->textLen);
            va_start(args, fmt);
            gFontCache->textLen += vsprintf((char *)run->glyphs, fmt, args);
            if (gFontCache->textLen >= FONT_TEXT_SIZE) {
                gFontCache->textLen = FONT_TEXT_SIZE - 1;
            }
            gFontCache->text[gFontCache->textLen] = 0;
            gFontCache->textLen++;
        } else {
            gFontCache->textLen = (gFontCache->textLen + 1) & ~1;
            run->glyphs = (s16 *)(gFontCache->text + gFontCache->textLen);
            dst = run->glyphs;
            src = (s16 *)fmt;
            for (;;) {
                *dst = *src;
                gFontCache->textLen += 2;
                if (*src < 0) {
                    break;
                }
                dst++;
                src++;
            }
            gFontCache->text[gFontCache->textLen] = 0;
            gFontCache->textLen++;
        }
    }
}

/* Draws the text runs of page idx once a frame; all pages; the frame's end. */
void Font_DrawPage(s32 idx)
{
    TextRun *run;
    s32 k;

    if (idx < FONT_PAGES) {
        if (gFontCache->pageDone[idx] == 0 && gFontCache->runCount[idx] != 0) {
            gFontCache->page = idx;
            Font_BeginPage();
            run = gFontCache->runs[idx];
            for (k = 0; k < gFontCache->runCount[idx]; k++, run++) {
                if (run->isGlyphs == 0) {
                    Text_DrawString(run);
                } else {
                    Text_DrawRun(run);
                }
            }
            Font_EndFrame();
            gFontCache->pageDone[idx] = 1;
        }
    }
}

/* Draws the text runs of all 7 pages. */
void Font_DrawAllPages(void)
{
    s32 i;

    for (i = 0; i < FONT_PAGES; i++) {
        Font_DrawPage(i);
    }
}

/* Draws the text runs of all 7 pages, then starts the next frame's runs. */
void Font_DrawAndClearPages(void)
{
    s32 i;

    for (i = 0; i < FONT_PAGES; i++) {
        Font_DrawPage(i);
        gFontCache->pageDone[i] = 0;
    }
    for (i = 0; i < FONT_PAGES; i++) {
        gFontCache->runPrev[i] = gFontCache->runCount[i];
        gFontCache->runCount[i] = 0;
    }
}

extern Image gFontImage;
extern Image gFontClutImage;

/* Creates the glyph cache texture handles (one per page) and palette handles. */
void Font_CreateHandles(void)
{
    s32 i;
    u32 *texHandle;
    s32 (*info)[4];
    u8 *pixels;
    u32 *palHandle;
    u32 palIndex;
    u32 handle;

    texHandle = gFontCache->tex;
    for (i = 0; i < gFontCache->texCount; i++) {
        *texHandle++ = flCreateTextureHandle(&gFontImage, TEX_MODE_ON_DEMAND);
    }
    palHandle = gFontCache->pal;
    info = gFontCache->palInfo;
    pixels = gFontCache->palPixels;
    for (palIndex = 0; palIndex < gFontCache->palCount; palIndex++) {
        gFontClutImage.pixels = pixels;
        handle = flCreatePaletteHandle(&gFontClutImage, TEX_MODE_ON_DEMAND);
        *palHandle++ = handle;
        Font_SetPalette(palIndex, handle, (*info)[0], (*info)[1], (*info)[2], (*info)[3]);
        info++;
        pixels += FONT_PAL_SIZE;
    }
    Font_ClearSlots();
}

/* Draws the Shift-JIS string of run (newlines go down by run->charH). */
void Text_DrawString(TextRun *run)
{
    u8 *str = (u8 *)run->glyphs;
    u32 code;
    s32 narrow;
    s32 k;

    gFontCache->x = run->x;
    gFontCache->y = run->y;
    for (;;) {
        code = *str++;
        if (code == 0) {
            break;
        }
        if (code == '\n') {
            gFontCache->x = run->x;
            gFontCache->y += run->charH;
            continue;
        }
        if (SJIS_IS_LEAD(code)) {
            if (*str == 0) {
                break;
            }
            code <<= 8;
            code |= *str++;
            narrow = 0;
        } else {
            code = Sjis_FromAscii(code);
            narrow = 1;
        }
        if (code != SJIS_SPACE) {
            k = Font_GlyphIndex(code);
            if (k >= 0) {
                Font_DrawGlyph(k, run);
            }
        }
        if (!narrow) {
            gFontCache->x += run->advance;
        } else if (gFontCache->halfWidth != 0) {
            gFontCache->x += run->advance / 2;
        } else {
            gFontCache->x += run->advance * 2 / 3;
        }
    }
}

/* Draws the glyphs of run from its position, one advance apart (Font_DrawGlyph). */
void Text_DrawRun(TextRun *run)
{
    s16 *glyph = run->glyphs;
    s32 code;

    gFontCache->x = run->x;
    gFontCache->y = run->y;
    for (;;) {
        code = *glyph++;
        if (code < 0) {
            break;
        }
        if ((u32)code < gFontCache->glyphs) {
            Font_DrawGlyph(code, run);
        }
        gFontCache->x += run->advance;
    }
}

/* 2 for a Shift-JIS lead byte (0x80-0x9F, 0xE0-0xFF), 1 otherwise. */
s32 Sjis_CharBytes(u8 *str)
{
    s32 lead = *str;

    if (SJIS_IS_LEAD(lead)) {
        return 2;
    }
    return 1;
}

/* The GS TEX0 register (Sony's SCE_GS_SET_TEX0 layout). */
#define GS_TEX0(tbp, tbw, psm, tw, th, tcc, tfx, cbp, cpsm, csm, csa, cld)                     \
    ((u64)(tbp) | ((u64)(tbw) << 14) | ((u64)(psm) << 20) | ((u64)(tw) << 26) |                 \
     ((u64)(th) << 30) | ((u64)(tcc) << 34) | ((u64)(tfx) << 35) | ((u64)(cbp) << 37) |         \
     ((u64)(cpsm) << 51) | ((u64)(csm) << 55) | ((u64)(csa) << 56) | ((u64)(cld) << 61))

extern u32 gPacketEnd;
/*
 * Queues the GS registers (TEX0_1, TEX1_1) of glyph texture key (texture | palette << 16).
 * Static: only Font_DrawGlyph calls it, and keeps its values in t7-t9 across the call.
 */
static void Font_SetTexRegs(u32 key)
{
    u32 *out;
    TexRec *tex;
    TexRec *clut;

    tex = &gTexRecs[(key & 0xFFFF) - 1];
    clut = &gPalRecs[((key & 0xFFFF0000) >> 16) - 1];
    out = (u32 *)gFontCache->pkt;

    if (gPacketEnd - (u32)out < 64) {
        return;
    }
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0x50000003;
    ((u64 *)out)[2] = 0x1000000000008002;
    ((u64 *)out)[3] = 14;
    ((u64 *)out)[4] = GS_TEX0(tex->vram, 4, tex->psm, tex->tw, tex->th, 1, 0, clut->vram, clut->psm, 0, 0, 2);
    ((u64 *)out)[5] = GS_REG_TEX0_1;
    ((u64 *)out)[6] = ((u64)gFontCache->keySet << 5) | ((u64)gFontCache->keySet << 6);
    ((u64 *)out)[7] = GS_REG_TEX1_1;
    gFontCache->pkt += 64;
}

extern s32 gDispHeight; /* main */

/* Adds the sprite of the glyph in cache slot `slot`, glyphW x glyphH from the pen, to the
 * font packets (clut is the TEX0 palette word); the cells are cellW x cellH in 256-texel
 * pages. Static, as Font_DrawGlyph (its only caller) needs: MWCC then knows which
 * registers it clobbers.
 * The order and the types are the game's register use, kept for the match: the far texel
 * corner computed in place (`cellW = texU + cellW - 1; texV += cellH;`), the screen
 * offsets added in place after the frame-mode halving, top a copy of y scaled in place;
 * scale read before left, slot % perPage assigned back to slot, x an s16 (the game
 * extends it again for x + glyphW), y an int cast back to s16 for y + glyphH.
 */
static void Font_AddGlyphSprite(s32 slot, u32 clut, s16 glyphW, s16 glyphH)
{
    u32 *out;
    u32 texU;
    u32 texV;
    u32 left;
    u32 right;
    u32 top;
    s32 originX;
    u32 bottom;
    s32 originY;
    u32 cellW;
    u32 cellH;
    s16 x;
    s32 y;
    f32 scale;

    if (gPacketEnd - (u32)gFontCache->pkt < 80) {
        return;
    }
    cellW = gFontCache->width;
    cellH = gFontCache->height;
    gFontCache->sprites++;
    out = (u32 *)gFontCache->pkt;
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0x50000004;
    ((u64 *)out)[2] = 0x8001 | ((u64)0x64000000 << 32);
    ((u64 *)out)[3] = 0x535310;
    ((u64 *)out)[4] = 342;
    ((u64 *)out)[5] = clut;
    slot = slot % gFontCache->perPage;
    texU = cellW * (slot % (256 / cellW));
    texV = cellH * (slot / (256 / cellH));
    ((u64 *)out)[6] = (u32)(texU * 16 + 8) | ((u64)(u32)(texV * 16 + 8) << 16);
    originX = ((4096 - gDispWidth) / 2) * 16;
    originY = ((4096 - gDispHeight) / 2) * 16;
    x = gFontCache->x;
    scale = gFontCache->scale;
    left = 16.0f * (x * scale);
    y = gFontCache->y;
    right = 16.0f * (scale * (x + glyphW));
    top = y;
    bottom = (s16)y + glyphH;
    top *= 16;
    bottom *= 16;
    if (gSysFrameMode == FRAME_MODE_FIELD) {
        top >>= 1;
        bottom >>= 1;
    }
    left += originX;
    right += originX;
    top += originY;
    bottom += originY;
    cellW = texU + cellW - 1;
    texV += cellH;
    ((u64 *)out)[7] = (u32)left | ((u64)(u32)top << 16) | ((u64)0xFFFFFFFF << 32);
    ((u64 *)out)[8] = (u32)(cellW * 16 + 8) | ((u64)(u32)(texV * 16 + 8) << 16);
    ((u64 *)out)[9] = (u32)right | ((u64)(u32)bottom << 16) | ((u64)0xFFFFFFFF << 32);
    gFontCache->pkt += 80;
}

/*
 * Draws glyph `glyph` of run from the glyph cache: switches the texture and palette when
 * they change (Font_SetTexRegs), then the sprite (Font_AddGlyphSprite).
 * Its registers come from Font_AddGlyphSprite being static C defined before it: MWCC then
 * knows that it leaves v0 alone, keeps v0 live to the end and puts slot, clut and run in
 * t7-t9 as the game does. slot is an s32 (the game passes the lhu value without a mask).
 */
void Font_DrawGlyph(s32 glyph, TextRun *run)
{
    s32 clut;
    u32 key;
    s32 slot = gFontCache->slot[glyph];

    if (slot == FONT_SLOT_NONE) {
        return;
    }
    if (run->pal < 0) {
        clut = gFontCache->clut;
    } else {
        clut = run->clut;
    }
    key = gFontCache->tex[slot / gFontCache->perPage];
    key |= gFontCache->pal[(u8)run->pal] << 16;
    if (key != gFontCache->key || gFontCache->keySet != 1) {
        gFontCache->key = key;
        gFontCache->keySet = 1;
        Font_SetTexRegs(key);
    }
    Font_AddGlyphSprite(slot, clut, run->advance, run->charH);
}

extern s32 gPacketCur;
/* Starts drawing a page of text: the render states for glyphs (alpha test, no depth
 * write, blend 4, 5), the page's glyphs cached and uploaded (Font_CachePage,
 * Font_UploadGlyphs), and a new packet in the packet buffer, its first quadword kept
 * for the DMA tag Font_EndFrame writes. */
void Font_BeginPage(void)
{
    s32 pkt;

    Gs_SetState(RS_BLEND_ON, RF_BLEND_ON);
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_ALPHA_TEST, RS_CMP_NOTEQUAL);
    gSysSettings.blendA = 255;
    gSysSettings.blendB = 255;
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Gs_SetState(RS_ZWRITE, 0);
    gFontCache->unk100 = -1;
    Font_CachePage();
    Font_UploadGlyphs();
    gFontCache->pktBase = (u8 *)((gPacketCur + 15) & ~0xF);
    gFontCache->pkt = gFontCache->pktBase;
    pkt = (s32)gFontCache->pkt;
    if ((u32) (*(s32 *)&gPacketEnd - pkt) >= 16U) {
        gFontCache->pkt = (u8 *)(pkt + 16);
        gFontCache->sprites = 0;
        gFontCache->key = 0;
        gFontCache->keySet = -1;
    }
}

extern DmaQueue gDmaQueue;

/*
 * Ends the glyph cache's frame: closes its packet (a CNT tag over the glyphs, an END tag)
 * and queues it, resets two render states, then evicts the cache's textures and palettes.
 */
void Font_EndFrame(void)
{
    u32 *base;
    u32 *end;
    s32 i;
    u32 *texHandle;
    u32 *palHandle;
    s32 j;

    if (gFontCache->pktBase < gFontCache->pkt) {
        base = (u32 *)gFontCache->pktBase;
        base[0] = (((u32)(gFontCache->pkt - (u8 *)base) >> 4) - 1) | DMA_TAG_CNT;
        base[1] = 0;
        base[2] = 0;
        base[3] = 0;
        end = (u32 *)gFontCache->pkt;
        end[0] = DMA_TAG_IRQ | DMA_TAG_END | 1;
        end[1] = 0;
        end[2] = 0;
        end[3] = 0;
        *(u128 *)&end[4] = 0;
        gFontCache->pkt += 32;
        Packet_Alloc(gFontCache->pkt - gFontCache->pktBase, 16);
        DmaQueue_AddChain(0, (u32)gFontCache->pktBase & 0x0FFFFFFF, end, &gDmaQueue);
    }
    Gs_SetState(RS_ALPHA_TEST, RS_CMP_GREATER);
    Gs_SetState(RS_ZWRITE, 1);
    for (i = 0, texHandle = gFontCache->tex; i < gFontCache->texCount; i++, texHandle++) {
        if ((u16)*texHandle != 0) {
            Tex_Evict(&gTexRecs[(u16)*texHandle - 1]);
        }
    }
    for (j = 0, palHandle = gFontCache->pal; j < gFontCache->palCount; j++, palHandle++) {
        if (*palHandle != 0) {
            Tex_Evict(&gPalRecs[*palHandle - 1]);
        }
    }
}

/* The low byte of a JIS code. An inline helper: written as `jis & 0xFF` in place,
 * MWCC schedules that mask before the high byte's shift. */
static inline s32 Jis_Low(s32 jis)
{
    return jis & 0xFF;
}

/* The font glyph index of a Shift-JIS code (row * 94 + column of its JIS code), or -1
 * past the table (FONT_GLYPH_CODES glyphs). */
s32 Font_GlyphIndex(u32 code)
{
    s32 jis = Font_SjisToJis(code);
    s32 i = ((jis >> 8) - 33) * 94 + (Jis_Low(jis) - 33);

    if (i >= FONT_GLYPH_CODES) {
        i = -1;
    }
    return i;
}

/*
 * Shift JIS to JIS X 0208: the first byte (0x81-0x9F, 0xE0-0xEF) gives the row pair, the
 * trail byte (0x40-0x7E, 0x80-0x9E, 0x9F-0xFC) the column and the odd/even row.
 * The second lead-byte case, the second trail-byte case and the sum are done in 64 bits
 * (daddiu, then dsll32/dsra32): `lead` is the first byte as a long, computed by the same
 * expression as jisHigh (MWCC shares the andi, so lead needs no masking or extension).
 */
s32 Font_SjisToJis(u32 sjis)
{
    s32 jisHigh = (sjis >> 8) & 0xFF;
    long lead = (sjis >> 8) & 0xFF;
    u32 jisLow = sjis & 0xFF;
    long trail;

    if (jisHigh >= 129 && jisHigh < 160) {
        jisHigh -= 129;
    } else if (jisHigh >= 224 && jisHigh < 240) {
        jisHigh = lead - 193;
    }
    trail = jisLow;
    jisHigh <<= 1;
    if (trail >= 64 && trail < 127) {
        jisLow -= 64;
    } else if (trail >= 128 && trail < 159) {
        jisLow = trail - 65;
    } else if (trail >= 159 && trail < 253) {
        jisLow -= 159;
        jisHigh++;
    }
    return (((long)(u32)jisHigh + 1) << 8) + (long)jisLow + 8225;
}

/* Draws the glyphs of the page being filled (its slots pageFirst..pageEnd) into the page
 * textures, then hands every texture and palette handle to Tex_Use. */
void Font_UploadGlyphs(void)
{
    Image img;
    u32 handles[8];
    u32 tex;
    s32 i;
    s32 end;
    s32 cur;
    u16 *glyph;
    u32 perRow;
    u32 perCol;
    s32 n;

    perRow = 256 / (u32)gFontCache->width;
    perCol = 256 / (u32)gFontCache->height;
    end = gFontCache->pageEnd[gFontCache->page];
    i = gFontCache->pageFirst[gFontCache->page];
    glyph = gFontCache->pageGlyphs[gFontCache->page];
    cur = i / gFontCache->perPage;
    if (end - i != 0) {
        Dma_WaitIdle();
        tex = gFontCache->tex[cur];
        if (tex != 0) {
            flLockTexture(0, tex, &img, 2);
        }
        for (; i < end; i++) {
            if (cur != i / gFontCache->perPage) {
                if (tex != 0) {
                    flUnlockTexture(tex);
                }
                cur = i / gFontCache->perPage;
                tex = gFontCache->tex[cur];
                if (tex == 0) {
                    break;
                }
                flLockTexture(0, tex, &img, 2);
            }
            Font_Expand2To4(gFontCache->bits + *glyph++ * gFontCache->glyphBytes,
                            img.pixels + ((u32)(i % gFontCache->perPage) / perCol) * (gFontCache->height * 128)
                                + ((u32)gFontCache->width >> 1) * (i % perRow));
        }
        if (tex != 0) {
            flUnlockTexture(tex);
        }
    }
    n = 0;
    for (i = 0; i < gFontCache->texCount; i++) {
        handles[n++] = gFontCache->tex[i];
    }
    for (i = 0; i < gFontCache->palCount; i++) {
        handles[n++] = gFontCache->pal[i] << 16;
    }
    Tex_Use(n, handles);
}

/* The font's 2-bit pixels at src as 4-bit ones at dst (rows of 128 bytes). */
void Font_Expand2To4(u8 *src, u8 *dst)
{
    s32 row;
    s32 i;
    s32 rows;
    s32 rowBytes;
    u16 *out;
    s32 packed;

    rows = gFontCache->height;
    rowBytes = (u32)((gFontCache->width + 3) & ~3) >> 2;
    for (row = 0; row < rows; row++) {
        out = (u16 *)dst;
        for (i = 0; i < rowBytes; i++) {
            packed = *src++;
            *out++ = ((packed & 0xC0) >> 6) | (packed & 0x30) | ((packed & 0xC) << 6) | ((packed & 3) << 12);
        }
        dst += 128;
    }
}

/* The largest end slot of the cache's seven pages (at least 0). Static: Font_CachePage keeps
 * gFontCache in a3 across the call. */
static s16 Font_MaxPageEnd(void)
{
    s32 i;
    s16 max = 0;

    for (i = 0; i < FONT_PAGES; i++) {
        if (gFontCache->pageEnd[i] > max) {
            max = gFontCache->pageEnd[i];
        }
    }
    return max;
}

/* Starts a new page of the glyph cache and gives every glyph of its text runs a slot. */
void Font_CachePage(void)
{
    FontCache *cache;
    s16 end;
    TextRun *run;
    s32 i;
    s32 n;
    s16 *glyph;
    u8 *str;
    u32 code;
    s32 k;

    cache = gFontCache;
    end = Font_MaxPageEnd();
    cache->pageEnd[cache->page] = end;
    cache->pageFirst[cache->page] = end;
    n = gFontCache->runCount[gFontCache->page];
    run = gFontCache->runs[gFontCache->page];
    for (i = 0; i < n; i++, run++) {
        if (run->isGlyphs != 0) {
            glyph = run->glyphs;
            for (;;) {
                k = *glyph++;
                if (k < 0) {
                    break;
                }
                Font_CacheGlyph(k);
            }
        } else {
            str = (u8 *)run->glyphs;
            for (;;) {
                code = *str++;
                if (code == 0) {
                    break;
                }
                if (code == '\n') {
                    continue;
                }
                if (SJIS_IS_LEAD(code)) {
                    if (*str == 0) {
                        break;
                    }
                    code <<= 8;
                    code |= *str++;
                } else {
                    code = Sjis_FromAscii(code);
                }
                if (code == SJIS_SPACE) {
                    continue;
                }
                k = Font_GlyphIndex(code);
                if (k >= 0 && k < FONT_GLYPH_CODES) {
                    Font_CacheGlyph(k);
                }
            }
        }
    }
}

/* Gives glyph code a slot of the cache (on the current page) unless it has one; 0 when full. */
s32 Font_CacheGlyph(u16 code)
{
    if (gFontCache->slot[code] != FONT_SLOT_NONE) {
        return 1;
    }
    if (gFontCache->used >= gFontCache->max) {
        return 0;
    }
    gFontCache->slot[code] = gFontCache->used;
    gFontCache->pageGlyphs[gFontCache->page][gFontCache->pageEnd[gFontCache->page] - gFontCache->pageFirst[gFontCache->page]] = code;
    gFontCache->used++;
    gFontCache->pageEnd[gFontCache->page]++;
    return 1;
}

/* The two-byte code of the glyph the font draws for a one-byte character: the full-width
 * space below 0x21, else a glyph of the font's row 0x85 (the trail byte skips 0x7F, as
 * Shift-JIS does; 0xA5 is the exception). */
s32 Sjis_FromAscii(u8 ascii)
{
    s32 code;

    code = ascii & 0xFF;
    if (code < 33) {
        return SJIS_SPACE;
    }
    if (code < 96) {
        return code + 32767 + 1312;
    }
    if (code < 128) {
        return code + 32767 + 1313;
    }
    if (code == 165) {
        return 34211;
    }
    return code + 32767 + 1279;
}

extern u8 *gSjisAsciiTable;

/* out = text with its one-byte characters widened to two-byte Shift-JIS ones (the table at
 * gSjisAsciiTable); two-byte characters are copied. */
void Sjis_Widen(char *text, char *out)
{
    u8 *src = (u8 *)text;
    u8 *dst = (u8 *)out;
    s32 lead;
    u8 *wideChar;

    while ((lead = *src) != 0) {
        if (SJIS_IS_LEAD(lead)) {
            dst[0] = lead;
            dst[1] = src[1];
            src += 2;
            dst += 2;
        } else {
            wideChar = gSjisAsciiTable + lead * 2;
            src++;
            dst[0] = wideChar[0];
            dst[1] = wideChar[1];
            dst += 2;
        }
    }
    *dst = 0;
}

extern char gSjisDigits[];
char *strncpy(char *dst, const char *src, u32 n);

/* dst = the last three decimal digits of n as two-byte Shift-JIS digits (gSjisDigits), no
 * terminator. */
void Sjis_PutDigits3(char *dst, s32 n)
{
    s32 i;
    s32 digit;

    for (i = 0, digit = 1000; i < 3; i++) {
        n %= digit;
        digit /= 10;
        strncpy(dst, &gSjisDigits[n / digit * 2], 2);
        dst += 2;
    }
}

/* gTextPulseColor = a colour pulsing with a sine over 32 frames (around 0x50, 0xE4, 0xF7). */
void Text_UpdatePulse(void)
{
    f32 wave = Math_Sin(3.1415927f / 32768.0f * (u16)((gFontCache->pulse & 0x1F) << 11));
    s8 red = 64.0f * wave;
    s8 green = 20.0f * wave;
    s8 blue = 7.0f * wave;

    gTextPulseColor = 0xFF000000 | (u8)(red + 80) << 16 | (u8)(green + 228) << 8 | (u8)(blue + 247);
    gFontCache->pulse++;
}

/* This file's initialised data. */
s32 gFontPalColors[4][4] = {
#include "sub_main/00402280.inc"
};
FontDesc gFontDescs[4] = {
#include "sub_main/004022C0.inc"
};
u32 gFontColors[16] = {
#include "sub_main/004023B0.inc"
};
Image gFontImage = {
#include "sub_main/004023F0.inc"
};
Image gFontClutImage = {
#include "sub_main/00402440.inc"
};
u8 *gSjisAsciiTable = (u8 *)gSjisAsciiTable_0;
char gSjisDigits[32] = {
#include "sub_main/00402490.inc"
};
