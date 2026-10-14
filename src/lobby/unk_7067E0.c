/*
 * NETWORK: the reply to 630C; the sender of 6318 (command entry 0x141).
 */
#include "common.h"
#include "lobby_net.h"

s32 LobbyPkt_6318(u16 arg0, u8 *arg1);

/* the reply to 630C */
void LobbyReply_630C(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6318(u16 v, u8 *bytes, void (*onReply)(NetMsg, NetMsg *))
{
    s32 i;

    i = NetReq_Alloc(1, NULL, onReply);
    if (i != -1) {
        gLobbyNet.req[i].seq = LobbyPkt_6318(v, bytes);
        return i;
    }
    return -1;
}
