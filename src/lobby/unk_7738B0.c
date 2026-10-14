/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "game_menu.h"
#include "m2c_macros.h"

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

s32 GameMenu_YesNoInput(u16 *pad) {
    s32 btn;
    s32 btn2;
    u16 raw;

    raw = *pad;
    *pad = raw & 0xF39F;
    if (gSubMenuWork->yesNo == 0) {
        btn = raw & 0xFFFF;
        if (btn & 0x20) {
            return 1;
        }
        if (btn & 0x40) {
            gSubMenuWork->yesNo = 1;
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        if (btn & 0x400) {
            gSubMenuWork->yesNo = 1;
            Sound_PlaySeFlat(0, 0x17, 0);
        }
        goto block_11;
    }
    btn2 = raw & 0xFFFF;
    if (btn2 & 0x60) {
        return 1;
    }
    if (btn2 & 0x800) {
        gSubMenuWork->yesNo = 0;
        Sound_PlaySeFlat(0, 0x17, 0);
    }
block_11:
    return 0;
}
