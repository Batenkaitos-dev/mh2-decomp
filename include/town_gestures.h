#ifndef TOWN_GESTURES_H
#define TOWN_GESTURES_H

#include "common.h"
#include "player.h"

/*
 * The town's gesture states (state group TOWN_PC_GROUP_GESTURE), the town's
 * copies of the field's gestures: TownPc_RunGestureState runs a town
 * player's gesture.
 */

void TownPc_RunGestureState(Player *pl);

#endif
