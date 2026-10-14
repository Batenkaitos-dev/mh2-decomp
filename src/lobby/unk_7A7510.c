/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"


s32 Pad_StepWrap(s32 cur, s32 n, u16 pad) {
    s32 btn;
    s32 ret;

    ret = cur;
    btn = pad & 0xFFFF;
    if (btn & 0x800) {
        if (ret == 0) {
            ret = n - 1;
        } else {
            ret -= 1;
        }
    } else if (btn & 0x400) {
        ret += 1;
        if (ret >= n) {
            ret = 0;
        }
    }
    return ret;
}
