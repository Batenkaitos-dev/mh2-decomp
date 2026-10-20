#ifndef EM_CHAMELEOS_H
#define EM_CHAMELEOS_H

/*
 * The Chameleos family (EM_KIND_CHAMELEOS): its motions and its states, by
 * group (em.h's EmStateGroup). The functions are EmChameleos_*. The monster
 * walks, flies and hovers (stance EM_STANCE_AIR in the air), attacks with its
 * tongue, which can steal an item, spits balls and breath, and cloaks itself:
 * with Em.fullCloak set it is invisible but for a shimmer every 900 frames,
 * without it it shimmers all the time; after its part break it can only
 * shimmer, and once part 0 is broken it cannot cloak at all. Each
 * enum lists the cases of the group's dispatcher (EmChameleos_Run*State); a
 * name ending in NONE is a number the dispatcher accepts but runs nothing
 * for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmChameleosMotion {
    CHAM_MOT_IDLE = 0x01,                   /* CHAM_IDLE, CHAM_EVENT_FOREST's pauses */
    CHAM_MOT_WALK = 0x02,                   /* CHAM_MOVE_WALK (Em_StepToGoal), CHAM_EVENT_FOREST */
    CHAM_MOT_RUN = 0x03,                    /* CHAM_MOVE_RUN and CHAM_MOVE_RUN_NO_HIT, to the goal */
    CHAM_MOT_RUN_STOP = 0x04,               /* the end of a run */
    CHAM_MOT_SPIN_TURN = 0x07,              /* CHAM_MOVE_SPIN_TURN, rotY falling 0x590 a frame from 44 to ... */
    CHAM_MOT_BACK_HOP = 0x09,               /* CHAM_MOVE_BACK_HOP, airborne from 44 to 82 */
    CHAM_MOT_TAKE_OFF = 0x0A,               /* CHAM_FLY_TAKE_OFF and CHAM_EVENT_LEAVE */
    CHAM_MOT_FLY = 0x0C,                    /* the forward flight of the route, area and goal flights ... */
    CHAM_MOT_FLY_START = 0x0D,              /* going into forward flight from slow (CHAM_FLY_ROUTE, ... */
    CHAM_MOT_HOVER_SETTLE = 0x0F,           /* CHAM_FLY_BRAKE_AND_LAND after braking, before the landing */
    CHAM_MOT_BRAKE = 0x10,                  /* CHAM_FLY_BRAKE and CHAM_FLY_BRAKE_AND_LAND: slowing out of ... */
    CHAM_MOT_LAND = 0x11,                   /* CHAM_FLY_LAND: touching down (stance ground, camera shake) */
    CHAM_MOT_HOVER = 0x12,                  /* hovering: CHAM_FLY_HOVER, LAND, HOVER_TO_GOAL, INTO_AREA, ... */
    CHAM_MOT_TAKE_OFF_END = 0x13,           /* the end of a take-off, 600 above the floor */
    CHAM_MOT_IDLE_ALERT = 0x1F,             /* CHAM_IDLE_ALERT */
    CHAM_MOT_TURN_TO_GOAL = 0x20,           /* CHAM_IDLE_FACE_GOAL */
    CHAM_MOT_IDLE_ACTION_A = 0x21,          /* CHAM_IDLE_ACTION_A */
    CHAM_MOT_CLOAK = 0x22,                  /* CHAM_IDLE_CLOAK_SHIMMER and CHAM_IDLE_CLOAK_FULL */
    CHAM_MOT_LIMP = 0x23,                   /* CHAM_MOVE_LIMP, moving on its root motion */
    CHAM_MOT_REST = 0x24,                   /* CHAM_IDLE_REST, lying with periodic puffs */
    CHAM_MOT_REST_END = 0x25,               /* CHAM_IDLE_REST_END */
    CHAM_MOT_TONGUE_LOW = 0x29,             /* CHAM_ATK_TONGUE_LOW: ChameleosAttack types 2 and 8 (type 2 ... */
    CHAM_MOT_SPIT_START = 0x2A,             /* the wind-up of the ball and breath attacks (ChameleosAttack ... */
    CHAM_MOT_SPIT_A = 0x2B,                 /* CHAM_ATK_BALL_PITCHED / CHAM_ATK_BREATH_PITCHED, the attack ... */
    CHAM_MOT_SPIT_A_END = 0x2C,             /* after CHAM_MOT_SPIT_A */
    CHAM_MOT_STOMP = 0x2D,                  /* CHAM_ATK_STOMP: ChameleosAttack type 3, three ground ... */
    CHAM_MOT_TAIL_SLAM = 0x2E,              /* CHAM_ATK_TAIL_SLAM: ChameleosAttack type 4 (backward), ... */
    CHAM_MOT_BACK_HOP_BALL = 0x2F,          /* CHAM_ATK_BACK_HOP_BALL, airborne from frame 88 */
    CHAM_MOT_TAIL_POUNDS = 0x30,            /* CHAM_ATK_TAIL_POUNDS and the start of the THUMP_TONGUE ... */
    CHAM_MOT_TONGUE = 0x31,                 /* CHAM_ATK_TONGUE, CHAM_ATK_TONGUE_QUICK and the eating ... */
    CHAM_MOT_TONGUE_B = 0x32,               /* CHAM_ATK_TONGUE_B: ChameleosAttack types 5 and 1 */
    CHAM_MOT_TONGUE_HIGH = 0x33,            /* CHAM_ATK_TONGUE_HIGH: tongue (type 1), ground effect 350 ... */
    CHAM_MOT_SPIT_B = 0x34,                 /* CHAM_ATK_BALL_LEVEL / CHAM_ATK_BREATH_LEVEL, the attack ... */
    CHAM_MOT_SPIT_B_END = 0x35,             /* after CHAM_MOT_SPIT_B */
    CHAM_MOT_FLINCH_B = 0x3D,               /* CHAM_REACT_FLINCH_B and CHAM_REACT_FLINCH_C */
    CHAM_MOT_RECOVER = 0x3E,                /* after CHAM_MOT_COLLAPSE from frame 96 (CHAM_REACT_STAGGER, ... */
    CHAM_MOT_FLINCH_A = 0x3F,               /* CHAM_REACT_FLINCH_A */
    CHAM_MOT_FALL = 0x40,                   /* falling out of the air (CHAM_REACT_FALL*, ... */
    CHAM_MOT_FALL_LAND = 0x41,              /* hitting the floor after CHAM_MOT_FALL */
    CHAM_MOT_DOWN = 0x42,                   /* lying knocked down (CHAM_REACT_KNOCK_DOWN, STUN, FALL*) */
    CHAM_MOT_GET_UP = 0x43,                 /* getting up from a knock-down, a fall or statuses 1 and 2 */
    CHAM_MOT_DIE = 0x44,                    /* CHAM_DEFEAT_DIE */
    CHAM_MOT_STATUS4 = 0x45,                /* CHAM_REACT_STATUS4 */
    CHAM_MOT_SHAKE_OFF = 0x46,              /* CHAM_IDLE_SHAKE_OFF, STUN_SHAKE_OFF, STATUS4_SHAKE_OFF */
    CHAM_MOT_STATUS_DOWN = 0x47,            /* lying under statuses 1 and 2 (CHAM_IDLE_STATUS1_DOWN, ... */
    CHAM_MOT_COLLAPSE = 0x49,               /* falling over: CHAM_REACT_STAGGER*, KNOCK_DOWN, STUN, ... */
    CHAM_MOT_PART_BREAK = 0x4A,             /* CHAM_REACT_PART_BREAK's start */
    CHAM_MOT_PART_BREAK_SPIN = 0x4B         /* CHAM_REACT_PART_BREAK, turning 630 a frame to frame 104 */
};

/* Group 0, EM_GROUP_IDLE (EmChameleos_RunIdleState). */
enum EmChameleosIdleState {
    CHAM_IDLE_STAND = 0x00,                 /* layered idle until the timer runs out */
    CHAM_IDLE = 0x01,                       /* idle once */
    CHAM_IDLE_ALERT = 0x02,                 /* idle in combat */
    CHAM_IDLE_FACE_GOAL = 0x03,             /* turn motion facing the goal */
    CHAM_IDLE_CLOAK_FULL = 0x04,            /* allows the full cloak (EmChameleos_SetFullCloak) */
    CHAM_IDLE_CLOAK_SHIMMER = 0x05,         /* only the shimmering cloak (EmChameleos_ClearFullCloak) */
    CHAM_IDLE_ACTION_A = 0x09,              /* one idle motion */
    CHAM_IDLE_EAT_NEED1 = 0x0A,             /* eats with the tongue, refills need 1 */
    CHAM_IDLE_EAT_NEED2 = 0x0B,             /* eats three times, refills need 2 */
    CHAM_IDLE_REST = 0x12,                  /* lies regenerating hp */
    CHAM_IDLE_STATUS2_DOWN = 0x14,          /* lies under status 2 */
    CHAM_IDLE_REST_END = 0x17,              /* gets up from rest */
    CHAM_IDLE_STATUS2_END = 0x18,           /* gets up from status 2 */
    CHAM_IDLE_SHAKE_OFF = 0x19,             /* shakes itself */
    CHAM_IDLE_STATUS1_DOWN = 0x1B,          /* lies under status 1 */
    CHAM_IDLE_STATUS1_END = 0x1C,           /* gets up from status 1 */
    CHAM_IDLE_STUN_SHAKE_OFF = 0x20,        /* shakes off the stun (Em_EndStatus5) */
    CHAM_IDLE_STATUS4_SHAKE_OFF = 0x21      /* shakes off status 4 (Em_EndStatus4) */
};

/* Group 1, EM_GROUP_MOVE (EmChameleos_RunMoveState). */
enum EmChameleosMoveState {
    CHAM_MOVE_WALK = 0x00,                  /* walk to the goal */
    CHAM_MOVE_LIMP = 0x01,                  /* slow walk below 15% hp */
    CHAM_MOVE_TURN = 0x02,                  /* turn to the target */
    CHAM_MOVE_NONE = 0x03,                  /* the dispatcher runs nothing */
    CHAM_MOVE_RUN = 0x04,                   /* run to the goal with a hit */
    CHAM_MOVE_SPIN_TURN = 0x05,             /* spinning turn */
    CHAM_MOVE_BACK_HOP = 0x06,              /* hop backward */
    CHAM_MOVE_RUN_NO_HIT = 0x07,            /* run without the hit */
    CHAM_MOVE_TURN_RANDOM = 0x08            /* turn to a random point 500 away */
};

/* Group 2, EM_GROUP_FLY (EmChameleos_RunFlyState). */
enum EmChameleosFlyState {
    CHAM_FLY_TAKE_OFF = 0x00,               /* take off to hover */
    CHAM_FLY_LAND = 0x01,                   /* sink and land */
    CHAM_FLY_HOVER = 0x02,                  /* hover once */
    CHAM_FLY_NONE_A = 0x03,                 /* empty (EndAirState 2) */
    CHAM_FLY_NONE_B = 0x04,                 /* empty */
    CHAM_FLY_NONE_C = 0x05,                 /* empty */
    CHAM_FLY_ROUTE = 0x06,                  /* four legs between area points */
    CHAM_FLY_HOVER_TO_GOAL = 0x07,          /* hover to the goal turning */
    CHAM_FLY_OUT_OF_AREA = 0x08,            /* fly out to the next area */
    CHAM_FLY_INTO_AREA = 0x09,              /* fly to the area point and land */
    CHAM_FLY_NONE_D = 0x0A,                 /* empty */
    CHAM_FLY_TO_GOAL_AND_LAND = 0x0B,       /* fly to the goal and land */
    CHAM_FLY_NONE_E = 0x0C,                 /* empty */
    CHAM_FLY_THROUGH_AREA = 0x0D,           /* cross an area on the way */
    CHAM_FLY_ASCEND = 0x0E,                 /* rise to the area's height */
    CHAM_FLY_FORWARD = 0x0F,                /* fly straight on once */
    CHAM_FLY_TURN_TO_GOAL = 0x10,           /* hover turning to the goal */
    CHAM_FLY_ROUTE_LOOP = 0x11,             /* endless flight between area points */
    CHAM_FLY_BRAKE_AND_LAND = 0x12,         /* brake from fast flight, then land */
    CHAM_FLY_TO_GOAL = 0x13,                /* fly to the goal */
    CHAM_FLY_NONE_F = 0x14,                 /* empty */
    CHAM_FLY_STRAFE_TO_GOAL = 0x15,         /* hover straight at the goal without turning */
    CHAM_FLY_BRAKE = 0x16,                  /* brake to a hover */
    CHAM_FLY_NONE_G = 0x17,                 /* empty */
    CHAM_FLY_RISE = 0x18                    /* rise 1000 above the goal */
};

/* Group 3, EM_GROUP_ATTACK (EmChameleos_RunAttackState). */
enum EmChameleosAttackState {
    CHAM_ATK_TONGUE = 0x00,                 /* tail thumps then tongue lash */
    CHAM_ATK_TONGUE_LOW = 0x01,             /* tongue at a target on its level */
    CHAM_ATK_STOMP = 0x02,                  /* ChameleosAttack 3, ground effects at bone 0x1F */
    CHAM_ATK_TAIL_SLAM = 0x03,              /* backward slam with impact */
    CHAM_ATK_TONGUE_B = 0x04,               /* tail thumps then the second tongue lash */
    CHAM_ATK_TONGUE_HIGH = 0x05,            /* tongue at a raised target */
    CHAM_ATK_BALL_PITCHED = 0x06,           /* poison ball, pitched */
    CHAM_ATK_BALL_LEVEL = 0x07,             /* poison ball, level */
    CHAM_ATK_BACK_HOP_BALL = 0x08,          /* leap back into the air with a ball */
    CHAM_ATK_BREATH_PITCHED = 0x09,         /* mist breath, pitched */
    CHAM_ATK_BREATH_LEVEL = 0x0A,           /* mist breath, level */
    CHAM_ATK_TONGUE_QUICK = 0x0B,           /* tongue lash alone */
    CHAM_ATK_TAIL_POUNDS = 0x0C             /* tail thumps alone */
};

/* Group 4, EM_GROUP_REACT (EmChameleos_RunReactState). */
enum EmChameleosReactState {
    CHAM_REACT_FLINCH_A = 0x00,             /* flinch of part 0 or 6 */
    CHAM_REACT_FLINCH_B = 0x01,             /* flinch of parts 1 to 5 */
    CHAM_REACT_STAGGER = 0x02,              /* broken part's flinch */
    CHAM_REACT_KNOCK_DOWN = 0x03,           /* broken part 3/4's flinch, lies */
    CHAM_REACT_PART_BREAK = 0x04,           /* horn broken */
    CHAM_REACT_STUN = 0x05,                 /* stunned (EM_DMG_STATUS5) */
    CHAM_REACT_NONE_A = 0x06,               /* empty */
    CHAM_REACT_NONE_B = 0x07,               /* empty */
    CHAM_REACT_FALL = 0x08,                 /* falls out of the air */
    CHAM_REACT_FALL_BLIND = 0x09,           /* falls, blind */
    CHAM_REACT_FLINCH_C = 0x0A,             /* copy of FLINCH_B */
    CHAM_REACT_STATUS4 = 0x0B,              /* under status 4 */
    CHAM_REACT_DECLOAK = 0x0C,              /* attack-kind-5 hit while visible: cloaks, drops a mine point */
    CHAM_REACT_DECLOAK_FALL = 0x0D          /* the same in the air */
};

/* Group 5, EM_GROUP_DEFEAT (EmChameleos_RunDefeatState). */
enum EmChameleosDefeatState {
    CHAM_DEFEAT_DIE = 0x00,                 /* dies on the ground */
    CHAM_DEFEAT_NONE = 0x01,                /* empty */
    CHAM_DEFEAT_DIE_FALLING = 0x02,         /* dies in the air */
    CHAM_DEFEAT_REAPPEAR = 0x03             /* appears again */
};

/* Group 6, EM_GROUP_EVENT (EmChameleos_RunEventState). */
enum EmChameleosEventState {
    CHAM_EVENT_FOREST = 0x00,               /* Forest and Hills area 9 event walk */
    CHAM_EVENT_LEAVE = 0x01,                /* quest over: flies off */
    CHAM_EVENT_LEAVE_AIR = 0x02             /* quest over in the air: flies off */
};

#endif
