/*
 * The lobby's way out.
 *
 * Lobby_Disconnect sets the quit reason (LOBBY_QUIT_*), sends the player's
 * changed text first (Lobby_UpdatePlayerText) and enters LOBBY_PHASE_EXIT,
 * whose step functions (a table by reason, LobbyExit_Run) fade the screen,
 * send the leave or logout request (LobbyLeave_Run), close the socket and go
 * on to the next screen: LobbyExit_RunPlanned for the player's own leaving
 * (LOBBY_QUIT_RECONNECT to LOBBY_QUIT_TO_GMSUB_KEEP), LobbyExit_RunCut for a
 * session cut short (LOBBY_QUIT_IDLE to LOBBY_QUIT_SENT_OUT), which shows
 * the reason first (LobbyExit_ShowReason).
 */
#include "common.h"
#include "em.h"
#include "lobby_state.h"
#include "lobby_globals.h"
#include "lobby_net.h"
#include "lobby_menu.h"
#include "lobby_text.h"
#include "player.h"
#include "pad.h"
#include "task.h"
#include "hud.h"
#include "kbd.h"
#include "lobby_servers.h"
#include "net_mode.h"
#include "stream.h"
#include "text.h"
#include "lobby_disconnect.h"
#include "draw_helpers.h"
#include "equip_shop.h"
#include "game_util.h"
#include "hunter_card.h"
#include "item_shop.h"
#include "lobby_connect.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "net_scene.h"
#include "quest_order_terms.h"
#include "screen_fade.h"
#include "libc.h"

extern u8 gLobbyTextDirty;
extern s8 gServerSkipTried;
extern LobbyMenu *gLobbyMenu;
extern u8 gLobbyLeaveStep;
extern s8 gNetDisconnectStep;
extern NetUser gNetUsers[];

/*
 * Builds the player's text (the hunter card, HunterCard_Build) and, when it
 * differs from gPlayerText, copies it into every copy: the player's member
 * entry, LobbyState.myText, gPlayerText and the player's network user.
 * The buffer is 224 bytes though 256 are compared and copied: the
 * game's stack frame has that size.
 */
void Lobby_UpdatePlayerText(void)
{
    u8 text[224];

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

extern s32 gNetSock;
extern u8 gNetEntryMode;

#define EXIT_STREAM_FADE 15     /* frames */

/*
 * The exit for the player's own leaving (LOBBY_QUIT_RECONNECT to
 * LOBBY_QUIT_TO_GMSUB_KEEP), by LobbyState.step:
 * - 0: with a reason other than LOBBY_QUIT_RECONNECT, the screen and the
 *   streams fade out (step 1); for LOBBY_QUIT_RECONNECT straight to step 3;
 * - 1: once the fade ends, the streams stop and, unless the local player
 *   is in state 15, the screen fades back in, the lobby's music stops and
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
                if (gPlayers[gScene.localPlayer].state != 15) {
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
                NetScene_ToPortal();
                break;
            case LOBBY_QUIT_TO_GMSUB_KEEP:
                Lobby_SetBootMode(BOOT_MODE_GMSUB_KEEP);
                NetScene_ToNetFileCheck();
                break;
            case LOBBY_QUIT_TO_PORTAL:
                gNetEntryMode = LOGIN_ENTRY_FIRST;
                gPortalPage = PORTAL_PAGE_QUIT;
                NetScene_ToPortal();
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
                NetScene_ToDisconnect();
                gPortalPage = PORTAL_PAGE_TOP;
                break;
            case LOBBY_QUIT_FAILED:
            case LOBBY_QUIT_SERVER_CLOSE:
            case LOBBY_QUIT_LINK_LOST:
            case LOBBY_QUIT_SENT_OUT:
                NetScene_ToPortal();
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

/* This file's initialised data. */
/* The exit's step functions, by quit reason (LobbyExit_Run). */
void (*jtbl_00819650[10])(void) = {
    LobbyExit_RunPlanned, LobbyExit_RunPlanned, LobbyExit_RunPlanned, LobbyExit_RunPlanned,
    LobbyExit_RunCut, LobbyExit_RunCut, LobbyExit_RunCut, LobbyExit_RunCut,
    LobbyExit_RunCut, NULL,
};
