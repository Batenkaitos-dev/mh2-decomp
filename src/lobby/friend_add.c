/* Adding a user to the friend list or to the mail block list, and the
 * question that starts taking one off the block list.
 *
 * A hunter's info window and the user lists call FriendList_AskAdd,
 * BlockList_AskAdd or BlockList_AskRemove with the user's id and name,
 * which are kept in gAskUserId and gAskUserName, then run the matching
 * input function each frame until it stops returning MENU_RUNNING. The
 * friend list (gFriendList, FriendEntry records) and the block list
 * (gMailBlockList, BlockedUser records) are found by id; the player's own
 * id is never added. The question and its answers are LOBBY_TEXT_FRIEND_*
 * and LOBBY_TEXT_BLOCK_* messages over the lobby menu (gLobbyMenu). */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "lobby_menu.h"
#include "pad.h"
#include "award.h"
#include "friend_add.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "libc.h"

extern u8 gAskUserName[24];   /* the user asked about: name */
extern u8 gAskUserId[LOBBY_USER_ID_LEN];   /* and id */
extern LobbyMenu *gLobbyMenu;

/* Asks whether to add a user to the friend list (a yes or no over the
 * user's id and name). userId and name are copied to gAskUserId and
 * gAskUserName; with either null, the user asked about before is asked
 * again. Takes field control from the player while the question is up. */
void FriendList_AskAdd(void *userId, void *name)
{
    gLobbyMenu->redraw = 1;
    gLobbyMenu->subStep = USER_ASK_FIRST;
    LobbyText_Open(LOBBY_TEXT_FRIEND_ADD_ASK);
    Menu_SetYesNo(0);
    if ((userId != 0) && (name != 0)) {
        memcpy(&gAskUserId, (void *) userId, LOBBY_USER_ID_LEN);
        memcpy(&gAskUserName, (void *) name, LOBBY_USER_NAME_LEN + 1);
    }
    LobbyDialog_SetMember(&gAskUserId, &gAskUserName);
    gFieldControl = 0;
}

extern char gMyUserId[];
extern FriendEntry gFriendList[];
extern s8 gFriendStatus[];
s32 Award_Grant(s32 a);
/* Runs the friend question FriendList_AskAdd opened. A yes is asked again
 * (the user is new, or is listed already and the entry is written again);
 * the player's own id and a full list end it. Filling the list's last entry
 * sets RATE_FLAG_FRIENDS and grants AWARD_FRIENDS. The last message waits
 * for PAD_CIRCLE. Returns MENU_DONE once the user is listed, MENU_UNDONE when nothing
 * was added, MENU_RUNNING before, MENU_CLOSE on PAD_START. */
s32 FriendList_AddInput(u16 pad)
{
    s32 i;
    u8 *entry;

    if (pad & PAD_START) {
        return MENU_CLOSE;
    }
    gLobbyMenu->redraw = 1;
    switch (gLobbyMenu->subStep) {
    case USER_ASK_FIRST:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            gLobbyMenu->subStep++;
            if (memcmp(&gAskUserId, gMyUserId, LOBBY_USER_ID_LEN) == 0) {
                gLobbyMenu->subStep = USER_ASK_ENDED;
                LobbyText_Open(LOBBY_TEXT_FRIEND_IS_SELF);
            } else if (FriendList_FindId((s8 *) gFriendList, FRIEND_LIST_LEN, &gAskUserId) == -1) {
                if (FriendList_FindFree((s8 *) gFriendList, FRIEND_LIST_LEN) == -1) {
                    gLobbyMenu->subStep = USER_ASK_ENDED;
                    LobbyText_Open(LOBBY_TEXT_FRIEND_LIST_FULL);
                } else {
                    LobbyText_Open(LOBBY_TEXT_FRIEND_NEW_ASK);
                    Menu_SetYesNo(0);
                }
            } else {
                LobbyText_Open(LOBBY_TEXT_FRIEND_LISTED_ASK);
                Menu_SetYesNo(0);
            }
            break;
        case MENU_CANCEL:
            gLobbyMenu->subStep = USER_ASK_ENDED;
            LobbyText_Open(LOBBY_TEXT_FRIEND_ADD_NO);
            break;
        }
        break;
    case USER_ASK_CONFIRM:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            i = FriendList_FindId((s8 *) gFriendList, FRIEND_LIST_LEN, &gAskUserId);
            if (i == -1) {
                i = FriendList_FindFree((s8 *) gFriendList, FRIEND_LIST_LEN);
                if (i == -1) {
                    i = 0;
                }
                LobbyText_Open(LOBBY_TEXT_FRIEND_ADDED);
            } else {
                LobbyText_Open(LOBBY_TEXT_FRIEND_UPDATED);
            }
            /* The id, then the name after it: one pointer stepped
             * across the entry, as the game's code does. */
            entry = (u8 *)&gFriendList[i];
            memset(entry, 0, LOBBY_USER_ID_LEN);
            memcpy((s32 *)entry, &gAskUserId, LOBBY_USER_ID_LEN);
            memset(entry += LOBBY_USER_ID_LEN, 0, LOBBY_USER_NAME_LEN);
            memcpy((s32 *)entry, &gAskUserName, LOBBY_USER_NAME_LEN);
            gFriendStatus[i] = FRIEND_STATUS_UNKNOWN;
            gLobbyMenu->subStep++;
            if (i == FRIEND_LIST_LEN - 1) {
                if (!(gSave.rateFlags & (1 << RATE_FLAG_FRIENDS))) {
                    RateFlag_SetAndNotify(RATE_FLAG_FRIENDS);
                }
                Award_Grant(AWARD_FRIENDS);
            }
            break;
        case MENU_CANCEL:
            FriendList_AskAdd(0, 0);
            gLobbyMenu->subStep = USER_ASK_FIRST;
            break;
        }
        break;
    case USER_ASK_DONE:
        if (pad & PAD_CIRCLE) {
            gLobbyMenu->subStep = USER_ASK_FIRST;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gFieldControl = 1;
            FriendList_FindId((s8 *) gFriendList, FRIEND_LIST_LEN, &gAskUserId);
            return MENU_DONE;
        }
        break;
    case USER_ASK_ENDED:
        if (pad & PAD_CIRCLE) {
            gLobbyMenu->subStep = USER_ASK_FIRST;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gFieldControl = 1;
            return MENU_UNDONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/* Asks whether to add a user to the mail block list, as FriendList_AskAdd
 * does for the friend list. */
void BlockList_AskAdd(void *userId, void *name)
{
    gLobbyMenu->redraw = 1;
    gLobbyMenu->subStep = USER_ASK_FIRST;
    LobbyText_Open(LOBBY_TEXT_BLOCK_ADD_ASK);
    Menu_SetYesNo(0);
    if ((userId != 0) && (name != 0)) {
        memcpy(&gAskUserId, (void *) userId, LOBBY_USER_ID_LEN);
        memcpy(&gAskUserName, (void *) name, LOBBY_USER_NAME_LEN + 1);
    }
    LobbyDialog_SetMember(&gAskUserId, &gAskUserName);
    gFieldControl = 0;
}

extern u8 gMailBlockList[][sizeof(BlockedUser)];

/* Runs the block list question BlockList_AskAdd opened, as
 * FriendList_AddInput runs the friend one (without the award). Returns
 * MENU_DONE once the user is listed, MENU_UNDONE when nothing was added,
 * MENU_RUNNING before, MENU_CLOSE on PAD_START. */
s32 BlockList_AddInput(u16 pad)
{
    s32 i;
    u8 *entry;

    if (pad & PAD_START) {
        return MENU_CLOSE;
    }
    gLobbyMenu->redraw = 1;
    switch (gLobbyMenu->subStep) {
    case USER_ASK_FIRST:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            gLobbyMenu->subStep++;
            if (memcmp(&gAskUserId, gMyUserId, LOBBY_USER_ID_LEN) == 0) {
                gLobbyMenu->subStep = USER_ASK_ENDED;
                LobbyText_Open(LOBBY_TEXT_BLOCK_ADD_NO);
            } else if (IdList_Find((s8 *) gMailBlockList, MAIL_BLOCK_LIST_LEN, &gAskUserId) == -1) {
                if (IdList_FindFree((s8 *) gMailBlockList, MAIL_BLOCK_LIST_LEN) == -1) {
                    gLobbyMenu->subStep = USER_ASK_ENDED;
                    LobbyText_Open(LOBBY_TEXT_BLOCK_LIST_FULL);
                } else {
                    LobbyText_Open(LOBBY_TEXT_BLOCK_NEW_ASK);
                    Menu_SetYesNo(0);
                }
            } else {
                LobbyText_Open(LOBBY_TEXT_BLOCK_LISTED_ASK);
                Menu_SetYesNo(0);
            }
            break;
        case MENU_CANCEL:
            gLobbyMenu->subStep = USER_ASK_ENDED;
            LobbyText_Open(LOBBY_TEXT_BLOCK_ADD_NO);
            break;
        }
        break;
    case USER_ASK_CONFIRM:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            i = IdList_Find((s8 *) gMailBlockList, MAIL_BLOCK_LIST_LEN, &gAskUserId);
            if (i == -1) {
                i = IdList_FindFree((s8 *) gMailBlockList, MAIL_BLOCK_LIST_LEN);
                if (i == -1) {
                    i = 0;
                }
                LobbyText_Open(LOBBY_TEXT_BLOCK_ADDED);
            } else {
                LobbyText_Open(LOBBY_TEXT_BLOCK_UPDATED);
            }
            /* The id, then the name: one pointer stepped across the entry. */
            entry = gMailBlockList[i];
            memset(entry, 0, LOBBY_USER_ID_LEN);
            memcpy((void *)entry, &gAskUserId, LOBBY_USER_ID_LEN);
            memset(entry += LOBBY_USER_ID_LEN, 0, LOBBY_USER_NAME_LEN);
            memcpy((void *)entry, &gAskUserName, LOBBY_USER_NAME_LEN);
            gLobbyMenu->subStep++;
            break;
        case MENU_CANCEL:
            BlockList_AskAdd(0, 0);
            gLobbyMenu->subStep = USER_ASK_FIRST;
            break;
        }
        break;
    case USER_ASK_DONE:
        if (pad & PAD_CIRCLE) {
            gLobbyMenu->subStep = USER_ASK_FIRST;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gFieldControl = 1;
            return MENU_DONE;
        }
        break;
    case USER_ASK_ENDED:
        if (pad & PAD_CIRCLE) {
            gLobbyMenu->subStep = USER_ASK_FIRST;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gFieldControl = 1;
            return MENU_UNDONE;
        }
        break;
    }
    return MENU_RUNNING;
}

/* Asks whether to take a user off the mail block list (a yes or no over
 * the entry's id and name, kept as FriendList_AskAdd keeps them). The
 * answer is read by BlockList_RemoveInput. */
void BlockList_AskRemove(void *userId, u8 *name)
{
    gLobbyMenu->redraw = 1;
    gLobbyMenu->subStep = USER_ASK_FIRST;
    LobbyText_Open(LOBBY_TEXT_BLOCK_REMOVE_ASK);
    Menu_SetYesNo(0);
    if ((userId != 0) && (name != 0)) {
        memcpy(&gAskUserId, (void *) userId, LOBBY_USER_ID_LEN);
        memcpy(&gAskUserName, (void *) name, LOBBY_USER_NAME_LEN + 1);
    }
    LobbyDialog_SetMember(&gAskUserId, &gAskUserName);
    gFieldControl = 0;
}

struct BlockedUser;
/* The game built the next function at optimization level 4. */
#pragma optimization_level 4
/* Runs the question BlockList_AskRemove opened: a yes takes the user in
 * gAskUserId off the mail block list and says so, a no says the user stays
 * listed; either message waits for PAD_CIRCLE, then gives field control
 * back. Returns MENU_DONE (taken off) or MENU_CANCEL (kept) then,
 * MENU_RUNNING before, MENU_CLOSE on PAD_START. */
s32 BlockList_RemoveInput(u16 pad)
{
    if (pad & PAD_START) {
        return MENU_CLOSE;
    }
    gLobbyMenu->redraw = 1;
    switch (gLobbyMenu->subStep) {
    case BLOCK_REMOVE_ASK:
        switch (Menu_YesNoInput()) {
        case MENU_DONE:
            gLobbyMenu->subStep++;
            IdList_Remove((struct BlockedUser *) gMailBlockList, IdList_Find((s8 *) gMailBlockList, MAIL_BLOCK_LIST_LEN, gAskUserId), MAIL_BLOCK_LIST_LEN);
            LobbyText_Open(LOBBY_TEXT_BLOCK_REMOVED);
            break;
        case MENU_CANCEL:
            gLobbyMenu->subStep = BLOCK_REMOVE_KEPT;
            LobbyText_Open(LOBBY_TEXT_BLOCK_KEPT);
            break;
        }
        break;
    case BLOCK_REMOVE_DONE:
        if (pad & PAD_CIRCLE) {
            gLobbyMenu->subStep = BLOCK_REMOVE_ASK;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gFieldControl = 1;
            return MENU_DONE;
        }
        break;
    case BLOCK_REMOVE_KEPT:
        if (pad & PAD_CIRCLE) {
            gLobbyMenu->subStep = BLOCK_REMOVE_ASK;
            gLobbyMenu->redraw = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            gFieldControl = 1;
            return MENU_CANCEL;
        }
        break;
    }
    return MENU_RUNNING;
}
#pragma optimization_level reset
