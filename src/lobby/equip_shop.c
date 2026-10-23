/* The equipment shop (TOWN_MODE_EQUIP_SHOP): its opening and closing (EquipShop_Open,
 * EquipShop_Close), its frame (EquipShop_Update,
 * by enum EquipShopState), the first menus (buy, sell, the townsperson's
 * offers; weapons or armour; the kind), the page of equipment on sale with
 * its detail views and the model preview on the hunter, the purchase's
 * questions, the box to sell from, and their drawing (EquipShop_DrawTask).
 * The shop's work is EquipShop (lobby_menu.h) in gMenuWork. Its data: the
 * equipment types of each tab's kinds, the header box's colours, the
 * pets', service and stationed townspeople's handler tables (their
 * functions come next, with the townspeople's motion effects), the
 * lobby's shadow sizes and its monster hit tables. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "lobby_state.h"
#include "player.h"
#include "capsule.h"
#include "lobby_menu.h"
#include "lobby_labels.h"
#include "village_cond.h"
#include "em.h"
#include "pad.h"
#include "lobby_text.h"
#include "item.h"
#include "prim.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "box_menu.h"
#include "camera.h"
#include "lobby_forge.h"
#include "npc_motion.h"
#include "equip_shop.h"
#include "equip_parts.h"
#include "font.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "member_card.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "talk.h"
#include "town_frame.h"
#include "libc.h"
#include "window.h"

extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
void Text_Printf(const char *, ...);
extern s8 gMenuInputLock;
extern u8 gEquipPreview[];
extern s8 gLobbyTextDirty;
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
struct Player;
/* The calls that mask the value again (u8 or & 0xFF) cast it to s32 first: the compiler keeps the game's extra mask that way. */
void Text_SetSize(s32 size);
void Text_PrintfColored(char *fmt, ...);
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: the definition takes s32 coordinates; with those the calls widen the s16 coordinates differently from the game's code */
void Item_DrawRarity(s16 x, s16 y, u8 rarity);
u8 Equip_GetRarity(u8 type, u16 id);

struct Em;
extern s32 gTownMenuOpen;
extern s32 gTownMode;

/* Opens the equipment shop (TOWN_MODE_EQUIP_SHOP, drawn by
 * EquipShop_DrawTask) for the townsperson npc, noting whether it also has
 * deliveries or quests to offer. */
void EquipShop_Open(u8 *npc)
{
    EquipShop *shop;

    gTownMenuOpen = 1;
    shop = (EquipShop *)gMenuWork;
    gTownMode = TOWN_MODE_EQUIP_SHOP;
    memset(shop, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(EquipShop_DrawTask, 0, 0, 0);
    gMenuInputLock = 6; /* frames before the menu takes buttons */
    shop->npc = npc;
    if (OfferMenu_BuildList((struct Em *) npc, 0) != 0) {
        shop->hasOffers = 1;
    }
    shop->previewOff = 1;
    LobbySe_Play(LOBBY_SE_SELECT);
}

/* Closes the equipment shop: back to TOWN_MODE_FREE, the chat's colour
 * reset, the preview's model freed and the camera's focus ended. */
void EquipShop_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    Chat_SetColor(-1);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    EquipPreview_Free();
    CamFocus_End();
    LobbySe_Play(LOBBY_SE_CANCEL);
}

/* The equipment shop's frame, each frame by its state (enum
 * EquipShopState): the first menus, the list and the purchase, the box and
 * the sale, then the farewell talk and the close. The model preview stays
 * hidden unless the list's draw shows it. */
void EquipShop_Update(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(2, 2);
    EquipPreview_Hide(1);
    switch (shop->state) {
    case EQUIP_SHOP_STATE_CHOOSE:
        switch (EquipShop_InputTop(pad)) {
        case MENU_DONE:
            shop->cursor = 0;
            shop->page = 0;
            shop->state++;
            WinSlide_Step(&shop->win, 640, 280);
            EquipPreview_Reset();
            shop->previewOff = 1;
            if (shop->choice == EQUIP_SHOP_BUY) {
                EquipShop_OpenList();
                Chat_SetColor(0);
            } else if (shop->choice == EQUIP_SHOP_SELL) {
                EquipBox_FirstPage();
            } else {
                OfferMenu_Open((struct Em *) shop->npc);
                break;
            }
            CamFocus_SetAside(1);
            break;
        case MENU_CANCEL:
            Talk_Say(1, 0);
            shop->state = EQUIP_SHOP_STATE_FAREWELL;
            break;
        }
        break;
    case EQUIP_SHOP_STATE_OPEN:
        switch (WinSlide_Step(&shop->win, 640, 280)) {
        case MENU_DONE:
            if (shop->choice == EQUIP_SHOP_BUY) {
                shop->state = EQUIP_SHOP_STATE_LIST;
            } else {
                shop->state = EQUIP_SHOP_STATE_BOX;
            }
            break;
        }
        break;
    case EQUIP_SHOP_STATE_LIST:
        switch (EquipShop_InputList(pad)) {
        case MENU_DONE:
            shop->state++;
            shop->view = EQUIP_SHOP_VIEW_COMPARE;
            EquipShop_TryOn();
            Chat_SetColor(-1);
            break;
        case MENU_CANCEL:
            shop->cursor = shop->kind;
            if (shop->choice == EQUIP_SHOP_BUY) {
                shop->cursor = shop->kind;
                shop->step = 2;
            } else {
                shop->cursor = shop->choice;
            }
            shop->state = EQUIP_SHOP_STATE_CHOOSE;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case EQUIP_SHOP_STATE_BUY:
        switch (EquipShop_InputBuy(pad)) {
        case MENU_DONE:
            shop->state = EQUIP_SHOP_STATE_LIST;
            Chat_SetColor(0);
            EquipShop_OpenList();
            break;
        case MENU_CANCEL:
            shop->state = EQUIP_SHOP_STATE_LIST;
            Chat_SetColor(0);
            break;
        }
        break;
    case EQUIP_SHOP_STATE_BOX:
        switch (EquipShop_InputSell(pad)) {
        case MENU_DONE:
            shop->state++;
            break;
        case MENU_CANCEL:
            shop->cursor = shop->choice;
            shop->state = EQUIP_SHOP_STATE_CHOOSE;
            CamFocus_SetAside(0);
            break;
        }
        break;
    case EQUIP_SHOP_STATE_SELL:
        switch (EquipBox_SellMenu(pad)) {
        case MENU_DONE:
            shop->state = EQUIP_SHOP_STATE_BOX;
            EquipBox_FirstPage();
            break;
        case MENU_CANCEL:
            shop->state = EQUIP_SHOP_STATE_BOX;
            shop->detailPages = 0;
            break;
        }
        break;
    case EQUIP_SHOP_STATE_FAREWELL:
        switch (Talk_Update()) {
        case MENU_DONE:
            shop->state++;
            break;
        }
        break;
    case EQUIP_SHOP_STATE_CLOSE:
        EquipShop_Close();
        break;
    }
}

/* The first menus, by step: the choice (buy, sell, or the townsperson's
 * offers when it has any; sell and offers end here), the tab (weapons or
 * armour), the kind. Circle takes, cross goes back (MENU_CANCEL from the
 * first). MENU_DONE once chosen, else MENU_RUNNING. */
s32 EquipShop_InputTop(u16 pad)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    switch (shop->step) {
    case 0:
        if (pad & PAD_CIRCLE) {
            shop->choice = shop->cursor;
            shop->cursor = 0;
            LobbySe_Play(LOBBY_SE_SELECT);
            if (shop->choice == EQUIP_SHOP_OFFERS) {
                return MENU_DONE;
            }
            if (shop->choice == EQUIP_SHOP_SELL) {
                return MENU_DONE;
            }
            shop->step++;
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else if (shop->hasOffers == 0) {
            shop->cursor = Menu_WrapUpDownSe(shop->cursor, 2, 2);
        } else {
            shop->cursor = Menu_WrapUpDownSe(shop->cursor, 3, 2);
        }
        break;
    case 1:
        if (pad & PAD_CIRCLE) {
            shop->tab = shop->cursor;
            shop->cursor = 0;
            shop->step++;
            LobbySe_Play(LOBBY_SE_SELECT);
        } else if (pad & PAD_CROSS) {
            shop->cursor = shop->choice;
            shop->step--;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else {
            shop->cursor = Menu_WrapUpDownSe(shop->cursor, 2, 2);
        }
        break;
    case 2:
        if (pad & PAD_CIRCLE) {
            shop->kind = shop->cursor;
            shop->cursor = 0;
            shop->step = 0;
            LobbySe_Play(28);
            return MENU_DONE;
        } else if (pad & PAD_CROSS) {
            shop->cursor = shop->tab;
            shop->step--;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else {
            shop->cursor = Menu_WrapUpDownSe(shop->cursor, 5, 2);
        }
        break;
    }
    return MENU_RUNNING;
}

/* 1 when the piece (type, id) is on sale to the local hunter: id isn't 0
 * and the hunter's gender can wear it. A piece not yet made is marked
 * made and new on the way. */
s32 EquipShop_IsListed(u8 type, s32 id)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    EquipItem piece;

    if (id == 0) {
        return 0;
    }
    memset(&piece, 0, sizeof(piece));
    piece.type = type;
    piece.id = id;
    if (!((1 << pl->gender) & Equip_GetWearMask(&gSave, &piece))) {
        return 0;
    }
    if (Equip_TestFlag(&piece, EQUIP_FLAG_MADE) == 0) {
        Equip_SetFlag(&piece, EQUIP_FLAG_NEW);
        Equip_SetFlag(&piece, EQUIP_FLAG_MADE);
    }
    return 1;
}

extern s32 **gEquipShopLists[];

/* The list of piece ids (0 ends it) for tab and kind listIdx (tab *
 * EQUIP_SHOP_KINDS + kind), from the shop's rank: online the hunter rank
 * gates, offline two of the village's conditions. */
s32 *EquipShop_GetList(s32 listIdx)
{
    s32 rank;

    if (Game_IsOnline() == 1) {
        if (Cond_Check(COND_HR_GATE_2_HR_31) == 1) {
            rank = 3;
        } else if (Cond_Check(COND_HR_GATE_1_HR_17) == 1) {
            rank = 2;
        } else if (Cond_Check(COND_HR_GATE_0_HR_11) == 1) {
            rank = 1;
        } else {
            rank = 0;
        }
    } else {
        if (Cond_Check(62) == 1) {
            rank = 2;
        } else if (Cond_Check(55) == 1) {
            rank = 1;
        } else {
            rank = 0;
        }
    }
    return gEquipShopLists[rank][listIdx];
}

extern u8 gEquipShopTypesTab0[];
extern u8 gEquipShopTypesTab1[];

/* Fills the page's lines from the list: the pieces on sale on this page,
 * their prices, and whether each can't be bought (too dear, or the box is
 * full). Keeps the cursor on a line. */
void EquipShop_BuildPage(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s32 *list = EquipShop_GetList(shop->kind + shop->tab * EQUIP_SHOP_KINDS);
    s32 listed;
    u8 type;
    u8 lineCount;

    if (shop->tab == 0) {
        type = gEquipShopTypesTab0[shop->kind];
    } else {
        type = gEquipShopTypesTab1[shop->kind];
    }
    memset(shop->line, 0, sizeof(shop->line));
    listed = lineCount = 0;
    for (; *list != 0; list++) {
        if (EquipShop_IsListed(type, *list) == 1) {
            listed++;
            if (shop->page * EQUIP_SHOP_LINES < listed) {
                shop->line[lineCount].item.type = type;
                shop->line[lineCount].item.id = *list;
                shop->line[lineCount].item.used = 1;
                shop->line[lineCount].price = Equip_GetPrice(type, *list);
                if (shop->line[lineCount].price <= (u32)gSave.money && (u16)Box_FindFreeSlot(&gSave) != BOX_NONE) {
                    shop->line[lineCount].noBuy = 0;
                } else {
                    shop->line[lineCount].noBuy = 1;
                }
                lineCount++;
                if (lineCount >= EQUIP_SHOP_LINES) {
                    break;
                }
            }
        }
    }
    shop->lines = lineCount;
    if (shop->cursor >= shop->lines) {
        shop->cursor = shop->lines - 1;
    }
}

/* The piece worn of the type (the save's equip[] by SaveEquipSlot), copied
 * to worn for the comparison; nothing for type 1. */
void EquipShop_LoadCompare(u8 type)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    memset(&shop->worn, 0, sizeof(EquipItem));
    shop->worn.type = type;
    switch (type) {
    case EQUIP_BLADE:
    case EQUIP_GUN:
        shop->worn = gSave.equip[SAVE_EQUIP_WEAPON];
        break;
    case EQUIP_HEAD:
        shop->worn = gSave.equip[SAVE_EQUIP_HEAD];
        break;
    case EQUIP_BODY:
        shop->worn = gSave.equip[SAVE_EQUIP_BODY];
        break;
    case EQUIP_ARMS:
        shop->worn = gSave.equip[SAVE_EQUIP_ARMS];
        break;
    case EQUIP_WAIST:
        shop->worn = gSave.equip[SAVE_EQUIP_WAIST];
        break;
    case EQUIP_LEGS:
        shop->worn = gSave.equip[SAVE_EQUIP_LEGS];
        break;
    }
    shop->worn.used = 1;
}


/* Opens the list of the chosen tab and kind: the piece worn to compare,
 * the count on sale and its pages, the first page; a notice when the box
 * is full. */
void EquipShop_OpenList(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s32 *list = EquipShop_GetList(shop->kind + shop->tab * EQUIP_SHOP_KINDS);
    u8 type;

    if (shop->tab == 0) {
        type = gEquipShopTypesTab0[shop->kind];
    } else {
        type = gEquipShopTypesTab1[shop->kind];
    }
    shop->detailPage = 0;
    shop->view = EQUIP_SHOP_VIEW_PAGE;
    EquipShop_LoadCompare(type);
    shop->count = 0;
    for (; *list != 0; list++) {
        if (EquipShop_IsListed(type, *list) == 1) {
            shop->count++;
        }
    }
    shop->pages = Menu_CountPages(shop->count, EQUIP_SHOP_LINES);
    EquipShop_BuildPage();
    if ((u16)Box_FindFreeSlot(&gSave) == BOX_NONE) {
        Menu_PushNotice(LOBBY_NOTICE_BOX_FULL);
    }
}

s32 Equip_GetModelId(u8 type, u16 id, u8 x); /* unit's own declaration: the definition takes (u16 kind, u16 id, u16 female); with those the calls convert the arguments differently from the game's code */
void EquipPreview_Load(u8 type, s32 n, u8 x); /* unit's own declaration: the definition takes (u16 kind, u16 id, u8 female); with those the calls convert the arguments differently from the game's code */

/* The detail of the line under the cursor for the view shown: the skills
 * with it (EQUIP_SHOP_VIEW_SKILLS), the comparison with the piece worn,
 * or the piece itself, whose model is loaded on the hunter when it
 * changed and the preview is on. detailPages takes the view's page count.
 * The game clears sizeof(EquipItem) bytes at the pointer's own address. */
void EquipShop_TryOn(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    EquipItem *lineItem = &shop->line[shop->cursor].item;
    EquipItem *piece;
    u8 female;

    memset(&piece, 0, sizeof(EquipItem));
    switch (shop->view) {
    case EQUIP_SHOP_VIEW_SKILLS:
        shop->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &shop->line[shop->cursor].item);
        shop->detailPage = 0;
        break;
    case EQUIP_SHOP_VIEW_COMPARE:
        if (lineItem->type != EQUIP_BLADE && lineItem->type != EQUIP_GUN) {
            shop->detailPages = Equip_LevelMark(&shop->worn, 80, lineItem);
        } else {
            shop->detailPages = Equip_LevelMark(&shop->worn, 64, lineItem);
        }
        break;
    case EQUIP_SHOP_VIEW_ITEM:
        shop->detailPages = Equip_LevelMark(&shop->line[shop->cursor].item, 64, NULL);
        if (shop->previewOff == 0) {
            piece = &shop->line[shop->cursor].item;
            if (piece->type != shop->shown.type || piece->id != shop->shown.id) {
                EquipPreview_Free();
                shop->shown = *piece;
                female = gSave.gender;
                EquipPreview_Load(piece->type, Equip_GetModelId(piece->type, piece->id, female), female);
            }
        }
        break;
    }
}

/* Hides the model preview (hide 1) or shows it (0). */
void EquipPreview_Hide(u8 hide)
{
    u8 *shown = gEquipPreview;

    if (hide == 0) {
        *shown = 1;
    } else {
        *shown = 0;
    }
}

/* The list's input: circle buys the line under the cursor (MENU_DONE),
 * cross closes the detail or the list (MENU_CANCEL), square steps the
 * detail views, the pad the pages and lines, R1 and L1 the detail's pages,
 * select the model preview in EQUIP_SHOP_VIEW_ITEM; the detail is redone
 * when the page or line changed. Else MENU_RUNNING. */
s32 EquipShop_InputList(u16 pad)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    EquipItem *piece;
    s16 prevCursor;
    s16 prevPage;
    u8 female;

    prevPage = shop->page;
    prevCursor = shop->cursor;
    if (pad & PAD_CIRCLE) {
        if (shop->line[prevCursor].noBuy == 0) {
            EquipShop_Choose();
            LobbySe_Play(LOBBY_SE_SELECT);
            return MENU_DONE;
        }
        LobbySe_Play(LOBBY_SE_REFUSE);
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(5);
        if (shop->view != EQUIP_SHOP_VIEW_PAGE) {
            shop->view = EQUIP_SHOP_VIEW_PAGE;
            shop->detailPage = 0;
        } else {
            return MENU_CANCEL;
        }
    } else if (pad & PAD_SQUARE) {
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
        if (++shop->view >= 4) {
            shop->view = EQUIP_SHOP_VIEW_ITEM;
        }
        EquipShop_TryOn();
        shop->detailPage = 0;
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        shop->page = Menu_WrapLeftRight(prevPage, shop->pages);
        if (prevPage != shop->page) {
            EquipShop_BuildPage();
        }
    } else if (pad & (PAD_UP | PAD_DOWN)) {
        shop->cursor = Menu_WrapUpDownSe(prevCursor, shop->lines, 2);
    } else if (pad & PAD_R1) {
        if (shop->view != EQUIP_SHOP_VIEW_PAGE && shop->detailPages > 1) {
            if (++shop->detailPage >= shop->detailPages) {
                shop->detailPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (shop->view != EQUIP_SHOP_VIEW_PAGE && shop->detailPages > 1) {
            if (--shop->detailPage < 0) {
                shop->detailPage = shop->detailPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_SELECT) {
        if (shop->view == EQUIP_SHOP_VIEW_ITEM) {
            if (shop->previewOff == 0) {
                shop->previewOff = 1;
                EquipPreview_Hide(1);
            } else {
                shop->previewOff = 0;
                piece = &shop->line[shop->cursor].item;
                if (piece->type != shop->shown.type || piece->id != shop->shown.id) {
                    EquipPreview_Free();
                    shop->shown = *piece;
                    female = gSave.gender;
                    EquipPreview_Load(piece->type, Equip_GetModelId(piece->type, piece->id, female), female);
                }
            }
        }
    }
    if (prevPage != shop->page || prevCursor != shop->cursor) {
        EquipShop_TryOn();
    }
    return MENU_RUNNING;
}

/* Opens the box at its first page, with no detail shown. */
void EquipBox_FirstPage(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    shop->detailPage = 0;
    shop->view = 0;
    shop->pages = Box_GetPageCount(&gSave);
}

/* The box's input (BOX_PAGE_COLS by BOX_PAGE_ROWS slots a page): circle
 * takes a slot to sell (MENU_DONE; not free and not worn), cross closes the
 * detail or the box (MENU_CANCEL), square the piece's detail, R1 and L1 its
 * pages, R2 and L2 the box's pages, the pad moves the cursor. Else
 * MENU_RUNNING. */
s32 EquipShop_InputSell(u16 pad)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s32 slot = shop->col + (shop->row * BOX_PAGE_COLS + shop->page * BOX_PAGE_SLOTS);

    if (pad & PAD_CIRCLE) {
        if (Box_IsSlotFree(&gSave, slot) == 0 && Equip_IsSlotEquipped(&gSave, slot) == 0) {
            EquipBox_SellPrice();
            LobbySe_Play(LOBBY_SE_SELECT);
            return MENU_DONE;
        }
        LobbySe_Play(LOBBY_SE_REFUSE);
    } else if (pad & PAD_CROSS) {
        if (shop->view != 0) {
            shop->view = 0;
            shop->detailPage = 0;
            shop->detailPages = 0;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        }
    } else if (pad & PAD_SQUARE) {
        /* the slot's item id (gSave.box[slot].id) */
        if (gSave.box[slot].id != 0) {
            shop->view ^= 1;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
            if (shop->view != 0) {
                shop->detailPages = Equip_LevelMark(&gSave.box[slot], 64, NULL);
            } else {
                shop->detailPages = 0;
                shop->detailPage = 0;
            }
        }
    } else if (pad & PAD_R1) {
        if (shop->view != 0 && shop->detailPages > 1) {
            if (++shop->detailPage >= shop->detailPages) {
                shop->detailPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (shop->view != 0 && shop->detailPages > 1) {
            if (--shop->detailPage < 0) {
                shop->detailPage = shop->detailPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_R2) {
        if (shop->pages > 1) {
            if (++shop->page >= shop->pages) {
                shop->page = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L2) {
        if (shop->pages > 1) {
            if (--shop->page < 0) {
                shop->page = shop->pages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (shop->view == 0) {
        shop->col = Menu_WrapLeftRight(shop->col, BOX_PAGE_COLS);
        shop->row = Menu_WrapUpDown(shop->row, BOX_PAGE_ROWS);
    }
    return MENU_RUNNING;
}

/* Takes the line under the cursor to buy: its price and piece, and the
 * first question: a warning when the hunter can't wear it, else the
 * price. */
void EquipShop_Choose(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    shop->price = shop->line[shop->cursor].price;
    shop->item = shop->line[shop->cursor].item;
    shop->item.used = 1;
    Menu_SetYesNo(0);
    gMenuInputLock = 9;
    if (Equip_CanWear(&gSave, &shop->item) == 0) {
        shop->msg = gEquipShopTexts[EQUIP_SHOP_TEXT_CANNOT_WEAR];
        shop->step = 0;
    } else {
        shop->msg = Text_FillDollar((char *) gEquipShopTexts[EQUIP_SHOP_TEXT_PRICE_ASK], shop->price);
        shop->step = 1;
    }
}

/* Buys the piece: it goes to the box and its price is paid. With equip,
 * the hunter puts it on and the local player is rebuilt; else a notice
 * that it went to the box. */
void EquipShop_Buy(u8 equip)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    u16 slot;

    slot = Box_AddItem(&gSave, shop->item.type, shop->item.id, 0);
    Save_AddMoney(-shop->price);
    LobbySe_Play(LOBBY_SE_MONEY);
    if (equip == 0) {
        Player_SetColorWord(&gPlayers[gScene.localPlayer], gSave.symbolColor);
        Menu_PushNotice(LOBBY_NOTICE_SENT_TO_BOX);
        return;
    }
    LobbySe_Play(LOBBY_SE_EQUIP);
    Equip_EquipSlot(&gSave, slot);
    Player_SetupFromSave(&gPlayers[gScene.localPlayer], &gSave);
    Player_CalcSkills(&gPlayers[gScene.localPlayer]);
    Town_ReloadLocalPlayer();
    gLobbyTextDirty = 1;
    Menu_PushNotice(LOBBY_NOTICE_EQUIPPED);
}

/* The purchase's questions, by step (R1 and L1 turn the detail's pages at
 * each): the warning that it can't be worn, the price, then whether to
 * put it on at once (with a warning when the weapon's kind changes).
 * MENU_DONE once bought, MENU_CANCEL on a no, else MENU_RUNNING. */
s32 EquipShop_InputBuy(u16 pad)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    switch (shop->step) {
    case MENU_DONE:
        if (pad & PAD_R1) {
            if (shop->detailPages > 1) {
                if (++shop->detailPage >= shop->detailPages) {
                    shop->detailPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (shop->detailPages > 1) {
                if (--shop->detailPage < 0) {
                    shop->detailPage = shop->detailPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (LobbyMenu_YesNoInput(LOBBY_SE_SELECT)) {
            case MENU_DONE:
                shop->step++;
                LobbyText_Open(1);
                shop->msg = Text_FillDollar((char *) gEquipShopTexts[EQUIP_SHOP_TEXT_PRICE_ASK], shop->price);
                break;
            case MENU_CANCEL:
                return MENU_CANCEL;
            }
        }
        break;
    case 1:
        if (pad & PAD_R1) {
            if (shop->detailPages > 1) {
                if (++shop->detailPage >= shop->detailPages) {
                    shop->detailPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (shop->detailPages > 1) {
                if (--shop->detailPage < 0) {
                    shop->detailPage = shop->detailPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (LobbyMenu_YesNoInput(19)) {
            case MENU_DONE:
                if (Equip_CanWear(&gSave, &shop->item) == 1) {
                    shop->step++;
                    shop->msg = gEquipShopTexts[EQUIP_SHOP_TEXT_EQUIP_ASK];
                    Menu_SetYesNo(0);
                    gMenuInputLock = 9;
                    switch (shop->worn.type) {
                    case EQUIP_BLADE:
                    case EQUIP_GUN:
                        if (shop->worn.type != shop->item.type) {
                            shop->msg = gEquipShopTexts[EQUIP_SHOP_TEXT_WEAPON_CHANGE_WARN];
                        }
                        break;
                    }
                } else {
                    EquipShop_Buy(0);
                    shop->step = 0;
                    return MENU_DONE;
                }
                break;
            case MENU_CANCEL:
                shop->step = 0;
                return MENU_CANCEL;
            }
        }
        break;
    case 2:
        if (pad & PAD_R1) {
            if (shop->detailPages > 1) {
                if (++shop->detailPage >= shop->detailPages) {
                    shop->detailPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (shop->detailPages > 1) {
                if (--shop->detailPage < 0) {
                    shop->detailPage = shop->detailPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (LobbyMenu_YesNoInput(LOBBY_SE_SELECT)) {
            case MENU_DONE:
                EquipShop_Buy(1);
                shop->step = 0;
                return MENU_DONE;
            case MENU_CANCEL:
                EquipShop_Buy(0);
                shop->step = 0;
                return MENU_DONE;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/* Asks the sale price of the box slot under the cursor. */
void EquipBox_SellPrice(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s32 slot = shop->col + (shop->row * BOX_PAGE_COLS + shop->page * BOX_PAGE_SLOTS);

    Menu_SetYesNo(0);
    gMenuInputLock = 9;
    shop->price = Equip_GetSellPrice(&gSave.box[slot]);
    shop->msg = Text_FillDollar((char *) gEquipShopTexts[EQUIP_SHOP_TEXT_PRICE_ASK], shop->price);
}

/* The sale's question (R1 and L1 turn the detail's pages): yes takes the
 * money, empties the slot and takes the piece out of the equipment sets
 * (MENU_DONE); no gives MENU_CANCEL. Else MENU_RUNNING. */
s32 EquipBox_SellMenu(u16 pad)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s32 slot = shop->col + (shop->row * BOX_PAGE_COLS + shop->page * BOX_PAGE_SLOTS);


    if (pad & PAD_R1) {
        if (shop->detailPages > 1) {
            if (++shop->detailPage >= shop->detailPages) {
                shop->detailPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (shop->detailPages > 1) {
            if (--shop->detailPage < 0) {
                shop->detailPage = shop->detailPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else {
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            Save_AddMoney(shop->price);
            Box_ClearSlot(&gSave, slot);
            LobbySe_Play(LOBBY_SE_MONEY);
            if ((u8)(s32)EquipSet_RemoveSlot(slot, 1)) {
                EquipSet_RemoveSlot(slot, 0);
            }
            return MENU_DONE;
        case MENU_CANCEL:
            return MENU_CANCEL;
        }
    }
    return MENU_RUNNING;
}


/* Draws the first menus: the choice, the tab or the kind, by step. */
void EquipShop_DrawMenu(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    Font_SetColor(0x807F7F7F);
    Font_SetSpacing(20, 20);
    switch (shop->step) {
    case 0:
        if (shop->hasOffers == 0) {
            Menu_DrawLabelList(460, &gShopMenuTexts[SHOP_MENU_TEXT_BUY], 2, shop->cursor, 0xFFB0B000);
        } else {
            Menu_DrawLabelList(460, gShopBuySellTexts, 3, shop->cursor, 0xFFB0B000);
        }
        break;
    case 1:
        Menu_DrawLabelList(460, gWeaponArmorTexts, 2, shop->cursor, 0xFFB0B000);
        break;
    case 2:
        if (shop->tab == 0) {
            Menu_DrawLabelList(420, &gEquipGroupTexts[shop->tab * EQUIP_SHOP_KINDS], 5, shop->cursor, 0xFFB0B000);
        } else {
            Menu_DrawLabelList(460, &gEquipGroupTexts[shop->tab * EQUIP_SHOP_KINDS], 5, shop->cursor, 0xFFB0B000);
        }
        break;
    }
}

/* The count of the piece (type, id) in the box. */
s32 Box_CountEquip(u8 type, u16 id)
{
    EquipItem *slot = gSave.box;
    s32 i;
    s32 capacity = (s16)Box_GetCapacity(&gSave);
    s32 count = 0;

    for (i = 0; i < capacity; i++, slot++) {
        if (slot->type == type && slot->id == id) {
            count++;
        }
    }
    return count;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/equip_shop", gEquipShopStrFmt);

extern char gEquipShopStrFmt[];

/* A line of the equipment shop: the icon (with its rarity unless it is a
 * random weapon), the name, the count owned and the wearer. y steps inside
 * the calls' arguments, as the game's code does (as statements, the
 * compiler extends y before the add). */
void EquipShop_DrawLine(s16 x, s16 y, EquipItem *piece)
{
    s32 textX;

    if (Forge_IsRandomWeapon(piece->id)) {
        Lobby_DrawEquipIcon(x, y, 60, 60, piece);
    } else {
        Lobby_DrawEquipIcon(x, y, 60, 60, piece);
        Item_DrawRarity(x + 12, y + 60, Equip_GetRarity(piece->type, piece->id));
    }
    Font_SetSpacing(18, 18);
    Font_SetColor(0x80101010);
    textX = x + 78;
    Font_SetPos(textX, y);
    if (Forge_IsRandomWeapon(piece->id)) {
        Text_PrintfColored(gEquipShopStrFmt, gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME]);
    } else {
        Text_PrintfColored(gEquipShopStrFmt, Equip_GetName(piece->type, piece->id));
    }
    Font_SetPos(textX, y += 22);
    Text_PrintfColored("%s%s%3d", gCountLabels[COUNT_LABEL_HELD], gLobbyWords[LOBBY_WORD_TIMES], Box_CountEquip(piece->type, piece->id));
    Font_SetPos(textX, y += 22);
    Equip_DrawWearer(piece);
}

/* The note under the list: pick a weapon, or pick armour. */
void EquipShop_DrawNote(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    char *msg;

    switch (shop->line[shop->cursor].item.type) {
    case EQUIP_BLADE:
    case EQUIP_GUN:
        msg = gEquipShopTexts[EQUIP_SHOP_TEXT_PICK_WEAPON];
        break;
    default:
        msg = gEquipShopTexts[EQUIP_SHOP_TEXT_PICK_ARMOR];
        break;
    }
    Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0x80202020, msg);
}

/* The page's frame: the title, the money and the window at (x, y). */
void EquipShop_DrawFrame(s16 x, s16 y)
{
    EquipShop_DrawTitle();
    Menu_DrawMoney(480, 16, 0);
    Icon_Draw(x, y, 340, 290, -8739, 0);
    Icon_Draw(x + 110, y + 45, 120, 200, 0x60FFFFFF, 36);
}

extern u16 gFrameTick;

/* The equipment shop's page at (x, y): the page counter, each line (the
 * icon blinking with the new mark, the cursor frame, the name and price,
 * greyed when it can't be bought), then the line under the cursor and the
 * note. */
void EquipShop_DrawPage(s16 x, s16 y)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s16 top = y;
    ShopLine *entry;
    s16 i;

    EquipShop_DrawFrame(x, y);
    y += 26;
    Font_SetColor(0x80101010);
    Menu_DrawPageNum((s16)(x + 40) + 200, y - 22, shop->page, shop->pages, 1);
    Font_SetSpacing(20, 20);
    Font_SetPos(x + 40, y);
    for (i = 0; i < EQUIP_SHOP_LINES; i++) {
        entry = &shop->line[i];
        if (entry->item.id == 0) {
            break;
        }
        if (Equip_TestFlag(&entry->item, EQUIP_FLAG_NEW) == 1 && (gFrameTick & 0x10)) {
            Icon_Draw((s16)(x + 40) - 18, y, 16, 20, 0xFFC03000, 99);
        } else {
            Lobby_DrawEquipIcon((s16)(x + 40) - 24, y, 20, 20, &entry->item);
        }
        if (i == shop->cursor) {
            Menu_DrawCursorRow(x + 40, y, 274, 50, 0xC0106060, 0xFF106060);
        }
        if (entry->noBuy != 0) {
            Font_SetColor(0x8042352E);
        } else {
            Font_SetColor(0x80101010);
        }
        Text_PrintfColored("%-20s %6d%s", Equip_GetName(entry->item.type, entry->item.id), entry->price, gLobbyWords[LOBBY_WORD_ZENNY]);
        Font_SetPos(x + 40, y += 22);
    }
    Menu_DrawDashLine(x + 20, top + 186, 300, 0xFF000000);
    if (shop->cursor < 0 || shop->cursor >= EQUIP_SHOP_LINES) {} /* compiled-out check */
    EquipShop_DrawLine(x + 16, top + 200, &shop->line[shop->cursor].item);
    EquipShop_DrawNote();
}

/* The equipment shop's page in the small layout: the lines at fixed
 * places, the cursor bar with its blinking arrow. */
void EquipShop_DrawPageSmall(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s16 blink;
    ShopLine *entry;
    s16 y;
    s16 i;

    Icon_Draw(0, 50, 290, 180, -8739, 0);
    Icon_Draw(100, 65, 90, 150, 0x60FFFFFF, 36);
    y = 82;
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(200, 62, shop->page, shop->pages, 1);
    Font_SetSpacing(17, 17);
    Font_SetPos(40, y);
    for (i = 0; i < EQUIP_SHOP_LINES; i++) {
        entry = &shop->line[i];
        if (entry->item.id == 0) {
            break;
        }
        if (Equip_TestFlag(&entry->item, EQUIP_FLAG_NEW) == 1 && (gFrameTick & 0x10)) {
            Icon_Draw(20, y, 16, 20, 0xFFC03000, 99);
        } else {
            Lobby_DrawEquipIcon(14, y, 20, 20, &entry->item);
        }
        if (i == shop->cursor) {
            blink = (gFrameTick & 0x10) ? (s16)0 : (s16)6;
            Icon_Draw(32, y + 12, 248, 7, 0x99106060, 111);
            Menu_DrawCursorIcon(-2 - blink, y - 2, 22, 0xFF20C0C0);
        }
        if (entry->noBuy != 0) {
            Font_SetColor(0x8042352E);
        } else {
            Font_SetColor(0x80101010);
        }
        Text_PrintfColored("%-20s", Equip_GetName(entry->item.type, entry->item.id));
        Font_SetPos(218, y);
        Text_PrintfColored("%6d%s", entry->price, gLobbyWords[LOBBY_WORD_ZENNY]);
        Font_SetPos(40, y += 20);
    }
}

extern s16 gMenuOffsetX;
extern s16 gMenuOffsetY;
void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: the definition takes s32 coordinates and an s16 kind; with those the calls convert the arguments differently from the game's code */
s16 Lobby_DrawButtonIcon(s16, s16, s32);

/* The preview window at (gMenuOffsetX, gMenuOffsetY) with a gradient;
 * mode 0 (the preview on) adds a header with the move and zoom buttons,
 * mode 1 (off) a line saying select shows the piece. */
void Equip_DrawPreviewHelp(u32 color, s8 mode)
{
    PrimGradRect box;
    s16 x;
    s16 y;

    box.x0 = gMenuOffsetX + 310;
    box.y0 = gMenuOffsetY + 54;
    box.x1 = box.x0 + 306;
    box.y1 = box.y0 + 150;
    box.color[0] = gHeaderBoxColors[0];
    box.color[1] = gHeaderBoxColors[1];
    box.color[2] = gHeaderBoxColors[2];
    box.color[3] = gHeaderBoxColors[3];
    Font_SetColor(0x807F7F7F);
    if (mode == 0) {
        x = gMenuOffsetX + 310;
        y = gMenuOffsetY + 24;
        Win_Draw(x, y, 154, 20, color, 0xC0202020, 6);
        Lobby_DrawButtonIcon(x + 4, y - 1, 15);
        Lobby_DrawButtonIcon(x + 76, y - 1, 14);
        Font_SetSpacing(17, 17);
        Font_SetPos(x + 28, y + 1);
        Text_PrintfColored(gEquipShopStrFmt, gTownLabels[TOWN_LABEL_MOVE]);
        Font_SetPos(x + 100, y + 1);
        Text_PrintfColored(gEquipShopStrFmt, gTownLabels[TOWN_LABEL_ZOOM]);
    }
    Prim_FillGradRect(&box);
    Win_Draw(gMenuOffsetX + 310, gMenuOffsetY + 54, 306, 150, color, 0, 6);
    if (mode == 1) {
        Font_SetPos(gMenuOffsetX + 380, gMenuOffsetY + 110);
        Font_SetSpacing(18, 18);
        Text_Printf(gForgeTexts[FORGE_TEXT_SELECT_SHOWS_EQUIP]);
    }
}

/* EQUIP_SHOP_VIEW_ITEM's list: the preview window, the small page and the note. */
void EquipShop_DrawList(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    Equip_DrawPreviewHelp(0xBDB59F6C, shop->previewOff);
    if (shop->previewOff == 0) {
        EquipPreview_Hide(0);
    }
    EquipShop_DrawPageSmall();
    EquipShop_DrawNote();
}

/* EQUIP_SHOP_VIEW_COMPARE: the small page and the comparison with the piece worn. */
void EquipShop_DrawPreview(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    ShopLine *entry = &shop->line[shop->cursor];

    EquipShop_DrawPageSmall();
    if (entry->item.type != EQUIP_BLADE && entry->item.type != EQUIP_GUN) {
        EquipCompare_Draw(&shop->worn, &entry->item, 310, 54, shop->detailPage | 0x50);
    } else {
        EquipCompare_Draw(&shop->worn, &entry->item, 310, 54, shop->detailPage | 0x40);
    }
    EquipShop_DrawNote();
}

/* EQUIP_SHOP_VIEW_SKILLS: the small page and the hunter's skills with the piece. */
void EquipShop_DrawPageDetail(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    EquipShop_DrawPageSmall();
    EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, 58, shop->detailPage);
}

/* The title: the chosen tab's kind. */
void EquipShop_DrawTitle(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 160, 0xFFB0B000, gEquipGroupTexts[shop->tab * EQUIP_SHOP_KINDS + shop->kind], 2, 1);
}

/* Draws the list by the view (enum EquipShopView). */
void EquipShop_Draw(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    ShopLine *entry = &shop->line[shop->cursor];

    EquipShop_DrawTitle();
    Menu_DrawMoney(480, 16, 0);
    switch (shop->view) {
    case EQUIP_SHOP_VIEW_PAGE:
        EquipShop_DrawPage(shop->win.pos + 12, 52);
        break;
    case EQUIP_SHOP_VIEW_ITEM:
        EquipShop_DrawList();
        EquipWindow_DrawLocal(&entry->item, 310, 216, shop->detailPage | 0x40, NULL);
        break;
    case EQUIP_SHOP_VIEW_COMPARE:
        EquipShop_DrawPreview();
        break;
    case EQUIP_SHOP_VIEW_SKILLS:
        EquipShop_DrawPageDetail();
        break;
    }
}

/* Draws the purchase's question: the message, the yes/no (or wear now) and
 * the comparison with the piece worn. */
void EquipShop_DrawItem(void)
{
    EquipShop *shop = (EquipShop *)gMenuWork;

    EquipShop_DrawTitle();
    Menu_DrawMoney(480, 16, 0);
    if (shop->msg != NULL) {
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xC0101010, shop->msg);
    }
    if ((s8)shop->step == 2) {
        Font_SetPos(324, 404);
        Text_Printf(gEquipNowTexts[gLobbyMenu->yesNo]);
    } else {
        Font_SetPos(432, 404);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
    }
    EquipCompare_Draw(&shop->worn, &shop->item, 310, 54, shop->detailPage | 0x50);
}


/* For armour, who can wear the piece when not everyone can: one of
 * gEquipLimitTexts by Equip_GetWearMask's bits 0-1 (the genders, as
 * EquipShop_IsListed tests them) and 2-3. */
void Equip_DrawWearer(EquipItem *piece)
{
    u8 flags;
    s32 limitText;

    if (piece->type != EQUIP_GUN && piece->type != EQUIP_BLADE) {
        flags = Equip_GetWearMask(&gSave, piece);
        limitText = 255;
        if ((flags & 3) != 3) {
            if (flags & 1) {
                switch (flags & 0xC) {
                case 4:
                    limitText = 7;
                    break;
                case 8:
                    limitText = 5;
                    break;
                default:
                    limitText = 3;
                    break;
                }
            } else {
                switch (flags & 0xC) {
                case 4:
                    limitText = 6;
                    break;
                case 8:
                    limitText = 4;
                    break;
                default:
                    limitText = 2;
                    break;
                }
            }
        } else if ((flags & 0xC) != 12) {
            if (flags & 4) {
                limitText = 1;
            } else {
                limitText = 0;
            }
        }
        if (limitText != 255) {
            Font_SetColor(0x807F7F28);
            Text_PrintfColored("%-16s", gEquipLimitTexts[limitText]);
        }
    }
}


/* The box's note (another when the piece is in an equipment set) and the
 * box's list, with the piece's detail when it shows. */
void EquipBox_DrawSellNote(s16 x, s16 y)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s32 slot = shop->col + (shop->row * BOX_PAGE_COLS + shop->page * BOX_PAGE_SLOTS);

    Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xC0101010, (u8)(s32)EquipSet_RemoveSlot(slot, 1) ? gEquipShopTexts[EQUIP_SHOP_TEXT_PICK_SELL_IN_SET] : gEquipShopTexts[EQUIP_SHOP_TEXT_PICK_SELL]);
    if (shop->view != 0) {
        BoxMenu_DrawItemList(slot, (shop->detailPage & 7) | 0x10002F8, x + 12);
    } else {
        BoxMenu_DrawItemList(slot, 0x1000290, x + 12);
        Font_SetSpacing(18, 18);
        Font_SetColor(0x807F7F7F);
        Font_SetPos(510, 404);
        Text_Printf(gGameMenuLabels[MENU_LABEL_DETAILS_GUIDE]);
    }
}

/* The sale's question and the box's list. */
void EquipBox_DrawSellPrice(s16 x, s16 y)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s32 slot = shop->col + (shop->row * BOX_PAGE_COLS + shop->page * BOX_PAGE_SLOTS);

    Equip_GetSellPrice(&gSave.box[slot]);
    if (shop->msg != NULL) {
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xC0101010, shop->msg);
    }
    Font_SetPos(432, 404);
    Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
    if (shop->view != 0) {
        BoxMenu_DrawItemList(slot, (shop->detailPage & 7) | 0x10002F8, x + 12);
    } else {
        BoxMenu_DrawItemList(slot, 0x1000290, x + 12);
    }
}

/* The equipment shop's draw task, by the shop's state. */
void EquipShop_DrawTask(u8 *task)
{
    EquipShop *shop = (EquipShop *)gMenuWork;
    s16 x;

    Text_SetSize(*(s32 *)(task + 24));
    x = shop->win.pos;
    switch (shop->state) {
    case EQUIP_SHOP_STATE_CHOOSE:
        EquipShop_DrawMenu();
        break;
    case EQUIP_SHOP_STATE_OPEN:
        if (shop->choice == EQUIP_SHOP_BUY) {
            EquipShop_DrawFrame(x, 52);
        } else {
            EquipBox_DrawSellNote(x + 14, 52);
        }
        break;
    case EQUIP_SHOP_STATE_LIST:
        EquipShop_Draw();
        break;
    case EQUIP_SHOP_STATE_BUY:
        EquipShop_DrawItem();
        break;
    case EQUIP_SHOP_STATE_BOX:
        EquipBox_DrawSellNote(x + 14, 52);
        break;
    case EQUIP_SHOP_STATE_SELL:
        EquipBox_DrawSellPrice(x + 14, 52);
        break;
    }
}

/* The data. */
u8 gEquipShopTypesTab0[8] = {
#include "lobby/007F1858.inc"
};
u8 gEquipShopTypesTab1[16] = {
#include "lobby/007F1860.inc"
};
u32 gHeaderBoxColors[4] = {
#include "lobby/007F1870.inc"
};
void *gPetHandlers[8] = {
    Pet_SpawnShadow, Pet_SpawnShadow, Pet_SpawnShadow, Pet_RunMotionFx,
    Pet_SpawnShadow, NULL, NULL, NULL,
};
void *gServiceNpcHandlers[8] = {
    ServiceNpc_SpawnShadow, ServiceNpc_SpawnShadow, ServiceNpc_NopHandler, ServiceNpc_RunMotionFx,
    ServiceNpc_NopHandler, NULL, NULL, NULL,
};
void *gStationedNpcHandlers[8] = {
    StationedNpc_SpawnShadow, StationedNpc_SpawnShadow, StationedNpc_NopHandler, StationedNpc_RunMotionFx,
    StationedNpc_NopHandler, NULL, NULL, NULL,
};
f32 gShadowSizesLobby[8] = {
#include "lobby/007F18E0.inc"
};
/* The lobby's own monster hit tables, which the push-apart and camera code
 * read while gScene.inLobby is set in place of the game's gEmBoneHits and
 * gEmPushSpheres: every one of the four kinds has the same one-capsule bone
 * list and the same one-sphere push list ({x, y, z, radius}; a radius of -1
 * ends it). */
HitShape gEmBoneHitsLobbyList[2] = {
#include "lobby/007F1900.inc"
};
HitShape *gEmBoneHitsLobby[4] = {
#include "lobby/007F1950.inc"
};
f32 gEmPushSpheresLobbyList[8] = {
#include "lobby/007F1960.inc"
};
f32 *gEmPushSpheresLobby[4] = {
#include "lobby/007F1980.inc"
};
