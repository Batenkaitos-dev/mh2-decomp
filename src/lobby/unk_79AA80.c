/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "player.h"

void Actor_SetStateNow(struct Player *, u8, u8, u16);
void Actor_SetIdle(void *, s32, s32, s32);
s32 Player_IsLocal(void *obj);

void TownPc_FishInput(Player *pl) {
    if (Player_IsLocal(pl) != 0) {
        if (pl->stateGroup != 0) {
            Actor_SetIdle(pl, 0, 4, 0);
            return;
        }
        if (pl->input.pressed & 0x20) {
            if (pl->fishTimer > 0) {
                Actor_SetStateNow(pl, 0, 0x63, 0);
                return;
            }
            if (pl->baited == 1) {
                Actor_SetStateNow(pl, 0, 0x65, 0);
                return;
            }
            Actor_SetStateNow(pl, 0, 0x64, 0);
        }
    }
}
