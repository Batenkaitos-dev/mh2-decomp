#ifndef BONE_EFFECTS_H
#define BONE_EFFECTS_H

#include "common.h"
#include "sprite.h"
#include "player.h"
#include "stageobj.h"

/*
 * The kind 6 effect objects: small effects that follow a hunter's or a
 * monster's bone (an item's glow, the poison and sleep effects, the charge
 * glow, the attack and defense up icons over a hunter, a hit's effect).
 * Effect06_New, Effect06_SpawnAtBone and Effect06_SpawnWeapon make one by
 * type; Effect06_Run moves and animates its parts along their key curves,
 * and Effect06_DrawPart draws each part with a model of the common set.
 */

void Effect06_Setup(StageObj *effect);
void Effect06_PlaceAtBone(StageObj *effect);
void Effect06_Run(StageObj *effect);
void Effect06_End(StageObj *effect);
void Effect06_Delete(StageObj *effect);
void Effect06_DrawPart(DrawSlot *slot);
void Effect06_PlaySound(StageObj *effect, f32 *pos);
StageObj *Effect06_New(Player *pl, s16 type, u8 sub);
void Effect06_SpawnAtBone(Player *pl, s16 type, u8 sub, s16 bone, f32 scale);
void Effect06_SpawnWeapon(Player *pl, s16 type, u8 sub);

#endif
