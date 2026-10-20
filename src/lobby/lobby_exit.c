/*
 * The lobby's state and its exit.
 *
 * The lobby keeps its state in LobbyState (gLobby) and its menu's draw
 * layers in LobbyMenu (gLobbyMenu). This file sets them up and resets them:
 * Lobby_ResetState at the lobby's start, LobbyMenu_Init and
 * LobbyMenu_SetDraw for the menu's eight draw layers, and the small
 * accessors the other lobby files call (the place, the level selections,
 * the clock, the chat targets).
 *
 * It also holds the lobby's way out. Lobby_Disconnect sets the quit reason
 * and enters LOBBY_PHASE_EXIT, whose step functions (a table by reason)
 * fade the screen, send the leave or logout request
 * (LobbyLeave_Run), close the socket and go on to the next screen:
 * LobbyExit_RunPlanned for the player's own leaving (LOBBY_QUIT_RECONNECT
 * to LOBBY_QUIT_TO_GMSUB_KEEP), LobbyExit_RunCut for a session cut short
 * (LOBBY_QUIT_IDLE to LOBBY_QUIT_SENT_OUT), which shows the reason first.
 * Lobby_StartRoomPhase moves on to the room phase once the party is ready.
 */
#include "common.h"
#include "em.h"
#include "lobby_state.h"
#include "item.h"
#include "save.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "scene.h"
#include "player.h"
#include "pad.h"
#include "task.h"

/* prototypes */
void Money_PayPending(u8 silent);
void EquipPreview_Hide(u8 hide);
void ServerList_ClearTried(void);
void ServerList_MarkTried(void);
void LobbyMenu_StartTimer(void);
s32 LobbyMenu_TickTimer(void);
s32 Lobby_SendHeld(void);
s32 LobbyLevel_SelId(s32 level);
void Lobby_ResetIdleTimer(s32 *timer);
void Lobby_ClearHost(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Lobby_DrawBlackLine(void);
s32 Lobby_HasQuest(void);
void LobbyTime_Tick(void);
void LobbyTime_SetClock(u32 time);
f32 LobbyTime_GetDayFrac(void);
f32 LobbyTime_GetDayLen30(void);
s32 LobbyTime_IsAm(void);
s32 LobbyTime_IsPm(void);
void Lobby_LoadTextures(void);
void Lobby_UpdatePlayerText(void);
void LobbyLeave_Reset(void);
void LobbyExit_Draw(MenuTask *layer);
void LobbyPkt_QuestPaidSend(void);
void EquipPreview_Free(void);
void Loading_Reset(void);
extern u8 gLobbyTextDirty;
extern s8 gServerSkipTried;
/* end of prototypes */
s32 Lobby_ReceiveAll(s32 sock);
extern u8 gChatTargets[];
void LobbyText_Open(s32 text);
void LobbyDialog_OpenWait(void *text);
void FileQueue_Cancel(void);
void Lobby_SetPlace(s32 place);
extern ItemStack gPouchBackup[POUCH_SLOTS];

s32 Lobby_GetPlace(void);
s32 Game_IsOnline(void);
void *memset(void *dst, s32 c, u32 n);
extern LobbyMenu *gLobbyMenu;
extern MenuTask gLobbyMenuLayer0;
extern MenuTask gLobbyMenuLayer1;
extern MenuTask gLobbyMenuLayer2;
extern MenuTask gLobbyMenuLayer3;
extern MenuTask gLobbyMenuLayer4;
extern MenuTask gLobbyMenuLayer5;
extern MenuTask gLobbyMenuLayer6;
extern MenuTask gLobbyMenuLayer7;
void VillageClock_Draw(void);
void LobbyMenu_DrawMemberNames(MenuTask *task);
void NameTags_DrawLayer(MenuTask *task);
void LobbyMenu_DrawDialog(MenuTask *layer);
extern u8 gLobbyLeaveStep;
void Lobby_Disconnect(s32);
void Chat_ClearTargets(void);
extern u8 gMemberNamesOff;
void Gfx_DrawBlackLineFixed(void);
void LobbyWorld_Tick(void);
void LobbyWorld_SetClock(u32 time);
f32 LobbyWorld_GetDayFrac(void);
f32 LobbyWorld_GetDayLen30(void);
extern u32 gLobbyTime;
void Hud_LoadTextures(void);
void NetMode_LoadTextures(s32);
extern s8 gNetDisconnectStep;
void Text_SetSize(s32);
s32 LobbyPkt_QuestPaid(void);

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

void NetReq_ClearAll(void);
void NetTask_ClearAll(void);

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

s32 LobbyDialog_Clear(void);
void LobbyNet_Reset(void);
void Lobby_ResetTimers(void);
void Rate_ClearSnapshot(void);
extern s8 gConnCheckState;
extern LobbyMenu gLobbyMenuData;                 /* the menu state gLobbyMenu points to */
extern LobbyState gLobbyState;
extern u8 D_008BFB10[];
extern u16 gFriendCheckTimers[FRIEND_CHECK_TIMERS];
extern s8 gFriendStatus[FRIEND_LIST_LEN];
extern u8 gCurServerName[];
#define LOBBY_CLEARED_BLOCK_SIZE 0x618  /* the block cleared at the start; no code names it otherwise */
#define CUR_SERVER_NAME_SIZE 0x80

/*
 * The lobby's start: clears the lobby work buffer up to the downloaded
 * quests' records, the records, the quest list file and the 0xF00 bytes
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
    memset((u8 *)((s32)QDL_LIST_FILE + QDL_LIST_SIZE), 0, 0xF00);
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

extern LobbyMenu *gLobbyMenu;
extern MenuTask gLobbyMenuLayer0;
extern MenuTask gLobbyMenuLayer1;
extern MenuTask gLobbyMenuLayer2;
extern MenuTask gLobbyMenuLayer3;
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

void HunterCard_Build(void *out);
s32 memcmp(void *a, void *b, s32 n);
void *memcpy(void *dst, const void *src, u32 n);
extern NetUser gNetUsers[];

/*
 * Builds the player's text (the hunter card, HunterCard_Build) and, when it
 * differs from gPlayerText, copies it into every copy: the player's member
 * entry, LobbyState.myText, gPlayerText and the player's network user.
 * The buffer is 0xE0 bytes though 0x100 are compared and copied: the
 * game's stack frame has that size.
 */
void Lobby_UpdatePlayerText(void)
{
    u8 text[0xE0];

    HunterCard_Build(text);
    if (memcmp(text, gPlayerText.raw, sizeof(gLobby->myText)) != 0) {
        memcpy(gLobby->members[gScene.localPlayer].text, text, sizeof(gLobby->myText));
        memcpy(gLobby->myText, text, sizeof(gLobby->myText));
        memcpy(gPlayerText.raw, text, sizeof(gLobby->myText));
        memcpy(gNetUsers[gScene.localPlayer].text, text, sizeof(gLobby->myText));
    }
}

/*
 * Leaves the lobby server for a reason (LOBBY_QUIT_*): enters
 * LOBBY_PHASE_EXIT with the reason as its mode, sends the player's changed
 * text first, pays what is pending without a message, puts the equipment
 * preview away and clears the menu's draws. A player sent out
 * (LOBBY_QUIT_SENT_OUT) marks the server tried, so the server list skips
 * it; any other reason clears the tried servers.
 */
void Lobby_Disconnect(s32 reason)
{
    gLobby->phase = LOBBY_PHASE_EXIT;
    gLobby->phaseStep = 0;
    gLobby->mode = reason;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->seqStep = 0;
    if (gLobbyTextDirty != 0) {
        Lobby_UpdatePlayerText();
        gLobbyTextDirty = 0;
    }
    gLobby->quitReason = reason;
    Money_PayPending(1U);
    EquipPreview_Hide(1U);
    EquipPreview_Free();
    LobbyMenu_SetDraw(0, 0, NULL, 0);
    if ((reason & 0xFF) == LOBBY_QUIT_SENT_OUT) {
        gServerSkipTried = 1;
        ServerList_MarkTried();
    } else {
        ServerList_ClearTried();
    }
    Loading_Reset();
}

extern void (*jtbl_00819650[])(void);
/* LOBBY_PHASE_EXIT: runs the step function of the quit reason
 * (LobbyState.mode). */
void LobbyExit_Run(void)
{
    if (gLobby->mode > LOBBY_QUIT_SENT_OUT) {
        gLobby->mode = LOBBY_QUIT_RECONNECT;
    }
    jtbl_00819650[gLobby->mode]();
}


s32 LobbyLeave_Run(u8 reason);
void LobbyLeave_Reset(void);
void LobbyExit_Draw(MenuTask *layer);
void LobbyStream_FadeOut(s32 frames);
void FadeA_Start(s32);
u8 FadeA_GetState(void);
void Stream_Stop(s32 ch);
void Kbd_Cancel(void);
void LobbyExit_ShowReason(s32 reason);
s32 NetTcp_Abort(s32 sock);
s32 NetTcp_Delete(s32 *sock);
extern s32 gNetSock;
void LobbySe_Play(s32 se);
void Game_ResetSubsystems(void);
void NetScene_ToState4b(void);
void NetScene_ToState6(void);
extern u8 gNetEntryMode;
u16 LobbyPad_Get(s32 a);
void LobbyStream_Stop(void);
void Hud_StartQuest(void);
void Area_FreeAll(void);
void Lobby_SetBootMode(s32 mode);
void NetScene_ToNetFileCheck(void);

#define EXIT_STREAM_FADE 15     /* frames */

/*
 * The exit for the player's own leaving (LOBBY_QUIT_RECONNECT to
 * LOBBY_QUIT_TO_GMSUB_KEEP), by LobbyState.step:
 * - 0: with a reason other than LOBBY_QUIT_RECONNECT, the screen and the
 *   streams fade out (step 1); for LOBBY_QUIT_RECONNECT straight to step 3;
 * - 1: once the fade ends, the streams stop and, unless the local player
 *   is in state 0xF, the screen fades back in, the lobby's music stops and
 *   the quest's HUD starts;
 * - 3: the leave starts, with its message (the idle one for LOBBY_QUIT_IDLE),
 *   and the areas are freed;
 * - 4, 5: LobbyLeave_Run, then the socket is closed;
 * - 6, 7: a last fade out, then the next screen by the reason: the network
 *   scene's connection (LOBBY_QUIT_RECONNECT), the portal's page 3
 *   (LOBBY_QUIT_TO_PORTAL), or the network scene's end with the boot mode
 *   set (LOBBY_QUIT_TO_GMSUB, _TO_GMSUB_KEEP).
 */
void LobbyExit_RunPlanned(void)
{
    switch (gLobby->step) {
    case 0:
        LobbyMenu_Init();
        Lobby_SetPlace(LOBBY_PLACE_NONE);
        if (gLobby->quitReason != LOBBY_QUIT_RECONNECT) {
            gLobby->step++;
            LobbyMenu_Init();
            LobbyMenu_SetDraw(LobbyExit_Draw, 0, 0, 0);
            LobbyStream_FadeOut(EXIT_STREAM_FADE);
            FadeA_Start(FADE_BLACK);
        } else {
            gLobby->step = 3;
        }
        break;
    case 1:
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            if (gLobby->quitReason != LOBBY_QUIT_RECONNECT) {
                Stream_Stop(0);
                Stream_Stop(1);
                if (gPlayers[gScene.localPlayer].state != 0xF) {
                    FadeA_Start(FADE_IN_BLACK);
                    LobbyStream_Stop();
                    Hud_StartQuest();
                    gLobby->step += 2;
                } else {
                    gLobby->step += 2;
                }
            } else {
                gLobby->step += 2;
            }
            Kbd_Cancel();
        }
        break;
    case 2:
        gLobby->step++;
        break;
    case 3:
        gLobby->step++;
        LobbyLeave_Reset();
        Kbd_Cancel();
        if (gLobby->quitReason == LOBBY_QUIT_IDLE) {
            LobbyText_Open(LOBBY_TEXT_IDLE_CUT);
            FadeA_Start(FADE_IN_BLACK);
        } else if (gLobby->quitReason != LOBBY_QUIT_RECONNECT) {
            FadeA_Start(FADE_IN_BLACK);
            LobbyText_Open(LOBBY_TEXT_DISCONNECTING);
        }
        Area_FreeAll();
        break;
    case 4:
        gLobby->step++;
    case 5:
        gLobbyMenu->redraw = 1;
        if (LobbyLeave_Run(gLobby->quitReason) != 0) {
            gLobby->step++;
            NetTcp_Abort(gNetSock);
            NetTcp_Delete(&gNetSock);
        }
        break;
    case 6:
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            gLobby->step++;
            if (gLobby->quitReason != LOBBY_QUIT_RECONNECT) {
                gLobbyMenu->redraw = 1;
                FadeA_Start(FADE_OUT_BLACK);
            }
        }
        break;
    case 7:
        if ((u8)FadeA_GetState() == FADE_STATE_RUNNING) {
            if (gLobby->quitReason != LOBBY_QUIT_RECONNECT) {
                gLobbyMenu->redraw = 1;
            }
            return;
        }
        {
            switch (gLobby->quitReason) {
            case LOBBY_QUIT_RECONNECT:
                NetScene_ToState6();
                break;
            case LOBBY_QUIT_TO_GMSUB_KEEP:
                Lobby_SetBootMode(BOOT_MODE_GMSUB_KEEP);
                NetScene_ToNetFileCheck();
                break;
            case LOBBY_QUIT_TO_PORTAL:
                gNetEntryMode = LOGIN_ENTRY_FIRST;
                gPortalPage = PORTAL_PAGE_QUIT;
                NetScene_ToState6();
                break;
            case LOBBY_QUIT_TO_GMSUB:
                Lobby_SetBootMode(BOOT_MODE_GMSUB);
                NetScene_ToNetFileCheck();
                gPortalPage = PORTAL_PAGE_TOP;
                break;
            }
        }
        break;
    }
}

#define SENT_OUT_WAIT 90        /* frames: 3 s */

/*
 * The exit for a session cut short (LOBBY_QUIT_IDLE to LOBBY_QUIT_SENT_OUT),
 * by LobbyState.step: the screen goes black at once and the streams fade
 * out (step 0 goes on at step 4); the leave starts with its message
 * (step 4); LobbyLeave_Run, then the reason's message (LobbyExit_ShowReason)
 * and the socket closed (step 7); PAD_CIRCLE (steps 8 and 9, which a player
 * sent out goes to); and after a fade out, back to the network scene: its
 * start (LOBBY_QUIT_IDLE) or its connection with the portal's page 3.
 */
void LobbyExit_RunCut(void)
{
    u16 pad = LobbyPad_Get(0);

    switch (gLobby->step) {
    case 0:
        LobbyMenu_Init();
        Lobby_SetPlace(LOBBY_PLACE_NONE);
        gLobbyMenu->hideLayers = 1;
        gLobby->step++;
        LobbyMenu_SetDraw(LobbyExit_Draw, 0, 0, 0);
        if (gLobby->quitReason != LOBBY_QUIT_RECONNECT) {
            LobbyStream_FadeOut(EXIT_STREAM_FADE);
        }
        gLobby->step = 4;
        FadeA_Start(FADE_BLACK);
        break;
    case 1:
        gLobbyMenu->hideLayers = 1;
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            gLobby->step++;
            if (gLobby->quitReason != LOBBY_QUIT_RECONNECT) {
                Stream_Stop(0);
                Stream_Stop(1);
            }
            Kbd_Cancel();
        }
        break;
    case 2:
        gLobbyMenu->hideLayers = 1;
        gLobby->step++;
        gLobby->connected = 0;
        break;
    case 3:
        gLobbyMenu->hideLayers = 1;
        gLobby->connected = 1;
        gLobby->step++;
        break;
    case 4:
        gLobbyMenu->hideLayers = 1;
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            gLobby->step++;
            LobbyLeave_Reset();
            FadeA_Start(FADE_IN_BLACK);
            LobbyText_Open(LOBBY_TEXT_DISCONNECTING);
            Kbd_Cancel();
        }
        break;
    case 5:
        gLobbyMenu->redraw = 1;
        gLobby->step++;
        break;
    case 6:
        gLobbyMenu->redraw = 1;
        gLobby->step = 7;
    case 7:
        gLobbyMenu->redraw = 1;
        if (LobbyLeave_Run(gLobby->quitReason) != 0) {
            LobbyExit_ShowReason((s8)gLobby->quitReason);
            if (gLobby->quitReason == LOBBY_QUIT_SENT_OUT) {
                gLobby->step = 9;
                gLobby->countdown = SENT_OUT_WAIT;
            } else {
                gLobby->step++;
            }
            NetTcp_Abort(gNetSock);
            NetTcp_Delete(&gNetSock);
        }
        break;
    case 8:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            gLobby->step = 10;
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
            FadeA_Start(FADE_OUT_BLACK);
        }
        break;
    case 9:
        gLobbyMenu->redraw = 1;
        if (pad & PAD_CIRCLE) {
            gLobby->step++;
            FadeA_Start(FADE_OUT_BLACK);
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        }
        break;
    case 10:
        if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
            Game_ResetSubsystems();
            gLobbyMenu->hideLayers = 1;
            switch (gLobby->quitReason) {
            case LOBBY_QUIT_IDLE:
                NetScene_ToState4b();
                gPortalPage = PORTAL_PAGE_TOP;
                break;
            case LOBBY_QUIT_FAILED:
            case LOBBY_QUIT_SERVER_CLOSE:
            case LOBBY_QUIT_LINK_LOST:
            case LOBBY_QUIT_SENT_OUT:
                NetScene_ToState6();
                gNetEntryMode = LOGIN_ENTRY_FIRST;
                gPortalPage = PORTAL_PAGE_QUIT;
                break;
            }
        }
        break;
    }
}

/* Shows the message for a quit reason: a text of the message table, or
 * the server's error text (LOBBY_QUIT_SERVER_CLOSE, _SENT_OUT); nothing
 * for LOBBY_QUIT_RECONNECT, _TO_PORTAL and _TO_GMSUB_KEEP. */
void LobbyExit_ShowReason(s32 reason)
{
    Loading_Reset();
    switch ((s8)reason) {
    case LOBBY_QUIT_RECONNECT:
    case LOBBY_QUIT_TO_PORTAL:
    case LOBBY_QUIT_TO_GMSUB_KEEP:
        break;
    case LOBBY_QUIT_TO_GMSUB:
        LobbyText_Open(LOBBY_TEXT_QUIT_GMSUB);
        break;
    case LOBBY_QUIT_IDLE:
        LobbyText_Open(LOBBY_TEXT_IDLE_QUIT);
        break;
    case LOBBY_QUIT_FAILED:
    case LOBBY_QUIT_LINK_LOST:
        LobbyText_Open(LOBBY_TEXT_LINK_LOST);
        break;
    case LOBBY_QUIT_SERVER_CLOSE:
    case LOBBY_QUIT_SENT_OUT:
        LobbyDialog_OpenWait(gLobby->errText);
        break;
    }
}

void LobbyLeave_Reset(void)
{
    gLobbyLeaveStep = 0;
    gNetDisconnectStep = 0;
}

void LobbyLeave_OnReply(void);
s32 LobbyReq_LobbyLeave(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_Logout(void (*onReply)(NetMsg, NetMsg *));
s32 NetIf_GetError(void);
s32 LobbyNet_DisconnectWait(void);

#define LEAVE_REPLY_WAIT 1800   /* frames: 60 s */
#define LEAVE_PAUSE 60          /* frames: 2 s */

/*
 * Leaves the lobby server for a quit reason, by gLobbyLeaveStep. In the
 * lobby (gScene.inLobby), the leave request goes out (6002, for
 * LOBBY_QUIT_RECONNECT, _TO_PORTAL and _SENT_OUT, the reasons that stay on
 * the network) or the logout (6006, for the others but LOBBY_QUIT_LINK_LOST,
 * which sends nothing); then a wait for the reply, an error or
 * LEAVE_REPLY_WAIT, and a pause (LEAVE_PAUSE, a frame outside the lobby).
 * Returns 1 once done, after LobbyNet_DisconnectWait for the reasons that
 * end the network (LOBBY_QUIT_TO_GMSUB, _TO_GMSUB_KEEP, _IDLE); else 0.
 */
s32 LobbyLeave_Run(u8 reason)
{
    s32 done = 0;

    switch (gLobbyLeaveStep) {
    case 0:
        gLobbyLeaveStep++;
        break;
    case 1:
        gLobby->countdown = LEAVE_REPLY_WAIT;
        if (gScene.inLobby != 0) {
            switch (reason) {
            case LOBBY_QUIT_RECONNECT:
            case LOBBY_QUIT_TO_PORTAL:
            case LOBBY_QUIT_SENT_OUT:
                gLobbyLeaveStep++;
                gLobby->awaitReply = LOBBY_AWAIT_LOBBY_LEAVE;
                LobbyReq_LobbyLeave((void (*)(NetMsg, NetMsg *))LobbyLeave_OnReply); /* defined (void): it ignores the reply */
                break;
            case LOBBY_QUIT_TO_GMSUB:
            case LOBBY_QUIT_TO_GMSUB_KEEP:
            case LOBBY_QUIT_IDLE:
            case LOBBY_QUIT_FAILED:
            case LOBBY_QUIT_SERVER_CLOSE:
                gLobbyLeaveStep++;
                gLobby->awaitReply = LOBBY_AWAIT_LOBBY_LEAVE;
                LobbyReq_Logout((void (*)(NetMsg, NetMsg *))LobbyLeave_OnReply);
                break;
            case LOBBY_QUIT_LINK_LOST:
                gLobbyLeaveStep = 3;
                break;
            }
        } else {
            gLobbyLeaveStep = 3;
        }
        break;
    case 2:
        gLobby->countdown--;
        if (NetIf_GetError() != 0 || gLobby->countdown < 0) {
            gLobbyLeaveStep++;
        }
        break;
    case 3:
        gLobbyLeaveStep++;
        if (gScene.inLobby != 0) {
            gLobby->countdown = LEAVE_PAUSE;
        } else {
            gLobby->countdown = 1;
        }
        break;
    case 4:
        if (--gLobby->countdown < 0) {
            gNetDisconnectStep = 0;
            gLobbyLeaveStep++;
        }
        break;
    case 5:
        switch (reason) {
        case LOBBY_QUIT_RECONNECT:
        case LOBBY_QUIT_TO_PORTAL:
        case LOBBY_QUIT_FAILED:
        case LOBBY_QUIT_SERVER_CLOSE:
        case LOBBY_QUIT_LINK_LOST:
        case LOBBY_QUIT_SENT_OUT:
            done = 1;
            break;
        case LOBBY_QUIT_TO_GMSUB:
        case LOBBY_QUIT_TO_GMSUB_KEEP:
        case LOBBY_QUIT_IDLE:
            if (LobbyNet_DisconnectWait() != 0) {
                done = 1;
            }
            break;
        }
        break;
    }
    return done;
}

/* The leave's or the logout's reply: ends LobbyLeave_Run's wait. */
void LobbyLeave_OnReply(void)
{
    if (gLobby->awaitReply == LOBBY_AWAIT_LOBBY_LEAVE) {
        gLobby->awaitReply = 0U;
        if (gLobbyLeaveStep == 2) {
            gLobbyLeaveStep += 1;
        }
    }
}

/* The exit's draw: the black line at the screen's foot. */
void LobbyExit_Draw(MenuTask *layer)
{
    Text_SetSize(layer->textSize);
    Gfx_DrawBlackLineFixed();
}

/* Once the party is ready (Lobby_HandleLeaveRequest): enters
 * LOBBY_PHASE_ROOM, cancels the file queue, clears the requests and the
 * place, and keeps a copy of the pouch (gPouchBackup). */
void Lobby_StartRoomPhase(void)
{
    s32 i;

    gLobby->phase = LOBBY_PHASE_ROOM;
    gLobby->phaseStep = 0;
    gLobby->mode = 0;
    gLobby->step = 0;
    FileQueue_Cancel();
    Lobby_ClearRequests();
    Lobby_SetPlace(LOBBY_PLACE_NONE);
    for (i = 0; i < POUCH_SLOTS; i++) {
        gPouchBackup[i].item = gSave.pouch[i].item;
        gPouchBackup[i].count = gSave.pouch[i].count;
    }
}

void LobbyPkt_QuestPaidSend(void)
{
    LobbyPkt_QuestPaid();
}

/* The exit's step functions, by quit reason (LobbyExit_Run). */
void (*jtbl_00819650[10])(void) = {
    LobbyExit_RunPlanned, LobbyExit_RunPlanned, LobbyExit_RunPlanned, LobbyExit_RunPlanned,
    LobbyExit_RunCut, LobbyExit_RunCut, LobbyExit_RunCut, LobbyExit_RunCut,
    LobbyExit_RunCut, NULL,
};
