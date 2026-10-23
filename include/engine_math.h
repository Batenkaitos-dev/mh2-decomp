#ifndef ENGINE_MATH_H
#define ENGINE_MATH_H

#include "common.h"
#include "model.h"

/*
 * The engine's maths: sine and cosine on VU0, the camera's projection and
 * viewport matrices (Camera_Set*), the 4x4 matrix operations (Mtx_*), the
 * vectors (Vec_*, Vec4_*), the angle conversions, the matrix slots
 * (MtxSlot_Store, MtxSlot_Load), a point's projection to the screen
 * (Camera_ProjectPoint), the C library's maths for floats (Math_*), the
 * quaternions (Quat_*) and the view volume's sphere test
 * (Frustum_TestSphere).
 */

f32 Math_SinVu0(f32 x);
f32 Math_CosVu0(f32 x);
void Camera_SetPerspective(s32 slot, f32 farZ, f32 nearZ, f32 fov, f32 aspect);
void Mtx_Perspective(f32 *mtx, f32 farZ, f32 nearZ, f32 fov, f32 aspect);
void Camera_StoreProjection(f32 *proj);
void Camera_SetOrtho(s32 slot, s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ);
void Camera_SetViewport(f32 *screen, s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ);
void Camera_SetViewportMtx(s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ);
void Mtx_Identity(f32 *mtx);
void Mtx_ClearRotation(f32 *mtx);
void Mtx_Scale(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_SetScale(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_Translation(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_ScaleBy(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_AddTrans(f32 *mtx, f32 *offset);
void Mtx_GetTrans(f32 *out, f32 *mtx);
void Mtx_RotateX(f32 *mtx, f32 angle);
void Mtx_RotateY(f32 *mtx, f32 angle);
void Mtx_RotateZ(f32 *mtx, f32 angle);
void Mtx_SetRotXYZ(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_SetRotZYX(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_RotateXYZ(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_RotateXZY(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_RotateYXZ(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_RotateZXY(f32 *mtx, f32 x, f32 y, f32 z);
void Mtx_LookAt(f32 *mtx, f32 *eye, f32 *target, f32 *up);
void Mtx_Transpose3x3(f32 *mtx);
void Mtx_Mul(f32 *dst, f32 *lhs, f32 *rhs);
void Mtx_Concat(f32 *mtx, f32 *other);
void Mtx_Mul3x3(f32 *dst, f32 *lhs, f32 *rhs);
void Mtx_Concat3x3(f32 *mtx, f32 *other);
void Mtx_Inverse(f32 *dst, f32 *src);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_CopyRot(f32 *mtx, f32 *src);
void Mtx_Blend(f32 *dst, f32 *first, f32 *second, f32 weightFirst, f32 weightSecond);
void Vec_Rotate(f32 *out, f32 *vec, f32 *mtx);
void Vec_RotateSelf(f32 *vec, f32 *mtx);
void Vec4_Transform(f32 *out, f32 *vec, f32 *mtx);
void Vec4_TransformSelf(f32 *vec, f32 *mtx);
void Vec_Project(f32 *out, f32 *vec, f32 *mtx);
void Vec_RotateX(f32 *vec, f32 angle);
void Vec_RotateY(f32 *vec, f32 angle);
void Vec_RotateZ(f32 *vec, f32 angle);
f32 Vec_Length(f32 *vec);
f32 Vec_Distance(f32 *from, f32 *to);
void Vec_Normalize(f32 *vec);
f32 Vec_Dot(f32 *lhs, f32 *rhs);
void Vec_Cross(f32 *out, f32 *lhs, f32 *rhs);
void Vec_Copy(f32 *dst, f32 *src);
u32 Math_RadToAngle(f32 rad);
f32 Math_Angle16ToRad(u16 angle);
void MtxSlot_Store(s32 slot, MtxSlot *mtx);
void MtxSlot_Load(MtxSlot *mtx, s32 slot);
void Camera_ProjectPoint(f32 *out, f32 *point);
f32 Math_Absf(f32 x);
f32 Math_Acos(f32 x); /* acos */
f32 Math_Asin(f32 x);
f32 Math_Atan(f32 x);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Ceil(f32 x);
f32 Math_Exp(f32 x); /* expf */
f32 Math_Sin(f32 x);
f32 Math_Cos(f32 x);
void Math_GetSinCos(f32 *sinOut, f32 *cosOut, f32 angle);
f32 Math_Floor(f32 x);
f32 Math_Pow(f32 x, f32 y);
f32 Math_Sqrtf(f32 x);
void Quat_ToMatrix(f32 *quat, f32 (*mtx)[4]);
void Quat_Set(f32 *axis, f32 *quat, f32 angle);
s32 Frustum_TestSphere(f32 *center, f32 *out, f32 *mtx, f32 *planes, f32 radius);

#endif
