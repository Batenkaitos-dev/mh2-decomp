/*
 * The camera controller (gCamCtl, CamCtl in camera.h), its second part:
 * the script mode, the shakes, the camera areas and their paths, and the
 * camera's collision tests. CamCtl_Update and the follow camera run the
 * frame; this file supplies what they call.
 *
 * - The script mode (CAM_MODE_SCRIPT): CamScript_Run starts a camera
 *   script (a built-in one, or a dmcam file's) and CamScript_Step runs
 *   its commands (CAMCMD_*, each a CamCmd); CamScript_Place sets the
 *   view at the move's time, easing each value from its start to its
 *   end, with the orbit, the swings and the wall test.
 * - CamCtl_RunMode hands gCamera the current mode's view, or the view of
 *   a mode that overrides it (aim, fixed, script), with the shakes.
 * - The shakes: CamShake_Set* and CamShake_Req* fill the request slots
 *   (gCamCtl.shakeA, B and C) and CamShake_Apply moves the view.
 * - The player's checks: CamCtl_CheckPlayerInView fades the player when
 *   its body is inside a pyramid in front of the eye (Frustum_Build);
 *   CamView_KeepTargetOnScreen keeps the player's low point on the
 *   screen.
 * - The camera data (an area's camera file, or the default):
 *   CamCtl_InitDefaultParams, CamCtl_LoadParams, and Cam_FindPlayerArea,
 *   which finds the camera area the player is in through the grid's
 *   lists (CamArea_TestObj, CamFace_ContainsXZ). CamView_SetTransition
 *   and CamBlend_EaseStep blend the views between areas.
 * - The follow camera's tests of the player (aiming, fishing, on a
 *   wall, the turn behind the player).
 * - The areas' look points and paths: CamPoint_*, the PathPos_* walk
 *   along a path's keys, PathSeg_Nearest (the nearest point of a cubic
 *   segment, through Quintic_Roots and Cubic_Roots on complex numbers)
 *   and Path_BuildSpline (the cubic segments through the keys,
 *   Tridiag_Solve).
 * - The eye's tests: Cam_LineHitDist against the walls, Cam_PushOutOfEms
 *   against the monsters' colliders.
 * - CamReq_QuestTarget, the view when a quest's last target falls; then
 *   the tables: the built-in scripts, the shakes' lengths and ranges, the
 *   follow camera's defaults and the modes' handlers.
 */
#include "common.h"
#include "syssettings.h"
#include "quest.h"
#include "scene.h"
#include "camera.h"
#include "em.h"
#include "pad.h"
#include "player.h"
#include "objhead.h"
#include "capsule.h"
#include "model.h"
#include "area.h"

/* prototypes */
u32 Player_TestFlag(void *obj, u32 flag);
void Vec_Copy(f32 *dst, f32 *src);
f32 Math_Pow(f32 x, f32 y);
void Cam_CopyDefaults(CamDistSet *dst);
s32 Player_LocalIsOnWall(void);
s32 CamArea_IsEnabled(CamArea *area);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Vec_Scale(f32 *, f32 *, f32);
void Vec_Transform(f32 *out, f32 *v, f32 *m);
s32 Hud_InputBlocked(void);
s32 Quest_PhaseIsOver(s32 a);
/* end of prototypes */

/* A complex number (the root finder Quintic_Roots). */
typedef struct Complex {
    f32 x;
    f32 y;
} Complex;

s32 PathPos_MoveToward(PathPos *pos, PathData *d, PathSeg *segs, f32 *pt);
s32 PathPos_PlaceNear(PathPos *pos, PathData *d, PathSeg *segs, f32 *pt);
s32 Path_NearestSeg(PathData *d, f32 *pt);
s32 PathPos_Build(PathPos *pos, PathSeg *segs, PathData *d, f32 *pt);
void PathPos_MoveOnSeg(PathPos *pos, PathData *d, s32 k, f32 dist);

extern u32 gDmcamFileIds[];
u8 *Dmcam_Find(s32 idx);
s32 CamScript_Step(CamCtl *cam, CamView *obj, CamScript *w);

/*
 * Runs the camera script of the script mode's view (view->blend is the
 * run's step). Step 0 starts script script->id: pc = gDmcamFileIds[id]
 * (an entry below DMCAM_ID_LIMIT is a dmcam file id, looked up with Dmcam_Find),
 * mark = pc, the view's and the script's start values copied from
 * cam->out (roll and fov as 16-bit angles), the swings, their noise, the
 * ease and the move (length and left -1) cleared; swingRand's four bytes
 * and easePow are cleared with word stores. Step 1 runs CamScript_Step.
 * Returns 1 when the script is done (CamScript_Step positive; the step
 * goes to 2, which returns 1 again) or skipped (CAM_SCRIPT_STARTED in
 * view->flags and PAD_SELECT newly pressed), else CamScript_Step's 0
 * (wait for the next frame) or -1 (held at a CAMCMD_END). For a step above
 * 2 the result is unset: the game's code returns whatever its return
 * register held on entry.
 */
s32 CamScript_Run(CamCtl *cam, CamView *view)
{
    CamScript *camScript = &cam->script;
    s32 result;

    if ((view->flags & CAM_SCRIPT_STARTED) && (cam->padPressed & PAD_SELECT)) {
        return 1;
    }
    switch (view->blend) {
    case CAM_RUN_START:
        view->blend++;
        camScript->pc = (s8 *)gDmcamFileIds[camScript->id];
        if ((u32)camScript->pc < DMCAM_ID_LIMIT) {
            camScript->pc = (s8 *)Dmcam_Find(camScript->id);
        }
        camScript->mark = camScript->pc;
        Vec_Copy(view->eye, cam->out.eye);
        Vec_Copy(view->at, cam->out.at);
        view->roll = cam->out.roll;
        view->fov = cam->out.fov;
        Vec_Copy(camScript->eye[0], view->eye);
        Vec_Copy(camScript->at[0], view->at);
        camScript->roll[0] = RAD_TO_ANGLE16 * view->roll;
        camScript->fov[0] = RAD_TO_ANGLE16 * view->fov;
        camScript->swing[3].speed = 0;
        camScript->swing[2].speed = 0;
        camScript->swing[1].speed = 0;
        camScript->swing[0].speed = 0;
        *(s32 *)camScript->swingRand = 0;
        camScript->ease = 0;
        *(s32 *)&camScript->easePow = 0;
        view->length = -1;
        view->left = -1;
    case CAM_RUN_STEP:
        if ((result = CamScript_Step(cam, view, camScript)) <= 0) {
            break;
        }
        view->blend++;
    case CAM_RUN_DONE:
        return 1;
    }
    return result;
}

/*
 * The world matrix of the script's object script->obj (an Em or a Player:
 * CamCtl.requestArg) while it exists and is active. Otherwise sets
 * CAM_SCRIPT_ENDED in view->flags (CamScript_Step then ends the script)
 * and returns NULL.
 */
static void *CamScript_ObjMtx(CamView *view, CamScript *script)
{
    Em *obj;

    obj = script->obj;
    if ((obj != NULL) && (obj->active != 0)) {
        return obj->worldMtx;
    }
    view->flags |= CAM_SCRIPT_ENDED;
    return NULL;
}


/*
 * An eye point of the script (CAMCMD_EYE_POINT) into out: the command's words
 * 1-3 (cmd[1..3], fixed point, 4096 = 1) taken in the space
 * script->eyeSpace: CAM_SPACE_PART through the local player's part
 * eyePart (its PlRec's m1), CAM_SPACE_BODY through the local player's
 * worldMtx, CAM_SPACE_POSITION added to its pos, CAM_SPACE_OBJECT through
 * the object's matrix (CamScript_ObjMtx; left as is without one),
 * CAM_SPACE_WORLD as is, CAM_SPACE_AT added to the target's start at[0].
 */
void CamScript_EyePoint(f32 *out, CamView *view, CamScript *script, s32 *cmd)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    void *objMtx;

    out[0] = 0.00024414062f * cmd[1];
    out[1] = 0.00024414062f * cmd[2];
    out[2] = 0.00024414062f * cmd[3];
    switch (script->eyeSpace) {
    case 2:
        Vec_Transform(out, out, pl->partRecs[script->eyePart]->m1);
        break;
    case 0:
        Vec_Transform(out, out, pl->worldMtx[0]);
        break;
    case 1:
        Vec_Add(out, out, pl->pos);
        break;
    case CAM_SPACE_OBJECT:
        objMtx = CamScript_ObjMtx(view, script);
        if (objMtx != NULL) {
            Vec_Transform(out, out, objMtx);
        }
        break;
    case CAM_SPACE_WORLD:
        break;
    case CAM_SPACE_AT:
        Vec_Add(out, out, script->at[0]);
        break;
    }
}

/*
 * A target point of the script (CAMCMD_AT_POINT): as CamScript_EyePoint, in the
 * space script->atSpace with the part atPart; there is no CAM_SPACE_AT.
 */
void CamScript_AtPoint(f32 *out, CamView *view, CamScript *script, s32 *cmd)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    void *objMtx;

    out[0] = 0.00024414062f * cmd[1];
    out[1] = 0.00024414062f * cmd[2];
    out[2] = 0.00024414062f * cmd[3];
    switch (script->atSpace) {
    case 2:
        Vec_Transform(out, out, pl->partRecs[script->atPart]->m1);
        break;
    case 0:
        Vec_Transform(out, out, pl->worldMtx[0]);
        break;
    case 1:
        Vec_Add(out, out, pl->pos);
        break;
    case CAM_SPACE_OBJECT:
        objMtx = CamScript_ObjMtx(view, script);
        if (objMtx != NULL) {
            Vec_Transform(out, out, objMtx);
        }
        break;
    case CAM_SPACE_WORLD:
        break;
    }
}

void Vec_Copy(f32 *dst, f32 *src);

/*
 * CAMCMD_KEEP: for each CAM_KEEP_* bit of mask copies a start value [0] into its end
 * value [1], so it stays put over the move: the eye, the target, the roll, the fov
 * and the amplitudes of swings 0 to 3.
 */
void CamScript_CopyToEnd(CamScript *script, s32 mask)
{
    if (mask & CAM_KEEP_EYE) {
        Vec_Copy(script->eye[1], script->eye[0]);
    }
    if (mask & CAM_KEEP_AT) {
        Vec_Copy(script->at[1], script->at[0]);
    }
    if (mask & CAM_KEEP_ROLL) {
        script->roll[1] = script->roll[0];
    }
    if (mask & CAM_KEEP_FOV) {
        script->fov[1] = script->fov[0];
    }
    if (mask & CAM_KEEP_SWING0) {
        script->swing[0].amp[1] = script->swing[0].amp[0];
    }
    if (mask & CAM_KEEP_SWING1) {
        script->swing[1].amp[1] = script->swing[1].amp[0];
    }
    if (mask & CAM_KEEP_SWING2) {
        script->swing[2].amp[1] = script->swing[2].amp[0];
    }
    if (mask & CAM_KEEP_SWING3) {
        script->swing[3].amp[1] = script->swing[3].amp[0];
    }
}

/*
 * The script's orbit angles (CamScript.angle, x then y) into out at the
 * script view's time: angle[1] + (angle[0] - angle[1]) * left / length
 * in integers. left counts down from length, so [0] at the move's start
 * and [1] at its end; angle[0] when the view's length is not positive.
 */
void CamScript_OrbitAngles(s16 *out, CamCtl *cam)
{
    CamView *view = &cam->modes[CAM_MODE_SCRIPT];
    CamScript *camScript = &cam->script;
    s16 *frames;

    if (cam->modes[CAM_MODE_SCRIPT].length > 0) {
        out[0] = camScript->angle[0][0] - camScript->angle[1][0];
        frames = &view->left;
        out[0] = camScript->angle[1][0] + out[0] * frames[0] / frames[1];
        out[1] = camScript->angle[0][1] - camScript->angle[1][1];
        out[1] = camScript->angle[1][1] + out[1] * frames[0] / frames[1];
    } else {
        out[0] = camScript->angle[0][0];
        out[1] = camScript->angle[0][1];
    }
}

void CamScript_OrbitAngles(s16 *out, CamCtl *c);
void Mtx_Identity(f32 *m);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Vec_Lerp(f32 *, f32 *, f32 *, f32 t);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Normalize(f32 *v);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
f32 Math_CosAngle(s32 a);
u16 Rand_Next(s32 stream);
/* unit's own declaration: wider than the definition's u8: with u8, CamScript_Place masks the value before its == 1, which the game's code doesn't */
s32 AreaHit_Raycast(f32 *from, f32 *to, f32 *out, u16 mask);
f32 AreaGrid2_GetFloorY(f32 *pos);
void Quat_Set(f32 *axis, f32 *q, f32 angle);
void Quat_ToMatrix(f32 *q, f32 (*m)[4]);

/*
 * Places the script view at the move's time. startWeight = left / length
 * (1 at the move's start, 0 at its end; 1 without a move), eased by
 * script->ease with the power easePow (CAM_EASE_*); each
 * pair is start * startWeight + end * endWeight (Vec_Lerp the same).
 * CAM_PLACE_EASE eases eye and target between their pairs. CAM_PLACE_ORBIT orbits: an
 * offset is turned by the orbit angles (CamScript_OrbitAngles), then by
 * the space's matrix, and added to one eased point to give the other:
 * pivotEye 1, target = eye + (0, 0, 500) turned (by eyeSpace, with the
 * part atPart); else eye = target + (0, 0, the eased dist) turned (by
 * atSpace, with the part eyePart). With CAM_SCRIPT_CLAMP_EYE in view->flags
 * (CAMCMD_CLAMP_EYE) the eye moves to where the line from the target hits a
 * wall (AreaHit_Raycast, CAM_SWEEP_MASK) and stays 30 above the floor.
 * Roll and fov are eased; swing 2 adds to the roll and swing 3 to the
 * fov (the amplitude eased, times the cosine of the phase, plus noise
 * from Rand_Next scaled by swingRand). Swings 0 and 1 sway eye and target
 * sideways and up (the offset turned about the view direction by the
 * roll). For swings 0 and 1 a nonzero swingRand only draws a random
 * number, unused.
 */
void CamScript_Place(CamCtl *cam, CamView *view, CamScript *camScript)
{
    f32 mtx[16];
    f32 rollMtx[16];
    f32 quat[4];
    f32 offset[3];
    f32 sway[3];
    f32 hit[3];
    f32 side[3];
    f32 upDir[3];
    f32 dir[3];
    s16 orbitAng[2];
    Player *pl;
    void *objMtx;
    f32 startWeight;
    f32 endWeight;
    f32 swingAmp;
    f32 y;
    u8 flags;

    if (view->length > 0) {
        startWeight = (f32)view->left / (f32)view->length;
        switch (camScript->ease) {
        case CAM_EASE_IN:
            startWeight = Math_Pow(startWeight, camScript->easePow);
            break;
        case CAM_EASE_OUT:
            startWeight = 1.0f - Math_Pow(1.0f - startWeight, camScript->easePow);
            break;
        case CAM_EASE_IN_OUT:
            if (startWeight < 0.5f) {
                startWeight = 0.5f * Math_Pow(2.0f * startWeight, camScript->easePow);
            } else {
                startWeight = 1.0f - 0.5f * Math_Pow(2.0f * (1.0f - startWeight), camScript->easePow);
            }
            break;
        }
    } else {
        startWeight = 1.0f;
    }
    endWeight = 1.0f - startWeight;
    switch (camScript->place) {
    case CAM_PLACE_ORBIT:
        CamScript_OrbitAngles(orbitAng, cam);
        Mtx_Identity(mtx);
        Mtx_RotateXYZ(mtx, ANGLE16_TO_RAD * orbitAng[0], ANGLE16_TO_RAD * orbitAng[1], 0.0f);
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offset[2] = 500.0f;
        pl = &gPlayers[gScene.localPlayer];
        if (camScript->pivotEye == 1) {
            Vec_Lerp(view->eye, camScript->eye[0], camScript->eye[1], startWeight);
            Vec_RotateSelf(offset, (f32 *)mtx);
            switch (camScript->eyeSpace) {
            case 2:
                Vec_RotateSelf(offset, pl->partRecs[camScript->atPart]->m1);
                break;
            case 0:
                Vec_RotateSelf(offset, (f32 *)pl->worldMtx[0]);
                break;
            case CAM_SPACE_OBJECT:
                objMtx = CamScript_ObjMtx(view, camScript);
                if (objMtx != NULL) {
                    Vec_RotateSelf(offset, (f32 *)objMtx);
                }
                break;
            case 1:
            case CAM_SPACE_WORLD:
            case CAM_SPACE_AT:
                break;
            }
            Vec_Add(view->at, view->eye, offset);
        } else {
            Vec_Lerp(view->at, camScript->at[0], camScript->at[1], startWeight);
            offset[2] = camScript->dist[0] * startWeight + camScript->dist[1] * endWeight;
            Vec_RotateSelf(offset, (f32 *)mtx);
            switch (camScript->atSpace) {
            case 2:
                Vec_RotateSelf(offset, pl->partRecs[camScript->eyePart]->m1);
                break;
            case 0:
                Vec_RotateSelf(offset, (f32 *)pl->worldMtx[0]);
                break;
            case CAM_SPACE_OBJECT:
                objMtx = CamScript_ObjMtx(view, camScript);
                if (objMtx != NULL) {
                    Vec_RotateSelf(offset, (f32 *)objMtx);
                }
                break;
            case 1:
            case CAM_SPACE_WORLD:
                break;
            }
            Vec_Add(view->eye, view->at, offset);
        }
        break;
    case CAM_PLACE_EASE:
        Vec_Lerp(view->at, camScript->at[0], camScript->at[1], startWeight);
        Vec_Lerp(view->eye, camScript->eye[0], camScript->eye[1], startWeight);
        break;
    }
    if (view->flags & CAM_SCRIPT_CLAMP_EYE) {
        if (AreaHit_Raycast(view->at, view->eye, hit, CAM_SWEEP_MASK) == 1) {
            Vec_Copy(view->eye, hit);
        }
        hit[1] = AreaGrid2_GetFloorY(view->eye);
        y = 30.0f + hit[1];
        if (!(y <= view->eye[1])) {
            view->eye[1] = y;
        }
    }
    view->roll = ANGLE16_TO_RAD * camScript->roll[0] * startWeight + ANGLE16_TO_RAD * camScript->roll[1] * endWeight;
    view->fov = ANGLE16_TO_RAD * camScript->fov[0] * startWeight + ANGLE16_TO_RAD * camScript->fov[1] * endWeight;
    if (camScript->swing[2].speed != 0) {
        swingAmp = (f32)camScript->swing[2].amp[0] * startWeight + (f32)camScript->swing[2].amp[1] * endWeight;
        swingAmp *= Math_CosAngle(camScript->swing[2].phase);
        camScript->swing[2].phase += camScript->swing[2].speed;
        if (camScript->swingRand[2] != 0) {
            swingAmp += 3.0637257e-5f * (swingAmp * camScript->swingRand[2] * (f32)((Rand_Next(1) & 0xFF) - 0x80));
        }
        view->roll += ANGLE16_TO_RAD * swingAmp;
    }
    if (camScript->swing[3].speed != 0) {
        swingAmp = (f32)camScript->swing[3].amp[0] * startWeight + (f32)camScript->swing[3].amp[1] * endWeight;
        swingAmp *= Math_CosAngle(camScript->swing[3].phase);
        camScript->swing[3].phase += camScript->swing[3].speed;
        if (camScript->swingRand[3] != 0) {
            swingAmp += 3.0637257e-5f * (swingAmp * camScript->swingRand[3] * (f32)((Rand_Next(1) & 0xFF) - 0x80));
        }
        view->fov += ANGLE16_TO_RAD * swingAmp;
    }
    sway[1] = 0.0f;
    sway[0] = 0.0f;
    flags = 0;
    if (camScript->swing[0].speed != 0) {
        swingAmp = (f32)camScript->swing[0].amp[0] * startWeight + (f32)camScript->swing[0].amp[1] * endWeight;
        sway[0] = swingAmp * Math_CosAngle(camScript->swing[0].phase);
        flags |= 1;
        camScript->swing[0].phase += camScript->swing[0].speed;
        if (camScript->swingRand[0] != 0) {
            Rand_Next(1);
        }
    }
    if (camScript->swing[1].speed != 0) {
        swingAmp = (f32)camScript->swing[1].amp[0] * startWeight + (f32)camScript->swing[1].amp[1] * endWeight;
        sway[1] = swingAmp * Math_CosAngle(camScript->swing[1].phase);
        flags |= 1;
        camScript->swing[1].phase += camScript->swing[1].speed;
        if (camScript->swingRand[1] != 0) {
            Rand_Next(1);
        }
    }
    if (flags & 1) {
        Vec_Diff(dir, view->at, view->eye);
        upDir[1] = 1.0f;
        upDir[2] = 0.0f;
        upDir[0] = 0.0f;
        Vec_Cross(side, upDir, dir);
        Vec_Normalize(side);
        Vec_Cross(upDir, dir, side);
        Vec_Normalize(upDir);
        Vec_Scale(side, side, sway[0]);
        Vec_Scale(upDir, upDir, sway[1]);
        Vec_Add(sway, side, upDir);
        Quat_Set(dir, quat, view->roll);
        Quat_ToMatrix(quat, (f32 (*)[4])rollMtx);
        Vec_Rotate(offset, sway, rollMtx);
        Vec_Add(view->eye, view->eye, offset);
        Vec_Add(view->at, view->at, offset);
    }
}

void CamScript_Place(CamCtl *cam, CamView *obj, CamScript *w);

/*
 * Runs script commands from script->pc until one yields (pc is left on
 * the next one). A command is cmd[0] (its kind), cmd[1] (its length in
 * 4-byte words), then byte arguments cmd[2] and cmd[3], or a 16-bit one
 * at cmd + 2, and words from cmd + 4. Kinds: 0 place; 1 eye space and
 * part; 2 an eye point (cmd[2]: 0 start, 1 end); 3 target space and part;
 * 4 a target point; 5 pivotEye; 6-9 the orbit angles (x start, x end,
 * y start, y end); 10, 11 the orbit distance's start and end (word 1 /
 * 16); 12, 13 the roll's; 14, 15 the fov's; 16 the move's length (left =
 * length); 17-32 the four swings' phase, speed, start and end amplitude;
 * 33 swingRand[cmd[2]] = cmd[3]; 34 CamScript_CopyToEnd(cmd[2]); 35 the
 * ease (top 2 bits) and its power (low 14 bits / 1024); 36 the move's
 * length and the loop's start (loop = the next command); 37 a loop
 * frame: CamScript_Place when cmd[2] is set, left counts down, and while
 * the move lasts (forever without a length) back to the loop, yielding
 * 0; once over, the end values become the start ones (eye and target
 * from the view), the swings stop, ease and move are cleared and it goes
 * on; 38 CamScript_Place, then back to the loop yielding 0 when cmd[2]
 * is set; 40 yields -1; 41 sets CAM_SCRIPT_CLAMP_EYE in view->flags (the eye's wall and
 * floor test); 43 sets the mark; 42 jumps to the mark when there is one.
 * Clears CAM_SCRIPT_ENDED first; returns 1 when a command sets it (the
 * object is gone) or on an unknown kind, else the yield's 0 or -1.
 */
s32 CamScript_Step(CamCtl *cam, CamView *view, CamScript *script)
{
    s8 *next;
    s32 run;
    CamCmd *cmd;
    s32 result;
    s32 *cmdWords;

    run = 1;
    view->flags &= (u8)~CAM_SCRIPT_ENDED;
    next = script->pc;
    do {
        cmd = (CamCmd *)next;
        /* the command's words are read through a second pointer of their own:
         * read through cmd, they compile to different code */
        cmdWords = (s32 *)next;
        next += cmd->words * 4;
        switch (cmd->kind) {
        case CAMCMD_PLACE:
            script->place = cmd->arg.byte[0];
            break;
        case CAMCMD_EYE_SPACE:
            script->eyeSpace = cmd->arg.byte[0];
            script->eyePart = cmd->arg.byte[1];
            break;
        case CAMCMD_EYE_POINT:
            CamScript_EyePoint(script->eye[cmd->arg.byte[0]], view, script, cmdWords);
            break;
        case CAMCMD_AT_SPACE:
            script->atSpace = cmd->arg.byte[0];
            script->atPart = cmd->arg.byte[1];
            break;
        case CAMCMD_AT_POINT:
            CamScript_AtPoint(script->at[cmd->arg.byte[0]], view, script, cmdWords);
            break;
        case CAMCMD_PIVOT:
            script->pivotEye = cmd->arg.byte[0];
            break;
        case CAMCMD_ANGLE_X0:
            script->angle[0][0] = cmd->arg.half;
            break;
        case CAMCMD_ANGLE_X1:
            script->angle[1][0] = cmd->arg.half;
            break;
        case CAMCMD_ANGLE_Y0:
            script->angle[0][1] = cmd->arg.half;
            break;
        case CAMCMD_ANGLE_Y1:
            script->angle[1][1] = cmd->arg.half;
            break;
        case CAMCMD_DIST0:
            script->dist[0] = 0.0625f * cmdWords[1];
            break;
        case CAMCMD_DIST1:
            script->dist[1] = 0.0625f * cmdWords[1];
            break;
        case CAMCMD_ROLL0:
            script->roll[0] = cmd->arg.half;
            break;
        case CAMCMD_ROLL1:
            script->roll[1] = cmd->arg.half;
            break;
        case CAMCMD_FOV0:
            script->fov[0] = cmd->arg.half;
            break;
        case CAMCMD_FOV1:
            script->fov[1] = cmd->arg.half;
            break;
        case CAMCMD_LENGTH:
            view->left = view->length = cmd->arg.half;
            break;
        case CAMCMD_SWAY_X_PHASE:
            script->swing[0].phase = cmd->arg.half;
            break;
        case CAMCMD_SWAY_X_SPEED:
            script->swing[0].speed = cmd->arg.half;
            break;
        case CAMCMD_SWAY_X_AMP0:
            script->swing[0].amp[0] = cmd->arg.half;
            break;
        case CAMCMD_SWAY_X_AMP1:
            script->swing[0].amp[1] = cmd->arg.half;
            break;
        case CAMCMD_SWAY_Y_PHASE:
            script->swing[1].phase = cmd->arg.half;
            break;
        case CAMCMD_SWAY_Y_SPEED:
            script->swing[1].speed = cmd->arg.half;
            break;
        case CAMCMD_SWAY_Y_AMP0:
            script->swing[1].amp[0] = cmd->arg.half;
            break;
        case CAMCMD_SWAY_Y_AMP1:
            script->swing[1].amp[1] = cmd->arg.half;
            break;
        case CAMCMD_ROLL_SWING_PHASE:
            script->swing[2].phase = cmd->arg.half;
            break;
        case CAMCMD_ROLL_SWING_SPEED:
            script->swing[2].speed = cmd->arg.half;
            break;
        case CAMCMD_ROLL_SWING_AMP0:
            script->swing[2].amp[0] = cmd->arg.half;
            break;
        case CAMCMD_ROLL_SWING_AMP1:
            script->swing[2].amp[1] = cmd->arg.half;
            break;
        case CAMCMD_FOV_SWING_PHASE:
            script->swing[3].phase = cmd->arg.half;
            break;
        case CAMCMD_FOV_SWING_SPEED:
            script->swing[3].speed = cmd->arg.half;
            break;
        case CAMCMD_FOV_SWING_AMP0:
            script->swing[3].amp[0] = cmd->arg.half;
            break;
        case CAMCMD_FOV_SWING_AMP1:
            script->swing[3].amp[1] = cmd->arg.half;
            break;
        case CAMCMD_SWING_RAND:
            script->swingRand[cmd->arg.byte[0]] = cmd->arg.byte[1];
            break;
        case CAMCMD_KEEP:
            CamScript_CopyToEnd(script, cmd->arg.byte[0]);
            break;
        case CAMCMD_EASE:
            script->ease = (cmd->arg.half >> 14) & 3;
            script->easePow = 0.0009765625f * (cmd->arg.half & 0x3FFF);
            break;
        case CAMCMD_LOOP:
            view->left = view->length = cmd->arg.half;
            script->loop = next;
            break;
        case CAMCMD_LOOP_FRAME:
            if (cmd->arg.byte[0] != 0) {
                CamScript_Place(cam, view, script);
            }
            (view->left)--;
            if (view->length <= 0 || view->left >= 0) {
                next = script->loop;
                run = 0;
                result = 0;
            } else {
                Vec_Copy(script->eye[0], view->eye);
                Vec_Copy(script->at[0], view->at);
                script->angle[0][0] = script->angle[1][0];
                script->angle[0][1] = script->angle[1][1];
                script->roll[0] = script->roll[1];
                script->fov[0] = script->fov[1];
                script->swing[0].speed = 0;
                script->swing[1].speed = 0;
                script->swing[2].speed = 0;
                script->swing[3].speed = 0;
                script->swing[0].amp[0] = script->swing[0].amp[1];
                script->swing[1].amp[0] = script->swing[1].amp[1];
                script->swing[2].amp[0] = script->swing[2].amp[1];
                script->swing[3].amp[0] = script->swing[3].amp[1];
                script->ease = 0;
                script->easePow = 0.0f;
                view->length = view->left = -1;
            }
            break;
        case CAMCMD_LOOP_PLACE:
            CamScript_Place(cam, view, script);
            if (cmd->arg.byte[0] != 0) {
                next = script->loop;
                run = 0;
                result = 0;
            }
            break;
        case CAMCMD_CLAMP_EYE:
            view->flags |= CAM_SCRIPT_CLAMP_EYE;
            break;
        case CAMCMD_SET_MARK:
            script->mark = next;
            break;
        case CAMCMD_GOTO_MARK:
            if (script->mark != NULL) {
                next = script->mark;
            }
            break;
        default:
            return 1;
        case CAMCMD_END:
            run = 0;
            result = -1;
            break;
        }
        if (view->flags & CAM_SCRIPT_ENDED) {
            return 1;
        }
        script->pc = next;
    } while (run);
    return result;
}

void Camera_SetRoll(f32 roll);
void Camera_StoreFov(f32 fov);
void CamFollow_SetYawFrom(s32 v);
void CamShake_Apply(CamShake *t);

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
void Frustum_Build(f32 *out, void *m, f32 fov, f32 k);
s32 Frustum_ClipSegment(f32 *a, f32 *b);
void Cam_GetPlayerBodySegment(f32 *seg, Player *pl);

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

EmBone * Em_GetBone(void *obj, s16 bone);
void Vec_Lerp(f32 *, f32 *, f32 *, f32 t);

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

void Math_GetSinCos(f32 *s, f32 *c, f32 a);
void Vec4_TransformSelf(f32 *v, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Normalize(f32 *v);

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

void Vec_Diff(f32 *out, f32 *a, f32 *b);
f32 Vec_Dot(f32 *a, f32 *b);
s32 Cam_GetPlayerLowPoint(f32 *out, Player *pl, CamView *view);
void Camera_ComputeUp(f32 *up, f32 *eye, f32 *at, f32 roll);
void Mtx_LookAt(f32 *m, f32 *eye, f32 *at, f32 *up);
void Mtx_Inverse(f32 *out, f32 *m);
void Vec_Cross(f32 *out, f32 *a, f32 *b);
void Mtx_Identity(f32 *m);
void Mtx_FromAxisAngle(f32 *m, f32 *axis, f32 angle);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
f32 Math_Atan2(f32 y, f32 x);

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

EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_Copy(f32 *dst, f32 *src);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
f32 Vec_Dot(f32 *a, f32 *b);
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

s32 Obj_InCurrentArea(void *obj);

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

/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);

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

f32 Math_Sin(f32 x);
f32 Vec_Distance(f32 *a, f32 *b);
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

extern CamDistSet gCamFollowDefaults;

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

s32 CamArea_TestObj(CamArea *area, Player *obj, u8 mask);

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

s32 CamFace_ContainsXZ(CamFace *face, f32 *pos);

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

void CamView_StartBlend(CamView *p, s32 n);

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

f32 Math_Atan2(f32 y, f32 x);

/*
 * The angle (0x10000 per turn) of the direction from fromPos to toPos in
 * the x-z plane: RAD_TO_ANGLE16 * Math_Atan2(dx, dz). Seen: the follow
 * camera's yaw from an eye to its target (CamFollow_Update).
 */
s16 Vec_AngleXZ16(f32 *toPos, f32 *fromPos)
{
    return (u16)(s32)(RAD_TO_ANGLE16 * Math_Atan2(toPos[0] - fromPos[0], toPos[2] - fromPos[2]));
}

/*
 * A bump over the 16-bit angle `angle` (0x10000 per turn): 0 up to
 * rangeLo; amp * (angle - rangeLo)^2 * (rangeHi - angle)^2 / 2^32 between
 * rangeLo and rangeHi; 0 from rangeHi to 0x10000 - rangeHi; the same
 * bump mirrored and negated from 0x10000 - rangeHi to 0x10000 - rangeLo;
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
            angle = 0x10000 - angle;
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
 * 1 when player pl has the fishing flag (0x80000, Player_TestFlag), else 0. Only the
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

s32 CamFollow_IsBlownAway(void);
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
 * yaw then eases behind the player when it is less than 0x6800 off, at
 * 1 from any gap, at 0 only past 0x1000 (Angle_Bump).
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

s8 CamFollow_GetCanFollow(void);
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

/*
 * Puts the look point of camera area `area` (CAM_AREA_FIXED and CAM_AREA_OFFSET, CamAreaFixed)
 * into out. By area->lookSpace: 0 fixed.at in the world, 1 fixed.atLocal
 * through the player's worldMtx, 2 the player's pos plus fixed.atOffset.
 * Then by area->lookLock: 0 takes fixed.at's y, 1 its x and z, 2 none.
 * The player is ctl->player. Seen: CamRail_Update.
 */
void CamPoint_GetPos(CamCtl *ctl, f32 *out, CamArea *area)
{
    Player *pl;
    u8 space;
    u8 lockMode;

    pl = ctl->player;
    space = area->lookSpace;
    switch (space) {
    case CAM_LOOK_SPACE_WORLD:
        Vec_Copy(out, area->fixed.at);
        break;
    case CAM_LOOK_SPACE_PLAYER:
        Vec_Transform(out, area->fixed.atLocal, pl->worldMtx[0]);
        break;
    case CAM_LOOK_SPACE_OFFSET:
        Vec_Add(out, pl->pos, area->fixed.atOffset);
        break;
    }
    lockMode = area->lookLock;
    switch (lockMode) {
    case CAM_LOOK_LOCK_Y:
        out[1] = area->fixed.at[1];
        break;
    case CAM_LOOK_LOCK_XZ:
        out[0] = area->fixed.at[0];
        out[2] = area->fixed.at[2];
        break;
    }
}

/*
 * CamPoint_GetPos for a camera area of kind CAM_AREA_PATH (CamAreaPath), with
 * pathPoint in place of the area's world point: lookSpace 0 copies
 * pathPoint, 1 is path.atLocal through the player's worldMtx, 2 the
 * player's pos plus path.atOffset; lookLock 0 takes pathPoint's y, 1 its
 * x and z. Seen: CamRail_Update passes the point of the area's second
 * key set (keys2) where the player is placed.
 */
void CamPoint_GetPosOr(CamCtl *ctl, f32 *out, CamArea *area, f32 *pathPoint)
{
    Player *pl;
    u8 space;
    u8 lockMode;

    pl = ctl->player;
    space = area->lookSpace;
    switch (space) {
    case 0:
        Vec_Copy(out, pathPoint);
        break;
    case 1:
        Vec_Transform(out, area->path.atLocal, pl->worldMtx[0]);
        break;
    case 2:
        Vec_Add(out, pl->pos, area->path.atOffset);
        break;
    }
    lockMode = area->lookLock;
    switch (lockMode) {
    case 0:
        out[1] = pathPoint[1];
        break;
    case 1:
        out[0] = pathPoint[0];
        out[2] = pathPoint[2];
        break;
    }
}

void Path_BuildSpline(PathSeg *seg, PathKey *k, s32 n);
void Vec_EvalCubic(f32 *out, PathSeg *k, f32 t);

/*
 * Puts into out the point of area's eye path (path.d.keys) at ctl's path
 * position: builds that path's segments into segs (Path_BuildSpline) and
 * evaluates segment ctl->pathPos.k at pathPos.t times the key's len
 * (Vec_EvalCubic). pathPos is placed on the second key set (keys2), so t
 * carries the place over from that path to this one.
 */
void Path_PointAt(f32 *out, CamCtl *ctl, CamArea *area, PathSeg *segs)
{
    u8 segNo;

    Path_BuildSpline(segs, area->path.d.keys, area->path.d.count);
    segNo = ctl->pathPos.k;
    Vec_EvalCubic(out, &segs[segNo], ctl->pathPos.t * area->path.d.keys[segNo].len);
}

f32 Vec_Distance(f32 *a, f32 *b);

/*
 * The segment of path nearest target: from the key of keys2 nearest to
 * it, or the one before when the key before is nearer than the key after;
 * the last key gives count - 2. The walk steps 12 bytes through the
 * 16-byte keys2 records (as the game's code does), so only key 0's position
 * is read whole.
 */
s32 Path_NearestSeg(PathData *path, f32 *target)
{
    f32 dist[16];
    f32 *keyPos = path->keys2[0].pos;
    f32 *distPtr = dist;
    s32 i;
    s32 best = 0;
    f32 minDist = 1e7f;

    for (i = 0; i < path->count; i++) {
        *distPtr = Vec_Distance(target, keyPos);
        if (minDist > *distPtr) {
            minDist = *distPtr;
            best = i;
        }
        keyPos += 3;
        distPtr++;
    }
    if (best != 0 && best < path->count - 1) {
        if (dist[best - 1] < dist[best + 1]) {
            best--;
        }
    } else if (best >= path->count - 1) {
        best = path->count - 2;
    }
    return best;
}

/*
 * Places pos->dist on the segment that starts at key (its length
 * key->len) from the candCount candidate distances cands (PathSeg_Nearest's
 * roots), in order: the first one within 0..len is taken (returns 0);
 * else pos->dist is clamped to the end nearest the closest candidate, 0
 * (returns -1) or len (returns 1). seg is not read.
 */
s32 PathPos_PlaceOnSeg(PathPos *pos, PathSeg *seg, PathKey *key, f32 *cands, s32 candCount)
{
    f32 best;
    f32 cand;
    f32 first;
    s32 side;

    first = *cands;
    if (first < 0.0f) {
        pos->dist = 0.0f;
        side = -1;
        best = -first;
    } else if (!(first <= key->len)) {
        best = first - key->len;
        side = 1;
        pos->dist = key->len;
    } else {
        side = 0;
        pos->dist = first;
        return side;
    }
    if (candCount == 1) {
        return side;
    }
    for (candCount--; candCount != 0; candCount--) {
        cand = *++cands;
        if (cand < 0.0f) {
            if (!(best <= -cand)) {
                best = -cand;
                side = -1;
                pos->dist = 0.0f;
            }
        } else if (!(cand <= key->len)) {
            cand -= key->len;
            if (!(best <= cand)) {
                pos->dist = key->len;
                side = 1;
                best = cand;
            }
        } else {
            side = 0;
            pos->dist = cand;
            return side;
        }
    }
    return side;
}

s32 PathSeg_Nearest(f32 *ts, PathSeg *seg, f32 *pt, s32 axes);

/*
 * Builds the segments of path's second key set (keys2) into segs and
 * places pos->dist on segment pos->k at the point nearest target
 * (PathSeg_Nearest, with path->nearest3D as its axes, then
 * PathPos_PlaceOnSeg). Past the segment's end it moves to the next
 * segment (while pos->k < count - 2), past its start to the one before
 * (pos->k > 0), when that segment has candidates. Returns 1; 0 with
 * pos->dist cleared when segment pos->k has no candidate.
 */
s32 PathPos_Build(PathPos *pos, PathSeg *segs, PathData *path, f32 *target)
{
    f32 cands[8];
    s32 candCount;
    s32 side;

    Path_BuildSpline(segs, path->keys2, path->count);
    candCount = PathSeg_Nearest(cands, &segs[pos->k], target, path->nearest3D);
    if (candCount == 0) {
        pos->dist = 0.0f;
        return 0;
    }
    side = PathPos_PlaceOnSeg(pos, &segs[pos->k], &path->keys2[pos->k], cands, candCount);
    if (side > 0) {
        if (pos->k < path->count - 2) {
            candCount = PathSeg_Nearest(cands, &segs[pos->k + 1], target, path->nearest3D);
            if (candCount != 0) {
                pos->k++;
                PathPos_PlaceOnSeg(pos, &segs[pos->k], &path->keys2[pos->k], cands, candCount);
            }
        }
    } else if (side < 0) {
        if (pos->k > 0) {
            candCount = PathSeg_Nearest(cands, &segs[pos->k - 1], target, path->nearest3D);
            if (candCount != 0) {
                pos->k--;
                PathPos_PlaceOnSeg(pos, &segs[pos->k], &path->keys2[pos->k], cands, candCount);
            }
        }
    }
    return 1;
}

/*
 * Moves pos along path's second key set (keys2) by at most 0.5 toward
 * distance goalDist on segment goalSeg, crossing segment ends on the way.
 * Going back, each segment entered starts at keys2[goalSeg].len, the
 * goal segment's length, as the code reads it. Seen: only
 * PathPos_MoveToward calls it, when goalSeg is another segment.
 */
void PathPos_MoveOnSeg(PathPos *pos, PathData *path, s32 goalSeg, f32 goalDist)
{
    f32 step = 0.5f;
    f32 rem;
    f32 cur;

    if (pos->k < goalSeg) {
        do {
            cur = pos->dist;
            rem = path->keys2[pos->k].len - cur;
            if (rem <= step) {
                step -= rem;
                pos->k++;
                pos->dist = 0.0f;
            } else {
                pos->dist = cur + step;
                return;
            }
        } while (pos->k < goalSeg);
        if (goalDist - pos->dist <= step) {
            pos->dist = goalDist;
        } else {
            pos->dist = pos->dist + step;
        }
    } else {
        do {
            if (pos->dist <= step) {
                step -= pos->dist;
                pos->k--;
                pos->dist = path->keys2[goalSeg].len;
            } else {
                pos->dist = pos->dist - step;
                return;
            }
        } while (pos->k > goalSeg);
        if (pos->dist - goalDist <= step) {
            pos->dist = goalDist;
        } else {
            pos->dist = pos->dist - step;
        }
    }
}

/*
 * Moves pos along path toward target: finds the place nearest target
 * (Path_NearestSeg, PathPos_Build), then steps pos->dist toward it by at
 * most 0.5 on the same segment, or with PathPos_MoveOnSeg across
 * segments, and sets pos->t to dist over the segment's keys2 len.
 * Returns 0; -1 with pos unchanged when no place is found.
 */
s32 PathPos_MoveToward(PathPos *pos, PathData *path, PathSeg *segs, f32 *target)
{
    PathPos goal;

    goal.k = Path_NearestSeg(path, target);
    if (PathPos_Build(&goal, segs, path, target) != 0) {
        if (pos->k == goal.k) {
            if (pos->dist < goal.dist) {
                if (goal.dist - pos->dist <= 0.5f) {
                    pos->dist = goal.dist;
                } else {
                    pos->dist = pos->dist + 0.5f;
                }
            } else if (!(pos->dist <= goal.dist)) {
                if (pos->dist - goal.dist <= 0.5f) {
                    pos->dist = goal.dist;
                } else {
                    pos->dist = pos->dist - 0.5f;
                }
            }
        } else {
            PathPos_MoveOnSeg(pos, path, goal.k, goal.dist);
        }
    } else {
        return -1;
    }
    pos->t = pos->dist / path->keys2[pos->k].len;
    return 0;
}


/*
 * Places pos on path at the place nearest target: the segment from
 * Path_NearestSeg, the distance from PathPos_Build, t the distance over
 * the segment's keys2 len. Returns 0; -1 with t and dist cleared when
 * PathPos_Build finds no place.
 */
s32 PathPos_PlaceNear(PathPos *pos, PathData *path, PathSeg *segs, f32 *target)
{
    pos->k = Path_NearestSeg(path, target);
    if (PathPos_Build(pos, segs, path, target) != 0) {
        pos->t = pos->dist / path->keys2[pos->k].len;
        return 0;
    }
    pos->t = 0.0f;
    pos->dist = 0.0f;
    return -1;
}

/*
 * Places ctl->pathPos on the current area's path (area->path.d) near
 * pos: at once when moveToward is 0 (PathPos_PlaceNear), else by one step
 * toward it (PathPos_MoveToward); segs receives the path's segments.
 * Returns their result; -1 when ctl->area is NULL. Seen: CamRail_Update
 * passes the player's position, moveToward 1 while the area is unchanged.
 */
s32 CamCtl_PathPlace(CamCtl *ctl, PathSeg *segs, f32 *pos, s32 moveToward)
{
    CamArea *area;
    s32 ret;

    area = ctl->area;
    if (area == NULL) {
        return -1;
    }
    if (moveToward == 0) {
        ret = PathPos_PlaceNear(&ctl->pathPos, &area->path.d, segs, pos);
    } else {
        ret = PathPos_MoveToward(&ctl->pathPos, &area->path.d, segs, pos);
    }
    return ret;
}

/*
 * out = ((c[0] t + c[1]) t + c[2]) t + c[3]: the point of segment seg at
 * t. Path_BuildSpline's segments take t from 0 to the key's len (seen:
 * PathPos.dist; Path_PointAt's t times len).
 */
void Vec_EvalCubic(f32 *out, PathSeg *seg, f32 frac)
{
    Vec_Scale(out, seg->c[0], frac);
    Vec_Add(out, out, seg->c[1]);
    Vec_Scale(out, out, frac);
    Vec_Add(out, out, seg->c[2]);
    Vec_Scale(out, out, frac);
    Vec_Add(out, out, seg->c[3]);
}

/* lhs . rhs in the x-z plane. */
static f32 PathVec_DotXZ(f32 *lhs, f32 *rhs)
{
    return lhs[2] * rhs[2] + lhs[0] * rhs[0];
}

/* lhs . rhs. */
static f32 PathVec_Dot(f32 *lhs, f32 *rhs)
{
    return lhs[0] * rhs[0] + lhs[1] * rhs[1] + lhs[2] * rhs[2];
}

f32 Math_Absf(f32 x);
void Quintic_Roots(Complex *roots, f32 *p);

s32 Cubic_Roots(f32 *out, f32 *c);

#define LEN2(v) ((v)[0] * (v)[0] + (v)[1] * (v)[1] + (v)[2] * (v)[2])

/*
 * The parameters t where segment seg comes nearest target: the real roots
 * of the derivative of the squared distance (halved), into cands; returns
 * how many (0 to 5). axes 0 measures in the x-z plane (PathVec_DotXZ),
 * else in 3D (PathVec_Dot). Coefficients with a squared length up to
 * 1e-10 (always in 3D) lower the degree: a quintic (Quintic_Roots, the
 * roots with an imaginary part under 0.001 kept), a cubic (Cubic_Roots)
 * or a line (one root); 0 when the segment is a point.
 */
s32 PathSeg_Nearest(f32 *cands, PathSeg *seg, f32 *target, s32 axes)
{
    f32 coefs[6];
    Complex roots[5];
    f32 offset[3];
    f32 point[3];
    f32 termA;
    f32 termB;
    s32 count;
    u32 i;

    Vec_Copy(point, target);
    if (axes == 0) {
        point[1] = 0.0f;
        Vec_Diff(offset, seg->c[3], point);
        if (LEN2(seg->c[0]) <= 1e-10f) {
            if (LEN2(seg->c[1]) <= 1e-10f) {
                if (LEN2(seg->c[2]) <= 1e-10f) {
                    return 0;
                }
                termA = PathVec_DotXZ(seg->c[2], seg->c[2]);
                cands[0] = -(PathVec_DotXZ(seg->c[2], offset) / termA);
                return 1;
            }
            coefs[0] = 2.0f * PathVec_DotXZ(seg->c[1], seg->c[1]);
            coefs[1] = 3.0f * PathVec_DotXZ(seg->c[1], seg->c[2]);
            termA = 2.0f * PathVec_DotXZ(seg->c[1], offset);
            coefs[2] = termA + PathVec_DotXZ(seg->c[2], seg->c[2]);
            coefs[3] = PathVec_DotXZ(seg->c[2], offset);
            return Cubic_Roots(cands, coefs);
        }
        coefs[0] = 3.0f * PathVec_DotXZ(seg->c[0], seg->c[0]);
        coefs[1] = 5.0f * PathVec_DotXZ(seg->c[0], seg->c[1]);
        termA = 4.0f * PathVec_DotXZ(seg->c[0], seg->c[2]);
        coefs[2] = termA + 2.0f * PathVec_DotXZ(seg->c[1], seg->c[1]);
        termA = PathVec_DotXZ(seg->c[1], seg->c[2]);
        termB = PathVec_DotXZ(seg->c[0], offset);
        coefs[3] = 3.0f * (termB + termA);
        termA = 2.0f * PathVec_DotXZ(seg->c[1], offset);
        coefs[4] = termA + PathVec_DotXZ(seg->c[2], seg->c[2]);
        coefs[5] = PathVec_DotXZ(seg->c[2], offset);
        Quintic_Roots(roots, coefs);
        for (i = count = 0; i < 5; i++) {
            if (Math_Absf(roots[i].y) < 0.001f) {
                count++;
                *cands++ = roots[i].x;
            }
        }
    } else {
        Vec_Diff(offset, seg->c[3], point);
        if (LEN2(seg->c[0]) <= 1e-10f) {
            if (LEN2(seg->c[1]) <= 1e-10f) {
                if (LEN2(seg->c[2]) <= 1e-10f) {
                    return 0;
                }
                termA = PathVec_Dot(seg->c[2], seg->c[2]);
                cands[0] = -(PathVec_Dot(seg->c[2], offset) / termA);
                return 1;
            }
            coefs[0] = 2.0f * PathVec_Dot(seg->c[1], seg->c[1]);
            coefs[1] = 3.0f * PathVec_Dot(seg->c[1], seg->c[2]);
            termA = 2.0f * PathVec_Dot(seg->c[1], offset);
            coefs[2] = termA + PathVec_Dot(seg->c[2], seg->c[2]);
            coefs[3] = PathVec_Dot(seg->c[2], offset);
            return Cubic_Roots(cands, coefs);
        }
        coefs[0] = 3.0f * PathVec_Dot(seg->c[0], seg->c[0]);
        coefs[1] = 5.0f * PathVec_Dot(seg->c[0], seg->c[1]);
        termA = 4.0f * PathVec_Dot(seg->c[0], seg->c[2]);
        coefs[2] = termA + 2.0f * PathVec_Dot(seg->c[1], seg->c[1]);
        termA = PathVec_Dot(seg->c[1], seg->c[2]);
        termB = PathVec_Dot(seg->c[0], offset);
        coefs[3] = 3.0f * (termB + termA);
        termA = 2.0f * PathVec_Dot(seg->c[1], offset);
        coefs[4] = termA + PathVec_Dot(seg->c[2], seg->c[2]);
        coefs[5] = PathVec_Dot(seg->c[2], offset);
        Quintic_Roots(roots, coefs);
        for (i = count = 0; i < 5; i++) {
            if (Math_Absf(roots[i].y) < 0.001f) {
                count++;
                *cands++ = roots[i].x;
            }
        }
    }
    return count;
}

/*
 * The fov's scale for area at distance dist (seen: the eye's distance from
 * the target, CamRail_Update): fovScale[0] up to fovDist[0], fovScale[1]
 * from fovDist[1], linear between. The mean of the two for equal
 * distances is reached only by a NaN dist (the tests before catch the
 * rest).
 */
f32 CamArea_FovScale(CamArea *area, f32 dist)
{
    f32 scale;
    f32 farDist;
    f32 nearDist;

    nearDist = area->fovDist[0];
    if (dist <= nearDist) {
        return area->fovScale[0];
    }
    farDist = area->fovDist[1];
    if (!(dist < farDist)) {
        return area->fovScale[1];
    }
    if (farDist == nearDist) {
        return 0.5f * (area->fovScale[0] + area->fovScale[1]);
    }
    scale = (area->fovScale[1] - area->fovScale[0]) * (dist - nearDist);
    scale /= farDist - nearDist;
    scale += area->fovScale[0];
    return scale;
}

/*
 * path->vals[keyNo] blended with vals[keyNo + 1] by frac (0 to 1). Seen:
 * CamRail_Update's fov on a path, at pathPos.k and pathPos.t.
 */
f32 PathKey_Blend(PathData *path, s32 keyNo, f32 frac)
{
    return path->vals[keyNo] * (1.0f - frac) + path->vals[keyNo + 1] * frac;
}

/*
 * path->angles[keyNo] blended with angles[keyNo + 1] by frac, in radians
 * (ANGLE16_TO_RAD): the difference times frac in 16.16 fixed point, whose
 * upper half (through prodHigh) gets angles[keyNo] added, so the result
 * wraps as a 16-bit angle. Seen: CamRail_Update's roll on a path.
 */
f32 PathKey_BlendAngle(PathData *path, s32 keyNo, f32 frac)
{
    s32 prod;
    s16 *prodHigh = (s16 *)&prod + 1;

    prod = (s32)(65536.0f * frac) * (path->angles[keyNo + 1] - path->angles[keyNo]);
    *prodHigh += path->angles[keyNo];
    return ANGLE16_TO_RAD * *prodHigh;
}

/*
 * Solves the tridiagonal system sub[i] sol[i-1] + diag[i] sol[i] +
 * super[i] sol[i+1] = rhs[i] for i < count (count <= 64) into sol (the
 * Thomas algorithm); a zero pivot is kept as 0 instead of inverted.
 */
static void Tridiag_Solve(f32 *sol, f32 *sub, f32 *diag, f32 *super, f32 *rhs, s32 count)
{
    f32 gamma[64];
    f32 invPivot;
    s32 i;

    invPivot = diag[0];
    if (invPivot != 0.0f) {
        invPivot = 1.0f / invPivot;
    }
    sol[0] = rhs[0] * invPivot;
    for (i = 1; i < count; i++) {
        gamma[i - 1] = invPivot * super[i - 1];
        invPivot = diag[i] - sub[i] * gamma[i - 1];
        if (invPivot != 0.0f) {
            invPivot = 1.0f / invPivot;
        }
        sol[i] = invPivot * (rhs[i] - sub[i] * sol[i - 1]);
    }
    for (i = count - 2; i >= 0; i--) {
        sol[i] -= gamma[i] * sol[i + 1];
    }
}

/*
 * Builds the keyCount - 1 cubic segments through the path keys into seg:
 * a cubic Hermite spline whose parameter runs from 0 to each key's len,
 * with natural ends. mat's rows 0-2 hold the tridiagonal system (sub,
 * diag, super), 3-5 the right sides for x, y and z, 6-8 the tangents
 * Tridiag_Solve finds, one axis at a time. Segment i: c[3] the key's pos,
 * c[2] its tangent, c[1] and c[0] from the next key and its tangent.
 */
void Path_BuildSpline(PathSeg *seg, PathKey *keys, s32 keyCount)
{
    f32 mat[9][16]; /* rows 0-2 the system (a, b, c), 3-5 the right sides and 6-8 the tangents (x, y, z) */
    f32 *col;
    s32 i;
    f32 ratioIn;
    f32 ratioOut;
    f32 invLen;
    f32 invLenSq;
    f32 invLenCube;

    col = mat[0];
    col[0] = 0.0f;
    col[1 * 16] = keys[0].len + keys[0].len;
    col[2 * 16] = keys[0].len;
    col[3 * 16] = 3.0f * (keys[1].pos[0] - keys[0].pos[0]);
    col[4 * 16] = 3.0f * (keys[1].pos[1] - keys[0].pos[1]);
    col[5 * 16] = 3.0f * (keys[1].pos[2] - keys[0].pos[2]);
    for (i = 1; i < keyCount - 1; i++) {
        ratioIn = keys[i].len / keys[i - 1].len;
        ratioOut = keys[i - 1].len / keys[i].len;
        col[i] = keys[i].len;
        col[1 * 16 + i] = 2.0f * (keys[i - 1].len + keys[i].len);
        col[2 * 16 + i] = keys[i - 1].len;
        col[3 * 16 + i] = 3.0f * (ratioIn * (keys[i].pos[0] - keys[i - 1].pos[0]) + ratioOut * (keys[i + 1].pos[0] - keys[i].pos[0]));
        col[4 * 16 + i] = 3.0f * (ratioIn * (keys[i].pos[1] - keys[i - 1].pos[1]) + ratioOut * (keys[i + 1].pos[1] - keys[i].pos[1]));
        col[5 * 16 + i] = 3.0f * (ratioIn * (keys[i].pos[2] - keys[i - 1].pos[2]) + ratioOut * (keys[i + 1].pos[2] - keys[i].pos[2]));
    }
    col[keyCount - 1] = keys[keyCount - 2].len;
    col[1 * 16 + keyCount - 1] = keys[keyCount - 2].len + keys[keyCount - 2].len;
    col[2 * 16 + keyCount - 1] = 0.0f;
    col[3 * 16 + keyCount - 1] = 3.0f * (keys[keyCount - 1].pos[0] - keys[keyCount - 2].pos[0]);
    col[4 * 16 + keyCount - 1] = 3.0f * (keys[keyCount - 1].pos[1] - keys[keyCount - 2].pos[1]);
    col[5 * 16 + keyCount - 1] = 3.0f * (keys[keyCount - 1].pos[2] - keys[keyCount - 2].pos[2]);
    Tridiag_Solve(col + 6 * 16, col, col + 16, col + 2 * 16, col + 3 * 16, keyCount);
    Tridiag_Solve(col + 7 * 16, col, col + 16, col + 2 * 16, col + 4 * 16, keyCount);
    Tridiag_Solve(col + 8 * 16, col, col + 16, col + 2 * 16, col + 5 * 16, keyCount);
    for (i = 0; i < keyCount - 1; i++) {
        invLen = 1.0f / keys->len;
        invLenSq = invLen * invLen;
        invLenCube = 1.0f / Math_Pow(keys->len, 3.0f);
        seg->c[0][0] = invLenSq * (col[6 * 16] + col[6 * 16 + 1]) + invLenCube * (2.0f * (keys[0].pos[0] - keys[1].pos[0]));
        seg->c[1][0] = invLenSq * (3.0f * (keys[1].pos[0] - keys[0].pos[0])) - invLen * (2.0f * col[6 * 16] + col[6 * 16 + 1]);
        seg->c[2][0] = col[6 * 16];
        seg->c[3][0] = keys[0].pos[0];
        seg->c[0][1] = invLenSq * (col[7 * 16] + col[7 * 16 + 1]) + invLenCube * (2.0f * (keys[0].pos[1] - keys[1].pos[1]));
        seg->c[1][1] = invLenSq * (3.0f * (keys[1].pos[1] - keys[0].pos[1])) - invLen * (2.0f * col[7 * 16] + col[7 * 16 + 1]);
        seg->c[2][1] = col[7 * 16];
        seg->c[3][1] = keys[0].pos[1];
        seg->c[0][2] = invLenSq * (col[8 * 16] + col[8 * 16 + 1]) + invLenCube * (2.0f * (keys[0].pos[2] - keys[1].pos[2]));
        seg->c[1][2] = invLenSq * (3.0f * (keys[1].pos[2] - keys[0].pos[2])) - invLen * (2.0f * col[8 * 16] + col[8 * 16 + 1]);
        seg->c[2][2] = col[8 * 16];
        seg->c[3][2] = keys[0].pos[2];
        col++;
        keys++;
        seg++;
    }
}

/* out = x + iy. */
static void Cplx_Set(Complex *out, f32 x, f32 y)
{
    out->x = x;
    out->y = y;
}

/* out = lhs - rhs as complex numbers. */
static void Cplx_Sub(Complex *out, Complex *lhs, Complex *rhs)
{
    out->x = lhs->x - rhs->x;
    out->y = lhs->y - rhs->y;
}

/*
 * out = lhs * rhs as complex numbers (lhs turned by rhs's angle and
 * scaled by its length); out may be lhs or rhs.
 */
static void Cplx_Mul(Complex *out, Complex *lhs, Complex *rhs)
{
    Complex prod;

    prod.x = lhs->x * rhs->x - lhs->y * rhs->y;
    prod.y = lhs->x * rhs->y + lhs->y * rhs->x;
    *out = prod;
}

/*
 * out = num / den as complex numbers; num itself when den is 0. out may
 * be num or den.
 */
static void Cplx_Div(Complex *out, Complex *num, Complex *den)
{
    Complex quot;
    f32 numRe;
    f32 numIm;
    f32 denIm;
    f32 denRe;
    f32 denNorm;

    denRe = den->x;
    denIm = den->y;
    denNorm = denRe * denRe + denIm * denIm;
    if (0.0f != denNorm) {
        denNorm = 1.0f / denNorm;
        numRe = num->x;
        numIm = num->y;
        quot.x = denNorm * (numRe * denRe + numIm * denIm);
        quot.y = denNorm * (numIm * denRe - numRe * denIm);
        *out = quot;
    } else {
        *out = *num;
    }
}

f32 Math_Absf(f32 x);
extern f32 gQuinticBoundExps[];
extern f32 gQuinticStart0[];
extern f32 gQuinticStart0Y;
extern f32 gQuinticStart1[];
extern f32 gQuinticStart1Y;
extern f32 gQuinticStart2[];
extern f32 gQuinticStart2Y;
extern f32 gQuinticStart3[];
extern f32 gQuinticStart3Y;
extern f32 gQuinticStart4[];
extern f32 gQuinticStart4Y[];

/*
 * The five complex roots of coefs[0] x^5 + ... + coefs[5] into roots
 * (Durand-Kerner). The coefficients are divided by coefs[0] (monic[1..5]).
 * The start points are gQuinticStart0..4 (x and y) scaled by 5 times the
 * largest abs(monic[i]) ^ gQuinticBoundExps[i - 2], i 2 to 5 (monic[1] is
 * not used). Each of the 26 passes moves every root in place: it
 * subtracts P(root) over the product of its differences to the others.
 */
void Quintic_Roots(Complex *roots, f32 *coefs)
{
    Complex den;
    Complex val;
    Complex est;
    Complex step;
    Complex diff;
    f32 monic[6];
    s32 i;
    f32 *boundExp;
    f32 *boundCoef;
    f32 invLead;
    f32 bound;
    f32 term;
    s32 k;
    s32 pass;
    Complex *root;
    f32 *monicCoef;
    Complex *other;
    s32 j;

    invLead = 1.0f / coefs[0];
    for (i = 1; i < 6; i++) {
        monic[i] = invLead * coefs[i];
    }
    bound = 0.0f;
    for (i = 2, boundExp = gQuinticBoundExps, boundCoef = &monic[2]; i <= 5; i++, boundExp++, boundCoef++) {
        term = Math_Pow(Math_Absf(*boundCoef), *boundExp);
        if (!(term <= bound)) {
            bound = term;
        }
    }
    bound *= 5.0f;
    roots[0].x = bound * gQuinticStart0[0];
    roots[0].y = bound * gQuinticStart0Y;
    roots[1].x = bound * gQuinticStart1[0];
    roots[1].y = bound * gQuinticStart1Y;
    roots[2].x = bound * gQuinticStart2[0];
    roots[2].y = bound * gQuinticStart2Y;
    roots[3].x = bound * gQuinticStart3[0];
    roots[3].y = bound * gQuinticStart3Y;
    roots[4].x = bound * gQuinticStart4[0];
    roots[4].y = bound * gQuinticStart4Y[0];
    for (pass = 25; pass >= 0; pass--) {
        for (j = 0, root = roots; j < 5; j++, root++) {
            Cplx_Set(&den, 1.0f, 0.0f);
            Cplx_Set(&val, 1.0f, 0.0f);
            est = *root;
            for (k = 0, monicCoef = monic, other = roots; k < 5; k++, monicCoef++, other++) {
                Cplx_Mul(&val, &val, &est);
                val.x += monicCoef[1];
                if (k != j) {
                    Cplx_Sub(&diff, &est, other);
                    Cplx_Mul(&den, &den, &diff);
                }
            }
            Cplx_Div(&step, &val, &den);
            Cplx_Sub(root, &est, &step);
        }
    }
}

f32 Math_Sqrtf(f32 x);
f32 Math_Cos(f32 x);

/*
 * The real roots of coefs[0] x^3 + coefs[1] x^2 + coefs[2] x + coefs[3]
 * into out (Cardano: shift the cubic by shift, then depP and depQ);
 * returns how many: 1 (a triple root, or one real root), 2 (a double
 * root) or 3 (the trigonometric form).
 */
s32 Cubic_Roots(f32 *out, f32 *coefs)
{
    f32 invLead;
    f32 shift;
    f32 depP;
    f32 depQ;
    f32 disc;
    f32 depPSq;
    f32 termU;
    f32 termV;
    f32 quadCoef;
    f32 linCoef;

    invLead = 1.0f / coefs[0];
    quadCoef = coefs[1] * invLead;
    linCoef = coefs[2] * invLead;
    shift = 0.33333334f * quadCoef;
    depP = 0.33333334f * linCoef - shift * shift;
    depQ = 0.5f * (coefs[3] * invLead + shift * (shift * shift + shift * shift - linCoef));
    if (Math_Absf(depP) < 1e-6f && Math_Absf(depQ) < 1e-6f) {
        out[0] = -shift;
        return 1;
    }
    depPSq = depP * depP;
    disc = depPSq * depP + depQ * depQ;
    if (!(disc <= 0.0f)) {
        if (depQ >= 0.0f) {
            termU = Math_Pow(depQ + Math_Sqrtf(disc), 0.33333334f);
        } else {
            termU = -Math_Pow(-depQ + Math_Sqrtf(disc), 0.33333334f);
        }
        termV = depP / termU;
        if (depP < 0.0f) {
            out[0] = termV - termU;
        } else {
            termV = termU * termU;
            out[0] = -2.0f * depQ * termV / (depPSq + termV * (termV + depP));
        }
        out[0] -= shift;
        return 1;
    }
    if (depQ < 0.0f) {
        depP = Math_Sqrtf(-depP);
    } else {
        depP = -Math_Sqrtf(-depP);
    }
    if (Math_Absf(disc) < 1e-6f) {
        out[0] = depP + depP - shift;
        out[1] = -depP - shift;
        return 2;
    }
    depQ = 0.33333334f * Math_Atan2(Math_Sqrtf(-disc), -depQ);
    termU = depP * Math_Cos(depQ);
    termV = 1.7320508f * (depP * Math_Sin(depQ));
    out[0] = termU + termU - shift;
    out[1] = -termU - termV - shift;
    out[2] = -termU + termV - shift;
    return 3;
}

s32 CamCtl_GetMode(void);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
f32 Vec_Distance(f32 *a, f32 *b);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 AreaGrid_SweepCells(f32 *from, f32 *to, f32 *out, u16 mask, f32 r);
extern f32 gCamCollideSphere[];

/*
 * Pushes the point fromPos out of the area's walls along the line from
 * toPos: sweeps a sphere of radius gCamCollideSphere[3] from toPos to
 * fromPos, both
 * offset by gCamCollideSphere[0..2] (AreaGrid_SweepCells, skipping the
 * triangle flags CAM_LINE_MASK), which resolves the hits into fromPos. *dist gets
 * how far fromPos moved, 0 when nothing is hit. Returns 0; does nothing
 * while CamCtl.mode has CAMCTL_NO_LINE_TEST. The sweep's result
 * is read as u8 (as this unit's code takes it), so its -1 (no segment)
 * counts as a hit too. The comma in the subscript gCamCollideSphere[0, 0]
 * gives the game's float register order. Seen: CamFollow_Update
 * passes the eye, CamFollow.unk18 and &hitDist.
 */
f32 Cam_LineHitDist(f32 *fromPos, f32 *toPos, f32 *dist)
{
    f32 sweepFrom[3];
    f32 sweepTo[3];
    f32 fromStart[3];

    if (CamCtl_GetMode() & CAMCTL_NO_LINE_TEST) {
        return 0.0f;
    }
    /* FAKEMATCH: the comma in the subscript gives the offsets the
     * game's float registers (f1, f3, f2); every plain spelling gives f3, f2, f1. */
    sweepFrom[0] = toPos[0] + gCamCollideSphere[0, 0];
    sweepFrom[1] = toPos[1] + gCamCollideSphere[1];
    sweepFrom[2] = toPos[2] + gCamCollideSphere[2];
    sweepTo[0] = fromPos[0] + gCamCollideSphere[0];
    sweepTo[1] = fromPos[1] + gCamCollideSphere[1];
    sweepTo[2] = fromPos[2] + gCamCollideSphere[2];
    Vec_Set(fromStart, fromPos[0], fromPos[1], fromPos[2]);
    if ((u8)AreaGrid_SweepCells(sweepFrom, sweepTo, fromPos, CAM_LINE_MASK, gCamCollideSphere[3]) > 0) {
        *dist = Vec_Distance(fromStart, fromPos);
    } else {
        *dist = 0.0f;
    }
    return 0.0f;
}

s32 CamCtl_GetMode(void);
s32 BoneHit_Place(ObjHead *o, HitShape *def, HitCapsule *c, HitSphere *s);
void Capsule_Copy(struct HitCapsule *src, struct Capsule *dst);
s32 Sphere_PushOut(f32 ra, f32 rb, f32 *a, f32 *b, f32 *out);
s32 Capsule_PushSphere(struct Capsule *cap, f32 *p, f32 *out, f32 r);
void HitList_Next(Em *p, HitShape **cur);
f32 AreaGrid2_GetFloorY(f32 *pos);
extern HitShape *gEmBoneHits[];
extern HitShape *gEmBoneHitsLobby[];

/* Adds push v to the push acc: the larger one when their signs differ, else their sum. */
#define PUSH_ADD(acc, v)                                                                      \
    if ((!(acc < 0.0f) && v < 0.0f) || (acc < 0.0f && !(v < 0.0f)) ||                         \
        (!(acc < -0.0f) && v < -0.0f) || (acc < -0.0f && !(v < -0.0f))) {                     \
        if (Math_Absf(acc) < Math_Absf(v)) {                                                  \
            acc = -v;                                                                         \
        }                                                                                     \
    } else {                                                                                  \
        acc -= v;                                                                             \
    }

/*
 * Pushes the point pos out of the hit shapes of the monsters in the
 * current area: each of the EM_COUNT slots that is active, a monster
 * (isEm), visible and in gScene.area tests a sphere at pos +
 * gCamCollideSphere[0..2] of radius gCamCollideSphere[3] against the
 * shapes of its kind's list (gEmBoneHits, or the online game's copy).
 * It skips the shapes with atkFlags 4, atkFlags2 2, openFlags 8 or 2, or
 * offFlags 2 (no camera push), steps over group headers (HitList_Next),
 * pushes from spheres with Sphere_PushOut and from capsules with
 * Capsule_PushSphere. Each axis collects the negated pushes (PUSH_ADD):
 * of two in the same direction the larger counts, opposite ones add up.
 * pos then moves by the sum and stays at least 10 above the floor
 * (AreaGrid2_GetFloorY). Every shape is tested from the starting pos.
 * Nothing while CamCtl.mode has CAMCTL_NO_EM_PUSH. unused1 is
 * not read (seen: CamFollow.unk18).
 */
void Cam_PushOutOfEms(f32 *pos, f32 *unused1)
{
    f32 cap[8];
    f32 sph[4];
    f32 work[16];
    f32 probe[3];
    f32 push[3];
    HitShape *shape;
    Em *em;
    f32 *sphere;
    s32 i;
    s32 boneNo;
    f32 minY;
    f32 pushZ;
    f32 pushY;
    f32 pushX;

    if (CamCtl_GetMode() & CAMCTL_NO_EM_PUSH) {
        return;
    }
    em = gEms;
    sphere = gCamCollideSphere;
    pushX = pushY = pushZ = 0.0f;
    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active != 0 && em->isEm != 0 && em->visible != 0 && gScene.area == em->area) {
            probe[0] = pos[0] + sphere[0];
            probe[1] = pos[1] + sphere[1];
            probe[2] = pos[2] + sphere[2];
            if (gScene.inLobby == 0) {
                shape = gEmBoneHits[em->type];
            } else {
                shape = gEmBoneHitsLobby[em->type];
            }
            while ((boneNo = shape->bone) != HIT_BONE_END) {
                if ((shape->atkFlags & 4) || (shape->atkFlags2 & 2) || (shape->openFlags & 8) || (shape->openFlags & 2) || (shape->offFlags & 2)) {
                    shape++;
                    continue;
                }
                if (boneNo == HIT_BONE_GROUP) {
                    HitList_Next(em, &shape);
                    continue;
                }
                switch ((s16)BoneHit_Place((ObjHead *)em, shape, (struct HitCapsule *) cap, (struct HitSphere *) sph)) {
                case 0:
                    if (Sphere_PushOut(sph[3], sphere[3], sph, probe, push) != 0) {
                        PUSH_ADD(pushX, push[0]);
                        PUSH_ADD(pushY, push[1]);
                        PUSH_ADD(pushZ, push[2]);
                    }
                    break;
                case 1:
                    Capsule_Copy((struct HitCapsule *) cap, (struct Capsule *) work);
                    if (Capsule_PushSphere((struct Capsule *) work, probe, push, sphere[3]) != 0) {
                        PUSH_ADD(pushX, push[0]);
                        PUSH_ADD(pushY, push[1]);
                        PUSH_ADD(pushZ, push[2]);
                    }
                    break;
                }
                shape++;
            }
        }
    }
    if (pushX != 0.0f || pushY != 0.0f || pushZ != 0.0f) {
        pos[0] += pushX;
        pos[1] += pushY;
        pos[2] += pushZ;
        minY = 10.0f + AreaGrid2_GetFloorY(pos);
        if (pos[1] < minY) {
            pos[1] = minY;
        }
    }
}

u16 Area_DayId(u16 a);
u8 MonsterGroup_First(u8 id);
void CamReq_Start(s32 req, void *arg);
void CamReq_Change(s32 req, void *arg);
extern u8 gQuestTargetCamIds[];

/*
 * Starts the camera request that shows the quest's monster: CamReq_Start
 * when flag is 0 (CamReq_TargetDown), else CamReq_Change
 * (CamReq_BossHunted), with the monster as the request's object. Nothing
 * when the latest done goal (gQuest.goalIdx, from the end) is not a
 * monster goal (QUEST_GOAL_MONSTER) or not of downEm's kind. The monster
 * is gQuest.downEm, else gQuest.spawnedEm when that is a Lao-Shan Lung,
 * Fatalis or Shen Gaoren kind; it must be in the current area. Request
 * CAM_REQ_LAO_SHAN_DOWN for the Lao-Shan Lungs in AREA_FORTRESS_5 (by Area_DayId), CAM_REQ_FATALIS_DOWN
 * for the Fatalis kinds, CAM_REQ_SHEN_GAOREN_DOWN for Shen Gaoren, CAM_REQ_ELDER_DOWN for Kushala Daora, its
 * rusted kind, Chameleos, Lunastra and Teostra, each only when no monster
 * is left (emLeft 0). The other kinds take gQuestTargetCamIds by family
 * (MonsterGroup_First), none for CAM_REQ_NONE; Plesioth and Green Plesioth get
 * none in EM_GROUP_DEFEAT state 2.
 */
void CamReq_QuestTarget(s32 flag)
{
    Em *em;
    s32 i;
    u8 goalNo;
    s32 req;

    for (i = 2; i >= 0; i--) {
        if (gQuest.goalIdx[i] != QUEST_GOAL_NONE) {
            goalNo = gQuest.goalIdx[i];
            if (!(gQuest.goals[goalNo].flags & QUEST_GOAL_MONSTER)) {
                return;
            }
            if (gQuest.downEm != NULL && gQuest.downEm->type != gQuest.goals[goalNo].target) {
                return;
            }
            break;
        }
    }
    em = gQuest.downEm;
    if (em == NULL) {
        em = gQuest.spawnedEm;
        if (em == NULL) {
            return;
        }
        if (em->type != EM_KIND_LAO_SHAN_LUNG && em->type != EM_KIND_ASHEN_LAO_SHAN_LUNG && em->type != EM_KIND_FATALIS && em->type != EM_KIND_FATALIS_B && em->type != EM_KIND_FATALIS_C && em->type != EM_KIND_SHEN_GAOREN) {
            return;
        }
    }
    if (gScene.area != em->area) {
        return;
    }
    switch (em->type) {
    case EM_KIND_LAO_SHAN_LUNG:
    case EM_KIND_ASHEN_LAO_SHAN_LUNG:
        if (Area_DayId(gScene.area) != AREA_FORTRESS_5) {
            return;
        }
        if (gQuest.emLeft == 0) {
            req = CAM_REQ_LAO_SHAN_DOWN;
        } else {
            return;
        }
        break;
    case EM_KIND_FATALIS:
    case EM_KIND_FATALIS_B:
    case EM_KIND_FATALIS_C:
        if (gQuest.emLeft == 0) {
            req = CAM_REQ_FATALIS_DOWN;
        } else {
            return;
        }
        break;
    case EM_KIND_SHEN_GAOREN:
        if (gQuest.emLeft == 0) {
            req = CAM_REQ_SHEN_GAOREN_DOWN;
        } else {
            return;
        }
        break;
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_CHAMELEOS:
        if (gQuest.emLeft == 0) {
            req = CAM_REQ_ELDER_DOWN;
        } else {
            return;
        }
        break;
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        if (gQuest.emLeft == 0) {
            req = 7;
        } else {
            return;
        }
        break;
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
        if (em->stateGroup == EM_GROUP_DEFEAT && em->state == 2) {
            return;
        }
    default:
        req = gQuestTargetCamIds[MonsterGroup_First(em->type)];
        if (req == CAM_REQ_NONE) {
            return;
        }
        break;
    }
    if (flag == 0) {
        CamReq_Start(req, em);
    } else {
        CamReq_Change(req, em);
    }
}

/* This file's initialised data. */
void CamFollow_Init(CamCtl *c, CamView *v);
void CamFollow_Update(CamCtl *c, CamView *v);
void CamRail_Init(CamCtl *c, CamView *v);
void CamRail_Update(CamCtl *c, CamView *v);
void CamAim_Init(CamCtl *c, CamView *v);
void CamAim_Update(CamCtl *c, CamView *v);
void CamFixed_Init(CamCtl *c, CamView *v);
void CamFixed_Update(CamCtl *c, CamView *v);
void CamScript_Init(CamCtl *c, CamView *v);
void CamScript_Update(CamCtl *c, CamView *v);
void CamOrbit_Init(CamCtl *c, CamView *v);
void CamOrbit_Update(CamCtl *c, CamView *v);
u32 gDmcamFileIds_1[92] = {
#include "sub_main/004187B0.inc"
};
u32 gDmcamFileIds_2[20] = {
#include "sub_main/00418920.inc"
};
u32 gDmcamFileIds_3[20] = {
#include "sub_main/00418970.inc"
};
u32 gDmcamFileIds_4[20] = {
#include "sub_main/004189C0.inc"
};
u32 gDmcamFileIds_5[84] = {
#include "sub_main/00418A10.inc"
};
u32 gDmcamFileIds_6[52] = {
#include "sub_main/00418B60.inc"
};
u32 gDmcamFileIds_7[76] = {
#include "sub_main/00418C30.inc"
};
u32 gDmcamFileIds_8[100] = {
#include "sub_main/00418D60.inc"
};
u32 gDmcamFileIds_9[100] = {
#include "sub_main/00418EF0.inc"
};
u32 gDmcamFileIds_10[36] = {
#include "sub_main/00419080.inc"
};
u32 gDmcamFileIds_11[112] = {
#include "sub_main/00419110.inc"
};
u32 gDmcamFileIds_12[68] = {
#include "sub_main/004192D0.inc"
};
u32 gDmcamFileIds_13[68] = {
#include "sub_main/004193E0.inc"
};
u32 gDmcamFileIds_14[104] = {
#include "sub_main/004194F0.inc"
};
u32 gDmcamFileIds_15[52] = {
#include "sub_main/00419690.inc"
};
u32 gDmcamFileIds_16[68] = {
#include "sub_main/00419760.inc"
};
u32 gDmcamFileIds_17[68] = {
#include "sub_main/00419870.inc"
};
u32 gDmcamFileIds_18[68] = {
#include "sub_main/00419980.inc"
};
u32 gDmcamFileIds_19[84] = {
#include "sub_main/00419A90.inc"
};
u32 gDmcamFileIds_20[100] = {
#include "sub_main/00419BE0.inc"
};
u32 gDmcamFileIds_21[152] = {
#include "sub_main/00419D70.inc"
};
u32 gDmcamFileIds_22[148] = {
#include "sub_main/00419FD0.inc"
};
u32 gDmcamFileIds_23[144] = {
#include "sub_main/0041A220.inc"
};
u32 gDmcamFileIds_24[20] = {
#include "sub_main/0041A460.inc"
};
u32 gDmcamFileIds_25[40] = {
#include "sub_main/0041A4B0.inc"
};
u32 gDmcamFileIds_26[20] = {
#include "sub_main/0041A550.inc"
};
u32 gDmcamFileIds_27[20] = {
#include "sub_main/0041A5A0.inc"
};
u32 gDmcamFileIds_28[20] = {
#include "sub_main/0041A5F0.inc"
};
u32 gDmcamFileIds_29[20] = {
#include "sub_main/0041A640.inc"
};
u32 gDmcamFileIds_30[20] = {
#include "sub_main/0041A690.inc"
};
u32 gDmcamFileIds_31[20] = {
#include "sub_main/0041A6E0.inc"
};
u32 gDmcamFileIds_32[20] = {
#include "sub_main/0041A730.inc"
};
u32 gDmcamFileIds_33[76] = {
#include "sub_main/0041A780.inc"
};
u32 gDmcamFileIds_34[76] = {
#include "sub_main/0041A8B0.inc"
};
u32 gDmcamFileIds_35[76] = {
#include "sub_main/0041A9E0.inc"
};
u32 gDmcamFileIds_36[68] = {
#include "sub_main/0041AB10.inc"
};
u32 gDmcamFileIds_37[44] = {
#include "sub_main/0041AC20.inc"
};
u32 gDmcamFileIds_38[28] = {
#include "sub_main/0041ACD0.inc"
};
u32 gDmcamFileIds_39[48] = {
#include "sub_main/0041AD40.inc"
};
u32 gDmcamFileIds_40[64] = {
#include "sub_main/0041AE00.inc"
};
u32 gDmcamFileIds_41[48] = {
#include "sub_main/0041AF00.inc"
};
u32 gDmcamFileIds_42[84] = {
#include "sub_main/0041AFC0.inc"
};
u32 gDmcamFileIds_43[72] = {
#include "sub_main/0041B110.inc"
};
u32 gDmcamFileIds_44[48] = {
#include "sub_main/0041B230.inc"
};
u32 gDmcamFileIds_45[64] = {
#include "sub_main/0041B2F0.inc"
};
u32 gDmcamFileIds_46[60] = {
#include "sub_main/0041B3F0.inc"
};
u32 gDmcamFileIds_47[144] = {
#include "sub_main/0041B4E0.inc"
};
u32 gDmcamFileIds_48[44] = {
#include "sub_main/0041B720.inc"
};
u32 gDmcamFileIds_49[72] = {
#include "sub_main/0041B7D0.inc"
};
u32 gDmcamFileIds_50[64] = {
#include "sub_main/0041B8F0.inc"
};
u32 gDmcamFileIds_51[24] = {
#include "sub_main/0041B9F0.inc"
};
u32 gDmcamFileIds_52[20] = {
#include "sub_main/0041BA50.inc"
};
u32 gDmcamFileIds_53[40] = {
#include "sub_main/0041BAA0.inc"
};
u32 gDmcamFileIds_54[40] = {
#include "sub_main/0041BB40.inc"
};
u32 gDmcamFileIds_55[80] = {
#include "sub_main/0041BBE0.inc"
};
u32 gDmcamFileIds_56[64] = {
#include "sub_main/0041BD20.inc"
};
u32 gDmcamFileIds_57[24] = {
#include "sub_main/0041BE20.inc"
};
u32 gDmcamFileIds_58[48] = {
#include "sub_main/0041BE80.inc"
};
u32 gDmcamFileIds_59[72] = {
#include "sub_main/0041BF40.inc"
};
u32 gDmcamFileIds_60[44] = {
#include "sub_main/0041C060.inc"
};
u32 gDmcamFileIds_61[48] = {
#include "sub_main/0041C110.inc"
};
u32 gDmcamFileIds_62[24] = {
#include "sub_main/0041C1D0.inc"
};
u32 gDmcamFileIds_63[64] = {
#include "sub_main/0041C230.inc"
};
u32 gDmcamFileIds_64[48] = {
#include "sub_main/0041C330.inc"
};
u32 gDmcamFileIds_65[48] = {
#include "sub_main/0041C3F0.inc"
};
u32 gDmcamFileIds_66[48] = {
#include "sub_main/0041C4B0.inc"
};
u32 gDmcamFileIds_67[26] = {
#include "sub_main/0041C570.inc"
};
u32 gDmcamFileIds_0[2] = {
#include "sub_main/0041C5D8.inc"
};
u32 gDmcamFileIds[140] = {
#include "sub_main/0041C5E0.inc"
};
s16 gCamShakeLengths[8] = {
#include "sub_main/0041C810.inc"
};
f32 gCamShakeRanges[4] = {
#include "sub_main/0041C820.inc"
};
CamDistSet gCamFollowDefaults = {
#include "sub_main/0041C830.inc"
};
CamDist gCamBlockedDists[2] = {
#include "sub_main/0041C930.inc"
};
CamDist gCamFallDist = {
#include "sub_main/0041C970.inc"
};
u32 gCamMode3Offsets[24] = {
#include "sub_main/0041C990.inc"
};
u32 gCamMode4AreaPos[152] = {
#include "sub_main/0041C9F0.inc"
};
u32 gCamMode4AreaFovs[28] = {
#include "sub_main/0041CC50.inc"
};
u32 gAreaCameraIdx[52] = {
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
u32 gCamOrbitExitBlend[2] = {
#include "sub_main/0041CDC8.inc"
};
u32 gCamOrbitEyeOfs[4] = {
#include "sub_main/0041CDD0.inc"
};
u32 gCamOrbitAimOfs[4] = {
#include "sub_main/0041CDE0.inc"
};
f32 gQuinticStart0[2] = {
#include "sub_main/0041CDF0.inc"
};
f32 gQuinticStart1[2] = {
#include "sub_main/0041CDF8.inc"
};
f32 gQuinticStart2[2] = {
#include "sub_main/0041CE00.inc"
};
f32 gQuinticStart3[2] = {
#include "sub_main/0041CE08.inc"
};
f32 gQuinticStart4[12] = {
#include "sub_main/0041CE10.inc"
};
f32 gCamCollideSphere[8] = {
#include "sub_main/0041CE40.inc"
};
u8 gQuestTargetCamIds[80] = {
#include "sub_main/0041CE60.inc"
};
