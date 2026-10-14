#ifndef CAPSULE_H
#define CAPSULE_H

/*
 * The collision capsules of sub_main (unk_3BE060.c's tests, unk_3AC9C0.c's pushes).
 */

#include "common.h"

/* A capsule: the segment from a to b and its radius. BoneHit_Place and Player_BodyCapsule write
 * one, Capsule_Copy reads one. */
typedef struct HitCapsule {
    f32 a[3];                   /* 0x00 */
    f32 b[3];                   /* 0x0C */
    f32 radius;                 /* 0x18 */
} HitCapsule;
typedef char HitCapsule_size_check[(sizeof(HitCapsule) == 0x1C) ? 1 : -1];

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

/* A hit shape: one record of a hit list (ObjAttach.hitList and hurtList from the kinds' tables;
 * a monster's gEmBoneHits[type], online D_7F1950), bone -1 ends the list. bone places it: one of
 * the owner's bones (BoneHit_Place), 0x7F a sphere or capsule at the owner's position
 * (HitShape_Place), 0x7E the capsule from an object's pos to pos2 with this radius
 * (Capsule_FromPoints), 0x7D the header of a group of records (HitList_Next): its kind holds the
 * bits of the owner's Em.fxFlags that skip the group, its zone the number of records in it. */
typedef struct HitShape {
    s16 bone;                   /* 0x00 */
    s16 kind;                   /* 0x02: 0 sphere, 1 capsule */
    s16 zone;                   /* 0x04: a monster's: the row of its hit zones (ObjAttach_DamageEm, hitPending); an attack's: 1 picks motion value kind 3 over 2 */
    s16 part;                   /* 0x06: the monster part (& 7: Em.partFlags, hitPartDamage; flinchPart) */
    u8 atkFlags;                /* 0x08: 1 hit by attack kinds 6 and 9, 2 by 5 and 0xE, 4 only by 6 and 9 on a monster this console runs (ObjAttach_HitPlayer) */
    u8 atkFlags2;               /* 0x09: 1 hit by attack kind 0x13, 2 only by it on a monster this console runs */
    u8 openFlags;               /* 0x0A: 3 while Em.windTimer runs, 0xC while Em.hermitTimer does (ObjAttach_HitFlags); 2 and 8 hit only then */
    u8 offFlags;                /* 0x0B: 1 never hit, 2 no camera push (Cam_PushOutOfEms), 4 no player push (PlayerEm_PushApart) */
    f32 radius;                 /* 0x0C: times the owner's scale (BoneHit_Place) or the object's hitScale */
    f32 a[3];                   /* 0x10: the centre or the capsule's first end (in bone space, or from the position) */
    f32 b[3];                   /* 0x1C: the capsule's second end */
} HitShape;
typedef char HitShape_size_check[(sizeof(HitShape) == 0x28) ? 1 : -1];

#endif /* CAPSULE_H */
