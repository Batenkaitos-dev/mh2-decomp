#ifndef LOCK_ON_MARK_H
#define LOCK_ON_MARK_H

#include "common.h"
#include "sprite.h"
#include "player.h"
#include "stageobj.h"

/*
 * The lobby's lock-on mark (STAGEOBJ_KIND_LOCK_ON_MARK): LockOnMark_Spawn
 * makes it for the local player, and while the player deals with a
 * townsperson it turns and bobs above it (the LockOnMark_* states,
 * LockOnMark_Draw).
 */

void LockOnMark_Spawn(Player *pl);
void LockOnMark_Update(StageObj *mark);
void LockOnMark_Setup(StageObj *mark);
void LockOnMark_Run(StageObj *mark);
void LockOnMark_End(StageObj *mark);
void LockOnMark_Delete(StageObj *mark);
void LockOnMark_Draw(DrawSlot *drawSlot);

#endif
