/*
 * The rail camera, mode 2 of the camera controller (CamCtl gCamCtl;
 * camera.h; its work is CamCtl.rail): an init and an update handler that
 * CamCtl_Update calls through gCamModeInit and gCamModeUpdate. It follows
 * the camera area's path by the area's kind: a fixed point, a point from
 * the player, or along the path's segments (gCamMode2Segs). It turns the
 * view toward the player within limits and blends in from the view it
 * started from.
 */
#include "common.h"
#include "em.h"
#include "camera.h"
#include "player.h"
#include "model.h"
#include "engine_math.h"
#include "cam_areas.h"
#include "vec_math.h"

/* end of prototypes */

/* The init handler of camera mode 2 (gCamModeInit): no roll, a 55-degree field of view. */
void CamRail_Init(CamCtl *ctl, CamView *view)
{
    view->roll = 0.0f;
    ctl->rail.roll = 0.0f;
    view->fov = 0.9599311f;
    ctl->rail.fov = 0.9599311f;
}

extern PathSeg gCamMode2Segs[];

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
