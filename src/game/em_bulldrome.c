/*
 * The Bulldrome (EM_KIND_BULLDROME), from its first function,
 * EmBulldrome_Init, to the next family's code. EmBulldrome_Setup, called by
 * the per-kind dispatch Em_TypeSetup, sets a monster up; EmBulldrome_Update
 * runs it every frame: it turns Em_UpdateDamage's result into a reaction,
 * starts the opening scene of its jungle quests, and runs the current state
 * through EmBulldrome_RunState. EmBulldrome_ChangeState moves to a state,
 * through the groups' setters (EmBulldrome_SetIdleState ...).
 *
 * Each state function takes the monster and its work area (EmWork; the
 * Bulldrome's own meanings are EmBulldromeWork); Em.step is the step within
 * the state. em_bulldrome.h names the motions (BULL_MOT_*) and the states of
 * each group; the travel between goals takes the flyers' group
 * (EM_GROUP_FLY). The charges create the Bulldrome's attack objects
 * (BulldromeAttack_Create), and a knock-back launches the monster through
 * the air (EmBulldrome_LaunchFromHit).
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_bulldrome.h"
#include "player.h"
#include "objattach.h"
#include "lobby_vobj.h"
#include "area.h"

/* The game built this file at optimization level 4; the pragma does the same. */
#pragma optimization_level 4





struct EmHolder;
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndAllStatus(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
s8 QuestEm_Appear(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmBulldrome_ChangeState, EmBulldrome_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_StartRage(Em *);
void Em_StartPendingRage(Em *);
void Em_RefillNeed3(Em *em);
void Em_ResetNotice(Em *em);
void Em_TargetPlayerInRange(Em *);
void EmBulldrome_SetMoveState(Em *em, u16 state, u8 mode);
void EmBulldrome_SetTravelState(Em *em, u16 state, u8 mode);
void EmBulldrome_SetAttackState(Em *em, u16 state, u8 mode);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
u16 Area_DayId(u16 id);
void Versus_MarkDefeat(Em *em);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Shadow_Spawn(void *obj, s16 type);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Vec_Copy(f32 *dst, f32 *src);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 Game_IsOnline(void);
void Em_ResetToBasePose(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_CheckRageByHp(Em *em, s16 n);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
void Em_SetTargetAngle(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
void Em_StartGoalAction(Em *em, s32 arg1);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_ClearPart(Em *em, u16 part);
void Em_AddHp(Em *em, s16 n);
void Em_AddNeed1(Em *em, s32 n);
void Em_AddNeed2(Em *em, s32 n);
void Em_ClearNeed1State(Em *em);
void Em_ClearNeed2State(Em *em);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void EmBulldrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
f32 *Mtx_FromAnglesXYZ(s32 *ang, f32 *mtx);
extern f32 EmBulldrome_LaunchVel[];
extern f32 EmBulldrome_LaunchAccel[];
struct EmBone;
struct MtxSlot;
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
ObjAttach *BulldromeAttack_Create(Em *em, s8 kind);
extern f32 EmBulldrome_Effect0BOfs[3];
void EmBulldrome_LaunchFromHit(Em *);
void EmBulldrome_SetKnockedMotion(Em *em, s16 arg1);
extern EmTurnParams EmBulldrome_TurnParams[1];
s8 EmMine_Create(Em *em, s32 idx);
void EmMine_Remove(Em *em);
s32 EmMine_Update(Em *em);
s32 EmMotion_InFrameRange(void *em, s32 layer, f32 from, f32 to);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
void Em_CheckLowHp(Em *em, f32 ratio);
void Em_ClearHitReactMode(Em *em);
void Em_ClearLowHpNeed(Em *em);
void Em_ClearPoison(Em *em);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void Em_HoldPosNoPlayers(Em *em, u16 a, u16 b);
void Em_InitParts(Em *em);
void Em_RaisePlayerAreaTrigger(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's s32 (Em *em, s16 n, s16 max, s16 interval), the calls in EmBulldrome_LieDown and EmChameleos_Rest compile differently */
s32 Em_RegenHp(Em *em, s32 n, s32 max, s32 interval);
void Em_ResetKindTimer(Em *em);
void Em_RestartScript(Em *em);
void Em_SetCaptured(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_SetHold2(Em *em);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
void Em_SetStatus2Timer(Em *em);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
u8 Em_TurnToGoal(Em *em, u16 step, u8 flags);
s32 Em_UpdateCombatTimer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void GmEm_ResetSize(Em *em);
u16 Map_DayId(u16 area);
void Motion_InitTime(void *p, s16 step);
f32 *Mtx_FromAnglesYXZ(s32 *angles, f32 *m);
void Player_ClearFlag(void *, u32 flag);
s32 Player_MoveFall(Player *p);
void Player_SetFlag(void *obj, u32 flag);
void QuestEm_Captured(Em *em);
void QuestEm_Hunted(Em *em);
s32 Save_TestEventFlag(s32);
s32 ScEvent_CurId(void);
void StunStars_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale);
void VersusBait_Create(Player *p, Em *em, u8 k, s32 unused, f32 *pos);
s32 Versus_BothDefeated(void);
s32 Versus_DefeatCamera(Em *pl);
void Versus_RouteEmByTeam(Em *em);
void EmBulldrome_Init(Em *em);
void EmBulldrome_LieDown(Em *, s32);
void EmBulldrome_TurnInPlace(Em *, s32);
void EmBulldrome_WanderHome(Em *em);
void EmBulldrome_QuarterTurn(Em *em, u8 variant);
void EmBulldrome_HalfTurn(Em *em, u8 variant);
void EmBulldrome_TravelTurn(Em *);
void EmBulldrome_TravelToGoal(Em *);
void EmBulldrome_Spin(Em *);
void EmBulldrome_Die(Em *);
void EmBulldrome_DieInAir(Em *);
void EmBulldrome_Captured(Em *);
void EmBulldrome_IntroScene(Em *);
void EmBulldrome_RunVersusState(Em *em);
void EmBulldrome_VsEatBait(Em *, EmWork *);
void EmBulldrome_EndState(Em *em);
void EmBulldrome_RunState(Em *em);
/* A position copied whole: the versus bait's position is copied from
 * EmBulldrome_CarveThrowPt as one struct, as the game's code does. */
extern Vec3f EmBulldrome_CarveThrowPt __attribute__((aligned(8)));
/* end of prototypes */


/*
 * Sets the monster up: on quest stage 0 a fixed spot by
 * slotNo (Snowy Mountains night area 9 grid, else a row at x 5000), idle
 * motion row, ground stance, hp/hpMax from Em_ComputeMaxHp, scriptPending,
 * clears the work's countdown, keeps the start position as home,
 * Em_InitParts.
 */
void EmBulldrome_Init(Em *em)
{
    EmBulldromeWork *emWork = (EmBulldromeWork *)&em->work;

    f32 slotF;
    s16 maxHp;
    u8 slotNo;

    if (gQuest.stage == 0) {
        if (gScene.area == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
        } else {
            slotNo = em->slotNo;
            slotF = (f32) (u32) slotNo;
            em->posX = 5000.0f + (200.0f * slotF);
            em->posY = 0.0f;
            em->posZ = 5000.0f;
        }
    }
    Em_SetMotionRow(em, BULL_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    maxHp = Em_ComputeMaxHp(em, (s16) em->variant);
    em->hp = maxHp;
    em->hpMax = maxHp;
    em->scriptPending = 1;
    em->blind = 0;
    em->unk_InitFlag = 1;
    emWork->countdown = 0;
    emWork->unk00E = 0;
    emWork->home[0] = em->posX;
    emWork->home[1] = 0.0f;
    emWork->home[2] = (f32) em->posZ;
    Em_InitParts(em);
}

/*
 * Init, then BULL_IDLE.
 */
void EmBulldrome_Start(Em *em)
{
    EmBulldrome_Init(em);
    EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE, EM_SYNC_LOCAL);
}

/*
 * Init only (callback slot 5 of
 * EmBulldrome_Callbacks).
 */
void EmBulldrome_Reinit(Em *em)
{
    EmBulldrome_Init(em);
}

/*
 * Ends any state: motionSpeed 1, ground stance,
 * lookAtTarget off, fieldCollide on, scriptPending, then BULL_IDLE (mode
 * EM_SYNC_LOCAL).
 */
void EmBulldrome_EndState(Em *em)
{
    em->motionSpeed = 1.0f;
    em->stance = EM_STANCE_GROUND;
    em->lookAtTarget = 0;
    em->fieldCollide = 1;
    em->scriptPending = 1;
    EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE, EM_SYNC_LOCAL);
}

/*
 * BULL_IDLE_NOP_A, empty.
 */
void EmBulldrome_IdleNopA(void)
{
}

/*
 * BULL_IDLE: BULL_MOT_IDLE once, then EmBulldrome_EndState (which
 * comes back here until the script picks another state).
 */
void EmBulldrome_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_IDLE, 0xA, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_IDLE_NOP_B, empty.
 */
void EmBulldrome_IdleNopB(void)
{
}

/*
 * BULL_IDLE_FORAGE: BULL_MOT_FORAGE from motionTime 0
 * until motionTime reaches 240, then EmBulldrome_EndState.
 */
void EmBulldrome_Forage(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_FORAGE, 6, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        if (em->motionTime >= 0xF0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_IDLE_AIM_SHORT/MEDIUM/LONG: BULL_MOT_AIM
 * while Em_TurnToGoal turns 0x200 a frame; loops counts down once per frame
 * the motion is not playing, then EmBulldrome_EndState.
 */
void EmBulldrome_AimAtGoal(Em *em, s32 loops)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->timer = loops;
        Em_SetMotion(em, BULL_MOT_AIM, 6, 0);
        break;
    case 1:
        Em_TurnToGoal(em, 0x200, 0);
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                EmBulldrome_EndState(em);
            }
        }
        break;
    }
}

/*
 * BULL_IDLE_NOP_C, empty.
 */
void EmBulldrome_IdleNopC(void)
{
}

/*
 * BULL_IDLE_DISPLAY, _DISPLAY_B, _DISPLAY_RAGE,
 * _DISPLAY_PENDING_RAGE: BULL_MOT_THREAT once, then EmBulldrome_EndState; the second
 * argument is unused (the setter does the rage).
 */
void EmBulldrome_Display(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_THREAT, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_IDLE_SHAKE_OFF: BULL_MOT_SHAKE_OFF once
 * after a lie get-up, then EmBulldrome_EndState.
 */
void EmBulldrome_ShakeOff(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_SHAKE_OFF, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_IDLE_STATUS2_LIE (kind 0),
 * BULL_IDLE_STATUS1_LIE (kind 1), BULL_IDLE_REST (kind 2): hitReactMode WAKE
 * and blind every frame, BULL_MOT_LIE with lying puffs at bone 0xB; the rest
 * sets 5400 frames and regains 1 hp every 10 frames up to 60%; at the
 * timer's end (rest: Em_ClearLowHpNeed) goes to STATUS2_END, STATUS1_END or
 * REST_END; step 2 repeats that change on the owner.
 */
void EmBulldrome_LieDown(Em *em, s32 lieKind)
{
    s32 kindAgain;
    s32 kind;

    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_LIE, 0, 0);
        if ((lieKind & 0xFFFF) == 2) {
            em->timer = 0x1518;
        }
        break;
    case 1:
        Em_SpawnLyingPuffs(em, 0xB, (f32 *) &EmBulldrome_Effect0BOfs, 1.0f);
        kind = lieKind & 0xFFFF;
        if (kind == 2) {
            Em_RegenHp(em, 1, (s32) (0.6f * (f32) em->hpMax), 0xA);
        }
        if (--em->timer <= 0) {
            em->step++;
            if (kind == 2) {
                Em_ClearLowHpNeed(em);
            }
            switch (kind) {
            case 0:
                EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            case 1:
                EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
                break;
            case 2:
                EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    case 2:
        kindAgain = lieKind & 0xFFFF;
        if (em->remote == 0) {
            switch (kindAgain) {
            case 0:
                EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
                break;
            case 1:
                EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS1_END, EM_SYNC_OWNER_FLAGGED);
                break;
            case 2:
                EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_REST_END, EM_SYNC_OWNER_FLAGGED);
                break;
            }
        }
        break;
    }
}

/*
 * BULL_IDLE_STATUS2_END, STATUS1_END, REST_END:
 * BULL_MOT_LIE_GET_UP (blind 2 while it plays), then BULL_IDLE_SHAKE_OFF.
 */
void EmBulldrome_LieGetUp(Em *em, s32 unused)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->blind = 2;
        Em_SetMotion(em, BULL_MOT_LIE_GET_UP, 0, 0);
        break;
    case 1:
        em->blind = 2;
        if (em->motion[0].playing == 0) {
            em->step++;
            em->blind = 0;
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_SHAKE_OFF, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BULL_IDLE_HOLD2_GET_UP, STATUS4_GET_UP,
 * STUN_GET_UP: guardTimer 2, noReact, rotY turned half round,
 * BULL_MOT_DOWN_GET_UP, then EmBulldrome_EndState.
 */
void EmBulldrome_DownGetUp(Em *em)
{
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_DOWN_GET_UP, 0, 0);
        em->noReact = 1;
        em->rotY = em->rotY + 0x7FFF + 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * Group 0 dispatcher; stores
 * prevStateGroup/prevState, then runs em->state's function with its loop
 * count or lie kind (table in the brief).
 */
void EmBulldrome_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case BULL_IDLE_NOP_A:
        EmBulldrome_IdleNopA();
        break;
    case BULL_IDLE:
        EmBulldrome_Idle(em);
        break;
    case BULL_IDLE_NOP_B:
        EmBulldrome_IdleNopB();
        break;
    case BULL_IDLE_FORAGE:
        EmBulldrome_Forage(em);
        break;
    case BULL_IDLE_AIM_SHORT:
        EmBulldrome_AimAtGoal(em, 1);
        break;
    case BULL_IDLE_NOP_C:
        EmBulldrome_IdleNopC();
        break;
    case BULL_IDLE_DISPLAY:
        EmBulldrome_Display(em, 1);
        break;
    case BULL_IDLE_AIM_MEDIUM:
        EmBulldrome_AimAtGoal(em, 2);
        break;
    case BULL_IDLE_AIM_LONG:
        EmBulldrome_AimAtGoal(em, 4);
        break;
    case BULL_IDLE_SHAKE_OFF:
        EmBulldrome_ShakeOff(em);
        break;
    case BULL_IDLE_DISPLAY_B:
        EmBulldrome_Display(em, 0);
        break;
    case BULL_IDLE_STATUS2_LIE:
        EmBulldrome_LieDown(em, 0);
        break;
    case BULL_IDLE_STATUS2_END:
        EmBulldrome_LieGetUp(em, 0);
        break;
    case BULL_IDLE_HOLD2_GET_UP:
        EmBulldrome_DownGetUp(em);
        break;
    case BULL_IDLE_STATUS1_LIE:
        EmBulldrome_LieDown(em, 1);
        break;
    case BULL_IDLE_STATUS1_END:
        EmBulldrome_LieGetUp(em, 1);
        break;
    case BULL_IDLE_STATUS4_GET_UP:
        EmBulldrome_DownGetUp(em);
        break;
    case BULL_IDLE_STUN_GET_UP:
        EmBulldrome_DownGetUp(em);
        break;
    case BULL_IDLE_REST:
        EmBulldrome_LieDown(em, 2);
        break;
    case BULL_IDLE_REST_END:
        EmBulldrome_LieGetUp(em, 2);
        break;
    case BULL_IDLE_DISPLAY_RAGE:
        EmBulldrome_Display(em, 0);
        break;
    case BULL_IDLE_DISPLAY_PENDING_RAGE:
        EmBulldrome_Display(em, 0);
        
    default:
        break;
    }
}

/*
 * BULL_MOVE_TURN_TO_TARGET (Em_StartTurnToTarget
 * with EmBulldrome_TurnParams) or BULL_MOVE_QUARTER_TURN_FIXED (aimAngle
 * rotY + 0x4000, the turn motion picked as Em_StartTurnToTarget would, front
 * motion without root motion); Em_TurnStep until done, then EmBulldrome_EndState.
 */
void EmBulldrome_TurnInPlace(Em *em, s32 variant)
{
    u32 turnDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (variant == 0) {
            Em_StartTurnToTarget(em, &EmBulldrome_TurnParams[0]);
            break;
        }
        em->aimAngle = em->rotY + 0x4000;
        turnDiff = (em->aimAngle - em->rotY) & 0xFFFF;
        if (turnDiff <= 0x1000 || turnDiff >= 0xF000) {
            Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
            Em_SetMotion(em, BULL_MOT_WALK, 0, 0);
        } else if (turnDiff >= 0x8000) {
            Em_SetMotion(em, BULL_MOT_QUARTER_TURN_B, 0, 0);
        } else {
            Em_SetMotion(em, BULL_MOT_QUARTER_TURN_A, 0, 0);
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmBulldrome_TurnParams[0]) == 1) {
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_MOVE_WALK (BULL_MOT_WALK) or
 * BULL_MOVE_WALK_B (BULL_MOT_WALK_B): Em_StepToGoal turning 0x200 a frame
 * until the goal, then EmBulldrome_EndState.
 */
void EmBulldrome_WalkToGoal(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, (u8) variant == 0 ? 2 : 3, 6, 0);
        break;
    case 1:
        if (Em_StepToGoal(em, 0x200, 0) == 1) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_MOVE_NOP_A, empty.
 */
void EmBulldrome_MoveNopA(void)
{
}

/*
 * BULL_MOVE_NOP_B, empty.
 */
void EmBulldrome_MoveNopB(void)
{
}

/*
 * BULL_MOVE_WANDER_HOME: BULL_MOT_WALK for
 * 120-247 frames (stateRand), turning 0x80 a frame toward the home position;
 * EmBulldrome_EndState on the owner when the timer ends.
 */
void EmBulldrome_WanderHome(Em *em)
{
    EmBulldromeWork *emWork = (EmBulldromeWork *)&em->work;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        Em_SetMotion(em, BULL_MOT_WALK, 6, 0);
        break;
    case 1:
        timer = em->timer;
        if (timer > 0) {
            em->timer = timer - 1;
        }
        if ((em->remote == 0) && (em->timer <= 0)) {
            EmBulldrome_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, &emWork->home[0]) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x80);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/*
 * BULL_MOVE_QUARTER_TURN,
 * _QUARTER_TURN_NEW_GOAL: turns to the goal's side with
 * BULL_MOT_QUARTER_TURN_B (goal under 0x8000, rotY + 0x135 a frame) or _A
 * (rotY - 0x135), EmBulldrome_EndState at the motion's end; variant 1 computes a clamped
 * turnStep it never reads.
 */
void EmBulldrome_QuarterTurn(Em *em, u8 variant)
{
    EmBulldromeWork *w = (EmBulldromeWork *)&em->work;
    u16 goalDiff;
    s16 turnStep;

    switch (em->step) {
    case 0:
        em->step++;
        goalDiff = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (goalDiff < 0x8000) {
            w->turnDir = 0;
        } else {
            w->turnDir = 1;
            goalDiff = 0x10000 - goalDiff;
        }
        if (variant == 0) {
            w->turnStep = 0x135;
        } else {
            if (goalDiff < 0x2000) {
                goalDiff = 0x2000;
            } else if (goalDiff > 0x6000) {
                goalDiff = 0x6000;
            }
            w->turnStep = goalDiff / 53;
        }
        Em_SetMotion(em, 5 - w->turnDir, 0, 0);
        break;
    case 1:
        turnStep = 0x135;
        if (w->turnDir != 0) {
            turnStep = -0x135;
        }
        em->rotY += turnStep;
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_MOVE_HALF_TURN (turnStep 0x26A a frame) or
 * BULL_MOVE_TURN_TO_NEW_GOAL (turnStep = goal angle clamped 0x4000-0xA000, /
 * 53): BULL_MOT_HALF_TURN, rotY + turnStep each frame, EmBulldrome_EndState at the end.
 */
void EmBulldrome_HalfTurn(Em *em, u8 variant)
{
    EmBulldromeWork *emWork = (EmBulldromeWork *)&em->work;
    u16 goalDiff;

    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BULL_MOT_HALF_TURN, 0, 0);
        if (variant == 0) {
            emWork->turnStep = 0x26A;
            break;
        }
        goalDiff = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (goalDiff < 0x4000) {
            goalDiff = 0x4000;
        } else if (goalDiff > 0xA000) {
            goalDiff = 0xA000;
        }
        emWork->turnStep = goalDiff / 53;
        break;
    case 1:
        em->rotY += emWork->turnStep;
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * Group 1 dispatcher (table in the brief).
 */
void EmBulldrome_RunMoveState(Em *em)
{
    switch (em->state) {
    case BULL_MOVE_TURN_TO_TARGET:
        EmBulldrome_TurnInPlace(em, 0);
        break;
    case BULL_MOVE_WALK:
        EmBulldrome_WalkToGoal(em, 0);
        break;
    case BULL_MOVE_WALK_B:
        EmBulldrome_WalkToGoal(em, 1);
        break;
    case BULL_MOVE_NOP_A:
        EmBulldrome_MoveNopA();
        break;
    case BULL_MOVE_NOP_B:
        EmBulldrome_MoveNopB();
        break;
    case BULL_MOVE_WANDER_HOME:
        EmBulldrome_WanderHome(em);
        break;
    case BULL_MOVE_QUARTER_TURN_FIXED:
        EmBulldrome_TurnInPlace(em, 1);
        break;
    case BULL_MOVE_QUARTER_TURN:
        EmBulldrome_QuarterTurn(em, 0);
        break;
    case BULL_MOVE_HALF_TURN:
        EmBulldrome_HalfTurn(em, 0);
        break;
    case BULL_MOVE_QUARTER_TURN_NEW_GOAL:
        EmBulldrome_QuarterTurn(em, 1);
        break;
    case BULL_MOVE_TURN_TO_NEW_GOAL:
        EmBulldrome_HalfTurn(em, 1);
        
    default:
        break;
    }
}

/*
 * BULL_TRAVEL_START: the unhittable, unseen
 * flags (see NOTE), BULL_MOT_TRAVEL_START, then BULL_TRAVEL_TURN.
 */
void EmBulldrome_TravelStart(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->fieldCollide = 0;
    em->noHitTimer = 5;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, BULL_MOT_TRAVEL_START, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_FLY, BULL_TRAVEL_TURN, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, EM_GROUP_FLY, BULL_TRAVEL_TURN, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BULL_TRAVEL_TURN: keeps the goal heading
 * (travelYaw), BULL_MOT_TRAVEL, rotY 0x400 a frame toward it; within 0x400
 * snaps and goes to BULL_TRAVEL_MOVE.
 */
void EmBulldrome_TravelTurn(Em *em)
{
    EmBulldromeWork *emWork = (EmBulldromeWork *)&em->work;

    u16 goalYaw;
    u32 diff;
    s32 rotY;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->fieldCollide = 0;
    em->noHitTimer = 5;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        emWork->travelYaw = Math_AngleXZ(&em->posX, em->goal);
        Em_SetMotion(em, BULL_MOT_TRAVEL, 0, 0);
        break;
    case 1:
        rotY = em->rotY;
        goalYaw = emWork->travelYaw;
        diff = (goalYaw - (rotY & 0xFFFF)) & 0xFFFF;
        if ((u32) ((diff + 0x400) & 0xFFFF) < 0x800U) {
            em->rotY = (s32) goalYaw;
            em->step++;
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmBulldrome_ChangeState(em, EM_GROUP_FLY, BULL_TRAVEL_MOVE, EM_SYNC_OWNER_FLAGGED);
        } else if (diff < 0x8000U) {
            em->rotY = (rotY + 0x400) & 0xFFFF;
        } else {
            em->rotY = (rotY - 0x400) & 0xFFFF;
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, EM_GROUP_FLY, BULL_TRAVEL_MOVE, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
    Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
}

/*
 * BULL_TRAVEL_MOVE: BULL_MOT_TRAVEL, steers
 * 0x40 a frame to the goal while steerToGoal, moves 80 a frame along rotY
 * and takes it off distLeft; at 0 goes to BULL_TRAVEL_END.
 */
void EmBulldrome_TravelToGoal(Em *em)
{
    f32 fwd[3];
    f32 stepOut[3];
    s32 angles[3];
    f32 mtx[16];
    EmBulldromeWork *emWork = (EmBulldromeWork *)&em->work;
    s32 goalDiff;

    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    em->fieldCollide = 0;
    em->noHitTimer = 5;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_SetMotion(em, BULL_MOT_TRAVEL, 0, 0);
        break;
    case 1:
        if (emWork->steerToGoal != 0) {
            goalDiff = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
            if (goalDiff <= 0x8000) {
                if (goalDiff < 0x40) {
                    em->rotY += goalDiff;
                } else {
                    em->rotY += 0x40;
                }
            } else if (goalDiff > 0xFFC0) {
                em->rotY += goalDiff;
            } else {
                em->rotY -= 0x40;
            }
        }
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        fwd[0] = 0.0f;
        fwd[1] = 0.0f;
        fwd[2] = 80.0f;
        angles[0] = 0;
        angles[1] = em->rotY;
        angles[2] = 0;
        Mtx_FromAnglesYXZ(angles, mtx);
        Vec_Rotate(stepOut, fwd, mtx);
        em->posX += stepOut[0];
        em->posZ += stepOut[2];
        emWork->distLeft -= fwd[2];
        if (emWork->distLeft <= 0.0f) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_FLY, BULL_TRAVEL_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, EM_GROUP_FLY, BULL_TRAVEL_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BULL_TRAVEL_END: BULL_MOT_TRAVEL_END (still
 * armored, shadow hidden), then EmBulldrome_EndState.
 */
void EmBulldrome_TravelEnd(Em *em)
{
    em->stateArmor = 1;
    em->noPushTimer = 2;
    em->guardTimer = 2;
    em->noReact = 1;
    em->hideShadowHold = 2;
    em->hideShadow = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_TRAVEL_END, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * Group 2 dispatcher, states 0-3.
 */
void EmBulldrome_RunTravelState(Em *em)
{
    switch (em->state) {
    case 0:
        EmBulldrome_TravelStart(em);
        break;
    case 1:
        EmBulldrome_TravelTurn(em);
        break;
    case 2:
        EmBulldrome_TravelToGoal(em);
        break;
    case 3:
        EmBulldrome_TravelEnd(em);
        break;
    }
}

/*
 * The charges: BULL_MOT_CHARGE with BulldromeAttack
 * type 3, 4 or 5 (variant 0, 1, 2+), Em_StepToGoal turning 0x40 a frame
 * until the goal or the setter's timer (90, or 30), then
 * BULL_MOT_CHARGE_STOP and EmBulldrome_EndState.
 */
void EmBulldrome_Charge(Em *em, s32 variant)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_CHARGE, 0xA, 0);
        switch (variant) {
        case 0:
            BulldromeAttack_Create(em, 3);
            break;
        case 1:
            BulldromeAttack_Create(em, 4);
            break;
        case 2:
            BulldromeAttack_Create(em, 5);
        }
        break;
    case 1:
        if (Em_StepToGoal(em, 0x40, 0) == 1) {
            em->timer = 1;
        }
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_CHARGE_STOP, 0xA, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_ATK_SPIN: picks the goal's quadrant (turnDir),
 * BULL_MOT_SPIN_A or _B with BulldromeAttack type 1 (spin); turns 0xE6 a
 * frame to frame 60, then 0x35C (front) or 0x92E (rear quadrants) a frame
 * over frames 142-164; EmBulldrome_EndState at the end.
 */
void EmBulldrome_Spin(Em *em)
{
    EmBulldromeWork *emWork = (EmBulldromeWork *)&em->work;
    u16 goalDiff;
    s16 spinStep;

    switch (em->step) {
    case 0:
        em->step++;
        goalDiff = Math_AngleXZ(&em->posX, em->goal) - em->rotY;
        if (goalDiff < 0x4000) {
            emWork->turnDir = 0;
        } else if (goalDiff < 0x8000) {
            emWork->turnDir = 2;
        } else if (goalDiff < 0xC000) {
            emWork->turnDir = 3;
        } else {
            emWork->turnDir = 1;
        }
        Em_SetMotion(em, (emWork->turnDir & 1) + 0x12, 0, 0);
        BulldromeAttack_Create(em, 1);
        break;
    case 1:
        spinStep = 0xE6;
        if (emWork->turnDir & 1) {
            spinStep = -0xE6;
        }
        em->rotY += spinStep;
        if (EmMotion_ReachedFrame(em, 0, 60.0f)) {
            em->step++;
        }
        break;
    case 2:
        if (EmMotion_InFrameRange(em, 0, 142.0f, 164.0f)) {
            spinStep = (emWork->turnDir & 2) == 0 ? 0x35C : 0x92E;
            if (emWork->turnDir & 1) {
                spinStep = -spinStep;
            }
            em->rotY += spinStep;
        }
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * Group 3 dispatcher; states 2-5 all run the
 * charge's variant 2.
 */
void EmBulldrome_RunAttackState(Em *em)
{
    switch (em->state) {
    case BULL_ATK_CHARGE_A:
        EmBulldrome_Charge(em, 0);
        break;
    case BULL_ATK_CHARGE_B:
        EmBulldrome_Charge(em, 1);
        break;
    case BULL_ATK_CHARGE_SHORT:
        EmBulldrome_Charge(em, 2);
        break;
    case BULL_ATK_CHARGE_SHORT_KEEP_A:
        EmBulldrome_Charge(em, 2);
        break;
    case BULL_ATK_CHARGE_SHORT_KEEP_B:
        EmBulldrome_Charge(em, 2);
        break;
    case BULL_ATK_CHARGE_SHORT_KEEP_C:
        EmBulldrome_Charge(em, 2);
        break;
    case BULL_ATK_SPIN:
        EmBulldrome_Spin(em);
        
    default:
        break;
    }
}

/*
 * BULL_REACT_FLINCH: BULL_MOT_FLINCH when hitAngle -
 * rotY is within 0x6000-0xA000, else BULL_MOT_FLINCH_SIDE_A (under 0x8000)
 * or _SIDE_B; Em_RestartScript; EmBulldrome_EndState at the end.
 */
void EmBulldrome_Flinch(Em *em)
{
    s32 diff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        diff = (em->hitAngle - em->rotY) & 0xFFFF;
        if ((diff > 24576) && (diff < 0xA000)) {
            Em_SetMotion(em, BULL_MOT_FLINCH, 0, 0);
        } else if (diff < 0x8000) {
            Em_SetMotion(em, BULL_MOT_FLINCH_SIDE_A, 0, 0);
        } else {
            Em_SetMotion(em, BULL_MOT_FLINCH_SIDE_B, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_REACT_KNOCKED_AWAY: knocked motion for
 * the hit's side, launched at frame 10 (EmBulldrome_LaunchFromHit), falls
 * (Player_MoveFall, z drag stopped when it would reverse),
 * BULL_MOT_KNOCKED_LAND for 150 frames, BULL_MOT_KNOCKED_GET_UP, EmBulldrome_EndState.
 */
void EmBulldrome_KnockedAway(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        EmBulldrome_SetKnockedMotion(em, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0U) != 0) {
            em->step++;
            EmBulldrome_LaunchFromHit(em);
        }
        break;
    case 2:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BULL_MOT_KNOCKED_LAND, 6, 0);
            em->timer = 0x96;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_KNOCKED_GET_UP, 6, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_REACT_KNOCKED_AWAY_IN_AIR: as
 * EmBulldrome_KnockedAway but the knocked motion starts at frame 10 and the launch is at
 * once.
 */
void EmBulldrome_KnockedAwayInAir(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        EmBulldrome_SetKnockedMotion(em, 0xA);
        EmBulldrome_LaunchFromHit(em);
        Em_RestartScript(em);
        break;
    case 1:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BULL_MOT_KNOCKED_LAND, 6, 0);
            em->timer = 0x96;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_KNOCKED_GET_UP, 6, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * BULL_REACT_STATUS4: hitReactMode STATUS,
 * BULL_MOT_DOWN facing away from the hit, status 4 effect each frame until
 * the timer (Em_SetStatus4Timer), then BULL_IDLE_STATUS4_GET_UP.
 */
void EmBulldrome_Status4Down(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_DOWN, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnStatus4Effect(em, 2);
        if (--em->timer <= 0) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS4_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS4_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BULL_REACT_STUN: BULL_MOT_DOWN, noReact, stun stars
 * for the timer's length (Em_SetStatus5Timer), then BULL_IDLE_STUN_GET_UP.
 */
void EmBulldrome_Stun(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, BULL_MOT_DOWN, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StunStars_CreateEm(em, 1, 0, *(s16 *)&em->timer, 100.0f, 2.2f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STUN_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STUN_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BULL_REACT_HOLD2: Em_SetHold2 every frame,
 * BULL_MOT_FLINCH without root motion, then BULL_MOT_DOWN until the timer,
 * then BULL_IDLE_HOLD2_GET_UP.
 */
void EmBulldrome_Hold2Down(Em *em)
{
    Em_SetHold2(em);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->lookAtTarget = 0;
        Em_RestartScript(em);
        Em_SetMotion(em, BULL_MOT_FLINCH, 0, 0);
        Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_DOWN, 0, 0);
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_HOLD2_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 3:
        if (em->remote == 0) {
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_HOLD2_GET_UP, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * Group 4 dispatcher (also run for group 7).
 */
void EmBulldrome_RunReactState(Em *em)
{
    switch (em->state) {
    case BULL_REACT_FLINCH:
        EmBulldrome_Flinch(em);
        break;
    case BULL_REACT_KNOCKED_AWAY:
        EmBulldrome_KnockedAway(em);
        break;
    case BULL_REACT_KNOCKED_AWAY_IN_AIR:
        EmBulldrome_KnockedAwayInAir(em);
        break;
    case BULL_REACT_STATUS4:
        EmBulldrome_Status4Down(em);
        break;
    case BULL_REACT_STUN:
        EmBulldrome_Stun(em);
        break;
    case BULL_REACT_HOLD2:
        EmBulldrome_Hold2Down(em);
        
    default:
        break;
    }
}

/*
 * BULL_DEFEAT_DIE: QuestEm_Hunted, knocked motion and
 * launch at frame 10, falls (x and z drag stopped when they would reverse),
 * BULL_MOT_KNOCKED_LAND, BULL_MOT_DEAD for 2700 frames with a carve point
 * (EmMine), fades out over 60 frames, then BULL_DEFEAT_REAPPEAR.
 */
void EmBulldrome_Die(Em *em)
{
    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        EmBulldrome_SetKnockedMotion(em, 0);
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            em->step++;
            EmBulldrome_LaunchFromHit(em);
        }
        break;
    case 2:
        if (!(em->velZ * em->accelZ < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (!(em->velX * em->accelX < 0.0f)) {
            em->accelX = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BULL_MOT_KNOCKED_LAND, 6, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_DEAD, 0, 0);
            em->timer = 2700;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
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
            em->step++;
            em->visible = 0;
        }
        break;
    case 6:
        if (em->remote == 0) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_DEFEAT, BULL_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BULL_DEFEAT_DIE_IN_AIR: as Die with the knocked
 * motion from frame 10 and the launch at once (no x drag clamp).
 */
void EmBulldrome_DieInAir(Em *em)
{
    em->noHitTimer = 10;
    em->noPushTimer = 10;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        EmBulldrome_SetKnockedMotion(em, 10);
        QuestEm_Hunted(em);
        EmBulldrome_LaunchFromHit(em);
        break;
    case 1:
        if (!(em->velZ * em->accelZ < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em) != 0) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, BULL_MOT_KNOCKED_LAND, 6, 0);
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_DEAD, 0, 0);
            em->timer = 2700;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        EmMine_Update(em);
        if (--em->timer <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 4:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
        }
        break;
    case 5:
        if (em->remote == 0) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_DEFEAT, BULL_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BULL_DEFEAT_REAPPEAR: back to BULL_IDLE if
 * QuestEm_CanAppear, else ends the task (taskPhase + 1) and waits.
 */
void EmBulldrome_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/*
 * Group 5 dispatcher, states 0-2.
 */
void EmBulldrome_RunDefeatState(Em *em)
{
    switch (em->state) {
    case BULL_DEFEAT_DIE:
        EmBulldrome_Die(em);
        break;
    case BULL_DEFEAT_DIE_IN_AIR:
        EmBulldrome_DieInAir(em);
        break;
    case BULL_DEFEAT_REAPPEAR:
        EmBulldrome_Reappear(em);
        break;
    }
}

/*
 * BULL_EVENT_CAPTURED: Em_SetCaptured each frame,
 * BULL_MOT_DOWN from frame 0x24, Em_BeginStatus1, poison cleared; at its end
 * QuestEm_Captured and the motion frozen; lying puffs, 150 frames once it
 * may appear, fade out, then BULL_DEFEAT_REAPPEAR.
 */
void EmBulldrome_Captured(Em *em)
{
    f32 pos[3];

    Em_SetCaptured(em, 1);
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BULL_MOT_DOWN, 0x1E, 0x24);
        em->timer = 150;
        Em_BeginStatus1(em);
        Em_ClearPoison(em);
        em->stance = EM_STANCE_GROUND;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            QuestEm_Captured(em);
            *(s32 *) &em->motionSpeed = 0;
        }
        break;
    case 2:
        pos[0] = 0.0f;
        pos[1] = -20.0f;
        pos[2] = 0.0f;
        Em_SpawnLyingPuffs(em, 0xB, pos, 1.0f);
        if (QuestEm_CanAppear(em) != 0) {
            if (--em->timer <= 0) {
                em->step++;
            }
        }
        break;
    case 3:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            em->step++;
        }
        break;
    case 4:
        if (em->remote == 0) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_DEFEAT, BULL_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * BULL_EVENT_INTRO (Jungle area 5 story scene,
 * event 0x99): placed at a fixed spot, BULL_MOT_WALK_B to a goal (300 frames
 * max), idle 240 frames, BULL_MOT_HALF_TURN, BULL_MOT_AIM easing toward a
 * second goal, BULL_MOT_CHARGE 90 frames, then waits EVENT_FLAG_BULLDROME_SCENE and
 * EmBulldrome_EndState.
 */
void EmBulldrome_IntroScene(Em *em)
{
    em->noHitTimer = 5;
    em->blind = 2;
    em->senseDelay = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        em->fieldCollide = 0;
        em->posX = 9800.0f;
        em->posY = -69.0f;
        em->posZ = 10336.0f;
        em->goal[0] = 7535.0f;
        em->goal[1] = -59.0f;
        em->goal[2] = 9654.0f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        GmEm_ResetSize(em);
        Em_SetMotion(em, BULL_MOT_WALK_B, 0, 0);
        em->timer = 300;
        break;
    case 1:
        em->rotY = Math_AngleXZ(&em->posX, em->goal);
        if (--em->timer <= 0 || Vec_DistanceXZ(&em->posX, em->goal) <= 100.0f) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_IDLE, 10, 0);
            em->timer = 240;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_HALF_TURN, 0, 0);
        }
        break;
    case 3:
        em->rotY += 0x26A;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_AIM, 6, 0);
            em->goal[0] = 11900.0f;
            em->goal[1] = -59.0f;
            em->goal[2] = 10300.0f;
        }
        break;
    case 4:
        em->rotY += (Math_AngleXZ(&em->posX, em->goal) - em->rotY) >> 4;
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_CHARGE, 10, 0);
            em->timer = 90;
            em->rotY = Math_AngleXZ(&em->posX, em->goal);
        }
        break;
    case 5:
        if (--em->timer <= 0) {
            em->timer = 0;
            if (Save_TestEventFlag(EVENT_FLAG_BULLDROME_SCENE) == 1) {
                em->senseDelay = 0;
                em->noHitTimer = 0;
                em->fieldCollide = 1;
                EmBulldrome_EndState(em);
            }
        }
        break;
    }
}

/*
 * Group 6 dispatcher, states 0-1.
 */
void EmBulldrome_RunEventState(Em *em)
{
    switch (em->state) {
    case BULL_EVENT_CAPTURED:
        EmBulldrome_Captured(em);
        break;
    case BULL_EVENT_INTRO:
        EmBulldrome_IntroScene(em);
        break;
    }
}

/*
 * BULL_VS_EAT_BAIT: after Versus_DefeatCamera,
 * BULL_MOT_FORAGE with a versus bait at EmBulldrome_CarveThrowPt from frame
 * 10, then Versus_RouteEmByTeam; the work argument is unused.
 */
void EmBulldrome_VsEatBait(Em *em, EmWork *work)
{
    Vec3f baitPos;

    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        if (Versus_DefeatCamera(em) == 1) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_FORAGE, 0, 0);
        }
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            baitPos = EmBulldrome_CarveThrowPt;
            VersusBait_Create(&gPlayers[(u8) em->side - 1], em, 0, 0, &baitPos.x);
        }
        if (em->motion[0].playing == 0) {
            Versus_RouteEmByTeam(em);
        }
        break;
    }
}

/*
 * BULL_VS_POSE_A: BULL_MOT_FLINCH once, idle,
 * BULL_VS_WAIT_BOTH.
 */
void EmBulldrome_VsPoseA(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BULL_MOT_FLINCH, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_IDLE, 0, 0);
            EmBulldrome_ChangeState(em, EM_GROUP_VERSUS, BULL_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * BULL_VS_POSE_B: BULL_MOT_AIM five times, idle,
 * BULL_VS_WAIT_BOTH.
 */
void EmBulldrome_VsPoseB(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BULL_MOT_AIM, 0, 0);
        em->timer = 5;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                em->step++;
                Em_SetMotion(em, BULL_MOT_IDLE, 0, 0);
                EmBulldrome_ChangeState(em, EM_GROUP_VERSUS, BULL_VS_WAIT_BOTH, EM_SYNC_SEND);
            }
        }
        break;
    }
}

/*
 * BULL_VS_POSE_C: BULL_MOT_THREAT once, idle,
 * BULL_VS_WAIT_BOTH.
 */
void EmBulldrome_VsPoseC(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        Em_SetMotion(em, BULL_MOT_THREAT, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, BULL_MOT_IDLE, 0, 0);
            EmBulldrome_ChangeState(em, EM_GROUP_VERSUS, BULL_VS_WAIT_BOTH, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * BULL_VS_WAIT_BOTH: idle until
 * Versus_BothDefeated, then BULL_VS_END.
 */
void EmBulldrome_VsWaitBoth(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->motionId[0] != EM_MOTION_ID(0, BULL_MOT_IDLE)) {
            Em_SetMotion(em, BULL_MOT_IDLE, 0, 0);
        }
        
    case 1:
        if (Versus_BothDefeated() == 1) {
            em->step++;
            EmBulldrome_ChangeState(em, EM_GROUP_VERSUS, BULL_VS_END, EM_SYNC_SEND);
        }
        break;
    }
}

/*
 * BULL_VS_END: idle 90 frames, clears senseDelay and
 * noHitTimer, EmBulldrome_EndState.
 */
void EmBulldrome_VsEnd(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->timer = 0x5A;
        if (em->motionId[0] != EM_MOTION_ID(0, BULL_MOT_IDLE)) {
            Em_SetMotion(em, BULL_MOT_IDLE, 0, 0);
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->senseDelay = 0;
            em->noHitTimer = 0;
            EmBulldrome_EndState(em);
        }
        break;
    }
}

/*
 * Group 8 dispatcher (0-3, 0x98, 0x99).
 */
void EmBulldrome_RunVersusState(Em *em)
{
    u8 *emWork = &em->work;

    switch (em->state) {
    case BULL_VS_EAT_BAIT:
        EmBulldrome_VsEatBait(em, (EmWork *) emWork);
        break;
    case BULL_VS_POSE_A:
        EmBulldrome_VsPoseA(em);
        break;
    case BULL_VS_POSE_B:
        EmBulldrome_VsPoseB(em);
        break;
    case BULL_VS_POSE_C:
        EmBulldrome_VsPoseC(em);
        break;
    case BULL_VS_WAIT_BOTH:
        EmBulldrome_VsWaitBoth(em);
        break;
    case BULL_VS_END:
        EmBulldrome_VsEnd(em);
        break;
    }
}

/*
 * Per frame: counts the work's countdown down, combat
 * timer, area trigger, holds position without players, low-hp check (0.3) in
 * Jungle/Swamp/Snowy maps; picks the state from Em_UpdateDamage (killed/dead
 * -> BULL_DEFEAT_DIE; status in air -> KNOCKED_AWAY_IN_AIR; status 4 ->
 * BULL_REACT_STATUS4; captured -> BULL_EVENT_CAPTURED; status 2 -> BULL_IDLE_STATUS2_LIE;
 * hit lying -> STATUS1_END from STATUS1_LIE, else STATUS2_END; part flinch
 * -> FLINCH if part 0's state bits 0-1, else KNOCKED_AWAY; hit mode 3 ->
 * KNOCKED_AWAY; stun -> BULL_REACT_STUN; attack 0x13 -> BULL_REACT_HOLD2); starts
 * BULL_EVENT_INTRO in the story quest, else runs the pending script; then
 * runs the state (twice on a stateRequest).
 */
void EmBulldrome_Update(Em *em)
{
    u8 dmgOut[3];
    EmBulldromeWork *emWork = (EmBulldromeWork *)&em->work;

    if (emWork->countdown != 0) {
        emWork->countdown -= 1;
    }
    Em_UpdateCombatTimer(em);
    Em_RaisePlayerAreaTrigger(em);
    Em_HoldPosNoPlayers(em, 0, 1);
    if (em->noAlert != 1 && em->combat == 1) {
        switch ((u16) Map_DayId(gScene.map)) {
        case MAP_JUNGLE:
        case MAP_SWAMP:
        case MAP_SNOWY_MOUNTAINS:
            Em_CheckLowHp(em, 0.3f);
            break;
        }
    }
    switch (Em_UpdateDamage(em, dmgOut) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        EmBulldrome_ChangeState(em, EM_GROUP_DEFEAT, BULL_DEFEAT_DIE, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS_IN_AIR:
        if (em->stateGroup != EM_GROUP_REACT || (u32) (em->state - BULL_REACT_KNOCKED_AWAY) > 1) {
            EmBulldrome_ChangeState(em, EM_GROUP_REACT, BULL_REACT_KNOCKED_AWAY_IN_AIR, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if (em->stateGroup != EM_GROUP_REACT || (u32) (em->state - BULL_REACT_KNOCKED_AWAY) > 1) {
            Em_RestartScript(em);
            Em_SetStatus4Timer(em);
            EmBulldrome_ChangeState(em, EM_GROUP_REACT, BULL_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        EmBulldrome_ChangeState(em, EM_GROUP_EVENT, BULL_EVENT_CAPTURED, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS2:
        if (em->stateGroup != EM_GROUP_REACT || (u32) (em->state - BULL_REACT_KNOCKED_AWAY) > 1) {
            Em_SetStatus2Timer(em);
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS2_LIE, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        switch (em->state) {
        case BULL_IDLE_STATUS1_LIE:
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS1_END, EM_SYNC_OWNER);
            break;
        default:
            Em_RestartScript(em);
            EmBulldrome_ChangeState(em, EM_GROUP_IDLE, BULL_IDLE_STATUS2_END, EM_SYNC_OWNER);
            break;
        }
        break;
    case EM_DMG_PART_FLINCH:
        if (em->parts[0].state & 3) {
            EmBulldrome_ChangeState(em, EM_GROUP_REACT, BULL_REACT_FLINCH, EM_SYNC_OWNER);
        } else {
            EmBulldrome_ChangeState(em, EM_GROUP_REACT, BULL_REACT_KNOCKED_AWAY, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_MODE3:
        EmBulldrome_ChangeState(em, EM_GROUP_REACT, BULL_REACT_KNOCKED_AWAY, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS5:
        if (em->stateGroup != EM_GROUP_REACT || (em->state != 4 && em->state != 2)) {
            Em_SetStatus5Timer(em);
            EmBulldrome_ChangeState(em, EM_GROUP_REACT, BULL_REACT_STUN, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_ATTACK13:
        Em_ResetKindTimer(em);
        EmBulldrome_ChangeState(em, EM_GROUP_REACT, BULL_REACT_HOLD2, EM_SYNC_OWNER);
        break;
    }
    if ((gQuest.stage == QUEST_ID_BULLDROME_SCENE_1 || gQuest.stage == QUEST_ID_BULLDROME_SCENE_2) && Area_DayId(em->area) == AREA_JUNGLE_5 && Save_TestEventFlag(EVENT_FLAG_BULLDROME_SCENE) == 0) {
        if (ScEvent_CurId() == 0x99 && em->stateGroup != EM_GROUP_EVENT) {
            EmBulldrome_ChangeState(em, EM_GROUP_EVENT, BULL_EVENT_INTRO, EM_SYNC_SEND);
        }
    } else {
        Em_UpdatePendingScript(em);
    }
    Em_ClearHitReactMode(em);
    EmBulldrome_RunState(em);
    if (em->stateRequest != 0) {
        EmBulldrome_RunState(em);
        em->stateRequest = 0;
    }
}

/*
 * Runs the group's dispatcher; group 7 runs the
 * react dispatcher.
 */
void EmBulldrome_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmBulldrome_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmBulldrome_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmBulldrome_RunTravelState(em);
        break;
    case EM_GROUP_ATTACK:
        EmBulldrome_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmBulldrome_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmBulldrome_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmBulldrome_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmBulldrome_RunReactState(em);
        break;
    case EM_GROUP_VERSUS:
        EmBulldrome_RunVersusState(em);
        
    default:
        break;
    }
}

/*
 * Empty per-kind setup called by Em_TypeSetup.
 */
void EmBulldrome_Setup(Em *em)
{
}

/*
 * Callback slots 0 and 1: Shadow_Spawn type
 * 0 (the flat model under the monster).
 */
void EmBulldrome_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0);
}

/*
 * Empty callback (slot 2).
 */
void EmBulldrome_TableNop(void)
{
}

/*
 * Entry work of group 0: STATUS2_LIE
 * Em_BeginStatus2, STATUS2_END Em_EndStatus2, STATUS1_LIE Em_BeginStatus1,
 * STATUS1_END Em_EndStatus1, HOLD2_GET_UP clears holdTimer2,
 * STATUS4/STUN_GET_UP end the status with noReact, REST Em_ResetNotice,
 * REST_END Em_RefillNeed3, DISPLAY_RAGE Em_StartRage, DISPLAY_PENDING_RAGE
 * Em_StartPendingRage; ground stance; Em_SetStateSync.
 */
void EmBulldrome_SetIdleState(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case BULL_IDLE_STATUS2_LIE:
        Em_BeginStatus2(em);
        break;
    case BULL_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case BULL_IDLE_STATUS1_LIE:
        Em_BeginStatus1(em);
        break;
    case BULL_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case BULL_IDLE_HOLD2_GET_UP:
        em->holdTimer2 = 0;
        break;
    case BULL_IDLE_STATUS4_GET_UP:
        Em_EndStatus4(em);
        em->noReact = 1;
        break;
    case BULL_IDLE_STUN_GET_UP:
        Em_EndStatus5(em);
        em->noReact = 1;
        break;
    case BULL_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case BULL_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case BULL_IDLE_DISPLAY_RAGE:
        Em_StartRage(em);
        break;
    case BULL_IDLE_DISPLAY_PENDING_RAGE:
        Em_StartPendingRage(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * WALK and WALK_B compute the goal and the walk
 * speed (WALK_B also timer 1800); QUARTER_TURN_NEW_GOAL and TURN_TO_NEW_GOAL
 * compute the goal; ground stance; Em_SetStateSync.
 */
void EmBulldrome_SetMoveState(Em *em, u16 state, u8 mode)
{
    switch ((u16) state) {
    case BULL_MOVE_WALK_B:
        em->timer = 1800;
    case BULL_MOVE_WALK:
        Em_ComputeGoal2(em, em->goal);
        Em_SetSpeedScaled(em, 2, 1000.0f, 25.0f);
        break;
    case BULL_MOVE_QUARTER_TURN_NEW_GOAL:
    case BULL_MOVE_TURN_TO_NEW_GOAL:
        Em_ComputeGoal2(em, em->goal);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * BULL_TRAVEL_TURN computes the goal; BULL_TRAVEL_MOVE
 * computes it again, sets steerToGoal and distLeft to the XZ distance;
 * Em_SetStateSync.
 */
void EmBulldrome_SetTravelState(Em *em, u16 state, u8 mode)
{
    EmBulldromeWork *bullWork = (EmBulldromeWork *)&em->work;

    switch ((u16) state) {
    case BULL_TRAVEL_TURN:
        Em_ComputeGoal2(em, em->goal);
        break;
    case BULL_TRAVEL_MOVE:
        Em_ComputeGoal2(em, em->goal);
        bullWork->steerToGoal = 1;
        bullWork->distLeft = Vec_DistanceXZ(&em->posX, em->goal);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Enters an attack state. BULL_ATK_CHARGE_A, _B and _SHORT compute the
 * goal, hasGoal and goalDist (the distance + 300) unless there is no
 * target, and a timer of 90 frames (30 for BULL_ATK_CHARGE_SHORT); then
 * EM_STANCE_GROUND and Em_SetStateSync.
 */
void EmBulldrome_SetAttackState(Em *em, u16 state, u8 mode)
{
    u16 attack = state;

    switch (attack) {
    case BULL_ATK_CHARGE_A:
    case BULL_ATK_CHARGE_B:
    case BULL_ATK_CHARGE_SHORT:
        Em_ComputeGoal2(em, em->goal);
        if (em->targetNo == -1) {
            em->hasGoal = 0;
        } else {
            em->hasGoal = 1;
            em->goalDist = 300.0f + Vec_DistanceXZ(&em->posX, em->goal);
        }
        em->timer = 90;
        if (attack == BULL_ATK_CHARGE_SHORT) {
            em->timer = 30;
        }
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * BULL_REACT_STATUS4 Em_BeginStatus4, BULL_REACT_STUN
 * Em_BeginStatus5, BULL_REACT_HOLD2 Em_EndAllStatus; Em_SetStateSync.
 */
void EmBulldrome_SetReactState(Em *em, u16 state, u8 mode)
{
    switch (state) {
    case BULL_REACT_STATUS4:
        Em_BeginStatus4(em);
        break;
    case BULL_REACT_STUN:
        Em_BeginStatus5(em);
        break;
    case BULL_REACT_HOLD2:
        Em_EndAllStatus(em);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_REACT, state, mode);
}

/*
 * On the owner retargets the player in range; a
 * remote monster ignores owner-only modes (3, 4). Resets motionSpeed and
 * calls the group's setter; group 5 BULL_DEFEAT_REAPPEAR calls
 * QuestEm_Appear when allowed; group 8 BULL_VS_WAIT_BOTH calls
 * Versus_MarkDefeat; others go straight to Em_SetStateSync.
 */
void EmBulldrome_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if ((mode == EM_SYNC_OWNER) || (mode == EM_SYNC_OWNER_FLAGGED)) {
        return;
    }
    em->motionSpeed = 1.0f;
    switch (group) {
    case EM_GROUP_IDLE:
        EmBulldrome_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmBulldrome_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmBulldrome_SetTravelState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmBulldrome_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_REACT:
        EmBulldrome_SetReactState(em, state, mode);
        return;
    case EM_GROUP_DEFEAT:
        if ((state == BULL_DEFEAT_REAPPEAR) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_VERSUS:
        if (state == BULL_VS_WAIT_BOTH) {
            Versus_MarkDefeat(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    default:
        Em_SetStateSync(em, group, state, mode);
        return;
    }
}

/*
 * Sets velocity (EmBulldrome_LaunchVel turned
 * to hitAngle + 0x8000) and acceleration (EmBulldrome_LaunchAccel) and
 * EM_STANCE_AIR: the knock-away jump.
 */
void EmBulldrome_LaunchFromHit(Em *em)
{
    s32 angles[3];
    f32 mtx[16];

    angles[2] = 0;
    angles[0] = 0;
    angles[1] = em->hitAngle + 0x8000;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(&em->velX, EmBulldrome_LaunchVel, mtx);
    Vec_Copy(&em->accelX, EmBulldrome_LaunchAccel);
    em->stance = EM_STANCE_AIR;
}

/*
 * Plays BULL_MOT_KNOCKED_SIDE_A (hitAngle -
 * rotY under 0x8000) or _SIDE_B from the given frame.
 */
void EmBulldrome_SetKnockedMotion(Em *em, s16 frame)
{
    s32 row;

    if ((u16) (em->hitAngle - em->rotY) < 0x8000) {
        row = BULL_MOT_KNOCKED_SIDE_A;
    } else {
        row = BULL_MOT_KNOCKED_SIDE_B;
    }
    Em_SetMotion(em, row, 0, frame);
}

/* The file's initialised data, in the game's order */
EmTurnParams EmBulldrome_TurnParams[1] = {
#include "game/008CB480.inc"
};
f32 EmBulldrome_Effect0BOfs[3] = {
#include "game/008CB4A0.inc"
};
Vec3f EmBulldrome_CarveThrowPt = {
#include "game/008CB4B0.inc"
};
s32 GmEm_ReturnZero(void); /* gm_sub */
void GmEm_UpdateSfx(Em *em); /* gm_sub */
void EmBulldrome_SpawnEffect01(u8 *em);
void EmBulldrome_TableNop(void);
void *EmBulldrome_Callbacks[8] = {
    EmBulldrome_SpawnEffect01, EmBulldrome_SpawnEffect01, EmBulldrome_TableNop, GmEm_UpdateSfx,
    GmEm_ReturnZero, EmBulldrome_Reinit, NULL, NULL,
};
f32 EmBulldrome_LaunchVel[4] = {
#include "game/008CB4E0.inc"
};
f32 EmBulldrome_LaunchAccel[4] = {
#include "game/008CB4F0.inc"
};
u32 gEmMotionRows_68[304] = {
#include "game/008CB500.inc"
};
u32 gEmKindTables_68_1_1[8] = {
#include "game/008CB9C0.inc"
};
u32 gEmKindTables_68_1_3[12] = {
#include "game/008CB9E0.inc"
};
u32 gEmKindTables_68_1_5[12] = {
#include "game/008CBA10.inc"
};
u32 gEmKindTables_68_1_7[8] = {
#include "game/008CBA40.inc"
};
u32 gEmKindTables_68_1_9[8] = {
#include "game/008CBA60.inc"
};
u32 gEmKindTables_68_1_11[12] = {
#include "game/008CBA80.inc"
};
u32 gEmKindTables_68_1_13[16] = {
#include "game/008CBAB0.inc"
};
u32 gEmKindTables_68_1_15[16] = {
#include "game/008CBAF0.inc"
};
u32 gEmKindTables_68_1_17[12] = {
#include "game/008CBB30.inc"
};
u32 gEmKindTables_68_1_19[10] = {
#include "game/008CBB60.inc"
};
u32 gEmKindTables_68_1_21[3] = {
#include "game/008CBB88.inc"
};
u32 gEmKindTables_68_1_23[8] = {
#include "game/008CBBA0.inc"
};
u32 gEmKindTables_68_1_25[12] = {
#include "game/008CBBC0.inc"
};
u32 gEmKindTables_68_1[28] = {
#include "game/008CBBF0.inc"
};
u32 gEmKindTables_68_3[2] = {
#include "game/008CBC60.inc"
};
u32 gEmKindTables_68_4[2] = {
#include "game/008CBC68.inc"
};
u32 gEmKindTables_68_5[4] = {
#include "game/008CBC70.inc"
};
u32 gEmKindTables_68_6_3_1[16] = {
#include "game/008CBC80.inc"
};
u32 gEmKindTables_68_6_3_7[16] = {
#include "game/008CBCC0.inc"
};
u32 gEmKindTables_68_6_3[12] = {
#include "game/008CBD00.inc"
};
u32 gEmKindTables_68_6_1_1[12] = {
#include "game/008CBD30.inc"
};
u32 gEmKindTables_68_6_1_7[20] = {
#include "game/008CBD60.inc"
};
u32 gEmKindTables_68_6_1[12] = {
#include "game/008CBDB0.inc"
};
u32 gEmKindTables_68_6_5_1[20] = {
#include "game/008CBDE0.inc"
};
u32 gEmKindTables_68_6_5[8] = {
#include "game/008CBE30.inc"
};
u32 gEmKindTables_68_6_9_1[28] = {
#include "game/008CBE50.inc"
};
u32 gEmKindTables_68_6_9[8] = {
#include "game/008CBEC0.inc"
};
u32 gEmKindTables_68_6_7_1[16] = {
#include "game/008CBEE0.inc"
};
u32 gEmKindTables_68_6_7[8] = {
#include "game/008CBF20.inc"
};
u32 gEmKindTables_68_6_11_1[16] = {
#include "game/008CBF40.inc"
};
u32 gEmKindTables_68_6_11_7[20] = {
#include "game/008CBF80.inc"
};
u32 gEmKindTables_68_6_11[12] = {
#include "game/008CBFD0.inc"
};
u32 D_008CC000[16] = {
#include "game/008CC000.inc"
};
u32 gEmKindTables_68_6_13_7[16] = {
#include "game/008CC040.inc"
};
u32 gEmKindTables_68_6_13[12] = {
#include "game/008CC080.inc"
};
u32 gEmKindTables_68_6_15_1[16] = {
#include "game/008CC0B0.inc"
};
u32 gEmKindTables_68_6_15_7[16] = {
#include "game/008CC0F0.inc"
};
u32 gEmKindTables_68_6_15[12] = {
#include "game/008CC130.inc"
};
u32 gEmKindTables_68_6_17_1[20] = {
#include "game/008CC160.inc"
};
u32 gEmKindTables_68_6_17_7[20] = {
#include "game/008CC1B0.inc"
};
u32 gEmKindTables_68_6_17[12] = {
#include "game/008CC200.inc"
};
u32 gEmKindTables_68_6_19_1[16] = {
#include "game/008CC230.inc"
};
u32 gEmKindTables_68_6_19_7[20] = {
#include "game/008CC270.inc"
};
u32 gEmKindTables_68_6_19_13[16] = {
#include "game/008CC2C0.inc"
};
u32 gEmKindTables_68_6_19[20] = {
#include "game/008CC300.inc"
};
u32 gEmKindTables_68_6_21_1[16] = {
#include "game/008CC350.inc"
};
u32 gEmKindTables_68_6_21_7[12] = {
#include "game/008CC390.inc"
};
u32 gEmKindTables_68_6_21[12] = {
#include "game/008CC3C0.inc"
};
u32 gEmKindTables_68_6_23_1[16] = {
#include "game/008CC3F0.inc"
};
u32 gEmKindTables_68_6_23[8] = {
#include "game/008CC430.inc"
};
u32 gEmKindTables_68_6_25_1[16] = {
#include "game/008CC450.inc"
};
u32 gEmKindTables_68_6_25_7[16] = {
#include "game/008CC490.inc"
};
u32 gEmKindTables_68_6_25[12] = {
#include "game/008CC4D0.inc"
};
u32 gEmKindTables_68_6[28] = {
#include "game/008CC500.inc"
};
extern u32 gEmKindTables_68_0[];
extern u32 gEmKindTables_68_2[];
u32 gEmKindTables_68[8] = {
    (u32)gEmKindTables_68_0, (u32)gEmKindTables_68_1, (u32)gEmKindTables_68_2, (u32)gEmKindTables_68_3,
    (u32)gEmKindTables_68_4, (u32)gEmKindTables_68_5, (u32)gEmKindTables_68_6, 0,
};
u32 gEmAreaTimer1Values_68[4] = {
#include "game/008CC590.inc"
};
u32 gEmAreaTimer2Values_68[16] = {
#include "game/008CC5A0.inc"
};
