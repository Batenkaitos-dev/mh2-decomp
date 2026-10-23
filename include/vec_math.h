#ifndef VEC_MATH_H
#define VEC_MATH_H

#include "common.h"

/*
 * Vector maths: 16-bit angles in radians, the angles of directions, rotation
 * matrices (from angles, a direction, or an axis and an angle), the vectors'
 * sums, differences, scales and blends, and the 2D triangle tests.
 */

f32 Math_AngleToRad(s32 angle);
void Vec_AnglesToRad(s32 *angles, f32 *out);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *mtx);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *mtx);
f32 *Mtx_RotateVec(f32 *mtx, f32 *vec, f32 *out);
u16 Math_Heading(f32 targetX, f32 targetZ, f32 originX, f32 originZ);
u16 Vec_Heading(f32 *target, f32 *origin);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
u16 Mtx_Heading(f32 *mtx);
void Mtx_Billboard(f32 *mtx, f32 *pos);
void Mtx_BillboardScaled(f32 *mtx, f32 *pos);
void Vec_Resize(f32 *out, f32 len, f32 *vec);
void Mtx_FromDirection(f32 *dir, f32 *mtx, u8 axis);
void Vec_Set(f32 *vec, f32 x, f32 y, f32 z);
void Vec_Sub(f32 *out, f32 *lhs, f32 *rhs);
void Vec_Add(f32 *out, f32 *lhs, f32 *rhs);
void Vec_Diff(f32 *out, f32 *lhs, f32 *rhs);
void Vec_Scale(f32 *out, f32 *vec, f32 scale);
s32 Tri_Normal(f32 *cornerA, f32 *cornerB, f32 *cornerC, f32 *out);
s32 Tri_Contains2D(f32 *tri, f32 *point);
void Vec_SnapSmall(f32 *dst, f32 *src);
void Mtx_TurnX(f32 *mtx, f32 angle);
void Mtx_TurnY(f32 *mtx, f32 angle);
void Mtx_TurnZ(f32 *mtx, f32 angle);
u16 Vec_Elevation(f32 *from, f32 *to);
void Vec_Lerp(f32 *out, f32 *first, f32 *second, f32 weight);
f32 Math_CosAngle(s32 angle);
s32 Math_Atan2Angle(f32 y, f32 x);
void Vec_Transform(f32 *out, f32 *vec, f32 *mtx);
void Mtx_FromAxisAngle(f32 *mtx, f32 *axis, f32 angle);
f32 Vec_DistanceXZ(f32 *posA, f32 *posB);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);

#endif
