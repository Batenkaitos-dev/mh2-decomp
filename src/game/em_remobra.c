/*
 * The Remobra family (EM_KIND_REMOBRA), the flying serpents: the code from
 * the family's first function, EmRemobra_MotionSounds, to its last,
 * EmRemobra_TableNop. EmRemobra_Init sets a monster up, EmRemobra_Update
 * reads the hits, and EmRemobra_RunState calls the group's dispatcher
 * (EmRemobra_RunIdleState to EmRemobra_RunEvent2State), which calls one
 * function per state with the monster; a state function switches on
 * Em.step. Its flights follow the height and speed curves
 * (EmRemobra_StartCurves, EmRemobra_UpdateCurves), and EmRemobra_UpdateHop
 * bobs it up and down while it flies. The family keeps its own values in
 * the monster's work area (EmRemobraWork: home, the goal's distance and
 * yaw, the hop); em_remobra.h names its motions and states. The game's code
 * places the family between the Chameleos family's main code and its state
 * changes.
 */
#include "common.h"
#include "em.h"
#include "shadow.h"
#include "em_remobra.h"
#include "objattach.h"
#include "player.h"
#include "capsule.h"
#include "quest.h"
#include "sound.h"
#include "engine_math.h"
#include "gm_load.h"
#include "part_effects.h"
#include "rand.h"
#include "vec_math.h"
#include "area.h"
#include "em_script.h"
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmChameleos_ChangeState, EmChameleos_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
/* unit's own declaration: declared here as the callers use it; with the definition's void (void *obj, s16 bone, f32 *out), the calls in EmChameleos_SpawnGroundEffect, EmChameleos_SpawnGroundEffectOffset and others compile differently */
void Em_BonePos(void *obj, s32 a, f32 *out);
extern f32 EmRemobra_HopFrames[3];
extern EmCurveKey *EmRemobra_VelYCurves[];
extern EmCurveKey *EmRemobra_SpeedCurves[];
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
/* end of prototypes */


/*
 * Plays the sounds of the playing motion on their frames at their bones
 * (Em_PlaySeAtBoneOnFrame), switching on work->fxMotion (the layer-0 motion
 * ids of motions 1 to 26).
 */
void EmRemobra_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, REMOBRA_MOT_FLY):
        Em_PlaySeAtBoneOnFrame(em, 64, Sound_RandomSe(11, 2, 11, 2), 21);
        Em_PlaySeAtBoneOnFrame(em, 72, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 148, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 152, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 222, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 224, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, 2):
        Em_PlaySeAtBoneOnFrame(em, 2, 0, 21);
        Em_PlaySeAtBoneOnFrame(em, 118, 1, 21);
        Em_PlaySeAtBoneOnFrame(em, 80, 26, 3);
        Em_PlaySeAtBoneOnFrame(em, 104, 32, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 152, 32, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 116, 2, 21);
        Em_PlaySeAtBoneOnFrame(em, 244, 27, 21);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_SWOOP_END):
        Em_PlaySeAtBoneOnFrame(em, 6, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 10, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 22, 11, 21);
        Em_PlaySeAtBoneOnFrame(em, 38, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 42, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 88, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 92, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 128, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 132, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 222, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 224, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_SWOOP):
        Em_PlaySeAtBoneOnFrame(em, 0, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 21, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 43, 20, 3);
        break;
    case EM_MOTION_ID(0, 5):
        Em_PlaySeAtBoneOnFrame(em, 0, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 16, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 21, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 43, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 64, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 78, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 82, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 85, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 106, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 127, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 138, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 159, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 180, 20, 3);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_BANK_SLOW_A):
        Em_PlaySeAtBoneOnFrame(em, 0, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 21, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 43, 20, 3);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_BANK_SLOW_B):
        Em_PlaySeAtBoneOnFrame(em, 0, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 21, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 43, 20, 3);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_BANK_FAST_A):
        Em_PlaySeAtBoneOnFrame(em, 0, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 21, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 43, 20, 3);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_BANK_FAST_B):
        Em_PlaySeAtBoneOnFrame(em, 0, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 21, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 43, 20, 3);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_BACK_OFF):
        Em_PlaySeAtBoneOnFrame(em, 6, 3, 21);
        Em_PlaySeAtBoneOnFrame(em, 44, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 108, 30, REMOBRA_BONE_WING_A);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_TAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 18, 4, 21);
        Em_PlaySeAtBoneOnFrame(em, 28, 34, 29);
        Em_PlaySeAtBoneOnFrame(em, 48, 25, 25);
        Em_PlaySeAtBoneOnFrame(em, 68, 34, 25);
        Em_PlaySeAtBoneOnFrame(em, 128, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 132, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 188, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 192, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 242, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 246, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 290, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 294, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_SPIT_DOWN):
        Em_PlaySeAtBoneOnFrame(em, 20, 5, 21);
        Em_PlaySeAtBoneOnFrame(em, 0, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 21, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 43, 20, 3);
        break;
    case EM_MOTION_ID(0, 13):
        Em_PlaySeAtBoneOnFrame(em, 2, 5, 21);
        Em_PlaySeAtBoneOnFrame(em, 4, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 8, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_SPIT_AIM):
        Em_PlaySeAtBoneOnFrame(em, 20, 6, 21);
        Em_PlaySeAtBoneOnFrame(em, 72, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_SPIT):
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 21);
        Em_PlaySeAtBoneOnFrame(em, 8, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 12, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 6, 7, 21);
        Em_PlaySeAtBoneOnFrame(em, 14, 30, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 18, 30, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 34, 8, 21);
        Em_PlaySeAtBoneOnFrame(em, 38, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 21, REMOBRA_BONE_STRIKE);
        Em_PlaySeAtBoneOnFrame(em, 42, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 72, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 76, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 104, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 108, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 144, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 148, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_SWOOP_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 24, 9, 21);
        Em_PlaySeAtBoneOnFrame(em, 58, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 66, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 110, 38, 3);
        Em_PlaySeAtBoneOnFrame(em, 120, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 124, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 140, 4, 21);
        Em_PlaySeAtBoneOnFrame(em, 154, 41, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 158, 42, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 156, 43, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 160, 44, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 204, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 210, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 250, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 254, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 2, 22, 3);
        Em_PlaySeAtBoneOnFrame(em, 12, 2, 21);
        Em_PlaySeAtBoneOnFrame(em, 16, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 20, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 44, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 48, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 68, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 72, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 100, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 104, 29, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_FALL):
        Em_PlaySeAtBoneOnFrame(em, 0, 22, 3);
        Em_PlaySeAtBoneOnFrame(em, 2, 3, 21);
        Em_PlaySeAtBoneOnFrame(em, 18, 32, 3);
        Em_PlaySeAtBoneOnFrame(em, 50, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 54, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 64, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 84, 20, 3);
        Em_PlaySeAtBoneOnFrame(em, 98, 20, 3);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 0, 21);
        Em_PlaySeAtBoneOnFrame(em, 14, 26, 3);
        Em_PlaySeAtBoneOnFrame(em, 34, 33, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 66, 32, REMOBRA_BONE_WING_B);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_DOWNED):
        Em_PlaySeAtBoneOnFrame(em, 2, 23, 3);
        Em_PlaySeAtBoneOnFrame(em, 48, Sound_RandomSe(10, 5, 2, 3), 21);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 10, 21);
        Em_PlaySeAtBoneOnFrame(em, 108, Sound_RandomSe(11, 2, 11, 2), 21);
        Em_PlaySeAtBoneOnFrame(em, 222, 10, 21);
        break;
    case EM_MOTION_ID(0, 23):
        Em_PlaySeAtBoneOnFrame(em, 4, 0, 21);
        Em_PlaySeAtBoneOnFrame(em, 116, 14, 21);
        Em_PlaySeAtBoneOnFrame(em, 116, 15, 21);
        Em_PlaySeAtBoneOnFrame(em, 158, 26, 3);
        Em_PlaySeAtBoneOnFrame(em, 178, 27, 21);
        Em_PlaySeAtBoneOnFrame(em, 244, 13, 21);
        break;
    case EM_MOTION_ID(0, 24):
        Em_PlaySeAtBoneOnFrame(em, 6, 3, 21);
        Em_PlaySeAtBoneOnFrame(em, 30, 4, 21);
        Em_PlaySeAtBoneOnFrame(em, 22, 34, 29);
        Em_PlaySeAtBoneOnFrame(em, 44, 25, 25);
        Em_PlaySeAtBoneOnFrame(em, 64, 34, 29);
        Em_PlaySeAtBoneOnFrame(em, 76, 4, 21);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_DEATH):
        Em_PlaySeAtBoneOnFrame(em, 8, 32, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 10, 1, 21);
        Em_PlaySeAtBoneOnFrame(em, 48, 31, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 90, 2, 21);
        Em_PlaySeAtBoneOnFrame(em, 144, 27, 21);
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_HOVER_CRY):
        Em_PlaySeAtBoneOnFrame(em, 14, 12, 21);
        Em_PlaySeAtBoneOnFrame(em, 44, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 52, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 98, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 106, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 158, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 166, 29, REMOBRA_BONE_WING_B);
        Em_PlaySeAtBoneOnFrame(em, 224, 28, REMOBRA_BONE_WING_A);
        Em_PlaySeAtBoneOnFrame(em, 230, 29, REMOBRA_BONE_WING_B);

    default:
        break;
    }
}

/*
 * Sets REMOBRA_MOT_FLY, the air stance, hp and hpMax from Em_ComputeMaxHp,
 * scriptPending and the parts; keeps the position (Y 0) as
 * EmRemobraWork.home.
 */
void EmRemobra_Init(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;

    s16 size;

    Em_SetMotionRow(em, REMOBRA_MOT_FLY, 0, 0);
    em->stance = EM_STANCE_AIR;
    size = Em_ComputeMaxHp(em, 0);
    em->hp = size;
    em->hpMax = size;
    em->scriptPending = 1;
    em->blind = 0;
    Em_InitParts(em);
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = (f32) em->posZ;
}

/*
 * Inits the monster and enters REMOBRA_FLY_TAKE_OFF_START (it appears
 * rising into the air).
 */
void EmRemobra_Start(Em *em)
{
    EmRemobra_Init(em);
    EmRemobra_ChangeState(em, EM_GROUP_FLY, REMOBRA_FLY_TAKE_OFF_START, EM_SYNC_LOCAL);
}

/* Inits the monster again (EmRemobra_Init), without a state change. */
void EmRemobra_Reinit(Em *em)
{
    EmRemobra_Init(em);
}

/*
 * Prepares a state before Em_SetStateSync moves to it: for the flights it
 * sets turnYaw (REMOBRA_FLY_TURN_TO_GOAL, FLY_TURN_RANDOM), distLeft,
 * steerToGoal and the timer (FLY_SWOOP, FLY_SWOOP_SPIT, FLY_GLIDE_TO_GOAL,
 * FLY_GLIDE_FIXED), the height goal (FLY_CHANGE_HEIGHT_GOAL, FLY_RISE_HIGH)
 * or the home goal (FLY_RETURN_HOME), and ends status 4 for
 * FLY_TAKE_OFF_STATUS4; REMOBRA_DEFEAT_WAIT_APPEAR appears again at once
 * when it can (QuestEm_Appear), REMOBRA_REACT_STATUS4 begins status 4.
 */
void EmRemobra_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    f32 stopShort;
    f32 left;

    switch (group) {
    case EM_GROUP_FLY:
        switch (state) {
        case REMOBRA_FLY_TURN_TO_GOAL:
            w->turnYaw = Math_AngleXZ(&em->posX, em->goal);
            break;
        case REMOBRA_FLY_TURN_RANDOM:
            w->turnYaw = em->rotY + (em->stateRand & 0x3FFF);
            break;
        case REMOBRA_FLY_SWOOP:
        case REMOBRA_FLY_SWOOP_SPIT:
            stopShort = 1300.0f;
            /* the glide's distance set-up below, shared by a jump as the game's code does */
            goto dist;
        case REMOBRA_FLY_GLIDE_TO_GOAL:
            stopShort = 300.0f;
        dist:
            if (em->curGoalKind == 0) {
                w->steerToGoal = 0;
                em->timer = (em->stateRand % 120 + 180) / 2;
                w->distLeft = 1000.0f;
            } else {
                w->steerToGoal = 1;
                w->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
                if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                    if (em->targetNo == -1) {
                        w->steerToGoal = 0;
                        w->distLeft = 1000.0f;
                        break;
                    }
                    left = w->distLeft - stopShort;
                    w->distLeft = left;
                    if (left <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                if (em->curGoalKind == 7) {
                    left = w->distLeft - stopShort;
                    w->distLeft = left;
                    if (left <= 0.0f) {
                        w->distLeft = 0.0f;
                    }
                }
                em->timer = (s32) w->distLeft / 7;
            }
            break;
        case REMOBRA_FLY_GLIDE_FIXED:
            w->steerToGoal = 0;
            left = 600.0f;
            w->distLeft = left;
            em->timer = (s32) left / 7;
            break;
        case REMOBRA_FLY_CHANGE_HEIGHT_RANDOM:
            em->timer = (em->stateRand & 3) * 30 + 30;
            break;
        case REMOBRA_FLY_CHANGE_HEIGHT_GOAL:
            em->timer = 120;
            if (em->curGoalKind == 0) {
                em->goal[1] = em->posY;
                em->goal[1] += (f32) ((u16) Rand_Next(1) % 200) - 50.0f;
            }
            break;
        case REMOBRA_FLY_TAKE_OFF_STATUS4:
            Em_EndStatus4(em);
            break;
        case REMOBRA_FLY_RETURN_HOME:
            Vec_Copy(em->goal, &w->home[0]);
            em->goal[1] += 20.0f;
            w->distLeft = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
            break;
        case REMOBRA_FLY_RISE_HIGH:
            em->goal[1] = 500.0f + em->floorY;
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == 1 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case REMOBRA_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        }
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends a state on the ground: motion speed 1, ground stance, scriptPending,
 * then REMOBRA_IDLE_GROUND.
 */
void EmRemobra_EndGroundState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->blind = 0;
    EmRemobra_ChangeState(em, EM_GROUP_IDLE, REMOBRA_IDLE_GROUND, EM_SYNC_LOCAL);
}

/*
 * Ends a state in the air: motion speed 1, air stance, scriptPending, then
 * REMOBRA_FLY_HOVER.
 */
void EmRemobra_EndFlyState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_AIR;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    em->blind = 0;
    EmRemobra_ChangeState(em, EM_GROUP_FLY, REMOBRA_FLY_HOVER, EM_SYNC_LOCAL);
}

/* REMOBRA_IDLE_NONE: does nothing. */
void EmRemobra_IdleNone(void)
{
}

/*
 * REMOBRA_IDLE_GROUND: stands on the ground (REMOBRA_MOT_IDLE_GROUND unless
 * already playing) for 30 to 120 frames, then enters itself again
 * (EmRemobra_EndGroundState).
 */
void EmRemobra_IdleGround(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        em->timer = ((em->stateRand & 3) * 30) + 30;
        if (em->motionId[0] != EM_MOTION_ID(0, REMOBRA_MOT_IDLE_GROUND)) {
            Em_SetMotion(em, REMOBRA_MOT_IDLE_GROUND, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            EmRemobra_EndGroundState(em);
        }
        break;
    }
}

/*
 * The EM_GROUP_IDLE dispatcher: keeps the previous group and state, runs
 * REMOBRA_IDLE_NONE and REMOBRA_IDLE_GROUND.
 */
void EmRemobra_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case REMOBRA_IDLE_NONE:
        EmRemobra_IdleNone();
        break;
    case REMOBRA_IDLE_GROUND:
        EmRemobra_IdleGround(em);
        break;
    }
}

/*
 * REMOBRA_MOVE_NONE: ends at once on the ground (EmRemobra_EndGroundState);
 * the family has no ground moves.
 */
void EmRemobra_MoveNone(Em *em)
{
    EmRemobra_EndGroundState(em);
}

/* The EM_GROUP_MOVE dispatcher: runs REMOBRA_MOVE_NONE. */
void EmRemobra_RunMoveState(Em *em)
{
    switch (em->state) {
    case REMOBRA_MOVE_NONE:
        EmRemobra_MoveNone(em);
        break;
    }
}

/*
 * Clears the hop (EmRemobraWork's hopVel, hopAccel and hopFrames) when a
 * flight state starts REMOBRA_MOT_FLY.
 */
void EmRemobra_StopHop(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;

    w->hopFrames = 0;
    w->hopVel = 0.0f;
    w->hopAccel = 0.0f;
}

/*
 * While REMOBRA_MOT_FLY plays, starts a hop at each of the three
 * EmRemobra_HopFrames (velocity 14 slowing over half of 34 frames) and
 * moves posY by the hop each frame; run after every EM_GROUP_FLY state.
 */
void EmRemobra_UpdateHop(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    s16 i;
    f32 *hopFrame;

    if (em->motionId[0] == EM_MOTION_ID(0, REMOBRA_MOT_FLY)) {
        for (i = 0, hopFrame = EmRemobra_HopFrames; i < 3; i++, hopFrame++) {
            if (EmMotion_PassedFrameScaled(em, *hopFrame, 0) != 0) {
                w->hopFrames = 34;
                w->hopVel = 14.0f;
                w->hopAccel = -(w->hopVel / (w->hopFrames / 2));
                w->hopFrames += 1;
            }
        }
        if (w->hopFrames > 0) {
            w->hopFrames -= 1;
            em->posY += w->hopVel;
            w->hopVel += w->hopAccel;
        }
    }
}

/*
 * REMOBRA_FLY_HOVER: flies in place (REMOBRA_MOT_FLY) for 30 frames, rising
 * 18 a frame while within 400 of the floor, then enters itself again
 * (EmRemobra_EndFlyState).
 */
void EmRemobra_Hover(Em *em)
{
    f32 posY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stateVar1 = 0;
        em->stance = EM_STANCE_AIR;
        em->timer = 30;
        if (em->motionId[0] != EM_MOTION_ID(0, REMOBRA_MOT_FLY)) {
            Em_SetMotion(em, REMOBRA_MOT_FLY, 0, 0);
            EmRemobra_StopHop(em);
        }

    case 1:
        if (--em->timer <= 0) {
            EmRemobra_EndFlyState(em);
            break;
        }
        posY = em->posY;
        if (((posY - em->floorY) <= 400.0f) && (em->stateVar0 == 0)) {
            em->posY += 18.0f;
            break;
        }
        em->stateVar0 = 1;
        break;
    }
}

/*
 * REMOBRA_FLY_SWOOP_END: plays REMOBRA_MOT_SWOOP_END to its end, then
 * REMOBRA_FLY_HOVER.
 */
void EmRemobra_SwoopEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, REMOBRA_MOT_SWOOP_END, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndFlyState(em);
        }
        break;
    }
}

/*
 * REMOBRA_FLY_SWOOP, REMOBRA_FLY_SWOOP_SPIT: swoops forward at speed 45
 * (REMOBRA_MOT_SWOOP, or with variant 1 REMOBRA_MOT_SPIT_DOWN replayed,
 * spitting at frame 24 each time), steering toward the goal while
 * steerToGoal is set, until near the target or the timer or distLeft runs
 * out; then slows with REMOBRA_MOT_SWOOP and enters REMOBRA_FLY_SWOOP_END.
 */
void EmRemobra_Swoop(Em *em, s32 variant)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    s32 yawDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        if (variant == 0) {
            Em_SetMotion(em, REMOBRA_MOT_SWOOP, 0, 0);
        } else {
            Em_SetMotion(em, REMOBRA_MOT_SPIT_DOWN, 0, 0);
        }
        Player_ClearVelocity(em);
        em->velZ = 45.0f;
    case 1:
        if (w->steerToGoal != 0) {
            yawDiff = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if (yawDiff <= ANGLE_TURN / 2) {
                if (yawDiff < 0x100) {
                    em->rotY += yawDiff;
                } else {
                    em->rotY += 0x100;
                }
            } else if (yawDiff > 0xFF00) {
                em->rotY += yawDiff;
            } else {
                em->rotY -= 0x100;
            }
        }
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 300.0f || --em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, REMOBRA_MOT_SWOOP, 0, 0);
                em->velZ *= 0.8f;
                break;
            }
        } else {
            w->distLeft -= em->velZ;
            if (w->distLeft <= 0.0f) {
                em->step++;
                Em_SetMotion(em, REMOBRA_MOT_SWOOP, 0, 0);
                em->velZ *= 0.8f;
                break;
            }
        }
        if (em->motionId[0] == EM_MOTION_ID(0, REMOBRA_MOT_SPIT_DOWN)) {
            if (em->motion[0].playing == 0) {
                Em_SetMotion(em, REMOBRA_MOT_SPIT_DOWN, 8, 0);
            } else if (EmMotion_PassedFrameScaled(em, 24.0f, 0)) {
                EmRangedAttack_CreateAtBone(em, REMOBRA_BONE_SPIT, EMRANGED_REMOBRA_SPIT, 0, 15474, 0);
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmRemobra_ChangeState(em, EM_GROUP_FLY, REMOBRA_FLY_SWOOP_END, EM_SYNC_LOCAL);
        } else {
            Em_MoveRotatedAccel(em, &em->rotX);
        }
        break;
    }
}

/*
 * REMOBRA_FLY_BANK_SLOW_A, _SLOW_B, _FAST_A, _FAST_B: flies a turning curve
 * for 30 frames (speed 24 turning 512 a frame, or 33.75 turning 1024;
 * dir picks the side and the motion), then enters REMOBRA_FLY_SWOOP.
 */
void EmRemobra_Bank(Em *em, s32 fast, s32 side)
{
    u16 turnStep;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Player_ClearVelocity(em);
        if (fast == 0) {
            if (side == 0) {
                Em_SetMotion(em, REMOBRA_MOT_BANK_SLOW_A, 0, 0);
            } else {
                Em_SetMotion(em, REMOBRA_MOT_BANK_SLOW_B, 0, 0);
            }
            em->velZ = 24.0f;
        } else {
            if (side == 0) {
                Em_SetMotion(em, REMOBRA_MOT_BANK_FAST_A, 0, 0);
            } else {
                Em_SetMotion(em, REMOBRA_MOT_BANK_FAST_B, 0, 0);
            }
            em->velZ = 33.75f;
        }
        em->timer = 30;
        Em_MoveRotatedAccel(em, &em->rotX);
        break;
    case 1:
        if (fast == 0) {
            if (side == 0) {
                turnStep = 512;
            } else {
                turnStep = -512;
            }
        } else {
            if (side == 0) {
                turnStep = 1024;
            } else {
                turnStep = -1024;
            }
        }
        em->rotY += turnStep;
        Em_MoveRotatedAccel(em, &em->rotX);
        if (--em->timer <= 0) {
            EmRemobra_ChangeState(em, EM_GROUP_FLY, REMOBRA_FLY_SWOOP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * REMOBRA_FLY_BACK_OFF_A, REMOBRA_FLY_BACK_OFF_B: plays
 * REMOBRA_MOT_BACK_OFF and from frame 40 leaps up and backward (variant 1
 * higher and farther), then REMOBRA_FLY_HOVER when the motion ends.
 */
void EmRemobra_BackOff(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Player_ClearVelocity(em);
        Em_SetMotion(em, REMOBRA_MOT_BACK_OFF, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            em->step++;
            if (variant == 0) {
                em->velY = 30.0f;
                em->accelY = -0.8f;
                em->velZ = -30.0f;
                break;
            }
            em->velY = 32.0f;
            em->accelY = -0.8f;
            em->velZ = -40.0f;
        }
        break;
    case 2:
        Em_MoveRotatedAccel(em, &em->rotX);
        if (em->motion[0].playing == 0) {
            EmRemobra_EndFlyState(em);
        }
        break;
    }
}

/*
 * REMOBRA_FLY_TAKE_OFF_START, REMOBRA_FLY_TAKE_OFF,
 * REMOBRA_FLY_TAKE_OFF_STATUS4: rises from the ground
 * (REMOBRA_MOT_TAKE_OFF; variant 0 starts it at frame 90); at frame 196 it
 * is moved 400 up, takes the air stance and REMOBRA_MOT_FLY (variant 2 clears
 * noReact) and enters REMOBRA_FLY_HOVER.
 */
void EmRemobra_TakeOff(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (variant == 0) {
            Em_SetMotion(em, REMOBRA_MOT_TAKE_OFF, 0, 90);
            break;
        }
        Em_SetMotion(em, REMOBRA_MOT_TAKE_OFF, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 196.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            em->posY += 400.0f;
            Em_SetMotionRow(em, REMOBRA_MOT_FLY, 0, 0);
            if (variant == 2) {
                em->noReact = 0;
            }
            EmRemobra_EndFlyState(em);
        }
        break;
    }
}

/*
 * REMOBRA_FLY_TURN_TO_GOAL, REMOBRA_FLY_TURN_RANDOM: turns rotY 0x440 a
 * frame toward turnYaw (set by EmRemobra_ChangeState) while flying
 * (REMOBRA_MOT_FLY); once there clears EM_FLAG_NO_ROOT_MOTION and enters
 * REMOBRA_FLY_HOVER.
 */
void EmRemobra_TurnInAir(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    u16 goalYaw;
    u32 diff;
    s32 yaw;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        if (em->motionId[0] != EM_MOTION_ID(0, REMOBRA_MOT_FLY)) {
            Em_SetMotion(em, REMOBRA_MOT_FLY, 0, 0);
            EmRemobra_StopHop(em);
        }
        break;
    case 1:
        yaw = em->rotY;
        goalYaw = w->turnYaw;
        diff = (goalYaw - (yaw & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + 1088) & 0xFFFF) < 2176U) {
            em->rotY = (s32) goalYaw;
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmRemobra_EndFlyState(em);
        } else if (diff < ANGLE_TURN / 2) {
            em->rotY = (yaw + 0x440) & 0xFFFF;
        } else {
            em->rotY = (yaw - 0x440) & 0xFFFF;
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * REMOBRA_FLY_CHANGE_HEIGHT_RANDOM, REMOBRA_FLY_CHANGE_HEIGHT_GOAL,
 * REMOBRA_FLY_RISE_HIGH: climbs or sinks 18 a frame toward a height
 * (variant 0 a random one from stateRand, variant 1 goal[1]) until within
 * 50, the timer runs out or it would go below 180 over the floor, then
 * REMOBRA_FLY_HOVER.
 */
void EmRemobra_ChangeHeight(Em *em, s32 variant)
{
    f32 posY;
    f32 curY;
    f32 targetY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        if (em->motionId[0] != EM_MOTION_ID(0, REMOBRA_MOT_FLY)) {
            Em_SetMotion(em, REMOBRA_MOT_FLY, 0, 0);
            EmRemobra_StopHop(em);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            EmRemobra_EndFlyState(em);
            break;
        }
        posY = 180.0f + em->floorY;
        if (em->posY < posY) {
            em->posY = posY;
            EmRemobra_EndFlyState(em);
        }
        if (variant == 0) {
            targetY = 600.0f - (f32) ((s32) em->stateRand % 400);
        } else {
            targetY = em->goal[1];
        }
        if (!(Math_Absf(em->posY - targetY) < 50.0f)) {
            curY = em->posY;
            if (curY <= targetY) {
                em->posY += 18.0f;
                break;
            }
            em->posY -= 18.0f;
            break;
        }
        EmRemobra_EndFlyState(em);
        break;
    }
}

/*
 * REMOBRA_FLY_GLIDE_TO_GOAL, REMOBRA_FLY_GLIDE_FIXED: flies forward 20 a
 * frame (REMOBRA_MOT_FLY), steering toward the goal at 0x280 a frame while
 * steerToGoal is set, until near the target or the timer or distLeft runs
 * out, then REMOBRA_FLY_HOVER.
 */
void EmRemobra_Glide(Em *em, s32 variant)
{
    f32 stepLocal[3];
    f32 stepWorld[3];
    s32 yawAngles[3];
    f32 mtx[16];
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    s32 yawDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        if (em->motionId[0] != EM_MOTION_ID(0, REMOBRA_MOT_FLY)) {
            Em_SetMotion(em, REMOBRA_MOT_FLY, 0, 0);
            EmRemobra_StopHop(em);
        }
        break;
    case 1:
        if (w->steerToGoal != 0) {
            yawDiff = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if (yawDiff <= ANGLE_TURN / 2) {
                if (yawDiff < 0x280) {
                    em->rotY += yawDiff;
                } else {
                    em->rotY += 0x280;
                }
            } else if (yawDiff > 0xFD80) {
                em->rotY += yawDiff;
            } else {
                em->rotY -= 0x280;
            }
        }
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        stepLocal[0] = 0.0f;
        stepLocal[1] = 0.0f;
        stepLocal[2] = 20.0f;
        yawAngles[0] = 0;
        yawAngles[1] = em->rotY;
        yawAngles[2] = 0;
        Mtx_FromAnglesYXZ(yawAngles, mtx);
        Vec_Rotate(stepWorld, stepLocal, mtx);
        em->posX += stepWorld[0];
        em->posZ += stepWorld[2];
        if (em->curGoalKind == 1 && em->curGoalArg == 0) {
            if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 300.0f || --em->timer <= 0) {
                em->step++;
                EmRemobra_EndFlyState(em);
            }
        } else {
            w->distLeft -= stepLocal[2];
            if (w->distLeft <= 0.0f) {
                em->step++;
                EmRemobra_EndFlyState(em);
            }
        }
        break;
    case 2:
        EmRemobra_EndFlyState(em);
        break;
    }
}

/*
 * REMOBRA_FLY_RISE_CRY: rises 24 a frame to 400 over the floor
 * (REMOBRA_MOT_FLY) if lower, then plays REMOBRA_MOT_HOVER_CRY to its end
 * and enters REMOBRA_FLY_HOVER.
 */
void EmRemobra_RiseAndCry(Em *em, s32 unused)
{
    f32 posY;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        if (em->posY <= (400.0f + em->floorY)) {
            em->step++;
            if (em->motionId[0] != EM_MOTION_ID(0, REMOBRA_MOT_FLY)) {
                Em_SetMotion(em, REMOBRA_MOT_FLY, 0, 0);
            }
            break;
        }
        em->step = 2;
        Em_SetMotion(em, REMOBRA_MOT_HOVER_CRY, 0, 0);
        break;
    case 1:
        posY = em->posY;
        if ((posY - em->floorY) <= 400.0f) {
            em->posY += 24.0f;
            break;
        }
        em->step++;
        Em_SetMotion(em, REMOBRA_MOT_HOVER_CRY, 0, 0);
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndFlyState(em);
        }
        break;
    }
}

/*
 * REMOBRA_FLY_RETURN_HOME: flies toward the goal set just above home
 * (Em_TurnToGoal, speed 20, kept above the floor) for 30 to 75 frames, then
 * REMOBRA_FLY_HOVER.
 */
void EmRemobra_ReturnHome(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        em->timer = ((em->stateRand & 3) * 15) + 30;
        Em_SetMotion(em, REMOBRA_MOT_FLY, 0, 0);
        Player_ClearVelocity(em);
        em->velX = 0.0f;
        em->velY = 0.0f;
        em->velZ = 20.0f;
        break;
    case 1:
        Em_TurnToGoal(em, 1456, 0);
        EmRemobra_MoveByVelocity(em);
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmRemobra_EndFlyState(em);
        }
        break;
    }
}

/*
 * REMOBRA_FLY_SHIFT_FORWARD, _BACK, _SIDE_A, _SIDE_B, _BACK_UP: drifts for
 * 40 frames in the direction of the variant relative to its facing
 * (REMOBRA_MOT_FLY), then REMOBRA_FLY_HOVER.
 */
void EmRemobra_Shift(Em *em, s32 variant)
{
    f32 stepWorld[3];
    s32 yawAngles[3];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 40;
        em->stance = EM_STANCE_AIR;
        if (em->motionId[0] != EM_MOTION_ID(0, REMOBRA_MOT_FLY)) {
            Em_SetMotion(em, REMOBRA_MOT_FLY, 0, 0);
            EmRemobra_StopHop(em);
        }
        Player_ClearVelocity(em);
        switch (variant) {
        case 0:
            em->velZ = 20.0f;
            break;
        case 1:
            em->velZ = -23.0f;
            break;
        case 2:
            em->velX = 20.0f;
            break;
        case 3:
            em->velX = -20.0f;
            break;
        case 4:
            em->velY = 20.0f;
            em->velZ = -23.0f;
            break;
        }
        break;
    case 1:
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        yawAngles[0] = 0;
        yawAngles[1] = em->rotY;
        yawAngles[2] = 0;
        Mtx_FromAnglesYXZ(yawAngles, mtx);
        Vec_Rotate(stepWorld, &em->velX, mtx);
        em->posX += stepWorld[0];
        em->posY += stepWorld[1];
        em->posZ += stepWorld[2];
        if (--em->timer <= 0) {
            em->step++;
            EmRemobra_EndFlyState(em);
        }
        break;
    case 2:
        EmRemobra_EndFlyState(em);
        break;
    }
}

/*
 * The EM_GROUP_FLY dispatcher: runs the REMOBRA_FLY_* states with their
 * variants, then the hop (EmRemobra_UpdateHop), and keeps posY on or above
 * the floor.
 */
void EmRemobra_RunFlyState(Em *em)
{
    f32 floorY;

    switch (em->state) {
    case 0:
        EmRemobra_Hover(em);
        break;
    case 1:
        EmRemobra_SwoopEnd(em);
        break;
    case 2:
        EmRemobra_Swoop(em, 0);
        break;
    case 3:
        EmRemobra_Bank(em, 0, 0);
        break;
    case 4:
        EmRemobra_Bank(em, 0, 1);
        break;
    case 5:
        EmRemobra_Bank(em, 1, 0);
        break;
    case 6:
        EmRemobra_Bank(em, 1, 1);
        break;
    case 7:
        EmRemobra_BackOff(em, 0);
        break;
    case 8:
        EmRemobra_BackOff(em, 1);
        break;
    case 9:
        EmRemobra_TakeOff(em, 0);
        break;
    case 10:
        EmRemobra_TurnInAir(em);
        break;
    case 11:
        EmRemobra_TakeOff(em, 1);
        break;
    case 12:
        EmRemobra_ChangeHeight(em, 0);
        break;
    case 13:
        EmRemobra_ChangeHeight(em, 1);
        break;
    case 14:
        EmRemobra_TakeOff(em, 2);
        break;
    case 15:
        EmRemobra_Glide(em, 0);
        break;
    case 16:
        EmRemobra_RiseAndCry(em, 0);
        break;
    case 17:
        EmRemobra_ReturnHome(em);
        break;
    case 18:
        EmRemobra_TurnInAir(em);
        break;
    case 19:
        EmRemobra_Glide(em, 0);
        break;
    case 20:
        EmRemobra_Swoop(em, 1);
        break;
    case 21:
        EmRemobra_Shift(em, 0);
        break;
    case 22:
        EmRemobra_Shift(em, 1);
        break;
    case 23:
        EmRemobra_Shift(em, 2);
        break;
    case 24:
        EmRemobra_Shift(em, 3);
        break;
    case 25:
        EmRemobra_Shift(em, 4);
        break;
    case 26:
        EmRemobra_ChangeHeight(em, 1);
        break;
    }
    EmRemobra_UpdateHop(em);
    floorY = em->floorY;
    if (em->posY <= floorY) {
        em->posY = floorY;
    }
}

/*
 * REMOBRA_ATK_STRIKE: plays REMOBRA_MOT_STRIKE with a RemobraAttack of type
 * 1 at frame 46, then REMOBRA_FLY_HOVER.
 */
void EmRemobra_Strike(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, REMOBRA_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndFlyState(em);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 46.0f, 0U) != 0) {
            RemobraAttack_Create(em, 1);
        }
        break;
    }
}

/*
 * REMOBRA_ATK_SPIT_A, _B, _C: holds REMOBRA_MOT_SPIT_AIM for 40, 50 or 60
 * frames, then plays REMOBRA_MOT_SPIT and spits (EMRANGED_REMOBRA_SPIT from
 * REMOBRA_BONE_SPIT) at frame 8; REMOBRA_FLY_HOVER at its end.
 */
void EmRemobra_SpitAimed(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        em->stance = EM_STANCE_AIR;
        switch (variant) {
        case 0:
            em->timer = 40;
            break;
        case 1:
            em->timer = 50;
            break;
        case 2:
            em->timer = 60;
            break;
        }
        Em_SetMotion(em, REMOBRA_MOT_SPIT_AIM, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, REMOBRA_MOT_SPIT, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndFlyState(em);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 8.0f, 0U) != 0) {
            switch (variant) {
            case 0:
                EmRangedAttack_CreateAtBone(em, REMOBRA_BONE_SPIT, EMRANGED_REMOBRA_SPIT, 0, 8192, 0);
                break;
            case 1:
                EmRangedAttack_CreateAtBone(em, REMOBRA_BONE_SPIT, EMRANGED_REMOBRA_SPIT, 0, 8192, 0);
                break;
            case 2:
                EmRangedAttack_CreateAtBone(em, REMOBRA_BONE_SPIT, EMRANGED_REMOBRA_SPIT, 0, 8192, 0);
                break;
            }
        }
        break;
    }
}

/*
 * REMOBRA_ATK_SPIT_DOWN: plays REMOBRA_MOT_SPIT_DOWN and spits at a steeper
 * pitch at frame 24, then REMOBRA_FLY_HOVER.
 */
void EmRemobra_SpitDown(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, REMOBRA_MOT_SPIT_DOWN, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndFlyState(em);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBone(em, REMOBRA_BONE_SPIT, EMRANGED_REMOBRA_SPIT, 0, 15474, 0);
        }
        break;
    }
}

/*
 * REMOBRA_ATK_SWOOP_STRIKE: plays REMOBRA_MOT_SWOOP_STRIKE moving along
 * curve kind 0 (EmRemobra_StartCurves, UpdateCurves), with RemobraAttacks
 * of types 2 and 3 at frames 104 and 168, then REMOBRA_FLY_HOVER.
 */
void EmRemobra_SwoopStrike(Em *em, s32 unused)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, REMOBRA_MOT_SWOOP_STRIKE, 0, 0);
        EmRemobra_StartCurves(em, 0U);
        break;
    case 1:
        EmRemobra_UpdateCurves(em);
        floorY = em->floorY;
        if (em->posY <= floorY) {
            em->posY = floorY;
        }
        if (em->motion[0].playing == 0) {
            EmRemobra_EndFlyState(em);
            break;
        }
        if (EmMotion_PassedFrameScaled(em, 104.0f, 0U) != 0) {
            RemobraAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 168.0f, 0U) != 0) {
            RemobraAttack_Create(em, 3);
        }
        break;
    }
}

/* The EM_GROUP_ATTACK dispatcher: runs the REMOBRA_ATK_* states. */
void EmRemobra_RunAttackState(Em *em)
{
    switch (em->state) {
    case REMOBRA_ATK_STRIKE:
        EmRemobra_Strike(em, 0);
        break;
    case REMOBRA_ATK_SPIT_A:
        EmRemobra_SpitAimed(em, 0);
        break;
    case REMOBRA_ATK_SPIT_B:
        EmRemobra_SpitAimed(em, 1);
        break;
    case REMOBRA_ATK_SPIT_C:
        EmRemobra_SpitAimed(em, 2);
        break;
    case REMOBRA_ATK_SPIT_DOWN:
        EmRemobra_SpitDown(em, 0);
        break;
    case REMOBRA_ATK_SWOOP_STRIKE:
        EmRemobra_SwoopStrike(em, 0);

    default:
        break;
    }
}

/*
 * REMOBRA_REACT_FLINCH: restarts the script, plays REMOBRA_MOT_FLINCH in
 * the air, then REMOBRA_FLY_HOVER.
 */
void EmRemobra_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_RestartScript(em);
        Em_SetMotion(em, REMOBRA_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmRemobra_EndFlyState(em);
        }
        break;
    }
}

/*
 * REMOBRA_REACT_FALL: falls (REMOBRA_MOT_FALL, faster from frame 66) to the
 * floor, plays REMOBRA_MOT_FALL_LAND on the ground and enters
 * REMOBRA_REACT_DOWNED.
 */
void EmRemobra_FallFromAir(Em *em, s32 unused)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, REMOBRA_MOT_FALL, 0, 0);
        Em_RestartScript(em);
        em->velY = 10.0f;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
            em->velY = 40.0f;
        }
        em->posY -= em->velY;
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, REMOBRA_MOT_FALL_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRemobra_ChangeState(em, EM_GROUP_REACT, REMOBRA_REACT_DOWNED, EM_SYNC_OWNER);
        }
        break;
    case 3:
        EmRemobra_ChangeState(em, EM_GROUP_REACT, REMOBRA_REACT_DOWNED, EM_SYNC_OWNER);
        break;
    }
}

/*
 * REMOBRA_REACT_DOWNED: lies on the ground (REMOBRA_MOT_DOWNED) for 150
 * frames, then gets up with REMOBRA_MOT_TAKE_OFF to frame 90 and enters
 * REMOBRA_IDLE_GROUND; hits keep the status (EM_HIT_REACT_STATUS).
 */
void EmRemobra_Downed(Em *em, s32 unused)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_RestartScript(em);
        em->timer = 150;
        Em_SetMotion(em, REMOBRA_MOT_DOWNED, 0, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, REMOBRA_MOT_TAKE_OFF, 0, 0);
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 90.0f) != 0) {
            em->step++;
            EmRemobra_EndGroundState(em);
        }
        break;
    }
}

/*
 * REMOBRA_REACT_STATUS4: sets noReact, plays REMOBRA_MOT_TAKE_OFF to frame
 * 90, then REMOBRA_MOT_STATUS4 with the status-4 effect until the timer
 * (Em_SetStatus4Timer) runs out, then REMOBRA_FLY_TAKE_OFF_STATUS4.
 */
void EmRemobra_Status4(Em *em, s32 unused)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, REMOBRA_MOT_TAKE_OFF, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 90.0f) != 0) {
            em->step++;
            Em_SetMotion(em, REMOBRA_MOT_STATUS4, 0, 0);
        }
        break;
    case 2:
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmRemobra_ChangeState(em, EM_GROUP_FLY, REMOBRA_FLY_TAKE_OFF_STATUS4, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmRemobra_ChangeState(em, EM_GROUP_FLY, REMOBRA_FLY_TAKE_OFF_STATUS4, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* The EM_GROUP_REACT dispatcher: runs the REMOBRA_REACT_* states. */
void EmRemobra_RunReactState(Em *em)
{
    switch (em->state) {
    case REMOBRA_REACT_FLINCH:
        EmRemobra_Flinch(em);
        break;
    case REMOBRA_REACT_FALL:
        EmRemobra_FallFromAir(em, 0);
        break;
    case REMOBRA_REACT_DOWNED:
        EmRemobra_Downed(em, 0);
        break;
    case REMOBRA_REACT_STATUS4:
        EmRemobra_Status4(em, 0);
        break;
    }
}

/*
 * REMOBRA_DEFEAT_HUNTED: dies (REMOBRA_MOT_DEATH, QuestEm_Hunted, dead
 * stance), makes the carve point (EmMine_Create) at frame 158 and keeps it
 * 300 frames, then fades out over 150 frames and enters
 * REMOBRA_DEFEAT_WAIT_APPEAR.
 */
void EmRemobra_Hunted(Em *em)
{
    em->noPushTimer = 5;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        Em_SetMotion(em, REMOBRA_MOT_DEATH, 0, 0);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_DEAD;
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 158.0f, 0) != 0 && em->stateVar0 == 0) {
            em->stateVar0 = 1;
            EmMine_Create(em, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 300;
            em->fieldCollide = 0;
        }
        break;
    case 2:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 150;
            EmMine_Remove(em);
        }
        break;
    case 3:
        if (em->timer > 0) {
            em->timer--;
        }
        if (em->timer <= 0) {
            if (em->remote == 0) {
                EmRemobra_ChangeState(em, EM_GROUP_DEFEAT, REMOBRA_DEFEAT_WAIT_APPEAR, EM_SYNC_OWNER_FLAGGED);
            }
        } else {
            em->alpha = em->timer / 150.0f;
        }
        break;
    }
}

/*
 * REMOBRA_DEFEAT_WAIT_APPEAR: when QuestEm_CanAppear allows, sets motion
 * REMOBRA_MOT_FLY and enters REMOBRA_EVENT2_REAPPEAR; otherwise advances taskPhase
 * (the task finishes).
 */
void EmRemobra_WaitAppear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            Em_SetMotionRow(em, REMOBRA_MOT_FLY, 0, 0);
            EmRemobra_ChangeState(em, EM_GROUP_EVENT2, REMOBRA_EVENT2_REAPPEAR, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * REMOBRA_DEFEAT_FALL: killed in the air, falls as in REMOBRA_REACT_FALL
 * (REMOBRA_MOT_FALL, REMOBRA_MOT_FALL_LAND), then REMOBRA_DEFEAT_HUNTED.
 */
void EmRemobra_FallDead(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        Em_SetMotion(em, REMOBRA_MOT_FALL, 0, 0);
        em->velY = 10.0f;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 66.0f, 0U) != 0) {
            em->velY = 40.0f;
        }
        em->posY -= em->velY;
        floorY = em->floorY;
        if (em->posY <= floorY) {
            em->posY = floorY;
            em->step++;
            Em_SetMotion(em, REMOBRA_MOT_FALL_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmRemobra_ChangeState(em, EM_GROUP_DEFEAT, REMOBRA_DEFEAT_HUNTED, EM_SYNC_OWNER);
        }
        break;
    }
}

/*
 * The EM_GROUP_DEFEAT dispatcher: keeps the monster unhittable and
 * unpushable, runs the REMOBRA_DEFEAT_* states.
 */
void EmRemobra_RunDefeatState(Em *em)
{
    em->noHitTimer = 10;
    em->noPushTimer = 10;
    switch (em->state) {
    case REMOBRA_DEFEAT_HUNTED:
        EmRemobra_Hunted(em);
        break;
    case REMOBRA_DEFEAT_WAIT_APPEAR:
        EmRemobra_WaitAppear(em);
        break;
    case REMOBRA_DEFEAT_FALL:
        EmRemobra_FallDead(em);
        break;
    }
}

/*
 * REMOBRA_EVENT_WAIT: keeps senses off and the monster unhittable during a
 * scripted scene.
 */
void EmRemobra_EventWait(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
}

/* The EM_GROUP_EVENT dispatcher: runs REMOBRA_EVENT_WAIT. */
void EmRemobra_RunEventState(Em *em)
{
    switch (em->state) {
    case REMOBRA_EVENT_WAIT:
        EmRemobra_EventWait(em);
        break;
    }
}

/*
 * REMOBRA_EVENT2_REAPPEAR: plays REMOBRA_MOT_FLY once after appearing
 * again, then REMOBRA_FLY_HOVER.
 */
void EmRemobra_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, REMOBRA_MOT_FLY, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmRemobra_EndFlyState(em);
        }
        break;
    }
}

/*
 * The EM_GROUP_EVENT2 dispatcher (its own, not the event dispatcher as in other families): keeps the monster
 * unhittable and senses off, runs REMOBRA_EVENT2_REAPPEAR.
 */
void EmRemobra_RunEvent2State(Em *em)
{
    em->noHitTimer = 10;
    em->noPushTimer = 10;
    em->senseDelay = 5;
    switch (em->state) {
    case 0:
        EmRemobra_Reappear(em);
        break;
    }
}

/*
 * Each frame: maps the result of Em_UpdateDamage to a defeat or reaction
 * state (see the NOTE lines), runs the pending script, clears the hit-react
 * mode and runs the state (twice when stateRequest is set).
 */
void EmRemobra_Update(Em *em)
{
    u8 dmgOut[3];

    em->motionSpeed = 1.0f;
    Em_UpdateCombatTimer(em);
    switch (Em_UpdateDamage(em, dmgOut) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_CAPTURED:
    case EM_DMG_STATUS2:
    case EM_DMG_FLASH:
    case EM_DMG_HIT_LYING:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmRemobra_ChangeState(em, EM_GROUP_DEFEAT, REMOBRA_DEFEAT_FALL, EM_SYNC_OWNER);
        } else {
            EmRemobra_ChangeState(em, EM_GROUP_DEFEAT, REMOBRA_DEFEAT_HUNTED, EM_SYNC_OWNER);
        }
        em->scriptPending = 0;
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != REMOBRA_REACT_FALL) {
            EmRemobra_ChangeState(em, EM_GROUP_REACT, REMOBRA_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if (em->stateGroup != EM_GROUP_REACT || em->state != REMOBRA_REACT_FALL) {
            Em_SetStatus4Timer(em);
            EmRemobra_ChangeState(em, EM_GROUP_REACT, REMOBRA_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_HIT_MODE3:
        if (em->stance == EM_STANCE_AIR) {
            EmRemobra_ChangeState(em, EM_GROUP_REACT, REMOBRA_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT:
        if (em->stance == EM_STANCE_AIR) {
            EmRemobra_ChangeState(em, EM_GROUP_REACT, REMOBRA_REACT_FLINCH, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        if (em->stance == EM_STANCE_AIR) {
            EmRemobra_ChangeState(em, EM_GROUP_REACT, REMOBRA_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmRemobra_RunState(em);
    if (em->stateRequest != 0) {
        EmRemobra_RunState(em);
        em->stateRequest = 0;
    }
}

/* Runs the dispatcher of em->stateGroup (EM_GROUP_IDLE to EM_GROUP_EVENT2). */
void EmRemobra_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmRemobra_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmRemobra_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmRemobra_RunFlyState(em);
        break;
    case EM_GROUP_ATTACK:
        EmRemobra_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmRemobra_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmRemobra_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmRemobra_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmRemobra_RunEvent2State(em);

    default:
        break;
    }
}

/*
 * When a player is in the area, spawns ground effect type a2 on the floor
 * under bone a1 (PartFx_SpawnOnGround), scaled by scaleX.
 */
void EmRemobra_SpawnGroundEffect(Em *em, s32 bone, s32 effectType, f32 scale)
{
    f32 pos[3];

    if (Player_InCurrentArea(em)) {
        Em_BonePos(em, bone, pos);
        pos[1] = em->floorY;
        PartFx_SpawnOnGround(pos, effectType, scale * em->scaleX);
    }
}

/* Spawns that ground effect only while within 300 of the floor. */
void EmRemobra_SpawnGroundEffectNearGround(Em *em, s32 bone, s32 effectType, f32 scale)
{
    if (em->posY <= (300.0f + em->floorY)) {
        EmRemobra_SpawnGroundEffect(em, bone, effectType, scale);
    }
}

/*
 * Keeps work->fxMotion as the playing motion, plays its sounds
 * (EmRemobra_MotionSounds) and spawns the wing-dust ground effects on the
 * frames of the motions.
 */
void EmRemobra_MotionEffects(Em *em, EmWork *work)
{
    s16 motion;
    u16 motionId;

    motionId = em->motionId[0];
    if (motionId != work->fxMotion) {
        work->fxMotion = (s16) motionId;
    }
    EmRemobra_MotionSounds(em, work);
    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, REMOBRA_MOT_FLY):
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 84.0f, 0U) != 0 || EmMotion_PassedFrameScaled(em, 156.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffectNearGround(em, 1, 7, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, 2):
        if (EmMotion_PassedFrameScaled(em, 84.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffect(em, 3, 7, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_SWOOP_END):
        if ((EmMotion_PassedFrameScaled(em, 50.0f, 0U) != 0) || (EmMotion_PassedFrameScaled(em, 102.0f, 0U) != 0)) {
            EmRemobra_SpawnGroundEffectNearGround(em, 1, 7, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_BACK_OFF):
        if (EmMotion_PassedFrameScaled(em, 52.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffectNearGround(em, 1, 7, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_TAKE_OFF):
        if (EmMotion_PassedFrameScaled(em, 138.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffect(em, 1, 4, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_SPIT_AIM):
        if (EmMotion_PassedFrameScaled(em, 78.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffectNearGround(em, 1, 7, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, REMOBRA_MOT_FALL_LAND):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffect(em, 3, 7, 2.0f);
        }
        break;
    case EM_MOTION_ID(0, 23):
        if (EmMotion_PassedFrameScaled(em, 164.0f, 0U) != 0) {
            EmRemobra_SpawnGroundEffect(em, 3, 7, 2.0f);
        }
        break;
    }
}

/*
 * Skips the first frame (fxStep 0 to 1), then runs EmRemobra_MotionEffects
 * every frame.
 */
void EmRemobra_UpdateMotionEffects(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmRemobra_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/* Moves the position by velX, velY, velZ rotated by rotY. */
void EmRemobra_MoveByVelocity(Em *em)
{
    f32 vel[3];

    Vec_Copy(vel, &em->velX);
    Vec_RotateY(vel, 6.2831855f * em->rotY / 65536.0f);
    em->posX += vel[0];
    em->posY += vel[1];
    em->posZ += vel[2];
}

/*
 * Starts the curves of the kind (kind 0: the vertical-speed and speed
 * curves from frame 0) and clears the velocity.
 */
void EmRemobra_StartCurves(Em *em, u8 kind)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    }
    Player_ClearVelocity(em);
}

/*
 * Sets velY from the vertical-speed curve of the kind at curveFrame;
 * returns 2 past the end of the curve, else 0.
 */
static u8 EmRemobra_CurveVelY(Em *em, EmWork *work)
{
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *curve;
    u8 ret;

    curve = EmRemobra_VelYCurves[((EmRemobraWork *)work)->curveKind];
    ret = 0;
    if (curve == NULL) {
        return ret;
    }
    frame = ((EmRemobraWork *)work)->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velY = curve->v;
    } else {
        i = 1;
        while (i != 0) {
            key = &curve[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame != 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 2;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * Sets velZ from the speed curve of the kind at curveFrame; returns 4 past
 * the end of the curve, else 0.
 */
static u8 EmRemobra_CurveSpeed(Em *em, EmWork *work)
{
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *curve;
    u8 ret;

    curve = EmRemobra_SpeedCurves[((EmRemobraWork *)work)->curveKind];
    ret = 0;
    if (curve == NULL) {
        return ret;
    }
    frame = ((EmRemobraWork *)work)->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velZ = curve->v;
    } else {
        i = 1;
        while (i != 0) {
            key = &curve[i];
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
 * Runs the started curves, advances curveFrame by the motion step and moves
 * the monster in the air (Em_MoveRotatedAccel); returns the end bits of the
 * curves.
 */
u8 EmRemobra_UpdateCurves(Em *em)
{
    EmRemobraWork *w = (EmRemobraWork *)&em->work;
    u8 ret = 0;

    if (w->curveVelY != 0) {
        ret |= EmRemobra_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmRemobra_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        Em_MoveRotatedAccel(em, &em->rotX);
    }
    return ret;
}

/*
 * Spawns effect 01 type 0 for the monster (the first two entries of
 * EmRemobra_Callbacks).
 */
void EmRemobra_SpawnShadow(u8 *em)
{
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/* The empty entry of EmRemobra_Callbacks. */
void EmRemobra_TableNop(void)
{
}

/* The file's initialised data, in the game's order */
EmCurveKey EmRemobra_VelYCurves_0[35] = {
#include "game/008CAC00.inc"
};
EmCurveKey *EmRemobra_VelYCurves[2] = {
    EmRemobra_VelYCurves_0, NULL,
};
EmCurveKey EmRemobra_SpeedCurves_0[35] = {
#include "game/008CAD20.inc"
};
EmCurveKey *EmRemobra_SpeedCurves[2] = {
    EmRemobra_SpeedCurves_0, NULL,
};
f32 EmRemobra_HopFrames[3] = {
#include "game/008CAE40.inc"
};
void *EmRemobra_Callbacks[8] = {
    EmRemobra_SpawnShadow, EmRemobra_SpawnShadow, EmRemobra_TableNop, EmRemobra_UpdateMotionEffects,
    GmEm_ReturnZero, EmRemobra_Reinit, NULL, NULL,
};
EmMotionRow gEmMotionRows_Remobra[101] = {
#include "game/008CAE70.inc"
};
