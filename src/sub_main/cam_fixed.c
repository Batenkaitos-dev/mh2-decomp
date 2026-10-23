/*
 * The fixed camera, mode 4 of the camera controller (CamCtl gCamCtl;
 * camera.h; its work is CamCtl.fixed): an init and an update handler that
 * CamCtl_Update calls through gCamModeInit and gCamModeUpdate. While the
 * player fishes, the view is the area's fixed one (gAreaCameraIdx,
 * gCamMode4AreaPos). Otherwise the camera focus runs (CamFocus_*; its work
 * is CamFixed.focus): it eases the view toward an object or a point over 20
 * frames and back, and the lobby's code starts and ends it.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "camera.h"
#include "player.h"
#include "model.h"
#include "cam_areas.h"
#include "engine_math.h"
#include "menu_util.h"
#include "vec_math.h"
#include "lobby_npc.h"

extern u8 gAreaCameraIdx[];
extern f32 gCamMode4AreaPos[][6];
extern f32 gCamMode4AreaFovs[];
/* end of prototypes */

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
    if (camIdx == 255) {
        return -1;
    }
    pos = gCamMode4AreaPos[camIdx];
    heading = ANGLE16_TO_RAD * (s32)*(u16 *)(obj + 14);
    view->fov = gCamMode4AreaFovs[camIdx];
    Vec_Copy(ofs, pos);
    Vec_RotateY(ofs, heading);
    Vec_Add(view->eye, (f32 *)(obj + 172), ofs);
    Vec_Copy(ofs, pos + 3);
    Vec_RotateY(ofs, heading);
    Vec_Add(view->at, (f32 *)(obj + 172), ofs);
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
                focusWork->asideTimer = 20;
            }
        }
        return;
    case 1:
        if (focusWork->timer < 20) {
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
                if (focusWork->asideTimer < 20) {
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

