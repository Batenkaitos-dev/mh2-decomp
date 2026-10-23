#ifndef PET_STATES_H
#define PET_STATES_H

#include "common.h"
#include "em.h"

/*
 * The pets' state handlers: what the Poogie, the care pet and the house pet
 * do in each actor state. Pet_RunState runs a pet's state; the others are
 * its handlers (running off along a route, wandering, following, waiting,
 * digging) and their helpers.
 */

void Pet_StateRunOff(Em *obj);
void Pet_StateRunRoute(Em *pet);
void Pet_StateWander(Em *pet);
void Pet_StateFollow(Em *pet);
s32 Pos_InZone(void *pos, f32 *zone);
s32 Npc_IsRising(Em *actor);
void Pet_StateFollowTalker(Em *pet, Em *talker);
void Pet_StateWait(Em *pet, Em *talker);
void Pet_StateDig(Em *pet);
s32 Pet_IsNearPlayer(Em *pet);
void Pet_EndDig(Em *pet);
u8 *ActSpot_FindFirstOfKind(u16 area, u16 id);
s32 Pet_RunState(Em *pet);

#endif
