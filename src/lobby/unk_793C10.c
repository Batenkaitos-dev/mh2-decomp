/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "quest.h"
#include "m2c_macros.h"

QuestInfo *Quest_GetInfoAny(u16);
extern u16 gLobbyQuestId;

s32 func_00793C10(void) {
    return (Quest_GetInfoAny(gLobbyQuestId)->flags & 0x400000) != 0;
}
