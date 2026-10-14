/*
 * select: the title menu (NEW GAME, CONTINUE, GALLERY, OPTIONS, NETWORK
 * OPTIONS, CONNECT) and MODE SELECT (0x6B9B10-0x6BCE20); Sel_DrawHeader
 * draws the menu headers for every screen of the overlay. The file owns its
 * .data (0x6CACC0-0x6CB340) and .rodata (0x6CDFD8-0x6CE0F0).
 */
#include "common.h"
#include "window.h"
#include "syssettings.h"
#include "m2c_macros.h"
#include "select.h"
#include "syssave.h"

/* Compiled at optimization level 4: Sel_DrawHeader needs it, and every other
 * function of the file matches at both levels. */
#pragma optimization_level 4

/* A sprite record with its texture (gCharaSelSlotIcons: the save slots' frames). */
typedef struct SelIcon {
    s32 tex;                /* 0x00: index into gTexHandles */
    s16 x, y;               /* 0x04: offsets */
    s16 w, h;               /* 0x08 */
    s16 u0, v0, u1, v1;     /* 0x0C */
    u32 color;              /* 0x14 */
    u16 angle;              /* 0x18 */
    u16 unk1A;
    f32 unk1C;              /* 0x1C: scale */
} SelIcon;

/* What 0x933380 points to while the title runs (the title background's
 * camera?). */
typedef struct TitleBg {
    u8 unk0[0x20];
    s32 unk20;              /* 0x20 */
    s32 unk24;              /* 0x24: the entry drawn highlighted */
    s32 sel;                /* 0x28: the menu entry */
    u8 unk2C[0x10];
    s32 frame;              /* 0x3C: 0-3 */
    s32 count;              /* 0x40 */
    u8 unk44[0xC];
    s32 unk50;              /* 0x50: the logo's state */
    u32 unk54;              /* 0x54: the highlighted entry's text colour */
    u32 unk58;              /* 0x58: ... the others' */
    f32 unk5C;              /* 0x5C: the logo's scale */
    s32 unk60;              /* 0x60 */
    s32 unk64;              /* 0x64 */
    s32 unk68;              /* 0x68 */
    s32 unk6C;              /* 0x6C */
} TitleBg;

/* The shared work area at 0x1000 as the character selection sees it. */
typedef struct CharaSelItem {
    s32 kind;               /* 0x00: 0xFF = empty */
    s32 unk4;
} CharaSelItem;

typedef struct CharaSelPos {
    f32 x, y;
} CharaSelPos;

typedef struct CharaSelWork {
    s8 state;               /* 0x000 */
    s8 unk1;                /* 0x001: entries that finished sliding */
    u8 unk2[2];
    s32 unk4;               /* 0x004 */
    s16 unk8;               /* 0x008: also gChrEdit's byte 1 */
    u8 unkA[0xE];
    s32 tex[16];            /* 0x018: texture handles */
    s32 unk58[1];           /* 0x058: per entry, a delay (from 0x58) */
    s32 unk5C[3];           /* 0x05C */
    u8 unk68[0xC];
    s32 unk74[5];           /* 0x074: per entry, 1-6 the slide state */
    u8 unk88[8];
    CharaSelItem items[5];  /* 0x090 */
    CharaSelPos items2[5];  /* 0x0B8 */
    CharaSelPos unkE0[5];   /* 0x0E0 */
    CharaSelPos unk108[5];  /* 0x108 */
    s32 order[5];           /* 0x130 */
    s32 count;              /* 0x144 */
    s32 unk148[3];          /* 0x148 */
    f32 unk154[3];          /* 0x154 */
    u8 unk160[0x1C];
    s32 unk17C;             /* 0x17C */
    s32 unk180;             /* 0x180 */
    u8 unk184[0x20];
} CharaSelWork;

/* The shared work area as CHARACTER SELECT sees it. */
typedef struct CharaSelBase {
    u8 unk0[0x1000];
    CharaSelWork w;         /* 0x1000 */
} CharaSelBase;

s32 Gs_GetState(u8 reg);

/* ... and as MODE SELECT sees it. */
typedef struct ModeItem {
    s32 kind;               /* 0x00: 0xFF = empty */
    s32 unk4;
} ModeItem;

typedef struct ModeWork {
    s8 state;               /* 0x000 */
    u8 unk1[3];
    s16 sel;                /* 0x004 */
    u8 unk6[6];
    s32 tex[16];            /* 0x00C: texture handles */
    u8 unk4C[0x14];
    s32 unk60[5];           /* 0x060 */
    ModeItem items[5];      /* 0x074 */
    s32 order[5];           /* 0x09C */
    s32 count;              /* 0x0B0 */
    f32 unkB4[2][2];        /* 0x0B4 */
    f32 unkC4[2];           /* 0x0C4 */
    f32 unkCC[8][2];        /* 0x0CC */
    f32 unk10C[2];          /* 0x10C: the buttons' slide speeds */
    u32 unk114[2];          /* 0x114: ... their phases */
    u32 unk11C[2];          /* 0x11C: ... and phase steps */
    s32 unk124;             /* 0x124 */
    s32 unk128;             /* 0x128 */
} ModeWork;

typedef struct ModeBase {
    u8 unk0[0x1000];
    ModeWork m;             /* 0x1000 */
} ModeBase;

void ModeSel_DrawButton(s32 *item);
void func_006BB690(void);
void ModeSel_DrawButtons(s32 sel, s32 arg1);
struct Frame;
void WindowFrame_Draw(struct Frame *f, s16 *off);
void CharaSel_DrawSlots(void);
void CharaSel_DrawEntry(s32 *item, CharaSelPos *pos);

extern s32 gOvlTex[];
s32 LoadCompressedFile(u8 *dst, s32 id);
s32 Pack_Count(s32 *buf);
void *Pack_GetPart(u8 *buf, s32 a);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
struct TexRect;
void TexRect_DrawRotated(struct TexRect *, u16); /* unit's own declaration: with the definition's void (TexRect *r, s32 angle) (sub_main/unk_363550.c), Sel_DrawPartsRotated 2 of 70 words differ */

extern void *gOptWork;
extern void *gWorkArea; /* sub_main: the overlays' shared work area */
void Title_DrawMenu(void *unused);
void Sel_DrawHeader(s16 kind);
extern M2C_UNK D_5B3680;
void Option_Start(TaskSlot *t, u16 net); /* select_option.c: t is a TaskSlot * */
void Option_Exit(void *t, s32 net);
void NetOption_Update(TaskSlot *t, s32 unused); /* select_option.c: t is a TaskSlot * */
void Gfx_SetBlend(s32, s32);
void Frame_Draw(void);
void Mem_Compact(void);
void Tex_FreeRange(s32 first, s32 count);

/* .data */
u32 gSelHeaderTexts[16] = {
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
u32 gModeSelIcons[26] = {
#include "select/006CAD80.inc"
};
u32 gModeSelStartParams[2] = {
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
u32 gModeSelButtonRects[20] = {
#include "select/006CAE20.inc"
};
u32 gModeSelLayoutA[4] = {
#include "select/006CAE70.inc"
};
u32 gModeSelLayoutB[16] = {
#include "select/006CAE80.inc"
};
u32 gCharaSelEntryIcons[56] = {
#include "select/006CAEC0.inc"
};
SelIcon gCharaSelSlotIcons[3] = {
#include "select/006CAFA0.inc"
};
u32 gCharaSelStartBytes[4] = {
#include "select/006CB000.inc"
};
u32 gCharaSelStartPos[4] = {
#include "select/006CB010.inc"
};
u32 gCharaSelInitA[4] = {
#include "select/006CB020.inc"
};
s8 gCharaSelSlotOrders[5][3] = {
#include "select/006CB030.inc"
};
u32 gCharaSelInitB[6] = {
#include "select/006CB040.inc"
};
u32 gCharaSelOrder[3] = {
#include "select/006CB058.inc"
};
u32 gCharaSelInitF[10] = {
#include "select/006CB070.inc"
};
u32 gCharaSelEntryColors[3] = {
#include "select/006CB098.inc"
};
u32 gCharaSelSlotPos[12] = {
#include "select/006CB0B0.inc"
};
/* The title logo's two parts. */
struct {
    SelPart p[2];
    u8 pad[12];
} gTitleLogoParts = {
#include "select/006CB0E0.inc"
};
u32 gTitleDigitParts[136] = {
#include "select/006CB110.inc"
};
u32 gTitleLogoScales[4] = {
#include "select/006CB330.inc"
};


extern TitleBg *gGmWork;
void Title_ResetBgAnim(void) {
    gGmWork->frame = 0;
    gGmWork->count = 0;
}

void Title_StepBgAnim(void) {
    TitleBg *bg;

    bg = gGmWork;
    bg->frame = (bg->frame + 1) & 3;
    bg = gGmWork;
    if (bg->frame == 0) {
        bg->count++;
    }
}

u16 Rand_Next(s32 stream);

/* A random view for the title background. */
void Title_RandomView(void) {
    s32 d;

    d = Rand_Next(1) % 9 * 6;
    if (Rand_Next(1) & 1) {
        d = -d;
    }
    gGmWork->unk60 = d + 0xB8;
    d = Rand_Next(1) % 7 * 6;
    if (Rand_Next(1) & 1) {
        d = -d;
    }
    gGmWork->unk64 = d + 0x64;
    d = Rand_Next(1) % 6 * 5;
    if (Rand_Next(1) & 1) {
        d = 0x167 - d;
    }
    gGmWork->unk68 = (u16)(s32)(0.5f + 65536.0f * d / 360.0f);
    gGmWork->unk6C = 0xFF505050;
}

void Title_Draw(void) {
    Sel_DrawHeader(0);
    Title_DrawMenu(&D_5B3680);
}

void Sel_DrawParts(SelPart *p, s32 n, u32 color) {
    Sprite spr;
    s32 i;

    for (i = 0; i < n; i++, p++) {
        Gfx_SetState63(p->flag);
        Tex_Prepare(1, p->tex);
        Tex_Bind(p->tex);
        spr.x = p->x;
        spr.y = p->y;
        spr.w = p->w;
        spr.h = p->h;
        spr.color = color;
        spr.u0 = p->u0;
        spr.v0 = p->v0;
        spr.u1 = p->u1;
        spr.v1 = p->v1;
        Sprite_Draw(&spr);
    }
}

void Sel_DrawPartsRotated(SelPart *p, s32 n, u32 color, s32 angle) {
    Sprite spr;
    s32 i;

    for (i = 0; i < n; i++, p++) {
        Gfx_SetState63(p->flag);
        Tex_Prepare(1, p->tex);
        Tex_Bind(p->tex);
        spr.x = p->x;
        spr.y = p->y;
        spr.w = p->w;
        spr.h = p->h;
        spr.color = color;
        spr.u0 = p->u0;
        spr.v0 = p->v0;
        spr.u1 = p->u1;
        spr.v1 = p->v1;
        TexRect_DrawRotated((struct TexRect *)&spr, angle);
    }
}

void Title_DrawDigitScaled(s32 digit, s32 kind, f32 scale, s32 color, s32 y, s32 x) {
    SelPart part;

    if (digit > 9) {
        digit %= 10;
    }
    part = ((SelPart *)gTitleDigitParts)[kind + digit * 3];
    if (scale > 1.0f) {
        part.x = x + 0x1C;
    } else {
        part.x = x + 0x64;
    }
    part.y = y;
    part.w = (u16)(part.w * scale);
    part.h = (u16)(part.h * scale);
    Sel_DrawParts(&part, 1, color);
}

/* The title logo (scaled around its centre by the state's scale) and its
 * rotating second part. The fields are written through pointers (the game
 * keeps them in s0-s3 for the whole function; py is set just before its
 * first use). */
void Title_DrawLogo(void) {
    SelPart part;
    s32 k;
    s16 w;
    s16 h;
    s16 sw;
    s16 sh;
    u32 c;
    u32 color;
    TitleBg *bg;
    s16 *px;
    s16 *py;
    s16 *pw;
    s16 *ph;

    bg = gGmWork;
    if (bg->unk20 != 0) {
        k = 2;
    } else {
        k = bg->unk50;
        if (k == 0) {
            k = 2;
        } else {
            k--;
            if (k >= 3) {
                k = 2;
            }
        }
    }
    bg->unk5C = ((f32 *)gTitleLogoScales)[k];
    Gs_SetState(0x6C, 0);
    Gfx_SetBlend(4, 5);
    part = gTitleLogoParts.p[0];
    pw = &part.w;
    ph = &part.h;
    px = &part.x;
    sw = *pw;
    sh = *ph;
    w = sw * gGmWork->unk5C;
    h = sh * gGmWork->unk5C;
    *px += (s16)((sw - w) / 2);
    py = &part.y;
    *py += (s16)((sh - h) / 2);
    *pw = w;
    *ph = h;
    Sel_DrawParts(&part, 1, -1);
    if (gGmWork->unk50 != 0) {
        Gs_SetState(0xD, 0x800);
        Gs_SetState(0x5E, 0x11);
        if (k == 0) {
            c = gGmWork->unk6C;
            color = (c & 0xFF000000) | (((c & 0xFF0000) >> 1) & 0xFF0000) | (((c & 0xFF00) >> 1) & 0xFF00) |
                    (((c & 0xFF) >> 1) & 0xFF);
        } else {
            color = gGmWork->unk6C;
        }
        part = gTitleLogoParts.p[1];
        bg = gGmWork;
        *px = bg->unk60;
        *py = bg->unk64;
        sw = *pw;
        sh = *ph;
        w = sw * bg->unk5C;
        h = sh * bg->unk5C;
        *px += (s16)((sw - w) / 2);
        *py += (s16)((sh - h) / 2);
        *pw = w;
        *ph = h;
        Sel_DrawPartsRotated(&part, 1, color, bg->unk68);
    }
    Gs_SetState(0xD, 0);
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x6C, 1);
}

extern char *gTitleMenuA[];     /* messages (select_psp.c): the title menu */
extern char *gTitleMenuB[];     /* ... and its help lines, two per entry */
/* unit's own declaration: with the definition's void (s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale) (sub_main/unk_36BE70.c), Title_DrawMenu: 43 of 245 words differ */
void Hud_DrawButtonIcon(s32, s16, s16, s32, u32, f32);  /* a button icon */
s32 Str_Length(char *);                         /* a message's width */
char *strchr(char *, s32);                  /* strchr */
char *strcpy(char *, char *);
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);

/* The title menu: the six entries (a big number and a name each; the
 * highlighted one, unk24, at 1.5 times the size in its own colours, its
 * number animated by count), then the highlighted entry's two help
 * lines, with each "%r" drawn as a button icon. The help lines reuse y. */
void Title_DrawMenu(void *unused) {
    char buf[0x100];
    char *b;
    s32 y;
    s32 i;
    s32 sel;
    f32 scale;
    f32 f;
    f32 sz;
    s32 kind;
    s32 color;
    char **m;
    u32 c2;
    s32 d;
    s32 ty;
    char *p;
    s16 x;

    WindowFrame_Draw(&gTitleMenuFrame, 0);
    gGmWork->unk54 = 0xFF195E32;
    gGmWork->unk58 = 0xFF0C2F19;
    Gs_SetState(0x6C, 0);
    Gfx_SetBlend(4, 5);
    m = gTitleMenuA;
    y = 0x5A;
    i = 0;
    sel = gGmWork->unk24;
    do {
        if (i == sel) {
            color = 0xFFFFBD29;
            c2 = gGmWork->unk54;
            scale = 1.5f;
            kind = gGmWork->count % 3;
            x = 0x64;
        } else {
            c2 = gGmWork->unk58;
            color = 0xFFA87600;
            scale = 1.0f;
            kind = 0;
            x = 0x9C;
        }
        f = (44.0f - 32.0f * scale) / 2.0f;
        d = i;
        if (i == 1) {
            d = 8;
        }
        if (i == 3) {
            d = 9;
        }
        Title_DrawDigitScaled(d, kind, scale, color, y + (s32)f, 0);
        sz = 24.0f * scale;
        Font_SetSpacing(0.9f * sz, sz);
        ty = y + (s32)((44.0f - sz) / 2.0f);
        Font_SetPos((s32)x + 2, ty + 2);
        Font_SetColor(0xFF000000);
        Font_Printf(*m);
        Font_SetPos((s32)x, ty);
        Font_SetColor(c2);
        Font_Printf(*m);
        i++;
        m++;
        y += 0x2C;
    } while (i < 6);
    m = gTitleMenuB;
    m += sel * 2;
    y = 0x174;
    Font_SetSpacing(0x12, 0x12);
    i = 0;
    do {
        if (Str_Length(*m) != 0) {
            strcpy(buf, *m);
            b = buf;
            for (;;) {
                p = strchr(b, '%');
                if (!p != 0) {
                    break;
                }
                if (p[1] == 'r') {
                    Hud_DrawButtonIcon(0x19, (p - b) * 9 + 0x32, y, 0x155, -1, 0.9f);
                    p[0] = ' ';
                    p[1] = ' ';
                }
            }
            Text_PrintfAt(0x32, y, 0, buf);
        }
        i++;
        m++;
        y += 0x14;
    } while (i < 2);
    Gs_SetState(0x6C, 1);
}

void ModeSel_Draw(s32 sel, s32 arg1);
extern char *gTitleHelpMenu[];     /* messages (select_psp.c) */
extern char *gTitleHelpTexts[];
void Text_PrintfAt(s16 x, s16 y, s16 color, const char *fmt, ...);
s32 Str_Length(char *);
void Hud_DrawButtonIconWhite(s32, s32, s32, s32, f32);
void Text_SetDefaults(void);

/* The title menu's help lines and buttons. */
void Title_DrawHelp(void) {
    s32 sel;
    char **m;
    char **s;
    s32 w;

    sel = gGmWork->sel;
    ModeSel_Draw(sel, gGmWork->count);
    m = gTitleHelpMenu;
    if (sel == 3) {
        sel = 4;
    }
    Font_SetSpacing(0x12, 0x12);
    s = &m[sel * 2];
    w = Str_Length(*s);
    if (w != 0) {
        Text_PrintfAt((s32)((640.0f - 9.0f * w) / 2.0f), 0x162, 0, *s);
    }
    s = &m[sel * 2 + 1];
    w = Str_Length(*s);
    if (w != 0) {
        Text_PrintfAt((s32)((640.0f - 9.0f * w) / 2.0f), 0x176, 0, *s);
    }
    Text_PrintfAt(0xE6, 0x18E, 0, gTitleHelpTexts[0]);
    Hud_DrawButtonIconWhite(0x14, 0xCA, 0x18C, 8, 0.9f);
    Hud_DrawButtonIconWhite(0x15, 0x13A, 0x18C, 8, 0.9f);
    Text_SetDefaults();
    Gs_SetState(0x6C, 1);
}

void Sel_DrawBackground(u8 c) {
    Sprite spr;

    Gfx_SetState63(1);
    Tex_Prepare(1, 7);
    Tex_Bind(7);
    Gs_SetState(0x6C, 0);
    spr.x = 0;
    spr.y = 0;
    spr.w = 0x280;
    spr.h = 0x1C0;
    spr.color = (0xFF << 24) | (c << 16) | (c << 8) | c;
    spr.v0 = 0;
    spr.u0 = 0;
    spr.u1 = 0xFF;
    spr.v1 = 0xFF;
    Sprite_Draw(&spr);
    Gs_SetState(0x6C, 1);
}

char *strchr(char *, s32);             /* strchr */
char *strncpy(char *, char *, s32);     /* strncpy */
char *strcat(char *, char *);          /* strcat */
char *strcpy(char *, char *);          /* strcpy */
char *strupr(char *);                  /* to upper case */

/* A screen's header (a text in which %a, %b and %c are spaces of 0x30, 0x14
 * and 0x20), centred, one sprite per letter of the header font. It needs
 * optimization level 4 (the second loop's first test reads out[0] through
 * sp), q declared before n, and `scale *= 2.0f` before the first colour
 * store (a shadow's y offset and colour, overwritten: probably a drop
 * shadow whose draw call was compiled out). */
void Sel_DrawHeader(s16 kind) {
    char *s;
    f32 scale;
    s32 c;
    char out[0x20];
    char *q;
    s32 size;
    char *p;
    s32 x;
    s32 extra;
    s32 y;
    char buf[0x20];
    s32 n;
    Sprite spr;

    Gfx_SetState63(1);
    s = ((char **)gSelHeaderTexts)[kind];
    size = gSelHeaderLayout[kind][0];
    y = gSelHeaderLayout[kind][1];
    extra = 0;
    p = s;
    out[0] = 0;
    for (;;) {
        if ((q = strchr(p, '%')) == NULL) {
            break;
        }
        n = q - p;
        strncpy(buf, p, n);
        buf[n] = 0;
        strcat(out, buf);
        switch (q[1]) {
        case 'a':
            extra += 0x30;
            break;
        case 'b':
            extra += 0x14;
            break;
        case 'c':
            extra += 0x20;
            break;
        }
        p = q + 2;
    }
    strcat(out, p);
    x = (0x280 - (extra + size * Str_Length(out))) / 2.0f;
    Tex_Prepare(1, 8);
    Tex_Bind(8);
    spr.h = size;
    spr.w = size;
    out[0] = 0;
    strcpy(out, s);
    strupr(out);
    p = out;
    while (*p != 0) {
        if (*p == '%') {
            switch (p[1]) {
            case 'A':
                x += 0x30;
                break;
            case 'B':
                x += 0x14;
                break;
            case 'C':
                x += 0x20;
                break;
            }
            p += 2;
        } else {
            c = *p - 'A';
            spr.u0 = (c % 8) * 32;
            spr.v0 = (c / 8) * 32;
            scale = size / 32.0f;
            spr.u1 = spr.u0 + 0x1F;
            spr.v1 = spr.v0 + 0x1F;
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
            p++;
        }
    }
}

void Title_DrawDigit(s32 digit, s32 kind, s32 color, s16 x, s16 y) {
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

/* MODE SELECT's layout (unk124: which button is on the left). Static, like
 * ModeSel_SwapLayout: in void functions MWCC keeps v0 free up to the end, and
 * through calls to static functions that don't use it (ModeSel_Start and
 * ModeSel_UpdateSlide never use v0 before calling these). */
static void ModeSel_SetLayout(void) {
    ModeBase *b;
    s16 *p;
    s32 k;
    s16 *r;
    s16 *q;
    s32 i;

    b = (ModeBase *)gSysSettings.workArea;
    k = b->m.unk124;
    p = (s16 *)gModeSelLayoutB + k * 16;
    for (i = 0; i < 8; i++) {
        b->m.unkCC[i][0] = *p++;
        b->m.unkCC[i][1] = *p++;
    }
    q = (s16 *)gModeSelLayoutA + k * 4;
    r = (s16 *)gModeSelLayoutA + (k ^ 1) * 4;
    for (i = 0; i < 2; i++) {
        b->m.unkB4[i][0] = *q++;
        b->m.unkB4[i][1] = *q++;
    }
    for (i = 0; i < 2; i++) {
        b->m.unkC4[i] = *r++;
        r++;
    }
    b->m.unk128 = b->m.unk124;
}

static void ModeSel_SwapLayout(void) {
    s32 k;
    ModeBase *b;
    s16 *p;
    s16 *q;
    s32 i;

    b = (ModeBase *)gSysSettings.workArea;
    k = b->m.unk124 ^ 1;
    p = (s16 *)gModeSelLayoutB + k * 16;
    q = (s16 *)gModeSelLayoutA + k * 4;
    for (i = 0; i < 8; i++) {
        b->m.unkCC[i][0] = *p++;
        b->m.unkCC[i][1] = *p++;
    }
    for (i = 0; i < 2; i++) {
        q++;
        b->m.unkB4[i][1] = *q++;
    }
    b->m.unk128 = k;
}

void ModeSel_StartSlide(void) {
    ModeBase *b;

    b = (ModeBase *)gSysSettings.workArea;
    b->m.unk60[1] = 2;
    b->m.unk60[2] = 2;
    b->m.unk10C[0] = -20.0f;
    b->m.unk10C[1] = 20.0f;
    b->m.unk114[0] = 0x4000;
    b->m.unk114[1] = 0x4000;
    b->m.unk11C[0] = 0x1000;
    b->m.unk11C[1] = 0x1000;
}

extern s32 gTexHandles[];
static void ModeSel_SetLayout(void);

/* MODE SELECT's start. */
void ModeSel_Start(s16 sel) {
    ModeWork *m;
    s32 i;
    s32 j;
    u8 *p;

    m = &((ModeBase *)gSysSettings.workArea)->m;
    memset(m, 0, 0x134);
    for (j = 0xA3, i = 0; i < 16; i++, j++) {
        m->tex[i] = gTexHandles[j];
    }
    p = (u8 *)gModeSelStartParams;
    for (i = 0; i < 4; i++) {
        m->items[i].kind = *p++;
        m->items[i].unk4 = *p++;
    }
    for (i = 0; i < 3; i++) {
        if (i > 0 && i < 3) {
            m->unk60[i] = 1;
        }
        m->count++;
    }
    m->sel = sel;
    m->unk124 = m->sel ^ 1;
    ModeSel_SetLayout();
}

s32 ModeSel_IsIdle(void) {
    s32 temp_v1;

    temp_v1 = ((ModeBase *)gSysSettings.workArea)->m.unk60[1];
    if ((temp_v1 == 1) && (temp_v1 == 1)) {
        return 1;
    }
    return 0;
}

f32 Math_Sin(f32);

/* MODE SELECT's buttons sliding out (2) and back in (3). */
void ModeSel_UpdateSlide(void) {
    ModeWork *m;
    s32 i;

    m = &((ModeBase *)gSysSettings.workArea)->m;
    for (i = 0; i < 2; i++) {
        if (m->unk60[i + 1] == 2) {
            m->unkB4[i][0] += m->unk10C[i] * Math_Sin(6.2831855f * m->unk114[i] / 65536.0f);
            m->unk114[i] = m->unk11C[i] + m->unk114[i];
            if (m->unk114[i] >= 0x8000) {
                m->unk60[i + 1] = 3;
                if (m->unk60[1] == 3 && m->unk60[2] == 3) {
                    ModeSel_SwapLayout();
                }
            }
        }
        if (m->unk60[i + 1] == 3) {
            m->unkB4[i][0] += m->unk10C[i] * Math_Sin(6.2831855f * m->unk114[i] / 65536.0f);
            m->unk114[i] = m->unk11C[i] + m->unk114[i];
            if (m->unk114[i] >= 0xC000) {
                m->unk114[i] = 0xC000;
            }
            if ((i == 0 && m->unkB4[i][0] >= m->unkC4[i]) || (i == 1 && m->unkB4[i][0] <= m->unkC4[i])) {
                m->unkB4[i][0] = m->unkC4[i];
                m->unk60[i + 1] = 1;
                if (m->unk60[1] == 1 && m->unk60[2] == 1) {
                    m->unk124 = m->sel ^ 1;
                    ModeSel_SetLayout();
                }
            }
        }
    }
}

void func_006BB690(void) {

}

s32 Tex_Use(s32, u32 *);

/* A MODE SELECT picture (gModeSelIcons, 20 bytes each). */
typedef struct ModeIcon {
    s32 tex;                /* 0x00: index into the mode's textures */
    s16 u0, v0, u1, v1;     /* 0x04 */
    u32 color;              /* 0x0C */
    u16 angle;              /* 0x10 */
    u16 unk12;
} ModeIcon;

/* One MODE SELECT button: its two pictures (the second of 1 and 3 with a
 * shadow). The pointers to spr.y and spr.color are locals (MWCC otherwise
 * hoists the two addresses into the lowest saved registers). */
void ModeSel_DrawButton(s32 *item) {
    Sprite spr;
    ModeIcon icon;
    s16 (*r)[4];
    s32 a;
    s32 b;
    s16 k;
    ModeWork *m;
    s32 i;
    ModeIcon *ip;
    s32 sx;
    s32 sy;
    u32 sc;
    u32 *pc;
    s16 *py;

    pc = &spr.color;
    py = &spr.y;
    m = &((ModeBase *)gWorkArea)->m;
    for (i = 0; i < 2; i++, item++) {
        k = *item;
        if (k == 0xFF) {
            continue;
        }
        *(ip = &icon) = ((ModeIcon *)gModeSelIcons)[k];
        r = &((s16 (*)[5][4])gModeSelButtonRects)[m->unk128][k];
        *pc = icon.color;
        if (m->unk128 != 0) {
            if (k == 1) {
                *pc = gModeSelColorsL1[0];
            }
            if (k == 3) {
                *pc = gModeSelColorsL1[1];
            }
            if (k == 2) {
                *pc = gModeSelButtonColor[0];
            }
            if (k == 4) {
                *pc = gModeSelColorsDim[0];
            }
        } else {
            if (k == 1) {
                *pc = gModeSelColorsL0[0];
            }
            if (k == 3) {
                *pc = gModeSelColorsL0[1];
            }
            if (k == 2) {
                *pc = gModeSelColorsDim[1];
            }
            if (k == 4) {
                *pc = gModeSelButtonColor[0];
            }
        }
        if (k == 0) {
            spr.x = (*r)[0];
            *py = (*r)[1];
        } else {
            if (k < 3) {
                b = 0;
                if (k == 1) {
                    a = 0;
                } else {
                    a = 1;
                }
            } else {
                b = 1;
                if (k == 3) {
                    a = 4;
                } else {
                    a = 5;
                }
            }
            spr.x = m->unkB4[b][0] + m->unkCC[a][0];
            *py = m->unkB4[b][1] + m->unkCC[a][1];
        }
        spr.w = (*r)[2];
        spr.h = (*r)[3];
        spr.u0 = ip->u0;
        spr.v0 = ip->v0;
        spr.u1 = ip->u1;
        spr.v1 = ip->v1;
        if (spr.x < 0x280 && spr.x + spr.w >= 0 && *py < 0x1C0 && *py + spr.h >= 0) {
            Gs_SetState(0x5E, 0x32);
            Gs_SetState(0x63, 0);
            Tex_Use(1, (u32 *)&m->tex[ip->tex]);
            Gs_SetState(4, m->tex[ip->tex]);
            if (k == 1 || k == 3) {
                sx = spr.x;
                sy = *py;
                sc = *pc;
                spr.x = sx + 8;
                *py += 8;
                *pc = 0x70000000;
                if (ip->angle != 0) {
                    TexRect_DrawRotated((struct TexRect *)&spr, ip->angle);
                } else {
                    Sprite_Draw(&spr);
                }
                spr.x = sx;
                *py = sy;
                *pc = sc;
            }
            if (ip->angle != 0) {
                TexRect_DrawRotated((struct TexRect *)&spr, ip->angle);
            } else {
                Sprite_Draw(&spr);
            }
        }
    }
}

extern char *gModeSelTexts[];     /* messages (select_psp.c): the modes' names */
s32 ModeSel_IsIdle(void);

/* MODE SELECT's two buttons. (`i ^ 1` written at each use; x an s16 and y
 * an s32 holding an s16, x passed as (s32)x to the second Font_SetPos: the
 * original's extensions.) */
void ModeSel_DrawButtons(s32 sel, s32 frame) {
    char **s;
    s32 kind;
    s32 i;
    s32 color;
    s32 f;
    s16 x;
    s32 y;
    ModeWork *m;
    u32 tcolor;
    f32 *q;

    s = gModeSelTexts;
    m = &((ModeBase *)gSysSettings.workArea)->m;
    f = frame % 3;
    for (i = 0; i < 2; i++) {
        if ((i ^ 1) == sel && ModeSel_IsIdle() != 0) {
            kind = f;
            color = 0xFFFFBD29;
            tcolor = 0x80195E32;
        } else {
            tcolor = 0x80000000;
            color = 0x80000000;
            kind = 0;
        }
        q = m->unkCC[i == 0 ? 2 : 6];
        Title_DrawDigit(1, kind, color, (s32)(m->unkB4[i][0] + q[0]), (s32)(m->unkB4[i][1] + q[1]));
        Font_SetSpacing(0x18, 0x18);
        q = m->unkCC[i == 0 ? 3 : 7];
        x = (s32)(m->unkB4[i][0] + q[0]);
        y = (s16)(s32)(m->unkB4[i][1] + q[1]);
        if ((i ^ 1) == sel) {
            Font_SetPos(x + 2, y + 2);
            Font_SetColor(0x80000000);
            Font_Printf(s[i ^ 1]);
        }
        Font_SetPos((s32)x, y);
        Font_SetColor(tcolor);
        Font_Printf(s[i ^ 1]);
    }
}

/* MODE SELECT: the buttons in their layout order, the help, the header.
 * The item is addressed through its index k at each use: a pointer
 * variable to it takes another saved register (the permuter found it). */
void ModeSel_Draw(s32 sel, s32 frame) {
    s32 i;
    ModeWork *w;
    s32 a;
    s32 b;
    s32 c;
    ModeBase *base;
    s32 k;

    base = (ModeBase *)gSysSettings.workArea;
    w = &base->m;
    base->m.order[1] = gModeSelOrders[base->m.unk128][0];
    base->m.order[2] = gModeSelOrders[base->m.unk128][1];
    a = Gs_GetState(0x6C);
    b = Gs_GetState(0x63);
    c = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    w->sel = sel;
    for (i = 0; i < w->count; i++) {
        k = w->order[i];
        if (w->items[k].kind != 0xFF) {
            ModeSel_DrawButton(&w->items[k].kind);
            if (w->items[k].kind == 0) {
                WindowFrame_Draw(&gModeSelFrame, 0);
            }
        }
    }
    if (w->state == 1) {
        func_006BB690();
    }
    ModeSel_DrawButtons(sel, frame);
    Sel_DrawHeader(1);
    Gs_SetState(0x6C, a);
    Gs_SetState(0x63, b);
    Gs_SetState(0x5E, c);
}


/* The character selection's start. */
void CharaSel_Start(void) {
    ChrEdit *c;
    CharaSelWork *w;
    s32 i;
    s32 j;
    u8 *p;
    s16 *q;

    c = &gChrEdit;
    w = &((CharaSelBase *)gSysSettings.workArea)->w;
    memset(w, 0, 0x1A4);
    for (j = 0xA3, i = 0; i < 16; i++, j++) {
        w->tex[i] = gTexHandles[j];
    }
    p = (u8 *)gCharaSelStartBytes;
    for (i = 0; i < 5; i++) {
        w->items[i].kind = *p++;
        w->items[i].unk4 = *p++;
    }
    q = (s16 *)gCharaSelStartPos;
    for (i = 0; i < 4; i++) {
        w->items2[i].x = *q++;
        w->items2[i].y = *q++;
        if (i > 0 && i < 4) {
            w->unk74[i] = 1;
            w->unk108[i].x = -80.0f;
        }
        w->order[i] = i;
        w->count++;
    }
    w->unk8 = gSysSave.lastSlot;
    c->slot = w->unk8;
    for (i = 0; i < 3; i++) {
        w->order[i + 1] = ((s8 *)gCharaSelOrder)[w->unk8 * 3 + i];
        w->unk154[i] = ((f32 *)gCharaSelInitF)[w->unk8 * 3 + i];
        w->unk148[i] = ((s16 *)gCharaSelInitB)[w->unk8 * 3 + i];
        w->unk5C[i] = ((s8 *)gCharaSelInitA)[w->unk8 * 3 + i];
        w->unkE0[i + 1].x = w->unk154[i] < 1.0f ? 0xF0 : 0x104;
    }
}

void CharaSel_Leave(void) {
    CharaSelBase *base;

    base = (CharaSelBase *)gSysSettings.workArea;
    base->w.unk74[1] = 4;
    base->w.unk108[1].x = 80.0f;
    base->w.unk5C[0] = gCharaSelSlotOrders[base->w.unk8][0];
    base->w.unk74[2] = 4;
    base->w.unk108[2].x = 80.0f;
    base->w.unk5C[1] = gCharaSelSlotOrders[base->w.unk8][1];
    base->w.unk74[3] = 4;
    base->w.unk108[3].x = 80.0f;
    base->w.unk5C[2] = gCharaSelSlotOrders[base->w.unk8][2];
    base->w.state = 2;
}

/* The character selection's update: the entries slide in (states 1-3) and
 * out (4-6). */
void CharaSel_Update(void) {
    CharaSelWork *w;
    ChrEdit *c;
    s32 i;

    w = &((CharaSelBase *)gSysSettings.workArea)->w;
    c = &gChrEdit;
    if (w->state == 1) {
        w->unk17C++;
        if (!(w->unk17C & 3)) {
            w->unk180++;
            w->unk180 %= 3;
        }
        w->unk8 = c->slot;
        for (i = 0; i < 3; i++) {
            w->order[i + 1] = ((s8 *)gCharaSelOrder)[w->unk8 * 3 + i];
            w->unk154[i] = ((f32 *)gCharaSelInitF)[w->unk8 * 3 + i];
            w->unk148[i] = ((s16 *)gCharaSelInitB)[w->unk8 * 3 + i];
            if (w->unk154[i] < 1.0f) {
                w->items2[i + 1].x = 240.0f;
            } else {
                w->items2[i + 1].x = 260.0f;
            }
        }
    }
    for (i = 0; i < 5; i++) {
        if (w->items[i].kind != 0xFF) {
            switch (w->unk74[i]) {
            case 1:
                if (w->unk58[i] == 0) {
                    w->unk74[i] = 2;
                } else {
                    w->unk58[i]--;
                    break;
                }
            case 2:
                w->items2[i].x += w->unk108[i].x;
                if (w->items2[i].x <= w->unkE0[i].x) {
                    w->items2[i].x = w->unkE0[i].x;
                    w->unk74[i] = 3;
                    w->unk1++;
                }
                break;
            case 4:
                if (w->unk58[i] == 0) {
                    w->unk74[i] = 5;
                } else {
                    w->unk58[i]--;
                    break;
                }
            case 5:
                w->items2[i].x += w->unk108[i].x;
                if (!(w->items2[i].x < 860.0f)) {
                    w->items[i].kind = 0xFF;
                    w->items[i].unk4 = 0xFF;
                    w->unk74[i] = 6;
                    w->unk1++;
                }
                break;
            case 0:
            case 3:
            case 6:
                break;
            }
        }
    }
    switch (w->state) {
    case 0:
        if (w->unk1 == 3) {
            w->state = 1;
            w->unk4 = 1;
        }
        break;
    case 1:
        break;
    case 3:
        break;
    case 2:
        if (w->unk1 == 6) {
            w->state = 3;
        }
        break;
    }
}

s32 CharaSel_IsIdle(void) {
    return ((CharaSelBase *)gSysSettings.workArea)->w.state == 1;
}

s32 CharaSel_IsDone(void) {
    return ((CharaSelBase *)gSysSettings.workArea)->w.state == 3;
}

s32 CharaSel_IsShown(void) {
    return ((CharaSelBase *)gSysSettings.workArea)->w.unk4;
}

s32 SaveMenu_SlotUsed(s32 slot);      /* the save slot is used */
s32 Tex_Use(s32, u32 *);

/* The save slots' frames on the character selection. */
void CharaSel_DrawSlots(void) {
    Sprite spr;
    SelIcon icon;
    CharaSelWork *c;
    SelIcon *ip;
    s16 (*pos)[2];
    s32 i;

    ip = &icon;
    c = &((CharaSelBase *)gSysSettings.workArea)->w;
    *ip = gCharaSelSlotIcons[0];
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x63, 0);
    Tex_Use(1, (u32 *)&gTexHandles[ip->tex]);
    Gs_SetState(4, gTexHandles[ip->tex]);
    pos = (s16 (*)[2])gCharaSelSlotPos + c->unk8 * 3;
    for (i = 0; i < 3; i++) {
        if (SaveMenu_SlotUsed(i) == 0) {
            pos++;
            continue;
        }
        if (i == c->unk8) {
            icon = gCharaSelSlotIcons[c->unk180];
            spr.w = ip->w;
            spr.h = ip->h;
            spr.color = 0xFFFF6000;
        } else {
            icon = gCharaSelSlotIcons[0];
            spr.w = 0.7f * ip->w;
            spr.h = 0.7f * ip->h;
            spr.color = 0xFF342B2A;
        }
        spr.x = (*pos)[0];
        spr.y = (*pos)[1];
        pos++;
        spr.u0 = ip->u0;
        spr.v0 = ip->v0;
        spr.u1 = ip->u1;
        spr.v1 = ip->v1;
        Sprite_Draw(&spr);
    }
}

/* One entry of the character selection: its two pictures (the second is
 * scaled by the entry's depth). */
void CharaSel_DrawEntry(s32 *item, CharaSelPos *pos) {
    s32 k;
    CharaSelWork *w;
    Sprite spr;
    s16 dy;
    SelIcon icon;
    s32 i;
    s16 ofs;
    SelIcon *ip;

    w = &((CharaSelBase *)gSysSettings.workArea)->w;
    for (i = 0; i < 2; i++, item++) {
        if (*item == 0xFF) {
            continue;
        }
        *(ip = &icon) = ((SelIcon *)gCharaSelEntryIcons)[*item];
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
                if (k == w->unk8) {
                    ip->color = -1;
                } else {
                    ip->color = 0xFFA0A0A0;
                    if (w->unk8 == 0 && *item == 3) {
                        dy = 0x14;
                    }
                    if (w->unk8 == 1 && *item == 1) {
                        dy = 0x14;
                    }
                    if (w->unk8 == 2 && *item == 5) {
                        dy = 0x14;
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
                ip->y = w->unk148[k];
                ip->unk1C = w->unk154[k];
                if (ip->unk1C < 1.0f) {
                    ip->color = ((u32 *)gCharaSelEntryColors)[k];
                }
            }
        }
        spr.x = dy + (ip->x + (s16)pos->x);
        spr.y = ip->y + (s16)pos->y;
        if (ip->unk1C < 1.0f) {
            spr.w = ip->w * ip->unk1C;
            spr.h = ip->h * ip->unk1C;
            ofs = (ip->h * ip->unk1C - spr.h) / 2.0f;
        } else {
            ofs = 0;
            spr.w = ip->w;
            spr.h = ip->h;
        }
        spr.x += (s16)(ip->w - spr.w);
        spr.y += ofs;
        spr.u0 = ip->u0;
        spr.v0 = ip->v0;
        spr.u1 = ip->u1;
        spr.v1 = ip->v1;
        spr.color = ip->color;
        if (spr.x < 0x280 && spr.x + spr.w >= 0 && spr.y < 0x1C0 && spr.y + spr.h >= 0) {
            Gs_SetState(0x5E, 0x32);
            Gs_SetState(0x63, 0);
            Tex_Use(1, (u32 *)&w->tex[ip->tex]);
            Gs_SetState(4, w->tex[ip->tex]);
            if (ip->angle != 0) {
                TexRect_DrawRotated((struct TexRect *)&spr, ip->angle);
            } else {
                Sprite_Draw(&spr);
            }
        }
    }
}

void CharaSel_Draw(void) {
    s32 i;
    CharaSelWork *w;
    s32 a;
    s32 b;
    s32 c;

    w = &((CharaSelBase *)gSysSettings.workArea)->w;
    a = Gs_GetState(0x6C);
    b = Gs_GetState(0x63);
    c = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    for (i = 0; i < w->count; i++) {
        if (w->items[w->order[i]].kind != 0xFF) {
            CharaSel_DrawEntry(&w->items[w->order[i]].kind, &w->items2[w->order[i]]);
        }
    }
    if (w->state == 1) {
        CharaSel_DrawSlots();
    }
    Sel_DrawHeader(5);
    Gs_SetState(0x6C, a);
    Gs_SetState(0x63, b);
    Gs_SetState(0x5E, c);
}

extern s32 D_0042F1DC;
void Sel_LoadCommonTextures(void) {
    s32 i;
    s32 n;
    s32 buf;

    buf = (s32)gSysSettings.loadBuf;
    LoadCompressedFile((u8 *)buf, D_0042F1DC);
    n = Pack_Count((s32 *)buf);
    if (gOvlTex[0] == 0) {
        for (i = 0; i < n; i++) {
            if (i >= 0x10) {
                break;
            }
            gOvlTex[i] = Tex_CreateFromTim2((u8 *) (Pack_GetPart((u8 *)buf, i)), 4);
        }
        Mem_Compact();
    }
}

/* Frees the 16 textures from 0xA3 that Sel_LoadCommonTextures made (Tex_FreeRange(first, count)). */
void Sel_FreeTextures(void) {
    Tex_FreeRange(0xA3, 0x10);
    Mem_Compact();
}
