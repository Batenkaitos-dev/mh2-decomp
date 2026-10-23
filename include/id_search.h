#ifndef ID_SEARCH_H
#define ID_SEARCH_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The id search, from the game menu: the player types a hunter id and the
 * hunter's info opens on it. IdSearch_Open starts it, IdSearch_Update runs
 * it a frame at a time and IdSearch_Draw draws it.
 */

void IdSearch_Open(void);
s32 IdSearch_Update(u16 pad);
s32 IdSearch_InputId(void *arg, u16 pad);
void IdSearch_DrawForm(IdSearchMenu *menu, s16 x, s16 y);
void IdSearch_Draw(MenuTask *task);
void IdSearch_Draw2(MenuTask *task);

#endif
