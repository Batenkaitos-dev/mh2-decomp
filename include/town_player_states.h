#ifndef TOWN_PLAYER_STATES_H
#define TOWN_PLAYER_STATES_H

#include "common.h"
#include "player.h"

/*
 * The town players' state handlers of group 0 (enum TownPcState,
 * TownPc_State*): idle, walk and run, the item hand-over between two players,
 * the bed, the drunk idle, the table seats with their poses and drinks, the
 * kitchen's order and the meal's ways of rising, and the walks to an action
 * spot or a seat (TownPc_WalkToSpot, TownPc_TakeSeat, Contest_TakeSeat).
 */

void TownPc_UnwindLookYaw(Player *pl);
void TownPc_StateIdle(Player *pl);
void TownPc_StateWalk(Player *pl, s32 variant);
void TownPc_StateRun(Player *pl);
void TownPc_StateRunStop(Player *pl);
s32 TownPc_TakeOfferedItem(Player *pl);
void TownPc_StateOfferItem(Player *pl);
void TownPc_StateTakeItemWait(Player *pl);
void TownPc_StateOfferItemEnd(Player *pl);
void TownPc_StateTakeItem(Player *pl);
void TownPc_StateBedLieDown(Player *pl);
void TownPc_StateBedSleep(Player *pl, s32 unused);
void TownPc_StateBedGetUp(Player *pl, s32 unused);
void TownPc_StateTurnAround(Player *pl);
void TownPc_StateDrunkCollapse(Player *pl);
void TownPc_StateDrunkGetUp(Player *pl);
s32 TownPc_IsSeatedState(u8 group, u8 state);
void TownPc_StateHairChair(Player *pl);
void TownPc_StateSitDown(Player *pl, s32 alreadySeated);
s32 TownPc_IsLeaveSeatState(s32 group, s32 state);
void TownPc_StateStandUp(Player *pl);
void TownPc_StateBedNap(Player *pl);
void TownPc_StateRest(Player *pl, s8 shortForm);
void TownPc_StateGetUpFaceUp(Player *pl);
void TownPc_StateSeatPoseRight(Player *pl);
void TownPc_StateSeatPoseRightEnd(Player *pl);
void TownPc_StateDrunkIdle(Player *pl);
void TownPc_StateKitchenOrder(Player *pl);
void TownPc_StateThrown(Player *pl);
void TownPc_StateGrabbed(Player *pl);
void TownPc_StateSeatDrink(Player *pl);
void TownPc_StateSeatCross(Player *pl);
void TownPc_StateSeatPoseLeftEnd(Player *pl);
void TownPc_StateSeatPoseLeft(Player *pl);
void TownPc_StateSeatPoseDown(Player *pl);
void TownPc_StateSeatPoseDownEnd(Player *pl);
void TownPc_StateMealRise(Player *pl, s8 goodMeal);
void TownPc_StateSeatPoseUp(Player *pl);
void TownPc_StateSeatPoseUpEnd(Player *pl);
void TownPc_StateMealServed(Player *pl);
void TownPc_StateMealCollapse(Player *pl);
void TownPc_State43(Player *pl);
void TownPc_WalkToSpot(Player *pl);
void Contest_TakeSeat(void);
void TownPc_TakeSeat(void);

#endif
