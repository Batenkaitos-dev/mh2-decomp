/*
 * The records menu's update (TOWN_MODE_RECORDS): Records_Update runs its
 * modes (the window sliding in and out, the menu of record kinds, then one
 * screen per kind), and the input of each screen: the equipment genres and
 * lists with the piece's 3D preview, the score and monster pages, the
 * guides' categories and pages, the titles (one may be taken as the
 * hunter's title) and the items by category. Each screen's input returns
 * MENU_RUNNING, or MENU_CANCEL when the player leaves it. The state is the
 * RecordMenu view of gMenuWork; the owner's records arrive by the
 * Records_Recv* functions, and Records_Draw draws the screens.
 *
 * This unit was built at optimization level 4.
 */
#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_vobj.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "bgm.h"
#include "chat_box.h"
#include "lobby_net.h"
#include "records_menu.h"
#include "equip_shop.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "my_house.h"
#include "records_draw.h"
#include "records_lists.h"
#include "libc.h"
#include "window.h"
#pragma optimization_level 4

extern s16 gMenuOffsetY;
extern s16 gMenuOffsetX;
extern s32 gTownMode;
extern s32 gTownMenuOpen;

/* Closes the records menu: back to TOWN_MODE_FREE, with no draw callbacks
 * and the menus' offsets cleared. */
void Records_Close(void)
{
    gTownMenuOpen = 0;
    gTownMode = TOWN_MODE_FREE;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gMenuOffsetY = 0;
    gMenuOffsetX = 0;
}

extern u8 gMenuWork[];
extern u8 gHouseAccess;
extern char gHousePassword[];

/*
 * The records menu's frame (TOWN_MODE_RECORDS): runs RecordMenu.mode
 * (enum RecordsMode). On the menu (Records_MenuInput), a kind chosen
 * slides the window out and prepares its screen: the items, weapons,
 * armour and titles note in eventMark whether their record count's event
 * word (gSave.recordCounts[RECORD_EVENT_ITEMS], _WEAPONS, _ARMOR, _TITLES)
 * is 1, and tally; the
 * house's access copies the current setting. Once the window is out the
 * kind's screen runs, and when its input returns MENU_DONE or MENU_CANCEL
 * the window slides back in to the menu. Cancelling the menu closes it.
 */
void Records_Update(void)
{
    RecordMenu *menu = (RecordMenu *)gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    switch (menu->mode) {
    case RECORDS_MODE_OPEN:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->mode++;
            break;
        }
        break;
    case RECORDS_MODE_MENU:
        switch (Records_MenuInput(menu, pad)) {
        case MENU_DONE:
            menu->mode++;
            menu->sel = 0;
            menu->page = 0;
            menu->toggle = 0;
            WinSlide_Step(&menu->win, 280, RECORDS_EQUIP_BITS);
            menu->eventMark = 0;
            switch (menu->kind[menu->menuSel]) {
            case RECORD_KIND_ITEMS:
                if (gSave.recordCounts[RECORD_EVENT_ITEMS] == 1) {
                    menu->eventMark = 1;
                }
                Records_TallyItems(menu);
                break;
            case RECORD_KIND_WEAPONS:
                if (gSave.recordCounts[RECORD_EVENT_WEAPONS] == 1) {
                    menu->eventMark = 1;
                }
                Records_EquipInit(menu);
                break;
            case RECORD_KIND_ARMOR:
                if (gSave.recordCounts[RECORD_EVENT_ARMOR] == 1) {
                    menu->eventMark = 1;
                }
                Records_EquipInit(menu);
                break;
            case RECORD_KIND_TITLES:
                if (gSave.recordCounts[RECORD_EVENT_TITLES] == 1) {
                    menu->eventMark = 1;
                }
                Records_TallyTitles(menu);
                break;
            case RECORD_KIND_HOUSE_ACCESS:
                menu->houseAccess = gHouseAccess;
                memset(menu->password, 0, HOUSE_PASSWORD_SIZE);
                memcpy(menu->password, gHousePassword, HOUSE_PASSWORD_SIZE);
                break;
            case RECORD_KIND_GUIDES:
                Records_GuidesInit(menu);
                break;
            case RECORD_KIND_MONSTERS:
                Records_MonstersInit(menu);
                break;
            case RECORD_KIND_SCORES_A:
                menu->pages = 5;
                break;
            case RECORD_KIND_SCORES_B:
                menu->pages = 4;
                break;
            }
            break;
        case MENU_CANCEL:
            Records_Close();
            break;
        }
        break;
    case RECORDS_MODE_TO_PAGE:
        switch (WinSlide_Step(&menu->win, 280, 640)) {
        case MENU_DONE:
            switch (menu->kind[menu->menuSel]) {
            case RECORD_KIND_HOUSE_ACCESS:
                menu->mode = RECORDS_MODE_HOUSE_ACCESS;
                break;
            case RECORD_KIND_ITEMS:
                menu->mode = RECORDS_MODE_ITEMS;
                break;
            case RECORD_KIND_WEAPONS:
            case RECORD_KIND_ARMOR:
                menu->mode = RECORDS_MODE_EQUIP;
                break;
            case RECORD_KIND_TITLES:
                menu->mode = RECORDS_MODE_TITLES;
                break;
            case RECORD_KIND_GUIDES:
                menu->mode = RECORDS_MODE_GUIDES;
                break;
            case RECORD_KIND_MONSTERS:
                menu->mode = RECORDS_MODE_MONSTERS;
                break;
            case RECORD_KIND_SCORES_A:
            case RECORD_KIND_SCORES_B:
                menu->mode = RECORDS_MODE_SCORES;
                break;
            }
            break;
        }
        break;
    case RECORDS_MODE_BACK:
        switch (WinSlide_Step(&menu->win, 640, 280)) {
        case MENU_DONE:
            menu->mode = RECORDS_MODE_MENU;
            break;
        }
        break;
    case RECORDS_MODE_EQUIP:
        Chat_SetWindowMode(2, 2);
        switch (Records_EquipInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = RECORDS_MODE_BACK;
            WinSlide_Step(&menu->win, 640, 280);
            break;
        }
        break;
    case RECORDS_MODE_SCORES:
        switch (Records_ScoresInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = RECORDS_MODE_BACK;
            WinSlide_Step(&menu->win, 640, 280);
            break;
        }
        break;
    case RECORDS_MODE_MONSTERS:
        switch (Records_MonstersInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = RECORDS_MODE_BACK;
            WinSlide_Step(&menu->win, 640, 280);
            break;
        }
        break;
    case RECORDS_MODE_GUIDES:
        switch (Records_GuidesInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = RECORDS_MODE_BACK;
            WinSlide_Step(&menu->win, 640, 280);
            break;
        }
        break;
    case RECORDS_MODE_TITLES:
        switch (Records_TitlesInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = RECORDS_MODE_BACK;
            WinSlide_Step(&menu->win, 640, 280);
            break;
        }
        break;
    case RECORDS_MODE_ITEMS:
        switch (Records_ItemsInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = RECORDS_MODE_BACK;
            WinSlide_Step(&menu->win, 640, 280);
            break;
        }
        break;
    case RECORDS_MODE_HOUSE_ACCESS:
        switch (Records_HouseAccessInput(menu, pad)) {
        case MENU_DONE:
        case MENU_CANCEL:
            menu->mode = RECORDS_MODE_BACK;
            WinSlide_Step(&menu->win, 640, 280);
            break;
        }
        break;
    }
}

extern LobbyMenu *gLobbyMenu;

/*
 * The equipment screen (the weapons or the armour): step 0 the genres or
 * armour types, then the list. Confirming packs the owner's flags of the
 * type when the player owns the house (or plays offline) and shows the
 * first page; otherwise it asks the owner (RECORDS_REQ_EQUIP), shows the
 * wait text and waits up to 480 frames for Records_RecvEquipBits (step
 * RECORDS_STEP_WAIT), then shows that the owner didn't answer (step 3).
 * Returns MENU_CANCEL when left, else MENU_RUNNING.
 */
s32 Records_EquipInput(RecordMenu *menu, u16 pad)
{
    s32 request;

    switch (menu->step) {
    case 0:
        if (pad & PAD_CIRCLE) {
            menu->sel = 0;
            menu->toggle = 0;
            EquipPreview_Reset();
            if (Game_IsOnline() == 0 || (u8)gLobby->memberSlot[gScene.localPlayer] == gScene.localPlayer + '0') {
                menu->step = 2;
                if (menu->kind[menu->menuSel] == RECORD_KIND_WEAPONS) {
                    if (menu->genre != RECORDS_GENRE_GUNS) {
                        menu->equipType = EQUIP_BLADE;
                        Bits_Pack(menu->equipBits, gSave.bladeFlags, RECORDS_EQUIP_BITS);
                    } else {
                        menu->equipType = EQUIP_GUN;
                        Bits_Pack(menu->equipBits, gSave.gunFlags, RECORDS_EQUIP_BITS);
                    }
                } else {
                    switch (menu->genre) {
                    case RECORDS_GENRE_GREAT_SWORDS:
                        menu->equipType = EQUIP_HEAD;
                        Bits_Pack(menu->equipBits, gSave.armorFlags[0], RECORDS_EQUIP_BITS);
                        break;
                    case RECORDS_GENRE_SMALL_SWORDS:
                        menu->equipType = EQUIP_BODY;
                        Bits_Pack(menu->equipBits, gSave.armorFlags[1], RECORDS_EQUIP_BITS);
                        break;
                    case RECORDS_GENRE_HAMMERS:
                        menu->equipType = EQUIP_ARMS;
                        Bits_Pack(menu->equipBits, gSave.armorFlags[2], RECORDS_EQUIP_BITS);
                        break;
                    case RECORDS_GENRE_LANCES:
                        menu->equipType = EQUIP_WAIST;
                        Bits_Pack(menu->equipBits, gSave.armorFlags[3], RECORDS_EQUIP_BITS);
                        break;
                    case RECORDS_GENRE_GUNS:
                        menu->equipType = EQUIP_LEGS;
                        Bits_Pack(menu->equipBits, gSave.armorFlags[4], RECORDS_EQUIP_BITS);
                        break;
                    }
                }
                Records_SetEquipPage(menu, 0);
            } else {
                menu->step++;
                LobbyText_Open(LOBBY_TEXT_PLEASE_WAIT); /* asking the owner: wait */
                gLobbyMenu->redraw = 1;
                memset(menu->equipBits, 0, RECORDS_EQUIP_BITS);
                if (menu->kind[menu->menuSel] == RECORD_KIND_WEAPONS) {
                    if (menu->genre != RECORDS_GENRE_GUNS) {
                        request = RECORDS_REQ_EQUIP(EQUIP_BLADE);
                        menu->equipType = EQUIP_BLADE;
                    } else {
                        request = RECORDS_REQ_EQUIP(EQUIP_GUN);
                        menu->equipType = EQUIP_GUN;
                    }
                } else {
                    switch (menu->genre) {
                    case RECORDS_GENRE_GREAT_SWORDS:
                    default:
                        menu->equipType = EQUIP_HEAD;
                        request = RECORDS_REQ_EQUIP(EQUIP_HEAD);
                        break;
                    case RECORDS_GENRE_SMALL_SWORDS:
                        menu->equipType = EQUIP_BODY;
                        request = RECORDS_REQ_EQUIP(EQUIP_BODY);
                        break;
                    case RECORDS_GENRE_HAMMERS:
                        menu->equipType = EQUIP_ARMS;
                        request = RECORDS_REQ_EQUIP(EQUIP_ARMS);
                        break;
                    case RECORDS_GENRE_LANCES:
                        menu->equipType = EQUIP_WAIST;
                        request = RECORDS_REQ_EQUIP(EQUIP_WAIST);
                        break;
                    case RECORDS_GENRE_GUNS:
                        menu->equipType = EQUIP_LEGS;
                        request = RECORDS_REQ_EQUIP(EQUIP_LEGS);
                        break;
                    }
                }
                TownMsg_SendRecordsAsk(request);
                menu->reqKind = request;
                menu->timer = 480; /* frames: 8 s */
            }
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_CANCEL;
        } else {
            menu->genre = Menu_WrapUpDown(menu->genre, menu->genreCount);
        }
        break;
    case 1:
        gLobbyMenu->redraw = 1;
        if (--menu->timer <= 0) {
            menu->step = 3;
            LobbyText_Open(121); /* the owner didn't answer */
        }
        break;
    case 2:
        switch (Records_EquipListInput(menu, pad)) {
        case 3:
            menu->step = 0;
            break;
        }
        break;
    case 3:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            menu->step = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        }
        break;
    }
    return MENU_RUNNING;
}

/* unit's own declaration: the type and the gender as bytes (the definition takes u16 kind, u16 id, u16 female); the caller converts its arguments this way, as the game's code does */
s32 Equip_GetModelId(u8 type, u16 id, u8 x);
/* unit's own declaration: the type as a byte, the id and the gender as words (the definition takes u16 kind, u16 id, u8 female); the caller converts its arguments this way, as the game's code does */
void EquipPreview_Load(u8 type, s32 n, s32 x);
extern NetUser gNetUsers[];

/* The house owner's gender, as their hunter card in gNetUsers gives it. */
static inline u8 Records_OwnerGender(void)
{
    return ((HunterCard *)((NetUser *)&gNetUsers[((Player *)&gPlayers[MyHouse_LocalSlot()])->index])->text)->gender;
}

/*
 * The equipment list: PAD_SQUARE shows or hides the piece's 3D preview
 * and its level line (RecordMenu.toggle), PAD_R1 and PAD_L1 step through
 * the levels while it shows, left and right turn the page, up and down
 * move the cursor, PAD_CROSS hides the preview or leaves the list.
 * Returns MENU_CANCEL when left, else MENU_RUNNING.
 */
s32 Records_EquipListInput(RecordMenu *menu, u16 pad)
{
    EquipItem item;
    s32 ownerGender;

    if (pad & PAD_CROSS) {
        if (menu->toggle == 0) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            EquipPreview_Free();
            return MENU_CANCEL;
        }
        EquipPreview_Free();
        EquipPreview_Hide(1);
        menu->toggle = 0;
        LobbySe_Play(LOBBY_SE_CANCEL);
    } else if (pad & PAD_SQUARE) {
        if (menu->rowCount != 0) {
            if (menu->toggle == 0) {
                if (Game_IsOnline() == 0) {
                    ownerGender = gSave.gender;
                } else {
                    ownerGender = Records_OwnerGender();
                }
                item = menu->items[menu->sel];
                if (Equip_IsNoModelArmor(&item) == 0) {
                    EquipPreview_Load(item.type, Equip_GetModelId(item.type, item.id, ownerGender), ownerGender);
                    EquipPreview_Hide(0);
                }
                menu->subCursor = 0;
                menu->levelCount = Equip_LevelMark(&item, 64, 0);
                menu->toggle = 1;
            } else {
                EquipPreview_Free();
                EquipPreview_Hide(1);
                menu->toggle = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_R1) {
        if (menu->toggle != 0) {
            if (menu->levelCount > 1) {
                if (++menu->subCursor >= menu->levelCount) {
                    menu->subCursor = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        }
    } else if (pad & PAD_L1) {
        if (menu->toggle != 0) {
            if (menu->levelCount > 1) {
                if (--menu->subCursor < 0) {
                    menu->subCursor = menu->levelCount - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (menu->toggle == 0) {
            if (menu->pages > 1) {
                menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
                Records_SetEquipPage(menu, menu->page);
            }
        }
    } else if (menu->toggle == 0) {
        menu->sel = Menu_WrapUpDown(menu->sel, menu->rowCount);
    }
    return MENU_RUNNING;
}

/* The score pages: left and right turn the page; PAD_CROSS leaves
 * (MENU_CANCEL), else MENU_RUNNING. */
s32 Records_ScoresInput(RecordMenu *menu, u16 pad)
{
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        return MENU_CANCEL;
    }
    menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
    return MENU_RUNNING;
}

/* Starts the monster pages at the first; the page count comes from the
 * monsters recorded. */
void Records_MonstersInit(RecordMenu *menu)
{
    menu->page = 0;
    menu->pages = Records_CountHuntPages((u8 *)menu);
}

/* The monster pages: left and right turn the page; PAD_CROSS leaves
 * (MENU_CANCEL), else MENU_RUNNING. */
s32 Records_MonstersInput(RecordMenu *menu, u16 pad)
{
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        return MENU_CANCEL;
    }
    if (pad & (PAD_LEFT | PAD_RIGHT)) {
        menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
    }
    return MENU_RUNNING;
}

/* Fills rows with page `page` (RECORDS_GUIDE_ROWS rows) of the guide
 * category chosen (gRecordGuideCats[sel]); sets page and pages and keeps the
 * cursor on a row. */
void Records_SetGuidePage(RecordMenu *menu, s16 page)
{
    RecordCat *cat = &gRecordGuideCats[menu->sel];
    s16 first = page * RECORDS_GUIDE_ROWS;
    s16 last = first + RECORDS_GUIDE_ROWS - 1;
    u8 *entry = cat->list;
    s16 i;

    menu->rowCount = 0;
    for (i = 0; i < cat->count; i++, entry += sizeof(RecordEntry)) {
        if (i >= first && i <= last) {
            menu->rows[menu->rowCount] = entry;
            menu->rowCount++;
        }
    }
    menu->page = page;
    menu->pages = Menu_CountPages(cat->count, RECORDS_GUIDE_ROWS);
    if (menu->cursor >= menu->rowCount) {
        menu->cursor = menu->rowCount - 1;
    }
}

/* The guides need nothing prepared: the menu calls this empty function. */
void Records_GuidesInit(RecordMenu *menu)
{

}

/*
 * The guides: step 0 the categories, 1 the entries of one (pages of
 * RECORDS_GUIDE_ROWS), 2 an entry's text, whose pages left and right
 * turn. The entry's page count is read as a word here, where the draws
 * read it as RecordEntry's s16, as the game's code does. Returns
 * MENU_CANCEL when left from the categories, else MENU_RUNNING.
 */
s32 Records_GuidesInput(RecordMenu *menu, u16 pad)
{
    switch (menu->step) {
    case 0:
        if (pad & PAD_CIRCLE) {
            menu->step++;
            menu->cursor = 0;
            Records_SetGuidePage(menu, 0);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            menu->step = 0;
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_CANCEL;
        } else {
            menu->sel = Menu_WrapUpDown(menu->sel, RECORD_GUIDE_COUNT);
        }
        break;
    case 1:
        if (pad & PAD_CIRCLE) {
            menu->step++;
            menu->subCursor = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            menu->step--;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
            Records_SetGuidePage(menu, menu->page);
        } else {
            menu->cursor = Menu_WrapUpDown(menu->cursor, menu->rowCount);
        }
        break;
    case 2:
        if (pad & PAD_CROSS) {
            menu->step--;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            menu->subCursor = Menu_WrapLeftRight(menu->subCursor, *(s32 *)&((RecordEntry *)menu->rows[menu->cursor])->pageCount);
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * The titles: step 0 the HUNTER_TITLE_GROUPS tabs, 1 the titles earned in
 * one (PAD_SQUARE shows a title's description), 2 a yes/no to take the
 * title chosen as the hunter's title (gSave.title), which only the house
 * owner may, and only for an earned title. Returns MENU_CANCEL when left
 * from the tabs, else MENU_RUNNING.
 */
s32 Records_TitlesInput(RecordMenu *menu, u16 pad)
{
    switch (menu->step) {
    case 0:
        if (pad & PAD_CIRCLE) {
            menu->step++;
            menu->cursor = 0;
            Records_SetTitlePage(menu, 0);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            menu->step = 0;
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_CANCEL;
        } else {
            menu->sel = Menu_WrapUpDown(menu->sel, HUNTER_TITLE_GROUPS);
        }
        break;
    case 1:
        if (pad & PAD_CIRCLE) {
            if (Game_IsOnline() != 0 && (u8)gLobby->memberSlot[gScene.localPlayer] != gScene.localPlayer + '0') {
                break;
            }
            if (menu->rowCount != 0) {
                if (Bits32_Test(menu->titleBits, menu->ids[menu->cursor]) != 0) {
                    menu->step++;
                    Menu_SetYesNo(0);
                    LobbySe_Play(LOBBY_SE_CONFIRM);
                } else {
                    LobbySe_Play(LOBBY_SE_REFUSE);
                }
            }
        } else if (pad & PAD_CROSS) {
            menu->step = 0;
            LobbySe_Play(LOBBY_SE_CANCEL);
        } else if (pad & PAD_SQUARE) {
            if (menu->rowCount != 0) {
                menu->toggle ^= 1;
                LobbySe_Play(LOBBY_SE_DESCRIPTION);
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
            Records_SetTitlePage(menu, menu->page);
        } else {
            menu->cursor = Menu_WrapUpDown(menu->cursor, menu->rowCount);
        }
        break;
    case 2:
        switch (LobbyMenu_YesNoInput(58 /* take this title? */)) {
        case 0:
            menu->step--;
            gSave.title = menu->ids[menu->cursor];
            break;
        case 3:
            menu->step--;
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

/* Fills ids with page `page` (RECORDS_PAGE_ROWS rows) of the titles
 * earned in tab sel; sets page and pages and keeps the cursor on a row. */
void Records_SetTitlePage(RecordMenu *menu, s16 page)
{
    s16 i;
    s16 shown;
    HunterTitle *title;
    s16 first = page * RECORDS_PAGE_ROWS;
    s16 last = first + RECORDS_PAGE_ROWS - 1;

    menu->rowCount = 0;
    for (i = 0, shown = 0, title = gHunterTitles; i < HUNTER_TITLE_COUNT; i++, title++) {
        if (title->group == menu->sel && Bits32_Test(menu->titleBits, i) == 1) {
            if (shown >= first && shown <= last) {
                menu->ids[menu->rowCount] = i;
                menu->rowCount++;
            }
            shown++;
        }
    }
    menu->page = page;
    menu->pages = Menu_CountPages(shown, RECORDS_PAGE_ROWS);
    if (menu->cursor >= menu->rowCount) {
        menu->cursor = menu->rowCount - 1;
    }
}

/* The same for the items obtained of category sel + 1. id is a u16 copy
 * of i: the game's code keeps the item id in a register of its own. */
void Records_SetItemPage(RecordMenu *menu, s16 page)
{
    s16 i;
    s16 shown;
    ItemData *itemData;
    s16 first = page * RECORDS_PAGE_ROWS;
    s16 last = first + RECORDS_PAGE_ROWS - 1;
    s16 cat;
    u16 id;

    menu->rowCount = 0;
    cat = menu->sel + 1;
    for (i = 0, shown = 0, itemData = gItemData; i < ITEM_ID_COUNT; i++, itemData++) {
        id = i;
        if (cat == itemData->category && ItemBits_Test(menu->itemBits, i) == 1) {
            if (shown >= first && shown <= last) {
                menu->ids[menu->rowCount] = id;
                menu->rowCount++;
            }
            shown++;
        }
    }
    menu->page = page;
    menu->pages = Menu_CountPages(shown, RECORDS_PAGE_ROWS);
    if (menu->cursor >= menu->rowCount) {
        menu->cursor = menu->rowCount - 1;
    }
}

/*
 * The items: step 0 the RECORDS_ITEM_CATEGORIES categories, 1 the items
 * obtained in one; PAD_R1 steps through a decoration's three detail
 * pages. Returns MENU_CANCEL when left from the categories, else
 * MENU_RUNNING.
 */
s32 Records_ItemsInput(RecordMenu *menu, u16 pad)
{
    s16 pageCount;

    switch (menu->step) {
    case 0:
        if (pad & PAD_CIRCLE) {
            menu->step++;
            menu->cursor = 0;
            Records_SetItemPage(menu, 0);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_CROSS) {
            menu->step = 0;
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            return MENU_CANCEL;
        } else {
            menu->sel = Menu_WrapUpDown(menu->sel, RECORDS_ITEM_CATEGORIES);
        }
        break;
    case 1:
        if (!(pad & PAD_CIRCLE)) {
            if (pad & PAD_CROSS) {
                menu->step = 0;
                LobbySe_Play(LOBBY_SE_CANCEL);
            } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
                pageCount = menu->pages;
                if (pageCount > 1) {
                    menu->page = Menu_WrapLeftRight(menu->page, pageCount);
                    Records_SetItemPage(menu, menu->page);
                }
            } else if (pad & PAD_R1) {
                if (menu->rowCount != 0 && Item_IsDeco(menu->ids[menu->cursor]) == 1) {
                    if (++menu->subCursor >= 3) {
                        menu->subCursor = 0;
                    }
                    LobbySe_Play(LOBBY_SE_SCROLL);
                }
            } else {
                menu->cursor = Menu_WrapUpDown(menu->cursor, menu->rowCount);
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/* Counts per title tab the titles earned and those in all (tally[group]
 * [0] and [1]), and resets the cursors. */
void Records_TallyTitles(RecordMenu *menu)
{
    s32 i;
    HunterTitle *title;
    u8 *tallyBase;

    menu->sel = 0;
    menu->subCursor = 0;
    memset(menu->tally, 0, sizeof(menu->tally));
    for (i = 0, title = gHunterTitles; i < HUNTER_TITLE_COUNT; i++, title++) {
        /* tallyBase is the menu plus the group's 8 bytes of tally: the game's code keeps
         * that one base for both counts, which the array form doesn't give */
        tallyBase = (u8 *)menu + title->group * 8;
        (*(s32 *)(tallyBase + 1760))++; /* tally[group][1] */
        if (Bits32_Test(menu->titleBits, i) == 1) {
            (*(s32 *)(tallyBase + 1756))++; /* tally[group][0] */
        }
    }
}

/* Resets the equipment screen: the cursors, the first page, the first of
 * the RECORDS_GENRE_COUNT genres. */
void Records_EquipInit(RecordMenu *menu)
{
    menu->sel = 0;
    menu->subCursor = 0;
    menu->page = 0;
    menu->genre = 0;
    menu->genreCount = RECORDS_GENRE_COUNT;
}
