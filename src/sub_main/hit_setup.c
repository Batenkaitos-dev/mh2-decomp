/*
 * Setting up hits: an attack object (ObjAttach) started on a row of its
 * family's attack table, with an owner (a Player or an Em, whose index it
 * keeps) or without one (ObjAttach_StartOwnedAttack, ObjAttach_StartAttack),
 * and the hit shapes (HitShape) placed in the world, by the owner's bone or
 * at a position, as a sphere or a capsule (BoneHit_Place, HitShape_Place,
 * Capsule_FromPoints). The attack families start their objects here, and
 * the hit tests, the pushes between bodies and the camera place their
 * shapes here. Player_StartNextHit, first, is empty: the hit timers call it
 * with the hits left.
 */
#include "common.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
#include "capsule.h"
#include "objattach.h"
#include "hit_setup.h"
#include "engine_math.h"
/* end of prototypes */

/* Does nothing: the game's function is empty. Player_UpdateHitTimers calls
 * it with the hits left, where an attack object would start its next hit. */
void Player_StartNextHit(Player *unused0, u8 unused1)
{
}

/*
 * ObjAttach_LoadRow with an owner: its index (ObjHead.index) as the
 * object's ownerIndex. An element already on the object (the low byte of
 * elementFlags less ELEMF_EM_STATUS1, with its elementPower) stays, added
 * to the row's.
 */
void ObjAttach_LoadOwnedRow(ObjAttach *obj, void *owner, s32 row)
{
    ObjHead *ownerHead = owner;
    s32 *src;
    s32 *dst;
    s32 k;
    u8 keepFlags;
    u8 keepPower;

    keepFlags = obj->elementFlags & 0xF7;
    keepPower = keepFlags == 0 ? 0 : obj->elementPower;
    src = *(s32 **)obj->attackRows;
    src += row * 6;
    dst = (s32 *)&obj->hitDelay;
    for (k = 0; k < 6; k++) {
        *dst++ = *src++;
    }
    obj->ownerIndex = ownerHead->index;
    obj->hitPhase = 1;
    obj->didHit = 0;
    obj->targetEm = NULL;
    obj->targetPart = NULL;
    if (keepPower != 0) {
        obj->elementFlags |= keepFlags;
        obj->elementPower = keepPower;
    }
    obj->hitDelay >>= 1;
    obj->hitFrames >>= 1;
    if (obj->mvFramesA != 255) {
        obj->mvFramesA >>= 1;
    }
    obj->mvFramesB >>= 1;
    obj->hitId = HitId_Next();
}

/* Starts an attack object on row `row` of the family's table `rows` (the
 * address of its table pointer), for the owner (a Player or an Em), with no
 * element carried over. */
void ObjAttach_StartOwnedAttack(ObjAttach *obj, s32 owner, s32 row, void *rows)
{
    obj->attackRows = rows;
    obj->elementPower = 0;
    obj->elementFlags = 0;
    ObjAttach_LoadOwnedRow(obj, (void *) owner, row);
}

/*
 * Copies attack row `row` of the object's table over its fields, word by
 * word, for an attack with no owner (ownerIndex 255). The hit window waits
 * (hitPhase 1), nothing is hit or targeted yet, the row's frame counts are
 * halved, and the attack takes a new hit id.
 */
void ObjAttach_LoadRow(ObjAttach *obj, s32 row)
{
    s32 *src;
    s32 *dst;
    s32 k;

    src = *(s32 **)obj->attackRows;
    src += row * 6;
    dst = (s32 *)&obj->hitDelay;

    for (k = 0; k < 6; k++) {
        *dst++ = *src++;
    }
    obj->ownerIndex = 255;
    obj->hitPhase = 1;
    obj->didHit = 0;
    obj->targetEm = NULL;
    obj->targetPart = NULL;
    obj->hitDelay >>= 1;
    obj->hitFrames >>= 1;
    if (obj->mvFramesA != 255) {
        obj->mvFramesA >>= 1;
    }
    obj->mvFramesB >>= 1;
    obj->hitId = HitId_Next();
}

/* Starts an attack object with no owner on row `row` of the family's table
 * `rows` (the address of its table pointer). */
void ObjAttach_StartAttack(ObjAttach *obj, s32 row, void *rows)
{
    obj->attackRows = rows;
    ObjAttach_LoadRow(obj, row);
}

/* Places hit shape `def` in the world by its owner's bone: a sphere into
 * sph (returns 0) or a capsule into cap (returns 1); -1 for a shape not on
 * a bone (bone 127) or of another kind. */
s32 BoneHit_Place(ObjHead *owner, HitShape *def, HitCapsule *cap, HitSphere *sph)
{
    f32 off[3];
    f32 pos[3];
    EmBone *bones;

    if (def->bone == 127) {
        return -1;
    }
    bones = owner->model->bones;
    Mtx_GetTrans(pos, bones[def->bone].world);
    switch (def->kind) {
    case 0:
        sph->radius = def->radius * owner->scale[0];
        Vec_Rotate(off, def->a, (f32 *)&bones[def->bone]);
        sph->pos[0] = pos[0] + off[0];
        sph->pos[1] = pos[1] + off[1];
        sph->pos[2] = pos[2] + off[2];
        return 0;
    case 1:
        cap->radius = def->radius * owner->scale[0];
        Vec_Rotate(off, def->a, (f32 *)&bones[def->bone]);
        cap->a[0] = pos[0] + off[0];
        cap->a[1] = pos[1] + off[1];
        cap->a[2] = pos[2] + off[2];
        Vec_Rotate(off, def->b, (f32 *)&bones[def->bone]);
        cap->b[0] = pos[0] + off[0];
        cap->b[1] = pos[1] + off[1];
        cap->b[2] = pos[2] + off[2];
        return 1;
    }
    return -1;
}

/* Places a hit shape at pos, its radius scaled by `scale`: a sphere (kind
 * 0) into sph, returning 0, or a capsule (kind 1) into cap, returning 1;
 * -1 for another kind. */
s32 HitShape_Place(f32 *pos, HitShape *shape, HitCapsule *cap, HitSphere *sph, f32 scale)
{
    switch (shape->kind) {
    case 0:
        sph->radius = shape->radius * scale;
        sph->pos[0] = pos[0] + shape->a[0];
        sph->pos[1] = pos[1] + shape->a[1];
        sph->pos[2] = pos[2] + shape->a[2];
        return 0;
    case 1:
        cap->radius = shape->radius * scale;
        cap->a[0] = pos[0] + shape->a[0];
        cap->a[1] = pos[1] + shape->a[1];
        cap->a[2] = pos[2] + shape->a[2];
        cap->b[0] = pos[0] + shape->b[0];
        cap->b[1] = pos[1] + shape->b[1];
        cap->b[2] = pos[2] + shape->b[2];
        return 1;
    }
    return -1;
}

/* The capsule from start to end with the shape's radius scaled by `scale`;
 * returns 1. */
s32 Capsule_FromPoints(f32 *start, f32 *end, HitShape *shape, HitCapsule *out, f32 scale)
{
    out->radius = shape->radius * scale;
    out->a[0] = start[0];
    out->a[1] = start[1];
    out->a[2] = start[2];
    out->b[0] = end[0];
    out->b[1] = end[1];
    out->b[2] = end[2];
    return 1;
}

