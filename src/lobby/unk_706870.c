/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_net.h"
#include "m2c_macros.h"


s32 LobbyPkt_6318(u16 v, u8 *bytes)
{
    s32 seq;

    seq = LobbyPkt_BeginCmd(&gLobbyPkt, 0x141);
    LobbyPkt_PutU16(&gLobbyPkt, v);
    LobbyPkt_PutU8(&gLobbyPkt, bytes[0]);
    LobbyPkt_PutU8(&gLobbyPkt, bytes[1]);
    LobbyPkt_PutU8(&gLobbyPkt, bytes[2]);
    LobbyPkt_PutU8(&gLobbyPkt, bytes[3]);
    LobbyPkt_End(&gLobbyPkt);
    LobbyPkt_Send(&gLobbyPkt);
    return seq;
}
