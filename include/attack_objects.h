#ifndef ATTACK_OBJECTS_H
#define ATTACK_OBJECTS_H

#include "common.h"
#include "player.h"
#include "sprite.h"
#include "em.h"
#include "objattach.h"
#include "stageobj.h"

/*
 * The attack objects the weapon states and the monsters make, as kinds of
 * ObjAttach: a player's weapon hit (PlayerAttack), the body attacks of
 * Rathalos, Fatalis and Lao-Shan Lung, the contact attacks of Kirin, Melynx,
 * Anteka and Mosswine, which report what they touch to their owner
 * (EmContactAttack), and an item a player throws or puts down (ItemAttack).
 * Each family has its Create, Update (by ObjAttach.state), Setup, Run, End
 * and Free.
 */

void PlayerAttack_Create(Player *pl, u8 attackId);
void PlayerAttack_CreateDual(Player *pl, s32 attackId, s32 mirrored);
void PlayerAttack_CreateBowStab(Player *pl, s8 attackId);
void PlayerAttack_Update(ObjAttach *obj);
void PlayerAttack_PlaceOnWeapon(ObjAttach *obj, Player *pl);
void PlayerAttack_Setup(ObjAttach *obj);
void PlayerAttack_Run(ObjAttach *obj);
void PlayerAttack_End(ObjAttach *obj);
void PlayerAttack_Free(ObjAttach *obj);
void RathalosAttack_Create(Em *em, s8 attackId);
void RathalosAttack_CreateOnObj(StageObj *src, s8 attackId);
void RathalosAttack_Update(ObjAttach *obj);
void RathalosAttack_Setup(ObjAttach *obj);
void RathalosAttack_Run(ObjAttach *obj);
void RathalosAttack_End(ObjAttach *obj);
void RathalosAttack_Free(ObjAttach *obj);
ObjAttach *EmContactAttack_Create(Em *em, s8 attackId);
void EmContactAttack_Update(ObjAttach *obj);
void EmContactAttack_Setup(ObjAttach *obj);
void EmContactAttack_Run(ObjAttach *obj);
void EmContactAttack_End(ObjAttach *obj);
void EmContactAttack_Free(ObjAttach *obj);
void EmContactAttack_DrawNop(ObjAttach *obj);
void ItemAttack_Create(void *owner, s32 type, s8 throwRow);
void ItemAttack_Update(ObjAttach *obj);
void ItemAttack_Setup(ObjAttach *obj);
void ItemAttack_SetupThrown(ObjAttach *obj);
void ItemAttack_InitParts(ObjAttach *obj);
void ItemAttack_SetupPlaced(ObjAttach *obj);
void ItemAttack_SetupHopping(ObjAttach *obj);
void ItemAttack_Run(ObjAttach *obj);
void ItemAttack_RunThrown(ObjAttach *obj);
void ItemAttack_RunPlaced(ObjAttach *obj);
void ItemAttack_RunHopping(ObjAttach *obj);
void ItemAttack_OnHit(ObjAttach *obj);
void ItemAttack_Deflect(ObjAttach *obj);
void ItemAttack_FlyBack(ObjAttach *obj);
s32 ItemAttack_BounceOff(ObjAttach *obj);
void ItemAttack_FreeDrawSlots(ObjAttach *obj);
void ItemAttack_End(ObjAttach *obj);
void ItemAttack_Free(ObjAttach *obj);
void ItemAttack_DrawPart(DrawSlot *slot);
void ItemAttack_Draw(DrawSlot *slot);
void FatalisAttack_Create(Em *em, s8 attackId);
void FatalisAttack_CreateOnObj(StageObj *src, s8 attackId);
void FatalisAttack_Update(ObjAttach *obj);
void FatalisAttack_Setup(ObjAttach *obj);
void FatalisAttack_Run(ObjAttach *obj);
void FatalisAttack_End(ObjAttach *obj);
void FatalisAttack_Free(ObjAttach *obj);
void LaoShanAttack_Create(Em *em, s8 attackId);
void LaoShanAttack_CreateSpan(Em *em, s32 attackId, s32 span);
void LaoShanAttack_CreateAt(Em *em, f32 *pos, s8 attackId);
void LaoShanAttack_CreateOnObj(StageObj *src, s8 attackId);
void LaoShanAttack_Update(ObjAttach *obj);
void LaoShanAttack_Setup(ObjAttach *obj);
void LaoShanAttack_Run(ObjAttach *obj);
void LaoShanAttack_End(ObjAttach *obj);
void LaoShanAttack_Free(ObjAttach *obj);
void LaoShanAttack_DrawNop(ObjAttach *obj);

#endif
