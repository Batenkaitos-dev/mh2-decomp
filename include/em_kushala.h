#ifndef EM_KUSHALA_H
#define EM_KUSHALA_H

/*
 * The Kushala Daora family's vocabulary (src/game/em_kushala.c, EmKushala_*):
 * Kushala Daora and its rusted form (EM_KIND_KUSHALA_DAORA,
 * EM_KIND_RUSTED_KUSHALA_DAORA), Lunastra and Teostra (EM_KIND_LUNASTRA,
 * EM_KIND_TEOSTRA). The four kinds share one set of state functions; the code
 * calls the last two "Tesca" (EmKushala_MotionSoundsTesca), and some attacks
 * and effects are theirs only.
 *
 * Stances (Em.stance): EM_STANCE_GROUND, 1 hovering low (the family's own
 * stance), EM_STANCE_AIR. EmKushala_ChangeState sends a state that doesn't
 * fit the stance to one that does.
 *
 * Motions: the kinds' motion tables have row N play motion N, so one number
 * serves Em_SetMotion's row and EM_MOTION_ID(layer, motion). The names say
 * what the states that play a motion show; motions no state explains stay
 * numbers.
 */
enum EmKushalaMotion {
    KUSH_MOT_IDLE = 0x01,               /* the standing loop, on all three layers */
    KUSH_MOT_WALK = 0x02,
    KUSH_MOT_WALK_B_START = 0x04,       /* KUSH_MOVE_WALK_B's first motion */
    KUSH_MOT_TURN_LEFT = 0x05,          /* EmKushala_TurnParams[0]: motLeft */
    KUSH_MOT_TURN_RIGHT = 0x06,         /* its motRight */
    KUSH_MOT_TURN_SMALL = 0x07,         /* KUSH_MOVE_TURN_ALERT within about 50 degrees */
    KUSH_MOT_TURN_BIG_RIGHT = 0x08,     /* KUSH_MOVE_TURN_ALERT, an angle under 0x8000 */
    KUSH_MOT_TURN_BIG_LEFT = 0x09,      /* KUSH_MOVE_TURN_ALERT, an angle of 0x8000 or more */
    KUSH_MOT_TAKE_OFF = 0x0A,           /* off the ground at frame 48 (curve kind 0) */
    KUSH_MOT_HOVER = 0x0B,              /* the wing-beat loop of hovering and slow flight */
    KUSH_MOT_FLY_FAST = 0x0C,           /* flight at speed 80 */
    KUSH_MOT_DASH_START = 0x0D,         /* the start of a fast flight (curve kind 1) */
    KUSH_MOT_DASH_END = 0x0E,           /* braking out of a fast flight (curve kind 2) */
    KUSH_MOT_IDLE_ACTION_B = 0x0F,
    KUSH_MOT_FLY_CLIMB = 0x10,          /* EmKushala_FlyParam's first motion: Em_UpdateFlight climbing */
    KUSH_MOT_LAND = 0x11,               /* touch-down: on the ground at frame 70; its tail from frame 146 settles */
    KUSH_MOT_HOVER_TURN = 0x12,         /* a turn in the hover */
    KUSH_MOT_SIDE_DASH_A = 0x13,
    KUSH_MOT_SIDE_DASH_B = 0x14,        /* also KUSH_MOVE_WALK_B's walking motion */
    KUSH_MOT_RUN_START = 0x15,
    KUSH_MOT_RUN_B = 0x16,              /* follows KUSH_MOT_RUN_START */
    KUSH_MOT_RUN_C = 0x17,              /* follows KUSH_MOT_RUN_B */
    KUSH_MOT_RUN_STOP = 0x18,
    KUSH_MOT_REAR_UP = 0x19,            /* opens the stomp and the lift into the hover */
    KUSH_MOT_STOMP = 0x1A,              /* attack 0xB at frame 16 */
    KUSH_MOT_LIFT_TO_HOVER = 0x1B,      /* into stance 1 */
    KUSH_MOT_BANK_A = 0x1C,             /* Em_UpdateFlight's banking motions, by rotZ */
    KUSH_MOT_BANK_B = 0x1D,
    KUSH_MOT_WING_FLAP = 0x1E,          /* before taking off from the idle, and after getting up in combat */
    KUSH_MOT_ALERT = 0x1F,              /* the alert stance */
    KUSH_MOT_ROAR = 0x20,               /* attack 0x1C at frame 62; the wind's burst at frame 186 */
    KUSH_MOT_TESCA_ACTION_C_END = 0x21,
    KUSH_MOT_IDLE_ACTION_A = 0x22,
    KUSH_MOT_EAT_START = 0x23,
    KUSH_MOT_EAT = 0x24,
    KUSH_MOT_LIMP = 0x25,
    KUSH_MOT_REST = 0x26,               /* lying at rest, with the sleeping puffs */
    KUSH_MOT_REST_END = 0x27,           /* getting up from the rest */
    KUSH_MOT_BREATH_STREAM_END = 0x28,
    KUSH_MOT_TESCA_ACTION_C = 0x29,
    KUSH_MOT_TESCA_ACTION_B = 0x2A,
    KUSH_MOT_TESCA_ACTION_A = 0x2B,
    KUSH_MOT_BREATH_FINISH = 0x2D,
    KUSH_MOT_STRIKE_BREATH = 0x2E,      /* attack at frame 60; Kushala's gust at frames 74-78 */
    KUSH_MOT_BREATH_CHAIN = 0x2F,       /* the chain of fire (EMFX_BREATH_CHAIN) at frame 104 */
    KUSH_MOT_BREATH_CHAIN_END = 0x30,
    KUSH_MOT_BREATH_WINDUP = 0x31,
    KUSH_MOT_BREATH_STREAM = 0x32,      /* the shot at frame 24 */
    KUSH_MOT_BREATH = 0x33,             /* one breath at frame 20; a pass ends at frame 46 */
    KUSH_MOT_LUNGE = 0x34,              /* the explosion 400 ahead at frame 52 */
    KUSH_MOT_HOVER_ACTION_A = 0x35,
    KUSH_MOT_HOVER_ACTION_B = 0x36,
    KUSH_MOT_TURN_SWEEP_A = 0x37,
    KUSH_MOT_TURN_SWEEP_B = 0x38,
    KUSH_MOT_HOVER_SWEEP_A = 0x39,
    KUSH_MOT_HOVER_SWEEP_B = 0x3A,
    KUSH_MOT_BLAST_CHARGE = 0x3B,       /* StageObj_K27 at frame 56 */
    KUSH_MOT_BLAST = 0x3C,              /* the blast ring at frame 36 */
    KUSH_MOT_FLINCH = 0x3D,
    KUSH_MOT_TOPPLE_RECOVER = 0x3E,     /* cut in after frame 104 of KUSH_MOT_TOPPLE */
    KUSH_MOT_FLINCH_HOVER = 0x3F,
    KUSH_MOT_FALL = 0x40,               /* falling out of the air */
    KUSH_MOT_FALL_LAND = 0x41,
    KUSH_MOT_DOWN = 0x42,               /* lying after a fall or a knock-down */
    KUSH_MOT_DOWN_GET_UP = 0x43,
    KUSH_MOT_DIE = 0x44,
    KUSH_MOT_STATUS4 = 0x45,
    KUSH_MOT_SHAKE_OFF = 0x46,
    KUSH_MOT_COLLAPSE = 0x47,           /* statuses 1 and 2 */
    KUSH_MOT_TOPPLE = 0x49,
    KUSH_MOT_PART_BREAK = 0x4A,
    KUSH_MOT_PART_BREAK_TURN = 0x4B,    /* one turn of the body over frames 0-104 */
    KUSH_MOT_DOWNED = 0x61              /* captured */
};

/* Group 0, EM_GROUP_IDLE (EmKushala_RunIdleState). */
enum EmKushalaIdleState {
    KUSH_IDLE_STAND = 0x00,             /* waits Em.timer, then asks the script */
    KUSH_IDLE = 0x01,                   /* EmKushala_ToIdle's state on the ground; KUSH_IDLE_ALERT in combat */
    KUSH_IDLE_HOVER = 0x02,             /* the idle of stance 1 */
    KUSH_IDLE_ALERT = 0x03,
    KUSH_IDLE_EAT_NEED1_START = 0x04,
    KUSH_IDLE_EAT_NEED1 = 0x05,         /* +5% hp, fills need 1 */
    KUSH_IDLE_ACTION_A = 0x06,
    KUSH_IDLE_STATUS4_SHAKE_OFF = 0x07, /* after KUSH_REACT_STATUS4 (Em_EndStatus4) */
    KUSH_IDLE_SHAKE_OFF = 0x08,         /* after the status ends */
    KUSH_IDLE_REST = 0x09,              /* lying, regaining hp; the wind stops */
    KUSH_IDLE_REST_END = 0x0A,
    KUSH_IDLE_STATUS2_COLLAPSE = 0x0B,
    KUSH_IDLE_STATUS2_END = 0x0C,
    KUSH_IDLE_EAT_START = 0x0D,
    KUSH_IDLE_EAT = 0x0E,               /* Em_StartGoalAction on entry */
    KUSH_IDLE_WIND_ROAR = 0x0F,         /* the roar that starts the wind */
    KUSH_IDLE_WIND_ROAR_B = 0x10,       /* the same, with Em.spawnParam 1 */
    KUSH_IDLE_WIND_ROAR_C = 0x11,       /* the same, with Em.spawnParam 2 */
    KUSH_IDLE_ACTION_B = 0x12,
    KUSH_IDLE_ALERT_B = 0x13,
    KUSH_IDLE_WIND_ROAR_OR_SWEEP = 0x14, /* with the wind already up: KUSH_ATK_TURN_SWEEP_B (Kushala) or KUSH_IDLE_ALERT */
    KUSH_IDLE_STATUS1_COLLAPSE = 0x15,
    KUSH_IDLE_STATUS1_END = 0x16,
    KUSH_IDLE_SETTLE = 0x17,            /* out of combat after an active motion */
    KUSH_IDLE_STUN_SHAKE_OFF = 0x18,    /* after KUSH_REACT_STUN (Em_EndStatus5) */
    KUSH_IDLE_EAT_NEED2_START = 0x19,
    KUSH_IDLE_EAT_NEED2 = 0x1A,         /* +5% hp, fills need 2 */
    KUSH_IDLE_ROAR = 0x1B               /* the roar with no change to the wind */
};

/* Group 1, EM_GROUP_MOVE (EmKushala_RunMoveState). 0x00-0x13 are ground moves, 0x15 and up hover moves. */
enum EmKushalaMoveState {
    KUSH_MOVE_WALK = 0x00,
    KUSH_MOVE_LIMP = 0x01,              /* the walk below 12% hp (Lunastra and Teostra: 15%) */
    KUSH_MOVE_TURN = 0x02,              /* KUSH_MOVE_TURN_ALERT in combat */
    KUSH_MOVE_TURN_ALERT = 0x03,
    KUSH_MOVE_WALK_B = 0x04,
    KUSH_MOVE_RUN = 0x05,               /* stops 500 short of the goal */
    KUSH_MOVE_STOMP = 0x06,
    KUSH_MOVE_LIFT_TO_HOVER = 0x07,
    KUSH_MOVE_RUN_PAST = 0x08,          /* the run, 700 past the goal */
    KUSH_MOVE_RUN_PAST_ARC_A = 0x09,    /* the same, curving: rotY +0x5B a frame */
    KUSH_MOVE_RUN_PAST_ARC_B = 0x0A,    /* +0xE3 */
    KUSH_MOVE_RUN_PAST_ARC_C = 0x0B,    /* -0x5B */
    KUSH_MOVE_RUN_PAST_ARC_D = 0x0C,    /* -0xE3 */
    KUSH_MOVE_TAKE_OFF_TO_HOVER = 0x14, /* the one move allowed on the ground and in the hover */
    KUSH_MOVE_SIDE_DASH_A = 0x15,
    KUSH_MOVE_SIDE_DASH_B = 0x16,
    KUSH_MOVE_HOVER_FORWARD = 0x17,
    KUSH_MOVE_HOVER_DASH = 0x18,
    KUSH_MOVE_HOVER_TURN = 0x19,
    KUSH_MOVE_LAND = 0x1A,              /* KUSH_MOVE_LAND_QUICK in combat */
    KUSH_MOVE_HOVER_SPIN = 0x1B,        /* one turn on the spot */
    KUSH_MOVE_HOVER_FORWARD_PAST = 0x1C, /* 200 past the goal */
    KUSH_MOVE_HOVER_BACKWARD = 0x1D,    /* backwards to the goal, facing away */
    KUSH_MOVE_LAND_QUICK = 0x1E         /* ends at frame 146 of KUSH_MOT_LAND */
};

/* Group 2, EM_GROUP_FLY (EmKushala_RunFlyState). */
enum EmKushalaFlyState {
    KUSH_FLY_TAKE_OFF = 0x00,
    KUSH_FLY_LAND = 0x01,               /* KUSH_FLY_LAND_IN_COMBAT in combat; where a state the stance forbids goes */
    KUSH_FLY_HOVER = 0x02,              /* the idle in the air */
    KUSH_FLY_HOVER_GUARDED = 0x03,      /* hits can't flinch or kill it (Update keeps guardTimer) */
    KUSH_FLY_DRIFT_TO_GOAL = 0x04,
    KUSH_FLY_DRIFT_TO_GOAL_PITCHED = 0x05,
    KUSH_FLY_SPIN_IN_PLACE = 0x06,
    KUSH_FLY_TURN_TO_GOAL = 0x07,
    KUSH_FLY_LEAVE_AREA = 0x08,
    KUSH_FLY_ENTER_AREA_AND_LAND = 0x09,
    KUSH_FLY_CROSS_AREA = 0x0A,         /* through an area on the way */
    KUSH_FLY_DASH_TO_GOAL = 0x0B,
    KUSH_FLY_DASH_BRAKE = 0x0C,
    KUSH_FLY_DASH_CIRCUIT = 0x0D,       /* a lap of the area's four points */
    KUSH_FLY_LAND_IN_COMBAT = 0x0E,     /* ends at frame 146 of KUSH_MOT_LAND */
    KUSH_FLY_RISE_TO_AREA_HEIGHT = 0x0F, /* the three rises are allowed in the hover */
    KUSH_FLY_RISE_TO_FIXED_HEIGHT = 0x10,
    KUSH_FLY_RISE_TO_GOAL_HEIGHT = 0x11
};

/* Group 3, EM_GROUP_ATTACK (EmKushala_RunAttackState): picked by the script; ChangeState doesn't remap them. */
enum EmKushalaAttackState {
    KUSH_ATK_TURN_SWEEP_A = 0x00,
    KUSH_ATK_TURN_SWEEP_B = 0x01,
    KUSH_ATK_STRIKE_BREATH = 0x02,
    KUSH_ATK_BREATH_CHAIN = 0x03,
    KUSH_ATK_TESCA_ACTION_A = 0x04,
    KUSH_ATK_BLAST_RING_A = 0x05,       /* the blast ring, ring type 0 */
    KUSH_ATK_TESCA_ACTION_B = 0x06,
    KUSH_ATK_BLAST_RING_B = 0x07,       /* ring type 1 */
    KUSH_ATK_TESCA_ACTION_C = 0x08,
    KUSH_ATK_HOVER_ACTION_A = 0x09,
    KUSH_ATK_BREATH_STREAM_ADVANCE = 0x0A,
    KUSH_ATK_LUNGE_BLAST = 0x0B,
    KUSH_ATK_HOVER_ACTION_B = 0x0C,
    KUSH_ATK_HOVER_SWEEP_A = 0x0D,
    KUSH_ATK_HOVER_SWEEP_B = 0x0E,
    KUSH_ATK_BREATH_QUICK = 0x0F,       /* EmKushala_BreathAttack's patterns, in the hover */
    KUSH_ATK_BREATH_FAN = 0x10,
    KUSH_ATK_BREATH_FULL = 0x11,
    KUSH_ATK_AIR_BREATH_QUICK = 0x12,   /* the same in the air */
    KUSH_ATK_AIR_BREATH_FAN = 0x13,
    KUSH_ATK_AIR_BREATH_FULL = 0x14,
    KUSH_ATK_BREATH_STREAM_WEAVE = 0x15,
    KUSH_ATK_BREATH_STREAM_SPIN = 0x16,
    KUSH_ATK_AIR_BREATH_TRIPLE = 0x17,  /* the first breath counts a hit on the fort */
    KUSH_ATK_BLAST_RING_FORT = 0x18,
    KUSH_ATK_AIR_BREATH_SINGLE = 0x19,
    KUSH_ATK_BLAST_RING_C = 0x1A,       /* ring type 2 */
    KUSH_ATK_BLAST_RING_D = 0x1B        /* ring type 3 */
};

/* Group 4, EM_GROUP_REACT (EmKushala_RunReactState): what EmKushala_Update picks from Em_UpdateDamage's result. */
enum EmKushalaReactState {
    KUSH_REACT_FLINCH = 0x00,
    KUSH_REACT_FLINCH_HEAVY = 0x01,     /* a part already damaged (parts[].state >= 2) */
    KUSH_REACT_FLINCH_HOVER = 0x02,
    KUSH_REACT_KNOCK_DOWN = 0x03,       /* the wind stops */
    KUSH_REACT_FALL = 0x04,             /* out of the air */
    KUSH_REACT_STATUS4 = 0x05,
    KUSH_REACT_PART_BREAK = 0x06,       /* the wind stops; part 2 breaks */
    KUSH_REACT_RETURN = 0x07,           /* back to the idle at once; nothing in the code enters it */
    KUSH_REACT_AIR_KNOCK_DOWN = 0x08,   /* the fall, with the wind stopped */
    KUSH_REACT_FLASH_FALL = 0x09,       /* a flash in the air (Em_ApplyFlash) */
    KUSH_REACT_FLASH_FLINCH = 0x0A,     /* a flash on the ground */
    KUSH_REACT_HOVER_KNOCK_DOWN = 0x0B, /* the wind stops */
    KUSH_REACT_STUN = 0x0C,
    KUSH_REACT_HOLD_RISE = 0x0D,        /* Kushala, held: rises into the hover */
    KUSH_REACT_HOLD_SETTLE = 0x0E,      /* Lunastra and Teostra, held: stay on the ground */
    KUSH_REACT_FIRST_BREAK_FLINCH = 0x0F /* Lunastra's and Teostra's first part-8 break: counted, then the flinch */
};

/* Group 5, EM_GROUP_DEFEAT (EmKushala_RunDefeatState). */
enum EmKushalaDefeatState {
    KUSH_DEFEAT_DIE = 0x00,
    KUSH_DEFEAT_DIE_B = 0x01,           /* the same function; nothing in the code enters it */
    KUSH_DEFEAT_DIE_FALLING = 0x02,
    KUSH_DEFEAT_REAPPEAR = 0x03
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmKushala_RunEventState). The scenes wait for a story event flag. */
enum EmKushalaEventState {
    KUSH_EVENT_LEAVE = 0x00,            /* flies up out of the area */
    KUSH_EVENT_LEAVE_FROM_AIR = 0x01,
    KUSH_EVENT_SCENE_WALK_IN = 0x02,    /* Kushala, AREA_SNOWY_MOUNTAINS_6 */
    KUSH_EVENT_SCENE_RISE = 0x03,       /* Lunastra and Teostra, AREA_VOLCANO_8 */
    KUSH_EVENT_CAPTURED = 0x04,
    KUSH_EVENT_SCENE_DESCEND = 0x05,    /* Lunastra and Teostra, AREA_VOLCANO_7 */
    KUSH_EVENT_SCENE_FLY_IN = 0x06,     /* Kushala, AREA_JUNGLE_3 */
    KUSH_EVENT_SCENE_WAKE = 0x07        /* Lunastra and Teostra, AREA_TOWER_0x7C */
};

#endif
