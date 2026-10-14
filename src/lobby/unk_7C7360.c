/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"

s32 Lobby_HandleLeaveRequest(void);
void Lobby_Disconnect(s32);
void LobbyTown_Enter(void);
void LobbyTown_Leave(void);
extern LobbyMenu *gLobbyMenu;

void LobbyTown_Run(void) {
    u8 mode;

    if (Lobby_HandleLeaveRequest() != 0) {
        if ((gLobby->mode == 0) && (gLobby->step == 3)) {
            gLobbyMenu->hideLayers = 0;
        }
        return;
    }
    mode = gLobby->mode;
    switch (mode) {                              /* irregular */
    case 0:
        LobbyTown_Enter();
        return;
    case 1:
        LobbyTown_Leave();
        return;
    default:
        Lobby_Disconnect(5);
        return;
    }
}
