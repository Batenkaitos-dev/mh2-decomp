#ifndef EM_FATALIS_H
#define EM_FATALIS_H

/*
 * The Fatalis family's vocabulary (src/game/em_rathalos.c, EmFatalis_*; its
 * Set*State, ChangeState and curve functions are in src/game/em_bulldrome.c).
 * Kinds EM_KIND_FATALIS, EM_KIND_FATALIS_B and EM_KIND_FATALIS_C share one
 * set of states; the first has a motion table of its own, the other two
 * share one.
 *
 * Motions: both motion tables have row N play motion N, so one number serves
 * Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion) kept in
 * Em.motionId. The names say what the states that play a motion show;
 * motions no state explains stay numbers (0x12, 0x13, 0x1E, 0x29).
 */
enum EmFatalisMotion {
    FAT_MOT_IDLE = 0x01,                /* the standing loop; layers 0-2 idle with EM_MOTION_ID(layer, 1) */
    FAT_MOT_WALK = 0x02,                /* Em_StepToGoal walking */
    FAT_MOT_TURN_LEFT = 0x03,           /* turning to the goal, rotY falling 372 a frame up to frame 89 */
    FAT_MOT_TURN_RIGHT = 0x04,          /* turning to the goal, rotY rising 372 a frame up to frame 89 */
    FAT_MOT_CHARGE_START = 0x05,        /* before a charge */
    FAT_MOT_TURN_SMALL = 0x06,          /* turning to a goal less than 0x2000 away, 91 a frame */
    FAT_MOT_CHARGE = 0x07,              /* running at the goal (Em_StepToGoal), with FatalisAttack 2 */
    FAT_MOT_RECOVER = 0x08,             /* back to standing after a landing, an attack, a status or a reaction */
    FAT_MOT_TAKE_OFF = 0x09,            /* airborne (stance 2) from frame 134, along curve 0 or 6 */
    FAT_MOT_FLY = 0x0A,                 /* flying and hovering; layers 0-2 hover with EM_MOTION_ID(layer, 0xA) */
    FAT_MOT_DESCEND = 0x0B,             /* the last stretch of a landing, down to the floor */
    FAT_MOT_TOUCH_DOWN = 0x0C,          /* on the floor again (stance 0), with a camera shake */
    FAT_MOT_FIREBALL = 0x0D,            /* a fireball on the ground at frame 102 (EmFatalis_SpawnFireball) */
    FAT_MOT_FIREBALL_B = 0x0E,          /* a fireball on the ground at frame 98 */
    FAT_MOT_SWEEP = 0x0F,               /* five effects at frames 80 to 320 (EmFatalis_SpawnSweepEffect) */
    FAT_MOT_AIR_FIREBALL = 0x10,        /* a fireball from the hover at frame 30 */
    FAT_MOT_FLINCH = 0x14,
    FAT_MOT_FLINCH_B = 0x15,
    FAT_MOT_FLINCH_TURN_RIGHT = 0x16,   /* rotY rising 0x28E a frame up to frame 50 */
    FAT_MOT_FLINCH_TURN_LEFT = 0x17,    /* rotY falling 0x28E a frame up to frame 50 */
    FAT_MOT_DIE = 0x18,                 /* falling dead (QuestEm_Hunted) */
    FAT_MOT_FALL = 0x19,                /* falling out of the air */
    FAT_MOT_FALL_LAND = 0x1A,           /* hitting the floor after FAT_MOT_FALL */
    FAT_MOT_DOWN = 0x1B,                /* lying knocked down (120 frames), or under status 4 */
    FAT_MOT_DOWN_GET_UP = 0x1C,         /* getting up from FAT_MOT_DOWN or FAT_MOT_DIE_B's pose */
    FAT_MOT_DIE_B = 0x1D,               /* falling dead after an attack of kind 9 (Em.lastAttackKind) */
    FAT_MOT_AIR_FIREBALL_AIM = 0x1F,    /* hovering 90 frames, turning to the goal, before FAT_MOT_AIR_FIREBALL */
    FAT_MOT_ROAR = 0x20,                /* the scenes start the rage with it (Em.rage 1, rageTimer 1800) */
    FAT_MOT_HOP_FIREBALL = 0x21,        /* airborne from frame 70, a fireball at frame 94 */
    FAT_MOT_LIE_DOWN = 0x22,            /* collapsing to lie under status 2 or 4 (lying from frame 296) */
    FAT_MOT_GLIDE_START = 0x23,         /* on the floor, before FAT_MOT_GLIDE */
    FAT_MOT_LAND_SLIDE = 0x24,          /* back on the floor after a glide or a dive */
    FAT_MOT_DIVE_RISE = 0x25,           /* rising and pitching up 0x1A8 a frame before a dive */
    FAT_MOT_DIVE = 0x26,                /* diving at the floor, velZ 80 and accelZ 9 */
    FAT_MOT_DIVE_PULL_UP = 0x27,        /* pulling out of the dive, pitch falling 0x6A2 a frame */
    FAT_MOT_GLIDE = 0x28                /* flying low to the goal along curve 4 (speed only) */
};

/* Group 0, EM_GROUP_IDLE (EmFatalis_RunIdleState). */
enum EmFatalisIdleState {
    FAT_IDLE_STAND = 0x00,              /* the idle loop on every layer until the timer ends */
    FAT_IDLE = 0x01,                    /* EmFatalis_SetIdleState picks FAT_IDLE or FAT_IDLE_ACTION at random for either */
    FAT_IDLE_ACTION = 0x02,
    FAT_IDLE_ROAR = 0x03,
    FAT_IDLE_STATUS2 = 0x04,            /* lying under status 2 (EM_DMG_STATUS2) until EM_DMG_HIT_LYING */
    FAT_IDLE_STATUS2_END = 0x05,
    FAT_IDLE_STATUS4_END = 0x06         /* after FAT_REACT_STATUS4 */
};

/* Group 1, EM_GROUP_MOVE (EmFatalis_RunMoveState). */
enum EmFatalisMoveState {
    FAT_MOVE_WALK = 0x00,
    FAT_MOVE_TURN_TO_TARGET = 0x01,
    FAT_MOVE_TURN_TO_GOAL = 0x02
};

/* Group 2, EM_GROUP_FLY (EmFatalis_RunFlyState). */
enum EmFatalisFlyState {
    FAT_FLY_TAKE_OFF = 0x00,
    FAT_FLY_LAND = 0x01,
    FAT_FLY_HOVER = 0x02,
    FAT_FLY_HOVER_TURN = 0x03,
    FAT_FLY_TO_GOAL = 0x04,
    FAT_FLY_RISE = 0x05,
    FAT_FLY_STRAIGHT_TO_GOAL = 0x06,
    FAT_FLY_DESCEND = 0x07,
    FAT_FLY_RISE_HIGH = 0x08,
    FAT_FLY_LEVEL_TO_GOAL = 0x09,
    FAT_FLY_CLIMB_TO_GOAL = 0x0A,
    FAT_FLY_TAKE_OFF_AND_RISE = 0x0B,
    FAT_FLY_CLIMB_TO_GOAL_NEAR = 0x0C,  /* FAT_FLY_CLIMB_TO_GOAL, ending 10 frames' flight from the goal instead of 100 */
    FAT_FLY_HOVER_TURN_STOP = 0x0D,     /* FAT_FLY_HOVER_TURN, stopping first */
    FAT_FLY_LAND_AT_HEIGHT = 0x0E,      /* lands at the height 3584, then FAT_ATK_BLASTS */
    FAT_FLY_TAKE_OFF_HIGH = 0x0F        /* FAT_FLY_TAKE_OFF counted airborne from the start (after FAT_ATK_BLASTS) */
};

/* Group 3, EM_GROUP_ATTACK (EmFatalis_RunAttackState). */
enum EmFatalisAttackState {
    FAT_ATK_SINGLE_A = 0x00,            /* motion 0x12 */
    FAT_ATK_SINGLE_B = 0x01,            /* motion 0x13 */
    FAT_ATK_FIREBALL = 0x02,
    FAT_ATK_FIREBALL_B = 0x03,
    FAT_ATK_SWEEP_NO_EFFECT = 0x04,     /* the sweep's motion with no effects */
    FAT_ATK_AIR_FIREBALL = 0x05,        /* the fireball's pitch 0x38E4 */
    FAT_ATK_CHARGE = 0x06,
    FAT_ATK_HOP_FIREBALL = 0x07,
    FAT_ATK_CHARGE_START = 0x08,        /* the charge's first motion only */
    FAT_ATK_AIR_FIREBALL_TRIPLE = 0x09, /* three fireballs, pitched 0x1E94, 0x2AAB and 0x38E4 */
    FAT_ATK_AIR_FIREBALL_B = 0x0A,      /* pitch 0x2AAB */
    FAT_ATK_AIR_FIREBALL_C = 0x0B,      /* pitch 0x1E94 */
    FAT_ATK_GLIDE = 0x0C,
    FAT_ATK_DIVE = 0x0D,                /* pulls out 850 above the floor */
    FAT_ATK_SWEEP_A = 0x0E,             /* the sweeps A-D take rows 1-4 of EmFatalis_SweepSpots */
    FAT_ATK_SWEEP_B = 0x0F,
    FAT_ATK_SWEEP_C = 0x10,
    FAT_ATK_SWEEP_D = 0x11,
    FAT_ATK_SWEEP_DOUBLE = 0x12,        /* two sweeps along row 5 */
    FAT_ATK_SINGLE_C = 0x13,            /* FAT_ATK_SINGLE_A's state with motion 0x29 */
    FAT_ATK_DIVE_B = 0x14,              /* pulls out at the height 1050 */
    FAT_ATK_BLASTS = 0x15               /* after FAT_FLY_LAND_AT_HEIGHT: 24 effects along row 6 or 7 */
};

/* Group 4, EM_GROUP_REACT (EmFatalis_RunReactState): what EmFatalis_Update picks from Em_UpdateDamage's result. */
enum EmFatalisReactState {
    FAT_REACT_FLINCH = 0x00,            /* a flinch of parts 0, 1, 4 and 5 (EM_DMG_PART_FLINCH), or EM_DMG_HIT_MODE3 */
    FAT_REACT_FLINCH_B = 0x01,          /* a flinch of part 6 */
    FAT_REACT_FLINCH_TURN_RIGHT = 0x02, /* a flinch of the other parts but 3 */
    FAT_REACT_FALL = 0x03,              /* EM_DMG_STATUS_IN_AIR, or a flinch in the air */
    FAT_REACT_KNOCK_DOWN = 0x04,        /* EM_DMG_ATTACK9 and EM_DMG_PART8_DOWN */
    FAT_REACT_STATUS4 = 0x05,           /* EM_DMG_STATUS4 */
    FAT_REACT_FLINCH_TURN_LEFT = 0x06,  /* a flinch of part 3 */
    FAT_REACT_FLINCH_C = 0x07,
    FAT_REACT_FALL_B = 0x08
};

/* Group 5, EM_GROUP_DEFEAT (EmFatalis_RunDefeatState): EM_DMG_KILLED. */
enum EmFatalisDefeatState {
    FAT_DEFEAT_DIE = 0x00,
    FAT_DEFEAT_DIE_B = 0x01,            /* the last hit an attack of kind 9 (Em.lastAttackKind) */
    FAT_DEFEAT_DIE_FALLING = 0x02       /* in the air */
};

/*
 * Group 6 (and 7), EM_GROUP_EVENT (EmFatalis_RunEventState): EmFatalis_Update
 * starts them in three areas (by Area_DayId), each kind in its own, while an
 * event runs and until the kind's event flag is set.
 */
enum EmFatalisEventState {
    FAT_EVENT_LEAVE = 0x00,             /* EM_KIND_FATALIS in AREA_CASTLE_1 once the quest's goal is met: flies up and away */
    FAT_EVENT_FLY_IN = 0x01,            /* EM_KIND_FATALIS's scene in AREA_CASTLE_1 (EVENT_FLAG_FATALIS_CASTLE_SCENE) */
    FAT_EVENT_LEAVE_FROM_AIR = 0x02,    /* FAT_EVENT_LEAVE from the hover */
    FAT_EVENT_GET_UP = 0x03,            /* EM_KIND_FATALIS_B's scene in AREA_BATTLEFIELD_1 (EVENT_FLAG_FATALIS_BATTLEFIELD_SCENE) */
    FAT_EVENT_DESCEND = 0x04            /* EM_KIND_FATALIS_C's scene in AREA_TOWER_0x7C (EVENT_FLAG_FATALIS_TOWER_SCENE) */
};

#endif /* EM_FATALIS_H */
