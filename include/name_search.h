#ifndef NAME_SEARCH_H
#define NAME_SEARCH_H

#include "common.h"
#include "menu_task.h"
#include "lobby_menu.h"

/*
 * The name search, from the game menu: the player types a hunter's name,
 * the server is asked for the hunters of that name, and choosing one from
 * the pages of results opens that hunter's info. NameSearch_Open starts it,
 * NameSearch_Update runs it and NameSearch_Draw draws it.
 */

void NameSearch_Open(void);
s32 NameSearch_Update(u16 pad);
s32 NameSearch_InputName(NameSearchMenu *menu, u16 pad);
void NameSearch_BuildPage(NameSearchMenu *menu, s16 page);
void NameSearch_ShowResults(NameSearchMenu *menu);
s32 NameSearch_ResultsInput(NameSearchMenu *menu, u16 pad);
void NameSearch_Draw(MenuTask *task);
void NameSearch_Draw2(MenuTask *task);

#endif
