/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "em.h"
#include "lobby_net.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"

typedef struct LobbyRec1004 {
    u8 b[0x1004];
} LobbyRec1004;
typedef struct Raw1004 {
    s16 h[0x802];
} Raw1004;

u8 * LobbyMsg_GetString(void *dst, u8 *src);
u8 * LobbyMsg_GetU32(void *dst, u8 *src);
void LobbyNotify_6104(void);
s32 LobbyNet_Send6105(u8 arg0);
s32 LobbyNet_Send6141(void);
s32 LobbyPkt_614C(void);
void LobbySrvReq_6189(void);
void LobbySrvReq_6138(void);
void LobbyPkt_6132(void);
s32 LobbyPkt_600E(void);
s32 LobbyReply_6132Read(void);
void LobbyPkt_61A0(void);
void LobbyPkt_61A1(void);
s32 LobbyNet_Get614D(s32 *arg0);
void LobbyReply_614D(void);
s32 LobbyPkt_614D(void);
void LobbyNotify_6106(void);
void LobbyNotify_6705(void);
void LobbyReply_6713(void);
void LobbySrvReq_6706(void);
s32 LobbyNet_Get6709(NetFound6709 **out);
void LobbyNet_Get6707(u16 *arg0, u16 *arg1, u16 *arg2, u8 *arg3);
void LobbyReply_6707(void);
void LobbyReply_6417(void);
s32 LobbyNet_Get6711(NetFound6711 **out);
s32 LobbyNet_Get6319(u32 **arg0);
void LobbyReply_631A(void);
s32 LobbyPkt_6508(void);
char *strcpy(void *dst, const void *src);
void Lobby_RouteMessage(s32 a);
s32 Net_ChallengeReply(char *out, s8 *ch, u16 key);
void NetLogin_Done(void);
extern LobbyHdr gLobbyRecvHdr;
extern u16 gSrvSyncDelay;
/* sub_main data it does not export (read by address) */
void LobbyMsg_ReadErrText(void);
s32 LobbyPkt_6712(void *id, u16 a, u16 b);
s32 LobbyPkt_6704(void *id, char *s);
s32 LobbyPkt_6713(void *id);
s32 LobbyPkt_6709(NetSearch6709 q);
s32 LobbyPkt_6709Next(u8 arg0);
s32 LobbyPkt_6416Next(u8 arg0);
s32 LobbyPkt_6707(void *id);
s32 LobbyPkt_670F(u8 arg0);
s32 LobbyPkt_6711(NetBlob b);
s32 LobbyPkt_6711Next(u8 arg0);
s32 LobbyPkt_6319(NetSearch6319 q);
s32 LobbyPkt_6319Next(u8 arg0);
s32 LobbyPkt_631A(u8 arg0);
s32 LobbyPkt_6139(void *arg0, s32 arg1);
void LobbyPkt_6105(u8 arg0);
void LobbyPkt_6141(void);
void LobbyPkt_6138(void);
void LobbyNotify_6705Read(void);
void LobbyReply_6713Read(void);
void LobbyReply_6707Read(void);
void LobbyReply_6417Read(void);
void LobbyReply_631ARead(void);
extern M2C_UNK gServerIdMin;
extern M2C_UNK gServerIdMax;

void LobbyNotify_6104(void) {
    if (gLobbyNet.task[0].active != 0) {
        NetLogin_Done();
    }
}

/* the server's 6105 at login: a text in parts of up to 0x1000 bytes (the
 * part received: gLobbyNet.recvResult > 1 while more follow, 1 for the last);
 * once whole, u8, u16, string, string into x6105 (task 0's onDone: kind 0, 4).
 * The size is built in n step by step (one expression goes through v1). */
void LobbySrvReq_6105(void)
{
    Net6105 *r;
    u16 n;
    s32 len;
    u8 *p;
    NetMsg m;

    if (gLobbyNet.task[0].active != 0) {
        if (gLobbyNet.recvResult >= 2) {
            n = 0x1000;
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
            r = &gLobbyNet.x6105;
            p = LobbyMsg_GetU8(&r->unk0, gLobbyNet.buf38C0C);
            p = LobbyMsg_GetU16(&r->unk2, p);
            p = LobbyMsg_GetString(r->text, p);
            LobbyMsg_GetString(r->text, p);
            m.kind = 0;
            m.flag = 4;
            gLobbyNet.task[0].onDone(m, &m);
        }
    }
}

void LobbyNet_Get6105(Net6105 *dst)
{
    *dst = gLobbyNet.x6105;
}

s32 LobbyNet_Send6105(u8 v) {
    LobbyPkt_6105(v);
    return 0;
}

void LobbyPkt_6105(u8 v)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0x12);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

s32 LobbyNet_Send6141(void) {
    LobbyPkt_6141();
    return 0;
}

void LobbyReply_6143(void)
{
    Lobby_RouteMessage(0);
}

/* 614C: the reassembly buffer reset first */
s32 LobbyReq_614C(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    gLobbyNet.rxLen = 0;
    gLobbyNet.rxSize = 0x1000;
    gLobbyNet.rxBuf = gLobbyNet.buf38C0C;
    memset(&gLobbyNet.x614C, 0, 0x1004);
    memset(gLobbyNet.buf38C0C, 0, 0x2000);
    gLobbyNet.rxLen = 0;
    gLobbyNet.rxSize = 0x1000;
    gLobbyNet.rxBuf = gLobbyNet.buf38C0C;
    memset(&gLobbyNet.x614C, 0, 0x1004);
    memset(gLobbyNet.buf38C0C, 0, 0x2000);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_614C();
        return i;
    }
    return -1;
}

s32 LobbyNet_Get614C(Net614C *dst)
{
    *dst = gLobbyNet.x614C;
    return 0;
}

s32 LobbyPkt_614C(void) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0x1B) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

/* the reply to 614C: in parts like 6105; once whole, u8, string into x614C */
void LobbyReply_614C(void)
{
    Net614C *r;
    u16 n;
    s32 len;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        if (gLobbyNet.recvResult >= 2) {
            n = 0x1000;
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
        r = &gLobbyNet.x614C;
        p = LobbyMsg_GetU8(&r->unk0, gLobbyNet.buf38C0C);
        LobbyMsg_GetString(r->text, p);
    }
    Lobby_RouteMessage(0);
}

/* the notification 6104: the login is over (task 0's onDone: kind 0, 0) */
void NetLogin_Done(void)
{
    NetMsg m;

    if (gLobbyNet.task[0].active != 0) {
        m.kind = 0;
        gLobbyNet.task[0].active = 0;
        m.flag = 0;
        gLobbyNet.task[0].state = 0;
        gLobbyNet.task[0].onDone(m, &m);
    }
}

void LobbySrvReq_6189(void) {

}

void LobbySrvReq_6138(void) {
    if (gLobbyNet.task[0].active != 0) {
        LobbyPkt_6138();
    }
}

/* 6139 */
s32 LobbyReq_6139(void *src, s32 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6139(src, n);
        return i;
    }
    return -1;
}

void LobbyReply_6139(void)
{
    Lobby_RouteMessage(0);
}

void LobbyPkt_6101(void)
{
    u8 reply[0x10];

    LobbyPkt_BeginCmd(&gLobbyPkt, 0xD);
    Net_ChallengeReply((char *) reply, (s8 *) gLobbyNet.login.id, (u16)((u16)(gLobbyPkt.echo[0] << 8) + gLobbyPkt.echo[1]));
    LobbyPkt_PutU16(&gLobbyPkt, 10);
    LobbyPkt_PutBytes(&gLobbyPkt, reply, 10);
    LobbyPkt_PutBlob2(&gLobbyPkt, gLobbyNet.login.pass, strlen(gLobbyNet.login.pass));
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

void LobbyPkt_6103(void)
{
    Net6103 *p = &gLobbyNet.x6103;

    LobbyPkt_BeginCmd(&gLobbyPkt, 0xF);
    LobbyPkt_PutU8(&gLobbyPkt, p->unk0);
    LobbyPkt_PutU8(&gLobbyPkt, p->unk1);
    LobbyPkt_PutU8(&gLobbyPkt, p->unk2);
    LobbyPkt_PutBlob(&gLobbyPkt, p->version, 10);
    LobbyPkt_PutU16(&gLobbyPkt, p->state[0]);
    LobbyPkt_PutU16(&gLobbyPkt, p->state[1]);
    LobbyPkt_PutU16(&gLobbyPkt, p->state[2]);
    LobbyPkt_PutU16(&gLobbyPkt, p->state[3]);
    LobbyPkt_PutU16(&gLobbyPkt, p->state[4]);
    LobbyPkt_PutU16(&gLobbyPkt, p->state[5]);
    LobbyPkt_PutU16(&gLobbyPkt, p->state[6]);
    LobbyPkt_PutU16(&gLobbyPkt, p->state[7]);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

void LobbyPkt_6132(void) {
    LobbyPkt_BeginCmd(&gLobbyPkt, 0x14);
    LobbyPkt_PutBlob(&gLobbyPkt, gLobbyNet.charas[3].id, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, gLobbyNet.charas[3].name, strlen(gLobbyNet.charas[3].name) & 0xFFFF);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

void LobbyPkt_6141(void)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0x18);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

s32 LobbyPkt_6190(void *src, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x1D);
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_600E(void) {
    s32 pkt;

    gLobbyNet.frames = 0;
    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0xA) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, "0", 1);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

/* the notification 6131's payload: u8 n (at most 3), then n characters
 * (blob id 8, blob name 0x10, blob text 0x100) into gLobbyNet.charas */
s32 NetLogin_ReadCharas(void)
{
    u8 n;
    u8 *p;
    s32 i;

    memset(gLobbyNet.charas, 0, 0x470);
    p = LobbyMsg_GetU8(&n, gLobbyRecvData);
    if (n > 3) {
        n = 3;
    }
    gLobbyNet.nCharas = n;
    for (i = 0; i < n; i++) {
        p = LobbyMsg_GetBlob(gLobbyNet.charas[i].id, 8, p);
        p = LobbyMsg_GetBlob(gLobbyNet.charas[i].name, 0x10, p);
        p = LobbyMsg_GetBlob(gLobbyNet.charas[i].text, 0x100, p);
    }
    return 0;
}

s32 LobbyReply_6132Read(void) {
    LobbyMsg_GetBlob(gLobbyNet.charas[3].id, 8, gLobbyRecvData);
    return 0;
}

void LobbyPkt_61A0(void) {
    LobbyPkt_BeginCmd(&gLobbyPkt, 0x23);
    LobbyPkt_PutU8(&gLobbyPkt, gLobbyNet.x6103.unk2);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the reply to 61A0: u32, u32 (task 0's onDone: kind 0, 12); an error: its
 * text, route 2 */
void LobbyReply_61A0(void)
{
    NetMsg m;

    if (gLobbyNet.task[0].active != 0) {
        if (gLobbyNet.replyErr == 0) {
            LobbyMsg_GetU32(&gLobbyNet.x61A0[1], LobbyMsg_GetU32(&gLobbyNet.x61A0[0], gLobbyRecvData));
            m.kind = 0;
            m.flag = 12;
            gLobbyNet.task[0].onDone(m, &m);
        } else {
            LobbyMsg_ReadErrText();
            Lobby_RouteMessage(2);
        }
    }
}

void LobbyPkt_61A1(void) {
    LobbyPkt_BeginCmd(&gLobbyPkt, 0x25);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the reply to 61A1: u32 a, u32 b (seconds; kept in minutes in sub_main's
 * 0x5BC4F8 and 0x5BC4F0 when not 0), u16 (sub_main's 0x5BC4E0) */
void LobbyReply_61A1(void)
{
    u32 a = 0;
    u32 b = 0;

    if (gLobbyNet.task[0].active != 0 && gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&gSrvSyncDelay, LobbyMsg_GetU32(&b, LobbyMsg_GetU32(&a, gLobbyRecvData)));
        if (a != 0) {
            gSrvQNetMaxStall = a / 60;
        }
        if (b != 0) {
            gSrvQNetPeerTimeout = b / 60;
        }
    }
}

/* 614D: the reply's string goes to dst */
s32 LobbyReq_614D(char *dst, s32 max, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.x614DDst = dst;
        gLobbyNet.x614DMax = max;
        gLobbyNet.x614D = 0;
        gLobbyNet.req[i].seq = LobbyPkt_614D();
        return i;
    }
    return -1;
}

s32 LobbyNet_Get614D(s32 *dst) {
    *dst = gLobbyNet.x614D;
    return 0;
}

void LobbyReply_614D(void) {
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&gLobbyNet.x614D, gLobbyRecvData);
        LobbyMsg_GetString(gLobbyNet.x614DDst, gLobbyRecvData);
    }
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_614D(void) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0x29) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

/* 6712 */
s32 LobbyReq_6712(void *id, u16 a, u16 b, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6712(id, a, b);
        return i;
    }
    return -1;
}

/* the reply to 6712: blob id (8), blob name (0x10), u16, u16 n, n bytes */
void LobbyReply_6712(void)
{
    Net6193 *r;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.x6712, 0, 0x122);
        r = &gLobbyNet.x6712;
        p = LobbyMsg_GetBlob(r->id, 8, gLobbyRecvData);
        p = LobbyMsg_GetBlob(r->name, 0x10, p);
        p = LobbyMsg_GetU16(&r->x, p);
        p = LobbyMsg_GetU16(&r->n, p);
        LobbyMsg_GetBytes(r->data, p, r->n);
    }
    Lobby_RouteMessage(0);
}

s32 LobbyNet_Get6712(Net6193 *dst)
{
    *dst = gLobbyNet.x6712;
    return 0;
}

s32 LobbyPkt_6712(void *id, u16 a, u16 b)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x13F);
    LobbyPkt_PutBlob(&gLobbyPkt, id, 6);
    LobbyPkt_PutU16(&gLobbyPkt, b);
    LobbyPkt_PutU16(&gLobbyPkt, a);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the notification 68B0: blob text (0x100), u16 n, blob (0x300) if n (task 0's
 * onDone: kind 0, 13) */
void LobbyNotify_68B0(void)
{
    u8 *r;
    u8 *p;
    NetMsg m;

    if (gLobbyNet.task[0].active != 0) {
        memset(&gNet68B0, 0, 0x408);
        memset(&m, 0, 6);
        r = (u8 *)&gNet68B0;
        p = LobbyMsg_GetBlob(r, 0x100, gLobbyRecvData);
        p = LobbyMsg_GetU16(r + 0x404, p);
        if (*(u16 *)(r + 0x404) != 0) {
            LobbyMsg_GetBlob(r + 0x104, 0x300, p);
        }
        m.kind = 0;
        m.flag = 13;
        gLobbyNet.task[0].onDone(m, &m);
    }
}

s32 LobbyNet_Get68B0(Net68B0 *dst)
{
    *dst = gNet68B0;
    return 0;
}

void LobbyNotify_6106(void) {
    if (gLobbyNet.task[0].active != 0) {
        LobbyMsg_GetBlob(&gServerIdMax, 0xC, LobbyMsg_GetBlob(&gServerIdMin, 0xC, gLobbyRecvData));
    }
}

void LobbySrvReq_614A(void)
{
    u32 a;
    u32 b;
    u16 n;

    LobbyMsg_GetU16((u8 *)&n, LobbyMsg_GetU32((u8 *)&b, LobbyMsg_GetU32((u8 *)&a, gLobbyRecvData)));
    LobbyPkt_BeginCmd(&gLobbyPkt, 0x2C);
    LobbyPkt_PutU32(&gLobbyPkt, a);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)b, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

void LobbyNotify_6705(void) {
    LobbyNotify_6705Read();
    Lobby_RouteMessage(3);
}

/* 6704: blob id (6), blob text */
s32 LobbyReq_6704(void *id, char *s, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6704(id, s);
        return i;
    }
    return -1;
}

void LobbyNet_Get6705(char *id, u8 *name, u8 *text)
{
    if (id != 0) {
        strcpy((void *) id, gLobbyNet.x6705.id);
    }
    if (name != NULL) {
        strcpy(name, gLobbyNet.x6705.name);
    }
    if (text != NULL) {
        strcpy(text, gLobbyNet.x6705.text);
    }
}

void LobbyReply_6704(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_6704(void *id, char *s)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x148);
    LobbyPkt_PutBlob(&gLobbyPkt, id, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, s, strlen(s));
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

void LobbyNotify_6705Read(void) {
    LobbyMsg_GetString(gLobbyNet.x6705.text, LobbyMsg_GetString(gLobbyNet.x6705.name, LobbyMsg_GetString(gLobbyNet.x6705.id, gLobbyRecvData)));
}

/* 6713: blob id (6); the reply fills gLobbyNet.x6713 */
s32 LobbyReq_6713(void *id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(&gLobbyNet.x6713, 0, 0x3CC);
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6713(id);
        return i;
    }
    return -1;
}

void LobbyNet_Get6713(Net6713 *dst)
{
    *dst = gLobbyNet.x6713;
}

void LobbyReply_6713(void) {
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_6713Read();
    }
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_6713(void *id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x12E);
    LobbyPkt_PutBlob(&gLobbyPkt, id, 6);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6713's reply: a user's details (Net6713) into gLobbyNet.x6713 */
void LobbyReply_6713Read(void)
{
    Net6713 *r;
    u8 *p;
    s32 i;

    memset(&gLobbyNet.x6707, 0, 0x20A);
    memset(&gLobbyNet.x6713, 0, 0x3CC);
    r = &gLobbyNet.x6713;
    p = LobbyMsg_GetBlob(r->s0, 0x10, gLobbyRecvData);
    p = LobbyMsg_GetBlob(r->s14, 0xC, p);
    p = LobbyMsg_GetU16(&r->h3B8, p);
    p = LobbyMsg_GetU16(&r->h3BA, p);
    p = LobbyMsg_GetU8(&r->b3C8, p);
    p = LobbyMsg_GetBlob(r->s24, 0x100, p);
    p = LobbyMsg_GetBlob(r->s128, 0x100, p);
    p = LobbyMsg_GetU32(&r->w3C4, p);
    p = LobbyMsg_GetBlob(r->s22C, 0x40, p);
    p = LobbyMsg_GetU8(&r->b3C9, p);
    p = LobbyMsg_GetBlob(r->s270, 0x40, p);
    p = LobbyMsg_GetBlob(r->s2B4, 0x100, p);
    p = LobbyMsg_GetU8(&r->b3CA, p);
    for (i = 0; i < 4; i++) {
        p = LobbyMsg_GetU8(&r->b3C0[i], p);
    }
    p = LobbyMsg_GetU16(&r->h3BC, p);
    LobbyMsg_GetU16(&r->h3BE, p);
}

void LobbySrvReq_6706(void) {
    LobbyMsg_GetString(gLobbyNet.x6705.text, LobbyMsg_GetString(gLobbyNet.x6705.name, gLobbyRecvData));
    Lobby_RouteMessage(4);
}

/* 6709: searches users (its pages fill gLobbyNet.found6709.users) */
s32 LobbyReq_6709(NetSearch6709 q, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(&gLobbyNet.found6709, 0, sizeof(NetFound6709));
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6709(q);
        gLobbyNet.searchReq = i;
        return i;
    }
    return -1;
}

s32 LobbyNet_Get6709(NetFound6709 **out)
{
    *out = &gLobbyNet.found6709;
    return 0;
}

/* 6709: u8, u8 n, n conditions (u8 type: 1 a 6-byte id, 2 a string, 3-5 a byte, 6 two bytes) */
s32 LobbyPkt_6709(NetSearch6709 q)
{
    s32 seq;
    s32 i;
    u8 n;
    u8 t;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x130);
    LobbyPkt_PutU8(&gLobbyPkt, q.unk0);
    n = q.n;
    LobbyPkt_PutU8(&gLobbyPkt, n);
    for (i = 0; i < n; i++) {
        t = q.cond[i].type;
        LobbyPkt_PutU8(&gLobbyPkt, t);
        switch (t) {
        case 0:
            break;
        case 1:
            LobbyPkt_PutBlob(&gLobbyPkt, q.cond[i].data, 6);
            break;
        case 2:
            LobbyPkt_PutBlob(&gLobbyPkt, q.cond[i].data, q.cond[i].n);
            break;
        case 3:
            LobbyPkt_PutU8(&gLobbyPkt, q.cond[i].data[0]);
            break;
        case 4:
            LobbyPkt_PutU8(&gLobbyPkt, q.cond[i].data[0]);
            break;
        case 5:
            LobbyPkt_PutU8(&gLobbyPkt, q.cond[i].data[0]);
            break;
        case 6:
            LobbyPkt_PutU8(&gLobbyPkt, q.cond[i].data[0]);
            LobbyPkt_PutU8(&gLobbyPkt, q.cond[i].data[1]);
            break;
        }
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_6709Next(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x132);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6709 (and 670A): a page of users */
void LobbyReply_6709(void)
{
    u8 total;
    u8 start;
    u8 count;
    u8 last;
    NetUser *e;
    u8 *p;
    s32 i;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU8(&total, gLobbyRecvData);
        p = LobbyMsg_GetU8(&start, p);
        p = LobbyMsg_GetU8(&count, p);
        p = LobbyMsg_GetU8(&last, p);
        e = &gLobbyNet.found6709.users[start];
        for (i = 0; i < count; i++) {
            p = LobbyMsg_GetBlob(e->id, 8, p);
            p = LobbyMsg_GetBlob(e->name, 0x10, p);
            p = LobbyMsg_GetBlob(e->text, 0x100, p);
            e++;
        }
        if (last == 0) {
            gLobbyNet.req[gLobbyNet.searchReq].seq = LobbyPkt_6709Next(count + start);
            return;
        }
        gLobbyNet.found6709.total = total;
    }
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_6416Next(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x139);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6416: a page of 16-byte records */
void LobbyReply_6416(void)
{
    u8 total;
    u8 start;
    u8 count;
    u8 last;
    Net6416Rec *e;
    u8 *p;
    s32 i;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU8(&total, gLobbyRecvData);
        p = LobbyMsg_GetU8(&start, p);
        p = LobbyMsg_GetU8(&count, p);
        p = LobbyMsg_GetU8(&last, p);
        e = &gLobbyNet.found6416.list[start];
        for (i = 0; i < count; i++) {
            p = LobbyMsg_GetU8(&e->b[0], p);
            p = LobbyMsg_GetU8(&e->b[1], p);
            p = LobbyMsg_GetU8(&e->b[2], p);
            p = LobbyMsg_GetU8(&e->b[3], p);
            p = LobbyMsg_GetU16(&e->h[0], p);
            p = LobbyMsg_GetU16(&e->h[1], p);
            p = LobbyMsg_GetU16(&e->h[2], p);
            p = LobbyMsg_GetU16(&e->h[3], p);
            p = LobbyMsg_GetU8(&e->c, p);
            e++;
        }
        if (last == 0) {
            gLobbyNet.req[gLobbyNet.searchReq].seq = LobbyPkt_6416Next(count + start);
            return;
        }
        gLobbyNet.found6416.total = total;
    }
    Lobby_RouteMessage(0);
}

/* 6707: blob id (6) */
s32 LobbyReq_6707(void *id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6707(id);
        return i;
    }
    return -1;
}

void LobbyNet_Get6707(u16 *tmp, u16 *b, u16 *c, u8 *d) {
    *tmp = gLobbyNet.x6707.a;
    *b = gLobbyNet.x6707.b;
    *c = gLobbyNet.x6707.c;
    *d = gLobbyNet.x6707.d;
}

void LobbyReply_6707(void) {
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_6707Read();
    }
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_6707(void *id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x12A);
    LobbyPkt_PutBlob(&gLobbyPkt, id, 6);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6707's reply: u16 x3, u8 */
void LobbyReply_6707Read(void)
{
    LobbyMsg_GetU8(&gLobbyNet.x6707.d, LobbyMsg_GetU16(&gLobbyNet.x6707.c, LobbyMsg_GetU16(&gLobbyNet.x6707.b, LobbyMsg_GetU16(&gLobbyNet.x6707.a, gLobbyRecvData))));
}

/* 670F: u8 */
s32 LobbyReq_670F(u8 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_670F(v);
        return i;
    }
    return -1;
}

void LobbyReply_670F(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_670F(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x12C);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

void LobbyReply_6417(void) {
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_6417Read();
    }
    Lobby_RouteMessage(0);
}

/* 6711: u8, blob (its pages fill gLobbyNet.found6711.list) */
s32 LobbyReq_6711(NetBlob b, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(&gLobbyNet.found6711, 0, sizeof(NetFound6711));
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6711(b);
        gLobbyNet.searchReq = i;
        return i;
    }
    return -1;
}

s32 LobbyNet_Get6711(NetFound6711 **out)
{
    *out = &gLobbyNet.found6711;
    return 0;
}

s32 LobbyPkt_6711(NetBlob b)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x13C);
    LobbyPkt_PutU8(&gLobbyPkt, b.kind);
    LobbyPkt_PutBlob(&gLobbyPkt, b.data, b.n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6711: a page of users (blob id, blob name, string) */
void LobbyReply_6711(void)
{
    u8 total;
    u8 start;
    u8 count;
    u8 last;
    Net6711Rec *e;
    u8 *p;
    s32 i;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU8(&total, gLobbyRecvData);
        p = LobbyMsg_GetU8(&start, p);
        p = LobbyMsg_GetU8(&count, p);
        p = LobbyMsg_GetU8(&last, p);
        e = &gLobbyNet.found6711.list[start];
        for (i = 0; i < count; i++) {
            p = LobbyMsg_GetBlob(e->id, 8, p);
            p = LobbyMsg_GetBlob(e->name, 0x10, p);
            p = LobbyMsg_GetString(e->text, p);
            e++;
        }
        if (last == 0) {
            gLobbyNet.req[gLobbyNet.searchReq].seq = LobbyPkt_6711Next(count + start);
            return;
        }
        gLobbyNet.found6711.total = total;
    }
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_6711Next(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x13E);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6319: a search of the second level's entries */
s32 LobbyReq_6319(NetSearch6319 q, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6319(q);
        return i;
    }
    return -1;
}

s32 LobbyNet_Get6319(u32 **out) {
    *out = &gLobbyNet.x6319;
    return 0;
}

/* 6319: blob (0xC), u8, u8 n, then n conditions (u8 type, u8 m, m bytes).
 * NON_MATCHING: n and MWCC's pointer into q.cond swap s1 and s2 */
/* 6319: blob (0xC), u8, u8 n, then n conditions (u8 type, u8 m, m bytes) */
s32 LobbyPkt_6319(NetSearch6319 q)
{
    s32 seq;
    s32 i;
    s32 j;
    u8 n;
    u8 m;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x143) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, q.id, 0xC);
    LobbyPkt_PutU8(&gLobbyPkt, q.unk10);
    LobbyPkt_PutU8(&gLobbyPkt, q.n);
    n = q.n;
    for (i = 0; i < n; i++) {
        LobbyPkt_PutU8(&gLobbyPkt, q.cond[i].type);
        m = q.cond[i].n;
        LobbyPkt_PutU8(&gLobbyPkt, m);
        for (j = 0; j < m; j++) {
            LobbyPkt_PutU8(&gLobbyPkt, q.cond[i].v[j]);
        }
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyPkt_6319Next(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x145);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6319: u32, then a page of entries */
void LobbyReply_6319(void)
{
    u32 x;
    u8 total;
    u8 start;
    u8 count;
    u8 last;
    Net6319Rec *e;
    u8 *p;
    s32 i;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU32(&x, gLobbyRecvData);
        p = LobbyMsg_GetU8(&total, p);
        p = LobbyMsg_GetU8(&start, p);
        p = LobbyMsg_GetU8(&count, p);
        p = LobbyMsg_GetU8(&last, p);
        e = &gLobbyNet.found6319.list[start];
        for (i = 0; i < count; i++) {
            p = LobbyMsg_GetU8(&e->b[0], p);
            p = LobbyMsg_GetU8(&e->b[1], p);
            p = LobbyMsg_GetU8(&e->b[2], p);
            p = LobbyMsg_GetU8(&e->b[3], p);
            p = LobbyMsg_GetString(e->s104, p);
            p = LobbyMsg_GetU16(&e->h118, p);
            p = LobbyMsg_GetU16(&e->h11A, p);
            p = LobbyMsg_GetString(e->text, p);
            p = LobbyMsg_GetU16(&e->h11C, p);
            p = LobbyMsg_GetU16(&e->h11E, p);
            p = LobbyMsg_GetU16(&e->h120, p);
            e->index = start + i;
            e++;
        }
        if (last == 0) {
            gLobbyNet.req[gLobbyNet.searchReq].seq = LobbyPkt_6319Next(count + start);
            return;
        }
        gLobbyNet.x6319 = x;
        gLobbyNet.found6319.total = total;
    }
    Lobby_RouteMessage(0);
}

/* 631A: u8 (the reply fills gLobbyNet.x631A) */
s32 LobbyReq_631A(u8 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(&gLobbyNet.x631A, 0, 0x2F4);
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_631A(v);
        return i;
    }
    return -1;
}

void LobbyNet_Get631A(NetInfo631A *dst)
{
    *dst = gLobbyNet.x631A;
}

s32 LobbyPkt_631A(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x146);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

void LobbyReply_631A(void) {
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_631ARead();
    }
    Lobby_RouteMessage(0);
}

void LobbyNotify_6910(void)
{
    Lobby_RouteMessage(0x1);
}

s32 LobbyPkt_6508(void) {
    LobbyPkt_BeginCmd(&gLobbyPkt, 0xDD);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return 0;
}
