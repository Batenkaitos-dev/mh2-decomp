/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_state.h"

void TownMsg_Send05(char *name, u8 a, u8 b);
void ActSpot_Take(u8);
s32 ActSpot_IsFree(u32 n);
s32 memcmp(void *, void *, s32);

void TownMsg_On04(void *name, u8 *data) {
    u8 n;

    if (memcmp(gLobby->myId, gLobby->hostId, 8) == 0) {
        n = *data;
        if (ActSpot_IsFree(n) == 0) {
            TownMsg_Send05(name, 0, n);
            return;
        }
        ActSpot_Take(n);
        TownMsg_Send05(name, 1, n);
    }
}
