#ifndef KITCHEN_H
#define KITCHEN_H

#include "common.h"
#include "lobby_menu.h"
#include "menu_task.h"

/*
 * The kitchen (TOWN_MODE_KITCHEN): the meal the house's cook serves at a
 * table. The player picks two ingredients for every diner, or the cook
 * serves a set course; the cooking lesson plays and the meal's bonuses for
 * the next quest are given. Kitchen_Open and Kitchen_Update run it, and
 * Kitchen_Draw draws it. Online, Kitchen_Broadcast tells the other diners,
 * whose menu calls Kitchen_EatSetCourse, Kitchen_EatDish and
 * Kitchen_DrawConfirm. gKitchenIngredients holds the ingredient groups.
 */

void Kitchen_Open(void);
void Kitchen_Broadcast(KitchenMenu *menu, u8 kind);
s32 Kitchen_SetCourseReady(void);
void Kitchen_ListDiners(KitchenMenu *menu);
void Kitchen_Update(void);
s32 Kitchen_IsIngredient(u16 item);
void Kitchen_EatSetCourse(u16 course, u16 rand);
s16 Kitchen_EatDish(s16 season, u16 first, u16 second, u16 rand);
s32 Kitchen_ChooseIngredient(KitchenMenu *menu, u16 pad);
void Kitchen_DrawConfirm(s16 x, s16 y, u16 firstPick, u16 secondPick);
void Kitchen_Draw(MenuTask *task);
extern KitchenIngredient *gKitchenIngredients[12];

#endif
