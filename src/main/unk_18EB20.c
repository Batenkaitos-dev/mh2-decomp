/*
 * One function between the save-data helpers
 * and the sound manager (sound.c). No code calls it.
 */
#include "common.h"

/* Maps the byte at +0x19 of a record (0-12) to 0-9: pairs 0-1, 2-3, 4-5,
 * 6-7 give 0-3, then 8-12 give 4, 5, 6, 8, 9; anything else 9. */
s8 SaveRecord_KindIndex_Unk(u8 *record)
{
    switch ((s8)record[0x19]) {
    case 0:
    case 1:
        return 0;
    case 2:
    case 3:
        return 1;
    case 4:
    case 5:
        return 2;
    case 6:
    case 7:
        return 3;
    case 8:
        return 4;
    case 9:
        return 5;
    case 10:
        return 6;
    case 11:
        return 8;
    case 12:
        return 9;
    }
    return 9;
}
