#ifndef SHAKALAKA_TEOSTRA_BULLDROME_H
#define SHAKALAKA_TEOSTRA_BULLDROME_H

#include "common.h"
#include "em.h"
#include "capsule.h"
#include "objattach.h"

/*
 * Three attack-object families, as kinds of ObjAttach: the attacks of the
 * Shakalaka, Teostra and Bulldrome. Each family has its Create, Update (by
 * ObjAttach.state), Setup, Run, End and Free, and an attack's type picks its
 * row of the family's tables (the hit's power, kind, element and shapes).
 */

void ShakalakaAttack_Create(Em *em, s8 kind);
void ShakalakaAttack_Update(ObjAttach *o);
void ShakalakaAttack_Setup(ObjAttach *o);
void ShakalakaAttack_Run(ObjAttach *o);
void ShakalakaAttack_End(ObjAttach *o);
void ShakalakaAttack_Free(ObjAttach *o);
void ShakalakaAttack_Draw(ObjAttach *o);
void TeostraAttack_Create(Em *em, s32 kind);
void TeostraAttack_Update(ObjAttach *o);
void TeostraAttack_Setup(ObjAttach *o);
void TeostraAttack_Run(ObjAttach *o);
void TeostraAttack_End(ObjAttach *o);
void TeostraAttack_Free(ObjAttach *o);
ObjAttach *BulldromeAttack_Create(Em *em, s8 kind);
void BulldromeAttack_Update(ObjAttach *o);
void BulldromeAttack_Setup(ObjAttach *o);
void BulldromeAttack_Run(ObjAttach *o);
void BulldromeAttack_End(ObjAttach *o);
void BulldromeAttack_Free(ObjAttach *o);
void BulldromeAttack_Draw(ObjAttach *o);
extern HitShape *ShakalakaAttack_HitShapes[4];
extern void *ShakalakaAttack_RowsPtr;
extern HitShape *TeostraAttack_HitShapes[20];
extern void *TeostraAttack_RowsPtr;
extern HitShape *BulldromeAttack_HitShapes[4];
extern void *BulldromeAttack_RowsPtr;

#endif
