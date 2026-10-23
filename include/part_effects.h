#ifndef PART_EFFECTS_H
#define PART_EFFECTS_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"

/*
 * The part effects (STAGEOBJ_KIND_PART_FX): small effects made of parts
 * that follow key curves, such as dust at the feet or a glow at a weapon's
 * tip. The PartFx_Spawn* functions place one at a player's or a monster's
 * bone, or at a point; Effect_SpawnGround raises the puffs of a footfall or
 * a landing by the ground's kind.
 */

void PartFx_Setup(StageObj *fx);
void PartFx_Run(StageObj *fx);
void PartFx_End(StageObj *fx);
void PartFx_Delete(StageObj *fx);
void PartFx_DrawPart(DrawSlot *drawSlot);
void PartFx_PlaySound(StageObj *fx);
s32 PartFx_PutOnGround(Player *pl, f32 *pos, s32 type);
void PartFx_InitForPlayer(Player *pl, s16 bone, s32 type, StageObj *fx);
s32 PartFx_PlayerPos(f32 *pos, Player *pl, s16 bone, s32 type);
void PartFx_SpawnForPlayer(Player *pl, s16 bone, s32 type);
void PartFx_SpawnForPlayerScaled(Player *pl, s16 bone, s32 type, f32 scale);
void PartFx_InitForEm(Em *em, s32 bone, s32 type, StageObj *fx);
s32 PartFx_EmPos(f32 *pos, Em *em, s32 bone, s32 fxType);
void PartFx_SpawnForEm(Em *em, s32 bone, s32 fxType);
void PartFx_SpawnForEmScaled(Em *em, s32 bone, s32 fxType, f32 scale);
void PartFx_SpawnAt(f32 *pos, s32 type, f32 scale);
void PartFx_SpawnIfHere(void *owner, f32 *pos, s32 type, f32 scale);
void PartFx_SpawnOnGround(f32 *pos, s32 type, f32 scale);
s8 Effect_SpawnGround(void *owner, f32 *pos, s16 groundKind, f32 scale);

#endif
