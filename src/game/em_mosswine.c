/*
 * Monster family: Mosswine and Bullfango (EM_KIND_MOSSWINE and
 * EM_KIND_BULLFANGO), which share one set of state functions (EmMosswine_*);
 * the per-kind hooks also send kind 0x20 to its ChangeState. The code from
 * the family's first function, EmMosswine_Init, to its answers to the quest
 * scripts (EmMosswine_ScriptQuery): one function per state, a dispatcher per
 * state group (Run*State) and ChangeState, the per-frame Update that turns
 * Em_UpdateDamage's result into reactions, the motion effects. The state
 * functions take the monster and its work area (EmMosswineWork, the
 * family's view of EmWork); Em.step is the step within the current state.
 * include/em_mosswine.h names the family's motions and states.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "em_mosswine.h"
#include "player.h"
#include "objattach.h"
#include "area.h"
void QuestEm_Hunted(Em *em);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Player_ClearAccel2(void *);
void EmMosswine_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_SetMotion(Em *em, s32 motion, s32 a, s32 b);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmAnteka_ChangeState, EmKhezu_ChangeState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
u16 Math_AngleXZ(f32 *from, f32 *to);
void Player_ClearFlag(void *, u32 flag);
void Player_SetFlag(void *obj, u32 flag);
ObjAttach *EmContactAttack_Create(Em *em, s8 kind);
void Em_ComputeGoal2(Em *em, f32 *target);
void Em_SpawnStatus4Effect(Em *em, s32 a);
void Em_SetHitReactMode(Em *em, u8 a);
void Em_StartTurnToTarget(Em *em, EmTurnParams *t);
s32 Em_TurnStep(Em *em, EmTurnParams *t);
void Em_RestartScript(Em *em);
void EmMosswine_Init(Em *);
void EmMosswine_Status2Lie(Em *);
void EmMosswine_Turn(Em *, s32);
void EmMosswine_Walk(Em *);
void EmMosswine_Run(Em *em);
void EmMosswine_WalkB(Em *em);
void EmMosswine_WalkScripted(Em *);
void EmMosswine_WanderHome(Em *em);
void EmMosswine_Charge(Em *, u32);
void EmMosswine_BlownAway(Em *);
void EmMosswine_BlownAwayQuick(Em *);
void EmMosswine_Die(Em *);
void EmMosswine_DieQuick(Em *);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale), the calls in EmKhezu_HeldDowned compile differently */
void StunStars_CreateEm(Em *, s32, s32, s32, f32, f32);
s32 EmMotion_PassedFrameScaled(void *p, f32 frame, s32 layer);
f32 Vec_Distance(f32 *a, f32 *b);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
s32 Angle_TurnToward(s32 *cur, s32 *target, s32 step);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
void Motion_InitTime(void *p, s16 step);
void Shadow_Spawn(void *obj, s16 type);
s8 QuestEm_Appear(Em *em);
void Em_BeginStatus5(Em *em);
void Em_EndStatus5(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's s16 (void *obj, u8 state, u8 sub), the calls in EmMosswine_ScriptQuery and EmMosswine_Update compile differently */
s32 Player_IsState(void *obj, s32 group, s32 state);
extern f32 EmMosswine_VariantParams[][6];
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
extern f32 EmMosswine_Effect0BOfs[3];
void Em_SetMotionRow(Em *em, s32 row, s32 arg2, s32 arg3);
s32 Em_ComputeMaxHp(Em *em, s16 idx);
s8 EmMine_Create(Em *em, s32 idx);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
void Em_FindLink(Em *em);
void Em_InitParts(Em *em);
void GmEm_ApplySize(Em *em);
extern EmTurnParams EmMosswine_TurnParams[1];
s32 Player_MoveFall(Player *p);
s32 Em_SetCombat(Em *em, u8 a, s16 b);
void Em_SetStatus4Timer(Em *em);
void Em_SetStatus2Timer(Em *em);
u8 Em_UpdateDamage(Em *em, u8 *out);
void Em_UpdatePendingScript(Em *em);
void Em_ClearHitReactMode(Em *em);
void Em_SetStatus5Timer(Em *em);
void Em_ResetPartHp(Em *em, u32 i);
void EmMosswine_RunState(Em *em);
/* end of prototypes */

/*
 * Sets up a Mosswine or a Bullfango. Outside a quest (gQuest.stage 0) it is
 * placed on a test row by its slot number. A Mosswine gets its hp from
 * Em_ComputeMaxHp; a Bullfango links to its pack's leader (Em_FindLink),
 * takes its hp row from Em.variant (row 0 past 3), and variants 1 and 3 are
 * drawn 2.7 times larger. Then the size (GmEm_ApplySize), its home
 * (EmMosswineWork.home, the position on the ground) and the parts.
 */
void EmMosswine_Init(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    f32 fslot;
    s16 fullHp;
    s16 variantHp;
    s16 baseHp;
    u8 row;
    u8 slotNo;

    if (gQuest.stage == 0) {
        if (gScene.area == AREA_SNOWY_MOUNTAINS_NIGHT_9) {
            em->posX = 9500.0f + (150.0f * (f32) (em->slotNo / 8));
            em->posY = 0.0f;
            em->posZ = 9200.0f + (150.0f * (f32) (em->slotNo & 7));
        } else {
            slotNo = em->slotNo;
            fslot = (f32) (u32) slotNo;
            em->posX = 5000.0f + (200.0f * fslot);
            em->posY = 0.0f;
            em->posZ = 5000.0f;
        }
    }
    em->blind = 0;
    em->remote = 0;
    Em_SetMotionRow(em, MOSS_MOT_IDLE, 0, 0);
    em->stance = EM_STANCE_GROUND;
    if (em->type == EM_KIND_MOSSWINE) {
        fullHp = Em_ComputeMaxHp(em, 0);
        em->hp = fullHp;
        em->hpMax = fullHp;
    } else {
        switch (em->variant) {
        case 1:
        case 3:
            em->scaleX = 2.7f;
            em->scaleY = 2.7f;
            em->scaleZ = 2.7f;
            em->bodyScale = 2.7f;
            break;
        }
        Em_FindLink(em);
        row = em->variant;
        if ((s32) row < 4) {
            variantHp = Em_ComputeMaxHp(em, (s16) row);
            em->hp = variantHp;
            em->hpMax = variantHp;
        } else {
            baseHp = Em_ComputeMaxHp(em, 0);
            em->hp = baseHp;
            em->hpMax = baseHp;
        }
    }
    GmEm_ApplySize(em);
    w->unk00C = 0;
    w->unk00E = 0;
    w->home[0] = em->posX;
    w->home[1] = 0.0f;
    w->home[2] = em->posZ;
    Em_InitParts(em);
}

/* Init, then MOSS_IDLE. */
void EmMosswine_Start(Em *em)
{
    EmMosswine_Init(em);
    EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE, EM_SYNC_LOCAL);
}

/* Callback slot 5 (EmVt.appear): Init again, the state kept. */
void EmMosswine_Reinit(Em *em)
{
    EmMosswine_Init(em);
}

/*
 * The family's state change (Em_ChangeState dispatches here by kind): the
 * motion speed back to 1, then per state: MOSS_IDLE becomes the kind's
 * alert idle in combat; MOSS_IDLE_STUN_END ends status 5 and blocks
 * reactions; the walks and the run head for Em_ComputeGoal2's goal for at
 * most 1800 frames, MOSS_MOVE_WALK_SCRIPTED from its variant's fixed start
 * to its goal (EmMosswine_VariantParams) for 600; each charge gets its
 * goal, its length (30 to 120 frames) and its motion speed; a Bullfango's
 * MOSS_REACT_STUN begins status 5; MOSS_DEFEAT_REAPPEAR lets the quest
 * place it first (QuestEm_Appear).
 */
void EmMosswine_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    f32 *params;

    em->motionSpeed = 1.0f;
    switch (group) {
    case EM_GROUP_IDLE:
        switch (state) {
        case MOSS_IDLE:
            if (em->combat == 1) {
                if (em->type == EM_KIND_MOSSWINE) {
                    state = MOSS_IDLE_ALERT_MOSSWINE;
                } else {
                    state = MOSS_IDLE_ALERT_BULLFANGO;
                }
            }
            break;
        case MOSS_IDLE_STUN_END:
            Em_EndStatus5(em);
            em->noReact = 1;
            break;
        }
        break;
    case EM_GROUP_MOVE:
        switch (state) {
        case MOSS_MOVE_WALK:
        case MOSS_MOVE_RUN:
        case MOSS_MOVE_WALK_B:
            em->timer = 1800;
            Em_ComputeGoal2(em, em->goal);
            break;
        case MOSS_MOVE_WALK_SCRIPTED:
            em->timer = 600;
            params = EmMosswine_VariantParams[em->variant];
            em->posX = *params++;
            em->posY = *params++;
            em->posZ = *params++;
            em->goal[0] = *params++;
            em->goal[1] = *params++;
            em->goal[2] = *params++;
            break;
        }
        break;
    case EM_GROUP_ATTACK:
        Em_ComputeGoal2(em, em->goal);
        switch (state) {
        case MOSS_ATK_CHARGE_A:
            em->timer = 90;
            em->motionSpeed = 0.8f;
            break;
        case MOSS_ATK_CHARGE_B:
            em->timer = 90;
            em->motionSpeed = 0.8f;
            break;
        case MOSS_ATK_CHARGE_C:
            em->timer = 90;
            em->motionSpeed = 1.0f;
            break;
        case MOSS_ATK_CHARGE_D:
            em->timer = 120;
            em->motionSpeed = 1.0f;
            break;
        case MOSS_ATK_CHARGE_E:
            em->timer = 30;
            em->motionSpeed = 0.8f;
            break;
        case MOSS_ATK_CHARGE_F:
            em->timer = 45;
            em->motionSpeed = 0.8f;
            break;
        }
        break;
    case EM_GROUP_REACT:
    case EM_GROUP_EVENT:
    case EM_GROUP_VERSUS:
        switch (state) {
        case MOSS_REACT_STUN:
            if (em->type == EM_KIND_BULLFANGO) {
                Em_BeginStatus5(em);
            }
            break;
        }
        break;
    case EM_GROUP_DEFEAT:
        if (state == MOSS_DEFEAT_REAPPEAR && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        break;
    case EM_GROUP_FLY:
    case EM_GROUP_EVENT2:
        break;
    }
    Em_SetStateSync(em, group, state, mode);
}

/*
 * Ends a state: on the ground, and MOSS_IDLE with the script asked for the
 * next state.
 */
void EmMosswine_EndState(Em *em)
{
    em->stance = EM_STANCE_GROUND;
    em->scriptPending = 1;
    EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE, EM_SYNC_LOCAL);
}

/* MOSS_IDLE_NONE: nothing. */
void EmMosswine_IdleNone(void)
{
}

/* MOSS_IDLE: the idle motion once, then EmMosswine_EndState. */
void EmMosswine_Idle(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE, 0xA, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ACTION_A: MOSS_MOT_IDLE_ACTION_A for 30 to 89 frames (by
 * Em.stateRand), its end motion, then EmMosswine_EndState.
 */
void EmMosswine_IdleActionA(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->timer = ((s32) em->stateRand % 60) + 0x1E;
        Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_A, 6, 0);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_A_END, 6, 0);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ACTION_B: MOSS_MOT_IDLE_ACTION_B for 240 frames of motion time,
 * then EmMosswine_EndState.
 */
void EmMosswine_IdleActionB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_B, 6, 0);
        Motion_InitTime(em, 0);
        break;
    case 1:
        if (em->motionTime >= 0xF0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ACTION_C, _TWICE and _4: MOSS_MOT_IDLE_ACTION_C `repeats`
 * times, turning toward the goal, then EmMosswine_EndState.
 */
void EmMosswine_IdleActionC(Em *em, s32 repeats)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        em->timer = repeats;
        Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_C, 6, 0);
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        if (em->motion[0].playing == 0) {
            if (--em->timer <= 0) {
                EmMosswine_EndState(em);
            }
        }
        break;
    }
}

/* MOSS_IDLE_ACTION_D: one motion, then EmMosswine_EndState. */
void EmMosswine_IdleActionD(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE_ACTION_D, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ALERT_BULLFANGO, the Bullfango's combat idle: one motion, then
 * EmMosswine_EndState.
 */
void EmMosswine_IdleAlertBullfango(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_ALERT_BULLFANGO, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_STATUS2: blinded and waking on a hit (EM_HIT_REACT_WAKE)
 * throughout; lies down (Em_BeginStatus2) with a periodic effect over it
 * until the status timer runs out, then MOSS_IDLE_STATUS2_END.
 */
void EmMosswine_Status2Lie(Em *em)
{
    Em_SetHitReactMode(em, EM_HIT_REACT_WAKE);
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_LIE, 0, 0);
        Em_BeginStatus2(em);
        break;
    case 1:
        Em_SpawnLyingPuffs(em, 0xB, (f32 *) &EmMosswine_Effect0BOfs, 1.0f);
        if (--em->timer <= 0) {
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_STATUS2_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MOSS_IDLE_STATUS2_END: ends status 2 and gets up, blinded until the
 * motion ends, then MOSS_IDLE_RECOVER.
 */
void EmMosswine_Status2End(Em *em)
{
    em->blind = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_EndStatus2(em);
        Em_SetMotion(em, MOSS_MOT_STATUS2_GET_UP, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->blind = 0;
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_RECOVER, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* MOSS_IDLE_RECOVER: one motion after status 2, then EmMosswine_EndState. */
void EmMosswine_Recover(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_RECOVER, 0, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_ALERT_MOSSWINE, the Mosswine's combat idle: one motion, then
 * EmMosswine_EndState.
 */
void EmMosswine_IdleAlertMosswine(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_ALERT_MOSSWINE, 6, 0);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_IDLE_STUN_END: guarded, gets up from the stun turning back round,
 * then EmMosswine_EndState.
 */
void EmMosswine_StunEnd(Em *em)
{
    em->guardTimer = 2;
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_DOWN_GET_UP, 0, 0);
        em->noReact = 1;
        em->rotY = em->rotY + 0x7FFF + 1;
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            em->step++;
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * Runs the state of group 0, EM_GROUP_IDLE (and of group 2, which the
 * family doesn't use).
 */
void EmMosswine_RunIdleState(Em *em)
{
    em->prevStateGroup = em->stateGroup;
    em->prevState = em->state;
    switch (em->state) {
    case MOSS_IDLE_NONE:
        EmMosswine_IdleNone();
        break;
    case MOSS_IDLE:
        EmMosswine_Idle(em);
        break;
    case MOSS_IDLE_ACTION_A:
        EmMosswine_IdleActionA(em);
        break;
    case MOSS_IDLE_ACTION_B:
        EmMosswine_IdleActionB(em);
        break;
    case MOSS_IDLE_ACTION_C:
        EmMosswine_IdleActionC(em, 1);
        break;
    case MOSS_IDLE_ACTION_D:
        EmMosswine_IdleActionD(em);
        break;
    case MOSS_IDLE_ALERT_BULLFANGO:
        EmMosswine_IdleAlertBullfango(em);
        break;
    case MOSS_IDLE_ACTION_C_TWICE:
        EmMosswine_IdleActionC(em, 2);
        break;
    case MOSS_IDLE_ACTION_C_4:
        EmMosswine_IdleActionC(em, 4);
        break;
    case MOSS_IDLE_RECOVER:
        EmMosswine_Recover(em);
        break;
    case MOSS_IDLE_ALERT_MOSSWINE:
        EmMosswine_IdleAlertMosswine(em);
        break;
    case MOSS_IDLE_STATUS2:
        EmMosswine_Status2Lie(em);
        break;
    case MOSS_IDLE_STATUS2_END:
        EmMosswine_Status2End(em);
        break;
    case MOSS_IDLE_STUN_END:
        EmMosswine_StunEnd(em);
        
    default:
        break;
    }
}

/*
 * MOSS_MOVE_TURN (quarterTurn 0) aims at the target with
 * EmMosswine_TurnParams[0]; MOSS_MOVE_TURN_QUARTER (quarterTurn 1) aims a
 * quarter turn to the right (raising rotY) with that side's turn motion.
 * Both then turn by Em_TurnStep with EmMosswine_TurnParams[0], and end with
 * EmMosswine_EndState.
 */
void EmMosswine_Turn(Em *em, s32 quarterTurn)
{
    u16 turnDiff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        if (quarterTurn == 0) {
            Em_StartTurnToTarget(em, &EmMosswine_TurnParams[0]);
        } else {
            em->aimAngle = em->rotY + 0x4000;
            turnDiff = em->aimAngle - em->rotY;
            if (turnDiff <= 0x1000U || turnDiff >= 0xF000U) {
                Player_SetFlag((Player *) em, EM_FLAG_NO_ROOT_MOTION);
                Em_SetMotion(em, MOSS_MOT_WALK, 0, 0);
            } else if (turnDiff >= 0x8000U) {
                Em_SetMotion(em, MOSS_MOT_TURN_LEFT, 0, 0);
            } else {
                Em_SetMotion(em, MOSS_MOT_TURN_RIGHT, 0, 0);
            }
        }
        break;
    case 1:
        if (Em_TurnStep(em, &EmMosswine_TurnParams[0]) == 1) {
            Player_ClearFlag(em, EM_FLAG_NO_ROOT_MOTION);
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_MOVE_WALK: walks toward the goal, turning to it, until within 200 or
 * the time is up, then EmMosswine_EndState.
 */
void EmMosswine_Walk(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_WALK, 6, 0);
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
            EmMosswine_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/* MOSS_MOVE_RUN: as EmMosswine_Walk, running, until within 100. */
void EmMosswine_Run(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_RUN, 6, 0);
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            EmMosswine_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/* MOSS_MOVE_WALK_B: as EmMosswine_Walk with MOSS_MOT_WALK_B. */
void EmMosswine_WalkB(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_WALK_B, 6, 0);
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
            EmMosswine_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/*
 * MOSS_MOVE_WALK_SCRIPTED: as EmMosswine_Walk, facing the goal at once;
 * restarts the script at the end.
 */
void EmMosswine_WalkScripted(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_WALK, 0, 0);
        em->rotY = em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 200.0f || --em->timer <= 0) {
            Em_RestartScript(em);
            EmMosswine_EndState(em);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/*
 * MOSS_MOVE_WANDER_HOME: walks for 120 to 247 frames (by Em.stateRand),
 * turning slowly toward its home; a remote copy only follows. Then
 * EmMosswine_EndState.
 */
void EmMosswine_WanderHome(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    s32 timer;

    switch (em->step) {
    case 0:
        em->step++;
        em->timer = (em->stateRand & 0x7F) + 0x78;
        Em_SetMotion(em, MOSS_MOT_WALK, 6, 0);
        break;
    case 1:
        timer = em->timer;
        if (timer > 0) {
            em->timer = timer - 1;
        }
        if ((em->remote == 0) && (em->timer <= 0)) {
            EmMosswine_EndState(em);
            break;
        }
        em->targetAngle = Math_AngleXZ(&em->posX, w->home) & 0xFFFF;
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x80);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        break;
    }
}

/* Runs the state of group 1, EM_GROUP_MOVE. */
void EmMosswine_RunMoveState(Em *em)
{
    switch (em->state) {
    case MOSS_MOVE_TURN:
        EmMosswine_Turn(em, 0);
        break;
    case MOSS_MOVE_WALK:
        EmMosswine_Walk(em);
        break;
    case MOSS_MOVE_RUN:
        EmMosswine_Run(em);
        break;
    case MOSS_MOVE_WALK_B:
        EmMosswine_WalkB(em);
        break;
    case MOSS_MOVE_WALK_SCRIPTED:
        EmMosswine_WalkScripted(em);
        break;
    case MOSS_MOVE_WANDER_HOME:
        EmMosswine_WanderHome(em);
        break;
    case MOSS_MOVE_TURN_QUARTER:
        EmMosswine_Turn(em, 1);
        
    default:
        break;
    }
}

/*
 * The charges (MOSS_ATK_CHARGE_A to _F, by `charge`): the contact attack of
 * the charge (EmContactAttack rows 3, 8, 9, 0xA, 0xE, 0xF), then a run at a
 * goal 100 past the target's position until within 100 of it or the
 * charge's time is up (cut to 15 frames when the target is gone or in
 * another area); the end motion, then EmMosswine_EndState.
 */
void EmMosswine_Charge(Em *em, u32 charge)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_CHARGE, 0xA, 0);
        switch (charge) {
        case 0:
            EmContactAttack_Create(em, 3);
            break;
        case 1:
            EmContactAttack_Create(em, 8);
            break;
        case 2:
            EmContactAttack_Create(em, 9);
            break;
        case 3:
            EmContactAttack_Create(em, 0xA);
            break;
        case 4:
            EmContactAttack_Create(em, 0xE);
            break;
        case 5:
            EmContactAttack_Create(em, 0xF);
            break;
        }
        ang[0] = 0;
        ang[1] = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, mtx);
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 100.0f;
        Vec_Rotate(out, ofs, mtx);
        em->goal[0] += out[0];
        em->goal[2] += out[2];
        em->targetAngle = Math_AngleXZ(&em->posX, em->goal) & 0xFFFF;
        break;
    case 1:
        if (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) < 100.0f || --em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_CHARGE_END, 0xA, 0);
            break;
        }
        Angle_TurnToward((s32 *) &em->rotY, (s32 *) &em->targetAngle, 0x200);
        Mtx_FromAnglesXYZ((s32 *) &em->rotX, em->rotMtx[0]);
        if (em->timer > 0xF) {
            if (em->targetNo == -1) {
                em->timer = 15;
            } else if (em->area != ((Player *) &gPlayers[em->targetNo])->area) {
                em->timer = 15;
            }
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/* Runs the state of group 3, EM_GROUP_ATTACK: the six charges. */
void EmMosswine_RunAttackState(Em *em)
{
    switch (em->state) {
    case MOSS_ATK_CHARGE_A:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_A);
        break;
    case MOSS_ATK_CHARGE_B:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_B);
        break;
    case MOSS_ATK_CHARGE_C:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_C);
        break;
    case MOSS_ATK_CHARGE_D:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_D);
        break;
    case MOSS_ATK_CHARGE_E:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_E);
        break;
    case MOSS_ATK_CHARGE_F:
        EmMosswine_Charge(em, MOSS_ATK_CHARGE_F);
        
    default:
        break;
    }
}

/*
 * MOSS_REACT_FLINCH: a flinch motion by the hit's side (the front, side A
 * or side B), the script restarted, then EmMosswine_EndState.
 */
void EmMosswine_Flinch(Em *em)
{
    s32 diff;

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        diff = (em->hitAngle - em->rotY) & 0xFFFF;
        if ((diff > 24576) && (diff < 0xA000)) {
            Em_SetMotion(em, MOSS_MOT_FLINCH_FRONT, 0, 0);
        } else if (diff < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_FLINCH_A, 0, 0);
        } else {
            Em_SetMotion(em, MOSS_MOT_FLINCH_B, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_REACT_BLOWN_AWAY: the motion by the hit's side; from frame 10 thrown
 * away from the hit (up 8, back 21), lands and lies 150 frames, gets up,
 * then EmMosswine_EndState.
 */
void EmMosswine_BlownAway(Em *em)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_BLOWN_A, 0, 0);
        } else {
            Em_SetMotion(em, MOSS_MOT_BLOWN_B, 0, 0);
        }
        Em_RestartScript(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            em->step++;
            ang[0] = 0;
            ang[1] = em->hitAngle + 0x8000;
            ang[2] = 0;
            Mtx_FromAnglesXYZ((s32 *) ang, mtx);
            ofs[0] = 0.0f;
            ofs[1] = 8.0f;
            ofs[2] = -21.0f;
            Vec_Rotate(out, ofs, mtx);
            Player_ClearAccel2(em);
            em->velX = out[0];
            em->velY = out[1];
            em->velZ = out[2];
            em->accelY = -1.09f;
            em->accelZ = 0.28f;
            em->stance = EM_STANCE_AIR;
        }
        break;
    case 2:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MOSS_MOT_BLOWN_LAND, 6, 0);
            em->timer = 150;
        }
        break;
    case 3:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_BLOWN_GET_UP, 6, 0);
        }
        break;
    case 4:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/* MOSS_REACT_BLOWN_AWAY_QUICK: as EmMosswine_BlownAway, thrown at once. */
void EmMosswine_BlownAwayQuick(Em *em)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_BLOWN_A, 0, 0xA);
        } else {
            Em_SetMotion(em, MOSS_MOT_BLOWN_B, 0, 0xA);
        }
        ang[0] = 0;
        ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, mtx);
        ofs[0] = 0.0f;
        ofs[1] = 8.0f;
        ofs[2] = -21.0f;
        Vec_Rotate(out, ofs, mtx);
        Player_ClearAccel2(em);
        em->velX = out[0];
        em->velY = out[1];
        em->velZ = out[2];
        em->accelY = -1.09f;
        em->accelZ = 0.28f;
        Em_RestartScript(em);
        break;
    case 1:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MOSS_MOT_BLOWN_LAND, 6, 0);
            em->timer = 150;
        }
        break;
    case 2:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_BLOWN_GET_UP, 6, 0);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_REACT_STATUS4: Em_BeginStatus4, down with the body turned away from
 * the hit and an effect on bone 2 while Em.timer counts down; then gets up
 * turning back (Em_EndStatus4), reactions blocked until then, and
 * EmMosswine_EndState.
 */
void EmMosswine_Status4(Em *em)
{
    s32 timer;

    Em_SetHitReactMode(em, EM_HIT_REACT_STATUS);
    timer = em->timer;
    if (timer > 0) {
        em->timer = timer - 1;
    }
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_DOWN, 0, 0);
        em->rotY = em->hitAngle + 0x7FFF + 1;
        Em_BeginStatus4(em);
        em->noReact = 1;
        Em_RestartScript(em);
        break;
    case 1:
        Em_SpawnStatus4Effect(em, 2);
        if (em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_DOWN_GET_UP, 0, 0);
            em->rotY = em->rotY + 0x7FFF + 1;
            Em_EndStatus4(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->noReact = 0;
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_REACT_DOWN: as EmMosswine_Status4 without the status: down while
 * Em.timer counts, gets up, EmMosswine_EndState.
 */
void EmMosswine_Down(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_DOWN, 0, 0);
        Em_RestartScript(em);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_DOWN_GET_UP, 0, 0);
            em->rotY = em->rotY + 0x7FFF + 1;
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            EmMosswine_EndState(em);
        }
        break;
    }
}

/*
 * MOSS_REACT_STUN (status 5, the Bullfango only): down with the stun's
 * stars (StageObj_Stun, for the low half of Em.timer frames) until Em.timer
 * runs out, then MOSS_IDLE_STUN_END.
 */
void EmMosswine_Stun(Em *em)
{
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_DOWN, 0, 0);
        Em_RestartScript(em);
        em->noReact = 1;
        StunStars_CreateEm(em, 1, 0, *(s16 *)&em->timer, 100.0f, 1.2f);
        break;
    case 1:
        if (--em->timer <= 0) {
            em->step++;
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    case 2:
        if (em->remote == 0) {
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_STUN_END, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* Runs the state of group 4, EM_GROUP_REACT. */
void EmMosswine_RunReactState(Em *em)
{
    switch (em->state) {
    case MOSS_REACT_FLINCH:
        EmMosswine_Flinch(em);
        break;
    case MOSS_REACT_BLOWN_AWAY:
        EmMosswine_BlownAway(em);
        break;
    case MOSS_REACT_BLOWN_AWAY_QUICK:
        EmMosswine_BlownAwayQuick(em);
        break;
    case MOSS_REACT_STATUS4:
        EmMosswine_Status4(em);
        break;
    case MOSS_REACT_DOWN:
        EmMosswine_Down(em);
        break;
    case MOSS_REACT_STUN:
        EmMosswine_Stun(em);
        
    default:
        break;
    }
}

/*
 * MOSS_DEFEAT_DIE: unhittable; hunted (QuestEm_Hunted), the motion by the
 * hit's side, thrown away from frame 10, lands, dies and lies as a carcass
 * with a carve point (EmMine) for 2400 frames (a Mosswine) or 900 (a
 * Bullfango) or until it is carved out, fades out over 60 frames, then
 * MOSS_DEFEAT_REAPPEAR.
 */
void EmMosswine_Die(Em *em)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_GROUND;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_BLOWN_A, 0, 0);
        } else {
            Em_SetMotion(em, MOSS_MOT_BLOWN_B, 0, 0);
        }
        QuestEm_Hunted(em);
        break;
    case 1:
        if (EmMotion_PassedFrameScaled(em, 10.0f, 0) != 0) {
            em->step++;
            ang[0] = 0;
            ang[1] = em->hitAngle + 0x8000;
            ang[2] = 0;
            Mtx_FromAnglesXYZ((s32 *) ang, mtx);
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = -19.0f;
            Vec_Rotate(out, ofs, mtx);
            Player_ClearAccel2(em);
            em->velY = 20.0f;
            em->velX = out[0];
            em->velZ = out[2];
            ofs[2] = 0.4f;
            Vec_Rotate(out, ofs, mtx);
            em->accelY = -2.08f;
            em->accelX = out[0];
            em->accelZ = out[2];
            em->stance = EM_STANCE_AIR;
        }
        break;
    case 2:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (!((em->velX * em->accelX) < 0.0f)) {
            em->accelX = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MOSS_MOT_BLOWN_LAND, 6, 0);
            em->timer = 150;
            Player_ClearVelocity(em);
        }
        break;
    case 3:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_DIE, 0, 0);
            em->timer = 60;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 4:
        EmMine_Update(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->type == EM_KIND_MOSSWINE) {
                em->timer = 2400;
            } else {
                em->timer = 900;
            }
        }
        break;
    case 5:
        if (--em->timer <= 0 || EmMine_Update(em) <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 6:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmMosswine_ChangeState(em, EM_GROUP_DEFEAT, MOSS_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/* MOSS_DEFEAT_DIE_QUICK: as EmMosswine_Die, thrown at once. */
void EmMosswine_DieQuick(Em *em)
{
    f32 ofs[4];
    f32 out[4];
    s32 ang[4];
    f32 mtx[16];

    em->noHitTimer = 0xA;
    em->noPushTimer = 0xA;
    Em_SetCombat(em, 0, 0);
    switch (em->step) {
    case 0:
        em->step++;
        em->stance = EM_STANCE_AIR;
        ang[0] = (u16) (em->hitAngle - em->rotY);
        if (ang[0] < 0x8000) {
            Em_SetMotion(em, MOSS_MOT_BLOWN_A, 0, 0xA);
        } else {
            Em_SetMotion(em, MOSS_MOT_BLOWN_B, 0, 0xA);
        }
        QuestEm_Hunted(em);
        ang[0] = 0;
        ang[1] = em->hitAngle + 0x8000;
        ang[2] = 0;
        Mtx_FromAnglesXYZ((s32 *) ang, mtx);
        ofs[0] = 0.0f;
        ofs[1] = 8.0f;
        ofs[2] = -21.0f;
        Vec_Rotate(out, ofs, mtx);
        Player_ClearAccel2(em);
        em->velX = out[0];
        em->velY = out[1];
        em->velZ = out[2];
        em->accelY = -1.09f;
        em->accelZ = 0.28f;
        break;
    case 1:
        if (!((em->velZ * em->accelZ) < 0.0f)) {
            em->accelZ = 0.0f;
        }
        if (Player_MoveFall((Player *) em)) {
            em->step++;
            em->stance = EM_STANCE_GROUND;
            Em_SetMotion(em, MOSS_MOT_BLOWN_LAND, 6, 0);
            em->timer = 150;
            Player_ClearVelocity(em);
        }
        break;
    case 2:
        if (em->motion[0].playing == 0) {
            em->step++;
            Em_SetMotion(em, MOSS_MOT_DIE, 0, 0);
            em->timer = 60;
            EmMine_Create(em, 0);
            em->fieldCollide = 0;
        }
        break;
    case 3:
        EmMine_Update(em);
        if (em->motion[0].playing == 0) {
            em->step++;
            if (em->type == EM_KIND_MOSSWINE) {
                em->timer = 2400;
            } else {
                em->timer = 900;
            }
        }
        break;
    case 4:
        if (--em->timer <= 0 || EmMine_Update(em) <= 0) {
            em->step++;
            EmMine_Remove(em);
        }
        break;
    case 5:
        em->alpha -= 1.0f / 60.0f;
        if (em->alpha <= 0.0f) {
            em->visible = 0;
            EmMosswine_ChangeState(em, EM_GROUP_DEFEAT, MOSS_DEFEAT_REAPPEAR, EM_SYNC_OWNER_FLAGGED);
        }
        break;
    }
}

/*
 * MOSS_DEFEAT_REAPPEAR: when the quest lets the monster appear again
 * (QuestEm_CanAppear), MOSS_IDLE; refused, it advances its task phase.
 */
void EmMosswine_Reappear(Em *em)
{
    switch (em->step) {
    case 0:
        if (QuestEm_CanAppear(em) == 1) {
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE, EM_SYNC_LOCAL);
            break;
        }
        em->taskPhase++;
    case 1:
        break;
    }
}

/* Runs the state of group 5, EM_GROUP_DEFEAT. */
void EmMosswine_RunDefeatState(Em *em)
{
    switch (em->state) {
    case MOSS_DEFEAT_DIE:
        EmMosswine_Die(em);
        break;
    case MOSS_DEFEAT_DIE_QUICK:
        EmMosswine_DieQuick(em);
        break;
    case MOSS_DEFEAT_REAPPEAR:
        EmMosswine_Reappear(em);
        break;
    }
}

/*
 * MOSS_EVENT_HIDE_A: hidden for one step (the script not asked), then shown
 * and EmMosswine_EndState.
 */
void EmMosswine_EventHideA(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmMosswine_EndState(em);
        break;
    }
}

/* MOSS_EVENT_HIDE_B: the same as EmMosswine_EventHideA. */
void EmMosswine_EventHideB(Em *em)
{
    em->senseDelay = 5;
    em->noHitTimer = 5;
    switch (em->step) {
    case 0:
        em->step++;
        em->lookAtTarget = 0;
        em->stance = EM_STANCE_GROUND;
        Em_SetMotion(em, MOSS_MOT_IDLE, 0, 0);
        em->visible = 0;
        em->scriptPending = 0;
        break;
    case 1:
        em->step++;
        em->visible = 1;
        EmMosswine_EndState(em);
        break;
    }
}

/* Runs the state of group 6, EM_GROUP_EVENT. */
void EmMosswine_RunEventState(Em *em)
{
    switch (em->state) {
    case MOSS_EVENT_HIDE_A:
        EmMosswine_EventHideA(em);
        break;
    case MOSS_EVENT_HIDE_B:
        EmMosswine_EventHideB(em);
        break;
    }
}

/*
 * The family's update, run every frame by GmEm_Update: counts
 * EmMosswineWork.unk00C down; then Em_UpdateDamage's result picks the
 * reaction: a death (MOSS_DEFEAT_DIE when in the air, else
 * MOSS_DEFEAT_DIE_QUICK), the blow-aways, status 4 and status 2 unless
 * being blown away, a flinch or blow-away for a part hit, a Bullfango's
 * stun (a Mosswine's ends at once). Then runs the state (twice when one
 * asked for it).
 */
void EmMosswine_Update(Em *em)
{
    EmMosswineWork *w = (EmMosswineWork *)&em->work;
    u8 hitInfo[3];

    if (w->unk00C != 0) {
        (w->unk00C)--;
    }
    switch (Em_UpdateDamage(em, hitInfo) & 0xFF) {
    case EM_DMG_NONE:
    case EM_DMG_ATTACK9:
    case EM_DMG_ATTACK6:
    case EM_DMG_FLASH:
    case EM_DMG_PART8_DOWN:
    case EM_DMG_HIT:
    case EM_DMG_ATTACK5:
    case EM_DMG_ATTACK5_FLINCH:
    case EM_DMG_ATTACK13:
        break;
    case EM_DMG_KILLED:
    case EM_DMG_DEAD:
        if (em->stance == EM_STANCE_AIR) {
            EmMosswine_ChangeState(em, EM_GROUP_DEFEAT, MOSS_DEFEAT_DIE, EM_SYNC_OWNER);
        } else {
            EmMosswine_ChangeState(em, EM_GROUP_DEFEAT, MOSS_DEFEAT_DIE_QUICK, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS_IN_AIR:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY) && (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY_QUICK)) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_STATUS4:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY) && (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY_QUICK)) {
            Em_SetStatus4Timer(em);
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_STATUS4, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_CAPTURED:
        EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        break;
    case EM_DMG_STATUS2:
        if ((em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY) && (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY_QUICK)) {
            Em_SetStatus2Timer(em);
            EmMosswine_ChangeState(em, EM_GROUP_IDLE, MOSS_IDLE_STATUS2, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_LYING:
        em->blind = 0;
        Em_EndStatus2(em);
        if (em->stance == EM_STANCE_AIR) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        } else {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_PART_FLINCH:
        if (em->stance == EM_STANCE_AIR) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        } else if ((s16) Player_IsState(em, 4, 0)) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY, EM_SYNC_OWNER);
        } else {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_FLINCH, EM_SYNC_OWNER);
        }
        break;
    case EM_DMG_HIT_MODE3:
        if (em->stance == EM_STANCE_AIR) {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY_QUICK, EM_SYNC_OWNER);
        } else {
            EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_BLOWN_AWAY, EM_SYNC_OWNER);
        }
        Em_ResetPartHp(em, 0);
        break;
    case EM_DMG_STATUS5:
        if (em->type == EM_KIND_BULLFANGO) {
            if ((em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_STUN) && (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY) &&
                (em->stateGroup != EM_GROUP_REACT || em->state != MOSS_REACT_BLOWN_AWAY_QUICK)) {
                Em_SetStatus5Timer(em);
                EmMosswine_ChangeState(em, EM_GROUP_REACT, MOSS_REACT_STUN, EM_SYNC_OWNER);
            }
        } else {
            Em_EndStatus5(em);
        }
        break;
    }
    Em_UpdatePendingScript(em);
    Em_ClearHitReactMode(em);
    EmMosswine_RunState(em);
    if (em->stateRequest != 0) {
        EmMosswine_RunState(em);
        em->stateRequest = 0;
    }
}

/* Runs the monster's state through its group's dispatcher. */
void EmMosswine_RunState(Em *em)
{
    switch (em->stateGroup) {
    case EM_GROUP_IDLE:
        EmMosswine_RunIdleState(em);
        break;
    case EM_GROUP_MOVE:
        EmMosswine_RunMoveState(em);
        break;
    case EM_GROUP_FLY:
        EmMosswine_RunIdleState(em);
        break;
    case EM_GROUP_ATTACK:
        EmMosswine_RunAttackState(em);
        break;
    case EM_GROUP_REACT:
        EmMosswine_RunReactState(em);
        break;
    case EM_GROUP_DEFEAT:
        EmMosswine_RunDefeatState(em);
        break;
    case EM_GROUP_EVENT:
        EmMosswine_RunEventState(em);
        break;
    case EM_GROUP_EVENT2:
        EmMosswine_RunReactState(em);
        
    default:
        break;
    }
}

/*
 * Callback slots 0 and 1 (EmVt.init, update): makes the monster's effect
 * 01, as the other families do.
 */
void EmMosswine_SpawnEffect01(u8 *em)
{
    Shadow_Spawn((Player *) em, 0);
}

/* Callback slot 2: nothing. */
void EmMosswine_TableNop(void)
{
}

/*
 * Callback slot 4: the script's question about the monster. Query 0: is its
 * leader (Em.link) in its area and in its group 0 state 6. Returns 1 or 0.
 */
s32 EmMosswine_ScriptQuery(Em *em, u8 query)
{
    Em *leader;

    switch (query) {
    case 0:
        leader = em->link;
        if (leader != NULL && em->area == leader->area && (s16) Player_IsState(leader, 0, 6)) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* The file's initialised data, in the game's order */
void GmEm_UpdateSfx(Em *em); /* gm_sub */
f32 EmMosswine_VariantParams[3][6] = {
#include "game/008C8B30.inc"
};
EmTurnParams EmMosswine_TurnParams[1] = {
#include "game/008C8B80.inc"
};
f32 EmMosswine_Effect0BOfs[3] = {
#include "game/008C8BA0.inc"
};
void *EmMosswine_Callbacks[8] = {
    EmMosswine_SpawnEffect01, EmMosswine_SpawnEffect01, EmMosswine_TableNop, GmEm_UpdateSfx,
    EmMosswine_ScriptQuery, EmMosswine_Reinit, NULL, NULL,
};
