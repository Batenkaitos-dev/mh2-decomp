#ifndef BIRD_FLOCK_H
#define BIRD_FLOCK_H

#include "common.h"
#include "lobby_vobj.h"
#include "sprite.h"
#include "stageobj.h"

/*
 * A flock of birds in the town's hubs, a lobby stage object
 * (LOBBY_OBJ_BIRD_FLOCK): each bird sits on a spot of its own and turns
 * about, flies off now and then, and lands on a free spot far away. The
 * hub's set-up spawns the flock (BirdFlock_Spawn); the other functions are
 * the object's update, states and draw.
 */

void BirdFlock_Update(StageObj *obj);
void BirdFlock_Init(StageObj *obj);
void FlockBird_Update(BirdFlock *flock, FlockBird *bird, s32 i);
void BirdFlock_Run(StageObj *obj);
void BirdFlock_End(StageObj *obj);
void BirdFlock_Free(StageObj *obj);
void BirdFlock_Draw(DrawSlot *slot);
void BirdFlock_Spawn(s8 type);

#endif
