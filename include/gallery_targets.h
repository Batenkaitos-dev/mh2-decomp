#ifndef GALLERY_TARGETS_H
#define GALLERY_TARGETS_H

#include "common.h"
#include "sprite.h"
#include "stageobj.h"

/*
 * The shooting gallery's target board, continued: its draw and spawn
 * (GalleryBoard_Draw, GalleryBoard_Spawn), the bursts a hit leaves
 * (GalleryBurst_SpawnHit, GalleryBurst_SpawnDowned), and the queries the
 * gallery's shots make of a target: its position, size, facing, tilt,
 * picture and points, and the hits that count off it and down it.
 */

void GalleryBoard_Draw(DrawSlot *slot);
void GalleryBoard_Spawn(u8 *kinds, u8 *order, Vec3f *boardDir, f32 holdFrames);
void GalleryBoard_GetTargetPos(Vec3f *out, StageObj *obj, u8 i);
void GalleryBoard_DownTarget(StageObj *obj, u8 i);
s32 GalleryBoard_IsFlippedDown(StageObj *obj);
void GalleryBurst_SpawnDowned(Vec3f *pos);
void GalleryBurst_SpawnHit(Vec3f *pos);
u8 GalleryBoard_GetTargetSize(StageObj *obj, u8 i);
u16 GalleryBoard_GetTargetFacing(StageObj *obj, u8 i);
f32 GalleryBoard_GetTargetTilt(StageObj *obj, u8 i);
u16 GalleryBoard_GetTargetPicture(StageObj *obj, u8 i);
s32 GalleryBoard_HitTarget(StageObj *obj, u8 i);
s16 GalleryBoard_GetTargetPoints(StageObj *obj, u8 i);
extern s8 gGalleryBurstDelaysA[8];
extern s8 gGalleryBurstDelaysB[8];

#endif
