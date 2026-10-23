#ifndef HIT_EFFECTS_H
#define HIT_EFFECTS_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "objattach.h"
#include "player.h"
#include "stageobj.h"

/*
 * Six kinds of stage objects drawn as effects: PlayerHitBurst (the burst on a
 * hunter who was hit), ItemProp (what an item leaves in the stage: smoke, a
 * bait put down, a dropped carried item, an eating puff), FireBlast (bomb
 * blasts and their smoke, fires, the special mine point's glow), ElementBurst
 * (an elemental or status hit's burst, rings), HitSplash (the spray, flashes
 * and sparks where a hit lands) and AttackBurst (the monsters' shot flares,
 * trails, impacts and rocks). Here are the kinds' creators, state handlers and
 * draw callbacks.
 */

/* a part of an ElementBurst (44 bytes): ElementBurst_PartCounts[type] of them
 * in the object's buffer (StageObj.buf) */
typedef struct ElementBurstPart {
    s16 kind;                   /* 0x00: its index; type 9: 1 */
    s16 slot;                   /* 0x02: its draw slot */
    f32 pos[3];                 /* 0x04 */
    f32 sc[3];                  /* 0x10: its scale, from a curve by its frame */
    f32 scale;                  /* 0x1C */
    DrawSlot *node;                /* 0x20 */
    s16 frame;                  /* 0x24: its frame, from a delay (negative) */
    u16 angle;                  /* 0x26: a random angle */
    s16 spin;                  /* 0x28: its spin */
    u8 alpha;                   /* 0x2A: from a curve by its frame */
    u8 unk2B;
} ElementBurstPart;

void PlayerHitBurst_Setup(StageObj *obj);
void PlayerHitBurst_Run(StageObj *obj);
void PlayerHitBurst_End(StageObj *obj);
void PlayerHitBurst_Free(StageObj *obj);
void PlayerHitBurst_Draw(DrawSlot *node);
void ItemProp_Update(StageObj *obj);
void ItemProp_Setup(StageObj *obj);
void ItemProp_Run(StageObj *obj);
void ItemProp_Draw(struct DrawSlot *node);
void ItemProp_SetupMeat(StageObj *obj);
void ItemProp_RunMeat(StageObj *obj);
void ItemProp_DrawMeat(StageObj *obj);
void ItemProp_SetupSmoke(StageObj *obj);
void ItemProp_RunSmoke(StageObj *obj);
void ItemProp_DrawSmoke(StageObj *obj, DrawSlot *node);
void ItemProp_SetupBait(StageObj *obj);
void ItemProp_RunBait(StageObj *obj);
void ItemProp_DrawBait(StageObj *obj);
void ItemProp_OffsetFromBone(f32 *out, f32 *boneMtx, f32 *ofs);
void ItemProp_SetupDropped(StageObj *obj);
void ItemProp_RunDropped(StageObj *obj);
void ItemProp_DrawDropped(StageObj *obj, DrawSlot *node);
void ItemProp_SetupPuffs(StageObj *obj);
void ItemProp_RunPuffs(StageObj *obj);
void ItemProp_DrawPuff(StageObj *obj, DrawSlot *node);
void ItemProp_End(StageObj *obj);
void ItemProp_Free(StageObj *obj);
void ItemProp_PlaySound(StageObj *obj, s16 cue);
StageObj *ItemProp_Alloc(s32 type);
void ItemProp_CreateAtPlayer(Player *pl, s32 k);
void ItemProp_CreateFromAttach(ObjAttach *attack, s32 k, u8 colorRow);
void ItemProp_Create(f32 *pos, s16 yaw, s32 k, u8 colorRow);
void ItemProp_CreateBait(Player *pl, s32 k, u8 baitKind);
void FireBlast_Setup(StageObj *obj);
void FireBlast_SetupParts(StageObj *obj);
void FireBlast_SetupGlow(StageObj *obj);
void FireBlast_Run(StageObj *obj);
void FireBlast_RunParts(StageObj *obj);
void FireBlast_RunGlow(StageObj *obj);
void FireBlast_End(StageObj *obj);
void FireBlast_Free(StageObj *obj);
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
void FireBlast_CreateAt(f32 *pos, s16 k);
void FireBlast_CreateOwned(f32 *pos, s16 k, f32 scale, void *owner);
void FireBlast_CreateOnEm(Em *em, s16 k);
void FireBlast_CreateScaledExact(f32 *pos, s16 k, f32 scale);
void ElementBurst_Setup(StageObj *obj);
void ElementBurst_Run(StageObj *obj);
void ElementBurst_End(StageObj *obj);
void ElementBurst_Free(StageObj *obj);
void ElementBurst_Draw(DrawSlot *node);
s32 ElementBurst_RepeatRing(StageObj *obj, ElementBurstPart *piece);
s32 ElementBurst_RoarBone(StageObj *obj);
void ElementBurst_PlaySound(StageObj *obj);
StageObj *ElementBurst_CreateAt(f32 *pos, s32 k, s32 repeats, f32 scale);
void ElementBurst_CreateAtSilent(f32 *pos, s32 k, s32 repeats, f32 scale);
void ElementBurst_CreateOnEm(Em *em, u8 k, u8 repeats, f32 scale);
void ElementBurst_CreateAtYaw(f32 *pos, u8 k, u8 quiet, u16 yaw, f32 scale);
void HitSplash_Setup(StageObj *obj);
void HitSplash_Run(StageObj *obj);
void HitSplash_End(StageObj *obj);
void HitSplash_Free(StageObj *obj);
void HitSplash_Draw(DrawSlot *node);
s32 HitSplash_SpinSpeed(s16 x);
void HitSplash_PlaySound(StageObj *obj, f32 *pos);
void HitSplash_CreateAt(f32 *pos, u8 k, s16 life, u8 spacing, f32 scale);
void HitSplash_CreateByHitFx(Em *em, f32 *pos, u8 k, s16 hitKind, s16 hitFxKind, f32 scale);
void HitSplash_CreateAtScaled(f32 *pos, u8 k, f32 scale);
f32 StageObj_RandUnit(void);
f32 StageObj_RandSigned(void);
void AttackBurst_Update(StageObj *obj);
void AttackBurst_Setup(StageObj *obj);
void AttackBurst_Run(StageObj *obj);
void AttackBurst_End(StageObj *obj);
void AttackBurst_Draw(DrawSlot *node);
void AttackBurst_SetupParts(StageObj *obj);
void AttackBurst_RunParts(StageObj *obj);
void AttackBurst_DrawPart(DrawSlot *node);
void AttackBurst_SetupRocks(StageObj *obj);
void AttackBurst_RunRocks(StageObj *obj);
void AttackBurst_DrawRock(DrawSlot *node);
void AttackBurst_EndParts(StageObj *obj);
void AttackBurst_EndRocks(StageObj *obj);
void AttackBurst_Free(StageObj *obj);
void AttackBurst_PlaceAtBone(StageObj *obj);
s32 AttackBurst_OwnerInMotion(StageObj *obj);
void AttackBurst_CreateAtBone(Em *em, s16 bone, s16 objType, u16 sizeCode);
void AttackBurst_CreateAtScaled(f32 *pos, s32 yaw, s16 objType, f32 scale);
void AttackBurst_CreateAtBoneTilted(Em *em, s16 bone, s16 objType, u16 tilt);
void AttackBurst_CreateAt(f32 *pos, s16 objType, u8 emKind, f32 scale);
void AttackBurst_CreateAtYaw(f32 *pos, s16 objType, u8 emKind, s32 yaw, f32 scale);
extern s16 AttackBurst_PartCounts[24];

#endif
