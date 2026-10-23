#ifndef DRINK_MUG_H
#define DRINK_MUG_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "stageobj.h"

/*
 * The drinking contest's mug, a lobby stage object (LOBBY_OBJ_DRINK_MUG)
 * that a drinking townsperson holds: it rests in the hand and swivels now
 * and then, flies out and circles the townsperson while the contest is on,
 * and spirals back to the hand when it ends. The townsperson's set-up spawns
 * it (DrinkMug_Spawn), and the contest's menu waits for it to be back
 * (DrinkMug_IsBack). StageObj_FindKind finds a stage object by its kind.
 */

void DrinkMug_Init(StageObj *obj);
void DrinkMug_Step(StageObj *obj);
void DrinkMug_End(StageObj *obj);
void DrinkMug_Free(StageObj *obj);
void DrinkMug_Draw(DrawSlot *slot);
void DrinkMug_Spawn(Em *npc);
s32 DrinkMug_IsBack(void);
StageObj *StageObj_FindKind(s32 kind);

#endif
