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

s32 LobbyPkt_6610(char *src, u16 n);
s32 LobbyPkt_660A(char *s);

s32 LobbyPkt_6609(char *src) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0xBE) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, strlen(src) & 0xFFFF);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

void LobbyReply_6609(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_660A(char *src, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_660A(src);
        return req;
    }
    return -1;
}

s32 LobbyPkt_660A(char *src) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0xC0) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, strlen(src) & 0xFFFF);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

void LobbyReply_660A(void)
{
    Lobby_RouteMessage(0);
}

s32 LobbyReq_6610(char *src, u16 n, void (*onReply)(NetMsg, NetMsg *))
{
    s32 req;

    req = NetReq_Alloc(1, 0, onReply);
    if (req != -1) {
        gLobbyNet.req[req].seq = LobbyPkt_6610(src, n);
        return req;
    }
    return -1;
}

s32 LobbyPkt_6610(char *src, u16 n) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0xC2) & 0xFFFF;
    LobbyPkt_PutBlob(&gLobbyPkt, src, n);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}
