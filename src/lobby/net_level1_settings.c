/*
 * The level-1 entry's settings, one request each, with the packet it
 * sends and its reply: the name (6315), the password (6316), the text
 * (6317), the colour (630C), the four group-info bytes (6318), and the end
 * of the settings (6311). NetLevel1SettingsTask sends all but the
 * colour, which is sent on its own. Then two more of the town's
 * requests: its clock (6892), which the login's steps fetch, and an
 * entry's kind (630E).
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_net.h"

/* 6315: the name */
s32 LobbyReq_Level1SetName(char *name, u16 len, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1SetName(name, len);
        return i;
    }
    return -1;
}

/* Sends 6315; returns its sequence number. */
s32 LobbyPkt_Level1SetName(char *src, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SET_NAME) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 6315: completes its request. */
void LobbyReply_Level1SetName(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6316 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1SetPass(s8 *src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level1SetPass(src, n);
        return req;
    }
    return -1;
}

/* Sends 6316; returns its sequence number. */
s32 LobbyPkt_Level1SetPass(s8 *src, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SET_PASS) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 6316: completes its request. */
void LobbyReply_Level1SetPass(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6317 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1SetText(s8 *src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level1SetText(src, n);
        return req;
    }
    return -1;
}

/* Sends 6317; returns its sequence number. */
s32 LobbyPkt_Level1SetText(s8 *src, u16 n)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SET_TEXT) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 6317: completes its request. */
void LobbyReply_Level1SetText(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 630C as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1SetColor(s32 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level1SetColor(value);
        return req;
    }
    return -1;
}

/* Sends 630C; returns its sequence number. */
s32 LobbyPkt_Level1SetColor(s32 value)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SET_COLOR) & 0xFFFF;
    LobbyPkt_PutU32(&gLobbyPkt, value);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 630C */
void LobbyReply_Level1SetColor(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6318 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1SetGroupInfo(u16 value, u8 *bytes, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_Level1SetGroupInfo(value, bytes);
        return i;
    }
    return -1;
}

/* Sends 6318; returns its sequence number. */
s32 LobbyPkt_Level1SetGroupInfo(u16 value, u8 *bytes)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SET_GROUP_INFO);
    LobbyPkt_PutU16(&gLobbyPkt, value);
    LobbyPkt_PutU8(&gLobbyPkt, bytes[0]);
    LobbyPkt_PutU8(&gLobbyPkt, bytes[1]);
    LobbyPkt_PutU8(&gLobbyPkt, bytes[2]);
    LobbyPkt_PutU8(&gLobbyPkt, bytes[3]);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* The reply to 6318: completes its request. */
void LobbyReply_Level1SetGroupInfo(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6311 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1SettingsEnd(void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_Level1SettingsEnd();
        return req;
    }
    return -1;
}

/* 6311 */
s32 LobbyPkt_Level1SettingsEnd(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_LEVEL1_SETTINGS_END) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6311 */
void LobbyReply_Level1SettingsEnd(void)
{
    Lobby_RouteMessage(NET_ROUTE_NONE);
}

/*
 * Sends 6892 as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_TownClock(void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_TownClock();
        return i;
    }
    return -1;
}

s32 LobbyNet_GetTownClock(NetTownClock *dst)
{
    *dst = gLobbyNet.townClock;
    return 0;
}

/* the reply to 6892 and its notification */
void LobbyReply_TownClock(void)
{
    u8 *cur;
    s32 i;
    s32 n;

    if (gLobbyNet.replyErr == 0) {
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.time, gLobbyRecvData);
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.dayTicks, cur);
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.day, cur);
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.season, cur);
        cur = LobbyMsg_GetU32(&gLobbyNet.townClock.n, cur);
        n = gLobbyNet.townClock.n;
        for (i = 0; i < n; i++) {
            cur = LobbyMsg_GetU32(&gLobbyNet.townClock.list[i], cur);
        }
        LobbyMsg_GetU32(&gLobbyNet.townClock.year, cur);
    }
    Lobby_RouteMessage(NET_ROUTE_TOWN_CLOCK);
}

/* 6891 */
s32 LobbyPkt_TownClock(void)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, LOBBY_CMD_TOWN_CLOCK) & 0xFFFF;
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/*
 * Sends 630E as a request; onReply gets its reply. Returns the request's
 * slot, -1 if none is free.
 */
s32 LobbyReq_Level1Kind(u16 value, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_EntryKind(1, value);
        return i;
    }
    return -1;
}

/* the reply to 630E and its notification: a room's id and a byte */
void LobbyReply_Level1Kind(void)
{
    u16 id;
    u8 value;

    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU16U8b2(&id, &value);
        gLobbyNet.list1[id - 1].id = id;
        gLobbyNet.list1[id - 1].kind = value;
        gLobbyNet.list1[id - 1].flags |= NET_ENTRY_KIND;
    }
    Lobby_RouteMessage(NET_ROUTE_LEVEL1_KIND);
}
