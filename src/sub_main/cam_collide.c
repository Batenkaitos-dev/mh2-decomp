/*
 * The camera eye's collision tests: Cam_LineHitDist sweeps a sphere
 * along the line to the eye and pushes the eye out of the area's walls,
 * and Cam_PushOutOfEms pushes a point out of the hit shapes of the
 * monsters in the current area. The data: the swept sphere's offset and
 * radius (gCamCollideSphere).
 */

#include "common.h"
#include "camera.h"
#include "em.h"
#include "objhead.h"
#include "capsule.h"
#include "area.h"
#include "engine_math.h"
#include "equip_shop.h"
#include "hit_setup.h"
#include "vec_math.h"

extern f32 gCamCollideSphere[];

/*
 * Pushes the point fromPos out of the area's walls along the line from
 * toPos: sweeps a sphere of radius gCamCollideSphere[3] from toPos to
 * fromPos, both
 * offset by gCamCollideSphere[0..2] (AreaGrid_SweepCells, skipping the
 * triangle flags CAM_LINE_MASK), which resolves the hits into fromPos. *dist gets
 * how far fromPos moved, 0 when nothing is hit. Returns 0; does nothing
 * while CamCtl.mode has CAMCTL_NO_LINE_TEST. The sweep's result
 * is read as u8 (as this unit's code takes it), so its -1 (no segment)
 * counts as a hit too. The comma in the subscript gCamCollideSphere[0, 0]
 * gives the game's float register order. Seen: CamFollow_Update
 * passes the eye, CamFollow.unk18 and &hitDist.
 */
f32 Cam_LineHitDist(f32 *fromPos, f32 *toPos, f32 *dist)
{
    f32 sweepFrom[3];
    f32 sweepTo[3];
    f32 fromStart[3];

    if (CamCtl_GetMode() & CAMCTL_NO_LINE_TEST) {
        return 0.0f;
    }
    /* FAKEMATCH: the comma in the subscript gives the offsets the
     * game's float registers (f1, f3, f2); every plain spelling gives f3, f2, f1. */
    sweepFrom[0] = toPos[0] + gCamCollideSphere[0, 0];
    sweepFrom[1] = toPos[1] + gCamCollideSphere[1];
    sweepFrom[2] = toPos[2] + gCamCollideSphere[2];
    sweepTo[0] = fromPos[0] + gCamCollideSphere[0];
    sweepTo[1] = fromPos[1] + gCamCollideSphere[1];
    sweepTo[2] = fromPos[2] + gCamCollideSphere[2];
    Vec_Set(fromStart, fromPos[0], fromPos[1], fromPos[2]);
    if ((u8)AreaGrid_SweepCells(sweepFrom, sweepTo, fromPos, CAM_LINE_MASK, gCamCollideSphere[3]) > 0) {
        *dist = Vec_Distance(fromStart, fromPos);
    } else {
        *dist = 0.0f;
    }
    return 0.0f;
}

extern HitShape *gEmBoneHits[];

/* Adds push v to the push acc: the larger one when their signs differ, else their sum. */
#define PUSH_ADD(acc, v)                                                                      \
    if ((!(acc < 0.0f) && v < 0.0f) || (acc < 0.0f && !(v < 0.0f)) ||                         \
        (!(acc < -0.0f) && v < -0.0f) || (acc < -0.0f && !(v < -0.0f))) {                     \
        if (Math_Absf(acc) < Math_Absf(v)) {                                                  \
            acc = -v;                                                                         \
        }                                                                                     \
    } else {                                                                                  \
        acc -= v;                                                                             \
    }

/*
 * Pushes the point pos out of the hit shapes of the monsters in the
 * current area: each of the EM_COUNT slots that is active, a monster
 * (isEm), visible and in gScene.area tests a sphere at pos +
 * gCamCollideSphere[0..2] of radius gCamCollideSphere[3] against the
 * shapes of its kind's list (gEmBoneHits, or the online game's copy).
 * It skips the shapes with atkFlags 4, atkFlags2 2, openFlags 8 or 2, or
 * offFlags 2 (no camera push), steps over group headers (HitList_Next),
 * pushes from spheres with Sphere_PushOut and from capsules with
 * Capsule_PushSphere. Each axis collects the negated pushes (PUSH_ADD):
 * of two in the same direction the larger counts, opposite ones add up.
 * pos then moves by the sum and stays at least 10 above the floor
 * (AreaGrid2_GetFloorY). Every shape is tested from the starting pos.
 * Nothing while CamCtl.mode has CAMCTL_NO_EM_PUSH. unused1 is
 * not read (seen: CamFollow.unk18).
 */
void Cam_PushOutOfEms(f32 *pos, f32 *unused1)
{
    f32 cap[8];
    f32 sph[4];
    f32 work[16];
    f32 probe[3];
    f32 push[3];
    HitShape *shape;
    Em *em;
    f32 *sphere;
    s32 i;
    s32 boneNo;
    f32 minY;
    f32 pushZ;
    f32 pushY;
    f32 pushX;

    if (CamCtl_GetMode() & CAMCTL_NO_EM_PUSH) {
        return;
    }
    em = gEms;
    sphere = gCamCollideSphere;
    pushX = pushY = pushZ = 0.0f;
    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active != 0 && em->isEm != 0 && em->visible != 0 && gScene.area == em->area) {
            probe[0] = pos[0] + sphere[0];
            probe[1] = pos[1] + sphere[1];
            probe[2] = pos[2] + sphere[2];
            if (gScene.inLobby == 0) {
                shape = gEmBoneHits[em->type];
            } else {
                shape = gEmBoneHitsLobby[em->type];
            }
            while ((boneNo = shape->bone) != HIT_BONE_END) {
                if ((shape->atkFlags & 4) || (shape->atkFlags2 & 2) || (shape->openFlags & 8) || (shape->openFlags & 2) || (shape->offFlags & 2)) {
                    shape++;
                    continue;
                }
                if (boneNo == HIT_BONE_GROUP) {
                    HitList_Next(em, &shape);
                    continue;
                }
                switch ((s16)BoneHit_Place((ObjHead *)em, shape, (struct HitCapsule *) cap, (struct HitSphere *) sph)) {
                case 0:
                    if (Sphere_PushOut(sph[3], sphere[3], sph, probe, push) != 0) {
                        PUSH_ADD(pushX, push[0]);
                        PUSH_ADD(pushY, push[1]);
                        PUSH_ADD(pushZ, push[2]);
                    }
                    break;
                case 1:
                    Capsule_Copy((struct HitCapsule *) cap, (struct Capsule *) work);
                    if (Capsule_PushSphere((struct Capsule *) work, probe, push, sphere[3]) != 0) {
                        PUSH_ADD(pushX, push[0]);
                        PUSH_ADD(pushY, push[1]);
                        PUSH_ADD(pushZ, push[2]);
                    }
                    break;
                }
                shape++;
            }
        }
    }
    if (pushX != 0.0f || pushY != 0.0f || pushZ != 0.0f) {
        pos[0] += pushX;
        pos[1] += pushY;
        pos[2] += pushZ;
        minY = 10.0f + AreaGrid2_GetFloorY(pos);
        if (pos[1] < minY) {
            pos[1] = minY;
        }
    }
}

/* This file's initialised data. */
f32 gCamCollideSphere[8] = {
#include "sub_main/0041CE40.inc"
};
