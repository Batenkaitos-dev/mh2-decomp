#include "common.h"
#include "window.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "sprite.h"
#include "lobby_globals.h"

/* Compiled at optimization level 4 (docs/mwcc-matching.md, "The compiler"):
 * BoxMenu_DrawAmount and BoxMenu_DrawZenny match only there, and every other
 * function of the file matches at both levels. */
#pragma optimization_level 4

void Draw_Icons(struct Icon *, u8 count);
void Text_Draw(char *s);
void Text_SetColor(s16 color);
void Gfx_DrawPulseGradient(s16 x, s16 y, s16 w, s16 h, u32 color, u8 dir);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
extern s16 gBoxControlIcons[];
extern s16 D_007F1C78[];
extern s16 gBoxControlIconsB[];
extern char *gCameraGuideTexts[];
extern s8 *D_0080CA3C;
extern char *gMenuShowTexts[];
void BoxMenu_DrawItemList(s16, u32, f32);
void BoxMenu_DrawControls(s32);
void BoxMenu_DrawActions(s32);
void BoxMenu_DrawStore(void);
void BoxMenu_DrawTake(void);
void BoxMenu_DrawEquip(void);
void BoxMenu_DrawArrange(void);
void BoxMenu_DrawSell(void);
void BoxMenu_DrawModeName(void);
s32 Town_IsMode(s32 arg0);
void HelpWindow_DrawCurrent(void);
extern s16 gBoxMenuIcon[];
void SkillList_DrawCompare(Player *pl, s16 x, s16 y);
void Window_DrawTextB2(Window *w, u8 *text);
void StatusPage_DrawStats(Player *, Player *, s32, s32);
extern Panel gEquipInfoWin;
extern u8 *D_0080CAC4;
extern char *gStatusHeadTexts[];
extern u8 *D_0080CACC;
extern char *gStatusHeadAltTexts[];
extern Player gPreviewPlayer;
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Window_DrawText(void *p, s16 *pos, u8 *text);
extern ListPanel gEquipNonePanel;
extern Panel gEquipDetailWin[4];
void HelpWindow_Draw(u8 page, u16 idx, s8 c);
void ButtonHint_DrawDetails(void);
void Text_Printf(const char *fmt, ...);
#include "box_menu.h"
extern char *gYesNoTexts[];
/* The box list (BoxList_Build): the gSave.box slots of one equipment class, sorted or in box
 * order, shown seven to a page (BoxList_GetPage, BoxList_MoveCursor). */
typedef struct BoxList {
    u16 idx[500];       /* 0x000: indexes into gSave.box */
    u16 count;          /* 0x3E8 */
    u8 invalid;         /* 0x3EA: 1 when BoxList_Build was given a kind over 5 */
    u8 kind;            /* 0x3EB */
    u8 sorted;          /* 0x3EC: by BoxEquip_SortKey */
    u8 pages;           /* 0x3ED */
    u8 lastCount;       /* 0x3EE: the entries on the last page */
} BoxList;
typedef char BoxList_size_check[(sizeof(BoxList) == 0x3F0) ? 1 : -1];
extern BoxList gBoxList;
s32 Shake_Value(u8);
void *Shake_GetOffset(u8 id);
extern ListPanel gBoxActionsPanel;
void Menu_DrawFrame(void *b, s16 *pos);
extern ListPanel gBoxListHeaders[2];
extern char *gColorRowFmtTexts[];
extern u8 D_0083B348;
void ItemList_DrawPouch(s32, s32, s32);
void BoxMenu_DrawHelp(s32 arg0, s32 arg1);
s32 sprintf(char *, const char *, ...);
extern char *gPriceNoteTexts[];
extern s8 *D_0080CA94;

f32 Math_Sin(f32 x);
s32 Sprite_Draw(struct Sprite *);

void BoxMenu_DrawPulseSprite(u8 *spr, s32 alpha)
{
    f32 s;
    u32 t;

    /* a colour that pulses with the frame counter */
    t = (u16)((gFrameTick & 0x1F) << 11);
    s = Math_Sin(0.0000958738f * t);
    *(u32 *)(spr + 8) = ((u8)alpha << 24) | ((u8)((s8)(108.0f * s) + 0x8C) << 16) | ((u8)((s8)(6.0f * s) + 0xF8) << 8) | (u8)((s8)(38.0f * s) + 0xD6);
    *(u32 *)(spr + 0xC) = 0;
    *(u32 *)(spr + 0x10) = 0x200020;
    Sprite_Draw((struct Sprite *)spr);
}


f32 Math_Sin(f32 x);
void Tex_Prepare(s32, s32);
void Gfx_SetTexFilter(s32);
void Tex_Bind(s32);
extern u16 gFrameTick;

void BoxMenu_DrawGridCursor(void)
{
    Sprite s;
    s16 n;
    f32 v;

    Gfx_SetTexFilter(0);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    n = gBoxWork->held % 100;
    s.x = 313.0f + 28.8f * (n % 10);
    s.y = (n / 10) * 25 + 0x3B;
    s.w = 0x20;
    s.h = 0x19;
    v = Math_Sin(0.0000958738f * (u16)((gFrameTick & 0x1F) << 11));
    s.color = ((u8)((s8)(96.0f * v) + 0x9F) << 24) | 0xFF0000;
    *(u32 *)&s.u0 = 0;
    *(u32 *)&s.u1 = 0x200020;
    Sprite_Draw((struct Sprite *)&s);
}

void BoxMenu_DrawList306(s16 cur, s32 flags)
{
    BoxMenu_DrawItemList(cur, (u32) flags, 306.0f);
}

typedef struct BoxLine {
    s16 x0, y0, x1, y1;
    u32 color;
} BoxLine;

extern Panel gBoxItemListWin;
void Window_DrawTextAlpha(Window *w, u8 *text, s32 alpha); /* unit's own declaration: with the definition's void (Window *w, u8 *text, u8 alpha) (sub_main/unk_356380.c), BoxMenu_DrawItemList 3 of 941 words, BoxMenu_DrawPageTabs 1 of 155 words, BoxMenu_DrawEquipGrid 3 of 455 words differ */
struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);
struct Pt;
void Equip_GetIconUV(EquipItem *e, struct Pt *, struct Pt *);
u8 Equip_GetRarity(u8 type, u16 id);
/* unit's own declaration: the lobby's BoxMenu functions match only with (u8, s32, u8) (the game
   doesn't mask alpha); the definition's (u8, u8, s32) adds an andi at each call */
u32 Equip_Color(u8 idx, s32 alpha, u8 alt); /* unit's own declaration: with the definition's u32 (u8 idx, u8 alpha, s32 alt) (sub_main/unk_3549F0.c), BoxMenu_DrawItemList 7 of 941 words, BoxMenu_DrawEquipGrid 5 of 455 words, BoxMenu_DrawEquipList 46 of 517 words differ */
char *Equip_GetName(u8 type, u16 id);
void BoxMenu_DrawPageTabs(s16 x, s16 y, s16 page, u32 pad, s32 arg4, u8 kind);
void *Box_BuildSortedList(u8 equip);
extern u32 gBoxPageColors[];
extern u32 gItemColors[];
extern char *gItemNames[];
extern char *D_0080C04C;
extern char *gCrossMarkTexts;
s32 Equip_IsSlotEquipped(SaveData *save, s32 idx);
s32 Equip_GetSellPrice(EquipItem *e);
void EquipWindow_DrawLocal(struct EquipItem *e, s16 x, s16 y, u8 flags, struct EquipItem *p);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Text_PrintfColored(const char *fmt, ...);

/* Loops 1 and 1b write the alpha byte as `(alpha % 256) << 24` (one value, shared by the two
 * loops): MWCC turns the unsigned % 256 into an andi only after its first scheduling pass, so the
 * mask stays live in a0 across the float constants as in the game; `(u8)alpha << 24` there gives
 * other registers (round 30). Loop 2 matches with the masked alpha in `a`, set in the for.
 * Equip_Color is declared (u8, s32, u8) for the whole file: alpha passes unmasked here and in
 * BoxMenu_DrawEquipGrid (BoxMenu_DrawEquipPick declares the definition's prototype in its own
 * block). */
void BoxMenu_DrawItemList(s16 cur, u32 flags, f32 fx)
{
    SaveData *save;
    u8 equip;
    void *box;
    s32 price;
    u16 page;
    u16 idx;
    s16 n;
    Sprite s;
    char buf[0x30];
    ItemStack *stk;
    u16 item;
    ItemData *d;
    EquipItem *e;
    BoxLine r;
    s16 c;
    s32 k;
    u8 t;
    u32 alpha;
    s32 dim;
    s32 mode;
    s32 u;
    f32 x0;
    s32 noSave;
    f32 w;
    u8 max;
    f32 bx;
    u8 rare;
    f32 ox;
    s32 hl;
    s16 c2;
    s16 i;
    s32 k2;
    u8 a;

    save = &gSave;
    equip = (flags & 0x10) != 0;
    alpha = (u8)((flags & 0x20) == 0 ? 0xFF : 0x60);
    if (!(flags & 0x100)) {
        if (equip == 0) {
            box = save->itemBox;
        } else {
            box = save->box;
        }
    } else {
        box = Box_BuildSortedList(equip);
        if (cur >= 0) {
            cur = cur % 100;
        }
    }
    gBoxItemListWin.x = fx;
    Window_DrawTextAlpha((Window *)&gBoxItemListWin, NULL, alpha);
    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    page = cur / 100;
    idx = page * 100;
    n = cur % 100;
    if (((flags >> 13) & 0x3FF) != 0) {
        cur = -1;
    }
    if (equip == 0) {
        for (i = 0; i < 100; i++) {
            r.x0 = (s16)(153.0f + fx - 146.0f + 28.8f * (i % 10)) + 3;
            r.x1 = 25.6f + r.x0;
            r.y0 = (i / 10) * 25 + 0x3D;
            r.y1 = r.y0 + 0x15;
            r.color = ((alpha % 256) << 24) | gBoxPageColors[page];
            Prim_FillRect((struct PrimLine *)&r);
        }
        s.w = 0x20;
        s.h = 0x19;
        stk = (ItemStack *)box + idx;
        for (i = 0; i < 100; i++, stk++) {
            item = stk->item;
            if (item != 0) {
                d = &gItemData[item];
                if (d->icon != 0xFF) {
                    u = d->icon + 1;
                    s.x = 153.0f + fx - 146.0f + 28.8f * (i % 10);
                    s.y = (i / 10) * 25 + 0x3B;
                    s.u0 = ((u & 7) << 5) + 1;
                    s.v0 = ((u >> 3) << 5) + 1;
                    s.u1 = s.u0 + 0x1E;
                    s.v1 = s.v0 + 0x1E;
                    s.color = gItemColors[d->color];
                    s.color &= 0xFFFFFF;
                    if (((flags >> 13) & 0x3FF) == 0 || ((flags >> 13) & 0x3FF) == item) {
                        s.color |= (alpha % 256) << 24;
                    } else {
                        s.color |= 0x50000000;
                    }
                    Sprite_Draw((struct Sprite *)&s);
                }
            }
        }
    } else {
        k = k2 = idx;
        for (i = 0, a = alpha; i < 100; i++, k++) {
            r.color = (a << 24) | gBoxPageColors[page];
            if (!(flags & 0x100) && Equip_IsSlotEquipped(save, k) == 1) {
                r.color = ((u8)alpha << 24) | 0x1078D4;
            }
            r.x0 = (s16)(153.0f + fx - 146.0f + 28.8f * (i % 10)) + 3;
            r.x1 = 25.6f + r.x0;
            r.y0 = (i / 10) * 25 + 0x3D;
            r.y1 = r.y0 + 0x15;
            Prim_FillRect((struct PrimLine *)&r);
        }
        s.w = 0x20;
        s.h = 0x19;
        e = (EquipItem *)box + idx;
        for (i = 0; i < 100; i++, k2++, e++) {
            if (e->used) {
                s.x = 153.0f + fx - 146.0f + 28.8f * (i % 10);
                s.y = (i / 10) * 25 + 0x3B;
                Equip_GetIconUV(e, (struct Pt *)&s.u0, (struct Pt *)&s.u1);
                rare = Equip_GetRarity(e->type, e->id);
                if (flags & 0x200) {
                    if (Equip_IsSlotEquipped(save, k2) == 1) {
                        s.color = Equip_Color(rare, alpha, 1);
                        Sprite_Draw((struct Sprite *)&s);
                        *(u32 *)&s.u0 = 0x01200060;
                        *(u32 *)&s.u1 = 0x01400080;
                        s.color = ((u8)alpha << 24) | 0x909010;
                    } else {
                        s.color = Equip_Color(rare, alpha, 0);
                    }
                } else if (flags & 0x400) {
                    t = 0;
                    if (e->type != 6 && e->type != 7) {
                        t = 1;
                    }
                    s.color = Equip_Color(rare, alpha, t);
                    if (Equip_IsSlotEquipped(save, k2) == 1) {
                        Sprite_Draw((struct Sprite *)&s);
                        *(u32 *)&s.u0 = 0x01200060;
                        *(u32 *)&s.u1 = 0x01400080;
                        if (t == 0) {
                            s.color = ((u8)alpha << 24) | 0xFFFF00;
                        } else {
                            s.color = ((u8)alpha << 24) | 0x808010;
                        }
                    }
                } else if (flags & 0x800000) {
                    t = 0;
                    if (e->type == 6 || e->type == 7) {
                        t = 1;
                    }
                    s.color = Equip_Color(rare, alpha, t);
                    if (Equip_IsSlotEquipped(save, k2) == 1) {
                        Sprite_Draw((struct Sprite *)&s);
                        *(u32 *)&s.u0 = 0x01200060;
                        *(u32 *)&s.u1 = 0x01400080;
                        if (t == 0) {
                            s.color = ((u8)alpha << 24) | 0xFFFF00;
                        } else {
                            s.color = ((u8)alpha << 24) | 0x808010;
                        }
                    }
                } else {
                    s.color = Equip_Color(rare, alpha, 0);
                    if (!(flags & 0x100) && Equip_IsSlotEquipped(save, k2) == 1) {
                        Sprite_Draw((struct Sprite *)&s);
                        *(u32 *)&s.u0 = 0x01200060;
                        *(u32 *)&s.u1 = 0x01400080;
                        s.color = ((u8)alpha << 24) | 0xFFFF00;
                    }
                }
                Sprite_Draw((struct Sprite *)&s);
            }
        }
    }
    if (cur >= 0) {
        if (!(flags & 0x100)) {
            s.x = 153.0f + fx - 146.0f + 28.8f * (n % 10);
            s.y = (n / 10) * 25 + 0x3B;
            BoxMenu_DrawPulseSprite((u8 *)&s, alpha);
        }
        Font_SetSpacing(0x12, 0x12);
        if (!(flags & 0x20)) {
            c = 0;
            c2 = 2;
        } else {
            c = 10;
            c2 = 13;
        }
        Text_SetColor(c);
        BoxMenu_DrawPageTabs(fx, 0x1B, page, flags, alpha, 0);
        if (equip == 0) {
            stk = (ItemStack *)box + cur;
            item = stk->item;
            if (!(flags & 0x100)) {
                if (item == 0) {
                    sprintf(buf, "%s", gItemNames[0]);
                    price = -1;
                } else {
                    max = gItemData[item].max;
                    switch (max) {
                    case 1:
                        sprintf(buf, "%s", gItemNames[item]);
                        break;
                    case 0xFF:
                        sprintf(buf, "%s %s", gItemNames[item], D_0080C04C);
                        break;
                    default:
                        if (stk->count >= max) {
                            sprintf(buf, "%s %s~C%02d%d~C%02d", gItemNames[item], gCrossMarkTexts, c2, stk->count, c);
                        } else {
                            sprintf(buf, "%s %s%d", gItemNames[item], gCrossMarkTexts, stk->count);
                        }
                        break;
                    }
                    price = gItemData[stk->item].value;
                }
            }
        } else {
            e = (EquipItem *)box + cur;
            if (!e->used) {
                sprintf(buf, "%s", gItemNames[0]);
                price = -1;
            } else {
                sprintf(buf, "%s", Equip_GetName(e->type, e->id));
                price = Equip_GetSellPrice(e);
            }
        }
        if (!(flags & 0x100)) {
            Font_SetPos(5.0f + fx, 0x13F);
            Text_Printf("%s", buf);
            if ((flags & 0x1000000) && price >= 0) {
                Font_SetSpacing(0x18, 0x12);
                Font_SetPos(198.0f + fx, 0x13F);
                Text_PrintfColored("%7dz", price);
                Font_SetSpacing(0x12, 0x12);
            }
            if (flags & 8) {
                if (equip == 0) {
                    stk = &save->itemBox[cur];
                    HelpWindow_Draw(1, (u16)(stk->item + 0x18), *(s8 *)&gHelpColumn);
                } else {
                    e = &save->box[cur];
                    if (e->used) {
                        EquipWindow_DrawLocal((struct EquipItem *)e, fx, 0xB8, flags & 0x47, (struct EquipItem *)(0));
                    }
                }
            }
        }
    } else if (((flags >> 13) & 0x3FF) != 0) {
        BoxMenu_DrawPageTabs(fx, 0x1B, page, flags, alpha, 0);
    }
    if (flags & 0x80) {
        Menu_DrawMoney(160.0f + fx, 0x13, (flags & 0x20) != 0);
    }
}


void Gfx_SetBlend(s32, s32);
void Menu_DrawArrow(s16, s16, s16, s16, u32, u8);
void Window_DrawTextAlpha(Window *w, u8 *text, s32 alpha); /* unit's own declaration: with the definition's void (Window *w, u8 *text, u8 alpha) (sub_main/unk_356380.c), BoxMenu_DrawItemList 3 of 941 words, BoxMenu_DrawPageTabs 1 of 155 words, BoxMenu_DrawEquipGrid 3 of 455 words differ */
void Text_PrintfColored(const char *fmt, ...);
s16 Box_GetPageCount(SaveData *save);
typedef struct BoxTab {
    f32 x;
    f32 l;
    f32 r;
    s16 w;
    s16 lx;
    s16 tx;
    u8 icon;
} BoxTab;
extern BoxTab gBoxPageTabs[];
extern Panel gBoxPageTabsWin;

void BoxMenu_DrawPageTabs(s16 x, s16 y, s16 page, u32 pad, s32 alpha, u8 kind)
{
    Sprite s;
    s32 col;
    BoxTab *t;
    s16 n;

    n = Box_GetPageCount(&gSave);
    if (n <= 1) {
        return;
    }
    t = &gBoxPageTabs[kind];
    if (t->icon != 0) {
        gBoxPageTabsWin.x = x;
        gBoxPageTabsWin.y = y + 2;
        Window_DrawTextAlpha((Window *)&gBoxPageTabsWin, NULL, alpha);
    } else {
        Gfx_SetTexFilter(1);
        Gfx_SetBlend(4, 5);
        Tex_Prepare(1, 0x155);
        Tex_Bind(0x155);
    }
    if (!(pad & 0x120)) {
        s.color = -1;
        col = 0xFF20FF28;
    } else {
        s.color = 0x60FFFFFF;
        col = 0xA0606060;
    }
    s.y = y;
    s.w = t->x;
    s.h = 0x14;
    s.x = x + t->l;
    *(s32 *)&s.u0 = 0x79;
    *(s32 *)&s.u1 = 0x180090;
    Sprite_Draw((struct Sprite *)&s);
    s.x = x + t->r;
    s.u0 = 0xA9;
    s.u1 = 0xC0;
    Sprite_Draw((struct Sprite *)&s);
    Menu_DrawArrow(x, y, t->w, 0x12, col, 1);
    Menu_DrawArrow(x + t->lx, y, t->w, 0x12, col, 0);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos((s16)(x + t->tx), y);
    Text_PrintfColored(gPageFmt, page + 1, n);
}

void *Box_BuildSortedList(u8 equip);
extern Panel gBoxItemListWin;
extern Panel gBoxEquipGridWin;
extern u32 gBoxPageColors[];
void Window_DrawTextAlpha(Window *w, u8 *text, s32 alpha); /* unit's own declaration: with the definition's void (Window *w, u8 *text, u8 alpha) (sub_main/unk_356380.c), BoxMenu_DrawItemList 3 of 941 words, BoxMenu_DrawPageTabs 1 of 155 words, BoxMenu_DrawEquipGrid 3 of 455 words differ */
s32 List6_Contains(u16 *, s32);
s32 Prim_FillRect(struct PrimLine *r);
void Equip_GetIconUV(EquipItem *e, struct Pt *, struct Pt *);
u8 Equip_GetRarity(u8 type, u16 id);
/* unit's own declaration: the lobby's BoxMenu functions match only with (u8, s32, u8) (the game
   doesn't mask alpha); the definition's (u8, u8, s32) adds an andi at each call */
u32 Equip_Color(u8 idx, s32 alpha, u8 alt); /* unit's own declaration: with the definition's u32 (u8 idx, u8 alpha, s32 alt) (sub_main/unk_3549F0.c), BoxMenu_DrawItemList 7 of 941 words, BoxMenu_DrawEquipGrid 5 of 455 words, BoxMenu_DrawEquipList 46 of 517 words differ */
s32 Equip_ArmorFits(EquipItem *e, u16 *slot);
char *Equip_GetName(u8 type, u16 id);
void BoxMenu_DrawPageTabs(s16 x, s16 y, s16 page, u32 pad, s32 arg4, u8 kind);

void BoxMenu_DrawEquipGrid(f32 fx, s16 y, s16 cur, u32 flags)
{
    EquipItem *box;
    u16 page;
    s16 n;
    s32 sel;
    s32 hl;
    s16 i;
    s32 alt;
    EquipItem *e;
    u32 *uv1;
    u16 idx;
    s32 dim;
    f32 bx;
    f32 x0;
    s32 k;
    s32 k2;
    s32 alpha;
    BoxLine r;
    Sprite s;
    s32 hl2;
    u8 rare;
    SaveData *save;
    u16 *eq;

    save = &gSave;
    alpha = (u8)((flags & 0x20) == 0 ? 0xFF : 0x60);
    if (!(flags & 0x100)) {
        box = save->box;
    } else {
        box = Box_BuildSortedList(1);
        if (cur >= 0) {
            cur = cur % 100;
        }
    }
    gBoxItemListWin.x = y;
    eq = save->equipIdx;
    Window_DrawTextAlpha((Window *)&gBoxEquipGridWin, NULL, alpha);
    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    page = cur / 100;
    idx = page * 100;
    k = idx;
    n = cur % 100;
    for (i = 0; i < 100; i++, k++) {
        r.color = ((u8)alpha << 24) | gBoxPageColors[page];
        if (!(flags & 0x100) && List6_Contains(save->equipIdx, k) == 1) {
            r.color = ((u8)alpha << 24) | 0x1078D4;
        }
        r.x0 = (s16)(117.0f + fx - 116.0f + 25.0f * (i % 10)) + 1;
        r.x1 = 1.0f + (22.2f + r.x0);
        r.y0 = y + (i / 10) * 17 + 1;
        r.y1 = r.y0 + 0x10;
        Prim_FillRect((struct PrimLine *)&r);
    }
    s.w = 0x1B;
    s.h = 0x11;
    e = &box[idx];
    /* a pointer from the permuter: 29 words differ without it */
    uv1 = (u32 *)&s.u1;
    x0 = 117.0f + fx;
    bx = x0 - 116.0f;
    for (i = 0, k2 = idx; i < 100; i++, k2++, e++) {
        if (e->used) {
            s.x = bx + 25.0f * (i % 10);
            s.y = y + (i / 10) * 17;
            Equip_GetIconUV(e, (struct Pt *)&s.u0, (struct Pt *)&s.u1);
            rare = Equip_GetRarity(e->type, e->id);
            if (flags & 0x2000000) {
                s.color = Equip_Color(rare, alpha, 0);
                if (List6_Contains(eq, k2) == 1) {
                    if (e->type == 6 || e->type == 7) {
                        s.color = Equip_Color(rare, alpha, 1);
                        Sprite_Draw((struct Sprite *)&s);
                        s.color = ((u8)alpha << 24) | 0x808010;
                    } else {
                        Sprite_Draw((struct Sprite *)&s);
                        s.color = ((u8)alpha << 24) | 0xFFFF00;
                    }
                    *(u32 *)&s.u0 = 0x01290061;
                    *uv1 = 0x013F007F;
                } else if (!Equip_ArmorFits(e, eq)) {
                    s.color = Equip_Color(rare, alpha, 1);
                }
            } else {
                s.color = Equip_Color(rare, alpha, 0);
                if (!(flags & 0x100) && List6_Contains(eq, k2) == 1) {
                    Sprite_Draw((struct Sprite *)&s);
                    *(u32 *)&s.u0 = 0x01290061;
                    *uv1 = 0x013F007F;
                    s.color = ((u8)alpha << 24) | 0xFFFF00;
                }
            }
            Sprite_Draw((struct Sprite *)&s);
        }
    }
    if (cur >= 0) {
        e = &box[cur];
        Text_SetColor((u8)((flags & 0x20) == 0 ? 0 : 10));
        if (!(flags & 0x100)) {
            s.x = (s16)(x0 - 116.0f + 25.0f * (n % 10)) - 1;
            s.y = y + (n / 10) * 17 - 1;
            s.h = 0x13;
            BoxMenu_DrawPulseSprite((u8 *)&s, alpha);
            Font_SetSpacing(0xE, 0x12);
            Font_SetPos(fx, y + 0xAC);
            if (!e->used) {
                Text_Draw(*(char * *)&gItemNames);
            } else {
                Text_Draw(Equip_GetName(e->type, e->id));
            }
        }
        BoxMenu_DrawPageTabs(146.0f + fx, y + 0xAC, page, flags, alpha, 1);
    }
}

extern s8 gBoxTypeKinds[];
s16 Box_GetPageCount(SaveData *save);
void BoxList_InsertSorted(u16 *list, u16 idx, u16 n);

/* The box list gBoxList of one equipment class (kind < 6, by
 * gBoxTypeKinds[type]): the box indices in box order or sorted
 * (BoxList_InsertSorted), the count (+0x3E8) and the pages of 7 (+0x3ED,
 * +0x3EE the last page's count); returns the pages, -1 for a bad kind. */
s8 BoxList_Build(u8 kind, u8 sorted)
{
    BoxList *base = (BoxList *)&gBoxList; /* the cast keeps the address in a register */
    EquipItem *e;
    u32 i;
    u32 k;
    u32 n;
    u16 *p;

    if (kind > 5) {
        base->invalid = 1;
        return -1;
    }
    base->invalid = 0;
    base->kind = kind;
    base->sorted = sorted;
    e = gSave.box;
    p = base->idx;
    n = Box_GetPageCount(&gSave) * 100;
    for (i = 0, k = 0; i < n; i++, e++) {
        if (e->used != 0 && gBoxTypeKinds[e->type] == kind) {
            if (base->sorted == 0) {
                *p++ = i;
            } else {
                BoxList_InsertSorted(base->idx, i, k);
            }
            k++;
        }
    }
    base->count = k;
    if (k != 0) {
        base->pages = (k + 6) / 7;
        base->lastCount = k - (base->pages - 1) * 7;
    } else {
        base->lastCount = 0;
        base->pages = 0;
    }
    return base->pages;
}

u32 BoxEquip_SortKey(EquipItem *);

void BoxList_InsertSorted(u16 *list, u16 idx, u16 n)
{
    SaveData *save = &gSave;
    u32 key;
    u16 t;

    key = BoxEquip_SortKey(&save->box[idx]);
    while (n > 0) {
        if (key < BoxEquip_SortKey(&save->box[*list])) {
            while (n > 0) {
                t = *list;
                n--;
                *list = idx;
                idx = t;
                list++;
            }
            break;
        }
        n--;
        list++;
    }
    *list = idx;
}

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

/* The box list's cursor (row and page of 7) moved by the pad bits
 * (0x800/0x400 pages, 0x2000/0x1000 rows, wrapping), with its sound;
 * returns the box index there, -1 for an empty list. */
s16 BoxList_MoveCursor(u8 *row, u8 *page, u16 pad)
{
    BoxList *base = (BoxList *)&gBoxList; /* the cast keeps the address in a register */
    s16 p;
    s16 r;
    s16 last;

    if (base->invalid != 0 || base->count == 0) {
        *row = 0;
        *page = 0;
        return -1;
    }
    r = *row;
    p = *page;
    if ((pad & 0xC00) && base->pages > 1) {
        if (pad & 0x800) {
            if (--p < 0) {
                p = base->pages - 1;
            }
        } else if (++p >= base->pages) {
            p = 0;
        }
        if (p == base->pages - 1 && r >= base->lastCount) {
            *row = r = base->lastCount - 1;
        }
        *page = p;
        Sound_PlaySeFlat(0, 0x11, 0);
    } else if (pad & 0x3000) {
        last = p >= base->pages - 1 ? base->lastCount : 7;
        if (pad & 0x2000) {
            if (--r < 0) {
                r = last - 1;
            }
        } else if (++r >= last) {
            r = 0;
        }
        *row = r;
        Sound_PlaySeFlat(0, 0x17, 0);
    }
    return base->idx[r + p * 7];
}

u16 BoxList_GetPage(u16 **list, u8 page)
{
    BoxList *base = (BoxList *)&gBoxList; /* the cast keeps the address in a register */
    s32 last;
    s16 n;

    if (base->invalid != 0 || base->count == 0) {
        return 0;
    }
    last = base->pages - 1;
    n = 7;
    if (page >= last) {
        page = last;
        n = base->lastCount;
    }
    *list = &base->idx[page * 7];
    return n;
}

void Gfx_SetTexFilter(s32);
void Tex_Prepare(s32, s32);
void Tex_Bind(s32);
void Menu_DrawArrow(s16, s16, s16, s16, u32, u8);

/* The blinking mark of an equipment line: x by the item's type, the colour
 * pulsing with the frame counter (sub_main 0x6AE188). */
void EquipLine_DrawMark(s16 x, s16 y, u8 *e)
{
    f32 s;
    u8 g;

    switch (e[1]) {
    default:
        x += 0x59;
        break;
    case 2:
        x += 0x79;
        break;
    case 3:
        x += 0x99;
        break;
    case 4:
        x += 0xB9;
        break;
    case 5:
        x += 0xD9;
        break;
    case 0:
        x += 0xF9;
        break;
    }
    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s = Math_Sin(0.0000958738f * (u32)(u16)((gFrameTick & 0x1F) << 11));
    g = (s8)(32.0f * s) + 0x20;
    Menu_DrawArrow(x, y + 4, 0x20, 0x10, 0xFF000000 | (g << 16) | (g << 8) | (u8)((s8)(31.0f * s) + 0xDF), 3);
}

extern u8 gSkillPreview;
extern char *gStatusPageFmt[];
extern s16 gEquipDetailIcons[];
extern ListPanel gEquipDetailPanel;
void EquipDetail_DrawNone(s16 x, s16 y);
void SkillList_DrawPage(s16 x, s16 y, s32 k, s32 n); /* unit's own declaration: with the definition's void (s16 x, s16 y, u8 page, u8 mark) (sub_main/unk_356380.c), EquipDetail_Draw 1 of 133 words differ */
void EquipLine_DrawMark(s16 x, s16 y, u8 *e);
void SkillList_DrawWithEquip(Player *pl, EquipItem *e, s16 x, s16 y);
void Player_DrawSkillsPlain(Player *a, s16 x, s16 y); /* unit's own declaration: with the definition's void (Player *a, s32 b, s16 c) (sub_main/unk_356380.c), EquipDetail_Draw 6 of 133 words differ */

/* The equipment detail window at (x, y): the title ("n/m" of the box pages
 * when there are several, flags bit 31 the arrows), the item line, then
 * the details (gSkillPreview +4) or the skills against the wearer (SkillList_DrawCompare)
 * or Player_DrawSkillsPlain. */
void EquipDetail_Draw(Player *a, s16 x, s16 y, u32 flags)
{
    u8 *d = (u8 *)&gSkillPreview;
    char buf[0x40];
    s32 k;
    u32 top;
    s32 yy;

    if (d[0x11] == 0) {
        EquipDetail_DrawNone(x, y);
        return;
    }
    gEquipDetailPanel.x = x;
    gEquipDetailPanel.y = y;
    k = flags & 0xF;
    if (d[0x10] > 1) {
        top = flags & 0x80000000;
        sprintf(buf, gStatusPageFmt[(u8)(top ? 0xA : 0xD)], (u8)k + 1, d[0x10]);
        ListPanel_Draw(&gEquipDetailPanel, 0, buf, -1);
        Menu_DrawFrame(&gEquipDetailPanel, 0);
        if (top) {
            Draw_Icons((struct Icon *)gEquipDetailIcons, 2);
        }
    } else {
        ListPanel_Draw(&gEquipDetailPanel, 0, gSkillDetailTitle, -1);
    }
    SkillList_DrawPage(x, y, k, d[0x12]);
    if (d[4] != 0) {
        EquipLine_DrawMark(x, y + 0x14, d + 4);
    }
    gEquipDetailWin[0].x = x;
    gEquipDetailWin[0].y = yy = y + 0xE8;
    Window_DrawText(&gEquipDetailWin[0], (s16 *)0, (u8 *)gActiveSkillsTitle);
    if (d[4] != 0) {
        SkillList_DrawWithEquip(a, (EquipItem *)(d + 4), x, yy);
    } else if (*(Player **)d != NULL) {
        SkillList_DrawCompare(*(Player **)d, x, yy);
    } else {
        Player_DrawSkillsPlain(a, x, yy);
    }
}

void EquipDetail_DrawNone(s16 x, s16 y)
{
    gEquipNonePanel.x = x;
    gEquipNonePanel.y = y;
    ListPanel_Draw(&gEquipNonePanel, 0, gSkillDetailTitleDim, -1);
    gEquipDetailWin[1].x = x;
    gEquipDetailWin[1].y = ((s16)(y)) + 0xE8;
    Window_DrawText(&gEquipDetailWin[1], (s16 *)0, (u8 *)gActiveSkillsTitle);
}

void BoxMenu_DrawControls(s32 arg0)
{
    s32 temp_a0;

    temp_a0 = arg0 & 0xFF;
    switch (temp_a0) {
    case 0:
        Gfx_DrawPulseGradient(0, 0x90, 0xFA, 0x24, 0x80006000, 0);
        Draw_Icons((struct Icon *)gBoxControlIcons, 3);
        Text_SetColor(0);
        Font_SetSpacing(0x10, 0xE);
        Font_SetPos(0x28, 0xA4);
        Text_Draw((char *) (gStartText));
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x3C, 0x90);
        Text_Draw(gCameraGuideTexts[0]);
        Font_SetPos(0x6E, 0xA2);
        Text_Draw((char *) D_0080CA3C);
        break;
    case 1:
        Gfx_DrawPulseGradient(0x1A4, 0x16D, 0xDC, 0x24, 0x80006000, 1);
        Draw_Icons((struct Icon *)D_007F1C78, 3);
        Text_SetColor(0);
        Font_SetSpacing(0x10, 0xE);
        Font_SetPos(0x1C8, 0x180);
        Text_Draw((char *) (gStartText));
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x1DC, 0x16C);
        Text_Draw(gCameraGuideTexts[0]);
        Font_SetPos(0x20E, 0x17E);
        Text_Draw(gMenuShowTexts[0]);
        break;
    case 2:
        Gfx_DrawPulseGradient(0, 0xE8, 0xFA, 0x24, 0x80006000, 0);
        Draw_Icons((struct Icon *)gBoxControlIconsB, 3);
        Text_SetColor(0);
        Font_SetSpacing(0x10, 0xE);
        Font_SetPos(0x28, 0xFC);
        Text_Draw((char *) (gStartText));
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x3C, 0xE8);
        Text_Draw(gCameraGuideTexts[0]);
        Font_SetPos(0x6E, 0xFA);
        Text_Draw((char *) D_0080CA3C);
        break;
    }
}

void BoxMenu_DrawActions(s32 arg0)
{
    s16 *pos;

    pos = Shake_GetOffset(2);
    if (Shake_Value(2) == 0) {
        BoxMenu_DrawControls(0);
    }
    if ((u8)arg0 == 0) {
        gBoxActionsPanel.cursorColor = 0xA9182;
    } else {
        gBoxActionsPanel.cursorColor = 0x808080;
    }
    ListPanel_Draw(&gBoxActionsPanel, pos, 0, gBoxWork->menu);
}

void BoxMenu_Draw(void)
{
    u8 temp_a0;
    u8 temp_a0_2;
    u8 temp_v1;

    if ((gBoxWork->unk0 != 0) && (Town_IsMode(0x1D) != 0)) {
        if (!(gBoxWork->inputOn & 1)) {
            BoxMenu_DrawControls(1);
            return;
        }
        if ((gHelpShow != 0) && ((temp_a0 = gBoxWork->help, ((temp_a0 & 0x80) != 0)) || (temp_a0 == 0))) {
            HelpWindow_DrawCurrent();
            if ((gBoxWork->blink & 0x1F) < 0x14) {
                Draw_Icons((struct Icon *)gBoxMenuIcon, 1);
            }
            if (gBoxWork->yesNo != 0xFF) {
                BoxMenu_DrawModeName();
            }
        }
        temp_v1 = gBoxWork->state;
        switch (temp_v1) {
        case 0:
            BoxMenu_DrawActions(0);
            return;
        case 1:
            temp_a0_2 = gBoxWork->menu;
            switch (temp_a0_2) {
            case 0:
                BoxMenu_DrawStore();
                return;
            case 1:
                BoxMenu_DrawTake();
                return;
            case 2:
                BoxMenu_DrawEquip();
                return;
            case 3:
                BoxMenu_DrawArrange();
                return;
            case 4:
                BoxMenu_DrawSell();
                return;
            }
            break;
        case 2:
            BoxMenu_DrawActions(1);
            break;
        }
    }
}

void BoxMenu_DrawStore(void)
{
    s32 var_a1;
    s32 flags;
    u8 temp_a3;

    BoxMenu_DrawActions(1);
    var_a1 = 0;
    temp_a3 = gBoxWork->step;
    flags = 0x80;
    switch (temp_a3) {
    case 0:
        flags = 0x10;
        switch (gBoxWork->prompt) {
        case 0:
            if (Shake_Value(1) == 0) {
                BoxMenu_DrawHelp(gBoxWork->help, gBoxWork->pouch);
            }
            var_a1 = 0xA9182;
            flags = 0xA8;
            break;
        }
    case 2:
        ItemList_DrawPouch((s32) gBoxWork->pouch, var_a1, flags);
        break;
    case 1:
        BoxMenu_DrawList306((s32) gBoxWork->slots[0], (gBoxWork->pouchItem & 0x3FF) << 0xD);
        break;
    }
}

void BoxMenu_DrawAmount(void);
void ButtonHint_DrawSort(u8 i);

void BoxMenu_DrawTake(void)
{
    s32 f;
    s32 step;

    BoxMenu_DrawActions(1);
    switch (step = gBoxWork->step) {
    case 0:
        if (gBoxWork->help != 0) {
            f = 8;
        } else if (gBoxWork->prompt != 0) {
            f = 0x100;
        } else {
            f = 0;
            ButtonHint_DrawDetails();
            ButtonHint_DrawSort(0);
        }
        BoxMenu_DrawList306(gBoxWork->slots[0], f);
        break;
    case 1:
        BoxMenu_DrawList306(gBoxWork->slots[0], 0x20);
        ItemList_DrawPouch(gBoxWork->pouch, 0xA9182, 8);
        BoxMenu_DrawHelp(gBoxWork->help, gBoxWork->pouch);
        break;
    case 2:
        BoxMenu_DrawAmount();
        BoxMenu_DrawList306(gBoxWork->slots[0], 0x20);
        ItemList_DrawPouch(gBoxWork->pouch, 0x808080, 0);
        break;
    case 3:
    case 4:
        BoxMenu_DrawList306(gBoxWork->slots[0], 0x20);
        ItemList_DrawPouch(gBoxWork->pouch, 0x808080, 0);
        break;
    }
}

/* The box slots of the six equipped items (0xFFFF: none) */
typedef struct EquipIdx {
    u16 idx[6];
} EquipIdx;


extern ListWin gBoxEquipPanel;
extern ListPanel gBoxEquipRows;
extern char *gEquipSlotTexts[];
extern char *D_0080CA9C;
extern char *D_0080CAA4;
extern char *gBackGuideTexts[];
extern char *D_0080CAAC;
extern char *gDiamondMarkTexts[];
extern char *D_0080CAB4;
extern u32 gPlayerColorWords[];
extern char *gSymbolColorNames[];
void BoxMenu_DrawSkillHint(void);
void EquipCompare_Draw(EquipItem *a, EquipItem *b, s32 c, s16 y, u8 flags);
void StatusPage_DrawBoxes(s32 a, s16 y, s32 c);
void EquipWindow_DrawLocal(struct EquipItem *e, s16 x, s16 y, u8 flags, struct EquipItem *p);
u32 Color_Cycle8(void);
u32 Color_Half(u32 c);
void Font_SetColor(s32 color);
u8 BoxMenu_DrawEquipList(u16 *slot, s32 kind, s32 flags);
void EquipInfo_DrawPage(s16, s32, s32, s32);
void BoxMenu_DrawEquipInfo(u16 *arg0, s32 arg1, s32 arg2, s32 arg3);
void BoxMenu_DrawEquipPick(EquipIdx *cur, s8 page, u32 flags);
struct ListWin;
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel);

void BoxMenu_DrawEquip(void)
{
    SaveData *save = &gSave;
    u32 flags;
    s16 cur;
    u8 f;
    u32 i;
    s16 y;
    s16 step;

    Font_SetSpacing(0x12, 0x12);
    switch (gBoxWork->step) {
    case 0:
        BoxMenu_DrawActions(1);
        gBoxEquipPanel.lines = gBoxWork->setEntries;
        ListPanel_DrawDim(&gBoxEquipPanel, 0, gBoxWork->kind);
        break;
    case 1:
        switch (gBoxWork->kind) {
        case 0:
            if (gChatSendTo == 0 || gChatWinTimer == 0) {
                BoxMenu_DrawControls(2);
            }
            switch (gBoxWork->sub) {
            case 0:
                if (gBoxWork->prompt != 0) {
                    flags = 0x100;
                } else {
                    flags = 0x2000000;
                    if (gBoxWork->help == 0) {
                        BoxMenu_DrawSkillHint();
                        ButtonHint_DrawSort(2);
                    } else {
                        Font_SetPos(0x21C, 0x18A);
                        Text_Printf(D_0080CA9C);
                    }
                }
                break;
            case 1:
            case 2:
                flags = 0x2000000;
                break;
            }
            if (gBoxWork->help == 0) {
                f = gBoxWork->levelSel | 0x40;
                if (gBoxWork->cursorClass == 0) {
                    f |= 0x10;
                    if (gBoxWork->canEquip == 0) {
                        f |= 8;
                    }
                }
                if (gBoxWork->boxItem != NULL) {
                    if (gBoxWork->boxItem->used != 0) {
                        EquipCompare_Draw(gBoxWork->equipped, gBoxWork->boxItem, 0x132, 0x38, f);
                    } else {
                        StatusPage_DrawBoxes(0x132, 0x38, 0xFF);
                    }
                } else {
                    EquipCompare_Draw(gBoxWork->equipped, NULL, 0x132, 0x38, f);
                }
            } else if (gBoxWork->canEquip != 0) {
                EquipDetail_Draw(gBoxWork->player, 0x132, 0x3E, gBoxWork->skillPage | 0x80000000);
            } else {
                EquipDetail_DrawNone(0x132, 0x3E);
            }
            BoxMenu_DrawEquipGrid(28.0f, 0x1C, (s16)gBoxWork->slots[1], flags);
            break;
        case 2:
            flags = 0;
            switch (gBoxWork->sub) {
            case 0:
                flags = 3;
                break;
            case 1:
                break;
            }
            BoxMenu_DrawActions(1);
            BoxMenu_DrawEquipInfo(save->equipSets[gBoxWork->set].idx, -1, flags, (s32)gBoxWork->player);
            break;
        case 1:
            switch (gBoxWork->sub) {
            case 0:
                BoxMenu_DrawActions(1);
                BoxMenu_DrawEquipInfo(save->equipSets[gBoxWork->set].idx, -1, 3, 0);
                break;
            case 1:
                switch (gBoxWork->sub2) {
                case 0:
                    BoxMenu_DrawEquipInfo(gBoxWork->cur.idx, (s8)gBoxWork->part, 6, 0);
                    cur = -1;
                    flags = 1;
                    break;
                case 1:
                    BoxMenu_DrawEquipList(gBoxWork->cur.idx, gBoxWork->part, 0xE);
                    Font_SetSpacing(0x12, 0x12);
                    Text_SetColor(0);
                    Font_SetPos(0x1F8, 0x18E);
                    if (gBoxWork->help == 0) {
                        Text_Printf(D_0080CAA4);
                        EquipInfo_DrawPage(0xDA, 1, 0, 0);
                    } else {
                        Text_Printf(gBackGuideTexts[0]);
                        EquipWindow_DrawLocal((struct EquipItem *)gBoxWork->boxItem, 0x132, 0xDA, (gBoxWork->levelSel & 7) | 0x40, (struct EquipItem *)NULL);
                    }
                    flags = 0;
                    cur = (s16)gBoxWork->row;
                    if (*(u16 *)&gHelpLine == 0xFFFF) {
                        Font_SetSpacing(0x12, 0x12);
                        Text_SetColor(0);
                        Font_SetPos(0x132, 0x166);
                        Text_PrintfColored(D_0080CAAC, gEquipSlotTexts[gBoxWork->part]);
                    }
                    break;
                case 2:
                    BoxMenu_DrawEquipList(gBoxWork->cur.idx, gBoxWork->part, 4);
                    EquipInfo_DrawPage(0xDA, 1, 0, 0);
                    cur = -1;
                    flags = 1;
                    break;
                }
                BoxMenu_DrawEquipPick((EquipIdx *)gBoxWork->cur.idx, cur, flags);
                break;
            case 2:
            case 3:
            case 4:
            case 5:
                BoxMenu_DrawActions(1);
                EquipInfo_DrawPage(0xC6, BoxMenu_DrawEquipList(gBoxWork->cur.idx, -1, 0), 0, 0);
                break;
            }
            break;
        case 3:
            BoxMenu_DrawActions(1);
            switch (gBoxWork->sub) {
            case 0:
                BoxMenu_DrawEquipInfo(save->equipSets[gBoxWork->set].idx, -1, 3, 0);
                break;
            case 1:
                BoxMenu_DrawEquipInfo(save->equipSets[gBoxWork->set].idx, -1, 0, 0);
                break;
            default:
                EquipInfo_DrawPage(0xC6, BoxMenu_DrawEquipList(gBoxWork->cur.idx, -1, 0), 0, 0);
                break;
            }
            break;
        case 4:
            BoxMenu_DrawActions(1);
            step = gBoxEquipRows.charH + 4;
            y = gBoxEquipRows.y + step + 2;
            for (i = 0; i < gBoxWork->count; i++) {
                Font_SetSpacing(0x14, 0x14);
                Font_SetPos(gBoxEquipRows.x - 1, y - 1);
                Font_SetColor(0x807F7F7F);
                Text_PrintfColored(gDiamondMarkTexts[0]);
                Font_SetSpacing(0x12, 0x12);
                Font_SetPos(gBoxEquipRows.x, y);
                if (i != 0xB) {
                    Font_SetColor(Color_Half(gPlayerColorWords[gBoxWork->list[i]]));
                } else {
                    Font_SetColor(Color_Half(Color_Cycle8()));
                }
                Text_PrintfColored(gDiamondMarkTexts[0]);
                Font_SetPos(gBoxEquipRows.x + 0x14, y);
                Font_SetColor(0x807F7F7F);
                Text_Draw(gSymbolColorNames[gBoxWork->list[i]]);
                y += step;
            }
            gBoxEquipRows.lines = gBoxWork->count;
            ListPanel_Draw(&gBoxEquipRows, 0, D_0080CAB4, gBoxWork->pick);
            if (gBoxWork->help == 0) {
                ButtonHint_DrawDetails();
            }
            break;
        }
        break;
    }
}

u32 Color_Half(u32 c)
{
    ((u8 *)&c)[0] >>= 1;
    ((u8 *)&c)[1] >>= 1;
    ((u8 *)&c)[2] >>= 1;
    ((u8 *)&c)[3] >>= 1;
    return c;
}

void BoxMenu_DrawListHeader(u8 *slot, s32 flags, u8 arg2)
{
    ListPanel *p;

    p = &gBoxListHeaders[arg2];
    ListPanel_Draw(p, 0, (char *)&D_0083B348, -1);
    if (flags & 1) {
        Menu_DrawFrame(p, 0);
    }
    Font_SetPos(0x132, 0x1C);
    Text_Printf(gColorRowFmtTexts[0], slot + 0xC, gBoxWork->set + 1);
}

extern char *D_0080CABC;
extern char *gRegisterLabelTexts[];
extern u8 gBoxEquipListU[];
extern char **gArmorNameTables[];
void BoxMenu_DrawListHeader(u8 *arg0, s32 arg1, u8 arg2);
void Equip_GetIconUV(EquipItem *e, struct Pt *, struct Pt *);
u8 Equip_GetRarity(u8 type, u16 id);
/* unit's own declaration: the lobby's BoxMenu functions match only with (u8, s32, u8) (the game
   doesn't mask alpha); the definition's (u8, u8, s32) adds an andi at each call */
u32 Equip_Color(u8 idx, s32 alpha, u8 alt); /* unit's own declaration: with the definition's u32 (u8 idx, u8 alpha, s32 alt) (sub_main/unk_3549F0.c), BoxMenu_DrawItemList 7 of 941 words, BoxMenu_DrawEquipGrid 5 of 455 words, BoxMenu_DrawEquipList 46 of 517 words differ */
char *Equip_GetName(u8 type, u16 id);
s32 Prim_FillRect(struct PrimLine *r);
struct PrimLine;
s32 Prim_DrawLine(struct PrimLine *);
void Font_SetColor(s32 color);

/* Compiled at level 3 (a pragma pair in this level 4 file, as Layout_TextFull; most likely a
 * boundary between two of Capcom's source files that the split merged): the level 3 patterns
 * are the colour ternaries, which MWCC at level 3 selects into v0 and masks into c (andi s6),
 * where level 4 writes them straight into c, and the name colour's `== 0 ||` branches.
 * Round 29: the second list's row y is the loop optimizer's i * 0x15 + 0x49 (its step is
 * scheduled between the table walkers', as the game's). */
#pragma optimization_level 3
u8 BoxMenu_DrawEquipList(u16 *slot, s32 kind, s32 flags)
{
    EquipItem *e;
    u32 i;
    u8 has;
    u16 v;
    SaveData *save;
    s16 off;
    s32 x;
    s16 yy;
    s16 y;
    u8 mask;
    u8 sel;
    char *name;
    u8 bit;
    u8 *id;
    u8 c;
    u8 ids[6];
    u8 idx[5];
    BoxLine r;
    BoxLine l;
    Sprite s;
    ArmorData *tab[5];

    gBoxListHeaders[0].lines = 6;
    if (flags & 4) {
        gBoxListHeaders[0].lines = 7;
    }
    BoxMenu_DrawListHeader((u8 *)slot, flags, 0);
    if (slot[0] != 0xFFFF) {
        save = &gSave;
        e = &save->box[slot[0]];
        id = ids;
        if (slot[2] == 0xFFFF) {
            v = 0;
        } else {
            v = save->box[slot[2]].id;
        }
        ids[2] = v;
        if (slot[3] == 0xFFFF) {
            v = 0;
        } else {
            v = save->box[slot[3]].id;
        }
        ids[3] = v;
        if (slot[4] == 0xFFFF) {
            v = 0;
        } else {
            v = save->box[slot[4]].id;
        }
        ids[4] = v;
        if (slot[5] == 0xFFFF) {
            v = 0;
        } else {
            v = save->box[slot[5]].id;
        }
        ids[5] = v;
        if (slot[1] == 0xFFFF) {
            v = 0;
        } else {
            v = save->box[slot[1]].id;
        }
        ids[0] = v;
        has = 1;
    } else {
        e = NULL;
        id = NULL;
        has = 0;
    }
    if (kind >= 0) {
        off = 0;
        x = 0x144;
        if (!(flags & 8) && (gFrameTick & 0x10)) {
            off = 5;
        }
        yy = kind * (kind != 6 ? 0x15 : 0x16) + 0x33;
        s.w = 0x12;
        s.h = 0x14;
        *(u32 *)&s.u0 = 0x6E007C;
        *(u32 *)&s.u1 = 0x7D0070;
        s.x = x - off + 1;
        s.y = yy;
        s.color = 0xFF000000;
        Sprite_Draw((struct Sprite *)&s);
        s.x -= 2;
        s.y -= 1;
        s.color = 0xFF00A3FF;
        Sprite_Draw((struct Sprite *)&s);
        s.u0 = 0x71;
        s.u1 = 0x7D;
        s.x = off + 0x240;
        s.y = yy;
        s.color = 0xFF000000;
        Sprite_Draw((struct Sprite *)&s);
        s.x -= 2;
        s.y -= 1;
        s.color = 0xFF00A3FF;
        Sprite_Draw((struct Sprite *)&s);
    }
    Gfx_SetTexFilter(1);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    s.x = 0x156;
    s.w = 0x20;
    s.h = 0x14;
    s.y = y = 0x34;
    if (e != NULL) {
        if (e->type == 6) {
            mask = 4;
        } else {
            mask = 8;
        }
        sel = 0xFF;
        if ((flags & 8) && kind >= 0) {
            sel = 1 << kind;
        }
        idx[0] = id[2];
        idx[1] = id[3];
        idx[2] = id[4];
        idx[3] = id[5];
        idx[4] = id[0];
        Equip_GetIconUV(e, (struct Pt *)&s.u0, (struct Pt *)&s.u1);
        s.color = Equip_Color(Equip_GetRarity(e->type, e->id), 0xFF, !(sel & 1) ? 1 : 0);
        name = Equip_GetName(e->type, e->id);
    } else {
        mask = 0;
        idx[4] = 0;
        sel = 0;
        idx[3] = 0;
        idx[2] = 0;
        idx[1] = 0;
        idx[0] = 0;
        *(u32 *)&s.u0 = 0xE100C1;
        *(u32 *)&s.u1 = 0xFF00DF;
        s.color = Equip_Color(0, 0xFF, 1);
        name = Equip_GetName(0, 0);
    }
    Sprite_Draw((struct Sprite *)&s);
    tab[0] = &gArmorHeadData[idx[0]];
    tab[1] = &gArmorBodyData[idx[1]];
    tab[2] = &gArmorArmsData[idx[2]];
    tab[3] = &gArmorWaistData[idx[3]];
    tab[4] = &gArmorLegsData[idx[4]];
    s.v0 = 0x121;
    s.v1 = 0x13F;
    for (i = 0, bit = 2; i < 5; i++) {
        y += 0x15;
        s.y = y;
        s.u0 = gBoxEquipListU[i];
        s.u1 = s.u0 + 0x20;
        s.color = Equip_Color(tab[i]->rarity, 0xFF, !(sel & bit));
        Sprite_Draw((struct Sprite *)&s);
        bit += bit;
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(0x175, 0x34);
    c = (sel & 1) ? 0 : 10;
    Text_Printf(D_0080CABC, c, name);
    for (i = 0, bit = 2; i < 5; i++) {
        c = ((mask & tab[i]->flags) == 0 || (sel & bit) == 0) ? 10 : 0;
        Font_SetPos(0x175, i * 0x15 + 0x49);
        Text_Printf(D_0080CABC, c, gArmorNameTables[i][idx[i]]);
        bit += bit;
    }
    if (flags & 4) {
        r.x0 = 0x134;
        r.x1 = 0x263;
        r.y0 = 0xB5;
        r.y1 = 0xCB;
        r.color = kind != 6 ? 0x30A0E0C0 : 0x2C88FF88;
        Prim_FillRect((struct PrimLine *)&r);
        l.x0 = 0x134;
        l.x1 = 0x263;
        l.y0 = l.y1 = 0xB3;
        l.color = 0xE0E0E0E0;
        Prim_DrawLine((struct PrimLine *)&l);
        Font_SetColor(kind != 6 ? 0x807F7F7F : 0x807F212E);
        Font_SetPos(0x18C, 0xB7);
        Text_Draw(gRegisterLabelTexts[0]);
    }
    return has;
}
#pragma optimization_level 4

void EquipInfo_DrawPage(s16 y, s32 arg1, s32 page, M2C_UNK p)
{
    s32 temp_a0;
    s32 temp_v1;
    s32 text;

    text = arg1;
    gEquipInfoWin.y = y;
    if (text != 0) {
        temp_a0 = page & 0xFF;
        switch (temp_a0) {                          /* switch 1; irregular */
        case 0:                                     /* switch 1 */
            Window_DrawTextB2((Window *)&gEquipInfoWin, D_0080CAC4);
            SkillList_DrawCompare((Player *)p, 0x132, y);
            return;
        case 1:                                     /* switch 1 */
            Window_DrawTextB2((Window *)&gEquipInfoWin, (u8 *)gStatusHeadTexts[0]);
            StatusPage_DrawStats(&gPreviewPlayer, (Player *)p, 0x132, (((s16)(y)) + 0x14) & 0xFFFF);
            return;
        }
    } else {
        temp_v1 = page & 0xFF;
        switch (temp_v1) {                          /* switch 2; irregular */
        case 0:                                     /* switch 2 */
            text = (s32)D_0080CACC;
            break;
        case 1:                                     /* switch 2 */
            text = (s32)gStatusHeadAltTexts[0];
            break;
        }
        Window_DrawTextB2((Window *)&gEquipInfoWin, (u8 *)text);
    }
}

void EquipDetail_Draw(Player *a, s16 x, s16 y, u32 flags);
u8 BoxMenu_DrawEquipList(u16 *, s32, s32);
extern char *D_0080CAD4;

void BoxMenu_DrawEquipInfo(u16 *slot, s32 kind, s32 flags, s32 player)
{
    s16 x;
    s16 y;
    u8 t;

    x = 0xC6;
    if (flags & 4) {
        x = 0xDA;
    }
    switch (gBoxWork->levelSel) {
    case 0:
    case 1:
        t = BoxMenu_DrawEquipList(slot, (s8)kind, flags);
        EquipInfo_DrawPage(x, t, gBoxWork->levelSel, player);
        y = 0x18E;
        break;
    case 2:
        BoxMenu_DrawListHeader((u8 *)slot, flags, 1);
        if (*slot != 0xFFFF) {
            EquipDetail_Draw(&gPreviewPlayer, 0x132, 0x3E, gBoxWork->skillPage | 0x80000000);
        } else {
            EquipDetail_DrawNone(0x132, 0x3E);
        }
        y = 0x18A;
        break;
    }
    if (flags & 2) {
        Font_SetPos(0x1F8, y);
        Font_SetSpacing(0x12, 0x12);
        Text_SetColor(0);
        Text_Printf(D_0080CAD4);
    }
}

extern char *D_0080CAD8;
extern char *D_0080CADC;
extern char *D_0080CAE4;
extern ListPanel gBoxEquipPickPanel;
extern char **gEquipNameTables[];
/* Matched in round 30: the empty-list text colour has its own local (ecol; it had shared y,
 * the list's y position), and alpha is declared after y and p. Equip_Color is declared in the
 * block with its definition's prototype (u8 idx, u8 alpha, s32 alt: unk_3549F0.c), which gives
 * the game's `daddiu` alpha constants with a u8 alpha; the file's prototype (u8, s32, u8) is
 * kept for the other functions. */
void BoxMenu_DrawEquipPick(EquipIdx *cur, s8 page, u32 flags)
{
    EquipItem *e;
    u16 *list;
    u32 i;
    s16 tc;
    u16 eq;
    ArmorData *rec;
    u16 cq;
    u8 mask;
    Sprite spr;
    SaveData *save;
    u16 *eqs;
    s16 col;
    s16 ecol;
    char buf[64];
    ArmorData *tbl;
    u16 n;
    s16 y;
    u16 *p;
    u8 alpha;
    u32 Equip_Color(u8 idx, u8 alpha, s32 alt);

    if (gBoxWork->part < 6) {
        if (flags & 1) {
            ecol = 0xD;
            tc = 9;
            col = 9;
            alpha = 0x90;
        } else {
            tc = 5;
            col = 0;
            ecol = 2;
            alpha = 0xFF;
        }
        sprintf(buf, D_0080CAD8, tc, gEquipSlotTexts[gBoxWork->part]);
        ListPanel_Draw(&gBoxEquipPickPanel, 0, buf, page);
        n = BoxList_GetPage(&list, gBoxWork->page);
        if (n == 0) {
            Text_SetColor(ecol);
            Font_SetPos(0x1E, 0x30);
            Text_Draw(D_0080CADC);
            return;
        }
        if (gBoxWork->pageCount > 1 && !(flags & 1)) {
            Menu_DrawFrame(&gBoxEquipPickPanel, 0);
            Font_SetPos(0xD2, 0x1C);
            Text_SetColor(col);
            Text_PrintfColored(D_0080CAE4, gBoxWork->page + 1, gBoxWork->pageCount);
        }
        p = list;
        save = &gSave;
        eqs = gSave.equipIdx;
        Gfx_SetTexFilter(1);
        Tex_Prepare(1, 0x144);
        Tex_Bind(0x144);
        spr.x = 0x22;
        spr.w = 0x1C;
        spr.h = 0x12;
        y = 0x2F;
        if (gBoxWork->part == 0) {
            for (i = 0; i < n; i++) {
                e = &save->box[*p];
                spr.y = y;
                Equip_GetIconUV(e, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
                spr.color = Equip_Color(Equip_GetRarity(e->type, e->id), alpha, 0);
                Sprite_Draw((struct Sprite *)&spr);
                if (*p == eqs[0]) {
                    *(u32 *)&spr.u0 = 0x1200060;
                    *(u32 *)&spr.u1 = 0x1400080;
                    spr.color = ((u8)alpha << 24) | 0xFFFF20;
                    Sprite_Draw((struct Sprite *)&spr);
                }
                if (*p == cur->idx[0]) {
                    *(u32 *)&spr.u0 = 0;
                    *(u32 *)&spr.u1 = 0x200020;
                    spr.color = ((u8)alpha << 24) | 0xFFFF00;
                    Sprite_Draw((struct Sprite *)&spr);
                    tc = 10;
                } else {
                    tc = col;
                }
                Text_SetColor(tc);
                Font_SetPos(0x42, y);
                Text_PrintfColored(gEquipNameTables[e->type][e->id]);
                y += 0x14;
                p++;
            }
        } else {
        switch (gBoxWork->part) {
        case 1:
            eq = eqs[2];
            tbl = gArmorHeadData;
            cq = cur->idx[2];
            break;
        case 2:
            eq = eqs[3];
            tbl = gArmorBodyData;
            cq = cur->idx[3];
            break;
        case 3:
            eq = eqs[4];
            tbl = gArmorArmsData;
            cq = cur->idx[4];
            break;
        case 4:
            eq = eqs[5];
            tbl = gArmorWaistData;
            cq = cur->idx[5];
            break;
        case 5:
            eq = eqs[1];
            tbl = gArmorLegsData;
            cq = cur->idx[1];
            break;
        }
        mask = save->box[cur->idx[0]].type == 6 ? 4 : 8;
        for (i = 0; i < n; i++) {
            e = &save->box[*p];
            rec = &tbl[e->id];
            spr.y = y;
            Equip_GetIconUV(e, (struct Pt *)&spr.u0, (struct Pt *)&spr.u1);
            spr.color = Equip_Color(Equip_GetRarity(e->type, e->id), alpha, 0);
            Sprite_Draw((struct Sprite *)&spr);
            if (*p == eq) {
                *(u32 *)&spr.u0 = 0x1200060;
                *(u32 *)&spr.u1 = 0x1400080;
                spr.color = ((u8)alpha << 24) | 0xFFFF20;
                Sprite_Draw((struct Sprite *)&spr);
            }
            if (*p == cq) {
                *(u32 *)&spr.u0 = 0;
                *(u32 *)&spr.u1 = 0x200020;
                spr.color = ((u8)alpha << 24) | 0xFFFF00;
                Sprite_Draw((struct Sprite *)&spr);
            }
            Text_SetColor((rec->flags & mask) == 0 ? 10 : col);
            Font_SetPos(0x42, y);
            Text_PrintfColored(gEquipNameTables[e->type][e->id]);
            y += 0x14;
            p++;
        }
        }
    }
}

extern ListWin gBoxListPanel;
void ListPanel_DrawDim(struct ListWin *win, char *title, s32 sel);
void BoxMenu_DrawZenny(void);

void BoxMenu_DrawGridCursor(void);

void BoxMenu_DrawArrange(void)
{
    s32 f;

    BoxMenu_DrawActions(1);
    switch (gBoxWork->step) {
    case 0:
        gBoxListPanel.y = 0x5E;
        ListPanel_DrawDim(&gBoxListPanel, 0, gBoxWork->kind);
        break;
    case 1:
        if (gBoxWork->help != 0) {
            if (gBoxWork->kind == 0) {
                f = 8;
            } else {
                f = (gBoxWork->levelSel & 7) | 0x38;
            }
        } else if (gBoxWork->prompt != 0) {
            f = ((gBoxWork->kind & 1) << 4) | 0x100;
        } else {
            f = (gBoxWork->kind & 1) << 4;
            ButtonHint_DrawDetails();
            ButtonHint_DrawSort(0);
        }
        BoxMenu_DrawList306(((s16 *)gBoxWork->slots)[gBoxWork->kind], f);
        if ((gBoxWork->help == 0 || gBoxWork->kind == 0)
            && gBoxWork->sub == 1) {
            if (gBoxWork->held / 100
                == gBoxWork->slots[gBoxWork->kind] / 100) {
                BoxMenu_DrawGridCursor();
            }
        }
        break;
    }
}

/* The box menu's draw in the item and equipment modes: the page title
 * (step 0), else the list frame with its flags (item or equipment, the
 * cursor's kind), the amount or zenny window. ButtonHint_DrawDetails takes no
 * argument here (declared without a prototype). */
void BoxMenu_DrawSell(void)
{
    s32 f;
    u8 k;

    BoxMenu_DrawActions(1);
    switch (gBoxWork->step) {
    case 0:
        gBoxListPanel.y = 0x74;
        ListPanel_DrawDim(&gBoxListPanel, 0, gBoxWork->kind);
        break;
    case 1:
        k = gBoxWork->kind;
        f = k == 0 ? 0x01000080 : 0x01000290;
        switch (gBoxWork->sub) {
        case 0:
            if (gBoxWork->help != 0) {
                if (k == 0) {
                    f = 0x01000088;
                } else {
                    f = (gBoxWork->levelSel & 7) | 0x010002B8;
                }
            } else if (gBoxWork->prompt != 0) {
                f = ((k & 1) << 4) | 0x180;
            } else if (gBoxWork->msg < 0) {
                ButtonHint_DrawDetails();
                ButtonHint_DrawSort(0);
            }
            break;
        case 1:
            BoxMenu_DrawAmount();
            break;
        case 2:
            BoxMenu_DrawZenny();
            break;
        }
        BoxMenu_DrawList306(((s16 *)gBoxWork->slots)[gBoxWork->kind], f);
        break;
    }
}

void BoxMenu_DrawHelp(s32 arg0, s32 slot)
{
    if ((arg0 & 0xFF) == 1) {
        HelpWindow_Draw(1, (gSave.pouch[slot & 0xFF].item + 0x18) & 0xFFFF, *(s8 *)&gHelpColumn);
        return;
    }
    ButtonHint_DrawDetails();
}

void Tex_Prepare(s32, s32);
void Menu_DrawArrow(s16, s16, s16, s16, u32, u8);
void Gfx_SetTexFilter(s32);
void Tex_Bind(s32);

/* The amount to move (1-99, two full-width digits) with its up and down
 * arrows; 9999 means none. */
void BoxMenu_DrawAmount(void)
{
    char buf[5];

    if (gBoxWork->amount != 9999) {
        Font_SetSpacing(0x12, 0x12);
        Text_SetColor(0);
        Font_SetPos(0x23A, 0x17A);
        /* two full-width digits */
        buf[0] = 0x82;
        buf[1] = gBoxWork->amount / 10 + 0x4F;
        buf[2] = 0x82;
        buf[3] = gBoxWork->amount % 10 + 0x4F;
        buf[4] = 0;
        Text_Draw(buf);
        Gfx_SetTexFilter(1);
        Tex_Prepare(1, 0x155);
        Tex_Bind(0x155);
        if (gBoxWork->atMax == 0) {
            Menu_DrawArrow(0x240, 0x168, 0x18, 0x12, 0xFF20FF28, 2);
        }
        if (gBoxWork->amount != 1) {
            Menu_DrawArrow(0x240, 0x18E, 0x18, 0x12, 0xFF20FF28, 3);
        }
    }
}

/* The sum at +0x18 in full-width digits followed by the zenny sign, or the
 * text D_0080CA94 when it is 0. */
void BoxMenu_DrawZenny(void)
{
    u16 wide[16];
    char buf[16];
    u16 *d;
    char *s;
    s32 v;

    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(0x132, 0x166);
    v = gBoxWork->price;
    if (v != 0) {
        /* the number in full-width digits */
        sprintf(buf, "%d", v);
        d = wide;
        s = buf;
        while (*s != 0) {
            *d = ((*s + 0x1F) << 8) | 0x82;
            s++;
            d++;
        }
        d[0] = 0x9A82;
        d[1] = 0;
        Text_SetColor(5);
        Text_Printf(gPriceNoteTexts[0], wide);
    } else {
        Text_SetColor(0);
        Text_Draw((char *)D_0080CA94);
    }
}

void BoxMenu_DrawModeName(void)
{
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(0x1B0, 0x18E);
    Text_Printf(gYesNoTexts[gBoxWork->yesNo]);
}

s16 Box_GetCapacity(SaveData *save);
void *memset(void *dst, s32 c, u32 n);
extern u8 gSkillTryPlayer[];
u16 ItemList_Add(ItemStack *list, u16 item, u16 n);
struct SortPair;
void Sort_PairsDesc(struct SortPair *a, u32 n);
void BoxEquip_ShellSort(EquipItem *list, u32 n);

/* The box as a list in gSkillTryPlayer: the items (merged by ItemList_Add, then
 * sorted by Sort_PairsDesc) or the used equipment (BoxEquip_ShellSort). Returns the
 * list, or the item box itself when an item could not be listed. */
void *Box_BuildSortedList(u8 equip)
{
    s32 n;
    SaveData *save = &gSave;

    memset(gSkillTryPlayer, 0, 0x1770);
    n = Box_GetCapacity(save);
    if (equip == 0) {
        s32 k;
        ItemStack *s = save->itemBox;
        ItemStack *list = (ItemStack *)gSkillTryPlayer;
        s32 i;

        for (k = n; k != 0; k--, s++) {
            if (s->item != 0 && ItemList_Add(list, s->item, s->count) != 0) {
                return save->itemBox;
            }
        }
        for (i = 0; n != 0; n--, i++, list++) {
            if (list->item == 0) {
                break;
            }
        }
        Sort_PairsDesc((struct SortPair *)((ItemStack *)gSkillTryPlayer), i);
        return gSkillTryPlayer;
    } else {
        EquipItem *e = save->box;
        EquipItem *d = (EquipItem *)gSkillTryPlayer;
        s32 i = 0;

        for (; n != 0; n--, e++) {
            if (e->used) {
                *d = *e;
                d++;
                i++;
            }
        }
        BoxEquip_ShellSort((EquipItem *)gSkillTryPlayer, i);
        return gSkillTryPlayer;
    }
}


u16 Box_FindEquip(EquipItem *box, EquipItem *item);

void Box_ApplySort(u8 equip)
{
    SaveData *save = &gSave;
    void *buf;
    s32 n;
    u32 i;
    u16 *set;
    u16 *p;
    u32 k;

    buf = Box_BuildSortedList(equip);
    n = Box_GetCapacity(save);
    if (equip == 0) {
        ItemStack *s = (ItemStack *)buf;
        ItemStack *d = save->itemBox;

        for (; n != 0; n--) {
            *d++ = *s++;
        }
    } else {
        EquipItem *s;
        EquipItem *d;

        if (save->equipIdx[0] != 0xFFFF) {
            save->equipIdx[0] = Box_FindEquip(buf, &save->box[save->equipIdx[0]]);
        }
        if (save->equipIdx[1] != 0xFFFF) {
            save->equipIdx[1] = Box_FindEquip(buf, &save->box[save->equipIdx[1]]);
        }
        if (save->equipIdx[2] != 0xFFFF) {
            save->equipIdx[2] = Box_FindEquip(buf, &save->box[save->equipIdx[2]]);
        }
        if (save->equipIdx[3] != 0xFFFF) {
            save->equipIdx[3] = Box_FindEquip(buf, &save->box[save->equipIdx[3]]);
        }
        if (save->equipIdx[4] != 0xFFFF) {
            save->equipIdx[4] = Box_FindEquip(buf, &save->box[save->equipIdx[4]]);
        }
        if (save->equipIdx[5] != 0xFFFF) {
            save->equipIdx[5] = Box_FindEquip(buf, &save->box[save->equipIdx[5]]);
        }
        set = save->equipSets[0].idx;
        for (k = 10; k != 0; k--) {
            for (i = 0, p = set; i < 6; i++) {
                if (*p != 0xFFFF) {
                    *p = Box_FindEquip(buf, &save->box[*p]);
                }
                p++;
            }
            set += 0x10;
        }
        s = (EquipItem *)buf;
        d = save->box;
        for (; n != 0; n--) {
            *d++ = *s++;
        }
    }
}

u8 Equip_GetClass(EquipItem *e);
u8 Equip_GetSlotCount(EquipItem *item);
s32 Equip_FreeSlots(EquipItem *e);
extern s32 gBoxSortTypeKeys[];
extern s32 gBoxSortClassKeys[];

u32 BoxEquip_SortKey(EquipItem *e)
{
    u32 key;
    ArmorData *data;

    key = Equip_GetSlotCount(e);
    key -= Equip_FreeSlots(e);
    if (e->type == 6 || e->type == 7) {
        key += e->id * 16;
        if (e->type == 6) {
            key += Equip_GetData(e)->blade.unk15[0] << 16;
        } else {
            key += Equip_GetData(e)->gun.unk0E << 16;
        }
        key += gBoxSortClassKeys[Equip_GetClass(e)];
    } else {
        data = &Equip_GetData(e)->armor;
        key += e->level & 0xF;
        key += e->id << 8;
        key += data->unk12 << 16;
        key += gBoxSortTypeKeys[e->type];
    }
    return key;
}

/* Shell sort of the equipment list by BoxEquip_SortKey (gaps 1, 4, 13, ...). */
void BoxEquip_ShellSort(EquipItem *list, u32 n)
{
    s32 i;
    s32 j;
    s32 h;
    EquipItem t;
    u32 key;

    h = 1;
    while (h < n) {
        h = h * 3 + 1;
    }
    for (h /= 3; h > 0; h /= 3) {
        for (i = h; i < n; i++) {
            t = list[i];
            key = BoxEquip_SortKey(&t);
            for (j = i - h; j >= 0; j -= h) {
                if (key >= BoxEquip_SortKey(&list[j])) {
                    break;
                }
                list[j + h] = list[j];
            }
            list[j + h] = t;
        }
    }
}

s16 Box_GetCapacity(SaveData *save);

u16 Box_FindEquip(EquipItem *box, EquipItem *item)
{
    s16 cap;
    u16 i;

    cap = Box_GetCapacity(&gSave);
    if (item->used == 0) {
        return 0xFFFF;
    }
    for (i = 0; i < cap; i++, box++) {
        if (box->used != 0 && box->type == item->type && box->id == item->id && box->level == item->level
            && box->deco[0] == item->deco[0] && box->deco[1] == item->deco[1] && box->deco[2] == item->deco[2]) {
            return i;
        }
    }
    return 0xFFFF;
}

/* A player copied whole: the game's copy moves 16 bytes at a time (lq/sq),
 * as for a 16-aligned type (Player itself is 4-aligned here). */
typedef struct PlayerCopy {
    u8 b[0x970];
} __attribute__((aligned(16))) PlayerCopy;
void Player_SetWeapon(Player *p, EquipItem *e);
void Player_RemoveUnwearable(Player *p);
void Player_CalcSkills(Player *p);
void Player_DrawSkills(Player *, s16, s16, s16 *); /* unit's own declaration: with the definition's void (Player *p, s32 x, s16 y, s16 *colors) (sub_main/unk_356380.c), SkillList_DrawWithEquip 18 of 150 words, SkillList_DrawCompare 18 of 102 words differ */
u8 Player_GetSkill(Player *p, u8 group);
s8 Player_GetSkillPoints(Player *p, u8 kind);

/* The skills of pl with e put on (a copy in gSkillTryPlayer, the weapon's
 * unwearable parts removed), each coloured against pl's as in
 * SkillList_DrawCompare; without e, pl's own. */
void SkillList_DrawWithEquip(Player *pl, EquipItem *e, s16 x, s16 y)
{
    u32 i;
    s16 col[5];
    Player *p;
    s8 pts;
    s16 *c;
    s8 q;
    u8 lv;

    Font_SetSpacing(0x12, 0x12);
    Text_SetColor(0);
    col[4] = 0;
    col[3] = 0;
    col[2] = 0;
    col[1] = 0;
    col[0] = 0;
    if (e != NULL) {
        p = (Player *)gSkillTryPlayer;
        *(PlayerCopy *)gSkillTryPlayer = *(PlayerCopy *)pl;
        if (e->type != 6 && e->type != 7) {
            p->equip[e->type] = *e;
        } else {
            Player_SetWeapon(p, e);
            if (pl->equip[6].type != e->type) {
                Player_RemoveUnwearable(p);
            }
        }
        Player_CalcSkills(p);
        for (i = 0, c = col; i < 5; i++, c++) {
            if (p->skillGroups[i] == 0) {
                break;
            }
            pts = Player_GetSkillPoints(p, p->skillGroups[i]);
            lv = Player_GetSkill(pl, p->skillGroups[i]);
            if (lv == 0) {
                if (pts > 0) {
                    *c = 0xE;
                } else if (pts < 0) {
                    *c = 2;
                }
            } else if (lv != p->skills[i]) {
                q = Player_GetSkillPoints(pl, p->skillGroups[i]);
                if (q < pts) {
                    *c = 4;
                } else if (pts < q) {
                    *c = 0xF;
                }
            }
        }
        pl = p;
    }
    Player_DrawSkills(pl, x, y, col);
}

u8 Player_GetSkill(Player *p, u8 group);
s8 Player_GetSkillPoints(Player *p, u8 kind);
void Player_DrawSkills(Player *, s16, s16, s16 *); /* unit's own declaration: with the definition's void (Player *p, s32 x, s16 y, s16 *colors) (sub_main/unk_356380.c), SkillList_DrawWithEquip 18 of 150 words, SkillList_DrawCompare 18 of 102 words differ */

/* The skill list of the preview player gPreviewPlayer at (x, y), each skill
 * coloured against pl's: 0xE gained, 2 lost, 4 lower, 0xF higher. */
void SkillList_DrawCompare(Player *pl, s16 x, s16 y)
{
    Player *p = (Player *)&gPreviewPlayer;
    s16 col[5];
    u32 i;
    s8 pts;
    s16 *c;
    s8 q;
    u8 lv;

    Font_SetSpacing(0x12, 0x12);
    Text_SetColor(0);
    col[4] = 0;
    col[3] = 0;
    col[2] = 0;
    col[1] = 0;
    col[0] = 0;
    if (pl != NULL) {
        for (i = 0, c = col; i < 5; i++, c++) {
            if (p->skillGroups[i] == 0) {
                break;
            }
            pts = Player_GetSkillPoints(p, p->skillGroups[i]);
            lv = Player_GetSkill(pl, p->skillGroups[i]);
            if (lv == 0) {
                if (pts > 0) {
                    *c = 0xE;
                } else if (pts < 0) {
                    *c = 2;
                }
            } else if (lv != p->skills[i]) {
                q = Player_GetSkillPoints(pl, p->skillGroups[i]);
                if (q < pts) {
                    *c = 4;
                } else if (pts < q) {
                    *c = 0xF;
                }
            }
        }
    }
    Player_DrawSkills(p, x, y, col);
}

/* unk_7683D0's .data (0x7F1A08-0x7F1D20) */
extern char *D_0080CA20[];
extern char *D_0080CA48[];
extern char *D_0080CA50[];
s8 gBoxTypeKinds[8] = {
#include "lobby/007F1A08.inc"
};
Panel gBoxItemListWin = {
#include "lobby/007F1A10.inc"
};
u32 gBoxPageColors[8] = {
#include "lobby/007F1A30.inc"
};
BoxTab gBoxPageTabs[2] = {
#include "lobby/007F1A50.inc"
};
Panel gBoxPageTabsWin = {
#include "lobby/007F1A80.inc"
};
Panel gBoxEquipGridWin = {
#include "lobby/007F1AA0.inc"
};
ListPanel gEquipDetailPanel = {
#include "lobby/007F1AC0.inc"
};
ListPanel gEquipNonePanel = {
#include "lobby/007F1AE0.inc"
};
Panel gEquipDetailWin[4] = {
#include "lobby/007F1B00.inc"
};
s16 gEquipDetailIcons[8] = {
#include "lobby/007F1B60.inc"
};
ListPanel gBoxActionsPanel = {
#include "lobby/007F1B70.inc"
};
ListWin gBoxListPanel = {
#include "lobby/007F1B90.inc"
};
ListWin gBoxEquipPanel = {
#include "lobby/007F1BB0.inc"
};
ListPanel gBoxListHeaders[2] = {
#include "lobby/007F1BD0.inc"
};
Panel gEquipInfoWin = {
#include "lobby/007F1C10.inc"
};
ListPanel gBoxEquipRows = {
#include "lobby/007F1C30.inc"
};
s16 gBoxMenuIcon[8] = {
#include "lobby/007F1C50.inc"
};
s16 gBoxControlIcons[24] = {
#include "lobby/007F1C60.inc"
};
s16 gBoxControlIconsB[12] = {
#include "lobby/007F1C90.inc"
};
u8 gBoxEquipListU[8] = {
#include "lobby/007F1CA8.inc"
};
ListPanel gBoxEquipPickPanel = {
#include "lobby/007F1CB0.inc"
};
s32 gBoxSortTypeKeys[8] = {
#include "lobby/007F1CD0.inc"
};
s32 gBoxSortClassKeys[12] = {
#include "lobby/007F1CF0.inc"
};
