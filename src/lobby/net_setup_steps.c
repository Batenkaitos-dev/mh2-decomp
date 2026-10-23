/*
 * The online set-up's steps on the network screen, which NetMode_RunSetup
 * runs by gNetMode.mode (enum NetSetupMode), and the login screen's
 * frame:
 * - LobbyCard_Select: the memory card's search, the read of the network
 *   file (gNetSaveData), or a new file made, with every card problem's
 *   message and question;
 * - NetSetup_Connect: yn (the network settings overlay) tries the saved
 *   settings of the two slots, and the result's message;
 * - NetSetup_Login: the connection to the network (NetMsg_Update), and
 *   NetSetup_ShowLoginError's message when it stops;
 * - LobbyKddi_Portal and LobbyKddi_PortalScene: the KDDI portal's pages in
 *   the browser;
 * - NetSetup_SettingsMenu: the online menu and the network settings'
 *   list (yn's screens, its set-up guide, the proxy, the login ID);
 * - the small modes: NetSetup_UseSaved, _CheckSaved, _Leave, _Disconnect,
 *   _Quit.
 * The login screen's frame (LobbyLogin_DrawMenu) is drawn under the server
 * menu and the area list, with the login wave (LoginWave) that makes the
 * chosen item's frame swell and glow; its wave table shares the data
 * piece with the steps' error tables, so the two stay one file. The
 * screen's frame, textures and messages are in the files
 * that run NetMode_RunSetup and draw gNetMode's boxes. The steps' cases
 * share their endings through gotos, which lay the code out as the game's
 * code does.
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
#include "texture.h"
#include "overlay.h"
#include "card.h"
#include "sound.h"
#include "render_state.h"
#include "net_tcp.h"
#include "lobby/net_setup_steps.h"   /* the texts this file uses (tools/content_gen.py) */
#include "lobby_servers.h"
#include "text.h"
#include "engine_math.h"
#include "file_load.h"
#include "frame.h"
#include "lobby_connect.h"
#include "lobby_dialog_box.h"
#include "lobby_util.h"
#include "main.h"
#include "mem_util.h"
#include "net_connection.h"
#include "net_file_save.h"
#include "screen_fade.h"
#include "libc.h"

/* prototypes */
extern char gBrowserUrl[256];       /* the URL the browser opens */
extern MmbbData gAccountData;
extern u8 gMenuWork[];
extern LobbyMenu *gLobbyMenu;
extern s8 gNetDisconnectStep;
/* end of prototypes */

extern s16 gNetStepTimeout;

/*
 * The login wave: the animation of the login screen's frame, in the save
 * buffer (gSaveBuf) while the lobby's menus run. The server menu marks
 * its screen (LOGIN_WAVE_SERVER_MENU) and the area list its own
 * (LOGIN_WAVE_AREA_LIST); LoginWave_Update turns the six angles each frame
 * and LobbyLogin_DrawMenu draws with their waves.
 */
typedef struct LoginWave {
    /* 0x00 */ s32 step;            /* 0: the cursor and page unset, then 1 */
    /* 0x04 */ u8 pad4[12];
    /* 0x10 */ s32 screen[2];       /* by LOGIN_WAVE_*: 1 while that screen is shown */
    /* 0x18 */ u8 pad18[8];
    /* 0x20 */ f32 wave[6];         /* the sines (the cosine for [1]) of the six angles */
    /* 0x38 */ u16 angle[6];        /* ANGLE_TURN a turn; [2]-[5] stay in the first half turn */
    /* 0x44 */ u16 speed[6];        /* each angle's step per frame (gLoginWaveValues) */
    /* 0x50 */ s32 cursor;          /* the menu's cursor (gLobbyMenu->cursor; -1 before the first frame) */
    /* 0x54 */ s32 page;            /* the area list's page (AreaList.group; -1 before the first frame) */
} LoginWave;

#define LOGIN_WAVE_SERVER_MENU 0    /* LoginWave.screen[]: the server menu: the frame's items unlit */
#define LOGIN_WAVE_AREA_LIST 1      /* the area list: its page's item lit and wobbling */
#define LOGIN_WAVE_HALF_TURN 0x7FFF /* the mask that keeps angles [2]-[5] in the first half turn */
#define LOGIN_WAVE_ALPHA_SHIFT 24   /* a sprite colour's alpha byte */

#define LOGIN_WAVES 6                /* LoginWave's angles */
/* gLoginFrameSprites' sprites. */
#define LOGIN_SPR_BASE 0
#define LOGIN_SPR_CURSOR 1          /* the colour of the server under the menu's cursor */
#define LOGIN_SPR_ITEM_FIRST 2      /* the four items: the lit one is gLoginPageColorIdx[page] */
#define LOGIN_SPR_ITEM_LAST 5
#define LOGIN_SPR_BAR 7             /* the bars down to the lit item's middle */
#define LOGIN_SPR_BAR2 8
#define LOGIN_SPR_SWELL 9           /* the sprites that swell around the lit item */
#define LOGIN_SPR_SWELL2 10

/* LobbyCard_Select's steps (gNetMode.step); each step's sub-steps are
 * gNetMode.subStep. */
enum NetCardStep {
    NET_CARD_STEP_START = 0,        /* the card task set up, the search's slots read */
    NET_CARD_STEP_SEARCH = 1,       /* the slots searched, then the file read */
    NET_CARD_STEP_CREATE = 2,       /* a new file: its slot chosen, searched, then saved */
    NET_CARD_STEP_FORMAT = 3,       /* the question to format the card, then the format */
    NET_CARD_STEP_PICK_SLOT = 4,    /* the player picks the slot to read */
    NET_CARD_STEP_OVERWRITE = 5,    /* the question to save over the slot's file */
    NET_CARD_STEP_PROBLEM = 10,     /* a problem's message, then its question (create, or quit) */
    NET_CARD_STEP_SAVE_PROBLEM = 15, /* a save's problem: the slot choice again, or quit */
    NET_CARD_STEP_QUIT = 20,        /* the screen fades out, then -1 */
    NET_CARD_STEP_LOADED = 30,      /* the file is read: its message until decided, or NET_CARD_LOADED_WAIT */
    NET_CARD_STEP_SAVED = 35        /* the new file is saved: its message until decided */
};

#define NET_CARD_LOADED_WAIT 120    /* frames: 4 s of NETMODE_MSG_LOADED (gNetMode.percent counts down) */
#define NET_CARD_LOADED_LOCK 45     /* frames of it the circle button can't skip (gNetMode.timer) */
#define NET_FILE_PACK_OFFSET 0x20000 /* where it is loaded, in gLoadBuf */
#define NET_FADE_FRAMES 30          /* the quit's fade */
#define NET_DISCONNECT_FADE 20

/* NetSetup_Connect's steps (gNetMode.step). */
enum NetConnectStep {
    NET_CONNECT_LOAD = 0,           /* yn is loaded, its title set */
    NET_CONNECT_START = 1,          /* Yn_Init */
    NET_CONNECT_RUN = 2,            /* Yn_Update until it ends */
    NET_CONNECT_OK = 3,             /* back to the lobby's scene state, then 1 */
    NET_CONNECT_OK_DONE = 4,
    NET_CONNECT_FAIL = 5,           /* the same, then -1 */
    NET_CONNECT_FAIL_DONE = 6,
    NET_CONNECT_OK_MSG = 10,        /* the connected message for NET_CONNECT_MSG_FRAMES */
    NET_CONNECT_FAIL_MSG = 11       /* the failure's message until decided */
};

#define NET_CONNECT_MSG_FRAMES 60   /* frames: 2 s */

/* Yn_Update's results. */
#define YN_RUNNING 0                /* while it runs; without its screens it fills gNetMode.error[] */
#define YN_DONE 1
#define YN_DONE_2 2
#define YN_FAILED -1

/* NetSetup_Login's steps (gNetMode.step). */
enum NetLoginStep {
    NET_LOGIN_OPEN = 0,             /* the boxes opened, the lobby's state reset */
    NET_LOGIN_RUN = 1,              /* NetMsg_Update until it connects or stops */
    NET_LOGIN_CLOSE = 2,            /* a wait (gNetMode.subStep), then the boxes close */
    NET_LOGIN_DONE = 3,             /* 1 */
    NET_LOGIN_ERROR_WAIT = 4,       /* a wait, then the error's icon */
    NET_LOGIN_ERROR = 5             /* the error's message until decided, then -1 */
};

#define NET_LOGIN_WAIT 8            /* frames before the boxes close */
#define NET_CANCEL_HOLD_STEP 2      /* gNetCancelHold's count per frame while start is held */
#define NET_ICON2_CONNECT 3         /* NetMode_OpenIcon2's icons */
#define NET_ICON2_ERROR 4

/* The KDDI portal's steps (gNetMode.step), in LobbyKddi_Portal and
 * LobbyKddi_PortalScene. */
enum KddiPortalStep {
    KDDI_STEP_START = 0,            /* the screen set up */
    KDDI_STEP_OPEN = 1,             /* the browser opens gPortalPage's page */
    KDDI_STEP_BROWSE = 2,           /* Browser_Update until it ends */
    KDDI_STEP_DONE = 3,             /* the browser closed, then 1 */
    KDDI_STEP_TOP_AGAIN = 4         /* the portal's top page is opened again */
};

#define BROWSER_GAME_MENU_2 2       /* Browser_Update: a page's game menu 2, which opens the top page again */

/* NetSetup_SettingsMenu's steps (gNetMode.step). */
enum NetMenuStep {
    NET_MENU_INIT = 0,              /* the lists' cursors set by gNetMode.subStep */
    NET_MENU_TOP = 1,               /* the online menu, NET_MENU_ROW_* */
    NET_MENU_SETTINGS = 2,          /* the network settings' list, NET_SETTINGS_ROW_* */
    NET_MENU_PROXY = 3,             /* yn's proxy settings */
    NET_MENU_DELETE_ID = 4,         /* the login ID's deletion (NetSetup_DeleteAccount) */
    NET_MENU_SETUP_GUIDE = 5        /* the question, then the fade out to yn's set-up guide */
};

/* NetSetup_SettingsMenu's gNetMode.subStep as it starts. */
#define NET_MENU_ENTER_TOP 0        /* the online menu, as it is */
#define NET_MENU_ENTER_RELOAD 1     /* the online menu, its frame loaded again (NetSetup_ToSettings) */
#define NET_MENU_ENTER_SETTINGS 2   /* the network settings' list */
#define NET_MENU_ENTER_NEW 255      /* the online menu, its title set */

/* The online menu's rows (gNetMode.sel3). */
#define NET_MENU_ROW_SETTINGS 0     /* the network settings' list */
#define NET_MENU_ROW_CONNECT 1      /* connect with the saved set-up; skipped without one */
#define NET_MENU_ROW_ID 2           /* the login ID; skipped without one */
#define NET_MENU_ROW_BACK 3

/* The network settings' rows (gNetMode.sel2). */
#define NET_SETTINGS_ROW_YN 0       /* yn's settings screens */
#define NET_SETTINGS_ROW_GUIDE 1    /* yn's set-up guide */
#define NET_SETTINGS_ROW_PROXY 2
#define NET_SETTINGS_ROW_BACK 3

/* NetSetup_SettingsMenu's results. */
#define NET_MENU_RUNNING 0
#define NET_MENU_CONNECT 1          /* connect with the saved set-up */
#define NET_MENU_YN 2               /* run yn's settings screens */
#define NET_MENU_GUIDE 3            /* yn's set-up guide was started */
#define NET_MENU_BACK -1

#define NET_LIST_MENU 21            /* NetModeMsg_Queue5's lists: the online menu */
#define NET_LIST_SETTINGS 22        /* the network settings */
#define NET_GUIDE_FADE 20           /* frames of the fade before the set-up guide */

/* NetSetup_Disconnect's steps (gNetMode.step). */
#define NET_DISC_FADE 0             /* the screen fades out */
#define NET_DISC_FADING 1           /* then the network is let go */
#define NET_DISC_WAIT 2             /* LobbyNet_DisconnectWait */
#define NET_DISC_DONE 3

#define LOGIN_SAVED_YES 1           /* gNetSaveData.setupResult: a set-up was saved */

extern u16 gLoginWaveValues[];
extern s16 gNetErrorCodes[];
extern u8 gNetErrorMsgs[];

extern u8 gLoginPageColorIdx[];
extern LoginBox gLoginBoxSprites[];
extern s8 gNetEntryMode;
struct NetSprDef;
void NetMode_DrawSprites(struct NetSprDef *d, s32 n, s32 dy);
extern NetSprDef gLoginFrameSprites[];
extern u8 D_008FAAA0[26];

/*
 * The set-up's memory card step (NET_SETUP_CARD), by gNetMode.step (enum
 * NetCardStep): searches the slots network option 0 names
 * (gNetMode.cardSearch), keeping what it finds in gNetMode.card[], and
 * reads the network file (gNetSaveData) from the card that has it. When no
 * card has it, the player is told why and asked whether to make a new file
 * (on a slot of their choice, formatting the card if needed) or to quit.
 * Returns 1 once the file is read or made (gNetMode.slot is its slot), -1
 * when the player quits, 0 meanwhile. The second slot's card state is
 * switched on through `pad`, a local the function already has: with a
 * local of its own the compiler gives the values other registers than the
 * game's code.
 */
s32 LobbyCard_Select(void)
{
    s32 pad;
    s32 result;
    NetModeState *net;

    result = 0;
    net = &gNetMode;
    pad = LoginPad_Triggered();
    switch (gNetMode.step) {
    case NET_CARD_STEP_START:
        NetMode_FadeReset();
        NetMode_OpenFrameTop();
        NetMode_SetTitle(NET_TITLE_CARD);
        net->card[0] = NET_CARD_UNCHECKED;
        net->card[1] = NET_CARD_UNCHECKED;
        CardTask_Begin(1);
        /* the buffer's address is offset as an integer: a pointer sum
         * compiles to other instructions than the game's */
        LoadCompressedFile((u8 *)(*(s32 *)&gLoadBuf + NET_FILE_PACK_OFFSET), NET_FILE_CARD_PACK);
        net->files = CardOp_SetFiles((s32 *)(*(s32 *)&gLoadBuf + NET_FILE_PACK_OFFSET));
        net->cardSearch = Lobby_GetNetOption0();
        if (net->cardSearch == NET_CARD_SEARCH_ASK) {
            net->step = NET_CARD_STEP_PICK_SLOT;
            net->sel3 = 0;
            net->sel2 = 0;
            net->slot = 0;
            net->slotUsed = 0;
            net->msg = NETMODE_MSG_NONE;
        } else {
            net->step = NET_CARD_STEP_SEARCH;
        }
        break;
    case NET_CARD_STEP_SEARCH:
        switch (net->subStep) {
        case 0:
            net->subStep++;
            net->sel3 = 0;
            net->sel2 = 0;
            if (net->cardSearch == NET_CARD_SEARCH_SLOT2) {
                net->slot = 1;
                net->slotUsed = 1;
            } else {
                net->slot = 0;
                net->slotUsed = 0;
            }
            net->msg = NETMODE_MSG_NONE;
            CardTask_StartFind(net->slotUsed, gLoadBuf, 1);
            break;
        case 1:
            if (net->cardSearch == NET_CARD_SEARCH_BOTH) {
                NetModeMsg_Page1(NETMODE_MSG_CHECKING_CARDS);
            } else {
                NetModeMsg_Page1(NETMODE_MSG_CHECKING_CARD);
            }
            CardTask_Update();
            switch (CardTask_Result()) {
            case CARD_BUSY:
                return 0;
            case CARD_OK:
                net->card[net->slotUsed] = NET_CARD_HAS_FILE;
                break;
            case CARD_ERR_UNFORMATTED:
            case CARD_ERR_NO_FILE:
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                break;
            case CARD_ERR_NO_SPACE:
                net->card[net->slotUsed] = NET_CARD_FULL;
                break;
            case CARD_ERR_NO_CARD:
                net->card[net->slotUsed] = NET_CARD_NONE;
                break;
            case CARD_ERR_BAD_DATA:
                net->card[net->slotUsed] = NET_CARD_BAD_DATA;
                break;
            default:
                net->card[net->slotUsed] = NET_CARD_ERROR;
                break;
            }
            if (net->cardSearch == NET_CARD_SEARCH_SLOT1) {
                net->slotUsed = 0;
                switch (net->card[0]) {
                case NET_CARD_NO_FILE:
                case NET_CARD_FULL:
                    NetMode_OpenYesNo(0);
                    net->msg = NETMODE_MSG_NO_FILE;
                    goto done;
                case NET_CARD_NONE:
                    net->msg = NETMODE_MSG_NO_CARD;
                    goto done;
                case NET_CARD_BAD_DATA:
                    net->msg = NETMODE_MSG_BAD_DATA;
                    goto done;
                case NET_CARD_HAS_FILE:
                    goto read;
                default:
                    net->msg = NETMODE_MSG_CARD_ERROR;
                    goto done;
                }
            } else if (net->cardSearch == NET_CARD_SEARCH_SLOT2) {
                net->slotUsed = 1;
                pad = net->card[1];
                switch (pad) {
                case NET_CARD_NO_FILE:
                case NET_CARD_FULL:
                    NetMode_OpenYesNo(0);
                    net->msg = NETMODE_MSG_NO_FILE;
                    goto done;
                case NET_CARD_NONE:
                    net->msg = NETMODE_MSG_NO_CARD;
                    goto done;
                case NET_CARD_BAD_DATA:
                    net->msg = NETMODE_MSG_BAD_DATA;
                    goto done;
                case NET_CARD_HAS_FILE:
                    goto read;
                default:
                    net->msg = NETMODE_MSG_CARD_ERROR;
                    goto done;
                }
            } else if (net->slotUsed == 0) {
                net->slotUsed = 1;
                CardTask_StartFind(net->slotUsed, gLoadBuf, 1);
                break;
            }
            if (net->card[0] == NET_CARD_HAS_FILE && net->card[1] == NET_CARD_HAS_FILE) {
                net->slotUsed = 0;
            read:
                net->slot = net->slotUsed;
                CardTask_StartRead(net->slotUsed, gLoadBuf);
                net->subStep++;
                break;
            }
            if (net->card[0] == NET_CARD_HAS_FILE) {
                net->slotUsed = 0;
                goto read;
            }
            if (net->card[1] == NET_CARD_HAS_FILE) {
                net->slotUsed = 1;
                goto read;
            }
            if (net->card[0] == NET_CARD_NONE && net->card[1] == NET_CARD_NONE) {
                net->msg = NETMODE_MSG_NO_CARDS;
                net->slotUsed = 0;
            done:
                net->slot = net->slotUsed;
                net->step = NET_CARD_STEP_PROBLEM;
                net->subStep = 0;
                break;
            }
            if (net->card[0] == NET_CARD_BAD_DATA && net->card[1] == NET_CARD_BAD_DATA) {
                net->slotUsed = 0;
                net->msg = NETMODE_MSG_BAD_DATA_BOTH;
                goto done;
            }
            net->msg = NETMODE_MSG_BAD_DATA;
            if (net->card[0] == NET_CARD_BAD_DATA) {
                net->slotUsed = 0;
                goto done;
            }
            if (net->card[1] == NET_CARD_BAD_DATA) {
                net->slotUsed = 1;
                goto done;
            }
            if (net->card[0] == NET_CARD_ERROR && net->card[1] == NET_CARD_ERROR) {
                net->slotUsed = 0;
                net->msg = NETMODE_MSG_CARD_ERROR_BOTH;
                goto done;
            }
            net->msg = NETMODE_MSG_CARD_ERROR;
            if (net->card[0] == NET_CARD_ERROR) {
                net->slotUsed = 0;
                goto done;
            }
            if (net->card[1] == NET_CARD_ERROR) {
                net->slotUsed = 1;
                goto done;
            }
            NetMode_OpenYesNo(0);
            net->yesNoOpen = 1;
            if (net->card[0] == NET_CARD_NO_FILE && net->card[1] == NET_CARD_NO_FILE) {
                net->slotUsed = 0;
                net->msg = NETMODE_MSG_NO_FILE_BOTH;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (net->card[0] == NET_CARD_NO_FILE && net->card[1] == NET_CARD_FULL) {
                net->slotUsed = 0;
                net->msg = NETMODE_MSG_NO_FILE_BOTH;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (net->card[0] == NET_CARD_FULL && net->card[1] == NET_CARD_NO_FILE) {
                net->slotUsed = 0;
                net->msg = NETMODE_MSG_NO_FILE_BOTH;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (net->card[0] == NET_CARD_FULL && net->card[1] == NET_CARD_FULL) {
                net->slotUsed = 0;
                net->msg = NETMODE_MSG_NO_FILE_BOTH;
                NetMode_OpenYesNo(0);
                goto done;
            }
            net->msg = NETMODE_MSG_NO_FILE;
            if (net->card[0] == NET_CARD_NO_FILE) {
                net->slotUsed = 0;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (net->card[1] == NET_CARD_NO_FILE) {
                net->slotUsed = 1;
                NetMode_OpenYesNo(0);
                goto done;
            }
            net->msg = NETMODE_MSG_NO_FILE;
            if (net->card[0] == NET_CARD_FULL) {
                net->slotUsed = 0;
                NetMode_OpenYesNo(0);
                goto done;
            }
            if (net->card[1] == NET_CARD_FULL) {
                net->slotUsed = 1;
                goto done;
            }
            break;
        case 2:
            NetModeMsg_Page1(NETMODE_MSG_READING);
            CardTask_Update();
            switch (CardTask_Result()) {
            case CARD_BUSY:
                break;
            case CARD_OK:
                if (LobbyCard_CheckNetValues(gLoadBuf) != 0) {
                    net->card[net->slotUsed] = NET_CARD_BAD_DATA;
                    net->msg = NETMODE_MSG_BAD_DATA;
                    goto done;
                }
                memset(&gNetSaveData, 0, sizeof(NetSaveData));
                memcpy(&gNetSaveData, gLoadBuf, sizeof(NetSaveData));
                net->slot = net->slotUsed;
                net->msg = NETMODE_MSG_LOADED;
                net->percent = NET_CARD_LOADED_WAIT;
                net->timer = NET_CARD_LOADED_LOCK;
                net->step = NET_CARD_STEP_LOADED;
                net->subStep = 0;
                LoginSe_Decide();
                break;
            case CARD_ERR_UNFORMATTED:
            case CARD_ERR_NO_FILE:
                net->msg = NETMODE_MSG_NO_FILE;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                NetMode_OpenYesNo(0);
            fail:
                LoginSe_Alert();
                net->slot = net->slotUsed;
                net->step = NET_CARD_STEP_PROBLEM;
                net->subStep = 0;
                break;
            case CARD_ERR_NO_SPACE:
                net->msg = NETMODE_MSG_NO_FILE;
                net->card[net->slotUsed] = NET_CARD_FULL;
                NetMode_OpenYesNo(0);
                goto fail;
            case CARD_ERR_NO_CARD:
                net->msg = NETMODE_MSG_NO_CARD;
                net->card[net->slotUsed] = NET_CARD_NONE;
                goto done;
            default:
                net->msg = NETMODE_MSG_READ_FAILED;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                goto done;
            }
            break;
        }
        break;
    case NET_CARD_STEP_CREATE:
        switch (net->subStep) {
        case 0:
            net->subStep++;
            net->sel3 = 0;
            net->sel2 = 0;
            net->msg = NETMODE_MSG_NONE;
            net->slot = 0;
            NetMode_PickSlot();
            break;
        case 1:
            NetModeMsg_Page1(NETMODE_MSG_PICK_SLOT_NEW);
            if ((pad & PAD_DOWN) && net->sel2 != 1) {
                LoginMenu_PlayCursorSe();
                net->sel2 = 1;
            } else if ((pad & PAD_UP) && net->sel2 != 0) {
                LoginMenu_PlayCursorSe();
                net->sel2 = 0;
            } else if (LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
                NetMode_CloseYesNoBox();
                net->subStep++;
                net->msg = NETMODE_MSG_NONE;
                net->slotUsed = net->sel2;
                CardTask_StartFind(net->slotUsed, gLoadBuf, 0);
            } else if (LoginPad_Cancel() != 0) {
                NetMode_CloseYesNoBox();
                net->slot = net->slotUsed;
                net->step = NET_CARD_STEP_PROBLEM;
                net->subStep = 0;
                NetMode_OpenYesNo(1);
                net->msg = NETMODE_MSG_ASK_QUIT;
            }
            break;
        case 2:
            NetModeMsg_Page1(NETMODE_MSG_CHECKING_CARD);
            CardTask_Update();
            switch (CardTask_Result()) {
            case CARD_BUSY:
                break;
            case CARD_ERR_UNFORMATTED:
                CardTask_StartPoll();
                net->slot = net->slotUsed;
                net->msg = NETMODE_MSG_ASK_FORMAT;
                net->step = NET_CARD_STEP_FORMAT;
                net->subStep = 0;
                NetMode_OpenYesNo(1);
                LoginSe_Alert();
                break;
            case CARD_OK:
                CardTask_StartPoll();
                net->slot = net->slotUsed;
                net->msg = NETMODE_MSG_ASK_OVERWRITE;
                net->step = NET_CARD_STEP_OVERWRITE;
                net->subStep = 0;
                NetMode_OpenYesNo(1);
                LoginSe_Alert();
                break;
            case CARD_ERR_NO_FILE:
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
                CardTask_StartSave(net->slotUsed, gLoadBuf);
                net->subStep++;
                break;
            case CARD_ERR_NO_SPACE:
                net->msg = NETMODE_MSG_NO_SPACE;
                net->card[net->slotUsed] = NET_CARD_FULL;
            retry:
                LoginSe_Alert();
                net->slot = net->slotUsed;
                net->step = NET_CARD_STEP_SAVE_PROBLEM;
                net->subStep = 0;
                break;
            case CARD_ERR_NO_CARD:
                net->msg = NETMODE_MSG_NO_CARD;
                net->card[net->slotUsed] = NET_CARD_NONE;
                goto retry;
            default:
                net->msg = NETMODE_MSG_SAVE_FAILED;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                goto retry;
            }
            break;
        case 3:
            NetModeMsg_Page1(NETMODE_MSG_SAVING);
            CardTask_Update();
            switch (CardTask_Result()) {
            case CARD_BUSY:
                break;
            case CARD_OK:
                net->slot = net->slotUsed;
                net->msg = NETMODE_MSG_SAVED;
                net->step = NET_CARD_STEP_SAVED;
                net->subStep = 0;
                LoginSe_Decide();
                break;
            case CARD_ERR_UNFORMATTED:
                CardTask_StartPoll();
                net->slot = net->slotUsed;
                net->msg = NETMODE_MSG_ASK_FORMAT;
                net->step = NET_CARD_STEP_FORMAT;
                net->subStep = 0;
                NetMode_OpenYesNo(1);
                LoginSe_Alert();
                break;
            case CARD_ERR_NO_FILE:
                net->msg = NETMODE_MSG_SAVE_FAILED;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                goto retry;
            case CARD_ERR_NO_SPACE:
                net->msg = NETMODE_MSG_NO_SPACE;
                net->card[net->slotUsed] = NET_CARD_FULL;
                NetMode_OpenYesNo(0);
                goto retry;
            case CARD_ERR_NO_CARD:
                net->msg = NETMODE_MSG_NO_CARD;
                net->card[net->slotUsed] = NET_CARD_NONE;
                goto retry;
            default:
                net->msg = NETMODE_MSG_SAVE_FAILED;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                goto retry;
            }
            break;
        }
        break;
    case NET_CARD_STEP_FORMAT:
        switch (net->subStep) {
        case 0:
            NetModeMsg_Page1(net->msg);
            CardTask_Update();
            if (Card_PortState(gNetMode.slotUsed) == 0) {
                NetMode_CloseYesNoBox();
                net->msg = NETMODE_MSG_CARD_REMOVED;
                net->step = NET_CARD_STEP_PROBLEM;
                net->subStep = 0;
                LoginSe_Alert();
                break;
            }
            switch (NetMode_ChooseYesNo()) {
            case 0:
                break;
            case 1:
                NetMode_CloseYesNoBox();
                net->subStep++;
                CardTask_StartFormat(gNetMode.slot);
                break;
            case -1:
                net->msg = NETMODE_MSG_ASK_QUIT;
                net->step = NET_CARD_STEP_PROBLEM;
                net->subStep = 0;
                NetMode_OpenYesNo(1);
                break;
            }
            break;
        case 1:
            NetModeMsg_Page1(NETMODE_MSG_FORMATTING);
            CardTask_Update();
            switch (CardTask_Result()) {
            case CARD_BUSY:
                break;
            case CARD_OK:
                memset(&gNetSaveData, 0, sizeof(NetSaveData));
                memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                CardTask_StartSave(net->slotUsed, gLoadBuf);
                net->step = NET_CARD_STEP_CREATE;
                net->subStep = 3;
                break;
            case CARD_ERR_FAILED:
            default:
                net->msg = NETMODE_MSG_FORMAT_FAILED;
                net->step = NET_CARD_STEP_PROBLEM;
                net->subStep = 0;
                LoginSe_Alert();
                break;
            }
            break;
        }
        break;
    case NET_CARD_STEP_OVERWRITE:
        NetModeMsg_Page1(net->msg);
        CardTask_Update();
        if (Card_PortState(gNetMode.slotUsed) == 0) {
            NetMode_CloseYesNoBox();
            net->msg = NETMODE_MSG_CARD_REMOVED;
            net->step = NET_CARD_STEP_PROBLEM;
            net->subStep = 0;
            LoginSe_Alert();
            break;
        }
        switch (NetMode_ChooseYesNo()) {
        case 0:
            break;
        case 1:
            net->card[net->slotUsed] = NET_CARD_NO_FILE;
            memcpy(gLoadBuf, &gNetSaveData, sizeof(NetSaveData));
            CardTask_StartSave(net->slotUsed, gLoadBuf);
            NetMode_CloseYesNoBox();
            net->step = NET_CARD_STEP_CREATE;
            net->subStep = 3;
            break;
        case -1:
            net->msg = NETMODE_MSG_ASK_QUIT;
            net->step = NET_CARD_STEP_PROBLEM;
            net->subStep = 0;
            NetMode_OpenYesNo(1);
            break;
        }
        break;
    case NET_CARD_STEP_PROBLEM:
        NetModeMsg_Page1(net->msg);
        if (net->yesNoOpen == 0 && LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
            NetMode_OpenYesNo(1);
            net->msg = NETMODE_MSG_ASK_QUIT;
        } else if (net->yesNoOpen != 0) {
            switch (NetMode_ChooseYesNo()) {
            case 0:
                break;
            case 1:
                switch (net->msg) {
                case NETMODE_MSG_ASK_QUIT:
                    net->step = NET_CARD_STEP_QUIT;
                    NetMode_CloseYesNoBox();
                    NetMode_FadeStart(0, NET_FADE_FRAMES, 1);
                    break;
                default:
                    memset(&gNetSaveData, 0, sizeof(NetSaveData));
                    net->step = NET_CARD_STEP_CREATE;
                    net->subStep = 0;
                    NetMode_CloseYesNoBox();
                    if (net->msg == NETMODE_MSG_NO_FILE) {
                        net->subStep = 2;
                        net->slotUsed = net->slot;
                        CardTask_StartFind(net->slotUsed, gLoadBuf, 0);
                    }
                    net->msg = NETMODE_MSG_NONE;
                    break;
                }
                break;
            case -1:
                switch (net->msg) {
                case NETMODE_MSG_ASK_QUIT:
                    goto again;
                default:
                    NetMode_OpenYesNo(1);
                    net->msg = NETMODE_MSG_ASK_QUIT;
                    break;
                }
                break;
            }
        }
        break;
    case NET_CARD_STEP_SAVE_PROBLEM:
        NetModeMsg_Page1(net->msg);
        if (net->yesNoOpen == 0 && LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
            NetMode_OpenYesNo(1);
            net->msg = NETMODE_MSG_ASK_QUIT;
        } else if (net->yesNoOpen != 0) {
            switch (NetMode_ChooseYesNo()) {
            case 0:
                break;
            case 1:
                switch (net->msg) {
                case NETMODE_MSG_ASK_QUIT:
                    net->step = NET_CARD_STEP_QUIT;
                    NetMode_CloseYesNoBox();
                    NetMode_FadeStart(0, NET_FADE_FRAMES, 1);
                    break;
                default:
                    goto again;
                }
                break;
            case -1:
                switch (net->msg) {
                case NETMODE_MSG_ASK_QUIT:
                again:
                    net->step = NET_CARD_STEP_PICK_SLOT;
                    net->subStep = 0;
                    net->sel3 = 0;
                    net->sel2 = net->slot;
                    net->msg = NETMODE_MSG_NONE;
                    NetMode_CloseYesNoBox();
                    break;
                default:
                    NetMode_OpenYesNo(1);
                    net->msg = NETMODE_MSG_ASK_QUIT;
                    break;
                }
                break;
            }
        }
        break;
    case NET_CARD_STEP_PICK_SLOT:
        switch (net->subStep) {
        case 0:
            net->subStep++;
            net->sel3 = 0;
            net->msg = NETMODE_MSG_NONE;
            NetMode_PickSlot();
            net->slotUsed = net->slot;
            break;
        case 1:
            NetModeMsg_Page1(NETMODE_MSG_PICK_SLOT);
            if ((pad & PAD_DOWN) && net->sel2 != 1) {
                LoginMenu_PlayCursorSe();
                net->sel2 = 1;
            } else if ((pad & PAD_UP) && net->sel2 != 0) {
                LoginMenu_PlayCursorSe();
                net->sel2 = 0;
            } else if (LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
                NetMode_CloseYesNoBox();
                net->subStep++;
                net->msg = NETMODE_MSG_NONE;
                net->slotUsed = net->sel2;
                net->slot = net->slotUsed;
                CardTask_StartFind(net->slotUsed, gLoadBuf, 0);
            } else if (LoginPad_Cancel() != 0) {
                NetMode_CloseYesNoBox();
                net->slot = 0;
                net->step = NET_CARD_STEP_PROBLEM;
                net->subStep = 0;
                NetMode_OpenYesNo(1);
                net->msg = NETMODE_MSG_ASK_QUIT;
            }
            break;
        case 2:
            NetModeMsg_Page1(NETMODE_MSG_CHECKING_CARD);
            CardTask_Update();
            switch (CardTask_Result()) {
            case CARD_BUSY:
                break;
            case CARD_ERR_NO_CARD:
                net->msg = NETMODE_MSG_NO_CARD;
                net->card[net->slotUsed] = NET_CARD_NONE;
                LoginSe_Alert();
                net->slot = net->slotUsed;
                net->step = NET_CARD_STEP_SAVE_PROBLEM;
                net->subStep = 0;
                break;
            case CARD_OK:
                CardTask_StartRead(net->slotUsed, gLoadBuf);
                net->subStep++;
                break;
            case CARD_ERR_UNFORMATTED:
            case CARD_ERR_NO_FILE:
            case CARD_ERR_NO_SPACE:
                net->msg = NETMODE_MSG_NO_FILE;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                net->step = NET_CARD_STEP_PROBLEM;
                net->subStep = 0;
                NetMode_OpenYesNo(0);
                break;
            default:
                net->msg = NETMODE_MSG_READ_FAILED;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                goto done;
            }
            break;
        case 3:
            NetModeMsg_Page1(NETMODE_MSG_READING);
            CardTask_Update();
            switch (CardTask_Result()) {
            case CARD_BUSY:
                break;
            case CARD_OK:
                if (LobbyCard_CheckNetValues(gLoadBuf) != 0) {
                    net->card[net->slotUsed] = NET_CARD_BAD_DATA;
                    net->msg = NETMODE_MSG_BAD_DATA;
                    goto done;
                }
                memset(&gNetSaveData, 0, sizeof(NetSaveData));
                memcpy(&gNetSaveData, gLoadBuf, sizeof(NetSaveData));
                net->slot = net->slotUsed;
                net->msg = NETMODE_MSG_LOADED;
                net->percent = NET_CARD_LOADED_WAIT;
                net->timer = NET_CARD_LOADED_LOCK;
                net->step = NET_CARD_STEP_LOADED;
                net->subStep = 0;
                LoginSe_Decide();
                break;
            case CARD_ERR_UNFORMATTED:
            case CARD_ERR_NO_FILE:
                net->msg = NETMODE_MSG_NO_FILE;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                NetMode_OpenYesNo(0);
                goto fail;
            case CARD_ERR_NO_SPACE:
                net->msg = NETMODE_MSG_NO_FILE;
                net->card[net->slotUsed] = NET_CARD_FULL;
                NetMode_OpenYesNo(0);
                goto fail;
            case CARD_ERR_NO_CARD:
                net->msg = NETMODE_MSG_NO_CARD;
                net->card[net->slotUsed] = NET_CARD_NONE;
                goto done;
            default:
                net->msg = NETMODE_MSG_READ_FAILED;
                net->card[net->slotUsed] = NET_CARD_NO_FILE;
                goto done;
            }
            break;
        }
        break;
    case NET_CARD_STEP_QUIT:
        if (NetMode_FadeRunning() == 0) {
            NetMode_CloseAll();
            result = -1;
        }
        break;
    case NET_CARD_STEP_LOADED:
        NetModeMsg_Page1(net->msg);
        if (--net->percent == 0) {
            LoginMenu_PlayDecideSe();
            goto ok;
        }
        if (net->timer != 0) {
            net->timer--;
        }
        if (net->yesNoOpen == 0 && LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
        ok:
            result = 1;
        }
        break;
    case NET_CARD_STEP_SAVED:
        NetModeMsg_Page1(net->msg);
        if (net->yesNoOpen == 0 && LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
            goto ok;
        }
        break;
    }
    return result;
}

void Yn_Init(s32 on);
s32 Yn_Update(void);
void Yn_Quit(void);

/*
 * The set-up's connection step (NET_SETUP_CONNECT), by gNetMode.step (enum
 * NetConnectStep): loads yn and runs it until it ends. Without yn's
 * screens (gNetMode.ynNoDraw), yn tries the saved settings of each slot
 * and leaves its failures in gNetMode.error[]: when a slot connected, its
 * message shows for NET_CONNECT_MSG_FRAMES and that slot is kept; when
 * both failed, the message for the pair of failures shows until decided.
 * The scene's lobby flag is off while yn runs. Returns 1 when connected,
 * -1 when not, 0 meanwhile; gNetMode.slotUsed is restored at the end.
 */
s32 NetSetup_Connect(void)
{
    NetModeState *net = &gNetMode;
    s32 result = 0;
    s32 slot1Err;
    s32 slot2Err;

    switch (gNetMode.step) {
    case NET_CONNECT_LOAD:
        net->savedSlot = net->slotUsed;
        net->step++;
        LoadOverlay(OVERLAY_YN);
        if (net->ynNoDraw == 0) {
            NetMode_SetTitle(NET_TITLE_CONNECT);
        } else {
            net->error[1] = net->error[0] = NET_YN_ERR_NOT_TRIED;
        }
        NetMode_FadeReset();
        gScene.inLobby = 0;
        break;
    case NET_CONNECT_START:
        net->step++;
        Yn_Init(net->ynNoDraw);
        break;
    case NET_CONNECT_RUN:
        switch (Yn_Update()) {
        case YN_RUNNING:
            if (net->ynNoDraw == 0) {
                break;
            }
            NetModeMsg_Page1(NETMODE_MSG_YN_TRYING);
            slot1Err = net->error[0];
            if (slot1Err == NET_YN_ERR_NOT_TRIED || (slot1Err != 0 && net->error[1] == NET_YN_ERR_NOT_TRIED)) {
                break;
            }
            if (slot1Err == 0 || net->error[1] == 0) {
                LoginSe_Decide();
                net->step = NET_CONNECT_OK_MSG;
                net->subStep = 0;
                if (net->error[0] == 0) {
                    net->slotUsed = 0;
                } else {
                    net->slotUsed = 1;
                }
                net->msg = NETMODE_MSG_YN_CONNECTED;
            } else {
                LoginSe_Alert();
                net->step = NET_CONNECT_FAIL_MSG;
                net->timer = 0;
                slot1Err = net->error[0];
                if (slot1Err == NET_YN_ERR_NO_CARD && net->error[1] == NET_YN_ERR_NO_CARD) {
                    net->msg = NETMODE_MSG_YN_NO_CARD;
                } else if (slot1Err == NET_YN_ERR_EMPTY_FILE && net->error[1] == NET_YN_ERR_EMPTY_FILE) {
                    net->msg = NETMODE_MSG_YN_EMPTY_FILE;
                } else if (slot1Err == NET_YN_ERR_NO_SETTINGS && net->error[1] == NET_YN_ERR_NO_SETTINGS) {
                    net->msg = NETMODE_MSG_YN_NO_SETTINGS;
                } else if (slot1Err == NET_YN_ERR_OTHER_PS2 && net->error[1] == NET_YN_ERR_OTHER_PS2) {
                    net->msg = NETMODE_MSG_YN_OTHER_PS2;
                } else if (slot1Err == NET_YN_ERR_CARD_REMOVED || (slot2Err = net->error[1]) == NET_YN_ERR_CARD_REMOVED) {
                    if (slot1Err == NET_YN_ERR_CARD_REMOVED) {
                        net->slotUsed = 0;
                    } else {
                        net->slotUsed = 1;
                    }
                    net->msg = NETMODE_MSG_YN_CARD_REMOVED;
                } else if (slot1Err == NET_YN_ERR_DEVICE || slot2Err == NET_YN_ERR_DEVICE) {
                    net->msg = NETMODE_MSG_YN_DEVICE;
                } else {
                    switch (slot1Err) {
                    case NET_YN_ERR_NO_CARD:
                        switch (slot2Err) {
                        case NET_YN_ERR_EMPTY_FILE:
                            net->msg = NETMODE_MSG_YN_NO_CARD_EMPTY;
                            break;
                        case NET_YN_ERR_OTHER_PS2:
                            net->msg = NETMODE_MSG_YN_NO_CARD_OTHER_PS2;
                            break;
                        case NET_YN_ERR_NO_SETTINGS:
                            net->msg = NETMODE_MSG_YN_NO_CARD_NO_SETTINGS;
                            break;
                        default:
                            net->slotUsed = 0;
                            net->msg = NETMODE_MSG_YN_NO_CARD;
                            break;
                        }
                        break;
                    case NET_YN_ERR_EMPTY_FILE:
                        switch (slot2Err) {
                        case NET_YN_ERR_NO_CARD:
                            net->msg = NETMODE_MSG_YN_EMPTY_NO_CARD;
                            break;
                        case NET_YN_ERR_OTHER_PS2:
                            net->msg = NETMODE_MSG_YN_EMPTY_OTHER_PS2;
                            break;
                        case NET_YN_ERR_NO_SETTINGS:
                            net->msg = NETMODE_MSG_YN_EMPTY_NO_SETTINGS;
                            break;
                        default:
                            net->slotUsed = 0;
                            net->msg = NETMODE_MSG_YN_FAILED;
                            break;
                        }
                        break;
                    case NET_YN_ERR_NO_SETTINGS:
                        switch (slot2Err) {
                        case NET_YN_ERR_NO_CARD:
                            net->msg = NETMODE_MSG_YN_NO_SETTINGS_NO_CARD;
                            break;
                        case NET_YN_ERR_OTHER_PS2:
                            net->msg = NETMODE_MSG_YN_NO_SETTINGS_OTHER_PS2;
                            break;
                        case NET_YN_ERR_EMPTY_FILE:
                            net->msg = NETMODE_MSG_YN_NO_SETTINGS_EMPTY;
                            break;
                        default:
                            net->slotUsed = 0;
                            net->msg = NETMODE_MSG_YN_FAILED;
                            break;
                        }
                        break;
                    case NET_YN_ERR_OTHER_PS2:
                        switch (slot2Err) {
                        case NET_YN_ERR_NO_CARD:
                            net->msg = NETMODE_MSG_YN_OTHER_PS2_NO_CARD;
                            break;
                        case NET_YN_ERR_EMPTY_FILE:
                            net->msg = NETMODE_MSG_YN_OTHER_PS2_EMPTY;
                            break;
                        case NET_YN_ERR_NO_SETTINGS:
                            net->msg = NETMODE_MSG_YN_OTHER_PS2_NO_SETTINGS;
                            break;
                        default:
                            net->slotUsed = 0;
                            net->msg = NETMODE_MSG_YN_FAILED;
                            break;
                        }
                        break;
                    default:
                        net->slotUsed = 0;
                        net->msg = NETMODE_MSG_YN_FAILED;
                        break;
                    }
                }
            }
            Yn_Quit();
            break;
        case YN_DONE_2:
        case YN_DONE:
            net->step = NET_CONNECT_OK;
            break;
        case YN_FAILED:
            net->step = NET_CONNECT_FAIL;
            break;
        }
        break;
    case NET_CONNECT_OK:
        net->step++;
        gScene.inLobby = 1;
        break;
    case NET_CONNECT_OK_DONE:
    done:
        net->slotUsed = net->savedSlot;
        NetMode_FadeReset();
        result = 1;
        break;
    case NET_CONNECT_FAIL:
        net->step++;
        gScene.inLobby = 1;
        break;
    case NET_CONNECT_FAIL_DONE:
    failed:
        net->slotUsed = net->savedSlot;
        NetMode_FadeReset();
        result = -1;
        break;
    case NET_CONNECT_OK_MSG:
        if (++net->subStep < NET_CONNECT_MSG_FRAMES) {
            NetModeMsg_Page1(net->msg);
        } else {
            if (net->ynResult == 2) {
                net->ynResult = 1;
            } else {
                net->ynResult = 0;
            }
            goto done;
        }
        break;
    case NET_CONNECT_FAIL_MSG:
        NetModeMsg_Page1(net->msg);
        if (LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
            net->msg = NETMODE_MSG_NONE;
            goto failed;
        }
        break;
    }
    return result;
}

/* Points gLobby at the lobby's state, clears a 26-byte block of the
 * lobby's work (no reader seen) and backs the lobby's work up. */
void NetSetup_ResetLobby(void)
{
    gLobby = &gLobbyState;
    memset(D_008FAAA0, 0, sizeof(D_008FAAA0));
    Lobby_BackupWork();
}

/*
 * The set-up's login step (NET_SETUP_LOGIN), by gNetMode.step (enum
 * NetLoginStep): opens the progress boxes, resets the lobby's state and
 * runs the connection to the network (NetMsg_Update), its progress shown
 * in percent; holding start counts gNetCancelHold up, which cancels it.
 * Returns 1 when connected, -1 after the error's message
 * (NetSetup_ShowLoginError), 0 meanwhile.
 */
s32 NetSetup_Login(void)
{
    NetModeState *net;
    s32 netStep;
    s32 result;

    net = &gNetMode;
    result = 0;
    switch (net->step) {
    case NET_LOGIN_OPEN:
        net->noDraw = 0;
        NetMode_SetTitle(NET_TITLE_LOGIN);
        NetMode_OpenProgress(0);
        NetMode_OpenPercent();
        NetMode_OpenIcon2(NET_ICON2_CONNECT);
        net->step++;
        net->subStep = 0;
        net->percent = 0;
        NetSetup_ResetLobby();
        Net_ClearState();
        ServerList_ClearTried();
        break;
    case NET_LOGIN_RUN:
        if (LoginPad_AnyStartHeld() != 0) {
            gNetCancelHold += NET_CANCEL_HOLD_STEP;
        } else {
            gNetCancelHold = 0;
        }
        netStep = NetMsg_Update();
        net->percent = gNetProgress;
        switch (netStep) {
        case NET_STEP_CONNECTED:
            net->step = NET_LOGIN_CLOSE;
            net->subStep = NET_LOGIN_WAIT;
            break;
        case NET_STEP_STOPPED:
            NetMode_CloseBoxes(NET_BOX_PROGRESS);
            net->step = NET_LOGIN_ERROR_WAIT;
            net->subStep = NET_LOGIN_WAIT;
            break;
        }
        break;
    case NET_LOGIN_CLOSE:
        if (net->subStep == 0) {
            net->step++;
            NetMode_CloseBoxes(NET_BOX_PROGRESS);
            NetMode_CloseBoxes(NET_BOX_PERCENT);
            NetMode_CloseBoxes(NET_BOX_ICON2);
        } else {
            net->subStep--;
        }
        break;
    case NET_LOGIN_DONE:
        result = 1;
        break;
    case NET_LOGIN_ERROR_WAIT:
        if (net->subStep == 0) {
            net->step++;
            NetMode_CloseBoxes(NET_BOX_PROGRESS);
            NetMode_OpenIcon2(NET_ICON2_ERROR);
        } else {
            net->subStep--;
        }
        break;
    case NET_LOGIN_ERROR:
        if (NetSetup_ShowLoginError() != 0) {
            NetMode_CloseBoxes(NET_BOX_PERCENT);
            NetMode_CloseBoxes(NET_BOX_ICON2);
            result = -1;
        }
        break;
    }
    return result;
}

/*
 * The set-up's portal step (NET_SETUP_PORTAL), by gNetMode.step (enum
 * KddiPortalStep): opens the browser at the page gPortalPage chooses (the
 * account's two URLs, gAccountData's sessionUrl and nextUrl, else the portal's top
 * page) and runs it. Returns 1 when the browser was left on the way to the
 * lobby (the music then stops), -1 when it failed or the page leads back,
 * 0 meanwhile; a game menu 2 or a login opens the top page again.
 */
s32 LobbyKddi_Portal(void)
{
    NetModeState *net = &gNetMode;
    s32 result;

    result = 0;
    switch (net->step) {
    case KDDI_STEP_START:
        net->step++;
        net->msg = NETMODE_MSG_NONE;
        net->noDraw = NET_NO_DRAW_HOOK | NET_NO_DRAW_SCREEN;
        NetMode_FadeReset();
        LoginWave_Clear();
        break;
    case KDDI_STEP_OPEN:
        memset(gBrowserUrl, 0, BROWSER_URL_LEN);
        switch (gPortalPage) {
        case PORTAL_PAGE_BACK:
            strcpy(gBrowserUrl, gAccountData.sessionUrl);
            gBrowserTopMode = BROWSER_TOP_PORTAL;
            break;
        case PORTAL_PAGE_NEXT:
        case PORTAL_PAGE_QUIT:
            gBrowserTopMode = BROWSER_TOP_LOBBY;
            strcpy(gBrowserUrl, gAccountData.nextUrl);
            break;
        default:
            gPortalPage = PORTAL_PAGE_TOP;
            gBrowserTopMode = BROWSER_TOP_PORTAL;
            strcpy(gBrowserUrl, TXT(0x00847100) /* the portal's top page */);
            break;
        }
open:
        Browser_Init(0);
        net->step++;
        break;
    case KDDI_STEP_BROWSE:
        switch (Browser_Update()) {
        case BROWSER_RUNNING:
            break;
        case BROWSER_DONE:
            if (gPortalPage == PORTAL_PAGE_NEXT || (gPortalPage != PORTAL_PAGE_NEXT && gBrowserState->page == PAGE_LOAD_NONE)) {
                LobbyKddi_CloseBrowser();
                result = -1;
                gPortalPage = PORTAL_PAGE_TOP;
            } else {
                net->step++;
                gPortalPage = PORTAL_PAGE_BACK;
            }
            break;
        case BROWSER_GAME_MENU_2:
            LobbyKddi_CloseBrowser();
            net->step = KDDI_STEP_TOP_AGAIN;
            break;
        case BROWSER_FAILED:
            LobbyKddi_CloseBrowser();
            result = -1;
            gPortalPage = PORTAL_PAGE_TOP;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            break;
        case BROWSER_LOGIN:
            LobbyKddi_CloseBrowser();
            net->step = KDDI_STEP_TOP_AGAIN;
            break;
        }
        break;
    case KDDI_STEP_DONE:
        LobbyKddi_CloseBrowser();
        net->bgmOn = 0;
        Sound_Silence();
        result = 1;
        break;
    case KDDI_STEP_TOP_AGAIN:
        memset(gBrowserUrl, 0, BROWSER_URL_LEN);
        strcpy(gBrowserUrl, TXT(0x00847100) /* the portal's top page */);
        gPortalPage = PORTAL_PAGE_TOP;
        gBrowserTopMode = BROWSER_TOP_PORTAL;
        net->step = KDDI_STEP_OPEN;
        goto open;
    }
    return result;
}

/* The saved set-up's outcome (gNetSaveData.setupResult), or -1 for none;
 * marks the saved set-up taken and yn to run without its screens. */
s32 NetSetup_UseSaved(void)
{
    s32 ret;
    s8 res;

    res = gNetSaveData.setupResult;
    if (res == 1) {
        gNetMode.savedTaken = 1;
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
/* Clears the engine's model, material, bone and primitive buffers and
 * the extra buffer before the lobby's scene. */
void NetMode_ClearBuffers(void)
{
    memset((void *)gObjBufBlocks, 0, 0x20000);
    memset((void *)gMatSets, 0, 53760);
    memset((void *)gMaterials, 0, 0x14000);
    memset(gBones, 0, 0xC8000);
    memset(gModels, 0, 16384);
    memset(gPrims, 0, 20480);
    memset(gSysSettings.extraBuf, 0, 0x14000);
}

/* The set-up's last mode on the way to the lobby (NET_SETUP_LEAVE): the
 * title and the music off, the buffers cleared. Returns 1. */
s32 NetSetup_Leave(void)
{
    NetMode_SetTitle(NET_TITLE_NONE);
    gNetMode.bgmOn = 0;
    Sound_Silence();
    NetMode_ClearBuffers();
    return 1;
}

/*
 * The set-up's disconnect (NET_SETUP_DISCONNECT): fades the screen out,
 * then lets the network go (LobbyNet_DisconnectWait). Returns 1 when done,
 * 0 meanwhile.
 */
s32 NetSetup_Disconnect(void)
{
    NetModeState *net;
    s32 result;

    net = &gNetMode;
    result = 0;
    switch (net->step) {
    case NET_DISC_FADE:
        NetMode_FadeStart(0, NET_DISCONNECT_FADE, 1);
        net->step++;
        break;
    case NET_DISC_FADING:
        if ((s8)NetMode_FadeRunning() == 0) {
            NetMode_SetTitle(NET_TITLE_NONE);
            NetMode_EmptyResetHook();
            net->step++;
            gNetDisconnectStep = 0;
            gNetStepTimeout = 0;
        }
        break;
    case NET_DISC_WAIT:
        if (LobbyNet_DisconnectWait() == 0) {
            return 0;
        }
        net->step++;
    case NET_DISC_DONE:
        result = 1;
        break;
    }
    return result;
}

/* The set-up's quit (NET_SETUP_QUIT): the title off. Returns 1. */
s32 NetSetup_Quit(void)
{
    NetMode_SetTitle(NET_TITLE_NONE);
    return 1;
}

/* Moves the set-up to the online menu (NET_SETUP_SETTINGS) with its frame
 * loaded again, and clears the connection's flags and the portal's page. */
void NetSetup_ToSettings(void)
{
    NetMode_ReleaseTextures(0);
    NetMode_SetMode(NET_SETUP_SETTINGS);
    gNetMode.noDraw = 0;
    gNetMode.subStep = NET_MENU_ENTER_RELOAD;
    gNetMode.entry = 0;
    gNetMode.ynResult = 0;
    gNetMode.netResult = NET_RESULT_NONE;
    gNetMode.savedTaken = 0;
    gNetMode.ynNoDraw = 0;
    gNetMode.pad17 = 0;
    gPortalPage = PORTAL_PAGE_TOP;
}

/* The set-up's check for a saved set-up (NET_SETUP_CHECK_SAVED): 1 when
 * one was saved, -1 when not. */
s32 NetSetup_CheckSaved(void)
{
    NetMode_SetTitle(NET_TITLE_LOGIN);
    if (gNetSaveData.setupResult == LOGIN_SAVED_YES) {
        return 1;
    }
    return -1;
}

/*
 * The message (the network screen's second page) for an error code: the
 * entry of gNetErrorMsgs at the code's place in gNetErrorCodes, a list of
 * negative codes. An entry of 255 takes the next one, and code 127 the one
 * after; a code not in the list takes the entry after the list.
 */
u8 NetMode_ErrorMsg(s32 code)
{
    s16 *codes;
    s16 entry;
    s32 i;

    i = 0;
    codes = gNetErrorCodes;
    entry = *codes;
    while (entry < 0) {
        if (entry == (s16)code) {
            break;
        }
        codes++;
        entry = *codes;
        i++;
    }
    if (gNetErrorMsgs[i] == 255) {
        i++;
    }
    if (code == 127) {
        i++;
    }
    return gNetErrorMsgs[i];
}

/* Restarts the menu icon's animation. */
void NetMode_ResetAnim(void)
{
    gNetMode.animTick = 0;
    gNetMode.animFrame = 0;
}

/*
 * Steps the menu icon's animation: animTick counts 0-3, and each time it
 * wraps animFrame steps 0-2. animFrame is reached as byte 219 of gNetMode
 * through a byte pointer: read by its field, the compiler addresses it in
 * another way than the game's code.
 */
static void NetMode_StepAnim(void)
{
    s8 *bytes;

    bytes = (s8 *)&gNetMode;
    (*(s8 *)&gNetMode.animTick) = ((*(s8 *)&gNetMode.animTick) + 1) & 3;
    if ((*(s8 *)&gNetMode.animTick) == 0) {
        bytes[219]++;
        bytes[219] %= 3;
    }
}

s32 Yn_UpdateProxy(void);
void Yn_LaunchSetupGuide(void);

/*
 * The online menu (NET_SETUP_SETTINGS), by gNetMode.step (enum
 * NetMenuStep): its first list (NET_MENU_ROW_*) connects with the saved
 * set-up, deletes the login ID or opens the network settings' list
 * (NET_SETTINGS_ROW_*), which runs yn's screens, its proxy settings or its
 * set-up guide. The rows without a saved set-up or login ID are skipped.
 * Returns NET_MENU_CONNECT, NET_MENU_YN or NET_MENU_GUIDE as the player
 * chooses, NET_MENU_BACK when backing out, NET_MENU_RUNNING meanwhile.
 * Yn_UpdateProxy's result is switched on with its default case first: in
 * that order the compiler lays the whole function out as the game's code.
 */
s32 NetSetup_SettingsMenu(void)
{
    NetModeState *net = &gNetMode;
    s32 pad = LoginPad_Triggered();
    s32 row;

    switch (gNetMode.step) {
    case NET_MENU_INIT:
        LoadOverlay(OVERLAY_YN);
        net->sel3 = NET_MENU_ROW_SETTINGS;
        net->sel2 = 0;
        net->openW = 0;
        net->openH = 0;
        net->step++;
        if (net->subStep == NET_MENU_ENTER_NEW) {
            NetMode_SetTitle(NET_TITLE_MENU);
            net->subStep = 0;
            if (gNetSaveData.setupResult == LOGIN_SAVED_YES) {
                net->sel3 = NET_MENU_ROW_CONNECT;
            } else {
                net->sel3 = NET_MENU_ROW_SETTINGS;
            }
        }
        if (net->subStep == NET_MENU_ENTER_RELOAD) {
            NetMode_ReloadFrame();
            net->subStep = 0;
        }
        if (net->subStep == NET_MENU_ENTER_SETTINGS) {
            NetMode_SetTitle(NET_TITLE_SETTINGS);
            net->sel3 = NET_MENU_ROW_CONNECT;
            if (gNetSaveData.setupResult == LOGIN_SAVED_YES) {
                net->sel3 = NET_MENU_ROW_CONNECT;
            } else {
                net->sel3 = NET_MENU_ROW_SETTINGS;
            }
            net->sel2 = 0;
            net->step = NET_MENU_SETTINGS;
            net->subStep = 0;
        }
        if (gNetSaveData.setupResult == LOGIN_SAVED_YES) {
            net->innerStep = 1;
        }
        net->noDraw = 0;
        NetMode_OpenIcon5();
        NetMode_FadeReset();
        NetMode_ResetAnim();
        break;
    case NET_MENU_TOP:
        NetMode_StepAnim();
        if ((pad & PAD_UP) || (pad & PAD_DOWN)) {
            row = net->sel3;
            if ((pad & PAD_UP) && row > 0) {
                net->sel3 = row - 1;
            }
            if ((pad & PAD_DOWN) && net->sel3 < NET_MENU_ROW_BACK) {
                net->sel3++;
            }
            if (gNetSaveData.setupResult == 0 && net->sel3 == NET_MENU_ROW_CONNECT) {
                if (pad & PAD_UP) {
                    net->sel3 = NET_MENU_ROW_SETTINGS;
                    LoginMenu_PlayCursorSe();
                    goto select;
                }
                if (gNetSaveData.mmbbLoginId[0] == LOGIN_ID_EMPTY) {
                    net->sel3 = NET_MENU_ROW_BACK;
                } else {
                    net->sel3 = NET_MENU_ROW_ID;
                }
                LoginMenu_PlayCursorSe();
                goto draw;
            }
            if (gNetSaveData.mmbbLoginId[0] == LOGIN_ID_EMPTY && net->sel3 == NET_MENU_ROW_ID) {
                if (pad & PAD_UP) {
                    if (gNetSaveData.setupResult == 0) {
                        net->sel3 = NET_MENU_ROW_SETTINGS;
                    } else {
                        net->sel3 = NET_MENU_ROW_CONNECT;
                    }
                } else {
                    net->sel3 = NET_MENU_ROW_BACK;
                }
            }
            if (row != net->sel3) {
                LoginMenu_PlayCursorSe();
            }
            goto draw;
        }
    select:
        if (net->sel3 == NET_MENU_ROW_ID) {
            row = LoginPad_DecideEx(LOGIN_SE_DECIDE, 1);
        } else {
            row = LoginPad_DecideEx(LOGIN_SE_DECIDE, 0);
        }
        if (row != 0) {
            switch (net->sel3) {
            case NET_MENU_ROW_CONNECT:
                NetMode_SetTitle(NET_TITLE_MENU);
                NetMode_CloseBoxes(NET_BOX_ICON);
                NetSetup_UseSaved();
                return 1;
            case NET_MENU_ROW_SETTINGS:
                NetMode_SetTitle(NET_TITLE_SETTINGS);
                net->sel2 = 0;
                net->step = NET_MENU_SETTINGS;
                NetMode_ResetAnim();
                break;
            case NET_MENU_ROW_ID:
                if (gNetSaveData.mmbbLoginId[0] != LOGIN_ID_EMPTY) {
                    NetMode_CloseBoxes(NET_BOX_ICON);
                    net->step = NET_MENU_DELETE_ID;
                    net->subStep = 0;
                }
                break;
            case NET_MENU_ROW_BACK:
            cancel:
                NetMode_CloseAll();
                return -1;
            }
        }
        if (LoginPad_Cancel() != 0) {
            if (net->sel3 == NET_MENU_ROW_BACK) {
                goto cancel;
            }
            net->sel3 = NET_MENU_ROW_BACK;
        }
    draw:
        NetModeMsg_Queue5(0, NET_LIST_MENU);
        break;
    case NET_MENU_SETTINGS:
        NetMode_StepAnim();
        if ((pad & PAD_DOWN) && net->sel2 < NET_SETTINGS_ROW_BACK) {
            LoginMenu_PlayCursorSe();
            net->sel2++;
        } else if ((pad & PAD_UP) && net->sel2 > 0) {
            LoginMenu_PlayCursorSe();
            net->sel2--;
        } else {
            if (LoginPad_Decide(LOGIN_SE_DECIDE) != 0) {
                switch (net->sel2) {
                case NET_SETTINGS_ROW_YN:
                    NetMode_SetTitle(NET_TITLE_CONNECT);
                    NetSetup_UseSaved();
                    gNetMode.ynNoDraw = 0;
                    return 2;
                case NET_SETTINGS_ROW_GUIDE:
                    net->step = NET_MENU_SETUP_GUIDE;
                    net->subStep = 0;
                    NetMode_CloseBoxes(NET_BOX_ICON);
                    NetMode_OpenYesNoAtNo();
                    goto draw2;
                case NET_SETTINGS_ROW_PROXY:
                    net->step = NET_MENU_PROXY;
                    net->subStep = 0;
                    NetMode_SetTitle(NET_TITLE_PROXY);
                    goto draw2;
                case NET_SETTINGS_ROW_BACK:
                back:
                    NetMode_ResetAnim();
                    NetMode_SetTitle(NET_TITLE_MENU);
                    net->step = NET_MENU_TOP;
                    return 0;
                }
            }
            if (LoginPad_Cancel() != 0) {
                if (net->sel2 == NET_SETTINGS_ROW_BACK) {
                    goto back;
                }
                net->sel2 = NET_SETTINGS_ROW_BACK;
            }
        }
    draw2:
        NetModeMsg_Queue5(0, NET_LIST_SETTINGS);
        break;
    case NET_MENU_PROXY:
        switch (net->subStep) {
        case 0:
            net->subStep++;
            Yn_Init(0);
            break;
        case 1:
            switch (Yn_UpdateProxy()) {
            default:
                net->step = NET_MENU_SETTINGS;
                net->subStep = 0;
                NetMode_SetTitle(NET_TITLE_SETTINGS);
                NetMode_OpenIcon5();
                break;
            case 0:
                break;
            }
            break;
        }
        break;
    case NET_MENU_DELETE_ID:
        switch (NetSetup_DeleteAccount(0)) {
        case 0:
            break;
        case 1:
        case 2:
            NetMode_SetTitle(NET_TITLE_MENU);
            NetMode_OpenIcon5();
            net->step = NET_MENU_TOP;
            net->subStep = 0;
            if (gNetSaveData.mmbbLoginId[0] == LOGIN_ID_EMPTY) {
                if (gNetSaveData.setupResult == 0) {
                    net->sel3 = NET_MENU_ROW_SETTINGS;
                } else {
                    net->sel3 = NET_MENU_ROW_CONNECT;
                }
            }
            break;
        case -1:
            net->subStep = 0;
            break;
        }
        break;
    case NET_MENU_SETUP_GUIDE:
        NetModeMsg_Page1(NETMODE_MSG_ASK_SETUP_GUIDE);
        switch (net->subStep) {
        case 0:
            switch (NetMode_ChooseYesNo()) {
            case 0:
                break;
            case 1:
                FadeA_Start(1);
                net->subStep = 1;
                net->innerStep = NET_GUIDE_FADE;
                break;
            case -1:
                NetMode_CloseYesNo();
                net->step = NET_MENU_SETTINGS;
                net->subStep = 0;
                NetMode_OpenIcon5();
                break;
            }
            break;
        case 1:
            if (--net->innerStep <= 0) {
                Sound_Silence();
                net->subStep = 2;
            }
            break;
        case 2:
            NetMode_CloseYesNo();
            Yn_LaunchSetupGuide();
            return 3;
        }
        break;
    }
    return 0;
}

/* Queues a message of the network screen's fourth page (the login's
 * errors). */
void NetModeMsg_Page4(s32 msg)
{
    NetModeMsg_Queue(NETMODE_QUEUE_FRONT, 4, msg);
}

/*
 * Shows why the login stopped (gNetMsgState.err): the error's message on
 * the fourth page with its second line, until circle is pressed. Returns 1
 * then, 0 before.
 */
s32 NetSetup_ShowLoginError(void)
{
    s32 msg;
    NetModeState *net;
    s32 result;

    net = &gNetMode;
    result = 0;
    switch (net->subStep) {
    case 0:
        net->subStep++;
    case 1:
        msg = 0;
        switch (gNetMsgState.err) {
        case NET_ERR_PPP_LINE:
            msg = 3;
            break;
        case NET_ERR_PPP_FLAG:
            msg = 4;
            break;
        case NET_ERR_PPP_E3:
            msg = 5;
            break;
        case NET_ERR_NO_DEVICE:
            msg = 1;
            break;
        case NET_ERR_FAILED:
            msg = 2;
            break;
        case NET_ERR_IF_OPT8:
            msg = 6;
            break;
        case NET_ERR_DHCP:
            msg = 7;
            break;
        case NET_ERR_PPP_E6:
            msg = 8;
            break;
        case NET_ERR_CANCELED:
            msg = 12;
            break;
        case NET_ERR_PPP_E10:
            msg = 16;
            break;
        case NET_ERR_NO_LINK:
            msg = 15;
            break;
        case NET_ERR_DHCP_TIMEOUT:
            msg = 17;
            break;
        }
        if (msg != 0) {
            if (msg == 12) {    /* the cancel's message: no percentage */
                net->percent = 0;
            }
            NetModeMsg_Page4(msg);
            NetModeMsg_Page0(20);   /* its second line */
        }
        if (LoginPad_Decide(LOGIN_SE_ALERT) != 0) {
            result = 1;
            net->subStep++;
        }
        break;
    }
    return result;
}

/* Frees the network screen's textures and sub_main's lobby set, clears
 * their handles in gNetMode.tex and compacts the memory. */
void LobbyLogin_FreeTextures(void)
{
    s32 i;

    Tex_FreeRange(NET_TEX_SLOT, NET_TEX_SLOTS);
    Tex_FreeRange(NET_LOBBY_TEX_SLOT, NET_LOBBY_TEX_SLOTS);
    for (i = 0; i < NET_TEX_COUNT; i++) {
        gNetMode.tex[i] = 0;
    }
    Mem_Compact();
}

/* Restarts the current mode at its first step. */
void NetMode_ResetStep(void)
{
    gNetMode.step = 0;
}

/*
 * The portal from the lobby's scene: as LobbyKddi_Portal, with the
 * network screen's textures loaded, the music on for the top page, and a
 * failed page sending the next entry back to the first one
 * (LOGIN_ENTRY_FIRST). When the browser is left on the way to the lobby,
 * the textures are freed, the lobby's data overlay is loaded and the music
 * stops. Returns 1 then, -1 on failure, 0 meanwhile.
 */
s32 LobbyKddi_PortalScene(void)
{
    NetModeState *net = &gNetMode;
    s32 result;

    result = 0;
    switch (net->step) {
    case KDDI_STEP_START:
        net->step++;
        NetMode_LoadTextures(0);
        LoginWave_Clear();
        net->msg = NETMODE_MSG_NONE;
        net->noDraw = NET_NO_DRAW_HOOK | NET_NO_DRAW_SCREEN;
        NetMode_FadeReset();
        if (gPortalPage == PORTAL_PAGE_TOP) {
            NetMode_BgmOn();
        }
        break;
    case KDDI_STEP_OPEN:
        memset(gBrowserUrl, 0, BROWSER_URL_LEN);
        switch (gPortalPage) {
        case PORTAL_PAGE_BACK:
            strcpy(gBrowserUrl, gAccountData.sessionUrl);
            gBrowserTopMode = BROWSER_TOP_PORTAL;
            break;
        case PORTAL_PAGE_NEXT:
        case PORTAL_PAGE_QUIT:
            gBrowserTopMode = BROWSER_TOP_LOBBY;
            strcpy(gBrowserUrl, gAccountData.nextUrl);
            break;
        default:
            gPortalPage = PORTAL_PAGE_TOP;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            gBrowserTopMode = BROWSER_TOP_PORTAL;
            strcpy(gBrowserUrl, TXT(0x00847100) /* the portal's top page */);
            break;
        }
open:
        Browser_Init(0);
        net->step++;
        break;
    case KDDI_STEP_BROWSE:
        switch (Browser_Update()) {
        case BROWSER_RUNNING:
            break;
        case BROWSER_DONE:
            if (gPortalPage == PORTAL_PAGE_NEXT || (gPortalPage != PORTAL_PAGE_NEXT && gBrowserState->page == PAGE_LOAD_NONE)) {
                LobbyKddi_CloseBrowser();
                result = -1;
                gPortalPage = PORTAL_PAGE_TOP;
            } else {
                net->step++;
                gPortalPage = PORTAL_PAGE_BACK;
            }
            break;
        case BROWSER_GAME_MENU_2:
            LobbyKddi_CloseBrowser();
            net->step = KDDI_STEP_TOP_AGAIN;
            break;
        case BROWSER_FAILED:
            LobbyKddi_CloseBrowser();
            result = -1;
            gPortalPage = PORTAL_PAGE_TOP;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            break;
        case BROWSER_LOGIN:
            LobbyKddi_CloseBrowser();
            net->step = KDDI_STEP_TOP_AGAIN;
            break;
        }
        break;
    case KDDI_STEP_DONE:
        LobbyKddi_CloseBrowser();
        LobbyLogin_FreeTextures();
        Scene_LoadOverlay(OVERLAY_LBGUILD);
        NetMode_BgmOff();
        result = 1;
        break;
    case KDDI_STEP_TOP_AGAIN:
        NetMode_BgmOn();
        memset(gBrowserUrl, 0, BROWSER_URL_LEN);
        strcpy(gBrowserUrl, TXT(0x00847100) /* the portal's top page */);
        gPortalPage = PORTAL_PAGE_TOP;
        gNetEntryMode = LOGIN_ENTRY_FIRST;
        gBrowserTopMode = BROWSER_TOP_PORTAL;
        net->step = KDDI_STEP_OPEN;
        goto open;
    }
    return result;
}

/*
 * Draws the login screen's frame under the server menu and the area list:
 * its base sprite, then the five sprites of gLoginFrameSprites[1..5]. The
 * first (the cursor's) takes the colour of the server under the menu's
 * cursor, dimmed to 70% on the area list, or none before the cursor is
 * set; the four items are lit (gLoginLitColors) for the area list's page
 * and unlit (gLoginItemColors) otherwise, or black on the server menu. On
 * the area list it then draws the title and the lit item's four
 * highlights: two sprites swelling with the wave around the item's box
 * (gLoginBoxSprites) and two bars down to its middle, each faded by a
 * wave.
 */
void LobbyLogin_DrawMenu(void)
{
    NetSprDef spr;
    LoginWave *anim;
    NetSprDef *tbl;
    u32 *col;
    s32 lit;
    s32 oldZwrite;
    s32 oldBlendOp;
    s32 oldBlend;
    s32 i;
    u32 red;
    u32 green;
    u32 blue;
    LoginBox *box;
    s16 width;
    s16 height;

    anim = (LoginWave *)gSaveBuf;
    NetMode_BindTextures();
    tbl = gLoginFrameSprites;
    if (anim->page == -1) {
        lit = -1;
    } else {
        lit = gLoginPageColorIdx[anim->page];
    }
    oldZwrite = Gs_GetState(RS_ZWRITE);
    oldBlendOp = Gs_GetState(RS_BLEND_OP);
    oldBlend = Gs_GetState(RS_BLEND);
    Gs_SetState(RS_ZWRITE, 0);
    Gs_SetState(RS_BLEND_OP, RF_BLEND_OP_ADD);
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    NetMode_DrawSprites((struct NetSprDef *)tbl, 1, 0);
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    for (i = LOGIN_SPR_CURSOR; i <= LOGIN_SPR_ITEM_LAST; i++) {
        spr = tbl[i];
        if (i == LOGIN_SPR_CURSOR) {
            if (anim->cursor == -1) {
                spr.color = 0;
            } else {
                spr.color = LobbySrv_ColorAt(gLobbyMenu->cursor + gLobbyMenu->page * 8) | 0xFF000000;
                if (anim->screen[LOGIN_WAVE_AREA_LIST] != 0) {
                    red = (spr.color & 0xFF0000) >> 16;
                    green = (spr.color & 0xFF00) >> 8;
                    blue = spr.color & 0xFF;
                    red = 0.7f * red;
                    green = 0.7f * green;
                    blue = 0.7f * blue;
                    spr.color = (spr.color & 0xFF000000) | (red << 16) | (green << 8) | blue;
                }
            }
        } else if (anim->screen[LOGIN_WAVE_SERVER_MENU] != 0) {
            spr.color = 0xFF000000;
        } else if (i - LOGIN_SPR_ITEM_FIRST == lit) {
            spr.color = gLoginLitColors[i - 1];
        } else {
            spr.color = gLoginItemColors[i - 1];
        }
        NetMode_DrawSprites((struct NetSprDef *)&spr, 1, 0);
    }
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    if (anim->screen[LOGIN_WAVE_AREA_LIST] != 0) {
        LobbyTitle_Draw();
        Gs_SetState(RS_BLEND, RS_BLEND_LIGHTEN);
        if (lit != -1) {
            box = &gLoginBoxSprites[lit];
            col = &spr.color;
            spr = tbl[LOGIN_SPR_SWELL];
            spr.x = box->x;
            spr.y = box->y;
            width = spr.w + (s16)(8.0f * anim->wave[0]);
            height = spr.h + (s16)(8.0f * anim->wave[1]);
            spr.x = spr.x + (s32)((f32)(spr.w - width) / 2.0f);
            spr.y = spr.y + (s32)((f32)(spr.h - height) / 2.0f);
            spr.w = width;
            spr.h = height;
            *col = (((u32)(255.0f * anim->wave[4]) & 0xFF) << LOGIN_WAVE_ALPHA_SHIFT) | (*col & 0xFFFFFF);
            NetMode_DrawSprites((struct NetSprDef *)&spr, 1, 0);
            spr = tbl[LOGIN_SPR_SWELL2];
            spr.x = box->x;
            spr.y = box->y;
            width = spr.w + (s16)(6.0f * anim->wave[1]);
            height = spr.h + (s16)(6.0f * anim->wave[0]);
            spr.x = spr.x + (s32)((f32)(spr.w - width) / 2.0f);
            spr.y = spr.y + (s32)((f32)(spr.h - height) / 2.0f);
            spr.w = width;
            spr.h = height;
            *col = (((u32)(255.0f * anim->wave[5]) & 0xFF) << LOGIN_WAVE_ALPHA_SHIFT) | (*col & 0xFFFFFF);
            NetMode_DrawSprites((struct NetSprDef *)&spr, 1, 0);
            spr = tbl[LOGIN_SPR_BAR];
            spr.x = box->x + 20;
            spr.y = 0;
            spr.h = box->y + box->h / 2;
            *col = (((u32)(255.0f * anim->wave[3]) & 0xFF) << LOGIN_WAVE_ALPHA_SHIFT) | (*col & 0xFFFFFF);
            NetMode_DrawSprites((struct NetSprDef *)&spr, 1, 0);
            spr = tbl[LOGIN_SPR_BAR2];
            spr.x = box->x + 10;
            spr.y = 0;
            spr.h = box->y + box->h / 2;
            *col = (((u32)(255.0f * anim->wave[2]) & 0xFF) << LOGIN_WAVE_ALPHA_SHIFT) | (*col & 0xFFFFFF);
            NetMode_DrawSprites((struct NetSprDef *)&spr, 1, 0);
        }
    }
    Gs_SetState(RS_BLEND_OP, oldBlendOp);
    Gs_SetState(RS_BLEND, oldBlend);
    Gs_SetState(RS_ZWRITE, oldZwrite);
}

/* Clears the login wave: no screen marked, its step back to 0. */
void LoginWave_Clear(void)
{
    memset(gSaveBuf, 0, sizeof(LoginWave));
}

/* Marks a screen (LOGIN_WAVE_*) as shown and starts the six angles: from
 * 0 (a quarter turn for [3] and [5]), at the speeds of gLoginWaveValues. */
void LoginWave_Mark(s32 screen)
{
    s32 i;
    LoginWave *anim;

    anim = (LoginWave *)gSaveBuf;
    if (screen == LOGIN_WAVE_SERVER_MENU || screen == LOGIN_WAVE_AREA_LIST) {
        anim->screen[screen] = 1;
        for (i = 0; i < LOGIN_WAVES; i++) {
            anim->angle[i] = 0;
            anim->speed[i] = gLoginWaveValues[i];
            if (i == 3) {
                anim->angle[i] = ANGLE_TURN / 4;
            }
            if (i == 5) {
                anim->angle[i] = ANGLE_TURN / 4;
            }
        }
    }
}

/* The login wave's frame: takes the menu's cursor and the area list's
 * page, and on the area list turns the six angles and takes their waves.
 * Returns 0. */
s32 LoginWave_Update(void)
{
    s16 *page;
    LoginWave *anim;

    page = (s16 *)gMenuWork;
    anim = (LoginWave *)gSaveBuf;
    switch (anim->step) {
    case 0:
        anim->cursor = -1;
        anim->page = -1;
        anim->step++;
    case 1:
        anim->cursor = gLobbyMenu->cursor;
        anim->page = *page;
        if (anim->screen[LOGIN_WAVE_AREA_LIST] != 0) {
            anim->angle[0] += anim->speed[0];
            anim->wave[0] = Math_Sin((6.2831855f * anim->angle[0]) / 65536.0f);
            anim->angle[1] += anim->speed[1];
            anim->wave[1] = Math_Cos((6.2831855f * anim->angle[1]) / 65536.0f);
            anim->angle[2] = (anim->angle[2] + anim->speed[2]) & LOGIN_WAVE_HALF_TURN;
            anim->wave[2] = Math_Sin((6.2831855f * anim->angle[2]) / 65536.0f);
            anim->angle[3] = (anim->angle[3] + anim->speed[3]) & LOGIN_WAVE_HALF_TURN;
            anim->wave[3] = Math_Sin((6.2831855f * anim->angle[3]) / 65536.0f);
            anim->angle[4] = (anim->angle[4] + anim->speed[4]) & LOGIN_WAVE_HALF_TURN;
            anim->wave[4] = Math_Sin((6.2831855f * anim->angle[4]) / 65536.0f);
            anim->angle[5] = (anim->angle[5] + anim->speed[5]) & LOGIN_WAVE_HALF_TURN;
            anim->wave[5] = Math_Sin((6.2831855f * anim->angle[5]) / 65536.0f);
        }
        break;
    }
    return 0;
}

/* Draws the login screen's frame base alone (gLoginFrameSprites[0]), for
 * the lobby's entry screens. */
void LobbyLogin_DrawFrameBase(void)
{
    s32 oldZwrite;
    s32 oldBlendOp;
    s32 oldBlend;

    NetMode_BindTextures();
    oldZwrite = Gs_GetState(RS_ZWRITE);
    oldBlendOp = Gs_GetState(RS_BLEND_OP);
    oldBlend = Gs_GetState(RS_BLEND);
    Gs_SetState(RS_ZWRITE, 0);
    Gs_SetState(RS_BLEND_OP, RF_BLEND_OP_ADD);
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    NetMode_DrawSprites((struct NetSprDef *)gLoginFrameSprites, 1, 0);
    Gs_SetState(RS_BLEND_OP, oldBlendOp);
    Gs_SetState(RS_BLEND, oldBlend);
    Gs_SetState(RS_ZWRITE, oldZwrite);
}

/* The lit colour of the item an area list page lights. */
u32 LobbyLogin_LitColor(s32 page)
{
    return gLoginLitColors[gLoginPageColorIdx[page] + 1];
}

/* The login wave's angle speeds, then NetMode_ErrorMsg's tables. */
u16 gLoginWaveValues[6] = {
#include "lobby/00819738.inc"
};
s16 gNetErrorCodes[40] = {
#include "lobby/00819750.inc"
};
u8 gNetErrorMsgs[48] = {
#include "lobby/008197A0.inc"
};
