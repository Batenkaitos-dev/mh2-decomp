#include "common.h"
#include "lobby_state.h"
#include "scene.h"
#include "player.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "save.h"
#include "pad.h"

typedef struct GradBox {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    u32 col[4];
} GradBox;
extern u8 gMenuWork;
extern char *gHairChangeTexts[];
extern char *gHairShowHintTexts[];
extern u32 gHeaderBoxColors[4];
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
struct PrimGradRect;
s32 Prim_FillGradRect(struct PrimGradRect *);
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), HairMenu_DrawConfirm 86 of 95 words differ */
void Text_Printf(char *fmt, ...);
void Text_SetSize(s32);
void HairMenu_DrawHeader(void);
void HairMenu_DrawList(s16, s16);
void HairMenu_DrawConfirm(s16 x, s16 y);

extern u32 gPresetColors[];
s32 Kbd_IsOpen(void); /* unit's own declaration: the definition (sub_main) returns s8;
 * HairMenu_ColorInput is 219 words off with s8 */
void StageModels_Free(void);
void StageModels_LoadEquip(u16 kind, u16 id, u8 female);
void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32, s32);
s32 Menu_WrapLeftRight(s32, s32);

void CamFocus_SetAside(s8 a);
void Menu_SetYesNo(s8 a);
s32 Menu_YesNoInput(void);
s32 WinSlide_Step(WinSlide *p, s16 from, s16 to);
void HairMenu_ResetColor(void);
void HairMenu_LoadPage(s16 page);
s32 HairMenu_ColorInput(u16 pad);

static inline void func_006F2E80_preview(HairMenuWork *w)
{
    StageModels_Free();
    D_5BAE0C = (w->r << 16) | 0xFF000000 | (w->g << 8) | w->b;
    if (w->list.color[w->sel].equip != -2) {
        StageModels_LoadEquip(0xFF, w->list.color[w->sel].equip, gSave.gender);
    } else {
        StageModels_LoadEquip(0xFF, gSave.hairStyle, gSave.gender);
    }
}

s32 HairMenu_ListInput(u16 pad)
{
    HairMenuWork *w = (HairMenuWork *)&gMenuWork;

    switch (w->state) {
    case 0:
        switch (WinSlide_Step(&w->win, -0x140, 0xA)) {
        case 0:
            w->state++;
        }
        break;
    case 1:
        if (pad & 0x4000) {
            w->toggle ^= 1;
            if (w->toggle == 0) {
                func_006F2E80_preview(w);
            } else {
                StageModels_Free();
            }
        } else if (pad & 0x20) {
            if (w->list.color[w->sel].rank <= gSave.money) {
                w->state++;
                HairMenu_ResetColor();
                WinSlide_Step(&w->win2, -0x140, 0xA);
                LobbySe_Play(8);
            } else {
                LobbySe_Play(9);
            }
        } else if (pad & 0x40) {
            w->state = 0;
            CamFocus_SetAside(0);
            LobbySe_Play(4);
            StageModels_Free();
            return 3;
        } else if (pad & 0xC00) {
            w->page = Menu_WrapLeftRight(w->page, w->pages);
            HairMenu_LoadPage(w->page);
            if (w->toggle == 0) {
                func_006F2E80_preview(w);
            }
        } else if (pad & 0x3000) {
            w->sel = Menu_WrapUpDown(w->sel, w->count);
            if (w->toggle == 0) {
                func_006F2E80_preview(w);
            }
        }
        break;
    case 2:
        switch (WinSlide_Step(&w->win2, -0x140, 0xA)) {
        case 0:
            w->state++;
        }
        break;
    case 3:
        switch (HairMenu_ColorInput(pad)) {
        case 0:
            w->state++;
            Menu_SetYesNo(1);
            break;
        case 3:
            w->state = 0;
            HairMenu_ResetColor();
            D_5BAE0C = (w->r << 16) | 0xFF000000 | (w->g << 8) | w->b;
            WinSlide_Step(&w->win, -0x140, 0xA);
            break;
        }
        break;
    case 4:
        switch (Menu_YesNoInput()) {
        case 0:
            w->state = 0;
            StageModels_Free();
            return 0;
        case 3:
            w->state--;
            break;
        }
        break;
    }
    return 2;
}

void HairMenu_ResetColor(void)
{
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;
    Player *p = &gPlayers[gScene.localPlayer];

    m->repeat = 0;
    m->row = 0;
    m->preset = 0;
    m->r = p->hairColor >> 16;
    m->g = p->hairColor >> 8;
    m->b = p->hairColor;
}


s32 HairMenu_ColorInput(u16 pad)
{
    HairMenuWork *w = (HairMenuWork *)&gMenuWork;

    if (Kbd_IsOpen() == 0) {
        if (gPadState[0].prevHeld == gPadState[0].held) {
            w->repeat++;
            if (w->repeat > 10) {
                pad = gPadState[0].held;
            }
        } else {
            w->repeat = 0;
        }
    }
    if (pad & 0x4000) {
        w->toggle ^= 1;
        if (w->toggle == 0) {
            StageModels_Free();
            D_5BAE0C = (w->r << 16) | 0xFF000000 | (w->g << 8) | w->b;
            if (w->list.color[w->sel].equip != -2) {
                StageModels_LoadEquip(0xFF, w->list.color[w->sel].equip, gSave.gender);
            } else {
                StageModels_LoadEquip(0xFF, gSave.hairStyle, gSave.gender);
            }
        } else {
            StageModels_Free();
        }
    } else if (pad & 0x20) {
        if (w->row == 3) {
            u32 c = gPresetColors[w->preset];
            w->r = c >> 16;
            w->g = c >> 8;
            w->b = c;
            LobbySe_Play(8);
        } else if (w->row == 4) {
            w->equip = w->list.color[w->sel].equip;
            if (w->equip == -2) {
                w->equip = gSave.hairStyle;
            }
            LobbySe_Play(0);
            return 0;
        }
    } else if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x3000) {
        w->row = Menu_WrapUpDown(w->row, 5);
    } else if (pad & 0xC00) {
        switch (w->row) {
        case 3:
            w->preset = Menu_WrapLeftRight(w->preset, 0x10);
            break;
        case 0:
            if (pad & 0x800) {
                if (w->r > 0) {
                    w->r--;
                }
            } else if (w->r < 0xFF) {
                w->r++;
            }
            LobbySe_Play(1);
            break;
        case 1:
            if (pad & 0x800) {
                if (w->g > 0) {
                    w->g--;
                }
            } else if (w->g < 0xFF) {
                w->g++;
            }
            LobbySe_Play(1);
            break;
        case 2:
            if (pad & 0x800) {
                if (w->b > 0) {
                    w->b--;
                }
            } else if (w->b < 0xFF) {
                w->b++;
            }
            LobbySe_Play(1);
            break;
        }
    }
    D_5BAE0C = (w->r << 16) | 0xFF000000 | (w->g << 8) | w->b;
    return 2;
}

u16 Rand_Next(s32 stream);
void Player_FreeChangedPartModels(Player *p);
void Player_LoadPartModels(Player *p);
void Save_AddMoney(s32 n);
void HairMenu_Apply(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;
    s32 cost;
    u8 style;
    s32 color;

    if (m->withNpc == 1) {
        switch (Rand_Next(1) % 3) {
        case 0:
            style = 0x11;
            break;
        case 1:
            style = 0x12;
            break;
        case 2:
            style = 0x14;
            break;
        }
        color = gSave.hairColor;
        cost = 500;
    } else {
        style = ((u8 *)m)[0x1B4]; /* equip's low byte */
        color = m->b | ((m->r << 16) | 0xFF000000 | (m->g << 8));
        cost = m->list.hair[m->sel].cost;
    }
    D_5BAE0C = color;
    pl->hairColor = color;
    gSave.hairColor = color;
    if (style != gSave.hairStyle) {
        gSave.hairStyle = style;
        pl->hairStyle = style;
        Player_FreeChangedPartModels(pl);
        Player_LoadPartModels(pl);
    }
    Save_AddMoney(-cost);
}

void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 color, u32 color2);
void Icon_DrawEquip(s16 x, s16 y, s32 w, s32 h, s32 color, s32 i); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 color, s32 i) (lobby/unk_790EC0.c), HairMenu_DrawList 3 of 159 words differ */
void Text_PrintfColored(char *fmt, ...);
void Font_SetColor(s32 color);
extern char D_00838278[];
extern char *D_0080C074;
extern char *gPriceMarks[];
extern char *D_0080C304;
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void HairMenu_DrawList(s16 x, s16 y)
{
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;
    s16 ty = y;
    s16 i;

    Icon_Draw(x, y, 0x12C, 0xC8, 0xFFE0D8C8, 0);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(x + 0xDC, ty + 0xC, m->page, m->pages, 1);
    Font_SetSpacing(0x14, 0x14);
    ty += 0x2A;
    i = 0;
    if (i < m->count) {
        do {
            if (m->list.hair[i].cost > gSave.money) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(x + 0x2E, ty);
            if (gPlayers[gScene.localPlayer].gender == 1) {
                Text_PrintfColored(D_00838278, m->list.hair[i].name, D_0080C074, m->list.hair[i].cost, gPriceMarks[0]);
            } else {
                Text_PrintfColored(D_00838278, m->list.hair[i].name + 0x20, D_0080C074, m->list.hair[i].cost, gPriceMarks[0]);
            }
            if (m->list.hair[i].id != -2) {
                Icon_DrawEquip((s16)(x + 0x2E) - 0x1E, ty, 0x14, 0x14, -1, 0x12);
            }
            if (m->sel == i) {
                Menu_DrawCursorRow(x + 0x2E, ty, 0xDC, 0x32, 0xC0106060, 0xFF106060);
            }
            ty += 0x16;
            i++;
        } while (i < m->count);
    }
    Font_SetSpacing(0x12, 0x12);
    Win_DrawText(0x14A, 0x15E, 0x122, 0x50, 0xBDB59F6C, 0xC0101010, D_0080C304);
}

void ColorEdit_Draw(s32 a, s16 x, s16 y, u8 r, u8 g, u8 b, s32 c, s32 d); /* unit's own declaration: with the definition's void (u8 mode, s16 x, s16 y, u8 r, u8 g, u8 b, u8 sel, u8 pal) (main/unk_18DA50.c), HairMenu_DrawConfirm 5 of 91 words differ */
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
char *Text_FillDollar(char *fmt, s32 n);
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;
extern char *gHairColorTexts[];
extern char *D_0080C30C;
void HairMenu_DrawConfirm(s16 x, s16 y)
{
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;
    char *s;

    Win_Draw(x, y, 0x12C, 0xC8, 0xBDB59F6C, 0xFF000000, 2);
    ColorEdit_Draw(1, x - 10, y - 0x12, m->r, m->g, m->b, ((u8 *)m)[0x1BE], ((u8 *)m)[0x1BC]) /* row's and preset's low bytes */;
    Font_SetSpacing(0x12, 0x12);
    switch (m->state) {
    case 2:
    case 3:
        s = gHairColorTexts[0];
        break;
    case 4:
        s = Text_FillDollar(D_0080C30C, m->list.hair[m->sel].cost);
        Font_SetPos(0x1AE, 0x190);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
    Win_DrawText(0x14A, 0x15E, 0x122, 0x50, 0xBDB59F6C, 0xC0101010, s);
}

void HairMenu_DrawHeader(void)
{
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;
    GradBox b;

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x14A, 0x12, 0x82, 0xFFB0B000, gHairChangeTexts[0], 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    b.x0 = 0x14A;
    b.y0 = 0x3A;
    b.x1 = b.x0 + 0x122;
    b.y1 = b.y0 + 0x118;
    b.col[0] = gHeaderBoxColors[0];
    b.col[1] = gHeaderBoxColors[1];
    b.col[2] = gHeaderBoxColors[2];
    b.col[3] = gHeaderBoxColors[3];
    Prim_FillGradRect((struct PrimGradRect *)&b);
    Win_Draw(0x14A, 0x3A, 0x122, 0x118, 0xBDB59F6C, 0, 6);
    if (m->toggle == 1) {
        Font_SetPos(0x17C, 0x9A);
        Font_SetSpacing(0x12, 0x12);
        Text_Printf(gHairShowHintTexts[0]);
    }
}

void HairMenu_Draw(u8 *o)
{
    HairMenuWork *m = (HairMenuWork *)&gMenuWork;

    if (m->step == 1) {
        Text_SetSize(*(s32 *)(o + 0x18));
        HairMenu_DrawHeader();
        switch (m->state) {
        case 0:
        case 1:
            HairMenu_DrawList(m->win.pos, 0x3A);
            break;
        case 2:
        case 3:
        case 4:
            HairMenu_DrawConfirm(m->win2.pos, 0x3A);
            break;
        }
    }
}

/* unk_6F2E80's .rodata (0x838278-0x838288) */
#pragma rodata on
char D_00838278[13] = {
#include "lobby/00838278.inc"
};
#pragma rodata off
