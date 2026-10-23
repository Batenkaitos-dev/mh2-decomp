#ifndef INGREDIENT_LIST_H
#define INGREDIENT_LIST_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The ingredient list of the player's house (TOWN_MODE_INGREDIENTS): talking
 * to the house's cook opens a window that lists the ingredients the player
 * holds, seven a page, with how many of each; nothing is chosen there. The
 * kitchen builds and draws its own lists with IngredientList_BuildPage and
 * IngredientList_DrawWindow.
 */

void IngredientList_Open(struct Em *cook);
void IngredientList_Update(void);
void IngredientList_BuildPage(KitchenMenu *menu, s16 page);
void IngredientList_DrawWindow(KitchenMenu *menu, s16 x, s16 y);
void IngredientList_Draw(MenuTask *task);

#endif
