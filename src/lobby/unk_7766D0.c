#include "common.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "item.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "em.h"
#include "player.h"
#include "lobby_menu.h"
#include "combine.h"
#include "area.h"

/* prototypes */
s32 WinSlide_Step(WinSlide *s, s16 from, s16 to);
/* end of prototypes */

typedef struct CombLine {
    /* 0x0 */ s32 id;             /* Recipe_YieldBase turns it into a per-line value */
    /* 0x4 */ s16 a;
    /* 0x6 */ s16 b;
    /* 0x8 */ s16 kind;
    /* 0xA */ s16 unkA;
} CombLine;

/* The combination shop's menu (in the shared work area gMenuWork): per
 * page, its item, the lines' extra values and their count, the lines. */
typedef struct CombMenu {
    /* 0x000 */ ItemListWin list;   /* the buy and sell list (Shop_StartBuy, Shop_StartSell) */
    /* 0x040 */ WinSlide win2;      /* the combination window (CombShop_Steps) */
    /* 0x048 */ s16 menu[4];        /* the top menu's entries (1 list, 2 box, 4 talk, 5 combine) */
    /* 0x050 */ s16 cursor;         /* the top menu's cursor */
    /* 0x052 */ s16 menuCount;      /* the top menu's entry count */
    /* 0x054 */ u16 item[8];
    /* 0x064 */ s32 per[7][3];
    /* 0x0B8 */ u8 count[8];
    /* 0x0C0 */ CombLine lines[7][3];
    /* 0x1BC */ s16 listPage;     /* the page of 7 combinable items shown */
    /* 0x1BE */ s16 listPages;
    /* 0x1C0 */ s16 itemCount;    /* the items on this page (item[], per[], count[], lines[]) */
    /* 0x1C2 */ s16 page;
    /* 0x1C4 */ s16 unk1C4;
    /* 0x1C6 */ s16 amount;
    /* 0x1C8 */ s16 line;
    /* 0x1CA */ s16 lineCount;
    /* 0x1CC */ s8 details;
    /* 0x1CD */ u8 npc;             /* the shopkeeper (Npc_Find) */
    /* 0x1CE */ s8 state;           /* 0 top menu, 1 combine, 2 list, 3 box, 4 talk */
    /* 0x1CF */ s8 step;            /* CombShop_Steps (0: the window slides in) */
} CombMenu;

void CombShop_BuildList(CombMenu *m, s16 page);

extern Player gPlayers[];
s16 Combine_Do(u8 *count, struct Player *player, s16 a, s16 b, s32 mode);
s16 Combine_Do3(u8 *count, struct Player *player, s32 a, s32 b, s32 c, s32 mode);
s32 CombShop_Price(u16 item);
void Save_AddMoney(s32 n);
void LobbySe_Play(s32 se);
s8 Items_Give(s32 item, s32 n);
s32 Menu_PushNoticeOnce(s32);

typedef struct V00777940_0 {
    u8 pad0[0x50];
    s16 f50;
    s16 f52;
} V00777940_0;

s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Item_IsOnePerPouch(u16 item);
s32 Recipe_IsListedFor(s16 item, s32 arg1);
void Text_SetSize(s32 size);
void CombShop_DrawWindow(CombMenu *m, s16 x, s16 y);
void Shop_DrawBuy(void *);
void Shop_DrawSell(void *);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
extern char *gItemShopMenuTexts[];
extern u8 gMenuWork[];
s32 Area_IsOnlineHouse(s32 arg0);
void KitchenGuest_Open(void);
void KitchenGuest_OnRoute(s32 arg0);
void KitchenGuest_OnDone(void);
void KitchenGuest_OnChosen(u8 *arg0);
s32 Recipe_IsKnown(Recipe *r);
s32 Items_HaveEnough(u16 item, s32 n);
s32 Items_Room(s32 item);

void CombShop_OpenList(CombMenu *m);
s32 CombShop_MenuInput(V00777940_0 *arg0, u16 arg1);
u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8 a, s8 b);
s32 CombShop_Steps(CombMenu *m, u16 pad);
void Chat_SetColor(s16 color);
void CamFocus_SetAside(s8 a);
void Shop_StartBuy(ItemListWin *list, u16 kind);
void Shop_StartSell(ItemListWin *list);
Em *Npc_Find(u8 kind);
struct OfferTask;
void OfferMenu_Open(struct OfferTask *t);
void Talk_Say(s32 a, s8 b);
s32 Shop_BuyStep(u16 pad, ItemListWin *list);
s32 Shop_SellStep(u16 pad, ItemListWin *list);
s32 Talk_Update(void);
void TownMenu_CloseQuiet(void);

/* The combination shop's task: the top menu, then the item list, the
 * combination pages, the storage box and the talk. */
void CombShop_Update(void)
{
    CombMenu *m = (CombMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (m->state) {
    case 0:
        switch (CombShop_MenuInput((V00777940_0 *)m, pad)) {
        case 0:
            switch (m->menu[m->cursor]) {
            case 5:
                m->state = 1;
                CombShop_OpenList(m);
                Chat_SetColor(0);
                CamFocus_SetAside(1);
                LobbySe_Play(0x17);
                break;
            case 1:
                m->state = 2;
                Shop_StartBuy(&m->list, m->npc);
                CamFocus_SetAside(1);
                LobbySe_Play(0x17);
                break;
            case 2:
                m->state = 3;
                Shop_StartSell(&m->list);
                CamFocus_SetAside(1);
                LobbySe_Play(8);
                break;
            case 4:
                OfferMenu_Open((struct OfferTask *)(Npc_Find(m->npc)));
                LobbySe_Play(8);
                break;
            }
            break;
        case 3:
            m->state = 4;
            Talk_Say(1, 0);
            break;
        }
        break;
    case 1:
        switch (CombShop_Steps(m, pad)) {
        case 0:
        case 3:
            m->state = 0;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case 2:
        switch (Shop_BuyStep(pad, &m->list)) {
        case 0:
        case 3:
            m->state = 0;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case 3:
        switch (Shop_SellStep(pad, &m->list)) {
        case 0:
        case 3:
            m->state = 0;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case 4:
        switch (Talk_Update()) {
        case 0:
        case 3:
            TownMenu_CloseQuiet();
            break;
        }
        break;
    }
}

void CombShop_OpenList(CombMenu *m)
{
    CombShop_BuildList(m, 0);
    WinSlide_Step(&m->win2, 0x280, 0x10E);
    m->amount = 1;
    m->details = 0;
}

s32 CombShop_InputPage(CombMenu *m, u16 pad);
s32 CombShop_InputLine(CombMenu *m, u16 pad);
s32 CombShop_InputAmount(CombMenu *m, u16 pad);
void Menu_SetYesNo(s8 a);
extern u8 gMenuInputLock;
s32 Item_IsDeco(u16 id);
s32 Menu_YesNoInput(void);
void CombShop_Combine(CombMenu *m);

/* The combination steps (+0x1CF): open the window, pick a page, pick a
 * line, the amount, then confirm (with the details toggle). Returns 3 when
 * the player leaves the pages, 2 otherwise. */
s32 CombShop_Steps(CombMenu *m, u16 pad)
{
    switch (m->step) {
    case 0:
        switch (WinSlide_Step(&m->win2, 0x280, 0x10E)) {
        case 0:
            (m->step)++;
            break;
        }
        break;
    case 1:
        switch (CombShop_InputPage(m, pad)) {
        case 0:
            (m->step)++;
            m->amount = 1;
            break;
        case 3:
            m->step = 0;
            return 3;
        }
        break;
    case 2:
        switch (CombShop_InputLine(m, pad)) {
        case 0:
            (m->step)++;
            m->amount = 1;
            break;
        case 3:
            m->step = 1;
            break;
        }
        break;
    case 3:
        switch (CombShop_InputAmount(m, pad)) {
        case 0:
            (m->step)++;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            break;
        case 3:
            m->step = 2;
            m->amount = 1;
            break;
        }
        break;
    case 4:
        if (pad & 0x200) {
            if (m->itemCount != 0) {
                m->details ^= 1;
                LobbySe_Play(0x39);
            }
        } else if (pad & 0x80) {
            if (m->details != 0 && Item_IsDeco(m->item[m->page]) != 0) {
                if (++m->unk1C4 >= 3) {
                    m->unk1C4 = 0;
                }
                LobbySe_Play(8);
            }
        } else if (pad & 8) {
            if (m->details != 0 && Item_IsDeco(m->item[m->page]) != 0) {
                if (--m->unk1C4 < 0) {
                    m->unk1C4 = 2;
                }
                LobbySe_Play(8);
            }
        } else {
            switch (Menu_YesNoInput()) {
            case 0:
                m->step = 1;
                CombShop_Combine(m);
                CombShop_BuildList(m, m->listPage);
                break;
            case 3:
                m->step = 2;
                m->amount = 1;
                break;
            }
        }
        break;
    }
    return 2;
}

void CombShop_Combine(CombMenu *m)
{
    CombLine *l;
    void *pl;
    u16 item;
    s32 total = 0;
    s32 i = 0;
    u8 got;

    l = &m->lines[m->page][m->line];
    pl = &gPlayers[gScene.localPlayer];
    for (; i < m->amount; i++) {
        if (l->kind == 0) {
            item = Combine_Do(&got, (struct Player *)pl, l->a, l->b, 1);
        } else {
            item = Combine_Do3(&got, (struct Player *)pl, l->a, l->b, l->kind, 1);
        }
        total += got;
    }
    Save_AddMoney((-(CombShop_Price(item) * m->amount)));
    LobbySe_Play(10);
    if (Items_Give(item, total) != 0) {
        Menu_PushNoticeOnce(0xF);
    }
}

s32 CombShop_CanCombine(CombLine *l, u16 item, s32 n, s32 per);
s32 CombShop_InputAmount(CombMenu *m, u16 pad)
{
    CombLine *l = &m->lines[m->page][m->line];
    s32 n;
    s8 ok;

    if (pad & 0x20) {
        LobbySe_Play(8);
        return 0;
    }
    if (pad & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    if (pad & 0x200) {
        if (m->itemCount != 0) {
            m->details ^= 1;
            LobbySe_Play(0x39);
        }
    } else if (pad & 0x80) {
        if (m->details != 0 && Item_IsDeco(m->item[m->page]) != 0) {
            if (++m->unk1C4 >= 3) {
                m->unk1C4 = 0;
            }
            LobbySe_Play(8);
        }
    } else if (pad & 8) {
        if (m->details != 0 && Item_IsDeco(m->item[m->page]) != 0) {
            if (--m->unk1C4 < 0) {
                m->unk1C4 = 2;
            }
            LobbySe_Play(8);
        }
    } else if (pad & 0x2000) {
        if (CombShop_CanCombine(l, m->item[m->page], m->amount + 1,
                                m->per[m->page][m->line]) != 0) {
            (m->amount)++;
            LobbySe_Play(1);
        }
    } else if (pad & 0x1000) {
        if (m->amount > 1) {
            (m->amount)--;
            LobbySe_Play(1);
        }
    } else if (pad & 0x800) {
        if (m->amount > 1) {
            m->amount = 1;
            LobbySe_Play(1);
        }
    } else if (pad & 0x400) {
        ok = 0;
        for (n = m->amount + 1; n <= 1000; n++) {
            if (CombShop_CanCombine(l, m->item[m->page], n,
                                    m->per[m->page][m->line]) == 0) {
                break;
            }
            ok = 1;
        }
        if (ok != 0) {
            m->amount = n - 1;
            LobbySe_Play(1);
        }
    }
    return 2;
}

s32 CombShop_CanCombine(CombLine *l, u16 item, s32 n, s32 per);
s32 Item_IsDeco(u16 id);

s32 CombShop_InputLine(CombMenu *m, u16 pad)
{
    s16 sel;
    s16 page;

    if (pad & 0x20) {
        sel = m->line;
        page = m->page;
        if (CombShop_CanCombine(&m->lines[page][sel], m->item[page], 1,
                                m->per[page][sel]) == 1) {
            m->amount = 1;
            LobbySe_Play(8);
            return 0;
        }
        LobbySe_Play(9);
    } else if (pad & 0x40) {
        *(u8 *)&m->details = 0;
        LobbySe_Play(4);
        return 3;
    } else if (pad & 0x200) {
        if (m->itemCount != 0) {
            m->details ^= 1;
            LobbySe_Play(0x39);
        }
    } else if (pad & 0x80) {
        if (m->details != 0 && Item_IsDeco(m->item[m->page]) != 0) {
            if (++m->unk1C4 >= 3) {
                m->unk1C4 = 0;
            }
            LobbySe_Play(8);
        }
    } else if (pad & 8) {
        if (m->details != 0 && Item_IsDeco(m->item[m->page]) != 0) {
            if (--m->unk1C4 < 0) {
                m->unk1C4 = 2;
            }
            LobbySe_Play(8);
        }
    } else {
        m->line = Menu_WrapUpDown(m->line, m->lineCount);
    }
    return 2;
}

s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);
s32 CombShop_PageReady(CombMenu *m, s16 page);

s32 CombShop_InputPage(CombMenu *m, u16 pad)
{
    if (pad & 0x20) {
        if (CombShop_PageReady(m, m->page) == 1) {
            m->amount = 1;
            m->line = 0;
            m->lineCount = m->count[m->page];
            LobbySe_Play(8);
            return 0;
        }
        if (m->itemCount != 0) {
            LobbySe_Play(9);
        }
    } else if (pad & 0x40) {
        if (m->details == 0) {
            LobbySe_Play(4);
            return 3;
        }
        *(u8 *)&m->details = 0;
        LobbySe_Play(0x39);
    } else if (pad & 0x200) {
        if (m->itemCount != 0) {
            m->details ^= 1;
            LobbySe_Play(0x39);
        }
    } else if (pad & 0xC00) {
        if (m->listPages > 1) {
            m->listPage = Menu_WrapLeftRight(m->listPage, m->listPages);
            CombShop_BuildList(m, m->listPage);
        }
    } else {
        m->page = Menu_WrapUpDown(m->page, m->itemCount);
    }
    return 2;
}

s32 Item_IsCombinable(u16 item)
{
    if (Item_IsOnePerPouch(item) == 1) {
        return 0;
    } else if (Recipe_IsListedFor(item, 0) != 0) {
        return 1;
    }
    return 0;
}

s32 CombShop_CanCombine(CombLine *l, u16 item, s32 n, s32 per)
{
    s32 i;

    if (Recipe_IsKnown(*(void **)l) == 0) {
        return 0;
    }
    if (n * CombShop_Price(item) > gSave.money) {
        return 0;
    }
    for (i = 0; i < 3; i++) {
        if (((s16 *)l)[2] != 0 && Items_HaveEnough(((s16 *)l)[2], n) == 0) {
            return 0;
        }
        l = (CombLine *)((s16 *)l + 1);
    }
    if (Items_Room(item) < n * per) {
        return 0;
    }
    if (n < 100) {
        return 1;
    }
    return 0;
}

#pragma optimization_level 4
/* 1 when the money covers the page's price and one of its lines can be
 * combined now. */
s32 CombShop_PageReady(CombMenu *m, s16 page)
{
    s32 i;

    if (CombShop_Price(m->item[page]) > gSave.money) {
        return 0;
    }
    for (i = 0; i < m->count[page]; i++) {
        if (CombShop_CanCombine(&m->lines[page][i], m->item[page], 1, m->per[page][i]) == 1) {
            return 1;
        }
    }
    return 0;
}
#pragma optimization_level reset

void *memset(void *d, s32 c, u32 n);
s32 Item_IsCombinable(u16 item);
struct RecipeHit;
u8 Combine_ListRecipesFor(struct RecipeHit *lines, s16 item, s32 flags);
struct Recipe;
u8 Recipe_YieldBase(struct Recipe *r);
s32 Menu_CountPages(s16 n, s16 per);

/* Fills the page `page` of the combination list: the items that can be
 * combined, 7 per page, with their lines and per-line values. */
void CombShop_BuildList(CombMenu *m, s16 page)
{
    s16 lo;
    s16 hi;
    s16 k;
    u16 item;
    u16 j;

    memset(m->item, 0, sizeof(m->item) - 2);
    memset(m->per, 0, sizeof(m->per));
    memset(m->count, 0, sizeof(m->count) - 1);
    memset(m->lines, 0, sizeof(m->lines));
    m->itemCount = 0;
    k = 0;
    lo = page * 7;
    hi = lo + 6;
    for (item = 0; item < 0x399; item++) {
        if (Item_IsCombinable(item) == 1) {
            if (lo <= k && k <= hi) {
                m->item[m->itemCount] = item;
                m->count[m->itemCount] = Combine_ListRecipesFor((struct RecipeHit *)m->lines[m->itemCount], item, 1);
                for (j = 0; j < m->count[m->itemCount]; j++) {
                    m->per[m->itemCount][j] = Recipe_YieldBase((struct Recipe *)m->lines[m->itemCount][j].id);
                }
                m->itemCount++;
            }
            k++;
        }
    }
    m->listPage = page;
    m->listPages = Menu_CountPages(k, 7);
    if (m->page >= m->itemCount) {
        m->page = m->itemCount - 1;
    }
    if (m->listPage >= m->listPages) {
        m->listPage = m->listPages - 1;
    }
}

s32 CombShop_MenuInput(V00777940_0 *m, u16 pad)
{
    s32 btn;

    btn = pad & 0xFFFF;
    if (btn & 0x20) {
        return 0;
    }
    if (btn & 0x40) {
        LobbySe_Play(4);
        return 3;
    }
    m->f50 = Menu_WrapUpDown((s32) m->f50, (s32) m->f52);
    return 2;
}

extern u8 D_45F11C;

/* The combination shop's price of an item: 14% of its price (42% for the
 * item types above 2), halved when the lobby state's +0x3924 is 4; at least 1. */
s32 CombShop_Price(u16 item)
{
    f32 rate;
    s32 n;

    switch (gItemData[item].rarity) {
    case 0:
    case 1:
    case 2:
        rate = 0.14f;
        break;
    default:
        rate = 0.42f;
        break;
    }
    if (gLobby->townEvent == 4) {
        rate *= 0.5f;
    }
    n = rate * (f32)(u32)gItemData[item].price;
    if (n == 0) {
        n = 1;
    }
    return n;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7766D0", D_0083B538);
INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7766D0", D_0083B540);

void Text_PrintfColored(const char *fmt, ...);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Font_SetSpacing(u8 a, u8 b);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void ItemInfo_Draw(s16 x, s16 y, s32 w, s32 a, s32 item, s32 b, s32 c, s32 color); /* unit's own declaration: with the definition's void (s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color) (lobby/unk_7A1120.c), CombShop_DrawLineWindow 163 of 203 words differ */
extern char *gItemNames[];
extern char D_0083B538[];
extern char D_0083B540[];
extern char *gNoCombineTexts[];

void CombShop_DrawList(CombMenu *m, s16 x, s16 y)
{
    s32 i;
    s32 price;
    s32 item;
    char *s;
    s16 ly = y;

    Icon_Draw(x, y, 0x15E, 0x132, -0x23, 0);
    Icon_Draw(x + 0x64, y + 0x4E, 0x96, 0x96, 0x60FFFFFF, 0x22);
    if (m->itemCount == 0) {
        Font_SetSpacing(0x14, 0x14);
        s = gNoCombineTexts[0];
        Font_SetColor(0x8000052C);
        Font_SetPos(Text_CenterX(s, x + 0xAF, 0x14), y + 0x78);
        Text_PrintfColored(D_0083B538, s);
        return;
    }
    ly += 10;
    Font_SetColor(0x80000000);
    Menu_DrawPageNum(x + 0xFA, ly, m->listPage, m->listPages, 1);
    Font_SetSpacing(0x14, 0x14);
    for (i = 0; i < m->itemCount; i++) {
        item = m->item[i];
        price = CombShop_Price(item);
        if (CombShop_PageReady(m, i) == 1) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Font_SetSpacing(0x14, 0x14);
        Font_SetPos(x + 0x2E, ly += 0x18);
        Text_PrintfColored(D_0083B538, gItemNames[(u16)item]);
        Font_SetSpacing(0x14, 0x14);
        Font_SetPos(x + 0xF8, ly);
        Text_PrintfColored(D_0083B540, price);
        Item_DrawIcon(x + 0x10, ly - 1, 0x18, (u16)item);
        if (m->page == i) {
            Menu_DrawCursorRow(x + 0x2E, ly, 0xFA, 0x32, 0xC0106060, 0xFF106060);
        }
    }
    item = m->item[m->page];
    Menu_DrawDashLine(x + 0x14, y + 0xCA, 0x136, 0xFF000000);
    if (item == 0 || item >= 0x399) {} /* compiled-out check */
    ItemInfo_Draw(x, y + 0xCE, 0x15E, 0, (u16)item, 0, -1, 0x80101010);
}

void Text_PrintfColored(const char *fmt, ...);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
void Font_SetSpacing(u8 a, u8 b);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
extern char *gItemNames[];
extern char D_0083B538[];
extern char D_0083B540[];

/* The combination list page: each recipe (item, price) coloured by
 * CombShop_PageReady, the page counter and the cursor frame. */
void CombShop_DrawPrices(CombMenu *m, s16 x0, s16 y0)
{
    s32 i;
    s32 price;
    s32 item;
    s16 y;

    Icon_Draw(0, 0x32, 0x122, 0xB4, -0x23, 0);
    Icon_Draw(0x69, 0x5A, 0x50, 0x64, 0x60FFFFFF, 0x22);
    y = 0x3C;
    Font_SetColor(0x80000000);
    Font_SetSpacing(0x11, 0x11);
    Menu_DrawPageNum(0xAA, y, m->listPage, m->listPages, 1);
    for (i = 0; i < m->itemCount; i++) {
        item = m->item[i];
        price = CombShop_Price(item);
        if (CombShop_PageReady(m, i) == 1) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Font_SetPos(0x38, y += 0x14);
        Text_PrintfColored(D_0083B538, gItemNames[(u16)item]);
        Font_SetPos(0xCA, y);
        Text_PrintfColored(D_0083B540, price);
        Item_DrawIcon(0x1A, y - 1, 0x14, (u16)item);
        if (m->page == i) {
            Menu_DrawCursorRow(0x38, y, 0xB4, 0x32, 0xC0106060, 0xFF106060);
        }
    }
}

void Text_PrintfColored(const char *fmt, ...);
void Font_SetPos(s16 x, s16 y);
void Font_SetColor(s32 color);
s32 Items_DrawNeedLine(s16 x, s16 y, u16 item, s32 n, u32 color);
s32 Items_DrawNeedLineAfter(s16 x, s16 y, u16 item, s32 n, u32 color);
extern char *gRecipeTexts[];
extern char *gUnknownLabelTexts[];
extern char D_0083B538[];

/* One line of the combination list: its label, then its three materials
 * (or the "not yet" text when Recipe_IsKnown says the recipe is unknown). */
void CombShop_DrawLine(CombMenu *m, CombLine *l, u8 i, s16 n, s16 x, s16 y)
{
    s32 k;
    char *s;

    Font_SetColor(0x807F7F28);
    Font_SetPos(x, y);
    Text_PrintfColored(gRecipeTexts[i]);
    Font_SetColor(0x807F7F7F);
    if (Recipe_IsKnown(*(void **)l) == 1) {
        for (k = 0; k < 3; k++) {
            if ((&l->a)[k] != 0) {
                if (m->step == 3 || m->step == 4) {
                    y += 0x14;
                    Items_DrawNeedLineAfter(x + 0x28, y, (&l->a)[k], m->amount, 0x807F7F7F);
                } else {
                    y += 0x14;
                    Items_DrawNeedLine(x + 0x28, y, (&l->a)[k], n, 0x807F7F7F);
                }
            }
        }
    } else {
        s = gUnknownLabelTexts[0];
        Font_SetColor(0x807F212E);
        Font_SetPos(Text_CenterX(s, x + 0xA6, 0x12), y + 0x28);
        Text_PrintfColored(D_0083B538, s);
    }
}

void CombShop_DrawLine(CombMenu *m, CombLine *l, u8 i, s16 n, s16 x, s16 y);
void Win_Draw(s16, s16, s32, s32, s32, s32, s32); /* unit's own declaration: with the definition's void (s32 x, s32 y, s32 w, s32 h, s32 c1, s32 c2, s16 kind) (lobby/unk_790EC0.c), CombShop_DrawLines 62 of 68 words, CombShop_DrawLineWindow 189 of 205 words, CombShop_DrawAmount 65 of 213 words differ */
void Font_SetSpacing(u8 a, u8 b);

void CombShop_DrawLines(CombMenu *m, s16 x, s16 y)
{
    s16 i;
    s32 off;

    Win_Draw(x, y, 0x14C, 0x126, 0xBDB59F6C, 0xA0202020, 4);
    Font_SetSpacing(0x12, 0x12);
    i = 0;
    y += 10;
    if (i < m->count[m->page]) {
        off = 0;
        do {
            CombShop_DrawLine(m, (CombLine *)((u8 *)m + m->page * 0x24 + off + 0xC0), i, 1, x, y);
            y += 90;
            off += 0xC;
            i++;
        } while (i < m->count[m->page]);
    }
}

void Menu_DrawLine(s16 x, s16 y0, s16 w, s16 y1, s8 flag);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);

void CombShop_DrawLineWindow(CombMenu *m, s16 x, s16 y)
{
    s16 ly;
    s16 i;
    s32 wy;
    u16 item = m->item[m->page];

    Win_Draw(x, y, 0x14C, 0x126, 0xBDB59F6C, 0xA0202020, 4);
    Font_SetSpacing(0x12, 0x12);
    if (Item_IsDeco(item) != 0) {
        ItemInfo_Draw(x, y, 0x158, (u8)m->details, item, 0, (s8)m->unk1C4, 0x807F7F7F);
    } else {
        ItemInfo_Draw(x, y, 0x158, (u8)m->details, item, 0, -1, 0x807F7F7F);
    }
    Font_SetSpacing(0x12, 0x12);
    wy = y;
    Gfx_DrawLine(x + 0x14, wy + 0x5E, 0x136, 2, 0xBDB59F6C);
    ly = wy + 0x66;
    for (i = 0; i < m->lineCount; i++) {
        if (CombShop_CanCombine(&m->lines[m->page][i], m->item[m->page], 1, m->per[m->page][i]) == 1) {
            Font_SetColor(0x807F7F7F);
        } else {
            Font_SetColor(0x80404040);
        }
        Font_SetPos(x + 0x14, ly);
        Text_PrintfColored(gRecipeTexts[i + 3]);
        if (i == m->line) {
            Menu_DrawLine(x + 0xA, ly - 1, 0x138, 0x18, m->step != 2);
        }
        ly += 0x14;
    }
    Gfx_DrawLine(x + 0x14, wy + 0xC0, 0x136, 2, 0xBDB59F6C);
    CombShop_DrawLine(m, &m->lines[m->page][m->line], m->line, m->amount, x, wy + 0xC4);
}

void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
u8 Recipe_YieldAmount(struct Recipe *r);
extern char *D_0080D14C;
extern char *gCombineFeeTexts[];
extern char *D_0080C074;
extern char *D_0080C06C;

/* The amount window: the item, its line and mode icon, then the yield, the
 * per-line total and the fee for the chosen amount.
 * FAKEMATCH: id = item is written in each branch (not once before the if):
 * the game converts item to s32 in each branch and keeps it in s2 for
 * Item_DrawIcon. Without the declared id, MWCC's own copy of the conversion
 * is numbered after tx and takes another saved register (17 words); id
 * assigned once before the if computes it before the branch (175). */
void CombShop_DrawAmount(CombMenu *m, s16 x, s16 y)
{
    u16 item = m->item[m->page];
    s16 ly;
    s32 wy;
    s32 wx;
    s32 id;
    s32 tx;
    s32 n;
    s32 price;
    s32 tot;

    Win_Draw(x, y, 0x14C, 0x126, 0xBDB59F6C, 0xA0202020, 4);
    Font_SetSpacing(0x12, 0x12);
    if (Item_IsDeco(item) != 0) {
        id = item;
        ItemInfo_Draw(x, y, 0x158, (u8)m->details, id, 0, (s8)m->unk1C4, 0x807F7F7F);
    } else {
        id = item;
        ItemInfo_Draw(x, y, 0x158, (u8)m->details, id, 0, -1, 0x807F7F7F);
    }
    Font_SetSpacing(0x12, 0x12);
    wy = y;
    wx = x;
    Gfx_DrawLine(wx + 0x14, wy + 0x5E, 0x136, 2, 0xBDB59F6C);
    CombShop_DrawLine(m, &m->lines[m->page][m->line], m->line, m->amount, x, wy + 0x66);
    Icon_DrawMode(wx + 0xA3, wy + 0xC0, 0x24, 0x16, 0xFF00FF00, 0x28, 2);
    ly = wy + 0xD2;
    Font_SetSpacing(0x12, 0x12);
    tx = wx + 0x28;
    Font_SetPos(tx, wy + 0xD2);
    Text_PrintfColored(D_0083B538, gItemNames[item]);
    Item_DrawIcon(wx + 0xC, (s16)(wy + 0xD2) - 1, 0x14, id);
    Font_SetPos(tx, ly += 0x18);
    n = (u8)Recipe_YieldAmount((struct Recipe *)(*(s32 *)((u8 *)m + m->page * 0x24 + m->line * 0xC + 0xC0)));
    n *= m->amount;
    tot = m->per[m->page][m->line];
    tot *= m->amount;
    Text_PrintfColored("%-17s%s%3d%s%3d", D_0080D14C, D_0080C074, n, D_0080C06C,
                       tot, m->amount);
    price = m->amount * CombShop_Price(item);
    Font_SetPos(tx, ly + 0x18);
    Text_PrintfColored("%-17s%s%7dz", gCombineFeeTexts[0], D_0080C074, price);
}

void Menu_DrawLabelBar(s16 x, s16 y, s16 w, u32 color, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
s32 sprintf(char *buf, char *fmt, ...);
void Sjis_Widen(char *src, char *dst);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
char *Text_FillDollar(char *fmt, M2C_UNK n);
void Text_Printf(const char *fmt, ...);
void CombShop_DrawList(CombMenu *m, s16 x, s16 y);
void CombShop_DrawLineWindow(CombMenu *m, s16 x, s16 y);
void CombShop_DrawAmount(CombMenu *m, s16 x, s16 y);
extern char *gCombineCountTexts[];
extern char *gPriceConfirmTexts;
extern char *gCombinePromptTexts[];
extern char *gRecipePromptTexts[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;

/* The combination window at (x, y) by step: the list (or its details),
 * the lines, the amount; then the help line, the amount with its arrows,
 * or the total price. */
void CombShop_DrawWindow(CombMenu *m, s16 x, s16 y)
{
    char buf[0x20];
    char wide[0x20];

    Font_SetSpacing(0x16, 0x16);
    Menu_DrawLabelBar(0x10E, 0x12, 0xA0, 0xFFB0B000, gItemShopMenuTexts[0], 2, 1);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    switch (m->step) {
    case 0:
    case 1:
        if (m->details == 0) {
            CombShop_DrawList(m, x, y);
        } else {
            CombShop_DrawPrices(m, x, y);
            CombShop_DrawLines(m, 0x120, y + 4);
        }
        break;
    case 2:
        CombShop_DrawLineWindow(m, 0x120, y + 4);
        break;
    case 3:
    case 4:
        CombShop_DrawAmount(m, 0x120, y + 4);
        break;
    }
    Font_SetSpacing(0x12, 0x12);
    switch (m->step) {
    case 0:
    case 1:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gCombinePromptTexts[0]);
        break;
    case 2:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gRecipePromptTexts[0]);
        break;
    case 3:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010, gCombineCountTexts[0]);
        Font_SetPos(0x1FB, 0x184);
        sprintf(buf, "%02d", m->amount);
        Sjis_Widen(buf, wide);
        Text_PrintfColored(wide);
        if (m->amount <= 98 &&
            CombShop_CanCombine(&m->lines[m->page][m->line], m->item[m->page], m->amount + 1,
                                m->per[m->page][m->line])) {
            Icon_DrawMode(0x1FC, 0x16E, 0x24, 0x16, 0xFF00FF00, 0x29, 2);
        }
        if (m->amount > 1) {
            Icon_DrawMode(0x1FD, 0x196, 0x24, 0x16, 0xFF00FF00, 0x28, 2);
        }
        break;
    case 4:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, 0xBDB59F6C, 0xC0101010,
                      Text_FillDollar(gPriceConfirmTexts, m->amount * CombShop_Price(m->item[m->page])));
        Font_SetPos(0x1A2, 0x192);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}


void CombShop_Draw(u8 *task)
{
    CombMenu *m = (CombMenu *)gMenuWork;

    Text_SetSize(*(s32 *)(task + 0x18));
    switch (m->state) {
    case 0:
        Menu_DrawLabelList(0x1CC, gItemShopMenuTexts, m->menuCount, m->cursor, 0xFFB0B000);
        break;
    case 2:
        Shop_DrawBuy(m);
        break;
    case 3:
        Shop_DrawSell(m);
        break;
    case 1:
        CombShop_DrawWindow(m, m->win2.pos, 0x31);
        break;
    }
}

s32 Lobby_GetPlace3928(void);
u8 Lobby_FindMember(u8 *id);
void TownMsg_OnMove(u8 pl, void *data, u8 cmd);
void TownMsg_OnActor(u8 pl, void *data);
void TownMsg_OnArea(u8 pl, u8 *msg);
void TownMsg_On39(u8, void *);
void TownMsg_On31(s8 idx, u8 *data);
void TownMsg_SendRecords(void *to, u8 kind);
void Records_RecvPage(void *id, s32 idx, void *data);
void Records_RecvMsg3A(void *id, void *data);
void Records_RecvMsg2C(void *id, void *data);
void Records_RecvMsg2D(void *id, void *data);
void Records_RecvMsg2E(void *id, u8 *data, s8 kind);
void TownMsg_On24(void *arg0, void *arg1);
void TownMsg_On25(s8 *arg0);
s32 LobbyBgm_IsActive(void);
void LobbyBgm_Play(s32 id, s32 pos, s32 arg2);
void ArenaMenu_Open(void);
void LobbyText_Open(s32);
void TownMsg_On28(s32 data);
void TownMsg_On1B(void *id, u8 *msg);
void TownMsg_On1C(s8 idx, u8 *msg);
void TownMsg_On1E(u8 *arg0, u8 *arg1);
void Contest_Open(void *p);
void Contest_RecvStart(void *arg0);
void Contest_RecvPower(void *src);
void Contest_RecvResult(s8 idx, void *data);
void TownMsg_On04(void *arg0, u8 *arg1);
void TownMsg_On15(char *id, u8 *msg);
void TownMsg_On05(s8, u8 *);
void TownMsg_On16(s8, u8 *);
void ActSpot_Release(s32 slot);
void Lobby_ClearReadyBit(s8 *msg);
s32 LobbyMember_InMyParty(s32 idx);
void NetQuest_SetInfo(u8 *);
void NetQuest_SetUnk4C(s8);
s32 Season_Get(void);
s32 LobbyTime_IsPm(void);
s32 MyHouse_IsOwner(void);
void Actor_SetIdle(void *obj, s32 a1, s32 n, s32 m);
void TownMsg_On0F(u8 *a0);
void Lobby_SetReadyBit(u8 *msg);
void Lobby_SetMemberSlot(s8 idx, u8 *msg);
void TownMsg_On06(s8 idx);
void TownMsg_On0A(s8 idx, void *src);
void TownMsg_On0B(void *to, void *src);
void Delivery_Take(void *src);
void TownMsg_On0D(u8 *arg0);
void Lobby_ShowMemberName(s8 idx, void *data);
void *memcpy(void *dst, const void *src, u32 n);
extern s32 gTownMode;
extern u8 gTownSeats[];
extern u8 D_6ADAD4[];
extern u8 D_6ADB0C[];
extern u8 D_6ADB16[];

/* The game's empty delay slots come from default-first switches (gPlayerText.card.arenaRole's tests and
 * cmd's 9 test). */
/* A town message from a member: the command byte, then its data. */
void TownMsg_Recv(void *from, u8 *pkt)
{
    u8 *data;
    u8 cmd;
    u8 idx;
    u16 a;
    s32 bgm[2];
    s32 pos;
    u8 buf[0x1A];
    u16 tmp;
    Player *pl;
    u8 me;

    cmd = pkt[0];
    data = pkt + 1;
    if (Lobby_GetPlace3928() != 0) {
        idx = Lobby_FindMember(from);
        if (idx == 0xFF) {
            return;
        }
    } else {
        switch (cmd) {
        default:
            return;
        case 9:
            break;
        }
    }
    switch (cmd) {
    case 0:
        TownMsg_OnMove(idx, data, cmd);
        break;
    case 16:
        memcpy(&a, data, 2);
        gPlayers[idx].rot[1] = a;
        gPlayers[idx].targetYaw = a;
        gPlayers[idx].netActive = 1;
        break;
    case 57:
        TownMsg_On39(idx, data);
        break;
    case 49:
        TownMsg_On31(idx, data);
        break;
    case 43:
        TownMsg_SendRecords(from, data[0]);
        break;
    case 50:
        Records_RecvPage(from, 0, data);
        break;
    case 51:
        Records_RecvPage(from, 2, data);
        break;
    case 52:
        Records_RecvPage(from, 3, data);
        break;
    case 53:
        Records_RecvPage(from, 4, data);
        break;
    case 54:
        Records_RecvPage(from, 5, data);
        break;
    case 55:
        Records_RecvPage(from, 6, data);
        break;
    case 56:
        Records_RecvPage(from, 7, data);
        break;
    case 58:
    case 59:
        Records_RecvMsg3A(from, data);
        break;
    case 44:
        Records_RecvMsg2C(from, data);
        break;
    case 45:
        Records_RecvMsg2D(from, data);
        break;
    case 46:
        Records_RecvMsg2E(from, data, 0);
        break;
    case 47:
        Records_RecvMsg2E(from, data, 1);
        break;
    case 48:
        Records_RecvMsg2E(from, data, 2);
        break;
    case 1:
    case 8:
        TownMsg_OnActor(idx, data);
        break;
    case 2:
        TownMsg_OnMove(idx, data, cmd);
        break;
    case 3:
        TownMsg_OnArea(idx, data);
        break;
    case 36:
        TownMsg_On24(from, data);
        break;
    case 37:
        TownMsg_On25((s8 *)data);
        break;
    case 35:
        if (!LobbyBgm_IsActive()) {
            memcpy(bgm, data, 8);
            if (gScene.area == AREA_HUB_0x87) {
                LobbyBgm_Play(bgm[0], bgm[1], 0);
            } else {
                LobbyBgm_Play(bgm[0], bgm[1], 1);
            }
        }
        break;
    case 38:
        if (!LobbyBgm_IsActive()) {
            memcpy(&pos, data, 4);
            if (gScene.area == AREA_HUB_0x87) {
                LobbyBgm_Play(0, pos, 0);
            } else {
                LobbyBgm_Play(0, pos, 1);
            }
        }
        break;
    case 39:
        switch (gPlayerText.card.arenaRole) {
        case 1:
        case 3:
            ArenaMenu_Open();
            break;
        default:
            gTownMode = 0x40;
            LobbyText_Open(0x8D);
            break;
        }
        break;
    case 40:
        TownMsg_On28((s32)data);
        break;
    case 41:
        memcpy(D_6ADAD4, data, 0x38);
        memcpy(D_6ADB0C, data + 0x38, 8);
        switch (gPlayerText.card.arenaRole) {
        default:
            D_6ADB14 = -1;
            break;
        case 1:
        case 3:
            break;
        }
        break;
    case 42:
        memcpy(D_6ADB16, data, 0x18);
        break;
    case 27:
        TownMsg_On1B(from, data);
        break;
    case 28:
        TownMsg_On1C(idx, data);
        break;
    case 29:
        gTownSeats[data[0]] = 0;
        break;
    case 30:
        TownMsg_On1E(from, data);
        break;
    case 31:
        idx = Lobby_FindMember(data);
        if (idx != 0xFF && gTownMode != 0x30) {
            me = gPlayers[gScene.localPlayer].state;
            if (me == 0x70 || me == 0x71) {
                Contest_Open(&gPlayers[idx]);
            }
        }
        break;
    case 34:
        Contest_RecvStart(from);
        break;
    case 32:
        Contest_RecvPower(data);
        break;
    case 33:
        Contest_RecvResult(idx, data);
        break;
    case 4:
        TownMsg_On04(from, data);
        break;
    case 21:
        TownMsg_On15(from, data);
        break;
    case 5:
        TownMsg_On05(idx, data);
        break;
    case 22:
        TownMsg_On16(idx, data);
        break;
    case 7:
        ActSpot_Release(data[0]);
        break;
    case 23:
        Lobby_ClearReadyBit((s8 *)data);
        break;
    case 26:
        if (LobbyMember_InMyParty(idx) == 1) {
            memcpy(buf, data, 0x1A);
            NetQuest_SetInfo(buf);
            NetQuest_SetUnk4C(buf[0x19]);
            gSeasonFlags = buf[0x18];
            gLobbyLastSeason = Season_Get();
            gLobbyLastPm = LobbyTime_IsPm();
        }
        break;
    case 25:
        me = gScene.localPlayer;
                if ((u8)gLobby->memberSlot[idx] == (u8)gLobby->memberSlot[me]) {
            pl = &gPlayers[me];
            if (pl->state == 0x25 && pl->stateGroup == 0 && MyHouse_IsOwner() == 0) {
                memcpy(&tmp, data, 2);
            }
        }
        break;
    case 18:
        pl = &gPlayers[idx];
        if (pl->stateGroup == 0 && pl->state == 0xB) {
            Actor_SetIdle(pl, 0, 2, 0);
        }
        break;
    case 15:
        TownMsg_On0F(data);
        break;
    case 24:
        Lobby_SetReadyBit(data);
        break;
    case 20:
        Lobby_SetMemberSlot(idx, data);
        break;
    case 6:
        TownMsg_On06(idx);
        break;
    case 10:
        TownMsg_On0A(idx, data);
        break;
    case 11:
        TownMsg_On0B(from, data);
        break;
    case 12:
        Delivery_Take(data);
        break;
    case 13:
        TownMsg_On0D(from);
        break;
    case 14:
        Lobby_ShowMemberName(idx, data);
        break;
    case 9:
    case 17:
    case 19:
        break;
    }
}

void TownMsg_On31(s8 idx, u8 *data)
{
    if (Area_IsOnlineHouse(gScene.area) != 0 && idx == (u8)gLobby->memberSlot[gScene.localPlayer] - 0x30) {
        switch (data[0]) {
        case 0:
            KitchenGuest_Open();
            break;
        case 1:
            KitchenGuest_OnRoute(data[1]);
            break;
        case 3:
            KitchenGuest_OnDone();
            break;
        case 2:
            KitchenGuest_OnChosen(data + 1);
            break;
        }
    }
}