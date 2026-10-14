/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
#include "box_menu.h"

s32 Box_YesNoInput(u16 *pad) {
    s32 btn;
    s32 btn2;
    u16 raw;

    raw = *pad;
    *pad = raw & 0xF39F;
    if (gBoxWork->yesNo == 0) {
        btn = raw & 0xFFFF;
        if (btn & 0x20) {
            gBoxWork->yesNo = 0xFFU;
            return 1;
        }
        if (btn & 0x40) {
            gBoxWork->yesNo = 1U;
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        if (btn & 0x400) {
            gBoxWork->yesNo = 1U;
            Sound_PlaySeFlat(0, 0x17, 0);
        }
        goto block_11;
    }
    btn2 = raw & 0xFFFF;
    if (btn2 & 0x60) {
        gBoxWork->yesNo = 0xFFU;
        return -1;
    }
    if (btn2 & 0x800) {
        gBoxWork->yesNo = 0U;
        Sound_PlaySeFlat(0, 0x17, 0);
    }
block_11:
    return 0;
}
