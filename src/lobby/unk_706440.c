/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_net.h"

s32 LobbyPkt_630C(s32 arg0);
s32 LobbyPkt_6317(s8 *src, u16 n);
s32 LobbyPkt_6316(s8 *src, u16 n);

s32 LobbyPkt_6315(char *src, u16 n) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0x6B) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

void LobbyReply_6315(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6316(s8 *src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_6316(src, n);
        return req;
    }
    return -1;
}

s32 LobbyPkt_6316(s8 *src, u16 n) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0x6D) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

void LobbyReply_6316(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6317(s8 *src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_6317(src, n);
        return req;
    }
    return -1;
}

s32 LobbyPkt_6317(s8 *src, u16 n) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0x71) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

void LobbyReply_6317(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_630C(s32 v, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_630C(v);
        return req;
    }
    return -1;
}

s32 LobbyPkt_630C(s32 v) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0x76) & 0xFFFF;
    LobbyPkt_PutU32(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}
