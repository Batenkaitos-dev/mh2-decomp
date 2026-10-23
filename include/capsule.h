#ifndef CAPSULE_H
#define CAPSULE_H

/*
 * The collision shapes: the capsules and spheres the hit tests and the pushes
 * between players and monsters build, the hit lists' records (HitShape), and
 * the geometry tests on them (segments, capsules, spheres, cylinders and
 * slabs: contacts, distances, push-outs and clipping).
 */

#include "common.h"
#include "objattach.h"

/* A capsule: the segment from a to b and its radius. BoneHit_Place and Player_BodyCapsule write
 * one, Capsule_Copy reads one. */
typedef struct HitCapsule {
    f32 a[3];                   /* 0x00 */
    f32 b[3];                   /* 0x0C */
    f32 radius;                 /* 0x18 */
} HitCapsule;
typedef char HitCapsule_size_check[(sizeof(HitCapsule) == 0x1C) ? 1 : -1];

/* A segment set up for the tests (Segment_Init): its two ends, the direction, the middle and
 * half its length. */
typedef struct Segment {
    f32 a[3];                   /* 0x00 */
    f32 b[3];                   /* 0x0C */
    f32 dir[3];                 /* 0x18: b - a */
    f32 mid[3];                 /* 0x24 */
    f32 halfLen;                /* 0x30: from a to mid */
} Segment;
typedef char Segment_size_check[(sizeof(Segment) == 0x34) ? 1 : -1];

/* A capsule set up for the tests (Capsule_Copy fills it from a HitCapsule): the two ends, the
 * radius, the direction, the middle, half the length plus the radius. */
typedef struct Capsule {
    f32 a[3];                   /* 0x00 */
    f32 b[3];                   /* 0x0C */
    f32 radius;                 /* 0x18 */
    f32 dir[3];                 /* 0x1C: b - a */
    f32 mid[3];                 /* 0x28 */
    f32 len;                    /* 0x34: half the length plus the radius */
} Capsule;
typedef char Capsule_size_check[(sizeof(Capsule) == 0x38) ? 1 : -1];

/* A sphere: its centre and radius (BoneHit_Place and HitShape_Place write one). */
typedef struct HitSphere {
    f32 pos[3];                 /* 0x00 */
    f32 radius;                 /* 0x0C */
} HitSphere;
typedef char HitSphere_size_check[(sizeof(HitSphere) == 0x10) ? 1 : -1];

/* A hit shape: one record of a hit list (ObjAttach.hitList and hurtList from
 * the kinds' tables; a monster's gEmBoneHits[type], and the online game's own
 * copy). bone places it: one of the owner's bones (BoneHit_Place), or one of
 * the HIT_BONE_* codes below. */
#define HIT_BONE_AT_POS 0x7F    /* a sphere or capsule at the owner's position (HitShape_Place) */
#define HIT_BONE_SEGMENT 0x7E   /* the capsule from the object's pos to pos2, with this radius (Capsule_FromPoints) */
#define HIT_BONE_GROUP 0x7D     /* the header of a group of records (HitList_Next): kind holds the owner's Em.fxFlags bits that skip the group, zone the number of records in it */
#define HIT_BONE_END (-1)       /* ends the list */
/* HitShape's flag bytes. An attack's hit (ObjAttach_HitTarget) and the
 * pushes (Cam_PushOutOfEms, PlayerEm_PushApart) test them. ATTACK_TRAP_A and
 * ATTACK_TRAP_B are the kinds every family takes as a trap (EM_DMG_ATTACK6
 * and EM_DMG_ATTACK9: RATH_REACT_TRAPPED), and ATTACK_HOLD the one that
 * gives EM_DMG_ATTACK13 (RATH_REACT_HOLD2). Both pushes skip a shape with HITSHAPE_TRAP_ONLY,
 * HITSHAPE2_HOLD_ONLY, HITSHAPE_WIND_ONLY or HITSHAPE_HERMIT_ONLY. */
/* atkFlags */
#define HITSHAPE_TRAP_HIT 0x01      /* ATTACK_TRAP_A and ATTACK_TRAP_B hit this shape (on a monster this console runs); without it they pass through */
#define HITSHAPE_WIND_HIT 0x02      /* ATTACK_WIND and ATTACK_WIND_WEAK hit this shape; without it they pass through */
#define HITSHAPE_TRAP_ONLY 0x04     /* only ATTACK_TRAP_A and ATTACK_TRAP_B hit it, on a monster this console runs */
/* atkFlags2 */
#define HITSHAPE2_HOLD_HIT 0x01     /* ATTACK_HOLD hits this shape (on a monster this console runs); without it, it passes through */
#define HITSHAPE2_HOLD_ONLY 0x02    /* only ATTACK_HOLD hits it, on a monster this console runs */
/* openFlags: ObjAttach_HitFlags reports bit 1 for a hit on a shape with
 * either wind bit while Em.windTimer runs, bit 2 for either hermit bit
 * while Em.hermitTimer runs. */
#define HITSHAPE_WIND_OPEN 0x01     /* reported while Em.windTimer runs */
#define HITSHAPE_WIND_ONLY 0x02     /* reported, and hit only while Em.windTimer runs and the attack has flag 0x10 */
#define HITSHAPE_HERMIT_OPEN 0x04   /* reported while Em.hermitTimer runs */
#define HITSHAPE_HERMIT_ONLY 0x08   /* reported, and hit only while Em.hermitTimer runs */
/* ObjAttach_HitFlags' result: which of the shape's open states hold now */
#define HIT_OPEN_WIND 0x01          /* a wind bit, while Em.windTimer runs */
#define HIT_OPEN_HERMIT 0x02        /* a hermit bit, while Em.hermitTimer runs */
/* offFlags */
#define HITSHAPE_NO_HIT 0x01        /* never hit; PlayerEm_PushApart skips it too */
#define HITSHAPE_NO_CAM_PUSH 0x02   /* doesn't push the camera out (Cam_PushOutOfEms) */
#define HITSHAPE_NO_PUSH 0x04       /* doesn't push a player out (PlayerEm_PushApart) */
#define HIT_SHAPE_SPHERE 0         /* HitShape.kind */
#define HIT_SHAPE_CAPSULE 1
typedef struct HitShape {
    s16 bone;                   /* 0x00 */
    s16 kind;                   /* 0x02: 0 sphere, 1 capsule (HIT_SHAPE_SPHERE, HIT_SHAPE_CAPSULE) */
    s16 zone;                   /* 0x04: a monster's: the row of its hit zones (ObjAttach_DamageEm, hitPending); an attack's: 1 picks motion value kind 3 over 2 */
    s16 part;                   /* 0x06: the monster part (& 7: Em.partFlags, hitPartDamage; flinchPart) */
    u8 atkFlags;                /* 0x08: HITSHAPE_TRAP_HIT, HITSHAPE_WIND_HIT, HITSHAPE_TRAP_ONLY (ObjAttach_HitTarget) */
    u8 atkFlags2;               /* 0x09: HITSHAPE2_HOLD_HIT, HITSHAPE2_HOLD_ONLY */
    u8 openFlags;               /* 0x0A: 3 while Em.windTimer runs, 0xC while Em.hermitTimer does (ObjAttach_HitFlags); 2 and 8 hit only then */
    u8 offFlags;                /* 0x0B: 1 never hit, 2 no camera push (Cam_PushOutOfEms), 4 no player push (PlayerEm_PushApart) */
    f32 radius;                 /* 0x0C: times the owner's scale (BoneHit_Place) or the object's hitScale */
    f32 a[3];                   /* 0x10: the centre or the capsule's first end (in bone space, or from the position) */
    f32 b[3];                   /* 0x1C: the capsule's second end */
} HitShape;
typedef char HitShape_size_check[(sizeof(HitShape) == 0x28) ? 1 : -1];

void Segment_Init(f32 *start, f32 *end, Segment *seg);
void Capsule_Copy(HitCapsule *src, Capsule *dst);
s32 Spheres_Touch(f32 radiusA, f32 radiusB, f32 *centerA, f32 *centerB, f32 *out);
s32 Sphere_PushOut(f32 radiusA, f32 radiusB, f32 *centerA, f32 *centerB, f32 *out);
u8 Capsule_ContactCapsule(Capsule *capA, Capsule *capB, f32 *out);
void Vec_PointAlong(f32 *from, f32 *dir, f32 *out, f32 len, f32 dist);
f32 Seg_NearestPointDist(f32 *out, f32 *start, f32 *end, f32 *point);
f32 Seg_SegDistance(f32 *startA, f32 *endA, f32 *startB, f32 *endB);
s32 Capsule_SphereContact(f32 *center, Capsule *cap, f32 *out, f32 sphereRadius);
s32 Capsule_PushSphere(Capsule *cap, f32 *center, f32 *out, f32 sphereRadius);
s32 Capsule_SphereContactOnSphere(Capsule *cap, f32 *center, f32 *out, f32 sphereRadius);
s32 Capsule_PushCapsule(Capsule *capA, Capsule *capB, f32 *out);
void Vec_PushOut(f32 *dir, f32 reach, f32 dist, f32 *out);
s32 Vec_ProjectOnDir(f32 *from, f32 *to, f32 *dir, f32 *out, f32 reach);
u8 Capsule_Classify(Segment *seg, f32 *point, f32 radius);
f32 Seg_PointDistance(f32 *start, f32 *end, f32 *dir, f32 *point, f32 len);
void Seg_NearestPoint(f32 *start, f32 *end, f32 *dir, f32 *point, f32 *out, f32 len);
s32 Cyl_Contains(f32 *point, f32 *center, f32 radius, f32 top, f32 bottom);
u8 Slab_Contains(f32 *pos, f32 *start, f32 *end, f32 height, f32 radius);
s32 Seg_ClipPlane(f32 *cap, f32 *planePoint, f32 *nrm, f32 *out);
s32 Seg_ClipYRange(void *cap, f32 top, f32 bottom, void *planePoint, f32 *nrm, void *out);
void Hit_ResetAll(void);
void Hit_ClearStatusAll(void);
void HitList_Next(Em *em, HitShape **cur);
void Em_ApplyHitStatus(Player *target, ObjAttach *attach);
void Player_UpdateHitTimers(Player *player);
void ObjAttach_HitObjects(ObjAttach *attach);
void ObjAttach_HitAll(void);
s32 ObjAttach_TargetIsRemote(Em *target);
void ObjAttach_DamageEm(ObjAttach *attach, Player *attacker, Em *em, HitShape *list, HitShape *elist, f32 *out);
void ObjAttach_DamagePlayer(ObjAttach *attach, Player *attacker, Player *victim, f32 *pos);
void ObjAttach_SetHitHeading(ObjAttach *attach, Player *target, f32 *from, f32 *to);
void ObjAttach_LinkTarget(ObjAttach *attach, Player *attacker);
void ObjAttach_NextMultiHit(ObjAttach *attach);
void ObjAttach_UpdateTimers(ObjAttach *attach);
void ObjAttach_UpdateAllTimers(void);
void HitId_Reset(void);
u8 HitId_Next(void);

#endif /* CAPSULE_H */
