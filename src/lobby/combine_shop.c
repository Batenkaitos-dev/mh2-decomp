/*
 * The town's combination shop (TOWN_MODE_COMBINE_SHOP).
 * - The shop's menu works in the shared menu work gMenuWork as CombMenu.
 *   CombShop_Open opens it at the shopkeeper and CombShop_Close closes it;
 *   CombShop_Update runs the top menu (combine, buy, sell and, when the
 *   shopkeeper has some, the offers; leaving starts the goodbye talk). Buying
 *   and selling are the item shop's lists (Shop_StartBuy, Shop_StartSell).
 * - The combination (CombShop_Steps): a list of the items the shop's recipes
 *   make, 7 a page (CombShop_BuildList, Item_IsCombinable); an item; one of
 *   its recipes (RecipeHit, up to 3 an item); the amount; then a yes/no, and
 *   CombShop_Combine makes them and takes the fee (CombShop_Price).
 *   CombShop_CanCombine and CombShop_ItemReady say what can be made now.
 * - The draws: CombShop_Draw by the menu's state, and the combination
 *   window (CombShop_DrawWindow and the draws before it).
 * The .rodata is the shop's print formats.
 */
#include "common.h"
#include "lobby_text.h"
#include "pad.h"
#include "quest.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "item.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "hud.h"
#include "scene.h"
#include "save.h"
#include "em.h"
#include "player.h"
#include "lobby_menu.h"
#include "combine.h"
#include "area.h"
#include "lobby_npc.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "camera.h"
#include "text.h"
#include "combine_shop.h"
#include "font.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "talk.h"
#include "town_frame.h"

/* prototypes */
/* end of prototypes */

void *memset(void *p, s32 c, u32 n);
struct Em;
extern u8 gMenuWork[];
extern u8 gMenuInputLock;
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* Opens the combination shop's menu at the townsperson: its town mode, the
 * menu work cleared, its draw, input held for 6 frames, and the top menu's
 * entries (CombMenu.menu): combine (5), buy (1), sell (2), and the offers (4)
 * when the townsperson has some (OfferMenu_BuildList); the shopkeeper's id
 * is kept for the menu (CombMenu.npc). */
void CombShop_Open(u8 *npc)
{
    CombMenu *shop = (CombMenu *)gMenuWork;
    NpcWork *npcWork = (NpcWork *)(npc + 1080);

    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_COMBINE_SHOP;
    memset(shop, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(CombShop_Draw, 0, 0, 0);
    gMenuInputLock = 6; /* frames before the menu takes buttons */
    shop->menu[0] = COMB_ENTRY_COMBINE;
    shop->menu[1] = COMB_ENTRY_BUY;
    shop->menu[2] = COMB_ENTRY_SELL;
    shop->menuCount = 3;
    if (OfferMenu_BuildList((struct Em *) npc, 0) != 0) {
        shop->menu[3] = COMB_ENTRY_OFFERS;
        shop->menuCount += 1;
    }
    shop->npc = npcWork->id;
    LobbySe_Play(LOBBY_SE_MENU_OPEN);
}

/* Closes the combination shop's menu once its talk ends: back to
 * TOWN_MODE_FREE with the menu state and the draws cleared, the camera given
 * back, the cancel sound, and the chat window's colour back to normal. */
void CombShop_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
    Chat_SetColor(-1); /* the chat window's normal colour */
}

extern Player gPlayers[];

void Shop_DrawBuy(void *);
void Shop_DrawSell(void *);

u16 LobbyPad_Get(s32 a);
void Shop_StartBuy(ItemListWin *list, u16 kind);
void Shop_StartSell(ItemListWin *list);
s32 Shop_BuyStep(u16 pad, ItemListWin *list);
s32 Shop_SellStep(u16 pad, ItemListWin *list);

/* The combination shop's menu, each frame, by CombMenu.state: the top menu
 * (PAD_CIRCLE takes its entry: the combination's window slides in, the buy
 * or the sell list opens, or the shopkeeper's offers open; PAD_CROSS starts
 * the goodbye talk), then the combination's steps, the buy list or the sell
 * list, each back to the top menu with the camera when left; the talk's end
 * closes the shop (CombShop_Close). */
void CombShop_Update(void)
{
    CombMenu *shop = (CombMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (shop->state) {
    case COMB_STATE_TOP:
        switch (CombShop_MenuInput(shop, pad)) {
        case MENU_DONE:
            switch (shop->menu[shop->cursor]) {
            case COMB_ENTRY_COMBINE:
                shop->state = COMB_STATE_COMBINE;
                CombShop_OpenList(shop);
                Chat_SetColor(0);
                CamFocus_SetAside(1);
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
                break;
            case COMB_ENTRY_BUY:
                shop->state = COMB_STATE_BUY;
                Shop_StartBuy(&shop->list, shop->npc);
                CamFocus_SetAside(1);
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
                break;
            case COMB_ENTRY_SELL:
                shop->state = COMB_STATE_SELL;
                Shop_StartSell(&shop->list);
                CamFocus_SetAside(1);
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case COMB_ENTRY_OFFERS:
                OfferMenu_Open((struct Em *)(Npc_Find(shop->npc)));
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            }
            break;
        case MENU_CANCEL:
            shop->state = COMB_STATE_TALK;
            Talk_Say(1, 0); /* the shopkeeper's goodbye */
            break;
        }
        break;
    case COMB_STATE_COMBINE:
        switch (CombShop_Steps(shop, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            shop->state = COMB_STATE_TOP;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case COMB_STATE_BUY:
        switch (Shop_BuyStep(pad, &shop->list)) {
        case MENU_DONE:
        case MENU_CANCEL:
            shop->state = COMB_STATE_TOP;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case COMB_STATE_SELL:
        switch (Shop_SellStep(pad, &shop->list)) {
        case MENU_DONE:
        case MENU_CANCEL:
            shop->state = COMB_STATE_TOP;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case COMB_STATE_TALK:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            CombShop_Close();
            break;
        }
        break;
    }
}

/* Opens the combination's window: the list's first page built, the
 * window's slide started, the amount 1 and the details hidden. */
void CombShop_OpenList(CombMenu *shop)
{
    CombShop_BuildList(shop, 0);
    WinSlide_Step(&shop->slideWin, 640, 270);
    shop->amount = 1;
    shop->details = 0;
}

s32 Item_IsDeco(u16 id);

/* The combination's steps (CombMenu.step): the window slides in; an item
 * is picked from the list (CombShop_InputItem); one of its recipes
 * (CombShop_InputLine); the amount (CombShop_InputAmount); then the yes/no:
 * yes combines (CombShop_Combine), rebuilds the page and goes back to the
 * list, no goes back to the recipe. At the yes/no, PAD_SQUARE, PAD_R1 and
 * PAD_L1 work as in the amount step. Returns MENU_CANCEL when the player
 * leaves the list, else MENU_RUNNING. */
s32 CombShop_Steps(CombMenu *shop, u16 pad)
{
    switch (shop->step) {
    case COMB_STEP_SLIDE_IN:
        switch (WinSlide_Step(&shop->slideWin, 640, 270)) {
        case MENU_DONE: /* settled */
            (shop->step)++;
            break;
        }
        break;
    case COMB_STEP_ITEM:
        switch (CombShop_InputItem(shop, pad)) {
        case MENU_DONE:
            (shop->step)++;
            shop->amount = 1;
            break;
        case MENU_CANCEL:
            shop->step = COMB_STEP_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case COMB_STEP_RECIPE:
        switch (CombShop_InputLine(shop, pad)) {
        case MENU_DONE:
            (shop->step)++;
            shop->amount = 1;
            break;
        case MENU_CANCEL:
            shop->step = COMB_STEP_ITEM;
            break;
        }
        break;
    case COMB_STEP_AMOUNT:
        switch (CombShop_InputAmount(shop, pad)) {
        case MENU_DONE:
            (shop->step)++;
            Menu_SetYesNo(0); /* the cursor on yes */
            gMenuInputLock = 9; /* frames before the yes/no takes buttons */
            break;
        case MENU_CANCEL:
            shop->step = COMB_STEP_RECIPE;
            shop->amount = 1;
            break;
        }
        break;
    case COMB_STEP_CONFIRM:
        if (pad & PAD_SQUARE) {
            if (shop->itemCount != 0) {
                shop->details ^= 1;
                LobbySe_Play(LOBBY_SE_DESCRIPTION);
            }
        } else if (pad & PAD_R1) {
            if (shop->details != 0 && Item_IsDeco(shop->items[shop->itemCursor]) != 0) {
                if (++shop->infoPage >= ITEM_INFO_PAGES) {
                    shop->infoPage = 0;
                }
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else if (pad & PAD_L1) {
            if (shop->details != 0 && Item_IsDeco(shop->items[shop->itemCursor]) != 0) {
                if (--shop->infoPage < 0) {
                    shop->infoPage = ITEM_INFO_PAGES - 1;
                }
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else {
            switch (Menu_YesNoInput()) {
            case MENU_DONE: /* yes */
                shop->step = COMB_STEP_ITEM;
                CombShop_Combine(shop);
                CombShop_BuildList(shop, shop->listPage);
                break;
            case MENU_CANCEL: /* no */
                shop->step = COMB_STEP_RECIPE;
                shop->amount = 1;
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/* Makes the chosen recipe (the cursor's item and recipe) CombMenu.amount
 * times (Combine_Do, or Combine_Do3 for a three-item recipe; the ingredients
 * come from the save's pouch, or the box), takes the fee (CombShop_Price of
 * the item made, times the amount) with the money sound, and gives the
 * total made (Items_Give), with a notice when some went to the box. */
void CombShop_Combine(CombMenu *shop)
{
    RecipeHit *choice;
    void *pl;
    u16 item;
    s32 total = 0;
    s32 i = 0;
    u8 got;

    choice = &shop->lines[shop->itemCursor][shop->line];
    pl = &gPlayers[gScene.localPlayer];
    for (; i < shop->amount; i++) {
        if (choice->info[2] == 0) {
            item = Combine_Do(&got, (struct Player *)pl, choice->info[0], choice->info[1], 1); /* mode 1: from the save's pouch, or the box */
        } else {
            item = Combine_Do3(&got, (struct Player *)pl, choice->info[0], choice->info[1], choice->info[2], 1);
        }
        total += got;
    }
    Save_AddMoney((-(CombShop_Price(item) * shop->amount)));
    LobbySe_Play(LOBBY_SE_MONEY);
    if (Items_Give(item, total) != 0) { /* some went to the box */
        Menu_PushNoticeOnce(LOBBY_NOTICE_SENT_TO_BOX);
    }
}

/* The amount step's input: PAD_CIRCLE takes the amount (MENU_DONE) and
 * PAD_CROSS goes back (MENU_CANCEL); PAD_SQUARE shows or hides the item's
 * details, and while they show a decoration PAD_R1 and PAD_L1 turn its
 * pages; PAD_UP adds one while CombShop_CanCombine allows it, PAD_DOWN takes
 * one off, PAD_LEFT goes back to 1, PAD_RIGHT goes to the most it allows.
 * Returns MENU_RUNNING otherwise. */
s32 CombShop_InputAmount(CombMenu *shop, u16 pad)
{
    RecipeHit *choice = &shop->lines[shop->itemCursor][shop->line];
    s32 tryAmount;
    s8 ok;

    if (pad & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_SQUARE) {
        if (shop->itemCount != 0) {
            shop->details ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & PAD_R1) {
        if (shop->details != 0 && Item_IsDeco(shop->items[shop->itemCursor]) != 0) {
            if (++shop->infoPage >= ITEM_INFO_PAGES) {
                shop->infoPage = 0;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else if (pad & PAD_L1) {
        if (shop->details != 0 && Item_IsDeco(shop->items[shop->itemCursor]) != 0) {
            if (--shop->infoPage < 0) {
                shop->infoPage = ITEM_INFO_PAGES - 1;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else if (pad & PAD_UP) {
        if (CombShop_CanCombine(choice, shop->items[shop->itemCursor], shop->amount + 1,
                                shop->yieldBase[shop->itemCursor][shop->line]) != 0) {
            (shop->amount)++;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_DOWN) {
        if (shop->amount > 1) {
            (shop->amount)--;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_LEFT) {
        if (shop->amount > 1) {
            shop->amount = 1;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_RIGHT) {
        ok = 0;
        for (tryAmount = shop->amount + 1; tryAmount <= 1000; tryAmount++) { /* CombShop_CanCombine stops it under 100 */
            if (CombShop_CanCombine(choice, shop->items[shop->itemCursor], tryAmount,
                                    shop->yieldBase[shop->itemCursor][shop->line]) == 0) {
                break;
            }
            ok = 1;
        }
        if (ok != 0) {
            shop->amount = tryAmount - 1;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    }
    return MENU_RUNNING;
}


/* The recipe step's input: PAD_CIRCLE takes the recipe under the cursor
 * when it can be made once (MENU_DONE; else the refuse sound); PAD_CROSS
 * hides the details and goes back (MENU_CANCEL); PAD_SQUARE, PAD_R1 and
 * PAD_L1 as in the amount step; up and down move between the item's
 * recipes. Returns MENU_RUNNING otherwise. */
s32 CombShop_InputLine(CombMenu *shop, u16 pad)
{
    s16 lineIndex;
    s16 itemIndex;

    if (pad & PAD_CIRCLE) {
        lineIndex = shop->line;
        itemIndex = shop->itemCursor;
        if (CombShop_CanCombine(&shop->lines[itemIndex][lineIndex], shop->items[itemIndex], 1,
                                shop->yieldBase[itemIndex][lineIndex]) == 1) {
            shop->amount = 1;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        LobbySe_Play(LOBBY_SE_REFUSE);
    } else if (pad & PAD_CROSS) {
        shop->details = 0;
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_SQUARE) {
        if (shop->itemCount != 0) {
            shop->details ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & PAD_R1) {
        if (shop->details != 0 && Item_IsDeco(shop->items[shop->itemCursor]) != 0) {
            if (++shop->infoPage >= ITEM_INFO_PAGES) {
                shop->infoPage = 0;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else if (pad & PAD_L1) {
        if (shop->details != 0 && Item_IsDeco(shop->items[shop->itemCursor]) != 0) {
            if (--shop->infoPage < 0) {
                shop->infoPage = ITEM_INFO_PAGES - 1;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else {
        shop->line = Menu_WrapUpDown(shop->line, shop->lineCount);
    }
    return MENU_RUNNING;
}

/* The list step's input: PAD_CIRCLE takes the item under the cursor when
 * CombShop_ItemReady allows it (MENU_DONE, its recipes' cursor at the first;
 * else the refuse sound, when the page has items); PAD_CROSS hides the
 * details, or leaves the list (MENU_CANCEL) when they are hidden; PAD_SQUARE
 * shows or hides them; left and right turn the list's page when it has
 * several; up and down move between the page's items. Returns MENU_RUNNING
 * otherwise. */
s32 CombShop_InputItem(CombMenu *shop, u16 pad)
{
    if (pad & PAD_CIRCLE) {
        if (CombShop_ItemReady(shop, shop->itemCursor) == 1) {
            shop->amount = 1;
            shop->line = 0;
            shop->lineCount = shop->recipeCounts[shop->itemCursor];
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        if (shop->itemCount != 0) {
            LobbySe_Play(LOBBY_SE_REFUSE);
        }
    } else if (pad & PAD_CROSS) {
        if (shop->details == 0) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
        shop->details = 0;
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
    } else if (pad & PAD_SQUARE) {
        if (shop->itemCount != 0) {
            shop->details ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (shop->listPages > 1) {
            shop->listPage = Menu_WrapLeftRight(shop->listPage, shop->listPages);
            CombShop_BuildList(shop, shop->listPage);
        }
    } else {
        shop->itemCursor = Menu_WrapUpDown(shop->itemCursor, shop->itemCount);
    }
    return MENU_RUNNING;
}

/* 1 when the shop lists the item: the pouch may hold more than one of it
 * and a known recipe makes it (Recipe_IsListedFor). */
s32 Item_IsCombinable(u16 item)
{
    if (Item_IsOnePerPouch(item) == 1) {
        return 0;
    } else if (Recipe_IsListedFor(item, 0) != 0) { /* flags 0: the known recipes only */
        return 1;
    }
    return 0;
}

/* 1 when recipe `entry` can make `item` `amount` times now: the recipe is
 * known, the money covers that many fees, the player holds that many of each
 * of its ingredients (Items_HaveEnough), the pouch and the box can take
 * amount times yieldEach (what one combination makes), and amount is under
 * 100. */
s32 CombShop_CanCombine(RecipeHit *entry, u16 item, s32 amount, s32 yieldEach)
{
    s32 i;

    if (Recipe_IsKnown(entry->recipe) == 0) {
        return 0;
    }
    if (amount * CombShop_Price(item) > gSave.money) {
        return 0;
    }
    for (i = 0; i < 3; i++) {
        if (entry->info[i] != 0 && Items_HaveEnough(entry->info[i], amount) == 0) {
            return 0;
        }
    }
    if (Items_Room(item) < amount * yieldEach) {
        return 0;
    }
    if (amount < 100) { /* at most 99 at a time */
        return 1;
    }
    return 0;
}

/* The game built this function at optimization level 4. */
#pragma optimization_level 4
/* 1 when the money covers the fee of the page's item `index` (an index into
 * CombMenu.items) and one of its recipes can be made once now. */
s32 CombShop_ItemReady(CombMenu *shop, s16 index)
{
    s32 i;

    if (CombShop_Price(shop->items[index]) > gSave.money) {
        return 0;
    }
    for (i = 0; i < shop->recipeCounts[index]; i++) {
        if (CombShop_CanCombine(&shop->lines[index][i], shop->items[index], 1, shop->yieldBase[index][i]) == 1) {
            return 1;
        }
    }
    return 0;
}
#pragma optimization_level reset

void *memset(void *d, s32 c, u32 n);
u8 Combine_ListRecipesFor(struct RecipeHit *lines, s16 item, s32 flags);
struct Recipe;

/* Builds page `page` of the combination list: of the items the shop lists
 * (Item_IsCombinable), that page's 7, each with its recipes (the unknown
 * ones too) and what each makes with the fixed-yield skill
 * (Recipe_YieldBase); then the page count, with the item cursor and the page
 * kept in range. */
void CombShop_BuildList(CombMenu *shop, s16 page)
{
    s16 first;
    s16 last;
    s16 listed;
    u16 item;
    u16 j;

    memset(shop->items, 0, sizeof(shop->items) - 2);
    memset(shop->yieldBase, 0, sizeof(shop->yieldBase));
    memset(shop->recipeCounts, 0, sizeof(shop->recipeCounts) - 1);
    memset(shop->lines, 0, sizeof(shop->lines));
    shop->itemCount = 0;
    listed = 0;
    first = page * COMB_PAGE_ITEMS;
    last = first + COMB_PAGE_ITEMS - 1;
    for (item = 0; item < ITEM_ID_COUNT; item++) {
        if (Item_IsCombinable(item) == 1) {
            if (first <= listed && listed <= last) {
                shop->items[shop->itemCount] = item;
                shop->recipeCounts[shop->itemCount] = Combine_ListRecipesFor(shop->lines[shop->itemCount], item, RECIPE_LIST_UNKNOWN);
                for (j = 0; j < shop->recipeCounts[shop->itemCount]; j++) {
                    shop->yieldBase[shop->itemCount][j] = Recipe_YieldBase(shop->lines[shop->itemCount][j].recipe);
                }
                shop->itemCount++;
            }
            listed++;
        }
    }
    shop->listPage = page;
    shop->listPages = Menu_CountPages(listed, COMB_PAGE_ITEMS);
    if (shop->itemCursor >= shop->itemCount) {
        shop->itemCursor = shop->itemCount - 1;
    }
    if (shop->listPage >= shop->listPages) {
        shop->listPage = shop->listPages - 1;
    }
}

/* The top menu's input: PAD_CIRCLE takes the entry under the cursor
 * (MENU_DONE), PAD_CROSS leaves with the cancel sound (MENU_CANCEL); up and
 * down move the cursor. Returns MENU_RUNNING otherwise. */
s32 CombShop_MenuInput(CombMenu *shop, u16 pad)
{
    s32 btn;

    btn = pad & PAD_ALL;
    if (btn & PAD_CIRCLE) {
        return MENU_DONE;
    }
    if (btn & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    shop->cursor = Menu_WrapUpDown((s32) shop->cursor, (s32) shop->menuCount);
    return MENU_RUNNING;
}



/* The combination's fee for an item: 14% of its price (42% above rarity
 * 2), halved during the town's sale (TOWN_EVENT_SALE); at least 1. */
s32 CombShop_Price(u16 item)
{
    f32 rate;
    s32 fee;

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
    if (gLobby->townEvent == TOWN_EVENT_SALE) {
        rate *= 0.5f;
    }
    fee = rate * (f32)(u32)gItemData[item].price;
    if (fee == 0) {
        fee = 1;
    }
    return fee;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/combine_shop", gCombStrFmt);
INCLUDE_RODATA("asm/lobby/nonmatchings/combine_shop", gCombFeeFmt);

void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
/* unit's own declaration: the width, mode, extra value, page and colour are declared s32 (the definition takes s16, u8, u16, s8 and u32); with the definition's types the calls in CombShop_DrawLineWindow build differently from the game's code */
void ItemInfo_Draw(s16 x, s16 y, s32 w, s32 mode, s32 item, s32 extra, s32 page, s32 color);
extern char *gItemNames[];
extern char gCombStrFmt[];
extern char gCombFeeFmt[];

/* Draws the combination list at (x, y): its sheet; with nothing to list,
 * the shop's no-recipes text; else the page counter, each item's icon, name
 * and fee (dark when CombShop_ItemReady allows it, dim otherwise), the
 * cursor's row, and under a dashed line the cursor item's details
 * (ItemInfo_Draw). */
void CombShop_DrawList(CombMenu *shop, s16 x, s16 y)
{
    s32 i;
    s32 price;
    s32 item;
    char *text;
    s16 rowY = y;

    Icon_Draw(x, y, 350, 306, -35, 0);
    Icon_Draw(x + 100, y + 78, 150, 150, 0x60FFFFFF, 34);
    if (shop->itemCount == 0) {
        Font_SetSpacing(20, 20);
        text = gMiscTexts[MISC_TEXT_NO_COMBINE];
        Font_SetColor(0x8000052C);
        Font_SetPos(Text_CenterX(text, x + 175, 20), y + 120);
        Text_PrintfColored(gCombStrFmt, text);
        return;
    }
    rowY += 10;
    Font_SetColor(0x80000000);
    Menu_DrawPageNum(x + 250, rowY, shop->listPage, shop->listPages, 1);
    Font_SetSpacing(20, 20);
    for (i = 0; i < shop->itemCount; i++) {
        item = shop->items[i];
        price = CombShop_Price(item);
        if (CombShop_ItemReady(shop, i) == 1) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Font_SetSpacing(20, 20);
        Font_SetPos(x + 46, rowY += 24);
        Text_PrintfColored(gCombStrFmt, gItemNames[(u16)item]);
        Font_SetSpacing(20, 20);
        Font_SetPos(x + 248, rowY);
        Text_PrintfColored(gCombFeeFmt, price);
        Item_DrawIcon(x + 16, rowY - 1, 24, (u16)item);
        if (shop->itemCursor == i) {
            Menu_DrawCursorRow(x + 46, rowY, 250, 50, 0xC0106060, 0xFF106060);
        }
    }
    item = shop->items[shop->itemCursor];
    Menu_DrawDashLine(x + 20, y + 202, 310, 0xFF000000);
    if (item == 0 || item >= ITEM_ID_COUNT) {} /* an empty check of the item's range, as the game's code has it */
    ItemInfo_Draw(x, y + 206, 350, 0, (u16)item, 0, -1, 0x80101010);
}


/* The combination list drawn small at the window's left while the details
 * show (its place is fixed: the position passed is unused): the page
 * counter, then each item's icon, name and fee, dark when
 * CombShop_ItemReady allows it, and the cursor's row. */
void CombShop_DrawListSmall(CombMenu *shop, s16 unusedX, s16 unusedY)
{
    s32 i;
    s32 price;
    s32 item;
    s16 rowY;

    Icon_Draw(0, 50, 290, 180, -35, 0);
    Icon_Draw(105, 90, 80, 100, 0x60FFFFFF, 34);
    rowY = 60;
    Font_SetColor(0x80000000);
    Font_SetSpacing(17, 17);
    Menu_DrawPageNum(170, rowY, shop->listPage, shop->listPages, 1);
    for (i = 0; i < shop->itemCount; i++) {
        item = shop->items[i];
        price = CombShop_Price(item);
        if (CombShop_ItemReady(shop, i) == 1) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Font_SetPos(56, rowY += 20);
        Text_PrintfColored(gCombStrFmt, gItemNames[(u16)item]);
        Font_SetPos(202, rowY);
        Text_PrintfColored(gCombFeeFmt, price);
        Item_DrawIcon(26, rowY - 1, 20, (u16)item);
        if (shop->itemCursor == i) {
            Menu_DrawCursorRow(56, rowY, 180, 50, 0xC0106060, 0xFF106060);
        }
    }
}

/* Draws recipe `entry` at (x, y): its label (number lineIndex), then each
 * of its ingredients with the count needAmount combinations need (on the amount and yes/no
 * steps, for the chosen amount and with the pouch and box counts as they
 * will be: Items_DrawNeedLineAfter); an unknown recipe (Recipe_IsKnown)
 * shows the unknown label in their place. */
void CombShop_DrawLine(CombMenu *shop, RecipeHit *entry, u8 lineIndex, s16 needAmount, s16 x, s16 y)
{
    s32 k;
    char *text;

    Font_SetColor(0x807F7F28);
    Font_SetPos(x, y);
    Text_PrintfColored(gCombineShopTexts[lineIndex]);
    Font_SetColor(MENU_INK_WHITE);
    if (Recipe_IsKnown(entry->recipe) == 1) {
        for (k = 0; k < 3; k++) {
            if (entry->info[k] != 0) {
                if (shop->step == COMB_STEP_AMOUNT || shop->step == COMB_STEP_CONFIRM) {
                    y += 20;
                    Items_DrawNeedLineAfter(x + 40, y, entry->info[k], shop->amount, MENU_INK_WHITE);
                } else {
                    y += 20;
                    Items_DrawNeedLine(x + 40, y, entry->info[k], needAmount, MENU_INK_WHITE);
                }
            }
        }
    } else {
        text = gCombineShopTexts[COMBINE_SHOP_TEXT_UNKNOWN];
        Font_SetColor(0x807F212E);
        Font_SetPos(Text_CenterX(text, x + 166, 18), y + 40);
        Text_PrintfColored(gCombStrFmt, text);
    }
}

/* unit's own declaration: x and y are declared s16 and the kind s32 (the definition takes s32 x and y and an s16 kind); with the definition's types the window draws here build differently from the game's code */
void Win_Draw(s16 x, s16 y, s32 w, s32 h, s32 color1, s32 color2, s32 kind);

/* The details' recipe window at (x, y): each recipe of the cursor's item
 * (CombShop_DrawLine, for one combination), 90 apart. */
void CombShop_DrawLines(CombMenu *shop, s16 x, s16 y)
{
    s16 i;
    s32 offset;

    Win_Draw(x, y, 332, 294, 0xBDB59F6C, 0xA0202020, 4);
    Font_SetSpacing(18, 18);
    i = 0;
    y += 10;
    if (i < shop->recipeCounts[shop->itemCursor]) {
        offset = 0;
        do {
            /* the recipe is reached by its byte offset in CombMenu.lines (rows of three 12-byte recipes, 36 bytes, from offset 0xC0): written by index, the compiler adds the addresses in another order than the game's code */
            CombShop_DrawLine(shop, (RecipeHit *)((u8 *)shop + shop->itemCursor * 36 + offset + 192), i, 1, x, y);
            y += 90;
            offset += 12;
            i++;
        } while (i < shop->recipeCounts[shop->itemCursor]);
    }
}

/* The recipe step's window at (x, y): the item's details (a decoration's
 * on its page), the item's recipe labels (bright when CombShop_CanCombine
 * allows one combination, grey otherwise) with the cursor's bar, then the
 * chosen recipe's ingredients (CombShop_DrawLine). */
void CombShop_DrawLineWindow(CombMenu *shop, s16 x, s16 y)
{
    s16 rowY;
    s16 i;
    s32 top;
    u16 item = shop->items[shop->itemCursor];

    Win_Draw(x, y, 332, 294, 0xBDB59F6C, 0xA0202020, 4);
    Font_SetSpacing(18, 18);
    if (Item_IsDeco(item) != 0) {
        ItemInfo_Draw(x, y, 344, (u8)shop->details, item, 0, (s8)shop->infoPage, MENU_INK_WHITE);
    } else {
        ItemInfo_Draw(x, y, 344, (u8)shop->details, item, 0, -1, MENU_INK_WHITE);
    }
    Font_SetSpacing(18, 18);
    top = y;
    Gfx_DrawLine(x + 20, top + 94, 310, 2, 0xBDB59F6C);
    rowY = top + 102;
    for (i = 0; i < shop->lineCount; i++) {
        if (CombShop_CanCombine(&shop->lines[shop->itemCursor][i], shop->items[shop->itemCursor], 1, shop->yieldBase[shop->itemCursor][i]) == 1) {
            Font_SetColor(MENU_INK_WHITE);
        } else {
            Font_SetColor(0x80404040);
        }
        Font_SetPos(x + 20, rowY);
        Text_PrintfColored(gCombineShopTexts[i + 3]);
        if (i == shop->line) {
            Menu_DrawRowHighlight(x + 10, rowY - 1, 312, 24, shop->step != COMB_STEP_RECIPE);
        }
        rowY += 20;
    }
    Gfx_DrawLine(x + 20, top + 192, 310, 2, 0xBDB59F6C);
    CombShop_DrawLine(shop, &shop->lines[shop->itemCursor][shop->line], shop->line, shop->amount, x, top + 196);
}

/* The amount step's window at (x, y): the item's details, the chosen
 * recipe's ingredients for the amount, a down arrow, then the item's icon
 * and name, what the amount makes (at least, and with the fixed-yield skill:
 * Recipe_YieldAmount and Recipe_YieldBase times the amount) and the fee.
 * FAKEMATCH: id = item is written in each branch, not once before the if:
 * the game's code converts the item in each branch and keeps it in one saved
 * register for Item_DrawIcon. Without id, or with id set once before the if,
 * the compiler converts it elsewhere and the registers differ from the
 * game's. */
void CombShop_DrawAmount(CombMenu *shop, s16 x, s16 y)
{
    u16 item = shop->items[shop->itemCursor];
    s16 rowY;
    s32 top;
    s32 left;
    s32 id;
    s32 textX;
    s32 yieldMin;
    s32 price;
    s32 yieldFixed;

    Win_Draw(x, y, 332, 294, 0xBDB59F6C, 0xA0202020, 4);
    Font_SetSpacing(18, 18);
    if (Item_IsDeco(item) != 0) {
        id = item;
        ItemInfo_Draw(x, y, 344, (u8)shop->details, id, 0, (s8)shop->infoPage, MENU_INK_WHITE);
    } else {
        id = item;
        ItemInfo_Draw(x, y, 344, (u8)shop->details, id, 0, -1, MENU_INK_WHITE);
    }
    Font_SetSpacing(18, 18);
    top = y;
    left = x;
    Gfx_DrawLine(left + 20, top + 94, 310, 2, 0xBDB59F6C);
    CombShop_DrawLine(shop, &shop->lines[shop->itemCursor][shop->line], shop->line, shop->amount, x, top + 102);
    Icon_DrawMode(left + 163, top + 192, 36, 22, 0xFF00FF00, 40, 2);
    rowY = top + 210;
    Font_SetSpacing(18, 18);
    textX = left + 40;
    Font_SetPos(textX, top + 210);
    Text_PrintfColored(gCombStrFmt, gItemNames[item]);
    Item_DrawIcon(left + 12, (s16)(top + 210) - 1, 20, id);
    Font_SetPos(textX, rowY += 24);
    /* the recipe is reached by its byte offset in CombMenu.lines (rows of three 12-byte recipes, 36 bytes, from offset 0xC0): written by index, the compiler adds the addresses in another order than the game's code */
    yieldMin = (u8)Recipe_YieldAmount((struct Recipe *)(*(s32 *)((u8 *)shop + shop->itemCursor * 36 + shop->line * 12 + 192)));
    yieldMin *= shop->amount;
    yieldFixed = shop->yieldBase[shop->itemCursor][shop->line];
    yieldFixed *= shop->amount;
    Text_PrintfColored("%-17s%s%3d%s%3d", gCombineShopTexts[COMBINE_SHOP_TEXT_RESULT], gLobbyWords[LOBBY_WORD_COLON], yieldMin, gLobbyWords[LOBBY_WORD_TILDE],
                       yieldFixed, shop->amount);
    price = shop->amount * CombShop_Price(item);
    Font_SetPos(textX, rowY + 24);
    Text_PrintfColored("%-17s%s%7dz", gCombineShopTexts[COMBINE_SHOP_TEXT_FEE], gLobbyWords[LOBBY_WORD_COLON], price);
}

void Win_DrawText(s16, s16, s16, s16, u32, u32, char *);
s32 sprintf(char *buf, char *fmt, ...);
extern LobbyMenu *gLobbyMenu;

/* The combination window at (x, y) by step: the list (or its details),
 * the lines, the amount; then the help line, the amount with its arrows,
 * or the total price. */
void CombShop_DrawWindow(CombMenu *shop, s16 x, s16 y)
{
    char buf[32];
    char wide[32];

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(270, 18, 160, 0xFFB0B000, gItemShopMenuTexts[0], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    switch (shop->step) {
    case COMB_STEP_SLIDE_IN:
    case COMB_STEP_ITEM:
        if (shop->details == 0) {
            CombShop_DrawList(shop, x, y);
        } else {
            CombShop_DrawListSmall(shop, x, y);
            CombShop_DrawLines(shop, 288, y + 4);
        }
        break;
    case COMB_STEP_RECIPE:
        CombShop_DrawLineWindow(shop, 288, y + 4);
        break;
    case COMB_STEP_AMOUNT:
    case COMB_STEP_CONFIRM:
        CombShop_DrawAmount(shop, 288, y + 4);
        break;
    }
    Font_SetSpacing(18, 18);
    switch (shop->step) {
    case COMB_STEP_SLIDE_IN:
    case COMB_STEP_ITEM:
        Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_PICK_COMBINE]);
        break;
    case COMB_STEP_RECIPE:
        Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_PICK_RECIPE]);
        break;
    case COMB_STEP_AMOUNT:
        Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_COMBINE_COUNT]);
        Font_SetPos(507, 388);
        sprintf(buf, "%02d", shop->amount);
        Sjis_Widen(buf, wide);
        Text_PrintfColored(wide);
        if (shop->amount <= 98 &&
            CombShop_CanCombine(&shop->lines[shop->itemCursor][shop->line], shop->items[shop->itemCursor], shop->amount + 1,
                                shop->yieldBase[shop->itemCursor][shop->line])) {
            Icon_DrawMode(508, 366, 36, 22, 0xFF00FF00, 41, 2);
        }
        if (shop->amount > 1) {
            Icon_DrawMode(509, 406, 36, 22, 0xFF00FF00, 40, 2);
        }
        break;
    case COMB_STEP_CONFIRM:
        Win_DrawText(288, 352, 332, 80, 0xBDB59F6C, 0xC0101010,
                      Text_FillDollar(gShopPromptTexts[SHOP_PROMPT_PRICE_ASK], shop->amount * CombShop_Price(shop->items[shop->itemCursor])));
        Font_SetPos(418, 402);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}


/* The shop's draw, a menu task's callback: the text size from the task,
 * then by CombMenu.state the top menu's entries, the buy or the sell list,
 * or the combination's window at its slide's x. The talk draws itself. */
void CombShop_Draw(MenuTask *task)
{
    CombMenu *shop = (CombMenu *)gMenuWork;

    Text_SetSize(task->textSize);
    switch (shop->state) {
    case COMB_STATE_TOP:
        Menu_DrawLabelList(460, gItemShopMenuTexts, shop->menuCount, shop->cursor, 0xFFB0B000);
        break;
    case COMB_STATE_BUY:
        Shop_DrawBuy(shop);
        break;
    case COMB_STATE_SELL:
        Shop_DrawSell(shop);
        break;
    case COMB_STATE_COMBINE:
        CombShop_DrawWindow(shop, shop->slideWin.pos, 49);
        break;
    }
}
