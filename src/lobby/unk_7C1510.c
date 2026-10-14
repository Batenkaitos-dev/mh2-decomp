/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"
#include "lobby_net.h"


/* A net handler (Net_SetHandler(5, ...)): the dispatcher passes the message. */
void LobbyRoute_OnMsg05(NetMsg m, NetMsg *pm) {
    u8 phase;

    phase = gLobby->phase;
    if ((phase != 5) && (phase == 4)) {
        gLobby->unk3963 = 1;
    }
}
