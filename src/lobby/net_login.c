/*
 * The login's commands, from the server's checks to the account's
 * characters, and the hunter cards. Network code: it stays byte for
 * byte, as the whole build does.
 * - The login task (NET_TASK_LOGIN) is started elsewhere; the server then
 *   drives it. Its requests and notifications here answer the server's
 *   challenge (6101), send the client's information (6103, with the
 *   average round trip of four pings, 600E), take the login notice (6105)
 *   and web page (68B0), the account's characters (6131) and the chosen
 *   character's id (6132), and end the login (6104). Each tells the task's
 *   onDone how far the login got (NetMsg.flag, enum NetProgress).
 * - Requests the menus make while logged in: the login information
 *   (614C), the text of the character choice (614D), the timeouts of the
 *   quest network (61A0, 61A1), and the server's memory check (614A).
 * - The hunter cards: the player's card sent whole (6190) or in part
 *   (6192, a range of its bytes), another user's card received (6191),
 *   a range of a user's card fetched (6712) or received (6193).
 */
#include "common.h"
#include "lobby_net.h"
#include "lobby_globals.h"

s32 LobbyNet_GetMyCharaId(char *dst);
s32 LobbyNet_GetMyCharaName(char *dst);
void LobbyPkt_ClientInfo(void);
s32 LobbyPkt_HunterCard(void *src, u16 n);
s32 LobbyPkt_Ping(void);
s32 NetLogin_ReadCharas(void);
s32 LobbyReply_CharaSelectRead(void);
void LobbyPkt_QnetTimeouts(void);
void LobbyMsg_ReadErrText(void);
s32 LobbyPkt_HunterCardPart(void *src, u16 n, u16 offset);
u8 * LobbyMsg_GetString(void *dst, u8 *src);
u8 * LobbyMsg_GetU32(void *dst, u8 *src);
void LobbyNotify_LoginDone(void);
s32 LobbyNet_SendLoginNotice(u8 arg0);
s32 LobbyNet_SendLoginReady(void);
s32 LobbyPkt_LoginInfo(void);
void LobbySrvReq_6189(void);
void LobbySrvReq_6138(void);
void LobbyPkt_CharaSelect(void);
void LobbyPkt_LoginParams(void);
s32 LobbyNet_GetCharaSelectText(s32 *arg0);
void LobbyReply_CharaSelectText(void);
s32 LobbyPkt_CharaSelectText(void);
void LobbyNotify_ServerIdRange(void);
void Lobby_RouteMessage(s32 a);
s32 Net_ChallengeReply(char *out, s8 *ch, u16 key);
void NetLogin_Done(void);
extern LobbyHdr gLobbyRecvHdr;
extern u16 gSrvSyncDelay;
s32 LobbyPkt_UserCardFetch(void *id, u16 size, u16 offset);
s32 LobbyPkt_QuestRecord(void *arg0, s32 arg1);
void LobbyPkt_LoginNotice(u8 arg0);
void LobbyPkt_LoginReady(void);
void LobbyPkt_6138(void);
extern char gServerIdMin[];                   /* 12 digits and a NUL */
extern char gServerIdMax[];

/* the reply to 600E (a ping, four times): the frames each took, summed x16,
 * then a quarter of the sum goes out with 6103's reply; then 61A1 */
void LobbyReply_Ping(void)
{
    gLobbyNet.rttSum += (gLobbyNet.frames * 0x10) & 0xFFFF;
    gLobbyNet.rttCount += 1;
    if ((s32)gLobbyNet.rttCount < 4) {
        LobbyPkt_Ping();
        return;
    }
    gLobbyNet.clientInfo.state[4] = (s16)((u32)gLobbyNet.rttSum >> 2);
    LobbyPkt_ClientInfo();
    LobbyPkt_QnetTimeouts();
}

/* the notification 6131: the account's characters (task 0's onDone: kind 0, 1) */
void LobbyNotify_CharaList(void)
{
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        NetLogin_ReadCharas();
        msg.kind = 0;
        msg.flag = NET_PROGRESS_CHARAS;
        gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
    }
}

/* the reply to 6132: the id the server gave the character (onDone: kind 0, 2;
 * an error: kind -1, 7) */
void LobbyReply_CharaSelect(void)
{
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        if (gLobbyNet.replyErr == 0) {
            LobbyReply_CharaSelectRead();
            LobbyNet_GetMyCharaId(gLobbyNet.self.id);
            LobbyNet_GetMyCharaName(gLobbyNet.self.name);
            msg.kind = 0;
            msg.flag = NET_PROGRESS_CHARA_CHOSEN;
            gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
        } else {
            LobbyMsg_ReadErrText();
            msg.kind = -1;
            msg.flag = NET_PROGRESS_CHARA_FAILED;
            gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
        }
    }
}

/* 6190: a blob */
s32 LobbyReq_HunterCard(void *src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_HunterCard(src, n);
        return i;
    }
    return -1;
}

/* the reply to 6190 */
void LobbyReply_HunterCard(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Copies the card 6191 brought (gLobbyNet.userCard); returns 0. */
s32 LobbyNet_GetUserCard(NetUser *dst)
{
    *dst = gLobbyNet.userCard;
    return 0;
}

/* the notification 6191: blob id (8), blob text (0x100); route 0x2C */
void LobbyNotify_UserCard(void)
{
    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.userCard, 0, sizeof(gLobbyNet.userCard));
        LobbyMsg_GetBlob(gLobbyNet.userCard.text, sizeof(gLobbyNet.userCard.text),
                         LobbyMsg_GetBlob(gLobbyNet.userCard.id, sizeof(gLobbyNet.userCard.id), gLobbyRecvData));
    }
    Lobby_RouteMessage(NET_ROUTE_USER_CARD);
}

/*
 * Sends 6192 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_HunterCardPart(void *src, u16 n, u16 offset, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_HunterCardPart(src, n, offset);
        return i;
    }
    return -1;
}

/* the reply to 6192 */
void LobbyReply_HunterCardPart(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Copies the card part 6193 brought (gLobbyNet.userCardPart); returns 0. */
s32 LobbyNet_GetUserCardPart(NetUserCardPart *dst)
{
    *dst = gLobbyNet.userCardPart;
    return 0;
}

/* the notification 6193: blob id (8), u16 offset, u16 n, n bytes: a range
 * of a user's card changed */
void LobbyNotify_UserCardPart(void)
{
    NetUserCardPart *card;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.userCardPart, 0, sizeof(gLobbyNet.userCardPart));
        card = &gLobbyNet.userCardPart;
        cur = LobbyMsg_GetBlob(card->id, sizeof(card->id), gLobbyRecvData);
        cur = LobbyMsg_GetU16(&card->offset, cur);
        cur = LobbyMsg_GetU16(&card->n, cur);
        LobbyMsg_GetBytes(card->data, cur, card->n);
    }
    Lobby_RouteMessage(NET_ROUTE_USER_CARD_PART);
}

/* 6192: u16 offset, blob */
s32 LobbyPkt_HunterCardPart(void *src, u16 n, u16 offset)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_HUNTER_CARD_PART) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, offset);
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The notification 6104: ends the login task (NetLogin_Done) while it runs. */
void LobbyNotify_LoginDone(void)
{
    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        NetLogin_Done();
    }
}

/*
 * The server's request 6105 at login: a text sent in parts of up to
 * LOBBY_RECV_MAX bytes (gLobbyNet.recvResult is 2 or more while more
 * follow, 1 for the last), gathered in gLobbyNet.rxBuf. Once whole: u8,
 * u16, string, string into gLobbyNet.loginNotice, the second string over
 * the first; then onDone gets NET_PROGRESS_NOTICE. The part's size is
 * built in n over three statements: written as one expression, the
 * compiler computes it with other registers than the game's code.
 */
void LobbySrvReq_LoginNotice(void)
{
    NetLoginNotice *notice;
    u16 n;
    s32 len;
    u8 *cur;
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        if (gLobbyNet.recvResult >= 2) {
            n = LOBBY_RECV_MAX;
        } else {
            n = gLobbyRecvHdr.size[0] << 8;
            n += gLobbyRecvHdr.size[1];
            n -= gLobbyNet.rxLen;
        }
        if ((u32)(gLobbyNet.rxSize - gLobbyNet.rxLen) <= n) {
            n = gLobbyNet.rxSize - gLobbyNet.rxLen - 1;
        }
        len = n;
        memcpy(gLobbyNet.rxBuf + gLobbyNet.rxLen, gLobbyRecvData, len);
        gLobbyNet.rxLen += len;
        if (gLobbyNet.recvResult == 1) {
            notice = &gLobbyNet.loginNotice;
            cur = LobbyMsg_GetU8(&notice->unk0, gLobbyNet.rxStore);
            cur = LobbyMsg_GetU16(&notice->unk2, cur);
            cur = LobbyMsg_GetString(notice->text, cur);
            LobbyMsg_GetString(notice->text, cur);
            msg.kind = 0;
            msg.flag = NET_PROGRESS_NOTICE;
            gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
        }
    }
}

/* Copies the login notice (6105). */
void LobbyNet_GetLoginNotice(NetLoginNotice *dst)
{
    *dst = gLobbyNet.loginNotice;
}

/* Answers the login notice with value (6105's answer); returns 0. */
s32 LobbyNet_SendLoginNotice(u8 value)
{
    LobbyPkt_LoginNotice(value);
    return 0;
}

/* Sends 6105's answer: one byte. */
void LobbyPkt_LoginNotice(u8 value)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LOGIN_NOTICE_ANSWER);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* Sends 6141, the client ready after the login; returns 0. */
s32 LobbyNet_SendLoginReady(void)
{
    LobbyPkt_LoginReady();
    return 0;
}

/* The reply to 6143: completes its request. */
void LobbyReply_6143(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 614C, the login information, after resetting the buffer its
 * reply is gathered in (the game's code resets it twice).
 */
s32 LobbyReq_LoginInfo(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    gLobbyNet.rxLen = 0;
    gLobbyNet.rxSize = 0x1000;
    gLobbyNet.rxBuf = gLobbyNet.rxStore;
    memset(&gLobbyNet.loginInfo, 0, sizeof(gLobbyNet.loginInfo));
    memset(gLobbyNet.rxStore, 0, sizeof(gLobbyNet.rxStore));
    gLobbyNet.rxLen = 0;
    gLobbyNet.rxSize = 0x1000;
    gLobbyNet.rxBuf = gLobbyNet.rxStore;
    memset(&gLobbyNet.loginInfo, 0, sizeof(gLobbyNet.loginInfo));
    memset(gLobbyNet.rxStore, 0, sizeof(gLobbyNet.rxStore));
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_LoginInfo();
        return i;
    }
    return -1;
}

/* Copies the login information (614C's reply); returns 0. */
s32 LobbyNet_GetLoginInfo(NetLoginInfo *dst)
{
    *dst = gLobbyNet.loginInfo;
    return 0;
}

/* Sends 614C; returns its sequence number. */
s32 LobbyPkt_LoginInfo(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LOGIN_INFO) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 614C: in parts, as 6105; once whole, u8, string into
 * gLobbyNet.loginInfo. */
void LobbyReply_LoginInfo(void)
{
    NetLoginInfo *info;
    u16 n;
    s32 len;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        if (gLobbyNet.recvResult >= 2) {
            n = LOBBY_RECV_MAX;
        } else {
            n = gLobbyRecvHdr.size[0] << 8;
            n += gLobbyRecvHdr.size[1];
            n -= gLobbyNet.rxLen;
        }
        if ((u32)(gLobbyNet.rxSize - gLobbyNet.rxLen) <= n) {
            n = gLobbyNet.rxSize - gLobbyNet.rxLen - 1;
        }
        len = n;
        memcpy(gLobbyNet.rxBuf + gLobbyNet.rxLen, gLobbyRecvData, len);
        gLobbyNet.rxLen += len;
        if (gLobbyNet.recvResult != 1) {
            return;
        }
        info = &gLobbyNet.loginInfo;
        cur = LobbyMsg_GetU8(&info->unk0, gLobbyNet.rxStore);
        LobbyMsg_GetString(info->text, cur);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Ends the login task: onDone gets NET_PROGRESS_LOGIN_DONE. */
void NetLogin_Done(void)
{
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        msg.kind = 0;
        gLobbyNet.task[NET_TASK_LOGIN].active = 0;
        msg.flag = NET_PROGRESS_LOGIN_DONE;
        gLobbyNet.task[NET_TASK_LOGIN].state = NET_TASK_RUNNING;
        gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
    }
}

/* The server's request 6189: nothing to do. */
void LobbySrvReq_6189(void)
{

}

/* The server's request 6138: answered while the login runs. */
void LobbySrvReq_6138(void)
{
    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        LobbyPkt_6138();
    }
}

/* Sends 6139, the quest record (n bytes at src); onReply gets the reply. */
s32 LobbyReq_QuestRecord(void *src, s32 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_QuestRecord(src, n);
        return i;
    }
    return -1;
}

/* The reply to 6139: completes its request. */
void LobbyReply_QuestRecord(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Answers the server's challenge 6101: 10 bytes Net_ChallengeReply makes
 * from the login id and the key in the request's header, then the
 * password.
 */
void LobbyPkt_LoginAuth(void)
{
    u8 reply[0x10];

    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LOGIN_AUTH_ANSWER);
    Net_ChallengeReply((char *) reply, (s8 *) gLobbyNet.login.id, (u16)((u16)(gLobbyPkt.echo[0] << 8) + gLobbyPkt.echo[1]));
    LobbyPkt_PutU16(&gLobbyPkt, 10);
    LobbyPkt_PutBytes(&gLobbyPkt, reply, 10);
    LobbyPkt_PutBlob2(&gLobbyPkt, gLobbyNet.login.pass, strlen(gLobbyNet.login.pass));
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* Answers the server's request 6103 with gLobbyNet.clientInfo. */
void LobbyPkt_ClientInfo(void)
{
    NetClientInfo *info = &gLobbyNet.clientInfo;

    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CLIENT_INFO_ANSWER);
    LobbyPkt_PutU8(&gLobbyPkt, info->unk0);
    LobbyPkt_PutU8(&gLobbyPkt, info->unk1);
    LobbyPkt_PutU8(&gLobbyPkt, info->unk2);
    LobbyPkt_PutBlob(&gLobbyPkt, info->version, 10);
    LobbyPkt_PutU16(&gLobbyPkt, info->state[0]);
    LobbyPkt_PutU16(&gLobbyPkt, info->state[1]);
    LobbyPkt_PutU16(&gLobbyPkt, info->state[2]);
    LobbyPkt_PutU16(&gLobbyPkt, info->state[3]);
    LobbyPkt_PutU16(&gLobbyPkt, info->state[4]);
    LobbyPkt_PutU16(&gLobbyPkt, info->state[5]);
    LobbyPkt_PutU16(&gLobbyPkt, info->state[6]);
    LobbyPkt_PutU16(&gLobbyPkt, info->state[7]);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* Sends 6132: the chosen character's id and name (gLobbyNet.charas[3]). */
void LobbyPkt_CharaSelect(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CHARA_SELECT);
    LobbyPkt_PutBlob(&gLobbyPkt, gLobbyNet.charas[3].id, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, gLobbyNet.charas[3].name, strlen(gLobbyNet.charas[3].name) & 0xFFFF);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* Sends 6141. */
void LobbyPkt_LoginReady(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LOGIN_READY);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* Sends 6190; returns its sequence number. */
s32 LobbyPkt_HunterCard(void *src, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_HUNTER_CARD);
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* Sends 600E; returns its sequence number. */
s32 LobbyPkt_Ping(void)
{
    s32 seq;

    gLobbyNet.frames = 0;
    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_PING) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, "0", 1);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the notification 6131's payload: u8 n (at most 3), then n characters
 * (blob id 8, blob name 0x10, blob text 0x100) into gLobbyNet.charas */
s32 NetLogin_ReadCharas(void)
{
    u8 n;
    u8 *cur;
    s32 i;

    memset(gLobbyNet.charas, 0, sizeof(gLobbyNet.charas));
    cur = LobbyMsg_GetU8(&n, gLobbyRecvData);
    if (n > 3) {
        n = 3;
    }
    gLobbyNet.nCharas = n;
    for (i = 0; i < n; i++) {
        cur = LobbyMsg_GetBlob(gLobbyNet.charas[i].id, 8, cur);
        cur = LobbyMsg_GetBlob(gLobbyNet.charas[i].name, 0x10, cur);
        cur = LobbyMsg_GetBlob(gLobbyNet.charas[i].text, 0x100, cur);
    }
    return 0;
}

/* Reads 6132's reply: the chosen character's id. */
s32 LobbyReply_CharaSelectRead(void)
{
    LobbyMsg_GetBlob(gLobbyNet.charas[3].id, 8, gLobbyRecvData);
    return 0;
}

/* Sends 61A0 with a byte of the client's information. */
void LobbyPkt_LoginParams(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LOGIN_PARAMS);
    LobbyPkt_PutU8(&gLobbyPkt, gLobbyNet.clientInfo.unk2);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* The reply to 61A0: two u32 into gLobbyNet.loginParams, then onDone gets
 * NET_PROGRESS_LOGIN_PARAMS; an error: its text, and
 * NET_ROUTE_SERVER_CLOSE. */
void LobbyReply_LoginParams(void)
{
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        if (gLobbyNet.replyErr == 0) {
            LobbyMsg_GetU32(&gLobbyNet.loginParams[1], LobbyMsg_GetU32(&gLobbyNet.loginParams[0], gLobbyRecvData));
            msg.kind = 0;
            msg.flag = NET_PROGRESS_LOGIN_PARAMS;
            gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
        } else {
            LobbyMsg_ReadErrText();
            Lobby_RouteMessage(NET_ROUTE_SERVER_CLOSE);
        }
    }
}

/* Sends 61A1. */
void LobbyPkt_QnetTimeouts(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_QNET_TIMEOUTS);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/*
 * The reply to 61A1: the quest network's timeouts, two u32 in frames
 * (kept in seconds, as gSrvQNetMaxStall and gSrvQNetPeerTimeout, when
 * not 0), then a u16 into gSrvSyncDelay.
 */
void LobbyReply_QnetTimeouts(void)
{
    u32 maxStall = 0;
    u32 peerTimeout = 0;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0 && gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&gSrvSyncDelay, LobbyMsg_GetU32(&peerTimeout, LobbyMsg_GetU32(&maxStall, gLobbyRecvData)));
        if (maxStall != 0) {
            gSrvQNetMaxStall = maxStall / 60;
        }
        if (peerTimeout != 0) {
            gSrvQNetPeerTimeout = peerTimeout / 60;
        }
    }
}

/* 614D: the reply's string goes to dst */
s32 LobbyReq_CharaSelectText(char *dst, s32 max, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.x614DDst = dst;
        gLobbyNet.charaSelectTextMax = max;
        gLobbyNet.charaSelectText = 0;
        gLobbyNet.req[i].seq = LobbyPkt_CharaSelectText();
        return i;
    }
    return -1;
}

/* The u16 614D's reply began with; returns 0. */
s32 LobbyNet_GetCharaSelectText(s32 *dst)
{
    *dst = gLobbyNet.charaSelectText;
    return 0;
}

/* The reply to 614D: a u16, and the same bytes read again as a string
 * into the caller's buffer. */
void LobbyReply_CharaSelectText(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&gLobbyNet.charaSelectText, gLobbyRecvData);
        LobbyMsg_GetString(gLobbyNet.x614DDst, gLobbyRecvData);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Sends 614D; returns its sequence number. */
s32 LobbyPkt_CharaSelectText(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CHARA_SELECT_TEXT) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* Sends 6712: size bytes from offset of user id's card (the menus fetch
 * the 0xD8 bytes of a HunterCard from 0). */
s32 LobbyReq_UserCardFetch(void *id, u16 size, u16 offset, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_UserCardFetch(id, size, offset);
        return i;
    }
    return -1;
}

/* the reply to 6712: blob id (8), blob name (0x10), u16 offset, u16 n, n bytes */
void LobbyReply_UserCardFetch(void)
{
    NetUserCardPart *card;
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.userCardFetch, 0, sizeof(gLobbyNet.userCardFetch));
        card = &gLobbyNet.userCardFetch;
        cur = LobbyMsg_GetBlob(card->id, sizeof(card->id), gLobbyRecvData);
        cur = LobbyMsg_GetBlob(card->name, 0x10, cur);
        cur = LobbyMsg_GetU16(&card->offset, cur);
        cur = LobbyMsg_GetU16(&card->n, cur);
        LobbyMsg_GetBytes(card->data, cur, card->n);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Copies what 6712's reply brought; returns 0. */
s32 LobbyNet_GetUserCardFetch(NetUserCardPart *dst)
{
    *dst = gLobbyNet.userCardFetch;
    return 0;
}

/* Sends 6712; returns its sequence number. */
s32 LobbyPkt_UserCardFetch(void *id, u16 size, u16 offset)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_USER_CARD_FETCH);
    LobbyPkt_PutBlob(&gLobbyPkt, id, 6);
    LobbyPkt_PutU16(&gLobbyPkt, offset);
    LobbyPkt_PutU16(&gLobbyPkt, size);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The notification 68B0: blob url (0x100), u16 secs, blob text (0x300) if secs, into
 * gNetLoginWebPage; onDone gets NET_PROGRESS_WEB_PAGE. */
void LobbyNotify_LoginWebPage(void)
{
    NetLoginWebPage *page;
    u8 *cur;
    NetMsg msg;

    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        memset(&gNetLoginWebPage, 0, sizeof(gNetLoginWebPage));
        memset(&msg, 0, sizeof(msg));
        /* through a byte pointer, as the game's code has it: with the plain
         * address, the compiler reloads the global's address at each use
         * instead of keeping page in a register */
        page = (NetLoginWebPage *)(u8 *)&gNetLoginWebPage;
        cur = LobbyMsg_GetBlob(page->url, 0x100, gLobbyRecvData);
        cur = LobbyMsg_GetU16(&page->secs, cur);
        if (page->secs != 0) {
            LobbyMsg_GetBlob(page->text, sizeof(page->text), cur);
        }
        msg.kind = 0;
        msg.flag = NET_PROGRESS_WEB_PAGE;
        gLobbyNet.task[NET_TASK_LOGIN].onDone(msg, &msg);
    }
}

/* Copies the login web page (68B0); returns 0. */
s32 LobbyNet_GetLoginWebPage(NetLoginWebPage *dst)
{
    *dst = gNetLoginWebPage;
    return 0;
}

/* The notification of the server ids the client may use: the first and the
 * last, 12 digits each, into gServerIdMin and gServerIdMax. */
void LobbyNotify_ServerIdRange(void)
{
    if (gLobbyNet.task[NET_TASK_LOGIN].active != 0) {
        LobbyMsg_GetBlob(gServerIdMax, 12, LobbyMsg_GetBlob(gServerIdMin, 12, gLobbyRecvData));
    }
}

/*
 * The server's request 614A: u32 tag, u32 address, u16 n. The answer sends
 * back the tag and the n bytes of the client's memory at that address.
 */
void LobbySrvReq_MemoryCheck(void)
{
    u32 tag;
    u32 addr;
    u16 n;

    LobbyMsg_GetU16((u8 *)&n, LobbyMsg_GetU32((u8 *)&addr, LobbyMsg_GetU32((u8 *)&tag, gLobbyRecvData)));
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_MEMORY_CHECK_ANSWER);
    LobbyPkt_PutU32(&gLobbyPkt, tag);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)addr, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}
