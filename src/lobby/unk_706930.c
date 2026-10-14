#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_net.h"

s32 LobbyPkt_6311(void);
void Lobby_RouteMessage(s32);

void LobbyReply_6318(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6311(void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_6311();
        return req;
    }
    return -1;
}
