/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

extern s32 gGameVSyncOn;

void Sys_EnableGameVSync(void) {
    gGameVSyncOn = 1;
}
