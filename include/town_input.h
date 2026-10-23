#ifndef TOWN_INPUT_H
#define TOWN_INPUT_H

#include "common.h"
#include "em.h"
#include "player.h"

/*
 * The local town player's input: the walk, the dash and the action button
 * (a talk, or the menu or act of the action spot the player stands at;
 * TownPc_CheckAction), and the steering of the other members' players toward
 * the points their messages send (TownPc_SteerToDest). With them, four
 * helpers of the input's cases: a townsperson's delayed talk sound, the
 * start of an offer the pouch asked for, the walk to the bed and the bed's
 * sleep-event test.
 */

void Npc_PlaySeDelayed(Em *npc, void *seId);
void TownPc_StartRequestedOffer(Player *pl);
void TownPc_WalkToBed(void);
s32 Town_SleepEventPending(void);
void TownPc_SteerToDest(Player *pl);
void TownPc_CheckAction(Player *pl);

#endif
