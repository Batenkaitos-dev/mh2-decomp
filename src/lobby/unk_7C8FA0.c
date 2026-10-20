/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "net_save.h"
#include "syssettings.h"
#include "net_mode.h"
#include "em.h"
#include "m2c_macros.h"
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
s32 NetSetup_ShowLoginError(void);
void FadeA_Start(s32);
u8 FadeA_GetState(void);
s32 Save_TestEventFlag(s32);
s32 Movie_Update(void);
void Movie_Stop(void);
void Movie_RequestStop(void);
s32 Movie_Play(s32);
s32 Movie_GetStatus(void);
void Dmcam_PreloadAlt(void);

/* prototypes */
void Tex_FreeRange(s32 a, s32 b);
void Browser_Init(u8 mode);         /* opens the HTML browser at gBrowserUrl */
void LobbyKddi_CloseBrowser(void);          /* closes the browser */
s32 Browser_Update(void);           /* the browser's step: 0 running, 1 done, 2, -1 failed, -100 */
void NetMode_SeqAReset(void);
extern char gBrowserUrl[0x100];       /* the URL the browser opens */
extern char gPortalUrlA[];
extern char gPortalUrlB[];
char *strcpy(char *dst, const char *src);
extern char D_00847100[];          /* the KDDI portal's top page (a URL) */
f32 Math_Cos(f32);
f32 Math_Sin(f32 x);
extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
void NetModeMsg_Page0(s32 a0);
s32 LobbyNet_DisconnectWait(void);
s32 NetMode_SeqABusy(void);            /* one caller reads it as s8 */
s32 NetMode_SeqAStart(s32 arg0, s32 arg1, s64 arg2);
extern s8 gNetDisconnectStep;
void ServerList_ClearTried(void);
s32 LoginPad_Any8000(void);
void NetMode_OpenIcon2(s8 arg0);
void NetMode_OpenProgress(s32 arg0);
void NetMode_OpenPercent(void);
void Net_ClearState(void);
s32 NetMsg_Update(void);
s32 LobbyCard_Select(void);
s32 NetSetup_Connect(void);
void Sys_EmptyHook(void);
s32 NetSetup_Mode5(void);
s32 NetSetup_SaveNetFile(void);
s32 NetSetup_Dnas(void);
s32 LobbyKddi_Portal(void);
s32 NetSetup_SettingsMenu(void);
void NetMode_EmptyDrawHook(s32 unused);
s32 NetSetup_Login(void);
s32 NetSetup_Disconnect(void);
/* end of prototypes */

/* sub_main's task slots (TaskSlot, task.h): the network mode runs in slot 5 */

void NetScene_Run(TaskSlot *t);
void NetScene_Quit(void *arg0);
s32 LobbyLogin_StateConnect(TaskSlot *t);
s32 NetScene_StateOffline(TaskSlot *t);
s32 NetScene_DropConnection(TaskSlot *t);
void TaskSlot_Prepare(void *func, s16 slot);
void Scene_QuestTask(u8 *obj);     /* a sub_main task */

void Lobby_ResetState(void);
s32 Lobby_Tick(void);
void FadeA_Reset(void);
s32 LobbyConn_Run(void);
void Loading_Reset(void);
void FadeB_Start(s32);
s32 LobbyLogin_Leave(void);
void FadeB_Reset(void);
s32 LobbyLogin_Exit(void);
void NetMode_ResetToFrame(void);
void NetMode_OpenIcon(void);
void NetMode_CloseAll(void);
void NetModeMsg_QueueKind3(u8 id);
s32 LoginPad_Decide(s32 sound);
void NetMode_CallDrawScreen(s32 a0);
s32 LobbyLogin_StepConnect(TaskSlot *t);
s32 LobbyLogin_RunConnect(void);
s32 Game_IsOnline(void);
s32 LobbyScreen_Run(void);
u16 LobbyPad_Read(s8 port);
void Hud_Update(void);
void Frame_DrawOverCapture(void);
extern u8 gMenuInputLock;
extern u32 gLobbyTime;
void LobbyWorld_SetClock(u32);
void LobbyWorld_HoldClock(void);
void LobbyWorld_Reset(void);
void LobbyLogin_DrawMenu(void);
s32 LobbyKddi_PortalScene(void);
void NetScene_ToState6(void);
void NetScene_Finish(TaskSlot *t);
extern s8 gNetDropped;
extern s8 gLoginExitStep;
extern s16 gNetStepTimeout;


/* the object at *(0x6AE040) (0x58 bytes, LoginWave_Clear clears it) */
typedef struct Unk6AE040 {
    /* 0x00 */ s32 state;
    /* 0x04 */ u8 pad4[0xC];
    /* 0x10 */ s32 unk10[2];
    /* 0x18 */ u8 pad18[8];
    /* 0x20 */ f32 wave[6];         /* sines of the six angles */
    /* 0x38 */ u16 unk38[6];        /* angles (0x10000 a turn) */
    /* 0x44 */ u16 unk44[6];        /* their steps per frame */
    /* 0x50 */ s32 unk50;           /* the menu cursor (-1 at the start) */
    /* 0x54 */ s32 unk54;
} Unk6AE040;

extern u16 gLoginWaveValues[];
extern s16 gLoginFontCodes[];
extern u8 gLoginFontColumns[];
/* a sprite of the login screen's tables (LobbyLogin_DrawSprites, 0x18 bytes) */
typedef struct LoginSpr {
    s16 tex;
    s16 pad2;
    s16 x, y, w, h;
    s16 u0, v0, u1, v1;
    u32 color;
} LoginSpr;

/* a menu item's box on the login screen */
typedef struct LoginBox {
    s16 x, y, w, h;
} LoginBox;

extern u32 gLoginItemColors[8];     /* the menu items' colours ([0] unused) */
extern u32 gLoginLitColors[8];     /* the lit item's colours ([0] unused) */
extern u8 gLoginPageColorIdx[];
extern LoginBox gLoginBoxSprites[];
s32 LobbySrv_ColorAt(s32 i);
void LobbyTitle_Draw(void);
void NetScene_DrawFrame(void);
static void NetScene_SetMode(TaskSlot *t, s8 mode);
static void NetScene_SetStep(TaskSlot *t, s8 step);
void NetScene_LoadOverlays(void);
void Lobby_SetBootMode(s32 arg0);
static void func_007C95F0(void);
static void func_007C9600(void);
s32 Lobby_GetDayState(void);
u8 Lobby_GetNetOption0(void);
s32 Lobby_GetNetOption1(void);
void NetMode_Init(void);
void NetMode_OpenIcon5(void);
void NetMode_OpenIcon7(void);
void NetMode_ReloadFrame(void);
void NetSetupTask_Init(TaskSlot *t);
void NetSetupTask_Step(TaskSlot *t);
s32 NetMode_RunSetup(void);
void NetMode_PlayBgm(void);
void NetMode_OpenBit21(void);
void NetSetupTask_Quit(TaskSlot *t);
void NetSetupTask_StartScene(TaskSlot *t);
void Lobby_SetSysBusy(void);
void NetMode_SetMode(s8 arg0);
void NetMode_Clear(void);
s32 NetMode_StartSetup(void);
void NetMode_CloseYesNoBox(void);
void NetSetup_ResetLobby(void);
s32 NetSetup_Leave(void);
s32 NetSetup_Quit(void);
s32 NetSetup_Mode2(void);
void NetMode_ResetAnim(void);
void NetMode_ResetStep(void);
void LoginMenu_PlayCursorSe(void);
void LoginMenu_PlayDecideSe(void);
s32 LoginPad_CancelPressed(s32 port);
TaskSlot *TaskSlot_Get(s16 slot);
void Gfx_ClearModelBuffers(void);
extern s8 gLoginState;
extern s8 gNetConnStep;
extern s8 gLbsConnectStep;
extern s8 gNetScreenStep;
extern u8 gLoginConnectStep;
extern s8 gNetEntryMode;
void func_8FC6A0(void);   /* QNetIf_Close (gm_sub) */
void Loading_Request(void);
void Town_Reset(void);
s32 LobbyNet_AbortSocket(void);
s32 Tex_LoadPack(s32, s32, s32);
void Tex_LoadSystemPack(s32);
void Scene_ResetAll(void);
void NetMode_BindTextures(void);
struct NetSprDef;
void LobbyLogin_DrawSprites(struct NetSprDef *d, s32 n, s32 dy);
s32 Gs_SetState(u8, u32);
s32 Gs_GetState(u8);
void Gfx_SetTexFilter(s32);
extern LoginSpr gLoginFrameSprites[];
s32 LoginPad_DecidePressed(s32);
void LoginSe_Decide(void);
void LoginSe_Play9(void);
s32 LoginPad_Held8000(s32);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void NetModeMsg_Queue(s32, s32, s32);
void NetMode_OpenYesNoBar(void);
void Scene_LoadOverlay(s32 id);
void Frame_Draw(void);
void NetMode_LoadTextures(s32 mode);
void LoginWave_Clear(void);
void NetMode_ReleaseTextures(M2C_UNK unused0);
void func_007CF620(void);
void func_007CF650(void);
void NetMode_OpenFrame(void);
void NetMode_OpenIconN(s32);
void *memset(void *, s32, u32);
void *memcpy(void *dst, const void *src, u32 n);
void CardTask_Begin(s32 mode);
s32 LoadCompressedFile(u8 *, s32);
s32 CardOp_SetFiles(s32 *data);
void CardTask_StartFind(s32 slot, u8 *buf, s32 a);
void CardTask_StartRead(s32 slot, u8 *buf);
void CardTask_StartSave(s32 slot, u8 *buf);
void CardTask_StartPoll(void);
void CardTask_StartFormat(s32 slot);
void CardTask_Update(void);
s32 CardTask_Result(void);
s32 Card_PortState(s32 slot);
s32 LobbyCard_CheckNetValues(u8 *buf);
extern s32 gTexHandles[];
void NetMode_BgmOn(void);
void NetMode_CloseBoxes(s32);
void NetMode_CloseYesNo(void);
void Lobby_BackupWork(void);
extern M2C_UNK D_008FAAA0;
void Sound_Silence(void);
void NetMode_SetTitle(s32);
void NetMode_ClearBuffers(void);
void LoginSe_Cursor(void);
void NetMode_OpenFrame4E(void);
void Sound_LoadCommonBanks(void);
void Lobby_ClearSysBusy(void);
void Mem_Compact(void);
void TaskSlot_End(TaskSlot *t);
void Task_StartBoot(void);
void Game_ResetSubsystems(void);
void TaskSlot_Set(void *func, s16 slot);
void Scene_EmptyFrameHook(void);   /* Scene_EmptyFrameHook (sub_main), empty */
u8 func_8FC6B0(void);     /* QNetIf_GetCount (gm_sub) */
void func_8FC600(void);   /* QNetIf_UpdateReceive (gm_sub) */
s32 func_8FC5F0(void);    /* QNetIf_IsRunning (gm_sub) */
void QuestSync_Clear(void);
s32 LoginPad_Triggered(void);
s32 LoginPad_Cancel(void);
s32 NetMode_ChooseYesNo(void);

void NetScene_DrawFrame(void) {
    Frame_Draw();
}

static void NetScene_SetMode(TaskSlot *t, s8 mode) {
    t->mode = mode;
    t->step = 0;
}

static void NetScene_SetStep(TaskSlot *t, s8 step) {
    t->step = step;
}

void NetScene_LoadOverlays(void) {
    Scene_LoadOverlay(4);
    Scene_LoadOverlay(0xA);
}

void NetScene_LoadOverlayA(void)
{
    Scene_LoadOverlay(0xA);
}

/* counts the play time: every 30 frames (in the server mode, or while
 * Lobby_GetDayState is below 2) a second more in the save (0x5D6A9C, at most
 * 35999999: 9999:59:59) */
void Lobby_CountPlayTime(void)
{
    (gScene.frame)++;
    if (Game_IsOnline() != 0 || Lobby_GetDayState() < 2) {
        if (gScene.frame % 30 == 0) {
            (*(u32 *)&gSave.playTime)++;
            if (*(u32 *)&gSave.playTime > 35999999) {
                gSave.playTime = 35999999;
            }
        }
    }
}

/* the network mode's scene task (sub_main task slot 5): 0 the opening
 * checks (4 plays the opening movie first), 1 resets the subsystems, 2
 * connects, 3 hands over to the next task (0x3260B0) */
void NetScene_Run(TaskSlot *t)
{
    s32 i;
    s32 r;

    Scene_EmptyFrameHook();
    switch (t->mode) {
    case 0:
        gSysSettings.petDayPending = 1;
        if (Game_IsOnline() == 0 && Save_TestEventFlag(0) == 0) {
            NetScene_SetMode(t, 4);
            break;
        }
    restart:
        Town_Reset();
        QuestSync_Clear();
        if (gScene.scene == 6 && gScene.step == 0 && NetScene_DropConnection(t) == 0) {
            break;
        }
        Dmcam_PreloadAlt();
        NetScene_LoadOverlayA();
        FadeA_Start(2);
        if (gScene.scene == 6) {
            Gfx_ClearModelBuffers();
            Scene_ResetAll();
            gScene.inLobby = 1;
            NetScene_SetMode(t, 2);
            break;
        }
        Scene_ResetAll();
        gSysSettings.offline = 0;
        gScene.inLobby = 1;
        gScene.unk03D = 1;
        gScene.host = 0;
        NetScene_SetMode(t, 1);
    case 1:
        if (Game_IsOnline() == 1) {
            NetMode_ReleaseTextures(0);
            Game_ResetSubsystems();
            NetScene_SetMode(t, 2);
            break;
        }
        Lobby_ResetState();
        NetMode_ReleaseTextures(0);
        Game_ResetSubsystems();
        NetScene_SetMode(t, 2);
        break;
    case 2:
        if (Game_IsOnline() == 1) {
            r = LobbyLogin_StateConnect(t);
        } else {
            r = NetScene_StateOffline(t);
        }
        if (r == -1) { /* compiled-out check */
        } else if (r == -2) {
            NetScene_Quit(t);
        }
        break;
    case 3:
        if (Game_IsOnline() == 1) {
            Loading_Request();
        }
        gSoundLoading = 0;
        if (Game_IsOnline() == 1) {
            if (gScene.playerCount == 0) {
                gScene.playerCount = func_8FC6B0();
                gScene.host = 0;
                for (i = 0; i < 4; i++) {
                    if (i < gScene.playerCount) {
                        gScene.inQuest[i] = 1;
                    } else {
                        gScene.inQuest[i] = 0;
                    }
                }
            }
            func_8FC600();
            FadeA_Reset();
            if (func_8FC5F0() == 0 && gScene.inQuest[gScene.localPlayer] != 0xFF) {
                break;
            }
        } else {
            gScene.playerCount = 1;
            for (i = 0; i < 4; i++) {
                if (i < gScene.playerCount) {
                    gScene.inQuest[i] = 1;
                } else {
                    gScene.inQuest[i] = 0;
                }
            }
        }
        gSysSettings.offline = 1;
        TaskSlot_Prepare(Scene_QuestTask, 5);
        break;
    case 4:
        switch (t->step) {
        case 0:
            FadeA_Start(2);
            Movie_Play(6);
            t->step++;
        case 1:
            Movie_Update();
            r = Movie_GetStatus();
            if (r == MOVIE_STATUS_FINISHED || (r == MOVIE_STATUS_PLAYING && (gPadState[0].pressed & 0x8000))) {
                FadeA_Start(1);
                t->step++;
                Movie_RequestStop();
            }
            break;
        case 2:
            Movie_Update();
            if ((u8)FadeA_GetState() != 1) {
                Movie_Stop();
                NetScene_SetMode(t, 0);
                goto restart;
            }
            break;
        }
        break;
    }
}

void NetScene_Quit(void *t)
{
    u8 temp_v1;

    Game_ResetSubsystems();
    Gfx_ClearModelBuffers();
    memset(&gScene, 0, 0x2BC);
    memset(&gNetMode, 0, 0xE0);
    gOnlineFlag = 0;
    gSysSettings.unk40 = 0;
    TaskSlot_End(t);
    gGmSubLoaded = 0;
    Scene_LoadOverlay(2);
    temp_v1 = gBootMode;
    if ((temp_v1 != 1) && (temp_v1 != 2)) {
        Lobby_SetBootMode(1);
    }
    Task_StartBoot();
}

void Lobby_SetBootMode(s32 mode) {
    gBootMode = mode;
}

static void func_007C95F0(void) {

}

static void func_007C9600(void) {

}

void NetScene_Finish(TaskSlot *t)
{
    NetScene_SetMode(t, 3);
    gNetMsgState.handler = 0;
    gNetMsgState.unk0 = 4;
    gNetMsgState.step = 0;
    gNetMsgState.timer = 0;
    gScene.inLobby = 0;
    gSysSettings.unk40 = 0;
    gScene.playerCount = 0;
}

/* step 1 of state 5: the connection sequence (LobbyLogin_RunConnect); 1: on to
 * step 2 (returns -1), -1: step 7 then 5, -2: step 4, 2: LobbyLogin_DrawMenu */
s32 LobbyLogin_StepConnect(TaskSlot *t)
{
    switch (LobbyLogin_RunConnect()) {
    case 1:
        NetScene_SetStep(t, 2);
        gScene.inLobby = 1;
        LobbyWorld_Reset();
        Lobby_ResetState();
        return -1;
    case -1:
        NetScene_SetStep(t, 7);
        gLoginExitStep = 5;
        break;
    case -2:
        NetScene_SetStep(t, 4);
        break;
    case 2:
        LobbyLogin_DrawMenu();
        break;
    }
    NetScene_DrawFrame();
    return 0;
}

/* state 5 of the network mode's scene task (sub_main task slot 5): its
 * steps (t->step) are the connection's stages; returns -1 or -2 when the
 * mode ends, else 0 */
s32 LobbyLogin_StateConnect(TaskSlot *t)
{
    s32 r;

    switch (t->step) {
    case 0:
        func_007C95F0();
        NetScene_SetStep(t, 1);
        Lobby_ResetState();
        gSysSettings.unk40 = 0;
        func_007C9600();
        return -1;
    case 1:
        if (LobbyLogin_StepConnect(t) < 0) {
            return -1;
        }
        break;
    case 2:
        func_007C95F0();
        if (Lobby_Tick() == 0) {
            NetScene_SetStep(t, 3);
            gNetEntryMode = LOGIN_ENTRY_QUEST_RETURN;
            FadeA_Reset();
        }
        func_007C9600();
        if (gLobby->phase != 4) {
            NetScene_DrawFrame();
        }
        break;
    case 3:
        switch (LobbyConn_Run()) {
        case 1:
            NetScene_Finish(t);
            break;
        case -1:
            Loading_Reset();
            NetScene_SetStep(t, 8);
            gLoginState = 0;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            FadeB_Start(0xA);
            break;
        }
        NetScene_DrawFrame();
        break;
    case 4:
        switch (LobbyLogin_Leave()) {
        case 1:
            NetScene_SetStep(t, 7);
            gLoginExitStep = 5;
            FadeB_Reset();
            break;
        case 2:
            gPortalPage = PORTAL_PAGE_QUIT;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            NetScene_ToState6();
            FadeB_Reset();
            break;
        case 3:
            NetScene_SetStep(t, 7);
            gLoginExitStep = 0;
            FadeB_Reset();
            break;
        default:
            NetScene_DrawFrame();
            break;
        }
        break;
    case 5:
        return -2;
    case 6:
        r = LobbyKddi_PortalScene();
        if (r != 0) {
            if (r == -1 && gPortalPage == PORTAL_PAGE_TOP) {
                NetScene_SetStep(t, 4);
                gNetConnStep = 0;
                gLoginState = 1;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            } else {
                NetScene_SetStep(t, 1);
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            }
        }
        break;
    case 7:
        switch (LobbyLogin_Exit()) {
        case 1:
            NetScene_SetStep(t, gLoginExitStep);
            gLoginExitStep = 0;
            break;
        }
        NetScene_DrawFrame();
        break;
    case 8:
        switch (gLoginConnectStep) {
        case 0:
            Loading_Reset();
            if (gOvlTex != 0) {
                if (gNetMode.boxes != 0) {
                    gNetDropped = 0;
                    gLoginConnectStep = 2;
                    gNetStepTimeout = 0x14;
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
            gNetStepTimeout = 0x14;
            gLoginConnectStep++;
            NetMode_OpenFrame();
            break;
        case 2:
            if (--gNetStepTimeout <= 0) {
                gLoginConnectStep++;
                gNetStepTimeout = 0x258;
                NetMode_OpenIcon();
            }
            break;
        case 3:
            NetModeMsg_QueueKind3(0xE);
            if (--gNetStepTimeout <= 0) {
                gLoginConnectStep++;
            } else if (gNetStepTimeout < 0x1E1 && LoginPad_Decide(2) != 0) {
                gLoginConnectStep++;
            }
            break;
        case 4:
            gLoginConnectStep++;
            NetMode_CloseAll();
            break;
        case 5:
            gNetEntryMode = LOGIN_ENTRY_QUEST_RETURN;
            NetScene_SetStep(t, 1);
            gScene.step = 0;
            break;
        }
        NetMode_CallDrawScreen(0);
        NetScene_DrawFrame();
        break;
    }
    return 0;
}

s32 NetScene_StateOffline(TaskSlot *t)
{
    if (Lobby_GetDayState() >= 2) {
        if (Lobby_GetDayState() == 4) {
            gScene.pauseState = 0;
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
        t->mode++;
        gNetMsgState.step = 0x14;
        gSysSettings.offline = 1;
        gSysSettings.unk40 = 0;
        break;
    case -1:
        return -2;
    }
    return 0;
}

void NetScene_Restart(void *em)
{
    NetScene_SetMode(em, 0);
}

s32 NetScene_DropConnection(TaskSlot *t)
{
    s32 r;

    switch (t->step) {
    case 0:
        t->step++;
        func_8FC6A0();
        Game_ResetSubsystems();
        NetScene_LoadOverlays();
        Town_Reset();
        gScene.inLobby = 1;
        break;
    case 1:
        r = LobbyNet_AbortSocket();
        if (r != 0) {
            t->step++;
            D_59D666 = 100;
            D_59D668 = r;
        }
        break;
    case 2:
        if (--D_59D666 <= 0) {
            if (D_59D668 == 2) {
                gLoginState = 0;
                gNetEntryMode = LOGIN_ENTRY_QUEST_RETURN;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                NetScene_SetStep(t, 1);
                gScene.step = 0;
                NetScene_Restart(t);
            } else {
                gLoginState = 0;
                gNetConnStep = 0;
                gNetDropped = 1;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
                t->mode = 0;
                t->step = 6;
                NetScene_SetStep(t, 6);
                gScene.step = 2;
                NetScene_Restart(t);
            }
            gNetMsgState.handler = 0;
            return 1;
        }
        break;
    }
    Loading_Request();
    return 0;
}

void NetScene_ToState6(void)
{
    NetScene_SetStep(TaskSlot_Get(5), 6);
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

/* the scene task of the network mode: to state 4 (the NET file checks) */
void NetScene_ToNetFileCheck(void)
{
    NetScene_SetStep(TaskSlot_Get(5), 4);
    gNetConnStep = 0;
    gLoginState = 4;
    gLbsConnectStep = 0;
    gNetScreenStep = 0;
    gLoginConnectStep = 0;
}

void NetScene_ToState4b(void)
{
    NetScene_SetStep(TaskSlot_Get(5), 4);
    gNetConnStep = 0;
    gLoginState = 1;
    gLbsConnectStep = 0;
    gNetScreenStep = 0;
    gLoginConnectStep = 0;
}

/* the day's first connection check (0x6ADCD5 its state; not in the
 * server mode): kind 0 in states 2-3 moves the clock to gLobbyTime
 * (state 4), kind 1 in state 0 holds the clock (state 1); 1 if done */
s32 Lobby_CheckDay(s32 kind)
{
    s32 st;
    u32 t;

    if (Game_IsOnline() == 1) {
        gScene.pauseState = 0;
        return 0;
    }
    st = gScene.pauseState;
    if (kind == 0 && st >= 2 && st <= 3) {
        t = gLobbyTime;
        gScene.pauseState = 4;
        LobbyWorld_SetClock(t);
        return 1;
    }
    if (kind == 1 && st == 0) {
        gScene.pauseState = 1;
        LobbyWorld_HoldClock();
        return 1;
    }
    return 0;
}

s32 Lobby_GetDayState(void) {
    return gScene.pauseState;
}

u8 Lobby_GetNetOption0(void) {
    return gSysSave.netOptions[0];
}

s32 Lobby_GetNetOption1(void) {
    return gSysSave.netOptions[1];
}

void NetMode_Init(void) {
    NetMode_ReleaseTextures(0);
    func_007CF620();
    func_007CF650();
    gNetMode.boxes = 0;
    NetMode_LoadTextures(0);
    LoginWave_Clear();
}

void NetMode_OpenFrame4E(void) {
    NetMode_OpenFrame();
    (*(s32 *)&gNetMode.iconY) = 0x4E;
}

void NetMode_OpenIcon5(void) {
    NetMode_OpenIconN(5);
    (*(s32 *)&gNetMode.iconY) = 0x15E;
}

void NetMode_OpenIcon7(void) {
    NetMode_OpenIconN(7);
    (*(s32 *)&gNetMode.iconY) = 0x14C;
}

void NetMode_ReloadFrame(void) {
    Sound_LoadCommonBanks();
    NetMode_LoadTextures(0);
    NetMode_OpenFrame4E();
}

void NetMode_SetTitle(s32 title)
{
    gNetMode.title = (s8) title;
    gNetMode.boxes = (s32) (gNetMode.boxes | 0x80000);
}

void NetMode_LoadTextures(s32 mode)
{
    NetModeState *st;
    s32 i;
    s32 n;

    st = &gNetMode;
    Tex_FreeRange(0xA3, 0xE);
    Tex_FreeRange(2, 0xA);
    for (n = 0; n < 16; n++) {
        st->tex[n] = 0;
    }
    n = Tex_LoadPack(gNetSetupTexPack, 0xA3, 0);
    for (i = 0; i < n; i++) {
        st->tex[i] = gTexHandles[0xA3 + i];
    }
    if (mode == 0) {
        Tex_LoadSystemPack(1);
        st->tex[i++] = gLoadLettersTex;
        st->tex[i++] = D_6AE3F8;
        st->tex[i] = gCardMenuTex;
    }
    Mem_Compact();
}

/* the network screen's textures: tex[0-10] from the lobby's pack
 * (gTexHandles[0xA3...]), tex[11-13] sub_main's 8, 10 and 7 */
void NetMode_BindTextures(void)
{
    s32 i;
    NetModeState *st;

    st = &gNetMode;
    for (i = 0; i < 11; i++) {
        st->tex[i] = gTexHandles[0xA3 + i];
    }
    st->tex[i++] = gTexHandles[8];
    st->tex[i++] = gTexHandles[10];
    st->tex[i] = gTexHandles[7];
}

void NetSetupTask_Run(void *t)
{
    u8 mode;

    mode = *(u8 *)((u8 *)t + 8);
    switch (mode) {
    case 0:
        NetSetupTask_Init(t);
        break;
    case 1:
        NetSetupTask_Step(t);
        break;
    }
}

void NetSetupTask_Init(TaskSlot *t)
{
    s32 n;
    s32 i;
    NetModeState *st;

    st = &gNetMode;
    Game_ResetSubsystems();
    Gfx_ClearModelBuffers();
    Scene_ResetAll();
    Lobby_SetSysBusy();
    NetMode_Clear();
    NetMode_ReleaseTextures(0);
    n = Tex_LoadPack(gNetSetupTexPack, 0xA3, 0);
    for (i = 0; i < n; i++) {
        st->tex[i] = gTexHandles[0xA3 + i];
    }
    Tex_LoadSystemPack(1);
    st->tex[i++] = gLoadLettersTex;
    st->tex[i++] = D_6AE3F8;
    st->tex[i] = gCardMenuTex;
    Mem_Compact();
    gPortalPage = PORTAL_PAGE_TOP;
    gBootMode = 1;
    t->mode++;
    t->step = 0;
}

/* state 1 of the task: a yes/no question (NetMode_RunSetup), then either end */
void NetSetupTask_Step(TaskSlot *t)
{
    switch (t->step) {
    case 0:
        switch (NetMode_RunSetup()) {
        case 0:
            break;
        case 1:
            t->step = 1;
            break;
        case -1:
            t->step = 2;
            break;
        }
        break;
    case 1:
        NetSetupTask_StartScene(t);
        break;
    case 2:
        NetSetupTask_Quit(t);
        return;
    }
}

void NetSetupTask_Quit(TaskSlot *t) {
    Lobby_ClearSysBusy();
    NetMode_ReleaseTextures(0);
    Mem_Compact();
    NetMode_ClearBuffers();
    Game_ResetSubsystems();
    memset(&gScene, 0, 0x2BC);
    gOnlineFlag = 0;
    gSysSettings.unk40 = 0;
    Scene_LoadOverlay(2);
    TaskSlot_End(t);
    Task_StartBoot();
}

void NetSetupTask_StartScene(TaskSlot *t) {
    Lobby_ClearSysBusy();
    NetMode_ReleaseTextures(0);
    NetMode_ClearBuffers();
    Game_ResetSubsystems();
    memset(&gScene, 0, 0x2BC);
    gSysSettings.unk40 = 0;
    TaskSlot_End(t);
    TaskSlot_Set((M2C_UNK *)NetScene_Run, 5);
}

void Lobby_SetSysBusy(void) {
    gSysBusy = 1;
}

void Lobby_ClearSysBusy(void) {
    gSysBusy = 0;
}

void NetMode_SetMode(s8 mode) {
    gNetMode.mode = mode;
    gNetMode.step = 0;
    gNetMode.subStep = 0;
    gNetMode.innerStep = 0;
}

void NetMode_Clear(void) {
    memset(&gNetMode, 0, 0xE0);
    memset(&gNetSaveData, 0, sizeof(NetSaveData));
}

s32 NetMode_RunSetup(void)
{
    s32 t;
    s32 k;
    s32 r;
    NetModeState *st;

    r = 0;
    st = &gNetMode;
    switch (gNetMode.mode) {
    case 0:
        if (NetMode_StartSetup() != 0) {
            NetMode_SetMode(1);
            Lobby_SetBootMode(2);
        }
        break;
    case 1:
        t = LobbyCard_Select();
        if (t != 0) {
            if (t == 1) {
                gSysSettings.unk4A = gNetMode.slot;
                if (Lobby_GetNetOption1() == 0) {
                    NetMode_CloseBoxes(0x20);
                    NetMode_SetMode(2);
                } else {
                    NetMode_SetMode(9);
                    st->subStep = 0xFF;
                }
            } else {
                NetMode_SetMode(0xD);
                r = -1;
            }
        }
        break;
    case 2:
        t = NetSetup_Mode2();
        if (t != 0) {
            if (t == 1) {
                NetMode_SetMode(5);
            } else {
                st->ynNoDraw = 0;
                NetMode_SetMode(9);
                st->subStep = 0xFF;
            }
        }
        break;
    case 3:
        k = NetSetup_Connect();
        if (k != 0) {
            NetMode_OpenFrame4E();
            if (k == 1) {
                NetMode_CloseBoxes(0x20);
                NetMode_SetMode(4);
            } else {
                NetMode_SetMode(9);
                if (st->ynNoDraw != 0) {
                    st->ynNoDraw = 0;
                } else {
                    NetMode_OpenIcon5();
                    st->subStep = 2;
                }
            }
        }
        break;
    case 4:
        t = NetSetup_Login();
        if (t != 0) {
            if (t == 1) {
                Sys_EmptyHook();
                NetMode_SetMode(6);
            } else {
                NetMode_SetMode(9);
                gNetMode.subStep = 0xFF;
            }
        }
        break;
    case 5:
        if (NetSetup_Mode5() != 0) {
            NetMode_SetMode(3);
        }
        break;
    case 6:
        if (NetSetup_SaveNetFile() != 0) {
            NetMode_SetMode(7);
        }
        break;
    case 7:
        t = NetSetup_Dnas();
        if (t != 0) {
            if (t == 1) {
                gPortalPage = PORTAL_PAGE_TOP;
                Lobby_SetBootMode(1);
                NetMode_SetMode(8);
            } else {
                NetMode_SetMode(0xB);
            }
        }
        break;
    case 8:
        t = LobbyKddi_Portal();
        if (t != 0) {
            if (t == -1 && gPortalPage == PORTAL_PAGE_TOP) {
                NetMode_SetMode(0xB);
            } else {
                NetMode_SetMode(0xA);
            }
        }
        break;
    case 9:
        k = NetSetup_SettingsMenu();
        if (k != 0) {
            if (k != 2) {
                NetMode_OpenFrame4E();
            }
            if (k == 1 || k == 2) {
                NetMode_SetMode(3);
            } else {
                NetMode_SetMode(0xD);
            }
        }
        break;
    case 10:
        if (NetSetup_Leave() != 0) {
            r = 1;
        }
        break;
    case 11:
    case 12:
        if (NetSetup_Disconnect() != 0) {
            NetMode_SetMode(0xD);
        }
        break;
    case 13:
        if (NetSetup_Quit() != 0) {
            r = -1;
        }
        break;
    }
    if (!(st->noDraw & 1)) {
        NetMode_EmptyDrawHook(0);
    }
    if (!(st->noDraw & 2)) {
        NetMode_CallDrawScreen(0);
    }
    return r;
}

/* Raw addresses kept here: the named form changes the code. */
void NetMode_BgmOn(void)
{
    u8 *st;

    st = (u8 *)&gNetMode;
    if ((*(u8 *)&gNetMode.bgmOn) == 0) {
        Sound_LoadCommonBanks();
        NetMode_PlayBgm();
        st[0xDD] = 1;
    }
}

/* Raw addresses kept here: the named form changes the code. */
void NetMode_BgmOff(void)
{
    u8 *st;

    st = (u8 *)&gNetMode;
    if ((*(u8 *)&gNetMode.bgmOn) == 1) {
        Sound_Silence();
        st[0xDD] = 0;
    }
}

s32 NetMode_StartSetup(void) {
    gNetMode.entry = 0;
    gNetMode.ynResult = 0;
    gNetMode.netResult = 0;
    gNetMode.unk10 = 0;
    NetMode_BgmOn();
    return 1;
}

void NetModeMsg_Page1(s32 a0)
{
    NetModeMsg_Queue(0x1, 0x1, a0);
}

void NetModeMsg_Queue5(s32 a0, s32 a1)
{
    NetModeMsg_Queue(0x5, a0, a1);
}

void NetModeMsg_Page3(s32 a0)
{
    NetModeMsg_Queue(0x1, 0x3, a0);
}

/* sets the yes/no cursor and opens the yes/no box */
void NetMode_OpenYesNo(s8 cursor)
{
    gNetMode.yesNo = cursor;
    gNetMode.yesNoOpen = 1;
    NetMode_OpenYesNoBar();
}

void NetMode_OpenYesNoAtNo(void)
{
    gNetMode.yesNo = 0x1;
    NetMode_OpenYesNoBar();
}

void NetMode_PickSlot(void)
{
    gNetMode.sel2 = gNetMode.slot;
    NetMode_OpenBit21();
}

void NetMode_CloseYesNoBox(void) {
    NetMode_CloseYesNo();
    NetMode_CloseBoxes(0x200000);
    gNetMode.yesNoOpen = 0;
}

/* the network mode's memory card step (step 0x5D46A1): finds the cards
 * (Lobby_GetNetOption0), reads the game data (gNetSaveData, 0x1BEC bytes) from the one
 * that has it, or offers another slot, a new save or deleting; card[] keeps
 * each slot's state. Returns 1 once the data is read (st->slot is the card),
 * -1 when the player quits, else 0. The second card's switch goes through
 * pad (a declared local: the card state takes a0, the reused constant 2 v1). */
s32 LobbyCard_Select(void)
{
    s32 pad;
    s32 r;
    NetModeState *st;

    r = 0;
    st = &gNetMode;
    pad = LoginPad_Triggered();
    switch (*(u8 *)&D_5D46A1) {
    case 0:
        NetMode_SeqAReset();
        NetMode_OpenFrame4E();
        NetMode_SetTitle(1);
        st->card[0] = -1;
        st->card[1] = -1;
        CardTask_Begin(1);
        LoadCompressedFile((u8 *)(*(s32 *)&gLoadBuf + 0x20000), 0x1715);
        st->files = CardOp_SetFiles((s32 *)(*(s32 *)&gLoadBuf + 0x20000));
        st->cardsFound = Lobby_GetNetOption0();
        if (st->cardsFound == 3) {
            st->step = 4;
            st->sel3 = 0;
            st->sel2 = 0;
            st->slot = 0;
            st->slotUsed = 0;
            st->msg = 0;
        } else {
            st->step = 1;
        }
        break;
    case 1:
        switch (st->subStep) {
        case 0:
            st->subStep++;
            st->sel3 = 0;
            st->sel2 = 0;
            if (st->cardsFound == 2) {
                st->slot = 1;
                st->slotUsed = 1;
            } else {
                st->slot = 0;
                st->slotUsed = 0;
            }
            st->msg = 0;
            CardTask_StartFind(st->slotUsed, gLoadBuf, 1);
            break;
        case 1:
            if (st->cardsFound == 0) {
                NetModeMsg_Page1(4);
            } else {
                NetModeMsg_Page1(0x2F);
            }
            CardTask_Update();
            switch (CardTask_Result()) {
            case -1:
                return 0;
            case 0:
                st->card[st->slotUsed] = 3;
                break;
            case -254:
            case -253:
                st->card[st->slotUsed] = 2;
                break;
            case -252:
                st->card[st->slotUsed] = 1;
                break;
            case -255:
                st->card[st->slotUsed] = 0;
                break;
            case -251:
                st->card[st->slotUsed] = 4;
                break;
            default:
                st->card[st->slotUsed] = 5;
                break;
            }
            if (st->cardsFound == 1) {
                st->slotUsed = 0;
                switch (st->card[0]) {
                case 2:
                case 1:
                    NetMode_OpenYesNo(0);
                    st->msg = 8;
                    goto done;
                case 0:
                    st->msg = 0x1A;
                    goto done;
                case 4:
                    st->msg = 0x1C;
                    goto done;
                case 3:
                    goto read;
                default:
                    st->msg = 0x25;
                    goto done;
                }
            } else if (st->cardsFound == 2) {
                st->slotUsed = 1;
                pad = st->card[1];
                switch (pad) {
                case 2:
                case 1:
                    NetMode_OpenYesNo(0);
                    st->msg = 8;
                    goto done;
                case 0:
                    st->msg = 0x1A;
                    goto done;
                case 4:
                    st->msg = 0x1C;
                    goto done;
                case 3:
                    goto read;
                default:
                    st->msg = 0x25;
                    goto done;
                }
            } else if (st->slotUsed == 0) {
                st->slotUsed = 1;
                CardTask_StartFind(st->slotUsed, gLoadBuf, 1);
                break;
            }
            if (st->card[0] == 3 && st->card[1] == 3) {
                st->slotUsed = 0;
            read:
                st->slot = st->slotUsed;
                CardTask_StartRead(st->slotUsed, gLoadBuf);
                st->subStep++;
                break;
            }
            if (st->card[0] == 3) {
                st->slotUsed = 0;
                goto read;
            }
            if (st->card[1] == 3) {
                st->slotUsed = 1;
                goto read;
            }
            if (st->card[0] == 0 && st->card[1] == 0) {
                st->msg = 5;
                st->slotUsed = 0;
            done:
                st->slot = st->slotUsed;
                st->step = 0xA;
                st->subStep = 0;
                break;
            }
            if (st->card[0] == 4 && st->card[1] == 4) {
                st->slotUsed = 0;
                st->msg = 0x1B;
                goto done;
            }
            st->msg = 0x1C;
            if (st->card[0] == 4) {
                st->slotUsed = 0;
                goto done;
            }
            if (st->card[1] == 4) {
                st->slotUsed = 1;
                goto done;
            }
            if (st->card[0] == 5 && st->card[1] == 5) {
                st->slotUsed = 0;
                st->msg = 0x24;
                goto done;
            }
            st->msg = 0x25;
            if (st->card[0] == 5) {
                st->slotUsed = 0;
                goto done;
            }
            if (st->card[1] == 5) {
                st->slotUsed = 1;
                goto done;
            }
            NetMode_OpenYesNo(0);
            st->yesNoOpen = 1;
            if (st->card[0] == 2 && st->card[1] == 2) {
                st->slotUsed = 0;
                st->msg = 7;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (st->card[0] == 2 && st->card[1] == 1) {
                st->slotUsed = 0;
                st->msg = 7;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (st->card[0] == 1 && st->card[1] == 2) {
                st->slotUsed = 0;
                st->msg = 7;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (st->card[0] == 1 && st->card[1] == 1) {
                st->slotUsed = 0;
                st->msg = 7;
                NetMode_OpenYesNo(0);
                goto done;
            }
            st->msg = 8;
            if (st->card[0] == 2) {
                st->slotUsed = 0;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (st->card[1] == 2) {
                st->slotUsed = 1;
                NetMode_OpenYesNo(0);
                goto done;
            }
            st->msg = 8;
            if (st->card[0] == 1) {
                st->slotUsed = 0;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (st->card[1] == 1) {
                st->slotUsed = 1;
                goto done;
            }
            break;
        case 2:
            NetModeMsg_Page1(0xA);
            CardTask_Update();
            switch (CardTask_Result()) {
            case -1:
                break;
            case 0:
                if (LobbyCard_CheckNetValues(gLoadBuf) != 0) {
                    st->card[st->slotUsed] = 4;
                    st->msg = 0x1C;
                    goto done;
                }
                memset(&gNetSaveData, 0, sizeof(NetSaveData));
                memcpy(&gNetSaveData, gLoadBuf, sizeof(NetSaveData));
                st->slot = st->slotUsed;
                st->msg = 0x28;
                st->percent = 0x78;
                st->timer = 0x2D;
                st->step = 0x1E;
                st->subStep = 0;
                LoginSe_Decide();
                break;
            case -254:
            case -253:
                st->msg = 8;
                st->card[st->slotUsed] = 2;
                NetMode_OpenYesNo(0);
            fail:
                LoginSe_Play9();
                st->slot = st->slotUsed;
                st->step = 0xA;
                st->subStep = 0;
                break;
            case -252:
                st->msg = 8;
                st->card[st->slotUsed] = 1;
                NetMode_OpenYesNo(0);
                goto fail;
            case -255:
                st->msg = 0x1A;
                st->card[st->slotUsed] = 0;
                goto done;
            default:
                st->msg = 0xB;
                st->card[st->slotUsed] = 2;
                goto done;
            }
            break;
        }
        break;
    case 2:
        switch (st->subStep) {
        case 0:
            st->subStep++;
            st->sel3 = 0;
            st->sel2 = 0;
            st->msg = 0;
            st->slot = 0;
            NetMode_PickSlot();
            break;
        case 1:
            NetModeMsg_Page1(9);
            if ((pad & 0x1000) && st->sel2 != 1) {
                LoginMenu_PlayCursorSe();
                st->sel2 = 1;
            } else if ((pad & 0x2000) && st->sel2 != 0) {
                LoginMenu_PlayCursorSe();
                st->sel2 = 0;
            } else if (LoginPad_Decide(1) != 0) {
                NetMode_CloseYesNoBox();
                st->subStep++;
                st->msg = 0;
                st->slotUsed = st->sel2;
                CardTask_StartFind(st->slotUsed, gLoadBuf, 0);
            } else if (LoginPad_Cancel() != 0) {
                NetMode_CloseYesNoBox();
                st->slot = st->slotUsed;
                st->step = 0xA;
                st->subStep = 0;
                NetMode_OpenYesNo(1);
                st->msg = 3;
            }
            break;
        case 2:
            NetModeMsg_Page1(0x2F);
            CardTask_Update();
            switch (CardTask_Result()) {
            case -1:
                break;
            case -254:
                CardTask_StartPoll();
                st->slot = st->slotUsed;
                st->msg = 0x10;
                st->step = 3;
                st->subStep = 0;
                NetMode_OpenYesNo(1);
                LoginSe_Play9();
                break;
            case 0:
                CardTask_StartPoll();
                st->slot = st->slotUsed;
                st->msg = 0x30;
                st->step = 5;
                st->subStep = 0;
                NetMode_OpenYesNo(1);
                LoginSe_Play9();
                break;
            case -253:
                st->card[st->slotUsed] = 2;
                memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
                CardTask_StartSave(st->slotUsed, gLoadBuf);
                st->subStep++;
                break;
            case -252:
                st->msg = 0xF;
                st->card[st->slotUsed] = 1;
            retry:
                LoginSe_Play9();
                st->slot = st->slotUsed;
                st->step = 0xF;
                st->subStep = 0;
                break;
            case -255:
                st->msg = 0x1A;
                st->card[st->slotUsed] = 0;
                goto retry;
            default:
                st->msg = 0xE;
                st->card[st->slotUsed] = 2;
                goto retry;
            }
            break;
        case 3:
            NetModeMsg_Page1(0xC);
            CardTask_Update();
            switch (CardTask_Result()) {
            case -1:
                break;
            case 0:
                st->slot = st->slotUsed;
                st->msg = 0xD;
                st->step = 0x23;
                st->subStep = 0;
                LoginSe_Decide();
                break;
            case -254:
                CardTask_StartPoll();
                st->slot = st->slotUsed;
                st->msg = 0x10;
                st->step = 3;
                st->subStep = 0;
                NetMode_OpenYesNo(1);
                LoginSe_Play9();
                break;
            case -253:
                st->msg = 0xE;
                st->card[st->slotUsed] = 2;
                goto retry;
            case -252:
                st->msg = 0xF;
                st->card[st->slotUsed] = 1;
                NetMode_OpenYesNo(0);
                goto retry;
            case -255:
                st->msg = 0x1A;
                st->card[st->slotUsed] = 0;
                goto retry;
            default:
                st->msg = 0xE;
                st->card[st->slotUsed] = 2;
                goto retry;
            }
            break;
        }
        break;
    case 3:
        switch (st->subStep) {
        case 0:
            NetModeMsg_Page1(st->msg);
            CardTask_Update();
            if (Card_PortState(D_5D4705) == 0) {
                NetMode_CloseYesNoBox();
                st->msg = 6;
                st->step = 0xA;
                st->subStep = 0;
                LoginSe_Play9();
                break;
            }
            switch (NetMode_ChooseYesNo()) {
            case 0:
                break;
            case 1:
                NetMode_CloseYesNoBox();
                st->subStep++;
                CardTask_StartFormat(D_5D4704);
                break;
            case -1:
                st->msg = 3;
                st->step = 0xA;
                st->subStep = 0;
                NetMode_OpenYesNo(1);
                break;
            }
            break;
        case 1:
            NetModeMsg_Page1(0x11);
            CardTask_Update();
            switch (CardTask_Result()) {
            case -1:
                break;
            case 0:
                memset(&gNetSaveData, 0, sizeof(NetSaveData));
                memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
                st->card[st->slotUsed] = 2;
                CardTask_StartSave(st->slotUsed, gLoadBuf);
                st->step = 2;
                st->subStep = 3;
                break;
            case -256:
            default:
                st->msg = 0x12;
                st->step = 0xA;
                st->subStep = 0;
                LoginSe_Play9();
                break;
            }
            break;
        }
        break;
    case 5:
        NetModeMsg_Page1(st->msg);
        CardTask_Update();
        if (Card_PortState(D_5D4705) == 0) {
            NetMode_CloseYesNoBox();
            st->msg = 6;
            st->step = 0xA;
            st->subStep = 0;
            LoginSe_Play9();
            break;
        }
        switch (NetMode_ChooseYesNo()) {
        case 0:
            break;
        case 1:
            st->card[st->slotUsed] = 2;
            memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
            CardTask_StartSave(st->slotUsed, gLoadBuf);
            NetMode_CloseYesNoBox();
            st->step = 2;
            st->subStep = 3;
            break;
        case -1:
            st->msg = 3;
            st->step = 0xA;
            st->subStep = 0;
            NetMode_OpenYesNo(1);
            break;
        }
        break;
    case 10:
        NetModeMsg_Page1(st->msg);
        if (st->yesNoOpen == 0 && LoginPad_Decide(1) != 0) {
            NetMode_OpenYesNo(1);
            st->msg = 3;
        } else if (st->yesNoOpen != 0) {
            switch (NetMode_ChooseYesNo()) {
            case 0:
                break;
            case 1:
                switch (st->msg) {
                case 3:
                    st->step = 0x14;
                    NetMode_CloseYesNoBox();
                    NetMode_SeqAStart(0, 0x1E, 1);
                    break;
                default:
                    memset(&gNetSaveData, 0, sizeof(NetSaveData));
                    st->step = 2;
                    st->subStep = 0;
                    NetMode_CloseYesNoBox();
                    if (st->msg == 8) {
                        st->subStep = 2;
                        st->slotUsed = st->slot;
                        CardTask_StartFind(st->slotUsed, gLoadBuf, 0);
                    }
                    st->msg = 0;
                    break;
                }
                break;
            case -1:
                switch (st->msg) {
                case 3:
                    goto again;
                default:
                    NetMode_OpenYesNo(1);
                    st->msg = 3;
                    break;
                }
                break;
            }
        }
        break;
    case 15:
        NetModeMsg_Page1(st->msg);
        if (st->yesNoOpen == 0 && LoginPad_Decide(1) != 0) {
            NetMode_OpenYesNo(1);
            st->msg = 3;
        } else if (st->yesNoOpen != 0) {
            switch (NetMode_ChooseYesNo()) {
            case 0:
                break;
            case 1:
                switch (st->msg) {
                case 3:
                    st->step = 0x14;
                    NetMode_CloseYesNoBox();
                    NetMode_SeqAStart(0, 0x1E, 1);
                    break;
                default:
                    goto again;
                }
                break;
            case -1:
                switch (st->msg) {
                case 3:
                again:
                    st->step = 4;
                    st->subStep = 0;
                    st->sel3 = 0;
                    st->sel2 = st->slot;
                    st->msg = 0;
                    NetMode_CloseYesNoBox();
                    break;
                default:
                    NetMode_OpenYesNo(1);
                    st->msg = 3;
                    break;
                }
                break;
            }
        }
        break;
    case 4:
        switch (st->subStep) {
        case 0:
            st->subStep++;
            st->sel3 = 0;
            st->msg = 0;
            NetMode_PickSlot();
            st->slotUsed = st->slot;
            break;
        case 1:
            NetModeMsg_Page1(0x23);
            if ((pad & 0x1000) && st->sel2 != 1) {
                LoginMenu_PlayCursorSe();
                st->sel2 = 1;
            } else if ((pad & 0x2000) && st->sel2 != 0) {
                LoginMenu_PlayCursorSe();
                st->sel2 = 0;
            } else if (LoginPad_Decide(1) != 0) {
                NetMode_CloseYesNoBox();
                st->subStep++;
                st->msg = 0;
                st->slotUsed = st->sel2;
                st->slot = st->slotUsed;
                CardTask_StartFind(st->slotUsed, gLoadBuf, 0);
            } else if (LoginPad_Cancel() != 0) {
                NetMode_CloseYesNoBox();
                st->slot = 0;
                st->step = 0xA;
                st->subStep = 0;
                NetMode_OpenYesNo(1);
                st->msg = 3;
            }
            break;
        case 2:
            NetModeMsg_Page1(0x2F);
            CardTask_Update();
            switch (CardTask_Result()) {
            case -1:
                break;
            case -255:
                st->msg = 0x1A;
                st->card[st->slotUsed] = 0;
                LoginSe_Play9();
                st->slot = st->slotUsed;
                st->step = 0xF;
                st->subStep = 0;
                break;
            case 0:
                CardTask_StartRead(st->slotUsed, gLoadBuf);
                st->subStep++;
                break;
            case -254:
            case -253:
            case -252:
                st->msg = 8;
                st->card[st->slotUsed] = 2;
                st->step = 0xA;
                st->subStep = 0;
                NetMode_OpenYesNo(0);
                break;
            default:
                st->msg = 0xB;
                st->card[st->slotUsed] = 2;
                goto done;
            }
            break;
        case 3:
            NetModeMsg_Page1(0xA);
            CardTask_Update();
            switch (CardTask_Result()) {
            case -1:
                break;
            case 0:
                if (LobbyCard_CheckNetValues(gLoadBuf) != 0) {
                    st->card[st->slotUsed] = 4;
                    st->msg = 0x1C;
                    goto done;
                }
                memset(&gNetSaveData, 0, sizeof(NetSaveData));
                memcpy(&gNetSaveData, gLoadBuf, sizeof(NetSaveData));
                st->slot = st->slotUsed;
                st->msg = 0x28;
                st->percent = 0x78;
                st->timer = 0x2D;
                st->step = 0x1E;
                st->subStep = 0;
                LoginSe_Decide();
                break;
            case -254:
            case -253:
                st->msg = 8;
                st->card[st->slotUsed] = 2;
                NetMode_OpenYesNo(0);
                goto fail;
            case -252:
                st->msg = 8;
                st->card[st->slotUsed] = 1;
                NetMode_OpenYesNo(0);
                goto fail;
            case -255:
                st->msg = 0x1A;
                st->card[st->slotUsed] = 0;
                goto done;
            default:
                st->msg = 0xB;
                st->card[st->slotUsed] = 2;
                goto done;
            }
            break;
        }
        break;
    case 20:
        if (NetMode_SeqABusy() == 0) {
            NetMode_CloseAll();
            r = -1;
        }
        break;
    case 30:
        NetModeMsg_Page1(st->msg);
        if (--st->percent == 0) {
            LoginMenu_PlayDecideSe();
            goto ok;
        }
        if (st->timer != 0) {
            st->timer--;
        }
        if (st->yesNoOpen == 0 && LoginPad_Decide(1) != 0) {
        ok:
            r = 1;
        }
        break;
    case 35:
        NetModeMsg_Page1(st->msg);
        if (st->yesNoOpen == 0 && LoginPad_Decide(1) != 0) {
            goto ok;
        }
        break;
    }
    return r;
}

void LoadOverlay(s32 id);
void func_AC9B90(s32 on);
s32 func_AC9C20(void);
void func_ACA480(void);

/* the network connection's steps (NetModeState.unk1): loads overlay 9,
 * connects (func_AC9B90/func_AC9C20), then shows the result message picked
 * from the two line states unk7C and unk7D; 1 when done, -1 on failure, 0
 * while running */
s32 NetSetup_Connect(void)
{
    NetModeState *st = &gNetMode;
    s32 r = 0;
    s32 a;
    s32 b;

    switch (gNetMode.step) {
    case 0:
        st->savedSlot = st->slotUsed;
        st->step++;
        LoadOverlay(9);
        if (st->ynNoDraw == 0) {
            NetMode_SetTitle(2);
        } else {
            st->error[1] = st->error[0] = 0x7F;
        }
        NetMode_SeqAReset();
        gScene.inLobby = 0;
        break;
    case 1:
        st->step++;
        func_AC9B90(st->ynNoDraw);
        break;
    case 2:
        switch (func_AC9C20()) {
        case 0:
            if (st->ynNoDraw == 0) {
                break;
            }
            NetModeMsg_Page1(0x1D);
            a = st->error[0];
            if (a == 0x7F || (a != 0 && st->error[1] == 0x7F)) {
                break;
            }
            if (a == 0 || st->error[1] == 0) {
                LoginSe_Decide();
                st->step = 0xA;
                st->subStep = 0;
                if (st->error[0] == 0) {
                    st->slotUsed = 0;
                } else {
                    st->slotUsed = 1;
                }
                st->msg = 0x2A;
            } else {
                LoginSe_Play9();
                st->step = 0xB;
                st->timer = 0;
                a = st->error[0];
                if (a == 0x20 && st->error[1] == 0x20) {
                    st->msg = 0x20;
                } else if (a == 0x2D && st->error[1] == 0x2D) {
                    st->msg = 0x2E;
                } else if (a == 0x1F && st->error[1] == 0x1F) {
                    st->msg = 0x2C;
                } else if (a == 0x1E && st->error[1] == 0x1E) {
                    st->msg = 0x2B;
                } else if (a == 0x21 || (b = st->error[1]) == 0x21) {
                    if (a == 0x21) {
                        st->slotUsed = 0;
                    } else {
                        st->slotUsed = 1;
                    }
                    st->msg = 0x21;
                } else if (a == 0x22 || b == 0x22) {
                    st->msg = 0x22;
                } else {
                    switch (a) {
                    case 0x20:
                        switch (b) {
                        case 0x2D:
                            st->msg = 0x31;
                            break;
                        case 0x1E:
                            st->msg = 0x32;
                            break;
                        case 0x1F:
                            st->msg = 0x37;
                            break;
                        default:
                            st->slotUsed = 0;
                            st->msg = 0x20;
                            break;
                        }
                        break;
                    case 0x2D:
                        switch (b) {
                        case 0x20:
                            st->msg = 0x33;
                            break;
                        case 0x1E:
                            st->msg = 0x34;
                            break;
                        case 0x1F:
                            st->msg = 0x38;
                            break;
                        default:
                            st->slotUsed = 0;
                            st->msg = 0x1F;
                            break;
                        }
                        break;
                    case 0x1F:
                        switch (b) {
                        case 0x20:
                            st->msg = 0x39;
                            break;
                        case 0x1E:
                            st->msg = 0x3A;
                            break;
                        case 0x2D:
                            st->msg = 0x3B;
                            break;
                        default:
                            st->slotUsed = 0;
                            st->msg = 0x1F;
                            break;
                        }
                        break;
                    case 0x1E:
                        switch (b) {
                        case 0x20:
                            st->msg = 0x35;
                            break;
                        case 0x2D:
                            st->msg = 0x36;
                            break;
                        case 0x1F:
                            st->msg = 0x3C;
                            break;
                        default:
                            st->slotUsed = 0;
                            st->msg = 0x1F;
                            break;
                        }
                        break;
                    default:
                        st->slotUsed = 0;
                        st->msg = 0x1F;
                        break;
                    }
                }
            }
            func_ACA480();
            break;
        case 2:
        case 1:
            st->step = 3;
            break;
        case -1:
            st->step = 5;
            break;
        }
        break;
    case 3:
        st->step++;
        gScene.inLobby = 1;
        break;
    case 4:
    done:
        st->slotUsed = st->savedSlot;
        NetMode_SeqAReset();
        r = 1;
        break;
    case 5:
        st->step++;
        gScene.inLobby = 1;
        break;
    case 6:
    failed:
        st->slotUsed = st->savedSlot;
        NetMode_SeqAReset();
        r = -1;
        break;
    case 10:
        if (++st->subStep < 60) {
            NetModeMsg_Page1(st->msg);
        } else {
            if (st->ynResult == 2) {
                st->ynResult = 1;
            } else {
                st->ynResult = 0;
            }
            goto done;
        }
        break;
    case 11:
        NetModeMsg_Page1(st->msg);
        if (LoginPad_Decide(1) != 0) {
            st->msg = 0;
            goto failed;
        }
        break;
    }
    return r;
}

void NetSetup_ResetLobby(void) {
    gLobby = &gLobbyState;
    memset(&D_008FAAA0, 0, 0x1A);
    Lobby_BackupWork();
}

s32 NetSetup_Login(void)
{
    NetModeState *st;
    s32 k;
    s32 r;

    st = &gNetMode;
    r = 0;
    switch (st->step) {
    case 0:
        st->noDraw = 0;
        NetMode_SetTitle(3);
        NetMode_OpenProgress(0);
        NetMode_OpenPercent();
        NetMode_OpenIcon2(3);
        st->step++;
        st->subStep = 0;
        st->percent = 0;
        NetSetup_ResetLobby();
        Net_ClearState();
        ServerList_ClearTried();
        break;
    case 1:
        if (LoginPad_Any8000() != 0) {
            D_59D666 += 2;
        } else {
            D_59D666 = 0;
        }
        k = NetMsg_Update();
        st->percent = gNetProgress;
        switch (k) {
        case 2:
            st->step = 2;
            st->subStep = 8;
            break;
        case 4:
            NetMode_CloseBoxes(0x40);
            st->step = 4;
            st->subStep = 8;
            break;
        }
        break;
    case 2:
        if (st->subStep == 0) {
            st->step++;
            NetMode_CloseBoxes(0x40);
            NetMode_CloseBoxes(0x80);
            NetMode_CloseBoxes(0x40000);
        } else {
            st->subStep--;
        }
        break;
    case 3:
        r = 1;
        break;
    case 4:
        if (st->subStep == 0) {
            st->step++;
            NetMode_CloseBoxes(0x40);
            NetMode_OpenIcon2(4);
        } else {
            st->subStep--;
        }
        break;
    case 5:
        if (NetSetup_ShowLoginError() != 0) {
            NetMode_CloseBoxes(0x80);
            NetMode_CloseBoxes(0x40000);
            r = -1;
        }
        break;
    }
    return r;
}

INCLUDE_RODATA("asm/lobby/nonmatchings/unk_7C8FA0", D_00847100);

/* the KDDI portal (the HTML browser, Browser_Init): opens the page
 * 0x6ADFD7 selects (1: sub_main's URL at 0x5CAEEC, 2 or 3: the one at
 * 0x5CAFED, else the portal's top page D_00847100), then waits for the
 * browser; returns 1 when it ended well, -1 when it failed */
s32 LobbyKddi_Portal(void)
{
    NetModeState *st = &gNetMode;
    s32 r;

    r = 0;
    switch (st->step) {
    case 0:
        st->step++;
        st->msg = 0;
        st->noDraw = 3;
        NetMode_SeqAReset();
        LoginWave_Clear();
        break;
    case 1:
        memset(gBrowserUrl, 0, 0x100);
        switch (gPortalPage) {
        case 1:
            strcpy(gBrowserUrl, gPortalUrlA);
            gBrowserTopMode = 0;
            break;
        case 2:
        case 3:
            gBrowserTopMode = 1;
            strcpy(gBrowserUrl, gPortalUrlB);
            break;
        default:
            gPortalPage = PORTAL_PAGE_TOP;
            gBrowserTopMode = 0;
            strcpy(gBrowserUrl, D_00847100);
            break;
        }
open:
        Browser_Init(0);
        st->step++;
        break;
    case 2:
        switch (Browser_Update()) {
        case 0:
            break;
        case 1:
            if (gPortalPage == PORTAL_PAGE_NEXT || (gPortalPage != PORTAL_PAGE_NEXT && gBrowserState->page == PAGE_LOAD_NONE)) {
                LobbyKddi_CloseBrowser();
                r = -1;
                gPortalPage = PORTAL_PAGE_TOP;
            } else {
                st->step++;
                gPortalPage = PORTAL_PAGE_BACK;
            }
            break;
        case 2:
            LobbyKddi_CloseBrowser();
            st->step = 4;
            break;
        case -1:
            LobbyKddi_CloseBrowser();
            r = -1;
            gPortalPage = PORTAL_PAGE_TOP;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            break;
        case -100:
            LobbyKddi_CloseBrowser();
            st->step = 4;
            break;
        }
        break;
    case 3:
        LobbyKddi_CloseBrowser();
        st->bgmOn = 0;
        Sound_Silence();
        r = 1;
        break;
    case 4:
        memset(gBrowserUrl, 0, 0x100);
        strcpy(gBrowserUrl, D_00847100);
        gPortalPage = PORTAL_PAGE_TOP;
        gBrowserTopMode = 0;
        st->step = 1;
        goto open;
    }
    return r;
}

s32 NetSetup_Mode5(void)
{
    s32 ret;
    s8 res;

    res = gNetSaveData.setupResult;
    if (res == 1) {
        gNetMode.unk10 = 1;
    }
    ret = (s8)(res);
    gNetMode.ynNoDraw = 1;
    if (ret == 0) {
        ret = -1;
    }
    return ret;
}

extern void *gBones;
extern void *gModels;
extern void *gPrims;
void NetMode_ClearBuffers(void) {
    memset((void *)gObjBufBlocks, 0, 0x20000);
    memset((void *)gMatSets, 0, 0xD200);
    memset((void *)gMaterials, 0, 0x14000);
    memset(gBones, 0, 0xC8000);
    memset(gModels, 0, 0x4000);
    memset(gPrims, 0, 0x5000);
    memset(gSysSettings.extraBuf, 0, 0x14000);
}

s32 NetSetup_Leave(void) {
    NetMode_SetTitle(0);
    gNetMode.bgmOn = 0;
    Sound_Silence();
    NetMode_ClearBuffers();
    return 1;
}

s32 NetSetup_Disconnect(void)
{
    NetModeState *st;
    s32 r;

    st = &gNetMode;
    r = 0;
    switch (st->step) {
    case 0:
        NetMode_SeqAStart(0, 0x14, 1);
        st->step++;
        break;
    case 1:
        if ((s8)NetMode_SeqABusy() == 0) {
            NetMode_SetTitle(0);
            func_007CF620();
            st->step++;
            gNetDisconnectStep = 0;
            gNetStepTimeout = 0;
        }
        break;
    case 2:
        if (LobbyNet_DisconnectWait() == 0) {
            return 0;
        }
        st->step++;
    case 3:
        r = 1;
        break;
    }
    return r;
}

s32 NetSetup_Quit(void) {
    NetMode_SetTitle(0);
    return 1;
}

void NetSetup_ToSettings(void)
{
    NetMode_ReleaseTextures(0);
    NetMode_SetMode(9);
    gNetMode.noDraw = 0;
    gNetMode.subStep = 1;
    gNetMode.entry = 0;
    gNetMode.ynResult = 0;
    gNetMode.netResult = 0;
    gNetMode.unk10 = 0;
    gNetMode.ynNoDraw = 0;
    gNetMode.pad17 = 0;
    gPortalPage = PORTAL_PAGE_TOP;
}

s32 NetSetup_Mode2(void) {
    NetMode_SetTitle(3);
    if (gNetSaveData.setupResult == 1) {
        return 1;
    }
    return -1;
}

/* the font column of a character code (a table of codes, then the columns) */
u8 LoginFont_GetColumn(s32 code)
{
    s16 *p;
    s16 v;
    s32 i;

    i = 0;
    p = gLoginFontCodes;
    v = *p;
    while (v < 0) {
        if (v == (s16)code) {
            break;
        }
        p++;
        v = *p;
        i++;
    }
    if (gLoginFontColumns[i] == 0xFF) {
        i++;
    }
    if (code == 0x7F) {
        i++;
    }
    return gLoginFontColumns[i];
}

void NetMode_ResetAnim(void) {
    gNetMode.padD9[1] = 0;
    gNetMode.padD9[2] = 0;
}

/* steps two animation counters: 0x5D477A (0-3), then 0x5D477B (0-2) */
/* Raw addresses kept here: the named form changes the code. */
static void NetMode_StepAnim(void)
{
    s8 *st;

    st = (s8 *)&gNetMode;
    (*(s8 *)&gNetMode.padD9[1]) = ((*(s8 *)&gNetMode.padD9[1]) + 1) & 3;
    if ((*(s8 *)&gNetMode.padD9[1]) == 0) {
        st[0xDB]++;
        st[0xDB] %= 3;
    }
}

s32 LoginPad_Triggered(void);
s32 func_ACED20(void);
s32 NetSetup_DeleteAccount(s32 mode);
s32 NetMode_ChooseYesNo(void);
void func_ACE630(void);
s32 LoginPad_DecideEx(s32 arg0, s32 arg1);
s32 LoginPad_Cancel(void);

/* the network settings menu's steps (NetModeState.unk1): the choice of
 * three (sel3: connect, the ID and password, back) with the ID and
 * password rows skipped while unset, the choice of two (sel2), the
 * connection, the ID entry and the error box; 1 to connect, 2 or 3 on the
 * other exits, -1 on cancel, 0 while running. The func_ACED20 test is a
 * default-first switch: MWCC then fills no branch slot from the fall-through
 * in the whole function. */
s32 NetSetup_SettingsMenu(void)
{
    NetModeState *st = &gNetMode;
    s32 pad = LoginPad_Triggered();
    s32 old;

    switch (gNetMode.step) {
    case 0:
        LoadOverlay(9);
        st->sel3 = 0;
        st->sel2 = 0;
        st->openW = 0;
        st->openH = 0;
        st->step++;
        if (st->subStep == 0xFF) {
            NetMode_SetTitle(6);
            st->subStep = 0;
            if (gNetSaveData.setupResult == 1) {
                st->sel3 = 1;
            } else {
                st->sel3 = 0;
            }
        }
        if (st->subStep == 1) {
            NetMode_ReloadFrame();
            st->subStep = 0;
        }
        if (st->subStep == 2) {
            NetMode_SetTitle(7);
            st->sel3 = 1;
            if (gNetSaveData.setupResult == 1) {
                st->sel3 = 1;
            } else {
                st->sel3 = 0;
            }
            st->sel2 = 0;
            st->step = 2;
            st->subStep = 0;
        }
        if (gNetSaveData.setupResult == 1) {
            st->innerStep = 1;
        }
        st->noDraw = 0;
        NetMode_OpenIcon5();
        NetMode_SeqAReset();
        NetMode_ResetAnim();
        break;
    case 1:
        NetMode_StepAnim();
        if ((pad & 0x2000) || (pad & 0x1000)) {
            old = st->sel3;
            if ((pad & 0x2000) && old > 0) {
                st->sel3 = old - 1;
            }
            if ((pad & 0x1000) && st->sel3 < 3) {
                st->sel3++;
            }
            if (gNetSaveData.setupResult == 0 && st->sel3 == 1) {
                if (pad & 0x2000) {
                    st->sel3 = 0;
                    LoginMenu_PlayCursorSe();
                    goto select;
                }
                if (gNetSaveData.mmbbLoginId[0] == 0) {
                    st->sel3 = 3;
                } else {
                    st->sel3 = 2;
                }
                LoginMenu_PlayCursorSe();
                goto draw;
            }
            if (gNetSaveData.mmbbLoginId[0] == 0 && st->sel3 == 2) {
                if (pad & 0x2000) {
                    if (gNetSaveData.setupResult == 0) {
                        st->sel3 = 0;
                    } else {
                        st->sel3 = 1;
                    }
                } else {
                    st->sel3 = 3;
                }
            }
            if (old != st->sel3) {
                LoginMenu_PlayCursorSe();
            }
            goto draw;
        }
    select:
        if (st->sel3 == 2) {
            old = LoginPad_DecideEx(1, 1);
        } else {
            old = LoginPad_DecideEx(1, 0);
        }
        if (old != 0) {
            switch (st->sel3) {
            case 1:
                NetMode_SetTitle(6);
                NetMode_CloseBoxes(0x20);
                NetSetup_Mode5();
                return 1;
            case 0:
                NetMode_SetTitle(7);
                st->sel2 = 0;
                st->step = 2;
                NetMode_ResetAnim();
                break;
            case 2:
                if (gNetSaveData.mmbbLoginId[0] != 0) {
                    NetMode_CloseBoxes(0x20);
                    st->step = 4;
                    st->subStep = 0;
                }
                break;
            case 3:
            cancel:
                NetMode_CloseAll();
                return -1;
            }
        }
        if (LoginPad_Cancel() != 0) {
            if (st->sel3 == 3) {
                goto cancel;
            }
            st->sel3 = 3;
        }
    draw:
        NetModeMsg_Queue5(0, 0x15);
        break;
    case 2:
        NetMode_StepAnim();
        if ((pad & 0x1000) && st->sel2 < 3) {
            LoginMenu_PlayCursorSe();
            st->sel2++;
        } else if ((pad & 0x2000) && st->sel2 > 0) {
            LoginMenu_PlayCursorSe();
            st->sel2--;
        } else {
            if (LoginPad_Decide(1) != 0) {
                switch (st->sel2) {
                case 0:
                    NetMode_SetTitle(2);
                    NetSetup_Mode5();
                    gNetMode.ynNoDraw = 0;
                    return 2;
                case 1:
                    st->step = 5;
                    st->subStep = 0;
                    NetMode_CloseBoxes(0x20);
                    NetMode_OpenYesNoAtNo();
                    goto draw2;
                case 2:
                    st->step = 3;
                    st->subStep = 0;
                    NetMode_SetTitle(4);
                    goto draw2;
                case 3:
                back:
                    NetMode_ResetAnim();
                    NetMode_SetTitle(6);
                    st->step = 1;
                    return 0;
                }
            }
            if (LoginPad_Cancel() != 0) {
                if (st->sel2 == 3) {
                    goto back;
                }
                st->sel2 = 3;
            }
        }
    draw2:
        NetModeMsg_Queue5(0, 0x16);
        break;
    case 3:
        switch (st->subStep) {
        case 0:
            st->subStep++;
            func_AC9B90(0);
            break;
        case 1:
            switch (func_ACED20()) {
            default:
                st->step = 2;
                st->subStep = 0;
                NetMode_SetTitle(7);
                NetMode_OpenIcon5();
                break;
            case 0:
                break;
            }
            break;
        }
        break;
    case 4:
        switch (NetSetup_DeleteAccount(0)) {
        case 0:
            break;
        case 1:
        case 2:
            NetMode_SetTitle(6);
            NetMode_OpenIcon5();
            st->step = 1;
            st->subStep = 0;
            if (gNetSaveData.mmbbLoginId[0] == 0) {
                if (gNetSaveData.setupResult == 0) {
                    st->sel3 = 0;
                } else {
                    st->sel3 = 1;
                }
            }
            break;
        case -1:
            st->subStep = 0;
            break;
        }
        break;
    case 5:
        NetModeMsg_Page1(0x29);
        switch (st->subStep) {
        case 0:
            switch (NetMode_ChooseYesNo()) {
            case 0:
                break;
            case 1:
                FadeA_Start(1);
                st->subStep = 1;
                st->innerStep = 0x14;
                break;
            case -1:
                NetMode_CloseYesNo();
                st->step = 2;
                st->subStep = 0;
                NetMode_OpenIcon5();
                break;
            }
            break;
        case 1:
            if (--st->innerStep <= 0) {
                Sound_Silence();
                st->subStep = 2;
            }
            break;
        case 2:
            NetMode_CloseYesNo();
            func_ACE630();
            return 3;
        }
        break;
    }
    return 0;
}

void NetModeMsg_Page4(s32 a0)
{
    NetModeMsg_Queue(0x1, 0x4, a0);
}

s32 NetSetup_ShowLoginError(void)
{
    s32 msg;
    NetModeState *st;
    s32 r;

    st = &gNetMode;
    r = 0;
    switch (st->subStep) {
    case 0:
        st->subStep++;
    case 1:
        msg = 0;
        switch (gNetMsgState.err) {
        case -2:
            msg = 3;
            break;
        case -3:
            msg = 4;
            break;
        case -5:
            msg = 5;
            break;
        case -81:
            msg = 1;
            break;
        case -82:
            msg = 2;
            break;
        case -83:
            msg = 6;
            break;
        case -84:
            msg = 7;
            break;
        case -85:
            msg = 8;
            break;
        case -86:
            msg = 0xC;
            break;
        case -87:
            msg = 0x10;
            break;
        case -88:
            msg = 0xF;
            break;
        case -89:
            msg = 0x11;
            break;
        }
        if (msg != 0) {
            if (msg == 0xC) {
                st->percent = 0;
            }
            NetModeMsg_Page4(msg);
            NetModeMsg_Page0(0x14);
        }
        if (LoginPad_Decide(2) != 0) {
            r = 1;
            st->subStep++;
        }
        break;
    }
    return r;
}

/* sounds 0xA3 and 2 off, the network mode's textures released (the handles
 * cleared, then the memory compacted) */
void LobbyLogin_FreeTextures(void)
{
    s32 i;

    Tex_FreeRange(0xA3, 0xE);
    Tex_FreeRange(2, 0xA);
    for (i = 0; i < 16; i++) {
        gNetMode.tex[i] = 0;
    }
    Mem_Compact();
}

void NetMode_ResetStep(void) {
    gNetMode.step = 0;
}

/* the KDDI portal from the network mode's scene: as LobbyKddi_Portal, with
 * the scene's own set-up (NetMode_LoadTextures, NetMode_BgmOn) and, once the
 * browser ended well, the textures released and the scene's end */
s32 LobbyKddi_PortalScene(void)
{
    NetModeState *st = &gNetMode;
    s32 r;

    r = 0;
    switch (st->step) {
    case 0:
        st->step++;
        NetMode_LoadTextures(0);
        LoginWave_Clear();
        st->msg = 0;
        st->noDraw = 3;
        NetMode_SeqAReset();
        if (gPortalPage == PORTAL_PAGE_TOP) {
            NetMode_BgmOn();
        }
        break;
    case 1:
        memset(gBrowserUrl, 0, 0x100);
        switch (gPortalPage) {
        case 1:
            strcpy(gBrowserUrl, gPortalUrlA);
            gBrowserTopMode = 0;
            break;
        case 2:
        case 3:
            gBrowserTopMode = 1;
            strcpy(gBrowserUrl, gPortalUrlB);
            break;
        default:
            gPortalPage = PORTAL_PAGE_TOP;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            gBrowserTopMode = 0;
            strcpy(gBrowserUrl, D_00847100);
            break;
        }
open:
        Browser_Init(0);
        st->step++;
        break;
    case 2:
        switch (Browser_Update()) {
        case 0:
            break;
        case 1:
            if (gPortalPage == PORTAL_PAGE_NEXT || (gPortalPage != PORTAL_PAGE_NEXT && gBrowserState->page == PAGE_LOAD_NONE)) {
                LobbyKddi_CloseBrowser();
                r = -1;
                gPortalPage = PORTAL_PAGE_TOP;
            } else {
                st->step++;
                gPortalPage = PORTAL_PAGE_BACK;
            }
            break;
        case 2:
            LobbyKddi_CloseBrowser();
            st->step = 4;
            break;
        case -1:
            LobbyKddi_CloseBrowser();
            r = -1;
            gPortalPage = PORTAL_PAGE_TOP;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            break;
        case -100:
            LobbyKddi_CloseBrowser();
            st->step = 4;
            break;
        }
        break;
    case 3:
        LobbyKddi_CloseBrowser();
        LobbyLogin_FreeTextures();
        Scene_LoadOverlay(0xA);
        NetMode_BgmOff();
        r = 1;
        break;
    case 4:
        NetMode_BgmOn();
        memset(gBrowserUrl, 0, 0x100);
        strcpy(gBrowserUrl, D_00847100);
        gPortalPage = PORTAL_PAGE_TOP;
        gNetEntryMode = LOGIN_ENTRY_FIRST;
        gBrowserTopMode = 0;
        st->step = 1;
        goto open;
    }
    return r;
}

/* draws the login screen's menu frame: the six sprites of gLoginFrameSprites (the
 * cursor's one tinted by the menu page's colour, dimmed while side 1 is
 * marked), then, while side 1 is marked and a choice is lit, the four
 * wobbling sprites around the chosen item's box (gLoginBoxSprites) */
void LobbyLogin_DrawMenu(void)
{
    LoginSpr spr;
    Unk6AE040 *p;
    LoginSpr *tbl;
    u32 *col;
    s32 sel;
    s32 st6C;
    s32 stD;
    s32 st5E;
    s32 i;
    u32 r8;
    u32 g8;
    u32 b8;
    LoginBox *r;
    s16 w;
    s16 h;

    p = *(Unk6AE040 * *)&gSaveBuf;
    NetMode_BindTextures();
    tbl = gLoginFrameSprites;
    if (p->unk54 == -1) {
        sel = -1;
    } else {
        sel = gLoginPageColorIdx[p->unk54];
    }
    st6C = Gs_GetState(0x6C);
    stD = Gs_GetState(0xD);
    st5E = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    Gs_SetState(0xD, 0);
    Gs_SetState(0x5E, 0x32);
    Gfx_SetTexFilter(1);
    LobbyLogin_DrawSprites((struct NetSprDef *)tbl, 1, 0);
    Gfx_SetTexFilter(0);
    for (i = 1; i <= 5; i++) {
        spr = tbl[i];
        if (i == 1) {
            if (p->unk50 == -1) {
                spr.color = 0;
            } else {
                spr.color = LobbySrv_ColorAt(gLobbyMenu->cursor + gLobbyMenu->page * 8) | 0xFF000000;
                if (p->unk10[1] != 0) {
                    r8 = (spr.color & 0xFF0000) >> 16;
                    g8 = (spr.color & 0xFF00) >> 8;
                    b8 = spr.color & 0xFF;
                    r8 = 0.7f * r8;
                    g8 = 0.7f * g8;
                    b8 = 0.7f * b8;
                    spr.color = (spr.color & 0xFF000000) | (r8 << 16) | (g8 << 8) | b8;
                }
            }
        } else if (p->unk10[0] != 0) {
            spr.color = 0xFF000000;
        } else if (i - 2 == sel) {
            spr.color = gLoginLitColors[i - 1];
        } else {
            spr.color = gLoginItemColors[i - 1];
        }
        LobbyLogin_DrawSprites((struct NetSprDef *)&spr, 1, 0);
    }
    Gfx_SetTexFilter(1);
    if (p->unk10[1] != 0) {
        LobbyTitle_Draw();
        Gs_SetState(0x5E, 0x28);
        if (sel != -1) {
            r = &gLoginBoxSprites[sel];
            col = &spr.color;
            spr = tbl[9];
            spr.x = r->x;
            spr.y = r->y;
            w = spr.w + (s16)(8.0f * p->wave[0]);
            h = spr.h + (s16)(8.0f * p->wave[1]);
            spr.x = spr.x + (s32)((f32)(spr.w - w) / 2.0f);
            spr.y = spr.y + (s32)((f32)(spr.h - h) / 2.0f);
            spr.w = w;
            spr.h = h;
            *col = (((u32)(255.0f * p->wave[4]) & 0xFF) << 24) | (*col & 0xFFFFFF);
            LobbyLogin_DrawSprites((struct NetSprDef *)&spr, 1, 0);
            spr = tbl[10];
            spr.x = r->x;
            spr.y = r->y;
            w = spr.w + (s16)(6.0f * p->wave[1]);
            h = spr.h + (s16)(6.0f * p->wave[0]);
            spr.x = spr.x + (s32)((f32)(spr.w - w) / 2.0f);
            spr.y = spr.y + (s32)((f32)(spr.h - h) / 2.0f);
            spr.w = w;
            spr.h = h;
            *col = (((u32)(255.0f * p->wave[5]) & 0xFF) << 24) | (*col & 0xFFFFFF);
            LobbyLogin_DrawSprites((struct NetSprDef *)&spr, 1, 0);
            spr = tbl[7];
            spr.x = r->x + 0x14;
            spr.y = 0;
            spr.h = r->y + r->h / 2;
            *col = (((u32)(255.0f * p->wave[3]) & 0xFF) << 24) | (*col & 0xFFFFFF);
            LobbyLogin_DrawSprites((struct NetSprDef *)&spr, 1, 0);
            spr = tbl[8];
            spr.x = r->x + 0xA;
            spr.y = 0;
            spr.h = r->y + r->h / 2;
            *col = (((u32)(255.0f * p->wave[2]) & 0xFF) << 24) | (*col & 0xFFFFFF);
            LobbyLogin_DrawSprites((struct NetSprDef *)&spr, 1, 0);
        }
    }
    Gs_SetState(0xD, stD);
    Gs_SetState(0x5E, st5E);
    Gs_SetState(0x6C, st6C);
}

void LoginWave_Clear(void)
{
    memset(gSaveBuf, 0, 0x58);
}

/* marks side `which` (0 or 1) of the object at 0x6AE040 and resets its six
 * values from gLoginWaveValues (the fourth and sixth get 0x4000) */
void LoginWave_Mark(s32 which)
{
    s32 i;
    Unk6AE040 *p;

    p = *(Unk6AE040 * *)&gSaveBuf;
    if (which == 0 || which == 1) {
        p->unk10[which] = 1;
        for (i = 0; i < 6; i++) {
            p->unk38[i] = 0;
            p->unk44[i] = gLoginWaveValues[i];
            if (i == 3) {
                p->unk38[i] = 0x4000;
            }
            if (i == 5) {
                p->unk38[i] = 0x4000;
            }
        }
    }
}

s32 LoginWave_Update(void)
{
    s16 *sel;
    Unk6AE040 *p;

    sel = (s16 *)gMenuWork;
    p = *(Unk6AE040 * *)&gSaveBuf;
    switch (p->state) {
    case 0:
        p->unk50 = -1;
        p->unk54 = -1;
        p->state++;
    case 1:
        p->unk50 = gLobbyMenu->cursor;
        p->unk54 = *sel;
        if (p->unk10[1] != 0) {
            p->unk38[0] += p->unk44[0];
            p->wave[0] = Math_Sin((6.2831855f * p->unk38[0]) / 65536.0f);
            p->unk38[1] += p->unk44[1];
            p->wave[1] = Math_Cos((6.2831855f * p->unk38[1]) / 65536.0f);
            p->unk38[2] = (p->unk38[2] + p->unk44[2]) & 0x7FFF;
            p->wave[2] = Math_Sin((6.2831855f * p->unk38[2]) / 65536.0f);
            p->unk38[3] = (p->unk38[3] + p->unk44[3]) & 0x7FFF;
            p->wave[3] = Math_Sin((6.2831855f * p->unk38[3]) / 65536.0f);
            p->unk38[4] = (p->unk38[4] + p->unk44[4]) & 0x7FFF;
            p->wave[4] = Math_Sin((6.2831855f * p->unk38[4]) / 65536.0f);
            p->unk38[5] = (p->unk38[5] + p->unk44[5]) & 0x7FFF;
            p->wave[5] = Math_Sin((6.2831855f * p->unk38[5]) / 65536.0f);
        }
        break;
    }
    return 0;
}

void LobbyEntry_DrawSprites(void)
{
    s32 temp_s2;
    s32 temp_s1;
    s32 v;

    NetMode_BindTextures();
    temp_s2 = Gs_GetState(0x6C);
    temp_s1 = Gs_GetState(0xD);
    v = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    Gs_SetState(0xD, 0);
    Gs_SetState(0x5E, 0x32);
    Gfx_SetTexFilter(1);
    LobbyLogin_DrawSprites((struct NetSprDef *)gLoginFrameSprites, 1, 0);
    Gs_SetState(0xD, temp_s1);
    Gs_SetState(0x5E, v);
    Gs_SetState(0x6C, temp_s2);
}

u32 LobbyLogin_LitColor(s32 slot)
{
    return gLoginLitColors[gLoginPageColorIdx[slot] + 1];
}

void LoginMenu_PlayCursorSe(void) {
    LoginSe_Cursor();
}

void LoginMenu_PlayDecideSe(void) {
    LoginSe_Decide();
}

/* the buttons pressed or repeating on the local player's pad */
s32 LoginPad_Triggered(void)
{
    s32 pad;
    u8 port;

    port = gScene.padIndex[0];
    pad = 0;
    pad |= gPadState[port].pressed | gPadState[port].repeat;
    return pad;
}

/* the buttons pressed this frame (on the first pad) */
s32 LoginPad_Pressed(void)
{
    s32 pad;

    pad = 0;
    pad |= gPadState[0].pressed;
    return pad;
}

/* the buttons held on pad `port` (only the first one is read) */
s32 LoginPad_Held(s8 port)
{
    s32 pad;

    pad = 0;
    if ((1 << port) & 1) {
        pad |= gPadState[0].held;
    }
    return pad;
}

s32 LoginPad_DecidePressed(M2C_UNK unused) {
    return (gPadState[0].pressed & 0x20) != 0;
}

s32 LoginPad_CancelPressed(s32 port) {
    return (gPadState[0].pressed & 0x40) != 0;
}

s32 LoginPad_Held8000(M2C_UNK unused) {
    return (gPadState[0].held & 0x8000) != 0;
}

s32 LoginPad_Any8000(void)
{
    if ((LoginPad_Held8000(0) == 0) && (LoginPad_Held8000(1) == 0)) {
        return 0;
    }
    return 1;
}

void LoginSe_Decide(void)
{
    Sound_PlaySeFlat(0, 0x13, 0);
}

void LoginSe_Cancel(void)
{
    Sound_PlaySeFlat(0, 0x14, 0);
}

void LoginSe_Play9(void)
{
    Sound_PlaySeFlat(0, 0x9, 0);
}

void LoginSe_Cursor(void)
{
    Sound_PlaySeFlat(0, 0x17, 0);
}

s32 LoginPad_Decide(s32 sound)
{
    s32 ret;

    ret = 0;
    if (LoginPad_DecidePressed(0) != 0) {
        switch (sound) {                             /* irregular */
        case 0:
            break;
        case 1:
            LoginSe_Decide();
            break;
        case 2:
            LoginSe_Play9();
            break;
        }
        ret = 1;
    }
    return ret;
}

s32 LoginPad_DecideEx(s32 arg0, s32 arg1)
{
    s32 ret;

    ret = 0;
    if (LoginPad_DecidePressed(0) != 0) {
        switch (arg0) {                             /* irregular */
        case 0:
            break;
        case 1:
            if (arg1 == 1) {
                if (gNetSaveData.mmbbLoginId[0] == 0) {
                    LoginSe_Play9();
                } else {
                    LoginSe_Decide();
                }
            } else {
                LoginSe_Decide();
            }
            break;
        case 2:
            LoginSe_Play9();
            break;
        }
        ret = 1;
    }
    return ret;
}

/* cancel (cross) pressed: plays the cancel sound, returns 1 */
s32 LoginPad_Cancel(void)
{
    s32 r;

    r = 0;
    if (LoginPad_CancelPressed(0) != 0) {
        LoginSe_Cancel();
        r = 1;
    }
    return r;
}

/* unk_7C8FA0's .data (0x819738-0x8197D0) */
u16 gLoginWaveValues[6] = {
#include "lobby/00819738.inc"
};
s16 gLoginFontCodes[40] = {
#include "lobby/00819750.inc"
};
u8 gLoginFontColumns[48] = {
#include "lobby/008197A0.inc"
};

/* A yes/no choice on the network screens: left and right move the cursor
 * (gNetMode.yesNo: 0 yes, 1 no) with the cursor's sound. Returns 1 when yes
 * is decided, -1 when no is decided or the choice is cancelled, 0 while
 * choosing. */
s32 NetMode_ChooseYesNo(void)
{
    s32 pad;

    pad = LoginPad_Pressed();
    if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if ((pad & PAD_LEFT) && gNetMode.yesNo != 0) {
            gNetMode.yesNo = 0;
            LoginSe_Cursor();
            return 0;
        }
        if ((pad & PAD_RIGHT) && gNetMode.yesNo == 0) {
            gNetMode.yesNo = 1;
            LoginSe_Cursor();
            return 0;
        }
    }
    if (LoginPad_Decide(1) != 0) {
        return (gNetMode.yesNo == 0) ? 1 : -1;
    }
    return -(LoginPad_Cancel() != 0);
}
