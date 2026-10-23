/*
 * The aim camera, mode 3 of the camera controller (CamCtl gCamCtl;
 * camera.h; its work is CamCtl.aim): an init and an update handler that
 * CamCtl_Update calls through gCamModeInit and gCamModeUpdate. By the
 * player's situation (CamAim_Situation: a bowgun, the binoculars, the
 * mounted gun or the lobby), the view is fixed to the player with the right
 * stick's zoom, turned about the player, or at the position CamAim_Set
 * gives; the eye and target then come from gCamMode3Offsets. The HUD reads
 * its zoom (CamAim_GetState).
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "camera.h"
#include "player.h"
#include "model.h"
#include "engine_math.h"
#include "vec_math.h"
#include "save.h"

s16 Player_IsState(void *obj, u8 state, u8 sub);
/* end of prototypes */

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
            aimWork->unk65 = 17;
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
            if (stickMag > 80) {
                if (aimWork->kind == 0) {
                    zoomFov = &aimWork->fov0;
                } else {
                    zoomFov = &aimWork->fov1;
                }
                if (stickMag > 112) {
                    zoomStep = 0.030543262f;
                } else {
                    zoomStep = stickMag - 80;
                    zoomStep *= 0.00095447694f;
                }
                stickDir = ctl->padDegR;
                if ((u16)(stickDir - 16384) <= 24576) {
                    *zoomFov -= zoomStep;
                    if (*zoomFov < aimWork->fovMin) {
                        *zoomFov = aimWork->fovMin;
                    }
                } else if ((u16)(stickDir - 32768 - 24576) <= 24576) {
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
            Mtx_FromAngles16Pos(aimWork->mtx, pl->aimPitch, (s16)(u16)(pl->rot[1] + ANGLE_TURN / 2), pl->pos);
            view->fov = aimWork->fov1;
            break;
        case 2:
            Mtx_FromAngles16Pos(aimWork->mtx, pl->aimPitch, (s16)(u16)(pl->rot[1] + ANGLE_TURN / 2), pl->pos);
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

/* 1 for the player motions 1002, 1010, 1401, 1404, 1405, 1408, 1411, 1422 and 1423. */
s32 CamAim_IsSpecialMotion(Player *pl)
{
    switch (pl->motionId[0]) {
    case 1002:
    case 1401:
    case 1404:
    case 1411:
    case 1405:
    case 1408:
    case 1010:
    case 1422:
    case 1423:
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

/* A class of the player's situation: 3 in the lobby, 1 in state PL_ACT_BINOCULARS or
 * PL_ACT_BINOCULARS_CROUCHED, 2 when Player_OnMountedGun is 1, 0 when the weapon type
 * (Player.weaponType) is 1 or 5, else -1. */
s32 CamAim_Situation(Player *pl)
{
    if (gScene.inLobby != 0) {
        return 3;
    }
    if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS_CROUCHED) != 0) {
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

