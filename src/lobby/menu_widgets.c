/*
 * The lobby's menu drawing kit: the small pieces the lobby's menus and
 * windows are drawn with.
 *
 * - Icon_Draw and its turned and blended forms draw an entry of the
 *   lobby's icon sheet (gIconSprites: a texture handle and two corners on
 *   it) as a textured rectangle; Icon_DrawEquip draws an equipment icon
 *   (gEquipIconUVs, stretched a quarter wider) and Icon_DrawPreset one of
 *   three icons placed in advance (gIconPresets).
 * - The pieces built from icons: the money and points boxes with their
 *   dashed underline, dashed lines, the row bars with their tab and the
 *   bobbing cursor, the side arrows, the framed box, the mail icon with
 *   its sparkle and the area list's marks.
 * - Text helpers: a number as decimal text (Str_FromInt), centring, a
 *   shadowed string, the page number with its arrows (page/count).
 * - The pulsing colours of cursors and highlights (Color_Pulse,
 *   Color_PulseAlpha), and Win_Draw, a window frame in one of nine styles.
 *
 * Its data: Str_FromInt's format (gIntFmt, .rodata), gEquipIconUVs, gIconSprites (ICON_SPRITE_COUNT entries, the
 * village clock's sectors from ICON_CLOCK_SECTOR on) and gIconPresets.
 */
#include "common.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "render_state.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "window.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "text.h"
#include "menu_widgets.h"
#include "engine_math.h"
#include "font.h"
#include "menu_sheets.h"
#include "name_tags.h"

struct Sprite;

s32 sprintf(char *buf, char *fmt, ...);
extern s32 gIntFmt;

/* Writes n into buf as decimal text (sprintf with gIntFmt, "%d"). The
 * lobby's text fills (Text_FillDollar, Text_FillDollar2) put it in place of
 * a template's '$'. */
void Str_FromInt(s32 n, char *buf)
{
    sprintf(buf, (char *)&gIntFmt, n);
}

/* The format of Str_FromInt, the first of the file's .rodata, kept as an
 * s32 (its four bytes) as the unit defines it. */
#pragma rodata on
s32 gIntFmt = {
#include "lobby/0083B7F8.inc"
};
#pragma rodata off

/* The x at which text starts when it is centred on x, its characters
 * charW wide: Text_DisplayLength gives its length in half-width
 * characters. */
s16 Text_CenterX(char *text, s16 x, s16 charW)
{
    s32 len = Text_DisplayLength(text);

    return x - charW * len / 4;
}

/* Draws the money held (gSave.money) at (x, y): the coin mark, then the
 * amount with a z, over a dashed underline of six bar segments, each with
 * its shadow. With dim set, the mark is darker and the amount in text
 * colour 10. The loop counter i then holds the text colour. */
void Menu_DrawMoney(s16 x, s16 y, s8 dim)
{
    char buf[32];
    s16 segX = x;
    s16 lineY = y + 18;
    s16 i = 0;
    u32 color;

    do {
        Icon_Draw(segX + 3, lineY + 2, 24, 10, 0xFF000000, ICON_BAR_SEGMENT);
        Icon_Draw(segX, lineY, 24, 10, 0xFF806000, ICON_BAR_SEGMENT);
        segX += 24;
        i++;
    } while (i < 6);
    if (dim == 0) {
        i = 0;
        color = 0xFFE0E000;
    } else {
        color = 0xFF808000;
        i = 10;
    }
    Icon_Draw(x, y, 24, 24, color, ICON_MONEY_MARK);
    Font_SetSpacing(28, 20);
    sprintf(buf, "%7dz", gSave.money);
    Text_DrawShadow(x + 26, y + 4, 1, (s32)i, buf);
}

/* Draws the points held (gSave.points) at (x, y) as Menu_DrawMoney draws
 * the money, with a P. */
void Menu_DrawPoints(s16 x, s16 y)
{
    char buf[32];
    s16 segX = x;
    s16 lineY = y + 18;
    s16 i = 0;

    do {
        Icon_Draw(segX + 3, lineY + 2, 24, 10, 0xFF000000, ICON_BAR_SEGMENT);
        Icon_Draw(segX, lineY, 24, 10, 0xFF806000, ICON_BAR_SEGMENT);
        segX += 24;
        i++;
    } while (i < 6);
    Icon_Draw(x, y, 24, 24, 0xFFE0E000, ICON_MONEY_MARK);
    Font_SetColor(0x807F7F7F);
    Font_SetSpacing(28, 20);
    sprintf(buf, "%7dP", gSave.points);
    Text_DrawShadow(x + 26, y + 4, 1, 0, buf);
}

/* Draws a dashed line at y from x to x + w: as many bar segments (20
 * wide, 30 at most) as fit. */
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color)
{
    s16 i;
    s32 end = x + w;

    for (i = 0; i < 30; i++) {
        if (x + 20 > end) {
            break;
        }
        Icon_Draw(x, y, 20, 8, color, ICON_BAR_SEGMENT);
        x += 20;
    }
}

/* Draws text at (x, y) in colour color over its shadow, 3 right and 2
 * down, in colour shadow. */
void Text_DrawShadowed(s16 x, s16 y, s32 shadow, s32 color, char *text)
{
    Font_SetColor(shadow);
    Font_SetPos(x + 3, y + 2);
    Text_PrintfColored(text);
    Font_SetColor(color);
    Font_SetPos(x, y);
    Text_PrintfColored(text);
}

/* Draws a bar at (x, y), w by height, turned by angle about (pivotX,
 * pivotY): a cap 16 wide at each end and the middle between them, icons
 * k + ICON_BAR_CAP_LEFT, k + ICON_BAR_MIDDLE and k + ICON_BAR_CAP_RIGHT;
 * for k above 1 the two caps' icons swap. */
void Menu_DrawBarRotated(s16 x, s16 y, s16 w, s16 height, u32 color, s32 angle, s16 pivotX, s16 pivotY, u8 k)
{
    if (k <= 1) {
        Icon_DrawRotatedAt(x, y, 16, height, color, angle, k + ICON_BAR_CAP_LEFT, pivotX, pivotY);
        Icon_DrawRotatedAt(x + 16, y, w - 32, height, color, angle, k + ICON_BAR_MIDDLE, pivotX, pivotY);
        Icon_DrawRotatedAt(x + w - 16, y, 16, height, color, angle, k + ICON_BAR_CAP_RIGHT, pivotX, pivotY);
    } else {
        Icon_DrawRotatedAt(x, y, 16, height, color, angle, k + ICON_BAR_CAP_RIGHT, pivotX, pivotY);
        Icon_DrawRotatedAt(x + 16, y, w - 32, height, color, angle, k + ICON_BAR_MIDDLE, pivotX, pivotY);
        Icon_DrawRotatedAt(x + w - 16, y, 16, height, color, angle, k + ICON_BAR_CAP_LEFT, pivotX, pivotY);
    }
}

/* Draws a row bar (Menu_DrawRowBar: w wide, in color, sel and kind as
 * there) with label, when given, on it in dark grey. */
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 sel, u8 kind)
{
    Menu_DrawRowBar(x, y, w, color, sel, kind);
    if (label != NULL) {
        Text_DrawShadowRgb(x + 18, y, 0xFF202020, -1, label);
    }
}

/* The icon of a row bar's kind (ICON_ROW_BAR_KINDS kinds from ICON_ROW_BAR_FIRST;
 * ICON_CURSOR_BAR is kind 1's). Menu_DrawRowBar calls it inline: written
 * there directly, the kind's remainder would be kept in another register
 * than the game's code keeps it in. */
static inline s32 Icon_KindSprite(u8 kind)
{
    return kind % ICON_ROW_BAR_KINDS + ICON_ROW_BAR_FIRST;
}

/* Draws a row's bar at (x, y + 18), w wide, in color with its shadow: the
 * icon of its kind (Icon_KindSprite), 10 high when sel is nonzero, else 8.
 * When sel is 1, the cursor (Menu_DrawCursorIcon) bobs to its left every
 * 16 frames. */
void Menu_DrawRowBar(s16 x, s16 y, s16 w, u32 color, u8 sel, u8 kind)
{
    s32 selected = sel;
    s32 sprite;
    s16 barH;
    s16 bobX;

    barH = selected ? (s16)10 : (s16)8;
    sprite = Icon_KindSprite(kind);
    Icon_Draw(x + 4, y + 20, w, barH, 0xFF202020, sprite);
    Icon_Draw(x, y + 18, w, barH, color, sprite);
    selected = sel;
    if (selected == 1) {
        bobX = 0;
        if (gFrameTick & 0x10) {
            bobX = 6;
        }
        Menu_DrawCursorIcon(x + bobX - 18, y, 24, color);
    }
}

/* Prints the page number at (x, y) as page + 1, the separator, and
 * pageCount (two places for the page from 10 pages on), with the arrows
 * either side (Menu_DrawArrows) when there is more than one page; color 0
 * gives the arrows the pulsing colour (Color_Pulse). */
void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 pageCount, u32 color)
{
    char buf[32];

    if (pageCount < 10) {
        sprintf(buf, "%d%s%d", page + 1, gLobbyMarks[LOBBY_MARK_SLASH], pageCount);
    } else {
        sprintf(buf, "%2d%s%d", page + 1, gLobbyMarks[LOBBY_MARK_SLASH], pageCount);
    }
    Font_SetSpacing(18, 18);
    Font_SetPos(x, y);
    Text_PrintfColored(gLobbyStrFmt, buf);
    if (color == 0) {
        color = Color_Pulse();
    }
    if (pageCount > 1) {
        Menu_DrawArrows(x, y, 18, buf, color);
    }
}

/* Prints the page number as Menu_DrawPageArrows does, shadowed, in
 * full-width characters (Sjis_Widen) unless narrow is set, with blue arrows
 * either side when there is more than one page. The font's colour is put
 * back after; Color_Pulse's result is not used. */
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 pageCount, u8 narrow)
{
    char buf[32];
    char wide[32];
    u32 savedColor;

    Color_Pulse();
    savedColor = Font_GetColor();
    if (pageCount < 10) {
        sprintf(buf, "%d%s%d", page + 1, gLobbyMarks[LOBBY_MARK_SLASH], pageCount);
    } else {
        sprintf(buf, "%2d%s%d", page + 1, gLobbyMarks[LOBBY_MARK_SLASH], pageCount);
    }
    Sjis_Widen(buf, wide);
    Font_SetSpacing(18, 18);
    Font_SetPos(x, y);
    if (narrow == 0) {
        Text_DrawShadowRgb(x, y, 0x80000000, 0x807F7F7F, wide);
    } else {
        Text_DrawShadowRgb(x, y, 0x80000000, 0x807F7F7F, buf);
    }
    if (pageCount > 1) {
        if (narrow == 0) {
            Menu_DrawArrowsSized(x, y, 18, 18, wide, 0xFF8484FF);
        } else {
            Menu_DrawArrowsSized(x, y, 18, 18, buf, 0xFF8484FF);
        }
    }
    Font_SetColor(savedColor);
}

/* Draws the cursor icon (ICON_CURSOR) at (x, y), size by size, in color. */
void Menu_DrawCursorIcon(s16 x, s16 y, s16 size, u32 color)
{
    Icon_Draw(x, y, size, size, color, ICON_CURSOR);
}

/* Draws a list's cursor line at (x, y): the bar under it, w wide, in color,
 * and the cursor in cursorColor, gap left of x, bobbing every 16 frames. */
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 gap, u32 color, u32 cursorColor)
{
    s16 bobX = (gFrameTick & 0x10) ? (s16)4 : (s16)10;

    Icon_Draw(x, y + 14, w, 8, color, ICON_CURSOR_BAR);
    Menu_DrawCursorIcon(x - gap - bobX, y - 2, 24, cursorColor);
}

/* Draws the arrows either side of an item at (x, y), w wide: size by size,
 * in color, bobbing outwards every 16 frames. flip swaps the two icons
 * (ICON_CURSOR and ICON_CURSOR_MIRROR). */
void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color)
{
    s16 bobX = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
    s32 leftIcon;
    s32 rightIcon;

    if (flip == 0) {
        leftIcon = ICON_CURSOR;
        rightIcon = ICON_CURSOR_MIRROR;
    } else {
        leftIcon = ICON_CURSOR_MIRROR;
        rightIcon = ICON_CURSOR;
    }
    Icon_Draw(x - size - bobX, y, size, size, color, leftIcon);
    Icon_Draw(x + w + bobX, y, size, size, color, rightIcon);
}

/* A green whose alpha pulses with the frame counter: a sine wave over 64
 * frames, 159 give or take 80. */
u32 Color_Pulse(void)
{
    u16 phase = (gFrameTick & 0x3F) << 10;
    s8 wave = 80.0f * Math_Sin(9.5873801e-05f * phase);

    return ((wave + 159) << 24) | 0x8020;
}

/* color with its alpha pulsing over 64 frames, 160 give or take 48. */
u32 Color_PulseAlpha(u32 color)
{
    u16 phase = (gFrameTick & 0x3F) << 10;

    color = (color & 0xFFFFFF) | (((s8)(48.0f * Math_Sin(0.0000958738f * phase)) + 160) << 24);
    return color;
}

/* The pulsing row highlight's colour, below the alpha Color_PulseAlpha
 * sets: red and green half up. */
#define ROW_HIGHLIGHT_RGB 0x8080

/* Fills a bar at (x, y), w by height: a list row's highlight, pulsing
 * (Color_PulseAlpha) unless still is set, then a fixed grey. */
void Menu_DrawRowHighlight(s16 x, s16 y, s16 w, s16 height, s8 still)
{
    u32 color;

    if (still == 0) {
        color = Color_PulseAlpha(ROW_HIGHLIGHT_RGB);
    } else {
        color = 0x80808080;
    }
    Gfx_DrawLine(x, y, w, height, color);
}

/* Draws kind marks (ICON_LEVEL_MARK) centred on (x, y), size high: 0 none, 1
 * one, 2 two side by side, 3 three in a triangle. */
void LobbyUi_DrawLevelMarks(s16 x, s16 y, s16 size, u32 color, s8 kind)
{
    s16 markW;

    switch (kind) {
    case 0:
        break;
    case 1:
        markW = 0.8f * size;
        Icon_Draw(x - (markW >> 1), y - (size >> 1), markW, size, color, ICON_LEVEL_MARK);
        break;
    case 2:
        markW = size >> 1;
        Icon_Draw(x - markW, y - (size >> 1), markW, size, color, ICON_LEVEL_MARK);
        Icon_Draw(x, y - (size >> 1), markW, size, color, ICON_LEVEL_MARK);
        break;
    case 3:
        markW = size >> 1;
        Icon_Draw(x - markW / 2, y - markW, markW, markW, color, ICON_LEVEL_MARK);
        Icon_Draw(x - markW, y, markW, markW, color, ICON_LEVEL_MARK);
        Icon_Draw(x, y, markW, markW, color, ICON_LEVEL_MARK);
        break;
    }
}

/* Draws a framed box at (x, y), w by height, in color: the top, bottom, left
 * and right edge icons (ICON_FRAME_TOP to ICON_FRAME_RIGHT), 8 thick, and the inside filled. */
void Menu_DrawFrameBox(s16 x, s16 y, s16 w, s16 height, u32 color)
{
    Icon_Draw(x, y, w, 8, color, ICON_FRAME_TOP);
    Icon_Draw(x, y + height - 8, w, 8, color, ICON_FRAME_BOTTOM);
    Icon_Draw(x, y + 8, 8, height - 16, color, ICON_FRAME_LEFT);
    Icon_Draw(x + w - 8, y + 8, 8, height - 16, color, ICON_FRAME_RIGHT);
    Gfx_DrawLine(x + 8, y + 8, w - 10, height - 16, color);
}

extern IconUV gEquipIconUVs[];

/* Draws equipment icon i (its corners in gEquipIconUVs, on texture slot 324)
 * at (x, y), height high and a quarter wider than w. */
void Icon_DrawEquip(s32 x, s32 y, s32 w, s32 height, s32 color, s32 i)
{
    TexRect rect;
    IconUV *uvs = &gEquipIconUVs[i];

    rect.y = y;
    rect.h = height;
    rect.x = x;
    rect.color = color;
    rect.w = 1.25f * w;
    rect.uv0 = uvs->uv0;
    rect.uv1 = uvs->uv1;
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    Sprite_Draw((struct Sprite *)&rect);
}

struct Frame;

/* Draws a window frame (WindowFrame_Draw) at (x, y), w by height, in color and
 * color2, in style kind, 0 to 8, each a Frame.style value. */
void Win_Draw(s32 x, s32 y, s32 w, s32 height, s32 color, s32 color2, s16 kind)
{
    Frame frame;

    frame.x = x;
    frame.y = y;
    frame.w = w;
    frame.h = height;
    frame.color = color;
    frame.color2 = color2;
    switch (kind) {
    case 0:
        frame.style = FRAME_FILL | FRAME_INSET | 4;
        break;
    case 1:
        frame.style = 5;
        break;
    case 2:
        frame.style = FRAME_INSET;
        break;
    case 3:
        frame.style = FRAME_FILL | FRAME_INSET | 1;
        break;
    case 7:
        frame.style = 0;
        break;
    case 4:
        frame.style = FRAME_INSET | 1;
        break;
    case 5:
        frame.style = FRAME_FILL | FRAME_INSET | 2;
        break;
    case 6:
        frame.style = FRAME_FILL | 1;
        break;
    case 8:
        frame.style = FRAME_INSET;
        break;
    }
    WindowFrame_Draw(&frame, (s16 *)(0));
}

extern s16 gMailIconBlink;
extern MailEntry gMailBox[MAIL_BOX_LEN];

/* While any mail of gMailBox is in use, draws the mail icon (ICON_MAIL) at
 * (x, y) and a sparkle (ICON_MAIL_SPARKLE) that fades in and out over 8 frames each way
 * and moves round its four corners (gMailIconBlink counts the frames). */
void Menu_DrawMailIcon(s16 x, s16 y)
{
    s8 i;
    MailEntry *mail;
    f32 alpha;

    for (i = 0, mail = gMailBox; i < MAIL_BOX_LEN; i++, mail++) {
        if (mail->unread != 0) {
            if (++gMailIconBlink & 8) {
                alpha = 255.0f - 36.42857f * (gMailIconBlink & 7);
            } else {
                alpha = 36.42857f * (gMailIconBlink & 7);
            }
            Icon_Draw(x, y, 24, 20, 0xFFFFFFFF, ICON_MAIL);
            if (gMailIconBlink & 0x10) {
                if (gMailIconBlink & 0x80) {
                    Icon_Draw(x - 7, y - 5, 14, 14, ((u32)alpha << 24) | 0xFFFFFF, ICON_MAIL_SPARKLE);
                    return;
                }
                Icon_Draw(x - 7, y + 12, 14, 14, ((u32)alpha << 24) | 0xFFFFFF, ICON_MAIL_SPARKLE);
                return;
            }
            if (gMailIconBlink & 0x80) {
                Icon_Draw(x + 14, y + 12, 14, 14, ((u32)alpha << 24) | 0xFFFFFF, ICON_MAIL_SPARKLE);
                return;
            }
            Icon_Draw(x + 14, y - 5, 14, 14, ((u32)alpha << 24) | 0xFFFFFF, ICON_MAIL_SPARKLE);
            return;
        }
    }
}

/* Draws icon i of the lobby's icon sheet (gIconSprites) at (x, y), w by height,
 * tinted by color (0xAABBGGRR): its texture bound, the alpha test's
 * reference at 0. */
void Icon_Draw(s16 x, s16 y, s16 w, s16 height, u32 color, s32 i)
{
    TexRect rect;
    IconSprite *sprite = &gIconSprites[i];

    rect.y = y;
    rect.w = w;
    rect.x = x;
    rect.h = height;
    rect.color = color;
    rect.uv0 = sprite->uv0;
    rect.uv1 = sprite->uv1;
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Tex_Prepare(1, sprite->tex);
    Tex_Bind(sprite->tex);
    Sprite_Draw((struct Sprite *)&rect);
}

/* Draws icon i as Icon_Draw does, turned by angle (TexRect_DrawRotated). */
void Icon_DrawRotated(s32 x, s32 y, s32 w, s32 height, s32 color, s32 angle, s32 i)
{
    TexRect rect;
    IconSprite *sprite = &gIconSprites[i];

    rect.y = y;
    rect.w = w;
    rect.x = x;
    rect.h = height;
    rect.color = color;
    rect.uv0 = sprite->uv0;
    rect.uv1 = sprite->uv1;
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Tex_Prepare(1, sprite->tex);
    Tex_Bind(sprite->tex);
    TexRect_DrawRotated(&rect, angle);
}

/* Draws icon i as Icon_Draw does, turned by angle about (pivotX, pivotY). */
void Icon_DrawRotatedAt(s16 x, s16 y, s16 w, s16 height, u32 color, s32 angle, s32 i, s16 pivotX, s16 pivotY)
{
    TexRect rect;
    IconSprite *sprite = &gIconSprites[i];

    rect.y = y;
    rect.w = w;
    rect.x = x;
    rect.h = height;
    rect.color = color;
    rect.uv0 = sprite->uv0;
    rect.uv1 = sprite->uv1;
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Tex_Prepare(1, sprite->tex);
    Tex_Bind(sprite->tex);
    TexRect_DrawRotatedAt(&rect, pivotX, pivotY, angle);
}

/* Draws icon i as Icon_Draw does, through TexRect_Draw in mode mode. */
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 height, u32 color, s32 i, u8 mode)
{
    TexRect rect;
    IconSprite *sprite = &gIconSprites[i];

    rect.y = y;
    rect.w = w;
    rect.x = x;
    rect.h = height;
    rect.color = color;
    rect.uv0 = sprite->uv0;
    rect.uv1 = sprite->uv1;
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    Tex_Prepare(1, sprite->tex);
    Tex_Bind(sprite->tex);
    TexRect_Draw(&rect, mode);
}

/* An icon placed in advance: gIconSprites' entry, its place, size and
 * colour (Icon_DrawPreset). */
typedef struct IconPreset {
    /* 0x0 */ s32 sprite;
    /* 0x4 */ s16 x;
    /* 0x6 */ s16 y;
    /* 0x8 */ s16 w;
    /* 0xA */ s16 h;
    /* 0xC */ u32 color;
} IconPreset;
extern IconPreset gIconPresets[];

/* Draws preset icon i of gIconPresets: its icon at its place and size, in
 * its colour (the village clock's two, and one of the leave menus). */
void Icon_DrawPreset(s32 i)
{
    IconPreset *preset = &gIconPresets[i];

    Icon_Draw(preset->x, preset->y, preset->w, preset->h, preset->color, preset->sprite);
}

/* The file's data: the equipment icons' corners, the icon sheet (the
 * village clock's entries are ICON_CLOCK_FACE to ICON_CLOCK_RING) and the
 * three preset icons. */
IconUV gEquipIconUVs[20] = {
#include "lobby/007FEAF0.inc"
};
IconSprite gIconSprites[ICON_SPRITE_COUNT] = {
#include "lobby/007FEB90.inc"
};
IconPreset gIconPresets[3] = {
#include "lobby/007FF250.inc"
};
