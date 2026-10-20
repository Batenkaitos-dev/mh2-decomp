/*
 * Monster family: the Anteka and kind 3, likely the Kelbi (EM_KIND_ANTEKA and
 * EM_KIND_KELBI), which run the same functions (EmAnteka_*). The code from
 * the family's first function, EmAnteka_MotionSounds (its motions' sounds),
 * to its motion effects: Init and Start, one function per state, a
 * dispatcher per state group (Run*State) and ChangeState, the per-frame
 * Update that turns Em_UpdateDamage's result into reactions. Only the Kelbi
 * has Em.gender. The state functions take the monster and its work area
 * (EmWork); Em.step is the step within the current state. include/
 * em_anteka.h names the family's motions and states.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_anteka.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
void QuestEm_Hunted(Em *em);
/* prototypes */
void EmAnteka_ChangeState(Em *em, u16 group, u16 state, u8 mode);
u16 Rand_Next(s32 stream);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetLayerMotion(Em *em, s32 motion, s32 blend, s32 frame, s32 layer);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmAnteka_ChangeState, EmKhezu_ChangeState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void QuestEm_Leave(Em *em);
ObjAttach *EmContactAttack_Create(Em *em, s8 kind);
void Em_MoveRotatedAccel(Em *, void *);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void EmAnteka_Init(Em *);
void EmAnteka_BlownBack(Em *em, EmWork *work, s16 variant);
void EmAnteka_Die(Em *, EmWork *);
void EmAnteka_EndState(Em *em);
void EmAnteka_SetMotion(Em *em, s32 arg1, s32 arg2, s32 arg3);
void EmAnteka_MotionEffects(Em *, EmWork *);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
s32 Em_GetFloorY(Em *em, f32 *pos, f32 *out);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Shadow_Spawn(void *obj, s16 type);
void Em_TargetPlayerInRange(Em *);
s8 QuestEm_Appear(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
void Em_InitParts(Em *em);
void GmEm_ApplySize(Em *em);
s32 Em_SetCombat(Em *em, u8 a, s16 b);
s32 Obj_SameArea(void *obj, Player *b);
s32 Em_InBlockArea1(Em *em);
void Em_SetRequest(Em *em, u8 a, u8 prio);
extern u8 gEmShowsOnMap[];
s32 Em_UpdateCombatTimer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmAnteka_RunState(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void PartFx_SpawnForEmScaled(Em *em, s32 a, s32 b, f32 scale);
/* end of prototypes */

/*
 * The Anteka family's motion sounds: footsteps, thuds and calls at fixed
 * frames of the layer-0 motion in work->fxMotion (some picked at random by
 * Sound_RandomSe; one only while hunted). Called by EmAnteka_MotionEffects.
 */
void EmAnteka_MotionSounds(Em *em, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_A):
        Em_PlaySeAtBoneOnFrame(em, 0x5C, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x92, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xB6, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xD2, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xF0, 0xD, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x10E, 0xD, 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0xB, 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_C):
        Em_PlaySeAtBoneOnFrame(em, 6, 4, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_D):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x10, 4, 0x11, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x54, Sound_RandomSe(0x11, 4, 0x12, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xC8, Sound_RandomSe(0x10, 4, 0x12, 4), 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_E):
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0xE, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0xE, 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_WALK):
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_RUN):
        Em_PlaySeAtBoneOnFrame(em, 6, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_CHARGE):
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_MOVE_ACTION_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_MOVE_ACTION_B):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_RECOVER):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_AROUND_RIGHT):
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_AROUND_LEFT):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x38, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_COMBO):
        Em_PlaySeAtBoneOnFrame(em, 4, 7, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x3E, 8, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 9, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x94, 0xA, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x12, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_SLAM):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 2, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_QUICK):
        Em_PlaySeAtBoneOnFrame(em, 0x18, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_LEAP):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0x13, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_STRIKE):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 3, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_F):
        Em_PlaySeAtBoneOnFrame(em, 0xC, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0);
        break;
    case EM_MOTION_ID(0, 0x27):
        Em_PlaySeAtBoneOnFrame(em, 2, 0xB, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x5A, 2, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_FLINCH_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_FLINCH_B):
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_BLOWN_BACK):
        Em_PlaySeAtBoneOnFrame(em, 4, 1, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 4, 5, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_BLOWN_BACK_LAND):
        if ((em->stateGroup == EM_GROUP_DEFEAT) && (em->state == 0)) {
            Em_PlaySeAtBoneOnFrame(em, 0x3E, 0xC, 0);
            Em_PlaySeAtBoneOnFrame(em, 4, 6, 0);
            Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0);
            break;
        }
        Em_PlaySeAtBoneOnFrame(em, 4, 6, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0x14, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x32, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x18, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x4C, 0, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x26, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x7C, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 4, 4, 0);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_LIE):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_STATUS4):
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0x13, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8A, 0xF, 0x17);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_STATUS4_END):
        Em_PlaySeAtBoneOnFrame(em, 4, Sound_RandomSe(0x10, 4, 0x11, 4), 0x17);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        
        break;
    }
}

/*
 * Sets up an Anteka (EM_KIND_ANTEKA) or a Kelbi (EM_KIND_KELBI): outside a quest a test
 * placement by slot number; the blindness, stance and motion speed; hp from
 * Em_ComputeMaxHp; Em.gender (the Kelbi only: from the quest entry's variant,
 * giving a body scaled by 0.85, a second hp row and the layer-1 idle); the
 * idle motion, the parts, and the work byte from Em.spawnParam.
 */
void EmAnteka_Init(Em *em)
{
    EmWork *w = (EmWork *)&em->work;
    s16 hp;

    if (gQuest.stage == 0) {
        switch (gScene.area) {
        case AREA_FOREST_HILLS_4:
            em->posX = 9500.0f + 150.0f * (f32) (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 10000.0f + 150.0f * (f32) (em->slotNo & 7);
            break;
        default:
            em->posX = 5000.0f - 80.0f * (f32) em->slotNo;
            em->posY = 0.0f;
            em->posZ = 5000.0f - 80.0f * (f32) em->slotNo;
            break;
        }
    }
    em->blind = 0;
    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    hp = Em_ComputeMaxHp(em, 0);
    em->hp = hp;
    em->hpMax = hp;
    if (em->type == EM_KIND_KELBI) {
        if (gQuest.stage == 0) {
            em->gender = (u16) Rand_Next(1) & 1;
            em->gender = 0;
        } else {
            em->gender = em->variant;
        }
        if (em->gender != 0) {
            hp = Em_ComputeMaxHp(em, 1);
            em->hp = hp;
            em->hpMax = hp;
            em->scaleX *= 0.85f;
            em->scaleY *= 0.85f;
            em->scaleZ *= 0.85f;
        }
        em->bodyScale = em->scaleX;
        GmEm_ApplySize(em);
    } else {
        em->gender = 0;
    }
    Em_SetMotionRow(em, ANTEKA_MOT_IDLE, 0, 0);
    if (em->gender != 0) {
        Em_SetLayerMotion(em, EM_MOTION_ID(1, ANTEKA_MOT_IDLE), 0, 0, 1);
    }
    Em_InitParts(em);
    switch (em->spawnParam) {
    case 1:
        w->unk019 = 1;
        break;
    default:
        w->unk019 = 0;
        break;
    }
}

/* Init, then ANTEKA_IDLE. */
void EmAnteka_Start(Em *em)
{
    EmAnteka_Init(em);
    EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE, EM_SYNC_LOCAL);
}

/*
 * Runs Init again: the family's appear hook (EmVt.appear, slot 5 of
 * EmAnteka_Callbacks).
 */
void EmAnteka_Reinit(Em *em)
{
    EmAnteka_Init(em);
}

/*
 * The per-frame update. Out of an event it asks for script requests (a
 * large monster fighting in its area; Em_InBlockArea1), then turns
 * Em_UpdateDamage's result into a state: killed or dead ANTEKA_DEFEAT_DIE;
 * a part or mode-3 hit a flinch by side, or ANTEKA_REACT_BLOWN_BACK when
 * the hit comes from behind, part 0 is out of hp or it is off the ground; a
 * plain hit on the ground a flinch by side; statuses 2 and 4 their states;
 * a hit while lying ANTEKA_REACT_BLOWN_BACK_LYING; the stun only ends
 * status 5. The family is never trapped or captured. Then the pending
 * script, and the state (twice when a state was requested).
 */
void EmAnteka_Update(Em *em)
{
    u8 largeFighting = 0;
    u8 inGroup6 = 0;
    u8 largeCalm = 0;
    s16 i;
    Em *other = gEms;
    u8 hitInfo[3];

    Em_UpdateCombatTimer(em);
    if (em->requested != 1 && em->stateGroup != EM_GROUP_EVENT) {
        for (i = 0; i < 20; i++, other++) {
            if (other->active && other->visible && (Obj_SameArea(em, (Player *) other) & 0xFF)) {
                if (gEmShowsOnMap[other->type] != 0) {
                    if (other->combat == 1) {
                        largeFighting = 1;
                        break;
                    }
                    if (other->combat == 0) {
                        largeCalm = 1;
                    }
                }
                if (other->variant == 0 && other->stateGroup == EM_GROUP_EVENT) {
                    inGroup6 = 1;
                }
            }
        }
        if (largeFighting != 0 || (largeCalm != 0 && inGroup6 != 0)) {
            Em_SetRequest(em, 1, 0x90);
            Em_RestartScript(em);
            em->scriptPending = 1;
        }
    }
    switch (Em_UpdateDamage(em, hitInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_STATUS_IN_AIR:
    case EM_DMG_CAPTURED:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        EmAnteka_ChangeState(em, EM_GROUP_DEFEAT, ANTEKA_DEFEAT_DIE, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS4:
        Em_SetStatus4Timer(em);
        EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_STATUS4, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS2:
        Em_SetStatus2Timer(em);
        EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE_STATUS2, EM_SYNC_OWNER);
        break;
    case EM_DMG_HIT_LYING:
        EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_BLOWN_BACK_LYING, EM_SYNC_OWNER);
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_HIT_MODE3:
        {
            s32 hitDelta = em->hitAngle - em->rotY;

            if (((hitDelta - 0x4000) & 0xFFFF) < 0x8000 && em->parts[0].hp > 0 && em->stance == EM_STANCE_GROUND) {
                if ((hitDelta & 0xFFFF) < 0x8000) {
                    EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_FLINCH_A, EM_SYNC_OWNER);
                } else {
                    EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_FLINCH_B, EM_SYNC_OWNER);
                }
            } else {
                EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_BLOWN_BACK, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_HIT:
        if (em->stance == EM_STANCE_GROUND) {
            s32 plainHitDelta = (em->hitAngle - em->rotY) & 0xFFFF;

            em->scriptPending = 0;
            if (plainHitDelta < 0x8000) {
                EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_FLINCH_A, EM_SYNC_OWNER);
            } else {
                EmAnteka_ChangeState(em, EM_GROUP_REACT, ANTEKA_REACT_FLINCH_B, EM_SYNC_OWNER);
            }
        }
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    }
    if (em->requested != 1 && (u8) Em_InBlockArea1(em) == 1) {
        Em_SetRequest(em, 2, 0x90);
        Em_RestartScript(em);
        em->scriptPending = 1;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmAnteka_RunState(em);
    if (em->stateRequest != 0) {
        EmAnteka_RunState(em);
        em->stateRequest = 0;
    }
}

/* ANTEKA_IDLE_NONE: does nothing. */
void EmAnteka_IdleNone(void)
{
}

/*
 * ANTEKA_IDLE: plays the idle motion once, then EmAnteka_EndState (the
 * script picks the next state).
 */
void EmAnteka_Idle(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_A: plays its motion once, then EmAnteka_EndState. */
void EmAnteka_IdleActionA(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_B: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionB(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_C: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionC(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_C, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_D: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionD(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_D, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_E: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionE(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_E, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_IDLE_FACE_GOAL: sets targetAngle toward the goal and plays the
 * quarter-turn motion for the goal's side; rotY is not changed here.
 */
void EmAnteka_FaceGoal(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (((em->targetAngle - em->rotY) & 0xFFFF) < 0x8001) {
            EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_RIGHT, 0, 0);
            break;
        }
        EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_LEFT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_IDLE_STATUS2: begins status 2, collapses and lies (hit-react mode
 * EM_HIT_REACT_WAKE, blind 2) until the status timer (Em_SetStatus2Timer)
 * runs out, then ANTEKA_IDLE_STATUS2_END.
 */
void EmAnteka_Status2Collapse(Em *em, EmWork *work)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_BeginStatus2(em);
        Em_SetMotion(em, ANTEKA_MOT_COLLAPSE, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, ANTEKA_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * ANTEKA_IDLE_STATUS2_END: ends status 2, gets up, sees again, then
 * EmAnteka_EndState.
 */
void EmAnteka_Status2End(Em *em, EmWork *work)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        Em_EndStatus2(em);
        Em_SetMotion(em, ANTEKA_MOT_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_IDLE_STATUS4_END: ends status 4 and plays its end motion, then
 * EmAnteka_EndState.
 */
void EmAnteka_Status4End(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, ANTEKA_MOT_STATUS4_END, 0, 0);
        Em_EndStatus4(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_IDLE_ACTION_F: as ANTEKA_IDLE_ACTION_A with its own motion. */
void EmAnteka_IdleActionF(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * Runs the group 0 state (EmAnteka_RunState also sends group 2 here),
 * keeping the group and state as the previous ones.
 */
void EmAnteka_RunIdleState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case ANTEKA_IDLE_NONE:
        EmAnteka_IdleNone();
        break;
    case ANTEKA_IDLE:
        EmAnteka_Idle(em, work);
        break;
    case ANTEKA_IDLE_ACTION_A:
        EmAnteka_IdleActionA(em, work);
        break;
    case ANTEKA_IDLE_ACTION_B:
        EmAnteka_IdleActionB(em, work);
        break;
    case ANTEKA_IDLE_ACTION_C:
        EmAnteka_IdleActionC(em, work);
        break;
    case ANTEKA_IDLE_ACTION_D:
        EmAnteka_IdleActionD(em, work);
        break;
    case ANTEKA_IDLE_ACTION_E:
        EmAnteka_IdleActionE(em, work);
        break;
    case ANTEKA_IDLE_FACE_GOAL:
        EmAnteka_FaceGoal(em, work);
        break;
    case ANTEKA_IDLE_STATUS2:
        EmAnteka_Status2Collapse(em, work);
        break;
    case ANTEKA_IDLE_STATUS2_END:
        EmAnteka_Status2End(em, work);
        break;
    case ANTEKA_IDLE_STATUS4_END:
        EmAnteka_Status4End(em, work);
        break;
    case ANTEKA_IDLE_ACTION_F:
        EmAnteka_IdleActionF(em, work);
        
    default:
        break;
    }
}

/*
 * ANTEKA_MOVE_WALK_TURN: walks while rotY turns toward the goal's angle
 * (0x100 a frame); ends when within 0x100.
 */
void EmAnteka_WalkTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        EmAnteka_SetMotion(em, ANTEKA_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x100) < 0x100) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * ANTEKA_MOVE_WALK: walks to the goal (Em_StepToGoal), then
 * EmAnteka_EndState.
 */
void EmAnteka_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_WALK, 0, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_RUN and ANTEKA_MOVE_RUN_FAR (they differ only in the stop
 * distance EmAnteka_ChangeState sets; `farStop` tells them apart and is not
 * read): runs to the goal, then the recover motion, then EmAnteka_EndState.
 */
void EmAnteka_Run(Em *em, EmWork *work, s32 farStop)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_RUN, 0, 0);
        break;
    case 1:
        if ((Em_StepToGoal(em, 0x40, 0) == 1) && (em->motion[0].playing == 0)) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 4, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_CHARGE and ANTEKA_MOVE_CHARGE_FAR: as EmAnteka_Run with the
 * charge motion; an Anteka (not a Kelbi) also makes one contact attack at
 * frame 20 (stateVar0 keeps it to one).
 */
void EmAnteka_Charge(Em *em, EmWork *work, s32 farStop)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stateVar0 = 0;
        EmAnteka_SetMotion(em, ANTEKA_MOT_CHARGE, 0, 0);
        break;
    case 1:
        if ((em->type == EM_KIND_ANTEKA) && (em->stateVar0 == 0) && (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0)) {
            EmContactAttack_Create(em, 0x16);
            em->stateVar0++;
        }
        if ((Em_StepToGoal(em, 0x40, 0) == 1) && (em->motion[0].playing == 0)) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 4, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_ACTION_A: plays its motion, then the recover motion from
 * frame 26, then EmAnteka_EndState.
 */
void EmAnteka_MoveActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_MOVE_ACTION_B: as ANTEKA_MOVE_ACTION_A with its own motion. */
void EmAnteka_MoveActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_B, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_TURN_AROUND_RIGHT: a half turn: rotY grows by 0x888 a frame
 * once the blend is over, plus 8 at frame 30 (exactly 0x8000 in all), then
 * EmAnteka_EndState.
 */
void EmAnteka_TurnAroundRight(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_AROUND_RIGHT, 0, 0);
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
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_MOVE_TURN_AROUND_LEFT: the mirror of EmAnteka_TurnAroundRight. */
void EmAnteka_TurnAroundLeft(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_AROUND_LEFT, 0, 0);
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
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_TURN_QUICK: plays the quick-turn motion alone (rotY
 * unchanged), then EmAnteka_EndState.
 */
void EmAnteka_TurnQuick(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_QUICK, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_ACTION_SERIES: plays the two move actions twice in turn, then
 * EmAnteka_EndState.
 */
void EmAnteka_MoveActionSeries(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_B, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_A, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_MOVE_ACTION_B, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_TURN_TO_GOAL: picks a turn motion from the goal's side and
 * angle (a quarter turn, the Kelbi's quick turn, or a half turn) and steps
 * rotY by hand each frame (the angle over half the motion's last frame),
 * snapping onto targetAngle when within a step; ends with the motion.
 */
void EmAnteka_TurnToGoal(Em *em, EmWork *work)
{
    f32 turnFrame;
    u32 turnStep;
    u32 angleDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        angleDiff = (em->targetAngle - em->rotY) & 0xFFFF;
        if ((em->type == EM_KIND_ANTEKA && angleDiff <= 0x4000U) || angleDiff >= 0xC000U || (em->type != EM_KIND_ANTEKA && angleDiff <= 0x3000U) || angleDiff >= 0xD000U) {
            if (em->type == EM_KIND_ANTEKA) {
                if (angleDiff >= 0x8000U) {
                    EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_LEFT, 0, 0);
                } else {
                    EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_RIGHT, 0, 0);
                }
            } else {
                EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_QUICK, 0, 0);
            }
        } else if (angleDiff >= 0x8000U) {
            EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_AROUND_LEFT, 0, 0);
        } else {
            EmAnteka_SetMotion(em, ANTEKA_MOT_TURN_AROUND_RIGHT, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].blend != 0) {
            break;
        }
        switch (em->motionId[0]) {
        case 0x3FA:
            turnFrame = 26.0f;
            turnStep = 0x4000U / (u32) (turnFrame / 2.0f);
            break;
        case 0x3EF:
        case 0x3F0:
            turnFrame = 48.0f;
            turnStep = 0x4000U / (u32) (turnFrame / 2.0f);
            break;
        case 0x3F7:
        case 0x3FE:
        default:
            turnFrame = 30.0f;
            turnStep = 0x8000U / (u32) (turnFrame / 2.0f);
            break;
        }
        angleDiff = (em->targetAngle - (em->rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((angleDiff + turnStep) & 0xFFFF) < turnStep * 2) {
            em->rotY = em->targetAngle;
        } else if (angleDiff < 0x8000U) {
            em->rotY = (em->rotY + turnStep) & 0xFFFF;
        } else {
            em->rotY = (em->rotY - turnStep) & 0xFFFF;
        }
        if (EmMotion_ReachedFrame(em, 0, turnFrame) != 0) {
            em->step++;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_MOVE_LEAP: a forward leap (velY 75, accelY -10, velZ 50), airborne
 * from frame 26; the motion holds at frame 36 until it lands, then the
 * recover motion, then EmAnteka_EndState. `entryStep` is the step on entry,
 * written back + 1 (the original's form).
 */
void EmAnteka_Leap(Em *em, EmWork *work)
{
    u8 tookOff;
    u8 entryStep;

    entryStep = em->step;
    tookOff = 0;
    switch (entryStep) {
    case 0:
        em->step = entryStep + 1;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, ANTEKA_MOT_LEAP, 0, 0);
        Player_ClearVelocity(em);
        em->velY = 75.0f;
        em->accelY = -10.0f;
        em->velZ = 50.0f;
        work->accelAngX = 0;
        work->accelAngZ = 0;
        break;
    case 1:
        if (EmMotion_ReachedFrame(em, 0, 26.0f) != 0) {
            em->stance = EM_STANCE_AIR;
            tookOff = 1;
            work->accelAngY = em->rotY;
            Em_MoveRotatedAccel(em, &work->accelAngX);
        }
        if (EmMotion_ReachedFrame(em, 0, 36.0f) != 0) {
            em->motionSpeed = 0.0f;
        }
        Em_GetFloorY(em, &em->posX, &em->floorY);
        if (tookOff && (em->posY <= em->floorY)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            em->posY = em->floorY;
            em->motionSpeed = 1.0f;
        }
        break;
    case 2:
        if (EmMotion_ReachedFrame(em, 0, 44.0f) != 0) {
            em->step++;
            Em_SetMotion(em, ANTEKA_MOT_RECOVER, 4, 0xA);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step = entryStep + 1;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * Runs the group 1 state, keeping the group and state as the previous ones;
 * ANTEKA_MOVE_RUN_FAR and ANTEKA_MOVE_CHARGE_FAR run the functions of
 * ANTEKA_MOVE_RUN and ANTEKA_MOVE_CHARGE.
 */
void EmAnteka_RunMoveState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case ANTEKA_MOVE_WALK_TURN:
        EmAnteka_WalkTurn(em);
        break;
    case ANTEKA_MOVE_WALK:
        EmAnteka_Walk(em);
        break;
    case ANTEKA_MOVE_RUN:
        EmAnteka_Run(em, work, 0);
        break;
    case ANTEKA_MOVE_CHARGE:
        EmAnteka_Charge(em, work, 0);
        break;
    case ANTEKA_MOVE_ACTION_A:
        EmAnteka_MoveActionA(em);
        break;
    case ANTEKA_MOVE_ACTION_B:
        EmAnteka_MoveActionB(em);
        break;
    case ANTEKA_MOVE_TURN_AROUND_RIGHT:
        EmAnteka_TurnAroundRight(em);
        break;
    case ANTEKA_MOVE_TURN_AROUND_LEFT:
        EmAnteka_TurnAroundLeft(em);
        break;
    case ANTEKA_MOVE_TURN_QUICK:
        EmAnteka_TurnQuick(em);
        break;
    case ANTEKA_MOVE_RUN_FAR:
        EmAnteka_Run(em, work, 1);
        break;
    case ANTEKA_MOVE_CHARGE_FAR:
        EmAnteka_Charge(em, work, 1);
        break;
    case ANTEKA_MOVE_ACTION_SERIES:
        EmAnteka_MoveActionSeries(em);
        break;
    case ANTEKA_MOVE_TURN_TO_GOAL:
        EmAnteka_TurnToGoal(em, work);
        break;
    case ANTEKA_MOVE_LEAP:
        EmAnteka_Leap(em, work);
        break;
    }
}

/*
 * ANTEKA_ATK_COMBO: plays the combo motion (its three contact attacks come
 * from EmAnteka_MotionEffects), then EmAnteka_EndState. The other branch of
 * the kind test cannot run: only EM_KIND_KELBI and EM_KIND_ANTEKA reach these
 * functions.
 */
void EmAnteka_AttackCombo(Em *em, EmWork *work)
{
    u8 kind;

    switch (em->step) {
    case 0:
        em->step++;
        kind = em->type;
        if ((kind == 3) || (kind == EM_KIND_ANTEKA)) {
            EmAnteka_SetMotion(em, ANTEKA_MOT_ATK_COMBO, 0, 0);
            break;
        }
        EmAnteka_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_F, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_ATK_SLAM: one hit with a ground impact, then the recover motion
 * from frame 26, then EmAnteka_EndState.
 */
void EmAnteka_AttackSlam(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_ATK_SLAM, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_RECOVER, 0, 0x1A);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_ATK_STRIKE: the leap's motion in place, then the strike from frame
 * 8 (one contact attack), then EmAnteka_EndState.
 */
void EmAnteka_AttackStrike(Em *em, EmWork *work)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_LEAP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_ATK_STRIKE, 0, 8);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_ATK_NONE_A: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneA(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_B: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneB(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_C: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneC(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_D: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneD(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_E: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneE(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/* ANTEKA_ATK_NONE_F: EmAnteka_EndState at once. */
void EmAnteka_AttackNoneF(Em *em, EmWork *work)
{
    EmAnteka_EndState(em);
}

/*
 * Runs the group 3 state, keeping the group and state as the previous ones.
 * Only the script enters group 3.
 */
void EmAnteka_RunAttackState(Em *em, EmWork *work)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case ANTEKA_ATK_COMBO:
        EmAnteka_AttackCombo(em, work);
        break;
    case ANTEKA_ATK_SLAM:
        EmAnteka_AttackSlam(em, work);
        break;
    case ANTEKA_ATK_STRIKE:
        EmAnteka_AttackStrike(em, work);
        break;
    case ANTEKA_ATK_NONE_A:
        EmAnteka_AttackNoneA(em, work);
        break;
    case ANTEKA_ATK_NONE_B:
        EmAnteka_AttackNoneB(em, work);
        break;
    case ANTEKA_ATK_NONE_C:
        EmAnteka_AttackNoneC(em, work);
        break;
    case ANTEKA_ATK_NONE_D:
        EmAnteka_AttackNoneD(em, work);
        break;
    case ANTEKA_ATK_NONE_E:
        EmAnteka_AttackNoneE(em, work);
        break;
    case ANTEKA_ATK_NONE_F:
        EmAnteka_AttackNoneF(em, work);
        
    default:
        break;
    }
}

/*
 * ANTEKA_REACT_FLINCH_A: plays its flinch, restarts the script, then
 * EmAnteka_EndState.
 */
void EmAnteka_FlinchA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_FLINCH_A, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/* ANTEKA_REACT_FLINCH_B: the mirror flinch. */
void EmAnteka_FlinchB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_FLINCH_B, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_REACT_BLOWN_BACK and (`fromLying` 1)
 * ANTEKA_REACT_BLOWN_BACK_LYING, which also ends status 2 and stays blind:
 * turns to face the hit, is thrown up and back (stance EM_STANCE_AIR) until
 * it reaches the floor, lies for 90 frames, gets up, then
 * EmAnteka_EndState.
 */
void EmAnteka_BlownBack(Em *em, EmWork *work, s16 fromLying)
{
    switch (fromLying) {
    case 1:
        em->blind = 2;
        break;
    }
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_BLOWN_BACK, 0, 0);
        switch (fromLying) {
        case 1:
            Em_EndStatus2(em);
            break;
        }
        em->rotY = em->hitAngle + 0x8000;
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
            EmAnteka_SetMotion(em, ANTEKA_MOT_BLOWN_BACK_LAND, 0, 0);
            em->timer = 0x5A;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * ANTEKA_REACT_STATUS4: begins status 4 (hit-react mode EM_HIT_REACT_STATUS
 * every frame) with an effect at bone 2 each frame, until the status timer
 * (Em_SetStatus4Timer) runs out; then ANTEKA_IDLE_STATUS4_END.
 */
void EmAnteka_Status4(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, ANTEKA_MOT_STATUS4, 0, 0);
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE_STATUS4_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* ANTEKA_REACT_COLLAPSE: collapses, gets up, then EmAnteka_EndState. */
void EmAnteka_CollapseAndGetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_COLLAPSE, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_SetMotion(em, ANTEKA_MOT_GET_UP, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmAnteka_EndState(em);
        }
        break;
    }
}

/*
 * Runs the group 4 state; ANTEKA_REACT_BLOWN_BACK_LYING runs
 * EmAnteka_BlownBack with 1.
 */
void EmAnteka_RunReactState(Em *em, EmWork *work)
{
    switch (em->state) {
    case ANTEKA_REACT_FLINCH_A:
        EmAnteka_FlinchA(em);
        break;
    case ANTEKA_REACT_FLINCH_B:
        EmAnteka_FlinchB(em);
        break;
    case ANTEKA_REACT_BLOWN_BACK:
        EmAnteka_BlownBack(em, work, 0);
        break;
    case ANTEKA_REACT_STATUS4:
        EmAnteka_Status4(em);
        break;
    case ANTEKA_REACT_COLLAPSE:
        EmAnteka_CollapseAndGetUp(em);
        break;
    case ANTEKA_REACT_BLOWN_BACK_LYING:
        EmAnteka_BlownBack(em, work, 1);
        break;
    }
}

/*
 * ANTEKA_DEFEAT_DIE, the death: unhittable (10 frames, refreshed); thrown
 * back (QuestEm_Hunted), lands, leaves a carcass mine point (EmMine_Create)
 * for 2400 frames or until it is used up, fades over 60 frames and hides,
 * then ANTEKA_DEFEAT_REAPPEAR. The body is kept on the floor every frame.
 */
void EmAnteka_Die(Em *em, EmWork *work)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        EmAnteka_SetMotion(em, ANTEKA_MOT_BLOWN_BACK, 0, 0);
        em->rotY = em->hitAngle + 0x8000;
        em->motionSpeed = 1.0f;
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
            EmAnteka_SetMotion(em, ANTEKA_MOT_BLOWN_BACK_LAND, 0, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 2400;
            em->motionSpeed = 0.0f;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        if (--em->timer <= 0 || EmMine_Update(em) <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 4:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmAnteka_ChangeState(em, EM_GROUP_DEFEAT, ANTEKA_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * ANTEKA_DEFEAT_REAPPEAR: back to ANTEKA_IDLE when QuestEm_CanAppear allows
 * (EmAnteka_ChangeState called QuestEm_Appear), else ends the task and
 * hides the monster.
 */
void EmAnteka_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
        em->visible = 0;
        break;
    }
}

/*
 * ANTEKA_DEFEAT_LEAVE: unhittable, fades over 100 frames, then
 * QuestEm_Leave ends the task and hides the monster.
 */
void EmAnteka_FadeAndLeave(Em *em)
{
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x64;
        break;
    case 1:
        em->timer -= 1;
        em->alpha = (f32) em->timer / 100.0f;
        if (em->timer <= 0) {
            QuestEm_Leave(em);
            em->taskPhase++;
            em->visible = 0;
        }
        break;
    }
}

/* Runs the group 5 state. */
void EmAnteka_RunDefeatState(Em *em, EmWork *work)
{
    switch (em->state) {
    case ANTEKA_DEFEAT_DIE:
        EmAnteka_Die(em, work);
        break;
    case ANTEKA_DEFEAT_REAPPEAR:
        EmAnteka_Reappear(em);
        break;
    case ANTEKA_DEFEAT_LEAVE:
        EmAnteka_FadeAndLeave(em);
        break;
    }
}

/*
 * ANTEKA_EVENT_LEAVE: fades over 60 frames, then QuestEm_Leave ends the
 * task and hides the monster.
 */
void EmAnteka_EventFadeAndLeave(Em *em)
{
    f32 alpha;

    switch (em->step) {
    case 0:
        em->step++;
        break;
    case 1:
        alpha = em->alpha - 0.016666668f;
        em->alpha = alpha;
        if (alpha <= 0.0f) {
            QuestEm_Leave(em);
            em->taskPhase++;
            em->visible = 0;
        }
        break;
    }
}

/*
 * ANTEKA_EVENT_SCENE: unhittable and not sensing; placed at a fixed spot
 * facing rotY 0x8000, plays three motions in a row, then EmAnteka_EndState.
 */
void EmAnteka_EventScene(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->posX = 14419.0f;
        em->posY = 0.0f;
        em->posZ = 15007.0f;
        em->rotX = 0;
        em->rotY = 0x8000;
        em->rotZ = 0;
        Em_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_A, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, ANTEKA_MOT_IDLE_ACTION_B, 6, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            em->timer = 0x96;
            Em_SetMotion(em, ANTEKA_MOT_CHARGE, 6, 0);
        }
        break;
    case 3:
        em->step++;
        EmAnteka_EndState(em);
        break;
    }
}

/*
 * ANTEKA_EVENT_IDLE: unhittable and not sensing; plays the idle motion,
 * then EmAnteka_EndState.
 */
void EmAnteka_EventIdle(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->motionSpeed = 1.0f;
        Em_SetMotion(em, ANTEKA_MOT_IDLE, 0, 0);
        break;
    case 1:
        EmAnteka_EndState(em);
        break;
    }
}

/* Runs the group 6 state (EmAnteka_RunState also sends group 7 here). */
void EmAnteka_RunEventState(Em *em)
{
    switch (em->state) {
    case ANTEKA_EVENT_LEAVE:
        EmAnteka_EventFadeAndLeave(em);
        break;
    case ANTEKA_EVENT_SCENE:
        EmAnteka_EventScene(em);
        break;
    case ANTEKA_EVENT_IDLE:
        EmAnteka_EventIdle(em);
        break;
    }
}

/*
 * Runs the state group's dispatcher: group 2 runs group 0's, group 7 group
 * 6's, group 8 nothing. The work area is passed on as an EmWork.
 */
void EmAnteka_RunState(Em *em)
{
    u8 *w = &em->work;

    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmAnteka_RunIdleState(em, (EmWork *) w);
        break;
    case EM_GROUP_MOVE:
        EmAnteka_RunMoveState(em, (EmWork *) w);
        break;
    case EM_GROUP_FLY:
        EmAnteka_RunIdleState(em, (EmWork *) w);
        break;
    case EM_GROUP_ATTACK:
        EmAnteka_RunAttackState(em, (EmWork *) w);
        break;
    case EM_GROUP_REACT:
        EmAnteka_RunReactState(em, (EmWork *) w);
        break;
    case EM_GROUP_DEFEAT:
        EmAnteka_RunDefeatState(em, (EmWork *) w);
        break;
    case EM_GROUP_EVENT:
        EmAnteka_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmAnteka_RunEventState(em);
        
    default:
        break;
    }
}

/*
 * Changes the state (Em_SetStateSync). On the machine that runs the monster
 * it first targets a player in range (Em_TargetPlayerInRange); the move
 * states set the stop distance (100 for the run and charge, 300 for their
 * _FAR versions); ANTEKA_DEFEAT_REAPPEAR calls QuestEm_Appear when
 * QuestEm_CanAppear allows. The `goto move` shares the speed call between
 * the case groups (the match).
 */
void EmAnteka_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 stopDist;

    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    }
    switch (group) {
    case EM_GROUP_MOVE:
        switch (state) {
        case ANTEKA_MOVE_WALK_TURN:
            Em_SetSpeedScaled(em, 0, 1000.0f, 100.0f);
            break;
        case ANTEKA_MOVE_WALK:
        case ANTEKA_MOVE_RUN:
        case ANTEKA_MOVE_CHARGE:
            stopDist = 100.0f;
            goto move;
        case ANTEKA_MOVE_RUN_FAR:
        case ANTEKA_MOVE_CHARGE_FAR:
            stopDist = 300.0f;
        move:
            Em_SetSpeedScaled(em, 0, 1000.0f, stopDist);
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == 4 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_IDLE:
    case EM_GROUP_FLY:
    case EM_GROUP_ATTACK:
    case EM_GROUP_REACT:
    case EM_GROUP_EVENT:
    case EM_GROUP_EVENT2:
    case EM_GROUP_VERSUS:
        break;
    }
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends the state: motion speed 1, on the ground, not looking at the target,
 * the script pending, then ANTEKA_IDLE.
 */
void EmAnteka_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->scriptPending = 1;
    EmAnteka_ChangeState(em, EM_GROUP_IDLE, ANTEKA_IDLE, EM_SYNC_LOCAL);
}

/*
 * Plays a motion on layer 0 (Em_SetMotion); for a Kelbi with Em.gender
 * also starts the same motion on layer 1 (Em_SetLayerMotion) for the seven
 * motions listed.
 */
void EmAnteka_SetMotion(Em *em, s32 motion, s32 blend, s32 frame)
{
    s32 layerMotion;

    Em_SetMotion(em, motion, blend, frame);
    if (em->gender != 0) {
        switch (em->motionId[0]) {
        case EM_MOTION_ID(0, ANTEKA_MOT_IDLE):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_IDLE);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_A):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_IDLE_ACTION_A);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_B):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_IDLE_ACTION_B);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_D):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_IDLE_ACTION_D);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_WALK):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_WALK);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_RUN):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_RUN);
            break;
        case EM_MOTION_ID(0, ANTEKA_MOT_CHARGE):
            layerMotion = EM_MOTION_ID(1, ANTEKA_MOT_CHARGE);
            break;
        default:
            layerMotion = 0;
            break;
        }
        if (layerMotion != 0) {
            Em_SetLayerMotion(em, layerMotion, blend, frame, 1);
        }
    }
}

void EmAnteka_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0);
}

void EmAnteka_TableNop(void)
{
}

void EmAnteka_MotionEffectsDefault(Em *em)
{
}

/*
 * Runs the motion sounds (EmAnteka_MotionSounds) and, by the layer-0 motion
 * kept in work->fxMotion, the dust and impact effects at the footfall and
 * impact frames and the contact attacks at the strike frames. The kind
 * tests are always true (only EM_KIND_KELBI and EM_KIND_ANTEKA reach it), and the
 * first test is redundant: both kept for the match.
 */
void EmAnteka_MotionEffects(Em *em, EmWork *work)
{
    f32 boneOffset[3];

    if (em->motionId[0] != work->fxMotion) {
        work->fxMotion = em->motionId[0];
    }
    EmAnteka_MotionSounds(em, work);
    switch (work->fxMotion) {
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_B):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_C):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_D):
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_E):
    case EM_MOTION_ID(0, ANTEKA_MOT_WALK):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_RUN):
        if (EmMotion_PassedFrameScaled(em, 4.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x15, 3, 0.6f);
        }
        if (EmMotion_PassedFrameScaled(em, 28.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x11, 3, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_CHARGE):
        if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x15, 3, 0.7f);
        }
        if (EmMotion_PassedFrameScaled(em, 18.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x11, 3, 0.7f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_MOVE_ACTION_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_MOVE_ACTION_B):
        if (work->fxMotion == 0x3F7) {
            if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
                PartFx_SpawnForEmScaled(em, 0x15, 3, 0.9f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 12.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x11, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_RECOVER):
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
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_AROUND_RIGHT):
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_AROUND_LEFT):
        if (work->fxMotion == 0x3F7) {
            if (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0) {
                PartFx_SpawnForEmScaled(em, 0x15, 3, 0.6f);
            }
        } else if (EmMotion_PassedFrameScaled(em, 34.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x11, 3, 0.6f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_COMBO):
        if (em->type == EM_KIND_KELBI || em->type == EM_KIND_ANTEKA) {
            if (EmMotion_PassedFrameScaled(em, 64.0f, 0U) != 0) {
                EmContactAttack_Create(em, 0x11);
            }
            if (EmMotion_PassedFrameScaled(em, 80.0f, 0U) != 0) {
                EmContactAttack_Create(em, 0x12);
            }
            if (EmMotion_PassedFrameScaled(em, 146.0f, 0U) != 0) {
                EmContactAttack_Create(em, 0x13);
            }
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_SLAM):
        if (EmMotion_PassedFrameScaled(em, 20.0f, 0U) != 0) {
            if (em->type == EM_KIND_KELBI || em->type == EM_KIND_ANTEKA) {
                EmContactAttack_Create(em, 0x15);
            } else {
                EmContactAttack_Create(em, 0xB);
            }
        }
        if (EmMotion_PassedFrameScaled(em, 24.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x16, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_TURN_QUICK):
        if (EmMotion_PassedFrameScaled(em, 26.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 0x16, 3, 0.9f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_LEAP):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_ATK_STRIKE):
        if (EmMotion_PassedFrameScaled(em, 14.0f, 0U) != 0) {
            if (em->type == EM_KIND_KELBI || em->type == EM_KIND_ANTEKA) {
                EmContactAttack_Create(em, 0x14);
            } else {
                EmContactAttack_Create(em, 0xC);
            }
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_IDLE_ACTION_F):
    case EM_MOTION_ID(0, 0x27):
    case EM_MOTION_ID(0, ANTEKA_MOT_FLINCH_A):
    case EM_MOTION_ID(0, ANTEKA_MOT_FLINCH_B):
    case EM_MOTION_ID(0, ANTEKA_MOT_BLOWN_BACK):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_BLOWN_BACK_LAND):
        if (EmMotion_PassedFrameScaled(em, 44.0f, 0U) != 0) {
            PartFx_SpawnForEmScaled(em, 2, 6, 0.5f);
        }
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_GET_UP):
    case EM_MOTION_ID(0, ANTEKA_MOT_COLLAPSE):
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_LIE):
        boneOffset[0] = 0.0f;
        boneOffset[1] = 0.0f;
        boneOffset[2] = 10.0f;
        Em_SpawnLyingPuffs(em, 0x17, boneOffset, 1.0f);
        break;
    case EM_MOTION_ID(0, ANTEKA_MOT_STATUS4):
    case EM_MOTION_ID(0, ANTEKA_MOT_STATUS4_END):
        break;
    default:
        EmAnteka_MotionEffectsDefault(em);
        break;
    }
}

/*
 * The family's hook 3 (Em_RunMotionFx, every frame): the first frame only
 * advances fxStep, later frames run EmAnteka_MotionEffects.
 */
void EmAnteka_UpdateMotionEffects(Em *em)
{
    EmWork *w = (EmWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmAnteka_MotionEffects(em, w);
        break;
    }
}

/* The file's initialised data, in the game's order */
s32 GmEm_ReturnZero(void); /* gm_sub */
void *EmAnteka_Callbacks[8] = {
    EmAnteka_SpawnEffect01, EmAnteka_SpawnEffect01, EmAnteka_TableNop, EmAnteka_UpdateMotionEffects,
    GmEm_ReturnZero, EmAnteka_Reinit, NULL, NULL,
};
