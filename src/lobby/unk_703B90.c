/*
 * NETWORK: the second level's entry requests 630D (a u32), 630B (a byte) and
 * 631B (four bytes): senders, replies and getters (gLobbyNet.list1).
 */
#include "common.h"
#include "lobby_net.h"

s32 LobbyPkt_631B(u16 id);
s32 LobbyPkt_630B(u16 id);
s32 LobbyPkt_630D(u16 id);
u8 *LobbyMsg_ReadU16U8(void *a, void *b);
u8 *LobbyMsg_ReadU16U32(void *a, void *b);

/* 630D: u16 id */
s32 LobbyPkt_630D(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x78) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 630D and its notification: u16 id, u32; route 0x19 */
void LobbyReply_630D(void)
{
    s32 v;
    u16 id;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U32(&id, &v);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.list1[id - 1].color = v;
        gLobbyNet.lastId = id;
        gLobbyNet.list1[id - 1].flags |= 0x80;
    }
    Lobby_RouteMessage(0x19);
}

/* 630B for entry id */
s32 LobbyReq_630B(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_630B(id);
        return i;
    }
    return -1;
}

/* 630B: u16 id */
s32 LobbyPkt_630B(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x73) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 630B and its notification: u16 id, u8; route 0x18 */
void LobbyReply_630B(void)
{
    u16 id;
    u8 v;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8(&id, &v);
        gLobbyNet.list1[id - 1].unk1F = v;
        gLobbyNet.list1[id - 1].flags |= 0x10;
        gLobbyNet.lastId = id;
    }
    Lobby_RouteMessage(0x18);
}

/* 631B for entry id */
s32 LobbyReq_631B(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_631B(id);
        return i;
    }
    return -1;
}

/* gLobbyNet.list1[-1].unk188: the original adds id * 0x18C to this address
 * (from gLobbyNet, the offset 0x8C88 would be added apart) */
extern u8 D_008530C8[];

s32 NetList1_Get631B(u16 id, u8 *out)
{
    u8 *p = D_008530C8 + id * sizeof(NetListEntry);

    out[0] = p[0];
    out[1] = p[1];
    out[2] = p[2];
    out[3] = p[3];
    return 0;
}

/* 631B: u16 id */
s32 LobbyPkt_631B(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x7B) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}
