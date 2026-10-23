/*
 * The versus quest's preparation in the town. Town_CheckQuestParty waits
 * for the room's members and both sides' monsters (gNextQuest.teams)
 * before setting out; the arena menu (ArenaMenu_*) lets a player pick one
 * of the records in gSave.arenaEms within 60 s, and an item to feed its
 * monster (ArenaFeed_*), and sends the choice.
 */
#include "common.h"
#include "lobby_text.h"
#include "lobby_state.h"
#include "save.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "quest.h"             /* TeamEntry: gTeamItemBits, gMonsterNoteMarks */
#include "lobby_labels.h"
#include "pad.h"
#include "menu_task.h"
#include "arena_menu.h"
#include "arena_cup.h"
#include "chat_box.h"
#include "draw_helpers.h"
#include "font.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_net.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "libc.h"
#pragma optimization_level 4

/* The versus preparation's feed list shows this many items a page. */
#define FEED_PAGE_ROWS 7

/* ArenaMenu.step: ArenaMenu_Update's steps. */
enum ArenaMenuStep {
    ARENA_MENU_SLIDE_IN = 0,    /* the record window slides in */
    ARENA_MENU_PICK = 1,        /* choosing the record: PAD_CIRCLE the feed list, PAD_SQUARE the monster's details */
    ARENA_MENU_INFO_IN = 2,     /* the details window slides in */
    ARENA_MENU_INFO_OUT = 3,    /* and out again */
    ARENA_MENU_INFO = 4,        /* the details: PAD_CROSS closes them */
    ARENA_MENU_FEED_IN = 5,     /* the feed list slides in */
    ARENA_MENU_FEED = 6         /* choosing the item to feed (ArenaMenu_ListInput) */
};

/* The versus preparation's menu, in gMenuWork. */
typedef struct ArenaMenu {
    /* 0x00 */ WinSlide win;        /* the record window */
    /* 0x08 */ WinSlide win2;       /* the details window or the feed list */
    /* 0x10 */ s32 pageItems[FEED_PAGE_ROWS]; /* the feed list's page: the items held, 0 past the end */
    /* 0x2C */ s32 feedItem;        /* the item chosen to feed, 0 for none */
    /* 0x30 */ s16 timeLeft;        /* frames: 1800 (60 s) at the start; the choice is sent when it runs out */
    /* 0x32 */ s16 page;
    /* 0x34 */ s16 pages;
    /* 0x36 */ s16 itemCount;       /* the items on the page */
    /* 0x38 */ s8 step;             /* ArenaMenuStep */
    /* 0x39 */ s8 record;           /* the gSave.arenaEms record chosen */
    /* 0x3A */ s8 cursor;
    /* 0x3B */ s8 used;             /* records of gSave.arenaEms in use */
    /* 0x3C */ s8 showDesc;         /* the item's description is shown (PAD_SQUARE toggles it; ItemInfo_Draw) */
} ArenaMenu;
extern s8 gQuestMinMembers;
extern s32 gTownMode;
extern u8 gMenuWork;
void Arena_DrawMonsterList(s32, s32, s32, s32); /* unit's own declaration: declared with int arguments, as ArenaMenu_DrawRecord passes them; with the definition's s16 and s8 parameters the call narrows them first and compiles differently */
void TownMsg_SendArenaTeam(void *, s32);
extern LobbyMenu *gLobbyMenu;
void Text_SetSize(s32);

/*
 * TOWN_MODE_QUEST_PARTY, each frame once the quest is paid: with fewer
 * members in the room than the quest needs (gQuestMinMembers), the
 * too-few-members message and TOWN_MODE_ROOM_LEAVE; for a versus quest,
 * once both sides' monsters are set (gNextQuest.teams), sends the teams
 * and the player's status and sets out (TOWN_MODE_SET_OUT_VERSUS).
 */
void Town_CheckQuestParty(void)
{
    if (gLobby->roomMembers < gQuestMinMembers) {
        LobbyText_Open(107);
        gTownMode = TOWN_MODE_ROOM_LEAVE;
        return;
    }
    if (gNextQuest.teams[0].emKind != 0 && gNextQuest.teams[1].emKind != 0) {
        TownMsg_SendArenaTeams();
        TownMsg_SendStatus();
        gTownMode = TOWN_MODE_SET_OUT_VERSUS;
    }
}

extern s32 gTownMenuOpen;
/*
 * Opens the versus preparation (TownMsg_Recv's TOWN_MSG_ARENA_OPEN, for
 * arena roles 1 and 3, gPlayerText.card.arenaRole): clears gMenuWork, sets the
 * draw callbacks and the 60 s limit, slides the record window in and counts
 * the gSave.arenaEms records in use.
 */
void ArenaMenu_Open(void)
{
    ArenaMenu *menu;
    s32 i;

    menu = (ArenaMenu *)&gMenuWork;
    gTownMenuOpen = 1;
    gTownMode = TOWN_MODE_ARENA_MENU;
    memset(menu, 0, MENU_WORK_SIZE);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(ArenaMenu_Draw, ArenaMenu_Draw2, 0, 0);
    menu->timeLeft = 1800; /* frames: 60 s */
    WinSlide_Step(&menu->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X);
    for (i = 0; i < 3; i++) {
        if (gSave.arenaEms[i].emKind != 0) {
            menu->used++;
        }
    }
    LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
}

/*
 * Sends the record chosen and the item to feed it (0 for none)
 * (TownMsg_SendArenaTeam), keeps the record in gNextQuest.arenaRecord,
 * shows the wait message and goes back to TOWN_MODE_QUEST_PARTY with the
 * menu closed.
 */
void ArenaMenu_Send(void)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;

    gTownMode = TOWN_MODE_QUEST_PARTY;
    if (menu->feedItem == 0) {
        menu->feedItem = 0;
    }
    gNextQuest.arenaRecord = menu->record;
    TownMsg_SendArenaTeam(&gSave.arenaEms[menu->record], menu->feedItem);
    LobbyText_Open(LOBBY_TEXT_PLEASE_WAIT);
    LobbyMenu_SetDraw(0, 0, 0, 0);
    gFieldControl = 0;
    gLobbyMenu->timer = 1800;
}

/*
 * TOWN_MODE_ARENA_MENU, each frame: counts the time down (sending the
 * choice when it runs out) and runs the step (ArenaMenuStep): the record,
 * its monster's details, the feed list.
 */
void ArenaMenu_Update(void)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;
    u16 pad = LobbyPad_Get(0);

    Chat_SetWindowMode(1, 2);
    if (--menu->timeLeft <= 0) {
        ArenaMenu_Send();
    }
    switch (menu->step) {
    case ARENA_MENU_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X)) {
        case MENU_DONE:
            menu->step++;
        }
        break;
    case ARENA_MENU_PICK:
        if (pad & PAD_CIRCLE) {
            menu->step = ARENA_MENU_FEED_IN;
            ArenaFeed_Open();
            WinSlide_Step(&menu->win2, ARENA_CUP_WIN_X_OUT, 266);
            LobbySe_Play(LOBBY_SE_CONFIRM);
        } else if (pad & PAD_SQUARE) {
            menu->step = ARENA_MENU_INFO_IN;
            WinSlide_Step(&menu->win2, ARENA_CUP_WIN_X_OUT, 284);
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        } else {
            menu->record = Menu_WrapUpDown(menu->record, menu->used);
        }
        break;
    case ARENA_MENU_INFO_IN:
        switch (WinSlide_Step(&menu->win2, ARENA_CUP_WIN_X_OUT, 284)) {
        case MENU_DONE:
            menu->step = ARENA_MENU_INFO;
        }
        break;
    case ARENA_MENU_INFO_OUT:
        switch (WinSlide_Step(&menu->win2, 284, ARENA_CUP_WIN_X_OUT)) {
        case MENU_DONE:
            menu->step = ARENA_MENU_PICK;
        }
        break;
    case ARENA_MENU_INFO:
        if (pad & PAD_CROSS) {
            menu->step = ARENA_MENU_INFO_OUT;
            WinSlide_Step(&menu->win2, 284, ARENA_CUP_WIN_X_OUT);
            LobbySe_Play(LOBBY_SE_CANCEL);
        }
        break;
    case ARENA_MENU_FEED_IN:
        switch (WinSlide_Step(&menu->win2, ARENA_CUP_WIN_X_OUT, 266)) {
        case MENU_DONE:
            menu->step++;
        }
        break;
    case ARENA_MENU_FEED:
        switch (ArenaMenu_ListInput(pad)) {
        case MENU_DONE:
            ArenaMenu_Send();
            break;
        case MENU_CANCEL:
            menu->step = ARENA_MENU_SLIDE_IN;
            menu->feedItem = 0;
            WinSlide_Step(&menu->win, ARENA_CUP_WIN_X_OUT, ARENA_CUP_WIN_X);
            break;
        }
        break;
    }
}

/*
 * The feed list's input: PAD_CIRCLE takes the item under the cursor
 * (MENU_DONE), PAD_CROSS goes back (MENU_CANCEL), PAD_SQUARE shows or hides
 * the item's description, left and right turn the page; else MENU_RUNNING.
 */
s32 ArenaMenu_ListInput(u16 pad)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;

    if (pad & PAD_CIRCLE) {
        menu->feedItem = menu->pageItems[menu->cursor];
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    }
    if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (pad & PAD_SQUARE) {
        menu->showDesc ^= 1;
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (menu->pages != 1) {
            menu->page = Menu_WrapLeftRight(menu->page, menu->pages);
            ArenaFeed_LoadPage(menu->page);
            menu->cursor = 0;
        }
    } else {
        menu->cursor = Menu_WrapUpDown(menu->cursor, menu->itemCount);
    }
    return MENU_RUNNING;
}

extern TeamEntry gTeamItemBits[];
extern TeamEntry *gMonsterNoteMarks[];
s32 SavePouch_Count(u16 item);
/*
 * Counts the items the player holds that can be fed to the chosen record's
 * monster: the team items (gTeamItemBits, up to the entry with key 0) and
 * the monster's own list (gMonsterNoteMarks, at the row of gMonsterNoteIds
 * that holds its kind). Sets the pages and loads the first.
 */
void ArenaFeed_Open(void)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;
    s32 kind;
    s32 held = 0;
    s32 entryNo = 0;
    s32 noteRow = 255;
    TeamEntry *entry;
    s32 key;
    u16 *noteId;
    s32 row;

    entry = gTeamItemBits;
    kind = gSave.arenaEms[menu->record].emKind;

    for (; entryNo < 921; entryNo++, entry++) {
        key = entry->key;
        if (key == 0) {
            break;
        }
        if (SavePouch_Count(key) != 0) {
            held++;
        }
    }
    for (row = 0, noteId = gMonsterNoteIds; row < 73; row++, noteId++) {
        if (*noteId == 0) {
            break;
        }
        if (kind == *noteId) {
            noteRow = row;
            break;
        }
    }
    if (noteRow != 255) {
        entry = gMonsterNoteMarks[noteRow];
        key = entry->key;
        while (key != 0) {
            if (SavePouch_Count(key) != 0) {
                held++;
            }
            entry++;
            key = entry->key;
        }
    }
    menu->pages = Menu_CountPages(held, FEED_PAGE_ROWS);
    menu->page = 0;
    menu->cursor = 0;
    ArenaFeed_LoadPage(0);
}

/*
 * Fills the feed list's page `page` (pageItems, itemCount) with the held
 * items ArenaFeed_Open counts, in the same order.
 */
void ArenaFeed_LoadPage(s32 page)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;
    s32 kind;
    s32 noteRow;
    TeamEntry *entry;
    s32 lastHeld;
    s32 held = 0;
    s32 key;
    s32 entryNo;
    TeamEntry *noteEntry;
    s32 firstHeld;
    s32 onPage;
    u16 *noteId;
    s32 row;
    u16 noteKey;

    kind = gSave.arenaEms[menu->record].emKind;
    noteRow = 255;
    memset(menu->pageItems, 0, sizeof(menu->pageItems));
    firstHeld = page * FEED_PAGE_ROWS;
    onPage = 0;
    entryNo = 0;
    lastHeld = firstHeld + FEED_PAGE_ROWS - 1;
    for (entry = gTeamItemBits; entryNo < 921; entryNo++, entry++) {
        key = entry->key;
        if (key == 0) {
            break;
        }
        if (SavePouch_Count(key) != 0) {
            if (held >= firstHeld && held <= lastHeld) {
                menu->pageItems[onPage] = key;
                onPage++;
            }
            held++;
        }
    }
    for (row = 0, noteId = gMonsterNoteIds; row < 73; row++, noteId++) {
        if (*noteId == 0) {
            break;
        }
        if (kind == *noteId) {
            noteRow = row;
            break;
        }
    }
    if (noteRow != 255) {
        noteEntry = gMonsterNoteMarks[noteRow];
        noteKey = noteEntry->key;
        while (noteKey != 0) {
            key = noteKey;
            if (SavePouch_Count(key) != 0) {
                if (held >= firstHeld && held <= lastHeld) {
                    menu->pageItems[onPage] = key;
                    onPage++;
                }
                held++;
            }
            noteEntry++;
            noteKey = noteEntry->key;
        }
    }
    menu->itemCount = onPage;
}

/* Draws the chosen record's monster list at (x, y): mode 1 while choosing the record, 2 after. */
void ArenaMenu_DrawRecord(s32 x, s32 y)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;
    s32 mode;

    switch (menu->step) {
    case ARENA_MENU_SLIDE_IN:
    case ARENA_MENU_PICK:
        mode = 1;
        break;
    default:
        mode = 2;
        break;
    }
    Arena_DrawMonsterList(x, y, menu->record, mode);
}

void Text_PrintfColored(const char *fmt, ...);
extern char *gItemNames[];
/*
 * Draws the feed list at (x, y): each item's name and count with its icon,
 * a bar under the cursor, the page arrows and the description of the item
 * under the cursor; the empty page's text when there is nothing to feed.
 */
void ArenaFeed_DrawList(s16 x, s16 y)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;
    s32 item;
    s16 row;
    s16 rowY = y;
    s32 selItem;

    LobbyDialog_DrawWin(x, y, 346, 288, 0x80FF2020);
    if (menu->pages > 1) {
        Menu_DrawPageArrows(x + 260, rowY + 10, menu->page, menu->pages, 0xBDB59F6C);
    }
    Font_SetColor(0x807F7F7F);
    rowY += 30;
    Font_SetSpacing(20, 20);
    if (menu->itemCount == 0) {
        Font_SetPos(Text_CenterX(gArenaFeedTexts[ARENA_FEED_NO_FEED], x + 173, 20), y + 134);
        Text_PrintfColored(gArenaFeedTexts[ARENA_FEED_NO_FEED]);
        return;
    }
    row = 0;
    do {
        Font_SetPos(x + 60, rowY);
        item = menu->pageItems[row];
        if (item != 0) {
            Text_PrintfColored("%-18s%s%2d", gItemNames[item], gLobbyWords[LOBBY_WORD_TIMES], SavePouch_Count(item));
            Item_DrawIcon((s16)(x + 60) - 36, rowY - 2, 20, item);
            if (row == menu->cursor) {
                selItem = item;
                Menu_DrawGlowBar((s16)(x + 60) + 133, rowY - 1, 346, 24, 0xB0008000);
            }
        }
        rowY += 22;
        row++;
    } while (row < FEED_PAGE_ROWS);
    ItemInfo_Draw((s16)(x + 60) - 50, rowY + 4, 346, menu->showDesc, selItem, 0, -1, 0x807F7F7F);
}

void Text_Printf(char *fmt, ...);
/*
 * The menu's first draw callback: the title, the seconds left, then the
 * feed list or the record window, and the help window of the step.
 */
void ArenaMenu_Draw(MenuTask *task)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;
    s32 sec = menu->timeLeft / 30;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(260, 18, 350, 0xFFB0B000, gArenaCupTexts[ARENA_TEXT_PREPARE], 2, 1);
    Font_SetSpacing(20, 20);
    Font_SetPos(480, 20);
    Text_Printf(gArenaCupTexts[ARENA_TEXT_TIME_LEFT], sec);
    switch (menu->step) {
    case ARENA_MENU_FEED_IN:
    case ARENA_MENU_FEED:
        ArenaFeed_DrawList(menu->win2.pos, 58);
        break;
    default:
        ArenaMenu_DrawRecord(menu->win.pos, 58);
        break;
    }
    switch (menu->step) {
    case ARENA_MENU_SLIDE_IN:
    case ARENA_MENU_PICK:
        Win_DrawText(260, 352, 360, 80, 0xBDB59F6C, 0x70401010, gArenaCupTexts[ARENA_TEXT_SECONDS]);
        break;
    case ARENA_MENU_FEED_IN:
    case ARENA_MENU_FEED:
        Win_DrawText(260, 352, 360, 80, 0xBDB59F6C, 0x70401010, gArenaFeedTexts[ARENA_FEED_TITLE]);
        break;
    }
}

/* The second draw callback: the monster's details while they are open. */
void ArenaMenu_Draw2(MenuTask *task)
{
    ArenaMenu *menu = (ArenaMenu *)&gMenuWork;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    switch (menu->step) {
    case ARENA_MENU_INFO_IN:
    case ARENA_MENU_INFO_OUT:
    case ARENA_MENU_INFO:
        Arena_DrawMonsterInfo(menu->win2.pos, 78, &gSave.arenaEms[menu->record]);
        break;
    }
}
