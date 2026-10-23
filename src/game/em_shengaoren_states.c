/*
 * Shen Gaoren's state changes (EM_KIND_SHEN_GAOREN):
 * EmShenGaoren_SetIdleState to EmShenGaoren_SetReactState start a state of
 * their group (em_shengaoren.h names the states), and
 * EmShenGaoren_ChangeState picks the group's function. The
 * game's code places them after the Blangonga family's code, apart from the
 * rest of the family, with Kushala Daora's state changes between them and
 * the family's first function.
 */
#include "common.h"
#include "em.h"
#include "em_shengaoren.h"
#include "vec_math.h"
#include "em_script.h"
#include "quest.h"
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmShenGaoren_SetIdleState and the other Set functions compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
/* end of prototypes */

/*
 * Starts an idle state: resets motion speed, sets SHEN_STANCE_LOW for
 * SHEN_IDLE_LOWER and SHEN_STANCE_TALL for SHEN_IDLE_RISE_A, and sets the state
 * (Em_SetStateSync).
 */
void EmShenGaoren_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case SHEN_IDLE_LOWER:
        em->stance = SHEN_STANCE_LOW;
        break;
    case SHEN_IDLE_RISE_A:
        em->stance = SHEN_STANCE_TALL;
        break;
    }
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Starts a move state: sets hasGoal and goalDist (500 without a goal kind);
 * the walk-back states become the forward walks when the goal is ahead
 * (within a quarter turn).
 */
void EmShenGaoren_SetMoveState(Em *em, u16 state, u8 mode)
{
    s32 angle;

    em->motionSpeed = 1.0f;
    switch (state) {
    case SHEN_MOVE_WALK_TALL:
    case SHEN_MOVE_WALK_LOW:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        break;
    case SHEN_MOVE_BACK_TALL:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        angle = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= angle && angle <= ANGLE_TURN / 4) || (angle >= (ANGLE_TURN * 3 / 4) && angle <= 0xFFFF)) {
            state = SHEN_MOVE_WALK_TALL;
        }
        break;
    case SHEN_MOVE_BACK_LOW:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->goalDist = 500.0f;
        } else {
            em->hasGoal = 1;
            em->goalDist = Vec_DistanceXZ(&em->posX, em->goal);
        }
        angle = (u16) (Math_AngleXZ(&em->posX, em->goal) - em->rotY);
        if ((0 <= angle && angle <= ANGLE_TURN / 4) || (angle >= (ANGLE_TURN * 3 / 4) && angle <= 0xFFFF)) {
            state = SHEN_MOVE_WALK_LOW;
        }
        break;
    case SHEN_MOVE_TURN_LOW:
    case SHEN_MOVE_TURN_TALL:
    case SHEN_MOVE_TURN_AWAY_TALL:
    case SHEN_MOVE_TURN_AWAY_LOW:
        Math_AngleXZ(&em->posX, em->goal);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Enters a fly state (EM_GROUP_FLY; the family has none): motion speed 1,
 * Em_SetStateSync.
 */
void EmShenGaoren_SetFlyState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, 2, state, mode);
}

/*
 * Resets motion speed and sets the attack state (Em_SetStateSync).
 */
void EmShenGaoren_SetAttackState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    Em_SetStateSync(em, 3, state, mode);
}

/*
 * Starts a react state: marks the first flinch of the leg (partFlinch 1)
 * for the first leg flinches, its second (2) for the AGAIN ones, and for
 * SHEN_REACT_LEGS_COLLAPSE and SHEN_REACT_LEGS_COLLAPSE_SHOT clears the
 * four leg counts and breaks part 1 (Em_BreakPart).
 */
void EmShenGaoren_SetReactState(Em *em, u16 state, u8 mode)
{
    EmShenGaorenWork *w = (EmShenGaorenWork *)&em->work;
    em->motionSpeed = 1.0f;
    switch (state) {
    case SHEN_REACT_LEG_B_LOW:
    case SHEN_REACT_LEG_D_LOW:
    case SHEN_REACT_LEG_C_LOW:
    case SHEN_REACT_LEG_A_LOW:
        w->partFlinch[state + 1] = 1;
        break;
    case SHEN_REACT_LEGS_COLLAPSE:
    case SHEN_REACT_LEGS_COLLAPSE_SHOT:
        w->partFlinch[3] = 0;
        w->partFlinch[4] = 0;
        w->partFlinch[5] = 0;
        w->partFlinch[6] = 0;
        Em_BreakPart(em, 1U);
        break;
    case SHEN_REACT_LEG_A_TALL:
    case SHEN_REACT_LEG_B_TALL:
    case SHEN_REACT_LEG_C_TALL:
    case SHEN_REACT_LEG_D_TALL:
        w->partFlinch[state - 6] = 1;
        break;
    case SHEN_REACT_LEG_A_AGAIN_LOW:
    case SHEN_REACT_LEG_B_AGAIN_LOW:
    case SHEN_REACT_LEG_C_AGAIN_LOW:
    case SHEN_REACT_LEG_D_AGAIN_LOW:
        w->partFlinch[state - 14] = 2;
        break;
    case SHEN_REACT_FLINCH_TALL:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_REACT, state, mode);
}

/*
 * Moves to (group, state): on the host first targets a player in range
 * (Em_TargetPlayerInRange); a remote copy ignores EM_SYNC_OWNER and
 * EM_SYNC_OWNER_FLAGGED; runs the group's Set function (the defeat and event
 * groups straight through Em_SetStateSync).
 */
void EmShenGaoren_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmShenGaoren_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmShenGaoren_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmShenGaoren_SetFlyState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmShenGaoren_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_REACT:
        EmShenGaoren_SetReactState(em, state, mode);
        return;
    case EM_GROUP_DEFEAT:
    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);

    default:
        return;
    }
}

