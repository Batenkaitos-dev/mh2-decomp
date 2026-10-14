/*
 * NETWORK: entering and leaving the levels (6305 enters a second-level entry,
 * 6408 leaves it, 6209 enters one entry of each level, 6208 leaves them all),
 * users entering or leaving (LobbyNotify_UserInOut) and the entry information of
 * 6205/6303/6403 (NetList_ReadValues).
 */
#include "common.h"
#include "lobby_net.h"

void NetList_Select(s32 level, u16 id);
void NetList_Leave(u8 level);
void NetList_Enter(u8 level);
s32 LobbyPkt_6209(u16 a, u16 b, u16 c);
s32 LobbyPkt_Leave(u16 kind);
void LobbyMsg_ReadU16Array(u16 *, u16, u16 *);
void LobbyMsg_ReadErrText(void);

/* the reply to 6305: the second-level entry was entered */
void LobbyReply_6305(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(1);
    }
    Lobby_RouteMessage(0);
}

/* 6408: leaves the second-level entry */
s32 LobbyReq_6408(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Leave(1);
        return i;
    }
    return -1;
}

/* the reply to 6408 */
void LobbyReply_6408(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Leave(1);
    }
    Lobby_RouteMessage(0);
}

/* a user entered (kind 0, 2, 4: blob id, blob name, blob text) or left
 * (kind 1, 3, 5: blob id); a notification is routed to 0x1A + kind ... */
void LobbyNotify_UserInOut(u8 kind)
{
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.userInOut, 0, 0x11C);
        p = LobbyMsg_GetBlob(gLobbyNet.userInOut.id, 8, gLobbyRecvData);
        switch (kind) {
        case 0:
        case 2:
        case 4:
            LobbyMsg_GetBlob(gLobbyNet.userInOut.text, 0x100, LobbyMsg_GetBlob(gLobbyNet.userInOut.name, 0x10, p));
            break;
        }
    }
    if (gLobbyNet.flag == 0x10) {
        switch (kind) {
        case 0:
            Lobby_RouteMessage(0x1A);
            break;
        case 1:
            Lobby_RouteMessage(0x1B);
            break;
        case 2:
            Lobby_RouteMessage(0x24);
            break;
        case 3:
            Lobby_RouteMessage(0x25);
            break;
        case 4:
            Lobby_RouteMessage(0x29);
            break;
        case 5:
            Lobby_RouteMessage(0x2A);
            break;
        }
    }
}

/* the information of 6205, 6303, 6403 (u16 id, u16 x4) into list[id - 1] */
/* the information of 6205, 6303, 6403 (u16 id, u16 x4) into list[id - 1] */
void NetList_ReadValues(NetListEntry *e)
{
    u16 v[4];
    u16 id;
    NetListEntry *p;

    if (gLobbyNet.replyErr == 0) {
        p = e;
        LobbyMsg_ReadU16Array(&id, 4, v);
        p += id - 1;
        p->id = id;
        p->users = v[0];
        p->questUsers = v[1];
        p->maxUsers = v[2];
        p->unk10 = v[3];
        p->flags |= 1;
        gLobbyNet.lastId = id;
    }
}

/* 6209: enters the entries a, b, c of the three levels */
s32 LobbyReq_6209(u16 a, u16 b, u16 c, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(0, a);
        NetList_Select(1, b);
        NetList_Select(2, c);
        gLobbyNet.req[i].seq = LobbyPkt_6209(a, b, c);
        return i;
    }
    return -1;
}

/* 6209: u16 x3 */
s32 LobbyPkt_6209(u16 a, u16 b, u16 c)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x39) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, a);
    LobbyPkt_PutU16(&gLobbyPkt, b);
    LobbyPkt_PutU16(&gLobbyPkt, c);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6209 */
void LobbyReply_6209(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(0);
        NetList_Enter(1);
        NetList_Enter(2);
        LobbyMsg_ReadErrText();
    }
    Lobby_RouteMessage(0);
}

/* the reply to 6208: left all three levels */
void LobbyReply_6208(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Leave(0);
        NetList_Leave(1);
        NetList_Leave(2);
    }
    Lobby_RouteMessage(0);
}

s32 LobbyNet_GetLastId(u16 *id)
{
    *id = gLobbyNet.lastId;
    return 0;
}

/* 6202, notification and reply; route 0xF */
void LobbyReply_6202(void)
{
    Lobby_RouteMessage(0xF);
}

/* the notification 640C: an error text; route 9 */
void LobbyNotify_640C(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadErrText();
    }
    Lobby_RouteMessage(9);
}
