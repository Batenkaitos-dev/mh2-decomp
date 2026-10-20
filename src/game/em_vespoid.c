/*
 * Monster family: the Vespoid and the Hornetaur (EM_KIND_VESPOID,
 * EM_KIND_HORNETAUR), which run the same code. From the family's first
 * function, EmVespoid_MotionSounds (its motions' sounds), to its movement
 * curves: Init, Start and Reinit, EmVespoid_ChangeState, the states of its
 * eight groups and their dispatchers, Update, the effects made with its
 * motions, and the curves that drive its flight (EmVespoid_StartCurves,
 * EmVespoid_UpdateCurves). em_vespoid.h holds its motions, states, curve sets
 * and its work area's own fields (EmVespoidWork). The state functions take
 * the monster and the work area its state dispatcher passes on (EmWork);
 * Em.step is the step within the current state.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_vespoid.h"
#include "player.h"
void EmVespoid_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus4(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
s32 Motion_PassedFrame(void *, s32 layer, f32 frame);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
u16 Rand_Next(s32 stream);
void VespoidAttack_Create(Em *, s8);
void Em_MoveRotatedAccel(Em *, void *);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void EmVespoid_Init(Em *em);
void EmVespoid_Status2Lie(Em *);
void EmVespoid_RunMoveState(Em *em);
void EmVespoid_FlyForward(Em *em, s32 arg1);
void EmVespoid_FlyBack(Em *, s16);
void EmVespoid_Hover(Em *em);
void EmVespoid_TakeOff(Em *em);
void EmVespoid_Land(Em *em);
void EmVespoid_Dart(Em *);
void EmVespoid_Lunge(Em *em);
void EmVespoid_StingA(Em *);
void EmVespoid_StingB(Em *);
void EmVespoid_FallFromAir(Em *em);
void EmVespoid_FallAndLie(Em *em);
void EmVespoid_Die(Em *);
void EmVespoid_DieCarvable(Em *);
void EmVespoid_Reappear(Em *);
void EmVespoid_FlyIn(Em *);
void EmVespoid_StartCurves(Em *em, u8 kind);
u8 EmVespoid_UpdateCurves(Em *);
void EmVespoid_MoveByVelocity(Em *em);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 QuestEm_CanAppear(Em *em);
void Player_Integrate(void *);
void Player_ClearVelocity(void *);
void Shadow_Spawn(void *obj, s16 type);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *ofs, f32 scale);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmGravios_ChangeState, EmGravios_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
s8 QuestEm_Appear(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_BeginStatus2(Em *em);
void Vec_Copy(f32 *out, f32 *in);
void Vec_Sub(f32 *out, f32 *a, f32 *b);
void Vec_Normalize(f32 *v);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void QuestEm_Hunted(Em *em);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
extern EmCurveKey *EmVespoid_VelYCurves[];
extern EmCurveKey *EmVespoid_SpeedCurves[];
void Vec_RotateY(f32 *v, f32 angle);
s32 Em_UpdateCombatTimer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
void EmVespoid_RunState(Em *em);
extern f32 gStartPos[];
void Player_Move(void *obj);
extern f32 EmVespoid_Scales[];
void GmEm_ApplySize(Em *em);
void Em_InitParts(Em *em);
/* end of prototypes */

/*
 * Plays the sounds of the current motion (work->fxMotion) at their frames
 * with Em_PlaySeAtBoneOnFrame: wing beats for VESP_MOT_HOVER, _FLY_FORWARD
 * and _FLY_BACK, random cries for VESP_MOT_IDLE and _IDLE_ACTION, and so on
 * for most motions. Takes
 * the monster as s32 and casts it back to Em* on every call.
 */
void EmVespoid_MotionSounds(s32 emAddr, EmWork *work)
{
    s16 motion;

    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, VESP_MOT_HOVER):
    case EM_MOTION_ID(0, VESP_MOT_FLY_FORWARD):
    case EM_MOTION_ID(0, VESP_MOT_FLY_BACK):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x2A, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x50, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x76, 0, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_IDLE):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, Sound_RandomSe(0xD, 1, 0xD, 1), 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_HOP):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 4, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x22, 0, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_HOP_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 5, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_WALK):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x3C, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x78, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0xB4, 1, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0xF0, 1, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_RUN):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 3, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x10, 3, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_LUNGE):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x24, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 5, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_STING):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 6, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x1C, 7, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x26, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x48, 0, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 5, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_AIR_FLINCH):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x44, 0, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_FALL):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 0xA, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_FALL_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 0xB, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 5, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_DIE_AIR):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 8, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_LIE):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 0xC, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_LAND):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x16, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x20, 2, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_FLINCH):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 9, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x14, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x10, 5, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_IDLE_ACTION):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, Sound_RandomSe(0xD, 1, 0xD, 1), 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_DIE):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 8, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_COLLAPSE):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 0xA, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0xC, 5, 0);
        break;
    case EM_MOTION_ID(0, VESP_MOT_GET_UP):
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 4, 5, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x32, 2, 0);
        Em_PlaySeAtBoneOnFrame((Em *) emAddr, 0x4C, 3, 0);
        
    default:
        break;
    }
}

/*
 * In quest stage 0 places the monster at the area start position, 150 apart
 * per slot; sets VESP_MOT_HOVER, EM_STANCE_GROUND, max hp from
 * Em_ComputeMaxHp(variant), scale from EmVespoid_Scales[variant],
 * scriptPending, blind 0; zeroes two work fields nothing reads, stores
 * home (posX, 0, posZ) and inits the parts.
 */
void EmVespoid_Init(Em *em)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;
    s16 maxHp;

    if (gQuest.stage == 0) {
        em->posX = 150.0f * (f32) (u32) em->slotNo + gStartPos[gScene.area * 3];
        em->posY = gStartPos[gScene.area * 3 + 1];
        em->posZ = gStartPos[gScene.area * 3 + 2];
    }
    Em_SetMotionRow(em, VESP_MOT_HOVER, 0, 0);
    em->stance = EM_STANCE_GROUND;
    maxHp = Em_ComputeMaxHp(em, em->variant);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scaleX = EmVespoid_Scales[em->variant];
    em->scaleY = EmVespoid_Scales[em->variant];
    em->scaleZ = EmVespoid_Scales[em->variant];
    em->bodyScale = em->scaleX;
    GmEm_ApplySize(em);
    em->scriptPending = 1;
    em->blind = 0;
    w->unk00C = 0;
    w->unk00E = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posZ;
    Em_InitParts(em);
    em->remote = 0;
}

/*
 * Init, then VESP_IDLE. Called by GmEm for Vespoid and Hornetaur.
 */
void EmVespoid_Start(Em *em)
{
    EmVespoid_Init(em);
    EmVespoid_ChangeState(em, EM_GROUP_IDLE, VESP_IDLE, EM_SYNC_LOCAL);
}

/*
 * Init only (callback slot 5 of EmVespoid_Callbacks).
 */
void EmVespoid_Reinit(Em *em)
{
    EmVespoid_Init(em);
}

/*
 * Per-state setup, then Em_SetStateSync. VESP_MOVE_WALK: with goalKind 0 the
 * goal becomes home and goalDist the XZ distance + 600, else the XZ distance
 * to the goal, less 50 for curGoalKind 1 arg 0 (if that is <= 0 it sets
 * scriptPending and returns without changing state). VESP_MOVE_RUN:
 * Em_SetSpeedScaled(2, goalDist, 50), timer 45-104 with goalKind 0, or
 * returns as above if goalDist <= 0. VESP_FLY_TO_GOAL: goalKind 0 drops the
 * goal and sets the timer, else raises the goal 20, scales speed, may return
 * as above, aims a velocity of 10 at the goal. VESP_FLY_DART: hasGoal =
 * goalKind != 0. VESP_FLY_HOME: goal = home + 20 up, homeDist = 3D distance,
 * velocity (0, 0, 10). VESP_ATK_CHARGE: Em_SetSpeedScaled(2, goalDist,
 * -100), timer/return as VESP_MOVE_RUN. VESP_DEFEAT_REAPPEAR: QuestEm_Appear
 * when QuestEm_CanAppear. Reached from Em_ChangeState in em_common.c
 * (scripts) and from this file.
 */
void EmVespoid_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;
    f32 toGoal[3];
    f32 trimmedDist;

    switch (group) {
    case EM_GROUP_MOVE:
        switch (state) {
        case VESP_MOVE_WALK:
        case VESP_MOVE_RUN:
            if (state == 0) {
                if (em->goalKind == 0) {
                    em->hasGoal = 1;
                    Vec_Copy(em->goal, w->home);
                    em->goalDist = 600.0f + Vec_DistanceXZ(&em->posX, em->goal);
                } else {
                    em->hasGoal = 1;
                    em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
                    if (em->curGoalKind == 1 && em->curGoalArg == 0) {
                        trimmedDist = em->goalDist - 50.0f;
                        em->goalDist = trimmedDist;
                        if (trimmedDist <= 0.0f) {
                            em->scriptPending = 1;
                            return;
                        }
                    }
                }
            } else {
                Em_SetSpeedScaled(em, 2, em->goalDist, 50.0f);
                if (em->goalKind == 0) {
                    em->timer = (em->stateRand % 120 + 90) / 2;
                } else if (em->goalDist <= 0.0f) {
                    em->scriptPending = 1;
                    return;
                }
            }
            break;
        }
        break;
    case EM_GROUP_FLY:
        switch (state) {
        case VESP_FLY_TO_GOAL:
            Player_ClearVelocity(em);
            if (em->goalKind == 0) {
                em->hasGoal = 0;
                em->timer = (em->stateRand % 120 + 90) / 2;
            } else {
                em->goal[1] += 20.0f;
                Em_SetSpeedScaled(em, 10, em->goalDist, 5.0f);
                if (em->curGoalKind == 1 && em->curGoalArg == 0 && em->goalDist <= 0.0f) {
                    em->scriptPending = 1;
                    return;
                }
                Vec_Sub(toGoal, em->goal, &em->posX);
                Vec_Normalize(toGoal);
                em->velX = 10.0f * toGoal[0];
                em->velY = 10.0f * toGoal[1];
                em->velZ = 10.0f * toGoal[2];
            }
            break;
        case VESP_FLY_DART:
            if (em->goalKind == 0) {
                em->hasGoal = 0;
            } else {
                em->hasGoal = 1;
            }
            break;
        case VESP_FLY_HOME:
            Player_ClearVelocity(em);
            Vec_Copy(em->goal, w->home);
            em->goal[1] += 20.0f;
            w->homeDist = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal);
            em->velX = 0.0f;
            em->velY = 0.0f;
            em->velZ = 10.0f;
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        switch (state) {
        case VESP_ATK_CHARGE:
            Em_SetSpeedScaled(em, 2, em->goalDist, -100.0f);
            if (em->goalKind == 0) {
                em->timer = (em->stateRand % 120 + 90) / 2;
            } else if (em->goalDist <= 0.0f) {
                em->scriptPending = 1;
                return;
            }
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_IDLE:
    case EM_GROUP_REACT:
    case EM_GROUP_EVENT:
    case EM_GROUP_EVENT2:
    case EM_GROUP_VERSUS:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Sets scriptPending and EM_STANCE_GROUND and goes to VESP_IDLE; the end of
 * every ground state.
 */
void EmVespoid_EndStateOnGround(Em *em)
{
    em->scriptPending = 1;
    em->stance = EM_STANCE_GROUND;
    EmVespoid_ChangeState(em, EM_GROUP_IDLE, VESP_IDLE, EM_SYNC_LOCAL);
}

/*
 * Sets scriptPending and EM_STANCE_AIR and goes to VESP_FLY_HOVER; the end
 * of every airborne state.
 */
void EmVespoid_EndStateInAir(Em *em)
{
    em->scriptPending = 1;
    em->stance = EM_STANCE_AIR;
    EmVespoid_ChangeState(em, EM_GROUP_FLY, VESP_FLY_HOVER, EM_SYNC_LOCAL);
}

/*
 * Empty state function (VESP_IDLE_NOP).
 */
void EmVespoid_IdleNop(void)
{
}

/*
 * Ground idle: EM_STANCE_GROUND, VESP_MOT_IDLE unless already playing, waits
 * 30/60/90/120 frames, then EmVespoid_EndStateOnGround (which re-enters it with a
 * script pending).
 */
void EmVespoid_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0x1E) + 0x1E;
        if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_IDLE)) {
            Em_SetMotion(em, VESP_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Plays VESP_MOT_IDLE_ACTION once on the ground (kept if already playing),
 * then EmVespoid_EndStateOnGround.
 */
void EmVespoid_IdleAction(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_IDLE_ACTION)) {
            Em_SetMotion(em, VESP_MOT_IDLE_ACTION, 0, 0);
        }
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Status 2 (Update on EM_DMG_STATUS2 after Em_SetStatus2Timer): hit-react
 * mode WAKE and blind 2 each frame, Em_BeginStatus2, VESP_MOT_COLLAPSE then
 * VESP_MOT_LIE (straight to the lie if VESP_MOT_FALL_LAND was playing),
 * lying puffs at bone 8 until the timer ends, then VESP_IDLE_STATUS2_END.
 */
void EmVespoid_Status2Lie(Em *em)
{
    f32 puffOfs[3];

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] == EM_MOTION_ID(0, VESP_MOT_FALL_LAND)) {
            Em_SetMotion(em, VESP_MOT_LIE, 0, 0);
            em->step = 2;
        } else {
            Em_SetMotion(em, VESP_MOT_COLLAPSE, 0, 0);
            em->step++;
        }
        Em_BeginStatus2(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        puffOfs[0] = 0.0f;
        puffOfs[1] = 0.0f;
        puffOfs[2] = 20.0f;
        Em_SpawnLyingPuffs(em, 8, puffOfs, 0.6f);
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_ChangeState(em, EM_GROUP_IDLE, VESP_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Em_EndStatus2, VESP_MOT_GET_UP with blind 2, then blind 0 and
 * EmVespoid_EndStateOnGround. Also entered on EM_DMG_HIT_LYING.
 */
void EmVespoid_Status2GetUp(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->blind = 2;
        Em_EndStatus2(em);
        Em_SetMotion(em, VESP_MOT_GET_UP, 0, 0);
        em->step++;
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Stores prevStateGroup and prevState (only this dispatcher does), then runs
 * the monster's EM_GROUP_IDLE state (enum EmVespoidIdleState) through its
 * state function.
 */
void EmVespoid_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case VESP_IDLE_NOP:
        EmVespoid_IdleNop();
        break;
    case VESP_IDLE:
        EmVespoid_Idle(em);
        break;
    case VESP_IDLE_ACTION:
        EmVespoid_IdleAction(em);
        break;
    case VESP_IDLE_STATUS2_LIE:
        EmVespoid_Status2Lie(em);
        break;
    case VESP_IDLE_STATUS2_END:
        EmVespoid_Status2GetUp(em);
        break;
    }
}

/*
 * VESP_MOT_WALK on the ground (motionSpeed 1.2 for EM_KIND_HORNETAUR);
 * Em_StepToGoal(0x40) until it arrives, or the timer without a goal, then
 * EmVespoid_EndStateOnGround.
 */
void EmVespoid_Walk(Em *em)
{
    if (em->type == EM_KIND_HORNETAUR) {
        em->motionSpeed = 1.2f;
    }
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, VESP_MOT_WALK, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                EmVespoid_EndStateOnGround(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                EmVespoid_EndStateOnGround(em);
            }
        }
        break;
    }
}

/*
 * As Walk with VESP_MOT_RUN and no kind speed-up; its EmVespoid_ChangeState setup
 * scales the speed to goalDist.
 */
void EmVespoid_Run(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, VESP_MOT_RUN, 0, 0);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                EmVespoid_EndStateOnGround(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                EmVespoid_EndStateOnGround(em);
            }
        }
        break;
    }
}

/*
 * Turns to the goal (0x71C a frame) during a hop in place: VESP_MOT_HOP
 * without root motion, VESP_MOT_HOP_LAND blended 10 from frame 16, then
 * clears the flag and EmVespoid_EndStateOnGround.
 */
void EmVespoid_HopTurn(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Player_SetFlag(em, EM_FLAG_NO_ROOT_MOTION);
        Em_SetMotion(em, VESP_MOT_HOP, 0, 0);
        break;
    case 1:
        Em_TurnToGoal(em, 0x71C, 0);
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_HOP_LAND, 0xA, 0);
        }
        break;
    case 2:
        Em_TurnToGoal(em, 0x71C, 0);
        if (em->motion[0].playing == 0) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            em->step++;
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_MOVE state (enum EmVespoidMoveState) through
 * its state function.
 */
void EmVespoid_RunMoveState(Em *em)
{
    switch (em->state) {
    case VESP_MOVE_WALK:
        EmVespoid_Walk(em);
        break;
    case VESP_MOVE_RUN:
        EmVespoid_Run(em);
        break;
    case VESP_MOVE_HOP_TURN:
        EmVespoid_HopTurn(em);
        break;
    }
}

/*
 * EM_STANCE_AIR, VESP_MOT_FLY_FORWARD, the curves VESP_CURVE_FLY_FORWARD (forward speed) for 100
 * frames, height kept between the floor and floor + 1000, then
 * EmVespoid_EndStateInAir. The second parameter is unused.
 */
void EmVespoid_FlyForward(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        em->timer = 0x64;
        Em_SetMotion(em, VESP_MOT_FLY_FORWARD, 0, 0);
        EmVespoid_StartCurves(em, VESP_CURVE_FLY_FORWARD);
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        if (--em->timer <= 0) {
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * VESP_MOT_FLY_BACK in the air: with variant 0 (VESP_FLY_BACK) the curves VESP_CURVE_FLY_BACK
 * (backward speed) for 100 frames; with variant 1 (VESP_FLY_DASH_BACK) motionSpeed 8
 * and the curves VESP_CURVE_DASH_BACK (short backward curve) for 10 frames, a quick dash back;
 * height clamped; then EmVespoid_EndStateInAir.
 */
void EmVespoid_FlyBack(Em *em, s16 dashBack)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Em_SetMotion(em, VESP_MOT_FLY_BACK, 0, 0);
        if (dashBack == 0) {
            em->timer = 100;
            EmVespoid_StartCurves(em, VESP_CURVE_FLY_BACK);
        } else {
            em->motionSpeed = 8.0f;
            em->timer = 10;
            EmVespoid_StartCurves(em, VESP_CURVE_DASH_BACK);
        }
        break;
    case 1:
        if (dashBack != 0) {
            em->motionSpeed = 8.0f;
        }
        EmVespoid_UpdateCurves(em);
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= 1000.0f + em->floorY)) {
            em->posY -= 20.0f;
        }
        if (--em->timer <= 0) {
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * Same code as EmVespoid_HopAttack: VESP_MOT_HOP with the curves
 * VESP_CURVE_HOP, airborne with VespoidAttack 8 from frame 6, VESP_MOT_HOP_LAND at
 * frame 32 back on the ground, then EmVespoid_EndStateOnGround. The second parameter
 * is unused.
 */
void EmVespoid_FlyHopAttack(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, VESP_MOT_HOP, 0, 0);
        EmVespoid_StartCurves(em, VESP_CURVE_HOP);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmVespoid_UpdateCurves(em);
            VespoidAttack_Create(em, 8);
        }
        break;
    case 2:
        EmVespoid_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_HOP_LAND, 0, 0);
            Player_ClearVelocity(em);
            em->stance = EM_STANCE_GROUND;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * EM_STANCE_AIR, velocity cleared, VESP_MOT_HOVER unless playing, 30-120
 * frames with the height clamped, then EmVespoid_EndStateInAir (back here with a
 * script pending).
 */
void EmVespoid_Hover(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Player_ClearVelocity(em);
        if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_HOVER)) {
            Em_SetMotion(em, VESP_MOT_HOVER, 0, 0);
        }
        em->timer = ((em->stateRand & 3) * 0x1E) + 0x1E;
        break;
    case 1:
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * VESP_MOT_HOP with the curves VESP_CURVE_HOP from the ground, airborne from frame 6,
 * VESP_MOT_HOVER blended 10 at frame 16, 10 more frames, then EmVespoid_EndStateInAir;
 * height clamped every frame.
 */
void EmVespoid_TakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, VESP_MOT_HOP, 0, 0);
        EmVespoid_StartCurves(em, VESP_CURVE_HOP);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmVespoid_UpdateCurves(em);
        }
        break;
    case 2:
        EmVespoid_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 16.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_HOVER, 0xA, 0);
            em->timer = 0xA;
        }
        break;
    case 3:
        EmVespoid_UpdateCurves(em);
        if (--em->timer <= 0) {
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
        return;
    }
    if (!(em->posY <= (1000.0f + em->floorY))) {
        em->posY -= 20.0f;
    }
}

/*
 * Hovers (VESP_MOT_HOVER) on the velocity EmVespoid_ChangeState aimed at the goal:
 * turns to the goal and counts goalDist down by 10 a frame (or the timer
 * without a goal), integrates, keeps above the floor, then EmVespoid_EndStateInAir;
 * the timer also ends it.
 */
void EmVespoid_FlyToGoal(Em *em)
{
    f32 goalDist;
    f32 floorY;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_HOVER)) {
            em->timer = ((Rand_Next(1) & 0xFFFF & 3) * 0x1E) + 0x1E;
            Em_SetMotion(em, VESP_MOT_HOVER, 0, 0);
        }
        break;
    case 1:
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x71C, 0);
            goalDist = em->goalDist - 10.0f;
            em->goalDist = goalDist;
            if (goalDist <= 0.0f) {
                em->step++;
                EmVespoid_EndStateInAir(em);
            }
        } else {
            if (--em->timer <= 0) {
                em->step++;
                EmVespoid_EndStateInAir(em);
            }
        }
        Player_Integrate(em);
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * Sinks at velY -5 with VESP_MOT_HOVER until below the floor, then
 * VESP_MOT_LAND on the floor, EM_STANCE_GROUND, then EmVespoid_EndStateOnGround.
 */
void EmVespoid_Land(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        em->velX = 0.0f;
        em->velY = -5.0f;
        em->velZ = 0.0f;
        em->accelX = 0.0f;
        em->accelY = 0.0f;
        em->accelZ = 0.0f;
        Em_SetMotion(em, VESP_MOT_HOVER, 0, 0);
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY < em->floorY) {
            em->step++;
            em->posY = em->floorY;
            Em_SetMotion(em, VESP_MOT_LAND, 0, 0);
            em->stance = EM_STANCE_GROUND;
            break;
        }
        if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * A random short dart in the air: velocity +-8 sideways with small random Y
 * and Z, rotated to rotY and copied to the acceleration,
 * VESP_MOT_FLY_FORWARD or VESP_MOT_FLY_BACK by the sign of Z; 3 frames out,
 * the acceleration reversed for 4-7 frames (Player_Move), 3 frames more,
 * turning to the goal if any, then EmVespoid_EndStateInAir.
 */
void EmVespoid_Dart(Em *em)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Player_ClearVelocity(em);
        if (em->stateRand & 1) {
            em->velX = 8.0f;
        } else {
            em->velX = -8.0f;
        }
        em->velY = (f32) ((em->stateRand & 7) - 4);
        em->velZ = (f32) (((em->stateRand >> 4) & 7) - 4);
        if (!(em->velZ < 0.0f)) {
            Em_SetMotion(em, VESP_MOT_FLY_FORWARD, 0, 0);
        } else {
            Em_SetMotion(em, VESP_MOT_FLY_BACK, 0, 0);
        }
        Vec_RotateY(&em->velX, 6.2831855f * em->rotY / 65536.0f);
        Vec_Copy(&em->accelX, &em->velX);
        em->timer = 3;
        w->dartLegFrames = em->timer;
        break;
    case 1:
        Player_Integrate(em);
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x71C, 0);
        }
        if (--em->timer <= 0) {
            em->timer = ((em->stateRand >> 1) & 3) + 4;
            em->accelX = -em->accelX;
            em->accelY = -em->accelY;
            em->accelZ = -em->accelZ;
            em->step++;
        }
        break;
    case 2:
        Player_Move(em);
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x71C, 0);
        }
        if (--em->timer <= 0) {
            em->step++;
            em->timer = 3;
        }
        break;
    case 3:
        Player_Integrate(em);
        if (em->hasGoal != 0) {
            Em_TurnToGoal(em, 0x71C, 0);
        }
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
        return;
    }
    if (!(em->posY <= (1000.0f + em->floorY))) {
        em->posY -= 20.0f;
    }
}

/*
 * VESP_MOT_FLY_FORWARD for 30-75 frames toward the goal EmVespoid_ChangeState set 20
 * above home: turns to it and moves by the velocity rotated to rotY, kept
 * above the floor, then EmVespoid_EndStateInAir.
 */
void EmVespoid_FlyHome(Em *em)
{
    f32 floorY;

    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0xF) + 0x1E;
        Em_SetMotion(em, VESP_MOT_FLY_FORWARD, 0, 0);
        break;
    case 1:
        Em_TurnToGoal(em, 0x71C, 0);
        EmVespoid_MoveByVelocity(em);
        floorY = em->floorY;
        if (em->posY < floorY) {
            em->posY = floorY;
        }
        if (--em->timer <= 0) {
            em->step++;
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_FLY state (enum EmVespoidFlyState) through its
 * state function.
 */
void EmVespoid_RunFlyState(Em *em)
{
    switch (em->state) {
    case 0:
        EmVespoid_FlyForward(em, 0);
        break;
    case 1:
        EmVespoid_FlyBack(em, 0);
        break;
    case 2:
        EmVespoid_FlyHopAttack(em, 0);
        break;
    case 3:
        EmVespoid_Hover(em);
        break;
    case 4:
        EmVespoid_TakeOff(em);
        break;
    case 5:
        EmVespoid_FlyToGoal(em);
        break;
    case 6:
        EmVespoid_Land(em);
        break;
    case 7:
        EmVespoid_FlyBack(em, 1);
        break;
    case 8:
        EmVespoid_Dart(em);
        break;
    case 9:
        EmVespoid_FlyHome(em);
        
    default:
        break;
    }
}

/*
 * Airborne lunge: VESP_MOT_LUNGE with the curves VESP_CURVE_LUNGE (fast forward),
 * VespoidAttack 6 made at frame 12 by EmVespoid_MotionEffects; height clamped;
 * EmVespoid_EndStateInAir when the motion ends.
 */
void EmVespoid_Lunge(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Em_SetMotion(em, VESP_MOT_LUNGE, 0, 0);
        EmVespoid_StartCurves(em, VESP_CURVE_LUNGE);
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        if (em->motion[0].playing == 0) {
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * Airborne sting: VESP_MOT_STING drifting forward at 2, at frame 34
 * VespoidAttack 7 (variant 1: 0xC, variant 2: 0x10); at the end clears
 * velocity, EmVespoid_EndStateInAir.
 */
void EmVespoid_StingA(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Em_SetMotion(em, VESP_MOT_STING, 0, 0);
        em->velX = 0.0f;
        em->velY = 0.0f;
        em->velZ = 2.0f;
        em->accelX = 0.0f;
        em->accelY = 0.0f;
        em->accelZ = 0.0f;
        Vec_RotateY(&em->velX, 6.2831855f * em->rotY / 65536.0f);
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= 1000.0f + em->floorY)) {
            em->posY -= 20.0f;
        }
        if (Motion_PassedFrame(em, 0, 34.0f) != 0) {
            switch (em->variant) {
            default:
                VespoidAttack_Create(em, 7);
                break;
            case 1:
                VespoidAttack_Create(em, 0xC);
                break;
            case 2:
                VespoidAttack_Create(em, 0x10);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            Player_ClearVelocity(em);
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * VESP_MOT_HOP with the curves VESP_CURVE_HOP, airborne with VespoidAttack 8 from frame
 * 6, VESP_MOT_HOP_LAND at frame 32 on the ground, EmVespoid_EndStateOnGround (same
 * code as EmVespoid_FlyHopAttack).
 */
void EmVespoid_HopAttack(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, VESP_MOT_HOP, 0, 0);
        EmVespoid_StartCurves(em, VESP_CURVE_HOP);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 6.0f, 0U) != 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            EmVespoid_UpdateCurves(em);
            VespoidAttack_Create(em, 8);
        }
        break;
    case 2:
        EmVespoid_UpdateCurves(em);
        if (EmMotion_PassedFrameScaled(em, 32.0f, 0U) != 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_HOP_LAND, 0, 0);
            Player_ClearVelocity(em);
            em->stance = EM_STANCE_GROUND;
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Ground charge: motionSpeed 1.3, VESP_MOT_RUN, VespoidAttack 0xB at the
 * start, Em_StepToGoal until it arrives (or the timer), then
 * EmVespoid_EndStateOnGround.
 */
void EmVespoid_Charge(Em *em)
{
    em->motionSpeed = 1.3f;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, VESP_MOT_RUN, 0, 0);
        VespoidAttack_Create(em, 0xB);
        break;
    case 1:
        if (em->hasGoal != 0) {
            if (Em_StepToGoal(em, 0x40, 1) == 1) {
                EmVespoid_EndStateOnGround(em);
                break;
            }
        } else {
            if (--em->timer <= 0) {
                EmVespoid_EndStateOnGround(em);
            }
        }
        break;
    }
}

/*
 * As EmVespoid_StingA with attack VESP_MOT_FALL_LAND (variant 1: 0xF, variant 2:
 * 0x11); nothing else tells the two apart.
 */
void EmVespoid_StingB(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Em_SetMotion(em, VESP_MOT_STING, 0, 0);
        em->velX = 0.0f;
        em->velY = 0.0f;
        em->velZ = 2.0f;
        em->accelX = 0.0f;
        em->accelY = 0.0f;
        em->accelZ = 0.0f;
        Vec_RotateY(&em->velX, 6.2831855f * em->rotY / 65536.0f);
        break;
    case 1:
        Player_Integrate(em);
        if (em->posY <= em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= 1000.0f + em->floorY)) {
            em->posY -= 20.0f;
        }
        if (Motion_PassedFrame(em, 0, 34.0f) != 0) {
            switch (em->variant) {
            default:
                VespoidAttack_Create(em, 0xE);
                break;
            case 1:
                VespoidAttack_Create(em, 0xF);
                break;
            case 2:
                VespoidAttack_Create(em, 0x11);
                break;
            }
        }
        if (em->motion[0].playing == 0) {
            Player_ClearVelocity(em);
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_ATTACK state (enum EmVespoidAttackState)
 * through its state function.
 */
void EmVespoid_RunAttackState(Em *em)
{
    switch (em->state) {
    case VESP_ATK_LUNGE:
        EmVespoid_Lunge(em);
        break;
    case VESP_ATK_STING_A:
        EmVespoid_StingA(em);
        break;
    case VESP_ATK_HOP:
        EmVespoid_HopAttack(em);
        break;
    case VESP_ATK_CHARGE:
        EmVespoid_Charge(em);
        break;
    case VESP_ATK_STING_B:
        EmVespoid_StingB(em);
        break;
    }
}

/*
 * Update on a part flinch or hit in the air: VESP_MOT_AIR_FLINCH,
 * Em_RestartScript, EmVespoid_EndStateInAir when it ends.
 */
void EmVespoid_AirFlinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Em_SetMotion(em, VESP_MOT_AIR_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndStateInAir(em);
        }
        break;
    }
}

/*
 * Update on EM_DMG_STATUS_IN_AIR: VESP_MOT_FALL with the curves VESP_CURVE_FALL until the
 * floor, then VESP_MOT_FALL_LAND on the ground and EmVespoid_EndStateOnGround.
 */
void EmVespoid_FallFromAir(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Em_SetMotion(em, VESP_MOT_FALL, 0, 0);
        Em_RestartScript(em);
        EmVespoid_StartCurves(em, VESP_CURVE_FALL);
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VESP_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
            break;
        }
        if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Update on EM_DMG_STATUS4 (after Em_SetStatus4Timer): Em_BeginStatus4,
 * noReact, hit-react mode STATUS; VESP_MOT_COLLAPSE then VESP_MOT_LIE
 * (straight to the lie after VESP_MOT_FALL_LAND), status-4 effect until the
 * timer, Em_EndStatus4 with VESP_MOT_GET_UP, then EmVespoid_EndStateOnGround.
 */
void EmVespoid_Status4(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        if (em->motionId[0] == EM_MOTION_ID(0, VESP_MOT_FALL_LAND)) {
            Em_SetMotion(em, VESP_MOT_LIE, 0, 0);
            em->step = 2;
        } else {
            Em_SetMotion(em, VESP_MOT_COLLAPSE, 0, 0);
            em->step++;
        }
        Em_RestartScript(em);
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
        break;
    case 1:
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_LIE, 0, 0);
        }
        break;
    case 2:
        Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_GET_UP, 0, 0);
            Em_EndStatus4(em);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Update on a part flinch or hit on the ground: VESP_MOT_FLINCH,
 * Em_RestartScript, EmVespoid_EndStateOnGround.
 */
void EmVespoid_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, VESP_MOT_FLINCH, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Falls like EmVespoid_FallFromAir (kept if VESP_MOT_FALL already plays), lands with
 * VESP_MOT_FALL_LAND, lies VESP_MOT_LIE 200 frames, VESP_MOT_GET_UP,
 * EmVespoid_EndStateOnGround. Not picked by Update; scripts only.
 */
void EmVespoid_FallAndLie(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_FALL)) {
            Em_SetMotion(em, VESP_MOT_FALL, 0, 0);
            EmVespoid_StartCurves(em, VESP_CURVE_FALL);
        }
        Em_RestartScript(em);
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VESP_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
            break;
        }
        if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY -= 20.0f;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_LIE, 0, 0);
            em->timer = 0xC8;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_GET_UP, 0, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * On the ground: VESP_MOT_COLLAPSE (kept if playing), VESP_MOT_LIE 200
 * frames, VESP_MOT_GET_UP, EmVespoid_EndStateOnGround. Not picked by Update; scripts
 * only.
 */
void EmVespoid_CollapseAndLie(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_COLLAPSE)) {
            Em_SetMotion(em, VESP_MOT_COLLAPSE, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_LIE, 0, 0);
            em->timer = 0xC8;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_GET_UP, 0, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmVespoid_EndStateOnGround(em);
        }
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_REACT state (enum EmVespoidReactState) through
 * its state function.
 */
void EmVespoid_RunReactState(Em *em)
{
    switch (em->state) {
    case VESP_REACT_AIR_FLINCH:
        EmVespoid_AirFlinch(em);
        break;
    case VESP_REACT_FALL:
        EmVespoid_FallFromAir(em);
        break;
    case VESP_REACT_STATUS4:
        EmVespoid_Status4(em);
        break;
    case VESP_REACT_FLINCH:
        EmVespoid_Flinch(em);
        break;
    case VESP_REACT_FALL_AND_LIE:
        EmVespoid_FallAndLie(em);
        break;
    case VESP_REACT_COLLAPSE_AND_LIE:
        EmVespoid_CollapseAndLie(em);
        
    default:
        break;
    }
}

/*
 * Out of combat; QuestEm_Hunted; VESP_MOT_DIE_AIR in the air or VESP_MOT_DIE
 * on the ground (kept if playing); hides the shadow, fades alpha by 1/60
 * from frame 100 (air) or 50, then invisible and VESP_DEFEAT_REAPPEAR.
 */
void EmVespoid_Die(Em *em)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        em->step++;
        if (em->stance == EM_STANCE_AIR) {
            if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_DIE_AIR)) {
                Em_SetMotion(em, VESP_MOT_DIE_AIR, 0, 0);
            }
        } else {
            if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_DIE)) {
                Em_SetMotion(em, VESP_MOT_DIE, 0, 0);
            }
        }
        break;
    case 1:
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        if (EmMotion_ReachedFrame(em, 0, (em->motionId[0] == EM_MOTION_ID(0, VESP_MOT_DIE_AIR)) ? 100 : 50) != 0) {
            em->alpha -= 1.0f / 60.0f;
            if (em->alpha <= 0.0f) {
                if (em->motion[0].playing == 0) {
                    em->visible = 0;
                    EmVespoid_ChangeState(em, EM_GROUP_DEFEAT, VESP_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
                }
                em->alpha = 0.0f;
            }
        }
        break;
    }
}

/*
 * Out of combat; QuestEm_Hunted; in the air falls (VESP_MOT_FALL, the curves
 * VESP_CURVE_FALL, VESP_MOT_FALL_LAND), on the ground VESP_MOT_COLLAPSE; then
 * VESP_MOT_LIE with a carving point (EmMine_Create, fieldCollide 0) until it
 * is used up or 900 frames, removes it, fades out, then
 * VESP_DEFEAT_REAPPEAR.
 */
void EmVespoid_DieCarvable(Em *em)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        if (em->stance == EM_STANCE_AIR) {
            em->step++;
            if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_FALL)) {
                Em_SetMotion(em, VESP_MOT_FALL, 0, 0);
                EmVespoid_StartCurves(em, VESP_CURVE_FALL);
            }
        } else {
            em->step = 2;
            if (em->motionId[0] != EM_MOTION_ID(0, VESP_MOT_COLLAPSE)) {
                Em_SetMotion(em, VESP_MOT_COLLAPSE, 0, 0);
            }
        }
        break;
    case 1:
        EmVespoid_UpdateCurves(em);
        if (em->posY <= em->floorY) {
            em->step++;
            em->posY = em->floorY;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, VESP_MOT_FALL_LAND, 0, 0);
            Player_ClearVelocity(em);
        } else if (!(em->posY <= em->floorY + 1000.0f)) {
            em->posY -= 20.0f;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, VESP_MOT_LIE, 0, 0);
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
            em->timer = 900;
        }
        break;
    case 3:
        if (EmMine_Update(em) <= 0 || --em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 4:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmVespoid_ChangeState(em, EM_GROUP_DEFEAT, VESP_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
            em->alpha = 0.0f;
        }
        break;
    }
}

/*
 * If QuestEm_CanAppear: 300 higher, alpha 0, motion row VESP_MOT_FLY_BACK,
 * to VESP_ENTER_FLY_IN; otherwise taskPhase++ and it waits.
 */
void EmVespoid_Reappear(Em *em)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY += 300.0f;
            em->alpha = 0.0f;
            Em_SetMotionRow(em, VESP_MOT_FLY_BACK, 0, 0);
            EmVespoid_ChangeState(em, EM_GROUP_EVENT2, VESP_ENTER_FLY_IN, EM_SYNC_LOCAL);
        } else {
            em->taskPhase++;
        }
        break;
    case 1:
        break;
    }
}

/*
 * Keeps noHitTimer and noPushTimer at 10, then runs the monster's
 * EM_GROUP_DEFEAT state (enum EmVespoidDefeatState) through its state
 * function.
 */
void EmVespoid_RunDefeatState(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case VESP_DEFEAT_DIE:
        EmVespoid_Die(em);
        break;
    case VESP_DEFEAT_DIE_CARVABLE:
        EmVespoid_DieCarvable(em);
        break;
    case VESP_DEFEAT_REAPPEAR:
        EmVespoid_Reappear(em);
        break;
    }
}

/*
 * Event: senseDelay and noHitTimer 5; one frame invisible on the ground with
 * VESP_MOT_HOVER, lookAtTarget and scriptPending 0, then visible and
 * EmVespoid_EndStateOnGround.
 */
void EmVespoid_ShowA(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VESP_MOT_HOVER, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmVespoid_EndStateOnGround(em);
        break;
    }
}

/*
 * Same code as EmVespoid_ShowA.
 */
void EmVespoid_ShowB(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, VESP_MOT_HOVER, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmVespoid_EndStateOnGround(em);
        break;
    }
}

/*
 * Runs the monster's EM_GROUP_EVENT state (enum EmVespoidEventState) through
 * its state function.
 */
void EmVespoid_RunEventState(Em *em)
{
    switch (em->state) {
    case VESP_EVENT_SHOW_A:
        EmVespoid_ShowA(em);
        break;
    case VESP_EVENT_SHOW_B:
        EmVespoid_ShowB(em);
        break;
    }
}

/*
 * After Reappear: motion row VESP_MOT_FLY_BACK, EM_STANCE_AIR, velocity
 * cleared, waits 150 frames, then sinks at velY -10 for 30 frames while
 * alpha rises by 1/30 (height clamped), then senseDelay 0, alpha 1 and
 * EmVespoid_EndStateInAir.
 */
void EmVespoid_FlyIn(Em *em)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        Em_SetMotionRow(em, VESP_MOT_FLY_BACK, 0, 0);
        em->stance = EM_STANCE_AIR;
        Player_ClearVelocity(em);
        em->timer = 0x96;
        
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->velY = -10.0f;
            em->timer = 0x1E;
        }
        break;
    case 2:
        em->alpha += 0.033333335f;
        if (!(em->alpha <= 1.0f)) {
            em->alpha = 1.0f;
        }
        Player_Integrate(em);
        if (em->posY < em->floorY) {
            em->posY = em->floorY;
        } else if (!(em->posY <= (1000.0f + em->floorY))) {
            em->posY = em->posY - 20.0f;
        }
        if (--em->timer <= 0) {
            em->timer = 0;
            em->step++;
            em->senseDelay = 0;
            EmVespoid_EndStateInAir(em);
            em->alpha = 1.0f;
        }
        break;
    }
}

/*
 * Keeps noHitTimer and noPushTimer at 10 and senseDelay 5, then runs the
 * monster's EM_GROUP_EVENT2 state (enum EmVespoidEnterState) through its
 * state function.
 */
void EmVespoid_RunEnterState(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->state) {
    case 0:
        EmVespoid_FlyIn(em);
        break;
    }
}

/*
 * The family's update, each frame: motionSpeed 1, the combat timer, then
 * Em_UpdateDamage's result picks the state:
 * - EM_DMG_KILLED (outside EM_GROUP_DEFEAT): VESP_DEFEAT_DIE_CARVABLE one
 *   time in 20, else VESP_DEFEAT_DIE; EM_DMG_DEAD: VESP_DEFEAT_DIE_CARVABLE;
 * - EM_DMG_STATUS_IN_AIR: VESP_REACT_FALL; EM_DMG_STATUS4:
 *   VESP_REACT_STATUS4; EM_DMG_STATUS2: VESP_IDLE_STATUS2_LIE;
 * - EM_DMG_HIT_LYING, in VESP_IDLE_STATUS2_LIE: VESP_IDLE_STATUS2_END;
 * - EM_DMG_PART_FLINCH and EM_DMG_HIT: VESP_REACT_AIR_FLINCH in the air,
 *   else VESP_REACT_FLINCH;
 * - EM_DMG_STATUS5: only Em_EndStatus5 (the family has no stun state);
 * - the rest are ignored.
 * Then the pending script, Em_ClearHitReactMode and EmVespoid_RunState
 * (twice when a state was requested).
 */
void EmVespoid_Update(Em *em)
{
    u8 dmgInfo[4];

    em->motionSpeed = 1.0f;
    Em_UpdateCombatTimer(em);
    switch (Em_UpdateDamage(em, dmgInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_CAPTURED:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT_MODE3:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
        if (em->stateGroup != EM_GROUP_DEFEAT) {
            if (em->stateRand % 20 == 0) {
                EmVespoid_ChangeState(em, EM_GROUP_DEFEAT, VESP_DEFEAT_DIE_CARVABLE, EM_SYNC_OWNER);
            } else {
                EmVespoid_ChangeState(em, EM_GROUP_DEFEAT, VESP_DEFEAT_DIE, EM_SYNC_OWNER);
            }
        }
        em->scriptPending = 0;
        break;
    case EM_DMG_DEAD:
        if (em->stateGroup != EM_GROUP_DEFEAT) {
            EmVespoid_ChangeState(em, EM_GROUP_DEFEAT, VESP_DEFEAT_DIE_CARVABLE, EM_SYNC_OWNER);
        }
        em->scriptPending = 0;
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || em->state != VESP_REACT_FALL) {
            EmVespoid_ChangeState(em, EM_GROUP_REACT, VESP_REACT_FALL, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if (em->stateGroup != EM_GROUP_REACT || em->state != VESP_REACT_STATUS4) {
            Em_SetStatus4Timer(em);
            EmVespoid_ChangeState(em, EM_GROUP_REACT, VESP_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS2:
        if (em->stateGroup != EM_GROUP_IDLE || em->state != VESP_IDLE_STATUS2_LIE) {
            Em_SetStatus2Timer(em);
            EmVespoid_ChangeState(em, EM_GROUP_IDLE, VESP_IDLE_STATUS2_LIE, EM_SYNC_OWNER);
            em->scriptPending = 0;
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case VESP_IDLE_STATUS2_LIE:
            EmVespoid_ChangeState(em, EM_GROUP_IDLE, VESP_IDLE_STATUS2_END, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
        break;
    case EM_DMG_PART_FLINCH:
    case EM_DMG_HIT:
        if (em->stance == EM_STANCE_AIR) {
            EmVespoid_ChangeState(em, EM_GROUP_REACT, VESP_REACT_AIR_FLINCH, EM_SYNC_OWNER);
        } else {
            EmVespoid_ChangeState(em, EM_GROUP_REACT, VESP_REACT_FLINCH, EM_SYNC_OWNER);
        }
        em->scriptPending = 0;
        break;
    case EM_DMG_STATUS5:
        Em_EndStatus5(em);
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmVespoid_RunState(em);
    if (em->stateRequest != 0) {
        EmVespoid_RunState(em);
        em->stateRequest = 0;
    }
}

/*
 * Runs the dispatcher of stateGroup 0-7.
 */
void EmVespoid_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmVespoid_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmVespoid_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmVespoid_RunFlyState(em);
        break;
    case EM_GROUP_ATTACK:
        EmVespoid_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmVespoid_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmVespoid_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmVespoid_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmVespoid_RunEnterState(em);
        
    default:
        break;
    }
}

/*
 * Keeps motionId[0] in work->fxMotion, plays EmVespoid_MotionSounds, and makes
 * VespoidAttack 6 at frame 12 of VESP_MOT_LUNGE; the other rows do nothing.
 */
void EmVespoid_MotionEffects(Em *em, EmWork *work)
{
    s16 motion;
    u16 motionId;

    motionId = em->motionId[0];
    if (motionId != work->fxMotion) {
        work->fxMotion = (s16) motionId;
    }
    EmVespoid_MotionSounds((s32) em, work);
    motion = work->fxMotion;
    switch (motion) {
    case EM_MOTION_ID(0, VESP_MOT_HOVER):
    case EM_MOTION_ID(0, VESP_MOT_FLY_FORWARD):
    case EM_MOTION_ID(0, VESP_MOT_FLY_BACK):
    case EM_MOTION_ID(0, 0x4):
    case EM_MOTION_ID(0, VESP_MOT_IDLE):
    case EM_MOTION_ID(0, VESP_MOT_HOP):
    case EM_MOTION_ID(0, VESP_MOT_HOP_LAND):
    case EM_MOTION_ID(0, VESP_MOT_WALK):
    case EM_MOTION_ID(0, VESP_MOT_RUN):
    case EM_MOTION_ID(0, VESP_MOT_STING):
    case EM_MOTION_ID(0, VESP_MOT_AIR_FLINCH):
    case EM_MOTION_ID(0, VESP_MOT_FALL):
    case EM_MOTION_ID(0, VESP_MOT_FALL_LAND):
    case EM_MOTION_ID(0, VESP_MOT_DIE_AIR):
    case EM_MOTION_ID(0, VESP_MOT_LIE):
    case EM_MOTION_ID(0, VESP_MOT_LAND):
    case EM_MOTION_ID(0, VESP_MOT_FLINCH):
    case EM_MOTION_ID(0, VESP_MOT_IDLE_ACTION):
    case EM_MOTION_ID(0, VESP_MOT_DIE):
    case EM_MOTION_ID(0, VESP_MOT_COLLAPSE):
    case EM_MOTION_ID(0, VESP_MOT_GET_UP):
        break;
    case EM_MOTION_ID(0, VESP_MOT_LUNGE):
        if (Motion_PassedFrame(em, 0, 12.0f) != 0) {
            VespoidAttack_Create(em, 6);
        }
        break;
    }
}

/*
 * Callback slot 3: skips the first frame (fxStep 0 to 1), then EmVespoid_MotionEffects
 * every frame.
 */
void EmVespoid_UpdateMotionEffects(Em *em)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmVespoid_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Callback slots 0 and 1: Shadow_Spawn(em, 0). Takes the monster as u8*.
 */
void EmVespoid_SpawnEffect01(u8 *obj)
{
    Shadow_Spawn((Player *) obj, 0);
}

/*
 * Empty callback, slot 2.
 */
void EmVespoid_TableNop(void)
{
}

/*
 * Selects the curve set kind (VESP_CURVE_*): runs the velY and speed curves
 * from the kind's start frame, and clears the velocity.
 */
void EmVespoid_StartCurves(Em *em, u8 kind)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case VESP_CURVE_FLY_FORWARD:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0xA;
        break;
    case VESP_CURVE_FLY_BACK:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0xA;
        break;
    case VESP_CURVE_LUNGE:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x4;
        break;
    case VESP_CURVE_HOP:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x6;
        break;
    case VESP_CURVE_FALL:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x6;
        break;
    case VESP_CURVE_UNUSED:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case VESP_CURVE_DASH_BACK:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x3;
        break;
    }
    Player_ClearVelocity(em);
}

/*
 * Stub curve: returns 0 (never enabled: EmVespoid_StartCurves clears curveNone).
 */
static s32 EmVespoid_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

/*
 * Sets velY from EmVespoid_VelYCurves[curveKind] at curveFrame (the first
 * key value on frames 0 and 1, else the slope of the key segment over the
 * motion step). Returns 2 once past the last key, 0 otherwise or when the
 * kind has no table.
 */
static u8 EmVespoid_CurveVelY(Em *em, EmWork *work)
{
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *curve;
    u8 endFlag;

    curve = EmVespoid_VelYCurves[((EmVespoidWork *)work)->curveKind];
    endFlag = 0;
    if (curve == NULL) {
        return endFlag;
    }
    frame = ((EmVespoidWork *)work)->curveFrame;
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
                endFlag = 2;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return endFlag;
}

/*
 * The same for velZ from EmVespoid_SpeedCurves. Returns 4 once past the last
 * key, else 0.
 */
static u8 EmVespoid_CurveSpeed(Em *em, EmWork *work)
{
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *curve;
    u8 endFlag;

    curve = EmVespoid_SpeedCurves[((EmVespoidWork *)work)->curveKind];
    endFlag = 0;
    if (curve == NULL) {
        return endFlag;
    }
    frame = ((EmVespoidWork *)work)->curveFrame;
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
                endFlag = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return endFlag;
}

/*
 * Runs the enabled curves, advances curveFrame by the motion step, and in
 * EM_STANCE_AIR moves the monster with Em_MoveRotatedAccel at angles (0,
 * rotY, 0). Returns the curve end flags ORed (2 velY ended, 4 speed ended);
 * no caller reads it.
 */
u8 EmVespoid_UpdateCurves(Em *em)
{
    EmVespoidWork *w = (EmVespoidWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmVespoid_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmVespoid_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmVespoid_CurveSpeed(em, (EmWork *) w);
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

/*
 * Adds the velocity, rotated by rotY, to the position (used by EmVespoid_FlyHome).
 */
void EmVespoid_MoveByVelocity(Em *em)
{
    f32 worldVel[3];

    Vec_Copy(worldVel, &em->velX);
    Vec_RotateY(worldVel, 6.2831855f * em->rotY / 65536.0f);
    em->posX += worldVel[0];
    em->posY += worldVel[1];
    em->posZ += worldVel[2];
}

/* The file's initialised data, in the game's order */
s32 GmEm_ReturnZero(void); /* gm_sub */
f32 EmVespoid_Scales[3] = {
#include "game/008C5370.inc"
};
void *EmVespoid_Callbacks[8] = {
    EmVespoid_SpawnEffect01, EmVespoid_SpawnEffect01, EmVespoid_TableNop, EmVespoid_UpdateMotionEffects,
    GmEm_ReturnZero, EmVespoid_Reinit, NULL, NULL,
};
EmCurveKey EmVespoid_VelYCurves_0[10] = {
#include "game/008C53A0.inc"
};
EmCurveKey EmVespoid_VelYCurves_2[14] = {
#include "game/008C53F0.inc"
};
EmCurveKey EmVespoid_VelYCurves_3[10] = {
#include "game/008C5460.inc"
};
EmCurveKey EmVespoid_VelYCurves_4[10] = {
#include "game/008C54B0.inc"
};
EmCurveKey EmVespoid_VelYCurves_5[8] = {
#include "game/008C5500.inc"
};
EmCurveKey *EmVespoid_VelYCurves[8] = {
    EmVespoid_VelYCurves_0, EmVespoid_VelYCurves_0, EmVespoid_VelYCurves_2, EmVespoid_VelYCurves_3,
    EmVespoid_VelYCurves_4, EmVespoid_VelYCurves_5, EmVespoid_VelYCurves_0, NULL,
};
EmCurveKey EmVespoid_SpeedCurves_0[8] = {
#include "game/008C5560.inc"
};
EmCurveKey EmVespoid_SpeedCurves_1[8] = {
#include "game/008C55A0.inc"
};
EmCurveKey EmVespoid_SpeedCurves_2[14] = {
#include "game/008C55E0.inc"
};
EmCurveKey EmVespoid_SpeedCurves_3[10] = {
#include "game/008C5650.inc"
};
EmCurveKey EmVespoid_SpeedCurves_4[10] = {
#include "game/008C56A0.inc"
};
EmCurveKey EmVespoid_SpeedCurves_5[8] = {
#include "game/008C56F0.inc"
};
EmCurveKey EmVespoid_SpeedCurves_6[8] = {
#include "game/008C5730.inc"
};
EmCurveKey *EmVespoid_SpeedCurves[8] = {
    EmVespoid_SpeedCurves_0, EmVespoid_SpeedCurves_1, EmVespoid_SpeedCurves_2, EmVespoid_SpeedCurves_3,
    EmVespoid_SpeedCurves_4, EmVespoid_SpeedCurves_5, EmVespoid_SpeedCurves_6, NULL,
};
