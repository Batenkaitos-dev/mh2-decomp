/*
 * The game menu (GameMenu, gSubMenuWork): the list PAD_START opens in the town, offline
 * and online. GameMenu_Open opens it (TOWN_MODE_GAME_MENU), GameMenu_Update runs it and
 * GameMenu_Draw draws it. The list depends on where the game is (GameMenu.kind): 0 offline
 * (pouch, combine, status, quest, info, phrases, options, pause, quit), 1 online in
 * AREA_ONLINE_VILLAGE (pouch, combine, status, info, chat, mail, search, options, exit), 2 online
 * elsewhere (the same with the quest page). Each entry opens a page through the tables at
 * the end of the file (gGameMenuOpen0..2, gGameMenuUpdate0..2, gGameMenuDraw0..2, one set
 * per list); a page's update returns PAD_START to close the whole menu or PAD_CROSS to go
 * back to the list. Also here: the pouch and combine pages' updates, the search page
 * (friend list, area, name and id search); the chat page follows in its own unit. The .data holds
 * the tables, the lists' help lines and the two list panels (gGameMenuPanel, gSearchPanel).
 */
#include "common.h"
#include "game_menu.h"
#include "em.h"
#include "scene.h"
#include "player.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "window.h"
#include "area.h"
#include "pad.h"
#include "sound.h"
#include "hud.h"
#include "texture.h"
#include "render_state.h"
#include "lobby_menu.h"
#include "chat_log.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "chat_box.h"
#include "lobby_search.h"
#include "shake.h"
#include "text.h"
#include "village_cond.h"
#include "chat_send.h"
#include "engine_math.h"
#include "friend_list.h"
#include "game_util.h"
#include "id_search.h"
#include "item_shop.h"
#include "lobby_queries.h"
#include "menu_util.h"
#include "name_search.h"
#include "net_scene.h"
#include "town_player_states.h"

/* prototypes */
struct ListPanel;
/* unit's own declaration: the cursor is passed as a plain int here (the definition takes an s16); the search page passes its cursor byte without narrowing it, as the game's code does, so this declaration stays */
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s32 sel);
extern ListPanel gSearchPanel;
/* end of prototypes */

extern s32 gTownMode;

/* Points gSubMenuWork at its memory, the menu closed and the cursor on the first entry. */
void GameMenu_Reset(void)
{
    gSubMenuWork = &gSubMenuWorkMem;
    gSubMenuWork->entry = 0;
    gSubMenuWork->open = 0;
}

extern u8 gGameMenuHelpLines0[];
extern u8 gGameMenuHelpLines1[];

/* Opens the game menu (the town calls it on PAD_START): offline only once condition 1
 * holds (Cond_Check), never while the hunter sits at the house's seat (Area_IsHouse,
 * TownPc_IsSeatedState) or an item pick-up is busy, and not when it is open already.
 * Marks the menu open (gScene.menuOpen), takes the town mode, clears the lobby menu's
 * draw layers, starts the list and plays SE_SYS_OPEN. */
void GameMenu_Open(void)
{
    s32 townMode;
    u8 localPlayer;
    Player *player;

    localPlayer = gScene.localPlayer;
    player = &gPlayers[localPlayer];
    if (Game_IsOnline() != 0 || Cond_Check(1) != 0) {
        if (((Area_IsHouse(gScene.area) != 1) || (TownPc_IsSeatedState(player->stateGroup, player->state) != 1)) && (Hud_ItemGetBusy() != 1)) {
            townMode = gTownMode;
            if (townMode != TOWN_MODE_GAME_MENU) {
                gScene.menuOpen = 1;
                gTownMode = TOWN_MODE_GAME_MENU;
                LobbyMenu_Init();
                LobbyMenu_SetDraw(0, 0, 0, 0);
                GameMenu_Start();
                gSubMenuWork->open = 1;
                gSubMenuWork->pad = 0;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_OPEN, 0);
            }
        }
    }
}

/* Starts the list: which one by where the game is (kind 0 offline with 9 entries, 1
 * online in AREA_ONLINE_VILLAGE with 9, 2 online elsewhere with 10), the help window on the
 * lobby page at the cursor's line, the chat shown, and the list's slide-in. */
void GameMenu_Start(void)
{
    gSubMenuWork->steps = 0;
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_LOBBY;
    if (Game_IsOnline() != 1) {
        gSubMenuWork->kind = GAME_MENU_OFFLINE;
        gSubMenuWork->count = 9;
    } else if (gScene.area == AREA_ONLINE_VILLAGE) {
        gSubMenuWork->kind = GAME_MENU_HUB;
        gSubMenuWork->count = 9;
    } else {
        gSubMenuWork->kind = GAME_MENU_ONLINE;
        gSubMenuWork->count = 10;
    }
    GameMenu_SetHelp();
    gChatHidden = 0;
    Shake_Start(SHAKE_FIELD_MENU, SHAKE_AXIS_X, 180.0f);
    Hud_ResetMenuState();
}

/* Closes the menu: clears open and the steps and hides the help window. */
void GameMenu_Clear(void)
{
    gSubMenuWork->open = 0;
    gSubMenuWork->steps = 0;
    gHelpShow = 0;
}

/* Once the menu is closed, clears it and returns 1. Otherwise stores this frame's
 * buttons in pad (the lobby's buttons when Lobby_GetPauseState() is 2 or more, else pad
 * 0's) and returns 0. */
s32 GameMenu_Poll(void)
{
    if (gSubMenuWork->open == 0) {
        GameMenu_Clear();
        return 1;
    }
    if (Lobby_GetPauseState() >= 2) {
        gSubMenuWork->pad = (u16) gLobby->buttons;
    } else {
        gSubMenuWork->pad = LobbyPad_Get(0);
    }
    return 0;
}

/* The buttons the town reads: the lobby's when Lobby_GetPauseState() is 2 or more, pad 0's
 * while the menu is closed, else the ones GameMenu_Poll stored (0 while a page takes
 * them). */
u16 GameMenu_GetPad(void)
{
    if (Lobby_GetPauseState() >= 2) {
        return gLobby->buttons;
    }
    if (gSubMenuWork->open == 0) {
        return LobbyPad_Get(0);
    }
    return gSubMenuWork->pad;
}

extern s32 (*gGameMenuOpen0[])(void *m);
extern s32 (*gGameMenuOpen1[])(void *m);
extern s32 (*gGameMenuOpen2[])(void *m);
extern u16 (*gGameMenuUpdate0[])(s32 pad);
extern u16 (*gGameMenuUpdate1[])(s32 pad);
extern u16 (*gGameMenuUpdate2[])(s32 pad);
extern u8 gTownAreaMsgPending;

/* The game menu's update. busy is set unless no quest is taken, the item menus are
 * allowed and an item pick-up is ready. Step 0, the list: while it slides in only the
 * D-pad and cross get through; circle opens the entry's page (gGameMenuOpen0..2 by
 * kind), cross or start begin closing (GameMenu_Back). Step 1, the page: its update
 * (gGameMenuUpdate0..2) returns PAD_START to close the menu or PAD_CROSS to go back to
 * the list; either way gLobby->connected is set again if a page cleared it, and the
 * area message is sent once more (gTownAreaMsgPending). Step 2 waits for the list to
 * slide out. Returns 1 when the menu is closed (or not open), else 0. */
s32 GameMenu_Update(u16 pad)
{
    GameMenu *menu;
    s32 refused;
    u16 pageKeys;

    if (gSubMenuWork->open == 0) {
        return 1;
    }
    if (Town_IsMode(TOWN_MODE_GAME_MENU) == 0) {
        return 1;
    }
    gSubMenuWork->pad = 0;
    gSubMenuWork->busy = 1;
    if (Lobby_HasQuest() == 0 && Hud_ItemMenusAllowed() == 1 && ItemGet_IsReady(0) == 1) {
        gSubMenuWork->busy = 0;
    }
    Chat_SetWindowMode(1, 2);
    menu = gSubMenuWork;
    switch (menu->step) {
    case GAME_MENU_LIST:
        if (Shake_Update(SHAKE_FIELD_MENU) == 1) {
            pad &= PAD_DPAD | PAD_CROSS;
        }
        Menu_CursorUpDown(&gSubMenuWork->entry, pad, gSubMenuWork->count);
        GameMenu_SetHelp();
        if (pad & PAD_CIRCLE) {
            Shake_Stop();
            menu = gSubMenuWork;
            switch (menu->kind) {
            case GAME_MENU_OFFLINE:
                refused = gGameMenuOpen0[menu->entry](menu);
                break;
            case GAME_MENU_HUB:
                refused = gGameMenuOpen1[menu->entry](menu);
                break;
            case GAME_MENU_ONLINE:
                refused = gGameMenuOpen2[menu->entry](menu);
                break;
            }
            if (refused == 0) {
                gSubMenuWork->step++;
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
            } else {
                Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_REFUSE, 0);
            }
        } else if (pad & (PAD_START | PAD_CROSS)) {
            GameMenu_Back();
        }
        break;
    case GAME_MENU_PAGE:
        switch (menu->kind) {
        case GAME_MENU_OFFLINE:
            pageKeys = gGameMenuUpdate0[menu->entry](pad);
            break;
        case GAME_MENU_HUB:
            pageKeys = gGameMenuUpdate1[menu->entry](pad);
            break;
        case GAME_MENU_ONLINE:
            pageKeys = gGameMenuUpdate2[menu->entry](pad);
            break;
        }
        if (pageKeys & PAD_START) {
            gSubMenuWork->open = 0;
            if (gLobby->connected == 0) {
                gLobby->connected = 1;
                gTownAreaMsgPending = 1;
            }
            return 1;
        }
        if (pageKeys & PAD_CROSS) {
            gSubMenuWork->steps = 0;
            gHelpShow = 1;
            gHelpPage = HELP_PAGE_LOBBY;
            GameMenu_SetHelp();
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
            if (gLobby->connected == 0) {
                gLobby->connected = 1;
                gTownAreaMsgPending = 1;
            }
        }
        break;
    case GAME_MENU_CLOSING:
        if (Shake_Update(SHAKE_FIELD_MENU) == 0) {
            gSubMenuWork->open = 0;
            return 1;
        }
        break;
    }
    return 0;
}

/* Leaves the list: the menu slides out (GAME_MENU_CLOSING), the help line goes and
 * SE_SYS_CANCEL plays. */
void GameMenu_Back(void)
{
    gSubMenuWork->step = GAME_MENU_CLOSING;
    gHelpShow = 0;
    Shake_Start(SHAKE_FIELD_MENU, SHAKE_SLIDE, 180.0f);
    Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CANCEL, 0);
}

/* Sets the help line for the list's highlighted entry: from the offline list's or the
 * hub's table of lines, or, online elsewhere, the entry's own number. */
void GameMenu_SetHelp(void)
{
    u8 menuKind;

    menuKind = gSubMenuWork->kind;
    switch (menuKind) {
    case GAME_MENU_OFFLINE:
        gHelpLine = (s16) *(gGameMenuHelpLines0 + gSubMenuWork->entry);
        break;
    case GAME_MENU_HUB:
        gHelpLine = (s16) *(gGameMenuHelpLines1 + gSubMenuWork->entry);
        break;
    case GAME_MENU_ONLINE:
        gHelpLine = (s16) gSubMenuWork->entry;
        break;
    }
}

s32 GameMenu_IsBusy(void)
{
    return gSubMenuWork->busy != 0;
}

typedef void (*GameMenuDrawFunc)(void);
extern GameMenuDrawFunc gGameMenuDraw0[];
extern GameMenuDrawFunc gGameMenuDraw1[];
extern GameMenuDrawFunc gGameMenuDraw2[];

/* The game menu's frame (by step): the list, the open page's draw function
 * by kind and entry (the tables are called without arguments; the exit entry, 8 of
 * list 1 and 9 of list 2, has none), or the list dimmed while closing. The help window
 * goes over them once the list has stopped sliding. */
void GameMenu_Draw(void)
{
    if (gSubMenuWork->open == 0) {
        return;
    }
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gs_SetState(RS_ALPHA_REF, 0);
    switch (gSubMenuWork->step) {
    case GAME_MENU_LIST:
        GameMenu_DrawList(0);
        break;
    case GAME_MENU_PAGE:
        switch (gSubMenuWork->kind) {
        case GAME_MENU_OFFLINE:
            gGameMenuDraw0[gSubMenuWork->entry]();
            break;
        case GAME_MENU_HUB:
            if (gSubMenuWork->entry != 8) {
                gGameMenuDraw1[gSubMenuWork->entry]();
            }
            break;
        case GAME_MENU_ONLINE:
            if (gSubMenuWork->entry != 9) {
                gGameMenuDraw2[gSubMenuWork->entry]();
            }
            break;
        }
        break;
    case GAME_MENU_CLOSING:
        GameMenu_DrawList(1);
        return;
    }
    if (Shake_Value(SHAKE_FIELD_MENU) == 0 && gHelpShow != 0) {
        HelpWindow_DrawCurrent();
    }
}

extern ListPanel gGameMenuPanel;



/* Draws the game menu's list (gGameMenuPanel) at the slide's offset under its title:
 * the current list's texts (the dimmed set while busy, for lists 0 and 2) and its
 * cursor bar, grey when dim is set (under a page, or while closing). The sine of the
 * frame counter's phase is computed and dropped, as in the game's code. The frame
 * counter is read as a named object (gFrameTick): the game loads it above the store to
 * gGameMenuPanel.cursorColor, which MWCC does only for a named one. */
void GameMenu_DrawList(s8 dim)
{
    s32 offset;
    u16 phase;

    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    offset = (s32) Shake_GetOffset(SHAKE_FIELD_MENU);
    if (dim == 0) {
        phase = (gFrameTick & 0x3F) << 10;
        gGameMenuPanel.cursorColor = 0xA9182;
        Math_Sin(0.0000958738f * phase);
    } else {
        gGameMenuPanel.cursorColor = 0x808080;
    }
    switch (gSubMenuWork->kind) {
    case GAME_MENU_OFFLINE:
        if (gSubMenuWork->busy == 0) {
            gGameMenuPanel.items = gGameMenuOfflineTexts;
        } else {
            gGameMenuPanel.items = gGameMenuOfflineBusyTexts;
        }
        break;
    case GAME_MENU_HUB:
        gGameMenuPanel.items = gGameMenuHubTexts;
        break;
    case GAME_MENU_ONLINE:
        if (gSubMenuWork->busy == 0) {
            gGameMenuPanel.items = gGameMenuOnlineTexts;
        } else {
            gGameMenuPanel.items = gGameMenuOnlineBusyTexts;
        }
        break;
    }
    gGameMenuPanel.lines = gSubMenuWork->count;
    ListPanel_Draw(&gGameMenuPanel, (s16 *)offset, gGameMenuLabels[MENU_LABEL_MENU_GUIDE], gSubMenuWork->entry);
}

/* The pouch page's update: PouchMenu_Update, then the pouch is copied into the save. */
s32 GameMenu_UpdateItems(u16 pad)
{
    s32 ret;

    ret = PouchMenu_Update(pad);
    Save_StorePouch(&gPlayers[gScene.localPlayer]);
    return ret;
}

void GameMenu_OpenCombine(void)
{
    Player_CalcSkills(&gPlayers[gScene.localPlayer]);
    CombineMenu_Open();
}

/* The combine page's update: CombineMenu_Input, then the pouch is copied into the save. */
s32 GameMenu_UpdateCombine(u16 pad)
{
    s32 ret;

    ret = CombineMenu_Input(pad);
    Save_StorePouch(&gPlayers[gScene.localPlayer]);
    return ret;
}

/* The search page's open handler: its own list at its first entry, with the lobby help
 * page's search line. Returns 0 (it always opens). */
s32 GameMenu_OpenSearch(void)
{
    gSubMenuWork->subStep = 0;
    gSubMenuWork->subEntry = 0;
    gHelpShow = 1;
    gHelpPage = HELP_PAGE_LOBBY;
    gHelpLine = 28;
    return 0;
}

/* The search page: subStep 0 its list (subEntry, four searches), 1 the
 * chosen search; the page's own input while one is open. Returns the pad, 0 when
 * the menu takes it. */
u16 GameMenu_UpdateSearch(u16 pad)
{
    GameMenu *menu;
    u8 *cursor;

    switch (gSubMenuWork->subStep) {
    case 0:
        if (pad & PAD_START) {
            GameMenu_Back();
            return 0;
        }
        Menu_CursorUpDown(&gSubMenuWork->subEntry, pad, 4);
        menu = gSubMenuWork;
        cursor = &menu->subEntry;
        gHelpLine = menu->subEntry + 28;
        if (pad & PAD_CIRCLE) {
            switch (*cursor) {
            case 0:
                menu->subStep++;
                FriendList_Open(0);
                break;
            case 1:
                menu->subStep++;
                AreaSearch_Open();
                break;
            case 2:
                menu->subStep++;
                NameSearch_Open();
                break;
            case 3:
                menu->subStep++;
                IdSearch_Open();
                break;
            }
            Sound_PlaySeFlat(SE_SLOT_SYSTEM, SE_SYS_CONFIRM, 0);
        }
        break;
    case 1:
        switch (gSubMenuWork->subEntry) {
        case 0:
            switch (FriendList_Update(pad)) {
            case MENU_CANCEL:
            case MENU_UNDONE:
                gSubMenuWork->subStep--;
                return 0;
            case MENU_RUNNING:
                return 0;
            case MENU_CLOSE:
                pad = PAD_START;
                break;
            }
            break;
        case 1:
            switch (AreaSearch_Update(pad)) {
            case MENU_CANCEL:
            case MENU_UNDONE:
                gSubMenuWork->subStep--;
                return 0;
            case MENU_CLOSE:
                pad = PAD_START;
                break;
            case MENU_RUNNING:
                return 0;
            }
            break;
        case 2:
            switch (NameSearch_Update(pad)) {
            case MENU_CANCEL:
            case MENU_UNDONE:
                gSubMenuWork->subStep--;
                return 0;
            case MENU_RUNNING:
                return 0;
            case MENU_CLOSE:
                pad = PAD_START;
                break;
            }
            break;
        case 3:
            switch (IdSearch_Update(pad)) {
            case MENU_CANCEL:
            case MENU_UNDONE:
                gSubMenuWork->subStep--;
                return 0;
            case MENU_RUNNING:
                return 0;
            case MENU_CLOSE:
                pad = PAD_START;
                break;
            }
            break;
        }
        if (pad & PAD_START) {
            GameMenu_Back();
            pad = 0;
        }
        break;
    }
    if (gSubMenuWork->subStep == 0) {
        gHelpShow = 1;
    } else {
        gHelpShow = 0;
    }
    return pad;
}

/* The search page's draw: in its own list step, the game menu's list dimmed and the
 * search list over it; nothing once a search is chosen (step 1). */
void GameMenu_DrawSearch(void)
{
    u8 step;

    step = gSubMenuWork->subStep;
    switch (step) {
    case 0:
        GameMenu_DrawList(1);
        ListPanel_Draw(&gSearchPanel, NULL, NULL, (s32) gSubMenuWork->subEntry);
        
    case 1:
        break;
    }
}

/* The game menu's data: the help lines of lists 0 and 1, the open, update and draw
 * functions of each list's entries (one table per list, NULL past its last entry),
 * and the two list panels. */
s32 PhraseMenu_Open(void *m);
s32 QuestMenu_Open(void *m);
u8 gGameMenuHelpLines0[16] = {
#include "lobby/007F1D20.inc"
};
u8 gGameMenuHelpLines1[16] = {
#include "lobby/007F1D30.inc"
};
s32 (*gGameMenuOpen0[12])(void *m) = {
    PouchMenu_Open, (s32 (*)(void *m))GameMenu_OpenCombine, StatusMenu_Open, QuestMenu_Open,
    InfoMenu_Open, PhraseMenu_Open, OptionMenu_Open, GameMenu_OpenPause,
    GameMenu_OpenQuit, NULL, NULL, NULL,
};
u16 (*gGameMenuUpdate0[12])(s32 pad) = {
    (u16 (*)(s32 pad))GameMenu_UpdateItems, (u16 (*)(s32 pad))GameMenu_UpdateCombine, (u16 (*)(s32 pad))StatusMenu_Update, (u16 (*)(s32 pad))QuestMenu_Update,
    (u16 (*)(s32 pad))InfoMenu_Update, (u16 (*)(s32 pad))PhraseMenu_Update, (u16 (*)(s32 pad))OptionMenu_Update, (u16 (*)(s32 pad))GameMenu_UpdatePause,
    (u16 (*)(s32 pad))GameMenu_UpdateQuit, NULL, NULL, NULL,
};
s32 (*gGameMenuOpen1[12])(void *m) = {
    PouchMenu_Open, (s32 (*)(void *m))GameMenu_OpenCombine, StatusMenu_Open, InfoMenu_Open,
    (s32 (*)(void *m))GameMenu_OpenChat, GameMenu_OpenMail, (s32 (*)(void *m))GameMenu_OpenSearch, OptionMenu_Open,
    GameMenu_OpenExit, NULL, NULL, NULL,
};
u16 (*gGameMenuUpdate1[12])(s32 pad) = {
    (u16 (*)(s32 pad))GameMenu_UpdateItems, (u16 (*)(s32 pad))GameMenu_UpdateCombine, (u16 (*)(s32 pad))StatusMenu_Update, (u16 (*)(s32 pad))InfoMenu_Update,
    (u16 (*)(s32 pad))GameMenu_UpdateChat, (u16 (*)(s32 pad))GameMenu_UpdateMail, (u16 (*)(s32 pad))GameMenu_UpdateSearch, (u16 (*)(s32 pad))OptionMenu_Update,
    (u16 (*)(s32 pad))GameMenu_UpdateExit, NULL, NULL, NULL,
};
s32 (*gGameMenuOpen2[12])(void *m) = {
    PouchMenu_Open, (s32 (*)(void *m))GameMenu_OpenCombine, StatusMenu_Open, QuestMenu_Open,
    InfoMenu_Open, (s32 (*)(void *m))GameMenu_OpenChat, GameMenu_OpenMail, (s32 (*)(void *m))GameMenu_OpenSearch,
    OptionMenu_Open, GameMenu_OpenExit, NULL, NULL,
};
u16 (*gGameMenuUpdate2[12])(s32 pad) = {
    (u16 (*)(s32 pad))GameMenu_UpdateItems, (u16 (*)(s32 pad))GameMenu_UpdateCombine, (u16 (*)(s32 pad))StatusMenu_Update, (u16 (*)(s32 pad))QuestMenu_Update,
    (u16 (*)(s32 pad))InfoMenu_Update, (u16 (*)(s32 pad))GameMenu_UpdateChat, (u16 (*)(s32 pad))GameMenu_UpdateMail, (u16 (*)(s32 pad))GameMenu_UpdateSearch,
    (u16 (*)(s32 pad))OptionMenu_Update, (u16 (*)(s32 pad))GameMenu_UpdateExit, NULL, NULL,
};
GameMenuDrawFunc gGameMenuDraw0[12] = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, QuestMenu_DrawPage,
    InfoMenu_Draw, PhraseMenu_Draw, OptionMenu_Draw, GameMenu_DrawPause,
    GameMenu_DrawQuit, NULL, NULL, NULL,
};
GameMenuDrawFunc gGameMenuDraw1[12] = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, InfoMenu_Draw,
    GameMenu_DrawChat, GameMenu_DrawMail, GameMenu_DrawSearch, OptionMenu_Draw,
    NULL, NULL, NULL, NULL,
};
GameMenuDrawFunc gGameMenuDraw2[12] = {
    PouchMenu_Draw, CombineMenu_DrawInfo, StatusMenu_Draw, QuestMenu_DrawPage,
    InfoMenu_Draw, GameMenu_DrawChat, GameMenu_DrawMail, GameMenu_DrawSearch,
    OptionMenu_Draw, NULL, NULL, NULL,
};
ListPanel gGameMenuPanel = {
#include "lobby/007F1EF0.inc"
};
ListPanel gSearchPanel = {
#include "lobby/007F1F10.inc"
};
