/*
 * Small drawing helpers of the menus and the HUD: the pad button icons
 * (Hud_DrawButtonIcon), the wrapping of Shift-JIS text, the play-time
 * strings, the black line and the pulsing highlight bar, and the loading
 * letters that bob along the screen while the game loads (their sprite,
 * wave and positions are this file's data).
 */
#include "common.h"
#include "syssettings.h"
#include "sprite.h"
#include "prim.h"
#include "model.h"
#include "sub_main/draw_helpers.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "overlay.h"
#include "texture.h"
#include "text.h"
#include "draw_helpers.h"
#include "engine_math.h"
#include "font.h"
#include "main.h"
#include "tex_files.h"
#pragma optimization_level 4

extern Sprite gBlackLineSprite __attribute__((aligned(16)));
extern char gTimeStrMinSec[];       /* Time_FormatMinSec's string */
extern char gTimeStrMinSecInHour[]; /* Time_FormatMinSecInHour's string */

struct TexRect;
void TexRect_DrawRotated(struct TexRect *r, s32 angle);
s32 sprintf(char *dst, char *fmt, ...);     /* sprintf */
/* unit's own declaration: declared as this file's callers use it; with the definition's void (s16 x, s16 y, s16 w, s16 h, s32 color), Gfx_DrawBlackLine compiles differently */
void Gfx_DrawLine(s32 a, s32 b, s32 c, s32 d, u32 color);

/*
 * Draws pad button icon id (a 24x24 cell of the icon sheet, 32 wide for 8 and 9)
 * at x, y scaled by scale. 22-24 are arrows drawn from cell 16 (flipped or
 * rotated), 20/21 use cells 0/1, 25 is a small 18x18 one. depth is unused.
 */
void Hud_DrawButtonIcon(s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale)
{
    Sprite spr;
    f32 size;
    s32 iconId;
    s16 cell;

    iconId = (s16)id;
    if (iconId == 22 || iconId == 23 || iconId == 24) {
        id = 16;
    }
    cell = id;
    if (cell == 8 || cell == 9) {
        size = 32.0f;
    } else {
        size = 24.0f;
    }
    if (cell == 20) {
        id = 0;
    } else if (cell == 21) {
        id = 1;
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    cell = id;
    spr.x = x;
    spr.y = y;
    spr.w = size * scale;
    spr.h = 24.0f * scale;
    spr.color = color;
    if (cell == 9) {
        spr.u0 = 224;
    } else {
        spr.u0 = (cell % 10) * 24;
    }
    spr.v0 = (cell / 10) * 24;
    spr.u1 = spr.u0 + (s16)size + 1;
    if (cell == 18 || cell == 17 || cell == 19) {
        spr.v1 = spr.v0 + 24;
    } else {
        spr.v1 = spr.v0 + 25;
    }
    if (cell == 25) {
        spr.u0 = 3;
        spr.v0 = 27;
        spr.u1 = spr.u0 + 18;
        spr.v1 = spr.v0 + 18;
        spr.w = spr.h = 18.0f * scale;
        spr.x++;
        spr.y++;
    }
    switch ((s16)iconId) {
    case 22: {
        s16 swapU = spr.u1 - 1;
        spr.u1 = spr.u0;
        spr.u0 = swapU;
        Sprite_Draw(&spr);
        break;
    }
    case 23:
        spr.u1--;
        TexRect_DrawRotated((struct TexRect *)&spr, 16384);
        break;
    case 24:
        spr.u1--;
        TexRect_DrawRotated((struct TexRect *)&spr, 49152);
        break;
    default:
        Sprite_Draw(&spr);
        break;
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
}

/* Hud_DrawButtonIcon in white. */
void Hud_DrawButtonIconWhite(s32 id, s32 x, s32 y, s32 depth, f32 scale)
{
    Hud_DrawButtonIcon(id, x, y, depth, -1, scale);
}

/* 1 if byte pos of text is the first byte of a two-byte (Shift-JIS)
 * character. */
s32 Text_IsDoubleByteAt(u8 *text, u32 pos)
{
    u32 i;
    u32 end;
    u8 lead;

    end = pos;
    end++;
    i = 0;
    while (i < end) {
        lead = *text;
        if ((lead >= 128 && lead <= 159) || (lead >= 224 && lead <= 255)) {
            if (i == pos) {
                return 1;
            }
            text += 2;
            i += 2;
        } else {
            text += 1;
            i += 1;
        }
    }
    return 0;
}

/* Text_DrawWrapped in lines of at most 33 bytes. */
void Text_DrawWrapped33(s16 x, s16 y, s16 lineh, char *text)
{
    Text_DrawWrapped(x, y, lineh, text, 33);
}

s32 Str_Length(char *s);
char *strchr(char *s, s32 c);
char *strcpy(char *dst, const char *src);

/* Draws text from (x, y) in lines of at most width bytes (a double-byte
 * character is not split) or up to each '\n', at most 50 lines; returns the
 * line count. */
s32 Text_DrawWrapped(s16 x, s16 y, s16 lineh, char *text, s16 width)
{
    char buf[128];
    char *rest;
    s32 lineCount;
    s32 i;
    s32 len;
    char *newline;

    if (text == NULL) {
        return 0;
    }
    rest = text;
    lineCount = 0;
    y -= lineh;
    for (i = 0; i < 50; i++) {
        if (rest == NULL) {
            break;
        }
        lineCount++;
        len = Str_Length(rest);
        strcpy(buf, rest);
        if (width - 1 < len) {
            if (Text_IsDoubleByteAt((u8 *)buf, width - 1) == 0) {
                buf[width] = 0;
                if ((newline = strchr(buf, '\n')) != NULL) {
                    *newline = 0;
                    rest = strchr(rest, '\n') + 1;
                } else {
                    rest += width;
                }
            } else {
                buf[width - 1] = 0;
                if ((newline = strchr(buf, '\n')) != NULL) {
                    *newline = 0;
                    rest = strchr(rest, '\n') + 1;
                } else {
                    rest += width - 1;
                }
            }
            y += lineh;
            Font_SetPos(x, y);
            Text_Draw(buf);
        } else if ((newline = strchr(buf, '\n')) != NULL) {
            *newline = 0;
            rest = strchr(rest, '\n') + 1;
            y += lineh;
            Font_SetPos(x, y);
            Text_Draw(buf);
        } else {
            Font_SetPos(x, y + lineh);
            Text_Draw(buf);
            break;
        }
    }
    return lineCount;
}

/* Seconds as minutes and seconds. */
char *Time_FormatMinSec(u32 totalSeconds)
{
    u32 seconds = totalSeconds % 60;

    sprintf(gTimeStrMinSec, TXT(0x00490A10) /* hours and minutes, with their full-width unit marks */, (totalSeconds - seconds) / 60, seconds);
    return gTimeStrMinSec;
}

/* Seconds as minutes (within the hour) and seconds. */
char *Time_FormatMinSecInHour(u32 totalSeconds)
{
    u32 seconds = totalSeconds % 60;

    sprintf(gTimeStrMinSecInHour, TXT(0x00490A20) /* minutes and seconds, with their full-width unit marks */, ((totalSeconds - seconds) % 3600) / 60, seconds);
    return gTimeStrMinSecInHour;
}

/* Draws the black line of gBlackLineSprite's rectangle. */
void Gfx_DrawBlackLineFixed(void)
{
    Sprite line = gBlackLineSprite;

    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_DrawLine(line.x, line.y, line.w, line.h, 0xFF000000);
}

/* Draws a black line from x, y, width wide and height high, with the
 * icon sheet's texture bound. */
void Gfx_DrawBlackLine(s32 x, s32 y, s32 width, s32 height)
{
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_DrawLine(x, y, width, height, 0xFF000000);
}

extern u16 gFrameTick;

/*
 * A pulsing highlight bar centred on x, w wide, from y down h: two gradient halves that
 * fade from transparent at the ends to the colour at the centre, alpha 160 + 48 sin(t).
 * (The game converts x and w to float and back.)
 */
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 height, u32 color)
{
    PrimGradRect rect;
    f32 wave;
    s32 half;

    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    wave = Math_Sin(ANGLE16_TO_RAD * (u16)((gFrameTick & 0x3F) << 10));
    x = (f32)x;
    w = (f32)w;
    half = w / 2;
    rect.x0 = x - half;
    rect.y0 = y;
    rect.x1 = x;
    rect.y1 = rect.y0 + height;
    rect.color[0] = color & 0xFFFFFF;
    rect.color[1] = rect.color[0] | (((s8)(48.0f * wave) + 160) << 24);
    rect.color[2] = rect.color[0];
    rect.color[3] = rect.color[1];
    Prim_FillGradRect(&rect);
    rect.x0 = rect.x1 + half;
    Prim_FillGradRect(&rect);
    Gs_SetState(RS_ALPHA_REF, 0);
}

extern s32 gLoadLettersTex;          /* the letters texture handle */
extern s16 gLoadingLetterPos[];        /* per letter: texture u, v, then screen x, y */
extern f32 gLoadingBobWave[];        /* 12 steps of the bob */

/* Draws the loading letters: ten sprites with a shadow, each bobbing on the wave
 * gLoadingBobWave one step after the other. */
void Loading_DrawLetters(void)
{
    SysSettings *settings;
    s32 savedZWrite;
    s32 savedTexFilter;
    s32 savedBlend;
    Sprite sprite;
    s32 i;
    s32 phase;
    s16 *letterPos;
    s16 x;
    s16 y;

    settings = &gSysSettings;
    if (gSysSettings.loadAnimOn != 0) {
        if (gLoadLettersTex == 0) {
            if (Overlay_IsLoaded(OVERLAY_GAME) != 1) {
                Tex_LoadCommon();
            }
            settings->loadAnimOn = 0;
        } else {
            settings->loadAnimFrame++;
            if (settings->loadAnimFrame & 1) {
                settings->loadAnimPhase++;
            }
            if (settings->loadAnimPhase > 11) {
                settings->loadAnimPhase -= 12;
            }
            savedZWrite = Gs_GetState(RS_ZWRITE);
            savedTexFilter = Gs_GetState(RS_TEX_FILTER);
            savedBlend = Gs_GetState(RS_BLEND);
            Gs_SetState(RS_ZWRITE, 0);
            Tex_Use(1, (u32 *) &gLoadLettersTex);
            Gs_SetState(RS_TEXTURE, gLoadLettersTex);
            Gs_SetState(RS_TEX_FILTER, 0);
            Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
            sprite.w = 28;
            sprite.h = 32;
            letterPos = gLoadingLetterPos;
            phase = settings->loadAnimPhase;
            for (i = 0; i < 10; i++) {
                sprite.u0 = *letterPos++;
                sprite.v0 = *letterPos++;
                sprite.u1 = sprite.u0 + 31;
                sprite.v1 = sprite.v0 + 31;
                x = *letterPos++;
                y = (f32)*letterPos++ + 4.0f * gLoadingBobWave[phase];
                sprite.color = 0xFF000000;
                sprite.x = x + 2;
                sprite.y = y + 2;
                sprite.w = 28;
                sprite.h = 32;
                Sprite_Draw(&sprite);
                sprite.color = -1;
                sprite.x = x;
                sprite.y = y;
                sprite.w = 28;
                sprite.h = 32;
                Sprite_Draw(&sprite);
                if (i == 2) {
                    phase += 2;
                } else {
                    phase += 1;
                }
                if (phase > 11) {
                    phase -= 12;
                }
            }
            Gs_SetState(RS_ZWRITE, savedZWrite);
            Gs_SetState(RS_TEX_FILTER, savedTexFilter);
            Gs_SetState(RS_BLEND, savedBlend);
            settings->loadAnimOn = 0;
        }
    }
}

/* Asks for the loading letters on the next frame. */
void Loading_Request(void)
{
    gSysSettings.loadAnimOn = 1;
}

/* This file's initialised data. */
Sprite gBlackLineSprite = {
#include "sub_main/0043C990.inc"
};
f32 gLoadingBobWave[12] = {
#include "sub_main/0043C9B0.inc"
};
s16 gLoadingLetterPos[40] = {
#include "sub_main/0043C9E0.inc"
};
