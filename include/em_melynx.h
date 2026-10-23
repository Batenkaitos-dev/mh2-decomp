#ifndef EM_MELYNX_H
#define EM_MELYNX_H
#include "em.h"

/*
 * The Melynx family (EM_KIND_MELYNX; the per-kind hooks also send kind 9
 * here): its motions and its states, by group (em.h's EmStateGroup). The
 * functions are EmMelynx_*. A Melynx wanders, pounces on a hunter to steal an
 * item (EmMelynx_StealItem) and burrows away with it, or takes out a bomb and
 * throws it. Its group 2, the flying families' group, holds only its fall.
 * Each enum lists the cases of the group's dispatcher (EmMelynx_Run*State);
 * where one state function serves several numbers, the names say which
 * variant the number asks for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmMelynxMotion {
    MELYNX_MOT_IDLE = 0x01,                 /* the standing loop: MELYNX_IDLE_WAIT, IDLE_STAND, EVENT_APPEAR (Reset plays its row) */
    MELYNX_MOT_IDLE_ACTION_A = 0x02,        /* MELYNX_IDLE_ACTION_A */
    MELYNX_MOT_IDLE_ACTION_B = 0x03,        /* MELYNX_IDLE_ACTION_B */
    MELYNX_MOT_IDLE_ACTION_C = 0x04,        /* MELYNX_IDLE_ACTION_C */
    MELYNX_MOT_IDLE_ACTION_D = 0x05,        /* MELYNX_IDLE_ACTION_D, held 120 frames */
    MELYNX_MOT_WALK = 0x0B,                 /* MELYNX_MOVE_WALK, WALK_LONG, WANDER, WALK_HOME, and the turn in place (MOVE_TURN) */
    MELYNX_MOT_RUN_A = 0x0C,                /* MELYNX_MOVE_CHASE, MOVE_RUN_A */
    MELYNX_MOT_FALL = 0x0D,                 /* MELYNX_FLY_FALL */
    MELYNX_MOT_POSTURE_MOVE = 0x0E,         /* MELYNX_MOVE_POSTURE: moving in the posture */
    MELYNX_MOT_RUN_B = 0x0F,                /* MELYNX_MOVE_RUN_B, MOVE_WANDER_RUN, the steal runs (ATK_STEAL, STEAL_CHASE) */
    MELYNX_MOT_DASH = 0x10,                 /* MELYNX_MOVE_CHARGE (with contact attack 4), MOVE_DASH */
    MELYNX_MOT_BURROW = 0x11,               /* MELYNX_IDLE_BURROW_AWAY and the hunted states: burrows (dust clouds and part effects at bone 2 in EmMelynx_MotionEffects) */
    MELYNX_MOT_POUNCE_START = 0x15,         /* MELYNX_ATK_POUNCE: the crouch before the leap */
    MELYNX_MOT_LEAP = 0x16,                 /* MELYNX_ATK_POUNCE, ATK_LEAP: in the air with contact attack 1 */
    MELYNX_MOT_SNATCH = 0x17,               /* after an attack connects: a pounce that hit, a steal that touched a player */
    MELYNX_MOT_LEAP_LAND = 0x18,            /* the landing of a missed pounce, and every landing of MELYNX_ATK_LEAP */
    MELYNX_MOT_DIZZY = 0x1F,                /* reeling: MELYNX_IDLE_DIZZY, REACT_DAZE, REACT_STUN */
    MELYNX_MOT_POSTURE_LOOP = 0x20,         /* MELYNX_IDLE_POSTURE, held 150 frames */
    MELYNX_MOT_POSTURE_START = 0x21,        /* MELYNX_MOVE_POSTURE: taking the posture (moves on at frame 46) */
    MELYNX_MOT_POSTURE_END = 0x22,          /* leaving the posture: MELYNX_IDLE_POSTURE, MOVE_POSTURE */
    MELYNX_MOT_IDLE_ACTION_G = 0x23,        /* MELYNX_IDLE_ACTION_G */
    MELYNX_MOT_BOMB_TAKE_OUT = 0x24,        /* MELYNX_IDLE_HOLD_BOMB, HOLD_BIG_BOMB, ATK_THROW_BOMB(_FAR): the bomb made 20 frames after it starts (a timer) */
    MELYNX_MOT_IDLE_ACTION_E = 0x25,        /* MELYNX_IDLE_ACTION_E */
    MELYNX_MOT_IDLE_ACTION_F = 0x26,        /* MELYNX_IDLE_ACTION_F, held 60 to 123 frames */
    MELYNX_MOT_FLINCH = 0x3C,               /* MELYNX_REACT_FLINCH, REACT_STATUS4, DEFEAT_FLINCH */
    MELYNX_MOT_KNOCK_BACK = 0x3D,           /* MELYNX_REACT_KNOCK_BACK, DEFEAT_KNOCK_BACK: thrown back through the air */
    MELYNX_MOT_LIE = 0x3E,                  /* lying after the knock-back (90 frames */
    MELYNX_MOT_GET_UP = 0x3F,               /* getting up: after the knock-back, IDLE_DIZZY, STATUS2_END, STUN_END, REACT_DAZE */
    MELYNX_MOT_FLINCH_END = 0x40,           /* the recovery after MELYNX_MOT_FLINCH (REACT_FLINCH, REACT_STATUS4, DEFEAT_FLINCH) */
    MELYNX_MOT_STATUS4_START = 0x41,        /* MELYNX_REACT_STATUS4, from frame 120 of the flinch, 58 frames */
    MELYNX_MOT_STATUS4_LOOP = 0x42,         /* MELYNX_REACT_STATUS4 until its timer ends */
    MELYNX_MOT_STATUS2_LIE = 0x43,          /* MELYNX_IDLE_STATUS2_LIE */
    MELYNX_MOT_BOMB_THROW = 0x4A            /* MELYNX_ATK_THROW_BOMB(_FAR): lets go of the bomb at frame 48 */
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the effect helpers read the position from). The family's
 * code passes one bone, named by what its contact attacks show: the charge's
 * and the steal's sphere (EmContactAttack kinds 4, 2 and 7: rows 5, 3 and
 * 8, radius 50 on bone 2) is the body's. The pounce's sphere (kind 1: row 2,
 * radius 30 on bone 9) is in the attack's data only.
 *
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind; Em_PlaySeOnFrame plays it at the monster).
 * None is named: none plays on the frame of an event it goes with (the
 * bomb is let go at frame 48 of MELYNX_MOT_BOMB_THROW, its sounds play at
 * 52 and 54; the burrow's dust and part effects fall 0 to 2 frames from
 * sounds that also play in other motions).
 */
enum EmMelynxBone {
    MELYNX_BONE_BODY = 2            /* the contact attacks' body sphere; MELYNX_MOT_BURROW's part effects start here */
};

/* Group 0, EM_GROUP_IDLE (EmMelynx_RunIdleState). */
enum EmMelynxIdleState {
    MELYNX_IDLE_WAIT = 0x00,                /* stands 5 to 20 frames, script picks next */
    MELYNX_IDLE_STAND = 0x01,               /* stands one idle cycle */
    MELYNX_IDLE_ACTION_A = 0x02,            /* idle action, MELYNX_MOT_IDLE_ACTION_A */
    MELYNX_IDLE_ACTION_B = 0x03,            /* idle action, MELYNX_MOT_IDLE_ACTION_B */
    MELYNX_IDLE_ACTION_C = 0x04,            /* idle action, MELYNX_MOT_IDLE_ACTION_C */
    MELYNX_IDLE_ACTION_D = 0x05,            /* MELYNX_MOT_IDLE_ACTION_D for 120 frames */
    MELYNX_IDLE_DIZZY = 0x06,               /* reels under stun stars 60 frames, gets up */
    MELYNX_IDLE_ACTION_E = 0x07,            /* idle action, MELYNX_MOT_IDLE_ACTION_E */
    MELYNX_IDLE_ACTION_F = 0x08,            /* MELYNX_MOT_IDLE_ACTION_F for 60 to 123 frames */
    MELYNX_IDLE_POSTURE = 0x09,             /* holds the posture 150 frames */
    MELYNX_IDLE_ACTION_G = 0x0A,            /* idle action, MELYNX_MOT_IDLE_ACTION_G, sight back */
    MELYNX_IDLE_HOLD_BOMB = 0x0B,           /* takes out and holds a bomb (type 0) */
    MELYNX_IDLE_STATUS2_LIE = 0x0C,         /* lies under status 2 (EM_DMG_STATUS2) */
    MELYNX_IDLE_STATUS2_END = 0x0D,         /* ends status 2, gets up */
    MELYNX_IDLE_BURROW_AWAY = 0x0E,         /* burrows away with the loot (QuestEm_Appear) */
    MELYNX_IDLE_HOLD_BIG_BOMB = 0x0F,       /* takes out and holds the big bomb (type 1) */
    MELYNX_IDLE_STUN_END = 0x10             /* ends status 5, gets up */
};

/* Group 1, EM_GROUP_MOVE (EmMelynx_RunMoveState). */
enum EmMelynxMoveState {
    MELYNX_MOVE_CHASE = 0x00,               /* runs to the goal re-aiming, off-target cut */
    MELYNX_MOVE_POSTURE = 0x01,             /* moves to the goal in the posture */
    MELYNX_MOVE_WALK_LONG = 0x02,           /* walks to the goal, 600 frames, goal kept */
    MELYNX_MOVE_CHARGE = 0x04,              /* dash with contact attack 4 */
    MELYNX_MOVE_TURN = 0x05,                /* turns in place to the goal */
    MELYNX_MOVE_RUN_A = 0x06,               /* runs to the goal (MELYNX_MOT_RUN_A) */
    MELYNX_MOVE_RUN_B = 0x07,               /* runs to the goal (MELYNX_MOT_RUN_B) */
    MELYNX_MOVE_WANDER = 0x08,              /* walks to a random heading */
    MELYNX_MOVE_DASH = 0x09,                /* dash without attack, 100 frames */
    MELYNX_MOVE_WANDER_RUN = 0x0A,          /* runs to a random heading, 120 frames */
    MELYNX_MOVE_WALK = 0x0B,                /* walks to the goal, 300 frames */
    MELYNX_MOVE_WALK_HOME = 0x0C            /* walks turning toward home */
};

/* Group 2, EM_GROUP_FLY (EmMelynx_RunFlyState): for this family, only the fall. */
enum EmMelynxFlyState {
    MELYNX_FLY_FALL = 0x00                  /* falls to the floor */
};

/* Group 3, EM_GROUP_ATTACK (EmMelynx_RunAttackState). */
enum EmMelynxAttackState {
    MELYNX_ATK_POUNCE = 0x00,               /* crouch, leap with contact attack 1, land */
    MELYNX_ATK_STEAL = 0x01,                /* steal run, contact attack 2, 60 frames */
    MELYNX_ATK_LEAP = 0x02,                 /* leap at once with contact attack 1 */
    MELYNX_ATK_STEAL_CHASE = 0x03,          /* steal run re-aiming, contact attack 7, 120 frames */
    MELYNX_ATK_THROW_BOMB = 0x04,           /* throws a bomb (type 2) */
    MELYNX_ATK_THROW_BOMB_FAR = 0x05        /* throws a bomb farther (type 3) */
};

/* Group 4, EM_GROUP_REACT (EmMelynx_RunReactState). */
enum EmMelynxReactState {
    MELYNX_REACT_FLINCH = 0x00,             /* EM_DMG_HIT on the ground */
    MELYNX_REACT_KNOCK_BACK = 0x01,         /* thrown back, lies, gets up (most hit results) */
    MELYNX_REACT_DAZE = 0x02,               /* reels, request 0 (entered by the script) */
    MELYNX_REACT_STATUS4 = 0x03,            /* EM_DMG_STATUS4 */
    MELYNX_REACT_STUN = 0x04                /* EM_DMG_STATUS5, stun stars */
};

/* Group 5, EM_GROUP_DEFEAT (EmMelynx_RunDefeatState). */
enum EmMelynxDefeatState {
    MELYNX_DEFEAT_KNOCK_BACK = 0x00,        /* hunted off the ground, item returned, burrows away */
    MELYNX_DEFEAT_FLINCH = 0x01,            /* hunted on the ground, item returned, burrows away */
    MELYNX_DEFEAT_REAPPEAR = 0x02           /* appears again or finishes */
};

/* Group 6, EM_GROUP_EVENT (EmMelynx_RunEventState). */
enum EmMelynxEventState {
    MELYNX_EVENT_APPEAR = 0x00              /* one hidden frame, then shown */
};

extern void *EmMelynx_Callbacks[8];
void EmMelynx_Reset(Em *em);
void EmMelynx_Start(Em *em);
void EmMelynx_DeductReward(Em *em);
void EmMelynx_CapTimerOffTarget(Em *em);
void EmMelynx_ChangeState(Em *em, u16 group, u16 state, u8 mode);
s32 EmMelynx_BombState(Em *em);
void EmMelynx_ReturnItem(Em *em, EmMelynxWork *work);
void EmMelynx_Status2Lie(Em *em);
void EmMelynx_BurrowAway(Em *em);
void EmMelynx_ChaseGoal(Em *em);
void EmMelynx_WalkLong(Em *em);
void EmMelynx_RunToGoalA(Em *em);
void EmMelynx_RunToGoalB(Em *em);
void EmMelynx_WalkToGoal(Em *em);
void EmMelynx_StealAttack(Em *em);
void EmMelynx_KnockBack(Em *em);
void EmMelynx_Status4(Em *em);
void EmMelynx_HuntedKnockBack(Em *em);
void EmMelynx_HuntedFlinch(Em *em);
void EmMelynx_Update(Em *em);
void EmMelynx_RunState(Em *em);
void EmMelynx_MotionEffects(Em *em, EmWork *work);

#endif
