/*
 * The camera controller's script mode (CAM_MODE_SCRIPT; gCamCtl, CamCtl
 * in camera.h): CamScript_Run starts a camera script (a built-in one, or
 * a dmcam file's) and CamScript_Step runs its commands (CAMCMD_*, each a
 * CamCmd); CamScript_Place sets the view at the move's time, easing each
 * value from its start to its end, with the orbit, the swings and the
 * wall test. The data: the built-in scripts (gDmcamFileIds: a script, or
 * below DMCAM_ID_LIMIT a dmcam file's id).
 */
#include "common.h"
#include "camera.h"
#include "em.h"
#include "pad.h"
#include "player.h"
#include "area.h"
#include "stream.h"
#include "engine_math.h"
#include "rand.h"
#include "vec_math.h"

/* prototypes */
/* end of prototypes */

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

/* unit's own declaration: wider than the definition's u8: with u8, CamScript_Place masks the value before its == 1, which the game's code doesn't */
s32 AreaHit_Raycast(f32 *from, f32 *to, f32 *out, u16 mask);

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
            swingAmp += 3.0637257e-5f * (swingAmp * camScript->swingRand[2] * (f32)((Rand_Next(1) & 0xFF) - 128));
        }
        view->roll += ANGLE16_TO_RAD * swingAmp;
    }
    if (camScript->swing[3].speed != 0) {
        swingAmp = (f32)camScript->swing[3].amp[0] * startWeight + (f32)camScript->swing[3].amp[1] * endWeight;
        swingAmp *= Math_CosAngle(camScript->swing[3].phase);
        camScript->swing[3].phase += camScript->swing[3].speed;
        if (camScript->swingRand[3] != 0) {
            swingAmp += 3.0637257e-5f * (swingAmp * camScript->swingRand[3] * (f32)((Rand_Next(1) & 0xFF) - 128));
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

/* This file's initialised data. */
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
/*
 * Capcom: the dmcam files (DATA.BIN file 6645's) are probably
 * motion\evcam%04d.bin, with motion\evmot%04d.mot beside them: format
 * strings in Monster Hunter Frontier's 2019 mhfo.dll (likely). MHF's PC 1.0
 * install has none of these files, so the pairing and the numbering are
 * unchecked. Long runs of 27 of the 68 built-in scripts above
 * (gDmcamFileIds_*) are in mhfo.dll byte for byte.
 */
u32 *gDmcamFileIds[140] = {
#include "sub_main/0041C5E0.inc"
};
