/*
 * The friend list (gFriendList: FRIEND_LIST_LEN FriendEntry records, each a
 * user's id and name). The menu shows a page of FRIEND_PAGE_LEN rows: each
 * friend's name, id and connection status (gFriendStatus, enum
 * FriendStatus). The statuses still unknown on a page are asked of the
 * server, one at a time, when the page is shown. Choosing a row opens its
 * action window: the hunter info window, or taking the friend off the list
 * after a yes or no. Opened for a pick (FRIEND_LIST_OPEN_PICK), choosing a
 * row returns to the caller instead: the short mail form takes its
 * addressee that way.
 *
 * The menu's state is gFriendListMenu (FriendListMenu), which
 * FriendList_Open sets up; adding a friend is FriendList_AskAdd's.
 */
#include "common.h"
#include "sprite.h"
#include "lobby_text.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "menu_task.h"
#include "lobby_labels.h"
#include "bgm.h"
#include "sprite_queue.h"
#include "lobby_net.h"
#include "friend_list.h"
#include "draw_slots.h"
#include "font.h"
#include "hunter_info.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "lobby_web_users.h"
#include "menu_steps.h"
#include "libc.h"

#pragma optimization_level 4

/* FriendListMenu.state */
enum FriendListState {
    FRIEND_LIST_SLIDE_IN = 0,   /* the window slides in */
    FRIEND_LIST_CHECK = 1,      /* the page's unknown statuses are asked of the server */
    FRIEND_LIST_LIST = 2,       /* the list */
    FRIEND_LIST_ACTIONS = 3,    /* the chosen friend's action window */
    FRIEND_LIST_REMOVE = 4,     /* the question whether to take the friend off the list */
    FRIEND_LIST_INFO = 5        /* the hunter info window */
};

/* FriendList_ListInput's results besides MENU_RUNNING, MENU_CANCEL and MENU_CLOSE */
enum FriendListInput {
    FRIEND_INPUT_PAGE_TURNED = 0, /* the page changed: build it again */
    FRIEND_INPUT_CHOSEN = 1     /* circle chose the row under the cursor */
};

/* The action window's lines (FriendListMenu.actionSel) */
enum FriendAction {
    FRIEND_ACTION_INFO = 0,     /* open the hunter info window */
    FRIEND_ACTION_REMOVE = 1,   /* take the friend off the list */
    FRIEND_ACTION_COUNT = 2
};

extern s8 gFriendStatus[];
extern FriendEntry gFriendList[];

struct FriendEntryWords;

extern s8 gFriendListOpen;

#pragma optimization_level reset
extern FriendListMenu gFriendListMenu;
/* Opens the friend list: clears gFriendListMenu, builds its first page, puts
 * the cursor on the first row, marks the list open, sets its two draw tasks
 * (the list and the hunter info window) and starts the window's slide.
 * mode FRIEND_LIST_OPEN_PICK makes a choice return to the caller (the short
 * mail form picks its addressee); 0 opens the chosen friend's actions.
 * The game built this function at the default optimization level, the rest
 * of the file at level 4. */
void FriendList_Open(s32 mode)
{
    memset(&gFriendListMenu, 0, sizeof(FriendListMenu));
    FriendList_BuildPage(&gFriendListMenu, 0);
    gFriendListMenu.cursor = 0;
    gFriendListOpen = 1;
    gFriendListMenu.layer[0].draw = FriendList_Draw;
    gFriendListMenu.pick = mode;
    gFriendListMenu.layer[1].draw = FriendList_Draw2;
    gFriendListMenu.layer[0].textSize = 0;
    gFriendListMenu.layer[1].textSize = 1;
    WinSlide_Step(&gFriendListMenu.win, 640, 230);
}
#pragma optimization_level 4

/*
 * Clears gFriendListOpen: the list is no longer open, so the lobby's tick
 * counts the pages' status timers down again.
 */
static void FriendList_ClearOpen(void)
{
    gFriendListOpen = 0;
}

/*
 * Returns the friend list entry under the cursor (its user id is at its
 * head); the short mail form copies the addressee's id from it after a
 * pick.
 */
void *FriendList_GetSelectedId(void)
{
    return &gFriendList[gFriendListMenu.list[gFriendListMenu.cursor]];
}

/* Returns the name of the friend list entry under the cursor. */
void *FriendList_GetSelectedName(void)
{
    return gFriendList[gFriendListMenu.list[gFriendListMenu.cursor]].name;
}

extern u8 gLobbyLevel1Sel;
extern LobbyMenu *gLobbyMenu;
/*
 * Runs the friend list each frame: the window slides in, the page's
 * unknown statuses are checked, then the list, the action window, the
 * removal question or the hunter info window take the pad. Choosing a row
 * in pick mode returns MENU_DONE; leaving with cancel clears
 * gFriendListOpen and returns MENU_CANCEL; PAD_START returns MENU_CLOSE;
 * otherwise it adds the two draw tasks and returns MENU_RUNNING.
 */
s32 FriendList_Update(u16 pad)
{
    FriendListMenu *menu = (FriendListMenu *)&gFriendListMenu;
    s32 friendIdx;

    switch (gFriendListMenu.state) {
    case FRIEND_LIST_SLIDE_IN:
        switch (WinSlide_Step(&menu->win, 640, 230)) {
        case MENU_DONE:
            if (menu->count != 0 && FriendList_HasUnchecked(menu) == 1) {
                menu->state++;
            } else {
                menu->state = FRIEND_LIST_LIST;
            }
            break;
        }
        break;
    case FRIEND_LIST_CHECK:
        switch (FriendList_CheckStatus(menu)) {
        case MENU_DONE:
            menu->state++;
            break;
        }
        break;
    case FRIEND_LIST_LIST:
        switch (FriendList_ListInput(menu, pad)) {
        case FRIEND_INPUT_PAGE_TURNED:
            FriendList_BuildPage(menu, menu->page);
            if (menu->count != 0 && FriendList_HasUnchecked(menu) == 1) {
                menu->state = FRIEND_LIST_CHECK;
            }
            break;
        case FRIEND_INPUT_CHOSEN:
            if (menu->pick == FRIEND_LIST_OPEN_PICK) {
                return MENU_DONE;
            }
            menu->state = FRIEND_LIST_ACTIONS;
            menu->actionSel = FRIEND_ACTION_INFO;
            break;
        case MENU_CANCEL:
            FriendList_ClearOpen();
            return MENU_CANCEL;
        case MENU_CLOSE:
            return MENU_CLOSE;
        }
        break;
    case FRIEND_LIST_ACTIONS:
        switch (FriendList_ActionInput(menu, pad)) {
        case MENU_DONE:
            friendIdx = menu->list[menu->cursor];
            switch (menu->actionSel) {
            case FRIEND_ACTION_INFO:
                menu->state = FRIEND_LIST_INFO;
                if (gLobbyLevel1Sel) {
                    HunterInfo_Open(&gFriendList[friendIdx], HUNTER_INFO_MAIL);
                } else {
                    HunterInfo_Open(&gFriendList[friendIdx], HUNTER_INFO_MAIL | HUNTER_INFO_JOIN);
                }
                break;
            case FRIEND_ACTION_REMOVE:
                menu->state = FRIEND_LIST_REMOVE;
                LobbyText_Open(LOBBY_TEXT_FRIEND_REMOVE_ASK);
                Menu_SetYesNo(1); /* the cursor starts on no */
                LobbyDialog_Grow();
                LobbyDialog_SetMember(&gFriendList[friendIdx], gFriendList[friendIdx].name);
                gLobbyMenu->redraw = 1;
                gFieldControl = 0;
                break;
            }
            break;
        case MENU_CANCEL:
            menu->state = FRIEND_LIST_LIST;
            break;
        case MENU_CLOSE:
            return MENU_CLOSE;
        }
        break;
    case FRIEND_LIST_REMOVE:
        switch (FriendList_RemoveInput(menu, pad)) {
        case MENU_DONE:
            menu->state = FRIEND_LIST_CHECK;
            gFieldControl = 1;
            break;
        case MENU_CANCEL:
            menu->state = FRIEND_LIST_ACTIONS;
            gFieldControl = 1;
            break;
        case MENU_CLOSE:
            gFieldControl = 1;
            return MENU_CLOSE;
        }
        break;
    case FRIEND_LIST_INFO:
        switch (HunterInfo_Update(pad)) {
        case MENU_DONE:
        case 1:
        case MENU_CANCEL:
            menu->state = FRIEND_LIST_ACTIONS;
            break;
        case MENU_CLOSE:
            return MENU_CLOSE;
        }
        break;
    }
    OT_AddAt(gDrawListText0b, &menu->layer[0], 0, 1);
    OT_AddAt(gDrawListText1, &menu->layer[1], 0, 1);
    return MENU_RUNNING;
}

/*
 * Returns 1 when a friend on the current page still has the status
 * FRIEND_STATUS_UNKNOWN, else 0.
 */
s32 FriendList_HasUnchecked(FriendListMenu *menu)
{
    s32 i;

    for (i = 0; i < menu->count; i++) {
        if (gFriendStatus[menu->list[i]] == FRIEND_STATUS_UNKNOWN) {
            return 1;
        }
    }
    return 0;
}

/*
 * Takes entry idx off the friend list: the later entries and their
 * statuses move down one, and the last status becomes
 * FRIEND_STATUS_UNKNOWN.
 */
void FriendList_Remove(s32 idx)
{
    s32 i;
    s8 *status;

    i = idx;
    FriendList_DeleteAt((struct FriendEntryWords *)&gFriendList, idx & 0xFF, FRIEND_LIST_LEN);
    if (i < FRIEND_LIST_LEN - 1) {
        status = gFriendStatus + i;
        do {
            i += 1;
            status[0] = status[1];
            status += 1;
        } while (i < FRIEND_LIST_LEN - 1);
    }
    gFriendStatus[FRIEND_LIST_LEN - 1] = FRIEND_STATUS_UNKNOWN;
}

extern s8 gMenuInputLock;
/*
 * Runs the removal question over the chosen friend: a yes removes the
 * entry, rebuilds the page, keeps the cursor on a row and shows the
 * removed message until PAD_CIRCLE (then MENU_DONE); a no returns
 * MENU_CANCEL. PAD_START returns MENU_CLOSE.
 */
s32 FriendList_RemoveInput(FriendListMenu *menu, u16 pad)
{
    s32 friendIdx;
    FriendEntry *entry;

    if (pad & PAD_START) {
        return MENU_CLOSE;
    }
    gLobbyMenu->redraw = 1;
    switch (menu->step) {
    case 0:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            menu->step++;
            LobbyText_Open(LOBBY_TEXT_FRIEND_REMOVED);
            friendIdx = menu->list[menu->cursor];
            entry = &gFriendList[friendIdx];
            LobbyDialog_SetMember(entry, entry->name);
            FriendList_Remove(friendIdx);
            FriendList_BuildPage(menu, menu->page);
            if (menu->cursor >= menu->count) {
                menu->cursor = menu->count - 1;
            }
            gMenuInputLock = 3; /* frames */
            break;
        case MENU_CANCEL:
            menu->step = 0;
            return MENU_CANCEL;
        }
        break;
    case 1:
        if (pad & PAD_CIRCLE) {
            menu->step = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Moves the action window's cursor (the status, i.e. the hunter info, or
 * the removal). Returns MENU_DONE on PAD_CIRCLE, MENU_CANCEL on PAD_CROSS,
 * MENU_CLOSE on PAD_START, else MENU_RUNNING.
 */
s32 FriendList_ActionInput(FriendListMenu *menu, u16 pad)
{
    s32 buttons;

    buttons = pad & 0xFFFF;
    if (buttons & PAD_CIRCLE) {
        LobbySe_Play(LOBBY_SE_CONFIRM);
        return MENU_DONE;
    }
    if (buttons & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    }
    if (buttons & PAD_START) {
        return MENU_CLOSE;
    }
    menu->actionSel = Menu_WrapUpDown((s32) menu->actionSel, FRIEND_ACTION_COUNT);
    return MENU_RUNNING;
}

/*
 * The list's input: up and down move the cursor over the page's rows, left
 * and right turn the page (returns 0 so the caller rebuilds it).
 * PAD_CIRCLE on a non-empty page returns 1 (a friend chosen), PAD_CROSS
 * MENU_CANCEL, PAD_START MENU_CLOSE, else MENU_RUNNING.
 */
s32 FriendList_ListInput(FriendListMenu *menu, u16 pad)
{
    s16 pageCount;
    s32 buttons;

    buttons = pad & 0xFFFF;
    if (buttons & PAD_CIRCLE) {
        if (menu->count != 0) {
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return FRIEND_INPUT_CHOSEN;
        }
    } else if (buttons & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (buttons & PAD_START) {
        return MENU_CLOSE;
    } else if (buttons & (PAD_LEFT | PAD_RIGHT)) {
        pageCount = menu->pageCount;
        if (pageCount > 1) {
            menu->page = Menu_WrapLeftRight((s32) menu->page, (s32) pageCount);
            return FRIEND_INPUT_PAGE_TURNED;
        }
    } else {
        menu->cursor = Menu_WrapUpDown((s32) menu->cursor, (s32) menu->count);
    }
    return MENU_RUNNING;
}

extern s16 gFriendCheckTimers[];
extern u16 gFriendCheckFrames;
/*
 * Asks the server for the status of each friend on the page whose status
 * is unknown, one at a time, and restarts the page's status timer
 * (gFriendCheckTimers) with gFriendCheckFrames. A reply gives the status
 * byte; a failed lookup or the player's own id gives
 * FRIEND_STATUS_OFFLINE. Returns MENU_DONE once the page is done,
 * MENU_RUNNING before.
 */
s32 FriendList_CheckStatus(FriendListMenu *menu)
{
    s32 friendIdx;
    u16 unused;

    switch (menu->step) {
    case 0:
        gFriendCheckTimers[menu->page] = gFriendCheckFrames;
        menu->step++;
        menu->checkIdx = 0;
    case 1:
        if (gFriendStatus[menu->list[menu->checkIdx]] != FRIEND_STATUS_UNKNOWN) {
            if (++menu->checkIdx >= menu->count) {
                menu->step = 0;
                return MENU_DONE;
            }
            break;
        }
        menu->step++;
    case 2:
        friendIdx = menu->list[menu->checkIdx];
        switch (LobbyUser_FindById((s8 *)&gFriendList[friendIdx])) {
        case 0:
            LobbyNet_GetUserStatus(&unused, &unused, &unused, (u8 *)&gFriendStatus[friendIdx]);
            if (++menu->checkIdx >= menu->count) {
                menu->step = 0;
                return MENU_DONE;
            }
            menu->step--;
            break;
        case 3: /* LobbyUser_FindById: 3 the player's own id, 1 not found */
        case 1:
            gFriendStatus[friendIdx] = FRIEND_STATUS_OFFLINE;
            if (++menu->checkIdx >= menu->count) {
                menu->step = 0;
                return MENU_DONE;
            }
            menu->step--;
            break;
        }
        break;
    default:
        menu->step = 0;
        return MENU_DONE;
    }
    return 2;
}

/*
 * Fills the menu's list with the record indexes of the used entries on
 * page `page` (FRIEND_PAGE_LEN a page), counts the pages, steps back a
 * page when this one is now past the end, and keeps the cursor on a row.
 */
void FriendList_BuildPage(FriendListMenu *menu, s16 page)
{
    s32 i;
    s32 listed = 0;
    s16 first;
    s16 last;

    menu->count = 0;
    first = page * FRIEND_PAGE_LEN;
    last = first + FRIEND_PAGE_LEN - 1;
    for (i = 0; i < FRIEND_LIST_LEN; i++) {
        if ((s8)gFriendList[i].userId[0] != 0) { /* a used entry; read as a signed byte, as the game's code does */
            if (listed >= first && listed <= last) {
                menu->list[menu->count] = i;
                menu->count++;
            }
            listed++;
        }
    }
    menu->page = page;
    menu->pageCount = Menu_CountPages(listed, FRIEND_PAGE_LEN);
    if (page >= menu->pageCount) {
        FriendList_BuildPage(menu, page - 1);
    }
    if (menu->cursor >= menu->count) {
        menu->cursor = menu->count - 1;
    }
}

void Win_Draw(s16 x, s16 y, s32 w, s32 h, u32 c1, u32 c2, s32 k); /* unit's own declaration: declared with the types the calls here were compiled with; the definition's (s32 coordinates and an s16 kind) change the calls' code */
void Gfx_DrawLine(s16 x, s16 y, s16 w, s16 h, s32 color);
void Menu_DrawRowHighlight(s16 x, s16 y, s32 w, s32 h, s32 sel); /* unit's own declaration: declared with the types the calls here were compiled with; the definition's (s16 coordinates and an s8 flag) change the calls' code */
void Font_SetColor(s32 color);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
/*
 * Draws the list window at x, y: the title and page number, the column
 * headings (name, id, connection), then each row with its name, its id
 * widened to full-width characters and its status label; the cursor row is
 * highlighted (dimmed outside the list's own state). An empty list draws
 * only the empty-list line.
 */
void FriendList_DrawList(FriendListMenu *menu, s16 x, s16 y)
{
    FriendEntry *entry;
    char wideId[64];
    char *label;
    s32 dim;
    s32 i;
    s32 friendIdx;
    s16 rowY = y;

    Win_Draw(x, y, 390, 290, 0xBDB59F6C, 0xBB402010, 8);
    Gfx_DrawLine(x + 8, y + 12, 374, 20, 0x80B59F6C);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 16, y + 14);
    Text_PrintfColored(gGameMenuLabels[MENU_LABEL_FRIEND_LIST]); /* the list window's title */
    if (menu->count == 0) {
        Font_SetColor(0x802B7F7F);
        Font_SetPos(Text_CenterX(gNetLabels[NET_LABEL_NOT_REGISTERED], x + 195, 18), (s16)(y + 14) + 90);
        Text_PrintfColored(gNetLabels[NET_LABEL_NOT_REGISTERED]);
        return;
    }
    Menu_DrawPageNum(x + 300, y + 14, menu->page, menu->pageCount, 1);
    rowY += 36;
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 16, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_NAME]);
    Font_SetPos(x + 170, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_ID]);
    Font_SetPos(x + 288, rowY);
    Text_PrintfColored(gNetLabels[NET_LABEL_CONN_STATE]);
    for (i = 0; i < menu->count; i++) {
        friendIdx = menu->list[i];
        entry = &gFriendList[friendIdx];
        Font_SetColor(0x807F7F7F);
        rowY += 22;
        if (menu->cursor == i) {
            switch (menu->state) {
            case FRIEND_LIST_LIST:
                dim = 0;
                break;
            default:
                dim = 1;
                break;
            }
            Menu_DrawRowHighlight(x + 8, rowY - 1, 374, 20, dim);
        }
        Font_SetPos(x + 16, rowY);
        Text_PrintfColored("%s", entry->name);
        Font_SetPos(x + 170, rowY);
        Sjis_Widen((char *)entry, wideId);
        Text_PrintfColored(wideId);
        Font_SetPos(x + 288, rowY);
        switch (gFriendStatus[friendIdx]) {
        case FRIEND_STATUS_UNKNOWN:
            label = gNetLabels[NET_LABEL_CHECKING];
            break;
        case FRIEND_STATUS_ONLINE:
            label = gNetLabels[NET_LABEL_ONLINE];
            break;
        case FRIEND_STATUS_ON_QUEST:
            label = gNetLabels[NET_LABEL_ON_QUEST];
            break;
        case FRIEND_STATUS_OFFLINE:
            label = gNetLabels[NET_LABEL_OFFLINE];
            break;
        }
        Text_Printf(label);
    }
}

/*
 * Draws the two-line action window to the left of the list (the status and
 * the removal), with the action cursor highlighted (dimmed outside the
 * action state).
 */
void FriendList_DrawActions(FriendListMenu *menu, s16 x, s16 y)
{
    s16 i;
    s32 dim;

    Win_Draw(x - 110, y, 110, 58, 0xBDB59F6C, 0xBB402010, 3);
    y += 8;
    Font_SetColor(0x807F7F7F);
    for (i = 0; i < FRIEND_ACTION_COUNT; i++) {
        if (i > 0) {
            Gfx_DrawLine((s16)(x - 110) + 8, y - 2, 94, 2, 0xBDB59F6C);
        }
        if (menu->actionSel == i) {
            switch (menu->state) {
            case FRIEND_LIST_ACTIONS:
                dim = 0;
                break;
            default:
                dim = 1;
                break;
            }
            Menu_DrawRowHighlight((s16)(x - 110) + 8, y - 1, 94, 20, dim);
        }
        Font_SetPos((s16)(x - 110) + 8, y);
        Text_PrintfColored(gNetLabels[NET_LABEL_STATUS + i]);
        y += 22;
    }
}

void Text_SetSize(s32);
/*
 * The friend list's first draw task: sets the task's text size, then draws
 * the list at the window's slide position, and the action window over it
 * while the action window or the removal question is up; nothing while the
 * hunter info window is open.
 */
void FriendList_Draw(MenuTask *task)
{
    FriendListMenu *menu = &gFriendListMenu;
    s16 x;

    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    if (gFriendListMenu.state != FRIEND_LIST_INFO) {
        x = menu->win.pos;
        FriendList_DrawList(menu, x, 58);
        switch (menu->state) {
        case FRIEND_LIST_REMOVE:
        case FRIEND_LIST_ACTIONS:
            FriendList_DrawActions(menu, x, 58);
            break;
        }
    }
}

/*
 * The friend list's second draw task: sets the task's text size and draws
 * the hunter info window while it is open.
 */
void FriendList_Draw2(MenuTask *task)
{
    Text_SetSize(task->textSize);
    Font_SetSpacing(18, 18);
    switch (gFriendListMenu.state) {
    case FRIEND_LIST_INFO:
        HunterInfo_DrawIfOpen();
        break;
    }
}
