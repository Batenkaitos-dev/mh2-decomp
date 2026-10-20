/*
 * The lobby server's connection.
 *
 * LobbyNet_ConnectServer finds the chosen server's "host:port" in the
 * MMBB data (gAccountData), resolves the host and opens the TCP connection
 * (Net_Connect); Lobby_RecvPacket receives the server's messages a part at
 * a time; LobbyNet_DisconnectWait waits for the line to close.
 * LobbyKddi_PortalStep runs the KDDI portal's pages in the browser before
 * the lobby. The network screen's steps (NetScreen_Run) and the
 * connection's steps (NetScreen_ConnectStep) are dispatched through the
 * two function tables at the end, which list the server list's and the
 * message dialog's steps.
 */
#include "common.h"
#include "save.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "lobby_net.h"
#include "lobby_npc.h"
#include "lobby_servers.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "net_tcp.h"
#include "browser.h"
#include "chat_log.h"
#include "lobby_labels.h"
#include "pad.h"
void NetScene_ToState6(void);
s32 NetTcp_Open(NetAddr *dest);
int strncmp(const char *, const char *, unsigned int);

extern u8 gLoginState;
extern u8 gNetScreenStep;

/* The network screen's steps and the connection's steps: function tables
 * (defined at the end of this file). */
extern s32 (*jtbl_008193E0[])(void);
extern s32 (*jtbl_008193D0[])(void);
extern u8 gNetConnStep;

extern u8 gLbsConnectStep;
extern char gLbsHost[0x40];           /* the lobby server's host */
extern u16 gLbsPort;                  /* its port */
extern u16 gLbsResolveTimer;
extern u8 gLoginConnectStep;
extern s8 gLbsConnectSecs;
extern s8 gLbsConnectTimer;
extern MmbbData gAccountData;
extern char gCurServerId[SERVER_ID_LEN];
s32 strncmp(const char *a, const char *b, u32 n);
char *strncpy(char *dst, const char *src, u32 n);
s32 Net_ResolveHost(s8 *st, s16 *timer, s32 *out, s8 *name);
/* LobbyNet_ConnectServer casts the value to s32 before Net_Connect's u16
 * parameter: the compiler then masks it to 16 bits as the game's code does */
u16 Swap16(s32 x);
extern s32 gNetSock;
s32 NetTcp_Abort(s32 sock);
s32 NetTcp_Delete(s32 *sock);
void *memset(void *dst, s32 c, u32 n);

s32 Net_DisconnectStep(s8 *st, s16 *t);
extern s16 gNetStepTimeout;
extern s8 gNetDisconnectWaiting;
extern s8 gNetSpareStep;
extern s8 gNetDisconnectStep;

extern s32 gLobbyRecvGot;
extern s32 gLobbyRecvSize;
extern s8 gLobbyRecvPart;
extern s8 gLobbyRecvHdrDone;

extern u8 gNetEntryMode;

/* Leaves the lobby for the portal's next page (PORTAL_PAGE_QUIT): the
 * next entry is a first one again. */
void NetScene_BackToPortal(void)
{
    gNetEntryMode = LOGIN_ENTRY_FIRST;
    gPortalPage = PORTAL_PAGE_QUIT;
    NetScene_ToState6();
}

/* One frame of closing the line (Net_DisconnectStep): 1 once it is
 * closed, 0 before. gNetDisconnectWaiting and gNetSpareStep are written
 * here only; nothing reads them. */
s32 LobbyNet_DisconnectWait(void)
{
    gNetDisconnectWaiting = 1;
    if (Net_DisconnectStep(&gNetDisconnectStep, &gNetStepTimeout) == 1) {
        gNetSpareStep = 0;
        gNetDisconnectStep = 0;
        return 1;
    }
    return 0;
}

/* Clears the receive state: the next message starts with its header. */
void LobbyRecv_Reset(void)
{
    gLobbyRecvGot = 0;
    gLobbyRecvSize = 0;
    gLobbyRecvPart = 1;
    gLobbyRecvHdrDone = 0;
}

/* Opens a TCP connection to addr and port (NetTcp_Open; openArg is
 * passed on, seen 0) and clears the receive state. The function has no
 * return statement: the game's code returns what v0 still holds,
 * NetTcp_Open's socket (LobbyRecv_Reset leaves v0 alone), and both
 * callers keep it in gNetSock. A port returns NetTcp_Open's value. */
s32 Net_Connect(s32 addr, u16 port, u16 openArg)
{
    NetAddr dest;

    dest.addr = addr;
    dest.port = port;
    dest.openArg = openArg;
    NetTcp_Open(&dest);
    LobbyRecv_Reset();
}


extern NetTcpStatus gNetTcpStatus;
s32 NetTcp_Status(s32 sock, NetTcpStatus *st);
s32 NetTcp_Recv(s32 sock, void *buf, s16 len);
/* The receive state (LobbyRecv_Reset resets it). The part number and the
 * header flag are declared s8 above and read as bytes here, unsigned. */
#define gRecvGot (gLobbyRecvGot)            /* bytes of the payload received */
#define gRecvSize (gLobbyRecvSize)           /* the payload size still to come */
#define gRecvPart (*(u8 *)&gLobbyRecvPart)   /* the part number, from 1 */
#define gRecvHdrDone (*(u8 *)&gLobbyRecvHdrDone) /* 1: the header was received */

#define LOBBY_HDR_RECV_LEN 12   /* the header's bytes on the line (LobbyHdr's first 12) */
#define LOBBY_SEND_SPACE_MIN 0x400 /* nothing is read while the send buffer has no more room than this */

/*
 * Receives one message, a part at a time: the LOBBY_HDR_RECV_LEN-byte
 * header first (once that many bytes wait), then its payload (the
 * header's big-endian size) into data, at most max bytes per part.
 * Returns -1 on an error or a lost connection, 0 while nothing complete
 * came, 1 when the message is complete, 2, 3 ... when data holds a full
 * part of a longer message (the next part starts again at data[0]).
 */
s32 Lobby_RecvPacket(s32 sock, u8 *hdr, u8 *data, u32 max)
{
    s32 n;
    s32 got;

    if (NetTcp_Status(sock, &gNetTcpStatus) < 0) {
        return -1;
    }
    if (gNetTcpStatus.state != NET_TCP_STATE_CONNECTED) {
        return -1;
    }
    if (gNetTcpStatus.sendSpace <= LOBBY_SEND_SPACE_MIN) {
        return -1;
    }
    if (gRecvHdrDone == 0 && gNetTcpStatus.recvLen >= (u32)LOBBY_HDR_RECV_LEN) {
        if (NetTcp_Recv(sock, hdr, LOBBY_HDR_RECV_LEN) != LOBBY_HDR_RECV_LEN) {
            return -1;
        }
        gRecvHdrDone = 1;
        gRecvSize = (u16)(((LobbyHdr *)hdr)->size[0] << 8);
        gRecvSize += ((LobbyHdr *)hdr)->size[1];
        gRecvSize = (u16)gRecvSize;
        gNetTcpStatus.recvLen -= LOBBY_HDR_RECV_LEN;
        gRecvGot = 0;
    }
    if (gRecvHdrDone != 0) {
        n = gRecvSize - gRecvGot;
        if (n == 0) {
            LobbyRecv_Reset();
            return 1;
        }
        if (n > gNetTcpStatus.recvLen) {
            n = gNetTcpStatus.recvLen;
        }
        if (n > max - gRecvGot) {
            n = max - gRecvGot;
        }
        if (0 < n) {
            got = NetTcp_Recv(sock, data + gRecvGot, n);
            if (got < 0) {
                return -1;
            }
            gRecvGot += got;
            if (gRecvSize == gRecvGot) {
                LobbyRecv_Reset();
                return 1;
            }
            if (gRecvGot == max) {
                gRecvGot = 0;
                gRecvSize -= max;
                return ++gRecvPart;
            }
        }
    }
    return 0;
}

#define LBS_CONNECT_SECS_MAX 20 /* the connection is given up after 21 seconds */

/*
 * Connects to the lobby server, one step a frame (gLbsConnectStep).
 * Returns -2 while busy, 0 once connected, -1 on a failure.
 * - Step 0: the address. Of the MMBB data's SERVER_LIST_MAX servers, the one whose id
 *   is the chosen server's (gCurServerId, from the server list) gives its
 *   "host:port"; the host is cut at the colon and the port read as a
 *   decimal number (up to a space).
 * - Step 1: the host's name is resolved (Net_ResolveHost) and the TCP
 *   connection opened (Net_Connect, the port in network byte order).
 * - Step 2: waits for the connection to be made, LBS_CONNECT_SECS_MAX
 *   seconds and one more at most (60 frames a second).
 */
s32 LobbyNet_ConnectServer(s32 unused)
{
    s32 i;
    u8 *rec;
    u8 *cursor;
    s32 addr = 0;

    switch (gLbsConnectStep) {
    case 0:
        gLbsHost[0] = 0;
        gLbsPort = 0;
        gLbsResolveTimer = 0;
        gLoginConnectStep = 0;
        memset(gLbsHost, 0, sizeof(gLbsHost));
        /* rec steps through the MMBB data by one serverRow entry a server,
         * and the entry is read at the field's offset from it: the compiler
         * then adds the offset at each call, as the game's code does. */
        for (i = 0, rec = (u8 *)&gAccountData; i < SERVER_LIST_MAX; i++, rec += sizeof(gAccountData.serverRow[0])) {
            if (strncmp(gCurServerId, ((MmbbData *)rec)->serverRow[0], SERVER_ID_LEN) == 0) {
                strncpy(gLbsHost, gAccountData.serverAddr[i], sizeof(gLbsHost));
                break;
            }
        }
        cursor = (u8 *)gLbsHost;
        for (;;) {
            if (*cursor == ':' || *cursor == 0) {
                break;
            }
            cursor++;
        }
        if (*cursor == ':') {
            *cursor = 0;
            cursor++;
            while (*cursor != ' ' && *cursor != 0) {
                gLbsPort *= 10;
                gLbsPort += *cursor++ - '0';
            }
        }
        gLbsConnectStep = 1;
        return -2;
    case 1:
        switch (Net_ResolveHost((s8 *) &gLoginConnectStep, (s16 *) &gLbsResolveTimer, &addr, (s8 *) gLbsHost)) {
        case 0:
            return -2;
        case 1:
            gLoginConnectStep = 0;
            break;
        case -1:
            gLoginConnectStep = 0;
            return -1;
        }
        if ((gNetSock = Net_Connect(addr, (s32)Swap16(gLbsPort), 0)) < 0) {
            return -1;
        }
        gLbsConnectSecs = 0;
        gLbsConnectStep = 2;
        gLbsConnectTimer = 60;
        return -2;
    case 2:
        gLbsConnectTimer--;
        if (gLbsConnectTimer <= 0) {
            gLbsConnectTimer = 60;
            gLbsConnectSecs++;
            if (gLbsConnectSecs > LBS_CONNECT_SECS_MAX) {
                NetTcp_Abort(gNetSock);
                NetTcp_Delete(&gNetSock);
                gLbsConnectTimer = 0;
                gLbsConnectSecs = 0;
                return -1;
            }
        }
        if (NetTcp_Status(gNetSock, &gNetTcpStatus) < 0) {
            NetTcp_Abort(gNetSock);
            NetTcp_Delete(&gNetSock);
            return -1;
        }
        if (gNetTcpStatus.state != NET_TCP_STATE_CONNECTED) {
            return -2;
        }
        gLbsConnectTimer = 0;
        gLbsConnectSecs = 0;
        gLbsConnectStep = 0;
        return 0;
    }
    return -1;
}

s32 Browser_Update(void);
void Browser_Init(u8 mode);
void LobbyKddi_CloseBrowser(void);
void NetMode_SeqAReset(void);
void FlushCache(s32 kind);
char *strcpy(char *dst, const char *src);
extern char gBrowserUrl[0x100];            /* the browser's URL */

/*
 * The KDDI portal's pages in the browser, before the lobby, one step a
 * frame (gLbsConnectStep). The page opened is gPortalPage's: the MMBB
 * data's f4 URL, its next URL, or the portal's top page. When the browser
 * ends (BROWSER_DONE), it is a cancel from PORTAL_PAGE_NEXT or from the
 * portal's first page, and otherwise the way on to the lobby (gPortalPage
 * becomes PORTAL_PAGE_BACK). The browser's result 2 and BROWSER_LOGIN
 * open the top page again. Returns 1 when the lobby is next, -1 when the
 * portal was left, 0 while it runs.
 */
s32 LobbyKddi_PortalStep(void)
{
    s32 result = 0;

    switch (gLbsConnectStep) {
    case PORTAL_STEP_START:
        gLbsConnectStep++;
        NetMode_SeqAReset();
        gLobbyServers.unk2B20[0] = 0;
        break;
    case PORTAL_STEP_OPEN:
        gLbsConnectStep++;
        FlushCache(0);
        memset(gBrowserUrl, 0, sizeof(gBrowserUrl));
        switch (gPortalPage) {
        case PORTAL_PAGE_BACK:
            strcpy(gBrowserUrl, gAccountData.f4);
            gBrowserTopMode = BROWSER_TOP_PORTAL;
            Browser_Init(0);
            break;
        case PORTAL_PAGE_NEXT:
        case PORTAL_PAGE_QUIT:
            gBrowserTopMode = BROWSER_TOP_LOBBY;
            strcpy(gBrowserUrl, gAccountData.nextUrl);
            Browser_Init(0);
            break;
        default:
            gPortalPage = PORTAL_PAGE_TOP;
            gBrowserTopMode = BROWSER_TOP_PORTAL;
            strcpy(gBrowserUrl, "https://www01.kddi-mmbb.jp/00000080/CRS-top.jsp");
            Browser_Init(0);
        }
        break;
    case PORTAL_STEP_BROWSE:
        switch (Browser_Update()) {
        case BROWSER_RUNNING:
            break;
        case BROWSER_DONE:
            if (gPortalPage == PORTAL_PAGE_NEXT || (gPortalPage != PORTAL_PAGE_NEXT && gBrowserState->page == 0)) {
                LobbyKddi_CloseBrowser();
                result = -1;
                gPortalPage = PORTAL_PAGE_TOP;
            } else {
                gLbsConnectStep++;
                gPortalPage = PORTAL_PAGE_BACK;
            }
            break;
        case 2:
            LobbyKddi_CloseBrowser();
            gPortalPage = PORTAL_PAGE_TOP;
            gLbsConnectStep = PORTAL_STEP_TOP_AGAIN;
            break;
        case BROWSER_FAILED:
            LobbyKddi_CloseBrowser();
            result = -1;
            gPortalPage = PORTAL_PAGE_TOP;
            gNetEntryMode = LOGIN_ENTRY_FIRST;
            break;
        case BROWSER_LOGIN:
            LobbyKddi_CloseBrowser();
            gPortalPage = PORTAL_PAGE_TOP;
            gLbsConnectStep = PORTAL_STEP_TOP_AGAIN;
        }
        break;
    case PORTAL_STEP_TOP_AGAIN:
        gLbsConnectStep = PORTAL_STEP_BROWSE;
        memset(gBrowserUrl, 0, sizeof(gBrowserUrl));
        strcpy(gBrowserUrl, "https://www01.kddi-mmbb.jp/00000080/CRS-top.jsp");
        gBrowserTopMode = BROWSER_TOP_PORTAL;
        Browser_Init(0);
        break;
    case PORTAL_STEP_CLOSE:
        gLbsConnectStep++;
        LobbyKddi_CloseBrowser();
        break;
    case PORTAL_STEP_WAIT:
        gLbsConnectStep = PORTAL_STEP_WAIT2;
        break;
    case PORTAL_STEP_WAIT2:
        gLbsConnectStep = PORTAL_STEP_DONE;
        break;
    case PORTAL_STEP_DONE:
        result = 1;
        break;
    }
    return result;
}

/* The connection's current step (gNetConnStep): LobbyConnect_Reset, then
 * LobbyConnect_Connect. */
s32 NetScreen_ConnectStep(void)
{
    return jtbl_008193D0[gNetConnStep]();
}

/* The connection's step 0: clears the connection's and the screen's steps,
 * then step 1. */
s32 LobbyConnect_Reset(void)
{
    gLbsConnectStep = 0;
    gNetScreenStep = 0;
    gNetConnStep = 1;
    return 0;
}

/* The connection's step 1: connects to the lobby server
 * (LobbyNet_ConnectServer). Returns 1 once connected (the steps start
 * again at 0, and the login goes on to its state 3), -1 on a failure, 0
 * while it runs. */
s32 LobbyConnect_Connect(void)
{
    s32 result = 0;

    switch (LobbyNet_ConnectServer(1)) {
    case -1:
        result = -1;
        break;
    case -2:
        break;
    case 0:
        gNetConnStep = 0;
        gLbsConnectStep = 0;
        gLoginState = 3;
        gNetScreenStep = 0;
        result = 1;
        gNetStepTimeout = 0;
        break;
    }
    return result;
}

/* The network screen's current step (gNetConnStep): NetScreen_Step0 to
 * NetScreen_Step5. */
s32 NetScreen_Run(void)
{
    return jtbl_008193E0[gNetConnStep]();
}

/* The two function tables (this file's .data). Their names are the ones
 * the build's tools look for in a table of jumps; they stay. */
s32 NetScreen_Step0(void);
s32 NetScreen_Step1(void);
s32 NetScreen_Step2(void);
s32 NetScreen_Step3(void);
s32 NetScreen_Step4(void);
s32 NetScreen_Step5(void);
s32 (*jtbl_008193D0[4])(void) = {
    LobbyConnect_Reset, LobbyConnect_Connect, NULL, NULL,
};
s32 (*jtbl_008193E0[8])(void) = {
    NetScreen_Step0, NetScreen_Step1, NetScreen_Step2, NetScreen_Step3,
    NetScreen_Step4, NetScreen_Step5, NULL, NULL,
};
