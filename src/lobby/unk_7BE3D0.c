/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"

u8 FadeA_GetState(void);
extern LobbyMenu *gLobbyMenu;

void LobbyRoom_Close3(void) {
    gLobbyMenu->redraw = 1;
    if ((FadeA_GetState() & 0xFF) != 1) {
        gLobby->mode = (u8) (gLobby->mode + 1);
        gLobby->step = 0;
    }
}
