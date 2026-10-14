/*
 * Segment and sphere geometry (0x3BE060; the static Vec_WithinDist is one unit
 * with Capsule_Classify, which keeps a1 and f12 across three calls of it), then
 * the ObjAttach objects (effects and hitboxes on players and monsters).
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
#include "sub_main/unk_3BE060.h"   /* the texts this file uses (tools/content_gen.py) */

#pragma optimization_level 4

/* The buffer of a type 2 effect object (StageObj.buf): five parts, a draw slot each. */
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
void Effect01_Setup(StageObj *o);
void Effect01_Run(StageObj *o);
void Effect01_End(StageObj *o);
StageObj *StageObj_Alloc(s32 nblk);
void Effect01_Delete(StageObj *o);
void Effect02_Init(StageObj *o);
void Effect02_RunTimed(StageObj *o);
void Effect02_End(StageObj *o);
void Effect02_Delete(StageObj *o);
void Effect02_Update(StageObj *o);
extern ObjAttach *gObjAttachList;
/* end of prototypes */

/* A segment: its two ends, the direction, the middle and the length. */
typedef struct Segment {
    f32 a[3];                   /* 0x00 */
    f32 b[3];                   /* 0x0C */
    f32 dir[3];                 /* 0x18: b - a */
    f32 mid[3];                 /* 0x24 */
    f32 len;                    /* 0x30 */
} Segment;


void Vec_Copy(f32 *dst, f32 *src);
f32 Vec_Distance(f32 *a, f32 *b);

void Segment_Init(f32 *a, f32 *b, Segment *seg)
{
    Vec_Copy(seg->a, a);
    Vec_Copy(seg->b, b);
    seg->dir[0] = b[0] - a[0];
    seg->dir[1] = b[1] - a[1];
    seg->dir[2] = b[2] - a[2];
    seg->mid[0] = 0.5f * (a[0] + b[0]);
    seg->mid[1] = 0.5f * (a[1] + b[1]);
    seg->mid[2] = 0.5f * (a[2] + b[2]);
    seg->len = Vec_Distance(seg->a, seg->mid);
}

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
static u8 Vec_WithinDist(f32 r, f32 *a, f32 *b)
{
    f32 dx = b[0] - a[0];
    f32 dy = b[1] - a[1];
    f32 dz = b[2] - a[2];

    return dx * dx + dy * dy + dz * dz <= r * r;
}

/* 1 if spheres (a, ra) and (b, rb) touch. */
u8 Sphere_Touch(f32 *a, f32 *b, f32 ra, f32 rb)
{
    f32 dx = a[0] - b[0];
    f32 dy = a[1] - b[1];
    f32 dz = a[2] - b[2];

    return dx * dx + dy * dy + dz * dz <= (ra + rb) * (ra + rb);
}

/* The contact point of two touching spheres, 0 if they don't touch. */
s32 Spheres_Touch(f32 ra, f32 rb, f32 *a, f32 *b, f32 *out)
{
    f32 d = Vec_Distance(a, b);

    if (d <= ra + rb) {
        if (d > 0.001f) {
            f32 dx = a[0] - b[0];
            f32 dy = a[1] - b[1];
            f32 dz = a[2] - b[2];

            d = rb / d;
            dx *= d;
            out[0] = dx + b[0];
            dy *= d;
            out[1] = dy + b[1];
            dz *= d;
            out[2] = dz + b[2];
        } else {
            Vec_Copy(out, b);
        }
        return 1;
    }
    return 0;
}

/* The push-out vector of two touching spheres, 0 if they don't touch. */
s32 Sphere_PushOut(f32 ra, f32 rb, f32 *a, f32 *b, f32 *out)
{
    f32 d = Vec_Distance(a, b);
    f32 sum = ra + rb;

    if (d <= sum) {
        if (d > 0.001f) {
            f32 t = (sum - d) / d;
            f32 dx = a[0] - b[0];
            f32 dy = a[1] - b[1];
            f32 dz = a[2] - b[2];

            out[0] = t * dx;
            out[1] = t * dy;
            out[2] = t * dz;
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

/* Capsule a against capsule b: 1 and the contact point (on b's surface) in out if they touch.
 * As in Capsule_PushCapsule: the float declaration order, hh, and no break after outer case 1. */
u8 Capsule_ContactCapsule(Capsule *a, Capsule *b, f32 *out)
{
    f32 da[3];
    f32 db[3];
    f32 d[3];
    f32 e[3];
    f32 pa[3];
    f32 pb[3];
    f32 c[3];
    f32 t;
    f32 s;
    f32 lenA;
    f32 cs;
    f32 r1;
    f32 rr;
    f32 h;
    f32 lenB;
    f32 t3;
    f32 k;
    f32 m;
    f32 r2;
    f32 den;
    f32 sa;
    f32 sb;
    f32 hh;
    u8 ca;
    u8 cb;

    if (Vec_WithinDist(a->len + b->len, a->mid, b->mid) == 0) {
        return 0;
    }
    rr = a->radius + b->radius;
    lenA = 2.0f * (a->len - a->radius);
    if (lenA < 0.001f) {
        lenA = 0.001f;
    }
    lenB = 2.0f * (b->len - b->radius);
    if (lenB < 0.001f) {
        lenB = 0.001f;
    }
    da[0] = a->dir[0] / lenA;
    da[1] = a->dir[1] / lenA;
    da[2] = a->dir[2] / lenA;
    db[0] = b->dir[0] / lenB;
    db[1] = b->dir[1] / lenB;
    db[2] = b->dir[2] / lenB;
    cs = Vec_Dot(da, db);
    if (1.0f - Math_Absf(cs) < 0.001f) {
        d[0] = a->a[0] - b->a[0];
        d[1] = a->a[1] - b->a[1];
        d[2] = a->a[2] - b->a[2];
        t = Vec_Dot(d, db);
        hh = Vec_Dot(d, d) - t * t;
        if (hh < 0.0f) {
            return 0;
        }
        h = Math_Sqrtf(hh);
        if (!(h <= rr)) {
            return 0;
        }
        if (h < 0.001f) {
            e[0] = 5.0f + db[0];
            e[1] = db[1];
            e[2] = db[2];
            Vec_Cross(c, db, e);
            if (Math_Absf(c[0]) < 0.001f && Math_Absf(c[1]) < 0.001f && Math_Absf(c[2]) < 0.001f) {
                e[1] += 5.0f;
                Vec_Cross(c, db, e);
            }
            Vec_Normalize(c);
            c[0] *= b->radius;
            c[1] *= b->radius;
            c[2] *= b->radius;
        } else {
            c[0] = t * db[0];
            c[1] = t * db[1];
            c[2] = t * db[2];
            pa[0] = b->a[0] + c[0];
            pa[1] = b->a[1] + c[1];
            pa[2] = b->a[2] + c[2];
            c[0] = a->a[0] - pa[0];
            c[1] = a->a[1] - pa[1];
            c[2] = a->a[2] - pa[2];
            k = b->radius / h;
            c[0] *= k;
            c[1] *= k;
            c[2] *= k;
        }
        if (t < 0.0f) {
            e[0] = a->b[0] - b->a[0];
            e[1] = a->b[1] - b->a[1];
            e[2] = a->b[2] - b->a[2];
            t3 = Vec_Dot(e, db);
            if (!(t3 < 0.0f)) {
                if (t3 < lenB) {
                    m = 0.5f * t3;
                } else {
                    m = 0.5f * lenB;
                }
                d[0] = m * db[0];
                d[1] = m * db[1];
                d[2] = m * db[2];
                pa[0] = b->a[0] + d[0];
                pa[1] = b->a[1] + d[1];
                pa[2] = b->a[2] + d[2];
                out[0] = pa[0] + c[0];
                out[1] = pa[1] + c[1];
                out[2] = pa[2] + c[2];
                return 1;
            }
            if (t < t3) {
                r1 = Vec_Length(e);
                if (r1 <= rr) {
                    if (r1 < 0.001f) {
                        r1 = 0.001f;
                    }
                    k = b->radius / r1;
                    e[0] *= k;
                    e[1] *= k;
                    e[2] *= k;
                    out[0] = b->a[0] + e[0];
                    out[1] = b->a[1] + e[1];
                    out[2] = b->a[2] + e[2];
                    return 1;
                }
                return 0;
            }
            r1 = Vec_Length(d);
            if (r1 <= rr) {
                if (r1 < 0.001f) {
                    r1 = 0.001f;
                }
                k = b->radius / r1;
                d[0] *= k;
                d[1] *= k;
                d[2] *= k;
                out[0] = b->a[0] + d[0];
                out[1] = b->a[1] + d[1];
                out[2] = b->a[2] + d[2];
                return 1;
            }
            return 0;
        }
        if (t <= lenB) {
            e[0] = a->b[0] - b->a[0];
            e[1] = a->b[1] - b->a[1];
            e[2] = a->b[2] - b->a[2];
            t3 = Vec_Dot(e, db);
            if (t3 < 0.0f) {
                m = 0.5f * t;
            } else {
                if (!(t3 <= lenB)) {
                    m = t + 0.5f * (lenB - t);
                } else {
                    m = t + 0.5f * (t3 - t);
                }
            }
            d[0] = m * db[0];
            d[1] = m * db[1];
            d[2] = m * db[2];
            pa[0] = b->a[0] + d[0];
            pa[1] = b->a[1] + d[1];
            pa[2] = b->a[2] + d[2];
            out[0] = pa[0] + c[0];
            out[1] = pa[1] + c[1];
            out[2] = pa[2] + c[2];
            return 1;
        }
        e[0] = a->b[0] - b->a[0];
        e[1] = a->b[1] - b->a[1];
        e[2] = a->b[2] - b->a[2];
        t3 = Vec_Dot(e, db);
        if (t3 < 0.0f) {
            out[0] = b->mid[0] + c[0];
            out[1] = b->mid[1] + c[1];
            out[2] = b->mid[2] + c[2];
            return 1;
        }
        if (t3 <= lenB) {
            m = t3 + 0.5f * (lenB - t3);
            d[0] = m * db[0];
            d[1] = m * db[1];
            d[2] = m * db[2];
            pa[0] = b->a[0] + d[0];
            pa[1] = b->a[1] + d[1];
            pa[2] = b->a[2] + d[2];
            out[0] = pa[0] + c[0];
            out[1] = pa[1] + c[1];
            out[2] = pa[2] + c[2];
            return 1;
        }
        if (t < t3) {
            r1 = Math_Sqrtf(h * h + t * t);
            if (r1 <= rr) {
                if (r1 < 0.001f) {
                    r1 = 0.001f;
                }
                k = b->radius / r1;
                d[0] *= k;
                d[1] *= k;
                d[2] *= k;
                out[0] = b->a[0] + d[0];
                out[1] = b->a[1] + d[1];
                out[2] = b->a[2] + d[2];
                return 1;
            }
            return 0;
        }
        r1 = Math_Sqrtf(h * h + t3 * t3);
        if (r1 <= rr) {
            if (r1 < 0.001f) {
                r1 = 0.001f;
            }
            k = b->radius / r1;
            e[0] *= k;
            e[1] *= k;
            e[2] *= k;
            out[0] = b->a[0] + e[0];
            out[1] = b->a[1] + e[1];
            out[2] = b->a[2] + e[2];
            return 1;
        }
        return 0;
    }
    d[0] = b->a[0] - a->a[0];
    d[1] = b->a[1] - a->a[1];
    d[2] = b->a[2] - a->a[2];
    s = Vec_Dot(d, da);
    t = Vec_Dot(d, db);
    den = 1.0f - cs * cs;
    sa = (s - t * cs) / den;
    sb = (s * cs - t) / den;
    if (sa < 0.0f) {
        ca = 0;
    } else if (sa <= lenA) {
        ca = 1;
    } else {
        ca = 2;
    }
    if (sb < 0.0f) {
        cb = 0;
    } else if (sb <= lenB) {
        cb = 1;
    } else {
        cb = 2;
    }
    switch (ca) {
    case 0:
        Seg_NearestPoint(b->a, b->b, db, a->a, pa, lenB);
        d[0] = a->a[0] - pa[0];
        d[1] = a->a[1] - pa[1];
        d[2] = a->a[2] - pa[2];
        r1 = Vec_Length(d);
        switch (cb) {
        case 0:
            Seg_NearestPoint(a->a, a->b, da, b->a, pb, lenA);
            e[0] = pb[0] - b->a[0];
            e[1] = pb[1] - b->a[1];
            e[2] = pb[2] - b->a[2];
            r2 = Vec_Length(e);
            if (r1 < r2) {
                if (r1 <= rr) {
                    Vec_PointAlong(pa, d, out, r1, b->radius);
                    return 1;
                }
                return 0;
            }
            if (r2 <= rr) {
                Vec_PointAlong(b->a, e, out, r2, b->radius);
                return 1;
            }
            return 0;
        case 1:
            if (r1 <= rr) {
                Vec_PointAlong(pa, d, out, r1, b->radius);
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(a->a, a->b, da, b->b, pb, lenA);
            e[0] = pb[0] - b->b[0];
            e[1] = pb[1] - b->b[1];
            e[2] = pb[2] - b->b[2];
            r2 = Vec_Length(e);
            if (r1 < r2) {
                if (r1 <= rr) {
                    Vec_PointAlong(pa, d, out, r1, b->radius);
                    return 1;
                }
                return 0;
            }
            if (r2 <= rr) {
                Vec_PointAlong(b->b, e, out, r2, b->radius);
                return 1;
            }
            return 0;
        }
        break;
    case 1:
        switch (cb) {
        case 0:
            Seg_NearestPoint(a->a, a->b, da, b->a, pa, lenA);
            d[0] = pa[0] - b->a[0];
            d[1] = pa[1] - b->a[1];
            d[2] = pa[2] - b->a[2];
            r1 = Vec_Length(d);
            if (r1 <= rr) {
                Vec_PointAlong(b->a, d, out, r1, b->radius);
                return 1;
            }
            return 0;
        case 1:
            d[0] = sa * da[0];
            d[1] = sa * da[1];
            d[2] = sa * da[2];
            pa[0] = a->a[0] + d[0];
            pa[1] = a->a[1] + d[1];
            pa[2] = a->a[2] + d[2];
            d[0] = sb * db[0];
            d[1] = sb * db[1];
            d[2] = sb * db[2];
            pb[0] = b->a[0] + d[0];
            pb[1] = b->a[1] + d[1];
            pb[2] = b->a[2] + d[2];
            d[0] = pa[0] - pb[0];
            d[1] = pa[1] - pb[1];
            d[2] = pa[2] - pb[2];
            r1 = Vec_Length(d);
            if (r1 <= rr) {
                if (r1 < 0.001f) {
                    Vec_Cross(d, da, db);
                    d[0] *= b->radius;
                    d[1] *= b->radius;
                    d[2] *= b->radius;
                    out[0] = pb[0] + d[0];
                    out[1] = pb[1] + d[1];
                    out[2] = pb[2] + d[2];
                } else {
                    Vec_PointAlong(pb, d, out, r1, b->radius);
                }
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(a->a, a->b, da, b->b, pa, lenA);
            d[0] = pa[0] - b->b[0];
            d[1] = pa[1] - b->b[1];
            d[2] = pa[2] - b->b[2];
            r1 = Vec_Length(d);
            if (r1 <= rr) {
                Vec_PointAlong(b->b, d, out, r1, b->radius);
                return 1;
            }
            return 0;
        }
    case 2:
        Seg_NearestPoint(b->a, b->b, db, a->b, pa, lenB);
        d[0] = a->b[0] - pa[0];
        d[1] = a->b[1] - pa[1];
        d[2] = a->b[2] - pa[2];
        r1 = Vec_Length(d);
        switch (cb) {
        case 0:
            Seg_NearestPoint(a->a, a->b, da, b->a, pb, lenA);
            e[0] = pb[0] - b->a[0];
            e[1] = pb[1] - b->a[1];
            e[2] = pb[2] - b->a[2];
            r2 = Vec_Length(e);
            if (r1 < r2) {
                if (r1 <= rr) {
                    Vec_PointAlong(pa, d, out, r1, b->radius);
                    return 1;
                }
                return 0;
            }
            if (r2 <= rr) {
                Vec_PointAlong(b->a, e, out, r2, b->radius);
                return 1;
            }
            return 0;
        case 1:
            if (r1 <= rr) {
                Vec_PointAlong(pa, d, out, r1, b->radius);
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(a->a, a->b, da, b->b, pb, lenA);
            e[0] = pb[0] - b->b[0];
            e[1] = pb[1] - b->b[1];
            e[2] = pb[2] - b->b[2];
            r2 = Vec_Length(e);
            if (r1 < r2) {
                if (r1 <= rr) {
                    Vec_PointAlong(pa, d, out, r1, b->radius);
                    return 1;
                }
                return 0;
            }
            if (r2 <= rr) {
                Vec_PointAlong(b->b, e, out, r2, b->radius);
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
f32 Seg_NearestPointDist(f32 *out, f32 *a, f32 *b, f32 *p)
{
    f32 ab[3];
    f32 *aby = &ab[1];
    f32 *abz = &ab[2];
    f32 ap[3];
    f32 len2;
    f32 t;

    ab[0] = b[0] - a[0];
    *aby = b[1] - a[1];
    *abz = b[2] - a[2];
    len2 = Vec_Dot(ab, ab);
    if (len2 < 0.001f) {
        Vec_Copy(out, a);
        return Vec_Distance(p, out);
    }
    ap[0] = p[0] - a[0];
    ap[1] = p[1] - a[1];
    ap[2] = p[2] - a[2];
    t = Vec_Dot(ab, ap) / len2;
    if (t <= 0.0f) {
        Vec_Copy(out, a);
    } else if (t >= 1.0f) {
        Vec_Copy(out, b);
    } else {
        out[0] = t * ab[0];
        out[1] = t * *aby;
        out[2] = t * *abz;
        out[0] += a[0];
        out[1] += a[1];
        out[2] += a[2];
    }
    return Vec_Distance(p, out);
}

f32 Vec_Length(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Math_Absf(f32 x);
asm f32 Math_Sqrtf(f32 x);
f32 Seg_PointDistance(f32 *a, f32 *b, f32 *dir, f32 *p, f32 len);
/* The distance between segments a0-a1 and b0-b1. */
f32 Seg_SegDistance(f32 *a0, f32 *a1, f32 *b0, f32 *b1)
{
    f32 ab[3];
    f32 cd[3];
    f32 da[3];
    f32 db[3];
    f32 d[3];
    f32 pa[3];
    f32 c;
    f32 lenA;
    f32 r;
    f32 pb[3];
    f32 t;
    f32 t2;
    f32 lenB;
    f32 s;
    f32 den;
    f32 sa;
    f32 sb;
    f32 r1;
    f32 r2;
    u8 ca;
    u8 cb;

    ab[0] = a1[0] - a0[0];
    ab[1] = a1[1] - a0[1];
    ab[2] = a1[2] - a0[2];
    cd[0] = b1[0] - b0[0];
    cd[1] = b1[1] - b0[1];
    cd[2] = b1[2] - b0[2];
    lenA = Vec_Length(ab);
    if (lenA < 0.001f) {
        lenA = 0.001f;
    }
    lenB = Vec_Length(cd);
    if (lenB < 0.001f) {
        lenB = 0.001f;
    }
    da[0] = ab[0] / lenA;
    da[1] = ab[1] / lenA;
    da[2] = ab[2] / lenA;
    db[0] = cd[0] / lenB;
    db[1] = cd[1] / lenB;
    db[2] = cd[2] / lenB;
    c = Vec_Dot(da, db);
    if (1.0f - Math_Absf(c) < 0.001f) {
        d[0] = a0[0] - b0[0];
        d[1] = a0[1] - b0[1];
        d[2] = a0[2] - b0[2];
        t = Vec_Dot(d, db);
        r = Math_Sqrtf(Vec_Dot(d, d) - t * t);
        if (t < 0.0f) {
            d[0] = a1[0] - b0[0];
            d[1] = a1[1] - b0[1];
            d[2] = a1[2] - b0[2];
            t2 = Vec_Dot(d, db);
            if (!(t2 < 0.0f)) {
                return r;
            }
            if (t < t2) {
                return Vec_Distance(a1, b0);
            } else {
                return Vec_Distance(a0, b0);
            }
        }
        if (t <= lenB) {
            return r;
        }
        d[0] = a1[0] - b1[0];
        d[1] = a1[1] - b1[1];
        d[2] = a1[2] - b1[2];
        t2 = Vec_Dot(d, db);
        if (t2 <= 0.0f) {
            return r;
        }
        t -= lenB;
        if (t < t2) {
            return Vec_Distance(a0, b1);
        } else {
            return Vec_Distance(a1, b1);
        }
    }
    d[0] = b0[0] - a0[0];
    d[1] = b0[1] - a0[1];
    d[2] = b0[2] - a0[2];
    s = Vec_Dot(d, da);
    t = Vec_Dot(d, db);
    den = 1.0f - c * c;
    sa = (s - t * c) / den;
    sb = (s * c - t) / den;
    if (sa < 0.0f) {
        ca = 0;
    } else if (sa <= lenA) {
        ca = 1;
    } else {
        ca = 2;
    }
    if (sb < 0.0f) {
        cb = 0;
    } else if (sb <= lenB) {
        cb = 1;
    } else {
        cb = 2;
    }
    switch (ca) {
    case 0:
        r1 = Seg_PointDistance(b0, b1, db, a0, lenB);
        switch (cb) {
        case 0:
            r2 = Seg_PointDistance(a0, a1, da, b0, lenA);
            break;
        case 1:
            return r1;
        case 2:
            r2 = Seg_PointDistance(a0, a1, da, b1, lenA);
            break;
        }
        break;
    case 1:
        switch (cb) {
        case 0:
            return Seg_PointDistance(a0, a1, da, b0, lenA);
        case 1:
            d[0] = sa * da[0];
            d[1] = sa * da[1];
            d[2] = sa * da[2];
            pa[0] = a0[0] + d[0];
            pa[1] = a0[1] + d[1];
            pa[2] = a0[2] + d[2];
            d[0] = sb * db[0];
            d[1] = sb * db[1];
            d[2] = sb * db[2];
            pb[0] = b0[0] + d[0];
            pb[1] = b0[1] + d[1];
            pb[2] = b0[2] + d[2];
            return Vec_Distance(pa, pb);
        case 2:
            return Seg_PointDistance(a0, a1, da, b1, lenA);
        }
    case 2:
        r1 = Seg_PointDistance(b0, b1, db, a1, lenB);
        switch (cb) {
        case 0:
            r2 = Seg_PointDistance(a0, a1, da, b0, lenA);
            break;
        case 1:
            return r1;
        case 2:
            r2 = Seg_PointDistance(a0, a1, da, b1, lenA);
            return 0.0f;
        }
        break;
    }
    if (r1 < r2) {
        return r1;
    }
    r1 = r2;
    return r1;
}

void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
f32 Math_Absf(f32 x);
f32 Vec_Distance(f32 *a, f32 *b);

/* Sphere (p, r) against capsule cap: 1 and the nearest point of the capsule's surface in out if they touch. */
s32 Capsule_SphereContact(f32 *p, Capsule *cap, f32 *out, f32 r)
{
    f32 dir[3];
    f32 d[3];
    f32 n[3];
    f32 perp[3];
    f32 foot[3];
    f32 q[3];
    f32 t;
    f32 rr;

    if (Sphere_Touch(p, cap->mid, r, cap->len) == 0) {
        return 0;
    }
    Vec_Copy(dir, cap->dir);
    d[0] = cap->a[0] - p[0];
    d[1] = cap->a[1] - p[1];
    d[2] = cap->a[2] - p[2];
    Vec_Cross(n, dir, d);
    Vec_Cross(perp, dir, n);
    Vec_Normalize(perp);
    t = Vec_Dot(perp, d);
    rr = r + cap->radius;
    if (!(Math_Absf(t) <= rr)) {
        return 0;
    }
    n[0] = t * perp[0];
    n[1] = t * perp[1];
    n[2] = t * perp[2];
    foot[0] = p[0] + n[0];
    foot[1] = p[1] + n[1];
    foot[2] = p[2] + n[2];
    d[0] = foot[0] - cap->a[0];
    d[1] = foot[1] - cap->a[1];
    d[2] = foot[2] - cap->a[2];
    if (Vec_Dot(d, dir) < 0.0f) {
        if (!(Vec_Distance(cap->a, p) <= rr)) {
            return 0;
        }
        d[0] = p[0] - cap->a[0];
        d[1] = p[1] - cap->a[1];
        d[2] = p[2] - cap->a[2];
        Vec_Normalize(d);
        d[0] *= cap->radius;
        d[1] *= cap->radius;
        d[2] *= cap->radius;
        out[0] = cap->a[0] + d[0];
        out[1] = cap->a[1] + d[1];
        out[2] = cap->a[2] + d[2];
        return 1;
    }
    d[0] = foot[0] - cap->b[0];
    d[1] = foot[1] - cap->b[1];
    d[2] = foot[2] - cap->b[2];
    dir[0] = -dir[0];
    dir[1] = -dir[1];
    dir[2] = -dir[2];
    if (Vec_Dot(d, dir) < 0.0f) {
        if (!(Vec_Distance(cap->b, p) <= rr)) {
            return 0;
        }
        d[0] = p[0] - cap->b[0];
        d[1] = p[1] - cap->b[1];
        d[2] = p[2] - cap->b[2];
        Vec_Normalize(d);
        d[0] *= cap->radius;
        d[1] *= cap->radius;
        d[2] *= cap->radius;
        out[0] = cap->b[0] + d[0];
        out[1] = cap->b[1] + d[1];
        out[2] = cap->b[2] + d[2];
        return 1;
    }
    if (Math_Absf(t) < 0.001f) {
        /* p on the axis: any direction perpendicular to it */
        Vec_Copy(q, foot);
        q[0] += 5.0f;
        d[0] = q[0] - cap->b[0];
        d[1] = q[1] - cap->b[1];
        d[2] = q[2] - cap->b[2];
        Vec_Cross(n, d, dir);
        if (Vec_Dot(n, n) < 0.001f) {
            foot[1] += 5.0f;
            d[0] = q[0] - cap->b[0];
            d[1] = q[1] - cap->b[1];
            d[2] = q[2] - cap->b[2];
            Vec_Cross(n, d, dir);
        }
        Vec_Cross(d, n, dir);
        Vec_Normalize(d);
    } else {
        d[0] = p[0] - foot[0];
        d[1] = p[1] - foot[1];
        d[2] = p[2] - foot[2];
        Vec_Normalize(d);
    }
    d[0] *= cap->radius;
    d[1] *= cap->radius;
    d[2] *= cap->radius;
    out[0] = foot[0] + d[0];
    out[1] = foot[1] + d[1];
    out[2] = foot[2] + d[2];
    return 1;
}


/* Sphere (p, r) against capsule cap: 1 and the push-out vector in out if they touch. */
s32 Capsule_PushSphere(Capsule *cap, f32 *p, f32 *out, f32 r)
{
    f32 dir[3];
    f32 d[3];
    f32 n[3];
    f32 perp[3];
    f32 rr;
    f32 t;
    f32 len;
    f32 k;
    f32 x, y, z;

    if (Sphere_Touch(p, cap->mid, r, cap->len) == 0) {
        return 0;
    }
    Vec_Copy(dir, cap->dir);
    d[0] = cap->a[0] - p[0];
    d[1] = cap->a[1] - p[1];
    d[2] = cap->a[2] - p[2];
    Vec_Cross(n, dir, d);
    Vec_Cross(perp, dir, n);
    Vec_Normalize(perp);
    rr = cap->radius + r;
    t = Vec_Dot(perp, d);
    if (!(Math_Absf(t) <= rr)) {
        return 0;
    }
    n[0] = t * perp[0];
    n[1] = t * perp[1];
    n[2] = t * perp[2];
    x = p[0] + n[0];
    y = p[1] + n[1];
    z = p[2] + n[2];
    d[0] = x - cap->a[0];
    d[1] = y - cap->a[1];
    d[2] = z - cap->a[2];
    if (Vec_Dot(d, dir) < 0.0f) {
        d[0] = cap->a[0] - p[0];
        d[1] = cap->a[1] - p[1];
        d[2] = cap->a[2] - p[2];
        len = Vec_Length(d);
        if (!(len <= rr)) {
            return 0;
        }
        Vec_Normalize(d);
        k = rr - len;
        out[0] = k * d[0];
        out[1] = k * d[1];
        out[2] = k * d[2];
        return 1;
    }
    d[0] = x - cap->b[0];
    d[1] = y - cap->b[1];
    d[2] = z - cap->b[2];
    dir[0] = -dir[0];
    dir[1] = -dir[1];
    dir[2] = -dir[2];
    if (Vec_Dot(d, dir) < 0.0f) {
        d[0] = cap->b[0] - p[0];
        d[1] = cap->b[1] - p[1];
        d[2] = cap->b[2] - p[2];
        len = Vec_Length(d);
        if (!(len <= rr)) {
            return 0;
        }
        Vec_Normalize(d);
        k = rr - len;
        out[0] = k * d[0];
        out[1] = k * d[1];
        out[2] = k * d[2];
        return 1;
    }
    if (t < 0.0f) {
        rr = -rr;
    }
    k = rr - t;
    out[0] = k * perp[0];
    out[1] = k * perp[1];
    out[2] = k * perp[2];
    return 1;
}


/* Sphere (p, r) against capsule cap: 1 and the point of the sphere nearest the axis in out if they touch. */
s32 Capsule_SphereContactOnSphere(Capsule *cap, f32 *p, f32 *out, f32 r)
{
    f32 dir[3];
    f32 d[3];
    f32 n[3];
    f32 perp[3];
    f32 rr;
    f32 t;
    f32 x, y, z;

    if (Vec_WithinDist(cap->len + r, p, cap->mid) == 0) {
        return 0;
    }
    Vec_Copy(dir, cap->dir);
    d[0] = cap->a[0] - p[0];
    d[1] = cap->a[1] - p[1];
    d[2] = cap->a[2] - p[2];
    Vec_Cross(n, dir, d);
    Vec_Cross(perp, dir, n);
    Vec_Normalize(perp);
    rr = cap->radius + r;
    t = Vec_Dot(perp, d);
    if (!(Math_Absf(t) <= rr)) {
        return 0;
    }
    n[0] = t * perp[0];
    n[1] = t * perp[1];
    n[2] = t * perp[2];
    x = p[0] + n[0];
    y = p[1] + n[1];
    z = p[2] + n[2];
    d[0] = x - cap->a[0];
    d[1] = y - cap->a[1];
    d[2] = z - cap->a[2];
    if (Vec_Dot(d, dir) < 0.0f) {
        d[0] = cap->a[0] - p[0];
        d[1] = cap->a[1] - p[1];
        d[2] = cap->a[2] - p[2];
        if (!(Vec_Length(d) <= rr)) {
            return 0;
        }
        Vec_Normalize(d);
        out[0] = r * d[0];
        out[1] = r * d[1];
        out[2] = r * d[2];
        out[0] += p[0];
        out[1] += p[1];
        out[2] += p[2];
        return 1;
    }
    d[0] = x - cap->b[0];
    d[1] = y - cap->b[1];
    d[2] = z - cap->b[2];
    dir[0] = -dir[0];
    dir[1] = -dir[1];
    dir[2] = -dir[2];
    if (Vec_Dot(d, dir) < 0.0f) {
        d[0] = cap->b[0] - p[0];
        d[1] = cap->b[1] - p[1];
        d[2] = cap->b[2] - p[2];
        if (!(Vec_Length(d) <= rr)) {
            return 0;
        }
        Vec_Normalize(d);
        out[0] = r * d[0];
        out[1] = r * d[1];
        out[2] = r * d[2];
        out[0] += p[0];
        out[1] += p[1];
        out[2] += p[2];
        return 1;
    }
    out[0] = -r * perp[0];
    out[1] = -r * perp[1];
    out[2] = -r * perp[2];
    out[0] += p[0];
    out[1] += p[1];
    out[2] += p[2];
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
/* Capsule a against capsule b: 1 and the push-out vector in out if they touch. The float
 * locals are declared in the order that gives the game's saved registers, and case 1 of the
 * outer switch has no break (its inner switch always returns: cb is 0-2). */
s32 Capsule_PushCapsule(Capsule *a, Capsule *b, f32 *out)
{
    f32 da[3];
    f32 db[3];
    f32 d[3];
    f32 e[3];
    f32 pa[3];
    f32 pb[3];
    f32 c[3];
    f32 t;
    f32 s;
    f32 lenA;
    f32 cs;
    f32 r1;
    f32 rr;
    f32 h;
    f32 lenB;
    f32 t3;
    f32 k;
    f32 r2;
    f32 den;
    f32 sa;
    f32 sb;
    f32 hh;
    u8 ca;
    u8 cb;

    if (Vec_WithinDist(a->len + b->len, a->mid, b->mid) == 0) {
        return 0;
    }
    rr = a->radius + b->radius;
    lenA = 2.0f * (a->len - a->radius);
    if (lenA < 0.001f) {
        lenA = 0.001f;
    }
    lenB = 2.0f * (b->len - b->radius);
    if (lenB < 0.001f) {
        lenB = 0.001f;
    }
    da[0] = a->dir[0] / lenA;
    da[1] = a->dir[1] / lenA;
    da[2] = a->dir[2] / lenA;
    db[0] = b->dir[0] / lenB;
    db[1] = b->dir[1] / lenB;
    db[2] = b->dir[2] / lenB;
    cs = Vec_Dot(da, db);
    if (1.0f - Math_Absf(cs) < 0.001f) {
        d[0] = b->a[0] - a->a[0];
        d[1] = b->a[1] - a->a[1];
        d[2] = b->a[2] - a->a[2];
        t = Vec_Dot(d, da);
        hh = Vec_Dot(d, d) - t * t;
        if (hh < 0.0f) {
            return 0;
        }
        h = Math_Sqrtf(hh);
        if (!(h <= rr)) {
            return 0;
        }
        if (h < 0.001f) {
            e[0] = 5.0f + da[0];
            e[1] = da[1];
            e[2] = da[2];
            Vec_Cross(c, da, e);
            if (Math_Absf(c[0]) < 0.001f && Math_Absf(c[1]) < 0.001f && Math_Absf(c[2]) < 0.001f) {
                e[1] += 5.0f;
                Vec_Cross(c, da, e);
            }
            Vec_Normalize(c);
            c[0] *= rr;
            c[1] *= rr;
            c[2] *= rr;
        } else {
            k = (rr - h) / h;
            c[0] = t * da[0];
            c[1] = t * da[1];
            c[2] = t * da[2];
            pa[0] = a->a[0] + c[0];
            pa[1] = a->a[1] + c[1];
            pa[2] = a->a[2] + c[2];
            c[0] = b->a[0] - pa[0];
            c[1] = b->a[1] - pa[1];
            c[2] = b->a[2] - pa[2];
            c[0] *= k;
            c[1] *= k;
            c[2] *= k;
        }
        if (t < 0.0f) {
            e[0] = b->b[0] - a->a[0];
            e[1] = b->b[1] - a->a[1];
            e[2] = b->b[2] - a->a[2];
            t3 = Vec_Dot(e, da);
            if (!(t3 < 0.0f)) {
                Vec_Copy(out, c);
                return 1;
            }
            if (t < t3) {
                r1 = Vec_Length(e);
                if (r1 <= rr) {
                    k = (rr - r1) / r1;
                    out[0] = k * e[0];
                    out[1] = k * e[1];
                    out[2] = k * e[2];
                    return 1;
                }
                return 0;
            }
            r1 = Vec_Length(d);
            if (r1 <= rr) {
                k = (rr - r1) / r1;
                out[0] = k * d[0];
                out[1] = k * d[1];
                out[2] = k * d[2];
                return 1;
            }
            return 0;
        }
        if (t <= lenB) {
            Vec_Copy(out, c);
            return 1;
        }
        e[0] = a->b[0] - b->a[0];
        e[1] = a->b[1] - b->a[1];
        e[2] = a->b[2] - b->a[2];
        t3 = Vec_Dot(e, db);
        if (t3 <= lenB) {
            Vec_Copy(out, c);
            return 1;
        }
        if (t < t3) {
            r1 = Math_Sqrtf(h * h + t * t);
            if (r1 <= rr) {
                k = (rr - r1) / r1;
                out[0] = k * d[0];
                out[1] = k * d[1];
                out[2] = k * d[2];
                return 1;
            }
            return 0;
        }
        r1 = Math_Sqrtf(h * h + t3 * t3);
        if (r1 <= rr) {
            k = (rr - r1) / r1;
            out[0] = k * e[0];
            out[1] = k * e[1];
            out[2] = k * e[2];
            return 1;
        }
        return 0;
    }
    d[0] = b->a[0] - a->a[0];
    d[1] = b->a[1] - a->a[1];
    d[2] = b->a[2] - a->a[2];
    s = Vec_Dot(d, da);
    t = Vec_Dot(d, db);
    den = 1.0f - cs * cs;
    sa = (s - t * cs) / den;
    sb = (s * cs - t) / den;
    if (sa < 0.0f) {
        ca = 0;
    } else if (sa <= lenA) {
        ca = 1;
    } else {
        ca = 2;
    }
    if (sb < 0.0f) {
        cb = 0;
    } else if (sb <= lenB) {
        cb = 1;
    } else {
        cb = 2;
    }
    switch (ca) {
    case 0:
        Seg_NearestPoint(b->a, b->b, db, a->a, pa, lenB);
        d[0] = pa[0] - a->a[0];
        d[1] = pa[1] - a->a[1];
        d[2] = pa[2] - a->a[2];
        r1 = Vec_Length(d);
        switch (cb) {
        case 0:
            Seg_NearestPoint(a->a, a->b, da, b->a, pb, lenA);
            e[0] = b->a[0] - pb[0];
            e[1] = b->a[1] - pb[1];
            e[2] = b->a[2] - pb[2];
            r2 = Vec_Length(e);
            if (r1 < r2) {
                if (r1 <= rr) {
                    Vec_PushOut(d, rr, r1, out);
                    return 1;
                }
                return 0;
            }
            if (r2 <= rr) {
                Vec_PushOut(e, rr, r2, out);
                return 1;
            }
            return 0;
        case 1:
            if (r1 <= rr) {
                Vec_PushOut(d, rr, r1, out);
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(a->a, a->b, da, b->b, pb, lenA);
            e[0] = b->b[0] - pb[0];
            e[1] = b->b[1] - pb[1];
            e[2] = b->b[2] - pb[2];
            r2 = Vec_Length(e);
            if (r1 < r2) {
                if (r1 <= rr) {
                    Vec_PushOut(d, rr, r1, out);
                    return 1;
                }
                return 0;
            }
            if (r2 <= rr) {
                Vec_PushOut(e, rr, r2, out);
                return 1;
            }
            return 0;
        }
        break;
    case 1:
        switch (cb) {
        case 0:
            Seg_NearestPoint(a->a, a->b, da, b->a, pa, lenA);
            d[0] = b->a[0] - pa[0];
            d[1] = b->a[1] - pa[1];
            d[2] = b->a[2] - pa[2];
            r1 = Vec_Length(d);
            if (r1 <= rr) {
                Vec_PushOut(d, rr, r1, out);
                return 1;
            }
            return 0;
        case 1:
            d[0] = sa * da[0];
            d[1] = sa * da[1];
            d[2] = sa * da[2];
            pa[0] = a->a[0] + d[0];
            pa[1] = a->a[1] + d[1];
            pa[2] = a->a[2] + d[2];
            d[0] = sb * db[0];
            d[1] = sb * db[1];
            d[2] = sb * db[2];
            pb[0] = b->a[0] + d[0];
            pb[1] = b->a[1] + d[1];
            pb[2] = b->a[2] + d[2];
            d[0] = pb[0] - pa[0];
            d[1] = pb[1] - pa[1];
            d[2] = pb[2] - pa[2];
            r1 = Vec_Length(d);
            if (r1 <= rr) {
                if (r1 < 0.001f) {
                    Vec_Cross(d, da, db);
                    out[0] = rr * d[0];
                    out[1] = rr * d[1];
                    out[2] = rr * d[2];
                } else {
                    Vec_PushOut(d, rr, r1, out);
                }
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(a->a, a->b, da, b->b, pa, lenA);
            d[0] = b->b[0] - pa[0];
            d[1] = b->b[1] - pa[1];
            d[2] = b->b[2] - pa[2];
            r1 = Vec_Length(d);
            if (r1 <= rr) {
                Vec_PushOut(d, rr, r1, out);
                return 1;
            }
            return 0;
        }
    case 2:
        Seg_NearestPoint(b->a, b->b, db, a->b, pa, lenB);
        d[0] = pa[0] - a->b[0];
        d[1] = pa[1] - a->b[1];
        d[2] = pa[2] - a->b[2];
        r1 = Vec_Length(d);
        switch (cb) {
        case 0:
            Seg_NearestPoint(a->a, a->b, da, b->a, pb, lenA);
            e[0] = b->a[0] - pb[0];
            e[1] = b->a[1] - pb[1];
            e[2] = b->a[2] - pb[2];
            r2 = Vec_Length(e);
            if (r1 < r2) {
                if (r1 <= rr) {
                    Vec_PushOut(d, rr, r1, out);
                    return 1;
                }
                return 0;
            }
            if (r2 <= rr) {
                Vec_PushOut(e, rr, r2, out);
                return 1;
            }
            return 0;
        case 1:
            if (r1 <= rr) {
                Vec_PushOut(d, rr, r1, out);
                return 1;
            }
            return 0;
        case 2:
            Seg_NearestPoint(a->a, a->b, da, b->b, pb, lenA);
            e[0] = b->b[0] - pb[0];
            e[1] = b->b[1] - pb[1];
            e[2] = b->b[2] - pb[2];
            r2 = Vec_Length(e);
            if (r1 < r2) {
                if (r1 <= rr) {
                    Vec_PushOut(d, rr, r1, out);
                    return 1;
                }
                return 0;
            }
            if (r2 <= rr) {
                Vec_PushOut(e, rr, r2, out);
                return 1;
            }
            return 0;
        }
        break;
    }
    return 0;
}

void Vec_PushOut(f32 *dir, f32 fparg0, f32 fparg1, f32 *out)
{
    f32 depth;
    f32 k;
    f32 var_f13;

    var_f13 = fparg1;
    if (var_f13 != 0.0f) {
        depth = fparg0 - var_f13;
        if (var_f13 < 0.001f) {
            var_f13 = 0.001f;
        }
        k = depth / var_f13;
        *(f32 *)((u8 *)out + 0) = k * *(f32 *)((u8 *)dir + 0);
        *(f32 *)((u8 *)out + 4) = (f32) (k * *(f32 *)((u8 *)dir + 4));
        *(f32 *)((u8 *)out + 8) = (f32) (k * *(f32 *)((u8 *)dir + 8));
    }
}

void Vec_Copy(f32 *dst, f32 *src);
void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
/* Projects b - a on the direction dir: out = dir * (dot + r); 0 when the projection is behind by more than r. */
s32 Vec_ProjectOnDir(f32 *a, f32 *b, f32 *dir, f32 *out, f32 r)
{
    f32 n[4];                   /* a quadword for the VU0 vector functions */
    f32 d[3];
    f32 t;

    d[0] = b[0] - a[0];
    d[1] = b[1] - a[1];
    d[2] = b[2] - a[2];
    Vec_Copy(n, dir);
    Vec_Normalize(n);
    t = Vec_Dot(n, d);
    if (t <= 0.0f && -t > r) {
        return 0;
    }
    t += r;
    out[0] = t * n[0];
    out[1] = t * n[1];
    out[2] = t * n[2];
    return 1;
}

void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);

/* Where p lies against the capsule of radius r around seg: 2 within r of both ends, 1 of
 * one end, else 1 if within r of the segment between them, 0 outside. */
u8 Capsule_Classify(Segment *seg, f32 *p, f32 r)
{
    f32 dir[3];
    f32 d[3];
    f32 *dy;
    f32 *dz;
    s32 ina;
    s32 inb;
    f32 t;
    u8 in;

    if (Vec_WithinDist(r + seg->len, seg->mid, p) == 0) {
        return 0;
    }
    ina = Vec_WithinDist(r, seg->a, p) != 0;
    inb = Vec_WithinDist(r, seg->b, p) != 0;
    if (ina && inb) {
        return 2;
    }
    if (ina || inb) {
        return 1;
    }
    Vec_Copy(dir, seg->dir);
    Vec_Normalize(dir);
    dy = &d[1];
    dz = &d[2];
    d[0] = p[0] - seg->a[0];
    *dy = p[1] - seg->a[1];
    *dz = p[2] - seg->a[2];
    t = Vec_Dot(d, dir);
    if (t < 0.0f) {
        return 0;
    }
    if (!(t <= 2.0f * seg->len)) {
        return 0;
    }
    in = d[0] * d[0] + *dy * *dy + *dz * *dz - t * t <= r * r;
    return in;
}

f32 Vec_Dot(f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
f32 Vec_Distance(f32 *a, f32 *b);
asm f32 Math_Sqrtf(f32 x);
/* The distance from p to the segment from a (direction dir, a unit vector, length len) to b. */
f32 Seg_PointDistance(f32 *a, f32 *b, f32 *dir, f32 *p, f32 len)
{
    f32 d[3];
    f32 t;
    f32 r;

    d[0] = p[0] - a[0];
    d[1] = p[1] - a[1];
    d[2] = p[2] - a[2];
    t = Vec_Dot(d, dir);
    if (t < 0.0f) {
        r = Vec_Length(d);
    } else if (t > len) {
        r = Vec_Distance(p, b);
    } else {
        r = Math_Sqrtf(Vec_Dot(d, d) - t * t);
    }
    return r;
}

f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Copy(f32 *dst, f32 *src);
void Seg_NearestPoint(f32 *a, f32 *b, f32 *dir, f32 *p, f32 *out, f32 len)
{
    f32 d[3];
    f32 *dy = &d[1];
    f32 *dz = &d[2];
    f32 t;

    d[0] = p[0] - a[0];
    *dy = p[1] - a[1];
    *dz = p[2] - a[2];
    t = Vec_Dot(d, dir);
    if (t < 0.0f) {
        Vec_Copy(out, a);
    } else if (t > len) {
        Vec_Copy(out, b);
    } else {
        d[0] = t * dir[0];
        *dy = t * dir[1];
        *dz = t * dir[2];
        out[0] = a[0] + d[0];
        out[1] = a[1] + *dy;
        out[2] = a[2] + *dz;
    }
}

/* 1 when p is within radius r of c in x-z and, when top > 0, between c.y + bottom and c.y + top. */
s32 Cyl_Contains(f32 *p, f32 *c, f32 r, f32 top, f32 bottom)
{
    f32 dx;
    f32 dz;

    if (top > 0.0f && (p[1] > c[1] + top || p[1] < c[1] + bottom)) {
        return 0;
    }
    dx = c[0] - p[0];
    dz = c[2] - p[2];
    if (dx * dx + dz * dz <= r * r) {
        return 1;
    }
    return 0;
}

f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Normalize(f32 *v);
f32 Math_Absf(f32 x);
/* 1 when pos is within height above a, within radius of the vertical plane through a and b, and between a and b. */
u8 Slab_Contains(f32 *pos, f32 *a, f32 *b, f32 height, f32 radius)
{
    f32 n[3];
    f32 ab[3];
    f32 ap[3];
    f32 *apy;
    f32 *apz;

    if (pos[1] < a[1] || !(pos[1] <= a[1] + height)) {
        return 0;
    }
    ab[0] = b[0] - a[0];
    ab[1] = b[1] - a[1];
    ab[2] = b[2] - a[2];
    n[0] = -height * ab[2];
    n[1] = 0.0f;
    n[2] = ab[0] * height;
    Vec_Normalize(n);
    apy = &ap[1];
    apz = &ap[2];
    ap[0] = pos[0] - a[0];
    *apy = pos[1] - a[1];
    *apz = pos[2] - a[2];
    if (!(Math_Absf(Vec_Dot(n, ap)) <= radius)) {
        return 0;
    }
    if (Vec_Dot(ab, ap) < 0.0f) {
        return 0;
    }
    ap[0] = pos[0] - b[0];
    *apy = pos[1] - b[1];
    *apz = pos[2] - b[2];
    return !(Vec_Dot(ab, ap) > 0.0f);
}

void Vec_RotateY(f32 *v, f32 a);
f32 Seg_SegDistance(f32 *a0, f32 *a1, f32 *b0, f32 *b1);
/* Capsule cap against a rectangle (half width w, half height h) in the XY
 * plane through c, turned by ang about Y: 1 if they touch. */
u8 Capsule_HitsRect(Capsule *cap, f32 *c, u16 ang, f32 h, f32 w)
{
    f32 pa[3];
    f32 pb[3];
    f32 d[3];
    f32 e0[3];
    f32 e1[3];
    f32 yhi;
    f32 a;
    f32 ylo;
    f32 xhi;
    f32 xlo;

    yhi = c[1] + h;
    ylo = c[1] - h;
    if ((!(cap->a[1] - cap->radius <= yhi) && !(cap->b[1] - cap->radius <= yhi))
        || (cap->a[1] + cap->radius < ylo && cap->b[1] + cap->radius < ylo)) {
        return 0;
    }
    a = (6.2831855f * -ang) / 65536.0f;
    d[0] = cap->a[0] - c[0];
    d[1] = cap->a[1] - c[1];
    d[2] = cap->a[2] - c[2];
    Vec_RotateY(d, a);
    pa[0] = c[0] + d[0];
    pa[1] = c[1] + d[1];
    pa[2] = c[2] + d[2];
    d[0] = cap->b[0] - c[0];
    d[1] = cap->b[1] - c[1];
    d[2] = cap->b[2] - c[2];
    Vec_RotateY(d, a);
    pb[0] = c[0] + d[0];
    pb[1] = c[1] + d[1];
    pb[2] = c[2] + d[2];
    if ((!(pa[2] - cap->radius <= c[2]) && !(pb[2] - cap->radius <= c[2]))
        || (pa[2] + cap->radius < c[2] && pb[2] + cap->radius < c[2])) {
        return 0;
    }
    xhi = c[0] + w;
    xlo = c[0] - w;
    if ((!(pa[0] - cap->radius <= xhi) && !(pb[0] - cap->radius <= xhi))
        || (pa[0] + cap->radius < xlo && pb[0] + cap->radius < xlo)) {
        return 0;
    }
    if (((pa[0] <= xhi && !(pa[0] < xlo)) || (pb[0] <= xhi && !(pb[0] < xlo)))
        && ((pa[1] <= yhi && !(pa[1] < ylo)) || (pb[1] <= yhi && !(pb[1] < ylo)))) {
        return 1;
    }
    e0[0] = xlo;
    e0[1] = ylo;
    e0[2] = c[2];
    e1[0] = xlo;
    e1[1] = yhi;
    e1[2] = c[2];
    if (Seg_SegDistance(pa, pb, e0, e1) <= cap->radius) {
        return 1;
    }
    e1[0] = xhi;
    e1[1] = ylo;
    if (Seg_SegDistance(pa, pb, e0, e1) <= cap->radius) {
        return 1;
    }
    e0[0] = xhi;
    e0[1] = yhi;
    if (Seg_SegDistance(pa, pb, e0, e1) <= cap->radius) {
        return 1;
    }
    e1[0] = xlo;
    e1[1] = yhi;
    return Seg_SegDistance(pa, pb, e0, e1) <= cap->radius;
}

void Vec_Copy(f32 *dst, f32 *src);
void Vec_Normalize(f32 *v);
f32 Vec_Dot(f32 *a, f32 *b);
f32 Math_Absf(f32 x);
/* Segment s (ends s[0..2], s[3..5], radius s[6]) against the plane through p
 * with normal nrm: the crossing point, or the near end pushed back onto the
 * plane when that end is within the radius. */
s32 Seg_ClipPlane(f32 *s, f32 *p, f32 *nrm, f32 *out)
{
    f32 va[3];
    f32 vb[3];
    f32 dir[3];
    f32 da;
    f32 db;
    f32 aa;
    f32 ab;
    f32 t;
    f32 x;
    f32 y;
    f32 z;

    va[0] = s[0] - p[0];
    va[1] = s[1] - p[1];
    va[2] = s[2] - p[2];
    vb[0] = s[3] - p[0];
    vb[1] = s[4] - p[1];
    vb[2] = s[5] - p[2];
    Vec_Copy(dir, nrm);
    Vec_Normalize(dir);
    da = Vec_Dot(dir, va);
    db = Vec_Dot(dir, vb);
    aa = Math_Absf(da);
    ab = Math_Absf(db);
    if ((da > 0.0f && db < 0.0f) || (da < 0.0f && db > 0.0f)) {
        t = aa / (aa + ab);
        x = s[3] - s[0];
        y = s[4] - s[1];
        z = s[5] - s[2];
        x *= t;
        out[0] = s[0] + x;
        y *= t;
        out[1] = s[1] + y;
        z *= t;
        out[2] = s[2] + z;
        return 1;
    }
    if (aa <= ab) {
        if (aa <= s[6]) {
            t = -aa;
            x = t * dir[0];
            y = t * dir[1];
            z = t * dir[2];
            out[0] = s[0] + x;
            out[1] = s[1] + y;
            out[2] = s[2] + z;
        }
    } else if (ab <= s[6]) {
        t = -ab;
        x = t * dir[0];
        y = t * dir[1];
        z = t * dir[2];
        out[0] = s[3] + x;
        out[1] = s[4] + y;
        out[2] = s[5] + z;
    }
    return 0;
}

s32 Seg_ClipPlane(f32 *s, f32 *p, f32 *nrm, f32 *out);

s32 Seg_ClipYRange(void *s, f32 lo, f32 hi, void *p, f32 *nrm, void *out)
{
    f32 y = *(f32 *)((u8 *)s + 4);

    if (!(y <= lo || *(f32 *)((u8 *)s + 0x10) <= lo) || (y < hi && *(f32 *)((u8 *)s + 0x10) < hi)) {
        return 0;
    }
    return Seg_ClipPlane(s, p, nrm, out);
}

/* Clears the 16 bytes at +0x3F9 and the bytes +0x412, +0x414 of a player or monster. */
static void Obj_ClearHitMarks(void *obj)
{
    ObjHead *o = obj;
    s16 i;

    o->hitPhase = 0;
    for (i = 0; i < 16; i++) {
        o->hitIds[i] = 0;
    }
    o->hitIdNext = 0;
}

void HitId_Reset(void);
extern Em gEms[];
void Hit_ResetAll(void)
{
    s32 i;

    HitId_Reset();
    for (i = 0; i < 4; i++) {
        Obj_ClearHitMarks(&gPlayers[i]);
    }
    for (i = 0; i < 20; i++) {
        Obj_ClearHitMarks(&gEms[i]);
    }
}

void ObjAttach_HitAll(void);
/* Clears the hit and status buildup state of the players and the monsters. */
void Hit_ClearStatusAll(void)
{
    Player *p;
    s32 i;
    s32 j;
    Em *q;
    s32 k;

    for (i = 0, p = gPlayers; i < 4; i++, p++) {
        p->hitReceived = 0;
        p->status5Hit = 0;
        for (j = 0; j < 8; j++) {
            p->hitDamage[j] = 0;
            p->partFlags[j] = 0;
        }
        p->status2Hit = 0;
        p->status1Hit = 0;
        p->status4Hit = 0;
        p->poisonHit = 0;
        p->mashBuildupHit = 0;
        p->status6Hit = 0;
        p->status7Hit = 0;
        p->status8Hit = 0;
        p->hitAttrs = 0;
    }
    for (k = 0, q = gEms; k < 20; k++, q++) {
        q->hitReceived = 0;
        q->status5Hit = 0;
        for (j = 0; j < 8; j++) {
            q->hitPartDamage[j] = 0;
            q->partFlags[j] = 0;
        }
        q->status2Hit = 0;
        q->status1Hit = 0;
        q->status4Hit = 0;
        q->poisonHit = 0;
        q->mashBuildupHit = 0;
        q->hitAttrs = 0;
        q->lastAttackKind = 0;
    }
    ObjAttach_HitAll();
}

/* Steps *cur past a hit shape, or past a group header and the zone records it holds when the monster's fxFlags has the bits of its kind. */
void HitList_Next(Em *p, HitShape **cur)
{
    HitShape *e = *cur;

    if (p->fxFlags & e->kind) {
        *cur = e + (e->zone + 1);
    } else {
        *cur = e + 1;
    }
}

void HitList_NextForObj(ObjAttach *o, HitShape **pp)
{
    HitShape *e;

    if (o->unk008 == 0xFF || o->isEm == 0) {
        *pp += 1;
        return;
    }
    e = *pp;
    if (((Em *)o->owner)->fxFlags & e->kind) {
        *pp = e + (e->zone + 1);
    } else {
        *pp = e + 1;
    }
}

s32 Quest_IsVersus(void);
/* Applies the status effects of hit o (elementFlags, elementPower) to its target, a player or a monster
 * read as a Player (the status 6 to 8 fields are the player's): the hit amounts and timers. */
void Em_ApplyHitStatus(Player *t, ObjAttach *o)
{
    Quest_IsVersus();
    if (o->elementFlags & 1) {
        t->status2Hit += o->elementPower;
    }
    if (o->elementFlags & 2) {
        t->poisonHit += o->elementPower;
    }
    if (o->elementFlags & 4) {
        t->status4Hit += o->elementPower;
    }
    if (o->elementFlags & 8) {
        t->status1Hit += o->elementPower;
    }
    if (o->elementFlags & 0x100) {
        t->mashBuildupHit += o->elementPower;
    }
    if (o->elementFlags & 0x400) {
        t->status6Hit = 0x708;
    }
    if (o->elementFlags & 0x1000) {
        t->status7Hit = 0x708;
    }
    if (o->elementFlags & 0x800) {
        t->status8Hit = 10;
    }
}

void StageObj_K29_CreateAt(f32 *src, u8 k, u8 c, f32 scale);
void Effect02_SpawnForPlayer(Player *p, u8 type, f32 *pos);
void StageObj_K10_CreateOnEm(Em *em, ObjAttach *o, u8 a, s16 b, f32 *pos, f32 scale);
s32 Player_CanGuard(Player *p);
s32 HeavyBowgun_HasShield(Player *p);
s32 Player_HasSkill(Player *p, u8 skill);
u32 Player_TestFlag(void *obj, u32 flag);
s32 Player_InGuardArc(u16 a, u16 b);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void StageObj_K12_CreateAt(f32 *pos, s32 k, u8 c);
void StageObj_K0F_Create(f32 *pos, s32 k, u16 v, void *owner, f32 scale);
void StageObj_K29_CreateAtDefault(f32 *src, u8 k, f32 scale);
/* unit's own declaration: the definition (game unk_868110.c) is (Em *em, f32 *pos, u8 k, s16 mode, s16 part, f32 scale); this unit set f12 (scale) before the part (t0): in the definition's order ObjAttach_HitEffects differs by 32 words */
void StageObj_K10_CreateOnEmAt(Em *em, f32 *pos, s32 a, s32 b, f32 scale, s16 kind);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
void Effect02_Spawn(u8 *owner, s16 kind, u8 type, u8 sub, f32 *pos, f32 scale);
void Effect06_SpawnWeapon(Player *p, s16 type, u8 sub);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
void StageObj_K29_CreateOnPlayerDefault(Player *p, u8 k, f32 scale);
void StageObj_K0B_Create(void *arg);
/* The effects and sounds of a hit of o (its element kind, +0x6F & 0x3F) on the player e at out. */
void ObjAttach_HitEffects(ObjAttach *o, Player *p, ObjHead *e, f32 *out)
{
    Player *q = (Player *)e;

    if ((o->hitFx & 0x3F) != 0 && o->area == gScene.area) {
        if (p != NULL && p->isEm == 0) {
            if (o->hitFx & 0x80) {
                StageObj_K29_CreateAt(out, 0x10, 0, 1.0f);
            } else if (o->hitFx & 0x40) {
                StageObj_K29_CreateAt(out, 0x10, 1, 1.0f);
            }
        }
        if (q->isEm == 1 && q->weaponType == 0x1D) {
            Effect02_SpawnForPlayer(q, 0, out);
            StageObj_K10_CreateOnEm((Em *) q, o, 6, 1, out, 1.0f);
            return;
        }
        if (q->isEm == 0 && Player_CanGuard(q) == 1 && (!(q->hitAttrs & 0x80) || Player_HasSkill(q, 0x29) == 1)) {
            if (Player_TestFlag(q, 0x8000) != 0
                || ((Player_HasSkill(q, 0x2C) == 1 || HeavyBowgun_HasShield(q) == 1) && q->weaponOut != 0 && q->prevFreeState != 0)) {
                if (Player_InGuardArc(q->rot[1], q->hitAngle) == 1 && q->stamina >= 0x4B) {
                    Effect02_SpawnForPlayer(q, 0, out);
                    StageObj_K10_CreateOnEm((Em *) q, o, 6, 1, out, 1.0f);
                    return;
                }
            }
        }
        switch (o->hitFx & 0x3F) {
        case 28:
            StageObj_K0E_CreateScaled(out, 0, 1.0f);
            return;
        case 18:
            StageObj_K12_CreateAt(out, 8, 0);
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
            switch (o->hitFx & 0x3F) {
            case 6:
                StageObj_K12_CreateAt(out, 7, 1);
                break;
            case 7:
                StageObj_K12_CreateAt(out, 7, 2);
                break;
            case 8:
                StageObj_K12_CreateAt(out, 7, 3);
                break;
            case 17:
                StageObj_K12_CreateAt(out, 7, 5);
                break;
            case 20:
                StageObj_K12_CreateAt(out, 7, 6);
                break;
            case 4:
            case 5:
                switch (o->hitKind) {
                case 0:
                case 2:
                case 3:
                    if ((o->hitFx & 0x3F) == 4) {
                        Effect02_SpawnForPlayer(q, 7, out);
                    } else {
                        Effect02_SpawnForPlayer(q, 5, out);
                    }
                    Effect02_SpawnForPlayer(q, 6, out);
                    break;
                }
            case 1:
            case 27:
                if (p != NULL && p->isEm == 0) {
                    if (o->elementFlags & 0x10) {
                        StageObj_K0F_Create(out, 0, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 0x20) {
                        StageObj_K0F_Create(out, 2, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 0x40) {
                        StageObj_K0F_Create(out, 3, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 0x80) {
                        StageObj_K0F_Create(out, 8, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 0x200) {
                        StageObj_K29_CreateAtDefault(out, 0xC, 1.0f);
                    } else if (o->elementFlags & 2) {
                        StageObj_K0F_Create(out, 1, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 4) {
                        StageObj_K0F_Create(out, 7, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 1) {
                        StageObj_K0F_Create(out, 6, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 0x1000) {
                        StageObj_K0F_Create(out, 6, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 0x400) {
                        StageObj_K0F_Create(out, 1, q->hitAngle, p, 1.0f);
                    } else if (o->elementFlags & 0x800) {
                        StageObj_K0F_Create(out, 7, q->hitAngle, p, 1.0f);
                    }
                }
                break;
            }
            switch (o->hitKind) {
            case 1:
                if (o->unk008 != 0xFF) {
                    Effect02_SpawnForPlayer(q, 0, out);
                    StageObj_K10_CreateOnEm((Em *) q, o, 6, 1, out, 1.0f);
                }
                break;
            default:
            case 0:
                StageObj_K10_CreateOnEm((Em *) q, o, 0, 0, out, 0.8f);
                StageObj_K10_CreateOnEmAt((Em *) q, out, 2, 0, 0.8f, o->hitFx & 0x3F);
                break;
            case 2:
                StageObj_K10_CreateOnEm((Em *) q, o, 0, 2, out, 0.8f);
                StageObj_K10_CreateOnEmAt((Em *) q, out, 2, 2, 0.8f, o->hitFx & 0x3F);
                break;
            case 3:
                StageObj_K10_CreateOnEm((Em *) q, o, 0, 3, out, 1.0f);
                StageObj_K10_CreateOnEmAt((Em *) q, out, 5, 3, 1.0f, o->hitFx & 0x3F);
                Effect02_SpawnForOwner(q, 0, 4, 3, out, 1.0f);
                break;
            }
            if ((o->hitFx & 0x3F) == 27 && o->hitKind != 1) {
                Effect02_SpawnForPlayer(q, 0xC, out);
                return;
            }
            break;
        case 9:
        case 11:
        case 13:
        case 15:
            StageObj_K12_CreateAt(out, 7, 0);
        case 10:
        case 12:
        case 14:
        case 16:
            switch (o->hitFx & 0x3F) {
            case 9:
            case 10:
                if (o->unk008 != 0xFF) {
                    Effect02_SpawnForPlayer(q, 0, out);
                    StageObj_K10_CreateOnEm((Em *) q, o, 6, 1, out, 1.0f);
                    return;
                }
                break;
            case 15:
            case 16:
                StageObj_K10_CreateOnEm((Em *) q, o, 0, 0, out, 0.8f);
                StageObj_K10_CreateOnEmAt((Em *) q, out, 2, 0, 0.8f, o->hitFx & 0x3F);
                return;
            case 11:
            case 12:
                StageObj_K10_CreateOnEm((Em *) q, o, 0, 2, out, 0.8f);
                StageObj_K10_CreateOnEmAt((Em *) q, out, 2, 2, 0.8f, o->hitFx & 0x3F);
                return;
            case 13:
            case 14:
                StageObj_K10_CreateOnEm((Em *) q, o, 0, 3, out, 1.0f);
                StageObj_K10_CreateOnEmAt((Em *) q, out, 5, 3, 1.0f, o->hitFx & 0x3F);
                Effect02_Spawn((u8 *)q, o->hitFx & 0x3F, 4, 3, out, 1.0f);
                return;
            }
            break;
        case 2:
            StageObj_K10_CreateOnEm((Em *) q, o, 8, 0, out, 1.0f);
            StageObj_K10_CreateOnEm((Em *) q, o, 9, 0, out, 1.0f);
            return;
        case 3:
            Effect06_SpawnWeapon(q, 0, 0);
            return;
        case 23:
            if (q->isEm == 0) {
                Effect06_SpawnWeapon(q, 0, 1);
                return;
            }
            break;
        case 21:
            if (q->isEm == 0) {
                Effect06_SpawnWeapon(q, 0, 2);
                return;
            }
            break;
        case 22:
            if (q->isEm == 0) {
                Effect06_SpawnWeapon(q, 0, 3);
                return;
            }
            break;
        case 29:
            if (q->isEm == 0) {
                Effect06_SpawnAtBone(q, 2, 0x19, 10, 4.0f);
                Em_GetBonePos(q, 10, (f32 *)&out);
                Sound_PlayPlayerSe(q, 0x72, 0, (f32 *)&out, 1, 0);
                return;
            }
            break;
        case 19:
            Effect06_SpawnWeapon(q, 9, 0);
            return;
        case 24:
            if (p != NULL && p->isEm == 0) {
                if (o->elementFlags & 0x10) {
                    StageObj_K0F_Create(out, 0, q->hitAngle, p, 1.5f);
                } else if (o->elementFlags & 0x20) {
                    StageObj_K0F_Create(out, 2, q->hitAngle, p, 1.2f);
                } else if (o->elementFlags & 0x40) {
                    StageObj_K0F_Create(out, 3, q->hitAngle, p, 1.2f);
                } else if (o->elementFlags & 0x200) {
                    StageObj_K29_CreateAtDefault(out, 0xC, 1.0f);
                } else if (o->elementFlags & 0x80) {
                    StageObj_K0F_Create(out, 8, q->hitAngle, p, 1.2f);
                } else if (o->elementFlags & 2) {
                    StageObj_K0F_Create(out, 1, q->hitAngle, p, 1.2f);
                } else if (o->elementFlags & 4) {
                    StageObj_K0F_Create(out, 7, q->hitAngle, p, 1.2f);
                } else if (o->elementFlags & 1) {
                    StageObj_K0F_Create(out, 6, q->hitAngle, p, 1.2f);
                }
            }
            StageObj_K10_CreateOnEm((Em *) q, o, 0, 2, out, 0.8f);
            StageObj_K10_CreateOnEmAt((Em *) q, out, 2, 2, 0.8f, 12);
            return;
        case 25:
            if (q->isEm == 0) {
                StageObj_K29_CreateOnPlayerDefault(q, 1, 1.0f);
                return;
            }
            break;
        case 26:
            if (q->isEm == 0) {
                StageObj_K0B_Create(q);
                return;
            }
            break;
        case 30:
            StageObj_K12_CreateAt(out, 7, 8);
            return;
        case 31:
            StageObj_K12_CreateAt(out, 9, 1);
            return;
        case 32:
            StageObj_K12_CreateAt(out, 9, 2);
            return;
        case 33:
            StageObj_K12_CreateAt(out, 9, 3);
            break;
        }
    }
}

u32 Player_TestFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);
void func_003B09B0(Player *unused0, u8 unused1);
/* The player's hitDelay/hitFrames timers by its hitPhase (1: waiting, 2: running; func_003B09B0 restarts it), as ObjAttach_UpdateTimers for objects. */
void Player_UpdateHitTimers(Player *p)
{
    u8 *b = (u8 *)p;

    if (Player_TestFlag(p, 0x800) != 0) {
        p->hitPhase = 1;
        Player_ClearFlag(p, 0x40000);
    }
    if (p->hitStop != 0) {
        return;
    }
    if (Player_TestFlag(p, 1) == 0) {
        p->hitPhase = 0;
        Player_ClearFlag(p, 0x40000);
    }
    switch (p->hitPhase) {
    case 0:
        break;
    case 1:
        if (p->hitDelay-- != 0) {
            break;
        }
        p->hitDelay = 0;
        p->hitPhase++;
    case 2:
        if (p->hitFrames == 0) {
            if (b[0x3E7] != 0) {
                func_003B09B0(p, b[0x3E7]);
                Player_ClearFlag(p, 0x40000);
                if (p->hitDelay == 0) {
                    p->hitPhase = 2;
                    p->hitFrames--;
                } else {
                    p->hitPhase = 1;
                }
            } else {
                p->hitPhase = 0;
            }
        } else {
            p->hitFrames--;
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
/* Tests an object's hit shapes (the list at +0x88) against those of the
 * other objects in its area (+0x8C); the first object hit is marked (+0xB4)
 * with the attacker's owner (+0xC4). Some kinds (+0x63) don't hit. */
void ObjAttach_HitObjects(ObjAttach *o)
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
    ObjAttach *e;
    void *owner;
    void *eowner;
    s16 a;
    s16 b;
    s16 hit;

    e = gObjAttachList;
    switch (o->attackKind) {
    case 6:
    case 9:
    case 0x13:
    case 4:
    case 0xF:
    case 0x15:
    case 5:
    case 0xE:
        return;
    }
    if (o->isEm == 0) {
        owner = &gPlayers[o->idx];
    } else {
        owner = &gEms[o->idx];
    }
    for (; e != NULL; e = e->next) {
        if (o->area != e->area || e->active == 0) {
            continue;
        }
        if (e->isEm == o->isEm && o->unk008 != 0xFF && e->unk008 != 0xFF) {
            continue;
        }
        if (e->hurtList == 0 || e->wasHit != 0) {
            continue;
        }
        if (Player_TestFlags(e, 0x100) != 0 && o->isEm != 0 && Player_TestFlags(o, 0x200) == 0) {
            continue;
        }
        list = o->hitList;
        if (e->isEm == 0) {
            eowner = &gPlayers[e->idx];
        } else {
            eowner = &gEms[e->idx];
        }
        while (list->bone != -1) {
            switch (list->bone) {
            case 0x7F:
                a = HitShape_Place(o->pos, list, &segA, &sphA, o->hitScale);
                break;
            case 0x7E:
                a = Capsule_FromPoints(o->pos, o->pos2, list, &segA, o->hitScale);
                break;
            case 0x7D:
                HitList_NextForObj(o, &list);
                continue;
            default:
                a = BoneHit_Place(owner, list, &segA, &sphA);
                break;
            }
            if (a != 0) {
                Capsule_Copy(&segA, &capA);
            }
            elist = e->hurtList;
            while (elist->bone != -1) {
                switch (elist->bone) {
                case 0x7F:
                    b = HitShape_Place(e->pos, elist, &segB, &sphB, 1.0f);
                    break;
                case 0x7E:
                    b = Capsule_FromPoints(e->pos, e->pos2, elist, &segB, 1.0f);
                    break;
                case 0x7D:
                    HitList_NextForObj(e, &elist);
                    continue;
                default:
                    b = BoneHit_Place(eowner, elist, &segB, &sphB);
                    break;
                }
                if (b != 0) {
                    Capsule_Copy(&segB, &capB);
                }
                if (a == 1) {
                    if (b == 1) {
                        hit = Capsule_ContactCapsule(&capA, &capB, out);
                    } else {
                        hit = (u8)Capsule_SphereContactOnSphere(&capA, sphB.pos, out, sphB.radius);
                    }
                } else if (b == 1) {
                    hit = (u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
                } else {
                    hit = (u8)Spheres_Touch(sphA.radius, sphB.radius, sphA.pos, sphB.pos, out);
                }
                if (hit != 0) {
                    e->wasHit = 1;
                    if (o->unk008 == 0xFF) {
                        e->hitBy = NULL;
                    } else if (o->isEm == 0) {
                        e->hitBy = &gPlayers[e->idx];
                    } else {
                        e->hitBy = &gEms[e->idx];
                    }
                    goto next;
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
void ObjAttach_TryHitPlayer(ObjAttach *o, Player *p, ObjHead *e);
/* For every active attached object: hits the monsters and the players in its area that it can hit (not its owner unless allowed, not twice with one hit id), by its attack kind (+0x63). */
void ObjAttach_HitAll(void)
{
    ObjAttach *o;
    void *owner;
    Em *e;
    s32 i;
    s32 k;
    s32 j;
    Player *p;

    for (o = gObjAttachList; o != NULL; o = o->next) {
        if (o->active == 0) {
            continue;
        }
        o->unk01E = 0;
        if (o->hitPhase == 2) {
            ObjAttach_HitObjects(o);
        }
        if (o->unk008 == 0xFF) {
            owner = NULL;
        } else if (o->isEm == 0) {
            owner = &gPlayers[o->idx];
        } else {
            owner = &gEms[o->idx];
        }
        for (i = 0, e = gEms; i < 20; i++, e++) {
            if (e->active != 0 && e->area == o->area && Player_TestFlags(o, 0x80) == 0) {
                if (o->unk008 == 0xFF || owner != e || Player_TestFlags(o, 4) != 0) {
                    for (k = 0; k < 16; k++) {
                        if (o->hitId == e->hitIds[k]) {
                            goto next_em;
                        }
                    }
                    if (e->hitReceived == 0) {
                        switch (o->attackKind) {
                        case 0x13:
                            if (e->stance == 4 || e->stateArmor > 0) {
                                break;
                            }
                        case 6:
                        case 9:
                            if (e->stance == 2 || e->hp <= 0) {
                                break;
                            }
                        default:
                            ObjAttach_TryHitPlayer(o, owner, (ObjHead *)e);
                            break;
                        }
                    }
                }
            }
        next_em:;
        }
        for (p = gPlayers, j = 0; j < gScene.playerCount; j++, p++) {
            if (p->active != 0 && p->area == o->area && Player_TestFlags(o, 0x400) == 0) {
                if (!(o->attrs & 1) || (Players_AnySide() != 0 && Side_Match(o->side, p->side, 1) != 1)) {
                    if (o->unk008 == 0xFF || owner != p || Player_TestFlags(o, 4) != 0) {
                        for (k = 0; k < 16; k++) {
                            if (o->hitId == p->hitIds[k]) {
                                goto next_pl;
                            }
                        }
                        if (p->hitReceived == 0) {
                            switch (o->attackKind) {
                            default:
                                ObjAttach_TryHitPlayer(o, owner, (ObjHead *)p);
                                break;
                            case 6:
                            case 9:
                            case 0x13:
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

s32 Obj_InCurrentArea(void *obj); /* unit's own declaration: u8, as at its marked declaration below (s32 changes this unit's callers) */
void ObjAttach_HitPlayer(ObjAttach *o, Player *p, ObjHead *e);
void ObjAttach_TryHitPlayer(ObjAttach *o, Player *p, ObjHead *e)
{
    if (e->active != 0 && (o->unk008 == 0xFF || p->hitStop == 0)) {
        if (o->delay == 0 && (u8)Obj_InCurrentArea(e) != 0) {
            switch (o->hitPhase) {
            case 2:
                ObjAttach_HitPlayer(o, p, e);
                break;
            }
        }
    }
}

s32 ObjAttach_CanHit_Unk(Em *t)
{
    if (Debug_IsOn(0x2A) != 0) {
        return 0;
    }
    if (t->isEm != 0 && t->remote != 0) {
        return 1;
    }
    return 0;
}

static u8 ObjAttach_HitFlags(Em *p, HitShape *e)
{
    u8 flags = e->openFlags;
    u8 r = 0;

    if ((flags & 3) && p->windTimer > 0) {
        r |= 1;
    }
    if ((flags & 0xC) && (s8)p->hermitTimer > 0) {
        r |= 2;
    }
    return r;
}

s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 ObjAttach_CanHit_Unk(Em *t);
static u8 ObjAttach_HitFlags(Em *p, HitShape *e);
void ObjAttach_LinkTarget(ObjAttach *o, Player *p);
void ObjAttach_StepHitTimer_Unk(ObjAttach *o);
s32 Player_TestFlags(void *obj, s32 mask);
s32 Side_Match(u8 a, u8 b, u8 c);
s32 Player_HasMealEffect(Player *pl, s16 kind);
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
extern u8 **D_90CBD0[];
extern s8 gSeVolumeByType[];
extern f32 gHitKindMvFactors[];
extern f32 gSharpnessRawMods[];
extern f32 gSharpnessElemMods[];
extern f32 gSwordGaugeHitFactors[];
extern s16 gSharpnessLossByHit[];
extern s16 gHitSeIds[];
extern s16 D_0045EF12[];
extern s16 D_0045EF5C;

s32 printf(const char *fmt, ...);
/* A hit by object o (of player p) on monster t at out, hit shape elist of list: the reaction of p
 * (by the motion value against the hit zone), the sounds, the damage and the element. */
void ObjAttach_DamageEm(ObjAttach *o, Player *p, Em *t, HitShape *list, HitShape *elist, f32 *out)
{
    f32 mv;
    f32 atk;
    f32 eatk;
    f32 fire;
    f32 thunder;
    f32 ice;
    f32 water;
    f32 dragon;
    f32 cut;
    f32 imp;
    f32 pct;
    f32 dmgf;
    s16 dmg;
    s16 se;
    u8 kind;
    u8 weak;
    u8 hard;
    u8 nostun;
    u8 *hz;
    u8 bow;
    u8 id;
    u8 crit;
    u8 old;
    u8 val;

    bow = 0;
    nostun = 0;
    weak = 0;
    o->hitKind = 0;
    mv = 1.0f;
    eatk = 1.0f;
    atk = 1.0f;
    fire = 0.0f;
    crit = 0;
    hz = D_90CBD0[t->type][(s8)t->hitzoneSet];
    hz += elist->zone * 10;
    thunder = 0.0f;
    water = 0.0f;
    ice = 0.0f;
    dragon = 0.0f;
    id = o->hitId;
    if (!(o->multiHit & 0xF)) {
        if (Player_TestFlags(o, 0x10) != 0 && (ObjAttach_HitFlags(t, elist) & 1)) {
            switch (t->type) {
            case 0x40:
            case 0x41:
                o->state = 9;
                break;
            default:
                o->state = 5;
                break;
            }
            nostun = 1;
        } else if (Player_TestFlags(o, 1) != 0 && (ObjAttach_HitFlags(t, elist) & 2)) {
            o->state = 5;
            nostun = 1;
        } else {
            o->state = 3;
        }
        o->step = 0;
        o->unk01E = 1;
    } else {
        ObjAttach_StepHitTimer_Unk(o);
    }
    o->targetEm = t;
    o->targetPart = elist;
    if (o->unk008 != 0xFF && p != NULL) {
        ObjAttach_LinkTarget(o, p);
        p->hitPending = elist->zone + 1;
        if (Player_TestFlags(o, 0x20) != 0) {
            p->hitStopAdd = o->hitStop;
            p->contact = t;
            p->contactAttach = o;
        }
        if (Player_TestFlags(o, 0x800) != 0) {
            p->hitStopAdd = o->hitStop;
            p->contact = t;
            p->contactAttach = o;
            p->attackHit800 = 1;
        }
        if (ObjAttach_HitFlags(t, elist) & 2) {
            p->hitPending |= 0x80;
            weak = 1;
            Em_PlaySeChecked(t, 0x2D, 0, (f32 *)&out, gSeVolumeByType[t->type], 0);
        }
        if (p->isEm != 0) {
            Sound_PlayPlayerSe(t, 0x60, 0, out, 1, 0);
        } else {
            bow = 1;
            if (o->elementFlags & 0x10) {
                fire = o->elementPower;
            }
            if (o->elementFlags & 0x20) {
                water = o->elementPower;
            }
            if (o->elementFlags & 0x40) {
                thunder = o->elementPower;
            }
            if (o->elementFlags & 0x80) {
                ice = o->elementPower;
            }
            if (o->elementFlags & 0x200) {
                dragon = o->elementPower;
            }
            if (Player_TestFlags(o, 0x40) != 0) {
                if (o->hitMarks & 1) {
                    crit = 1;
                }
                if (o->unk075[0] == 0xFF) {
                    kind = 0;
                } else if (o->unk075[0] != 0) {
                    kind = 1;
                } else if (o->unk075[1] != 0) {
                    if (list->zone != 1) {
                        kind = 2;
                    } else {
                        kind = 3;
                    }
                } else {
                    kind = 0;
                }
                mv = gHitKindMvFactors[kind + p->sharpnessLevel * 4];
                atk = p->attackFactor;
                if (p->weaponType == 0xA) {
                    eatk = 1.0f;
                } else {
                    mv *= gSharpnessRawMods[p->sharpnessLevel];
                    eatk = gSharpnessElemMods[p->sharpnessLevel];
                }
                cut = p->attackPercent / 100.0f;
                imp = p->attackPercentBase / 100.0f;
                cut *= hz[1] / 100.0f;
                imp *= hz[2] / 100.0f;
                if (!(cut < imp)) {
                    mv *= cut;
                } else {
                    mv *= imp;
                }
                hard = weak;
                if (hard != 0 && !(mv <= 0.1f)) {
                    mv = 0.1f;
                }
                if (!(mv < 1.0f)) {
                    p->hitKind = 3;
                    p->hitStopAdd = o->hitStop * 3;
                } else if (!(mv < 0.45f)) {
                    p->hitKind = 2;
                    p->hitStopAdd = 0;
                    p->hitSlowTimer = o->hitStop;
                } else {
                    if (!(mv < 0.25f)) {
                        p->hitKind = 0;
                    } else {
                        p->hitKind = 1;
                    }
                    p->hitStopAdd = 0;
                }
                val = 0;
                o->hitKind = p->hitKind;
                if (Player_HasMealEffect(p, 0) == 1 && p->mealEffect[0] >= 2) {
                    val = 1;
                }
                if (Player_HasSkill(p, 0x24) != 0 || Player_HasSkill(p, 0x25) != 0) {
                    val = 1;
                }
                if (val != 0 && p->hitKind == 1) {
                    p->hitKind = 0;
                }
                if (p != NULL) {
                    if (o->swordGaugeAdd > 0) {
                        LongSword_AddGauge(p, o->swordGaugeAdd * gSwordGaugeHitFactors[p->hitKind]);
                    }
                }
                if (p->weaponType != 0xA) {
                    if (hard != 0) {
                        Player_AddSharpness(p, gSharpnessLossByHit[p->hitKind] * 5);
                    } else {
                        Player_AddSharpness(p, gSharpnessLossByHit[p->hitKind]);
                    }
                }
                switch (o->hitSe) {
                case 0xFF:
                    break;
                case 0:
                default:
                    Sound_PlayPlayerSe(t, gHitSeIds[p->weaponType * 8 + p->hitKind * 2], 0, out, 1, 0);
                    se = D_0045EF12[p->weaponType * 8 + p->hitKind * 2];
                    if (se != 0) {
                        Sound_PlayPlayerSe(t, se, 0, out, 1, 0);
                    }
                    break;
                case 0xA:
                    Sound_PlayPlayerSe(t, 0x82, 0, out, 1, 0);
                    break;
                case 1:
                    Sound_PlayPlayerSe(t, 0x20, 0, out, 1, 0);
                    break;
                }
            } else {
                mv = 1.0f;
                if (Player_TestFlags(o, 8) != 0) {
                    mv *= hz[3] / 100.0f;
                    if (Debug_IsOn(0xAE) != 0) {
                        if (o->hitFx & 0x80) {
                            printf(TXT(0x004AB080) /* debug message: a ranged attack hit (critical) */);
                        } else if (o->hitFx & 0x40) {
                            printf(TXT(0x004AB0B0) /* debug message: a ranged attack hit (critical failed) */);
                        } else {
                            printf(TXT(0x004AB0F0) /* debug message: a ranged attack hit */);
                        }
                        printf(TXT(0x004AB110) /* debug message: the player's attack modifier */, fptodp(1.0f));
                        printf(TXT(0x004AB130) /* debug message: the hit zone's result against a shot */, fptodp(mv));
                    }
                } else if (Debug_IsOn(0xAE) != 0) {
                    printf(TXT(0x004AB150) /* debug message: another player attack hit */);
                }
                switch (o->hitSe) {
                case 0xFF:
                    break;
                case 2:
                case 3:
                    Sound_PlayPlayerSe(t, 0x5D, 0, out, 1, 0);
                    break;
                case 4:
                    Sound_PlayPlayerSe(t, 0x5C, 0, out, 1, 0);
                    break;
                case 5:
                    break;
                case 6:
                    Sound_PlayPlayerSe(t, 0x4D, 0, out, 1, 0);
                    break;
                case 1:
                    Sound_PlayPlayerSe(t, 0x20, 0, out, 1, 0);
                    break;
                case 7:
                    Sound_PlayPlayerSe(t, D_0045EF5C, 0, out, 1, 0);
                    break;
                case 8:
                    Sound_PlayPlayerSe(t, 0x80, 0, out, 1, 0);
                    break;
                case 9:
                    Sound_PlayPlayerSe(t, 0x81, 0, out, 1, 0);
                    break;
                case 0xA:
                    Sound_PlayPlayerSe(t, 0x82, 0, out, 1, 0);
                    break;
                case 0xB:
                    Sound_PlayPlayerSe(t, 0x83, 0, out, 1, 0);
                    break;
                case 0xC:
                    Sound_PlayPlayerSe(t, 0x84, 0, out, 1, 0);
                    break;
                }
            }
            fire *= hz[4] / 100.0f;
            water *= hz[5] / 100.0f;
            thunder *= hz[7] / 100.0f;
            ice *= hz[8] / 100.0f;
            dragon *= hz[6] / 100.0f;
            fire = eatk * (fire + water + thunder + ice + dragon);
            Vib_StartLocal(p, 0);
        }
    }
    if (o->attackKind == 7 && t->painted != 0xFF && (u8)ObjAttach_CanHit_Unk(t) == 0 && nostun == 0) {
        t->painted = 1;
        t->paintTimer = 0x4650;
    }
    t->hitIds[t->hitIdNext] = id;
    t->hitIdNext = (t->hitIdNext + 1) & 0xF;
    if ((u8)ObjAttach_CanHit_Unk(t) == 0 && Side_Match(o->side, ((ObjHead *)t)->side, 1) != 1) { /* the unit read the side as the shared u8 (Em.side is s8) */
        dmgf = o->power;
        if (o->powerPct > 0) {
            pct = o->powerPct;
            pct /= 100.0f;
            dmgf *= pct;
        }
        switch (t->hitReactMode) {
        case 2:
            if ((s8)t->guardTimer <= 0 && bow == 0) {
                dmgf *= 3.0f;
            }
            break;
        case 3:
            dmgf *= 1.1f;
            break;
        case 4:
            dmgf *= 1.1f;
            break;
        case 0:
        case 1:
            break;
        }
        if (o->attackKind == 8) {
            dmg = -1.0f * o->power;
        } else {
            dmg = atk * (dmgf * mv);
        }
        if (o->isEm != 0 && t->type != 0x1D) {
            dmg /= 4;
        }
        if (dmg == 0 && o->power != 0) {
            dmg = 1;
        }
        dmg += (s16)fire;
        if (nostun == 0) {
            if (crit != 0) {
                t->partFlags[elist->part & 7] |= 1;
            }
            if (o->hitMarks & 2) {
                t->partFlags[elist->part & 7] |= 2;
            }
            if (o->hitMarks & 8) {
                t->partFlags[elist->part & 7] |= 8;
            }
            if (o->elementFlags & 0x10) {
                t->partFlags[elist->part & 7] |= 4;
            }
            if (o->elementFlags & 0x20) {
                t->partFlags[elist->part & 7] |= 0x10;
            }
            if (o->elementFlags & 0x80) {
                t->partFlags[elist->part & 7] |= 0x20;
            }
            if (o->elementFlags & 0x40) {
                t->partFlags[elist->part & 7] |= 0x40;
            }
            if (o->elementFlags & 0x200) {
                t->partFlags[elist->part & 7] |= 0x80;
            }
            t->hitPartDamage[elist->part & 7] += dmg;
            Em_ApplyHitStatus((Player *)t, o);
            if (o->isEm == 0) {
                t->status5Hit += (s16)(o->stun * (hz[9] / 100.0f));
            }
            if (o->unk008 != 0xFF && p->isEm == 0) {
                t->reactValue[p->index & 3] += dmg;
            }
            t->hitStopAdd = o->hitStop;
            t->hitReceived = 2;
            t->flinchPart = elist->part;
            if (Player_TestFlags(o, 0x20) == 0) {
                Player_SetFlag(t, 0x80000001);
            } else {
                Player_ClearFlag(t, 0x80000001);
            }
            t->hitAttach = o;
            t->lastAttackKind = o->attackKind;
            t->hitForce = o->hitForce;
            old = t->hitAttrs;
            t->hitAttrs = old | o->attrs;
            if (o->attackKind == 0x10) {
                Em_EndRage(t);
            }
        }
    }
}

/* The objects of .rodata from 0x4AB180 to 0x4AE590 that no code loads
 * (data may point to them). */
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB180);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB1F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB218);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB228);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB238);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB258);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB268);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB278);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB2B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB2C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB408);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB418);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB4A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB550);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB560);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB580);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB5A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB5B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB5B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB5C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB5D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB5E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB5F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB608);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB618);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB620);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB630);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB640);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB678);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB680);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB690);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB698);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB6A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB6B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB6C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB6D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB6E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB6F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB708);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB718);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB728);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB738);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB748);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB758);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB768);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB788);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB798);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB7E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB7F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB808);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB818);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB828);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB838);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB850);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB860);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB870);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB880);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB890);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB8A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB8B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB8C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB8D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB8E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB8F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB900);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB910);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB920);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB930);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB940);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB950);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB958);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB968);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB978);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB988);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AB9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABA20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABA50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABA60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABA70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABAD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABAF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABB90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABBA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABBA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABBB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABBB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABBC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABBD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABBE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABBF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABC90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABCA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABCB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABCC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABCD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABCF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABD98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABDA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABDB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABDC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABDE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABE98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABEA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABEB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABEF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABF00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABF10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABF28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABF40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABF58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABF68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABF78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABF90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABFD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ABFF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC000);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC008);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC010);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC038);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC048);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC068);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC070);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC078);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC080);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC088);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC098);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC0A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC0B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC0F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC108);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC118);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC128);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC138);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC150);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC188);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC208);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC288);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC298);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC2A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC2E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC2F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC308);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC348);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC358);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC3A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC3B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC3C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC3E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC440);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC450);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC4B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC4C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC4D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC4E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC4F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC500);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC520);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC588);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC5B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC5C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC5D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC5E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC600);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC620);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC638);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC680);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC6A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC6B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC6C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC6D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC6E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC6F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC708);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC718);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC728);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC738);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC748);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC758);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC768);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC788);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC798);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC7E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC7F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC808);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC818);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC828);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC838);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC878);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC888);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC898);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC8A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC8B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC8C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC8D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC8E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC8F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC908);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC928);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC938);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC948);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC958);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC968);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC978);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC988);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AC9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACA98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACAA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACAB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACAC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACAD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACAF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACB98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACBA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACBB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACBC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACBD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACBE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACC98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACCA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACCB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACCC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACCE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACD00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACD30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACD50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACD90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACDC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACDD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACDE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACDF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACE08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACE20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACE38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACE48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACE58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACE68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACE78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACE88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACEA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACEB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACEF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACF98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACFC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACFD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ACFF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD008);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD038);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD048);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD068);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD078);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD088);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD098);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD0A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD0B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD0E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD0F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD108);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD118);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD128);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD138);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD148);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD158);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD180);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD2F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD330);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD370);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD390);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD430);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD468);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD478);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD488);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD508);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD518);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD538);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD548);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD568);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD578);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD588);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD5A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD5B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD5C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD5E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD5F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD608);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD618);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD628);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD638);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD678);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD688);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD698);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD6A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD6B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD6C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD6D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD6E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD700);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD720);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD740);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD760);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD790);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD7F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD810);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD830);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD880);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD8A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD8C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD8D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD8F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD908);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD930);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD948);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD960);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD980);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD9A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD9D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AD9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADAC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADAD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADAE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADAF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADB90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADBA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADBB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADBC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADBD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADBE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADBF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADC98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADCA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADCB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADCC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADCE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADCF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADD98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADDD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADDE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADE00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADE18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADE28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADE38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADE48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADE58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADE70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADE90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADEB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADEF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADF98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADFD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004ADFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE000);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE040);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE070);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE090);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE0B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE0F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE110);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE130);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE150);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE188);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE198);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE1F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE208);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE218);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE228);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE238);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE248);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE258);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE268);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE278);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE288);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE298);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE2F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE308);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE330);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE348);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE358);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE370);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE390);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE3F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE408);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE430);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE440);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE450);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE468);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE478);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE488);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE498);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE500);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE508);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE518);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE520);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE538);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE548);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE550);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE560);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE568);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE578);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE580);
INCLUDE_RODATA("asm/sub_main/nonmatchings/unk_3BE060", D_004AE588);

void ObjAttach_LinkTarget(ObjAttach *o, Player *p);
void ObjAttach_StepHitTimer_Unk(ObjAttach *o);
s32 Player_IsLocal(void *obj);
s32 Player_CanGuard(Player *p);
s32 HeavyBowgun_HasShield(Player *p);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Player_InGuardArc(u16 a, u16 b);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
/* A hit by object o (owned by player p) on player t at pos: the reaction,
 * the damage (attack factor, defence, element resistance; halved in versus
 * quests) and the hit sound. */
void ObjAttach_DamagePlayer(ObjAttach *o, Player *p, Player *t, f32 *pos)
{
    f32 atk;
    f32 dmgf;
    f32 def;
    f32 res;
    u8 stun;
    s16 dmg;
    u8 guard;
    u8 id;
    u8 elem;

    id = o->hitId;
    if (!(o->multiHit & 0xF)) {
        o->state = 3;
        o->step = 0;
        o->unk01E = 1;
    } else {
        ObjAttach_StepHitTimer_Unk(o);
    }
    o->targetEm = (Em *) t;
    o->targetPart = 0;
    o->hitKind = 0;
    atk = 1.0f;
    if (o->unk008 != 0xFF) {
        ObjAttach_LinkTarget(o, p);
        p->hitPending = 1;
        if (Player_TestFlags(o, 0x20) != 0) {
            p->hitStopAdd = o->hitStop;
        }
        if (Player_TestFlags(o, 0x800) != 0) {
            p->attackHit800 = 1;
        }
        p->hitKind = 0;
        p->contact = t;
        p->contactAttach = o;
        atk = p->attackFactor;
    }
    t->hitIds[t->hitIdNext] = id;
    t->hitIdNext = (t->hitIdNext + 1) & 0xF;
    t->partFlags[0] |= o->hitMarks;
    if ((u8)ObjAttach_CanHit_Unk((Em *)t) != 0) {
        return;
    }
    if (Player_IsLocal(t) == 1) {
        switch (o->attackKind) {
        case 0xB:
            t->attackDrug = 3;
            break;
        case 0xC:
            t->defenseDrug = 6;
            break;
        case 0xD:
            t->poison = 0;
            break;
        case 0x18:
            t->status6Timer = 0;
            break;
        }
    }
    if (Side_Match(t->side, o->side, 1) != 1 || o->attackKind != 8) {
        if ((t->side == 0 && o->isEm != 0) || o->unk008 == 0xFF || o->attackKind == 8
            || Side_Match(t->side, o->side, 0) == 0 || (o->side == 0 && o->isEm != 0)) {
            Em_ApplyHitStatus(t, o);
            if (o->attackKind == 8) {
                t->hitDamage[0] -= o->power;
            } else {
                dmgf = (f32)o->power * atk;
                if (o->powerPct > 0) {
                    res = o->powerPct;
                    res /= 100.0f;
                    dmgf *= res;
                }
                def = t->defenseFactor;
                res = 80.0f;
                dmgf = dmgf - dmgf * def / (def + res);
                res = 1.0f;
                elem = o->hitMarks;
                if (elem & 4) {
                    res = (100.0f - t->resist[0]) / 100.0f;
                } else if (elem & 0x10) {
                    res = (100.0f - t->resist[1]) / 100.0f;
                } else if (elem & 0x40) {
                    res = (100.0f - t->resist[2]) / 100.0f;
                } else if (elem & 0x20) {
                    res = (100.0f - t->resist[3]) / 100.0f;
                } else if (elem & 0x80) {
                    res = (100.0f - t->resist[4]) / 100.0f;
                }
                stun = o->stun;
                dmg = dmgf * res;
                if (Quest_IsVersus() == 1 && t->isEm == 0 && o->isEm == 0) {
                    if (dmg > 0) {
                        dmg /= 2;
                        if (dmg <= 0) {
                            dmg = 1;
                        }
                    }
                    if (stun > 0) {
                        stun = stun / 2;
                        if (stun <= 0) {
                            stun = 1;
                        }
                    }
                }
                t->hitDamage[0] += dmg;
                t->status5Hit += stun;
            }
        }
    }
    t->target = p;
    t->hitPos[0] = pos[0];
    t->hitPos[1] = pos[1];
    t->hitPos[2] = pos[2];
    t->hitReceived = 2;
    if (Player_CanGuard(t) == 0) {
        guard = 1;
    } else if ((Player_HasSkill(t, 0x2C) == 1 || HeavyBowgun_HasShield(t) == 1) && t->weaponOut != 0 && t->prevFreeState != 0) {
        guard = 0;
    } else if (Player_TestFlag(t, 0x8000) != 0) {
        guard = 0;
    } else {
        guard = 1;
    }
    if (Player_TestFlag(t, 0x8000) == 0 || Player_InGuardArc(*(u16 *)&t->rot[1], t->hitAngle) == 0
        || t->stamina < 0x4B || guard == 1) {
        switch (o->hitSe) {
        case 0xFF:
            break;
        case 0:
        default:
            Sound_PlayPlayerSe(t, 0x21, 0, pos, 1, 0);
            break;
        case 1:
            Sound_PlayPlayerSe(t, 0x20, 0, pos, 1, 0);
            break;
        }
    }
    t->flinchPart = 0;
    t->hitAttach = o;
    t->lastAttackKind = o->attackKind;
    *(s16 *)&t->hitForce = o->hitForce;
    t->hitAttrs |= o->attrs;
}

u16 Vec_Heading(f32 *a, f32 *b);
void ObjAttach_SetHitHeading(ObjAttach *o, Player *p, f32 *a, f32 *b)
{
    if (o->attrs & 0x10) {
        p->hitAngle = Vec_Heading(a, b) + 0x4000;
    } else {
        p->hitAngle = *(u16 *)&o->yaw + (u16)(s32)(0.5f + 65536.0f * o->hitYawOfs / 360.0f);
    }
}

void Player_BodyCapsule(Player *p, HitCapsule *cap, f32 r);
void HitList_Next(Em *p, HitShape **cur);
void ObjAttach_SetHitHeading(ObjAttach *o, Player *p, f32 *a, f32 *b);
void ObjAttach_DamagePlayer(ObjAttach *o, Player *p, Player *t, f32 *out);
void ObjAttach_HitEffects(ObjAttach *o, Player *p, ObjHead *e, f32 *out);
void ObjAttach_DamageEm(ObjAttach *o, Player *p, Em *t, HitShape *list, HitShape *elist, f32 *out);
extern HitShape *gEmBoneHits[];
/* Tests an object's hit shapes against player e: its body capsule when
 * e->unk010 is 0, else each of the hit shapes for its weapon type
 * (gEmBoneHits), some of them only open to some object kinds (+0x63). The
 * first hit goes to ObjAttach_DamagePlayer or ObjAttach_DamageEm. */
void ObjAttach_HitPlayer(ObjAttach *o, Player *p, ObjHead *e)
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
    s16 a;
    s16 b;
    s16 hit;
    u8 f8;
    u8 f9;
    u8 kind;
    Em *em;

    if (e->noHitTimer != 0) {
        return;
    }
    if (o->hitList == 0) {
        return;
    }
    list = o->hitList;
    if (e->isEm == 0) {
        if (e->stateGroup == 3) {
            if (o->attackKind != 0xA) {
                return;
            }
        } else if (o->attackKind == 0xA) {
            return;
        }
        Player_BodyCapsule((Player *)e, &segB, 40.0f);
        Capsule_Copy(&segB, &capB);
    }
    while (list->bone != -1) {
        switch (list->bone) {
        case 0x7F:
            a = HitShape_Place(o->pos, list, &segA, &sphA, o->hitScale);
            break;
        case 0x7E:
            a = Capsule_FromPoints(o->pos, o->pos2, list, &segA, o->hitScale);
            break;
        case 0x7D:
            HitList_NextForObj(o, &list);
            continue;
        default:
            a = BoneHit_Place((ObjHead *)p, list, &segA, &sphA);
            break;
        }
        if (a != 0) {
            Capsule_Copy(&segA, &capA);
        }
        if (e->isEm == 0) {
            hit = (a == 1) ? Capsule_ContactCapsule(&capA, &capB, out) : (s32)(u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
            if (hit != 0) {
                if (a == 1) {
                    ObjAttach_SetHitHeading(o, (Player *)e, capB.mid, capA.mid);
                } else {
                    ObjAttach_SetHitHeading(o, (Player *)e, capB.mid, sphA.pos);
                }
                ObjAttach_DamagePlayer(o, p, (Player *)e, out);
                ObjAttach_HitEffects(o, p, e, out);
                return;
            }
        } else {
            em = (Em *)e;
            elist = gEmBoneHits[e->weaponType];
            while (elist->bone != -1) {
                switch (elist->bone) {
                case 0x7F:
                case 0x7E:
                    b = HitShape_Place(e->pos, elist, &segB, &sphB, 1.0f);
                    break;
                case 0x7D:
                    HitList_Next((Em *)e, &elist);
                    continue;
                default:
                    b = BoneHit_Place(e, elist, &segB, &sphB);
                    break;
                }
                if ((elist->openFlags & 2) && (em->windTimer == 0 || Player_TestFlags(o, 0x10) == 0)) {
                    goto next;
                }
                if ((elist->openFlags & 8) && (s8)em->hermitTimer == 0) {
                    goto next;
                }
                if (elist->offFlags & 1) {
                    goto next;
                }
                f8 = elist->atkFlags;
                if (f8 & 4) {
                    if (em->remote != 0 || (o->attackKind != 6 && o->attackKind != 9)) {
                        goto next;
                    }
                }
                f9 = elist->atkFlags2;
                if (f9 & 2) {
                    if (em->remote != 0 || o->attackKind != 0x13) {
                        goto next;
                    }
                }
                kind = o->attackKind;
                if (kind == 6 || kind == 9) {
                    if (!(f8 & 1) || em->remote != 0) {
                        goto next;
                    }
                }
                if (kind == 0x13) {
                    if (!(f9 & 1) || em->remote != 0) {
                        goto next;
                    }
                }
                if (kind == 5 || kind == 0xE) {
                    if (!(f8 & 2)) {
                        goto next;
                    }
                }
                if (b != 0) {
                    Capsule_Copy(&segB, &capB);
                }
                if (a == 1) {
                    if (b == 1) {
                        hit = Capsule_ContactCapsule(&capA, &capB, out);
                    } else {
                        hit = (u8)Capsule_SphereContactOnSphere(&capA, sphB.pos, out, sphB.radius);
                    }
                } else if (b == 1) {
                    hit = (u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
                } else {
                    hit = (u8)Spheres_Touch(sphA.radius, sphB.radius, sphA.pos, sphB.pos, out);
                }
                if (hit != 0) {
                    if (a == 1) {
                        if (b == 1) {
                            ObjAttach_SetHitHeading(o, (Player *)e, capB.mid, capA.mid);
                        } else {
                            ObjAttach_SetHitHeading(o, (Player *)e, sphB.pos, capA.mid);
                        }
                    } else if (b == 1) {
                        ObjAttach_SetHitHeading(o, (Player *)e, capB.mid, sphA.pos);
                    } else {
                        ObjAttach_SetHitHeading(o, (Player *)e, sphB.pos, sphA.pos);
                    }
                    ObjAttach_DamageEm(o, p, (Em *)e, list, elist, out);
                    ObjAttach_HitEffects(o, p, e, out);
                    return;
                }
            next:
                elist++;
            }
        }
        list++;
    }
}

void ObjAttach_LinkTarget(ObjAttach *o, Player *p)
{
    p->hitTimers = &o->hitDelay;
}

u8 HitId_Next(void);

void ObjAttach_StepHitTimer_Unk(ObjAttach *o)
{
    u8 v = o->multiHit;

    o->multiHit = (v & 0xF0) | (s8)((s8)(v & 0xF) - 1);
    if (o->multiHit >> 4) {
        o->hitDelay = o->multiHit >> 4;
        o->hitPhase = 1;
        o->hitId = HitId_Next();
    }
}

struct MoveObj;
void Obj_StartMotionOwned(struct MoveObj *o, void *owner, s32 i);
/* Counts down the object's +0x60/+0x61 timers by its unk00B state (1: waiting, 2: running; Obj_StartMotionOwned restarts it). */
void ObjAttach_UpdateTimers(ObjAttach *o)
{
    u8 *b = (u8 *)o;

    switch (o->hitPhase) {
    case 0:
        break;
    case 1:
        if (o->hitDelay-- != 0) {
            break;
        }
        o->hitDelay = 0;
        o->hitPhase++;
    case 2:
        if (o->hitFrames == 0) {
            if (b[0x73] != 0) {
                Obj_StartMotionOwned((struct MoveObj *)o, &gPlayers[o->idx], b[0x73]);
                o->hitPhase = 1;
                if (o->hitDelay == 0) {
                    o->hitPhase = 2;
                    o->hitFrames--;
                } else {
                    o->hitPhase = 1;
                    break;
                }
            } else {
                o->hitPhase = 0;
                o->hitDelay = 0;
                o->hitFrames = 0;
                break;
            }
        } else {
            o->hitFrames--;
        }
        if (b[0x75] != 0 && b[0x75] != 0xFF) {
            b[0x75]--;
            break;
        }
        if (b[0x76] != 0) {
            b[0x76]--;
        }
        break;
    }
}

void ObjAttach_UpdateAllTimers(void)
{
    ObjAttach *o;

    o = gObjAttachList;
    if (o != NULL) {
        do {
            if (o->active != 0 && o->delay == 0) {
                ObjAttach_UpdateTimers(o);
            }
            o = o->next;
        } while (o != NULL);
    }
}

void HitId_Reset(void)
{
    gScene.hitId = 0;
}

/* Steps the id counter SceneState.hitId (gScene.hitId is the same byte), skipping 0; returns it. */
u8 HitId_Next(void)
{
    SceneState *s = (SceneState *)&gScene;

    gScene.hitId++;
    if (gScene.hitId == 0) {
        s->hitId = 1;
    }
    return s->hitId;
}

/* A part of a monster's model (0x8C bytes; Model +0x30, +0x2C of them). */
typedef struct EmModelPart {
    s32 model;                  /* 0x00: -1: none */
    s32 nlight;                 /* 0x04 */
    s32 light[32];              /* 0x08: indices into the model's lights (+0x10, 0x4C bytes each) */
    s32 unk88;                  /* 0x88 */
} EmModelPart;

void LightSet_SetAmbient(void *obj, s32 n);
void LightSet_FadeAmbient(void *obj);
void Vec_AnglesToRad(s32 *in, f32 *out);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Gfx_SetState63(s32 a);
struct EmBone;
void Bone_StoreMatrices(struct EmBone *b);
void Mtx_Identity(f32 *m);
void Tex_Prepare(s32 a, s32 b);
void Draw_ApplyStates(u32 f);
void Draw_SetDefaultState(void);
s32 Model_Draw(u32 h, s32 flags);
/* Draws a monster (the slot's owner): its world matrix, then its model's parts with their lights (+0x6E4; off for the unflagged parts of kind 1). */
void Em_Draw(DrawSlot *slot)
{
    f32 rad[4];
    f32 m[4][4];
    f32 ident[4][4];
    f32 scale[4][4];
    s32 i;
    Em *em;
    s32 n;
    s32 j;
    EmModelPart *part;
    u8 *lights;
    Model *model;

    em = slot->owner;
    model = em->model;
    if (em->active == 0 || em->visible == 0) {
        return;
    }
    LightSet_SetAmbient(em, 1);
    LightSet_FadeAmbient(em);
    Vec_AnglesToRad(&em->rotX, rad);
    Mtx_Scale(scale[0], em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, m[0]);
    Mtx_SetTrans(m[0], em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(m[0], scale[0]);
    Mtx_Copy((f32 *)em->worldMtx, m[0]);
    Gfx_SetState63(1);
    Gs_SetState(0x60, 0);
    part = *(EmModelPart **)((u8 *)model + 0x30);
    n = *(s16 *)((u8 *)model + 0x2C);
    lights = *(u8 **)((u8 *)model + 0x10);
    Bone_StoreMatrices(model->bones);
    Mtx_Identity(ident[0]);
    Gs_SetState(0x19, (s32)ident);
    Tex_Prepare(10, em->type * 20 + 0xA3);
    for (i = 0; i < n; i++, part++) {
        if (em->type != 0 || *(s8 *)((u32)em + i + 0x4C8) != 0) {
            Gs_SetState(0x67, -1);
            if (part->model != -1) {
                for (j = 0; j < part->nlight; j++) {
                    u8 *l = lights + part->light[j] * 0x4C;

                    switch (em->type) {
                    case 1:
                        if (*(s8 *)((u32)em + j + 0x4C8) == 0) {
                            *(s32 *)(l + 0x10) = 0;
                        } else {
                            *(f32 *)(l + 0x10) = em->alpha;
                        }
                        break;
                    default:
                        *(f32 *)(l + 0x10) = em->alpha;
                        break;
                    }
                    Gs_SetState(j + 0x3A, (s32)l);
                }
                Draw_ApplyStates(part->unk88);
                Model_Draw(part->model, 0);
            }
        }
    }
    Draw_SetDefaultState();
    Gs_SetState(0x60, 0);
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
            *(Em **)((u8 *)em->drawSlot + 0x18) = em;
            ((DrawSlot *)em->drawSlot)->draw = Em_Draw;
        }
    }
}

void EmSlot_SetupModel(Em *em);
void Em_InitCollMask(Em *em);
void Em_InitHandlers(Em *em);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_SetMotion(Player *p, s32 motion, s32 a, s16 b, s32 layer);
void Player_BeginLayerMotion(Player *p, u16 motion, s16 a, s32 layer); /* unit's own declaration: the definition's (Player *p, s32 frame, s32 n, s32 part) (sub_main/unk_2E3940.c) matches this function's calls, but the file's second declaration (Player_PrepareMotions, below) needs u16 and s16, and the two must agree */
void Player_UpdateAreaPos(void *obj);
extern f32 gStartPos[][3];
extern s16 gEmVariantShownMeshes[][2];
extern u16 *gEmVariantMeshes[];
/* Sets up this monster kind (state 0 of Em_RunByState): its model, scale, the area's start position (kind 1: moved by 200 * +0x1B), the parts it shows (+0x4C8, from the tables by +0x1B), its draw slot and first motions. */
void Em_SetupKind(Em *em)
{
    u16 *list;
    u16 v;
    s32 i;

    EmSlot_SetupModel(em);
    em->taskPhase++;
    em->isEm = 0;
    ((u8 *)em)[0x1E] = 1;
    em->unk414 = 0;
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
    for (i = 0; i < 32; i++) {
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
        v = *list;
        while (v != 0xFFFF) {
            em->meshShown[v] = 1;
            list++;
            v = *list;
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
extern u16 D_006AE0B4;
/* A monster's state update (states 0 and 1 of group 0), then its matrix, motion speed and floor. */
void Em_UpdateState(Em *em)
{
    s32 type;
    u16 i;
    u16 *list;
    u16 k;

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
                if (D_006AE0B4 & 0x200) {
                    em->variant++;
                    em->variant = em->variant % 6;
                    for (i = 0; i < 32; i++) {
                        em->meshShown[i] = 0;
                    }
                    list = gEmVariantMeshes[em->variant];
                    while ((k = *list) != 0xFFFF) {
                        list++;
                        em->meshShown[k] = 1;
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
    *(f32 *)((u8 *)em + 0x250) = 2.0f * em->motionSpeed;
    Player_PrepareMotions((Player *)em);
    Em_CollideField(em, 0);
    Em_GetFloorInfoEx(em, &em->posX, (struct TriHdr *)&em->floorKind, &em->floorY, &em->surfaceY);
    em->posY = em->floorY;
}

void Em_IncUnk04(void *em)
{
    *(u8 *)((u8 *)em + 4) = (u8) (*(u8 *)((u8 *)em + 4) + 1);
}

void Em_StateFree(u8 *e)
{
    EmSlot_Free((Em *) e);
}

void Em_SetupKind(Em *em);
void Em_UpdateState(Em *em);
void Player_UpdateAreaPos(void *obj);
void Em_RunHook3(Em *em);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
/* The update of this monster kind by state (0 set up, 1 run, 2 end, 3 free); 1 once freed. */
s32 Em_RunByState(Em *em)
{
    switch (em->taskPhase) {
    case 0:
        Em_SetupKind(em);
        Em_RunHook3(em);
        Player_UpdateAreaPos(em);
        return 0;
    case 1:
        Em_UpdateState(em);
        break;
    case 2:
        Em_IncUnk04(em);
        break;
    case 3:
        Em_StateFree((u8 *)em);
        return 1;
    }
    Em_RunHook3(em);
    if (em->visible != 0 && em->area == gScene.area) {
        *(f32 *)((u8 *)em->drawSlot + 8) = em->posX;
        *(f32 *)((u8 *)em->drawSlot + 0xC) = em->posY;
        *(f32 *)((u8 *)em->drawSlot + 0x10) = em->posZ;
        OT_AddByDepth(gDrawOt32, em->drawSlot, 0x20, 0);
    }
    Player_UpdateAreaPos(em);
    return 0;
}

s32 Debug_IsOn(s32 unused0);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
/* unit's own declaration: with the definition's void (Player *p, s32 frame, s32 n, s32 part) (sub_main/unk_2E3940.c), Player_PrepareMotions: 2 of 44 words differ */
void Player_BeginLayerMotion(Player *p, u16 motion, s16 a, s32 layer);
s32 Player_UpdateMotion(Player *p);
/* Unless flag 0x28 is set: the player's rotation matrix, then (while not in hit stop) starts the motions pending on layers 0 and 1. */
void Player_PrepareMotions(Player *p)
{
    if (Debug_IsOn(0x28) == 0) {
        Mtx_FromAnglesXYZ(p->rot, p->rotMtx);
        if (p->hitStop == 0) {
            if (p->motionStarted[0] == 0) {
                p->motionStarted[0]++;
                Player_BeginLayerMotion(p, p->motionFrame[0], p->motionBlend[0], 0);
            }
            if (p->motionStarted[1] == 0 && p->motionLayerCount > 1) {
                p->motionStarted[1]++;
                Player_BeginLayerMotion(p, p->motionFrame[1], p->motionBlend[1], 1);
            }
            Player_UpdateMotion(p);
        }
    }
}

void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Mtx_Copy(f32 *dst, f32 *src);
void Bone_UpdateWorldScaledRoot(struct EmBone *bones, f32 *m);
/* The monster's world matrix (scale, rotation, position) into +0x60 and its skeleton. */
void Em_UpdateWorldMatrix(Em *em)
{
    Model *model = em->model;
    f32 m[4][4];
    f32 scale[4][4];

    Mtx_Scale(scale[0], em->scaleX, em->scaleY, em->scaleZ);
    Mtx_FromAnglesYXZ(&em->rotX, m[0]);
    Mtx_SetTrans(m[0], em->posX, em->posY, em->posZ);
    Mtx_Concat3x3(m[0], scale[0]);
    Mtx_Copy((f32 *)em->worldMtx, m[0]);
    Bone_UpdateWorldScaledRoot(model->bones, m[0]);
}

/* Calls the fourth handler of the monster's table. */
void Em_RunHook3(Em *em)
{
    em->handlers->unk0C(em);
}

void Em_NopHandlerA(void)
{
}

void Em_NopHandlerB(void)
{
}

/* The update of the kind 1 objects (Effect01_Spawn), by state: set up, run, end, free. */
void Effect01_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        Effect01_Setup(o);
        break;
    case 1:
        Effect01_Run(o);
        break;
    case 2:
        Effect01_End(o);
        break;
    case 3:
        Effect01_Delete(o);
        break;
    }
}

s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlotB_Get(s32 i);
void Effect01_Draw(DrawSlot *s);
/* Sets up an effect object: its draw slots (type 2: five, one per part), drawn by Effect01_Draw. */
void Effect01_Setup(StageObj *o)
{
    EffectParts *w;
    s16 i;

    o->state++;
    o->var0 = 0;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    case 2:
        w = (EffectParts *)o->buf;
        for (i = 0; i < 5; i++) {
            w->slot[i] = DrawSlot_Alloc();
            if (w->slot[i] != -1) {
                w->draw[i] = DrawSlot_Get(w->slot[i]);
                w->draw[i]->owner = o;
                w->draw[i]->part = i;
                w->draw[i]->draw = Effect01_Draw;
            } else {
                w->draw[i] = NULL;
            }
            *(DrawSlot **)&o->node = NULL;
        }
        break;
    default:
        if (o->keep) {
            o->slot = DrawSlotB_Alloc();
        } else {
            o->slot = DrawSlot_Alloc();
        }
        if (o->slot != -1) {
            if (o->keep) {
                *(DrawSlot **)&o->node = DrawSlotB_Get(o->slot);
            } else {
                *(DrawSlot **)&o->node = DrawSlot_Get(o->slot);
            }
            (*(DrawSlot **)&o->node)->owner = o;
            (*(DrawSlot **)&o->node)->draw = Effect01_Draw;
            break;
        }
        Effect01_Delete(o);
        return;
    }
    Effect01_Run(o);
}

s32 Obj_InCurrentArea(void *obj); /* unit's own declaration: u8, as at its marked declaration below (s32 changes this unit's callers) */
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
f32 AreaGrid2_GetFloorY(f32 *pos);
void Vec_RotateSelf(f32 *v, f32 *m);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Em_BonePos(void *obj, s16 bone, f32 *out);
extern f32 gCamMatrix[];
extern s16 gShadowPartBones[];
extern f32 gShadowPartScales[];
extern f32 D_7F18E0;
extern f32 gEffect01Sizes;
extern s8 *gLobby;
/* Effect01_Update's running state: follows the owner player (type 1: its record's
 * position; type 2: five bones, each part over the floor; others: bone 2) and
 * puts the draw slots in the ordering table; the next state once the owner is gone. */
void Effect01_Run(StageObj *o)
{
    f32 v[3];
    EffectParts *w;
    f32 *k;
    s16 i;
    Player *p;
    f32 h;

    p = o->owner;
    switch (o->type) {
    case 1:
        if (gScene.online != 0) {
            if (p->active == 0 || gLobby[p->index + 0x152E] == 0) {
                return;
            }
        } else if (p->active == 0) {
            return;
        }
        o->pos[0] = *(f32 *)((u8 *)p->partRecs[10] + 0x70);
        o->pos[1] = p->floorY;
        o->pos[2] = *(f32 *)((u8 *)p->partRecs[10] + 0x78);
        goto draw;
    case 2:
        w = (EffectParts *)o->buf;
        if (p->active == 0) {
            o->state++;
            return;
        }
        if (p->visible == 0 || (u8)Obj_InCurrentArea(p) == 0) {
            return;
        }
        for (i = 0; i < 5; i++) {
            if (w->draw[i] != NULL) {
                Mtx_GetTrans(w->pos[i], Em_GetBone(p, gShadowPartBones[i])->world);
                h = AreaGrid2_GetFloorY(w->pos[i]);
                if (!(w->pos[i][1] < 8.0f + h)) {
                    w->pos[i][1] = 5.0f + h;
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = p->scale[0] * (-10.0f * gShadowPartScales[i]);
                    Vec_RotateSelf(v, gCamMatrix);
                    w->draw[i]->pos[0] = v[0] + w->pos[i][0];
                    w->draw[i]->pos[1] = v[1] + w->pos[i][1];
                    w->draw[i]->pos[2] = v[2] + w->pos[i][2];
                    OT_AddByDepth(gDrawOtSorted, w->draw[i], 0x40, 0);
                }
            }
        }
        return;
    default:
        if (p->active == 0) {
            o->state++;
            return;
        }
        Em_BonePos(p, 2, o->pos);
        o->pos[1] = p->floorY;
    draw:
        if (gScene.online == 0 || p->unk01E != 1 || p->weaponType != 3) {
            if ((u8)Obj_InCurrentArea(p) == 0) {
                return;
            }
        }
        if (p->visible == 0) {
            return;
        }
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 0.0f;
        if (p->isEm != 0) {
            if (gScene.online != 0) {
                k = &D_7F18E0;
            } else {
                k = &gEffect01Sizes;
            }
            v[2] = p->scale[0] * (-10.0f * (*k + p->weaponType * 2));
            Vec_RotateSelf(v, gCamMatrix);
        }
        ((DrawSlot *)o->node)->pos[0] = o->pos[0] + v[0];
        ((DrawSlot *)o->node)->pos[1] = o->pos[1] + v[1];
        ((DrawSlot *)o->node)->pos[2] = o->pos[2] + v[2];
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
        break;
    }
}

void DrawSlot_Free(s32 id);
void DrawSlotB_Free(s32 id);
/* Frees an effect object's draw slots (type 2: the five of its buffer). */
void Effect01_End(StageObj *o)
{
    EffectParts *w;
    s16 i;

    o->state++;
    switch (o->type) {
    case 2:
        w = (EffectParts *)o->buf;
        for (i = 0; i < 5; i++) {
            if (w->draw[i] != NULL) {
                DrawSlot_Free(w->slot[i]);
            }
        }
        break;
    default:
        if (*(DrawSlot **)&o->node != NULL) {
            if (o->keep) {
                DrawSlotB_Free(o->slot);
            } else {
                DrawSlot_Free(o->slot);
            }
        }
        break;
    }
}

void Effect01_Delete(StageObj *o)
{
    StageObj_Free(o);
}


void Effect01_SetLights(u8 *o, u8 *lights, s32 *list, f32 scale)
{
    s32 i;

    for (i = 0; i < list[1]; i++) {
        u8 *l = lights + list[i + 2] * 0x4C;

        *(f32 *)(l + 0x10) = *(f32 *)(o + 0x6E4) * scale;
        Gs_SetState(i + 0x3A, (s32)l);
    }
}

f32 AreaGrid2_GetFloorY(f32 *pos);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
/* Walks a bone tree (child +0xD0, sibling +0xCC): each bone's matrix with its height set to the ground's (clamped to base +-30) plus 5 + 0.3 * n, times the bone's +0x80 matrix, to entry 0x1A + index. */
void Bone_SnapToGround(u8 *bone, u8 n, f32 base)
{
    f32 pos[3];
    f32 out[4][4];
    f32 m[4][4];
    f32 *y;
    f32 h;

    Mtx_Copy(m[0], (f32 *)bone);
    y = &m[3][1];
    pos[0] = m[3][0];
    pos[1] = *y;
    pos[2] = m[3][2];
    h = AreaGrid2_GetFloorY(pos);
    if (h > base + 30.0f) {
        h = base + 30.0f;
    } else if (h < base - 30.0f) {
        h = base - 30.0f;
    }
    *y = h + (5.0f + 0.3f * n);
    Mtx_Mul(out[0], (f32 *)(bone + 0x80), m[0]);
    Gs_SetState(*(s16 *)(bone + 0xC4) + 0x1A, (s32)out);
    if (*(u8 **)(bone + 0xD0) != NULL) {
        Bone_SnapToGround(*(u8 **)(bone + 0xD0), n, base);
    }
    if (*(u8 **)(bone + 0xCC) != NULL) {
        Bone_SnapToGround(*(u8 **)(bone + 0xCC), n, base);
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
extern Model *D_005DA034;
extern s16 gShadowBonesEm07[];
extern s16 gShadowBonesEm37[];
extern s16 gShadowBonesEm3A[];
/* Draws an effect object's model (the slot's owner) under its owner monster or player. */
void Effect01_Draw(DrawSlot *s)
{
    f32 m[4][4];
    f32 out[4][4];
    f32 sm[4][4];
    f32 v[4][3];
    f32 pos[3];
    StageObj *o;
    f32 *vp;
    Model *set;
    f32 *tbl;
    ModelMatSet *model;
    ModelMat *mats;
    s32 i;
    Player *p;
    s16 *bones;
    s16 k;
    f32 sx;
    f32 sz;
    f32 bright;
    u16 ang;
    f32 h;
    f32 ax;
    f32 az;
    EffectParts *w;

    o = s->owner;
    p = o->owner;
    if (p->hideShadow != 0) {
        return;
    }
    if (o->type == 0 || o->type == 3) {
        set = D_005DA034;
    } else if (o->type == 4) {
        k = EmSlot_FindKind(p->weaponType);
        if (k == -1) {
            return;
        }
        set = gScene.partModels[k];
    } else {
        set = gCommonModels[0];
    }
    if (p->isEm == 0) {
        if (Player_IsLocal(p) == 1 && p->aimLockTimer != 0) {
            return;
        }
    } else if (gScene.online != 0) {
        tbl = &D_7F18E0;
    } else {
        tbl = &gEffect01Sizes;
    }
    if (gScene.online == 0 || p->unk01E != 1 || p->weaponType != 3) {
        if (gScene.area != p->area) {
            return;
        }
    }
    if (set == NULL || p->visible == 0) {
        return;
    }
    if (set->active == 0) {
        return;
    }
    Gs_SetState(0x60, 0);
    Gs_SetState(0x6C, 0);
    Gfx_SetState63(1);
    mats = set->mats;
    switch (o->type) {
    case 0:
    case 3:
        model = set->matSets;
        Gs_SetState(0x67, -1);
        if (p->isEm == 0) {
            Mtx_Scale(sm[0], 4.0f, 1.0f, 4.0f);
            bright = 1.0f;
        } else {
            sx = p->scale[0] * tbl[p->weaponType * 2];
            sz = p->scale[2] * tbl[p->weaponType * 2 + 1];
            switch (p->weaponType) {
            case 7:
            case 0x32:
            case 0x37:
            case 0x3A:
            case 0x48:
                switch (p->weaponType) {
                case 7:
                case 0x32:
                    bones = gShadowBonesEm07;
                    break;
                case 0x37:
                    bones = gShadowBonesEm37;
                    break;
                case 0x3A:
                case 0x48:
                    bones = gShadowBonesEm3A;
                    break;
                }
                model = &set->matSets[1];
                for (i = 0; i < 4; i++) {
                    Em_GetBonePos(p, bones[i], v[i]);
                    Vec_Diff(v[i], v[i], o->pos);
                    Vec_RotateY(v[i], 6.2831855f * -p->rot[1] / 65536.0f);
                }
                ax = Math_Absf(0.5f * (v[0][0] + v[2][0]) - 0.5f * (v[1][0] + v[3][0]));
                az = Math_Absf(0.5f * (v[0][2] + v[1][2]) - 0.5f * (v[2][2] + v[3][2]));
                sx += 0.02f * ax;
                sz += 0.02f * az;
                bright = 1.0f;
                break;
            case 0x33:
            case 0x34:
            case 0x35:
                bright = EmBlangonga_ShadowFade((Em *) p);
                break;
            case 0x3B:
                if (EmChameleos_IsInvisible((Em *) p) == 1) {
                    bright = 0.0f;
                } else {
                    bright = 1.0f;
                }
                break;
            default:
                bright = 1.0f;
                break;
            }
            Mtx_Scale(sm[0], sx, 1.0f, sz);
        }
        Mtx_Translation(m[0], o->pos[0], 3.0f + o->pos[1], o->pos[2]);
        Mtx_RotateY(m[0], Math_Angle16ToRad(((Player *)o->owner)->rot[1]));
        Mtx_Concat3x3(m[0], sm[0]);
        Bone_UpdateWorld(set->bones, m[0]);
        if (o->type == 0) {
            Bone_StoreMatrices(set->bones);
        } else {
            Bone_SnapToGround((u8 *)set->bones, o->ownerIndex, p->floorY);
        }
        if (model != NULL && model->id != -1) {
            Effect01_SetLights((u8 *)p, (u8 *)mats, (s32 *)model, bright);
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        break;
    case 1:
        k = Effect_MapTexId(0x33);
        if (k != -1) {
            model = &set->matSets[k];
            Mtx_Scale(sm[0], 1.0f, 1.0f, 1.0f);
            for (i = 0; i < 2; i++) {
                if (i == 0) {
                    Mtx_GetTrans(pos, p->partRecs[5]->m1);
                } else {
                    Mtx_GetTrans(pos, p->partRecs[8]->m1);
                }
                pos[1] -= 13.0f;
                h = AreaGrid2_GetFloorY(pos);
                if (!(h < pos[1])) {
                    h = pos[1];
                }
                Mtx_Identity(m[0]);
                ang = Vec_Heading(p->pos, gAreaState.skyPos) + 0x4000;
                Mtx_RotateY(m[0], 6.2831855f * ang / 65536.0f);
                Mtx_SetTrans(m[0], pos[0], 4.0f + h, pos[2]);
                Mtx_Mul(out[0], (f32 *)sm, m[0]);
                Gs_SetState(0x1A, (s32)out);
                if (gScene.online == 0 && Area_IsDay(gScene.area) == 0) {
                    bright = 0.5f;
                } else {
                    bright = 1.0f;
                }
                Effect01_SetLights((u8 *)p, (u8 *)mats, (s32 *)model, bright);
                Draw_ApplyStates(model->flags);
                Model_Draw(model->id, 0);
            }
        }
        break;
    case 2:
        w = (EffectParts *)o->buf;
        k = Effect_MapTexId(0x32);
        if (k != -1) {
            model = &set->matSets[k];
            Mtx_Scale(sm[0], gShadowPartScales[s->part], gShadowPartScales[s->part], gShadowPartScales[s->part]);
            Mtx_Translation(m[0], w->pos[s->part][0], w->pos[s->part][1], w->pos[s->part][2]);
            Mtx_Mul(out[0], (f32 *)sm, m[0]);
            Gs_SetState(0x1A, (s32)out);
            Effect01_SetLights((u8 *)p, (u8 *)mats, (s32 *)model, 1.0f);
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        break;
    case 4:
        model = &set->matSets[3];
        Mtx_Scale(sm[0], tbl[p->weaponType * 2], 1.0f, tbl[p->weaponType * 2 + 1]);
        Mtx_Translation(m[0], o->pos[0], 3.0f + o->pos[1], o->pos[2]);
        Mtx_Mul(out[0], (f32 *)sm, m[0]);
        Gs_SetState(0x1A, (s32)out);
        Gs_SetState(0x67, 0x80FFFFFF);
        Effect01_SetLights((u8 *)p, (u8 *)mats, (s32 *)model, 1.0f);
        Draw_ApplyStates(model->flags);
        Model_Draw(model->id, 0);
        break;
    }
    Gs_SetState(0x6C, 1);
    Draw_SetDefaultState();
}

void Em_BonePos(void *obj, s16 bone, f32 *out);
/* A player's object (kind 1, of the given type; type 2 takes a buffer block) at its bone 10, on the ground. */
void Effect01_Spawn(void *obj, s16 type)
{
    Player *p = obj;
    StageObj *o;

    if (type == 2) {
        if ((o = StageObj_Alloc(1)) == NULL) {
            return;
        }
    } else {
        if ((o = StageObj_Alloc(0)) == NULL) {
            return;
        }
    }
    o->kind = 1;
    o->update = Effect01_Update;
    o->draw = NULL;
    o->owner = p;
    o->type = type;
    if (o->type == 1) {
        o->keep = 1;
        return;
    }
    o->ownerIndex = p->index;
    Em_BonePos(p, 10, o->pos);
    o->pos[1] = p->floorY;
}

/* The update of the kind 2 effect objects (Effect02_SpawnForPlayer and the like), by state. */
void Effect02_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        Effect02_Init(o);
        break;
    case 1:
        Effect02_RunTimed(o);
        break;
    case 2:
        Effect02_End(o);
        break;
    case 3:
        Effect02_Delete(o);
        break;
    }
}

u16 Rand_Next(s32 a);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Effect02_Draw(DrawSlot *s);

/* Effect02_Update's first state for types 0-12: shows the object, randomizes
 * it per type (angle, scale, colour; type 8 starts at a bone of its owner)
 * and gives it a draw slot drawn by Effect02_Draw (none free: freed). The
 * factors are one bit above 0.0004f and 0.01f: the quotients fold to them. */
void Effect02_Init(StageObj *o)
{
    f32 m[16];
    f32 v[3];

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    switch (o->type) {
    case 0:
        o->timer = -4;
        break;
    case 4:
        o->angle = Rand_Next(1);
        break;
    case 5:
        o->scale *= 0.8f + (0.4f / 1000.0f) * (Rand_Next(1) & 0x3FF);
        break;
    case 7:
        o->scale *= 0.8f + (0.4f / 1000.0f) * (Rand_Next(1) & 0x3FF);
        o->angle = Rand_Next(1);
        break;
    case 6:
        o->var1 = Rand_Next(1) % 3;
        break;
    case 8:
        v[0] = (0.3f / 30.0f) * ((Rand_Next(1) & 0x3FF) - 0x200);
        v[1] = 5.0f + (0.3f / 30.0f) * (Rand_Next(1) & 0x3FF);
        v[2] = 0.0f;
        Mtx_Copy(m, (f32 *)Em_GetBone(o->owner, o->var0));
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(o->pos, m);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        break;
    case 9:
    case 10:
    case 11:
        if (o->var0 & 1) {
            o->timer = -8;
        }
        o->var1 = Rand_Next(1) >> 8;
        o->var2 = Rand_Next(1) >> 8;
        o->angle = Rand_Next(1);
        break;
    case 12:
        o->scale = 2.0f;
        break;
    case 2:
        break;
    case 1:
    case 3:
    default:
        o->var1 = Rand_Next(1) >> 8;
        o->var2 += ((Rand_Next(1) & 0xFFF) - 0x800) >> 8;
        o->angle += (s16)((Rand_Next(1) & 0xFFF) - 0x800);
        break;
    }
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        *(DrawSlot **)&o->node = DrawSlot_Get(o->slot);
        (*(DrawSlot **)&o->node)->owner = o;
        (*(DrawSlot **)&o->node)->draw = Effect02_Draw;
        return;
    }
    StageObj_Free(o);
}

extern f32 gCamMatrix[];

/* Effect02_Update's timed kinds (types 0-12): the timer runs in steps of 2
 * to a life per type (then the next state, hidden); some drift up, types 4,
 * 6 and 12 are drawn pushed towards the camera. */
void Effect02_RunTimed(StageObj *o)
{
    s16 life;
    f32 v[3];

    o->timer += 2;
    switch (o->type) {
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
    if (o->timer <= 0) {
        return;
    }
    if (o->timer > life) {
        o->state++;
        o->visible = 0;
        return;
    }
    if (o->node == NULL) {
        return;
    }
    switch (o->type) {
    case 0:
    case 2:
        if (o->timer >= 16) {
            o->pos[1] -= 4.0f;
        }
        break;
    case 3:
        if (o->timer >= 10) {
            o->pos[1] -= 5.0f;
        }
        break;
    case 4:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = -20.0f;
        Vec_RotateSelf(v, gCamMatrix);
        ((DrawSlot *)o->node)->pos[0] = o->pos[0] + v[0];
        ((DrawSlot *)o->node)->pos[1] = o->pos[1] + v[1];
        ((DrawSlot *)o->node)->pos[2] = o->pos[2] + v[2];
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
        return;
    case 6:
    case 12:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 50.0f;
        Vec_RotateSelf(v, gCamMatrix);
        ((DrawSlot *)o->node)->pos[0] = o->pos[0] + v[0];
        ((DrawSlot *)o->node)->pos[1] = o->pos[1] + v[1];
        ((DrawSlot *)o->node)->pos[2] = o->pos[2] + v[2];
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
        return;
    case 8:
        o->pos[1] += -0.4f * o->timer;
        break;
    }
    ((DrawSlot *)o->node)->pos[0] = o->pos[0];
    ((DrawSlot *)o->node)->pos[1] = o->pos[1];
    ((DrawSlot *)o->node)->pos[2] = o->pos[2];
    OT_AddByDepth(gDrawOt32, o->node, 0x20, 0);
}

void Effect02_End(StageObj *o)
{
    o->state++;
    DrawSlot_Free(o->slot);
}

void Effect02_Delete(StageObj *o)
{
    StageObj_Free(o);
}

extern u8 gEffectColors[][4];
extern f32 gEffect02ScaleKeys6[];
extern f32 gEffect02Keys12[];
extern s32 gEffect02ColorKeys12[];
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_RotateZXY(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_TurnZ(f32 *m, f32 a);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_SetBlendFlags(u16 flags);
void Effect_ScaleVec(f32 *v);
s32 Effect_KindClass_Unk(s32 a);
s16 Effect_MapTexId(s16 id);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);

/* The kind 2 effect's draw slot: per type, a model of the stage model set
 * (+0x30) with its scale, rotation and colour (a colour of the table at
 * 0x41CEC0, or keyed for type 12); type 0 is drawn twice, the second turned
 * half a circle. */
void Effect02_Draw(DrawSlot *s)
{
    Model *set;
    StageObj *o;
    ModelMat *mats;
    ModelMatSet *mdl;
    u16 flags;
    s16 ci;
    u8 r, g, b;
    u8 t;
    s16 id;
    f32 d;
    f32 m2[16];
    f32 m1[16];
    f32 v[3];
    u32 color;

    set = gCommonModels[0];
    o = s->owner;
    flags = 0;
    if (set != NULL && set->active != 0) {
    mats = set->mats;
    Mtx_Identity(m1);
    switch (o->type) {
    case 0:
    case 1:
        Gfx_SetState63(0);
        flags |= 0x10;
        Gs_SetState(0x60, 0x80);
        v[0] = 1.0f;
        v[1] = 1.0f;
        v[2] = 1.0f;
        if (o->type == 0) {
            r = 0xFF;
            ci = -1;
            g = r;
            b = r;
        } else {
            Effect_ScaleVec(v);
            ci = Effect_KindClass_Unk(o->ownerIndex);
        }
        Mtx_Scale(m2, v[0], v[1], v[2]);
        t = o->type;
        Mtx_RotateZ(m2, 6.2831855f * (u16)((o->var1 << 8) + o->var2) / 65536.0f);
        Mtx_RotateY(m2, 6.2831855f * (u16)o->angle / 65536.0f);
        mdl = &set->matSets[o->timer / 2];
        Mtx_SetTrans(m1, 0.0f, 0.125f * t, 0.0f);
        break;
    case 2:
        Gfx_SetState63(0);
        flags |= 0x10;
        Gs_SetState(0x60, 0x80);
        Mtx_Scale(m2, 0.1f, 0.1f, 0.7f);
        Mtx_RotateZXY(m2, 6.2831855f * (o->var1 << 8) / 65536.0f, 6.2831855f * (o->var2 << 8) / 65536.0f, 6.2831855f * o->angle / 65536.0f);
        mdl = &set->matSets[o->timer / 2];
        Mtx_SetTrans(m1, 0.75f, 0.25f, 0.0f);
        r = 0xFF;
        ci = -1;
        g = r;
        b = r;
        break;
    case 3:
        Gs_SetState(0x60, 0xC0);
        mdl = set->matSets + o->timer / 2 + 97;
        v[0] = o->scale;
        v[1] = o->scale;
        v[2] = o->scale;
        Mtx_Scale(m2, v[0], v[1], v[2]);
        Mtx_RotateZXY(m2, 6.2831855f * (o->var2 << 8) / 65536.0f, 6.2831855f * o->angle / 65536.0f, 6.2831855f * (o->var1 << 8) / 65536.0f);
        if (o->var0 == 0) {
            Mtx_SetTrans(m1, 0.75f, 0.25f, 0.0f);
        } else {
            Mtx_SetTrans(m1, 0.0f, 0.0f, 0.0f);
        }
        ci = Effect_KindClass_Unk(o->ownerIndex);
        break;
    case 4:
        Gs_SetState(0x60, 0);
        mdl = set->matSets + o->timer / 2 + 27;
        v[0] = o->scale;
        v[1] = o->scale;
        v[2] = o->scale;
        if (o->var1 == 1) {
            r = 0xFF;
            ci = -1;
            g = r;
            b = r;
            flags |= 2;
            Mtx_SetTrans(m1, 0.125f, 0.0f, 0.0f);
        } else {
            Effect_ScaleVec(v);
            ci = Effect_KindClass_Unk(o->ownerIndex);
            Mtx_SetTrans(m1, 0.0f, 0.0f, 0.0f);
        }
        Mtx_Scale(m2, v[0], v[1], v[2]);
        Mtx_RotateZ(m2, 6.2831855f * o->angle / 65536.0f);
        Mtx_Concat3x3(m2, gCamMatrix);
        break;
    case 5:
    case 7:
        if (o->type == 5) {
            d = 24.0f;
            id = 0x35;
        } else {
            d = 24.0f;
            id = 0x4C;
        }
        id = Effect_MapTexId(id);
        if (id == -1) {
            return;
        }
        mdl = &set->matSets[id];
        Gs_SetState(0x60, 0x80);
        Mtx_Scale(m2, o->scale, o->scale, o->scale);
        Mtx_RotateY(m2, 6.2831855f * o->angle / 65536.0f);
        Mtx_SetTrans(m1, 0.0f, o->timer * (0.125f / d), 0.0f);
        r = 0xFF;
        ci = -1;
        g = r;
        b = r;
        break;
    case 6:
        flags |= 2;
        Key_LerpVec3(gEffect02ScaleKeys6, v, o->timer / 2.0f);
        v[0] *= o->scale;
        v[1] *= o->scale;
        v[2] *= o->scale;
        Gs_SetState(0x60, 0);
        mdl = set->matSets + o->var1 + 32;
        Mtx_Scale(m2, v[0], v[1], v[2]);
        Mtx_SetTrans(m1, 0.25f, 0.0f, 0.0f);
        Mtx_Concat3x3(m2, gCamMatrix);
        r = 0xFF;
        ci = -1;
        g = r;
        b = r;
        break;
    case 8:
        Gs_SetState(0x60, 0);
        mdl = &set->matSets[39];
        Mtx_Scale(m2, o->scale, o->scale, o->scale);
        Mtx_Concat3x3(m2, gCamMatrix);
        r = 0xFF;
        ci = -1;
        g = r;
        b = r;
        break;
    case 9:
    case 10:
    case 11:
        Gs_SetState(0x60, 0x80);
        Mtx_Scale(m2, o->scale, o->scale, o->scale);
        Mtx_RotateXYZ(m2, 6.2831855f * (o->var2 << 8) / 65536.0f, 6.2831855f * o->angle / 65536.0f, 6.2831855f * (o->var1 << 8) / 65536.0f);
        mdl = set->matSets + o->timer / 2 + 134;
        ci = -1;
        if (o->type == 9) {
            r = 0xFF;
            g = r;
            b = r;
        } else if (o->type == 10) {
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
        Gs_SetState(0x60, 0);
        flags |= 2;
        mdl = set->matSets;
        Key_LerpVec3(gEffect02Keys12, v, o->timer / 2.0f);
        ci = -2;
        v[0] = o->scale;
        v[1] = o->scale;
        v[2] = o->scale;
        Color_EvalKeys(gEffect02ColorKeys12, o->timer / 2.0f, &color);
        Mtx_Scale(m2, v[0], v[1], v[2]);
        Mtx_RotateZ(m2, 1.308997f);
        Mtx_Concat3x3(m2, gCamMatrix);
        break;
    }
    Mtx_SetTrans(m2, s->pos[0], s->pos[1], s->pos[2]);
    Gs_SetState(0x19, (s32)m1);
    if (ci == -2) {
        Gs_SetState(0x67, color);
    } else {
        if (ci >= 0) {
            r = gEffectColors[ci][0];
            g = gEffectColors[ci][1];
            b = gEffectColors[ci][2];
        }
        Gs_SetState(0x67, (r << 16) | 0xFF000000 | (g << 8) | b);
    }
    Gs_SetState(0x1A, (s32)m2);
    if (mdl != NULL && mdl->id != -1) {
        Gs_SetLightList(mats, mdl);
        Draw_ApplyStates(mdl->flags);
        Draw_SetBlendFlags(flags);
        Model_Draw(mdl->id, 0);
    }
    if (o->type == 0) {
        Gs_SetState(0x67, (r << 16) | 0xFF000000 | (g << 8) | b);
        Mtx_TurnZ(m2, 3.1415927f);
        Gs_SetState(0x1A, (s32)m2);
        if (mdl != NULL && mdl->id != -1) {
            Gs_SetLightList(mats, mdl);
            Draw_ApplyStates(mdl->flags);
            Draw_SetBlendFlags(flags);
            Model_Draw(mdl->id, 0);
        }
    }
    Gfx_SetState63(1);
    Draw_SetDefaultState();
    if (o->type == 6) {
        Gs_SetState(0x60, 0);
    } else {
        Gs_SetState(0x60, 0x80);
    }
}
}

/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Obj_InCurrentArea(void *obj);
StageObj *StageObj_Alloc(s32 nblk);
/* An effect object (kind 2, of the given type) at pos, belonging to the player and turned away from its heading. */
void Effect02_SpawnForPlayer(Player *p, u8 type, f32 *pos)
{
    StageObj *o;

    if (p != NULL && (u8)Obj_InCurrentArea(p) != 0) {
        o = StageObj_Alloc(0);
        if (o != NULL) {
            o->kind = 2;
            o->update = Effect02_Update;
            o->type = type;
            o->owner = p;
            o->scale = 1.0f;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->angle = p->hitAngle + 0x8000;
        }
    }
}

u16 Rand_Next(s32 a);
/* An effect object (kind 2, of the given type) at pos, with the high bytes of a and b and a random angle. */
void Effect02_SpawnRandomAngle(u16 a, u16 b, u8 type, f32 *pos)
{
    StageObj *o;

    if ((o = StageObj_Alloc(0)) != NULL) {
        o->kind = 2;
        o->update = Effect02_Update;
        o->type = type;
        o->scale = 1.0f;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->var1 = a >> 8;
        o->var2 = b >> 8;
        o->angle = Rand_Next(1);
    }
}

/* An effect object (kind 2, of the given type) at pos with angle ang and scale, belonging to o. */
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale)
{
    StageObj *o;

    if ((u8)Obj_InCurrentArea(owner) != 0 && (o = StageObj_Alloc(0)) != NULL) {
        o->kind = 2;
        o->update = Effect02_Update;
        o->type = type;
        o->owner = owner;
        o->scale = scale;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->var0 = sub;
        o->var2 = 0;
        o->angle = ang;
        if (((u8 *)owner)[0x10] != 0) {
            o->ownerIndex = ((u8 *)owner)[2];
        } else {
            o->ownerIndex = 0;
        }
    }
}

/* An effect object (kind 2, of the given type) at pos with angle ang and scale. */
void Effect02_SpawnAtScaled(u16 a, u16 ang, s32 type, f32 *pos, f32 scale)
{
    StageObj *o = StageObj_Alloc(0);

    if (o != NULL) {
        o->kind = 2;
        o->update = Effect02_Update;
        o->type = type;
        o->owner = NULL;
        o->scale = scale;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->var2 = a >> 8;
        o->angle = ang;
    }
}

/* An effect object (kind 2, of the given type) at pos with scale: unk006 is
 * 1 for kinds 9-16, else 0 and the owner's area byte goes to unk01E. */
void Effect02_Spawn(u8 *owner, s16 kind, u8 type, u8 sub, f32 *pos, f32 scale)
{
    StageObj *o;

    if ((u8)Obj_InCurrentArea(owner) != 0 && (o = StageObj_Alloc(0)) != NULL) {
        o->kind = 2;
        o->update = Effect02_Update;
        o->type = type;
        o->scale = scale;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->var0 = sub;
        o->ownerIndex = 0;
        switch (kind) {
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
            o->var1 = 1;
            break;
        default:
            o->var1 = 0;
            if (owner[0x10] != 0) {
                o->ownerIndex = owner[2];
            }
            break;
        }
        o->var2 = 0;
        o->angle = 0;
    }
}

/* An effect object (kind 2, of the given type) belonging to owner, with the given scale. */
void Effect02_SpawnOnOwner(void *owner, u8 type, u8 sub, f32 scale)
{
    StageObj *o;

    if ((u8)Obj_InCurrentArea(owner) != 0) {
        o = StageObj_Alloc(0);
        if (o != NULL) {
            o->kind = 2;
            o->update = Effect02_Update;
            o->type = type;
            o->owner = owner;
            o->scale = scale;
            o->var0 = sub;
            o->var2 = 0;
            o->angle = 0;
        }
    }
}

StageObj *StageObj_Alloc(s32 nblk);
/* An effect object (kind 2, of the given type) at pos, turned to ang. */
void Effect02_SpawnAt(f32 *pos, s32 type, u16 ang)
{
    StageObj *o = StageObj_Alloc(0);

    if (o != NULL) {
        o->kind = 2;
        o->update = Effect02_Update;
        o->type = type;
        o->owner = NULL;
        o->scale = 1.0f;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->angle = ang;
    }
}

/* An effect object (kind 2, of the given type) at pos with angle ang and scale. */
void Effect02_SpawnScaled(s16 ang, u8 type, u8 sub, f32 *pos, f32 scale)
{
    StageObj *o = StageObj_Alloc(0);

    if (o != NULL) {
        o->kind = 2;
        o->update = Effect02_Update;
        o->type = type;
        o->owner = NULL;
        o->scale = scale;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->var0 = sub;
        o->var2 = 0;
        o->angle = ang;
        o->ownerIndex = 0;
    }
}

/* unk_3BE060's .data (0x4655E0-0x4657A0) */
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
f32 gEffect02ScaleKeys6[20] = {
#include "sub_main/00465700.inc"
};
f32 gEffect02Keys12[12] = {
#include "sub_main/00465750.inc"
};
s32 gEffect02ColorKeys12[8] = {
#include "sub_main/00465780.inc"
};
