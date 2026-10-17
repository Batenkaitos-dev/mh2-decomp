#ifndef EM_KHEZU_H
#define EM_KHEZU_H

/*
 * The Khezu family's vocabulary (src/game/em_khezu.c): Khezu and Red Khezu
 * (EM_KIND_KHEZU, EM_KIND_RED_KHEZU) share one set of state functions and
 * one numbering of motions and states.
 *
 * Motions: the kinds' motion tables (gEmMotionRows[kind]) have row N play
 * motion N, so one number serves Em_SetMotion's row and the motion id
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions no state explains stay numbers.
 */
enum EmKhezuMotion {
    KHEZU_MOT_IDLE = 0x01,              /* the standing loop; layers 0-2 idle with EM_MOTION_ID(layer, 1) */
    KHEZU_MOT_WALK = 0x03,              /* Em_StepToGoal walking */
    KHEZU_MOT_TURN_RIGHT = 0x05,        /* EmKhezu_TurnParams[0] (KHEZU_MOVE_TURN): motRight */
    KHEZU_MOT_TURN_LEFT = 0x06,         /* their motLeft */
    KHEZU_MOT_TURN_LEFT_ALERT = 0x07,  /* EmKhezu_TurnParams[1] (KHEZU_MOVE_TURN_ALERT): motLeft */
    KHEZU_MOT_TURN_RIGHT_ALERT = 0x08, /* its motRight */
    KHEZU_MOT_LIMP = 0x0A,              /* walking with low hp */
    KHEZU_MOT_DESCEND = 0x0B,           /* the last stretch of a landing, down to the floor */
    KHEZU_MOT_HOVER = 0x0F,             /* hovering and flying to a goal; wing-beat dust at frames 8, 86 and 160 */
    KHEZU_MOT_TAKE_OFF = 0x12,          /* airborne (stance 2) from frame 30 or 38 */
    KHEZU_MOT_TOUCH_DOWN = 0x13,        /* on the floor again (stance 0), with a camera shake and the landing dust */
    KHEZU_MOT_IDLE_ACTION_D = 0x14,     /* KHEZU_IDLE_ACTION_D's motion (layer 1) */
    KHEZU_MOT_IDLE_ACTION_B = 0x18,     /* KHEZU_IDLE_ACTION_B's motion (layer 1) */
    KHEZU_MOT_REST = 0x1F,              /* lying at rest while hp comes back */
    KHEZU_MOT_REST_START = 0x20,        /* lying down to rest */
    KHEZU_MOT_REST_END = 0x21,          /* getting up from the rest */
    KHEZU_MOT_LIE = 0x22,               /* lying under status 1 or 2 (a periodic effect at bone 0x22) */
    KHEZU_MOT_ATK_ACTION_B = 0x23,      /* KHEZU_ATK_ACTION_B's motion: one effect at frame 40 */
    KHEZU_MOT_GET_UP = 0x25,            /* after status 1 or 2 */
    KHEZU_MOT_COLLAPSE = 0x26,          /* falling over when status 1 or 2 starts */
    KHEZU_MOT_LIE_DOWN = 0x27,          /* lying down under status 2 (KHEZU_IDLE_STATUS2_LIE_DOWN) */
    KHEZU_MOT_SPIN = 0x2B,              /* spinning in place, rotY falling 0x200 a frame; camera shakes at frames 44 and 73 */
    KHEZU_MOT_SHOT_FAN = 0x2F,          /* three shots (EmRangedAttack kind 6) from the head at frame 176, sparks at 58 and 172 */
    KHEZU_MOT_ALERT_IDLE = 0x33,        /* standing alert, in combat */
    KHEZU_MOT_TURN_IN_PLACE = 0x36,     /* the turns to the goal, rotY stepped by hand until frame 60 */
    KHEZU_MOT_EAT = 0x3B,               /* eating, after KHEZU_MOT_EAT_START */
    KHEZU_MOT_FLINCH = 0x3C,
    KHEZU_MOT_SHAKE_OFF = 0x3E,         /* shaking off a status, the stun or a hold */
    KHEZU_MOT_FLINCH_B = 0x3F,
    KHEZU_MOT_FLINCH_C = 0x40,
    KHEZU_MOT_DIE = 0x44,
    KHEZU_MOT_KNOCK_DOWN_A = 0x45,      /* knocked down on one side (an impact object with flag 0x80 at frame 26) */
    KHEZU_MOT_DOWN_A = 0x46,            /* lying after KHEZU_MOT_KNOCK_DOWN_A */
    KHEZU_MOT_GET_UP_A = 0x47,          /* getting up after KHEZU_MOT_DOWN_A or a fall */
    KHEZU_MOT_GET_UP_B = 0x48,          /* getting up after KHEZU_MOT_DOWN_B */
    KHEZU_MOT_KNOCK_DOWN_B = 0x4A,      /* knocked down on the other side (flag 0x81) */
    KHEZU_MOT_DOWN_B = 0x4B,
    KHEZU_MOT_STATUS4 = 0x4C,           /* under status 4; the second hold's lying */
    KHEZU_MOT_FALL = 0x4D,              /* falling out of the air */
    KHEZU_MOT_FALL_LAND = 0x4E,         /* hitting the floor after the fall */
    KHEZU_MOT_EAT_START = 0x4F,         /* lowering the head to eat (bite effects at frames 48, 106 and 174) */
    KHEZU_MOT_DIE_FALLING = 0x52,       /* dying after a fall from the air */
    KHEZU_MOT_HOP_SLAM = 0x54,          /* a hop, airborne from frame 44, landing with a camera shake */
    KHEZU_MOT_TRAPPED = 0x5F,
    KHEZU_MOT_STRUGGLE = 0x60,          /* replayed until Em.trapTimer runs out */
    KHEZU_MOT_DOWNED = 0x61,            /* lying in a hold: statuses, the capture, death in a hold */
    KHEZU_MOT_DOWNED_GET_UP = 0x62,
    KHEZU_MOT_IDLE_LOOP = 0x64,         /* KHEZU_IDLE_LOOP's motion (layer 1) */
    KHEZU_MOT_IDLE_LOOP_START = 0x65,   /* KHEZU_IDLE_LOOP_START's motion (layer 1) */
    KHEZU_MOT_IDLE_ACTION_A = 0x66,     /* KHEZU_IDLE_ACTION_A's motion (layer 1) */
    KHEZU_MOT_CEIL_HANG = 0x67,         /* hanging from the ceiling */
    KHEZU_MOT_CEIL_LEAP = 0x68,         /* the leap from the floor to the ceiling, launched at frame 112 */
    KHEZU_MOT_IDLE_ACTION_C = 0x69,     /* KHEZU_IDLE_ACTION_C's motion (layer 1); also a versus pose and the story scene's */
    KHEZU_MOT_LEAP = 0x6A,              /* a leap at the target (a hit at frame 70) */
    KHEZU_MOT_LEAP_LAND = 0x6B,         /* landing from a leap */
    KHEZU_MOT_LEAP_QUICK = 0x6C,        /* the leap with a short wind-up */
    KHEZU_MOT_CEIL_DROP = 0x6D,         /* letting go of the ceiling; the body is moved down at frame 12 */
    KHEZU_MOT_CEIL_SHOTS = 0x6E,        /* on the ceiling: a shot (EmRangedAttack) from the head every 16 frames from frame 200 */
    KHEZU_MOT_CEIL_SHOTS_END = 0x6F,    /* after the mine point is left below */
    KHEZU_MOT_ATK_ACTION_A = 0x70,      /* KHEZU_ATK_ACTION_A's motion: impact effects at frames 172-180 */
    KHEZU_MOT_ATK_ACTION_C = 0x71,      /* KHEZU_ATK_ACTION_C's motion: Red Khezu adds sparks from frame 120 to 180 */
    KHEZU_MOT_ATK_ACTION_D = 0x72,      /* KHEZU_ATK_ACTION_D's motion: sparks at frame 2, a hit at 114; also the story scene's and a versus pose */
    KHEZU_MOT_ATK_RECOVER = 0x73,       /* follows KHEZU_MOT_ATK_ACTION_D and the shot fan */
    KHEZU_MOT_CEIL_DROP_LAND = 0x75,    /* landing after a drop from the ceiling */
    KHEZU_MOT_CEIL_LATCH = 0x76,        /* taking hold of the ceiling after the leap */
    KHEZU_MOT_CEIL_CRAWL = 0x77,        /* crawling on the ceiling; the ceiling turn rows' motFront */
    KHEZU_MOT_CEIL_TURN_RIGHT = 0x78,   /* the ceiling turn rows' motRight (EmKhezu_TurnParams[2..5]) */
    KHEZU_MOT_CEIL_TURN_LEFT = 0x79     /* their motLeft */
};

/* Group 0, EM_GROUP_IDLE (EmKhezu_RunIdleState). */
enum EmKhezuIdleState {
    KHEZU_IDLE_STAND = 0x00,            /* the idle motion for 30 frames */
    KHEZU_IDLE = 0x01,                  /* EmKhezu_EndState's state: the script picks the next; KHEZU_IDLE_ALERT in combat */
    KHEZU_IDLE_LOOP_START = 0x02,
    KHEZU_IDLE_LOOP = 0x03,             /* restarts itself and never ends on its own */
    KHEZU_IDLE_ACTION_A = 0x04,
    KHEZU_IDLE_ACTION_B = 0x05,
    KHEZU_IDLE_ACTION_C = 0x06,
    KHEZU_IDLE_WAIT = 0x07,             /* 30 frames; KHEZU_IDLE_WAIT_ALERT in combat */
    KHEZU_IDLE_WAIT_ALERT = 0x08,
    KHEZU_IDLE_TURN_TO_GOAL = 0x0A,
    KHEZU_IDLE_ACTION_D = 0x0D,
    KHEZU_IDLE_EAT_NEED2 = 0x0E,        /* raises the need-2 gauge as it eats */
    KHEZU_IDLE_ALERT = 0x11,
    KHEZU_IDLE_REST = 0x12,             /* lies and regains hp (entered by the script) */
    KHEZU_IDLE_EAT_START = 0x13,        /* KHEZU_IDLE_EAT follows at frame 200 */
    KHEZU_IDLE_STATUS2_COLLAPSE = 0x14, /* status 2 outside a hold (EM_DMG_STATUS2) */
    KHEZU_IDLE_STATUS2_LIE_DOWN = 0x15, /* status 2 with its own timer, regaining hp (entered by the script) */
    KHEZU_IDLE_STATUS4_SHAKE_OFF = 0x16,
    KHEZU_IDLE_REST_END = 0x17,
    KHEZU_IDLE_STATUS2_END = 0x18,
    KHEZU_IDLE_SHAKE_OFF = 0x19,
    KHEZU_IDLE_EAT_NEED1 = 0x1A,        /* fills need 1 to its maximum */
    KHEZU_IDLE_STATUS1_COLLAPSE = 0x1B, /* status 1 outside a hold (EM_DMG_CAPTURED without the hold) */
    KHEZU_IDLE_STATUS1_END = 0x1C,
    KHEZU_IDLE_STATUS2_DOWNED = 0x1D,   /* status 2 in a hold */
    KHEZU_IDLE_STATUS1_DOWNED = 0x1F,   /* status 1 in a hold */
    KHEZU_IDLE_HOLD2_SHAKE_OFF = 0x21,
    KHEZU_IDLE_STUN_SHAKE_OFF = 0x22,
    KHEZU_IDLE_EAT = 0x28,              /* after the eating start (Em_StartGoalAction) */
    KHEZU_IDLE_ROAR_RAGE = 0x29,        /* the alert idle, with Em_StartRage on entry */
    KHEZU_IDLE_ROAR_PENDING_RAGE = 0x2A /* the alert idle, with Em_StartPendingRage on entry */
};

/* Group 1, EM_GROUP_MOVE (EmKhezu_RunMoveState). */
enum EmKhezuMoveState {
    KHEZU_MOVE_WALK = 0x00,             /* KHEZU_MOVE_LIMP at low hp */
    KHEZU_MOVE_TURN = 0x03,             /* KHEZU_MOVE_TURN_ALERT in combat */
    KHEZU_MOVE_TURN_ALERT = 0x05,
    KHEZU_MOVE_LIMP = 0x06,
    KHEZU_MOVE_TURN_TO_GOAL = 0x07
};

/*
 * Group 2, EM_GROUP_FLY (EmKhezu_RunFlyState): flight (0x00-0x17) and the
 * ceiling (0x18-0x22: the states keep Em.ceilingTimer at 5 and the body at
 * Em.surfaceY, the ceiling's height).
 */
enum EmKhezuFlyState {
    KHEZU_FLY_TAKE_OFF = 0x00,
    KHEZU_FLY_LAND = 0x01,              /* down to the floor; KHEZU_DEFEAT_REAPPEAR flies in this way */
    KHEZU_FLY_HOVER = 0x02,
    KHEZU_FLY_HOP_TURN = 0x03,          /* up and down on the spot, turning to the goal */
    KHEZU_FLY_HOP_FORWARD = 0x04,
    KHEZU_FLY_HOP_BACK = 0x05,
    KHEZU_FLY_TO_GOAL = 0x06,
    KHEZU_FLY_TO_GOAL_PITCHED = 0x07,
    KHEZU_FLY_OUT_OF_AREA = 0x08,
    KHEZU_FLY_TO_GOAL_AND_LAND = 0x09,
    KHEZU_FLY_TO_GOAL_AND_LAND_B = 0x0B,
    KHEZU_FLY_RISE_TO_AREA_HEIGHT = 0x0C,
    KHEZU_FLY_INTO_AREA = 0x0D,
    KHEZU_FLY_HOVER_ONCE = 0x0E,        /* EmKhezu_EndFlyState's state: the script picks the next */
    KHEZU_FLY_RISE_ABOVE_GOAL = 0x0F,
    KHEZU_FLY_HOVER_TURN = 0x10,
    KHEZU_FLY_EMPTY_A = 0x11,           /* 0x11, 0x12, 0x13 and 0x16 run empty functions */
    KHEZU_FLY_EMPTY_B = 0x12,
    KHEZU_FLY_EMPTY_C = 0x13,
    KHEZU_FLY_HOP_SLAM = 0x14,
    KHEZU_FLY_TO_GOAL_DIRECT = 0x15,
    KHEZU_FLY_EMPTY_D = 0x16,
    KHEZU_FLY_ESCAPE_TRAP = 0x17,       /* after the trap's struggle */
    KHEZU_FLY_CEIL_LEAP_UP = 0x18,      /* KHEZU_IDLE_WAIT where there is no ceiling */
    KHEZU_FLY_CEIL_HANG = 0x19,         /* EmKhezu_EndCeilingState's state: the script picks the next */
    KHEZU_FLY_CEIL_DROP = 0x1A,
    KHEZU_FLY_CEIL_TURN = 0x1B,
    KHEZU_FLY_CEIL_CRAWL = 0x1C,
    KHEZU_FLY_CEIL_CIRCUIT = 0x1D,      /* crawls round the area's four ceiling points (EmKhezuWork.legsLeft legs) */
    KHEZU_FLY_CEIL_CIRCUIT_REVERSE = 0x1E,
    KHEZU_FLY_CEIL_TURN_TO_CIRCUIT = 0x1F,
    KHEZU_FLY_CEIL_CRAWL_TO_ZONE = 0x20, /* crawls to the ceiling point of the target's zone */
    KHEZU_FLY_CEIL_TURN_TO_ZONE = 0x21,
    KHEZU_FLY_CEIL_TURN_TO_CRAWL = 0x22
};

/* Group 3, EM_GROUP_ATTACK (EmKhezu_RunAttackState): picked by the script. */
enum EmKhezuAttackState {
    KHEZU_ATK_LEAP = 0x00,
    KHEZU_ATK_CEIL_SHOTS = 0x01,        /* shots from the ceiling, then a mine point left on the floor below */
    KHEZU_ATK_ACTION_A = 0x02,
    KHEZU_ATK_ACTION_B = 0x03,
    KHEZU_ATK_ACTION_C = 0x04,
    KHEZU_ATK_SHOT_FAN = 0x05,          /* three shots, the side ones wide or narrow by Em.stateRand */
    KHEZU_ATK_SPIN = 0x06,
    KHEZU_ATK_ACTION_D = 0x07,
    KHEZU_ATK_LEAP_QUICK = 0x08
};

/* Group 4, EM_GROUP_REACT (EmKhezu_RunReactState): what EmKhezu_Update picks from Em_UpdateDamage's result. */
enum EmKhezuReactState {
    KHEZU_REACT_FLINCH = 0x00,          /* a flinch of parts 0 and 7 (EM_DMG_PART_FLINCH) on the ground, or EM_DMG_HIT_MODE3 */
    KHEZU_REACT_CEIL_FALL = 0x01,       /* a flinch or a status on the ceiling */
    KHEZU_REACT_FLINCH_B = 0x02,        /* parts 5 and 6 */
    KHEZU_REACT_FLINCH_C = 0x03,        /* parts 1 and 2 */
    KHEZU_REACT_KNOCK_DOWN_PART3 = 0x04,
    KHEZU_REACT_KNOCK_DOWN = 0x05,      /* part 4 or any part not named above */
    KHEZU_REACT_FALL = 0x08,            /* a flinch or a status in flight */
    KHEZU_REACT_STATUS4_DOWNED_END = 0x0A,
    KHEZU_REACT_STATUS4 = 0x0B,         /* status 4 outside a hold */
    KHEZU_REACT_TRAPPED = 0x0C,         /* EM_DMG_ATTACK9 and EM_DMG_ATTACK6 outside a hold */
    KHEZU_REACT_TRAPPED_STRUGGLE = 0x0D,
    KHEZU_REACT_STATUS4_DOWNED = 0x0E,  /* status 4 in a hold */
    KHEZU_REACT_STATUS2_DOWNED_END = 0x0F,
    KHEZU_REACT_STATUS1_DOWNED_END = 0x10,
    KHEZU_REACT_HOLD2 = 0x11,           /* EM_DMG_ATTACK13 */
    KHEZU_REACT_STUN = 0x12,            /* the stun, hit from one side ... */
    KHEZU_REACT_STUN_B = 0x13,          /* ... or the other (the hit's angle against rotY) */
    KHEZU_REACT_STUN_DOWNED = 0x14,     /* the stun in a hold */
    KHEZU_REACT_STUN_DOWNED_END = 0x15
};

/* Group 5, EM_GROUP_DEFEAT (EmKhezu_RunDefeatState). */
enum EmKhezuDefeatState {
    KHEZU_DEFEAT_DIE = 0x00,
    KHEZU_DEFEAT_DIE_HELD = 0x01,
    KHEZU_DEFEAT_DIE_FALLING = 0x02,
    KHEZU_DEFEAT_REAPPEAR = 0x03        /* EmKhezu_ChangeState calls QuestEm_Appear when the quest lets it */
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmKhezu_RunEventState). */
enum EmKhezuEventState {
    KHEZU_EVENT_STORY = 0x00,           /* the story scene of quest stage 0x422 */
    KHEZU_EVENT_CAPTURED = 0x04,        /* status 1 in holdTimer's hold (0x01-0x03 run the same function) */
    KHEZU_EVENT_CAPTURED_HOLD2 = 0x05   /* status 1 in holdTimer2's hold */
};

/* Group 8, EM_GROUP_VERSUS (EmKhezu_RunVersusState). */
enum EmKhezuVersusState {
    KHEZU_VS_EAT_BAIT = 0x00,
    KHEZU_VS_POSE_A = 0x01,
    KHEZU_VS_POSE_B = 0x02,
    KHEZU_VS_POSE_C = 0x03,
    KHEZU_VS_WAIT_BOTH = 0x98,
    KHEZU_VS_END = 0x99
};

/* A position (EmKhezu_BaitThrowPt), copied as one struct (the original's copy). */
typedef struct EmKhezuVec {
    f32 x, y, z;
} EmKhezuVec;

#endif /* EM_KHEZU_H */
