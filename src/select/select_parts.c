/*
 * select: the parts lists of character creation (face, hair, colours, the
 * search conditions) (0x6BCE20-0x6BF3B0). The file owns its .data
 * (0x6CB340-0x6CB758) and .rodata (0x6CE0F0-0x6CE188).
 */
#include "common.h"
#include "syssettings.h"
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "m2c_macros.h"
#include "select.h"
#include "select/select_parts.h"   /* the texts this file uses (tools/content_gen.py) */

/* gPartsPanels's records: a panel of character creation (Parts_DrawPanels). */
typedef struct PartsRec {
    s16 x, y, w, h;         /* 0x00 */
    s32 color;              /* 0x08 */
    u16 part;               /* 0x0C: its SelPart in gPartsCursorParts */
    s16 tx, ty;             /* 0x0E: the label */
    s16 tx2, ty2;           /* 0x12: the value */
    s16 unk16;
} PartsRec;

struct TexRect;
void TexRect_DrawRotated(struct TexRect *r, s32 angle);  /* draws a rotated sprite */
void Text_PrintfColored(const char *fmt, ...);
void Parts_DrawRotated(s16 x, s16 y, s16 w, s16 h, s32 deg);
void Parts_DrawMarker(s16 x, s16 y, s16 w, s16 h, s32 angle, s32 color, u8 idx);

/* .data */
u32 gPartsSprites[10] = {
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
void Sel_DrawPartsRotated(SelPart *p, s32 n, u32 color, s32 angle);

SelPart gPartsPieces[8] = {
#include "select/006CB420.inc"
};
u32 gPartsCursorParts[24] = {
#include "select/006CB4B0.inc"
};
u32 gPartsTypeSprites[20] = {
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
/* The search conditions of the FACE, HAIR and VOICE pages (ChrEdit bytes
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


void Parts_Init(void) {
    CharaWork *c;

    c = &((SelWork *)gSysSettings.workArea)->chara;
    memset(c, 0, sizeof(CharaWork));
    c->slide[0] = 2;
    c->slide[1] = 6;
    c->slide[4] = 0x24;
}

void Parts_DrawSwatches(s16 kind, s16 alt) {
    s16 t;
    Sprite spr;
    s16 n;
    u32 color;
    s16 i;
    s16 *pos;
    s16 *tex;

    Gfx_SetState63(1);
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    if (kind == 0) {
        n = 4;
        if (alt == 0) {
            color = 0xFF3F8EEA;
            pos = gPartsSwatchPosA[0];
        } else {
            color = 0xFF254B61;
            pos = gPartsSwatchPosB[0];
        }
        tex = &gPartsSwatchTex[2];
    } else {
        n = 6;
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
        spr.w = 0x20;
    } else {
        spr.w = 0x18;
    }
    spr.h = spr.w;
    for (i = 0; i < n; i++) {
        spr.x = pos[0];
        spr.y = pos[1];
        pos += 2;
        t = *tex++;
        spr.u0 = (t & 7) << 5;
        spr.v0 = (t >> 3) << 5;
        spr.u1 = spr.u0 + 0x1F;
        spr.v1 = spr.v0 + 0x1F;
        Sprite_Draw(&spr);
    }
}

extern void *gWorkArea; /* sub_main: the overlays' shared work area */
extern char *gOkTypeTexts[];  /* messages (select_psp.c) */
extern char *gGenderBackTexts[];
extern char *gAgeFeatureTexts[];
extern char *D_006CD8F0[];
extern char *D_006CD8F8[];
extern char *gPartsLabels[];
s32 sprintf(char *, const char *, ...);    /* sprintf */
void Sjis_Widen(char *src, char *dst);
s32 Str_Length(char *);                /* a message's width */

/* The two side panels of character creation sliding in (states 0-5 per
 * side in CharaWork bytes 0/1, timers in unk58[0]/[1]), then the colour
 * swatches (state 7); returns 1 once the shared delay unk58[4] is over.
 * k is one variable in two roles: the delay read before the loop, then
 * the side's index (a reused variable's second web is coloured before the
 * temporaries, which gives it v0 and the state address v1). Parts_DrawSwatches
 * taking s16 parameters is what gives the game's `move a0, s2` for i. */
s32 Parts_DrawSides(TaskSlot *p) {
    ChrEdit *e;
    CharaWork *c;
    SelWork *base;
    s16 busy;
    s32 k;
    s32 color;
    s16 i;
    u16 angle;
    s16 alt;
    SelPart part;

    e = &gChrEdit;
    base = gWorkArea;
    c = &base->chara;
    busy = 0;
    k = c->slide[4];
    if (k != 0) {
        busy = 1;
        c->slide[4] = k - 1;
    }
    for (i = 0; i < 2; i++) {
        part = ((SelPart *)gPartsSprites)[i];
        k = i;
        color = (i == 0) ? 0xFF3F8EEA : 0xFFBC1D0D;
        angle = 0;
        switch (((s8 *)c)[k]) {
        case 0:
            c->slide[i]--;
            if (c->slide[i] <= 0) {
                ((s8 *)c)[k]++;
                c->slide[i] = 8;
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
            c->slide[i]--;
            switch (((s8 *)c)[k]) {
            case 1:
                part.y -= (s16)(c->slide[i] * 40);
                break;
            case 2:
                part.y -= (s16)((4 - c->slide[i]) * 4);
                break;
            case 3:
                part.y -= (s16)(c->slide[i] * 4);
                break;
            case 4:
                part.y -= (s16)((2 - c->slide[i]) * 3);
                break;
            case 5:
                part.y -= (s16)(c->slide[i] * 3);
                break;
            }
            if (c->slide[i] <= 0) {
                ((s8 *)c)[k]++;
                if (((s8 *)c)[k] >= 4) {
                    c->slide[i] = 2;
                } else {
                    c->slide[i] = 4;
                }
            }
            break;
        case 6:
            if (c->slide[4] == 0) {
                ((s8 *)c)[k]++;
                if (i == 0) {
                    Sound_PlaySeFlat(0, 0x12, 0);
                }
            }
            break;
        case 7:
            alt = 1;
            if (e->gender != i) {
                if (!k) {
                    color = 0xFF254B61;
                    angle = -0x1C71;
                    part.x = 0x71;
                    part.y = 0x8B;
                    part.w = 0x80;
                    part.h = 0x8C;
                } else {
                    color = 0xFF47050B;
                    angle = 0x1C72;
                    part.x = 0x189;
                    part.y = 0x8B;
                    part.w = 0x80;
                    part.h = 0x8C;
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

void Chara_StartTabAnim(u8 sel) {
    SelWork *base;
    CharaWork *c;
    s16 i;
    s32 n;

    base = (SelWork *)gSysSettings.workArea;
    base->chara.slideOut = 0;
    c = &base->chara;
    Sound_PlaySeFlat(1, 0xB, 0);
    for (i = 0, n = 10; i < 7; i++, n += 2) {
        if (i == sel) {
            c->slide[i] = 0;
            c->sliding[i] = 0;
        } else {
            c->slide[i] = n;
            c->sliding[i] = 1;
        }
    }
}

void Chara_StartTabAnim2(u8 sel) {
    SelWork *base;
    CharaWork *c;
    s16 i;
    s16 n;

    base = (SelWork *)gSysSettings.workArea;
    n = 0;
    base->chara.slideOut = 1;
    c = &base->chara;
    for (i = 0; i < 7; i++) {
        c->slide[i] = 0;
        if (i == sel) {
            c->slideDelay[i] = 0;
            c->sliding[i] = 0;
        } else {
            c->slideDelay[i] = n;
            c->sliding[i] = 1;
            n += 2;
        }
    }
}

void Parts_StartBounce(u8 i) {
    CharaWork *c;

    c = &((SelWork *)gSysSettings.workArea)->chara;
    c->bounce[i] = 1;
    c->bounceOfs[i] = 0;
    Sound_PlaySeFlat(0, 0x16, 0);
}

void Parts_ResetBounces(void) {
    s16 i;
    CharaWork *c;

    c = &((SelWork *)gSysSettings.workArea)->chara;
    for (i = 0; i < 10; i++) {
        c->bounce[i] = 0;
        c->bounceOfs[i] = 0;
    }
}

void Parts_StartPanel(s16 flag) {
    CharaWork *c;
    SelWork *base;
    s16 v;

    base = (SelWork *)gSysSettings.workArea;
    c = &base->chara;
    v = gPartsPanels[gChrEdit.panel][1];
    base->chara.cursorJump = 1;
    base->chara.cursorJumpFrame = 0;
    if (flag == 0) {
        c->cursorY = v;
        c->cursorYEnd = gPartsPanels[0][1];
    } else {
        c->cursorY = gPartsPanels[0][1];
        c->cursorYEnd = v;
    }
}

f32 Math_Absf(f32);   /* fabsf */

/* Character creation's animations: the parts lists' bounces, the panel
 * sliding in and out, the cursor blink and the entries' fade. The bounce
 * loop reads its two bytes through `e = (u8 *)c + k` declared first
 * (unk184[k] and unk18E[k] written out swap the registers of k and the
 * address; a typed `s8 *` pointer changes the code). */
void Parts_Animate(void) {
    u8 *e;
    CharaWork *c;
    s16 i;
    s32 k;

    c = &((SelWork *)gSysSettings.workArea)->chara;
    for (i = 0; i < 10; i++) {
        k = i;
        e = (u8 *)c + k;
        switch (((s8 *)e)[0x184]) {
        case 0:
            break;
        case 1:
            if (k & 1) {
                ((s8 *)e)[0x18E]++;
            } else {
                ((s8 *)e)[0x18E]--;
            }
            if (Math_Absf(((s8 *)e)[0x18E]) >= 5.0f) {
                ((s8 *)e)[0x184] = 2;
            }
            break;
        case 2:
            if (k & 1) {
                ((s8 *)e)[0x18E]--;
                if (((s8 *)e)[0x18E] <= 0) {
                    ((s8 *)e)[0x184] = 0;
                }
            } else {
                ((s8 *)e)[0x18E]++;
                if (((s8 *)e)[0x18E] >= 0) {
                    ((s8 *)e)[0x184] = 0;
                }
            }
            break;
        }
    }
    switch (c->cursorJump) {
    case 0:
        break;
    case 1:
        c->cursorY -= 0x1C;
        c->cursorJumpFrame++;
        if (c->cursorJumpFrame >= 0xC) {
            c->cursorJump = 2;
        }
        break;
    case 2:
        c->cursorY += 0x24;
        if (c->cursorY >= c->cursorYEnd) {
            c->cursorY = c->cursorYEnd;
            c->cursorJump = 0;
        }
        break;
    }
    if (!(gFrameTick & 3)) {
        c->blink++;
        if (c->blink > 2) {
            c->blink = 0;
        }
    }
    for (i = 0; i < 7; i++) {
        if (c->slideOut == 0) {
            if (c->slide[i] > 0) {
                c->slide[i]--;
            } else {
                c->sliding[i] = 0;
            }
        } else if (c->sliding[i] != 0) {
            if (c->slideDelay[i] > 0) {
                c->slideDelay[i]--;
                c->slide[i]++;
            } else {
                c->slide[i]++;
                if (c->slide[i] >= 10) {
                    c->sliding[i] = 0;
                }
            }
        }
    }
}

void Parts_DrawCursor(s16 x, s16 y, s32 unused) {
    SelWork *base;
    CharaWork *c;
    SelPart part;

    base = (SelWork *)gSysSettings.workArea;
    c = &base->chara;
    if (base->chara.cursorJump == 0) {
        part = gPartsPieces[c->blink + 2];
    } else {
        part = gPartsPieces[5];
    }
    part.x = x;
    if (c->cursorJump == 0) {
        part.y = y;
    } else {
        part.y = c->cursorY;
    }
    Sel_DrawParts(&part, 1, 0xFFFF6000);
}

const char D_006CE110[3] = {
#include "select/006CE110.inc"
};

extern char *gCharaPartTitles[];   /* the panels' names (select_psp.c) */

/* Character creation's seven panels (FACE, HAIR, ...), the chosen one
 * (ChrEdit byte 2) drawn last and lit: each with its rotated frame, its
 * name and value (TYPE%02d or the RGB colour) and, for the chosen one, the
 * cursor. The panels slide by 80 per unk58 step. Returns 1.
 * The chosen panel goes through k (`k = e[2]; order[6] = k;`, an earlier
 * web of k that puts k's loop web after x's), and the work area is read as
 * gSysSettings.workArea. */
s32 Parts_DrawPanels(TaskSlot *t) {
    CharaWork *c;
    s16 i;
    s16 j;
    ChrEdit *e;
    s16 x;
    s16 dy;
    s16 *rot;
    PartsRec *r;
    s32 size;
    s32 size2;
    u8 *op;
    SelPart part;
    u8 order[7];
    s32 k;

    e = &gChrEdit;
    c = &((SelWork *)gSysSettings.workArea)->chara;
    Gs_SetState(0x6C, 0);
    k = e->panel;
    order[6] = k;
    for (j = 6, i = 5; i >= 0; j--, i--) {
        if (j == e->panel) {
            j--;
        }
        order[i] = j;
    }
    for (i = 0, op = order; i < 7; op++, i++) {
        k = (s16)*op;
        dy = 0;
        x = c->slide[k] * 0x50;
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
            r = (PartsRec *)gPartsPanels[k];
            size = 0x18;
            size2 = 0x12;
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
            if (t->mode == 5) {
                dy = gPartsPanels[0][1] - r->y;
            }
        } else {
            r = (PartsRec *)gPartsPanels[k + 7];
            size = 0x14;
            size2 = 0x10;
            Font_SetColor(0xFF000000);
        }
        part = ((SelPart *)gPartsCursorParts)[r->part];
        part.x = r->x + x;
        if (part.x < 700) {
            part.y = r->y + dy;
            part.w = r->w;
            part.h = r->h;
            Sel_DrawParts(&part, 1, r->color);
            if (c->slide[k] == 0 && c->slideDelay[k] == 0) {
                Font_SetPos(r->tx + x, r->ty + dy);
                Font_SetSpacing(size, size);
                Text_PrintfColored(D_006CE110, gCharaPartTitles[k]);
                if (k != 6) {
                    Font_SetPos(r->tx2 + x, r->ty2 + dy);
                    Font_SetSpacing(size2, size2);
                    switch (k) {
                    case 0:
                        Text_PrintfColored(D_006CE110, e->name);
                        break;
                    case 1:
                        Text_PrintfColored(TXT(0x006CE118) /* a choice label and its number */, e->type + 1);
                        break;
                    case 2:
                        Text_PrintfColored(TXT(0x006CE118) /* a choice label and its number */, e->face + 1);
                        break;
                    case 5:
                        Text_PrintfColored(TXT(0x006CE118) /* a choice label and its number */, e->voice + 1);
                        break;
                    case 3:
                        Text_PrintfColored(TXT(0x006CE118) /* a choice label and its number */, e->hair + 1);
                        break;
                    case 4:
                        Text_PrintfColored("R%03d G%03d B%03d", (e->color >> 16) & 0xFF,
                                           (e->color >> 8) & 0xFF, e->color & 0xFF);
                        break;
                    }
                }
                if (i == 6) {
                    Parts_DrawCursor(gPartsCursorPos[k][0], dy + gPartsCursorPos[k][1], 0);
                }
            }
        }
    }
    Gs_SetState(0x6C, 1);
    return 1;
}

void Parts_DrawRotated(s16 x, s16 y, s16 w, s16 h, s32 deg) {
    SelPart p;

    p = gPartsPieces[6];
    p.x = x;
    p.y = y;
    p.w = w;
    p.h = h;
    Sel_DrawPartsRotated(&p, 1, -1, (u16)(s32)(0.5f + 65536.0f * deg / 360.0f));
}

void Parts_DrawMarker(s16 x, s16 y, s16 w, s16 h, s32 angle, s32 color, u8 idx) {
    CharaWork *c;
    Sprite spr;
    s32 col;

    c = &((SelWork *)gSysSettings.workArea)->chara;
    if (idx == 0xFF) {
        col = color;
    } else if (c->bounce[idx] == 0) {
        col = 0xFF342B2A;
    } else {
        col = 0xFFFB7103;
    }
    Tex_Prepare(1, 0x16B);
    Tex_Bind(0x16B);
    spr.u1 = 0xF;
    spr.v1 = 0xF;
    spr.x = x;
    spr.y = y;
    spr.w = w;
    spr.h = h;
    spr.color = col;
    spr.u0 = 0;
    spr.v0 = 0;
    TexRect_DrawRotated((struct TexRect *)&spr, angle);
}

/* The TYPE 01-04 list: the entry sel is highlighted, its marker blinks. */
void Parts_DrawTypes(s16 x, s16 y, s32 sel) {
    SelPart part;
    s32 pc;
    s16 bx;
    SelPart *src;
    s32 bc;
    s16 i;
    s16 (*pos)[2];
    s32 cur;
    s32 tx;
    s32 ty;
    s32 tc;
    s32 k;

    Parts_DrawRotated(x, y, 0x14E, 0xF2, -2);
    cur = (u8)sel;
    i = 0;
    src = (SelPart *)gPartsTypeSprites;
    pos = gPartsTypePos;
    for (; i < 4; i++) {
        part = *src;
        part.x += x;
        part.y += y;
        k = i;
        if (k == cur) {
            pc = -1;
            tc = 0xFF32DA5D;
            bc = 0xFFFB7103;
        } else {
            pc = 0xFF606060;
            tc = 0xFF2A2120;
            bc = 0xFF342B2A;
        }
        Sel_DrawParts(&part, 1, pc);
        Font_SetColor(tc);
        ty = (*pos)[1] + y;
        tx = (*pos)[0] + x;
        Font_SetPos(tx, ty);
        Text_PrintfColored(TXT(0x006CE168) /* a choice label and its number */, k + 1);
        bx = tx - 0x1E;
        if (k == cur && (gFrameTick & 0x10)) {
            bx -= 4;
        }
        Parts_DrawMarker(bx, ty - 2, 0x18, 0x18, 0x8000, bc, 0xFF);
        src++;
        pos++;
    }
}

const char D_006CE178[11] = {
#include "select/006CE178.inc"
};

/* The FACE page's panel: the face number and how many faces fit the
 * conditions ("NN(MM)", with arrows), then one row per search condition
 * (skin colour, then two that depend on the sex) with arrows around the
 * value, and OK. The row under the cursor (ChrEdit 0x5D) is highlighted and
 * its arrows blink. sel is unused (Parts_DrawTypes's parameter). The HAIR
 * and VOICE panels below are the same code with other rows. */
void Parts_DrawFacePanel(s16 x, s16 y, s32 sel) {
    ChrEdit *e;
    char text[0x30];
    char buf[0x30];
    s16 i;
    s16 ry;
    s32 color;
    s32 color2;
    s16 off;
    char **p;
    s16 tx;

    e = &gChrEdit;
    Parts_DrawRotated(x, y, 0x14E, 0xF2, -2);
    off = 0;
    if (gChrEdit.cursor == 0) {
        if (gFrameTick & 0x10) {
            off = 6;
        }
        color = 0xFF32DA5D;
    } else {
        color = 0xFF2A2120;
    }
    Parts_DrawMarker(x + 0x37 - off, y + 0x32, 0x1E, 0x1E, 0, 0xFF342B2A, 0);
    Parts_DrawMarker(off + (x + 0xEB), y + 0x32, 0x1E, 0x1E, 0x8000, 0xFF342B2A, 1);
    Font_SetSpacing(0x16, 0x16);
    Font_SetColor(color);
    sprintf(buf, D_006CE178, e->face + 1, e->matchCount);
    Sjis_Widen(buf, text);
    Font_SetPos(x + 0x65, y + 0x36);
    Text_PrintfColored(D_006CE110, text);
    Font_SetSpacing(0x14, 0x14);
    for (i = 0; i < 4; i++) {
        ry = i * 0x24 + 0x5A;
        off = 0;
        if (e->cursor == i + 1) {
            color = 0xFF32DA5D;
            if (gFrameTick & 0x10) {
                off = 4;
            }
        } else {
            color = 0xFF2A2120;
        }
        color2 = color;
        if (e->matchCount == 0) {
            color2 = 0xFFFFC000;
        }
        if (i != 3) {
            Parts_DrawMarker(x + 0x82 - off, ry + y, 0x18, 0x18, 0, 0xFF342B2A, i * 2 + 2);
            Parts_DrawMarker(off + (x + 0x104), ry + y, 0x18, 0x18, 0x8000, 0xFF342B2A, i * 2 + 3);
        }
        Font_SetPos(x + 0x28, ry + 2 + y);
        Font_SetColor(color);
        switch (i) {
        case 0:
            Text_PrintfColored(D_006CE110, gGenderBackTexts[1]);
            p = &gPartsLabels[gPartsFaceLabels0[e->cond[0]]];
            tx = 0xD2 - Str_Length(*p) / 2 * 10;
            Font_SetPos(tx + x, ry + 2 + y);
            Font_SetColor(color2);
            Text_PrintfColored(D_006CE110, *p);
            break;
        case 1:
            if (e->gender == 0) {
                Text_PrintfColored(D_006CE110, gAgeFeatureTexts[0]);
            } else {
                Text_PrintfColored(D_006CE110, D_006CD8F8[0]);
            }
            p = &gPartsLabels[gPartsFaceLabels1[e->cond[1] + e->gender * 3]];
            tx = 0xD2 - Str_Length(*p) / 2 * 10;
            Font_SetPos(tx + x, ry + 2 + y);
            Font_SetColor(color2);
            Text_PrintfColored(D_006CE110, *p);
            break;
        case 2:
            Text_PrintfColored(D_006CE110, gAgeFeatureTexts[1]);
            p = &gPartsLabels[gPartsFaceLabels2[e->cond[2] + e->gender * 3]];
            tx = 0xD2 - Str_Length(*p) / 2 * 10;
            Font_SetPos(tx + x, ry + 2 + y);
            Font_SetColor(color2);
            Text_PrintfColored(D_006CE110, *p);
            break;
        case 3:
            Text_PrintfColored(D_006CE110, gOkTypeTexts[0]);
            break;
        }
    }
}

/* The HAIR page: length, bangs and decoration. */
void Parts_DrawHairPanel(s16 x, s16 y, s32 sel) {
    ChrEdit *e;
    char text[0x30];
    char buf[0x30];
    s16 i;
    s16 ry;
    s32 color;
    s32 color2;
    s16 off;
    char **p;
    s16 tx;

    e = &gChrEdit;
    Parts_DrawRotated(x, y, 0x14E, 0xF2, -2);
    off = 0;
    if (gChrEdit.cursor == 0) {
        if (gFrameTick & 0x10) {
            off = 6;
        }
        color = 0xFF32DA5D;
    } else {
        color = 0xFF2A2120;
    }
    Parts_DrawMarker(x + 0x32 - off, y + 0x32, 0x1E, 0x1E, 0, 0xFF342B2A, 0);
    Parts_DrawMarker(off + (x + 0xE6), y + 0x32, 0x1E, 0x1E, 0x8000, 0xFF342B2A, 1);
    Font_SetSpacing(0x16, 0x16);
    Font_SetColor(color);
    sprintf(buf, D_006CE178, e->hair + 1, e->matchCount);
    Sjis_Widen(buf, text);
    Font_SetPos(x + 0x60, y + 0x36);
    Text_PrintfColored(D_006CE110, text);
    Font_SetSpacing(0x14, 0x14);
    for (i = 0; i < 4; i++) {
        ry = i * 0x24 + 0x5A;
        off = 0;
        if (e->cursor == i + 1) {
            color = 0xFF32DA5D;
            if (gFrameTick & 0x10) {
                off = 4;
            }
        } else {
            color = 0xFF2A2120;
        }
        color2 = color;
        if (e->matchCount == 0) {
            color2 = 0xFFFFC000;
        }
        if (i != 3) {
            Parts_DrawMarker(x + 0x82 - off, ry + y, 0x18, 0x18, 0, 0xFF342B2A, i * 2 + 2);
            Parts_DrawMarker(off + (x + 0x104), ry + y, 0x18, 0x18, 0x8000, 0xFF342B2A, i * 2 + 3);
        }
        Font_SetPos(x + 0x28, ry + 2 + y);
        Font_SetColor(color);
        switch (i) {
        case 0:
            Text_PrintfColored(D_006CE110, D_006CD8F0[0]);
            p = &gPartsLabels[gPartsHairLabels0[e->cond[3] + e->gender * 3]];
            tx = 0xD2 - Str_Length(*p) / 2 * 10;
            Font_SetPos(tx + x, ry + 2 + y);
            Font_SetColor(color2);
            Text_PrintfColored(D_006CE110, *p);
            break;
        case 1:
            Text_PrintfColored(D_006CE110, D_006CD8F8[1]);
            p = &gPartsLabels[gPartsHairLabels1[e->cond[4]]];
            tx = 0xD2 - Str_Length(*p) / 2 * 10;
            Font_SetPos(tx + x, ry + 2 + y);
            Font_SetColor(color2);
            Text_PrintfColored(D_006CE110, *p);
            break;
        case 2:
            Text_PrintfColored(D_006CE110, gAgeFeatureTexts[1]);
            p = &gPartsLabels[gPartsHairLabels2[e->cond[5] + e->gender * 3]];
            tx = 0xD2 - Str_Length(*p) / 2 * 10;
            Font_SetPos(tx + x, ry + 2 + y);
            Font_SetColor(color2);
            Text_PrintfColored(D_006CE110, *p);
            break;
        case 3:
            Text_PrintfColored(D_006CE110, gOkTypeTexts[0]);
            break;
        }
    }
}

/* The VOICE page: age and character (three rows). */
void Parts_DrawVoicePanel(s16 x, s16 y, s32 sel) {
    ChrEdit *e;
    char text[0x30];
    char buf[0x30];
    s16 i;
    s16 ry;
    s32 color;
    s32 color2;
    s16 off;
    char **p;
    s16 tx;

    e = &gChrEdit;
    Parts_DrawRotated(x, y, 0x14E, 0xF2, -2);
    off = 0;
    if (gChrEdit.cursor == 0) {
        if (gFrameTick & 0x10) {
            off = 6;
        }
        color = 0xFF32DA5D;
    } else {
        color = 0xFF2A2120;
    }
    Parts_DrawMarker(x + 0x32 - off, y + 0x32, 0x1E, 0x1E, 0, 0xFF342B2A, 0);
    Parts_DrawMarker(off + (x + 0xE6), y + 0x32, 0x1E, 0x1E, 0x8000, 0xFF342B2A, 1);
    Font_SetSpacing(0x16, 0x16);
    Font_SetColor(color);
    sprintf(buf, D_006CE178, e->voice + 1, e->matchCount);
    Sjis_Widen(buf, text);
    Font_SetPos(x + 0x60, y + 0x36);
    Text_PrintfColored(D_006CE110, text);
    Font_SetSpacing(0x14, 0x14);
    for (i = 0; i < 3; i++) {
        ry = i * 0x28 + 0x64;
        off = 0;
        if (e->cursor == i + 1) {
            color = 0xFF32DA5D;
            if (gFrameTick & 0x10) {
                off = 4;
            }
        } else {
            color = 0xFF2A2120;
        }
        color2 = color;
        if (e->matchCount == 0) {
            color2 = 0xFFFFC000;
        }
        if (i != 2) {
            Parts_DrawMarker(x + 0x82 - off, ry + y, 0x18, 0x18, 0, 0xFF342B2A, i * 2 + 2);
            Parts_DrawMarker(off + (x + 0x104), ry + y, 0x18, 0x18, 0x8000, 0xFF342B2A, i * 2 + 3);
        }
        Font_SetPos(x + 0x28, ry + 2 + y);
        Font_SetColor(color);
        switch (i) {
        case 0:
            Text_PrintfColored(D_006CE110, gAgeFeatureTexts[0]);
            p = &gPartsLabels[gPartsVoiceLabels0[e->cond[6] + e->gender * 4]];
            tx = 0xD2 - Str_Length(*p) / 2 * 10;
            Font_SetPos(tx + x, ry + 2 + y);
            Font_SetColor(color2);
            Text_PrintfColored(D_006CE110, *p);
            break;
        case 1:
            Text_PrintfColored(D_006CE110, gAgeFeatureTexts[1]);
            p = &gPartsLabels[gPartsVoiceLabels1[e->cond[7] + e->gender * 3]];
            tx = 0xD2 - Str_Length(*p) / 2 * 10;
            Font_SetPos(tx + x, ry + 2 + y);
            Font_SetColor(color2);
            Text_PrintfColored(D_006CE110, *p);
            break;
        case 2:
            Text_PrintfColored(D_006CE110, gOkTypeTexts[0]);
            break;
        }
    }
}
