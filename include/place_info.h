#ifndef PLACE_INFO_H
#define PLACE_INFO_H

#include "common.h"

/*
 * The field menu's place info page in the town: its open (PlaceInfo_Open)
 * and input, and the window that says where the player is: the season and
 * the area's name (LobbyArea_GetName), online the server, gate and room
 * chosen, and in the village the facilities' levels.
 */

s32 PlaceInfo_Open(void);
void *LobbyArea_GetName(u16 id);

#endif
