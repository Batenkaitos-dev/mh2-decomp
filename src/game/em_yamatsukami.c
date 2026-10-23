/*
 * The Yama Tsukami family (EM_KIND_YAMA_TSUKAMI and EM_KIND_YAMA_TSUKAMI_B,
 * include/em_yamatsukami.h): the giant that floats over the fortress. The
 * file starts with the family's curves (EmYamaTsukami_StartCurves,
 * UpdateCurves), its state changes (EmYamaTsukami_ChangeState and its
 * Set<Group>State functions) and its motion sounds, which come before the
 * setup in the game's code. Then the family's setup (EmYamaTsukami_Setup, called by Em_TypeSetup), its start,
 * its states by group (each run by EmYamaTsukami_Run<Group>State from
 * EmYamaTsukami_RunState), its update (EmYamaTsukami_Update: damage, the
 * part flinches, the tower scene), and its effects: the carcass mine point,
 * the tentacles' dust, the fortress impact and the body's swelling
 * (EmYamaTsukamiWork.bodyScale, eased by EmYamaTsukami_EaseToSize and
 * applied to the bones by EmYamaTsukami_ScaleBones). The file ends with the
 * family's data.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "mine_point.h"
#include "em.h"
#include "shadow.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
#include "qsync.h"
#include "em_yamatsukami.h"
#include "bgm.h"
#include "place_rules.h"
#include "camera.h"
#include "capsule.h"
#include "card.h"
#include "save.h"
#include "sound.h"
#include "cam_shake.h"
#include "dust_flash_effects.h"
#include "elder_attacks.h"
#include "engine_math.h"
#include "part_effects.h"
#include "hit_spark.h"
#include "vec_math.h"
#include "em_kind38.h"
#include "em_script.h"




/* prototypes */
u16 Area_IdForCurrentTime(u16);
void * EmEffectAttack_Create(Em *, s32, s32, s32, s32);
extern s8 EmYamaTsukami_TentacleBones[4][2];
void EmBodyFx_Spawn(Em *em, s16 kind);
void GmEm_ResetSize(); /* unit's own declaration: GmEm_ResetSize reads a0 = em (the incoming argument, still in a0); the call passes no argument */
/* The body scale per bodySize. EmYamaTsukami_EaseToSize keeps its (f32 (*)[3]) casts: without them its code doesn't match. */
extern f32 EmYamaTsukami_SizeScales[4][3];
extern f32 EmYamaTsukami_BoneGrowth[8];
extern f32 EmYamaTsukami_BoneGrowthLast;
extern f32 EmYamaTsukami_KindBStartPos[3];
extern EmYamaTsukamiYawKey *EmYamaTsukami_SpinYawKeys[];
static u8 EmYamaTsukami_TurnByKeys(Em *em, EmYamaTsukamiWork *w, s32 idx);
static void EmYamaTsukami_MotionEffectsDefault(Em *em);
EmBone *Em_Bone(void *obj, s16 bone);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
extern f32 gStartPos[];
/* unit's own declaration: group and state are declared u16; with the definition's s32 the callers (EmKind38_ChangeState, EmShakalaka_ChangeState ...) pass the state without the 16-bit mask the game's code applies */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
/* end of prototypes */

/*
 * Starts curve kind (enum EmYamaTsukamiCurve) from its first frame: the
 * speed curves for the surges, the height curve for the others; clears the
 * velocity.
 */
void EmYamaTsukami_StartCurves(Em *em, u8 kind)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case YAMA_CURVE_RISE:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_DESCEND:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_RISE_TO_GOAL:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_RISE_TO_GOAL_END:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_SURGE_FORWARD:
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_SURGE_BACK:
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_SPIN_START:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_SPIN_END:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_DIVE_SLAM:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_9:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_DIE_FALLING:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_SOFT_LAND:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case YAMA_CURVE_RECOVER_RISE:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    }
    Player_ClearVelocity(em);
}

/* The curve that does nothing. */
static s32 EmYamaTsukami_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmYamaTsukami_VelYCurves[];

/*
 * The height curve: sets velY from the two keys of EmYamaTsukami_VelYCurves[curveKind]
 * around the curve frame (the change of height between them, spread over their frames).
 * Returns 2 at the table's end, where velY falls by 1 a frame down to -50 (or, for a
 * table that ends with a negative frame, stops and restarts at frame 0); else 0.
 */
static u8 EmYamaTsukami_CurveVelY(Em *em, EmWork *work)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *keys;
    u8 ret;

    keys = EmYamaTsukami_VelYCurves[((EmYamaTsukamiWork *)work)->curveKind];
    ret = 0;
    if (keys == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame > 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyFrame < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmYamaTsukami_SpeedCurves[];

/*
 * The speed curve: sets velZ from the two keys of EmYamaTsukami_SpeedCurves[curveKind]
 * around the curve frame, as EmYamaTsukami_CurveVelY does for the height. Returns 4 at
 * the table's end, else 0.
 */
static u8 EmYamaTsukami_CurveSpeed(Em *em, EmWork *work)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *keys;
    u8 ret;

    keys = EmYamaTsukami_SpeedCurves[((EmYamaTsukamiWork *)work)->curveKind];
    ret = 0;
    if (keys == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame != 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * Runs the curves EmYamaTsukami_StartCurves turned on, advances the curve frame by the
 * motion's step and, in the air, moves the body by its velocity along its heading
 * (Em_MoveRotatedAccel). Returns the curves' results or'ed together.
 */
u8 EmYamaTsukami_UpdateCurves(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmYamaTsukami_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmYamaTsukami_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmYamaTsukami_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return ret;
}

/* Enters state of group 0: YAMA_IDLE_RECOVER restores the full size. */
void EmYamaTsukami_SetIdleState(Em *em, u16 state, u8 mode)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch ((u16) state) {
    case 4:
        w->bodySize = YAMA_SIZE_FULL;
        break;
    }
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Enters state of group 1. The walks take the distance to the goal (500
 * with none), and YAMA_MOVE_BACKWARD becomes YAMA_MOVE_FORWARD when the
 * goal is within a quarter turn of the facing.
 */
void EmYamaTsukami_SetMoveState(Em *em, u16 state, u8 mode)
{
    s32 turn;

    em->motionSpeed = 1.0f;
    switch (state) {
    case YAMA_MOVE_FORWARD:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case YAMA_MOVE_BACKWARD:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        turn = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= turn && turn <= 16384) || (turn >= 0xC000 && turn <= 0xFFFF)) {
            state = YAMA_MOVE_FORWARD;
        }
        break;
    case YAMA_MOVE_TURN:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Enters state of group 2. The turns become YAMA_FLY_IDLE on the owner when
 * the goal is the target and there is none; the recoveries restore the full
 * size.
 */
void EmYamaTsukami_SetFlyState(Em *em, u16 state, u8 mode)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case YAMA_FLY_TURN:
    case YAMA_FLY_TURN_GUARDED:
        if (em->remote == 0 && em->curGoalKind == 1 && em->curGoalArg == 0 && em->targetNo == -1) {
            state = YAMA_FLY_IDLE;
        }
        break;
    case YAMA_FLY_DRIFT_TO_GOAL:
    case YAMA_FLY_DASH_TO_GOAL:
        break;
    case YAMA_FLY_RECOVER:
        w->bodySize = YAMA_SIZE_FULL;
        break;
    case YAMA_FLY_RECOVER_RISE:
        w->bodySize = YAMA_SIZE_FULL;
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/* Enters state of group 3. */
void EmYamaTsukami_SetAttackState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Enters state of group 4 and moves the flinches on: a fall shrinks the
 * body fully and pauses 30 cycles after it, flinch A leads to B and B to C,
 * and flinch C shrinks the body a step and starts again at A.
 */
void EmYamaTsukami_SetReactState(Em *em, u16 state, u8 mode)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case YAMA_REACT_FALL_LAND:
    case YAMA_REACT_FALL_RECOVER:
        w->bodySize = YAMA_SIZE_SHRUNK;
        w->flinchStage = YAMA_FLINCH_STAGE_A;
        w->pauseCycles = 30;
        break;
    case YAMA_REACT_FLINCH_A:
        w->flinchStage = YAMA_FLINCH_STAGE_B;
        break;
    case YAMA_REACT_FLINCH_B:
        w->flinchStage = YAMA_FLINCH_STAGE_C;
        break;
    case YAMA_REACT_FLINCH_C_FROM_FULL:
        w->bodySize = YAMA_SIZE_HALF;
        w->flinchStage = YAMA_FLINCH_STAGE_A;
        break;
    case YAMA_REACT_FLINCH_C_FROM_HALF:
        w->bodySize = YAMA_SIZE_SHRUNK;
        w->flinchStage = YAMA_FLINCH_STAGE_A;
        break;
    }
    Em_SetStateSync(em, EM_GROUP_REACT, state, mode);
}

/*
 * Changes the monster's state (group, state, sync mode). The owner first
 * picks a target in range; a remote copy ignores the owner-only modes. Any
 * state but YAMA_IDLE and YAMA_FLY_IDLE ends the guard (flyGuard).
 */
void EmYamaTsukami_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    if ((group != 0 || state != 1) && (group != 2 || state != 0)) {
        w->flyGuard = 0;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmYamaTsukami_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmYamaTsukami_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmYamaTsukami_SetFlyState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmYamaTsukami_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_REACT:
        EmYamaTsukami_SetReactState(em, state, mode);
        break;
    case EM_GROUP_DEFEAT:
    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

/*
 * Plays the sounds of the motion on layer 0 at its frames
 * (Em_PlaySeAtBoneOnFrame and the volume form: frame, sound id, bone),
 * switching on the motion EmYamaTsukami_MotionEffects keeps in fxMotion.
 */
void EmYamaTsukami_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = ((EmYamaTsukamiWork *)work)->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, YAMA_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 90, Sound_RandomSe(13, 4, 15, 4), 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(101, 4, 102, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 112, Sound_RandomSe(101, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 216, Sound_RandomSe(101, 4, 102, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 320, Sound_RandomSe(101, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 120, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 220, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 224, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 324, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 328, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_RISE):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, Sound_RandomSe(13, 4, 15, 4), 3, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 101, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 158, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 92, 102, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 96, 76, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 104, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 92, Sound_RandomSe(13, 4, 15, 4), 3, 9);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_DESCEND):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, Sound_RandomSe(13, 4, 15, 4), 3, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 101, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 158, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 92, 102, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 96, 76, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 104, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 92, Sound_RandomSe(13, 4, 15, 4), 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 136, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 244, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 248, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_RISE_TO_GOAL):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 83, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 68, 44, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 11, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 12, 103, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SURGE_FORWARD):
    case EM_MOTION_ID(0, YAMA_MOT_SURGE_BACK):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 12, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 104, 102, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 106, 45, 16, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(12, 4, 14, 4), 23);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, Sound_RandomSe(12, 4, 14, 4), 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, 18, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 19, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_A_MID);
        Em_PlaySeAtBoneOnFrame((Em *) em, 46, 19, YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 36, 21, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 104, Sound_RandomSe(12, 4, 14, 4), 23);
        Em_PlaySeAtBoneOnFrame((Em *) em, 122, Sound_RandomSe(12, 4, 14, 4), 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 130, 18, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 136, 19, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 118, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_A_MID);
        Em_PlaySeAtBoneOnFrame((Em *) em, 146, 19, YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 136, 21, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 104, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 108, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_RECOVER):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 76, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 40, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, 17, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 112, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 50, 102, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 148, 101, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_FALL):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 81, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 34, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 12, 101, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 82, 101, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 84, 10, 3);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_FALL_LAND):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 76, 75, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 36, YAMA_SE_THUD, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 172, Sound_RandomSe(12, 4, 14, 4), 23);
        Em_PlaySeAtBoneOnFrame((Em *) em, 184, Sound_RandomSe(13, 4, 15, 4), 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 230, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_A_MID);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SOFT_LAND):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 76, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 11, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 22, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 22, YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 22, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 16, 63, YAMA_BONE_TENTACLE_C_TIP, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 20, 63, YAMA_BONE_TENTACLE_D_TIP, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 16, 63, YAMA_BONE_TENTACLE_A_TIP, 5);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 20, 63, YAMA_BONE_TENTACLE_B_TIP, 5);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 19, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, 20, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 66, 103, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 72, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_RISE_TO_GOAL_END):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 88, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 11, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_RECOVER_RISE):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 70, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 180, 88, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 188, 40, 36);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 180, 102, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 210, 10, 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 260, 19, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 242, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, 19, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 88, 20, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 136, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 244, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 248, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 220, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 266, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 20, 101, 3, 22);
        break;
    case EM_MOTION_ID(0, 13):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 82, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, 18, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, 18, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 14, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 18, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(13, 4, 15, 4), 23);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 19, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(13, 4, 15, 4), 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 21, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_MID);
        Em_PlaySeAtBoneOnFrame((Em *) em, 86, 19, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_MID);
        Em_PlaySeAtBoneOnFrame((Em *) em, 96, 20, YAMA_BONE_TENTACLE_A_TIP);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_RECOVER_PAUSE):
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 120, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(101, 4, 102, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 112, Sound_RandomSe(101, 4, 103, 4), 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_DOWNED):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 12, 82, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(0, 5, 1, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(1, 3, 2, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 17, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 11, 36);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_GET_UP):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 52, 76, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 72, YAMA_SE_THUD, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 10, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 72, 16, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 142, 103, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_MOVE):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 18, Sound_RandomSe(101, 4, 102, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 70, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 28, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_MOVE_CRUISE):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, Sound_RandomSe(101, 4, 102, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 18, 70, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_MOVE_STOP):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 71, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 50, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, Sound_RandomSe(101, 4, 101, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 92, Sound_RandomSe(103, 4, 102, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_SLAM_A):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 72, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 10, 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 19, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 21, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 12, 23);
        Em_PlaySeAtBoneOnFrame((Em *) em, 160, 19, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 156, 21, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 152, Sound_RandomSe(0, 5, 1, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 156, Sound_RandomSe(1, 3, 2, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 22, Sound_RandomSe(101, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 122, Sound_RandomSe(101, 4, 102, 4), 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_SLAM_B):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 72, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 10, 23);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 19, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 64, 21, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, 12, 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 160, 19, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 156, 21, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 152, Sound_RandomSe(0, 5, 1, 3), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 156, Sound_RandomSe(1, 3, 2, 5), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 22, Sound_RandomSe(101, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 122, Sound_RandomSe(101, 4, 102, 4), 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_HIT_A):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 71, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 10, 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 108, 16, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 104, 21, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 200, 15, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 17, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 220, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 224, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 120, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_HIT_B):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 71, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 10, 23);
        Em_PlaySeAtBoneOnFrame((Em *) em, 108, 16, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 104, 21, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 200, 15, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 16, 17, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 220, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 224, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 120, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_SLAM_C):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_MID);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 21, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 78, 19, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_MID);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 21, YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 156, 19, YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 194, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 220, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 224, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 120, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_SLAM_D):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_MID);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 21, YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 78, 19, YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_MID);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 128, 21, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 156, 19, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 194, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 220, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 224, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 120, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SMASH):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 22, 71, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 122, 72, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 228, 101, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 10, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 34, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 158, YAMA_SE_SMASH_HIT, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 162, YAMA_SE_SMASH_HIT, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 160, 21, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 164, 21, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 162, YAMA_SE_SMASH_HIT, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 158, YAMA_SE_SMASH_HIT, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 224, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 226, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 174, YAMA_SE_FORT_IMPACT_B, 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 184, 9, 23);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SMASH_END):
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, Sound_RandomSe(13, 4, 15, 4), 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 100, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 90, Sound_RandomSe(13, 4, 15, 4), 23);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 22, 101, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 122, 101, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 104, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 106, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_HOLD_POSE):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 86, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 144, 39, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 40, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 144, 103, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_HOLD_POSE_END):
    case EM_MOTION_ID(0, YAMA_MOT_SUMMON_END):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 88, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 17, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 38, 21, YAMA_BONE_TENTACLE_C_TIP, 6);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 44, 21, YAMA_BONE_TENTACLE_D_TIP, 6);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_INHALE):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 22, 78, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 106, 39, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 222, 40, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 260, 40, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 136, 39, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 142, 39, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 224, 39, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 228, 39, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 284, 39, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 288, 39, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 314, 60, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 258, 60, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 238, 62, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 296, 62, 36, 9);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_INHALE_END):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 88, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, 17, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 116, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 120, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_STRIKE_A):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 77, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 22, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 22, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, 22, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 58, 22, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 76, 9, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 72, 9, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 10, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 94, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 98, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 96, 101, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_STRIKE_B):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 77, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, 22, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, 22, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, 22, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, 22, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 10, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 66, 101, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_STRIKE_B_HOLD):
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 9, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, 8, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 102, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 106, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 16, 101, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 10, 3);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_STRIKE_RECOVER):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 76, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 130, 70, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 11, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 108, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 112, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 60, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 152, 103, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 30, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 32, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 142, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 144, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 254, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 256, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SPIN_START):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 71, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 30, Sound_RandomSe(102, 4, 102, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 16, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 50, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 46, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_B_TIP);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SPIN):
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 18, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 18, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 20, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 20, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 39, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SPIN_END):
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 19, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 19, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 2, 21, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 18, 21, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 88, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 30, Sound_RandomSe(102, 4, 102, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 16, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SUMMON):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 6, 86, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 20, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 142, 40, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 40, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 144, 103, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_CRASH_FALL):
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 18, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 20, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 40, 102, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_DIVE_SLAM):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 48, 87, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 10, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 208, 11, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 56, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 60, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 74, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 160, 49, YAMA_BONE_TENTACLE_B_BASE, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 168, 49, YAMA_BONE_TENTACLE_A_BASE, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 40, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 104, 101, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 222, 102, 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_CRASH_LIE):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 44, 71, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 68, 12, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 11, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 126, 7, 3, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 40, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 132, YAMA_SE_THUD, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 134, 8, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 138, 8, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 136, 9, YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 132, 9, YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 180, Sound_RandomSe(0, 5, 1, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 184, Sound_RandomSe(1, 3, 2, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 300, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 304, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_CRASH_GET_UP):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 68, 70, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, 11, 3);
        Em_PlaySeAtBoneOnFrame((Em *) em, 6, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 84, 103, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 114, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 118, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_FLINCH_A):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 73, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(13, 6, 15, 2), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(13, 2, 15, 6), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_FLINCH_B):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 74, 36, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(12, 4, 14, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 10, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 70, 17, 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 92, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 74, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 78, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_DOWNED_FLINCH):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 74, 36);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(14, 4, 16, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 84, 19, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 88, 20, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 102, 9, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 106, 9, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 22, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 26, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_DIE):
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 4, 75, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 220, 70, 36, 9);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 178, YAMA_SE_THUD, 3, 9);
        Em_PlaySeAtBoneOnFrame((Em *) em, 162, 18, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 176, 9, YAMA_BONE_SMALL_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 182, 20, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 194, 8, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 17, YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 146, Sound_RandomSe(13, 6, 15, 2), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 310, Sound_RandomSe(13, 2, 15, 6), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 306, Sound_RandomSe(13, 6, 15, 2), YAMA_BONE_TENTACLE_A_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 310, Sound_RandomSe(13, 2, 15, 6), YAMA_BONE_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, 17, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 246, Sound_RandomSe(13, 6, 15, 2), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 336, 8, YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 12, 103, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 84, 101, 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 172, 102, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 74, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 78, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_FLINCH_D):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 73, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(14, 4, 16, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 82, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_EVENT_TURN):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 73, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(14, 4, 16, 4), YAMA_BONE_TENTACLE_C_TIP);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 82, Sound_RandomSe(13, 4, 15, 4), YAMA_BONE_TENTACLE_D_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 12, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_FLINCH_HOVER):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 73, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 62, 18, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 84, 19, YAMA_BONE_SMALL_TENTACLE_B_TIP);
        Em_PlaySeAtBoneOnFrame((Em *) em, 8, Sound_RandomSe(12, 4, 14, 4), 3);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 12, 103, 3, 22);
        Em_PlaySeAtBoneOnFrame((Em *) em, 34, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 38, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        break;
    case EM_MOTION_ID(0, YAMA_MOT_FLINCH_C):
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, 75, 36);
        Em_PlaySeAtBoneOnFrame((Em *) em, 4, Sound_RandomSe(12, 4, 14, 4), 23);
        Em_PlaySeAtBoneOnFrame((Em *) em, 24, Sound_RandomSe(12, 4, 14, 4), 29);
        Em_PlaySeAtBoneOnFrame((Em *) em, 48, Sound_RandomSe(3, 5, 4, 3), YAMA_BONE_TENTACLE_A_BASE);
        Em_PlaySeAtBoneOnFrame((Em *) em, 52, Sound_RandomSe(4, 3, 5, 5), YAMA_BONE_TENTACLE_B_BASE);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 8, Sound_RandomSe(103, 4, 103, 4), 3, 22);
        Em_PlaySeAtBoneVolOnFrame((Em *) em, 78, Sound_RandomSe(102, 4, 103, 4), 3, 22);
        
    default:
        break;
    }
}

/*
 * The family's setup (Em_TypeSetup calls it): clears queryValue and, while
 * the carcass's mine point exists, moves it to the monster's area and to
 * its place above YAMA_BONE_BODY.
 */
void EmYamaTsukami_Setup(Em *em)
{
    f32 pos[4];
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    w->queryValue = 0;
    if (w->mineSlot != -1) {
        MinePoint_SetArea(w->mineSlot, em->area);
        EmYamaTsukami_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
    }
}

/*
 * One of the family's table hooks (slots 0 and 1): creates the effect drawn
 * with the monster (its shadow, SHADOW_GROUND_BONES) and, for the second kind, the EmBodyFx
 * effect of kind 5.
 */
void EmYamaTsukami_SpawnEffects(Em *em)
{
    Shadow_Spawn((Player *) &em->active, SHADOW_GROUND_BONES);
    if (em->type == EM_KIND_YAMA_TSUKAMI_B) {
        EmBodyFx_Spawn(em, 5);
    }
}

/*
 * Writes the carcass mine point's position to out: 400 above
 * YAMA_BONE_BODY, along the bone's rotation.
 */
void EmYamaTsukami_GetMinePointPos(Em *em, f32 *out)
{
    f32 upOfs[3];
    f32 rotated[3];
    f32 bonePos[3];

    upOfs[1] = 400.0f;
    upOfs[0] = 0.0f;
    upOfs[2] = 0.0f;
    Em_BonePos(em, YAMA_BONE_BODY, bonePos);
    Vec_Rotate((f32 *) rotated, (f32 *) upOfs, (f32 *) (Em_Bone(em, YAMA_BONE_BODY)));
    out[0] = bonePos[0] + rotated[0];
    out[1] = bonePos[1] + rotated[1];
    out[2] = bonePos[2] + rotated[2];
}

/*
 * Makes the carcass's carve point, a mine point of radius 800 with
 * mineCount carves left and its item table by the quest's tier, at
 * EmYamaTsukami_GetMinePointPos's position; with no carves left it makes
 * none (mineSlot -1).
 */
void EmYamaTsukami_AddMinePoint(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    MinePoint point;
    f32 pos[3];

    if (w->mineCount <= 0) {
        w->mineSlot = -1;
        return;
    }
    point.radius = 800.0f;
    switch (gQuest.header->tier) {
    default:
        point.table = 99;
        break;
    case 1:
        point.table = 224;
        break;
    case 2:
        point.table = 348;
        break;
    case 3:
        point.table = 348;
        break;
    }
    point.action = MINE_ACTION_CARVE;
    point.count = w->mineCount;
    point.area = (u8) em->area;
    point.flags = 1;
    point.timer = 0;
    EmYamaTsukami_GetMinePointPos(em, pos);
    w->mineSlot = MinePoint_AddUntimed(&point, pos, 0);
}

/* Removes the three mine points in minePoints and marks them gone (-1). */
void EmYamaTsukami_RemoveMinePoints(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) &em->work;
    s32 i;
    s8 *slot;

    for (i = 0; i < 3; i++) {
        slot = &w->minePoints[i];
        if (*slot != -1) {
            MinePoint_Remove(*slot);
            *slot = -1;
        }
    }
}

/*
 * Sets the work's three move angles toward the goal: X pitches the body by
 * the goal's height over its distance, Y is the heading to the goal less a
 * quarter turn, Z is 0.
 */
void EmYamaTsukami_AimAtGoal(Em *em, EmYamaTsukamiWork *w)
{
    w->accelAngX = ANGLE_TURN - (Math_Heading(Vec_Distance((f32 *) &em->posX, (f32 *) em->goal), em->posY, 0.0f, em->goal[1]) & 0xFFFF);
    w->accelAngY = (s32) ((Math_Heading(em->posX, em->posZ, em->goal[0], em->goal[2]) & 0xFFFF) - ANGLE_TURN / 4);
    w->accelAngZ = 0;
}

/*
 * The start hook. At the quest's start the monster is placed at the area's
 * start position. The second kind goes to its fixed place
 * (EmYamaTsukami_KindBStartPos), sees nobody, stays guarded and leaves no
 * carcass; the first fights every player and makes its carcass mine point
 * with three carves. Then: hp from the kind's table, the idle motion, a
 * random state of group 2, full size, and the flight stance.
 */
void EmYamaTsukami_Start(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    s16 maxHp;

    if (gQuest.stage == 0) {
        em->posX = gStartPos[gScene.area * 3];
        em->posY = gStartPos[gScene.area * 3 + 1];
        em->posZ = gStartPos[gScene.area * 3 + 2];
    }
    if (em->type == EM_KIND_YAMA_TSUKAMI_B) {
        Area_DayId(em->area);
        em->posX = EmYamaTsukami_KindBStartPos[0];
        em->posY = EmYamaTsukami_KindBStartPos[1];
        em->posZ = EmYamaTsukami_KindBStartPos[2];
        em->blind = 2;
        em->unk_InitFlag = 0;
        em->combat = 1;
        w->mineCount = 0;
        w->mineSlot = -1;
        w->flyGuard = 1;
    } else {
        em->unk_InitFlag = 1;
        em->combat = 1;
        em->combatPlayers = 15;
        em->blind = 0;
        w->flyGuard = 0;
        w->mineCount = 3;
        EmYamaTsukami_AddMinePoint(em);
    }
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    Em_SetMotionRow(em, YAMA_MOT_IDLE, 0, 0);
    Em_SetStateRand(em, 2, 0);
    w->flinchStage = YAMA_FLINCH_STAGE_A;
    w->queryValue = 0;
    w->pauseCycles = 15;
    w->bodySize = YAMA_SIZE_FULL;
    w->easeMode = 0;
    w->bodyScale[0] = 1.0f;
    w->bodyScale[1] = 1.0f;
    w->bodyScale[2] = 1.0f;
    w->syncTimer = 150;
    em->stance = EM_STANCE_AIR;
}

/*
 * Ends a state on the ground: back to YAMA_IDLE, with the script to choose
 * the next state.
 */
void EmYamaTsukami_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->stance = EM_STANCE_GROUND;
    EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE, EM_SYNC_LOCAL);
}

/*
 * Ends a state in the air: back to YAMA_FLY_IDLE, with the script to choose
 * the next state.
 */
void EmYamaTsukami_EndFlyState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->stance = EM_STANCE_AIR;
    EmYamaTsukami_ChangeState(em, EM_GROUP_FLY, YAMA_FLY_IDLE, EM_SYNC_LOCAL);
}

/*
 * YAMA_IDLE_HOLD: keeps the idle motion set unless debug flag 0x4D is on.
 * The second test compares layer 1 with 1101, which is no layer-1
 * motion id (those start at 1201), so it always sets the idle again.
 */
void EmYamaTsukami_IdleHold(Em *em, EmWork *work)
{
    if (Debug_IsOn(77) == 0) {
        if (em->motionId[0] != EM_MOTION_ID(0, YAMA_MOT_IDLE)) {
            Em_SetMotion(em, YAMA_MOT_IDLE, 10, 0);
        }
        if (em->motionId[1] != 1101) { /* 1000 + 100 + the idle; layer 1's ids are 1200 + the motion */
            Em_SetMotion(em, YAMA_MOT_IDLE, 10, 0);
        }
    }
}

/*
 * YAMA_IDLE: one cycle of the idle motion on the ground, then
 * EmYamaTsukami_EndState.
 */
void EmYamaTsukami_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, YAMA_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

/*
 * YAMA_IDLE_DOWNED: lies on the ground for five cycles of the motion, then
 * gets up (YAMA_IDLE_GET_UP). A remote copy waits for the owner's change.
 */
void EmYamaTsukami_Downed(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, YAMA_MOT_DOWNED, 0, 0);
        em->timer = 5;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE_GET_UP, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * YAMA_IDLE_GET_UP: gets up, unhurt and with no reactions meanwhile, then
 * swells back (YAMA_IDLE_RECOVER).
 */
void EmYamaTsukami_GetUp(Em *em, EmWork *work)
{
    em->guardTimer = 5;
    em->noReact = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, YAMA_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE_RECOVER, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE_RECOVER, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * YAMA_IDLE_RECOVER: one cycle of the recover motion on the ground (the
 * body eases back to full size), then EmYamaTsukami_EndState.
 */
void EmYamaTsukami_Recover(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, YAMA_MOT_RECOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

/* Runs the current state of group 0 (EM_GROUP_IDLE). */
void EmYamaTsukami_RunIdleState(Em *em)
{
    EmWork *work = (EmWork *) &em->work;

    switch (em->state) {
    case YAMA_IDLE_HOLD:
        EmYamaTsukami_IdleHold(em, work);
        break;
    case YAMA_IDLE:
        EmYamaTsukami_Idle(em, work);
        break;
    case YAMA_IDLE_DOWNED:
        EmYamaTsukami_Downed(em, work);
        break;
    case YAMA_IDLE_GET_UP:
        EmYamaTsukami_GetUp(em, work);
        break;
    case YAMA_IDLE_RECOVER:
        EmYamaTsukami_Recover(em, work);
        break;
    }
}

/*
 * YAMA_MOVE_FORWARD: walks forward by the motion's root movement, turning
 * toward the goal by up to 64 a frame, until goalDist runs out; the
 * motion then ends the state.
 */
void EmYamaTsukami_WalkForward(Em *em)
{
    f32 pos[3];
    s32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, YAMA_MOT_MOVE, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            turn = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
            if (turn <= 0x8000) {
                if (turn <= 63) {
                    em->rotY += turn;
                } else {
                    em->rotY += 64;
                }
            } else {
                if (turn > 0xFFC0) {
                    em->rotY += turn;
                } else {
                    em->rotY -= 64;
                }
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist -= pos[2]) <= 0.0f) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

/*
 * YAMA_MOVE_BACKWARD: as EmYamaTsukami_WalkForward with the back turned to
 * the goal, until the distance walked away (added to goalDist) brings it to
 * 0.
 */
void EmYamaTsukami_WalkBackward(Em *em)
{
    f32 pos[3];
    s32 turn;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, YAMA_MOT_MOVE, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            turn = ((((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) + 0x8000) & 0xFFFF) - em->rotY) & 0xFFFF;
            if (turn <= 0x8000) {
                if (turn <= 63) {
                    em->rotY += turn;
                } else {
                    em->rotY += 64;
                }
            } else {
                if (turn > 0xFFC0) {
                    em->rotY += turn;
                } else {
                    em->rotY -= 64;
                }
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist += pos[2]) <= 0.0f) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

/*
 * Turns to the goal, speeding up by 32 a frame until half the turn is
 * made and then slowing down; the motion's end ends the state. variant 0:
 * on the ground (YAMA_MOVE_TURN); 1: in the air (YAMA_FLY_TURN); 2: in the
 * air, guarded (YAMA_FLY_TURN_GUARDED).
 */
void EmYamaTsukami_Turn(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    u32 diff;
    u16 aim;
    s32 stepAngle;
    s32 rot;

    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        switch (variant) {
        case 0:
            em->stance = EM_STANCE_GROUND;
            if (em->motionId[1] == EM_MOTION_ID(0, YAMA_MOT_IDLE)) { /* a layer-0 id: never true on layer 1 */
                Em_SetMotion(em, YAMA_MOT_IDLE, 0, 0);
            }
            break;
        case 2:
            w->flyGuard = 1;
        case 1:
            em->stance = EM_STANCE_AIR;
            Em_SetMotion(em, YAMA_MOT_MOVE, 0, 0);
            break;
        }
        em->lookAtTarget = 0;
        em->aimAngle = Math_AngleXZ(&em->posX, em->goal);
        em->timer = 0;
        diff = (em->aimAngle - *(u16 *) &em->rotY) & 0xFFFF; /* rotY's low half, loaded as a halfword as the game does */
        if (diff < 0x8000) {
            w->turnLeft = diff;
        } else {
            w->turnLeft = 0x10000 - diff;
        }
        w->turnLeft /= 2;
        w->turnSpeed = 0;
        break;
    case 1:
        if (em->stateVar0 == 0) {
            w->turnSpeed += 32;
            w->turnLeft -= w->turnSpeed;
            if (w->turnLeft <= 0) {
                em->stateVar0 = 1;
            }
        } else {
            w->turnSpeed -= 32;
            if (w->turnSpeed <= 0) {
                w->turnSpeed = 32;
            }
        }
        rot = em->rotY;
        aim = em->aimAngle;
        stepAngle = w->turnSpeed;
        diff = (aim - (rot & 0xFFFF)) & 0xFFFF;
        if (((diff + stepAngle) & 0xFFFF) < stepAngle * 2) {
            em->step++;
            em->rotY = em->aimAngle;
        } else if (diff < 0x8000) {
            em->rotY = (rot + stepAngle) & 0xFFFF;
        } else {
            em->rotY = (rot - stepAngle) & 0xFFFF;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            switch (variant) {
            case 0:
                EmYamaTsukami_EndState(em);
                break;
            case 1:
            case 2:
                EmYamaTsukami_EndFlyState(em);
                break;
            }
        }
        break;
    }
}

/* Runs the current state of group 1 (EM_GROUP_MOVE). */
void EmYamaTsukami_RunMoveState(Em *em)
{
    switch (em->state) {
    case YAMA_MOVE_FORWARD:
        EmYamaTsukami_WalkForward(em);
        break;
    case YAMA_MOVE_BACKWARD:
        EmYamaTsukami_WalkBackward(em);
        break;
    case YAMA_MOVE_TURN:
        EmYamaTsukami_Turn(em, 0);
        break;
    }
}

/*
 * YAMA_FLY_IDLE: one cycle of the idle motion in the air, then
 * EmYamaTsukami_EndFlyState.
 */
void EmYamaTsukami_FlyIdle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_FLY_RECOVER_PAUSE: after a short fall, holds the pause motion for
 * pauseCycles cycles, then rises back (YAMA_FLY_RECOVER_RISE).
 */
void EmYamaTsukami_RecoverPause(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_RECOVER_PAUSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (--w->pauseCycles <= 0) {
                em->step++;
                EmYamaTsukami_ChangeState(em, EM_GROUP_FLY, YAMA_FLY_RECOVER_RISE, EM_SYNC_OWNER_FLAGGED);
            }
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_FLY, YAMA_FLY_RECOVER_RISE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Floats straight to the goal (EmYamaTsukami_AimAtGoal each frame) with a
 * speed that swells and ebbs, snapping to the goal when within 10, until
 * the distance runs out. variant 0: YAMA_FLY_DRIFT_TO_GOAL (10, give or
 * take 1); 1: YAMA_FLY_DASH_TO_GOAL, guarded (30 frames, then 20, give or
 * take 2, and a stop motion at the end); 2: YAMA_FLY_DRIFT_TO_GOAL_GUARDED;
 * 3: YAMA_FLY_CRUISE_TO_GOAL (15, give or take 1.5).
 */
void EmYamaTsukami_FlyToGoal(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Player_ClearVelocity(em);
        em->goalDist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        EmYamaTsukami_AimAtGoal(em, w);
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
        }
        em->timer = 0;
        switch (variant) {
        case 1:
            w->flyGuard = 1;
            Em_SetMotion(em, YAMA_MOT_MOVE, 0, 0);
            w->runUpTimer = 30;
            break;
        case 2:
            w->flyGuard = 1;
        case 0:
        case 3:
            Em_SetMotion(em, YAMA_MOT_MOVE, 0, 0);
            em->step++;
            break;
        }
        break;
    case 1:
        if (--w->runUpTimer <= 0) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_MOVE_CRUISE, 0, 0);
        }
    case 2:
        switch (variant) {
        case 1:
            em->velZ = 20.0f + 2.0f * Math_Sin((6.2831855f * (f32) (em->timer << 10)) / 65536.0f);
            break;
        case 2:
        case 0:
            em->velZ = 10.0f + Math_Sin((6.2831855f * (f32) (em->timer << 10)) / 65536.0f);
            break;
        case 3:
            em->velZ = 15.0f + 1.5f * Math_Sin((6.2831855f * (f32) (em->timer << 10)) / 65536.0f);
            break;
        }
        em->timer++;
        EmYamaTsukami_AimAtGoal(em, w);
        Em_MoveRotated(em, &w->accelAngX);
        if (Vec_DistanceXZ(&em->posX, em->goal) < 10.0f) {
            em->posX = em->goal[0];
            em->posZ = em->goal[2];
        }
        if (Math_Absf(em->goal[1] - em->posY) < 10.0f) {
            em->posY = em->goal[1];
        }
        if ((em->goalDist -= em->velZ) <= 0.0f) {
            em->step = 3;
            if (variant == 1) {
                Em_SetMotion(em, YAMA_MOT_MOVE_STOP, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_FLY_RISE_TO_GOAL_HEIGHT (variant 1: guarded): rises on its curve
 * until the goal's height or the motion's end, then plays the end motion on
 * its own curve. The body never goes under the floor.
 */
void EmYamaTsukami_RiseToGoalHeight(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_RISE_TO_GOAL, 0, 0);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_RISE_TO_GOAL);
        if (variant != 0) {
            w->flyGuard = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if ((em->motion[0].playing == 0) || !(em->posY <= em->goal[1])) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_RISE_TO_GOAL_END, 0, 0);
            EmYamaTsukami_StartCurves(em, YAMA_CURVE_RISE_TO_GOAL_END);
        }
        break;
    case 2:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * YAMA_FLY_RISE (variant 1: guarded): one rise motion on its height curve,
 * kept above the floor.
 */
void EmYamaTsukami_Rise(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_RISE, 0, 0);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_RISE);
        if (variant != 0) {
            w->flyGuard = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * YAMA_FLY_DESCEND (variant 1: guarded): one descent motion on its height
 * curve, kept above the floor.
 */
void EmYamaTsukami_Descend(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_DESCEND, 0, 0);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_DESCEND);
        if (variant != 0) {
            w->flyGuard = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * YAMA_FLY_SURGE_FORWARD (variant 1: guarded): moves on its speed curve
 * until within 500 of the goal, kept above the floor.
 */
void EmYamaTsukami_SurgeForward(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_SURGE_FORWARD, 0, 0);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_SURGE_FORWARD);
        if (variant != 0) {
            w->flyGuard = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 500.0f) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * YAMA_FLY_SURGE_BACK (variant 1: guarded): one motion on its speed curve,
 * kept above the floor.
 */
void EmYamaTsukami_SurgeBack(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_SURGE_BACK, 0, 0);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_SURGE_BACK);
        if (variant != 0) {
            w->flyGuard = 1;
        }
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * YAMA_FLY_RECOVER: one cycle of the recover motion in the air (the body
 * eases back to full size), then EmYamaTsukami_EndFlyState.
 */
void EmYamaTsukami_FlyRecover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_RECOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_FLY_RECOVER_RISE: rises back on its curve after a short fall, then
 * EmYamaTsukami_EndFlyState.
 */
void EmYamaTsukami_RecoverRise(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_RECOVER_RISE, 0, 0);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_RECOVER_RISE);
        em->timer = 5;
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_FLY_WAIT_FOR_HUNTERS: idles until a player in the quest is in the
 * monster's area (the owner checks), then EmYamaTsukami_EndFlyState.
 */
void EmYamaTsukami_WaitForHunters(Em *em)
{
    s32 i;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->remote == 0) {
            for (i = 0; i < gScene.playerCount; i++) {
                if (gScene.inQuest[i] == 1 && em->area == gPlayers[i].area) {
                    EmYamaTsukami_EndFlyState(em);
                    break;
                }
            }
        }
        break;
    }
}

/*
 * YAMA_FLY_SINK_TO_FLOOR: sinks at about 10 a frame until it reaches the
 * floor, then plays the move motion out and ends in the air stance.
 */
void EmYamaTsukami_SinkToFloor(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Player_ClearVelocity(em);
        em->goalDist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
        em->timer = 0;
        Em_SetMotion(em, YAMA_MOT_MOVE, 0, 0);
        break;
    case 1:
        em->velY = -10.0f + Math_Sin((6.2831855f * (f32) (em->timer << 10)) / 65536.0f);
        em->timer++;
        Em_MoveRotated(em, &w->accelAngX);
        if (!(em->floorY < em->posY)) {
            em->step++;
            em->posY = em->floorY;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/* Runs the current state of group 2 (EM_GROUP_FLY), the family's usual one. */
void EmYamaTsukami_RunFlyState(Em *em)
{
    switch (em->state) {
    case YAMA_FLY_IDLE:
        EmYamaTsukami_FlyIdle(em);
        break;
    case YAMA_FLY_RECOVER_PAUSE:
        EmYamaTsukami_RecoverPause(em);
        break;
    case YAMA_FLY_TURN:
        EmYamaTsukami_Turn(em, 1);
        break;
    case YAMA_FLY_TURN_GUARDED:
        EmYamaTsukami_Turn(em, 2);
        break;
    case YAMA_FLY_DRIFT_TO_GOAL:
        EmYamaTsukami_FlyToGoal(em, 0);
        break;
    case YAMA_FLY_DRIFT_TO_GOAL_GUARDED:
        EmYamaTsukami_FlyToGoal(em, 2);
        break;
    case YAMA_FLY_DASH_TO_GOAL:
        EmYamaTsukami_FlyToGoal(em, 1);
        break;
    case YAMA_FLY_RISE_TO_GOAL_HEIGHT:
        EmYamaTsukami_RiseToGoalHeight(em, 0);
        break;
    case YAMA_FLY_RISE_TO_GOAL_HEIGHT_GUARDED:
        EmYamaTsukami_RiseToGoalHeight(em, 1);
        break;
    case YAMA_FLY_RISE:
        EmYamaTsukami_Rise(em, 0);
        break;
    case YAMA_FLY_DESCEND:
        EmYamaTsukami_Descend(em, 0);
        break;
    case YAMA_FLY_SURGE_FORWARD:
        EmYamaTsukami_SurgeForward(em, 0);
        break;
    case YAMA_FLY_SURGE_BACK:
        EmYamaTsukami_SurgeBack(em, 0);
        break;
    case YAMA_FLY_RISE_GUARDED:
        EmYamaTsukami_Rise(em, 1);
        break;
    case YAMA_FLY_DESCEND_GUARDED:
        EmYamaTsukami_Descend(em, 1);
        break;
    case YAMA_FLY_SURGE_FORWARD_GUARDED:
        EmYamaTsukami_SurgeForward(em, 1);
        break;
    case YAMA_FLY_SURGE_BACK_GUARDED:
        EmYamaTsukami_SurgeBack(em, 1);
        break;
    case YAMA_FLY_RECOVER:
        EmYamaTsukami_FlyRecover(em);
        break;
    case YAMA_FLY_RECOVER_RISE:
        EmYamaTsukami_RecoverRise(em);
        break;
    case YAMA_FLY_CRUISE_TO_GOAL:
        EmYamaTsukami_FlyToGoal(em, 3);
        break;
    case YAMA_FLY_WAIT_FOR_HUNTERS:
        EmYamaTsukami_WaitForHunters(em);
        break;
    case YAMA_FLY_RISE_B:
        EmYamaTsukami_Rise(em, 0);
        break;
    case YAMA_FLY_SINK_TO_FLOOR:
        EmYamaTsukami_SinkToFloor(em);
        
    default:
        break;
    }
}

/*
 * YAMA_ATK_SUMMON: in the summon pose, calls up kind 56 monsters at its
 * bone (EmKind38_SpawnAtEmBone), one after 140 frames and then every 20
 * frames, five in all less those already alive.
 */
void EmYamaTsukami_Summon(Em *em)
{
    s32 i;
    Em *other;

    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar1 = 5;
        em->stateVar0 = 5;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->timer = 140;
        Em_SetMotion(em, YAMA_MOT_SUMMON, 0, 0);
        for (i = 0, other = gEms; i < 20; i++, other++) {
            if (other->type == 56 && other->active != 0 && em->stateVar0 > 0) {
                em->stateVar0--;
            }
        }
        break;
    case 1:
        if (--em->timer < 0) {
            if (em->stateVar0 > 0) {
                em->stateVar0--;
                EmKind38_SpawnAtEmBone(em);
            }
            if (em->stateVar1 > 0) {
                em->stateVar1--;
                em->timer = 20;
            } else {
                em->step++;
                Em_SetMotion(em, YAMA_MOT_SUMMON_END, 0, 0);
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * Turns the body by key table idx of EmYamaTsukami_SpinYawKeys at the curve
 * frame: the yaw's change between the two keys around the frame, in
 * degrees, spread over the frames between them. Returns 2 at the table's
 * end, else 0.
 */
static u8 EmYamaTsukami_TurnByKeys(Em *em, EmYamaTsukamiWork *w, s32 idx)
{
    u8 ret;
    EmYamaTsukamiYawKey *key;
    s32 i;
    EmYamaTsukamiYawKey *keys;
    s32 frame;
    s32 keyFrame;
    s32 degPerFrame;

    frame = w->curveFrame;
    ret = 0;
    keys = EmYamaTsukami_SpinYawKeys[idx];
    degPerFrame = 0;
    if ((u32) frame > 1) {
        i = 1;
        while (i != 0) {
            key = &keys[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame > 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 2;
                break;
            } else if (keyFrame < 0) {
                w->curveFrame = 0;
                ret = 2;
                break;
            } else {
                i = 0;
                degPerFrame = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    em->rotY += (u16) (s32) (0.5f + 65536.0f * degPerFrame / 360.0f);
    return ret;
}

/*
 * Spins with the attack's hit (YamaTsukamiAttack type 0): a spin-up on its
 * curve and yaw keys, the spin (its step slowed by 113 a cycle for ten
 * cycles, then sped up again), and a spin-down. variant 0
 * (YAMA_ATK_SPIN_TO_HEIGHT) rises or sinks to the goal's height meanwhile,
 * at most a 160th of the way a frame; variant 1 (YAMA_ATK_SPIN) spins in
 * place.
 */
void EmYamaTsukami_Spin(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    f32 dy;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_SPIN_START, 0, 0);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_SPIN_START);
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        EmYamaTsukami_TurnByKeys(em, w, 0);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_SPIN, 0, 0);
            em->timer = 10;
            w->turnSpeed = -1365;
            if (variant == 0) {
                em->velY = (em->goal[1] - em->posY) / 160.0f;
            }
            YamaTsukamiAttack_Create(em, 0);
        }
        break;
    case 2:
        em->rotY += w->turnSpeed;
        if (variant == 0) {
            dy = em->goal[1] - em->posY;
            if (em->velY > dy) {
                em->velY = dy;
            }
            em->posY += em->velY;
        }
        if (em->motion[0].playing == 0 && em->timer > 0) {
            em->timer--;
            w->turnSpeed -= 113;
        }
        if (variant == 0) {
            if (Math_Absf(em->goal[1] - em->posY) < 100.0f && em->timer <= 0) {
                em->step++;
                em->timer = 10;
            }
        } else if (em->timer <= 0) {
            em->step++;
            em->timer = 10;
        }
        break;
    case 3:
        em->rotY += w->turnSpeed;
        if (em->motion[0].playing == 0) {
            if (--em->timer < 0) {
                em->step++;
                Em_SetMotion(em, YAMA_MOT_SPIN_END, 0, 0);
                EmYamaTsukami_StartCurves(em, YAMA_CURVE_SPIN_END);
            } else {
                w->turnSpeed += 113;
                Em_SetMotion(em, YAMA_MOT_SPIN, 0, 0);
            }
        }
        break;
    case 4:
        EmYamaTsukami_UpdateCurves(em);
        EmYamaTsukami_TurnByKeys(em, w, 1);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/* YAMA_ATK_HOLD_POSE: holds the pose 200 frames, then its end motion. */
void EmYamaTsukami_HoldPose(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_HOLD_POSE, 0, 0);
        em->timer = 200;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_HOLD_POSE_END, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_ATK_TENTACLE_SLAM_A: the slam motion (its hits come from
 * EmYamaTsukami_MotionEffects), with dust where each tentacle reaches the
 * floor.
 */
void EmYamaTsukami_TentacleSlamA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_TENTACLE_SLAM_A, 0, 0);
        EmYamaTsukami_InitTentacleDust(em);
        break;
    case 1:
        EmYamaTsukami_UpdateTentacleDust(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/* YAMA_ATK_TENTACLE_SLAM_B, as EmYamaTsukami_TentacleSlamA. */
void EmYamaTsukami_TentacleSlamB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_TENTACLE_SLAM_B, 0, 0);
        EmYamaTsukami_InitTentacleDust(em);
        break;
    case 1:
        EmYamaTsukami_UpdateTentacleDust(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/* YAMA_ATK_TENTACLE_SLAM_C, as EmYamaTsukami_TentacleSlamA. */
void EmYamaTsukami_TentacleSlamC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_TENTACLE_SLAM_C, 0, 0);
        EmYamaTsukami_InitTentacleDust(em);
        break;
    case 1:
        EmYamaTsukami_UpdateTentacleDust(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/* YAMA_ATK_TENTACLE_SLAM_D, as EmYamaTsukami_TentacleSlamA. */
void EmYamaTsukami_TentacleSlamD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_TENTACLE_SLAM_D, 0, 0);
        EmYamaTsukami_InitTentacleDust(em);
        break;
    case 1:
        EmYamaTsukami_UpdateTentacleDust(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * The smash: held 450 frames, then its end motion, with the tentacles'
 * dust. variant 1 (YAMA_ATK_SMASH_FORT) hits the fortress at frame 170: the
 * owner counts fortHits up to 10 (the tenth raises the quest's event) and
 * copies it to gScene.fortStage, and every console makes the impact
 * (EmYamaTsukami_SpawnFortImpact). variant 0 is YAMA_ATK_SMASH.
 */
void EmYamaTsukami_Smash(Em *em, s32 variant)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) &em->work;
    s8 n;

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_SMASH, 0, 0);
        em->timer = 450;
        EmYamaTsukami_InitTentacleDust(em);
        break;
    case 1:
        EmYamaTsukami_UpdateTentacleDust(em);
        if ((variant == 1) && (EmMotion_PassedFrameScaled(em, 170.0f, 0U) != 0)) {
            if (em->remote == 0) {
                n = w->fortHits;
                if (n < 10) {
                    w->fortHits = n + 1;
                    if (w->fortHits >= 10) {
                        Quest_TriggerEvent(em);
                    }
                    gScene.fortStage = w->fortHits;
                }
            }
            EmYamaTsukami_SpawnFortImpact(em);
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_SMASH_END, 0, 0);
        }
        break;
    case 2:
        EmYamaTsukami_UpdateTentacleDust(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_ATK_DIVE_SLAM: one dive motion on its curve; its hits come from
 * EmYamaTsukami_MotionEffects.
 */
void EmYamaTsukami_DiveSlam(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_DIVE_SLAM, 0, 0);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_DIVE_SLAM);
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_ATK_CRASH: drops to the floor (at least 50 a frame, gravity 10),
 * lies there 450 frames, gets up and ends on the ground
 * (EmYamaTsukami_EndState). Unhurt throughout.
 */
void EmYamaTsukami_Crash(Em *em)
{
    em->guardTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_CRASH_FALL, 0, 0);
        em->noReact = 1;
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_CRASH_LIE, 0, 0);
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Player_ClearVelocity(em);
            em->timer = 450;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_CRASH_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndState(em);
        }
        break;
    }
}

/* YAMA_ATK_TENTACLE_HIT_A, as EmYamaTsukami_TentacleSlamA with one hit. */
void EmYamaTsukami_TentacleHitA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_TENTACLE_HIT_A, 0, 0);
        EmYamaTsukami_InitTentacleDust(em);
        break;
    case 1:
        EmYamaTsukami_UpdateTentacleDust(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/* YAMA_ATK_TENTACLE_HIT_B, as EmYamaTsukami_TentacleHitA. */
void EmYamaTsukami_TentacleHitB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_TENTACLE_HIT_B, 0, 0);
        EmYamaTsukami_InitTentacleDust(em);
        break;
    case 1:
        EmYamaTsukami_UpdateTentacleDust(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_ATK_INHALE: a strike, then the inhale for 240 frames; at its frame
 * 100 it makes the inhale's attack (YamaTsukamiAttack type 16) and the
 * pull toward its mouth (EMFX_YAMA_INHALE).
 */
void EmYamaTsukami_Inhale(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_STRIKE_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 240;
            Em_SetMotion(em, YAMA_MOT_INHALE, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 100.0f) != 0) {
            em->step++;
            YamaTsukamiAttack_Create(em, 16);
            EmEffectAttack_Create(em, EMFX_YAMA_INHALE, 0, 0, 0);
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_INHALE_END, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/* YAMA_ATK_STRIKE_A: the strike, a 10-frame pause, then the recover motion. */
void EmYamaTsukami_StrikeA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_STRIKE_A, 0, 0);
        em->timer = 10;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, YAMA_MOT_STRIKE_RECOVER, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_ATK_STRIKE_B: the strike for 120 frames, held 200 frames, then the
 * recover motion.
 */
void EmYamaTsukami_StrikeB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        em->timer = 120;
        Em_SetMotion(em, YAMA_MOT_STRIKE_B, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 200;
            Em_SetMotion(em, YAMA_MOT_STRIKE_B_HOLD, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_STRIKE_RECOVER, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/* Runs the current state of group 3 (EM_GROUP_ATTACK). */
void EmYamaTsukami_RunAttackState(Em *em)
{
    switch (em->state) {
    case YAMA_ATK_SUMMON:
        EmYamaTsukami_Summon(em);
        break;
    case YAMA_ATK_SPIN_TO_HEIGHT:
        EmYamaTsukami_Spin(em, 0);
        break;
    case YAMA_ATK_HOLD_POSE:
        EmYamaTsukami_HoldPose(em);
        break;
    case YAMA_ATK_TENTACLE_SLAM_A:
        EmYamaTsukami_TentacleSlamA(em);
        break;
    case YAMA_ATK_TENTACLE_SLAM_B:
        EmYamaTsukami_TentacleSlamB(em);
        break;
    case YAMA_ATK_TENTACLE_SLAM_C:
        EmYamaTsukami_TentacleSlamC(em);
        break;
    case YAMA_ATK_TENTACLE_SLAM_D:
        EmYamaTsukami_TentacleSlamD(em);
        break;
    case YAMA_ATK_SMASH:
        EmYamaTsukami_Smash(em, 0);
        break;
    case YAMA_ATK_DIVE_SLAM:
        EmYamaTsukami_DiveSlam(em);
        break;
    case YAMA_ATK_CRASH:
        EmYamaTsukami_Crash(em);
        break;
    case YAMA_ATK_TENTACLE_HIT_A:
        EmYamaTsukami_TentacleHitA(em);
        break;
    case YAMA_ATK_TENTACLE_HIT_B:
        EmYamaTsukami_TentacleHitB(em);
        break;
    case YAMA_ATK_INHALE:
        EmYamaTsukami_Inhale(em);
        break;
    case YAMA_ATK_STRIKE_A:
        EmYamaTsukami_StrikeA(em);
        break;
    case YAMA_ATK_STRIKE_B:
        EmYamaTsukami_StrikeB(em);
        break;
    case YAMA_ATK_SPIN:
        EmYamaTsukami_Spin(em, 1);
        break;
    case YAMA_ATK_SMASH_FORT:
        EmYamaTsukami_Smash(em, 1);
        
    default:
        break;
    }
}

/*
 * YAMA_REACT_FALL_LAND: falls (at least 50 a frame, gravity 10), lands with
 * a timed mine point at its position, then lies down (YAMA_IDLE_DOWNED).
 */
void EmYamaTsukami_FallLand(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, YAMA_MOT_FALL, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, YAMA_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
            EmMine_CreateTimed0(em, &em->posX);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE_DOWNED, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE_DOWNED, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * YAMA_REACT_FALL_RECOVER: falls as EmYamaTsukami_FallLand but at most
 * 1000, catches itself on its curve and pauses in the air
 * (YAMA_FLY_RECOVER_PAUSE).
 */
void EmYamaTsukami_FallRecover(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, YAMA_MOT_FALL, 0, 0);
        if (!(em->velY <= -50.0f)) {
            em->velY = -50.0f;
        }
        em->goalDist = 1000.0f;
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        em->accelZ = 0.0f;
        break;
    case 1:
        Player_Integrate(em);
        em->goalDist -= em->velY;
        if (em->goalDist <= 0.0f || em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, YAMA_MOT_SOFT_LAND, 0, 0);
            Player_ClearVelocity(em);
            EmYamaTsukami_StartCurves(em, YAMA_CURVE_SOFT_LAND);
        }
        break;
    case 2:
        EmYamaTsukami_UpdateCurves(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            Player_ClearVelocity(em);
            EmYamaTsukami_ChangeState(em, EM_GROUP_FLY, YAMA_FLY_RECOVER_PAUSE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_FLY, YAMA_FLY_RECOVER_PAUSE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
    }
}

/*
 * Flinch A: variant 0 in the air (YAMA_REACT_FLINCH_A), ending with
 * EmYamaTsukami_EndFlyState; 1 on the ground (YAMA_REACT_GROUND_FLINCH_A),
 * ending with EmYamaTsukami_EndState.
 */
void EmYamaTsukami_FlinchA(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        if (variant == 0) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_GROUND;
        }
        Em_SetMotion(em, YAMA_MOT_FLINCH_A, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            if (variant == 0) {
                EmYamaTsukami_EndFlyState(em);
                break;
            }
            EmYamaTsukami_EndState(em);
        } else {
        }
        break;
    }
}

/*
 * Flinch B, as EmYamaTsukami_FlinchA (YAMA_REACT_FLINCH_B,
 * YAMA_REACT_GROUND_FLINCH_B).
 */
void EmYamaTsukami_FlinchB(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_FLINCH_B, 0, 0);
        if (variant == 0) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_GROUND;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (variant == 0) {
                EmYamaTsukami_EndFlyState(em);
                break;
            }
            EmYamaTsukami_EndState(em);
        } else {
        }
        break;
    }
}

/*
 * Flinch C, as EmYamaTsukami_FlinchA; the body shrinks a step meanwhile
 * (EmYamaTsukami_SetReactState). Both sizes' states run it.
 */
void EmYamaTsukami_FlinchC(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_FLINCH_C, 0, 0);
        if (variant == 0) {
            em->stance = EM_STANCE_AIR;
        } else {
            em->stance = EM_STANCE_GROUND;
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (variant == 0) {
                EmYamaTsukami_EndFlyState(em);
                break;
            }
            EmYamaTsukami_EndState(em);
        } else {
        }
        break;
    }
}

/*
 * YAMA_REACT_FLINCH_D: one flinch motion in the air, then
 * EmYamaTsukami_EndFlyState.
 */
void EmYamaTsukami_FlinchD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_FLINCH_D, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/* YAMA_REACT_FLINCH_D_B, the same as EmYamaTsukami_FlinchD. */
void EmYamaTsukami_FlinchDB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_FLINCH_D, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_EndFlyState(em);
        }
        break;
    }
}

/*
 * YAMA_REACT_DOWNED_FLINCH: a flinch while lying down, then back to
 * YAMA_IDLE_DOWNED.
 */
void EmYamaTsukami_DownedFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, YAMA_MOT_DOWNED_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE_DOWNED, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_IDLE, YAMA_IDLE_DOWNED, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * YAMA_REACT_FLINCH_HOVER: a flinch during the pause after a short fall,
 * then back to YAMA_FLY_RECOVER_PAUSE.
 */
void EmYamaTsukami_FlinchHover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_FLINCH_HOVER, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmYamaTsukami_ChangeState(em, EM_GROUP_FLY, YAMA_FLY_RECOVER_PAUSE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_FLY, YAMA_FLY_RECOVER_PAUSE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* Runs the current state of group 4 (EM_GROUP_REACT). */
void EmYamaTsukami_RunReactState(Em *em)
{
    switch (em->state) {
    case YAMA_REACT_FALL_LAND:
        EmYamaTsukami_FallLand(em);
        break;
    case YAMA_REACT_FALL_RECOVER:
        EmYamaTsukami_FallRecover(em);
        break;
    case YAMA_REACT_FLINCH_A:
        EmYamaTsukami_FlinchA(em, 0);
        break;
    case YAMA_REACT_FLINCH_B:
        EmYamaTsukami_FlinchB(em, 0);
        break;
    case YAMA_REACT_FLINCH_C_FROM_FULL:
        EmYamaTsukami_FlinchC(em, 0);
        break;
    case YAMA_REACT_FLINCH_C_FROM_HALF:
        EmYamaTsukami_FlinchC(em, 0);
        break;
    case YAMA_REACT_FLINCH_D:
        EmYamaTsukami_FlinchD(em);
        break;
    case YAMA_REACT_FLINCH_D_B:
        EmYamaTsukami_FlinchDB(em);
        break;
    case YAMA_REACT_DOWNED_FLINCH:
        EmYamaTsukami_DownedFlinch(em);
        break;
    case YAMA_REACT_GROUND_FLINCH_A:
        EmYamaTsukami_FlinchA(em, 1);
        break;
    case YAMA_REACT_GROUND_FLINCH_B:
        EmYamaTsukami_FlinchB(em, 1);
        break;
    case YAMA_REACT_GROUND_FLINCH_C_FROM_FULL:
        EmYamaTsukami_FlinchC(em, 1);
        break;
    case YAMA_REACT_GROUND_FLINCH_C_FROM_HALF:
        EmYamaTsukami_FlinchC(em, 1);
        break;
    case YAMA_REACT_FLINCH_HOVER:
        EmYamaTsukami_FlinchHover(em);
        
    default:
        break;
    }
}

/*
 * YAMA_DEFEAT_DIE: the death on the ground (QuestEm_Hunted), then the
 * carcass's carving (EmMine_Create, EmMine_Update). Step 99, set from
 * outside this code, removes the mine points, gives it 13333 hp and returns
 * it to YAMA_IDLE.
 */
void EmYamaTsukami_Die(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, YAMA_MOT_DIE, 0, 0);
        em->timer = 0;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        break;
    case 99:
        em->step++;
        EmYamaTsukami_RemoveMinePoints(em);
        em->hp = 13333;
        EmYamaTsukami_EndState(em);
        break;
    }
}

/*
 * YAMA_DEFEAT_DIE_FALLING: falls on its curve, lands, plays the death and
 * carves as EmYamaTsukami_Die, with the same step 99.
 */
void EmYamaTsukami_DieFalling(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_FALL, 0, 0);
        QuestEm_Hunted(em);
        EmYamaTsukami_StartCurves(em, YAMA_CURVE_DIE_FALLING);
        em->accelX = 0.0f;
        em->accelY = -10.0f;
        *(s32 *) &em->accelZ = 0; /* stored as the integer 0, as the game does */
        break;
    case 1:
        EmYamaTsukami_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_DEAD;
            Em_SetMotion(em, YAMA_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            Em_SetMotion(em, YAMA_MOT_DIE, 0, 0);
        }
        break;
    case 3:
        EmMine_Update(em);
        break;
    case 99:
        em->step++;
        em->hp = 13333;
        EmYamaTsukami_RemoveMinePoints(em);
        EmYamaTsukami_EndState(em);
        break;
    }
}

/* Runs the current state of group 5 (EM_GROUP_DEFEAT). */
void EmYamaTsukami_RunDefeatState(Em *em)
{
    switch (em->state) {
    case YAMA_DEFEAT_DIE:
        EmYamaTsukami_Die(em);
        break;
    case YAMA_DEFEAT_DIE_FALLING:
        EmYamaTsukami_DieFalling(em);
        break;
    }
}

/*
 * YAMA_EVENT_TOWER_FLYBY, the tower's opening scene: three passes between
 * fixed points at 20 a frame, each until its goal is within 100 or its time
 * runs out (300, 300 and 900 frames). Then, once EVENT_FLAG_YAMA_TSUKAMI_TOWER_SCENE is set,
 * the monster moves to its place in AREA_TOWER_3 and waits for the hunters
 * (YAMA_FLY_WAIT_FOR_HUNTERS). Untouchable and blind throughout.
 */
void EmYamaTsukami_TowerFlyby(Em *em)
{
    f32 distFirst;
    f32 distSecond;
    f32 distThird;
    u8 step;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    step = em->step;
    switch (step) {
    case 0:
        GmEm_ResetSize();
        em->step++;
        em->scriptPending = 0;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, YAMA_MOT_MOVE, 0, 0);
        em->timer = 300;
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->posX = 12650.0f;
        em->posY = 3360.0f;
        em->posZ = 8900.0f;
        em->goal[0] = 4010.0f;
        em->goal[1] = 3360.0f;
        em->goal[2] = 10544.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        Em_MoveRotated(em, &em->rotX);
        distFirst = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer > 0 && !(distFirst <= 100.0f)) {
            break;
        }
        em->step++;
        em->timer = 300;
        em->posX = 12650.0f;
        em->posY = 3360.0f;
        em->posZ = 8900.0f;
        em->goal[0] = 695.0f;
        em->goal[1] = 3360.0f;
        em->goal[2] = 11203.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 2:
        Em_MoveRotated(em, &em->rotX);
        distSecond = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || distSecond <= 100.0f) {
            em->step++;
            em->timer = 900;
            em->posX = 4123.0f;
            em->posY = 5000.0f;
            em->posZ = 14785.0f;
            em->goal[0] = -3000.0f;
            em->goal[1] = 5000.0f;
            em->goal[2] = 8036.0f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        }
        break;
    case 3:
        Em_MoveRotated(em, &em->rotX);
        distThird = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || distThird <= 100.0f) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_IDLE, 0, 0);
        }
        break;
    case 4:
        if (Save_TestEventFlag(EVENT_FLAG_YAMA_TSUKAMI_TOWER_SCENE) == 1) {
            em->area = Area_IdForCurrentTime(AREA_TOWER_3);
            em->rotY = 0x4FE0;
            em->posX = 4510.0f;
            em->posY = -2619.0f;
            em->posZ = 12860.0f;
            EmYamaTsukami_Setup(em);
            EmYamaTsukami_ChangeState(em, EM_GROUP_FLY, YAMA_FLY_WAIT_FOR_HUNTERS, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * YAMA_EVENT_WALK_IN: from a fixed point on the ground, plays the opening
 * motions, turns for 64 frames, then walks to two fixed goals in turn and
 * floats (EmYamaTsukami_EndFlyState). Untouchable and blind throughout.
 */
void EmYamaTsukami_WalkIn(Em *em)
{
    f32 pos[3];
    s32 turn;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_FAMILY;
        Em_SetMotion(em, YAMA_MOT_FLINCH_D, 0, 0);
        em->posX = 6070.0f;
        em->posY = 0.0f;
        em->posZ = 17350.0f;
        em->goal[0] = 3600.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 19500.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) - ANGLE_TURN / 4;
        em->goalDist = Vec_DistanceXZ(&em->posX, em->goal) - 500.0f;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_EVENT_TURN, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].blend == 0) {
            if (!EmMotion_ReachedFrame(em, 0, 64.0f)) {
                em->rotY += 0x210;
            }
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetMotion(em, YAMA_MOT_DESCEND, 0, 0);
                Em_SetMotion(em, YAMA_MOT_FLINCH_HOVER, 0, 0);
            }
        }
        break;
    case 3:
        if (em->motion[1].playing == 0 && em->motionId[1] == EM_MOTION_ID(1, YAMA_MOT_FLINCH_HOVER)) {
            Em_SetLayerMotion(em, EM_MOTION_ID(1, YAMA_MOT_DESCEND), 0, 0, 1);
        }
        turn = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
        if (turn <= 0x8000) {
            if (turn <= 63) {
                em->rotY += turn;
            } else {
                em->rotY += 64;
            }
        } else {
            if (turn > 0xFFC0) {
                em->rotY += turn;
            } else {
                em->rotY -= 64;
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist -= pos[2]) <= 0.0f) {
            em->goal[0] = 430.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 23930.0f;
            em->step++;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case 4:
        turn = ((Math_AngleXZ(&em->posX, em->goal) & 0xFFFF) - em->rotY) & 0xFFFF;
        if (turn <= 0x8000) {
            if (turn <= 63) {
                em->rotY += turn;
            } else {
                em->rotY += 64;
            }
        } else {
            if (turn > 0xFFC0) {
                em->rotY += turn;
            } else {
                em->rotY -= 64;
            }
        }
        Em_MotionDelta(em, pos);
        if ((em->goalDist -= pos[2]) <= 0.0f) {
            em->step++;
            Em_SetMotion(em, YAMA_MOT_DESCEND, 0, 0);
        }
        break;
    case 5:
        em->step++;
        EmYamaTsukami_EndFlyState(em);
        break;
    }
}

/* Runs the current state of group 6 (EM_GROUP_EVENT). */
void EmYamaTsukami_RunEventState(Em *em)
{
    switch (em->state) {
    case YAMA_EVENT_TOWER_FLYBY:
        EmYamaTsukami_TowerFlyby(em);
        break;
    case YAMA_EVENT_WALK_IN:
        EmYamaTsukami_WalkIn(em);
        break;
    }
}

/*
 * The monster's update each frame: keeps the carcass mine point on the body
 * (and drops it once carved out), sends its state every 150 frames (the
 * first kind; the second stays blind), counts quakeTimer down, reacts to
 * the damage result (death; a part's flinch by stage, size and stance; the
 * stun's end), breaks part 0 once part 6 is broken, starts the tower scene
 * or runs the script, runs the state (twice on a request), keeps guardTimer
 * while flyGuard is set, and shakes the ground except in the two quiet
 * motions.
 */
void EmYamaTsukami_Update(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;
    f32 pos[3];
    u8 dmgInfo[4];

    if (w->mineSlot != -1) {
        EmYamaTsukami_GetMinePointPos(em, pos);
        MinePoint_SetPos(w->mineSlot, pos);
        if ((w->mineCount = MinePoint_Count(w->mineSlot)) <= 0) {
            MinePoint_Remove(w->mineSlot);
            w->mineSlot = -1;
        }
    }
    if (em->type == EM_KIND_YAMA_TSUKAMI_B) {
        em->blind = 2;
    } else if (w->syncTimer != 0 && em->stateGroup != EM_GROUP_EVENT && em->remote == 0) {
        if (--w->syncTimer <= 0) {
            w->syncTimer = 150;
            QSync_SendEm(em, QSYNC_EMMSG_STATE_NO_PARTS, 0);
        }
    }
    if (em->quakeTimer != 0) {
        if (--em->quakeTimer <= 0) {
            em->quakeTimer = 0;
        }
    }
    switch (Em_UpdateDamage(em, dmgInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_STATUS_IN_AIR:
    case EM_DMG_STATUS4:
    case EM_DMG_CAPTURED:
    case EM_DMG_STATUS2:
    case EM_DMG_FLASH:
    case EM_DMG_HIT_LYING:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_GROUND) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_DEFEAT, YAMA_DEFEAT_DIE, EM_SYNC_OWNER);
        } else {
            EmYamaTsukami_ChangeState(em, EM_GROUP_DEFEAT, YAMA_DEFEAT_DIE_FALLING, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    case EM_DMG_PART_FLINCH:
        if (em->stance == EM_STANCE_GROUND) {
            if (em->stateGroup == EM_GROUP_IDLE && em->state == YAMA_IDLE_DOWNED) {
                EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_DOWNED_FLINCH, EM_SYNC_OWNER);
            } else {
                switch (w->flinchStage) {
                case YAMA_FLINCH_STAGE_A:
                    EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_GROUND_FLINCH_A, EM_SYNC_OWNER);
                    break;
                case YAMA_FLINCH_STAGE_B:
                    EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_GROUND_FLINCH_B, EM_SYNC_OWNER);
                    break;
                case YAMA_FLINCH_STAGE_C:
                    switch (w->bodySize) {
                    case YAMA_SIZE_SHRUNK:
                        EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_FALL_LAND, EM_SYNC_OWNER);
                        break;
                    case YAMA_SIZE_HALF:
                        EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_GROUND_FLINCH_C_FROM_HALF, EM_SYNC_OWNER);
                        break;
                    case YAMA_SIZE_FULL:
                        EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_GROUND_FLINCH_C_FROM_FULL, EM_SYNC_OWNER);
                        break;
                    }
                    break;
                }
            }
        } else if (em->stateGroup == EM_GROUP_FLY && em->state == YAMA_FLY_RECOVER_PAUSE) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_FLINCH_HOVER, EM_SYNC_OWNER);
        } else {
            switch (w->flinchStage) {
            case YAMA_FLINCH_STAGE_A:
                EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_FLINCH_A, EM_SYNC_OWNER);
                break;
            case YAMA_FLINCH_STAGE_B:
                EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_FLINCH_B, EM_SYNC_OWNER);
                break;
            case YAMA_FLINCH_STAGE_C:
                switch (w->bodySize) {
                case YAMA_SIZE_SHRUNK:
                    if (!(em->posY <= 2000.0f + em->floorY)) {
                        EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_FALL_RECOVER, EM_SYNC_OWNER);
                    } else {
                        EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_FALL_LAND, EM_SYNC_OWNER);
                    }
                    break;
                case YAMA_SIZE_HALF:
                    EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_FLINCH_C_FROM_HALF, EM_SYNC_OWNER);
                    break;
                case YAMA_SIZE_FULL:
                    EmYamaTsukami_ChangeState(em, EM_GROUP_REACT, YAMA_REACT_FLINCH_C_FROM_FULL, EM_SYNC_OWNER);
                    break;
                }
                break;
            }
        }
        break;
    }
    if (em->parts[6].state >= 8) {
        Em_BreakPart(em, 0);
    }
    if (em->type == EM_KIND_YAMA_TSUKAMI && Area_DayId(em->area) == AREA_TOWER_0 && Save_TestEventFlag(EVENT_FLAG_YAMA_TSUKAMI_TOWER_SCENE) == 0) {
        if (Area_DayId(gScene.area) == AREA_TOWER_0 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
            EmYamaTsukami_ChangeState(em, EM_GROUP_EVENT, YAMA_EVENT_TOWER_FLYBY, EM_SYNC_LOCAL);
        }
    } else {
        Em_UpdatePendingScript(em);
    }
    Em_ClearHitReactMode(em);
    EmYamaTsukami_RunState(em);
    if (em->stateRequest != 0) {
        EmYamaTsukami_RunState(em);
        em->stateRequest = 0;
    }
    if (w->flyGuard != 0) {
        em->guardTimer = 2;
    }
    if (EmYamaTsukami_IsQuakeFreeMotion(em) == 1) {
        em->quakeTimer = 0;
    } else {
        em->quakeTimer = 5;
    }
}

/* Runs the current state by group; group 7 runs group 6's states. */
void EmYamaTsukami_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmYamaTsukami_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmYamaTsukami_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmYamaTsukami_RunFlyState(em);
        break;
    case EM_GROUP_ATTACK:
        EmYamaTsukami_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmYamaTsukami_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmYamaTsukami_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmYamaTsukami_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmYamaTsukami_RunEventState(em);
        
    default:
        break;
    }
}

/* The effects of the motions with none: nothing. */
static void EmYamaTsukami_MotionEffectsDefault(Em *em)
{
}

/*
 * Plays the motion sounds and makes the effects and attacks of the motion
 * on layer 0 at their frames: the YamaTsukamiAttack hits of the slams,
 * strikes and the landing, impact attacks, the dust and debris stage
 * objects, and the dive slam's impact under YAMA_BONE_BODY (when a player
 * is in the area). Keeps the motion in fxMotion.
 */
void EmYamaTsukami_MotionEffects(Em *em, EmWork *work)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) work;
    f32 pos[3];

    if (em->motionId[0] != w->fxMotion) {
        w->unk00A = 0;
        w->fxMotion = (s16) em->motionId[0];
    }
    EmYamaTsukami_MotionSounds(em, (EmWork *) w);
    switch (w->fxMotion) {
    case EM_MOTION_ID(0, YAMA_MOT_SURGE_BACK):
        if (EmMotion_PassedFrameScaled(em, 2.0f, 0) != 0) {
            EmImpactAttack_CreateAt(em, &em->posX, 10);
        }
        if (EmMotion_PassedFrameScaled(em, 100.0f, 0) != 0) {
            EmImpactAttack_CreateAt(em, &em->posX, 10);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 36.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 4.0f * em->scaleX);
            YamaTsukamiAttack_Create(em, 20);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SOFT_LAND):
        break;
    case EM_MOTION_ID(0, YAMA_MOT_GET_UP):
        if (EmMotion_PassedFrameScaled(em, 74.0f, 0) != 0) {
            DustCloud_Create(em, 19, 0, 4.0f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_SLAM_A):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 3);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_SLAM_B):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 4);
        }
        if (EmMotion_PassedFrameScaled(em, 134.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 5);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_SLAM_C):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 6);
        }
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 7);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_SLAM_D):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 8);
        }
        if (EmMotion_PassedFrameScaled(em, 124.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 9);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_SMASH):
        if (EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 10);
            YamaTsukamiAttack_Create(em, 21);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_DIVE_SLAM):
        if (EmMotion_PassedFrameScaled(em, 70.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 22);
        }
        if ((EmMotion_PassedFrameScaled(em, 156.0f, 0) != 0) && (Player_InCurrentArea(em) & 0xFF)) {
            Em_BonePos(em, YAMA_BONE_BODY, pos);
            pos[1] -= 300.0f * em->scaleY;
            DustCloud_CreateAt(pos, 21, 0, 4.3f * em->scaleX);
            DustCloud_CreateAt(pos, 13, 0, 4.3f * em->scaleX);
            EmImpactAttack_CreateAt(em, pos, 11);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_CRASH_LIE):
        if (EmMotion_PassedFrameScaled(em, 86.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 11);
        }
        if (EmMotion_PassedFrameScaled(em, 122.0f, 0) != 0) {
            DustCloud_Create(em, 11, 0, 4.0f * em->scaleX);
            YamaTsukamiAttack_Create(em, 19);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_CRASH_GET_UP):
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0) != 0) {
            DustCloud_Create(em, 19, 0, 4.0f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_DIE):
        if (EmMotion_PassedFrameScaled(em, 182.0f, 0) != 0) {
            DustCloud_Create(em, 0, 0, 4.0f * em->scaleX);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_HIT_A):
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 12);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_TENTACLE_HIT_B):
        if (EmMotion_PassedFrameScaled(em, 56.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 13);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_STRIKE_A):
        if (EmMotion_PassedFrameScaled(em, 50.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 14);
            YamaTsukamiAttack_Create(em, 21);
        }
        break;
    case EM_MOTION_ID(0, YAMA_MOT_STRIKE_B):
        if (EmMotion_PassedFrameScaled(em, 42.0f, 0) != 0) {
            YamaTsukamiAttack_Create(em, 15);
            YamaTsukamiAttack_Create(em, 21);
        }
        break;
    default:
        EmYamaTsukami_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Eases bodyScale a 85th of the way toward the row of
 * EmYamaTsukami_SizeScales for bodySize, each frame (easeMode 0).
 */
void EmYamaTsukami_EaseToSize(Em *em, EmWork *work)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)work;

    switch (w->easeMode) {
    case 0:
        /* The table is read through a cast to rows of three: declared two-dimensional, it compiles differently. */
        w->bodyScale[0] = w->bodyScale[0] + (((f32 (*)[3]) EmYamaTsukami_SizeScales)[w->bodySize][0] - w->bodyScale[0]) / 85.0f;
        w->bodyScale[1] = w->bodyScale[1] + (((f32 (*)[3]) EmYamaTsukami_SizeScales)[w->bodySize][1] - w->bodyScale[1]) / 85.0f;
        w->bodyScale[2] = w->bodyScale[2] + (((f32 (*)[3]) EmYamaTsukami_SizeScales)[w->bodySize][2] - w->bodyScale[2]) / 85.0f;
        break;
    }
}

/*
 * Swells the body: adds EmYamaTsukami_BoneGrowth (x, y, z per bone; the last value is
 * EmYamaTsukami_BoneGrowthLast) times bodyScale[0], [1] and [2] to the scale of
 * YAMA_BONE_SWELL_A, _B and _C. Em_AdjustKindBones calls it for the family when it poses the bones.
 */
void EmYamaTsukami_ScaleBones(Em *em, EmBone *bones)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    bones[YAMA_BONE_SWELL_A].pose[0] = bones[YAMA_BONE_SWELL_A].pose[0] + EmYamaTsukami_BoneGrowth[0] * w->bodyScale[0];
    bones[YAMA_BONE_SWELL_A].pose[1] = bones[YAMA_BONE_SWELL_A].pose[1] + EmYamaTsukami_BoneGrowth[1] * w->bodyScale[0];
    bones[YAMA_BONE_SWELL_A].pose[2] = bones[YAMA_BONE_SWELL_A].pose[2] + EmYamaTsukami_BoneGrowth[2] * w->bodyScale[0];
    bones[YAMA_BONE_SWELL_B].pose[0] = bones[YAMA_BONE_SWELL_B].pose[0] + EmYamaTsukami_BoneGrowth[3] * w->bodyScale[1];
    bones[YAMA_BONE_SWELL_B].pose[1] = bones[YAMA_BONE_SWELL_B].pose[1] + EmYamaTsukami_BoneGrowth[4] * w->bodyScale[1];
    bones[YAMA_BONE_SWELL_B].pose[2] = bones[YAMA_BONE_SWELL_B].pose[2] + EmYamaTsukami_BoneGrowth[5] * w->bodyScale[1];
    bones[YAMA_BONE_SWELL_C].pose[0] = bones[YAMA_BONE_SWELL_C].pose[0] + EmYamaTsukami_BoneGrowth[6] * w->bodyScale[2];
    bones[YAMA_BONE_SWELL_C].pose[1] = bones[YAMA_BONE_SWELL_C].pose[1] + EmYamaTsukami_BoneGrowth[7] * w->bodyScale[2];
    bones[YAMA_BONE_SWELL_C].pose[2] = bones[YAMA_BONE_SWELL_C].pose[2] + EmYamaTsukami_BoneGrowthLast * w->bodyScale[2];
}

/*
 * The family's per-frame effects hook: skips the first frame, then runs
 * EmYamaTsukami_MotionEffects and EmYamaTsukami_EaseToSize.
 */
void EmYamaTsukami_UpdateMotionEffects(Em *em)
{
    EmYamaTsukamiWork *work = (EmYamaTsukamiWork *)&em->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        EmYamaTsukami_MotionEffects(em, (EmWork *) work);
        EmYamaTsukami_EaseToSize(em, (EmWork *) work);
        break;
    }
}

/*
 * The fortress impact of YAMA_ATK_SMASH_FORT at a fixed point of the area
 * (one for AREA_BATTLE_TOWN_1, one for the others): two sounds, the impact
 * effects, the debris and a camera shake.
 */
void EmYamaTsukami_SpawnFortImpact(Em *em)
{
    f32 pos[3];

    switch (Area_DayId(em->area)) {
    case AREA_BATTLE_TOWN_1:
        pos[0] = 10069.0f;
        pos[1] = 1630.0f;
        pos[2] = 16847.0f;
        break;
    default:
        pos[0] = 7300.0f;
        pos[1] = 1500.0f;
        pos[2] = 12400.0f;
        break;
    }
    Em_PlaySeChecked(em, YAMA_SE_FORT_IMPACT_A, 0, pos, 9, 0);
    Em_PlaySeChecked(em, YAMA_SE_FORT_IMPACT_B, 0, pos, 9, 0);
    HitSpark_SpawnForOwner(em, 0, 9, 0, pos, 3.0f);
    HitSpark_SpawnForOwner(em, 0, 9, 1, pos, 3.0f);
    PartFx_SpawnIfHere(em, pos, 34, 30.0f);
    CamShake_ReqARange(em, 2, 2);
}

/*
 * Starts the tentacle dust: for each bone of EmYamaTsukami_TentacleBones, 1
 * if it is above the floor, else 0 (all 0 when the player's area is
 * another).
 */
void EmYamaTsukami_InitTentacleDust(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) &em->work;
    s32 i;
    s32 j;
    f32 pos[3];

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            if (gScene.area != em->area) {
                w->dustState[i][j] = 0;
            } else {
                Em_BonePos(em, EmYamaTsukami_TentacleBones[i][j], pos);
                if (pos[1] <= AreaGrid2_GetFloorY(pos)) {
                    w->dustState[i][j] = 0;
                } else {
                    w->dustState[i][j] = 1;
                }
            }
        }
    }
}

/*
 * Makes the dust where a tentacle reaches the floor: a bone that was above
 * the floor and is now at or under it (the floor is looked for up to 1000
 * above the body) makes the dust there, and its pair is done. Only in the
 * player's area.
 */
void EmYamaTsukami_UpdateTentacleDust(Em *em)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *) &em->work;
    s32 i;
    s32 j;
    f32 pos[3];
    f32 floorY;
    f32 boneY;
    s8 *state;

    if (gScene.area != em->area) {
        return;
    }
    for (i = 0; i < 4; i++) {
        for (j = 0; j < 2; j++) {
            Em_BonePos(em, EmYamaTsukami_TentacleBones[i][j], pos);
            floorY = AreaGrid2_GetFloorY(pos);
            state = &w->dustState[i][j];
            switch (*state) {
            case 0:
                if (!(pos[1] <= floorY)) {
                    *state = 1;
                }
                break;
            case 1:
                boneY = pos[1];
                if (!(boneY <= floorY)) {
                    pos[1] = 1000.0f + em->posY;
                    floorY = AreaGrid2_GetFloorY(pos);
                    pos[1] = boneY;
                }
                if (pos[1] <= floorY) {
                    pos[1] = floorY;
                    PartFx_SpawnAt(pos, 3, 10.0f * em->scaleX);
                    w->dustState[i][1] = 2;
                    w->dustState[i][0] = 2;
                }
                break;
            case 2:
                break;
            }
        }
    }
}

/*
 * Returns 1 in the two motions that don't shake the ground
 * (YAMA_MOT_RECOVER_PAUSE, YAMA_MOT_FLINCH_HOVER), else 0.
 */
s32 EmYamaTsukami_IsQuakeFreeMotion(Em *em)
{
    u16 motionId;

    motionId = em->motionId[0];
    if ((motionId == EM_MOTION_ID(0, YAMA_MOT_RECOVER_PAUSE)) || (motionId == EM_MOTION_ID(0, YAMA_MOT_FLINCH_HOVER))) {
        return 1;
    }
    return 0;
}

/* An empty hook of the family's table. */
void EmYamaTsukami_TableNop(void)
{
}

/*
 * The script's question hook: kind 1 asks whether the monster is in
 * AREA_TOWER_0x53 or AREA_TOWER_3 (1) or elsewhere (0); every other kind
 * gets queryValue.
 */
u8 EmYamaTsukami_ScriptQuery(Em *em, s32 kind)
{
    EmYamaTsukamiWork *w = (EmYamaTsukamiWork *)&em->work;

    if ((kind & 0xFF) != 1) {
        return (u8)w->queryValue;
    }
    switch (Area_DayId(em->area) & 0xFFFF) {
    case AREA_TOWER_0x53:
    case AREA_TOWER_3:
        return 1U;
    }
    return 0U;
}


/* The file's initialised data, in the game's order */
EmCurveKey EmYamaTsukami_VelYCurves_0[10] = {
#include "game/008CA370.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_1[10] = {
#include "game/008CA3C0.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_2[12] = {
#include "game/008CA410.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_3[12] = {
#include "game/008CA470.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_6[12] = {
#include "game/008CA4D0.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_7[12] = {
#include "game/008CA530.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_8[18] = {
#include "game/008CA590.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_9[18] = {
#include "game/008CA620.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_10[18] = {
#include "game/008CA6B0.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_11[18] = {
#include "game/008CA740.inc"
};
EmCurveKey EmYamaTsukami_VelYCurves_12[18] = {
#include "game/008CA7D0.inc"
};
EmCurveKey EmYamaTsukami_SpeedCurves_4[12] = {
#include "game/008CA860.inc"
};
EmCurveKey EmYamaTsukami_SpeedCurves_5[14] = {
#include "game/008CA8C0.inc"
};
EmCurveKey *EmYamaTsukami_VelYCurves[16] = {
    EmYamaTsukami_VelYCurves_0, EmYamaTsukami_VelYCurves_1, EmYamaTsukami_VelYCurves_2, EmYamaTsukami_VelYCurves_3,
    NULL, NULL, EmYamaTsukami_VelYCurves_6, EmYamaTsukami_VelYCurves_7,
    EmYamaTsukami_VelYCurves_8, EmYamaTsukami_VelYCurves_9, EmYamaTsukami_VelYCurves_10, EmYamaTsukami_VelYCurves_11,
    EmYamaTsukami_VelYCurves_12, NULL, NULL, NULL,
};
EmCurveKey *EmYamaTsukami_SpeedCurves[14] = {
    NULL, NULL, NULL, NULL,
    EmYamaTsukami_SpeedCurves_4, EmYamaTsukami_SpeedCurves_5, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL,
};
f32 EmYamaTsukami_KindBStartPos[3] = {
#include "game/008CA9A8.inc"
};
EmYamaTsukamiYawKey EmYamaTsukami_SpinStartYawKeys[12] = {
#include "game/008CA9C0.inc"
};
EmYamaTsukamiYawKey EmYamaTsukami_SpinEndYawKeys[11] = {
#include "game/008CAA20.inc"
};
EmYamaTsukamiYawKey *EmYamaTsukami_SpinYawKeys[2] = {
    EmYamaTsukami_SpinStartYawKeys, EmYamaTsukami_SpinEndYawKeys,
};
f32 EmYamaTsukami_SizeScales[4][3] = {
#include "game/008CAA80.inc"
};
f32 EmYamaTsukami_BoneGrowth[8] = {
#include "game/008CAAB0.inc"
};
f32 EmYamaTsukami_BoneGrowthLast = {
#include "game/008CAAD0.inc"
};
s8 EmYamaTsukami_TentacleBones[4][2] = {
#include "game/008CAAD8.inc"
};
void *EmYamaTsukami_Callbacks[8] = {
    EmYamaTsukami_SpawnEffects, EmYamaTsukami_SpawnEffects, EmYamaTsukami_TableNop, EmYamaTsukami_UpdateMotionEffects,
    EmYamaTsukami_ScriptQuery, EmYamaTsukami_TableNop, NULL, NULL,
};
