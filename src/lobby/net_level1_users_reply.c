/*
 * 630A (the users of an entry), the notifications 6410/6411 (a user left,
 * entered), 6890/6891 and 6310 (entering a level-1 entry to recruit).
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"


void NetList_Select(s32 level, u16 id);
void NetList_Enter(u8 level);
void LobbyNotify_UserInOut(u8 kind);
void LobbyReply_Level1UsersRead(void);
void LobbyMsg_ReadUsers(u8 *count, NetUserRec *out);
s32 LobbyPkt_CurrentPlace(void);
s32 LobbyPkt_Level1Open(u16 id);
u8 *LobbyMsg_ReadU32(void *dst);

/* the reply to 630A */
void LobbyReply_Level1Users(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyReply_Level1UsersRead();
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* Reads 630A's reply: the users into gLobbyNet.users. */
void LobbyReply_Level1UsersRead(void)
{
    memset(gLobbyNet.users, 0, sizeof(gLobbyNet.users));
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
    u8 *cur;
    s32 i;
    s32 j;

    cur = LobbyMsg_GetU16(&len, gLobbyRecvData);
    cur = LobbyMsg_GetU8(&fields, cur);
    cur = LobbyMsg_GetU8(&n, cur);
    if (n > 10) {
        n = 10;
    }
    *count = n;
    for (i = 0; i < n; i++) {
        cur = LobbyMsg_GetBlob(out->u.id, 8, cur);
        cur = LobbyMsg_GetBlob(out->u.name, 0x10, cur);
        cur = LobbyMsg_GetBlob(out->u.text, 0x100, cur);
        out++;
        if (fields > 3) {
            for (j = 0; j < fields - 3; j++) {
                cur = LobbyMsg_GetU16(&len, cur) + len;
            }
        }
    }
}

/* the notification 6410: a user left */
void LobbyNotify_Level1UserOut(void)
{
    LobbyNotify_UserInOut(1);
}

/* the notification 6411: a user entered */
void LobbyNotify_Level1UserIn(void)
{
    LobbyNotify_UserInOut(0);
}

/* the reply to 6890 and its notification: u32; route 0x2B */
void LobbyReply_6890(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU32(&gLobbyNet.x6890);
    }
    Lobby_RouteMessage(NET_ROUTE_6890);
}

/* 6891 */
s32 LobbyReq_CurrentPlace(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_CurrentPlace();
        return i;
    }
    return -1;
}

s32 LobbyNet_GetCurrentPlace(NetCurrentPlace *dst)
{
    *dst = gLobbyNet.currentPlace;
    return 0;
}

/* the reply to 6891: u16 x3 */
void LobbyReply_CurrentPlace(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_GetU16(&gLobbyNet.currentPlace.v[2], LobbyMsg_GetU16(&gLobbyNet.currentPlace.v[1], LobbyMsg_GetU16(&gLobbyNet.currentPlace.v[0], gLobbyRecvData)));
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6891 */
s32 LobbyPkt_CurrentPlace(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CURRENT_PLACE) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* 6310: enters second-level entry id */
s32 LobbyReq_Level1Open(u16 id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(1, id);
        gLobbyNet.req[i].seq = LobbyPkt_Level1Open(id);
        return i;
    }
    return -1;
}

/* 6310: u16 id */
s32 LobbyPkt_Level1Open(u16 id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_OPEN) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6310 */
void LobbyReply_Level1Open(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(1);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}
