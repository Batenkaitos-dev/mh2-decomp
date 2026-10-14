/*
 * NETWORK: task 6's steps (the third-level entry records, gLobbyNet.recs2):
 * stage 0 asks 6603 for the count and 6607 for each record's sub-entry count;
 * stage 1 asks 6601, 6602 and 660F what may be sent; stage 2 asks 6604, 6606,
 * 6605 for each record, then 6608 and 660E for each sub-entry. Also 6405
 * (an entry's byte) and 640A (the users of a third-level entry).
 */
#include "common.h"
#include "lobby_net.h"
char *strcpy(char *, const char *);
s32 NetReq_CountFree(void);

u8 *LobbyMsg_ReadU8(void *dst);
u8 *LobbyMsg_ReadU16U8(void *a, void *b);
void LobbyMsg_ReadUsers(u8 *n, NetUserRec *users);
s32 LobbyReq_6604(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *)); /* unit's own declaration: with the definition's s32 (s32 id, s32 i, void (*onReply)(NetMsg, NetMsg *)) (lobby/unk_716E40.c), NetTask6_S2Step1 2 of 43 words differ */
s32 LobbyReq_6607(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6606(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6605(u16 id, u8 i, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6608(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_660E(u16 id, u8 i, u8 j, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_660F(u16 id, void (*onReply)(NetMsg, NetMsg *));
void NetTask6_OnReply(NetMsg m, NetMsg *pm);
void NetTask6_OnRecReply(NetMsg m, NetMsg *pm);
s32 LobbyReq_6603(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6603(u16 id);
s32 LobbyReq_6601(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6601(u16 id);
s32 LobbyReq_6602(s32 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6602(u16 id);
s32 LobbyPkt_6405(u16 id);
s32 LobbyPkt_640A(u16 id);
void LobbyReply_640A_Users(void);

/* stage 0, step 0: clears the records */
void NetTask6_S0Step0(NetTask *t)
{
    t->step++;
    t->idx = 1;
    t->pos = 1;
    t->reply = 0;
    gLobbyNet.nReplies = 0;
    memset(&gLobbyNet.recs2, 0, sizeof(NetRecs2));
}

/* 6603: the count */
void NetTask6_S0Step1(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_6603(t->arg, NetTask6_OnReply);
    }
}

void NetTask6_S0Step2(NetTask *t)
{
    if (t->reply == 1) {
        t->reply = 0;
        if (gLobbyNet.recs2.n == 0) {
            t->state = 1;
            t->step = 0;
        } else {
            t->step++;
        }
    } else if (t->reply == 2) {
        t->state = 0xB;
        t->step = 0;
    }
}

void NetTask6_S0Step3(NetTask *t)
{
    t->step++;
    t->idx = 0;
    t->pos = 0;
}

/* 6607 for each record, one a frame */
void NetTask6_S0Step4(NetTask *t)
{
    u32 i = t->pos;

    if (NetReq_CountFree() > 0) {
        LobbyReq_6607(t->arg, i, NetTask6_OnRecReply);
        t->pos++;
        if (i + 1 >= gLobbyNet.recs2.n) {
            t->step++;
        }
    }
}

/* waits for every record's 6607 reply, then clears the flags */
void NetTask6_S0Step5(NetTask *t)
{
    s32 ok = 1;
    u32 i;
    s32 k;

    for (i = 0; i < gLobbyNet.recs2.n; i++) {
        if ((gLobbyNet.recs2.rec[i].flags & 0xF) != 4) {
            ok = 0;
            break;
        }
    }
    if (ok) {
        t->state++;
        t->step = 0;
        t->idx = 0;
        for (k = 0; k < 32; k++) {
            gLobbyNet.recs2.rec[k].flags = 0;
        }
    }
}

/* stage 1, step 0: tells onDone how many replies are coming */
void NetTask6_S1Step0(NetTask *t)
{
    NetMsg m;
    s32 sum = 0;
    s32 i;
    u8 n;

    t->step++;
    n = gLobbyNet.recs2.n;
    for (i = 0; i < n; i++) {
        sum += gLobbyNet.recs2.rec[i].m;
    }
    gLobbyNet.nReplies = n * 3 + 2 + sum * 2;
    m.kind = 2;
    m.flag = 10;
    t->onDone(m, &m);
}

/* 6601: may the name be sent */
void NetTask6_S1Step1(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_6601(t->arg, NetTask6_OnReply);
    }
}

void NetTask6_S1Step2(NetTask *t)
{
    NetMsg m;

    if (t->reply != 0) {
        t->step++;
        t->reply = 0;
        m.kind = 2;
        m.flag = 11;
        t->onDone(m, &m);
    }
}

/* 6602: may the password be sent */
void NetTask6_S1Step3(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_6602(t->arg, NetTask6_OnReply);
    }
}

void NetTask6_S1Step4(NetTask *t)
{
    NetMsg m;

    if (t->reply != 0) {
        t->step++;
        t->reply = 0;
        m.kind = 2;
        m.flag = 11;
        t->onDone(m, &m);
    }
}

/* 660F: may the text be sent */
void NetTask6_S1Step5(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_660F(t->arg, NetTask6_OnReply);
    }
}

void NetTask6_S1Step6(NetTask *t)
{
    NetMsg m;

    if (t->reply != 0) {
        t->reply = 0;
        if (gLobbyNet.recs2.n == 0) {
            t->state = 10;
            t->step = 0;
        } else {
            t->state++;
            t->step = 0;
            m.kind = 2;
            m.flag = 11;
            t->onDone(m, &m);
        }
    }
}

void NetTask6_S2Step0(NetTask *t)
{
    t->step++;
    t->idx = 0;
    t->pos = 0;
}

/* stage 2: 6604, 6606 and 6605 for each record */
void NetTask6_S2Step1(NetTask *t)
{
    u32 i = t->pos;

    if (NetReq_CountFree() >= 3) {
        LobbyReq_6604(t->arg, i, NetTask6_OnRecReply);
        LobbyReq_6606(t->arg, i, NetTask6_OnRecReply);
        LobbyReq_6605(t->arg, i, NetTask6_OnRecReply);
        t->pos++;
        if (i + 1 >= gLobbyNet.recs2.n) {
            t->step++;
        }
    }
}

/* waits for the three replies of every record, then clears the sub-entry flags */
void NetTask6_S2Step2(NetTask *t)
{
    s32 ok = 1;
    u32 i;
    s32 k;

    for (i = 0; i < gLobbyNet.recs2.n; i++) {
        if ((gLobbyNet.recs2.rec[i].flags & 0xF) != 0xB) {
            ok = 0;
            break;
        }
    }
    if (ok) {
        t->step++;
        t->idx = 0;
        for (k = 0; k < 32; k++) {
            memset(gLobbyNet.recs2.rec[k].subFlags, 0, 0x20);
        }
    }
}

void NetTask6_S2Step3(NetTask *t)
{
    t->step++;
    t->pos = 0;
}

/* 6608 and 660E for each sub-entry of record unk8 */
void NetTask6_S2Step4(NetTask *t)
{
    u32 j = t->pos;

    if (gLobbyNet.recs2.rec[t->idx].m == 0) {
        t->state = 10;
        t->step = 0;
    } else if (NetReq_CountFree() >= 2) {
        LobbyReq_6608(t->arg, t->idx, j, NetTask6_OnRecReply);
        LobbyReq_660E(t->arg, t->idx, j, NetTask6_OnRecReply);
        t->pos++;
        if (j + 1 >= gLobbyNet.recs2.rec[t->idx].m) {
            t->step++;
        }
    }
}

/* waits for the sub-entries of record unk8, then goes on with the next record */
void NetTask6_S2Step5(NetTask *t)
{
    s32 ok = 1;
    s32 j;

    for (j = 0; j < gLobbyNet.recs2.rec[t->idx].m; j++) {
        if ((gLobbyNet.recs2.rec[t->idx].subFlags[j] & 3) != 3) {
            ok = 0;
        }
    }
    if (ok) {
        t->idx++;
        if (t->idx >= gLobbyNet.recs2.n) {
            t->state = 10;
            t->step = 0;
        } else {
            t->step = 3;
        }
    }
}

/* the replies to task 6's requests */
void NetTask6_OnReply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[6];

    if (m.kind == 0) {
        t->reply = 1;
    } else {
        t->reply = 2;
    }
}

/* the replies to the per-record requests: one more reply for onDone */
void NetTask6_OnRecReply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[6];
    NetMsg p;

    if (m.kind == 0) {
        p.kind = 2;
        p.flag = 11;
        t->onDone(p, &p);
        t->reply = 1;
    } else {
        t->reply = 2;
    }
}

/* 6603 for third-level entry id */
s32 LobbyReq_6603(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6603(id);
        return i;
    }
    return -1;
}

/* 6603: u16 id */
s32 LobbyPkt_6603(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xB0);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6603: u8, the count */
void LobbyReply_6603(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8(&gLobbyNet.recs2.n);
    }
    Lobby_RouteMessage(0);
}

/* 6601 for third-level entry id */
s32 LobbyReq_6601(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6601(id);
        return i;
    }
    return -1;
}

/* 6601: u16 id */
s32 LobbyPkt_6601(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xAA);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6601: u8, whether the name may be sent */
void LobbyReply_6601(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8(&gLobbyNet.recs2.sendName);
    }
    Lobby_RouteMessage(0);
}

/* 6602 for third-level entry id */
s32 LobbyReq_6602(s32 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6602(id);
        return i;
    }
    return -1;
}

/* the reply to 6602: u8, whether the password may be sent */
void LobbyReply_6602(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8(&gLobbyNet.recs2.sendPass);
    }
    Lobby_RouteMessage(0);
}

/* 6602: u16 id */
s32 LobbyPkt_6602(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xAC);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6405 for third-level entry id */
s32 LobbyReq_6405(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6405(id);
        return i;
    }
    return -1;
}

/* the byte 6405 gave entry id */
s32 NetList2_Get6405(u16 id, u8 *out)
{
    *out = gLobbyNet.list2[id - 1].unk1F;
    return 0;
}

/* 6405: u16 id */
s32 LobbyPkt_6405(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x9E);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6405 and its notification: u16 id, u8; route 0x22 */
void LobbyReply_6405(void)
{
    u8 v;
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8(&id, &v);
        gLobbyNet.list2[id - 1].unk1F = v;
        gLobbyNet.list2[id - 1].flags |= 0x10;
        gLobbyNet.lastId = id;
    }
    Lobby_RouteMessage(0x22);
}

/* 640A: the users of third-level entry id */
s32 LobbyReq_640A(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    memset(gLobbyNet.users2, 0, sizeof(gLobbyNet.users2));
    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_640A(id);
        return i;
    }
    return -1;
}

/* user i of 640A's reply */
s32 NetList2_GetUser(u16 i, char *id, char *name, u8 *text)
{
    strcpy(id, gLobbyNet.users2[i].u.id);
    strcpy(name, gLobbyNet.users2[i].u.name);
    memcpy(text, gLobbyNet.users2[i].u.text, 0x100);
    return 0;
}

/* 640A: u16 id */
s32 LobbyPkt_640A(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xA1);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 640A */
void LobbyReply_640A(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_640A_Users();
    }
    Lobby_RouteMessage(0);
}

/* 640A's users: into users2 */
void LobbyReply_640A_Users(void)
{
    memset(gLobbyNet.users2, 0, sizeof(gLobbyNet.users2));
    LobbyMsg_ReadUsers(&gLobbyNet.nUsers2, gLobbyNet.users2);
}
