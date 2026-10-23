/*
 * The lobby overlay's scene task, NetScene_Run: sub_main runs it in
 * TASK_SLOT_SCENE (TASK_REQUEST_NET_SCENE) whenever the lobby overlay is the
 * scene, online in the lobby server's town and offline in the village.
 *
 * Its modes (TaskSlot.mode, enum NetSceneMode) load the overlays, reset the
 * game's subsystems, run the scene and hand over to sub_main's quest task
 * when a quest starts. Online, the running mode is a state machine of its
 * own (NetScene_StateOnline, TaskSlot.step, enum NetSceneStep): the login's
 * connection, the lobby's frames (Lobby_Tick), the quest server's connection
 * (LobbyConn_Run), the leave, the KDDI portal and the exit. Offline it runs
 * the lobby screen (LobbyScreen_Run) and the offline pause.
 *
 * The other lobby files move the task between those steps through
 * NetScene_ToPortal, NetScene_ToNetFileCheck and NetScene_ToDisconnect. The
 * file also keeps the play time's count (Lobby_CountPlayTime) and the
 * lobby's copies of the offline pause (Lobby_SetPause, Lobby_GetPauseState).
 */
#include "common.h"
#include "net_save.h"
#include "syssettings.h"
#include "net_mode.h"
#include "em.h"
#include "lobby_state.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "syssave.h"
#include "browser.h"
#include "task.h"
#include "movie.h"
#include "overlay.h"
#include "hud.h"
#include "lobby_net.h"
#include "stream.h"
#include "text.h"
#include "net_scene.h"
#include "draw_helpers.h"
#include "frame.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_frame.h"
#include "lobby_screen.h"
#include "lobby_world.h"
#include "login_connect_leave.h"
#include "quest.h"
#include "screen_fade.h"
#include "libc.h"

extern u8 gMenuInputLock;
extern u32 gLobbyTime;
extern s8 gNetDropped;
extern s8 gLoginExitStep;
extern s16 gNetStepTimeout;

static void NetScene_SetMode(TaskSlot *task, s8 mode);
static void NetScene_SetStep(TaskSlot *task, s8 step);
static void NetScene_TickBegin(void);
static void NetScene_TickEnd(void);
extern s8 gLoginState;
extern s8 gNetConnStep;
extern s8 gLbsConnectStep;
extern s8 gNetScreenStep;
extern u8 gLoginConnectStep;
extern s8 gNetEntryMode;
void func_8FC6A0(void);   /* QNetIf_Close (gm_sub) */
u8 func_8FC6B0(void);     /* QNetIf_GetCount (gm_sub) */
void func_8FC600(void);   /* QNetIf_UpdateReceive (gm_sub) */
s32 func_8FC5F0(void);    /* QNetIf_IsRunning (gm_sub) */

/* NetScene_Run's modes (TaskSlot.mode). */
enum NetSceneMode {
    NET_SCENE_START = 0,        /* the overlays and the scene's reset; offline, the movie first */
    NET_SCENE_RESET = 1,        /* the game's subsystems reset (and the lobby's state, offline) */
    NET_SCENE_RUN = 2,          /* NetScene_StateOnline or NetScene_StateOffline */
    NET_SCENE_TO_QUEST = 3,     /* the players counted, then sub_main's quest task */
    NET_SCENE_MOVIE = 4         /* OFFLINE_ENTRY_MOVIE, then NET_SCENE_START again */
};

/* NetScene_StateOnline's steps (TaskSlot.step in NET_SCENE_RUN, online). */
enum NetSceneStep {
    NET_STEP_START = 0,         /* the lobby's state reset */
    NET_STEP_CONNECT = 1,       /* the login's connection (NetScene_StepConnect) */
    NET_STEP_LOBBY = 2,         /* the lobby's frames (Lobby_Tick) */
    NET_STEP_QUEST_CONNECT = 3, /* the quest server's connection (LobbyConn_Run) */
    NET_STEP_LEAVE = 4,         /* leaving the network (LobbyLogin_Leave) */
    NET_STEP_END = 5,           /* the mode ends: NetScene_Run quits the scene */
    NET_STEP_PORTAL = 6,        /* the KDDI portal (LobbyKddi_PortalScene) */
    NET_STEP_EXIT = 7,          /* LobbyLogin_SaveAndExit, then gLoginExitStep */
    NET_STEP_CONN_FAILED = 8    /* the quest server's connection failed: a message, then NET_STEP_CONNECT */
};

#define OFFLINE_ENTRY_MOVIE 6       /* Movie_Play's id: played before an offline entry while event flag 0 is clear */
#define FRAMES_PER_SECOND 30        /* the play time's count */

/* the network mode's message after a failed quest connection (NET_STEP_CONN_FAILED) */
#define CONN_FAILED_MSG 14
#define CONN_FAILED_DELAY 20        /* frames before the message opens */
#define CONN_FAILED_FRAMES 600      /* frames: the message's time on screen */
#define CONN_FAILED_SKIP_BELOW 481  /* circle closes it once the timer is below this (120 frames in) */
#define DROP_WAIT_FRAMES 100        /* NetScene_DropConnection's wait after the socket's close */

void NetScene_DrawFrame(void)
{
    Frame_Draw();
}

static void NetScene_SetMode(TaskSlot *task, s8 mode)
{
    task->mode = mode;
    task->step = 0;
}

static void NetScene_SetStep(TaskSlot *task, s8 step)
{
    task->step = step;
}

void NetScene_LoadOverlays(void)
{
    Scene_LoadOverlay(OVERLAY_LOBBY);
    Scene_LoadOverlay(OVERLAY_LBGUILD);
}

void NetScene_LoadLbguild(void)
{
    Scene_LoadOverlay(OVERLAY_LBGUILD);
}

/* Counts the play time: online, or offline while no pause is shown, every
 * 30th frame adds a second to the save's play time, up to PLAYTIME_MAX
 * (9999:59:59). The lobby screen calls it each frame. */
void Lobby_CountPlayTime(void)
{
    (gScene.frame)++;
    if (Game_IsOnline() != 0 || Lobby_GetPauseState() < PAUSE_CAPTURED) {
        if (gScene.frame % FRAMES_PER_SECOND == 0) {
            gSave.playTime++;
            if (gSave.playTime > PLAYTIME_MAX) {
                gSave.playTime = PLAYTIME_MAX;
            }
        }
    }
}

/*
 * The lobby overlay's scene task (TASK_SLOT_SCENE), by mode:
 * - NET_SCENE_START: offline, while event flag 0 is clear, the movie first
 *   (NET_SCENE_MOVIE). Then the town's and the quest sync's reset; coming
 *   back from a quest's teardown (SCENE_EXIT, step 0), the quest server's
 *   connection is dropped first (NetScene_DropConnection). The lbguild
 *   overlay loads and the screen fades in. After a quest's teardown the
 *   scene goes straight to NET_SCENE_RUN; else the scene's state is reset
 *   and the mode goes on to NET_SCENE_RESET in the same frame.
 * - NET_SCENE_RESET: the textures released and the subsystems reset (the
 *   lobby's state too, offline), then NET_SCENE_RUN.
 * - NET_SCENE_RUN: NetScene_StateOnline or NetScene_StateOffline; -2 from
 *   them quits the scene (NetScene_Quit).
 * - NET_SCENE_TO_QUEST: online, the players are counted from the quest
 *   server (the first frame) and the task waits until the quest network
 *   runs or the local player has no slot; offline, one player. Then
 *   sub_main's quest task takes the slot.
 * - NET_SCENE_MOVIE: plays OFFLINE_ENTRY_MOVIE (Start skips it), fades out,
 *   and goes back to NET_SCENE_START's set-up past the movie's test (the
 *   goto is the game's own jump into the case).
 */
void NetScene_Run(TaskSlot *task)
{
    s32 i;
    s32 result;

    Scene_EmptyFrameHook();
    switch (task->mode) {
    case NET_SCENE_START:
        gSysSettings.petDayPending = 1;
        if (Game_IsOnline() == 0 && Save_TestEventFlag(0) == 0) {
            NetScene_SetMode(task, NET_SCENE_MOVIE);
            break;
        }
    restart:
        Town_Reset();
        QuestSync_Clear();
        if (gScene.scene == SCENE_EXIT && gScene.step == 0 && NetScene_DropConnection(task) == 0) {
            break;
        }
        Dmcam_PreloadAlt();
        NetScene_LoadLbguild();
        FadeA_Start(FADE_IN_BLACK);
        if (gScene.scene == SCENE_EXIT) {
            Gfx_ClearModelBuffers();
            Scene_ResetAll();
            gScene.inLobby = 1;
            NetScene_SetMode(task, NET_SCENE_RUN);
            break;
        }
        Scene_ResetAll();
        gSysSettings.offline = 0;
        gScene.inLobby = 1;
        gScene.unk03D = 1;
        gScene.host = 0;
        NetScene_SetMode(task, NET_SCENE_RESET);
    case NET_SCENE_RESET:
        if (Game_IsOnline() == 1) {
            NetMode_ReleaseTextures(0);
            Game_ResetSubsystems();
            NetScene_SetMode(task, NET_SCENE_RUN);
            break;
        }
        Lobby_ResetState();
        NetMode_ReleaseTextures(0);
        Game_ResetSubsystems();
        NetScene_SetMode(task, NET_SCENE_RUN);
        break;
    case NET_SCENE_RUN:
        if (Game_IsOnline() == 1) {
            result = NetScene_StateOnline(task);
        } else {
            result = NetScene_StateOffline(task);
        }
        if (result == -1) {
            /* empty: the game's code tests -1 and does nothing */
        } else if (result == -2) {
            NetScene_Quit(task);
        }
        break;
    case NET_SCENE_TO_QUEST:
        if (Game_IsOnline() == 1) {
            Loading_Request();
        }
        gSoundLoading = 0;
        if (Game_IsOnline() == 1) {
            if (gScene.playerCount == 0) {
                gScene.playerCount = func_8FC6B0();
                gScene.host = 0;
                for (i = 0; i < SCENE_PLAYERS; i++) {
                    if (i < gScene.playerCount) {
                        gScene.inQuest[i] = IN_QUEST_YES;
                    } else {
                        gScene.inQuest[i] = 0;
                    }
                }
            }
            func_8FC600();
            FadeA_Reset();
            if (func_8FC5F0() == 0 && gScene.inQuest[gScene.localPlayer] != IN_QUEST_NO_PLAYER) {
                break;
            }
        } else {
            gScene.playerCount = 1;
            for (i = 0; i < SCENE_PLAYERS; i++) {
                if (i < gScene.playerCount) {
                    gScene.inQuest[i] = IN_QUEST_YES;
                } else {
                    gScene.inQuest[i] = 0;
                }
            }
        }
        gSysSettings.offline = 1;
        TaskSlot_Prepare(Scene_QuestTask, TASK_SLOT_SCENE);
        break;
    case NET_SCENE_MOVIE:
        switch (task->step) {
        case 0:
            FadeA_Start(FADE_IN_BLACK);
            Movie_Play(OFFLINE_ENTRY_MOVIE);
            task->step++;
        case 1:
            Movie_Update();
            result = Movie_GetStatus();
            if (result == MOVIE_STATUS_FINISHED || (result == MOVIE_STATUS_PLAYING && (gPadState[0].pressed & PAD_START))) {
                FadeA_Start(FADE_OUT_BLACK);
                task->step++;
                Movie_RequestStop();
            }
            break;
        case 2:
            Movie_Update();
            if ((u8)FadeA_GetState() != FADE_STATE_RUNNING) {
                Movie_Stop();
                NetScene_SetMode(task, NET_SCENE_START);
                goto restart;
            }
            break;
        }
        break;
    }
}

/* Ends the lobby's scene: the subsystems reset, the scene's and the network
 * mode's state cleared, the task's slot ended and the select overlay loaded,
 * then select's boot task starts (going on to gm_sub, BOOT_MODE_GMSUB,
 * unless a gm_sub boot mode is already set). */
void NetScene_Quit(TaskSlot *task)
{
    u8 bootMode;

    Game_ResetSubsystems();
    Gfx_ClearModelBuffers();
    memset(&gScene, 0, sizeof(gScene));
    memset(&gNetMode, 0, sizeof(gNetMode));
    gOnlineFlag = 0;
    gSysSettings.unk40 = 0;
    TaskSlot_End(task);
    gGmSubLoaded = 0;
    Scene_LoadOverlay(OVERLAY_SELECT);
    bootMode = gBootMode;
    if ((bootMode != BOOT_MODE_GMSUB) && (bootMode != BOOT_MODE_GMSUB_KEEP)) {
        Lobby_SetBootMode(BOOT_MODE_GMSUB);
    }
    Task_StartBoot();
}

/* Sets where select's boot task goes next (enum BootMode). */
void Lobby_SetBootMode(s32 mode)
{
    gBootMode = mode;
}

/* Empty: NetScene_StateOnline calls them around its start and around the
 * lobby's frame. */
static void NetScene_TickBegin(void)
{

}

static void NetScene_TickEnd(void)
{

}

/* A quest begins: the task goes on to NET_SCENE_TO_QUEST, the network's
 * state machine is idle, and the lobby is left. */
void NetScene_Finish(TaskSlot *task)
{
    NetScene_SetMode(task, NET_SCENE_TO_QUEST);
    gNetMsgState.handler = 0;
    gNetMsgState.unk0 = 4;
    gNetMsgState.step = 0;
    gNetMsgState.timer = 0;
    gScene.inLobby = 0;
    gSysSettings.unk40 = 0;
    gScene.playerCount = 0;
}

/* NET_STEP_CONNECT: runs the login's connection (LobbyLogin_RunConnect).
 * Connected (1), the lobby starts (NET_STEP_LOBBY; returns -1); failed
 * (-1), the exit and then the end (NET_STEP_EXIT, NET_STEP_END); -2 leaves
 * (NET_STEP_LEAVE); 2 draws the login's menu. Else returns 0. */
s32 NetScene_StepConnect(TaskSlot *task)
{
    switch (LobbyLogin_RunConnect()) {
    case 1:
        NetScene_SetStep(task, NET_STEP_LOBBY);
        gScene.inLobby = 1;
        LobbyWorld_Reset();
        Lobby_ResetState();
        return -1;
    case -1:
        NetScene_SetStep(task, NET_STEP_EXIT);
        gLoginExitStep = NET_STEP_END;
        break;
    case -2:
        NetScene_SetStep(task, NET_STEP_LEAVE);
        break;
    case 2:
        LobbyLogin_DrawMenu();
        break;
    }
    NetScene_DrawFrame();
    return 0;
}

/*
 * NET_SCENE_RUN online, by step (enum NetSceneStep). gLoginState and the
 * other connection counters are the stages' own states; each move to a
 * stage clears them (0: the stage's start).
 * - NET_STEP_LOBBY runs the lobby until Lobby_Tick ends it, then connects
 *   to the quest server; the frame is drawn except in the room phase.
 * - NET_STEP_QUEST_CONNECT: connected, the quest begins (NetScene_Finish);
 *   failed, the screen goes black and NET_STEP_CONN_FAILED.
 * - NET_STEP_LEAVE: LobbyLogin_Leave's 1 exits to the end, 2 goes to the
 *   portal's quit page, 3 exits back to the start.
 * - NET_STEP_PORTAL: from the portal's top page with -1, the leave from
 *   its disconnect (gLoginState LOGIN_LEAVE_DISCONNECT); else the
 *   connection again.
 * - NET_STEP_CONN_FAILED (gLoginConnectStep): the network mode's frame
 *   (opened anew if its textures are gone), the message CONN_FAILED_MSG
 *   for CONN_FAILED_FRAMES, then the connection again as after a quest.
 * Returns -2 at NET_STEP_END, -1 from the start and from a new lobby, else 0.
 */
s32 NetScene_StateOnline(TaskSlot *task)
{
    s32 result;

    switch (task->step) {
    case NET_STEP_START:
        NetScene_TickBegin();
        NetScene_SetStep(task, NET_STEP_CONNECT);
        Lobby_ResetState();
        gSysSettings.unk40 = 0;
        NetScene_TickEnd();
        return -1;
    case NET_STEP_CONNECT:
        if (NetScene_StepConnect(task) < 0) {
            return -1;
        }
        break;
    case NET_STEP_LOBBY:
        NetScene_TickBegin();
        if (Lobby_Tick() == 0) {
            NetScene_SetStep(task, NET_STEP_QUEST_CONNECT);
            gNetEntryMode = LOGIN_ENTRY_QUEST_RETURN;
            FadeA_Reset();
        }
        NetScene_TickEnd();
        if (gLobby->phase != LOBBY_PHASE_ROOM) {
            NetScene_DrawFrame();
        }
        break;
    case NET_STEP_QUEST_CONNECT:
        switch (LobbyConn_Run()) {
        case 1:
            NetScene_Finish(task);
            break;
        case -1:
            Loading_Reset();
            NetScene_SetStep(task, NET_STEP_CONN_FAILED);
            gLoginState = LOGIN_START;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            FadeB_Start(FADE_BLACK);
            break;
        }
        NetScene_DrawFrame();
        break;
    case NET_STEP_LEAVE:
        switch (LobbyLogin_Leave()) {
        case 1:
            NetScene_SetStep(task, NET_STEP_EXIT);
            gLoginExitStep = NET_STEP_END;
            FadeB_Reset();
            break;
        case 2:
            gPortalPage = PORTAL_PAGE_QUIT;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            NetScene_ToPortal();
            FadeB_Reset();
            break;
        case 3:
            NetScene_SetStep(task, NET_STEP_EXIT);
            gLoginExitStep = NET_STEP_START;
            FadeB_Reset();
            break;
        default:
            NetScene_DrawFrame();
            break;
        }
        break;
    case NET_STEP_END:
        return -2;
    case NET_STEP_PORTAL:
        result = LobbyKddi_PortalScene();
        if (result != 0) {
            if (result == -1 && gPortalPage == PORTAL_PAGE_TOP) {
                NetScene_SetStep(task, NET_STEP_LEAVE);
                gNetConnStep = 0;
                gLoginState = LOGIN_LEAVE_DISCONNECT;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            } else {
                NetScene_SetStep(task, NET_STEP_CONNECT);
                gLoginState = LOGIN_START;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            }
        }
        break;
    case NET_STEP_EXIT:
        switch (LobbyLogin_SaveAndExit()) {
        case 1:
            NetScene_SetStep(task, gLoginExitStep);
            gLoginExitStep = NET_STEP_START;
            break;
        }
        NetScene_DrawFrame();
        break;
    case NET_STEP_CONN_FAILED:
        switch (gLoginConnectStep) {
        case 0:
            Loading_Reset();
            if (gOvlTex != 0) {
                if (gNetMode.boxes != 0) {
                    gNetDropped = 0;
                    gLoginConnectStep = 2;
                    gNetStepTimeout = CONN_FAILED_DELAY;
                    NetMode_ResetToFrame();
                }
            } else {
                gNetDropped = 0;
                gLoginConnectStep = 1;
                Game_ResetSubsystems();
                NetMode_ReleaseTextures(0);
            }
            break;
        case 1:
            NetMode_Init();
            gNetDropped = 0;
            gNetStepTimeout = CONN_FAILED_DELAY;
            gLoginConnectStep++;
            NetMode_OpenFrame();
            break;
        case 2:
            if (--gNetStepTimeout <= 0) {
                gLoginConnectStep++;
                gNetStepTimeout = CONN_FAILED_FRAMES;
                NetMode_OpenIcon();
            }
            break;
        case 3:
            NetModeMsg_QueuePage4(CONN_FAILED_MSG);
            if (--gNetStepTimeout <= 0) {
                gLoginConnectStep++;
            } else if (gNetStepTimeout < CONN_FAILED_SKIP_BELOW && LoginPad_Decide(2) != 0) {
                gLoginConnectStep++;
            }
            break;
        case 4:
            gLoginConnectStep++;
            NetMode_CloseAll();
            break;
        case 5:
            gNetEntryMode = LOGIN_ENTRY_QUEST_RETURN;
            NetScene_SetStep(task, NET_STEP_CONNECT);
            gScene.step = 0;
            break;
        }
        NetMode_CallDrawScreen(0);
        NetScene_DrawFrame();
        break;
    }
    return 0;
}

/* NET_SCENE_RUN offline: while the pause is shown (PAUSE_CAPTURED or
 * PAUSE_SHOWN) only the HUD runs over the captured screen; PAUSE_LEAVING
 * ends the pause. Else the lobby screen runs: its 1 (a quest set out) moves
 * the task to NET_SCENE_TO_QUEST, its -1 returns -2 (the scene quits).
 * Else returns 0. */
s32 NetScene_StateOffline(TaskSlot *task)
{
    if (Lobby_GetPauseState() >= PAUSE_CAPTURED) {
        if (Lobby_GetPauseState() == PAUSE_LEAVING) {
            gScene.pauseState = PAUSE_NONE;
        } else {
            if (gMenuInputLock > 0) {
                gMenuInputLock--;
            }
            gLobby->buttons = LobbyPad_Read(0);
            Hud_Update();
            Frame_DrawOverCapture();
            return 0;
        }
    }
    switch (LobbyScreen_Run()) {
    case 0:
        NetScene_DrawFrame();
        break;
    case 1:
        task->mode++;
        gNetMsgState.step = 20;
        gSysSettings.offline = 1;
        gSysSettings.unk40 = 0;
        break;
    case -1:
        return -2;
    }
    return 0;
}

/* Back to NET_SCENE_START. */
void NetScene_Restart(TaskSlot *task)
{
    NetScene_SetMode(task, NET_SCENE_START);
}

/*
 * After a quest's teardown: drops the quest server's connection. Step 0
 * closes the quest network and reloads the lobby's overlays; step 1 closes
 * the lobby's socket (LobbyNet_AbortSocket, whose result is kept in
 * gNetRetries); step 2 waits DROP_WAIT_FRAMES (counted in gNetCancelHold),
 * then restarts the scene: with the result 2 (the one AbortSocket gives),
 * the connection again as after a quest; else the portal, with the link
 * marked dropped. These two steps borrow the network state's cancelHold and
 * retries (gNetMsgState) as a timer and a result. Returns 1 when done, else
 * 0.
 */
s32 NetScene_DropConnection(TaskSlot *task)
{
    s32 result;

    switch (task->step) {
    case 0:
        task->step++;
        func_8FC6A0();
        Game_ResetSubsystems();
        NetScene_LoadOverlays();
        Town_Reset();
        gScene.inLobby = 1;
        break;
    case 1:
        result = LobbyNet_AbortSocket();
        if (result != 0) {
            task->step++;
            gNetCancelHold = DROP_WAIT_FRAMES;
            gNetRetries = result;
        }
        break;
    case 2:
        if (--gNetCancelHold <= 0) {
            if (gNetRetries == 2) {
                gLoginState = LOGIN_START;
                gNetEntryMode = LOGIN_ENTRY_QUEST_RETURN;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                NetScene_SetStep(task, NET_STEP_CONNECT);
                gScene.step = 0;
                NetScene_Restart(task);
            } else {
                gLoginState = LOGIN_START;
                gNetConnStep = 0;
                gNetDropped = 1;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
                task->mode = NET_SCENE_START;
                task->step = NET_STEP_PORTAL;
                NetScene_SetStep(task, NET_STEP_PORTAL);
                gScene.step = 2;
                NetScene_Restart(task);
            }
            gNetMsgState.handler = 0;
            return 1;
        }
        break;
    }
    Loading_Request();
    return 0;
}

/* Moves the scene task to the KDDI portal (NET_STEP_PORTAL), the stages'
 * states cleared; from the portal's top page the next login is a first
 * entry. */
void NetScene_ToPortal(void)
{
    NetScene_SetStep(TaskSlot_Get(TASK_SLOT_SCENE), NET_STEP_PORTAL);
    gLoginState = 0;
    gNetConnStep = 0;
    gLbsConnectStep = 0;
    gNetScreenStep = 0;
    gLoginConnectStep = 0;
    if (gPortalPage == PORTAL_PAGE_TOP) {
        gNetEntryMode = LOGIN_ENTRY_FIRST;
    }
    NetMode_ResetStep();
}

/* Moves the scene task to the leave (NET_STEP_LEAVE) at its NET file
 * check (gLoginState LOGIN_LEAVE_DONE). */
void NetScene_ToNetFileCheck(void)
{
    NetScene_SetStep(TaskSlot_Get(TASK_SLOT_SCENE), NET_STEP_LEAVE);
    gNetConnStep = 0;
    gLoginState = LOGIN_LEAVE_DONE;
    gLbsConnectStep = 0;
    gNetScreenStep = 0;
    gLoginConnectStep = 0;
}

/* Moves the scene task to the leave (NET_STEP_LEAVE) at its disconnect
 * (gLoginState LOGIN_LEAVE_DISCONNECT). */
void NetScene_ToDisconnect(void)
{
    NetScene_SetStep(TaskSlot_Get(TASK_SLOT_SCENE), NET_STEP_LEAVE);
    gNetConnStep = 0;
    gLoginState = LOGIN_LEAVE_DISCONNECT;
    gLbsConnectStep = 0;
    gNetScreenStep = 0;
    gLoginConnectStep = 0;
}

/* The lobby's copy of the offline pause's request. Online it only clears
 * the pause and returns 0. pause 1 asks for the pause when none is under
 * way (PAUSE_REQUESTED) and holds the lobby's clock; pause 0 ends a shown
 * pause (PAUSE_LEAVING) and sets the clock back to gLobbyTime. Returns 1
 * if the state changed, else 0. */
s32 Lobby_SetPause(s32 pause)
{
    s32 state;
    u32 time;

    if (Game_IsOnline() == 1) {
        gScene.pauseState = PAUSE_NONE;
        return 0;
    }
    state = gScene.pauseState;
    if (pause == 0 && state >= PAUSE_CAPTURED && state <= PAUSE_SHOWN) {
        time = gLobbyTime;
        gScene.pauseState = PAUSE_LEAVING;
        LobbyWorld_SetClock(time);
        return 1;
    }
    if (pause == 1 && state == PAUSE_NONE) {
        gScene.pauseState = PAUSE_REQUESTED;
        LobbyWorld_HoldClock();
        return 1;
    }
    return 0;
}

/* The offline pause's state (PAUSE_*). */
s32 Lobby_GetPauseState(void)
{
    return gScene.pauseState;
}
