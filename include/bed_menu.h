#ifndef BED_MENU_H
#define BED_MENU_H

#include "common.h"
#include "menu_task.h"

/*
 * The bed's menu in the hunter's house (TOWN_MODE_SAVE: TownSave_*): the
 * save, a rest to the next half of the day and a sleep to the next day.
 * Also the save menu over the town (TownNetMenu_*), which stops the town's
 * input and music, runs sub_main's card menu and starts the town again;
 * the game menu's save line runs it too.
 */

void TownSave_Open(void);
void TownNetMenu_Resume(void);
s32 TownSave_Update(void);
void TownSave_Draw(MenuTask *task);
void TownNetMenu_Begin(void);
s32 TownNetMenu_Update(s32 fromGameMenu);

#endif
