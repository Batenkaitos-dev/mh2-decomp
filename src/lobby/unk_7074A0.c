/*
 * NETWORK: 6191, 6192 and 6193 (a user's text, data sent to a user and
 * received from one).
 */
#include "common.h"
#include "lobby_net.h"


s32 LobbyPkt_6192(void *src, u16 n, u16 x);

/* the notification 6191: blob id (8), blob text (0x100); route 0x2C */
void LobbyNotify_6191(void)
{
    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.x6191, 0, 0x11C);
        LobbyMsg_GetBlob(gLobbyNet.x6191.text, 0x100, LobbyMsg_GetBlob(gLobbyNet.x6191.id, 8, gLobbyRecvData));
    }
    Lobby_RouteMessage(0x2C);
}

s32 LobbyReq_6192(void *src, u16 n, u16 x, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6192(src, n, x);
        return i;
    }
    return -1;
}

/* the reply to 6192 */
void LobbyReply_6192(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyNet_Get6193(Net6193 *dst)
{
    *dst = gLobbyNet.x6193;
    return 0;
}

/* the notification 6193: blob id (8), u16, u16 n, n bytes; route 0x2E */
void LobbyNotify_6193(void)
{
    Net6193 *r;
    u8 *p;

    if (gLobbyNet.replyErr == 0) {
        memset(&gLobbyNet.x6193, 0, 0x122);
        r = &gLobbyNet.x6193;
        p = LobbyMsg_GetBlob(r->id, 8, gLobbyRecvData);
        p = LobbyMsg_GetU16(&r->x, p);
        p = LobbyMsg_GetU16(&r->n, p);
        LobbyMsg_GetBytes(r->data, p, r->n);
    }
    Lobby_RouteMessage(0x2E);
}

/* 6192: u16 x, blob */
s32 LobbyPkt_6192(void *src, u16 n, u16 x)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x20) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, x);
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}
