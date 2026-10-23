#ifndef GALLERY_OBJECTS_H
#define GALLERY_OBJECTS_H

#include "common.h"
#include "sprite.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"

/*
 * The rest of the shooting gallery's objects, the town's particles thrown
 * from an actor, and the house meal scene. Each object is a lobby stage
 * object with its update, four states and draw: the gun's shot
 * (GalleryShot_*), the score shown digit by digit (GalleryScore_*), the
 * particles (ActorParticles_*), the points rising from a downed target
 * (GalleryPoints_*), the meal scene (MealScene_*, with Meal_ApplyEffects)
 * and the gun's muzzle flash (GalleryMuzzle_*). Also the target board's last
 * queries and its targets' draw (GalleryBoard_DrawTarget).
 */

u8 GalleryBoard_IsCleared(StageObj *obj);
void GalleryBoard_SetTargetKind(StageObj *obj, u8 i, u8 kind);
void GalleryBoard_SwingTarget(StageObj *obj, u8 i);
void GalleryBoard_DrawTarget(Vec3f *pos, u8 kind, u32 tilt, u32 yaw, s8 *timer);
void GalleryShot_Init(StageObj *obj);
void GalleryShot_Run(StageObj *obj);
void GalleryShot_End(StageObj *obj);
void GalleryShot_Free(StageObj *obj);
void GalleryShot_Draw(DrawSlot *slot);
void GalleryShot_Spawn(Vec3f *muzzle, Vec3f *target);
void GalleryShot_GetCapsule(f32 *out, f32 *capsule, StageObj *obj, f32 radius);
void GalleryShot_SetHit(StageObj *obj);
void GalleryShot_SpawnWallHit(Vec3f *pos, Vec3f *shotDir);
void GalleryScore_Update(StageObj *obj);
void GalleryScore_Init(StageObj *obj);
void GalleryScore_Run(StageObj *obj);
void GalleryScore_End(StageObj *obj);
void GalleryScore_Free(StageObj *obj);
void GalleryScore_Draw(DrawSlot *slot);
void GalleryScore_Spawn(s16 points, u16 y);
void ActorParticles_Update(StageObj *obj);
void ActorParticles_Init(StageObj *obj);
void ActorParticles_Run(StageObj *obj);
void ActorParticles_End(StageObj *obj);
void ActorParticles_Free(StageObj *obj);
void ActorParticles_Draw(DrawSlot *slot);
void ActorParticles_SpawnSpray(void *obj);
void ActorParticles_Spawn(void *actor, void *barber, u8 k);
void GalleryPoints_Update(StageObj *obj);
void GalleryPoints_Init(StageObj *obj);
void GalleryPoints_Run(StageObj *obj);
void GalleryPoints_End(StageObj *obj);
void GalleryPoints_Free(StageObj *obj);
void GalleryPoints_Draw(DrawSlot *slot);
void GalleryPoints_Spawn(Vec3f *pos, s16 points);
void MealScene_Open(Em *npc);
void MealScene_Close(void);
void MealScene_Update(void);
void Actor_MoveToSceneSpot(Player *actor, s16 id);
void GalleryMuzzle_Update(StageObj *obj);
void GalleryMuzzle_Init(StageObj *obj);
void GalleryMuzzle_Run(StageObj *obj);
void GalleryMuzzle_End(StageObj *obj);
void GalleryMuzzle_Free(StageObj *obj);
void GalleryMuzzle_Draw(DrawSlot *slot);
void GalleryMuzzle_Spawn(f32 *pos);

#endif
