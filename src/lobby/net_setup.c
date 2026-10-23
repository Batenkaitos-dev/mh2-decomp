/*
 * The network screen of the online set-up: the frame under every step.
 * NetMode_Init and NetMode_LoadTextures set the screen up (its texture
 * pack in slots NET_TEX_SLOT on, with sub_main's lobby set), and the
 * set-up's task (NetSetupTask_*, one of sub_main's tasks) runs
 * NetMode_RunSetup each frame. NetMode_RunSetup calls the step of the
 * current mode (gNetMode.mode, enum NetSetupMode: the memory card, the
 * connection, the login, the portal, the online menu ...) and moves to
 * the next mode by the step's result; at the end the task starts the
 * lobby's scene or goes back to gm_sub. The rest are the helpers the
 * steps share: the title, the icons, the music (NetMode_BgmOn/_BgmOff),
 * the message queue's pages and the yes/no box. The steps themselves are
 * LobbyCard_Select, the NetSetup_* functions and the portal's steps.
 *
 * The file opens with the two network options only the set-up reads
 * (Lobby_GetNetOption0, which memory card slots to search;
 * Lobby_GetNetOption1, whether the settings menu follows the card step).
 */
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
#include "texture.h"
#include "overlay.h"
#include "sound.h"
#include "text.h"
#include "file_load.h"
#include "frame.h"
#include "main.h"
#include "mem_util.h"
#include "net_scene.h"
#include "libc.h"
#include "game_util.h"
#include "tex_files.h"

/* prototypes */
/* end of prototypes */

extern s32 gTexHandles[];

/* NetSetupTask_Step's steps (TaskSlot.step). */
enum NetSetupTaskStep {
    NET_SETUP_TASK_RUN = 0,         /* NetMode_RunSetup */
    NET_SETUP_TASK_TO_SCENE = 1,    /* NetSetupTask_StartScene */
    NET_SETUP_TASK_QUIT = 2         /* NetSetupTask_Quit */
};

#define NET_TEX_PACK_LEN 11         /* the textures of its pack (gNetSetupTexPack) */
#define TEX_SLOT_CARD_MENU 7        /* gCardMenuTex's slot */
#define TEX_SLOT_LOAD_LETTERS 8     /* gLoadLettersTex's slot */
#define TEX_SLOT_10 10
#define NET_SUBSTEP_NONE 255        /* NetModeState.subStep */
#define NET_MODE_BGM_ON 221         /* bgmOn's offset in NetModeState */
#define ICON_Y_TOP 78               /* NetModeState.iconY */
#define ICON_Y_LOW 350
#define ICON_Y_MID 332

/* Network option 0: the network mode's memory card choice (3: the choice
 * is skipped). */
u8 Lobby_GetNetOption0(void)
{
    return gSysSave.netOptions[0];
}

/* Network option 1, on or off: 0 skips the settings menu after the card
 * step. */
s32 Lobby_GetNetOption1(void)
{
    return gSysSave.netOptions[1];
}

/* Sets up the network screen: the textures released and its own loaded
 * (with sub_main's lobby set), the boxes closed and the title's wave
 * reset. */
void NetMode_Init(void)
{
    NetMode_ReleaseTextures(0);
    NetMode_EmptyResetHook();
    NetMode_EmptyInitHook();
    gNetMode.boxes = 0;
    NetMode_LoadTextures(0);
    LoginWave_Clear();
}

/* NetMode_OpenFrame, with the icon at the top. iconY is written as a word
 * here and in the two functions below, as the game's code stores it. */
void NetMode_OpenFrameTop(void)
{
    NetMode_OpenFrame();
    (*(s32 *)&gNetMode.iconY) = ICON_Y_TOP;
}

/* Opens icon 5, low on the screen. */
void NetMode_OpenIcon5(void)
{
    NetMode_OpenIconN(5);
    (*(s32 *)&gNetMode.iconY) = ICON_Y_LOW;
}

/* Opens icon 7, a little higher. */
void NetMode_OpenIcon7(void)
{
    NetMode_OpenIconN(7);
    (*(s32 *)&gNetMode.iconY) = ICON_Y_MID;
}

/* Back from a screen that used the memory: the common sound banks and the
 * textures loaded again, and the frame opened. */
void NetMode_ReloadFrame(void)
{
    Sound_LoadNetModeBanks();
    NetMode_LoadTextures(0);
    NetMode_OpenFrameTop();
}

/* Shows the title text (1-7; 0 none) in the title's box. */
void NetMode_SetTitle(s32 title)
{
    gNetMode.title = (s8) title;
    gNetMode.boxes = (s32) (gNetMode.boxes | NET_BOX_TITLE);
}

/* Loads the network screen's textures into its slots and records their
 * handles in gNetMode.tex: the pack's, then (mode 0) sub_main's lobby set
 * and three of its slots' handles. */
void NetMode_LoadTextures(s32 mode)
{
    NetModeState *state;
    s32 i;
    s32 n;

    state = &gNetMode;
    Tex_FreeRange(NET_TEX_SLOT, NET_TEX_SLOTS);
    Tex_FreeRange(NET_LOBBY_TEX_SLOT, NET_LOBBY_TEX_SLOTS);
    for (n = 0; n < NET_TEX_COUNT; n++) {
        state->tex[n] = 0;
    }
    n = Tex_LoadPack(gNetSetupTexPack, NET_TEX_SLOT, 0);
    for (i = 0; i < n; i++) {
        state->tex[i] = gTexHandles[NET_TEX_SLOT + i];
    }
    if (mode == 0) {
        Tex_LoadSystemPack(SYSTEM_TEX_LOBBY);
        state->tex[i++] = gLoadLettersTex;
        state->tex[i++] = D_6AE3F8;
        state->tex[i] = gCardMenuTex;
    }
    Mem_Compact();
}

/* Records the handles of the network screen's textures, already loaded:
 * tex[0-10] from its pack, tex[11-13] sub_main's slots 8, 10 and 7. */
void NetMode_BindTextures(void)
{
    s32 i;
    NetModeState *state;

    state = &gNetMode;
    for (i = 0; i < NET_TEX_PACK_LEN; i++) {
        state->tex[i] = gTexHandles[NET_TEX_SLOT + i];
    }
    state->tex[i++] = gTexHandles[TEX_SLOT_LOAD_LETTERS];
    state->tex[i++] = gTexHandles[TEX_SLOT_10];
    state->tex[i] = gTexHandles[TEX_SLOT_CARD_MENU];
}

/* The network setup's task (TASK_REQUEST_NET_SETUP: gm_sub's choice of
 * the online mode), in TASK_SLOT_SCENE: its first frame sets up, the
 * others run the setup. */
void NetSetupTask_Run(void *task)
{
    u8 mode;

    mode = ((TaskSlot *)task)->mode;
    switch (mode) {
    case 0:
        NetSetupTask_Init(task);
        break;
    case 1:
        NetSetupTask_Step(task);
        break;
    }
}

/* The setup's first frame: the game reset, the network state cleared, the
 * screen's textures loaded; the portal's top page and the boot back to
 * gm_sub (BOOT_MODE_GMSUB) are set. */
void NetSetupTask_Init(TaskSlot *task)
{
    s32 n;
    s32 i;
    NetModeState *state;

    state = &gNetMode;
    Game_ResetSubsystems();
    Gfx_ClearModelBuffers();
    Scene_ResetAll();
    Lobby_SetSysBusy();
    NetMode_Clear();
    NetMode_ReleaseTextures(0);
    n = Tex_LoadPack(gNetSetupTexPack, NET_TEX_SLOT, 0);
    for (i = 0; i < n; i++) {
        state->tex[i] = gTexHandles[NET_TEX_SLOT + i];
    }
    Tex_LoadSystemPack(SYSTEM_TEX_LOBBY);
    state->tex[i++] = gLoadLettersTex;
    state->tex[i++] = D_6AE3F8;
    state->tex[i] = gCardMenuTex;
    Mem_Compact();
    gPortalPage = PORTAL_PAGE_TOP;
    gBootMode = BOOT_MODE_GMSUB;
    task->mode++;
    task->step = 0;
}

/* The setup's frames: NetMode_RunSetup until it ends; 1 starts the lobby's
 * scene, -1 goes back to gm_sub. */
void NetSetupTask_Step(TaskSlot *task)
{
    switch (task->step) {
    case NET_SETUP_TASK_RUN:
        switch (NetMode_RunSetup()) {
        case 0:
            break;
        case 1:
            task->step = NET_SETUP_TASK_TO_SCENE;
            break;
        case -1:
            task->step = NET_SETUP_TASK_QUIT;
            break;
        }
        break;
    case NET_SETUP_TASK_TO_SCENE:
        NetSetupTask_StartScene(task);
        break;
    case NET_SETUP_TASK_QUIT:
        NetSetupTask_Quit(task);
        return;
    }
}

/* Ends the setup back to gm_sub: the textures and buffers freed, the
 * scene's state cleared, the select overlay loaded and select's boot task
 * started. */
void NetSetupTask_Quit(TaskSlot *task)
{
    Lobby_ClearSysBusy();
    NetMode_ReleaseTextures(0);
    Mem_Compact();
    NetMode_ClearBuffers();
    Game_ResetSubsystems();
    memset(&gScene, 0, sizeof(gScene));
    gOnlineFlag = 0;
    gSysSettings.unk40 = 0;
    Scene_LoadOverlay(OVERLAY_SELECT);
    TaskSlot_End(task);
    Task_StartBoot();
}

/* Ends the setup and starts the lobby's scene task (NetScene_Run) in the
 * same slot. */
void NetSetupTask_StartScene(TaskSlot *task)
{
    Lobby_ClearSysBusy();
    NetMode_ReleaseTextures(0);
    NetMode_ClearBuffers();
    Game_ResetSubsystems();
    memset(&gScene, 0, sizeof(gScene));
    gSysSettings.unk40 = 0;
    TaskSlot_End(task);
    TaskSlot_Set((void *)NetScene_Run, TASK_SLOT_SCENE);
}

void Lobby_SetSysBusy(void)
{
    gSysBusy = 1;
}

void Lobby_ClearSysBusy(void)
{
    gSysBusy = 0;
}

/* Moves the setup to a mode (enum NetSetupMode), its steps from 0. */
void NetMode_SetMode(s8 mode)
{
    gNetMode.mode = mode;
    gNetMode.step = 0;
    gNetMode.subStep = 0;
    gNetMode.innerStep = 0;
}

/* Clears the network mode's state and the NET file's data. */
void NetMode_Clear(void)
{
    memset(&gNetMode, 0, sizeof(gNetMode));
    memset(&gNetSaveData, 0, sizeof(NetSaveData));
}

/*
 * The network setup, one frame, by mode (enum NetSetupMode): the music,
 * the memory card, then the saved set-up's check (when network option 1
 * is off) or the settings menu; the connection, the login, the NET file's
 * save, the DNAS check and the KDDI portal. A failure goes to the settings
 * menu (with the icon of a failed connection) or, after the DNAS check, to
 * the disconnect; quitting goes to NET_SETUP_QUIT. The screen is drawn
 * unless noDraw's bits say otherwise. Returns 1 when the lobby's scene is
 * to start, -1 when the setup is quit, else 0.
 */
s32 NetMode_RunSetup(void)
{
    s32 result;
    s32 menuResult;
    s32 ret;
    NetModeState *state;

    ret = 0;
    state = &gNetMode;
    switch (gNetMode.mode) {
    case NET_SETUP_START:
        if (NetMode_StartSetup() != 0) {
            NetMode_SetMode(NET_SETUP_CARD);
            Lobby_SetBootMode(BOOT_MODE_GMSUB_KEEP);
        }
        break;
    case NET_SETUP_CARD:
        result = LobbyCard_Select();
        if (result != 0) {
            if (result == 1) {
                gSysSettings.cardSlot = gNetMode.slot;
                if (Lobby_GetNetOption1() == 0) {
                    NetMode_CloseBoxes(NET_BOX_ICON);
                    NetMode_SetMode(NET_SETUP_CHECK_SAVED);
                } else {
                    NetMode_SetMode(NET_SETUP_SETTINGS);
                    state->subStep = NET_SUBSTEP_NONE;
                }
            } else {
                NetMode_SetMode(NET_SETUP_QUIT);
                ret = -1;
            }
        }
        break;
    case NET_SETUP_CHECK_SAVED:
        result = NetSetup_CheckSaved();
        if (result != 0) {
            if (result == 1) {
                NetMode_SetMode(NET_SETUP_USE_SAVED);
            } else {
                state->ynNoDraw = 0;
                NetMode_SetMode(NET_SETUP_SETTINGS);
                state->subStep = NET_SUBSTEP_NONE;
            }
        }
        break;
    case NET_SETUP_CONNECT:
        menuResult = NetSetup_Connect();
        if (menuResult != 0) {
            NetMode_OpenFrameTop();
            if (menuResult == 1) {
                NetMode_CloseBoxes(NET_BOX_ICON);
                NetMode_SetMode(NET_SETUP_LOGIN);
            } else {
                NetMode_SetMode(NET_SETUP_SETTINGS);
                if (state->ynNoDraw != 0) {
                    state->ynNoDraw = 0;
                } else {
                    NetMode_OpenIcon5();
                    state->subStep = 2;
                }
            }
        }
        break;
    case NET_SETUP_LOGIN:
        result = NetSetup_Login();
        if (result != 0) {
            if (result == 1) {
                Sys_EmptyHook();
                NetMode_SetMode(NET_SETUP_SAVE_NET_FILE);
            } else {
                NetMode_SetMode(NET_SETUP_SETTINGS);
                gNetMode.subStep = NET_SUBSTEP_NONE;
            }
        }
        break;
    case NET_SETUP_USE_SAVED:
        if (NetSetup_UseSaved() != 0) {
            NetMode_SetMode(NET_SETUP_CONNECT);
        }
        break;
    case NET_SETUP_SAVE_NET_FILE:
        if (NetSetup_SaveNetFile() != 0) {
            NetMode_SetMode(NET_SETUP_DNAS);
        }
        break;
    case NET_SETUP_DNAS:
        result = NetSetup_Dnas();
        if (result != 0) {
            if (result == 1) {
                gPortalPage = PORTAL_PAGE_TOP;
                Lobby_SetBootMode(BOOT_MODE_GMSUB);
                NetMode_SetMode(NET_SETUP_PORTAL);
            } else {
                NetMode_SetMode(NET_SETUP_DISCONNECT);
            }
        }
        break;
    case NET_SETUP_PORTAL:
        result = LobbyKddi_Portal();
        if (result != 0) {
            if (result == -1 && gPortalPage == PORTAL_PAGE_TOP) {
                NetMode_SetMode(NET_SETUP_DISCONNECT);
            } else {
                NetMode_SetMode(NET_SETUP_LEAVE);
            }
        }
        break;
    case NET_SETUP_SETTINGS:
        menuResult = NetSetup_SettingsMenu();
        if (menuResult != 0) {
            if (menuResult != 2) {
                NetMode_OpenFrameTop();
            }
            if (menuResult == 1 || menuResult == 2) {
                NetMode_SetMode(NET_SETUP_CONNECT);
            } else {
                NetMode_SetMode(NET_SETUP_QUIT);
            }
        }
        break;
    case NET_SETUP_LEAVE:
        if (NetSetup_Leave() != 0) {
            ret = 1;
        }
        break;
    case NET_SETUP_DISCONNECT:
    case NET_SETUP_DISCONNECT_2:
        if (NetSetup_Disconnect() != 0) {
            NetMode_SetMode(NET_SETUP_QUIT);
        }
        break;
    case NET_SETUP_QUIT:
        if (NetSetup_Quit() != 0) {
            ret = -1;
        }
        break;
    }
    if (!(state->noDraw & 1)) {
        NetMode_EmptyDrawHook(0);
    }
    if (!(state->noDraw & 2)) {
        NetMode_CallDrawScreen(0);
    }
    return ret;
}

/* Starts the network mode's music, with the common sound banks, unless it
 * plays. bgmOn is stored through a byte pointer to the state, as the
 * game's code does: the field's own store compiles differently. */
void NetMode_BgmOn(void)
{
    u8 *bytes;

    bytes = (u8 *)&gNetMode;
    if ((*(u8 *)&gNetMode.bgmOn) == 0) {
        Sound_LoadNetModeBanks();
        NetMode_PlayBgm();
        bytes[NET_MODE_BGM_ON] = 1;
    }
}

/* Stops the network mode's music if it plays (stored as in NetMode_BgmOn). */
void NetMode_BgmOff(void)
{
    u8 *bytes;

    bytes = (u8 *)&gNetMode;
    if ((*(u8 *)&gNetMode.bgmOn) == 1) {
        Sound_Silence();
        bytes[NET_MODE_BGM_ON] = 0;
    }
}

/* NET_SETUP_START: the results cleared and the music on; returns 1. */
s32 NetMode_StartSetup(void)
{
    gNetMode.entry = 0;
    gNetMode.ynResult = 0;
    gNetMode.netResult = NET_RESULT_NONE;
    gNetMode.savedTaken = 0;
    NetMode_BgmOn();
    return 1;
}

/* Queues message id on the message box's page 1. */
void NetModeMsg_Page1(s32 id)
{
    NetModeMsg_Queue(NETMODE_QUEUE_FRONT, 1, id);
}

/* Queues a message of kind 5. */
void NetModeMsg_Queue5(s32 arg, s32 id)
{
    NetModeMsg_Queue(NETMODE_QUEUE_LIST, arg, id);
}

/* Queues message id on the message box's page 3. */
void NetModeMsg_Page3(s32 id)
{
    NetModeMsg_Queue(NETMODE_QUEUE_FRONT, 3, id);
}

/* Sets the yes/no cursor (0 yes, 1 no) and opens the yes/no box. */
void NetMode_OpenYesNo(s8 cursor)
{
    gNetMode.yesNo = cursor;
    gNetMode.yesNoOpen = 1;
    NetMode_OpenYesNoBar();
}

/* Opens the yes/no bar with the cursor on no. */
void NetMode_OpenYesNoAtNo(void)
{
    gNetMode.yesNo = 1;
    NetMode_OpenYesNoBar();
}

/* Opens the slot's choice, at the current memory card slot. */
void NetMode_PickSlot(void)
{
    gNetMode.sel2 = gNetMode.slot;
    NetMode_OpenSlotChoice();
}

/* Closes the yes/no box and the slot's choice. */
void NetMode_CloseYesNoBox(void)
{
    NetMode_CloseYesNo();
    NetMode_CloseBoxes(NET_BOX_SLOT_CHOICE);
    gNetMode.yesNoOpen = 0;
}
