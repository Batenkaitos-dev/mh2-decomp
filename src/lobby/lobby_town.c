/*
 * The lobby's town phase (LOBBY_PHASE_TOWN), and the requests the town makes
 * of the server's second and third levels.
 *
 * - The phase: LobbyTown_Begin starts it once a town (a level-1 entry) has
 *   been entered, LobbyTown_BeginRunning when the town's screen is already
 *   in, LobbyTown_BeginLeave when the player leaves for the village.
 *   LobbyTown_Run is the phase's frame, by LobbyState.mode: LobbyTown_Enter
 *   brings the town's screen in and runs the town (LobbyScene_Run);
 *   LobbyTown_Leave shows the server's text with a countdown, leaves the
 *   town's level, clears the other players and goes back to the village
 *   (LobbyVillage_BeginReturn).
 * - The second level (the towns): the search for groups
 *   (LobbyLevel1_Search, into gGroupSearchResults) and one group's details
 *   (LobbyLevel1_FetchInfo, into gGroupSearchInfo), which the group board
 *   shows.
 * - The third level (the quest rooms of the town, gLobbyLevel2Entries): the
 *   rooms' list, opening a free room for a quest, its records and
 *   settings, the quest's acceptance and posting, entering a room with its
 *   password, and the room's users and member count.
 *
 * Each request is a step function run once a frame on LobbyState.seqStep:
 * it sends the request with its awaitReply tag (enum LobbyAwait), waits, and
 * returns LOBBY_STEP_BUSY, then LOBBY_STEP_DONE or LOBBY_STEP_FAILED. The
 * reply handlers move seqStep on, or to LOBBY_SEQ_FAILED with the server's
 * error text. They read gLobby through a local pointer and the tag through
 * its address, the form that gives the game's loads.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_globals.h"
#include "lobby_text.h"
#include "scene.h"
#include "player.h"
#include "pad.h"
#include "kbd.h"
#include "model.h"
#include "text.h"
#include "lobby_town.h"
#include "elder_forecast.h"
#include "equip_shop.h"
#include "game_util.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_leave_menus.h"
#include "lobby_queries.h"
#include "lobby_screen.h"
#include "lobby_steps.h"
#include "lobby_util.h"
#include "lobby_village.h"
#include "mem_util.h"
#include "quest.h"
#include "town_members.h"
#include "screen_fade.h"
#include "libc.h"

s32 LobbyNet_GetErrText(s8 *);

extern LevelEntry gLobbyLevel1Entries[];
extern NetLevel1SearchReply *gGroupSearchResults;
extern NetLevel1SearchInfo gGroupSearchInfo;
extern u8 gLobbyLevel2Sel;
s32 NetList2_GetMembers(s32 id, u16 *a, u16 *b);
extern u8 gLobbyLevel2Index;
extern char gMyUserId[];
extern char gMyName[];
extern u16 gLobbyLevel2Count;

extern LobbyRecs gLobbyRecs;
extern LobbyMenu *gLobbyMenu;
extern char gRoomText[61];          /* the quest room's text */
extern u16 gLobbyQuestId;             /* the quest */
s32 NetLevel2RecsTask_GetRecs(u16 id, NetRecs2 *dst); /* unit's own declaration: a u16 id, as the callers pass it; with the definition's s32, LobbyLevel2_FetchRecords compiles one instruction differently */
u8 Quest_GetListFlags(u16 id);
extern u8 gLobbyScreenMode;
struct Player;
extern u8 gHouseAccess;
extern u8 gChatToLobby;
extern u8 gLobbyTextDirty;
extern NetUser gNetUsers[];

/* Starts the town phase once its entry has been entered: the lobby state's
 * steps back to the start (LOBBY_TOWN_ENTER), the town's rooms and room
 * choice cleared, no party set out, the player's pad input given back to
 * the field, and the town entry's users asked for (6303). */
void LobbyTown_Begin(void)
{
    gLobby->phase = LOBBY_PHASE_TOWN;
    gLobby->phaseStep = 0;
    gLobby->mode = LOBBY_TOWN_ENTER;
    gLobby->step = LOBBY_TOWN_STEP_PLACE;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    Lobby_ClearRequests();
    memset(gLobbyLevel2Entries, 0, sizeof(gLobbyLevel2Entries));
    gLobbyLevel2Sel = 0;
    gLobby->setOutOn = 0;
    gFieldControl = 1;
    Chat_ClearTargets();
    LobbyReq_Level1Values(LobbyLevel_SelId(LOBBY_PLACE_LEVEL1) & 0xFFFF, &LobbyTown_OnUsersReply);
}

/* As LobbyTown_Begin, for a town whose screen is already in (the lobby's
 * re-entry): the town starts at LOBBY_TOWN_STEP_RUN. */
void LobbyTown_BeginRunning(void)
{
    gLobby->phase = LOBBY_PHASE_TOWN;
    gLobby->phaseStep = 0;
    gLobby->mode = LOBBY_TOWN_ENTER;
    gLobby->step = LOBBY_TOWN_STEP_RUN;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    Lobby_ClearRequests();
    memset(gLobbyLevel2Entries, 0, sizeof(gLobbyLevel2Entries));
    gLobbyLevel2Sel = 0;
    gLobby->setOutOn = 0;
    gFieldControl = 1;
    Chat_ClearTargets();
    LobbyReq_Level1Values(LobbyLevel_SelId(LOBBY_PLACE_LEVEL1) & 0xFFFF, &LobbyTown_OnUsersReply);
}

/* Starts leaving the town (LOBBY_TOWN_LEAVE): the keyboard closed, and the
 * leave of the town's level and of its room marked as the player's own. */
void LobbyTown_BeginLeave(void)
{
    gLobby->phase = LOBBY_PHASE_TOWN;
    gLobby->phaseStep = 0;
    gLobby->mode = LOBBY_TOWN_LEAVE;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    Lobby_ClearRequests();
    Kbd_Cancel();
    gLobby->level1Kicked = LEAVE_BEGUN;
    gLobby->level2Kicked = LEAVE_BEGUN;
}

/* 1 while the town is being left and its leave has not started a step of
 * its own (phaseStep 0), else 0. */
s32 LobbyTown_IsLeaving(void)
{
    if ((gLobby->phase == LOBBY_PHASE_TOWN) && (gLobby->mode == LOBBY_TOWN_LEAVE) && (gLobby->phaseStep == 0)) {
        return 1;
    }
    return 0;
}

/* The town phase's frame: nothing while a leave the server asked for is
 * handled (the menu layers come back if the town was running), else the
 * town's entry and run or its leave by LobbyState.mode. A mode it does not
 * know leaves the lobby. */
void LobbyTown_Run(void)
{
    u8 mode;

    if (Lobby_HandleLeaveRequest() != 0) {
        if ((gLobby->mode == LOBBY_TOWN_ENTER) && (gLobby->step == LOBBY_TOWN_STEP_RUN)) {
            gLobbyMenu->hideLayers = 0;
        }
        return;
    }
    mode = gLobby->mode;
    switch (mode) {
    case LOBBY_TOWN_ENTER:
        LobbyTown_Enter();
        return;
    case LOBBY_TOWN_LEAVE:
        LobbyTown_Leave();
        return;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        return;
    }
}

/*
 * The town's entry and run, by LobbyState.step (enum LobbyTownStep): the
 * place becomes the town; the menus fade out and the town's screen comes
 * in (LOBBY_ENTER_TOWN_MEMBERS after a group's join, which is then
 * dropped, else LOBBY_ENTER_TOWN); then the town runs (LobbyScene_Run)
 * until the raid's movie is due. After the movie, the area becomes the one
 * to come back to after a quest, every player's model and look are marked
 * to load again, and the screen comes back in with the members.
 */
void LobbyTown_Enter(void)
{
    u8 *step = &gLobby->step;

    switch (*step) {
    case LOBBY_TOWN_STEP_PLACE:
        (*step)++;
        gLobby->subStep = 0;
        Lobby_SetPlace(LOBBY_PLACE_LEVEL1);
    case LOBBY_TOWN_STEP_FADE_OUT:
        switch (LobbyScreen_FadeOut(gLobby->groupJoin ? LOBBY_ENTER_TOWN_MEMBERS : LOBBY_ENTER_TOWN)) {
        case LOBBY_STEP_DONE:
            gLobby->step++;
            break;
        }
        break;
    case LOBBY_TOWN_STEP_ENTER:
        switch (LobbyScreen_EnterSeq(gLobby->groupJoin ? LOBBY_ENTER_TOWN_MEMBERS : LOBBY_ENTER_TOWN)) {
        case LOBBY_STEP_DONE:
            gLobby->step++;
            gLobby->subStep = 0;
            gLobbyScreenMode = LOBBY_SCREEN_TOWN;
            if (gLobby->groupJoin != 0) {
                memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
            }
            if (FadeB_GetState() != FADE_STATE_IDLE) {
                FadeB_Start(FADE_IN_BLACK_FAST);
            }
            break;
        }
        break;
    case LOBBY_TOWN_STEP_RUN:
        if (LobbyTown_WantsMovie() == 1) {
            gLobby->step = LOBBY_TOWN_STEP_MOVIE;
            LobbyTown_StartMovie();
        } else {
            LobbyScene_Run();
        }
        break;
    case LOBBY_TOWN_STEP_MOVIE:
        if (LobbyTown_MovieDone() == 1) {
            gLobby->movieState = RAID_MOVIE_NONE;
            gQuestReturnArea = gScene.area;
            memset(gLobby->pcLoaded, 0, sizeof(gLobby->pcLoaded));
            memset(gLobby->lookPending, 0, LOBBY_MEMBER_COUNT);
            gLobby->step++;
        }
        break;
    case LOBBY_TOWN_STEP_REENTER:
        switch (LobbyScreen_EnterSeq(LOBBY_ENTER_TOWN_MEMBERS)) {
        case LOBBY_STEP_DONE:
            gLobby->step = LOBBY_TOWN_STEP_RUN;
            gLobby->subStep = 0;
            gLobbyScreenMode = LOBBY_SCREEN_TOWN;
            break;
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/*
 * Leaving the town (LOBBY_TOWN_LEAVE), on LobbyState.subStep: the
 * equipment preview and the menu draws go, the server's text is shown with
 * a countdown of 300 frames (10 s), the house is closed to visitors, the
 * chat no longer goes to the lobby, the player's quest status is cleared
 * and the hunter card's changes are sent; then the town's level is left
 * (LobbyStep_Level1Leave). Once the countdown has run out and PAD_CIRCLE
 * is pressed (LobbyMenu.promptStep, PROMPT_*), the place goes back to the
 * area and every other player's character, user and member slot is
 * cleared; the screen fades out and the village starts again
 * (LobbyVillage_BeginReturn). A failed leave leaves the lobby.
 */
void LobbyTown_Leave(void)
{
    char buf[16];
    u16 pad;
    s32 i;

    pad = Pad_GetPressedRepeat(0);
    gLobbyMenu->redraw = 1;
    switch (gLobby->subStep) {
    case 0:
        gLobby->subStep++;
        gLobby->seqStep = 0;
        EquipPreview_Hide(1);
        EquipPreview_Free();
        LobbyMenu_SetDraw(0, 0, 0, 0);
        gLobby->countdown = 300;    /* frames: 10 s */
        LobbyDialog_OpenWait(gLobby->errText);
        gLobbyMenu->promptStep = PROMPT_COUNTDOWN;
        gHouseAccess = HOUSE_ACCESS_CLOSED;
        gChatToLobby = 0;
        gPlayers[gScene.localPlayer].questStatus = 0;
        HunterCard_SendChanged();
        gLobbyTextDirty = 0;
        break;
    case 1:
        switch (LobbyStep_Level1Leave()) {
        case LOBBY_STEP_DONE:
            gLobby->subStep++;
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
        break;
    case 2:
        if (gLobbyMenu->promptStep == PROMPT_DONE) {
            Lobby_SetPlace(LOBBY_PLACE_LEVEL0);
            for (i = 0; i < LOBBY_MEMBER_COUNT; i++) {
                if (gScene.localPlayer != i) {
                    TownPc_Remove(&gPlayers[i]);
                    Mem_Fill(&gPlayers[i], 0, sizeof(Player));
                    Mem_Fill(&gNetUsers[i], 0, sizeof(NetUser));
                    Mem_Fill(&gLobby->members[i], 0, sizeof(NetUser));
                }
            }
            gLobby->subStep++;
        }
        break;
    case 3:
        switch (LobbyScreen_FadeOut(LOBBY_ENTER_VILLAGE_RETURN)) {
        case LOBBY_STEP_DONE:
            gLobby->subStep++;
            break;
        }
        break;
    case 4:
        if (FadeA_GetState() != FADE_STATE_RUNNING) {
            LobbyMenu_SetDraw(0, 0, 0, 0);
            LobbyVillage_BeginReturn(1);
        }
        break;
    case LOBBY_SEQ_FAILED:
        gLobby->subStep = 0;
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    switch (gLobbyMenu->promptStep) {
    case PROMPT_COUNTDOWN:
        if (--gLobby->countdown == 0) {
            gLobbyMenu->promptStep++;
        } else {
            Text_SetSize(5);
            sprintf(buf, "%3d", gLobby->countdown / 30); /* in seconds */
            Text_DrawShadow(470, 84, 1, 5, buf);
        }
        break;
    case PROMPT_PRESS:
        Lobby_DrawPressButton();
        if (pad & PAD_CIRCLE) {
            gLobbyMenu->promptStep++;
            LobbyText_Open(LOBBY_TEXT_TOWN_ENTERING);
            LobbySe_Play(LOBBY_SE_CONFIRM_ALT);
        }
        break;
    }
}

/* The town entry's users (6303's reply): the entry's user and quest-user
 * counts into its gLobbyLevel1Entries record; failed, its user count is
 * cleared. */
void LobbyTown_OnUsersReply(NetMsg msg, NetMsg *msgPtr)
{
    s32 id;

    if (gLobby->phase != LOBBY_PHASE_EXIT) {
        id = LobbyLevel_SelId(LOBBY_PLACE_LEVEL1);
        if (msg.kind == NET_MSG_DONE) {
            NetList1_GetUsers(id, &gLobbyLevel1Entries[id - 1].users, &gLobbyLevel1Entries[id - 1].questUsers);
        } else {
            (gLobbyLevel1Entries - 1)[id].users = 0;
        }
    }
}

/* The group search's reply (LOBBY_AWAIT_LEVEL1_SEARCH): the results into
 * gGroupSearchResults; failed, the server's error text. */
void LobbyLevel1_OnLevel1SearchReply(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_SEARCH) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
                LobbyNet_GetLevel1Search((u32 **)&gGroupSearchResults);
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
            }
        }
    }
}

/* Searches the area's groups (6319) with the query. Returns
 * LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel1_Search(NetLevel1SearchQuery *query)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_SEARCH;
        LobbyReq_Level1Search(*query, LobbyLevel1_OnLevel1SearchReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* A group's details' reply (LOBBY_AWAIT_LEVEL1_SEARCH_INFO): into
 * gGroupSearchInfo; failed, the server's error text in the dialog. */
void LobbyLevel1_OnLevel1SearchInfoReply(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL1_SEARCH_INFO) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
                LobbyNet_GetLevel1SearchInfo(&gGroupSearchInfo);
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
            }
        }
    }
}

/* Fetches the details of the search's result i (631A). Returns
 * LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel1_FetchInfo(u8 i)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL1_SEARCH_INFO;
        LobbyReq_Level1SearchInfo(i, LobbyLevel1_OnLevel1SearchInfoReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* Asks for the room the player is in (gLobbyLevel2Sel): its users (640A),
 * its user counts (6403) and its member count (6412); the place becomes the
 * room. */
void LobbyLevel2_Refresh(void)
{
    s32 id;

    gLobby->subStep = 0;
    Lobby_ClearRequests();
    gLobby->roomMembers = 0;
    id = gLobbyLevel2Sel;
    LobbyReq_Level2Users(id, LobbyLevel2_OnLevel2UsersReply);
    LobbyReq_Level2Values(id, LobbyLevel2_OnLevel2ValuesReply);
    LobbyReq_Level2Members(id, LobbyLevel2_OnLevel2MembersReply);
    Lobby_SetPlace(LOBBY_PLACE_LEVEL2);
}

/* Fetches the town's rooms (the level-2 list task) into
 * gLobbyLevel2Entries, cleared first with the ids 1-5 set. Returns
 * LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel2_FetchList(void)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        memset(gLobbyLevel2Entries, 0, sizeof(gLobbyLevel2Entries));
        gLobbyLevel2Entries[0].id = 1;
        gLobbyLevel2Entries[1].id = 2;
        gLobbyLevel2Entries[2].id = 3;
        gLobbyLevel2Entries[3].id = 4;
        gLobbyLevel2Entries[4].id = 5;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL2_LIST;
        NetLevel2ListTask_Start(0, NET_ENTRY_VALUES | NET_ENTRY_STATE | NET_ENTRY_TEXT | NET_ENTRY_LOCKED
                                       | NET_ENTRY_LIMITS | NET_ENTRY_COLOR | NET_ENTRY_POSTED_QUEST,
                                LobbyLevel2_OnListDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    default:
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The rooms' list task's end (LOBBY_AWAIT_LEVEL2_LIST). A progress message
 * (NET_PROGRESS_LIST_REPLY) only reads the replies still expected. Done:
 * the room count (gLobbyLevel2Count), and each room's state, user counts,
 * limits, lock, colour, text and posted quest copied into its
 * gLobbyLevel2Entries record and gPostedQuests. Failed: the server's error
 * text in the dialog. */
void LobbyLevel2_OnListDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;
    s32 replies;
    s32 i;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL2_LIST) {
            if (msg.kind == NET_MSG_PROGRESS) {
                if (msg.flag == NET_PROGRESS_LIST_REPLY) {
                    LobbyNet_GetListReplies((u16 *)&replies);
                    return;
                }
                return;
            }
            if (msg.kind == NET_MSG_DONE) {
                *awaited = LOBBY_AWAIT_NONE;
                gLobby->seqStep++;
                NetList2_GetCount(&gLobbyLevel2Count);
                for (i = 0; i < gLobbyLevel2Count; i++) {
                    gLobbyLevel2Entries[i].id = i + 1;
                    NetList2_GetByte(i + 1, &gLobbyLevel2Entries[i].state);
                    NetList2_GetUsers(i + 1, &gLobbyLevel2Entries[i].users, &gLobbyLevel2Entries[i].questUsers);
                    NetList2_GetLimits(i + 1, &gLobbyLevel2Entries[i].maxUsers, &gLobbyLevel2Entries[i].unkA[0], &gLobbyLevel2Entries[i].unkA[1], &gLobbyLevel2Entries[i].unkA[2], &gLobbyLevel2Entries[i].unkA[3]);
                    NetList2_GetLocked(i + 1, &gLobbyLevel2Entries[i].locked);
                    Net_GetLevel2Color(i + 1, &gLobbyLevel2Entries[i].color);
                    NetList2_GetText(i + 1, gLobbyLevel2Entries[i].text);
                    Net_GetLevel2PostedQuest(i + 1, (u8 *)&gPostedQuests[i]);
                }
            } else {
                lobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
            }
        }
    }
}

/* Opens the first free room of the town for a quest (6407, with value): the
 * room becomes gLobbyLevel2Sel (its id) and gLobbyLevel2Index. With no room
 * free, the busy desk's message and LOBBY_STEP_FAILED. Opened, the quest is
 * taken and paid; failed, the server's error text and the quest not taken.
 * Returns LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel2_EnterFree(s32 value)
{
    s32 i;
    LevelEntry *room;

    switch (gLobby->seqStep) {
    case 0:
        for (i = 0, room = gLobbyLevel2Entries; i < LEVEL2_ENTRY_MAX; i++, room++) {
            if (room->state == LEVEL_ENTRY_FREE) {
                gLobbyLevel2Sel = gLobbyLevel2Entries[i].id;
                gLobbyLevel2Index = i;
                break;
            }
        }
        if (i == LEVEL2_ENTRY_MAX) {
            LobbyText_Open(LOBBY_TEXT_QUEST_DESK_BUSY);
            gLobby->seqStep = 0;
            return LOBBY_STEP_FAILED;
        }
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL2_OPEN;
        LobbyReq_Level2Open(LobbyLevel_SelId(LOBBY_PLACE_LEVEL2), value, LobbyLevel2_OnLevel2OpenReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        gLobby->questPaid = 1;
        gLobby->questTaken = 1;
        return LOBBY_STEP_DONE;
    default:
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        LobbyDialog_OpenWait(gLobby->errText);
        gLobby->questTaken = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The room opening's reply (LOBBY_AWAIT_LEVEL2_OPEN); failed, the server's
 * error text. */
void LobbyLevel2_OnLevel2OpenReply(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL2_OPEN) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* Fetches the room's records (the level-2 records task) and copies them
 * into gLobbyRecs: each record's name, choices and their data, its default
 * choice also the current one. Returns LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel2_FetchRecords(void)
{
    NetRecs2 recs;
    s32 id;
    s32 i;
    s32 j;
    s32 k;

    id = LobbyLevel_SelId(LOBBY_PLACE_LEVEL2);
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL2_RECS;
        NetLevel2RecsTask_Start(id, LobbyLevel2_OnRecsDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        gLobby->unk3940 = 0;
        NetLevel2RecsTask_GetRecs(id, &recs);
        memset(&gLobbyRecs, 0, sizeof(LobbyRecs));
        gLobbyRecs.sendName = recs.sendName;
        gLobbyRecs.sendPass = recs.sendPass;
        gLobbyRecs.n = recs.n;
        for (i = 0; i < gLobbyRecs.n; i++) {
            strcpy(gLobbyRecs.rec[i].name, recs.rec[i].name);
            gLobbyRecs.rec[i].editable = recs.rec[i].editable;
            gLobbyRecs.rec[i].subCount = recs.rec[i].subCount;
            gLobbyRecs.rec[i].defaultChoice = gLobbyRecs.rec[i].cur = recs.rec[i].defaultChoice;
            for (j = 0; j < gLobbyRecs.rec[i].subCount; j++) {
                strcpy(gLobbyRecs.rec[i].subName[j], recs.rec[i].subName[j]);
            }
            for (j = 0; j < 32; j++) {
                gLobbyRecs.rec[i].subN[j] = recs.rec[i].subN[j];
                for (k = 0; k < 32; k++) {
                    gLobbyRecs.rec[i].subData[j][k][0] = recs.rec[i].subData[j][k][0];
                    gLobbyRecs.rec[i].subData[j][k][1] = recs.rec[i].subData[j][k][1];
                    gLobbyRecs.rec[i].subData[j][k][2] = recs.rec[i].subData[j][k][2];
                }
            }
        }
        gLobbyMenu->promptStep = 3;     /* set only here; no reader tests 3 */
        return LOBBY_STEP_DONE;
    default:
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The records task's end (LOBBY_AWAIT_LEVEL2_RECS): progress messages are
 * let pass; failed, the server's error text in the dialog. */
void LobbyLevel2_OnRecsDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL2_RECS) {
            if (msg.kind == NET_MSG_PROGRESS) {
                return;
            }
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* Sends the room's settings (the level-2 settings task): the password, the
 * room's text (gRoomText), each record's chosen value, and the quest
 * (gLobbyQuestId: its id's two bytes, its list flags and stars); then asks
 * for the room again (LobbyLevel2_Refresh). Failed, the lobby is left.
 * Returns LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel2_SendSettings(void)
{
    NetEntrySettings2 settings;
    s32 i;

    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep = 1;
        memset(&settings, 0, sizeof(settings));
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL2_SETTINGS;
        memcpy(settings.pass, gLobbyRecs.pass, sizeof(settings.pass));
        memcpy(settings.text, gRoomText, sizeof(gRoomText));
        for (i = 0; i < gLobbyRecs.n; i++) {
            settings.b[i] = gLobbyRecs.rec[i].cur;
        }
        settings.c[0] = gLobbyQuestId;
        settings.c[1] = (gLobbyQuestId & 0xFF00) >> 8;
        settings.c[2] = Quest_GetListFlags(gLobbyQuestId);
        settings.c[3] = Quest_GetStars(gLobbyQuestId);
        NetLevel2SettingsTask_Start(settings, LobbyLevel2_OnSettingsDone);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        LobbyLevel2_Refresh();
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    default:
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The settings task's end (LOBBY_AWAIT_LEVEL2_SETTINGS): done, the quest is
 * taken; failed, the server's error text in the dialog. */
void LobbyLevel2_OnSettingsDone(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL2_SETTINGS) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
                gLobby->questTaken = 1;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* Sends the quest's acceptance (6509, with value). Returns LOBBY_STEP_BUSY,
 * _DONE or _FAILED. */
s32 LobbyLevel2_SendQuestAccept(u32 value)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_QUEST_ACCEPT;
        LobbyReq_QuestAccept(value, LobbyLevel2_OnQuestAcceptReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    default:
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The acceptance's reply (LOBBY_AWAIT_QUEST_ACCEPT); failed, the server's
 * error text. */
void LobbyLevel2_OnQuestAcceptReply(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_QUEST_ACCEPT) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* Posts the quest (650B): the posting post, with the player's user id (its
 * NET_USER_ID_MATCH significant bytes) and name (8 bytes) filled in.
 * Returns LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel2_SendQuestPost(MsgQuestPost *post)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        memset(post->userId, 0, sizeof(post->userId));
        memset(post->name, 0, sizeof(post->name));
        memcpy(post->userId, gMyUserId, NET_USER_ID_MATCH);
        memcpy(post->name, gMyName, 8);
        LobbyReq_QuestPost(post, sizeof(MsgQuestPost), LobbyLevel2_OnQuestPostReply);
        gLobby->awaitReply = LOBBY_AWAIT_QUEST_POST;
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        return LOBBY_STEP_DONE;
    default:
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The posting's reply (LOBBY_AWAIT_QUEST_POST); failed, the server's error
 * text in the dialog. */
void LobbyLevel2_OnQuestPostReply(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_QUEST_POST) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* Enters the room gLobbyLevel2Index + 1 with the password typed
 * (gLobbyRecs.pass; 6406, with value); entered, asks for the room
 * (LobbyLevel2_Refresh). Returns LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel2_EnterWithPass(s32 value)
{
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL2_ENTER;
        gLobbyLevel2Sel = gLobbyLevel2Index + 1;
        LobbyReq_Level2Enter(LobbyLevel_SelId(LOBBY_PLACE_LEVEL2), gLobbyRecs.pass, value, LobbyLevel2_OnLevel2EnterReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        LobbyLevel2_Refresh();
        return LOBBY_STEP_DONE;
    default:
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        return LOBBY_STEP_FAILED;
    }
    return LOBBY_STEP_BUSY;
}

/* The room entry's reply (LOBBY_AWAIT_LEVEL2_ENTER): done, the quest is
 * taken; failed, the server's error text in the dialog. */
void LobbyLevel2_OnLevel2EnterReply(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL2_ENTER) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
                gLobby->questTaken = 1;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
                LobbyNet_GetErrText((s8 *)gLobby->errText);
                LobbyDialog_OpenWait(gLobby->errText);
            }
        }
    }
}

/* The room's users (640A's reply): the room's LOBBY_ROOM_USER_COUNT
 * players (LobbyState.players: id, name, text), cleared first; failed, the
 * lobby is left. */
void LobbyLevel2_OnLevel2UsersReply(NetMsg msg, NetMsg *msgPtr)
{
    s32 i;

    if (gLobby->phase != LOBBY_PHASE_EXIT) {
        memset(gLobby->players, 0, sizeof(gLobby->players));
        if (msg.kind == NET_MSG_DONE) {
            for (i = 0; i < LOBBY_ROOM_USER_COUNT; i++) {
                NetList2_GetUser(i, gLobby->players[i].id, gLobby->players[i].name, gLobby->players[i].text);
            }
        } else {
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
        }
    }
}

/* The room's user counts (6403's reply, the room gLobbyLevel2Sel): into its
 * gLobbyLevel2Entries record; failed, its user count is cleared. */
void LobbyLevel2_OnLevel2ValuesReply(NetMsg msg, NetMsg *msgPtr)
{
    s32 id;

    if (gLobby->phase != LOBBY_PHASE_EXIT) {
        id = gLobbyLevel2Sel;
        if (msg.kind == NET_MSG_DONE) {
            NetList2_GetUsers(id, &gLobbyLevel2Entries[id - 1].users, &gLobbyLevel2Entries[id - 1].questUsers);
        } else {
            (gLobbyLevel2Entries - 1)[id].users = 0;
        }
    }
}

/* The room's member count (6412's reply) into LobbyState.roomMembers; 0
 * when it failed. The reply's second value is not kept: it is written into
 * the message's own copy on the stack, as the game does. */
void LobbyLevel2_OnLevel2MembersReply(NetMsg msg, NetMsg *msgPtr)
{
    if (msg.kind == NET_MSG_DONE) {
        NetList2_GetMembers(gLobbyLevel2Sel, &gLobby->roomMembers, (u16 *)&msg + 3);
    } else {
        gLobby->roomMembers = 0;
    }
}
