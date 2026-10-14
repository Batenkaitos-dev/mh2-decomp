/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

struct LobbyPkt;
s32 LobbyPkt_Send(struct LobbyPkt *);
u16 LobbyPkt_BeginCmd(struct LobbyPkt *, u16 cmd);
void LobbyPkt_End(struct LobbyPkt *);
extern M2C_UNK gLobbyPkt;

s32 LobbyPkt_660C(void) {
    s32 pkt;

    pkt = LobbyPkt_BeginCmd((struct LobbyPkt *)&gLobbyPkt, 0xC6) & 0xFFFF;
    LobbyPkt_End((struct LobbyPkt *)&gLobbyPkt);
    LobbyPkt_Send((struct LobbyPkt *)&gLobbyPkt);
    return pkt;
}
