/*
 * The level-1 entry's settings, one request each, with the packet it
 * sends and its reply: the name (6315), the password (6316), the text
 * (6317), the colour (630C), the four group-info bytes (6318), and the end
 * of the settings (6311). NetLevel1SettingsTask sends all but the
 * colour, which is sent on its own.
 * Network code: it stays byte for byte, as the whole build does.
 */
#include "common.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_net.h"

s32 LobbyPkt_Level1SetColor(s32 arg0);
s32 LobbyPkt_Level1SetText(s8 *src, u16 n);
s32 LobbyPkt_Level1SetPass(s8 *src, u16 n);
s32 LobbyPkt_Level1SetName(char *s, u16 n);
s32 LobbyPkt_Level1SetGroupInfo(u16 arg0, u8 *arg1);
s32 LobbyPkt_Level1SettingsEnd(void);
void Lobby_RouteMessage(s32);

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
