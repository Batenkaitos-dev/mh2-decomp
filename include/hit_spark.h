#ifndef HIT_SPARK_H
#define HIT_SPARK_H

#include "common.h"
#include "sprite.h"
#include "player.h"
#include "stageobj.h"

/*
 * The hit sparks (STAGEOBJ_KIND_HIT_SPARK), the small effect objects shown
 * where a hit lands: HitSpark_Spawn and its forms (at a point, on an owner,
 * for a player, scaled, at a random angle) start one, HitSpark_Update runs
 * it by state and HitSpark_Draw draws it.
 */

void HitSpark_Update(StageObj *fx);
void HitSpark_Init(StageObj *fx);
void HitSpark_RunTimed(StageObj *fx);
void HitSpark_End(StageObj *fx);
void HitSpark_Delete(StageObj *fx);
void HitSpark_Draw(DrawSlot *slot);
void HitSpark_SpawnForPlayer(Player *pl, u8 type, f32 *pos);

void HitSpark_SpawnRandomAngle(u16 angle1, u16 angle2, u8 type, f32 *pos);
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void HitSpark_SpawnAtScaled(u16 angle2, u16 ang, s32 type, f32 *pos, f32 scale);
void HitSpark_Spawn(void *owner, s16 kind, u8 type, u8 sub, f32 *pos, f32 scale);
void HitSpark_SpawnOnOwner(void *owner, u8 type, u8 sub, f32 scale);
void HitSpark_SpawnAt(f32 *pos, s32 type, u16 ang);
void HitSpark_SpawnScaled(s16 ang, u8 type, u8 sub, f32 *pos, f32 scale);

#endif
