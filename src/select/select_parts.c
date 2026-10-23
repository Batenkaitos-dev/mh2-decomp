/*
 * select: the drawing and animation of character creation's pages
 * for character creation's tasks.
 *
 * The first page's two side panels (one per gender) drop in
 * (Parts_DrawSides); the editing page's seven panels (name, type, face,
 * hair, colour, voice, done) slide in and out (Parts_SlideInPanels,
 * Parts_SlideOutPanels, Parts_DrawPanels), with the cursor that jumps
 * between them; the face, hair and voice panels list the search conditions
 * that filter the parts, with arrow markers that bounce when a value
 * changes. The state lives in CharaWork (select.h), at 0x1000 in the
 * shared work area; Parts_Animate steps it each frame.
 *
 * The file owns its .data (the pieces, positions and label tables) and
 * .rodata.
 */
#include "common.h"
#include "syssettings.h"
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "select.h"
#include "select/select_parts.h"   /* the texts this file uses (tools/content_gen.py) */
#include "render_state.h"
#include "text.h"
#include "engine_math.h"
#include "font.h"
#include "libc.h"
#include "sound.h"
#include "sprite_queue.h"


struct TexRect;

/* .data */
SelPart gPartsSprites[2] = {
#include "select/006CB340.inc"
};
/* The colour swatches (Parts_DrawSwatches): texture cells, then positions. */
s16 gPartsSwatchTex[6] = {
#include "select/006CB368.inc"
};
s16 gPartsSwatchPosA[4][2] = {
#include "select/006CB380.inc"
};
s16 gPartsSwatchPosB[4][2] = {
#include "select/006CB390.inc"
};
s16 gPartsSwatchPos[12][2] = {
#include "select/006CB3A0.inc"
};
/* The panels' frames (Parts_DrawPanels): x, y, w, h, angle in degrees. */
s16 gPartsRotations[8][5] = {
#include "select/006CB3D0.inc"
};

SelPart gPartsPieces[8] = {
#include "select/006CB420.inc"
};
SelPart gPartsCursorParts[5] = {
#include "select/006CB4B0.inc"
};
SelPart gPartsTypeSprites[4] = {
#include "select/006CB510.inc"
};
s16 gPartsPanels[14][12] = {
#include "select/006CB560.inc"
};
/* The cursor's position on each panel (Parts_DrawPanels). */
s16 gPartsCursorPos[8][2] = {
#include "select/006CB6B0.inc"
};
s16 gPartsTypePos[4][2] = {
#include "select/006CB6D0.inc"
};
/* The search conditions of the face, hair and voice pages (ChrEdit bytes
 * 0x50-0x57): the name of each value, as an index into select_psp.c's
 * gPartsLabels (0x1A: not set). Indexed by the value, or by sex * 3 + value
 * (the voice's age: sex * 4 + value). */
s16 gPartsFaceLabels0[8] = {
#include "select/006CB6E0.inc"
};
s16 gPartsFaceLabels1[8] = {
#include "select/006CB6F0.inc"
};
s16 gPartsFaceLabels2[8] = {
#include "select/006CB700.inc"
};
s16 gPartsHairLabels0[8] = {
#include "select/006CB710.inc"
};
s16 gPartsHairLabels1[4] = {
#include "select/006CB720.inc"
};
s16 gPartsHairLabels2[6] = {
#include "select/006CB728.inc"
};
s16 gPartsVoiceLabels0[7] = {
#include "select/006CB738.inc"
};
s16 gPartsVoiceLabels1[6] = {
#include "select/006CB748.inc"
};


/*
 * Clears character creation's work (CharaWork) and sets the side panels'
 * first slide steps and their shared delay.
 */
void Parts_Init(void)
{
    CharaWork *charaWork;

    charaWork = &((SelWork *)gSysSettings.workArea)->chara;
    memset(charaWork, 0, sizeof(CharaWork));
    charaWork->slide[0] = 2;
    charaWork->slide[1] = 6;
    charaWork->slide[4] = 36;
}

/*
 * Draws the colour swatches beside a side panel: kind 0 (the first side) has
 * 4, kind 1 has 6; alt draws them smaller and dimmed, at the other layout.
 */
void Parts_DrawSwatches(s16 kind, s16 alt)
{
    s16 cell;
    Sprite spr;
    s16 count;
    u32 color;
    s16 i;
    s16 *pos;
    s16 *tex;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    if (kind == 0) {
        count = 4;
        if (alt == 0) {
            color = 0xFF3F8EEA;
            pos = gPartsSwatchPosA[0];
        } else {
            color = 0xFF254B61;
            pos = gPartsSwatchPosB[0];
        }
        tex = &gPartsSwatchTex[2];
    } else {
        count = 6;
        if (alt == 0) {
            color = 0xFFBC1D0D;
            pos = gPartsSwatchPos[0];
        } else {
            color = 0xFF47050B;
            pos = gPartsSwatchPos[6];
        }
        tex = gPartsSwatchTex;
    }
    spr.color = color;
    if (alt == 0) {
        spr.w = 32;
    } else {
        spr.w = 24;
    }
    spr.h = spr.w;
    for (i = 0; i < count; i++) {
        spr.x = pos[0];
        spr.y = pos[1];
        pos += 2;
        cell = *tex++;
        spr.u0 = (cell & 7) << 5;
        spr.v0 = (cell >> 3) << 5;
        spr.u1 = spr.u0 + 31;
        spr.v1 = spr.v0 + 31;
        Sprite_Draw(&spr);
    }
}

extern void *gWorkArea; /* sub_main: the overlays' shared work area */
extern char *gOkTypeTexts[];  /* messages (select_psp.c) */
extern char *gGenderBackTexts[];
extern char *gAgeFeatureTexts[];
extern char *gHairRowTexts[];
extern char *gFaceHairRowTexts[];
extern char *gPartsLabels[];
s32 Str_Length(char *);                /* a message's width */

/*
 * The two side panels of the first page (one per gender) sliding in: per
 * side, its state in charaWork->sideState (0 a delay, 1-5 the drop and
 * bounces, 6 waiting for the shared delay, 7 in place: the side of the
 * other gender tilts away and dims) and its timer in slide[side]; slide[4] is
 * the shared delay. Returns 1 once the shared delay is over.
 * k serves two roles (the delay, then the side's index), and
 * Parts_DrawSwatches takes s16 parameters: the game's register use needs
 * both.
 */
s32 Parts_DrawSides(TaskSlot *task)
{
    ChrEdit *edit;
    CharaWork *charaWork;
    SelWork *base;
    s16 busy;
    s32 k;
    s32 color;
    s16 i;
    u16 angle;
    s16 alt;
    SelPart part;

    edit = &gChrEdit;
    base = gWorkArea;
    charaWork = &base->chara;
    busy = 0;
    k = charaWork->slide[4];
    if (k != 0) {
        busy = 1;
        charaWork->slide[4] = k - 1;
    }
    for (i = 0; i < 2; i++) {
        part = ((SelPart *)gPartsSprites)[i];
        k = i;
        color = (i == 0) ? 0xFF3F8EEA : 0xFFBC1D0D;
        angle = 0;
        switch (charaWork->sideState[k]) {
        case 0:
            charaWork->slide[i]--;
            if (charaWork->slide[i] <= 0) {
                charaWork->sideState[k]++;
                charaWork->slide[i] = 8;
                if (i == 0) {
                    Sound_PlaySeFlat(1, 8, 0);
                }
            }
            continue;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            charaWork->slide[i]--;
            switch (charaWork->sideState[k]) {
            case 1:
                part.y -= (s16)(charaWork->slide[i] * 40);
                break;
            case 2:
                part.y -= (s16)((4 - charaWork->slide[i]) * 4);
                break;
            case 3:
                part.y -= (s16)(charaWork->slide[i] * 4);
                break;
            case 4:
                part.y -= (s16)((2 - charaWork->slide[i]) * 3);
                break;
            case 5:
                part.y -= (s16)(charaWork->slide[i] * 3);
                break;
            }
            if (charaWork->slide[i] <= 0) {
                charaWork->sideState[k]++;
                if (charaWork->sideState[k] >= 4) {
                    charaWork->slide[i] = 2;
                } else {
                    charaWork->slide[i] = 4;
                }
            }
            break;
        case 6:
            if (charaWork->slide[4] == 0) {
                charaWork->sideState[k]++;
                if (i == 0) {
                    Sound_PlaySeFlat(0, 18, 0);
                }
            }
            break;
        case 7:
            alt = 1;
            if (edit->gender != i) {
                if (!k) {
                    color = 0xFF254B61;
                    angle = -0x1C71;
                    part.x = 113;
                    part.y = 139;
                    part.w = 128;
                    part.h = 140;
                } else {
                    color = 0xFF47050B;
                    angle = 0x1C72;
                    part.x = 393;
                    part.y = 139;
                    part.w = 128;
                    part.h = 140;
                }
            } else {
                alt = 0;
            }
            Parts_DrawSwatches(i, alt);
            break;
        }
        Sel_DrawPartsRotated(&part, 1, color, angle);
    }
    return busy == 0;
}

/*
 * Starts the seven panels sliding in, one after another (2 steps apart), the
 * panel sel (0xFF: none) already in place.
 */
void Parts_SlideInPanels(u8 sel)
{
    SelWork *base;
    CharaWork *charaWork;
    s16 i;
    s32 delay;

    base = (SelWork *)gSysSettings.workArea;
    base->chara.slideOut = 0;
    charaWork = &base->chara;
    Sound_PlaySeFlat(1, 11, 0);
    for (i = 0, delay = 10; i < 7; i++, delay += 2) {
        if (i == sel) {
            charaWork->slide[i] = 0;
            charaWork->sliding[i] = 0;
        } else {
            charaWork->slide[i] = delay;
            charaWork->sliding[i] = 1;
        }
    }
}

/*
 * Starts the seven panels sliding out, one after another (2 frames apart),
 * the panel sel staying.
 */
void Parts_SlideOutPanels(u8 sel)
{
    SelWork *base;
    CharaWork *charaWork;
    s16 i;
    s16 delay;

    base = (SelWork *)gSysSettings.workArea;
    delay = 0;
    base->chara.slideOut = 1;
    charaWork = &base->chara;
    for (i = 0; i < 7; i++) {
        charaWork->slide[i] = 0;
        if (i == sel) {
            charaWork->slideDelay[i] = 0;
            charaWork->sliding[i] = 0;
        } else {
            charaWork->slideDelay[i] = delay;
            charaWork->sliding[i] = 1;
            delay += 2;
        }
    }
}

/* Starts the bounce of the list entry entry (its arrow marker lights). */
void Parts_StartBounce(u8 entry)
{
    CharaWork *charaWork;

    charaWork = &((SelWork *)gSysSettings.workArea)->chara;
    charaWork->bounce[entry] = 1;
    charaWork->bounceOfs[entry] = 0;
    Sound_PlaySeFlat(0, 22, 0);
}

/* Stops every list entry's bounce. */
void Parts_ResetBounces(void)
{
    s16 i;
    CharaWork *charaWork;

    charaWork = &((SelWork *)gSysSettings.workArea)->chara;
    for (i = 0; i < 10; i++) {
        charaWork->bounce[i] = 0;
        charaWork->bounceOfs[i] = 0;
    }
}

/*
 * Starts the cursor's jump: from the chosen panel's row up to the top row,
 * or (toPanel) from the top row down to the chosen panel's.
 */
void Parts_StartPanel(s16 toPanel)
{
    CharaWork *charaWork;
    SelWork *base;
    s16 panelY;

    base = (SelWork *)gSysSettings.workArea;
    charaWork = &base->chara;
    panelY = gPartsPanels[gChrEdit.panel][1];
    base->chara.cursorJump = 1;
    base->chara.cursorJumpFrame = 0;
    if (toPanel == 0) {
        charaWork->cursorY = panelY;
        charaWork->cursorYEnd = gPartsPanels[0][1];
    } else {
        charaWork->cursorY = gPartsPanels[0][1];
        charaWork->cursorYEnd = panelY;
    }
}

/*
 * Character creation's animations, a frame: the list entries' bounces, the
 * cursor's jump, the cursor's blink (a step each 4 frames) and the panels
 * sliding in or out. The bounce loop reads bounce[k] and bounceOfs[k]
 * through shifted = (u8 *)charaWork + k, at their offsets 0x184 and 0x18E:
 * the field forms change the game's register use.
 */
void Parts_Animate(void)
{
    u8 *shifted;
    CharaWork *charaWork;
    s16 i;
    s32 k;

    charaWork = &((SelWork *)gSysSettings.workArea)->chara;
    for (i = 0; i < 10; i++) {
        k = i;
        shifted = (u8 *)charaWork + k;
        switch (((s8 *)shifted)[388]) {
        case 0:
            break;
        case 1:
            if (k & 1) {
                ((s8 *)shifted)[398]++;
            } else {
                ((s8 *)shifted)[398]--;
            }
            if (Math_Absf(((s8 *)shifted)[398]) >= 5.0f) {
                ((s8 *)shifted)[388] = 2;
            }
            break;
        case 2:
            if (k & 1) {
                ((s8 *)shifted)[398]--;
                if (((s8 *)shifted)[398] <= 0) {
                    ((s8 *)shifted)[388] = 0;
                }
            } else {
                ((s8 *)shifted)[398]++;
                if (((s8 *)shifted)[398] >= 0) {
                    ((s8 *)shifted)[388] = 0;
                }
            }
            break;
        }
    }
    switch (charaWork->cursorJump) {
    case 0:
        break;
    case 1:
        charaWork->cursorY -= 28;
        charaWork->cursorJumpFrame++;
        if (charaWork->cursorJumpFrame >= 12) {
            charaWork->cursorJump = 2;
        }
        break;
    case 2:
        charaWork->cursorY += 36;
        if (charaWork->cursorY >= charaWork->cursorYEnd) {
            charaWork->cursorY = charaWork->cursorYEnd;
            charaWork->cursorJump = 0;
        }
        break;
    }
    if (!(gFrameTick & 3)) {
        charaWork->blink++;
        if (charaWork->blink > 2) {
            charaWork->blink = 0;
        }
    }
    for (i = 0; i < 7; i++) {
        if (charaWork->slideOut == 0) {
            if (charaWork->slide[i] > 0) {
                charaWork->slide[i]--;
            } else {
                charaWork->sliding[i] = 0;
            }
        } else if (charaWork->sliding[i] != 0) {
            if (charaWork->slideDelay[i] > 0) {
                charaWork->slideDelay[i]--;
                charaWork->slide[i]++;
            } else {
                charaWork->slide[i]++;
                if (charaWork->slide[i] >= 10) {
                    charaWork->sliding[i] = 0;
                }
            }
        }
    }
}

/* Draws the cursor at x, y (during a jump, at the jump's y), blinking. */
void Parts_DrawCursor(s16 x, s16 y, s32 unused)
{
    SelWork *base;
    CharaWork *charaWork;
    SelPart part;

    base = (SelWork *)gSysSettings.workArea;
    charaWork = &base->chara;
    if (base->chara.cursorJump == 0) {
        part = gPartsPieces[charaWork->blink + 2];
    } else {
        part = gPartsPieces[5];
    }
    part.x = x;
    if (charaWork->cursorJump == 0) {
        part.y = y;
    } else {
        part.y = charaWork->cursorY;
    }
    Sel_DrawParts(&part, 1, 0xFFFF6000);
}

const char gPartsStrFormat[3] = {
#include "select/006CE110.inc"
};

extern char *gCharaPartTitles[];   /* the panels' names (select_psp.c) */

/*
 * Character creation's seven panels (name, type, face, hair, colour, voice,
 * done), the chosen one (gChrEdit.panel) drawn last and lit: each with its
 * rotated frame, its title and value (the choice's number, the name or the
 * RGB colour) and, on the chosen one, the cursor. A panel is shifted right
 * by 80 per slide step. In the task's mode 5 the chosen panel is drawn at
 * the top row's height. Returns 1.
 * The chosen panel is stored through k, as the game's register use needs.
 */
s32 Parts_DrawPanels(TaskSlot *task)
{
    CharaWork *charaWork;
    s16 i;
    s16 j;
    ChrEdit *edit;
    s16 x;
    s16 dy;
    s16 *rot;
    PartsRec *rec;
    s32 titleSize;
    s32 valueSize;
    u8 *orderPtr;
    SelPart part;
    u8 order[7];
    s32 k;

    edit = &gChrEdit;
    charaWork = &((SelWork *)gSysSettings.workArea)->chara;
    Gs_SetState(RS_ZWRITE, 0);
    k = edit->panel;
    order[6] = k;
    for (j = 6, i = 5; i >= 0; j--, i--) {
        if (j == edit->panel) {
            j--;
        }
        order[i] = j;
    }
    for (i = 0, orderPtr = order; i < 7; orderPtr++, i++) {
        k = (s16)*orderPtr;
        dy = 0;
        x = charaWork->slide[k] * 80;
        part = gPartsPieces[k & 1];
        rot = (s16 *)gPartsRotations + k * 5;
        part.x = x + rot[0];
        if (part.x < 700) {
            part.y = rot[1];
            part.w = rot[2];
            part.h = rot[3];
            Sel_DrawPartsRotated(&part, 1, i == 6 ? -1 : 0xFFA0A0A0, (u16)(s32)(0.5f + 65536.0f * rot[4] / 360.0f));
        }
        if (i == 6) {
            rec = (PartsRec *)gPartsPanels[k];
            titleSize = 24;
            valueSize = 18;
            switch (k) {
            case 2:
            case 4:
            case 5:
                Font_SetColor(0xFF000000);
                break;
            default:
                Font_SetColor(-1);
                break;
            }
            if (task->mode == CHARA_CREATE_EDIT) {
                dy = gPartsPanels[0][1] - rec->y;
            }
        } else {
            rec = (PartsRec *)gPartsPanels[k + 7];
            titleSize = 20;
            valueSize = 16;
            Font_SetColor(0xFF000000);
        }
        part = ((SelPart *)gPartsCursorParts)[rec->part];
        part.x = rec->x + x;
        if (part.x < 700) {
            part.y = rec->y + dy;
            part.w = rec->w;
            part.h = rec->h;
            Sel_DrawParts(&part, 1, rec->color);
            if (charaWork->slide[k] == 0 && charaWork->slideDelay[k] == 0) {
                Font_SetPos(rec->tx + x, rec->ty + dy);
                Font_SetSpacing(titleSize, titleSize);
                Text_PrintfColored(gPartsStrFormat, gCharaPartTitles[k]);
                if (k != 6) {
                    Font_SetPos(rec->tx2 + x, rec->ty2 + dy);
                    Font_SetSpacing(valueSize, valueSize);
                    switch (k) {
                    case 0:
                        Text_PrintfColored(gPartsStrFormat, edit->name);
                        break;
                    case 1:
                        Text_PrintfColored(TXT(0x006CE118) /* a choice label and its number */, edit->type + 1);
                        break;
                    case 2:
                        Text_PrintfColored(TXT(0x006CE118) /* a choice label and its number */, edit->face + 1);
                        break;
                    case 5:
                        Text_PrintfColored(TXT(0x006CE118) /* a choice label and its number */, edit->voice + 1);
                        break;
                    case 3:
                        Text_PrintfColored(TXT(0x006CE118) /* a choice label and its number */, edit->hair + 1);
                        break;
                    case 4:
                        Text_PrintfColored("R%03d G%03d B%03d", (edit->color >> 16) & 0xFF,
                                           (edit->color >> 8) & 0xFF, edit->color & 0xFF);
                        break;
                    }
                }
                if (i == 6) {
                    Parts_DrawCursor(gPartsCursorPos[k][0], dy + gPartsCursorPos[k][1], 0);
                }
            }
        }
    }
    Gs_SetState(RS_ZWRITE, 1);
    return 1;
}

/*
 * Draws the rotated backing piece (gPartsPieces[6]) at x, y, sized width x
 * height, turned deg degrees.
 */
void Parts_DrawRotated(s16 x, s16 y, s16 width, s16 height, s32 deg)
{
    SelPart part;

    part = gPartsPieces[6];
    part.x = x;
    part.y = y;
    part.w = width;
    part.h = height;
    Sel_DrawPartsRotated(&part, 1, -1, (u16)(s32)(0.5f + 65536.0f * deg / 360.0f));
}

/*
 * Draws an arrow marker (system texture 363), turned by angle (ANGLE_TURN a
 * turn). Its colour is color when entry is 255, else lit while list entry
 * entry bounces.
 */
void Parts_DrawMarker(s16 x, s16 y, s16 width, s16 height, s32 angle, s32 color, u8 entry)
{
    CharaWork *charaWork;
    Sprite spr;
    s32 markerColor;

    charaWork = &((SelWork *)gSysSettings.workArea)->chara;
    if (entry == 255) {
        markerColor = color;
    } else if (charaWork->bounce[entry] == 0) {
        markerColor = PARTS_COLOR_MARKER;
    } else {
        markerColor = PARTS_COLOR_LIT;
    }
    Tex_Prepare(1, 363);
    Tex_Bind(363);
    spr.u1 = 15;
    spr.v1 = 15;
    spr.x = x;
    spr.y = y;
    spr.w = width;
    spr.h = height;
    spr.color = markerColor;
    spr.u0 = 0;
    spr.v0 = 0;
    TexRect_DrawRotated((struct TexRect *)&spr, angle);
}

/*
 * The type list (four entries): the entry sel is highlighted, its marker
 * moves every 16 frames.
 */
void Parts_DrawTypes(s16 x, s16 y, s32 sel)
{
    SelPart part;
    s32 partColor;
    s16 markerX;
    SelPart *src;
    s32 markerColor;
    s16 i;
    s16 (*pos)[2];
    s32 selected;
    s32 textX;
    s32 textY;
    s32 textColor;
    s32 k;

    Parts_DrawRotated(x, y, 334, 242, -2);
    selected = (u8)sel;
    i = 0;
    src = (SelPart *)gPartsTypeSprites;
    pos = gPartsTypePos;
    for (; i < 4; i++) {
        part = *src;
        part.x += x;
        part.y += y;
        k = i;
        if (k == selected) {
            partColor = -1;
            textColor = PARTS_COLOR_SELECTED;
            markerColor = PARTS_COLOR_LIT;
        } else {
            partColor = 0xFF606060;
            textColor = PARTS_COLOR_TEXT;
            markerColor = PARTS_COLOR_MARKER;
        }
        Sel_DrawParts(&part, 1, partColor);
        Font_SetColor(textColor);
        textY = (*pos)[1] + y;
        textX = (*pos)[0] + x;
        Font_SetPos(textX, textY);
        Text_PrintfColored(TXT(0x006CE168) /* a choice label and its number */, k + 1);
        markerX = textX - 30;
        if (k == selected && (gFrameTick & 0x10)) {
            markerX -= 4;
        }
        Parts_DrawMarker(markerX, textY - 2, 24, 24, 32768, markerColor, 255);
        src++;
        pos++;
    }
}

const char gPartsNumberFormat[11] = {
#include "select/006CE178.inc"
};

/*
 * The face page's panel: the face number and how many faces fit the
 * conditions (with arrows), then one row per search condition (skin colour,
 * then two that depend on the gender) with arrows around the value, and the
 * done row. The row under the cursor (gChrEdit.cursor) is highlighted and its
 * arrows move every 16 frames. sel is unused (Parts_DrawTypes's parameter).
 * The hair and voice panels below are the same code with other rows.
 */
void Parts_DrawFacePanel(s16 x, s16 y, s32 sel)
{
    ChrEdit *edit;
    char text[48];
    char buf[48];
    s16 i;
    s16 rowY;
    s32 color;
    s32 valueColor;
    s16 blinkOffset;
    char **label;
    s16 labelX;

    edit = &gChrEdit;
    Parts_DrawRotated(x, y, 334, 242, -2);
    blinkOffset = 0;
    if (gChrEdit.cursor == 0) {
        if (gFrameTick & 0x10) {
            blinkOffset = 6;
        }
        color = PARTS_COLOR_SELECTED;
    } else {
        color = PARTS_COLOR_TEXT;
    }
    Parts_DrawMarker(x + 55 - blinkOffset, y + 50, 30, 30, 0, PARTS_COLOR_MARKER, 0);
    Parts_DrawMarker(blinkOffset + (x + 235), y + 50, 30, 30, 32768, PARTS_COLOR_MARKER, 1);
    Font_SetSpacing(22, 22);
    Font_SetColor(color);
    sprintf(buf, gPartsNumberFormat, edit->face + 1, edit->matchCount);
    Sjis_Widen(buf, text);
    Font_SetPos(x + 101, y + 54);
    Text_PrintfColored(gPartsStrFormat, text);
    Font_SetSpacing(20, 20);
    for (i = 0; i < 4; i++) {
        rowY = i * 36 + 90;
        blinkOffset = 0;
        if (edit->cursor == i + 1) {
            color = PARTS_COLOR_SELECTED;
            if (gFrameTick & 0x10) {
                blinkOffset = 4;
            }
        } else {
            color = PARTS_COLOR_TEXT;
        }
        valueColor = color;
        if (edit->matchCount == 0) {
            valueColor = PARTS_COLOR_NO_MATCH;
        }
        if (i != 3) {
            Parts_DrawMarker(x + 130 - blinkOffset, rowY + y, 24, 24, 0, PARTS_COLOR_MARKER, i * 2 + 2);
            Parts_DrawMarker(blinkOffset + (x + 260), rowY + y, 24, 24, 32768, PARTS_COLOR_MARKER, i * 2 + 3);
        }
        Font_SetPos(x + 40, rowY + 2 + y);
        Font_SetColor(color);
        switch (i) {
        case 0:
            Text_PrintfColored(gPartsStrFormat, gGenderBackTexts[1]);
            label = &gPartsLabels[gPartsFaceLabels0[edit->cond[0]]];
            labelX = 210 - Str_Length(*label) / 2 * 10;
            Font_SetPos(labelX + x, rowY + 2 + y);
            Font_SetColor(valueColor);
            Text_PrintfColored(gPartsStrFormat, *label);
            break;
        case 1:
            if (edit->gender == 0) {
                Text_PrintfColored(gPartsStrFormat, gAgeFeatureTexts[0]);
            } else {
                Text_PrintfColored(gPartsStrFormat, gFaceHairRowTexts[0]);
            }
            label = &gPartsLabels[gPartsFaceLabels1[edit->cond[1] + edit->gender * 3]];
            labelX = 210 - Str_Length(*label) / 2 * 10;
            Font_SetPos(labelX + x, rowY + 2 + y);
            Font_SetColor(valueColor);
            Text_PrintfColored(gPartsStrFormat, *label);
            break;
        case 2:
            Text_PrintfColored(gPartsStrFormat, gAgeFeatureTexts[1]);
            label = &gPartsLabels[gPartsFaceLabels2[edit->cond[2] + edit->gender * 3]];
            labelX = 210 - Str_Length(*label) / 2 * 10;
            Font_SetPos(labelX + x, rowY + 2 + y);
            Font_SetColor(valueColor);
            Text_PrintfColored(gPartsStrFormat, *label);
            break;
        case 3:
            Text_PrintfColored(gPartsStrFormat, gOkTypeTexts[0]);
            break;
        }
    }
}

/* The hair page's panel: the hair style's number and its three conditions. */
void Parts_DrawHairPanel(s16 x, s16 y, s32 sel)
{
    ChrEdit *edit;
    char text[48];
    char buf[48];
    s16 i;
    s16 rowY;
    s32 color;
    s32 valueColor;
    s16 blinkOffset;
    char **label;
    s16 labelX;

    edit = &gChrEdit;
    Parts_DrawRotated(x, y, 334, 242, -2);
    blinkOffset = 0;
    if (gChrEdit.cursor == 0) {
        if (gFrameTick & 0x10) {
            blinkOffset = 6;
        }
        color = PARTS_COLOR_SELECTED;
    } else {
        color = PARTS_COLOR_TEXT;
    }
    Parts_DrawMarker(x + 50 - blinkOffset, y + 50, 30, 30, 0, PARTS_COLOR_MARKER, 0);
    Parts_DrawMarker(blinkOffset + (x + 230), y + 50, 30, 30, 32768, PARTS_COLOR_MARKER, 1);
    Font_SetSpacing(22, 22);
    Font_SetColor(color);
    sprintf(buf, gPartsNumberFormat, edit->hair + 1, edit->matchCount);
    Sjis_Widen(buf, text);
    Font_SetPos(x + 96, y + 54);
    Text_PrintfColored(gPartsStrFormat, text);
    Font_SetSpacing(20, 20);
    for (i = 0; i < 4; i++) {
        rowY = i * 36 + 90;
        blinkOffset = 0;
        if (edit->cursor == i + 1) {
            color = PARTS_COLOR_SELECTED;
            if (gFrameTick & 0x10) {
                blinkOffset = 4;
            }
        } else {
            color = PARTS_COLOR_TEXT;
        }
        valueColor = color;
        if (edit->matchCount == 0) {
            valueColor = PARTS_COLOR_NO_MATCH;
        }
        if (i != 3) {
            Parts_DrawMarker(x + 130 - blinkOffset, rowY + y, 24, 24, 0, PARTS_COLOR_MARKER, i * 2 + 2);
            Parts_DrawMarker(blinkOffset + (x + 260), rowY + y, 24, 24, 32768, PARTS_COLOR_MARKER, i * 2 + 3);
        }
        Font_SetPos(x + 40, rowY + 2 + y);
        Font_SetColor(color);
        switch (i) {
        case 0:
            Text_PrintfColored(gPartsStrFormat, gHairRowTexts[0]);
            label = &gPartsLabels[gPartsHairLabels0[edit->cond[3] + edit->gender * 3]];
            labelX = 210 - Str_Length(*label) / 2 * 10;
            Font_SetPos(labelX + x, rowY + 2 + y);
            Font_SetColor(valueColor);
            Text_PrintfColored(gPartsStrFormat, *label);
            break;
        case 1:
            Text_PrintfColored(gPartsStrFormat, gFaceHairRowTexts[1]);
            label = &gPartsLabels[gPartsHairLabels1[edit->cond[4]]];
            labelX = 210 - Str_Length(*label) / 2 * 10;
            Font_SetPos(labelX + x, rowY + 2 + y);
            Font_SetColor(valueColor);
            Text_PrintfColored(gPartsStrFormat, *label);
            break;
        case 2:
            Text_PrintfColored(gPartsStrFormat, gAgeFeatureTexts[1]);
            label = &gPartsLabels[gPartsHairLabels2[edit->cond[5] + edit->gender * 3]];
            labelX = 210 - Str_Length(*label) / 2 * 10;
            Font_SetPos(labelX + x, rowY + 2 + y);
            Font_SetColor(valueColor);
            Text_PrintfColored(gPartsStrFormat, *label);
            break;
        case 3:
            Text_PrintfColored(gPartsStrFormat, gOkTypeTexts[0]);
            break;
        }
    }
}

/* The voice page's panel: the voice's number and its two conditions. */
void Parts_DrawVoicePanel(s16 x, s16 y, s32 sel)
{
    ChrEdit *edit;
    char text[48];
    char buf[48];
    s16 i;
    s16 rowY;
    s32 color;
    s32 valueColor;
    s16 blinkOffset;
    char **label;
    s16 labelX;

    edit = &gChrEdit;
    Parts_DrawRotated(x, y, 334, 242, -2);
    blinkOffset = 0;
    if (gChrEdit.cursor == 0) {
        if (gFrameTick & 0x10) {
            blinkOffset = 6;
        }
        color = PARTS_COLOR_SELECTED;
    } else {
        color = PARTS_COLOR_TEXT;
    }
    Parts_DrawMarker(x + 50 - blinkOffset, y + 50, 30, 30, 0, PARTS_COLOR_MARKER, 0);
    Parts_DrawMarker(blinkOffset + (x + 230), y + 50, 30, 30, 32768, PARTS_COLOR_MARKER, 1);
    Font_SetSpacing(22, 22);
    Font_SetColor(color);
    sprintf(buf, gPartsNumberFormat, edit->voice + 1, edit->matchCount);
    Sjis_Widen(buf, text);
    Font_SetPos(x + 96, y + 54);
    Text_PrintfColored(gPartsStrFormat, text);
    Font_SetSpacing(20, 20);
    for (i = 0; i < 3; i++) {
        rowY = i * 40 + 100;
        blinkOffset = 0;
        if (edit->cursor == i + 1) {
            color = PARTS_COLOR_SELECTED;
            if (gFrameTick & 0x10) {
                blinkOffset = 4;
            }
        } else {
            color = PARTS_COLOR_TEXT;
        }
        valueColor = color;
        if (edit->matchCount == 0) {
            valueColor = PARTS_COLOR_NO_MATCH;
        }
        if (i != 2) {
            Parts_DrawMarker(x + 130 - blinkOffset, rowY + y, 24, 24, 0, PARTS_COLOR_MARKER, i * 2 + 2);
            Parts_DrawMarker(blinkOffset + (x + 260), rowY + y, 24, 24, 32768, PARTS_COLOR_MARKER, i * 2 + 3);
        }
        Font_SetPos(x + 40, rowY + 2 + y);
        Font_SetColor(color);
        switch (i) {
        case 0:
            Text_PrintfColored(gPartsStrFormat, gAgeFeatureTexts[0]);
            label = &gPartsLabels[gPartsVoiceLabels0[edit->cond[6] + edit->gender * 4]];
            labelX = 210 - Str_Length(*label) / 2 * 10;
            Font_SetPos(labelX + x, rowY + 2 + y);
            Font_SetColor(valueColor);
            Text_PrintfColored(gPartsStrFormat, *label);
            break;
        case 1:
            Text_PrintfColored(gPartsStrFormat, gAgeFeatureTexts[1]);
            label = &gPartsLabels[gPartsVoiceLabels1[edit->cond[7] + edit->gender * 3]];
            labelX = 210 - Str_Length(*label) / 2 * 10;
            Font_SetPos(labelX + x, rowY + 2 + y);
            Font_SetColor(valueColor);
            Text_PrintfColored(gPartsStrFormat, *label);
            break;
        case 2:
            Text_PrintfColored(gPartsStrFormat, gOkTypeTexts[0]);
            break;
        }
    }
}
