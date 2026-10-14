/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_net.h"

void NetList_ReadValues(NetListEntry *e);
void Lobby_RouteMessage(s32);

void LobbyReply_6303(void) {
    if (gLobbyNet.replyErr == 0) {
        NetList_ReadValues(gLobbyNet.list1);
    }
    Lobby_RouteMessage(0x15);
}
