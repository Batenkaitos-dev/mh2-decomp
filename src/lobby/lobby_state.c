/*
 * The lobby's state.
 *
 * The lobby keeps its state in LobbyState (gLobby) and its menu's draw
 * layers in LobbyMenu (gLobbyMenu). This file sets them up and resets them:
 * Lobby_ResetState at the lobby's start, LobbyMenu_Init and
 * LobbyMenu_SetDraw for the menu's eight draw layers, and the small
 * accessors the other lobby files call (the place, the level selections,
 * the clock, the chat targets).
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "net_mode.h"
#include "draw_helpers.h"
#include "game_util.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_frame.h"
#include "lobby_world.h"
#include "member_list.h"
#include "menu_util.h"
#include "name_tags.h"
#include "save.h"
#include "village_clock.h"
#include "libc.h"

/* end of prototypes */
extern u8 gChatTargets[];
extern ItemStack gPouchBackup[POUCH_SLOTS];

extern LobbyMenu *gLobbyMenu;
extern MenuTask gLobbyMenuLayer0;
extern MenuTask gLobbyMenuLayer1;
extern MenuTask gLobbyMenuLayer2;
extern MenuTask gLobbyMenuLayer3;
extern MenuTask gLobbyMenuLayer4;
extern MenuTask gLobbyMenuLayer5;
extern MenuTask gLobbyMenuLayer6;
extern MenuTask gLobbyMenuLayer7;
extern u8 gMemberNamesOff;
extern u32 gLobbyTime;

/* Starts the menu's wait: a minute before LobbyMenu_TickTimer gives up. */
void LobbyMenu_StartTimer(void)
{
    gLobby->timer = LOBBY_MENU_TIMEOUT;
}

/* Counts the menu's wait down; once it runs out, leaves the lobby
 * (LOBBY_QUIT_FAILED) and returns 1, else 0. */
s32 LobbyMenu_TickTimer(void)
{
    gLobby->timer = gLobby->timer - 1;
    if (gLobby->timer < 0) {
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        return 1;
    }
    return 0;
}

/*
 * Clears the lobby menu's state and gives it its eight draw layers: layers
 * 0-3 without a draw (the screens set theirs with LobbyMenu_SetDraw), 4 the
 * message dialog, 5 the names over the characters, 6 the members' names, 7
 * the village clock. Each layer gets its text size (Text_SetSize). The
 * test of the place calls Game_IsOnline and drops its result.
 */
void LobbyMenu_Init(void)
{
    memset(gLobbyMenu, 0, sizeof(LobbyMenu));
    gLobbyMenu->layer[0] = &gLobbyMenuLayer0;
    gLobbyMenu->layer[1] = &gLobbyMenuLayer1;
    gLobbyMenu->layer[2] = &gLobbyMenuLayer2;
    gLobbyMenu->layer[3] = &gLobbyMenuLayer3;
    gLobbyMenu->layer[4] = &gLobbyMenuLayer4;
    gLobbyMenu->layer[4]->draw = LobbyMenu_DrawDialog;
    gLobbyMenu->layer[5] = &gLobbyMenuLayer5;
    gLobbyMenu->layer[5]->draw = NameTags_DrawLayer;
    gLobbyMenu->layer[6] = &gLobbyMenuLayer6;
    gLobbyMenu->layer[6]->draw = LobbyMenu_DrawMemberNames;
    gLobbyMenu->layer[7] = &gLobbyMenuLayer7;
    gLobbyMenu->layer[7]->draw = VillageClock_Draw;
    gLobbyMenu->layer[0]->textSize = 0;
    gLobbyMenu->layer[5]->textSize = 6;
    gLobbyMenu->layer[6]->textSize = 0;
    gLobbyMenu->layer[7]->textSize = 0;
    gLobbyMenu->layer[1]->textSize = 1;
    gLobbyMenu->layer[2]->textSize = 2;
    gLobbyMenu->layer[3]->textSize = 3;
    gLobbyMenu->layer[4]->textSize = 5;
    if (Lobby_GetPlace() == LOBBY_PLACE_LEVEL0) {
        Game_IsOnline();
    }
}

/* Clears the lobby's pending leave requests, the party's ready flag, the
 * room's close and the awaited reply, then every network request and task. */
void Lobby_ClearRequests(void)
{
    memset(&gLobby->level0Kicked, 0, 3); /* level0Kicked, level1Kicked, level2Kicked */
    gLobby->playersListed = 0;
    gLobby->roomCloseKind = 0;
    gLobby->awaitReply = 0;
    NetReq_ClearAll();
    NetTask_ClearAll();
}

/* 1 once the party's players list came (LobbyState.playersListed): the town's
 * messages are held back then (TownMsg_Send sends only its command 3). */
s32 Lobby_SendHeld(void)
{
    return gLobby->playersListed != 0;
}

extern s8 gConnCheckState;
extern LobbyMenu gLobbyMenuData;                 /* the menu state gLobbyMenu points to */
extern LobbyState gLobbyState;
extern u8 D_008BFB10[];
extern u16 gFriendCheckTimers[FRIEND_CHECK_TIMERS];
extern s8 gFriendStatus[FRIEND_LIST_LEN];
extern u8 gCurServerName[];
#define LOBBY_CLEARED_BLOCK_SIZE 1560   /* the block cleared at the start; no code names it otherwise */
#define CUR_SERVER_NAME_SIZE 128

/*
 * The lobby's start: clears the lobby work buffer up to the downloaded
 * quests' records, the records, the quest list file and the 3840 bytes
 * after it; points gLobbyMenu and gLobby at their states and resets the
 * state's steps, timers, host, dialog, place (LOBBY_PLACE_NONE) and chat
 * targets; resets the network's state, stops the friend list's timers and
 * marks every friend's status unknown; and clears the current server's name.
 * The block after the list file is reached through an integer: the game's
 * code adds the list's offset and its size one by one, as this form does.
 */
void Lobby_ResetState(void)
{
    s32 i;

    memset(gLobbyWorkBuf, 0, QDL_OFFSET);
    memset(QDL_SAVED, 0, QDL_FILE_COUNT * sizeof(NetDlFile));
    memset(QDL_LIST_FILE, 0, QDL_LIST_SIZE);
    memset((u8 *)((s32)QDL_LIST_FILE + QDL_LIST_SIZE), 0, 3840);
    gLobbyMenu = &gLobbyMenuData;
    gLobby = &gLobbyState;
    gLobby->phase = LOBBY_PHASE_ENTRY;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    gLobby->taskStep = 0;
    Lobby_ResetIdleTimer(&gLobby->idleTimer);
    Lobby_ClearHost();
    LobbyDialog_Clear();
    gLobby->recvTimer = LOBBY_RECV_TIMEOUT;
    gLobby->serverClose = 0;
    gLobby->linkState = 0;
    gLobby->unk3968 = 0;
    Lobby_SetPlace(LOBBY_PLACE_NONE);
    Chat_ClearTargets();
    gLobby->areaLoaded = 0;
    gConnCheckState = 0;
    LobbyNet_Reset();
    memset(D_008BFB10, 0, LOBBY_CLEARED_BLOCK_SIZE);
    for (i = 0; i < FRIEND_CHECK_TIMERS; i++) {
        gFriendCheckTimers[i] = FRIEND_TIMER_OFF;
    }
    for (i = 0; i < FRIEND_LIST_LEN; i++) {
        gFriendStatus[i] = FRIEND_STATUS_UNKNOWN;
    }
    Chat_ClearTargets();
    Lobby_ResetTimers();
    Rate_ClearSnapshot();
    memset(gCurServerName, 0, CUR_SERVER_NAME_SIZE);
}

extern u8 gLobbyLevel0Sel;
extern u8 gLobbyLevel1Sel;
extern u8 gLobbyLevel2Sel;
/* The entry selected on the server's level (0-2; LobbyPlace's levels), 0
 * for any other level. */
s32 LobbyLevel_SelId(s32 level)
{
    s32 levelByte;

    levelByte = level & 0xFF;
    switch (levelByte) {
    case LOBBY_PLACE_LEVEL0:
        return gLobbyLevel0Sel;
    case LOBBY_PLACE_LEVEL1:
        return gLobbyLevel1Sel;
    case LOBBY_PLACE_LEVEL2:
        return gLobbyLevel2Sel;
    default:
        return 0U;
    }
}

/* Starts an idle timer from LOBBY_IDLE_TIMEOUT (an hour). */
void Lobby_ResetIdleTimer(s32 *timer)
{
    *timer = LOBBY_IDLE_TIMEOUT;
}

#define RECEIVE_WAIT_MAX 100    /* messages taken in one call */

/* While a screen waits, online, takes up to RECEIVE_WAIT_MAX messages off
 * the socket; each one restarts recvTimer. */
void Lobby_ReceiveWhileWaiting(void)
{
    s32 n;

    if (Game_IsOnline() == 1) {
        n = 0;
        do {
            if (Lobby_ReceiveAll(gNetSock) != 1) {
                break;
            }
            n++;
            gLobby->recvTimer = LOBBY_RECV_TIMEOUT;
        } while (n < RECEIVE_WAIT_MAX);
    }
}

/* Clears the host's id and the chat targets, and puts the members' names
 * back to their saved on/off setting. */
void Lobby_ClearHost(void)
{
    gLobby->hostId[0] = 0;
    Chat_ClearTargets();
    gMemberNamesOff = gMemberNamesOffSaved;
}

/* Sets the draw callbacks of the menu's layers 0-3, the screens' own
 * (NULL: the layer draws nothing). */
void LobbyMenu_SetDraw(void *draw0, void *draw1, void *draw2, void *draw3)
{
    gLobbyMenu->layer[0] = &gLobbyMenuLayer0;
    gLobbyMenu->layer[0]->draw = draw0;
    gLobbyMenu->layer[1] = &gLobbyMenuLayer1;
    gLobbyMenu->layer[1]->draw = draw1;
    gLobbyMenu->layer[2] = &gLobbyMenuLayer2;
    gLobbyMenu->layer[2]->draw = draw2;
    gLobbyMenu->layer[3] = &gLobbyMenuLayer3;
    gLobbyMenu->layer[3]->draw = draw3;
}

void Lobby_DrawBlackLine(void)
{
    Gfx_DrawBlackLineFixed();
}

/* Entry idx of level 2's list (gLobbyLevel2Entries). */
void *LobbyLevel2_Entry(s32 idx)
{
    return &gLobbyLevel2Entries[(s16)idx];
}

/* 1 while a quest is taken. */
s32 Lobby_HasQuest(void)
{
    return gLobby->questTaken != 0;
}

/* Sets the level the player is in (enum LobbyPlace), and inLevel with it. */
void Lobby_SetPlace(s32 place)
{
    switch (place) {
    case LOBBY_PLACE_NONE:
        gLobby->inLevel = 0;
        break;
    case LOBBY_PLACE_LEVEL0:
    case LOBBY_PLACE_LEVEL1:
    case LOBBY_PLACE_LEVEL2:
        gLobby->inLevel = 1;
        break;
    }
    gLobby->place = place;
}

s32 Lobby_GetPlace(void)
{
    return gLobby->place;
}

/* The lobby's clock (gLobbyTime), through the world's functions. */
void LobbyTime_Tick(void)
{
    LobbyWorld_Tick();
}

void LobbyTime_SetClock(u32 time)
{
    LobbyWorld_SetClock(time);
}

f32 LobbyTime_GetDayFrac(void)
{
    return LobbyWorld_GetDayFrac();
}

f32 LobbyTime_GetDayLen30(void)
{
    return LobbyWorld_GetDayLen30();
}

/* 1 before noon. */
s32 LobbyTime_IsAm(void)
{
    return (u32) gLobbyTime < (u32) LOBBY_CLOCK_NOON;
}

/* 1 from noon on. */
s32 LobbyTime_IsPm(void)
{
    return (u32) gLobbyTime >= (u32) LOBBY_CLOCK_NOON;
}

/* Clears the chat's targets: chat lines are said to everyone again. */
void Chat_ClearTargets(void)
{
    s8 i;
    u8 *userId;

    gLobby->chatToTargets = 0;
    for (i = 0, userId = gChatTargets; i < LOBBY_CHAT_TARGETS; i++, userId += LOBBY_USER_ID_LEN) {
        memset(userId, 0, LOBBY_USER_ID_LEN);
    }
}

void Lobby_LoadTextures(void)
{
    Hud_LoadTextures();
    NetMode_LoadTextures(0);
}
