/*
 * The lobby protocol's message layer.
 * Network code: it stays byte for byte, as the whole build does.
 *
 * - Requests: up to 0x80 pending requests (NetReq, LobbyNet.req), each
 *   waiting for the reply whose echo is its sequence number, plus 0x10 of a
 *   second kind (LobbyNet.req2) and 0x10 tasks run every frame
 *   (LobbyNet.task).
 * - Receiving: the header of the message received (gLobbyRecvHdr, 0x10 bytes)
 *   is unpacked into LobbyNet by Lobby_HandleMessage; Lobby_DispatchCommand then looks the
 *   command up in the command tables (0x155 entries: code, kind, flag and a
 *   handler) and calls its handler. The readers LobbyMsg_GetU8 ..
 *   LobbyMsg_ReadU8U8Str take the payload (gLobbyRecvData) apart field by field.
 * - Sending: LobbyPkt_BeginCmd (a request, with the command's code and flag from
 *   the tables) or LobbyPkt_Begin (a raw code) starts gLobbyPkt, the
 *   writers append big-endian fields and obfuscated blobs
 *   (Net_LockrockObfuscate), LobbyPkt_End closes it and LobbyPkt_Send sends
 *   it with NetTcp_Send.
 * - Routing: a notification or a request of the server's goes on to the
 *   handler registered for its route (Lobby_RouteMessage, NET_ROUTE_*); a
 *   reply completes the request it answers (NetReq_OnReply).
 * - The session's own commands: leaving the lobby (6002), logging out
 *   (6006), the keep-alive (6001) and the server's closing notices
 *   (6003-6007).
 * - At the end, the command tables (code, kind, flag and the handler
 *   gLobbyCmdHandler calls for each entry); the request and reply functions
 *   of the other commands follow in the next files, by command family.
 */
#include "lobby_net.h"

extern void (*gLobbyCmdHandler[])(void);
extern void (*gNetHandlers[])(NetMsg m, NetMsg *pm);
extern struct {
    u16 seq;
} gLobbyCmdSeq;

void LobbyMsg_ReadErrText(void);
void Net_CallHandler(s32 i, s32 b);
extern LobbyNet gLobbyNet;
extern u8 gLobbyCmdHi[];
extern u8 gLobbyCmdLo[];
extern u8 gLobbyCmdKind[];
extern u8 gLobbyCmdFlag[];
extern LobbyHdr gLobbyRecvHdr;
char *strcpy(void *dst, const void *src);
s32 LobbyPkt_EntryKind(u16 kind, u16 arg1);
s32 LobbyPkt_LobbyLeave(void);
s32 LobbyPkt_Logout(void);
void LobbyMsg_ReadU16U8b2(void *a, void *b);
s32 Lobby_RecvPacket(s32 sock, u8 *hdr, u8 *data, u32 max);

/* Copies the third level's records (gLobbyNet.recs2) out; returns 0. */
s32 NetLevel2RecsTask_GetRecs(s32 unused, NetRecs2 *dst)
{
    *dst = gLobbyNet.recs2;
    return 0;
}

/*
 * The reply to 6507, a command the client never sends: completes its request.
 */
void LobbyReply_6507(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Asks for the kind of level-2 entry id (6418); onReply gets the answer.
 * Returns the request's slot, -1 if none is free.
 */
s32 LobbyReq_Level2Kind(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryKind(2, id);
        return i;
    }
    return -1;
}

/*
 * The reply to 6418, or the same as a notification: u16 id, u8 kind, stored
 * in the level-2 list's entry.
 */
void LobbyReply_Level2Kind(void)
{
    u8 entryKind;
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b2(&id, &entryKind);
        gLobbyNet.list2[id - 1].id = id;
        gLobbyNet.list2[id - 1].kind = entryKind;
        gLobbyNet.list2[id - 1].flags |= NET_ENTRY_KIND;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL2_KIND);
}

/* Registers the handler of a route: Lobby_RouteMessage(route) calls it. */
void Net_SetHandler(s32 route, void (*handler)(NetMsg, NetMsg *))
{
    gNetHandlers[route] = handler;
}

/*
 * Calls the handler of a route (if one is registered) with a message of
 * kind 1 whose flag is the route.
 */
void Net_CallHandler(s32 route, s32 unused)
{
    NetMsg msg;

    msg.flag = route;
    msg.kind = 1;
    if (gNetHandlers[(u16)route] != NULL) {
        gNetHandlers[(u16)route](msg, NULL);
    }
}

/*
 * Passes a message received on. A notification or a request of the server's
 * goes to the handler of route (none for 0); a reply completes the request it
 * answers (NetReq_OnReply), with kind 0, or -1 and the error text read when
 * the reply reports an error.
 */
void Lobby_RouteMessage(s32 route)
{
    NetMsg msg;

    if (gLobbyNet.flag == LOBBY_FLAG_NOTIFY || gLobbyNet.flag == LOBBY_FLAG_REQUEST) {
        if (route != 0) {
            Net_CallHandler(route, 0);
        }
        return;
    }
    if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
        if (gLobbyNet.replyErr == 0) {
            msg.kind = 0;
        } else {
            msg.kind = -1;
            LobbyMsg_ReadErrText();
        }
        NetReq_OnReply(msg, 1, 0);
    }
}

/* Frees every request slot. */
void NetReq_ClearAll(void)
{
    memset(gLobbyNet.req, 0, sizeof(gLobbyNet.req));
}

/* Stops every network task. */
void NetTask_ClearAll(void)
{
    memset(gLobbyNet.task, 0, sizeof(gLobbyNet.task));
}

/*
 * Takes a free request slot for a request about to be sent, with the
 * callbacks of its reply; returns the slot, -1 if none is free.
 */
s32 NetReq_Alloc(u8 state, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    for (i = 0; i < NET_REQ_COUNT; i++) {
        if (gLobbyNet.req[i].state == NET_REQ_FREE) {
            gLobbyNet.req[i].state = state;
            gLobbyNet.req[i].unk19 = 0;
            gLobbyNet.req[i].onReply = onReply;
            gLobbyNet.req[i].onTick = onTick;
            return i;
        }
    }
    return -1;
}

/* Takes fixed request slot i (gLobbyNet.req2) whatever it held; returns 1. */
s32 NetReq_TakeFixed(s32 i, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *))
{
    NetReq *req = &gLobbyNet.req2[i];

    req->state = NET_REQ_WAITING;
    req->unk19 = 0;
    req->onReply = onReply;
    req->onTick = onTick;
    return 1;
}

/*
 * The reply received (its echo is the request's sequence number): frees the
 * request it answers and calls the request's onReply with msg. Returns the
 * slot, -1 if no request waits for it.
 */
s32 NetReq_OnReply(NetMsg msg, s32 unusedA, s32 unusedB)
{
    s32 i;
    u16 echo = gLobbyNet.echo;

    for (i = 0; i < NET_REQ_COUNT; i++) {
        if (echo == gLobbyNet.req[i].seq) {
            gLobbyNet.req[i].state = NET_REQ_FREE;
            gLobbyNet.req[i].unk19 = 0;
            if (gLobbyNet.req[i].onReply != NULL) {
                gLobbyNet.req[i].onReply(msg, &msg);
            }
            return i;
        }
    }
    for (i = 0; i < NET_REQ2_COUNT; i++) {
        if (echo == gLobbyNet.req2[i].seq) {
            gLobbyNet.req2[i].state = NET_REQ_FREE;
            gLobbyNet.req2[i].unk19 = 0;
            if (gLobbyNet.req2[i].onReply != NULL) {
                gLobbyNet.req2[i].onReply(msg, &msg);
            }
            return i;
        }
    }
    return -1;
}

/*
 * Once a frame: runs onTick of every request that asks for it, then the
 * dispatcher of every active network task.
 */
void Net_TickRequests(void)
{
    s32 i;

    for (i = 0; i < NET_REQ_COUNT; i++) {
        if (gLobbyNet.req[i].state == NET_REQ_TICKING && gLobbyNet.req[i].onTick != NULL) {
            gLobbyNet.req[i].onTick(i);
        }
    }
    for (i = 0; i < NET_TASK_COUNT; i++) {
        if (gLobbyNet.task[i].active == 1 && gLobbyNet.task[i].func != NULL) {
            gLobbyNet.task[i].func(i);
        }
    }
}

/* the number of free request slots */
s32 NetReq_CountFree(void)
{
    s32 n = 0;
    s32 i;

    for (i = 0; i < NET_REQ_COUNT; i++) {
        if (gLobbyNet.req[i].state == NET_REQ_FREE) {
            n++;
        }
    }
    return n;
}

/* the request the message received answers, -1 if none */
s32 NetReq_FindReply(void)
{
    s32 i;

    for (i = 0; i < NET_REQ_COUNT; i++) {
        if (gLobbyNet.echo == gLobbyNet.req[i].seq) {
            return i;
        }
    }
    return -1;
}

/* calls the handler of a command received; 0 if there is none */
s32 Lobby_DispatchCommand(u16 cmd, s32 kind, u8 flag)
{
    s32 i;
    u16 code;

    for (i = 0; i < LOBBY_CMD_COUNT; i++) {
        code = gLobbyCmdHi[i] << 8;
        code |= gLobbyCmdLo[i];
        if (gLobbyCmdKind[i] != LOBBY_KIND_SEND && cmd == code && gLobbyCmdFlag[i] == flag && gLobbyCmdHandler[i] != NULL) {
            gLobbyCmdHandler[i]();
            return 1;
        }
    }
    return 0;
}

/* once a frame: up to four messages received and handled, then the
 * requests and tasks ticked; 1 if a message came */
s32 Lobby_ReceiveAll(s32 sock)
{
    s32 i;
    s32 got;

    if (gLobbyNet.connected == 0) {
        return 0;
    }
    gLobbyNet.sock = sock;
    got = 0;
    i = 0;
    gLobbyNet.frames++;
    for (i = 0; i < LOBBY_RECV_PER_FRAME; i++) {
        gLobbyNet.recvResult = Lobby_RecvPacket(gLobbyNet.sock, (u8 *)&gLobbyRecvHdr, gLobbyRecvData, LOBBY_RECV_MAX);
        switch (gLobbyNet.recvResult) {
        default:
            got = 1;
            Lobby_HandleMessage();
            break;
        case 0:
        case -1:
            break;
        }
    }
    Net_TickRequests();
    return got;
}

/* unpacks the header received (kind << 4, flag, cmd, size, echo, error) and
 * calls the command's handler */
void Lobby_HandleMessage(void)
{
    u16 cmd;

    gLobbyNet.flag = gLobbyRecvHdr.flag;
    cmd = gLobbyRecvHdr.cmd[0] << 8;
    cmd |= gLobbyRecvHdr.cmd[1];
    gLobbyNet.replyErr = gLobbyRecvHdr.err;
    gLobbyNet.cmd = cmd;
    gLobbyNet.echo = gLobbyRecvHdr.echo[0] << 8;
    gLobbyNet.echo |= gLobbyRecvHdr.echo[1];
    gLobbyNet.size = gLobbyRecvHdr.size[0] << 8;
    gLobbyNet.size |= gLobbyRecvHdr.size[1];
    if (Lobby_DispatchCommand(cmd, gLobbyRecvHdr.magic >> 4 & 0xF, gLobbyRecvHdr.flag) == 0) {
        return;
    }
}

/*
 * Leaves the lobby but keeps the session (6002): before a server switch, a
 * web page or a quest. Returns the request's slot, -1 if none is free.
 */
s32 LobbyReq_LobbyLeave(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_LobbyLeave();
        return i;
    }
    return -1;
}

/* Sends 6002 (no payload); returns its sequence number. */
s32 LobbyPkt_LobbyLeave(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LOBBY_LEAVE);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * The reply to 6002: completes the request, kind -1 with the error text on an error.
 */
void LobbyReply_LobbyLeave(void)
{
    NetMsg msg;

    if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
        if (gLobbyNet.replyErr == 0) {
            msg.kind = 0;
        } else {
            msg.kind = -1;
            LobbyMsg_ReadErrText();
        }
        NetReq_OnReply(msg, 1, 0);
    }
}

/* Logs out (6006). Returns the request's slot, -1 if none is free. */
s32 LobbyReq_Logout(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Logout();
        return i;
    }
    return -1;
}

/* Sends 6006 (no payload); returns its sequence number. */
s32 LobbyPkt_Logout(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LOGOUT);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * The reply to 6006: completes the request, kind -1 with the error text on an error.
 */
void LobbyReply_Logout(void)
{
    NetMsg msg;

    if (gLobbyNet.flag == LOBBY_FLAG_REPLY) {
        if (gLobbyNet.replyErr == 0) {
            msg.kind = 0;
        } else {
            msg.kind = -1;
            LobbyMsg_ReadErrText();
        }
        NetReq_OnReply(msg, 1, 0);
    }
}

/* Copies the error text of the last reply that reported one; returns 0. */
s32 LobbyNet_GetErrText(char *dst)
{
    strcpy(dst, gLobbyNet.errText);
    return 0;
}

/*
 * The server's keep-alive request (6001): answers it and notes the answer sent.
 */
void LobbySrvReq_Keepalive(void)
{
    LobbyPkt_Keepalive();
    gLobbyNet.sentKeepalive = 1;
}

/* Sends the keep-alive's answer (6001, no payload). */
void LobbyPkt_Keepalive(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_KEEPALIVE_ANSWER);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* The server closes the session (6003): keeps its text and routes it. */
void LobbyNotify_ServerClose(void)
{
    LobbyMsg_ReadErrText();
    Lobby_RouteMessage(NET_ROUTE_SERVER_CLOSE);
}

/* The departure for a quest is cancelled (6004). */
void LobbyNotify_DepartCancel(void)
{
    Lobby_RouteMessage(NET_ROUTE_DEPART_CANCEL);
}

/* The departure for a quest failed (6005): keeps the server's text. */
void LobbyNotify_DepartError(void)
{
    LobbyMsg_ReadErrText();
    Lobby_RouteMessage(NET_ROUTE_DEPART_ERROR);
}

/* The server sends the player out of the lobby (6007), with a text. */
void LobbyNotify_ServerLeave(void)
{
    LobbyMsg_ReadErrText();
    Lobby_RouteMessage(NET_ROUTE_SERVER_LEAVE);
}

/* Reads a byte of a message; returns the next field. */
u8 *LobbyMsg_GetU8(void *dst, u8 *src)
{
    *(u8 *)dst = *src;
    return src + 1;
}

/* Reads a big-endian u16 of a message; returns the next field. */
u8 *LobbyMsg_GetU16(void *dst, u8 *src)
{
    ((u8 *)dst)[1] = src[0];
    ((u8 *)dst)[0] = src[1];
    return src + 2;
}

/* Reads a big-endian u32 of a message; returns the next field. */
u8 *LobbyMsg_GetU32(void *dst, u8 *src)
{
    ((u8 *)dst)[3] = src[0];
    ((u8 *)dst)[2] = src[1];
    ((u8 *)dst)[1] = src[2];
    ((u8 *)dst)[0] = src[3];
    return src + 4;
}

/*
 * Reads a string (a big-endian u16 length, then the characters) and ends it
 * with a 0; returns the next field.
 */
u8 *LobbyMsg_GetString(void *dst, u8 *src)
{
    u16 n;

    n = (u16)(src[0] << 8) | src[1];
    memcpy(dst, src + 2, n);
    ((u8 *)dst)[n] = 0;
    return src + (n + 2);
}

/* Reads n bytes; returns the next field. */
u8 *LobbyMsg_GetBytes(void *dst, u8 *src, u16 n)
{
    memcpy(dst, src, n);
    return src + n;
}

/* a blob: a big-endian length, then the bytes, of which at most max are kept */
u8 *LobbyMsg_GetBlob(void *dst, u16 max, u8 *src)
{
    u16 n;
    s32 copy;

    n = (u16)(src[0] << 8) | src[1];
    copy = n;
    if (max < n) {
        copy = max;
    }
    if (copy != 0) {
        memcpy(dst, src + 2, (u16)copy);
    }
    return src + (n + 2);
}

/* Reads the error text a reply carries (a blob) into gLobbyNet.errText. */
void LobbyMsg_ReadErrText(void)
{
    memset(gLobbyNet.errText, 0, sizeof(gLobbyNet.errText));
    LobbyMsg_GetBlob(gLobbyNet.errText, sizeof(gLobbyNet.errText), gLobbyRecvData);
}

/* Reads the first field of the message received, a byte. */
u8 *LobbyMsg_ReadU8(void *dst)
{
    return LobbyMsg_GetU8(dst, gLobbyRecvData);
}

/* Reads the first field of the message received, a u16. */
u8 *LobbyMsg_RecvU16(void *dst)
{
    return LobbyMsg_GetU16(dst, gLobbyRecvData);
}

/* Reads the first field of the message received, a u32. */
u8 *LobbyMsg_ReadU32(void *dst)
{
    return LobbyMsg_GetU32(dst, gLobbyRecvData);
}

/* Reads the message received as a byte and a string. */
u8 *LobbyMsg_ReadU8Str(void *first, void *second)
{
    return LobbyMsg_GetString(second, LobbyMsg_GetU8(first, gLobbyRecvData));
}

/* Reads the message received as two bytes. */
u8 *LobbyMsg_ReadU8U8(void *first, void *second)
{
    return LobbyMsg_GetU8(second, LobbyMsg_GetU8(first, gLobbyRecvData));
}

/* Reads the message received as a u16 and a byte. */
u8 *LobbyMsg_ReadU16U8(void *first, void *second)
{
    return LobbyMsg_GetU8(second, LobbyMsg_GetU16(first, gLobbyRecvData));
}

/* Reads the message received as two u16. */
u8 *LobbyMsg_ReadU16U16(void *first, void *second)
{
    return LobbyMsg_GetU16(second, LobbyMsg_GetU16(first, gLobbyRecvData));
}

/* Reads the message received as a u16 and a u32. */
u8 *LobbyMsg_ReadU16U32(void *first, void *second)
{
    return LobbyMsg_GetU32(second, LobbyMsg_GetU16(first, gLobbyRecvData));
}

/* Reads the message received as two bytes and a string. */
u8 *LobbyMsg_ReadU8U8Str(void *first, void *second, void *third)
{
    return LobbyMsg_GetString(third, LobbyMsg_GetU8(second, LobbyMsg_GetU8(first, gLobbyRecvData)));
}

/*
 * Starts gLobbyPkt as the message of entry `entry` of the command tables
 * (its code and flag): a reply echoes the request received, anything else
 * takes the next sequence number. Returns the sequence number, which the
 * reply will echo.
 */
u16 LobbyPkt_BeginCmd(LobbyPkt *pkt, u16 entry)
{
    memset(pkt->data, 0, sizeof(pkt->data));
    pkt->cmd[0] = gLobbyCmdHi[entry];
    pkt->cmd[1] = gLobbyCmdLo[entry];
    pkt->flag = gLobbyCmdFlag[entry];
    pkt->len = 0;
    pkt->pos = 0;
    pkt->magic = LOBBY_MAGIC_CMD;
    if (pkt->flag == LOBBY_FLAG_REPLY) {
        gLobbyPkt.echo[0] = gLobbyRecvHdr.echo[0];
        gLobbyPkt.echo[1] = gLobbyRecvHdr.echo[1];
    } else {
        gLobbyCmdSeq.seq++;
        pkt->echo[0] = gLobbyCmdSeq.seq >> 8;
        pkt->echo[1] = gLobbyCmdSeq.seq;
    }
    pkt->ff[0] = 0xFF;
    pkt->ff[1] = 0xFF;
    pkt->ff[2] = 0xFF;
    pkt->err = 0;
    return gLobbyCmdSeq.seq;
}

/* starts a message (magic 0x82) with a raw code, echoing the message received */
void LobbyPkt_Begin(LobbyPkt *pkt, u16 rawCode)
{
    s32 code;

    memset(pkt->data, 0, sizeof(pkt->data));
    code = rawCode;
    pkt->cmd[0] = code >> 8;
    pkt->cmd[1] = code;
    pkt->len = 0;
    pkt->pos = 0;
    pkt->magic = LOBBY_MAGIC_RAW;
    pkt->ff[0] = 0xFF;
    pkt->ff[1] = 0xFF;
    pkt->ff[2] = 0xFF;
    pkt->flag = 0;
    pkt->err = 0;
    memcpy(pkt->echo, gLobbyRecvHdr.echo, 2);
}

/* Sets the message's flag (LOBBY_FLAG_*). */
void LobbyPkt_SetFlag(LobbyPkt *pkt, u8 flag)
{
    pkt->flag = flag;
}

/* Sets a reply's error byte: 0xFF refuses the server's request. */
void LobbyPkt_SetError(LobbyPkt *pkt, u8 err)
{
    pkt->err = err;
}

/* closes the message: the payload size into the header */
void LobbyPkt_End(LobbyPkt *pkt)
{
    pkt->size[0] = pkt->pos >> 8;
    pkt->size[1] = pkt->pos;
}

/* Appends a byte to the payload. */
void LobbyPkt_PutU8(LobbyPkt *pkt, u8 value)
{
    /* pkt->data[pkt->pos], written as an offset from the packet's start:
     * the compiler then adds the two in the game's order */
    *((u8 *)pkt + pkt->pos + 0x10) = value;
    pkt->len++;
    pkt->pos++;
}

/* Appends a big-endian u16 to the payload. */
void LobbyPkt_PutU16(LobbyPkt *pkt, u16 value)
{
    s32 wide = value;
    u8 *dst = &pkt->data[pkt->pos];

    dst[0] = wide >> 8;
    dst[1] = wide;
    pkt->len += 2;
    pkt->pos += 2;
}

/* Appends a big-endian u32 to the payload. */
void LobbyPkt_PutU32(LobbyPkt *pkt, u32 value)
{
    u8 *dst = &pkt->data[pkt->pos];

    dst[0] = value >> 24;
    dst[1] = value >> 16;
    dst[2] = value >> 8;
    dst[3] = value;
    pkt->len += 4;
    pkt->pos += 4;
}

/* Appends n bytes to the payload as they are. */
void LobbyPkt_PutBytes(LobbyPkt *pkt, void *src, u16 n)
{
    memcpy(&pkt->data[pkt->pos], src, n);
    pkt->len += n;
    pkt->pos += n;
}

/* an obfuscated blob: length + 2, the checksum, the bytes */
void LobbyPkt_PutBlob(LobbyPkt *pkt, void *src, u16 n)
{
    u8 buf[0x210];
    s32 sum;

    sum = Net_LockrockObfuscate(buf, src, (u16)((u16)(pkt->echo[0] << 8) + pkt->echo[1]), n, gLobbyNet.key);
    LobbyPkt_PutU16(pkt, n + 2);
    LobbyPkt_PutU16(pkt, sum);
    LobbyPkt_PutBytes(pkt, buf, n);
}

/* The same as LobbyPkt_PutBlob (the login's password is sent with it). */
void LobbyPkt_PutBlob2(LobbyPkt *pkt, void *src, u16 n)
{
    u8 buf[0x210];
    s32 sum;

    sum = Net_LockrockObfuscate(buf, src, (u16)((u16)(pkt->echo[0] << 8) + pkt->echo[1]), n, gLobbyNet.key);
    LobbyPkt_PutU16(pkt, n + 2);
    LobbyPkt_PutU16(pkt, sum);
    LobbyPkt_PutBytes(pkt, buf, n);
}

/* The same as LobbyPkt_PutBytes. */
void LobbyPkt_PutBytes2(LobbyPkt *pkt, void *src, u16 n)
{
    LobbyPkt_PutBytes(pkt, src, n);
}

/*
 * Sends the message: the header from magic, then the payload. Returns
 * NetTcp_Send's result.
 */
s32 LobbyPkt_Send(LobbyPkt *pkt)
{
    return NetTcp_Send(gLobbyNet.sock, &pkt->magic, pkt->len + LOBBY_HEADER_SIZE);
}

/* The command tables: for each entry the code's high and low bytes, its kind
 * (LOBBY_KIND_*), its flag (LOBBY_FLAG_*) and the handler of a message
 * received; LobbyPkt_BeginCmd takes an entry's index. */
void LobbyNotify_ServerClose(void);
void LobbyNotify_DepartCancel(void);
void LobbyNotify_DepartError(void);
void LobbyNotify_ServerLeave(void);
void LobbyNotify_ServerMessage(void);
void LobbyNotify_LoginDone(void);
void LobbyNotify_ServerIdRange(void);
void LobbyNotify_PatchHeader(void);
void LobbyNotify_PatchData(void);
void LobbyNotify_6123(void);
void LobbyNotify_CharaList(void);
void LobbyNotify_UserCard(void);
void LobbyNotify_UserCardPart(void);
void LobbyNotify_Level0Kick(void);
void LobbyNotify_Level1Kick(void);
void LobbyNotify_Level1UserOut(void);
void LobbyNotify_Level1UserIn(void);
void LobbyNotify_6414(void);
void LobbyNotify_Level2UserOut(void);
void LobbyNotify_Level2UserIn(void);
void LobbyNotify_Level2Kick(void);
void LobbyNotify_Level2MembersNotify(void);
void LobbyNotify_ChatMessage(void);
void LobbyNotify_ShortMail(void);
void LobbyNotify_TownMsg(void);
void LobbyNotify_ChatTellMessage(void);
void LobbyNotify_TownMsgToRecv(void);
void LobbyNotify_TownDataChanged(void);
void LobbyNotify_LoginWebPage(void);
void LobbyNotify_PartyReady(void);
void LobbyReply_LobbyLeave(void);
void LobbyReply_Logout(void);
void LobbyReply_Ping(void);
void LobbyReply_CharaSelect(void);
void LobbyReply_QuestRecord(void);
void LobbyReply_6143(void);
void LobbyReply_6144(void);
void LobbyReply_6145(void);
void LobbyReply_6146(void);
void LobbyReply_SaveSummaryBegin(void);
void LobbyReply_SaveSummaryPart(void);
void LobbyReply_LoginInfo(void);
void LobbyReply_CharaSelectText(void);
void LobbyReply_6181(void);
void LobbyReply_6188(void);
void LobbyReply_HunterCard(void);
void LobbyReply_HunterCardPart(void);
void LobbyReply_LoginParams(void);
void LobbyReply_QnetTimeouts(void);
void LobbyReply_6202(void);
void LobbyReply_Level0Count(void);
void LobbyReply_Level0Name(void);
void LobbyReply_Level0Values(void);
void LobbyReply_Level0State(void);
void LobbyReply_Level0Enter(void);
void LobbyReply_LeaveAll(void);
void LobbyReply_EnterLevels(void);
void LobbyReply_Level0Text(void);
void LobbyReply_Level0Kind(void);
void LobbyReply_Level1Count(void);
void LobbyReply_Level1Name(void);
void LobbyReply_Level1Values(void);
void LobbyReply_Level1State(void);
void LobbyReply_Level1Enter(void);
void LobbyReply_Level0Leave(void);
void LobbyReply_Level1Text(void);
void LobbyReply_Level1Users(void);
void LobbyReply_630B(void);
void LobbyReply_Level1SetColor(void);
void LobbyReply_Level1Color(void);
void LobbyReply_Level1Kind(void);
void LobbyReply_Level1Open(void);
void LobbyReply_Level1SettingsEnd(void);
void LobbyReply_Level1NameAllowed(void);
void LobbyReply_Level1PassAllowed(void);
void LobbyReply_Level1TextAllowed(void);
void LobbyReply_Level1SetName(void);
void LobbyReply_Level1SetPass(void);
void LobbyReply_Level1SetText(void);
void LobbyReply_Level1SetGroupInfo(void);
void LobbyReply_Level1Search(void);
void LobbyReply_Level1SearchInfo(void);
void LobbyReply_Level1GroupInfo(void);
void LobbyReply_Level2Count(void);
void LobbyReply_Level2Name(void);
void LobbyReply_Level2Values(void);
void LobbyReply_Level2State(void);
void LobbyReply_Level2Locked(void);
void LobbyReply_Level2Enter(void);
void LobbyReply_Level2Open(void);
void LobbyReply_Level1Leave(void);
void LobbyReply_6409(void);
void LobbyReply_Level2Users(void);
void LobbyReply_Level2Limits(void);
void LobbyReply_Level2Text(void);
void LobbyReply_640E(void);
void LobbyReply_640F(void);
void LobbyReply_Level2Members(void);
void LobbyReply_6413(void);
void LobbyReply_Level2Quest(void);
void LobbyReply_6416(void);
void LobbyReply_6417(void);
void LobbyReply_Level2Leave(void);
void LobbyReply_SetOut(void);
void LobbyReply_QuestAccept(void);
void LobbyReply_Level2Color(void);
void LobbyReply_QuestPost(void);
void LobbyReply_Level2PostedQuest(void);
void LobbyReply_Level2NameEditable(void);
void LobbyReply_Level2PassEditable(void);
void LobbyReply_Level2RecCount(void);
void LobbyReply_Level2RecName(void);
void LobbyReply_Level2RecEditable(void);
void LobbyReply_Level2RecDefault(void);
void LobbyReply_Level2RecChoiceCount(void);
void LobbyReply_Level2RecChoiceName(void);
void LobbyReply_Level2SetName(void);
void LobbyReply_Level2SetPass(void);
void LobbyReply_Level2SetRecChoice(void);
void LobbyReply_Level2SettingsEnd(void);
void LobbyReply_Level2RecChoiceValues(void);
void LobbyReply_Level2TextEditable(void);
void LobbyReply_Level2SetText(void);
void LobbyReply_ShortMailSend(void);
void LobbyReply_UserStatus(void);
void LobbyReply_UserSearch(void);
void LobbyReply_ChatTell(void);
void LobbyReply_TownMsgTo(void);
void LobbyReply_ChatColorMode(void);
void LobbyReply_UserFindByName(void);
void LobbyReply_UserCardFetch(void);
void LobbyReply_UserProfile(void);
void LobbyReply_DlList(void);
void LobbyReply_DlData(void);
void LobbyReply_DlFileInfo(void);
void LobbyReply_TownData(void);
void LobbyReply_6890(void);
void LobbyReply_CurrentPlace(void);
void LobbyReply_TownClock(void);
void LobbyReply_68A0(void);
void LobbyReply_68A1(void);
void LobbyReply_ServerName(void);
void LobbyReply_PartyCount(void);
void LobbyReply_PartySelf(void);
void LobbyReply_PartyMember(void);
void LobbyReply_PartyText(void);
void LobbyReply_PartyRoom(void);
void LobbyReply_PartyServer(void);
void LobbyReply_PartyMemberValues(void);
void LobbySrvReq_Keepalive(void);
void LobbySrvReq_LoginAuth(void);
void LobbySrvReq_ClientInfo(void);
void LobbySrvReq_LoginNotice(void);
void LobbySrvReq_PatchAck(void);
void LobbySrvReq_PatchEnd(void);
void LobbySrvReq_6138(void);
void LobbySrvReq_MemoryCheck(void);
void LobbySrvReq_6189(void);
void LobbySrvReq_6706(void);
u8 gLobbyCmdHi[352] = {
#include "lobby/007EC980.inc"
};
u8 gLobbyCmdLo[352] = {
#include "lobby/007ECAE0.inc"
};
u8 gLobbyCmdKind[352] = {
#include "lobby/007ECC40.inc"
};
u8 gLobbyCmdFlag[352] = {
#include "lobby/007ECDA0.inc"
};
void HttpLbs_Receive(void); /* nethttp */
void (*gLobbyCmdHandler[344])(void) = {
    LobbySrvReq_Keepalive, NULL, NULL, LobbyReply_LobbyLeave,
    NULL, LobbyReply_Logout, LobbyNotify_ServerClose, LobbyNotify_DepartCancel,
    LobbyNotify_DepartError, LobbyNotify_ServerLeave, NULL, LobbyReply_Ping,
    LobbySrvReq_LoginAuth, NULL, LobbySrvReq_ClientInfo, NULL,
    LobbyNotify_LoginDone, LobbySrvReq_LoginNotice, NULL, LobbyNotify_CharaList,
    NULL, LobbyReply_CharaSelect, LobbySrvReq_6138, NULL,
    NULL, NULL, LobbyReply_6143, NULL,
    LobbyReply_LoginInfo, NULL, LobbyReply_HunterCard, LobbyNotify_UserCard,
    NULL, LobbyReply_HunterCardPart, LobbyNotify_UserCardPart, NULL,
    LobbyReply_LoginParams, NULL, LobbyReply_QnetTimeouts, NULL,
    LobbyReply_QuestRecord, NULL, LobbyReply_CharaSelectText, LobbySrvReq_MemoryCheck,
    NULL, LobbyNotify_LoginWebPage, LobbyNotify_ServerIdRange, NULL,
    LobbyReply_6144, NULL, LobbyReply_6145, NULL,
    LobbyReply_6146, LobbyReply_6202, LobbyReply_6202, NULL,
    LobbyReply_LeaveAll, NULL, LobbyReply_EnterLevels, NULL,
    LobbyReply_Level0Enter, NULL, LobbyReply_Level0Count, NULL,
    LobbyReply_Level0Name, NULL, LobbyReply_Level0Values, LobbyReply_Level0Values,
    NULL, LobbyReply_Level0State, LobbyReply_Level0State, NULL,
    LobbyReply_Level0Text, LobbyReply_Level0Text, NULL, LobbyReply_Level0Leave,
    LobbyNotify_Level0Kick, NULL, LobbyReply_Level0Kind, LobbyReply_Level0Kind,
    NULL, LobbyReply_Level1Enter, NULL, LobbyReply_Level1Leave,
    NULL, LobbyReply_Level1Count, NULL, LobbyReply_Level1Name,
    LobbyReply_Level1Name, NULL, LobbyReply_Level1Values, LobbyReply_Level1Values,
    NULL, LobbyReply_Level1State, LobbyReply_Level1State, NULL,
    LobbyReply_Level1Text, LobbyReply_Level1Text, LobbyNotify_Level1Kick, NULL,
    LobbyReply_Level1Open, NULL, LobbyReply_Level1NameAllowed, NULL,
    LobbyReply_Level1PassAllowed, NULL, LobbyReply_Level1TextAllowed, NULL,
    LobbyReply_Level1SetName, NULL, LobbyReply_Level1SetPass, NULL,
    LobbyReply_Level1SettingsEnd, NULL, LobbyReply_Level1SetText, NULL,
    LobbyReply_630B, LobbyReply_630B, NULL, LobbyReply_Level1SetColor,
    NULL, LobbyReply_Level1Color, LobbyReply_Level1Color, NULL,
    LobbyReply_Level1GroupInfo, LobbyReply_Level1GroupInfo, NULL, LobbyReply_Level1Users,
    LobbyNotify_Level1UserOut, LobbyNotify_Level1UserIn, NULL, LobbyReply_Level1Kind,
    LobbyReply_Level1Kind, NULL, LobbyReply_Level2Enter, NULL,
    LobbyReply_Level2Leave, NULL, NULL, LobbyReply_Level2Count,
    NULL, LobbyReply_Level2Name, LobbyReply_Level2Name, NULL,
    LobbyReply_Level2Values, LobbyReply_Level2Values, NULL, LobbyReply_Level2State,
    LobbyReply_Level2State, NULL, LobbyReply_Level2Text, LobbyReply_Level2Text,
    NULL, LobbyReply_Level2Color, LobbyReply_Level2Color, NULL,
    LobbyReply_Level2PostedQuest, LobbyReply_Level2PostedQuest, NULL, LobbyReply_Level2Locked,
    LobbyReply_Level2Locked, NULL, LobbyReply_Level2Users, NULL,
    LobbyReply_Level2Limits, LobbyReply_Level2Limits, NULL, LobbyReply_6409,
    NULL, LobbyReply_Level2Open, NULL, LobbyReply_Level2NameEditable,
    NULL, LobbyReply_Level2PassEditable, NULL, LobbyReply_Level2TextEditable,
    NULL, LobbyReply_Level2RecCount, NULL, LobbyReply_Level2RecName,
    NULL, LobbyReply_Level2RecEditable, NULL, LobbyReply_Level2RecDefault,
    NULL, LobbyReply_Level2RecChoiceCount, NULL, LobbyReply_Level2RecChoiceName,
    NULL, LobbyReply_Level2RecChoiceValues, NULL, LobbyReply_Level2SetName,
    NULL, LobbyReply_Level2SetPass, NULL, LobbyReply_Level2SetText,
    NULL, LobbyReply_Level2SetRecChoice, NULL, LobbyReply_Level2SettingsEnd,
    LobbyNotify_Level2UserOut, LobbyNotify_Level2UserIn, LobbyNotify_Level2Kick, NULL,
    LobbyReply_6413, LobbyNotify_6414, NULL, LobbyReply_QuestAccept,
    NULL, LobbyReply_QuestPost, NULL, LobbyReply_6507,
    NULL, LobbyReply_Level2Kind, LobbyReply_Level2Kind, NULL,
    LobbyReply_SetOut, NULL, LobbyReply_Level2Members, LobbyReply_Level2Members,
    LobbyNotify_Level2MembersNotify, NULL, LobbyNotify_PartyReady, NULL,
    LobbyReply_PartyCount, NULL, LobbyReply_PartySelf, NULL,
    LobbyReply_PartyText, NULL, LobbyReply_PartyMember, NULL,
    LobbyReply_PartyMemberValues, NULL, NULL, LobbyReply_PartyRoom,
    NULL, LobbyReply_PartyServer, NULL, LobbyReply_6181,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, LobbyReply_6188,
    LobbySrvReq_6189, NULL, LobbyNotify_PatchHeader, LobbyNotify_PatchData,
    LobbyNotify_6123, LobbySrvReq_PatchAck, NULL, LobbySrvReq_PatchEnd,
    NULL, NULL, NULL, HttpLbs_Receive,
    NULL, LobbyReply_DlList, NULL, LobbyReply_DlData,
    NULL, LobbyReply_DlFileInfo, LobbyNotify_ServerMessage, NULL,
    LobbyReply_SaveSummaryBegin, NULL, LobbyReply_SaveSummaryPart, NULL,
    LobbyReply_TownData, LobbyNotify_TownDataChanged, NULL, LobbyReply_6890,
    LobbyReply_6890, NULL, LobbyReply_CurrentPlace, NULL,
    LobbyReply_TownClock, LobbyReply_TownClock, NULL, LobbyReply_640E,
    NULL, LobbyReply_640F, LobbyReply_640F, NULL,
    LobbyNotify_ChatMessage, NULL, LobbyReply_ChatTell, LobbyNotify_ChatTellMessage,
    NULL, LobbyNotify_TownMsg, NULL, LobbyReply_TownMsgTo,
    LobbyNotify_TownMsgToRecv, NULL, NULL, LobbyReply_UserStatus,
    NULL, LobbyReply_ChatColorMode, NULL, LobbyReply_UserProfile,
    NULL, LobbyReply_UserSearch, NULL, NULL,
    LobbyReply_UserSearch, NULL, LobbyReply_Level2Quest, NULL,
    LobbyReply_6416, NULL, NULL, LobbyReply_6417,
    NULL, LobbyReply_UserFindByName, NULL, NULL,
    LobbyReply_UserCardFetch, NULL, LobbyReply_Level1SetGroupInfo, NULL,
    LobbyReply_Level1Search, NULL, NULL, LobbyReply_Level1SearchInfo,
    NULL, LobbyReply_ShortMailSend, LobbyNotify_ShortMail, LobbySrvReq_6706,
    NULL, NULL, LobbyReply_68A0, NULL,
    LobbyReply_68A1, NULL, LobbyReply_ServerName, NULL,
    HttpLbs_Receive, NULL, NULL, NULL,
};
