#ifndef EM_LAOSHAN_H
#define EM_LAOSHAN_H

/*
 * The Lao-Shan Lung family (EM_KIND_LAO_SHAN_LUNG and
 * EM_KIND_ASHEN_LAO_SHAN_LUNG): its motions and its states, by group (em.h's
 * EmStateGroup). The functions are EmLaoshan_*. The monster walks through the
 * fortress areas towards the fort, on all fours (LAOSHAN_STANCE_ALL_FOURS)
 * or reared on its hind legs (LAOSHAN_STANCE_REARED): LAOSHAN_MOT_REAR_UP and
 * LAOSHAN_MOT_REAR_DOWN switch between the two. It rams and smashes the fort, counting the hits in
 * fortHits, and it has no flying states. Each enum lists the cases of the
 * group's dispatcher (EmLaoshan_Run*State).
 */

/*
 * The family's two stances (Em.stance), by em.h's EmStance numbers. The
 * family uses stance 0 for the reared posture: LAOSHAN_MOT_REAR_UP takes the
 * monster from 1 to 0 and LAOSHAN_MOT_REAR_DOWN back, the reared smash hits
 * the fort at a height of 1700 where the ram on all fours hits it at 500,
 * and Init starts on all fours. "Reared" is read from this.
 */
#define LAOSHAN_STANCE_REARED       EM_STANCE_GROUND
#define LAOSHAN_STANCE_ALL_FOURS    EM_STANCE_FAMILY

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmLaoshanMotion {
    LAOSHAN_MOT_IDLE = 0x01,                /* the idle on all fours (layers 0 to 2: LAOSHAN_IDLE, _HOLD, ... */
    LAOSHAN_MOT_WALK_BACK = 0x02,           /* LAOSHAN_MOVE_WALK_BACK and _BACK_ON (goal behind, ... */
    LAOSHAN_MOT_WALK = 0x03,                /* LAOSHAN_MOVE_WALK, _WALK_ON, _TURN_TO_GOAL (in place) and ... */
    LAOSHAN_MOT_WALK_TURN_RIGHT = 0x04,     /* LAOSHAN_MOVE_WALK_TURN_RIGHT and LAOSHAN_MOVE_TURN_TO_GOAL: ... */
    LAOSHAN_MOT_WALK_TURN_LEFT = 0x05,      /* LAOSHAN_MOVE_TURN_TO_GOAL: walking turn, rotY falling */
    LAOSHAN_MOT_STOMP_START = 0x06,         /* LAOSHAN_IDLE_STOMP, before LAOSHAN_MOT_STOMP */
    LAOSHAN_MOT_STOMP = 0x07,               /* LAOSHAN_IDLE_STOMP with quakeTimer (effects: ElementBurst, ... */
    LAOSHAN_MOT_REAR_UP = 0x08,             /* LAOSHAN_IDLE_REAR_UP (to stance 0), LAOSHAN_DEFEAT_DIE, ... */
    LAOSHAN_MOT_RAM = 0x0A,                 /* LAOSHAN_ATK_RAM_FORT and its stage variants: hits the fort ... */
    LAOSHAN_MOT_REARED_IDLE = 0x0C,         /* LAOSHAN_IDLE_REARED, _REARED_SHORT, LAOSHAN_EVENT_INTRO */
    LAOSHAN_MOT_REARED_STRIKE = 0x0D,       /* LAOSHAN_ATK_REARED_STRIKE, _STRIKE_TURN, LAOSHAN_EVENT_INTRO */
    LAOSHAN_MOT_REARED_SMASH = 0x0E,        /* LAOSHAN_ATK_REARED_SMASH_FORT: hits the fort high point at ... */
    LAOSHAN_MOT_REAR_DOWN = 0x0F,           /* LAOSHAN_IDLE_REAR_DOWN, LAOSHAN_REACT_REAR_DOWN, ... */
    LAOSHAN_MOT_REARED_WALK = 0x10,         /* LAOSHAN_MOVE_WALK_REARED */
    LAOSHAN_MOT_TURN_RIGHT_START = 0x14,    /* LAOSHAN_MOVE_TURN_RIGHT_START: rotY rising 0x80 a frame, ... */
    LAOSHAN_MOT_TURN_RIGHT_HOLD = 0x15,     /* LAOSHAN_MOVE_TURN_RIGHT_HOLD */
    LAOSHAN_MOT_TURN_RIGHT_BACK = 0x16,     /* LAOSHAN_MOVE_TURN_RIGHT_BACK: rotY falling 0x80 a frame, ... */
    LAOSHAN_MOT_TURN_RIGHT_END = 0x17,      /* LAOSHAN_MOVE_TURN_RIGHT_END */
    LAOSHAN_MOT_QUARTER_TURN_RIGHT = 0x18,  /* LAOSHAN_MOVE_QUARTER_TURN_RIGHT: rotY rising 0x7E a frame, ... */
    LAOSHAN_MOT_HALF_TURN_RIGHT = 0x19,     /* LAOSHAN_MOVE_HALF_TURN_RIGHT: rotY rising 0x100 a frame, ... */
    LAOSHAN_MOT_TURN_LEFT_START = 0x1A,     /* LAOSHAN_MOVE_TURN_LEFT_START: rotY falling 0x80 a frame */
    LAOSHAN_MOT_TURN_LEFT_HOLD = 0x1B,      /* LAOSHAN_MOVE_TURN_LEFT_HOLD */
    LAOSHAN_MOT_TURN_LEFT_BACK = 0x1C,      /* LAOSHAN_MOVE_TURN_LEFT_BACK: rotY rising 0x80 a frame */
    LAOSHAN_MOT_TURN_LEFT_END = 0x1D,       /* LAOSHAN_MOVE_TURN_LEFT_END */
    LAOSHAN_MOT_QUARTER_TURN_LEFT = 0x1E,   /* LAOSHAN_MOVE_QUARTER_TURN_LEFT */
    LAOSHAN_MOT_HALF_TURN_LEFT = 0x1F,      /* LAOSHAN_MOVE_HALF_TURN_LEFT */
    LAOSHAN_MOT_FALL = 0x3C,                /* LAOSHAN_REACT_FALL and _FALL_LONG: falls (effects: ... */
    LAOSHAN_MOT_GET_UP = 0x3D,              /* LAOSHAN_REACT_FALL and _FALL_LONG: gets up */
    LAOSHAN_MOT_REARED_FLINCH = 0x3E,       /* LAOSHAN_REACT_REARED_FLINCH and _REARED_FLINCH_DOWN */
    LAOSHAN_MOT_PUSHED_BACK = 0x3F,         /* LAOSHAN_REACT_PUSHED_BACK, moving backward */
    LAOSHAN_MOT_LEAVE_START = 0x40,         /* LAOSHAN_EVENT_REPELLED_LEAVE, first motion */
    LAOSHAN_MOT_LEAVE_TURN = 0x41,          /* LAOSHAN_EVENT_REPELLED_LEAVE: turn, rotY rising 0x210 a ... */
    LAOSHAN_MOT_WALK_ATTACK = 0x42,         /* LAOSHAN_MOVE_WALK_ATTACK (LaoShanAttack kinds 0x42, 0x43) ... */
    LAOSHAN_MOT_DIE = 0x43,                 /* LAOSHAN_DEFEAT_DIE and _DIE_REARED: the collapse (effects: ... */
    LAOSHAN_MOT_FLINCH = 0x44               /* LAOSHAN_REACT_FLINCH, and the start of LAOSHAN_REACT_FALL ... */
};

/* Group 0, EM_GROUP_IDLE (EmLaoshan_RunIdleState). */
enum EmLaoshanIdleState {
    LAOSHAN_IDLE_HOLD = 0x00,               /* keeps the idle motion on all layers, no end */
    LAOSHAN_IDLE = 0x01,                    /* idle on all fours, Init and EndState target */
    LAOSHAN_IDLE_STOMP = 0x02,              /* stomp with quakeTimer */
    LAOSHAN_IDLE_REAR_UP = 0x03,            /* rears up to stance 0 */
    LAOSHAN_IDLE_REARED = 0x04,             /* reared idle, EndState target in stance 0 */
    LAOSHAN_IDLE_REAR_DOWN = 0x05,          /* drops back to all fours */
    LAOSHAN_IDLE_SHORT = 0x06,              /* idle of at most 30 frames */
    LAOSHAN_IDLE_REARED_SHORT = 0x07        /* reared idle of at most 30 frames */
};

/* Group 1, EM_GROUP_MOVE (EmLaoshan_RunMoveState). */
enum EmLaoshanMoveState {
    LAOSHAN_MOVE_WALK = 0x00,               /* walk to the goal */
    LAOSHAN_MOVE_WALK_BACK = 0x01,          /* walk to a goal behind */
    LAOSHAN_MOVE_TURN_TO_GOAL = 0x02,       /* turning walk until facing the goal */
    LAOSHAN_MOVE_WALK_TURN_RIGHT = 0x03,    /* walking turn, rotY rising */
    LAOSHAN_MOVE_TURN_RIGHT_START = 0x04,   /* quarter turn, then held (turnHeld 1) */
    LAOSHAN_MOVE_TURN_RIGHT_HOLD = 0x05,    /* holds the turned pose */
    LAOSHAN_MOVE_TURN_RIGHT_END = 0x06,     /* leaves the pose, new heading kept */
    LAOSHAN_MOVE_TURN_RIGHT_BACK = 0x07,    /* leaves the pose turning back */
    LAOSHAN_MOVE_TURN_LEFT_START = 0x08,    /* quarter turn, then held (turnHeld 2) */
    LAOSHAN_MOVE_TURN_LEFT_HOLD = 0x09,     /* holds the turned pose */
    LAOSHAN_MOVE_TURN_LEFT_END = 0x0A,      /* leaves the pose, new heading kept */
    LAOSHAN_MOVE_TURN_LEFT_BACK = 0x0B,     /* leaves the pose turning back */
    LAOSHAN_MOVE_HALF_TURN_RIGHT = 0x0C,    /* half turn, rotY rising */
    LAOSHAN_MOVE_HALF_TURN_LEFT = 0x0D,     /* half turn, rotY falling */
    LAOSHAN_MOVE_QUARTER_TURN_RIGHT = 0x0E, /* quarter turn, rotY rising */
    LAOSHAN_MOVE_QUARTER_TURN_LEFT = 0x0F,  /* quarter turn, rotY falling */
    LAOSHAN_MOVE_WALK_REARED = 0x10,        /* reared walk to the goal */
    LAOSHAN_MOVE_WALK_ATTACK = 0x11,        /* walk with LaoShanAttack hits */
    LAOSHAN_MOVE_WALK_ON = 0x12,            /* walk, then the rest of the motion */
    LAOSHAN_MOVE_WALK_BACK_ON = 0x13        /* backward walk, then the rest of the motion */
};

/* Group 3, EM_GROUP_ATTACK (EmLaoshan_RunAttackState). */
enum EmLaoshanAttackState {
    LAOSHAN_ATK_REARED_STRIKE = 0x00,       /* reared strike */
    LAOSHAN_ATK_REARED_SMASH_FORT = 0x01,   /* reared smash at the fort high point */
    LAOSHAN_ATK_RAM_FORT = 0x02,            /* ram counting fortHits */
    LAOSHAN_ATK_REARED_STRIKE_TURN = 0x03,  /* reared strike with turns */
    LAOSHAN_ATK_RAM_FORT_STAGE2 = 0x04,     /* ram setting stage 2 */
    LAOSHAN_ATK_RAM_FORT_STAGE3 = 0x05      /* ram setting stage 3 and emEventFlags bit 0 */
};

/* Group 4, EM_GROUP_REACT (EmLaoshan_RunReactState). */
enum EmLaoshanReactState {
    LAOSHAN_REACT_FALL = 0x00,              /* flinch, fall and get up (part 2) */
    LAOSHAN_REACT_REARED_FLINCH = 0x01,     /* reared flinch */
    LAOSHAN_REACT_PUSHED_BACK = 0x02,       /* pushed back on all fours */
    LAOSHAN_REACT_FLINCH = 0x03,            /* flinch on all fours */
    LAOSHAN_REACT_REAR_DOWN = 0x04,         /* drop from reared */
    LAOSHAN_REACT_REARED_FLINCH_DOWN = 0x05,/* reared flinch then drop */
    LAOSHAN_REACT_FALL_LONG = 0x06          /* fall held 300 frames (part 3) */
};

/* Group 5, EM_GROUP_DEFEAT (EmLaoshan_RunDefeatState). */
enum EmLaoshanDefeatState {
    LAOSHAN_DEFEAT_DIE = 0x00,              /* rears, collapses, carcass mine points */
    LAOSHAN_DEFEAT_DIE_REARED = 0x01        /* collapses from reared */
};

/* Group 6, EM_GROUP_EVENT (EmLaoshan_RunEventState). */
enum EmLaoshanEventState {
    LAOSHAN_EVENT_INTRO = 0x00,             /* the fortress area 1 opening scene */
    LAOSHAN_EVENT_REPELLED_LEAVE = 0x01     /* leaves fortress area 5 once the goal is met */
};

#endif
