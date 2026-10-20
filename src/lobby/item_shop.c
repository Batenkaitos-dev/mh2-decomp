/*
 * The item shop: a townsperson's counter menu (buy, sell, and at some
 * counters the shooting gallery or the townsperson's offers), the buy window
 * over the townsperson's stock and the sell window over the pouch, the item
 * info panel both windows draw under their list, and LobbyPad_Read, the
 * lobby menus' pad read.
 *
 * The menu's work is a ShopMenu in gMenuWork (ItemShop_Open sets it up and
 * Town_UpdateMode runs ItemShop_Update while the menu is open). Its
 * ItemListWin holds one page of SHOP_PAGE_ROWS items with their prices.
 *
 * The file's data starts with the townspeople's stock lists, by the
 * townsperson's id (Shop_BuildStock): a fixed list, a list picked by a
 * village timer, a facility's level or the town event, or the first list of
 * a (condition, list) table whose condition holds (Cond_Check). The rest of
 * its data is the lobby's label tables (lobby_labels.h): each holds one
 * screen's labels or one list, read by enum.
 */
#include "common.h"
#include "m2c_macros.h"
#include "pad.h"
#include "save.h"
#include "item.h"
#include "player.h"
#include "sprite.h"
#include "menu_task.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "lobby_text.h"
#include "lobby_npc.h"
#include "quest.h"


void Shop_BuildSellList(ItemListWin *list, s16 page);
s32 WinSlide_Step(WinSlide *, s16, s16);
s32 Village_Is3924Kind4(void);
s32 Game_IsOnline(void);

extern u8 gMenuWork[];
u16 LobbyPad_Get(s32 a);
void Chat_SetWindowMode(s8, s8);
void ItemShop_SetRows(u16 pad);
void Shop_StartBuy(ItemListWin *list, u16 kind);
void CamFocus_SetAside(s8);
void LobbySe_Play(s32);
void Shop_StartSell(ItemListWin *list);
s32 Menu_PushNoticeOnce(s32 arg0);
void Gallery_Open(void);
struct Em *Npc_Find(u8 kind);
struct Em;
void OfferMenu_Open(struct Em *t);
void Talk_Say(s32 arg0, s8 arg1);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Shop_BuyStep(u16 pad, ItemListWin *list);
s32 Shop_SellStep(u16 pad, ItemListWin *list);
s32 Talk_Update(void);
void TownMenu_Close(void);

/*
 * The shop counter's frame (TOWN_MODE_ITEM_SHOP), by ShopMenu.state: the
 * menu's lines are set once, then the menu runs. Circle takes a line: buy and
 * sell open their windows and move the camera aside; the gallery is refused
 * while a quest is taken; the offers open the townsperson's offer menu.
 * Cross starts the townsperson's closing talk, whose end closes the menu.
 * The buy and sell windows go back to the menu when they are left.
 */
void ItemShop_Update(void)
{
    ShopMenu *shop = (ShopMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);
    s32 state;

    Chat_SetWindowMode(1, 2);
    state = shop->state;
    switch (state) {
    case SHOP_MENU_OPEN:
        shop->state = state + 1;
        ItemShop_SetRows(pad);
        break;
    case SHOP_MENU_CHOOSE:
        if (pad & PAD_CIRCLE) {
            switch (shop->items[shop->cur]) {
            case SHOP_LINE_BUY:
                shop->state = SHOP_MENU_BUY;
                Shop_StartBuy(&shop->list, shop->npc);
                CamFocus_SetAside(1);
                LobbySe_Play(LOBBY_SE_WINDOW_OUT);
                break;
            case SHOP_LINE_SELL:
                shop->state = SHOP_MENU_SELL;
                Shop_StartSell(&shop->list);
                CamFocus_SetAside(1);
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            case SHOP_LINE_GALLERY:
                if (gLobby->questTaken != 0) {
                    Menu_PushNoticeOnce(LOBBY_NOTICE_QUEST_TAKEN);
                    break;
                }
                CamFocus_SetAside(1);
                Gallery_Open();
                break;
            case SHOP_LINE_OFFERS:
                OfferMenu_Open((struct Em *) (Npc_Find(shop->npc)));
                LobbySe_Play(LOBBY_SE_CONFIRM);
                break;
            }
        } else if (pad & PAD_CROSS) {
            shop->state = SHOP_MENU_TALK_END;
            Talk_Say(1, 0);
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else {
            shop->cur = Menu_WrapUpDown(shop->cur, shop->n);
        }
        break;
    case SHOP_MENU_BUY:
        switch (Shop_BuyStep(pad, &shop->list)) {
        case MENU_DONE:
        case MENU_CANCEL:
            shop->state = SHOP_MENU_CHOOSE;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case SHOP_MENU_SELL:
        switch (Shop_SellStep(pad, &shop->list)) {
        case MENU_DONE:
        case MENU_CANCEL:
            shop->state = SHOP_MENU_CHOOSE;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case SHOP_MENU_TALK_END:
        switch (Talk_Update()) {
        case MENU_DONE:
        case MENU_CANCEL:
            TownMenu_Close();
            break;
        }
        break;
    }
}

s32 Item_GetSellPrice(s32 arg0);
s32 Menu_CountPages(s16 n, s16 per);
void *memset(void *, s32, u32);

/* Fills the sell window with page `page` of the pouch: each slot's item and
 * the price one sells for. The cursor stays on the page. */
void Shop_BuildSellList(ItemListWin *list, s16 page)
{
    s32 base = page * SHOP_PAGE_ROWS;
    s32 i;
    s32 idx;
    u16 item;

    list->n = 0;
    memset(list->ent, 0, sizeof(list->ent));
    for (i = 0; i < SHOP_PAGE_ROWS; i++) {
        idx = base + i;
        if (idx < POUCH_SLOTS) {
            item = gSave.pouch[idx].item;
            list->ent[i].id = item;
            list->ent[i].price = Item_GetSellPrice(item);
            list->n++;
        }
    }
    if (list->cur >= list->n) {
        list->cur = list->n - 1;
    }
    list->page = page;
    list->pages = Menu_CountPages(POUCH_SLOTS, SHOP_PAGE_ROWS);
}

/* Opens the sell window on the pouch's first page and starts its slide. */
void Shop_StartSell(ItemListWin *list)
{
    list->page = 0;
    list->infoMode = 0;
    list->cur = 0;
    list->color = 0xBDB59F6C;
    Shop_BuildSellList(list, 0);
    WinSlide_Step(&list->win, 0x280, 0x10E);
}

void LobbySe_Play(s32);
void Shop_Sell(ItemListWin *list);
s32 Shop_SellAmountInput(u16 pad, ItemListWin *list);
s32 Item_IsDeco(u16 id);
s32 Shop_SellListInput(u16 pad, ItemListWin *list);
s32 Menu_YesNoInput(void);
void Menu_SetYesNo(s8 arg0);
extern u8 gMenuInputLock;

/*
 * The sell window's frame, by ItemListWin.state: the slide, the list, the
 * count, then the price's yes/no question. At the question square shows or
 * hides the item's details and R1 turns a decoration's detail pages; yes
 * sells and rebuilds the page. Returns MENU_CANCEL when the list is left,
 * else MENU_RUNNING.
 */
s32 Shop_SellStep(u16 pad, ItemListWin *list)
{
    switch (list->state) {
    case SHOP_STEP_SLIDE_IN:
        switch (WinSlide_Step(&list->win, 0x280, 0x10E)) {
        case MENU_DONE:
            list->state++;
            break;
        }
        break;
    case SHOP_STEP_LIST:
        switch (Shop_SellListInput(pad, list)) {
        case MENU_DONE:
            list->state++;
            list->count = 1;
            break;
        case MENU_CANCEL:
            list->state = SHOP_STEP_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case SHOP_STEP_COUNT:
        switch (Shop_SellAmountInput(pad, list)) {
        case MENU_DONE:
            list->state++;
            Menu_SetYesNo(0);
            gMenuInputLock = 9; /* frames without input */
            break;
        case MENU_CANCEL:
            list->state = SHOP_STEP_LIST;
            break;
        }
        break;
    case SHOP_STEP_CONFIRM:
        if (pad & PAD_SQUARE) {
            if (list->ent[list->cur].id != 0) {
                list->infoMode ^= 1;
                LobbySe_Play(LOBBY_SE_DESCRIPTION);
            }
        } else if (pad & PAD_R1) {
            if (list->ent[list->cur].id != 0 && list->infoMode != 0 && Item_IsDeco(list->ent[list->cur].id) != 0) {
                if (++list->infoPage >= 3) {
                    list->infoPage = 0;
                }
                LobbySe_Play(LOBBY_SE_CONFIRM);
            }
        } else {
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                list->state = SHOP_STEP_LIST;
                Shop_Sell(list);
                Shop_BuildSellList(list, list->page);
                break;
            case MENU_CANCEL:
                list->state = SHOP_STEP_LIST;
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

u16 SavePouch_Add(u16, s16); /* unit's own declaration: the definition returns u8; declared here returning u16, as Shop_Sell compares the whole result with 4 */
s32 SavePouch_ClearSlot(s16);
void Save_AddMoney(s32 n);

/* Sells list->count of the item under the cursor: they leave the pouch (4
 * from SavePouch_Add: the slot is empty now), or an unlimited item's slot is
 * cleared; then the money sound, and the price times the count is added. */
void Shop_Sell(ItemListWin *list)
{
    s16 cur = list->cur;
    u16 price = list->ent[cur].price;
    u16 id = list->ent[cur].id;
    s32 count = list->count;
    s32 money = price * count;

    if (gItemData[id].max != ITEM_MAX_UNLIMITED) {
        if (SavePouch_Add(id, -count) == 4) {
            list->ent[list->cur].id = 0;
        }
    } else {
        SavePouch_ClearSlot(cur + list->page * SHOP_PAGE_ROWS);
    }
    LobbySe_Play(LOBBY_SE_MONEY);
    Save_AddMoney(money);
}

/*
 * The sell window's count: circle takes it (MENU_DONE), cross goes back to
 * the list (MENU_CANCEL). Up and down change it by one, left sets 1 and
 * right the most the pouch holds (1 for an unlimited item); square and R1
 * work as in the list. Returns MENU_RUNNING otherwise.
 */
s32 Shop_SellAmountInput(u16 pad, ItemListWin *list)
{
    s32 SavePouch_Count(u16 item);
    u16 id = list->ent[list->cur].id;
    s16 max = SavePouch_Count(id);

    if (gItemData[id].max == ITEM_MAX_UNLIMITED) {
        max = 1;
    }
    if (pad & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_SQUARE) {
        if (list->ent[list->cur].id != 0) {
            list->infoMode ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & PAD_R1) {
        if (list->ent[list->cur].id != 0 && list->infoMode != 0 && Item_IsDeco(list->ent[list->cur].id) != 0) {
            if (++list->infoPage >= 3) {
                list->infoPage = 0;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else if (pad & PAD_UP) {
        if (list->count < max) {
            list->count++;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_DOWN) {
        if (list->count > 1) {
            list->count--;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_LEFT) {
        if (list->count > 1) {
            list->count = 1;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_RIGHT) {
        if (list->count < max) {
            list->count = max;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    }
    return MENU_RUNNING;
}

/* True when the item is a decoration (its details have pages). */
s32 Item_IsDeco(u16 id)
{
    return gItemData[id].deco != 0;
}

void LobbySe_Play(s32);
s32 Menu_WrapUpDown(s32 arg0, s32 arg1);
s32 Menu_WrapLeftRight(s32 arg0, s32 arg1);

/*
 * The sell window's list: circle takes the item under the cursor
 * (MENU_DONE; an empty slot is refused), cross leaves (MENU_CANCEL), square
 * shows or hides the details, R1 turns a decoration's detail pages, left and
 * right turn the pouch's pages, up and down move the cursor. Returns
 * MENU_RUNNING otherwise.
 */
s32 Shop_SellListInput(u16 pad, ItemListWin *list)
{
    if (pad & PAD_CIRCLE) {
        if (list->ent[list->cur].id != 0) {
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        LobbySe_Play(LOBBY_SE_REFUSE);
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_SQUARE) {
        if (list->ent[list->cur].id != 0) {
            list->infoMode ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & PAD_R1) {
        if (list->ent[list->cur].id != 0 && list->infoMode != 0 && Item_IsDeco(list->ent[list->cur].id) != 0) {
            if (++list->infoPage >= 3) {
                list->infoPage = 0;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        list->page = Menu_WrapLeftRight(list->page, list->pages);
        Shop_BuildSellList(list, list->page);
    } else {
        list->cur = Menu_WrapUpDown(list->cur, list->n);
    }
    return MENU_RUNNING;
}

struct Em *Npc_Find(u8 kind);
s16 OfferMenu_BuildList(struct Em *t, s32 b);
extern u8 gMenuWork[];

/* Sets the shop menu's lines: buy and sell, then the shooting gallery at
 * NPC_GALLERY_KEEPER's counter, or at another counter the townsperson's
 * offers when it has any (with the label list that names them). `pad` isn't
 * read. */
void ItemShop_SetRows(u16 pad)
{
    ShopMenu *shop = (ShopMenu *)gMenuWork;

    shop->lines = &gShopMenuTexts[SHOP_MENU_TEXT_BUY];
    shop->items[0] = SHOP_LINE_BUY;
    shop->items[1] = SHOP_LINE_SELL;
    shop->n = 2;
    shop->color = 0xBDB59F6C;
    switch (shop->npc) {
    case NPC_GALLERY_KEEPER:
        shop->n = 3;
        shop->items[2] = SHOP_LINE_GALLERY;
        break;
    default:
        if (OfferMenu_BuildList((struct Em *) (Npc_Find(shop->npc)), 0) != 0) {
            shop->n = 3;
            shop->items[2] = SHOP_LINE_OFFERS;
            shop->lines = gShopBuySellTexts;
        }
        break;
    }
}

s32 PspLink_Count(void);
s32 Save_TestEventFlag(s32 flag);
/* unit's own declaration: the definition returns s16; declared here returning s32, as the stock pickers test it (with s16 their calls compile differently) */
s32 Village_MilestoneState(s32);
extern u16 *gShopStockNpc12Psp[];
extern u16 *gShopStockNpc12Timer[];

/*
 * NPC_NAMING_TALKER's stock. While village timer 3 runs: the second of its
 * timer lists when PSP links were made, else the first. Otherwise one of
 * its four PSP lists: the first with no links, then by the link count mod 3.
 * The links count only once EVENT_FLAG_PSP_LINK_6 is set.
 */
u16 *Shop_PickStockList(void)
{
    s32 n;
    s32 idx;
    u16 *ret;

    n = PspLink_Count();
    idx = 0;
    if (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_6) == 0) {
        n = 0;
    }
    if (Village_MilestoneState(3) != 0) {
        if (n != 0) {
            ret = gShopStockNpc12Timer[1];
        } else {
            ret = gShopStockNpc12Timer[0];
        }
        return ret;
    }
    if (n != 0) {
        n %= 3;
        idx = n + 1;
    }
    return gShopStockNpc12Psp[idx];
}

/* Of a pair of stock lists, the second while village timer `timer` runs
 * offline, else the first. */
u16 *Shop_PickByTimer(u16 **lists, s32 timer)
{
    if (Game_IsOnline() == 0 && Village_MilestoneState(timer) != 0) {
        lists++;
    }
    return *lists;
}

u8 Village_FacilityLevel(s32 sub);
s32 Shop_GetBuyPrice(s32 arg0);
s32 Cond_Check(u16 cond);
extern ShopStockCond gShopCondStockNpc07[];
extern ShopStockCond gShopCondStockNpc08[];
extern u16 *gShopStockNpc0A[];
extern u16 gShopStockNpc0B[];
extern u16 gShopStockNpc0BLv2[];
extern u16 gShopStockNpc0BTimer[];
extern u16 gShopStockNpc0C[];
extern u16 gShopStockNpc0CLv2[];
extern ShopStockCond gShopCondStockNpc28[];
extern u16 *gShopStockNpc2C[];
extern u16 *gShopStockNpc2D[];
extern u16 *gShopStockNpc2E[];
extern ShopStockCond gShopCondStockNpc39[];
extern ShopStockCond gShopCondStockNpc42[];
extern u16 gShopStockNpc50[];
extern u16 gShopStockNpc50Ev122[];
extern ShopStockCond gShopCondStockNpc5D[];
extern ShopStockCond gShopCondStockNpc6A[];

/*
 * Fills the buy window with page `page` (SHOP_PAGE_ROWS items) of the stock
 * of townsperson list->kind, with each item's buy price. The stock is a
 * fixed list; a list picked by a village timer, the port's or the smithy's
 * level, the town event or an event flag; or the first list of a
 * (condition, list) table whose condition holds. The stock lists end with
 * item 0.
 */
void Shop_BuildStock(ItemListWin *list, s16 page)
{
    u16 *items = NULL;
    s32 first = page * SHOP_PAGE_ROWS;
    s32 last = first + 6;
    s32 i;
    ShopStockCond *condList;
    s32 k;
    u16 id;

    list->n = 0;
    list->page = page;
    memset(list->ent, 0, sizeof(list->ent));
    condList = NULL;
    switch (list->kind) {
    case 7:
        condList = gShopCondStockNpc07;
        break;
    case 8:
        condList = gShopCondStockNpc08;
        break;
    case 10:
        items = Shop_PickByTimer(gShopStockNpc0A, 0);
        break;
    case 11:
        if (Village_MilestoneState(0) != 0) {
            items = gShopStockNpc0BTimer;
        } else if (Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2) {
            items = gShopStockNpc0BLv2;
        } else {
            items = gShopStockNpc0B;
        }
        break;
    case NPC_NAMING_TALKER:
        items = Shop_PickStockList();
        break;
    case 0x28:
        condList = gShopCondStockNpc28;
        break;
    case 0x2C:
        if (gLobby->townEvent == 1) {
            items = gShopStockNpc2C[1];
        } else {
            items = gShopStockNpc2C[0];
        }
        break;
    case 0x2D:
        if (gLobby->townEvent == 3) {
            items = gShopStockNpc2D[1];
        } else {
            items = gShopStockNpc2D[0];
        }
        break;
    case 0x2E:
        if (gLobby->townEvent == 2) {
            items = gShopStockNpc2E[1];
        } else {
            items = gShopStockNpc2E[0];
        }
        break;
    case NPC_GALLERY_KEEPER:
        condList = gShopCondStockNpc39;
        break;
    case 0x42:
        condList = gShopCondStockNpc42;
        break;
    case 0x50:
        if (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 1) {
            items = gShopStockNpc50Ev122;
        } else {
            items = gShopStockNpc50;
        }
        break;
    case 0x5D:
        condList = gShopCondStockNpc5D;
        break;
    case 0x6A:
        condList = gShopCondStockNpc6A;
        break;
    case 0xC:
    case 0xD:
        if (Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) == 2) {
            items = gShopStockNpc0CLv2;
        } else {
            items = gShopStockNpc0C;
        }
        break;
    default:
        condList = NULL; /* already NULL: the game still jumps through this block */
        break;
    }
    for (; condList != NULL; condList++) {
        if (Cond_Check(condList->cond) == 1) {
            items = condList->items;
            break;
        }
    }
    if (items != NULL) {
        k = 0;
        for (i = 0; (id = *items) != 0; items++, i++) {
            if (i >= first && i <= last) {
                list->ent[k].id = id;
                list->ent[k].price = Shop_GetBuyPrice(id);
                k++;
                list->n++;
            }
        }
    }
    if (list->cur >= list->n) {
        list->cur = list->n - 1;
    }
    /* the game reads a register it never set: i is s1, still the caller's, when the kind has no stock list (items NULL) */
    list->pages = Menu_CountPages(i, SHOP_PAGE_ROWS);
}

s16 SavePouch_HasFreeSlot(void);
void Shop_BuildStock(ItemListWin *list, s16 page);

/* Opens the buy window on the first page of townsperson `kind`'s stock:
 * starts its slide, notes whether the pouch has a free slot, and picks the
 * picture behind the list by the townsperson. */
void Shop_StartBuy(ItemListWin *list, u16 kind)
{
    list->page = 0;
    list->color = 0xBDB59F6C;
    list->infoMode = 0;
    list->cur = 0;
    list->kind = kind;
    Shop_BuildStock(list, 0);
    WinSlide_Step(&list->win, 0x280, 0x10E);
    list->pouchHasRoom = SavePouch_HasFreeSlot();
    switch (list->kind) {
    case 0xD:
    case 0xC:
        list->backdrop = 0;
        break;
    case 7:
    case NPC_GALLERY_KEEPER:
    case 0x42:
    case 0x50:
    case 0x5D:
        list->backdrop = 2;
        break;
    case 0xB:
    case 0x2D:
        list->backdrop = 3;
        break;
    case 8:
    case 0x2C:
        list->backdrop = 4;
        break;
    default:
        list->backdrop = 1;
        break;
    }
}

void Shop_Buy(ItemListWin *list);
s32 Shop_BuyAmountInput(u16 pad, ItemListWin *list);
s32 Shop_BuyListInput(u16 pad, ItemListWin *list);
s32 Menu_YesNoInput(void);
void Menu_SetYesNo(s8 arg0);
extern u8 gMenuInputLock;

/*
 * The buy window's frame, by ItemListWin.state: the slide, the list, the
 * count, then the price's yes/no question (square shows or hides the item's
 * details there); yes buys. Returns MENU_CANCEL when the list is left, else
 * MENU_RUNNING.
 */
s32 Shop_BuyStep(u16 pad, ItemListWin *list)
{
    switch (list->state) {
    case SHOP_STEP_SLIDE_IN:
        switch (WinSlide_Step(&list->win, 0x280, 0x10E)) {
        case MENU_DONE:
            list->state++;
            break;
        }
        break;
    case SHOP_STEP_LIST:
        switch (Shop_BuyListInput(pad, list)) {
        case MENU_DONE:
            list->state++;
            list->count = 1;
            break;
        case MENU_CANCEL:
            list->state = SHOP_STEP_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case SHOP_STEP_COUNT:
        switch (Shop_BuyAmountInput(pad, list)) {
        case MENU_DONE:
            list->state++;
            Menu_SetYesNo(0);
            gMenuInputLock = 9; /* frames without input */
            break;
        case MENU_CANCEL:
            list->state = SHOP_STEP_LIST;
            break;
        }
        break;
    case SHOP_STEP_CONFIRM:
        if (pad & PAD_SQUARE) {
            list->infoMode ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        } else {
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                list->state = SHOP_STEP_LIST;
                Shop_Buy(list);
                break;
            case MENU_CANCEL:
                list->state = SHOP_STEP_LIST;
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

static s32 Shop_IsUnlockItem(s32 x);
static s32 Shop_IsOnceOnlyItem(u16 arg0);
u16 SavePouch_Add(u16, s16); /* unit's own declaration: the definition returns u8; declared here returning u16, as Shop_Sell compares the whole result with 4 */
void Save_AddMoney(s32 n);
void Save_SetItemObtained(SaveData *save, u16 item);
u8 Village_FacilityLevel(s32 sub);
void Village_SetFacilityLevel(s32 sub, u8 lvl);
s32 Shop_GetBuyPrice(s32 arg0);
s32 Menu_PushNoticeOnce(s32 arg0);
s32 HunterCard_SendChanged(void);

/*
 * Buys list->count of the item under the cursor: the money sound, the price
 * times the count is paid. An upgrade item is kept as obtained and does its
 * work (the box's two show their notice, the house's two raise the house's
 * level), and online the hunter card is sent again; a once-only item is kept
 * as obtained; any other goes into the pouch. Then the pouch's free slot is
 * checked again.
 */
void Shop_Buy(ItemListWin *list)
{
    s32 cost;
    u16 id = list->ent[list->cur].id;

    cost = Shop_GetBuyPrice(id);
    cost *= list->count;

    LobbySe_Play(LOBBY_SE_MONEY);
    Save_AddMoney(-cost);
    if (Shop_IsUnlockItem(id) == 1) {
        Save_SetItemObtained(&gSave, id);
        switch (id) {
        case ITEM_ID_BOX_UPGRADE_1:
            Menu_PushNoticeOnce(LOBBY_NOTICE_BOX_UPGRADE_1);
            break;
        case ITEM_ID_BOX_UPGRADE_2:
            Menu_PushNoticeOnce(LOBBY_NOTICE_BOX_UPGRADE_2);
            break;
        case ITEM_ID_HOUSE_UPGRADE_1:
            Village_SetFacilityLevel(VILLAGE_FACILITY_HOUSE, Village_FacilityLevel(VILLAGE_FACILITY_HOUSE) + 1);
            if (Village_FacilityLevel(VILLAGE_FACILITY_HOUSE) == 2) {
                Menu_PushNoticeOnce(LOBBY_NOTICE_HOUSE_TOP);
            } else {
                Menu_PushNoticeOnce(LOBBY_NOTICE_HOUSE_UPGRADED);
            }
            break;
        case ITEM_ID_HOUSE_UPGRADE_2:
            Village_SetFacilityLevel(VILLAGE_FACILITY_HOUSE, Village_FacilityLevel(VILLAGE_FACILITY_HOUSE) + 1);
            if (Village_FacilityLevel(VILLAGE_FACILITY_HOUSE) == 2) {
                Menu_PushNoticeOnce(LOBBY_NOTICE_HOUSE_TOP);
            } else {
                Menu_PushNoticeOnce(LOBBY_NOTICE_HOUSE_UPGRADED);
            }
            break;
        }
        if (Game_IsOnline() == 1) {
            HunterCard_SendChanged();
        }
    } else if (Shop_IsOnceOnlyItem(id) == 1) {
        Save_SetItemObtained(&gSave, id);
    } else {
        SavePouch_Add(id, list->count);
    }
    list->pouchHasRoom = SavePouch_HasFreeSlot();
}

s32 Shop_CanBuy(u16 item, s16 n);

/*
 * The buy window's count: circle takes it (MENU_DONE), cross goes back to
 * the list (MENU_CANCEL), square shows or hides the details. Up adds one
 * while Shop_CanBuy allows it, down takes one off, left sets 1 and right the
 * most that can be bought (at most 100 tried). Returns MENU_RUNNING
 * otherwise.
 */
s32 Shop_BuyAmountInput(u16 pad, ItemListWin *list)
{
    s32 count;
    s16 before;
    u16 id = list->ent[list->cur].id;

    if (pad & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_SQUARE) {
        list->infoMode ^= 1;
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
    } else if (pad & PAD_UP) {
        if (Shop_CanBuy(id, list->count + 1) == 1) {
            list->count++;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_DOWN) {
        if (list->count > 1) {
            list->count--;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_LEFT) {
        if (list->count > 1) {
            list->count = 1;
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_RIGHT) {
        count = list->count;
        before = count;
        for (; count <= 100; count++) {
            if (Shop_CanBuy(id, count) == 0) {
                list->count = count - 1;
                break;
            }
        }
        if (list->count != before) {
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    }
    return MENU_RUNNING;
}

u8 Equip_GetClass(EquipItem *e);
void Shop_BuildStock(ItemListWin *list, s16 page);
s32 Shop_CanBuy(u16 item, s16 n);
s32 Menu_PushNoticeOnce(s32 arg0);

/*
 * The buy window's list: circle takes the item under the cursor when one
 * can be bought (MENU_DONE), with a notice when it doesn't suit the weapon
 * (ammo without a bowgun, a coating without a bow, ITEM_ID_SHARPEN_150 with
 * a bowgun); else the refusal sound. Cross leaves (MENU_CANCEL), square
 * shows or hides the details, R1 turns a decoration's detail pages, left and
 * right turn the stock's pages, up and down move the cursor. Returns
 * MENU_RUNNING otherwise.
 */
s32 Shop_BuyListInput(u16 pad, ItemListWin *list)
{
    u16 id;
    u8 weaponClass = Equip_GetClass((EquipItem *) gSave.equip);
    u8 type;

    if (pad & PAD_CIRCLE) {
        id = list->ent[list->cur].id;
        if (Shop_CanBuy(id, 1) == 1) {
            type = gItemData[id].type;
            if (type == ITEM_TYPE_COATING && weaponClass != WEAPON_BOW) {
                Menu_PushNoticeOnce(LOBBY_NOTICE_BOW_ONLY);
            } else if (type == ITEM_TYPE_AMMO && weaponClass != WEAPON_LIGHT_BOWGUN && weaponClass != WEAPON_HEAVY_BOWGUN) {
                Menu_PushNoticeOnce(LOBBY_NOTICE_GUNNER_ONLY);
            } else if (id == ITEM_ID_SHARPEN_150 && gSave.equip[SAVE_EQUIP_WEAPON].type != EQUIP_BLADE) {
                Menu_PushNoticeOnce(LOBBY_NOTICE_BLADE_ONLY);
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        LobbySe_Play(LOBBY_SE_REFUSE);
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_SQUARE) {
        list->infoMode ^= 1;
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
    } else if (pad & PAD_R1) {
        if (list->infoMode != 0 && Item_IsDeco(list->ent[list->cur].id) != 0) {
            if (++list->infoPage >= 3) {
                list->infoPage = 0;
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (list->pages > 1) {
            list->page = Menu_WrapLeftRight(list->page, list->pages);
            Shop_BuildStock(list, list->page);
        }
    } else {
        list->cur = Menu_WrapUpDown(list->cur, list->n);
    }
    return MENU_RUNNING;
}

/* True for the upgrade items: the box's two and the house's two. They are
 * kept as obtained, never in the pouch, and are bought once. */
static s32 Shop_IsUnlockItem(s32 x)
{
    switch (x) {
    case ITEM_ID_BOX_UPGRADE_1:
    case ITEM_ID_BOX_UPGRADE_2:
    case ITEM_ID_HOUSE_UPGRADE_1:
    case ITEM_ID_HOUSE_UPGRADE_2:
        return 1;
    }
    return 0;
}

/* True for items 0x2C1 and 0x2C2, which are bought once and kept as
 * obtained rather than in the pouch. */
static s32 Shop_IsOnceOnlyItem(u16 item)
{
    switch (item & 0xFFFF) {
    case 0x2C1:
    case 0x2C2:
        return 1;
    }
    return 0;
}

/* An item's buy price; online it is halved while the town event is 4
 * (Village_Is3924Kind4). */
s32 Shop_GetBuyPrice(s32 item)
{
    s32 ret;

    ret = gItemData[item].price;
    if ((Game_IsOnline() == 1) && (Village_Is3924Kind4() == 1)) {
        ret = ret >> 1;
    }
    return ret;
}

/* What one of the item sells for. */
s32 Item_GetSellPrice(s32 item)
{
    return gItemData[item].value;
}

s32 SavePouch_Space(u16 item);
s32 Save_IsItemObtained(SaveData *save, u16 item);
s32 Item_IsRecordOnly(u16 item);

/*
 * True when n of the item can be bought: a record-only item, an upgrade
 * item or a once-only one only one at a time and only when not obtained
 * yet; an unlimited item only one, when the pouch holds none and has room;
 * any other when the pouch has room for n. Then the money must cover the
 * price times n.
 */
s32 Shop_CanBuy(u16 item, s16 n)
{
    s32 SavePouch_Count(u16 item);
    s16 space;
    s32 price;
    u32 money;

    if (Item_IsRecordOnly(item) == 1) {
        if (n > 1) {
            return 0;
        }
        if (Save_IsItemObtained(&gSave, item) == 1) {
            return 0;
        }
    } else if (Shop_IsUnlockItem(item) == 1 || Shop_IsOnceOnlyItem(item) == 1) {
        if (Save_IsItemObtained(&gSave, item) == 1 || n > 1) {
            return 0;
        }
    } else if (gItemData[item].max == ITEM_MAX_UNLIMITED) {
        if (n > 1 || SavePouch_Count(item) != 0 || SavePouch_Space(item) == -1) {
            return 0;
        }
    } else {
        space = SavePouch_Space(item);
        if (space == -1) {
            return 0;
        }
        if (space < n) {
            return 0;
        }
    }
    price = Shop_GetBuyPrice(item);
    money = gSave.money;
    return (s32)money >= price * n;
}

void Font_SetPos(s16 x, s16 y);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Sjis_Widen(char *, char *);
s32 sprintf(char *buf, char *fmt, ...);
/* unit's own declaration: the definition returns s8; declared returning s32 as this file's other declaration of it is */
s32 Kbd_IsOpen(void);
s16 Lobby_DrawButtonIcon(s16 x, s16 y, s32 i);
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
struct DecoData *Item_GetDecoration(u16 item);
s32 ItemBox_Total(u16 item);
s32 ItemBox_Room(u16 item);
void Deco_DrawDetails(u16 item, s16 x, s16 y, s8 attr, u8 dim);
extern char *gItemNames[];
extern s32 gTownMode;

/*
 * The item info panel at x, y, w wide, in `color`: the details label with
 * the square button's icon (pressed down while square is held), the item's
 * icon and rarity, then by `mode` (ItemListWin.infoMode):
 * - 0: the item's name, the slots a decoration needs, and how many the pouch
 *   and the box hold (`extra` added to the pouch's count): obtained or not
 *   for an upgrade, record-only or once-only item, the infinity mark for an
 *   unlimited one held; the pouch's count is highlighted at its limit, and
 *   the box's when the box has no room;
 * - 1: the item's details (Deco_DrawDetails, page `attr`; dimmed unless the
 *   colour is the sell window's), marked as outside the workshop unless
 *   TOWN_MODE_WORKSHOP.
 * The item's records are read through a byte offset (item * 0x18) the game
 * computes once and reuses for the rarity and the limit.
 * SavePouch_Count is called with no argument: in the game's code no
 * prototype was in scope, so the call passes nothing and the function reads
 * the item that Shop_IsOnceOnlyItem's call left in the argument register.
 */
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 mode, s32 item, u16 extra, s8 attr, u32 color)
{
    char num[16];
    char wide[16];
    s16 pressOffset;
    s32 total;
    s32 left;
    s16 labelY;
    s32 right;
    s32 have;
    DecoData *deco;
    s32 itemOffset;
    s32 countY;

    left = x;
    pressOffset = 0;
    total = 0;
    labelY = y + 0x44;
    Font_SetSpacing(18, 18);
    right = (s16)(left + w);
    Font_SetPos(right - 0x3C, labelY);
    Font_SetColor(color);
    Text_PrintfColored(gGameMenuLabels[MENU_LABEL_DETAILS]);
    if (Kbd_IsOpen() == 0 && (gPadState[0].pressed & PAD_SQUARE)) {
        pressOffset = 2;
    }
    Lobby_DrawButtonIcon((s16)(right - 0x3C) - 0x1A, labelY - 2 + pressOffset, 3);
    Font_SetSpacing(18, 18);
    y += 8;
    Item_DrawIcon(left + 10, y, 0x36, item);
    itemOffset = item * 0x18;
    Item_DrawRarity((s16)(left + 10) + 8, y + 0x36, (&gItemData[0].rarity)[itemOffset]);
    if (mode == 0) {
        Font_SetPos(left + 0x50, y);
        Font_SetColor(color);
        Text_PrintfColored("%s", gItemNames[item]);
        if (Item_IsDeco(item) == 1) {
            deco = Item_GetDecoration(item);
            Font_SetPos(left + 0x50, y += 0x17);
            sprintf(num, "%d", deco->size);
            Sjis_Widen(num, wide);
            Text_PrintfColored("%s%s", gTownLabels[TOWN_LABEL_NEEDED_SLOTS], wide);
        }
        Font_SetPos(left + 0x50, y += 0x17);
        Text_PrintfColored("%s%s", gCountLabels[COUNT_LABEL_HELD], gLobbyWords[LOBBY_WORD_TIMES]);
        Font_SetSpacing(0x1C, 0x14);
        if (Shop_IsUnlockItem(item) == 1) {
            if (Save_IsItemObtained(&gSave, item) == 1) {
                have = 1;
            } else {
                have = 0;
            }
        } else if (Item_IsRecordOnly(item) == 1 || Shop_IsOnceOnlyItem(item) == 1) {
            if (Save_IsItemObtained(&gSave, item) == 1) {
                have = 1;
            } else {
                have = 0;
            }
        } else {
            have = SavePouch_Count();
            total = ItemBox_Total(item);
            if (total > 999) {
                total = 999;
            }
        }
        if (extra > 0) {
            have += extra;
        }
        if ((&gItemData[0].max)[itemOffset] == ITEM_MAX_UNLIMITED) {
            Font_SetColor(color);
            countY = y;
            Font_SetPos(left + 0x98, countY - 1);
            if (have == 0) {
                Text_PrintfColored("%2d", have);
            } else {
                Text_Printf("%s", gLobbyWords[LOBBY_WORD_INFINITY]);
            }
            Font_SetColor(color);
            Font_SetPos((s16)(left + 0x98) + 0x1C, countY - 1);
            if (total == 0) {
                Text_PrintfColored("(%3d)", total);
            } else {
                s16 countX = left + 0x98;

                Font_SetPos(countX + 0x1C, countY - 1);
                Text_Printf("(", gLobbyWords[LOBBY_WORD_INFINITY]);
                Font_SetPos(countX + 0x23, countY - 1);
                Text_Printf(" %s", gLobbyWords[LOBBY_WORD_INFINITY]);
                Font_SetColor(color);
                Font_SetPos(countX + 0x1C, countY - 1);
                Text_Printf("    )");
            }
        } else {
            countY = y;
            Font_SetPos(left + 0x98, countY - 1);
            if (have >= (&gItemData[0].max)[itemOffset]) {
                Text_Printf("~C02%2d", have);
                Font_SetPos(left + 0x98, countY - 1);
                Font_SetColor(color);
                Text_PrintfColored("  (%3d)", total);
                if (ItemBox_Room(item) == 0) {
                    Font_SetPos(left + 0x98, countY - 1);
                    Font_SetColor(0x807F212E);
                    Text_PrintfColored("   %3d", total);
                }
            } else {
                Font_SetColor(color);
                Text_PrintfColored("%2d(%3d)", have, total);
                if (ItemBox_Room(item) == 0 && ItemBox_Total(item) != 0) {
                    Font_SetPos(left + 0x98, countY - 1);
                    Text_Printf("   ~C02%3d", total);
                }
            }
        }
    } else if (color == 0x807F7F7F) {
        if (gTownMode == TOWN_MODE_WORKSHOP) {
            Deco_DrawDetails(item, left + 0x50, y, attr, 0);
        } else {
            Deco_DrawDetails(item, left + 0x50, y, attr | 0x10, 0);
        }
    } else {
        if (gTownMode == TOWN_MODE_WORKSHOP) {
            Deco_DrawDetails(item, left + 0x50, y, attr, 1);
        } else {
            Deco_DrawDetails(item, left + 0x50, y, attr | 0x10, 1);
        }
    }
}

s16 Rarity_Color(u8 i);
s32 sprintf(char *buf, char *fmt, ...);
void Font_SetPos(s16 x, s16 y);
void Font_SetSpacing(u8 a, u8 b);
void Text_DrawShadow(s16 x, s16 y, s32 c1, s32 c2, char *s); /* unit's own declaration: the definition takes s16 colours; declared with s32, as Item_DrawRarity passes the colour without narrowing it */

/* The rarity's label and number (rarity + 1) at x, y, with a shadow, in the
 * rarity's colour. */
void Item_DrawRarity(s16 x, s16 y, u8 rarity)
{
    char buf[32];
    s16 color;

    Font_SetPos(x, y);
    Font_SetSpacing(18, 18);
    color = Rarity_Color(rarity);
    sprintf(buf, "%s%d", gLobbyWords[LOBBY_WORD_RARE], rarity + 1);
    Text_DrawShadow(x, y, 1, color, buf);
}

s32 Sprite_Draw(struct Sprite *s);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32);
extern u32 gItemColors[];

void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item);

/*
 * The item's icon at x, y, `size` high and 1.25 times as wide: its cell of
 * the system icons' texture (0x144; eight 32-pixel cells a row, cell
 * icon + 1) in the item's colour, then the overlay mark of an item with bit
 * 0 of flags2. The item's record is read through a byte offset, and
 * rect.x is stored before icon + 1 as a statement of its own: the game's
 * code orders them so.
 */
void Item_DrawIcon(s16 x, s16 y, s16 size, s32 item)
{
    s32 itemOffset;
    Sprite rect;
    s16 *right;
    s16 *bottom;
    u32 *color;
    s16 *left;
    s16 *top;
    s32 icon;

    itemOffset = item * 0x18;
    icon = ((u8 *)&gItemData[0].icon)[itemOffset];
    rect.x = x;
    icon = icon + 1;
    rect.h = size;
    rect.y = y;
    rect.w = 1.25f * size;
    left = &rect.u0;
    top = &rect.v0;
    right = &rect.u1;
    bottom = &rect.v1;
    color = &rect.color;
    *left = (icon & 7) * 32 + 1;
    *top = (icon >> 3) * 32 + 1;
    *right = (icon & 7) * 32 + 31;
    *bottom = (icon >> 3) * 32 + 31;
    *color = gItemColors[((u8 *)&gItemData[0].color)[itemOffset]];
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    Sprite_Draw(&rect);
    if (((u8 *)&gItemData[0].flags2)[itemOffset] & 1) {
        *left = 0x80;
        *top = 0xE0;
        *right = *left + 0x20;
        *bottom = *top + 0x20;
        *color = -1;
        Sprite_Draw(&rect);
    }
}

void Font_SetColor(s32 color);
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Icon_Draw(s16 x, s16 y, s16 w, s16 h, u32 color, s32 spr);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Menu_DrawCursorRow(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2);
void Menu_DrawDashLine(s16 x, s16 y, s16 w, u32 color);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 a3, s32 item, u16 t1, s8 t2, u32 color);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Sjis_Widen(char *, char *);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
char *Text_FillDollar(char *fmt, s32 n);
extern char *gItemNames[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;

/*
 * The buy window: its title and the money held, the panel with the
 * townsperson's picture (ItemListWin.backdrop), the page number, the page's
 * items with their prices (dark when they can't be bought) and the cursor's
 * row, the item info panel for the item under the cursor, and the prompt of
 * the step: pick an item (or the pouch is full), how many (with the arrows
 * the count can move by), or the price with yes and no. `work` is an
 * ItemListWin: the shop's, or the combine shop's menu work, which starts
 * with one.
 */
void Shop_DrawBuy(void *work)
{
    ItemListWin *list = work;
    s32 price;
    s16 x = list->win.pos;
    u16 id;
    s16 y = 0x31;
    s16 i;
    s32 item;
    char num[16];
    char buf[32];

    y += 10;
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x10E, 0x12, 0xA0, 0xFFB0B000, gShopMenuTexts[SHOP_MENU_TEXT_BUY], 2, 1);
    switch (list->backdrop) {
    case 0:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFFFDDDD, 0);
        Icon_Draw(x + 0x73, 0x66, 0x78, 0xC8, 0x60FFFFFF, 0x24);
        break;
    case 1:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFD3EDDC, 0);
        Icon_Draw(x + 0x64, 0x7F, 0x96, 0x96, 0x60FFFFFF, 0x25);
        break;
    case 3:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFFFFFDD, 0);
        Icon_Draw(x + 0x50, 0x7F, 0xBE, 0x96, 0x60FFFFFF, 0x21);
        break;
    case 4:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFFFFFDD, 0);
        Icon_Draw(x + 0x64, 0x7F, 0x96, 0x96, 0x60FFFFFF, 0x22);
        break;
    default:
        Icon_Draw(x, 0x31, 0x15E, 0x132, 0xFFFFFFDD, 0);
        Icon_Draw(x + 0x64, 0x7F, 0x96, 0x96, 0x60FFFFFF, 0x26);
        break;
    }
    Menu_DrawMoney(0x1E0, 0x10, 0);
    Font_SetColor(0x80000000);
    Menu_DrawPageNum(x + 0xFA, y, list->page, list->pages, 1);
    Font_SetSpacing(20, 20);
    for (i = 0; i < list->n; i++) {
        id = list->ent[i].id;
        price = Shop_GetBuyPrice(id);
        if (Shop_CanBuy(id, 1) == 1) {
            Font_SetColor(0x80101010);
        } else {
            Font_SetColor(0x8042352E);
        }
        Font_SetSpacing(20, 20);
        y += 0x18;
        Font_SetPos(x + 0x2E, y);
        Text_PrintfColored("%s", gItemNames[id]);
        Font_SetSpacing(20, 20);
        Font_SetPos(x + 0xF8, y);
        Text_PrintfColored("% 7dz", price);
        Item_DrawIcon(x + 0x10, y - 1, 0x18, id);
        if (list->cur == i) {
            Menu_DrawCursorRow(x + 0x2E, y, 0xFA, 0x32, 0xC0106060, 0xFF106060);
        }
    }
    id = list->ent[list->cur].id;
    Menu_DrawDashLine(x + 0x14, 0xFB, 0x136, 0xFF000000);
    if (Item_IsDeco(id) != 0) {
        ItemInfo_Draw(x, 0xFF, 0x15E, list->infoMode, item = id, 0, list->infoPage, 0x80101010);
    } else {
        ItemInfo_Draw(x, 0xFF, 0x15E, list->infoMode, item = id, 0, -1, 0x80101010);
    }
    Font_SetSpacing(18, 18);
    switch (list->state) {
    case SHOP_STEP_SLIDE_IN:
    case SHOP_STEP_LIST:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010,
                      list->pouchHasRoom == 0 ? gShopPromptTexts[SHOP_PROMPT_PICK_BUY_POUCH_FULL] : gShopPromptTexts[SHOP_PROMPT_PICK_BUY]);
        break;
    case SHOP_STEP_COUNT:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_BUY_COUNT]);
        Font_SetPos(0x1FB, 0x184);
        sprintf(num, "%02d", list->count);
        Sjis_Widen(num, buf);
        Text_PrintfColored(buf);
        if (Shop_CanBuy(id, list->count + 1) == 1 && list->count < 99) {
            Icon_DrawMode(0x1FC, 0x16E, 0x24, 0x16, 0xFF00FF00, ICON_ARROW_UP, 2);
        }
        if (list->count > 1) {
            Icon_DrawMode(0x1FD, 0x196, 0x24, 0x16, 0xFF00FF00, ICON_ARROW_DOWN, 2);
        }
        break;
    case SHOP_STEP_CONFIRM:
        price = Shop_GetBuyPrice(item);
        price *= list->count;
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010,
                      Text_FillDollar(gShopPromptTexts[SHOP_PROMPT_PRICE_ASK], price));
        Font_SetPos(0x1A2, 0x192);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

void Font_SetColor(s32 color);
void Menu_DrawLabelBar(s16 x, s16 y, s16 a2, u32 a3, char *label, u8 t1, u8 t2);
void Menu_DrawMoney(s16 x, s16 y, s8 flag);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Win_Draw(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, s16 kind); /* unit's own declaration: the definition takes s32 positions, sizes and colours; declared with s16 ones, as Shop_DrawSell and Win_DrawText pass them, since with the definition's types their calls compile differently */
u32 Color_PulseAlpha(u32 color);
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void ItemInfo_Draw(s16 x, s16 y, s16 w, u8 a3, s32 item, u16 t1, s8 t2, u32 color);
void Win_DrawText(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, char *s);
void Sjis_Widen(char *, char *);
void Icon_DrawMode(s16 x, s16 y, s16 w, s16 h, u32 color, s32 i, u8 mode);
char *Text_FillDollar(char *fmt, s32 n);
extern char *gItemNames[];
extern char *gYesNoTexts[];
extern LobbyMenu *gLobbyMenu;

/*
 * The sell window: its title, the money held and the page number, the
 * pouch's page with each item's sell price (a dashed line for an empty
 * slot) and a pulsing bar on the cursor's row, the item info panel for the
 * item under the cursor, and the prompt of the step: pick an item, how many
 * (with the arrows the count can move by), or the price with yes and no.
 * `work` is an ItemListWin, as for Shop_DrawBuy.
 */
void Shop_DrawSell(void *work)
{
    s32 SavePouch_Count(u16 item);
    ItemListWin *list = work;
    s32 price;
    s16 x = list->win.pos + 4;
    u16 id;
    s16 y = 0x31;
    s16 i;
    s16 have;
    char num[16];
    char buf[32];

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(0x10E, 0x12, 0xA0, 0xFFB0B000, gShopMenuTexts[SHOP_MENU_TEXT_SELL], 2, 1);
    Font_SetColor(0x807F7F7F);
    Menu_DrawMoney(0x1E0, 0x10, 0);
    Menu_DrawPageNum(x + 0xF0, 0x3B, list->page, list->pages, 1);
    Win_Draw(x, y, 0x15E, 0x132, list->color, 0x80202020, 2);
    y += 10;
    Font_SetColor(0x80000000);
    Font_SetSpacing(20, 20);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < list->n; i++) {
        id = list->ent[i].id;
        price = list->ent[i].price;
        y += 0x18;
        if (list->cur == i) {
            Gfx_DrawLine(x + 0x14, y - 1, 0x136, 0x14, Color_PulseAlpha(0x8080));
        }
        if (id == 0) {
            Font_SetPos(x + 0x14, y);
            Text_PrintfColored("------------------------------");
        } else {
            Font_SetSpacing(20, 20);
            Font_SetPos(x + 0x2E, y);
            Text_PrintfColored("%s", gItemNames[id]);
            Font_SetSpacing(20, 20);
            Font_SetPos(x + 0xF8, y);
            Text_PrintfColored("% 7dz", price);
            Item_DrawIcon(x + 0x10, y - 1, 0x18, id);
        }
    }
    id = list->ent[list->cur].id;
    Gfx_DrawLine(x + 0x14, 0xFB, 0x136, 2, list->color);
    if (id != 0) {
        if (Item_IsDeco(id) != 0) {
            ItemInfo_Draw(x, 0xFF, 0x15E, list->infoMode, id, 0, list->infoPage, 0x807F7F7F);
        } else {
            ItemInfo_Draw(x, 0xFF, 0x15E, list->infoMode, id, 0, -1, 0x807F7F7F);
        }
    }
    Font_SetSpacing(18, 18);
    switch (list->state) {
    case SHOP_STEP_SLIDE_IN:
    case SHOP_STEP_LIST:
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_PICK_SELL]);
        break;
    case SHOP_STEP_COUNT:
        have = SavePouch_Count(id);
        if (gItemData[id].max == ITEM_MAX_UNLIMITED) {
            have = 1;
        }
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_SELL_COUNT]);
        Font_SetPos(0x1E9, 0x184);
        sprintf(num, "%02d", list->count);
        Sjis_Widen(num, buf);
        Text_PrintfColored(buf);
        if (list->count < have) {
            Icon_DrawMode(0x1EA, 0x16E, 0x24, 0x16, 0xFF00FF00, ICON_ARROW_UP, 2);
        }
        if (list->count > 1) {
            Icon_DrawMode(0x1EB, 0x196, 0x24, 0x16, 0xFF00FF00, ICON_ARROW_DOWN, 2);
        }
        break;
    case SHOP_STEP_CONFIRM:
        price = list->ent[list->cur].price;
        price *= list->count;
        Win_DrawText(0x120, 0x160, 0x14C, 0x50, list->color, 0xC0101010,
                      Text_FillDollar(gShopPromptTexts[SHOP_PROMPT_SELL_PRICE_ASK], price));
        Font_SetPos(0x190, 0x192);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

void Text_SetSize(s32);
void Shop_DrawBuy(void *work);
void Shop_DrawSell(void *work);
void Menu_DrawLabelList(s16 x, char **lines, s16 n, s16 cur, u32 color);
extern u8 gMenuWork[];

/* The shop's draw (LobbyMenu_SetDraw): the menu's lines while it is shown,
 * else the buy or the sell window. */
void ItemShop_Draw(MenuTask *task)
{
    ShopMenu *shop = (ShopMenu *)gMenuWork;
    ItemListWin *list = &shop->list;

    Text_SetSize(task->textSize);
    switch (shop->state) {
    case SHOP_MENU_CHOOSE:
        Menu_DrawLabelList(0x1CC, shop->lines, shop->n, shop->cur, 0xFFB0B000);
        break;
    case SHOP_MENU_BUY:
        Shop_DrawBuy(list);
        break;
    case SHOP_MENU_SELL:
        Shop_DrawSell(list);
        break;
    }
}

void Text_Printf(const char *fmt, ...);
void Win_Draw(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, s16 kind); /* unit's own declaration: the definition takes s32 positions, sizes and colours; declared with s16 ones, as Shop_DrawSell and Win_DrawText pass them, since with the definition's types their calls compile differently */
void Font_SetSpacing(u8 a, u8 b);
void Font_SetColor(s32 color);

/* A plain window (Win_Draw kind 0) at x, y, w by h in the two colours, with
 * `text` in it (none when NULL). */
void Win_DrawText(s16 x, s16 y, s16 w, s16 height, u32 fillColor, u32 edgeColor, char *text)
{
    Win_Draw(x, y, w, height, fillColor, edgeColor, 0);
    Font_SetSpacing(18, 18);
    Font_SetColor(0x807F7F7F);
    Font_SetPos(x + 10, y + 12);
    if (text != NULL) {
        Text_Printf(text);
    }
}

/* unit's own declaration: the definition returns s8; declared returning s32, as ItemInfo_Draw and LobbyPad_Read test it (with s8 their calls compile differently) */
s32 Kbd_IsOpen(void);
extern u8 gMenuInputLock;

/* The lobby menus' pad read for port `port`: nothing while gMenuInputLock
 * counts down or the keyboard is open; else the buttons just pressed and the
 * d-pad's repeats. A press locks the input for 3 frames. */
u16 LobbyPad_Read(s8 port)
{
    u16 pad = 0;

    if (gMenuInputLock != 0) {
        return 0;
    }
    if (Kbd_IsOpen() == 0) {
        pad = (gPadState[port].repeat & PAD_DPAD) | gPadState[port].pressed;
    }
    if (pad != 0) {
        gMenuInputLock = 3;
    }
    return pad;
}

/* The file's .data: the townspeople's stock lists, then the lobby's label tables. */
extern char D_0083BA90[];
extern char D_0083BA98[];
extern char D_0083BAA0[];
extern char D_0083BAA8[];
extern char D_0083BAB0[];
extern char D_0083BAB8[];
extern char D_0083BAC0[];
extern char D_0083BAC8[];
extern char D_0083BAD0[];
extern char D_0083BAE0[];
extern char D_0083BAF8[];
extern char D_0083BB00[];
extern char D_0083BB08[];
extern char D_0083BB10[];
extern char D_0083BB18[];
extern char D_0083BB28[];
extern char D_0083BB30[];
extern char D_0083BB38[];
extern char D_0083BB40[];
extern char D_0083BB48[];
extern char D_0083BB50[];
extern char D_0083BB58[];
extern char D_0083BB60[];
extern char D_0083BB68[];
extern char D_0083BB70[];
extern char D_0083BB78[];
extern char D_0083BB88[];
extern char D_0083BB98[];
extern char D_0083BBA8[];
extern char D_0083BBC0[];
extern char D_0083BBD8[];
extern char D_0083BBE0[];
extern char D_0083BBF0[];
extern char D_0083BC00[];
extern char D_0083BC10[];
extern char D_0083BC18[];
extern char D_0083BC28[];
extern char D_0083BC38[];
extern char D_0083BC48[];
extern char D_0083BC50[];
extern char D_0083BC60[];
extern char D_0083BC70[];
extern char D_0083BC80[];
extern char D_0083BC90[];
extern char D_0083BCA0[];
extern char D_0083BCB0[];
extern char D_0083BCC8[];
extern char D_0083BCD8[];
extern char D_0083BCE8[];
extern char D_0083BD00[];
extern char D_0083BD18[];
extern char D_0083BD28[];
extern char D_0083BD40[];
extern char D_0083BD58[];
extern char D_0083BD68[];
extern char D_0083BD78[];
extern char D_0083BD90[];
extern char D_0083BDB0[];
extern char D_0083BDD0[];
extern char D_0083BDF0[];
extern char D_0083BE08[];
extern char D_0083BE18[];
extern char D_0083BE28[];
extern char D_0083BE38[];
extern char D_0083BE48[];
extern char D_0083BE58[];
extern char D_0083BE68[];
extern char D_0083BE70[];
extern char D_0083BE78[];
extern char D_0083BE90[];
extern char D_0083BEA0[];
extern char D_0083BEB0[];
extern char D_0083BEC0[];
extern char D_0083BED0[];
extern char D_0083BEE0[];
extern char D_0083BEF0[];
extern char D_0083BF00[];
extern char D_0083BF10[];
extern char D_0083BF20[];
extern char D_0083BF30[];
extern char D_0083BF40[];
extern char D_0083BF50[];
extern char D_0083BF60[];
extern char D_0083BF70[];
extern char D_0083BF80[];
extern char D_0083BF90[];
extern char D_0083BFA0[];
extern char D_0083BFB0[];
extern char D_0083BFD0[];
extern char D_0083BFE8[];
extern char D_0083BFF8[];
extern char D_0083C008[];
extern char D_0083C018[];
extern char D_0083C030[];
extern char D_0083C050[];
extern char D_0083C060[];
extern char D_0083C078[];
extern char D_0083C080[];
extern char D_0083C088[];
extern char D_0083C098[];
extern char D_0083C0A0[];
extern char D_0083C0B8[];
extern char D_0083C0D0[];
extern char D_0083C0E8[];
extern char D_0083C100[];
extern char D_0083C120[];
extern char D_0083C150[];
extern char D_0083C170[];
extern char D_0083C180[];
extern char D_0083C1A0[];
extern char D_0083C1A8[];
extern char D_0083C1B8[];
extern char D_0083C1D0[];
extern char D_0083C1F0[];
extern char D_0083C208[];
extern char D_0083C218[];
extern char D_0083C228[];
extern char D_0083C240[];
extern char D_0083C258[];
extern char D_0083C260[];
extern char D_0083C270[];
extern char D_0083C280[];
extern char D_0083C290[];
extern char D_0083C2A0[];
extern char D_0083C2A8[];
extern char D_0083C2B8[];
extern char D_0083C2C0[];
extern char D_0083C2D0[];
extern char D_0083C2E0[];
extern char D_0083C2F0[];
extern char D_0083C300[];
extern char D_0083C310[];
extern char D_0083C320[];
extern char D_0083C328[];
extern char D_0083C340[];
extern char D_0083C370[];
extern char D_0083C3A0[];
extern char D_0083C3C8[];
extern char D_0083C3D0[];
extern char D_0083C3E0[];
extern char D_0083C400[];
extern char D_0083C440[];
extern char D_0083C470[];
extern char D_0083C498[];
extern char D_0083C4B0[];
extern char D_0083C4C8[];
extern char D_0083C4D8[];
extern char D_0083C4E8[];
extern char D_0083C500[];
extern char D_0083C520[];
extern char D_0083C540[];
extern char D_0083C560[];
extern char D_0083C580[];
extern char D_0083C598[];
extern char D_0083C5A8[];
extern char D_0083C5B8[];
extern char D_0083C5C8[];
extern char D_0083C5D0[];
extern char D_0083C5D8[];
extern char D_0083C5E0[];
extern char D_0083C5F0[];
extern char D_0083C610[];
extern char D_0083C630[];
extern char D_0083C660[];
extern char D_0083C690[];
extern char D_0083C6A8[];
extern char D_0083C6B8[];
extern char D_0083C6C8[];
extern char D_0083C6D8[];
extern char D_0083C6E8[];
extern char D_0083C6F8[];
extern char D_0083C700[];
extern char D_0083C710[];
extern char D_0083C730[];
extern char D_0083C760[];
extern char D_0083C790[];
extern char D_0083C7C0[];
extern char D_0083C810[];
extern char D_0083C830[];
extern char D_0083C860[];
extern char D_0083C890[];
extern char D_0083C8B0[];
extern char D_0083C8D0[];
extern char D_0083C8E0[];
extern char D_0083C8F0[];
extern char D_0083C908[];
extern char D_0083C918[];
extern char D_0083C928[];
extern char D_0083C930[];
extern char D_0083C940[];
extern char D_0083C950[];
extern char D_0083C960[];
extern char D_0083C980[];
extern char D_0083C9A0[];
extern char gHunterNotesTexts_8[];
extern char D_0083C9D0[];
extern char D_0083C9E8[];
extern char D_0083C9F0[];
extern char D_0083C9F8[];
extern char D_0083CA08[];
extern char D_0083CA10[];
extern char D_0083CA28[];
extern char D_0083CA38[];
extern char D_0083CA40[];
extern char D_0083CA60[];
extern char D_0083CA70[];
extern char D_0083CA90[];
extern char D_0083CAB0[];
extern char D_0083CAC8[];
extern char D_0083CAD8[];
extern char D_0083CAE8[];
extern char D_0083CAF0[];
extern char D_0083CB00[];
extern char D_0083CB10[];
extern char D_0083CB20[];
extern char D_0083CB30[];
extern char D_0083CB38[];
extern char D_0083CB50[];
extern char D_0083CB80[];
extern char D_0083CBD0[];
extern char D_0083CC00[];
extern char D_0083CC20[];
extern char D_0083CC30[];
extern char D_0083CC40[];
extern char D_0083CC70[];
extern char D_0083CCC0[];
extern char D_0083CD10[];
extern char D_0083CD40[];
extern char D_0083CD90[];
extern char D_0083CDB0[];
extern char D_0083CDC8[];
extern char D_0083CDD8[];
extern char D_0083CDE8[];
extern char D_0083CDF8[];
extern char D_0083CE08[];
extern char D_0083CE18[];
extern char D_0083CE30[];
extern char D_0083CE60[];
extern char D_0083CE80[];
extern char D_0083CEB0[];
extern char D_0083CEC8[];
extern char D_0083CED8[];
extern char D_0083CEF0[];
extern char D_0083CF08[];
extern char D_0083CF18[];
extern char D_0083CF20[];
extern char D_0083CF28[];
extern char D_0083CF30[];
extern char D_0083CF38[];
extern char D_0083CF40[];
extern char D_0083CF48[];
extern char D_0083CF58[];
extern char D_0083CF60[];
extern char D_0083CF68[];
extern char D_0083CF80[];
extern char D_0083CFC0[];
extern char D_0083CFD0[];
extern char D_0083CFE0[];
extern char D_0083CFE8[];
extern char D_0083D000[];
extern char D_0083D018[];
extern char D_0083D028[];
extern char D_0083D030[];
extern char D_0083D040[];
extern char D_0083D050[];
extern char D_0083D060[];
extern char D_0083D070[];
extern char D_0083D088[];
extern char D_0083D0A0[];
extern char D_0083D0B8[];
extern char D_0083D0C8[];
extern char D_0083D0D8[];
extern char D_0083D0E8[];
extern char D_0083D0F8[];
extern char D_0083D100[];
extern char D_0083D118[];
extern char D_0083D128[];
extern char D_0083D130[];
extern char D_0083D140[];
extern char D_0083D158[];
extern char D_0083D168[];
extern char D_0083D170[];
extern char D_0083D180[];
extern char D_0083D188[];
extern char D_0083D190[];
extern char D_0083D198[];
extern char D_0083D1B0[];
extern char D_0083D220[];
extern char D_0083D280[];
extern char D_0083D2B0[];
extern char D_0083D310[];
extern char D_0083D318[];
extern char D_0083D328[];
extern char D_0083D338[];
extern char D_0083D340[];
extern char D_0083D350[];
extern char D_0083D360[];
extern char D_0083D370[];
extern char D_0083D378[];
extern char D_0083D380[];
extern char D_0083D388[];
extern char D_0083D398[];
extern char D_0083D3A8[];
extern char D_0083D3B8[];
extern char D_0083D3C8[];
extern char D_0083D3D8[];
extern char D_0083D3E8[];
extern char D_0083D3F8[];
extern char D_0083D400[];
extern char D_0083D420[];
extern char D_0083D440[];
extern char D_0083D458[];
extern char D_0083D468[];
extern char D_0083D478[];
extern char D_0083D488[];
extern char D_0083D498[];
extern char D_0083D4A8[];
extern char D_0083D4B8[];
extern char D_0083D4C0[];
extern char D_0083D4D0[];
extern char D_0083D500[];
extern char D_0083D520[];
extern char D_0083D540[];
extern char D_0083D550[];
extern char D_0083D560[];
extern char D_0083D570[];
extern char D_0083D590[];
extern char D_0083D5A0[];
extern char D_0083D5A8[];
extern char D_0083D5B0[];
extern char D_0083D5B8[];
extern char D_0083D5C8[];
extern char D_0083D5D8[];
extern char D_0083D5E8[];
extern char D_0083D600[];
extern char D_0083D620[];
extern char D_0083D680[];
extern char D_0083D700[];
extern char D_0083D718[];
extern char D_0083D720[];
extern char D_0083D728[];
extern char D_0083D730[];
extern char D_0083D738[];
extern char D_0083D740[];
extern char D_0083D748[];
extern char D_0083D750[];
extern char D_0083D758[];
extern char D_0083D760[];
extern char D_0083D768[];
extern char D_0083D770[];
extern char D_0083D778[];
extern char D_0083D780[];
extern char D_0083D788[];
extern char D_0083D790[];
extern char D_0083D7A0[];
extern char D_0083D7B8[];
extern char D_0083D7C0[];
extern char D_0083D7E0[];
extern char D_0083D800[];
extern char D_0083D820[];
extern char D_0083D838[];
extern char D_0083D840[];
extern char D_0083D850[];
extern char D_0083D860[];
extern char D_0083D880[];
extern char D_0083D898[];
extern char D_0083D8A8[];
extern char D_0083D8B8[];
extern char D_0083D8C8[];
extern char D_0083D8D8[];
extern char D_0083D8E8[];
extern char D_0083D8F8[];
extern char D_0083D908[];
extern char D_0083D918[];
extern char D_0083D928[];
extern char D_0083D940[];
extern char D_0083D960[];
extern char D_0083D978[];
extern char D_0083D988[];
extern char D_0083D9A0[];
extern char D_0083D9D0[];
extern char D_0083DA00[];
extern char D_0083DA50[];
extern char D_0083DAA0[];
extern char D_0083DAD0[];
extern char D_0083DAF0[];
extern char D_0083DB20[];
extern char D_0083DB70[];
extern char D_0083DBB0[];
extern char D_0083DBE0[];
extern char D_0083DC30[];
extern char D_0083DC78[];
extern char D_0083DC88[];
extern char D_0083DC98[];
extern char D_0083DCA0[];
extern char D_0083DCB0[];
extern char D_0083DCC0[];
extern char D_0083DCE0[];
extern char D_0083DD00[];
extern char D_0083DD10[];
extern char D_0083DD20[];
extern char D_0083DD30[];
extern char D_0083DD40[];
extern char D_0083DD50[];
extern char D_0083DD60[];
extern char D_0083DD70[];
extern char D_0083DD80[];
extern char D_0083DD90[];
extern char D_0083DDA0[];
extern char D_0083DDC0[];
extern char D_0083DDD8[];
extern char D_0083DDF0[];
extern char D_0083DE30[];
extern char D_0083DE70[];
extern char D_0083DEB0[];
extern char D_0083DEE0[];
extern char D_0083DF30[];
extern char D_0083DF90[];
extern char D_0083DFB0[];
extern char D_0083DFD0[];
extern char D_0083DFE8[];
extern char D_0083DFF8[];
extern char D_0083E008[];
extern char D_0083E020[];
extern char D_0083E038[];
extern char D_0083E050[];
extern char D_0083E070[];
extern char D_0083E090[];
extern char D_0083E0B0[];
extern char D_0083E0D0[];
extern char D_0083E0F0[];
extern char D_0083E108[];
extern char D_0083E118[];
extern char D_0083E130[];
extern char D_0083E150[];
extern char D_0083E170[];
extern char D_0083E1C0[];
extern char D_0083E260[];
extern char D_0083E2C0[];
extern char D_0083E350[];
extern char D_0083E3B0[];
extern char D_0083E420[];
extern char D_0083E490[];
extern char D_0083E530[];
extern char D_0083E590[];
extern char D_0083E610[];
extern char D_0083E670[];
extern char D_0083E6A0[];
extern char D_0083E6F0[];
extern char D_0083E740[];
extern char D_0083E790[];
extern char D_0083E7E0[];
extern char D_0083E830[];
extern char D_0083E880[];
extern char D_0083E8D0[];
extern char D_0083E930[];
extern char D_0083E9A0[];
extern char D_0083E9D0[];
extern char D_0083EA00[];
extern char D_0083EA30[];
extern char D_0083EA50[];
extern char D_0083EA90[];
extern char D_0083EAB0[];
extern char D_0083EB00[];
extern char D_0083EB70[];
extern char D_0083EBC0[];
extern char D_0083EC00[];
extern char D_0083EC30[];
extern char D_0083EC60[];
extern char D_0083EC80[];
extern char D_0083EC98[];
extern char D_0083ECA8[];
extern char D_0083ECC0[];
extern char D_0083ECD8[];
extern char D_0083ECF0[];
extern char D_0083ED10[];
extern char D_0083ED30[];
extern char D_0083ED50[];
extern char D_0083ED70[];
extern char D_0083ED90[];
extern char D_0083EDA8[];
extern char D_0083EDB0[];
extern char D_0083EDC0[];
extern char D_0083EDE0[];
extern char D_0083EE00[];
extern char D_0083EE38[];
extern char D_0083EE50[];
extern char D_0083EE68[];
extern char D_0083EE80[];
extern char D_0083EE98[];
extern char D_0083EEA8[];
extern char D_0083EEC0[];
extern char D_0083EED8[];
extern char D_0083EEE8[];
extern char D_0083EEF8[];
extern char D_0083EF08[];
extern char D_0083EF18[];
extern char D_0083EF28[];
extern char D_0083EF38[];
extern char D_0083EF48[];
extern char D_0083EF58[];
extern char D_0083EF68[];
extern char D_0083EF78[];
extern char D_0083EF90[];
extern char D_0083EFB0[];
extern char D_0083EFD0[];
extern char D_0083EFF0[];
extern char D_0083F010[];
extern char D_0083F030[];
extern char D_0083F050[];
extern char D_0083F070[];
extern char D_0083F090[];
extern char D_0083F0B0[];
extern char D_0083F0D0[];
extern char D_0083F0F0[];
extern char D_0083F110[];
extern char D_0083F130[];
extern char D_0083F140[];
extern char D_0083F150[];
extern char D_0083F170[];
extern char D_0083F190[];
extern char D_0083F1B0[];
extern char D_0083F1D0[];
extern char D_0083F1F0[];
extern char D_0083F210[];
extern char D_0083F230[];
extern char D_0083F250[];
extern char D_0083F270[];
extern char D_0083F290[];
extern char D_0083F2B0[];
extern char D_0083F2D0[];
extern char D_0083F2E8[];
extern char D_0083F300[];
extern char D_0083F318[];
extern char D_0083F328[];
extern char D_0083F338[];
extern char D_0083F348[];
extern char D_0083F358[];
extern char D_0083F368[];
extern char D_0083F370[];
extern char D_0083F380[];
extern char D_0083F3A0[];
extern char D_0083F3C0[];
extern char D_0083F3D8[];
extern char D_0083F3E8[];
extern char D_0083F3F0[];
extern char D_0083F3F8[];
extern char D_0083F400[];
extern char D_0083F408[];
extern char D_0083F410[];
extern char D_0083F430[];
extern char D_0083F450[];
extern char D_0083F470[];
extern char D_0083F480[];
extern char D_0083F4A0[];
extern char D_0083F4B0[];
extern char D_0083F4C0[];
extern char D_0083F4E0[];
extern char D_0083F4F0[];
extern char D_0083F510[];
extern char D_0083F530[];
extern char D_0083F540[];
extern char D_0083F550[];
extern char D_0083F560[];
extern char D_0083F570[];
extern char D_0083F590[];
extern char D_0083F5A8[];
extern char D_0083F5C0[];
extern char D_0083F5E0[];
extern char D_0083F600[];
extern char D_0083F618[];
extern char D_0083F620[];
extern char D_0083F650[];
extern char D_0083F670[];
extern char D_0083F6A0[];
extern char D_0083F6D0[];
extern char D_0083F700[];
extern char D_0083F720[];
extern char D_0083F750[];
extern char D_0083F770[];
extern char D_0083F7A0[];
extern char D_0083F7C0[];
extern char D_0083F810[];
extern char D_0083F830[];
extern char D_0083F860[];
extern char D_0083F880[];
extern char D_0083F8A0[];
extern char D_0083F8B8[];
extern char D_0083F8D0[];
extern char D_0083F8F0[];
extern char D_0083F910[];
extern char D_0083F940[];
extern char D_0083F970[];
extern char D_0083F9A0[];
extern char D_0083F9C0[];
extern char D_0083F9E0[];
extern char D_0083FA00[];
extern char D_0083FA30[];
extern char D_0083FA78[];
extern char D_0083FA90[];
extern char D_0083FAB0[];
extern char D_0083FAE0[];
extern char D_0083FB10[];
extern char D_0083FB40[];
extern char D_0083FB70[];
extern char D_0083FB90[];
extern char D_0083FBB0[];
extern char D_0083FBE0[];
extern char D_0083FC10[];
extern char D_0083FC30[];
extern char D_0083FC70[];
extern char D_0083FCA0[];
extern char D_0083FCB0[];
extern char D_0083FCD0[];
extern char D_0083FD20[];
extern char D_0083FD40[];
extern char D_0083FD60[];
extern char D_0083FD80[];
extern char D_0083FDB0[];
extern char D_0083FDD0[];
extern char D_0083FE10[];
extern char D_0083FE50[];
extern char D_0083FE80[];
extern char D_0083FEB0[];
extern char D_0083FED0[];
extern char D_0083FF10[];
extern char D_0083FF50[];
extern char D_0083FF90[];
extern char D_0083FFC0[];
extern char D_00840000[];
extern char D_00840040[];
extern char D_00840080[];
extern char D_008400B0[];
extern char D_008400E0[];
extern char D_00840120[];
extern char D_00840160[];
extern char D_00840190[];
extern char D_008401A8[];
extern char D_008401B0[];
extern char D_008401B8[];
extern char D_008401C0[];
extern char D_008401D0[];
extern char D_008401D8[];
extern char D_008401E0[];
extern char D_008401F8[];
extern char D_00840208[];
extern char D_00840220[];
extern char D_00840240[];
extern char D_00840258[];
extern char D_00840268[];
extern char D_00840270[];
extern char D_00840280[];
extern char D_00840288[];
extern char D_00840298[];
extern char D_008402A8[];
extern char D_008402B8[];
extern char D_008402C8[];
extern char D_008402D8[];
extern char D_008402E0[];
extern char D_008402F8[];
extern char D_00840308[];
extern char D_00840318[];
extern char D_00840328[];
extern char D_00840338[];
extern char D_00840348[];
extern char D_00840360[];
extern char D_00840378[];
extern char D_00840390[];
extern char D_008403B0[];
extern char D_008403D0[];
extern char D_008403F0[];
extern char D_00840410[];
extern char D_00840430[];
extern char D_00840450[];
extern char D_00840470[];
extern char D_00840490[];
extern char D_008404B0[];
extern char D_008404D0[];
extern char D_008404F0[];
extern char D_00840510[];
extern char D_00840528[];
extern char D_00840538[];
extern char D_00840550[];
extern char D_00840568[];
extern char D_00840578[];
extern char D_00840588[];
extern char D_008405A0[];
extern char D_008405D0[];
extern char D_008405F0[];
extern char D_00840620[];
extern char D_00840638[];
extern char D_00840648[];
extern char D_00840658[];
extern char D_00840668[];
extern char D_00840678[];
extern char D_00840688[];
extern char D_008406A0[];
extern char D_008406C0[];
extern char D_008406D8[];
extern char D_008406E8[];
extern char D_008406F8[];
extern char D_00840708[];
extern char D_00840718[];
extern char D_00840728[];
extern char D_00840740[];
extern char D_00840760[];
extern char D_00840778[];
extern char D_00840788[];
extern char D_00840798[];
extern char D_008407A8[];
extern char D_008407C0[];
extern char D_008407D8[];
extern char D_008407E8[];
extern char D_008407F8[];
extern char D_00840808[];
extern char D_00840818[];
extern char D_00840828[];
extern char D_00840838[];
extern char D_00840850[];
extern char D_00840870[];
extern char D_00840888[];
extern char D_00840898[];
extern char D_008408A8[];
extern char D_008408B8[];
extern char D_008408C8[];
extern char D_008408D0[];
extern char D_008408E0[];
extern char D_008408F0[];
extern char D_00840900[];
extern char D_00840910[];
extern char D_00840920[];
extern char D_00840930[];
extern char D_00840940[];
extern char D_00840950[];
extern char D_00840990[];
extern char D_008409B0[];
extern char D_008409E0[];
extern char D_00840A10[];
extern char D_00840A30[];
extern char D_00840A70[];
extern char D_00840AB0[];
extern char D_00840AD0[];
extern char D_00840AF0[];
extern char D_00840B10[];
extern char D_00840B30[];
extern char D_00840B50[];
extern char D_00840B70[];
extern char D_00840B90[];
extern char D_00840BB0[];
extern char D_00840BF0[];
extern char D_00840C40[];
extern char D_00840C80[];
extern char D_00840CC0[];
extern char D_00840CF0[];
extern char D_00840D20[];
extern char D_00840D50[];
extern char D_00840DB0[];
extern char D_00840E10[];
extern char D_00840E40[];
extern char D_00840E70[];
extern char D_00840EA0[];
extern char D_00840EC0[];
extern char D_00840F00[];
extern char D_00840F60[];
extern char D_00840F90[];
extern char D_00840FD0[];
extern char D_00841010[];
extern char D_00841048[];
extern char D_00841058[];
extern char D_00841068[];
extern char D_00841078[];
extern char D_00841088[];
extern char D_00841098[];
extern char D_008410B0[];
extern char D_008410C8[];
extern char D_008410E0[];
extern char D_00841100[];
extern char D_00841130[];
extern char D_00841160[];
extern char D_008411B0[];
extern char D_00841240[];
extern char D_00841290[];
extern char D_008412F8[];
extern char D_00841310[];
extern char D_00841330[];
extern char D_00841340[];
extern char D_00841350[];
extern char D_00841370[];
extern char D_00841390[];
extern char D_008413B0[];
extern char D_008413D0[];
extern char D_008413F0[];
extern char D_00841410[];
extern char D_00841420[];
extern char D_00841430[];
extern char D_00841450[];
extern char D_00841470[];
extern char D_00841490[];
extern char D_008414A8[];
extern char D_008414B8[];
extern char D_008414C8[];
extern char D_008414D8[];
extern char D_008414F0[];
extern char D_00841508[];
extern char D_00841510[];
extern char D_00841520[];
extern char D_00841530[];
extern char D_00841540[];
extern char D_00841548[];
extern char D_00841550[];
extern char D_00841558[];
extern char D_00841560[];
extern char D_00841580[];
extern char D_008415A0[];
extern char D_008415C0[];
extern char D_008415E0[];
extern char D_00841600[];
extern char D_00841620[];
extern char D_00841640[];
extern char D_00841660[];
extern char D_00841680[];
extern char D_008416A0[];
extern char D_008416C0[];
extern char D_008416D8[];
extern char D_008416F0[];
extern char D_00841710[];
extern char D_00841728[];
extern char D_00841738[];
extern char D_00841750[];
extern char D_00841770[];
extern char D_00841790[];
extern char D_008417B0[];
extern char D_008417C8[];
extern char D_008417E0[];
extern char D_00841800[];
extern char D_00841820[];
extern char D_00841838[];
extern char D_00841850[];
extern char D_00841860[];
extern char D_00841870[];
extern char D_00841888[];
extern char D_00841890[];
extern char D_008418A0[];
extern char D_008418A8[];
extern char D_008418B0[];
extern char D_008418B8[];
extern char D_008418C8[];
extern char D_008418D8[];
extern char D_008418E8[];
extern char D_008418F8[];
extern char D_00841908[];
extern char D_00841918[];
extern char D_00841920[];
extern char D_00841930[];
extern char D_00841940[];
extern char D_00841960[];
extern char D_00841980[];
extern char D_008419A0[];
extern char D_008419D0[];
extern char D_00841A00[];
extern char D_00841A70[];
extern char D_00841AA0[];
extern char D_00841AD0[];
extern char D_00841B40[];
extern char D_00841B70[];
extern char D_00841BA0[];
extern char D_00841C10[];
extern char D_00841C70[];
extern char D_00841CE0[];
extern char D_00841D20[];
extern char D_00841D78[];
extern char D_00841D88[];
extern char D_00841D98[];
extern char D_00841DA8[];
extern char D_00841DB8[];
extern char D_00841DD0[];
extern char D_00841DE8[];
extern char D_00841E00[];
extern char D_00841E18[];
extern char D_00841E30[];
extern char D_00841E60[];
extern char D_00841E80[];
extern char D_00841EA0[];
extern char D_00841EA8[];
extern char D_00841EB0[];
extern char D_00841EC0[];
extern char D_00841F00[];
extern char D_00841F60[];
extern char D_00841FB0[];
extern char D_00841FE0[];
extern char D_00841FF0[];
extern char D_00841FF8[];
extern char D_00842000[];
extern char D_00842008[];
extern char D_00842010[];
extern char D_00842018[];
extern char D_00842020[];
extern char D_00842030[];
extern char D_00842040[];
extern char D_00842050[];
extern char D_00842060[];
extern char D_00842070[];
extern char D_00842080[];
extern char D_00842090[];
extern char D_008420A0[];
extern char D_008420A8[];
extern char D_008420C0[];
extern char D_008420E0[];
extern char D_00842100[];
extern char D_00842120[];
extern char D_00842140[];
extern char D_00842160[];
extern char D_00842170[];
extern char D_00842190[];
extern char D_008421B0[];
extern char D_008421D0[];
extern char D_008421F0[];
extern char D_00842210[];
extern char D_00842230[];
extern char D_00842260[];
extern char D_00842290[];
extern char D_008422C0[];
extern char D_008422F0[];
extern char D_00842320[];
extern char D_00842340[];
extern char D_00842360[];
extern char D_00842380[];
extern char D_008423A0[];
extern char D_008423C0[];
extern char D_008423E0[];
extern char D_00842400[];
extern char D_00842420[];
extern char D_00842440[];
extern char D_00842470[];
extern char D_00842490[];
extern char D_008424B0[];
extern char D_008424D0[];
extern char D_008424E0[];
extern char D_00842500[];
extern char D_00842530[];
extern char D_00842560[];
extern char D_00842580[];
extern char D_008425B0[];
extern char D_008425D8[];
extern char D_008425E8[];
extern char D_00842600[];
extern char D_00842620[];
extern char D_00842660[];
extern char D_00842680[];
extern char D_008426A0[];
extern char D_008426D0[];
extern char D_00842700[];
extern char D_00842720[];
extern char D_00842750[];
extern char D_00842770[];
extern char D_00842790[];
extern char D_008427B0[];
extern char D_008427D0[];
extern char D_008427E8[];
extern char D_008427F8[];
extern char D_00842810[];
extern char D_00842840[];
extern char D_00842860[];
extern char D_00842880[];
extern char D_008428A0[];
extern char D_008428B0[];
extern char D_008428C8[];
extern char D_008428D8[];
extern char D_008428E8[];
extern char D_00842900[];
extern char D_00842920[];
extern char D_00842930[];
extern char D_00842940[];
extern char D_00842958[];
extern char D_00842968[];
extern char D_00842978[];
extern char D_00842988[];
extern char D_00842990[];
extern char D_008429A0[];
extern char D_008429B8[];
extern char D_008429C8[];
extern char D_008429D8[];
extern char D_008429E8[];
extern char D_008429F8[];
extern char D_00842A10[];
extern char D_00842A28[];
extern char D_00842A38[];
extern char D_00842A40[];
extern char D_00842A58[];
extern char D_00842A70[];
extern char D_00842A88[];
extern char D_00842A98[];
extern char D_00842AB0[];
extern char D_00842AD0[];
extern char D_00842AF0[];
extern char D_00842AF8[];
extern char D_00842B08[];
extern char D_00842B18[];
extern char D_00842B28[];
extern char D_00842B40[];
extern char D_00842B60[];
extern char D_00842C00[];
extern char D_00842C30[];
extern char D_00842C60[];
extern char D_00842C80[];
extern char D_00842D00[];
extern char D_00842D30[];
extern char D_00842D50[];
extern char D_00842D90[];
extern char D_00842DC0[];
extern char D_00842DF0[];
extern char D_00842E10[];
extern char D_00842E40[];
extern char D_00842EC0[];
extern char D_00842F00[];
extern char D_00842F30[];
extern char D_00842F60[];
extern char D_00842FC0[];
extern char D_00842FE0[];
extern char D_00843090[];
extern char D_00843130[];
extern char D_008431C0[];
extern char D_00843260[];
extern char D_00843290[];
extern char D_008432D0[];
extern char D_008432F0[];
extern char D_00843340[];
extern char D_008433E0[];
extern char D_00843480[];
extern char D_008434B0[];
extern char D_008434F0[];
extern char D_00843520[];
extern char D_008435B0[];
extern char D_00843660[];
extern char D_008436F0[];
extern char D_00843730[];
extern char D_00843770[];
extern char D_008437B0[];
extern char D_00843800[];
extern char D_00843840[];
extern char D_00843880[];
extern char D_008438C0[];
extern char D_008438F0[];
extern char D_00843980[];
extern char D_00843988[];
extern char D_00843998[];
extern char D_008439A8[];
extern char D_008439B0[];
extern char D_008439B8[];
extern char D_008439C0[];
extern char D_008439C8[];
extern char D_008439D8[];
extern char D_008439E0[];
extern char D_008439E8[];
extern char D_008439F0[];
extern char D_008439F8[];
extern char D_00843A00[];
extern char D_00843A08[];
extern char D_00843A18[];
extern char D_00843A20[];
extern char D_00843A28[];
extern char D_00843A40[];
extern char D_00843A60[];
extern char D_00843A78[];
extern char D_00843A88[];
extern char D_00843AA0[];
extern char D_00843AB8[];
extern char D_00843AC8[];
extern char D_00843AE0[];
extern char D_00843AF8[];
extern char D_00843B00[];
extern char D_00843B10[];
extern char D_00843B20[];
extern char D_00843B30[];
extern char D_00843B40[];
extern char D_00843B60[];
extern char D_00843B80[];
extern char D_00843BA0[];
extern char D_00843BC0[];
extern char D_00843BE0[];
extern char D_00843C00[];
extern char D_00843C20[];
extern char D_00843C40[];
extern char D_00843C48[];
extern char D_00843C58[];
extern char D_00843C68[];
extern char D_00843C80[];
extern char D_00843CA0[];
extern char D_00843CC0[];
extern char D_00843CE0[];
extern char D_00843D10[];
extern char D_00843D30[];
extern char D_00843D40[];
extern char D_00843D50[];
extern char D_00843D60[];
extern char D_00843D70[];
extern char D_00843D88[];
extern char D_00843D98[];
extern char D_00843DB0[];
extern char D_00843DD0[];
extern char D_00843DE8[];
extern char D_00843DF0[];
extern char D_00843DF8[];
extern char D_00843E10[];
extern char D_00843E28[];
extern char D_00843E30[];
extern char D_00843E40[];
extern char D_00843E50[];
extern char D_00843E60[];
extern char D_00843E70[];
extern char D_00843E80[];
extern char D_00843E90[];
extern char D_00843EB0[];
extern char D_00843ED0[];
extern char D_00843EE0[];
extern char D_00843EF0[];
extern char D_00843EF8[];
extern char D_00843F08[];
extern char D_00843F18[];
extern char D_00843F28[];
extern char D_00843F38[];
extern char D_00843F48[];
extern char D_00843F58[];
extern char D_00843F68[];
extern char D_00843F70[];
extern char D_00843F88[];
extern char D_00843F90[];
extern char D_00843FA0[];
extern char D_00843FB0[];
extern char D_00843FC0[];
extern char D_00843FD0[];
extern char D_00843FE0[];
extern char D_00844000[];
extern char D_00844020[];
extern char D_00844040[];
extern char D_00844080[];
extern char D_008440C0[];
extern char D_008440F0[];
extern char D_00844120[];
extern char D_00844160[];
extern char D_008441C0[];
extern char D_008441E0[];
extern char D_00844220[];
extern char D_00844260[];
extern char D_008442B0[];
extern char D_008442F0[];
extern char D_00844320[];
extern char D_00844340[];
extern char D_00844380[];
extern char D_008443C0[];
extern char D_008443E0[];
extern char D_00844430[];
extern char D_00844460[];
extern char D_00844490[];
extern char D_008444C0[];
extern char D_008444F0[];
extern char D_00844520[];
extern char D_00844550[];
extern char D_00844570[];
extern char D_008445D0[];
extern char D_00844620[];
extern char D_00844650[];
extern char D_00844670[];
extern char D_00844690[];
extern char D_008446C0[];
extern char D_008446F0[];
extern char D_00844730[];
extern char D_00844760[];
extern char D_00844790[];
extern char D_008447C0[];
extern char D_008447F0[];
extern char D_00844820[];
extern char D_00844860[];
extern char D_008448A0[];
extern char D_00844910[];
extern char D_00844950[];
extern char D_00844980[];
extern char D_008449E0[];
extern char D_00844A10[];
extern char D_00844A40[];
extern char D_00844A70[];
extern char D_00844AA0[];
extern char D_00844AE0[];
extern char D_00844B50[];
extern char D_00844BA0[];
extern char D_00844BD0[];
extern char D_00844BF0[];
extern char D_00844C30[];
extern char D_00844C50[];
extern char D_00844CA0[];
extern char D_00844D00[];
extern char D_00844D60[];
extern char D_00844D90[];
extern char D_00844DD0[];
extern char D_00844E10[];
extern char D_00844E30[];
extern char D_00844E50[];
extern char D_00844E70[];
extern char D_00844EA0[];
extern char D_00844EC0[];
extern char D_00844EE0[];
extern char D_00844F00[];
extern char D_00844F30[];
extern char D_00844F60[];
extern char D_00844F80[];
extern char D_00844FD0[];
extern char D_00845030[];
extern char D_00845090[];
extern char D_008450F0[];
extern char D_00845120[];
extern char D_00845180[];
extern char D_008451E0[];
extern char D_00845240[];
extern char D_00845270[];
extern char D_008452A0[];
extern char D_008452D0[];
extern char D_00845310[];
extern char D_00845370[];
extern char D_008453A0[];
extern char D_008453D0[];
extern char D_00845400[];
extern char D_00845460[];
extern char D_008454B0[];
extern char D_008454E0[];
extern char D_00845510[];
extern char D_00845540[];
extern char D_008455A0[];
extern char D_008456B0[];
extern char D_00845740[];
extern char D_00845770[];
extern char D_008457A0[];
extern char D_008457F0[];
extern char D_00845840[];
extern char D_00845870[];
extern char D_008458B0[];
extern char D_008458E0[];
extern char D_00845910[];
extern char D_00845930[];
extern char D_00845980[];
extern char D_008459B0[];
extern char D_008459E0[];
extern char D_00845A70[];
extern char D_00845B10[];
extern char D_00845B70[];
extern char D_00845C30[];
extern char D_00845CC0[];
extern char D_00845D90[];
extern char D_00845E90[];
extern char D_00845EB0[];
extern char D_00845ED0[];
extern char D_00845F00[];
extern char D_00845F20[];
extern char D_00845F40[];
extern char D_00845F70[];
extern char D_00845FD0[];
extern char D_00846030[];
extern char D_00846090[];
extern char D_008460F0[];
extern char D_00846170[];
extern char D_008461A0[];
extern char D_008461F0[];
extern char D_00846250[];
extern char D_00846280[];
extern char D_008462D0[];
extern char D_00846310[];
extern char D_00846350[];
extern char D_00846370[];
extern char D_008463A0[];
extern char D_00846400[];
extern char D_00846430[];
extern char D_00846480[];
extern char D_008464B0[];
extern char D_008464F0[];
extern char D_008465A0[];
extern char D_00846620[];
extern char D_00846650[];
extern char D_008466A0[];
u32 gShopCondStockNpc07_8[24] = {
#include "lobby/0080B1A0.inc"
};
u32 gShopCondStockNpc07_7[24] = {
#include "lobby/0080B200.inc"
};
u32 gShopCondStockNpc07_6[24] = {
#include "lobby/0080B260.inc"
};
u32 gShopCondStockNpc07_5[24] = {
#include "lobby/0080B2C0.inc"
};
u32 gShopCondStockNpc07_4[28] = {
#include "lobby/0080B320.inc"
};
u32 gShopCondStockNpc07_3[28] = {
#include "lobby/0080B390.inc"
};
u32 gShopCondStockNpc07_2[28] = {
#include "lobby/0080B400.inc"
};
u32 gShopCondStockNpc07_1[32] = {
#include "lobby/0080B470.inc"
};
u32 gShopCondStockNpc07_0[32] = {
#include "lobby/0080B4F0.inc"
};
ShopStockCond gShopCondStockNpc07[10] = {
#include "lobby/0080B570.inc"
};
u32 gShopCondStockNpc08_5[8] = {
#include "lobby/0080B5C0.inc"
};
u32 gShopCondStockNpc08_4[12] = {
#include "lobby/0080B5E0.inc"
};
u32 gShopCondStockNpc08_3[12] = {
#include "lobby/0080B610.inc"
};
u32 gShopCondStockNpc08_2[12] = {
#include "lobby/0080B640.inc"
};
u32 gShopCondStockNpc08_1[12] = {
#include "lobby/0080B670.inc"
};
u32 gShopCondStockNpc08_0[12] = {
#include "lobby/0080B6A0.inc"
};
ShopStockCond gShopCondStockNpc08[6] = {
#include "lobby/0080B6D0.inc"
};
u16 gShopStockNpc0A_0[24] = {
#include "lobby/0080B700.inc"
};
u16 gShopStockNpc0A_1[28] = {
#include "lobby/0080B730.inc"
};
u16 *gShopStockNpc0A[2] = {
    gShopStockNpc0A_0, gShopStockNpc0A_1,
};
u16 gShopStockNpc0B[16] = {
#include "lobby/0080B770.inc"
};
u16 gShopStockNpc0BLv2[24] = {
#include "lobby/0080B790.inc"
};
u16 gShopStockNpc0BTimer[24] = {
#include "lobby/0080B7C0.inc"
};
u16 gShopStockNpc0C[32] = {
#include "lobby/0080B7F0.inc"
};
u16 gShopStockNpc0CLv2[32] = {
#include "lobby/0080B830.inc"
};
u16 gShopStockNpc12Psp_0[24] = {
#include "lobby/0080B870.inc"
};
u16 gShopStockNpc12Psp_1[32] = {
#include "lobby/0080B8A0.inc"
};
u16 gShopStockNpc12Psp_2[32] = {
#include "lobby/0080B8E0.inc"
};
u16 gShopStockNpc12Psp_3[32] = {
#include "lobby/0080B920.inc"
};
u16 gShopStockNpc12Timer_0[32] = {
#include "lobby/0080B960.inc"
};
u16 gShopStockNpc12Timer_1[40] = {
#include "lobby/0080B9A0.inc"
};
u16 *gShopStockNpc12Psp[4] = {
    gShopStockNpc12Psp_0, gShopStockNpc12Psp_1, gShopStockNpc12Psp_2, gShopStockNpc12Psp_3,
};
u16 *gShopStockNpc12Timer[2] = {
    gShopStockNpc12Timer_0, gShopStockNpc12Timer_1,
};
u32 gShopCondStockNpc28_2[2] = {
#include "lobby/0080BA08.inc"
};
u32 gShopCondStockNpc28_1[4] = {
#include "lobby/0080BA10.inc"
};
u32 gShopCondStockNpc28_0[4] = {
#include "lobby/0080BA20.inc"
};
ShopStockCond gShopCondStockNpc28[4] = {
#include "lobby/0080BA30.inc"
};
u16 gShopStockNpc2C_0[24] = {
#include "lobby/0080BA50.inc"
};
u16 gShopStockNpc2C_1[32] = {
#include "lobby/0080BA80.inc"
};
u16 *gShopStockNpc2C[4] = {
    gShopStockNpc2C_0, gShopStockNpc2C_1, NULL, NULL,
};
u16 gShopStockNpc2D_0[24] = {
#include "lobby/0080BAD0.inc"
};
u16 gShopStockNpc2D_1[24] = {
#include "lobby/0080BB00.inc"
};
u16 *gShopStockNpc2D[4] = {
    gShopStockNpc2D_0, gShopStockNpc2D_1, NULL, NULL,
};
u16 gShopStockNpc2E_0[24] = {
#include "lobby/0080BB40.inc"
};
u16 gShopStockNpc2E_1[28] = {
#include "lobby/0080BB70.inc"
};
u16 *gShopStockNpc2E[2] = {
    gShopStockNpc2E_0, gShopStockNpc2E_1,
};
u32 gShopCondStockNpc39_3[24] = {
#include "lobby/0080BBB0.inc"
};
u32 gShopCondStockNpc39_2[28] = {
#include "lobby/0080BC10.inc"
};
u32 gShopCondStockNpc39_1[28] = {
#include "lobby/0080BC80.inc"
};
u32 gShopCondStockNpc39_0[32] = {
#include "lobby/0080BCF0.inc"
};
ShopStockCond gShopCondStockNpc39[4] = {
#include "lobby/0080BD70.inc"
};
u32 gShopCondStockNpc42_0[14] = {
#include "lobby/0080BD90.inc"
};
ShopStockCond gShopCondStockNpc42[1] = {
    {0, (void *)gShopCondStockNpc42_0},
};
u16 gShopStockNpc50[72] = {
#include "lobby/0080BDD0.inc"
};
u16 gShopStockNpc50Ev122[72] = {
#include "lobby/0080BE60.inc"
};
u32 gShopCondStockNpc5D_0[20] = {
#include "lobby/0080BEF0.inc"
};
ShopStockCond gShopCondStockNpc5D[2] = {
    {0, (void *)gShopCondStockNpc5D_0},
    {0, NULL},
};
u32 gShopCondStockNpc6A_1[28] = {
#include "lobby/0080BF50.inc"
};
u32 gShopCondStockNpc6A_0[28] = {
#include "lobby/0080BFC0.inc"
};
ShopStockCond gShopCondStockNpc6A[2] = {
#include "lobby/0080C030.inc"
};
/* the lobby's common words and marks, read by many screens */
char *gLobbyWords[20] = {
    D_0083BA90, D_0083BA98, D_0083BAA0, D_0083BAA8,
    D_0083BAB0, D_0083BAB8, D_0083BAC0, D_0083BAC8,
    D_0083BAD0, D_0083BAE0, D_0083BAF8, D_0083BB00,
    D_0083BB08, D_0083BB10, D_0083BB18, D_0083BB28,
    D_0083BB30, D_0083BB38, D_0083BB40, D_0083BB48,
};
/* "none" and "set", indexed by whether a password is set */
char *gNoneSetTexts[2] = {
    D_0083BB50, D_0083BB58,
};
/* the column heads of a count */
char *gCountLabels[2] = {
    D_0083BB60, D_0083BB68,
};
/* the lobby game menu's entries and guide lines, then the price board's unknown name and title */
char *gGameMenuLabels[32] = {
    D_0083BB70, D_0083BB78, D_0083BB88, D_0083BB98,
    D_0083BBA8, D_0083BBC0, D_0083BBD8, D_0083BBE0,
    D_0083BBF0, D_0083BC00, D_0083BC10, D_0083BC18,
    D_0083BC28, D_0083BC38, D_0083BC48, D_0083BC50,
    D_0083BC60, D_0083BC70, D_0083BC80, D_0083BC90,
    D_0083BCA0, D_0083BCB0, D_0083BCC8, D_0083BCD8,
    D_0083BCE8, D_0083BD00, D_0083BD18, D_0083BD28,
    D_0083BD40, D_0083BD58, D_0083BD68, D_0083BD78,
};
/* the monster price board's lines */
char *gPriceInfoTexts[12] = {
    D_0083BD90, D_0083BDB0, D_0083BDD0, D_0083BDF0,
    D_0083BE08, D_0083BE18, D_0083BE28, D_0083BB50,
    D_0083BE38, D_0083BE48, D_0083BE58, D_0083BE68,
};
/* the titles and guides of the town's selection windows */
char *gListTitles[16] = {
    D_0083BE70, D_0083BE78, D_0083BE90, D_0083BEA0,
    D_0083BEB0, D_0083BEC0, D_0083BED0, D_0083BEE0,
    D_0083BEF0, D_0083BF00, D_0083BF10, D_0083BF20,
    D_0083BF30, D_0083BF40, D_0083BF50, D_0083BF60,
};
/* the quest board's rows: the desks, the quest kinds, the taker */
char *gQuestBoardRows[12] = {
    D_0083BF70, D_0083BF80, D_0083BF90, D_0083BFA0,
    D_0083BFB0, D_0083BFD0, D_0083BFE8, D_0083BFF8,
    D_0083C008, D_0083C018, D_0083C030, D_0083C050,
};
/* labels and help lines of the town's small windows */
char *gTownLabels[24] = {
    D_0083C060, D_0083C078, D_0083C080, D_0083C088,
    D_0083C098, D_0083C0A0, D_0083C0B8, D_0083C0D0,
    D_0083C0E8, D_0083C100, D_0083C120, D_0083C150,
    D_0083C170, D_0083C180, D_0083C1A0, D_0083BBD8,
    D_0083C1A8, D_0083C1B8, D_0083C1D0, D_0083C1F0,
    D_0083C208, D_0083C218, D_0083C228, D_0083C240,
};
char *gAreaNameTexts[16] = {
    D_0083C258, D_0083C260, D_0083C270, D_0083C280,
    D_0083C290, D_0083C2A0, D_0083C2A8, D_0083C2B8,
    D_0083C2B8, D_0083C2B8, D_0083C2C0, D_0083C2D0,
    D_0083C2D0, D_0083C2D0, D_0083C2D0, D_0083C2E0,
};
/* the ship menu's entries, drawn as a list */
char *gShipMenuTexts[2] = {
    D_0083C2F0, D_0083C300,
};
/* the town guide's groups, by group */
char *gTownGuideTopics[3] = {
    D_0083C310, D_0083C320, D_0083C328,
};
/* the town guide's help lines by group, then its scroll arrows */
char *gTownGuideHelp[8] = {
    D_0083C340, D_0083C370, D_0083C3A0, D_0083C3C8,
    D_0083C3D0, NULL, NULL, NULL,
};
/* the village board's title lines and prompts */
char *gVillageBoardTitles[4] = {
    D_0083C3E0, D_0083C400, D_0083C440, D_0083C470,
};
/* the village board's article kinds by kind from 0, then each kind's description from 5 */
char *gVillageBoardTexts[12] = {
    D_0083C498, D_0083C4B0, D_0083C4C8, D_0083C4D8,
    D_0083C4E8, D_0083C500, D_0083C520, D_0083C540,
    D_0083C560, D_0083C580, NULL, NULL,
};
/* the elder dragon forecast's menu, from 0, then its panel's labels */
char *gElderForecastTexts[8] = {
    D_0083C598, D_0083C5A8, D_0083BC60, D_0083C598,
    D_0083C5B8, D_0083C5C8, D_0083C5D0, D_0083C5D8,
};
/* the hair style menu's labels */
char *gHairMenuTexts[8] = {
    D_0083C5E0, D_0083C5F0, D_0083C610, D_0083C630,
    D_0083C660, NULL, NULL, NULL,
};
/* the records menu's pages, by RecordKind from 0, then its prompts */
char *gRecordsMenuTexts[24] = {
    D_0083C690, D_0083C6A8, D_0083C6B8, D_0083C6C8,
    D_0083C6D8, D_0083C6E8, D_0083C6F8, D_0083C700,
    D_0083C710, NULL, NULL, NULL,
    D_0083C730, D_0083C760, D_0083C790, D_0083C7C0,
    D_0083C810, D_0083C830, D_0083C860, D_0083C890,
    D_0083C8B0, NULL, NULL, NULL,
};
/* the house access page's rows, by HouseAccessRow */
char *gHouseAccessTexts[4] = {
    D_0083C8D0, D_0083C8E0, D_0083C8F0, D_0083C908,
};
/* the records' title genres, item kinds and hunt lines */
char *gRecordsListTexts[24] = {
    D_0083C918, D_0083C928, D_0083C930, D_0083C940,
    D_0083C950, D_0083C960, D_0083C980, D_0083C9A0,
    gHunterNotesTexts_8, D_0083C9D0, D_0083C9E8, D_0083C9F0,
    D_0083C9F8, D_0083CA08, D_0083CA10, D_0083CA28,
    D_0083CA38, D_0083C960, D_0083C980, D_0083CA40,
    D_0083CA60, D_0083CA70, D_0083CA90, D_0083CAB0,
};
/* the song menu's labels */
char *gSongMenuTexts[4] = {
    D_0083CAC8, D_0083CAD8, D_0083CAE8, NULL,
};
/* the village offer menu's labels */
char *gOfferMenuTexts[10] = {
    D_0083CAF0, D_0083CB00, D_0083CB10, D_0083CB20,
    D_0083CB30, D_0083CB38, D_0083CB50, D_0083CB80,
    D_0083CBD0, D_0083CC00,
};
/* how long an offer stays, by period */
char *gQuestPeriodTexts[2] = {
    D_0083CC20, D_0083CC30,
};
/* the offered quest's help lines */
char *gOfferHelpTexts[8] = {
    D_0083CC40, D_0083CC70, D_0083CCC0, D_0083CD10,
    D_0083CD40, NULL, NULL, NULL,
};
/* the hunter dojo's menus and titles */
char *gDojoMenuTexts[16] = {
    D_0083CD90, D_0083CDB0, D_0083C300, D_0083CDC8,
    D_0083CDD8, D_0083CDE8, D_0083CDF8, D_0083CE08,
    D_0083CE18, D_0083CE30, D_0083CE60, D_0083CE80,
    D_0083CEB0, D_0083CEC8, D_0083CED8, D_0083CEF0,
};
/* the network screens' labels: the group and area boards, the searches, the
 * friend and block lists, the hunter info and the short mail */
char *gNetLabels[52] = {
    D_0083CF08, D_0083CF18, D_0083CF20, D_0083BB60,
    D_0083CF28, D_0083CF30, D_0083CF38, D_0083CF40,
    D_0083CF48, D_0083CF58, D_0083CF60, D_0083CF68,
    D_0083CF80, D_0083CFC0, D_0083CFD0, D_0083CFE0,
    D_0083CB38, D_0083CFE8, D_0083D000, D_0083D018,
    D_0083CF08, D_0083D028, D_0083D030, D_0083D040,
    D_0083CFD0, D_0083D050, D_0083D060, D_0083D070,
    D_0083D088, D_0083D0A0, D_0083D0B8, D_0083D0C8,
    D_0083D0D8, D_0083D0E8, D_0083BC80, D_0083D0F8,
    D_0083D100, D_0083D118, D_0083D128, D_0083D130,
    D_0083D140, D_0083D158, D_0083D168, D_0083D170,
    D_0083D180, D_0083D188, D_0083D190, D_0083BC18,
    D_0083D198, D_0083D1B0, D_0083D220, D_0083D280,
};
/* the group search form's help line */
char *gGroupSearchHelpTexts[2] = {
    D_0083D2B0, NULL,
};
/* the hunter info's actions, by action */
char *gMemberActTexts[3] = {
    D_0083D310, D_0083D318, D_0083D328,
};
/* the mailbox's actions, by action */
char *gMailActTexts[3] = {
    D_0083D338, D_0083D318, D_0083D340,
};
/* a group's limit, by GroupLimit, then the same in short from 3 */
char *gGroupLimitTexts[8] = {
    D_0083BB50, D_0083D350, D_0083D360, D_0083BB50,
    D_0083D370, D_0083D378, NULL, NULL,
};
/* the search form's limit choices for a new hunter, by rank */
char *gGroupLimitNewTexts[4] = {
    D_0083D380, D_0083BB50, D_0083D350, D_0083D388,
};
/* the search form's limit choices for an expert, by rank */
char *gGroupLimitExpTexts[4] = {
    D_0083D380, D_0083BB50, D_0083D360, D_0083D398,
};
/* the arena cup's entry form, records, monster list and monster pages */
char *gArenaCupTexts[40] = {
    D_0083D3A8, D_0083D3B8, D_0083D3C8, D_0083D3D8,
    D_0083D170, D_0083D3E8, D_0083D3F8, D_0083D400,
    D_0083D420, D_0083D440, D_0083D458, D_0083D468,
    D_0083D478, D_0083D488, D_0083D498, D_0083D4A8,
    D_0083D4B8, D_0083CF30, D_0083D4C0, D_0083D380,
    D_0083D4D0, D_0083D500, D_0083D520, D_0083D540,
    D_0083D550, D_0083D560, D_0083D570, D_0083D590,
    D_0083CF30, D_0083D4C0, D_0083D5A0, D_0083D5A8,
    D_0083D4B8, D_0083D5B0, D_0083D5B8, D_0083D5C8,
    D_0083D5D8, D_0083D5E8, D_0083D600, D_0083D620,
};
/* the arena feed list's lines, then the digits */
char *gArenaFeedTexts[14] = {
    D_0083D680, D_0083D700, NULL, NULL,
    D_0083D718, D_0083D720, D_0083D728, D_0083D730,
    D_0083D738, D_0083D740, D_0083D748, D_0083D750,
    D_0083D758, D_0083D760,
};
/* two marks: the quest sheet's star and the page counter's slash */
char *gLobbyMarks[2] = {
    D_0083D768, D_0083D770,
};
/* the member counts by count - 1 (one to four), then two labels */
char *gMemberCountTexts[6] = {
    D_0083D778, D_0083D780, D_0083D788, D_0083D790,
    D_0083D7A0, D_0083D7B8,
};
/* two unrelated lines that share one 8-byte object */
char *gMiscTexts[2] = {
    D_0083D7C0, D_0083D7E0,
};
/* the ID menu's labels */
char *gIdMenuTexts[2] = {
    D_0083D800, D_0083D820,
};
/* the equipment shop's two lists: weapons, armour */
char *gWeaponArmorTexts[2] = {
    D_0083CF58, D_0083D838,
};
/* the equipment groups: the weapon classes, then the armour parts */
char *gEquipGroupTexts[12] = {
    D_0083D840, D_0083D850, D_0083D860, D_0083D880,
    D_0083D898, D_0083D8A8, D_0083D8B8, D_0083D8C8,
    D_0083D8D8, D_0083D8E8, NULL, NULL,
};
/* who can wear a piece, by the piece's wearer limit */
char *gEquipLimitTexts[8] = {
    D_0083D8F8, D_0083D908, D_0083D918, D_0083D928,
    D_0083D940, D_0083D960, D_0083D978, D_0083D988,
};
/* the equip-now and send-home choice, by the yes/no cursor */
char *gEquipNowTexts[4] = {
    D_0083D9A0, D_0083D9D0, NULL, NULL,
};
/* the equipment shop's notes and questions */
char *gEquipShopTexts[12] = {
    D_0083DA00, D_0083DA50, D_0083DAA0, D_0083DAD0,
    D_0083DAF0, D_0083DB20, D_0083DB70, D_0083DBB0,
    D_0083DBE0, D_0083DC30, NULL, NULL,
};
/* the workshop's first menu */
char *gSmithyMenuTexts[4] = {
    D_0083DC78, D_0083DC88, D_0083DC98, D_0083C300,
};
/* the workshop's make menu, then the weapon forge's empty-list line */
char *gWorkshopTexts[4] = {
    D_0083DCA0, D_0083DCB0, D_0083C300, D_0083DCC0,
};
/* the armour forge's empty-list line */
char *gNoArmorToMakeTexts[4] = {
    D_0083DCE0, NULL, NULL, NULL,
};
/* the weapon forge's titles, by its choice: make, upgrade */
char *gWeaponForgeTexts[2] = {
    D_0083DCA0, D_0083DD00,
};
/* the armour forge's titles */
char *gArmorForgeTexts[2] = {
    D_0083DCB0, D_0083DD10,
};
/* the decorations counter's titles, then the make menu's parts */
char *gDecoMenuTexts[8] = {
    D_0083DD20, D_0083DD30, D_0083DD40, D_0083DD50,
    D_0083DD60, D_0083DD70, D_0083DD80, D_0083DD90,
};
/* the weapon classes, by the forge's row */
char *gWeaponClassTexts[8] = {
    D_0083D840, D_0083D850, D_0083D860, D_0083D880,
    D_0083D898, NULL, NULL, NULL,
};
/* the forges' and the decorations counter's questions, headers and notes */
char *gForgeTexts[68] = {
    D_0083DDA0, D_0083DDC0, D_0083C630, D_0083DDD8,
    D_0083DDD8, D_0083DDF0, D_0083DE30, D_0083DE70,
    D_0083DEB0, D_0083DAD0, D_0083DEE0, D_0083DF30,
    D_0083DF90, D_0083DFB0, D_0083DFD0, D_0083DFE8,
    D_0083DFF8, D_0083E008, D_0083E020, D_0083E038,
    D_0083E050, D_0083E070, D_0083E090, D_0083E0B0,
    D_0083E0D0, D_0083E0F0, D_0083E108, D_0083E118,
    D_0083E130, D_0083E150, D_0083E170, D_0083E1C0,
    D_0083E260, D_0083E2C0, D_0083E350, D_0083E3B0,
    D_0083E420, D_0083E490, D_0083E530, D_0083E590,
    D_0083E610, D_0083E670, D_0083E6A0, D_0083E6F0,
    D_0083E740, D_0083E790, D_0083E7E0, D_0083E830,
    D_0083E880, D_0083E8D0, D_0083E930, D_0083E9A0,
    D_0083E9D0, D_0083EA00, D_0083EA30, D_0083EA50,
    D_0083EA90, D_0083EAB0, D_0083EB00, D_0083EB70,
    D_0083EBC0, D_0083EC00, D_0083EC30, D_0083DDC0,
    D_0083EC60, D_0083EC80, NULL, NULL,
};
/* the kitchen's labels, questions and stat-change lines */
char *gKitchenTexts[20] = {
    D_0083EC98, D_0083ECA8, D_0083ECC0, D_0083ECD8,
    D_0083ECF0, D_0083ED10, D_0083ED30, D_0083ED50,
    D_0083ED70, D_0083ED90, D_0083EDA8, D_0083EDB0,
    D_0083EDC0, D_0083EDE0, D_0083EE00, D_0083EE38,
    D_0083EE50, D_0083EE68, NULL, NULL,
};
char *gMealNameTexts[16] = {
    D_0083EE80, D_0083EE98, D_0083EEA8, D_0083EEC0,
    D_0083EED8, D_0083EEE8, D_0083EEF8, D_0083EF08,
    D_0083EF18, D_0083EF28, D_0083EF38, D_0083EF48,
    D_0083EF58, D_0083EF68, D_0083EF78, D_0083EF90,
};
char *gMealTasteTexts_0[4] = {
    D_0083EFB0, D_0083EFD0, D_0083EFF0, D_0083F010,
};
char *gMealTasteTexts_1[4] = {
    D_0083F030, D_0083F050, D_0083F070, D_0083F090,
};
char *gMealTasteTexts_2[4] = {
    D_0083F0B0, D_0083F0D0, D_0083F0F0, D_0083F110,
};
char *gMealTasteTexts_3[4] = {
    D_0083F130, D_0083F130, D_0083F130, D_0083F130,
};
char *gMealTasteTexts_4[4] = {
    D_0083F140, D_0083F150, D_0083F170, D_0083F190,
};
char *gMealTasteTexts_5[4] = {
    D_0083F1B0, D_0083F1D0, D_0083F1F0, D_0083F210,
};
char *gMealTasteTexts_6[4] = {
    D_0083F230, D_0083F250, D_0083F270, D_0083F290,
};
char **gMealTasteTexts[8] = {
    gMealTasteTexts_0, gMealTasteTexts_1, gMealTasteTexts_2, gMealTasteTexts_3,
    gMealTasteTexts_4, gMealTasteTexts_5, gMealTasteTexts_6, NULL,
};
/* the item box's first menu, by action */
char *gBoxActionTexts[6] = {
    D_0083F2B0, D_0083F2D0, D_0083F2E8, D_0083F300,
    D_0083F318, NULL,
};
/* the item box's control guide */
char *gBoxControlTexts[2] = {
    D_0083F328, D_0083F338,
};
/* the control guide's show-the-menu line */
char *gMenuShowTexts[2] = {
    D_0083F348, NULL,
};
/* the box's two lists: items, equipment */
char *gBoxKindTexts[2] = {
    D_0083F358, D_0083F368,
};
/* the equipment menu's entries */
char *gEquipMenuTexts[8] = {
    D_0083F370, D_0083F380, D_0083F3A0, D_0083F3C0,
    D_0083F3D8, NULL, NULL, NULL,
};
/* the equipment slots' names, by part */
char *gEquipSlotTexts[8] = {
    D_0083CF58, D_0083F3E8, D_0083F3F0, D_0083F3F8,
    D_0083F400, D_0083F408, NULL, NULL,
};
/* the item box's labels and formats */
char *gBoxMenuTexts[24] = {
    D_0083F410, D_0083F430, D_0083F450, D_0083F470,
    D_0083F480, D_0083F4A0, D_0083F4B0, D_0083F4C0,
    D_0083F4E0, D_0083F4F0, D_0083F510, D_0083F530,
    D_0083F540, D_0083F550, D_0083F560, D_0083F570,
    D_0083F590, D_0083F5A8, D_0083F5C0, D_0083F5E0,
    D_0083F600, D_0083F618, NULL, NULL,
};
/* the item box's help lines, by message (the field menus' help lists hold it too) */
char *gBoxHelpTexts[72] = {
    D_0083F620, D_0083F650, D_0083F670, D_0083F6A0,
    D_0083F6D0, D_0083F700, D_0083F720, D_0083F750,
    D_0083F770, gHunterNotesTexts_8, D_0083F7A0, D_0083F7C0,
    D_0083F810, D_0083F830, D_0083F860, D_0083F880,
    D_0083F8A0, D_0083F8B8, D_0083F8D0, D_0083F8F0,
    D_0083F910, D_0083F940, D_0083F970, D_0083F9A0,
    D_0083F9C0, D_0083F9E0, D_0083FA00, D_0083FA30,
    D_0083F880, D_0083FA78, D_0083FA90, D_0083FAB0,
    D_0083FAE0, D_0083FB10, D_0083FB40, D_0083FB70,
    D_0083FB90, D_0083FBB0, D_0083FBE0, D_0083FC10,
    D_0083FC30, D_0083FC70, D_0083FCA0, D_0083FCB0,
    D_0083FCD0, D_0083FD20, D_0083FD40, D_0083FD60,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_0083FD80, D_0083FDB0,
    D_0083FDD0, D_0083FE10, D_0083FE50, D_0083FE80,
    D_0083FEB0, gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8,
    D_0083FED0, D_0083FF10, D_0083FF50, D_0083FF90,
    D_0083FFC0, D_00840000, D_00840040, D_00840080,
    D_008400B0, D_008400E0, D_00840120, D_00840160,
};
char *gAreaChatTexts[4] = {
    D_00840190, D_008401A8, D_008401B0, D_008401B8,
};
/* the quest counter's board rows, room pages and arena teams */
char *gQuestCounterTexts[56] = {
    D_0083BFE8, D_0083C050, D_008401C0, D_008401D0,
    D_008401D8, D_008401E0, D_008401F8, D_00840208,
    D_00840220, D_00840240, D_00840258, D_00840268,
    D_0083D7A0, D_00840270, D_00840280, D_00840288,
    D_00840298, D_0083D3D8, D_008402A8, D_008402B8,
    D_008402C8, D_008402D8, D_008402E0, D_008402F8,
    D_00840308, D_00840318, D_00840328, D_00840338,
    D_00840348, D_00840360, D_00840378, D_00840390,
    D_008403B0, D_008403D0, D_008403F0, D_00840410,
    D_00840430, D_00840450, D_00840470, D_00840490,
    D_008404B0, D_008404D0, D_008404F0, D_00840510,
    D_00840528, D_00840538, D_00840550, D_00840568,
    D_00840578, D_00840588, D_008405A0, D_008405D0,
    D_008405F0, D_00840620, NULL, NULL,
};
char *gInfoIndexTexts[8] = {
    D_00840638, D_0083C328, D_00840648, D_00840658,
    D_00840668, NULL, NULL, NULL,
};
/* the quest sheets' and the room forms' labels */
char *gQuestSheetTexts[36] = {
    D_0083BFA0, D_00840678, D_00840688, D_008406A0,
    D_008406C0, D_008406D8, D_008406E8, D_008406F8,
    D_00840708, D_00840718, D_00840728, D_00840740,
    D_00840760, D_008402B8, D_00840778, D_0083D3D8,
    D_00840788, D_00840798, D_00840328, D_00840338,
    D_0083CB20, D_008407A8, D_008407C0, D_008407D8,
    D_008407E8, D_008407F8, D_0083C5B8, D_00840808,
    D_00840818, D_008402E0, D_00840828, D_00840838,
    D_008402F8, D_00840638, D_00840850, D_00840870,
};
char *gOfficialEventTexts[4] = {
    D_00840888, D_00840898, D_008408A8, D_008408B8,
};
/* the game menu's entries in a hall (GAME_MENU_HUB) */
char *gGameMenuHubTexts[12] = {
    D_0083F358, D_008408C8, D_0083BC80, D_008408D0,
    D_008408E0, D_0083BC50, D_0083BC10, D_008408F0,
    D_00840900, NULL, NULL, NULL,
};
/* the game menu's entries online (GAME_MENU_ONLINE) */
char *gGameMenuOnlineTexts[12] = {
    D_0083F358, D_008408C8, D_0083BC80, D_00840910,
    D_008408D0, D_008408E0, D_0083BC50, D_0083BC10,
    D_008408F0, D_00840900, NULL, NULL,
};
/* the same in the busy colours */
char *gGameMenuOnlineBusyTexts[12] = {
    D_0083F358, D_00840920, D_0083BC80, D_00840910,
    D_008408D0, D_008408E0, D_0083BC50, D_0083BC10,
    D_008408F0, D_00840930, NULL, NULL,
};
/* the game menu's entries offline (GAME_MENU_OFFLINE) */
char *gGameMenuOfflineTexts[12] = {
    D_0083F358, D_008408C8, D_0083BC80, D_00840910,
    D_008408D0, D_0083BCC8, D_008408F0, D_00840940,
    D_00840900, NULL, NULL, NULL,
};
/* the same in the busy colours */
char *gGameMenuOfflineBusyTexts[12] = {
    D_0083F358, D_00840920, D_0083BC80, D_00840910,
    D_008408D0, D_0083BCC8, D_008408F0, D_00840940,
    D_00840930, NULL, NULL, NULL,
};
/* the game menu's help lines, by entry (the field menus' help lists hold it too) */
char *gGameMenuHelpTexts[40] = {
    D_00840950, D_00840990, D_008409B0, D_008409E0,
    D_00840A10, D_00840A30, D_00840A70, D_00840AB0,
    D_00840AD0, D_00840AF0, D_00840B10, D_00840B30,
    gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8,
    D_00840B50, D_00840B70, D_00840B90, D_00840BB0,
    D_00840BF0, D_00840C40, D_00840C80, D_00840CC0,
    D_00840CF0, D_00840D20, gHunterNotesTexts_8, D_00840B30,
    D_00840D50, D_00840DB0, D_00840E10, D_00840E40,
    D_00840E70, D_00840EA0, D_00840EC0, D_00840F00,
    D_00840F60, D_00840F90, D_00840FD0, D_00841010,
};
/* the search menu's entries, which also title their pages */
char *gSearchMenuTexts[4] = {
    D_0083BC18, D_00841048, D_00841058, D_00841068,
};
char *gMailboxMenuTexts[4] = {
    D_00841078, D_00841088, D_00841098, D_0083C1B8,
};
char *gQuitMenuTexts[2] = {
    D_008410B0, D_008410C8,
};
char *gQuitMenuDimTexts[2] = {
    D_008410B0, D_008410E0,
};
char *gQuitSaveTexts[4] = {
    D_00841100, D_00841130, NULL, NULL,
};
char *gQuitConfirmTexts[4] = {
    D_00841160, D_008411B0, D_00841240, D_00841290,
};
/* the chat pages' titles and labels: the speakers, the muted players, the chat state */
char *gChatPageTexts[12] = {
    D_008412F8, D_00841310, D_00841330, D_00841340,
    D_00841350, D_00841370, D_00841390, D_008413B0,
    D_008413D0, D_008413F0, NULL, NULL,
};
char *gChatMenuTexts[6] = {
    D_0083BCD8, D_0083BCC8, D_00841410, D_00841420,
    D_00841430, D_00841450,
};
char *gChatStateTexts[2] = {
    D_00841470, D_00841490,
};
char *gChatUserActTexts[4][3] = {
    {D_0083BC80, D_008414A8, D_008414B8},
    {D_0083BC80, D_008414C8, D_008414D8},
    {D_0083BC80, D_008414F0, D_008414D8},
    {D_008414D8, NULL, NULL},
};
/* the friend limit's two settings: off, on */
char *gFriendLimitTexts[2] = {
    D_0083C218, D_0083C228,
};
/* the place info's labels */
char *gPlaceInfoTexts[12] = {
    D_00841508, D_00841510, D_0083D088, D_0083CFD0,
    D_0083D030, D_0083D040, D_00841520, D_00841530,
    D_00841540, D_00841548, D_00841550, D_00841558,
};
char *gTavernLevelTexts[4] = {
    D_00841560, D_00841580, D_008415A0, NULL,
};
char *gPortLevelTexts[4] = {
    D_008415C0, D_008415E0, D_00841600, NULL,
};
char *gSmithyLevelTexts[4] = {
    D_00841620, D_00841640, D_00841660, NULL,
};
char *gMineLevelTexts[4] = {
    D_00841680, D_008416A0, D_008416C0, NULL,
};
/* the online marks' names, by mark (ONLINE_MARK_*) */
char *gOnlineMarkNames[12] = {
    D_008416D8, D_00841750, D_00841770, D_00841790,
    D_008417B0, D_00841710, D_00841728, D_008416F0,
    D_00841738, NULL, NULL, NULL,
};
/* The member status texts, by status (MemberCard_DrawStatus). */
char *gMemberStatusTexts[12] = {
    D_008417C8, D_008417E0, D_00841800, D_00841820,
    D_00841838, D_00841850, D_00841860, D_00841870,
    D_00841888, NULL, NULL, NULL,
};
/* the shops' menu entries; the item shop's list starts at its second */
char *gShopMenuTexts[4] = {
    D_00841890, D_008418A0, D_008418A8, D_008418B0,
};
/* the shop list of buy, sell and request */
char *gShopBuySellTexts[4] = {
    D_008418A0, D_008418A8, D_0083C300, NULL,
};
/* the combine shop's menu */
char *gItemShopMenuTexts[4] = {
    D_00841890, D_008418A0, D_008418A8, D_0083C300,
};
/* the combine shop's labels */
char *gCombineShopTexts[12] = {
    D_008418B8, D_008418C8, D_008418D8, D_008418E8,
    D_008418F8, D_00841908, D_00841918, D_00841920,
    D_00841930, NULL, NULL, NULL,
};
/* the shops' and the gallery's questions and notes */
char *gShopPromptTexts[16] = {
    D_00841940, D_00841960, D_00841980, D_0083C630,
    D_0083C630, D_008419A0, D_008419D0, D_00841A00,
    D_00841A70, D_00841AA0, D_00841AD0, D_00841B40,
    D_00841B70, D_00841BA0, D_00841C10, D_00841C70,
};
char *gSongHelpTexts[4] = {
    D_00841CE0, D_00841D20, NULL, NULL,
};
char *gMemberCardPanels_0[10] = {
    D_00841D78, D_00841D88, D_00841D98, D_00841DA8,
    gHunterNotesTexts_8, D_00841DB8, D_00841DD0, D_00841DE8,
    D_00841E00, D_00841510,
};
char *gStatusTitleTexts[2] = {
    D_00841E18, D_00841E30,
};
char *gFriendRegTexts[4] = {
    D_00841E60, D_00841E80, NULL, NULL,
};
/* the bed's menu: its entries, their help lines, the living cost question */
char *gBedMenuTexts[8] = {
    D_00841EA0, D_00841EA8, D_00841EB0, D_00841EC0,
    D_00841F00, D_00841F60, D_00841FB0, NULL,
};
char *gMonsterClassTexts[76] = {
    D_00841FE0, D_00841FF0, D_00841FF8, gHunterNotesTexts_8,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_00842000, D_00841FF8,
    D_00842008, gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF0,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF0, D_00841FF0,
    gHunterNotesTexts_8, D_00841FF0, gHunterNotesTexts_8, gHunterNotesTexts_8,
    D_00842000, D_00842008, D_00841FF0, gHunterNotesTexts_8,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF0, D_00842000,
    D_00842000, gHunterNotesTexts_8, gHunterNotesTexts_8, D_00842000,
    gHunterNotesTexts_8, D_00841FF8, gHunterNotesTexts_8, gHunterNotesTexts_8,
    D_00841FF8, D_00841FF0, D_00842000, D_00842000,
    D_00842000, D_00841FF0, D_00841FF0, D_00841FF0,
    D_00841FF0, D_00841FF0, D_00842008, D_00841FF0,
    D_00842010, D_00841FF0, D_00841FF8, D_00842018,
    D_00842018, D_00842018, D_00841FF8, D_00842010,
    gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF8, D_00841FF8,
    D_00841FF8, gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF0,
    D_00841FF8, D_00841FF8, gHunterNotesTexts_8, D_00842010,
    D_00842018, gHunterNotesTexts_8, gHunterNotesTexts_8, D_00841FF8,
    gHunterNotesTexts_8, gHunterNotesTexts_8, NULL, NULL,
};
/* the shooting gallery's courses, by course, then its score and shot labels */
char *gGalleryCourseTexts[12] = {
    D_00842020, D_00842030, D_00842040, D_00842050,
    D_00842060, D_00842070, D_00842080, D_00842090,
    D_008420A0, D_008420A8, NULL, NULL,
};
/* the notes of the lists and searches that come up empty */
char *gEmptyListTexts[8] = {
    D_008420C0, D_008420E0, D_00842100, D_00842120,
    D_00842140, D_00842160, D_00841350, D_00842170,
};
char *gLobbyNoticeTexts[44] = {
    D_00842190, D_008421B0, D_008421D0, D_008421F0,
    D_00842210, D_00842230, D_00842260, D_00842290,
    D_008422C0, D_008422F0, D_00842320, D_00842340,
    D_00842360, D_00842380, D_008423A0, D_008423C0,
    D_008423E0, D_00842400, D_00842420, D_00842440,
    D_00842470, D_00842490, D_008424B0, D_008424D0,
    D_008424E0, D_00842500, D_00842530, D_00842560,
    D_00842580, D_008425B0, D_008425D8, D_008425E8,
    D_00842600, D_00842620, D_00842660, D_00842680,
    D_008426A0, D_008426D0, D_00842700, D_00842720,
    D_00842750, NULL, NULL, NULL,
};
char *gLobbyEventTexts[12] = {
    D_00842770, D_00842790, D_008427B0, D_008427D0,
    D_008427E8, D_008427F8, D_00842810, D_00842840,
    D_00842860, D_00842880, D_008428A0, D_008428B0,
};
char *gSpotActionTexts[28] = {
    D_008428C8, D_008428D8, D_008428E8, D_00842900,
    D_00842920, D_00842930, D_00842940, D_00842958,
    D_00842968, D_00842978, D_008428D8, D_00842988,
    D_00842990, D_008429A0, D_008429B8, D_008429C8,
    D_0083CF08, D_008429D8, D_008429E8, D_008429F8,
    D_00842A10, D_00842A28, D_00842A38, D_00842A40,
    D_00842A58, D_00842A70, D_00842A88, NULL,
};
/* the actions the members' name tags offer */
char *gNameTagActionTexts[4] = {
    D_00842A98, D_00842AB0, D_00842AD0, NULL,
};
char *gLobbyTitleTexts[8] = {
    D_00842AF0, D_00842AF8, D_00842B08, D_00842B18,
    D_00842B28, NULL, NULL, NULL,
};
/* the lobby dialogs' help lines, by help id, then the lists' column heads */
char *gLobbyHelpTexts[68] = {
    D_00842B40, D_00842B60, D_00842C00, D_00842C30,
    D_00842C60, D_00842C80, D_00842D00, D_00842D30,
    D_00842D50, D_00842D90, D_00842DC0, D_00842DF0,
    D_00840B10, D_00842E10, D_00842E40, D_00842EC0,
    D_00842F00, D_00842F30, D_00842F60, D_00842FC0,
    D_00842FE0, D_00843090, D_00843130, D_008431C0,
    D_00843260, D_00843290, D_008432D0, D_008432F0,
    D_00843340, D_008433E0, gHunterNotesTexts_8, D_00843480,
    D_008434B0, D_008434F0, D_00843520, D_008435B0,
    D_00843660, D_008436F0, D_00843730, D_00843770,
    D_008437B0, D_00843800, D_00840BF0, D_00843840,
    D_00843880, D_00840CC0, D_008438C0, D_008438F0,
    D_0083CF30, D_0083CF58, D_0083CF38, D_00843980,
    D_0083CF40, D_00843988, D_00843998, D_008439A8,
    D_0083BB60, D_0083CF28, D_0083C950, D_0083CF48,
    D_008439B0, D_008439B8, D_008439C0, D_008439C8,
    D_008439D8, D_008439E0, D_008439E8, D_008439F0,
};
/* the lobby dialogs' labels, titles and choices */
char *gLobbyLabelTexts[58] = {
    D_008439F8, D_00843A00, D_00843A08, D_0083D130,
    D_00843A18, D_0083D088, D_0083D170, D_0083BC80,
    D_00843A20, D_00843A28, D_00843A40, D_00843A60,
    D_00843A78, D_00843A88, D_00843AA0, D_00843AB8,
    D_00843AC8, D_00843AE0, D_00843AF8, D_00843998,
    D_00843B00, D_00843B10, D_00840318, D_00840828,
    D_008402B8, D_00840240, D_00843B20, D_00843B30,
    D_00843B40, D_00843B60, D_00843B80, D_00843BA0,
    D_00843BC0, D_00843BE0, D_00843C00, D_00843C20,
    D_00840318, D_00843C40, D_00843C48, D_00843C58,
    D_00843C68, D_00843C80, D_00843CA0, D_00843CC0,
    D_0083D060, D_00843CE0, D_00843D10, D_00843D30,
    D_00843D40, D_00843D50, D_00841920, D_00843D60,
    D_00843D70, D_00843D88, D_00843D98, D_00843DB0,
    D_00843DD0, D_00843DE8,
};
/* the lobby dialogs' no and save choices */
char *gLobbyNoSaveTexts[2] = {
    D_00843DF0, D_00843DF8,
};
/* the lobby dialogs' quit choice, states and button guide labels */
char *gLobbyButtonTexts[40] = {
    D_00843E10, D_00843E28, D_00843E30, D_0083C2E0,
    D_00843E40, D_0083C5B8, D_00843E50, D_00843E60,
    D_00843E70, D_00843E80, NULL, NULL,
    D_00843E90, D_00843EB0, D_00843ED0, D_00843EE0,
    D_0083CFE0, D_00843EF0, D_00843EF8, D_0083BC80,
    D_00843F08, D_0083D318, D_00843F18, D_00843F28,
    D_00843F38, D_00843A88, D_00843F48, D_00843F58,
    D_00843F68, D_00843F70, D_00843F88, D_0083D328,
    D_00843F90, D_00843FA0, D_008402C8, D_00843FB0,
    D_00841410, D_00843FC0, D_00843FD0, NULL,
};
LobbyMsgDef gLobbyMsgTexts[152] = {
#include "lobby/0080D7B0.inc"
};

/* The file's .rodata: the label tables' strings. */
#pragma rodata on
char D_0083BA90[0x8] = {
#include "lobby/0083BA90.inc"
};
char D_0083BA98[0x8] = {
#include "lobby/0083BA98.inc"
};
char D_0083BAA0[0x8] = {
#include "lobby/0083BAA0.inc"
};
char D_0083BAA8[0x8] = {
#include "lobby/0083BAA8.inc"
};
char D_0083BAB0[0x8] = {
#include "lobby/0083BAB0.inc"
};
char D_0083BAB8[0x8] = {
#include "lobby/0083BAB8.inc"
};
char D_0083BAC0[0x8] = {
#include "lobby/0083BAC0.inc"
};
char D_0083BAC8[0x8] = {
#include "lobby/0083BAC8.inc"
};
char D_0083BAD0[0x10] = {
#include "lobby/0083BAD0.inc"
};
char D_0083BAE0[0x18] = {
#include "lobby/0083BAE0.inc"
};
char D_0083BAF8[0x8] = {
#include "lobby/0083BAF8.inc"
};
char D_0083BB00[0x8] = {
#include "lobby/0083BB00.inc"
};
char D_0083BB08[0x8] = {
#include "lobby/0083BB08.inc"
};
char D_0083BB10[0x8] = {
#include "lobby/0083BB10.inc"
};
char D_0083BB18[0xC] = {
#include "lobby/0083BB18.inc"
};
char D_0083BB28[0x8] = {
#include "lobby/0083BB28.inc"
};
char D_0083BB30[0x8] = {
#include "lobby/0083BB30.inc"
};
char D_0083BB38[0x8] = {
#include "lobby/0083BB38.inc"
};
char D_0083BB40[0x8] = {
#include "lobby/0083BB40.inc"
};
char D_0083BB48[0x8] = {
#include "lobby/0083BB48.inc"
};
char D_0083BB50[0x8] = {
#include "lobby/0083BB50.inc"
};
char D_0083BB58[0x8] = {
#include "lobby/0083BB58.inc"
};
char D_0083BB60[0x8] = {
#include "lobby/0083BB60.inc"
};
char D_0083BB68[0x8] = {
#include "lobby/0083BB68.inc"
};
char D_0083BB70[0x8] = {
#include "lobby/0083BB70.inc"
};
char D_0083BB78[0xC] = {
#include "lobby/0083BB78.inc"
};
char D_0083BB88[0xC] = {
#include "lobby/0083BB88.inc"
};
char D_0083BB98[0xC] = {
#include "lobby/0083BB98.inc"
};
char D_0083BBA8[15] = {
#include "lobby/0083BBA8.inc"
};
char D_0083BBC0[0x18] = {
#include "lobby/0083BBC0.inc"
};
char D_0083BBD8[0x8] = {
#include "lobby/0083BBD8.inc"
};
char D_0083BBE0[0x10] = {
#include "lobby/0083BBE0.inc"
};
char D_0083BBF0[0x10] = {
#include "lobby/0083BBF0.inc"
};
char D_0083BC00[0x10] = {
#include "lobby/0083BC00.inc"
};
char D_0083BC10[0x8] = {
#include "lobby/0083BC10.inc"
};
char D_0083BC18[15] = {
#include "lobby/0083BC18.inc"
};
char D_0083BC28[15] = {
#include "lobby/0083BC28.inc"
};
char D_0083BC38[13] = {
#include "lobby/0083BC38.inc"
};
char D_0083BC48[0x8] = {
#include "lobby/0083BC48.inc"
};
char D_0083BC50[0x10] = {
#include "lobby/0083BC50.inc"
};
char D_0083BC60[0x10] = {
#include "lobby/0083BC60.inc"
};
char D_0083BC70[0x10] = {
#include "lobby/0083BC70.inc"
};
char D_0083BC80[0x10] = {
#include "lobby/0083BC80.inc"
};
char D_0083BC90[0x10] = {
#include "lobby/0083BC90.inc"
};
char D_0083BCA0[0x10] = {
#include "lobby/0083BCA0.inc"
};
char D_0083BCB0[0x18] = {
#include "lobby/0083BCB0.inc"
};
char D_0083BCC8[0xC] = {
#include "lobby/0083BCC8.inc"
};
char D_0083BCD8[13] = {
#include "lobby/0083BCD8.inc"
};
char D_0083BCE8[0xC] = {
#include "lobby/0083BCE8.inc"
};
char D_0083BD00[0x18] = {
#include "lobby/0083BD00.inc"
};
char D_0083BD18[0xC] = {
#include "lobby/0083BD18.inc"
};
char D_0083BD28[13] = {
#include "lobby/0083BD28.inc"
};
char D_0083BD40[0x18] = {
#include "lobby/0083BD40.inc"
};
char D_0083BD58[15] = {
#include "lobby/0083BD58.inc"
};
char D_0083BD68[13] = {
#include "lobby/0083BD68.inc"
};
char D_0083BD78[0xC] = {
#include "lobby/0083BD78.inc"
};
char D_0083BD90[0x20] = {
#include "lobby/0083BD90.inc"
};
char D_0083BDB0[0x20] = {
#include "lobby/0083BDB0.inc"
};
char D_0083BDD0[0x20] = {
#include "lobby/0083BDD0.inc"
};
char D_0083BDF0[0x18] = {
#include "lobby/0083BDF0.inc"
};
char D_0083BE08[13] = {
#include "lobby/0083BE08.inc"
};
char D_0083BE18[0xC] = {
#include "lobby/0083BE18.inc"
};
char D_0083BE28[13] = {
#include "lobby/0083BE28.inc"
};
char D_0083BE38[0xC] = {
#include "lobby/0083BE38.inc"
};
char D_0083BE48[0xC] = {
#include "lobby/0083BE48.inc"
};
char D_0083BE58[15] = {
#include "lobby/0083BE58.inc"
};
char D_0083BE68[0x8] = {
#include "lobby/0083BE68.inc"
};
char D_0083BE70[0x8] = {
#include "lobby/0083BE70.inc"
};
char D_0083BE78[0xC] = {
#include "lobby/0083BE78.inc"
};
char D_0083BE90[0x10] = {
#include "lobby/0083BE90.inc"
};
char D_0083BEA0[0x10] = {
#include "lobby/0083BEA0.inc"
};
char D_0083BEB0[0x10] = {
#include "lobby/0083BEB0.inc"
};
char D_0083BEC0[0x10] = {
#include "lobby/0083BEC0.inc"
};
char D_0083BED0[0x10] = {
#include "lobby/0083BED0.inc"
};
char D_0083BEE0[0x10] = {
#include "lobby/0083BEE0.inc"
};
char D_0083BEF0[0x10] = {
#include "lobby/0083BEF0.inc"
};
char D_0083BF00[0x10] = {
#include "lobby/0083BF00.inc"
};
char D_0083BF10[0x10] = {
#include "lobby/0083BF10.inc"
};
char D_0083BF20[0x10] = {
#include "lobby/0083BF20.inc"
};
char D_0083BF30[0x10] = {
#include "lobby/0083BF30.inc"
};
char D_0083BF40[0x10] = {
#include "lobby/0083BF40.inc"
};
char D_0083BF50[0x10] = {
#include "lobby/0083BF50.inc"
};
char D_0083BF60[0x10] = {
#include "lobby/0083BF60.inc"
};
char D_0083BF70[0x10] = {
#include "lobby/0083BF70.inc"
};
char D_0083BF80[0x10] = {
#include "lobby/0083BF80.inc"
};
char D_0083BF90[0x10] = {
#include "lobby/0083BF90.inc"
};
char D_0083BFA0[0x10] = {
#include "lobby/0083BFA0.inc"
};
char D_0083BFB0[0x20] = {
#include "lobby/0083BFB0.inc"
};
char D_0083BFD0[0x18] = {
#include "lobby/0083BFD0.inc"
};
char D_0083BFE8[0xC] = {
#include "lobby/0083BFE8.inc"
};
char D_0083BFF8[15] = {
#include "lobby/0083BFF8.inc"
};
char D_0083C008[0xC] = {
#include "lobby/0083C008.inc"
};
char D_0083C018[0xC] = {
#include "lobby/0083C018.inc"
};
char D_0083C030[0x20] = {
#include "lobby/0083C030.inc"
};
char D_0083C050[0x10] = {
#include "lobby/0083C050.inc"
};
char D_0083C060[0x18] = {
#include "lobby/0083C060.inc"
};
char D_0083C078[0x8] = {
#include "lobby/0083C078.inc"
};
char D_0083C080[0x8] = {
#include "lobby/0083C080.inc"
};
char D_0083C088[0xC] = {
#include "lobby/0083C088.inc"
};
char D_0083C098[0x8] = {
#include "lobby/0083C098.inc"
};
char D_0083C0A0[0x18] = {
#include "lobby/0083C0A0.inc"
};
char D_0083C0B8[13] = {
#include "lobby/0083C0B8.inc"
};
char D_0083C0D0[0x18] = {
#include "lobby/0083C0D0.inc"
};
char D_0083C0E8[15] = {
#include "lobby/0083C0E8.inc"
};
char D_0083C100[0x20] = {
#include "lobby/0083C100.inc"
};
char D_0083C120[0x30] = {
#include "lobby/0083C120.inc"
};
char D_0083C150[0x20] = {
#include "lobby/0083C150.inc"
};
char D_0083C170[0x10] = {
#include "lobby/0083C170.inc"
};
char D_0083C180[0x20] = {
#include "lobby/0083C180.inc"
};
char D_0083C1A0[0x8] = {
#include "lobby/0083C1A0.inc"
};
char D_0083C1A8[0xC] = {
#include "lobby/0083C1A8.inc"
};
char D_0083C1B8[0xC] = {
#include "lobby/0083C1B8.inc"
};
char D_0083C1D0[0x20] = {
#include "lobby/0083C1D0.inc"
};
char D_0083C1F0[0x18] = {
#include "lobby/0083C1F0.inc"
};
char D_0083C208[0xC] = {
#include "lobby/0083C208.inc"
};
char D_0083C218[0xC] = {
#include "lobby/0083C218.inc"
};
char D_0083C228[0xC] = {
#include "lobby/0083C228.inc"
};
char D_0083C240[0x18] = {
#include "lobby/0083C240.inc"
};
char D_0083C258[0x8] = {
#include "lobby/0083C258.inc"
};
char D_0083C260[0x10] = {
#include "lobby/0083C260.inc"
};
char D_0083C270[0x10] = {
#include "lobby/0083C270.inc"
};
char D_0083C280[0x10] = {
#include "lobby/0083C280.inc"
};
char D_0083C290[0x10] = {
#include "lobby/0083C290.inc"
};
char D_0083C2A0[0x8] = {
#include "lobby/0083C2A0.inc"
};
char D_0083C2A8[0xC] = {
#include "lobby/0083C2A8.inc"
};
char D_0083C2B8[0x8] = {
#include "lobby/0083C2B8.inc"
};
char D_0083C2C0[0x10] = {
#include "lobby/0083C2C0.inc"
};
char D_0083C2D0[0x10] = {
#include "lobby/0083C2D0.inc"
};
char D_0083C2E0[0x10] = {
#include "lobby/0083C2E0.inc"
};
char D_0083C2F0[0x10] = {
#include "lobby/0083C2F0.inc"
};
char D_0083C300[0x10] = {
#include "lobby/0083C300.inc"
};
char D_0083C310[0x10] = {
#include "lobby/0083C310.inc"
};
char D_0083C320[0x8] = {
#include "lobby/0083C320.inc"
};
char D_0083C328[0xC] = {
#include "lobby/0083C328.inc"
};
char D_0083C340[0x30] = {
#include "lobby/0083C340.inc"
};
char D_0083C370[0x30] = {
#include "lobby/0083C370.inc"
};
char D_0083C3A0[0x28] = {
#include "lobby/0083C3A0.inc"
};
char D_0083C3C8[0x8] = {
#include "lobby/0083C3C8.inc"
};
char D_0083C3D0[0x10] = {
#include "lobby/0083C3D0.inc"
};
char D_0083C3E0[0x20] = {
#include "lobby/0083C3E0.inc"
};
char D_0083C400[0x40] = {
#include "lobby/0083C400.inc"
};
char D_0083C440[0x30] = {
#include "lobby/0083C440.inc"
};
char D_0083C470[0x28] = {
#include "lobby/0083C470.inc"
};
char D_0083C498[13] = {
#include "lobby/0083C498.inc"
};
char D_0083C4B0[0x18] = {
#include "lobby/0083C4B0.inc"
};
char D_0083C4C8[13] = {
#include "lobby/0083C4C8.inc"
};
char D_0083C4D8[0xC] = {
#include "lobby/0083C4D8.inc"
};
char D_0083C4E8[0xC] = {
#include "lobby/0083C4E8.inc"
};
char D_0083C500[0x20] = {
#include "lobby/0083C500.inc"
};
char D_0083C520[0x20] = {
#include "lobby/0083C520.inc"
};
char D_0083C540[0x20] = {
#include "lobby/0083C540.inc"
};
char D_0083C560[0x20] = {
#include "lobby/0083C560.inc"
};
char D_0083C580[0x18] = {
#include "lobby/0083C580.inc"
};
char D_0083C598[15] = {
#include "lobby/0083C598.inc"
};
char D_0083C5A8[15] = {
#include "lobby/0083C5A8.inc"
};
char D_0083C5B8[0xC] = {
#include "lobby/0083C5B8.inc"
};
char D_0083C5C8[0x8] = {
#include "lobby/0083C5C8.inc"
};
char D_0083C5D0[0x8] = {
#include "lobby/0083C5D0.inc"
};
char D_0083C5D8[0x8] = {
#include "lobby/0083C5D8.inc"
};
char D_0083C5E0[0x10] = {
#include "lobby/0083C5E0.inc"
};
char D_0083C5F0[0x20] = {
#include "lobby/0083C5F0.inc"
};
char D_0083C610[0x20] = {
#include "lobby/0083C610.inc"
};
char D_0083C630[0x30] = {
#include "lobby/0083C630.inc"
};
char D_0083C660[0x30] = {
#include "lobby/0083C660.inc"
};
char D_0083C690[0x18] = {
#include "lobby/0083C690.inc"
};
char D_0083C6A8[15] = {
#include "lobby/0083C6A8.inc"
};
char D_0083C6B8[0xC] = {
#include "lobby/0083C6B8.inc"
};
char D_0083C6C8[13] = {
#include "lobby/0083C6C8.inc"
};
char D_0083C6D8[0xC] = {
#include "lobby/0083C6D8.inc"
};
char D_0083C6E8[0xC] = {
#include "lobby/0083C6E8.inc"
};
char D_0083C6F8[0x8] = {
#include "lobby/0083C6F8.inc"
};
char D_0083C700[0x10] = {
#include "lobby/0083C700.inc"
};
char D_0083C710[0x20] = {
#include "lobby/0083C710.inc"
};
char D_0083C730[0x30] = {
#include "lobby/0083C730.inc"
};
char D_0083C760[0x30] = {
#include "lobby/0083C760.inc"
};
char D_0083C790[0x30] = {
#include "lobby/0083C790.inc"
};
char D_0083C7C0[0x50] = {
#include "lobby/0083C7C0.inc"
};
char D_0083C810[0x20] = {
#include "lobby/0083C810.inc"
};
char D_0083C830[0x30] = {
#include "lobby/0083C830.inc"
};
char D_0083C860[0x30] = {
#include "lobby/0083C860.inc"
};
char D_0083C890[0x20] = {
#include "lobby/0083C890.inc"
};
char D_0083C8B0[0x20] = {
#include "lobby/0083C8B0.inc"
};
char D_0083C8D0[0x10] = {
#include "lobby/0083C8D0.inc"
};
char D_0083C8E0[0x10] = {
#include "lobby/0083C8E0.inc"
};
char D_0083C8F0[0x18] = {
#include "lobby/0083C8F0.inc"
};
char D_0083C908[0xC] = {
#include "lobby/0083C908.inc"
};
char D_0083C918[15] = {
#include "lobby/0083C918.inc"
};
char D_0083C928[0x8] = {
#include "lobby/0083C928.inc"
};
char D_0083C930[0x10] = {
#include "lobby/0083C930.inc"
};
char D_0083C940[0x10] = {
#include "lobby/0083C940.inc"
};
char D_0083C950[0x10] = {
#include "lobby/0083C950.inc"
};
char D_0083C960[0x20] = {
#include "lobby/0083C960.inc"
};
char D_0083C980[0x20] = {
#include "lobby/0083C980.inc"
};
char D_0083C9A0[0x20] = {
#include "lobby/0083C9A0.inc"
};
#pragma explicit_zero_data on
char gHunterNotesTexts_8[0x10] = {
#include "lobby/0083C9C0.inc"
};
#pragma explicit_zero_data reset
char D_0083C9D0[0x18] = {
#include "lobby/0083C9D0.inc"
};
char D_0083C9E8[0x8] = {
#include "lobby/0083C9E8.inc"
};
char D_0083C9F0[0x8] = {
#include "lobby/0083C9F0.inc"
};
char D_0083C9F8[0xC] = {
#include "lobby/0083C9F8.inc"
};
char D_0083CA08[0x8] = {
#include "lobby/0083CA08.inc"
};
char D_0083CA10[0x18] = {
#include "lobby/0083CA10.inc"
};
char D_0083CA28[0xC] = {
#include "lobby/0083CA28.inc"
};
char D_0083CA38[0x8] = {
#include "lobby/0083CA38.inc"
};
char D_0083CA40[0x20] = {
#include "lobby/0083CA40.inc"
};
char D_0083CA60[0x10] = {
#include "lobby/0083CA60.inc"
};
char D_0083CA70[0x20] = {
#include "lobby/0083CA70.inc"
};
char D_0083CA90[0x20] = {
#include "lobby/0083CA90.inc"
};
char D_0083CAB0[0x18] = {
#include "lobby/0083CAB0.inc"
};
char D_0083CAC8[0xC] = {
#include "lobby/0083CAC8.inc"
};
char D_0083CAD8[0xC] = {
#include "lobby/0083CAD8.inc"
};
char D_0083CAE8[0x8] = {
#include "lobby/0083CAE8.inc"
};
char D_0083CAF0[0x10] = {
#include "lobby/0083CAF0.inc"
};
char D_0083CB00[0x10] = {
#include "lobby/0083CB00.inc"
};
char D_0083CB10[0x10] = {
#include "lobby/0083CB10.inc"
};
char D_0083CB20[0x10] = {
#include "lobby/0083CB20.inc"
};
char D_0083CB30[0x8] = {
#include "lobby/0083CB30.inc"
};
char D_0083CB38[0xC] = {
#include "lobby/0083CB38.inc"
};
char D_0083CB50[0x30] = {
#include "lobby/0083CB50.inc"
};
char D_0083CB80[0x50] = {
#include "lobby/0083CB80.inc"
};
char D_0083CBD0[0x30] = {
#include "lobby/0083CBD0.inc"
};
char D_0083CC00[0x20] = {
#include "lobby/0083CC00.inc"
};
char D_0083CC20[0x10] = {
#include "lobby/0083CC20.inc"
};
char D_0083CC30[0x10] = {
#include "lobby/0083CC30.inc"
};
char D_0083CC40[0x30] = {
#include "lobby/0083CC40.inc"
};
char D_0083CC70[0x50] = {
#include "lobby/0083CC70.inc"
};
char D_0083CCC0[0x50] = {
#include "lobby/0083CCC0.inc"
};
char D_0083CD10[0x30] = {
#include "lobby/0083CD10.inc"
};
char D_0083CD40[0x50] = {
#include "lobby/0083CD40.inc"
};
char D_0083CD90[0x20] = {
#include "lobby/0083CD90.inc"
};
char D_0083CDB0[0x18] = {
#include "lobby/0083CDB0.inc"
};
char D_0083CDC8[0xC] = {
#include "lobby/0083CDC8.inc"
};
char D_0083CDD8[15] = {
#include "lobby/0083CDD8.inc"
};
char D_0083CDE8[13] = {
#include "lobby/0083CDE8.inc"
};
char D_0083CDF8[0xC] = {
#include "lobby/0083CDF8.inc"
};
char D_0083CE08[0xC] = {
#include "lobby/0083CE08.inc"
};
char D_0083CE18[0xC] = {
#include "lobby/0083CE18.inc"
};
char D_0083CE30[0x30] = {
#include "lobby/0083CE30.inc"
};
char D_0083CE60[0x20] = {
#include "lobby/0083CE60.inc"
};
char D_0083CE80[0x30] = {
#include "lobby/0083CE80.inc"
};
char D_0083CEB0[0x18] = {
#include "lobby/0083CEB0.inc"
};
char D_0083CEC8[0xC] = {
#include "lobby/0083CEC8.inc"
};
char D_0083CED8[0xC] = {
#include "lobby/0083CED8.inc"
};
char D_0083CEF0[0x18] = {
#include "lobby/0083CEF0.inc"
};
char D_0083CF08[15] = {
#include "lobby/0083CF08.inc"
};
char D_0083CF18[0x8] = {
#include "lobby/0083CF18.inc"
};
char D_0083CF20[0x8] = {
#include "lobby/0083CF20.inc"
};
char D_0083CF28[0x8] = {
#include "lobby/0083CF28.inc"
};
char D_0083CF30[0x8] = {
#include "lobby/0083CF30.inc"
};
char D_0083CF38[0x8] = {
#include "lobby/0083CF38.inc"
};
char D_0083CF40[0x8] = {
#include "lobby/0083CF40.inc"
};
char D_0083CF48[0xC] = {
#include "lobby/0083CF48.inc"
};
char D_0083CF58[0x8] = {
#include "lobby/0083CF58.inc"
};
char D_0083CF60[0x8] = {
#include "lobby/0083CF60.inc"
};
char D_0083CF68[0xC] = {
#include "lobby/0083CF68.inc"
};
char D_0083CF80[0x40] = {
#include "lobby/0083CF80.inc"
};
char D_0083CFC0[0x10] = {
#include "lobby/0083CFC0.inc"
};
char D_0083CFD0[0x10] = {
#include "lobby/0083CFD0.inc"
};
char D_0083CFE0[0x8] = {
#include "lobby/0083CFE0.inc"
};
char D_0083CFE8[13] = {
#include "lobby/0083CFE8.inc"
};
char D_0083D000[0x18] = {
#include "lobby/0083D000.inc"
};
char D_0083D018[13] = {
#include "lobby/0083D018.inc"
};
char D_0083D028[0x8] = {
#include "lobby/0083D028.inc"
};
char D_0083D030[0x10] = {
#include "lobby/0083D030.inc"
};
char D_0083D040[0x10] = {
#include "lobby/0083D040.inc"
};
char D_0083D050[0x10] = {
#include "lobby/0083D050.inc"
};
char D_0083D060[0x10] = {
#include "lobby/0083D060.inc"
};
char D_0083D070[0x18] = {
#include "lobby/0083D070.inc"
};
char D_0083D088[0xC] = {
#include "lobby/0083D088.inc"
};
char D_0083D0A0[0x18] = {
#include "lobby/0083D0A0.inc"
};
char D_0083D0B8[0xC] = {
#include "lobby/0083D0B8.inc"
};
char D_0083D0C8[15] = {
#include "lobby/0083D0C8.inc"
};
char D_0083D0D8[15] = {
#include "lobby/0083D0D8.inc"
};
char D_0083D0E8[15] = {
#include "lobby/0083D0E8.inc"
};
char D_0083D0F8[0x8] = {
#include "lobby/0083D0F8.inc"
};
char D_0083D100[0x18] = {
#include "lobby/0083D100.inc"
};
char D_0083D118[15] = {
#include "lobby/0083D118.inc"
};
char D_0083D128[0x8] = {
#include "lobby/0083D128.inc"
};
char D_0083D130[0x10] = {
#include "lobby/0083D130.inc"
};
char D_0083D140[0x18] = {
#include "lobby/0083D140.inc"
};
char D_0083D158[0xC] = {
#include "lobby/0083D158.inc"
};
char D_0083D168[0x8] = {
#include "lobby/0083D168.inc"
};
char D_0083D170[0x10] = {
#include "lobby/0083D170.inc"
};
char D_0083D180[0x8] = {
#include "lobby/0083D180.inc"
};
char D_0083D188[0x8] = {
#include "lobby/0083D188.inc"
};
char D_0083D190[0x8] = {
#include "lobby/0083D190.inc"
};
char D_0083D198[15] = {
#include "lobby/0083D198.inc"
};
char D_0083D1B0[0x70] = {
#include "lobby/0083D1B0.inc"
};
char D_0083D220[0x60] = {
#include "lobby/0083D220.inc"
};
char D_0083D280[0x30] = {
#include "lobby/0083D280.inc"
};
char D_0083D2B0[0x60] = {
#include "lobby/0083D2B0.inc"
};
char D_0083D310[0x8] = {
#include "lobby/0083D310.inc"
};
char D_0083D318[13] = {
#include "lobby/0083D318.inc"
};
char D_0083D328[0xC] = {
#include "lobby/0083D328.inc"
};
char D_0083D338[0x8] = {
#include "lobby/0083D338.inc"
};
char D_0083D340[0x10] = {
#include "lobby/0083D340.inc"
};
char D_0083D350[0x10] = {
#include "lobby/0083D350.inc"
};
char D_0083D360[0x10] = {
#include "lobby/0083D360.inc"
};
char D_0083D370[0x8] = {
#include "lobby/0083D370.inc"
};
char D_0083D378[0x8] = {
#include "lobby/0083D378.inc"
};
char D_0083D380[0x8] = {
#include "lobby/0083D380.inc"
};
char D_0083D388[15] = {
#include "lobby/0083D388.inc"
};
char D_0083D398[15] = {
#include "lobby/0083D398.inc"
};
char D_0083D3A8[15] = {
#include "lobby/0083D3A8.inc"
};
char D_0083D3B8[0xC] = {
#include "lobby/0083D3B8.inc"
};
char D_0083D3C8[0xC] = {
#include "lobby/0083D3C8.inc"
};
char D_0083D3D8[0xC] = {
#include "lobby/0083D3D8.inc"
};
char D_0083D3E8[0xC] = {
#include "lobby/0083D3E8.inc"
};
char D_0083D3F8[0x8] = {
#include "lobby/0083D3F8.inc"
};
char D_0083D400[0x20] = {
#include "lobby/0083D400.inc"
};
char D_0083D420[0x20] = {
#include "lobby/0083D420.inc"
};
char D_0083D440[0x18] = {
#include "lobby/0083D440.inc"
};
char D_0083D458[0xC] = {
#include "lobby/0083D458.inc"
};
char D_0083D468[0xC] = {
#include "lobby/0083D468.inc"
};
char D_0083D478[0xC] = {
#include "lobby/0083D478.inc"
};
char D_0083D488[0xC] = {
#include "lobby/0083D488.inc"
};
char D_0083D498[0xC] = {
#include "lobby/0083D498.inc"
};
char D_0083D4A8[15] = {
#include "lobby/0083D4A8.inc"
};
char D_0083D4B8[0x8] = {
#include "lobby/0083D4B8.inc"
};
char D_0083D4C0[0x10] = {
#include "lobby/0083D4C0.inc"
};
char D_0083D4D0[0x30] = {
#include "lobby/0083D4D0.inc"
};
char D_0083D500[0x20] = {
#include "lobby/0083D500.inc"
};
char D_0083D520[0x20] = {
#include "lobby/0083D520.inc"
};
char D_0083D540[0x10] = {
#include "lobby/0083D540.inc"
};
char D_0083D550[0x10] = {
#include "lobby/0083D550.inc"
};
char D_0083D560[0x10] = {
#include "lobby/0083D560.inc"
};
char D_0083D570[0x20] = {
#include "lobby/0083D570.inc"
};
char D_0083D590[0x10] = {
#include "lobby/0083D590.inc"
};
char D_0083D5A0[0x8] = {
#include "lobby/0083D5A0.inc"
};
char D_0083D5A8[0x8] = {
#include "lobby/0083D5A8.inc"
};
char D_0083D5B0[0x8] = {
#include "lobby/0083D5B0.inc"
};
char D_0083D5B8[0xC] = {
#include "lobby/0083D5B8.inc"
};
char D_0083D5C8[0xC] = {
#include "lobby/0083D5C8.inc"
};
char D_0083D5D8[0xC] = {
#include "lobby/0083D5D8.inc"
};
char D_0083D5E8[13] = {
#include "lobby/0083D5E8.inc"
};
char D_0083D600[0x20] = {
#include "lobby/0083D600.inc"
};
char D_0083D620[0x60] = {
#include "lobby/0083D620.inc"
};
char D_0083D680[0x80] = {
#include "lobby/0083D680.inc"
};
char D_0083D700[0x18] = {
#include "lobby/0083D700.inc"
};
char D_0083D718[0x8] = {
#include "lobby/0083D718.inc"
};
char D_0083D720[0x8] = {
#include "lobby/0083D720.inc"
};
char D_0083D728[0x8] = {
#include "lobby/0083D728.inc"
};
char D_0083D730[0x8] = {
#include "lobby/0083D730.inc"
};
char D_0083D738[0x8] = {
#include "lobby/0083D738.inc"
};
char D_0083D740[0x8] = {
#include "lobby/0083D740.inc"
};
char D_0083D748[0x8] = {
#include "lobby/0083D748.inc"
};
char D_0083D750[0x8] = {
#include "lobby/0083D750.inc"
};
char D_0083D758[0x8] = {
#include "lobby/0083D758.inc"
};
char D_0083D760[0x8] = {
#include "lobby/0083D760.inc"
};
char D_0083D768[0x8] = {
#include "lobby/0083D768.inc"
};
char D_0083D770[0x8] = {
#include "lobby/0083D770.inc"
};
char D_0083D778[0x8] = {
#include "lobby/0083D778.inc"
};
char D_0083D780[0x8] = {
#include "lobby/0083D780.inc"
};
char D_0083D788[0x8] = {
#include "lobby/0083D788.inc"
};
char D_0083D790[0x10] = {
#include "lobby/0083D790.inc"
};
char D_0083D7A0[0x18] = {
#include "lobby/0083D7A0.inc"
};
char D_0083D7B8[0x8] = {
#include "lobby/0083D7B8.inc"
};
char D_0083D7C0[0x20] = {
#include "lobby/0083D7C0.inc"
};
char D_0083D7E0[0x20] = {
#include "lobby/0083D7E0.inc"
};
char D_0083D800[0x20] = {
#include "lobby/0083D800.inc"
};
char D_0083D820[0x18] = {
#include "lobby/0083D820.inc"
};
char D_0083D838[0x8] = {
#include "lobby/0083D838.inc"
};
char D_0083D840[0x10] = {
#include "lobby/0083D840.inc"
};
char D_0083D850[0x10] = {
#include "lobby/0083D850.inc"
};
char D_0083D860[0x20] = {
#include "lobby/0083D860.inc"
};
char D_0083D880[0x18] = {
#include "lobby/0083D880.inc"
};
char D_0083D898[13] = {
#include "lobby/0083D898.inc"
};
char D_0083D8A8[0xC] = {
#include "lobby/0083D8A8.inc"
};
char D_0083D8B8[0xC] = {
#include "lobby/0083D8B8.inc"
};
char D_0083D8C8[0xC] = {
#include "lobby/0083D8C8.inc"
};
char D_0083D8D8[0xC] = {
#include "lobby/0083D8D8.inc"
};
char D_0083D8E8[0xC] = {
#include "lobby/0083D8E8.inc"
};
char D_0083D8F8[13] = {
#include "lobby/0083D8F8.inc"
};
char D_0083D908[0xC] = {
#include "lobby/0083D908.inc"
};
char D_0083D918[0xC] = {
#include "lobby/0083D918.inc"
};
char D_0083D928[0xC] = {
#include "lobby/0083D928.inc"
};
char D_0083D940[0x20] = {
#include "lobby/0083D940.inc"
};
char D_0083D960[0x18] = {
#include "lobby/0083D960.inc"
};
char D_0083D978[13] = {
#include "lobby/0083D978.inc"
};
char D_0083D988[13] = {
#include "lobby/0083D988.inc"
};
char D_0083D9A0[0x30] = {
#include "lobby/0083D9A0.inc"
};
char D_0083D9D0[0x30] = {
#include "lobby/0083D9D0.inc"
};
char D_0083DA00[0x50] = {
#include "lobby/0083DA00.inc"
};
char D_0083DA50[0x50] = {
#include "lobby/0083DA50.inc"
};
char D_0083DAA0[0x30] = {
#include "lobby/0083DAA0.inc"
};
char D_0083DAD0[0x20] = {
#include "lobby/0083DAD0.inc"
};
char D_0083DAF0[0x30] = {
#include "lobby/0083DAF0.inc"
};
char D_0083DB20[0x50] = {
#include "lobby/0083DB20.inc"
};
char D_0083DB70[0x40] = {
#include "lobby/0083DB70.inc"
};
char D_0083DBB0[0x30] = {
#include "lobby/0083DBB0.inc"
};
char D_0083DBE0[0x50] = {
#include "lobby/0083DBE0.inc"
};
char D_0083DC30[0x48] = {
#include "lobby/0083DC30.inc"
};
char D_0083DC78[0xC] = {
#include "lobby/0083DC78.inc"
};
char D_0083DC88[0xC] = {
#include "lobby/0083DC88.inc"
};
char D_0083DC98[0x8] = {
#include "lobby/0083DC98.inc"
};
char D_0083DCA0[0x10] = {
#include "lobby/0083DCA0.inc"
};
char D_0083DCB0[0x10] = {
#include "lobby/0083DCB0.inc"
};
char D_0083DCC0[0x20] = {
#include "lobby/0083DCC0.inc"
};
char D_0083DCE0[0x20] = {
#include "lobby/0083DCE0.inc"
};
char D_0083DD00[0x10] = {
#include "lobby/0083DD00.inc"
};
char D_0083DD10[0x10] = {
#include "lobby/0083DD10.inc"
};
char D_0083DD20[0x10] = {
#include "lobby/0083DD20.inc"
};
char D_0083DD30[0x10] = {
#include "lobby/0083DD30.inc"
};
char D_0083DD40[0x10] = {
#include "lobby/0083DD40.inc"
};
char D_0083DD50[0x10] = {
#include "lobby/0083DD50.inc"
};
char D_0083DD60[0x10] = {
#include "lobby/0083DD60.inc"
};
char D_0083DD70[0x10] = {
#include "lobby/0083DD70.inc"
};
char D_0083DD80[0x10] = {
#include "lobby/0083DD80.inc"
};
char D_0083DD90[0x10] = {
#include "lobby/0083DD90.inc"
};
char D_0083DDA0[0x20] = {
#include "lobby/0083DDA0.inc"
};
char D_0083DDC0[0x18] = {
#include "lobby/0083DDC0.inc"
};
char D_0083DDD8[15] = {
#include "lobby/0083DDD8.inc"
};
char D_0083DDF0[0x40] = {
#include "lobby/0083DDF0.inc"
};
char D_0083DE30[0x40] = {
#include "lobby/0083DE30.inc"
};
char D_0083DE70[0x40] = {
#include "lobby/0083DE70.inc"
};
char D_0083DEB0[0x30] = {
#include "lobby/0083DEB0.inc"
};
char D_0083DEE0[0x50] = {
#include "lobby/0083DEE0.inc"
};
char D_0083DF30[0x60] = {
#include "lobby/0083DF30.inc"
};
char D_0083DF90[0x20] = {
#include "lobby/0083DF90.inc"
};
char D_0083DFB0[0x20] = {
#include "lobby/0083DFB0.inc"
};
char D_0083DFD0[0x18] = {
#include "lobby/0083DFD0.inc"
};
char D_0083DFE8[13] = {
#include "lobby/0083DFE8.inc"
};
char D_0083DFF8[13] = {
#include "lobby/0083DFF8.inc"
};
char D_0083E008[15] = {
#include "lobby/0083E008.inc"
};
char D_0083E020[0x18] = {
#include "lobby/0083E020.inc"
};
char D_0083E038[13] = {
#include "lobby/0083E038.inc"
};
char D_0083E050[0x20] = {
#include "lobby/0083E050.inc"
};
char D_0083E070[0x20] = {
#include "lobby/0083E070.inc"
};
char D_0083E090[0x20] = {
#include "lobby/0083E090.inc"
};
char D_0083E0B0[0x20] = {
#include "lobby/0083E0B0.inc"
};
char D_0083E0D0[0x20] = {
#include "lobby/0083E0D0.inc"
};
char D_0083E0F0[0x18] = {
#include "lobby/0083E0F0.inc"
};
char D_0083E108[13] = {
#include "lobby/0083E108.inc"
};
char D_0083E118[15] = {
#include "lobby/0083E118.inc"
};
char D_0083E130[0x20] = {
#include "lobby/0083E130.inc"
};
char D_0083E150[0x20] = {
#include "lobby/0083E150.inc"
};
char D_0083E170[0x50] = {
#include "lobby/0083E170.inc"
};
char D_0083E1C0[0xA0] = {
#include "lobby/0083E1C0.inc"
};
char D_0083E260[0x60] = {
#include "lobby/0083E260.inc"
};
char D_0083E2C0[0x90] = {
#include "lobby/0083E2C0.inc"
};
char D_0083E350[0x60] = {
#include "lobby/0083E350.inc"
};
char D_0083E3B0[0x70] = {
#include "lobby/0083E3B0.inc"
};
char D_0083E420[0x70] = {
#include "lobby/0083E420.inc"
};
char D_0083E490[0xA0] = {
#include "lobby/0083E490.inc"
};
char D_0083E530[0x60] = {
#include "lobby/0083E530.inc"
};
char D_0083E590[0x80] = {
#include "lobby/0083E590.inc"
};
char D_0083E610[0x60] = {
#include "lobby/0083E610.inc"
};
char D_0083E670[0x30] = {
#include "lobby/0083E670.inc"
};
char D_0083E6A0[0x50] = {
#include "lobby/0083E6A0.inc"
};
char D_0083E6F0[0x50] = {
#include "lobby/0083E6F0.inc"
};
char D_0083E740[0x50] = {
#include "lobby/0083E740.inc"
};
char D_0083E790[0x50] = {
#include "lobby/0083E790.inc"
};
char D_0083E7E0[0x50] = {
#include "lobby/0083E7E0.inc"
};
char D_0083E830[0x50] = {
#include "lobby/0083E830.inc"
};
char D_0083E880[0x50] = {
#include "lobby/0083E880.inc"
};
char D_0083E8D0[0x60] = {
#include "lobby/0083E8D0.inc"
};
char D_0083E930[0x70] = {
#include "lobby/0083E930.inc"
};
char D_0083E9A0[0x30] = {
#include "lobby/0083E9A0.inc"
};
char D_0083E9D0[0x30] = {
#include "lobby/0083E9D0.inc"
};
char D_0083EA00[0x30] = {
#include "lobby/0083EA00.inc"
};
char D_0083EA30[0x20] = {
#include "lobby/0083EA30.inc"
};
char D_0083EA50[0x40] = {
#include "lobby/0083EA50.inc"
};
char D_0083EA90[0x20] = {
#include "lobby/0083EA90.inc"
};
char D_0083EAB0[0x50] = {
#include "lobby/0083EAB0.inc"
};
char D_0083EB00[0x70] = {
#include "lobby/0083EB00.inc"
};
char D_0083EB70[0x50] = {
#include "lobby/0083EB70.inc"
};
char D_0083EBC0[0x40] = {
#include "lobby/0083EBC0.inc"
};
char D_0083EC00[0x30] = {
#include "lobby/0083EC00.inc"
};
char D_0083EC30[0x30] = {
#include "lobby/0083EC30.inc"
};
char D_0083EC60[0x20] = {
#include "lobby/0083EC60.inc"
};
char D_0083EC80[0x18] = {
#include "lobby/0083EC80.inc"
};
char D_0083EC98[0xC] = {
#include "lobby/0083EC98.inc"
};
char D_0083ECA8[0xC] = {
#include "lobby/0083ECA8.inc"
};
char D_0083ECC0[0x18] = {
#include "lobby/0083ECC0.inc"
};
char D_0083ECD8[15] = {
#include "lobby/0083ECD8.inc"
};
char D_0083ECF0[0x20] = {
#include "lobby/0083ECF0.inc"
};
char D_0083ED10[0x20] = {
#include "lobby/0083ED10.inc"
};
char D_0083ED30[0x20] = {
#include "lobby/0083ED30.inc"
};
char D_0083ED50[0x20] = {
#include "lobby/0083ED50.inc"
};
char D_0083ED70[0x20] = {
#include "lobby/0083ED70.inc"
};
char D_0083ED90[0x18] = {
#include "lobby/0083ED90.inc"
};
char D_0083EDA8[0x8] = {
#include "lobby/0083EDA8.inc"
};
char D_0083EDB0[0x10] = {
#include "lobby/0083EDB0.inc"
};
char D_0083EDC0[0x20] = {
#include "lobby/0083EDC0.inc"
};
char D_0083EDE0[0x20] = {
#include "lobby/0083EDE0.inc"
};
char D_0083EE00[0x38] = {
#include "lobby/0083EE00.inc"
};
char D_0083EE38[0xC] = {
#include "lobby/0083EE38.inc"
};
char D_0083EE50[0x18] = {
#include "lobby/0083EE50.inc"
};
char D_0083EE68[15] = {
#include "lobby/0083EE68.inc"
};
char D_0083EE80[0x18] = {
#include "lobby/0083EE80.inc"
};
char D_0083EE98[15] = {
#include "lobby/0083EE98.inc"
};
char D_0083EEA8[15] = {
#include "lobby/0083EEA8.inc"
};
char D_0083EEC0[0x18] = {
#include "lobby/0083EEC0.inc"
};
char D_0083EED8[15] = {
#include "lobby/0083EED8.inc"
};
char D_0083EEE8[15] = {
#include "lobby/0083EEE8.inc"
};
char D_0083EEF8[15] = {
#include "lobby/0083EEF8.inc"
};
char D_0083EF08[15] = {
#include "lobby/0083EF08.inc"
};
char D_0083EF18[13] = {
#include "lobby/0083EF18.inc"
};
char D_0083EF28[13] = {
#include "lobby/0083EF28.inc"
};
char D_0083EF38[0xC] = {
#include "lobby/0083EF38.inc"
};
char D_0083EF48[15] = {
#include "lobby/0083EF48.inc"
};
char D_0083EF58[13] = {
#include "lobby/0083EF58.inc"
};
char D_0083EF68[15] = {
#include "lobby/0083EF68.inc"
};
char D_0083EF78[15] = {
#include "lobby/0083EF78.inc"
};
char D_0083EF90[0x20] = {
#include "lobby/0083EF90.inc"
};
char D_0083EFB0[0x20] = {
#include "lobby/0083EFB0.inc"
};
char D_0083EFD0[0x20] = {
#include "lobby/0083EFD0.inc"
};
char D_0083EFF0[0x20] = {
#include "lobby/0083EFF0.inc"
};
char D_0083F010[0x20] = {
#include "lobby/0083F010.inc"
};
char D_0083F030[0x20] = {
#include "lobby/0083F030.inc"
};
char D_0083F050[0x20] = {
#include "lobby/0083F050.inc"
};
char D_0083F070[0x20] = {
#include "lobby/0083F070.inc"
};
char D_0083F090[0x20] = {
#include "lobby/0083F090.inc"
};
char D_0083F0B0[0x20] = {
#include "lobby/0083F0B0.inc"
};
char D_0083F0D0[0x20] = {
#include "lobby/0083F0D0.inc"
};
char D_0083F0F0[0x20] = {
#include "lobby/0083F0F0.inc"
};
char D_0083F110[0x20] = {
#include "lobby/0083F110.inc"
};
char D_0083F130[0x10] = {
#include "lobby/0083F130.inc"
};
char D_0083F140[0x10] = {
#include "lobby/0083F140.inc"
};
char D_0083F150[0x20] = {
#include "lobby/0083F150.inc"
};
char D_0083F170[0x20] = {
#include "lobby/0083F170.inc"
};
char D_0083F190[0x20] = {
#include "lobby/0083F190.inc"
};
char D_0083F1B0[0x20] = {
#include "lobby/0083F1B0.inc"
};
char D_0083F1D0[0x20] = {
#include "lobby/0083F1D0.inc"
};
char D_0083F1F0[0x20] = {
#include "lobby/0083F1F0.inc"
};
char D_0083F210[0x20] = {
#include "lobby/0083F210.inc"
};
char D_0083F230[0x20] = {
#include "lobby/0083F230.inc"
};
char D_0083F250[0x20] = {
#include "lobby/0083F250.inc"
};
char D_0083F270[0x20] = {
#include "lobby/0083F270.inc"
};
char D_0083F290[0x20] = {
#include "lobby/0083F290.inc"
};
char D_0083F2B0[0x20] = {
#include "lobby/0083F2B0.inc"
};
char D_0083F2D0[0x18] = {
#include "lobby/0083F2D0.inc"
};
char D_0083F2E8[15] = {
#include "lobby/0083F2E8.inc"
};
char D_0083F300[0x18] = {
#include "lobby/0083F300.inc"
};
char D_0083F318[13] = {
#include "lobby/0083F318.inc"
};
char D_0083F328[15] = {
#include "lobby/0083F328.inc"
};
char D_0083F338[0xC] = {
#include "lobby/0083F338.inc"
};
char D_0083F348[0xC] = {
#include "lobby/0083F348.inc"
};
char D_0083F358[0xC] = {
#include "lobby/0083F358.inc"
};
char D_0083F368[0x8] = {
#include "lobby/0083F368.inc"
};
char D_0083F370[0x10] = {
#include "lobby/0083F370.inc"
};
char D_0083F380[0x20] = {
#include "lobby/0083F380.inc"
};
char D_0083F3A0[0x20] = {
#include "lobby/0083F3A0.inc"
};
char D_0083F3C0[0x18] = {
#include "lobby/0083F3C0.inc"
};
char D_0083F3D8[15] = {
#include "lobby/0083F3D8.inc"
};
char D_0083F3E8[0x8] = {
#include "lobby/0083F3E8.inc"
};
char D_0083F3F0[0x8] = {
#include "lobby/0083F3F0.inc"
};
char D_0083F3F8[0x8] = {
#include "lobby/0083F3F8.inc"
};
char D_0083F400[0x8] = {
#include "lobby/0083F400.inc"
};
char D_0083F408[0x8] = {
#include "lobby/0083F408.inc"
};
char D_0083F410[0x20] = {
#include "lobby/0083F410.inc"
};
char D_0083F430[0x20] = {
#include "lobby/0083F430.inc"
};
char D_0083F450[0x20] = {
#include "lobby/0083F450.inc"
};
char D_0083F470[0x10] = {
#include "lobby/0083F470.inc"
};
char D_0083F480[0x20] = {
#include "lobby/0083F480.inc"
};
char D_0083F4A0[0x10] = {
#include "lobby/0083F4A0.inc"
};
char D_0083F4B0[0x10] = {
#include "lobby/0083F4B0.inc"
};
char D_0083F4C0[0x20] = {
#include "lobby/0083F4C0.inc"
};
char D_0083F4E0[0x10] = {
#include "lobby/0083F4E0.inc"
};
char D_0083F4F0[0x20] = {
#include "lobby/0083F4F0.inc"
};
char D_0083F510[0x20] = {
#include "lobby/0083F510.inc"
};
char D_0083F530[0x10] = {
#include "lobby/0083F530.inc"
};
char D_0083F540[0x10] = {
#include "lobby/0083F540.inc"
};
char D_0083F550[0x10] = {
#include "lobby/0083F550.inc"
};
char D_0083F560[0x10] = {
#include "lobby/0083F560.inc"
};
char D_0083F570[0x20] = {
#include "lobby/0083F570.inc"
};
char D_0083F590[0x18] = {
#include "lobby/0083F590.inc"
};
char D_0083F5A8[13] = {
#include "lobby/0083F5A8.inc"
};
char D_0083F5C0[0x20] = {
#include "lobby/0083F5C0.inc"
};
char D_0083F5E0[0x20] = {
#include "lobby/0083F5E0.inc"
};
char D_0083F600[0x18] = {
#include "lobby/0083F600.inc"
};
char D_0083F618[0x8] = {
#include "lobby/0083F618.inc"
};
char D_0083F620[0x30] = {
#include "lobby/0083F620.inc"
};
char D_0083F650[0x20] = {
#include "lobby/0083F650.inc"
};
char D_0083F670[0x30] = {
#include "lobby/0083F670.inc"
};
char D_0083F6A0[0x30] = {
#include "lobby/0083F6A0.inc"
};
char D_0083F6D0[0x30] = {
#include "lobby/0083F6D0.inc"
};
char D_0083F700[0x20] = {
#include "lobby/0083F700.inc"
};
char D_0083F720[0x30] = {
#include "lobby/0083F720.inc"
};
char D_0083F750[0x20] = {
#include "lobby/0083F750.inc"
};
char D_0083F770[0x30] = {
#include "lobby/0083F770.inc"
};
char D_0083F7A0[0x20] = {
#include "lobby/0083F7A0.inc"
};
char D_0083F7C0[0x50] = {
#include "lobby/0083F7C0.inc"
};
char D_0083F810[0x20] = {
#include "lobby/0083F810.inc"
};
char D_0083F830[0x30] = {
#include "lobby/0083F830.inc"
};
char D_0083F860[0x20] = {
#include "lobby/0083F860.inc"
};
char D_0083F880[0x20] = {
#include "lobby/0083F880.inc"
};
char D_0083F8A0[0x18] = {
#include "lobby/0083F8A0.inc"
};
char D_0083F8B8[13] = {
#include "lobby/0083F8B8.inc"
};
char D_0083F8D0[0x20] = {
#include "lobby/0083F8D0.inc"
};
char D_0083F8F0[0x20] = {
#include "lobby/0083F8F0.inc"
};
char D_0083F910[0x30] = {
#include "lobby/0083F910.inc"
};
char D_0083F940[0x30] = {
#include "lobby/0083F940.inc"
};
char D_0083F970[0x30] = {
#include "lobby/0083F970.inc"
};
char D_0083F9A0[0x20] = {
#include "lobby/0083F9A0.inc"
};
char D_0083F9C0[0x20] = {
#include "lobby/0083F9C0.inc"
};
char D_0083F9E0[0x20] = {
#include "lobby/0083F9E0.inc"
};
char D_0083FA00[0x30] = {
#include "lobby/0083FA00.inc"
};
char D_0083FA30[0x48] = {
#include "lobby/0083FA30.inc"
};
char D_0083FA78[13] = {
#include "lobby/0083FA78.inc"
};
char D_0083FA90[0x20] = {
#include "lobby/0083FA90.inc"
};
char D_0083FAB0[0x30] = {
#include "lobby/0083FAB0.inc"
};
char D_0083FAE0[0x30] = {
#include "lobby/0083FAE0.inc"
};
char D_0083FB10[0x30] = {
#include "lobby/0083FB10.inc"
};
char D_0083FB40[0x30] = {
#include "lobby/0083FB40.inc"
};
char D_0083FB70[0x20] = {
#include "lobby/0083FB70.inc"
};
char D_0083FB90[0x20] = {
#include "lobby/0083FB90.inc"
};
char D_0083FBB0[0x30] = {
#include "lobby/0083FBB0.inc"
};
char D_0083FBE0[0x30] = {
#include "lobby/0083FBE0.inc"
};
char D_0083FC10[0x20] = {
#include "lobby/0083FC10.inc"
};
char D_0083FC30[0x40] = {
#include "lobby/0083FC30.inc"
};
char D_0083FC70[0x30] = {
#include "lobby/0083FC70.inc"
};
char D_0083FCA0[0x10] = {
#include "lobby/0083FCA0.inc"
};
char D_0083FCB0[0x20] = {
#include "lobby/0083FCB0.inc"
};
char D_0083FCD0[0x50] = {
#include "lobby/0083FCD0.inc"
};
char D_0083FD20[0x20] = {
#include "lobby/0083FD20.inc"
};
char D_0083FD40[0x20] = {
#include "lobby/0083FD40.inc"
};
char D_0083FD60[0x20] = {
#include "lobby/0083FD60.inc"
};
char D_0083FD80[0x30] = {
#include "lobby/0083FD80.inc"
};
char D_0083FDB0[0x20] = {
#include "lobby/0083FDB0.inc"
};
char D_0083FDD0[0x40] = {
#include "lobby/0083FDD0.inc"
};
char D_0083FE10[0x40] = {
#include "lobby/0083FE10.inc"
};
char D_0083FE50[0x30] = {
#include "lobby/0083FE50.inc"
};
char D_0083FE80[0x30] = {
#include "lobby/0083FE80.inc"
};
char D_0083FEB0[0x20] = {
#include "lobby/0083FEB0.inc"
};
char D_0083FED0[0x40] = {
#include "lobby/0083FED0.inc"
};
char D_0083FF10[0x40] = {
#include "lobby/0083FF10.inc"
};
char D_0083FF50[0x40] = {
#include "lobby/0083FF50.inc"
};
char D_0083FF90[0x30] = {
#include "lobby/0083FF90.inc"
};
char D_0083FFC0[0x40] = {
#include "lobby/0083FFC0.inc"
};
char D_00840000[0x40] = {
#include "lobby/00840000.inc"
};
char D_00840040[0x40] = {
#include "lobby/00840040.inc"
};
char D_00840080[0x30] = {
#include "lobby/00840080.inc"
};
char D_008400B0[0x30] = {
#include "lobby/008400B0.inc"
};
char D_008400E0[0x40] = {
#include "lobby/008400E0.inc"
};
char D_00840120[0x40] = {
#include "lobby/00840120.inc"
};
char D_00840160[0x30] = {
#include "lobby/00840160.inc"
};
char D_00840190[0x18] = {
#include "lobby/00840190.inc"
};
char D_008401A8[0x8] = {
#include "lobby/008401A8.inc"
};
char D_008401B0[0x8] = {
#include "lobby/008401B0.inc"
};
char D_008401B8[0x8] = {
#include "lobby/008401B8.inc"
};
char D_008401C0[0x10] = {
#include "lobby/008401C0.inc"
};
char D_008401D0[0x8] = {
#include "lobby/008401D0.inc"
};
char D_008401D8[0x8] = {
#include "lobby/008401D8.inc"
};
char D_008401E0[0x18] = {
#include "lobby/008401E0.inc"
};
char D_008401F8[0xC] = {
#include "lobby/008401F8.inc"
};
char D_00840208[0xC] = {
#include "lobby/00840208.inc"
};
char D_00840220[0x20] = {
#include "lobby/00840220.inc"
};
char D_00840240[0x18] = {
#include "lobby/00840240.inc"
};
char D_00840258[0xC] = {
#include "lobby/00840258.inc"
};
char D_00840268[0x8] = {
#include "lobby/00840268.inc"
};
char D_00840270[0x10] = {
#include "lobby/00840270.inc"
};
char D_00840280[0x8] = {
#include "lobby/00840280.inc"
};
char D_00840288[0xC] = {
#include "lobby/00840288.inc"
};
char D_00840298[0xC] = {
#include "lobby/00840298.inc"
};
char D_008402A8[0xC] = {
#include "lobby/008402A8.inc"
};
char D_008402B8[0xC] = {
#include "lobby/008402B8.inc"
};
char D_008402C8[13] = {
#include "lobby/008402C8.inc"
};
char D_008402D8[0x8] = {
#include "lobby/008402D8.inc"
};
char D_008402E0[0x18] = {
#include "lobby/008402E0.inc"
};
char D_008402F8[0xC] = {
#include "lobby/008402F8.inc"
};
char D_00840308[13] = {
#include "lobby/00840308.inc"
};
char D_00840318[13] = {
#include "lobby/00840318.inc"
};
char D_00840328[0xC] = {
#include "lobby/00840328.inc"
};
char D_00840338[0xC] = {
#include "lobby/00840338.inc"
};
char D_00840348[13] = {
#include "lobby/00840348.inc"
};
char D_00840360[0x18] = {
#include "lobby/00840360.inc"
};
char D_00840378[15] = {
#include "lobby/00840378.inc"
};
char D_00840390[0x20] = {
#include "lobby/00840390.inc"
};
char D_008403B0[0x20] = {
#include "lobby/008403B0.inc"
};
char D_008403D0[0x20] = {
#include "lobby/008403D0.inc"
};
char D_008403F0[0x20] = {
#include "lobby/008403F0.inc"
};
char D_00840410[0x20] = {
#include "lobby/00840410.inc"
};
char D_00840430[0x20] = {
#include "lobby/00840430.inc"
};
char D_00840450[0x20] = {
#include "lobby/00840450.inc"
};
char D_00840470[0x20] = {
#include "lobby/00840470.inc"
};
char D_00840490[0x20] = {
#include "lobby/00840490.inc"
};
char D_008404B0[0x20] = {
#include "lobby/008404B0.inc"
};
char D_008404D0[0x20] = {
#include "lobby/008404D0.inc"
};
char D_008404F0[0x20] = {
#include "lobby/008404F0.inc"
};
char D_00840510[0x18] = {
#include "lobby/00840510.inc"
};
char D_00840528[0xC] = {
#include "lobby/00840528.inc"
};
char D_00840538[13] = {
#include "lobby/00840538.inc"
};
char D_00840550[0x18] = {
#include "lobby/00840550.inc"
};
char D_00840568[0xC] = {
#include "lobby/00840568.inc"
};
char D_00840578[0xC] = {
#include "lobby/00840578.inc"
};
char D_00840588[0xC] = {
#include "lobby/00840588.inc"
};
char D_008405A0[0x30] = {
#include "lobby/008405A0.inc"
};
char D_008405D0[0x20] = {
#include "lobby/008405D0.inc"
};
char D_008405F0[0x30] = {
#include "lobby/008405F0.inc"
};
char D_00840620[0x18] = {
#include "lobby/00840620.inc"
};
char D_00840638[15] = {
#include "lobby/00840638.inc"
};
char D_00840648[13] = {
#include "lobby/00840648.inc"
};
char D_00840658[15] = {
#include "lobby/00840658.inc"
};
char D_00840668[15] = {
#include "lobby/00840668.inc"
};
char D_00840678[13] = {
#include "lobby/00840678.inc"
};
char D_00840688[13] = {
#include "lobby/00840688.inc"
};
char D_008406A0[0x20] = {
#include "lobby/008406A0.inc"
};
char D_008406C0[0x18] = {
#include "lobby/008406C0.inc"
};
char D_008406D8[15] = {
#include "lobby/008406D8.inc"
};
char D_008406E8[13] = {
#include "lobby/008406E8.inc"
};
char D_008406F8[15] = {
#include "lobby/008406F8.inc"
};
char D_00840708[13] = {
#include "lobby/00840708.inc"
};
char D_00840718[15] = {
#include "lobby/00840718.inc"
};
char D_00840728[13] = {
#include "lobby/00840728.inc"
};
char D_00840740[0x20] = {
#include "lobby/00840740.inc"
};
char D_00840760[0x18] = {
#include "lobby/00840760.inc"
};
char D_00840778[0xC] = {
#include "lobby/00840778.inc"
};
char D_00840788[0xC] = {
#include "lobby/00840788.inc"
};
char D_00840798[0xC] = {
#include "lobby/00840798.inc"
};
char D_008407A8[0xC] = {
#include "lobby/008407A8.inc"
};
char D_008407C0[0x18] = {
#include "lobby/008407C0.inc"
};
char D_008407D8[0xC] = {
#include "lobby/008407D8.inc"
};
char D_008407E8[13] = {
#include "lobby/008407E8.inc"
};
char D_008407F8[15] = {
#include "lobby/008407F8.inc"
};
char D_00840808[15] = {
#include "lobby/00840808.inc"
};
char D_00840818[0xC] = {
#include "lobby/00840818.inc"
};
char D_00840828[0xC] = {
#include "lobby/00840828.inc"
};
char D_00840838[13] = {
#include "lobby/00840838.inc"
};
char D_00840850[0x20] = {
#include "lobby/00840850.inc"
};
char D_00840870[0x18] = {
#include "lobby/00840870.inc"
};
char D_00840888[13] = {
#include "lobby/00840888.inc"
};
char D_00840898[13] = {
#include "lobby/00840898.inc"
};
char D_008408A8[0xC] = {
#include "lobby/008408A8.inc"
};
char D_008408B8[0xC] = {
#include "lobby/008408B8.inc"
};
char D_008408C8[0x8] = {
#include "lobby/008408C8.inc"
};
char D_008408D0[0x10] = {
#include "lobby/008408D0.inc"
};
char D_008408E0[0x10] = {
#include "lobby/008408E0.inc"
};
char D_008408F0[0x10] = {
#include "lobby/008408F0.inc"
};
char D_00840900[0x10] = {
#include "lobby/00840900.inc"
};
char D_00840910[0x10] = {
#include "lobby/00840910.inc"
};
char D_00840920[0x10] = {
#include "lobby/00840920.inc"
};
char D_00840930[0x10] = {
#include "lobby/00840930.inc"
};
char D_00840940[0x10] = {
#include "lobby/00840940.inc"
};
char D_00840950[0x40] = {
#include "lobby/00840950.inc"
};
char D_00840990[0x20] = {
#include "lobby/00840990.inc"
};
char D_008409B0[0x30] = {
#include "lobby/008409B0.inc"
};
char D_008409E0[0x30] = {
#include "lobby/008409E0.inc"
};
char D_00840A10[0x20] = {
#include "lobby/00840A10.inc"
};
char D_00840A30[0x40] = {
#include "lobby/00840A30.inc"
};
char D_00840A70[0x40] = {
#include "lobby/00840A70.inc"
};
char D_00840AB0[0x20] = {
#include "lobby/00840AB0.inc"
};
char D_00840AD0[0x20] = {
#include "lobby/00840AD0.inc"
};
char D_00840AF0[0x20] = {
#include "lobby/00840AF0.inc"
};
char D_00840B10[0x20] = {
#include "lobby/00840B10.inc"
};
char D_00840B30[0x20] = {
#include "lobby/00840B30.inc"
};
char D_00840B50[0x20] = {
#include "lobby/00840B50.inc"
};
char D_00840B70[0x20] = {
#include "lobby/00840B70.inc"
};
char D_00840B90[0x20] = {
#include "lobby/00840B90.inc"
};
char D_00840BB0[0x40] = {
#include "lobby/00840BB0.inc"
};
char D_00840BF0[0x50] = {
#include "lobby/00840BF0.inc"
};
char D_00840C40[0x40] = {
#include "lobby/00840C40.inc"
};
char D_00840C80[0x40] = {
#include "lobby/00840C80.inc"
};
char D_00840CC0[0x30] = {
#include "lobby/00840CC0.inc"
};
char D_00840CF0[0x30] = {
#include "lobby/00840CF0.inc"
};
char D_00840D20[0x30] = {
#include "lobby/00840D20.inc"
};
char D_00840D50[0x60] = {
#include "lobby/00840D50.inc"
};
char D_00840DB0[0x60] = {
#include "lobby/00840DB0.inc"
};
char D_00840E10[0x30] = {
#include "lobby/00840E10.inc"
};
char D_00840E40[0x30] = {
#include "lobby/00840E40.inc"
};
char D_00840E70[0x30] = {
#include "lobby/00840E70.inc"
};
char D_00840EA0[0x20] = {
#include "lobby/00840EA0.inc"
};
char D_00840EC0[0x40] = {
#include "lobby/00840EC0.inc"
};
char D_00840F00[0x60] = {
#include "lobby/00840F00.inc"
};
char D_00840F60[0x30] = {
#include "lobby/00840F60.inc"
};
char D_00840F90[0x40] = {
#include "lobby/00840F90.inc"
};
char D_00840FD0[0x40] = {
#include "lobby/00840FD0.inc"
};
char D_00841010[0x38] = {
#include "lobby/00841010.inc"
};
char D_00841048[13] = {
#include "lobby/00841048.inc"
};
char D_00841058[0xC] = {
#include "lobby/00841058.inc"
};
char D_00841068[0xC] = {
#include "lobby/00841068.inc"
};
char D_00841078[15] = {
#include "lobby/00841078.inc"
};
char D_00841088[0xC] = {
#include "lobby/00841088.inc"
};
char D_00841098[0xC] = {
#include "lobby/00841098.inc"
};
char D_008410B0[0x18] = {
#include "lobby/008410B0.inc"
};
char D_008410C8[15] = {
#include "lobby/008410C8.inc"
};
char D_008410E0[0x20] = {
#include "lobby/008410E0.inc"
};
char D_00841100[0x30] = {
#include "lobby/00841100.inc"
};
char D_00841130[0x30] = {
#include "lobby/00841130.inc"
};
char D_00841160[0x50] = {
#include "lobby/00841160.inc"
};
char D_008411B0[0x90] = {
#include "lobby/008411B0.inc"
};
char D_00841240[0x50] = {
#include "lobby/00841240.inc"
};
char D_00841290[0x68] = {
#include "lobby/00841290.inc"
};
char D_008412F8[13] = {
#include "lobby/008412F8.inc"
};
char D_00841310[0x20] = {
#include "lobby/00841310.inc"
};
char D_00841330[0x10] = {
#include "lobby/00841330.inc"
};
char D_00841340[0x10] = {
#include "lobby/00841340.inc"
};
char D_00841350[0x20] = {
#include "lobby/00841350.inc"
};
char D_00841370[0x20] = {
#include "lobby/00841370.inc"
};
char D_00841390[0x20] = {
#include "lobby/00841390.inc"
};
char D_008413B0[0x20] = {
#include "lobby/008413B0.inc"
};
char D_008413D0[0x20] = {
#include "lobby/008413D0.inc"
};
char D_008413F0[0x20] = {
#include "lobby/008413F0.inc"
};
char D_00841410[0x10] = {
#include "lobby/00841410.inc"
};
char D_00841420[0x10] = {
#include "lobby/00841420.inc"
};
char D_00841430[0x20] = {
#include "lobby/00841430.inc"
};
char D_00841450[0x20] = {
#include "lobby/00841450.inc"
};
char D_00841470[0x20] = {
#include "lobby/00841470.inc"
};
char D_00841490[0x18] = {
#include "lobby/00841490.inc"
};
char D_008414A8[0xC] = {
#include "lobby/008414A8.inc"
};
char D_008414B8[13] = {
#include "lobby/008414B8.inc"
};
char D_008414C8[13] = {
#include "lobby/008414C8.inc"
};
char D_008414D8[0xC] = {
#include "lobby/008414D8.inc"
};
char D_008414F0[0x18] = {
#include "lobby/008414F0.inc"
};
char D_00841508[0x8] = {
#include "lobby/00841508.inc"
};
char D_00841510[0x10] = {
#include "lobby/00841510.inc"
};
char D_00841520[0x10] = {
#include "lobby/00841520.inc"
};
char D_00841530[0x10] = {
#include "lobby/00841530.inc"
};
char D_00841540[0x8] = {
#include "lobby/00841540.inc"
};
char D_00841548[0x8] = {
#include "lobby/00841548.inc"
};
char D_00841550[0x8] = {
#include "lobby/00841550.inc"
};
char D_00841558[0x8] = {
#include "lobby/00841558.inc"
};
char D_00841560[0x20] = {
#include "lobby/00841560.inc"
};
char D_00841580[0x20] = {
#include "lobby/00841580.inc"
};
char D_008415A0[0x20] = {
#include "lobby/008415A0.inc"
};
char D_008415C0[0x20] = {
#include "lobby/008415C0.inc"
};
char D_008415E0[0x20] = {
#include "lobby/008415E0.inc"
};
char D_00841600[0x20] = {
#include "lobby/00841600.inc"
};
char D_00841620[0x20] = {
#include "lobby/00841620.inc"
};
char D_00841640[0x20] = {
#include "lobby/00841640.inc"
};
char D_00841660[0x20] = {
#include "lobby/00841660.inc"
};
char D_00841680[0x20] = {
#include "lobby/00841680.inc"
};
char D_008416A0[0x20] = {
#include "lobby/008416A0.inc"
};
char D_008416C0[0x18] = {
#include "lobby/008416C0.inc"
};
char D_008416D8[14] = {
#include "lobby/008416D8.inc"
};
char D_008416F0[0x20] = {
#include "lobby/008416F0.inc"
};
char D_00841710[0x18] = {
#include "lobby/00841710.inc"
};
char D_00841728[14] = {
#include "lobby/00841728.inc"
};
char D_00841738[0xC] = {
#include "lobby/00841738.inc"
};
char D_00841750[0x20] = {
#include "lobby/00841750.inc"
};
char D_00841770[0x20] = {
#include "lobby/00841770.inc"
};
char D_00841790[0x20] = {
#include "lobby/00841790.inc"
};
char D_008417B0[0x18] = {
#include "lobby/008417B0.inc"
};
char D_008417C8[0xC] = {
#include "lobby/008417C8.inc"
};
char D_008417E0[0x20] = {
#include "lobby/008417E0.inc"
};
char D_00841800[0x20] = {
#include "lobby/00841800.inc"
};
char D_00841820[0x18] = {
#include "lobby/00841820.inc"
};
char D_00841838[14] = {
#include "lobby/00841838.inc"
};
char D_00841850[0x10] = {
#include "lobby/00841850.inc"
};
char D_00841860[0x10] = {
#include "lobby/00841860.inc"
};
char D_00841870[0x18] = {
#include "lobby/00841870.inc"
};
char D_00841888[0x8] = {
#include "lobby/00841888.inc"
};
char D_00841890[0x10] = {
#include "lobby/00841890.inc"
};
char D_008418A0[0x8] = {
#include "lobby/008418A0.inc"
};
char D_008418A8[0x8] = {
#include "lobby/008418A8.inc"
};
char D_008418B0[0x8] = {
#include "lobby/008418B0.inc"
};
char D_008418B8[13] = {
#include "lobby/008418B8.inc"
};
char D_008418C8[13] = {
#include "lobby/008418C8.inc"
};
char D_008418D8[13] = {
#include "lobby/008418D8.inc"
};
char D_008418E8[0xC] = {
#include "lobby/008418E8.inc"
};
char D_008418F8[0xC] = {
#include "lobby/008418F8.inc"
};
char D_00841908[0xC] = {
#include "lobby/00841908.inc"
};
char D_00841918[0x8] = {
#include "lobby/00841918.inc"
};
char D_00841920[0x10] = {
#include "lobby/00841920.inc"
};
char D_00841930[0x10] = {
#include "lobby/00841930.inc"
};
char D_00841940[0x20] = {
#include "lobby/00841940.inc"
};
char D_00841960[0x20] = {
#include "lobby/00841960.inc"
};
char D_00841980[0x20] = {
#include "lobby/00841980.inc"
};
char D_008419A0[0x30] = {
#include "lobby/008419A0.inc"
};
char D_008419D0[0x30] = {
#include "lobby/008419D0.inc"
};
char D_00841A00[0x70] = {
#include "lobby/00841A00.inc"
};
char D_00841A70[0x30] = {
#include "lobby/00841A70.inc"
};
char D_00841AA0[0x30] = {
#include "lobby/00841AA0.inc"
};
char D_00841AD0[0x70] = {
#include "lobby/00841AD0.inc"
};
char D_00841B40[0x30] = {
#include "lobby/00841B40.inc"
};
char D_00841B70[0x30] = {
#include "lobby/00841B70.inc"
};
char D_00841BA0[0x70] = {
#include "lobby/00841BA0.inc"
};
char D_00841C10[0x60] = {
#include "lobby/00841C10.inc"
};
char D_00841C70[0x70] = {
#include "lobby/00841C70.inc"
};
char D_00841CE0[0x40] = {
#include "lobby/00841CE0.inc"
};
char D_00841D20[0x58] = {
#include "lobby/00841D20.inc"
};
char D_00841D78[0xC] = {
#include "lobby/00841D78.inc"
};
char D_00841D88[0xC] = {
#include "lobby/00841D88.inc"
};
char D_00841D98[0xC] = {
#include "lobby/00841D98.inc"
};
char D_00841DA8[0xC] = {
#include "lobby/00841DA8.inc"
};
char D_00841DB8[0xC] = {
#include "lobby/00841DB8.inc"
};
char D_00841DD0[0x18] = {
#include "lobby/00841DD0.inc"
};
char D_00841DE8[0xC] = {
#include "lobby/00841DE8.inc"
};
char D_00841E00[0x18] = {
#include "lobby/00841E00.inc"
};
char D_00841E18[13] = {
#include "lobby/00841E18.inc"
};
char D_00841E30[0x30] = {
#include "lobby/00841E30.inc"
};
char D_00841E60[0x20] = {
#include "lobby/00841E60.inc"
};
char D_00841E80[0x20] = {
#include "lobby/00841E80.inc"
};
char D_00841EA0[0x8] = {
#include "lobby/00841EA0.inc"
};
char D_00841EA8[0x8] = {
#include "lobby/00841EA8.inc"
};
char D_00841EB0[0x10] = {
#include "lobby/00841EB0.inc"
};
char D_00841EC0[0x40] = {
#include "lobby/00841EC0.inc"
};
char D_00841F00[0x60] = {
#include "lobby/00841F00.inc"
};
char D_00841F60[0x50] = {
#include "lobby/00841F60.inc"
};
char D_00841FB0[0x30] = {
#include "lobby/00841FB0.inc"
};
char D_00841FE0[0x10] = {
#include "lobby/00841FE0.inc"
};
char D_00841FF0[0x8] = {
#include "lobby/00841FF0.inc"
};
char D_00841FF8[0x8] = {
#include "lobby/00841FF8.inc"
};
char D_00842000[0x8] = {
#include "lobby/00842000.inc"
};
char D_00842008[0x8] = {
#include "lobby/00842008.inc"
};
char D_00842010[0x8] = {
#include "lobby/00842010.inc"
};
char D_00842018[0x8] = {
#include "lobby/00842018.inc"
};
char D_00842020[0x10] = {
#include "lobby/00842020.inc"
};
char D_00842030[0x10] = {
#include "lobby/00842030.inc"
};
char D_00842040[0x10] = {
#include "lobby/00842040.inc"
};
char D_00842050[0x10] = {
#include "lobby/00842050.inc"
};
char D_00842060[0x10] = {
#include "lobby/00842060.inc"
};
char D_00842070[0x10] = {
#include "lobby/00842070.inc"
};
char D_00842080[0x10] = {
#include "lobby/00842080.inc"
};
char D_00842090[0x10] = {
#include "lobby/00842090.inc"
};
char D_008420A0[0x8] = {
#include "lobby/008420A0.inc"
};
char D_008420A8[13] = {
#include "lobby/008420A8.inc"
};
char D_008420C0[0x20] = {
#include "lobby/008420C0.inc"
};
char D_008420E0[0x20] = {
#include "lobby/008420E0.inc"
};
char D_00842100[0x20] = {
#include "lobby/00842100.inc"
};
char D_00842120[0x20] = {
#include "lobby/00842120.inc"
};
char D_00842140[0x20] = {
#include "lobby/00842140.inc"
};
char D_00842160[0x10] = {
#include "lobby/00842160.inc"
};
char D_00842170[0x20] = {
#include "lobby/00842170.inc"
};
char D_00842190[0x20] = {
#include "lobby/00842190.inc"
};
char D_008421B0[0x20] = {
#include "lobby/008421B0.inc"
};
char D_008421D0[0x20] = {
#include "lobby/008421D0.inc"
};
char D_008421F0[0x20] = {
#include "lobby/008421F0.inc"
};
char D_00842210[0x20] = {
#include "lobby/00842210.inc"
};
char D_00842230[0x30] = {
#include "lobby/00842230.inc"
};
char D_00842260[0x30] = {
#include "lobby/00842260.inc"
};
char D_00842290[0x30] = {
#include "lobby/00842290.inc"
};
char D_008422C0[0x30] = {
#include "lobby/008422C0.inc"
};
char D_008422F0[0x30] = {
#include "lobby/008422F0.inc"
};
char D_00842320[0x20] = {
#include "lobby/00842320.inc"
};
char D_00842340[0x20] = {
#include "lobby/00842340.inc"
};
char D_00842360[0x20] = {
#include "lobby/00842360.inc"
};
char D_00842380[0x20] = {
#include "lobby/00842380.inc"
};
char D_008423A0[0x20] = {
#include "lobby/008423A0.inc"
};
char D_008423C0[0x20] = {
#include "lobby/008423C0.inc"
};
char D_008423E0[0x20] = {
#include "lobby/008423E0.inc"
};
char D_00842400[0x20] = {
#include "lobby/00842400.inc"
};
char D_00842420[0x20] = {
#include "lobby/00842420.inc"
};
char D_00842440[0x30] = {
#include "lobby/00842440.inc"
};
char D_00842470[0x20] = {
#include "lobby/00842470.inc"
};
char D_00842490[0x20] = {
#include "lobby/00842490.inc"
};
char D_008424B0[0x20] = {
#include "lobby/008424B0.inc"
};
char D_008424D0[0x10] = {
#include "lobby/008424D0.inc"
};
char D_008424E0[0x20] = {
#include "lobby/008424E0.inc"
};
char D_00842500[0x30] = {
#include "lobby/00842500.inc"
};
char D_00842530[0x30] = {
#include "lobby/00842530.inc"
};
char D_00842560[0x20] = {
#include "lobby/00842560.inc"
};
char D_00842580[0x30] = {
#include "lobby/00842580.inc"
};
char D_008425B0[0x28] = {
#include "lobby/008425B0.inc"
};
char D_008425D8[13] = {
#include "lobby/008425D8.inc"
};
char D_008425E8[13] = {
#include "lobby/008425E8.inc"
};
char D_00842600[0x20] = {
#include "lobby/00842600.inc"
};
char D_00842620[0x40] = {
#include "lobby/00842620.inc"
};
char D_00842660[0x20] = {
#include "lobby/00842660.inc"
};
char D_00842680[0x20] = {
#include "lobby/00842680.inc"
};
char D_008426A0[0x30] = {
#include "lobby/008426A0.inc"
};
char D_008426D0[0x30] = {
#include "lobby/008426D0.inc"
};
char D_00842700[0x20] = {
#include "lobby/00842700.inc"
};
char D_00842720[0x30] = {
#include "lobby/00842720.inc"
};
char D_00842750[0x20] = {
#include "lobby/00842750.inc"
};
char D_00842770[0x20] = {
#include "lobby/00842770.inc"
};
char D_00842790[0x20] = {
#include "lobby/00842790.inc"
};
char D_008427B0[0x20] = {
#include "lobby/008427B0.inc"
};
char D_008427D0[0x18] = {
#include "lobby/008427D0.inc"
};
char D_008427E8[0xC] = {
#include "lobby/008427E8.inc"
};
char D_008427F8[13] = {
#include "lobby/008427F8.inc"
};
char D_00842810[0x30] = {
#include "lobby/00842810.inc"
};
char D_00842840[0x20] = {
#include "lobby/00842840.inc"
};
char D_00842860[0x20] = {
#include "lobby/00842860.inc"
};
char D_00842880[0x20] = {
#include "lobby/00842880.inc"
};
char D_008428A0[0x10] = {
#include "lobby/008428A0.inc"
};
char D_008428B0[0x18] = {
#include "lobby/008428B0.inc"
};
char D_008428C8[13] = {
#include "lobby/008428C8.inc"
};
char D_008428D8[0xC] = {
#include "lobby/008428D8.inc"
};
char D_008428E8[0xC] = {
#include "lobby/008428E8.inc"
};
char D_00842900[0x20] = {
#include "lobby/00842900.inc"
};
char D_00842920[0x10] = {
#include "lobby/00842920.inc"
};
char D_00842930[0x10] = {
#include "lobby/00842930.inc"
};
char D_00842940[0x18] = {
#include "lobby/00842940.inc"
};
char D_00842958[0xC] = {
#include "lobby/00842958.inc"
};
char D_00842968[0xC] = {
#include "lobby/00842968.inc"
};
char D_00842978[15] = {
#include "lobby/00842978.inc"
};
char D_00842988[0x8] = {
#include "lobby/00842988.inc"
};
char D_00842990[0x10] = {
#include "lobby/00842990.inc"
};
char D_008429A0[0x18] = {
#include "lobby/008429A0.inc"
};
char D_008429B8[0xC] = {
#include "lobby/008429B8.inc"
};
char D_008429C8[0xC] = {
#include "lobby/008429C8.inc"
};
char D_008429D8[15] = {
#include "lobby/008429D8.inc"
};
char D_008429E8[0xC] = {
#include "lobby/008429E8.inc"
};
char D_008429F8[15] = {
#include "lobby/008429F8.inc"
};
char D_00842A10[0x18] = {
#include "lobby/00842A10.inc"
};
char D_00842A28[0xC] = {
#include "lobby/00842A28.inc"
};
char D_00842A38[0x8] = {
#include "lobby/00842A38.inc"
};
char D_00842A40[0x18] = {
#include "lobby/00842A40.inc"
};
char D_00842A58[0xC] = {
#include "lobby/00842A58.inc"
};
char D_00842A70[0x18] = {
#include "lobby/00842A70.inc"
};
char D_00842A88[0xC] = {
#include "lobby/00842A88.inc"
};
char D_00842A98[0xC] = {
#include "lobby/00842A98.inc"
};
char D_00842AB0[0x20] = {
#include "lobby/00842AB0.inc"
};
char D_00842AD0[0x20] = {
#include "lobby/00842AD0.inc"
};
char D_00842AF0[0x8] = {
#include "lobby/00842AF0.inc"
};
char D_00842AF8[0xC] = {
#include "lobby/00842AF8.inc"
};
char D_00842B08[0xC] = {
#include "lobby/00842B08.inc"
};
char D_00842B18[0xC] = {
#include "lobby/00842B18.inc"
};
char D_00842B28[0xC] = {
#include "lobby/00842B28.inc"
};
char D_00842B40[0x20] = {
#include "lobby/00842B40.inc"
};
char D_00842B60[0xA0] = {
#include "lobby/00842B60.inc"
};
char D_00842C00[0x30] = {
#include "lobby/00842C00.inc"
};
char D_00842C30[0x30] = {
#include "lobby/00842C30.inc"
};
char D_00842C60[0x20] = {
#include "lobby/00842C60.inc"
};
char D_00842C80[0x80] = {
#include "lobby/00842C80.inc"
};
char D_00842D00[0x30] = {
#include "lobby/00842D00.inc"
};
char D_00842D30[0x20] = {
#include "lobby/00842D30.inc"
};
char D_00842D50[0x40] = {
#include "lobby/00842D50.inc"
};
char D_00842D90[0x30] = {
#include "lobby/00842D90.inc"
};
char D_00842DC0[0x30] = {
#include "lobby/00842DC0.inc"
};
char D_00842DF0[0x20] = {
#include "lobby/00842DF0.inc"
};
char D_00842E10[0x30] = {
#include "lobby/00842E10.inc"
};
char D_00842E40[0x80] = {
#include "lobby/00842E40.inc"
};
char D_00842EC0[0x40] = {
#include "lobby/00842EC0.inc"
};
char D_00842F00[0x30] = {
#include "lobby/00842F00.inc"
};
char D_00842F30[0x30] = {
#include "lobby/00842F30.inc"
};
char D_00842F60[0x60] = {
#include "lobby/00842F60.inc"
};
char D_00842FC0[0x20] = {
#include "lobby/00842FC0.inc"
};
char D_00842FE0[0xB0] = {
#include "lobby/00842FE0.inc"
};
char D_00843090[0xA0] = {
#include "lobby/00843090.inc"
};
char D_00843130[0x90] = {
#include "lobby/00843130.inc"
};
char D_008431C0[0xA0] = {
#include "lobby/008431C0.inc"
};
char D_00843260[0x30] = {
#include "lobby/00843260.inc"
};
char D_00843290[0x40] = {
#include "lobby/00843290.inc"
};
char D_008432D0[0x20] = {
#include "lobby/008432D0.inc"
};
char D_008432F0[0x50] = {
#include "lobby/008432F0.inc"
};
char D_00843340[0xA0] = {
#include "lobby/00843340.inc"
};
char D_008433E0[0xA0] = {
#include "lobby/008433E0.inc"
};
char D_00843480[0x30] = {
#include "lobby/00843480.inc"
};
char D_008434B0[0x40] = {
#include "lobby/008434B0.inc"
};
char D_008434F0[0x30] = {
#include "lobby/008434F0.inc"
};
char D_00843520[0x90] = {
#include "lobby/00843520.inc"
};
char D_008435B0[0xB0] = {
#include "lobby/008435B0.inc"
};
char D_00843660[0x90] = {
#include "lobby/00843660.inc"
};
char D_008436F0[0x40] = {
#include "lobby/008436F0.inc"
};
char D_00843730[0x40] = {
#include "lobby/00843730.inc"
};
char D_00843770[0x40] = {
#include "lobby/00843770.inc"
};
char D_008437B0[0x50] = {
#include "lobby/008437B0.inc"
};
char D_00843800[0x40] = {
#include "lobby/00843800.inc"
};
char D_00843840[0x40] = {
#include "lobby/00843840.inc"
};
char D_00843880[0x40] = {
#include "lobby/00843880.inc"
};
char D_008438C0[0x30] = {
#include "lobby/008438C0.inc"
};
char D_008438F0[0x90] = {
#include "lobby/008438F0.inc"
};
char D_00843980[0x8] = {
#include "lobby/00843980.inc"
};
char D_00843988[0xC] = {
#include "lobby/00843988.inc"
};
char D_00843998[0xC] = {
#include "lobby/00843998.inc"
};
char D_008439A8[0x8] = {
#include "lobby/008439A8.inc"
};
char D_008439B0[0x8] = {
#include "lobby/008439B0.inc"
};
char D_008439B8[0x8] = {
#include "lobby/008439B8.inc"
};
char D_008439C0[0x8] = {
#include "lobby/008439C0.inc"
};
char D_008439C8[0xC] = {
#include "lobby/008439C8.inc"
};
char D_008439D8[0x8] = {
#include "lobby/008439D8.inc"
};
char D_008439E0[0x8] = {
#include "lobby/008439E0.inc"
};
char D_008439E8[0x8] = {
#include "lobby/008439E8.inc"
};
char D_008439F0[0x8] = {
#include "lobby/008439F0.inc"
};
char D_008439F8[0x8] = {
#include "lobby/008439F8.inc"
};
char D_00843A00[0x8] = {
#include "lobby/00843A00.inc"
};
char D_00843A08[0xC] = {
#include "lobby/00843A08.inc"
};
char D_00843A18[0x8] = {
#include "lobby/00843A18.inc"
};
char D_00843A20[0x8] = {
#include "lobby/00843A20.inc"
};
char D_00843A28[13] = {
#include "lobby/00843A28.inc"
};
char D_00843A40[0x20] = {
#include "lobby/00843A40.inc"
};
char D_00843A60[0x18] = {
#include "lobby/00843A60.inc"
};
char D_00843A78[0xC] = {
#include "lobby/00843A78.inc"
};
char D_00843A88[0xC] = {
#include "lobby/00843A88.inc"
};
char D_00843AA0[0x18] = {
#include "lobby/00843AA0.inc"
};
char D_00843AB8[0xC] = {
#include "lobby/00843AB8.inc"
};
char D_00843AC8[13] = {
#include "lobby/00843AC8.inc"
};
char D_00843AE0[0x18] = {
#include "lobby/00843AE0.inc"
};
char D_00843AF8[0x8] = {
#include "lobby/00843AF8.inc"
};
char D_00843B00[0x10] = {
#include "lobby/00843B00.inc"
};
char D_00843B10[0x10] = {
#include "lobby/00843B10.inc"
};
char D_00843B20[0x10] = {
#include "lobby/00843B20.inc"
};
char D_00843B30[0x10] = {
#include "lobby/00843B30.inc"
};
char D_00843B40[0x20] = {
#include "lobby/00843B40.inc"
};
char D_00843B60[0x20] = {
#include "lobby/00843B60.inc"
};
char D_00843B80[0x20] = {
#include "lobby/00843B80.inc"
};
char D_00843BA0[0x20] = {
#include "lobby/00843BA0.inc"
};
char D_00843BC0[0x20] = {
#include "lobby/00843BC0.inc"
};
char D_00843BE0[0x20] = {
#include "lobby/00843BE0.inc"
};
char D_00843C00[0x20] = {
#include "lobby/00843C00.inc"
};
char D_00843C20[0x20] = {
#include "lobby/00843C20.inc"
};
char D_00843C40[0x8] = {
#include "lobby/00843C40.inc"
};
char D_00843C48[15] = {
#include "lobby/00843C48.inc"
};
char D_00843C58[13] = {
#include "lobby/00843C58.inc"
};
char D_00843C68[15] = {
#include "lobby/00843C68.inc"
};
char D_00843C80[0x20] = {
#include "lobby/00843C80.inc"
};
char D_00843CA0[0x20] = {
#include "lobby/00843CA0.inc"
};
char D_00843CC0[0x20] = {
#include "lobby/00843CC0.inc"
};
char D_00843CE0[0x30] = {
#include "lobby/00843CE0.inc"
};
char D_00843D10[0x20] = {
#include "lobby/00843D10.inc"
};
char D_00843D30[0x10] = {
#include "lobby/00843D30.inc"
};
char D_00843D40[0x10] = {
#include "lobby/00843D40.inc"
};
char D_00843D50[0x10] = {
#include "lobby/00843D50.inc"
};
char D_00843D60[0x10] = {
#include "lobby/00843D60.inc"
};
char D_00843D70[0x18] = {
#include "lobby/00843D70.inc"
};
char D_00843D88[13] = {
#include "lobby/00843D88.inc"
};
char D_00843D98[15] = {
#include "lobby/00843D98.inc"
};
char D_00843DB0[0x20] = {
#include "lobby/00843DB0.inc"
};
char D_00843DD0[0x18] = {
#include "lobby/00843DD0.inc"
};
char D_00843DE8[0x8] = {
#include "lobby/00843DE8.inc"
};
char D_00843DF0[0x8] = {
#include "lobby/00843DF0.inc"
};
char D_00843DF8[0xC] = {
#include "lobby/00843DF8.inc"
};
char D_00843E10[0x18] = {
#include "lobby/00843E10.inc"
};
char D_00843E28[0x8] = {
#include "lobby/00843E28.inc"
};
char D_00843E30[0x10] = {
#include "lobby/00843E30.inc"
};
char D_00843E40[0x10] = {
#include "lobby/00843E40.inc"
};
char D_00843E50[0x10] = {
#include "lobby/00843E50.inc"
};
char D_00843E60[0x10] = {
#include "lobby/00843E60.inc"
};
char D_00843E70[0x10] = {
#include "lobby/00843E70.inc"
};
char D_00843E80[0x10] = {
#include "lobby/00843E80.inc"
};
char D_00843E90[0x20] = {
#include "lobby/00843E90.inc"
};
char D_00843EB0[0x20] = {
#include "lobby/00843EB0.inc"
};
char D_00843ED0[0x10] = {
#include "lobby/00843ED0.inc"
};
char D_00843EE0[0x10] = {
#include "lobby/00843EE0.inc"
};
char D_00843EF0[0x8] = {
#include "lobby/00843EF0.inc"
};
char D_00843EF8[0xC] = {
#include "lobby/00843EF8.inc"
};
char D_00843F08[0xC] = {
#include "lobby/00843F08.inc"
};
char D_00843F18[15] = {
#include "lobby/00843F18.inc"
};
char D_00843F28[0xC] = {
#include "lobby/00843F28.inc"
};
char D_00843F38[0xC] = {
#include "lobby/00843F38.inc"
};
char D_00843F48[15] = {
#include "lobby/00843F48.inc"
};
char D_00843F58[15] = {
#include "lobby/00843F58.inc"
};
char D_00843F68[0x8] = {
#include "lobby/00843F68.inc"
};
char D_00843F70[0x18] = {
#include "lobby/00843F70.inc"
};
char D_00843F88[0x8] = {
#include "lobby/00843F88.inc"
};
char D_00843F90[0x10] = {
#include "lobby/00843F90.inc"
};
char D_00843FA0[0x10] = {
#include "lobby/00843FA0.inc"
};
char D_00843FB0[0x10] = {
#include "lobby/00843FB0.inc"
};
char D_00843FC0[0x10] = {
#include "lobby/00843FC0.inc"
};
char D_00843FD0[0x10] = {
#include "lobby/00843FD0.inc"
};
char D_00843FE0[0x20] = {
#include "lobby/00843FE0.inc"
};
char D_00844000[0x20] = {
#include "lobby/00844000.inc"
};
char D_00844020[0x20] = {
#include "lobby/00844020.inc"
};
char D_00844040[0x40] = {
#include "lobby/00844040.inc"
};
char D_00844080[0x40] = {
#include "lobby/00844080.inc"
};
char D_008440C0[0x30] = {
#include "lobby/008440C0.inc"
};
char D_008440F0[0x30] = {
#include "lobby/008440F0.inc"
};
char D_00844120[0x40] = {
#include "lobby/00844120.inc"
};
char D_00844160[0x60] = {
#include "lobby/00844160.inc"
};
char D_008441C0[0x20] = {
#include "lobby/008441C0.inc"
};
char D_008441E0[0x40] = {
#include "lobby/008441E0.inc"
};
char D_00844220[0x40] = {
#include "lobby/00844220.inc"
};
char D_00844260[0x50] = {
#include "lobby/00844260.inc"
};
char D_008442B0[0x40] = {
#include "lobby/008442B0.inc"
};
char D_008442F0[0x30] = {
#include "lobby/008442F0.inc"
};
char D_00844320[0x20] = {
#include "lobby/00844320.inc"
};
char D_00844340[0x40] = {
#include "lobby/00844340.inc"
};
char D_00844380[0x40] = {
#include "lobby/00844380.inc"
};
char D_008443C0[0x20] = {
#include "lobby/008443C0.inc"
};
char D_008443E0[0x50] = {
#include "lobby/008443E0.inc"
};
char D_00844430[0x30] = {
#include "lobby/00844430.inc"
};
char D_00844460[0x30] = {
#include "lobby/00844460.inc"
};
char D_00844490[0x30] = {
#include "lobby/00844490.inc"
};
char D_008444C0[0x30] = {
#include "lobby/008444C0.inc"
};
char D_008444F0[0x30] = {
#include "lobby/008444F0.inc"
};
char D_00844520[0x30] = {
#include "lobby/00844520.inc"
};
char D_00844550[0x20] = {
#include "lobby/00844550.inc"
};
char D_00844570[0x60] = {
#include "lobby/00844570.inc"
};
char D_008445D0[0x50] = {
#include "lobby/008445D0.inc"
};
char D_00844620[0x30] = {
#include "lobby/00844620.inc"
};
char D_00844650[0x20] = {
#include "lobby/00844650.inc"
};
char D_00844670[0x20] = {
#include "lobby/00844670.inc"
};
char D_00844690[0x30] = {
#include "lobby/00844690.inc"
};
char D_008446C0[0x30] = {
#include "lobby/008446C0.inc"
};
char D_008446F0[0x40] = {
#include "lobby/008446F0.inc"
};
char D_00844730[0x30] = {
#include "lobby/00844730.inc"
};
char D_00844760[0x30] = {
#include "lobby/00844760.inc"
};
char D_00844790[0x30] = {
#include "lobby/00844790.inc"
};
char D_008447C0[0x30] = {
#include "lobby/008447C0.inc"
};
char D_008447F0[0x30] = {
#include "lobby/008447F0.inc"
};
char D_00844820[0x40] = {
#include "lobby/00844820.inc"
};
char D_00844860[0x40] = {
#include "lobby/00844860.inc"
};
char D_008448A0[0x70] = {
#include "lobby/008448A0.inc"
};
char D_00844910[0x40] = {
#include "lobby/00844910.inc"
};
char D_00844950[0x30] = {
#include "lobby/00844950.inc"
};
char D_00844980[0x60] = {
#include "lobby/00844980.inc"
};
char D_008449E0[0x30] = {
#include "lobby/008449E0.inc"
};
char D_00844A10[0x30] = {
#include "lobby/00844A10.inc"
};
char D_00844A40[0x30] = {
#include "lobby/00844A40.inc"
};
char D_00844A70[0x30] = {
#include "lobby/00844A70.inc"
};
char D_00844AA0[0x40] = {
#include "lobby/00844AA0.inc"
};
char D_00844AE0[0x70] = {
#include "lobby/00844AE0.inc"
};
char D_00844B50[0x50] = {
#include "lobby/00844B50.inc"
};
char D_00844BA0[0x30] = {
#include "lobby/00844BA0.inc"
};
char D_00844BD0[0x20] = {
#include "lobby/00844BD0.inc"
};
char D_00844BF0[0x40] = {
#include "lobby/00844BF0.inc"
};
char D_00844C30[0x20] = {
#include "lobby/00844C30.inc"
};
char D_00844C50[0x50] = {
#include "lobby/00844C50.inc"
};
char D_00844CA0[0x60] = {
#include "lobby/00844CA0.inc"
};
char D_00844D00[0x60] = {
#include "lobby/00844D00.inc"
};
char D_00844D60[0x30] = {
#include "lobby/00844D60.inc"
};
char D_00844D90[0x40] = {
#include "lobby/00844D90.inc"
};
char D_00844DD0[0x40] = {
#include "lobby/00844DD0.inc"
};
char D_00844E10[0x20] = {
#include "lobby/00844E10.inc"
};
char D_00844E30[0x20] = {
#include "lobby/00844E30.inc"
};
char D_00844E50[0x20] = {
#include "lobby/00844E50.inc"
};
char D_00844E70[0x30] = {
#include "lobby/00844E70.inc"
};
char D_00844EA0[0x20] = {
#include "lobby/00844EA0.inc"
};
char D_00844EC0[0x20] = {
#include "lobby/00844EC0.inc"
};
char D_00844EE0[0x20] = {
#include "lobby/00844EE0.inc"
};
char D_00844F00[0x30] = {
#include "lobby/00844F00.inc"
};
char D_00844F30[0x30] = {
#include "lobby/00844F30.inc"
};
char D_00844F60[0x20] = {
#include "lobby/00844F60.inc"
};
char D_00844F80[0x50] = {
#include "lobby/00844F80.inc"
};
char D_00844FD0[0x60] = {
#include "lobby/00844FD0.inc"
};
char D_00845030[0x60] = {
#include "lobby/00845030.inc"
};
char D_00845090[0x60] = {
#include "lobby/00845090.inc"
};
char D_008450F0[0x30] = {
#include "lobby/008450F0.inc"
};
char D_00845120[0x60] = {
#include "lobby/00845120.inc"
};
char D_00845180[0x60] = {
#include "lobby/00845180.inc"
};
char D_008451E0[0x60] = {
#include "lobby/008451E0.inc"
};
char D_00845240[0x30] = {
#include "lobby/00845240.inc"
};
char D_00845270[0x30] = {
#include "lobby/00845270.inc"
};
char D_008452A0[0x30] = {
#include "lobby/008452A0.inc"
};
char D_008452D0[0x40] = {
#include "lobby/008452D0.inc"
};
char D_00845310[0x60] = {
#include "lobby/00845310.inc"
};
char D_00845370[0x30] = {
#include "lobby/00845370.inc"
};
char D_008453A0[0x30] = {
#include "lobby/008453A0.inc"
};
char D_008453D0[0x30] = {
#include "lobby/008453D0.inc"
};
char D_00845400[0x60] = {
#include "lobby/00845400.inc"
};
char D_00845460[0x50] = {
#include "lobby/00845460.inc"
};
char D_008454B0[0x30] = {
#include "lobby/008454B0.inc"
};
char D_008454E0[0x30] = {
#include "lobby/008454E0.inc"
};
char D_00845510[0x30] = {
#include "lobby/00845510.inc"
};
char D_00845540[0x60] = {
#include "lobby/00845540.inc"
};
char D_008455A0[0x110] = {
#include "lobby/008455A0.inc"
};
char D_008456B0[0x90] = {
#include "lobby/008456B0.inc"
};
char D_00845740[0x30] = {
#include "lobby/00845740.inc"
};
char D_00845770[0x30] = {
#include "lobby/00845770.inc"
};
char D_008457A0[0x50] = {
#include "lobby/008457A0.inc"
};
char D_008457F0[0x50] = {
#include "lobby/008457F0.inc"
};
char D_00845840[0x30] = {
#include "lobby/00845840.inc"
};
char D_00845870[0x40] = {
#include "lobby/00845870.inc"
};
char D_008458B0[0x30] = {
#include "lobby/008458B0.inc"
};
char D_008458E0[0x30] = {
#include "lobby/008458E0.inc"
};
char D_00845910[0x20] = {
#include "lobby/00845910.inc"
};
char D_00845930[0x50] = {
#include "lobby/00845930.inc"
};
char D_00845980[0x30] = {
#include "lobby/00845980.inc"
};
char D_008459B0[0x30] = {
#include "lobby/008459B0.inc"
};
char D_008459E0[0x90] = {
#include "lobby/008459E0.inc"
};
char D_00845A70[0xA0] = {
#include "lobby/00845A70.inc"
};
char D_00845B10[0x60] = {
#include "lobby/00845B10.inc"
};
char D_00845B70[0xC0] = {
#include "lobby/00845B70.inc"
};
char D_00845C30[0x90] = {
#include "lobby/00845C30.inc"
};
char D_00845CC0[0xD0] = {
#include "lobby/00845CC0.inc"
};
char D_00845D90[0x100] = {
#include "lobby/00845D90.inc"
};
char D_00845E90[0x20] = {
#include "lobby/00845E90.inc"
};
char D_00845EB0[0x20] = {
#include "lobby/00845EB0.inc"
};
char D_00845ED0[0x30] = {
#include "lobby/00845ED0.inc"
};
char D_00845F00[0x20] = {
#include "lobby/00845F00.inc"
};
char D_00845F20[0x20] = {
#include "lobby/00845F20.inc"
};
char D_00845F40[0x30] = {
#include "lobby/00845F40.inc"
};
char D_00845F70[0x60] = {
#include "lobby/00845F70.inc"
};
char D_00845FD0[0x60] = {
#include "lobby/00845FD0.inc"
};
char D_00846030[0x60] = {
#include "lobby/00846030.inc"
};
char D_00846090[0x60] = {
#include "lobby/00846090.inc"
};
char D_008460F0[0x80] = {
#include "lobby/008460F0.inc"
};
char D_00846170[0x30] = {
#include "lobby/00846170.inc"
};
char D_008461A0[0x50] = {
#include "lobby/008461A0.inc"
};
char D_008461F0[0x60] = {
#include "lobby/008461F0.inc"
};
char D_00846250[0x30] = {
#include "lobby/00846250.inc"
};
char D_00846280[0x50] = {
#include "lobby/00846280.inc"
};
char D_008462D0[0x40] = {
#include "lobby/008462D0.inc"
};
char D_00846310[0x40] = {
#include "lobby/00846310.inc"
};
char D_00846350[0x20] = {
#include "lobby/00846350.inc"
};
char D_00846370[0x30] = {
#include "lobby/00846370.inc"
};
char D_008463A0[0x60] = {
#include "lobby/008463A0.inc"
};
char D_00846400[0x30] = {
#include "lobby/00846400.inc"
};
char D_00846430[0x50] = {
#include "lobby/00846430.inc"
};
char D_00846480[0x30] = {
#include "lobby/00846480.inc"
};
char D_008464B0[0x40] = {
#include "lobby/008464B0.inc"
};
char D_008464F0[0xB0] = {
#include "lobby/008464F0.inc"
};
char D_008465A0[0x80] = {
#include "lobby/008465A0.inc"
};
char D_00846620[0x30] = {
#include "lobby/00846620.inc"
};
char D_00846650[0x50] = {
#include "lobby/00846650.inc"
};
char D_008466A0[0x30] = {
#include "lobby/008466A0.inc"
};
#pragma rodata off
