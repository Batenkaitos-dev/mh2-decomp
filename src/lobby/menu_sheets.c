/*
 * Drawing helpers that the lobby's menus share: the quest counters, the shops'
 * lists and the records draw with them.
 * - The paper sheets (Menu_DrawQuestSheet): a sheet turned per page by
 *   gStripTurnOffsets (SheetTurn: each page's offsets, pivot and angle), with its
 *   tabs (Menu_DrawLabelsPos/Neg, placed by gLabelIconOffsets), its frame strip
 *   (Menu_DrawFrameStrip) and its emblem.
 * - The decorated frames (Menu_DrawDecoFrame, Menu_DrawDecoBox), the quest's stamp
 *   (Menu_DrawStamp, QuestBoard_DrawStamp) and the bobbing arrows beside a text
 *   (Menu_DrawArrows*).
 * The .data holds gLabelIconOffsets (the tabs' places).
 */
#include "common.h"
#include "sprite.h"

#include "lobby_menu.h"
#include "lobby_globals.h"
#include "model.h"
#include "text.h"
#include "menu_sheets.h"
#include "name_tags.h"
#include "quest_desk_draw.h"
#include "libc.h"
#include "sprite_queue.h"

void Icon_DrawRotatedAt(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s32 i, s16 cx, s16 cy);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, s32 color, s32 spr); /* unit's own declaration: an s32 colour where the function takes a u32; the three arrow helpers pass their s32 colour through and only build as the game's code does with this declaration */
s32 Text_DisplayLength(char *s);

/* A tab's place on the quest sheet (gLabelIconOffsets): its x offset and its width. */
typedef struct TabPlace {
    s16 x;
    s16 w;
} TabPlace;
extern TabPlace gLabelIconOffsets[];

/* Draws the tabs down the side of a sheet whose left is x and top is y. tabs[] holds up to 12
 * rows, 0 ending it; each row above 0 is drawn as tab sprite 15 + row, 28 pixels high, one
 * under the other from y + 20, at gLabelIconOffsets' x offset and width for its row, in
 * colour gLabelIconsPos[value] (99 takes entry 0), turned by angle about (pivotX, pivotY). */
void Menu_DrawLabelsPos(s16 x, s16 y, s32 angle, s16 pivotX, s16 pivotY, s16 *tabs)
{
    s16 i;

    y += 20;
    for (i = 0; i < 12; i++) {
        if (tabs[i] == 0) {
            break;
        }
        if (tabs[i] > 0) {
            Icon_DrawRotatedAt(x + gLabelIconOffsets[i].x, y, gLabelIconOffsets[i].w, 28, (tabs[i] == 99) ? gLabelIconsPos[0] : gLabelIconsPos[tabs[i]], angle, i % 12 + 15, pivotX, pivotY);
        }
        y += 28;
    }
}

/* As Menu_DrawLabelsPos for the rows below 0: row value -n takes colour gLabelIconsNeg[n]
 * (-99 takes entry 0). A sheet draws these on top of itself, turned with it. */
void Menu_DrawLabelsNeg(s16 x, s16 y, s32 angle, s16 pivotX, s16 pivotY, s16 *tabs)
{
    s16 i;

    y += 20;
    for (i = 0; i < 12; i++) {
        if (tabs[i] == 0) {
            break;
        }
        if (tabs[i] < 0) {
            Icon_DrawRotatedAt(x + gLabelIconOffsets[i].x, y, gLabelIconOffsets[i].w, 28, (tabs[i] * -1 == 99) ? gLabelIconsNeg[0] : gLabelIconsNeg[tabs[i] * -1], angle, i % 12 + 15, pivotX, pivotY);
        }
        y += 28;
    }
}

extern s32 *gDecoFrameColors[];

/* Draws a decorated frame at (left, top), width wide and height high: the frame in
 * frameColor (sprites 7 the left edge, 3 the middle, 11 the right edge), the mark at its
 * centre in markColor, and two bands (sprite 27 and its end 28) 120 pixels above and
 * below the centre. Kind 3 takes both colours from gDecoFrameColors[emblem] and draws the two
 * halves of emblem `emblem` (sprites 122 + 2 * emblem and the next) as the mark; kinds 2, 4
 * and 5 have fixed colours and draw the round mark (sprite 31). Nothing is turned. */
void Menu_DrawDecoFrame(s16 left, s16 top, s16 width, s16 height, s8 kind, s8 emblem)
{
    s32 w;
    s32 x;
    s32 markColor;
    s32 innerX;
    s16 centerX;
    s32 emblemSprite;
    s16 centerY;
    s32 angle;
    s32 frameColor;
    s32 midY;
    s32 emblemY;
    s32 y;
    s32 *colors;
    s32 bandTopY;
    s32 bandBottomY;

    w = width;
    x = left;
    centerX = x + w / 2;
    y = top;
    centerY = y + height / 2;
    switch (kind) {
    case 2:
        frameColor = 0xFF445C79;
        angle = 0;
        markColor = 0xFFB57510;
        break;
    case 3:
        angle = 0;
        colors = gDecoFrameColors[emblem];
        frameColor = colors[0];
        markColor = colors[1];
        break;
    case 5:
    case 4:
        frameColor = 0xFF7B400E;
        angle = 0;
        markColor = 0x80B57510;
        break;
    }
    innerX = (s16)(x + 10);
    Icon_DrawRotatedAt(innerX + 16, y + 3, w - 32, height, frameColor, angle, 3, centerX, centerY);
    Icon_DrawRotatedAt(x + 10, y + 3, 16, height, frameColor, angle, 7, centerX, centerY);
    Icon_DrawRotatedAt(innerX + w - 16, y + 3, 16, height, frameColor, angle, 11, centerX, centerY);
    if (kind == 3) {
        midY = centerY;
        emblemY = midY - 48;
        emblemSprite = emblem * 2;
        Icon_DrawRotatedAt(centerX - 56, emblemY, 56, 112, markColor, angle, emblemSprite + 122, centerX, centerY);
        Icon_DrawRotatedAt(centerX, emblemY, 56, 112, markColor, angle, emblemSprite + 123, centerX, centerY);
    } else {
        midY = centerY;
        Icon_DrawRotatedAt(centerX - 56, midY - 56, 112, 112, markColor, angle, 31, centerX, centerY);
    }
    bandTopY = midY - 120;
    Icon_DrawRotatedAt(x + 20, bandTopY, 0x100, 32, markColor, angle, 27, centerX, centerY);
    bandBottomY = midY + 120;
    Icon_DrawRotatedAt(x + 20, bandBottomY, 0x100, 32, markColor, angle, 27, centerX, centerY);
    Icon_DrawRotatedAt(x + 0x114, bandTopY, 46, 32, markColor, angle, 28, centerX, centerY);
    Icon_DrawRotatedAt(x + 0x114, bandBottomY, 46, 32, markColor, angle, 28, centerX, centerY);
}

/* Draws a grey box at (x, y), w by height, with gQuestMapGroups' tabs beside it
 * (Menu_DrawLabelsPos). Every case of the switch on kind sets the angle to 0: the test
 * stays as the game's code has it. */
void Menu_DrawDecoBox(s16 x, s16 y, s16 w, s16 height, s8 kind)
{
    s16 centerX = x + w / 2;
    s16 centerY = y + height / 2;
    s32 angle;
    s16 innerX;

    Icon_Draw(x, y, w, height, 0xFFA0A0A0, 0);
    switch (kind) {
    case 2:
        angle = 0;
        break;
    case 3:
        angle = 0;
        break;
    case 5:
    case 4:
        angle = 0;
        break;
    }
    innerX = x + 10;
    Menu_DrawLabelsPos(innerX - 50, y + 3, angle, centerX, centerY, gQuestMapGroups);
}

typedef struct IconTex {
    s32 tex;
    UV uv0;
    UV uv1;
} IconTex;
extern IconTex gIconSprites[];
extern s32 gFrameStripColors[];
extern SheetTurn gStripTurnOffsets[];

#pragma optimization_level 4
/*
 * Draws one sheet of a menu's pile of papers: a grey shadow box at (x, y), w by height, then
 * the sheet (sprite 0 or 1 by the page's parity) moved and turned by page `page`'s entry of
 * the sheet tables (gStripTurnOffsets: the x and y offsets, the pivot's offsets from the
 * centre, the angle). Then its frame strip of kind `kind` (Menu_DrawFrameStrip; -1 none), its
 * tabs (tabs, NULL none: the rows above 0 unturned under the sheet, those below 0 on it,
 * turned with it) and its emblem: -2 none, -1 the round mark (sprite 31), otherwise the two
 * halves of emblem `emblem`.
 * Built at optimization level 4 (the pragma pair). Forms kept for the match:
 * - x + w / 2 is written out at each use: with a local for w / 2 the compiler keeps a widened
 *   copy of x on the stack.
 * - (y + height / 2) - 48 is written out at both emblem calls: a local for it changes where
 *   the compiler widens height.
 * - k = emblem; k *= 2 takes two statements: emblem * 2 in one swaps two registers.
 * FAKEMATCH: the emblem colour col is set twice, the second time in the tabs block. With two
 * assignments the compiler keeps col as a value of its own and loads the constant again just
 * before each emblem call, where it goes second in the call's set-up as in the game; with the
 * constant written at both calls (or col set once, which the compiler folds in) it goes last.
 * The round-mark call takes the constant itself: col there moves its load too
 * (docs/notes/nm/Menu_DrawQuestSheet.md).
 */
void Menu_DrawQuestSheet(s16 x, s16 y, s16 w, s16 height, s16 *tabs, s16 page, s8 kind, s32 emblem)
{
    s32 centerX;
    s32 centerY;
    s16 pivotX;
    s16 pivotY;
    u16 angle;
    s32 emblemSprite;
    s16 backPage;
    u32 col;

    Icon_Draw(x, y, w, height, 0xFFA0A0A0, 0);
    backPage = (page + 2) % 12;
    col = 0x40000000;
    if (tabs != NULL) {
        col = 0x40000000; /* FAKEMATCH: set twice (see above) */
        Menu_DrawLabelsPos(x - 64, y, 0, gStripTurnOffsets[backPage].dx, gStripTurnOffsets[backPage].dy, tabs);
    }
    x += gStripTurnOffsets[page].dx;
    y += gStripTurnOffsets[page].dy;
    centerX = x + w / 2;
    pivotX = centerX + gStripTurnOffsets[page].pivotDx;
    centerY = y + height / 2;
    pivotY = centerY + gStripTurnOffsets[page].pivotDy;
    angle = gStripTurnOffsets[page].angle;
    Icon_DrawRotatedAt(x, y, w, height, 0xFFE0E0E0, angle, 1 - (page & 1), pivotX, pivotY);
    switch (kind) {
    default:
        Menu_DrawFrameStrip(x, y, w, height, kind, page);
        break;
    case -1:
        break;
    }
    if (tabs != NULL) {
        Menu_DrawLabelsNeg(x - 64, y, angle, pivotX, pivotY, tabs);
    }
    if (emblem != -2) {
        if (emblem != -1) {
            emblemSprite = emblem;
            emblemSprite *= 2;
            Icon_DrawRotatedAt((x + w / 2) - 56, (y + height / 2) - 48, 56, 112, col, angle, emblemSprite + 122, pivotX, pivotY);
            Icon_DrawRotatedAt(x + w / 2, (y + height / 2) - 48, 56, 112, col, angle, emblemSprite + 123, pivotX, pivotY);
        } else {
            Icon_DrawRotatedAt(centerX - 56, centerY - 56, 112, 112, 0x40000000, angle, 31, pivotX, pivotY);
        }
    }
}
#pragma optimization_level reset

/* Draws a sheet's frame strip: tiles of sprite 100 + kind, 24 pixels square, in two rows
 * (near the sheet's top and height - 57 below it), turned with page `page`'s sheet. 11
 * tiles; kind 1 draws 9 tiles 30 wide, kind 2 alternates short and full tiles, kinds 0 and
 * 3/5 mirror the tile across u or v. Kinds below 7 take gFrameStripColors' colour. */
void Menu_DrawFrameStrip(s16 x, s16 y, s16 w, s16 height, u8 kind, s8 page)
{
    s16 tileX;
    s16 tileY;
    s16 pivotX = x + w / 2;
    s16 pivotY = y + height / 2;
    s16 rowY;
    TexRect tile;
    s16 i;
    IconTex *iconTex;
    u32 angle;
    s16 swap;

    iconTex = &gIconSprites[kind + 100];
    if (kind < 7) {
        tile.color = gFrameStripColors[kind];
    }
    tile.x = x + 26;
    tile.y = y + 40;
    rowY = tile.y;
    tile.h = 24;
    tile.w = 24;
    tile.uv0 = iconTex->uv0;
    tile.uv1 = iconTex->uv1;
    tile.x += gStripTurnOffsets[page].dx;
    tile.y += gStripTurnOffsets[page].dy;
    pivotX += gStripTurnOffsets[page].pivotDx;
    pivotY += gStripTurnOffsets[page].pivotDy;
    angle = (u16)gStripTurnOffsets[page].angle;
    Tex_Prepare(1, iconTex->tex);
    Tex_Bind(iconTex->tex);
    for (i = 0; i < 11; i++) {
        tile.y = rowY;
        switch (kind) {
        case 0:
            swap = tile.uv0.u;
            tile.uv0.u = tile.uv1.u;
            tile.uv1.u = swap;
            break;
        case 1:
            tile.w = 30;
            if (i > 8) {
                return;
            }
            break;
        case 2:
            if (i & 1) {
                tile.h = 24;
            } else {
                tile.y = rowY + 8;
                tile.h = 16;
            }
            break;
        case 3:
        case 5:
            swap = tile.uv0.v;
            tile.uv0.v = tile.uv1.v;
            tile.uv1.v = swap;
            break;
        }
        tileX = tile.x + tile.w / 2;
        tileY = tile.y + tile.h / 2;
        TexRect_DrawRotated2(&tile, tileX, tileY, pivotX, pivotY, 0, angle);
        tile.y = tile.y + (height - 57);
        TexRect_DrawRotated2(&tile, tileX, tileY, pivotX, pivotY, 0, angle);
        tile.x += tile.w;
    }
}

void Icon_DrawRotated(s16 x, s16 y, s16 w, s16 h, u32 colour, u16 angle, s32 spr); /* unit's own declaration: s16 place and size, a u32 colour and a u16 angle where the function takes s32s; the two stamps pass them so and only build as the game's code does with these narrower types */

/* Draws the quest's stamp (sprite 35, 164 pixels square) on a sheet at (x, y), w by
 * height. Its place, angle (within about 45 degrees of upright) and alpha come from seed.
 * pressTimer is the press's countdown: above 3 the stamp is drawn (pressTimer - 3) * 20
 * pixels larger, at 3 and 2 it is moved by a pixel or two, below that it lies flat. */
void Menu_DrawStamp(s16 x, s16 y, s16 w, s16 height, u16 seed, s16 pressTimer)
{
    u16 angle;
    s16 size;
    s16 dx;
    s16 dy;
    f32 half;

    angle = seed % (ANGLE_TURN / 4) + 0xE001;
    x += (s16)(abs(seed % (w - 204)) + 10);
    y += (s16)(abs((seed >> 3) % (height - 204)) + 10);
    dx = 0;
    dy = 0;
    if (pressTimer > 3) {
        size = (pressTimer - 3) * 20;
    } else {
        size = 0;
        switch (pressTimer) {
        case 3:
            dx = -2;
            break;
        case 2:
            dy = 1;
            break;
        }
    }
    x += dx;
    y += dy;
    half = 0.5f * size;
    x = x - half;
    y = y - half;
    size += 164;
    Icon_DrawRotated(x, y, size, size, ((angle % 8) * 16 + 128) << 24 | 0x50140A, angle, 35);
}

/* As Menu_DrawStamp for the quest board's smaller stamp: the round mark (sprite 31,
 * 96 pixels square) in its own colour, placed and turned from other bits of seed. */
void QuestBoard_DrawStamp(s16 x, s16 y, s16 w, s16 height, u16 seed, s16 pressTimer)
{
    u16 angle;
    s16 stampX;
    s16 size;
    s16 dx;
    s16 stampY;
    s16 dy;
    f32 half;

    angle = (seed >> 2) % (ANGLE_TURN / 4) + 0xE001;
    stampX = x + (abs((seed >> 3) % (w - 146)) + 10);
    stampY = y + (abs((seed >> 1) % (height - 146)) + 10);
    dx = 0;
    dy = 0;
    if (pressTimer > 3) {
        size = (pressTimer - 3) * 20;
    } else {
        size = 0;
        switch (pressTimer) {
        case 3:
            dx = -2;
            break;
        case 2:
            dy = 1;
            break;
        }
    }
    stampX += dx;
    stampY += dy;
    half = 0.5f * size;
    stampX -= half;
    stampY -= half;
    size += 96;
    Icon_DrawRotated(stampX, stampY, size, size, ((angle % 8) * 16 + 128) << 24 | 0xA1450, angle, 31);
}

/* Draws the arrows on both sides of a text of size-pixel characters that starts at (x, y):
 * the left arrow (sprite 40) before x and, when text is given, the right arrow (41)
 * after its end (Text_DisplayLength, in half widths). Both are size + 8 pixels square,
 * centred on the line, and step 6 pixels outward every other 16 frames (gFrameTick). */
void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color)
{
    s16 leftX;
    s16 arrowY;
    s16 arrowSize;
    s16 bob;
    s16 rightX;

    arrowSize = size + 8;
    arrowY = y - (arrowSize - size) / 2;
    leftX = x - arrowSize;
    bob = (gFrameTick & 0x10) ? 6 : 0;
    Icon_Draw(leftX - 4 - bob, arrowY, arrowSize, arrowSize, color, 40);
    if (text != NULL) {
        rightX = leftX + arrowSize + size * Text_DisplayLength(text) / 2;
        Icon_Draw(rightX + 4 + bob, arrowY, arrowSize, arrowSize, color, 41);
    }
}

/* As Menu_DrawArrows with the two arrow sprites swapped: both point at the text. */
void Menu_DrawArrowsRev(s16 x, s16 y, s16 size, char *text, s32 color)
{
    s16 leftX;
    s16 arrowY;
    s16 arrowSize;
    s16 bob;
    s16 rightX;

    arrowSize = size + 8;
    arrowY = y - (arrowSize - size) / 2;
    leftX = x - arrowSize;
    bob = (gFrameTick & 0x10) ? 6 : 0;
    Icon_Draw(leftX - 4 - bob, arrowY, arrowSize, arrowSize, color, 41);
    if (text != NULL) {
        rightX = leftX + arrowSize + size * Text_DisplayLength(text) / 2;
        Icon_Draw(rightX + 4 + bob, arrowY, arrowSize, arrowSize, color, 40);
    }
}

/* As Menu_DrawArrows with the arrows' size given (w pixels square). */
void Menu_DrawArrowsSized(s16 x, s16 y, s16 arrowSize, s16 size, char *text, s32 color)
{
    s16 leftX;
    s16 arrowY;
    s16 bob;
    s16 rightX;

    arrowY = y - (arrowSize - size) / 2;
    leftX = x - arrowSize;
    bob = (gFrameTick & 0x10) ? 6 : 0;
    Icon_Draw(leftX - 4 - bob, arrowY, arrowSize, arrowSize, color, 40);
    if (text != NULL) {
        rightX = leftX + arrowSize + size * Text_DisplayLength(text) / 2;
        Icon_Draw(rightX + 4 + bob, arrowY, arrowSize, arrowSize, color, 41);
    }
}

/* The file's initialised data, in the game's order */
TabPlace gLabelIconOffsets[12] = {
#include "lobby/007F19D0.inc"
};
