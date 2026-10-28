/*
 * Small helpers of the engine: a BMP file's pixels (Bmp_GetPixels), the
 * animation curves' key base and their first and last keys' times
 * (AnimCurve_StartTime, AnimCurve_EndTime) with the cubic Hermite curve
 * (Math_Hermite), 3-vector helpers (Vec_*2) and a 3x3 matrix copy, the
 * plane through three points and a view frustum's planes, a free list
 * (FreeList_Init) and a list whose links are tagged in bit 0
 * (TagList_Push, TagList_Pop). The data: Bmp_GetPixels' debug message.
 */

#include "common.h"
#include "motion.h"
#include "image.h"
#include "heap.h"
#include "engine_util.h"
#include "libgcc.h"
#include "libm.h"

extern u8 *gAnimKeyBase;

/* The pixels of BMP file bmp (its header's offBits on from its start), or NULL with a
 * debug message when bmp has no BMP_SIGNATURE. The lobby calls it. */
u8 *Bmp_GetPixels(u8 *bmp)
{
    BmpFileHeader *header = (BmpFileHeader *)bmp;

    if (header->type != BMP_SIGNATURE) {
        Debug_Sprintf("This is not BMP image. @plCreateContextFromBMPImage");
        return NULL;
    }
    return bmp + (header->offBits[0] | (header->offBits[1] << 16));
}

/* Sets gAnimKeyBase, the address the curves' key offsets (AnimCurve.keys) count from. */
void Anim_SetKeyBase(void *buf)
{
    gAnimKeyBase = (u8 *)buf;
}

/* The cubic Hermite curve through (startTime, startValue) and (endTime, endValue), with the
 * tangents startTangent and endTangent, at time. */
f32 Math_Hermite(f32 time, f32 startValue, f32 startTime, f32 startTangent, f32 endValue, f32 endTime, f32 endTangent)
{
    f32 elapsed;
    f32 invSpan;
    f32 cubeTerm;
    f32 invSpanSq;
    f32 sqTerm;
    f32 elapsedSq;
    f32 twoCube;
    f32 threeSq;
    f32 sqTermB;
    f32 basisStart;
    f32 basisStartTangent;
    f32 basisEnd;
    f32 basisEndTangent;

    elapsed = time - startTime;
    invSpan = 1.0f / (endTime - startTime);
    elapsedSq = elapsed * elapsed;
    invSpanSq = invSpan * invSpan;
    sqTerm = elapsedSq * invSpan;
    cubeTerm = invSpanSq * (elapsedSq * elapsed);
    twoCube = 2.0f * cubeTerm * invSpan;
    threeSq = 3.0f * elapsedSq * invSpanSq;
    sqTermB = sqTerm;
    basisStart = 1.0f + (twoCube - threeSq);
    basisStartTangent = elapsed + (cubeTerm - sqTermB - sqTermB);
    basisEnd = -twoCube + threeSq;
    basisEndTangent = cubeTerm - sqTermB;
    return startValue * basisStart + endValue * basisEnd + startTangent * basisStartTangent + endTangent * basisEndTangent;
}

/* The time of curve's first key (0 for ANIM_KEY_NONE and ANIM_KEY_S16_NONE). */
f32 AnimCurve_StartTime(AnimCurve *curve)
{
    u8 *keys = (u8 *)(curve->keys + (u32)gAnimKeyBase);
    f32 time;

    switch (curve->type) {
    case ANIM_KEY_NONE:
        time = 0.0f;
        break;
    case ANIM_KEY_F32_LINEAR:
        time = ((KeyF2 *)keys)->t;
        break;
    case ANIM_KEY_F32_CUBIC:
        time = ((KeyF4 *)keys)->t;
        break;
    case ANIM_KEY_F32_MIXED:
        time = ((KeyFM *)keys)->t;
        break;
    case ANIM_KEY_S16_NONE:
        time = 0.0f;
        break;
    case ANIM_KEY_S16_LINEAR:
        time = ((KeyS2 *)keys)->t;
        break;
    case ANIM_KEY_S16_CUBIC:
        time = ((KeyS4 *)keys)->t;
        break;
    case ANIM_KEY_S16_MIXED:
        time = ((KeySM *)keys)->t;
        break;
    }
    return time;
}

/* The time of curve's last key. */
f32 AnimCurve_GetEndTime(AnimCurve *curve)
{
    return AnimCurve_EndTime(curve);
}

/* The time of curve's last key (0 for ANIM_KEY_NONE and ANIM_KEY_S16_NONE). */
f32 AnimCurve_EndTime(AnimCurve *curve)
{
    u8 *keys = (u8 *)(curve->keys + (u32)gAnimKeyBase);
    f32 time;

    switch (curve->type) {
    case ANIM_KEY_NONE:
        time = 0.0f;
        break;
    case ANIM_KEY_F32_LINEAR:
        time = ((KeyF2 *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_F32_CUBIC:
        time = ((KeyF4 *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_F32_MIXED:
        time = ((KeyFM *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_S16_NONE:
        time = 0.0f;
        break;
    case ANIM_KEY_S16_LINEAR:
        time = ((KeyS2 *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_S16_CUBIC:
        time = ((KeyS4 *)keys)[curve->count - 1].t;
        break;
    case ANIM_KEY_S16_MIXED:
        time = ((KeySM *)keys)[curve->count - 1].t;
        break;
    }
    return time;
}

/* Copies the 3x3 rotation part of the 4x4 matrix src (rows 0-2, columns 0-2) into dst,
 * leaving the rest of dst as it is. */
void Mtx_Copy3x3(f32 *dst, f32 *src)
{
    dst[0] = src[0];
    dst[1] = src[1];
    dst[2] = src[2];
    dst[4] = src[4];
    dst[5] = src[5];
    dst[6] = src[6];
    dst[8] = src[8];
    dst[9] = src[9];
    dst[10] = src[10];
}

f32 sqrtf(f32 x);

/* |vec| (the length of a 3-vector). */
f32 Vec_Length2(f32 *vec)
{
    return sqrtf(vec[0] * vec[0] + vec[1] * vec[1] + vec[2] * vec[2]);
}

/* Scales the 3-vector vec to unit length (as Vec_Normalize); a zero vector stays zero. */
void Vec_Normalize2(f32 *vec)
{
    f32 len;

    len = Vec_Length2(vec);
    if (len == 0.0f) {
        vec[2] = 0.0f;
        vec[1] = 0.0f;
        vec[0] = 0.0f;
        return;
    }
    vec[0] /= len;
    vec[1] = vec[1] / len;
    vec[2] = vec[2] / len;
}

/* lhs . rhs (as Vec_Dot). */
f32 Vec_Dot2(f32 *lhs, f32 *rhs)
{
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

/* dst = lhs x rhs. */
void Vec_Cross2(f32 *dst, f32 *lhs, f32 *rhs)
{
    dst[0] = lhs[1] * rhs[2] - rhs[1] * lhs[2];
    dst[1] = lhs[2] * rhs[0] - rhs[2] * lhs[0];
    dst[2] = lhs[0] * rhs[1] - rhs[0] * lhs[1];
}

/* The plane through pointA, pointB and pointC: the unit normal of edgeAB x edgeAC, and in
 * [3] minus its dot product with edgeAB (as the game computes it: that is 0, where a
 * plane's distance would use pointA). */
void Plane_FromPoints(f32 *plane, f32 *pointA, f32 *pointB, f32 *pointC)
{
    f32 edgeAB[3];
    f32 edgeAC[3];
    f32 normal[3];

    edgeAB[0] = pointB[0] - pointA[0];
    edgeAB[1] = pointB[1] - pointA[1];
    edgeAB[2] = pointB[2] - pointA[2];
    edgeAC[0] = pointC[0] - pointA[0];
    edgeAC[1] = pointC[1] - pointA[1];
    edgeAC[2] = pointC[2] - pointA[2];
    Vec_Cross2(normal, edgeAB, edgeAC);
    Vec_Normalize2(normal);
    plane[0] = normal[0];
    plane[1] = normal[1];
    plane[2] = normal[2];
    plane[3] = -Vec_Dot2(normal, edgeAB);
}

/* (f32)tan(x), with main's double helpers named as they are. */
#define TAN(x) dptofp(tan(fptodp(x)))

/* The four side planes of the view frustum (fov, aspect) through the eye, and in [16], [17]
 * minus near and minus far. */
void Frustum_BuildPlanes(f32 *planes, f32 fov, f32 aspect, f32 near, f32 far)
{
    f32 eye[4];
    f32 cornerA[4];
    f32 cornerB[4];
    f32 halfFov;
    f32 negFar;

    halfFov = 0.5f * fov;
    negFar = -far;
    eye[0] = 0.0f;
    eye[1] = 0.0f;
    eye[2] = 0.0f;
    cornerB[2] = negFar;
    cornerA[2] = negFar;
    cornerA[0] = far * (TAN(halfFov) / aspect);
    cornerA[1] = negFar * TAN(halfFov);
    cornerB[0] = cornerA[0];
    cornerB[1] = -cornerA[1];
    Plane_FromPoints(planes, eye, cornerA, cornerB);
    cornerA[0] = negFar * (TAN(halfFov) / aspect);
    cornerA[1] = far * TAN(halfFov);
    cornerB[0] = cornerA[0];
    cornerB[1] = -cornerA[1];
    Plane_FromPoints(planes + 4, eye, cornerA, cornerB);
    cornerA[0] = far * (TAN(halfFov) / aspect);
    cornerA[1] = far * TAN(halfFov);
    cornerB[0] = -cornerA[0];
    cornerB[1] = cornerA[1];
    Plane_FromPoints(planes + 8, eye, cornerA, cornerB);
    cornerA[0] = negFar * (TAN(halfFov) / aspect);
    cornerA[1] = negFar * TAN(halfFov);
    cornerB[0] = -cornerA[0];
    cornerB[1] = cornerA[1];
    Plane_FromPoints(planes + 12, eye, cornerA, cornerB);
    planes[17] = negFar;
    planes[16] = -near;
}

/* Links the nodeCount words at nodes into a free list: each holds the address of the next, the last 0. */
void FreeList_Init(s32 nodeCount, u32 *nodes)
{
    s32 nodeIdx;

    for (nodeIdx = 0; nodeIdx < nodeCount - 1; nodeIdx++) {
        nodes[nodeIdx] = (u32)&nodes[nodeIdx + 1];
    }
    nodes[nodeIdx] = 0;
}

/* Pushes node onto the list at *link: node takes the old head, and *link the node's
 * address with bit 0 set (the tag TagList_Pop looks for). */
void TagList_Push(u32 *node, u32 *link)
{
    u32 next;

    next = *link;
    *link = (u32)node | 1;
    *node = next;
}

/* Pops the head of a list whose link word carries a flag in bit 0. */
void *TagList_Pop(u32 *head)
{
    u32 nextWord;

    for (;;) {
        if (*head == 0) {
            return NULL;
        }
        nextWord = *(u32 *)*head;
        if (nextWord & 1) {
            return (void *)(*head = nextWord & ~1);
        }
        *head = nextWord;
    }
}
