/*
 * The lobby's connection to the quest server, and the step that leaves the
 * lobby server.
 *
 * LobbyConn_Run runs one step of gLobbyConnSteps a frame (enum
 * LobbyConnStep): it closes the old socket, connects to the address the
 * quest party gave (6916), waits for the TCP connection, sets the socket's
 * options and answers the quest server's challenge (1031); then it loads
 * gm_sub and starts the quest network (QNetIf_Start), or fails.
 * LobbyNet_DisconnectStep leaves the lobby server, a step a call, and
 * LobbyNet_AbortSocket closes the socket at once.
 */
#include "common.h"
#include "lobby_net.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "lobby_connect.h"
#include "main.h"
#include "net_codes.h"
#include "net_settings.h"
#include "quest.h"

/* prototypes */
/* gm_sub's QNetIf_Start, which the lobby loads before it calls it (LobbyConn_StartQuest).
 * The lobby's symbols can't name it: nethttp, loaded with the lobby, has code at that
 * address. With the definition's u8 count and self, the call compiles differently.
 * The unit's own declaration: */
void func_8FC460(s32 sock, s32 count, s32 self);
extern u8 gNetPartySelf;
/* end of prototypes */

extern u8 gNetConnStep;
extern s8 gNetConnEnd;
extern s32 gNetSock;                    /* the socket (sub_main) */
extern NetTcpStatus gNetTcpStatus;

extern s16 gNetStepTimeout;                  /* frames left before the step fails */
extern u8 gNetScreenStep;
extern s8 gNetDisconnectStep;
extern s8 gNetErrorCode;                   /* the quest server's result byte */
extern s16 gNetConnTries;                  /* tries */
extern void (*gLobbyConnSteps[])(void);      /* the connection's steps */

#define CONN_OPEN_TRIES 3           /* LobbyConn_Open fails after this many tries */
#define CONN_OPEN_TIMEOUT 600       /* frames: 10 s for the TCP connection */
#define CONN_REPLY_TIMEOUT 3600     /* frames: a minute for the server's challenge */
#define NET_CHALLENGE_CMD 0x1031    /* the quest server's challenge, and the answer's command */
#define NET_CHALLENGE_ANSWER_LEN 10 /* the answer's bytes (Net_ChallengeReply) */

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

/*
 * CONN_CHALLENGE: waits (gNetStepTimeout frames at most) for the quest server's
 * challenge (NET_CHALLENGE_CMD) and answers it: a reply of the same command whose
 * NET_CHALLENGE_ANSWER_LEN bytes Net_ChallengeReply makes from the account id and the
 * challenge's sequence number, sent straight on the socket, header and payload in one.
 * The connection goes on to the next step, or to CONN_FAILED.
 */
void LobbyConn_AnswerChallenge(void)
{
    u8 buf[32];
    u8 answer[16];
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

/* The connection's steps, by enum LobbyConnStep (LobbyConn_Run). */
void (*gLobbyConnSteps[8])(void) = {
    LobbyConn_Close, LobbyConn_Open, LobbyConn_WaitOpen, LobbyConn_SetOption,
    LobbyConn_AnswerChallenge, LobbyConn_Next, LobbyConn_StartQuest, LobbyConn_Fail,
};
