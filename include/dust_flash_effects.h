#ifndef DUST_FLASH_EFFECTS_H
#define DUST_FLASH_EFFECTS_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"

/*
 * Six kinds of stage objects drawn as effects: ShotFlash (the flash, smoke,
 * sparks and pieces of a shot or an impact), AlertMark (the mark over a
 * monster), DustCloud (the ground puffs of monsters and players),
 * FallingDebris, StunStars (the stars over a stunned hunter or monster) and
 * MashDebris (the pieces a player sheds in the mash status). Here are each
 * kind's creators, state handlers and draw callbacks.
 */

void ShotFlash_Setup(StageObj *obj);
void ShotFlash_Run(StageObj *obj);
void ShotFlash_Draw(DrawSlot *node);
void ShotFlash_SetupPuffs(StageObj *obj);
void ShotFlash_RunPuffs(StageObj *obj);
void ShotFlash_DrawPuff(DrawSlot *node);
void ShotFlash_SetupSpark(StageObj *obj);
void ShotFlash_RunSpark(StageObj *obj);
void ShotFlash_DrawSpark(DrawSlot *node);
void ShotFlash_SetupPieces(StageObj *obj);
void ShotFlash_RunPieces(StageObj *obj);
void ShotFlash_DrawPiece(DrawSlot *node);
void ShotFlash_End(StageObj *obj);
void ShotFlash_Free(StageObj *obj);
void ShotFlash_PlaySound(StageObj *obj, f32 *pos);
StageObj *ShotFlash_Alloc(s32 objType);
void ShotFlash_CreateOnBone(s32 owner, s16 bone, s32 objType);
void ShotFlash_CreateAt(f32 *pos, s32 objType, u8 colorSet);
void ShotFlash_CreateAtScaled(f32 *pos, s32 objType, u8 colorSet, f32 scale);
void ShotFlash_CreateFromShot(u8 *shot, s32 objType, u8 colorSet);
void ShotFlash_CreateOnBoneColored(Player *pl, s16 bone, s32 objType, u8 colorSet);
void ShotFlash_CreateAtAngles(f32 *pos, s32 objType, u16 pitch, u16 yaw);
void ShotFlash_CreateGunlanceShell(Player *pl, s16 variant);
void AlertMark_Setup(StageObj *obj);
void AlertMark_Run(StageObj *obj);
void AlertMark_End(StageObj *obj);
void AlertMark_Free(StageObj *obj);
void AlertMark_Draw(DrawSlot *node);
void AlertMark_Create(void *owner, s16 bone, u8 objType);
void DustCloud_Setup(StageObj *obj);
void DustCloud_Run(StageObj *obj);
void DustCloud_End(StageObj *obj);
void DustCloud_Free(StageObj *obj);
void DustCloud_Draw(DrawSlot *node);
s16 DustCloud_PlaceAtEm(f32 *pos, Em *em, s16 objType, s16 place);
void DustCloud_Create(Em *em, s16 objType, s16 place, f32 scale);
void DustCloud_CreateAtYaw(f32 *pos, s32 objType, u16 yaw, f32 scale);
void DustCloud_CreateOnPlayer(Player *pl, s16 objType, s16 place, f32 scale);
void DustCloud_CreateAt(f32 *pos, u8 objType, u8 variant, f32 scale);
void FallingDebris_Create(Em *em, u8 objType);
void FallingDebris_Update(StageObj *obj);
void FallingDebris_Setup(StageObj *obj);
void FallingDebris_Run(StageObj *obj);
void FallingDebris_End(StageObj *obj);
void FallingDebris_Free(StageObj *obj);
void FallingDebris_Draw(DrawSlot *node);
void StunStars_CreatePlayer(Player *pl, u8 objType);
void StunStars_Update(StageObj *obj);
void StunStars_Setup(StageObj *obj);
void StunStars_Run(StageObj *obj);
void StunStars_End(StageObj *obj);
void StunStars_Free(StageObj *obj);
void StunStars_Draw(DrawSlot *node);
void MashDebris_Update(StageObj *obj);
void MashDebris_Setup(StageObj *obj);
void MashDebris_Run(StageObj *obj);
void MashDebris_End(StageObj *obj);
void MashDebris_Free(StageObj *obj);
void MashDebris_Draw(void *slotArg);
void MashDebris_Create(s8 variant, s16 *player);

#endif
