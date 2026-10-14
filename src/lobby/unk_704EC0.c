/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

struct LobbyPkt;
s32 LobbyPkt_Send(struct LobbyPkt *);
u16 LobbyPkt_BeginCmd(struct LobbyPkt *, u16 cmd);
void LobbyPkt_End(struct LobbyPkt *);
void LobbyPkt_PutU16(struct LobbyPkt *, s32); /* LobbyPkt_PutU16(LobbyPkt *, u16): s32 keeps this file's code */ /* unit's own declaration: with the definition's void (LobbyPkt *p, u16 v) (lobby/lobby_net.c), LobbyPkt_630A 1 of 26 words differ */
extern struct LobbyPkt gLobbyPkt;

s32 LobbyPkt_630A(s32 v) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd(&gLobbyPkt, 0x7E) & 0xFFFF;
    LobbyPkt_PutU16(&gLobbyPkt, v);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return pkt;
}
