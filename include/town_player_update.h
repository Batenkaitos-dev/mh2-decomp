#ifndef TOWN_PLAYER_UPDATE_H
#define TOWN_PLAYER_UPDATE_H

#include "common.h"
#include "player.h"

/*
 * The town players' update: TownPc_UpdateAll steps every shown player each
 * frame, and TownPc_Update runs one player's state group. With them, whether
 * a player is free to act in the current town mode (TownPc_IsFree) and the
 * turn toward a target yaw (Actor_TurnToTarget).
 */

void TownPc_UpdateAll(void);
void TownPc_Update(Player *pl);
s32 TownPc_IsFree(Player *pl);
void Actor_TurnToTarget(Player *pl);

#endif
