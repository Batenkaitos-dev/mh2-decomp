/*
 * Fatalis's state changes and flight curves (EM_KIND_FATALIS,
 * EM_KIND_FATALIS_B and EM_KIND_FATALIS_C; include/em_fatalis.h names the
 * states): EmFatalis_SetIdleState, SetMoveState, SetFlyState and
 * SetAttackState start a state of their group, and EmFatalis_ChangeState
 * picks the group's function; EmFatalis_MoveByVelYCurve, StartCurves and
 * UpdateCurves move the monster along its height and speed curves. The
 * game's code places these functions after the Bulldrome family's, apart
 * from the rest of the Fatalis family, and their curve tables after the
 * Bulldrome's data.
 */
#include "common.h"
#include "em.h"
#include "em_fatalis.h"
/* The game built this file at optimization level 4. */
#pragma optimization_level 4
void Em_BeginStatus2(Em *em);
void Em_BeginStatus4(Em *em);
void Em_EndAllStatus(Em *em);
void Em_EndStatus2(Em *em);
void Em_EndStatus4(Em *em);
void Em_MoveRotatedAccel(Em *em, void *ang);
/* unit's own declaration: declared here as the callers use it; with the definition's void (Em *em, s32 group, s32 state, u8 mode), the calls in EmBulldrome_ChangeState, EmBulldrome_SetIdleState and others compile differently */
void Em_SetStateSync(Em *em, u16 group, u16 state, u8 mode);
void Em_TargetPlayerInRange(Em *);
void EmFatalis_SetIdleState(Em *em, u16 state, u8 mode);
void EmFatalis_SetMoveState(Em *em, u16 state, u8 mode);
void EmFatalis_SetFlyState(Em *em, u16 state, u8 mode);
void EmFatalis_SetAttackState(Em *em, u16 state, u8 mode);
void Player_ClearVelocity(void *);
void Em_SetSpeedScaled(Em *em, u16 flags, f32 x, f32 y);
f32 Vec_DistanceXZ(f32 *a, f32 *b);
f32 Vec_Distance(f32 *a, f32 *b);
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void Em_MoveRotated(Em *em, void *src);
extern EmCurveKey *EmFatalis_MoveVelYCurves[];
/* end of prototypes */

/*
 * Starts a state of group 0: FAT_IDLE and FAT_IDLE_ACTION pick one of the
 * two at random (Em.stateRand); the status states begin or end their
 * status.
 */
void EmFatalis_SetIdleState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case FAT_IDLE:
    case FAT_IDLE_ACTION:
        if (em->stateRand & 1) {
            state = FAT_IDLE;
        } else {
            state = FAT_IDLE_ACTION;
        }
        break;
    case FAT_IDLE_STATUS2:
        Em_BeginStatus2(em);
        break;
    case FAT_IDLE_STATUS2_END:
        Em_EndStatus2(em);
        break;
    case FAT_IDLE_STATUS4_END:
        Em_EndStatus4(em);
        break;
    }
    Em_SetStateSync(em, EM_GROUP_IDLE, state, mode);
}

/* Starts a state of group 1; the walk sets its speed. */
void EmFatalis_SetMoveState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch ((u16) state) {
    case 0:
        Em_SetSpeedScaled(em, 0, 1000.0f, 500.0f);
        break;
    case 1:
    case 2:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_MOVE, state, mode);
}

/*
 * Starts a state of group 2. A flight asked for on the ground takes off
 * first (FAT_FLY_TAKE_OFF); the flights set their speed and their timer
 * (300 frames, or from the goal's distance).
 */
void EmFatalis_SetFlyState(Em *em, u16 state, u8 mode)
{
    em->motionSpeed = 1.0f;
    switch (state) {
    case FAT_FLY_HOVER:
    case FAT_FLY_HOVER_TURN:
    case FAT_FLY_HOVER_TURN_STOP:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        em->timer = 300;
        break;
    case FAT_FLY_TO_GOAL:
    case FAT_FLY_LEVEL_TO_GOAL:
    case FAT_FLY_CLIMB_TO_GOAL:
    case FAT_FLY_CLIMB_TO_GOAL_NEAR:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = Vec_DistanceXZ(&em->posX, em->goal) / em->velZ;
        if (em->timer < 30) {
            em->timer = 30;
        }
        break;
    case FAT_FLY_RISE:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case FAT_FLY_STRAIGHT_TO_GOAL:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = (s32) (Vec_Distance((f32 *) &em->posX, (f32 *) em->goal) / em->velZ) * 3;
        break;
    case FAT_FLY_DESCEND:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = 300;
        break;
    case FAT_FLY_RISE_HIGH:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = 300;
        break;
    case FAT_FLY_TAKE_OFF_AND_RISE:
        Player_ClearVelocity(em);
        em->velZ = 10.0f;
        em->timer = 300;
        break;
    case FAT_FLY_TAKE_OFF:
    case FAT_FLY_LAND:
    case FAT_FLY_LAND_AT_HEIGHT:
    case FAT_FLY_TAKE_OFF_HIGH:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_FLY, state, mode);
}

/*
 * Starts a state of group 3. A ground attack asked for in the air lands
 * first (FAT_FLY_LAND) and an air attack on the ground takes off first; the
 * charge and the glide set their speed, the air fireballs their count, the
 * dives their distance (100, or the goal's) and timer.
 */
void EmFatalis_SetAttackState(Em *em, u16 state, u8 mode)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    f32 dist;

    em->motionSpeed = 1.0f;
    switch (state) {
    case FAT_ATK_SINGLE_A:
    case FAT_ATK_SINGLE_B:
    case FAT_ATK_FIREBALL:
    case FAT_ATK_FIREBALL_B:
    case FAT_ATK_SWEEP_NO_EFFECT:
    case FAT_ATK_HOP_FIREBALL:
    case FAT_ATK_CHARGE_START:
    case FAT_ATK_SWEEP_A:
    case FAT_ATK_SWEEP_B:
    case FAT_ATK_SWEEP_C:
    case FAT_ATK_SWEEP_D:
    case FAT_ATK_SWEEP_DOUBLE:
    case FAT_ATK_SINGLE_C:
        if (em->remote == 0 && em->stance == EM_STANCE_AIR) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_LAND, EM_SYNC_SEND);
            return;
        }
        break;
    case FAT_ATK_CHARGE:
    case FAT_ATK_GLIDE:
        if (em->remote == 0 && em->stance == EM_STANCE_AIR) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_LAND, EM_SYNC_SEND);
            return;
        }
        Em_SetSpeedScaled(em, 6, 1000.0f, 500.0f);
        break;
    case FAT_ATK_AIR_FIREBALL:
    case FAT_ATK_AIR_FIREBALL_B:
    case FAT_ATK_AIR_FIREBALL_C:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        w->countdown = 1;
        break;
    case FAT_ATK_AIR_FIREBALL_TRIPLE:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        w->countdown = 3;
        break;
    case FAT_ATK_DIVE:
    case FAT_ATK_DIVE_B:
        if (em->remote == 0 && em->stance == EM_STANCE_GROUND) {
            EmFatalis_ChangeState(em, EM_GROUP_FLY, FAT_FLY_TAKE_OFF, EM_SYNC_SEND);
            return;
        }
        if (em->curGoalKind == 0) {
            w->diveToGoal = 0;
            w->runDist = 100.0f;
            em->timer = (em->stateRand % 120 + 180) / 2;
        } else {
            w->diveToGoal = 1;
            dist = Vec_DistanceXZ(&em->posX, em->goal);
            w->runDist = dist;
            em->timer = (s32) dist / 7;
        }
        break;
    case FAT_ATK_BLASTS:
        break;
    }
    Em_SetStateSync(em, EM_GROUP_ATTACK, state, mode);
}

/*
 * Changes the state; the sync mode says how the change reaches the other
 * players online, and a remote copy drops the owner's modes (EM_SYNC_OWNER,
 * EM_SYNC_OWNER_FLAGGED). Groups 0-3 go through their Set function;
 * FAT_REACT_KNOCK_DOWN ends every status (Em_EndAllStatus) and
 * FAT_REACT_STATUS4 begins status 4.
 */
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode)
{
    if (em->remote == 0) {
        Em_TargetPlayerInRange(em);
    } else if (mode == EM_SYNC_OWNER || mode == EM_SYNC_OWNER_FLAGGED) {
        return;
    }
    switch (group) {
    case EM_GROUP_IDLE:
        EmFatalis_SetIdleState(em, state, mode);
        return;
    case EM_GROUP_MOVE:
        EmFatalis_SetMoveState(em, state, mode);
        return;
    case EM_GROUP_FLY:
        EmFatalis_SetFlyState(em, state, mode);
        return;
    case EM_GROUP_ATTACK:
        EmFatalis_SetAttackState(em, state, mode);
        return;
    case EM_GROUP_EVENT:
    case EM_GROUP_DEFEAT:
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        return;
    case EM_GROUP_REACT:
        switch (state) {
        case FAT_REACT_KNOCK_DOWN:
            Em_EndAllStatus(em);
            break;
        case FAT_REACT_STATUS4:
            Em_BeginStatus4(em);
            break;
        }
        em->motionSpeed = 1.0f;
        Em_SetStateSync(em, group, state, mode);
        
    default:
        return;
    }
}

/*
 * Sets velY from the curve EmFatalis_MoveVelYCurves[kind] at the motion
 * frame (unblended motions only) and moves along rotY.
 */
void EmFatalis_MoveByVelYCurve(Em *em, s32 kind)
{
    EmWork *work = (EmWork *) &em->work;
    EmCurveKey *tbl;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;

    tbl = EmFatalis_MoveVelYCurves[kind];
    if (tbl != NULL && em->motion[0].blend == 0) {
        frame = em->motion[0].frame;
        if (frame == 0.0f || frame == 1.0f) {
            em->velY = tbl->v;
        } else {
            i = 1;
            while (i != 0) {
                key = &tbl[i];
                keyFrame = key->t;
                if (frame > keyFrame && keyFrame != 0.0f) {
                    i++;
                } else if (keyFrame == 0.0f) {
                    break;
                } else {
                    i = 0;
                    em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
                }
            }
        }
        work->moveAngX = 0;
        work->moveAngY = em->rotY;
        work->moveAngZ = 0;
        Em_MoveRotated(em, &work->moveAngX);
    }
}

/*
 * Starts the curves of a flight (velY, speed or both, by kind) at the frame
 * the kind gives (134, 70, 60 or 0; the take-off's curves start at frame
 * 134, where FAT_MOT_TAKE_OFF leaves the floor). Stops the monster.
 */
void EmFatalis_StartCurves(Em *em, u8 kind)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;

    w->curveNone = 0;
    w->curveVelY = 0;
    w->curveSpeed = 0;
    w->curveKind = kind;
    switch (w->curveKind) {
    case 0:
    case 6:
        w->curveVelY = 1;
        w->curveFrame = 0x86;
        break;
    case 2:
        w->curveVelY = 1;
        w->curveFrame = 0;
        break;
    case 3:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x46;
        break;
    case 4:
        w->curveSpeed = 1;
        w->curveFrame = 0x46;
        break;
    case 5:
        w->curveVelY = 1;
        w->curveSpeed = 1;
        w->curveFrame = 0x3C;
        break;
    }
    Player_ClearVelocity(em);
}

/* The curve that does nothing. */
static s32 EmFatalis_CurveNone(Em *em, EmWork *work)
{
    return 0;
}

extern EmCurveKey *EmFatalis_VelYCurves[];

/*
 * The velY curve of EmFatalisWork.curveKind (EmFatalis_VelYCurves): sets
 * velY from the key pair around the curve frame. Returns 2 at the curve's
 * end (a key at t 0: velY then falls 1 a frame down to -50; t below 0: velY
 * 0 and the frame back to 0), else 0.
 */
static u8 EmFatalis_CurveVelY(Em *em, EmWork *work)
{
    EmFatalisWork *w = (EmFatalisWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmFatalis_VelYCurves[((EmFatalisWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame > 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 2;
                if (em->velY > -50.0f) {
                    em->velY -= 1.0f;
                }
                break;
            } else if (keyFrame < 0.0f) {
                w->curveFrame = 0;
                ret = 2;
                em->velY = 0.0f;
                break;
            } else {
                i = 0;
                em->velY = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

extern EmCurveKey *EmFatalis_SpeedCurves[];

/*
 * The speed curve of EmFatalisWork.curveKind (EmFatalis_SpeedCurves): sets
 * velZ from the key pair around the curve frame. Returns 4 at the curve's
 * end, else 0.
 */
static u8 EmFatalis_CurveSpeed(Em *em, EmWork *work)
{
    EmFatalisWork *w = (EmFatalisWork *) work;
    EmCurveKey *key;
    f32 frame;
    f32 keyFrame;
    s32 i;
    EmCurveKey *tbl;
    u8 ret;

    tbl = EmFatalis_SpeedCurves[((EmFatalisWork *)work)->curveKind];
    ret = 0;
    if (tbl == NULL) {
        return ret;
    }
    frame = w->curveFrame;
    if (frame != 0.0f && frame != 1.0f) {
        i = 1;
        while (i != 0) {
            key = &tbl[i];
            keyFrame = key->t;
            if (frame > keyFrame && keyFrame != 0.0f) {
                i++;
            } else if (keyFrame == 0.0f) {
                ret = 4;
                break;
            } else {
                i = 0;
                em->velZ = (key->v - key[-1].v) / ((keyFrame - key[-1].t) / em->motion[0].step);
            }
        }
    }
    return ret;
}

/*
 * Runs the curves EmFatalis_StartCurves turned on, advances the curve frame
 * by the motion's step, and moves the monster along rotY while airborne.
 * Returns the ended curves' bits (2 velY, 4 speed), 0 while they run.
 */
u8 EmFatalis_UpdateCurves(Em *em)
{
    EmFatalisWork *w = (EmFatalisWork *)&em->work;
    u8 ret = 0;

    if (w->curveNone != 0) {
        ret = EmFatalis_CurveNone(em, (EmWork *) w);
    }
    if (w->curveVelY != 0) {
        ret |= EmFatalis_CurveVelY(em, (EmWork *) w);
    }
    if (w->curveSpeed != 0) {
        ret |= EmFatalis_CurveSpeed(em, (EmWork *) w);
    }
    w->curveFrame += (s16) em->motion[0].step;
    if (em->stance == EM_STANCE_AIR) {
        w->moveAngX = 0;
        w->moveAngY = em->rotY;
        w->moveAngZ = 0;
        Em_MoveRotatedAccel(em, &w->moveAngX);
    }
    return ret;
}

/* The file's initialised data, in the game's order */
EmCurveKey EmFatalis_MoveVelYCurves_1[17] = {
#include "game/008CC5E0.inc"
};
EmCurveKey *EmFatalis_MoveVelYCurves[2] = {
    NULL, EmFatalis_MoveVelYCurves_1,
};
EmCurveKey EmFatalis_VelYCurves_0[38] = {
#include "game/008CC670.inc"
};
EmCurveKey EmFatalis_VelYCurves_2[26] = {
#include "game/008CC7A0.inc"
};
EmCurveKey EmFatalis_VelYCurves_3[30] = {
#include "game/008CC870.inc"
};
EmCurveKey EmFatalis_VelYCurves_5[10] = {
#include "game/008CC960.inc"
};
EmCurveKey EmFatalis_VelYCurves_6[42] = {
#include "game/008CC9B0.inc"
};
EmCurveKey *EmFatalis_VelYCurves[8] = {
    EmFatalis_VelYCurves_0, NULL, EmFatalis_VelYCurves_2, EmFatalis_VelYCurves_3,
    NULL, EmFatalis_VelYCurves_5, EmFatalis_VelYCurves_6, NULL,
};
EmCurveKey EmFatalis_SpeedCurves_3[14] = {
#include "game/008CCB20.inc"
};
EmCurveKey EmFatalis_SpeedCurves_4[10] = {
#include "game/008CCB90.inc"
};
EmCurveKey EmFatalis_SpeedCurves_5[10] = {
#include "game/008CCBE0.inc"
};
EmCurveKey *EmFatalis_SpeedCurves[8] = {
    NULL, NULL, NULL, EmFatalis_SpeedCurves_3,
    EmFatalis_SpeedCurves_4, EmFatalis_SpeedCurves_5, NULL, NULL,
};
