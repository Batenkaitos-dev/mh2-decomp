/*
 * The lobby protocol's commands for chat (67xx): a say (6714), chat
 * messages (6702, 670C), town messages (6708, 670E) and the two sends to
 * one user (670B, 670D); then the replies to 6181 and 6188, which only
 * complete their requests.
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "lobby_net.h"



void LobbyNet_SendChatSay(s32 text, u16 len, u8 option, u8 kind)
{
    LobbyPkt_ChatSay(text, len, option, kind);
}

void LobbyNet_GetChatMessage(NetChatMessage *dst)
{
    *dst = gLobbyNet.chatMessage;
}

/* the notification 6714 (sent): blob, u8, u8 */
void LobbyPkt_ChatSay(s32 text, u16 len, u8 option, u8 kind)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CHAT_SAY);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)text, len);
    LobbyPkt_PutU8(&gLobbyPkt, option);
    LobbyPkt_PutU8(&gLobbyPkt, kind);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the notification 6702: a user (blob id 8, blob name 16, blob text 256,
 * u8 x4, u8) into gLobbyNet.chatMessage; route NET_ROUTE_CHAT_MESSAGE */
void LobbyNotify_ChatMessage(void)
{
    memset(&gLobbyNet.chatMessage, 0, 292);
    LobbyMsg_GetU8(&gLobbyNet.chatMessage.kind, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 3, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 2, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 1, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color, LobbyMsg_GetBlob(gLobbyNet.chatMessage.r.u.text, 256, LobbyMsg_GetBlob(gLobbyNet.chatMessage.r.u.name, 16, LobbyMsg_GetBlob(&gLobbyNet.chatMessage, 8, gLobbyRecvData))))))));
    Lobby_RouteMessage(NET_ROUTE_CHAT_MESSAGE);
}

void LobbyNet_SendTownMsg(s32 src, u16 n)
{
    LobbyPkt_TownMsg(src, n);
}

void LobbyNet_GetTownMsg(NetTownMsg *dst)
{
    *dst = gLobbyNet.townMsg;
}

/* the notification 6708 (sent): a chat message (blob) */
void LobbyPkt_TownMsg(s32 src, u16 n)
{
    LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_TOWN_MSG);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
}

/* the notification 6708 (received): blob id (8), blob message (768); route NET_ROUTE_TOWN_MSG */
void LobbyNotify_TownMsg(void)
{
    LobbyMsg_GetBlob(gLobbyNet.townMsg.text, 768, LobbyMsg_GetBlob(&gLobbyNet.townMsg, 8, gLobbyRecvData));
    Lobby_RouteMessage(NET_ROUTE_TOWN_MSG);
}

/*
 * Sends 670B as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_ChatTell(u8 value, void *src, s32 text, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_ChatTell(value, src, text, n);
        return i;
    }
    return -1;
}

/* 670B: blob id (6), blob text, u8 */
s32 LobbyPkt_ChatTell(u8 value, void *src, s32 text, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_CHAT_TELL) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)text, n);
    LobbyPkt_PutU8(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 670B */
void LobbyReply_ChatTell(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the notification 670C: the same user record as 6702's; route NET_ROUTE_CHAT_TELL */
void LobbyNotify_ChatTellMessage(void)
{
    memset(&gLobbyNet.chatMessage, 0, 292);
    LobbyMsg_GetU8(&gLobbyNet.chatMessage.kind, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 3, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 2, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color + 1, LobbyMsg_GetU8(gLobbyNet.chatMessage.r.color, LobbyMsg_GetBlob(gLobbyNet.chatMessage.r.u.text, 256, LobbyMsg_GetBlob(gLobbyNet.chatMessage.r.u.name, 16, LobbyMsg_GetBlob(&gLobbyNet.chatMessage, 8, gLobbyRecvData))))))));
    Lobby_RouteMessage(NET_ROUTE_CHAT_TELL);
}

/*
 * Sends 670D as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_TownMsgTo(void *src, void *data, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_TownMsgTo(src, (s32) data, n);
        return i;
    }
    return -1;
}

/* 670D: blob id (6), blob text */
s32 LobbyPkt_TownMsgTo(void *src, s32 data, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_TOWN_MSG_TO) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, 6);
    LobbyPkt_PutBlob(&gLobbyPkt, (void *)data, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 670D */
void LobbyReply_TownMsgTo(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the notification 670E: blob id (8), blob message (768); route NET_ROUTE_TOWN_MSG */
void LobbyNotify_TownMsgToRecv(void)
{
    LobbyMsg_GetBlob(gLobbyNet.townMsg.text, 768, LobbyMsg_GetBlob(&gLobbyNet.townMsg, 8, gLobbyRecvData));
    Lobby_RouteMessage(NET_ROUTE_TOWN_MSG);
}

/* the reply to 6181 */
void LobbyReply_6181(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/* the reply to 6188 */
void LobbyReply_6188(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}
