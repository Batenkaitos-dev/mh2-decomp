#include "common.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "m2c_macros.h"
#include "lobby_net.h"

s32 LobbyPkt_6604(s32 arg0, s32 arg1); /* unit's own declaration: with the definition's s32 (u16 id, u8 i) (lobby/unk_716EC0.c), LobbyReq_6604 2 of 32 words differ */

s32 LobbyReq_6604(s32 id, s32 i, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_6604(id, i);
        return req;
    }
    return -1;
}
