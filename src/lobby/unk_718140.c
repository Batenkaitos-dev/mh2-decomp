/*
 * NETWORK: 660F's reply (may the third-level entry's text be sent), 6415
 * (the entry's four bytes) and the notification 6505.
 */
#include "common.h"
#include "lobby_net.h"

u8 *LobbyMsg_ReadU8(void *dst);
void LobbyMsg_ReadErrText(void);
s32 LobbyPkt_6415(u16 id, u8 *c);

/* the reply to 660F: u8, whether the text may be sent */
void LobbyReply_660F(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadU8(&gLobbyNet.recs2.sendText);
    }
    Lobby_RouteMessage(0);
}

/* 6415 for third-level entry id */
s32 LobbyReq_6415(u16 id, u8 *c, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6415(id, c);
        return i;
    }
    return -1;
}

/* 6415: u16 id, u8 x4 */
s32 LobbyPkt_6415(u16 id, u8 *c)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x135);
    LobbyPkt_PutU16(&gLobbyPkt, id);
    LobbyPkt_PutU8(&gLobbyPkt, c[0]);
    LobbyPkt_PutU8(&gLobbyPkt, c[1]);
    LobbyPkt_PutU8(&gLobbyPkt, c[2]);
    LobbyPkt_PutU8(&gLobbyPkt, c[3]);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}

/* the reply to 6415 */
void LobbyReply_6415(void)
{
    Lobby_RouteMessage(0);
}

/* the notification 6505: an error text; route 0xA */
void LobbyNotify_6505(void)
{
    if (gLobbyNet.replyErr == 0) {
        LobbyMsg_ReadErrText();
    }
    Lobby_RouteMessage(0xA);
}
