#ifndef CAM_HELPERS_H
#define CAM_HELPERS_H

#include "common.h"
#include "camera.h"
#include "player.h"

/*
 * What the camera controller asks of the player each frame: whether the aim
 * camera serves the player (CamCtl_IsAiming), a segment along the player's
 * body for the check that fades a player too close to the eye
 * (Cam_GetPlayerBodySegment), and the point of the player that the view
 * keeps on the screen (Cam_GetPlayerLowPoint).
 */

void CamCtl_RunMode(CamCtl *cam);
s32 CamCtl_IsRailMode(void);
s32 CamCtl_IsAiming(Player *pl);
s32 CamCtl_GetPlayerFade(f32 *out);
void CamCtl_CheckPlayerInView(void);
void Cam_GetPlayerBodySegment(f32 *seg, Player *pl);
void Frustum_Build(f32 *out, void *mtx, f32 fov, f32 apexScale);
s32 Frustum_ClipSegment(f32 *frustum, f32 *seg);
void CamView_KeepTargetOnScreen(CamCtl *cam, CamView *view);
s32 Cam_GetPlayerLowPoint(f32 *out, Player *pl, CamView *view);

#endif
