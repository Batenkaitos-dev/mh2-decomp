/*
 * Monster family: the dromes, the Velocidrome, the Gendrome and the Iodrome
 * (EM_KIND_VELOCIDROME, EM_KIND_GENDROME and EM_KIND_IODROME), the leaders
 * of the raptor packs; they share the Velociprey's motion table. From the
 * sounds of their motions (EmDrome_MotionSounds) to the curves that drive
 * their hops and leaps (EmDrome_StartCurves, EmDrome_UpdateCurves): Setup,
 * Init, the states of the nine groups and their dispatchers, the setters,
 * Update, EmDrome_ChangeState and the effects made with their motions.
 * em_drome.h holds the motions, states,
 * curve sets and the work area's own fields (EmDromeWork). The state
 * functions take the monster and the work area its state dispatcher passes
 * on (EmWork); Em.step is the step within the current state.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_drome.h"
#include "objattach.h"
#include "player.h"
#include "area.h"
#include "shadow.h"
#include "capsule.h"
#include "save.h"
#include "sound.h"
#include "engine_math.h"
#include "game_util.h"
#include "gm_em.h"
#include "gm_load.h"
#include "part_effects.h"
#include "vec_math.h"
#include "em_hermitaur.h"
#include "em_script.h"
/* prototypes of callees */

/* prototypes */
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmDrome_ChangeState, EmDrome_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void StunStars_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
extern EmTurnParams EmDrome_TurnParams[2];
void Em_EndAllStatus(Em *em);
void Em_AddHp(Em *em, s16 n);
s16 Player_IsState(void *obj, u8 state, u8 sub);
extern Vec3f EmDrome_CarveThrowPt __attribute__((aligned(8)));
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
/* A quest appearance point: the position and the point the monster faces. */
typedef struct EmDromeAppearPoint {
    f32 pos[3];
    f32 target[3];
} EmDromeAppearPoint;
extern EmDromeAppearPoint EmDrome_AppearPtForestHills10;
extern EmDromeAppearPoint EmDrome_AppearPtsForestHills2[2];
extern EmDromeAppearPoint EmDrome_AppearPtsForestHills1[4];
/* end of prototypes */

/*
 * Plays the sounds of the motion the drome family's monster started last
 * (work->prevMotion): per motion id, Em_PlaySeAtBoneOnFrame(em, frame,
 * sound, bone) for each sound and the frame it plays at. The numbers are
 * motion ids, frames, sound ids and bones: a table written as code.
 */
void EmDrome_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case EM_MOTION_ID(0, DROME_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame(em, 200, Sound_RandomSe(14, 2, 14, 2), DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 8, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 32, 2, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_HOP):
        Em_PlaySeAtBoneOnFrame(em, 20, 8, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 16, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 20, 3, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_HOP_UP):
        Em_PlaySeAtBoneOnFrame(em, 22, 8, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 22, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_HOP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 10, 1, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, DROME_MOT_TURN_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 6, 5, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 26, 4, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 42, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 48, 1, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_MOVE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 8, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 8, 4, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 26, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 32, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 64, 7, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_MOVE_ACTION_B):
    case EM_MOTION_ID(0, DROME_MOT_MOVE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 12, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 12, 4, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 30, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 36, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 64, 7, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_ALERT):
        Em_PlaySeAtBoneOnFrame(em, 68, 17, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 164, 18, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 34, 23, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 260, 23, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 288, 23, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_IDLE_B):
        Em_PlaySeAtBoneOnFrame(em, 118, 14, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame(em, 22, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 22, 8, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_LEAP_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 6, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 36, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 40, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 60, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 118, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 26, 16, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_POUNCE):
        Em_PlaySeAtBoneOnFrame(em, 22, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 22, 3, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 22, 8, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_POUNCE_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 6, 1, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_QUICK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 2, 16, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_SPIT):
        Em_PlaySeAtBoneOnFrame(em, 60, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 128, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 72, 28, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 60, 8, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame(em, 4, 9, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 30, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 28, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 54, 8, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_BACK):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_FORWARD):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_SIDE_B):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_SIDE_A):
        Em_PlaySeAtBoneOnFrame(em, 4, 11, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_BACK_LAND):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_FORWARD_LAND):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_SIDE_B_LAND):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_SIDE_A_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 15, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 24, 3, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 44, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 54, 2, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 88, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_DAZED):
        Em_PlaySeAtBoneOnFrame(em, 180, 9, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 132, 11, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 52, 10, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 68, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 86, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 132, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 140, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 4, 17, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 66, 17, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 130, 18, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 48, 23, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 122, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 162, 15, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_LIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 14, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_DIE):
        Em_PlaySeAtBoneOnFrame(em, 10, 13, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 14, 25, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_THREAT):
        Em_PlaySeAtBoneOnFrame(em, 32, 21, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 68, 21, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 104, 22, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 20, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 160, 23, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_CALL):
        Em_PlaySeAtBoneOnFrame(em, 10, 20, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 64, 20, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 6, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 122, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 134, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 8, 21, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 98, 22, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 22, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 58, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 106, 1, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 158, 0, DROME_BONE_ROOT);
        break;
    case EM_MOTION_ID(0, DROME_MOT_VS_BAIT):
        Em_PlaySeAtBoneOnFrame(em, 84, 24, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 126, 24, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 216, 26, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 258, 26, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 292, 27, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 156, 11, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 14, 0, DROME_BONE_ROOT);
        Em_PlaySeAtBoneOnFrame(em, 46, 2, DROME_BONE_ROOT);
        
    default:
        break;
    }
}

/*
 * Empty; the per-kind dispatch Em_TypeSetup calls it for the three kinds.
 */
void EmDrome_Setup(Em *em)
{
}

/*
 * Slots 0 and 1 of EmDrome_Callbacks: spawns the monster's shadow
 * (SHADOW_MODEL).
 */
void EmDrome_SpawnShadow(u8 *em)
{
    Shadow_Spawn((Player *) em, SHADOW_MODEL);
}

/*
 * Out of a quest (gQuest.stage 0) puts the monster at a start spot by area
 * (AREA_SNOWY_MOUNTAINS_NIGHT_9, area 13, which area.h doesn't name, and
 * AREA_FOREST_HILLS_10: a grid by slotNo; else a row by slotNo), then
 * DROME_MOT_IDLE, ground stance, hp/hpMax from Em_ComputeMaxHp,
 * scriptPending, clears blind, the pack call flag and hunted,
 * Em_InitParts. Velociprey's copy is EmVelociprey_Init.
 */
void EmDrome_Init(Em *em)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;

    f32 slotF;
    s16 maxHp;
    s32 dayId;
    u8 slotNo;

    if (gQuest.stage == 0) {
        dayId = Area_DayId(gScene.area) & 0xFFFF;
        switch (dayId) {
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case 13:
            em->posX = 5000.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9000.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case AREA_FOREST_HILLS_10:
            em->posX = 9522.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9559.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        default:
            slotNo = em->slotNo;
            slotF = (f32) (u32) slotNo;
            em->posX = 2400.0f + (150.0f * slotF);
            em->posY = 0.0f;
            em->posZ = 3600.0f;
            break;
        }
    }
    Em_SetMotionRow(em, DROME_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    w->countdown = 0;
    w->hunted = 0;
    Em_InitParts(em);
}

/*
 * EmDrome_Init then DROME_IDLE; GmEm (gm_em.c) starts the three kinds with
 * it.
 */
void EmDrome_Start(Em *em)
{
    EmDrome_Init(em);
    EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE, EM_SYNC_LOCAL);
}

/*
 * EmDrome_Init again (EmDrome_Callbacks slot 5).
 */
void EmDrome_Reinit(Em *em)
{
    EmDrome_Init(em);
}

/*
 * Ends a state: motionSpeed 1.2, ground stance, lookAtTarget 0,
 * scriptPending 1, then DROME_IDLE (mode 0); the arguments are unused.
 */
void EmDrome_EndState(Em *em, s32 unused1, s32 unused2)
{
    em->motionSpeed = 1.2f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE, EM_SYNC_LOCAL);
}

/*
 * Does nothing; the monster stays until the script or a hit changes the
 * state (EVENT2_TO_IDLE_A/B go here).
 */
void EmDrome_IdleNone(void)
{
}

/*
 * Ground stance, the idle motion (kept if it already plays), scriptPending
 * once it ends; runs DROME_IDLE and DROME_IDLE_WAIT.
 */
void EmDrome_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] != EM_MOTION_ID(0, DROME_MOT_IDLE)) {
            Em_SetMotion(em, DROME_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->scriptPending = 1;
        }
        break;
    }
}

/*
 * DROME_MOT_THREAT once, then EmDrome_EndState.
 */
void EmDrome_IdleThreat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_CALL; at frame 40 puts callObj (at its position, radius 1000,
 * its area, kind 0) into the sense list (EmSenseObjs_Add, gEmSenseObjs) for
 * the other raptors and sets callActive; removes it when the motion ends,
 * then EmDrome_EndState.
 */
void EmDrome_PackCall(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_CALL, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            Vec_Set((f32 *) &work->moveAngBY, em->posX, em->posY, em->posZ);
            work->moveAngCY = 0x447A0000;
            ((EmDromeWork *)work)->callObj.area = (s16) *(u8 *)&em->area;
            ((EmDromeWork *)work)->callObj.kind = 0;
            EmSenseObjs_Add((EmObj *)&work->moveAngBY);
            work->countdown = 1;
        }
        if (em->motion[0].playing == 0) {
            if (work->countdown != 0) {
                EmSenseObjs_Remove((EmObj *)&work->moveAngBY);
                work->countdown = 0;
            }
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_SHAKE_OFF once, then EmDrome_EndState.
 */
void EmDrome_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * The combat idle DROME_MOT_ALERT once, then EmDrome_EndState.
 */
void EmDrome_IdleAlert(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_GET_UP with noReact set (after the four knock-backs), then
 * EmDrome_EndState.
 */
void EmDrome_GetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_GET_UP, 0, 0);
        em->noReact = 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Lying under status 2 (variant 0) or 1 (variant 1): hit-react mode WAKE,
 * blind 2 each frame, DROME_MOT_COLLAPSE then DROME_MOT_LIE until em->timer
 * runs out, then DROME_IDLE_STATUS2_END or DROME_IDLE_STATUS1_END (mode 4; a
 * remote copy retries in step 3).
 */
void EmDrome_StatusCollapse(Em *em, EmWork *work, s32 variant)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            if (variant == 0) {
                EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            if (variant == 0) {
                EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Blind 2, DROME_MOT_GET_UP, clears blind and EmDrome_EndState; variant unused (the
 * setter ends status 2 for DROME_IDLE_STATUS2_END, status 1 for
 * DROME_IDLE_STATUS1_END).
 */
void EmDrome_StatusGetUp(Em *em, EmWork *work, s32 variant)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_DAZED (kept if playing) until Em.blindTimer is 0, then EmDrome_EndState.
 */
void EmDrome_Blinded(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] != EM_MOTION_ID(0, DROME_MOT_DAZED)) {
            Em_SetMotion(em, DROME_MOT_DAZED, 0, 0);
        }
        break;
    case 1:
        if (em->blindTimer == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_ALERT once (the setter ends status 4), then EmDrome_EndState.
 */
void EmDrome_Status4End(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_ALERT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_IDLE_B once, then EmDrome_EndState; DROME_DEFEAT_REAPPEAR's state
 * outside the four areas with an entrance hop.
 */
void EmDrome_IdleReappear(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_IDLE_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_SHAKE_OFF once after DROME_REACT_HOLD2 (the setter clears
 * holdTimer2), then EmDrome_EndState.
 */
void EmDrome_Hold2ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_SHAKE_OFF once after the stun (the setter ends status 5), then
 * EmDrome_EndState.
 */
void EmDrome_StunEnd(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_THREAT once as the rage starts (the setter calls Em_StartRage),
 * then EmDrome_EndState.
 */
void EmDrome_RageThreat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_CALL once (no call object) as the pending rage starts
 * (Em_StartPendingRage in the setter), then EmDrome_EndState.
 */
void EmDrome_PendingRageCall(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_CALL, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_IDLE state (enum EmDromeIdleState) through its
 * state function.
 */
void EmDrome_RunIdleState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DROME_IDLE_NONE:
        EmDrome_IdleNone();
        break;
    case DROME_IDLE:
        EmDrome_Idle(em);
        break;
    case DROME_IDLE_THREAT:
        EmDrome_IdleThreat(em);
        break;
    case DROME_IDLE_PACK_CALL:
        EmDrome_PackCall(em, work);
        break;
    case DROME_IDLE_SHAKE_OFF:
        EmDrome_ShakeOff(em);
        break;
    case DROME_IDLE_ALERT:
        EmDrome_IdleAlert(em);
        break;
    case DROME_IDLE_GET_UP:
        EmDrome_GetUp(em);
        break;
    case DROME_IDLE_STATUS2:
        EmDrome_StatusCollapse(em, work, 0);
        break;
    case DROME_IDLE_STATUS2_END:
        EmDrome_StatusGetUp(em, work, 0);
        break;
    case DROME_IDLE_BLINDED:
        EmDrome_Blinded(em);
        break;
    case DROME_IDLE_STATUS4_END:
        EmDrome_Status4End(em);
        break;
    case DROME_IDLE_REAPPEAR:
        EmDrome_IdleReappear(em);
        break;
    case DROME_IDLE_WAIT:
        EmDrome_Idle(em);
        break;
    case DROME_IDLE_STATUS1:
        EmDrome_StatusCollapse(em, work, 1);
        break;
    case DROME_IDLE_STATUS1_END:
        EmDrome_StatusGetUp(em, work, 1);
        break;
    case DROME_IDLE_HOLD2_SHAKE_OFF:
        EmDrome_Hold2ShakeOff(em);
        break;
    case DROME_IDLE_STUN_END:
        EmDrome_StunEnd(em);
        break;
    case DROME_IDLE_RAGE:
        EmDrome_RageThreat(em);
        break;
    case DROME_IDLE_PENDING_RAGE:
        EmDrome_PendingRageCall(em);
        break;
    }
}

/*
 * Turns to the target with EmDrome_TurnParams[0]
 * (Em_StartTurnToTarget/Em_TurnStep), then EmDrome_EndState.
 */
void EmDrome_TurnToTarget(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDrome_TurnParams[0]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDrome_TurnParams[0]) == 1) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_WALK to the goal (Em_StepToGoal 128), keeping the start
 * position and the frames walked; reaching the goal within 100 of the start
 * after more than 60 frames (stuck) starts DROME_HOP_FORWARD, else EmDrome_EndState.
 */
void EmDrome_Walk(Em *em, EmWork *work)
{
    f32 startPos[3];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DROME_MOT_WALK, 0, 0);
        work->startX = em->posX;
        work->startZ = em->posZ;
        work->walkFrames = 0;
        break;
    case 1:
        work->walkFrames++;
        if (Em_StepToGoal(em, 128, 0) == 1) {
            if (em->remote == 0) {
                startPos[0] = work->startX;
                startPos[1] = em->posY;
                startPos[2] = work->startZ;
                if (Vec_DistanceXZ(&em->posX, startPos) < 100.0f && work->walkFrames > 60) {
                    EmDrome_ChangeState(em, EM_GROUP_FLY, DROME_HOP_FORWARD, EM_SYNC_OWNER_FLAGGED);
                    break;
                }
                EmDrome_EndState(em, 0, 0);
            } else {
                EmDrome_EndState(em, 0, 0);
            }
        }
        break;
    }
}

/*
 * DROME_MOT_MOVE_ACTION_A once (dust at frame 12), then EmDrome_EndState.
 */
void EmDrome_MoveActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_MOVE_ACTION_B once (dust at frame 14), then EmDrome_EndState; leaves the
 * stance as it is.
 */
void EmDrome_MoveActionB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_MOVE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_MOVE_ACTION_C once (dust at frame 14), then EmDrome_EndState.
 */
void EmDrome_MoveActionC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_MOVE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As EmDrome_TurnToTarget with EmDrome_TurnParams[1] (the same values as row
 * 0 in the data); the setter picks it for DROME_MOVE_TURN in combat.
 */
void EmDrome_TurnToTargetAlert(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_StartTurnToTarget(em, &EmDrome_TurnParams[1]);
        break;
    case 1:
        if (Em_TurnStep(em, &EmDrome_TurnParams[1]) == 1) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_MOVE state (enum EmDromeMoveState) through its
 * state function.
 */
void EmDrome_RunMoveState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DROME_MOVE_TURN:
        EmDrome_TurnToTarget(em, work);
        break;
    case DROME_MOVE_WALK:
        EmDrome_Walk(em, work);
        break;
    case DROME_MOVE_ACTION_A:
        EmDrome_MoveActionA(em, work);
        break;
    case DROME_MOVE_ACTION_B:
        EmDrome_MoveActionB(em, work);
        break;
    case DROME_MOVE_ACTION_C:
        EmDrome_MoveActionC(em, work);
        break;
    case DROME_MOVE_TURN_ALERT:
        EmDrome_TurnToTargetAlert(em, work);

    default:
        break;
    }
}

/*
 * DROME_MOT_HOP on the curve DROME_CURVE_HOP_FORWARD (up and forward), airborne from frame 10;
 * when a curve has ended at the floor DROME_MOT_HOP_LAND, then EmDrome_EndState.
 */
void EmDrome_HopForward(Em *em, EmWork *work)
{
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DROME_MOT_HOP, 0, 0);
        EmDrome_StartCurves(em, DROME_CURVE_HOP_FORWARD);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmDrome_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DROME_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_HOP_UP on the curve DROME_CURVE_HOP_UP (vertical only), airborne from frame 22,
 * then the landing as EmDrome_HopForward.
 */
void EmDrome_HopUp(Em *em, EmWork *work)
{
    s32 ended;

    ended = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DROME_MOT_HOP_UP, 0, 0);
        EmDrome_StartCurves(em, DROME_CURVE_HOP_UP);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmDrome_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DROME_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_FLY state (enum EmDromeHopState) through its
 * state function.
 */
void EmDrome_RunHopState(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDrome_HopForward(em, work);
        break;
    case 1:
        EmDrome_HopUp(em, work);
        break;
    }
}

/*
 * DROME_MOT_STRIKE turning to the goal (128), the kind's DromeAttack
 * (Velocidrome 1, Gendrome 13, Iodrome 7) at frame 40, then EmDrome_EndState.
 */
void EmDrome_AttackStrikeA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIDROME:
                DromeAttack_Create(em, 1);
                break;
            case EM_KIND_GENDROME:
                DromeAttack_Create(em, 13);
                break;
            case EM_KIND_IODROME:
                DromeAttack_Create(em, 7);
                break;
            }
        }
        Em_TurnToGoal(em, 128, 0);
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_POUNCE on the curve DROME_CURVE_POUNCE, airborne from frame 24, turning to the
 * goal in the air; DROME_MOT_POUNCE_LAND at the floor, then EmDrome_EndState (its
 * DromeAttack 2 comes from EmDrome_MotionEffects).
 */
void EmDrome_AttackPounce(Em *em)
{
    s32 ended;
    u8 step;

    step = em->step;
    ended = 0;
    switch (step) {
    case 0:
        em->step = step + 1;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_POUNCE, 0, 0);
        EmDrome_StartCurves(em, DROME_CURVE_POUNCE);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 24.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmDrome_UpdateCurves(em) & 0xFF;
        }
        if ((em->hasGoal != 0) && (em->stance == EM_STANCE_AIR)) {
            Em_TurnToGoal(em, 64, 0);
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DROME_MOT_POUNCE_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_LEAP with velY 75, velZ 50 (Em_MoveRotatedAccel from frame 22,
 * velZ held while a wall ahead stops it, at least 10), the kind's
 * DromeAttack (3, 8, 9) at frame 40; DROME_MOT_LEAP_LAND at the floor, then
 * EmDrome_EndState.
 */
void EmDrome_AttackLeapMediumA(Em *em, EmWork *work)
{
    EmDromeWork *dromeWork = (EmDromeWork *)work;
    u8 airborne;

    airborne = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        em->accelZ = -1.5f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        dromeWork->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIDROME:
                DromeAttack_Create(em, 3);
                break;
            case EM_KIND_GENDROME:
                DromeAttack_Create(em, 8);
                break;
            case EM_KIND_IODROME:
                DromeAttack_Create(em, 9);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            airborne = 1;
            if (em->wallHitDirs & 0xF000000F) {
                dromeWork->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (dromeWork->wallVelZ != 0.0f) {
                em->velZ = dromeWork->wallVelZ;
                dromeWork->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (airborne && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, DROME_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_QUICK_STRIKE, the kind's DromeAttack (5, 10, 11) at frame 12,
 * then EmDrome_EndState.
 */
void EmDrome_AttackQuickStrikeA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_QUICK_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIDROME:
                DromeAttack_Create(em, 5);
                break;
            case EM_KIND_GENDROME:
                DromeAttack_Create(em, 10);
                break;
            case EM_KIND_IODROME:
                DromeAttack_Create(em, 11);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As LeapMediumA with velZ 100, accelZ -0.75 and turning to the goal (256)
 * in the air; the kind's DromeAttack (3, 8, 9).
 */
void EmDrome_AttackLeapLongA(Em *em, EmWork *work)
{
    EmDromeWork *dromeWork = (EmDromeWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 100.0f;
        em->accelZ = -0.75f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        dromeWork->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIDROME:
                DromeAttack_Create(em, 3);
                break;
            case EM_KIND_GENDROME:
                DromeAttack_Create(em, 8);
                break;
            case EM_KIND_IODROME:
                DromeAttack_Create(em, 9);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                dromeWork->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (dromeWork->wallVelZ != 0.0f) {
                em->velZ = dromeWork->wallVelZ;
                dromeWork->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, DROME_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As LeapLongA with velY 150, accelY -20, velZ 25; the kind's DromeAttack
 * (3, 8, 9).
 */
void EmDrome_AttackLeapHighA(Em *em, EmWork *work)
{
    EmDromeWork *dromeWork = (EmDromeWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 150.0f;
        em->accelY = -20.0f;
        em->velZ = 25.0f;
        em->accelZ = -3.0f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        dromeWork->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            switch (em->type) {
            case EM_KIND_VELOCIDROME:
                DromeAttack_Create(em, 3);
                break;
            case EM_KIND_GENDROME:
                DromeAttack_Create(em, 8);
                break;
            case EM_KIND_IODROME:
                DromeAttack_Create(em, 9);
                break;
            }
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                dromeWork->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (dromeWork->wallVelZ != 0.0f) {
                em->velZ = dromeWork->wallVelZ;
                dromeWork->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, DROME_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_SPIT; at frame 80 an EMRANGED_PREY_SPIT from DROME_BONE_MOUTH, variant 1
 * (variant arg 0) or 3 (variant arg 1: twice the speed), then EmDrome_EndState.
 */
void EmDrome_AttackSpit(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_SPIT, 0, 0);
        break;
    case 1:
        switch (variant) {
        case 0:
        default:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, DROME_BONE_MOUTH, EMRANGED_PREY_SPIT, 1, 63717, 0);
            }
            break;
        case 1:
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmRangedAttack_CreateAtBone(em, DROME_BONE_MOUTH, EMRANGED_PREY_SPIT, 3, 63717, 0);
            }
            break;
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As StrikeA with DromeAttack 6 for every kind, turning only with a goal.
 */
void EmDrome_AttackStrikeB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            DromeAttack_Create(em, 6);
        }
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 128, 0);
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As LeapMediumA with DromeAttack 3 for every kind.
 */
void EmDrome_AttackLeapMediumB(Em *em, EmWork *work)
{
    EmDromeWork *dromeWork = (EmDromeWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        em->accelZ = -1.5f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        dromeWork->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            DromeAttack_Create(em, 3);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                dromeWork->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (dromeWork->wallVelZ != 0.0f) {
                em->velZ = dromeWork->wallVelZ;
                dromeWork->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, DROME_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As QuickStrikeA with DromeAttack 5 for every kind.
 */
void EmDrome_AttackQuickStrikeB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_QUICK_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            DromeAttack_Create(em, 5);
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As LeapLongA with DromeAttack 3 for every kind.
 */
void EmDrome_AttackLeapLongB(Em *em, EmWork *work)
{
    EmDromeWork *dromeWork = (EmDromeWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 100.0f;
        em->accelZ = -0.75f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        dromeWork->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            DromeAttack_Create(em, 3);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                dromeWork->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (dromeWork->wallVelZ != 0.0f) {
                em->velZ = dromeWork->wallVelZ;
                dromeWork->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, DROME_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As LeapHighA with DromeAttack 3 for every kind.
 */
void EmDrome_AttackLeapHighB(Em *em, EmWork *work)
{
    EmDromeWork *dromeWork = (EmDromeWork *)work;
    u8 landed;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 150.0f;
        em->accelY = -20.0f;
        em->velZ = 25.0f;
        em->accelZ = -3.0f;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        dromeWork->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            DromeAttack_Create(em, 3);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngX = 0;
            work->accelAngY = em->rotY;
            work->accelAngZ = 0;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                dromeWork->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (dromeWork->wallVelZ != 0.0f) {
                em->velZ = dromeWork->wallVelZ;
                dromeWork->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (em->hasGoal != 0 && em->stance == EM_STANCE_AIR) {
            Em_TurnToGoal(em, 256, 0);
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, DROME_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_LEAP with DromeAttack 3; when the goal is higher, velZ 100 and
 * velY computed to reach 1000 above the goal height over the XZ distance
 * (em->timer frames), else the medium leap.
 */
void EmDrome_AttackLeapToGoal(Em *em, EmWork *work)
{
    EmDromeWork *dromeWork = (EmDromeWork *)work;
    u8 landed;
    f32 goalDist;

    landed = 0;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        if (!(em->posY < em->goal[1])) {
            em->velY = 75.0f;
            em->accelY = -10.0f;
            em->velZ = 50.0f;
            em->accelZ = -1.5f;
        } else {
            goalDist = Vec_DistanceXZ(&em->posX, em->goal);
            em->velZ = 100.0f;
            em->accelY = -10.0f;
            em->timer = goalDist / em->velZ;
            em->velY = (1000.0f + em->goal[1] - em->posY) / (f32) em->timer - (em->accelY * (f32) em->timer) / 2.0f;
        }
        work->accelAngX = 0;
        work->accelAngZ = 0;
        work->pitchStep = 0;
        work->turnSpeed = 0;
        work->rollSpeed = 0;
        work->unk048 = 0;
        work->unk04C = 0;
        dromeWork->unk050 = 0;
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            DromeAttack_Create(em, 3);
        }
        if (EmMotion_ReachedFrame(em, 0, 22.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            landed = 1;
            if (em->wallHitDirs & 0xF000000F) {
                dromeWork->wallVelZ = em->velZ;
                em->velZ = 0.0f;
            }
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (dromeWork->wallVelZ != 0.0f) {
                em->velZ = dromeWork->wallVelZ;
                dromeWork->wallVelZ = 0.0f;
            }
            if (em->velZ < 10.0f) {
                em->velZ = 10.0f;
                em->accelZ = 0.0f;
            }
        }
        if (landed && em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, DROME_MOT_LEAP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * As StrikeB with DromeAttack 12.
 */
void EmDrome_AttackStrikeC(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 40.0f, 0U) != 0) {
            DromeAttack_Create(em, 12);
        }
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 128, 0);
        }
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_ATTACK state (enum EmDromeAttackState) through
 * its state function.
 */
void EmDrome_RunAttackState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DROME_ATK_STRIKE_A:
        EmDrome_AttackStrikeA(em, work);
        break;
    case DROME_ATK_POUNCE:
        EmDrome_AttackPounce(em);
        break;
    case DROME_ATK_LEAP_MEDIUM_A:
        EmDrome_AttackLeapMediumA(em, work);
        break;
    case DROME_ATK_QUICK_STRIKE_A:
        EmDrome_AttackQuickStrikeA(em, work);
        break;
    case DROME_ATK_LEAP_LONG_A:
        EmDrome_AttackLeapLongA(em, work);
        break;
    case DROME_ATK_LEAP_HIGH_A:
        EmDrome_AttackLeapHighA(em, work);
        break;
    case DROME_ATK_SPIT:
        EmDrome_AttackSpit(em, work, 0);
        break;
    case DROME_ATK_STRIKE_B:
        EmDrome_AttackStrikeB(em);
        break;
    case DROME_ATK_LEAP_MEDIUM_B:
        EmDrome_AttackLeapMediumB(em, work);
        break;
    case DROME_ATK_QUICK_STRIKE_B:
        EmDrome_AttackQuickStrikeB(em);
        break;
    case DROME_ATK_LEAP_LONG_B:
        EmDrome_AttackLeapLongB(em, work);
        break;
    case DROME_ATK_LEAP_HIGH_B:
        EmDrome_AttackLeapHighB(em, work);
        break;
    case DROME_ATK_LEAP_TO_GOAL:
        EmDrome_AttackLeapToGoal(em, work);
        break;
    case DROME_ATK_STRIKE_C:
        EmDrome_AttackStrikeC(em);
        break;
    case DROME_ATK_SPIT_FAST:
        EmDrome_AttackSpit(em, work, 1);
        break;
    }
}

/*
 * DROME_MOT_FLINCH, Em_RestartScript, then EmDrome_EndState.
 */
void EmDrome_Flinch(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * DROME_MOT_KNOCKED_BACK flung backward (velY 23.35, velZ -27, accelY -2
 * along rotY) from frame 10, DROME_MOT_KNOCKED_BACK_LAND at the floor, then
 * DROME_IDLE_GET_UP.
 */
void EmDrome_KnockedBack(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_KNOCKED_BACK, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, DROME_MOT_KNOCKED_BACK_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * As EmDrome_KnockedBack along rotY + half a turn with
 * DROME_MOT_KNOCKED_FORWARD(_LAND); turns rotY by -0x555 a frame until the
 * landing reaches frame 26.
 */
void EmDrome_KnockedForward(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_KNOCKED_FORWARD, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + 0x7FFF + 1) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, DROME_MOT_KNOCKED_FORWARD_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY -= 0x555;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * As EmDrome_KnockedBack along rotY - ANGLE_TURN / 4 with
 * DROME_MOT_KNOCKED_SIDE_A(_LAND); rotY -0x2AA a frame until frame 26.
 */
void EmDrome_KnockedSideA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_KNOCKED_SIDE_A, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, DROME_MOT_KNOCKED_SIDE_A_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY -= 0x2AA;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * As EmDrome_KnockedBack along rotY + ANGLE_TURN / 4 with
 * DROME_MOT_KNOCKED_SIDE_B(_LAND); rotY +0x2AA a frame until frame 26.
 */
void EmDrome_KnockedSideB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_KNOCKED_SIDE_B, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            Em_MoveRotatedAccel(em, &work->accelAngX);
            if (em->posY <= em->floorY) {
                em->step++;
                em->stance = EM_STANCE_GROUND;
                Em_SetMotion(em, DROME_MOT_KNOCKED_SIDE_B_LAND, 0, 0);
                break;
            }
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) == 0) {
            em->rotY += 0x2AA;
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_GET_UP, EM_SYNC_LOCAL);
        }
        break;
    }
}

/*
 * DROME_MOT_DAZED once, Em_RestartScript, then EmDrome_EndState.
 */
void EmDrome_Daze(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_DAZED, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * hit-react mode STATUS, DROME_MOT_STATUS4 and the status-4 effect each
 * frame until em->timer runs out, then DROME_IDLE_STATUS4_END (mode 4).
 */
void EmDrome_Status4(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_STATUS4, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS4_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS4_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Em_SpawnStatus4Effect(em, 2);
}

/*
 * Em_SetHold2 each frame; DROME_MOT_FLINCH without root motion to frame 40,
 * then DROME_MOT_STATUS4 until em->timer runs out, then
 * DROME_IDLE_HOLD2_SHAKE_OFF (mode 4).
 */
void EmDrome_Hold2(Em *em, EmWork *work)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_FLINCH, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 40.0f) != 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_STATUS4, 4, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_HOLD2_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * DROME_MOT_DAZED with stun stars at DROME_BONE_STUN_STARS for em->timer frames, then
 * DROME_IDLE_STUN_END (mode 4).
 */
void EmDrome_Stun(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, DROME_MOT_DAZED, 0, 0);
        StunStars_CreateEm(em, 1, DROME_BONE_STUN_STARS, *(s16 *)&em->timer, 50.0f, 1.0f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_REACT state (enum EmDromeReactState) through
 * its state function.
 */
void EmDrome_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DROME_REACT_FLINCH:
        EmDrome_Flinch(em, work);
        break;
    case DROME_REACT_KNOCKED_BACK:
        EmDrome_KnockedBack(em, work);
        break;
    case DROME_REACT_KNOCKED_FORWARD:
        EmDrome_KnockedForward(em, work);
        break;
    case DROME_REACT_KNOCKED_SIDE_A:
        EmDrome_KnockedSideA(em, work);
        break;
    case DROME_REACT_KNOCKED_SIDE_B:
        EmDrome_KnockedSideB(em, work);
        break;
    case DROME_REACT_DAZE:
        EmDrome_Daze(em, work);
        break;
    case DROME_REACT_STATUS4:
        EmDrome_Status4(em, work);
        break;
    case DROME_REACT_HOLD2:
        EmDrome_Hold2(em, work);
        break;
    case DROME_REACT_STUN:
        EmDrome_Stun(em, work);

    default:
        break;
    }
}

/*
 * Out of combat; sets hunted, QuestEm_Hunted; flung as EmDrome_KnockedBack (airborne
 * at once), DROME_MOT_DIE with field collision off, then DROME_MOT_DEAD as a
 * carcass with a carve point (EmMine_Create) for 2700 frames, a 60-frame
 * fade-out, then DROME_DEFEAT_REAPPEAR.
 */
void EmDrome_DieBack(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->hunted = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, DROME_MOT_KNOCKED_BACK, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = em->rotY;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DROME_MOT_KNOCKED_BACK_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_DEAD, 0, 0);
            em->timer = 2700;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * As EmDrome_DieBack, flung along rotY + ANGLE_TURN / 2 with
 * DROME_MOT_KNOCKED_FORWARD(_LAND).
 */
void EmDrome_DieForward(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->hunted = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, DROME_MOT_KNOCKED_FORWARD, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + ANGLE_TURN / 2) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DROME_MOT_KNOCKED_FORWARD_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_DEAD, 0, 0);
            em->timer = 2700;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * As EmDrome_DieBack, flung along rotY - ANGLE_TURN / 4 with
 * DROME_MOT_KNOCKED_SIDE_A(_LAND).
 */
void EmDrome_DieSideA(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->hunted = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, DROME_MOT_KNOCKED_SIDE_A, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY - ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DROME_MOT_KNOCKED_SIDE_A_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_DEAD, 0, 0);
            em->timer = 2700;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * As EmDrome_DieBack, flung along rotY + ANGLE_TURN / 4 with
 * DROME_MOT_KNOCKED_SIDE_B(_LAND).
 */
void EmDrome_DieSideB(Em *em, EmWork *work)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        work->hunted = 1;
        QuestEm_Hunted(em);
        Em_SetMotion(em, DROME_MOT_KNOCKED_SIDE_B, 0, 0);
        work->accelAngX = 0;
        work->accelAngY = (em->rotY + ANGLE_TURN / 4) & 0xFFFF;
        work->accelAngZ = 0;
        Player_ClearVelocity(em);
        em->velY = 23.35f;
        em->velZ = -27.0f;
        em->accelY = -2.0f;
        em->stance = EM_STANCE_AIR;
        break;
    case 1:
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DROME_MOT_KNOCKED_SIDE_B_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_DIE, 0, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_DEAD, 0, 0);
            em->timer = 2700;
            EmMine_Create(em, 0);
        }
        break;
    case 4:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->step++;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * If QuestEm_CanAppear, puts the monster at an appear point by area
 * (AREA_FOREST_HILLS_10, _2 or _1, by stateRand, facing its target point;
 * AREA_JUNGLE_7: 1300 up, in the air) and starts DROME_EVENT2_HOP_IN (the
 * forest), DROME_EVENT2_LEAP_DOWN_IN (the jungle) or DROME_IDLE_REAPPEAR;
 * otherwise
 * advances taskPhase (the slot finishes).
 */
void EmDrome_Reappear(Em *em, EmWork *work)
{
    f32 target[4];
    u16 pointNo;

    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            switch (Area_DayId(em->area)) {
            case AREA_FOREST_HILLS_10:
                Vec_Set(&em->posX, EmDrome_AppearPtForestHills10.pos[0], EmDrome_AppearPtForestHills10.pos[1], EmDrome_AppearPtForestHills10.pos[2]);
                Vec_Set(&em->prevPosX, EmDrome_AppearPtForestHills10.pos[0], EmDrome_AppearPtForestHills10.pos[1], EmDrome_AppearPtForestHills10.pos[2]);
                Vec_Set(target, EmDrome_AppearPtForestHills10.target[0], EmDrome_AppearPtForestHills10.target[1], EmDrome_AppearPtForestHills10.target[2]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                break;
            case AREA_FOREST_HILLS_2:
                pointNo = em->stateRand % 2;
                Vec_Set(&em->posX, EmDrome_AppearPtsForestHills2[pointNo].pos[0], EmDrome_AppearPtsForestHills2[pointNo].pos[1], EmDrome_AppearPtsForestHills2[pointNo].pos[2]);
                Vec_Set(&em->prevPosX, EmDrome_AppearPtsForestHills2[pointNo].pos[0], EmDrome_AppearPtsForestHills2[pointNo].pos[1], EmDrome_AppearPtsForestHills2[pointNo].pos[2]);
                Vec_Set(target, EmDrome_AppearPtsForestHills2[pointNo].target[0], EmDrome_AppearPtsForestHills2[pointNo].target[1], EmDrome_AppearPtsForestHills2[pointNo].target[2]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                break;
            case AREA_FOREST_HILLS_1:
                pointNo = em->stateRand % 4;
                Vec_Set(&em->posX, EmDrome_AppearPtsForestHills1[pointNo].pos[0], EmDrome_AppearPtsForestHills1[pointNo].pos[1], EmDrome_AppearPtsForestHills1[pointNo].pos[2]);
                Vec_Set(&em->prevPosX, EmDrome_AppearPtsForestHills1[pointNo].pos[0], EmDrome_AppearPtsForestHills1[pointNo].pos[1], EmDrome_AppearPtsForestHills1[pointNo].pos[2]);
                Vec_Set(target, EmDrome_AppearPtsForestHills1[pointNo].target[0], EmDrome_AppearPtsForestHills1[pointNo].target[1], EmDrome_AppearPtsForestHills1[pointNo].target[2]);
                em->rotY = Math_AngleXZ(&em->posX, target) & 0xFFFF;
                break;
            case AREA_JUNGLE_7:
                em->posY += 1300.0f;
                em->stance = EM_STANCE_AIR;
                em->rotY = 0;
                break;
            }
            switch (Area_DayId(em->area)) {
            case AREA_FOREST_HILLS_10:
                EmDrome_ChangeState(em, EM_GROUP_EVENT2, DROME_EVENT2_HOP_IN, EM_SYNC_LOCAL);
                break;
            case AREA_FOREST_HILLS_2:
                EmDrome_ChangeState(em, EM_GROUP_EVENT2, DROME_EVENT2_HOP_IN, EM_SYNC_LOCAL);
                break;
            case AREA_FOREST_HILLS_1:
                EmDrome_ChangeState(em, EM_GROUP_EVENT2, DROME_EVENT2_HOP_IN, EM_SYNC_LOCAL);
                break;
            case AREA_JUNGLE_7:
                EmDrome_ChangeState(em, EM_GROUP_EVENT2, DROME_EVENT2_LEAP_DOWN_IN, EM_SYNC_LOCAL);
                break;
            default:
                EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_REAPPEAR, EM_SYNC_LOCAL);
                break;
            }
            break;
        }
        em->taskPhase++;
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_DEFEAT state (enum EmDromeDefeatState) through
 * its state function.
 */
void EmDrome_RunDefeatState(Em *em, s32 work)
{
    em->noHitTimer = 10;
    em->noPushTimer = 10;
    switch (em->state) {
    case DROME_DEFEAT_DIE_BACK:
        EmDrome_DieBack(em, (EmWork *) work);
        break;
    case DROME_DEFEAT_DIE_FORWARD:
        EmDrome_DieForward(em, (EmWork *) work);
        break;
    case DROME_DEFEAT_DIE_SIDE_A:
        EmDrome_DieSideA(em, (EmWork *) work);
        break;
    case DROME_DEFEAT_DIE_SIDE_B:
        EmDrome_DieSideB(em, (EmWork *) work);
        break;
    case DROME_DEFEAT_REAPPEAR:
        EmDrome_Reappear(em, (EmWork *) work);
        break;
    }
}

/*
 * Em_SetCaptured each frame; Em_BeginStatus1, clears poison,
 * DROME_MOT_COLLAPSE then DROME_MOT_LIE and QuestEm_Captured; once it can
 * appear again, 150 frames, then a 150-frame fade and DROME_DEFEAT_REAPPEAR.
 */
void EmDrome_Captured(Em *em, EmWork *work)
{
    s32 fadeTimer;
    s32 prevTimer;

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_COLLAPSE, 0, 0);
        em->timer = 150;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_LIE, 0, 0);
            QuestEm_Captured(em);
        }
        break;
    case 2:
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
                em->timer = 150;
                break;
            }
        }
        break;
    case 3:
        prevTimer = em->timer;
        if (prevTimer > 0) {
            em->timer = prevTimer - 1;
        }
        fadeTimer = em->timer;
        if (fadeTimer <= 0) {
            if (em->remote == 0) {
                EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        } else {
            em->alpha = (f32) fadeTimer / 150.0f;
        }
        break;
    }
}

/*
 * The Velocidrome scene in AREA_JUNGLE_7: unhittable and not sensing, placed at a
 * fixed spot, idles 90 frames, walks to a fixed goal (300 frames or within
 * 100), idles, DROME_MOT_ALERT, DROME_MOT_THREAT, DROME_MOT_ALERT, waits for
 * EVENT_FLAG_VELOCIDROME_SCENE, then EmDrome_EndState.
 */
void EmDrome_EventJungleScene(Em *em, EmWork *work)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        GmEm_ResetSize(em);
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 13007.0f;
        em->posY = 0.0f;
        em->posZ = 3167.0f;
        em->goal[0] = 13360.0f;
        em->goal[1] = 0.0f;
        em->goal[2] = 5724.0f;
        em->rotY = 0xC00;
        Em_SetMotion(em, DROME_MOT_IDLE, 0, 0);
        em->timer = 90;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_WALK, 0, 0);
            em->timer = 300;
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_IDLE, 0, 0);
            em->timer = 90;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_ALERT, 0, 0);
            em->timer = 60;
        }
        break;
    case 4:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_THREAT, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_ALERT, 0, 0);
        }
        break;
    case 6:
        if (Save_TestEventFlag(EVENT_FLAG_VELOCIDROME_SCENE) == 1) {
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_EVENT state (enum EmDromeEventState) through
 * its state function.
 */
void EmDrome_RunEventState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DROME_EVENT_CAPTURED:
        EmDrome_Captured(em, work);
        break;
    case DROME_EVENT_JUNGLE_SCENE:
        EmDrome_EventJungleScene(em, work);
        break;
    }
}

/*
 * Not sensing, no script; DROME_MOT_HOP on the curve DROME_CURVE_HOP_FORWARD into the area,
 * DROME_MOT_HOP_LAND, then noHitTimer 5 and EmDrome_EndState.
 */
void EmDrome_HopIn(Em *em, EmWork *work)
{
    s32 ended;

    ended = 0;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->scriptPending = 0;
        Em_SetMotion(em, DROME_MOT_HOP, 0, 0);
        EmDrome_StartCurves(em, DROME_CURVE_HOP_FORWARD);
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            ended = EmDrome_UpdateCurves(em) & 0xFF;
        }
        if ((ended != 0) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, DROME_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->senseDelay = 0;
            em->noHitTimer = 5;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Not sensing; DROME_MOT_HOP with velY 70, velZ 50 (falling at most 50, velZ
 * at least 20) down from 1300 above the floor, DROME_MOT_HOP_LAND, then
 * EmDrome_EndState.
 */
void EmDrome_LeapDownIn(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotion(em, DROME_MOT_HOP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 70.0f;
        em->velZ = 50.0f;
        em->accelY = -4.0f;
        em->accelZ = -1.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 10.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (em->velY < -50.0f) {
            em->velY = -50.0f;
            em->accelY = 0.0f;
        }
        if (em->velZ < 20.0f) {
            em->velZ = 20.0f;
            em->accelZ = 0.0f;
        }
        if ((em->velY < 0.0f) && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            Em_SetMotion(em, DROME_MOT_HOP_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->senseDelay = 0;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Changes to DROME_IDLE_NONE at once.
 */
void EmDrome_Event2ToIdleA(Em *em, EmWork *work)
{
    u8 step;

    step = em->step;
    switch (step) {
    case 0:
        em->step = step + 1;
        EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_NONE, EM_SYNC_LOCAL);
        break;
    }
}

/*
 * The same as EmDrome_Event2ToIdleA; nothing tells them apart.
 */
void EmDrome_Event2ToIdleB(Em *em, EmWork *work)
{
    u8 step;

    step = em->step;
    switch (step) {
    case 0:
        em->step = step + 1;
        EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_NONE, EM_SYNC_LOCAL);
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_EVENT2 state (enum EmDromeEvent2State) through
 * its state function.
 */
void EmDrome_RunEvent2State(Em *em, EmWork *work)
{
    switch (em->state) {
    case 0:
        EmDrome_HopIn(em, work);
        break;
    case 1:
        EmDrome_LeapDownIn(em, work);
        break;
    case 2:
        EmDrome_Event2ToIdleA(em, work);
        break;
    case 3:
        EmDrome_Event2ToIdleB(em, work);
        break;
    }
}

/*
 * The versus end (no carving): waits Versus_DefeatCamera, DROME_MOT_VS_BAIT
 * with a VersusBait from its side player at EmDrome_CarveThrowPt at frame
 * 10, then Versus_RouteEmByTeam.
 */
void EmDrome_VsEatBait(Em *em, EmWork *work)
{
    Vec3f throwPt;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_VS_BAIT, 0, 0);
            em->timer = 1;
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            throwPt = EmDrome_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &throwPt.x);
        }
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                Versus_RouteEmByTeam(em);
            }
        }
        break;
    case 2:
        break;
    }
}

/*
 * Unhittable; DROME_MOT_DAZED once, then idle and DROME_VS_WAIT_BOTH.
 */
void EmDrome_VsPoseA(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_DAZED, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_IDLE, 0, 0);
            EmDrome_ChangeState(em, EM_GROUP_VERSUS, DROME_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * As EmDrome_VsPoseA with DROME_MOT_CALL.
 */
void EmDrome_VsPoseB(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_CALL, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_IDLE, 0, 0);
            EmDrome_ChangeState(em, EM_GROUP_VERSUS, DROME_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * As EmDrome_VsPoseA with DROME_MOT_THREAT.
 */
void EmDrome_VsPoseC(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, DROME_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, DROME_MOT_IDLE, 0, 0);
            EmDrome_ChangeState(em, EM_GROUP_VERSUS, DROME_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * Idle until Versus_BothDefeated, then DROME_VS_END (EmDrome_ChangeState marks the
 * defeat on entry).
 */
void EmDrome_VsWaitBoth(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, DROME_MOT_IDLE)) {
            Em_SetMotion(em, DROME_MOT_IDLE, 0, 0);
        }

    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmDrome_ChangeState(em, EM_GROUP_VERSUS, DROME_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * Idle 90 frames, clears senseDelay and noHitTimer, then EmDrome_EndState.
 */
void EmDrome_VsEnd(Em *em, EmWork *work)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 90;
        if (em->motionId[0] != EM_MOTION_ID(0, DROME_MOT_IDLE)) {
            Em_SetMotion(em, DROME_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmDrome_EndState(em, 0, 0);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_VERSUS state (enum EmDromeVersusState) through
 * its state function.
 */
void EmDrome_RunVersusState(Em *em, EmWork *work)
{
    switch (em->state) {
    case DROME_VS_EAT_BAIT:
        EmDrome_VsEatBait(em, work);
        break;
    case DROME_VS_POSE_A:
        EmDrome_VsPoseA(em, work);
        break;
    case DROME_VS_POSE_B:
        EmDrome_VsPoseB(em, work);
        break;
    case DROME_VS_POSE_C:
        EmDrome_VsPoseC(em, work);
        break;
    case DROME_VS_WAIT_BOTH:
        EmDrome_VsWaitBoth(em, work);
        break;
    case DROME_VS_END:
        EmDrome_VsEnd(em, work);
        break;
    }
}

/*
 * Per frame: visible from EM_GROUP_IDLE to EM_GROUP_REACT, motionSpeed 1.2, combat timer, area
 * trigger, Em_SenseObjects (another raptor call raises trigger 4 out of
 * combat), low-hp check; Em_UpdateDamage picks: KILLED/DEAD a death by the
 * hit side (DROME_DEFEAT_DIE_*), STATUS_IN_AIR a knock-back (unless one
 * runs), STATUS4 DROME_REACT_STATUS4, CAPTURED DROME_EVENT_CAPTURED, STATUS2
 * DROME_IDLE_STATUS2, HIT_LYING the status end, PART_FLINCH and HIT by kind (see
 * the kinds NOTE), STATUS5 DROME_REACT_STUN, ATTACK13 DROME_REACT_HOLD2; then the Jungle
 * 7 scene or the pending script, and EmDrome_RunState (twice after a state request).
 */
void EmDrome_Update(Em *em)
{
    u8 dmgOut[4];
    EmDromeWork *w = (EmDromeWork *)&em->work;
    s32 heardCall;

    if (em->stateGroup <= EM_GROUP_REACT) {
        em->alpha = 1.0f;
        em->visible = 1;
    }
    em->motionSpeed = 1.2f;
    if (em->senseDelay != 0) {
        em->noHitTimer = 5;
    }
    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTrigger(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    heardCall = Em_SenseObjects(em, &w->callObj) & 0xFF;
    if (em->senseDelay == 0 && em->combat == 0 && heardCall != 0 && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->triggers |= 4;
    }
    if (em->noAlert != 1 && em->combat == 1) {
        Em_CheckLowHp(em, 0.2f);
    }
    switch (Em_UpdateDamage(em, dmgOut) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD: {
        u16 hitAngle = em->hitAngle;
        s32 hitRel = (hitAngle - em->rotY) & 0xFFFF;
        if (hitRel >= 24576 && hitRel < 0xA000) {
            em->rotY = hitAngle + ANGLE_TURN / 2;
            EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_DIE_BACK, EM_SYNC_OWNER);
        } else if (hitRel >= 8192 && hitRel < 24576) {
            em->rotY = hitAngle - ANGLE_TURN / 4;
            EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_DIE_SIDE_A, EM_SYNC_OWNER);
        } else if (hitRel >= 0xA000 && hitRel < 0xE000) {
            em->rotY = hitAngle + ANGLE_TURN / 4;
            EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_DIE_SIDE_B, EM_SYNC_OWNER);
        } else {
            em->rotY = hitAngle & 0xFFFF;
            EmDrome_ChangeState(em, EM_GROUP_DEFEAT, DROME_DEFEAT_DIE_FORWARD, EM_SYNC_OWNER);
        }
        break;
    }
    case 5:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_BACK) && (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_FORWARD) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_SIDE_A) && (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_SIDE_B)) {
            u16 hitAngle = em->hitAngle;
            s32 hitRel = (hitAngle - em->rotY) & 0xFFFF;
            if (hitRel >= 24576 && hitRel < 0xA000) {
                em->rotY = hitAngle + ANGLE_TURN / 2;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
            } else if (hitRel >= 8192 && hitRel < 24576) {
                em->rotY = hitAngle - ANGLE_TURN / 4;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_SIDE_A, EM_SYNC_OWNER);
            } else if (hitRel >= 0xA000 && hitRel < 0xE000) {
                em->rotY = hitAngle + ANGLE_TURN / 4;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = hitAngle & 0xFFFF;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_FORWARD, EM_SYNC_OWNER);
            }
        }
        break;
    case 6:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_BACK) && (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_FORWARD) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_SIDE_A) && (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_SIDE_B)) {
            Em_RestartScript(em);
            Em_SetStatus4Timer(em);
            EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case 7:
        EmDrome_ChangeState(em, EM_GROUP_EVENT, DROME_EVENT_CAPTURED, EM_SYNC_OWNER);
        break;
    case 8:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_BACK) && (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_FORWARD) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_SIDE_A) && (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_SIDE_B)) {
            Em_SetStatus2Timer(em);
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS2, EM_SYNC_OWNER);
        }
        break;
    case 10:
        switch (em->state) {
        case 13:
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS1_END, EM_SYNC_OWNER);
            break;
        default:
            Em_RestartScript(em);
            EmDrome_ChangeState(em, EM_GROUP_IDLE, DROME_IDLE_STATUS2_END, EM_SYNC_OWNER);
            break;
        }
        break;
    case 12:
        if (em->type == EM_KIND_VELOCIDROME) {
            u16 hitAngle = em->hitAngle;
            s32 hitRel = (hitAngle - em->rotY) & 0xFFFF;
            if (hitRel >= 24576 && hitRel < 0xA000) {
                em->rotY = hitAngle + ANGLE_TURN / 2;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
            } else if (hitRel >= 8192 && hitRel < 24576) {
                em->rotY = hitAngle - ANGLE_TURN / 4;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_SIDE_A, EM_SYNC_OWNER);
            } else if (hitRel >= 0xA000 && hitRel < 0xE000) {
                em->rotY = hitAngle + ANGLE_TURN / 4;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = hitAngle & 0xFFFF;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_FORWARD, EM_SYNC_OWNER);
            }
        } else if (em->stance == EM_STANCE_AIR) {
            u16 hitAngle = em->hitAngle;
            s32 hitRel = (hitAngle - em->rotY) & 0xFFFF;
            if (hitRel >= 24576 && hitRel < 0xA000) {
                em->rotY = hitAngle + ANGLE_TURN / 2;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
            } else if (hitRel >= 8192 && hitRel < 24576) {
                em->rotY = hitAngle - ANGLE_TURN / 4;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_SIDE_A, EM_SYNC_OWNER);
            } else if (hitRel >= 0xA000 && hitRel < 0xE000) {
                em->rotY = hitAngle + ANGLE_TURN / 4;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_SIDE_B, EM_SYNC_OWNER);
            } else {
                em->rotY = hitAngle & 0xFFFF;
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_FORWARD, EM_SYNC_OWNER);
            }
        } else {
            EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_FLINCH, EM_SYNC_OWNER);
        }
        break;
    case 14:
        if (em->type == EM_KIND_VELOCIDROME) {
            em->scriptPending = 0;
            if (em->stance == EM_STANCE_AIR) {
                if (em->hitAngle >= (ANGLE_TURN * 3 / 8) && em->hitAngle < (ANGLE_TURN * 5 / 8)) {
                    em->rotY = em->hitAngle + ANGLE_TURN / 2;
                    EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_BACK, EM_SYNC_OWNER);
                } else if (em->hitAngle >= (ANGLE_TURN / 8) && em->hitAngle < (ANGLE_TURN * 3 / 8)) {
                    em->rotY = em->hitAngle - ANGLE_TURN / 4;
                    EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_SIDE_A, EM_SYNC_OWNER);
                } else if (em->hitAngle >= (ANGLE_TURN * 5 / 8) && em->hitAngle < (ANGLE_TURN * 7 / 8)) {
                    em->rotY = em->hitAngle + ANGLE_TURN / 4;
                    EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_SIDE_B, EM_SYNC_OWNER);
                } else {
                    em->rotY = em->hitAngle & 0xFFFF;
                    EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_KNOCKED_FORWARD, EM_SYNC_OWNER);
                }
            } else {
                EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_FLINCH, EM_SYNC_OWNER);
            }
        }
        break;
    case 17:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_BACK) && (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_FORWARD) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_SIDE_A) && (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_KNOCKED_SIDE_B) &&
            (em->stateGroup != EM_GROUP_REACT || em->state != DROME_REACT_STUN)) {
            Em_SetStatus5Timer(em);
            EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    case 18:
        Em_ResetKindTimer(em);
        EmDrome_ChangeState(em, EM_GROUP_REACT, DROME_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    }
    switch (gQuest.stage) {
    case QUEST_ID_VELOCIDROME_SCENE_1:
    case QUEST_ID_VELOCIDROME_SCENE_2:
    case QUEST_ID_VELOCIDROME_SCENE_3:
        if (em->type == EM_KIND_VELOCIDROME && Area_DayId(em->area) == AREA_JUNGLE_7 && Save_TestEventFlag(EVENT_FLAG_VELOCIDROME_SCENE) == 0) {
            if (Area_DayId(gScene.area) == AREA_JUNGLE_7 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmHermitaur_ChangeState(em, 6, 2, EM_SYNC_SEND);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmDrome_RunState(em, (s32) w);
    if (em->stateRequest != 0) {
        EmDrome_RunState(em, (s32) w);
        em->stateRequest = 0;
    }
}

/*
 * Keeps prevStateGroup/prevState and runs the group dispatcher.
 */
void EmDrome_RunState(Em *em, s32 work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmDrome_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmDrome_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmDrome_RunHopState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmDrome_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmDrome_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmDrome_RunDefeatState(em, work);
        break;
    case EM_GROUP_EVENT:
        EmDrome_RunEventState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT2:
        EmDrome_RunEvent2State(em, (EmWork *) work);
        break;
    case EM_GROUP_VERSUS:
        EmDrome_RunVersusState(em, (EmWork *) work);

    default:
        break;
    }
}

/*
 * Empty; the default case of EmDrome_MotionEffects.
 */
void EmDrome_MotionEffectsDefault(Em *em)
{
}

/*
 * Keeps motionId[0] in prevMotion, plays the motion's sounds
 * (EmDrome_MotionSounds), then by motion: the dust at DROME_BONE_HOP_DUST in
 * the hops (frames 18 and 20) and the moves aside (12 and 14); in the
 * pounce the dust at DROME_BONE_POUNCE_DUST (frame 22) and DromeAttack row 2
 * (frame 32); the lying puffs at DROME_BONE_MOUTH during DROME_MOT_LIE.
 */
void EmDrome_MotionEffects(Em *em, EmWork *work)
{
    f32 puffOfs[3];

    if (em->motionId[0] != work->prevMotion) {
        work->prevMotion = em->motionId[0];
    }
    EmDrome_MotionSounds(em, work);
    switch (work->prevMotion) {
    case EM_MOTION_ID(0, DROME_MOT_HOP):
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_HOP_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, DROME_MOT_HOP_UP):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_HOP_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, DROME_MOT_MOVE_ACTION_A):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_HOP_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, DROME_MOT_MOVE_ACTION_B):
    case EM_MOTION_ID(0, DROME_MOT_MOVE_ACTION_C):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_HOP_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, DROME_MOT_POUNCE):
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            DromeAttack_Create(em, 2);
        }
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
            PartFx_SpawnForEm(em, DROME_BONE_POUNCE_DUST, 0);
        }
        break;
    case EM_MOTION_ID(0, DROME_MOT_LIE):
        puffOfs[0] = 0.0f;
        puffOfs[1] = 10.0f;
        puffOfs[2] = 40.0f;
        Em_SpawnLyingPuffs(em, DROME_BONE_MOUTH, puffOfs, 1.0f);
        break;
    default:
        EmDrome_MotionEffectsDefault(em);
        break;
    case EM_MOTION_ID(0, DROME_MOT_IDLE):
    case EM_MOTION_ID(0, DROME_MOT_WALK):
    case EM_MOTION_ID(0, DROME_MOT_HOP_LAND):
    case EM_MOTION_ID(0, DROME_MOT_TURN_LEFT):
    case EM_MOTION_ID(0, DROME_MOT_TURN_RIGHT):
    case EM_MOTION_ID(0, DROME_MOT_ALERT):
    case EM_MOTION_ID(0, DROME_MOT_IDLE_B):
    case EM_MOTION_ID(0, DROME_MOT_LEAP):
    case EM_MOTION_ID(0, DROME_MOT_LEAP_LAND):
    case EM_MOTION_ID(0, DROME_MOT_STRIKE):
    case EM_MOTION_ID(0, DROME_MOT_POUNCE_LAND):
    case EM_MOTION_ID(0, DROME_MOT_QUICK_STRIKE):
    case EM_MOTION_ID(0, DROME_MOT_SPIT):
    case EM_MOTION_ID(0, DROME_MOT_FLINCH):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_BACK):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_BACK_LAND):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_FORWARD):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_FORWARD_LAND):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_SIDE_B):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_SIDE_B_LAND):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_SIDE_A):
    case EM_MOTION_ID(0, DROME_MOT_KNOCKED_SIDE_A_LAND):
    case EM_MOTION_ID(0, DROME_MOT_GET_UP):
    case EM_MOTION_ID(0, DROME_MOT_DAZED):
    case EM_MOTION_ID(0, DROME_MOT_COLLAPSE):
    case EM_MOTION_ID(0, DROME_MOT_DIE):
    case EM_MOTION_ID(0, DROME_MOT_STATUS4):
    case EM_MOTION_ID(0, DROME_MOT_THREAT):
    case EM_MOTION_ID(0, DROME_MOT_CALL):
    case EM_MOTION_ID(0, DROME_MOT_SHAKE_OFF):
    case EM_MOTION_ID(0, DROME_MOT_VS_BAIT):
        break;
    }
}

/*
 * Skips the first frame (fxStep 0 then 1), then EmDrome_MotionEffects each
 * frame; EmDrome_Callbacks slot 3.
 */
void EmDrome_UpdateMotionEffects(Em *em)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmDrome_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Empty; EmDrome_Callbacks slot 2.
 */
void EmDrome_TableNop(void)
{
}

/*
 * DROME_IDLE becomes DROME_IDLE_ALERT in combat with hp at least 30%;
 * DROME_IDLE_WAIT becomes ALERT (owner only, offline or mode nonzero) in
 * combat when the goal is a player outside nearRadius, on an even stateRand;
 * begins/ends the statuses (DROME_IDLE_STATUS2, _STATUS2_END, _STATUS4_END,
 * _STATUS1, _STATUS1_END, _STUN_END), DROME_IDLE_HOLD2_SHAKE_OFF clears
 * holdTimer2, DROME_IDLE_RAGE Em_StartRage, DROME_IDLE_PENDING_RAGE
 * Em_StartPendingRage; ground stance, Em_SetStateSync.
 */
void EmDrome_SetIdleState(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case DROME_IDLE:
        if (em->combat != 0 && em->hp >= (s16) (0.3f * em->hpMax)) {
            state = DROME_IDLE_ALERT;
        }
        break;
    case DROME_IDLE_WAIT:
        if (em->remote == 0 && (Game_IsOnline() == 0 || (mode & 0xFF) != 0) && em->combat == 1 &&
            em->curGoalKind == 1 && em->curGoalArg == 0 && em->curGoalNo != -1 && em->stateRand % 2 == 0 &&
            !(((f32 *) em->playerDist)[em->curGoalNo] <= em->nearRadius)) {
            state = DROME_IDLE_ALERT;
        }
        break;
    case DROME_IDLE_STATUS2:
        Em_BeginStatus2(em);
        break;
    case DROME_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case DROME_IDLE_STATUS4_END:
        Em_EndStatus4(em);
        break;
    case DROME_IDLE_STATUS1:
        Em_BeginStatus1(em);
        break;
    case DROME_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case DROME_IDLE_HOLD2_SHAKE_OFF:
        em->holdTimer2 = 0;
        break;
    case DROME_IDLE_STUN_END:
        Em_EndStatus5(em);
        break;
    case DROME_IDLE_RAGE:
        Em_StartRage(em);
        break;
    case DROME_IDLE_PENDING_RAGE:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Ground stance; DROME_MOVE_TURN becomes DROME_MOVE_TURN_ALERT in combat;
 * DROME_MOVE_WALK sets the speed (Em_SetSpeedScaled 1000/300);
 * Em_SetStateSync.
 */
void EmDrome_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->stance = EM_STANCE_GROUND;
    switch (state) {
    case DROME_MOVE_TURN:
        if (em->combat == 1) {
            state = DROME_MOVE_TURN_ALERT;
        }
        break;
    case DROME_MOVE_WALK:
        Em_SetSpeedScaled(em, 2, 1000.0f, 300.0f);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Em_SetStateSync(em, 2, state, mode).
 */
void EmDrome_SetHopState(Em *em, u16 state, u8 mode)
{
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Sets an attack state: for every attack but DROME_ATK_SPIT_FAST (which
 * keeps the old hasGoal), hasGoal is 1 when the goal is a player
 * (curGoalKind 1) and there is a target; then Em_SetStateSync.
 */
void EmDrome_SetAttackState(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case DROME_ATK_STRIKE_A:
    case DROME_ATK_LEAP_MEDIUM_A:
    case DROME_ATK_LEAP_LONG_A:
    case DROME_ATK_LEAP_HIGH_A:
    case DROME_ATK_SPIT:
    case DROME_ATK_STRIKE_B:
    case DROME_ATK_LEAP_MEDIUM_B:
    case DROME_ATK_QUICK_STRIKE_B:
    case DROME_ATK_LEAP_LONG_B:
    case DROME_ATK_LEAP_HIGH_B:
    case DROME_ATK_LEAP_TO_GOAL:
    case DROME_ATK_STRIKE_C:
    case DROME_ATK_QUICK_STRIKE_A:
    case DROME_ATK_POUNCE:
        em->hasGoal = 0;
        if ((em->curGoalKind == 1) && (em->targetNo != -1)) {
            em->hasGoal = 1;
        }
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Changes the monster's state. It removes an active pack call from the
 * sense list; the owner targets a player in range, while a remote copy
 * ignores EM_SYNC_OWNER and EM_SYNC_OWNER_FLAGGED changes. EM_GROUP_IDLE to
 * EM_GROUP_ATTACK go through their setters; EM_GROUP_DEFEAT calls
 * QuestEm_Appear for DROME_DEFEAT_REAPPEAR; EM_GROUP_REACT begins status 4
 * (DROME_REACT_STATUS4), ends all statuses (DROME_REACT_HOLD2) or begins
 * status 5 (DROME_REACT_STUN); EM_GROUP_VERSUS marks the defeat for
 * DROME_VS_WAIT_BOTH and sets motionSpeed 1.0; then Em_SetStateSync.
 */
void EmDrome_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;

    if ((u8)w->countdown != 0) {
        EmSenseObjs_Remove((EmObj *)((&w->callObj)));
        w->countdown = 0U;
    }
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmDrome_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmDrome_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmDrome_SetHopState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmDrome_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_DEFEAT:
        if ((state == 4) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_REACT:
        switch (state) {
        case DROME_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case DROME_REACT_HOLD2:
            Em_EndAllStatus(em);
            break;
        case DROME_REACT_STUN:
            Em_BeginStatus5(em);
            break;
        }

    case EM_GROUP_EVENT:
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_EVENT2:
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_VERSUS:
        if (state == 152) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);

    default:
        return;
    }
}

/*
 * Clears the curve flags, keeps the kind, starts the vertical curve (and the
 * speed curve for DROME_CURVE_UNUSED and DROME_CURVE_HOP_FORWARD) at frame
 * 22, clears the velocity.
 */
void EmDrome_StartCurves(Em *em, u8 kind)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case DROME_CURVE_UNUSED:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 22;
        break;
    case DROME_CURVE_POUNCE:
        w->curveVelY = 1;
        w->curveFrame = 22;
        break;
    case DROME_CURVE_HOP_FORWARD:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 22;
        break;
    case DROME_CURVE_HOP_UP:
        w->curveVelY = 1;
        w->curveFrame = 22;
        break;
    }
    Player_ClearVelocity(em);
}

/*
 * Returns 0 (its flag is never set).
 */
static s32 EmDrome_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

extern EmCurveKey *EmDrome_VelYCurves[];

/*
 * Sets velY from EmDrome_VelYCurves[curveKind] at curveFrame (the first key
 * value at frames 0 and 1, else the slope between keys over motion[0].step);
 * past the table end velY falls by 1 a frame to -50. Returns 2 at the end, 0
 * otherwise or without a table.
 */
static u8 EmDrome_CurveVelY(Em *em, EmWork *work)
{
    EmDromeWork *w = (EmDromeWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *curve;
    u8 ended;

    curve = EmDrome_VelYCurves[((EmDromeWork *)work)->curveKind];
    ended = 0;
    if (curve == NULL) {
        return ended;
    }
    frame = w->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velY = curve->v;
    } else {
        i = 1;
        while (i != 0) {
            key = &curve[i];
            keyT = key->t;
            if (frame > keyT && keyT != 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                ended = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ended;
}

extern EmCurveKey *EmDrome_SpeedCurves[];

/*
 * As EmDrome_CurveVelY for velZ from EmDrome_SpeedCurves[curveKind]; velZ 0
 * while a wall is ahead (wallHitDirs 0xF000000F; the old comment names it
 * unk698). Returns 4 at the table end, else 0.
 */
static u8 EmDrome_CurveSpeed(Em *em, EmWork *work)
{
    EmDromeWork *w = (EmDromeWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *curve;
    u8 ended;

    curve = EmDrome_SpeedCurves[((EmDromeWork *)work)->curveKind];
    ended = 0;
    if (curve == NULL) {
        return ended;
    }
    frame = w->curveFrame;
    if (frame == 0.0f || frame == 1.0f) {
        em->velZ = curve->v;
    } else {
        i = 1;
        while (i != 0) {
            key = &curve[i];
            keyT = key->t;
            if (frame > keyT && keyT != 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                ended = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    if (em->wallHitDirs & 0xF000000F) {
        *(s32 *) &em->velZ = 0;
    }
    return ended;
}

/*
 * Runs the flagged curves, advances curveFrame by motion[0].step, and in the
 * air moves by Em_MoveRotatedAccel along rotY. Returns the curves' end bits
 * ORed (nonzero: a curve ended).
 */
u8 EmDrome_UpdateCurves(Em *em)
{
    EmDromeWork *w = (EmDromeWork *)&em->work;
    u8 ended = 0;

    if (w->curveNone != 0) {
        ended = EmDrome_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ended |= EmDrome_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ended |= EmDrome_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->accelAngX = 0;
        w->accelAngY = em->rotY;
        w->accelAngZ = 0;
        Em_MoveRotatedAccel(em, &w->accelAngX);
    }
    return ended;
}

/* The file's initialised data, in the game's order */
EmDromeAppearPoint EmDrome_AppearPtForestHills10 = {
#include "game/008C3E50.inc"
};
EmDromeAppearPoint EmDrome_AppearPtsForestHills2[2] = {
#include "game/008C3E70.inc"
};
EmDromeAppearPoint EmDrome_AppearPtsForestHills1[4] = {
#include "game/008C3EA0.inc"
};
EmTurnParams EmDrome_TurnParams[2] = {
#include "game/008C3F00.inc"
};
Vec3f EmDrome_CarveThrowPt = {
#include "game/008C3F38.inc"
};
void *EmDrome_Callbacks[8] = {
    EmDrome_SpawnShadow, EmDrome_SpawnShadow, EmDrome_TableNop, EmDrome_UpdateMotionEffects,
    GmEm_ReturnZero, EmDrome_Reinit, NULL, NULL,
};
EmCurveKey EmDrome_VelYCurves_0[10] = {
#include "game/008C3F70.inc"
};
EmCurveKey EmDrome_VelYCurves_1[12] = {
#include "game/008C3FC0.inc"
};
EmCurveKey EmDrome_VelYCurves_2[8] = {
#include "game/008C4020.inc"
};
EmCurveKey EmDrome_VelYCurves_3[8] = {
#include "game/008C4060.inc"
};
EmCurveKey *EmDrome_VelYCurves[4] = {
    EmDrome_VelYCurves_0, EmDrome_VelYCurves_1, EmDrome_VelYCurves_2, EmDrome_VelYCurves_3,
};
EmCurveKey EmDrome_SpeedCurves_0[10] = {
#include "game/008C40B0.inc"
};
EmCurveKey EmDrome_SpeedCurves_2[6] = {
#include "game/008C4100.inc"
};
EmCurveKey *EmDrome_SpeedCurves[4] = {
    EmDrome_SpeedCurves_0, NULL, EmDrome_SpeedCurves_2, NULL,
};
