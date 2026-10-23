/*
 * Kushala Daora's wind, state changes and flight curves (the Kushala family:
 * Kushala Daora, its rusted form, Lunastra and Teostra; include/em_kushala.h
 * names the states). EmKushala_SetWind, WindActive and WindActiveTesca turn
 * the wind (or the aura of Lunastra and Teostra) on and test it;
 * EmKushala_SetIdleState .. SetReactState start a state of their group and
 * EmKushala_ChangeState picks the group's function; EmKushala_StartCurves
 * and UpdateCurves move the monster along its height and speed curves. The
 * game's code places these functions after Shen Gaoren's state changes,
 * apart from the rest of the family, and their curve tables after the
 * Blangonga family's data.
 */
#include "common.h"
#include "em.h"
#include "em_kushala.h"
#include "player.h"
#include "engine_math.h"
#include "vec_math.h"
#include "em_script.h"
#include "quest.h"
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s16 n), the calls in EmBlangonga_EatNeed2 compile differently */
void Em_AddHp(Em *em, s32 n);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmBlangonga_ChangeState, EmKushala_ChangeState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
s32 Em_PartBroken(Em *em, s32 arg1);
/* unit's own declaration: Em_EndAllStatus reads a0 = em at EmKushala_SetReactState state 6 (left by EmKushala_SetWind(em, ...), which keeps a0) */
void Em_EndAllStatus();
/* end of prototypes */

/*
 * Turns the wind on (on 1) or off: sets windOn and windFrames. The first
 * argument is unused.
 */
void EmKushala_SetWind(void *unused, s32 w, s32 on)
{
    if (on == 1) {
        ((EmKushalaWork *)w)->windOn = 1;
        ((EmKushalaWork *)w)->windFrames = 90;
        return;
    }
    ((EmKushalaWork *)w)->windOn = 0;
    ((EmKushalaWork *)w)->windFrames = 0;
}

/*
 * Returns 1 while Kushala Daora's wind blows: windOn set, and not blinded,
 * poisoned, under a status, stunned, or with part 0 or 2 broken.
 */
s32 EmKushala_WindActive(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    if ((em->blindTimer > 0) || (em->poisonDamage > 0) || (em->hitReactMode == EM_HIT_REACT_STATUS) || ((em->stateGroup == EM_GROUP_REACT) && (em->state == KUSH_REACT_STUN))) {
        return 0;
    }
    if (Em_PartBroken(em, 2) == 1) {
        return 0;
    }
    if (Em_PartBroken(em, 0) == 1) {
        return 0;
    }
    return w->windOn != 0;
}

/* The same test for Lunastra and Teostra (an identical copy). */
s32 EmKushala_WindActiveTesca(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    if ((em->blindTimer > 0) || (em->poisonDamage > 0) || (em->hitReactMode == EM_HIT_REACT_STATUS) || ((em->stateGroup == EM_GROUP_REACT) && (em->state == KUSH_REACT_STUN))) {
        return 0;
    }
    if (Em_PartBroken(em, 2) == 1) {
        return 0;
    }
    if (Em_PartBroken(em, 0) == 1) {
        return 0;
    }
    return w->windOn != 0;
}

/*
 * Enters a group 0 (idle) state: KUSH_IDLE becomes KUSH_IDLE_ALERT in
 * combat; starts or stops the wind and the statuses each state needs, then
 * Em_SetStateSync.
 */
void EmKushala_SetIdleState(Em *em, u16 state, u8 mode)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case KUSH_IDLE:
        if (em->combat == 1) {
            state = KUSH_IDLE_ALERT;
        }
        break;
    case KUSH_IDLE_REST:
        Em_ClearPart(em, 0);
        em->parts[0].state = KUSH_IDLE_STAND;
        Em_ResetNotice(em);
        EmKushala_SetWind(em, (s32) w, 0);
        break;
    case KUSH_IDLE_STATUS2_COLLAPSE:
        Em_BeginStatus2(em);
        EmKushala_SetWind(em, (s32) w, 0);
        break;
    case KUSH_IDLE_STATUS1_COLLAPSE:
        Em_BeginStatus1(em);
        EmKushala_SetWind(em, (s32) w, 0);
        break;
    case KUSH_IDLE_EAT_NEED1:
        Em_ClearPart(em, 0);
        em->parts[0].state = KUSH_IDLE_STAND;
        Em_AddHp(em, 0.05f * em->hpMax);
        Em_AddNeed1(em, em->need1Max);
        Em_ClearNeed1State(em);
        break;
    case KUSH_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    case KUSH_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case KUSH_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case KUSH_IDLE_EAT:
        Em_StartGoalAction(em, 10000);
        break;
    case KUSH_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case KUSH_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case KUSH_IDLE_EAT_NEED2:
        Em_AddHp(em, 0.05f * em->hpMax);
        Em_AddNeed2(em, em->need2Max);
        Em_ClearNeed2State(em);
        break;
    case KUSH_IDLE_WIND_ROAR_OR_SWEEP:
        if (em->remote == 0 && w->windOn != 0) {
            if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
                EmKushala_ChangeState(em, EM_GROUP_ATTACK, KUSH_ATK_TURN_SWEEP_B, EM_SYNC_SEND);
                return;
            }
            state = KUSH_IDLE_ALERT;
            break;
        }
        EmKushala_SetWind(em, (s32) w, 1);
        break;
    case KUSH_IDLE_WIND_ROAR:
        EmKushala_SetWind(em, (s32) w, 1);
        break;
    case KUSH_IDLE_WIND_ROAR_B:
        em->spawnParam = 1;
        EmKushala_SetWind(em, (s32) w, 1);
        break;
    case KUSH_IDLE_WIND_ROAR_C:
        em->spawnParam = 2;
        EmKushala_SetWind(em, (s32) w, 1);
        break;
    case KUSH_IDLE_STAND:
    case KUSH_IDLE_HOVER:
    case KUSH_IDLE_ALERT:
    case KUSH_IDLE_EAT_NEED1_START:
    case KUSH_IDLE_ACTION_A:
    case KUSH_IDLE_SHAKE_OFF:
    case KUSH_IDLE_EAT_START:
    case KUSH_IDLE_ACTION_B:
    case KUSH_IDLE_ALERT_B:
    case KUSH_IDLE_SETTLE:
    case KUSH_IDLE_EAT_NEED2_START:
    case KUSH_IDLE_ROAR:
        break;
    }
    Em_SetStateSync(em, 0, state, mode);
}

/*
 * Enters a group 1 (move) state: the walk becomes the limp at low hp, the
 * turn and the landing their combat forms; the side dashes keep their goal;
 * sets the speed to the goal, then Em_SetStateSync.
 */
void EmKushala_SetMoveState(Em *em, u16 state, u8 mode)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;
    f32 v[3];
    s32 rot[3];
    f32 m[16];
    f32 f;

    em->motionSpeed = 1.0f;
    switch (state) {
    case KUSH_MOVE_WALK:
    case KUSH_MOVE_LIMP:
        if (em->type == EM_KIND_KUSHALA_DAORA || em->type == EM_KIND_RUSTED_KUSHALA_DAORA) {
            f = 0.12f;
        } else {
            f = 0.15f;
        }
        if (em->hp < (s16) (em->hpMax * f)) {
            state = KUSH_MOVE_LIMP;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case KUSH_MOVE_HOVER_FORWARD:
    case KUSH_MOVE_HOVER_BACKWARD:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case KUSH_MOVE_HOVER_DASH:
        Em_SetSpeedScaled(em, 0, 1000.0f, 800.0f);
        break;
    case KUSH_MOVE_HOVER_FORWARD_PAST:
        Em_SetSpeedScaled(em, 0, 1000.0f, -200.0f);
        break;
    case KUSH_MOVE_TURN:
        if (em->combat == 1) {
            state = KUSH_MOVE_TURN_ALERT;
        }
        break;
    case KUSH_MOVE_LAND:
        if (em->combat == 1) {
            state = KUSH_MOVE_LAND_QUICK;
        }
        break;
    case KUSH_MOVE_SIDE_DASH_A:
    case KUSH_MOVE_SIDE_DASH_B:
        if (em->remote == 0) {
            em->timer = Vec_DistanceXZ(&em->posX, em->goal);
            Vec_Copy(w->dashGoal, em->goal);
        } else {
            rot[0] = 0;
            rot[1] = em->rotY;
            rot[2] = 0;
            Vec_Set(v, 0.0f, 0.0f, em->timer);
            Mtx_FromAnglesYXZ(rot, m);
            Vec_Rotate(w->dashGoal, v, m);
            w->dashGoal[0] += em->posX;
            w->dashGoal[1] += em->posY;
            w->dashGoal[2] += em->posZ;
        }
        break;
    case KUSH_MOVE_WALK_B:
    case KUSH_MOVE_RUN:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case KUSH_MOVE_RUN_PAST:
    case KUSH_MOVE_RUN_PAST_ARC_A:
    case KUSH_MOVE_RUN_PAST_ARC_B:
    case KUSH_MOVE_RUN_PAST_ARC_C:
    case KUSH_MOVE_RUN_PAST_ARC_D:
        Em_SetSpeedScaled(em, 0, 1000.0f, -700.0f);
        break;
    case KUSH_MOVE_TURN_ALERT:
    case KUSH_MOVE_STOMP:
    case KUSH_MOVE_LIFT_TO_HOVER:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case KUSH_MOVE_TAKE_OFF_TO_HOVER:
    case KUSH_MOVE_HOVER_TURN:
    case KUSH_MOVE_HOVER_SPIN:
    case KUSH_MOVE_LAND_QUICK:
        break;
    }
    Em_SetStateSync(em, 1, state, mode);
}

/*
 * Enters a group 2 (fly) state: picks the landing or the arrival, the
 * drift's speed and time, the circuit's legs, then Em_SetStateSync.
 */
void EmKushala_SetFlyState(Em *em, u16 state, u8 mode)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case KUSH_FLY_RISE_TO_GOAL_HEIGHT:
        if (em->remote == 0 && em->curGoalKind == 0) {
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_LAND, EM_SYNC_SEND);
            return;
        }
        break;
    case KUSH_FLY_LAND:
        if (em->combat == 1) {
            state = KUSH_FLY_LAND_IN_COMBAT;
        }
        break;
    case KUSH_FLY_DRIFT_TO_GOAL:
    case KUSH_FLY_DRIFT_TO_GOAL_PITCHED:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case KUSH_FLY_DASH_TO_GOAL:
        w->legsLeft = 1;
        if (em->remote == 0) {
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case KUSH_FLY_DASH_CIRCUIT:
        w->legsLeft = 4;
        if (em->remote == 0) {
            em->goalNo = em->stateRand & 3;
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case KUSH_FLY_CROSS_AREA:
        Em_TypeSetup(em);
    case KUSH_FLY_LEAVE_AREA:
        if (em->remote == 0 && em->curGoalKind != 3) {
            EmKushala_ChangeState(em, EM_GROUP_FLY, KUSH_FLY_LAND, EM_SYNC_SEND);
            return;
        }
        break;
    case KUSH_FLY_ENTER_AREA_AND_LAND:
        if (em->remote == 0) {
            em->goalKind = 2;
            if (em->areaSeqActive != 0) {
                em->goalArg = 3;
                em->goalNo = 0;
            } else {
                em->goalArg = 0;
            }
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        Em_TypeSetup(em);
        break;
    case KUSH_FLY_TAKE_OFF:
    case KUSH_FLY_HOVER:
    case KUSH_FLY_HOVER_GUARDED:
    case KUSH_FLY_SPIN_IN_PLACE:
    case KUSH_FLY_TURN_TO_GOAL:
    case KUSH_FLY_DASH_BRAKE:
    case KUSH_FLY_LAND_IN_COMBAT:
    case KUSH_FLY_RISE_TO_AREA_HEIGHT:
    case KUSH_FLY_RISE_TO_FIXED_HEIGHT:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Enters a group 3 (attack) state: the lunge sets its speed to the goal,
 * then Em_SetStateSync.
 */
void EmKushala_SetAttackState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch ((u16) state) {
    case 11:
        Em_SetSpeedScaled(em, 0, 1000.0f, 520.0f);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Enters a group 4 (react) state: the statuses begin, the knock-downs stop
 * the wind, the part break breaks part 2, then Em_SetStateSync.
 */
void EmKushala_SetReactState(Em *em, u16 state, u8 mode)
{
    u8 *w;

    em->motionSpeed = 1.0f;
    w = &em->work;
    switch (state) {
    case KUSH_REACT_STATUS4:
        Em_BeginStatus4(em);
        break;
    case KUSH_REACT_PART_BREAK:
        EmKushala_SetWind(em, (s32) w, 0);
        em->fxFlags |= 1;
        em->partBreakCount = (s8) em->partBreakCount + 1;
        em->meshShown[1] = 0;
        Em_EndAllStatus();
        Em_BreakPart(em, 2U);
        break;
    case KUSH_REACT_STUN:
        Em_BeginStatus5(em);
        break;
    case KUSH_REACT_KNOCK_DOWN:
    case KUSH_REACT_AIR_KNOCK_DOWN:
    case KUSH_REACT_HOVER_KNOCK_DOWN:
        EmKushala_SetWind(em, (s32) w, 0);
        break;
    case KUSH_REACT_HOLD_RISE:
    case KUSH_REACT_HOLD_SETTLE:
        Em_EndAllStatus(em);
        break;
    case KUSH_REACT_FIRST_BREAK_FLINCH:
        em->partBreakCount = (s8) em->partBreakCount + 1;
        break;
    }
    Em_SetStateSync(em, 4, state, mode);
}

/*
 * Changes state, first sending a state the stance doesn't allow to one it
 * does (to the hover idle, the landing or the ground idle); a remote copy
 * takes the state as it is. Groups 0-4 go through their Set function.
 */
void EmKushala_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
        switch (em->stance) {
        case EM_STANCE_FAMILY:
            switch (group) {
            case EM_GROUP_IDLE:
                switch (state) {
                case KUSH_IDLE:
                case KUSH_IDLE_ALERT:
                case KUSH_IDLE_ALERT_B:
                    state = KUSH_IDLE_HOVER;
                    break;
                }
                break;
            case EM_GROUP_MOVE:
                if (state < 20) {
                    state = KUSH_MOVE_TURN;
                    group = 0;
                }
                break;
            case EM_GROUP_FLY:
                if (state != 15 && state != 16 && state != 17) {
                    state = KUSH_FLY_LAND;
                }
                break;
            }
            break;
        case 0:
            switch (group) {
            case EM_GROUP_IDLE:
                switch (state) {
                case KUSH_IDLE_HOVER:
                    state = KUSH_IDLE;
                    break;
                }
                break;
            case EM_GROUP_MOVE:
                if (state >= 21) {
                    state = KUSH_MOVE_LIMP;
                    group = 0;
                }
                break;
            case EM_GROUP_FLY:
                if (state >= 2) {
                    state = KUSH_FLY_LAND;
                }
                break;
            }
            break;
        case 2:
            switch (group) {
            case EM_GROUP_IDLE:
                switch (state) {
                case KUSH_IDLE:
                case KUSH_IDLE_HOVER:
                case KUSH_IDLE_ALERT:
                case KUSH_IDLE_ALERT_B:
                    group = 2;
                    state = KUSH_IDLE;
                    break;
                }
                break;
            case EM_GROUP_MOVE:
                group = 2;
                state = KUSH_MOVE_LIMP;
                break;
            case EM_GROUP_FLY:
                break;
            }
            break;
        }
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case EM_GROUP_IDLE:
        EmKushala_SetIdleState(em, state, mode);
        break;
    case EM_GROUP_MOVE:
        EmKushala_SetMoveState(em, state, mode);
        break;
    case EM_GROUP_FLY:
        EmKushala_SetFlyState(em, state, mode);
        break;
    case EM_GROUP_ATTACK:
        EmKushala_SetAttackState(em, state, mode);
        break;
    case EM_GROUP_REACT:
        EmKushala_SetReactState(em, state, mode);
        break;
    case EM_GROUP_DEFEAT:
        if (state == 3 && QuestEm_CanAppear(em) == 1) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        break;
    }
}

/*
 * Starts the flight curves of a kind: 0 the take-off's rise (vertical speed
 * only, from frame 48), 1 a dash's start, 2 its brake (vertical and forward
 * speed).
 */
void EmKushala_StartCurves(Em *em, u8 kind)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
        w->curveVelY = 1;
        w->curveFrame = 48;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0;
        break;
    }
    Player_ClearVelocity(em);
}

/* The third curve slot: nothing (StartCurves never sets its flag). */
static s32 EmKushala_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmKushala_VelYCurves[];

/* The vertical speed curve of kind curveKind at curveFrame (a table of keys): sets
 * em->velY from the key pair around the frame. Returns 0 inside the curve, 2 at its end
 * (where a key time of 0 lets the monster fall, at most 50 a frame). */
static u8 EmKushala_CurveVelY(Em *em, EmWork *work)
{
    EmKushalaWork *w = (EmKushalaWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmKushala_VelYCurves[((EmKushalaWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            p = &tbl[i];
            t = p->t;
            if (frame > t && t > 0.0f) {
                i++;
            } else if (t == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (t < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (p->v - p[-1].v) / ((t - p[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmKushala_SpeedCurves[];

/* The forward speed curve of kind curveKind at curveFrame: sets em->velZ from the key
 * pair around the frame. Returns 0 inside the curve, 4 at its end. */
static u8 EmKushala_CurveSpeed(Em *em, EmWork *work)
{
    EmKushalaWork *w = (EmKushalaWork *) work;
    EmCurveKey *p;
    f32 frame;
    f32 t;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmKushala_SpeedCurves[((EmKushalaWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            p = &tbl[i];
            t = p->t;
            if (frame > t && t != 0.0f) {
                i++;
            } else if (t == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (p->v - p[-1].v) / ((t - p[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/* Runs the curves EmKushala_StartCurves started (curveVelY, curveSpeed), advances curveFrame
 * by the motion's step, and in the hover or the air moves the monster along rotY by its
 * speeds. Returns the ended curves' bits (2, 4); the callers ignore them. */
u8 EmKushala_UpdateCurves(Em *em)
{
    EmKushalaWork *w = (EmKushalaWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmKushala_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmKushala_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmKushala_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR || em->stance == EM_STANCE_FAMILY) {
        w->moveAngX = 0;
        w->moveAngY = em->rotY;
        w->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngX);
    }
    return ret;
}

/* The file's initialised data, in the game's order */
EmCurveKey EmKushala_VelYCurves_0[6] = {
#include "game/008C9E60.inc"
};
EmCurveKey EmKushala_VelYCurves_1[12] = {
#include "game/008C9E90.inc"
};
EmCurveKey EmKushala_SpeedCurves_1[12] = {
#include "game/008C9EF0.inc"
};
EmCurveKey EmKushala_VelYCurves_2[14] = {
#include "game/008C9F50.inc"
};
EmCurveKey EmKushala_SpeedCurves_2[14] = {
#include "game/008C9FC0.inc"
};
EmCurveKey *EmKushala_VelYCurves[4] = {
    EmKushala_VelYCurves_0, EmKushala_VelYCurves_1, EmKushala_VelYCurves_2, NULL,
};
EmCurveKey *EmKushala_SpeedCurves[4] = {
    NULL, EmKushala_SpeedCurves_1, EmKushala_SpeedCurves_2, NULL,
};
