#ifndef FONT_H
#define FONT_H

#include "common.h"

/*
 * The text drawing: the glyph cache (Font_Init), the text a frame queues on
 * its pages with the pen, size, colour and spacing setters and getters
 * (Font_Set*, Font_Get*), the pages' drawing as sprites (Font_DrawPage,
 * Font_DrawAllPages), the Shift-JIS helpers (Sjis_*) and the colour pulse of
 * highlighted text (Text_UpdatePulse).
 */

/* A string to draw with the glyph cache (Text_DrawRun). */
typedef struct TextRun {
    s16 x;                      /* 0x00 */
    s16 y;                      /* 0x02 */
    f32 size;                   /* 0x04: the glyph size (the cache's +0x54) */
    u8 advance;                 /* 0x08: pen step per glyph */
    u8 charH;                   /* 0x09: the glyph height (a line on a newline) */
    u8 pal;                     /* 0x0A: a palette of the cache */
    u8 isGlyphs;                /* 0x0B: glyphs holds glyph indices, else a Shift-JIS string */
    s32 clut;                   /* 0x0C */
    s16 *glyphs;                /* 0x10: glyph indices ended by a negative one, or the string */
} TextRun;

void Font_Init(s32 unused0);
void TextObj_Reset(void);
void Font_ClearPage(s32 page);
void Font_FlushSlots(void);
void Font_ResetState(void); /* sub_main */
void Font_SetPalette(s32 slot, u32 handle, u32 colorA, u32 colorB, u32 colorC, u32 colorD);
void Font_SetSpacing(u8 penStep, u8 lineStep);
void Font_SetPos(s16 x, s16 y);
void Font_SetSize(f32 size);
void Font_SetColor(s32 color);
void Font_SetColorIndex(s16 index);
void Font_SetPal(s32 palette);
s32 Font_GetPal(void);
u32 Font_GetColor(void);
void Font_SetHalfWidth(s32 unused);
s32 Font_GetPage(void);
s16 Font_GetX(void);
s16 Font_GetY(void);
s16 Font_GetAdvance(void);
s16 Font_GetCharH(void);
s32 Font_GetHalfWidth(void);
s32 Font_IsLineGapOn(void);
void Font_SetLineGapOn(u8 value);
void Font_SetLineGap(s32 value);
void Font_DrawPage(s32 idx);
void Font_DrawAllPages(void);
void Font_DrawAndClearPages(void);
void Font_CreateHandles(void);
void Text_DrawString(TextRun *run);
void Text_DrawRun(TextRun *run);
void Font_DrawGlyph(s32 glyph, TextRun *run);
void Font_BeginPage(void);
void Font_EndFrame(void);
s32 Font_GlyphIndex(u32 code);
s32 Font_SjisToJis(u32 sjis);
void Font_UploadGlyphs(void);
void Font_Expand2To4(u8 *src, u8 *dst);
void Font_CachePage(void);
s32 Font_CacheGlyph(u16 code);
s32 Sjis_FromAscii(u8 ascii);
void Sjis_Widen(char *text, char *out);
void Sjis_PutDigits3(char *dst, s32 n);
void Text_UpdatePulse(void);

#endif
