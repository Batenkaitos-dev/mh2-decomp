#ifndef KITCHEN_GUEST_H
#define KITCHEN_GUEST_H

#include "common.h"
#include "lobby_menu.h"

/*
 * The kitchen seen by a guest (TOWN_MODE_KITCHEN_GUEST): online, the other
 * diners follow the host's meal through the kitchen's town messages.
 * KitchenGuest_Open shows the host choosing, KitchenGuest_SendCook sends the
 * house's cook to the host, KitchenGuest_OnMealChosen plays the meal and
 * KitchenGuest_OnMealEnd ends it; KitchenGuest_Update runs the menu each
 * frame.
 */

void KitchenGuest_Open(void);
void KitchenGuest_Close(void);
void KitchenGuest_Update(void);
void KitchenGuest_SendCook(s32 spot);
void KitchenGuest_OnMealChosen(KitchenMeal *meal);
void KitchenGuest_OnMealEnd(void);

#endif
