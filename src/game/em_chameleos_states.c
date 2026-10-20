/*
 * Chameleos's state changes, flight curves and invisibility
 * (EM_KIND_CHAMELEOS). EmChameleos_SetIdleState to
 * EmChameleos_SetAttackState start a state of their group (em_chameleos.h
 * names the states) and EmChameleos_ChangeState picks the group's
 * function; EmChameleos_StartCurves and UpdateCurves move the monster along
 * its height and speed curves; EmChameleos_IsInvisible, UpdateInvisibility,
 * WantsInvisible, CanTurnInvisible and the full cloak's helpers keep its
 * cloak, and EmChameleos_SpawnLyingPuffs adds an effect at
 * intervals. The file ends with the test the family's states make when a
 * hit is pending. The game's code places these functions after the
 * Remobra family's, apart from the rest of the Chameleos family, and their
 * curve tables after the Remobra's data.
 */
#include "common.h"
#include "em.h"
#include "em_chameleos.h"
#include "player.h"
#include "objattach.h"
void EmChameleos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
u8 EmChameleos_UpdateCurves(Em *em);
void Em_BeginStatus4(Em *em);
void Em_BeginStatus5(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_EndAllStatus(Em *em);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmChameleos_ChangeState, EmChameleos_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
s8 QuestEm_Appear(Em *em);
void Em_MoveRotatedAccel(Em *, void *);
void Em_LoadReactionSet(Em *em, u8 k);
void Em_SpawnLyingPuffs(Em *em, s32 bone, f32 *pos, f32 scale);
void Em_TargetPlayerInRange(Em *);
void EmChameleos_SetIdleState(Em *em, u16 state, u8 mode);
void EmChameleos_SetMoveState(Em *em, u16 state, u8 mode);
void EmChameleos_SetFlyState(Em *em, u16 state, u8 mode);
void EmChameleos_SetAttackState(Em *em, u16 state, u8 mode);
void EmChameleos_StartCurves(Em *em, u8 kind);
void EmChameleos_SetFullCloak(Em *em);
void EmChameleos_ClearFullCloak(Em *em);
s32 EmChameleos_IsInvisible(Em *em);
s32 EmChameleos_WantsInvisible(Em *);
s32 EmChameleos_InvisibilityBlocked(Em *em);
void EmChameleos_SpawnLyingPuffs(Em *em);
s32 EmChameleos_TongueHitPlayer(Em *em);
f32 Vec_Distance(f32 *a, f32 *b);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
s32 QuestEm_CanAppear(Em *em);
void Player_ClearVelocity(void *);
s32 Em_PartBroken(Em *em, s32 arg1);
s32 EmChameleos_HasFullCloak(Em *em);
void Em_BeginStatus1(Em *em);
void Em_BeginStatus2(Em *em);
void Em_EndStatus1(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_EndStatus5(Em *em);
void Em_RefillNeed3(Em *em);
void Em_ResetNotice(Em *em);
s32 Game_IsOnline(void);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
void Em_ComputeGoal(Em *em, f32 *target);
void Em_TypeSetup(Em *em);
s32 Em_GetTargetPos(Em *em, u8 idx, f32 *out);
s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 EmChameleos_CanTurnInvisible(Em *em);
void EmChameleos_UpdateInvisibility(Em *em);
/* end of prototypes */

/*
 * Enters an idle state: CHAM_IDLE becomes CHAM_IDLE_ALERT in combat;
 * CHAM_IDLE_CLOAK_FULL sets and CHAM_IDLE_CLOAK_SHIMMER clears the full
 * cloak, REST resets the notice, STATUS2/STATUS1 begin and their _END
 * states end statuses 2/1, REST_END refills need 3, STUN_END and
 * STATUS4_END end statuses 5 and 4; then ground stance and Em_SetStateSync.
 *
 */
void EmChameleos_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case CHAM_IDLE:
        if (em->combat == 1) {
            state = CHAM_IDLE_ALERT;
        }
        break;
    case CHAM_IDLE_CLOAK_FULL:
        EmChameleos_SetFullCloak(em);
        break;
    case CHAM_IDLE_CLOAK_SHIMMER:
        EmChameleos_ClearFullCloak(em);
        break;
    case CHAM_IDLE_REST:
        Em_ResetNotice(em);
        break;
    case CHAM_IDLE_STATUS2_DOWN:
        Em_BeginStatus2(em);
        break;
    case CHAM_IDLE_REST_END:
        Em_RefillNeed3(em);
        break;
    case CHAM_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case CHAM_IDLE_STATUS1_DOWN:
        Em_BeginStatus1(em);
        break;
    case CHAM_IDLE_STATUS1_END:
        Em_EndStatus1(em);
        break;
    case CHAM_IDLE_STUN_SHAKE_OFF:
        Em_EndStatus5(em);
        break;
    case CHAM_IDLE_STATUS4_SHAKE_OFF:
        Em_EndStatus4(em);
        break;
    }
    em->stance = EM_STANCE_GROUND;
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/*
 * Enters a move state: CHAM_MOVE_WALK becomes CHAM_MOVE_LIMP below 15% hp;
 * LIMP sets its time from the goal distance (or a random time without a
 * goal, at most 300); RUN sets its speed; RUN_NO_ATTACK clears the goal;
 * then Em_SetStateSync.
 */
void EmChameleos_SetMoveState(Em *em, u16 state, u8 mode)
{
    f32 dist;
    f32 rate;

    em->stance = EM_STANCE_GROUND;
    em->motionSpeed = 1.0f;
    switch (state) {
    case CHAM_MOVE_WALK:
        rate = 0.15f;
        if (em->hp < (s16) (em->hpMax * rate)) {
            state = CHAM_MOVE_LIMP;
            /* the limp's set-up below, shared by a jump as the game's code does */
            goto limp;
        }
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case CHAM_MOVE_LIMP:
    limp:
        if (em->curGoalKind == 0) {
            em->hasGoal = 0;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            em->hasGoal = 1;
            dist = Vec_DistanceXZ(&em->posX, em->goal);
            em->goalDist = dist;
            em->timer = (s32) dist / 7;
        }
        if (em->timer > 300) {
            em->timer = 300;
        }
        break;
    case CHAM_MOVE_RUN:
        Em_SetSpeedScaled(em, 2, 1000.0f, 500.0f);
        break;
    case CHAM_MOVE_RUN_NO_HIT:
        em->hasGoal = 0;
        em->goalDist = 1000.0f;
        break;
    case CHAM_MOVE_TURN:
    case CHAM_MOVE_NONE:
    case CHAM_MOVE_SPIN_TURN:
    case CHAM_MOVE_BACK_HOP:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Enters a flight state: CHAM_FLY_LAND becomes CHAM_FLY_BRAKE_AND_LAND when
 * flying fast; ROUTE and ROUTE_ENDLESS set 4 legs and a random goal point;
 * HOVER_TO_GOAL and STRAIGHT_TO_GOAL set speed 20 and the time to the goal;
 * OUT_OF_AREA and THROUGH_AREA land instead without an area goal; INTO_AREA
 * and GLIDE_TO_GOAL_AND_LAND aim at the area goal; then Em_SetStateSync.
 *
 */
void EmChameleos_SetFlyState(Em *em, u16 state, u8 mode)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    em->motionSpeed = 1.0f;
    switch (state) {
    case 1:
        if (em->remote == 0 && !(em->velZ < 50.0f)) {
            state = 18;
        }
        break;
    case 6:
    case 17:
        w->routeLegs = 4;
        if (em->remote == 0) {
            em->goalNo = em->stateRand & 3;
            em->goalKind = 2;
            em->goalArg = 2;
            Em_ComputeGoal(em, em->goal);
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case 7:
    case 21:
        Player_ClearVelocity(em);
        em->velZ = 20.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case 13:
        Em_TypeSetup(em);
    case 8:
        if (em->remote == 0 && em->curGoalKind != 3) {
            EmChameleos_ChangeState(em, EM_GROUP_FLY, CHAM_FLY_LAND, EM_SYNC_SEND);
            return;
        }
        break;
    case 9:
        if (em->remote == 0) {
            em->goalKind = 2;
            em->goalArg = 3;
            em->goalNo = 0;
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        Em_TypeSetup(em);
        break;
    case 11:
        if (em->remote == 0) {
            if (em->areaSeqActive != 0) {
                em->goalKind = 2;
                em->goalArg = 3;
                em->goalNo = 0;
            }
            Em_ComputeGoal(em, em->goal);
        }
        em->timer = 500;
        break;
    case 19:
        w->routeLegs = 1;
        if (em->remote == 0) {
            em->timer = 1.5f * Vec_DistanceXZ(&em->posX, em->goal) / 50.0f;
        }
        break;
    case 24:
        if (em->velZ <= 0.0f) {
            em->velZ = 20.0f;
        }
        if (em->remote == 0) {
            em->timer = Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ;
        }
        break;
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
    case 10:
    case 12:
    case 14:
    case 15:
    case 16:
    case 18:
    case 20:
    case 22:
    case 23:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Enters an attack state: CHAM_ATK_TONGUE_HIGH becomes CHAM_ATK_TONGUE_LOW
 * when the target player is no more than 50 above the monster (decided on
 * the host, when Game_IsOnline is 0 or a sync mode is given); then
 * Em_SetStateSync.
 */
void EmChameleos_SetAttackState(Em *em, u16 state, u8 mode)
{
    f32 pos[3];

    em->motionSpeed = 1.0f;
    switch (state) {
    case CHAM_ATK_TONGUE_HIGH:
        if (em->remote == 0 && (Game_IsOnline() == 0 || mode != 0) && em->curGoalKind == 1 && em->curGoalArg == 0 &&
            em->targetNo != -1) {
            Em_GetTargetPos(em, em->targetNo, pos);
            if (pos[1] <= em->posY + 50.0f) {
                state = CHAM_ATK_TONGUE_LOW;
            } else {
                state = CHAM_ATK_TONGUE_HIGH;
            }
        }
        break;
    case CHAM_ATK_TONGUE:
    case CHAM_ATK_TONGUE_LOW:
    case CHAM_ATK_STOMP:
    case CHAM_ATK_TAIL_SLAM:
    case CHAM_ATK_TONGUE_B:
    case CHAM_ATK_BALL_PITCHED:
    case CHAM_ATK_BALL_LEVEL:
    case CHAM_ATK_BACK_HOP_BALL:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Moves to (group, state): a remote copy ignores EM_SYNC_OWNER and
 * EM_SYNC_OWNER_FLAGGED; loads
 * reaction set 0 and calls the group's Set*State; for defeat,
 * CHAM_DEFEAT_REAPPEAR makes the quest monster appear; for react,
 * PART_BREAK counts the break, sets fxFlags bit 0, ends all statuses and
 * breaks part 1, STUN and STATUS4 begin statuses 5 and 4, the DECLOAK
 * states clear the full cloak; the reaction, defeat and event groups then
 * go to Em_SetStateSync.
 */
void EmChameleos_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    Em_LoadReactionSet(em, 0);
    switch (group) {
    case EM_GROUP_IDLE:
        EmChameleos_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmChameleos_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmChameleos_SetFlyState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmChameleos_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_DEFEAT:
        if ((state == 3) && (QuestEm_CanAppear(em) == 1)) {
            QuestEm_Appear(em);
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_REACT:
        switch (state) {
        case CHAM_REACT_PART_BREAK:
            em->partBreakCount = (s8) em->partBreakCount + 1;
            em->fxFlags |= 1;
            Em_EndAllStatus(em);
            Em_BreakPart(em, 1U);
            break;
        case CHAM_REACT_STUN:
            Em_BeginStatus5(em);
            break;
        case CHAM_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        case CHAM_REACT_DECLOAK:
        case CHAM_REACT_DECLOAK_FALL:
            EmChameleos_ClearFullCloak(em);
            break;
        }

    case EM_GROUP_EVENT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);

    default:
        return;
    }
}

/*
 * Starts curve set kind: clears the curve flags, enables the height curve
 * (all kinds) and the speed curve (kinds 1, 2) with their start frames, and
 * stops the monster.
 */
void EmChameleos_StartCurves(Em *em, u8 kind)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    u8 k;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    k = w->curveKind;
    switch (k) {
    case 0:
        w->curveVelY = 1;
        w->curveSpeed = 0;
        w->curveFrame = 0x3A;
        break;
    case 1:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x66;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x4E;
        break;
    }
    Player_ClearVelocity(em);
}

/* The unused first curve: returns 0. */
static s32 EmChameleos_CurveNone(Em *em, EmWork *work)
{
    return 0;
}


extern EmCurveKey *EmChameleos_VelYCurves[];

/* The value of curve work[0x47] at the frame in the work area (a table of keys): sets
 * em->velY from the key pair around the frame, 2 at the table's end. */
static u8 EmChameleos_CurveVelY(Em *em, EmWork *work)
{
    EmChameleosWork *w = (EmChameleosWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmChameleos_VelYCurves[((EmChameleosWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyT = key->t;
            if (frame > keyT && keyT > 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyT < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmChameleos_SpeedCurves[];

/* The value of curve work[0x47] at the frame in the work area: sets em->velZ from the key
 * pair around the frame, 4 at the table's end. */
static u8 EmChameleos_CurveSpeed(Em *em, EmWork *work)
{
    EmChameleosWork *w = (EmChameleosWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyT;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmChameleos_SpeedCurves[((EmChameleosWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyT = key->t;
            if (frame > keyT && keyT != 0.0f) {
                i++;
            } else if (keyT == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyT - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/* Runs the family's three curves when their flags in the work area are set, advances
 * the curve frame by the motion's step, and moves along the yaw while in the air (EM_STANCE_AIR). */
u8 EmChameleos_UpdateCurves(Em *em)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;
    u8 ret = 0;

    if ((u8)w->curveNone != 0) {
        ret = EmChameleos_CurveNone(em, (EmWork *) w);
    }
    if ((u8)w->curveVelY != 0) {
        ret |= EmChameleos_CurveVelY(em, (EmWork *) w);
    }
    if ((u8)w->curveSpeed != 0) {
        ret |= EmChameleos_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->moveAngCX = 0;
        w->moveAngCY = em->rotY;
        w->moveAngX = 0;
        Em_MoveRotatedAccel(em, &w->moveAngCX);
    }
    return ret;
}

/*
 * Sets Em.fullCloak (WantsInvisible then gives the full cloak, mode 2).
 *
 */
void EmChameleos_SetFullCloak(Em *em)
{
    em->fullCloak = 1;
}

/*
 * Clears Em.fullCloak (WantsInvisible then gives the shimmering cloak, mode
 * 1, except in CHAM_EVENT_FOREST where it is visible).
 */
void EmChameleos_ClearFullCloak(Em *em)
{
    em->fullCloak = 0;
}

/*
 * Returns 1 while the cloak shimmer runs (invisFade not 0xFFFF) or
 * WantsInvisible gives a nonzero mode, else 0; the drawing hides all but
 * the cloak meshes and the shadow then.
 */
s32 EmChameleos_IsInvisible(Em *em)
{
    if (em->invisFade != 0xFFFF || (EmChameleos_WantsInvisible(em) & 0xFF) != 0) {
        return 1;
    }
    return 0;
}

/* Returns 1 when Em.fullCloak is set. */
s32 EmChameleos_HasFullCloak(Em *em)
{
    return em->fullCloak != 0;
}

/*
 * Counts the 900-frame shimmer period down and runs the 32-frame cloak
 * shimmer (invisFade): while invisible, mode 1 restarts it continually,
 * mode 2 starts it on turning invisible and once a period,
 * CHAM_EVENT_FOREST starts it; while visible, a just-ended mode 2 starts it
 * unless blocked; keeps the last mode in wantInvisible.
 */
void EmChameleos_UpdateInvisibility(Em *em)
{
    EmChameleosWork *w = (EmChameleosWork *)&em->work;

    s16 periodLeft;
    s32 mode;
    u16 fade;
    u16 nextFade;

    periodLeft = w->shimmerTimer - 1;
    w->shimmerTimer = periodLeft;
    if (periodLeft <= 0) {
        w->shimmerTimer = 0x384;
    }
    if (EmChameleos_IsInvisible(em) == 1) {
        fade = w->invisFade;
        if (fade == 0xFFFF) {
            if ((em->stateGroup == EM_GROUP_EVENT) && (em->state == 0)) {
                w->invisFade = 1;
            } else {
                mode = EmChameleos_WantsInvisible(em) & 0xFF;
                switch (mode) {
                case 0:
                    w->invisFade = 0xFFFF;
                    break;
                case 2:
                    if (w->wantInvisible == 0) {
                        w->invisFade = 1;
                    }
                    if (w->shimmerTimer == 0x384) {
                        w->invisFade = 1;
                    }
                    break;
                case 1:
                    w->invisFade = 1;
                    break;
                }
            }
        } else {
            w->invisFade = fade + 1;
            if ((s32) w->invisFade >= 0x20) {
                w->invisFade = 0xFFFF;
            }
        }
        w->wantInvisible = EmChameleos_WantsInvisible(em);
        return;
    }
    if (w->wantInvisible == 2) {
        if (EmChameleos_InvisibilityBlocked(em) == 1) {
            w->invisFade = 0xFFFF;
        } else if (w->invisFade == 0xFFFF) {
            w->invisFade = 0;
        }
    }
    nextFade = w->invisFade;
    if (nextFade != 0xFFFF) {
        w->invisFade = nextFade + 1;
        if ((s32) w->invisFade >= 0x20) {
            w->invisFade = 0xFFFF;
        }
    }
    w->wantInvisible = 0;
}

/*
 * Returns the invisibility mode: 0 (visible) while blocked, in
 * CHAM_MOVE_RUN, lying or under a status, after the horn break
 * (CanTurnInvisible 0) and in the attack motions; 1 in the ball and breath
 * ends and CHAM_MOT_TAIL_POUNDS, or without the full cloak; else
 * CanTurnInvisible's 1 or 2.
 */
s32 EmChameleos_WantsInvisible(Em *em)
{
    s32 canMode;
    if (EmChameleos_InvisibilityBlocked(em) == 1 || Player_IsState(em, 1, 4) || em->hitReactMode == EM_HIT_REACT_WAKE || em->hitReactMode == EM_HIT_REACT_STATUS) {
        return 0;
    }
    canMode = (u8) EmChameleos_CanTurnInvisible(em);
    if (canMode == 0) {
        return canMode;
    }
    switch (em->motionId[0]) {
    /* the attacks show the monster; the tail pounds and the spits' ends only shimmer */
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_LOW):
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_START):
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_A):
    case EM_MOTION_ID(0, CHAM_MOT_STOMP):
    case EM_MOTION_ID(0, CHAM_MOT_TAIL_SLAM):
    case EM_MOTION_ID(0, CHAM_MOT_BACK_HOP_BALL):
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE):
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_B):
    case EM_MOTION_ID(0, CHAM_MOT_TONGUE_HIGH):
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_B):
        return 0;
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_A_END):
    case EM_MOTION_ID(0, CHAM_MOT_TAIL_POUNDS):
    case EM_MOTION_ID(0, CHAM_MOT_SPIT_B_END):
        return 1;
    }
    if (EmChameleos_HasFullCloak(em) == 0) {
        if (em->stateGroup == EM_GROUP_EVENT && em->state == CHAM_EVENT_FOREST) {
            return 0;
        }
        return 1;
    }
    return canMode;
}

/*
 * Returns 2 before the part-break reaction (fxFlags bit 0 clear), 1 after
 * it while part 0 is not broken, 0 once part 0 is broken.
 */
s32 EmChameleos_CanTurnInvisible(Em *em)
{
    if (em->fxFlags & 1) {
        if (Em_PartBroken(em, 0) != 1) {
            return 1;
        }
        return 0;
    }
    return 2;
}

/*
 * Returns 1 in the defeat states and in CHAM_EVENT_LEAVE and
 * CHAM_EVENT_LEAVE_AIR, else 0.
 */
s32 EmChameleos_InvisibilityBlocked(Em *em)
{
    if (em->stateGroup == EM_GROUP_DEFEAT || Player_IsState(em, 6, 1) || Player_IsState(em, 6, 2)) {
        return 1;
    }
    return 0;
}

/*
 * Makes the lying puffs (Em_SpawnLyingPuffs) at bone 0x25 with a fixed
 * offset, scale 2; called every frame by the resting and status-lying
 * states.
 */
void EmChameleos_SpawnLyingPuffs(Em *em)
{
    f32 ofs[3];

    ofs[0] = 0.0f;
    ofs[1] = 80.0f;
    ofs[2] = 290.0f;
    Em_SpawnLyingPuffs(em, 0x25, ofs, 2.0f);
}

/*
 * Returns 1 when a hit is pending, the contact is an active player in
 * gPlayers and the contact attack (contactAttach) is a ChameleosAttack
 * (kind ATTACH_CHAMELEOS_ATTACK) of type 1 or 2, the tongue hits; the
 * tongue attacks then steal an item (EmChameleos_StealItem).
 */
s32 EmChameleos_TongueHitPlayer(Em *em)
{
    u16 playerIndex;
    u8 attackType;
    ObjAttach *attack;
    Player *contact;

    if (em->hitPending != 0) {
        contact = em->contact;
        if (contact != NULL) {
            playerIndex = contact->index;
            if (((s32) playerIndex < 4) && (contact == &gPlayers[playerIndex]) && (contact->active != 0)) {
                attack = em->contactAttach;
                if ((attack->kind == ATTACH_CHAMELEOS_ATTACK) && ((attackType = attack->type, (attackType == 1)) || (attackType == 2))) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* The file's initialised data, in the game's order */
EmCurveKey EmChameleos_VelYCurves_0[6] = {
#include "game/008CB330.inc"
};
EmCurveKey EmChameleos_VelYCurves_1[10] = {
#include "game/008CB360.inc"
};
EmCurveKey EmChameleos_VelYCurves_2[5] = {
#include "game/008CB3B0.inc"
};
EmCurveKey *EmChameleos_VelYCurves[3] = {
    EmChameleos_VelYCurves_0, EmChameleos_VelYCurves_1, EmChameleos_VelYCurves_2,
};
EmCurveKey EmChameleos_SpeedCurves_1[10] = {
#include "game/008CB3F0.inc"
};
EmCurveKey EmChameleos_SpeedCurves_2[5] = {
#include "game/008CB440.inc"
};
EmCurveKey *EmChameleos_SpeedCurves[3] = {
    NULL, EmChameleos_SpeedCurves_1, EmChameleos_SpeedCurves_2,
};
