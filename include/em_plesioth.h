#ifndef EM_PLESIOTH_H
#define EM_PLESIOTH_H

/*
 * The Plesioth family (EM_KIND_PLESIOTH, EM_KIND_GREEN_PLESIOTH): its
 * motions and its states, by group (em.h's EmStateGroup). The functions are
 * EmPlesioth_*. Group 2, the flying families' group, holds this family's
 * swimming states; Em.stance is EM_STANCE_UNDER under water. Each enum lists
 * the cases of the group's dispatcher (EmPlesioth_Run*State); where one
 * state function serves several numbers, the names say which variant the
 * number asks for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmPlesiothMotion {
    PLES_MOT_IDLE = 0x01,               /* the standing loop */
    PLES_MOT_WALK = 0x03,               /* PLES_MOVE_WALK */
    PLES_MOT_RUN = 0x04,                /* PLES_MOVE_RUN, followed by PLES_MOT_RUN_STOP */
    PLES_MOT_TURN_RIGHT = 0x05,         /* the land turn table (motRight), PLES_MOVE_TURN */
    PLES_MOT_TURN_LEFT = 0x06,          /* the land turn table (motLeft), PLES_MOVE_TURN */
    PLES_MOT_LIMP = 0x0A,               /* PLES_MOVE_LIMP (below a fifth of max hp) */
    PLES_MOT_IDLE_ACTION = 0x19,        /* PLES_IDLE_ACTION */
    PLES_MOT_REST = 0x1F,               /* the rest loop of PLES_IDLE_REST */
    PLES_MOT_REST_START = 0x20,         /* lying down to rest (PLES_IDLE_REST) */
    PLES_MOT_REST_END = 0x21,           /* getting up from the rest (PLES_IDLE_REST_END) */
    PLES_MOT_STATUS_LIE = 0x22,         /* lying under status 1 or 2 on land ... */
    PLES_MOT_LAND_ATTACK_A = 0x24,      /* PLES_ATK_LAND_A (attack row 1 at frame 50) */
    PLES_MOT_STATUS_GET_UP = 0x25,      /* getting up after a status (PLES_IDLE_STATUS2_END, ... */
    PLES_MOT_STATUS_COLLAPSE = 0x26,    /* collapsing under status 1 or 2 on land (the STATUS COLLAPSE ... */
    PLES_MOT_SPIN = 0x2B,               /* PLES_ATK_SPIN, played twice (attack row 0x10 at frame 30) */
    PLES_MOT_BREATH = 0x2F,             /* PLES_ATK_BREATH (attack row 0x19 at frame 66, ranged attack ... */
    PLES_MOT_FLINCH = 0x3C,             /* PLES_REACT_FLINCH, and the start of PLES_REACT_HOLD2 */
    PLES_MOT_FLINCH_BLIND = 0x3D,       /* PLES_REACT_FLINCH_BLIND */
    PLES_MOT_SHAKE_OFF = 0x3E,          /* PLES_IDLE_SHAKE_OFF and the STATUS4, HOLD2 and STUN ... */
    PLES_MOT_FLINCH_C = 0x3F,           /* PLES_REACT_FLINCH_C */
    PLES_MOT_FLINCH_D = 0x40,           /* PLES_REACT_FLINCH_D */
    PLES_MOT_FLINCH_B = 0x42,           /* PLES_REACT_FLINCH_B */
    PLES_MOT_DIE = 0x44,                /* PLES_DEFEAT_DIE (dust at frame 50) */
    PLES_MOT_FALL_A = 0x45,             /* KnockDown odd variants (KNOCK_DOWN, STUN_A): falls over ... */
    PLES_MOT_LIE_A = 0x46,              /* KnockDown odd variants: lies flapping (effects at 16 and 66) */
    PLES_MOT_GET_UP_A = 0x47,           /* KnockDown odd variants: gets up */
    PLES_MOT_FALL_B = 0x4A,             /* KnockDown even variants (KNOCK_DOWN_PART3, STUN_B): falls ... */
    PLES_MOT_LIE_DOWN = 0x4B,           /* KnockDown even variants, FlinchLying, Status4Lying, ... */
    PLES_MOT_STAND_STILL = 0x4C,        /* PLES_REACT_STATUS4 and PLES_REACT_HOLD2: stands under ... */
    PLES_MOT_TRAPPED = 0x5F,            /* PLES_REACT_TRAPPED (dust when enraged) */
    PLES_MOT_TRAP_STRUGGLE = 0x60,      /* TRAPPED_STRUGGLE, STATUS4_HELD_END, STUN_HELD_END: looped ... */
    PLES_MOT_TRAPPED_LIE = 0x61,        /* lying under a status while held (PLES_IDLE_STATUS2_TRAPPED, ... */
    PLES_MOT_WAKE_IN_TRAP = 0x62,       /* WakeInTrap */
    PLES_MOT_SWIM_IDLE = 0x64,          /* hovering under water (PLES_SWIM_IDLE, _AREA, _B, ... */
    PLES_MOT_SWIM_START = 0x65,         /* the start of a swim (PLES_SWIM_FORWARD) */
    PLES_MOT_SWIM = 0x66,               /* the swim loop after PLES_MOT_SWIM_START (PLES_SWIM_FORWARD) */
    PLES_MOT_SWIM_FAST = 0x67,          /* fast swim (PLES_SWIM_DASH, PLES_SWIM_ROUND_NEXT and _PREV) ... */
    PLES_MOT_SWIM_RISE = 0x68,          /* rising to the surface (PLES_SWIM_RISE) */
    PLES_MOT_SWIM_DIVE = 0x69,          /* sinking to the floor (PLES_SWIM_DIVE) */
    PLES_MOT_SWIM_TURN_LEFT = 0x6A,     /* the swim turn tables (motLeft): PLES_SWIM_TURN, ... */
    PLES_MOT_SWIM_TURN_RIGHT = 0x6B,    /* the swim turn tables (motRight): PLES_SWIM_TURN, ... */
    PLES_MOT_LEAP_ATTACK = 0x6C,        /* PLES_ATK_LEAP and EntranceScene (attack row 0xE at frame 10) */
    PLES_MOT_SWIM_CURVE = 0x6E,         /* PLES_SWIM_CURVE only (moved by curve set 0 */
    PLES_MOT_DEAD_FLOAT = 0x6F,         /* PLES_DEFEAT_DIE_UNDER: floats up dead */
    PLES_MOT_SURFACE_BREATH_C = 0x70,   /* PLES_ATK_SURFACE_BREATH_C */
    PLES_MOT_SWIM_CIRCLE_RIGHT = 0x72,  /* PLES_SWIM_CIRCLE_RIGHT */
    PLES_MOT_SWIM_CIRCLE_LEFT = 0x73,   /* PLES_SWIM_CIRCLE_LEFT */
    PLES_MOT_SURFACE_BREATH_A = 0x74,   /* PLES_ATK_SURFACE_BREATH_A */
    PLES_MOT_SURFACE_BREATH_B = 0x75,   /* PLES_ATK_SURFACE_BREATH_B and EntranceScene */
    PLES_MOT_LEAP = 0x77,               /* airborne in a leap (PLES_SWIM_LEAP, PLES_SWIM_LEAP_IN, ... */
    PLES_MOT_LEAP_CROUCH = 0x78,        /* the crouch on land before a leap into the water ... */
    PLES_MOT_FLOP_A = 0x79,             /* landing flop of PLES_IDLE_LAND_FLOP_A */
    PLES_MOT_BEACHED = 0x7A,            /* lying beached on its side (stance EM_STANCE_FAMILY) in the ... */
    PLES_MOT_FLOP_B = 0x7B,             /* landing flop of PLES_IDLE_LAND_FLOP_B */
    PLES_MOT_GET_UP = 0x7C,             /* FlinchLying, KnockDown even variants, PulledAshore, ... */
    PLES_MOT_TUMBLE = 0x7E,             /* FallFromAir, FallBlinded, PulledAshore, ThrownOutOfWater, ... */
    PLES_MOT_DIE_LYING = 0x7F,          /* PLES_DEFEAT_DIE_ASHORE (dust at frame 50, as DIE) */
    PLES_MOT_LIE_FLAP = 0x80,           /* after LIE_DOWN in FlinchLying, Status4Lying, PulledAshore, ... */
    PLES_MOT_LIE_FLAP_B = 0x81,         /* KnockDown even variants after LIE_DOWN (effect at frame 42) */
    PLES_MOT_SWIM_STATUS_FLOAT = 0x82,  /* floating under a status in the water ... */
    PLES_MOT_SWIM_RECOVER = 0x83,       /* recovering in the water (PLES_SWIM_STATUS2_END, ... */
    PLES_MOT_HOOKED = 0x84,             /* pulled on the line (PLES_SWIM_HOOKED) */
    PLES_MOT_RUN_STOP = 0x85,           /* after the run (PLES_MOVE_RUN) */
    PLES_MOT_SWIM_SURFACED = 0x86,      /* the end of PLES_SWIM_RISE at the surface */
    PLES_MOT_SWIM_BOTTOMED = 0x87,      /* the end of PLES_SWIM_DIVE on the floor */
    PLES_MOT_LAND_ATTACK_B = 0x88,      /* PLES_ATK_LAND_B (attack row 0x16 at frame 54) */
    PLES_MOT_SIDE_LIE = 0x89,           /* lying on its side under a status (the _BEACHED collapses) */
    PLES_MOT_STATUS4_LYING = 0x8A       /* Status4Lying: lies under status 4 */
};

/* Group 0, EM_GROUP_IDLE (EmPlesioth_RunIdleState). */
enum EmPlesiothIdleState {
    PLES_IDLE_STAND = 0x00,             /* layered idle until the timer, then the script decides */
    PLES_IDLE = 0x01,                   /* idle on land */
    PLES_IDLE_ACTION = 0x02,            /* motion 0x19 then the idle motion */
    PLES_IDLE_LAND_FLOP_A = 0x04,       /* landed from a leap: flop 0x79, beached, gets up (State2_0E, ... */
    PLES_IDLE_STATUS2_COLLAPSE = 0x05,  /* entry begins status 2 */
    PLES_IDLE_STATUS2_END = 0x06,       /* entry ends status 2 */
    PLES_IDLE_SHAKE_OFF = 0x07,         /* motion 0x3E after getting up */
    PLES_IDLE_STATUS2_TRAPPED = 0x08,   /* entry begins status 2 */
    PLES_IDLE_LAND_FLOP_B = 0x09,       /* as LAND_FLOP_A with flop 0x7B (not entered by this file) */
    PLES_IDLE_STATUS1_TRAPPED = 0x0A,   /* entry begins status 1 */
    PLES_IDLE_STATUS1_COLLAPSE = 0x0C,  /* entry begins status 1 */
    PLES_IDLE_STATUS1_END = 0x0D,       /* entry ends status 1 */
    PLES_IDLE_STATUS2_COLLAPSE_BEACHED = 0x0E,/* entry begins status 2 */
    PLES_IDLE_STATUS2_END_BEACHED = 0x0F,/* entry ends status 2 */
    PLES_IDLE_STATUS1_COLLAPSE_BEACHED = 0x10,/* entry begins status 1 */
    PLES_IDLE_STATUS1_END_BEACHED = 0x11,/* entry ends status 1 */
    PLES_IDLE_REST = 0x12,              /* rests 1800 frames regaining hp */
    PLES_IDLE_REST_END = 0x13,          /* gets up from the rest (0x21) */
    PLES_IDLE_STATUS4_SHAKE_OFF = 0x14, /* entry ends status 4 */
    PLES_IDLE_HOLD2_SHAKE_OFF = 0x15,   /* entry clears holdTimer2 */
    PLES_IDLE_STUN_SHAKE_OFF = 0x16,    /* entry ends status 5 (the stun) */
    PLES_IDLE_STATUS1_COLLAPSE_B = 0x29,/* entry begins status 1 */
    PLES_IDLE_STATUS1_END_B = 0x2A      /* entry ends status 1 */
};

/* Group 1, EM_GROUP_MOVE (EmPlesioth_RunMoveState). */
enum EmPlesiothMoveState {
    PLES_MOVE_WALK = 0x00,              /* walk to the goal */
    PLES_MOVE_RUN = 0x01,               /* run to 300 short of the goal, then stop */
    PLES_MOVE_TURN = 0x02,              /* turn toward the target on land */
    PLES_MOVE_LIMP = 0x03               /* limp to the goal (low hp) */
};

/* Group 2, EM_GROUP_FLY (EmPlesioth_RunSwimState): for this family, swimming, surfacing and leaping. */
enum EmPlesiothSwimState {
    PLES_SWIM_IDLE = 0x00,              /* hovering under water */
    PLES_SWIM_IDLE_AREA = 0x01,         /* swim idle that applies the area values */
    PLES_SWIM_NONE = 0x02,              /* empty state */
    PLES_SWIM_TURN = 0x03,              /* turn toward the target under water */
    PLES_SWIM_FORWARD = 0x04,           /* swim to the goal */
    PLES_SWIM_DASH = 0x05,              /* fast swim to the goal with wakes */
    PLES_SWIM_CURVE = 0x06,             /* motion 0x6E on curve set 0 */
    PLES_SWIM_RISE = 0x07,              /* rise to the surface */
    PLES_SWIM_DIVE = 0x08,              /* sink to the floor */
    PLES_SWIM_ROUND_TURN = 0x09,        /* turn to the next area point, then ROUND_NEXT */
    PLES_SWIM_ROUND_NEXT = 0x0A,        /* 4 legs round the area points, rising order (entry may pick ... */
    PLES_SWIM_ROUND_PREV = 0x0B,        /* 4 legs round the area points, falling order */
    PLES_SWIM_CIRCLE_RIGHT = 0x0C,      /* swims a circle turning right */
    PLES_SWIM_CIRCLE_LEFT = 0x0D,       /* swims a circle turning left */
    PLES_SWIM_LEAP = 0x0E,              /* leap out of the water, back in or onto land */
    PLES_SWIM_SURFACE_FLOP = 0x0F,      /* flop at the surface with a splash */
    PLES_SWIM_LEAP_IN = 0x10,           /* leap from land into the water */
    PLES_SWIM_BAIT_WAIT = 0x11,         /* waits by a baited line */
    PLES_SWIM_HOOKED = 0x12,            /* pulled toward the reeling player, then react 0xF */
    PLES_SWIM_STATUS2_COLLAPSE = 0x13,  /* entry begins status 2 */
    PLES_SWIM_STATUS1_COLLAPSE = 0x14,  /* entry begins status 1 (not entered by this file) */
    PLES_SWIM_STATUS4_RECOVER = 0x15,   /* entry ends status 4 */
    PLES_SWIM_TO_IDLE = 0x16,           /* goes straight to PLES_SWIM_IDLE */
    PLES_SWIM_TRAP_ESCAPE = 0x17,       /* out of a trap when its time ends */
    PLES_SWIM_STATUS2_END = 0x18,       /* entry ends status 2 */
    PLES_SWIM_STATUS1_END = 0x19,       /* entry ends status 1 */
    PLES_SWIM_LEAP_IN_TO_GOAL = 0x1A,   /* leap from land onto the goal in 60 frames */
    PLES_SWIM_IDLE_B = 0x1B             /* SwimIdle without the deep-water check on entry */
};

/* Group 3, EM_GROUP_ATTACK (EmPlesioth_RunAttackState). */
enum EmPlesiothAttackState {
    PLES_ATK_LAND_A = 0x00,             /* land attack, motion LAND_ATTACK_A */
    PLES_ATK_BREATH = 0x01,             /* breath on land */
    PLES_ATK_SPIN = 0x02,               /* turning spin on land */
    PLES_ATK_SURFACE_BREATH_A = 0x03,   /* breath from the water surface (falls back to land idle 0/1 ... */
    PLES_ATK_SURFACE_BREATH_B = 0x04,   /* as _A, other motion (same deep-water check) */
    PLES_ATK_SURFACE_BREATH_C = 0x05,   /* as _A, other ranged attack arguments (same check) */
    PLES_ATK_LEAP = 0x06,               /* leap out of the water (same check) */
    PLES_ATK_LAND_B = 0x07              /* land attack, motion LAND_ATTACK_B */
};

/* Group 4, EM_GROUP_REACT (EmPlesioth_RunReactState). */
enum EmPlesiothReactState {
    PLES_REACT_FLINCH = 0x00,           /* flinch, parts 0 and 7, or EM_DMG_HIT_MODE3 */
    PLES_REACT_FLINCH_B = 0x01,         /* flinch, unbroken part 3 or 4 */
    PLES_REACT_FLINCH_C = 0x02,         /* flinch, parts 5 and 6 */
    PLES_REACT_FLINCH_D = 0x03,         /* flinch, parts 1 and 2 */
    PLES_REACT_FLINCH_LYING = 0x04,     /* flinch while lying on land (stance 1) */
    PLES_REACT_KNOCK_DOWN = 0x05,       /* flinch of a broken part other than 3 (KnockDown variant 1) */
    PLES_REACT_FALL = 0x06,             /* falls out of the air (EM_DMG_STATUS_IN_AIR, a flinch in the ... */
    PLES_REACT_STATUS4_LYING = 0x07,    /* status 4 while lying on land */
    PLES_REACT_STATUS4_HELD_END = 0x08, /* ends status 4, struggles in the trap */
    PLES_REACT_FALL_BLIND = 0x09,       /* blinded fall, no C entry */
    PLES_REACT_FLINCH_BLIND = 0x0A,     /* blinded flinch on land, no C entry */
    PLES_REACT_STATUS4 = 0x0B,          /* status 4 on land */
    PLES_REACT_TRAPPED = 0x0C,          /* caught by a trap (ends all statuses) */
    PLES_REACT_TRAPPED_STRUGGLE = 0x0D, /* struggles until trapTimer runs out */
    PLES_REACT_STATUS4_HELD = 0x0E,     /* status 4 while held (begins status 4) */
    PLES_REACT_PULLED_ASHORE = 0x0F,    /* hooked and pulled out of the water (ChangeState breaks part ... */
    PLES_REACT_LAND = 0x10,             /* lands on the ground after a fall and gets up */
    PLES_REACT_THROWN_OUT = 0x11,       /* thrown up out of the water (attack kind 5, a flinch under ... */
    PLES_REACT_STATUS4_UNDER = 0x12,    /* status 4 under water: floats at the surface (begins status ... */
    PLES_REACT_STATUS2_TRAPPED_END = 0x13,/* ends status 2, wakes in the trap */
    PLES_REACT_STATUS1_TRAPPED_END = 0x14,/* ends status 1, wakes in the trap */
    PLES_REACT_STATUS4_LYING_END = 0x15,/* ends status 4, gets up from lying */
    PLES_REACT_STATUS1_UNDER = 0x16,    /* status 1 under water: floats at the surface (begins status ... */
    PLES_REACT_KNOCK_DOWN_PART3 = 0x17, /* flinch of the broken part 3 (KnockDown variant 0) */
    PLES_REACT_HOLD2 = 0x18,            /* EM_DMG_ATTACK13 (ends all statuses) */
    PLES_REACT_STUN_A = 0x19,           /* the stun, hit from (hitAngle - rotY) above 0x8000 (begins ... */
    PLES_REACT_STUN_B = 0x1A,           /* the stun, hit from the other side (begins the stun) */
    PLES_REACT_STUN_HELD_END = 0x1B,    /* ends the stun, struggles in the trap */
    PLES_REACT_STUN_HELD = 0x1C         /* the stun while held (begins the stun) */
};

/* Group 5, EM_GROUP_DEFEAT (EmPlesioth_RunDefeatState). */
enum EmPlesiothDefeatState {
    PLES_DEFEAT_DIE = 0x00,             /* dies on land */
    PLES_DEFEAT_DIE_HELD = 0x01,        /* dies while held */
    PLES_DEFEAT_DIE_UNDER = 0x02,       /* dies under water, floats up */
    PLES_DEFEAT_DIE_ASHORE = 0x03,      /* dies from the pull ashore */
    PLES_DEFEAT_GONE = 0x04             /* after the fade: RunGroup5 has no case, nothing runs */
};

/* Group 6, EM_GROUP_EVENT (EmPlesioth_RunEventState). */
enum EmPlesiothEventState {
    PLES_EVENT_CAPTURED = 0x00,         /* captured in a hold (begins status 1) */
    PLES_EVENT_ENTRANCE = 0x01,         /* scripted entrance swim */
    PLES_EVENT_CAPTURED_HOLD2 = 0x02    /* captured in the second hold */
};

#endif
