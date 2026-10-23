#ifndef TOWN_PLAYER_ACTIVITIES_H
#define TOWN_PLAYER_ACTIVITIES_H

#include "common.h"
#include "player.h"

/*
 * The town players' activity states of group 0 (enum TownPcState): fishing,
 * the shooting gallery's aim, the arm-wrestling contest, digging at a Poogie
 * dig spot, the drinking contest's reactions and the walk to the barber's
 * chair; and TownPc_RunActionState, which runs every state of the group.
 */

void TownPc_UpdateFishingInput(Player *pl);
void TownPc_StateFishReel(Player *pl, s32 unused);
s32 TownPc_IsInContest(Player *pl);
void TownPc_StateContestWin(Player *pl);
void TownPc_StateContestLose(Player *pl);
void TownPc_StateDig(Player *pl);
void TownPc_StateWalkToHairChair(Player *pl);
void TownPc_RunActionState(Player *pl);

#endif
