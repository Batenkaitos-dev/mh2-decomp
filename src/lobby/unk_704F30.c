/*
 * NETWORK: 630A (the users of an entry), the notifications 6410/6411 (a user
 * left, entered), 6890/6891 and 6310 (enter a second-level entry).
 */
#include "common.h"
#include "lobby_net.h"


void NetList_Select(s32 level, u16 id);
void NetList_Enter(u8 level);
void LobbyNotify_UserInOut(u8 kind);
void LobbyReply_630A_Users(void);
void LobbyMsg_ReadUsers(u8 *count, NetUserRec *out);
s32 LobbyPkt_6891(void);
s32 LobbyPkt_6310(u16 id);
u8 *LobbyMsg_ReadU32(void *dst);

/* the reply to 630A */
void LobbyReply_630A(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_630A_Users();
    }
    Lobby_RouteMessage(0);
}

void LobbyReply_630A_Users(void)
{
    memset(gLobbyNet.users, 0, 0xB40);
    LobbyMsg_ReadUsers(&gLobbyNet.nUsers, gLobbyNet.users);
}

/* 630A's reply: u16, u8 fields per user, u8 users (at most 10), then per user
 * blob id (8), blob name (0x10), blob text (0x100) and the fields after the
 * third skipped (u16 length, bytes) */
void LobbyMsg_ReadUsers(u8 *count, NetUserRec *out)
{
    u16 len;
    u8 fields;
    u8 n;
    u8 *p;
    s32 i;
    s32 j;

    p = LobbyMsg_GetU16(&len, gLobbyRecvData);
    p = LobbyMsg_GetU8(&fields, p);
    p = LobbyMsg_GetU8(&n, p);
    if (n > 10) {
        n = 10;
    }
    *count = n;
    for (i = 0; i < n; i++) {
        p = LobbyMsg_GetBlob(out->u.id, 8, p);
        p = LobbyMsg_GetBlob(out->u.name, 0x10, p);
        p = LobbyMsg_GetBlob(out->u.text, 0x100, p);
        out++;
        if (fields > 3) {
            for (j = 0; j < fields - 3; j++) {
                p = LobbyMsg_GetU16(&len, p) + len;
            }
        }
    }
}

/* the notification 6410: a user left */
void LobbyNotify_6410(void)
{
    LobbyNotify_UserInOut(1);
}

/* the notification 6411: a user entered */
void LobbyNotify_6411(void)
{
    LobbyNotify_UserInOut(0);
}

/* the reply to 6890 and its notification: u32; route 0x2B */
void LobbyReply_6890(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU32(&gLobbyNet.x6890);
    }
    Lobby_RouteMessage(0x2B);
}

/* 6891 */
s32 LobbyReq_6891(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6891();
        return i;
    }
    return -1;
}

s32 LobbyNet_Get6891(Net6891 *dst)
{
    *dst = gLobbyNet.x6891;
    return 0;
}

/* the reply to 6891: u16 x3 */
void LobbyReply_6891(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&gLobbyNet.x6891.v[2], LobbyMsg_GetU16(&gLobbyNet.x6891.v[1], LobbyMsg_GetU16(&gLobbyNet.x6891.v[0], gLobbyRecvData)));
    }
    Lobby_RouteMessage(0);
}

/* 6891 */
s32 LobbyPkt_6891(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x115) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6310: enters second-level entry id */
s32 LobbyReq_6310(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(1, id);
        gLobbyNet.req[i].seq = LobbyPkt_6310(id);
        return i;
    }
    return -1;
}

/* 6310: u16 id */
s32 LobbyPkt_6310(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x63) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6310 */
void LobbyReply_6310(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(1);
    }
    Lobby_RouteMessage(0);
}
