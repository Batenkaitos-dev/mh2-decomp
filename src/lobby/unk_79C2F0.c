/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

extern struct Em *gDelayedSeNpc;
extern s32 gDelayedSeTimer;
extern s32 gDelayedSeId;

void Npc_PlaySeDelayed(struct Em *npc, void *p) {
    gDelayedSeNpc = npc;
    gDelayedSeId = (s32) p;
    gDelayedSeTimer = 0x14;
}
