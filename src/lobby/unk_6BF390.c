/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"
#include "save.h"

void Font_SetSpacing(u8 a, u8 b);
s32 Game_IsOnline(void);
/* unit's own declaration: with the definition's s16 (s16 x, s16 y, s32 h) (lobby/unk_6BE8E0.c), PlaceInfo_DrawAt: 20 of 39 words differ */
s32 PlaceInfo_DrawPlace(s32, s32, s32);
void PlaceInfo_DrawOnline(s32, s32); /* unit's own declaration: with the definition's void (s16 x, s16 y) (lobby/unk_6BE8E0.c), PlaceInfo_DrawAt 27 of 39 words differ */
void PlaceInfo_DrawVillage(s32, s32);

void PlaceInfo_DrawAt(s32 x, M2C_UNK y) {
    Font_SetSpacing(0x12, 0x12);
    if (Game_IsOnline() == 1) {
        PlaceInfo_DrawOnline(x, y);
        return;
    }
    if (gSave.unk36E8 != 0) {
        PlaceInfo_DrawPlace(x, y, 0x44);
        return;
    }
    PlaceInfo_DrawVillage(x, y);
}
