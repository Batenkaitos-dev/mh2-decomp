/*
 * Entering and leaving the levels (6305 enters a level-1 entry, 6408
 * leaves it, 6209 enters one entry of each level, 6208 leaves them all),
 * users entering or leaving (LobbyNotify_UserInOut) and the entry information of
 * 6205/6303/6403 (NetList_ReadValues). Network code: it stays byte for
 * byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"

void NetList_Select(s32 level, u16 id);
void NetList_Leave(u8 level);
void NetList_Enter(u8 level);
s32 LobbyPkt_EnterLevels(u16 a, u16 b, u16 c);
s32 LobbyPkt_Leave(u16 kind);
void LobbyMsg_ReadU16Array(u16 *, u16, u16 *);
void LobbyMsg_ReadErrText(void);

/* the reply to 6305: the second-level entry was entered */
void LobbyReply_Level1Enter(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(1);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* 6408: leaves the second-level entry */
s32 LobbyReq_Level1Leave(void (*onReply)(NetMsg, NetMsg *))
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
void LobbyReply_Level1Leave(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Leave(1);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* a user entered (kind 0, 2, 4: blob id, blob name, blob text) or left
 * (kind 1, 3, 5: blob id); a notification is routed to 0x1A + kind ... */
void LobbyNotify_UserInOut(u8 kind)
{
    u8 *cur;

    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.userInOut, 0, 0x11C);
        cur = LobbyMsg_GetBlob(gLobbyNet.userInOut.id, 8, gLobbyRecvData);
        switch (kind) {
        case 0:
        case 2:
        case 4:
            LobbyMsg_GetBlob(gLobbyNet.userInOut.text, 0x100, LobbyMsg_GetBlob(gLobbyNet.userInOut.name, 0x10, cur));
            break;
        }
    }
    if (gLobbyNet.flag == LOBBY_FLAG_NOTIFY) {
        switch (kind) {
        case 0:
            Lobby_RouteMessage(NET_ROUTE_LEVEL1_USER_IN);
            break;
        case 1:
            Lobby_RouteMessage(NET_ROUTE_LEVEL1_USER_OUT);
            break;
        case 2:
            Lobby_RouteMessage(NET_ROUTE_LEVEL2_USER_IN);
            break;
        case 3:
            Lobby_RouteMessage(NET_ROUTE_LEVEL2_USER_OUT);
            break;
        case 4:
            Lobby_RouteMessage(NET_ROUTE_USER_IN_KIND4);
            break;
        case 5:
            Lobby_RouteMessage(NET_ROUTE_USER_OUT_KIND5);
            break;
        }
    }
}

/* the information of 6205, 6303, 6403 (u16 id, u16 x4) into list[id - 1] */
void NetList_ReadValues(NetListEntry *list)
{
    u16 value[4];
    u16 id;
    NetListEntry *entry;

    if (gLobbyNet.replyErr == 0) {
        entry = list;
        LobbyMsg_ReadU16Array(&id, 4, value);
        entry += id - 1;
        entry->id = id;
        entry->users = value[0];
        entry->questUsers = value[1];
        entry->maxUsers = value[2];
        entry->unk10 = value[3];
        entry->flags |= NET_ENTRY_VALUES;
        gLobbyNet.lastId = id;
    }
}

/* 6209: enters one entry of each level (0 for none); selects them first */
s32 LobbyReq_EnterLevels(u16 level0Id, u16 level1Id, u16 level2Id, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        NetList_Select(0, level0Id);
        NetList_Select(1, level1Id);
        NetList_Select(2, level2Id);
        gLobbyNet.req[i].seq = LobbyPkt_EnterLevels(level0Id, level1Id, level2Id);
        return i;
    }
    return -1;
}

/* 6209: u16 x3 */
s32 LobbyPkt_EnterLevels(u16 level0Id, u16 level1Id, u16 level2Id)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_ENTER_LEVELS) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, level0Id);
    LobbyPkt_PutU16(&gLobbyPkt, level1Id);
    LobbyPkt_PutU16(&gLobbyPkt, level2Id);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6209 */
void LobbyReply_EnterLevels(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Enter(0);
        NetList_Enter(1);
        NetList_Enter(2);
        LobbyMsg_ReadErrText();
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the reply to 6208: left all three levels */
void LobbyReply_LeaveAll(void)
{
    if (gLobbyNet.replyErr == 0) {
        NetList_Leave(0);
        NetList_Leave(1);
        NetList_Leave(2);
    }
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

s32 LobbyNet_GetLastId(u16 *id)
{
    *id = gLobbyNet.lastId;
    return 0;
}

/* 6202, notification and reply; route 0xF */
void LobbyReply_6202(void)
{
    Lobby_RouteMessage(NET_ROUTE_6202);
}

/* the notification 640C: an error text; route 9 */
void LobbyNotify_Level1Kick(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadErrText();
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_KICK);
}
