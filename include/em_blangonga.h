#ifndef EM_BLANGONGA_H
#define EM_BLANGONGA_H

/*
 * The Blangonga family's vocabulary (src/game/em_blangonga.c): Blangonga,
 * Congalala and Rajang, and the small Blango and Conga, share one set of
 * state functions and one numbering of motions and states.
 *
 * Motions: each kind's motion table (gEmMotionRows[kind]; Blangonga, Rajang
 * and Blango share one, Congalala and Conga another) has row N play motion
 * N, so one number serves Em_SetMotion's row and the motion id
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. A number is the same
 * motion for the kinds that share a state; where kind-specific states play
 * the same number as different motions, each use has its own name (the
 * aliases below). Motions no state explains stay numbers.
 */
enum EmBlangongaMotion {
    BLANG_MOT_IDLE = 0x01,              /* the standing loop (BLANG_IDLE) */
    BLANG_MOT_WALK = 0x03,              /* Em_StepToGoal walking, the approaches */
    BLANG_MOT_BURIED = 0x04,            /* under the snow (Blangonga kinds' buried states; the Blango's hidden start) */
    BLANG_MOT_TURN_LEFT = 0x05,         /* the slow turn (EmBlangonga_TurnData.turn[0]) */
    BLANG_MOT_TURN_RIGHT = 0x06,
    BLANG_MOT_TURN_LEFT_QUICK = 0x07,   /* the quick turn (turn[1]: frames 2-30 instead of 2-50) */
    BLANG_MOT_TURN_RIGHT_QUICK = 0x08,
    BLANG_MOT_LIMP = 0x09,              /* walking with low hp (ChangeState picks it) */
    BLANG_MOT_MOVE_ACTION_D = 0x0A,     /* BLANG_MOVE_ACTION_D: a hit (BlangongaAttack 0xB) at frame 10; Blangonga's scene jump off its ledge */
    BLANG_MOT_MOVE_ACTION_A = 0x0C,     /* BLANG_MOVE_ACTION_A: a hit (BlangongaAttack 0xC) at frame 18 */
    BLANG_MOT_MOVE_ACTION_B = 0x0E,     /* BLANG_MOVE_ACTION_B: a hit (BlangongaAttack 0xD) at frame 18 */
    BLANG_MOT_MOVE_ACTION_C = 0x10,     /* BLANG_MOVE_ACTION_C: footstep effects only */
    BLANG_MOT_JUMP = 0x11,              /* crouch and take-off (airborne at frame 52, 56 or 48 by kind) */
    BLANG_MOT_JUMP_FALL = 0x12,         /* falling after a jump's apex */
    BLANG_MOT_CHARGE = 0x13,            /* running at the goal with a hit along (BLANG_ATK_CHARGE, _CONGALALA_CHARGE) */
    BLANG_MOT_JUMP_LAND = 0x14,         /* landing, with a camera shake */
    BLANG_MOT_DIG_IN = 0x15,            /* Blangonga digging into the snow (the shadow goes from frame 80) */
    BLANG_MOT_EMERGE = 0x16,            /* coming up out of the snow or a trap's pit, with a camera shake */
    BLANG_MOT_WALK_START = 0x17,        /* the start of a walk; BLANG_MOT_WALK follows from frame 68 */
    BLANG_MOT_LIE = 0x1F,               /* lying: resting or under status 1 or 2 */
    BLANG_MOT_GET_UP = 0x20,
    BLANG_MOT_COLLAPSE = 0x21,          /* falling over under status 1 or 2, or captured in the second hold */
    BLANG_MOT_LIE_DOWN = 0x22,          /* lying down to rest */
    BLANG_MOT_TRIPLE_HIT = 0x28,        /* Blangonga kinds: three hits (BLANG_ATK_TRIPLE_HIT) */
    BLANG_MOT_CONGALALA_EATEN_EMISSION = 0x28, /* Congalala: BLANG_ATK_CONGALALA_EATEN_EMISSION */
    BLANG_MOT_TRIPLE_HIT_END = 0x29,
    BLANG_MOT_CONGALALA_COMBO = 0x29,   /* Congalala: BLANG_ATK_CONGALALA_COMBO */
    BLANG_MOT_SLAM_WINDUP = 0x2A,       /* held 20 or 40 frames before BLANG_MOT_SLAM */
    BLANG_MOT_STRIKE_A = 0x2B,          /* also Rajang's combo slam's first blow */
    BLANG_MOT_STRIKE_B = 0x2C,          /* also Rajang's combo slam's slam */
    BLANG_MOT_CONGALALA_EMISSION = 0x2D, /* BLANG_ATK_CONGALALA_EMISSION */
    BLANG_MOT_THROW = 0x2E,             /* the Blango's and Congalala's throws */
    BLANG_MOT_DIG_THROW = 0x2F,         /* Blangonga: digs up and throws a lump (BLANG_ATK_DIG_THROW_*) */
    BLANG_MOT_CHARGE_END = 0x30,        /* Blangonga and Rajang: the end of BLANG_ATK_CHARGE */
    BLANG_MOT_CONGALALA_HOP = 0x30,     /* Congalala: the hop's take-off (airborne at frame 26) */
    BLANG_MOT_HOP_LAND = 0x31,          /* Congalala: the hop's landing */
    BLANG_MOT_ALERT_IDLE = 0x33,
    BLANG_MOT_ROAR = 0x34,              /* the roars (BLANG_IDLE_ROAR*): the call, the rage's start */
    BLANG_MOT_TURN_ABOUT = 0x35,        /* half a turn over frames 102-202, back over 402-452 */
    BLANG_MOT_TURN_ABOUT_B = 0x36,      /* the same, the other way */
    BLANG_MOT_EAT_NEED2 = 0x37,         /* a 300-frame loop */
    BLANG_MOT_EAT_NEED2_END = 0x38,
    BLANG_MOT_EAT = 0x39,               /* eating: Congalala holds the food in frames 36-382 (EmBlangonga_EatPropMode) */
    BLANG_MOT_EAT_END = 0x3A,
    BLANG_MOT_ROAR_B = 0x3B,            /* Blangonga's roar in BLANG_IDLE_ROAR_B */
    BLANG_MOT_FLINCH_B = 0x3C,
    BLANG_MOT_FLINCH = 0x3D,
    BLANG_MOT_SHAKE_OFF = 0x3E,         /* after status 4, the stun and the second hold */
    BLANG_MOT_DOWN = 0x3F,              /* lying after BLANG_MOT_KNOCK_DOWN or a fall */
    BLANG_MOT_DOWN_GET_UP = 0x40,
    BLANG_MOT_STATUS4 = 0x41,           /* under status 4, and in the second hold */
    BLANG_MOT_FALL = 0x42,              /* falling out of the air (curve 0) */
    BLANG_MOT_FALL_LAND = 0x43,
    BLANG_MOT_DIE = 0x44,
    BLANG_MOT_KNOCK_DOWN = 0x45,
    BLANG_MOT_KNOCK_DOWN_B = 0x46,      /* from the other side */
    BLANG_MOT_DIE_END = 0x47,           /* the large kinds, after BLANG_MOT_DIE */
    BLANG_MOT_TRAPPED = 0x49,           /* caught (holdTimer 50) */
    BLANG_MOT_STRUGGLE = 0x4A,          /* struggling in a trap */
    BLANG_MOT_DOWNED = 0x4B,            /* lying in a hold: a status, a capture or a death */
    BLANG_MOT_DOWN_B = 0x4D,            /* lying after BLANG_MOT_KNOCK_DOWN_B */
    BLANG_MOT_DOWN_GET_UP_B = 0x4E,
    BLANG_MOT_DROP_CARRIED = 0x4F,      /* Congalala dropping a carried item (BLANG_REACT_DROP_CARRIED*) */
    BLANG_MOT_EFFECT_STRIKE = 0x50,     /* Blangonga: BLANG_ATK_EFFECT_STRIKE */
    BLANG_MOT_CONGALALA_COMBO_END = 0x50, /* Congalala: after BLANG_MOT_CONGALALA_COMBO */
    BLANG_MOT_RAJANG_AIM = 0x50,        /* Rajang: the beam's and the shot's pose */
    BLANG_MOT_EFFECT_STRIKE_END = 0x51,
    BLANG_MOT_CONGALALA_HITZONE_COMBO = 0x51, /* Congalala: BLANG_ATK_CONGALALA_HITZONE_COMBO's first part (also BLANG_REACT_FLINCH_ATTACK) */
    BLANG_MOT_RAJANG_AIM_END = 0x51,
    BLANG_MOT_CONGALALA_HITZONE_COMBO_END = 0x52,
    BLANG_MOT_RAJANG_JUMP = 0x52,       /* Rajang: the jump shot's take-off (airborne at frame 24) */
    BLANG_MOT_RAJANG_JUMP_SHOT = 0x53,  /* Rajang: the shot from the air */
    BLANG_MOT_CONGALALA_CLIMB_OUT = 0x53, /* Congalala: leaving a trap (BLANG_IDLE_LEAVE_TRAP) */
    BLANG_MOT_RAJANG_JUMP_SHOT_LAND = 0x54,
    BLANG_MOT_CONGALALA_EAT = 0x54,     /* Congalala: BLANG_IDLE_EAT_START_B, the versus bait */
    BLANG_MOT_BACK_STRIKE = 0x55,       /* hits behind (BLANG_ATK_BACK_STRIKE); also BLANG_IDLE_ACTION_A */
    BLANG_MOT_IDLE_ACTION_B = 0x56,     /* a hit (BlangongaAttack 0x22) at frame 128 */
    BLANG_MOT_RAJANG_STRIKE = 0x56,     /* Rajang: BLANG_ATK_RAJANG_STRIKE */
    BLANG_MOT_IDLE_ACTION_B_END = 0x57, /* also BLANG_IDLE_CLEAR_PAINT's motion */
    BLANG_MOT_RAJANG_CHARGE = 0x57,     /* Rajang: BLANG_ATK_RAJANG_CHARGE */
    BLANG_MOT_SLAM = 0x58,              /* after BLANG_MOT_SLAM_WINDUP */
    BLANG_MOT_IDLE_ACTION_C = 0x59,     /* also Blangonga's first versus pose */
    BLANG_MOT_RAJANG_CHARGE_END = 0x5A
};

/* Group 0, EM_GROUP_IDLE (EmBlangonga_RunIdleState). */
enum EmBlangongaIdleState {
    BLANG_IDLE_NONE = 0x00,
    BLANG_IDLE = 0x01,                  /* in combat EmBlangonga_ChangeState makes it BLANG_IDLE_ALERT */
    BLANG_IDLE_REST = 0x03,             /* lies and regains hp */
    BLANG_IDLE_STATUS2_END = 0x04,
    BLANG_IDLE_STATUS2_COLLAPSE = 0x05, /* status 2 outside a hold (EM_DMG_STATUS2) */
    BLANG_IDLE_ALERT = 0x06,
    BLANG_IDLE_ROAR = 0x07,
    BLANG_IDLE_TURN_ABOUT = 0x08,
    BLANG_IDLE_TURN_ABOUT_B = 0x09,
    BLANG_IDLE_STATUS4_SHAKE_OFF = 0x0A,
    BLANG_IDLE_EAT_NEED2 = 0x0B,
    BLANG_IDLE_EAT_NEED1 = 0x0C,
    BLANG_IDLE_CLEAR_PAINT = 0x0D,      /* clears Em.painted when its motion ends */
    BLANG_IDLE_LEAVE_TRAP = 0x0E,       /* after BLANG_REACT_TRAPPED_STRUGGLE */
    BLANG_IDLE_EAT_START_B = 0x0F,
    BLANG_IDLE_ACTION_A = 0x10,
    BLANG_IDLE_ACTION_B = 0x11,
    BLANG_IDLE_ALERT_BRIEF = 0x12,      /* instead of BLANG_LEAP_DIG_IN where the floor isn't snow (ScriptQuery 1) */
    BLANG_IDLE_STATUS2_DOWNED = 0x13,   /* status 2 in a hold */
    BLANG_IDLE_ROAR_CALL = 0x14,        /* adds the sensed object other monsters react to */
    BLANG_IDLE_ROAR_RAISE_RAGE = 0x15,  /* Em_CheckRageByHp at frame 50 */
    BLANG_IDLE_ROAR_B = 0x16,
    BLANG_IDLE_ACTION_C = 0x17,
    BLANG_IDLE_BURIED = 0x18,           /* hidden under the snow until the script moves on (the Blango's hidden start) */
    BLANG_IDLE_STUN_SHAKE_OFF = 0x19,
    BLANG_IDLE_REST_END = 0x1A,
    BLANG_IDLE_HOLD2_SHAKE_OFF = 0x1B,
    BLANG_IDLE_EAT_KIND_A = 0x1C,       /* eats, and EmBlangongaWork.eatenKind becomes 1 (_B 2, _C 3, _D 4) */
    BLANG_IDLE_EAT_KIND_B = 0x1D,
    BLANG_IDLE_EAT_KIND_C = 0x1E,
    BLANG_IDLE_EAT_KIND_D = 0x1F,
    BLANG_IDLE_LEAVE_MINE_POINT = 0x20, /* leaves a timed mine point where it stands */
    BLANG_IDLE_EAT_NEED1_B = 0x21,
    BLANG_IDLE_EAT = 0x22,              /* after the eating start (Em_StartGoalAction) */
    BLANG_IDLE_ROAR_SIGNAL = 0x23,      /* ScriptQuery 4 asks whether a Blangonga nearby is in it */
    BLANG_IDLE_STATUS1_DOWNED = 0x24,
    BLANG_IDLE_STATUS1_COLLAPSE = 0x25, /* status 1 outside a hold (EM_DMG_CAPTURED without the hold) */
    BLANG_IDLE_STATUS1_END = 0x26,
    BLANG_IDLE_EAT_START = 0x27,
    BLANG_IDLE_ROAR_RAGE = 0x28,        /* Em_StartRage on entry */
    BLANG_IDLE_ROAR_PENDING_RAGE = 0x29 /* Em_StartPendingRage on entry */
};

/* Group 1, EM_GROUP_MOVE (EmBlangonga_RunMoveState). */
enum EmBlangongaMoveState {
    BLANG_MOVE_TURN = 0x00,
    BLANG_MOVE_WALK = 0x01,             /* BLANG_MOVE_LIMP at low hp */
    BLANG_MOVE_LIMP = 0x02,
    BLANG_MOVE_TURN_QUICK = 0x03,
    BLANG_MOVE_ACTION_A = 0x04,         /* ACTION_A..D count in EmBlangongaWork.moveCount */
    BLANG_MOVE_ACTION_B = 0x05,
    BLANG_MOVE_ACTION_C = 0x06,
    BLANG_MOVE_ACTION_D = 0x07,
    BLANG_MOVE_TURN_TO_EXIT = 0x08,     /* turns to the area's exit, then BLANG_LEAP_OUT_OF_AREA */
    BLANG_MOVE_TURN_AWAY = 0x09,
    BLANG_MOVE_TURN_AWAY_QUICK = 0x0A,
    BLANG_MOVE_START_AND_APPROACH = 0x0B, /* BLANG_MOVE_LIMP_APPROACH at low hp */
    BLANG_MOVE_LIMP_APPROACH = 0x0C,
    BLANG_MOVE_APPROACH = 0x0D,         /* BLANG_MOVE_LIMP_APPROACH at low hp */
    BLANG_MOVE_START_AND_WALK = 0x0E,   /* BLANG_MOVE_LIMP at low hp */
    BLANG_MOVE_ACTION_E = 0x0F          /* runs BLANG_MOVE_ACTION_D's function; BLANG_MOVE_LIMP at low hp */
};

/*
 * Group 2, EM_GROUP_FLY (EmBlangonga_RunLeapState): the family doesn't fly;
 * the group holds its jumps and hops, the leaps out of an area and into
 * the next, and the Blangonga kinds' burrowing under the snow.
 */
enum EmBlangongaLeapState {
    BLANG_LEAP_JUMP = 0x00,
    BLANG_LEAP_HOP_SHORT = 0x01,        /* the Congalala kinds' hops (_SHORT, plain, _LONG) */
    BLANG_LEAP_DIG_IN = 0x02,           /* EmBlangonga_ChangeState makes it BLANG_IDLE_ALERT_BRIEF off the snow */
    BLANG_LEAP_BURIED = 0x03,           /* the buried states return here (EmBlangonga_EndBuriedState) */
    BLANG_LEAP_DIG_OUT = 0x04,
    BLANG_LEAP_BURIED_TURN = 0x05,
    BLANG_LEAP_BURIED_MOVE = 0x06,
    BLANG_LEAP_BURIED_WAIT = 0x07,
    BLANG_LEAP_OUT_OF_AREA = 0x08,      /* leaps up and out, then BLANG_LEAP_DROP_IN or _CROSS_AREA */
    BLANG_LEAP_DROP_IN = 0x09,          /* drops into the area from above */
    BLANG_LEAP_DIG_OUT_RAGE = 0x0A,     /* BLANG_LEAP_DIG_OUT with Em_CheckRageByHp */
    BLANG_LEAP_CROSS_AREA = 0x0B,       /* unseen, to the next area's exit */
    BLANG_LEAP_JUMP_HIGH = 0x0C,
    BLANG_LEAP_BURIED_WAIT_RANDOM = 0x0D,
    BLANG_LEAP_HOP = 0x0E,
    BLANG_LEAP_HOP_LONG = 0x0F,
    BLANG_LEAP_JUMP_TO_GOAL = 0x10      /* lands on the goal in 60 frames */
};

/* Group 3, EM_GROUP_ATTACK (EmBlangonga_RunAttackState); the kinds that use each in its function's comment. */
enum EmBlangongaAttackState {
    BLANG_ATK_TRIPLE_HIT = 0x00,
    BLANG_ATK_SLAM = 0x01,
    BLANG_ATK_STRIKE_A = 0x02,
    BLANG_ATK_STRIKE_B = 0x03,
    BLANG_ATK_CONGALALA_EMISSION = 0x04,
    BLANG_ATK_BLANGO_THROW = 0x05,
    BLANG_ATK_CHARGE = 0x06,
    BLANG_ATK_DIG_THROW_A = 0x07,
    BLANG_ATK_CONGALALA_EATEN_EMISSION = 0x08, /* by what it ate (EmBlangongaWork.eatenKind) */
    BLANG_ATK_CONGALALA_COMBO = 0x09,
    BLANG_ATK_CONGALALA_HITZONE_COMBO = 0x0A, /* the second damage table (Em.hitzoneSet 1) during its first part */
    BLANG_ATK_CONGALALA_THROW_A = 0x0B,
    BLANG_ATK_BACK_STRIKE = 0x0C,
    BLANG_ATK_SLAM_HEAVY = 0x0D,
    BLANG_ATK_EFFECT_STRIKE = 0x0E,
    BLANG_ATK_DIG_THROW_B = 0x0F,
    BLANG_ATK_DIG_THROW_C = 0x10,
    BLANG_ATK_RAJANG_COMBO_SLAM_A = 0x11,
    BLANG_ATK_RAJANG_SHOT_A = 0x12,
    BLANG_ATK_RAJANG_BEAM_A = 0x13,
    BLANG_ATK_RAJANG_JUMP_SHOT = 0x14,
    BLANG_ATK_RAJANG_STRIKE = 0x15,
    BLANG_ATK_RAJANG_CHARGE = 0x16,
    BLANG_ATK_CONGALALA_CHARGE = 0x17,
    BLANG_ATK_RAJANG_BEAM_B = 0x18,
    BLANG_ATK_CONGALALA_THROW_B = 0x19,
    BLANG_ATK_RAJANG_COMBO_SLAM_B = 0x1A,
    BLANG_ATK_RAJANG_SHOT_B = 0x1B
};

/* Group 4, EM_GROUP_REACT (EmBlangonga_RunReactState): what EmBlangonga_Update picks from Em_UpdateDamage's result. */
enum EmBlangongaReactState {
    BLANG_REACT_FLINCH = 0x00,          /* a flinch while the parts are little damaged, or a Blango hit on the ground */
    BLANG_REACT_FLINCH_B = 0x01,
    BLANG_REACT_KNOCK_DOWN = 0x02,      /* a flinch of part 2 */
    BLANG_REACT_KNOCK_DOWN_B = 0x03,    /* a flinch of part 3 (Congalala kinds) or 1 (the others) */
    BLANG_REACT_STATUS4 = 0x04,         /* status 4 outside a hold */
    BLANG_REACT_TRAPPED = 0x05,         /* EM_DMG_ATTACK9 and EM_DMG_ATTACK6 outside a hold */
    BLANG_REACT_TRAPPED_STRUGGLE = 0x06,
    BLANG_REACT_FALL = 0x07,            /* EM_DMG_STATUS_IN_AIR, or a flinch in the air */
    BLANG_REACT_NONE = 0x08,            /* does nothing; not entered in this file */
    BLANG_REACT_STATUS4_DOWNED = 0x09,  /* status 4 in a hold */
    BLANG_REACT_STATUS4_DOWNED_END = 0x0A,
    BLANG_REACT_STATUS2_DOWNED_END = 0x0B,
    BLANG_REACT_STUN = 0x0C,            /* hit from the one side */
    BLANG_REACT_STUN_B = 0x0D,          /* from the other */
    BLANG_REACT_STUN_DOWNED = 0x0E,     /* the stun in a hold */
    BLANG_REACT_STUN_DOWNED_END = 0x0F,
    BLANG_REACT_HOLD2 = 0x10,           /* EM_DMG_ATTACK13 */
    BLANG_REACT_DROP_CARRIED = 0x11,    /* drops nothing (no mine point); not entered in this file */
    BLANG_REACT_FLINCH_ATTACK = 0x12,   /* Congalala: a flinch of part 0 that turns into an attack */
    BLANG_REACT_STATUS1_DOWNED_END = 0x13,
    BLANG_REACT_PART_BREAK = 0x14,      /* EM_DMG_PART8_DOWN, once (Em.partBreakCount) */
    BLANG_REACT_DROP_CARRIED_A = 0x15,  /* Congalala drops carried item 0 as a mine point (_B item 1, _C item 2) */
    BLANG_REACT_DROP_CARRIED_B = 0x16,
    BLANG_REACT_DROP_CARRIED_C = 0x17,
    BLANG_REACT_TRAP_ESCAPE = 0x18      /* Rajang, calm and without a status, leaps out of a trap */
};

/* Group 5, EM_GROUP_DEFEAT (EmBlangonga_RunDefeatState). */
enum EmBlangongaDefeatState {
    BLANG_DEFEAT_DIE = 0x00,
    BLANG_DEFEAT_DIE_HELD = 0x01,
    BLANG_DEFEAT_DIE_ARMORED = 0x02,    /* while Em.stateArmor (buried) */
    BLANG_DEFEAT_DIE_FALLING = 0x03,
    BLANG_DEFEAT_REAPPEAR = 0x04        /* EmBlangonga_ChangeState calls QuestEm_Appear when the quest lets it */
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmBlangonga_RunEventState). */
enum EmBlangongaEventState {
    BLANG_EVENT_CAPTURED = 0x00,        /* status 1 in holdTimer's hold */
    BLANG_EVENT_CAPTURED_HOLD2 = 0x01,  /* status 1 in holdTimer2's hold */
    BLANG_EVENT_CONGALALA_SCENE = 0x02, /* Congalala's story scene (two quests, until event flag 0x9B) */
    BLANG_EVENT_BLANGONGA_SCENE = 0x03, /* Blangonga's story scene (two quests, until event flag 0xA4): hands each monster its part */
    BLANG_EVENT_BLANGONGA_LEAP_DOWN = 0x04,
    BLANG_EVENT_BLANGO_EMERGE_A = 0x05, /* the first three Blangos (_A, _B, _C), by their order in gEms */
    BLANG_EVENT_BLANGO_EMERGE_B = 0x06,
    BLANG_EVENT_BLANGO_EMERGE_C = 0x07,
    BLANG_EVENT_BLANGO_HIDDEN = 0x08    /* the other Blangos */
};

/* Group 8, EM_GROUP_VERSUS (EmBlangonga_RunVersusState). */
enum EmBlangongaVersusState {
    BLANG_VS_EAT_BAIT = 0x00,
    BLANG_VS_POSE_A = 0x01,
    BLANG_VS_POSE_B = 0x02,
    BLANG_VS_POSE_C = 0x03,
    BLANG_VS_WAIT_BOTH = 0x98,
    BLANG_VS_END = 0x99
};

/* The questions the monster script asks the family (EmBlangonga_ScriptQuery's `query`). */
enum EmBlangongaQuery {
    BLANG_QUERY_MOVE_COUNT = 0,         /* whether EmBlangongaWork.moveCount reached 4 */
    BLANG_QUERY_ON_SNOW = 1,            /* whether it stands on snow (floor attribute 1 in the snowy areas) in the player's area */
    BLANG_QUERY_ARMOR = 2,              /* Em.stateArmor */
    BLANG_QUERY_EATEN_KIND = 3,         /* EmBlangongaWork.eatenKind */
    BLANG_QUERY_ROAR_SIGNAL = 4,        /* whether a visible Blangonga in its area is in BLANG_IDLE_ROAR_SIGNAL */
    BLANG_QUERY_DEST_AREA = 5,          /* the destination area's index among the family's twelve, 0xFF if none */
    BLANG_QUERY_CARRIED = 6             /* the first carried item shown (item 0: 1, item 2: 2, item 1: 3; none: 0) */
};

/* A row of a weight table (EmBlangonga_PickCarried): the rows end with weight 0xFFFF. */
typedef struct EmBlangongaPickRow {
    u16 weight;
    u16 value;
} EmBlangongaPickRow;

/* The family's data object: the turn parameters of the slow and the quick turn, and the two maps' weight tables of the carried items. */
typedef struct EmBlangongaTurnData {
    EmTurnParams turn[2];
    EmBlangongaPickRow pick0[4];        /* map 6 (Map_DayId) */
    EmBlangongaPickRow pick1[4];        /* map 4 */
} EmBlangongaTurnData;

#endif /* EM_BLANGONGA_H */
