#ifndef ENGINE_UTIL_H
#define ENGINE_UTIL_H

#include "common.h"
#include "motion.h"

/*
 * Small helpers of the engine: the animation curves' first and last keys'
 * times (AnimCurve_StartTime, AnimCurve_EndTime, AnimCurve_GetEndTime), with
 * a BMP file's pixels, 3-vector and 3x3 matrix helpers, the plane through
 * three points, a view frustum's planes, a free list and a list whose links
 * are tagged in bit 0.
 */

void Anim_SetKeyBase(void *buf);
f32 Math_Hermite(f32 time, f32 startValue, f32 startTime, f32 startTangent, f32 endValue, f32 endTime, f32 endTangent);
f32 AnimCurve_StartTime(AnimCurve *curve);
f32 AnimCurve_GetEndTime(AnimCurve *curve);
f32 AnimCurve_EndTime(AnimCurve *curve);
void Mtx_Copy3x3(f32 *dst, f32 *src);
f32 Vec_Length2(f32 *vec);
void Frustum_BuildPlanes(f32 *planes, f32 fov, f32 aspect, f32 near, f32 far);
void FreeList_Init(s32 nodeCount, u32 *nodes);
void TagList_Push(u32 *node, u32 *link);
void *TagList_Pop(u32 *head);

#endif
