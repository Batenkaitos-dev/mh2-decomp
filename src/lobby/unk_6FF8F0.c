/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "lobby_state.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"

extern s16 D_008BEFC0;

void ElderRaid_CheckChanged(void) {
    if (D_008BEFC0 != gSave.unk3698) {
        if (gLobby->elderRaidState == 1) {
            gLobby->movieState = 2;
        }
        gLobby->elderRaidState = 0;
    }
}
