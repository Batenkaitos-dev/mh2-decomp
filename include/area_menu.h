#ifndef AREA_MENU_H
#define AREA_MENU_H

#include "common.h"
#include "lobby_state.h"

/*
 * The area menu (TOWN_MODE_AREA_MENU), which a door opens (AreaMenu_Open),
 * and the walk to the next area of the village or the town (AreaMenu_GoTo),
 * with the areas' own entry points (gSpawnPoints).
 */

void AreaMenu_Open(u16 destArea);
void AreaMenu_Update(void);
void AreaMenu_GoTo(u16 area);
extern SpawnPt gSpawnPoints[15];

#endif
