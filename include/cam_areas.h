#ifndef CAM_AREAS_H
#define CAM_AREAS_H

#include "common.h"
#include "camera.h"
#include "player.h"

/*
 * The camera's areas and the follow camera's tests of the player.
 * CamArea_TestObj tests an object's position against a camera area's faces;
 * the tests say whether the player is fishing, whether the follow camera
 * follows the player's aim or turns behind the player, and whether the
 * camera may take the whole pad.
 */

void CamCtl_ReadPad(CamCtl *cam);
void Cam_CopyDefaults(CamDistSet *dst);
void CamCtl_LoadParams(CamCtl *cam);
s32 Cam_FindPlayerArea(CamCtl *cam);
s32 CamArea_TestObj(CamArea *area, Player *obj, u8 mask);
s32 Player_IsOnWall(void *obj);
s32 Player_LocalIsOnWall(void);
s32 CamArea_IsEnabled(CamArea *area);
s32 CamFace_ContainsXZ(CamFace *face, f32 *pos);
void CamView_SetTransition(CamCtl *cam, CamView *view, CamArea *area);
f32 CamBlend_EaseStep(CamView *view);
void CamView_StartBlend(CamView *view, s32 frames);
s16 Vec_AngleXZ16(f32 *toPos, f32 *fromPos);
s32 Angle_Bump(u32 angle, f32 rangeLo, f32 rangeHi, f32 amp);
s32 Player_IsFishing(Player *pl);
s32 Cam_PlayerCanFollow(Player *pl);
s32 CamFollow_ShouldTurnBehind(Player *pl, CamFollow *follow);
s32 Cam_PlayerFree(Player *pl);
s32 Player_LocalCanUseBinoculars(void);
extern CamDistSet gCamFollowDefaults;
extern CamDist gCamBlockedDists[2];
extern CamDist gCamFallDist;
extern CamEntFn gCamModeInit[8];
extern CamEntFn gCamModeUpdate[6];

#endif
