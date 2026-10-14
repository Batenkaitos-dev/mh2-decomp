/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "lobby_globals.h"

void ChatLog_ResetScroll(void);

s32 ChatLog_Close(void) {
    gHelpShow = 0;
    ChatLog_ResetScroll();
    return 0;
}
