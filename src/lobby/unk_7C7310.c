/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"


s32 LobbyTown_IsLeaving(void) {
    if ((gLobby->phase == 3) && (gLobby->mode == 1) && (gLobby->phaseStep == 0)) {
        return 1;
    }
    return 0;
}
