/*
 * NETWORK: the 67xx messages (users, chat), the 68xx downloads (tasks 13
 * and 14: the files the server offers, and one file fetched 0x200 bytes at
 * a time), 6884/6885 (buffers filled on the server's notice), 6148/6149,
 * 68A0/68A1/68C0, 6138/6139, the three levels' selection bytes and task 3
 * (the second level's list: 6301's count, then the information its flags
 * ask for, entry by entry).
 */
#include "common.h"
#include "lobby_net.h"

typedef struct NetUnk951BC {
    /* 0x00 */ u8 unk0[0x10];
    /* 0x10 */ u8 unk10;
    /* 0x11 */ u8 pad11[3];
    /* 0x14 */ u16 unk14[8];
} NetUnk951BC;

extern void *gLobby68C0Buf;
extern u16 gLobby68C0Size;
extern u8 gLobby6138Data[];
extern u8 gLobby6149Byte;

void LobbyPkt_6714(s32 arg0, u16 arg1, u8 arg2, u8 arg3);
void LobbyPkt_6708(s32 arg0, u16 arg1);
s32 LobbyPkt_670B(u8 arg0, void *arg1, s32 arg2, u16 arg3);
s32 LobbyPkt_670D(void *arg0, s32 arg1, u16 arg2);
void NetTask13_Run(s32 task);
void NetTask13_Steps(NetTask *t);
void NetTask13_Done(NetTask *t);
void NetTask13_Failed(NetTask *t);
void NetTask13_Step0(NetTask *t);
void NetTask13_Step1(NetTask *t);
void NetTask13_Step2(NetTask *t);
void NetTask13_Step3(NetTask *t);
void NetTask13_OnReply(NetMsg m, NetMsg *pm);
void NetTask14_Run(s32 task);
void NetTask14_Steps(NetTask *t);
void NetTask14_Done(NetTask *t);
void NetTask14_Failed(NetTask *t);
void NetTask14_Step0(NetTask *t);
void NetTask14_Step1(NetTask *t);
void NetTask14_Step2(NetTask *t);
void NetTask14_OnReply(NetMsg m, NetMsg *pm);
s32 LobbyReq_6882(u8 arg0, s32 arg1, s32 arg2, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6882(u8 arg0, s32 arg1, s32 arg2);
s32 LobbyReq_6881(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6881(void);
s32 LobbyReq_6883(u8 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6883(u8 arg0);
s32 LobbyPkt_6148(u8 arg0);
s32 LobbyPkt_6149(u8 arg0, u8 arg1, s32 arg2, u16 arg3);
s32 LobbyPkt_6884(u8 arg0);
void Bits16_Set(u16 *arg0, s32 arg1);
s32 LobbyReq2_6884(u8 i, void (*onReply)(NetMsg, NetMsg *));
void LobbyNotify_6885OnReply(NetMsg m, NetMsg *pm);
s32 LobbyPkt_68C0(void *arg0);
void NetTask3_Run(s32 task);
void NetTask3_Steps(NetTask *t);
void NetTask3_Done(NetTask *t);
void NetTask3_Failed(NetTask *t);
void NetTask3_Step0(NetTask *t);
void NetTask3_Step1(NetTask *t);
void NetTask3_Step2(NetTask *t);
void NetTask3_Step3(NetTask *t);
void NetTask3_Step4(NetTask *t);
void NetTask3_Step5(NetTask *t);
void NetTask3_Step1Reply(NetMsg m, NetMsg *pm);
void NetTask3_Step4Reply(NetMsg m, NetMsg *pm);
s32 LobbyReq_6301(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6302(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6303(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6304(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6308(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_630D(u16 arg0, void (*onReply)(NetMsg, NetMsg *)); /* unit's own declaration: with the definition's s32 (s32 id, void (*onReply)(NetMsg, NetMsg *)) (lobby/unk_7037D0.c), NetTask3_Step4 1 of 105 words differ */
s32 LobbyReq_630B(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_631B(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_ListCount(u16 kind);
s32 LobbyReq_630E(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
void LobbyMsg_ReadU16(void *a0);
s32 NetReq_TakeFixed(s32 i, void (*onTick)(s32), void (*onReply)(NetMsg, NetMsg *));
void LobbyMsg_ReadErrText(void);
void Net_CallHandler(s32 i, s32 b);
s32 NetReq_CountFree(void);
s32 NetReq_FindReply(void);
void NetReq_ClearAll(void);

void LobbyNet_Send6714(s32 src, u16 n, u8 v, u8 arg3)
{
    LobbyPkt_6714(src, n, v, arg3);
}

void LobbyNet_Get6702(Net6702 *dst)
{
    *dst = gLobbyNet.x6702;
}

/* the notification 6714 (sent): blob, u8, u8 */
void LobbyPkt_6714(s32 src, u16 n, u8 v, u8 arg3)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0x129);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)src, n);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_PutU8(&gLobbyPkt, arg3);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the notification 6702: a user (blob id 8, blob name 0x10, blob text 0x100,
 * u8 x4, u8) into gLobbyNet.x6702; route 0xB */
void LobbyNotify_6702(void)
{
    memset(&gLobbyNet.x6702, 0, 0x124);
    LobbyMsg_GetU8(&gLobbyNet.x6702.unk120, LobbyMsg_GetU8(gLobbyNet.x6702.r.color + 3, LobbyMsg_GetU8(gLobbyNet.x6702.r.color + 2, LobbyMsg_GetU8(gLobbyNet.x6702.r.color + 1, LobbyMsg_GetU8(gLobbyNet.x6702.r.color, LobbyMsg_GetBlob(gLobbyNet.x6702.r.u.text, 0x100, LobbyMsg_GetBlob(gLobbyNet.x6702.r.u.name, 0x10, LobbyMsg_GetBlob(&gLobbyNet.x6702, 8, gLobbyRecvData))))))));
    Lobby_RouteMessage(0xB);
}

void LobbyNet_Send6708(s32 src, u16 n)
{
    LobbyPkt_6708(src, n);
}

void LobbyNet_Get6708(Net6708 *dst)
{
    *dst = gLobbyNet.x6708;
}

/* the notification 6708 (sent): a chat message (blob) */
void LobbyPkt_6708(s32 src, u16 n)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, 0x124);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the notification 6708 (received): blob id (8), blob message (0x300); route 0xD */
void LobbyNotify_6708(void)
{
    LobbyMsg_GetBlob(gLobbyNet.x6708.text, 0x300, LobbyMsg_GetBlob(&gLobbyNet.x6708, 8, gLobbyRecvData));
    Lobby_RouteMessage(0xD);
}

s32 LobbyReq_670B(u8 v, void *src, s32 text, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_670B(v, src, text, n);
        return i;
    }
    return -1;
}

/* 670B: blob id (6), blob text, u8 */
s32 LobbyPkt_670B(u8 v, void *src, s32 text, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x121) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)text, n);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 670B */
void LobbyReply_670B(void)
{
    Lobby_RouteMessage(0);
}

/* the notification 670C: the same user record as 6702's; route 0xC */
void LobbyNotify_670C(void)
{
    memset(&gLobbyNet.x6702, 0, 0x124);
    LobbyMsg_GetU8(&gLobbyNet.x6702.unk120, LobbyMsg_GetU8(gLobbyNet.x6702.r.color + 3, LobbyMsg_GetU8(gLobbyNet.x6702.r.color + 2, LobbyMsg_GetU8(gLobbyNet.x6702.r.color + 1, LobbyMsg_GetU8(gLobbyNet.x6702.r.color, LobbyMsg_GetBlob(gLobbyNet.x6702.r.u.text, 0x100, LobbyMsg_GetBlob(gLobbyNet.x6702.r.u.name, 0x10, LobbyMsg_GetBlob(&gLobbyNet.x6702, 8, gLobbyRecvData))))))));
    Lobby_RouteMessage(0xC);
}

s32 LobbyReq_670D(void *src, void *data, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_670D(src, (s32) data, n);
        return i;
    }
    return -1;
}

/* 670D: blob id (6), blob text */
s32 LobbyPkt_670D(void *src, s32 data, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x126) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)data, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 670D */
void LobbyReply_670D(void)
{
    Lobby_RouteMessage(0);
}

/* the notification 670E: blob id (8), blob message (0x300); route 0xD */
void LobbyNotify_670E(void)
{
    LobbyMsg_GetBlob(gLobbyNet.x6708.text, 0x300, LobbyMsg_GetBlob(&gLobbyNet.x6708, 8, gLobbyRecvData));
    Lobby_RouteMessage(0xD);
}

/* the reply to 6181 */
void LobbyReply_6181(void)
{
    Lobby_RouteMessage(0);
}

/* the reply to 6188 */
void LobbyReply_6188(void)
{
    Lobby_RouteMessage(0);
}

/* starts task 13: the files offered for download (6881, then 6883 for each) */
s32 NetTask13_Start(void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *t = &gLobbyNet.task[13];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        gLobbyNet.dlCount = 0;
        memset(gLobbyNet.dlFiles, 0, 0x100);
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask13_Run;
        return 0;
    }
    return -1;
}

void NetTask13_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[13];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask13_Steps(t);
            break;
        case 1:
            NetTask13_Done(t);
            break;
        case 2:
            NetTask13_Failed(t);
            break;
        }
    }
}

void NetTask13_Steps(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask13_Step0(t);
        break;
    case 1:
        NetTask13_Step1(t);
        break;
    case 2:
        NetTask13_Step2(t);
        break;
    case 3:
        NetTask13_Step3(t);
        break;
    }
}

void NetTask13_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

void NetTask13_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = -1;
    t->onDone(m, &m);
}

/* task 13, step 0: 6881 */
void NetTask13_Step0(NetTask *t)
{
    t->step++;
    t->reply = 0;
    LobbyReq_6881(NetTask13_OnReply);
}

/* task 13, step 1: 6881's reply; nothing offered is a failure */
void NetTask13_Step1(NetTask *t)
{
    if (t->reply == 1) {
        t->reply = 0;
        if (gLobbyNet.dlCount == 0) {
            t->state = 2;
        } else {
            t->step++;
            t->idx = 0;
            t->pos = 0;
            t->reqCount = 0;
        }
    } else if (t->reply == 2) {
        t->state = 2;
    }
}

/* task 13, step 2: 6883 for each file, one a frame while a request slot is free */
void NetTask13_Step2(NetTask *t)
{
    u8 i = t->pos;

    if (NetReq_CountFree() > 0) {
        LobbyReq_6883(i, NetTask13_OnReply);
        t->pos++;
        if ((u8)(i + 1) >= gLobbyNet.dlCount) {
            t->step++;
        }
    }
}

/* task 13, step 3: the last reply */
void NetTask13_Step3(NetTask *t)
{
    if (t->reply == 1) {
        t->reply = 0;
        if (gLobbyNet.dlCount == 0) {
            t->state = 2;
        } else {
            t->state = 1;
        }
    } else if (t->reply == 2) {
        t->state = 2;
    }
}

/* task 13's reply callback */
void NetTask13_OnReply(NetMsg m, NetMsg *pm)
{
    if (m.kind == 0) {
        gLobbyNet.task[13].reply = 1;
    } else {
        gLobbyNet.task[13].reply = 2;
    }
}

/* the files offered: their number and table */
s32 LobbyNet_GetDlFiles(u8 *count, NetDlFile **files)
{
    *count = gLobbyNet.dlCount;
    *files = gLobbyNet.dlFiles;
    return 0;
}

/* starts task 14: downloads file `file` (size bytes) into buf with 6882 */
s32 NetTask14_Start(u8 file, u8 *buf, s32 size, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *t = &gLobbyNet.task[14];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        gLobbyNet.dlFile = file;
        gLobbyNet.dlBuf = buf;
        gLobbyNet.dlSize = size;
        gLobbyNet.dlGot = 0;
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask14_Run;
        return 0;
    }
    return -1;
}

void NetTask14_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[14];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask14_Steps(t);
            break;
        case 1:
            NetTask14_Done(t);
            break;
        case 2:
            NetTask14_Failed(t);
            break;
        }
    }
}

void NetTask14_Steps(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask14_Step0(t);
        break;
    case 1:
        NetTask14_Step1(t);
        break;
    case 2:
        NetTask14_Step2(t);
        break;
    }
}

void NetTask14_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

void NetTask14_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = -1;
    t->onDone(m, &m);
}

/* task 14, step 0: nothing to fetch is a failure */
void NetTask14_Step0(NetTask *t)
{
    if (gLobbyNet.dlSize != 0) {
        t->step++;
        t->reply = 0;
        t->pos = 0;
        return;
    }
    t->state = 2;
    t->step = 0;
}

/* task 14, step 1: 6882 for the next 0x200 bytes, one a frame while a request slot is free */
void NetTask14_Step1(NetTask *t)
{
    u32 off;
    u32 size;
    u32 n;

    if (NetReq_CountFree() > 0) {
        off = t->pos;
        size = gLobbyNet.dlSize;
        if (off + 0x200 < size) {
            t->pos = off + 0x200;
            n = 0x200;
        } else {
            t->step++;
            n = size - off;
        }
        LobbyReq_6882(gLobbyNet.dlFile, off, n, NetTask14_OnReply);
    }
}

/* task 14, step 2: done once every byte arrived */
void NetTask14_Step2(NetTask *t)
{
    if (t->reply == 1) {
        if (gLobbyNet.dlSize <= gLobbyNet.dlGot) {
            t->state = 1;
            t->step = 0;
        }
    } else if (t->reply & 0x80) {
        t->reply = 0;
        t->state = 2;
        t->step = 0;
    }
}

/* task 14's reply callback */
void NetTask14_OnReply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[14];

    if (m.kind == 0) {
        t->reply |= 1;
    } else {
        t->reply |= 0x80;
    }
}

s32 LobbyReq_6882(u8 v, s32 off, s32 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6882(v, off, n);
        return i;
    }
    return -1;
}

/* the reply to 6882: u8 file, u32 offset, blob (u32 size) into the download
 * buffer. NON_MATCHING: the original loads gLobbyNet.dlBuf before gLobbyNet.dlGot */
/* the reply to 6882: u8 file, u32 offset, blob (u32 size) into the download buffer */
void LobbyReply_6882(void)
{
    u32 off;
    u32 n;
    u8 x;
    u8 *p;
    u8 *dst;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU32(&n, LobbyMsg_GetU32(&off, LobbyMsg_GetU8(&x, gLobbyRecvData)));
        dst = gLobbyNet.dlBuf + off;
        gLobbyNet.dlGot += n;
        LobbyMsg_GetBlob(dst, n, p);
    }
    Lobby_RouteMessage(0);
}

/* 6882: u8 file, u32 offset, u32 length */
s32 LobbyPkt_6882(u8 v, s32 off, s32 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x106) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_PutU32(&gLobbyPkt, off);
    LobbyPkt_PutU32(&gLobbyPkt, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyReq_6881(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6881();
        return i;
    }
    return -1;
}

/* the reply to 6881: u8 count, a u32 for each file */
void LobbyReply_6881(void)
{
    s32 i;
    NetDlFile *f = gLobbyNet.dlFiles;
    u8 n;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU8(&n, gLobbyRecvData);
        gLobbyNet.dlCount = n;
        for (i = 0; i < n; i++) {
            p = LobbyMsg_GetU32(&f->a, p);
            f++;
        }
    }
    Lobby_RouteMessage(0);
}

/* 6881 */
s32 LobbyPkt_6881(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x104) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyReq_6883(u8 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6883(v);
        return i;
    }
    return -1;
}

/* 6883: u8 file */
s32 LobbyPkt_6883(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x108) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6883: u8 file, u32, u32 */
void LobbyReply_6883(void)
{
    u32 a;
    u32 b;
    u8 i;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU32(&b, LobbyMsg_GetU32(&a, LobbyMsg_GetU8(&i, gLobbyRecvData)));
        gLobbyNet.dlFiles[i].a = a;
        gLobbyNet.dlFiles[i].b = b;
    }
    Lobby_RouteMessage(0);
}

/* the notification 6010: an error text; route 0x2D */
void LobbyNotify_6010(void)
{
    LobbyMsg_ReadErrText();
    Lobby_RouteMessage(0x2D);
}

/* the reply to 68A0: u8 n (at most 3), four u8 */
/* the reply to 68A0: u8 n (at most 3), four u8 */
void LobbyReply_68A0(void)
{
    u8 *d;
    u8 n;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU8(&n, gLobbyRecvData);
        if (n > 3) {
            n = 3;
        }
        d = (u8 *)gLobbyNet.x68A0;
        gLobbyNet.x68A0n = n;
        p = LobbyMsg_GetU8(d, p);
        p = LobbyMsg_GetU8(d + 1, p);
        p = LobbyMsg_GetU8(d + 2, p);
        LobbyMsg_GetU8(d + 3, p);
    }
    Lobby_RouteMessage(0);
}

/* the reply to 68A1: a string */
void LobbyReply_68A1(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetString(gLobbyNet.x68A1Str, gLobbyRecvData);
    }
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6148(u8 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6148(v);
        return i;
    }
    return -1;
}

/* 6148: u8 */
s32 LobbyPkt_6148(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x10B) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6148: u8 */
void LobbyReply_6148(void)
{
    gLobby6149Byte = 0;
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU8(&gLobby6149Byte, gLobbyRecvData);
    }
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6149(u8 v, u8 subStep2, s32 src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6149(v, subStep2, src, n);
        return i;
    }
    return -1;
}

/* 6149: u8, u8, blob */
s32 LobbyPkt_6149(u8 v, u8 subStep2, s32 src, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x10D) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_PutU8(&gLobbyPkt, subStep2);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6149: u8 */
void LobbyReply_6149(void)
{
    gLobby6149Byte = 0;
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU8(&gLobby6149Byte, gLobbyRecvData);
    }
    Lobby_RouteMessage(0);
}

s32 LobbyNet_Get6149(u8 *out)
{
    *out = gLobby6149Byte;
    return 0;
}

/* 6884 for slot i: its reply goes to dst (at most max bytes) */
s32 LobbyReq_6884(u8 i, u8 *dst, s32 max, void (*onReply)(NetMsg, NetMsg *))
{
    s32 k;
    s32 seq;

    k = NetReq_Alloc(1, NULL, onReply);
    if (k != -1) {
        gLobbyNet.x6884Buf = dst;
        gLobbyNet.x6884Max = max;
        seq = LobbyPkt_6884(i);
        gLobbyNet.req[k].seq = seq;
        gLobbyNet.req[k].unk1A = 0;
        return k;
    }
    return -1;
}

/* 6884: u8 slot */
s32 LobbyPkt_6884(u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x10F) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

void Bits16_Set(u16 *slotMask, s32 i)
{
    *slotMask |= (1 << (i & 0xFFFF)) & 0xFFFF;
}

/* the reply to 6884: u8 slot, blob; for a request of the first list, or a
 * slot of the second (gLobbyNet.req2) */
void LobbyReply_6884(void)
{
    u8 i;
    s32 k;
    u8 *p;

    gLobbyNet.x6884Slot = 0;
    if (NetReq_FindReply() != -1) {
        if (gLobbyNet.replyErr == 0) {
            LobbyMsg_GetBlob(gLobbyNet.x6884Buf, gLobbyNet.slotMax, LobbyMsg_GetU8(&gLobbyNet.x6884Slot, gLobbyRecvData));
        }
    } else if (gLobbyNet.replyErr == 0) {
        for (k = 0; k < 0x10; k++) {
            if (gLobbyNet.echo == gLobbyNet.req2[k].seq) {
                p = LobbyMsg_GetU8(&i, gLobbyRecvData);
                if (gLobbyNet.slotBuf[i] != NULL) {
                    LobbyMsg_GetBlob(gLobbyNet.slotBuf[i], gLobbyNet.slotMax, p);
                    Bits16_Set((u16 *)&gLobbyNet.slotMask, i);
                }
                break;
            }
        }
    }
    Lobby_RouteMessage(0);
}

s32 LobbyNet_Get6884(u8 *slot)
{
    *slot = gLobbyNet.x6884Slot;
    return 0;
}

/* registers the buffer of slot i (the notification 6885 fills it) */
s32 LobbyNet_SetSlotBuf(u8 i, char *buf, s32 n)
{
    *(s32 *)((u8 *)gLobbyNet.slotBuf + ((i & 0xFF) * 4)) = (s32) buf;
    gLobbyNet.slotMax = n;
    gLobbyNet.unk4A912 = 0;
    gLobbyNet.slotMask = 0;
    memset(gLobbyNet.req2, 0, 0x1C0);
    return 0;
}

/* 6884 for slot i on gLobbyNet.req2[i] */
s32 LobbyReq2_6884(u8 i, void (*onReply)(NetMsg, NetMsg *))
{
    NetReq_TakeFixed(i, NULL, onReply);
    gLobbyNet.req2[i].seq = LobbyPkt_6884(i);
    return 1;
}

void LobbyNotify_6885OnReply(NetMsg m, NetMsg *pm)
{
    if (m.kind == 0) {
        Net_CallHandler(0x30, 0);
    }
}

/* the notification 6885: u16, a bit for each slot to fetch (6884) */
void LobbyNotify_6885(void)
{
    u16 mask = 0;
    s32 i;

    LobbyMsg_GetU16(&mask, gLobbyRecvData);
    if (mask != 0) {
        gLobbyNet.slotMask = 0;
    }
    for (i = 0; i < 0x10; i++) {
        if (((mask >> i) & 1) && gLobbyNet.slotBuf[i] != NULL) {
            LobbyReq2_6884(i, LobbyNotify_6885OnReply);
        }
    }
}

s32 LobbyReq_68C0(void *src, void *buf, s32 size, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobby68C0Buf = buf;
        gLobby68C0Size = (s16) size;
        gLobbyNet.req[req].seq = LobbyPkt_68C0(src);
        return req;
    }
    return -1;
}

/* 68C0: blob (0xC) */
s32 LobbyPkt_68C0(void *src)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x151) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, 0xC);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 68C0: a blob */
void LobbyReply_68C0(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetBlob(gLobby68C0Buf, gLobby68C0Size, gLobbyRecvData);
    }
    Lobby_RouteMessage(0);
}

/* the replies to 6144, 6145, 6146 */
void LobbyReply_6144(void)
{
}

void LobbyReply_6145(void)
{
}

void LobbyReply_6146(void)
{
}

/* the reply to the server's 6138: 0x5678, blob (0xF), u8 0, u8 0, u8, eight u16;
 * or only the flag byte 0xFF */
void LobbyPkt_6138(void)
{
    NetUnk951BC *p = (NetUnk951BC *)gLobby6138Data;

    LobbyPkt_BeginCmd(&gLobbyPkt, 0x17);
    if (p->unk10 != 0) {
        LobbyPkt_PutU16(&gLobbyPkt, 0x5678);
        LobbyPkt_PutBlob(&gLobbyPkt, p, 0xF);
        LobbyPkt_PutU8(&gLobbyPkt, 0);
        LobbyPkt_PutU8(&gLobbyPkt, 0);
        LobbyPkt_PutU8(&gLobbyPkt, p->unk10);
        LobbyPkt_PutU16(&gLobbyPkt, p->unk14[0]);
        LobbyPkt_PutU16(&gLobbyPkt, p->unk14[1]);
        LobbyPkt_PutU16(&gLobbyPkt, p->unk14[2]);
        LobbyPkt_PutU16(&gLobbyPkt, p->unk14[3]);
        LobbyPkt_PutU16(&gLobbyPkt, p->unk14[4]);
        LobbyPkt_PutU16(&gLobbyPkt, p->unk14[5]);
        LobbyPkt_PutU16(&gLobbyPkt, p->unk14[6]);
        LobbyPkt_PutU16(&gLobbyPkt, p->unk14[7]);
    } else {
        func_00719440(&gLobbyPkt, 0xFF);
    }
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* 6139: 0x7654, bytes */
s32 LobbyPkt_6139(void *src, s32 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x27) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, 0x7654);
    LobbyPkt_PutBytes2(&gLobbyPkt, src, n & 0xFFFF);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* reads 6417's reply (NetInfo6417) into gLobbyNet.x6417 */
/* reads 6417's reply (NetInfo6417) into gLobbyNet.x6417 */
void LobbyReply_6417Read(void)
{
    NetInfo6417 *r = (NetInfo6417 *)&gLobbyNet.x6417;
    u16 n;
    u8 *p;
    s32 i;

    p = LobbyMsg_GetU16(&r->unk0, gLobbyRecvData);
    p = LobbyMsg_GetU16(&r->unk2, p);
    p = LobbyMsg_GetU8(&r->unk4[0], p);
    p = LobbyMsg_GetU8(&r->unk4[1], p);
    p = LobbyMsg_GetU8(&r->unk4[2], p);
    p = LobbyMsg_GetU8(&r->unk4[3], p);
    p = LobbyMsg_GetU16(&n, p);
    p = LobbyMsg_GetU16(&r->unkA, p);
    p = LobbyMsg_GetU8(&r->unk10D, p);
    if (n > 4) {
        n = 4;
    }
    r->n = n;
    p = LobbyMsg_GetString(r->text, p);
    for (i = 0; i < n; i++) {
        p = LobbyMsg_GetString(r->members[i].id, p);
        p = LobbyMsg_GetString(r->members[i].name, p);
        p = LobbyMsg_GetU8(&r->members[i].unk19, p);
        p = LobbyMsg_GetU8(&r->members[i].unk1A, p);
    }
}

/* reads 631A's reply (NetInfo631A) into gLobbyNet.x631A */
/* reads 631A's reply (NetInfo631A) into gLobbyNet.x631A */
void LobbyReply_631ARead(void)
{
    s32 i;
    NetInfo631A *r = (NetInfo631A *)&gLobbyNet.x631A;
    u16 n;
    u8 buf[0x10];
    u8 *p;

    p = LobbyMsg_GetString(r->name, gLobbyRecvData);
    p = LobbyMsg_GetU16(&r->unk10, p);
    p = LobbyMsg_GetU16(&r->unk12, p);
    p = LobbyMsg_GetString(r->text14, p);
    p = LobbyMsg_GetString(r->text58, p);
    p = LobbyMsg_GetU16(&r->unkA0[0], p);
    p = LobbyMsg_GetU16(&r->unkA0[1], p);
    p = LobbyMsg_GetU16(&r->unkA0[2], p);
    p = LobbyMsg_GetU16(&r->unkA0[3], p);
    p = LobbyMsg_GetU8(&r->unk9C[0], p);
    p = LobbyMsg_GetU8(&r->unk9C[1], p);
    p = LobbyMsg_GetU8(&r->unk9C[2], p);
    p = LobbyMsg_GetU8(&r->unk9C[3], p);
    p = LobbyMsg_GetString(r->textB0, p);
    p = LobbyMsg_GetU16(&n, p);
    p = LobbyMsg_GetU16(&r->unkAA, p);
    p = LobbyMsg_GetU16(&r->unkAC, p);
    if (n > 10) {
        n = 10;
    }
    r->n = n;
    for (i = 0; i < n; i++) {
        p = LobbyMsg_GetString(r->users[i].id, p);
        p = LobbyMsg_GetString(r->users[i].name, p);
        p = LobbyMsg_GetBlob(buf, 0x10, p);
        r->users[i].unk1C = (buf[1] << 8) | buf[0];
        r->users[i].unk1E = buf[3];
    }
}

/* the selection byte of level 0-2 */
void NetList_Select(s32 arg0, u16 id)
{
    s32 level;

    level = arg0 & 0xFF;
    switch (level) {
    case 0:
        gLobbyNet.sel[0] = id;
        break;
    case 1:
        gLobbyNet.sel[1] = id;
        break;
    case 2:
        gLobbyNet.sel[2] = id;
        break;
    }
}

void NetList_Leave(u8 level)
{
    switch (level) {
    case 0:
        gLobbyNet.cur[0] = 0;
        break;
    case 1:
        gLobbyNet.cur[1] = 0;
        break;
    case 2:
        gLobbyNet.cur[2] = 0;
        break;
    }
}

void NetList_Enter(u8 level)
{
    switch (level) {
    case 0:
        gLobbyNet.cur[0] = gLobbyNet.sel[0];
        break;
    case 1:
        gLobbyNet.cur[1] = gLobbyNet.sel[1];
        break;
    case 2:
        gLobbyNet.cur[2] = gLobbyNet.sel[2];
        break;
    }
}

s32 LobbyNet_GetListReplies(u16 *nReplies)
{
    *nReplies = gLobbyNet.nReplies;
    return 0;
}

/* starts task 3: the second level's list, with the information flags asks for
 * (bits: 1 6303, 2 6304, 4 6302, 8 6308, 0x10 630B, 0x80 630D, 0x200 631B,
 * 0x400 630E) */
s32 NetTask3_Start(s32 unused, s32 flags, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *t = &gLobbyNet.task[3];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask3_Run;
        t->reqFlags = flags & 0x68F;
        return 0;
    }
    return -1;
}

void NetTask3_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[3];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask3_Steps(t);
            break;
        case 1:
            NetTask3_Done(t);
            break;
        case 2:
            NetTask3_Failed(t);
            break;
        }
    }
}

void NetTask3_Steps(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask3_Step0(t);
        return;
    case 1:
        NetTask3_Step1(t);
        return;
    case 2:
        NetTask3_Step2(t);
        return;
    case 3:
        NetTask3_Step3(t);
        return;
    case 4:
        NetTask3_Step4(t);
        return;
    case 5:
        NetTask3_Step5(t);
    default:
        return;
    }
}

void NetTask3_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

void NetTask3_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = -1;
    t->onDone(m, &m);
}

/* task 3, step 0: counts the requests per entry, clears the list */
void NetTask3_Step0(NetTask *t)
{
    u32 flags;
    u32 i;

    t->step++;
    t->idx = 1;
    t->pos = 1;
    t->reply = 0;
    flags = t->reqFlags;
    t->reqCount = 0;
    for (i = 0; i < 0x20; i++) {
        if (flags & 1) {
            t->reqCount++;
        }
        flags >>= 1;
    }
    gLobbyNet.nReplies = 0;
    gLobbyNet.count1 = 0;
    memset(gLobbyNet.list1, 0, 0x459C);
}

/* task 3, step 1: 6301 (the count) */
void NetTask3_Step1(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_6301(NetTask3_Step1Reply);
    }
}

/* task 3, step 2: 6301's reply; tells onDone how many replies to expect (kind 2) */
void NetTask3_Step2(NetTask *t)
{
    NetMsg m;

    if (t->reply == 1) {
        t->step++;
        t->reply = 0;
        gLobbyNet.nReplies = gLobbyNet.count1 * t->reqCount;
        m.kind = 2;
        m.flag = 10;
        t->onDone(m, &m);
    } else if (t->reply == 2) {
        t->state = 2;
        t->step = 0;
    }
}

/* task 3, step 3 */
void NetTask3_Step3(NetTask *t)
{
    t->step++;
    t->reply = 0;
    if (gLobbyNet.count1 == 0) {
        t->state = 1;
        t->step = 0;
    }
}

/* task 3, step 4: the requests of one entry, when enough request slots are free */
void NetTask3_Step4(NetTask *t)
{
    u32 id;

    if (t->reply & 0x80) {
        t->state = 2;
        NetReq_ClearAll();
        return;
    }
    id = t->pos;
    if (NetReq_CountFree() >= t->reqCount) {
        if (t->reqFlags & 1) {
            LobbyReq_6303(id, NetTask3_Step4Reply);
        }
        if (t->reqFlags & 2) {
            LobbyReq_6304(id, NetTask3_Step4Reply);
        }
        if (t->reqFlags & 4) {
            LobbyReq_6302(id, NetTask3_Step4Reply);
        }
        if (t->reqFlags & 8) {
            LobbyReq_6308(id, NetTask3_Step4Reply);
        }
        if (t->reqFlags & 0x10) {
            LobbyReq_630B(id, NetTask3_Step4Reply);
        }
        if (t->reqFlags & 0x80) {
            LobbyReq_630D(id, NetTask3_Step4Reply);
        }
        if (t->reqFlags & 0x200) {
            LobbyReq_631B(id, NetTask3_Step4Reply);
        }
        if (t->reqFlags & 0x400) {
            LobbyReq_630E(id, NetTask3_Step4Reply);
        }
        t->pos++;
        if (id + 1 > gLobbyNet.count1 || id + 1 > 0x2D) {
            t->step++;
        }
    }
}

/* task 3, step 5: done once every entry has all the information asked for */
void NetTask3_Step5(NetTask *t)
{
    s32 ok = 1;
    u32 j;
    u32 i = 0;

    for (j = 0; j < 0x2D; j++) {
        i++;
        if (i > gLobbyNet.count1) {
            break;
        }
        if ((t->reqFlags & gLobbyNet.list1[i - 1].flags) != t->reqFlags) {
            ok = 0;
            break;
        }
    }
    if (t->reply & 0x80) {
        t->state = 2;
        t->step = 0;
        NetReq_ClearAll();
    } else if (ok) {
        t->state = 1;
        t->step = 0;
    }
}

/* task 3's reply callback for 6301 */
void NetTask3_Step1Reply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[3];

    if (m.kind == 0) {
        t->reply |= 1;
    } else {
        t->reply |= 0x80;
    }
}

/* task 3's reply callback for an entry's requests: each reply is reported to onDone (kind 2) */
void NetTask3_Step4Reply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[3];
    NetMsg n;

    if (m.kind == 0) {
        t->reply |= 1;
        n.kind = 2;
        n.flag = 11;
        t->onDone(n, &n);
    } else {
        t->reply |= 0x80;
    }
}

s32 LobbyReq_6301(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ListCount(1);
        return i;
    }
    return -1;
}

s32 NetList1_GetCount(u16 *count1)
{
    *count1 = gLobbyNet.count1;
    return 0;
}

/* the reply to 6301: u16 count */
void LobbyReply_6301(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16(&gLobbyNet.count1);
    }
    Lobby_RouteMessage(0);
}
