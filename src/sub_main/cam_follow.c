/*
 * The follow and orbit cameras, two of the camera controller's modes
 * (CamCtl gCamCtl; camera.h), each an init and an update handler that
 * CamCtl_Update calls through gCamModeInit and gCamModeUpdate:
 * - mode 0, the follow camera (CamFollow_*; its work is CamCtl.follow):
 *   stays behind the player at one of its distance settings, with its pad
 *   keys, the turn behind the player or toward the battle target, the
 *   blocked and falling views and the floor under the target;
 * - mode 1, the orbit camera (CamOrbit_*; CamCtl.orbit): the follow
 *   camera's over-the-shoulder view, which runs while the follow view is
 *   active and takes the follow camera's yaw, distance setting and floor.
 *
 * This code is built at optimization level 4 and declares Player_IsState
 * as taking and returning ints: CamFollow_Update compiles to the game's
 * code only with both, and the aim camera's code declares Player_IsState
 * with the definition's own types.
 */
#include "common.h"
#include "em.h"
#include "kbd.h"
#include "scene.h"
#include "player.h"
#include "camera.h"
#include "pad.h"
#include "quest.h"
#include "cam_areas.h"
#include "engine_math.h"
#include "vec_math.h"
#include "cam_helpers.h"

#pragma optimization_level 4

f32 AreaGrid2_GetFloorY(f32 *pos); /* the ground height at pos */
/* end of prototypes */

/*
 * The follow camera's init handler (CAM_MODE_FOLLOW; its work is
 * CamCtl.follow). Its settings are the camera area's in a CAM_AREA_FOLLOW
 * area and gCamFollowDefaults elsewhere, at setting distNo. The eye goes
 * behind the player, the goals start at the current eye and target, the
 * floor under the target is taken, and the follow delay is set to 3 frames.
 */
void CamFollow_Init(CamCtl *ctl, CamView *view)
{
    f32 ofsLocal[4];
    f32 ofsWorld[4];
    s32 angles[4];
    f32 mtx[16];
    CamFollow *work;
    Player *pl;

    pl = ctl->player;
    view->yawFrom = CAM_YAW_FROM_NONE;
    work = &ctl->follow;
    if (ctl->area->kind != CAM_AREA_FOLLOW) {
        work->set = &gCamFollowDefaults;
    } else {
        work->set = &ctl->area->follow;
    }
    work->dist = &work->set->dist[work->distNo];
    work->fov = work->fovGoal = work->set->fov;
    work->roll = work->set->roll;
    work->yaw = pl->rot[1] + ANGLE_TURN / 2;
    angles[0] = 0;
    angles[1] = work->yaw;
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, mtx);
    ofsLocal[0] = 0.0f;
    ofsLocal[1] = work->dist->eyeHeight;
    ofsLocal[2] = work->dist->eyeBack;
    Vec_Rotate(ofsWorld, ofsLocal, mtx);
    work->at[0] = pl->pos[0];
    work->at[1] = pl->pos[1] + work->dist->atY;
    work->at[2] = pl->pos[2];
    work->eye[0] = work->at[0] + ofsWorld[0];
    work->eye[1] = pl->pos[1] + ofsWorld[1];
    work->eye[2] = work->at[2] + ofsWorld[2];
    work->yawGoal = work->yaw;
    Vec_Copy(work->eyeGoal, work->eye);
    Vec_Copy(work->atGoal, work->at);
    work->floorY = AreaGrid2_GetFloorY(work->at);
    work->unk54 = -1;
    work->unk79 = 0;
    Vec_Copy(work->unk18, work->eye);
    *(s32 *)&work->hitDist = 0;
    work->unk58 = 0;
    work->unk60 = 0;
    work->unk73 = 0;
    work->heightState = CAM_HEIGHT_NORMAL;
    work->heightTimer = 0;
    work->eyeHeld = 0;
    work->blockedTimer = 0;
    work->turningBehind = 0;
    work->unk78 = 0;
    work->unk76 = 0;
    work->followDelay = 3;
}

typedef struct Vec3 {
    f32 x, y, z;
} Vec3;

extern s16 gCamOrbitExitBlend;
extern f32 gPlayerBodySpheres[3];
f32 AreaGrid2_GetFloorY(f32 *pos);
/* unit's own declaration: s32 arguments and result (the definition's s16 and u8 ones give CamFollow_Update other code) */
s32 Player_IsState(void *obj, s32 a, s32 b);
s32 AreaGrid_SweepSphere(f32 *from, f32 *to, f32 *hit, u16 flags, f32 radius);

/*
 * The follow camera's update handler (CAM_MODE_FOLLOW; its work is
 * CamCtl.follow), active while areaMode is the follow camera.
 * Distance setting: CAM_DIST_FALL in the fall view, CAM_DIST_BLOCKED (with
 * a 70-degree fov) while blockedTimer runs, else distNo, which PAD_UP and
 * PAD_DOWN step (gScene.camControl swaps them); CAM_DIST_CROUCH replaces
 * CAM_DIST_DEFAULT while the player crouches.
 * Yaw goal: toward the battle target; from another mode's view (yawFrom);
 * turned by the D-pad's left and right; behind the player; following the
 * player's heading (gScene.camFollow); or swung back after a wall hit. When
 * the orbit view ends, the camera blends back at the setting nearest the
 * orbit eye's height.
 * It then places the eye goal around the player (high while he rides a
 * monster, low on ledges), keeps the eye above the floor and out of walls
 * (AreaGrid_SweepSphere) and monsters (Cam_PushOutOfEms), and the target at
 * most 30 degrees above the eye. After an area change it blends in from
 * CamCtl.out.
 */
void CamFollow_Update(CamCtl *ctl, CamView *view)
{
    Vec3 off;
    Vec3 rotOfs;
    s32 angles[3];
    Vec3 bodyCenter;
    Vec3 hit;
    Vec3 sweepFrom;
    Vec3 pos;
    Vec3 eyeLevelAt;
    Vec3 eyeHit;
    Vec3 probe;
    f32 maxDist;
    f32 mtx[16];
    CamFollow *work;
    Player *pl;
    CamArea *area;
    u16 distKind;
    u8 autoFollowNow;
    u16 prevDistNo;
    s32 i;
    s32 bestNo;
    f32 bestErr;
    f32 factor;
    f32 raiseLimit;
    f32 floorY;
    f32 dx;
    f32 goalDistSq;
    f32 eyeDistSq;
    f32 len;
    f32 goalDx;
    Vec3 toAt;
    f32 farDist;
    f32 curDx;
    f32 curDz;
    f32 curDist;
    f32 dz;
    f32 dxSq;
    f32 dzSq;
    f32 lenSq;
    f32 horiz;
    f32 goalDz;
    s16 turnStep;
    s32 fromMode;
    f32 *fromEye;
    CamDist *setting;
    s32 yawDiff;
    u32 yawGap;
    s32 eyeBlocked;

    if (ctl->areaMode != CAM_MODE_FOLLOW) {
        view->active = 0;
        view->step = 0;
        view->blend = 0;
        return;
    }
    work = &ctl->follow;
    view->active = 1;
    ctl->followView = 1;
    ctl->playerFade = 1.0f;
    pl = ctl->player;
    area = ctl->area;
    CamFollow_UpdateCanFollow(work, pl);
    CamFollow_GetKeys(ctl, work);
    Vec_Copy(work->unk18, work->eye);
    work->fovGoal = work->set->fov;
    if (area->kind != CAM_AREA_FOLLOW) {
        work->set = &gCamFollowDefaults;
    } else {
        work->set = &area->follow;
    }
    if (CamFollow_UpdateFallView(work) == 1) {
        distKind = CAM_DIST_FALL;
        work->dist = &gCamFallDist;
    } else if (work->blockedTimer != 0) {
        distKind = CAM_DIST_BLOCKED;
        work->dist = &gCamBlockedDists[0];
        work->fovGoal = 1.2217306f;
    } else {
        prevDistNo = work->distNo;
        if (gScene.camControl == 0) {
            if ((work->pressed & PAD_DOWN) && work->distNo > 0) {
                work->distNo--;
            }
            if ((work->pressed & PAD_UP) && work->distNo < CAM_DIST_MAX) {
                work->distNo++;
            }
        } else {
            if ((work->pressed & PAD_UP) && work->distNo > 0) {
                work->distNo--;
            }
            if ((work->pressed & PAD_DOWN) && work->distNo < CAM_DIST_MAX) {
                work->distNo++;
            }
        }
        if (prevDistNo != work->distNo) {
            work->unk76 = 8;
        }
        if (pl->action == 1 && work->distNo == CAM_DIST_DEFAULT) {
            distKind = CAM_DIST_CROUCH;
            work->dist = &work->set->dist[CAM_DIST_CROUCH];
        } else {
            work->dist = &work->set->dist[work->distNo];
            distKind = work->distNo;
        }
    }
    autoFollowNow = 0;
    if (ctl->targetObj != NULL) {
        work->wallHit = 0;
        work->yawGoal = pl->targetAngle + ANGLE_TURN / 2;
    } else if ((fromMode = view->yawFrom) != CAM_YAW_FROM_NONE) {
        view->yawFrom = CAM_YAW_FROM_NONE;
        fromEye = ctl->modes[fromMode].eye;
        work->yaw = work->yawGoal = Vec_AngleXZ16(fromEye, fromEye + 3);
    } else if (ctl->prevAreaMode != CAM_MODE_FOLLOW && ctl->prevAreaMode != CAM_MODE_ORBIT) {
        work->yaw = work->yawGoal = Vec_AngleXZ16(ctl->out.eye, ctl->out.at);
    } else if (work->held & (PAD_LEFT | PAD_RIGHT)) {
        if (gScene.camControl != 2) {
            turnStep = 1150;
        } else {
            turnStep = -1150;
        }
        if (work->held & PAD_LEFT) {
            work->yawGoal += turnStep;
        }
        if (work->held & PAD_RIGHT) {
            work->yawGoal -= turnStep;
        }
        work->unk76 = 8;
    } else if (CamFollow_ShouldTurnBehind(pl, work) != 0) {
        work->yawGoal = pl->rot[1] + ANGLE_TURN / 2;
        work->turningBehind = 1;
        work->unk76 = 20;
        autoFollowNow = work->unk78;
    } else if (work->heightState == CAM_HEIGHT_NORMAL && work->canFollow >= 0 && gScene.camFollow == 0) {
        yawDiff = (pl->rot[1] ^ 0x8000) - work->yawGoal & 0xFFFF;
        if (work->canFollow == 0) {
            work->yawGoal = work->yawGoal + Angle_Bump(yawDiff, 4096.0f, 26624.0f, 0.00021701389f);
        } else {
            work->yawGoal = work->yawGoal + Angle_Bump(yawDiff, 0.0f, 26624.0f, 0.00021701389f);
        }
        autoFollowNow = 1;
    } else if (work->held & PAD_L1) {
        if (Player_TestFlag(pl, 8) != 0) {
            work->yawGoal = work->yawGoal + Angle_Bump((pl->rot[1] ^ 0x8000) - work->yawGoal & 0xFFFF, 4096.0f, 26624.0f, 0.00013563369f);
        }
    } else if (work->wallHit != 0) {
        work->yawGoal += (s16)((s16)((pl->rot[1] ^ 0x8000) - work->yawGoal) >> 3);
        work->wallHit = 0;
    }
    work->unk78 = autoFollowNow;
    work->yaw += (s16)((s16)(work->yawGoal - work->yaw) >> 2);
    if (work->turningBehind != 0 && ((yawGap = (u16)(work->yawGoal - work->yaw)) < 0x1000 || yawGap > (ANGLE_TURN * 15 / 16))) {
        work->turningBehind = 0;
    }
    if (ctl->prevAreaMode == CAM_MODE_ORBIT && work->unk78 == 0) {
        CamView_StartBlend(view, gCamOrbitExitBlend);
        Vec_Diff(&off.x, ctl->modes[CAM_MODE_ORBIT].eye, pl->pos);
        setting = &work->set->dist[1];
        bestErr = 1e8f;
        bestNo = 0;
        for (i = 1; i < 4; i++, setting++) {
            factor = setting->eyeHeight - off.y;
            if (bestErr > factor * factor) {
                bestErr = factor * factor;
                bestNo = i;
            }
        }
        work->distNo = bestNo;
        distKind = (u8)bestNo;
        work->dist = &work->set->dist[bestNo];
    }
    off.x = 0.0f;
    if (pl->rideState != 3 && pl->rideState != 0) {
        off.y = 400.0f;
        off.z = 400.0f;
        work->atGoal[1] = 100.0f + pl->pos[1];
    } else if (pl->rideState != 0 || Player_IsState(pl, 0, 17) != 0 ||
               Player_IsState(pl, 0, 27) != 0 || Player_IsState(pl, 0, 30) != 0 ||
               Player_IsState(pl, 0, 58) != 0) {
        off.y = 20.0f;
        off.z = work->dist->eyeBack;
        work->atGoal[1] = 30.0f + pl->pos[1];
    } else {
        off.y = work->dist->eyeHeight;
        off.z = work->dist->eyeBack;
        /* Kept for the match: an empty check of atY. It compiles to nothing but makes MWCC
         * load the add's two operands in the original's order (docs/mwcc-matching.md,
         * "Statements scheduled as if a check were still there"). */
        if (work->dist->atY < 0.0f || work->dist->atY > work->dist->eyeBack) {
        }
        work->atGoal[1] = pl->pos[1] + work->dist->atY;
    }
    work->atGoal[0] = pl->pos[0];
    work->atGoal[2] = pl->pos[2];
    angles[2] = 0;
    angles[0] = 0;
    angles[1] = work->yaw;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(&rotOfs.x, &off.x, mtx);
    work->eyeGoal[0] = work->atGoal[0] + rotOfs.x;
    work->eyeGoal[1] = pl->pos[1] + rotOfs.y;
    work->eyeGoal[2] = work->atGoal[2] + rotOfs.z;
    if (view->step == 0) {
        view->step++;
        Vec_Copy(work->eye, work->eyeGoal);
        Vec_Copy(work->at, work->atGoal);
        work->fov = work->fovGoal;
    }
    work->at[0] = work->atGoal[0];
    work->at[2] = work->atGoal[2];
    work->at[1] += 0.5f * (work->atGoal[1] - work->at[1]);
    if (CamFollow_IsBlownAway() == 1) {
        Vec_Add(&bodyCenter.x, pl->pos, gPlayerBodySpheres);
        if (AreaGrid_SweepSphere(&bodyCenter.x, work->at, &off.x, 4, 33.0f) != 0) {
            Vec_Copy(work->at, &off.x);
        }
    }
    work->fov += 0.05f * (work->fovGoal - work->fov);
    raiseLimit = 100.0f;
    floorY = AreaGrid2_GetFloorY(work->atGoal);
    if (-(floorY - work->floorY) < 210.0f) {
        work->unk54 = 0;
        work->floorY = floorY;
        if (work->unk79 != 0) {
            raiseLimit = 256.0f;
            work->unk79 = 0;
        }
    } else {
        switch (work->unk54) {
        case 0:
            work->unk54 = 1;
        case 1:
            raiseLimit = 256.0f;
            break;
        default:
            work->unk54 = 0;
            work->floorY = floorY;
            break;
        }
        work->unk79 = 0;
    }
    floorY = CamFollow_FloorY(work->eyeGoal, pl);
    if (floorY - work->eyeGoal[1] < raiseLimit) {
        floorY += work->dist->floorGap;
        if (floorY > work->eyeGoal[1]) {
            work->eyeGoal[1] = floorY;
            work->unk79 = 1;
        } else {
            work->unk54 = -1;
        }
    }
    if (work->heightState != CAM_HEIGHT_NORMAL) {
        work->eyeHeld = 0;
        work->eye[1] += 0.25f * (work->eyeGoal[1] - work->eye[1]);
        work->eye[0] = work->eyeGoal[0];
        work->eye[2] = work->eyeGoal[2];
    } else if (work->eyeHeld == 0) {
        eyeBlocked = AreaGrid_SweepSphere(work->eye, work->eyeGoal, &hit.x, CAM_SWEEP_MASK, 28.0f);
        work->wallHit = AreaGrid_SweepSphere(work->at, work->eyeGoal, &hit.x, CAM_SWEEP_MASK, 28.0f);
        if (eyeBlocked != 0 && work->wallHit != 0) {
            work->eye[0] = hit.x;
            work->eye[1] = hit.y;
            work->eye[2] = hit.z;
            work->eyeHeld = 1;
            work->unk76 = 8;
        } else {
            work->eye[1] += 0.125f * (work->eyeGoal[1] - work->eye[1]);
            work->eye[0] = work->eyeGoal[0];
            work->eye[2] = work->eyeGoal[2];
        }
    } else {
        Vec_Copy(&sweepFrom.x, work->at);
        if (work->eyeGoal[1] < sweepFrom.y) {
            sweepFrom.y = 0.5f * (sweepFrom.y + work->eyeGoal[1]);
        }
        if (AreaGrid_SweepSphere(&sweepFrom.x, work->eyeGoal, &hit.x, CAM_SWEEP_MASK, 28.0f) != 0) {
            if (work->unk76 != 0) {
                work->eye[0] = hit.x;
                work->eye[1] = hit.y;
                work->eye[2] = hit.z;
                work->unk18[0] = hit.x;
                work->unk18[1] = hit.y;
                work->unk18[2] = hit.z;
            } else {
                maxDist = work->dist->eyeBack;
                dx = work->eye[0] - work->at[0];
                dz = work->eye[2] - work->at[2];
                eyeDistSq = dx * dx + dz * dz;
                if (eyeDistSq > maxDist * maxDist) {
                    factor = maxDist / Math_Sqrtf(eyeDistSq);
                    pos.x = work->at[0] + dx * factor;
                    pos.y = work->eye[1];
                    pos.z = work->at[2] + dz * factor;
                    work->wallHit = AreaGrid_SweepSphere(work->at, &pos.x, &hit.x, CAM_SWEEP_MASK, 28.0f);
                    if (work->wallHit != 0) {
                        Vec_Copy(work->eye, &hit.x);
                    } else {
                        Vec_Copy(work->eye, &pos.x);
                    }
                }
                if (work->unk78 == 0) {
                    work->yawGoal = Vec_AngleXZ16(work->eye, work->at);
                }
            }
        } else {
            work->eyeHeld = 0;
        }
    }
    if (work->unk76 != 0) {
        work->unk76--;
    }
    if (work->eyeHeld == 0) {
        if (work->heightState == CAM_HEIGHT_NORMAL) {
            Cam_PushOutOfEms(work->eye, work->unk18);
            Cam_LineHitDist(work->eye, work->unk18, &work->hitDist);
        } else {
            eyeLevelAt.x = work->at[0];
            eyeLevelAt.y = work->eye[1];
            eyeLevelAt.z = work->at[2];
            if (AreaGrid_SweepSphere(&eyeLevelAt.x, work->eye, &eyeHit.x, CAM_SWEEP_MASK, 28.0f) != 0) {
                Vec_Copy(work->eye, &eyeHit.x);
                work->yawGoal += 0x400;
            }
        }
    }
    if (AreaGrid_SweepSphere(work->eye, work->eyeGoal, &probe.x, CAM_SWEEP_MASK, 28.0f) != 0) {
        Vec_Diff(&probe.x, work->eye, work->at);
        farDist = 150.0f + work->dist->eyeBack;
        if (probe.x * probe.x + probe.z * probe.z > farDist * farDist) {
            Vec_Copy(work->eye, work->eyeGoal);
            Vec_Copy(work->unk18, work->atGoal);
            Cam_PushOutOfEms(work->eye, work->unk18);
            Cam_LineHitDist(work->eye, work->unk18, &work->hitDist);
        }
    }
    if (work->eyeHeld != 0 && distKind > 0 && distKind < 5) {
        goalDx = work->atGoal[0] - work->eyeGoal[0];
        goalDz = work->atGoal[2] - work->eyeGoal[2];
        goalDistSq = goalDx * goalDx + goalDz * goalDz;
        curDx = work->at[0] - work->eye[0];
        curDz = work->at[2] - work->eye[2];
        curDist = curDx * curDx + curDz * curDz;
        if (goalDistSq > curDist) {
            curDist = Math_Sqrtf(curDist);
            if (curDist > 0.0f) {
                factor = Math_Sqrtf(goalDistSq) / curDist;
                work->at[0] = work->eye[0] + curDx * factor;
                work->at[2] = work->eye[2] + curDz * factor;
            }
        }
    }
    floorY = CamFollow_FloorY(work->eye, pl);
    floorY += work->dist->floorGap;
    if (floorY > work->eye[1]) {
        work->eye[1] = floorY;
    }
    if (work->eye[1] < work->at[1]) {
        Vec_Diff(&toAt.x, work->at, work->eye);
        dxSq = toAt.x * toAt.x;
        dzSq = toAt.z * toAt.z;
        lenSq = dxSq + toAt.y * toAt.y + dzSq;
        if (lenSq > 0.0f) {
            horiz = dxSq + dzSq;
            if (0.75f > horiz / lenSq) {
                horiz = Math_Sqrtf(horiz);
                horiz *= 0.57735f;
                work->at[1] = work->eye[1] + horiz;
            }
        }
    }
    CamView_SetTransition(ctl, view, area);
    CamFollow_UpdateBlocked(ctl, work);
    Vec_Copy(view->eye, work->eye);
    Vec_Copy(view->at, work->at);
    switch (view->blend) {
    case 0:
        view->roll = work->roll;
        view->fov = work->fov;
        break;
    case 1:
        Vec_Copy(view->eye0, ctl->out.eye);
        Vec_Copy(view->at0, ctl->out.at);
        view->roll0 = ctl->out.roll;
        view->fov0 = ctl->out.fov;
        view->blend++;
    case 2:
        factor = CamBlend_EaseStep(view);
        Vec_Lerp(view->eye, view->eye0, view->eye, factor);
        Vec_Lerp(view->at, view->at0, view->at, factor);
        view->roll = view->roll0 * factor + work->roll * (1.0f - factor);
        view->fov = view->fov0 * factor + work->fov * (1.0f - factor);
        break;
    }
    CamView_KeepTargetOnScreen(ctl, view);
}


/*
 * 1 while the follow camera is the view shown and is turning behind the
 * player (turningBehind), else 0.
 */
s32 CamCtl_IsTurningBehind(void)
{
    CamCtl *ctl = (CamCtl *)&gCamCtl;

    if (gCamCtl.override >= 0) {
        return 0;
    }
    if (ctl->areaMode != CAM_MODE_FOLLOW) {
        return 0;
    }
    return ctl->follow.turningBehind != 0;
}

/*
 * When obj is the local player, makes the follow camera turn toward the
 * player's battle target on its next frame (seen: the battle alert).
 */
void CamCtl_SetTargetObj(u8 *obj)
{
    CamCtl *ctl = (CamCtl *)&gCamCtl;

    if (*(u16 *)(obj + 12) == gScene.localPlayer) {
        ctl->targetObj = obj;
    }
}


/*
 * When obj is the local player: turns the follow camera to an eighth of a
 * turn off the player's front (heading + ANGLE_TURN / 8), at the default distance
 * setting, and clears its wall hit (seen: Player_StateSpitRoastStart).
 */
void CamCtl_TurnToPlayerFront(u8 *obj)
{
    if (*(u16 *)(obj + 12) == gScene.localPlayer) {
        CamFollow *work = &gCamCtl.follow;

        work->wallHit = 0;
        work->yawGoal = *(s32 *)(obj + 164) + ANGLE_TURN / 8;
        work->distNo = CAM_DIST_DEFAULT;
    }
}

/*
 * Gives the follow camera's work the camera's pad buttons while the follow
 * camera is current and the script view is off: all of them while the
 * player is free (Cam_PlayerFree), less the D-pad on the mounted gun with L1
 * held; else, unless the keyboard is open, only L1 (L1 and R1 with the bow
 * drawn, outside the hit and gesture groups). Otherwise none.
 */
void CamFollow_GetKeys(CamCtl *ctl, CamFollow *work)
{
    Player *pl;

    if (ctl->areaMode == CAM_MODE_FOLLOW && ctl->modes[CAM_MODE_SCRIPT].active == 0) {
        pl = ctl->player;
        if (Cam_PlayerFree(pl) == 1) {
            work->held = ctl->padHeld;
            work->pressed = ctl->padPressed;
            if (Player_OnMountedGun(pl) == 1 && (pl->input.held & PAD_L1)) {
                work->held &= (PAD_ALL & ~PAD_DPAD);
                work->pressed &= (PAD_ALL & ~PAD_DPAD);
            }
            return;
        }
        if ((s8)Kbd_IsOpen() == 0) {
            if (pl->weaponType == 10 && pl->weaponOut != 0 && pl->stateGroup != 2 && pl->stateGroup != 6) {
                work->held = ctl->padHeld & (PAD_R1 | PAD_L1);
                work->pressed = ctl->padPressed & (PAD_R1 | PAD_L1);
                return;
            }
            work->held = ctl->padHeld & PAD_L1;
            work->pressed = ctl->padPressed & PAD_L1;
            return;        }
    }
    work->pressed = 0;
    work->held = 0;
}



/*
 * Makes the follow camera take its yaw from mode's view on its next update.
 * A negative mode or the script mode gives none (CAM_YAW_FROM_NONE).
 */
void CamFollow_SetYawFrom(s32 mode)
{
    CamView *view = &gCamCtl.modes[CAM_MODE_FOLLOW];

    if (mode == CAM_MODE_SCRIPT) {
        mode = -1;
    }
    if (mode >= 0) {
        view->yawFrom = mode;
    } else {
        view->yawFrom = -1;
    }
}

/*
 * The follow camera's blocked view: starts while the player is blown away
 * (offline) and the sweep from the target goal to the eye goal stops more
 * than 50 short. blockedTimer is set to CAM_BLOCKED_HOLD, held there while
 * he is blown away, then counts down; PAD_UP or PAD_DOWN ends it. distNo
 * becomes 1 and the yaw goal turns to the side the player came from.
 */
void CamFollow_UpdateBlocked(CamCtl *ctl, CamFollow *work)
{
    f32 delta[3];
    Player *pl;

    if (work->blockedTimer == CAM_BLOCKED_HOLD) {
        if (CamFollow_IsBlownAway() == 1) {
            return;
        }
        work->blockedTimer--;
        return;
    }
    if (work->blockedTimer != 0) {
        work->blockedTimer--;
        if (work->pressed & (PAD_UP | PAD_DOWN)) {
            work->blockedTimer = 0;
        }
    }
    if (CamFollow_IsBlownAway() == 1 && AreaGrid_SweepSphere(work->atGoal, work->eyeGoal, delta, CAM_SWEEP_MASK, 28.0f) != 0) {
        delta[0] -= work->eyeGoal[0];
        delta[2] -= work->eyeGoal[2];
        if (delta[0] * delta[0] + delta[2] * delta[2] > 2500.0f) {
            pl = ctl->player;
            work->blockedTimer = CAM_BLOCKED_HOLD;
            work->distNo = 1;
            Vec_Diff(delta, pl->prevPos, pl->pos);
            if (work->unk76 == 0) {
                work->yawGoal = RAD_TO_ANGLE16 * Math_Atan2(delta[0], delta[2]);
            }
        }
    }
}

/*
 * 1 while the follow camera uses its fall setting, else 0. The player's
 * kind each frame: 0 not airborne, 1 airborne, 2 dropping more than 10 a
 * frame, 3 dropping more than 300 above the floor. Four frames of kind 3
 * start the fall view (CAM_HEIGHT_FALL); it holds while the kind is 1 or 3
 * and for 15 frames after. A hit or a status ends it at once.
 */
s32 CamFollow_UpdateFallView(CamFollow *work)
{
    Player *pl = gCamCtl.player;
    u8 kind;
    f32 ground;

    if (pl->stateGroup == 2) {
        work->heightState = CAM_HEIGHT_NORMAL;
        return 0;
    }
    kind = 0;
    if (pl->action == 2) {
        kind = 1;
        if (pl->pos[1] < pl->prevPos[1] - 10.0f) {
            kind = 2;
            ground = AreaGrid2_GetFloorY(pl->pos);
            if (!(pl->pos[1] <= 300.0f + ground)) {
                kind = 3;
            }
        }
    }
    switch (work->heightState) {
    case CAM_HEIGHT_NORMAL:
        if (kind <= 2) {
            work->heightTimer = 4;
            break;
        }
        work->heightTimer -= 1;
        if (work->heightTimer > 0) {
            break;
        }
        work->heightState++;
    case CAM_HEIGHT_FALL:
        if (kind & 1) {
            work->heightTimer = 15;
            return 1;
        }
        work->heightTimer -= 1;
        if (work->heightTimer > 0) {
            return 1;
        }
        work->heightState = CAM_HEIGHT_NORMAL;
        break;
    }
    return 0;
}

/*
 * The floor height under pos; when the player pl is above that floor, the
 * floor found from 150 higher instead.
 */
f32 CamFollow_FloorY(f32 *pos, Player *pl)
{
    f32 probe[3];
    f32 floorY = AreaGrid2_GetFloorY(pos);

    if (pl->pos[1] <= floorY) {
        return floorY;
    }
    Vec_Copy(probe, pos);
    probe[1] += 150.0f;
    return AreaGrid2_GetFloorY(probe);
}

/*
 * Updates canFollow from Cam_PlayerCanFollow (-1: the yaw doesn't follow
 * the player). A change from negative to 0 or 1 waits followDelay frames.
 * Also sets blownAway while offline and the player is blown away.
 */
void CamFollow_UpdateCanFollow(CamFollow *work, Player *pl)
{
    s8 newCanFollow = Cam_PlayerCanFollow(pl);

    if (newCanFollow >= 0) {
        if (work->canFollow < 0) {
            if (pl->weaponType == 10) {
                work->followDelay -= 1;
            } else if (newCanFollow == 1) {
                work->followDelay -= 1;
            }
            if (work->followDelay > 0) {
                newCanFollow = -1;
            }
        }
    } else {
        work->followDelay = 3;
    }
    work->canFollow = newCanFollow;
    work->blownAway = 0;
    if (gScene.inLobby == 0 && Player_InBlownAway(pl) == 1) {
        work->blownAway = 1;
    }
}

s8 CamFollow_GetCanFollow(void)
{
    return gCamCtl.follow.canFollow;
}

s32 CamFollow_IsBlownAway(void)
{
    return gCamCtl.follow.blownAway != 0;
}

/*
 * The orbit camera's init handler: does nothing (CamOrbit_Update sets the
 * work up at step 0).
 */
void CamOrbit_Init(CamCtl *ctl, CamView *view)
{
}

extern f32 gCamOrbitEyeOfs[4];
extern f32 gCamOrbitAimOfs[4];

/*
 * The orbit camera's update handler (CAM_MODE_ORBIT; its work is
 * CamCtl.orbit): an over-the-shoulder view that runs while the follow view
 * is active and follows the player's heading, and then sets areaMode to the
 * orbit. It starts with a 9-frame blend from the follow camera; its pitch
 * follows the player's aimWeight and its yaw eases to a thirty-second of a turn to one side or
 * the other. The eye is placed around a target 170 above the player, kept
 * out of walls and above the floor; the target then moves to where the line
 * from the eye to a point ahead (gCamOrbitAimOfs) crosses the vertical
 * plane through the target. For the match, the reciprocal of the
 * denominator is taken before the numerator, and the floor gap is added in
 * a statement of its own (the original's order).
 */
void CamOrbit_Update(CamCtl *ctl, CamView *view)
{
    f32 pos[3];
    f32 rotOfs[3];
    f32 eyeOfs[3];
    f32 aimOfs[3];
    s32 angles[3];
    f32 hit[3];
    f32 mtx[16];
    f32 line[2][3];
    Player *pl;
    CamOrbit *orbitWork;
    CamFollow *followWork;
    f32 y;
    f32 factor;
    f32 planeX;
    f32 planeZ;
    f32 normalX;
    f32 normalZ;
    f32 normalY;
    f32 planeY;
    f32 dirX;
    f32 dirY;
    f32 dirZ;
    f32 floorY;
    u32 faceDiff;

    if (ctl->modes[CAM_MODE_FOLLOW].active == 0) {
        view->active = 0;
        *(u16 *)&view->step = 0;
        return;
    }
    pl = ctl->player;
    followWork = &ctl->follow;
    orbitWork = &ctl->orbit;
    switch (view->step) {
    case 0:
        if (followWork->unk78 == 0) {
            view->active = 0;
            *(u16 *)&view->step = 0;
            return;
        }
        Vec_Copy(orbitWork->unk18, ctl->modes[CAM_MODE_FOLLOW].eye);
        orbitWork->pitch = (pl->aimWeight + 100) * 34 - 0xB00;
        orbitWork->dir = 2048;
        orbitWork->yaw = 0;
        view->step++;
        view->blend = 1;
        view->left = 9;
        view->rate = 1.0f / view->left;
        break;
    case 1:
        orbitWork->pitch += (s16)((s16)((pl->aimWeight + 100) * 34 - 0xB00 - orbitWork->pitch) >> 2);
        if (followWork->unk78 == 0) {
            view->active = 0;
            *(u16 *)&view->step = 0;
            return;
        }
        break;
    }
    view->active = 1;
    ctl->areaMode = CAM_MODE_ORBIT;
    ctl->followView = 0;
    faceDiff = ((pl->rot[1] ^ 0x8000) - (followWork->yaw + orbitWork->yaw)) & 0xFFFF;
    if (faceDiff > 256 && faceDiff < 30720) {
        orbitWork->dir = -2048;
    } else if (faceDiff > 0x8800 && faceDiff < 0xFF00) {
        orbitWork->dir = 2048;
    }
    if (orbitWork->dir < 0) {
        if (orbitWork->yaw > -0x800) {
            orbitWork->yaw += (s16)((s16)(-(ANGLE_TURN / 32) - orbitWork->yaw) * 6 >> 6);
        }
    } else if (orbitWork->yaw < (ANGLE_TURN / 32)) {
        orbitWork->yaw += (s16)((s16)((ANGLE_TURN / 32) - orbitWork->yaw) * 6 >> 6);
    }
    switch (view->blend) {
    case 0:
        Vec_Copy(eyeOfs, gCamOrbitEyeOfs);
        Vec_Copy(aimOfs, gCamOrbitAimOfs);
        y = 170.0f;
        break;
    case 1:
        (view->blend)++;
    case 2:
        factor = CamBlend_EaseStep(view);
        eyeOfs[0] = 0.0f;
        eyeOfs[1] = 0.0f;
        eyeOfs[2] = followWork->dist->eyeBack;
        Vec_Lerp(eyeOfs, eyeOfs, gCamOrbitEyeOfs, factor);
        aimOfs[2] = 0.0f;
        aimOfs[1] = 0.0f;
        aimOfs[0] = 0.0f;
        Vec_Lerp(aimOfs, aimOfs, gCamOrbitAimOfs, factor);
        y = followWork->dist->atY * factor + 170.0f * (1.0f - factor);
        break;
    }
    orbitWork->at[0] = pos[0] = pl->pos[0];
    orbitWork->at[1] = pos[1] = pl->pos[1] + y;
    orbitWork->at[2] = pos[2] = pl->pos[2];
    angles[0] = orbitWork->pitch;
    angles[1] = followWork->yaw + orbitWork->yaw;
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(rotOfs, eyeOfs, mtx);
    Vec_Add(orbitWork->eye, orbitWork->at, rotOfs);
    if (AreaGrid_SweepSphere(pos, orbitWork->eye, hit, CAM_SWEEP_MASK, 28.0f) != 0) {
        Vec_Copy(orbitWork->eye, hit);
    }
    floorY = CamFollow_FloorY(orbitWork->eye, pl);
    floorY += followWork->dist->floorGap;
    if (floorY > orbitWork->eye[1]) {
        orbitWork->eye[1] = floorY;
    }
    angles[1] = followWork->yaw;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(rotOfs, aimOfs, mtx);
    planeX = orbitWork->at[0];
    planeZ = orbitWork->at[2];
    normalX = rotOfs[0];
    normalZ = rotOfs[2];
    normalY = 0.0f;
    planeY = 0.0f;
    Vec_Copy(line[0], orbitWork->eye);
    Vec_Add(line[1], orbitWork->at, rotOfs);
    dirX = line[1][0] - line[0][0];
    dirY = line[1][1] - line[0][1];
    dirZ = line[1][2] - line[0][2];
    factor = 1.0f / (dirX * normalX + dirY * normalY + dirZ * normalZ);
    factor = (normalX * (planeX - line[1][0]) + normalY * (planeY - line[1][1]) + normalZ * (planeZ - line[1][2])) * factor;
    line[1][0] += dirX * factor;
    line[1][1] += dirY * factor;
    line[1][2] += dirZ * factor;
    Vec_Copy(orbitWork->at, line[1]);
    view->roll = followWork->roll;
    view->fov = followWork->fov;
    Vec_Copy(view->eye, orbitWork->eye);
    Vec_Copy(view->at, orbitWork->at);
}

