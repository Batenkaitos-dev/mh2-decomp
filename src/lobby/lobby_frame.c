/*
 * The online lobby's frame and its menu's draw layers.
 *
 * Lobby_Tick runs once a frame online: it receives, runs the idle,
 * receive and connection timeouts (a timeout leaves the lobby through
 * Lobby_Disconnect), the clock, the friend list's and the area search's
 * timers, then the current screen: gLobbyScreens holds one by
 * LobbyState.phase. LobbyMenu_QueueLayers puts the menu's first five draw
 * layers into the frame's text draw lists, and LobbyMenu_DrawDialog is the
 * dialog layer's draw callback.
 */
#include "common.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "lobby_menu.h"
#include "kbd.h"
#include "pad.h"
#include "browser.h"
#include "window.h"
#include "lobby_frame.h"
#include "area_list.h"
#include "draw_slots.h"
#include "game_util.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_disconnect.h"
#include "lobby_enter.h"
#include "lobby_login.h"
#include "lobby_room.h"
#include "lobby_town.h"
#include "lobby_util.h"
#include "lobby_village.h"
#include "text.h"

extern LobbyMenu *gLobbyMenu;
extern s32 gNetSock;                    /* the socket (sub_main) */
extern u8 gMenuInputLock;
extern u8 gConnCheckState;
extern s8 gFriendListOpen;
extern u16 gFriendCheckTimers[5];
extern s8 gFriendStatus[];                 /* the friends' statuses (enum FriendStatus): rows of 7 a page,
                                            * though Lobby_Tick's loop clears FRIEND_PAGE_LEN a row */

/* The friend list (gFriendCheckTimers: when a page's timer runs out, its friends' statuses
 * are cleared, to be asked for again). */
#define FRIEND_PAGES 5
#define FRIEND_PAGE_LEN 10
#define FRIEND_MAX 51
extern u16 gAreaSearchTimer;
extern void (*gLobbyScreens[])(void);      /* the lobby screens (LobbyState.phase) */
extern s32 gLobbyActive;

/* Input that keeps the lobby from its idle timeout: the D-pad or either stick. */
#define PAD_DIR_ANY (PAD_DPAD | PAD_RSTICK_UP | PAD_RSTICK_DOWN | PAD_RSTICK_LEFT | PAD_RSTICK_RIGHT)

/*
 * The online lobby's frame: receives (a message restarts the receive timeout,
 * LOBBY_RECV_TIMEOUT frames), counts down the menu's input lock and reads the pad.
 * On the lobby server, input restarts the idle timeout, and the lobby is left
 * (Lobby_Disconnect) when it runs out, the connection fails or the server ends the
 * session; after a disconnection the same checks start the connection check
 * (gConnCheckState). Then the clock, the friend list's timers (a list's statuses are
 * cleared when its timer runs out), the area search's timer, the current screen
 * (gLobbyScreens) and the menu's layers. Returns 1 after a timeout, else gLobbyActive.
 */
s32 Lobby_Tick(void)
{
    s32 pad = Pad_GetPressedRepeat(0);
    s32 j;
    s32 i;

    i = gSysBusy; /* read into i: read directly, it is tested in another order than the game's code */
    if (i == 0 && Lobby_ReceiveAll(gNetSock) == 1) {
        gLobby->recvTimer = LOBBY_RECV_TIMEOUT;
    }
    gLobby->recvTimer--;
    if (gMenuInputLock > 0) {
        gMenuInputLock--;
    }
    gLobby->buttons = LobbyPad_Read(0);
    if (gLobby->phase != LOBBY_PHASE_EXIT) {
        if (Lobby_IsSysBusy() == 0 && gLobby->connected != 0) {
            if (pad != 0 || gKbd->usbKey != 0 || (gPadState[0].dir & PAD_DIR_ANY)) {
                Lobby_ResetIdleTimer(&gLobby->idleTimer);
            }
            if (--gLobby->idleTimer < 0) {
                Lobby_Disconnect(LOBBY_QUIT_IDLE);
                return 1;
            }
            if (NetIf_GetError() != 0 || gConnCheckState != CONN_LOST_NONE) {
                Lobby_Disconnect(LOBBY_QUIT_LINK_LOST);
                return 1;
            }
            if (gLobby->recvTimer < 0) {
                Lobby_Disconnect(LOBBY_QUIT_LINK_LOST);
                return 1;
            }
            if (gLobby->serverClose == SERVER_CLOSE_CLOSED) {
                Lobby_Disconnect(LOBBY_QUIT_SERVER_CLOSE);
                return 1;
            }
            if (gLobby->serverClose == SERVER_CLOSE_SENT_OUT) {
                Lobby_Disconnect(LOBBY_QUIT_SENT_OUT);
                return 1;
            }
        } else if (gLobby->linkState == LINK_STATE_DOWN && gConnCheckState == CONN_LOST_NONE) {
            if (pad != 0 || gKbd->usbKey != 0) {
                Lobby_ResetIdleTimer(&gLobby->idleTimer);
            }
            if (gLobby->recvTimer < 0) {
                gConnCheckState = CONN_LOST_FOUND;
            } else if (--gLobby->idleTimer < 0) {
                gConnCheckState = CONN_LOST_FOUND;
            } else if (gLobby->serverClose == SERVER_CLOSE_CLOSED || gLobby->serverClose == SERVER_CLOSE_SENT_OUT) {
                gConnCheckState = CONN_LOST_FOUND;
            } else if (NetIf_GetError() != 0) {
                gConnCheckState = CONN_LOST_FOUND;
            }
        }
    }
    LobbyTime_Tick();
    gLobbyMenu->redraw = 0;
    gLobbyMenu->hideLayers = 0;
    Text_SetSize(6);
    if (gFriendListOpen == 0) {
        for (i = 0; i < FRIEND_PAGES; i++) {
            if (0 < gFriendCheckTimers[i]) {
                gFriendCheckTimers[i]--;
            } else if (gFriendCheckTimers[i] == 0) {
                for (j = 0; j < FRIEND_PAGE_LEN; j++) {
                    if (j + i * FRIEND_PAGE_LEN < FRIEND_MAX) {
                        gFriendStatus[i * 7 + j] = FRIEND_STATUS_UNKNOWN;
                    }
                }
                gFriendCheckTimers[i] = FRIEND_TIMER_OFF;
            }
        }
    }
    if (gAreaSearchTimer != 0) {
        gAreaSearchTimer--;
    }
    Chat_ClearBeep();
    gLobbyScreens[gLobby->phase]();
    if (gLobbyMenu->hideLayers == 0) {
        LobbyMenu_QueueLayers();
    }
    return gLobbyActive;
}

/* Puts the menu's first five draw layers that have a draw callback into the frame's
 * text draw lists, in their order (layer 3 into gDrawOtText3's 16 entries). */
void LobbyMenu_QueueLayers(void)
{
    MenuTask *layer;

    layer = gLobbyMenu->layer[0];
    if ((layer != NULL) && (layer->draw != NULL)) {
        OT_AddAt(gDrawListText0b, layer, 0, 1);
    }
    layer = gLobbyMenu->layer[1];
    if ((layer != NULL) && (layer->draw != NULL)) {
        OT_AddAt(gDrawListText1, layer, 0, 1);
    }
    layer = gLobbyMenu->layer[2];
    if ((layer != NULL) && (layer->draw != NULL)) {
        OT_AddAt(gDrawListText2, layer, 0, 1);
    }
    layer = gLobbyMenu->layer[3];
    if ((layer != NULL) && (layer->draw != NULL)) {
        OT_AddAt(gDrawOtText3, layer, 0, 16);
    }
    layer = gLobbyMenu->layer[4];
    if ((layer != NULL) && (layer->draw != NULL)) {
        OT_AddAt(gDrawListText5, layer, 0, 1);
    }
}

/* The menu's dialog layer (layer 4's draw callback): in the layer's text size, the
 * dialog and its user while the menu asks for its texts (LobbyMenu.redraw). */
void LobbyMenu_DrawDialog(MenuTask *task)
{
    u8 redraw;

    Text_SetSize(task->textSize);
    redraw = gLobbyMenu->redraw;
    if (redraw == 1) {
        LobbyDialog_Draw();
        LobbyDialog_DrawMember();
    }
}

/* The lobby's screens, by LobbyState.phase (Lobby_Tick). */
void (*gLobbyScreens[8])(void) = {
    LobbyEntry_Run, AreaList_Run, LobbyVillage_Run, LobbyTown_Run,
    LobbyRoom_Run, LobbyExit_Run, LobbyRoom_RunClose, LobbyEnter_Run,
};
