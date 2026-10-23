/*
 * The login's stages, which the lobby overlay's scene task runs as its
 * online steps (NetScene_StateOnline: NET_STEP_CONNECT, NET_STEP_EXIT and
 * NET_STEP_LEAVE).
 *
 * gLoginState holds the stage's state, and the counters gNetConnStep,
 * gLbsConnectStep, gNetScreenStep and gLoginConnectStep are cleared at each
 * move to a new state.
 * - The connection (LobbyLogin_RunConnect): the network screen, the lobby
 *   server's connection, the failed connection's message and the KDDI
 *   portal.
 * - The save and exit (LobbyLogin_SaveAndExit): the save menu, then the
 *   fade.
 * - The leave (LobbyLogin_Leave): the portal and the socket are closed,
 *   then one of four exits is chosen by the line's error (gNetErrorCode).
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "scene.h"
#include "card.h"
#include "net_mode.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_vobj.h"
#include "task.h"
#include "text.h"
#include "login_connect_leave.h"
#include "game_util.h"
#include "lobby_connect.h"
#include "lobby_dialog_box.h"
#include "lobby_frame.h"
#include "lobby_servers.h"
#include "lobby_util.h"
#include "net_scene.h"
#include "quest.h"
#include "screen_fade.h"

extern u8 gLoginState;
extern s8 gLoginConnectStep;
extern s16 gNetStepTimeout;
extern s8 gNetEntryMode;
extern s8 gNetErrorCode;
extern s8 gNetDropped;
extern u8 gLbsConnectStep;
extern u8 gNetScreenStep;
u8 NetMenu_Update(void);
extern u8 gNetConnStep;
extern M2C_UNK Gfx_DrawBlackLineFixed;
extern s16 gNetConnTries;

/* The portal's quit page's wait, each frame from its 90th: the text is
 * reset and, in 16 frames of every 32 (bit 4 of gNetConnTries clear), an
 * empty function is called (what it drew is not in the game). */
void LobbyLogin_BlinkWait(void)
{
    Text_Reset();
    gNetConnTries += 1;
    if (!(gNetConnTries & 0x10)) {
        LobbyLogin_DrawBlinkOn();
    }
}

/*
 * The login's connection (NET_STEP_CONNECT), one step a frame by
 * gLoginState (enum LoginConnectState):
 * - LOGIN_NET_SCREEN runs the network screen: 1 moves on to
 *   LOGIN_CONNECT; -1 starts the screen again at its step 1; -2 returns
 *   -2 (the leave); -3 opens the portal's next page; -4 returns -1 (the
 *   exit); -5 starts the stage again; any other result returns 2 (the
 *   caller draws the login's menu).
 * - LOGIN_CONNECT shows the connecting message and connects to the lobby
 *   server: connected, returns 1; failed, LOGIN_CONN_FAILED.
 * - LOGIN_CONN_FAILED (its steps in gNetConnStep): the network mode's
 *   frame and icon, message 13 for 300 frames (a button after 60), then
 *   the portal's quit page (NetScene_ToPortal, LOGIN_ENTRY_FIRST).
 * - LOGIN_PORTAL runs the portal: done, LOGIN_NET_SCREEN; left, returns
 *   -2; on the quit page LobbyLogin_BlinkWait from the 90th frame.
 * Returns 1 when connected, -1 to exit, -2 to leave, 2 to draw the menu,
 * else 0.
 */
s32 LobbyLogin_RunConnect(void)
{
    s16 timeLeft;
    s32 screenResult;
    s32 connResult;
    s32 portalResult;
    s32 ret;

    ret = 0;
    switch (gLoginState) {
    case LOGIN_START:
        gLoginState += 1;
        break;
    case LOGIN_NET_SCREEN:
        screenResult = NetScreen_Run();
        switch (screenResult) {
        case 1:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            gLoginState += 1;
            break;
        case -1:
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginState = LOGIN_NET_SCREEN;
            gNetConnStep = 1;
            gLoginConnectStep = 0;
            break;
        case -2:
            gNetConnStep = 0;
            ret = -2;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginState = LOGIN_NET_SCREEN;
            gLoginConnectStep = 0;
            break;
        case -3:
            gPortalPage = PORTAL_PAGE_NEXT;
            NetScene_ToPortal();
            break;
        case -4:
            gLoginState = LOGIN_START;
            ret = -1;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            break;
        case -5:
            gLoginState = LOGIN_START;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            break;
        default:
            ret = 2;
        }
        break;
    case LOGIN_CONNECT:
        LobbyDialog_ShowConnMsg(2);
        connResult = NetScreen_ConnectStep();
        switch (connResult) {
        case 1:
            Loading_Stop();
            ret = 1;
            gLoginState = LOGIN_START;
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            break;
        case -1:
            Loading_Stop();
            gNetConnStep = 0;
            gLoginState = LOGIN_CONN_FAILED;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
        }
        break;
    case LOGIN_CONN_FAILED:
        switch (gNetConnStep) {
        case 0:
            NetMode_Init();
            gNetConnStep += 1;
            
        case 1:
            gLbsConnectStep = 0;
            gNetConnStep += 1;
            gNetStepTimeout = 1;
            NetMode_OpenFrame();
            break;
        case 2:
            timeLeft = gNetStepTimeout - 1;
            gNetStepTimeout = timeLeft;
            if (timeLeft <= 0) {
                gNetConnStep += 1;
                gNetStepTimeout = 300;
                NetMode_OpenIcon();
                NetMode_FadeStart(0, 10, 0);
            }
            break;
        case 3:
            gNetStepTimeout -= 1;
            if (gNetStepTimeout > 0) {
                if ((gNetStepTimeout < 240) && (LoginPad_Decide(2) != 0)) {
                    goto end_message;
                }
            } else {
end_message:
                NetMode_FadeStart(0, 10, 1);
                gNetStepTimeout = 20;
                gNetConnStep += 1;
            }
            /* The two gotos stay: steps 3 and 4 share the message's end
             * and its drawing, as the game's code does. */
draw_message:
            NetModeMsg_QueuePage4(13);
            NetMode_CallDrawScreen(0);
            break;
        case 4:
            gNetStepTimeout -= 1;
            if (gNetStepTimeout <= 0) {
                NetMode_CloseAll();
                gNetEntryMode = LOGIN_ENTRY_FIRST;
                gPortalPage = PORTAL_PAGE_QUIT;
                NetScene_ToPortal();
                gNetStepTimeout = 0;
                gNetConnTries = 0;
            } else {
                goto draw_message;
            }
        }
        break;
    case LOGIN_PORTAL:
        portalResult = LobbyKddi_PortalStep();
        switch (portalResult) {
        case 1:
            gLoginState = LOGIN_NET_SCREEN;
            break;
        case -1:
            gNetConnStep = 0;
            ret = -2;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginState = LOGIN_NET_SCREEN;
            gLoginConnectStep = 0;
            break;
        default:
            if (gPortalPage == PORTAL_PAGE_QUIT) {
                gNetStepTimeout += 1;
                if (gNetStepTimeout >= 90) {
                    LobbyLogin_BlinkWait();
                }
            }
        }
        break;
    }
    return ret;
}

/*
 * The login's save-and-exit (NET_STEP_EXIT), one step a frame
 * (gNetConnStep): resets the game's subsystems, releases the network
 * textures, sets the menus' draw to Gfx_DrawBlackLineFixed, fades in and
 * stops the lobby's stream; then runs the logout's save menu
 * (NETMENU_MODE_GAME_SAVE_LOGOUT; a quit without saving also sets
 * BOOT_MODE_GMSUB) and fades out. Returns 1 once the fade is over, else
 * 0. The menus' layers are queued each frame.
 */
s32 LobbyLogin_SaveAndExit(void)
{
    s32 menuResult;
    s32 ret;

    ret = 0;
    switch (gNetConnStep) {
    case 0:
        Game_ResetSubsystems();
        NetMode_ReleaseTextures(0);
        LobbyMenu_SetDraw(0, 0, &Gfx_DrawBlackLineFixed, 0);
        FadeA_Start(FADE_IN_BLACK);
        LobbyStream_Stop();
        NetMenu_Open(NETMENU_MODE_GAME_SAVE_LOGOUT);
        gNetConnStep += 1;
        break;
    case 1:
        menuResult = NetMenu_Update() & 0xFF;
        if (menuResult != 0) {
            if (menuResult == 2) {
                Lobby_SetBootMode(BOOT_MODE_GMSUB);
            }
            gNetConnStep += 1;
            FadeA_Start(FADE_OUT_BLACK);
        }
        break;
    case 2:
        if ((FadeA_GetState() & 0xFF) != FADE_STATE_RUNNING) {
            gNetConnStep += 1;
        }
        break;
    case 3:
        ret = 1;
        break;
    }
    LobbyMenu_QueueLayers();
    return ret;
}

/*
 * The login's leave (NET_STEP_LEAVE), one step a frame by gLoginState
 * (enum LoginLeaveState): the portal's step ends, the lobby server's
 * socket closes (LobbyNet_DisconnectStep, which moves the lobby's error
 * to gNetErrorCode), and the error chooses the exit (enum LoginLeaveExit,
 * in gNetConnStep): a line's error (NET_ERR_*) shows its message, then
 * the set-up's settings; -9 goes to the settings at once; -11 releases
 * the textures; -50 and -17 (every server tried) show the failed
 * connection's message; any other error, or none, ends at once.
 * Returns, as the exit chose, 1 (on to the scene's end), 2 (the portal:
 * the caller opens its quit page) or 3 (back to the scene's start), else
 * 0. The network screen is drawn each frame.
 */
s32 LobbyLogin_Leave(void)
{
    s32 result;

    result = 0;
    switch (gLoginState) {
    case LOGIN_LEAVE_PORTAL:
        switch (LobbyKddi_PortalStep()) {
        case 1:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            gNetDropped = 0;
            gLoginState += 1;
            break;
        case -1:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            gNetDropped = 0;
            gLoginState += 1;
        }
        break;
    case LOGIN_LEAVE_DISCONNECT:
        switch (LobbyNet_DisconnectStep()) {
        case 1:
            gNetConnStep = 0;
            gLbsConnectStep = 0;
            gNetScreenStep = 0;
            gLoginConnectStep = 0;
            gNetDropped = 0;
            gLoginState += 1;
            break;
        }
        break;
    case LOGIN_LEAVE_CHOOSE:
        switch (gNetErrorCode) {
        case NET_ERR_PPP_LINE:
        case NET_ERR_PPP_FLAG:
        case NET_ERR_PPP_E3:
        case NET_ERR_NO_DEVICE:
        case NET_ERR_FAILED:
        case NET_ERR_IF_OPT8:
        case NET_ERR_DHCP:
        case NET_ERR_PPP_E6:
        case -80:
        case NET_ERR_CANCELED:
            gNetConnStep = LEAVE_EXIT_NET_ERROR;
            gLbsConnectStep = 0;
            gLoginState += 1;
            break;
        case -9:
            gLoginState += 1;
            gNetConnStep = LEAVE_EXIT_SETTINGS;
            gLbsConnectStep = 0;
            break;
        case -11:
            gNetConnStep = LEAVE_EXIT_RELEASE;
            gLbsConnectStep = 0;
            gLoginState += 1;
            break;
        case -50:
        case -17:   /* every server tried (NET_ERR_NO_SERVER_LEFT) */
            gLbsConnectStep = 0;
            gLoginState += 1;
            gNetConnStep = LEAVE_EXIT_CONN_FAILED;
            break;
        default:
            gLoginState = LOGIN_LEAVE_DONE;
        }
        break;
    case LOGIN_LEAVE_EXIT:
        switch (gNetConnStep) {
        case LEAVE_EXIT_NET_ERROR:
            if (LobbyLogin_ShowNetError() != 0) {
                result = 3;
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            }
            break;
        case LEAVE_EXIT_SETTINGS:
            if (LobbyLogin_LeaveToSettings() != 0) {
                result = 3;
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            }
            break;
        case LEAVE_EXIT_RELEASE:
            if (LobbyLogin_LeaveRelease() != 0) {
                result = 1;
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
            }
            break;
        case LEAVE_EXIT_CONN_FAILED:
            if (LobbyLogin_ShowConnFailed() != 0) {
                result = 2;
                gLoginState = 0;
                gNetConnStep = 0;
                gLbsConnectStep = 0;
                gNetScreenStep = 0;
                gLoginConnectStep = 0;
                gPortalPage = PORTAL_PAGE_NEXT;
            }
        }
        break;
    case LOGIN_LEAVE_DONE:
        gLoginState = 0;
        result = 1;
        gNetConnStep = 0;
        gLbsConnectStep = 0;
        gNetScreenStep = 0;
        gLoginConnectStep = 0;
        break;
    }
    NetMode_CallDrawScreen(0);
    return result;
}

/*
 * The leave's exit for a line's error, one step a frame (gLbsConnectStep):
 * the network mode's frame (reset when its textures and boxes are still
 * up, else opened anew after the game's subsystems are reset) and its
 * icon, then the error's message (the same messages as the set-up's
 * connection errors) for 600 frames (a button after 120), then the
 * set-up's settings (NetSetup_ToSettings). Returns 1 when done, else 0.
 */
s32 LobbyLogin_ShowNetError(void)
{
    s16 timeLeft;
    s16 unusedTimeLeft;
    s32 ret;

    ret = 0;
    switch (gLbsConnectStep) {
    case 0:
        if (gOvlTex != 0) {
            if (gNetMode.boxes != 0) {
                gLbsConnectStep = 2;
                gNetStepTimeout = 20;
                NetMode_ResetToFrame();
            }
        } else {
            gNetScreenStep = 0;
            gLbsConnectStep = 1;
            Game_ResetSubsystems();
            NetMode_ReleaseTextures(0);
        }
        break;
    case 1:
        switch (gNetScreenStep) {
        case 0:
            gNetScreenStep += 1;
            break;
        case 1:
            gLbsConnectStep += 1;
            gNetStepTimeout = 20;
            gNetScreenStep = 0;
            NetMode_OpenFrame();
        }
        break;
    case 2:
        timeLeft = gNetStepTimeout - 1;
        gNetStepTimeout = timeLeft;
        if (timeLeft <= 0) {
            gLbsConnectStep += 1;
            gNetStepTimeout = 600;
            NetMode_OpenIcon();
        }
        break;
    case 3:
        switch (gNetErrorCode) {
        case -2:
            NetModeMsg_QueuePage4(3U);
            break;
        case -3:
            NetModeMsg_QueuePage4(4U);
            break;
        case -5:
            NetModeMsg_QueuePage4(5U);
            break;
        case -81:
            NetModeMsg_QueuePage4(1U);
            break;
        case -82:
            NetModeMsg_QueuePage4(2U);
            break;
        case -83:
            NetModeMsg_QueuePage4(6U);
            break;
        case -84:
            NetModeMsg_QueuePage4(7U);
            break;
        case -85:
            NetModeMsg_QueuePage4(8U);
            break;
        case -86:
            NetModeMsg_QueuePage4(12U);
            break;
        }
        if (--gNetStepTimeout <= 0) {
            gLbsConnectStep += 1;
        } else if ((gNetStepTimeout < 481) && (LoginPad_Decide(2) != 0)) {
            gLbsConnectStep += 1;
        }
        break;
    case 4:
        gLbsConnectStep += 1;
        NetSetup_ToSettings();
        NetMode_CloseAll();
        
    case 5:
        ret = 1;
        break;
    }
    return ret;
}

/* The leave's exit to the set-up's settings: resets the game's
 * subsystems, releases the network textures, closes the icon's box, then
 * NetSetup_ToSettings. Returns 1 when done, else 0. */
s32 LobbyLogin_LeaveToSettings(void)
{
    s32 ret;

    ret = 0;
    switch (gLbsConnectStep) {
    case 0:
        gLbsConnectStep += 1;
        gNetScreenStep = 0;
        Game_ResetSubsystems();
        NetMode_ReleaseTextures(0);
        break;
    case 1:
        switch (gNetScreenStep) {
        case 0:
            gNetScreenStep += 1;
            NetMode_CloseBoxes(NET_BOX_ICON);
            break;
        case 1:
            gLbsConnectStep += 1;
            gNetScreenStep = 0;
            NetMode_CloseBoxes(NET_BOX_ICON);
        }
        break;
    case 2:
        gLbsConnectStep += 1;
        NetSetup_ToSettings();
        NetMode_CloseAll();
        
    case 3:
        ret = 1;
        break;
    }
    return ret;
}

/* The leave's exit that only releases the network textures. Returns 1. */
s32 LobbyLogin_LeaveRelease(void)
{
    Lobby_ReleaseNetTextures();
    return 1;
}

/* The leave's exit after a failed connection: the network mode's frame
 * and icon, message 13 for 600 frames (a button after 120), then the
 * boxes close and gNetErrorCode is cleared. Returns 1 when done, else 0. */
s32 LobbyLogin_ShowConnFailed(void)
{
    s32 result;

    result = 0;
    switch (gLbsConnectStep) {
    case 0:
        gLbsConnectStep += 1;
        gNetScreenStep = 0;
        Game_ResetSubsystems();
        NetMode_ReleaseTextures(0);
        break;
    case 1:
        switch (gNetScreenStep) {
        case 0:
            gNetScreenStep += 1;
            break;
        case 1:
            gLbsConnectStep += 1;
            gNetStepTimeout = 20;
            gNetScreenStep = 0;
            NetMode_OpenFrame();
        }
        break;
    case 2:
        if (--gNetStepTimeout <= 0) {
            gLbsConnectStep += 1;
            gNetStepTimeout = 600;
            NetMode_OpenIcon();
        }
        break;
    case 3:
        NetModeMsg_QueuePage4(13);
        if (--gNetStepTimeout <= 0) {
            gLbsConnectStep += 1;
            NetMode_CloseBoxes(NET_BOX_ICON);
        } else if (gNetStepTimeout < 481 && LoginPad_Decide(2) != 0) {
            gLbsConnectStep += 1;
            NetMode_CloseBoxes(NET_BOX_ICON);
        }
        break;
    case 4:
        gLbsConnectStep += 1;
        NetMode_CloseAll();
    case 5:
        gNetErrorCode = 0;
        result = 1;
        break;
    }
    return result;
}
