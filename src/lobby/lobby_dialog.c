/*
 * The lobby's connection to the quest server, its message dialog, its help
 * line and its frame.
 *
 * - NetScreen_Step4 and NetScreen_Step5 are the network screen's last steps
 *   (gNetScreenStep counts within one): the account's portal page in the
 *   browser, then the message that says why the lobby was left.
 * - The connection to the quest server (LobbyConn_*): LobbyConn_Run runs one
 *   step of gLobbyConnSteps a frame (enum LobbyConnStep): it closes the old
 *   socket, connects to the address the quest party gave (6916), waits for
 *   the TCP connection, sets the socket's options and answers the quest
 *   server's challenge (1031); then it loads gm_sub and starts the quest
 *   network (QNetIf_Start), or fails.
 * - The message dialog (LobbyDialog_*, LobbyText_Open): one message of
 *   gLobbyMsgTexts, a waiting text or a countdown, in a box centred on the
 *   screen that grows in over a few frames. Its kind (enum LobbyDialogKind)
 *   adds a choice (yes or no, save or quit, a list), a page count, or makes
 *   the text blink. LobbyDialog_Draw draws it each frame.
 * - The help line at the foot of the screen (LobbyHelp_*), typed out one
 *   character a frame, and the screen's title (LobbyTitle_*).
 * - Lobby_Tick, the online lobby's frame: it receives, runs the idle and
 *   connection timeouts, the friend list's timers, then the current screen
 *   (gLobbyScreens); LobbyMenu_QueueLayers queues the menu's draw layers.
 */
#include "common.h"
#include "lobby_menu.h"
#include "sprite.h"
#include "texture.h"
#include "render_state.h"
#include "prim.h"
#include "lobby_text.h"
#include "em.h"
#include "m2c_macros.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "kbd.h"
#include "pad.h"
#include "lobby_state.h"
#include "menu_task.h"
#include "browser.h"
#include "lobby_labels.h"

/* prototypes */
/* As this file's calls use it, with s16 coordinates and an s32 kind: with the definition's
 * s32 coordinates and s16 kind, LobbyDialog_DrawWin's and LobbyDialog_Draw's calls compile
 * differently. The unit's own declaration: */
void Win_Draw(s16, s16, s16, s16, s32, s32, s32);
s32 NetScreen_Step4(void);
void LobbyConn_Next(void);
void LobbyConn_Fail(void);
s32 LobbyNet_AbortSocket(void);
void Menu_SetYesNo(s8 choice);
void LobbyDialog_Grow(void);
s16 LobbyDialog_GetGrow(void);
void LobbyDialog_SetMember(void *id, void *name);
void LobbyDialog_ClearMember(void);
static void LobbyHelp_Reset(void);
void LobbyHelp_SetBox(s16 x, s16 y, s16 w);
void LobbyDialog_ShowConnMsg(s32 stage);
void LobbyMenu_DrawDialog(MenuTask *task);
void Overlay_LoadGmSub(void);
/* gm_sub's QNetIf_Start, which the lobby loads before it calls it (LobbyConn_StartQuest).
 * The lobby's symbols can't name it: nethttp, loaded with the lobby, has code at that
 * address. With the definition's u8 count and self, the call compiles differently.
 * The unit's own declaration: */
void func_8FC460(s32 sock, s32 count, s32 self);
extern u8 gNetPartySelf;
/* end of prototypes */

s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);
extern LobbyMenu *gLobbyMenu;
extern char *gLobbyTitleTexts[];
extern char *gLobbyTitleText;
void Game_ResetSubsystems(void);
extern u8 gNetConnStep;
extern s8 gNetConnEnd;
s32 NetTcp_Abort(s32 sock);
s32 NetTcp_Delete(s32 *sock);
extern s32 gNetSock;                    /* the socket (sub_main) */
extern NetTcpStatus gNetTcpStatus;
extern s8 gLobbyDialogChoice;
extern u8 gMenuInputLock;
extern s16 gLobbyDialogGrow;
extern char gLobbyDialogMemberId[8];
extern char gLobbyDialogMemberName[0x11];
extern s32 gMsgWinTimer;
extern s32 gMsgWinSubTimer;
extern s16 gMsgWinX;
extern s16 gMsgWinY;
extern s16 gMsgWinW;
void Text_SetSize(s32);
void LobbyDialog_Draw(void);
void LobbyDialog_DrawMember(void);
void LobbyText_Open(s32 msg);
extern s32 gLobbyDialogMsg;
extern u8 gLobbyDialogOpen;

extern s16 gNetStepTimeout;                  /* frames left before the step fails */
extern u8 gNetScreenStep;
extern s8 gNetDisconnectStep;
extern s8 gNetErrorCode;                   /* the quest server's result byte */
s32 LobbyNet_DisconnectWait(void);
u16 LobbyPad_Get(s32 a);
void Text_PrintCodes(s16 max, const char *fmt, ...);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
void Font_SetColor(s32 color);
extern s32 gLobbyDialogWaitText;
extern s32 gLobbyDialogCountdown;
extern s16 gLobbyDialogW;
extern s16 gLobbyDialogH;
extern u8 gLobbyDialogKind;
extern char *gMsgWinText;
extern s16 gNetConnTries;                  /* tries */
extern void (*gLobbyConnSteps[])(void);      /* the connection's steps */
void LobbyNet_GetPartyAddr(s32 *arg0, s16 *arg1);
s32 Net_Connect(s32 addr, u16 port, u16 openArg);
void NetTcp_SetOption(s32 sock);
s32 Game_IsOnline(void);
void LobbyMenu_DrawNameId(s16 y, char *name, char *id);
void NetMode_DrawBigText(char *str, s32 x, s32 y, s32 size);

#define CONN_OPEN_TRIES 3           /* LobbyConn_Open fails after this many tries */
#define CONN_OPEN_TIMEOUT 600       /* frames: 10 s for the TCP connection */
#define CONN_REPLY_TIMEOUT 3600     /* frames: a minute for the server's challenge */
#define NET_CHALLENGE_CMD 0x1031    /* the quest server's challenge, and the answer's command */
#define NET_CHALLENGE_ANSWER_LEN 10 /* the answer's bytes (Net_ChallengeReply) */

/* Where the message box sits: centred on the 640 x 448 screen. */
#define SCREEN_CENTER_X 0x140
#define SCREEN_CENTER_Y 0xE0

/* The dialog's colours. */
#define DIALOG_FRAME_COLOR 0xBDB59F6C   /* the windows' frame (Win_Draw), the underlines */
#define DIALOG_FILL_COLOR 0x404020      /* the message box's fill */
#define CHOICE_BAR_COLOR 0xB0008000     /* the bar under the chosen answer (Menu_DrawGlowBar) */
#define CHOICE_ON_COLOR 0x807F5230      /* the chosen answer's text */
#define CHOICE_OFF_COLOR 0x80404040     /* the other answer's text */
#define TEXT_WHITE 0x807F7F7F

#define DIALOG_STATE_SIZE 0x3C      /* the dialog's state, from gLobbyDialogMsg on (LobbyDialog_Clear) */
#define DIALOG_GROW_FRAMES 3        /* the box's opening (LobbyDialog_Grow) */
#define DIALOG_LARGE_W 0x1F4        /* the box of a waiting text or a countdown */
#define DIALOG_LARGE_H 0x17C
#define DIALOG_LINE_H 20            /* a line of text */
#define DIALOG_MARGIN 0x3C          /* the box's size beyond its text */
#define DIALOG_BLINK_PERIOD 0x28    /* frames: gLobbyDialogBlink's cycle */

/* Texts of the lobby's text tables that the dialog and the name line draw. */
#define TEXT_YES (gLobbyLabelTexts[LOBBY_LABEL_YES])
#define TEXT_NO (gLobbyNoSaveTexts[LOBBY_NO_SAVE_NO])
#define TEXT_SAVE (gLobbyNoSaveTexts[LOBBY_NO_SAVE_SAVE])
#define TEXT_QUIT_NO_SAVE (gLobbyButtonTexts[LOBBY_BUTTON_QUIT_NO_SAVE])
#define TEXT_BACK (gLobbyButtonTexts[LOBBY_BUTTON_BACK])
#define TEXT_NAME_LABEL (gLobbyHelpTexts[LOBBY_HELP_NAME_LABEL])
#define TEXT_ID_LABEL (gLobbyLabelTexts[LOBBY_LABEL_ID])
/* The text that nothing is offered now (DIALOG_PAGES without pages):
 * gLobbyLabelTexts[LOBBY_LABEL_NOTHING_OFFERED] under a name of its own (a
 * linker alias), which the match needs: read through the table, it is loaded
 * into another register. */
extern char *gTextNoneNow;

/* The network screen's step 4: resets the game's subsystems; returns -3. */
s32 NetScreen_Step4(void)
{
    Game_ResetSubsystems();
    return -3;
}

extern u8 gLbsConnectStep;
extern LobbyMenu gLobbyMenuData;        /* the menu state gLobbyMenu points to */
u16 Pad_GetPressedRepeat(s32 port);
void LobbyStream_Stop(void);
void FadeA_Start(s32);
s32 LobbyFade_IsRunning(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void Lobby_DrawBlackLine(void);
void LobbyDialog_DrawPending(MenuTask *task);
void NetMode_SeqAReset(void);
void FlushCache(s32 kind);
extern char gBrowserUrl[0x100];            /* the browser's URL */
extern char gPortalUrlB[];
char *strcpy(char *dst, const char *src);
void Browser_Init(u8 mode);
s32 Browser_Update(void);
void LobbyKddi_CloseBrowser(void);
void NetScene_BackToPortal(void);
s32 NetMode_SeqAStart(s32, s32, s64);
void Text_SetColorIndex(s16 i);
void LobbyMenu_QueueLayers(void);

/*
 * The network screen's step 5, after the lobby was left: fades out, waits for the
 * disconnection, shows the account's portal page (PORTAL_PAGE_NEXT) in the browser when
 * the lobby server's connection had begun, then, after 8 frames, the message that
 * says why the line was cut (no input for too long, or an unstable line), until
 * PAD_CIRCLE. Returns -4 then (the step's end), else 0.
 */
s32 NetScreen_Step5(void)
{
    u16 pad = Pad_GetPressedRepeat(0);
    s32 result = 0;

    switch (gNetScreenStep) {
    case 0:
        gNetScreenStep++;
        LobbyStream_Stop();
        FadeA_Start(1);
        break;
    case 1:
        if (LobbyFade_IsRunning() == 0) {
            if (gLbsConnectStep != 0) {
                gNetScreenStep++;
            } else {
                gNetScreenStep = 7;
            }
            LobbyMenu_SetDraw(0, Lobby_DrawBlackLine, LobbyDialog_DrawPending, 0);
            NetMode_SeqAReset();
            gNetDisconnectStep = 0;
        }
        break;
    case 2:
        gNetScreenStep++;
    case 3:
        gNetScreenStep++;
        FlushCache(0);
        memset(gBrowserUrl, 0, 0x100);
        gPortalPage = PORTAL_PAGE_NEXT;
        gBrowserTopMode = BROWSER_TOP_LOBBY;
        strcpy(gBrowserUrl, gPortalUrlB);
        Browser_Init(0);
        break;
    case 4:
        switch (Browser_Update()) {
        case 0:
            break;
        case -100:
            LobbyKddi_CloseBrowser();
            NetScene_BackToPortal();
            break;
        default:
            LobbyKddi_CloseBrowser();
            gNetScreenStep++;
            break;
        }
        break;
    case 5:
        gNetScreenStep++;
        break;
    case 6:
        gNetScreenStep++;
    case 7:
        if (LobbyNet_DisconnectWait() != 0) {
            gNetScreenStep++;
        }
        break;
    case 8:
        gNetScreenStep = 9;
        gNetStepTimeout = 8;
        NetMode_SeqAStart(0, 0xA, 0);
        if (gLbsConnectStep == 0) {
            LobbyText_Open(LOBBY_TEXT_LINE_UNSTABLE);
        } else {
            LobbyText_Open(LOBBY_TEXT_IDLE_CUT);
        }
        gLobbyMenuData.redraw = 1;
        break;
    case 9:
        gLobbyMenuData.redraw = 1;
        gNetStepTimeout--;
        if (gNetStepTimeout < 0) {
            gNetScreenStep++;
        }
        break;
    case 10:
        gLobbyMenuData.redraw = 1;
        if (pad & PAD_CIRCLE) {
            result = -4;
            gLobbyNetError = 0;
            gPortalPage = PORTAL_PAGE_TOP;
        }
        Text_SetColorIndex(5);
        Font_SetSpacing(0x14, 0x14);
        break;
    }
    LobbyMenu_QueueLayers();
    return result;
}

/* Runs the connection's current step (gLobbyConnSteps[gNetConnStep]); returns 1 once
 * it is connected (CONN_START_QUEST), -1 once it failed (CONN_FAILED), else 0. */
s32 LobbyConn_Run(void)
{
    s32 result;

    if (gNetConnStep == CONN_START_QUEST) {
        result = 1;
    } else if (gNetConnStep == CONN_FAILED) {
        result = -1;
    } else {
        result = 0;
    }
    gLobbyConnSteps[gNetConnStep]();
    return result;
}

/* CONN_CLOSE: closes the old socket. */
void LobbyConn_Close(void)
{
    gNetStepTimeout = 0;
    gNetConnTries = 0;
    gLobbyConnDone = 0;
    gNetConnStep++;
    NetTcp_Abort(gNetSock);
    NetTcp_Delete(&gNetSock);
    memset(&gNetTcpStatus, 0, sizeof(NetTcpStatus));
}

/* CONN_OPEN: when the wait is over, opens a connection to the address the quest party
 * gave (6916, LobbyNet_GetPartyAddr), with CONN_OPEN_TIMEOUT frames for CONN_WAIT_OPEN;
 * after CONN_OPEN_TRIES tries the connection fails. */
void LobbyConn_Open(void)
{
    u16 port;
    s32 addr;

    if (--gNetStepTimeout <= 0) {
        gNetStepTimeout = 0;
        gNetConnTries++;
        if (gNetConnTries > CONN_OPEN_TRIES) {
            gNetStepTimeout = 0;
            gNetConnStep = CONN_FAILED;
            gNetConnTries = 0;
            return;
        }
        LobbyNet_GetPartyAddr(&addr, (s16 *) &port);
        if ((gNetSock = Net_Connect(addr, port, 0)) < 0) {
            gNetStepTimeout = CONN_OPEN_TIMEOUT;
            return;
        }
        gNetConnStep++;
        gNetStepTimeout = CONN_OPEN_TIMEOUT;
    }
}

/* CONN_WAIT_OPEN: waits for the TCP connection; back to CONN_OPEN, after a pause of 10
 * frames, on a failure or when the time is up. */
void LobbyConn_WaitOpen(void)
{
    if (--gNetStepTimeout <= 0) {
        gNetConnStep--;
        gNetStepTimeout = 10;
        NetTcp_Abort(gNetSock);
        NetTcp_Delete(&gNetSock);
        return;
    }
    if (NetTcp_Status(gNetSock, &gNetTcpStatus) < 0) {
        gNetConnStep--;
        gNetStepTimeout += 10;
        NetTcp_Abort(gNetSock);
        NetTcp_Delete(&gNetSock);
        return;
    }
    if (gNetTcpStatus.state == NET_TCP_STATE_CONNECTED) {
        gNetStepTimeout = 0;
        gNetConnTries = 0;
        gNetConnStep++;
    }
}

/* CONN_SET_OPTION: sets the socket's options (NetTcp_SetOption) and gives the server
 * CONN_REPLY_TIMEOUT frames for its challenge. */
void LobbyConn_SetOption(void)
{
    gNetConnStep++;
    NetTcp_SetOption(gNetSock);
    gNetStepTimeout = CONN_REPLY_TIMEOUT;
}

extern LobbyHdr gLobbyRecvHdr;
extern u8 gAccountId[];                   /* the account id (sub_main) */
s32 Lobby_RecvPacket(s32 sock, u8 *hdr, u8 *data, u32 max);
s32 Net_ChallengeReply(char *out, s8 *ch, u16 key);

/*
 * CONN_CHALLENGE: waits (gNetStepTimeout frames at most) for the quest server's
 * challenge (NET_CHALLENGE_CMD) and answers it: a reply of the same command whose
 * NET_CHALLENGE_ANSWER_LEN bytes Net_ChallengeReply makes from the account id and the
 * challenge's sequence number, sent straight on the socket, header and payload in one.
 * The connection goes on to the next step, or to CONN_FAILED.
 */
void LobbyConn_AnswerChallenge(void)
{
    u8 buf[0x20];
    u8 answer[0x10];
    s32 len;

    if (gNetStepTimeout-- < 0) {
        gNetConnStep = CONN_FAILED;
        return;
    }
    switch (Lobby_RecvPacket(gNetSock, (u8 *)&gLobbyRecvHdr, gLobbyRecvData, LOBBY_RECV_MAX)) {
    case 1:
        break;
    case 0:
        return;
    case -1:
        gNetConnStep = CONN_FAILED;
        return;
    }
    switch ((u16)((u16)(gLobbyRecvHdr.cmd[0] << 8) | gLobbyRecvHdr.cmd[1])) {
    case NET_CHALLENGE_CMD:
        Net_ChallengeReply((char *) answer, (s8 *) gAccountId, (u16)((u16)(gLobbyRecvHdr.echo[0] << 8) + gLobbyRecvHdr.echo[1]));
        LobbyPkt_Begin(&gLobbyPkt, NET_CHALLENGE_CMD);
        LobbyPkt_SetFlag(&gLobbyPkt, LOBBY_FLAG_REPLY);
        gLobbyPkt.echo[0] = gLobbyRecvHdr.echo[0];
        gLobbyPkt.echo[1] = gLobbyRecvHdr.echo[1];
        LobbyPkt_PutBytes(&gLobbyPkt, answer, NET_CHALLENGE_ANSWER_LEN);
        LobbyPkt_End(&gLobbyPkt);
        memcpy(buf, &gLobbyPkt.magic, LOBBY_HEADER_SIZE);
        memcpy(buf + LOBBY_HEADER_SIZE, gLobbyPkt.data, gLobbyPkt.len);
        len = gLobbyPkt.len + LOBBY_HEADER_SIZE;
        if (NetTcp_Send(gNetSock, buf, len) != len) {
            gNetConnStep = CONN_FAILED;
            return;
        }
        gNetConnStep++;
        break;
    }
}

/* CONN_NEXT: goes on to CONN_START_QUEST. */
void LobbyConn_Next(void)
{
    gNetConnStep += 1;
}

/* CONN_START_QUEST, connected: loads gm_sub and starts the quest network on the socket
 * with the party's size and the local player's place in it (QNetIf_Start). */
void LobbyConn_StartQuest(void)
{
    gNetConnStep = 0;
    gGmSubLoaded = 0;
    Overlay_LoadGmSub();
    func_8FC460(gNetSock, gLobby->partyCount, gNetPartySelf);
}

/* CONN_FAILED: ends the connection (gLobbyConnDone 1) and starts the steps again. */
void LobbyConn_Fail(void)
{
    gNetConnStep = CONN_CLOSE;
    gNetConnEnd = 3;
    gLobbyConnDone = 1;
}

/*
 * Leaving the lobby server, a step a call through gNetConnStep: waits a frame, then
 * until LobbyNet_DisconnectWait is done, or closes the socket at once when a portal page
 * is to be shown (gPortalPage). Returns 1 then, with the lobby's error (gLobbyNetError)
 * moved to gNetErrorCode; else 0.
 */
s32 LobbyNet_DisconnectStep(void)
{
    s32 result;

    result = 0;
    switch (gNetConnStep) {
    case 0:
        gNetConnStep++;
        break;
    case 1:
        gNetStepTimeout = 1;
        gNetScreenStep = 0;
        gNetConnStep++;
        gNetDisconnectStep = 0;
        break;
    case 2:
        if (--gNetStepTimeout < 0) {
            gNetConnStep++;
        }
        break;
    case 3:
        if (gPortalPage != PORTAL_PAGE_TOP) {
            NetTcp_Abort(gNetSock);
            NetTcp_Delete(&gNetSock);
            result = 1;
            gNetErrorCode = gLobbyNetError;
            gLobbyNetError = 0;
        } else if (LobbyNet_DisconnectWait() != 0) {
            result = 1;
            gNetErrorCode = gLobbyNetError;
            gLobbyNetError = 0;
        }
        break;
    }
    return result;
}

/* Closes the socket at once; returns 2. */
s32 LobbyNet_AbortSocket(void)
{
    NetTcp_Abort(gNetSock);
    NetTcp_Delete(&gNetSock);
    return 2;
}

/* Opens the dialog on a text of its own (the login's information, a tagged text) to
 * wait on: DIALOG_WAIT, in the large box. */
void LobbyDialog_OpenWait(void *text)
{
    gLobbyDialogWaitText = (s32) text;
    gLobbyDialogKind = DIALOG_WAIT;
    gLobbyDialogW = DIALOG_LARGE_W;
    gLobbyDialogH = DIALOG_LARGE_H;
    LobbyDialog_ClearMember();
}

/* As LobbyDialog_OpenWait, with a countdown of countdown frames shown in seconds
 * (DIALOG_COUNTDOWN). */
void LobbyDialog_OpenCountdown(void *text, s32 countdown)
{
    gLobbyDialogCountdown = countdown;
    gLobbyDialogWaitText = (s32) text;
    gLobbyDialogKind = DIALOG_COUNTDOWN;
    gLobbyDialogW = DIALOG_LARGE_W;
    gLobbyDialogH = DIALOG_LARGE_H;
    LobbyDialog_ClearMember();
}

/* Sets the dialog's choice and the menu's yes/no cursor (0 yes). */
void Menu_SetYesNo(s8 choice)
{
    gLobbyDialogChoice = choice;
    gLobbyMenu->yesNo = choice;
}

/* Clears the dialog's state (DIALOG_STATE_SIZE bytes from gLobbyDialogMsg). */
s32 LobbyDialog_Clear(void)
{
    return (s32)memset(&gLobbyDialogMsg, 0, DIALOG_STATE_SIZE);
}

/* Starts the box's opening: it grows in over DIALOG_GROW_FRAMES frames, and the menu
 * takes no input for 9. */
void LobbyDialog_Grow(void)
{
    gLobbyDialogGrow = DIALOG_GROW_FRAMES;
    gMenuInputLock = 9;
}

/* The frames left of the box's opening; negative once it is open. */
s16 LobbyDialog_GetGrow(void)
{
    return gLobbyDialogGrow;
}

/* The user whose id and name the dialog shows under its text (LobbyDialog_DrawMember). */
void LobbyDialog_SetMember(void *id, void *name)
{
    memcpy(&gLobbyDialogMemberId, id, sizeof(gLobbyDialogMemberId));
    memcpy(&gLobbyDialogMemberName, name, sizeof(gLobbyDialogMemberName));
}

/* Clears that user: the dialog shows none. */
void LobbyDialog_ClearMember(void)
{
    memset(&gLobbyDialogMemberId, 0, sizeof(gLobbyDialogMemberId));
    memset(&gLobbyDialogMemberName, 0, sizeof(gLobbyDialogMemberName));
}

extern LobbyMsgDef gLobbyMsgTexts[];
extern char *gLobbyDialogText;
extern s16 gLobbyDialogSpare;           /* cleared when a message opens; nothing reads it */
extern u16 gLobbyDialogBlink;
extern s16 gLobbyDialogX;
extern s16 gLobbyDialogY;
extern s8 gLobbyDialogLines;
extern s8 gLobbyDialogNoLine;
char *strchr(char *s, s32 c);      /* strchr */
s32 Text_DisplayLength(char *s);

/*
 * Opens the dialog on message msg of gLobbyMsgTexts: its kind from the message's flags,
 * the screen dimmed behind it when LOBBY_MSG_DIM is set, no user shown. The box is
 * sized to the text, 10 pixels a byte of its longest line and DIALOG_LINE_H a line
 * (two lines more for an answer below it), and centred.
 */
void LobbyText_Open(s32 msg)
{
    LobbyMsgDef *def;
    char *line;
    char *cur;
    s32 kind;
    s32 n;
    s32 w = 0;
    s32 len;
    s32 boxW;
    s32 boxH;

    gLobbyDialogMsg = msg;
    gLobbyDialogText = gLobbyMsgTexts[msg].text;
    def = &gLobbyMsgTexts[msg];
    if (def->flags & LOBBY_MSG_DIM) {
        gLobbyDialogNoLine = 0;
    } else {
        gLobbyDialogNoLine = 1;
    }
    memset(gLobbyDialogMemberId, 0, sizeof(gLobbyDialogMemberId));
    memset(gLobbyDialogMemberName, 0, sizeof(gLobbyDialogMemberName));
    cur = gLobbyDialogText;
    n = 0;
    kind = def->flags & ~LOBBY_MSG_DIM;
    gLobbyDialogOpen = 1;
    gLobbyDialogKind = kind;
    gLobbyDialogSpare = 0;
    gLobbyDialogBlink = 0;
    line = cur;
    if (*cur != 0) {
        do {
            cur = strchr(cur, '\n');
            n++;
            if (cur != NULL) {
                len = cur - line;
                line = cur;
            } else {
                len = Text_DisplayLength(line);
            }
            if (len < 0) {
                len = -len;
            }
            if (w < len) {
                w = len;
            }
            if (cur == NULL) {
                break;
            }
            cur++;
        } while (*cur != 0);
    }
    switch (gLobbyDialogKind) {
    case DIALOG_YES_NO:
    case DIALOG_SAVE_QUIT:
        gLobbyDialogChoice = 0;
    case DIALOG_OK:
        n += 2;
        break;
    }
    boxW = w * 10 + DIALOG_MARGIN;
    gLobbyDialogLines = n;
    boxH = n * DIALOG_LINE_H + DIALOG_MARGIN;
    gLobbyDialogX = SCREEN_CENTER_X - boxW / 2;
    gLobbyDialogY = SCREEN_CENTER_Y - boxH / 2;
    gLobbyDialogW = boxW;
    gLobbyDialogH = boxH;
}

/* The screen's title (LobbyTitle_Draw): entry idx of gLobbyTitleTexts. */
void LobbyTitle_Set(s32 idx)
{
    gLobbyTitleText = gLobbyTitleTexts[idx];
}

/* Starts the help line's typing again, in its usual place at the foot of the screen. */
static void LobbyHelp_Reset(void)
{
    gMsgWinTimer = 0;
    gMsgWinSubTimer = 0;
    gMsgWinX = 0x39;
    gMsgWinY = 0x16C;
    gMsgWinW = 0x244;
}

/* The help line shows entry i of gLobbyHelpTexts. */
void LobbyHelp_Set(s32 i)
{
    gMsgWinText = gLobbyHelpTexts[i];
    LobbyHelp_Reset();
}

/* The help line shows text. */
void LobbyHelp_SetText(void *text)
{
    gMsgWinText = text;
    LobbyHelp_Reset();
}

/* Moves the help line's window: its text at (x, y), width w. */
void LobbyHelp_SetBox(s16 x, s16 y, s16 w)
{
    gMsgWinX = x;
    gMsgWinY = y;
    gMsgWinW = w;
}

/* A window (Win_Draw's frame style 2) at (x, y), w by h, filled with fillColor, or
 * with a dark grey when that is 0. */
void LobbyDialog_DrawWin(s16 x, s16 y, s16 w, s16 height, s32 fillColor)
{
    if (fillColor != 0) {
        Win_Draw(x, y, w, height, DIALOG_FRAME_COLOR, fillColor, 2);
        return;
    }
    Win_Draw(x, y, w, height, DIALOG_FRAME_COLOR, 0x80202020, 2);
}

s32 Prim_FillGradRect(struct PrimGradRect *);
extern u32 gMsgBoxGradient[4];              /* the message box's gradient */
void Gfx_DrawBlackLineFixed(void);
void Tex_Prepare(s32 n, s32 tex);
void Tex_Bind(s32 a);
void Gfx_SetTexFilter(s32 a);
s32 Gs_SetState(u8, u32);
void TagText_Print(char *text, f32 x, f32 y);
void Lobby_DrawPressButton(void);
void Gfx_DrawBlackLine(s32 a, s32 b, s32 c, s32 d);
void Menu_DrawPageNum(s16 x, s16 y, s16 page, s16 n, u8 alt);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
s16 Lobby_DrawButtonIcon(s16, s16, s32);
void Text_PrintfColored(char *fmt, ...);
void Text_Printf(const char *fmt, ...);
s32 Lobby_Strlen(char *s);
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);
/* gLobbyDialogKind read as a struct member: the compiler then loads it again after
 * each store, as the game's code does. */
typedef struct LobbyMsgKind {
    u8 kind;
} LobbyMsgKind;
#define MSG_KIND (((LobbyMsgKind *)&gLobbyDialogKind)->kind)

/*
 * Draws the dialog of kind gLobbyDialogKind (enum LobbyDialogKind): the dimmed screen,
 * the box with its shadow (smaller while it grows in: 15% of its size a frame left),
 * then, once it is open, its text and its answers with the bar under the chosen one.
 */
void LobbyDialog_Draw(void)
{
    s16 w;
    s16 height;
    s16 x;
    s16 y;
    char *text;
    char *newline;
    PrimGradRect shadow; /* the box's shadow, a gradient */
    char buf[0x80];

    if (MSG_KIND == DIALOG_NONE) {
        return;
    }
    if (++gLobbyDialogBlink >= DIALOG_BLINK_PERIOD) {
        gLobbyDialogBlink = 0;
    }
    if (MSG_KIND == DIALOG_WAIT) {
        Gfx_DrawBlackLineFixed();
    } else if (gLobbyDialogNoLine == 0 || MSG_KIND == DIALOG_DIM) {
        Gfx_DrawBlackLineFixed();
        if (MSG_KIND == DIALOG_DIM) {
            return;
        }
    }
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    Gfx_SetTexFilter(1);
    Gs_SetState(RS_ALPHA_REF, 0);
    shadow.color[0] = gMsgBoxGradient[0];
    shadow.color[1] = gMsgBoxGradient[1];
    shadow.color[2] = gMsgBoxGradient[2];
    shadow.color[3] = gMsgBoxGradient[3];
    switch (MSG_KIND) {
    case DIALOG_CHOICE2:
    case DIALOG_CHOICES:
        {
            s16 boxX;
            s16 boxY;
            s16 boxW;
            s16 boxH;

            if (gLobbyDialogGrow >= 0) {
                boxW = gLobbyDialogW - 0.15f * gLobbyDialogW * gLobbyDialogGrow;
                boxH = gLobbyDialogH - 0.15f * gLobbyDialogH * gLobbyDialogGrow;
            } else {
                boxW = gLobbyDialogW;
                boxH = gLobbyDialogH;
            }
            boxX = SCREEN_CENTER_X - boxW / 2;
            boxY = SCREEN_CENTER_Y - boxH / 2;
            shadow.x0 = boxX - 4;
            shadow.y0 = boxY - 4;
            shadow.x1 = boxX + boxW + 4;
            shadow.y1 = boxY + boxH + 4;
            Prim_FillGradRect(&shadow);
            Win_Draw(boxX, boxY, boxW, boxH, DIALOG_FRAME_COLOR, DIALOG_FILL_COLOR, 7);
        }
        break;
    case DIALOG_PAGES:
        break;
    default:
        if (gLobbyDialogGrow >= 0) {
            w = gLobbyDialogW - 0.15f * gLobbyDialogW * gLobbyDialogGrow;
            height = gLobbyDialogH - 0.15f * gLobbyDialogH * gLobbyDialogGrow;
        } else {
            w = gLobbyDialogW;
            height = gLobbyDialogH;
        }
        x = SCREEN_CENTER_X - w / 2;
        y = SCREEN_CENTER_Y - height / 2;
        shadow.x0 = x - 4;
        shadow.y0 = y - 4;
        shadow.x1 = x + w + 4;
        shadow.y1 = y + height + 4;
        Prim_FillGradRect(&shadow);
        Win_Draw(x, y, w, height, DIALOG_FRAME_COLOR, DIALOG_FILL_COLOR, 1);
        if (MSG_KIND == DIALOG_BLINK && gLobbyDialogBlink >= 0x15) {
            return;
        }
        break;
    }
    if (gLobbyDialogGrow >= 0) {
        gLobbyDialogGrow--;
        return;
    }
    Font_SetSpacing(0x14, 0x14);
    switch (MSG_KIND) {
    case DIALOG_COUNTDOWN:
        TagText_Print((char *)gLobbyDialogWaitText, 100.0f, 60.0f);
        Font_SetColor(0x807F7F28);
        Font_SetSpacing(0x14, 0x14);
        if (--gLobbyDialogCountdown < 0) {
            gLobbyDialogCountdown = 0;
            Lobby_DrawPressButton();
            return;
        }
        Font_SetPos(0x1F4, 0x40);
        Text_PrintfColored("%2d", gLobbyDialogCountdown / 30 + 1);
        return;
    case DIALOG_WAIT:
        TagText_Print((char *)gLobbyDialogWaitText, 100.0f, 60.0f);
        if (gLobby->level0Kicked == 0 && gLobby->level1Kicked == 0 && gLobby->level2Kicked == 0) {
            Lobby_DrawPressButton();
        }
        break;
    case DIALOG_PAGES:
        Gfx_DrawBlackLine(0x46, 0x22, DIALOG_LARGE_W, DIALOG_LARGE_H);
        if (gLobbyMenu->pages != 0) {
            Menu_DrawPageNum(0x19A, 0x176, gLobbyMenu->page, gLobbyMenu->pages, 0);
            TagText_Print((char *)gLobbyDialogWaitText, 120.0f, 60.0f);
        } else {
            text = gTextNoneNow;
            Text_DrawShadow(Text_CenterX(text, SCREEN_CENTER_X, 0x14), 0xA0, 1, 4, text);
        }
        if (gFrameTick & 0x10) {
            Lobby_DrawButtonIcon(0x78, 0x174, 1);
        }
        Font_SetPos(0x96, 0x178);
        Font_SetColor(TEXT_WHITE);
        Font_SetSpacing(0x12, 0x12);
        Text_PrintfColored(TEXT_BACK);
        break;
    default:
        y = gLobbyDialogY + 0x1E;
        text = gLobbyDialogText;
        while (*text != 0) {
            memcpy(buf, text, sizeof(buf));
            newline = strchr(buf, '\n');
            if (newline != NULL) {
                *newline = 0;
            }
            {
                s32 lineX = Text_CenterX(buf, SCREEN_CENTER_X, 0x14);

                Font_SetPos(lineX, y);
            }
            Text_Printf(buf);
            if (newline == NULL) {
                break;
            }
            y += DIALOG_LINE_H;
            text += Lobby_Strlen(buf) + 1;
        }
        switch (MSG_KIND) {
        case DIALOG_YES_NO:
            if (gLobbyDialogChoice == 0) {
                Font_SetColor(CHOICE_ON_COLOR);
                y += 0x28;
                Font_SetPos(0xFA, y);
                Text_PrintfColored("%s", TEXT_YES);
                Font_SetColor(CHOICE_OFF_COLOR);
                Font_SetPos(0x154, y);
                Text_PrintfColored("%s", TEXT_NO);
                Menu_DrawGlowBar(0x10E, y - 2, 0x50, 0x18, CHOICE_BAR_COLOR);
            } else {
                Font_SetColor(CHOICE_OFF_COLOR);
                y += 0x28;
                Font_SetPos(0xFA, y);
                Text_PrintfColored("%s", TEXT_YES);
                Font_SetColor(CHOICE_ON_COLOR);
                Font_SetPos(0x154, y);
                Text_PrintfColored("%s", TEXT_NO);
                Menu_DrawGlowBar(0x172, y - 2, 0x64, 0x18, CHOICE_BAR_COLOR);
            }
            break;
        case DIALOG_OK:
            Font_SetColor(CHOICE_ON_COLOR);
            y += 0x28;
            Font_SetPos(0x12C, y);
            Text_PrintfColored("%s", TEXT_YES);
            Menu_DrawGlowBar(SCREEN_CENTER_X, y - 2, 0x50, 0x18, CHOICE_BAR_COLOR);
            break;
        case DIALOG_CHOICE2:
            y += 0x28;
            if (gLobbyDialogChoice == 0) {
                Menu_DrawGlowBar(SCREEN_CENTER_X, y - 0xA2, 0xF0, 0x18, CHOICE_BAR_COLOR);
            } else {
                Menu_DrawGlowBar(SCREEN_CENTER_X, y - 0x7A, 0xF0, 0x18, CHOICE_BAR_COLOR);
            }
            break;
        case DIALOG_CHOICES:
            y += 0x28;
            Menu_DrawGlowBar(SCREEN_CENTER_X, y - (10 - gLobbyDialogChoice * 2) * DIALOG_LINE_H - 2, 0xF0, 0x18,
                             CHOICE_BAR_COLOR);
            break;
        case DIALOG_SAVE_QUIT:
            if (gLobbyDialogChoice == 0) {
                Font_SetColor(CHOICE_ON_COLOR);
                y += 0x28;
                Font_SetPos(0xC4, y);
                Text_PrintfColored("%s", TEXT_SAVE);
                Font_SetColor(CHOICE_OFF_COLOR);
                Font_SetPos(0x154, y);
                Text_PrintfColored("%s", TEXT_QUIT_NO_SAVE);
                Menu_DrawGlowBar(0xF6, y - 2, 0x8C, 0x18, CHOICE_BAR_COLOR);
            } else {
                Font_SetColor(CHOICE_OFF_COLOR);
                y += 0x28;
                Font_SetPos(0xC4, y);
                Text_PrintfColored("%s", TEXT_SAVE);
                Font_SetColor(CHOICE_ON_COLOR);
                Font_SetPos(0x154, y);
                Text_PrintfColored("%s", TEXT_QUIT_NO_SAVE);
                Menu_DrawGlowBar(0x1A1, y - 2, 0xC2, 0x18, CHOICE_BAR_COLOR);
            }
            break;
        }
        break;
    }
}

/* Online, once the box is open: the name and id of the user the dialog is about
 * (LobbyDialog_SetMember), if any. */
void LobbyDialog_DrawMember(void)
{
    if (Game_IsOnline() != 0 && gLobbyDialogGrow < 0 && gLobbyDialogMemberId[0] != 0) {
        LobbyMenu_DrawNameId(0x98, gLobbyDialogMemberName, gLobbyDialogMemberId);
    }
}

/* Draws the screen's title (LobbyTitle_Set) in large letters, centred at the top. */
void LobbyTitle_Draw(void)
{
    NetMode_DrawBigText(gLobbyTitleText, -1, 0x1C, 0x22);
}

/*
 * Draws the help line: its window and gMsgWinText typed out one character a frame
 * (gMsgWinTimer, the characters shown; -1 all of them, once 0x100 are or on
 * PAD_TRIANGLE).
 */
void LobbyHelp_Draw(void)
{
    u16 pad;

    pad = LobbyPad_Get(0);
    Win_Draw((s16)(gMsgWinX - 0x10), (s16)(gMsgWinY - 8), gMsgWinW, 0x43, DIALOG_FRAME_COLOR, 0x70402020, 0);
    if (gMsgWinTimer >= 0) {
        if (++gMsgWinSubTimer > 0) {
            gMsgWinSubTimer = 0;
            if (++gMsgWinTimer > 0x100) {
                gMsgWinTimer = -1;
            }
        }
    }
    if (pad & PAD_TRIANGLE) {
        gMsgWinTimer = -1;
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetColor(TEXT_WHITE);
    if (gMsgWinText != NULL) {
        Font_SetPos(gMsgWinX, gMsgWinY);
        Text_PrintCodes(gMsgWinTimer, gMsgWinText);
    }
}

s32 LobbyVillage_IsLeaving(void);
s32 LobbyTown_IsLeaving(void);
void Sjis_Widen(char *src, char *dst);
void Text_DrawShadow(s16 x, s16 y, s16 c1, s16 c2, char *s);
void Gfx_DrawRectFrame(s16 x, s16 y, s16 w, s16 h, s32 color);
extern s32 gTownMode;

/*
 * A user's name and id (made full width) after their labels, each underlined, the name
 * at y; not while the lobby, the village or the town is being left, nor in the town's
 * mode 0x16 (a message box up: Town_CheckQuestParty).
 */
void LobbyMenu_DrawNameId(s16 y, char *name, char *id)
{
    char buf[0x40];

    if (LobbyVillage_IsLeaving() == 1 || LobbyTown_IsLeaving() == 1 || gTownMode == TOWN_MODE_ROOM_LEAVE) {
        return;
    }
    if (gLobby->phase != LOBBY_PHASE_EXIT) {
        Font_SetSpacing(0x14, 0x14);
        Sjis_Widen(name, buf);
        Text_DrawShadow(0xEE, y, 1, 4, buf);
        Sjis_Widen(id, buf);
        Text_DrawShadow(0xEE, y + 0x1E, 1, 4, buf);
        Text_DrawShadow(0xB2, y, 1, 4, TEXT_NAME_LABEL);
        Text_DrawShadow(0xB2, y + 0x1E, 1, 4, TEXT_ID_LABEL);
        Gfx_DrawRectFrame(0xB2, y + 0x16, 0x11C, 1, DIALOG_FRAME_COLOR);
        Gfx_DrawRectFrame(0xB2, y + 0x34, 0x11C, 1, DIALOG_FRAME_COLOR);
    }
}

/*
 * Shows the connection's waiting message for stage (1 the quest is being prepared,
 * 2 on the way to the town, 3 the network is being prepared; 0 none), opening it
 * unless it is already up, and draws the dialog.
 */
void LobbyDialog_ShowConnMsg(s32 stage)
{
    switch (stage) {
    case 0:
        break;
    case 1:
        if (gLobbyDialogOpen == 0 || gLobbyDialogMsg != LOBBY_TEXT_QUEST_PREPARING) {
            LobbyText_Open(LOBBY_TEXT_QUEST_PREPARING);
        }
        break;
    case 2:
        if (gLobbyDialogOpen == 0 || gLobbyDialogMsg != LOBBY_TEXT_TO_TOWN) {
            LobbyText_Open(LOBBY_TEXT_TO_TOWN);
        }
        break;
    case 3:
        if ((gLobbyDialogOpen == 0) || (gLobbyDialogMsg != LOBBY_TEXT_NET_PREPARING)) {
            LobbyText_Open(LOBBY_TEXT_NET_PREPARING);
        }
        break;
    }
    LobbyDialog_Draw();
}

s32 Lobby_ReceiveAll(s32 sock);
u16 LobbyPad_Read(s8 port);
s32 Lobby_IsSysBusy(void);
void Lobby_ResetIdleTimer(s32 *timer);
void Lobby_Disconnect(s32);
s32 NetIf_GetError(void);
extern u8 gConnCheckState;
void LobbyTime_Tick(void);
extern s8 gFriendListOpen;
extern u16 gFriendCheckTimers[5];
extern s8 gFriendStatus[];                 /* the friends' statuses (enum FriendStatus): rows of 7 a page,
                                            * though Lobby_Tick's loop clears FRIEND_PAGE_LEN a row */

/* The friend list (gFriendCheckTimers: when a page's timer runs out, its friends' statuses
 * are cleared, to be asked for again). */
#define FRIEND_PAGES 5
#define FRIEND_PAGE_LEN 10
#define FRIEND_MAX 0x33
extern u16 gAreaSearchTimer;
void Chat_ClearBeep(void);
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
            if (NetIf_GetError() != 0 || gConnCheckState != 0) {
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
        } else if (gLobby->linkState == LINK_STATE_DOWN && gConnCheckState == 0) {
            if (pad != 0 || gKbd->usbKey != 0) {
                Lobby_ResetIdleTimer(&gLobby->idleTimer);
            }
            if (gLobby->recvTimer < 0) {
                gConnCheckState = 1;
            } else if (--gLobby->idleTimer < 0) {
                gConnCheckState = 1;
            } else if (gLobby->serverClose == SERVER_CLOSE_CLOSED || gLobby->serverClose == SERVER_CLOSE_SENT_OUT) {
                gConnCheckState = 1;
            } else if (NetIf_GetError() != 0) {
                gConnCheckState = 1;
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
        OT_AddAt(gDrawOtText3, layer, 0, 0x10);
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

/* The file's data: the connection's steps, the message box's shadow and the screens. */
void AreaList_Run(void);
void LobbyEnter_Run(void);
void LobbyEntry_Run(void);
void LobbyExit_Run(void);
void LobbyVillage_Run(void);
void LobbyRoom_Run(void);
void LobbyRoom_RunClose(void);
void LobbyTown_Run(void);
void (*gLobbyConnSteps[8])(void) = {
    LobbyConn_Close, LobbyConn_Open, LobbyConn_WaitOpen, LobbyConn_SetOption,
    LobbyConn_AnswerChallenge, LobbyConn_Next, LobbyConn_StartQuest, LobbyConn_Fail,
};
u32 gMsgBoxGradient[4] = {
#include "lobby/00819420.inc"
};
void (*gLobbyScreens[8])(void) = {
    LobbyEntry_Run, AreaList_Run, LobbyVillage_Run, LobbyTown_Run,
    LobbyRoom_Run, LobbyExit_Run, LobbyRoom_RunClose, LobbyEnter_Run,
};
