#ifndef HIT_SETUP_H
#define HIT_SETUP_H

#include "common.h"
#include "capsule.h"
#include "objattach.h"
#include "objhead.h"

/*
 * Setting up hits: an attack object (ObjAttach) started on a row of its
 * family's attack table, with an owner or without one
 * (ObjAttach_StartOwnedAttack, ObjAttach_StartAttack), and the hit shapes
 * placed in the world as a sphere or a capsule: at the owner's bone
 * (BoneHit_Place), at a position (HitShape_Place) or between two points
 * (Capsule_FromPoints).
 */

void ObjAttach_LoadOwnedRow(ObjAttach *obj, void *owner, s32 row);
void ObjAttach_StartOwnedAttack(ObjAttach *obj, s32 owner, s32 row, void *rows);
void ObjAttach_LoadRow(ObjAttach *obj, s32 row);
void ObjAttach_StartAttack(ObjAttach *obj, s32 row, void *rows);
s32 BoneHit_Place(ObjHead *owner, HitShape *def, HitCapsule *cap, HitSphere *sph);
s32 HitShape_Place(f32 *pos, HitShape *shape, HitCapsule *cap, HitSphere *sph, f32 scale);
s32 Capsule_FromPoints(f32 *start, f32 *end, HitShape *shape, HitCapsule *out, f32 scale);

#endif
