#ifndef CHAT_STATE_H
#define CHAT_STATE_H

#include "common.h"

/*
 * The game menu's chat state page, the chat page's last entry: whether the
 * chat goes to the lobby's chat or to the chosen hunters.
 * ChatPage_OpenState opens it and ChatPage_DrawState draws it.
 */

s32 ChatPage_OpenState(void);
void ChatPage_DrawState(void);

#endif
