#ifndef EM_GARUGA_H
#define EM_GARUGA_H

/*
 * The Yian Garuga family (EM_KIND_YIAN_GARUGA): its states, by group (em.h's
 * EmStateGroup). The functions are EmGaruga_*. The family follows the
 * Rathalos template (the Rathalos family's states with the same numbers
 * mostly do the same), with Yian Garuga's own pecks, back flip and hop.
 * Each enum lists the cases of the group's dispatcher (Run*State); where
 * one state function serves several numbers, the names say which variant
 * the number asks for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. Most numbers are the
 * Rathalos family's motions, and the states that play them show the same
 * thing; the names say what those states show. Motions only the motion
 * sounds and effects name stay numbers.
 */
enum EmGarugaMotion {
    GARUGA_MOT_IDLE = 0x01,
    GARUGA_MOT_WALK = 0x03,             /* Em_StepToGoal walking; Em_TurnToTargetMotion's forward motion */
    GARUGA_MOT_TURN_RIGHT = 0x05,       /* Em_TurnToTargetMotion's motRight */
    GARUGA_MOT_TURN_LEFT = 0x06,        /* Em_TurnToTargetMotion's motLeft */
    GARUGA_MOT_LIMP = 0x0A,
    GARUGA_MOT_DESCEND = 0x0B,
    GARUGA_MOT_GLIDE = 0x0C,
    GARUGA_MOT_FLY = 0x0E,
    GARUGA_MOT_HOVER = 0x0F,
    GARUGA_MOT_RUN = 0x11,              /* running to the goal (a charge in group 3) */
    GARUGA_MOT_TAKE_OFF = 0x12,
    GARUGA_MOT_TOUCH_DOWN = 0x13,
    GARUGA_MOT_IDLE_ACTION_J = 0x14,    /* EmGaruga_IdleActionJ */
    GARUGA_MOT_VERSUS_EAT_START = 0x1A, /* EmGaruga_VersusEatBait's first motion, before the bait is thrown */
    GARUGA_MOT_IDLE_ACTION_H = 0x1C,    /* EmGaruga_IdleActionH */
    GARUGA_MOT_IDLE_ACTION_I = 0x1D,    /* EmGaruga_IdleActionI */
    GARUGA_MOT_LIE = 0x22,              /* lying under a status or resting */
    GARUGA_MOT_STARE = 0x24,
    GARUGA_MOT_GET_UP = 0x25,
    GARUGA_MOT_COLLAPSE = 0x26,         /* falling over when a status starts */
    GARUGA_MOT_LIE_DOWN = 0x27,
    GARUGA_MOT_SINGLE_ATTACK = 0x28,    /* the one motion of EmGaruga_SingleAttack */
    GARUGA_MOT_FLIP = 0x29,             /* the back flip (EmGaruga_Somersault), then FLIP_FALL and FLIP_LAND */
    GARUGA_MOT_TAIL_SPIN = 0x2A,
    GARUGA_MOT_FLIP_FALL = 0x2D,
    GARUGA_MOT_FIREBALL = 0x2F,
    GARUGA_MOT_ALERT_IDLE = 0x33,
    GARUGA_MOT_BACK_HOP = 0x34,
    GARUGA_MOT_TURN_IN_PLACE = 0x36,
    GARUGA_MOT_BACK_HOP_LAND = 0x39,
    GARUGA_MOT_FEED = 0x3B,
    GARUGA_MOT_FLINCH_A = 0x3C,
    GARUGA_MOT_FLINCH_E = 0x3D,
    GARUGA_MOT_SHAKE_OFF = 0x3E,        /* after getting up from a status */
    GARUGA_MOT_FLINCH_C = 0x3F,
    GARUGA_MOT_FLINCH_D = 0x40,
    GARUGA_MOT_PART_BREAK = 0x41,
    GARUGA_MOT_FLINCH_B = 0x42,
    GARUGA_MOT_DIE = 0x44,
    GARUGA_MOT_KNOCK_DOWN = 0x45,       /* knocked down (a broken part's flinch, the stun) */
    GARUGA_MOT_DOWN = 0x46,             /* lying knocked down */
    GARUGA_MOT_DOWN_GET_UP = 0x47,      /* getting up from a knock-down or a fall */
    GARUGA_MOT_KNOCK_DOWN_B_GET_UP = 0x48,
    GARUGA_MOT_KNOCK_DOWN_B = 0x4A,     /* EmGaruga_KnockDown plays 0x4A, 0x4B, then 0x48 */
    GARUGA_MOT_KNOCK_DOWN_B_LIE = 0x4B,
    GARUGA_MOT_STATUS4 = 0x4C,          /* under status 4 (EmGaruga_Status4, Hold2) */
    GARUGA_MOT_FALL = 0x4D,             /* falling out of the air */
    GARUGA_MOT_FALL_LAND = 0x4E,
    GARUGA_MOT_HEAD_DOWN = 0x4F,        /* lowering the head to eat */
    GARUGA_MOT_IDLE_SIDE_A = 0x50,
    GARUGA_MOT_IDLE_SIDE_B = 0x51,
    GARUGA_MOT_DIE_FALLING = 0x52,
    GARUGA_MOT_HOP_IN_PLACE = 0x54,
    GARUGA_MOT_FLIP_LAND = 0x57,
    GARUGA_MOT_LEVEL_OUT = 0x5A,
    GARUGA_MOT_CHARGE_TRIP = 0x5B,      /* the end of a charge */
    GARUGA_MOT_TRAPPED = 0x5F,
    GARUGA_MOT_STRUGGLE = 0x60,
    GARUGA_MOT_DOWNED = 0x61,
    GARUGA_MOT_DOWNED_GET_UP = 0x62,
    GARUGA_MOT_ROAR = 0x64,
    GARUGA_MOT_TURN_STOMP = 0x65,       /* a turn in place (EmGaruga_Stand, IdleTurnToGoal, RunBAndTurn) */
    GARUGA_MOT_PECK_COMBO = 0x66,
    GARUGA_MOT_RUN_B = 0x68,
    GARUGA_MOT_FIREBALL_B = 0x69,
    GARUGA_MOT_PECK_COMBO_B = 0x6A
};

/* Group 0, EM_GROUP_IDLE (EmGaruga_RunIdleState). */
enum EmGarugaIdleState {
    GARUGA_IDLE_STAND = 0x00,
    GARUGA_IDLE = 0x01,
    GARUGA_IDLE_STEP_PECK = 0x02,
    GARUGA_IDLE_PECK = 0x03,
    GARUGA_IDLE_ACTION_C = 0x04,
    GARUGA_IDLE_SKIP_A = 0x05,
    GARUGA_IDLE_SKIP_B = 0x06,
    GARUGA_IDLE_SKIP_C = 0x07,
    GARUGA_IDLE_HOLD2_SHAKE_OFF = 0x08,
    GARUGA_IDLE_PAIR_A = 0x09,
    GARUGA_IDLE_TURN_TO_GOAL = 0x0A,
    GARUGA_IDLE_ACTION_H = 0x0B,
    GARUGA_IDLE_ACTION_I = 0x0C,
    GARUGA_IDLE_ACTION_J = 0x0D,
    GARUGA_IDLE_SKIP_D = 0x0E,
    GARUGA_IDLE_PAIR_B = 0x0F,
    GARUGA_IDLE_PAIR_C = 0x10,
    GARUGA_IDLE_ALERT = 0x11,
    GARUGA_IDLE_REST = 0x12,
    GARUGA_IDLE_EAT_START = 0x13,
    GARUGA_IDLE_STATUS2_COLLAPSE = 0x14,
    GARUGA_IDLE_STATUS2_REGEN = 0x15,
    GARUGA_IDLE_ACTION_K = 0x16,
    GARUGA_IDLE_REST_END = 0x17,
    GARUGA_IDLE_STATUS2_END = 0x18,
    GARUGA_IDLE_SHAKE_OFF = 0x19,
    GARUGA_IDLE_SKIP_E = 0x1A,
    GARUGA_IDLE_STATUS1_COLLAPSE = 0x1B,
    GARUGA_IDLE_STATUS1_END = 0x1C,
    GARUGA_IDLE_STATUS2_DOWNED = 0x1D,
    GARUGA_IDLE_STATUS1_DOWNED = 0x1F,
    GARUGA_IDLE_ROAR = 0x21,
    GARUGA_IDLE_STATUS4_SHAKE_OFF = 0x22,
    GARUGA_IDLE_ROAR_RAGE_BY_HP = 0x23,
    GARUGA_IDLE_STUN_SHAKE_OFF = 0x24,
    GARUGA_IDLE_ROAR_PENDING_RAGE = 0x25,
    GARUGA_IDLE_EAT = 0x28
};

/* Group 1, EM_GROUP_MOVE (EmGaruga_RunMoveState). */
enum EmGarugaMoveState {
    GARUGA_MOVE_WALK = 0x00,
    GARUGA_MOVE_END = 0x01,
    GARUGA_MOVE_RUN_B = 0x02,
    GARUGA_MOVE_TURN = 0x03,
    GARUGA_MOVE_RUN = 0x04,
    GARUGA_MOVE_TURN_IN_COMBAT = 0x05,
    GARUGA_MOVE_LIMP = 0x06,
    GARUGA_MOVE_TURN_TO_GOAL = 0x07,
    GARUGA_MOVE_RUN_SLOW = 0x08,
    GARUGA_MOVE_NOP = 0x09,
    GARUGA_MOVE_RUN_B_AND_TURN = 0x0A
};

/* Group 2, EM_GROUP_FLY (EmGaruga_RunFlyState). */
enum EmGarugaFlyState {
    GARUGA_FLY_TAKE_OFF = 0x00,
    GARUGA_FLY_LAND = 0x01,
    GARUGA_FLY_HOVER = 0x02,
    GARUGA_FLY_HOP_TO_GOAL = 0x03,
    GARUGA_FLY_HOP_FORWARD = 0x04,
    GARUGA_FLY_HOP_BACK = 0x05,
    GARUGA_FLY_ROUTE = 0x06,
    GARUGA_FLY_HOVER_TO_GOAL = 0x07,
    GARUGA_FLY_OUT_OF_AREA = 0x08,
    GARUGA_FLY_TO_GOAL_AND_LAND = 0x09,
    GARUGA_FLY_END_FLIGHT_TO_GLIDE = 0x0A,
    GARUGA_FLY_GLIDE_TO_GOAL = 0x0B,
    GARUGA_FLY_BACK_HOP = 0x0C,
    GARUGA_FLY_INTO_AREA = 0x0D,
    GARUGA_FLY_HOVER_ONCE = 0x0E,
    GARUGA_FLY_GLIDE_ONCE = 0x0F,
    GARUGA_FLY_HOVER_TURN = 0x10,
    GARUGA_FLY_ROUTE_B = 0x11,
    GARUGA_FLY_LEVEL_OUT_AND_LAND = 0x12,
    GARUGA_FLY_GLIDE_TO_GOAL_B = 0x13,
    GARUGA_FLY_HOP_IN_PLACE = 0x14,
    GARUGA_FLY_HOVER_STRAIGHT_TO_GOAL = 0x15,
    GARUGA_FLY_LEVEL_OUT = 0x16,
    GARUGA_FLY_ESCAPE_UPWARD = 0x17,
    GARUGA_FLY_HOVER_RISE = 0x18
};

/* Group 3, EM_GROUP_ATTACK (EmGaruga_RunAttackState). */
enum EmGarugaAttackState {
    GARUGA_ATK_TURN_AND_STARE = 0x00,
    GARUGA_ATK_STARE = 0x01,
    GARUGA_ATK_TAIL_SPIN = 0x02,
    GARUGA_ATK_TURN_AND_PECK_A = 0x03,
    GARUGA_ATK_CHARGE_TRIP = 0x04,
    GARUGA_ATK_TURN_AND_PECK_B = 0x05,
    GARUGA_ATK_SINGLE_ATTACK = 0x06,
    GARUGA_ATK_FIREBALL = 0x07,
    GARUGA_ATK_FIREBALL_LOW = 0x08,
    GARUGA_ATK_FIREBALL_HIGH = 0x09,
    GARUGA_ATK_FIREBALL_B = 0x0A,
    GARUGA_ATK_SOMERSAULT = 0x0B,
    GARUGA_ATK_TAIL_SPIN_B = 0x0C,
    GARUGA_ATK_PECK_A = 0x0D
};

/* Group 4, EM_GROUP_REACT (EmGaruga_RunReactState). */
enum EmGarugaReactState {
    GARUGA_REACT_FLINCH = 0x00,
    GARUGA_REACT_FLINCH_B = 0x01,
    GARUGA_REACT_FLINCH_C = 0x02,
    GARUGA_REACT_FLINCH_D = 0x03,
    GARUGA_REACT_PART_BREAK = 0x04,
    GARUGA_REACT_KNOCK_DOWN = 0x05,
    GARUGA_REACT_KNOCK_DOWN_PART3 = 0x06,
    GARUGA_REACT_FALL_THEN_GLIDE = 0x07,
    GARUGA_REACT_FALL = 0x08,
    GARUGA_REACT_FALL_B = 0x09,
    GARUGA_REACT_FLINCH_E = 0x0A,
    GARUGA_REACT_STATUS4 = 0x0B,
    GARUGA_REACT_TRAPPED = 0x0C,
    GARUGA_REACT_TRAPPED_STRUGGLE = 0x0D,
    GARUGA_REACT_STATUS4_DOWNED = 0x0E,
    GARUGA_REACT_TRAP_ESCAPE_HOP = 0x10,
    GARUGA_REACT_STATUS4_DOWNED_END = 0x11,
    GARUGA_REACT_STATUS2_DOWNED_END = 0x12,
    GARUGA_REACT_STATUS1_DOWNED_END = 0x13,
    GARUGA_REACT_PART_BREAK_END = 0x14,
    GARUGA_REACT_STUN = 0x15,
    GARUGA_REACT_STUN_DOWNED = 0x16,
    GARUGA_REACT_STUN_DOWNED_END = 0x17,
    GARUGA_REACT_HOLD2 = 0x18
};

/* Group 5, EM_GROUP_DEFEAT (EmGaruga_RunDefeatState). */
enum EmGarugaDefeatState {
    GARUGA_DEFEAT_DIE = 0x00,
    GARUGA_DEFEAT_DIE_HELD = 0x01,
    GARUGA_DEFEAT_DIE_FALLING = 0x02,
    GARUGA_DEFEAT_REAPPEAR = 0x03
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmGaruga_RunEventState). */
enum EmGarugaEventState {
    GARUGA_EVENT_STORY_INTRO = 0x00,
    GARUGA_EVENT_CAPTURED = 0x04,
    GARUGA_EVENT_CAPTURED_HOLD2 = 0x05
};

/* Group 8, EM_GROUP_VERSUS (EmGaruga_RunVersusState). */
enum EmGarugaVersusState {
    GARUGA_VS_EAT_BAIT = 0x00,
    GARUGA_VS_POSE_A = 0x01,
    GARUGA_VS_POSE_B = 0x02,
    GARUGA_VS_POSE_C = 0x03,
    GARUGA_VS_WAIT_BOTH = 0x98,
    GARUGA_VS_END = 0x99
};

#endif
