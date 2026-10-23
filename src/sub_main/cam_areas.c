/*
 * The camera's data and areas, and the follow camera's tests:
 * - the pad as the camera reads it (CamCtl_PadAllowed, CamCtl_ReadPad);
 * - the camera data: an area's camera file or the defaults
 *   (CamCtl_InitDefaultParams, CamCtl_LoadParams);
 * - the camera area the player is in, found through the grid's lists
 *   (Cam_FindPlayerArea, CamArea_TestObj, CamFace_ContainsXZ), and the
 *   blend of the views between areas (CamView_SetTransition,
 *   CamBlend_EaseStep);
 * - the follow camera's tests of the player (fishing, on a wall, free to
 *   follow, the turn behind the player, the binoculars).
 * The data: the follow camera's defaults, the modes' tables (their
 * offsets, area positions and handlers) and the orbit camera's offsets.
 */

#include "common.h"
#include "syssettings.h"
#include "quest.h"
#include "camera.h"
#include "em.h"
#include "pad.h"
#include "player.h"
#include "objhead.h"
#include "area.h"
#include "hud.h"
#include "cam_areas.h"
#include "engine_math.h"
#include "game_util.h"

/*
 * 1 when the camera may read the pad. 0 offline once the quest is over
 * (Quest_PhaseIsOver(1)), while the followed player's tradeMsgTimer
 * runs, or while Hud_InputBlocked is 1. cam is the CamCtl.
 */
s32 CamCtl_PadAllowed(CamCtl *cam)
{
    if ((gScene.inLobby == 0) && (Quest_PhaseIsOver(1) == 1)) {
        return 0;
    }
    if (cam->player->tradeMsgTimer != 0) {
        return 0;
    }
    return Hud_InputBlocked() != 1;
}

/*
 * Copies gPadState[0]'s held, pressed, dir, degR and magR into the
 * controller's padHeld, padPressed, padDir, padDegR and padMagR when
 * CamCtl_PadAllowed, else zeroes them.
 */
void CamCtl_ReadPad(CamCtl *cam)
{
    if (CamCtl_PadAllowed(cam) == 0) {
        cam->padDir = 0U;
        cam->padPressed = 0U;
        cam->padHeld = 0U;
        cam->padMagR = 0U;
        cam->padDegR = 0U;
        return;
    }
    cam->padHeld = (u16) gPadState->held;
    cam->padPressed = (u16) gPadState->pressed;
    cam->padDir = (u16) gPadState->dir;
    cam->padDegR = (u16) gPadState->degR;
    cam->padMagR = (u16) gPadState->magR;
}

/* dst = the default follow settings gCamFollowDefaults. */
void Cam_CopyDefaults(CamDistSet *dst)
{
    *dst = gCamFollowDefaults;
}

/*
 * Writes a default camera data into gSysSettings.camData and makes it
 * cam->data: version 0x102, areaCount 0, a 1 x 1 grid without cells or
 * area lists, unk08, unk0A, unk14, unk18 20000 and unk0C, unk10 0, and
 * one area right after the header (areas): id 0, kind CAM_AREA_FOLLOW,
 * lookLock 2 (no axis kept), lookSpace 0, no faces, the fov scale 1.0 at
 * distance 400 to 0.75 at 2400, no transition table, the follow settings
 * of gCamFollowDefaults. Without cells Cam_FindPlayerArea always gives
 * this area.
 */
void CamCtl_InitDefaultParams(CamCtl *cam)
{
    CamArea *area;
    CamDataHdr *hdr;

    hdr = (CamDataHdr *)gSysSettings.camData;
    hdr->version = CAM_DATA_VERSION;
    area = (CamArea *)(hdr + 1);
    hdr->areaCount = 0;
    hdr->gridW = 1;
    hdr->gridH = 1;
    hdr->unk08 = 20000;
    hdr->unk0A = 20000;
    hdr->unk0C = 0;
    hdr->unk10 = 0;
    hdr->unk14 = 20000;
    hdr->unk18 = 20000;
    hdr->cells = NULL;
    hdr->areaLists = NULL;
    hdr->unk24 = 0;
    hdr->areas = area;
    area->unk00 = 0;
    area->id = 0;
    area->kind = CAM_AREA_FOLLOW;
    area->lookLock = CAM_LOOK_LOCK_NONE;
    area->lookSpace = CAM_LOOK_SPACE_WORLD;
    area->faceCount = 0;
    area->fovDist[0] = 400.0f;
    area->fovDist[1] = 2400.0f;
    area->fovScale[0] = 1.0f;
    area->fovScale[1] = 0.75f;
    area->faces = NULL;
    area->trans = NULL;
    Cam_CopyDefaults(&area->follow);
    cam->data = hdr;
}

/*
 * Takes the grid parameters from the camera data (the default one first
 * when cam->data is NULL): the header's gridW, gridH and unk08 to unk18
 * into cam->gridW, gridH and unk394 to unk3A4.
 */
void CamCtl_LoadParams(CamCtl *cam)
{
    CamDataHdr *hdr;

    if (cam->data == NULL) {
        CamCtl_InitDefaultParams(cam);
    }
    hdr = cam->data;
    cam->gridW = hdr->gridW;
    cam->gridH = hdr->gridH;
    cam->unk394 = hdr->unk08;
    cam->unk396 = hdr->unk0A;
    cam->unk398 = hdr->unk0C;
    cam->unk39C = hdr->unk10;
    cam->unk3A0 = hdr->unk14;
    cam->unk3A4 = hdr->unk18;
}

/*
 * Looks up the camera area the local player is in. cam->area becomes the
 * data's first area; then, unless keepFirstArea is set, the first area listed
 * in grid cell (cellX, cellZ) that CamArea_TestObj puts the player in
 * (faces with CAM_FACE_NO_ENTER skipped). Returns 1 when one was found, 0 when
 * not (the first area stays), -1 without camera data (area NULL).
 */
s32 Cam_FindPlayerArea(CamCtl *cam)
{
    CamCell *grid;
    CamCell *cell;
    CamArea **list;
    s32 left;
    Player *pl = &gPlayers[gScene.localPlayer];
    CamDataHdr *map = cam->data;

    if (map == NULL) {
        cam->area = NULL;
        return -1;
    }
    cam->area = map->areas;
    if (cam->keepFirstArea != 0) {
        return 0;
    }
    grid = map->cells;
    if (grid == NULL) {
        return 0;
    }
    cell = &grid[cam->cellX + cam->gridW * cam->cellZ];
    left = cell->count;
    if (left == 0) {
        return 0;
    }
    list = cell->areas;
    while (left != 0) {
        if (CamArea_TestObj(*list, pl, CAM_FACE_NO_ENTER) == 0) {
            cam->area = *list;
            return 1;
        }
        left--;
        list++;
    }
    return 0;
}

/*
 * 0 when obj's pos is in front of one of area's faces whose flags share
 * no bit with mask, no farther than the face's depth, and
 * CamFace_ContainsXZ accepts it; 2 otherwise, and when the area is not
 * enabled (CamArea_IsEnabled). Masks seen: CAM_FACE_NO_ENTER looking for an area
 * (Cam_FindPlayerArea), CAM_FACE_NO_STAY testing the current one (CamCtl_Update). mask
 * is copied into a local of its own for the match: used directly, the
 * compiler loads face before the count test, where the game's code loads
 * it in the branch's delay slot.
 */
s32 CamArea_TestObj(CamArea *area, Player *obj, u8 mask)
{
    f32 toObj[3];
    f32 dist;
    CamFace *face;
    s32 left;
    s32 skipMask;
    f32 *dy;
    f32 *dz;

    if (CamArea_IsEnabled(area) == 0) {
        return 2;
    }
    left = area->faceCount;
    face = area->faces;
    if (left != 0) {
        dy = &toObj[1];
        dz = &toObj[2];
        skipMask = mask;
        do {
            if (!(face->flags & skipMask)) {
                toObj[0] = obj->pos[0] - face->pos[0];
                *dy = obj->pos[1] - face->pos[1];
                *dz = obj->pos[2] - face->pos[2];
                dist = Vec_Dot(face->normal, toObj);
                if (!(dist < 0.0f) && dist <= face->depth && CamFace_ContainsXZ(face, obj->pos) == 0) {
                    return 0;
                }
            }
            left--;
            face++;
        } while (left != 0);
    }
    return 2;
}

/*
 * 1 when obj (a player; AreaHit_CollideCapsule passes its object, read
 * through the header players and monsters share) is on a climbable wall:
 * stateGroup PL_GROUP_ACTION and state PL_ACT_WALL_HOLD to
 * PL_ACT_WALL_GRAB, or PL_ACT_WALL_CLIMB_UP_FAST or
 * PL_ACT_WALL_CLIMB_DOWN_FAST; else 0. The group is tested twice and the
 * state read twice, as the game's code does. AreaHit_CollideCapsule
 * uses mask 0x7FFF while on a wall.
 */
s32 Player_IsOnWall(void *obj)
{
    ObjHead *head = obj;
    u8 group;
    u8 state;
    u8 state2;

    group = head->stateGroup;
    if (group == PL_GROUP_ACTION) {
        state = head->state;
        if (((s32) state >= PL_ACT_WALL_HOLD) && ((s32) state <= PL_ACT_WALL_GRAB)) {
            return 1;
        }
    }
    if (group == PL_GROUP_ACTION) {
        state2 = head->state;
        if (((s32) state2 >= PL_ACT_WALL_CLIMB_UP_FAST) && ((s32) state2 <= PL_ACT_WALL_CLIMB_DOWN_FAST)) {
            return 1;
        }
    }
    return 0;
}

/* Player_IsOnWall for the local player. */
s32 Player_LocalIsOnWall(void)
{
    return Player_IsOnWall(&gPlayers[gScene.localPlayer]);
}

/*
 * 0 when the area has CAM_AREA_ON_WALL (it counts only while the local player
 * is on a wall) and the local player is not on one; else 1.
 */
s32 CamArea_IsEnabled(CamArea *area)
{
    if ((area->flags & CAM_AREA_ON_WALL) && (Player_LocalIsOnWall() == 0)) {
        return 0;
    }
    return 1;
}

/*
 * a b - c d: the 2D cross product CamFace_ContainsXZ tests its edges
 * with (edgeZ dx - dz edgeX). An inline helper, as the game's code used
 * one: plain C orders the multiply-add's operands the other way.
 */
static inline f32 Cross2(f32 edgeZ, f32 dx, f32 dz, f32 edgeX)
{
    return edgeZ * dx - dz * edgeX;
}

/*
 * 0 when pos (x pos[0], z pos[2]) is on the inner side of the face's
 * four edges in the x-z plane: from corner (quad[0], quad[2]) to
 * (quad[4], quad[6]) and to (quad[5], quad[7]), from corner (quad[1],
 * quad[3]) to (quad[4], quad[6]) and to (quad[5], quad[7]); 2 when it is
 * outside one.
 */
s32 CamFace_ContainsXZ(CamFace *face, f32 *pos)
{
    f32 *corners = face->quad;
    f32 dx;
    f32 dz;

    dx = pos[0] - corners[0];
    dz = pos[2] - corners[2];
    if (Cross2(corners[2] - corners[6], dx, dz, corners[0] - corners[4]) < 0.0f) {
        return 2;
    }
    if (Cross2(corners[7] - corners[2], dx, dz, corners[5] - corners[0]) < 0.0f) {
        return 2;
    }
    dx = pos[0] - corners[1];
    dz = pos[2] - corners[3];
    if (Cross2(corners[6] - corners[3], dx, dz, corners[4] - corners[1]) < 0.0f) {
        return 2;
    }
    if (Cross2(corners[3] - corners[7], dx, dz, corners[1] - corners[5]) < 0.0f) {
        return 2;
    }
    return 0;
}

/*
 * When cam->areaChanged is set, starts or stops view's blend into the new
 * area: frames is what area's transition table (area->trans, 16
 * {area id, frames} pairs) gives for cam->prevAreaId, 15 without an
 * area, a table or a matching pair; 0 or 1 frames clear view->blend (no
 * blend), more start one (CamView_StartBlend).
 */
void CamView_SetTransition(CamCtl *cam, CamView *view, CamArea *area)
{
    s32 frames;
    CamTrans *entry;
    s32 i;

    if (cam->areaChanged != 0) {
        frames = CAM_TRANS_DEFAULT;
        if (area != NULL) {
            entry = area->trans;
            if (entry != NULL) {
                for (i = CAM_TRANS_COUNT; i != 0; i--, entry++) {
                    if (entry->areaId == cam->prevAreaId) {
                        frames = entry->frames;
                        break;
                    }
                }
            }
        }
        switch (frames) {
        default:
            CamView_StartBlend(view, frames);
            break;
        case 0:
        case 1:
            view->blend = CAM_BLEND_NONE;
            break;
        }
    }
}

/*
 * One frame of view's blend. Returns the weight of the blend's start
 * values (eye0, at0, roll0, fov0: the callers' Vec_Lerp): the quadratic
 * ease in and out of t = left times rate (left / the blend's length;
 * 2 t^2 below 0.5, 1 - 2 (1 - t)^2 above), so from near 1 down to 0.
 * Then counts left down and clears blend when it reaches 0.
 */
f32 CamBlend_EaseStep(CamView *view)
{
    f32 leftFrac;
    f32 ease;

    leftFrac = (f32) view->left * view->rate;
    if (leftFrac < 0.5f) {
        ease = 0.5f * Math_Pow(2.0f * leftFrac, 2.0f);
    } else {
        ease = 1.0f - (0.5f * Math_Pow(2.0f * (1.0f - leftFrac), 2.0f));
    }
    view->left = (s16) (view->left - 1);
    if (view->left == 0) {
        view->blend = CAM_BLEND_NONE;
    }
    return ease;
}

/*
 * Starts a blend of view over frames frames: blend = 1 (the caller then
 * keeps the start values), left = frames - 1, rate = 1 / frames.
 */
void CamView_StartBlend(CamView *view, s32 frames)
{
    view->blend = CAM_BLEND_START;
    view->left = frames - 1;
    view->rate = 1.0f / frames;
}

/*
 * The angle (ANGLE_TURN to a turn) of the direction from fromPos to toPos in
 * the x-z plane: RAD_TO_ANGLE16 * Math_Atan2(dx, dz). Seen: the follow
 * camera's yaw from an eye to its target (CamFollow_Update).
 */
s16 Vec_AngleXZ16(f32 *toPos, f32 *fromPos)
{
    return (u16)(s32)(RAD_TO_ANGLE16 * Math_Atan2(toPos[0] - fromPos[0], toPos[2] - fromPos[2]));
}

/*
 * A bump over the 16-bit angle `angle` (ANGLE_TURN to a turn): 0 up to
 * rangeLo; amp * (angle - rangeLo)^2 * (rangeHi - angle)^2 / 2^32 between
 * rangeLo and rangeHi; 0 from rangeHi to ANGLE_TURN - rangeHi; the same
 * bump mirrored and negated from ANGLE_TURN - rangeHi to ANGLE_TURN - rangeLo;
 * 0 above. Returns it truncated to an integer. Seen: CamFollow_Update
 * adds it to yawGoal, angle being the gap from yawGoal to behind the
 * player, so the view eases round behind the player.
 */
s32 Angle_Bump(u32 angle, f32 rangeLo, f32 rangeHi, f32 amp)
{
    f32 gapHi;
    f32 gapLo;

    if (angle <= rangeLo) {
        return 0;
    }
    if (!(angle < rangeHi)) {
        if (angle <= 65536.0f - rangeHi) {
            return 0;
        }
        if (angle < 65536.0f - rangeLo) {
            angle = ANGLE_TURN - angle;
            amp = -amp;
        } else {
            return 0;
        }
    }
    gapHi = (s32)angle;
    gapLo = gapHi - rangeLo;
    gapHi = rangeHi - gapHi;
    gapLo = 0.000015258789f * (gapLo * gapLo);
    gapHi = 0.000015258789f * (gapHi * gapHi);
    return amp * (gapLo * gapHi);
}

/*
 * 1 when player pl has the fishing flag (PLF_FISHING, Player_TestFlag), else 0. Only the
 * fishing states set it (Player_StateFishCast, Player_StateFishWait,
 * Player_StateFishReel, Player_StateFishReelEmpty); the reactions, the
 * faint, the leave and the quest abort clear it. Seen: CAM_MODE_FIXED's
 * fixed view at the player's action spot runs while it is set
 * (CamFixed_UpdateTarget), and it keeps the camera's keys from the pad
 * (Cam_PlayerFree).
 */
s32 Player_IsFishing(Player *pl)
{
    return Player_TestFlag(pl, 0x80000U) != 0;
}

s16 Player_IsState(void *obj, u8 state, u8 sub);

/*
 * Whether the follow camera's yaw follows player pl's aim. -1 while the
 * player is blown away offline (CamFollow_IsBlownAway) and whenever none
 * of these holds; with the weapon drawn (weaponOut):
 * - a bowgun (WEAPON_HEAVY_BOWGUN, WEAPON_LIGHT_BOWGUN) with PAD_L1 held
 *   in the player's input: 1 in PL_ACT_IDLE or the weapon states
 *   BOWGUN_FIRE_RECOIL_0, _1 and _2, BOWGUN_RELOAD, BOWGUN_DRAW_RELOAD
 *   and BOWGUN_DRY_FIRE, else 0;
 * - the bow (WEAPON_BOW) with PAD_R1 held: 1 in the weapon states
 *   BOW_CHARGE, BOW_CHARGE_RESUME, BOW_CHARGE_FROM_IDLE,
 *   BOW_CHARGE_AFTER_SHOT, BOW_AIM, BOW_AIM_RESUME, BOW_FIRE,
 *   BOW_FIRE_QUICK, BOW_EVADE and BOW_EVADE_2, else 0.
 * Seen: CamFollow_UpdateCanFollow keeps it in CamFollow.canFollow; the
 * yaw then eases behind the player when it is less than 26624 off, at
 * 1 from any gap, at 0 only past 4096, a sixteenth of a turn (Angle_Bump).
 */
s32 Cam_PlayerCanFollow(Player *pl)
{
    s32 inState;

    if (CamFollow_IsBlownAway() == 1) {
        return -1;
    }
    if (pl->weaponOut != 0) {
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
            if (pl->input.held & PAD_L1) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_IDLE) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_FIRE_RECOIL_0) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_FIRE_RECOIL_1) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_FIRE_RECOIL_2) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_RELOAD) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_DRAW_RELOAD) != 0) {
                    return 1;
                }
                inState = Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_DRY_FIRE) != 0;
                return inState;
            }
        } else if (pl->weaponType == WEAPON_BOW) {
            if (pl->input.held & PAD_R1) {
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_CHARGE) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_CHARGE_RESUME) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_CHARGE_FROM_IDLE) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_CHARGE_AFTER_SHOT) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_AIM) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_AIM_RESUME) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_FIRE) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_FIRE_QUICK) != 0) {
                    return 1;
                }
                if (Player_IsState(pl, PL_GROUP_WEAPON, BOW_EVADE) != 0) {
                    return 1;
                }
                inState = Player_IsState(pl, PL_GROUP_WEAPON, BOW_EVADE_2) != 0;
                return inState;
            }
        }
    }
    return -1;
}

/*
 * 1 when the follow camera should turn behind player pl, else 0: while
 * the local player is on a climbing wall (Player_LocalIsOnWall)
 * and the eye is held at a wall (follow->eyeHeld); when the camera's keys
 * (follow->pressed, from CamFollow_GetKeys) have PAD_L1 newly pressed;
 * or with the bow drawn (WEAPON_BOW, weaponOut) when PAD_R1 is newly
 * pressed for the camera and held in the player's input, outside
 * PL_GROUP_REACT and PL_GROUP_GESTURE. Seen: CamFollow_Update then sets
 * yawGoal behind the player and turningBehind.
 */
s32 CamFollow_ShouldTurnBehind(Player *pl, CamFollow *follow)
{
    u16 camPressed;
    u8 group;

    if ((Player_LocalIsOnWall() == 1) && (follow->eyeHeld != 0)) {
        return 1;
    }
    camPressed = follow->pressed;
    if (camPressed & PAD_L1) {
        return 1;
    }
    if ((pl->weaponType == WEAPON_BOW) && (camPressed & PAD_R1) && (pl->input.held & PAD_R1) && (pl->weaponOut != 0)) {
        group = pl->stateGroup;
        if ((group != PL_GROUP_REACT) && (group != PL_GROUP_GESTURE)) {
            return 1;
        }
    }
    return 0;
}

/*
 * 1 when the camera may take the whole pad for player pl, else 0: no HUD
 * input lock (Hud_InputBlocked), none of ammoMenuOpen, supplyBoxOpen,
 * deliveryOpen, deliveryMenu and aimLockTimer set, not fishing
 * (Player_IsFishing), and the follow camera's yaw not following
 * the player's aim (CamFollow_GetCanFollow negative). Seen:
 * CamFollow_GetKeys gives the follow camera every key while it is 1.
 */
s32 Cam_PlayerFree(Player *pl)
{
    if (Hud_InputBlocked() == 1) {
        return 0;
    }
    if (pl->ammoMenuOpen != 0) {
        return 0;
    }
    if (pl->supplyBoxOpen != 0) {
        return 0;
    }
    if (pl->deliveryOpen != 0) {
        return 0;
    }
    if (pl->deliveryMenu != 0) {
        return 0;
    }
    if (pl->aimLockTimer != 0) {
        return 0;
    }
    if (Player_IsFishing(pl) != 0) {
        return 0;
    }
    return CamFollow_GetCanFollow() < 0;
}

/*
 * 0 while the local player (gPlayers[gScene.localPlayer]) is in
 * AREA_BATTLE_TOWN_0 or AREA_BATTLE_TOWN_NIGHT_0, else 1. Seen: the
 * binoculars (the player's item states and the item menu's
 * check) and the bowgun's aim mode are allowed only while it is 1.
 */
s32 Player_LocalCanUseBinoculars(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    switch (pl->area) {
    case AREA_BATTLE_TOWN_0:
    case AREA_BATTLE_TOWN_NIGHT_0:
        return 0;
    }
    return 1;
}

/* This file's initialised data. */
CamDistSet gCamFollowDefaults = {
#include "sub_main/0041C830.inc"
};
CamDist gCamBlockedDists[2] = {
#include "sub_main/0041C930.inc"
};
CamDist gCamFallDist = {
#include "sub_main/0041C970.inc"
};
f32 gCamMode3Offsets[4][6] = {
#include "sub_main/0041C990.inc"
};
f32 gCamMode4AreaPos[25][6] = {
#include "sub_main/0041C9F0.inc"
};
f32 gCamMode4AreaFovs[28] = {
#include "sub_main/0041CC50.inc"
};
u8 gAreaCameraIdx[208] = {
#include "sub_main/0041CCC0.inc"
};
CamEntFn gCamModeInit[8] = {
    CamFollow_Init, CamOrbit_Init, CamRail_Init, CamAim_Init,
    CamFixed_Init, CamScript_Init, NULL, NULL,
};
CamEntFn gCamModeUpdate[6] = {
    CamFollow_Update, CamOrbit_Update, CamRail_Update, CamAim_Update,
    CamFixed_Update, CamScript_Update,
};
s16 gCamOrbitExitBlend[4] = {
#include "sub_main/0041CDC8.inc"
};
f32 gCamOrbitEyeOfs[4] = {
#include "sub_main/0041CDD0.inc"
};
f32 gCamOrbitAimOfs[4] = {
#include "sub_main/0041CDE0.inc"
};
