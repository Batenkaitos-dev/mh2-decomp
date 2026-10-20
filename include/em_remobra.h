#ifndef EM_REMOBRA_H
#define EM_REMOBRA_H

/*
 * The Remobra family (EM_KIND_REMOBRA): its motions and its states, by group
 * (em.h's EmStateGroup). The functions are EmRemobra_*. These flying serpents
 * spend most of their time in the air (stance EM_STANCE_AIR): they hover,
 * bank, swoop and glide to their goal, spit from the air
 * (EMRANGED_REMOBRA_SPIT) and strike, and fall to the ground when hit hard.
 * Once hunted, a quest's Remobra waits to appear again
 * (REMOBRA_DEFEAT_WAIT_APPEAR, then the reappearance in EM_GROUP_EVENT2). Each enum
 * lists the cases of the group's dispatcher (EmRemobra_Run*State); a name
 * ending in NONE is a number the dispatcher accepts but runs nothing for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmRemobraMotion {
    REMOBRA_MOT_FLY = 0x01,                 /* the flying loop (motion row 1) with the hops of ... */
    REMOBRA_MOT_SWOOP_END = 0x03,           /* REMOBRA_FLY_SWOOP_END, the pull-up after a swoop */
    REMOBRA_MOT_SWOOP = 0x04,               /* REMOBRA_FLY_SWOOP, and the slowing end of ... */
    REMOBRA_MOT_BANK_SLOW_A = 0x06,         /* REMOBRA_FLY_BANK_SLOW_A */
    REMOBRA_MOT_BANK_SLOW_B = 0x07,         /* REMOBRA_FLY_BANK_SLOW_B */
    REMOBRA_MOT_BANK_FAST_A = 0x08,         /* REMOBRA_FLY_BANK_FAST_A */
    REMOBRA_MOT_BANK_FAST_B = 0x09,         /* REMOBRA_FLY_BANK_FAST_B */
    REMOBRA_MOT_BACK_OFF = 0x0A,            /* REMOBRA_FLY_BACK_OFF_A/_B: the leap up and back from frame ... */
    REMOBRA_MOT_TAKE_OFF = 0x0B,            /* REMOBRA_FLY_TAKE_OFF_* (airborne at frame 196) */
    REMOBRA_MOT_SPIT_DOWN = 0x0C,           /* REMOBRA_ATK_SPIT_DOWN and REMOBRA_FLY_SWOOP_SPIT: a steep ... */
    REMOBRA_MOT_SPIT_AIM = 0x0E,            /* REMOBRA_ATK_SPIT_A/B/C: held 40, 50 or 60 frames before the ... */
    REMOBRA_MOT_SPIT = 0x0F,                /* REMOBRA_ATK_SPIT_A/B/C: the spit at frame 8 */
    REMOBRA_MOT_STRIKE = 0x10,              /* REMOBRA_ATK_STRIKE (RemobraAttack type 1 at frame 46) */
    REMOBRA_MOT_SWOOP_STRIKE = 0x11,        /* REMOBRA_ATK_SWOOP_STRIKE (curve kind 0, RemobraAttack types ... */
    REMOBRA_MOT_FLINCH = 0x12,              /* REMOBRA_REACT_FLINCH */
    REMOBRA_MOT_FALL = 0x13,                /* REMOBRA_REACT_FALL and REMOBRA_DEFEAT_FALL: falling from ... */
    REMOBRA_MOT_FALL_LAND = 0x14,           /* hitting the ground at the end of REMOBRA_REACT_FALL and ... */
    REMOBRA_MOT_DOWNED = 0x15,              /* REMOBRA_REACT_DOWNED: lying on the ground for 150 frames */
    REMOBRA_MOT_STATUS4 = 0x16,             /* REMOBRA_REACT_STATUS4, with the status-4 effect */
    REMOBRA_MOT_DEATH = 0x19,               /* REMOBRA_DEFEAT_HUNTED (carve point at frame 158) */
    REMOBRA_MOT_HOVER_CRY = 0x1A,           /* REMOBRA_FLY_RISE_CRY: a cry while hovering (voice at the ... */
    REMOBRA_MOT_IDLE_GROUND = 0x1B          /* REMOBRA_IDLE_GROUND: standing on the ground */
};

/* Group 0, EM_GROUP_IDLE (EmRemobra_RunIdleState). */
enum EmRemobraIdleState {
    REMOBRA_IDLE_NONE = 0x00,               /* does nothing */
    REMOBRA_IDLE_GROUND = 0x01              /* stands on the ground, loops */
};

/* Group 1, EM_GROUP_MOVE (EmRemobra_RunMoveState). */
enum EmRemobraMoveState {
    REMOBRA_MOVE_NONE = 0x00                /* ends at once to REMOBRA_IDLE_GROUND */
};

/* Group 2, EM_GROUP_FLY (EmRemobra_RunFlyState). */
enum EmRemobraFlyState {
    REMOBRA_FLY_HOVER = 0x00,               /* flies in place, rising to 400 over the floor */
    REMOBRA_FLY_SWOOP_END = 0x01,           /* pull-up after a swoop */
    REMOBRA_FLY_SWOOP = 0x02,               /* swoop toward the goal */
    REMOBRA_FLY_BANK_SLOW_A = 0x03,         /* slow turning curve, one side, then swoop */
    REMOBRA_FLY_BANK_SLOW_B = 0x04,         /* slow turning curve, other side, then swoop */
    REMOBRA_FLY_BANK_FAST_A = 0x05,         /* fast turning curve, one side, then swoop */
    REMOBRA_FLY_BANK_FAST_B = 0x06,         /* fast turning curve, other side, then swoop */
    REMOBRA_FLY_BACK_OFF_A = 0x07,          /* leap up and back */
    REMOBRA_FLY_BACK_OFF_B = 0x08,          /* higher, farther leap up and back */
    REMOBRA_FLY_TAKE_OFF_START = 0x09,      /* take-off from frame 90 (EmRemobra_Start) */
    REMOBRA_FLY_TURN_TO_GOAL = 0x0A,        /* turn in the air to the goal */
    REMOBRA_FLY_TAKE_OFF = 0x0B,            /* take-off from the ground */
    REMOBRA_FLY_CHANGE_HEIGHT_RANDOM = 0x0C,/* climb or sink to a random height */
    REMOBRA_FLY_CHANGE_HEIGHT_GOAL = 0x0D,  /* climb or sink to the goal height */
    REMOBRA_FLY_TAKE_OFF_STATUS4 = 0x0E,    /* take-off ending status 4 */
    REMOBRA_FLY_GLIDE_TO_GOAL = 0x0F,       /* fly forward to the goal, stopping 300 short */
    REMOBRA_FLY_RISE_CRY = 0x10,            /* rise, then the hovering cry */
    REMOBRA_FLY_RETURN_HOME = 0x11,         /* fly back to the home position */
    REMOBRA_FLY_TURN_RANDOM = 0x12,         /* turn in the air by a random angle */
    REMOBRA_FLY_GLIDE_FIXED = 0x13,         /* fly straight 600 without steering */
    REMOBRA_FLY_SWOOP_SPIT = 0x14,          /* swoop toward the goal while spitting */
    REMOBRA_FLY_SHIFT_FORWARD = 0x15,       /* drift forward */
    REMOBRA_FLY_SHIFT_BACK = 0x16,          /* drift back */
    REMOBRA_FLY_SHIFT_SIDE_A = 0x17,        /* drift sideways (+X) */
    REMOBRA_FLY_SHIFT_SIDE_B = 0x18,        /* drift sideways (-X) */
    REMOBRA_FLY_SHIFT_BACK_UP = 0x19,       /* drift back and up */
    REMOBRA_FLY_RISE_HIGH = 0x1A            /* climb to 500 over the floor */
};

/* Group 3, EM_GROUP_ATTACK (EmRemobra_RunAttackState). */
enum EmRemobraAttackState {
    REMOBRA_ATK_STRIKE = 0x00,              /* RemobraAttack type 1 */
    REMOBRA_ATK_SPIT_A = 0x01,              /* aimed spit, 40-frame hold */
    REMOBRA_ATK_SPIT_B = 0x02,              /* aimed spit, 50-frame hold */
    REMOBRA_ATK_SPIT_C = 0x03,              /* aimed spit, 60-frame hold */
    REMOBRA_ATK_SPIT_DOWN = 0x04,           /* quick steep spit */
    REMOBRA_ATK_SWOOP_STRIKE = 0x05         /* curve-driven pass with RemobraAttack types 2 and 3 */
};

/* Group 4, EM_GROUP_REACT (EmRemobra_RunReactState). */
enum EmRemobraReactState {
    REMOBRA_REACT_FLINCH = 0x00,            /* flinch in the air */
    REMOBRA_REACT_FALL = 0x01,              /* fall to the ground */
    REMOBRA_REACT_DOWNED = 0x02,            /* lie on the ground, get up */
    REMOBRA_REACT_STATUS4 = 0x03            /* status 4 */
};

/* Group 5, EM_GROUP_DEFEAT (EmRemobra_RunDefeatState). */
enum EmRemobraDefeatState {
    REMOBRA_DEFEAT_HUNTED = 0x00,           /* death, carve point, fade out */
    REMOBRA_DEFEAT_WAIT_APPEAR = 0x01,      /* wait to appear again */
    REMOBRA_DEFEAT_FALL = 0x02              /* killed in the air: fall, then hunted */
};

/* Group 6, EM_GROUP_EVENT (EmRemobra_RunEventState). */
enum EmRemobraEventState {
    REMOBRA_EVENT_WAIT = 0x00               /* scripted scene, senses off */
};

/* Group 7, EM_GROUP_EVENT2 (EmRemobra_RunEvent2State). */
enum EmRemobraEvent2State {
    REMOBRA_EVENT2_REAPPEAR = 0x00          /* fly in after appearing again */
};

#endif
