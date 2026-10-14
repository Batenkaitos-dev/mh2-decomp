/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_menu.h"

void Gallery_DrawGunDebug(f32 *base, f32 *out);
extern M2C_UNK gGalleryGunPos;
extern MenuWork gMenuWork;

void Gallery_RunGunDebug(void) {
    Gallery_DrawGunDebug((f32 *) &gGalleryGunPos, gMenuWork.prize.muzzle);
}
