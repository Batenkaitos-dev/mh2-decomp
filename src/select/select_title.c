/*
 * select: the drawing of the title menu, mode select and the character
 * selection, and the overlay's common drawing helpers.
 *
 * gm_sub's title state machine (gm_main.c) runs these screens; this file
 * draws them and animates their pieces:
 *  - the title menu: the logo (Title_DrawLogo), the six entries with their
 *    big numbers and the chosen entry's help lines (Title_DrawMenu), over
 *    gm_sub's work area (GmWork);
 *  - mode select: two buttons that slide out and back in on the other side
 *    when the choice changes (ModeWork, at 0x1000 in the shared work area);
 *  - the character selection: the three save slots' characters sliding in
 *    and out (CharaSelWork, the same memory).
 * Sel_DrawParts, Sel_DrawPartsRotated (SelPart tables) and Sel_DrawHeader
 * (a screen's title in the header font) serve every screen of the
 * overlay. Sel_LoadCommonTextures loads the 16 textures they share.
 *
 * The file owns its .data and .rodata. The three screens share it because
 * their tables do not follow the code's order: the title's lie after the
 * character selection's in .data.
 */
#include "common.h"
#include "window.h"
#include "syssettings.h"
#include "m2c_macros.h"
#include "select.h"
#include "syssave.h"
#include "render_state.h"
#include "texture.h"
#include "card.h"
#include "model.h"
#include "text.h"
#include "engine_math.h"
#include "file_load.h"
#include "frame.h"
#include "gm_main.h"
#include "mem_util.h"
#include "packet_tim2.h"
#include "rand.h"
#include "packs.h"
#include "font.h"
#include "tex_files.h"

/* Compiled at optimization level 4: Sel_DrawHeader needs it, and every other
 * function of the file matches at both levels. */
#pragma optimization_level 4

struct Frame;

extern s32 gOvlTex[];
struct TexRect;
void TexRect_DrawRotated(struct TexRect *, u16); /* unit's own declaration: a u16 angle as Sel_DrawPartsRotated passes it; with the definition's s32, the call compiles differently */

extern void *gOptWork;
extern void *gWorkArea; /* sub_main: the overlays' shared work area */
extern TaskSlot gTaskSlots[16]; /* sub_main's task slots */

/* .data */
char *gSelHeaderTexts[16] = {
#include "select/006CACC0.inc"
};
/* The headers' letter size and y. */
s16 gSelHeaderLayout[16][2] = {
#include "select/006CAD00.inc"
};
Frame gTitleMenuFrame = {
#include "select/006CAD40.inc"
};
Frame gModeSelFrame = {
#include "select/006CAD60.inc"
};
ModeIcon gModeSelIcons[5] = {
#include "select/006CAD80.inc"
};
u8 gModeSelStartParams[8] = {
#include "select/006CADE8.inc"
};
s8 gModeSelOrders[8][2] = {
#include "select/006CADF0.inc"
};
u32 gModeSelColorsL1[2] = {
#include "select/006CAE00.inc"
};
u32 gModeSelColorsL0[2] = {
#include "select/006CAE08.inc"
};
u32 gModeSelColorsDim[2] = {
#include "select/006CAE10.inc"
};
u32 gModeSelButtonColor[2] = {
#include "select/006CAE18.inc"
};
s16 gModeSelButtonRects[2][5][4] = {
#include "select/006CAE20.inc"
};
s16 gModeSelLayoutA[2][4] = {
#include "select/006CAE70.inc"
};
s16 gModeSelLayoutB[2][16] = {
#include "select/006CAE80.inc"
};
SelIcon gCharaSelEntryIcons[7] = {
#include "select/006CAEC0.inc"
};
SelIcon gCharaSelSlotIcons[3] = {
#include "select/006CAFA0.inc"
};
u8 gCharaSelStartBytes[16] = {
#include "select/006CB000.inc"
};
s16 gCharaSelStartPos[8] = {
#include "select/006CB010.inc"
};
s8 gCharaSelInitA[16] = {
#include "select/006CB020.inc"
};
s8 gCharaSelSlotOrders[5][3] = {
#include "select/006CB030.inc"
};
s16 gCharaSelInitB[12] = {
#include "select/006CB040.inc"
};
s8 gCharaSelOrder[12] = {
#include "select/006CB058.inc"
};
f32 gCharaSelInitF[10] = {
#include "select/006CB070.inc"
};
u32 gCharaSelEntryColors[3] = {
#include "select/006CB098.inc"
};
s16 gCharaSelSlotPos[12][2] = {
#include "select/006CB0B0.inc"
};
/* The title logo's two parts. */
struct {
    SelPart p[2];
    u8 pad[12];
} gTitleLogoParts = {
#include "select/006CB0E0.inc"
};
SelPart gTitleDigitParts[30] = {
#include "select/006CB110.inc"
};
f32 gTitleLogoScales[4] = {
#include "select/006CB330.inc"
};

/* Restarts the title menu's frame counters. */
void Title_ResetBgAnim(void)
{
    gGmWork->frame = 0;
    gGmWork->count = 0;
}

/* Steps the title menu's frame counter (0-3) and, every fourth frame, count. */
void Title_StepBgAnim(void)
{
    GmWork *work;

    work = gGmWork;
    work->frame = (work->frame + 1) & 3;
    work = gGmWork;
    if (work->frame == 0) {
        work->count++;
    }
}

/*
 * Places the logo's second part at random around (184, 100), turned by a
 * random small angle, in grey.
 */
void Title_RandomView(void)
{
    s32 offset;

    offset = Rand_Next(1) % 9 * 6;
    if (Rand_Next(1) & 1) {
        offset = -offset;
    }
    gGmWork->logoX = offset + 184;
    offset = Rand_Next(1) % 7 * 6;
    if (Rand_Next(1) & 1) {
        offset = -offset;
    }
    gGmWork->logoY = offset + 100;
    offset = Rand_Next(1) % 6 * 5;
    if (Rand_Next(1) & 1) {
        offset = 359 - offset;
    }
    gGmWork->logoAngle = (u16)(s32)(0.5f + 65536.0f * offset / 360.0f);
    gGmWork->logoColor = 0xFF505050;
}

/*
 * Draws the title menu: its header and the menu (the argument, task slot 2,
 * is unused).
 */
void Title_Draw(void)
{
    Sel_DrawHeader(0);
    Title_DrawMenu(&gTaskSlots[2]);
}

/* Draws n parts of a SelPart table in one colour. */
void Sel_DrawParts(SelPart *part, s32 n, u32 color)
{
    Sprite spr;
    s32 i;

    for (i = 0; i < n; i++, part++) {
        Gfx_SetTexFilter(part->flag);
        Tex_Prepare(1, part->tex);
        Tex_Bind(part->tex);
        spr.x = part->x;
        spr.y = part->y;
        spr.w = part->w;
        spr.h = part->h;
        spr.color = color;
        spr.u0 = part->u0;
        spr.v0 = part->v0;
        spr.u1 = part->u1;
        spr.v1 = part->v1;
        Sprite_Draw(&spr);
    }
}

/*
 * Draws n parts of a SelPart table in one colour, turned by angle (ANGLE_TURN
 * a turn).
 */
void Sel_DrawPartsRotated(SelPart *part, s32 n, u32 color, s32 angle)
{
    Sprite spr;
    s32 i;

    for (i = 0; i < n; i++, part++) {
        Gfx_SetTexFilter(part->flag);
        Tex_Prepare(1, part->tex);
        Tex_Bind(part->tex);
        spr.x = part->x;
        spr.y = part->y;
        spr.w = part->w;
        spr.h = part->h;
        spr.color = color;
        spr.u0 = part->u0;
        spr.v0 = part->v0;
        spr.u1 = part->u1;
        spr.v1 = part->v1;
        TexRect_DrawRotated((struct TexRect *)&spr, angle);
    }
}

/*
 * Draws a big menu number (digit 0-9, in the style kind 0-2) scaled by
 * scale, at y and at x plus a margin that depends on the scale.
 */
void Title_DrawDigitScaled(s32 digit, s32 kind, f32 scale, s32 color, s32 y, s32 x)
{
    SelPart part;

    if (digit > 9) {
        digit %= 10;
    }
    part = ((SelPart *)gTitleDigitParts)[kind + digit * 3];
    if (scale > 1.0f) {
        part.x = x + 28;
    } else {
        part.x = x + 100;
    }
    part.y = y;
    part.w = (u16)(part.w * scale);
    part.h = (u16)(part.h * scale);
    Sel_DrawParts(&part, 1, color);
}

/*
 * Draws the title logo, scaled around its centre by its state's scale, and
 * (while it animates) its second part at the random view of
 * Title_RandomView, dimmed at the first step. The part's fields are written
 * through pointers, as the game's register use needs.
 */
void Title_DrawLogo(void)
{
    SelPart part;
    s32 k;
    s16 width;
    s16 height;
    s16 fullWidth;
    s16 fullHeight;
    u32 baseColor;
    u32 color;
    GmWork *work;
    s16 *partX;
    s16 *partY;
    s16 *partW;
    s16 *partH;

    work = gGmWork;
    if (work->logoFull != 0) {
        k = 2;
    } else {
        k = work->logoAnim;
        if (k == 0) {
            k = 2;
        } else {
            k--;
            if (k >= 3) {
                k = 2;
            }
        }
    }
    work->logoScale = ((f32 *)gTitleLogoScales)[k];
    Gs_SetState(RS_ZWRITE, 0);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    part = gTitleLogoParts.p[0];
    partW = &part.w;
    partH = &part.h;
    partX = &part.x;
    fullWidth = *partW;
    fullHeight = *partH;
    width = fullWidth * gGmWork->logoScale;
    height = fullHeight * gGmWork->logoScale;
    *partX += (s16)((fullWidth - width) / 2);
    partY = &part.y;
    *partY += (s16)((fullHeight - height) / 2);
    *partW = width;
    *partH = height;
    Sel_DrawParts(&part, 1, -1);
    if (gGmWork->logoAnim != 0) {
        Gs_SetState(RS_BLEND_OP, RF_BLEND_OP_REV_SUB);
        Gs_SetState(RS_BLEND, RS_FACTOR_ONE | RS_BLEND_DST(RS_FACTOR_ONE));
        if (k == 0) {
            baseColor = gGmWork->logoColor;
            color = (baseColor & 0xFF000000) | (((baseColor & 0xFF0000) >> 1) & 0xFF0000) | (((baseColor & 0xFF00) >> 1) & 0xFF00) |
                    (((baseColor & 0xFF) >> 1) & 0xFF);
        } else {
            color = gGmWork->logoColor;
        }
        part = gTitleLogoParts.p[1];
        work = gGmWork;
        *partX = work->logoX;
        *partY = work->logoY;
        fullWidth = *partW;
        fullHeight = *partH;
        width = fullWidth * work->logoScale;
        height = fullHeight * work->logoScale;
        *partX += (s16)((fullWidth - width) / 2);
        *partY += (s16)((fullHeight - height) / 2);
        *partW = width;
        *partH = height;
        Sel_DrawPartsRotated(&part, 1, color, work->logoAngle);
    }
    Gs_SetState(RS_BLEND_OP, RF_BLEND_OP_ADD);
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gs_SetState(RS_ZWRITE, 1);
}

extern char *gTitleMenuA[];     /* messages (select_psp.c): the title menu */
extern char *gTitleMenuB[];     /* ... and its help lines, two per entry */
/* unit's own declaration: s16 x and y as Title_DrawMenu passes them; with the definition's s32, the calls compile differently */
void Hud_DrawButtonIcon(s32, s16, s16, s32, u32, f32);  /* a button icon */
s32 Str_Length(char *);                         /* a message's width */
char *strchr(char *, s32);                  /* strchr */
char *strcpy(char *, char *);

/*
 * Draws the title menu: the six entries (a big number and a name each; the
 * chosen one, menuSel, at 1.5 times the size in its own colours, its number
 * animated by count), then the chosen entry's two help lines, with each "%r"
 * drawn as a button icon. The help lines reuse y. The argument is unused.
 */
void Title_DrawMenu(void *unused)
{
    char buf[256];
    char *cursor;
    s32 y;
    s32 i;
    s32 sel;
    f32 scale;
    f32 digitOffset;
    f32 textSize;
    s32 kind;
    s32 color;
    char **texts;
    u32 textColor;
    s32 digit;
    s32 textY;
    char *mark;
    s16 x;

    WindowFrame_Draw(&gTitleMenuFrame, 0);
    gGmWork->menuSelColor = 0xFF195E32;
    gGmWork->menuColor = 0xFF0C2F19;
    Gs_SetState(RS_ZWRITE, 0);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    texts = gTitleMenuA;
    y = 90;
    i = 0;
    sel = gGmWork->menuSel;
    do {
        if (i == sel) {
            color = 0xFFFFBD29;
            textColor = gGmWork->menuSelColor;
            scale = 1.5f;
            kind = gGmWork->count % 3;
            x = 100;
        } else {
            textColor = gGmWork->menuColor;
            color = 0xFFA87600;
            scale = 1.0f;
            kind = 0;
            x = 156;
        }
        digitOffset = (44.0f - 32.0f * scale) / 2.0f;
        digit = i;
        if (i == 1) {
            digit = 8;
        }
        if (i == 3) {
            digit = 9;
        }
        Title_DrawDigitScaled(digit, kind, scale, color, y + (s32)digitOffset, 0);
        textSize = 24.0f * scale;
        Font_SetSpacing(0.9f * textSize, textSize);
        textY = y + (s32)((44.0f - textSize) / 2.0f);
        Font_SetPos((s32)x + 2, textY + 2);
        Font_SetColor(0xFF000000);
        Font_Printf(*texts);
        Font_SetPos((s32)x, textY);
        Font_SetColor(textColor);
        Font_Printf(*texts);
        i++;
        texts++;
        y += 44;
    } while (i < 6);
    texts = gTitleMenuB;
    texts += sel * 2;
    y = 372;
    Font_SetSpacing(18, 18);
    i = 0;
    do {
        if (Str_Length(*texts) != 0) {
            strcpy(buf, *texts);
            cursor = buf;
            for (;;) {
                mark = strchr(cursor, '%');
                if (!mark != 0) {
                    break;
                }
                if (mark[1] == 'r') {
                    Hud_DrawButtonIcon(25, (mark - cursor) * 9 + 50, y, 341, -1, 0.9f);
                    mark[0] = ' ';
                    mark[1] = ' ';
                }
            }
            Text_PrintfAt(50, y, 0, buf);
        }
        i++;
        texts++;
        y += 20;
    } while (i < 2);
    Gs_SetState(RS_ZWRITE, 1);
}

extern char *gTitleHelpMenu[];     /* messages (select_psp.c) */
extern char *gTitleHelpTexts[];
s32 Str_Length(char *);
void Hud_DrawButtonIconWhite(s32, s32, s32, s32, f32);

/*
 * Draws mode select with the help lines of its entry and the two button
 * icons of the bottom line.
 */
void ModeSel_DrawScreen(void)
{
    s32 sel;
    char **texts;
    char **line;
    s32 width;

    sel = gGmWork->sel;
    ModeSel_Draw(sel, gGmWork->count);
    texts = gTitleHelpMenu;
    if (sel == 3) {
        sel = 4;
    }
    Font_SetSpacing(18, 18);
    line = &texts[sel * 2];
    width = Str_Length(*line);
    if (width != 0) {
        Text_PrintfAt((s32)((640.0f - 9.0f * width) / 2.0f), 354, 0, *line);
    }
    line = &texts[sel * 2 + 1];
    width = Str_Length(*line);
    if (width != 0) {
        Text_PrintfAt((s32)((640.0f - 9.0f * width) / 2.0f), 374, 0, *line);
    }
    Text_PrintfAt(230, 398, 0, gTitleHelpTexts[0]);
    Hud_DrawButtonIconWhite(20, 202, 396, 8, 0.9f);
    Hud_DrawButtonIconWhite(21, 314, 396, 8, 0.9f);
    Text_SetDefaults();
    Gs_SetState(RS_ZWRITE, 1);
}

/*
 * Draws the screens' background picture (system texture 7) at the grey
 * level gray.
 */
void Sel_DrawBackground(u8 gray)
{
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 7);
    Tex_Bind(7);
    Gs_SetState(RS_ZWRITE, 0);
    spr.x = 0;
    spr.y = 0;
    spr.w = 640;
    spr.h = 448;
    spr.color = (255 << 24) | (gray << 16) | (gray << 8) | gray;
    spr.v0 = 0;
    spr.u0 = 0;
    spr.u1 = 255;
    spr.v1 = 255;
    Sprite_Draw(&spr);
    Gs_SetState(RS_ZWRITE, 1);
}

char *strchr(char *, s32);             /* strchr */
char *strncpy(char *, char *, s32);     /* strncpy */
char *strcat(char *, char *);          /* strcat */
char *strcpy(char *, char *);          /* strcpy */
char *strupr(char *);                  /* to upper case */

/*
 * Draws a screen's title (gSelHeaderTexts[kind]) in the header font, one
 * sprite per letter, centred; %a, %b and %c in the text are spaces of 48,
 * 20 and 32 pixels. A shadow's y offset and colour are computed, then
 * overwritten (seen: nothing draws between them). Optimization level 4 and
 * the declaration order are what the match needs.
 */
void Sel_DrawHeader(s16 kind)
{
    char *text;
    f32 scale;
    s32 letter;
    char out[32];
    char *mark;
    s32 size;
    char *cursor;
    s32 x;
    s32 extra;
    s32 y;
    char buf[32];
    s32 n;
    Sprite spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    text = gSelHeaderTexts[kind];
    size = gSelHeaderLayout[kind][0];
    y = gSelHeaderLayout[kind][1];
    extra = 0;
    cursor = text;
    out[0] = 0;
    for (;;) {
        if ((mark = strchr(cursor, '%')) == NULL) {
            break;
        }
        n = mark - cursor;
        strncpy(buf, cursor, n);
        buf[n] = 0;
        strcat(out, buf);
        switch (mark[1]) {
        case 'a':
            extra += 48;
            break;
        case 'b':
            extra += 20;
            break;
        case 'c':
            extra += 32;
            break;
        }
        cursor = mark + 2;
    }
    strcat(out, cursor);
    x = (640 - (extra + size * Str_Length(out))) / 2.0f;
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    spr.h = size;
    spr.w = size;
    out[0] = 0;
    strcpy(out, text);
    strupr(out);
    cursor = out;
    while (*cursor != 0) {
        if (*cursor == '%') {
            switch (cursor[1]) {
            case 'A':
                x += 48;
                break;
            case 'B':
                x += 20;
                break;
            case 'C':
                x += 32;
                break;
            }
            cursor += 2;
        } else {
            letter = *cursor - 'A';
            spr.u0 = (letter % 8) * 32;
            spr.v0 = (letter / 8) * 32;
            scale = size / 32.0f;
            spr.u1 = spr.u0 + 31;
            spr.v1 = spr.v0 + 31;
            if (scale < 1.0f) {
                scale = 1.0f;
            }
            scale *= 2.0f;
            spr.color = 0xFF000000;
            spr.y = y + (s16)scale;
            spr.color = 0xFF2691A1;
            spr.x = x;
            spr.y = y;
            Sprite_Draw(&spr);
            x += size;
            cursor++;
        }
    }
}

/* Draws a big menu number (digit 0-9, in the style kind 0-2) at x, y. */
void Title_DrawDigit(s32 digit, s32 kind, s32 color, s16 x, s16 y)
{
    SelPart part;

    if (digit > 9) {
        digit %= 10;
    }
    part = ((SelPart *)gTitleDigitParts)[kind + digit * 3];
    part.x = x;
    part.y = y;
    part.w = part.w * 1;
    part.h = part.h * 1;
    Sel_DrawParts(&part, 1, color);
}

/*
 * Sets mode select's button pieces and positions for its layout
 * (which button is on the left), and where each button's slide back stops.
 * Static, as the callers' code needs.
 */
static void ModeSel_SetLayout(void)
{
    SelWork *base;
    s16 *layoutB;
    s32 k;
    s16 *home;
    s16 *layoutA;
    s32 i;

    base = (SelWork *)gSysSettings.workArea;
    k = base->modeSel.layout;
    layoutB = (s16 *)gModeSelLayoutB + k * 16;
    for (i = 0; i < 8; i++) {
        base->modeSel.piecePos[i][0] = *layoutB++;
        base->modeSel.piecePos[i][1] = *layoutB++;
    }
    layoutA = (s16 *)gModeSelLayoutA + k * 4;
    home = (s16 *)gModeSelLayoutA + (k ^ 1) * 4;
    for (i = 0; i < 2; i++) {
        base->modeSel.buttonPos[i][0] = *layoutA++;
        base->modeSel.buttonPos[i][1] = *layoutA++;
    }
    for (i = 0; i < 2; i++) {
        base->modeSel.buttonHomeX[i] = *home++;
        home++;
    }
    base->modeSel.shownLayout = base->modeSel.layout;
}

/*
 * Switches the pieces to the other layout while the buttons are off screen
 * (between the slide out and the slide back).
 */
static void ModeSel_SwapLayout(void)
{
    s32 k;
    SelWork *base;
    s16 *layoutB;
    s16 *layoutA;
    s32 i;

    base = (SelWork *)gSysSettings.workArea;
    k = base->modeSel.layout ^ 1;
    layoutB = (s16 *)gModeSelLayoutB + k * 16;
    layoutA = (s16 *)gModeSelLayoutA + k * 4;
    for (i = 0; i < 8; i++) {
        base->modeSel.piecePos[i][0] = *layoutB++;
        base->modeSel.piecePos[i][1] = *layoutB++;
    }
    for (i = 0; i < 2; i++) {
        layoutA++;
        base->modeSel.buttonPos[i][1] = *layoutA++;
    }
    base->modeSel.shownLayout = k;
}

/*
 * Starts both buttons sliding out: the left one to the left, the right one
 * to the right, on a quarter sine.
 */
void ModeSel_StartSlide(void)
{
    SelWork *base;

    base = (SelWork *)gSysSettings.workArea;
    base->modeSel.slideState[1] = 2;
    base->modeSel.slideState[2] = 2;
    base->modeSel.slideSpeed[0] = -20.0f;
    base->modeSel.slideSpeed[1] = 20.0f;
    base->modeSel.slidePhase[0] = 16384;
    base->modeSel.slidePhase[1] = 16384;
    base->modeSel.slidePhaseStep[0] = 4096;
    base->modeSel.slidePhaseStep[1] = 4096;
}

extern s32 gTexHandles[];
static void ModeSel_SetLayout(void);

/*
 * Starts mode select with sel chosen: its textures, its items and the
 * layout for sel.
 */
void ModeSel_Start(s16 sel)
{
    ModeWork *mode;
    s32 i;
    s32 j;
    u8 *params;

    mode = &((SelWork *)gSysSettings.workArea)->modeSel;
    memset(mode, 0, 308);
    for (j = 163, i = 0; i < 16; i++, j++) {
        mode->tex[i] = gTexHandles[j];
    }
    params = (u8 *)gModeSelStartParams;
    for (i = 0; i < 4; i++) {
        mode->items[i].kind = *params++;
        mode->items[i].pic2 = *params++;
    }
    for (i = 0; i < 3; i++) {
        if (i > 0 && i < 3) {
            mode->slideState[i] = 1;
        }
        mode->count++;
    }
    mode->sel = sel;
    mode->layout = mode->sel ^ 1;
    ModeSel_SetLayout();
}

/* 1 while the buttons are in place. */
s32 ModeSel_IsIdle(void)
{
    s32 state;

    state = ((SelWork *)gSysSettings.workArea)->modeSel.slideState[1];
    if ((state == 1) && (state == 1)) { /* the test twice: the game's code has both */
        return 1;
    }
    return 0;
}

/*
 * Mode select's buttons sliding out (state 2) and back in (3): swaps the
 * layout when both are out, and sets the new one when both are back.
 */
void ModeSel_UpdateSlide(void)
{
    ModeWork *mode;
    s32 i;

    mode = &((SelWork *)gSysSettings.workArea)->modeSel;
    for (i = 0; i < 2; i++) {
        if (mode->slideState[i + 1] == 2) {
            mode->buttonPos[i][0] += mode->slideSpeed[i] * Math_Sin(6.2831855f * mode->slidePhase[i] / 65536.0f);
            mode->slidePhase[i] = mode->slidePhaseStep[i] + mode->slidePhase[i];
            if (mode->slidePhase[i] >= 0x8000) {
                mode->slideState[i + 1] = 3;
                if (mode->slideState[1] == 3 && mode->slideState[2] == 3) {
                    ModeSel_SwapLayout();
                }
            }
        }
        if (mode->slideState[i + 1] == 3) {
            mode->buttonPos[i][0] += mode->slideSpeed[i] * Math_Sin(6.2831855f * mode->slidePhase[i] / 65536.0f);
            mode->slidePhase[i] = mode->slidePhaseStep[i] + mode->slidePhase[i];
            if (mode->slidePhase[i] >= 0xC000) {
                mode->slidePhase[i] = 49152;
            }
            if ((i == 0 && mode->buttonPos[i][0] >= mode->buttonHomeX[i]) || (i == 1 && mode->buttonPos[i][0] <= mode->buttonHomeX[i])) {
                mode->buttonPos[i][0] = mode->buttonHomeX[i];
                mode->slideState[i + 1] = 1;
                if (mode->slideState[1] == 1 && mode->slideState[2] == 1) {
                    mode->layout = mode->sel ^ 1;
                    ModeSel_SetLayout();
                }
            }
        }
    }
}

/* Empty in this build. */
void ModeSel_DrawExtra(void)
{

}


/*
 * Draws one mode select button: its two pictures (the second of pictures 1
 * and 3 with a shadow), in the colours of the shown layout, if on screen.
 * spr.y and spr.color are written through pointers, as the match needs.
 */
void ModeSel_DrawButton(s32 *item)
{
    Sprite spr;
    ModeIcon icon;
    s16 (*rect)[4];
    s32 pieceIndex;
    s32 buttonIndex;
    s16 k;
    ModeWork *mode;
    s32 i;
    ModeIcon *iconPtr;
    s32 savedX;
    s32 savedY;
    u32 savedColor;
    u32 *colorPtr;
    s16 *yPtr;

    colorPtr = &spr.color;
    yPtr = &spr.y;
    mode = &((SelWork *)gWorkArea)->modeSel;
    for (i = 0; i < 2; i++, item++) {
        k = *item;
        if (k == 255) {
            continue;
        }
        *(iconPtr = &icon) = ((ModeIcon *)gModeSelIcons)[k];
        rect = &((s16 (*)[5][4])gModeSelButtonRects)[mode->shownLayout][k];
        *colorPtr = icon.color;
        if (mode->shownLayout != 0) {
            if (k == 1) {
                *colorPtr = gModeSelColorsL1[0];
            }
            if (k == 3) {
                *colorPtr = gModeSelColorsL1[1];
            }
            if (k == 2) {
                *colorPtr = gModeSelButtonColor[0];
            }
            if (k == 4) {
                *colorPtr = gModeSelColorsDim[0];
            }
        } else {
            if (k == 1) {
                *colorPtr = gModeSelColorsL0[0];
            }
            if (k == 3) {
                *colorPtr = gModeSelColorsL0[1];
            }
            if (k == 2) {
                *colorPtr = gModeSelColorsDim[1];
            }
            if (k == 4) {
                *colorPtr = gModeSelButtonColor[0];
            }
        }
        if (k == 0) {
            spr.x = (*rect)[0];
            *yPtr = (*rect)[1];
        } else {
            if (k < 3) {
                buttonIndex = 0;
                if (k == 1) {
                    pieceIndex = 0;
                } else {
                    pieceIndex = 1;
                }
            } else {
                buttonIndex = 1;
                if (k == 3) {
                    pieceIndex = 4;
                } else {
                    pieceIndex = 5;
                }
            }
            spr.x = mode->buttonPos[buttonIndex][0] + mode->piecePos[pieceIndex][0];
            *yPtr = mode->buttonPos[buttonIndex][1] + mode->piecePos[pieceIndex][1];
        }
        spr.w = (*rect)[2];
        spr.h = (*rect)[3];
        spr.u0 = iconPtr->u0;
        spr.v0 = iconPtr->v0;
        spr.u1 = iconPtr->u1;
        spr.v1 = iconPtr->v1;
        if (spr.x < 640 && spr.x + spr.w >= 0 && *yPtr < 448 && *yPtr + spr.h >= 0) {
            Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
            Gs_SetState(RS_TEX_FILTER, 0);
            Tex_Use(1, (u32 *)&mode->tex[iconPtr->tex]);
            Gs_SetState(RS_TEXTURE, mode->tex[iconPtr->tex]);
            if (k == 1 || k == 3) {
                savedX = spr.x;
                savedY = *yPtr;
                savedColor = *colorPtr;
                spr.x = savedX + 8;
                *yPtr += 8;
                *colorPtr = 0x70000000;
                if (iconPtr->angle != 0) {
                    TexRect_DrawRotated((struct TexRect *)&spr, iconPtr->angle);
                } else {
                    Sprite_Draw(&spr);
                }
                spr.x = savedX;
                *yPtr = savedY;
                *colorPtr = savedColor;
            }
            if (iconPtr->angle != 0) {
                TexRect_DrawRotated((struct TexRect *)&spr, iconPtr->angle);
            } else {
                Sprite_Draw(&spr);
            }
        }
    }
}

extern char *gModeSelTexts[];     /* messages (select_psp.c): the modes' names */

/*
 * Draws the two buttons' numbers and names; the button of the chosen mode
 * is lit (when the buttons are in place) and its number animated by frame.
 * i ^ 1 is the mode shown on side i; the types and casts of x and y give the
 * game's sign extensions.
 */
void ModeSel_DrawButtons(s32 sel, s32 frame)
{
    char **texts;
    s32 kind;
    s32 i;
    s32 color;
    s32 anim;
    s16 x;
    s32 y;
    ModeWork *mode;
    u32 textColor;
    f32 *piece;

    texts = gModeSelTexts;
    mode = &((SelWork *)gSysSettings.workArea)->modeSel;
    anim = frame % 3;
    for (i = 0; i < 2; i++) {
        if ((i ^ 1) == sel && ModeSel_IsIdle() != 0) {
            kind = anim;
            color = 0xFFFFBD29;
            textColor = 0x80195E32;
        } else {
            textColor = 0x80000000;
            color = 0x80000000;
            kind = 0;
        }
        piece = mode->piecePos[i == 0 ? 2 : 6];
        Title_DrawDigit(1, kind, color, (s32)(mode->buttonPos[i][0] + piece[0]), (s32)(mode->buttonPos[i][1] + piece[1]));
        Font_SetSpacing(24, 24);
        piece = mode->piecePos[i == 0 ? 3 : 7];
        x = (s32)(mode->buttonPos[i][0] + piece[0]);
        y = (s16)(s32)(mode->buttonPos[i][1] + piece[1]);
        if ((i ^ 1) == sel) {
            Font_SetPos(x + 2, y + 2);
            Font_SetColor(0x80000000);
            Font_Printf(texts[i ^ 1]);
        }
        Font_SetPos((s32)x, y);
        Font_SetColor(textColor);
        Font_Printf(texts[i ^ 1]);
    }
}

/*
 * Draws mode select: the items in the layout's order, the buttons' texts and
 * the header. The item is addressed through its index at each use, as the
 * match needs.
 */
void ModeSel_Draw(s32 sel, s32 frame)
{
    s32 i;
    ModeWork *mode;
    s32 savedZWrite;
    s32 savedTexFilter;
    s32 savedBlend;
    SelWork *base;
    s32 k;

    base = (SelWork *)gSysSettings.workArea;
    mode = &base->modeSel;
    base->modeSel.order[1] = gModeSelOrders[base->modeSel.shownLayout][0];
    base->modeSel.order[2] = gModeSelOrders[base->modeSel.shownLayout][1];
    savedZWrite = Gs_GetState(RS_ZWRITE);
    savedTexFilter = Gs_GetState(RS_TEX_FILTER);
    savedBlend = Gs_GetState(RS_BLEND);
    Gs_SetState(RS_ZWRITE, 0);
    mode->sel = sel;
    for (i = 0; i < mode->count; i++) {
        k = mode->order[i];
        if (mode->items[k].kind != 255) {
            ModeSel_DrawButton(&mode->items[k].kind);
            if (mode->items[k].kind == 0) {
                WindowFrame_Draw(&gModeSelFrame, 0);
            }
        }
    }
    if (mode->state == 1) {
        ModeSel_DrawExtra();
    }
    ModeSel_DrawButtons(sel, frame);
    Sel_DrawHeader(1);
    Gs_SetState(RS_ZWRITE, savedZWrite);
    Gs_SetState(RS_TEX_FILTER, savedTexFilter);
    Gs_SetState(RS_BLEND, savedBlend);
}


/*
 * Starts the character selection: its textures, items and positions, with
 * the slot under the cursor taken from the last save slot used.
 */
void CharaSel_Start(void)
{
    ChrEdit *edit;
    CharaSelWork *work;
    s32 i;
    s32 j;
    u8 *params;
    s16 *posInit;

    edit = &gChrEdit;
    work = &((SelWork *)gSysSettings.workArea)->charaSel;
    memset(work, 0, 420);
    for (j = 163, i = 0; i < 16; i++, j++) {
        work->tex[i] = gTexHandles[j];
    }
    params = (u8 *)gCharaSelStartBytes;
    for (i = 0; i < 5; i++) {
        work->items[i].kind = *params++;
        work->items[i].pic2 = *params++;
    }
    posInit = (s16 *)gCharaSelStartPos;
    for (i = 0; i < 4; i++) {
        work->pos[i].x = *posInit++;
        work->pos[i].y = *posInit++;
        if (i > 0 && i < 4) {
            work->slideState[i] = 1;
            work->speed[i].x = -80.0f;
        }
        work->order[i] = i;
        work->count++;
    }
    work->slot = gSysSave.lastSlot;
    edit->slot = work->slot;
    for (i = 0; i < 3; i++) {
        work->order[i + 1] = ((s8 *)gCharaSelOrder)[work->slot * 3 + i];
        work->entryScale[i] = ((f32 *)gCharaSelInitF)[work->slot * 3 + i];
        work->entryY[i] = ((s16 *)gCharaSelInitB)[work->slot * 3 + i];
        work->delay[i + 1] = ((s8 *)gCharaSelInitA)[work->slot * 3 + i];
        work->target[i + 1].x = work->entryScale[i] < 1.0f ? 240 : 260;
    }
}

/*
 * Starts the three save slots' entries sliding out (state 2), each after
 * its delay for the slot under the cursor.
 */
void CharaSel_Leave(void)
{
    SelWork *base;

    base = (SelWork *)gSysSettings.workArea;
    base->charaSel.slideState[1] = 4;
    base->charaSel.speed[1].x = 80.0f;
    base->charaSel.delay[1] = gCharaSelSlotOrders[base->charaSel.slot][0];
    base->charaSel.slideState[2] = 4;
    base->charaSel.speed[2].x = 80.0f;
    base->charaSel.delay[2] = gCharaSelSlotOrders[base->charaSel.slot][1];
    base->charaSel.slideState[3] = 4;
    base->charaSel.speed[3].x = 80.0f;
    base->charaSel.delay[3] = gCharaSelSlotOrders[base->charaSel.slot][2];
    base->charaSel.state = 2;
}

/*
 * The character selection's frame: while choosing (state 1), follows the
 * slot under the cursor and blinks its frame; then the entries slide in
 * (states 1-3) or out (4-6), and the screen's state follows them.
 */
void CharaSel_Update(void)
{
    CharaSelWork *work;
    ChrEdit *edit;
    s32 i;

    work = &((SelWork *)gSysSettings.workArea)->charaSel;
    edit = &gChrEdit;
    if (work->state == 1) {
        work->frame++;
        if (!(work->frame & 3)) {
            work->blink++;
            work->blink %= 3;
        }
        work->slot = edit->slot;
        for (i = 0; i < 3; i++) {
            work->order[i + 1] = ((s8 *)gCharaSelOrder)[work->slot * 3 + i];
            work->entryScale[i] = ((f32 *)gCharaSelInitF)[work->slot * 3 + i];
            work->entryY[i] = ((s16 *)gCharaSelInitB)[work->slot * 3 + i];
            if (work->entryScale[i] < 1.0f) {
                work->pos[i + 1].x = 240.0f;
            } else {
                work->pos[i + 1].x = 260.0f;
            }
        }
    }
    for (i = 0; i < 5; i++) {
        if (work->items[i].kind != 255) {
            switch (work->slideState[i]) {
            case 1:
                if (work->delay[i] == 0) {
                    work->slideState[i] = 2;
                } else {
                    work->delay[i]--;
                    break;
                }
            case 2:
                work->pos[i].x += work->speed[i].x;
                if (work->pos[i].x <= work->target[i].x) {
                    work->pos[i].x = work->target[i].x;
                    work->slideState[i] = 3;
                    work->slidDone++;
                }
                break;
            case 4:
                if (work->delay[i] == 0) {
                    work->slideState[i] = 5;
                } else {
                    work->delay[i]--;
                    break;
                }
            case 5:
                work->pos[i].x += work->speed[i].x;
                if (!(work->pos[i].x < 860.0f)) {
                    work->items[i].kind = 255;
                    work->items[i].pic2 = 255;
                    work->slideState[i] = 6;
                    work->slidDone++;
                }
                break;
            case 0:
            case 3:
            case 6:
                break;
            }
        }
    }
    switch (work->state) {
    case 0:
        if (work->slidDone == 3) {
            work->state = 1;
            work->shown = 1;
        }
        break;
    case 1:
        break;
    case 3:
        break;
    case 2:
        if (work->slidDone == 6) {
            work->state = 3;
        }
        break;
    }
}

/* 1 while the player chooses a slot. */
s32 CharaSel_IsIdle(void)
{
    return ((SelWork *)gSysSettings.workArea)->charaSel.state == 1;
}

/* 1 once the entries have slid out. */
s32 CharaSel_IsDone(void)
{
    return ((SelWork *)gSysSettings.workArea)->charaSel.state == 3;
}

/* 1 once the entries have slid in. */
s32 CharaSel_IsShown(void)
{
    return ((SelWork *)gSysSettings.workArea)->charaSel.shown;
}

/*
 * Draws the save slots' frames: the slot under the cursor full size and
 * blinking, the other used slots smaller and dark.
 */
void CharaSel_DrawSlots(void)
{
    Sprite spr;
    SelIcon icon;
    CharaSelWork *work;
    SelIcon *iconPtr;
    s16 (*pos)[2];
    s32 i;

    iconPtr = &icon;
    work = &((SelWork *)gSysSettings.workArea)->charaSel;
    *iconPtr = gCharaSelSlotIcons[0];
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gs_SetState(RS_TEX_FILTER, 0);
    Tex_Use(1, (u32 *)&gTexHandles[iconPtr->tex]);
    Gs_SetState(RS_TEXTURE, gTexHandles[iconPtr->tex]);
    pos = (s16 (*)[2])gCharaSelSlotPos + work->slot * 3;
    for (i = 0; i < 3; i++) {
        if (SaveMenu_SlotUsed(i) == 0) {
            pos++;
            continue;
        }
        if (i == work->slot) {
            icon = gCharaSelSlotIcons[work->blink];
            spr.w = iconPtr->w;
            spr.h = iconPtr->h;
            spr.color = 0xFFFF6000;
        } else {
            icon = gCharaSelSlotIcons[0];
            spr.w = 0.7f * iconPtr->w;
            spr.h = 0.7f * iconPtr->h;
            spr.color = 0xFF342B2A;
        }
        spr.x = (*pos)[0];
        spr.y = (*pos)[1];
        pos++;
        spr.u0 = iconPtr->u0;
        spr.v0 = iconPtr->v0;
        spr.u1 = iconPtr->u1;
        spr.v1 = iconPtr->v1;
        Sprite_Draw(&spr);
    }
}

/*
 * Draws one entry of the character selection: its two pictures; the first
 * is dimmed (and lowered) unless it is the slot under the cursor, the second
 * is scaled by the slot's depth.
 */
void CharaSel_DrawEntry(s32 *item, CharaSelPos *pos)
{
    s32 k;
    CharaSelWork *work;
    Sprite spr;
    s16 dy;
    SelIcon icon;
    s32 i;
    s16 yOffset;
    SelIcon *iconPtr;

    work = &((SelWork *)gSysSettings.workArea)->charaSel;
    for (i = 0; i < 2; i++, item++) {
        if (*item == 255) {
            continue;
        }
        *(iconPtr = &icon) = ((SelIcon *)gCharaSelEntryIcons)[*item];
        dy = 0;
        if (i == 0) {
            k = -1;
            if (*item == 1) {
                k = 2;
            }
            if (*item == 3) {
                k = 1;
            }
            if (*item == 5) {
                k = 0;
            }
            if (k >= 0) {
                if (k == work->slot) {
                    iconPtr->color = -1;
                } else {
                    iconPtr->color = 0xFFA0A0A0;
                    if (work->slot == 0 && *item == 3) {
                        dy = 20;
                    }
                    if (work->slot == 1 && *item == 1) {
                        dy = 20;
                    }
                    if (work->slot == 2 && *item == 5) {
                        dy = 20;
                    }
                }
            }
        }
        if (i == 1) {
            k = -1;
            if (*item == 2) {
                k = 0;
            }
            if (*item == 4) {
                k = 1;
            }
            if (*item == 6) {
                k = 2;
            }
            if (k != -1) {
                iconPtr->y = work->entryY[k];
                iconPtr->scale = work->entryScale[k];
                if (iconPtr->scale < 1.0f) {
                    iconPtr->color = ((u32 *)gCharaSelEntryColors)[k];
                }
            }
        }
        spr.x = dy + (iconPtr->x + (s16)pos->x);
        spr.y = iconPtr->y + (s16)pos->y;
        if (iconPtr->scale < 1.0f) {
            spr.w = iconPtr->w * iconPtr->scale;
            spr.h = iconPtr->h * iconPtr->scale;
            yOffset = (iconPtr->h * iconPtr->scale - spr.h) / 2.0f;
        } else {
            yOffset = 0;
            spr.w = iconPtr->w;
            spr.h = iconPtr->h;
        }
        spr.x += (s16)(iconPtr->w - spr.w);
        spr.y += yOffset;
        spr.u0 = iconPtr->u0;
        spr.v0 = iconPtr->v0;
        spr.u1 = iconPtr->u1;
        spr.v1 = iconPtr->v1;
        spr.color = iconPtr->color;
        if (spr.x < 640 && spr.x + spr.w >= 0 && spr.y < 448 && spr.y + spr.h >= 0) {
            Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
            Gs_SetState(RS_TEX_FILTER, 0);
            Tex_Use(1, (u32 *)&work->tex[iconPtr->tex]);
            Gs_SetState(RS_TEXTURE, work->tex[iconPtr->tex]);
            if (iconPtr->angle != 0) {
                TexRect_DrawRotated((struct TexRect *)&spr, iconPtr->angle);
            } else {
                Sprite_Draw(&spr);
            }
        }
    }
}

/*
 * Draws the character selection: the entries in their order, the slots'
 * frames while choosing, and the header.
 */
void CharaSel_Draw(void)
{
    s32 i;
    CharaSelWork *work;
    s32 savedZWrite;
    s32 savedTexFilter;
    s32 savedBlend;

    work = &((SelWork *)gSysSettings.workArea)->charaSel;
    savedZWrite = Gs_GetState(RS_ZWRITE);
    savedTexFilter = Gs_GetState(RS_TEX_FILTER);
    savedBlend = Gs_GetState(RS_BLEND);
    Gs_SetState(RS_ZWRITE, 0);
    for (i = 0; i < work->count; i++) {
        if (work->items[work->order[i]].kind != 255) {
            CharaSel_DrawEntry(&work->items[work->order[i]].kind, &work->pos[work->order[i]]);
        }
    }
    if (work->state == 1) {
        CharaSel_DrawSlots();
    }
    Sel_DrawHeader(5);
    Gs_SetState(RS_ZWRITE, savedZWrite);
    Gs_SetState(RS_TEX_FILTER, savedTexFilter);
    Gs_SetState(RS_BLEND, savedBlend);
}

/*
 * Loads the overlay's common texture pack into gOvlTex (up to 16
 * textures), unless they are loaded already.
 */
void Sel_LoadCommonTextures(void)
{
    s32 i;
    s32 n;
    s32 buf;

    buf = (s32)gSysSettings.loadBuf;
    LoadCompressedFile((u8 *)buf, D_0042F1DC);
    n = Pack_Count((s32 *)buf);
    if (gOvlTex[0] == 0) {
        for (i = 0; i < n; i++) {
            if (i >= 16) {
                break;
            }
            gOvlTex[i] = Tex_CreateFromTim2((u8 *) (Pack_GetPart((u8 *)buf, i)), TEX_MODE_ON_DEMAND);
        }
        Mem_Compact();
    }
}

/* Frees the 16 textures from 163 that Sel_LoadCommonTextures made. */
void Sel_FreeTextures(void)
{
    Tex_FreeRange(163, 16);
    Mem_Compact();
}
