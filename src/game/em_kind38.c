/*
 * Monster kind 0x38 (no code shows its species): the hovering monster Yama
 * Tsukami calls up, which a quest can also place in a row at the start
 * (EmKind38_Start). It hovers and drifts at a height between two limits,
 * charges with one contact attack, can blow itself up, and keeps a looping
 * sound going (EmKind38_UpdateLoopSe). The state
 * functions take the monster and its work area (EmKind38Work, its view of
 * EmWork); Em.step is the step within the current state.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_kind38.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
s32 EmKind38_MoveByVelocity(Em *em);
void Em_EndStatus5(Em *em);
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
/* unit's own declaration: group and state are declared u16; with the definition's s32 the callers (EmKind38_ChangeState, EmShakalaka_ChangeState ...) pass the state without the 16-bit mask the game's code applies */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
f32 Math_Sin(f32 x);
s8 QuestEm_Appear(Em *em);
ObjAttach *BulldromeAttack_Create(Em *em, s8 kind);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_PlaySeAtBoneVol(Em *em, s32 se, s32 bone, s32 vol);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
s32 Em_StepToGoal(Em *em, u16 step, u8 flags);
void Em_RestartScript(Em *em);
void EmKind38_UpdateLoopSe(Em *em, EmWork *work);
void EmKind38_Init(Em *);
void EmKind38_FlyForward(Em *, s16);
void EmKind38_FlyBackward(Em *);
void EmKind38_Climb(Em *);
void EmKind38_Descend(Em *);
void EmKind38_Sidestep(Em *, s32);
void EmKind38_Drift(Em *);
void EmKind38_BurstOut(Em *);
void EmKind38_Charge(Em *, u8);
void EmKind38_SelfDestruct(Em *);
void EmKind38_Die(Em *, u8);
void EmKind38_Reappear(Em *);
void EmKind38_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmBodyFx_Spawn(Em *em, s16 kind);
void Vec_RotateY(f32 *v, f32 angle);
void Vec_Copy(f32 *out, f32 *in);
void Vec_Add(f32 *, f32 *, f32 *);
s32 QuestEm_CanAppear(Em *em);
void Player_Integrate(void *);
void Player_ClearVelocity(void *);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
void Shadow_Spawn(void *obj, s16 type);
extern u16 EmKind38_Effect02Chance[];
extern f32 EmKind38_HeightRange[][2];
EmBone *Em_Bone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
f32 Math_Absf(f32 x);
Em *QuestEm_Spawn(QuestEm *q);
f32 Math_Sqrtf(f32 x);
f32 Math_Atan2(f32 y, f32 x);
extern f32 EmKind38_SpawnOfs[];
void QuestEm_Hunted(Em *em);
void EmMine_Drop(Em *em, f32 *pos);
void EmKind38_PlaySe(Em *em, s32 arg1);
void EmKind38_ClampVelocity(Em *em, f32 *lim);
extern f32 EmKind38_VelLimits[];
void EmKind38_KeepHeight(Em *em, u8 k);
extern Vec3f EmKind38_DriftAccels[];
extern f32 EmKind38_DriftVelLimits[];
void Em_InitParts(Em *em);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
void EmKind38_InitHover(Em *em, EmWork *work);
extern f32 gStartPos[];
typedef struct { f32 x, y; } EmKPair;
extern EmKPair EmKind38_ChargeReach[][2];
void *EmKind38_CreateChargeAttack(Em *em);
void Vec_RotateX(f32 *v, f32 angle);
void Vec_Scale(f32 *out, f32 *in, f32 s);
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
void Explosion_CreateAt(f32 *pos, s32 kind, u16 area, u8 arg3);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
s32 Em_UpdateCombatTimer(Em *em);
s32 EmKind38_HitByIce(Em *em);
void EmKind38_RunState(Em *em);
void EmKind38_SteerHover(Em *em, EmWork *work);
void EmKind38_ApplyHover(Em *em, EmWork *work);
void Em_EndStatus5(Em *em);
/* end of prototypes */

/*
 * The looped sound by seKind: kind 1 plays sound 0 at bone 2 every 10
 * frames, kind 2 sound 1 every 11 frames; otherwise the timer stays at 0.
 */
void EmKind38_UpdateLoopSe(Em *em, EmWork *work)
{
    EmKind38Work *w = (EmKind38Work *)work;

    switch (w->seKind) {
    case 1:
        if (w->seTimer == 0) {
            Em_PlaySeAtBoneVol(em, 0, 2, 3);
        }
        (w->seTimer)++;
        if (w->seTimer >= 10) {
            w->seTimer = 0;
        }
        break;
    case 2:
        if (w->seTimer == 0) {
            Em_PlaySeAtBoneVol(em, 1, 2, 3);
        }
        (w->seTimer)++;
        if (w->seTimer >= 11) {
            w->seTimer = 0;
        }
        break;
    default:
        w->seTimer = 0;
        break;
    }
}

/*
 * Sets the monster up: places a quest-placed one on the area's start points
 * at the quest's start, sets hp and size, clears the work flags and starts
 * the hover's bob (EmKind38_InitHover).
 */
void EmKind38_Init(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    s16 hpValue;

    if (gQuest.stage == 0 && !(em->spawnParam & 1)) {
        if (gScene.area == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->posX = 9500.0f + 150.0f * (em->slotNo / 8);
            em->posY = 0.0f;
            em->posZ = 9200.0f + 150.0f * (em->slotNo & 7);
        } else {
            em->posX = 150.0f * em->slotNo + gStartPos[gScene.area * 3];
            em->posY = gStartPos[gScene.area * 3 + 1];
            em->posZ = gStartPos[gScene.area * 3 + 2];
        }
    }
    Em_SetMotionRow(em, K38_MOT_IDLE, 0, 0);
    Player_ClearVelocity(em);
    hpValue = Em_ComputeMaxHp(em, 0);
    em->hp = hpValue;
    em->hpMax = hpValue;
    w->baseScale = em->bodyScale;
    em->scriptPending = 1;
    em->blind = 0;
    w->dashing = 0;
    w->charging = 0;
    w->seKind = 0;
    w->seTimer = 0;
    Em_InitParts(em);
    EmKind38_InitHover(em, (EmWork *) w);
    em->remote = 0;
}

/*
 * The start hook: a spawned one (spawnParam bit 0) starts in the air with
 * an 1800-frame stay and K38_FLY_BURST_OUT; a placed one starts on the
 * ground in K38_IDLE.
 */
void EmKind38_Start(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;

    EmKind38_Init(em);
    if (em->spawnParam & 1) {
        em->stance = EM_STANCE_AIR;
        w->stayTimer = 0x708;
        em->scriptPending = 0;
        EmKind38_ChangeState(em, EM_GROUP_FLY, K38_FLY_BURST_OUT, EM_SYNC_LOCAL);
        return;
    }
    em->stance = EM_STANCE_GROUND;
    w->stayTimer = -1;
    EmKind38_ChangeState(em, EM_GROUP_IDLE, K38_IDLE, EM_SYNC_LOCAL);
}

/* Callback slot 5: runs EmKind38_Init. */
void EmKind38_Reinit(Em *em)
{
    EmKind38_Init(em);
}

/*
 * Ends a state on the ground: clears the dash flag, asks the script for its
 * next state and enters K38_IDLE.
 */
void EmKind38_EndGroundState(Em *em)
{
    ((EmKind38Work *)&em->work)->dashing = 0;
    em->scriptPending = 1;
    em->stance = EM_STANCE_GROUND;
    EmKind38_ChangeState(em, EM_GROUP_IDLE, K38_IDLE, EM_SYNC_LOCAL);
}

/* Ends a state in the air: the same, entering K38_FLY_HOVER. */
void EmKind38_EndAirState(Em *em)
{
    ((EmKind38Work *)&em->work)->dashing = 0;
    em->scriptPending = 1;
    em->stance = EM_STANCE_AIR;
    EmKind38_ChangeState(em, EM_GROUP_FLY, K38_FLY_HOVER, EM_SYNC_LOCAL);
}

/* K38_IDLE_NONE: nothing. */
void EmKind38_IdleNop(void)
{
}

/* K38_IDLE: waits 30-75 frames on the ground, then ends the state. */
void EmKind38_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0xF) + 0x1E;
        break;
    case 1:
        if (--em->timer <= 0) {
            EmKind38_EndGroundState(em);
        }
        break;
    }
}

/*
 * Runs the current group 0 (idle) state's function, remembering the state
 * as the previous one.
 */
void EmKind38_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case K38_IDLE_NONE:
        EmKind38_IdleNop();
        break;
    case K38_IDLE:
        EmKind38_Idle(em);
        break;
    }
}

/* K38_MOVE_NONE: nothing. */
void EmKind38_MoveNop(void)
{
}

/* Runs the current group 1 (move) state's function. */
void EmKind38_RunMoveState(Em *em)
{
    switch (em->state) {
    case K38_MOVE_NONE:
        EmKind38_MoveNop();
        break;
    }
}

/*
 * K38_FLY_FORWARD_SHORT and _LONG (variant 0, 1): flies toward the goal at
 * its hover height (at most 800 or 1200 ahead), turning up to 0x20 a frame,
 * then brakes.
 */
void EmKind38_FlyForward(Em *em, s16 variant)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    f32 lim;

    w->seKind = 1;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        w->bobGainGoal = 0.6f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (variant == 0) {
            em->accelZ = 0.345f;
            Em_SetSpeedScaled(em, 0, 300.0f, 200.0f);
            if (!(em->goalDist <= 800.0f)) {
                em->goalDist = 800.0f;
            }
        } else {
            em->accelZ = 0.555f;
            Em_SetSpeedScaled(em, 0, 300.0f, 100.0f);
            if (!(em->goalDist <= 1200.0f)) {
                em->goalDist = 1200.0f;
            }
        }
        break;
    case 1:
        EmKind38_KeepHeight(em, 0);
        if (variant == 0) {
            lim = 8.0f;
        } else {
            lim = 10.0f;
        }
        if (!(em->velZ <= lim)) {
            em->velZ = lim;
        }
        if (Em_StepToGoal(em, 0x20, 2) == 1) {
            em->accelZ = 0.0625f * -em->velZ;
            em->accelY = 0.0f;
            em->step++;
        }
        break;
    case 2:
        em->velY *= 0.7f;
        if (em->velZ - em->accelZ <= 0.0f) {
            w->bobGainGoal = 1.0f;
            Player_ClearVelocity(em);
            EmKind38_EndAirState(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

/* K38_FLY_BACKWARD: backs 800 away while facing the goal, then brakes. */
void EmKind38_FlyBackward(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;

    w->seKind = 1;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        w->bobGainGoal = 0.6f;
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        em->goalDist = -800.0f;
        em->accelZ = -0.421f;
        break;
    case 1:
        if (em->velZ < -6.4f) {
            em->velZ = -6.4f;
        }
        if (!((em->goalDist -= em->velZ) < 0.0f)) {
            em->accelZ = 0.0625f * -em->velZ;
            em->step++;
        }
        break;
    case 2:
        if (!(em->velZ - em->accelZ < 0.0f)) {
            w->bobGainGoal = 1.0f;
            Player_ClearVelocity(em);
            EmKind38_EndAirState(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

/* K38_FLY_HOVER: faces the goal for 30-120 frames, then ends the state. */
void EmKind38_Hover(Em *em)
{
    ((EmKind38Work *)&em->work)->seKind = 1;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        em->timer = ((em->stateRand & 3) * 0x1E) + 0x1E;
        break;
    case 1:
        em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        if (--em->timer <= 0) {
            em->step++;
            EmKind38_EndAirState(em);
        }
        break;
    }
}

/*
 * K38_FLY_CLIMB: rises forward for 30-54 frames (from the ground after a
 * 10-frame pause), its speed clamped and damped.
 */
void EmKind38_Climb(Em *em)
{
    em->unk472 = 1;
    switch (em->step) {
    case 0:
        em->step++;
        if (em->stance == EM_STANCE_GROUND) {
            Player_ClearVelocity(em);
            em->timer = 10;
            break;
        }
        em->timer = 0;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->stance = EM_STANCE_AIR;
            em->timer = (em->stateRand & 3) * 8 + 0x1E;
            em->accelY = 0.777f;
            em->accelZ = 0.55f;
        }
        break;
    case 2:
        EmKind38_ClampVelocity(em, EmKind38_VelLimits);
        if (--em->timer > 0) {
            break;
        }
        em->step++;
        em->accelZ = 0.0f;
        em->accelY = 0.0f;
    case 3:
        em->velY *= 0.876f;
        em->velZ *= 0.876f;
        if (Math_Absf(em->velY) < 0.05f) {
            em->step++;
            em->velZ = 0.0f;
            em->velY = 0.0f;
            EmKind38_EndAirState(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

/*
 * K38_FLY_DESCEND: sinks forward for 30-54 frames; touching the floor it
 * lands and ends the state on the ground.
 */
void EmKind38_Descend(Em *em)
{
    em->unk472 = 1;
    switch (em->step) {
    case 0:
        if (em->stance == EM_STANCE_GROUND) {
            em->accelY = -0.5f;
        } else {
            em->step++;
            em->timer = 5;
        }
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = (em->stateRand & 3) * 8 + 0x1E;
            em->accelY = -0.777f;
            em->accelZ = 0.55f;
        }
        break;
    case 2:
        EmKind38_ClampVelocity(em, EmKind38_VelLimits);
        if (--em->timer > 0) {
            break;
        }
        em->step++;
        em->accelZ = 0.0f;
        em->accelY = 0.0f;
    case 3:
        em->velY *= 0.876f;
        em->velZ *= 0.876f;
        if (Math_Absf(em->velY) < 0.05f) {
            em->step++;
            em->velZ = 0.0f;
            em->velY = 0.0f;
            EmKind38_EndAirState(em);
            return;
        }
        break;
    }
    if ((s16) EmKind38_MoveByVelocity(em)) {
        em->stance = EM_STANCE_GROUND;
        em->accelY = 0.0f;
        em->velY = 0.0f;
        em->accelZ = 0.0f;
        em->velZ = 0.0f;
        EmKind38_EndGroundState(em);
    }
}

/*
 * K38_FLY_SIDESTEP_A and _B (variant 0, 1): flies 90 frames with its
 * heading 0x3800 off the goal's direction, keeping its height, then brakes.
 */
void EmKind38_Sidestep(Em *em, s32 dir)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    s32 turn;

    w->seKind = 1;
    em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
    if (dir == 0) {
        turn = -0x3800;
    } else {
        turn = 0x3800;
    }
    em->rotY += turn;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        w->bobGainGoal = 0.6f;
        em->timer = 0x5A;
        em->accelZ = 0.333f;
        break;
    case 1:
        EmKind38_KeepHeight(em, 0);
        if (em->velZ < -6.4f) {
            em->velZ = -6.4f;
        }
        if (--em->timer <= 0) {
            em->accelZ = 0.0625f * -em->velZ;
            em->accelY = 0.0f;
            em->step++;
        }
        break;
    case 2:
        em->velY *= 0.7f;
        if (em->velZ - em->accelZ <= 0.0f) {
            w->bobGainGoal = 1.0f;
            Player_ClearVelocity(em);
            EmKind38_EndAirState(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

/*
 * K38_FLY_DRIFT: drifts 15-27 frames along one of four directions
 * (EmKind38_DriftAccels), its speed clamped and damped.
 */
void EmKind38_Drift(Em *em)
{
    em->unk472 = 1;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Player_ClearVelocity(em);
        em->stateVar0 = em->stateRand & 3;
        em->timer = 5;
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            em->timer = (em->stateRand & 3) * 4 + 0xF;
            Vec_Copy(&em->accelX, &EmKind38_DriftAccels[em->stateVar0].x);
        }
        break;
    case 2:
        EmKind38_ClampVelocity(em, EmKind38_DriftVelLimits);
        if (--em->timer > 0) {
            break;
        }
        em->step++;
        em->accelZ = 0.0f;
        em->accelX = 0.0f;
    case 3:
        em->velX *= 0.876f;
        em->velZ *= 0.876f;
        if (Math_Absf(em->velZ) + Math_Absf(em->velZ) < 0.03f) {
            em->step++;
            em->velZ = 0.0f;
            em->velX = 0.0f;
            EmKind38_EndAirState(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

/* K38_FLY_NONE: nothing. */
void EmKind38_FlyNop(void)
{
}

/*
 * K38_FLY_BURST_OUT: flung out fast with a random pitch and yaw, growing
 * from nothing to its size over 15-24 frames, then slowing down.
 */
void EmKind38_BurstOut(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    f32 scale;

    switch (em->step) {
    case 0:
        em->noReact = 1;
        em->stance = EM_STANCE_AIR;
        em->step++;
        w->bobGainGoal = 0.0f;
        w->scaleFrames = em->timer = (em->stateRand & 3) * 3 + 0xF;
        em->rotX += (em->stateRand & 0xF00) + 0x600;
        em->rotY += ((em->stateRand >> 4) & 0xF00) - 0x780;
        Player_ClearVelocity(em);
        em->velZ = 50.0f + 7.0f * ((em->stateRand >> 2) & 3);
        Vec_RotateX(&em->velX, (6.2831855f * (f32) em->rotX) / 65536.0f);
        em->noPushTimer = 5;
        EmKind38_PlaySe(em, 2);
        break;
    case 1:
        em->noPushTimer = 5;
        w->seKind = 1;
        scale = w->baseScale * (w->scaleFrames - em->timer) / w->scaleFrames;
        em->scaleX = em->scaleY = em->scaleZ = scale;
        if (--em->timer <= 0) {
            em->step++;
            Vec_Scale(&em->accelX, &em->velX, -0.025f);
            em->scaleX = em->scaleY = em->scaleZ = w->baseScale;
            w->bobGainGoal = 1.0f;
        }
        break;
    case 2:
        w->seKind = 1;
        if (em->velZ - em->accelZ <= 0.0f) {
            em->step++;
            em->noReact = 0;
            Player_ClearVelocity(em);
            EmKind38_EndAirState(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

/* Runs the current group 2 (fly) state's function. */
void EmKind38_RunFlyState(Em *em)
{
    switch (em->state) {
    case K38_FLY_FORWARD_SHORT:
        EmKind38_FlyForward(em, 0);
        break;
    case K38_FLY_BACKWARD:
        EmKind38_FlyBackward(em);
        break;
    case K38_FLY_FORWARD_LONG:
        EmKind38_FlyForward(em, 1);
        break;
    case K38_FLY_HOVER:
        EmKind38_Hover(em);
        break;
    case K38_FLY_CLIMB:
        EmKind38_Climb(em);
        break;
    case K38_FLY_DESCEND:
        EmKind38_Descend(em);
        break;
    case K38_FLY_SIDESTEP_A:
        EmKind38_Sidestep(em, 0);
        break;
    case K38_FLY_SIDESTEP_B:
        EmKind38_Sidestep(em, 1);
        break;
    case K38_FLY_DRIFT:
        EmKind38_Drift(em);
        break;
    case K38_FLY_NONE:
        EmKind38_FlyNop();
        break;
    case K38_FLY_BURST_OUT:
        EmKind38_BurstOut(em);
        
    default:
        break;
    }
}

/*
 * K38_ATK_CHARGE_SHORT and _LONG (variant 0, 1): waits 10 frames, makes the
 * charge attack and charges 400 past the goal (at most the reach of
 * EmKind38_ChargeReach), then brakes.
 */
void EmKind38_Charge(Em *em, u8 k)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    EmKPair *reachRow;

    w->charging = 1;
    w->seKind = 2;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Player_ClearVelocity(em);
        em->timer = 10;
        w->bobGainGoal = 1.0f;
        break;
    case 1:
        EmKind38_KeepHeight(em, 1);
        if (--em->timer <= 0) {
            em->step++;
            w->dashing = 1;
            EmKind38_CreateChargeAttack(em);
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            em->accelY = 0.0f;
            reachRow = &EmKind38_ChargeReach[em->variant][k];
            Em_SetSpeedScaled(em, 0, 500.0f, -400.0f);
            if (!(em->goalDist <= reachRow->x)) {
                em->goalDist = reachRow->x;
            }
            em->accelZ = reachRow->y;
            w->bobGainGoal = 0.0f;
            em->accelY = 0.0f;
        }
        break;
    case 2:
        em->velY *= 0.6f;
        if ((em->goalDist -= em->velZ) < 0.0f) {
            w->dashing = 0;
            w->charging = 0;
            em->accelZ = 0.025f * -em->velZ;
            em->step++;
        }
        break;
    case 3:
        w->charging = 0;
        if (em->velZ - em->accelZ <= 0.0f) {
            w->bobGainGoal = 1.0f;
            Player_ClearVelocity(em);
            EmKind38_EndAirState(em);
            return;
        }
        break;
    }
    EmKind38_MoveByVelocity(em);
}

/*
 * Makes the charge's contact attack (BulldromeAttack type 2 by a
 * per-variant chance, else type 0). Returns it, or NULL when no player is
 * in the area.
 */
void * EmKind38_CreateChargeAttack(Em *em)
{
    s8 kind;

    kind = 0;
    if ((em->stateRand & 0x1F) < (s32) EmKind38_Effect02Chance[em->variant]) {
        kind = 2;
    }
    return BulldromeAttack_Create(em, kind);
}

/*
 * K38_ATK_SELF_DESTRUCT: waits 20 frames, flies at the target with its body
 * pulsing, and explodes at bone 2 on arrival (K38_DEFEAT_VANISH).
 */
void EmKind38_SelfDestruct(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    f32 pos[3];
    f32 scale;

    w->charging = 1;
    w->seKind = 2;
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Player_ClearVelocity(em);
        em->timer = 20;
        w->bobGainGoal = 1.0f;
        w->pulseAng = 0;
        break;
    case 1:
        EmKind38_KeepHeight(em, 1);
        if (--em->timer <= 0) {
            em->step++;
            em->accelZ = 0.4f;
            em->rotY = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
            Em_SetSpeedScaled(em, 0, 500.0f, -50.0f);
            if (!(em->goalDist <= 1600.0f)) {
                em->goalDist = 1600.0f;
            }
            w->bobGainGoal = 0.0f;
        }
        break;
    case 2:
        EmKind38_KeepHeight(em, 1);
        if (!(em->velZ <= 9.6f)) {
            em->velZ = 9.6f;
        }
        if (Em_StepToGoal(em, 0x20, 2) == 1) {
            if ((s32)Player_InCurrentArea(em) & 0xFF) {
                Mtx_GetTrans(pos, em->model->bones[2].world);
                FireBlast_CreateScaled(pos, 0, 1.0f);
                Explosion_CreateAt(pos, EXPLOSION_DIVE_IMPACT, em->area, 0);
            }
            EmKind38_ChangeState(em, EM_GROUP_DEFEAT, K38_DEFEAT_VANISH, EM_SYNC_OWNER);
            return;
        }
        break;
    }
    scale = w->baseScale * (1.0f + 0.4f * Math_Sin(9.58738019e-05f * w->pulseAng));
    em->scaleX = em->scaleY = em->scaleZ = scale;
    w->pulseAng += 0x777;
    EmKind38_MoveByVelocity(em);
}

/* Runs the current group 3 (attack) state's function. */
void EmKind38_RunAttackState(Em *em)
{
    switch (em->state) {
    case K38_ATK_CHARGE_SHORT:
        EmKind38_Charge(em, 0U);
        break;
    case K38_ATK_CHARGE_LONG:
        EmKind38_Charge(em, 1U);
        break;
    case K38_ATK_SELF_DESTRUCT:
        EmKind38_SelfDestruct(em);
        break;
    }
}

/*
 * K38_REACT_FLINCH_AIR and K38_REACT_STATUS_FLINCH: asks the script for the
 * reaction in the air and waits for its motion to end.
 */
void EmKind38_Flinch(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_AIR;
        em->step++;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmKind38_EndAirState(em);
        }
        break;
    }
}

/* K38_REACT_NONE: nothing. */
void EmKind38_ReactNop(void)
{
}

/* K38_REACT_FLINCH_GROUND: the same on the ground. */
void EmKind38_FlinchGround(Em *em)
{
    switch (em->step) {
    case 0:
        em->stance = EM_STANCE_GROUND;
        em->step++;
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmKind38_EndGroundState(em);
        }
        break;
    }
}

/* Runs the current group 4 (react) state's function. */
void EmKind38_RunReactState(Em *em)
{
    switch (em->state) {
    case K38_REACT_FLINCH_AIR:
        EmKind38_Flinch(em);
        break;
    case K38_REACT_STATUS_FLINCH:
        EmKind38_Flinch(em);
        break;
    case K38_REACT_NONE:
        EmKind38_ReactNop();
        break;
    case K38_REACT_FLINCH_GROUND:
        EmKind38_FlinchGround(em);
        break;
    }
}

/*
 * K38_DEFEAT_DIE and K38_DEFEAT_VANISH (variant 1: hidden at once):
 * QuestEm_Hunted, a 60-frame hold and fade, a mine point for a placed one,
 * then K38_DEFEAT_REAPPEAR after 150 frames.
 */
void EmKind38_Die(Em *em, u8 variant)
{
    f32 pos[3];

    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        QuestEm_Hunted(em);
        em->step++;
        if (variant == 0) {
            em->stateVar0 = 0x3C;
        } else {
            em->stateVar0 = 0;
            em->alpha = 0.0f;
        }
        if (em->variant == 0) {
            Vec_Copy(pos, &em->posX);
            pos[1] = em->floorY;
            EmMine_Drop(em, pos);
        }
        break;
    case 1:
        em->hideShadowHold = 2;
        em->hideShadow = 2;
        if (em->stateVar0 != 0) {
            em->stateVar0--;
            if (em->stateVar0 == 0) {
                EmKind38_PlaySe(em, 3);
            }
        } else if ((em->alpha -= 0.016666668f) <= 0.0f) {
            em->visible = 0;
            em->alpha = 0.0f;
            em->timer = 150;
            em->step++;
            em->painted = 0;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            EmKind38_ChangeState(em, EM_GROUP_DEFEAT, K38_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * K38_DEFEAT_REAPPEAR: comes back 300 above the floor (K38_EVENT_FADE_IN)
 * if the quest lets it appear again, else ends the task.
 */
void EmKind38_Reappear(Em *em)
{
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            em->posY += 300.0f;
            em->alpha = 0.0f;
            EmKind38_ChangeState(em, EM_GROUP_EVENT2, K38_EVENT_FADE_IN, EM_SYNC_LOCAL);
        } else {
            em->taskPhase++;
        }
        break;
    case 1:
        break;
    }
}

/*
 * Runs the current group 5 (defeat) state's function, unhittable and
 * unpushable.
 */
void EmKind38_RunDefeatState(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    switch (em->state) {
    case K38_DEFEAT_DIE:
        EmKind38_Die(em, 0U);
        break;
    case K38_DEFEAT_VANISH:
        EmKind38_Die(em, 1U);
        break;
    case K38_DEFEAT_REAPPEAR:
        EmKind38_Reappear(em);
        break;
    }
}

/* K38_EVENT_NONE: nothing. */
void EmKind38_EventNop(void)
{
}

/* Runs the current group 6 (event) state's function. */
void EmKind38_RunEventState(Em *em)
{
    switch (em->state) {
    case K38_EVENT_NONE:
        EmKind38_EventNop();
        break;
    }
}

/*
 * K38_EVENT_FADE_IN: sinks for 30 frames while fading in, kept within 1000
 * of the floor.
 */
void EmKind38_FadeIn(Em *em)
{

    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_AIR;
        Player_ClearVelocity(em);
        em->velY = -10.0f;
        em->timer = 0x1E;
        break;
    case 1:
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
            EmKind38_EndAirState(em);
            em->alpha = 1.0f;
        }
        break;
    }
}

/* Runs the current group 7 state's function, unhittable and unpushable. */
void EmKind38_RunEvent2State(Em *em)
{
    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    em->senseDelay = 5;
    switch (em->state) {
    case K38_EVENT_FADE_IN:
        EmKind38_FadeIn(em);
        break;
    }
}

/*
 * The per-frame update: Em_UpdateDamage's result as a reaction or a death
 * (an ice hit kills it), the spawned one's stay time, the script, the
 * state, then the hover's bob.
 */
void EmKind38_Update(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;
    u8 buf[4];

    em->motionSpeed = 1.0f;
    Em_UpdateCombatTimer(em);
    switch (Em_UpdateDamage(em, buf) & 0xFF) {
    case 0:
    case 3:
    case 4:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 13:
    case 15:
    case 16:
    case 18:
        break;
    case 1:
    case 2:
        if (em->stateGroup != EM_GROUP_DEFEAT) {
            EmKind38_ChangeState(em, EM_GROUP_DEFEAT, K38_DEFEAT_DIE, EM_SYNC_OWNER);
            if (EmKind38_HitByIce(em) == 1) {
                EmKind38_PlaySe(em, 4);
            }
        }
        em->scriptPending = 0;
        break;
    case 5:
        if (EmKind38_HitByIce(em) == 1) {
            EmKind38_ChangeState(em, EM_GROUP_DEFEAT, K38_DEFEAT_DIE, EM_SYNC_OWNER);
            EmKind38_PlaySe(em, 4);
        } else if (em->stateGroup != EM_GROUP_REACT || em->state != K38_REACT_STATUS_FLINCH) {
            EmKind38_ChangeState(em, EM_GROUP_REACT, K38_REACT_STATUS_FLINCH, EM_SYNC_OWNER);
            EmKind38_PlaySe(em, 5);
        }
        em->scriptPending = 0;
        break;
    case 12:
    case 14:
        if (EmKind38_HitByIce(em) == 1) {
            EmKind38_ChangeState(em, EM_GROUP_DEFEAT, K38_DEFEAT_DIE, EM_SYNC_OWNER);
            EmKind38_PlaySe(em, 4);
        } else {
            if (em->stance == EM_STANCE_AIR) {
                EmKind38_ChangeState(em, EM_GROUP_REACT, K38_REACT_FLINCH_AIR, EM_SYNC_OWNER);
            } else {
                EmKind38_ChangeState(em, EM_GROUP_REACT, K38_REACT_FLINCH_GROUND, EM_SYNC_OWNER);
            }
            EmKind38_PlaySe(em, 5);
        }
        em->scriptPending = 0;
        break;
    case 17:
        Em_EndStatus5(em);
        break;
    }
    if (w->stayTimer > 0) {
        (w->stayTimer)--;
    }
    if (w->stayTimer == 0) {
        switch (em->stateGroup) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
            break;
        case 0:
        case 1:
        case 2:
            EmKind38_ChangeState(em, EM_GROUP_DEFEAT, K38_DEFEAT_DIE, EM_SYNC_OWNER);
            em->scriptPending = 0;
            break;
        }
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmKind38_RunState(em);
    if (em->stateRequest != 0) {
        EmKind38_RunState(em);
        em->stateRequest = 0;
    }
    EmKind38_SteerHover(em, (EmWork *) w);
    EmKind38_ApplyHover(em, (EmWork *) w);
}

/* Clears the per-frame flags, then runs the state's group dispatcher. */
void EmKind38_RunState(Em *em)
{
    ((EmKind38Work *)&em->work)->charging = 0;
    em->unk472 = 0;
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmKind38_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmKind38_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmKind38_RunFlyState(em);
        break;
    case EM_GROUP_ATTACK:
        EmKind38_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmKind38_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmKind38_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmKind38_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmKind38_RunEvent2State(em);
        
    default:
        break;
    }
}

/* Keeps the current motion in the work area and runs the looped sound. */
void EmKind38_MotionEffects(Em *em, EmWork *work)
{
    s16 *words = (s16 *) work;

    if (em->motionId[0] != words[0x21]) {
        words[0x21] = em->motionId[0];
    }
    EmKind38_UpdateLoopSe(em, work);
}

/*
 * Callback slot 3: skips the first frame, then runs EmKind38_MotionEffects
 * every frame.
 */
void EmKind38_UpdateMotionEffects(Em *em)
{
    EmKind38Work *w = (EmKind38Work *)&em->work;

    switch (w->fxStep) {
    case 0:
        w->fxStep++;
        break;
    case 1:
        EmKind38_MotionEffects(em, (EmWork *) w);
        break;
    }
}

/*
 * Callback slots 0 and 1: the charge's glow (EmBodyFx) and the shadow-like
 * effect under the monster.
 */
void EmKind38_SpawnEffects(void *em)
{
    EmBodyFx_Spawn(em, 2);
    Shadow_Spawn((Player *) em, 0);
}

/* Callback slot 2: nothing. */
void EmKind38_TableNop(void)
{
}

/*
 * Changes state: resets the size, computes the goal for the flights and
 * attacks that need one, sets up the burst-out and the reappearance, then
 * Em_SetStateSync.
 */
void EmKind38_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 scale;

    em->unk473 = 0;
    scale = ((EmKind38Work *)&em->work)->baseScale;
    em->scaleZ = scale;
    em->scaleY = scale;
    em->scaleX = scale;
    switch (group) {
    case EM_GROUP_FLY:
        switch (state) {
        case K38_FLY_FORWARD_SHORT:
        case K38_FLY_BACKWARD:
        case K38_FLY_FORWARD_LONG:
        case K38_FLY_SIDESTEP_A:
        case K38_FLY_SIDESTEP_B:
            Em_ComputeGoal2(em, em->goal);
            break;
        case K38_FLY_BURST_OUT:
            em->noReact = 1;
            em->scaleZ = 0.0f;
            em->scaleY = 0.0f;
            em->scaleX = 0.0f;
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        switch (state) {
        case K38_ATK_CHARGE_SHORT:
        case K38_ATK_CHARGE_LONG:
        case K38_ATK_SELF_DESTRUCT:
            Em_ComputeGoal2(em, em->goal);
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == 2 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_IDLE:
    case EM_GROUP_MOVE:
    case EM_GROUP_REACT:
    case EM_GROUP_EVENT:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Starts the hover's bob: an offset of 15-24, its sign from the height
 * above the floor.
 */
void EmKind38_InitHover(Em *em, EmWork *work)
{
    u16 rnd;
    f32 startOffset;
    f32 ground;
    f32 y;
    EmKind38Work *hover = (EmKind38Work *)work;

    ground = em->floorY;
    rnd = em->stateRand;
    y = em->posY;
    startOffset = -15 - ((rnd >> 8) & 3) * 3;
    if (!(y <= 300.0f + ground)) {
        startOffset = -startOffset;
    } else if (!(y <= 60.0f + ground) && (rnd & 0x8000)) {
        startOffset = -startOffset;
    }
    hover->bobPos = startOffset;
    hover->bobAccel = 0.0f;
    hover->bobVel = 0.0f;
    hover->bobGainGoal = 1.0f;
    hover->bobGain = 0.0f;
}

/*
 * Picks the bob's acceleration toward the centre, enough to stop within 50
 * of it; the bob's speed is at most 7.5.
 */
void EmKind38_SteerHover(Em *em, EmWork *work)
{
    EmKind38Work *hover = (EmKind38Work *)work;
    f32 stopAccel;

    if (!(hover->bobPos * hover->bobAccel < 0.0f)) {
        if (!(hover->bobVel <= 7.5f)) {
            hover->bobVel = 7.5f;
        }
        if (hover->bobVel < -7.5f) {
            hover->bobVel = -7.5f;
        }
        hover->bobAccel = 0.512f + 0.007f * ((em->stateRand >> 8) & 0x1F);
        stopAccel = hover->bobVel * hover->bobVel / (2.0f * (50.0f - Math_Absf(hover->bobPos)));
        stopAccel *= 1.05f;
        if (hover->bobAccel < stopAccel) {
            hover->bobAccel = stopAccel;
        }
        if (!(hover->bobPos + hover->bobVel <= 0.0f)) {
            hover->bobAccel = -hover->bobAccel;
        }
    }
}

/*
 * In the air, eases the bob in and adds it to the height, never below the
 * floor; on the ground, no bob.
 */
void EmKind38_ApplyHover(Em *em, EmWork *work)
{
    EmKind38Work *hover = (EmKind38Work *)work;

    if (em->stance == EM_STANCE_AIR) {
        hover->bobGain = hover->bobGain + 0.25f * (hover->bobGainGoal - hover->bobGain);
    } else {
        hover->bobGain = 0.0f;
    }
    hover->bobVel += hover->bobAccel;
    hover->bobPos += hover->bobVel;
    em->posY += hover->bobVel * hover->bobGain;
    if (em->posY < em->floorY) {
        em->posY = em->floorY;
    }
}

/*
 * Adds the acceleration to the velocity and the velocity, turned by rotY,
 * to the position. Returns -1 if it ended below the floor (then put on it),
 * else 0.
 */
s32 EmKind38_MoveByVelocity(Em *em)
{
    f32 vel[3];
    f32 *pos;
    f32 floorY;
    f32 *out;

    out = &em->velX;
    Vec_Add((f32 *) out, (f32 *) out, &em->accelX);
    Vec_Copy(vel, &em->velX);
    Vec_RotateY(vel, (6.2831855f * (f32) em->rotY) / 65536.0f);
    pos = &em->posX;
    Vec_Add(pos, pos, vel);
    floorY = em->floorY;
    if (em->posY < floorY) {
        em->posY = floorY;
        return -1;
    }
    return 0;
}

/*
 * Clamps each axis of the velocity to its limit, clearing that axis's
 * acceleration when it clamps.
 */
void EmKind38_ClampVelocity(Em *em, f32 *max)
{
    f32 limit;

    limit = max[0];
    if (Math_Absf(em->velX) > limit) {
        em->velX = (em->velX > 0.0f) ? limit : -limit;
        em->accelX = 0.0f;
    }
    limit = max[1];
    if (Math_Absf(em->velY) > limit) {
        em->velY = (em->velY > 0.0f) ? limit : -limit;
        em->accelY = 0.0f;
    }
    limit = max[2];
    if (Math_Absf(em->velZ) > limit) {
        em->velZ = (em->velZ > 0.0f) ? limit : -limit;
        em->accelZ = 0.0f;
    }
}

/*
 * Holds the height between 20 and 60 above the goal's, damping the vertical
 * speed inside the band.
 */
void EmKind38_KeepHeight(Em *em, u8 k)
{
    f32 *range = EmKind38_HeightRange[k];

    if (em->posY < 20.0f + em->goal[1]) {
        em->accelY = range[0];
    } else if (em->posY > 60.0f + em->goal[1]) {
        em->accelY = -range[0];
    } else {
        em->accelY = 0.0f;
        em->velY *= 0.8f;
    }
    if (em->velY > range[1]) {
        em->velY = range[1];
    }
    if (em->velY < -range[1]) {
        em->velY = -range[1];
    }
}

/*
 * Returns 1 when an ice hit has marked part 0 (partFlags 0x20) and the
 * monster isn't being defeated.
 */
s32 EmKind38_HitByIce(Em *em)
{
    if ((em->stateGroup != EM_GROUP_DEFEAT) && (em->partFlags[0] & 0x20)) {
        return 1;
    }
    return 0;
}

/*
 * Spawns a kind 0x38 monster at bone 0x22 of the given monster (Yama
 * Tsukami), facing along the bone, when that monster is in the current
 * area.
 */
void EmKind38_SpawnAtEmBone(Em *em)
{
    QuestEm entry;
    f32 spawnPos[3];
    Em *minion;
    f32 *mtx;

    if (em->area == gScene.area) {
        entry.kind = 0x38;
        entry.variant = 1;
        entry.hp = -1;
        entry.spawnParam = 1;
        entry.size = 100;
        entry.level = em->level;
        minion = QuestEm_Spawn(&entry);
        if (minion != NULL) {
            mtx = (f32 *)Em_Bone(em, 0x22);
            Vec_RotateSelf(EmKind38_SpawnOfs, mtx);
            Vec_Add(&minion->posX, spawnPos, mtx + 12);
            spawnPos[2] = Math_Sqrtf(mtx[8] * mtx[8] + mtx[10] * mtx[10]);
            minion->rotX = (u16) (s32) (10430.378f * Math_Atan2(-mtx[9], spawnPos[2]));
            minion->rotY = (u16) (s32) (10430.378f * Math_Atan2(mtx[8], mtx[10]));
            minion->rotZ = 0;
        }
    }
}

/*
 * Returns 1 while the monster charges (the glow EmKind38_SpawnEffects made
 * brightens then).
 */
u8 EmKind38_IsCharging(Em *em)
{
    return ((EmKind38Work *)&em->work)->charging;
}

/* Plays a sound of the kind at bone 2. */
void EmKind38_PlaySe(Em *em, s32 soundId)
{
    Em_PlaySeAtBoneVol(em, soundId, 0x2, 0x3);
}

/* The file's initialised data, in the game's order */
s32 GmEm_ReturnZero(void); /* gm_sub */
f32 EmKind38_VelLimits[6] = {
#include "game/008CA260.inc"
};
u16 EmKind38_Effect02Chance[4] = {
#include "game/008CA278.inc"
};
EmKPair EmKind38_ChargeReach[2][2] = {
#include "game/008CA280.inc"
};
Vec3f EmKind38_DriftAccels[4] = {
#include "game/008CA2A0.inc"
};
void *EmKind38_Callbacks[8] = {
    EmKind38_SpawnEffects, EmKind38_SpawnEffects, EmKind38_TableNop, EmKind38_UpdateMotionEffects,
    GmEm_ReturnZero, EmKind38_Reinit, NULL, NULL,
};
f32 EmKind38_HeightRange[2][2] = {
#include "game/008CA2F0.inc"
};
f32 EmKind38_SpawnOfs[4] = {
#include "game/008CA300.inc"
};
