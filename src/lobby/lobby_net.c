/*
 * The lobby protocol's message layer.
 * NETWORK: this code must stay byte-for-byte for online play.
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
 */
#include "lobby_net.h"

extern void (*gLobbyCmdHandler[])(void);
extern void (*gNetHandlers[])(NetMsg m, NetMsg *pm);
extern struct {
    u16 seq;
} gLobbyCmdSeq;
typedef struct LobbyBig {
    u8 b[0x294A4];
} LobbyBig;
typedef struct LobbyRec124 {
    u8 b[0x124];
} LobbyRec124;

void LobbyMsg_ReadErrText(void);
void Net_CallHandler(s32 i, s32 b);
extern LobbyNet gLobbyNet;
extern u8 gLobbyCmdHi[];
extern u8 gLobbyCmdLo[];
extern u8 gLobbyCmdKind[];
extern u8 gLobbyCmdFlag[];
extern LobbyHdr gLobbyRecvHdr;
char *strcpy(void *dst, const void *src);
s32 LobbyPkt_EntryFlag(u16 kind, u16 arg1);
s32 LobbyPkt_6002(void);
s32 LobbyPkt_6006(void);
void LobbyMsg_ReadU16U8b2(void *a, void *b);
s32 Lobby_RecvPacket(s32 sock, u8 *hdr, u8 *data, u32 max);

s32 NetTask6_GetRecs(s32 a, NetRecs2 *dst)
{
    *dst = gLobbyNet.recs2;
    return 0;
}

void LobbyReply_6507(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6418(u16 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryFlag(2, v);
        return i;
    }
    return -1;
}

/* the reply to 6418 and its notification: u16 id, u8; route 0x36 */
void LobbyReply_6418(void)
{
    u8 v;
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b2(&id, &v);
        gLobbyNet.list2[id - 1].id = id;
        gLobbyNet.list2[id - 1].kind = v;
        gLobbyNet.list2[id - 1].flags |= 0x400;
    }
    Lobby_RouteMessage(0x36);
}

void Net_SetHandler(s32 i, void (*f)(NetMsg, NetMsg *))
{
    gNetHandlers[i] = f;
}

void Net_CallHandler(s32 i, s32 b)
{
    NetMsg m;

    m.flag = i;
    m.kind = 1;
    if (gNetHandlers[(u16)i] != NULL) {
        gNetHandlers[(u16)i](m, NULL);
    }
}

void Lobby_RouteMessage(s32 a)
{
    NetMsg m;

    if (gLobbyNet.flag == 0x10 || gLobbyNet.flag == 1) {
        if (a != 0) {
            Net_CallHandler(a, 0);
        }
        return;
    }
    if (gLobbyNet.flag == 2) {
        if (gLobbyNet.replyErr == 0) {
            m.kind = 0;
        } else {
            m.kind = -1;
            LobbyMsg_ReadErrText();
        }
        NetReq_OnReply(m, 1, 0);
    }
}

void NetReq_ClearAll(void)
{
    memset(gLobbyNet.req, 0, sizeof(gLobbyNet.req));
}

void NetTask_ClearAll(void)
{
    memset(gLobbyNet.task, 0, sizeof(gLobbyNet.task));
}

/* a free request slot, -1 if none */
s32 NetReq_Alloc(u8 state, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    for (i = 0; i < 0x80; i++) {
        if (gLobbyNet.req[i].state == 0) {
            gLobbyNet.req[i].state = state;
            gLobbyNet.req[i].unk19 = 0;
            gLobbyNet.req[i].onReply = onReply;
            gLobbyNet.req[i].onTick = onTick;
            return i;
        }
    }
    return -1;
}

/* takes request slot i of req2 (fixed slots)
 * NON_MATCHING: the original computes the slot address in other registers */
s32 NetReq_TakeFixed(s32 i, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *))
{
    NetReq *r = &gLobbyNet.req2[i];

    r->state = 1;
    r->unk19 = 0;
    r->onReply = onReply;
    r->onTick = onTick;
    return 1;
}

/* the reply received: frees the request it answers and calls its callback;
 * returns the slot, -1 if none */
s32 NetReq_OnReply(NetMsg m, s32 a, s32 b)
{
    s32 i;
    u16 echo = gLobbyNet.echo;

    for (i = 0; i < 0x80; i++) {
        if (echo == gLobbyNet.req[i].seq) {
            gLobbyNet.req[i].state = 0;
            gLobbyNet.req[i].unk19 = 0;
            if (gLobbyNet.req[i].onReply != NULL) {
                gLobbyNet.req[i].onReply(m, &m);
            }
            return i;
        }
    }
    for (i = 0; i < 0x10; i++) {
        if (echo == gLobbyNet.req2[i].seq) {
            gLobbyNet.req2[i].state = 0;
            gLobbyNet.req2[i].unk19 = 0;
            if (gLobbyNet.req2[i].onReply != NULL) {
                gLobbyNet.req2[i].onReply(m, &m);
            }
            return i;
        }
    }
    return -1;
}

void Net_TickRequests(void)
{
    s32 i;

    for (i = 0; i < 0x80; i++) {
        if (gLobbyNet.req[i].state == 2 && gLobbyNet.req[i].onTick != NULL) {
            gLobbyNet.req[i].onTick(i);
        }
    }
    for (i = 0; i < 0x10; i++) {
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

    for (i = 0; i < 0x80; i++) {
        if (gLobbyNet.req[i].state == 0) {
            n++;
        }
    }
    return n;
}

/* the request the message received answers, -1 if none */
s32 NetReq_FindReply(void)
{
    s32 i;

    for (i = 0; i < 0x80; i++) {
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
    u16 c;

    for (i = 0; i < 0x155; i++) {
        c = gLobbyCmdHi[i] << 8;
        c |= gLobbyCmdLo[i];
        if (gLobbyCmdKind[i] != 8 && cmd == c && gLobbyCmdFlag[i] == flag && gLobbyCmdHandler[i] != NULL) {
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
    for (i = 0; i < 4; i++) {
        gLobbyNet.recvResult = Lobby_RecvPacket(gLobbyNet.sock, (u8 *)&gLobbyRecvHdr, gLobbyRecvData, 0x1000);
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

s32 LobbyReq_6002(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6002();
        return i;
    }
    return -1;
}

s32 LobbyPkt_6002(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 2);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

void LobbyReply_6002(void)
{
    NetMsg m;

    if (gLobbyNet.flag == 2) {
        if (gLobbyNet.replyErr == 0) {
            m.kind = 0;
        } else {
            m.kind = -1;
            LobbyMsg_ReadErrText();
        }
        NetReq_OnReply(m, 1, 0);
    }
}

s32 LobbyReq_6006(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6006();
        return i;
    }
    return -1;
}

s32 LobbyPkt_6006(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 4);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

void LobbyReply_6006(void)
{
    NetMsg m;

    if (gLobbyNet.flag == 2) {
        if (gLobbyNet.replyErr == 0) {
            m.kind = 0;
        } else {
            m.kind = -1;
            LobbyMsg_ReadErrText();
        }
        NetReq_OnReply(m, 1, 0);
    }
}

s32 LobbyNet_GetErrText(char *a)
{
    strcpy(a, gLobbyNet.errText);
    return 0;
}

void LobbySrvReq_6001(void)
{
    LobbyPkt_6001();
    gLobbyNet.sent6001 = 1;
}

void LobbyPkt_6001(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 1);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

void LobbyNotify_6003(void)
{
    LobbyMsg_ReadErrText();
    Lobby_RouteMessage(2);
}

void LobbyNotify_6004(void)
{
    Lobby_RouteMessage(5);
}

void LobbyNotify_6005(void)
{
    LobbyMsg_ReadErrText();
    Lobby_RouteMessage(6);
}

void LobbyNotify_6007(void)
{
    LobbyMsg_ReadErrText();
    Lobby_RouteMessage(7);
}

u8 *LobbyMsg_GetU8(void *dst, u8 *src)
{
    *(u8 *)dst = *src;
    return src + 1;
}

u8 *LobbyMsg_GetU16(void *dst, u8 *src)
{
    ((u8 *)dst)[1] = src[0];
    ((u8 *)dst)[0] = src[1];
    return src + 2;
}

u8 *LobbyMsg_GetU32(void *dst, u8 *src)
{
    ((u8 *)dst)[3] = src[0];
    ((u8 *)dst)[2] = src[1];
    ((u8 *)dst)[1] = src[2];
    ((u8 *)dst)[0] = src[3];
    return src + 4;
}

/* a string: a big-endian length, then the characters */
u8 *LobbyMsg_GetString(void *dst, u8 *src)
{
    u16 n;

    n = (u16)(src[0] << 8) | src[1];
    memcpy(dst, src + 2, n);
    ((u8 *)dst)[n] = 0;
    return src + (n + 2);
}

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

void LobbyMsg_ReadErrText(void)
{
    memset(gLobbyNet.errText, 0, 0x300);
    LobbyMsg_GetBlob(gLobbyNet.errText, 0x300, gLobbyRecvData);
}

u8 *LobbyMsg_ReadU8(void *dst)
{
    return LobbyMsg_GetU8(dst, gLobbyRecvData);
}

u8 *LobbyMsg_RecvU16(void *dst)
{
    return LobbyMsg_GetU16(dst, gLobbyRecvData);
}

u8 *LobbyMsg_ReadU32(void *dst)
{
    return LobbyMsg_GetU32(dst, gLobbyRecvData);
}

u8 *LobbyMsg_ReadU8Str(void *a, void *b)
{
    return LobbyMsg_GetString(b, LobbyMsg_GetU8(a, gLobbyRecvData));
}

u8 *LobbyMsg_ReadU8U8(void *a, void *b)
{
    return LobbyMsg_GetU8(b, LobbyMsg_GetU8(a, gLobbyRecvData));
}

u8 *LobbyMsg_ReadU16U8(void *a, void *b)
{
    return LobbyMsg_GetU8(b, LobbyMsg_GetU16(a, gLobbyRecvData));
}

u8 *LobbyMsg_ReadU16U16(void *a, void *b)
{
    return LobbyMsg_GetU16(b, LobbyMsg_GetU16(a, gLobbyRecvData));
}

u8 *LobbyMsg_ReadU16U32(void *a, void *b)
{
    return LobbyMsg_GetU32(b, LobbyMsg_GetU16(a, gLobbyRecvData));
}

u8 *LobbyMsg_ReadU8U8Str(void *a, void *b, void *c)
{
    return LobbyMsg_GetString(c, LobbyMsg_GetU8(b, LobbyMsg_GetU8(a, gLobbyRecvData)));
}

/* starts a request (magic 0x81): code and flag from the command tables;
 * returns its sequence number */
u16 LobbyPkt_BeginCmd(LobbyPkt *p, u16 cmd)
{
    memset(p->data, 0, 0x300);
    p->cmd[0] = gLobbyCmdHi[cmd];
    p->cmd[1] = gLobbyCmdLo[cmd];
    p->flag = gLobbyCmdFlag[cmd];
    p->len = 0;
    p->pos = 0;
    p->magic = 0x81;
    if (p->flag == 2) {
        gLobbyPkt.echo[0] = gLobbyRecvHdr.echo[0];
        gLobbyPkt.echo[1] = gLobbyRecvHdr.echo[1];
    } else {
        gLobbyCmdSeq.seq++;
        p->echo[0] = gLobbyCmdSeq.seq >> 8;
        p->echo[1] = gLobbyCmdSeq.seq;
    }
    p->ff[0] = 0xFF;
    p->ff[1] = 0xFF;
    p->ff[2] = 0xFF;
    p->unkC = 0;
    return gLobbyCmdSeq.seq;
}

/* starts a message (magic 0x82) with a raw code, echoing the message received */
void LobbyPkt_Begin(LobbyPkt *p, u16 cmd)
{
    s32 c;

    memset(p->data, 0, 0x300);
    c = cmd;
    p->cmd[0] = c >> 8;
    p->cmd[1] = c;
    p->len = 0;
    p->pos = 0;
    p->magic = 0x82;
    p->ff[0] = 0xFF;
    p->ff[1] = 0xFF;
    p->ff[2] = 0xFF;
    p->flag = 0;
    p->unkC = 0;
    memcpy(p->echo, gLobbyRecvHdr.echo, 2);
}

void LobbyPkt_SetFlag(LobbyPkt *p, u8 flag)
{
    p->flag = flag;
}

void func_00719440(LobbyPkt *p, u8 v)
{
    p->unkC = v;
}

/* closes the message: the payload size into the header */
void LobbyPkt_End(LobbyPkt *p)
{
    p->size[0] = p->pos >> 8;
    p->size[1] = p->pos;
}

void LobbyPkt_PutU8(LobbyPkt *p, u8 v)
{
    *((u8 *)p + p->pos + 0x10) = v;
    p->len++;
    p->pos++;
}

void LobbyPkt_PutU16(LobbyPkt *p, u16 v)
{
    s32 x = v;
    u8 *d = (u8 *)p + p->pos;

    d[0x10] = x >> 8;
    d[0x11] = x;
    p->len += 2;
    p->pos += 2;
}

void LobbyPkt_PutU32(LobbyPkt *p, u32 v)
{
    u8 *d = &p->data[p->pos];

    d[0] = v >> 24;
    d[1] = v >> 16;
    d[2] = v >> 8;
    d[3] = v;
    p->len += 4;
    p->pos += 4;
}

void LobbyPkt_PutBytes(LobbyPkt *p, void *src, u16 n)
{
    memcpy(&p->data[p->pos], src, n);
    p->len += n;
    p->pos += n;
}

/* an obfuscated blob: length + 2, the checksum, the bytes */
void LobbyPkt_PutBlob(LobbyPkt *p, void *src, u16 n)
{
    u8 buf[0x210];
    s32 sum;

    sum = Net_LockrockObfuscate(buf, src, (u16)((u16)(p->echo[0] << 8) + p->echo[1]), n, gLobbyNet.key);
    LobbyPkt_PutU16(p, n + 2);
    LobbyPkt_PutU16(p, sum);
    LobbyPkt_PutBytes(p, buf, n);
}

void LobbyPkt_PutBlob2(LobbyPkt *p, void *src, u16 n)
{
    u8 buf[0x210];
    s32 sum;

    sum = Net_LockrockObfuscate(buf, src, (u16)((u16)(p->echo[0] << 8) + p->echo[1]), n, gLobbyNet.key);
    LobbyPkt_PutU16(p, n + 2);
    LobbyPkt_PutU16(p, sum);
    LobbyPkt_PutBytes(p, buf, n);
}

void LobbyPkt_PutBytes2(LobbyPkt *p, void *src, u16 n)
{
    LobbyPkt_PutBytes(p, src, n);
}

s32 LobbyPkt_Send(LobbyPkt *p)
{
    return NetTcp_Send(gLobbyNet.sock, &p->magic, p->len + 12);
}

/* lobby_net's .data (0x7EC980-0x7ED460) */
void LobbyNotify_6003(void);
void LobbyNotify_6004(void);
void LobbyNotify_6005(void);
void LobbyNotify_6007(void);
void LobbyNotify_6010(void);
void LobbyNotify_6104(void);
void LobbyNotify_6106(void);
void LobbyNotify_6121(void);
void LobbyNotify_6122(void);
void LobbyNotify_6123(void);
void LobbyNotify_6131(void);
void LobbyNotify_6191(void);
void LobbyNotify_6193(void);
void LobbyNotify_6307(void);
void LobbyNotify_640C(void);
void LobbyNotify_6410(void);
void LobbyNotify_6411(void);
void LobbyNotify_6414(void);
void LobbyNotify_6502(void);
void LobbyNotify_6503(void);
void LobbyNotify_6505(void);
void LobbyNotify_6506(void);
void LobbyNotify_6702(void);
void LobbyNotify_6705(void);
void LobbyNotify_6708(void);
void LobbyNotify_670C(void);
void LobbyNotify_670E(void);
void LobbyNotify_6885(void);
void LobbyNotify_68B0(void);
void LobbyNotify_6910(void);
void LobbyReply_6002(void);
void LobbyReply_6006(void);
void LobbyReply_600E(void);
void LobbyReply_6132(void);
void LobbyReply_6139(void);
void LobbyReply_6143(void);
void LobbyReply_6144(void);
void LobbyReply_6145(void);
void LobbyReply_6146(void);
void LobbyReply_6148(void);
void LobbyReply_6149(void);
void LobbyReply_614C(void);
void LobbyReply_614D(void);
void LobbyReply_6181(void);
void LobbyReply_6188(void);
void LobbyReply_6190(void);
void LobbyReply_6192(void);
void LobbyReply_61A0(void);
void LobbyReply_61A1(void);
void LobbyReply_6202(void);
void LobbyReply_6203(void);
void LobbyReply_6204(void);
void LobbyReply_6205(void);
void LobbyReply_6206(void);
void LobbyReply_6207(void);
void LobbyReply_6208(void);
void LobbyReply_6209(void);
void LobbyReply_620A(void);
void LobbyReply_620B(void);
void LobbyReply_6301(void);
void LobbyReply_6302(void);
void LobbyReply_6303(void);
void LobbyReply_6304(void);
void LobbyReply_6305(void);
void LobbyReply_6306(void);
void LobbyReply_6308(void);
void LobbyReply_630A(void);
void LobbyReply_630B(void);
void LobbyReply_630C(void);
void LobbyReply_630D(void);
void LobbyReply_630E(void);
void LobbyReply_6310(void);
void LobbyReply_6311(void);
void LobbyReply_6312(void);
void LobbyReply_6313(void);
void LobbyReply_6314(void);
void LobbyReply_6315(void);
void LobbyReply_6316(void);
void LobbyReply_6317(void);
void LobbyReply_6318(void);
void LobbyReply_6319(void);
void LobbyReply_631A(void);
void LobbyReply_631B(void);
void LobbyReply_6401(void);
void LobbyReply_6402(void);
void LobbyReply_6403(void);
void LobbyReply_6404(void);
void LobbyReply_6405(void);
void LobbyReply_6406(void);
void LobbyReply_6407(void);
void LobbyReply_6408(void);
void LobbyReply_6409(void);
void LobbyReply_640A(void);
void LobbyReply_640B(void);
void LobbyReply_640D(void);
void LobbyReply_640E(void);
void LobbyReply_640F(void);
void LobbyReply_6412(void);
void LobbyReply_6413(void);
void LobbyReply_6415(void);
void LobbyReply_6416(void);
void LobbyReply_6417(void);
void LobbyReply_6501(void);
void LobbyReply_6504(void);
void LobbyReply_6509(void);
void LobbyReply_650A(void);
void LobbyReply_650B(void);
void LobbyReply_650C(void);
void LobbyReply_6601(void);
void LobbyReply_6602(void);
void LobbyReply_6603(void);
void LobbyReply_6604(void);
void LobbyReply_6605(void);
void LobbyReply_6606(void);
void LobbyReply_6607(void);
void LobbyReply_6608(void);
void LobbyReply_6609(void);
void LobbyReply_660A(void);
void LobbyReply_660B(void);
void LobbyReply_660C(void);
void LobbyReply_660E(void);
void LobbyReply_660F(void);
void LobbyReply_6610(void);
void LobbyReply_6704(void);
void LobbyReply_6707(void);
void LobbyReply_6709(void);
void LobbyReply_670B(void);
void LobbyReply_670D(void);
void LobbyReply_670F(void);
void LobbyReply_6711(void);
void LobbyReply_6712(void);
void LobbyReply_6713(void);
void LobbyReply_6881(void);
void LobbyReply_6882(void);
void LobbyReply_6883(void);
void LobbyReply_6884(void);
void LobbyReply_6890(void);
void LobbyReply_6891(void);
void LobbyReply_6892(void);
void LobbyReply_68A0(void);
void LobbyReply_68A1(void);
void LobbyReply_68C0(void);
void LobbyReply_6911(void);
void LobbyReply_6912(void);
void LobbyReply_6913(void);
void LobbyReply_6914(void);
void LobbyReply_6915(void);
void LobbyReply_6916(void);
void LobbyReply_6917(void);
void LobbySrvReq_6001(void);
void LobbySrvReq_6101(void);
void LobbySrvReq_6103(void);
void LobbySrvReq_6105(void);
void LobbySrvReq_6124(void);
void LobbySrvReq_6125(void);
void LobbySrvReq_6138(void);
void LobbySrvReq_614A(void);
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
    LobbySrvReq_6001, NULL, NULL, LobbyReply_6002,
    NULL, LobbyReply_6006, LobbyNotify_6003, LobbyNotify_6004,
    LobbyNotify_6005, LobbyNotify_6007, NULL, LobbyReply_600E,
    LobbySrvReq_6101, NULL, LobbySrvReq_6103, NULL,
    LobbyNotify_6104, LobbySrvReq_6105, NULL, LobbyNotify_6131,
    NULL, LobbyReply_6132, LobbySrvReq_6138, NULL,
    NULL, NULL, LobbyReply_6143, NULL,
    LobbyReply_614C, NULL, LobbyReply_6190, LobbyNotify_6191,
    NULL, LobbyReply_6192, LobbyNotify_6193, NULL,
    LobbyReply_61A0, NULL, LobbyReply_61A1, NULL,
    LobbyReply_6139, NULL, LobbyReply_614D, LobbySrvReq_614A,
    NULL, LobbyNotify_68B0, LobbyNotify_6106, NULL,
    LobbyReply_6144, NULL, LobbyReply_6145, NULL,
    LobbyReply_6146, LobbyReply_6202, LobbyReply_6202, NULL,
    LobbyReply_6208, NULL, LobbyReply_6209, NULL,
    LobbyReply_6207, NULL, LobbyReply_6203, NULL,
    LobbyReply_6204, NULL, LobbyReply_6205, LobbyReply_6205,
    NULL, LobbyReply_6206, LobbyReply_6206, NULL,
    LobbyReply_620A, LobbyReply_620A, NULL, LobbyReply_6306,
    LobbyNotify_6307, NULL, LobbyReply_620B, LobbyReply_620B,
    NULL, LobbyReply_6305, NULL, LobbyReply_6408,
    NULL, LobbyReply_6301, NULL, LobbyReply_6302,
    LobbyReply_6302, NULL, LobbyReply_6303, LobbyReply_6303,
    NULL, LobbyReply_6304, LobbyReply_6304, NULL,
    LobbyReply_6308, LobbyReply_6308, LobbyNotify_640C, NULL,
    LobbyReply_6310, NULL, LobbyReply_6312, NULL,
    LobbyReply_6313, NULL, LobbyReply_6314, NULL,
    LobbyReply_6315, NULL, LobbyReply_6316, NULL,
    LobbyReply_6311, NULL, LobbyReply_6317, NULL,
    LobbyReply_630B, LobbyReply_630B, NULL, LobbyReply_630C,
    NULL, LobbyReply_630D, LobbyReply_630D, NULL,
    LobbyReply_631B, LobbyReply_631B, NULL, LobbyReply_630A,
    LobbyNotify_6410, LobbyNotify_6411, NULL, LobbyReply_630E,
    LobbyReply_630E, NULL, LobbyReply_6406, NULL,
    LobbyReply_6501, NULL, NULL, LobbyReply_6401,
    NULL, LobbyReply_6402, LobbyReply_6402, NULL,
    LobbyReply_6403, LobbyReply_6403, NULL, LobbyReply_6404,
    LobbyReply_6404, NULL, LobbyReply_640D, LobbyReply_640D,
    NULL, LobbyReply_650A, LobbyReply_650A, NULL,
    LobbyReply_650C, LobbyReply_650C, NULL, LobbyReply_6405,
    LobbyReply_6405, NULL, LobbyReply_640A, NULL,
    LobbyReply_640B, LobbyReply_640B, NULL, LobbyReply_6409,
    NULL, LobbyReply_6407, NULL, LobbyReply_6601,
    NULL, LobbyReply_6602, NULL, LobbyReply_660F,
    NULL, LobbyReply_6603, NULL, LobbyReply_6604,
    NULL, LobbyReply_6605, NULL, LobbyReply_6606,
    NULL, LobbyReply_6607, NULL, LobbyReply_6608,
    NULL, LobbyReply_660E, NULL, LobbyReply_6609,
    NULL, LobbyReply_660A, NULL, LobbyReply_6610,
    NULL, LobbyReply_660B, NULL, LobbyReply_660C,
    LobbyNotify_6502, LobbyNotify_6503, LobbyNotify_6505, NULL,
    LobbyReply_6413, LobbyNotify_6414, NULL, LobbyReply_6509,
    NULL, LobbyReply_650B, NULL, LobbyReply_6507,
    NULL, LobbyReply_6418, LobbyReply_6418, NULL,
    LobbyReply_6504, NULL, LobbyReply_6412, LobbyReply_6412,
    LobbyNotify_6506, NULL, LobbyNotify_6910, NULL,
    LobbyReply_6911, NULL, LobbyReply_6912, NULL,
    LobbyReply_6914, NULL, LobbyReply_6913, NULL,
    LobbyReply_6917, NULL, NULL, LobbyReply_6915,
    NULL, LobbyReply_6916, NULL, LobbyReply_6181,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, LobbyReply_6188,
    LobbySrvReq_6189, NULL, LobbyNotify_6121, LobbyNotify_6122,
    LobbyNotify_6123, LobbySrvReq_6124, NULL, LobbySrvReq_6125,
    NULL, NULL, NULL, HttpLbs_Receive,
    NULL, LobbyReply_6881, NULL, LobbyReply_6882,
    NULL, LobbyReply_6883, LobbyNotify_6010, NULL,
    LobbyReply_6148, NULL, LobbyReply_6149, NULL,
    LobbyReply_6884, LobbyNotify_6885, NULL, LobbyReply_6890,
    LobbyReply_6890, NULL, LobbyReply_6891, NULL,
    LobbyReply_6892, LobbyReply_6892, NULL, LobbyReply_640E,
    NULL, LobbyReply_640F, LobbyReply_640F, NULL,
    LobbyNotify_6702, NULL, LobbyReply_670B, LobbyNotify_670C,
    NULL, LobbyNotify_6708, NULL, LobbyReply_670D,
    LobbyNotify_670E, NULL, NULL, LobbyReply_6707,
    NULL, LobbyReply_670F, NULL, LobbyReply_6713,
    NULL, LobbyReply_6709, NULL, NULL,
    LobbyReply_6709, NULL, LobbyReply_6415, NULL,
    LobbyReply_6416, NULL, NULL, LobbyReply_6417,
    NULL, LobbyReply_6711, NULL, NULL,
    LobbyReply_6712, NULL, LobbyReply_6318, NULL,
    LobbyReply_6319, NULL, NULL, LobbyReply_631A,
    NULL, LobbyReply_6704, LobbyNotify_6705, LobbySrvReq_6706,
    NULL, NULL, LobbyReply_68A0, NULL,
    LobbyReply_68A1, NULL, LobbyReply_68C0, NULL,
    HttpLbs_Receive, NULL, NULL, NULL,
};
