/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_net.h"

struct NetMsg;

void Lobby_RouteMessage(s32);
s32 LobbyPkt_6911(void);

/* v: u8, as LobbyPkt_PutU8 takes it (LobbyReq_6504 declares s32: a u8 there changes its code) */
s32 LobbyPkt_6504(u8 v) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0xD7) & 0xFFFF;
    LobbyPkt_PutU8(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}

void LobbyReply_6504(void)
{
    Lobby_RouteMessage(0);
}

/* Starts the party task (task 9, no steps): clears gLobbyNet.party and sends 6911. */
s32 NetTask9_Start(void (*onDone)(NetMsg, NetMsg *)) {
    if (gLobbyNet.task[9].active == 0) {
        memset(&gLobbyNet.party, 0, sizeof(NetParty));
        LobbyPkt_6911();
        gLobbyNet.task[9].onDone = onDone;
        gLobbyNet.task[9].active = 1;
        gLobbyNet.task[9].state = 0;
        gLobbyNet.task[9].func = NULL;
        return 0;
    }
    return -1;
}
