#ifndef AMBIENT_LIFE_H
#define AMBIENT_LIFE_H

#include "common.h"
#include "sprite.h"
#include "model.h"
#include "obj.h"

/*
 * AmbientLife, the area objects of kind 9: the small animals and the falling
 * leaves that move about some areas. An area's spawner keeps up to 32 of them
 * going, each a kind of mover: a falling leaf, a wanderer, a darter, a steady
 * flyer, a butterfly, a spinner, a circling bird, a glow bug or a crossing
 * flyer. Here are the kind's update, state handlers and draw, and each
 * mover's own Setup, Run and Draw.
 */

void AmbientLife_Create(u8 unusedSet);
s16 AmbientLife_FindFreeChild(Obj *parent);
void AmbientLife_CreateAt(void *spot, s16 childType);
void AmbientLife_Update(Obj *obj);
void AmbientLife_Setup(Obj *obj);
void AmbientLife_Run(Obj *obj);
void AmbientLife_End(Obj *obj);
void AmbientLife_Free(Obj *obj);
void AmbientLife_Draw(DrawSlot *drawSlot);
void AmbientLife_DrawModel(ModelMatSet *set, s32 mtx);
void AmbientLife_SetupSpawner(Obj *spawner);
void AmbientLife_RunSpawner(Obj *spawner);
void AmbientLife_SetupFallingLeaf(Obj *obj);
void AmbientLife_RunFallingLeaf(Obj *obj);
void AmbientLife_DrawFallingLeaf(DrawSlot *drawSlot);
void AmbientLife_SetupWanderer(Obj *obj);
void AmbientLife_RunWanderer(Obj *obj);
void AmbientLife_DrawWanderer(DrawSlot *drawSlot);
void AmbientLife_SetupDarter(Obj *obj);
void AmbientLife_RunDarter(Obj *obj);
void AmbientLife_DrawDarter(DrawSlot *drawSlot);
void AmbientLife_SetupSteadyFlyer(Obj *obj);
void AmbientLife_RunSteadyFlyer(Obj *obj);
void AmbientLife_DrawSteadyFlyer(DrawSlot *drawSlot);
void AmbientLife_SetupButterfly(Obj *obj);
void AmbientLife_RunButterfly(Obj *obj);
void AmbientLife_DrawButterfly(DrawSlot *drawSlot);
void AmbientLife_SetupSpinner(Obj *obj);
void AmbientLife_RunSpinner(Obj *obj);
void AmbientLife_DrawSpinner(DrawSlot *drawSlot);
void AmbientLife_SetupCirclingBird(Obj *obj);
u32 AmbientLife_RunCirclingFlyer(Obj *obj);
void AmbientLife_DrawCirclingFlyer(DrawSlot *drawSlot);
void AmbientLife_ResetGlowBug(Obj *obj);
void AmbientLife_SetupGlowBug(Obj *obj);
void AmbientLife_RunGlowBug(Obj *obj);
void AmbientLife_DrawGlowBug(DrawSlot *drawSlot);
void AmbientLife_SetupCrossingFlyer(Obj *obj);
void AmbientLife_RunCrossingFlyer(Obj *obj);
void AmbientLife_DrawCrossingFlyer(DrawSlot *drawSlot);

void AmbientLife_SetupUnused(void);
void AmbientLife_RunUnused(void);
void AmbientLife_DrawUnused(void);
void AmbientLife_SetupUnusedNext(void);
void AmbientLife_RunUnusedNext(void);
void AmbientLife_DrawUnusedNext(void);
extern f32 AmbientLife_GlowBugTowerPos[32];
extern f32 AmbientLife_CrossingFlyerPoints[6];

#endif
