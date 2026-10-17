#ifndef EM_RATHALOS_H
#define EM_RATHALOS_H

/*
 * The Rathalos family's vocabulary (src/game/em_rathalos.c): Rathian,
 * Rathalos and their subspecies share one motion table and one set of
 * states.
 *
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and the motion id
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions no state explains stay numbers.
 */
enum EmRathalosMotion {
    RATH_MOT_IDLE = 0x01,               /* the standing loop; layers 0-2 idle with EM_MOTION_ID(layer, 1) */
    RATH_MOT_WALK = 0x03,               /* Em_StepToGoal walking; Em_TurnToTargetMotion's forward motion */
    RATH_MOT_TURN_RIGHT = 0x05,         /* Em_TurnToTargetMotion's motRight */
    RATH_MOT_TURN_LEFT = 0x06,          /* Em_TurnToTargetMotion's motLeft */
    RATH_MOT_RUN_STOP = 0x09,           /* after a run to the goal (EmRathalos_RunAndStop) */
    RATH_MOT_LIMP = 0x0A,               /* walking with low hp (EmRathalos_SetMoveState) */
    RATH_MOT_DESCEND = 0x0B,            /* the last stretch of a landing, down to the floor */
    RATH_MOT_GLIDE = 0x0C,              /* flying forward (Em_UpdateFlight), entering a flight */
    RATH_MOT_FLY = 0x0E,                /* flying forward, after the glide motion ends */
    RATH_MOT_HOVER = 0x0F,              /* hovering in place */
    RATH_MOT_RUN = 0x11,                /* running to the goal (a charge in group 3) */
    RATH_MOT_TAKE_OFF = 0x12,           /* airborne (stance 2) from frame 30 or 38 */
    RATH_MOT_TOUCH_DOWN = 0x13,         /* on the floor again (stance 0), with a camera shake */
    RATH_MOT_LIE = 0x22,                /* lying under a status or resting (a periodic effect at bone 0x22) */
    RATH_MOT_ROAR = 0x23,               /* with attached effect 1 (RathalosAttack_Create); the quest's opening landing ends with it */
    RATH_MOT_GET_UP = 0x25,             /* after resting or a status */
    RATH_MOT_COLLAPSE = 0x26,           /* falling over when a status starts */
    RATH_MOT_LIE_DOWN = 0x27,           /* lying down to rest */
    RATH_MOT_LEAP = 0x28,               /* a leap at the target, airborne from frame 60 */
    RATH_MOT_LEAP_LAND = 0x29,          /* landing from a leap or a swoop */
    RATH_MOT_TAIL_SPIN_B = 0x2A,        /* spinning in place, rotY rising 0x200 a frame */
    RATH_MOT_TAIL_SPIN = 0x2B,          /* spinning in place, rotY falling 0x200 a frame */
    RATH_MOT_SWOOP_DIVE = 0x2C,         /* a swoop's start, aimed at the target */
    RATH_MOT_SWOOP_CLAW = 0x2D,         /* the swoop's pass over the target (follows RATH_MOT_SWOOP) */
    RATH_MOT_AIR_FIREBALL = 0x2E,       /* a fireball while hovering: two projectiles (stage object kind 0x11) from bone 0x24 at frame 6 */
    RATH_MOT_FIREBALL = 0x2F,           /* a fireball on the ground: two projectiles from bone 0x24 at frame 78 */
    RATH_MOT_AIR_FIREBALL_RISE = 0x31,  /* rising before RATH_MOT_AIR_FIREBALL */
    RATH_MOT_FIREBALL_B = 0x32,         /* a second ground fireball, projectiles at frame 78 */
    RATH_MOT_ALERT_IDLE = 0x33,         /* standing while it fights the hunters (EmRathalos_SetIdleState) */
    RATH_MOT_BACK_HOP = 0x34,           /* a hop backward, airborne from frame 76 */
    RATH_MOT_AIR_DIVE = 0x35,           /* a lunge at the target from the hover (EmRathalos_AirDive) */
    RATH_MOT_TURN_IN_PLACE = 0x36,      /* turning to the goal 0x200 a frame */
    RATH_MOT_BACK_HOP_LAND = 0x39,      /* landing from a back hop */
    RATH_MOT_FEED = 0x3B,               /* eating (after RATH_MOT_HEAD_DOWN); hp and a need gauge refilled */
    RATH_MOT_SHAKE_OFF = 0x3E,          /* after getting up from a status */
    RATH_MOT_DIE = 0x44,                /* falling dead (QuestEm_Hunted) */
    RATH_MOT_KNOCK_DOWN = 0x45,         /* knocked down (a broken part's flinch, the stun) */
    RATH_MOT_DOWN = 0x46,               /* lying knocked down */
    RATH_MOT_DOWN_GET_UP = 0x47,        /* getting up from a knock-down or a fall */
    RATH_MOT_FALL = 0x4D,               /* falling out of the air */
    RATH_MOT_FALL_LAND = 0x4E,          /* hitting the floor after RATH_MOT_FALL */
    RATH_MOT_HEAD_DOWN = 0x4F,          /* lowering the head to eat */
    RATH_MOT_DIE_FALLING = 0x52,        /* dead after falling out of the air */
    RATH_MOT_SWOOP = 0x57,              /* the swoop's descent, after RATH_MOT_SWOOP_DIVE */
    RATH_MOT_LEVEL_OUT = 0x5A,          /* levelling pitch and roll in the air */
    RATH_MOT_CHARGE_TRIP = 0x5B,        /* the end of a charge, with attached effect 0x24 */
    RATH_MOT_FLIP = 0x5C,               /* a somersault, going up (the Rathian kinds' versus pose) */
    RATH_MOT_FLIP_FALL = 0x5D,          /* the somersault's descent */
    RATH_MOT_FLIP_LAND = 0x5E,          /* the somersault's landing */
    RATH_MOT_TRAPPED = 0x5F,            /* caught (reaction 3 or 4: holdTimer 50) */
    RATH_MOT_STRUGGLE = 0x60,           /* struggling before it flies off (EmRathalos_EscapeUpward) */
    RATH_MOT_DOWNED = 0x61,             /* lying downed under a status, a capture or a death in a hold */
    RATH_MOT_DOWNED_GET_UP = 0x62       /* getting up from RATH_MOT_DOWNED */
};

/* Group 0, EM_GROUP_IDLE (EmRathalos_RunIdleState). */
enum EmRathalosIdleState {
    RATH_IDLE_STAND = 0x00,
    RATH_IDLE = 0x01,                   /* in combat EmRathalos_SetIdleState makes it RATH_IDLE_ALERT */
    RATH_IDLE_ACTION_A = 0x02,
    RATH_IDLE_ACTION_B = 0x03,
    RATH_IDLE_ACTION_C = 0x04,
    RATH_IDLE_ACTION_D = 0x05,
    RATH_IDLE_ACTION_E = 0x06,
    RATH_IDLE_ACTION_F = 0x07,
    RATH_IDLE_ACTION_G = 0x08,
    RATH_IDLE_PAIR_A = 0x09,
    RATH_IDLE_TURN_TO_GOAL = 0x0A,
    RATH_IDLE_ACTION_H = 0x0B,
    RATH_IDLE_ACTION_I = 0x0C,
    RATH_IDLE_ACTION_J = 0x0D,
    RATH_IDLE_EAT_NEED2 = 0x0E,
    RATH_IDLE_PAIR_B = 0x0F,
    RATH_IDLE_PAIR_C = 0x10,
    RATH_IDLE_ALERT = 0x11,
    RATH_IDLE_REST = 0x12,
    RATH_IDLE_EAT_START = 0x13,
    RATH_IDLE_STATUS2_COLLAPSE = 0x14,
    RATH_IDLE_STATUS2_LIE_DOWN = 0x15,
    RATH_IDLE_ACTION_K = 0x16,
    RATH_IDLE_REST_END = 0x17,
    RATH_IDLE_STATUS2_END = 0x18,
    RATH_IDLE_SHAKE_OFF = 0x19,
    RATH_IDLE_EAT_NEED1 = 0x1A,
    RATH_IDLE_STATUS1_COLLAPSE = 0x1B,
    RATH_IDLE_STATUS1_END = 0x1C,
    RATH_IDLE_STATUS2_DOWNED = 0x1D,
    RATH_IDLE_STATUS1_DOWNED = 0x1F,
    RATH_IDLE_STATUS4_SHAKE_OFF = 0x21,
    RATH_IDLE_STUN_SHAKE_OFF = 0x22,
    RATH_IDLE_HOLD2_SHAKE_OFF = 0x23,
    RATH_IDLE_EAT = 0x28
};

/* Group 1, EM_GROUP_MOVE (EmRathalos_RunMoveState). */
enum EmRathalosMoveState {
    RATH_MOVE_WALK = 0x00,
    RATH_MOVE_END = 0x01,               /* EmRathalos_SetMoveState only asks the script for the next state */
    RATH_MOVE_END2 = 0x02,              /* becomes RATH_MOVE_WALK, or RATH_MOVE_RUN in combat */
    RATH_MOVE_TURN = 0x03,              /* becomes RATH_MOVE_TURN_IN_COMBAT in combat */
    RATH_MOVE_RUN = 0x04,
    RATH_MOVE_TURN_IN_COMBAT = 0x05,
    RATH_MOVE_LIMP = 0x06,              /* chosen for the walks below 10% hp (Rathian kinds) or 30% */
    RATH_MOVE_TURN_TO_GOAL = 0x07,
    RATH_MOVE_RUN_SLOW = 0x08,          /* the run at 0.7 speed */
    RATH_MOVE_TURN_RAGE = 0x09,         /* the turn to the goal, starting the rage (Em_StartRage) */
    RATH_MOVE_TURN_PENDING_RAGE = 0x0A  /* the turn to the goal, starting a pending rage */
};

/* Group 2, EM_GROUP_FLY (EmRathalos_RunFlyState). */
enum EmRathalosFlyState {
    RATH_FLY_TAKE_OFF = 0x00,
    RATH_FLY_LAND = 0x01,
    RATH_FLY_HOVER = 0x02,
    RATH_FLY_HOP_TO_GOAL = 0x03,
    RATH_FLY_HOP_FORWARD = 0x04,
    RATH_FLY_HOP_BACK = 0x05,
    RATH_FLY_ROUTE = 0x06,
    RATH_FLY_HOVER_TO_GOAL = 0x07,
    RATH_FLY_OUT_OF_AREA = 0x08,
    RATH_FLY_TO_GOAL_AND_LAND = 0x09,
    RATH_FLY_OUT_OF_AREA_ALT = 0x0A,
    RATH_FLY_GLIDE_TO_GOAL = 0x0B,
    RATH_FLY_BACK_HOP = 0x0C,
    RATH_FLY_INTO_AREA = 0x0D,
    RATH_FLY_HOVER_ONCE = 0x0E,
    RATH_FLY_GLIDE_ONCE = 0x0F,
    RATH_FLY_HOVER_TURN = 0x10,
    RATH_FLY_ROUTE_B = 0x11,
    RATH_FLY_LEVEL_OUT_AND_LAND = 0x12,
    RATH_FLY_GLIDE_TO_GOAL_B = 0x13,
    RATH_FLY_END = 0x14,
    RATH_FLY_HOVER_STRAIGHT_TO_GOAL = 0x15,
    RATH_FLY_LEVEL_OUT = 0x16,
    RATH_FLY_ESCAPE_UPWARD = 0x17,
    RATH_FLY_HOVER_RISE = 0x18
};

/* Group 3, EM_GROUP_ATTACK (EmRathalos_RunAttackState). */
enum EmRathalosAttackState {
    RATH_ATK_TURN_AND_STARE = 0x00,
    RATH_ATK_TURN_AND_STARE_B = 0x01,   /* set up by EmRathalos_SetAttackState; the dispatcher has no case for it */
    RATH_ATK_LEAP = 0x02,
    RATH_ATK_STARE = 0x03,
    RATH_ATK_FIREBALL = 0x04,
    RATH_ATK_ROAR = 0x05,
    RATH_ATK_TAIL_SPIN = 0x06,
    RATH_ATK_LIE_AND_RISE = 0x07,
    RATH_ATK_SWOOP_THEN_LEAVE = 0x08,
    RATH_ATK_AIR_FIREBALL = 0x09,
    RATH_ATK_AIR_FIREBALL_B = 0x0A,
    RATH_ATK_AIR_DIVE = 0x0B,
    RATH_ATK_ROAR_AT_POINT = 0x0C,
    RATH_ATK_SINGLE_A = 0x0D,
    RATH_ATK_SINGLE_B = 0x0E,
    RATH_ATK_CHARGE = 0x0F,
    RATH_ATK_BACK_HOP_FIREBALL = 0x10,
    RATH_ATK_AIR_FIREBALL_DOUBLE = 0x11,
    RATH_ATK_CHARGE_TRIP = 0x12,
    RATH_ATK_ROAR_B = 0x13,
    RATH_ATK_SOMERSAULT = 0x14,
    RATH_ATK_SWOOP_THEN_LAND = 0x15,
    RATH_ATK_AIR_FIREBALL_DOUBLE_B = 0x16,
    RATH_ATK_FIREBALL_B = 0x17,
    RATH_ATK_FIREBALL_LOW = 0x18,       /* EmRathalos_SetAttackState picks LOW, FIREBALL or HIGH by the target's height */
    RATH_ATK_FIREBALL_HIGH = 0x19,
    RATH_ATK_TAIL_SPIN_B = 0x1A,
    RATH_ATK_RUN_AND_STOP = 0x1B,
    RATH_ATK_AIR_FIREBALL_FORWARD = 0x1C,
    RATH_ATK_AIR_FIREBALL_DOUBLE_FORWARD = 0x1D,
    RATH_ATK_SOMERSAULT_DOUBLE = 0x1E,
    RATH_ATK_BACK_HOP_FIREBALL_LEFT = 0x1F, /* turning rotY + 0x666 a frame for its first 20 frames */
    RATH_ATK_BACK_HOP_FIREBALL_RIGHT = 0x20 /* turning rotY - 0x666 a frame */
};

/* Group 4, EM_GROUP_REACT (EmRathalos_RunReactState): what EmRathalos_Update picks from Em_UpdateDamage's result. */
enum EmRathalosReactState {
    RATH_REACT_FLINCH = 0x00,           /* a flinch of parts 0 and 7 (EM_DMG_PART_FLINCH), or EM_DMG_HIT_MODE3 */
    RATH_REACT_FLINCH_B = 0x01,         /* a flinch of an unbroken part 3 or 4 */
    RATH_REACT_FLINCH_C = 0x02,         /* a flinch of parts 5 and 6 */
    RATH_REACT_FLINCH_D = 0x03,         /* a flinch of parts 1 and 2 */
    RATH_REACT_PART_BREAK = 0x04,       /* EM_DMG_PART8_DOWN, once (Em.partBreakCount) */
    RATH_REACT_KNOCK_DOWN = 0x05,       /* a flinch of a broken part other than 3 */
    RATH_REACT_END_AIR = 0x06,
    RATH_REACT_END_AIR_B = 0x07,
    RATH_REACT_FALL = 0x08,             /* EM_DMG_STATUS_IN_AIR, or a flinch in the air */
    RATH_REACT_FALL_B = 0x09,
    RATH_REACT_FLINCH_E = 0x0A,
    RATH_REACT_STATUS4 = 0x0B,
    RATH_REACT_TRAPPED = 0x0C,          /* EM_DMG_ATTACK9 and EM_DMG_ATTACK6 outside a hold */
    RATH_REACT_TRAPPED_STRUGGLE = 0x0D,
    RATH_REACT_STATUS4_DOWNED = 0x0E,
    RATH_REACT_PART_BREAK_END = 0x0F,   /* EmRathalos_ChangeState breaks part 3 on entry */
    RATH_REACT_KNOCK_DOWN_PART3 = 0x10, /* a flinch of the broken part 3 */
    RATH_REACT_STATUS4_DOWNED_END = 0x11,
    RATH_REACT_STATUS2_DOWNED_END = 0x12,
    RATH_REACT_STATUS1_DOWNED_END = 0x13,
    RATH_REACT_STUN = 0x14,
    RATH_REACT_STUN_DOWNED = 0x15,
    RATH_REACT_STUN_DOWNED_END = 0x16,
    RATH_REACT_HOLD2 = 0x17             /* EM_DMG_ATTACK13 */
};

/* Group 5, EM_GROUP_DEFEAT (EmRathalos_RunDefeatState). */
enum EmRathalosDefeatState {
    RATH_DEFEAT_DIE = 0x00,
    RATH_DEFEAT_DIE_HELD = 0x01,
    RATH_DEFEAT_DIE_FALLING = 0x02,
    RATH_DEFEAT_REAPPEAR = 0x03         /* EmRathalos_ChangeState calls QuestEm_Appear when the quest lets it */
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmRathalos_RunEventState). */
enum EmRathalosEventState {
    RATH_EVENT_INTRO = 0x02,
    RATH_EVENT_CAPTURED = 0x04,         /* status 1 in holdTimer's hold */
    RATH_EVENT_CAPTURED_HOLD2 = 0x05,   /* status 1 in holdTimer2's hold */
    RATH_EVENT_STORY = 0x06
};

/* Group 8, EM_GROUP_VERSUS (EmRathalos_RunVersusState). */
enum EmRathalosVersusState {
    RATH_VS_EAT_BAIT = 0x00,
    RATH_VS_POSE_A = 0x01,
    RATH_VS_POSE_B = 0x02,
    RATH_VS_POSE_C = 0x03,
    RATH_VS_WAIT_BOTH = 0x98,
    RATH_VS_END = 0x99
};

#endif /* EM_RATHALOS_H */
