#ifndef COMBINE_SHOP_H
#define COMBINE_SHOP_H

#include "common.h"
#include "menu_task.h"
#include "hud.h"
#include "lobby_menu.h"

/*
 * The town's combination shop (TOWN_MODE_COMBINE_SHOP). Its menu works in
 * the shared menu work gMenuWork as a CombMenu: CombShop_Open opens it at
 * the shopkeeper, CombShop_Update runs it (combining items by the shop's
 * recipes, buying, selling) and CombShop_Draw draws it. Item_IsCombinable,
 * CombShop_CanCombine and CombShop_Price say what the shop can make and
 * what it costs.
 */

/* The combination shop's menu, in the shared menu work gMenuWork: the buy
 * and sell list, the combination window's slide, the top menu, and the
 * list's page: up to 7 items, each with up to 3 recipes and what each
 * makes. */
typedef struct CombMenu {
    /* 0x000 */ ItemListWin list;   /* the buy and sell list (Shop_StartBuy, Shop_StartSell) */
    /* 0x040 */ WinSlide slideWin;  /* the combination window's slide in (its pos is the window's x) */
    /* 0x048 */ s16 menu[4];        /* the top menu's entries: 5 combine, 1 buy, 2 sell, 4 the offers (when the shopkeeper has some) */
    /* 0x050 */ s16 cursor;         /* the top menu's cursor */
    /* 0x052 */ s16 menuCount;      /* the top menu's entry count */
    /* 0x054 */ u16 items[8];       /* the page's items (7 used: CombShop_BuildList clears 7) */
    /* 0x064 */ s32 yieldBase[7][3]; /* per item and recipe, what one combination makes with the fixed-yield skill (Recipe_YieldBase) */
    /* 0x0B8 */ u8 recipeCounts[8]; /* per item, its recipes (Combine_ListRecipesFor; 7 used) */
    /* 0x0C0 */ RecipeHit lines[7][3];
    /* 0x1BC */ s16 listPage;     /* the page of 7 combinable items shown */
    /* 0x1BE */ s16 listPages;
    /* 0x1C0 */ s16 itemCount;    /* the items on this page (item[], per[], count[], lines[]) */
    /* 0x1C2 */ s16 itemCursor;     /* the cursor on the page's items */
    /* 0x1C4 */ s16 infoPage;       /* a decoration's details page, 0-2 (ItemInfo_Draw's page): PAD_R1 and PAD_L1 turn it */
    /* 0x1C6 */ s16 amount;
    /* 0x1C8 */ s16 line;
    /* 0x1CA */ s16 lineCount;
    /* 0x1CC */ s8 details;         /* 1 while the item's details show (PAD_SQUARE; ItemInfo_Draw's mode) */
    /* 0x1CD */ u8 npc;             /* the shopkeeper's id (NpcWork.id): Npc_Find, Shop_StartBuy's stock */
    /* 0x1CE */ s8 state;           /* 0 the top menu, 1 the combination, 2 buying, 3 selling, 4 the goodbye talk */
    /* 0x1CF */ s8 step;            /* CombShop_Steps: 0 the window slides in, 1 the item, 2 the recipe, 3 the amount, 4 the yes/no */
} CombMenu;

/* CombMenu.state: what the shop shows (CombShop_Update, CombShop_Draw). */
enum CombState {
    COMB_STATE_TOP = 0,         /* the top menu */
    COMB_STATE_COMBINE = 1,     /* the combination's window (CombShop_Steps) */
    COMB_STATE_BUY = 2,         /* the buy list */
    COMB_STATE_SELL = 3,        /* the sell list */
    COMB_STATE_TALK = 4         /* the shopkeeper's goodbye */
};

/* CombMenu.step: the combination's steps (CombShop_Steps). */
enum CombStep {
    COMB_STEP_SLIDE_IN = 0,     /* the window slides in */
    COMB_STEP_ITEM = 1,         /* an item of the list */
    COMB_STEP_RECIPE = 2,       /* one of its recipes */
    COMB_STEP_AMOUNT = 3,       /* how many times */
    COMB_STEP_CONFIRM = 4       /* the yes/no */
};

/* CombMenu.menu's entries: the top menu's choices. */
enum CombEntry {
    COMB_ENTRY_BUY = 1,
    COMB_ENTRY_SELL = 2,
    COMB_ENTRY_OFFERS = 4,      /* only when the shopkeeper has offers */
    COMB_ENTRY_COMBINE = 5
};

#define COMB_PAGE_ITEMS 7       /* the items on a page of the combination list */
#define ITEM_INFO_PAGES 3       /* a decoration's details pages (CombMenu.infoPage) */

void CombShop_Open(u8 *npc);
void CombShop_Close(void);
void CombShop_Update(void);
void CombShop_OpenList(CombMenu *shop);
s32 CombShop_Steps(CombMenu *shop, u16 pad);
void CombShop_Combine(CombMenu *shop);
s32 CombShop_InputAmount(CombMenu *shop, u16 pad);
s32 CombShop_InputLine(CombMenu *shop, u16 pad);
s32 CombShop_InputItem(CombMenu *shop, u16 pad);
s32 Item_IsCombinable(u16 item);
s32 CombShop_CanCombine(RecipeHit *entry, u16 item, s32 amount, s32 yieldEach);
s32 CombShop_ItemReady(CombMenu *shop, s16 index);
void CombShop_BuildList(CombMenu *shop, s16 page);
s32 CombShop_MenuInput(CombMenu *shop, u16 pad);
s32 CombShop_Price(u16 item);
void CombShop_DrawList(CombMenu *shop, s16 x, s16 y);
void CombShop_DrawLine(CombMenu *shop, RecipeHit *entry, u8 lineIndex, s16 needAmount, s16 x, s16 y);
void CombShop_DrawLineWindow(CombMenu *shop, s16 x, s16 y);
void CombShop_DrawAmount(CombMenu *shop, s16 x, s16 y);
void CombShop_DrawWindow(CombMenu *shop, s16 x, s16 y);
void CombShop_Draw(MenuTask *task);

#endif
