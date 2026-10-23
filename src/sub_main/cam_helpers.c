/*
 * What the camera controller (gCamCtl, CamCtl in camera.h) calls each
 * frame:
 * - CamCtl_RunMode hands gCamera the current mode's view, or the view of
 *   a mode that overrides it (aim, fixed, script), with the shakes
 *   (CamShake_Apply).
 * - The player's checks: CamCtl_CheckPlayerInView fades the player when
 *   its body is inside a pyramid in front of the eye (Frustum_Build);
 *   CamView_KeepTargetOnScreen keeps the player's low point on the
 *   screen.
 */

#include "common.h"
#include "camera.h"
#include "em.h"
#include "player.h"
#include "objhead.h"
#include "cam_helpers.h"
#include "engine_math.h"
#include "vec_math.h"

/*
 * Ticks the three shake requests (an active one ends when its length
 * runs out), copies the current mode's view (modes[areaMode]) into
 * cam->out, and sets cam->override to the first active view of the
 * script, aim and fixed modes (5, 3, 4), else -1. With an override,
 * gCamera takes that view's eye, target, roll and fov, plays shakeC for
 * the script view and shakeB for the aim view (CamShake_Apply), and
 * followView is cleared; else gCamera takes cam->out and plays shakeA.
 * Then hands the override to CamFollow_SetYawFrom.
 */
void CamCtl_RunMode(CamCtl *cam)
{
    CamView *view;
    CamShake *shake;
    s32 i;

    shake = &cam->shakeA;
    i = 3;
    do {
        if (shake->active != 0) {
            if (--shake->length <= 0) {
                shake->active = 0;
            }
        }
        i--;
        shake++;
    } while (i > 0);
    view = &cam->modes[cam->areaMode];
    Vec_Copy(cam->out.eye, view->eye);
    Vec_Copy(cam->out.at, view->at);
    cam->out.roll = view->roll;
    cam->out.fov = view->fov;
    cam->override = CAM_OVERRIDE_NONE;
    if (cam->modes[CAM_MODE_SCRIPT].active != 0) {
        cam->override = CAM_MODE_SCRIPT;
    } else if (cam->modes[CAM_MODE_AIM].active != 0) {
        cam->override = CAM_MODE_AIM;
    } else if (cam->modes[CAM_MODE_FIXED].active != 0) {
        cam->override = CAM_MODE_FIXED;
    }
    if (cam->override >= 0) {
        view = &cam->modes[cam->override];
        Vec_Copy(gCamera->eye, view->eye);
        Vec_Copy(gCamera->at, view->at);
        if (cam->override == CAM_MODE_SCRIPT) {
            CamShake_Apply(&cam->shakeC);
        } else if (cam->override == CAM_MODE_AIM) {
            CamShake_Apply(&cam->shakeB);
        }
        Camera_SetRoll(view->roll);
        Camera_StoreFov(view->fov);
        cam->followView = 0;
    } else {
        Vec_Copy(gCamera->eye, cam->out.eye);
        Vec_Copy(gCamera->at, cam->out.at);
        CamShake_Apply(&cam->shakeA);
        Camera_SetRoll(cam->out.roll);
        Camera_StoreFov(cam->out.fov);
    }
    CamFollow_SetYawFrom(cam->override);
}

/* 1 when the current mode (gCamCtl.areaMode) is CAM_MODE_RAIL. */
s32 CamCtl_IsRailMode(void)
{
    return gCamCtl.areaMode == CAM_MODE_RAIL;
}

/*
 * Whether the aim camera serves player pl: 0 while the script view is
 * active; 1 while pl->aimLockTimer runs; else whether the aim view
 * (modes[CAM_MODE_AIM]) is active. Reads gCamCtl both by name and
 * through cam.
 */
s32 CamCtl_IsAiming(Player *pl)
{
    CamCtl *cam = (CamCtl *)&gCamCtl;

    if (gCamCtl.modes[CAM_MODE_SCRIPT].active != 0) {
        return 0;
    }
    if (pl->aimLockTimer != 0) {
        return 1;
    }
    return cam->modes[CAM_MODE_AIM].active != 0;
}

/*
 * While followView is set (the follow camera shows the view), sets out
 * to gCamCtl.playerFade and returns 1; else sets out to 1.0, returns 0.
 */
s32 CamCtl_GetPlayerFade(f32 *out)
{
    CamCtl *cam = (CamCtl *)&gCamCtl;

    if (gCamCtl.followView == 0) {
        *out = 1.0f;
        return 0;
    }
    *out = cam->playerFade;
    return 1;
}

extern f32 gCamMatrix[];

/*
 * Run by CamCtl_Update while followView is set. Builds the view pyramid
 * of gCamMatrix and gCamera->fov (Frustum_Build with apexScale 45, 50
 * while playerInView is set) and clips to it the segment
 * Cam_GetPlayerBodySegment gives for gCamCtl.player (the local player,
 * CamCtl_Init). When some of it is inside, playerFade =
 * 0.3 (the player is drawn faded) and playerInView = 1; else playerInView = 0.
 */
void CamCtl_CheckPlayerInView(void)
{
    CamCtl *cam = &gCamCtl;
    f32 frustum[16];
    f32 seg[6];

    Frustum_Build(frustum, gCamMatrix, gCamera->fov, gCamCtl.playerInView == 0 ? 45.0f : 50.0f);
    Cam_GetPlayerBodySegment(seg, cam->player);
    if (Frustum_ClipSegment(frustum, seg) == 1) {
        cam->playerFade = 0.3f;
        cam->playerInView = 1;
    } else {
        cam->playerInView = 0;
    }
}

/*
 * A segment along the body of player pl (CamCtl_CheckPlayerInView
 * passes gCamCtl.player) into seg: seg[0..2] =
 * 2 bone10 - bone2 (bone 10's position moved on by the distance between
 * the two), seg[3..5] = bone 2's position (each bone's world
 * translation, Em_GetBone). The second Vec_Lerp (weight 1) leaves
 * seg[3..5] as it is.
 */
void Cam_GetPlayerBodySegment(f32 *seg, Player *pl)
{
    f32 *segEnd;

    Vec_Copy(seg, Em_GetBone(pl, 10)->world + 12);
    Vec_Copy(seg + 3, Em_GetBone(pl, 2)->world + 12);
    Vec_Lerp(seg, seg, seg + 3, 2.0f);
    segEnd = seg + 3;
    Vec_Lerp(segEnd, segEnd, seg, 1.0f);
}

/*
 * The view pyramid of matrix mtx for the field of view fov (radians)
 * into out: out[0..2] its apex, (0, 0, -1.25 apexScale / tan(fov / 2))
 * through mtx (Vec4_TransformSelf); out[3..14] the unit normals of its
 * four sides, two in x and two in y for a 640 x 480 screen (screenDist
 * = 320 / tan(fov / 2) against the half sizes 320 and 240), turned by
 * mtx. A point is inside a side when the side's normal has a non-negative
 * dot with apex - point (Frustum_ClipSegment).
 */
void Frustum_Build(f32 *out, void *mtx, f32 fov, f32 apexScale)
{
    f32 sinHalf;
    f32 cosHalf;
    f32 apex[4];
    f32 cotHalf;
    f32 screenDist;
    f32 halfW = 320.0f;
    f32 halfH = 240.0f;

    Math_GetSinCos(&sinHalf, &cosHalf, 0.5f * fov);
    cotHalf = cosHalf / sinHalf;
    screenDist = halfW * cotHalf;
    apex[0] = apex[1] = 0.0f;
    apex[2] = -(1.25f * apexScale * cotHalf);
    apex[3] = 1.0f;
    Vec4_TransformSelf(apex, mtx);
    out[0] = apex[0];
    out[1] = apex[1];
    out[2] = apex[2];
    out[3] = screenDist;
    out[4] = 0.0f;
    out[5] = -halfW;
    out[6] = -screenDist;
    out[7] = 0.0f;
    out[8] = -halfW;
    out[9] = 0.0f;
    out[10] = screenDist;
    out[11] = -halfH;
    out[12] = 0.0f;
    out[13] = -screenDist;
    out[14] = -halfH;
    Vec_RotateSelf(out + 3, (f32 *)mtx);
    Vec_RotateSelf(out + 6, (f32 *)mtx);
    Vec_RotateSelf(out + 9, (f32 *)mtx);
    Vec_RotateSelf(out + 12, (f32 *)mtx);
    Vec_Normalize(out + 3);
    Vec_Normalize(out + 6);
    Vec_Normalize(out + 9);
    Vec_Normalize(out + 12);
}

/*
 * Clips the segment seg (seg[0..2] to seg[3..5]) to the pyramid frustum
 * (Frustum_Build), one side at a time: returns 0 as soon as both ends are
 * outside the same side; an end outside a side is moved onto it. Returns
 * 1 when some of the segment is left.
 */
s32 Frustum_ClipSegment(f32 *frustum, f32 *seg)
{
    f32 *normal = frustum + 3;
    s32 i;

    for (i = 4; i != 0; i--) {
        s32 inside = 0;
        f32 distStart = normal[0] * (frustum[0] - seg[0]) + normal[1] * (frustum[1] - seg[1]) + normal[2] * (frustum[2] - seg[2]);
        f32 distEnd;
        f32 dx;
        f32 dy;
        f32 inv;
        f32 dz;

        if (distStart >= 0.0f) {
            inside |= 1;
        }
        distEnd = normal[0] * (frustum[0] - seg[3]) + normal[1] * (frustum[1] - seg[4]) + normal[2] * (frustum[2] - seg[5]);
        if (distEnd >= 0.0f) {
            inside |= 2;
        }
        switch (inside) {
        case 0:
            return 0;
        case 1:
            dx = seg[3] - seg[0];
            dy = seg[4] - seg[1];
            dz = seg[5] - seg[2];
            inv = 1.0f / (dx * normal[0] + dy * normal[1] + dz * normal[2]);
            seg[3] += inv * (dx * distEnd);
            seg[4] += inv * (dy * distEnd);
            seg[5] += inv * (dz * distEnd);
            break;
        case 2:
            dx = seg[0] - seg[3];
            dy = seg[1] - seg[4];
            dz = seg[2] - seg[5];
            inv = 1.0f / (dx * normal[0] + dy * normal[1] + dz * normal[2]);
            seg[0] += inv * (dx * distStart);
            seg[1] += inv * (dy * distStart);
            seg[2] += inv * (dz * distStart);
            break;
        }
        normal += 3;
    }
    return 1;
}

/*
 * Sets view->dir = at - eye. When Cam_GetPlayerLowPoint finds a point of
 * cam->player below the eye and in front of the view, turns the view
 * down so the point is not below the screen's bottom edge: screenDist =
 * 224 / tan(fov / 2) (fov as the vertical angle, 448 lines); edgeDir
 * (0, -224, -screenDist) is the bottom edge's direction and edgeNormal
 * (0, -screenDist, 224) its side's normal, both turned into the world by
 * the inverse of the look matrix (up from the roll, Camera_ComputeUp).
 * When the point lies beyond the edge (beyond, along edgeNormal, > 0),
 * dir is turned about edgeDir x edgeNormal by atan2(beyond, along) and
 * at = eye + the turned dir (view->dir keeps the old direction).
 */
void CamView_KeepTargetOnScreen(CamCtl *cam, CamView *view)
{
    f32 look[16];
    f32 inv[16];
    f32 target[3];
    f32 eye[3];
    f32 edgeNormal[3];
    f32 edgeDir[3];
    f32 axis[3];
    f32 toTarget[3];
    f32 sinHalf;
    f32 cosHalf;
    f32 screenDist;
    f32 beyond;
    f32 along;

    Vec_Diff(view->dir, view->at, view->eye);
    if (Cam_GetPlayerLowPoint(target, cam->player, view) == 1) {
        Camera_ComputeUp(view->up, view->eye, view->at, view->roll);
        Mtx_LookAt(look, view->eye, view->at, view->up);
        Mtx_Inverse(inv, look);
        Math_GetSinCos(&sinHalf, &cosHalf, 0.5f * view->fov);
        screenDist = 224.0f * (cosHalf / sinHalf);
        Vec_Copy(eye, view->eye);
        edgeNormal[0] = 0.0f;
        edgeNormal[1] = -screenDist;
        edgeNormal[2] = 224.0f;
        Vec_RotateSelf(edgeNormal, (f32 *)inv);
        Vec_Normalize(edgeNormal);
        edgeDir[0] = 0.0f;
        edgeDir[1] = -224.0f;
        edgeDir[2] = -screenDist;
        Vec_RotateSelf(edgeDir, (f32 *)inv);
        Vec_Normalize(edgeDir);
        Vec_Diff(toTarget, target, eye);
        beyond = Vec_Dot(toTarget, edgeNormal);
        if (!(beyond <= 0.0f)) {
            along = Vec_Dot(toTarget, edgeDir);
            Vec_Cross(axis, edgeDir, edgeNormal);
            Mtx_Identity(look);
            Mtx_FromAxisAngle(look, axis, Math_Atan2(beyond, along));
            Vec_Rotate(axis, view->dir, look);
            Vec_Add(view->at, view->eye, axis);
        }
    }
}

/*
 * The position of player pl's bone 20 (seen: the head, as in Chara_SetCamera's
 * face view) into out. Returns 0 when it is not below the view's eye;
 * 1 when it is below and in front (its offset from the eye has a
 * positive dot with view->dir). Below but behind: out becomes bone 2's
 * position and the result is whether that one is in front.
 */
s32 Cam_GetPlayerLowPoint(f32 *out, Player *pl, CamView *view)
{
    f32 toPoint[3];

    Vec_Copy(out, Em_GetBone(pl, 20)->world + 12);
    if (!(out[1] < view->eye[1])) {
        return 0;
    }
    Vec_Diff(toPoint, out, view->eye);
    if (!(Vec_Dot(view->dir, toPoint) <= 0.0f)) {
        return 1;
    }
    Vec_Copy(out, Em_GetBone(pl, 2)->world + 12);
    Vec_Diff(toPoint, out, view->eye);
    return Vec_Dot(view->dir, toPoint) > 0.0f;
}
