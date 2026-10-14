/*
 * NETWORK: entering the third level (6406 with a password, 6407), leaving it
 * (6501), and task 7, which sends the settings of the third-level entry the
 * player is in (gLobbyNet.settings2): 6609 name, 660A password, 6610 text
 * (each only if the server allowed it: gLobbyNet.recs2.send*), 660B one byte
 * per record, 6415 four bytes, then 660C.
 */
#include "common.h"
#include "lobby_net.h"
s32 NetReq_CountFree(void);

void NetList_Select(s32 level, u16 id);
void NetList_Leave(u8 level);
void NetList_Enter(u8 level);
s32 LobbyPkt_Leave(u16 kind);
s32 LobbyPkt_6406(u16 id, char *pass, u8 v);
s32 LobbyPkt_6407(u16 id, u8 v);
void NetTask7_Run(s32 task);
void NetTask7_Steps(NetTask *t);
void NetTask7_Done(NetTask *t);
void NetTask7_Failed(NetTask *t);
void NetTask7_Step0(NetTask *t);
void NetTask7_Step1(NetTask *t);
void NetTask7_Step2(NetTask *t);
void NetTask7_Step3(NetTask *t);
void NetTask7_Step4(NetTask *t);
void NetTask7_Step5(NetTask *t);
void NetTask7_Step6(NetTask *t);
void NetTask7_Step7(NetTask *t);
void NetTask7_Step8(NetTask *t);
void NetTask7_Step9(NetTask *t);
void NetTask7_Step10(NetTask *t);
void NetTask7_Step11(NetTask *t);
void NetTask7_Step12(NetTask *t);
void NetTask7_OnReply(NetMsg m, NetMsg *pm);
s32 LobbyPkt_660B(u8 i, u8 v);
s32 LobbyReq_6609(char *name, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_660A(char *pass, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6610(char *text, u16 n, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_660C(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6415(u16 id, u8 *c, void (*onReply)(NetMsg, NetMsg *));

/* the reply to 6406: the third-level entry was entered */
void LobbyReply_6406(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(2);
    }
    Lobby_RouteMessage(0);
}

/* 6406: enters third-level entry id (with its password) */
s32 LobbyReq_6406(u16 id, char *pass, u8 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(2, id);
        gLobbyNet.req[i].seq = LobbyPkt_6406(id, pass, v);
        return i;
    }
    return -1;
}

/* 6406: u16 id, blob password, u8 */
s32 LobbyPkt_6406(u16 id, char *pass, u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x85);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    if (pass == NULL) {
        LobbyPkt_PutBlob(&gLobbyPkt, "", 0);
    } else {
        LobbyPkt_PutBlob(&gLobbyPkt, pass, strlen(pass));
    }
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6501: leaves the third-level entry */
s32 LobbyReq_6501(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Leave(2);
        return i;
    }
    return -1;
}

/* the reply to 6501 */
void LobbyReply_6501(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Leave(2);
    }
    Lobby_RouteMessage(0);
}

/* 6407: enters third-level entry id */
s32 LobbyReq_6407(u16 id, u8 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(2, id);
        gLobbyNet.req[i].seq = LobbyPkt_6407(id, v);
        return i;
    }
    return -1;
}

/* 6407: u16 id, u8 */
s32 LobbyPkt_6407(u16 id, u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xA8);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6407 */
void LobbyReply_6407(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(2);
    }
    Lobby_RouteMessage(0);
}

/* task 7: sends these settings for the third-level entry the player is in */
s32 NetTask7_Start(NetEntrySettings2 s, void (*onDone)(NetMsg, NetMsg *))
{
    NetTask *t = &gLobbyNet.task[7];

    if (t->active == 0) {
        memset(t, 0, sizeof(NetTask));
        gLobbyNet.settings2 = s;
        t->active = 1;
        t->onDone = onDone;
        t->func = NetTask7_Run;
        return 0;
    }
    return -1;
}

/* task 7: its dispatcher */
void NetTask7_Run(s32 task)
{
    NetTask *t = &gLobbyNet.task[7];

    if (t->active != 0) {
        switch (t->state) {
        case 0:
            NetTask7_Steps(t);
            break;
        case 10:
            NetTask7_Done(t);
            break;
        case 11:
            NetTask7_Failed(t);
            break;
        }
    }
}

/* task 7: its steps */
void NetTask7_Steps(NetTask *t)
{
    switch (t->step) {
    case 0:
        NetTask7_Step0(t);
        break;
    case 1:
        NetTask7_Step1(t);
        break;
    case 2:
        NetTask7_Step2(t);
        break;
    case 3:
        NetTask7_Step3(t);
        break;
    case 4:
        NetTask7_Step4(t);
        break;
    case 5:
        NetTask7_Step5(t);
        break;
    case 6:
        NetTask7_Step6(t);
        break;
    case 7:
        NetTask7_Step7(t);
        break;
    case 8:
        NetTask7_Step8(t);
        break;
    case 9:
        NetTask7_Step9(t);
        break;
    case 10:
        NetTask7_Step10(t);
        break;
    case 11:
        NetTask7_Step11(t);
        break;
    case 12:
        NetTask7_Step12(t);
        break;
    }
}

/* task 7 done */
void NetTask7_Done(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = 0;
    t->onDone(m, &m);
}

/* task 7 failed */
void NetTask7_Failed(NetTask *t)
{
    NetMsg m;

    t->active = 0;
    t->state = 0;
    m.kind = -1;
    t->onDone(m, &m);
}

void NetTask7_Step0(NetTask *t)
{
    t->step++;
    t->reply = 0;
}

/* 6609: the name, if there is one and the server allows it */
void NetTask7_Step1(NetTask *t)
{
    if (gLobbyNet.settings2.name[0] != 0 && gLobbyNet.recs2.sendName == 1) {
        if (NetReq_CountFree() > 0) {
            t->step++;
            LobbyReq_6609(gLobbyNet.settings2.name, NetTask7_OnReply);
        }
    } else {
        t->step = 3;
    }
}

void NetTask7_Step2(NetTask *t)
{
    if (t->reply == 1) {
        t->step++;
        t->reply = 0;
    } else if (t->reply == 2) {
        t->state = 0xB;
        t->step = 0;
    }
}

/* 660A: the password */
void NetTask7_Step3(NetTask *t)
{
    if (gLobbyNet.settings2.pass[0] != 0 && gLobbyNet.recs2.sendPass == 1) {
        if (NetReq_CountFree() > 0) {
            t->step++;
            LobbyReq_660A(gLobbyNet.settings2.pass, NetTask7_OnReply);
        }
    } else {
        t->step = 5;
    }
}

void NetTask7_Step4(NetTask *t)
{
    if (t->reply == 1) {
        t->step++;
        t->reply = 0;
    } else if (t->reply == 2) {
        t->state = 0xB;
        t->step = 0;
    }
}

/* 660B: the byte of each record the server allows */
void NetTask7_Step5(NetTask *t)
{
    s32 i;

    t->step++;
    for (i = 0; i < gLobbyNet.recs2.n; i++) {
        if (gLobbyNet.recs2.rec[i].unk42 == 1) {
            LobbyPkt_660B(i, gLobbyNet.settings2.b[i]);
        }
    }
}

void NetTask7_Step6(NetTask *t)
{
    t->step++;
}

/* 6610: the text */
void NetTask7_Step7(NetTask *t)
{
    if (gLobbyNet.settings2.text[0] != 0 && gLobbyNet.recs2.sendText == 1) {
        if (NetReq_CountFree() > 0) {
            t->step++;
            LobbyReq_6610(gLobbyNet.settings2.text, strlen(gLobbyNet.settings2.text), NetTask7_OnReply);
        }
    } else {
        t->step = 9;
    }
}

void NetTask7_Step8(NetTask *t)
{
    if (t->reply == 1) {
        t->step++;
        t->reply = 0;
    } else if (t->reply == 2) {
        t->state = 0xB;
        t->step = 0;
    }
}

/* 6415: the four bytes */
void NetTask7_Step9(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_6415(gLobbyNet.cur[2], gLobbyNet.settings2.c, NetTask7_OnReply);
    }
}

void NetTask7_Step10(NetTask *t)
{
    if (t->reply == 1) {
        t->step++;
        t->reply = 0;
    } else if (t->reply == 2) {
        t->state = 0xB;
        t->step = 0;
    }
}

/* 660C */
void NetTask7_Step11(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_660C(NetTask7_OnReply);
    }
}

void NetTask7_Step12(NetTask *t)
{
    if (t->reply == 1) {
        t->state = 10;
        t->step = 0;
    } else if (t->reply == 2) {
        t->state = 11;
        t->step = 0;
    }
}

/* the replies to task 7's requests */
void NetTask7_OnReply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[7];

    if (m.kind == 0) {
        t->reply = 1;
    } else {
        t->reply = 2;
    }
}

/* 660B: u8 record, u8 */
s32 LobbyPkt_660B(u8 i, u8 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xC4);
    LobbyPkt_PutU8(&gLobbyPkt, i);
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 660B */
void LobbyReply_660B(void)
{
}
