/*
 * The engine's maths: sine and cosine on VU0 (Math_SinVu0, Math_CosVu0,
 * Math_SinCos), the camera's projection and viewport matrices
 * (Camera_SetPerspective, Camera_SetOrtho, Camera_SetViewport), the 4x4
 * matrix operations (Mtx_*), the vectors (Vec_*, Vec4_*), the angle
 * conversions, the matrix slots (MtxSlot_Store, MtxSlot_Load), a point's
 * projection to the screen, the C library's maths for floats (Math_*),
 * the quaternions (Quat_*) and the view volume's sphere test
 * (Frustum_TestSphere).
 */

#include "common.h"
#include "syswork.h"
#include "model.h"
#include "engine_math.h"
#include "gs_regs.h"
#include "libm.h"

#pragma optimization_level 4

extern s32 gDispWidth; /* main: display width (divided by 640 for the font scale) */
extern s32 gSysFrameMode; /* main */

extern f32 gSinVu0Coefs[];
extern f32 gSinVu0Poly[];
/* The sqrt(1 - s * s) half of Math_SinCos (Math_Sin calls it). */
asm f32 Math_SinVu0(f32 x)
{
    .set noreorder
    /* Handwritten function */
    lui v0, (0xC0490FDB >> 16)
    ori v0, v0, (0xC0490FDB & 0xFFFF)
    mtc1 v0, f0
    nop
    c.le.s f12, f0
    bc1t L002EC308
    lui v0, (0x40490FDB >> 16)
    ori v0, v0, (0x40490FDB & 0xFFFF)
    mtc1 v0, f0
    nop
    c.lt.s f12, f0
    bc1t L002EC358
    nop
    nop
L002EC308:
    la v0, gSinVu0Coefs
    mfc1 t0, f12
    lqc2 vf6, 0x0(v0)
    qmtc2.ni t0, vf4
    slt t0, zero, t0
    ctc2.ni t0, vi2
    vmulx.x vf7, vf4, vf6x
    vftoi0.x vf7, vf7
    vmtir vi1, vf7x
    viaddi vi3, vi0, -0x2
    viadd vi1, vi1, vi2
    viand vi1, vi1, vi3
    vmfir.x vf7, vi1
    vitof0.x vf7, vf7
    vmuly.x vf7, vf7, vf6y
    vsub.x vf7, vf4, vf7
    qmfc2.ni t0, vf7
    mtc1 t0, f12
    nop
L002EC358:
    la v1, gSinVu0Poly
    mtc1 zero, f0
    nop
    c.lt.s f12, f0
    lui at, (0x3FC90FDB >> 16)
    ori v0, at, (0x3FC90FDB & 0xFFFF)
    bc1f L002EC390
    mtc1 v0, f0
    vnop
    add.s f12, f0, f12
    j L002EC398
    addi a3, zero, 0x1 /* handwritten instruction */
    vnop
L002EC390:
    sub.s f12, f0, f12
    addu a3, zero, zero
L002EC398:
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf6
    vmulx.x vf7, vf6, vf6x
    lqc2 vf5, 0x0(v1)
    vmulx.xyzw vf5, vf5, vf6x
    vmulaw.x ACC, vf6, vf0w
    vmulx.x vf8, vf7, vf7x
    vsub.zw vf6, vf0, vf0
    vmulx.x vf9, vf7, vf8x
    vmulx.x vf10, vf8, vf8x
    vmaddaw.x ACC, vf7, vf5w
    vmaddaz.x ACC, vf8, vf5z
    vmadday.x ACC, vf9, vf5y
    vmaddx.x vf6, vf10, vf5x
    vmul.x vf7, vf6, vf6
    vsubx.w vf7, vf0, vf7x
    vsqrt Q, vf7w
    bnez a3, L002EC3F8
    vwaitq
    vnop
    b L002EC400
    vaddq.x vf6, vf0, Q
    vnop
L002EC3F8:
    vsubq.x vf6, vf0, Q
    nop
L002EC400:
    qmfc2.ni t0, vf6
    mtc1 t0, f0
    jr ra
    nop
}
/* The polynomial half of Math_SinCos. */
asm f32 Math_CosVu0(f32 x)
{
    .set noreorder
    lui v0, (0xC0490FDB >> 16)
    ori v0, v0, (0xC0490FDB & 0xFFFF)
    mtc1 v0, f0
    nop
    c.le.s f12, f0
    bc1t L002EC448
    lui v0, (0x40490FDB >> 16)
    ori v0, v0, (0x40490FDB & 0xFFFF)
    mtc1 v0, f0
    nop
    c.lt.s f12, f0
    bc1t L002EC498
    nop
    nop
L002EC448:
    la v0, gSinVu0Coefs
    mfc1 t0, f12
    lqc2 vf6, 0x0(v0)
    qmtc2.ni t0, vf4
    slt t0, zero, t0
    ctc2.ni t0, vi2
    vmulx.x vf7, vf4, vf6x
    vftoi0.x vf7, vf7
    vmtir vi1, vf7x
    viaddi vi3, vi0, -0x2
    viadd vi1, vi1, vi2
    viand vi1, vi1, vi3
    vmfir.x vf7, vi1
    vitof0.x vf7, vf7
    vmuly.x vf7, vf7, vf6y
    vsub.x vf7, vf4, vf7
    qmfc2.ni t0, vf7
    mtc1 t0, f12
    nop
L002EC498:
    la v1, gSinVu0Poly
    mtc1 zero, f0
    nop
    c.lt.s f12, f0
    lui at, (0x3FC90FDB >> 16)
    ori v0, at, (0x3FC90FDB & 0xFFFF)
    bc1f L002EC4D0
    mtc1 v0, f0
    vnop
    j L002EC4D8
    add.s f12, f0, f12
    vnop
    nop
L002EC4D0:
    sub.s f12, f0, f12
    nop
L002EC4D8:
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf6
    vmulx.x vf7, vf6, vf6x
    lqc2 vf5, 0x0(v1)
    vmulx.xyzw vf5, vf5, vf6x
    vmulaw.x ACC, vf6, vf0w
    vmulx.x vf8, vf7, vf7x
    vsub.yzw vf6, vf0, vf0
    vmulx.x vf9, vf7, vf8x
    vmulx.x vf10, vf8, vf8x
    vmaddaw.x ACC, vf7, vf5w
    vmaddaz.x ACC, vf8, vf5z
    vmadday.x ACC, vf9, vf5y
    vmaddx.x vf6, vf10, vf5x
    vnop
    qmfc2.ni t1, vf6
    mtc1 t1, f0
    jr ra
    nop
}
/* out[0] = sin a, out[1] = cos a (VU0 polynomial). Static: Math_GetSinCos
 * keeps values in t1 and a2 across the call, which MWCC only does for a
 * static callee it has compiled. */
static asm void Math_SinCos(f32 *out, f32 angle)
{
    .set noreorder
    /* Handwritten function */
    lui v1, (0xC0490FDB >> 16)
    ori v1, v1, (0xC0490FDB & 0xFFFF)
    mtc1 v1, f0
    nop
    c.le.s f12, f0
    bc1t L002EC568
    lui v1, (0x40490FDB >> 16)
    ori v1, v1, (0x40490FDB & 0xFFFF)
    mtc1 v1, f0
    nop
    c.lt.s f12, f0
    bc1t L002EC5B8
    nop
    nop
L002EC568:
    la v1, gSinVu0Coefs
    mfc1 t0, f12
    lqc2 vf6, 0x0(v1)
    qmtc2.ni t0, vf4
    slt t0, zero, t0
    ctc2.ni t0, vi2
    vmulx.x vf7, vf4, vf6x
    vftoi0.x vf7, vf7
    vmtir vi1, vf7x
    viaddi vi3, vi0, -0x2
    viadd vi1, vi1, vi2
    viand vi1, vi1, vi3
    vmfir.x vf7, vi1
    vitof0.x vf7, vf7
    vmuly.x vf7, vf7, vf6y
    vsub.x vf7, vf4, vf7
    qmfc2.ni t0, vf7
    mtc1 t0, f12
    nop
L002EC5B8:
    la a1, gSinVu0Poly
    mtc1 zero, f0
    nop
    c.lt.s f12, f0
    lui at, (0x3FC90FDB >> 16)
    ori v1, at, (0x3FC90FDB & 0xFFFF)
    bc1f L002EC5F0
    mtc1 v1, f0
    vnop
    add.s f12, f0, f12
    j L002EC5F8
    addi a3, zero, 0x1 /* handwritten instruction */
    vnop
L002EC5F0:
    sub.s f12, f0, f12
    addu a3, zero, zero
L002EC5F8:
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf6
    vmulx.x vf7, vf6, vf6x
    lqc2 vf5, 0x0(a1)
    vmulx.xyzw vf5, vf5, vf6x
    vmulaw.x ACC, vf6, vf0w
    vmulx.x vf8, vf7, vf7x
    vsub.zw vf6, vf0, vf0
    vmulx.x vf9, vf7, vf8x
    vmulx.x vf10, vf8, vf8x
    vmaddaw.x ACC, vf7, vf5w
    vmaddaz.x ACC, vf8, vf5z
    vmadday.x ACC, vf9, vf5y
    vmaddx.x vf6, vf10, vf5x
    vmul.x vf7, vf6, vf6
    vaddx.y vf6, vf0, vf6x
    vsubx.w vf7, vf0, vf7x
    vsqrt Q, vf7w
    bnez a3, L002EC660
    vwaitq
    vnop
    b L002EC668
    vaddq.x vf6, vf0, Q
    vnop
    nop
L002EC660:
    vsubq.x vf6, vf0, Q
    nop
L002EC668:
    jr ra
    sqc2 vf6, 0x0(a0)
}

extern MtxSlot gMtxSlots[];

/* The camera's matrices. */
extern f32 gMtxProj[16];
extern f32 gViewportMtx[16];
extern f32 gMtxProjClip[16];
extern f32 gMtxProjScreen[16];
extern f32 gMtxScreen[16];
extern f32 gMtxCamProj[16];
extern f32 gMtxClip[16];

/*
 * main's render state (the viewport), named in main.txt
 * (gSysFrameMode and gDispWidth are declared at the top of the file).
 */
extern f32 gViewClipH;
extern f32 gViewClipW;
extern f32 gViewCenterY;
extern f32 gViewCenterX;
extern f32 gViewH;
extern f32 gViewW;
extern f32 gViewY;
extern f32 gViewX;
extern f32 gSysZMax;
extern s32 gDispHeight;

/* Matrix slot `slot` = a perspective projection (Mtx_Perspective, Camera_StoreProjection), then
 * the camera matrices. */
void Camera_SetPerspective(s32 slot, f32 farZ, f32 nearZ, f32 fov, f32 aspect)
{
    f32 *mtx = gMtxSlots[slot].m[0];

    Mtx_Perspective(mtx, farZ, nearZ, fov, aspect);
    Camera_StoreProjection(mtx);
    Mtx_Mul(gMtxProjClip, gMtxCamProj, gMtxClip);
    Mtx_Mul(gMtxProjScreen, gMtxCamProj, gMtxScreen);
}

/* mtx = a perspective projection: depth nearZ to farZ, vertical field of view fov (radians),
 * aspect ratio aspect. The far depth comes first: the callers pass Camera.farZ there, and
 * the depth terms are only right that way round. */
void Mtx_Perspective(f32 *mtx, f32 farZ, f32 nearZ, f32 fov, f32 aspect)
{
    f32 cosHalf;
    f32 sinHalf;

    fov *= 0.5f;
    Math_GetSinCos(&sinHalf, &cosHalf, fov);
    Mtx_Identity(mtx);
    mtx[0] = cosHalf / sinHalf / aspect;
    mtx[5] = cosHalf / sinHalf;
    mtx[10] = -(farZ + nearZ) / (farZ - nearZ);
    mtx[11] = -1.0f;
    mtx[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);
    mtx[15] = 0.0f;
}

/* gViewportMtx = proj (a projection) with its x and y scale fitted to the viewport.
 * This function and the two viewport ones below read main's render-state globals by
 * name: as raw addresses, MWCC can't move their loads above the matrix stores. */
void Camera_StoreProjection(f32 *proj)
{
    Mtx_Copy(gViewportMtx, proj);
    gViewportMtx[0] = 2.0f * gViewportMtx[0] / (gViewClipW / (0.5f * gViewW));
    gViewportMtx[5] = 2.0f * gViewportMtx[5] / (gViewClipH / (0.5f * gViewH));
}

/* Matrix slot `slot` = an orthographic screen matrix for the viewport (x, y, width, height),
 * then the camera matrices and the GS scissor (y and height halved while gSysFrameMode is
 * 0: a field, half the lines). */
void Camera_SetOrtho(s32 slot, s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ)
{
    Camera_SetViewport(gMtxSlots[slot].m[0], x, y, width, height, nearZ, farZ);
    Camera_StoreProjection(gMtxCamProj);
    Mtx_Mul(gMtxProjClip, gMtxCamProj, gMtxClip);
    Mtx_Mul(gMtxProjScreen, gMtxCamProj, gMtxScreen);
    if (gSysFrameMode == FRAME_MODE_FIELD) {
        y /= 2;
        height /= 2;
    }
    Gs_SetScissor(x, y, width, height, 2);
}

/*
 * The viewport (x, y, width, height; depth nearZ to farZ) into gMtxScreen and the screen
 * matrix `screen` (centred on the GS's 2048, 2048), and the viewport globals of main;
 * then Camera_SetViewportMtx.
 */
void Camera_SetViewport(f32 *screen, s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ)
{
    f32 halfW;
    f32 halfH;
    f32 depth;

    halfW = width / 2;
    halfH = height / 2;
    Mtx_Identity(gMtxScreen);
    gMtxScreen[0] = halfW;
    gMtxScreen[5] = -halfH;
    depth = farZ - nearZ;
    gMtxScreen[10] = depth;
    gMtxScreen[12] = x + halfW;
    gMtxScreen[13] = y + halfH;
    gMtxScreen[14] = nearZ;
    gMtxScreen[15] = 1.0f;
    if (gSysFrameMode == FRAME_MODE_FIELD) {
        y /= 2;
        height /= 2;
    }
    halfW = width / 2;
    halfH = height / 2;
    Mtx_Identity(screen);
    screen[0] = halfW;
    screen[5] = -halfH;
    screen[10] = depth;
    screen[12] = x + halfW;
    screen[13] = y + halfH;
    screen[15] = 1.0f;
    screen[10] *= 0.5f * -gSysZMax;
    screen[12] += 2048 - (gDispWidth >> 1);
    screen[13] += 2048 - (gDispHeight >> 1);
    screen[14] = 0.5f * gSysZMax;
    gViewCenterX = screen[12];
    gViewCenterY = screen[13];
    gViewX = x;
    gViewY = y;
    gViewW = width;
    gViewH = height;
    gViewClipW = 2048.0f - gViewCenterX;
    if (gViewClipW < 0.0f) {
        gViewClipW *= -1.0f;
    }
    gViewClipW = 2048.0f - gViewClipW;
    gViewClipH = 2048.0f - gViewCenterY;
    if (gViewClipH < 0.0f) {
        gViewClipH *= -1.0f;
    }
    gViewClipH = 2048.0f - gViewClipH;
    Camera_SetViewportMtx(0, 0, gViewClipW, gViewClipH, nearZ, farZ);
}

/* gMtxProj = the viewport matrix for a width x height screen (depth nearZ to farZ). */
void Camera_SetViewportMtx(s32 x, s32 y, s32 width, s32 height, f32 nearZ, f32 farZ)
{
    f32 halfW;
    f32 halfH;

    Mtx_Identity(gMtxProj);
    halfW = width / 2;
    halfH = height / 2;
    gMtxProj[0] = halfW;
    gMtxProj[5] = -halfH;
    gMtxProj[15] = 1.0f;
    gMtxProj[10] = farZ - nearZ;
    gMtxProj[10] *= 0.5f * -gSysZMax;
    gMtxProj[14] = 0.5f * gSysZMax;
    gMtxProj[12] = gViewCenterX;
    gMtxProj[13] = gViewCenterY;
}
extern u8 gMtxIdentity[];
/* mtx = the identity matrix (copied from gMtxIdentity). */
asm void Mtx_Identity(f32 *mtx)
{
    .set noreorder
    la v1, gMtxIdentity
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf7, 0x30(v1)
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    sqc2 vf7, 0x30(a0)
}
/* The 3x3 part of mtx becomes the identity; the translation stays. */
asm void Mtx_ClearRotation(f32 *mtx)
{
    .set noreorder
    la v1, gMtxIdentity
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    vadd.xyz vf8, vf0, vf4
    vadd.xyz vf9, vf0, vf5
    vadd.xyz vf10, vf0, vf6
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    jr ra
    sqc2 vf10, 0x20(a0)
}
/* mtx = a scale matrix. */
asm void Mtx_Scale(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    la v1, gMtxIdentity
    mfc1 a2, f12
    mfc1 a3, f13
    mfc1 t0, f14
    lqc2 vf4, 0x0(v1)
    qmtc2.ni a2, vf8
    qmtc2.ni a3, vf9
    qmtc2.ni t0, vf10
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf7, 0x30(v1)
    vaddx.x vf4, vf0, vf8x
    vaddx.y vf5, vf0, vf9x
    vaddx.z vf6, vf0, vf10x
    sqc2 vf7, 0x30(a0)
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    jr ra
    sqc2 vf6, 0x20(a0)
}
/* The 3x3 part of mtx becomes a scale; the translation stays. */
asm void Mtx_SetScale(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    la v1, gMtxIdentity
    mfc1 a2, f12
    mfc1 a3, f13
    mfc1 t0, f14
    lqc2 vf4, 0x0(v1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    qmtc2.ni t0, vf22
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    vaddx.x vf4, vf0, vf20x
    vaddx.y vf5, vf0, vf21x
    vaddx.z vf6, vf0, vf22x
    vaddx.w vf4, vf8, vf0x
    vaddx.w vf5, vf9, vf0x
    vaddx.w vf6, vf10, vf0x
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    jr ra
    sqc2 vf6, 0x20(a0)
}
/* mtx = a translation matrix. */
asm void Mtx_Translation(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    la v1, gMtxIdentity
    mfc1 a2, f12
    mfc1 a3, f13
    mfc1 t0, f14
    lqc2 vf7, 0x30(v1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    qmtc2.ni t0, vf22
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    vaddx.x vf7, vf0, vf20x
    vaddx.y vf7, vf0, vf21x
    vaddx.z vf7, vf0, vf22x
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    sqc2 vf7, 0x30(a0)
}

/* The 3x3 part of mtx = mtx * scale(x, y, z). */
void Mtx_ScaleBy(f32 *mtx, f32 x, f32 y, f32 z)
{
    f32 scale[16];

    Mtx_SetScale(scale, x, y, z);
    Mtx_Concat3x3(mtx, scale);
}

/* mtx's translation = (x, y, z). */
void Mtx_SetTrans(f32 *mtx, f32 x, f32 y, f32 z)
{
    mtx[12] = x;
    mtx[13] = y;
    mtx[14] = z;
}

/* mtx's translation += offset. */
void Mtx_AddTrans(f32 *mtx, f32 *offset)
{
    mtx[12] = (mtx[12] + offset[0]);
    mtx[13] = (mtx[13] + offset[1]);
    mtx[14] = (mtx[14] + offset[2]);
}

/* out = mtx's translation. */
void Mtx_GetTrans(f32 *out, f32 *mtx)
{
    out[0] = mtx[12];
    out[1] = mtx[13];
    out[2] = mtx[14];
}
/* mtx = mtx * (rotation by angle about x). */
asm void Mtx_RotateX(f32 *mtx, f32 angle)
{
    .set noreorder
    addiu sp, sp, -0x20
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x20
}
/* mtx = mtx * (rotation by angle about y). */
asm void Mtx_RotateY(f32 *mtx, f32 angle)
{
    .set noreorder
    addiu sp, sp, -0x20
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x20
}
/* mtx = mtx * (rotation by angle about z). */
asm void Mtx_RotateZ(f32 *mtx, f32 angle)
{
    .set noreorder
    addiu sp, sp, -0x20
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x20
}
/* The rotation part of mtx = rotations about x, then y, then z. */
asm void Mtx_SetRotXYZ(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x60
    sd ra, 0x20(sp)
    sq s0, 0x10(sp)
    swc1 f22, 0x8(sp)
    daddu s0, a0, zero
    swc1 f21, 0x4(sp)
    swc1 f20, 0x0(sp)
    mov.s f22, f12
    mov.s f21, f13
    jal Mtx_ClearRotation
    mov.s f20, f14
    mov.s f12, f22
    jal Math_SinCos
    addiu a0, sp, 0x50
    lwc1 f1, 0x50(sp)
    lwc1 f0, 0x54(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    mov.s f12, f21
    jal Math_SinCos
    addiu a0, sp, 0x40
    lwc1 f1, 0x40(sp)
    lwc1 f0, 0x44(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    mov.s f12, f20
    jal Math_SinCos
    addiu a0, sp, 0x30
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    ld ra, 0x20(sp)
    lwc1 f22, 0x8(sp)
    lq s0, 0x10(sp)
    lwc1 f21, 0x4(sp)
    lwc1 f20, 0x0(sp)
    jr ra
    addiu sp, sp, 0x60
}
/* The rotation part of mtx = rotations about z, then y, then x. */
asm void Mtx_SetRotZYX(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x60
    sd ra, 0x20(sp)
    sq s0, 0x10(sp)
    swc1 f22, 0x8(sp)
    daddu s0, a0, zero
    swc1 f21, 0x4(sp)
    swc1 f20, 0x0(sp)
    mov.s f22, f12
    mov.s f21, f13
    jal Mtx_ClearRotation
    mov.s f20, f14
    mov.s f12, f20
    jal Math_SinCos
    addiu a0, sp, 0x50
    lwc1 f1, 0x50(sp)
    lwc1 f0, 0x54(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    mov.s f12, f21
    jal Math_SinCos
    addiu a0, sp, 0x40
    lwc1 f1, 0x40(sp)
    lwc1 f0, 0x44(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    mov.s f12, f22
    jal Math_SinCos
    addiu a0, sp, 0x30
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(s0)
    lqc2 vf5, 0x10(s0)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(s0)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(s0)
    sqc2 vf5, 0x10(s0)
    sqc2 vf6, 0x20(s0)
    ld ra, 0x20(sp)
    lwc1 f22, 0x8(sp)
    lq s0, 0x10(sp)
    lwc1 f21, 0x4(sp)
    lwc1 f20, 0x0(sp)
    jr ra
    addiu sp, sp, 0x60
}
/* mtx rotated about x, then y, then z. */
asm void Mtx_RotateXYZ(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x40
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x30
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f13
    jal Math_SinCos
    addiu a0, sp, 0x20
    lwc1 f1, 0x20(sp)
    lwc1 f0, 0x24(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f14
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x40
}
/* mtx rotated about x, then z, then y. */
asm void Mtx_RotateXZY(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x40
    daddu t1, a0, zero
    sd ra, 0x0(sp)
    jal Math_SinCos
    addiu a0, sp, 0x30
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f14
    jal Math_SinCos
    addiu a0, sp, 0x20
    lwc1 f1, 0x20(sp)
    lwc1 f0, 0x24(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f13
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x40
}
/* mtx rotated about y, then x, then z. */
asm void Mtx_RotateYXZ(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x40
    daddu t1, a0, zero
    mov.s f2, f12
    sd ra, 0x0(sp)
    addiu a0, sp, 0x30
    jal Math_SinCos
    mov.s f12, f13
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f2
    jal Math_SinCos
    addiu a0, sp, 0x20
    lwc1 f1, 0x20(sp)
    lwc1 f0, 0x24(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f14
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x40
}
/* mtx rotated about z, then x, then y. */
asm void Mtx_RotateZXY(f32 *mtx, f32 x, f32 y, f32 z)
{
    .set noreorder
    addiu sp, sp, -0x40
    daddu t1, a0, zero
    mov.s f2, f12
    sd ra, 0x0(sp)
    addiu a0, sp, 0x30
    jal Math_SinCos
    mov.s f12, f14
    lwc1 f1, 0x30(sp)
    lwc1 f0, 0x34(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.z vf10, vf0, vf0
    vaddx.y vf10, vf0, vf20x
    vaddx.x vf10, vf0, vf21x
    vadd.z vf11, vf0, vf0
    vsubx.x vf11, vf0, vf20x
    vaddx.y vf11, vf0, vf21x
    vmr32.xyz vf12, vf0
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f2
    jal Math_SinCos
    addiu a0, sp, 0x20
    lwc1 f1, 0x20(sp)
    lwc1 f0, 0x24(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.yz vf10, vf0, vf0
    vaddw.x vf10, vf0, vf0w
    vadd.x vf11, vf0, vf0
    vaddx.y vf11, vf0, vf21x
    vaddx.z vf11, vf0, vf20x
    vadd.x vf12, vf0, vf0
    vsubx.y vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    mov.s f12, f13
    jal Math_SinCos
    addiu a0, sp, 0x10
    lwc1 f1, 0x10(sp)
    lwc1 f0, 0x14(sp)
    mfc1 a2, f1
    mfc1 a3, f0
    lqc2 vf4, 0x0(t1)
    lqc2 vf5, 0x10(t1)
    qmtc2.ni a2, vf20
    qmtc2.ni a3, vf21
    vadd.xz vf11, vf0, vf0
    vaddw.y vf11, vf0, vf0w
    vadd.y vf10, vf0, vf0
    vaddx.x vf10, vf0, vf21x
    vsubx.z vf10, vf0, vf20x
    vadd.y vf12, vf0, vf0
    vaddx.x vf12, vf0, vf20x
    vaddx.z vf12, vf0, vf21x
    lqc2 vf6, 0x20(t1)
    vmulax.xyz ACC, vf10, vf4x
    vmadday.xyz ACC, vf11, vf4y
    vmaddz.xyz vf4, vf12, vf4z
    vmulax.xyz ACC, vf10, vf5x
    vmadday.xyz ACC, vf11, vf5y
    vmaddz.xyz vf5, vf12, vf5z
    vmulax.xyz ACC, vf10, vf6x
    vmadday.xyz ACC, vf11, vf6y
    vmaddz.xyz vf6, vf12, vf6z
    sqc2 vf4, 0x0(t1)
    sqc2 vf5, 0x10(t1)
    sqc2 vf6, 0x20(t1)
    ld ra, 0x0(sp)
    jr ra
    addiu sp, sp, 0x40
}

typedef struct Vec3Copy {
    f32 x, y, z;
} Vec3Copy;

/* mtx = the view matrix of a camera at eye looking at target, with up as the up direction. */
void Mtx_LookAt(f32 *mtx, f32 *eye, f32 *target, f32 *up)
{
    f32 side[3];
    f32 upAxis[3];
    f32 back[3];
    Vec3Copy upDir;

    back[0] = eye[0] - target[0];
    back[1] = eye[1] - target[1];
    back[2] = eye[2] - target[2];
    upDir = *(Vec3Copy *)up;
    Vec_Normalize(back);
    Vec_Normalize((f32 *)&upDir);
    Vec_Cross(side, (f32 *)&upDir, back);
    Vec_Normalize(side);
    Vec_Cross(upAxis, back, side);
    mtx[3] = 0.0f;
    mtx[7] = 0.0f;
    mtx[11] = 0.0f;
    mtx[15] = 1.0f;
    mtx[0] = side[0];
    mtx[4] = side[1];
    mtx[8] = side[2];
    mtx[1] = upAxis[0];
    mtx[5] = upAxis[1];
    mtx[9] = upAxis[2];
    mtx[2] = back[0];
    mtx[6] = back[1];
    mtx[10] = back[2];
    mtx[12] = -Vec_Dot(eye, side);
    mtx[13] = -Vec_Dot(eye, upAxis);
    mtx[14] = -Vec_Dot(eye, back);
}
/* Transposes the 3x3 part of mtx. */
asm void Mtx_Transpose3x3(f32 *mtx)
{
    .set noreorder
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    vaddx.x vf4, vf0, vf8x
    vaddy.x vf5, vf0, vf8y
    vaddz.x vf6, vf0, vf8z
    vaddx.y vf4, vf0, vf9x
    vaddy.y vf5, vf0, vf9y
    vaddz.y vf6, vf0, vf9z
    vaddx.z vf4, vf0, vf10x
    vaddy.z vf5, vf0, vf10y
    vaddz.z vf6, vf0, vf10z
    vaddx.w vf4, vf8, vf0x
    vaddx.w vf5, vf9, vf0x
    vaddx.w vf6, vf10, vf0x
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    jr ra
    sqc2 vf6, 0x20(a0)
}
/* dst = lhs * rhs (4x4, row vectors). */
asm void Mtx_Mul(f32 *dst, f32 *lhs, f32 *rhs)
{
    .set noreorder
    lqc2 vf4, 0x0(a2)
    lqc2 vf5, 0x10(a2)
    lqc2 vf6, 0x20(a2)
    lqc2 vf7, 0x30(a2)
    lqc2 vf8, 0x0(a1)
    lqc2 vf9, 0x10(a1)
    lqc2 vf10, 0x20(a1)
    lqc2 vf11, 0x30(a1)
    vmulax.xyzw ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw vf8, vf7, vf8w
    vmulax.xyzw ACC, vf4, vf9x
    vmadday.xyzw ACC, vf5, vf9y
    vmaddaz.xyzw ACC, vf6, vf9z
    vmaddw.xyzw vf9, vf7, vf9w
    vmulax.xyzw ACC, vf4, vf10x
    vmadday.xyzw ACC, vf5, vf10y
    vmaddaz.xyzw ACC, vf6, vf10z
    vmaddw.xyzw vf10, vf7, vf10w
    vmulax.xyzw ACC, vf4, vf11x
    vmadday.xyzw ACC, vf5, vf11y
    vmaddaz.xyzw ACC, vf6, vf11z
    vmaddw.xyzw vf11, vf7, vf11w
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    sqc2 vf10, 0x20(a0)
    sqc2 vf11, 0x30(a0)
    jr ra
    nop
}
/* mtx = mtx * other (4x4). */
asm void Mtx_Concat(f32 *mtx, f32 *other)
{
    .set noreorder
    lqc2 vf4, 0x0(a1)
    lqc2 vf5, 0x10(a1)
    lqc2 vf6, 0x20(a1)
    lqc2 vf7, 0x30(a1)
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    lqc2 vf11, 0x30(a0)
    vmulax.xyzw ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw vf8, vf7, vf8w
    vmulax.xyzw ACC, vf4, vf9x
    vmadday.xyzw ACC, vf5, vf9y
    vmaddaz.xyzw ACC, vf6, vf9z
    vmaddw.xyzw vf9, vf7, vf9w
    vmulax.xyzw ACC, vf4, vf10x
    vmadday.xyzw ACC, vf5, vf10y
    vmaddaz.xyzw ACC, vf6, vf10z
    vmaddw.xyzw vf10, vf7, vf10w
    vmulax.xyzw ACC, vf4, vf11x
    vmadday.xyzw ACC, vf5, vf11y
    vmaddaz.xyzw ACC, vf6, vf11z
    vmaddw.xyzw vf11, vf7, vf11w
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    sqc2 vf10, 0x20(a0)
    sqc2 vf11, 0x30(a0)
    jr ra
    nop
}
/* The 3x3 part of dst = lhs * rhs (3x3); the rest of dst stays. */
asm void Mtx_Mul3x3(f32 *dst, f32 *lhs, f32 *rhs)
{
    .set noreorder
    lqc2 vf4, 0x0(a2)
    lqc2 vf5, 0x10(a2)
    lqc2 vf6, 0x20(a2)
    lqc2 vf8, 0x0(a1)
    lqc2 vf9, 0x10(a1)
    lqc2 vf10, 0x20(a1)
    lqc2 vf11, 0x0(a0)
    lqc2 vf12, 0x10(a0)
    lqc2 vf13, 0x20(a0)
    vmulax.xyz ACC, vf4, vf8x
    vmadday.xyz ACC, vf5, vf8y
    vmaddz.xyz vf11, vf6, vf8z
    vmulax.xyz ACC, vf4, vf9x
    vmadday.xyz ACC, vf5, vf9y
    vmaddz.xyz vf12, vf6, vf9z
    vmulax.xyz ACC, vf4, vf10x
    vmadday.xyz ACC, vf5, vf10y
    vmaddz.xyz vf13, vf6, vf10z
    sqc2 vf11, 0x0(a0)
    sqc2 vf12, 0x10(a0)
    sqc2 vf13, 0x20(a0)
    jr ra
    nop
}
/* The 3x3 part of mtx = mtx * other (3x3). */
asm void Mtx_Concat3x3(f32 *mtx, f32 *other)
{
    .set noreorder
    lqc2 vf4, 0x0(a1)
    lqc2 vf5, 0x10(a1)
    lqc2 vf6, 0x20(a1)
    lqc2 vf8, 0x0(a0)
    lqc2 vf9, 0x10(a0)
    lqc2 vf10, 0x20(a0)
    vmulax.xyz ACC, vf4, vf8x
    vmadday.xyz ACC, vf5, vf8y
    vmaddz.xyz vf8, vf6, vf8z
    vmulax.xyz ACC, vf4, vf9x
    vmadday.xyz ACC, vf5, vf9y
    vmaddz.xyz vf9, vf6, vf9z
    vmulax.xyz ACC, vf4, vf10x
    vmadday.xyz ACC, vf5, vf10y
    vmaddz.xyz vf10, vf6, vf10z
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    sqc2 vf10, 0x20(a0)
    jr ra
    nop
}
/* Inverse of a rotation, scale and translation matrix (transpose / scale^2). */
asm void Mtx_Inverse(f32 *dst, f32 *src)
{
    .set noreorder
    la v1, gMtxIdentity
    lqc2 vf8, 0x0(a1)
    lqc2 vf9, 0x10(a1)
    lqc2 vf10, 0x20(a1)
    lqc2 vf11, 0x30(a1)
    vmul.xyz vf1, vf8, vf8
    vaddy.x vf1, vf1, vf1y
    vaddz.x vf1, vf1, vf1z
    vrsqrt Q, vf0w, vf1x
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x20(v1)
    lqc2 vf7, 0x30(v1)
    vmul.xyz vf2, vf9, vf9
    vaddy.x vf2, vf2, vf2y
    vaddz.x vf2, vf2, vf2z
    vmul.xyz vf3, vf10, vf10
    vaddy.x vf3, vf3, vf3y
    vaddz.x vf3, vf3, vf3z
    vsub.xyz vf21, vf0, vf11
    vwaitq
    vaddq.x vf20, vf0, Q
    vrsqrt Q, vf0w, vf2x
    vmulx.x vf4, vf20, vf8x
    vmuly.x vf5, vf20, vf8y
    vmulz.x vf6, vf20, vf8z
    vwaitq
    vaddq.y vf20, vf0, Q
    vrsqrt Q, vf0w, vf3x
    vmulx.y vf4, vf20, vf9x
    vmuly.y vf5, vf20, vf9y
    vmulz.y vf6, vf20, vf9z
    vwaitq
    vaddq.z vf20, vf0, Q
    vmulx.z vf4, vf20, vf10x
    vmuly.z vf5, vf20, vf10y
    vmulz.z vf6, vf20, vf10z
    vmulax.xyz ACC, vf4, vf21x
    vmadday.xyz ACC, vf5, vf21y
    vmaddz.xyz vf7, vf6, vf21z
    vmul.xyz vf4, vf4, vf20
    vmul.xyz vf5, vf5, vf20
    vmul.xyz vf6, vf6, vf20
    vmul.xyz vf7, vf7, vf20
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    sqc2 vf7, 0x30(a0)
}

/* dst = src (a 4x4 matrix, 16-byte aligned; hand-written, with lq/sq). */
asm void Mtx_Copy(f32 *dst, f32 *src)
{
    .set noreorder
    lq a2, 0x0(a1)
    lq a3, 0x10(a1)
    lq t0, 0x20(a1)
    lq t1, 0x30(a1)
    sq a2, 0x0(a0)
    sq a3, 0x10(a0)
    sq t0, 0x20(a0)
    jr ra
    sq t1, 0x30(a0)
}

/* The 3x3 part of mtx = src's. */
void Mtx_CopyRot(f32 *mtx, f32 *src)
{
    mtx[0] = src[0];
    mtx[1] = src[1];
    mtx[2] = src[2];
    mtx[4] = src[4];
    mtx[5] = src[5];
    mtx[6] = src[6];
    mtx[8] = src[8];
    mtx[9] = src[9];
    mtx[10] = src[10];
}
/* dst = first * weightFirst + second * weightSecond, the rotation made orthonormal again
 * (rows 0 and 1, then their cross products). */
asm void Mtx_Blend(f32 *dst, f32 *first, f32 *second, f32 weightFirst, f32 weightSecond)
{
    .set noreorder
    vmr32.xyzw vf6, vf0
    vmove.xyzw vf7, vf0
    vmr32.xyzw vf5, vf6
    vmr32.xyzw vf4, vf5
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf8
    mfc1 t0, f13
    nop
    qmtc2.ni t0, vf9
    lqc2 vf10, 0x0(a1)
    lqc2 vf11, 0x0(a2)
    lqc2 vf12, 0x10(a1)
    lqc2 vf13, 0x10(a2)
    lqc2 vf14, 0x30(a1)
    lqc2 vf15, 0x30(a2)
    vmulax.xyz ACC, vf10, vf8x
    vmaddx.xyz vf10, vf11, vf9x
    vmulax.xyz ACC, vf12, vf8x
    vmaddx.xyz vf12, vf13, vf9x
    vmulax.xyz ACC, vf14, vf8x
    vmaddx.xyz vf7, vf15, vf9x
    vmul.xyz vf4, vf10, vf10
    vaddy.x vf4, vf4, vf4y
    vaddz.x vf4, vf4, vf4z
    vrsqrt Q, vf0w, vf4x
    vwaitq
    vmulq.xyz vf4, vf10, Q
    vmul.xyz vf5, vf12, vf12
    vaddy.x vf5, vf5, vf5y
    vaddz.x vf5, vf5, vf5z
    vrsqrt Q, vf0w, vf5x
    vwaitq
    vmulq.xyz vf5, vf12, Q
    vopmula.xyz ACC, vf4, vf5
    vopmsub.xyz vf6, vf5, vf4
    vmul.xyz vf8, vf6, vf6
    vaddy.x vf8, vf8, vf8y
    vaddz.x vf8, vf8, vf8z
    vrsqrt Q, vf0w, vf8x
    vwaitq
    vmulq.xyz vf6, vf6, Q
    vopmula.xyz ACC, vf6, vf4
    vopmsub.xyz vf5, vf4, vf6
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    sqc2 vf7, 0x30(a0)
}
/* out = vec * (the 3x3 part of mtx). */
asm void Vec_Rotate(f32 *out, f32 *vec, f32 *mtx)
{
    .set noreorder
    lwc1 f2, 0x0(a1)
    addiu sp, sp, -0x10
    lwc1 f1, 0x4(a1)
    addiu v1, sp, 0x0
    lwc1 f0, 0x8(a1)
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(a2)
    lqc2 vf6, 0x10(a2)
    lqc2 vf7, 0x20(a2)
    vmulax.xyz ACC, vf5, vf4x
    vmadday.xyz ACC, vf6, vf4y
    vmaddz.xyz vf4, vf7, vf4z
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f2, 0x0(v1)
    lwc1 f1, 0x4(v1)
    lwc1 f0, 0x8(v1)
    swc1 f2, 0x0(a0)
    swc1 f1, 0x4(a0)
    swc1 f0, 0x8(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* vec = vec * (the 3x3 part of mtx). */
asm void Vec_RotateSelf(f32 *vec, f32 *mtx)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x10
    lwc1 f1, 0x4(a0)
    addiu v1, sp, 0x0
    lwc1 f0, 0x8(a0)
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(a1)
    lqc2 vf6, 0x10(a1)
    lqc2 vf7, 0x20(a1)
    vmulax.xyz ACC, vf5, vf4x
    vmadday.xyz ACC, vf6, vf4y
    vmaddz.xyz vf4, vf7, vf4z
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f2, 0x0(v1)
    lwc1 f1, 0x4(v1)
    lwc1 f0, 0x8(v1)
    swc1 f2, 0x0(a0)
    swc1 f1, 0x4(a0)
    swc1 f0, 0x8(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* out = vec * mtx (4 components). */
asm void Vec4_Transform(f32 *out, f32 *vec, f32 *mtx)
{
    .set noreorder
    lwc1 f3, 0x0(a1)
    addiu sp, sp, -0x10
    lwc1 f2, 0x4(a1)
    addiu v1, sp, 0x0
    lwc1 f1, 0x8(a1)
    lwc1 f0, 0xC(a1)
    swc1 f3, 0x0(v1)
    swc1 f2, 0x4(v1)
    swc1 f1, 0x8(v1)
    swc1 f0, 0xC(v1)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(a2)
    lqc2 vf6, 0x10(a2)
    lqc2 vf7, 0x20(a2)
    lqc2 vf8, 0x30(a2)
    vmulax.xyzw ACC, vf5, vf4x
    vmadday.xyzw ACC, vf6, vf4y
    vmaddaz.xyzw ACC, vf7, vf4z
    vmaddw.xyzw vf4, vf8, vf4w
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f3, 0x0(v1)
    lwc1 f2, 0x4(v1)
    lwc1 f1, 0x8(v1)
    lwc1 f0, 0xC(v1)
    swc1 f3, 0x0(a0)
    swc1 f2, 0x4(a0)
    swc1 f1, 0x8(a0)
    swc1 f0, 0xC(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* vec = vec * mtx (4 components). */
asm void Vec4_TransformSelf(f32 *vec, f32 *mtx)
{
    .set noreorder
    lwc1 f3, 0x0(a0)
    addiu sp, sp, -0x10
    lwc1 f2, 0x4(a0)
    addiu v1, sp, 0x0
    lwc1 f1, 0x8(a0)
    lwc1 f0, 0xC(a0)
    swc1 f3, 0x0(v1)
    swc1 f2, 0x4(v1)
    swc1 f1, 0x8(v1)
    swc1 f0, 0xC(v1)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(a1)
    lqc2 vf6, 0x10(a1)
    lqc2 vf7, 0x20(a1)
    lqc2 vf8, 0x30(a1)
    vmulax.xyzw ACC, vf5, vf4x
    vmadday.xyzw ACC, vf6, vf4y
    vmaddaz.xyzw ACC, vf7, vf4z
    vmaddw.xyzw vf4, vf8, vf4w
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f3, 0x0(v1)
    lwc1 f2, 0x4(v1)
    lwc1 f1, 0x8(v1)
    lwc1 f0, 0xC(v1)
    swc1 f3, 0x0(a0)
    swc1 f2, 0x4(a0)
    swc1 f1, 0x8(a0)
    swc1 f0, 0xC(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* out = (vec, 1) * mtx divided by its w. */
asm void Vec_Project(f32 *out, f32 *vec, f32 *mtx)
{
    .set noreorder
    lwc1 f0, 0x0(a1)
    addiu sp, sp, -0x10
    lui v1, (0x3F800000 >> 16)
    addiu a3, sp, 0x0
    swc1 f0, 0x0(sp)
    lwc1 f0, 0x4(a1)
    swc1 f0, 0x4(sp)
    lwc1 f0, 0x8(a1)
    swc1 f0, 0x8(sp)
    sw v1, 0xC(sp)
    lqc2 vf4, 0x0(a3)
    lqc2 vf5, 0x0(a2)
    lqc2 vf6, 0x10(a2)
    lqc2 vf7, 0x20(a2)
    lqc2 vf8, 0x30(a2)
    vmulax.xyzw ACC, vf5, vf4x
    vmadday.xyzw ACC, vf6, vf4y
    vmaddaz.xyzw ACC, vf7, vf4z
    vmaddw.xyzw vf4, vf8, vf4w
    vdiv Q, vf0w, vf4w
    vwaitq
    vmulq.xyz vf4, vf4, Q
    sqc2 vf4, 0x0(a3)
    addiu v1, sp, 0x0
    lwc1 f3, 0x0(v1)
    lwc1 f2, 0x4(v1)
    lwc1 f1, 0x8(v1)
    lwc1 f0, 0xC(v1)
    swc1 f3, 0x0(a0)
    swc1 f2, 0x4(a0)
    swc1 f1, 0x8(a0)
    swc1 f0, 0xC(a0)
    jr ra
    addiu sp, sp, 0x10
}

/* vec rotated by angle about the x axis. */
void Vec_RotateX(f32 *vec, f32 angle)
{
    f32 sinA;
    f32 cosA;
    Vec3Copy orig;

    orig = *(Vec3Copy *)vec;
    Math_GetSinCos(&sinA, &cosA, angle);
    vec[1] = cosA * orig.y - sinA * orig.z;
    vec[2] = sinA * orig.y + cosA * orig.z;
}

/* vec rotated by angle about the y axis. */
void Vec_RotateY(f32 *vec, f32 angle)
{
    f32 sinA;
    f32 cosA;
    Vec3Copy orig;

    orig = *(Vec3Copy *)vec;
    Math_GetSinCos(&sinA, &cosA, angle);
    vec[0] = cosA * orig.x + sinA * orig.z;
    vec[2] = -sinA * orig.x + cosA * orig.z;
}

/* vec rotated by angle about the z axis. */
void Vec_RotateZ(f32 *vec, f32 angle)
{
    f32 sinA;
    f32 cosA;
    Vec3Copy orig;

    orig = *(Vec3Copy *)vec;
    Math_GetSinCos(&sinA, &cosA, angle);
    vec[0] = cosA * orig.x - sinA * orig.y;
    vec[1] = sinA * orig.x + cosA * orig.y;
}
/* |vec|. */
asm f32 Vec_Length(f32 *vec)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x10
    lwc1 f1, 0x4(a0)
    addiu v0, sp, 0x0
    lwc1 f0, 0x8(a0)
    swc1 f2, 0x0(v0)
    swc1 f1, 0x4(v0)
    swc1 f0, 0x8(v0)
    lqc2 vf4, 0x0(v0)
    vmul.xyz vf4, vf4, vf4
    vaddy.x vf4, vf4, vf4y
    vaddz.x vf4, vf4, vf4z
    vsqrt Q, vf4x
    vwaitq
    vaddq.x vf4, vf0, Q
    qmfc2.ni v0, vf4
    mtc1 v0, f0
    jr ra
    addiu sp, sp, 0x10
}
/* |from - to|. */
asm f32 Vec_Distance(f32 *from, f32 *to)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x20
    lwc1 f1, 0x4(a0)
    addiu v1, sp, 0x10
    lwc1 f0, 0x8(a0)
    addiu v0, sp, 0x0
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lwc1 f2, 0x0(a1)
    lwc1 f1, 0x4(a1)
    lwc1 f0, 0x8(a1)
    swc1 f2, 0x0(v0)
    swc1 f1, 0x4(v0)
    swc1 f0, 0x8(v0)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(v0)
    vsub.xyz vf4, vf4, vf5
    vmul.xyz vf4, vf4, vf4
    vaddy.x vf4, vf4, vf4y
    vaddz.x vf4, vf4, vf4z
    vsqrt Q, vf4x
    vwaitq
    vaddq.x vf4, vf0, Q
    qmfc2.ni v1, vf4
    mtc1 v1, f0
    jr ra
    addiu sp, sp, 0x20
}
/* vec = vec / |vec|. */
asm void Vec_Normalize(f32 *vec)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x10
    lwc1 f1, 0x4(a0)
    addiu v1, sp, 0x0
    lwc1 f0, 0x8(a0)
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lqc2 vf4, 0x0(v1)
    vmul.xyz vf5, vf4, vf4
    vaddy.x vf5, vf5, vf5y
    vaddz.x vf5, vf5, vf5z
    vrsqrt Q, vf0w, vf5x
    vwaitq
    vmulq.xyz vf4, vf4, Q
    sqc2 vf4, 0x0(v1)
    addiu v1, sp, 0x0
    lwc1 f2, 0x0(v1)
    lwc1 f1, 0x4(v1)
    lwc1 f0, 0x8(v1)
    swc1 f2, 0x0(a0)
    swc1 f1, 0x4(a0)
    swc1 f0, 0x8(a0)
    jr ra
    addiu sp, sp, 0x10
}
/* lhs . rhs (3 components). */
asm f32 Vec_Dot(f32 *lhs, f32 *rhs)
{
    .set noreorder
    lwc1 f2, 0x0(a0)
    addiu sp, sp, -0x20
    lwc1 f1, 0x4(a0)
    addiu v1, sp, 0x10
    lwc1 f0, 0x8(a0)
    addiu v0, sp, 0x0
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lwc1 f2, 0x0(a1)
    lwc1 f1, 0x4(a1)
    lwc1 f0, 0x8(a1)
    swc1 f2, 0x0(v0)
    swc1 f1, 0x4(v0)
    swc1 f0, 0x8(v0)
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x0(v0)
    vmul.xyz vf4, vf4, vf5
    vaddy.x vf4, vf4, vf4y
    vaddz.x vf4, vf4, vf4z
    qmfc2.ni v1, vf4
    mtc1 v1, f0
    jr ra
    addiu sp, sp, 0x20
}
/* out = lhs x rhs. */
asm void Vec_Cross(f32 *out, f32 *lhs, f32 *rhs)
{
    .set noreorder
    lwc1 f2, 0x0(a1)
    addiu sp, sp, -0x20
    lwc1 f1, 0x4(a1)
    addiu a3, sp, 0x10
    lwc1 f0, 0x8(a1)
    addiu v1, sp, 0x0
    swc1 f2, 0x0(a3)
    swc1 f1, 0x4(a3)
    swc1 f0, 0x8(a3)
    lwc1 f2, 0x0(a2)
    lwc1 f1, 0x4(a2)
    lwc1 f0, 0x8(a2)
    swc1 f2, 0x0(v1)
    swc1 f1, 0x4(v1)
    swc1 f0, 0x8(v1)
    lqc2 vf4, 0x0(a3)
    lqc2 vf5, 0x0(v1)
    vopmula.xyz ACC, vf4, vf5
    vopmsub.xyz vf6, vf5, vf4
    sqc2 vf6, 0x0(a3)
    addiu v1, sp, 0x10
    lwc1 f2, 0x0(v1)
    lwc1 f1, 0x4(v1)
    lwc1 f0, 0x8(v1)
    swc1 f2, 0x0(a0)
    swc1 f1, 0x4(a0)
    swc1 f0, 0x8(a0)
    jr ra
    addiu sp, sp, 0x20
}

void Vec_Copy(f32 *dst, f32 *src)
{
    *(Vec3Copy *)dst = *(Vec3Copy *)src;
}

/* Radians to a 16-bit angle (ANGLE_TURN to a turn, in [0, ANGLE_TURN)). */
u32 Math_RadToAngle(f32 rad)
{
    f32 deg = (180.0f * rad) / 3.1415927f;

    if (deg < 0.0f) {
        deg += 360.0f;
    }
    return (u32)(182.04445f * deg);
}

/* A 16-bit angle (ANGLE_TURN to a turn) in radians, in (-pi, pi]. */
f32 Math_Angle16ToRad(u16 angle)
{
    f32 rad = 3.1415927f * (0.0054931641f * angle) / 180.0f;

    if (rad > 3.1415927f) {
        rad += -6.2831855f;
    }
    return rad;
}

/* mtx into slot `slot` of main's matrix table gMtxSlots. */
void MtxSlot_Store(s32 slot, MtxSlot *mtx)
{
    gMtxSlots[slot] = *mtx;
}

/* Slot `slot` of main's matrix table gMtxSlots into mtx. */
void MtxSlot_Load(MtxSlot *mtx, s32 slot)
{
    *mtx = gMtxSlots[slot];
}

extern f32 gMtxView[];
extern f32 gMtxProjScreen[];

/* out = point projected by gMtxSlots[0] * gMtxView * gMtxProjScreen. */
void Camera_ProjectPoint(f32 *out, f32 *point)
{
    f32 mtx[16];

    Mtx_Mul(mtx, (f32 *)gMtxSlots, gMtxView);
    Mtx_Concat(mtx, gMtxProjScreen);
    Vec_Project(out, point, mtx);
}

/* |x|. */
f32 Math_Absf(f32 x)
{
    f32 ret;

    ret = x;
    if (ret < 0.0f) {
        ret = -ret;
    }
    return ret;
}

f32 Math_Acos(f32 x)
{
    return acosf(x);
}

/* asin x = atan(x / sqrt(1 - x * x)). */
f32 Math_Asin(f32 x)
{
    return Math_Atan(x / Math_Sqrtf(1.0f + (-x * x)));
}

f32 Math_Atan(f32 x)
{
    return atanf(x);
}

/* atan2(y, x). */
f32 Math_Atan2(f32 y, f32 x)
{
    return atan2f(y, x);
}

f32 Math_Ceil(f32 x)
{
    return ceilf(x);
}

/* e to the x. */
f32 Math_Exp(f32 x)
{
    return expf(x);
}

f32 Math_Sin(f32 x)
{
    return Math_SinVu0(x);
}

f32 Math_Cos(f32 x)
{
    return Math_CosVu0(x);
}

/* *sinOut = sin angle, *cosOut = cos angle. */
void Math_GetSinCos(f32 *sinOut, f32 *cosOut, f32 angle)
{
    f32 sinCos[4];

    Math_SinCos(sinCos, angle);
    *sinOut = sinCos[0];
    *cosOut = sinCos[1];
}

f32 Math_Floor(f32 x)
{
    return floorf(x);
}

/* x to the y. */
f32 Math_Pow(f32 x, f32 y)
{
    return powf(x, y);
}
/* sqrt x (VU0). */
asm f32 Math_Sqrtf(f32 x)
{
    .set noreorder
    mfc1 t0, f12
    nop
    qmtc2.ni t0, vf4
    vsqrt Q, vf4x
    vwaitq
    vaddq.x vf4, vf0, Q
    qmfc2.ni t0, vf4
    mtc1 t0, f0
    jr ra
    nop
}

/* The rotation matrix of quaternion quat (x, y, z, w), scaled by 2 / |quat|^2 (0 for a
 * zero quat). */
void Quat_ToMatrix(f32 *quat, f32 (*mtx)[4])
{
    f32 normSq = quat[0] * quat[0] + quat[1] * quat[1] + quat[2] * quat[2] + quat[3] * quat[3];
    f32 scale;
    f32 xx, yy, zz, xy, xz, yz, wx, wy, wz;

    scale = (normSq > 0.0f) ? 2.0f / normSq : 0.0f;
    xx = scale * (quat[0] * quat[0]);
    xy = scale * (quat[0] * quat[1]);
    xz = scale * (quat[0] * quat[2]);
    wx = scale * (quat[3] * quat[0]);
    yy = scale * (quat[1] * quat[1]);
    yz = scale * (quat[1] * quat[2]);
    wy = scale * (quat[3] * quat[1]);
    wz = scale * (quat[3] * quat[2]);
    zz = scale * (quat[2] * quat[2]);
    mtx[0][0] = 1.0f - (yy + zz);
    mtx[0][1] = xy + wz;
    mtx[0][2] = xz - wy;
    mtx[1][0] = xy - wz;
    mtx[1][1] = 1.0f - (xx + zz);
    mtx[1][2] = yz + wx;
    mtx[2][0] = xz + wy;
    mtx[2][1] = yz - wx;
    mtx[2][2] = 1.0f - (xx + yy);
    mtx[0][3] = mtx[1][3] = mtx[2][3] = 0.0f;
    mtx[3][0] = mtx[3][1] = mtx[3][2] = 0.0f;
    mtx[3][3] = 1.0f;
}

/* quat (its axis in quat[0..2]) = the rotation by angle about that axis. */
void Quat_FromAxisAngle(f32 *quat, f32 angle)
{
    f32 len;
    f32 halfSin;

    angle *= 0.5f;
    len = Math_Sqrtf(quat[0] * quat[0] + quat[1] * quat[1] + quat[2] * quat[2]);
    quat[3] = 0.0f;
    quat[0] /= len;
    quat[1] /= len;
    quat[2] /= len;
    halfSin = Math_Sin(angle);
    quat[3] = Math_Cos(angle);
    quat[0] *= halfSin;
    quat[1] *= halfSin;
    quat[2] *= halfSin;
}

/* quat = the rotation by angle about axis. */
void Quat_Set(f32 *axis, f32 *quat, f32 angle)
{
    quat[3] = 0.0f;
    quat[0] = axis[0];
    quat[1] = axis[1];
    quat[2] = axis[2];
    Quat_FromAxisAngle(quat, angle);
}

/*
 * Whether the sphere at center of the given radius is in the view volume `planes` (four
 * side planes through the eye at planes[0], [4], [8], [12]; the depth range at planes[16],
 * [17]); out = center in view space (mtx, the view matrix).
 */
s32 Frustum_TestSphere(f32 *center, f32 *out, f32 *mtx, f32 *planes, f32 radius)
{
    out[2] = mtx[2] * center[0] + mtx[6] * center[1] + mtx[10] * center[2] + mtx[14];
    if (out[2] - radius > planes[16]) {
        return 0;
    }
    if (out[2] + radius < planes[17]) {
        return 0;
    }
    out[0] = mtx[0] * center[0] + mtx[4] * center[1] + mtx[8] * center[2] + mtx[12];
    if (out[0] * planes[0] + out[2] * planes[2] > radius) {
        return 0;
    }
    if (out[0] * planes[4] + out[2] * planes[6] > radius) {
        return 0;
    }
    out[1] = mtx[1] * center[0] + mtx[5] * center[1] + mtx[9] * center[2] + mtx[13];
    if (out[1] * planes[9] + out[2] * planes[10] > radius) {
        return 0;
    }
    if (out[1] * planes[13] + out[2] * planes[14] > radius) {
        return 0;
    }
    return 1;
}

/* This file's initialised data. */
f32 gSinVu0Coefs[4] = {
#include "sub_main/00402550.inc"
};
f32 gSinVu0Poly[4] = {
#include "sub_main/00402560.inc"
};
