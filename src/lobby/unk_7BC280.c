/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

s32 func_006B5300(void);
extern s32 gLobbyStartPosIdx;
extern s8 gLobbyScreenMode;

s32 func_007BC280(void) {
    if (gLobbyScreenMode == 4) {
        if (gLobbyStartPosIdx == 5) {
            return 2;
        }
        goto block_7;
    }
    if (func_006B5300() == 1) {
        return 1;
    }
block_7:
    return 0;
}
