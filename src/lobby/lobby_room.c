/*
 * The lobby's room phase: the quest's party ready to set out.
 *
 * Once the party's players list has come, Lobby_StartRoomPhase enters
 * LOBBY_PHASE_ROOM, whose screen (LobbyRoom_Run) runs two steps
 * (gLobbyRoomSteps): LobbyRoom_FetchParty takes the room's party from the
 * server and makes its members the quest's players, and
 * LobbyRoom_SendLobbyLeave leaves the lobby server and sets the network
 * scene up to go on to the quest. LobbyStep_SetOut is the request step the
 * town runs when the party sets out, and LobbyPkt_QuestPaidSend tells the
 * server the quest's fee was paid.
 *
 * When the room closes under the player (Lobby_HandleLeaveRequest),
 * LobbyRoom_BeginClose enters LOBBY_PHASE_ROOM_CLOSE, whose steps (a table
 * by LobbyState.mode) black the screen out, show the message until a
 * button or 10 seconds, fade out, then leave the quest room and enter the
 * town again (LobbyRoomClose_LeaveRoom).
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "scene.h"
#include "quest.h"
#include "save.h"
#include "stream.h"
#include "pad.h"
#include "text.h"
#include "lobby_room.h"
#include "arena_cup.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_enter.h"
#include "lobby_leave_menus.h"
#include "lobby_util.h"
#include "quest_order_terms.h"
#include "screen_fade.h"

extern void (*gLobbyRoomSteps[])(void);
extern LobbyMenu *gLobbyMenu;
s32 LobbyNet_GetErrText(char *);
extern s32 D_008F8C98;
extern u8 gNetConnStep;
extern u8 gNetConnEnd;
extern u8 gNetEntryMode;
extern u8 gLoginState;
extern u8 gLbsConnectStep;
extern u8 gNetScreenStep;
extern u8 gLoginConnectStep;
extern u8 gNetDropped;
extern s32 gLobbyActive;
extern s32 D_008F8CA0;
extern u8 gNetPartySelf;
extern u8 gNetQuestSeeds[];
extern void (*jtbl_00819680[])(void);   /* the room close's steps */
extern NextQuest gNextQuest;
extern u8 gPlayerIds[SCENE_PLAYERS][8];   /* the players' ids (sub_main) */
extern u8 gMemberNames[SCENE_PLAYERS][17]; /* their names */
extern u8 gPlayerCardTexts[10][256];          /* their texts */
extern ItemStack gPouchBackup[POUCH_SLOTS];

#define LOADING_MSG_ROOM 4      /* Loading_SetMessage's message while the room phase runs */
#define SIDE_ID_LEN 6           /* the bytes of a user id NextQuest.sideIds keeps */
#define ROOM_STREAM_FADE 15     /* frames */

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

/* Sends the quest-paid packet (LobbyPkt_QuestPaid) for Quest_SendPaid, once
 * the quest was taken with its fee paid. */
void LobbyPkt_QuestPaidSend(void)
{
    LobbyPkt_QuestPaid();
}

/* LOBBY_PHASE_ROOM: unless a leave starts (Lobby_HandleLeaveRequest), runs
 * the step of LobbyState.mode, shows the menu's layers and, while the phase
 * lasts, the loading message. */
void LobbyRoom_Run(void)
{
    if (Lobby_HandleLeaveRequest() == 0) {
        gLobbyRoomSteps[gLobby->mode]();
        gLobbyMenu->hideLayers = 0;
        if (gLobby->phase != LOBBY_PHASE_EXIT) {
            Loading_SetMessage(LOADING_MSG_ROOM);
        }
    }
}

/*
 * The room phase's first step: the party task (NetPartyTask_Start) fetches
 * the room's party, and LobbyRoom_PartyDone moves on. Then the party's
 * count, the room's name (also the quest's seeds) and, in an arena cup
 * quest, the versus quest's time limit and win rule from the party's text,
 * with the user ids of sides 2 and 3 swapped. Each member becomes player
 * i: its id, name and text into LobbyState.players and sub_main's tables
 * (gPlayerIds, gMemberNames, gPlayerCardTexts).
 */
void LobbyRoom_FetchParty(void)
{
    u8 swapId[8];   /* a side id kept while the two swap */
    NetParty list;
    s32 i;
    u8 step;

    step = gLobby->step;
    switch (step) {
    case 0:
        gLobby->step = (u8)(step + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_PARTY;
        NetPartyTask_Start(LobbyRoom_PartyDone);
        return;
    case 1:
        LobbyMenu_TickTimer();
        return;
    case 2:
        gLobby->mode++;
        gLobby->step = 0;
        LobbyNet_GetParty(&list);
        gLobby->partyCount = list.count;
        gNetPartySelf = list.unk1;
        memcpy(gLobby->roomName, list.name, sizeof(list.name));
        memcpy(gNetQuestSeeds, gLobby->roomName, sizeof(gLobby->roomName));
        if (ArenaCup_QuestTaken() == 1) {
            gNextQuest.timeLimit = list.text[0];
            gNextQuest.unk5E = list.text[2];
            memset(swapId, 0, 7);
            memcpy(swapId, gNextQuest.sideIds[1], SIDE_ID_LEN);
            memcpy(gNextQuest.sideIds[1], gNextQuest.sideIds[2], SIDE_ID_LEN);
            memcpy(gNextQuest.sideIds[2], swapId, SIDE_ID_LEN);
        }
        for (i = 0; i < gLobby->partyCount; i++) {
            memset(&gLobby->players[i], 0, sizeof(NetUser));
            memcpy(gLobby->players[i].id, list.members[i].id, LOBBY_USER_ID_LEN);
            memcpy(gLobby->players[i].name, list.members[i].name, 16);
            memcpy(gLobby->players[i].text, list.members[i].text, sizeof(list.members[i].text));
            memset(gPlayerIds[i], 0, LOBBY_USER_ID_LEN);
            memcpy(gPlayerIds[i], list.members[i].id, LOBBY_USER_ID_LEN);
            memcpy(gMemberNames[i], list.members[i].name, sizeof(gMemberNames[i]));
            memcpy(gPlayerCardTexts[i], list.members[i].text, sizeof(list.members[i].text));
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/*
 * The room phase's second step: the streams fade out and the leave request
 * (6002) goes out; once LobbyRoom_LobbyLeaveDone moves on, the network
 * scene's connection state is set for the quest: the connection ended
 * (gNetConnEnd), the entry mode 3, the login state 4, the other steps at 0,
 * and the frame of the leave kept.
 */
void LobbyRoom_SendLobbyLeave(void)
{
    u8 step;

    step = gLobby->step;
    switch (step) {
    case 0:
        gLobby->step = (u8)(step + 1);
        LobbyStream_FadeOut(ROOM_STREAM_FADE);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_ROOM_LOBBY_LEAVE;
        LobbyReq_LobbyLeave(LobbyRoom_LobbyLeaveDone);
        return;
    case 1:
        LobbyMenu_TickTimer();
        return;
    case 2:
        D_008F8C98 = 0;
        gNetConnStep = 0;
        gNetConnEnd = 1;
        gNetEntryMode = LOGIN_ENTRY_QUEST_RETURN;
        gLoginState = 4;
        gLbsConnectStep = 0;
        gNetScreenStep = 0;
        gLoginConnectStep = 0;
        gNetDropped = 0;
        gLobbyActive = 0;
        D_008F8CA0 = gFrameTick;
        return;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        return;
    }
}

/* The party task's end: done, the subsystems reset and the step goes on;
 * failed, the room closes with the server's error text. */
void LobbyRoom_PartyDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_PARTY) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                Game_ResetSubsystems();
                gLobby->step++;
            } else {
                gLobby->roomCloseKind = ROOM_CLOSE_WITH_TEXT;
                LobbyNet_GetErrText(gLobby->errText);
            }
        }
    }
}

/* The leave request's reply (6002): the step goes on. */
void LobbyRoom_LobbyLeaveDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_ROOM_LOBBY_LEAVE) {
            *awaited = LOBBY_AWAIT_NONE;
            gLobby->step++;
        }
    }
}

/* The party sets out: sends 6504 (on) and, once done, gives the scene the
 * quest's paid flag. Returns LOBBY_STEP_BUSY while it waits, then
 * LOBBY_STEP_DONE or LOBBY_STEP_FAILED. */
s32 LobbyStep_SetOut(void)
{
    s16 *seq;
    LobbyState *lobby;
    s16 step;

    lobby = gLobby;
    seq = &lobby->seqStep;
    step = *seq;
    switch (step) {
    case 0:
        *seq = (s16)(step + 1);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_SET_OUT;
        LobbyReq_SetOut(1, LobbyStep_SetOutDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gScene.questPaid = lobby->questPaid;
        *seq = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        *seq = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* 6504's reply: done, setOutOn is set; failed, the server's error text is
 * kept and the step fails. */
void LobbyStep_SetOutDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_SET_OUT) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                gLobby->seqStep++;
                gLobby->setOutOn = 1;
            } else {
                LobbyNet_GetErrText(gLobby->errText);
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* Enters LOBBY_PHASE_ROOM_CLOSE: its message is LOBBY_TEXT_ROOM_CLOSED
 * (kind 0) or the server's error text (1), and the requests are cleared. */
void LobbyRoom_BeginClose(u8 kind)
{
    gLobby->phase = LOBBY_PHASE_ROOM_CLOSE;
    gLobby->phaseStep = kind;
    gLobby->mode = 0;
    gLobby->step = 0;
    Lobby_ClearRequests();
}

/* LOBBY_PHASE_ROOM_CLOSE: runs the step of LobbyState.mode. */
void LobbyRoom_RunClose(void)
{
    jtbl_00819680[gLobby->mode]();
}

/* The room close's first step: the screen goes black at once, with the
 * black line drawn, and the loading message stops. */
void LobbyRoomClose_Start(void)
{
    gLobby->mode++;
    LobbyMenu_Init();
    LobbyMenu_SetDraw(Lobby_DrawBlackLine, 0, 0, 0);
    FadeA_Start(FADE_BLACK);
    Loading_Stop();
}

/* Shows the close's message (LobbyRoom_BeginClose's kind) and fades the
 * screen back in. */
void LobbyRoomClose_ShowText(void)
{
    gLobby->mode++;
    gLobby->countdown = 0;
    gLobbyMenu->redraw = 1;
    if (gLobby->phaseStep == 0) {
        LobbyText_Open(LOBBY_TEXT_ROOM_CLOSED);
    } else {
        LobbyDialog_OpenWait(gLobby->errText);
    }
    gLobby->countdown = 20;
    FadeA_Start(FADE_IN_BLACK);
}

#define CLOSE_TEXT_MAX 300      /* frames: 10 s */
#define CLOSE_TEXT_MIN 60       /* frames before a button counts: 2 s */

/* Waits CLOSE_TEXT_MAX frames, or a circle or cross press after
 * CLOSE_TEXT_MIN, then fades out. */
void LobbyRoomClose_WaitButton(void)
{
    u16 pad;

    pad = LobbyPad_Get(0);
    gLobbyMenu->redraw = 1;
    if (gLobby->countdown++ >= CLOSE_TEXT_MAX
        || (gLobby->countdown >= CLOSE_TEXT_MIN && (pad & (PAD_CIRCLE | PAD_CROSS)))) {
        gLobby->mode++;
        FadeA_Start(FADE_OUT_BLACK);
        LobbySe_Play(LOBBY_SE_CONFIRM);
    }
}

/* Goes on once the fade out ends. */
void LobbyRoomClose_WaitFade(void)
{
    gLobbyMenu->redraw = 1;
    if ((FadeA_GetState() & 0xFF) != FADE_STATE_RUNNING) {
        gLobby->mode = (u8) (gLobby->mode + 1);
        gLobby->step = 0;
    }
}

/* The room close's last step: the subsystems and the lobby's textures
 * reset, the menu's layers hidden, the pending payment made; then the
 * quest room is left (LobbyStep_Level2Leave) and the town entered again
 * (LobbyEnter_Begin). */
void LobbyRoomClose_LeaveRoom(void)
{
    u8 step;

    step = gLobby->step;
    switch (step) {
    case 0:
        gLobby->step = (u8) (step + 1);
        gLobby->seqStep = 0;
        Game_ResetSubsystems();
        Lobby_LoadTextures();
        gLobbyMenu->hideLayers = 1;
        gLobby->connected = 1;
        Money_PayPending(1U);
        break;
    case 1:
        if (LobbyStep_Level2Leave() == 1) {
            LobbyEnter_Begin();
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/* The room phase's steps (LobbyState.mode) and the room close's. */
void (*gLobbyRoomSteps[2])(void) = {
    LobbyRoom_FetchParty, LobbyRoom_SendLobbyLeave,
};
void (*jtbl_00819680[8])(void) = {
    LobbyRoomClose_Start, LobbyRoomClose_ShowText, LobbyRoomClose_WaitButton, LobbyRoomClose_WaitFade,
    LobbyRoomClose_LeaveRoom, NULL, NULL, NULL,
};
