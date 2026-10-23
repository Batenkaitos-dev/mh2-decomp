/*
 * The hit tests' geometry: segments (Segment), spheres and capsules
 * (Capsule, HitCapsule): touch tests, contact points, push-out vectors and
 * distances, a cylinder's and a slab's containment, a capsule against a
 * rectangle, and a segment clipped by a plane or a height range. The attack
 * objects' hits use them, and so do the pushes between bodies, the camera's
 * collision, the monsters' sight and the area's drawing and collision.
 * Vec_WithinDist is static so that the compiler keeps Capsule_Classify's
 * point and radius in the same registers across its three calls, as the
 * game's code does.
 */
#include "common.h"
#include "capsule.h"
#include "engine_math.h"

/* Built at optimization level 4: at the default level, Capsule_ContactCapsule,
 * Seg_SegDistance, Capsule_SphereContact, Capsule_PushCapsule and
 * Seg_ClipPlane compile differently from the game's code. */
#pragma optimization_level 4

/* end of prototypes */

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

