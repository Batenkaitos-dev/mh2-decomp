/*
 * Three of the camera controller's modes (CamCtl gCamCtl; camera.h), each an
 * init and an update handler that CamCtl_Update calls through gCamModeInit
 * and gCamModeUpdate, and the camera focus:
 * - mode 2, the rail camera (CamRail_*): follows the camera area's path,
 *   a fixed point, one from the player, or along the path's segments, and
 *   turns toward the player within limits;
 * - mode 3, the aim camera (CamAim_*): the bowgun's scope with the right
 *   stick's zoom, the mounted gun, or a set position in the lobby, with
 *   the eye and target from gCamMode3Offsets;
 * - mode 4, the fixed camera (CamFixed_*): the area's fixed view while the
 *   player fishes (gAreaCameraIdx, gCamMode4AreaPos);
 * - the camera focus (CamFocus_*): eases the view toward an object or a
 *   point over 20 frames and back.
 * The controller's core, the follow and orbit cameras come before this
 * unit; the requests and the script mode after it.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "camera.h"
#include "player.h"
#include "model.h"

/* prototypes */
f32 Math_Pow(f32 x, f32 y);
void Mtx_Identity(f32 *m);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Vec_Copy(f32 *dst, f32 *src);
void CamFixed_UpdateTarget(CamCtl *c, CamView *v, CamFixed *w);

s32 Player_IsFishing(Player *pl);
s32 CamFixed_AreaCamera(CamCtl *c, CamView *v, CamFixed *w);
s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 Player_OnMountedGun(Player *p);
void Vec_RotateY(f32 *v, f32 a);
void Vec_Add(f32 *out, f32 *a, f32 *b);
extern u8 gAreaCameraIdx[];
extern f32 gCamMode4AreaPos[][6];
extern f32 gCamMode4AreaFovs[];
/* end of prototypes */


void CamFocus_Update(CamCtl *c, CamView *v, CamFocus *f);


/* The init handler of camera mode 2 (gCamModeInit): no roll, a 55-degree field of view. */
void CamRail_Init(CamCtl *ctl, CamView *view)
{
    view->roll = 0.0f;
    ctl->rail.roll = 0.0f;
    view->fov = 0.9599311f;
    ctl->rail.fov = 0.9599311f;
}

extern PathSeg gCamMode2Segs[];
s32 Math_Atan2Angle(f32 y, f32 x);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_Length(f32 *v);
void Vec_RotateX(f32 *v, f32 a);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t);
void CamPoint_GetPos(CamCtl *c, f32 *out, CamArea *area);
void CamPoint_GetPosOr(CamCtl *c, f32 *out, CamArea *area, f32 *k);
void Path_PointAt(f32 *out, CamCtl *o, CamArea *area, PathSeg *segs);
s32 CamCtl_PathPlace(CamCtl *c, PathSeg *segs, f32 *pos, s32 flag);
void Vec_EvalCubic(f32 *out, PathSeg *k, f32 t);
f32 CamArea_FovScale(CamArea *area, f32 dist);
f32 PathKey_Blend(PathData *d, s32 i, f32 t);
f32 PathKey_BlendAngle(PathData *d, s32 i, f32 t);
void CamView_SetTransition(CamCtl *c, CamView *v, CamArea *area);
f32 CamBlend_EaseStep(CamView *v);

/* The update handler of camera mode 2 (ctl->areaMode == 2): follows the camera area's path (ctl->area) by its
 * kind (1: fixed, 3: from the player, 2: along the segments at gCamMode2Segs); from step 1 it
 * turns the view by at most the limits from the distance unless ctl->areaChanged is set; then
 * blends from the view it started from (view->blend 1..2). */
void CamRail_Update(CamCtl *ctl, CamView *view)
{
    CamRail *railWork;
    CamArea *area;
    s16 pitchKick;
    PathSeg *segs;
    PathData *pathData;
    f32 fovBase;
    f32 blendT;
    s16 yaw;
    s16 pitch;
    s16 pitchLimit;
    s16 yawLimit;
    Player *pl;
    s16 diff;
    f32 vec[3];

    segs = gCamMode2Segs;
    view->active = 0;
    if (ctl->areaMode != 2) {
        *(u16 *)&view->step = 0;
        return;
    }
    area = ctl->area;
    if (area->kind == 0) {
        *(u16 *)&view->step = 0;
        ctl->areaMode = 0;
        return;
    }
    pl = ctl->player;
    railWork = &ctl->rail;
    switch (view->step) {
    case 0:
        view->step++;
        view->active = 1;
        view->blend = 0;
        switch (area->kind) {
        case 1:
            Vec_Copy(railWork->pos, area->fixed.eye);
            CamPoint_GetPos(ctl, railWork->at, area);
            fovBase = area->fixed.fov;
            railWork->roll = area->fixed.roll;
            break;
        case 3:
            Vec_Add(railWork->pos, pl->pos, area->fixed.eye);
            CamPoint_GetPos(ctl, railWork->at, area);
            fovBase = area->fixed.fov;
            railWork->roll = area->fixed.roll;
            break;
        case 2:
            pathData = &area->path.d;
            CamCtl_PathPlace(ctl, segs, pl->pos, 0);
            Vec_EvalCubic(vec, &segs[ctl->pathPos.k], ctl->pathPos.dist);
            CamPoint_GetPosOr(ctl, railWork->at, area, vec);
            Path_PointAt(railWork->pos, ctl, area, segs);
            fovBase = PathKey_Blend(pathData, ctl->pathPos.k, ctl->pathPos.t);
            railWork->roll = PathKey_BlendAngle(pathData, ctl->pathPos.k, ctl->pathPos.t);
            CamPoint_GetPosOr(ctl, railWork->at, area, vec);
            Path_PointAt(railWork->pos, ctl, area, segs);
            break;
        }
        Vec_Diff(view->dir, railWork->at, railWork->pos);
        railWork->ry = Math_Atan2Angle(view->dir[0], view->dir[2]);
        railWork->rx = Math_Atan2Angle(-view->dir[1], Vec_DistanceXZ(railWork->at, railWork->pos));
        railWork->unk22 = 0;
        railWork->unk20 = 0;
        break;
    case 1:
        view->active = 1;
        switch (area->kind) {
        case 1:
            Vec_Copy(railWork->pos, area->fixed.eye);
            CamPoint_GetPos(ctl, railWork->at, area);
            fovBase = area->fixed.fov;
            railWork->roll = area->fixed.roll;
            break;
        case 3:
            Vec_Add(railWork->pos, pl->pos, area->fixed.eye);
            CamPoint_GetPos(ctl, railWork->at, area);
            fovBase = area->fixed.fov;
            railWork->roll = area->fixed.roll;
            break;
        case 2:
            pathData = &area->path.d;
            if (ctl->areaChanged == 0) {
                CamCtl_PathPlace(ctl, segs, pl->pos, 1);
            } else {
                CamCtl_PathPlace(ctl, segs, pl->pos, 0);
            }
            Vec_EvalCubic(vec, &segs[ctl->pathPos.k], ctl->pathPos.dist);
            fovBase = PathKey_Blend(pathData, ctl->pathPos.k, ctl->pathPos.t);
            railWork->roll = PathKey_BlendAngle(pathData, ctl->pathPos.k, ctl->pathPos.t);
            CamPoint_GetPosOr(ctl, railWork->at, area, vec);
            Path_PointAt(railWork->pos, ctl, area, segs);
            break;
        }
        Vec_Diff(view->dir, railWork->at, railWork->pos);
        if (ctl->areaChanged != 0) {
            railWork->ry = Math_Atan2Angle(view->dir[0], view->dir[2]);
            railWork->rx = Math_Atan2Angle(-view->dir[1], Vec_DistanceXZ(railWork->at, railWork->pos));
            railWork->unk22 = 0;
            railWork->unk20 = 0;
        } else {
            yaw = Math_Atan2Angle(view->dir[0], view->dir[2]);
            pitch = Math_Atan2Angle(-view->dir[1], Vec_DistanceXZ(railWork->at, railWork->pos));
            pitchKick = 52.15189f * railWork->fov;
            pitchLimit = 521.5189f * railWork->fov;
            diff = pitch - railWork->rx;
            if (pitchLimit < diff) {
                railWork->rx = pitch - pitchLimit;
                railWork->unk20 = pitchKick;
            } else if (diff < -pitchLimit) {
                railWork->rx = pitch + pitchLimit;
                railWork->unk20 = -pitchKick / 2;
            }
            diff = yaw - railWork->ry;
            yawLimit = 1.4285715f * (651.8986f * railWork->fov);
            if (yawLimit < diff) {
                railWork->unk22 = railWork->ry;
                railWork->ry = yaw - yawLimit;
                railWork->unk22 = railWork->ry - railWork->unk22;
                railWork->unk22 = railWork->unk22 - (s16)(railWork->unk22 >> 2);
            } else if (diff < -yawLimit) {
                railWork->unk22 = railWork->ry;
                railWork->ry = yaw + yawLimit;
                railWork->unk22 = railWork->ry - railWork->unk22;
                railWork->unk22 = railWork->unk22 - (s16)(railWork->unk22 >> 2);
            }
        }
        vec[0] = vec[1] = 0.0f;
        vec[2] = Vec_Length(view->dir);
        Vec_RotateX(vec, ANGLE16_TO_RAD * railWork->rx);
        Vec_RotateY(vec, ANGLE16_TO_RAD * railWork->ry);
        Vec_Add(railWork->at, railWork->pos, vec);
        break;
    }
    railWork->fov = fovBase * CamArea_FovScale(area, Vec_Distance(railWork->at, railWork->pos));
    CamView_SetTransition(ctl, view, area);
    switch (view->blend) {
    case 0:
        Vec_Copy(view->eye, railWork->pos);
        Vec_Copy(view->at, railWork->at);
        view->roll = railWork->roll;
        view->fov = railWork->fov;
        break;
    case 1:
        Vec_Copy(view->eye0, ctl->out.eye);
        Vec_Copy(view->at0, ctl->out.at);
        view->roll0 = ctl->out.roll;
        view->fov0 = ctl->out.fov;
        (view->blend)++;
    case 2:
        blendT = CamBlend_EaseStep(view);
        Vec_Lerp(view->eye, view->eye0, railWork->pos, blendT);
        Vec_Lerp(view->at, view->at0, railWork->at, blendT);
        view->roll = view->roll0 * blendT + railWork->roll * (1.0f - blendT);
        view->fov = view->fov0 * blendT + railWork->fov * (1.0f - blendT);
        break;
    }
}

/* The init handler of camera mode 3: a 45-degree field of view; the zoom from 15 to 60 degrees. */
void CamAim_Init(CamCtl *ctl, CamView *view)
{
    f32 fovStart;

    view->fov = 0.7853982f;
    view->roll = 0.0f;
    ctl->aim.fovMin = 0.2617994f;
    ctl->aim.fovMax = 1.0471976f;
    ctl->aim.fovScale = 1.2732395f;
    fovStart = view->fov;
    ctl->aim.fov1 = fovStart;
    ctl->aim.fov0 = fovStart;
}

extern f32 gCamMode3Offsets[][6];
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
s32 Bowgun_HasScope(Player *p);
s32 CamAim_IsSpecialMotion(Player *p);
void Mtx_FromAngles16Pos(f32 *m, s16 x, s16 y, f32 *pos);
s32 CamAim_Situation(Player *pl);

/* The update handler of camera mode 3: by the player's class (CamAim_Situation) a view
 * fixed to the player (its field of view zoomed by the right stick), a rotation about it,
 * or the event position; eye and target then come from gCamMode3Offsets[class]. */
void CamAim_Update(CamCtl *ctl, CamView *view)
{
    CamAim *aimWork;
    Player *pl;
    f32 *zoomFov;
    f32 zoomStep;
    u16 stickMag;
    u16 stickDir;
    f32 moved[4];
    f32 off[4];
    f32 base[4];

    pl = ctl->player;
    view->active = 0;
    if (pl->aimMode == 0) {
        *(u16 *)&view->step = 0;
        return;
    }
    aimWork = &ctl->aim;
    aimWork->kind = CamAim_Situation(pl);
    switch (view->step) {
    case 0:
        aimWork->zoom = 0;
        switch (aimWork->kind) {
        case 0:
            if (CamAim_IsSpecialMotion(pl) == 1) {
                return;
            }
            aimWork->unk65 = 1;
            if (Bowgun_HasScope(pl) == 1) {
                aimWork->zoom = 1;
                aimWork->fovMin = 0.2617994f;
                aimWork->fovMax = 1.0471976f;
                aimWork->fovScale = 1.2732395f;
            }
            break;
        case 1:
            aimWork->unk65 = 0;
            aimWork->zoom = 1;
            aimWork->fovMin = 0.2617994f;
            aimWork->fovMax = 1.0471976f;
            aimWork->fovScale = 1.2732395f;
            break;
        case 2:
            aimWork->unk65 = 0x11;
            view->fov = 0.7853982f;
            break;
        case 3:
            aimWork->unk65 = 0;
            view->fov = 0.890118f;
            break;
        }
        view->step++;
    case 1:
        if (aimWork->zoom != 0) {
            stickMag = ctl->padMagR;
            if (stickMag > 0x50) {
                if (aimWork->kind == 0) {
                    zoomFov = &aimWork->fov0;
                } else {
                    zoomFov = &aimWork->fov1;
                }
                if (stickMag > 0x70) {
                    zoomStep = 0.030543262f;
                } else {
                    zoomStep = stickMag - 0x50;
                    zoomStep *= 0.00095447694f;
                }
                stickDir = ctl->padDegR;
                if ((u16)(stickDir - 0x4000) <= 0x6000) {
                    *zoomFov -= zoomStep;
                    if (*zoomFov < aimWork->fovMin) {
                        *zoomFov = aimWork->fovMin;
                    }
                } else if ((u16)(stickDir - 0x8000 - 0x6000) <= 0x6000) {
                    *zoomFov += zoomStep;
                    if (*zoomFov > aimWork->fovMax) {
                        *zoomFov = aimWork->fovMax;
                    }
                }
            }
        }
        switch (aimWork->kind) {
        case 0:
            if (CamAim_IsSpecialMotion(pl) == 0) {
                Vec_Copy(aimWork->pos, pl->pos);
                Mtx_Copy(aimWork->mtx, pl->partRecs[14]->m1);
            } else {
                Vec_Diff(moved, pl->pos, aimWork->pos);
                Vec_Copy(aimWork->pos, pl->pos);
                Vec_Add(&aimWork->mtx[12], &aimWork->mtx[12], moved);
            }
            view->fov = aimWork->fov0;
            break;
        case 1:
            Mtx_FromAngles16Pos(aimWork->mtx, pl->aimPitch, (s16)(u16)(pl->rot[1] + 0x8000), pl->pos);
            view->fov = aimWork->fov1;
            break;
        case 2:
            Mtx_FromAngles16Pos(aimWork->mtx, pl->aimPitch, (s16)(u16)(pl->rot[1] + 0x8000), pl->pos);
            break;
        case 3:
            Mtx_FromAngles16Pos(aimWork->mtx, aimWork->rx, aimWork->ry, aimWork->pos);
            break;
        }
    default:
        Mtx_GetTrans(base, aimWork->mtx);
        off[0] = gCamMode3Offsets[aimWork->kind][0];
        off[1] = 0.0f;
        off[2] = gCamMode3Offsets[aimWork->kind][2];
        Vec_RotateSelf((f32 *) off, aimWork->mtx);
        Vec_Add(view->eye, base, off);
        view->eye[1] += gCamMode3Offsets[aimWork->kind][1];
        Vec_Rotate(off, &gCamMode3Offsets[aimWork->kind][3], aimWork->mtx);
        Vec_Add(view->at, base, off);
        view->at[1] += gCamMode3Offsets[aimWork->kind][1];
        view->active = 1;
    }
}

/* 1 for the player motions 0x3EA, 0x3F2, 0x579, 0x57C, 0x57D, 0x580, 0x583, 0x58E, 0x58F. */
s32 CamAim_IsSpecialMotion(Player *pl)
{
    switch (pl->motionId[0]) {
    case 0x3EA:
    case 0x579:
    case 0x57C:
    case 0x583:
    case 0x57D:
    case 0x580:
    case 0x3F2:
    case 0x58E:
    case 0x58F:
        return 1;
    }
    return 0;
}

/* mtx = a rotation by the 16-bit angles pitch and yaw, and the translation pos. */
void Mtx_FromAngles16Pos(f32 *mtx, s16 pitch, s16 yaw, f32 *pos)
{
    Mtx_Identity(mtx);
    Mtx_RotateXYZ(mtx, ANGLE16_TO_RAD * (f32)pitch, ANGLE16_TO_RAD * (f32)yaw, 0.0f);
    Vec_Copy(&mtx[12], pos);
}

/* A class of the player's situation: 3 in the lobby, 1 in state 0/0x65 or
 * 0/0x66, 2 when Player_OnMountedGun is 1, 0 when the weapon type
 * (Player.weaponType) is 1 or 5, else -1. */
s32 CamAim_Situation(Player *pl)
{
    if (gScene.inLobby != 0) {
        return 3;
    }
    if (Player_IsState(pl, 0, 0x65) != 0 || Player_IsState(pl, 0, 0x66) != 0) {
        return 1;
    }
    if (Player_OnMountedGun(pl) == 1) {
        return 2;
    }
    if (pl->weaponType == 1 || pl->weaponType == 5) {
        return 0;
    }
    return -1;
}

/* The aim camera's state: *aimFlags its unk65, *zooms whether it zooms; when it does, *zoomT the zoom
 * (the aim view's field of view from fovMin, 0 to 1). Returns the class. */
s8 CamAim_GetState(s32 unused, u8 *aimFlags, u8 *zooms, f32 *zoomT)
{
    CamView *aimView = &gCamCtl.modes[3];
    CamAim *aimWork = (CamAim *)&gCamCtl.aim;

    *aimFlags = aimWork->unk65;
    *zooms = aimWork->zoom;
    if (aimWork->zoom != 0) {
        *zoomT = aimView->fov - aimWork->fovMin;
        *zoomT *= aimWork->fovScale;
    }
    return aimWork->kind;
}

/* Sets the aim camera's position and angles (class 3). */
void CamAim_Set(f32 *pos, s16 pitch, s16 yaw)
{
    CamAim *aimWork = (CamAim *)&gCamCtl.aim;

    Vec_Copy(aimWork->pos, pos);
    aimWork->rx = pitch;
    aimWork->ry = yaw;
}

/* The init handler of camera mode 4 (its work ctl->fixed and the focus; the bytes are cleared as words). */
void CamFixed_Init(CamCtl *ctl, CamView *view)
{
    view->fov = 0.9599311f;
    view->roll = 0.0f;
    *(s32 *)&ctl->fixed.started = 0;
    *(s32 *)&ctl->fixed.focus.state = 0;
    ctl->fixed.focus.on = 0;
}

/* The update handler of camera mode 4. */
void CamFixed_Update(CamCtl *ctl, CamView *view)
{
    CamFixed *fixedWork = &ctl->fixed;

    CamFixed_UpdateTarget(ctl, view, fixedWork);
    if (view->active == 0) {
        CamFocus_Update(ctl, view, &fixedWork->focus);
    }
}

/* Camera mode 4's target step: fixedWork->on gets Player_IsFishing of the player
 * (off: fixedWork->started = 0); the first time, fixedWork->spot from the player's
 * action spot and a 55-degree view; view->active is set when CamFixed_AreaCamera
 * succeeds. */
void CamFixed_UpdateTarget(CamCtl *ctl, CamView *view, CamFixed *fixedWork)
{
    Player *pl;
    s32 fishing;

    view->active = 0;
    pl = ctl->player;
    fishing = Player_IsFishing(pl);
    fixedWork->on = fishing;
    if (fishing == 0) {
        fixedWork->started = 0;
        return;
    }
    if (fixedWork->started == 0) {
        fixedWork->started++;
        fixedWork->spot = pl->spot;
        view->fov = 0.9599311f;
        view->roll = 0.0f;
    }
    if (CamFixed_AreaCamera(ctl, view, fixedWork) >= 0) {
        view->active = 1;
    }
}

/* The area's fixed camera (gAreaCameraIdx by area, 0xFF: none): eye and target from
 * gCamMode4AreaPos turned by the target object's heading, the fov from gCamMode4AreaFovs. */
s32 CamFixed_AreaCamera(CamCtl *ctl, CamView *view, CamFixed *fixedWork)
{
    f32 ofs[3];
    u8 *obj;
    f32 *pos;
    f32 heading;
    u8 camIdx;

    obj = (u8 *)ctl->player;
    if (fixedWork->spot == NULL) {
        return -1;
    }
    camIdx = gAreaCameraIdx[gScene.area];
    if (camIdx == 0xFF) {
        return -1;
    }
    pos = gCamMode4AreaPos[camIdx];
    heading = ANGLE16_TO_RAD * (s32)*(u16 *)(obj + 0xE);
    view->fov = gCamMode4AreaFovs[camIdx];
    Vec_Copy(ofs, pos);
    Vec_RotateY(ofs, heading);
    Vec_Add(view->eye, (f32 *)(obj + 0xAC), ofs);
    Vec_Copy(ofs, pos + 3);
    Vec_RotateY(ofs, heading);
    Vec_Add(view->at, (f32 *)(obj + 0xAC), ofs);
    return 0;
}

/* Starts a camera focus on obj (its position, 20 degrees), or without one
 * at 120 and 300 (30 degrees). */
void CamFocus_Start(Em *obj)
{
    CamFocus *focusWork = (CamFocus *)&gCamCtl.fixed.focus;

    focusWork->state = 0;
    focusWork->on = 1;
    focusWork->obj = obj;
    focusWork->aside = 0;
    if (obj != NULL) {
        focusWork->kind = 0;
        Vec_Copy(focusWork->pos, &obj->posX);
        focusWork->angle = 0.34906587f;
    } else {
        focusWork->kind = 1;
        focusWork->height = 120.0f;
        focusWork->dist = 300.0f;
        focusWork->angle = 0.5235988f;
    }
}

void CamFocus_End(void)
{
    gCamCtl.fixed.focus.on = 0;
}

s32 CamFocus_IsOn(void)
{
    return gCamCtl.fixed.focus.on != 0;
}

void CamFocus_SetAside(s8 asideOn)
{
    gCamCtl.fixed.focus.aside = asideOn;
}

/* The camera focus state: 1 while it runs (state 1 before 20 frames, or
 * state 2), -1 after, 0 when idle. */
s32 CamFocus_GetState(void)
{
    CamFocus *focusWork = (CamFocus *)&gCamCtl.fixed.focus;

    switch (focusWork->state) {
    case 1:
        if (focusWork->timer >= 20) {
            return -1;
        }
    case 2:
        return 1;
    }
    return 0;
}

/* Frame n of 20 as 0..1, eased: 1 out, 2 in (power 2.25), else linear. */
f32 CamFocus_Ease(s32 unused, s16 frame, u8 ease)
{
    f32 eased;

    eased = 0.05f * (f32)frame;
    switch (ease) {
    case 1:
        eased = 1.0f - Math_Pow(1.0f - eased, 2.25f);
        break;
    case 2:
        eased = Math_Pow(eased, 2.25f);
        break;
    }
    return eased;
}

void Vec_Lerp(f32 *out, f32 *a, f32 *b, f32 t);
s32 Npc_IsPet(Em *em);
s32 Chat_InInputMode(void);
u16 Pad_StickToDpad(u8 pad);
void Vec_Resize(f32 *out, f32 len, f32 *v);
f32 CamFocus_Ease(s32 unused, s16 n, u8 ease);

/* The camera focus: step 0 waits for a request (on), steps 1 and 2 ease in and out
 * over 20 frames; the view is lerped from the current mode's view to the focus (by
 * kind: 0 the player and an object or pos, 1 the player with pad-adjusted height and
 * distance, 2 the position pos). */
void CamFocus_Update(CamCtl *ctl, CamView *view, CamFocus *focusWork)
{
    Player *pl;
    CamView *src;
    f32 blendT;
    u16 btn;
    f32 point[4];

    view->active = 0;
    switch (focusWork->state) {
    case 0:
        if (focusWork->on != 0) {
            focusWork->timer = 0;
            focusWork->state++;
            if (focusWork->aside != 0) {
                focusWork->asideTimer = 0x14;
            }
        }
        return;
    case 1:
        if (focusWork->timer < 0x14) {
            focusWork->timer++;
        }
        if (focusWork->on == 0) {
            focusWork->state++;
        }
        break;
    case 2:
        if (focusWork->timer > 0) {
            focusWork->timer--;
        }
        if (focusWork->on != 0) {
            focusWork->state = 1;
            break;
        }
        if (focusWork->timer == 0) {
            focusWork->state = 0;
            return;
        }
        break;
    }
    pl = ctl->player;
    src = &ctl->modes[ctl->areaMode];
    blendT = CamFocus_Ease((s32)focusWork, focusWork->timer, focusWork->state);
    switch (focusWork->kind) {
    case 0:
        if (Npc_IsPet(focusWork->obj) == 1) {
            Vec_Lerp(focusWork->at, pl->pos, &focusWork->obj->posX, 0.5f);
            focusWork->at[1] += 64.0f;
        } else {
            Vec_Lerp(focusWork->at, pl->pos, focusWork->pos, 0.2f);
            focusWork->at[1] += 100.0f;
            if (focusWork->aside != 0) {
                if (focusWork->asideTimer < 0x14) {
                    focusWork->asideTimer++;
                }
            } else {
                if (focusWork->asideTimer > 0) {
                    focusWork->asideTimer--;
                }
            }
            if (focusWork->asideTimer > 0) {
                Vec_Diff(point, focusWork->pos, src->eye);
                Vec_RotateY(point, -(0.45f * view->fov));
                point[0] += src->eye[0];
                point[1] = 100.0f + focusWork->pos[1];
                point[2] += src->eye[2];
                Vec_Lerp(focusWork->at, point, focusWork->at, CamFocus_Ease((s32)focusWork, focusWork->asideTimer, focusWork->aside != 0 ? 1 : 2));
            }
        }
        Vec_Copy(view->eye, src->eye);
        break;
    case 1:
        if (Chat_InInputMode() != 1) {
            btn = Pad_StickToDpad(0);
            if (btn & 0x2000) {
                focusWork->height += 2.0f;
                if (focusWork->height > 160.0f) {
                    focusWork->height = 160.0f;
                }
            }
            if (btn & 0x1000) {
                focusWork->height -= 2.0f;
                if (focusWork->height < 40.0f) {
                    focusWork->height = 40.0f;
                }
            }
            if (btn & 0x800) {
                focusWork->dist -= 8.0f;
                if (focusWork->dist < 120.0f) {
                    focusWork->dist = 120.0f;
                }
            }
            if (btn & 0x400) {
                focusWork->dist += 8.0f;
                if (focusWork->dist > 375.0f) {
                    focusWork->dist = 375.0f;
                }
            }
        }
        Vec_Copy(focusWork->at, pl->pos);
        focusWork->at[1] += focusWork->height;
        Vec_Diff(point, src->eye, focusWork->at);
        Vec_Resize(point, focusWork->dist, point);
        Vec_Add(point, focusWork->at, point);
        Vec_Lerp(view->eye, point, src->eye, blendT);
        break;
    case 2:
        Vec_Lerp(view->eye, focusWork->pos, src->eye, blendT);
        break;
    }
    Vec_Lerp(view->at, focusWork->at, src->at, blendT);
    view->fov = focusWork->angle * blendT + src->fov * (1.0f - blendT);
    view->roll = src->roll;
    view->active = 1;
}
