#include "common.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "window.h"
#include "lobby_labels.h"

extern IconSprite gIconSprites[];
void Gfx_SetTexFilter(s32 a);
s32 Gs_SetState(u8, u32);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32);
struct Sprite;
s32 Sprite_Draw(struct Sprite *);

void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);

s32 Text_DisplayLength(char *s);

s16 Text_CenterX(char *s, s16 x, s16 w)
{
    s32 len = Text_DisplayLength(s);

    return x - w * len / 4;
}

s32 sprintf(char *buf, char *fmt, ...);
void Font_SetSpacing(u8 a, u8 b);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);

void Menu_DrawMoney(s16 x, s16 y, s8 flag)
{
    char buf[0x20];
    s16 cx = x;
    s16 ty = y + 18;
    s16 i = 0;
    u32 color;

    do {
        Icon_Draw(cx + 3, ty + 2, 24, 10, 0xFF000000, 0x42);
        Icon_Draw(cx, ty, 24, 10, 0xFF806000, 0x42);
        cx += 24;
        i++;
    } while (i < 6);
    if (flag == 0) {
        i = 0;
        color = 0xFFE0E000;
    } else {
        color = 0xFF808000;
        i = 10;
    }
    Icon_Draw(x, y, 24, 24, color, 0x6C);
    Font_SetSpacing(28, 20);
    sprintf(buf, "%7dz", gSave.money);
    Text_DrawShadow(x + 26, y + 4, 1, (s32)i, buf);
}

void Font_SetColor(s32 color);

void Menu_DrawPoints(s16 x, s16 y)
{
    char buf[0x20];
    s16 cx = x;
    s16 ty = y + 18;
    s16 i = 0;

    do {
        Icon_Draw(cx + 3, ty + 2, 24, 10, 0xFF000000, 0x42);
        Icon_Draw(cx, ty, 24, 10, 0xFF806000, 0x42);
        cx += 24;
        i++;
    } while (i < 6);
    Icon_Draw(x, y, 24, 24, 0xFFE0E000, 0x6C);
    Font_SetColor(0x807F7F7F);
    Font_SetSpacing(28, 20);
    sprintf(buf, "%7dP", gSave.points);
    Text_DrawShadow(x + 26, y + 4, 1, 0, buf);
}

void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color)
{
    s16 i;
    s32 end = x + w;

    for (i = 0; i < 30; i++) {
        if (x + 20 > end) {
            break;
        }
        Icon_Draw(x, y, 20, 8, color, 0x42);
        x += 20;
    }
}

void Font_SetColor(s32 color);
void Font_SetPos(s16, s16);
void Text_PrintfColored(const char *, ...);

void Text_DrawShadowed(s16 x, s16 y, s32 shadow, s32 color, char *s)
{
    Font_SetColor(shadow);
    Font_SetPos(x + 3, y + 2);
    Text_PrintfColored(s);
    Font_SetColor(color);
    Font_SetPos(x, y);
    Text_PrintfColored(s);
}

void Icon_DrawRotatedAt(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s32 i, s16 cx, s16 cy);

void Menu_DrawBarRotated(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s16 cx, s16 cy, u8 k)
{
    if (k <= 1) {
        Icon_DrawRotatedAt(x, y, 16, h, color, angle, k + 7, cx, cy);
        Icon_DrawRotatedAt(x + 16, y, w - 32, h, color, angle, k + 3, cx, cy);
        Icon_DrawRotatedAt(x + w - 16, y, 16, h, color, angle, k + 11, cx, cy);
    } else {
        Icon_DrawRotatedAt(x, y, 16, h, color, angle, k + 11, cx, cy);
        Icon_DrawRotatedAt(x + 16, y, w - 32, h, color, angle, k + 3, cx, cy);
        Icon_DrawRotatedAt(x + w - 16, y, 16, h, color, angle, k + 7, cx, cy);
    }
}

void Menu_DrawRowBar(s16 x, s16 y, s16 w, u32 color, u8 sel, u8 kind);
void Text_DrawShadowRgb(s16 x, s16 y, u32 c1, u32 c2, char *s);

void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2)
{
    Menu_DrawRowBar(x, y, a2, a3, t1, t2);
    if (label != NULL) {
        Text_DrawShadowRgb(x + 0x12, y, 0xFF202020, -1, label);
    }
}

/* The sprite of a kind's tab; a helper in the game: called inline, it keeps
   the kind's modulo out of a saved register (Menu_DrawRowBar). */
static inline s32 Icon_KindSprite(u8 kind)
{
    return kind % 6 + 0x6E;
}

void Menu_DrawCursorIcon(s16 a0, s16 a1, s16 a2, u32 a3);

void Menu_DrawRowBar(s16 x, s16 y, s16 w, u32 color, u8 sel, u8 kind)
{
    s32 s = sel;
    s32 spr;
    s16 h;
    s16 off;

    h = s ? (s16)10 : (s16)8;
    spr = Icon_KindSprite(kind);
    Icon_Draw(x + 4, y + 20, w, h, 0xFF202020, spr);
    Icon_Draw(x, y + 18, w, h, color, spr);
    s = sel;
    if (s == 1) {
        off = 0;
        if (gFrameTick & 0x10) {
            off = 6;
        }
        Menu_DrawCursorIcon(x + off - 18, y, 24, color);
    }
}


s32 sprintf(char *buf, char *fmt, ...);
void Font_SetSpacing(u8 a, u8 b);
u32 Color_Pulse(void);
void Menu_DrawArrows(s16 x, s16 y, s16 size, char *text, s32 color);
extern char gLobbyStrFmt[];

void Menu_DrawPageArrows(s16 x, s16 y, s16 page, s16 n, u32 color)
{
    char buf[0x20];

    if (n < 10) {
        sprintf(buf, "%d%s%d", page + 1, gLobbyMarks[LOBBY_MARK_SLASH], n);
    } else {
        sprintf(buf, "%2d%s%d", page + 1, gLobbyMarks[LOBBY_MARK_SLASH], n);
    }
    Font_SetSpacing(18, 18);
    Font_SetPos(x, y);
    Text_PrintfColored(gLobbyStrFmt, buf);
    if (color == 0) {
        color = Color_Pulse();
    }
    if (n > 1) {
        Menu_DrawArrows(x, y, 18, buf, color);
    }
}

u32 Font_GetColor(void);
void Sjis_Widen(char *src, char *dst);
void Menu_DrawArrowsSized(s16 x, s16 y, s16 w, s16 size, char *text, s32 color);

void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt)
{
    char buf[0x20];
    char conv[0x20];
    u32 saved;

    Color_Pulse();
    saved = Font_GetColor();
    if (n < 10) {
        sprintf(buf, "%d%s%d", page + 1, gLobbyMarks[LOBBY_MARK_SLASH], n);
    } else {
        sprintf(buf, "%2d%s%d", page + 1, gLobbyMarks[LOBBY_MARK_SLASH], n);
    }
    Sjis_Widen(buf, conv);
    Font_SetSpacing(18, 18);
    Font_SetPos(x, y);
    if (alt == 0) {
        Text_DrawShadowRgb(x, y, 0x80000000, 0x807F7F7F, conv);
    } else {
        Text_DrawShadowRgb(x, y, 0x80000000, 0x807F7F7F, buf);
    }
    if (n > 1) {
        if (alt == 0) {
            Menu_DrawArrowsSized(x, y, 18, 18, conv, 0xFF8484FF);
        } else {
            Menu_DrawArrowsSized(x, y, 18, 18, buf, 0xFF8484FF);
        }
    }
    Font_SetColor(saved);
}

void Menu_DrawCursorIcon(s16 a0, s16 a1, s16 a2, u32 a3)
{
    Icon_Draw(a0, a1, a2, a2, a3, 0x29);
}

void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 color, u32 color2)
{
    s16 off = (gFrameTick & 0x10) ? (s16)4 : (s16)10;

    Icon_Draw(x, y + 14, w, 8, color, 0x6F);
    Menu_DrawCursorIcon(x - h - off, y - 2, 24, color2);
}

void Menu_DrawSideArrows(s16 x, s16 y, s16 w, s16 size, s8 flip, u32 color)
{
    s16 off = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
    s32 l;
    s32 r;

    if (flip == 0) {
        l = 0x29;
        r = 0x28;
    } else {
        l = 0x28;
        r = 0x29;
    }
    Icon_Draw(x - size - off, y, size, size, color, l);
    Icon_Draw(x + w + off, y, size, size, color, r);
}

f32 Math_Sin(f32 x);

u32 Color_Pulse(void)
{
    u16 a = (gFrameTick & 0x3F) << 10;
    s8 v = 80.0f * Math_Sin(9.5873801e-05f * a);

    return ((v + 0x9F) << 24) | 0x8020;
}

u32 Color_PulseAlpha(u32 color)
{
    u16 t = (gFrameTick & 0x3F) << 10;

    color = (color & 0xFFFFFF) | (((s8)(48.0f * Math_Sin(0.0000958738f * t)) + 0xA0) << 24);
    return color;
}

u32 Color_PulseAlpha(u32 color);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);

void Menu_DrawLine(s16 x0, s16 y0, s16 x1, s16 y1, s8 flag)
{
    u32 color;

    if (flag == 0) {
        color = Color_PulseAlpha(0x8080);
    } else {
        color = 0x80808080;
    }
    Gfx_DrawLine(x0, y0, x1, y1, color);
}

void LobbyUi_DrawIcon(s16 x, s16 y, s16 size, u32 color, s8 kind)
{
    s16 h;

    switch (kind) {
    case 0:
        break;
    case 1:
        h = 0.8f * size;
        Icon_Draw(x - (h >> 1), y - (size >> 1), h, size, color, 0x79);
        break;
    case 2:
        h = size >> 1;
        Icon_Draw(x - h, y - (size >> 1), h, size, color, 0x79);
        Icon_Draw(x, y - (size >> 1), h, size, color, 0x79);
        break;
    case 3:
        h = size >> 1;
        Icon_Draw(x - h / 2, y - h, h, h, color, 0x79);
        Icon_Draw(x - h, y, h, h, color, 0x79);
        Icon_Draw(x, y, h, h, color, 0x79);
        break;
    }
}

void Menu_DrawFrameBox(s16 x, s16 y, s16 w, s16 h, u32 color)
{
    Icon_Draw(x, y, w, 8, color, 0x43);
    Icon_Draw(x, y + h - 8, w, 8, color, 0x44);
    Icon_Draw(x, y + 8, 8, h - 16, color, 0x45);
    Icon_Draw(x + w - 8, y + 8, 8, h - 16, color, 0x46);
    Gfx_DrawLine(x + 8, y + 8, w - 10, h - 16, color);
}

extern IconUV gEquipIconUVs[];

void Icon_DrawEquip(s32 x, s32 y, s32 w, s32 h, s32 color, s32 i)
{
    TexRect r;
    IconUV *t = &gEquipIconUVs[i];

    r.y = y;
    r.h = h;
    r.x = x;
    r.color = color;
    r.w = 1.25f * w;
    r.uv0 = t->uv0;
    r.uv1 = t->uv1;
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    Sprite_Draw((struct Sprite *)&r);
}

struct Frame;
void WindowFrame_Draw(struct Frame *, s16 *);

void Win_Draw(s32 x, s32 y, s32 w, s32 h, s32 color, s32 color2, s16 kind)
{
    Frame r;

    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    r.color = color;
    r.color2 = color2;
    switch (kind) {
    case 0:
        r.style = 0xC0000004;
        break;
    case 1:
        r.style = 5;
        break;
    case 2:
        r.style = 0x40000000;
        break;
    case 3:
        r.style = 0xC0000001;
        break;
    case 7:
        r.style = 0;
        break;
    case 4:
        r.style = 0x40000001;
        break;
    case 5:
        r.style = 0xC0000002;
        break;
    case 6:
        r.style = 0x80000001;
        break;
    case 8:
        r.style = 0x40000000;
        break;
    }
    WindowFrame_Draw(&r, (s16 *)(0));
}

extern s16 gMailIconBlink;
extern MailEntry gMailBox[8];

void Menu_DrawMailIcon(s16 x, s16 y)
{
    s8 i;
    MailEntry *p;
    f32 a;

    for (i = 0, p = gMailBox; i < 8; i++, p++) {
        if (p->used != 0) {
            if (++gMailIconBlink & 8) {
                a = 255.0f - 36.42857f * (gMailIconBlink & 7);
            } else {
                a = 36.42857f * (gMailIconBlink & 7);
            }
            Icon_Draw(x, y, 24, 20, 0xFFFFFFFF, 0x55);
            if (gMailIconBlink & 0x10) {
                if (gMailIconBlink & 0x80) {
                    Icon_Draw(x - 7, y - 5, 14, 14, ((u32)a << 24) | 0xFFFFFF, 0x57);
                    return;
                }
                Icon_Draw(x - 7, y + 12, 14, 14, ((u32)a << 24) | 0xFFFFFF, 0x57);
                return;
            }
            if (gMailIconBlink & 0x80) {
                Icon_Draw(x + 14, y + 12, 14, 14, ((u32)a << 24) | 0xFFFFFF, 0x57);
                return;
            }
            Icon_Draw(x + 14, y - 5, 14, 14, ((u32)a << 24) | 0xFFFFFF, 0x57);
            return;
        }
    }
}

void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i)
{
    TexRect r;
    IconSprite *t = &gIconSprites[i];

    r.y = y;
    r.w = w;
    r.x = x;
    r.h = h;
    r.color = color;
    r.uv0 = t->uv0;
    r.uv1 = t->uv1;
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Tex_Prepare(1, t->tex);
    Tex_Bind(t->tex);
    Sprite_Draw((struct Sprite *)&r);
}

void TexRect_DrawRotated(TexRect *r, s32 angle);

void Icon_DrawRotated(s32 x, s32 y, s32 w, s32 h, s32 color, s32 angle, s32 i)
{
    TexRect r;
    IconSprite *t = &gIconSprites[i];

    r.y = y;
    r.w = w;
    r.x = x;
    r.h = h;
    r.color = color;
    r.uv0 = t->uv0;
    r.uv1 = t->uv1;
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Tex_Prepare(1, t->tex);
    Tex_Bind(t->tex);
    TexRect_DrawRotated(&r, angle);
}

void TexRect_DrawRotatedAt(TexRect *r, s16 cx, s16 cy, u32 angle);

void Icon_DrawRotatedAt(s16 x, s16 y, s16 w, s16 h, u32 color, s32 angle, s32 i, s16 cx, s16 cy)
{
    TexRect r;
    IconSprite *t = &gIconSprites[i];

    r.y = y;
    r.w = w;
    r.x = x;
    r.h = h;
    r.color = color;
    r.uv0 = t->uv0;
    r.uv1 = t->uv1;
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Tex_Prepare(1, t->tex);
    Tex_Bind(t->tex);
    TexRect_DrawRotatedAt(&r, cx, cy, angle);
}

void TexRect_Draw(TexRect *r, s8 mode);

void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode)
{
    TexRect r;
    IconSprite *t = &gIconSprites[i];

    r.y = y;
    r.w = w;
    r.x = x;
    r.h = h;
    r.color = color;
    r.uv0 = t->uv0;
    r.uv1 = t->uv1;
    Gfx_SetTexFilter(1);
    Gs_SetState(0x60, 0);
    Tex_Prepare(1, t->tex);
    Tex_Bind(t->tex);
    TexRect_Draw(&r, mode);
}

typedef struct Rec7FF250 {
    s32 f0;
    s16 f4;
    s16 f6;
    s16 f8;
    s16 fA;
    u32 fC;
} Rec7FF250;
extern Rec7FF250 gIconPresets[];

void Icon_DrawPreset(s32 i)
{
    Rec7FF250 *r = &gIconPresets[i];

    Icon_Draw(r->f4, r->f6, r->f8, r->fA, r->fC, r->f0);
}

/* unk_790EC0's .data (0x7FEAF0-0x7FF280) */
IconUV gEquipIconUVs[20] = {
#include "lobby/007FEAF0.inc"
};
IconSprite gIconSprites[48] = {
#include "lobby/007FEB90.inc"
};
u32 gClockSectorTex[288] = {
#include "lobby/007FEDD0.inc"
};
Rec7FF250 gIconPresets[3] = {
#include "lobby/007FF250.inc"
};
