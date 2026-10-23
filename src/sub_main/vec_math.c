/*
 * Vector maths: 16-bit angles (ANGLE_TURN to a turn) in radians, the
 * angles of directions (headings, elevations, atan2), rotation matrices
 * (from angles, a direction, or an axis and an angle; the turns about one
 * axis; billboards), the vectors' sums, differences, scales and blends,
 * and the 2D triangle tests (Tri_Contains2D). It uses no data of its own.
 */

#include "common.h"
#include "model.h"
#include "vec_math.h"
#include "engine_math.h"

/* A 16-bit angle (ANGLE_TURN to a turn) in radians. */
f32 Math_AngleToRad(s32 angle)
{
    return ANGLE16_TO_RAD * (f32) (angle & 0xFFFF);
}

/* Three integer angles to floats (Player.rotX). */
void Vec_AnglesToRad(s32 *angles, f32 *out)
{
    out[0] = Math_AngleToRad(angles[0]);
    out[1] = Math_AngleToRad(angles[1]);
    out[2] = Math_AngleToRad(angles[2]);
}

/* A rotation matrix from three integer angles (X, Y, Z order). */
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *mtx)
{
    f32 rad[3];

    Vec_AnglesToRad(angles, rad);
    Mtx_Identity(mtx);
    Mtx_SetRotXYZ(mtx, rad[0], rad[1], rad[2]);
    return mtx;
}


/* A rotation matrix from three integer angles (Y, X, Z order). */
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *mtx)
{
    f32 rad[3];

    Vec_AnglesToRad(angles, rad);
    Mtx_Identity(mtx);
    Mtx_TurnY(mtx, rad[1]);
    Mtx_TurnX(mtx, rad[0]);
    Mtx_TurnZ(mtx, rad[2]);
    return mtx;
}

/* out = vec * mtx (the rotation part); returns out. */
f32 *Mtx_RotateVec(f32 *mtx, f32 *vec, f32 *out)
{
    Vec_Rotate(out, vec, mtx);
    return out;
}

/* The 16-bit angle of the direction from (originX, originZ) to (targetX, targetZ). */
u16 Math_Heading(f32 targetX, f32 targetZ, f32 originX, f32 originZ)
{
    f32 dir[3];

    dir[1] = 0.0f;
    dir[0] = targetX - originX;
    dir[2] = targetZ - originZ;
    Vec_Normalize(dir);
    return (s32)(0.5f + 65536.0f * Math_Atan2(-dir[2], dir[0]) / 6.2831855f);
}

/* The 16-bit angle of the direction from origin to target (x, z). */
u16 Vec_Heading(f32 *target, f32 *origin)
{
    f32 dir[3];

    dir[0] = target[0] - origin[0];
    dir[1] = 0.0f;
    dir[2] = target[2] - origin[2];
    Vec_Normalize(dir);
    return (s32)(0.5f + 65536.0f * Math_Atan2(-dir[2], dir[0]) / 6.2831855f);
}

/* The 16-bit angles of a direction: *yaw about y, *pitch about x. */
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw)
{
    *yaw = (s32)(0.5f + 65536.0f * Math_Atan2(dir[0], dir[2]) / 6.2831855f);
    *pitch = (s32)(0.5f + 65536.0f * -Math_Atan2(dir[1], Math_Sqrtf(dir[0] * dir[0] + dir[2] * dir[2])) / 6.2831855f);
}

/* The 16-bit heading of matrix mtx (its z axis in the x-z plane). */
u16 Mtx_Heading(f32 *mtx)
{
    f32 axisZ[4];
    f32 out[4];

    axisZ[2] = 1.0f;
    axisZ[3] = 1.0f;
    axisZ[0] = 0.0f;
    axisZ[1] = 0.0f;
    Vec4_Transform(out, axisZ, mtx);
    return (s32)(0.5f + 65536.0f * Math_Atan2(-(out[2] - mtx[14]), out[0] - mtx[12]) / 6.2831855f);
}

extern f32 gCamPos[];

/* Turns the x and y axes of mtx about its z axis so that its x axis is square to the direction from pos to the point gCamPos (a billboard). */
void Mtx_Billboard(f32 *mtx, f32 *pos)
{
    f32 toCam[3];
    f32 z[3];
    f32 x[3];
    f32 y[3];

    Vec_Sub(toCam, gCamPos, pos);
    Vec_Copy(z, mtx + 8);
    Vec_Cross(x, toCam, z);
    Vec_Normalize(x);
    Vec_Cross(y, z, x);
    if (!(Vec_Dot(y, y) <= 0.0001f)) {
        Vec_Cross(x, y, z);
        Vec_Copy(mtx, x);
        Vec_Copy(mtx + 4, y);
    }
}

/* Mtx_Billboard keeping the scale of each axis of mtx. */
void Mtx_BillboardScaled(f32 *mtx, f32 *pos)
{
    f32 scaleX = Vec_Length(mtx);
    f32 scaleY = Vec_Length(mtx + 4);
    f32 scaleZ = Vec_Length(mtx + 8);

    Vec_Normalize(mtx + 8);
    Mtx_Billboard(mtx, pos);
    mtx[0] *= scaleX;
    mtx[1] *= scaleX;
    mtx[2] *= scaleX;
    mtx[4] *= scaleY;
    mtx[5] *= scaleY;
    mtx[6] *= scaleY;
    mtx[8] *= scaleZ;
    mtx[9] *= scaleZ;
    mtx[10] *= scaleZ;
}

/* out = vec normalized, times len. */
void Vec_Resize(f32 *out, f32 len, f32 *vec)
{
    Vec_Copy(out, vec);
    Vec_Normalize(out);
    out[0] *= len;
    out[1] *= len;
    out[2] *= len;
}


/* A rotation matrix whose axis `axis` (0 x, 1 y, 2 z) is the direction dir
 * (normalized in place); the other two axes are made from world z (or x/y,
 * when dir is too close to it). */
void Mtx_FromDirection(f32 *dir, f32 *mtx, u8 axis)
{
    f32 x[4];
    f32 y[4];
    f32 z[4];
    f32 refAxis[3];

    Vec_Normalize(dir);
    switch (axis) {
    case 0:
        refAxis[0] = 0.0f;
        refAxis[1] = 0.0f;
        refAxis[2] = 1.0f;
        Vec_Cross(y, refAxis, dir);
        if (Vec_Dot(y, y) < 0.001f) {
            refAxis[2] = 0.0f;
            refAxis[0] = 1.0f;
            Vec_Cross(y, refAxis, dir);
        }
        Vec_Normalize(y);
        Vec_Copy(x, dir);
        Vec_Cross(z, x, y);
        break;
    case 1:
        refAxis[0] = 0.0f;
        refAxis[1] = 0.0f;
        refAxis[2] = 1.0f;
        Vec_Cross(x, dir, refAxis);
        if (Vec_Dot(x, x) < 0.001f) {
            refAxis[2] = 0.0f;
            refAxis[1] = 1.0f;
            Vec_Cross(x, refAxis, dir);
        }
        Vec_Normalize(x);
        Vec_Copy(y, dir);
        Vec_Cross(z, x, y);
        break;
    case 2:
        refAxis[0] = 0.0f;
        refAxis[1] = 1.0f;
        refAxis[2] = 0.0f;
        Vec_Cross(x, refAxis, dir);
        if (Vec_Dot(x, x) < 0.001f) {
            refAxis[2] = 1.0f;
            refAxis[1] = 0.0f;
            Vec_Cross(x, dir, refAxis);
        }
        Vec_Normalize(x);
        Vec_Copy(z, dir);
        Vec_Cross(y, z, x);
        break;
    }
    Mtx_Identity(mtx);
    Vec_Copy(mtx, x);
    Vec_Copy(mtx + 4, y);
    Vec_Copy(mtx + 8, z);
}

/* vec = (x, y, z). */
void Vec_Set(f32 *vec, f32 x, f32 y, f32 z)
{
    vec[0] = x;
    vec[1] = y;
    vec[2] = z;
}

/* out = lhs - rhs. */
void Vec_Sub(f32 *out, f32 *lhs, f32 *rhs)
{
    out[0] = lhs[0] - rhs[0];
    out[1] = lhs[1] - rhs[1];
    out[2] = lhs[2] - rhs[2];
}

/* out = lhs + rhs. */
void Vec_Add(f32 *out, f32 *lhs, f32 *rhs)
{
    out[0] = lhs[0] + rhs[0];
    out[1] = lhs[1] + rhs[1];
    out[2] = lhs[2] + rhs[2];
}

/* out = lhs - rhs (as Vec_Sub). */
void Vec_Diff(f32 *out, f32 *lhs, f32 *rhs)
{
    out[0] = lhs[0] - rhs[0];
    out[1] = lhs[1] - rhs[1];
    out[2] = lhs[2] - rhs[2];
}

/* out = vec * scale. */
void Vec_Scale(f32 *out, f32 *vec, f32 scale)
{
    out[0] = vec[0] * scale;
    out[1] = vec[1] * scale;
    out[2] = vec[2] * scale;
}



/* out = the unit normal of the triangle cornerA, cornerB, cornerC; returns
 * 1, or 0 when the triangle is degenerate. */
s32 Tri_Normal(f32 *cornerA, f32 *cornerB, f32 *cornerC, f32 *out)
{
    f32 edgeBA[3];
    f32 edgeBC[3];

    Vec_Sub(edgeBA, cornerA, cornerB);
    Vec_Sub(edgeBC, cornerC, cornerB);
    Vec_Cross(out, edgeBC, edgeBA);
    Vec_Normalize(out);
    if (Vec_Length(out) == 0.0f) {
        return 0;
    }
    return 1;
}

/* The 2D cross product (cornerB - cornerA) x (cornerC - cornerA) in x-y;
 * products one float step apart (their bits compared) count as equal. */
f32 Tri_Cross2D(f32 *cornerA, f32 *cornerB, f32 *cornerC)
{
    f32 products[2];
    s32 bitsA;
    s32 bitsB;

    products[0] = (cornerB[0] - cornerA[0]) * (cornerC[1] - cornerA[1]);
    products[1] = (cornerC[0] - cornerA[0]) * (cornerB[1] - cornerA[1]);
    bitsB = *(s32 *)&products[1];
    bitsA = *(s32 *)&products[0];
    if (bitsA == bitsB + 1) {
        products[0] = products[1];
    }
    if (bitsA == bitsB - 1) {
        products[0] = products[1];
    }
    return products[0] - products[1];
}

/* Whether point is inside the triangle cornerA, cornerB, cornerC (by Tri_Cross2D): 1 when the
 * triangle's winding is negative, 2 when positive, 0 outside or when the
 * triangle is degenerate. */
u8 Tri_Side2D(f32 *cornerA, f32 *cornerB, f32 *cornerC, f32 *point)
{
    f32 winding = Tri_Cross2D(cornerA, cornerB, cornerC);

    if (winding == 0.0f) {
        return 0;
    }
    if (winding < 0.0f) {
        if (!(Tri_Cross2D(cornerA, cornerB, point) <= 0.0f)) {
            return 0;
        }
        if (!(Tri_Cross2D(cornerB, cornerC, point) <= 0.0f)) {
            return 0;
        }
        return !(Tri_Cross2D(cornerC, cornerA, point) > 0.0f) ? 1 : 0;
    }
    if (Tri_Cross2D(cornerA, cornerB, point) < 0.0f) {
        return 0;
    }
    if (Tri_Cross2D(cornerB, cornerC, point) < 0.0f) {
        return 0;
    }
    if (Tri_Cross2D(cornerC, cornerA, point) < 0.0f) {
        return 0;
    }
    return 2;
}

/* Whether point lies inside the triangle tri (three f32[3] corners), by the
 * side Tri_Side2D reports and the winding (Tri_Cross2D). */
s32 Tri_Contains2D(f32 *tri, f32 *point)
{
    u8 side;
    s8 sum;
    s32 clockwise;

    clockwise = (s32)Tri_Cross2D(tri, tri + 3, tri + 6) <= 0;
    sum = 0;
    side = Tri_Side2D(tri, tri + 3, tri + 6, point);
    if (side != 0) {
        if (clockwise) {
            if (side == 1) {
                sum++;
            } else {
                sum--;
            }
        } else if (side == 2) {
            sum++;
        } else {
            sum--;
        }
    }
    return sum > 0;
}

/* dst = src with the components below 0.001 in size made 0. */
void Vec_SnapSmall(f32 *dst, f32 *src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    if (Math_Absf(src[0]) < 0.001f) {
        dst[0] = 0.0f;
    }
    if (Math_Absf(src[1]) < 0.001f) {
        dst[1] = 0.0f;
    }
    if (Math_Absf(src[2]) < 0.001f) {
        dst[2] = 0.0f;
    }
}

/* mtx = the rotation by angle (radians) about x times mtx (3x3). */
void Mtx_TurnX(f32 *mtx, f32 angle)
{
    f32 rot[16];

    Mtx_Identity(rot);
    Mtx_RotateX(rot, angle);
    Mtx_Mul3x3(mtx, rot, mtx);
}

/* mtx = the rotation by angle (radians) about y times mtx (3x3). */
void Mtx_TurnY(f32 *mtx, f32 angle)
{
    f32 rot[16];

    Mtx_Identity(rot);
    Mtx_RotateY(rot, angle);
    Mtx_Mul3x3(mtx, rot, mtx);
}

/* mtx = the rotation by angle (radians) about z times mtx (3x3). */
void Mtx_TurnZ(f32 *mtx, f32 angle)
{
    f32 rot[16];

    Mtx_Identity(rot);
    Mtx_RotateZ(rot, angle);
    Mtx_Mul3x3(mtx, rot, mtx);
}


/* The 16-bit elevation angle of the direction from `from` to `to`. */
u16 Vec_Elevation(f32 *from, f32 *to)
{
    f32 dx = to[0] - from[0];
    f32 dy = to[1] - from[1];
    f32 dz = to[2] - from[2];

    return (s32)(0.5f + 65536.0f * Math_Atan2(dy, Math_Sqrtf(dx * dx + dz * dz)) / 6.2831855f);
}

/* out = first * weight + second * (1 - weight). */
void Vec_Lerp(f32 *out, f32 *first, f32 *second, f32 weight)
{
    f32 rest = 1.0f - weight;

    out[0] = first[0] * weight + second[0] * rest;
    out[1] = first[1] * weight + second[1] * rest;
    out[2] = first[2] * weight + second[2] * rest;
}

/* The cosine of a 16-bit angle (ANGLE_TURN to a turn). */
f32 Math_CosAngle(s32 angle)
{
    return Math_Cos(3.1415927f * angle / 32768.0f);
}

/* atan2 as a 16-bit angle (ANGLE_TURN to a turn). */
s32 Math_Atan2Angle(f32 y, f32 x)
{
    return RAD_TO_ANGLE16 * Math_Atan2(y, x);
}

/* out = vec * mtx plus the translation of mtx. */
void Vec_Transform(f32 *out, f32 *vec, f32 *mtx)
{
    Vec_Rotate(out, vec, mtx);
    out[0] = out[0] + mtx[12];
    out[1] = out[1] + mtx[13];
    out[2] = out[2] + mtx[14];
}

/* mtx = the rotation by angle about axis (through a quaternion). */
void Mtx_FromAxisAngle(f32 *mtx, f32 *axis, f32 angle)
{
    f32 quat[3];

    Quat_Set((f32 *) axis, quat, angle);
    Quat_ToMatrix(quat, (void *) mtx);
}


/* The distance between posA and posB in the x-z plane. */
f32 Vec_DistanceXZ(f32 *posA, f32 *posB)
{
    f32 delta[3];

    delta[0] = posA[0] - posB[0];
    delta[1] = 0.0f;
    delta[2] = posA[2] - posB[2];
    return Vec_Length(delta);
}

/* Turns the 16-bit angle *cur toward *target by at most step (the shorter way); returns the step taken. */
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step)
{
    s32 delta;
    s32 dist;

    *target = (u16)*target;
    delta = (u16)(*target - *cur);
    dist = (delta >= 0x8000) ? (u16)-delta : delta;
    if (dist < step) {
        step = dist;
    }
    if (delta >= 0x8000) {
        *cur -= step;
    } else {
        *cur += step;
    }
    return step;
}
