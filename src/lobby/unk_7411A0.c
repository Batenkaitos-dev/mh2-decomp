/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "browser.h"
#include "m2c_macros.h"

void LobbySe_Play(s32);

void Dialog_Close(s32 yes) {
    s32 temp_a0;

    temp_a0 = yes & 0xFF;
    switch (temp_a0) {                              /* irregular */
    case 0:
        LobbySe_Play(4);
        break;
    case 1:
        LobbySe_Play(0);
        break;
    }
    gBrowserSub->dlgMode = 0;
    gBrowserWork->dlgBoxKind = 1;
}
