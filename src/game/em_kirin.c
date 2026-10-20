/*
 * Monster family: the Kirin (EM_KIND_KIRIN). From the family's first
 * function, EmKirin_MotionSounds (its motions' sounds), to its motion
 * effects: Init, Update, the states of its groups and their dispatchers,
 * EmKirin_ChangeState and the effects made with its motions. em_kirin.h
 * holds its motions and states. The Kirin fights on the ground, with
 * charges, strikes and the lightning it calls down (EmRangedAttack's
 * EMRANGED_KIRIN_BOLTS). The state functions take the monster and the work
 * area its state dispatcher passes on (EmWork); Em.step is the step within
 * the current state.
 */
#include "common.h"
#include "stageobj.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_kirin.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
/* prototypes of callees */
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmKirin_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_EndStatus5(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
u16 Math_AngleXZ(f32 *from, f32 *to);
ObjAttach *EmContactAttack_Create(Em *em, s8 kind);
void EmRangedAttack_CreateAtBone(Em *, s32, s32, s32, s32, s32);
void Em_MoveRotatedAccel(Em *, void *);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmDrome_ChangeState, EmDrome_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_RestartScript(Em *em);
void EmKirin_Hunted(Em *, EmWork *);
void EmKirin_SnowyMountainsScene(Em *);
void EmKirin_EndState(Em *em);
void EmKirin_MotionEffects(Em *, EmWork *);
void ThunderSpark_CreateTimed(Em *em, u8 k, s32 n, f32 f);
void EmBodyFx_Spawn(Em *em, s16 kind);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
void Player_ClearVelocity(void *);
void Shadow_Spawn(void *obj, s16 type);
void Em_InitParts(Em *em);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
u16 Area_DayId(u16 id);
void EmKirin_KnockBack(Em *em, EmWork *work, s16 variant);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_BeginStatus2(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_TargetPlayerInRange(Em *);
void Em_BeginStatus4(Em *em);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
f32 GmEm_ScaleBySize(Em *em, f32 v);
s32 Em_GetFloorY(Em *em, f32 *pos, f32 *out);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
s32 Save_TestEventFlag(s32 flag);
void GmEm_ResetSize(Em *em);
s32 Em_UpdateCombatTimer(Em *em);
void Em_RaisePlayerAreaTrigger(Em *em);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmKirin_RunState(Em *em);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
/* end of prototypes */

/*
 * Plays the sounds of the motion in work->fxMotion (EM_MOTION_ID(0,
 * KIRIN_MOT_*)) at its frames: voice sounds at bone 0x17 (two motions pick
 * one at random with Sound_RandomSe), footsteps at bone 0. Called first by
 * EmKirin_MotionEffects.
 */
void EmKirin_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, 0xD, 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x51, 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x4A, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x10, 4, 0x11, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x54, Sound_RandomSe(0x11, 4, 0x12, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, Sound_RandomSe(0x10, 4, 0x12, 4), 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_E):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xE, 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_GALLOP):
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x13, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STRIDE_A):
    case EM_MOTION_ID(0, KIRIN_MOT_STRIDE_B):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x13, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STOP):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_TURN_AROUND_LEFT):
    case EM_MOTION_ID(0, KIRIN_MOT_TURN_AROUND_RIGHT):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x13, 0);
        break;
    case EM_MOTION_ID(0, 0x10):
        Em_PlaySeAtBoneOnFrame(em, 4, 7, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 8, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0xA, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_LUNGE):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 2, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_HOP):
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x13, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_JUMP):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x13, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x49, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_THREAT):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_CALL_LIGHTNING):
        Em_PlaySeAtBoneOnFrame(em, 2, 0x51, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_FLINCH_A):
    case EM_MOTION_ID(0, KIRIN_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4B, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_KNOCKBACK):
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4B, 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_KNOCKBACK_LAND):
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x4A, 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STATUS2_LIE):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x59, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xB8, 0x59, 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 0x55, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0x55, 0x17);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STATUS4_SHAKE_OFF):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x56, 4, 0x57, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        
        break;
    }
}

/*
 * With no quest (gQuest.stage 0) places the monster by area: a grid by
 * slotNo in four named areas, else a diagonal row by slotNo. Clears blind,
 * stance and gender, sets unk_InitFlag, hp and hpMax from
 * Em_ComputeMaxHp(em, 0), motionSpeed 1, motion row KIRIN_MOT_IDLE, the
 * parts, then KIRIN_IDLE (EM_SYNC_LOCAL).
 */
void EmKirin_Init(Em *em)
{
    f32 slotXf;
    f32 slotZf;
    s16 maxHp;
    u16 area;
    u8 slotNo;
    u8 slotNo2;

    if (gQuest.stage == 0) {
        area = gScene.area;
        switch (area) {
        case AREA_SNOWY_MOUNTAINS_NIGHT_9:
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case AREA_SWAMP_NIGHT_0:
            em->posX = 6000.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 7000.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case AREA_JUNGLE_NIGHT_6:
            em->posX = 11000.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 10000.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        case AREA_FOREST_HILLS_4:
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 10000.0f + (150.0f * (f32) (em->slotNo & 7));
            break;
        default:
            slotNo = em->slotNo;
            slotXf = (f32) (u32) slotNo;
            em->posX = 5000.0f - (80.0f * slotXf);
            em->posY = 0.0f;
            slotNo2 = em->slotNo;
            slotZf = (f32) (u32) slotNo2;
            em->posZ = 5000.0f - (80.0f * slotZf);
            break;
        }
    }
    em->blind = 0;
    em->stance = EM_STANCE_GROUND;
    em->unk_InitFlag = 1;
    em->gender = 0;
    maxHp = Em_ComputeMaxHp(em, 0);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->motionSpeed = 1.0f;
    Em_SetMotionRow(em, KIRIN_MOT_IDLE, 0, 0);
    Em_InitParts(em);
    EmKirin_ChangeState(em, EM_GROUP_IDLE, KIRIN_IDLE, EM_SYNC_LOCAL);
}

/*
 * Per frame: combat timer, area trigger, hold in place with no hunters
 * (except in KIRIN_IDLE). By Em_UpdateDamage: killed/dead faces away from
 * the hit and goes KIRIN_DEFEAT_HUNTED; status 4 -> KIRIN_REACT_STATUS4;
 * status 2 -> KIRIN_IDLE_STATUS2_COLLAPSE; hit while lying ->
 * KIRIN_REACT_WAKE_KNOCKBACK; part flinch / hit mode 3 ->
 * KIRIN_REACT_FLINCH_A/_B when hit from the front on the ground with part 0
 * intact, else KIRIN_REACT_KNOCKBACK; the stun is ended at once
 * (Em_EndStatus5). In its scene's quests (QUEST_ID_KIRIN_SCENE_1, QUEST_ID_KIRIN_SCENE_2), in
 * AREA_SNOWY_MOUNTAINS_8, while event flag 0xAC (its scene seen) is clear, a
 * running scene starts KIRIN_EVENT_SNOWY_MOUNTAINS_SCENE,
 * else the pending script runs; then the state runs (twice on stateRequest).
 */
void EmKirin_Update(Em *em)
{
    u8 dmgOut[3];
    s32 hitAngle;
    s32 hitDelta;

    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTrigger(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    switch (Em_UpdateDamage(em, dmgOut) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_STATUS_IN_AIR:
    case EM_DMG_CAPTURED:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        em->rotY = em->hitAngle + 0x7FFF + 1;
        EmKirin_ChangeState(em, EM_GROUP_DEFEAT, KIRIN_DEFEAT_HUNTED, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS4:
        Em_SetStatus4Timer(em);
        EmKirin_ChangeState(em, EM_GROUP_REACT, KIRIN_REACT_STATUS4, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS2:
        Em_SetStatus2Timer(em);
        EmKirin_ChangeState(em, EM_GROUP_IDLE, KIRIN_IDLE_STATUS2_COLLAPSE, EM_SYNC_OWNER);
        break;
    case EM_DMG_HIT_LYING:
        em->rotY = em->hitAngle + 0x7FFF + 1;
        EmKirin_ChangeState(em, EM_GROUP_REACT, KIRIN_REACT_WAKE_KNOCKBACK, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_HIT_MODE3:
        hitAngle = em->hitAngle;
        hitDelta = hitAngle - em->rotY;
        if (((hitDelta - 0x4000) & 0xFFFF) < 0x8000 && em->parts[0].hp > 0 && em->stance == EM_STANCE_GROUND) {
            if ((hitDelta & 0xFFFF) < 0x8000) {
                EmKirin_ChangeState(em, EM_GROUP_REACT, KIRIN_REACT_FLINCH_A, EM_SYNC_OWNER);
            } else {
                EmKirin_ChangeState(em, EM_GROUP_REACT, KIRIN_REACT_FLINCH_B, EM_SYNC_OWNER);
            }
        } else {
            em->rotY = hitAngle + 0x7FFF + 1;
            EmKirin_ChangeState(em, EM_GROUP_REACT, KIRIN_REACT_KNOCKBACK, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    }
    switch (gQuest.stage) {
    case QUEST_ID_KIRIN_SCENE_1:
    case QUEST_ID_KIRIN_SCENE_2:
        if (Area_DayId(em->area) == AREA_SNOWY_MOUNTAINS_8 && Save_TestEventFlag(0xAC) == 0) {
            if (Area_DayId(gScene.area) == AREA_SNOWY_MOUNTAINS_8 && gScene.eventRunning == 1 && em->stateGroup != EM_GROUP_EVENT) {
                EmKirin_ChangeState(em, EM_GROUP_EVENT, KIRIN_EVENT_SNOWY_MOUNTAINS_SCENE, EM_SYNC_SEND);
            }
            break;
        }
    default:
        Em_UpdatePendingScript(em);
        break;
    }
    Em_ClearHitReactMode(em);
    EmKirin_RunState(em);
    if (em->stateRequest != 0) {
        EmKirin_RunState(em);
        em->stateRequest = 0;
    }
}

/*
 * Empty state (KIRIN_IDLE_NOP).
 */
void EmKirin_IdleNop(void)
{
}

/*
 * KIRIN_MOT_IDLE once, then EmKirin_EndState (which comes back here and runs
 * the script).
 */
void EmKirin_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_IDLE_ACTION_A once, then EmKirin_EndState.
 */
void EmKirin_IdleActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_IDLE_ACTION_B once, then EmKirin_EndState.
 */
void EmKirin_IdleActionB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_IDLE_ACTION_C once, then EmKirin_EndState.
 */
void EmKirin_IdleActionC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_IDLE_ACTION_D once, then EmKirin_EndState.
 */
void EmKirin_IdleActionD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_IDLE_ACTION_E once, then EmKirin_EndState.
 */
void EmKirin_IdleActionE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Sets targetAngle to the goal and plays KIRIN_MOT_TURN_TO_GOAL_LEFT when
 * the goal is up to 0x8000 on the rotY-rising side, else
 * KIRIN_MOT_TURN_TO_GOAL_RIGHT (the motion turns; no rotY change in code);
 * then EmKirin_EndState.
 */
void EmKirin_TurnToGoal(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (((em->targetAngle - em->rotY) & 0xFFFF) < 0x8001) {
            Em_SetMotion(em, KIRIN_MOT_TURN_TO_GOAL_LEFT, 0, 0);
            break;
        }
        Em_SetMotion(em, KIRIN_MOT_TURN_TO_GOAL_RIGHT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Status 2 (EmKirin_ChangeState begins it): hit react EM_HIT_REACT_WAKE and blind 2
 * each frame, KIRIN_MOT_COLLAPSE then KIRIN_MOT_STATUS2_LIE until em->timer
 * (Em_SetStatus2Timer) runs out, then KIRIN_IDLE_STATUS2_GET_UP; a remote
 * copy retries that change until it runs the monster.
 */
void EmKirin_Status2Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STATUS2_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmKirin_ChangeState(em, EM_GROUP_IDLE, KIRIN_IDLE_STATUS2_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmKirin_ChangeState(em, EM_GROUP_IDLE, KIRIN_IDLE_STATUS2_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Status 2 ended by EmKirin_ChangeState: KIRIN_MOT_GET_UP with blind 2, blind
 * cleared at the end, then EmKirin_EndState.
 */
void EmKirin_Status2GetUp(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->blind = 2;
        Em_SetMotion(em, KIRIN_MOT_GET_UP, 0, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Status 4 ended by EmKirin_ChangeState: KIRIN_MOT_STATUS4_SHAKE_OFF, clears noReact
 * at the end, then EmKirin_EndState.
 */
void EmKirin_Status4ShakeOff(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_STATUS4_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_THREAT once, then EmKirin_EndState.
 */
void EmKirin_IdleThreat(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_THREAT, then KIRIN_MOT_IDLE blended in over 8 frames, then
 * EmKirin_EndState.
 */
void EmKirin_IdleThreatThenIdle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_IDLE, 8, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Stores prevStateGroup and prevState, then runs the monster's EM_GROUP_IDLE
 * state (enum EmKirinIdleState) through its state function. EM_GROUP_FLY
 * runs it too: the Kirin doesn't fly.
 */
void EmKirin_RunIdleState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case KIRIN_IDLE_NOP:
        EmKirin_IdleNop();
        break;
    case KIRIN_IDLE:
        EmKirin_Idle(em, work);
        break;
    case KIRIN_IDLE_ACTION_A:
        EmKirin_IdleActionA(em, work);
        break;
    case KIRIN_IDLE_ACTION_B:
        EmKirin_IdleActionB(em, work);
        break;
    case KIRIN_IDLE_ACTION_C:
        EmKirin_IdleActionC(em, work);
        break;
    case KIRIN_IDLE_ACTION_D:
        EmKirin_IdleActionD(em, work);
        break;
    case KIRIN_IDLE_ACTION_E:
        EmKirin_IdleActionE(em, work);
        break;
    case KIRIN_IDLE_TURN_TO_GOAL:
        EmKirin_TurnToGoal(em, work);
        break;
    case KIRIN_IDLE_STATUS2_COLLAPSE:
        EmKirin_Status2Collapse(em, work);
        break;
    case KIRIN_IDLE_STATUS2_GET_UP:
        EmKirin_Status2GetUp(em, work);
        break;
    case KIRIN_IDLE_STATUS4_SHAKE_OFF:
        EmKirin_Status4ShakeOff(em, work);
        break;
    case KIRIN_IDLE_THREAT:
        EmKirin_IdleThreat(em, work);
        break;
    case KIRIN_IDLE_THREAT_THEN_IDLE:
        EmKirin_IdleThreatThenIdle(em, work);
        
    default:
        break;
    }
}

/*
 * KIRIN_MOT_WALK while rotY turns 0x100 a frame toward the goal angle;
 * EmKirin_EndState when within 0x100. Rebuilds rotMtx from the angles each
 * frame.
 */
void EmKirin_WalkTurnToGoal(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Em_SetMotion(em, KIRIN_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100) < 0x100) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * KIRIN_MOT_WALK, Em_StepToGoal (turn step 0x40) until the goal is reached,
 * then EmKirin_EndState.
 */
void EmKirin_WalkToGoal(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_CHARGE with contact attack 0x10, Em_StepToGoal until the goal is
 * reached and the motion ended, then KIRIN_MOT_STOP and EmKirin_EndState.
 * The variant is unused: states 2 and 9 differ only in the stop margin
 * EmKirin_ChangeState sets.
 */
void EmKirin_ChargeToGoal(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_CHARGE, 0, 0);
        EmContactAttack_Create(em, 0x10);
        break;
    case 1:
        if ((Em_StepToGoal(em, 0x40, 0) == 1) && (em->motion[0].playing == 0)) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STOP, 4, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_GALLOP with contact attack 0x10, restarted by hand on layers 0
 * and 1 each time it ends until Em_StepToGoal reaches the goal, then
 * KIRIN_MOT_STOP on both layers and EmKirin_EndState. The variant is unused
 * (states 3 and 0xA differ in EmKirin_ChangeState).
 */
void EmKirin_GallopToGoal(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_GALLOP, 0, 0);
        EmContactAttack_Create(em, 0x10);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            if (em->motion[0].playing == 0) {
                em->step++;
                Em_SetLayerMotion(em, EM_MOTION_ID(0, KIRIN_MOT_STOP), 0, 2, 0);
                Em_SetLayerMotion(em, EM_MOTION_ID(1, KIRIN_MOT_STOP), 0, 2, 1);
                break;
            }
        } else {
            if (em->motion[0].playing == 0) {
                Em_SetLayerMotion(em, EM_MOTION_ID(0, KIRIN_MOT_GALLOP), 0, 0, 0);
                Em_SetLayerMotion(em, EM_MOTION_ID(1, KIRIN_MOT_GALLOP), 0, 0, 1);
            }
            break;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_STRIDE_A, then KIRIN_MOT_STOP from frame 0x1A, then
 * EmKirin_EndState.
 */
void EmKirin_StrideA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_STRIDE_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STOP, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_STRIDE_B, then KIRIN_MOT_STOP from frame 0x1A, then
 * EmKirin_EndState.
 */
void EmKirin_StrideB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_STRIDE_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STOP, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_TURN_AROUND_LEFT; once unblended, rotY + 0x888 a frame to motion
 * frame 30, then + 8 (a half turn, rotY rising), then EmKirin_EndState.
 */
void EmKirin_TurnAroundLeft(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_TURN_AROUND_LEFT, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            em->rotY += 0x888;
            if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
                em->step++;
                em->rotY += 8;
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_TURN_AROUND_RIGHT; the mirror of EmKirin_TurnAroundLeft (rotY - 0x888 a
 * frame, then - 8).
 */
void EmKirin_TurnAroundRight(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_TURN_AROUND_RIGHT, 0, 0);
        break;
    case 1:
        if (em->motion[0].blend == 0) {
            em->rotY -= 0x888;
            if (EmMotion_ReachedFrame(em, 0, 30.0f) != 0) {
                em->step++;
                em->rotY -= 8;
                break;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_HOP once (no turn in code), then EmKirin_EndState.
 */
void EmKirin_Hop(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_HOP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Four strides, KIRIN_MOT_STRIDE_A, _B, _A, _B, each with contact
 * KIRIN_ATK_BOLTS_D_ALT, then EmKirin_EndState.
 */
void EmKirin_StrideRampage(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_STRIDE_A, 0, 0);
        EmContactAttack_Create(em, 0xD);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STRIDE_B, 0, 0);
            EmContactAttack_Create(em, 0xD);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STRIDE_A, 0, 0);
            EmContactAttack_Create(em, 0xD);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STRIDE_B, 0, 0);
            EmContactAttack_Create(em, 0xD);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Ground stance, lookAtTarget off, aims targetAngle at the goal: within
 * 0x3000 either way KIRIN_MOT_HOP (up to a quarter turn over frame 26), else
 * KIRIN_MOT_TURN_AROUND_RIGHT or _LEFT (up to a half turn over frame 30);
 * turns rotY by the step a frame, snapping onto the goal angle, then
 * EmKirin_EndState.
 */
void EmKirin_FaceGoal(Em *em, EmWork *work)
{
    f32 turnEndFrame;
    u32 turnStep;
    u32 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        angleDiff = (em->targetAngle - em->rotY) & 0xFFFF;
        if (angleDiff <= 0x3000U || angleDiff >= 0xD000U) {
            Em_SetMotion(em, KIRIN_MOT_HOP, 0, 0);
        } else if (angleDiff >= 0x8000U) {
            Em_SetMotion(em, KIRIN_MOT_TURN_AROUND_RIGHT, 0, 0);
        } else {
            Em_SetMotion(em, KIRIN_MOT_TURN_AROUND_LEFT, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            break;
        }
        if (em->motionId[0] == EM_MOTION_ID(0, KIRIN_MOT_HOP)) {
            turnEndFrame = 26.0f;
            turnStep = 0x4000U / (u32) (turnEndFrame / 2.0f);
        } else {
            turnEndFrame = 30.0f;
            turnStep = 0x8000U / (u32) (turnEndFrame / 2.0f);
        }
        angleDiff = (em->targetAngle - (em->rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((angleDiff + turnStep) & 0xFFFF) < turnStep * 2) {
            em->rotY = em->targetAngle;
        } else if (angleDiff < 0x8000U) {
            em->rotY = (em->rotY + turnStep) & 0xFFFF;
        } else {
            em->rotY = (em->rotY - turnStep) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, turnEndFrame) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_JUMP: below the goal a ballistic arc (velZ 100, apex 1000 above
 * the goal height) timed to the XZ distance, else a short jump (velY 75,
 * velZ 50); airborne (EM_STANCE_AIR) moving by Em_MoveRotatedAccel once the
 * motion ends, frozen from frame 36; on the floor ground stance and
 * KIRIN_MOT_STOP (frame 0xA), then EmKirin_EndState.
 */
void EmKirin_JumpToGoal(Em *em, EmWork *work)
{
    f32 dist;
    u8 airborne;
    u8 curStep;

    curStep = em->step;
    airborne = 0;
    switch (curStep) {
    case 0:
        em->step = curStep + 1;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, KIRIN_MOT_JUMP, 0, 0);
        Player_ClearVelocity(em);
        work->accelAngX = 0;
        work->accelAngZ = 0;
        if (!(em->posY < em->goal[1])) {
            em->velY = 75.0f;
            em->accelY = -10.0f;
            em->velZ = 50.0f;
            break;
        }
        dist = Vec_DistanceXZ(&em->posX, em->goal);
        em->velZ = 100.0f;
        em->accelY = -10.0f;
        em->timer = (s32) (dist / em->velZ);
        em->velY = (1000.0f + em->goal[1] - em->posY) / (f32) em->timer - (em->accelY * (f32) em->timer) / 2.0f;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->stance = EM_STANCE_AIR;
            airborne = 1;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
            em->motionSpeed = 0.0f;
        }
        Em_GetFloorY(em, &em->posX, &em->floorY);
        if (airborne && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            em->motionSpeed = 1.0f;
            Em_SetMotion(em, KIRIN_MOT_STOP, 4, 0xA);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step = curStep + 1;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_MOVE state (enum EmKirinMoveState) through its
 * state function.
 */
void EmKirin_RunMoveState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case KIRIN_MOVE_WALK_TURN_TO_GOAL:
        EmKirin_WalkTurnToGoal(em);
        break;
    case KIRIN_MOVE_WALK:
        EmKirin_WalkToGoal(em);
        break;
    case KIRIN_MOVE_CHARGE:
        EmKirin_ChargeToGoal(em, work, 0);
        break;
    case KIRIN_MOVE_GALLOP:
        EmKirin_GallopToGoal(em, work, 0);
        break;
    case KIRIN_MOVE_STRIDE_A:
        EmKirin_StrideA(em);
        break;
    case KIRIN_MOVE_STRIDE_B:
        EmKirin_StrideB(em);
        break;
    case KIRIN_MOVE_TURN_AROUND_LEFT:
        EmKirin_TurnAroundLeft(em);
        break;
    case KIRIN_MOVE_TURN_AROUND_RIGHT:
        EmKirin_TurnAroundRight(em);
        break;
    case KIRIN_MOVE_HOP:
        EmKirin_Hop(em);
        break;
    case KIRIN_MOVE_CHARGE_WIDE:
        EmKirin_ChargeToGoal(em, work, 1);
        break;
    case KIRIN_MOVE_GALLOP_WIDE:
        EmKirin_GallopToGoal(em, work, 1);
        break;
    case KIRIN_MOVE_STRIDE_RAMPAGE:
        EmKirin_StrideRampage(em);
        break;
    case KIRIN_MOVE_FACE_GOAL:
        EmKirin_FaceGoal(em, work);
        break;
    case KIRIN_MOVE_JUMP_TO_GOAL:
        EmKirin_JumpToGoal(em, work);
        break;
    }
}

/*
 * KIRIN_MOT_THREAT once (attack group), then EmKirin_EndState.
 */
void EmKirin_Threat(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_LUNGE (its effects: contact KIRIN_ATK_BOLTS_B_ALT at frame 20),
 * then KIRIN_MOT_STOP from frame 0x1A, then EmKirin_EndState.
 */
void EmKirin_Lunge(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_LUNGE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STOP, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_JUMP in place, then KIRIN_MOT_STRIKE from frame 8 (contact
 * KIRIN_ATK_BOLTS_C_ALT at its frame 14), then EmKirin_EndState.
 */
void EmKirin_JumpStrike(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_JUMP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_STRIKE, 0, 8);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_STRIKE; at frame 22 lightning (EMRANGED_KIRIN_BOLTS) from bone
 * 0x17 with variant 0 (variant 1: 6, the same strike spots with attack row
 * 6); then EmKirin_EndState.
 */
void EmKirin_StrikeBolts(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_STRIKE, 0, 0);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 22.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 0, 0, 0);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 6, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_CALL_LIGHTNING with spark type 8 for 90 frames; at frame 90
 * lightning from bone 0x17 with variant 1 (variant 1: 7); then
 * EmKirin_EndState.
 */
void EmKirin_CallBoltsA(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_CALL_LIGHTNING, 0, 0);
        ThunderSpark_CreateTimed(em, SPARK_KIRIN_CHARGE, 0x5A, 1.0f);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 1, 0, 0);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 7, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * As EmKirin_CallBoltsA with lightning variant 2 (8).
 */
void EmKirin_CallBoltsB(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_CALL_LIGHTNING, 0, 0);
        ThunderSpark_CreateTimed(em, SPARK_KIRIN_CHARGE, 0x5A, 1.0f);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 2, 0, 0);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 8, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * As EmKirin_CallBoltsA with lightning variant 3 (9).
 */
void EmKirin_CallBoltsC(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_CALL_LIGHTNING, 0, 0);
        ThunderSpark_CreateTimed(em, SPARK_KIRIN_CHARGE, 0x5A, 1.0f);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 3, 0, 0);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 9, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * As EmKirin_CallBoltsA with lightning variant 4 (0xA).
 */
void EmKirin_CallBoltsD(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_CALL_LIGHTNING, 0, 0);
        ThunderSpark_CreateTimed(em, SPARK_KIRIN_CHARGE, 0x5A, 1.0f);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 4, 0, 0);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 0xA, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * As EmKirin_CallBoltsA with lightning variant 5 (0xB).
 */
void EmKirin_CallBoltsE(Em *em, EmWork *work, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_CALL_LIGHTNING, 0, 0);
        ThunderSpark_CreateTimed(em, SPARK_KIRIN_CHARGE, 0x5A, 1.0f);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            if (!(variant & 0xFF)) {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 5, 0, 0);
            } else {
                EmRangedAttack_CreateAtBone(em, 0x17, 8, 0xB, 0, 0);
            }
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_ATTACK state (enum EmKirinAttackState) through
 * its state function.
 */
void EmKirin_RunAttackState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case KIRIN_ATK_THREAT:
        EmKirin_Threat(em);
        break;
    case KIRIN_ATK_LUNGE:
        EmKirin_Lunge(em);
        break;
    case KIRIN_ATK_JUMP_STRIKE:
        EmKirin_JumpStrike(em);
        break;
    case KIRIN_ATK_STRIKE_BOLTS:
        EmKirin_StrikeBolts(em, work, 0);
        break;
    case KIRIN_ATK_BOLTS_A:
        EmKirin_CallBoltsA(em, work, 0);
        break;
    case KIRIN_ATK_BOLTS_B:
        EmKirin_CallBoltsB(em, work, 0);
        break;
    case KIRIN_ATK_BOLTS_C:
        EmKirin_CallBoltsC(em, work, 0);
        break;
    case KIRIN_ATK_BOLTS_D:
        EmKirin_CallBoltsD(em, work, 0);
        break;
    case KIRIN_ATK_BOLTS_E:
        EmKirin_CallBoltsE(em, work, 0);
        break;
    case KIRIN_ATK_STRIKE_BOLTS_ALT:
        EmKirin_StrikeBolts(em, work, 1);
        break;
    case KIRIN_ATK_BOLTS_A_ALT:
        EmKirin_CallBoltsA(em, work, 1);
        break;
    case KIRIN_ATK_BOLTS_B_ALT:
        EmKirin_CallBoltsB(em, work, 1);
        break;
    case KIRIN_ATK_BOLTS_C_ALT:
        EmKirin_CallBoltsC(em, work, 1);
        break;
    case KIRIN_ATK_BOLTS_D_ALT:
        EmKirin_CallBoltsD(em, work, 1);
        break;
    case KIRIN_ATK_BOLTS_E_ALT:
        EmKirin_CallBoltsE(em, work, 1);
        break;
    }
}

/*
 * KIRIN_MOT_FLINCH_A, Em_RestartScript, then EmKirin_EndState.
 */
void EmKirin_FlinchA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_FLINCH_A, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_FLINCH_B, Em_RestartScript, then EmKirin_EndState.
 */
void EmKirin_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_FLINCH_B, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * KIRIN_MOT_KNOCKBACK, thrown back airborne (velY 17, velZ -20, accelY
 * -2.38, accelZ 0.11 until velZ stops), Em_RestartScript; variant 1 clears
 * blind. On the floor KIRIN_MOT_KNOCKBACK_LAND for 90 frames,
 * KIRIN_MOT_GET_UP, then EmKirin_EndState.
 */
void EmKirin_KnockBack(Em *em, EmWork *work, s16 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_KNOCKBACK, 0, 0);
        switch (variant) {
        case 1:
            em->blind = 0;
            break;
        }
        Player_ClearVelocity(em);
        em->velX = 0.0f;
        em->velY = 17.0f;
        em->velZ = -20.0f;
        em->accelY = -2.38f;
        em->accelZ = 0.11f;
        em->stance = EM_STANCE_AIR;
        Em_RestartScript(em);
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        work->accelAngY = em->rotY;
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KIRIN_MOT_KNOCKBACK_LAND, 0, 0);
            em->timer = 0x5A;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Status 4 (EmKirin_ChangeState begins it): hit react EM_HIT_REACT_STATUS,
 * KIRIN_MOT_STATUS4, noReact 1, Em_RestartScript, the status 4 effect at
 * bone 2 until em->timer (Em_SetStatus4Timer) runs out, then
 * KIRIN_IDLE_STATUS4_SHAKE_OFF (retried until the monster is local).
 */
void EmKirin_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_STATUS4, 0, 0);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmKirin_ChangeState(em, EM_GROUP_IDLE, KIRIN_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmKirin_ChangeState(em, EM_GROUP_IDLE, KIRIN_IDLE_STATUS4_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * KIRIN_MOT_COLLAPSE, Em_RestartScript, then KIRIN_MOT_GET_UP, then
 * EmKirin_EndState. Nothing in this file sets it.
 */
void EmKirin_CollapseAndRise(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_COLLAPSE, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_REACT state (enum EmKirinReactState) through
 * its state function.
 */
void EmKirin_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KIRIN_REACT_FLINCH_A:
        EmKirin_FlinchA(em);
        break;
    case KIRIN_REACT_FLINCH_B:
        EmKirin_FlinchB(em);
        break;
    case KIRIN_REACT_KNOCKBACK:
        EmKirin_KnockBack(em, work, 0);
        break;
    case KIRIN_REACT_STATUS4:
        EmKirin_Status4(em);
        break;
    case KIRIN_REACT_COLLAPSE_AND_RISE:
        EmKirin_CollapseAndRise(em);
        break;
    case KIRIN_REACT_WAKE_KNOCKBACK:
        EmKirin_KnockBack(em, work, 1);
        break;
    }
}

/*
 * Unhittable and unpushable (10 frames, set each frame), out of combat;
 * QuestEm_Hunted, KIRIN_MOT_KNOCKBACK thrown back (velY 17, velZ -23), lands
 * with KIRIN_MOT_KNOCKBACK_LAND, then stops the motion (motionSpeed 0), adds
 * carcass point 0 (EmMine_Create), clears fieldCollide and runs
 * EmMine_Update for good; never below the floor.
 */
void EmKirin_Hunted(Em *em, EmWork *work)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, KIRIN_MOT_KNOCKBACK, 0, 0);
        Player_ClearVelocity(em);
        em->velX = 0.0f;
        em->velY = 17.0f;
        em->velZ = -23.0f;
        em->accelY = -3.15f;
        em->accelZ = 0.08f;
        em->stance = EM_STANCE_AIR;
        QuestEm_Hunted(em);
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        work->accelAngY = em->rotY;
        Em_MoveRotatedAccel(em, &work->accelAngX);
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (em->posY <= em->floorY) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, KIRIN_MOT_KNOCKBACK_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 90;
            em->motionSpeed = 0.0f;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        EmMine_Update(em);
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * Runs the monster's EM_GROUP_DEFEAT state (enum EmKirinDefeatState) through
 * its state function.
 */
void EmKirin_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case KIRIN_DEFEAT_HUNTED:
        EmKirin_Hunted(em, work);
        break;
    }
}

/*
 * The scripted scene (senseDelay and noHitTimer 5 each frame): resets size,
 * set at a fixed point facing a goal, idles 140 frames, walks to the goal
 * (600 frames or within 100), KIRIN_IDLE_ACTION_C, KIRIN_IDLE_ACTION_B, HOP; moved to a
 * second fixed point, STOP, walks 30 frames, KIRIN_IDLE_ACTION_D, THREAT for 80
 * frames, CALL_LIGHTNING with bolt variant 4 at frame 90, KIRIN_IDLE_ACTION_B,
 * IDLE; waits for event flag 0xAC, then EmKirin_EndState.
 */
void EmKirin_SnowyMountainsScene(Em *em)
{
    f32 goalDist;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        GmEm_ResetSize(em);
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 12773.0f;
        em->posY = 1200.0f;
        em->posZ = 3971.0f;
        em->goal[0] = 12368.0f;
        em->goal[1] = 1240.0f;
        em->goal[2] = 4606.0f;
        em->rotX = 0;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->rotZ = 0;
        Em_SetMotion(em, KIRIN_MOT_IDLE, 0, 0);
        em->timer = 140;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->posX = 12773.0f;
            em->posY = 1200.0f;
            em->posZ = 3971.0f;
            Em_SetMotion(em, KIRIN_MOT_WALK, 0, 0);
            em->timer = 600;
        }
        break;
    case 2:
        goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || goalDist <= 100.0f) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_C, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_B, 0, 0);
            em->timer = 0;
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_HOP, 0, 0);
        }
        break;
    case 5:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->posX = 9964.0f;
            em->posY = 0.0f;
            em->posZ = 9481.0f;
            em->goal[0] = 9882.0f;
            em->goal[1] = 0.0f;
            em->goal[2] = 9808.0f;
            em->rotX = 0;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->rotZ = 0;
            Em_SetMotion(em, KIRIN_MOT_STOP, 0, 0);
        }
        break;
    case 6:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_WALK, 0, 0);
            em->timer = 30;
        }
        break;
    case 7:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_D, 0, 0);
        }
        break;
    case 8:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_THREAT, 0, 0);
            em->timer = 80;
        }
        break;
    case 9:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_CALL_LIGHTNING, 0, 0);
            ThunderSpark_CreateTimed(em, SPARK_KIRIN_CHARGE, 0x5A, 1.0f);
        }
        break;
    case 10:
        if (EmMotion_PassedFrameScaled(em, 90.0f, 0U) != 0) {
            EmRangedAttack_CreateAtBone(em, 0x17, 8, 4, 0, 0);
        }
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_IDLE_ACTION_B, 0, 0);
        }
        break;
    case 11:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, KIRIN_MOT_IDLE, 0, 0);
        }
        break;
    case 12:
        if (Save_TestEventFlag(0xAC) == 1) {
            EmKirin_EndState(em);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_EVENT state (enum EmKirinEventState; only
 * KIRIN_EVENT_SNOWY_MOUNTAINS_SCENE). EM_GROUP_EVENT2 runs it too.
 */
void EmKirin_RunEventState(Em *em)
{
    switch (em->state) {
    case KIRIN_EVENT_SNOWY_MOUNTAINS_SCENE:
        EmKirin_SnowyMountainsScene(em);
        break;
    }
}

/*
 * Runs the current group dispatcher with the work area: 0 and 2
 * EmKirin_RunIdleState, 1 EmKirin_RunMoveState, 3 EmKirin_RunAttackState, 4 EmKirin_RunReactState, 5
 * EmKirin_RunDefeatState, 6 and 7 EmKirin_RunEventState.
 */
void EmKirin_RunState(Em *em)
{
    u8 *work = &em->work;

    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmKirin_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_MOVE:
        EmKirin_RunMoveState(em, (EmWork *) work);
        break;
    case EM_GROUP_FLY:
        EmKirin_RunIdleState(em, (EmWork *) work);
        break;
    case EM_GROUP_ATTACK:
        EmKirin_RunAttackState(em, (EmWork *) work);
        break;
    case EM_GROUP_REACT:
        EmKirin_RunReactState(em, (EmWork *) work);
        break;
    case EM_GROUP_DEFEAT:
        EmKirin_RunDefeatState(em, (EmWork *) work);
        break;
    case EM_GROUP_EVENT:
        EmKirin_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmKirin_RunEventState(em);
        
    default:
        break;
    }
}

/*
 * Locally sets the target (Em_TargetPlayerInRange); a remote copy ignores
 * modes 3 and 4. Entry work: KIRIN_IDLE_STATUS2_COLLAPSE begins status 2,
 * KIRIN_IDLE_STATUS2_GET_UP and KIRIN_REACT_WAKE_KNOCKBACK end it,
 * KIRIN_IDLE_STATUS4_SHAKE_OFF ends status 4, KIRIN_REACT_STATUS4 begins it; KIRIN_MOVE_WALK,
 * CHARGE and GALLOP get Em_SetSpeedScaled stop margin 100, CHARGE_WIDE and
 * GALLOP_WIDE 300, GALLOP_WIDE also cuts goalDist by 300 (size-scaled)
 * unless the goal is a player. Then Em_SetStateSync.
 */
void EmKirin_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 stopMargin;
    f32 goalDist;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        switch (state) {
        case KIRIN_IDLE_TURN_TO_GOAL:
            break;
        case KIRIN_IDLE_STATUS2_COLLAPSE:
            Em_BeginStatus2(em);
            break;
        case KIRIN_IDLE_STATUS2_GET_UP:
            Em_EndStatus2(em);
            break;
        case KIRIN_IDLE_STATUS4_SHAKE_OFF:
            Em_EndStatus4(em);
            break;
        }
        break;
    case EM_GROUP_MOVE:
        switch (state) {
        case KIRIN_MOVE_WALK_TURN_TO_GOAL:
            break;
        case KIRIN_MOVE_WALK:
        case KIRIN_MOVE_CHARGE:
        case KIRIN_MOVE_GALLOP:
            stopMargin = 100.0f;
            goto move; /* joins the wide cases at their speed call */
        case KIRIN_MOVE_CHARGE_WIDE:
        case KIRIN_MOVE_GALLOP_WIDE:
            stopMargin = 300.0f;
        move:
            Em_SetSpeedScaled(em, 0, 1000.0f, stopMargin);
            if (state == KIRIN_MOVE_GALLOP_WIDE && em->curGoalKind != 1) { /* curGoalKind 1: a player */
                goalDist = em->goalDist - GmEm_ScaleBySize(em, 300.0f);
                em->goalDist = goalDist;
                if (goalDist <= 0.0f) {
                    em->goalDist = 0.0f;
                }
            }
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        break;
    case EM_GROUP_REACT:
        switch (state) {
        case KIRIN_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case KIRIN_REACT_WAKE_KNOCKBACK:
            Em_EndStatus2(em);
            break;
        }
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends a state: motionSpeed 1, ground stance, lookAtTarget off,
 * scriptPending 1, then KIRIN_IDLE (EM_SYNC_LOCAL).
 */
void EmKirin_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmKirin_ChangeState(em, EM_GROUP_IDLE, KIRIN_IDLE, EM_SYNC_LOCAL);
}

/*
 * Kind callback slots 0 and 1 (EmKirin_Callbacks): EmFxTri kinds 0 and 1 and
 * effect 01 type 4 on the monster.
 */
void EmKirin_SpawnEffects(Em *em)
{
    EmBodyFx_Spawn(em, EMBODYFX_KIRIN_FLASH);
    EmBodyFx_Spawn(em, EMBODYFX_KIRIN_GLOW);
    Shadow_Spawn((Player *) em, 4);
}

/*
 * Empty callback (slots 2 and 5 of EmKirin_Callbacks).
 */
void EmKirin_TableNop(void)
{
}

/*
 * Empty; the default case of EmKirin_MotionEffects.
 */
void EmKirin_MotionEffectsDefault(Em *em)
{
}

/*
 * Keeps motionId[0] in work->fxMotion, plays EmKirin_MotionSounds, then the
 * motion effects at their frames: dust PartFx for the charge, gallop,
 * strides, stop, turn-arounds, lunge, hop and knockback landing; contact
 * attacks 0xB (KIRIN_MOT_LUNGE f20) and 0xC (KIRIN_MOT_STRIKE f14); lying
 * puffs at bone 0x17 for KIRIN_MOT_STATUS2_LIE.
 */
void EmKirin_MotionEffects(Em *em, EmWork *work)
{
    f32 puffOfs[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmKirin_MotionSounds(em, work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE):
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_B):
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_C):
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_D):
    case EM_MOTION_ID(0, KIRIN_MOT_IDLE_ACTION_E):
    case EM_MOTION_ID(0, KIRIN_MOT_WALK):
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_CHARGE):
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x15, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x11, 3, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_GALLOP):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x15, 3, 0.7f);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x11, 3, 0.7f);
        }
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STRIDE_A):
    case EM_MOTION_ID(0, KIRIN_MOT_STRIDE_B):
        if (work->fxMotion == 0x3F7) {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
                PartFx_SpawnForEmScaled(em, 0x15, 3, 0.9f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x11, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STOP):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0xB, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0xB, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 7, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 7, 3, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_TURN_AROUND_LEFT):
    case EM_MOTION_ID(0, KIRIN_MOT_TURN_AROUND_RIGHT):
        if (work->fxMotion == 0x3F7) {
            if (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0) {
                PartFx_SpawnForEmScaled(em, 0x15, 3, 0.6f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x11, 3, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, 0x10):
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_LUNGE):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            EmContactAttack_Create(em, 0xB);
        }
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x16, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_HOP):
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x16, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_JUMP):
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STRIKE):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            EmContactAttack_Create(em, 0xC);
        }
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_THREAT):
    case EM_MOTION_ID(0, KIRIN_MOT_CALL_LIGHTNING):
    case EM_MOTION_ID(0, KIRIN_MOT_FLINCH_A):
    case EM_MOTION_ID(0, KIRIN_MOT_FLINCH_B):
    case EM_MOTION_ID(0, KIRIN_MOT_KNOCKBACK):
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_KNOCKBACK_LAND):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 6, 0.5f);
        }
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_GET_UP):
    case EM_MOTION_ID(0, KIRIN_MOT_COLLAPSE):
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STATUS2_LIE):
        puffOfs[0] = 0.0f;
        puffOfs[1] = 0.0f;
        puffOfs[2] = 10.0f;
        Em_SpawnLyingPuffs(em, 0x17, puffOfs, 1.0f);
        break;
    case EM_MOTION_ID(0, KIRIN_MOT_STATUS4):
    case EM_MOTION_ID(0, KIRIN_MOT_STATUS4_SHAKE_OFF):
        break;
    default:
        EmKirin_MotionEffectsDefault(em);
        break;
    }
}

/*
 * Callback slot 3: skips the first frame (fxStep 0 to 1), then runs
 * EmKirin_MotionEffects each frame.
 */
void EmKirin_UpdateMotionEffects(Em *em)
{
    EmWork *work = (EmWork *)&em->work;

    switch (work->fxStep) {
    case 0:
        work->fxStep++;
        break;
    case 1:
        EmKirin_MotionEffects(em, work);
        break;
    }
}

/* The file's initialised data, in the game's order */
s32 GmEm_ReturnZero(void); /* gm_sub */
void *EmKirin_Callbacks[8] = {
    EmKirin_SpawnEffects, EmKirin_SpawnEffects, EmKirin_TableNop, EmKirin_UpdateMotionEffects,
    GmEm_ReturnZero, EmKirin_TableNop, NULL, NULL,
};
