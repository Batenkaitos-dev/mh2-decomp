/*
 * The item shop: a townsperson's counter menu (buy, sell, and at some
 * counters the shooting gallery or the townsperson's offers), the buy window
 * over the townsperson's stock and the sell window over the pouch, the item
 * info panel both windows draw under their list. Three of the lobby menus'
 * helpers end it: Win_DrawText (a plain window with a text), LobbyPad_Read
 * (the menus' pad read) and LobbyPad_Get (the buttons it read this frame).
 *
 * The menu's work is a ShopMenu in gMenuWork (ItemShop_Open, the file's
 * first function, sets it up; Town_UpdateMode runs ItemShop_Update while the
 * menu is open, and TownMenu_Close ends it). Its
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
#include "em.h"
#include "lobby_npc.h"
#include "quest.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "camera.h"
#include "village_cond.h"
#include "window.h"
#include "item_shop.h"
#include "font.h"
#include "game_util.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_steps.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_util.h"
#include "menu_widgets.h"
#include "name_tags.h"
#include "offer_menu.h"
#include "shooting_gallery.h"
#include "talk.h"
#include "town_frame.h"

void *memset(void *dst, s32 c, u32 n);
extern u8 gMenuWork[];
extern u8 gMenuInputLock;
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* Opens a townsperson's shop counter (TOWN_MODE_ITEM_SHOP; a ShopMenu in
 * gMenuWork, drawn by ItemShop_Draw): `npc` is the townsperson's Em, and
 * `sel`, when not negative, the line the cursor starts on. Townsperson 13
 * opens with LOBBY_SE_SELECT; the others with LOBBY_SE_MENU_OPEN, except
 * when the menu is reopened from the shooting gallery (TOWN_MODE_GALLERY). */
void ItemShop_Open(u8 *npc, s8 sel)
{
    ShopMenu *w;
    u8 *id;
    u8 fromGallery;

    w = (ShopMenu *)gMenuWork;
    id = &((NpcWork *)&((Em *)npc)->work)->id;
    fromGallery = 0;
    gTownMenuOpen = 1;
    if (gTownMode == TOWN_MODE_GALLERY) {
        fromGallery = 1;
    }
    gTownMode = TOWN_MODE_ITEM_SHOP;
    memset(gMenuWork, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(ItemShop_Draw, 0, 0, 0);
    gMenuInputLock = 6; /* frames without input */
    w->person = npc;
    w->npc = *id;
    w->list.kind = *id;
    if (sel >= 0) {
        w->cur = sel;
    }
    if (*id == 13) {
        LobbySe_Play(LOBBY_SE_SELECT);
    } else if (fromGallery == 0) {
        LobbySe_Play(LOBBY_SE_MENU_OPEN);
    }
}

/* Closes the shop counter: back to TOWN_MODE_FREE, no menu drawn, the
 * camera's focus on the townsperson ended, LOBBY_SE_CANCEL. */
void TownMenu_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
}

struct Em;

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
                LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
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
    WinSlide_Step(&list->win, 640, 270);
}

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
        switch (WinSlide_Step(&list->win, 640, 270)) {
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
    case 40:
        condList = gShopCondStockNpc28;
        break;
    case 44:
        if (gLobby->townEvent == 1) {
            items = gShopStockNpc2C[1];
        } else {
            items = gShopStockNpc2C[0];
        }
        break;
    case 45:
        if (gLobby->townEvent == 3) {
            items = gShopStockNpc2D[1];
        } else {
            items = gShopStockNpc2D[0];
        }
        break;
    case 46:
        if (gLobby->townEvent == 2) {
            items = gShopStockNpc2E[1];
        } else {
            items = gShopStockNpc2E[0];
        }
        break;
    case NPC_GALLERY_KEEPER:
        condList = gShopCondStockNpc39;
        break;
    case 66:
        condList = gShopCondStockNpc42;
        break;
    case 80:
        if (Save_TestEventFlag(EVENT_FLAG_SONG_TRACK_3) == 1) {
            items = gShopStockNpc50Ev122;
        } else {
            items = gShopStockNpc50;
        }
        break;
    case 93:
        condList = gShopCondStockNpc5D;
        break;
    case 106:
        condList = gShopCondStockNpc6A;
        break;
    case 12:
    case 13:
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
    WinSlide_Step(&list->win, 640, 270);
    list->pouchHasRoom = SavePouch_HasFreeSlot();
    switch (list->kind) {
    case 13:
    case 12:
        list->backdrop = 0;
        break;
    case 7:
    case NPC_GALLERY_KEEPER:
    case 66:
    case 80:
    case 93:
        list->backdrop = 2;
        break;
    case 11:
    case 45:
        list->backdrop = 3;
        break;
    case 8:
    case 44:
        list->backdrop = 4;
        break;
    default:
        list->backdrop = 1;
        break;
    }
}

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
        switch (WinSlide_Step(&list->win, 640, 270)) {
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

/* True for items 705 and 706, which are bought once and kept as
 * obtained rather than in the pouch. */
static s32 Shop_IsOnceOnlyItem(u16 item)
{
    switch (item & 0xFFFF) {
    case 705:
    case 706:
        return 1;
    }
    return 0;
}

/* An item's buy price; online it is halved during the town's sale event
 * (Town_IsSaleEvent). */
s32 Shop_GetBuyPrice(s32 item)
{
    s32 ret;

    ret = gItemData[item].price;
    if ((Game_IsOnline() == 1) && (Town_IsSaleEvent() == 1)) {
        ret = ret >> 1;
    }
    return ret;
}

/* What one of the item sells for. */
s32 Item_GetSellPrice(s32 item)
{
    return gItemData[item].value;
}

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

void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
s32 sprintf(char *buf, char *fmt, ...);
/* unit's own declaration: the definition returns s8; declared returning s32 as this file's other declaration of it is */
s32 Kbd_IsOpen(void);
extern char *gItemNames[];

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
 * The item's records are read through a byte offset (item * 24) the game
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
    labelY = y + 68;
    Font_SetSpacing(18, 18);
    right = (s16)(left + w);
    Font_SetPos(right - 60, labelY);
    Font_SetColor(color);
    Text_PrintfColored(gGameMenuLabels[MENU_LABEL_DETAILS]);
    if (Kbd_IsOpen() == 0 && (gPadState[0].pressed & PAD_SQUARE)) {
        pressOffset = 2;
    }
    Lobby_DrawButtonIcon((s16)(right - 60) - 26, labelY - 2 + pressOffset, 3);
    Font_SetSpacing(18, 18);
    y += 8;
    Item_DrawIcon(left + 10, y, 54, item);
    itemOffset = item * 24;
    Item_DrawRarity((s16)(left + 10) + 8, y + 54, (&gItemData[0].rarity)[itemOffset]);
    if (mode == 0) {
        Font_SetPos(left + 80, y);
        Font_SetColor(color);
        Text_PrintfColored("%s", gItemNames[item]);
        if (Item_IsDeco(item) == 1) {
            deco = Item_GetDecoration(item);
            Font_SetPos(left + 80, y += 23);
            sprintf(num, "%d", deco->size);
            Sjis_Widen(num, wide);
            Text_PrintfColored("%s%s", gTownLabels[TOWN_LABEL_NEEDED_SLOTS], wide);
        }
        Font_SetPos(left + 80, y += 23);
        Text_PrintfColored("%s%s", gCountLabels[COUNT_LABEL_HELD], gLobbyWords[LOBBY_WORD_TIMES]);
        Font_SetSpacing(28, 20);
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
            Font_SetPos(left + 152, countY - 1);
            if (have == 0) {
                Text_PrintfColored("%2d", have);
            } else {
                Text_Printf("%s", gLobbyWords[LOBBY_WORD_INFINITY]);
            }
            Font_SetColor(color);
            Font_SetPos((s16)(left + 152) + 28, countY - 1);
            if (total == 0) {
                Text_PrintfColored("(%3d)", total);
            } else {
                s16 countX = left + 152;

                Font_SetPos(countX + 28, countY - 1);
                Text_Printf("(", gLobbyWords[LOBBY_WORD_INFINITY]);
                Font_SetPos(countX + 35, countY - 1);
                Text_Printf(" %s", gLobbyWords[LOBBY_WORD_INFINITY]);
                Font_SetColor(color);
                Font_SetPos(countX + 28, countY - 1);
                Text_Printf("    )");
            }
        } else {
            countY = y;
            Font_SetPos(left + 152, countY - 1);
            if (have >= (&gItemData[0].max)[itemOffset]) {
                Text_Printf("~C02%2d", have);
                Font_SetPos(left + 152, countY - 1);
                Font_SetColor(color);
                Text_PrintfColored("  (%3d)", total);
                if (ItemBox_Room(item) == 0) {
                    Font_SetPos(left + 152, countY - 1);
                    Font_SetColor(0x807F212E);
                    Text_PrintfColored("   %3d", total);
                }
            } else {
                Font_SetColor(color);
                Text_PrintfColored("%2d(%3d)", have, total);
                if (ItemBox_Room(item) == 0 && ItemBox_Total(item) != 0) {
                    Font_SetPos(left + 152, countY - 1);
                    Text_Printf("   ~C02%3d", total);
                }
            }
        }
    } else if (color == 0x807F7F7F) {
        if (gTownMode == TOWN_MODE_WORKSHOP) {
            Deco_DrawDetails(item, left + 80, y, attr, 0);
        } else {
            Deco_DrawDetails(item, left + 80, y, attr | 0x10, 0);
        }
    } else {
        if (gTownMode == TOWN_MODE_WORKSHOP) {
            Deco_DrawDetails(item, left + 80, y, attr, 1);
        } else {
            Deco_DrawDetails(item, left + 80, y, attr | 0x10, 1);
        }
    }
}

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

void Tex_Bind(s32);
extern u32 gItemColors[];


/*
 * The item's icon at x, y, `size` high and 1.25 times as wide: its cell of
 * the system icons' texture (324; eight 32-pixel cells a row, cell
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

    itemOffset = item * 24;
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
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    Sprite_Draw(&rect);
    if (((u8 *)&gItemData[0].flags2)[itemOffset] & 1) {
        *left = 128;
        *top = 224;
        *right = *left + 32;
        *bottom = *top + 32;
        *color = -1;
        Sprite_Draw(&rect);
    }
}

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
    s16 y = 49;
    s16 i;
    s32 item;
    char num[16];
    char buf[32];

    y += 10;
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(270, 18, 160, 0xFFB0B000, gShopMenuTexts[SHOP_MENU_TEXT_BUY], 2, 1);
    switch (list->backdrop) {
    case 0:
        Icon_Draw(x, 49, 350, 306, 0xFFFFDDDD, ICON_PANEL);
        Icon_Draw(x + 115, 102, 120, 200, 0x60FFFFFF, 36);
        break;
    case 1:
        Icon_Draw(x, 49, 350, 306, 0xFFD3EDDC, ICON_PANEL);
        Icon_Draw(x + 100, 127, 150, 150, 0x60FFFFFF, 37);
        break;
    case 3:
        Icon_Draw(x, 49, 350, 306, 0xFFFFFFDD, ICON_PANEL);
        Icon_Draw(x + 80, 127, 190, 150, 0x60FFFFFF, 33);
        break;
    case 4:
        Icon_Draw(x, 49, 350, 306, 0xFFFFFFDD, ICON_PANEL);
        Icon_Draw(x + 100, 127, 150, 150, 0x60FFFFFF, 34);
        break;
    default:
        Icon_Draw(x, 49, 350, 306, 0xFFFFFFDD, ICON_PANEL);
        Icon_Draw(x + 100, 127, 150, 150, 0x60FFFFFF, 38);
        break;
    }
    Menu_DrawMoney(480, 16, 0);
    Font_SetColor(0x80000000);
    Menu_DrawPageNum(x + 250, y, list->page, list->pages, 1);
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
        y += 24;
        Font_SetPos(x + 46, y);
        Text_PrintfColored("%s", gItemNames[id]);
        Font_SetSpacing(20, 20);
        Font_SetPos(x + 248, y);
        Text_PrintfColored("% 7dz", price);
        Item_DrawIcon(x + 16, y - 1, 24, id);
        if (list->cur == i) {
            Menu_DrawCursorRow(x + 46, y, 250, 50, 0xC0106060, 0xFF106060);
        }
    }
    id = list->ent[list->cur].id;
    Menu_DrawDashLine(x + 20, 251, 310, 0xFF000000);
    if (Item_IsDeco(id) != 0) {
        ItemInfo_Draw(x, 255, 350, list->infoMode, item = id, 0, list->infoPage, 0x80101010);
    } else {
        ItemInfo_Draw(x, 255, 350, list->infoMode, item = id, 0, -1, 0x80101010);
    }
    Font_SetSpacing(18, 18);
    switch (list->state) {
    case SHOP_STEP_SLIDE_IN:
    case SHOP_STEP_LIST:
        Win_DrawText(288, 352, 332, 80, list->color, 0xC0101010,
                      list->pouchHasRoom == 0 ? gShopPromptTexts[SHOP_PROMPT_PICK_BUY_POUCH_FULL] : gShopPromptTexts[SHOP_PROMPT_PICK_BUY]);
        break;
    case SHOP_STEP_COUNT:
        Win_DrawText(288, 352, 332, 80, list->color, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_BUY_COUNT]);
        Font_SetPos(507, 388);
        sprintf(num, "%02d", list->count);
        Sjis_Widen(num, buf);
        Text_PrintfColored(buf);
        if (Shop_CanBuy(id, list->count + 1) == 1 && list->count < 99) {
            Icon_DrawMode(508, 366, 36, 22, 0xFF00FF00, ICON_CURSOR, 2);
        }
        if (list->count > 1) {
            Icon_DrawMode(509, 406, 36, 22, 0xFF00FF00, ICON_CURSOR_MIRROR, 2);
        }
        break;
    case SHOP_STEP_CONFIRM:
        price = Shop_GetBuyPrice(item);
        price *= list->count;
        Win_DrawText(288, 352, 332, 80, list->color, 0xC0101010,
                      Text_FillDollar(gShopPromptTexts[SHOP_PROMPT_PRICE_ASK], price));
        Font_SetPos(418, 402);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

void Win_Draw(s16 x, s16 y, s16 w, s16 h, u32 c1, u32 c2, s16 kind); /* unit's own declaration: the definition takes s32 positions, sizes and colours; declared with s16 ones, as Shop_DrawSell and Win_DrawText pass them, since with the definition's types their calls compile differently */
u32 Color_PulseAlpha(u32 color);

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
    s16 y = 49;
    s16 i;
    s16 have;
    char num[16];
    char buf[32];

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(270, 18, 160, 0xFFB0B000, gShopMenuTexts[SHOP_MENU_TEXT_SELL], 2, 1);
    Font_SetColor(0x807F7F7F);
    Menu_DrawMoney(480, 16, 0);
    Menu_DrawPageNum(x + 240, 59, list->page, list->pages, 1);
    Win_Draw(x, y, 350, 306, list->color, 0x80202020, 2);
    y += 10;
    Font_SetColor(0x80000000);
    Font_SetSpacing(20, 20);
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < list->n; i++) {
        id = list->ent[i].id;
        price = list->ent[i].price;
        y += 24;
        if (list->cur == i) {
            Gfx_DrawLine(x + 20, y - 1, 310, 20, Color_PulseAlpha(32896));
        }
        if (id == 0) {
            Font_SetPos(x + 20, y);
            Text_PrintfColored("------------------------------");
        } else {
            Font_SetSpacing(20, 20);
            Font_SetPos(x + 46, y);
            Text_PrintfColored("%s", gItemNames[id]);
            Font_SetSpacing(20, 20);
            Font_SetPos(x + 248, y);
            Text_PrintfColored("% 7dz", price);
            Item_DrawIcon(x + 16, y - 1, 24, id);
        }
    }
    id = list->ent[list->cur].id;
    Gfx_DrawLine(x + 20, 251, 310, 2, list->color);
    if (id != 0) {
        if (Item_IsDeco(id) != 0) {
            ItemInfo_Draw(x, 255, 350, list->infoMode, id, 0, list->infoPage, 0x807F7F7F);
        } else {
            ItemInfo_Draw(x, 255, 350, list->infoMode, id, 0, -1, 0x807F7F7F);
        }
    }
    Font_SetSpacing(18, 18);
    switch (list->state) {
    case SHOP_STEP_SLIDE_IN:
    case SHOP_STEP_LIST:
        Win_DrawText(288, 352, 332, 80, list->color, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_PICK_SELL]);
        break;
    case SHOP_STEP_COUNT:
        have = SavePouch_Count(id);
        if (gItemData[id].max == ITEM_MAX_UNLIMITED) {
            have = 1;
        }
        Win_DrawText(288, 352, 332, 80, list->color, 0xC0101010, gShopPromptTexts[SHOP_PROMPT_SELL_COUNT]);
        Font_SetPos(489, 388);
        sprintf(num, "%02d", list->count);
        Sjis_Widen(num, buf);
        Text_PrintfColored(buf);
        if (list->count < have) {
            Icon_DrawMode(490, 366, 36, 22, 0xFF00FF00, ICON_CURSOR, 2);
        }
        if (list->count > 1) {
            Icon_DrawMode(491, 406, 36, 22, 0xFF00FF00, ICON_CURSOR_MIRROR, 2);
        }
        break;
    case SHOP_STEP_CONFIRM:
        price = list->ent[list->cur].price;
        price *= list->count;
        Win_DrawText(288, 352, 332, 80, list->color, 0xC0101010,
                      Text_FillDollar(gShopPromptTexts[SHOP_PROMPT_SELL_PRICE_ASK], price));
        Font_SetPos(400, 402);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

void Text_SetSize(s32);

/* The shop's draw (LobbyMenu_SetDraw): the menu's lines while it is shown,
 * else the buy or the sell window. */
void ItemShop_Draw(MenuTask *task)
{
    ShopMenu *shop = (ShopMenu *)gMenuWork;
    ItemListWin *list = &shop->list;

    Text_SetSize(task->textSize);
    switch (shop->state) {
    case SHOP_MENU_CHOOSE:
        Menu_DrawLabelList(460, shop->lines, shop->n, shop->cur, 0xFFB0B000);
        break;
    case SHOP_MENU_BUY:
        Shop_DrawBuy(list);
        break;
    case SHOP_MENU_SELL:
        Shop_DrawSell(list);
        break;
    }
}


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

/* The lobby menus' buttons of this frame (read once by LobbyPad_Read). */
u16 LobbyPad_Get(s32 unused)
{
    return gLobby->buttons;
}

/* The file's .data: the townspeople's stock lists, then the lobby's label tables. */
extern char gLobbyWords_0[];
extern char gLobbyWords_1[];
extern char gLobbyWords_2[];
extern char gLobbyWords_3[];
extern char gLobbyWords_4[];
extern char gLobbyWords_5[];
extern char gLobbyWords_6[];
extern char gLobbyWords_7[];
extern char gLobbyWords_8[];
extern char gLobbyWords_9[];
extern char gLobbyWords_10[];
extern char gLobbyWords_11[];
extern char gLobbyWords_12[];
extern char gLobbyWords_13[];
extern char gLobbyWords_14[];
extern char gLobbyWords_15[];
extern char gLobbyWords_16[];
extern char gLobbyWords_17[];
extern char gLobbyWords_18[];
extern char gLobbyWords_19[];
extern char D_0083BB50[];
extern char gNoneSetTexts_1[];
extern char D_0083BB60[];
extern char gCountLabels_1[];
extern char gGameMenuLabels_0[];
extern char gGameMenuLabels_1[];
extern char gGameMenuLabels_2[];
extern char gGameMenuLabels_3[];
extern char gGameMenuLabels_4[];
extern char gGameMenuLabels_5[];
extern char D_0083BBD8[];
extern char gGameMenuLabels_7[];
extern char gGameMenuLabels_8[];
extern char gGameMenuLabels_9[];
extern char D_0083BC10[];
extern char D_0083BC18[];
extern char gGameMenuLabels_12[];
extern char gGameMenuLabels_13[];
extern char gGameMenuLabels_14[];
extern char D_0083BC50[];
extern char D_0083BC60[];
extern char gGameMenuLabels_17[];
extern char D_0083BC80[];
extern char gGameMenuLabels_19[];
extern char gGameMenuLabels_20[];
extern char gGameMenuLabels_21[];
extern char D_0083BCC8[];
extern char D_0083BCD8[];
extern char gGameMenuLabels_24[];
extern char gGameMenuLabels_25[];
extern char gGameMenuLabels_26[];
extern char gGameMenuLabels_27[];
extern char gGameMenuLabels_28[];
extern char gGameMenuLabels_29[];
extern char gGameMenuLabels_30[];
extern char gGameMenuLabels_31[];
extern char gPriceInfoTexts_0[];
extern char gWantedLineTexts_0[];
extern char gWantedLineTexts_1[];
extern char gWantedLineTexts_2[];
extern char D_0083BE08[];
extern char D_0083BE18[];
extern char D_0083BE28[];
extern char D_0083BE38[];
extern char D_0083BE48[];
extern char D_0083BE58[];
extern char D_0083BE68[];
extern char gListTitles_0[];
extern char gListTitles_1[];
extern char gListTitles_2[];
extern char gListTitles_3[];
extern char gListTitles_4[];
extern char gListTitles_5[];
extern char gListTitles_6[];
extern char gListTitles_7[];
extern char gListTitles_8[];
extern char gListTitles_9[];
extern char gListTitles_10[];
extern char gListTitles_11[];
extern char gListTitles_12[];
extern char gListTitles_13[];
extern char gListTitles_14[];
extern char gListTitles_15[];
extern char gQuestBoardRows_0[];
extern char gQuestBoardRows_1[];
extern char gQuestBoardRows_2[];
extern char D_0083BFA0[];
extern char gQuestBoardRows_4[];
extern char gQuestBoardRows_5[];
extern char D_0083BFE8[];
extern char gQuestBoardRows_7[];
extern char gQuestBoardRows_8[];
extern char gQuestBoardRows_9[];
extern char gQuestBoardRows_10[];
extern char D_0083C050[];
extern char gTownLabels_0[];
extern char gTownLabels_1[];
extern char gTownLabels_2[];
extern char gTownLabels_3[];
extern char gTownLabels_4[];
extern char gTownLabels_5[];
extern char gTownLabels_6[];
extern char gTownLabels_7[];
extern char gTownLabels_8[];
extern char gTownLabels_9[];
extern char gTownLabels_10[];
extern char gTownLabels_11[];
extern char gTownLabels_12[];
extern char gTownLabels_13[];
extern char gTownLabels_14[];
extern char gTownLabels_16[];
extern char D_0083C1B8[];
extern char gTownLabels_18[];
extern char gTownLabels_19[];
extern char gTownLabels_20[];
extern char D_0083C218[];
extern char D_0083C228[];
extern char gTownLabels_23[];
extern char gAreaNameTexts_0[];
extern char gAreaNameTexts_1[];
extern char gAreaNameTexts_2[];
extern char gAreaNameTexts_3[];
extern char gAreaNameTexts_4[];
extern char gAreaNameTexts_5[];
extern char gAreaNameTexts_6[];
extern char D_0083C2B8[];
extern char gAreaNameTexts_10[];
extern char D_0083C2D0[];
extern char D_0083C2E0[];
extern char gShipMenuTexts_0[];
extern char D_0083C300[];
extern char gTownGuideTopics_0[];
extern char gTownGuideTopics_1[];
extern char D_0083C328[];
extern char gTownGuideHelp_0[];
extern char gTownGuideHelp_1[];
extern char gTownGuideHelp_2[];
extern char gTownGuideHelp_3[];
extern char gTownGuideHelp_4[];
extern char gVillageBoardTitles_0[];
extern char gVillageBoardTitles_1[];
extern char gVillageBoardTitles_2[];
extern char gVillageBoardTitles_3[];
extern char gVillageBoardTexts_0[];
extern char gVillageBoardTexts_1[];
extern char gVillageBoardTexts_2[];
extern char gVillageBoardTexts_3[];
extern char gVillageBoardTexts_4[];
extern char gVillageBoardTexts_5[];
extern char gVillageBoardTexts_6[];
extern char gVillageBoardTexts_7[];
extern char gVillageBoardTexts_8[];
extern char gVillageBoardTexts_9[];
extern char D_0083C598[];
extern char gElderForecastTexts_1[];
extern char D_0083C5B8[];
extern char gElderForecastTexts_5[];
extern char gElderForecastTexts_6[];
extern char gElderForecastTexts_7[];
extern char gHairMenuTexts_0[];
extern char gHairMenuTexts_1[];
extern char gHairMenuTexts_2[];
extern char D_0083C630[];
extern char gHairMenuTexts_4[];
extern char gRecordsMenuTexts_0[];
extern char gRecordsMenuTexts_1[];
extern char gRecordsMenuTexts_2[];
extern char gRecordsMenuTexts_3[];
extern char gRecordsMenuTexts_4[];
extern char gRecordsMenuTexts_5[];
extern char gRecordsMenuTexts_6[];
extern char gRecordsMenuTexts_7[];
extern char gRecordsMenuTexts_8[];
extern char gRecordsMenuTexts_12[];
extern char gRecordsMenuTexts_13[];
extern char gRecordsMenuTexts_14[];
extern char gRecordsMenuTexts_15[];
extern char gRecordsMenuTexts_16[];
extern char gRecordsMenuTexts_17[];
extern char gRecordsMenuTexts_18[];
extern char gRecordsMenuTexts_19[];
extern char gRecordsMenuTexts_20[];
extern char gHouseAccessTexts_0[];
extern char gHouseAccessTexts_1[];
extern char gHouseAccessTexts_2[];
extern char gHouseAccessTexts_3[];
extern char gRecordsListTexts_0[];
extern char gRecordsListTexts_1[];
extern char gRecordsListTexts_2[];
extern char gRecordsListTexts_3[];
extern char D_0083C950[];
extern char D_0083C960[];
extern char D_0083C980[];
extern char gRecordsListTexts_7[];
extern char gHunterNotesTexts_8[];
extern char gRecordsListTexts_9[];
extern char gRecordsListTexts_10[];
extern char gRecordsListTexts_11[];
extern char gRecordsListTexts_12[];
extern char gRecordsListTexts_13[];
extern char gRecordsListTexts_14[];
extern char gRecordsListTexts_15[];
extern char gRecordsListTexts_16[];
extern char gRecordsListTexts_19[];
extern char gRecordsListTexts_20[];
extern char gRecordsListTexts_21[];
extern char gRecordsListTexts_22[];
extern char gRecordsListTexts_23[];
extern char gSongMenuTexts_0[];
extern char gSongMenuTexts_1[];
extern char gSongMenuTexts_2[];
extern char gOfferMenuTexts_0[];
extern char gOfferMenuTexts_1[];
extern char gOfferMenuTexts_2[];
extern char D_0083CB20[];
extern char gOfferMenuTexts_4[];
extern char D_0083CB38[];
extern char gOfferMenuTexts_6[];
extern char gOfferMenuTexts_7[];
extern char gOfferMenuTexts_8[];
extern char gOfferMenuTexts_9[];
extern char gQuestPeriodTexts_0[];
extern char gQuestPeriodTexts_1[];
extern char gOfferHelpTexts_0[];
extern char gOfferHelpTexts_1[];
extern char gOfferHelpTexts_2[];
extern char gOfferHelpTexts_3[];
extern char gOfferHelpTexts_4[];
extern char gDojoMenuTexts_0[];
extern char gDojoMenuTexts_1[];
extern char gDojoMenuTexts_3[];
extern char gDojoMenuTexts_4[];
extern char gDojoMenuTexts_5[];
extern char gDojoMenuTexts_6[];
extern char gDojoMenuTexts_7[];
extern char gDojoMenuTexts_8[];
extern char gDojoMenuTexts_9[];
extern char gDojoMenuTexts_10[];
extern char gDojoMenuTexts_11[];
extern char gDojoMenuTexts_12[];
extern char gDojoMenuTexts_13[];
extern char gDojoMenuTexts_14[];
extern char gDojoMenuTexts_15[];
extern char D_0083CF08[];
extern char gNetLabels_1[];
extern char gNetLabels_2[];
extern char D_0083CF28[];
extern char D_0083CF30[];
extern char D_0083CF38[];
extern char D_0083CF40[];
extern char D_0083CF48[];
extern char D_0083CF58[];
extern char gNetLabels_10[];
extern char gNetLabels_11[];
extern char gNetLabels_12[];
extern char gNetLabels_13[];
extern char D_0083CFD0[];
extern char D_0083CFE0[];
extern char gNetLabels_17[];
extern char gNetLabels_18[];
extern char gNetLabels_19[];
extern char gNetLabels_21[];
extern char D_0083D030[];
extern char D_0083D040[];
extern char gNetLabels_25[];
extern char D_0083D060[];
extern char gNetLabels_27[];
extern char D_0083D088[];
extern char gNetLabels_29[];
extern char gNetLabels_30[];
extern char gNetLabels_31[];
extern char gNetLabels_32[];
extern char gNetLabels_33[];
extern char gNetLabels_35[];
extern char gNetLabels_36[];
extern char gNetLabels_37[];
extern char gNetLabels_38[];
extern char D_0083D130[];
extern char gNetLabels_40[];
extern char gNetLabels_41[];
extern char gNetLabels_42[];
extern char D_0083D170[];
extern char gNetLabels_44[];
extern char gNetLabels_45[];
extern char gNetLabels_46[];
extern char gNetLabels_48[];
extern char gNetLabels_49[];
extern char gNetLabels_50[];
extern char gNetLabels_51[];
extern char gGroupSearchHelpTexts_0[];
extern char gMemberActTexts_0[];
extern char D_0083D318[];
extern char D_0083D328[];
extern char gMailActTexts_0[];
extern char gMailActTexts_2[];
extern char D_0083D350[];
extern char D_0083D360[];
extern char gGroupLimitTexts_4[];
extern char gGroupLimitTexts_5[];
extern char D_0083D380[];
extern char gGroupLimitNewTexts_3[];
extern char gGroupLimitExpTexts_3[];
extern char gArenaCupTexts_0[];
extern char gArenaCupTexts_1[];
extern char gArenaCupTexts_2[];
extern char D_0083D3D8[];
extern char gArenaCupTexts_5[];
extern char gArenaCupTexts_6[];
extern char gArenaCupTexts_7[];
extern char gArenaCupTexts_8[];
extern char gArenaCupTexts_9[];
extern char gArenaCupTexts_10[];
extern char gArenaCupTexts_11[];
extern char gArenaCupTexts_12[];
extern char gArenaCupTexts_13[];
extern char gArenaCupTexts_14[];
extern char gArenaCupTexts_15[];
extern char D_0083D4B8[];
extern char D_0083D4C0[];
extern char gArenaCupTexts_20[];
extern char gArenaCupTexts_21[];
extern char gArenaCupTexts_22[];
extern char gArenaCupTexts_23[];
extern char gArenaCupTexts_24[];
extern char gArenaCupTexts_25[];
extern char gArenaCupTexts_26[];
extern char gArenaCupTexts_27[];
extern char gArenaCupTexts_30[];
extern char gArenaCupTexts_31[];
extern char gArenaCupTexts_33[];
extern char gArenaCupTexts_34[];
extern char gArenaCupTexts_35[];
extern char gArenaCupTexts_36[];
extern char gArenaCupTexts_37[];
extern char gArenaCupTexts_38[];
extern char gArenaCupTexts_39[];
extern char gArenaFeedTexts_0[];
extern char gArenaFeedTexts_1[];
extern char gArenaFeedTexts_4[];
extern char gArenaFeedTexts_5[];
extern char gArenaFeedTexts_6[];
extern char gArenaFeedTexts_7[];
extern char gArenaFeedTexts_8[];
extern char gArenaFeedTexts_9[];
extern char gArenaFeedTexts_10[];
extern char gArenaFeedTexts_11[];
extern char gArenaFeedTexts_12[];
extern char gArenaFeedTexts_13[];
extern char gLobbyMarks_0[];
extern char gLobbyMarks_1[];
extern char gMemberCountTexts_0[];
extern char gMemberCountTexts_1[];
extern char gMemberCountTexts_2[];
extern char gMemberCountTexts_3[];
extern char D_0083D7A0[];
extern char gMemberCountTexts_5[];
extern char gMiscTexts_0[];
extern char gMiscTexts_1[];
extern char gIdMenuTexts_0[];
extern char gIdMenuTexts_1[];
extern char gWeaponArmorTexts_1[];
extern char D_0083D840[];
extern char D_0083D850[];
extern char D_0083D860[];
extern char D_0083D880[];
extern char D_0083D898[];
extern char gEquipGroupTexts_5[];
extern char gEquipGroupTexts_6[];
extern char gEquipGroupTexts_7[];
extern char gEquipGroupTexts_8[];
extern char gEquipGroupTexts_9[];
extern char gEquipLimitTexts_0[];
extern char gEquipLimitTexts_1[];
extern char gEquipLimitTexts_2[];
extern char gEquipLimitTexts_3[];
extern char gEquipLimitTexts_4[];
extern char gEquipLimitTexts_5[];
extern char gEquipLimitTexts_6[];
extern char gEquipLimitTexts_7[];
extern char gEquipNowTexts_0[];
extern char gEquipNowTexts_1[];
extern char gEquipShopTexts_0[];
extern char gEquipShopTexts_1[];
extern char gEquipShopTexts_2[];
extern char D_0083DAD0[];
extern char gEquipShopTexts_4[];
extern char gEquipShopTexts_5[];
extern char gEquipShopTexts_6[];
extern char gEquipShopTexts_7[];
extern char gEquipShopTexts_8[];
extern char gEquipShopTexts_9[];
extern char gSmithyMenuTexts_0[];
extern char gSmithyMenuTexts_1[];
extern char gSmithyMenuTexts_2[];
extern char D_0083DCA0[];
extern char D_0083DCB0[];
extern char gWorkshopTexts_3[];
extern char gNoArmorToMakeTexts_0[];
extern char gWeaponForgeTexts_1[];
extern char gArmorForgeTexts_1[];
extern char gDecoMenuTexts_0[];
extern char gDecoMenuTexts_1[];
extern char gDecoMenuTexts_2[];
extern char gDecoMenuTexts_3[];
extern char gDecoMenuTexts_4[];
extern char gDecoMenuTexts_5[];
extern char gDecoMenuTexts_6[];
extern char gDecoMenuTexts_7[];
extern char gForgeTexts_0[];
extern char D_0083DDC0[];
extern char D_0083DDD8[];
extern char gForgeTexts_5[];
extern char gForgeTexts_6[];
extern char gForgeTexts_7[];
extern char gForgeTexts_8[];
extern char gForgeTexts_10[];
extern char gForgeTexts_11[];
extern char gForgeTexts_12[];
extern char gForgeTexts_13[];
extern char gForgeTexts_14[];
extern char gForgeTexts_15[];
extern char gForgeTexts_16[];
extern char gForgeTexts_17[];
extern char gForgeTexts_18[];
extern char gForgeTexts_19[];
extern char gForgeTexts_20[];
extern char gForgeTexts_21[];
extern char gForgeTexts_22[];
extern char gForgeTexts_23[];
extern char gForgeTexts_24[];
extern char gForgeTexts_25[];
extern char gForgeTexts_26[];
extern char gForgeTexts_27[];
extern char gForgeTexts_28[];
extern char gForgeTexts_29[];
extern char gForgeTexts_30[];
extern char gForgeTexts_31[];
extern char gForgeTexts_32[];
extern char gForgeTexts_33[];
extern char gForgeTexts_34[];
extern char gForgeTexts_35[];
extern char gForgeTexts_36[];
extern char gForgeTexts_37[];
extern char gForgeTexts_38[];
extern char gForgeTexts_39[];
extern char gForgeTexts_40[];
extern char gForgeTexts_41[];
extern char gForgeTexts_42[];
extern char gForgeTexts_43[];
extern char gForgeTexts_44[];
extern char gForgeTexts_45[];
extern char gForgeTexts_46[];
extern char gForgeTexts_47[];
extern char gForgeTexts_48[];
extern char gForgeTexts_49[];
extern char gForgeTexts_50[];
extern char gForgeTexts_51[];
extern char gForgeTexts_52[];
extern char gForgeTexts_53[];
extern char gForgeTexts_54[];
extern char gForgeTexts_55[];
extern char gForgeTexts_56[];
extern char gForgeTexts_57[];
extern char gForgeTexts_58[];
extern char gForgeTexts_59[];
extern char gForgeTexts_60[];
extern char gForgeTexts_61[];
extern char gForgeTexts_62[];
extern char gForgeTexts_64[];
extern char gForgeTexts_65[];
extern char gKitchenTexts_0[];
extern char gKitchenTexts_1[];
extern char gKitchenTexts_2[];
extern char gKitchenTexts_3[];
extern char gKitchenTexts_4[];
extern char gKitchenTexts_5[];
extern char gKitchenTexts_6[];
extern char gKitchenTexts_7[];
extern char gKitchenTexts_8[];
extern char gKitchenTexts_9[];
extern char gKitchenTexts_10[];
extern char gKitchenTexts_11[];
extern char gKitchenTexts_12[];
extern char gKitchenTexts_13[];
extern char gKitchenTexts_14[];
extern char gKitchenTexts_15[];
extern char gKitchenTexts_16[];
extern char gKitchenTexts_17[];
extern char gMealNameTexts_0[];
extern char gMealNameTexts_1[];
extern char gMealNameTexts_2[];
extern char gMealNameTexts_3[];
extern char gMealNameTexts_4[];
extern char gMealNameTexts_5[];
extern char gMealNameTexts_6[];
extern char gMealNameTexts_7[];
extern char gMealNameTexts_8[];
extern char gMealNameTexts_9[];
extern char gMealNameTexts_10[];
extern char gMealNameTexts_11[];
extern char gMealNameTexts_12[];
extern char gMealNameTexts_13[];
extern char gMealNameTexts_14[];
extern char gMealNameTexts_15[];
extern char gMealTasteTexts_0_0[];
extern char gMealTasteTexts_0_1[];
extern char gMealTasteTexts_0_2[];
extern char gMealTasteTexts_0_3[];
extern char gMealTasteTexts_1_0[];
extern char gMealTasteTexts_1_1[];
extern char gMealTasteTexts_1_2[];
extern char gMealTasteTexts_1_3[];
extern char gMealTasteTexts_2_0[];
extern char gMealTasteTexts_2_1[];
extern char gMealTasteTexts_2_2[];
extern char gMealTasteTexts_2_3[];
extern char D_0083F130[];
extern char gMealTasteTexts_4_0[];
extern char gMealTasteTexts_4_1[];
extern char gMealTasteTexts_4_2[];
extern char gMealTasteTexts_4_3[];
extern char gMealTasteTexts_5_0[];
extern char gMealTasteTexts_5_1[];
extern char gMealTasteTexts_5_2[];
extern char gMealTasteTexts_5_3[];
extern char gMealTasteTexts_6_0[];
extern char gMealTasteTexts_6_1[];
extern char gMealTasteTexts_6_2[];
extern char gMealTasteTexts_6_3[];
extern char gBoxActionTexts_0[];
extern char gBoxActionTexts_1[];
extern char gBoxActionTexts_2[];
extern char gBoxActionTexts_3[];
extern char gBoxActionTexts_4[];
extern char gBoxControlTexts_0[];
extern char gBoxControlTexts_1[];
extern char gMenuShowTexts_0[];
extern char D_0083F358[];
extern char gBoxKindTexts_1[];
extern char gEquipMenuTexts_0[];
extern char gEquipMenuTexts_1[];
extern char gEquipMenuTexts_2[];
extern char gEquipMenuTexts_3[];
extern char gEquipMenuTexts_4[];
extern char gEquipSlotTexts_1[];
extern char gEquipSlotTexts_2[];
extern char gEquipSlotTexts_3[];
extern char gEquipSlotTexts_4[];
extern char gEquipSlotTexts_5[];
extern char gBoxMenuTexts_0[];
extern char gBoxMenuTexts_1[];
extern char gBoxMenuTexts_2[];
extern char gBoxMenuTexts_3[];
extern char gBoxMenuTexts_4[];
extern char gBoxMenuTexts_5[];
extern char gBoxMenuTexts_6[];
extern char gBoxMenuTexts_7[];
extern char gBoxMenuTexts_8[];
extern char gBoxMenuTexts_9[];
extern char gBoxMenuTexts_10[];
extern char gBoxMenuTexts_11[];
extern char gBoxMenuTexts_12[];
extern char gBoxMenuTexts_13[];
extern char gBoxMenuTexts_14[];
extern char gBoxMenuTexts_15[];
extern char gBoxMenuTexts_16[];
extern char gBoxMenuTexts_17[];
extern char gBoxMenuTexts_18[];
extern char gBoxMenuTexts_19[];
extern char gBoxMenuTexts_20[];
extern char gBoxMenuTexts_21[];
extern char gBoxHelpTexts_0[];
extern char gBoxHelpTexts_1[];
extern char gBoxHelpTexts_2[];
extern char gBoxHelpTexts_3[];
extern char gBoxHelpTexts_4[];
extern char gBoxHelpTexts_5[];
extern char gBoxHelpTexts_6[];
extern char gBoxHelpTexts_7[];
extern char gBoxHelpTexts_8[];
extern char gBoxHelpTexts_10[];
extern char gBoxHelpTexts_11[];
extern char gBoxHelpTexts_12[];
extern char gBoxHelpTexts_13[];
extern char gBoxHelpTexts_14[];
extern char D_0083F880[];
extern char gBoxHelpTexts_16[];
extern char gBoxHelpTexts_17[];
extern char gBoxHelpTexts_18[];
extern char gBoxHelpTexts_19[];
extern char gBoxHelpTexts_20[];
extern char gBoxHelpTexts_21[];
extern char gBoxHelpTexts_22[];
extern char gBoxHelpTexts_23[];
extern char gBoxHelpTexts_24[];
extern char gBoxHelpTexts_25[];
extern char gBoxHelpTexts_26[];
extern char gBoxHelpTexts_27[];
extern char gBoxHelpTexts_29[];
extern char gBoxHelpTexts_30[];
extern char gBoxHelpTexts_31[];
extern char gBoxHelpTexts_32[];
extern char gBoxHelpTexts_33[];
extern char gBoxHelpTexts_34[];
extern char gBoxHelpTexts_35[];
extern char gBoxHelpTexts_36[];
extern char gBoxHelpTexts_37[];
extern char gBoxHelpTexts_38[];
extern char gBoxHelpTexts_39[];
extern char gBoxHelpTexts_40[];
extern char gBoxHelpTexts_41[];
extern char gBoxHelpTexts_42[];
extern char gBoxHelpTexts_43[];
extern char gBoxHelpTexts_44[];
extern char gBoxHelpTexts_45[];
extern char gBoxHelpTexts_46[];
extern char gBoxHelpTexts_47[];
extern char gBoxHelpTexts_50[];
extern char gBoxHelpTexts_51[];
extern char gBoxHelpTexts_52[];
extern char gBoxHelpTexts_53[];
extern char gBoxHelpTexts_54[];
extern char gBoxHelpTexts_55[];
extern char gBoxHelpTexts_56[];
extern char gBoxHelpTexts_60[];
extern char gBoxHelpTexts_61[];
extern char gBoxHelpTexts_62[];
extern char gBoxHelpTexts_63[];
extern char gBoxHelpTexts_64[];
extern char gBoxHelpTexts_65[];
extern char gBoxHelpTexts_66[];
extern char gBoxHelpTexts_67[];
extern char gBoxHelpTexts_68[];
extern char gBoxHelpTexts_69[];
extern char gBoxHelpTexts_70[];
extern char gBoxHelpTexts_71[];
extern char gAreaChatTexts_0[];
extern char gAreaChatTexts_1[];
extern char gAreaChatTexts_2[];
extern char gAreaChatTexts_3[];
extern char gQuestCounterTexts_2[];
extern char gQuestCounterTexts_3[];
extern char gQuestCounterTexts_4[];
extern char gQuestCounterTexts_5[];
extern char gQuestCounterTexts_6[];
extern char gQuestCounterTexts_7[];
extern char gQuestCounterTexts_8[];
extern char D_00840240[];
extern char gQuestCounterTexts_10[];
extern char gQuestCounterTexts_11[];
extern char gQuestCounterTexts_13[];
extern char gQuestCounterTexts_14[];
extern char gQuestCounterTexts_15[];
extern char gQuestCounterTexts_16[];
extern char gQuestCounterTexts_18[];
extern char D_008402B8[];
extern char D_008402C8[];
extern char gQuestCounterTexts_21[];
extern char D_008402E0[];
extern char D_008402F8[];
extern char gQuestCounterTexts_24[];
extern char D_00840318[];
extern char D_00840328[];
extern char D_00840338[];
extern char gQuestCounterTexts_28[];
extern char gQuestCounterTexts_29[];
extern char gQuestCounterTexts_30[];
extern char gQuestCounterTexts_31[];
extern char gQuestCounterTexts_32[];
extern char gQuestCounterTexts_33[];
extern char gQuestCounterTexts_34[];
extern char gQuestCounterTexts_35[];
extern char gQuestCounterTexts_36[];
extern char gQuestCounterTexts_37[];
extern char gQuestCounterTexts_38[];
extern char gQuestCounterTexts_39[];
extern char gQuestCounterTexts_40[];
extern char gQuestCounterTexts_41[];
extern char gQuestCounterTexts_42[];
extern char gQuestCounterTexts_43[];
extern char gQuestCounterTexts_44[];
extern char gQuestCounterTexts_45[];
extern char gQuestCounterTexts_46[];
extern char gQuestCounterTexts_47[];
extern char gQuestCounterTexts_48[];
extern char gQuestCounterTexts_49[];
extern char gQuestCounterTexts_50[];
extern char gQuestCounterTexts_51[];
extern char gQuestCounterTexts_52[];
extern char gQuestCounterTexts_53[];
extern char D_00840638[];
extern char gInfoIndexTexts_2[];
extern char gInfoIndexTexts_3[];
extern char gInfoIndexTexts_4[];
extern char gQuestSheetTexts_1[];
extern char gQuestSheetTexts_2[];
extern char gQuestSheetTexts_3[];
extern char gQuestSheetTexts_4[];
extern char gQuestSheetTexts_5[];
extern char gQuestSheetTexts_6[];
extern char gQuestSheetTexts_7[];
extern char gQuestSheetTexts_8[];
extern char gQuestSheetTexts_9[];
extern char gQuestSheetTexts_10[];
extern char gQuestSheetTexts_11[];
extern char gQuestSheetTexts_12[];
extern char gQuestSheetTexts_14[];
extern char gQuestSheetTexts_16[];
extern char gQuestSheetTexts_17[];
extern char gQuestSheetTexts_21[];
extern char gQuestSheetTexts_22[];
extern char gQuestSheetTexts_23[];
extern char gQuestSheetTexts_24[];
extern char gQuestSheetTexts_25[];
extern char gQuestSheetTexts_27[];
extern char gQuestSheetTexts_28[];
extern char D_00840828[];
extern char gQuestSheetTexts_31[];
extern char gQuestSheetTexts_34[];
extern char gQuestSheetTexts_35[];
extern char gOfficialEventTexts_0[];
extern char gOfficialEventTexts_1[];
extern char gOfficialEventTexts_2[];
extern char gOfficialEventTexts_3[];
extern char D_008408C8[];
extern char D_008408D0[];
extern char D_008408E0[];
extern char D_008408F0[];
extern char D_00840900[];
extern char D_00840910[];
extern char D_00840920[];
extern char D_00840930[];
extern char D_00840940[];
extern char gGameMenuHelpTexts_0[];
extern char gGameMenuHelpTexts_1[];
extern char gGameMenuHelpTexts_2[];
extern char gGameMenuHelpTexts_3[];
extern char gGameMenuHelpTexts_4[];
extern char gGameMenuHelpTexts_5[];
extern char gGameMenuHelpTexts_6[];
extern char gGameMenuHelpTexts_7[];
extern char gGameMenuHelpTexts_8[];
extern char gGameMenuHelpTexts_9[];
extern char D_00840B10[];
extern char D_00840B30[];
extern char gGameMenuHelpTexts_16[];
extern char gGameMenuHelpTexts_17[];
extern char gGameMenuHelpTexts_18[];
extern char gGameMenuHelpTexts_19[];
extern char D_00840BF0[];
extern char gGameMenuHelpTexts_21[];
extern char gGameMenuHelpTexts_22[];
extern char D_00840CC0[];
extern char gGameMenuHelpTexts_24[];
extern char gGameMenuHelpTexts_25[];
extern char gGameMenuHelpTexts_28[];
extern char gGameMenuHelpTexts_29[];
extern char gGameMenuHelpTexts_30[];
extern char gGameMenuHelpTexts_31[];
extern char gGameMenuHelpTexts_32[];
extern char gGameMenuHelpTexts_33[];
extern char gGameMenuHelpTexts_34[];
extern char gGameMenuHelpTexts_35[];
extern char gGameMenuHelpTexts_36[];
extern char gGameMenuHelpTexts_37[];
extern char gGameMenuHelpTexts_38[];
extern char gGameMenuHelpTexts_39[];
extern char gSearchMenuTexts_1[];
extern char gSearchMenuTexts_2[];
extern char gSearchMenuTexts_3[];
extern char gMailboxMenuTexts_0[];
extern char gMailboxMenuTexts_1[];
extern char gMailboxMenuTexts_2[];
extern char D_008410B0[];
extern char gQuitMenuTexts_1[];
extern char gQuitMenuDimTexts_1[];
extern char gQuitSaveTexts_0[];
extern char gQuitSaveTexts_1[];
extern char gQuitConfirmTexts_0[];
extern char gQuitConfirmTexts_1[];
extern char gQuitConfirmTexts_2[];
extern char gQuitConfirmTexts_3[];
extern char gChatPageTexts_0[];
extern char gChatPageTexts_1[];
extern char gChatPageTexts_2[];
extern char gChatPageTexts_3[];
extern char D_00841350[];
extern char gChatPageTexts_5[];
extern char gChatPageTexts_6[];
extern char gChatPageTexts_7[];
extern char gChatPageTexts_8[];
extern char gChatPageTexts_9[];
extern char D_00841410[];
extern char gChatMenuTexts_3[];
extern char gChatMenuTexts_4[];
extern char gChatMenuTexts_5[];
extern char gChatStateTexts_0[];
extern char gChatStateTexts_1[];
extern char gChatUserActTexts_1[];
extern char gChatUserActTexts_2[];
extern char gChatUserActTexts_4[];
extern char D_008414D8[];
extern char gChatUserActTexts_7[];
extern char gPlaceInfoTexts_0[];
extern char D_00841510[];
extern char gPlaceInfoTexts_6[];
extern char gPlaceInfoTexts_7[];
extern char gPlaceInfoTexts_8[];
extern char gPlaceInfoTexts_9[];
extern char gPlaceInfoTexts_10[];
extern char gPlaceInfoTexts_11[];
extern char gTavernLevelTexts_0[];
extern char gTavernLevelTexts_1[];
extern char gTavernLevelTexts_2[];
extern char gPortLevelTexts_0[];
extern char gPortLevelTexts_1[];
extern char gPortLevelTexts_2[];
extern char gSmithyLevelTexts_0[];
extern char gSmithyLevelTexts_1[];
extern char gSmithyLevelTexts_2[];
extern char gMineLevelTexts_0[];
extern char gMineLevelTexts_1[];
extern char gMineLevelTexts_2[];
extern char gOnlineMarkNames_0[];
extern char gOnlineMarkNames_7[];
extern char gOnlineMarkNames_5[];
extern char gOnlineMarkNames_6[];
extern char gOnlineMarkNames_8[];
extern char gOnlineMarkNames_1[];
extern char gOnlineMarkNames_2[];
extern char gOnlineMarkNames_3[];
extern char gOnlineMarkNames_4[];
extern char gMemberStatusTexts_0[];
extern char gMemberStatusTexts_1[];
extern char gMemberStatusTexts_2[];
extern char gMemberStatusTexts_3[];
extern char gMemberStatusTexts_4[];
extern char gMemberStatusTexts_5[];
extern char gMemberStatusTexts_6[];
extern char gMemberStatusTexts_7[];
extern char gMemberStatusTexts_8[];
extern char D_00841890[];
extern char D_008418A0[];
extern char D_008418A8[];
extern char gShopMenuTexts_3[];
extern char gCombineShopTexts_0[];
extern char gCombineShopTexts_1[];
extern char gCombineShopTexts_2[];
extern char gCombineShopTexts_3[];
extern char gCombineShopTexts_4[];
extern char gCombineShopTexts_5[];
extern char gCombineShopTexts_6[];
extern char D_00841920[];
extern char gCombineShopTexts_8[];
extern char gShopPromptTexts_0[];
extern char gShopPromptTexts_1[];
extern char gShopPromptTexts_2[];
extern char gShopPromptTexts_5[];
extern char gShopPromptTexts_6[];
extern char gShopPromptTexts_7[];
extern char gShopPromptTexts_8[];
extern char gShopPromptTexts_9[];
extern char gShopPromptTexts_10[];
extern char gShopPromptTexts_11[];
extern char gShopPromptTexts_12[];
extern char gShopPromptTexts_13[];
extern char gShopPromptTexts_14[];
extern char gShopPromptTexts_15[];
extern char gSongHelpTexts_0[];
extern char gSongHelpTexts_1[];
extern char gMemberCardPanels_0_0[];
extern char gMemberCardPanels_0_1[];
extern char gMemberCardPanels_0_2[];
extern char gMemberCardPanels_0_3[];
extern char gMemberCardPanels_0_5[];
extern char gMemberCardPanels_0_6[];
extern char gMemberCardPanels_0_7[];
extern char gMemberCardPanels_0_8[];
extern char gStatusTitleTexts_0[];
extern char gStatusTitleTexts_1[];
extern char gFriendRegTexts_0[];
extern char gFriendRegTexts_1[];
extern char gBedMenuTexts_0[];
extern char gBedMenuTexts_1[];
extern char gBedMenuTexts_2[];
extern char gBedMenuTexts_3[];
extern char gBedMenuTexts_4[];
extern char gBedMenuTexts_5[];
extern char gBedMenuTexts_6[];
extern char gMonsterClassTexts_0[];
extern char D_00841FF0[];
extern char D_00841FF8[];
extern char D_00842000[];
extern char D_00842008[];
extern char D_00842010[];
extern char D_00842018[];
extern char gGalleryCourseTexts_0[];
extern char gGalleryCourseTexts_1[];
extern char gGalleryCourseTexts_2[];
extern char gGalleryCourseTexts_3[];
extern char gGalleryCourseTexts_4[];
extern char gGalleryCourseTexts_5[];
extern char gGalleryCourseTexts_6[];
extern char gGalleryCourseTexts_7[];
extern char gGalleryCourseTexts_8[];
extern char gGalleryCourseTexts_9[];
extern char gEmptyListTexts_0[];
extern char gEmptyListTexts_1[];
extern char gEmptyListTexts_2[];
extern char gEmptyListTexts_3[];
extern char gEmptyListTexts_4[];
extern char gEmptyListTexts_5[];
extern char gEmptyListTexts_7[];
extern char gLobbyNoticeTexts_0[];
extern char gLobbyNoticeTexts_1[];
extern char gLobbyNoticeTexts_2[];
extern char gLobbyNoticeTexts_3[];
extern char gLobbyNoticeTexts_4[];
extern char gLobbyNoticeTexts_5[];
extern char gLobbyNoticeTexts_6[];
extern char gLobbyNoticeTexts_7[];
extern char gLobbyNoticeTexts_8[];
extern char gLobbyNoticeTexts_9[];
extern char gLobbyNoticeTexts_10[];
extern char gLobbyNoticeTexts_11[];
extern char gLobbyNoticeTexts_12[];
extern char gLobbyNoticeTexts_13[];
extern char gLobbyNoticeTexts_14[];
extern char gLobbyNoticeTexts_15[];
extern char gLobbyNoticeTexts_16[];
extern char gLobbyNoticeTexts_17[];
extern char gLobbyNoticeTexts_18[];
extern char gLobbyNoticeTexts_19[];
extern char gLobbyNoticeTexts_20[];
extern char gLobbyNoticeTexts_21[];
extern char gLobbyNoticeTexts_22[];
extern char gLobbyNoticeTexts_23[];
extern char gLobbyNoticeTexts_24[];
extern char gLobbyNoticeTexts_25[];
extern char gLobbyNoticeTexts_26[];
extern char gLobbyNoticeTexts_27[];
extern char gLobbyNoticeTexts_28[];
extern char gLobbyNoticeTexts_29[];
extern char gLobbyNoticeTexts_30[];
extern char gLobbyNoticeTexts_31[];
extern char gLobbyNoticeTexts_32[];
extern char gLobbyNoticeTexts_33[];
extern char gLobbyNoticeTexts_34[];
extern char gLobbyNoticeTexts_35[];
extern char gLobbyNoticeTexts_36[];
extern char gLobbyNoticeTexts_37[];
extern char gLobbyNoticeTexts_38[];
extern char gLobbyNoticeTexts_39[];
extern char gLobbyNoticeTexts_40[];
extern char gLobbyEventTexts_0[];
extern char gLobbyEventTexts_1[];
extern char gLobbyEventTexts_2[];
extern char gLobbyEventTexts_3[];
extern char gLobbyEventTexts_4[];
extern char gLobbyEventTexts_5[];
extern char gLobbyEventTexts_6[];
extern char gLobbyEventTexts_7[];
extern char gLobbyEventTexts_8[];
extern char gLobbyEventTexts_9[];
extern char gLobbyEventTexts_11[];
extern char gSpotActionTexts_0[];
extern char D_008428D8[];
extern char gSpotActionTexts_2[];
extern char gSpotActionTexts_3[];
extern char gSpotActionTexts_4[];
extern char gSpotActionTexts_5[];
extern char gSpotActionTexts_6[];
extern char gSpotActionTexts_7[];
extern char gSpotActionTexts_8[];
extern char gSpotActionTexts_9[];
extern char gSpotActionTexts_11[];
extern char gSpotActionTexts_12[];
extern char gSpotActionTexts_13[];
extern char gSpotActionTexts_14[];
extern char gSpotActionTexts_15[];
extern char gSpotActionTexts_17[];
extern char gSpotActionTexts_18[];
extern char gSpotActionTexts_19[];
extern char gSpotActionTexts_20[];
extern char gSpotActionTexts_21[];
extern char gSpotActionTexts_22[];
extern char gSpotActionTexts_23[];
extern char gSpotActionTexts_24[];
extern char gSpotActionTexts_25[];
extern char gSpotActionTexts_26[];
extern char gNameTagActionTexts_0[];
extern char gNameTagActionTexts_1[];
extern char gNameTagActionTexts_2[];
extern char gLobbyTitleTexts_0[];
extern char gLobbyTitleTexts_1[];
extern char gLobbyTitleTexts_2[];
extern char gLobbyTitleTexts_3[];
extern char gLobbyTitleTexts_4[];
extern char gLobbyHelpTexts_0[];
extern char gLobbyHelpTexts_1[];
extern char gLobbyHelpTexts_2[];
extern char gLobbyHelpTexts_3[];
extern char gLobbyHelpTexts_4[];
extern char gLobbyHelpTexts_5[];
extern char gLobbyHelpTexts_6[];
extern char gLobbyHelpTexts_7[];
extern char gLobbyHelpTexts_8[];
extern char gLobbyHelpTexts_9[];
extern char gLobbyHelpTexts_10[];
extern char gLobbyHelpTexts_11[];
extern char gLobbyHelpTexts_13[];
extern char gLobbyHelpTexts_14[];
extern char gLobbyHelpTexts_15[];
extern char gLobbyHelpTexts_16[];
extern char gLobbyHelpTexts_17[];
extern char gLobbyHelpTexts_18[];
extern char gLobbyHelpTexts_19[];
extern char gLobbyHelpTexts_20[];
extern char gLobbyHelpTexts_21[];
extern char gLobbyHelpTexts_22[];
extern char gLobbyHelpTexts_23[];
extern char gLobbyHelpTexts_24[];
extern char gLobbyHelpTexts_25[];
extern char gLobbyHelpTexts_26[];
extern char gLobbyHelpTexts_27[];
extern char gLobbyHelpTexts_28[];
extern char gLobbyHelpTexts_29[];
extern char gLobbyHelpTexts_31[];
extern char gLobbyHelpTexts_32[];
extern char gLobbyHelpTexts_33[];
extern char gLobbyHelpTexts_34[];
extern char gLobbyHelpTexts_35[];
extern char gLobbyHelpTexts_36[];
extern char gLobbyHelpTexts_37[];
extern char gLobbyHelpTexts_38[];
extern char gLobbyHelpTexts_39[];
extern char gLobbyHelpTexts_40[];
extern char gLobbyHelpTexts_41[];
extern char gLobbyHelpTexts_43[];
extern char gLobbyHelpTexts_44[];
extern char gLobbyHelpTexts_46[];
extern char gLobbyHelpTexts_47[];
extern char gLobbyHelpTexts_51[];
extern char gLobbyHelpTexts_53[];
extern char D_00843998[];
extern char gLobbyHelpTexts_55[];
extern char gLobbyHelpTexts_60[];
extern char gLobbyHelpTexts_61[];
extern char gLobbyHelpTexts_62[];
extern char gLobbyHelpTexts_63[];
extern char gLobbyHelpTexts_64[];
extern char gLobbyHelpTexts_65[];
extern char gLobbyHelpTexts_66[];
extern char gLobbyHelpTexts_67[];
extern char gLobbyLabelTexts_0[];
extern char gLobbyLabelTexts_1[];
extern char gLobbyLabelTexts_2[];
extern char gLobbyLabelTexts_4[];
extern char gLobbyLabelTexts_8[];
extern char gLobbyLabelTexts_9[];
extern char gLobbyLabelTexts_10[];
extern char gLobbyLabelTexts_11[];
extern char gLobbyLabelTexts_12[];
extern char D_00843A88[];
extern char gLobbyLabelTexts_14[];
extern char gLobbyLabelTexts_15[];
extern char gLobbyLabelTexts_16[];
extern char gLobbyLabelTexts_17[];
extern char gLobbyLabelTexts_18[];
extern char gLobbyLabelTexts_20[];
extern char gLobbyLabelTexts_21[];
extern char gLobbyLabelTexts_26[];
extern char gLobbyLabelTexts_27[];
extern char gLobbyLabelTexts_28[];
extern char gLobbyLabelTexts_29[];
extern char gLobbyLabelTexts_30[];
extern char gLobbyLabelTexts_31[];
extern char gLobbyLabelTexts_32[];
extern char gLobbyLabelTexts_33[];
extern char gLobbyLabelTexts_34[];
extern char gLobbyLabelTexts_35[];
extern char gLobbyLabelTexts_37[];
extern char gLobbyLabelTexts_38[];
extern char gLobbyLabelTexts_39[];
extern char gLobbyLabelTexts_40[];
extern char gLobbyLabelTexts_41[];
extern char gLobbyLabelTexts_42[];
extern char gLobbyLabelTexts_43[];
extern char gLobbyLabelTexts_45[];
extern char gLobbyLabelTexts_46[];
extern char gLobbyLabelTexts_47[];
extern char gLobbyLabelTexts_48[];
extern char gLobbyLabelTexts_49[];
extern char gLobbyLabelTexts_51[];
extern char gLobbyLabelTexts_52[];
extern char gLobbyLabelTexts_53[];
extern char gLobbyLabelTexts_54[];
extern char gLobbyLabelTexts_55[];
extern char gLobbyLabelTexts_56[];
extern char gLobbyLabelTexts_57[];
extern char gLobbyNoSaveTexts_0[];
extern char gLobbyNoSaveTexts_1[];
extern char gLobbyButtonTexts_0[];
extern char gLobbyButtonTexts_1[];
extern char gLobbyButtonTexts_2[];
extern char gLobbyButtonTexts_4[];
extern char gLobbyButtonTexts_6[];
extern char gLobbyButtonTexts_7[];
extern char gLobbyButtonTexts_8[];
extern char gLobbyButtonTexts_9[];
extern char gLobbyButtonTexts_12[];
extern char gLobbyButtonTexts_13[];
extern char gLobbyButtonTexts_14[];
extern char gLobbyButtonTexts_15[];
extern char gLobbyButtonTexts_17[];
extern char gLobbyButtonTexts_18[];
extern char gLobbyButtonTexts_20[];
extern char gLobbyButtonTexts_22[];
extern char gLobbyButtonTexts_23[];
extern char gLobbyButtonTexts_24[];
extern char gLobbyButtonTexts_26[];
extern char gLobbyButtonTexts_27[];
extern char gLobbyButtonTexts_28[];
extern char gLobbyButtonTexts_29[];
extern char gLobbyButtonTexts_30[];
extern char gLobbyButtonTexts_32[];
extern char gLobbyButtonTexts_33[];
extern char gLobbyButtonTexts_35[];
extern char gLobbyButtonTexts_37[];
extern char gLobbyButtonTexts_38[];
extern char gLobbyMsgTexts_0[];
extern char gLobbyMsgTexts_1[];
extern char gLobbyMsgTexts_2[];
extern char gLobbyMsgTexts_3[];
extern char gLobbyMsgTexts_4[];
extern char gLobbyMsgTexts_5[];
extern char gLobbyMsgTexts_6[];
extern char gLobbyMsgTexts_7[];
extern char gLobbyMsgTexts_8[];
extern char gLobbyMsgTexts_9[];
extern char D_008441E0[];
extern char gLobbyMsgTexts_12[];
extern char D_00844260[];
extern char gLobbyMsgTexts_15[];
extern char gLobbyMsgTexts_16[];
extern char D_00844320[];
extern char gLobbyMsgTexts_18[];
extern char gLobbyMsgTexts_19[];
extern char gLobbyMsgTexts_20[];
extern char gLobbyMsgTexts_21[];
extern char gLobbyMsgTexts_22[];
extern char gLobbyMsgTexts_23[];
extern char gLobbyMsgTexts_24[];
extern char gLobbyMsgTexts_25[];
extern char gLobbyMsgTexts_27[];
extern char gLobbyMsgTexts_28[];
extern char gLobbyMsgTexts_29[];
extern char gLobbyMsgTexts_30[];
extern char gLobbyMsgTexts_31[];
extern char gLobbyMsgTexts_32[];
extern char gLobbyMsgTexts_33[];
extern char gLobbyMsgTexts_34[];
extern char gLobbyMsgTexts_35[];
extern char gLobbyMsgTexts_36[];
extern char gLobbyMsgTexts_37[];
extern char gLobbyMsgTexts_38[];
extern char gLobbyMsgTexts_39[];
extern char gLobbyMsgTexts_40[];
extern char gLobbyMsgTexts_41[];
extern char gLobbyMsgTexts_42[];
extern char gLobbyMsgTexts_43[];
extern char gLobbyMsgTexts_44[];
extern char gLobbyMsgTexts_45[];
extern char gLobbyMsgTexts_46[];
extern char gLobbyMsgTexts_47[];
extern char gLobbyMsgTexts_48[];
extern char gLobbyMsgTexts_49[];
extern char gLobbyMsgTexts_50[];
extern char gLobbyMsgTexts_51[];
extern char gLobbyMsgTexts_52[];
extern char gLobbyMsgTexts_53[];
extern char gLobbyMsgTexts_54[];
extern char gLobbyMsgTexts_55[];
extern char gLobbyMsgTexts_56[];
extern char gLobbyMsgTexts_57[];
extern char gLobbyMsgTexts_58[];
extern char gLobbyMsgTexts_59[];
extern char gLobbyMsgTexts_60[];
extern char gLobbyMsgTexts_61[];
extern char gLobbyMsgTexts_62[];
extern char gLobbyMsgTexts_63[];
extern char gLobbyMsgTexts_64[];
extern char gLobbyMsgTexts_65[];
extern char gLobbyMsgTexts_66[];
extern char gLobbyMsgTexts_67[];
extern char gLobbyMsgTexts_68[];
extern char gLobbyMsgTexts_69[];
extern char gLobbyMsgTexts_70[];
extern char D_00844EC0[];
extern char gLobbyMsgTexts_72[];
extern char gLobbyMsgTexts_73[];
extern char gLobbyMsgTexts_74[];
extern char gLobbyMsgTexts_75[];
extern char gLobbyMsgTexts_76[];
extern char gLobbyMsgTexts_77[];
extern char gLobbyMsgTexts_78[];
extern char gLobbyMsgTexts_79[];
extern char gLobbyMsgTexts_80[];
extern char gLobbyMsgTexts_81[];
extern char gLobbyMsgTexts_82[];
extern char gLobbyMsgTexts_83[];
extern char gLobbyMsgTexts_84[];
extern char gLobbyMsgTexts_85[];
extern char gLobbyMsgTexts_86[];
extern char gLobbyMsgTexts_87[];
extern char gLobbyMsgTexts_88[];
extern char gLobbyMsgTexts_89[];
extern char gLobbyMsgTexts_90[];
extern char gLobbyMsgTexts_91[];
extern char gLobbyMsgTexts_92[];
extern char gLobbyMsgTexts_93[];
extern char gLobbyMsgTexts_94[];
extern char gLobbyMsgTexts_95[];
extern char gLobbyMsgTexts_96[];
extern char gLobbyMsgTexts_97[];
extern char gLobbyMsgTexts_98[];
extern char gLobbyMsgTexts_99[];
extern char gLobbyMsgTexts_100[];
extern char gLobbyMsgTexts_101[];
extern char gLobbyMsgTexts_102[];
extern char gLobbyMsgTexts_103[];
extern char gLobbyMsgTexts_104[];
extern char gLobbyMsgTexts_105[];
extern char gLobbyMsgTexts_106[];
extern char gLobbyMsgTexts_107[];
extern char gLobbyMsgTexts_108[];
extern char gLobbyMsgTexts_109[];
extern char gLobbyMsgTexts_110[];
extern char gLobbyMsgTexts_111[];
extern char gLobbyMsgTexts_112[];
extern char gLobbyMsgTexts_113[];
extern char gLobbyMsgTexts_114[];
extern char gLobbyMsgTexts_115[];
extern char gLobbyMsgTexts_116[];
extern char gLobbyMsgTexts_117[];
extern char gLobbyMsgTexts_118[];
extern char gLobbyMsgTexts_119[];
extern char gLobbyMsgTexts_120[];
extern char gLobbyMsgTexts_121[];
extern char gLobbyMsgTexts_122[];
extern char gLobbyMsgTexts_123[];
extern char gLobbyMsgTexts_124[];
extern char gLobbyMsgTexts_126[];
extern char gLobbyMsgTexts_127[];
extern char gLobbyMsgTexts_128[];
extern char gLobbyMsgTexts_129[];
extern char gLobbyMsgTexts_130[];
extern char gLobbyMsgTexts_131[];
extern char gLobbyMsgTexts_132[];
extern char gLobbyMsgTexts_133[];
extern char gLobbyMsgTexts_134[];
extern char gLobbyMsgTexts_135[];
extern char gLobbyMsgTexts_136[];
extern char gLobbyMsgTexts_137[];
extern char gLobbyMsgTexts_139[];
extern char gLobbyMsgTexts_140[];
extern char gLobbyMsgTexts_141[];
extern char gLobbyMsgTexts_142[];
extern char gLobbyMsgTexts_143[];
extern char gLobbyMsgTexts_144[];
extern char gLobbyMsgTexts_145[];
extern char gLobbyMsgTexts_146[];
extern char gLobbyMsgTexts_147[];
extern char gLobbyMsgTexts_148[];
extern char gLobbyMsgTexts_149[];
extern char gLobbyMsgTexts_150[];
u16 gShopCondStockNpc07_8[48] = {
#include "lobby/0080B1A0.inc"
};
u16 gShopCondStockNpc07_7[48] = {
#include "lobby/0080B200.inc"
};
u16 gShopCondStockNpc07_6[48] = {
#include "lobby/0080B260.inc"
};
u16 gShopCondStockNpc07_5[48] = {
#include "lobby/0080B2C0.inc"
};
u16 gShopCondStockNpc07_4[56] = {
#include "lobby/0080B320.inc"
};
u16 gShopCondStockNpc07_3[56] = {
#include "lobby/0080B390.inc"
};
u16 gShopCondStockNpc07_2[56] = {
#include "lobby/0080B400.inc"
};
u16 gShopCondStockNpc07_1[64] = {
#include "lobby/0080B470.inc"
};
u16 gShopCondStockNpc07_0[64] = {
#include "lobby/0080B4F0.inc"
};
ShopStockCond gShopCondStockNpc07[10] = {
#include "lobby/0080B570.inc"
};
u16 gShopCondStockNpc08_5[16] = {
#include "lobby/0080B5C0.inc"
};
u16 gShopCondStockNpc08_4[24] = {
#include "lobby/0080B5E0.inc"
};
u16 gShopCondStockNpc08_3[24] = {
#include "lobby/0080B610.inc"
};
u16 gShopCondStockNpc08_2[24] = {
#include "lobby/0080B640.inc"
};
u16 gShopCondStockNpc08_1[24] = {
#include "lobby/0080B670.inc"
};
u16 gShopCondStockNpc08_0[24] = {
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
u16 gShopCondStockNpc28_2[4] = {
#include "lobby/0080BA08.inc"
};
u16 gShopCondStockNpc28_1[8] = {
#include "lobby/0080BA10.inc"
};
u16 gShopCondStockNpc28_0[8] = {
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
u16 gShopCondStockNpc39_3[48] = {
#include "lobby/0080BBB0.inc"
};
u16 gShopCondStockNpc39_2[56] = {
#include "lobby/0080BC10.inc"
};
u16 gShopCondStockNpc39_1[56] = {
#include "lobby/0080BC80.inc"
};
u16 gShopCondStockNpc39_0[64] = {
#include "lobby/0080BCF0.inc"
};
ShopStockCond gShopCondStockNpc39[4] = {
#include "lobby/0080BD70.inc"
};
u16 gShopCondStockNpc42_0[28] = {
#include "lobby/0080BD90.inc"
};
ShopStockCond gShopCondStockNpc42[1] = {
    {0, gShopCondStockNpc42_0},
};
u16 gShopStockNpc50[72] = {
#include "lobby/0080BDD0.inc"
};
u16 gShopStockNpc50Ev122[72] = {
#include "lobby/0080BE60.inc"
};
u16 gShopCondStockNpc5D_0[40] = {
#include "lobby/0080BEF0.inc"
};
ShopStockCond gShopCondStockNpc5D[2] = {
    {0, gShopCondStockNpc5D_0},
    {0, NULL},
};
u16 gShopCondStockNpc6A_1[56] = {
#include "lobby/0080BF50.inc"
};
u16 gShopCondStockNpc6A_0[56] = {
#include "lobby/0080BFC0.inc"
};
ShopStockCond gShopCondStockNpc6A[2] = {
#include "lobby/0080C030.inc"
};
/* the lobby's common words and marks, read by many screens */
char *gLobbyWords[20] = {
    gLobbyWords_0, gLobbyWords_1, gLobbyWords_2, gLobbyWords_3,
    gLobbyWords_4, gLobbyWords_5, gLobbyWords_6, gLobbyWords_7,
    gLobbyWords_8, gLobbyWords_9, gLobbyWords_10, gLobbyWords_11,
    gLobbyWords_12, gLobbyWords_13, gLobbyWords_14, gLobbyWords_15,
    gLobbyWords_16, gLobbyWords_17, gLobbyWords_18, gLobbyWords_19,
};
/* "none" and "set", indexed by whether a password is set */
char *gNoneSetTexts[2] = {
    D_0083BB50, gNoneSetTexts_1,
};
/* the column heads of a count */
char *gCountLabels[2] = {
    D_0083BB60, gCountLabels_1,
};
/* the lobby game menu's entries and guide lines, then the price board's unknown name and title */
char *gGameMenuLabels[32] = {
    gGameMenuLabels_0, gGameMenuLabels_1, gGameMenuLabels_2, gGameMenuLabels_3,
    gGameMenuLabels_4, gGameMenuLabels_5, D_0083BBD8, gGameMenuLabels_7,
    gGameMenuLabels_8, gGameMenuLabels_9, D_0083BC10, D_0083BC18,
    gGameMenuLabels_12, gGameMenuLabels_13, gGameMenuLabels_14, D_0083BC50,
    D_0083BC60, gGameMenuLabels_17, D_0083BC80, gGameMenuLabels_19,
    gGameMenuLabels_20, gGameMenuLabels_21, D_0083BCC8, D_0083BCD8,
    gGameMenuLabels_24, gGameMenuLabels_25, gGameMenuLabels_26, gGameMenuLabels_27,
    gGameMenuLabels_28, gGameMenuLabels_29, gGameMenuLabels_30, gGameMenuLabels_31,
};
/* the monster price board's lines */
char *gPriceInfoTexts[12] = {
    gPriceInfoTexts_0, gWantedLineTexts_0, gWantedLineTexts_1, gWantedLineTexts_2,
    D_0083BE08, D_0083BE18, D_0083BE28, D_0083BB50,
    D_0083BE38, D_0083BE48, D_0083BE58, D_0083BE68,
};
/* the titles and guides of the town's selection windows */
char *gListTitles[16] = {
    gListTitles_0, gListTitles_1, gListTitles_2, gListTitles_3,
    gListTitles_4, gListTitles_5, gListTitles_6, gListTitles_7,
    gListTitles_8, gListTitles_9, gListTitles_10, gListTitles_11,
    gListTitles_12, gListTitles_13, gListTitles_14, gListTitles_15,
};
/* the quest board's rows: the desks, the quest kinds, the taker */
char *gQuestBoardRows[12] = {
    gQuestBoardRows_0, gQuestBoardRows_1, gQuestBoardRows_2, D_0083BFA0,
    gQuestBoardRows_4, gQuestBoardRows_5, D_0083BFE8, gQuestBoardRows_7,
    gQuestBoardRows_8, gQuestBoardRows_9, gQuestBoardRows_10, D_0083C050,
};
/* labels and help lines of the town's small windows */
char *gTownLabels[24] = {
    gTownLabels_0, gTownLabels_1, gTownLabels_2, gTownLabels_3,
    gTownLabels_4, gTownLabels_5, gTownLabels_6, gTownLabels_7,
    gTownLabels_8, gTownLabels_9, gTownLabels_10, gTownLabels_11,
    gTownLabels_12, gTownLabels_13, gTownLabels_14, D_0083BBD8,
    gTownLabels_16, D_0083C1B8, gTownLabels_18, gTownLabels_19,
    gTownLabels_20, D_0083C218, D_0083C228, gTownLabels_23,
};
char *gAreaNameTexts[16] = {
    gAreaNameTexts_0, gAreaNameTexts_1, gAreaNameTexts_2, gAreaNameTexts_3,
    gAreaNameTexts_4, gAreaNameTexts_5, gAreaNameTexts_6, D_0083C2B8,
    D_0083C2B8, D_0083C2B8, gAreaNameTexts_10, D_0083C2D0,
    D_0083C2D0, D_0083C2D0, D_0083C2D0, D_0083C2E0,
};
/* the ship menu's entries, drawn as a list */
char *gShipMenuTexts[2] = {
    gShipMenuTexts_0, D_0083C300,
};
/* the town guide's groups, by group */
char *gTownGuideTopics[3] = {
    gTownGuideTopics_0, gTownGuideTopics_1, D_0083C328,
};
/* the town guide's help lines by group, then its scroll arrows */
char *gTownGuideHelp[8] = {
    gTownGuideHelp_0, gTownGuideHelp_1, gTownGuideHelp_2, gTownGuideHelp_3,
    gTownGuideHelp_4, NULL, NULL, NULL,
};
/* the village board's title lines and prompts */
char *gVillageBoardTitles[4] = {
    gVillageBoardTitles_0, gVillageBoardTitles_1, gVillageBoardTitles_2, gVillageBoardTitles_3,
};
/* the village board's article kinds by kind from 0, then each kind's description from 5 */
char *gVillageBoardTexts[12] = {
    gVillageBoardTexts_0, gVillageBoardTexts_1, gVillageBoardTexts_2, gVillageBoardTexts_3,
    gVillageBoardTexts_4, gVillageBoardTexts_5, gVillageBoardTexts_6, gVillageBoardTexts_7,
    gVillageBoardTexts_8, gVillageBoardTexts_9, NULL, NULL,
};
/* the elder dragon forecast's menu, from 0, then its panel's labels */
char *gElderForecastTexts[8] = {
    D_0083C598, gElderForecastTexts_1, D_0083BC60, D_0083C598,
    D_0083C5B8, gElderForecastTexts_5, gElderForecastTexts_6, gElderForecastTexts_7,
};
/* the hair style menu's labels */
char *gHairMenuTexts[8] = {
    gHairMenuTexts_0, gHairMenuTexts_1, gHairMenuTexts_2, D_0083C630,
    gHairMenuTexts_4, NULL, NULL, NULL,
};
/* the records menu's pages, by RecordKind from 0, then its prompts */
char *gRecordsMenuTexts[24] = {
    gRecordsMenuTexts_0, gRecordsMenuTexts_1, gRecordsMenuTexts_2, gRecordsMenuTexts_3,
    gRecordsMenuTexts_4, gRecordsMenuTexts_5, gRecordsMenuTexts_6, gRecordsMenuTexts_7,
    gRecordsMenuTexts_8, NULL, NULL, NULL,
    gRecordsMenuTexts_12, gRecordsMenuTexts_13, gRecordsMenuTexts_14, gRecordsMenuTexts_15,
    gRecordsMenuTexts_16, gRecordsMenuTexts_17, gRecordsMenuTexts_18, gRecordsMenuTexts_19,
    gRecordsMenuTexts_20, NULL, NULL, NULL,
};
/* the house access page's rows, by HouseAccessRow */
char *gHouseAccessTexts[4] = {
    gHouseAccessTexts_0, gHouseAccessTexts_1, gHouseAccessTexts_2, gHouseAccessTexts_3,
};
/* the records' title genres, item kinds and hunt lines */
char *gRecordsListTexts[24] = {
    gRecordsListTexts_0, gRecordsListTexts_1, gRecordsListTexts_2, gRecordsListTexts_3,
    D_0083C950, D_0083C960, D_0083C980, gRecordsListTexts_7,
    gHunterNotesTexts_8, gRecordsListTexts_9, gRecordsListTexts_10, gRecordsListTexts_11,
    gRecordsListTexts_12, gRecordsListTexts_13, gRecordsListTexts_14, gRecordsListTexts_15,
    gRecordsListTexts_16, D_0083C960, D_0083C980, gRecordsListTexts_19,
    gRecordsListTexts_20, gRecordsListTexts_21, gRecordsListTexts_22, gRecordsListTexts_23,
};
/* the song menu's labels */
char *gSongMenuTexts[4] = {
    gSongMenuTexts_0, gSongMenuTexts_1, gSongMenuTexts_2, NULL,
};
/* the village offer menu's labels */
char *gOfferMenuTexts[10] = {
    gOfferMenuTexts_0, gOfferMenuTexts_1, gOfferMenuTexts_2, D_0083CB20,
    gOfferMenuTexts_4, D_0083CB38, gOfferMenuTexts_6, gOfferMenuTexts_7,
    gOfferMenuTexts_8, gOfferMenuTexts_9,
};
/* how long an offer stays, by period */
char *gQuestPeriodTexts[2] = {
    gQuestPeriodTexts_0, gQuestPeriodTexts_1,
};
/* the offered quest's help lines */
char *gOfferHelpTexts[8] = {
    gOfferHelpTexts_0, gOfferHelpTexts_1, gOfferHelpTexts_2, gOfferHelpTexts_3,
    gOfferHelpTexts_4, NULL, NULL, NULL,
};
/* the hunter dojo's menus and titles */
char *gDojoMenuTexts[16] = {
    gDojoMenuTexts_0, gDojoMenuTexts_1, D_0083C300, gDojoMenuTexts_3,
    gDojoMenuTexts_4, gDojoMenuTexts_5, gDojoMenuTexts_6, gDojoMenuTexts_7,
    gDojoMenuTexts_8, gDojoMenuTexts_9, gDojoMenuTexts_10, gDojoMenuTexts_11,
    gDojoMenuTexts_12, gDojoMenuTexts_13, gDojoMenuTexts_14, gDojoMenuTexts_15,
};
/* the network screens' labels: the group and area boards, the searches, the
 * friend and block lists, the hunter info and the short mail */
char *gNetLabels[52] = {
    D_0083CF08, gNetLabels_1, gNetLabels_2, D_0083BB60,
    D_0083CF28, D_0083CF30, D_0083CF38, D_0083CF40,
    D_0083CF48, D_0083CF58, gNetLabels_10, gNetLabels_11,
    gNetLabels_12, gNetLabels_13, D_0083CFD0, D_0083CFE0,
    D_0083CB38, gNetLabels_17, gNetLabels_18, gNetLabels_19,
    D_0083CF08, gNetLabels_21, D_0083D030, D_0083D040,
    D_0083CFD0, gNetLabels_25, D_0083D060, gNetLabels_27,
    D_0083D088, gNetLabels_29, gNetLabels_30, gNetLabels_31,
    gNetLabels_32, gNetLabels_33, D_0083BC80, gNetLabels_35,
    gNetLabels_36, gNetLabels_37, gNetLabels_38, D_0083D130,
    gNetLabels_40, gNetLabels_41, gNetLabels_42, D_0083D170,
    gNetLabels_44, gNetLabels_45, gNetLabels_46, D_0083BC18,
    gNetLabels_48, gNetLabels_49, gNetLabels_50, gNetLabels_51,
};
/* the group search form's help line */
char *gGroupSearchHelpTexts[2] = {
    gGroupSearchHelpTexts_0, NULL,
};
/* the hunter info's actions, by action */
char *gMemberActTexts[3] = {
    gMemberActTexts_0, D_0083D318, D_0083D328,
};
/* the mailbox's actions, by action */
char *gMailActTexts[3] = {
    gMailActTexts_0, D_0083D318, gMailActTexts_2,
};
/* a group's limit, by GroupLimit, then the same in short from 3 */
char *gGroupLimitTexts[8] = {
    D_0083BB50, D_0083D350, D_0083D360, D_0083BB50,
    gGroupLimitTexts_4, gGroupLimitTexts_5, NULL, NULL,
};
/* the search form's limit choices for a new hunter, by rank */
char *gGroupLimitNewTexts[4] = {
    D_0083D380, D_0083BB50, D_0083D350, gGroupLimitNewTexts_3,
};
/* the search form's limit choices for an expert, by rank */
char *gGroupLimitExpTexts[4] = {
    D_0083D380, D_0083BB50, D_0083D360, gGroupLimitExpTexts_3,
};
/* the arena cup's entry form, records, monster list and monster pages */
char *gArenaCupTexts[40] = {
    gArenaCupTexts_0, gArenaCupTexts_1, gArenaCupTexts_2, D_0083D3D8,
    D_0083D170, gArenaCupTexts_5, gArenaCupTexts_6, gArenaCupTexts_7,
    gArenaCupTexts_8, gArenaCupTexts_9, gArenaCupTexts_10, gArenaCupTexts_11,
    gArenaCupTexts_12, gArenaCupTexts_13, gArenaCupTexts_14, gArenaCupTexts_15,
    D_0083D4B8, D_0083CF30, D_0083D4C0, D_0083D380,
    gArenaCupTexts_20, gArenaCupTexts_21, gArenaCupTexts_22, gArenaCupTexts_23,
    gArenaCupTexts_24, gArenaCupTexts_25, gArenaCupTexts_26, gArenaCupTexts_27,
    D_0083CF30, D_0083D4C0, gArenaCupTexts_30, gArenaCupTexts_31,
    D_0083D4B8, gArenaCupTexts_33, gArenaCupTexts_34, gArenaCupTexts_35,
    gArenaCupTexts_36, gArenaCupTexts_37, gArenaCupTexts_38, gArenaCupTexts_39,
};
/* the arena feed list's lines, then the digits */
char *gArenaFeedTexts[14] = {
    gArenaFeedTexts_0, gArenaFeedTexts_1, NULL, NULL,
    gArenaFeedTexts_4, gArenaFeedTexts_5, gArenaFeedTexts_6, gArenaFeedTexts_7,
    gArenaFeedTexts_8, gArenaFeedTexts_9, gArenaFeedTexts_10, gArenaFeedTexts_11,
    gArenaFeedTexts_12, gArenaFeedTexts_13,
};
/* two marks: the quest sheet's star and the page counter's slash */
char *gLobbyMarks[2] = {
    gLobbyMarks_0, gLobbyMarks_1,
};
/* the member counts by count - 1 (one to four), then two labels */
char *gMemberCountTexts[6] = {
    gMemberCountTexts_0, gMemberCountTexts_1, gMemberCountTexts_2, gMemberCountTexts_3,
    D_0083D7A0, gMemberCountTexts_5,
};
/* two unrelated lines that share one 8-byte object */
char *gMiscTexts[2] = {
    gMiscTexts_0, gMiscTexts_1,
};
/* the ID menu's labels */
char *gIdMenuTexts[2] = {
    gIdMenuTexts_0, gIdMenuTexts_1,
};
/* the equipment shop's two lists: weapons, armour */
char *gWeaponArmorTexts[2] = {
    D_0083CF58, gWeaponArmorTexts_1,
};
/* the equipment groups: the weapon classes, then the armour parts */
char *gEquipGroupTexts[12] = {
    D_0083D840, D_0083D850, D_0083D860, D_0083D880,
    D_0083D898, gEquipGroupTexts_5, gEquipGroupTexts_6, gEquipGroupTexts_7,
    gEquipGroupTexts_8, gEquipGroupTexts_9, NULL, NULL,
};
/* who can wear a piece, by the piece's wearer limit */
char *gEquipLimitTexts[8] = {
    gEquipLimitTexts_0, gEquipLimitTexts_1, gEquipLimitTexts_2, gEquipLimitTexts_3,
    gEquipLimitTexts_4, gEquipLimitTexts_5, gEquipLimitTexts_6, gEquipLimitTexts_7,
};
/* the equip-now and send-home choice, by the yes/no cursor */
char *gEquipNowTexts[4] = {
    gEquipNowTexts_0, gEquipNowTexts_1, NULL, NULL,
};
/* the equipment shop's notes and questions */
char *gEquipShopTexts[12] = {
    gEquipShopTexts_0, gEquipShopTexts_1, gEquipShopTexts_2, D_0083DAD0,
    gEquipShopTexts_4, gEquipShopTexts_5, gEquipShopTexts_6, gEquipShopTexts_7,
    gEquipShopTexts_8, gEquipShopTexts_9, NULL, NULL,
};
/* the workshop's first menu */
char *gSmithyMenuTexts[4] = {
    gSmithyMenuTexts_0, gSmithyMenuTexts_1, gSmithyMenuTexts_2, D_0083C300,
};
/* the workshop's make menu, then the weapon forge's empty-list line */
char *gWorkshopTexts[4] = {
    D_0083DCA0, D_0083DCB0, D_0083C300, gWorkshopTexts_3,
};
/* the armour forge's empty-list line */
char *gNoArmorToMakeTexts[4] = {
    gNoArmorToMakeTexts_0, NULL, NULL, NULL,
};
/* the weapon forge's titles, by its choice: make, upgrade */
char *gWeaponForgeTexts[2] = {
    D_0083DCA0, gWeaponForgeTexts_1,
};
/* the armour forge's titles */
char *gArmorForgeTexts[2] = {
    D_0083DCB0, gArmorForgeTexts_1,
};
/* the decorations counter's titles, then the make menu's parts */
char *gDecoMenuTexts[8] = {
    gDecoMenuTexts_0, gDecoMenuTexts_1, gDecoMenuTexts_2, gDecoMenuTexts_3,
    gDecoMenuTexts_4, gDecoMenuTexts_5, gDecoMenuTexts_6, gDecoMenuTexts_7,
};
/* the weapon classes, by the forge's row */
char *gWeaponClassTexts[8] = {
    D_0083D840, D_0083D850, D_0083D860, D_0083D880,
    D_0083D898, NULL, NULL, NULL,
};
/* the forges' and the decorations counter's questions, headers and notes */
char *gForgeTexts[68] = {
    gForgeTexts_0, D_0083DDC0, D_0083C630, D_0083DDD8,
    D_0083DDD8, gForgeTexts_5, gForgeTexts_6, gForgeTexts_7,
    gForgeTexts_8, D_0083DAD0, gForgeTexts_10, gForgeTexts_11,
    gForgeTexts_12, gForgeTexts_13, gForgeTexts_14, gForgeTexts_15,
    gForgeTexts_16, gForgeTexts_17, gForgeTexts_18, gForgeTexts_19,
    gForgeTexts_20, gForgeTexts_21, gForgeTexts_22, gForgeTexts_23,
    gForgeTexts_24, gForgeTexts_25, gForgeTexts_26, gForgeTexts_27,
    gForgeTexts_28, gForgeTexts_29, gForgeTexts_30, gForgeTexts_31,
    gForgeTexts_32, gForgeTexts_33, gForgeTexts_34, gForgeTexts_35,
    gForgeTexts_36, gForgeTexts_37, gForgeTexts_38, gForgeTexts_39,
    gForgeTexts_40, gForgeTexts_41, gForgeTexts_42, gForgeTexts_43,
    gForgeTexts_44, gForgeTexts_45, gForgeTexts_46, gForgeTexts_47,
    gForgeTexts_48, gForgeTexts_49, gForgeTexts_50, gForgeTexts_51,
    gForgeTexts_52, gForgeTexts_53, gForgeTexts_54, gForgeTexts_55,
    gForgeTexts_56, gForgeTexts_57, gForgeTexts_58, gForgeTexts_59,
    gForgeTexts_60, gForgeTexts_61, gForgeTexts_62, D_0083DDC0,
    gForgeTexts_64, gForgeTexts_65, NULL, NULL,
};
/* the kitchen's labels, questions and stat-change lines */
char *gKitchenTexts[20] = {
    gKitchenTexts_0, gKitchenTexts_1, gKitchenTexts_2, gKitchenTexts_3,
    gKitchenTexts_4, gKitchenTexts_5, gKitchenTexts_6, gKitchenTexts_7,
    gKitchenTexts_8, gKitchenTexts_9, gKitchenTexts_10, gKitchenTexts_11,
    gKitchenTexts_12, gKitchenTexts_13, gKitchenTexts_14, gKitchenTexts_15,
    gKitchenTexts_16, gKitchenTexts_17, NULL, NULL,
};
char *gMealNameTexts[16] = {
    gMealNameTexts_0, gMealNameTexts_1, gMealNameTexts_2, gMealNameTexts_3,
    gMealNameTexts_4, gMealNameTexts_5, gMealNameTexts_6, gMealNameTexts_7,
    gMealNameTexts_8, gMealNameTexts_9, gMealNameTexts_10, gMealNameTexts_11,
    gMealNameTexts_12, gMealNameTexts_13, gMealNameTexts_14, gMealNameTexts_15,
};
char *gMealTasteTexts_0[4] = {
    gMealTasteTexts_0_0, gMealTasteTexts_0_1, gMealTasteTexts_0_2, gMealTasteTexts_0_3,
};
char *gMealTasteTexts_1[4] = {
    gMealTasteTexts_1_0, gMealTasteTexts_1_1, gMealTasteTexts_1_2, gMealTasteTexts_1_3,
};
char *gMealTasteTexts_2[4] = {
    gMealTasteTexts_2_0, gMealTasteTexts_2_1, gMealTasteTexts_2_2, gMealTasteTexts_2_3,
};
char *gMealTasteTexts_3[4] = {
    D_0083F130, D_0083F130, D_0083F130, D_0083F130,
};
char *gMealTasteTexts_4[4] = {
    gMealTasteTexts_4_0, gMealTasteTexts_4_1, gMealTasteTexts_4_2, gMealTasteTexts_4_3,
};
char *gMealTasteTexts_5[4] = {
    gMealTasteTexts_5_0, gMealTasteTexts_5_1, gMealTasteTexts_5_2, gMealTasteTexts_5_3,
};
char *gMealTasteTexts_6[4] = {
    gMealTasteTexts_6_0, gMealTasteTexts_6_1, gMealTasteTexts_6_2, gMealTasteTexts_6_3,
};
char **gMealTasteTexts[8] = {
    gMealTasteTexts_0, gMealTasteTexts_1, gMealTasteTexts_2, gMealTasteTexts_3,
    gMealTasteTexts_4, gMealTasteTexts_5, gMealTasteTexts_6, NULL,
};
/* the item box's first menu, by action */
char *gBoxActionTexts[6] = {
    gBoxActionTexts_0, gBoxActionTexts_1, gBoxActionTexts_2, gBoxActionTexts_3,
    gBoxActionTexts_4, NULL,
};
/* the item box's control guide */
char *gBoxControlTexts[2] = {
    gBoxControlTexts_0, gBoxControlTexts_1,
};
/* the control guide's show-the-menu line */
char *gMenuShowTexts[2] = {
    gMenuShowTexts_0, NULL,
};
/* the box's two lists: items, equipment */
char *gBoxKindTexts[2] = {
    D_0083F358, gBoxKindTexts_1,
};
/* the equipment menu's entries */
char *gEquipMenuTexts[8] = {
    gEquipMenuTexts_0, gEquipMenuTexts_1, gEquipMenuTexts_2, gEquipMenuTexts_3,
    gEquipMenuTexts_4, NULL, NULL, NULL,
};
/* the equipment slots' names, by part */
char *gEquipSlotTexts[8] = {
    D_0083CF58, gEquipSlotTexts_1, gEquipSlotTexts_2, gEquipSlotTexts_3,
    gEquipSlotTexts_4, gEquipSlotTexts_5, NULL, NULL,
};
/* the item box's labels and formats */
char *gBoxMenuTexts[24] = {
    gBoxMenuTexts_0, gBoxMenuTexts_1, gBoxMenuTexts_2, gBoxMenuTexts_3,
    gBoxMenuTexts_4, gBoxMenuTexts_5, gBoxMenuTexts_6, gBoxMenuTexts_7,
    gBoxMenuTexts_8, gBoxMenuTexts_9, gBoxMenuTexts_10, gBoxMenuTexts_11,
    gBoxMenuTexts_12, gBoxMenuTexts_13, gBoxMenuTexts_14, gBoxMenuTexts_15,
    gBoxMenuTexts_16, gBoxMenuTexts_17, gBoxMenuTexts_18, gBoxMenuTexts_19,
    gBoxMenuTexts_20, gBoxMenuTexts_21, NULL, NULL,
};
/* the item box's help lines, by message (the field menus' help lists hold it too) */
char *gBoxHelpTexts[72] = {
    gBoxHelpTexts_0, gBoxHelpTexts_1, gBoxHelpTexts_2, gBoxHelpTexts_3,
    gBoxHelpTexts_4, gBoxHelpTexts_5, gBoxHelpTexts_6, gBoxHelpTexts_7,
    gBoxHelpTexts_8, gHunterNotesTexts_8, gBoxHelpTexts_10, gBoxHelpTexts_11,
    gBoxHelpTexts_12, gBoxHelpTexts_13, gBoxHelpTexts_14, D_0083F880,
    gBoxHelpTexts_16, gBoxHelpTexts_17, gBoxHelpTexts_18, gBoxHelpTexts_19,
    gBoxHelpTexts_20, gBoxHelpTexts_21, gBoxHelpTexts_22, gBoxHelpTexts_23,
    gBoxHelpTexts_24, gBoxHelpTexts_25, gBoxHelpTexts_26, gBoxHelpTexts_27,
    D_0083F880, gBoxHelpTexts_29, gBoxHelpTexts_30, gBoxHelpTexts_31,
    gBoxHelpTexts_32, gBoxHelpTexts_33, gBoxHelpTexts_34, gBoxHelpTexts_35,
    gBoxHelpTexts_36, gBoxHelpTexts_37, gBoxHelpTexts_38, gBoxHelpTexts_39,
    gBoxHelpTexts_40, gBoxHelpTexts_41, gBoxHelpTexts_42, gBoxHelpTexts_43,
    gBoxHelpTexts_44, gBoxHelpTexts_45, gBoxHelpTexts_46, gBoxHelpTexts_47,
    gHunterNotesTexts_8, gHunterNotesTexts_8, gBoxHelpTexts_50, gBoxHelpTexts_51,
    gBoxHelpTexts_52, gBoxHelpTexts_53, gBoxHelpTexts_54, gBoxHelpTexts_55,
    gBoxHelpTexts_56, gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8,
    gBoxHelpTexts_60, gBoxHelpTexts_61, gBoxHelpTexts_62, gBoxHelpTexts_63,
    gBoxHelpTexts_64, gBoxHelpTexts_65, gBoxHelpTexts_66, gBoxHelpTexts_67,
    gBoxHelpTexts_68, gBoxHelpTexts_69, gBoxHelpTexts_70, gBoxHelpTexts_71,
};
char *gAreaChatTexts[4] = {
    gAreaChatTexts_0, gAreaChatTexts_1, gAreaChatTexts_2, gAreaChatTexts_3,
};
/* the quest counter's board rows, room pages and arena teams */
char *gQuestCounterTexts[56] = {
    D_0083BFE8, D_0083C050, gQuestCounterTexts_2, gQuestCounterTexts_3,
    gQuestCounterTexts_4, gQuestCounterTexts_5, gQuestCounterTexts_6, gQuestCounterTexts_7,
    gQuestCounterTexts_8, D_00840240, gQuestCounterTexts_10, gQuestCounterTexts_11,
    D_0083D7A0, gQuestCounterTexts_13, gQuestCounterTexts_14, gQuestCounterTexts_15,
    gQuestCounterTexts_16, D_0083D3D8, gQuestCounterTexts_18, D_008402B8,
    D_008402C8, gQuestCounterTexts_21, D_008402E0, D_008402F8,
    gQuestCounterTexts_24, D_00840318, D_00840328, D_00840338,
    gQuestCounterTexts_28, gQuestCounterTexts_29, gQuestCounterTexts_30, gQuestCounterTexts_31,
    gQuestCounterTexts_32, gQuestCounterTexts_33, gQuestCounterTexts_34, gQuestCounterTexts_35,
    gQuestCounterTexts_36, gQuestCounterTexts_37, gQuestCounterTexts_38, gQuestCounterTexts_39,
    gQuestCounterTexts_40, gQuestCounterTexts_41, gQuestCounterTexts_42, gQuestCounterTexts_43,
    gQuestCounterTexts_44, gQuestCounterTexts_45, gQuestCounterTexts_46, gQuestCounterTexts_47,
    gQuestCounterTexts_48, gQuestCounterTexts_49, gQuestCounterTexts_50, gQuestCounterTexts_51,
    gQuestCounterTexts_52, gQuestCounterTexts_53, NULL, NULL,
};
char *gInfoIndexTexts[8] = {
    D_00840638, D_0083C328, gInfoIndexTexts_2, gInfoIndexTexts_3,
    gInfoIndexTexts_4, NULL, NULL, NULL,
};
/* the quest sheets' and the room forms' labels */
char *gQuestSheetTexts[36] = {
    D_0083BFA0, gQuestSheetTexts_1, gQuestSheetTexts_2, gQuestSheetTexts_3,
    gQuestSheetTexts_4, gQuestSheetTexts_5, gQuestSheetTexts_6, gQuestSheetTexts_7,
    gQuestSheetTexts_8, gQuestSheetTexts_9, gQuestSheetTexts_10, gQuestSheetTexts_11,
    gQuestSheetTexts_12, D_008402B8, gQuestSheetTexts_14, D_0083D3D8,
    gQuestSheetTexts_16, gQuestSheetTexts_17, D_00840328, D_00840338,
    D_0083CB20, gQuestSheetTexts_21, gQuestSheetTexts_22, gQuestSheetTexts_23,
    gQuestSheetTexts_24, gQuestSheetTexts_25, D_0083C5B8, gQuestSheetTexts_27,
    gQuestSheetTexts_28, D_008402E0, D_00840828, gQuestSheetTexts_31,
    D_008402F8, D_00840638, gQuestSheetTexts_34, gQuestSheetTexts_35,
};
char *gOfficialEventTexts[4] = {
    gOfficialEventTexts_0, gOfficialEventTexts_1, gOfficialEventTexts_2, gOfficialEventTexts_3,
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
    gGameMenuHelpTexts_0, gGameMenuHelpTexts_1, gGameMenuHelpTexts_2, gGameMenuHelpTexts_3,
    gGameMenuHelpTexts_4, gGameMenuHelpTexts_5, gGameMenuHelpTexts_6, gGameMenuHelpTexts_7,
    gGameMenuHelpTexts_8, gGameMenuHelpTexts_9, D_00840B10, D_00840B30,
    gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8, gHunterNotesTexts_8,
    gGameMenuHelpTexts_16, gGameMenuHelpTexts_17, gGameMenuHelpTexts_18, gGameMenuHelpTexts_19,
    D_00840BF0, gGameMenuHelpTexts_21, gGameMenuHelpTexts_22, D_00840CC0,
    gGameMenuHelpTexts_24, gGameMenuHelpTexts_25, gHunterNotesTexts_8, D_00840B30,
    gGameMenuHelpTexts_28, gGameMenuHelpTexts_29, gGameMenuHelpTexts_30, gGameMenuHelpTexts_31,
    gGameMenuHelpTexts_32, gGameMenuHelpTexts_33, gGameMenuHelpTexts_34, gGameMenuHelpTexts_35,
    gGameMenuHelpTexts_36, gGameMenuHelpTexts_37, gGameMenuHelpTexts_38, gGameMenuHelpTexts_39,
};
/* the search menu's entries, which also title their pages */
char *gSearchMenuTexts[4] = {
    D_0083BC18, gSearchMenuTexts_1, gSearchMenuTexts_2, gSearchMenuTexts_3,
};
char *gMailboxMenuTexts[4] = {
    gMailboxMenuTexts_0, gMailboxMenuTexts_1, gMailboxMenuTexts_2, D_0083C1B8,
};
char *gQuitMenuTexts[2] = {
    D_008410B0, gQuitMenuTexts_1,
};
char *gQuitMenuDimTexts[2] = {
    D_008410B0, gQuitMenuDimTexts_1,
};
char *gQuitSaveTexts[4] = {
    gQuitSaveTexts_0, gQuitSaveTexts_1, NULL, NULL,
};
char *gQuitConfirmTexts[4] = {
    gQuitConfirmTexts_0, gQuitConfirmTexts_1, gQuitConfirmTexts_2, gQuitConfirmTexts_3,
};
/* the chat pages' titles and labels: the speakers, the muted players, the chat state */
char *gChatPageTexts[12] = {
    gChatPageTexts_0, gChatPageTexts_1, gChatPageTexts_2, gChatPageTexts_3,
    D_00841350, gChatPageTexts_5, gChatPageTexts_6, gChatPageTexts_7,
    gChatPageTexts_8, gChatPageTexts_9, NULL, NULL,
};
char *gChatMenuTexts[6] = {
    D_0083BCD8, D_0083BCC8, D_00841410, gChatMenuTexts_3,
    gChatMenuTexts_4, gChatMenuTexts_5,
};
char *gChatStateTexts[2] = {
    gChatStateTexts_0, gChatStateTexts_1,
};
char *gChatUserActTexts[4][3] = {
    {D_0083BC80, gChatUserActTexts_1, gChatUserActTexts_2},
    {D_0083BC80, gChatUserActTexts_4, D_008414D8},
    {D_0083BC80, gChatUserActTexts_7, D_008414D8},
    {D_008414D8, NULL, NULL},
};
/* the friend limit's two settings: off, on */
char *gFriendLimitTexts[2] = {
    D_0083C218, D_0083C228,
};
/* the place info's labels */
char *gPlaceInfoTexts[12] = {
    gPlaceInfoTexts_0, D_00841510, D_0083D088, D_0083CFD0,
    D_0083D030, D_0083D040, gPlaceInfoTexts_6, gPlaceInfoTexts_7,
    gPlaceInfoTexts_8, gPlaceInfoTexts_9, gPlaceInfoTexts_10, gPlaceInfoTexts_11,
};
char *gTavernLevelTexts[4] = {
    gTavernLevelTexts_0, gTavernLevelTexts_1, gTavernLevelTexts_2, NULL,
};
char *gPortLevelTexts[4] = {
    gPortLevelTexts_0, gPortLevelTexts_1, gPortLevelTexts_2, NULL,
};
char *gSmithyLevelTexts[4] = {
    gSmithyLevelTexts_0, gSmithyLevelTexts_1, gSmithyLevelTexts_2, NULL,
};
char *gMineLevelTexts[4] = {
    gMineLevelTexts_0, gMineLevelTexts_1, gMineLevelTexts_2, NULL,
};
/* the online marks' names, by mark (ONLINE_MARK_*) */
char *gOnlineMarkNames[12] = {
    gOnlineMarkNames_0, gOnlineMarkNames_1, gOnlineMarkNames_2, gOnlineMarkNames_3,
    gOnlineMarkNames_4, gOnlineMarkNames_5, gOnlineMarkNames_6, gOnlineMarkNames_7,
    gOnlineMarkNames_8, NULL, NULL, NULL,
};
/* The member status texts, by status (MemberCard_DrawStatus). */
char *gMemberStatusTexts[12] = {
    gMemberStatusTexts_0, gMemberStatusTexts_1, gMemberStatusTexts_2, gMemberStatusTexts_3,
    gMemberStatusTexts_4, gMemberStatusTexts_5, gMemberStatusTexts_6, gMemberStatusTexts_7,
    gMemberStatusTexts_8, NULL, NULL, NULL,
};
/* the shops' menu entries; the item shop's list starts at its second */
char *gShopMenuTexts[4] = {
    D_00841890, D_008418A0, D_008418A8, gShopMenuTexts_3,
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
    gCombineShopTexts_0, gCombineShopTexts_1, gCombineShopTexts_2, gCombineShopTexts_3,
    gCombineShopTexts_4, gCombineShopTexts_5, gCombineShopTexts_6, D_00841920,
    gCombineShopTexts_8, NULL, NULL, NULL,
};
/* the shops' and the gallery's questions and notes */
char *gShopPromptTexts[16] = {
    gShopPromptTexts_0, gShopPromptTexts_1, gShopPromptTexts_2, D_0083C630,
    D_0083C630, gShopPromptTexts_5, gShopPromptTexts_6, gShopPromptTexts_7,
    gShopPromptTexts_8, gShopPromptTexts_9, gShopPromptTexts_10, gShopPromptTexts_11,
    gShopPromptTexts_12, gShopPromptTexts_13, gShopPromptTexts_14, gShopPromptTexts_15,
};
char *gSongHelpTexts[4] = {
    gSongHelpTexts_0, gSongHelpTexts_1, NULL, NULL,
};
char *gMemberCardPanels_0[10] = {
    gMemberCardPanels_0_0, gMemberCardPanels_0_1, gMemberCardPanels_0_2, gMemberCardPanels_0_3,
    gHunterNotesTexts_8, gMemberCardPanels_0_5, gMemberCardPanels_0_6, gMemberCardPanels_0_7,
    gMemberCardPanels_0_8, D_00841510,
};
char *gStatusTitleTexts[2] = {
    gStatusTitleTexts_0, gStatusTitleTexts_1,
};
char *gFriendRegTexts[4] = {
    gFriendRegTexts_0, gFriendRegTexts_1, NULL, NULL,
};
/* the bed's menu: its entries, their help lines, the living cost question */
char *gBedMenuTexts[8] = {
    gBedMenuTexts_0, gBedMenuTexts_1, gBedMenuTexts_2, gBedMenuTexts_3,
    gBedMenuTexts_4, gBedMenuTexts_5, gBedMenuTexts_6, NULL,
};
char *gMonsterClassTexts[76] = {
    gMonsterClassTexts_0, D_00841FF0, D_00841FF8, gHunterNotesTexts_8,
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
    gGalleryCourseTexts_0, gGalleryCourseTexts_1, gGalleryCourseTexts_2, gGalleryCourseTexts_3,
    gGalleryCourseTexts_4, gGalleryCourseTexts_5, gGalleryCourseTexts_6, gGalleryCourseTexts_7,
    gGalleryCourseTexts_8, gGalleryCourseTexts_9, NULL, NULL,
};
/* the notes of the lists and searches that come up empty */
char *gEmptyListTexts[8] = {
    gEmptyListTexts_0, gEmptyListTexts_1, gEmptyListTexts_2, gEmptyListTexts_3,
    gEmptyListTexts_4, gEmptyListTexts_5, D_00841350, gEmptyListTexts_7,
};
char *gLobbyNoticeTexts[44] = {
    gLobbyNoticeTexts_0, gLobbyNoticeTexts_1, gLobbyNoticeTexts_2, gLobbyNoticeTexts_3,
    gLobbyNoticeTexts_4, gLobbyNoticeTexts_5, gLobbyNoticeTexts_6, gLobbyNoticeTexts_7,
    gLobbyNoticeTexts_8, gLobbyNoticeTexts_9, gLobbyNoticeTexts_10, gLobbyNoticeTexts_11,
    gLobbyNoticeTexts_12, gLobbyNoticeTexts_13, gLobbyNoticeTexts_14, gLobbyNoticeTexts_15,
    gLobbyNoticeTexts_16, gLobbyNoticeTexts_17, gLobbyNoticeTexts_18, gLobbyNoticeTexts_19,
    gLobbyNoticeTexts_20, gLobbyNoticeTexts_21, gLobbyNoticeTexts_22, gLobbyNoticeTexts_23,
    gLobbyNoticeTexts_24, gLobbyNoticeTexts_25, gLobbyNoticeTexts_26, gLobbyNoticeTexts_27,
    gLobbyNoticeTexts_28, gLobbyNoticeTexts_29, gLobbyNoticeTexts_30, gLobbyNoticeTexts_31,
    gLobbyNoticeTexts_32, gLobbyNoticeTexts_33, gLobbyNoticeTexts_34, gLobbyNoticeTexts_35,
    gLobbyNoticeTexts_36, gLobbyNoticeTexts_37, gLobbyNoticeTexts_38, gLobbyNoticeTexts_39,
    gLobbyNoticeTexts_40, NULL, NULL, NULL,
};
char *gLobbyEventTexts[12] = {
    gLobbyEventTexts_0, gLobbyEventTexts_1, gLobbyEventTexts_2, gLobbyEventTexts_3,
    gLobbyEventTexts_4, gLobbyEventTexts_5, gLobbyEventTexts_6, gLobbyEventTexts_7,
    gLobbyEventTexts_8, gLobbyEventTexts_9, gTextFillNameFmt, gLobbyEventTexts_11,
};
char *gSpotActionTexts[28] = {
    gSpotActionTexts_0, D_008428D8, gSpotActionTexts_2, gSpotActionTexts_3,
    gSpotActionTexts_4, gSpotActionTexts_5, gSpotActionTexts_6, gSpotActionTexts_7,
    gSpotActionTexts_8, gSpotActionTexts_9, D_008428D8, gSpotActionTexts_11,
    gSpotActionTexts_12, gSpotActionTexts_13, gSpotActionTexts_14, gSpotActionTexts_15,
    D_0083CF08, gSpotActionTexts_17, gSpotActionTexts_18, gSpotActionTexts_19,
    gSpotActionTexts_20, gSpotActionTexts_21, gSpotActionTexts_22, gSpotActionTexts_23,
    gSpotActionTexts_24, gSpotActionTexts_25, gSpotActionTexts_26, NULL,
};
/* the actions the members' name tags offer */
char *gNameTagActionTexts[4] = {
    gNameTagActionTexts_0, gNameTagActionTexts_1, gNameTagActionTexts_2, NULL,
};
char *gLobbyTitleTexts[8] = {
    gLobbyTitleTexts_0, gLobbyTitleTexts_1, gLobbyTitleTexts_2, gLobbyTitleTexts_3,
    gLobbyTitleTexts_4, NULL, NULL, NULL,
};
/* the lobby dialogs' help lines, by help id, then the lists' column heads */
char *gLobbyHelpTexts[68] = {
    gLobbyHelpTexts_0, gLobbyHelpTexts_1, gLobbyHelpTexts_2, gLobbyHelpTexts_3,
    gLobbyHelpTexts_4, gLobbyHelpTexts_5, gLobbyHelpTexts_6, gLobbyHelpTexts_7,
    gLobbyHelpTexts_8, gLobbyHelpTexts_9, gLobbyHelpTexts_10, gLobbyHelpTexts_11,
    D_00840B10, gLobbyHelpTexts_13, gLobbyHelpTexts_14, gLobbyHelpTexts_15,
    gLobbyHelpTexts_16, gLobbyHelpTexts_17, gLobbyHelpTexts_18, gLobbyHelpTexts_19,
    gLobbyHelpTexts_20, gLobbyHelpTexts_21, gLobbyHelpTexts_22, gLobbyHelpTexts_23,
    gLobbyHelpTexts_24, gLobbyHelpTexts_25, gLobbyHelpTexts_26, gLobbyHelpTexts_27,
    gLobbyHelpTexts_28, gLobbyHelpTexts_29, gHunterNotesTexts_8, gLobbyHelpTexts_31,
    gLobbyHelpTexts_32, gLobbyHelpTexts_33, gLobbyHelpTexts_34, gLobbyHelpTexts_35,
    gLobbyHelpTexts_36, gLobbyHelpTexts_37, gLobbyHelpTexts_38, gLobbyHelpTexts_39,
    gLobbyHelpTexts_40, gLobbyHelpTexts_41, D_00840BF0, gLobbyHelpTexts_43,
    gLobbyHelpTexts_44, D_00840CC0, gLobbyHelpTexts_46, gLobbyHelpTexts_47,
    D_0083CF30, D_0083CF58, D_0083CF38, gLobbyHelpTexts_51,
    D_0083CF40, gLobbyHelpTexts_53, D_00843998, gLobbyHelpTexts_55,
    D_0083BB60, D_0083CF28, D_0083C950, D_0083CF48,
    gLobbyHelpTexts_60, gLobbyHelpTexts_61, gLobbyHelpTexts_62, gLobbyHelpTexts_63,
    gLobbyHelpTexts_64, gLobbyHelpTexts_65, gLobbyHelpTexts_66, gLobbyHelpTexts_67,
};
/* the lobby dialogs' labels, titles and choices */
char *gLobbyLabelTexts[58] = {
    gLobbyLabelTexts_0, gLobbyLabelTexts_1, gLobbyLabelTexts_2, D_0083D130,
    gLobbyLabelTexts_4, D_0083D088, D_0083D170, D_0083BC80,
    gLobbyLabelTexts_8, gLobbyLabelTexts_9, gLobbyLabelTexts_10, gLobbyLabelTexts_11,
    gLobbyLabelTexts_12, D_00843A88, gLobbyLabelTexts_14, gLobbyLabelTexts_15,
    gLobbyLabelTexts_16, gLobbyLabelTexts_17, gLobbyLabelTexts_18, D_00843998,
    gLobbyLabelTexts_20, gLobbyLabelTexts_21, D_00840318, D_00840828,
    D_008402B8, D_00840240, gLobbyLabelTexts_26, gLobbyLabelTexts_27,
    gLobbyLabelTexts_28, gLobbyLabelTexts_29, gLobbyLabelTexts_30, gLobbyLabelTexts_31,
    gLobbyLabelTexts_32, gLobbyLabelTexts_33, gLobbyLabelTexts_34, gLobbyLabelTexts_35,
    D_00840318, gLobbyLabelTexts_37, gLobbyLabelTexts_38, gLobbyLabelTexts_39,
    gLobbyLabelTexts_40, gLobbyLabelTexts_41, gLobbyLabelTexts_42, gLobbyLabelTexts_43,
    D_0083D060, gLobbyLabelTexts_45, gLobbyLabelTexts_46, gLobbyLabelTexts_47,
    gLobbyLabelTexts_48, gLobbyLabelTexts_49, D_00841920, gLobbyLabelTexts_51,
    gLobbyLabelTexts_52, gLobbyLabelTexts_53, gLobbyLabelTexts_54, gLobbyLabelTexts_55,
    gLobbyLabelTexts_56, gLobbyLabelTexts_57,
};
/* the lobby dialogs' no and save choices */
char *gLobbyNoSaveTexts[2] = {
    gLobbyNoSaveTexts_0, gLobbyNoSaveTexts_1,
};
/* the lobby dialogs' quit choice, states and button guide labels */
char *gLobbyButtonTexts[40] = {
    gLobbyButtonTexts_0, gLobbyButtonTexts_1, gLobbyButtonTexts_2, D_0083C2E0,
    gLobbyButtonTexts_4, D_0083C5B8, gLobbyButtonTexts_6, gLobbyButtonTexts_7,
    gLobbyButtonTexts_8, gLobbyButtonTexts_9, NULL, NULL,
    gLobbyButtonTexts_12, gLobbyButtonTexts_13, gLobbyButtonTexts_14, gLobbyButtonTexts_15,
    D_0083CFE0, gLobbyButtonTexts_17, gLobbyButtonTexts_18, D_0083BC80,
    gLobbyButtonTexts_20, D_0083D318, gLobbyButtonTexts_22, gLobbyButtonTexts_23,
    gLobbyButtonTexts_24, D_00843A88, gLobbyButtonTexts_26, gLobbyButtonTexts_27,
    gLobbyButtonTexts_28, gLobbyButtonTexts_29, gLobbyButtonTexts_30, D_0083D328,
    gLobbyButtonTexts_32, gLobbyButtonTexts_33, D_008402C8, gLobbyButtonTexts_35,
    D_00841410, gLobbyButtonTexts_37, gLobbyButtonTexts_38, NULL,
};
LobbyMsgDef gLobbyMsgTexts[152] = {
#include "lobby/0080D7B0.inc"
};

/* The file's .rodata: the label tables' strings. */
#pragma rodata on
char gLobbyWords_0[8] = {
#include "lobby/0083BA90.inc"
};
char gLobbyWords_1[8] = {
#include "lobby/0083BA98.inc"
};
char gLobbyWords_2[8] = {
#include "lobby/0083BAA0.inc"
};
char gLobbyWords_3[8] = {
#include "lobby/0083BAA8.inc"
};
char gLobbyWords_4[8] = {
#include "lobby/0083BAB0.inc"
};
char gLobbyWords_5[8] = {
#include "lobby/0083BAB8.inc"
};
char gLobbyWords_6[8] = {
#include "lobby/0083BAC0.inc"
};
char gLobbyWords_7[8] = {
#include "lobby/0083BAC8.inc"
};
char gLobbyWords_8[16] = {
#include "lobby/0083BAD0.inc"
};
char gLobbyWords_9[24] = {
#include "lobby/0083BAE0.inc"
};
char gLobbyWords_10[8] = {
#include "lobby/0083BAF8.inc"
};
char gLobbyWords_11[8] = {
#include "lobby/0083BB00.inc"
};
char gLobbyWords_12[8] = {
#include "lobby/0083BB08.inc"
};
char gLobbyWords_13[8] = {
#include "lobby/0083BB10.inc"
};
char gLobbyWords_14[12] = {
#include "lobby/0083BB18.inc"
};
char gLobbyWords_15[8] = {
#include "lobby/0083BB28.inc"
};
char gLobbyWords_16[8] = {
#include "lobby/0083BB30.inc"
};
char gLobbyWords_17[8] = {
#include "lobby/0083BB38.inc"
};
char gLobbyWords_18[8] = {
#include "lobby/0083BB40.inc"
};
char gLobbyWords_19[8] = {
#include "lobby/0083BB48.inc"
};
char D_0083BB50[8] = {
#include "lobby/0083BB50.inc"
};
char gNoneSetTexts_1[8] = {
#include "lobby/0083BB58.inc"
};
char D_0083BB60[8] = {
#include "lobby/0083BB60.inc"
};
char gCountLabels_1[8] = {
#include "lobby/0083BB68.inc"
};
char gGameMenuLabels_0[8] = {
#include "lobby/0083BB70.inc"
};
char gGameMenuLabels_1[12] = {
#include "lobby/0083BB78.inc"
};
char gGameMenuLabels_2[12] = {
#include "lobby/0083BB88.inc"
};
char gGameMenuLabels_3[12] = {
#include "lobby/0083BB98.inc"
};
char gGameMenuLabels_4[15] = {
#include "lobby/0083BBA8.inc"
};
char gGameMenuLabels_5[24] = {
#include "lobby/0083BBC0.inc"
};
char D_0083BBD8[8] = {
#include "lobby/0083BBD8.inc"
};
char gGameMenuLabels_7[16] = {
#include "lobby/0083BBE0.inc"
};
char gGameMenuLabels_8[16] = {
#include "lobby/0083BBF0.inc"
};
char gGameMenuLabels_9[16] = {
#include "lobby/0083BC00.inc"
};
char D_0083BC10[8] = {
#include "lobby/0083BC10.inc"
};
char D_0083BC18[15] = {
#include "lobby/0083BC18.inc"
};
char gGameMenuLabels_12[15] = {
#include "lobby/0083BC28.inc"
};
char gGameMenuLabels_13[13] = {
#include "lobby/0083BC38.inc"
};
char gGameMenuLabels_14[8] = {
#include "lobby/0083BC48.inc"
};
char D_0083BC50[16] = {
#include "lobby/0083BC50.inc"
};
char D_0083BC60[16] = {
#include "lobby/0083BC60.inc"
};
char gGameMenuLabels_17[16] = {
#include "lobby/0083BC70.inc"
};
char D_0083BC80[16] = {
#include "lobby/0083BC80.inc"
};
char gGameMenuLabels_19[16] = {
#include "lobby/0083BC90.inc"
};
char gGameMenuLabels_20[16] = {
#include "lobby/0083BCA0.inc"
};
char gGameMenuLabels_21[24] = {
#include "lobby/0083BCB0.inc"
};
char D_0083BCC8[12] = {
#include "lobby/0083BCC8.inc"
};
char D_0083BCD8[13] = {
#include "lobby/0083BCD8.inc"
};
char gGameMenuLabels_24[12] = {
#include "lobby/0083BCE8.inc"
};
char gGameMenuLabels_25[24] = {
#include "lobby/0083BD00.inc"
};
char gGameMenuLabels_26[12] = {
#include "lobby/0083BD18.inc"
};
char gGameMenuLabels_27[13] = {
#include "lobby/0083BD28.inc"
};
char gGameMenuLabels_28[24] = {
#include "lobby/0083BD40.inc"
};
char gGameMenuLabels_29[15] = {
#include "lobby/0083BD58.inc"
};
char gGameMenuLabels_30[13] = {
#include "lobby/0083BD68.inc"
};
char gGameMenuLabels_31[12] = {
#include "lobby/0083BD78.inc"
};
char gPriceInfoTexts_0[32] = {
#include "lobby/0083BD90.inc"
};
char gWantedLineTexts_0[32] = {
#include "lobby/0083BDB0.inc"
};
char gWantedLineTexts_1[32] = {
#include "lobby/0083BDD0.inc"
};
char gWantedLineTexts_2[24] = {
#include "lobby/0083BDF0.inc"
};
char D_0083BE08[13] = {
#include "lobby/0083BE08.inc"
};
char D_0083BE18[12] = {
#include "lobby/0083BE18.inc"
};
char D_0083BE28[13] = {
#include "lobby/0083BE28.inc"
};
char D_0083BE38[12] = {
#include "lobby/0083BE38.inc"
};
char D_0083BE48[12] = {
#include "lobby/0083BE48.inc"
};
char D_0083BE58[15] = {
#include "lobby/0083BE58.inc"
};
char D_0083BE68[8] = {
#include "lobby/0083BE68.inc"
};
char gListTitles_0[8] = {
#include "lobby/0083BE70.inc"
};
char gListTitles_1[12] = {
#include "lobby/0083BE78.inc"
};
char gListTitles_2[16] = {
#include "lobby/0083BE90.inc"
};
char gListTitles_3[16] = {
#include "lobby/0083BEA0.inc"
};
char gListTitles_4[16] = {
#include "lobby/0083BEB0.inc"
};
char gListTitles_5[16] = {
#include "lobby/0083BEC0.inc"
};
char gListTitles_6[16] = {
#include "lobby/0083BED0.inc"
};
char gListTitles_7[16] = {
#include "lobby/0083BEE0.inc"
};
char gListTitles_8[16] = {
#include "lobby/0083BEF0.inc"
};
char gListTitles_9[16] = {
#include "lobby/0083BF00.inc"
};
char gListTitles_10[16] = {
#include "lobby/0083BF10.inc"
};
char gListTitles_11[16] = {
#include "lobby/0083BF20.inc"
};
char gListTitles_12[16] = {
#include "lobby/0083BF30.inc"
};
char gListTitles_13[16] = {
#include "lobby/0083BF40.inc"
};
char gListTitles_14[16] = {
#include "lobby/0083BF50.inc"
};
char gListTitles_15[16] = {
#include "lobby/0083BF60.inc"
};
char gQuestBoardRows_0[16] = {
#include "lobby/0083BF70.inc"
};
char gQuestBoardRows_1[16] = {
#include "lobby/0083BF80.inc"
};
char gQuestBoardRows_2[16] = {
#include "lobby/0083BF90.inc"
};
char D_0083BFA0[16] = {
#include "lobby/0083BFA0.inc"
};
char gQuestBoardRows_4[32] = {
#include "lobby/0083BFB0.inc"
};
char gQuestBoardRows_5[24] = {
#include "lobby/0083BFD0.inc"
};
char D_0083BFE8[12] = {
#include "lobby/0083BFE8.inc"
};
char gQuestBoardRows_7[15] = {
#include "lobby/0083BFF8.inc"
};
char gQuestBoardRows_8[12] = {
#include "lobby/0083C008.inc"
};
char gQuestBoardRows_9[12] = {
#include "lobby/0083C018.inc"
};
char gQuestBoardRows_10[32] = {
#include "lobby/0083C030.inc"
};
char D_0083C050[16] = {
#include "lobby/0083C050.inc"
};
char gTownLabels_0[24] = {
#include "lobby/0083C060.inc"
};
char gTownLabels_1[8] = {
#include "lobby/0083C078.inc"
};
char gTownLabels_2[8] = {
#include "lobby/0083C080.inc"
};
char gTownLabels_3[12] = {
#include "lobby/0083C088.inc"
};
char gTownLabels_4[8] = {
#include "lobby/0083C098.inc"
};
char gTownLabels_5[24] = {
#include "lobby/0083C0A0.inc"
};
char gTownLabels_6[13] = {
#include "lobby/0083C0B8.inc"
};
char gTownLabels_7[24] = {
#include "lobby/0083C0D0.inc"
};
char gTownLabels_8[15] = {
#include "lobby/0083C0E8.inc"
};
char gTownLabels_9[32] = {
#include "lobby/0083C100.inc"
};
char gTownLabels_10[48] = {
#include "lobby/0083C120.inc"
};
char gTownLabels_11[32] = {
#include "lobby/0083C150.inc"
};
char gTownLabels_12[16] = {
#include "lobby/0083C170.inc"
};
char gTownLabels_13[32] = {
#include "lobby/0083C180.inc"
};
char gTownLabels_14[8] = {
#include "lobby/0083C1A0.inc"
};
char gTownLabels_16[12] = {
#include "lobby/0083C1A8.inc"
};
char D_0083C1B8[12] = {
#include "lobby/0083C1B8.inc"
};
char gTownLabels_18[32] = {
#include "lobby/0083C1D0.inc"
};
char gTownLabels_19[24] = {
#include "lobby/0083C1F0.inc"
};
char gTownLabels_20[12] = {
#include "lobby/0083C208.inc"
};
char D_0083C218[12] = {
#include "lobby/0083C218.inc"
};
char D_0083C228[12] = {
#include "lobby/0083C228.inc"
};
char gTownLabels_23[24] = {
#include "lobby/0083C240.inc"
};
char gAreaNameTexts_0[8] = {
#include "lobby/0083C258.inc"
};
char gAreaNameTexts_1[16] = {
#include "lobby/0083C260.inc"
};
char gAreaNameTexts_2[16] = {
#include "lobby/0083C270.inc"
};
char gAreaNameTexts_3[16] = {
#include "lobby/0083C280.inc"
};
char gAreaNameTexts_4[16] = {
#include "lobby/0083C290.inc"
};
char gAreaNameTexts_5[8] = {
#include "lobby/0083C2A0.inc"
};
char gAreaNameTexts_6[12] = {
#include "lobby/0083C2A8.inc"
};
char D_0083C2B8[8] = {
#include "lobby/0083C2B8.inc"
};
char gAreaNameTexts_10[16] = {
#include "lobby/0083C2C0.inc"
};
char D_0083C2D0[16] = {
#include "lobby/0083C2D0.inc"
};
char D_0083C2E0[16] = {
#include "lobby/0083C2E0.inc"
};
char gShipMenuTexts_0[16] = {
#include "lobby/0083C2F0.inc"
};
char D_0083C300[16] = {
#include "lobby/0083C300.inc"
};
char gTownGuideTopics_0[16] = {
#include "lobby/0083C310.inc"
};
char gTownGuideTopics_1[8] = {
#include "lobby/0083C320.inc"
};
char D_0083C328[12] = {
#include "lobby/0083C328.inc"
};
char gTownGuideHelp_0[48] = {
#include "lobby/0083C340.inc"
};
char gTownGuideHelp_1[48] = {
#include "lobby/0083C370.inc"
};
char gTownGuideHelp_2[40] = {
#include "lobby/0083C3A0.inc"
};
char gTownGuideHelp_3[8] = {
#include "lobby/0083C3C8.inc"
};
char gTownGuideHelp_4[16] = {
#include "lobby/0083C3D0.inc"
};
char gVillageBoardTitles_0[32] = {
#include "lobby/0083C3E0.inc"
};
char gVillageBoardTitles_1[64] = {
#include "lobby/0083C400.inc"
};
char gVillageBoardTitles_2[48] = {
#include "lobby/0083C440.inc"
};
char gVillageBoardTitles_3[40] = {
#include "lobby/0083C470.inc"
};
char gVillageBoardTexts_0[13] = {
#include "lobby/0083C498.inc"
};
char gVillageBoardTexts_1[24] = {
#include "lobby/0083C4B0.inc"
};
char gVillageBoardTexts_2[13] = {
#include "lobby/0083C4C8.inc"
};
char gVillageBoardTexts_3[12] = {
#include "lobby/0083C4D8.inc"
};
char gVillageBoardTexts_4[12] = {
#include "lobby/0083C4E8.inc"
};
char gVillageBoardTexts_5[32] = {
#include "lobby/0083C500.inc"
};
char gVillageBoardTexts_6[32] = {
#include "lobby/0083C520.inc"
};
char gVillageBoardTexts_7[32] = {
#include "lobby/0083C540.inc"
};
char gVillageBoardTexts_8[32] = {
#include "lobby/0083C560.inc"
};
char gVillageBoardTexts_9[24] = {
#include "lobby/0083C580.inc"
};
char D_0083C598[15] = {
#include "lobby/0083C598.inc"
};
char gElderForecastTexts_1[15] = {
#include "lobby/0083C5A8.inc"
};
char D_0083C5B8[12] = {
#include "lobby/0083C5B8.inc"
};
char gElderForecastTexts_5[8] = {
#include "lobby/0083C5C8.inc"
};
char gElderForecastTexts_6[8] = {
#include "lobby/0083C5D0.inc"
};
char gElderForecastTexts_7[8] = {
#include "lobby/0083C5D8.inc"
};
char gHairMenuTexts_0[16] = {
#include "lobby/0083C5E0.inc"
};
char gHairMenuTexts_1[32] = {
#include "lobby/0083C5F0.inc"
};
char gHairMenuTexts_2[32] = {
#include "lobby/0083C610.inc"
};
char D_0083C630[48] = {
#include "lobby/0083C630.inc"
};
char gHairMenuTexts_4[48] = {
#include "lobby/0083C660.inc"
};
char gRecordsMenuTexts_0[24] = {
#include "lobby/0083C690.inc"
};
char gRecordsMenuTexts_1[15] = {
#include "lobby/0083C6A8.inc"
};
char gRecordsMenuTexts_2[12] = {
#include "lobby/0083C6B8.inc"
};
char gRecordsMenuTexts_3[13] = {
#include "lobby/0083C6C8.inc"
};
char gRecordsMenuTexts_4[12] = {
#include "lobby/0083C6D8.inc"
};
char gRecordsMenuTexts_5[12] = {
#include "lobby/0083C6E8.inc"
};
char gRecordsMenuTexts_6[8] = {
#include "lobby/0083C6F8.inc"
};
char gRecordsMenuTexts_7[16] = {
#include "lobby/0083C700.inc"
};
char gRecordsMenuTexts_8[32] = {
#include "lobby/0083C710.inc"
};
char gRecordsMenuTexts_12[48] = {
#include "lobby/0083C730.inc"
};
char gRecordsMenuTexts_13[48] = {
#include "lobby/0083C760.inc"
};
char gRecordsMenuTexts_14[48] = {
#include "lobby/0083C790.inc"
};
char gRecordsMenuTexts_15[80] = {
#include "lobby/0083C7C0.inc"
};
char gRecordsMenuTexts_16[32] = {
#include "lobby/0083C810.inc"
};
char gRecordsMenuTexts_17[48] = {
#include "lobby/0083C830.inc"
};
char gRecordsMenuTexts_18[48] = {
#include "lobby/0083C860.inc"
};
char gRecordsMenuTexts_19[32] = {
#include "lobby/0083C890.inc"
};
char gRecordsMenuTexts_20[32] = {
#include "lobby/0083C8B0.inc"
};
char gHouseAccessTexts_0[16] = {
#include "lobby/0083C8D0.inc"
};
char gHouseAccessTexts_1[16] = {
#include "lobby/0083C8E0.inc"
};
char gHouseAccessTexts_2[24] = {
#include "lobby/0083C8F0.inc"
};
char gHouseAccessTexts_3[12] = {
#include "lobby/0083C908.inc"
};
char gRecordsListTexts_0[15] = {
#include "lobby/0083C918.inc"
};
char gRecordsListTexts_1[8] = {
#include "lobby/0083C928.inc"
};
char gRecordsListTexts_2[16] = {
#include "lobby/0083C930.inc"
};
char gRecordsListTexts_3[16] = {
#include "lobby/0083C940.inc"
};
char D_0083C950[16] = {
#include "lobby/0083C950.inc"
};
char D_0083C960[32] = {
#include "lobby/0083C960.inc"
};
char D_0083C980[32] = {
#include "lobby/0083C980.inc"
};
char gRecordsListTexts_7[32] = {
#include "lobby/0083C9A0.inc"
};
#pragma explicit_zero_data on
char gHunterNotesTexts_8[16] = {
#include "lobby/0083C9C0.inc"
};
#pragma explicit_zero_data reset
char gRecordsListTexts_9[24] = {
#include "lobby/0083C9D0.inc"
};
char gRecordsListTexts_10[8] = {
#include "lobby/0083C9E8.inc"
};
char gRecordsListTexts_11[8] = {
#include "lobby/0083C9F0.inc"
};
char gRecordsListTexts_12[12] = {
#include "lobby/0083C9F8.inc"
};
char gRecordsListTexts_13[8] = {
#include "lobby/0083CA08.inc"
};
char gRecordsListTexts_14[24] = {
#include "lobby/0083CA10.inc"
};
char gRecordsListTexts_15[12] = {
#include "lobby/0083CA28.inc"
};
char gRecordsListTexts_16[8] = {
#include "lobby/0083CA38.inc"
};
char gRecordsListTexts_19[32] = {
#include "lobby/0083CA40.inc"
};
char gRecordsListTexts_20[16] = {
#include "lobby/0083CA60.inc"
};
char gRecordsListTexts_21[32] = {
#include "lobby/0083CA70.inc"
};
char gRecordsListTexts_22[32] = {
#include "lobby/0083CA90.inc"
};
char gRecordsListTexts_23[24] = {
#include "lobby/0083CAB0.inc"
};
char gSongMenuTexts_0[12] = {
#include "lobby/0083CAC8.inc"
};
char gSongMenuTexts_1[12] = {
#include "lobby/0083CAD8.inc"
};
char gSongMenuTexts_2[8] = {
#include "lobby/0083CAE8.inc"
};
char gOfferMenuTexts_0[16] = {
#include "lobby/0083CAF0.inc"
};
char gOfferMenuTexts_1[16] = {
#include "lobby/0083CB00.inc"
};
char gOfferMenuTexts_2[16] = {
#include "lobby/0083CB10.inc"
};
char D_0083CB20[16] = {
#include "lobby/0083CB20.inc"
};
char gOfferMenuTexts_4[8] = {
#include "lobby/0083CB30.inc"
};
char D_0083CB38[12] = {
#include "lobby/0083CB38.inc"
};
char gOfferMenuTexts_6[48] = {
#include "lobby/0083CB50.inc"
};
char gOfferMenuTexts_7[80] = {
#include "lobby/0083CB80.inc"
};
char gOfferMenuTexts_8[48] = {
#include "lobby/0083CBD0.inc"
};
char gOfferMenuTexts_9[32] = {
#include "lobby/0083CC00.inc"
};
char gQuestPeriodTexts_0[16] = {
#include "lobby/0083CC20.inc"
};
char gQuestPeriodTexts_1[16] = {
#include "lobby/0083CC30.inc"
};
char gOfferHelpTexts_0[48] = {
#include "lobby/0083CC40.inc"
};
char gOfferHelpTexts_1[80] = {
#include "lobby/0083CC70.inc"
};
char gOfferHelpTexts_2[80] = {
#include "lobby/0083CCC0.inc"
};
char gOfferHelpTexts_3[48] = {
#include "lobby/0083CD10.inc"
};
char gOfferHelpTexts_4[80] = {
#include "lobby/0083CD40.inc"
};
char gDojoMenuTexts_0[32] = {
#include "lobby/0083CD90.inc"
};
char gDojoMenuTexts_1[24] = {
#include "lobby/0083CDB0.inc"
};
char gDojoMenuTexts_3[12] = {
#include "lobby/0083CDC8.inc"
};
char gDojoMenuTexts_4[15] = {
#include "lobby/0083CDD8.inc"
};
char gDojoMenuTexts_5[13] = {
#include "lobby/0083CDE8.inc"
};
char gDojoMenuTexts_6[12] = {
#include "lobby/0083CDF8.inc"
};
char gDojoMenuTexts_7[12] = {
#include "lobby/0083CE08.inc"
};
char gDojoMenuTexts_8[12] = {
#include "lobby/0083CE18.inc"
};
char gDojoMenuTexts_9[48] = {
#include "lobby/0083CE30.inc"
};
char gDojoMenuTexts_10[32] = {
#include "lobby/0083CE60.inc"
};
char gDojoMenuTexts_11[48] = {
#include "lobby/0083CE80.inc"
};
char gDojoMenuTexts_12[24] = {
#include "lobby/0083CEB0.inc"
};
char gDojoMenuTexts_13[12] = {
#include "lobby/0083CEC8.inc"
};
char gDojoMenuTexts_14[12] = {
#include "lobby/0083CED8.inc"
};
char gDojoMenuTexts_15[24] = {
#include "lobby/0083CEF0.inc"
};
char D_0083CF08[15] = {
#include "lobby/0083CF08.inc"
};
char gNetLabels_1[8] = {
#include "lobby/0083CF18.inc"
};
char gNetLabels_2[8] = {
#include "lobby/0083CF20.inc"
};
char D_0083CF28[8] = {
#include "lobby/0083CF28.inc"
};
char D_0083CF30[8] = {
#include "lobby/0083CF30.inc"
};
char D_0083CF38[8] = {
#include "lobby/0083CF38.inc"
};
char D_0083CF40[8] = {
#include "lobby/0083CF40.inc"
};
char D_0083CF48[12] = {
#include "lobby/0083CF48.inc"
};
char D_0083CF58[8] = {
#include "lobby/0083CF58.inc"
};
char gNetLabels_10[8] = {
#include "lobby/0083CF60.inc"
};
char gNetLabels_11[12] = {
#include "lobby/0083CF68.inc"
};
char gNetLabels_12[64] = {
#include "lobby/0083CF80.inc"
};
char gNetLabels_13[16] = {
#include "lobby/0083CFC0.inc"
};
char D_0083CFD0[16] = {
#include "lobby/0083CFD0.inc"
};
char D_0083CFE0[8] = {
#include "lobby/0083CFE0.inc"
};
char gNetLabels_17[13] = {
#include "lobby/0083CFE8.inc"
};
char gNetLabels_18[24] = {
#include "lobby/0083D000.inc"
};
char gNetLabels_19[13] = {
#include "lobby/0083D018.inc"
};
char gNetLabels_21[8] = {
#include "lobby/0083D028.inc"
};
char D_0083D030[16] = {
#include "lobby/0083D030.inc"
};
char D_0083D040[16] = {
#include "lobby/0083D040.inc"
};
char gNetLabels_25[16] = {
#include "lobby/0083D050.inc"
};
char D_0083D060[16] = {
#include "lobby/0083D060.inc"
};
char gNetLabels_27[24] = {
#include "lobby/0083D070.inc"
};
char D_0083D088[12] = {
#include "lobby/0083D088.inc"
};
char gNetLabels_29[24] = {
#include "lobby/0083D0A0.inc"
};
char gNetLabels_30[12] = {
#include "lobby/0083D0B8.inc"
};
char gNetLabels_31[15] = {
#include "lobby/0083D0C8.inc"
};
char gNetLabels_32[15] = {
#include "lobby/0083D0D8.inc"
};
char gNetLabels_33[15] = {
#include "lobby/0083D0E8.inc"
};
char gNetLabels_35[8] = {
#include "lobby/0083D0F8.inc"
};
char gNetLabels_36[24] = {
#include "lobby/0083D100.inc"
};
char gNetLabels_37[15] = {
#include "lobby/0083D118.inc"
};
char gNetLabels_38[8] = {
#include "lobby/0083D128.inc"
};
char D_0083D130[16] = {
#include "lobby/0083D130.inc"
};
char gNetLabels_40[24] = {
#include "lobby/0083D140.inc"
};
char gNetLabels_41[12] = {
#include "lobby/0083D158.inc"
};
char gNetLabels_42[8] = {
#include "lobby/0083D168.inc"
};
char D_0083D170[16] = {
#include "lobby/0083D170.inc"
};
char gNetLabels_44[8] = {
#include "lobby/0083D180.inc"
};
char gNetLabels_45[8] = {
#include "lobby/0083D188.inc"
};
char gNetLabels_46[8] = {
#include "lobby/0083D190.inc"
};
char gNetLabels_48[15] = {
#include "lobby/0083D198.inc"
};
char gNetLabels_49[112] = {
#include "lobby/0083D1B0.inc"
};
char gNetLabels_50[96] = {
#include "lobby/0083D220.inc"
};
char gNetLabels_51[48] = {
#include "lobby/0083D280.inc"
};
char gGroupSearchHelpTexts_0[96] = {
#include "lobby/0083D2B0.inc"
};
char gMemberActTexts_0[8] = {
#include "lobby/0083D310.inc"
};
char D_0083D318[13] = {
#include "lobby/0083D318.inc"
};
char D_0083D328[12] = {
#include "lobby/0083D328.inc"
};
char gMailActTexts_0[8] = {
#include "lobby/0083D338.inc"
};
char gMailActTexts_2[16] = {
#include "lobby/0083D340.inc"
};
char D_0083D350[16] = {
#include "lobby/0083D350.inc"
};
char D_0083D360[16] = {
#include "lobby/0083D360.inc"
};
char gGroupLimitTexts_4[8] = {
#include "lobby/0083D370.inc"
};
char gGroupLimitTexts_5[8] = {
#include "lobby/0083D378.inc"
};
char D_0083D380[8] = {
#include "lobby/0083D380.inc"
};
char gGroupLimitNewTexts_3[15] = {
#include "lobby/0083D388.inc"
};
char gGroupLimitExpTexts_3[15] = {
#include "lobby/0083D398.inc"
};
char gArenaCupTexts_0[15] = {
#include "lobby/0083D3A8.inc"
};
char gArenaCupTexts_1[12] = {
#include "lobby/0083D3B8.inc"
};
char gArenaCupTexts_2[12] = {
#include "lobby/0083D3C8.inc"
};
char D_0083D3D8[12] = {
#include "lobby/0083D3D8.inc"
};
char gArenaCupTexts_5[12] = {
#include "lobby/0083D3E8.inc"
};
char gArenaCupTexts_6[8] = {
#include "lobby/0083D3F8.inc"
};
char gArenaCupTexts_7[32] = {
#include "lobby/0083D400.inc"
};
char gArenaCupTexts_8[32] = {
#include "lobby/0083D420.inc"
};
char gArenaCupTexts_9[24] = {
#include "lobby/0083D440.inc"
};
char gArenaCupTexts_10[12] = {
#include "lobby/0083D458.inc"
};
char gArenaCupTexts_11[12] = {
#include "lobby/0083D468.inc"
};
char gArenaCupTexts_12[12] = {
#include "lobby/0083D478.inc"
};
char gArenaCupTexts_13[12] = {
#include "lobby/0083D488.inc"
};
char gArenaCupTexts_14[12] = {
#include "lobby/0083D498.inc"
};
char gArenaCupTexts_15[15] = {
#include "lobby/0083D4A8.inc"
};
char D_0083D4B8[8] = {
#include "lobby/0083D4B8.inc"
};
char D_0083D4C0[16] = {
#include "lobby/0083D4C0.inc"
};
char gArenaCupTexts_20[48] = {
#include "lobby/0083D4D0.inc"
};
char gArenaCupTexts_21[32] = {
#include "lobby/0083D500.inc"
};
char gArenaCupTexts_22[32] = {
#include "lobby/0083D520.inc"
};
char gArenaCupTexts_23[16] = {
#include "lobby/0083D540.inc"
};
char gArenaCupTexts_24[16] = {
#include "lobby/0083D550.inc"
};
char gArenaCupTexts_25[16] = {
#include "lobby/0083D560.inc"
};
char gArenaCupTexts_26[32] = {
#include "lobby/0083D570.inc"
};
char gArenaCupTexts_27[16] = {
#include "lobby/0083D590.inc"
};
char gArenaCupTexts_30[8] = {
#include "lobby/0083D5A0.inc"
};
char gArenaCupTexts_31[8] = {
#include "lobby/0083D5A8.inc"
};
char gArenaCupTexts_33[8] = {
#include "lobby/0083D5B0.inc"
};
char gArenaCupTexts_34[12] = {
#include "lobby/0083D5B8.inc"
};
char gArenaCupTexts_35[12] = {
#include "lobby/0083D5C8.inc"
};
char gArenaCupTexts_36[12] = {
#include "lobby/0083D5D8.inc"
};
char gArenaCupTexts_37[13] = {
#include "lobby/0083D5E8.inc"
};
char gArenaCupTexts_38[32] = {
#include "lobby/0083D600.inc"
};
char gArenaCupTexts_39[96] = {
#include "lobby/0083D620.inc"
};
char gArenaFeedTexts_0[128] = {
#include "lobby/0083D680.inc"
};
char gArenaFeedTexts_1[24] = {
#include "lobby/0083D700.inc"
};
char gArenaFeedTexts_4[8] = {
#include "lobby/0083D718.inc"
};
char gArenaFeedTexts_5[8] = {
#include "lobby/0083D720.inc"
};
char gArenaFeedTexts_6[8] = {
#include "lobby/0083D728.inc"
};
char gArenaFeedTexts_7[8] = {
#include "lobby/0083D730.inc"
};
char gArenaFeedTexts_8[8] = {
#include "lobby/0083D738.inc"
};
char gArenaFeedTexts_9[8] = {
#include "lobby/0083D740.inc"
};
char gArenaFeedTexts_10[8] = {
#include "lobby/0083D748.inc"
};
char gArenaFeedTexts_11[8] = {
#include "lobby/0083D750.inc"
};
char gArenaFeedTexts_12[8] = {
#include "lobby/0083D758.inc"
};
char gArenaFeedTexts_13[8] = {
#include "lobby/0083D760.inc"
};
char gLobbyMarks_0[8] = {
#include "lobby/0083D768.inc"
};
char gLobbyMarks_1[8] = {
#include "lobby/0083D770.inc"
};
char gMemberCountTexts_0[8] = {
#include "lobby/0083D778.inc"
};
char gMemberCountTexts_1[8] = {
#include "lobby/0083D780.inc"
};
char gMemberCountTexts_2[8] = {
#include "lobby/0083D788.inc"
};
char gMemberCountTexts_3[16] = {
#include "lobby/0083D790.inc"
};
char D_0083D7A0[24] = {
#include "lobby/0083D7A0.inc"
};
char gMemberCountTexts_5[8] = {
#include "lobby/0083D7B8.inc"
};
char gMiscTexts_0[32] = {
#include "lobby/0083D7C0.inc"
};
char gMiscTexts_1[32] = {
#include "lobby/0083D7E0.inc"
};
char gIdMenuTexts_0[32] = {
#include "lobby/0083D800.inc"
};
char gIdMenuTexts_1[24] = {
#include "lobby/0083D820.inc"
};
char gWeaponArmorTexts_1[8] = {
#include "lobby/0083D838.inc"
};
char D_0083D840[16] = {
#include "lobby/0083D840.inc"
};
char D_0083D850[16] = {
#include "lobby/0083D850.inc"
};
char D_0083D860[32] = {
#include "lobby/0083D860.inc"
};
char D_0083D880[24] = {
#include "lobby/0083D880.inc"
};
char D_0083D898[13] = {
#include "lobby/0083D898.inc"
};
char gEquipGroupTexts_5[12] = {
#include "lobby/0083D8A8.inc"
};
char gEquipGroupTexts_6[12] = {
#include "lobby/0083D8B8.inc"
};
char gEquipGroupTexts_7[12] = {
#include "lobby/0083D8C8.inc"
};
char gEquipGroupTexts_8[12] = {
#include "lobby/0083D8D8.inc"
};
char gEquipGroupTexts_9[12] = {
#include "lobby/0083D8E8.inc"
};
char gEquipLimitTexts_0[13] = {
#include "lobby/0083D8F8.inc"
};
char gEquipLimitTexts_1[12] = {
#include "lobby/0083D908.inc"
};
char gEquipLimitTexts_2[12] = {
#include "lobby/0083D918.inc"
};
char gEquipLimitTexts_3[12] = {
#include "lobby/0083D928.inc"
};
char gEquipLimitTexts_4[32] = {
#include "lobby/0083D940.inc"
};
char gEquipLimitTexts_5[24] = {
#include "lobby/0083D960.inc"
};
char gEquipLimitTexts_6[13] = {
#include "lobby/0083D978.inc"
};
char gEquipLimitTexts_7[13] = {
#include "lobby/0083D988.inc"
};
char gEquipNowTexts_0[48] = {
#include "lobby/0083D9A0.inc"
};
char gEquipNowTexts_1[48] = {
#include "lobby/0083D9D0.inc"
};
char gEquipShopTexts_0[80] = {
#include "lobby/0083DA00.inc"
};
char gEquipShopTexts_1[80] = {
#include "lobby/0083DA50.inc"
};
char gEquipShopTexts_2[48] = {
#include "lobby/0083DAA0.inc"
};
char D_0083DAD0[32] = {
#include "lobby/0083DAD0.inc"
};
char gEquipShopTexts_4[48] = {
#include "lobby/0083DAF0.inc"
};
char gEquipShopTexts_5[80] = {
#include "lobby/0083DB20.inc"
};
char gEquipShopTexts_6[64] = {
#include "lobby/0083DB70.inc"
};
char gEquipShopTexts_7[48] = {
#include "lobby/0083DBB0.inc"
};
char gEquipShopTexts_8[80] = {
#include "lobby/0083DBE0.inc"
};
char gEquipShopTexts_9[72] = {
#include "lobby/0083DC30.inc"
};
char gSmithyMenuTexts_0[12] = {
#include "lobby/0083DC78.inc"
};
char gSmithyMenuTexts_1[12] = {
#include "lobby/0083DC88.inc"
};
char gSmithyMenuTexts_2[8] = {
#include "lobby/0083DC98.inc"
};
char D_0083DCA0[16] = {
#include "lobby/0083DCA0.inc"
};
char D_0083DCB0[16] = {
#include "lobby/0083DCB0.inc"
};
char gWorkshopTexts_3[32] = {
#include "lobby/0083DCC0.inc"
};
char gNoArmorToMakeTexts_0[32] = {
#include "lobby/0083DCE0.inc"
};
char gWeaponForgeTexts_1[16] = {
#include "lobby/0083DD00.inc"
};
char gArmorForgeTexts_1[16] = {
#include "lobby/0083DD10.inc"
};
char gDecoMenuTexts_0[16] = {
#include "lobby/0083DD20.inc"
};
char gDecoMenuTexts_1[16] = {
#include "lobby/0083DD30.inc"
};
char gDecoMenuTexts_2[16] = {
#include "lobby/0083DD40.inc"
};
char gDecoMenuTexts_3[16] = {
#include "lobby/0083DD50.inc"
};
char gDecoMenuTexts_4[16] = {
#include "lobby/0083DD60.inc"
};
char gDecoMenuTexts_5[16] = {
#include "lobby/0083DD70.inc"
};
char gDecoMenuTexts_6[16] = {
#include "lobby/0083DD80.inc"
};
char gDecoMenuTexts_7[16] = {
#include "lobby/0083DD90.inc"
};
char gForgeTexts_0[32] = {
#include "lobby/0083DDA0.inc"
};
char D_0083DDC0[24] = {
#include "lobby/0083DDC0.inc"
};
char D_0083DDD8[15] = {
#include "lobby/0083DDD8.inc"
};
char gForgeTexts_5[64] = {
#include "lobby/0083DDF0.inc"
};
char gForgeTexts_6[64] = {
#include "lobby/0083DE30.inc"
};
char gForgeTexts_7[64] = {
#include "lobby/0083DE70.inc"
};
char gForgeTexts_8[48] = {
#include "lobby/0083DEB0.inc"
};
char gForgeTexts_10[80] = {
#include "lobby/0083DEE0.inc"
};
char gForgeTexts_11[96] = {
#include "lobby/0083DF30.inc"
};
char gForgeTexts_12[32] = {
#include "lobby/0083DF90.inc"
};
char gForgeTexts_13[32] = {
#include "lobby/0083DFB0.inc"
};
char gForgeTexts_14[24] = {
#include "lobby/0083DFD0.inc"
};
char gForgeTexts_15[13] = {
#include "lobby/0083DFE8.inc"
};
char gForgeTexts_16[13] = {
#include "lobby/0083DFF8.inc"
};
char gForgeTexts_17[15] = {
#include "lobby/0083E008.inc"
};
char gForgeTexts_18[24] = {
#include "lobby/0083E020.inc"
};
char gForgeTexts_19[13] = {
#include "lobby/0083E038.inc"
};
char gForgeTexts_20[32] = {
#include "lobby/0083E050.inc"
};
char gForgeTexts_21[32] = {
#include "lobby/0083E070.inc"
};
char gForgeTexts_22[32] = {
#include "lobby/0083E090.inc"
};
char gForgeTexts_23[32] = {
#include "lobby/0083E0B0.inc"
};
char gForgeTexts_24[32] = {
#include "lobby/0083E0D0.inc"
};
char gForgeTexts_25[24] = {
#include "lobby/0083E0F0.inc"
};
char gForgeTexts_26[13] = {
#include "lobby/0083E108.inc"
};
char gForgeTexts_27[15] = {
#include "lobby/0083E118.inc"
};
char gForgeTexts_28[32] = {
#include "lobby/0083E130.inc"
};
char gForgeTexts_29[32] = {
#include "lobby/0083E150.inc"
};
char gForgeTexts_30[80] = {
#include "lobby/0083E170.inc"
};
char gForgeTexts_31[160] = {
#include "lobby/0083E1C0.inc"
};
char gForgeTexts_32[96] = {
#include "lobby/0083E260.inc"
};
char gForgeTexts_33[144] = {
#include "lobby/0083E2C0.inc"
};
char gForgeTexts_34[96] = {
#include "lobby/0083E350.inc"
};
char gForgeTexts_35[112] = {
#include "lobby/0083E3B0.inc"
};
char gForgeTexts_36[112] = {
#include "lobby/0083E420.inc"
};
char gForgeTexts_37[160] = {
#include "lobby/0083E490.inc"
};
char gForgeTexts_38[96] = {
#include "lobby/0083E530.inc"
};
char gForgeTexts_39[128] = {
#include "lobby/0083E590.inc"
};
char gForgeTexts_40[96] = {
#include "lobby/0083E610.inc"
};
char gForgeTexts_41[48] = {
#include "lobby/0083E670.inc"
};
char gForgeTexts_42[80] = {
#include "lobby/0083E6A0.inc"
};
char gForgeTexts_43[80] = {
#include "lobby/0083E6F0.inc"
};
char gForgeTexts_44[80] = {
#include "lobby/0083E740.inc"
};
char gForgeTexts_45[80] = {
#include "lobby/0083E790.inc"
};
char gForgeTexts_46[80] = {
#include "lobby/0083E7E0.inc"
};
char gForgeTexts_47[80] = {
#include "lobby/0083E830.inc"
};
char gForgeTexts_48[80] = {
#include "lobby/0083E880.inc"
};
char gForgeTexts_49[96] = {
#include "lobby/0083E8D0.inc"
};
char gForgeTexts_50[112] = {
#include "lobby/0083E930.inc"
};
char gForgeTexts_51[48] = {
#include "lobby/0083E9A0.inc"
};
char gForgeTexts_52[48] = {
#include "lobby/0083E9D0.inc"
};
char gForgeTexts_53[48] = {
#include "lobby/0083EA00.inc"
};
char gForgeTexts_54[32] = {
#include "lobby/0083EA30.inc"
};
char gForgeTexts_55[64] = {
#include "lobby/0083EA50.inc"
};
char gForgeTexts_56[32] = {
#include "lobby/0083EA90.inc"
};
char gForgeTexts_57[80] = {
#include "lobby/0083EAB0.inc"
};
char gForgeTexts_58[112] = {
#include "lobby/0083EB00.inc"
};
char gForgeTexts_59[80] = {
#include "lobby/0083EB70.inc"
};
char gForgeTexts_60[64] = {
#include "lobby/0083EBC0.inc"
};
char gForgeTexts_61[48] = {
#include "lobby/0083EC00.inc"
};
char gForgeTexts_62[48] = {
#include "lobby/0083EC30.inc"
};
char gForgeTexts_64[32] = {
#include "lobby/0083EC60.inc"
};
char gForgeTexts_65[24] = {
#include "lobby/0083EC80.inc"
};
char gKitchenTexts_0[12] = {
#include "lobby/0083EC98.inc"
};
char gKitchenTexts_1[12] = {
#include "lobby/0083ECA8.inc"
};
char gKitchenTexts_2[24] = {
#include "lobby/0083ECC0.inc"
};
char gKitchenTexts_3[15] = {
#include "lobby/0083ECD8.inc"
};
char gKitchenTexts_4[32] = {
#include "lobby/0083ECF0.inc"
};
char gKitchenTexts_5[32] = {
#include "lobby/0083ED10.inc"
};
char gKitchenTexts_6[32] = {
#include "lobby/0083ED30.inc"
};
char gKitchenTexts_7[32] = {
#include "lobby/0083ED50.inc"
};
char gKitchenTexts_8[32] = {
#include "lobby/0083ED70.inc"
};
char gKitchenTexts_9[24] = {
#include "lobby/0083ED90.inc"
};
char gKitchenTexts_10[8] = {
#include "lobby/0083EDA8.inc"
};
char gKitchenTexts_11[16] = {
#include "lobby/0083EDB0.inc"
};
char gKitchenTexts_12[32] = {
#include "lobby/0083EDC0.inc"
};
char gKitchenTexts_13[32] = {
#include "lobby/0083EDE0.inc"
};
char gKitchenTexts_14[56] = {
#include "lobby/0083EE00.inc"
};
char gKitchenTexts_15[12] = {
#include "lobby/0083EE38.inc"
};
char gKitchenTexts_16[24] = {
#include "lobby/0083EE50.inc"
};
char gKitchenTexts_17[15] = {
#include "lobby/0083EE68.inc"
};
char gMealNameTexts_0[24] = {
#include "lobby/0083EE80.inc"
};
char gMealNameTexts_1[15] = {
#include "lobby/0083EE98.inc"
};
char gMealNameTexts_2[15] = {
#include "lobby/0083EEA8.inc"
};
char gMealNameTexts_3[24] = {
#include "lobby/0083EEC0.inc"
};
char gMealNameTexts_4[15] = {
#include "lobby/0083EED8.inc"
};
char gMealNameTexts_5[15] = {
#include "lobby/0083EEE8.inc"
};
char gMealNameTexts_6[15] = {
#include "lobby/0083EEF8.inc"
};
char gMealNameTexts_7[15] = {
#include "lobby/0083EF08.inc"
};
char gMealNameTexts_8[13] = {
#include "lobby/0083EF18.inc"
};
char gMealNameTexts_9[13] = {
#include "lobby/0083EF28.inc"
};
char gMealNameTexts_10[12] = {
#include "lobby/0083EF38.inc"
};
char gMealNameTexts_11[15] = {
#include "lobby/0083EF48.inc"
};
char gMealNameTexts_12[13] = {
#include "lobby/0083EF58.inc"
};
char gMealNameTexts_13[15] = {
#include "lobby/0083EF68.inc"
};
char gMealNameTexts_14[15] = {
#include "lobby/0083EF78.inc"
};
char gMealNameTexts_15[32] = {
#include "lobby/0083EF90.inc"
};
char gMealTasteTexts_0_0[32] = {
#include "lobby/0083EFB0.inc"
};
char gMealTasteTexts_0_1[32] = {
#include "lobby/0083EFD0.inc"
};
char gMealTasteTexts_0_2[32] = {
#include "lobby/0083EFF0.inc"
};
char gMealTasteTexts_0_3[32] = {
#include "lobby/0083F010.inc"
};
char gMealTasteTexts_1_0[32] = {
#include "lobby/0083F030.inc"
};
char gMealTasteTexts_1_1[32] = {
#include "lobby/0083F050.inc"
};
char gMealTasteTexts_1_2[32] = {
#include "lobby/0083F070.inc"
};
char gMealTasteTexts_1_3[32] = {
#include "lobby/0083F090.inc"
};
char gMealTasteTexts_2_0[32] = {
#include "lobby/0083F0B0.inc"
};
char gMealTasteTexts_2_1[32] = {
#include "lobby/0083F0D0.inc"
};
char gMealTasteTexts_2_2[32] = {
#include "lobby/0083F0F0.inc"
};
char gMealTasteTexts_2_3[32] = {
#include "lobby/0083F110.inc"
};
char D_0083F130[16] = {
#include "lobby/0083F130.inc"
};
char gMealTasteTexts_4_0[16] = {
#include "lobby/0083F140.inc"
};
char gMealTasteTexts_4_1[32] = {
#include "lobby/0083F150.inc"
};
char gMealTasteTexts_4_2[32] = {
#include "lobby/0083F170.inc"
};
char gMealTasteTexts_4_3[32] = {
#include "lobby/0083F190.inc"
};
char gMealTasteTexts_5_0[32] = {
#include "lobby/0083F1B0.inc"
};
char gMealTasteTexts_5_1[32] = {
#include "lobby/0083F1D0.inc"
};
char gMealTasteTexts_5_2[32] = {
#include "lobby/0083F1F0.inc"
};
char gMealTasteTexts_5_3[32] = {
#include "lobby/0083F210.inc"
};
char gMealTasteTexts_6_0[32] = {
#include "lobby/0083F230.inc"
};
char gMealTasteTexts_6_1[32] = {
#include "lobby/0083F250.inc"
};
char gMealTasteTexts_6_2[32] = {
#include "lobby/0083F270.inc"
};
char gMealTasteTexts_6_3[32] = {
#include "lobby/0083F290.inc"
};
char gBoxActionTexts_0[32] = {
#include "lobby/0083F2B0.inc"
};
char gBoxActionTexts_1[24] = {
#include "lobby/0083F2D0.inc"
};
char gBoxActionTexts_2[15] = {
#include "lobby/0083F2E8.inc"
};
char gBoxActionTexts_3[24] = {
#include "lobby/0083F300.inc"
};
char gBoxActionTexts_4[13] = {
#include "lobby/0083F318.inc"
};
char gBoxControlTexts_0[15] = {
#include "lobby/0083F328.inc"
};
char gBoxControlTexts_1[12] = {
#include "lobby/0083F338.inc"
};
char gMenuShowTexts_0[12] = {
#include "lobby/0083F348.inc"
};
char D_0083F358[12] = {
#include "lobby/0083F358.inc"
};
char gBoxKindTexts_1[8] = {
#include "lobby/0083F368.inc"
};
char gEquipMenuTexts_0[16] = {
#include "lobby/0083F370.inc"
};
char gEquipMenuTexts_1[32] = {
#include "lobby/0083F380.inc"
};
char gEquipMenuTexts_2[32] = {
#include "lobby/0083F3A0.inc"
};
char gEquipMenuTexts_3[24] = {
#include "lobby/0083F3C0.inc"
};
char gEquipMenuTexts_4[15] = {
#include "lobby/0083F3D8.inc"
};
char gEquipSlotTexts_1[8] = {
#include "lobby/0083F3E8.inc"
};
char gEquipSlotTexts_2[8] = {
#include "lobby/0083F3F0.inc"
};
char gEquipSlotTexts_3[8] = {
#include "lobby/0083F3F8.inc"
};
char gEquipSlotTexts_4[8] = {
#include "lobby/0083F400.inc"
};
char gEquipSlotTexts_5[8] = {
#include "lobby/0083F408.inc"
};
char gBoxMenuTexts_0[32] = {
#include "lobby/0083F410.inc"
};
char gBoxMenuTexts_1[32] = {
#include "lobby/0083F430.inc"
};
char gBoxMenuTexts_2[32] = {
#include "lobby/0083F450.inc"
};
char gBoxMenuTexts_3[16] = {
#include "lobby/0083F470.inc"
};
char gBoxMenuTexts_4[32] = {
#include "lobby/0083F480.inc"
};
char gBoxMenuTexts_5[16] = {
#include "lobby/0083F4A0.inc"
};
char gBoxMenuTexts_6[16] = {
#include "lobby/0083F4B0.inc"
};
char gBoxMenuTexts_7[32] = {
#include "lobby/0083F4C0.inc"
};
char gBoxMenuTexts_8[16] = {
#include "lobby/0083F4E0.inc"
};
char gBoxMenuTexts_9[32] = {
#include "lobby/0083F4F0.inc"
};
char gBoxMenuTexts_10[32] = {
#include "lobby/0083F510.inc"
};
char gBoxMenuTexts_11[16] = {
#include "lobby/0083F530.inc"
};
char gBoxMenuTexts_12[16] = {
#include "lobby/0083F540.inc"
};
char gBoxMenuTexts_13[16] = {
#include "lobby/0083F550.inc"
};
char gBoxMenuTexts_14[16] = {
#include "lobby/0083F560.inc"
};
char gBoxMenuTexts_15[32] = {
#include "lobby/0083F570.inc"
};
char gBoxMenuTexts_16[24] = {
#include "lobby/0083F590.inc"
};
char gBoxMenuTexts_17[13] = {
#include "lobby/0083F5A8.inc"
};
char gBoxMenuTexts_18[32] = {
#include "lobby/0083F5C0.inc"
};
char gBoxMenuTexts_19[32] = {
#include "lobby/0083F5E0.inc"
};
char gBoxMenuTexts_20[24] = {
#include "lobby/0083F600.inc"
};
char gBoxMenuTexts_21[8] = {
#include "lobby/0083F618.inc"
};
char gBoxHelpTexts_0[48] = {
#include "lobby/0083F620.inc"
};
char gBoxHelpTexts_1[32] = {
#include "lobby/0083F650.inc"
};
char gBoxHelpTexts_2[48] = {
#include "lobby/0083F670.inc"
};
char gBoxHelpTexts_3[48] = {
#include "lobby/0083F6A0.inc"
};
char gBoxHelpTexts_4[48] = {
#include "lobby/0083F6D0.inc"
};
char gBoxHelpTexts_5[32] = {
#include "lobby/0083F700.inc"
};
char gBoxHelpTexts_6[48] = {
#include "lobby/0083F720.inc"
};
char gBoxHelpTexts_7[32] = {
#include "lobby/0083F750.inc"
};
char gBoxHelpTexts_8[48] = {
#include "lobby/0083F770.inc"
};
char gBoxHelpTexts_10[32] = {
#include "lobby/0083F7A0.inc"
};
char gBoxHelpTexts_11[80] = {
#include "lobby/0083F7C0.inc"
};
char gBoxHelpTexts_12[32] = {
#include "lobby/0083F810.inc"
};
char gBoxHelpTexts_13[48] = {
#include "lobby/0083F830.inc"
};
char gBoxHelpTexts_14[32] = {
#include "lobby/0083F860.inc"
};
char D_0083F880[32] = {
#include "lobby/0083F880.inc"
};
char gBoxHelpTexts_16[24] = {
#include "lobby/0083F8A0.inc"
};
char gBoxHelpTexts_17[13] = {
#include "lobby/0083F8B8.inc"
};
char gBoxHelpTexts_18[32] = {
#include "lobby/0083F8D0.inc"
};
char gBoxHelpTexts_19[32] = {
#include "lobby/0083F8F0.inc"
};
char gBoxHelpTexts_20[48] = {
#include "lobby/0083F910.inc"
};
char gBoxHelpTexts_21[48] = {
#include "lobby/0083F940.inc"
};
char gBoxHelpTexts_22[48] = {
#include "lobby/0083F970.inc"
};
char gBoxHelpTexts_23[32] = {
#include "lobby/0083F9A0.inc"
};
char gBoxHelpTexts_24[32] = {
#include "lobby/0083F9C0.inc"
};
char gBoxHelpTexts_25[32] = {
#include "lobby/0083F9E0.inc"
};
char gBoxHelpTexts_26[48] = {
#include "lobby/0083FA00.inc"
};
char gBoxHelpTexts_27[72] = {
#include "lobby/0083FA30.inc"
};
char gBoxHelpTexts_29[13] = {
#include "lobby/0083FA78.inc"
};
char gBoxHelpTexts_30[32] = {
#include "lobby/0083FA90.inc"
};
char gBoxHelpTexts_31[48] = {
#include "lobby/0083FAB0.inc"
};
char gBoxHelpTexts_32[48] = {
#include "lobby/0083FAE0.inc"
};
char gBoxHelpTexts_33[48] = {
#include "lobby/0083FB10.inc"
};
char gBoxHelpTexts_34[48] = {
#include "lobby/0083FB40.inc"
};
char gBoxHelpTexts_35[32] = {
#include "lobby/0083FB70.inc"
};
char gBoxHelpTexts_36[32] = {
#include "lobby/0083FB90.inc"
};
char gBoxHelpTexts_37[48] = {
#include "lobby/0083FBB0.inc"
};
char gBoxHelpTexts_38[48] = {
#include "lobby/0083FBE0.inc"
};
char gBoxHelpTexts_39[32] = {
#include "lobby/0083FC10.inc"
};
char gBoxHelpTexts_40[64] = {
#include "lobby/0083FC30.inc"
};
char gBoxHelpTexts_41[48] = {
#include "lobby/0083FC70.inc"
};
char gBoxHelpTexts_42[16] = {
#include "lobby/0083FCA0.inc"
};
char gBoxHelpTexts_43[32] = {
#include "lobby/0083FCB0.inc"
};
char gBoxHelpTexts_44[80] = {
#include "lobby/0083FCD0.inc"
};
char gBoxHelpTexts_45[32] = {
#include "lobby/0083FD20.inc"
};
char gBoxHelpTexts_46[32] = {
#include "lobby/0083FD40.inc"
};
char gBoxHelpTexts_47[32] = {
#include "lobby/0083FD60.inc"
};
char gBoxHelpTexts_50[48] = {
#include "lobby/0083FD80.inc"
};
char gBoxHelpTexts_51[32] = {
#include "lobby/0083FDB0.inc"
};
char gBoxHelpTexts_52[64] = {
#include "lobby/0083FDD0.inc"
};
char gBoxHelpTexts_53[64] = {
#include "lobby/0083FE10.inc"
};
char gBoxHelpTexts_54[48] = {
#include "lobby/0083FE50.inc"
};
char gBoxHelpTexts_55[48] = {
#include "lobby/0083FE80.inc"
};
char gBoxHelpTexts_56[32] = {
#include "lobby/0083FEB0.inc"
};
char gBoxHelpTexts_60[64] = {
#include "lobby/0083FED0.inc"
};
char gBoxHelpTexts_61[64] = {
#include "lobby/0083FF10.inc"
};
char gBoxHelpTexts_62[64] = {
#include "lobby/0083FF50.inc"
};
char gBoxHelpTexts_63[48] = {
#include "lobby/0083FF90.inc"
};
char gBoxHelpTexts_64[64] = {
#include "lobby/0083FFC0.inc"
};
char gBoxHelpTexts_65[64] = {
#include "lobby/00840000.inc"
};
char gBoxHelpTexts_66[64] = {
#include "lobby/00840040.inc"
};
char gBoxHelpTexts_67[48] = {
#include "lobby/00840080.inc"
};
char gBoxHelpTexts_68[48] = {
#include "lobby/008400B0.inc"
};
char gBoxHelpTexts_69[64] = {
#include "lobby/008400E0.inc"
};
char gBoxHelpTexts_70[64] = {
#include "lobby/00840120.inc"
};
char gBoxHelpTexts_71[48] = {
#include "lobby/00840160.inc"
};
char gAreaChatTexts_0[24] = {
#include "lobby/00840190.inc"
};
char gAreaChatTexts_1[8] = {
#include "lobby/008401A8.inc"
};
char gAreaChatTexts_2[8] = {
#include "lobby/008401B0.inc"
};
char gAreaChatTexts_3[8] = {
#include "lobby/008401B8.inc"
};
char gQuestCounterTexts_2[16] = {
#include "lobby/008401C0.inc"
};
char gQuestCounterTexts_3[8] = {
#include "lobby/008401D0.inc"
};
char gQuestCounterTexts_4[8] = {
#include "lobby/008401D8.inc"
};
char gQuestCounterTexts_5[24] = {
#include "lobby/008401E0.inc"
};
char gQuestCounterTexts_6[12] = {
#include "lobby/008401F8.inc"
};
char gQuestCounterTexts_7[12] = {
#include "lobby/00840208.inc"
};
char gQuestCounterTexts_8[32] = {
#include "lobby/00840220.inc"
};
char D_00840240[24] = {
#include "lobby/00840240.inc"
};
char gQuestCounterTexts_10[12] = {
#include "lobby/00840258.inc"
};
char gQuestCounterTexts_11[8] = {
#include "lobby/00840268.inc"
};
char gQuestCounterTexts_13[16] = {
#include "lobby/00840270.inc"
};
char gQuestCounterTexts_14[8] = {
#include "lobby/00840280.inc"
};
char gQuestCounterTexts_15[12] = {
#include "lobby/00840288.inc"
};
char gQuestCounterTexts_16[12] = {
#include "lobby/00840298.inc"
};
char gQuestCounterTexts_18[12] = {
#include "lobby/008402A8.inc"
};
char D_008402B8[12] = {
#include "lobby/008402B8.inc"
};
char D_008402C8[13] = {
#include "lobby/008402C8.inc"
};
char gQuestCounterTexts_21[8] = {
#include "lobby/008402D8.inc"
};
char D_008402E0[24] = {
#include "lobby/008402E0.inc"
};
char D_008402F8[12] = {
#include "lobby/008402F8.inc"
};
char gQuestCounterTexts_24[13] = {
#include "lobby/00840308.inc"
};
char D_00840318[13] = {
#include "lobby/00840318.inc"
};
char D_00840328[12] = {
#include "lobby/00840328.inc"
};
char D_00840338[12] = {
#include "lobby/00840338.inc"
};
char gQuestCounterTexts_28[13] = {
#include "lobby/00840348.inc"
};
char gQuestCounterTexts_29[24] = {
#include "lobby/00840360.inc"
};
char gQuestCounterTexts_30[15] = {
#include "lobby/00840378.inc"
};
char gQuestCounterTexts_31[32] = {
#include "lobby/00840390.inc"
};
char gQuestCounterTexts_32[32] = {
#include "lobby/008403B0.inc"
};
char gQuestCounterTexts_33[32] = {
#include "lobby/008403D0.inc"
};
char gQuestCounterTexts_34[32] = {
#include "lobby/008403F0.inc"
};
char gQuestCounterTexts_35[32] = {
#include "lobby/00840410.inc"
};
char gQuestCounterTexts_36[32] = {
#include "lobby/00840430.inc"
};
char gQuestCounterTexts_37[32] = {
#include "lobby/00840450.inc"
};
char gQuestCounterTexts_38[32] = {
#include "lobby/00840470.inc"
};
char gQuestCounterTexts_39[32] = {
#include "lobby/00840490.inc"
};
char gQuestCounterTexts_40[32] = {
#include "lobby/008404B0.inc"
};
char gQuestCounterTexts_41[32] = {
#include "lobby/008404D0.inc"
};
char gQuestCounterTexts_42[32] = {
#include "lobby/008404F0.inc"
};
char gQuestCounterTexts_43[24] = {
#include "lobby/00840510.inc"
};
char gQuestCounterTexts_44[12] = {
#include "lobby/00840528.inc"
};
char gQuestCounterTexts_45[13] = {
#include "lobby/00840538.inc"
};
char gQuestCounterTexts_46[24] = {
#include "lobby/00840550.inc"
};
char gQuestCounterTexts_47[12] = {
#include "lobby/00840568.inc"
};
char gQuestCounterTexts_48[12] = {
#include "lobby/00840578.inc"
};
char gQuestCounterTexts_49[12] = {
#include "lobby/00840588.inc"
};
char gQuestCounterTexts_50[48] = {
#include "lobby/008405A0.inc"
};
char gQuestCounterTexts_51[32] = {
#include "lobby/008405D0.inc"
};
char gQuestCounterTexts_52[48] = {
#include "lobby/008405F0.inc"
};
char gQuestCounterTexts_53[24] = {
#include "lobby/00840620.inc"
};
char D_00840638[15] = {
#include "lobby/00840638.inc"
};
char gInfoIndexTexts_2[13] = {
#include "lobby/00840648.inc"
};
char gInfoIndexTexts_3[15] = {
#include "lobby/00840658.inc"
};
char gInfoIndexTexts_4[15] = {
#include "lobby/00840668.inc"
};
char gQuestSheetTexts_1[13] = {
#include "lobby/00840678.inc"
};
char gQuestSheetTexts_2[13] = {
#include "lobby/00840688.inc"
};
char gQuestSheetTexts_3[32] = {
#include "lobby/008406A0.inc"
};
char gQuestSheetTexts_4[24] = {
#include "lobby/008406C0.inc"
};
char gQuestSheetTexts_5[15] = {
#include "lobby/008406D8.inc"
};
char gQuestSheetTexts_6[13] = {
#include "lobby/008406E8.inc"
};
char gQuestSheetTexts_7[15] = {
#include "lobby/008406F8.inc"
};
char gQuestSheetTexts_8[13] = {
#include "lobby/00840708.inc"
};
char gQuestSheetTexts_9[15] = {
#include "lobby/00840718.inc"
};
char gQuestSheetTexts_10[13] = {
#include "lobby/00840728.inc"
};
char gQuestSheetTexts_11[32] = {
#include "lobby/00840740.inc"
};
char gQuestSheetTexts_12[24] = {
#include "lobby/00840760.inc"
};
char gQuestSheetTexts_14[12] = {
#include "lobby/00840778.inc"
};
char gQuestSheetTexts_16[12] = {
#include "lobby/00840788.inc"
};
char gQuestSheetTexts_17[12] = {
#include "lobby/00840798.inc"
};
char gQuestSheetTexts_21[12] = {
#include "lobby/008407A8.inc"
};
char gQuestSheetTexts_22[24] = {
#include "lobby/008407C0.inc"
};
char gQuestSheetTexts_23[12] = {
#include "lobby/008407D8.inc"
};
char gQuestSheetTexts_24[13] = {
#include "lobby/008407E8.inc"
};
char gQuestSheetTexts_25[15] = {
#include "lobby/008407F8.inc"
};
char gQuestSheetTexts_27[15] = {
#include "lobby/00840808.inc"
};
char gQuestSheetTexts_28[12] = {
#include "lobby/00840818.inc"
};
char D_00840828[12] = {
#include "lobby/00840828.inc"
};
char gQuestSheetTexts_31[13] = {
#include "lobby/00840838.inc"
};
char gQuestSheetTexts_34[32] = {
#include "lobby/00840850.inc"
};
char gQuestSheetTexts_35[24] = {
#include "lobby/00840870.inc"
};
char gOfficialEventTexts_0[13] = {
#include "lobby/00840888.inc"
};
char gOfficialEventTexts_1[13] = {
#include "lobby/00840898.inc"
};
char gOfficialEventTexts_2[12] = {
#include "lobby/008408A8.inc"
};
char gOfficialEventTexts_3[12] = {
#include "lobby/008408B8.inc"
};
char D_008408C8[8] = {
#include "lobby/008408C8.inc"
};
char D_008408D0[16] = {
#include "lobby/008408D0.inc"
};
char D_008408E0[16] = {
#include "lobby/008408E0.inc"
};
char D_008408F0[16] = {
#include "lobby/008408F0.inc"
};
char D_00840900[16] = {
#include "lobby/00840900.inc"
};
char D_00840910[16] = {
#include "lobby/00840910.inc"
};
char D_00840920[16] = {
#include "lobby/00840920.inc"
};
char D_00840930[16] = {
#include "lobby/00840930.inc"
};
char D_00840940[16] = {
#include "lobby/00840940.inc"
};
char gGameMenuHelpTexts_0[64] = {
#include "lobby/00840950.inc"
};
char gGameMenuHelpTexts_1[32] = {
#include "lobby/00840990.inc"
};
char gGameMenuHelpTexts_2[48] = {
#include "lobby/008409B0.inc"
};
char gGameMenuHelpTexts_3[48] = {
#include "lobby/008409E0.inc"
};
char gGameMenuHelpTexts_4[32] = {
#include "lobby/00840A10.inc"
};
char gGameMenuHelpTexts_5[64] = {
#include "lobby/00840A30.inc"
};
char gGameMenuHelpTexts_6[64] = {
#include "lobby/00840A70.inc"
};
char gGameMenuHelpTexts_7[32] = {
#include "lobby/00840AB0.inc"
};
char gGameMenuHelpTexts_8[32] = {
#include "lobby/00840AD0.inc"
};
char gGameMenuHelpTexts_9[32] = {
#include "lobby/00840AF0.inc"
};
char D_00840B10[32] = {
#include "lobby/00840B10.inc"
};
char D_00840B30[32] = {
#include "lobby/00840B30.inc"
};
char gGameMenuHelpTexts_16[32] = {
#include "lobby/00840B50.inc"
};
char gGameMenuHelpTexts_17[32] = {
#include "lobby/00840B70.inc"
};
char gGameMenuHelpTexts_18[32] = {
#include "lobby/00840B90.inc"
};
char gGameMenuHelpTexts_19[64] = {
#include "lobby/00840BB0.inc"
};
char D_00840BF0[80] = {
#include "lobby/00840BF0.inc"
};
char gGameMenuHelpTexts_21[64] = {
#include "lobby/00840C40.inc"
};
char gGameMenuHelpTexts_22[64] = {
#include "lobby/00840C80.inc"
};
char D_00840CC0[48] = {
#include "lobby/00840CC0.inc"
};
char gGameMenuHelpTexts_24[48] = {
#include "lobby/00840CF0.inc"
};
char gGameMenuHelpTexts_25[48] = {
#include "lobby/00840D20.inc"
};
char gGameMenuHelpTexts_28[96] = {
#include "lobby/00840D50.inc"
};
char gGameMenuHelpTexts_29[96] = {
#include "lobby/00840DB0.inc"
};
char gGameMenuHelpTexts_30[48] = {
#include "lobby/00840E10.inc"
};
char gGameMenuHelpTexts_31[48] = {
#include "lobby/00840E40.inc"
};
char gGameMenuHelpTexts_32[48] = {
#include "lobby/00840E70.inc"
};
char gGameMenuHelpTexts_33[32] = {
#include "lobby/00840EA0.inc"
};
char gGameMenuHelpTexts_34[64] = {
#include "lobby/00840EC0.inc"
};
char gGameMenuHelpTexts_35[96] = {
#include "lobby/00840F00.inc"
};
char gGameMenuHelpTexts_36[48] = {
#include "lobby/00840F60.inc"
};
char gGameMenuHelpTexts_37[64] = {
#include "lobby/00840F90.inc"
};
char gGameMenuHelpTexts_38[64] = {
#include "lobby/00840FD0.inc"
};
char gGameMenuHelpTexts_39[56] = {
#include "lobby/00841010.inc"
};
char gSearchMenuTexts_1[13] = {
#include "lobby/00841048.inc"
};
char gSearchMenuTexts_2[12] = {
#include "lobby/00841058.inc"
};
char gSearchMenuTexts_3[12] = {
#include "lobby/00841068.inc"
};
char gMailboxMenuTexts_0[15] = {
#include "lobby/00841078.inc"
};
char gMailboxMenuTexts_1[12] = {
#include "lobby/00841088.inc"
};
char gMailboxMenuTexts_2[12] = {
#include "lobby/00841098.inc"
};
char D_008410B0[24] = {
#include "lobby/008410B0.inc"
};
char gQuitMenuTexts_1[15] = {
#include "lobby/008410C8.inc"
};
char gQuitMenuDimTexts_1[32] = {
#include "lobby/008410E0.inc"
};
char gQuitSaveTexts_0[48] = {
#include "lobby/00841100.inc"
};
char gQuitSaveTexts_1[48] = {
#include "lobby/00841130.inc"
};
char gQuitConfirmTexts_0[80] = {
#include "lobby/00841160.inc"
};
char gQuitConfirmTexts_1[144] = {
#include "lobby/008411B0.inc"
};
char gQuitConfirmTexts_2[80] = {
#include "lobby/00841240.inc"
};
char gQuitConfirmTexts_3[104] = {
#include "lobby/00841290.inc"
};
char gChatPageTexts_0[13] = {
#include "lobby/008412F8.inc"
};
char gChatPageTexts_1[32] = {
#include "lobby/00841310.inc"
};
char gChatPageTexts_2[16] = {
#include "lobby/00841330.inc"
};
char gChatPageTexts_3[16] = {
#include "lobby/00841340.inc"
};
char D_00841350[32] = {
#include "lobby/00841350.inc"
};
char gChatPageTexts_5[32] = {
#include "lobby/00841370.inc"
};
char gChatPageTexts_6[32] = {
#include "lobby/00841390.inc"
};
char gChatPageTexts_7[32] = {
#include "lobby/008413B0.inc"
};
char gChatPageTexts_8[32] = {
#include "lobby/008413D0.inc"
};
char gChatPageTexts_9[32] = {
#include "lobby/008413F0.inc"
};
char D_00841410[16] = {
#include "lobby/00841410.inc"
};
char gChatMenuTexts_3[16] = {
#include "lobby/00841420.inc"
};
char gChatMenuTexts_4[32] = {
#include "lobby/00841430.inc"
};
char gChatMenuTexts_5[32] = {
#include "lobby/00841450.inc"
};
char gChatStateTexts_0[32] = {
#include "lobby/00841470.inc"
};
char gChatStateTexts_1[24] = {
#include "lobby/00841490.inc"
};
char gChatUserActTexts_1[12] = {
#include "lobby/008414A8.inc"
};
char gChatUserActTexts_2[13] = {
#include "lobby/008414B8.inc"
};
char gChatUserActTexts_4[13] = {
#include "lobby/008414C8.inc"
};
char D_008414D8[12] = {
#include "lobby/008414D8.inc"
};
char gChatUserActTexts_7[24] = {
#include "lobby/008414F0.inc"
};
char gPlaceInfoTexts_0[8] = {
#include "lobby/00841508.inc"
};
char D_00841510[16] = {
#include "lobby/00841510.inc"
};
char gPlaceInfoTexts_6[16] = {
#include "lobby/00841520.inc"
};
char gPlaceInfoTexts_7[16] = {
#include "lobby/00841530.inc"
};
char gPlaceInfoTexts_8[8] = {
#include "lobby/00841540.inc"
};
char gPlaceInfoTexts_9[8] = {
#include "lobby/00841548.inc"
};
char gPlaceInfoTexts_10[8] = {
#include "lobby/00841550.inc"
};
char gPlaceInfoTexts_11[8] = {
#include "lobby/00841558.inc"
};
char gTavernLevelTexts_0[32] = {
#include "lobby/00841560.inc"
};
char gTavernLevelTexts_1[32] = {
#include "lobby/00841580.inc"
};
char gTavernLevelTexts_2[32] = {
#include "lobby/008415A0.inc"
};
char gPortLevelTexts_0[32] = {
#include "lobby/008415C0.inc"
};
char gPortLevelTexts_1[32] = {
#include "lobby/008415E0.inc"
};
char gPortLevelTexts_2[32] = {
#include "lobby/00841600.inc"
};
char gSmithyLevelTexts_0[32] = {
#include "lobby/00841620.inc"
};
char gSmithyLevelTexts_1[32] = {
#include "lobby/00841640.inc"
};
char gSmithyLevelTexts_2[32] = {
#include "lobby/00841660.inc"
};
char gMineLevelTexts_0[32] = {
#include "lobby/00841680.inc"
};
char gMineLevelTexts_1[32] = {
#include "lobby/008416A0.inc"
};
char gMineLevelTexts_2[24] = {
#include "lobby/008416C0.inc"
};
char gOnlineMarkNames_0[14] = {
#include "lobby/008416D8.inc"
};
char gOnlineMarkNames_7[32] = {
#include "lobby/008416F0.inc"
};
char gOnlineMarkNames_5[24] = {
#include "lobby/00841710.inc"
};
char gOnlineMarkNames_6[14] = {
#include "lobby/00841728.inc"
};
char gOnlineMarkNames_8[12] = {
#include "lobby/00841738.inc"
};
char gOnlineMarkNames_1[32] = {
#include "lobby/00841750.inc"
};
char gOnlineMarkNames_2[32] = {
#include "lobby/00841770.inc"
};
char gOnlineMarkNames_3[32] = {
#include "lobby/00841790.inc"
};
char gOnlineMarkNames_4[24] = {
#include "lobby/008417B0.inc"
};
char gMemberStatusTexts_0[12] = {
#include "lobby/008417C8.inc"
};
char gMemberStatusTexts_1[32] = {
#include "lobby/008417E0.inc"
};
char gMemberStatusTexts_2[32] = {
#include "lobby/00841800.inc"
};
char gMemberStatusTexts_3[24] = {
#include "lobby/00841820.inc"
};
char gMemberStatusTexts_4[14] = {
#include "lobby/00841838.inc"
};
char gMemberStatusTexts_5[16] = {
#include "lobby/00841850.inc"
};
char gMemberStatusTexts_6[16] = {
#include "lobby/00841860.inc"
};
char gMemberStatusTexts_7[24] = {
#include "lobby/00841870.inc"
};
char gMemberStatusTexts_8[8] = {
#include "lobby/00841888.inc"
};
char D_00841890[16] = {
#include "lobby/00841890.inc"
};
char D_008418A0[8] = {
#include "lobby/008418A0.inc"
};
char D_008418A8[8] = {
#include "lobby/008418A8.inc"
};
char gShopMenuTexts_3[8] = {
#include "lobby/008418B0.inc"
};
char gCombineShopTexts_0[13] = {
#include "lobby/008418B8.inc"
};
char gCombineShopTexts_1[13] = {
#include "lobby/008418C8.inc"
};
char gCombineShopTexts_2[13] = {
#include "lobby/008418D8.inc"
};
char gCombineShopTexts_3[12] = {
#include "lobby/008418E8.inc"
};
char gCombineShopTexts_4[12] = {
#include "lobby/008418F8.inc"
};
char gCombineShopTexts_5[12] = {
#include "lobby/00841908.inc"
};
char gCombineShopTexts_6[8] = {
#include "lobby/00841918.inc"
};
char D_00841920[16] = {
#include "lobby/00841920.inc"
};
char gCombineShopTexts_8[16] = {
#include "lobby/00841930.inc"
};
char gShopPromptTexts_0[32] = {
#include "lobby/00841940.inc"
};
char gShopPromptTexts_1[32] = {
#include "lobby/00841960.inc"
};
char gShopPromptTexts_2[32] = {
#include "lobby/00841980.inc"
};
char gShopPromptTexts_5[48] = {
#include "lobby/008419A0.inc"
};
char gShopPromptTexts_6[48] = {
#include "lobby/008419D0.inc"
};
char gShopPromptTexts_7[112] = {
#include "lobby/00841A00.inc"
};
char gShopPromptTexts_8[48] = {
#include "lobby/00841A70.inc"
};
char gShopPromptTexts_9[48] = {
#include "lobby/00841AA0.inc"
};
char gShopPromptTexts_10[112] = {
#include "lobby/00841AD0.inc"
};
char gShopPromptTexts_11[48] = {
#include "lobby/00841B40.inc"
};
char gShopPromptTexts_12[48] = {
#include "lobby/00841B70.inc"
};
char gShopPromptTexts_13[112] = {
#include "lobby/00841BA0.inc"
};
char gShopPromptTexts_14[96] = {
#include "lobby/00841C10.inc"
};
char gShopPromptTexts_15[112] = {
#include "lobby/00841C70.inc"
};
char gSongHelpTexts_0[64] = {
#include "lobby/00841CE0.inc"
};
char gSongHelpTexts_1[88] = {
#include "lobby/00841D20.inc"
};
char gMemberCardPanels_0_0[12] = {
#include "lobby/00841D78.inc"
};
char gMemberCardPanels_0_1[12] = {
#include "lobby/00841D88.inc"
};
char gMemberCardPanels_0_2[12] = {
#include "lobby/00841D98.inc"
};
char gMemberCardPanels_0_3[12] = {
#include "lobby/00841DA8.inc"
};
char gMemberCardPanels_0_5[12] = {
#include "lobby/00841DB8.inc"
};
char gMemberCardPanels_0_6[24] = {
#include "lobby/00841DD0.inc"
};
char gMemberCardPanels_0_7[12] = {
#include "lobby/00841DE8.inc"
};
char gMemberCardPanels_0_8[24] = {
#include "lobby/00841E00.inc"
};
char gStatusTitleTexts_0[13] = {
#include "lobby/00841E18.inc"
};
char gStatusTitleTexts_1[48] = {
#include "lobby/00841E30.inc"
};
char gFriendRegTexts_0[32] = {
#include "lobby/00841E60.inc"
};
char gFriendRegTexts_1[32] = {
#include "lobby/00841E80.inc"
};
char gBedMenuTexts_0[8] = {
#include "lobby/00841EA0.inc"
};
char gBedMenuTexts_1[8] = {
#include "lobby/00841EA8.inc"
};
char gBedMenuTexts_2[16] = {
#include "lobby/00841EB0.inc"
};
char gBedMenuTexts_3[64] = {
#include "lobby/00841EC0.inc"
};
char gBedMenuTexts_4[96] = {
#include "lobby/00841F00.inc"
};
char gBedMenuTexts_5[80] = {
#include "lobby/00841F60.inc"
};
char gBedMenuTexts_6[48] = {
#include "lobby/00841FB0.inc"
};
char gMonsterClassTexts_0[16] = {
#include "lobby/00841FE0.inc"
};
char D_00841FF0[8] = {
#include "lobby/00841FF0.inc"
};
char D_00841FF8[8] = {
#include "lobby/00841FF8.inc"
};
char D_00842000[8] = {
#include "lobby/00842000.inc"
};
char D_00842008[8] = {
#include "lobby/00842008.inc"
};
char D_00842010[8] = {
#include "lobby/00842010.inc"
};
char D_00842018[8] = {
#include "lobby/00842018.inc"
};
char gGalleryCourseTexts_0[16] = {
#include "lobby/00842020.inc"
};
char gGalleryCourseTexts_1[16] = {
#include "lobby/00842030.inc"
};
char gGalleryCourseTexts_2[16] = {
#include "lobby/00842040.inc"
};
char gGalleryCourseTexts_3[16] = {
#include "lobby/00842050.inc"
};
char gGalleryCourseTexts_4[16] = {
#include "lobby/00842060.inc"
};
char gGalleryCourseTexts_5[16] = {
#include "lobby/00842070.inc"
};
char gGalleryCourseTexts_6[16] = {
#include "lobby/00842080.inc"
};
char gGalleryCourseTexts_7[16] = {
#include "lobby/00842090.inc"
};
char gGalleryCourseTexts_8[8] = {
#include "lobby/008420A0.inc"
};
char gGalleryCourseTexts_9[13] = {
#include "lobby/008420A8.inc"
};
char gEmptyListTexts_0[32] = {
#include "lobby/008420C0.inc"
};
char gEmptyListTexts_1[32] = {
#include "lobby/008420E0.inc"
};
char gEmptyListTexts_2[32] = {
#include "lobby/00842100.inc"
};
char gEmptyListTexts_3[32] = {
#include "lobby/00842120.inc"
};
char gEmptyListTexts_4[32] = {
#include "lobby/00842140.inc"
};
char gEmptyListTexts_5[16] = {
#include "lobby/00842160.inc"
};
char gEmptyListTexts_7[32] = {
#include "lobby/00842170.inc"
};
char gLobbyNoticeTexts_0[32] = {
#include "lobby/00842190.inc"
};
char gLobbyNoticeTexts_1[32] = {
#include "lobby/008421B0.inc"
};
char gLobbyNoticeTexts_2[32] = {
#include "lobby/008421D0.inc"
};
char gLobbyNoticeTexts_3[32] = {
#include "lobby/008421F0.inc"
};
char gLobbyNoticeTexts_4[32] = {
#include "lobby/00842210.inc"
};
char gLobbyNoticeTexts_5[48] = {
#include "lobby/00842230.inc"
};
char gLobbyNoticeTexts_6[48] = {
#include "lobby/00842260.inc"
};
char gLobbyNoticeTexts_7[48] = {
#include "lobby/00842290.inc"
};
char gLobbyNoticeTexts_8[48] = {
#include "lobby/008422C0.inc"
};
char gLobbyNoticeTexts_9[48] = {
#include "lobby/008422F0.inc"
};
char gLobbyNoticeTexts_10[32] = {
#include "lobby/00842320.inc"
};
char gLobbyNoticeTexts_11[32] = {
#include "lobby/00842340.inc"
};
char gLobbyNoticeTexts_12[32] = {
#include "lobby/00842360.inc"
};
char gLobbyNoticeTexts_13[32] = {
#include "lobby/00842380.inc"
};
char gLobbyNoticeTexts_14[32] = {
#include "lobby/008423A0.inc"
};
char gLobbyNoticeTexts_15[32] = {
#include "lobby/008423C0.inc"
};
char gLobbyNoticeTexts_16[32] = {
#include "lobby/008423E0.inc"
};
char gLobbyNoticeTexts_17[32] = {
#include "lobby/00842400.inc"
};
char gLobbyNoticeTexts_18[32] = {
#include "lobby/00842420.inc"
};
char gLobbyNoticeTexts_19[48] = {
#include "lobby/00842440.inc"
};
char gLobbyNoticeTexts_20[32] = {
#include "lobby/00842470.inc"
};
char gLobbyNoticeTexts_21[32] = {
#include "lobby/00842490.inc"
};
char gLobbyNoticeTexts_22[32] = {
#include "lobby/008424B0.inc"
};
char gLobbyNoticeTexts_23[16] = {
#include "lobby/008424D0.inc"
};
char gLobbyNoticeTexts_24[32] = {
#include "lobby/008424E0.inc"
};
char gLobbyNoticeTexts_25[48] = {
#include "lobby/00842500.inc"
};
char gLobbyNoticeTexts_26[48] = {
#include "lobby/00842530.inc"
};
char gLobbyNoticeTexts_27[32] = {
#include "lobby/00842560.inc"
};
char gLobbyNoticeTexts_28[48] = {
#include "lobby/00842580.inc"
};
char gLobbyNoticeTexts_29[40] = {
#include "lobby/008425B0.inc"
};
char gLobbyNoticeTexts_30[13] = {
#include "lobby/008425D8.inc"
};
char gLobbyNoticeTexts_31[13] = {
#include "lobby/008425E8.inc"
};
char gLobbyNoticeTexts_32[32] = {
#include "lobby/00842600.inc"
};
char gLobbyNoticeTexts_33[64] = {
#include "lobby/00842620.inc"
};
char gLobbyNoticeTexts_34[32] = {
#include "lobby/00842660.inc"
};
char gLobbyNoticeTexts_35[32] = {
#include "lobby/00842680.inc"
};
char gLobbyNoticeTexts_36[48] = {
#include "lobby/008426A0.inc"
};
char gLobbyNoticeTexts_37[48] = {
#include "lobby/008426D0.inc"
};
char gLobbyNoticeTexts_38[32] = {
#include "lobby/00842700.inc"
};
char gLobbyNoticeTexts_39[48] = {
#include "lobby/00842720.inc"
};
char gLobbyNoticeTexts_40[32] = {
#include "lobby/00842750.inc"
};
char gLobbyEventTexts_0[32] = {
#include "lobby/00842770.inc"
};
char gLobbyEventTexts_1[32] = {
#include "lobby/00842790.inc"
};
char gLobbyEventTexts_2[32] = {
#include "lobby/008427B0.inc"
};
char gLobbyEventTexts_3[24] = {
#include "lobby/008427D0.inc"
};
char gLobbyEventTexts_4[12] = {
#include "lobby/008427E8.inc"
};
char gLobbyEventTexts_5[13] = {
#include "lobby/008427F8.inc"
};
char gLobbyEventTexts_6[48] = {
#include "lobby/00842810.inc"
};
char gLobbyEventTexts_7[32] = {
#include "lobby/00842840.inc"
};
char gLobbyEventTexts_8[32] = {
#include "lobby/00842860.inc"
};
char gLobbyEventTexts_9[32] = {
#include "lobby/00842880.inc"
};
char gTextFillNameFmt[16] = {
#include "lobby/008428A0.inc"
};
char gLobbyEventTexts_11[24] = {
#include "lobby/008428B0.inc"
};
char gSpotActionTexts_0[13] = {
#include "lobby/008428C8.inc"
};
char D_008428D8[12] = {
#include "lobby/008428D8.inc"
};
char gSpotActionTexts_2[12] = {
#include "lobby/008428E8.inc"
};
char gSpotActionTexts_3[32] = {
#include "lobby/00842900.inc"
};
char gSpotActionTexts_4[16] = {
#include "lobby/00842920.inc"
};
char gSpotActionTexts_5[16] = {
#include "lobby/00842930.inc"
};
char gSpotActionTexts_6[24] = {
#include "lobby/00842940.inc"
};
char gSpotActionTexts_7[12] = {
#include "lobby/00842958.inc"
};
char gSpotActionTexts_8[12] = {
#include "lobby/00842968.inc"
};
char gSpotActionTexts_9[15] = {
#include "lobby/00842978.inc"
};
char gSpotActionTexts_11[8] = {
#include "lobby/00842988.inc"
};
char gSpotActionTexts_12[16] = {
#include "lobby/00842990.inc"
};
char gSpotActionTexts_13[24] = {
#include "lobby/008429A0.inc"
};
char gSpotActionTexts_14[12] = {
#include "lobby/008429B8.inc"
};
char gSpotActionTexts_15[12] = {
#include "lobby/008429C8.inc"
};
char gSpotActionTexts_17[15] = {
#include "lobby/008429D8.inc"
};
char gSpotActionTexts_18[12] = {
#include "lobby/008429E8.inc"
};
char gSpotActionTexts_19[15] = {
#include "lobby/008429F8.inc"
};
char gSpotActionTexts_20[24] = {
#include "lobby/00842A10.inc"
};
char gSpotActionTexts_21[12] = {
#include "lobby/00842A28.inc"
};
char gSpotActionTexts_22[8] = {
#include "lobby/00842A38.inc"
};
char gSpotActionTexts_23[24] = {
#include "lobby/00842A40.inc"
};
char gSpotActionTexts_24[12] = {
#include "lobby/00842A58.inc"
};
char gSpotActionTexts_25[24] = {
#include "lobby/00842A70.inc"
};
char gSpotActionTexts_26[12] = {
#include "lobby/00842A88.inc"
};
char gNameTagActionTexts_0[12] = {
#include "lobby/00842A98.inc"
};
char gNameTagActionTexts_1[32] = {
#include "lobby/00842AB0.inc"
};
char gNameTagActionTexts_2[32] = {
#include "lobby/00842AD0.inc"
};
char gLobbyTitleTexts_0[8] = {
#include "lobby/00842AF0.inc"
};
char gLobbyTitleTexts_1[12] = {
#include "lobby/00842AF8.inc"
};
char gLobbyTitleTexts_2[12] = {
#include "lobby/00842B08.inc"
};
char gLobbyTitleTexts_3[12] = {
#include "lobby/00842B18.inc"
};
char gLobbyTitleTexts_4[12] = {
#include "lobby/00842B28.inc"
};
char gLobbyHelpTexts_0[32] = {
#include "lobby/00842B40.inc"
};
char gLobbyHelpTexts_1[160] = {
#include "lobby/00842B60.inc"
};
char gLobbyHelpTexts_2[48] = {
#include "lobby/00842C00.inc"
};
char gLobbyHelpTexts_3[48] = {
#include "lobby/00842C30.inc"
};
char gLobbyHelpTexts_4[32] = {
#include "lobby/00842C60.inc"
};
char gLobbyHelpTexts_5[128] = {
#include "lobby/00842C80.inc"
};
char gLobbyHelpTexts_6[48] = {
#include "lobby/00842D00.inc"
};
char gLobbyHelpTexts_7[32] = {
#include "lobby/00842D30.inc"
};
char gLobbyHelpTexts_8[64] = {
#include "lobby/00842D50.inc"
};
char gLobbyHelpTexts_9[48] = {
#include "lobby/00842D90.inc"
};
char gLobbyHelpTexts_10[48] = {
#include "lobby/00842DC0.inc"
};
char gLobbyHelpTexts_11[32] = {
#include "lobby/00842DF0.inc"
};
char gLobbyHelpTexts_13[48] = {
#include "lobby/00842E10.inc"
};
char gLobbyHelpTexts_14[128] = {
#include "lobby/00842E40.inc"
};
char gLobbyHelpTexts_15[64] = {
#include "lobby/00842EC0.inc"
};
char gLobbyHelpTexts_16[48] = {
#include "lobby/00842F00.inc"
};
char gLobbyHelpTexts_17[48] = {
#include "lobby/00842F30.inc"
};
char gLobbyHelpTexts_18[96] = {
#include "lobby/00842F60.inc"
};
char gLobbyHelpTexts_19[32] = {
#include "lobby/00842FC0.inc"
};
char gLobbyHelpTexts_20[176] = {
#include "lobby/00842FE0.inc"
};
char gLobbyHelpTexts_21[160] = {
#include "lobby/00843090.inc"
};
char gLobbyHelpTexts_22[144] = {
#include "lobby/00843130.inc"
};
char gLobbyHelpTexts_23[160] = {
#include "lobby/008431C0.inc"
};
char gLobbyHelpTexts_24[48] = {
#include "lobby/00843260.inc"
};
char gLobbyHelpTexts_25[64] = {
#include "lobby/00843290.inc"
};
char gLobbyHelpTexts_26[32] = {
#include "lobby/008432D0.inc"
};
char gLobbyHelpTexts_27[80] = {
#include "lobby/008432F0.inc"
};
char gLobbyHelpTexts_28[160] = {
#include "lobby/00843340.inc"
};
char gLobbyHelpTexts_29[160] = {
#include "lobby/008433E0.inc"
};
char gLobbyHelpTexts_31[48] = {
#include "lobby/00843480.inc"
};
char gLobbyHelpTexts_32[64] = {
#include "lobby/008434B0.inc"
};
char gLobbyHelpTexts_33[48] = {
#include "lobby/008434F0.inc"
};
char gLobbyHelpTexts_34[144] = {
#include "lobby/00843520.inc"
};
char gLobbyHelpTexts_35[176] = {
#include "lobby/008435B0.inc"
};
char gLobbyHelpTexts_36[144] = {
#include "lobby/00843660.inc"
};
char gLobbyHelpTexts_37[64] = {
#include "lobby/008436F0.inc"
};
char gLobbyHelpTexts_38[64] = {
#include "lobby/00843730.inc"
};
char gLobbyHelpTexts_39[64] = {
#include "lobby/00843770.inc"
};
char gLobbyHelpTexts_40[80] = {
#include "lobby/008437B0.inc"
};
char gLobbyHelpTexts_41[64] = {
#include "lobby/00843800.inc"
};
char gLobbyHelpTexts_43[64] = {
#include "lobby/00843840.inc"
};
char gLobbyHelpTexts_44[64] = {
#include "lobby/00843880.inc"
};
char gLobbyHelpTexts_46[48] = {
#include "lobby/008438C0.inc"
};
char gLobbyHelpTexts_47[144] = {
#include "lobby/008438F0.inc"
};
char gLobbyHelpTexts_51[8] = {
#include "lobby/00843980.inc"
};
char gLobbyHelpTexts_53[12] = {
#include "lobby/00843988.inc"
};
char D_00843998[12] = {
#include "lobby/00843998.inc"
};
char gLobbyHelpTexts_55[8] = {
#include "lobby/008439A8.inc"
};
char gLobbyHelpTexts_60[8] = {
#include "lobby/008439B0.inc"
};
char gLobbyHelpTexts_61[8] = {
#include "lobby/008439B8.inc"
};
char gLobbyHelpTexts_62[8] = {
#include "lobby/008439C0.inc"
};
char gLobbyHelpTexts_63[12] = {
#include "lobby/008439C8.inc"
};
char gLobbyHelpTexts_64[8] = {
#include "lobby/008439D8.inc"
};
char gLobbyHelpTexts_65[8] = {
#include "lobby/008439E0.inc"
};
char gLobbyHelpTexts_66[8] = {
#include "lobby/008439E8.inc"
};
char gLobbyHelpTexts_67[8] = {
#include "lobby/008439F0.inc"
};
char gLobbyLabelTexts_0[8] = {
#include "lobby/008439F8.inc"
};
char gLobbyLabelTexts_1[8] = {
#include "lobby/00843A00.inc"
};
char gLobbyLabelTexts_2[12] = {
#include "lobby/00843A08.inc"
};
char gLobbyLabelTexts_4[8] = {
#include "lobby/00843A18.inc"
};
char gLobbyLabelTexts_8[8] = {
#include "lobby/00843A20.inc"
};
char gLobbyLabelTexts_9[13] = {
#include "lobby/00843A28.inc"
};
char gLobbyLabelTexts_10[32] = {
#include "lobby/00843A40.inc"
};
char gLobbyLabelTexts_11[24] = {
#include "lobby/00843A60.inc"
};
char gLobbyLabelTexts_12[12] = {
#include "lobby/00843A78.inc"
};
char D_00843A88[12] = {
#include "lobby/00843A88.inc"
};
char gLobbyLabelTexts_14[24] = {
#include "lobby/00843AA0.inc"
};
char gLobbyLabelTexts_15[12] = {
#include "lobby/00843AB8.inc"
};
char gLobbyLabelTexts_16[13] = {
#include "lobby/00843AC8.inc"
};
char gLobbyLabelTexts_17[24] = {
#include "lobby/00843AE0.inc"
};
char gLobbyLabelTexts_18[8] = {
#include "lobby/00843AF8.inc"
};
char gLobbyLabelTexts_20[16] = {
#include "lobby/00843B00.inc"
};
char gLobbyLabelTexts_21[16] = {
#include "lobby/00843B10.inc"
};
char gLobbyLabelTexts_26[16] = {
#include "lobby/00843B20.inc"
};
char gLobbyLabelTexts_27[16] = {
#include "lobby/00843B30.inc"
};
char gLobbyLabelTexts_28[32] = {
#include "lobby/00843B40.inc"
};
char gLobbyLabelTexts_29[32] = {
#include "lobby/00843B60.inc"
};
char gLobbyLabelTexts_30[32] = {
#include "lobby/00843B80.inc"
};
char gLobbyLabelTexts_31[32] = {
#include "lobby/00843BA0.inc"
};
char gLobbyLabelTexts_32[32] = {
#include "lobby/00843BC0.inc"
};
char gLobbyLabelTexts_33[32] = {
#include "lobby/00843BE0.inc"
};
char gLobbyLabelTexts_34[32] = {
#include "lobby/00843C00.inc"
};
char gLobbyLabelTexts_35[32] = {
#include "lobby/00843C20.inc"
};
char gLobbyLabelTexts_37[8] = {
#include "lobby/00843C40.inc"
};
char gLobbyLabelTexts_38[15] = {
#include "lobby/00843C48.inc"
};
char gLobbyLabelTexts_39[13] = {
#include "lobby/00843C58.inc"
};
char gLobbyLabelTexts_40[15] = {
#include "lobby/00843C68.inc"
};
char gLobbyLabelTexts_41[32] = {
#include "lobby/00843C80.inc"
};
char gLobbyLabelTexts_42[32] = {
#include "lobby/00843CA0.inc"
};
char gLobbyLabelTexts_43[32] = {
#include "lobby/00843CC0.inc"
};
char gLobbyLabelTexts_45[48] = {
#include "lobby/00843CE0.inc"
};
char gLobbyLabelTexts_46[32] = {
#include "lobby/00843D10.inc"
};
char gLobbyLabelTexts_47[16] = {
#include "lobby/00843D30.inc"
};
char gLobbyLabelTexts_48[16] = {
#include "lobby/00843D40.inc"
};
char gLobbyLabelTexts_49[16] = {
#include "lobby/00843D50.inc"
};
char gLobbyLabelTexts_51[16] = {
#include "lobby/00843D60.inc"
};
char gLobbyLabelTexts_52[24] = {
#include "lobby/00843D70.inc"
};
char gLobbyLabelTexts_53[13] = {
#include "lobby/00843D88.inc"
};
char gLobbyLabelTexts_54[15] = {
#include "lobby/00843D98.inc"
};
char gLobbyLabelTexts_55[32] = {
#include "lobby/00843DB0.inc"
};
char gLobbyLabelTexts_56[24] = {
#include "lobby/00843DD0.inc"
};
char gLobbyLabelTexts_57[8] = {
#include "lobby/00843DE8.inc"
};
char gLobbyNoSaveTexts_0[8] = {
#include "lobby/00843DF0.inc"
};
char gLobbyNoSaveTexts_1[12] = {
#include "lobby/00843DF8.inc"
};
char gLobbyButtonTexts_0[24] = {
#include "lobby/00843E10.inc"
};
char gLobbyButtonTexts_1[8] = {
#include "lobby/00843E28.inc"
};
char gLobbyButtonTexts_2[16] = {
#include "lobby/00843E30.inc"
};
char gLobbyButtonTexts_4[16] = {
#include "lobby/00843E40.inc"
};
char gLobbyButtonTexts_6[16] = {
#include "lobby/00843E50.inc"
};
char gLobbyButtonTexts_7[16] = {
#include "lobby/00843E60.inc"
};
char gLobbyButtonTexts_8[16] = {
#include "lobby/00843E70.inc"
};
char gLobbyButtonTexts_9[16] = {
#include "lobby/00843E80.inc"
};
char gLobbyButtonTexts_12[32] = {
#include "lobby/00843E90.inc"
};
char gLobbyButtonTexts_13[32] = {
#include "lobby/00843EB0.inc"
};
char gLobbyButtonTexts_14[16] = {
#include "lobby/00843ED0.inc"
};
char gLobbyButtonTexts_15[16] = {
#include "lobby/00843EE0.inc"
};
char gLobbyButtonTexts_17[8] = {
#include "lobby/00843EF0.inc"
};
char gLobbyButtonTexts_18[12] = {
#include "lobby/00843EF8.inc"
};
char gLobbyButtonTexts_20[12] = {
#include "lobby/00843F08.inc"
};
char gLobbyButtonTexts_22[15] = {
#include "lobby/00843F18.inc"
};
char gLobbyButtonTexts_23[12] = {
#include "lobby/00843F28.inc"
};
char gLobbyButtonTexts_24[12] = {
#include "lobby/00843F38.inc"
};
char gLobbyButtonTexts_26[15] = {
#include "lobby/00843F48.inc"
};
char gLobbyButtonTexts_27[15] = {
#include "lobby/00843F58.inc"
};
char gLobbyButtonTexts_28[8] = {
#include "lobby/00843F68.inc"
};
char gLobbyButtonTexts_29[24] = {
#include "lobby/00843F70.inc"
};
char gLobbyButtonTexts_30[8] = {
#include "lobby/00843F88.inc"
};
char gLobbyButtonTexts_32[16] = {
#include "lobby/00843F90.inc"
};
char gLobbyButtonTexts_33[16] = {
#include "lobby/00843FA0.inc"
};
char gLobbyButtonTexts_35[16] = {
#include "lobby/00843FB0.inc"
};
char gLobbyButtonTexts_37[16] = {
#include "lobby/00843FC0.inc"
};
char gLobbyButtonTexts_38[16] = {
#include "lobby/00843FD0.inc"
};
char gLobbyMsgTexts_0[32] = {
#include "lobby/00843FE0.inc"
};
char gLobbyMsgTexts_1[32] = {
#include "lobby/00844000.inc"
};
char gLobbyMsgTexts_2[32] = {
#include "lobby/00844020.inc"
};
char gLobbyMsgTexts_3[64] = {
#include "lobby/00844040.inc"
};
char gLobbyMsgTexts_4[64] = {
#include "lobby/00844080.inc"
};
char gLobbyMsgTexts_5[48] = {
#include "lobby/008440C0.inc"
};
char gLobbyMsgTexts_6[48] = {
#include "lobby/008440F0.inc"
};
char gLobbyMsgTexts_7[64] = {
#include "lobby/00844120.inc"
};
char gLobbyMsgTexts_8[96] = {
#include "lobby/00844160.inc"
};
char gLobbyMsgTexts_9[32] = {
#include "lobby/008441C0.inc"
};
char D_008441E0[64] = {
#include "lobby/008441E0.inc"
};
char gLobbyMsgTexts_12[64] = {
#include "lobby/00844220.inc"
};
char D_00844260[80] = {
#include "lobby/00844260.inc"
};
char gLobbyMsgTexts_15[64] = {
#include "lobby/008442B0.inc"
};
char gLobbyMsgTexts_16[48] = {
#include "lobby/008442F0.inc"
};
char D_00844320[32] = {
#include "lobby/00844320.inc"
};
char gLobbyMsgTexts_18[64] = {
#include "lobby/00844340.inc"
};
char gLobbyMsgTexts_19[64] = {
#include "lobby/00844380.inc"
};
char gLobbyMsgTexts_20[32] = {
#include "lobby/008443C0.inc"
};
char gLobbyMsgTexts_21[80] = {
#include "lobby/008443E0.inc"
};
char gLobbyMsgTexts_22[48] = {
#include "lobby/00844430.inc"
};
char gLobbyMsgTexts_23[48] = {
#include "lobby/00844460.inc"
};
char gLobbyMsgTexts_24[48] = {
#include "lobby/00844490.inc"
};
char gLobbyMsgTexts_25[48] = {
#include "lobby/008444C0.inc"
};
char gLobbyMsgTexts_27[48] = {
#include "lobby/008444F0.inc"
};
char gLobbyMsgTexts_28[48] = {
#include "lobby/00844520.inc"
};
char gLobbyMsgTexts_29[32] = {
#include "lobby/00844550.inc"
};
char gLobbyMsgTexts_30[96] = {
#include "lobby/00844570.inc"
};
char gLobbyMsgTexts_31[80] = {
#include "lobby/008445D0.inc"
};
char gLobbyMsgTexts_32[48] = {
#include "lobby/00844620.inc"
};
char gLobbyMsgTexts_33[32] = {
#include "lobby/00844650.inc"
};
char gLobbyMsgTexts_34[32] = {
#include "lobby/00844670.inc"
};
char gLobbyMsgTexts_35[48] = {
#include "lobby/00844690.inc"
};
char gLobbyMsgTexts_36[48] = {
#include "lobby/008446C0.inc"
};
char gLobbyMsgTexts_37[64] = {
#include "lobby/008446F0.inc"
};
char gLobbyMsgTexts_38[48] = {
#include "lobby/00844730.inc"
};
char gLobbyMsgTexts_39[48] = {
#include "lobby/00844760.inc"
};
char gLobbyMsgTexts_40[48] = {
#include "lobby/00844790.inc"
};
char gLobbyMsgTexts_41[48] = {
#include "lobby/008447C0.inc"
};
char gLobbyMsgTexts_42[48] = {
#include "lobby/008447F0.inc"
};
char gLobbyMsgTexts_43[64] = {
#include "lobby/00844820.inc"
};
char gLobbyMsgTexts_44[64] = {
#include "lobby/00844860.inc"
};
char gLobbyMsgTexts_45[112] = {
#include "lobby/008448A0.inc"
};
char gLobbyMsgTexts_46[64] = {
#include "lobby/00844910.inc"
};
char gLobbyMsgTexts_47[48] = {
#include "lobby/00844950.inc"
};
char gLobbyMsgTexts_48[96] = {
#include "lobby/00844980.inc"
};
char gLobbyMsgTexts_49[48] = {
#include "lobby/008449E0.inc"
};
char gLobbyMsgTexts_50[48] = {
#include "lobby/00844A10.inc"
};
char gLobbyMsgTexts_51[48] = {
#include "lobby/00844A40.inc"
};
char gLobbyMsgTexts_52[48] = {
#include "lobby/00844A70.inc"
};
char gLobbyMsgTexts_53[64] = {
#include "lobby/00844AA0.inc"
};
char gLobbyMsgTexts_54[112] = {
#include "lobby/00844AE0.inc"
};
char gLobbyMsgTexts_55[80] = {
#include "lobby/00844B50.inc"
};
char gLobbyMsgTexts_56[48] = {
#include "lobby/00844BA0.inc"
};
char gLobbyMsgTexts_57[32] = {
#include "lobby/00844BD0.inc"
};
char gLobbyMsgTexts_58[64] = {
#include "lobby/00844BF0.inc"
};
char gLobbyMsgTexts_59[32] = {
#include "lobby/00844C30.inc"
};
char gLobbyMsgTexts_60[80] = {
#include "lobby/00844C50.inc"
};
char gLobbyMsgTexts_61[96] = {
#include "lobby/00844CA0.inc"
};
char gLobbyMsgTexts_62[96] = {
#include "lobby/00844D00.inc"
};
char gLobbyMsgTexts_63[48] = {
#include "lobby/00844D60.inc"
};
char gLobbyMsgTexts_64[64] = {
#include "lobby/00844D90.inc"
};
char gLobbyMsgTexts_65[64] = {
#include "lobby/00844DD0.inc"
};
char gLobbyMsgTexts_66[32] = {
#include "lobby/00844E10.inc"
};
char gLobbyMsgTexts_67[32] = {
#include "lobby/00844E30.inc"
};
char gLobbyMsgTexts_68[32] = {
#include "lobby/00844E50.inc"
};
char gLobbyMsgTexts_69[48] = {
#include "lobby/00844E70.inc"
};
char gLobbyMsgTexts_70[32] = {
#include "lobby/00844EA0.inc"
};
char D_00844EC0[32] = {
#include "lobby/00844EC0.inc"
};
char gLobbyMsgTexts_72[32] = {
#include "lobby/00844EE0.inc"
};
char gLobbyMsgTexts_73[48] = {
#include "lobby/00844F00.inc"
};
char gLobbyMsgTexts_74[48] = {
#include "lobby/00844F30.inc"
};
char gLobbyMsgTexts_75[32] = {
#include "lobby/00844F60.inc"
};
char gLobbyMsgTexts_76[80] = {
#include "lobby/00844F80.inc"
};
char gLobbyMsgTexts_77[96] = {
#include "lobby/00844FD0.inc"
};
char gLobbyMsgTexts_78[96] = {
#include "lobby/00845030.inc"
};
char gLobbyMsgTexts_79[96] = {
#include "lobby/00845090.inc"
};
char gLobbyMsgTexts_80[48] = {
#include "lobby/008450F0.inc"
};
char gLobbyMsgTexts_81[96] = {
#include "lobby/00845120.inc"
};
char gLobbyMsgTexts_82[96] = {
#include "lobby/00845180.inc"
};
char gLobbyMsgTexts_83[96] = {
#include "lobby/008451E0.inc"
};
char gLobbyMsgTexts_84[48] = {
#include "lobby/00845240.inc"
};
char gLobbyMsgTexts_85[48] = {
#include "lobby/00845270.inc"
};
char gLobbyMsgTexts_86[48] = {
#include "lobby/008452A0.inc"
};
char gLobbyMsgTexts_87[64] = {
#include "lobby/008452D0.inc"
};
char gLobbyMsgTexts_88[96] = {
#include "lobby/00845310.inc"
};
char gLobbyMsgTexts_89[48] = {
#include "lobby/00845370.inc"
};
char gLobbyMsgTexts_90[48] = {
#include "lobby/008453A0.inc"
};
char gLobbyMsgTexts_91[48] = {
#include "lobby/008453D0.inc"
};
char gLobbyMsgTexts_92[96] = {
#include "lobby/00845400.inc"
};
char gLobbyMsgTexts_93[80] = {
#include "lobby/00845460.inc"
};
char gLobbyMsgTexts_94[48] = {
#include "lobby/008454B0.inc"
};
char gLobbyMsgTexts_95[48] = {
#include "lobby/008454E0.inc"
};
char gLobbyMsgTexts_96[48] = {
#include "lobby/00845510.inc"
};
char gLobbyMsgTexts_97[96] = {
#include "lobby/00845540.inc"
};
char gLobbyMsgTexts_98[272] = {
#include "lobby/008455A0.inc"
};
char gLobbyMsgTexts_99[144] = {
#include "lobby/008456B0.inc"
};
char gLobbyMsgTexts_100[48] = {
#include "lobby/00845740.inc"
};
char gLobbyMsgTexts_101[48] = {
#include "lobby/00845770.inc"
};
char gLobbyMsgTexts_102[80] = {
#include "lobby/008457A0.inc"
};
char gLobbyMsgTexts_103[80] = {
#include "lobby/008457F0.inc"
};
char gLobbyMsgTexts_104[48] = {
#include "lobby/00845840.inc"
};
char gLobbyMsgTexts_105[64] = {
#include "lobby/00845870.inc"
};
char gLobbyMsgTexts_106[48] = {
#include "lobby/008458B0.inc"
};
char gLobbyMsgTexts_107[48] = {
#include "lobby/008458E0.inc"
};
char gLobbyMsgTexts_108[32] = {
#include "lobby/00845910.inc"
};
char gLobbyMsgTexts_109[80] = {
#include "lobby/00845930.inc"
};
char gLobbyMsgTexts_110[48] = {
#include "lobby/00845980.inc"
};
char gLobbyMsgTexts_111[48] = {
#include "lobby/008459B0.inc"
};
char gLobbyMsgTexts_112[144] = {
#include "lobby/008459E0.inc"
};
char gLobbyMsgTexts_113[160] = {
#include "lobby/00845A70.inc"
};
char gLobbyMsgTexts_114[96] = {
#include "lobby/00845B10.inc"
};
char gLobbyMsgTexts_115[192] = {
#include "lobby/00845B70.inc"
};
char gLobbyMsgTexts_116[144] = {
#include "lobby/00845C30.inc"
};
char gLobbyMsgTexts_117[208] = {
#include "lobby/00845CC0.inc"
};
char gLobbyMsgTexts_118[256] = {
#include "lobby/00845D90.inc"
};
char gLobbyMsgTexts_119[32] = {
#include "lobby/00845E90.inc"
};
char gLobbyMsgTexts_120[32] = {
#include "lobby/00845EB0.inc"
};
char gLobbyMsgTexts_121[48] = {
#include "lobby/00845ED0.inc"
};
char gLobbyMsgTexts_122[32] = {
#include "lobby/00845F00.inc"
};
char gLobbyMsgTexts_123[32] = {
#include "lobby/00845F20.inc"
};
char gLobbyMsgTexts_124[48] = {
#include "lobby/00845F40.inc"
};
char gLobbyMsgTexts_126[96] = {
#include "lobby/00845F70.inc"
};
char gLobbyMsgTexts_127[96] = {
#include "lobby/00845FD0.inc"
};
char gLobbyMsgTexts_128[96] = {
#include "lobby/00846030.inc"
};
char gLobbyMsgTexts_129[96] = {
#include "lobby/00846090.inc"
};
char gLobbyMsgTexts_130[128] = {
#include "lobby/008460F0.inc"
};
char gLobbyMsgTexts_131[48] = {
#include "lobby/00846170.inc"
};
char gLobbyMsgTexts_132[80] = {
#include "lobby/008461A0.inc"
};
char gLobbyMsgTexts_133[96] = {
#include "lobby/008461F0.inc"
};
char gLobbyMsgTexts_134[48] = {
#include "lobby/00846250.inc"
};
char gLobbyMsgTexts_135[80] = {
#include "lobby/00846280.inc"
};
char gLobbyMsgTexts_136[64] = {
#include "lobby/008462D0.inc"
};
char gLobbyMsgTexts_137[64] = {
#include "lobby/00846310.inc"
};
char gLobbyMsgTexts_139[32] = {
#include "lobby/00846350.inc"
};
char gLobbyMsgTexts_140[48] = {
#include "lobby/00846370.inc"
};
char gLobbyMsgTexts_141[96] = {
#include "lobby/008463A0.inc"
};
char gLobbyMsgTexts_142[48] = {
#include "lobby/00846400.inc"
};
char gLobbyMsgTexts_143[80] = {
#include "lobby/00846430.inc"
};
char gLobbyMsgTexts_144[48] = {
#include "lobby/00846480.inc"
};
char gLobbyMsgTexts_145[64] = {
#include "lobby/008464B0.inc"
};
char gLobbyMsgTexts_146[176] = {
#include "lobby/008464F0.inc"
};
char gLobbyMsgTexts_147[128] = {
#include "lobby/008465A0.inc"
};
char gLobbyMsgTexts_148[48] = {
#include "lobby/00846620.inc"
};
char gLobbyMsgTexts_149[80] = {
#include "lobby/00846650.inc"
};
char gLobbyMsgTexts_150[48] = {
#include "lobby/008466A0.inc"
};
#pragma rodata off
