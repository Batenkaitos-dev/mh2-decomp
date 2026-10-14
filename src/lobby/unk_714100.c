/*
 * NETWORK: the third level entry requests 640B, 650A, 650C, and 6509, 650B:
 * builders, senders, replies and getters (gLobbyNet.list2).
 */
#include "common.h"
#include "lobby_net.h"

u8 *LobbyMsg_ReadU16U32(void *a, void *b);
s32 LobbyPkt_650A(u16 id);
s32 LobbyPkt_650C(u16 id);
s32 LobbyPkt_6509(u32 v);
s32 LobbyPkt_650B(void *data, u16 n);

/* 640B (entry id): the entry values */
s32 LobbyPkt_640B(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xA3);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 640B and its notification: u16 id, u16 x5; route 0x21 */
void LobbyReply_640B(void)
{
    NetListEntry *e;
    u16 id;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU16(&id, gLobbyRecvData);
        e = &gLobbyNet.list2[id - 1];
        p = LobbyMsg_GetU16(&e->maxUsers, p);
        p = LobbyMsg_GetU16(&e->unk12, p);
        p = LobbyMsg_GetU16(&e->unk16, p);
        p = LobbyMsg_GetU16(&e->unk14, p);
        LobbyMsg_GetU16(&e->unk18, p);
        e->flags |= 0x20;
    }
    Lobby_RouteMessage(0x21);
}

/* 650A for entry id */
s32 LobbyReq_650A(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_650A(id);
        return i;
    }
    return -1;
}

/* the value 650A gave entry id */
s32 Net_Get650A(u16 id, s32 *out)
{
    *out = gLobbyNet.list2[id - 1].color;
    return 0;
}

/* 650A (entry id) */
s32 LobbyPkt_650A(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x98);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 650A and its notification: u16 id, u32; route 0x23 */
void LobbyReply_650A(void)
{
    s32 v;
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U32(&id, &v);
        gLobbyNet.list2[id - 1].id = id;
        gLobbyNet.list2[id - 1].color = v;
        gLobbyNet.lastId = id;
        gLobbyNet.list2[id - 1].flags |= 0x80;
    }
    Lobby_RouteMessage(0x23);
}

/* 650C for entry id */
s32 LobbyReq_650C(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_650C(id);
        return i;
    }
    return -1;
}

/* the 0x20 bytes 650C gave entry id */
s32 Net_Get650C(u16 id, u8 *dst)
{
    s32 i;

    for (i = 0; i < 0x20; i++) {
        dst[i] = gLobbyNet.list2[id - 1].postedQuest[i];
    }
    return 0;
}

/* 650C (entry id) */
s32 LobbyPkt_650C(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x9B);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 650C and its notification: u16 id, blob (0x20 bytes); route 0x31 */
void LobbyReply_650C(void)
{
    u16 id;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        p = LobbyMsg_GetU16(&id, gLobbyRecvData);
        LobbyMsg_GetBlob(gLobbyNet.list2[id - 1].postedQuest, 0x20, p);
        gLobbyNet.list2[id - 1].id = id;
        gLobbyNet.lastId = id;
        gLobbyNet.list2[id - 1].flags |= 0x100;
    }
    Lobby_RouteMessage(0x31);
}

/* 6509 (u32) */
s32 LobbyReq_6509(u32 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6509(v);
        return i;
    }
    return -1;
}

s32 LobbyPkt_6509(u32 v)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xCE);
    LobbyPkt_PutU32(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6509 */
void LobbyReply_6509(void)
{
    Lobby_RouteMessage(0);
}

/* 650B (a blob) */
s32 LobbyReq_650B(void *data, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_650B(data, n);
        return i;
    }
    return -1;
}

s32 LobbyPkt_650B(void *data, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0xD0);
    LobbyPkt_PutBlob(&gLobbyPkt, data, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}
