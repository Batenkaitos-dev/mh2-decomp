/*
 * The lobby's village phase (LOBBY_PHASE_VILLAGE): an area of the server
 * has been entered, and the player walks the village alone until a town of
 * the area is entered or the area is left.
 *
 * - The phase's starts set the lobby state's steps back and choose how the
 *   screen comes in (LobbyState.enterSeq, LOBBY_ENTER_*): LobbyVillage_Begin
 *   from the area list, LobbyVillage_BeginStay with the player kept where
 *   it is, LobbyVillage_BeginReturn back from a town, LobbyVillage_Restart
 *   after the elder dragon raid's movie. LobbyVillage_BeginLeave starts the
 *   area's leave.
 * - LobbyVillage_Run is the phase's frame, by LobbyState.mode
 *   (enum LobbyVillageMode): the area's towns and quests are fetched
 *   (LobbyVillage_FetchTowns), the screen comes in, then the village runs
 *   (LobbyScene_Run). A group joined goes straight on into its town
 *   (LobbyVillage_BeginJoin, LobbyLevel1_Enter, then LobbyTown_Begin).
 * - Leaving the area: LobbyLevel0_Leave sends the leave request (6306) and
 *   goes back to the area list (AreaList_Begin); LobbyLevel0_LeaveCountdown
 *   does the same behind the server's text and a countdown.
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "scene.h"
#include "pad.h"
#include "stream.h"
#include "text.h"
#include "lobby_village.h"
#include "area_list.h"
#include "elder_forecast.h"
#include "game_util.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_leave_menus.h"
#include "lobby_queries.h"
#include "lobby_screen.h"
#include "lobby_steps.h"
#include "lobby_town.h"
#include "lobby_util.h"
#include "screen_fade.h"
#include "libc.h"

extern s8 gLobbyLevel1Sel;
extern s8 gLobbyLevel2Sel;
extern LevelEntry gLobbyLevel1Entries[];
extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyScreenMode;
extern u16 gAreaSearchTimer;

/* Starts the village from the area list: the lobby state's steps back to
 * the start, the screen entered as LOBBY_ENTER_VILLAGE, the selections and
 * the area's town list cleared, and the area-entry message. */
void LobbyVillage_Begin(void)
{
    gLobby->phase = LOBBY_PHASE_VILLAGE;
    gLobby->phaseStep = 0;
    gLobby->mode = LOBBY_VILLAGE_START;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    gLobby->enterSeq = LOBBY_ENTER_VILLAGE;
    Lobby_ClearRequests();
    gLobbyLevel1Sel = 0;
    gLobbyLevel2Sel = 0;
    memset(gLobbyLevel1Entries, 0, LEVEL1_ENTRY_MAX * sizeof(LevelEntry));
    Chat_ClearTargets();
    LobbyMenu_Init();
    LobbyText_Open(LOBBY_TEXT_AREA_ENTERING);
}

/* Starts the village again after the raid's movie (LobbyVillage_Run): as
 * LobbyVillage_Begin, with the selections and the lists kept. */
void LobbyVillage_Restart(void)
{
    gLobby->phase = LOBBY_PHASE_VILLAGE;
    gLobby->phaseStep = 0;
    gLobby->mode = LOBBY_VILLAGE_START;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    gLobby->enterSeq = LOBBY_ENTER_VILLAGE;
    Lobby_ClearRequests();
    LobbyMenu_Init();
}

/* As LobbyVillage_Begin, with the screen entered as
 * LOBBY_ENTER_VILLAGE_STAY: the player is placed again in its area. */
void LobbyVillage_BeginStay(void)
{
    gLobby->phase = LOBBY_PHASE_VILLAGE;
    gLobby->phaseStep = 0;
    gLobby->mode = LOBBY_VILLAGE_START;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    gLobby->enterSeq = LOBBY_ENTER_VILLAGE_STAY;
    Lobby_ClearRequests();
    gLobbyLevel1Sel = 0;
    gLobbyLevel2Sel = 0;
    memset(gLobbyLevel1Entries, 0, LEVEL1_ENTRY_MAX * sizeof(LevelEntry));
    Chat_ClearTargets();
    LobbyMenu_Init();
    LobbyText_Open(LOBBY_TEXT_AREA_ENTERING);
}

/* Back to the village from a town (LOBBY_ENTER_VILLAGE_RETURN): as
 * LobbyVillage_Begin, with the menu drawing the black line, and the "moving"
 * message only when showText is nonzero. */
void LobbyVillage_BeginReturn(s32 showText)
{
    gLobby->phase = LOBBY_PHASE_VILLAGE;
    gLobby->phaseStep = 0;
    gLobby->mode = LOBBY_VILLAGE_START;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    gLobby->enterSeq = LOBBY_ENTER_VILLAGE_RETURN;
    Lobby_ClearRequests();
    LobbyMenu_Init();
    gLobbyLevel1Sel = 0;
    gLobbyLevel2Sel = 0;
    memset(gLobbyLevel1Entries, 0, LEVEL1_ENTRY_MAX * sizeof(LevelEntry));
    Chat_ClearTargets();
    LobbyMenu_SetDraw(Lobby_DrawBlackLine, 0, 0, 0);
    if (showText != 0) {
        LobbyText_Open(LOBBY_TEXT_TOWN_ENTERING);
    }
}

/* Starts leaving the area: the village's mode goes to
 * LOBBY_VILLAGE_LEAVE_COUNTDOWN, marked as the player's own leave. */
void LobbyVillage_BeginLeave(void)
{
    gLobby->phase = LOBBY_PHASE_VILLAGE;
    gLobby->phaseStep = 0;
    gLobby->mode = LOBBY_VILLAGE_LEAVE_COUNTDOWN;
    gLobby->step = 0;
    gLobby->subStep = 0;
    gLobby->subStep2 = 0;
    gLobby->spareStep0 = 0;
    gLobby->spareStep1 = 0;
    Lobby_ClearRequests();
    gLobby->level0Kicked = LEAVE_BEGUN;
}

/* 1 while the area is being left (the village's leave countdown), else 0. */
s32 LobbyVillage_IsLeaving(void)
{
    if ((gLobby->phase == LOBBY_PHASE_VILLAGE) && (gLobby->mode == LOBBY_VILLAGE_LEAVE_COUNTDOWN)) {
        return 1;
    }
    return 0;
}

/*
 * The village phase's frame, by LobbyState.mode (enum LobbyVillageMode):
 * - START: the menus fade in, then FETCH_TOWNS;
 * - FETCH_TOWNS: the area's towns and quests (LobbyVillage_FetchTowns);
 *   then, for a group joined, its town (JOIN_TOWN), else the screen with no
 *   menu draws (ENTER);
 * - ENTER: the raid's movie when one waits (MOVIE), else the screen comes
 *   in (LobbyScreen_EnterSeq) and the village runs (RUN);
 * - RUN: the village (LobbyScene_Run) while no leave is asked and the line
 *   holds, or the raid's movie;
 * - LEAVE_COUNTDOWN: the area's leave (LobbyLevel0_LeaveCountdown);
 * - JOIN_TOWN: the group's town entered (LobbyLevel1_Enter), then the town
 *   phase (LobbyTown_Begin); failed, the join is dropped and the screen
 *   comes in;
 * - MOVIE: the raid's movie, then the village starts again.
 * A failed fetch, or a mode it does not know, leaves the lobby.
 */
void LobbyVillage_Run(void)
{
    switch (gLobby->mode) {
    case LOBBY_VILLAGE_START:
        gLobby->mode++;
        gLobby->step = 0;
        gLobby->seqStep = 0;
        if (FadeA_GetState() != FADE_STATE_IDLE) {
            FadeA_Start(FADE_IN_BLACK_FAST);
        }
        gLobbyMenu->redraw = 1;
    case LOBBY_VILLAGE_FETCH_TOWNS:
        gLobbyMenu->redraw = 1;
        switch (LobbyVillage_FetchTowns()) {
        case LOBBY_STEP_DONE:
            gLobby->mode++;
            if (gLobby->groupJoin != 0) {
                gLobby->mode = LOBBY_VILLAGE_JOIN_TOWN;
                LobbyVillage_BeginJoin();
            } else {
                LobbyMenu_SetDraw(0, 0, 0, 0);
                Town_Reset();
            }
            break;
        case LOBBY_STEP_FAILED:
            Lobby_Disconnect(LOBBY_QUIT_FAILED);
            break;
        }
        break;
    case LOBBY_VILLAGE_ENTER:
        if (LobbyTown_WantsMovie() == 1) {
            gLobby->mode = LOBBY_VILLAGE_MOVIE;
            LobbyTown_StartMovie();
        } else {
            gLobbyMenu->hideLayers = 1;
            switch (LobbyScreen_EnterSeq(gLobby->enterSeq)) {
            case LOBBY_STEP_DONE:
                if (FadeB_GetState() != FADE_STATE_IDLE) {
                    FadeB_Start(FADE_IN_BLACK_FAST);
                }
                gLobbyScreenMode = LOBBY_SCREEN_TOWN;
                gLobby->mode++;
                break;
            }
        }
        break;
    case LOBBY_VILLAGE_RUN:
        if (Lobby_HandleLeaveRequest() == 0) {
            if (LobbyTown_WantsMovie() == 1) {
                gLobby->mode = LOBBY_VILLAGE_MOVIE;
                LobbyTown_StartMovie();
            } else if (gLobby->linkState == LINK_STATE_UP) {
                LobbyScene_Run();
            }
        }
        break;
    case LOBBY_VILLAGE_LEAVE_COUNTDOWN:
        LobbyLevel0_LeaveCountdown();
        break;
    case LOBBY_VILLAGE_JOIN_TOWN:
        switch (LobbyLevel1_Enter()) {
        case LOBBY_STEP_DONE:
            LobbyTown_Begin();
            Stream_Stop(0);
            Stream_Stop(1);
            break;
        case LOBBY_STEP_FAILED:
            gLobby->mode = LOBBY_VILLAGE_ENTER;
            memset(gLobby, 0, LOBBY_GROUP_JOIN_SIZE);
            break;
        }
        break;
    case LOBBY_VILLAGE_MOVIE:
        if (LobbyTown_MovieDone() == 1) {
            gLobby->movieState = RAID_MOVIE_NONE;
            LobbyVillage_Restart();
            gLobby->mode = LOBBY_VILLAGE_ENTER;
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
}

/* Starts entering the town of the group joined: the menu work cleared, the
 * "sending" message, and the town chosen is the group's land. */
void LobbyVillage_BeginJoin(void)
{
    memset(gMenuWork, 0, MENU_WORK_SIZE);
    LobbyText_Open(LOBBY_TEXT_SENDING);
    gLobbyMenu->redraw = 1;
    gLobbyLevel1Sel = (s8)gLobby->land;
}

/* Enters the town of the group joined (the level-1 entry gLobby->land), one
 * step a frame; the step is the menu work's first byte. The player's hunter
 * rank is checked against the group's limit first (refused: the message,
 * and LOBBY_STEP_FAILED); then the entry (LobbyStep_EnterLevel1) and the
 * quest files (LobbyStep_DownloadQuests), after which the menu draws are
 * cleared, every player's model marked unloaded and the file queue
 * cancelled. Returns LOBBY_STEP_BUSY, _DONE or _FAILED. */
s32 LobbyLevel1_Enter(void)
{
    s8 *step = (s8 *)gMenuWork;

    switch (*step) {
    case 0:
        if (LobbyLevel1_CanEnter(gLobby->land) == 1) {
            (*step)++;
            break;
        }
        *step = 0;
        gLobbyLevel1Sel = 0;
        LobbyText_Open(LOBBY_TEXT_GROUP_RANK_REFUSED);
        return LOBBY_STEP_FAILED;
    case 1:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_EnterLevel1()) {
        case LOBBY_STEP_DONE:
            (*step)++;
            break;
        case LOBBY_STEP_FAILED:
            gLobbyLevel1Sel = 0;
            return LOBBY_STEP_FAILED;
        }
        break;
    case 2:
        gLobbyMenu->redraw = 1;
        switch (LobbyStep_DownloadQuests()) {
        case LOBBY_STEP_DONE:
        case LOBBY_STEP_FAILED:
            LobbyMenu_SetDraw(0, 0, 0, 0);
            memset(gLobby->pcLoaded, 0, sizeof(gLobby->pcLoaded));
            FileQueue_Cancel();
            return LOBBY_STEP_DONE;
        }
        break;
    }
    return LOBBY_STEP_BUSY;
}

/* Fetches the area's town list (LobbyStep_Level1List, every entry) and its
 * quest files (LobbyStep_DownloadQuests; their failure is let pass), then
 * marks the lobby connected, sets the menu up again and restarts the area
 * search's timer. Returns LOBBY_STEP_BUSY, _DONE or _FAILED (the list); a
 * step it does not know leaves the lobby. */
s32 LobbyVillage_FetchTowns(void)
{
    gLobbyMenu->redraw = 1;
    switch (gLobby->step) {
    case 0:
        switch (LobbyStep_Level1List(0)) {
        case LOBBY_STEP_DONE:
            gLobby->step++;
            break;
        case LOBBY_STEP_FAILED:
            return LOBBY_STEP_FAILED;
        }
        break;
    case 1:
        switch (LobbyStep_DownloadQuests()) {
        case LOBBY_STEP_FAILED:
        case LOBBY_STEP_DONE:
            gLobby->connected = 1;
            LobbyMenu_Init();
            gLobby->step = 0;
            gAreaSearchTimer = 0;
            return LOBBY_STEP_DONE;
        }
        break;
    default:
        Lobby_Disconnect(LOBBY_QUIT_FAILED);
        break;
    }
    return LOBBY_STEP_BUSY;
}

/* The area leave's reply for LobbyLevel0_Leave: seqStep goes on, or to
 * LOBBY_SEQ_FAILED. */
void LobbyLevel0_OnLeaveReply(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL0_LEAVE) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->seqStep)++;
            } else {
                gLobby->seqStep = LOBBY_SEQ_FAILED;
            }
        }
    }
}

/* Leaves the area (the request 6306, on seqStep): then the subsystems are
 * reset and the area list starts again; failed, the village starts again.
 * Returns LOBBY_STEP_BUSY until either, then LOBBY_STEP_DONE. */
s32 LobbyLevel0_Leave(void)
{
    gLobbyMenu->redraw = 1;
    switch (gLobby->seqStep) {
    case 0:
        gLobby->seqStep++;
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL0_LEAVE;
        LobbyReq_Level0Leave(LobbyLevel0_OnLeaveReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        gLobby->seqStep = 0;
        Game_ResetSubsystems();
        gLobbyMenu->hideLayers = 1;
        Lobby_LoadTextures();
        AreaList_Begin();
        return LOBBY_STEP_DONE;
    case LOBBY_SEQ_FAILED:
        gLobby->seqStep = 0;
        LobbyVillage_Begin();
        return LOBBY_STEP_DONE;
    }
    return LOBBY_STEP_BUSY;
}

/* The village's leave countdown (LOBBY_VILLAGE_LEAVE_COUNTDOWN), on
 * LobbyState.step: the server's text is shown with a countdown of 300 frames
 * (10 s) while the leave request (6306) goes out. Once the countdown has run
 * out and PAD_CIRCLE is pressed (LobbyMenu.promptStep, PROMPT_*), the
 * subsystems are reset and the area list starts again; when the request
 * failed, the village starts again instead. */
void LobbyLevel0_LeaveCountdown(void)
{
    char buf[16];
    u16 pad;

    pad = Pad_GetPressedRepeat(0);
    gLobbyMenu->redraw = 1;
    switch (gLobby->step) {
    case 0:
        gLobby->step++;
        LobbyMenu_Init();
        gLobby->countdown = 300;    /* frames: 10 s */
        LobbyDialog_OpenWait(gLobby->errText);
        LobbyMenu_StartTimer();
        gLobby->awaitReply = LOBBY_AWAIT_LEVEL0_LEAVE;
        LobbyReq_Level0Leave(LobbyLevel0_OnCountdownReply);
        break;
    case 1:
        LobbyMenu_TickTimer();
        break;
    case 2:
        if (gLobbyMenu->promptStep == PROMPT_DONE) {
            Game_ResetSubsystems();
            gLobbyMenu->hideLayers = 1;
            Lobby_LoadTextures();
            AreaList_Begin();
            return;
        }
        break;
    case LOBBY_SEQ_FAILED:
        if (gLobbyMenu->promptStep == PROMPT_DONE) {
            LobbyVillage_Begin();
            return;
        }
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

/* The area leave's reply for LobbyLevel0_LeaveCountdown: its step goes on,
 * or to LOBBY_SEQ_FAILED. */
void LobbyLevel0_OnCountdownReply(NetMsg msg, NetMsg *msgPtr)
{
    u8 *awaited;
    LobbyState *lobby = gLobby;

    if (lobby->phase != LOBBY_PHASE_EXIT) {
        awaited = &lobby->awaitReply;
        if (*awaited == LOBBY_AWAIT_LEVEL0_LEAVE) {
            *awaited = LOBBY_AWAIT_NONE;
            if (msg.kind == NET_MSG_DONE) {
                (gLobby->step)++;
            } else {
                gLobby->step = LOBBY_SEQ_FAILED;
            }
        }
    }
}
