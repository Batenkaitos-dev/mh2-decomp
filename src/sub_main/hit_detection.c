/*
 * The hit tests and what follows a hit.
 * - The geometry: segments (Segment), spheres and capsules (Capsule,
 *   HitCapsule): touch tests, contact points, push-out vectors and
 *   distances. Vec_WithinDist is static so that the compiler keeps
 *   Capsule_Classify's point and radius in the same registers across its
 *   three calls, as the game's code does.
 * - The attack objects (ObjAttach): each frame
 *   ObjAttach_HitAll tests every object whose hit window is open against the
 *   monsters, the players and the other objects of its area, by their hit
 *   shapes (HitShape lists); the first hit runs ObjAttach_DamageEm or
 *   ObjAttach_DamagePlayer (damage, statuses, hit stop, sounds) and
 *   ObjAttach_HitEffects (sparks and splashes). The hit windows' timers
 *   (hitPhase, hitDelay, hitFrames) and the hit ids are kept here too.
 * - The monsters sub_main runs itself (Em.ownTask set, the lobby's NPCs:
 *   Em_SetupKind, Em_UpdateState, Em_RunByState, Em_Draw), the shadows drawn
 *   under the players and monsters (Shadow_*, enum ShadowType: one model, two
 *   part shadows, five parts in EffectParts, ground-hugging bones, or the
 *   monster's own model), and the hit sparks (HitSpark_*) that ObjAttach_HitEffects and the
 *   attack code spawn.
 */
#include "common.h"
#include "em.h"
#include "scene.h"
#include "player.h"
#include "objhead.h"
#include "objattach.h"
#include "stageobj.h"
#include "sprite.h"
#include "capsule.h"
#include "area.h"
#include "render_state.h"
#include "pad.h"
#include "lobby_state.h"
#include "shadow.h"
#include "item.h"
#include "sub_main/hit_detection.h"   /* the texts this file uses (tools/content_gen.py) */

#pragma optimization_level 4

/* The buffer of a five-part shadow (SHADOW_PARTS, StageObj.buf): one shadow part under each of
 * five bones of the owner (gShadowPartBones), a draw slot each. */
typedef struct EffectParts {
    f32 pos[5][3];              /* 0x00: the parts' positions */
    DrawSlot *draw[5];          /* 0x3C */
    s16 slot[5];                /* 0x50 */
} EffectParts;

/* prototypes */
void EmSlot_Free(Em *em);
void StageObj_Free(StageObj *o);
s32 Debug_IsOn(s32 unused0);
void DrawSlot_Free(s32 id);
s32 Gs_SetState(u8 reg, u32 v);
void ObjAttach_UpdateTimers(ObjAttach *o);
void Shadow_Setup(StageObj *o);
void Shadow_Run(StageObj *o);
void Shadow_End(StageObj *o);
StageObj *StageObj_Alloc(s32 nblk);
void Shadow_Delete(StageObj *o);
void HitSpark_Init(StageObj *o);
void HitSpark_RunTimed(StageObj *o);
void HitSpark_End(StageObj *o);
void HitSpark_Delete(StageObj *o);
void HitSpark_Update(StageObj *o);
extern ObjAttach *gObjAttachList;
/* end of prototypes */


void Vec_Copy(f32 *dst, f32 *src);
f32 Vec_Distance(f32 *a, f32 *b);

/* Fills seg for the segment from start to end: its ends, direction, middle and half length. */
void Segment_Init(f32 *start, f32 *end, Segment *seg)
{
    Vec_Copy(seg->a, start);
    Vec_Copy(seg->b, end);
    seg->dir[0] = end[0] - start[0];
    seg->dir[1] = end[1] - start[1];
    seg->dir[2] = end[2] - start[2];
    seg->mid[0] = 0.5f * (start[0] + end[0]);
    seg->mid[1] = 0.5f * (start[1] + end[1]);
    seg->mid[2] = 0.5f * (start[2] + end[2]);
    seg->halfLen = Vec_Distance(seg->a, seg->mid);
}

/* Sets up dst for the tests from capsule src: its ends and radius, the direction, the middle,
 * and len = half the length plus the radius (the radius of a sphere holding it all). */
void Capsule_Copy(HitCapsule *src, Capsule *dst)
{
    Vec_Copy(dst->a, src->a);
    Vec_Copy(dst->b, src->b);
    dst->radius = src->radius;
    dst->dir[0] = src->b[0] - src->a[0];
    dst->dir[1] = src->b[1] - src->a[1];
    dst->dir[2] = src->b[2] - src->a[2];
    dst->mid[0] = 0.5f * (src->a[0] + src->b[0]);
    dst->mid[1] = 0.5f * (src->a[1] + src->b[1]);
    dst->mid[2] = 0.5f * (src->a[2] + src->b[2]);
    dst->len = Vec_Distance(src->a, dst->mid);
    dst->len += src->radius;
}

/* 1 if b is within r of a. */
static u8 Vec_WithinDist(f32 reach, f32 *center, f32 *point)
{
    f32 dx = point[0] - center[0];
    f32 dy = point[1] - center[1];
    f32 dz = point[2] - center[2];

    return dx * dx + dy * dy + dz * dz <= reach * reach;
}

/* 1 if spheres (a, ra) and (b, rb) touch. */
u8 Sphere_Touch(f32 *centerA, f32 *centerB, f32 radiusA, f32 radiusB)
{
    f32 dx = centerA[0] - centerB[0];
    f32 dy = centerA[1] - centerB[1];
    f32 dz = centerA[2] - centerB[2];

    return dx * dx + dy * dy + dz * dz <= (radiusA + radiusB) * (radiusA + radiusB);
}

/* The contact point of two touching spheres, 0 if they don't touch. */
s32 Spheres_Touch(f32 radiusA, f32 radiusB, f32 *centerA, f32 *centerB, f32 *out)
{
    f32 dist = Vec_Distance(centerA, centerB);

    if (dist <= radiusA + radiusB) {
        if (dist > 0.001f) {
            f32 dx = centerA[0] - centerB[0];
            f32 dy = centerA[1] - centerB[1];
            f32 dz = centerA[2] - centerB[2];

            dist = radiusB / dist;
            dx *= dist;
            out[0] = dx + centerB[0];
            dy *= dist;
            out[1] = dy + centerB[1];
            dz *= dist;
            out[2] = dz + centerB[2];
        } else {
            Vec_Copy(out, centerB);
        }
        return 1;
    }
    return 0;
}

/* The push-out vector of two touching spheres, 0 if they don't touch. */
s32 Sphere_PushOut(f32 radiusA, f32 radiusB, f32 *centerA, f32 *centerB, f32 *out)
{
    f32 dist = Vec_Distance(centerA, centerB);
    f32 sum = radiusA + radiusB;

    if (dist <= sum) {
        if (dist > 0.001f) {
            f32 scale = (sum - dist) / dist;
            f32 dx = centerA[0] - centerB[0];
            f32 dy = centerA[1] - centerB[1];
            f32 dz = centerA[2] - centerB[2];

            out[0] = scale * dx;
            out[1] = scale * dy;
            out[2] = scale * dz;
        } else {
            out[0] = 0.0f;
            out[1] = 0.0f;
            out[2] = 0.0f;
        }
        return 1;
    }
    return 0;
}

f32 Vec_Dot(f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
f32 Math_Absf(f32 x);
asm f32 Math_Sqrtf(f32 x);
void Vec_Normalize(f32 *v);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
void Seg_NearestPoint(f32 *a, f32 *b, f32 *dir, f32 *p, f32 *out, f32 len);
void Vec_PointAlong(f32 *from, f32 *dir, f32 *out, f32 len, f32 dist);

/* Capsule capA against capsule capB: 1 and the contact point on capB's surface in out if they
 * touch, else 0. Parallel axes are handled first; otherwise the closest points of the two axes
 * are clamped to the segments by region (before, on, past each end). As in Capsule_PushCapsule,
 * the float locals are declared in the order that gives the game's saved registers, lineDistSq
 * is kept as its own variable, and case 1 of the outer switch has no break (its inner switch
 * always returns). */
u8 Capsule_ContactCapsule(Capsule *capA, Capsule *capB, f32 *out)
{
    f32 dirA[3];
    f32 dirB[3];
    f32 delta[3];
    f32 delta2[3];
    f32 pointA[3];
    f32 pointB[3];
    f32 offset[3];
    f32 alongB;
    f32 alongA;
    f32 lenA;
    f32 cosAB;
    f32 dist;
    f32 radiusSum;
    f32 lineDist;
    f32 lenB;
    f32 endAlongB;
    f32 scale;
    f32 contactAlong;
    f32 distOther;
    f32 den;
    f32 paramA;
    f32 paramB;
    f32 lineDistSq;
    u8 regionA;
    u8 regionB;

    if (Vec_WithinDist(capA->len + capB->len, capA->mid, capB->mid) == 0) {
        return 0;
    }
    radiusSum = capA->radius + capB->radius;
    lenA = 2.0f * (capA->len - capA->radius);
    if (lenA < 0.001f) {
        lenA = 0.001f;
    }
    lenB = 2.0f * (capB->len - capB->radius);
    if (lenB < 0.001f) {
        lenB = 0.001f;
    }
    dirA[0] = capA->dir[0] / lenA;
    dirA[1] = capA->dir[1] / lenA;
    dirA[2] = capA->dir[2] / lenA;
    dirB[0] = capB->dir[0] / lenB;
    dirB[1] = capB->dir[1] / lenB;
    dirB[2] = capB->dir[2] / lenB;
    cosAB = Vec_Dot(dirA, dirB);
    if (1.0f - Math_Absf(cosAB) < 0.001f) {
        delta[0] = capA->a[0] - capB->a[0];
        delta[1] = capA->a[1] - capB->a[1];
        delta[2] = capA->a[2] - capB->a[2];
        alongB = Vec_Dot(delta, dirB);
        lineDistSq = Vec_Dot(delta, delta) - alongB * alongB;
        if (lineDistSq < 0.0f) {
            return 0;
        }
        lineDist = Math_Sqrtf(lineDistSq);
        if (!(lineDist <= radiusSum)) {
            return 0;
        }
        if (lineDist < 0.001f) {
            delta2[0] = 5.0f + dirB[0];
            delta2[1] = dirB[1];
            delta2[2] = dirB[2];
            Vec_Cross(offset, dirB, delta2);
            if (Math_Absf(offset[0]) < 0.001f && Math_Absf(offset[1]) < 0.001f && Math_Absf(offset[2]) < 0.001f) {
                delta2[1] += 5.0f;
                Vec_Cross(offset, dirB, delta2);
            }
            Vec_Normalize(offset);
            offset[0] *= capB->radius;
            offset[1] *= capB->radius;
            offset[2] *= capB->radius;
        } else {
            offset[0] = alongB * dirB[0];
            offset[1] = alongB * dirB[1];
            offset[2] = alongB * dirB[2];
            pointA[0] = capB->a[0] + offset[0];
            pointA[1] = capB->a[1] + offset[1];
            pointA[2] = capB->a[2] + offset[2];
            offset[0] = capA->a[0] - pointA[0];
            offset[1] = capA->a[1] - pointA[1];
            offset[2] = capA->a[2] - pointA[2];
            scale = capB->radius / lineDist;
            offset[0] *= scale;
            offset[1] *= scale;
            offset[2] *= scale;
        }
        if (alongB < 0.0f) {
            delta2[0] = capA->b[0] - capB->a[0];
            delta2[1] = capA->b[1] - capB->a[1];
            delta2[2] = capA->b[2] - capB->a[2];
            endAlongB = Vec_Dot(delta2, dirB);
            if (!(endAlongB < 0.0f)) {
                if (endAlongB < lenB) {
                    contactAlong = 0.5f * endAlongB;
                } else {
                    contactAlong = 0.5f * lenB;
                }
                delta[0] = contactAlong * dirB[0];
                delta[1] = contactAlong * dirB[1];
                delta[2] = contactAlong * dirB[2];
                pointA[0] = capB->a[0] + delta[0];
                pointA[1] = capB->a[1] + delta[1];
                pointA[2] = capB->a[2] + delta[2];
                out[0] = pointA[0] + offset[0];
                out[1] = pointA[1] + offset[1];
                out[2] = pointA[2] + offset[2];
                return 1;
            }
            if (alongB < endAlongB) {
                dist = Vec_Length(delta2);
                if (dist <= radiusSum) {
                    if (dist < 0.001f) {
                        dist = 0.001f;
                    }
                    scale = capB->radius / dist;
                    delta2[0] *= scale;
                    delta2[1] *= scale;
                    delta2[2] *= scale;
                    out[0] = capB->a[0] + delta2[0];
                    out[1] = capB->a[1] + delta2[1];
                    out[2] = capB->a[2] + delta2[2];
                    return 1;
                }
                return 0;
            }
            dist = Vec_Length(delta);
            if (dist <= radiusSum) {
                if (dist < 0.001f) {
                    dist = 0.001f;
                }
                scale = capB->radius / dist;
                delta[0] *= scale;
                delta[1] *= scale;
                delta[2] *= scale;
                out[0] = capB->a[0] + delta[0];
                out[1] = capB->a[1] + delta[1];
                out[2] = capB->a[2] + delta[2];
                return 1;
            }
            return 0;
        }
        if (alongB <= lenB) {
            delta2[0] = capA->b[0] - capB->a[0];
            delta2[1] = capA->b[1] - capB->a[1];
            delta2[2] = capA->b[2] - capB->a[2];
            endAlongB = Vec_Dot(delta2, dirB);
            if (endAlongB < 0.0f) {
                contactAlong = 0.5f * alongB;
            } else {
                if (!(endAlongB <= lenB)) {
                    contactAlong = alongB + 0.5f * (lenB - alongB);
                } else {
                    contactAlong = alongB + 0.5f * (endAlongB - alongB);
                }
            }
            delta[0] = contactAlong * dirB[0];
            delta[1] = contactAlong * dirB[1];
            delta[2] = contactAlong * dirB[2];
            pointA[0] = capB->a[0] + delta[0];
            pointA[1] = capB->a[1] + delta[1];
            pointA[2] = capB->a[2] + delta[2];
            out[0] = pointA[0] + offset[0];
            out[1] = pointA[1] + offset[1];
            out[2] = pointA[2] + offset[2];
            return 1;
        }
        delta2[0] = capA->b[0] - capB->a[0];
        delta2[1] = capA->b[1] - capB->a[1];
        delta2[2] = capA->b[2] - capB->a[2];
        endAlongB = Vec_Dot(delta2, dirB);
        if (endAlongB < 0.0f) {
            out[0] = capB->mid[0] + offset[0];
            out[1] = capB->mid[1] + offset[1];
            out[2] = capB->mid[2] + offset[2];
            return 1;
        }
        if (endAlongB <= lenB) {
            contactAlong = endAlongB + 0.5f * (lenB - endAlongB);
            delta[0] = contactAlong * dirB[0];
            delta[1] = contactAlong * dirB[1];
            delta[2] = contactAlong * dirB[2];
            pointA[0] = capB->a[0] + delta[0];
            pointA[1] = capB->a[1] + delta[1];
            pointA[2] = capB->a[2] + delta[2];
            out[0] = pointA[0] + offset[0];
            out[1] = pointA[1] + offset[1];
            out[2] = pointA[2] + offset[2];
            return 1;
        }
        if (alongB < endAlongB) {
            dist = Math_Sqrtf(lineDist * lineDist + alongB * alongB);
            if (dist <= radiusSum) {
                if (dist < 0.001f) {
                    dist = 0.001f;
                }
                scale = capB->radius / dist;
                delta[0] *= scale;
                delta[1] *= scale;
                delta[2] *= scale;
                out[0] = capB->a[0] + delta[0];
                out[1] = capB->a[1] + delta[1];
                out[2] = capB->a[2] + delta[2];
                return 1;
            }
            return 0;
        }
        dist = Math_Sqrtf(lineDist * lineDist + endAlongB * endAlongB);
        if (dist <= radiusSum) {
            if (dist < 0.001f) {
                dist = 0.001f;
            }
            scale = capB->radius / dist;
            delta2[0] *= scale;
            delta2[1] *= scale;
            delta2[2] *= scale;
            out[0] = capB->a[0] + delta2[0];
            out[1] = capB->a[1] + delta2[1];
            out[2] = capB->a[2] + delta2[2];
            return 1;
        }
        return 0;
    }
    delta[0] = capB->a[0] - capA->a[0];
    delta[1] = capB->a[1] - capA->a[1];
    delta[2] = capB->a[2] - capA->a[2];
    alongA = Vec_Dot(delta, dirA);
    alongB = Vec_Dot(delta, dirB);
    den = 1.0f - cosAB * cosAB;
    paramA = (alongA - alongB * cosAB) / den;
    paramB = (alongA * cosAB - alongB) / den;
    if (paramA < 0.0f) {
        regionA = 0;
    } else if (paramA <= lenA) {
        regionA = 1;
    } else {
        regionA = 2;
    }
    if (paramB < 0.0f) {
        regionB = 0;
    } else if (paramB <= lenB) {
        regionB = 1;
    } else {
        regionB = 2;
    }
    switch (regionA) {
    case 0:
        Seg_NearestPoint(capB->a, capB->b, dirB, capA->a, pointA, lenB);
        delta[0] = capA->a[0] - pointA[0];
        delta[1] = capA->a[1] - pointA[1];
        delta[2] = capA->a[2] - pointA[2];
        dist = Vec_Length(delta);
        switch (regionB) {
        case 0:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->a, pointB, lenA);
            delta2[0] = pointB[0] - capB->a[0];
            delta2[1] = pointB[1] - capB->a[1];
            delta2[2] = pointB[2] - capB->a[2];
            distOther = Vec_Length(delta2);
            if (dist < distOther) {
                if (dist <= radiusSum) {
                    Vec_PointAlong(pointA, delta, out, dist, capB->radius);
                    return 1;
                }
                return 0;
            }
            if (distOther <= radiusSum) {
                Vec_PointAlong(capB->a, delta2, out, distOther, capB->radius);
                return 1;
            }
            return 0;
        case 1:
            if (dist <= radiusSum) {
                Vec_PointAlong(pointA, delta, out, dist, capB->radius);
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->b, pointB, lenA);
            delta2[0] = pointB[0] - capB->b[0];
            delta2[1] = pointB[1] - capB->b[1];
            delta2[2] = pointB[2] - capB->b[2];
            distOther = Vec_Length(delta2);
            if (dist < distOther) {
                if (dist <= radiusSum) {
                    Vec_PointAlong(pointA, delta, out, dist, capB->radius);
                    return 1;
                }
                return 0;
            }
            if (distOther <= radiusSum) {
                Vec_PointAlong(capB->b, delta2, out, distOther, capB->radius);
                return 1;
            }
            return 0;
        }
        break;
    case 1:
        switch (regionB) {
        case 0:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->a, pointA, lenA);
            delta[0] = pointA[0] - capB->a[0];
            delta[1] = pointA[1] - capB->a[1];
            delta[2] = pointA[2] - capB->a[2];
            dist = Vec_Length(delta);
            if (dist <= radiusSum) {
                Vec_PointAlong(capB->a, delta, out, dist, capB->radius);
                return 1;
            }
            return 0;
        case 1:
            delta[0] = paramA * dirA[0];
            delta[1] = paramA * dirA[1];
            delta[2] = paramA * dirA[2];
            pointA[0] = capA->a[0] + delta[0];
            pointA[1] = capA->a[1] + delta[1];
            pointA[2] = capA->a[2] + delta[2];
            delta[0] = paramB * dirB[0];
            delta[1] = paramB * dirB[1];
            delta[2] = paramB * dirB[2];
            pointB[0] = capB->a[0] + delta[0];
            pointB[1] = capB->a[1] + delta[1];
            pointB[2] = capB->a[2] + delta[2];
            delta[0] = pointA[0] - pointB[0];
            delta[1] = pointA[1] - pointB[1];
            delta[2] = pointA[2] - pointB[2];
            dist = Vec_Length(delta);
            if (dist <= radiusSum) {
                if (dist < 0.001f) {
                    Vec_Cross(delta, dirA, dirB);
                    delta[0] *= capB->radius;
                    delta[1] *= capB->radius;
                    delta[2] *= capB->radius;
                    out[0] = pointB[0] + delta[0];
                    out[1] = pointB[1] + delta[1];
                    out[2] = pointB[2] + delta[2];
                } else {
                    Vec_PointAlong(pointB, delta, out, dist, capB->radius);
                }
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->b, pointA, lenA);
            delta[0] = pointA[0] - capB->b[0];
            delta[1] = pointA[1] - capB->b[1];
            delta[2] = pointA[2] - capB->b[2];
            dist = Vec_Length(delta);
            if (dist <= radiusSum) {
                Vec_PointAlong(capB->b, delta, out, dist, capB->radius);
                return 1;
            }
            return 0;
        }
    case 2:
        Seg_NearestPoint(capB->a, capB->b, dirB, capA->b, pointA, lenB);
        delta[0] = capA->b[0] - pointA[0];
        delta[1] = capA->b[1] - pointA[1];
        delta[2] = capA->b[2] - pointA[2];
        dist = Vec_Length(delta);
        switch (regionB) {
        case 0:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->a, pointB, lenA);
            delta2[0] = pointB[0] - capB->a[0];
            delta2[1] = pointB[1] - capB->a[1];
            delta2[2] = pointB[2] - capB->a[2];
            distOther = Vec_Length(delta2);
            if (dist < distOther) {
                if (dist <= radiusSum) {
                    Vec_PointAlong(pointA, delta, out, dist, capB->radius);
                    return 1;
                }
                return 0;
            }
            if (distOther <= radiusSum) {
                Vec_PointAlong(capB->a, delta2, out, distOther, capB->radius);
                return 1;
            }
            return 0;
        case 1:
            if (dist <= radiusSum) {
                Vec_PointAlong(pointA, delta, out, dist, capB->radius);
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->b, pointB, lenA);
            delta2[0] = pointB[0] - capB->b[0];
            delta2[1] = pointB[1] - capB->b[1];
            delta2[2] = pointB[2] - capB->b[2];
            distOther = Vec_Length(delta2);
            if (dist < distOther) {
                if (dist <= radiusSum) {
                    Vec_PointAlong(pointA, delta, out, dist, capB->radius);
                    return 1;
                }
                return 0;
            }
            if (distOther <= radiusSum) {
                Vec_PointAlong(capB->b, delta2, out, distOther, capB->radius);
                return 1;
            }
            return 0;
        }
        break;
    }
    return 0;
}

void Vec_Copy(f32 *dst, f32 *src);

/* out = from + dir * (dist / len); out = from when len is about 0. */
void Vec_PointAlong(f32 *from, f32 *dir, f32 *out, f32 len, f32 dist)
{
    if (len < 0.001f) {
        Vec_Copy(out, from);
        return;
    }
    len = dist / len;
    dir[0] *= len;
    dir[1] *= len;
    dir[2] *= len;
    out[0] = from[0] + dir[0];
    out[1] = from[1] + dir[1];
    out[2] = from[2] + dir[2];
}

f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Copy(f32 *dst, f32 *src);
f32 Vec_Distance(f32 *a, f32 *b);
/* The point of the segment a-b nearest to p, into out; returns its distance to p. */
f32 Seg_NearestPointDist(f32 *out, f32 *start, f32 *end, f32 *point)
{
    f32 seg[3];
    f32 *segY = &seg[1];
    f32 *segZ = &seg[2];
    f32 toPoint[3];
    f32 lenSq;
    f32 param;

    seg[0] = end[0] - start[0];
    *segY = end[1] - start[1];
    *segZ = end[2] - start[2];
    lenSq = Vec_Dot(seg, seg);
    if (lenSq < 0.001f) {
        Vec_Copy(out, start);
        return Vec_Distance(point, out);
    }
    toPoint[0] = point[0] - start[0];
    toPoint[1] = point[1] - start[1];
    toPoint[2] = point[2] - start[2];
    param = Vec_Dot(seg, toPoint) / lenSq;
    if (param <= 0.0f) {
        Vec_Copy(out, start);
    } else if (param >= 1.0f) {
        Vec_Copy(out, end);
    } else {
        out[0] = param * seg[0];
        out[1] = param * *segY;
        out[2] = param * *segZ;
        out[0] += start[0];
        out[1] += start[1];
        out[2] += start[2];
    }
    return Vec_Distance(point, out);
}

f32 Vec_Length(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Math_Absf(f32 x);
asm f32 Math_Sqrtf(f32 x);
f32 Seg_PointDistance(f32 *a, f32 *b, f32 *dir, f32 *p, f32 len);
/* The distance between segments a0-a1 and b0-b1. */
f32 Seg_SegDistance(f32 *startA, f32 *endA, f32 *startB, f32 *endB)
{
    f32 vecA[3];
    f32 vecB[3];
    f32 dirA[3];
    f32 dirB[3];
    f32 delta[3];
    f32 pointA[3];
    f32 cosAB;
    f32 lenA;
    f32 lineDist;
    f32 pointB[3];
    f32 alongB;
    f32 endAlongB;
    f32 lenB;
    f32 alongA;
    f32 den;
    f32 paramA;
    f32 paramB;
    f32 dist;
    f32 distOther;
    u8 regionA;
    u8 regionB;

    vecA[0] = endA[0] - startA[0];
    vecA[1] = endA[1] - startA[1];
    vecA[2] = endA[2] - startA[2];
    vecB[0] = endB[0] - startB[0];
    vecB[1] = endB[1] - startB[1];
    vecB[2] = endB[2] - startB[2];
    lenA = Vec_Length(vecA);
    if (lenA < 0.001f) {
        lenA = 0.001f;
    }
    lenB = Vec_Length(vecB);
    if (lenB < 0.001f) {
        lenB = 0.001f;
    }
    dirA[0] = vecA[0] / lenA;
    dirA[1] = vecA[1] / lenA;
    dirA[2] = vecA[2] / lenA;
    dirB[0] = vecB[0] / lenB;
    dirB[1] = vecB[1] / lenB;
    dirB[2] = vecB[2] / lenB;
    cosAB = Vec_Dot(dirA, dirB);
    if (1.0f - Math_Absf(cosAB) < 0.001f) {
        delta[0] = startA[0] - startB[0];
        delta[1] = startA[1] - startB[1];
        delta[2] = startA[2] - startB[2];
        alongB = Vec_Dot(delta, dirB);
        lineDist = Math_Sqrtf(Vec_Dot(delta, delta) - alongB * alongB);
        if (alongB < 0.0f) {
            delta[0] = endA[0] - startB[0];
            delta[1] = endA[1] - startB[1];
            delta[2] = endA[2] - startB[2];
            endAlongB = Vec_Dot(delta, dirB);
            if (!(endAlongB < 0.0f)) {
                return lineDist;
            }
            if (alongB < endAlongB) {
                return Vec_Distance(endA, startB);
            } else {
                return Vec_Distance(startA, startB);
            }
        }
        if (alongB <= lenB) {
            return lineDist;
        }
        delta[0] = endA[0] - endB[0];
        delta[1] = endA[1] - endB[1];
        delta[2] = endA[2] - endB[2];
        endAlongB = Vec_Dot(delta, dirB);
        if (endAlongB <= 0.0f) {
            return lineDist;
        }
        alongB -= lenB;
        if (alongB < endAlongB) {
            return Vec_Distance(startA, endB);
        } else {
            return Vec_Distance(endA, endB);
        }
    }
    delta[0] = startB[0] - startA[0];
    delta[1] = startB[1] - startA[1];
    delta[2] = startB[2] - startA[2];
    alongA = Vec_Dot(delta, dirA);
    alongB = Vec_Dot(delta, dirB);
    den = 1.0f - cosAB * cosAB;
    paramA = (alongA - alongB * cosAB) / den;
    paramB = (alongA * cosAB - alongB) / den;
    if (paramA < 0.0f) {
        regionA = 0;
    } else if (paramA <= lenA) {
        regionA = 1;
    } else {
        regionA = 2;
    }
    if (paramB < 0.0f) {
        regionB = 0;
    } else if (paramB <= lenB) {
        regionB = 1;
    } else {
        regionB = 2;
    }
    switch (regionA) {
    case 0:
        dist = Seg_PointDistance(startB, endB, dirB, startA, lenB);
        switch (regionB) {
        case 0:
            distOther = Seg_PointDistance(startA, endA, dirA, startB, lenA);
            break;
        case 1:
            return dist;
        case 2:
            distOther = Seg_PointDistance(startA, endA, dirA, endB, lenA);
            break;
        }
        break;
    case 1:
        switch (regionB) {
        case 0:
            return Seg_PointDistance(startA, endA, dirA, startB, lenA);
        case 1:
            delta[0] = paramA * dirA[0];
            delta[1] = paramA * dirA[1];
            delta[2] = paramA * dirA[2];
            pointA[0] = startA[0] + delta[0];
            pointA[1] = startA[1] + delta[1];
            pointA[2] = startA[2] + delta[2];
            delta[0] = paramB * dirB[0];
            delta[1] = paramB * dirB[1];
            delta[2] = paramB * dirB[2];
            pointB[0] = startB[0] + delta[0];
            pointB[1] = startB[1] + delta[1];
            pointB[2] = startB[2] + delta[2];
            return Vec_Distance(pointA, pointB);
        case 2:
            return Seg_PointDistance(startA, endA, dirA, endB, lenA);
        }
    case 2:
        dist = Seg_PointDistance(startB, endB, dirB, endA, lenB);
        switch (regionB) {
        case 0:
            distOther = Seg_PointDistance(startA, endA, dirA, startB, lenA);
            break;
        case 1:
            return dist;
        case 2:
            distOther = Seg_PointDistance(startA, endA, dirA, endB, lenA);
            return 0.0f; /* as the game: 0 when both closest points lie past the ends */
        }
        break;
    }
    if (dist < distOther) {
        return dist;
    }
    dist = distOther;
    return dist;
}

void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
f32 Math_Absf(f32 x);
f32 Vec_Distance(f32 *a, f32 *b);

/* Sphere (p, r) against capsule cap: 1 and the nearest point of the capsule's surface in out if they touch. */
s32 Capsule_SphereContact(f32 *center, Capsule *cap, f32 *out, f32 sphereRadius)
{
    f32 dir[3];
    f32 delta[3];
    f32 normal[3];
    f32 perp[3];
    f32 foot[3];
    f32 probe[3];
    f32 axisDist;
    f32 radiusSum;

    if (Sphere_Touch(center, cap->mid, sphereRadius, cap->len) == 0) {
        return 0;
    }
    Vec_Copy(dir, cap->dir);
    delta[0] = cap->a[0] - center[0];
    delta[1] = cap->a[1] - center[1];
    delta[2] = cap->a[2] - center[2];
    Vec_Cross(normal, dir, delta);
    Vec_Cross(perp, dir, normal);
    Vec_Normalize(perp);
    axisDist = Vec_Dot(perp, delta);
    radiusSum = sphereRadius + cap->radius;
    if (!(Math_Absf(axisDist) <= radiusSum)) {
        return 0;
    }
    normal[0] = axisDist * perp[0];
    normal[1] = axisDist * perp[1];
    normal[2] = axisDist * perp[2];
    foot[0] = center[0] + normal[0];
    foot[1] = center[1] + normal[1];
    foot[2] = center[2] + normal[2];
    delta[0] = foot[0] - cap->a[0];
    delta[1] = foot[1] - cap->a[1];
    delta[2] = foot[2] - cap->a[2];
    if (Vec_Dot(delta, dir) < 0.0f) {
        if (!(Vec_Distance(cap->a, center) <= radiusSum)) {
            return 0;
        }
        delta[0] = center[0] - cap->a[0];
        delta[1] = center[1] - cap->a[1];
        delta[2] = center[2] - cap->a[2];
        Vec_Normalize(delta);
        delta[0] *= cap->radius;
        delta[1] *= cap->radius;
        delta[2] *= cap->radius;
        out[0] = cap->a[0] + delta[0];
        out[1] = cap->a[1] + delta[1];
        out[2] = cap->a[2] + delta[2];
        return 1;
    }
    delta[0] = foot[0] - cap->b[0];
    delta[1] = foot[1] - cap->b[1];
    delta[2] = foot[2] - cap->b[2];
    dir[0] = -dir[0];
    dir[1] = -dir[1];
    dir[2] = -dir[2];
    if (Vec_Dot(delta, dir) < 0.0f) {
        if (!(Vec_Distance(cap->b, center) <= radiusSum)) {
            return 0;
        }
        delta[0] = center[0] - cap->b[0];
        delta[1] = center[1] - cap->b[1];
        delta[2] = center[2] - cap->b[2];
        Vec_Normalize(delta);
        delta[0] *= cap->radius;
        delta[1] *= cap->radius;
        delta[2] *= cap->radius;
        out[0] = cap->b[0] + delta[0];
        out[1] = cap->b[1] + delta[1];
        out[2] = cap->b[2] + delta[2];
        return 1;
    }
    if (Math_Absf(axisDist) < 0.001f) {
        /* p on the axis: any direction perpendicular to it */
        Vec_Copy(probe, foot);
        probe[0] += 5.0f;
        delta[0] = probe[0] - cap->b[0];
        delta[1] = probe[1] - cap->b[1];
        delta[2] = probe[2] - cap->b[2];
        Vec_Cross(normal, delta, dir);
        if (Vec_Dot(normal, normal) < 0.001f) {
            foot[1] += 5.0f;
            delta[0] = probe[0] - cap->b[0];
            delta[1] = probe[1] - cap->b[1];
            delta[2] = probe[2] - cap->b[2];
            Vec_Cross(normal, delta, dir);
        }
        Vec_Cross(delta, normal, dir);
        Vec_Normalize(delta);
    } else {
        delta[0] = center[0] - foot[0];
        delta[1] = center[1] - foot[1];
        delta[2] = center[2] - foot[2];
        Vec_Normalize(delta);
    }
    delta[0] *= cap->radius;
    delta[1] *= cap->radius;
    delta[2] *= cap->radius;
    out[0] = foot[0] + delta[0];
    out[1] = foot[1] + delta[1];
    out[2] = foot[2] + delta[2];
    return 1;
}


/* Sphere (p, r) against capsule cap: 1 and the push-out vector in out if they touch. */
s32 Capsule_PushSphere(Capsule *cap, f32 *center, f32 *out, f32 sphereRadius)
{
    f32 dir[3];
    f32 delta[3];
    f32 normal[3];
    f32 perp[3];
    f32 radiusSum;
    f32 axisDist;
    f32 len;
    f32 depth;
    f32 x, y, z;

    if (Sphere_Touch(center, cap->mid, sphereRadius, cap->len) == 0) {
        return 0;
    }
    Vec_Copy(dir, cap->dir);
    delta[0] = cap->a[0] - center[0];
    delta[1] = cap->a[1] - center[1];
    delta[2] = cap->a[2] - center[2];
    Vec_Cross(normal, dir, delta);
    Vec_Cross(perp, dir, normal);
    Vec_Normalize(perp);
    radiusSum = cap->radius + sphereRadius;
    axisDist = Vec_Dot(perp, delta);
    if (!(Math_Absf(axisDist) <= radiusSum)) {
        return 0;
    }
    normal[0] = axisDist * perp[0];
    normal[1] = axisDist * perp[1];
    normal[2] = axisDist * perp[2];
    x = center[0] + normal[0];
    y = center[1] + normal[1];
    z = center[2] + normal[2];
    delta[0] = x - cap->a[0];
    delta[1] = y - cap->a[1];
    delta[2] = z - cap->a[2];
    if (Vec_Dot(delta, dir) < 0.0f) {
        delta[0] = cap->a[0] - center[0];
        delta[1] = cap->a[1] - center[1];
        delta[2] = cap->a[2] - center[2];
        len = Vec_Length(delta);
        if (!(len <= radiusSum)) {
            return 0;
        }
        Vec_Normalize(delta);
        depth = radiusSum - len;
        out[0] = depth * delta[0];
        out[1] = depth * delta[1];
        out[2] = depth * delta[2];
        return 1;
    }
    delta[0] = x - cap->b[0];
    delta[1] = y - cap->b[1];
    delta[2] = z - cap->b[2];
    dir[0] = -dir[0];
    dir[1] = -dir[1];
    dir[2] = -dir[2];
    if (Vec_Dot(delta, dir) < 0.0f) {
        delta[0] = cap->b[0] - center[0];
        delta[1] = cap->b[1] - center[1];
        delta[2] = cap->b[2] - center[2];
        len = Vec_Length(delta);
        if (!(len <= radiusSum)) {
            return 0;
        }
        Vec_Normalize(delta);
        depth = radiusSum - len;
        out[0] = depth * delta[0];
        out[1] = depth * delta[1];
        out[2] = depth * delta[2];
        return 1;
    }
    if (axisDist < 0.0f) {
        radiusSum = -radiusSum;
    }
    depth = radiusSum - axisDist;
    out[0] = depth * perp[0];
    out[1] = depth * perp[1];
    out[2] = depth * perp[2];
    return 1;
}


/* Sphere (p, r) against capsule cap: 1 and the point of the sphere nearest the axis in out if they touch. */
s32 Capsule_SphereContactOnSphere(Capsule *cap, f32 *center, f32 *out, f32 sphereRadius)
{
    f32 dir[3];
    f32 delta[3];
    f32 normal[3];
    f32 perp[3];
    f32 radiusSum;
    f32 axisDist;
    f32 x, y, z;

    if (Vec_WithinDist(cap->len + sphereRadius, center, cap->mid) == 0) {
        return 0;
    }
    Vec_Copy(dir, cap->dir);
    delta[0] = cap->a[0] - center[0];
    delta[1] = cap->a[1] - center[1];
    delta[2] = cap->a[2] - center[2];
    Vec_Cross(normal, dir, delta);
    Vec_Cross(perp, dir, normal);
    Vec_Normalize(perp);
    radiusSum = cap->radius + sphereRadius;
    axisDist = Vec_Dot(perp, delta);
    if (!(Math_Absf(axisDist) <= radiusSum)) {
        return 0;
    }
    normal[0] = axisDist * perp[0];
    normal[1] = axisDist * perp[1];
    normal[2] = axisDist * perp[2];
    x = center[0] + normal[0];
    y = center[1] + normal[1];
    z = center[2] + normal[2];
    delta[0] = x - cap->a[0];
    delta[1] = y - cap->a[1];
    delta[2] = z - cap->a[2];
    if (Vec_Dot(delta, dir) < 0.0f) {
        delta[0] = cap->a[0] - center[0];
        delta[1] = cap->a[1] - center[1];
        delta[2] = cap->a[2] - center[2];
        if (!(Vec_Length(delta) <= radiusSum)) {
            return 0;
        }
        Vec_Normalize(delta);
        out[0] = sphereRadius * delta[0];
        out[1] = sphereRadius * delta[1];
        out[2] = sphereRadius * delta[2];
        out[0] += center[0];
        out[1] += center[1];
        out[2] += center[2];
        return 1;
    }
    delta[0] = x - cap->b[0];
    delta[1] = y - cap->b[1];
    delta[2] = z - cap->b[2];
    dir[0] = -dir[0];
    dir[1] = -dir[1];
    dir[2] = -dir[2];
    if (Vec_Dot(delta, dir) < 0.0f) {
        delta[0] = cap->b[0] - center[0];
        delta[1] = cap->b[1] - center[1];
        delta[2] = cap->b[2] - center[2];
        if (!(Vec_Length(delta) <= radiusSum)) {
            return 0;
        }
        Vec_Normalize(delta);
        out[0] = sphereRadius * delta[0];
        out[1] = sphereRadius * delta[1];
        out[2] = sphereRadius * delta[2];
        out[0] += center[0];
        out[1] += center[1];
        out[2] += center[2];
        return 1;
    }
    out[0] = -sphereRadius * perp[0];
    out[1] = -sphereRadius * perp[1];
    out[2] = -sphereRadius * perp[2];
    out[0] += center[0];
    out[1] += center[1];
    out[2] += center[2];
    return 1;
}

u8 Vec_WithinDist(f32 r, f32 *a, f32 *b);
void Seg_NearestPoint(f32 *a, f32 *b, f32 *dir, f32 *p, f32 *out, f32 len);
void Vec_PushOut(f32 *arg0, f32 fparg0, f32 fparg1, f32 *arg1);
void Vec_Copy(f32 *dst, f32 *src);
void Vec_Normalize(f32 *v);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
f32 Math_Absf(f32 x);
asm f32 Math_Sqrtf(f32 x);
/* Capsule capA against capsule capB: 1 and the push-out vector in out if they touch. The float
 * locals are declared in the order that gives the game's saved registers, and case 1 of the
 * outer switch has no break (its inner switch always returns: regionB is 0-2). */
s32 Capsule_PushCapsule(Capsule *capA, Capsule *capB, f32 *out)
{
    f32 dirA[3];
    f32 dirB[3];
    f32 delta[3];
    f32 delta2[3];
    f32 pointA[3];
    f32 pointB[3];
    f32 offset[3];
    f32 alongB;
    f32 alongA;
    f32 lenA;
    f32 cosAB;
    f32 dist;
    f32 radiusSum;
    f32 lineDist;
    f32 lenB;
    f32 endAlongB;
    f32 scale;
    f32 distOther;
    f32 den;
    f32 paramA;
    f32 paramB;
    f32 lineDistSq;
    u8 regionA;
    u8 regionB;

    if (Vec_WithinDist(capA->len + capB->len, capA->mid, capB->mid) == 0) {
        return 0;
    }
    radiusSum = capA->radius + capB->radius;
    lenA = 2.0f * (capA->len - capA->radius);
    if (lenA < 0.001f) {
        lenA = 0.001f;
    }
    lenB = 2.0f * (capB->len - capB->radius);
    if (lenB < 0.001f) {
        lenB = 0.001f;
    }
    dirA[0] = capA->dir[0] / lenA;
    dirA[1] = capA->dir[1] / lenA;
    dirA[2] = capA->dir[2] / lenA;
    dirB[0] = capB->dir[0] / lenB;
    dirB[1] = capB->dir[1] / lenB;
    dirB[2] = capB->dir[2] / lenB;
    cosAB = Vec_Dot(dirA, dirB);
    if (1.0f - Math_Absf(cosAB) < 0.001f) {
        delta[0] = capB->a[0] - capA->a[0];
        delta[1] = capB->a[1] - capA->a[1];
        delta[2] = capB->a[2] - capA->a[2];
        alongB = Vec_Dot(delta, dirA);
        lineDistSq = Vec_Dot(delta, delta) - alongB * alongB;
        if (lineDistSq < 0.0f) {
            return 0;
        }
        lineDist = Math_Sqrtf(lineDistSq);
        if (!(lineDist <= radiusSum)) {
            return 0;
        }
        if (lineDist < 0.001f) {
            delta2[0] = 5.0f + dirA[0];
            delta2[1] = dirA[1];
            delta2[2] = dirA[2];
            Vec_Cross(offset, dirA, delta2);
            if (Math_Absf(offset[0]) < 0.001f && Math_Absf(offset[1]) < 0.001f && Math_Absf(offset[2]) < 0.001f) {
                delta2[1] += 5.0f;
                Vec_Cross(offset, dirA, delta2);
            }
            Vec_Normalize(offset);
            offset[0] *= radiusSum;
            offset[1] *= radiusSum;
            offset[2] *= radiusSum;
        } else {
            scale = (radiusSum - lineDist) / lineDist;
            offset[0] = alongB * dirA[0];
            offset[1] = alongB * dirA[1];
            offset[2] = alongB * dirA[2];
            pointA[0] = capA->a[0] + offset[0];
            pointA[1] = capA->a[1] + offset[1];
            pointA[2] = capA->a[2] + offset[2];
            offset[0] = capB->a[0] - pointA[0];
            offset[1] = capB->a[1] - pointA[1];
            offset[2] = capB->a[2] - pointA[2];
            offset[0] *= scale;
            offset[1] *= scale;
            offset[2] *= scale;
        }
        if (alongB < 0.0f) {
            delta2[0] = capB->b[0] - capA->a[0];
            delta2[1] = capB->b[1] - capA->a[1];
            delta2[2] = capB->b[2] - capA->a[2];
            endAlongB = Vec_Dot(delta2, dirA);
            if (!(endAlongB < 0.0f)) {
                Vec_Copy(out, offset);
                return 1;
            }
            if (alongB < endAlongB) {
                dist = Vec_Length(delta2);
                if (dist <= radiusSum) {
                    scale = (radiusSum - dist) / dist;
                    out[0] = scale * delta2[0];
                    out[1] = scale * delta2[1];
                    out[2] = scale * delta2[2];
                    return 1;
                }
                return 0;
            }
            dist = Vec_Length(delta);
            if (dist <= radiusSum) {
                scale = (radiusSum - dist) / dist;
                out[0] = scale * delta[0];
                out[1] = scale * delta[1];
                out[2] = scale * delta[2];
                return 1;
            }
            return 0;
        }
        if (alongB <= lenB) {
            Vec_Copy(out, offset);
            return 1;
        }
        delta2[0] = capA->b[0] - capB->a[0];
        delta2[1] = capA->b[1] - capB->a[1];
        delta2[2] = capA->b[2] - capB->a[2];
        endAlongB = Vec_Dot(delta2, dirB);
        if (endAlongB <= lenB) {
            Vec_Copy(out, offset);
            return 1;
        }
        if (alongB < endAlongB) {
            dist = Math_Sqrtf(lineDist * lineDist + alongB * alongB);
            if (dist <= radiusSum) {
                scale = (radiusSum - dist) / dist;
                out[0] = scale * delta[0];
                out[1] = scale * delta[1];
                out[2] = scale * delta[2];
                return 1;
            }
            return 0;
        }
        dist = Math_Sqrtf(lineDist * lineDist + endAlongB * endAlongB);
        if (dist <= radiusSum) {
            scale = (radiusSum - dist) / dist;
            out[0] = scale * delta2[0];
            out[1] = scale * delta2[1];
            out[2] = scale * delta2[2];
            return 1;
        }
        return 0;
    }
    delta[0] = capB->a[0] - capA->a[0];
    delta[1] = capB->a[1] - capA->a[1];
    delta[2] = capB->a[2] - capA->a[2];
    alongA = Vec_Dot(delta, dirA);
    alongB = Vec_Dot(delta, dirB);
    den = 1.0f - cosAB * cosAB;
    paramA = (alongA - alongB * cosAB) / den;
    paramB = (alongA * cosAB - alongB) / den;
    if (paramA < 0.0f) {
        regionA = 0;
    } else if (paramA <= lenA) {
        regionA = 1;
    } else {
        regionA = 2;
    }
    if (paramB < 0.0f) {
        regionB = 0;
    } else if (paramB <= lenB) {
        regionB = 1;
    } else {
        regionB = 2;
    }
    switch (regionA) {
    case 0:
        Seg_NearestPoint(capB->a, capB->b, dirB, capA->a, pointA, lenB);
        delta[0] = pointA[0] - capA->a[0];
        delta[1] = pointA[1] - capA->a[1];
        delta[2] = pointA[2] - capA->a[2];
        dist = Vec_Length(delta);
        switch (regionB) {
        case 0:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->a, pointB, lenA);
            delta2[0] = capB->a[0] - pointB[0];
            delta2[1] = capB->a[1] - pointB[1];
            delta2[2] = capB->a[2] - pointB[2];
            distOther = Vec_Length(delta2);
            if (dist < distOther) {
                if (dist <= radiusSum) {
                    Vec_PushOut(delta, radiusSum, dist, out);
                    return 1;
                }
                return 0;
            }
            if (distOther <= radiusSum) {
                Vec_PushOut(delta2, radiusSum, distOther, out);
                return 1;
            }
            return 0;
        case 1:
            if (dist <= radiusSum) {
                Vec_PushOut(delta, radiusSum, dist, out);
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->b, pointB, lenA);
            delta2[0] = capB->b[0] - pointB[0];
            delta2[1] = capB->b[1] - pointB[1];
            delta2[2] = capB->b[2] - pointB[2];
            distOther = Vec_Length(delta2);
            if (dist < distOther) {
                if (dist <= radiusSum) {
                    Vec_PushOut(delta, radiusSum, dist, out);
                    return 1;
                }
                return 0;
            }
            if (distOther <= radiusSum) {
                Vec_PushOut(delta2, radiusSum, distOther, out);
                return 1;
            }
            return 0;
        }
        break;
    case 1:
        switch (regionB) {
        case 0:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->a, pointA, lenA);
            delta[0] = capB->a[0] - pointA[0];
            delta[1] = capB->a[1] - pointA[1];
            delta[2] = capB->a[2] - pointA[2];
            dist = Vec_Length(delta);
            if (dist <= radiusSum) {
                Vec_PushOut(delta, radiusSum, dist, out);
                return 1;
            }
            return 0;
        case 1:
            delta[0] = paramA * dirA[0];
            delta[1] = paramA * dirA[1];
            delta[2] = paramA * dirA[2];
            pointA[0] = capA->a[0] + delta[0];
            pointA[1] = capA->a[1] + delta[1];
            pointA[2] = capA->a[2] + delta[2];
            delta[0] = paramB * dirB[0];
            delta[1] = paramB * dirB[1];
            delta[2] = paramB * dirB[2];
            pointB[0] = capB->a[0] + delta[0];
            pointB[1] = capB->a[1] + delta[1];
            pointB[2] = capB->a[2] + delta[2];
            delta[0] = pointB[0] - pointA[0];
            delta[1] = pointB[1] - pointA[1];
            delta[2] = pointB[2] - pointA[2];
            dist = Vec_Length(delta);
            if (dist <= radiusSum) {
                if (dist < 0.001f) {
                    Vec_Cross(delta, dirA, dirB);
                    out[0] = radiusSum * delta[0];
                    out[1] = radiusSum * delta[1];
                    out[2] = radiusSum * delta[2];
                } else {
                    Vec_PushOut(delta, radiusSum, dist, out);
                }
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->b, pointA, lenA);
            delta[0] = capB->b[0] - pointA[0];
            delta[1] = capB->b[1] - pointA[1];
            delta[2] = capB->b[2] - pointA[2];
            dist = Vec_Length(delta);
            if (dist <= radiusSum) {
                Vec_PushOut(delta, radiusSum, dist, out);
                return 1;
            }
            return 0;
        }
    case 2:
        Seg_NearestPoint(capB->a, capB->b, dirB, capA->b, pointA, lenB);
        delta[0] = pointA[0] - capA->b[0];
        delta[1] = pointA[1] - capA->b[1];
        delta[2] = pointA[2] - capA->b[2];
        dist = Vec_Length(delta);
        switch (regionB) {
        case 0:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->a, pointB, lenA);
            delta2[0] = capB->a[0] - pointB[0];
            delta2[1] = capB->a[1] - pointB[1];
            delta2[2] = capB->a[2] - pointB[2];
            distOther = Vec_Length(delta2);
            if (dist < distOther) {
                if (dist <= radiusSum) {
                    Vec_PushOut(delta, radiusSum, dist, out);
                    return 1;
                }
                return 0;
            }
            if (distOther <= radiusSum) {
                Vec_PushOut(delta2, radiusSum, distOther, out);
                return 1;
            }
            return 0;
        case 1:
            if (dist <= radiusSum) {
                Vec_PushOut(delta, radiusSum, dist, out);
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(capA->a, capA->b, dirA, capB->b, pointB, lenA);
            delta2[0] = capB->b[0] - pointB[0];
            delta2[1] = capB->b[1] - pointB[1];
            delta2[2] = capB->b[2] - pointB[2];
            distOther = Vec_Length(delta2);
            if (dist < distOther) {
                if (dist <= radiusSum) {
                    Vec_PushOut(delta, radiusSum, dist, out);
                    return 1;
                }
                return 0;
            }
            if (distOther <= radiusSum) {
                Vec_PushOut(delta2, radiusSum, distOther, out);
                return 1;
            }
            return 0;
        }
        break;
    }
    return 0;
}

/* out = dir * ((reach - dist) / dist): the push that moves a point at distance dist along dir
 * out to distance reach (dist is clamped to 0.001 for the division). out is left as it is when
 * dist is 0. */
void Vec_PushOut(f32 *dir, f32 reach, f32 dist, f32 *out)
{
    f32 depth;
    f32 scale;
    f32 safeDist;

    safeDist = dist;
    if (safeDist != 0.0f) {
        depth = reach - safeDist;
        if (safeDist < 0.001f) {
            safeDist = 0.001f;
        }
        scale = depth / safeDist;
        out[0] = scale * dir[0];
        out[1] = scale * dir[1];
        out[2] = scale * dir[2];
    }
}

void Vec_Copy(f32 *dst, f32 *src);
void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
/* Projects b - a on the direction dir: out = dir * (dot + r); 0 when the projection is behind by more than r. */
s32 Vec_ProjectOnDir(f32 *from, f32 *to, f32 *dir, f32 *out, f32 reach)
{
    f32 unitDir[4];                   /* a quadword for the VU0 vector functions */
    f32 delta[3];
    f32 along;

    delta[0] = to[0] - from[0];
    delta[1] = to[1] - from[1];
    delta[2] = to[2] - from[2];
    Vec_Copy(unitDir, dir);
    Vec_Normalize(unitDir);
    along = Vec_Dot(unitDir, delta);
    if (along <= 0.0f && -along > reach) {
        return 0;
    }
    along += reach;
    out[0] = along * unitDir[0];
    out[1] = along * unitDir[1];
    out[2] = along * unitDir[2];
    return 1;
}

void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);

/* Where p lies against the capsule of radius r around seg: 2 within r of both ends, 1 of
 * one end, else 1 if within r of the segment between them, 0 outside. */
u8 Capsule_Classify(Segment *seg, f32 *point, f32 radius)
{
    f32 dir[3];
    f32 delta[3];
    f32 *deltaY;
    f32 *deltaZ;
    s32 nearA;
    s32 nearB;
    f32 along;
    u8 inside;

    if (Vec_WithinDist(radius + seg->halfLen, seg->mid, point) == 0) {
        return 0;
    }
    nearA = Vec_WithinDist(radius, seg->a, point) != 0;
    nearB = Vec_WithinDist(radius, seg->b, point) != 0;
    if (nearA && nearB) {
        return 2;
    }
    if (nearA || nearB) {
        return 1;
    }
    Vec_Copy(dir, seg->dir);
    Vec_Normalize(dir);
    deltaY = &delta[1];
    deltaZ = &delta[2];
    delta[0] = point[0] - seg->a[0];
    *deltaY = point[1] - seg->a[1];
    *deltaZ = point[2] - seg->a[2];
    along = Vec_Dot(delta, dir);
    if (along < 0.0f) {
        return 0;
    }
    if (!(along <= 2.0f * seg->halfLen)) {
        return 0;
    }
    inside = delta[0] * delta[0] + *deltaY * *deltaY + *deltaZ * *deltaZ - along * along <= radius * radius;
    return inside;
}

f32 Vec_Dot(f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
f32 Vec_Distance(f32 *a, f32 *b);
asm f32 Math_Sqrtf(f32 x);
/* The distance from p to the segment from a (direction dir, a unit vector, length len) to b. */
f32 Seg_PointDistance(f32 *start, f32 *end, f32 *dir, f32 *point, f32 len)
{
    f32 delta[3];
    f32 along;
    f32 dist;

    delta[0] = point[0] - start[0];
    delta[1] = point[1] - start[1];
    delta[2] = point[2] - start[2];
    along = Vec_Dot(delta, dir);
    if (along < 0.0f) {
        dist = Vec_Length(delta);
    } else if (along > len) {
        dist = Vec_Distance(point, end);
    } else {
        dist = Math_Sqrtf(Vec_Dot(delta, delta) - along * along);
    }
    return dist;
}

f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Copy(f32 *dst, f32 *src);
/* The point of the segment from start (unit direction dir, length len) to end nearest to point,
 * into out. */
void Seg_NearestPoint(f32 *start, f32 *end, f32 *dir, f32 *point, f32 *out, f32 len)
{
    f32 delta[3];
    f32 *deltaY = &delta[1];
    f32 *deltaZ = &delta[2];
    f32 along;

    delta[0] = point[0] - start[0];
    *deltaY = point[1] - start[1];
    *deltaZ = point[2] - start[2];
    along = Vec_Dot(delta, dir);
    if (along < 0.0f) {
        Vec_Copy(out, start);
    } else if (along > len) {
        Vec_Copy(out, end);
    } else {
        delta[0] = along * dir[0];
        *deltaY = along * dir[1];
        *deltaZ = along * dir[2];
        out[0] = start[0] + delta[0];
        out[1] = start[1] + *deltaY;
        out[2] = start[2] + *deltaZ;
    }
}

/* 1 when p is within radius r of c in x-z and, when top > 0, between c.y + bottom and c.y + top. */
s32 Cyl_Contains(f32 *point, f32 *center, f32 radius, f32 top, f32 bottom)
{
    f32 dx;
    f32 dz;

    if (top > 0.0f && (point[1] > center[1] + top || point[1] < center[1] + bottom)) {
        return 0;
    }
    dx = center[0] - point[0];
    dz = center[2] - point[2];
    if (dx * dx + dz * dz <= radius * radius) {
        return 1;
    }
    return 0;
}

f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Normalize(f32 *v);
f32 Math_Absf(f32 x);
/* 1 when pos is within height above a, within radius of the vertical plane through a and b, and between a and b. */
u8 Slab_Contains(f32 *pos, f32 *start, f32 *end, f32 height, f32 radius)
{
    f32 normal[3];
    f32 seg[3];
    f32 toPos[3];
    f32 *toPosY;
    f32 *toPosZ;

    if (pos[1] < start[1] || !(pos[1] <= start[1] + height)) {
        return 0;
    }
    seg[0] = end[0] - start[0];
    seg[1] = end[1] - start[1];
    seg[2] = end[2] - start[2];
    normal[0] = -height * seg[2];
    normal[1] = 0.0f;
    normal[2] = seg[0] * height;
    Vec_Normalize(normal);
    toPosY = &toPos[1];
    toPosZ = &toPos[2];
    toPos[0] = pos[0] - start[0];
    *toPosY = pos[1] - start[1];
    *toPosZ = pos[2] - start[2];
    if (!(Math_Absf(Vec_Dot(normal, toPos)) <= radius)) {
        return 0;
    }
    if (Vec_Dot(seg, toPos) < 0.0f) {
        return 0;
    }
    toPos[0] = pos[0] - end[0];
    *toPosY = pos[1] - end[1];
    *toPosZ = pos[2] - end[2];
    return !(Vec_Dot(seg, toPos) > 0.0f);
}

void Vec_RotateY(f32 *v, f32 a);
f32 Seg_SegDistance(f32 *a0, f32 *a1, f32 *b0, f32 *b1);
/* Capsule cap against a rectangle (half width w, half height h) in the XY
 * plane through c, turned by ang about Y: 1 if they touch. */
u8 Capsule_HitsRect(Capsule *cap, f32 *center, u16 ang, f32 halfHeight, f32 halfWidth)
{
    f32 endA[3];
    f32 endB[3];
    f32 delta[3];
    f32 edgeA[3];
    f32 edgeB[3];
    f32 yhi;
    f32 angle;
    f32 ylo;
    f32 xhi;
    f32 xlo;

    yhi = center[1] + halfHeight;
    ylo = center[1] - halfHeight;
    if ((!(cap->a[1] - cap->radius <= yhi) && !(cap->b[1] - cap->radius <= yhi))
        || (cap->a[1] + cap->radius < ylo && cap->b[1] + cap->radius < ylo)) {
        return 0;
    }
    angle = (6.2831855f * -ang) / 65536.0f;
    delta[0] = cap->a[0] - center[0];
    delta[1] = cap->a[1] - center[1];
    delta[2] = cap->a[2] - center[2];
    Vec_RotateY(delta, angle);
    endA[0] = center[0] + delta[0];
    endA[1] = center[1] + delta[1];
    endA[2] = center[2] + delta[2];
    delta[0] = cap->b[0] - center[0];
    delta[1] = cap->b[1] - center[1];
    delta[2] = cap->b[2] - center[2];
    Vec_RotateY(delta, angle);
    endB[0] = center[0] + delta[0];
    endB[1] = center[1] + delta[1];
    endB[2] = center[2] + delta[2];
    if ((!(endA[2] - cap->radius <= center[2]) && !(endB[2] - cap->radius <= center[2]))
        || (endA[2] + cap->radius < center[2] && endB[2] + cap->radius < center[2])) {
        return 0;
    }
    xhi = center[0] + halfWidth;
    xlo = center[0] - halfWidth;
    if ((!(endA[0] - cap->radius <= xhi) && !(endB[0] - cap->radius <= xhi))
        || (endA[0] + cap->radius < xlo && endB[0] + cap->radius < xlo)) {
        return 0;
    }
    if (((endA[0] <= xhi && !(endA[0] < xlo)) || (endB[0] <= xhi && !(endB[0] < xlo)))
        && ((endA[1] <= yhi && !(endA[1] < ylo)) || (endB[1] <= yhi && !(endB[1] < ylo)))) {
        return 1;
    }
    edgeA[0] = xlo;
    edgeA[1] = ylo;
    edgeA[2] = center[2];
    edgeB[0] = xlo;
    edgeB[1] = yhi;
    edgeB[2] = center[2];
    if (Seg_SegDistance(endA, endB, edgeA, edgeB) <= cap->radius) {
        return 1;
    }
    edgeB[0] = xhi;
    edgeB[1] = ylo;
    if (Seg_SegDistance(endA, endB, edgeA, edgeB) <= cap->radius) {
        return 1;
    }
    edgeA[0] = xhi;
    edgeA[1] = yhi;
    if (Seg_SegDistance(endA, endB, edgeA, edgeB) <= cap->radius) {
        return 1;
    }
    edgeB[0] = xlo;
    edgeB[1] = yhi;
    return Seg_SegDistance(endA, endB, edgeA, edgeB) <= cap->radius;
}

void Vec_Copy(f32 *dst, f32 *src);
void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
f32 Math_Absf(f32 x);
/* Capsule cap (a HitCapsule read as floats: ends a and b, then the radius) against the plane
 * through planePoint with normal nrm: 1 and the crossing point in out when the ends lie on both
 * sides; else 0, with out set to the nearer end pushed back onto the plane when that end is
 * within the radius. */
s32 Seg_ClipPlane(f32 *cap, f32 *planePoint, f32 *nrm, f32 *out)
{
    f32 toA[3];
    f32 toB[3];
    f32 dir[3];
    f32 distA;
    f32 distB;
    f32 absA;
    f32 absB;
    f32 frac;
    f32 x;
    f32 y;
    f32 z;

    toA[0] = cap[0] - planePoint[0];
    toA[1] = cap[1] - planePoint[1];
    toA[2] = cap[2] - planePoint[2];
    toB[0] = cap[3] - planePoint[0];
    toB[1] = cap[4] - planePoint[1];
    toB[2] = cap[5] - planePoint[2];
    Vec_Copy(dir, nrm);
    Vec_Normalize(dir);
    distA = Vec_Dot(dir, toA);
    distB = Vec_Dot(dir, toB);
    absA = Math_Absf(distA);
    absB = Math_Absf(distB);
    if ((distA > 0.0f && distB < 0.0f) || (distA < 0.0f && distB > 0.0f)) {
        frac = absA / (absA + absB);
        x = cap[3] - cap[0];
        y = cap[4] - cap[1];
        z = cap[5] - cap[2];
        x *= frac;
        out[0] = cap[0] + x;
        y *= frac;
        out[1] = cap[1] + y;
        z *= frac;
        out[2] = cap[2] + z;
        return 1;
    }
    if (absA <= absB) {
        if (absA <= cap[6]) {
            frac = -absA;
            x = frac * dir[0];
            y = frac * dir[1];
            z = frac * dir[2];
            out[0] = cap[0] + x;
            out[1] = cap[1] + y;
            out[2] = cap[2] + z;
        }
    } else if (absB <= cap[6]) {
        frac = -absB;
        x = frac * dir[0];
        y = frac * dir[1];
        z = frac * dir[2];
        out[0] = cap[3] + x;
        out[1] = cap[4] + y;
        out[2] = cap[5] + z;
    }
    return 0;
}

s32 Seg_ClipPlane(f32 *s, f32 *p, f32 *nrm, f32 *out);

/* Seg_ClipPlane for a capsule that reaches the band from bottom to top in y: 0 unless one end is
 * at or below top and not both ends are below bottom. */
s32 Seg_ClipYRange(void *cap, f32 top, f32 bottom, void *planePoint, f32 *nrm, void *out)
{
    HitCapsule *seg = cap;
    f32 y = seg->a[1];

    if (!(y <= top || seg->b[1] <= top) || (y < bottom && seg->b[1] < bottom)) {
        return 0;
    }
    return Seg_ClipPlane(cap, planePoint, nrm, out);
}

/* Clears the hit marks of a player or monster (its ObjHead): hitPhase, the 16 hit ids taken
 * (hitIds) and the next slot (hitIdNext). */
static void Obj_ClearHitMarks(void *obj)
{
    ObjHead *head = obj;
    s16 i;

    head->hitPhase = 0;
    for (i = 0; i < 16; i++) {
        head->hitIds[i] = 0;
    }
    head->hitIdNext = 0;
}

void HitId_Reset(void);
extern Em gEms[];
/* Restarts the hit ids and clears the hit marks of the four players and the 20 monsters. */
void Hit_ResetAll(void)
{
    s32 i;

    HitId_Reset();
    for (i = 0; i < SCENE_PLAYERS; i++) {
        Obj_ClearHitMarks(&gPlayers[i]);
    }
    for (i = 0; i < EM_COUNT; i++) {
        Obj_ClearHitMarks(&gEms[i]);
    }
}

void ObjAttach_HitAll(void);
/* Clears the hit and status buildup state of the players and the monsters. */
void Hit_ClearStatusAll(void)
{
    Player *player;
    s32 i;
    s32 j;
    Em *em;
    s32 k;

    for (i = 0, player = gPlayers; i < SCENE_PLAYERS; i++, player++) {
        player->hitReceived = 0;
        player->status5Hit = 0;
        for (j = 0; j < 8; j++) {
            player->hitDamage[j] = 0;
            player->partFlags[j] = 0;
        }
        player->status2Hit = 0;
        player->status1Hit = 0;
        player->status4Hit = 0;
        player->poisonHit = 0;
        player->mashBuildupHit = 0;
        player->status6Hit = 0;
        player->status7Hit = 0;
        player->status8Hit = 0;
        player->hitAttrs = 0;
    }
    for (k = 0, em = gEms; k < EM_COUNT; k++, em++) {
        em->hitReceived = 0;
        em->status5Hit = 0;
        for (j = 0; j < 8; j++) {
            em->hitPartDamage[j] = 0;
            em->partFlags[j] = 0;
        }
        em->status2Hit = 0;
        em->status1Hit = 0;
        em->status4Hit = 0;
        em->poisonHit = 0;
        em->mashBuildupHit = 0;
        em->hitAttrs = 0;
        em->lastAttackKind = 0;
    }
    ObjAttach_HitAll();
}

/* Steps *cur past a hit shape, or past a group header and the zone records it holds when the monster's fxFlags has the bits of its kind. */
void HitList_Next(Em *em, HitShape **cur)
{
    HitShape *shape = *cur;

    if (em->fxFlags & shape->kind) {
        *cur = shape + (shape->zone + 1);
    } else {
        *cur = shape + 1;
    }
}

/* HitList_Next for an attack object's list: a group header (HIT_BONE_GROUP) tests the fxFlags
 * of the monster that owns the object; with no owner, or a player owner, it steps one record. */
void HitList_NextForObj(ObjAttach *attach, HitShape **cur)
{
    HitShape *shape;

    if (attach->ownerIndex == ATTACH_OWNER_NONE || attach->isEm == 0) {
        *cur += 1;
        return;
    }
    shape = *cur;
    if (((Em *)attach->owner)->fxFlags & shape->kind) {
        *cur = shape + (shape->zone + 1);
    } else {
        *cur = shape + 1;
    }
}

s32 Quest_IsVersus(void);
/* Applies the status effects of hit o (elementFlags, elementPower) to its target, a player or a monster
 * read as a Player (the status 6 to 8 fields are the player's): the hit amounts and timers. */
void Em_ApplyHitStatus(Player *target, ObjAttach *attach)
{
    Quest_IsVersus();
    if (attach->elementFlags & ELEMF_EM_STATUS2) {
        target->status2Hit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_POISON) {
        target->poisonHit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_EM_STATUS4) {
        target->status4Hit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_EM_STATUS1) {
        target->status1Hit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_MASH) {
        target->mashBuildupHit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_STATUS6) {
        target->status6Hit = 1800; /* frames: 30 s */
    }
    if (attach->elementFlags & ELEMF_STATUS7) {
        target->status7Hit = 1800; /* frames: 30 s */
    }
    if (attach->elementFlags & ELEMF_STATUS8) {
        target->status8Hit = 10;
    }
}

void PlayerFx_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void HitSpark_SpawnForPlayer(Player *p, u8 type, f32 *pos);
void HitSplash_CreateOnEm(Em *em, ObjAttach *o, u8 a, s16 b, f32 *pos, f32 scale);
s32 Player_CanGuard(Player *p);
s32 HeavyBowgun_HasShield(Player *p);
s32 Player_HasSkill(Player *p, u8 skill);
u32 Player_TestFlag(void *obj, u32 flag);
s32 Player_InGuardArc(u16 a, u16 b);
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
void ShotFlash_CreateAt(f32 *pos, s32 k, u8 c);
void ElementBurst_Create(f32 *pos, s32 k, u16 v, void *owner, f32 scale);
void PlayerFx_CreateAtDefault(f32 *src, u8 k, f32 scale);
/* unit's own declaration: the definition is (Em *em, f32 *pos, u8 k, s16 mode, s16 part, f32 scale); the calls here pass the scale before the hit kind, the order in which the game's code loads them */
void HitSplash_CreateByHitFx(Em *em, f32 *pos, s32 a, s32 b, f32 scale, s16 kind);
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void HitSpark_Spawn(void *owner, s16 kind, u8 type, u8 sub, f32 *pos, f32 scale);
void Effect06_SpawnWeapon(Player *p, s16 type, u8 sub);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
void PlayerFx_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
void PlayerHitBurst_Create(void *arg);
/* The effects and sounds of a hit of attach (owned by attacker) on target (a player or a monster,
 * read as a Player) at out, by the hit effect kind (hitFx & HITFX_KIND_MASK): a guard spark when
 * a player guards it or the target is monster kind 0x1D, else the sparks, splashes and element
 * bursts of the kind, by the hit's hitKind. */
void ObjAttach_HitEffects(ObjAttach *attach, Player *attacker, ObjHead *targetHead, f32 *out)
{
    Player *target = (Player *)targetHead;

    if ((attach->hitFx & HITFX_KIND_MASK) != 0 && attach->area == gScene.area) {
        if (attacker != NULL && attacker->isEm == 0) {
            if (attach->hitFx & HITFX_CRITICAL) {
                PlayerFx_CreateAt(out, 0x10, 0, 1.0f);
            } else if (attach->hitFx & HITFX_WEAK) {
                PlayerFx_CreateAt(out, 0x10, 1, 1.0f);
            }
        }
        if (target->isEm == 1 && target->weaponType == 0x1D) {
            HitSpark_SpawnForPlayer(target, 0, out);
            HitSplash_CreateOnEm((Em *) target, attach, 6, 1, out, 1.0f);
            return;
        }
        if (target->isEm == 0 && Player_CanGuard(target) == 1 && (!(target->hitAttrs & 0x80) || Player_HasSkill(target, SKILL_GUARD_PLUS20) == 1)) {
            if (Player_TestFlag(target, PLF_GUARD) != 0
                || ((Player_HasSkill(target, SKILL_AUTO_GUARD) == 1 || HeavyBowgun_HasShield(target) == 1) && target->weaponOut != 0 && target->prevFreeState != 0)) {
                if (Player_InGuardArc(target->rot[1], target->hitAngle) == 1 && target->stamina >= 75) {
                    HitSpark_SpawnForPlayer(target, 0, out);
                    HitSplash_CreateOnEm((Em *) target, attach, 6, 1, out, 1.0f);
                    return;
                }
            }
        }
        switch (attach->hitFx & HITFX_KIND_MASK) {
        case 28:
            FireBlast_CreateScaled(out, 0, 1.0f);
            return;
        case 18:
            ShotFlash_CreateAt(out, 8, 0);
            return;
        case 1:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 17:
        case 20:
        case 27:
            switch (attach->hitFx & HITFX_KIND_MASK) {
            case 6:
                ShotFlash_CreateAt(out, 7, 1);
                break;
            case 7:
                ShotFlash_CreateAt(out, 7, 2);
                break;
            case 8:
                ShotFlash_CreateAt(out, 7, 3);
                break;
            case 17:
                ShotFlash_CreateAt(out, 7, 5);
                break;
            case 20:
                ShotFlash_CreateAt(out, 7, 6);
                break;
            case 4:
            case 5:
                switch (attach->hitKind) {
                case 0:
                case 2:
                case 3:
                    if ((attach->hitFx & HITFX_KIND_MASK) == 4) {
                        HitSpark_SpawnForPlayer(target, 7, out);
                    } else {
                        HitSpark_SpawnForPlayer(target, 5, out);
                    }
                    HitSpark_SpawnForPlayer(target, 6, out);
                    break;
                }
            case 1:
            case 27:
                if (attacker != NULL && attacker->isEm == 0) {
                    if (attach->elementFlags & ELEMF_FIRE) {
                        ElementBurst_Create(out, 0, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_WATER) {
                        ElementBurst_Create(out, 2, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_THUNDER) {
                        ElementBurst_Create(out, 3, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_ICE) {
                        ElementBurst_Create(out, 8, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_DRAGON) {
                        PlayerFx_CreateAtDefault(out, 0xC, 1.0f);
                    } else if (attach->elementFlags & ELEMF_POISON) {
                        ElementBurst_Create(out, 1, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_EM_STATUS4) {
                        ElementBurst_Create(out, 7, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_EM_STATUS2) {
                        ElementBurst_Create(out, 6, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_STATUS7) {
                        ElementBurst_Create(out, 6, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_STATUS6) {
                        ElementBurst_Create(out, 1, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_STATUS8) {
                        ElementBurst_Create(out, 7, target->hitAngle, attacker, 1.0f);
                    }
                }
                break;
            }
            switch (attach->hitKind) {
            case 1:
                if (attach->ownerIndex != ATTACH_OWNER_NONE) {
                    HitSpark_SpawnForPlayer(target, 0, out);
                    HitSplash_CreateOnEm((Em *) target, attach, 6, 1, out, 1.0f);
                }
                break;
            default:
            case 0:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 0, out, 0.8f);
                HitSplash_CreateByHitFx((Em *) target, out, 2, 0, 0.8f, attach->hitFx & HITFX_KIND_MASK);
                break;
            case 2:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 2, out, 0.8f);
                HitSplash_CreateByHitFx((Em *) target, out, 2, 2, 0.8f, attach->hitFx & HITFX_KIND_MASK);
                break;
            case 3:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 3, out, 1.0f);
                HitSplash_CreateByHitFx((Em *) target, out, 5, 3, 1.0f, attach->hitFx & HITFX_KIND_MASK);
                HitSpark_SpawnForOwner(target, 0, 4, 3, out, 1.0f);
                break;
            }
            if ((attach->hitFx & HITFX_KIND_MASK) == 27 && attach->hitKind != 1) {
                HitSpark_SpawnForPlayer(target, 0xC, out);
                return;
            }
            break;
        case 9:
        case 11:
        case 13:
        case 15:
            ShotFlash_CreateAt(out, 7, 0);
        case 10:
        case 12:
        case 14:
        case 16:
            switch (attach->hitFx & HITFX_KIND_MASK) {
            case 9:
            case 10:
                if (attach->ownerIndex != ATTACH_OWNER_NONE) {
                    HitSpark_SpawnForPlayer(target, 0, out);
                    HitSplash_CreateOnEm((Em *) target, attach, 6, 1, out, 1.0f);
                    return;
                }
                break;
            case 15:
            case 16:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 0, out, 0.8f);
                HitSplash_CreateByHitFx((Em *) target, out, 2, 0, 0.8f, attach->hitFx & HITFX_KIND_MASK);
                return;
            case 11:
            case 12:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 2, out, 0.8f);
                HitSplash_CreateByHitFx((Em *) target, out, 2, 2, 0.8f, attach->hitFx & HITFX_KIND_MASK);
                return;
            case 13:
            case 14:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 3, out, 1.0f);
                HitSplash_CreateByHitFx((Em *) target, out, 5, 3, 1.0f, attach->hitFx & HITFX_KIND_MASK);
                HitSpark_Spawn((u8 *)target, attach->hitFx & HITFX_KIND_MASK, 4, 3, out, 1.0f);
                return;
            }
            break;
        case 2:
            HitSplash_CreateOnEm((Em *) target, attach, 8, 0, out, 1.0f);
            HitSplash_CreateOnEm((Em *) target, attach, 9, 0, out, 1.0f);
            return;
        case 3:
            Effect06_SpawnWeapon(target, 0, 0);
            return;
        case 23:
            if (target->isEm == 0) {
                Effect06_SpawnWeapon(target, 0, 1);
                return;
            }
            break;
        case 21:
            if (target->isEm == 0) {
                Effect06_SpawnWeapon(target, 0, 2);
                return;
            }
            break;
        case 22:
            if (target->isEm == 0) {
                Effect06_SpawnWeapon(target, 0, 3);
                return;
            }
            break;
        case 29:
            if (target->isEm == 0) {
                Effect06_SpawnAtBone(target, 2, 0x19, 10, 4.0f);
                /* (f32 *)&out, as the game: the address of the pointer out itself, so the bone's
                 * position is written over it and the stack after it */
                Em_GetBonePos(target, 10, (f32 *)&out);
                Sound_PlayPlayerSe(target, 0x72, 0, (f32 *)&out, 1, 0);
                return;
            }
            break;
        case 19:
            Effect06_SpawnWeapon(target, 9, 0);
            return;
        case 24:
            if (attacker != NULL && attacker->isEm == 0) {
                if (attach->elementFlags & ELEMF_FIRE) {
                    ElementBurst_Create(out, 0, target->hitAngle, attacker, 1.5f);
                } else if (attach->elementFlags & ELEMF_WATER) {
                    ElementBurst_Create(out, 2, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_THUNDER) {
                    ElementBurst_Create(out, 3, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_DRAGON) {
                    PlayerFx_CreateAtDefault(out, 0xC, 1.0f);
                } else if (attach->elementFlags & ELEMF_ICE) {
                    ElementBurst_Create(out, 8, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_POISON) {
                    ElementBurst_Create(out, 1, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_EM_STATUS4) {
                    ElementBurst_Create(out, 7, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_EM_STATUS2) {
                    ElementBurst_Create(out, 6, target->hitAngle, attacker, 1.2f);
                }
            }
            HitSplash_CreateOnEm((Em *) target, attach, 0, 2, out, 0.8f);
            HitSplash_CreateByHitFx((Em *) target, out, 2, 2, 0.8f, 12);
            return;
        case 25:
            if (target->isEm == 0) {
                PlayerFx_CreateOnPlayerDefault(target, 1, 1.0f);
                return;
            }
            break;
        case 26:
            if (target->isEm == 0) {
                PlayerHitBurst_Create(target);
                return;
            }
            break;
        case 30:
            ShotFlash_CreateAt(out, 7, 8);
            return;
        case 31:
            ShotFlash_CreateAt(out, 9, 1);
            return;
        case 32:
            ShotFlash_CreateAt(out, 9, 2);
            return;
        case 33:
            ShotFlash_CreateAt(out, 9, 3);
            break;
        }
    }
}

u32 Player_TestFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);
void Player_StartNextHit(Player *unused0, u8 unused1);
/* Counts down the player's own hit window (hitDelay, then hitFrames) by its hitPhase (0 off,
 * 1 waiting, 2 open), as ObjAttach_UpdateTimers does for the attack objects. When the window
 * closes with a next row set (nextRow), Player_StartNextHit is called with it (it does nothing)
 * and the window reopens. */
void Player_UpdateHitTimers(Player *player)
{
    if (Player_TestFlag(player, 0x800) != 0) {
        player->hitPhase = 1;
        Player_ClearFlag(player, 0x40000);
    }
    if (player->hitStop != 0) {
        return;
    }
    if (Player_TestFlag(player, PLF_HIT_WINDOW) == 0) {
        player->hitPhase = 0;
        Player_ClearFlag(player, 0x40000);
    }
    switch (player->hitPhase) {
    case 0:
        break;
    case 1:
        if (player->hitDelay-- != 0) {
            break;
        }
        player->hitDelay = 0;
        player->hitPhase++;
    case 2:
        if (player->hitFrames == 0) {
            if (player->nextRow != 0) {
                Player_StartNextHit(player, player->nextRow);
                Player_ClearFlag(player, 0x40000);
                if (player->hitDelay == 0) {
                    player->hitPhase = 2;
                    player->hitFrames--;
                } else {
                    player->hitPhase = 1;
                }
            } else {
                player->hitPhase = 0;
            }
        } else {
            player->hitFrames--;
        }
        break;
    }
}

s32 Spheres_Touch(f32 ra, f32 rb, f32 *a, f32 *b, f32 *out);
s32 Player_TestFlags(void *obj, s32 mask);
s32 HitShape_Place(f32 *pos, HitShape *e, HitCapsule *cap, HitSphere *sph, f32 s);
s32 Capsule_FromPoints(f32 *a, f32 *b, HitShape *c, HitCapsule *out, f32 s);
s32 BoneHit_Place(ObjHead *o, HitShape *def, HitCapsule *c, HitSphere *s);
u8 Capsule_ContactCapsule(Capsule *a, Capsule *b, f32 *out);
extern Player gPlayers[];
extern Em gEms[];
/* Tests attach's attack shapes (hitList) against the hurt shapes (hurtList) of the other
 * active objects of its area: not those of its own side's owners, not one already hit
 * (wasHit), not an ATKF_SHIELDED one unless attach is a monster's with ATKF_PIERCES. The first
 * object hit gets wasHit and hitBy (the player or monster at the HIT object's own idx, as the
 * game indexes it; NULL when attach has no owner). The trap, hold, roar and wind attack kinds
 * don't hit objects. */
void ObjAttach_HitObjects(ObjAttach *attach)
{
    HitShape *list;
    HitShape *elist;
    f32 out[3];
    HitCapsule segA;
    HitCapsule segB;
    HitSphere sphA;
    HitSphere sphB;
    Capsule capA;
    Capsule capB;
    ObjAttach *other;
    void *owner;
    void *eowner;
    s16 shapeA;
    s16 shapeB;
    s16 hit;

    other = gObjAttachList;
    switch (attach->attackKind) {
    case ATTACK_TRAP_A:
    case ATTACK_TRAP_B:
    case ATTACK_HOLD:
    case ATTACK_ROAR:
    case ATTACK_ROAR_WEAK:
    case ATTACK_ROAR_STRONG:
    case ATTACK_WIND:
    case ATTACK_WIND_WEAK:
        return;
    }
    if (attach->isEm == 0) {
        owner = &gPlayers[attach->idx];
    } else {
        owner = &gEms[attach->idx];
    }
    for (; other != NULL; other = other->next) {
        if (attach->area != other->area || other->active == 0) {
            continue;
        }
        if (other->isEm == attach->isEm && attach->ownerIndex != ATTACH_OWNER_NONE && other->ownerIndex != ATTACH_OWNER_NONE) {
            continue;
        }
        if (other->hurtList == 0 || other->wasHit != 0) {
            continue;
        }
        if (Player_TestFlags(other, ATKF_SHIELDED) != 0 && attach->isEm != 0 && Player_TestFlags(attach, ATKF_PIERCES) == 0) {
            continue;
        }
        list = attach->hitList;
        if (other->isEm == 0) {
            eowner = &gPlayers[other->idx];
        } else {
            eowner = &gEms[other->idx];
        }
        while (list->bone != HIT_BONE_END) {
            switch (list->bone) {
            case HIT_BONE_AT_POS:
                shapeA = HitShape_Place(attach->pos, list, &segA, &sphA, attach->hitScale);
                break;
            case HIT_BONE_SEGMENT:
                shapeA = Capsule_FromPoints(attach->pos, attach->pos2, list, &segA, attach->hitScale);
                break;
            case HIT_BONE_GROUP:
                HitList_NextForObj(attach, &list);
                continue;
            default:
                shapeA = BoneHit_Place(owner, list, &segA, &sphA);
                break;
            }
            if (shapeA != 0) {
                Capsule_Copy(&segA, &capA);
            }
            elist = other->hurtList;
            while (elist->bone != HIT_BONE_END) {
                switch (elist->bone) {
                case HIT_BONE_AT_POS:
                    shapeB = HitShape_Place(other->pos, elist, &segB, &sphB, 1.0f);
                    break;
                case HIT_BONE_SEGMENT:
                    shapeB = Capsule_FromPoints(other->pos, other->pos2, elist, &segB, 1.0f);
                    break;
                case HIT_BONE_GROUP:
                    HitList_NextForObj(other, &elist);
                    continue;
                default:
                    shapeB = BoneHit_Place(eowner, elist, &segB, &sphB);
                    break;
                }
                if (shapeB != 0) {
                    Capsule_Copy(&segB, &capB);
                }
                if (shapeA == 1) {
                    if (shapeB == 1) {
                        hit = Capsule_ContactCapsule(&capA, &capB, out);
                    } else {
                        hit = (u8)Capsule_SphereContactOnSphere(&capA, sphB.pos, out, sphB.radius);
                    }
                } else if (shapeB == 1) {
                    hit = (u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
                } else {
                    hit = (u8)Spheres_Touch(sphA.radius, sphB.radius, sphA.pos, sphB.pos, out);
                }
                if (hit != 0) {
                    other->wasHit = 1;
                    if (attach->ownerIndex == ATTACH_OWNER_NONE) {
                        other->hitBy = NULL;
                    } else if (attach->isEm == 0) {
                        other->hitBy = &gPlayers[other->idx];
                    } else {
                        other->hitBy = &gEms[other->idx];
                    }
                    goto next; /* leaves both shape loops at once: the next object */
                }
                elist++;
            }
            list++;
        }
    next:;
    }
}

void ObjAttach_HitObjects(ObjAttach *o);
s32 Player_TestFlags(void *obj, s32 mask);
s32 Players_AnySide(void);
s32 Side_Match(u8 a, u8 b, u8 c);
void ObjAttach_TryHitTarget(ObjAttach *o, Player *p, ObjHead *e);
/* For every active attack object: clears didHit, tests the other objects while its window is
 * open (ObjAttach_HitObjects), then tries the monsters and the players of its area it may hit:
 * not its owner unless ATKF_HITS_OWNER, not one that already took its hit id, not one that took
 * a hit this frame. The trap kinds skip a monster in the air or dead, the hold kind also one
 * under the sand or in a move it doesn't break off; neither hits players. */
void ObjAttach_HitAll(void)
{
    ObjAttach *attach;
    void *owner;
    Em *em;
    s32 i;
    s32 k;
    s32 j;
    Player *player;

    for (attach = gObjAttachList; attach != NULL; attach = attach->next) {
        if (attach->active == 0) {
            continue;
        }
        attach->didHit = 0;
        if (attach->hitPhase == 2) {
            ObjAttach_HitObjects(attach);
        }
        if (attach->ownerIndex == ATTACH_OWNER_NONE) {
            owner = NULL;
        } else if (attach->isEm == 0) {
            owner = &gPlayers[attach->idx];
        } else {
            owner = &gEms[attach->idx];
        }
        for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->area == attach->area && Player_TestFlags(attach, ATKF_NO_EM) == 0) {
                if (attach->ownerIndex == ATTACH_OWNER_NONE || owner != em || Player_TestFlags(attach, ATKF_HITS_OWNER) != 0) {
                    for (k = 0; k < 16; k++) {
                        if (attach->hitId == em->hitIds[k]) {
                            goto next_em; /* hit by this attack already; a break and a test after the loop compile differently */
                        }
                    }
                    if (em->hitReceived == 0) {
                        switch (attach->attackKind) {
                        case ATTACK_HOLD:
                            if (em->stance == EM_STANCE_UNDER || em->stateArmor > 0) {
                                break;
                            }
                        case ATTACK_TRAP_A:
                        case ATTACK_TRAP_B:
                            if (em->stance == EM_STANCE_AIR || em->hp <= 0) {
                                break;
                            }
                        default:
                            ObjAttach_TryHitTarget(attach, owner, (ObjHead *)em);
                            break;
                        }
                    }
                }
            }
        next_em:;
        }
        for (player = gPlayers, j = 0; j < gScene.playerCount; j++, player++) {
            if (player->active != 0 && player->area == attach->area && Player_TestFlags(attach, ATKF_NO_PLAYER) == 0) {
                if (!(attach->attrs & 1) || (Players_AnySide() != 0 && Side_Match(attach->side, player->side, 1) != 1)) {
                    if (attach->ownerIndex == ATTACH_OWNER_NONE || owner != player || Player_TestFlags(attach, ATKF_HITS_OWNER) != 0) {
                        for (k = 0; k < 16; k++) {
                            if (attach->hitId == player->hitIds[k]) {
                                goto next_pl; /* as next_em */
                            }
                        }
                        if (player->hitReceived == 0) {
                            switch (attach->attackKind) {
                            default:
                                ObjAttach_TryHitTarget(attach, owner, (ObjHead *)player);
                                break;
                            case ATTACK_TRAP_A:
                            case ATTACK_TRAP_B:
                            case ATTACK_HOLD:
                                break;
                            }
                        }
                    }
                }
            }
        next_pl:;
        }
    }
}

s32 Obj_InCurrentArea(void *obj); /* the calls in this file test only the low byte: (u8) casts */
void ObjAttach_HitTarget(ObjAttach *o, Player *p, ObjHead *e);
/* Runs ObjAttach_HitTarget for attach on target (a player or a monster) when the target is
 * active and in the current area, the attacker is not in hit stop (or there is none), and
 * attach's delay is over and its window open (hitPhase 2). */
void ObjAttach_TryHitTarget(ObjAttach *attach, Player *attacker, ObjHead *target)
{
    if (target->active != 0 && (attach->ownerIndex == ATTACH_OWNER_NONE || attacker->hitStop == 0)) {
        if (attach->delay == 0 && (u8)Obj_InCurrentArea(target) != 0) {
            switch (attach->hitPhase) {
            case 2:
                ObjAttach_HitTarget(attach, attacker, target);
                break;
            }
        }
    }
}

/* 1 when target is a monster another console runs (Em.remote): the damage functions then mark
 * the hit but leave the damage to that console. Debug switch 0x2A makes it 0 always. */
s32 ObjAttach_TargetIsRemote(Em *target)
{
    if (Debug_IsOn(0x2A) != 0) {
        return 0;
    }
    if (target->isEm != 0 && target->remote != 0) {
        return 1;
    }
    return 0;
}

/* Which of shape's open states hold now: bit 1 when it has a wind bit (HITSHAPE_WIND_*) and
 * em's windTimer runs, bit 2 when it has a hermit bit (HITSHAPE_HERMIT_*) and its hermitTimer
 * runs. ObjAttach_DamageEm deflects an attack on them. */
static u8 ObjAttach_HitFlags(Em *em, HitShape *shape)
{
    u8 flags = shape->openFlags;
    u8 open = 0;

    if ((flags & (HITSHAPE_WIND_OPEN | HITSHAPE_WIND_ONLY)) && em->windTimer > 0) {
        open |= HIT_OPEN_WIND;
    }
    if ((flags & (HITSHAPE_HERMIT_OPEN | HITSHAPE_HERMIT_ONLY)) && (s8)em->hermitTimer > 0) {
        open |= HIT_OPEN_HERMIT;
    }
    return open;
}

s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 ObjAttach_TargetIsRemote(Em *t);
static u8 ObjAttach_HitFlags(Em *p, HitShape *e);
void ObjAttach_LinkTarget(ObjAttach *o, Player *p);
void ObjAttach_NextMultiHit(ObjAttach *o);
s32 Player_TestFlags(void *obj, s32 mask);
s32 Side_Match(u8 a, u8 b, u8 c);
s32 Player_HasSongEffect(Player *pl, s16 kind);
s32 Player_HasSkill(Player *p, u8 skill);
void LongSword_AddGauge(Player *p, s16 n);
void Player_AddSharpness(Player *p, s16 n);
s32 Debug_IsOn(s32 unused0);
f64 fptodp(f32 x);
void Vib_StartLocal(void *player, s32 i);
void Em_ApplyHitStatus(Player *t, ObjAttach *o);
void Player_SetFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);
void Em_EndRage(Em *em);
extern EmHitzone **gEmHitzoneTables[];
extern s8 gSeVolumeByType[];
extern f32 gHitKindMvFactors[];
extern f32 gSharpnessRawMods[];
extern f32 gSharpnessElemMods[];
extern f32 gSwordGaugeHitFactors[];
extern s16 gSharpnessLossByHit[];
extern s16 gHitSeIds[];

s32 printf(const char *fmt, ...);
/* A hit by object o (of player p) on monster t at out, hit shape elist of list: the reaction of p
 * (by the motion value against the hit zone), the sounds, the damage and the element. */
void ObjAttach_DamageEm(ObjAttach *attach, Player *attacker, Em *em, HitShape *list, HitShape *elist, f32 *out)
{
    f32 motionValue;
    f32 atk;
    f32 elemFactor;
    f32 fire;
    f32 thunder;
    f32 ice;
    f32 water;
    f32 dragon;
    f32 cutRate;
    f32 impactRate;
    f32 pct;
    f32 powerF;
    s16 damage;
    s16 extraSe;
    u8 mvKind;
    u8 shellOpen;
    u8 shellHit;
    u8 deflected;
    EmHitzone *zoneRow;
    u8 byPlayer;
    u8 attackId;
    u8 crit;
    u8 oldAttrs;
    u8 noBounce;

    byPlayer = 0;
    deflected = 0;
    shellOpen = 0;
    attach->hitKind = 0;
    motionValue = 1.0f;
    elemFactor = 1.0f;
    atk = 1.0f;
    fire = 0.0f;
    crit = 0;
    zoneRow = gEmHitzoneTables[em->type][(s8)em->hitzoneSet];
    zoneRow += elist->zone;
    thunder = 0.0f;
    water = 0.0f;
    ice = 0.0f;
    dragon = 0.0f;
    attackId = attach->hitId;
    if (!(attach->multiHit & 0xF)) {
        if (Player_TestFlags(attach, ATKF_WIND_DEFLECTS) != 0 && (ObjAttach_HitFlags(em, elist) & HIT_OPEN_WIND)) {
            switch (em->type) {
            case EM_KIND_LUNASTRA:
            case EM_KIND_TEOSTRA:
                attach->state = ATTACH_DEFLECTED_AURA;
                break;
            default:
                attach->state = ATTACH_DEFLECTED;
                break;
            }
            deflected = 1;
        } else if (Player_TestFlags(attach, ATKF_SHELL_DEFLECTS) != 0 && (ObjAttach_HitFlags(em, elist) & HIT_OPEN_HERMIT)) {
            attach->state = ATTACH_DEFLECTED;
            deflected = 1;
        } else {
            attach->state = ATTACH_HIT;
        }
        attach->step = 0;
        attach->didHit = 1;
    } else {
        ObjAttach_NextMultiHit(attach);
    }
    attach->targetEm = em;
    attach->targetPart = elist;
    if (attach->ownerIndex != ATTACH_OWNER_NONE && attacker != NULL) {
        ObjAttach_LinkTarget(attach, attacker);
        attacker->hitPending = elist->zone + 1;
        if (Player_TestFlags(attach, ATKF_OWNER_CONTACT) != 0) {
            attacker->hitStopAdd = attach->hitStop;
            attacker->contact = em;
            attacker->contactAttach = attach;
        }
        if (Player_TestFlags(attach, ATKF_REPORT_CONTACT) != 0) {
            attacker->hitStopAdd = attach->hitStop;
            attacker->contact = em;
            attacker->contactAttach = attach;
            attacker->contactHit = 1;
        }
        if (ObjAttach_HitFlags(em, elist) & HIT_OPEN_HERMIT) {
            attacker->hitPending |= 0x80;
            shellOpen = 1;
            /* (f32 *)&out, as the game: the address of the pointer out, not the hit point */
            Em_PlaySeChecked(em, 0x2D, 0, (f32 *)&out, gSeVolumeByType[em->type], 0);
        }
        if (attacker->isEm != 0) {
            Sound_PlayPlayerSe(em, 0x60, 0, out, 1, 0);
        } else {
            byPlayer = 1;
            if (attach->elementFlags & ELEMF_FIRE) {
                fire = attach->elementPower;
            }
            if (attach->elementFlags & ELEMF_WATER) {
                water = attach->elementPower;
            }
            if (attach->elementFlags & ELEMF_THUNDER) {
                thunder = attach->elementPower;
            }
            if (attach->elementFlags & ELEMF_ICE) {
                ice = attach->elementPower;
            }
            if (attach->elementFlags & ELEMF_DRAGON) {
                dragon = attach->elementPower;
            }
            if (Player_TestFlags(attach, ATKF_WEAPON) != 0) {
                if (attach->hitMarks & 1) {
                    crit = 1;
                }
                if (attach->mvFramesA == 0xFF) {
                    mvKind = 0;
                } else if (attach->mvFramesA != 0) {
                    mvKind = 1;
                } else if (attach->mvFramesB != 0) {
                    if (list->zone != 1) {
                        mvKind = 2;
                    } else {
                        mvKind = 3;
                    }
                } else {
                    mvKind = 0;
                }
                motionValue = gHitKindMvFactors[mvKind + attacker->sharpnessLevel * 4];
                atk = attacker->attackFactor;
                if (attacker->weaponType == WEAPON_BOW) {
                    elemFactor = 1.0f;
                } else {
                    motionValue *= gSharpnessRawMods[attacker->sharpnessLevel];
                    elemFactor = gSharpnessElemMods[attacker->sharpnessLevel];
                }
                cutRate = attacker->attackPercent / 100.0f;
                impactRate = attacker->attackPercentBase / 100.0f;
                cutRate *= zoneRow->cut / 100.0f;
                impactRate *= zoneRow->impact / 100.0f;
                if (!(cutRate < impactRate)) {
                    motionValue *= cutRate;
                } else {
                    motionValue *= impactRate;
                }
                shellHit = shellOpen;
                if (shellHit != 0 && !(motionValue <= 0.1f)) {
                    motionValue = 0.1f;
                }
                if (!(motionValue < 1.0f)) {
                    attacker->hitKind = 3;
                    attacker->hitStopAdd = attach->hitStop * 3;
                } else if (!(motionValue < 0.45f)) {
                    attacker->hitKind = 2;
                    attacker->hitStopAdd = 0;
                    attacker->hitSlowTimer = attach->hitStop;
                } else {
                    if (!(motionValue < 0.25f)) {
                        attacker->hitKind = 0;
                    } else {
                        attacker->hitKind = 1;
                    }
                    attacker->hitStopAdd = 0;
                }
                noBounce = 0;
                attach->hitKind = attacker->hitKind;
                if (Player_HasSongEffect(attacker, 0) == 1 && attacker->songEffect[0] >= 2) {
                    noBounce = 1;
                }
                if (Player_HasSkill(attacker, SKILL_AFFINITY_PLUS10_NO_BOUNCE) != 0 || Player_HasSkill(attacker, SKILL_AFFINITY_PLUS15_NO_BOUNCE) != 0) {
                    noBounce = 1;
                }
                if (noBounce != 0 && attacker->hitKind == 1) {
                    attacker->hitKind = 0;
                }
                if (attacker != NULL) {
                    if (attach->swordGaugeAdd > 0) {
                        LongSword_AddGauge(attacker, attach->swordGaugeAdd * gSwordGaugeHitFactors[attacker->hitKind]);
                    }
                }
                if (attacker->weaponType != WEAPON_BOW) {
                    if (shellHit != 0) {
                        Player_AddSharpness(attacker, gSharpnessLossByHit[attacker->hitKind] * 5);
                    } else {
                        Player_AddSharpness(attacker, gSharpnessLossByHit[attacker->hitKind]);
                    }
                }
                switch (attach->hitSe) {
                case 0xFF:
                    break;
                case 0:
                default:
                    Sound_PlayPlayerSe(em, gHitSeIds[attacker->weaponType * 8 + attacker->hitKind * 2], 0, out, 1, 0);
                    extraSe = gHitSeIds[attacker->weaponType * 8 + attacker->hitKind * 2 + 1];
                    if (extraSe != 0) {
                        Sound_PlayPlayerSe(em, extraSe, 0, out, 1, 0);
                    }
                    break;
                case 0xA:
                    Sound_PlayPlayerSe(em, 0x82, 0, out, 1, 0);
                    break;
                case 1:
                    Sound_PlayPlayerSe(em, 0x20, 0, out, 1, 0);
                    break;
                }
            } else {
                motionValue = 1.0f;
                if (Player_TestFlags(attach, ATKF_SHOT) != 0) {
                    motionValue *= zoneRow->shot / 100.0f;
                    if (Debug_IsOn(0xAE) != 0) {
                        if (attach->hitFx & HITFX_CRITICAL) {
                            printf(TXT(0x004AB080) /* debug message: a ranged attack hit (critical) */);
                        } else if (attach->hitFx & HITFX_WEAK) {
                            printf(TXT(0x004AB0B0) /* debug message: a ranged attack hit (critical failed) */);
                        } else {
                            printf(TXT(0x004AB0F0) /* debug message: a ranged attack hit */);
                        }
                        printf(TXT(0x004AB110) /* debug message: the player's attack modifier */, fptodp(1.0f));
                        printf(TXT(0x004AB130) /* debug message: the hit zone's result against a shot */, fptodp(motionValue));
                    }
                } else if (Debug_IsOn(0xAE) != 0) {
                    printf(TXT(0x004AB150) /* debug message: another player attack hit */);
                }
                switch (attach->hitSe) {
                case 0xFF:
                    break;
                case 2:
                case 3:
                    Sound_PlayPlayerSe(em, 0x5D, 0, out, 1, 0);
                    break;
                case 4:
                    Sound_PlayPlayerSe(em, 0x5C, 0, out, 1, 0);
                    break;
                case 5:
                    break;
                case 6:
                    Sound_PlayPlayerSe(em, 0x4D, 0, out, 1, 0);
                    break;
                case 1:
                    Sound_PlayPlayerSe(em, 0x20, 0, out, 1, 0);
                    break;
                case 7:
                    /* the sword and shield's sound for hit kind 3 (gHitSeIds' row: weaponType * 8 + hitKind * 2) */
                    Sound_PlayPlayerSe(em, gHitSeIds[38], 0, out, 1, 0);
                    break;
                case 8:
                    Sound_PlayPlayerSe(em, 0x80, 0, out, 1, 0);
                    break;
                case 9:
                    Sound_PlayPlayerSe(em, 0x81, 0, out, 1, 0);
                    break;
                case 0xA:
                    Sound_PlayPlayerSe(em, 0x82, 0, out, 1, 0);
                    break;
                case 0xB:
                    Sound_PlayPlayerSe(em, 0x83, 0, out, 1, 0);
                    break;
                case 0xC:
                    Sound_PlayPlayerSe(em, 0x84, 0, out, 1, 0);
                    break;
                }
            }
            fire *= zoneRow->fire / 100.0f;
            water *= zoneRow->water / 100.0f;
            thunder *= zoneRow->thunder / 100.0f;
            ice *= zoneRow->ice / 100.0f;
            dragon *= zoneRow->dragon / 100.0f;
            fire = elemFactor * (fire + water + thunder + ice + dragon);
            Vib_StartLocal(attacker, 0);
        }
    }
    if (attach->attackKind == ATTACK_PAINT && em->painted != 0xFF && (u8)ObjAttach_TargetIsRemote(em) == 0 && deflected == 0) {
        em->painted = 1;
        em->paintTimer = EM_PAINT_FRAMES;
    }
    em->hitIds[em->hitIdNext] = attackId;
    em->hitIdNext = (em->hitIdNext + 1) & 0xF;
    if ((u8)ObjAttach_TargetIsRemote(em) == 0 && Side_Match(attach->side, ((ObjHead *)em)->side, 1) != 1) { /* the unit read the side as the shared u8 (Em.side is s8) */
        powerF = attach->power;
        if (attach->powerPct > 0) {
            pct = attach->powerPct;
            pct /= 100.0f;
            powerF *= pct;
        }
        switch (em->hitReactMode) {
        case EM_HIT_REACT_WAKE:
            if ((s8)em->guardTimer <= 0 && byPlayer == 0) {
                powerF *= 3.0f;
            }
            break;
        case EM_HIT_REACT_NO_FLINCH:
            powerF *= 1.1f;
            break;
        case EM_HIT_REACT_STATUS:
            powerF *= 1.1f;
            break;
        case EM_HIT_REACT_FLINCH:
        case 1:
            break;
        }
        if (attach->attackKind == ATTACK_HEAL) {
            damage = -1.0f * attach->power;
        } else {
            damage = atk * (powerF * motionValue);
        }
        if (attach->isEm != 0 && em->type != 0x1D) {
            damage /= 4;
        }
        if (damage == 0 && attach->power != 0) {
            damage = 1;
        }
        damage += (s16)fire;
        if (deflected == 0) {
            if (crit != 0) {
                em->partFlags[elist->part & 7] |= 1;
            }
            if (attach->hitMarks & 2) {
                em->partFlags[elist->part & 7] |= 2;
            }
            if (attach->hitMarks & 8) {
                em->partFlags[elist->part & 7] |= 8;
            }
            if (attach->elementFlags & ELEMF_FIRE) {
                em->partFlags[elist->part & 7] |= 4;
            }
            if (attach->elementFlags & ELEMF_WATER) {
                em->partFlags[elist->part & 7] |= 0x10;
            }
            if (attach->elementFlags & ELEMF_ICE) {
                em->partFlags[elist->part & 7] |= 0x20;
            }
            if (attach->elementFlags & ELEMF_THUNDER) {
                em->partFlags[elist->part & 7] |= 0x40;
            }
            if (attach->elementFlags & ELEMF_DRAGON) {
                em->partFlags[elist->part & 7] |= 0x80;
            }
            em->hitPartDamage[elist->part & 7] += damage;
            Em_ApplyHitStatus((Player *)em, attach);
            if (attach->isEm == 0) {
                em->status5Hit += (s16)(attach->stun * (zoneRow->stun / 100.0f));
            }
            if (attach->ownerIndex != ATTACH_OWNER_NONE && attacker->isEm == 0) {
                em->reactValue[attacker->index & 3] += damage;
            }
            em->hitStopAdd = attach->hitStop;
            em->hitReceived = 2;
            em->flinchPart = elist->part;
            if (Player_TestFlags(attach, ATKF_OWNER_CONTACT) == 0) {
                Player_SetFlag(em, 0x80000001);
            } else {
                Player_ClearFlag(em, 0x80000001);
            }
            em->hitAttach = attach;
            em->lastAttackKind = attach->attackKind;
            em->hitForce = attach->hitForce;
            oldAttrs = em->hitAttrs;
            em->hitAttrs = oldAttrs | attach->attrs;
            if (attach->attackKind == ATTACK_CALM) {
                Em_EndRage(em);
            }
        }
    }
}

/* The rest of this unit's .rodata: objects no code loads (data may point to them), kept as
 * the game's bytes. */
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB180);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB218);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB228);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB238);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB258);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB268);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB278);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB2B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB2C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB408);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB418);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB550);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB560);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB580);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB608);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB618);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB620);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB630);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB640);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB678);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB680);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB690);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB698);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB708);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB718);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB728);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB738);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB748);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB758);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB768);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB788);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB798);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB808);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB818);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB828);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB838);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB850);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB860);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB870);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB880);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB890);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB900);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB910);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB920);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB930);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB940);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB950);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB958);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB968);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB978);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB988);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABAD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABAF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC000);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC008);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC010);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC038);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC048);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC068);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC070);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC078);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC080);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC088);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC098);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC108);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC118);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC128);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC138);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC150);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC188);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC208);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC288);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC298);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC308);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC348);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC358);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC440);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC450);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC500);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC520);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC588);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC5B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC5C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC5D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC5E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC600);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC620);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC638);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC680);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC708);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC718);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC728);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC738);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC748);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC758);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC768);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC788);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC798);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC808);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC818);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC828);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC838);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC878);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC888);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC898);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC908);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC928);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC938);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC948);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC958);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC968);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC978);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC988);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACCA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACCB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACCC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACCE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD008);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD038);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD048);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD068);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD078);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD088);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD098);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD108);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD118);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD128);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD138);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD148);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD158);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD180);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD330);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD370);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD390);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD430);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD468);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD478);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD488);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD508);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD518);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD538);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD548);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD568);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD578);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD588);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD608);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD618);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD628);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD638);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD678);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD688);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD698);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD700);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD720);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD740);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD760);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD790);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD810);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD830);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD880);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD8A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD8C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD8D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD8F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD908);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD930);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD948);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD960);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD980);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADDD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADDE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADEB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADEF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADFD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE000);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE040);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE070);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE090);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE0B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE0F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE110);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE130);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE150);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE188);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE198);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE208);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE218);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE228);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE238);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE248);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE258);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE268);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE278);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE288);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE298);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE308);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE330);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE348);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE358);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE370);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE390);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE408);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE430);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE440);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE450);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE468);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE478);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE488);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE498);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE500);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE508);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE518);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE520);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE538);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE548);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE550);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE560);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE568);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE578);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE580);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE588);

void ObjAttach_LinkTarget(ObjAttach *o, Player *p);
void ObjAttach_NextMultiHit(ObjAttach *o);
s32 Player_IsLocal(void *obj);
s32 Player_CanGuard(Player *p);
s32 HeavyBowgun_HasShield(Player *p);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Player_InGuardArc(u16 a, u16 b);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
/* A hit by object o (owned by player p) on player t at pos: the reaction,
 * the damage (attack factor, defence, element resistance; halved in versus
 * quests) and the hit sound. */
void ObjAttach_DamagePlayer(ObjAttach *attach, Player *attacker, Player *victim, f32 *pos)
{
    f32 atk;
    f32 powerF;
    f32 defense;
    f32 factor;
    u8 stun;
    s16 damage;
    u8 guard;
    u8 attackId;
    u8 elemMarks;

    attackId = attach->hitId;
    if (!(attach->multiHit & 0xF)) {
        attach->state = ATTACH_HIT;
        attach->step = 0;
        attach->didHit = 1;
    } else {
        ObjAttach_NextMultiHit(attach);
    }
    attach->targetEm = (Em *) victim;
    attach->targetPart = 0;
    attach->hitKind = 0;
    atk = 1.0f;
    if (attach->ownerIndex != ATTACH_OWNER_NONE) {
        ObjAttach_LinkTarget(attach, attacker);
        attacker->hitPending = 1;
        if (Player_TestFlags(attach, ATKF_OWNER_CONTACT) != 0) {
            attacker->hitStopAdd = attach->hitStop;
        }
        if (Player_TestFlags(attach, ATKF_REPORT_CONTACT) != 0) {
            attacker->contactHit = 1;
        }
        attacker->hitKind = 0;
        attacker->contact = victim;
        attacker->contactAttach = attach;
        atk = attacker->attackFactor;
    }
    victim->hitIds[victim->hitIdNext] = attackId;
    victim->hitIdNext = (victim->hitIdNext + 1) & 0xF;
    victim->partFlags[0] |= attach->hitMarks;
    if ((u8)ObjAttach_TargetIsRemote((Em *)victim) != 0) {
        return;
    }
    if (Player_IsLocal(victim) == 1) {
        switch (attach->attackKind) {
        case ATTACK_GIVE_ATTACK_UP:
            victim->attackDrug = 3;
            break;
        case ATTACK_GIVE_DEFENSE_UP:
            victim->defenseDrug = 6;
            break;
        case ATTACK_CURE_POISON:
            victim->poison = 0;
            break;
        case ATTACK_CURE_STATUS6:
            victim->status6Timer = 0;
            break;
        }
    }
    if (Side_Match(victim->side, attach->side, 1) != 1 || attach->attackKind != ATTACK_HEAL) {
        if ((victim->side == 0 && attach->isEm != 0) || attach->ownerIndex == ATTACH_OWNER_NONE || attach->attackKind == ATTACK_HEAL
            || Side_Match(victim->side, attach->side, 0) == 0 || (attach->side == 0 && attach->isEm != 0)) {
            Em_ApplyHitStatus(victim, attach);
            if (attach->attackKind == ATTACK_HEAL) {
                victim->hitDamage[0] -= attach->power;
            } else {
                powerF = (f32)attach->power * atk;
                if (attach->powerPct > 0) {
                    factor = attach->powerPct;
                    factor /= 100.0f;
                    powerF *= factor;
                }
                defense = victim->defenseFactor;
                factor = 80.0f;
                powerF = powerF - powerF * defense / (defense + factor);
                factor = 1.0f;
                elemMarks = attach->hitMarks;
                if (elemMarks & 4) {
                    factor = (100.0f - victim->resist[0]) / 100.0f;
                } else if (elemMarks & 0x10) {
                    factor = (100.0f - victim->resist[1]) / 100.0f;
                } else if (elemMarks & 0x40) {
                    factor = (100.0f - victim->resist[2]) / 100.0f;
                } else if (elemMarks & 0x20) {
                    factor = (100.0f - victim->resist[3]) / 100.0f;
                } else if (elemMarks & 0x80) {
                    factor = (100.0f - victim->resist[4]) / 100.0f;
                }
                stun = attach->stun;
                damage = powerF * factor;
                if (Quest_IsVersus() == 1 && victim->isEm == 0 && attach->isEm == 0) {
                    if (damage > 0) {
                        damage /= 2;
                        if (damage <= 0) {
                            damage = 1;
                        }
                    }
                    if (stun > 0) {
                        stun = stun / 2;
                        if (stun <= 0) {
                            stun = 1;
                        }
                    }
                }
                victim->hitDamage[0] += damage;
                victim->status5Hit += stun;
            }
        }
    }
    victim->target = attacker;
    victim->hitPos[0] = pos[0];
    victim->hitPos[1] = pos[1];
    victim->hitPos[2] = pos[2];
    victim->hitReceived = 2;
    if (Player_CanGuard(victim) == 0) {
        guard = 1;
    } else if ((Player_HasSkill(victim, SKILL_AUTO_GUARD) == 1 || HeavyBowgun_HasShield(victim) == 1) && victim->weaponOut != 0 && victim->prevFreeState != 0) {
        guard = 0;
    } else if (Player_TestFlag(victim, PLF_GUARD) != 0) {
        guard = 0;
    } else {
        guard = 1;
    }
    if (Player_TestFlag(victim, PLF_GUARD) == 0 || Player_InGuardArc((u16)victim->rot[1], victim->hitAngle) == 0
        || victim->stamina < 75 || guard == 1) {
        switch (attach->hitSe) {
        case 0xFF:
            break;
        case 0:
        default:
            Sound_PlayPlayerSe(victim, 0x21, 0, pos, 1, 0);
            break;
        case 1:
            Sound_PlayPlayerSe(victim, 0x20, 0, pos, 1, 0);
            break;
        }
    }
    victim->flinchPart = 0;
    victim->hitAttach = attach;
    victim->lastAttackKind = attach->attackKind;
    victim->hitForce = attach->hitForce;
    victim->hitAttrs |= attach->attrs;
}

u16 Vec_Heading(f32 *a, f32 *b);
/* Sets the hit target's hitAngle: the heading from `from` to `to` plus a quarter turn when
 * attach's attrs has 0x10, else attach's yaw plus its hitYawOfs (degrees). */
void ObjAttach_SetHitHeading(ObjAttach *attach, Player *target, f32 *from, f32 *to)
{
    if (attach->attrs & 0x10) {
        target->hitAngle = Vec_Heading(from, to) + 0x4000;
    } else {
        target->hitAngle = attach->yaw + (u16)(s32)(0.5f + 65536.0f * attach->hitYawOfs / 360.0f);
    }
}

void Player_BodyCapsule(Player *p, HitCapsule *cap, f32 r);
void HitList_Next(Em *p, HitShape **cur);
void ObjAttach_SetHitHeading(ObjAttach *o, Player *p, f32 *a, f32 *b);
void ObjAttach_DamagePlayer(ObjAttach *o, Player *p, Player *t, f32 *out);
void ObjAttach_HitEffects(ObjAttach *o, Player *p, ObjHead *e, f32 *out);
void ObjAttach_DamageEm(ObjAttach *o, Player *p, Em *t, HitShape *list, HitShape *elist, f32 *out);
extern HitShape *gEmBoneHits[];
/* Tests attach's attack shapes against target: a player's body capsule (a fainted player is hit
 * only by ATTACK_REVIVE, and only it hits one), or each hurt shape of a monster's kind
 * (gEmBoneHits[type]), skipping the shapes this attack can't hit (the HITSHAPE_* flags: closed
 * wind or hermit shapes, never-hit shapes, the trap and hold shapes). The first hit sets the
 * hit's heading and runs ObjAttach_DamagePlayer or ObjAttach_DamageEm, then ObjAttach_HitEffects. */
void ObjAttach_HitTarget(ObjAttach *attach, Player *attacker, ObjHead *target)
{
    HitShape *list;
    HitShape *elist;
    f32 out[3];
    HitCapsule segA;
    HitCapsule segB;
    HitSphere sphA;
    HitSphere sphB;
    Capsule capA;
    Capsule capB;
    s16 shapeA;
    s16 shapeB;
    s16 hit;
    u8 shapeAtk;
    u8 shapeAtk2;
    u8 kind;
    Em *em;

    if (target->noHitTimer != 0) {
        return;
    }
    if (attach->hitList == 0) {
        return;
    }
    list = attach->hitList;
    if (target->isEm == 0) {
        if (target->stateGroup == PL_GROUP_FAINT) {
            if (attach->attackKind != ATTACK_REVIVE) {
                return;
            }
        } else if (attach->attackKind == ATTACK_REVIVE) {
            return;
        }
        Player_BodyCapsule((Player *)target, &segB, 40.0f);
        Capsule_Copy(&segB, &capB);
    }
    while (list->bone != HIT_BONE_END) {
        switch (list->bone) {
        case HIT_BONE_AT_POS:
            shapeA = HitShape_Place(attach->pos, list, &segA, &sphA, attach->hitScale);
            break;
        case HIT_BONE_SEGMENT:
            shapeA = Capsule_FromPoints(attach->pos, attach->pos2, list, &segA, attach->hitScale);
            break;
        case HIT_BONE_GROUP:
            HitList_NextForObj(attach, &list);
            continue;
        default:
            shapeA = BoneHit_Place((ObjHead *)attacker, list, &segA, &sphA);
            break;
        }
        if (shapeA != 0) {
            Capsule_Copy(&segA, &capA);
        }
        if (target->isEm == 0) {
            hit = (shapeA == 1) ? Capsule_ContactCapsule(&capA, &capB, out) : (s32)(u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
            if (hit != 0) {
                if (shapeA == 1) {
                    ObjAttach_SetHitHeading(attach, (Player *)target, capB.mid, capA.mid);
                } else {
                    ObjAttach_SetHitHeading(attach, (Player *)target, capB.mid, sphA.pos);
                }
                ObjAttach_DamagePlayer(attach, attacker, (Player *)target, out);
                ObjAttach_HitEffects(attach, attacker, target, out);
                return;
            }
        } else {
            em = (Em *)target;
            elist = gEmBoneHits[em->type];
            while (elist->bone != HIT_BONE_END) {
                switch (elist->bone) {
                case HIT_BONE_AT_POS:
                case HIT_BONE_SEGMENT:
                    shapeB = HitShape_Place(target->pos, elist, &segB, &sphB, 1.0f);
                    break;
                case HIT_BONE_GROUP:
                    HitList_Next((Em *)target, &elist);
                    continue;
                default:
                    shapeB = BoneHit_Place(target, elist, &segB, &sphB);
                    break;
                }
                if ((elist->openFlags & HITSHAPE_WIND_ONLY) && (em->windTimer == 0 || Player_TestFlags(attach, ATKF_WIND_DEFLECTS) == 0)) {
                    goto next;
                }
                if ((elist->openFlags & HITSHAPE_HERMIT_ONLY) && (s8)em->hermitTimer == 0) {
                    goto next;
                }
                if (elist->offFlags & HITSHAPE_NO_HIT) {
                    goto next;
                }
                shapeAtk = elist->atkFlags;
                if (shapeAtk & HITSHAPE_TRAP_ONLY) {
                    if (em->remote != 0 || (attach->attackKind != ATTACK_TRAP_A && attach->attackKind != ATTACK_TRAP_B)) {
                        goto next;
                    }
                }
                shapeAtk2 = elist->atkFlags2;
                if (shapeAtk2 & HITSHAPE2_HOLD_ONLY) {
                    if (em->remote != 0 || attach->attackKind != ATTACK_HOLD) {
                        goto next;
                    }
                }
                kind = attach->attackKind;
                if (kind == 6 || kind == 9) {
                    if (!(shapeAtk & HITSHAPE_TRAP_HIT) || em->remote != 0) {
                        goto next;
                    }
                }
                if (kind == 0x13) {
                    if (!(shapeAtk2 & HITSHAPE2_HOLD_HIT) || em->remote != 0) {
                        goto next;
                    }
                }
                if (kind == ATTACK_WIND || kind == ATTACK_WIND_WEAK) {
                    if (!(shapeAtk & HITSHAPE_WIND_HIT)) {
                        goto next;
                    }
                }
                if (shapeB != 0) {
                    Capsule_Copy(&segB, &capB);
                }
                if (shapeA == 1) {
                    if (shapeB == 1) {
                        hit = Capsule_ContactCapsule(&capA, &capB, out);
                    } else {
                        hit = (u8)Capsule_SphereContactOnSphere(&capA, sphB.pos, out, sphB.radius);
                    }
                } else if (shapeB == 1) {
                    hit = (u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
                } else {
                    hit = (u8)Spheres_Touch(sphA.radius, sphB.radius, sphA.pos, sphB.pos, out);
                }
                if (hit != 0) {
                    if (shapeA == 1) {
                        if (shapeB == 1) {
                            ObjAttach_SetHitHeading(attach, (Player *)target, capB.mid, capA.mid);
                        } else {
                            ObjAttach_SetHitHeading(attach, (Player *)target, sphB.pos, capA.mid);
                        }
                    } else if (shapeB == 1) {
                        ObjAttach_SetHitHeading(attach, (Player *)target, capB.mid, sphA.pos);
                    } else {
                        ObjAttach_SetHitHeading(attach, (Player *)target, sphB.pos, sphA.pos);
                    }
                    ObjAttach_DamageEm(attach, attacker, (Em *)target, list, elist, out);
                    ObjAttach_HitEffects(attach, attacker, target, out);
                    return;
                }
            next: /* the shape is skipped; written as a for loop with continue, the loop compiles differently */
                elist++;
            }
        }
        list++;
    }
}

void ObjAttach_LinkTarget(ObjAttach *attach, Player *attacker)
{
    attacker->hitTimers = &attach->hitDelay;
}

u8 HitId_Next(void);

/* A multi-hit attack's next hit: one fewer hit left (multiHit's low 4 bits); when the delay
 * (the high 4 bits) is set, the window reopens after it with a new hit id. */
void ObjAttach_NextMultiHit(ObjAttach *attach)
{
    u8 bits = attach->multiHit;

    attach->multiHit = (bits & 0xF0) | (s8)((s8)(bits & 0xF) - 1);
    if (attach->multiHit >> 4) {
        attach->hitDelay = attach->multiHit >> 4;
        attach->hitPhase = 1;
        attach->hitId = HitId_Next();
    }
}

void ObjAttach_LoadOwnedRow(ObjAttach *o, void *owner, s32 i);
/* Counts down the object's hit window by hitPhase (0 off, 1 waiting hitDelay frames, 2 open for
 * hitFrames). When it closes and nextRow is set, the owner player's row nextRow is loaded
 * (ObjAttach_LoadOwnedRow) and the window reopens; while open, mvFramesA counts down (0xFF:
 * never), then mvFramesB: ObjAttach_DamageEm picks the motion value row by them. */
void ObjAttach_UpdateTimers(ObjAttach *attach)
{
    switch (attach->hitPhase) {
    case 0:
        break;
    case 1:
        if (attach->hitDelay-- != 0) {
            break;
        }
        attach->hitDelay = 0;
        attach->hitPhase++;
    case 2:
        if (attach->hitFrames == 0) {
            if (attach->nextRow != 0) {
                ObjAttach_LoadOwnedRow(attach, &gPlayers[attach->idx], attach->nextRow);
                attach->hitPhase = 1;
                if (attach->hitDelay == 0) {
                    attach->hitPhase = 2;
                    attach->hitFrames--;
                } else {
                    attach->hitPhase = 1;
                    break;
                }
            } else {
                attach->hitPhase = 0;
                attach->hitDelay = 0;
                attach->hitFrames = 0;
                break;
            }
        } else {
            attach->hitFrames--;
        }
        if (attach->mvFramesA != 0 && attach->mvFramesA != 0xFF) {
            attach->mvFramesA--;
            break;
        }
        if (attach->mvFramesB != 0) {
            attach->mvFramesB--;
        }
        break;
    }
}

/* ObjAttach_UpdateTimers for every active attack object whose delay is over. */
void ObjAttach_UpdateAllTimers(void)
{
    ObjAttach *attach;

    attach = gObjAttachList;
    if (attach != NULL) {
        do {
            if (attach->active != 0 && attach->delay == 0) {
                ObjAttach_UpdateTimers(attach);
            }
            attach = attach->next;
        } while (attach != NULL);
    }
}

void HitId_Reset(void)
{
    gScene.hitId = 0;
}

/* Steps the id counter SceneState.hitId (gScene.hitId is the same byte), skipping 0; returns it. */
u8 HitId_Next(void)
{
    SceneState *scene = (SceneState *)&gScene;

    gScene.hitId++;
    if (gScene.hitId == 0) {
        scene->hitId = 1;
    }
    return scene->hitId;
}

void LightSet_SetColors(void *obj, s32 n);
void LightSet_FadeColors(void *obj);
void Vec_AnglesToRad(s32 *in, f32 *out);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Gfx_SetTexFilter(s32 a);
struct EmBone;
void Bone_StoreMatrices(struct EmBone *b);
void Mtx_Identity(f32 *m);
void Tex_Prepare(s32 a, s32 b);
void Draw_ApplyStates(u32 f);
void Draw_SetDefaultState(void);
s32 Model_Draw(u32 h, s32 flags);
/* Draws a monster sub_main runs itself (the slot's owner): its world matrix, then each material set
 * of its model with its materials' alpha set to em->alpha. Kind 0 draws only the sets meshShown
 * marks; kind 1 draws them all, its materials' alpha 0 where meshShown[j] is 0 (j: the material's
 * place in the set, as the game indexes it). */
void Em_Draw(DrawSlot *slot)
{
    f32 rad[4];
    f32 mtx[4][4];
    f32 ident[4][4];
    f32 scale[4][4];
    s32 i;
    Em *em;
    s32 partCount;
    s32 j;
    ModelMatSet *part;
    ModelMat *mats;
    Model *model;

    em = slot->owner;
    model = em->model;
    if (em->active == 0 || em->visible == 0) {
        return;
    }
    LightSet_SetColors(em, 1);
    LightSet_FadeColors(em);
    Vec_AnglesToRad(&em->rotX, rad);
    Mtx_Scale(scale[0], em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, mtx[0]);
    Mtx_SetTrans(mtx[0], em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(mtx[0], scale[0]);
    Mtx_Copy((f32 *)em->worldMtx, mtx[0]);
    Gfx_SetTexFilter(1);
    Gs_SetState(RS_ALPHA_REF, 0);
    part = model->matSets;
    partCount = model->matSetCount;
    mats = model->mats;
    Bone_StoreMatrices(model->bones);
    Mtx_Identity(ident[0]);
    Gs_SetState(RS_UV_MTX, (s32)ident);
    Tex_Prepare(10, em->type * 20 + 0xA3);
    for (i = 0; i < partCount; i++, part++) {
        if (em->type != 0 || (s8)em->meshShown[i] != 0) {
            Gs_SetState(RS_COLOR2, -1);
            if (part->id != -1) {
                for (j = 0; j < part->count; j++) {
                    ModelMat *mat = &mats[part->mat[j]];

                    switch (em->type) {
                    case 1:
                        if ((s8)em->meshShown[j] == 0) {
                            mat->alpha = 0.0f;
                        } else {
                            mat->alpha = em->alpha;
                        }
                        break;
                    default:
                        mat->alpha = em->alpha;
                        break;
                    }
                    Gs_SetState(RS_MATERIAL(j), (s32)mat);
                }
                Draw_ApplyStates(part->flags);
                Model_Draw(part->id, 0);
            }
        }
    }
    Draw_SetDefaultState();
    Gs_SetState(RS_ALPHA_REF, 0);
}

extern void *gEmNopHandlers[];
void Em_Draw(DrawSlot *slot);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
/* Sets up a monster: its handler table, the table's first handler, and a draw slot drawn by Em_Draw. */
void Em_InitHandlers(Em *em)
{
    em->handlers = (EmVt *)gEmNopHandlers;
    em->handlers->init(em);
    if (em->visible != 0) {
        em->drawSlotId = DrawSlot_Alloc();
        if (em->drawSlotId != -1) {
            em->drawSlot = DrawSlot_Get(em->drawSlotId);
            em->drawSlot->owner = em;
            em->drawSlot->draw = Em_Draw;
        }
    }
}

void EmSlot_SetupModel(Em *em);
void Em_InitCollMask(Em *em);
void Em_InitHandlers(Em *em);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_SetMotion(Player *p, s32 motion, s32 a, s16 b, s32 layer);
void Player_BeginLayerMotion(Player *p, u16 motion, s16 a, s32 layer); /* unit's own declaration: u16 and s16, as Player_PrepareMotions' calls need (the definition takes s32) */
void Player_UpdateAreaPos(void *obj);
extern f32 gStartPos[][3];
extern s16 gEmVariantShownMeshes[][2];
extern u16 *gEmVariantMeshes[];
/* Sets up a monster sub_main runs itself (taskPhase 0 of Em_RunByState; ownTask = 1): its model,
 * scale, the area's start position (kind 1: moved by 200 * variant), the meshes it shows
 * (meshShown, from gEmVariantShownMeshes or gEmVariantMeshes by variant), its handlers and draw
 * slot (Em_InitHandlers) and first motions. Kind 0 has two motion layers and is not flagged isEm. */
void Em_SetupKind(Em *em)
{
    u16 *list;
    u16 mesh;
    s32 i;

    EmSlot_SetupModel(em);
    em->taskPhase++;
    em->isEm = 0;
    em->ownTask = 1;
    em->hitPhase = 0;
    em->visible = 1;
    em->motion[0].step = 2.0f;
    em->motion[1].step = 2.0f;
    em->motion[2].step = 2.0f;
    em->motion[3].step = 2.0f;
    em->motionSpeed = 1.0f;
    if (em->type == 0) {
        em->motionLayerCount = 2;
    } else {
        em->isEm = 1;
        em->motionLayerCount = 1;
    }
    em->scaleX = 1.0f;
    em->scaleY = 1.0f;
    em->scaleZ = 1.0f;
    em->rotX = 0;
    em->rotY = 0;
    em->rotZ = 0;
    em->motionTime = 0;
    em->area = gScene.area;
    em->posX = gStartPos[em->area][0];
    em->posY = gStartPos[em->area][1];
    em->posZ = gStartPos[em->area][2];
    if (em->type == 1) {
        em->posX += 200.0f * em->variant;
        em->posZ += 200.0f * em->variant;
    }
    em->alpha = 1.0f;
    em->hideShadow = 0;
    em->fieldCollide = 1;
    em->motionId[0] = 0;
    em->motionId[1] = 0;
    em->motionId[2] = 0;
    em->motionId[3] = 0;
    Em_InitCollMask(em);
    for (i = 0; i < (s32)sizeof(em->meshShown); i++) {
        em->meshShown[i] = 0;
    }
    switch (em->type) {
    case 0:
        em->meshShown[gEmVariantShownMeshes[em->variant][0]] = 1;
        if (em->variant * 2 + 1 != 0xFF) {
            em->meshShown[gEmVariantShownMeshes[em->variant][1]] = 1;
        }
        break;
    case 1:
        list = gEmVariantMeshes[em->variant];
        mesh = *list;
        while (mesh != 0xFFFF) {
            em->meshShown[mesh] = 1;
            list++;
            mesh = *list;
        }
        break;
    }
    Em_InitHandlers(em);
    if (em->type == 0) {
        Player_SetMotionPair((Player *)em, 1, 0, 0);
        Player_BeginLayerMotion((Player *)em, em->motionFrame[0], em->motionBlend[0], 0);
        Player_BeginLayerMotion((Player *)em, em->motionFrame[1], em->motionBlend[1], 1);
    } else {
        Player_SetMotion((Player *)em, 0x3E9, 0, 0, 0);
        Player_BeginLayerMotion((Player *)em, em->motionFrame[0], em->motionBlend[0], 0);
    }
    Player_UpdateAreaPos(em);
}

void Player_TickTimers(Player *p);
void Player_UpdateHitStop(void *);
void Player_ForceState(void *obj, s32 state, s32 sub);
void Em_CollideField(Em *em, u8 keep);
struct TriHdr;
s32 Em_GetFloorInfoEx(Em *em, f32 *pos, struct TriHdr *, f32 *out, f32 *depth);
u16 Rand_Next(s32 a);
void Player_PrepareMotions(Player *p);
extern u16 *gEmVariantMeshes[];
/* The state update of a monster sub_main runs itself (states 0 and 1 of group 0: idle and turn
 * in place by random times; kind 1 in state 0 steps its variant, of 6, when square is pressed and
 * shows that variant's meshes), then its matrix, motion speed and floor. */
void Em_UpdateState(Em *em)
{
    s32 type;
    u16 i;
    u16 *list;
    u16 mesh;

    em->noPushTimer = 10;
    Player_TickTimers((Player *) em);
    Player_UpdateHitStop(em);
    em->prevPosX = em->posX;
    em->prevPosY = em->posY;
    em->prevPosZ = em->posZ;
    type = em->type;
    switch (em->stateGroup) {
    case 0:
        switch (em->state) {
        case 0:
            if (type == 0) {
                if (em->step == 0) {
                    em->step++;
                    em->timer = (u8)(Rand_Next(1) + 60);
                    if (em->motionId[0] != 1) {
                        Player_SetMotionPair((Player *)em, 1, 4, 0);
                    }
                } else if (--em->timer <= 0) {
                    Player_ForceState(em, 0, 1);
                }
            } else {
                em->timer++;
                if (gPadState[0].pressed & PAD_SQUARE) {
                    em->variant++;
                    em->variant = em->variant % 6;
                    for (i = 0; i < 32; i++) {
                        em->meshShown[i] = 0;
                    }
                    list = gEmVariantMeshes[em->variant];
                    while ((mesh = *list) != 0xFFFF) {
                        list++;
                        em->meshShown[mesh] = 1;
                    }
                }
            }
            break;
        case 1:
            if (em->step == 0) {
                em->step++;
                em->stateVar0 = Rand_Next(1) & 1;
                em->timer = (u8)(Rand_Next(1) + 60);
                Player_SetMotionPair((Player *)em, 3, 4, 0);
            } else if (--em->timer <= 0) {
                Player_ForceState(em, 0, 0);
            } else if (em->stateVar0 == 0) {
                em->rotY += Rand_Next(1) & 0x1FF;
            } else {
                em->rotY -= Rand_Next(1) & 0x1FF;
            }
            break;
        }
        break;
    default:
        em->stateGroup = 0;
        em->state = 0;
        em->step = 0;
        break;
    }
    em->rotX = (u16)em->rotX;
    em->rotY = (u16)em->rotY;
    em->rotZ = (u16)em->rotZ;
    Mtx_FromAnglesYXZ(&em->rotX, em->rotMtx[0]);
    em->motion[0].step = 2.0f * em->motionSpeed;
    em->motion[1].step = 2.0f * em->motionSpeed;
    em->motion[2].step = 2.0f * em->motionSpeed;
    em->motion[3].step = 2.0f * em->motionSpeed;
    Player_PrepareMotions((Player *)em);
    Em_CollideField(em, 0);
    Em_GetFloorInfoEx(em, &em->posX, (struct TriHdr *)&em->floorKind, &em->floorY, &em->surfaceY);
    em->posY = em->floorY;
}

/* Steps the monster's taskPhase (Em_RunByState: from the end to the free). */
void Em_NextTaskPhase(Em *em)
{
    em->taskPhase++;
}

/* Frees the monster's slot (Em_RunByState's last phase). */
void Em_StateFree(Em *em)
{
    EmSlot_Free(em);
}

void Em_SetupKind(Em *em);
void Em_UpdateState(Em *em);
void Player_UpdateAreaPos(void *obj);
void Em_RunMotionFx(Em *em);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
/* The frame of a monster sub_main runs itself, by taskPhase (0 set up, 1 run, 2 end, 3 free): runs
 * the handlers' hook, puts its draw slot at its position in the ordering table when it is visible
 * in the current area, and updates its area position. Returns 1 once freed. */
s32 Em_RunByState(Em *em)
{
    switch (em->taskPhase) {
    case 0:
        Em_SetupKind(em);
        Em_RunMotionFx(em);
        Player_UpdateAreaPos(em);
        return 0;
    case 1:
        Em_UpdateState(em);
        break;
    case 2:
        Em_NextTaskPhase(em);
        break;
    case 3:
        Em_StateFree(em);
        return 1;
    }
    Em_RunMotionFx(em);
    if (em->visible != 0 && em->area == gScene.area) {
        em->drawSlot->pos[0] = em->posX;
        em->drawSlot->pos[1] = em->posY;
        em->drawSlot->pos[2] = em->posZ;
        OT_AddByDepth(gDrawOt32, em->drawSlot, 0x20, 0);
    }
    Player_UpdateAreaPos(em);
    return 0;
}

s32 Debug_IsOn(s32 unused0);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
/* unit's own declaration: the definition takes (Player *p, s32 frame, s32 n, s32 part); the game's
 * Player_PrepareMotions passes the frame as a u16 and the second value as an s16, so its calls
 * truncate them first */
void Player_BeginLayerMotion(Player *p, u16 motion, s16 a, s32 layer); /* unit's own declaration: u16 and s16, as the calls below need (the definition takes s32) */
s32 Player_UpdateMotion(Player *p);
/* Unless debug switch 0x28 is on: the player's rotation matrix, then (while not in hit stop)
 * starts the motions pending on layers 0 and 1 and advances them. */
void Player_PrepareMotions(Player *pl)
{
    if (Debug_IsOn(0x28) == 0) {
        Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
        if (pl->hitStop == 0) {
            if (pl->motionStarted[0] == 0) {
                pl->motionStarted[0]++;
                Player_BeginLayerMotion(pl, pl->motionFrame[0], pl->motionBlend[0], 0);
            }
            if (pl->motionStarted[1] == 0 && pl->motionLayerCount > 1) {
                pl->motionStarted[1]++;
                Player_BeginLayerMotion(pl, pl->motionFrame[1], pl->motionBlend[1], 1);
            }
            Player_UpdateMotion(pl);
        }
    }
}

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Bone_UpdateWorldScaledRoot(struct EmBone *bones, f32 *m);
/* The monster's world matrix (scale, rotation, position) into worldMtx, and its skeleton's. */
void Em_UpdateWorldMatrix(Em *em)
{
    Model *model = em->model;
    f32 mtx[4][4];
    f32 scale[4][4];

    Mtx_Scale(scale[0], em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, mtx[0]);
    Mtx_SetTrans(mtx[0], em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(mtx[0], scale[0]);
    Mtx_Copy((f32 *)em->worldMtx, mtx[0]);
    Bone_UpdateWorldScaledRoot(model->bones, mtx[0]);
}

/* Calls the monster's motion-effects handler (EmVt.motionFx); for these monsters
 * gEmNopHandlers' Em_NopHandlerB. */
void Em_RunMotionFx(Em *em)
{
    em->handlers->motionFx(em);
}

void Em_NopHandlerA(void)
{
}

void Em_NopHandlerB(void)
{
}

/* The update of a shadow (STAGEOBJ_KIND_SHADOW, Shadow_Spawn), by state: set up, run, end,
 * free. */
void Shadow_Update(StageObj *shadow)
{
    switch (shadow->state) {
    case 0:
        Shadow_Setup(shadow);
        break;
    case 1:
        Shadow_Run(shadow);
        break;
    case 2:
        Shadow_End(shadow);
        break;
    case 3:
        Shadow_Delete(shadow);
        break;
    }
}

s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
void Shadow_Draw(DrawSlot *s);
/* Sets up a shadow: its draw slots (SHADOW_PARTS: five, one per part; the others one, from the second
 * pool when keep is set), drawn by Shadow_Draw; with no slot free it is freed. Then runs its
 * first frame. */
void Shadow_Setup(StageObj *shadow)
{
    EffectParts *parts;
    s16 i;

    shadow->state++;
    shadow->var0 = 0;
    shadow->visible = 1;
    shadow->draw = NULL;
    switch (shadow->type) {
    case SHADOW_PARTS:
        parts = (EffectParts *)shadow->buf;
        for (i = 0; i < 5; i++) {
            parts->slot[i] = DrawSlot_Alloc();
            if (parts->slot[i] != -1) {
                parts->draw[i] = DrawSlot_Get(parts->slot[i]);
                parts->draw[i]->owner = shadow;
                parts->draw[i]->part = i;
                parts->draw[i]->draw = Shadow_Draw;
            } else {
                parts->draw[i] = NULL;
            }
            shadow->node = NULL;
        }
        break;
    default:
        if (shadow->keep) {
            shadow->slot = DrawSlotB_Alloc();
        } else {
            shadow->slot = DrawSlot_Alloc();
        }
        if (shadow->slot != -1) {
            if (shadow->keep) {
                shadow->node = DrawSlotB_Get(shadow->slot);
            } else {
                shadow->node = DrawSlot_Get(shadow->slot);
            }
            shadow->node->owner = shadow;
            shadow->node->draw = Shadow_Draw;
            break;
        }
        Shadow_Delete(shadow);
        return;
    }
    Shadow_Run(shadow);
}

s32 Obj_InCurrentArea(void *obj);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
f32 AreaGrid2_GetFloorY(f32 *pos);
void Vec_RotateSelf(f32 *v, f32 *m);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Em_BonePos(void *obj, s16 bone, f32 *out);
extern f32 gCamMatrix[];
extern s16 gShadowPartBones[];
extern f32 gShadowPartScales[];
extern f32 gShadowSizesLobby[][2];
extern f32 gShadowSizes[][2];
/* Shadow_Update's running state: follows the owner (SHADOW_PLAYER: the position of its
 * part 10 on the floor, and in the town only once its model is loaded; SHADOW_PARTS: five bones, each
 * part put 5 above the floor and drawn when its bone is 8 or more above it; the others: bone 2
 * on the floor), pushes the draw slot towards the camera for a monster (by -10 times the size
 * table's first value plus twice the kind, as the game computes it), and puts it in the ordering
 * table. The end state follows once the owner is gone. */
void Shadow_Run(StageObj *shadow)
{
    f32 offset[3];
    EffectParts *parts;
    f32 *sizes;
    s16 i;
    Player *pl;
    f32 groundY;

    pl = shadow->owner;
    switch (shadow->type) {
    case SHADOW_PLAYER:
        if (gScene.inLobby != 0) {
            if (pl->active == 0 || (s8)gLobby->pcLoaded[pl->index] == 0) { /* read as a signed byte, as the game does */
                return;
            }
        } else if (pl->active == 0) {
            return;
        }
        shadow->pos[0] = pl->partRecs[10]->m1[12];
        shadow->pos[1] = pl->floorY;
        shadow->pos[2] = pl->partRecs[10]->m1[14];
        goto draw; /* the player's shadow shares the default case's draw: the game's code jumps into it */
    case SHADOW_PARTS:
        parts = (EffectParts *)shadow->buf;
        if (pl->active == 0) {
            shadow->state++;
            return;
        }
        if (pl->visible == 0 || (u8)Obj_InCurrentArea(pl) == 0) {
            return;
        }
        for (i = 0; i < 5; i++) {
            if (parts->draw[i] != NULL) {
                Mtx_GetTrans(parts->pos[i], Em_GetBone(pl, gShadowPartBones[i])->world);
                groundY = AreaGrid2_GetFloorY(parts->pos[i]);
                if (!(parts->pos[i][1] < 8.0f + groundY)) {
                    parts->pos[i][1] = 5.0f + groundY;
                    offset[0] = 0.0f;
                    offset[1] = 0.0f;
                    offset[2] = pl->scale[0] * (-10.0f * gShadowPartScales[i]);
                    Vec_RotateSelf(offset, gCamMatrix);
                    parts->draw[i]->pos[0] = offset[0] + parts->pos[i][0];
                    parts->draw[i]->pos[1] = offset[1] + parts->pos[i][1];
                    parts->draw[i]->pos[2] = offset[2] + parts->pos[i][2];
                    OT_AddByDepth(gDrawOtSorted, parts->draw[i], 0x40, 0);
                }
            }
        }
        return;
    default:
        if (pl->active == 0) {
            shadow->state++;
            return;
        }
        Em_BonePos(pl, 2, shadow->pos);
        shadow->pos[1] = pl->floorY;
    draw:
        if (gScene.inLobby == 0 || pl->ownTask != 1 || pl->weaponType != 3) { /* the town's monster of kind 3 is drawn in any area */
            if ((u8)Obj_InCurrentArea(pl) == 0) {
                return;
            }
        }
        if (pl->visible == 0) {
            return;
        }
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 0.0f;
        if (pl->isEm != 0) {
            if (gScene.inLobby != 0) {
                sizes = gShadowSizesLobby[0];
            } else {
                sizes = gShadowSizes[0];
            }
            offset[2] = pl->scale[0] * (-10.0f * (*sizes + pl->weaponType * 2));
            Vec_RotateSelf(offset, gCamMatrix);
        }
        shadow->node->pos[0] = shadow->pos[0] + offset[0];
        shadow->node->pos[1] = shadow->pos[1] + offset[1];
        shadow->node->pos[2] = shadow->pos[2] + offset[2];
        OT_AddByDepth(gDrawOtSorted, shadow->node, 0x40, 0);
        break;
    }
}

void DrawSlot_Free(s32 id);
void DrawSlotB_Free(s32 id);
/* Frees an effect object's draw slots (SHADOW_PARTS: the five of its buffer). */
void Shadow_End(StageObj *shadow)
{
    EffectParts *parts;
    s16 i;

    shadow->state++;
    switch (shadow->type) {
    case SHADOW_PARTS:
        parts = (EffectParts *)shadow->buf;
        for (i = 0; i < 5; i++) {
            if (parts->draw[i] != NULL) {
                DrawSlot_Free(parts->slot[i]);
            }
        }
        break;
    default:
        if (shadow->node != NULL) {
            if (shadow->keep) {
                DrawSlotB_Free(shadow->slot);
            } else {
                DrawSlot_Free(shadow->slot);
            }
        }
        break;
    }
}

void Shadow_Delete(StageObj *shadow)
{
    StageObj_Free(shadow);
}


/* Sets the materials of the set matSet (from mats) for the draw, each with its alpha = the owner's
 * alpha times scale (the shadow's brightness). */
void Shadow_SetMaterials(ObjHead *owner, ModelMat *mats, ModelMatSet *matSet, f32 scale)
{
    s32 i;

    for (i = 0; i < matSet->count; i++) {
        ModelMat *mat = &mats[matSet->mat[i]];

        mat->alpha = owner->alpha * scale;
        Gs_SetState(RS_MATERIAL(i), (s32)mat);
    }
}

f32 AreaGrid2_GetFloorY(f32 *pos);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
/* Walks a bone tree (child, then sibling): each bone's world matrix with its height set to the
 * ground's (clamped to base +-30) plus 5 + 0.3 * n, times the bone's skin matrix, into its matrix
 * slot (RS_MTX_SLOT(index)). Lays a shadow's bones on the ground (SHADOW_GROUND_BONES). */
void Bone_SnapToGround(EmBone *bone, u8 n, f32 base)
{
    f32 pos[3];
    f32 out[4][4];
    f32 mtx[4][4];
    f32 *y;
    f32 groundY;

    Mtx_Copy(mtx[0], bone->world);
    y = &mtx[3][1];
    pos[0] = mtx[3][0];
    pos[1] = *y;
    pos[2] = mtx[3][2];
    groundY = AreaGrid2_GetFloorY(pos);
    if (groundY > base + 30.0f) {
        groundY = base + 30.0f;
    } else if (groundY < base - 30.0f) {
        groundY = base - 30.0f;
    }
    *y = groundY + (5.0f + 0.3f * n);
    Mtx_Mul(out[0], bone->skin, mtx[0]);
    Gs_SetState(RS_MTX_SLOT(bone->index), (s32)out);
    if (bone->child != NULL) {
        Bone_SnapToGround(bone->child, n, base);
    }
    if (bone->sibling != NULL) {
        Bone_SnapToGround(bone->sibling, n, base);
    }
}

void Mtx_RotateY(f32 *m, f32 a);
s16 Effect_MapTexId(s16 id);
s16 EmSlot_FindKind(s16 k);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
struct EmBone;
void Bone_UpdateWorld(struct EmBone *b, f32 *m);
f32 Math_Angle16ToRad(u16 a);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Area_IsDay(u16 area);
f32 EmBlangonga_ShadowFade(Em *em);
s32 EmChameleos_IsInvisible(Em *em);
f32 Math_Absf(f32 x);
extern s16 gShadowBonesEm07[];
extern s16 gShadowBonesEm37[];
extern s16 gShadowBonesEm3A[];
/* Draws a shadow (the slot's owner) under its owner, by type: 0 and 3 one model of the common
 * models (scaled by the monster's shadow size, stretched along four bones for some kinds, faded
 * for the Blangonga family and an invisible Chameleos; 3 laid on the ground bone by bone), 1 a
 * player's two part shadows turned away from the sky's far point (half bright at night in a
 * quest), 2 one of the five parts, 4 the monster's own model set (Kirin). Skipped while the owner's
 * hideShadow is set, for a local player aiming, and outside the current area. */
void Shadow_Draw(DrawSlot *slot)
{
    f32 mtx[4][4];
    f32 out[4][4];
    f32 scaleMtx[4][4];
    f32 bonePos[4][3];
    f32 pos[3];
    StageObj *shadow;
    Model *set;
    f32 *sizes;
    ModelMatSet *matSet;
    ModelMat *mats;
    s32 i;
    Player *pl;
    s16 *bones;
    s16 idx;
    f32 scaleX;
    f32 scaleZ;
    f32 bright;
    u16 ang;
    f32 groundY;
    f32 spanX;
    f32 spanZ;
    EffectParts *w;

    shadow = slot->owner;
    pl = shadow->owner;
    if (pl->hideShadow != 0) {
        return;
    }
    if (shadow->type == SHADOW_MODEL || shadow->type == SHADOW_GROUND_BONES) {
        set = gCommonModels[1];
    } else if (shadow->type == SHADOW_OWN_MODEL) {
        idx = EmSlot_FindKind(pl->weaponType);
        if (idx == -1) {
            return;
        }
        set = gScene.partModels[idx];
    } else {
        set = gCommonModels[0];
    }
    if (pl->isEm == 0) {
        if (Player_IsLocal(pl) == 1 && pl->aimLockTimer != 0) {
            return;
        }
    } else if (gScene.inLobby != 0) {
        sizes = gShadowSizesLobby[0];
    } else {
        sizes = gShadowSizes[0];
    }
    if (gScene.inLobby == 0 || pl->ownTask != 1 || pl->weaponType != 3) { /* the town's monster of kind 3 is drawn in any area */
        if (gScene.area != pl->area) {
            return;
        }
    }
    if (set == NULL || pl->visible == 0) {
        return;
    }
    if (set->active == 0) {
        return;
    }
    Gs_SetState(RS_ALPHA_REF, 0);
    Gs_SetState(RS_ZWRITE, 0);
    Gfx_SetTexFilter(1);
    mats = set->mats;
    switch (shadow->type) {
    case SHADOW_MODEL:
    case SHADOW_GROUND_BONES:
        matSet = set->matSets;
        Gs_SetState(RS_COLOR2, -1);
        if (pl->isEm == 0) {
            Mtx_Scale(scaleMtx[0], 4.0f, 1.0f, 4.0f);
            bright = 1.0f;
        } else {
            scaleX = pl->scale[0] * sizes[pl->weaponType * 2];
            scaleZ = pl->scale[2] * sizes[pl->weaponType * 2 + 1];
            switch (pl->weaponType) {
            case EM_KIND_LAO_SHAN_LUNG:
            case EM_KIND_ASHEN_LAO_SHAN_LUNG:
            case EM_KIND_SHEN_GAOREN:
            case EM_KIND_YAMA_TSUKAMI:
            case EM_KIND_YAMA_TSUKAMI_B:
                switch (pl->weaponType) {
                case EM_KIND_LAO_SHAN_LUNG:
                case EM_KIND_ASHEN_LAO_SHAN_LUNG:
                    bones = gShadowBonesEm07;
                    break;
                case EM_KIND_SHEN_GAOREN:
                    bones = gShadowBonesEm37;
                    break;
                case EM_KIND_YAMA_TSUKAMI:
                case EM_KIND_YAMA_TSUKAMI_B:
                    bones = gShadowBonesEm3A;
                    break;
                }
                matSet = &set->matSets[1];
                for (i = 0; i < 4; i++) {
                    Em_GetBonePos(pl, bones[i], bonePos[i]);
                    Vec_Diff(bonePos[i], bonePos[i], shadow->pos);
                    Vec_RotateY(bonePos[i], 6.2831855f * -pl->rot[1] / 65536.0f);
                }
                spanX = Math_Absf(0.5f * (bonePos[0][0] + bonePos[2][0]) - 0.5f * (bonePos[1][0] + bonePos[3][0]));
                spanZ = Math_Absf(0.5f * (bonePos[0][2] + bonePos[1][2]) - 0.5f * (bonePos[2][2] + bonePos[3][2]));
                scaleX += 0.02f * spanX;
                scaleZ += 0.02f * spanZ;
                bright = 1.0f;
                break;
            case EM_KIND_BLANGONGA:
            case EM_KIND_CONGALALA:
            case EM_KIND_RAJANG:
                bright = EmBlangonga_ShadowFade((Em *) pl);
                break;
            case EM_KIND_CHAMELEOS:
                if (EmChameleos_IsInvisible((Em *) pl) == 1) {
                    bright = 0.0f;
                } else {
                    bright = 1.0f;
                }
                break;
            default:
                bright = 1.0f;
                break;
            }
            Mtx_Scale(scaleMtx[0], scaleX, 1.0f, scaleZ);
        }
        Mtx_Translation(mtx[0], shadow->pos[0], 3.0f + shadow->pos[1], shadow->pos[2]);
        Mtx_RotateY(mtx[0], Math_Angle16ToRad(((Player *)shadow->owner)->rot[1]));
        Mtx_Concat3x3(mtx[0], scaleMtx[0]);
        Bone_UpdateWorld(set->bones, mtx[0]);
        if (shadow->type == SHADOW_MODEL) {
            Bone_StoreMatrices(set->bones);
        } else {
            Bone_SnapToGround(set->bones, shadow->ownerIndex, pl->floorY);
        }
        if (matSet != NULL && matSet->id != -1) {
            Shadow_SetMaterials((ObjHead *)pl, mats, matSet, bright);
            Draw_ApplyStates(matSet->flags);
            Model_Draw(matSet->id, 0);
        }
        break;
    case SHADOW_PLAYER:
        idx = Effect_MapTexId(0x33);
        if (idx != -1) {
            matSet = &set->matSets[idx];
            Mtx_Scale(scaleMtx[0], 1.0f, 1.0f, 1.0f);
            for (i = 0; i < 2; i++) {
                if (i == 0) {
                    Mtx_GetTrans(pos, pl->partRecs[5]->m1);
                } else {
                    Mtx_GetTrans(pos, pl->partRecs[8]->m1);
                }
                pos[1] -= 13.0f;
                groundY = AreaGrid2_GetFloorY(pos);
                if (!(groundY < pos[1])) {
                    groundY = pos[1];
                }
                Mtx_Identity(mtx[0]);
                ang = Vec_Heading(pl->pos, gAreaState.skyPos) + 0x4000;
                Mtx_RotateY(mtx[0], 6.2831855f * ang / 65536.0f);
                Mtx_SetTrans(mtx[0], pos[0], 4.0f + groundY, pos[2]);
                Mtx_Mul(out[0], (f32 *)scaleMtx, mtx[0]);
                Gs_SetState(RS_MTX_SLOT(0), (s32)out);
                if (gScene.inLobby == 0 && Area_IsDay(gScene.area) == 0) {
                    bright = 0.5f;
                } else {
                    bright = 1.0f;
                }
                Shadow_SetMaterials((ObjHead *)pl, mats, matSet, bright);
                Draw_ApplyStates(matSet->flags);
                Model_Draw(matSet->id, 0);
            }
        }
        break;
    case SHADOW_PARTS:
        w = (EffectParts *)shadow->buf;
        idx = Effect_MapTexId(0x32);
        if (idx != -1) {
            matSet = &set->matSets[idx];
            Mtx_Scale(scaleMtx[0], gShadowPartScales[slot->part], gShadowPartScales[slot->part], gShadowPartScales[slot->part]);
            Mtx_Translation(mtx[0], w->pos[slot->part][0], w->pos[slot->part][1], w->pos[slot->part][2]);
            Mtx_Mul(out[0], (f32 *)scaleMtx, mtx[0]);
            Gs_SetState(RS_MTX_SLOT(0), (s32)out);
            Shadow_SetMaterials((ObjHead *)pl, mats, matSet, 1.0f);
            Draw_ApplyStates(matSet->flags);
            Model_Draw(matSet->id, 0);
        }
        break;
    case SHADOW_OWN_MODEL:
        matSet = &set->matSets[3];
        Mtx_Scale(scaleMtx[0], sizes[pl->weaponType * 2], 1.0f, sizes[pl->weaponType * 2 + 1]);
        Mtx_Translation(mtx[0], shadow->pos[0], 3.0f + shadow->pos[1], shadow->pos[2]);
        Mtx_Mul(out[0], (f32 *)scaleMtx, mtx[0]);
        Gs_SetState(RS_MTX_SLOT(0), (s32)out);
        Gs_SetState(RS_COLOR2, 0x80FFFFFF);
        Shadow_SetMaterials((ObjHead *)pl, mats, matSet, 1.0f);
        Draw_ApplyStates(matSet->flags);
        Model_Draw(matSet->id, 0);
        break;
    }
    Gs_SetState(RS_ZWRITE, 1);
    Draw_SetDefaultState();
}

void Em_BonePos(void *obj, s16 bone, f32 *out);
/* Makes the shadow of a player or monster (STAGEOBJ_KIND_SHADOW, of the given ShadowType; SHADOW_PARTS takes a
 * buffer block for its five parts): SHADOW_PLAYER is kept (keep) and placed by its run; the others start
 * at the owner's bone 10 on the floor. */
void Shadow_Spawn(void *obj, s16 type)
{
    Player *pl = obj;
    StageObj *shadow;

    if (type == 2) {
        if ((shadow = StageObj_Alloc(1)) == NULL) {
            return;
        }
    } else {
        if ((shadow = StageObj_Alloc(0)) == NULL) {
            return;
        }
    }
    shadow->kind = STAGEOBJ_KIND_SHADOW;
    shadow->update = Shadow_Update;
    shadow->draw = NULL;
    shadow->owner = pl;
    shadow->type = type;
    if (shadow->type == SHADOW_PLAYER) {
        shadow->keep = 1;
        return;
    }
    shadow->ownerIndex = pl->index;
    Em_BonePos(pl, 10, shadow->pos);
    shadow->pos[1] = pl->floorY;
}

/* The update of the hit sparks (STAGEOBJ_KIND_HIT_SPARK; HitSpark_SpawnForPlayer and the like), by state. */
void HitSpark_Update(StageObj *fx)
{
    switch (fx->state) {
    case 0:
        HitSpark_Init(fx);
        break;
    case 1:
        HitSpark_RunTimed(fx);
        break;
    case 2:
        HitSpark_End(fx);
        break;
    case 3:
        HitSpark_Delete(fx);
        break;
    }
}

u16 Rand_Next(s32 a);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void HitSpark_Draw(DrawSlot *s);

/* HitSpark_Update's first state for types 0-12: shows the spark, randomizes it per type (angle,
 * scale, colour; type 8 starts at a bone of its owner) and gives it a draw slot drawn by
 * HitSpark_Draw (none free: freed). The factors are written as the quotients 0.4f / 1000.0f and
 * 0.3f / 30.0f: the game's constants are those quotients, one bit above 0.0004f and 0.01f. */
void HitSpark_Init(StageObj *fx)
{
    f32 mtx[16];
    f32 offset[3];

    fx->state++;
    fx->visible = 1;
    fx->draw = NULL;
    fx->timer = 0;
    switch (fx->type) {
    case 0:
        fx->timer = -4;
        break;
    case 4:
        fx->angle = Rand_Next(1);
        break;
    case 5:
        fx->scale *= 0.8f + (0.4f / 1000.0f) * (Rand_Next(1) & 0x3FF);
        break;
    case 7:
        fx->scale *= 0.8f + (0.4f / 1000.0f) * (Rand_Next(1) & 0x3FF);
        fx->angle = Rand_Next(1);
        break;
    case 6:
        fx->var1 = Rand_Next(1) % 3;
        break;
    case 8:
        offset[0] = (0.3f / 30.0f) * ((Rand_Next(1) & 0x3FF) - 0x200);
        offset[1] = 5.0f + (0.3f / 30.0f) * (Rand_Next(1) & 0x3FF);
        offset[2] = 0.0f;
        Mtx_Copy(mtx, (f32 *)Em_GetBone(fx->owner, fx->var0));
        Vec_RotateSelf(offset, mtx);
        Mtx_GetTrans(fx->pos, mtx);
        fx->pos[0] += offset[0];
        fx->pos[1] += offset[1];
        fx->pos[2] += offset[2];
        break;
    case 9:
    case 10:
    case 11:
        if (fx->var0 & 1) {
            fx->timer = -8;
        }
        fx->var1 = Rand_Next(1) >> 8;
        fx->var2 = Rand_Next(1) >> 8;
        fx->angle = Rand_Next(1);
        break;
    case 12:
        fx->scale = 2.0f;
        break;
    case 2:
        break;
    case 1:
    case 3:
    default:
        fx->var1 = Rand_Next(1) >> 8;
        fx->var2 += ((Rand_Next(1) & 0xFFF) - 0x800) >> 8;
        fx->angle += (s16)((Rand_Next(1) & 0xFFF) - 0x800);
        break;
    }
    fx->slot = DrawSlot_Alloc();
    if (fx->slot != -1) {
        fx->node = DrawSlot_Get(fx->slot);
        fx->node->owner = fx;
        fx->node->draw = HitSpark_Draw;
        return;
    }
    StageObj_Free(fx);
}

extern f32 gCamMatrix[];

/* HitSpark_Update's timed kinds (types 0-12): the timer runs in steps of 2
 * to a life per type (then the next state, hidden); some drift up, types 4,
 * 6 and 12 are drawn pushed towards the camera. */
void HitSpark_RunTimed(StageObj *fx)
{
    s16 life;
    f32 offset[3];

    fx->timer += 2;
    switch (fx->type) {
    case 4:
        life = 8;
        break;
    case 5:
        life = 0x18;
        break;
    case 6:
        life = 8;
        break;
    case 7:
        life = 0x18;
        break;
    case 8:
        life = 0x20;
        break;
    case 9:
    case 10:
    case 11:
        life = 0x10;
        break;
    case 12:
        life = 8;
        break;
    default:
        life = 0x1E;
        break;
    }
    if (fx->timer <= 0) {
        return;
    }
    if (fx->timer > life) {
        fx->state++;
        fx->visible = 0;
        return;
    }
    if (fx->node == NULL) {
        return;
    }
    switch (fx->type) {
    case 0:
    case 2:
        if (fx->timer >= 16) {
            fx->pos[1] -= 4.0f;
        }
        break;
    case 3:
        if (fx->timer >= 10) {
            fx->pos[1] -= 5.0f;
        }
        break;
    case 4:
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = -20.0f;
        Vec_RotateSelf(offset, gCamMatrix);
        fx->node->pos[0] = fx->pos[0] + offset[0];
        fx->node->pos[1] = fx->pos[1] + offset[1];
        fx->node->pos[2] = fx->pos[2] + offset[2];
        OT_AddByDepth(gDrawOtSorted, fx->node, 0x40, 0);
        return;
    case 6:
    case 12:
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 50.0f;
        Vec_RotateSelf(offset, gCamMatrix);
        fx->node->pos[0] = fx->pos[0] + offset[0];
        fx->node->pos[1] = fx->pos[1] + offset[1];
        fx->node->pos[2] = fx->pos[2] + offset[2];
        OT_AddByDepth(gDrawOtSorted, fx->node, 0x40, 0);
        return;
    case 8:
        fx->pos[1] += -0.4f * fx->timer;
        break;
    }
    fx->node->pos[0] = fx->pos[0];
    fx->node->pos[1] = fx->pos[1];
    fx->node->pos[2] = fx->pos[2];
    OT_AddByDepth(gDrawOt32, fx->node, 0x20, 0);
}

void HitSpark_End(StageObj *fx)
{
    fx->state++;
    DrawSlot_Free(fx->slot);
}

void HitSpark_Delete(StageObj *fx)
{
    StageObj_Free(fx);
}

extern u8 gEffectColors[][4];
extern f32 gHitSparkScaleKeys6[];
extern f32 gHitSparkKeys12[];
extern s32 gHitSparkColorKeys12[];
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_RotateZXY(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_TurnZ(f32 *m, f32 a);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_SetBlendFlags(u16 flags);
void Effect_ScaleVec(f32 *v);
s32 Effect_OwnerColorRow(s32 a);
s16 Effect_MapTexId(s16 id);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);

/* Draws a hit spark (STAGEOBJ_KIND_HIT_SPARK): per type, a material set of the first common model
 * (gCommonModels[0]->matSets) with its scale, rotation, UV offset and colour (white, a row of
 * gEffectColors by the owner, or keyed for type 12); type 0 is drawn twice, the second turned
 * half a circle. */
void HitSpark_Draw(DrawSlot *slot)
{
    Model *set;
    StageObj *fx;
    ModelMat *mats;
    ModelMatSet *matSet;
    u16 flags;
    s16 colorRow;
    u8 r, g, b;
    u8 uvRow;
    s16 id;
    f32 uvDiv;
    f32 mtx[16];
    f32 uvMtx[16];
    f32 scaleVec[3];
    u32 color;

    set = gCommonModels[0];
    fx = slot->owner;
    flags = 0;
    if (set != NULL && set->active != 0) {
        mats = set->mats;
        Mtx_Identity(uvMtx);
        switch (fx->type) {
        case 0:
        case 1:
            Gfx_SetTexFilter(0);
            flags |= 0x10;
            Gs_SetState(RS_ALPHA_REF, 0x80);
            scaleVec[0] = 1.0f;
            scaleVec[1] = 1.0f;
            scaleVec[2] = 1.0f;
            if (fx->type == 0) {
                r = 0xFF;
                colorRow = -1;
                g = r;
                b = r;
            } else {
                Effect_ScaleVec(scaleVec);
                colorRow = Effect_OwnerColorRow(fx->ownerIndex);
            }
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            uvRow = fx->type;
            Mtx_RotateZ(mtx, 6.2831855f * (u16)((fx->var1 << 8) + fx->var2) / 65536.0f);
            Mtx_RotateY(mtx, 6.2831855f * (u16)fx->angle / 65536.0f);
            matSet = &set->matSets[fx->timer / 2];
            Mtx_SetTrans(uvMtx, 0.0f, 0.125f * uvRow, 0.0f);
            break;
        case 2:
            Gfx_SetTexFilter(0);
            flags |= 0x10;
            Gs_SetState(RS_ALPHA_REF, 0x80);
            Mtx_Scale(mtx, 0.1f, 0.1f, 0.7f);
            Mtx_RotateZXY(mtx, 6.2831855f * (fx->var1 << 8) / 65536.0f, 6.2831855f * (fx->var2 << 8) / 65536.0f, 6.2831855f * fx->angle / 65536.0f);
            matSet = &set->matSets[fx->timer / 2];
            Mtx_SetTrans(uvMtx, 0.75f, 0.25f, 0.0f);
            r = 0xFF;
            colorRow = -1;
            g = r;
            b = r;
            break;
        case 3:
            Gs_SetState(RS_ALPHA_REF, RS_ALPHA_REF_OPAQUE);
            matSet = set->matSets + fx->timer / 2 + 97;
            scaleVec[0] = fx->scale;
            scaleVec[1] = fx->scale;
            scaleVec[2] = fx->scale;
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_RotateZXY(mtx, 6.2831855f * (fx->var2 << 8) / 65536.0f, 6.2831855f * fx->angle / 65536.0f, 6.2831855f * (fx->var1 << 8) / 65536.0f);
            if (fx->var0 == 0) {
                Mtx_SetTrans(uvMtx, 0.75f, 0.25f, 0.0f);
            } else {
                Mtx_SetTrans(uvMtx, 0.0f, 0.0f, 0.0f);
            }
            colorRow = Effect_OwnerColorRow(fx->ownerIndex);
            break;
        case 4:
            Gs_SetState(RS_ALPHA_REF, 0);
            matSet = set->matSets + fx->timer / 2 + 27;
            scaleVec[0] = fx->scale;
            scaleVec[1] = fx->scale;
            scaleVec[2] = fx->scale;
            if (fx->var1 == 1) {
                r = 0xFF;
                colorRow = -1;
                g = r;
                b = r;
                flags |= 2;
                Mtx_SetTrans(uvMtx, 0.125f, 0.0f, 0.0f);
            } else {
                Effect_ScaleVec(scaleVec);
                colorRow = Effect_OwnerColorRow(fx->ownerIndex);
                Mtx_SetTrans(uvMtx, 0.0f, 0.0f, 0.0f);
            }
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_RotateZ(mtx, 6.2831855f * fx->angle / 65536.0f);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        case 5:
        case 7:
            if (fx->type == 5) {
                uvDiv = 24.0f;
                id = 0x35;
            } else {
                uvDiv = 24.0f;
                id = 0x4C;
            }
            id = Effect_MapTexId(id);
            if (id == -1) {
                return;
            }
            matSet = &set->matSets[id];
            Gs_SetState(RS_ALPHA_REF, 0x80);
            Mtx_Scale(mtx, fx->scale, fx->scale, fx->scale);
            Mtx_RotateY(mtx, 6.2831855f * fx->angle / 65536.0f);
            Mtx_SetTrans(uvMtx, 0.0f, fx->timer * (0.125f / uvDiv), 0.0f);
            r = 0xFF;
            colorRow = -1;
            g = r;
            b = r;
            break;
        case 6:
            flags |= 2;
            Key_LerpVec3(gHitSparkScaleKeys6, scaleVec, fx->timer / 2.0f);
            scaleVec[0] *= fx->scale;
            scaleVec[1] *= fx->scale;
            scaleVec[2] *= fx->scale;
            Gs_SetState(RS_ALPHA_REF, 0);
            matSet = set->matSets + fx->var1 + 32;
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_SetTrans(uvMtx, 0.25f, 0.0f, 0.0f);
            Mtx_Concat3x3(mtx, gCamMatrix);
            r = 0xFF;
            colorRow = -1;
            g = r;
            b = r;
            break;
        case 8:
            Gs_SetState(RS_ALPHA_REF, 0);
            matSet = &set->matSets[39];
            Mtx_Scale(mtx, fx->scale, fx->scale, fx->scale);
            Mtx_Concat3x3(mtx, gCamMatrix);
            r = 0xFF;
            colorRow = -1;
            g = r;
            b = r;
            break;
        case 9:
        case 10:
        case 11:
            Gs_SetState(RS_ALPHA_REF, 0x80);
            Mtx_Scale(mtx, fx->scale, fx->scale, fx->scale);
            Mtx_RotateXYZ(mtx, 6.2831855f * (fx->var2 << 8) / 65536.0f, 6.2831855f * fx->angle / 65536.0f, 6.2831855f * (fx->var1 << 8) / 65536.0f);
            matSet = set->matSets + fx->timer / 2 + 134;
            colorRow = -1;
            if (fx->type == 9) {
                r = 0xFF;
                g = r;
                b = r;
            } else if (fx->type == 10) {
                r = 0x5F;
                g = r;
                b = r;
            } else {
                r = 0xBF;
                g = r;
                b = r;
            }
            break;
        case 12:
            Gs_SetState(RS_ALPHA_REF, 0);
            flags |= 2;
            matSet = set->matSets;
            Key_LerpVec3(gHitSparkKeys12, scaleVec, fx->timer / 2.0f);
            colorRow = -2;
            scaleVec[0] = fx->scale;
            scaleVec[1] = fx->scale;
            scaleVec[2] = fx->scale;
            Color_EvalKeys(gHitSparkColorKeys12, fx->timer / 2.0f, &color);
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_RotateZ(mtx, 1.308997f);
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        }
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        Gs_SetState(RS_UV_MTX, (s32)uvMtx);
        if (colorRow == -2) {
            Gs_SetState(RS_COLOR2, color);
        } else {
            if (colorRow >= 0) {
                r = gEffectColors[colorRow][0];
                g = gEffectColors[colorRow][1];
                b = gEffectColors[colorRow][2];
            }
            Gs_SetState(RS_COLOR2, (r << 16) | 0xFF000000 | (g << 8) | b);
        }
        Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
        if (matSet != NULL && matSet->id != -1) {
            Gs_SetLightList(mats, matSet);
            Draw_ApplyStates(matSet->flags);
            Draw_SetBlendFlags(flags);
            Model_Draw(matSet->id, 0);
        }
        if (fx->type == 0) {
            Gs_SetState(RS_COLOR2, (r << 16) | 0xFF000000 | (g << 8) | b);
            Mtx_TurnZ(mtx, 3.1415927f);
            Gs_SetState(RS_MTX_SLOT(0), (s32)mtx);
            if (matSet != NULL && matSet->id != -1) {
                Gs_SetLightList(mats, matSet);
                Draw_ApplyStates(matSet->flags);
                Draw_SetBlendFlags(flags);
                Model_Draw(matSet->id, 0);
            }
        }
        Gfx_SetTexFilter(1);
        Draw_SetDefaultState();
        if (fx->type == 6) {
            Gs_SetState(RS_ALPHA_REF, 0);
        } else {
            Gs_SetState(RS_ALPHA_REF, 0x80);
        }
    }
}

/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Obj_InCurrentArea(void *obj);
StageObj *StageObj_Alloc(s32 nblk);
/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos, belonging to the player and turned away from its heading. */
void HitSpark_SpawnForPlayer(Player *pl, u8 type, f32 *pos)
{
    StageObj *fx;

    if (pl != NULL && (u8)Obj_InCurrentArea(pl) != 0) {
        fx = StageObj_Alloc(0);
        if (fx != NULL) {
            fx->kind = STAGEOBJ_KIND_HIT_SPARK;
            fx->update = HitSpark_Update;
            fx->type = type;
            fx->owner = pl;
            fx->scale = 1.0f;
            fx->pos[0] = pos[0];
            fx->pos[1] = pos[1];
            fx->pos[2] = pos[2];
            fx->angle = pl->hitAngle + 0x8000;
        }
    }
}

u16 Rand_Next(s32 a);
/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos, with the high bytes of a and b and a random angle. */
void HitSpark_SpawnRandomAngle(u16 angle1, u16 angle2, u8 type, f32 *pos)
{
    StageObj *fx;

    if ((fx = StageObj_Alloc(0)) != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->scale = 1.0f;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var1 = angle1 >> 8;
        fx->var2 = angle2 >> 8;
        fx->angle = Rand_Next(1);
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos with angle ang and scale, belonging to o. */
void HitSpark_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale)
{
    StageObj *fx;

    if ((u8)Obj_InCurrentArea(owner) != 0 && (fx = StageObj_Alloc(0)) != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->owner = owner;
        fx->scale = scale;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var0 = sub;
        fx->var2 = 0;
        fx->angle = ang;
        if (((ObjHead *)owner)->isEm != 0) {
            fx->ownerIndex = ((Em *)owner)->type;
        } else {
            fx->ownerIndex = 0;
        }
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos with angle ang and scale. */
void HitSpark_SpawnAtScaled(u16 angle2, u16 ang, s32 type, f32 *pos, f32 scale)
{
    StageObj *fx = StageObj_Alloc(0);

    if (fx != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->owner = NULL;
        fx->scale = scale;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var2 = angle2 >> 8;
        fx->angle = ang;
    }
}

/* A hit spark (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos with scale, sub in var0. var1 is 1 for the hit
 * effect kinds 9-16 (kind: hitFx & HITFX_KIND_MASK); for the others it is 0, and when the owner is
 * a monster its kind goes to ownerIndex (the colour row of Effect_OwnerColorRow). */
void HitSpark_Spawn(void *owner, s16 kind, u8 type, u8 sub, f32 *pos, f32 scale)
{
    StageObj *fx;

    if ((u8)Obj_InCurrentArea(owner) != 0 && (fx = StageObj_Alloc(0)) != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->scale = scale;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var0 = sub;
        fx->ownerIndex = 0;
        switch (kind) {
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            fx->var1 = 1;
            break;
        default:
            fx->var1 = 0;
            if (((ObjHead *)owner)->isEm != 0) {
                fx->ownerIndex = ((Em *)owner)->type;
            }
            break;
        }
        fx->var2 = 0;
        fx->angle = 0;
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) belonging to owner, with the given scale. */
void HitSpark_SpawnOnOwner(void *owner, u8 type, u8 sub, f32 scale)
{
    StageObj *fx;

    if ((u8)Obj_InCurrentArea(owner) != 0) {
        fx = StageObj_Alloc(0);
        if (fx != NULL) {
            fx->kind = STAGEOBJ_KIND_HIT_SPARK;
            fx->update = HitSpark_Update;
            fx->type = type;
            fx->owner = owner;
            fx->scale = scale;
            fx->var0 = sub;
            fx->var2 = 0;
            fx->angle = 0;
        }
    }
}

StageObj *StageObj_Alloc(s32 nblk);
/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos, turned to ang. */
void HitSpark_SpawnAt(f32 *pos, s32 type, u16 ang)
{
    StageObj *fx = StageObj_Alloc(0);

    if (fx != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->owner = NULL;
        fx->scale = 1.0f;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->angle = ang;
    }
}

/* An effect object (STAGEOBJ_KIND_HIT_SPARK, of the given type) at pos with angle ang and scale. */
void HitSpark_SpawnScaled(s16 ang, u8 type, u8 sub, f32 *pos, f32 scale)
{
    StageObj *fx = StageObj_Alloc(0);

    if (fx != NULL) {
        fx->kind = STAGEOBJ_KIND_HIT_SPARK;
        fx->update = HitSpark_Update;
        fx->type = type;
        fx->owner = NULL;
        fx->scale = scale;
        fx->pos[0] = pos[0];
        fx->pos[1] = pos[1];
        fx->pos[2] = pos[2];
        fx->var0 = sub;
        fx->var2 = 0;
        fx->angle = ang;
        fx->ownerIndex = 0;
    }
}

/* This unit's .data: the meshes of the monsters sub_main runs itself, by variant, their handler
 * table, the shadows' bones and scales, and the hit sparks' keys. */
u16 gEmVariantMeshes_0[8] = {
#include "sub_main/004655E0.inc"
};
u16 gEmVariantMeshes_1[8] = {
#include "sub_main/004655F0.inc"
};
u16 gEmVariantMeshes_2[8] = {
#include "sub_main/00465600.inc"
};
u16 gEmVariantMeshes_3[8] = {
#include "sub_main/00465610.inc"
};
u16 gEmVariantMeshes_4[8] = {
#include "sub_main/00465620.inc"
};
u16 gEmVariantMeshes_5[8] = {
#include "sub_main/00465630.inc"
};
u16 *gEmVariantMeshes[8] = {
    gEmVariantMeshes_0, gEmVariantMeshes_1, gEmVariantMeshes_2, gEmVariantMeshes_3,
    gEmVariantMeshes_4, gEmVariantMeshes_5, NULL, NULL,
};
s16 gEmVariantShownMeshes[16][2] = {
#include "sub_main/00465660.inc"
};
void *gEmNopHandlers[8] = {
    (void *)Em_NopHandlerA, (void *)Em_NopHandlerA, (void *)Em_NopHandlerB, (void *)Em_NopHandlerB,
    NULL, NULL, NULL, NULL,
};
s16 gShadowPartBones[8] = {
#include "sub_main/004656C0.inc"
};
f32 gShadowPartScales[6] = {
#include "sub_main/004656D0.inc"
};
s16 gShadowBonesEm07[4] = {
#include "sub_main/004656E8.inc"
};
s16 gShadowBonesEm37[4] = {
#include "sub_main/004656F0.inc"
};
s16 gShadowBonesEm3A[4] = {
#include "sub_main/004656F8.inc"
};
f32 gHitSparkScaleKeys6[20] = {
#include "sub_main/00465700.inc"
};
f32 gHitSparkKeys12[12] = {
#include "sub_main/00465750.inc"
};
s32 gHitSparkColorKeys12[8] = {
#include "sub_main/00465780.inc"
};
