/*
 * The camera's shakes: CamShake_Set* and CamShake_Req* fill the request
 * slots (gCamCtl.shakeA, B and C), a request near enough to the eye
 * or anywhere, and CamShake_Apply moves the view by them each frame.
 * The data: the shakes' lengths and ranges.
 */

#include "common.h"
#include "camera.h"
#include "em.h"
#include "player.h"
#include "objhead.h"
#include "cam_shake.h"
#include "engine_math.h"
#include "area.h"
extern s16 gCamShakeLengths[];

/*
 * A shake request in slot gCamCtl.shakeA: kind, its length
 * gCamShakeLengths[kind] (10 to 60 frames for kinds 0-5), coming from
 * pos, range 0.
 */
void CamShake_SetA(s32 shakeKind, f32 *pos)
{
    CamShake *shake = &gCamCtl.shakeA;

    shake->active = 1;
    shake->kind = shakeKind;
    shake->length = gCamShakeLengths[shakeKind];
    shake->pos[0] = pos[0];
    shake->pos[1] = pos[1];
    shake->pos[2] = pos[2];
    shake->range = 0;
}

/*
 * A shake request in slot gCamCtl.shakeA felt everywhere: kind with
 * CAM_SHAKE_EVERYWHERE set, its length; pos and range stay as they were.
 */
void CamShake_SetAEverywhere(s32 shakeKind)
{
    CamShake *shake = &gCamCtl.shakeA;

    shake->active = 1;
    shake->kind = shakeKind | CAM_SHAKE_EVERYWHERE;
    shake->length = gCamShakeLengths[shakeKind];
}

/*
 * A shake request kind in slot gCamCtl.shakeA at obj's position (its
 * ObjHead pos), range 0, when Obj_InCurrentArea(obj) is set.
 */
void CamShake_ReqA(void *obj, s32 shakeKind)
{
    CamShake *shake = &gCamCtl.shakeA;

    if ((u8)Obj_InCurrentArea(obj)) {
        shake->active = 1;
        shake->kind = shakeKind;
        shake->length = gCamShakeLengths[shakeKind];
        shake->pos[0] = ((ObjHead *)obj)->pos[0];
        shake->pos[1] = ((ObjHead *)obj)->pos[1];
        shake->pos[2] = ((ObjHead *)obj)->pos[2];
        shake->range = 0;
    }
}

/*
 * As CamShake_ReqA for a monster or player obj (posX, posY, posZ), when
 * Player_InCurrentArea(obj) is set.
 */
void CamShake_ReqAIf(Em *obj, s32 shakeKind)
{
    CamShake *shake = &gCamCtl.shakeA;

    if ((u8)(s32)Player_InCurrentArea(obj)) {
        shake->active = 1;
        shake->kind = shakeKind;
        shake->length = gCamShakeLengths[shakeKind];
        shake->pos[0] = obj->posX;
        shake->pos[1] = obj->posY;
        shake->pos[2] = obj->posZ;
        shake->range = 0;
    }
}

/*
 * CamShake_ReqAIf with range: the gCamShakeRanges index (the distance
 * past which the shake is not felt).
 */
void CamShake_ReqARange(Em *obj, s32 shakeKind, s32 shakeRange)
{
    CamShake *shake = &gCamCtl.shakeA;

    if ((u8)(s32)Player_InCurrentArea(obj)) {
        shake->active = 1;
        shake->kind = shakeKind;
        shake->length = gCamShakeLengths[shakeKind];
        shake->pos[0] = obj->posX;
        shake->pos[1] = obj->posY;
        shake->pos[2] = obj->posZ;
        shake->range = shakeRange;
    }
}

/*
 * CamShake_ReqAIf in slot gCamCtl.shakeC (played with the script view,
 * CamCtl_RunMode).
 */
void CamShake_ReqC(Em *obj, s32 shakeKind)
{
    CamShake *shake = &gCamCtl.shakeC;

    if ((u8)(s32)Player_InCurrentArea(obj)) {
        shake->active = 1;
        shake->kind = shakeKind;
        shake->length = gCamShakeLengths[shakeKind];
        shake->pos[0] = obj->posX;
        shake->pos[1] = obj->posY;
        shake->pos[2] = obj->posZ;
        shake->range = 0;
    }
}

/* CamShake_ReqA in slot gCamCtl.shakeB (played with the aim view). */
void CamShake_ReqB(void *obj, s32 shakeKind)
{
    CamShake *shake = &gCamCtl.shakeB;

    if ((u8)Obj_InCurrentArea(obj)) {
        shake->active = 1;
        shake->kind = shakeKind;
        shake->length = gCamShakeLengths[shakeKind];
        shake->pos[0] = ((ObjHead *)obj)->pos[0];
        shake->pos[1] = ((ObjHead *)obj)->pos[1];
        shake->pos[2] = ((ObjHead *)obj)->pos[2];
        shake->range = 0;
    }
}

extern f32 gCamShakeRanges[];

/*
 * Plays the shake request shake on gCamera while it is active: moves the
 * eye and the target up or down by 10 (kinds 0, 1) or 12, times
 * sin(90 degrees times left / gCamShakeLengths[kind]) (fading as the
 * length runs out), times falloff: 1 when felt everywhere (CAM_SHAKE_EVERYWHERE in
 * kind), else 1 - dist / 2000 by the eye's distance from the shake, and
 * 0 past gCamShakeRanges[range] (2000 to 5000; past 2000 the falloff
 * turns negative). The sign flips every two frames (bit 1 of the length
 * left).
 */
void CamShake_Apply(CamShake *shake)
{
    s32 kind;
    f32 phaseDeg;
    f32 amp;
    f32 falloff;
    f32 dist;

    if (shake->active != 0) {
        kind = shake->kind & CAM_SHAKE_KIND_MASK;
        phaseDeg = 90.0f * ((f32)shake->length / (f32)gCamShakeLengths[kind]);
        if (kind < 2) {
            amp = 10.0f;
        } else {
            amp = 12.0f;
        }
        amp *= Math_Sin(6.2831855f * phaseDeg / 360.0f);
        if (shake->kind & CAM_SHAKE_EVERYWHERE) {
            falloff = 1.0f;
        } else {
            dist = Vec_Distance(shake->pos, gCamera->eye);
            if (dist > gCamShakeRanges[shake->range]) {
                falloff = 0.0f;
            } else {
                falloff = 1.0f - 0.0005f * dist;
            }
        }
        amp *= falloff;
        if ((shake->length >> 1) & 1) {
            amp = -amp;
        }
        gCamera->eye[1] += amp;
        gCamera->at[1] += amp;
    }
}

/* This file's initialised data. */
s16 gCamShakeLengths[8] = {
#include "sub_main/0041C810.inc"
};
f32 gCamShakeRanges[4] = {
#include "sub_main/0041C820.inc"
};
