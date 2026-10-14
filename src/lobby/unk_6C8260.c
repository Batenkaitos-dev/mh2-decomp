/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void Talk_Say(s32, s8);
void LobbyMenu_Init(void);
void LobbyMenu_SetDraw(void *, void *, void *, void *);
void *memset(void *dst, s32 c, u32 n);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
extern M2C_UNK gMenuWork;
extern s8 gMenuInputLock;
extern u16 gLobbyEnterRand;
extern M2C_UNK IngredientList_Draw;

void IngredientList_Open(u8 *unused0) {
    gTownMenuOpen = 1;
    gTownMode = 0x47;
    memset(&gMenuWork, 0, 0xF00);
    LobbyMenu_Init();
    LobbyMenu_SetDraw(&IngredientList_Draw, 0, 0, 0);
    gMenuInputLock = 6;
    Talk_Say(((s32) ((s32) gLobbyEnterRand >> 1) % 15) + 0xB, 0);
}
