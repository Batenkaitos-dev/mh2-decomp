/*
 * NETWORK: task 5 (gLobbyNet.task[5], started by NetTask5_Start), steps 0-2:
 * for each entry 1..gLobbyNet.count1 (the count 6301's reply gave), the
 * requests 6312, 6313 and 6314 (command entries 0x65, 0x67, 0x69, one u16:
 * the entry), three at a time; their replies (u8 value, u16 entry) fill
 * gLobbyNet.rec[entry - 1] and mark gLobbyNet.flags[0..2][entry - 1].
 */
#include "common.h"
#include "lobby_net.h"
s32 NetReq_CountFree(void);


s32 LobbyReq_6312(u16 entry, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6312(u16 entry);
s32 LobbyReq_6313(u16 entry, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6313(u16 entry);
s32 LobbyReq_6314(u16 entry, void (*onReply)(NetMsg, NetMsg *));
s32 LobbyPkt_6314(u16 entry);
void NetTask5_Step1Reply1(NetMsg m, NetMsg *pm);
void NetTask5_Step1Reply2(NetMsg m, NetMsg *pm);
void NetTask5_Step1Reply4(NetMsg m, NetMsg *pm);

void NetTask5_Step0(NetTask *t)
{
    t->step++;
    gLobbyNet.nReplies = 0;
    memset(gLobbyNet.rec, 0, 0xB4);
    memset(gLobbyNet.flags, 0, 0x87);
    t->idx = 1;
    t->pos = 1;
    t->reply = 0;
}

void NetTask5_Step1(NetTask *t)
{
    u32 entry = t->pos;

    if (NetReq_CountFree() >= 3) {
        LobbyReq_6312(entry, NetTask5_Step1Reply1);
        LobbyReq_6313(entry, NetTask5_Step1Reply2);
        LobbyReq_6314(entry, NetTask5_Step1Reply4);
        t->pos++;
        if (entry + 1 > gLobbyNet.count1 || entry + 1 > 0x2D) {
            t->step++;
        }
    }
}

void NetTask5_Step2(NetTask *t)
{
    s32 ok = 1;
    u32 j;
    u32 i = 0;

    for (j = 0; j < 0x2D; j++) {
        i++;
        if (i > gLobbyNet.count1) {
            break;
        }
        if (gLobbyNet.flags[0][i - 1] == 0) {
            ok = 0;
            break;
        }
        if (gLobbyNet.flags[1][i - 1] == 0) {
            ok = 0;
            break;
        }
        if (gLobbyNet.flags[2][i - 1] == 0) {
            ok = 0;
            break;
        }
    }
    if (t->reply & 0x80) {
        t->state = 2;
        t->step = 0;
        t->reply = 0;
    } else if (ok) {
        t->state++;
        t->step = 0;
    }
}

void NetTask5_Step1Reply1(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[5];

    if (m.kind == 0) {
        t->reply |= 1;
    } else {
        t->reply |= 0x80;
    }
}

void NetTask5_Step1Reply2(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[5];

    if (m.kind == 0) {
        t->reply |= 2;
    } else {
        t->reply |= 0x80;
    }
}

void NetTask5_Step1Reply4(NetMsg m, NetMsg *pm)
{
    NetTask *t = &gLobbyNet.task[5];

    if (m.kind == 0) {
        t->reply |= 4;
    } else {
        t->reply |= 0x80;
    }
}

/* entry's three values (clamped to 1..0x2D) */
/* the values of entry (clamped to 1..0x2D) */
s32 LobbyNet_GetEntryVals(u16 entry, NetEntryVals *out)
{
    if (entry == 0) {
        entry = 1;
    } else if (entry > 0x2D) {
        entry = 0x2D;
    }
    *out = gLobbyNet.rec[entry - 1];
    return 0;
}

s32 LobbyReq_6312(u16 entry, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6312(entry);
        return i;
    }
    return -1;
}

/* the reply to 6312: u8 value, u16 entry */
void LobbyReply_6312(void)
{
    u16 entry;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&entry, LobbyMsg_GetU8(&v, gLobbyRecvData));
        if ((s32)entry < 0x2E) {
            gLobbyNet.rec[entry - 1].v[0] = v;
            gLobbyNet.flags[0][entry - 1] = 1;
        }
    }
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_6312(u16 entry)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x65) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, entry);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyReq_6313(u16 entry, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6313(entry);
        return i;
    }
    return -1;
}

/* the reply to 6313 */
void LobbyReply_6313(void)
{
    u16 entry;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&entry, LobbyMsg_GetU8(&v, gLobbyRecvData));
        if ((s32)entry < 0x2E) {
            gLobbyNet.rec[entry - 1].v[1] = v;
            gLobbyNet.flags[1][entry - 1] = 1;
        }
    }
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_6313(u16 entry)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x67) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, entry);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

s32 LobbyReq_6314(u16 entry, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6314(entry);
        return i;
    }
    return -1;
}

/* the reply to 6314 */
void LobbyReply_6314(void)
{
    u16 entry;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&entry, LobbyMsg_GetU8(&v, gLobbyRecvData));
        if ((s32)entry < 0x2E) {
            gLobbyNet.rec[entry - 1].v[2] = v;
            gLobbyNet.flags[2][entry - 1] = 1;
        }
    }
    Lobby_RouteMessage(0);
}

s32 LobbyPkt_6314(u16 entry)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x69) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, entry);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}
