#ifndef HOUSE_ACCESS_H
#define HOUSE_ACCESS_H

#include "common.h"
#include "menu_task.h"

/*
 * The house access menu (TOWN_MODE_HOUSE_ACCESS): the town's members, with
 * whether each one's house is open. Picking an open house sends the visit
 * and opens the area menu on that house's area.
 */

void HouseAccess_Open(void);
void HouseAccess_Close(void);
void HouseAccess_Update(void);
void HouseAccess_Draw(MenuTask *task);

#endif
