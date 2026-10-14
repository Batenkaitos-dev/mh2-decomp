#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_net.h"

s32 LobbyPkt_6504(s32 arg0); /* unit's own declaration: with the definition's s32 (u8 v) (lobby/unk_70A130.c), LobbyReq_6504 1 of 29 words differ */
void Lobby_RouteMessage(s32);

void LobbyReply_640E(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6504(s32 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_6504(v);
        return req;
    }
    return -1;
}
