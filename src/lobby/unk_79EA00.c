/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

extern s16 gRecruitFrames;
extern s16 gFriendCheckFrames;
extern s16 gAreaSearchFrames;
extern s16 gTownMoveFrames;

void Lobby_ResetTimers(void) {
    gTownMoveFrames = 0x14;
    gFriendCheckFrames = 0x1518;
    gAreaSearchFrames = 0x708;
    gRecruitFrames = 0x1518;
}
