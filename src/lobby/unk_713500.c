/*
 * NETWORK: task 4's steps (the third level's list: 6401's count, then per
 * entry the requests its flags ask for; unk_7115D0.c starts it), 6401 and 6403.
 */
#include "common.h"
#include "lobby_net.h"

void NetList_ReadValues(NetListEntry *e);
s32 LobbyPkt_ListCount(u16 kind);
s32 LobbyPkt_EntryValues(u16 kind, u16 id);
void LobbyMsg_ReadU16(void *a0);
void NetTask4_Step1Reply(NetMsg m, NetMsg *pm);
void NetTask4_Step4Reply(NetMsg m, NetMsg *pm);
s32 LobbyReq_6401(void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6403(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6404(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6402(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_640D(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_640B(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_650A(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_650C(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6405(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6412(u16 id, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyReq_6418(u16 arg0, void (*onReply)(NetMsg, NetMsg *));
s32 NetReq_CountFree(void);
void NetReq_ClearAll(void);

/* task 4, step 1: 6401 (the count) */
void NetTask4_Step1(NetTask *t)
{
    if (NetReq_CountFree() > 0) {
        t->step++;
        LobbyReq_6401(NetTask4_Step1Reply);
    }
}

/* task 4, step 2: 6401's reply; tells onDone how many replies to expect (kind 2) */
void NetTask4_Step2(NetTask *t)
{
    NetMsg m;

    if (t->reply == 1) {
        t->step++;
        t->reply = 0;
        gLobbyNet.nReplies = gLobbyNet.count2 * t->reqCount;
        m.kind = 2;
        m.flag = 10;
        t->onDone(m, &m);
    } else if (t->reply == 2) {
        t->state = 2;
        t->step = 0;
    }
}

/* task 4, step 3 */
void NetTask4_Step3(NetTask *t)
{
    t->step++;
    t->reply = 0;
    if (gLobbyNet.count2 == 0) {
        t->state = 1;
        t->step = 0;
    }
}

/* task 4, step 4: the requests of one entry (bits: 1 6403, 2 6404, 4 6402,
 * 0x20 640B, 8 640D, 0x10 6405, 0x40 6409, 0x80 650A, 0x100 650C, 0x400 6418) */
void NetTask4_Step4(NetTask *t)
{
    u32 id;

    if (t->reply == 2) {
        t->state = 2;
        NetReq_ClearAll();
        return;
    }
    id = t->pos;
    if (NetReq_CountFree() >= t->reqCount) {
        if (t->reqFlags & 1) {
            LobbyReq_6403(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 2) {
            LobbyReq_6404(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 4) {
            LobbyReq_6402(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 0x20) {
            LobbyReq_640B(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 8) {
            LobbyReq_640D(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 0x10) {
            LobbyReq_6405(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 0x40) {
            LobbyReq_6412(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 0x80) {
            LobbyReq_650A(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 0x100) {
            LobbyReq_650C(id, NetTask4_Step4Reply);
        }
        if (t->reqFlags & 0x400) {
            LobbyReq_6418(id, NetTask4_Step4Reply);
        }
        t->pos++;
        if (id + 1 > gLobbyNet.count2 || id + 1 > 5) {
            t->step++;
        }
    }
}

/* task 4, step 5: done once every entry has all the information asked for */
void NetTask4_Step5(NetTask *t)
{
    s32 ok = 1;
    u32 j;
    u32 i = 0;

    for (j = 0; j < 5; j++) {
        i++;
        if (i > gLobbyNet.count2) {
            break;
        }
        if ((t->reqFlags & gLobbyNet.list2[i - 1].flags) != t->reqFlags) {
            ok = 0;
            break;
        }
    }
    if (t->reply == 2) {
        t->state = 2;
        t->step = 0;
        NetReq_ClearAll();
    } else if (ok) {
        t->state = 1;
        t->step = 0;
    }
}

/* task 4's reply callback for 6401 */
void NetTask4_Step1Reply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[4];

    if (m.kind == 0) {
        t->reply = 1;
    } else {
        t->reply = 2;
    }
}

/* task 4's reply callback for an entry's requests: each reply is reported to onDone (kind 2) */
void NetTask4_Step4Reply(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[4];
    NetMsg n;

    if (m.kind == 0) {
        n.kind = 2;
        n.flag = 11;
        t->onDone(n, &n);
        t->reply = 1;
    } else {
        t->reply = 2;
    }
}

/* 6401 (the count) */
s32 LobbyReq_6401(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ListCount(2);
        return i;
    }
    return -1;
}

s32 NetList2_GetCount(u16 *out)
{
    *out = gLobbyNet.count2;
    return 0;
}

/* the reply to 6401: u16 count */
void LobbyReply_6401(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16(&gLobbyNet.count2);
    }
    Lobby_RouteMessage(0);
}

/* 6403 for entry id */
s32 LobbyReq_6403(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryValues(2, id);
        return i;
    }
    return -1;
}

s32 NetList2_GetUsers(u16 id, u16 *a, u16 *b)
{
    *a = gLobbyNet.list2[id - 1].users;
    *b = gLobbyNet.list2[id - 1].questUsers;
    return 0;
}

/* the reply to 6403 and its notification; route 0x1E */
void LobbyReply_6403(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_ReadValues(gLobbyNet.list2);
    }
    Lobby_RouteMessage(0x1E);
}
