#ifndef EM_CEPHADROME_H
#define EM_CEPHADROME_H

/*
 * The Cephadrome family (EM_KIND_CEPHADROME, EM_KIND_CEPHALOS): its motions
 * and its states, by group (em.h's EmStateGroup). The functions are
 * EmCephadrome_*. The family swims under the sand (Em.stance
 * EM_STANCE_UNDER), surfaces to breathe sand at hunters, leaps out of the
 * sand and back, and lies beached on its side after a leap ashore
 * (EM_STANCE_FAMILY). Its group 2, the flying families' group, holds the
 * sand-swimming states. Each enum lists the cases of the group's dispatcher
 * (EmCephadrome_Run*State); where one state function serves several numbers,
 * the names say which variant the number asks for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmCephadromeMotion {
    CEPH_MOT_IDLE = 0x01,                   /* the standing loop on all three layers (CEPH_IDLE_STAND, ... */
    CEPH_MOT_WALK = 0x03,                   /* CEPH_MOVE_WALK */
    CEPH_MOT_RUN = 0x04,                    /* CEPH_MOVE_RUN, followed by CEPH_MOT_RUN_STOP */
    CEPH_MOT_TURN_RIGHT = 0x05,             /* the land turn row (motRight of EmCephadrome_TurnParams[0]), ... */
    CEPH_MOT_TURN_LEFT = 0x06,              /* the land turn row (motLeft of EmCephadrome_TurnParams[0]), ... */
    CEPH_MOT_LIMP = 0x0A,                   /* CEPH_MOVE_LIMP */
    CEPH_MOT_IDLE_ACTION = 0x19,            /* CEPH_IDLE_ACTION */
    CEPH_MOT_REST = 0x1F,                   /* the rest loop of CEPH_IDLE_REST */
    CEPH_MOT_REST_START = 0x20,             /* lying down to rest (CEPH_IDLE_REST) */
    CEPH_MOT_REST_END = 0x21,               /* getting up from the rest (CEPH_IDLE_REST_END) */
    CEPH_MOT_STATUS_LIE = 0x22,             /* lying under status 1 or 2 on the ground until the timer ... */
    CEPH_MOT_LAND_ATTACK_A = 0x24,          /* CEPH_ATK_LAND_A (attack row 1 at frame 50) */
    CEPH_MOT_STATUS_GET_UP = 0x25,          /* getting up after a status (CEPH_IDLE_STATUS2_END, ... */
    CEPH_MOT_STATUS_COLLAPSE = 0x26,        /* collapsing under status 1 or 2 on the ground ... */
    CEPH_MOT_SPIN = 0x2B,                   /* CEPH_ATK_SPIN, played twice (attack row 0x10 at frame 30, ... */
    CEPH_MOT_BREATH = 0x2F,                 /* CEPH_ATK_BREATH (attack row 0x19 at 66, ranged attack at 78) */
    CEPH_MOT_FEED = 0x3B,                   /* feeding after CEPH_MOT_HEAD_DOWN (CEPH_IDLE_EAT_NEED2, ... */
    CEPH_MOT_FLINCH = 0x3C,                 /* CEPH_REACT_FLINCH and the start of CEPH_REACT_HOLD2 ... */
    CEPH_MOT_FLINCH_BLIND = 0x3D,           /* CEPH_REACT_FLINCH_BLIND (effect at 24) */
    CEPH_MOT_SHAKE_OFF = 0x3E,              /* CEPH_IDLE_SHAKE_OFF and the STATUS4, HOLD2 and STUN ... */
    CEPH_MOT_FLINCH_C = 0x3F,               /* CEPH_REACT_FLINCH_C */
    CEPH_MOT_FLINCH_D = 0x40,               /* CEPH_REACT_FLINCH_D */
    CEPH_MOT_FLINCH_B = 0x42,               /* CEPH_REACT_FLINCH_B (dust at 268) */
    CEPH_MOT_DIE = 0x44,                    /* CEPH_DEFEAT_DIE, the end of DIE_UNDER and DIE_FALLING ... */
    CEPH_MOT_FALL_A = 0x45,                 /* KnockDown odd variants (KNOCK_DOWN, STUN_A): falls over ... */
    CEPH_MOT_LIE_A = 0x46,                  /* KnockDown odd variants: lies flapping (effects at 16 and 66) */
    CEPH_MOT_GET_UP_A = 0x47,               /* KnockDown odd variants and ThrownOutOfSand: gets up */
    CEPH_MOT_FALL_B = 0x4A,                 /* KnockDown even variants (KNOCK_DOWN_PART3, STUN_B): falls ... */
    CEPH_MOT_LIE_DOWN = 0x4B,               /* lying down at the start of the _BEACHED status collapses */
    CEPH_MOT_STAND_STILL = 0x4C,            /* CEPH_REACT_STATUS4 and CEPH_REACT_HOLD2: stands still until ... */
    CEPH_MOT_HEAD_DOWN = 0x4F,              /* lowering the head to eat, dust clouds and a sound on given ... */
    CEPH_MOT_DIE_HELD = 0x61,               /* CEPH_DEFEAT_DIE_HELD only (Plesioth plays its 0x61 as the ... */
    CEPH_MOT_SAND_IDLE = 0x64,              /* CEPH_SAND_IDLE */
    CEPH_MOT_SAND_SWIM_START = 0x65,        /* the start of CEPH_SAND_FORWARD */
    CEPH_MOT_SAND_SWIM = 0x66,              /* the swim loop after SWIM_START (CEPH_SAND_FORWARD) */
    CEPH_MOT_SAND_SWIM_FAST = 0x67,         /* fast swim (CEPH_SAND_DASH, ROUND_NEXT/PREV and their LEGs) ... */
    CEPH_MOT_SAND_RISE = 0x68,              /* CEPH_SAND_RISE */
    CEPH_MOT_SAND_DIVE = 0x69,              /* CEPH_SAND_DIVE */
    CEPH_MOT_SAND_TURN_LEFT = 0x6A,         /* the in-place turn of CEPH_EVENT_ENTRANCE (second turn) */
    CEPH_MOT_SAND_TURN_RIGHT = 0x6B,        /* the in-place turn of CEPH_EVENT_ENTRANCE (first turn) */
    CEPH_MOT_LEAP_ATTACK = 0x6C,            /* CEPH_ATK_LEAP (attack row 0xE at frame 10) */
    CEPH_MOT_SAND_IDLE_B = 0x6D,            /* CEPH_SAND_IDLE_B */
    CEPH_MOT_SAND_CURVE = 0x6E,             /* CEPH_SAND_CURVE only (curve set 0) */
    CEPH_MOT_SURFACE_BREATH_C = 0x70,       /* CEPH_ATK_SURFACE_BREATH_C (attack row 0x1D and pad ... */
    CEPH_MOT_SURFACE_LUNGE = 0x71,          /* CEPH_ATK_SURFACE_LUNGE (attack row 0x1B at 28) */
    CEPH_MOT_SAND_CIRCLE_RIGHT = 0x72,      /* CEPH_SAND_CIRCLE_RIGHT and CIRCLE_RIGHT_B */
    CEPH_MOT_SAND_CIRCLE_LEFT = 0x73,       /* CEPH_SAND_CIRCLE_LEFT */
    CEPH_MOT_SURFACE_BREATH_A = 0x74,       /* CEPH_ATK_SURFACE_BREATH_A */
    CEPH_MOT_SURFACE_BREATH_B = 0x75,       /* CEPH_ATK_SURFACE_BREATH_B */
    CEPH_MOT_LEAP = 0x77,                   /* airborne in a leap (SAND_LEAP_ASHORE, SAND_LEAP_IN, ... */
    CEPH_MOT_LEAP_CROUCH = 0x78,            /* the crouch on land before SAND_LEAP_IN and SAND_LEAP_ON_LAND */
    CEPH_MOT_FLOP_A = 0x79,                 /* landing flop of CEPH_IDLE_LAND_FLOP_A */
    CEPH_MOT_BEACHED = 0x7A,                /* lying beached on its side (stance EM_STANCE_FAMILY) in the ... */
    CEPH_MOT_FLOP_B = 0x7B,                 /* landing flop of CEPH_IDLE_LAND_FLOP_B */
    CEPH_MOT_GET_UP = 0x7C,                 /* getting up from its side: the end of the land flops, ... */
    CEPH_MOT_TUMBLE = 0x7E,                 /* FallFromAir, FallBlinded, ThrownOutOfSand: tumbling in the ... */
    CEPH_MOT_LIE_FLAP = 0x80,               /* after CEPH_MOT_LIE_DOWN in the _BEACHED status collapses */
    CEPH_MOT_LIE_FLAP_B = 0x81,             /* KnockDown even variants after LIE_DOWN (effect at 46) */
    CEPH_MOT_SAND_RECOVER = 0x83,           /* recovering under the sand (SAND_STATUS4_RECOVER, ... */
    CEPH_MOT_RUN_STOP = 0x85,               /* after the run (CEPH_MOVE_RUN) */
    CEPH_MOT_SAND_SURFACED = 0x86,          /* the end of CEPH_SAND_RISE (attack row 0x1C at frame 2) */
    CEPH_MOT_SAND_BOTTOMED = 0x87,          /* the end of CEPH_SAND_DIVE on the floor */
    CEPH_MOT_LAND_ATTACK_B = 0x88,          /* CEPH_ATK_LAND_B (attack row 0x16 at frame 60) */
    CEPH_MOT_SIDE_LIE = 0x89,               /* lying on its side under a status until the timer, sand ... */
    CEPH_MOT_STATUS4_LYING = 0x8A           /* Status4Lying: lies under status 4 */
};

/* Group 0, EM_GROUP_IDLE (EmCephadrome_RunIdleState). */
enum EmCephadromeIdleState {
    CEPH_IDLE_STAND = 0x00,                 /* layered idle until the timer, then the script decides */
    CEPH_IDLE = 0x01,                       /* idle on land */
    CEPH_IDLE_ACTION = 0x02,                /* CEPH_MOT_IDLE_ACTION then the idle motion */
    CEPH_IDLE_EAT_NEED2 = 0x03,             /* eats in one go, +5% hp, refills need 2 */
    CEPH_IDLE_LAND_FLOP_A = 0x04,           /* landed from a leap: flop A, beached, gets up (entered by ... */
    CEPH_IDLE_STATUS2_COLLAPSE = 0x05,      /* entry begins status 2 */
    CEPH_IDLE_STATUS2_END = 0x06,           /* entry ends status 2 */
    CEPH_IDLE_SHAKE_OFF = 0x07,             /* CEPH_MOT_SHAKE_OFF after getting up */
    CEPH_IDLE_LAND_FLOP_B = 0x09,           /* as LAND_FLOP_A with flop B (not entered by this file) */
    CEPH_IDLE_STATUS2_COLLAPSE_BEACHED = 0x0E,/* entry begins status 2 */
    CEPH_IDLE_STATUS2_END_BEACHED = 0x0F,   /* entry ends status 2 */
    CEPH_IDLE_STATUS1_COLLAPSE_BEACHED = 0x10,/* entry begins status 1 */
    CEPH_IDLE_STATUS1_END_BEACHED = 0x11,   /* entry ends status 1 */
    CEPH_IDLE_REST = 0x12,                  /* entry resets the notice */
    CEPH_IDLE_REST_END = 0x13,              /* entry refills need 3 */
    CEPH_IDLE_EAT_START = 0x14,             /* lowers its head, then CEPH_IDLE_EAT */
    CEPH_IDLE_STATUS4_SHAKE_OFF = 0x15,     /* entry ends status 4 */
    CEPH_IDLE_HOLD2_SHAKE_OFF = 0x16,       /* entry clears holdTimer2 */
    CEPH_IDLE_STUN_SHAKE_OFF = 0x17,        /* entry ends status 5 (the stun) */
    CEPH_IDLE_EAT = 0x28,                   /* entry starts the goal action */
    CEPH_IDLE_STATUS1_COLLAPSE = 0x29,      /* entry begins status 1 */
    CEPH_IDLE_STATUS1_END = 0x2A            /* entry ends status 1 */
};

/* Group 1, EM_GROUP_MOVE (EmCephadrome_RunMoveState). */
enum EmCephadromeMoveState {
    CEPH_MOVE_WALK = 0x00,                  /* walk to the goal (CEPH_MOVE_LIMP instead below a tenth of ... */
    CEPH_MOVE_RUN = 0x01,                   /* run to the goal, then stop */
    CEPH_MOVE_TURN = 0x02,                  /* turn toward the target on land */
    CEPH_MOVE_LIMP = 0x03                   /* limp to the goal (low hp) */
};

/*
 * Group 2, EM_GROUP_FLY (EmCephadrome_RunSandState): for this family,
 * swimming under the sand, surfacing and leaping.
 */
enum EmCephadromeSandState {
    CEPH_SAND_IDLE = 0x00,                  /* lies under the sand */
    CEPH_SAND_IDLE_B = 0x01,                /* sand idle, other motion */
    CEPH_SAND_CIRCLE_RIGHT_B = 0x02,        /* circle right, heading not wrapped */
    CEPH_SAND_TURN = 0x03,                  /* turn toward the target under the sand */
    CEPH_SAND_FORWARD = 0x04,               /* swim to the goal (speed scaled on entry) */
    CEPH_SAND_DASH = 0x05,                  /* fast swim to the goal with wakes (speed scaled on entry) */
    CEPH_SAND_CURVE = 0x06,                 /* curve set 0 */
    CEPH_SAND_RISE = 0x07,                  /* rise to the swim height under the sand top */
    CEPH_SAND_DIVE = 0x08,                  /* sink to the floor */
    CEPH_SAND_ROUND_TURN = 0x09,            /* turn to the next area point, then ROUND_NEXT */
    CEPH_SAND_ROUND_NEXT = 0x0A,            /* 4 legs round the area points, forward order (entry sets ... */
    CEPH_SAND_ROUND_PREV = 0x0B,            /* 4 legs round the area points, backward order (entry sets ... */
    CEPH_SAND_CIRCLE_RIGHT = 0x0C,          /* swims a circle turning right */
    CEPH_SAND_CIRCLE_LEFT = 0x0D,           /* swims a circle turning left */
    CEPH_SAND_LEAP_ASHORE = 0x0E,           /* leap out of the sand, always landing on the ground */
    CEPH_SAND_SURFACE_FLOP = 0x0F,          /* flop under the sand top */
    CEPH_SAND_LEAP_IN = 0x10,               /* leap from land into the sand (or onto land) */
    CEPH_SAND_LEAP = 0x11,                  /* leap out of the sand, back in or onto land */
    CEPH_SAND_LEAP_ON_LAND = 0x12,          /* crouch and leap on land, landing on the ground */
    CEPH_SAND_ROUND_NEXT_LEG = 0x13,        /* the following legs of ROUND_NEXT (no entry work, ... */
    CEPH_SAND_ROUND_PREV_LEG = 0x14,        /* the following legs of ROUND_PREV */
    CEPH_SAND_STATUS4_RECOVER = 0x15,       /* entry ends status 4 */
    CEPH_SAND_LEAP_TO_FIXED_POINT = 0x16,   /* leap to the fixed point (6000, -1000, 10000) */
    CEPH_SAND_EMPTY = 0x17,                 /* empty case, nothing runs */
    CEPH_SAND_STATUS2_END = 0x18,           /* entry ends status 2 */
    CEPH_SAND_STATUS1_END = 0x19,           /* entry ends status 1 */
    CEPH_SAND_TURN_NO_REACT = 0x1A,         /* SandTurn with noReact and the sense/hit/push timers */
    CEPH_SAND_DASH_NO_REACT = 0x1B          /* SandDash with noReact and the timers (also set on entry) */
};

/* Group 3, EM_GROUP_ATTACK (EmCephadrome_RunAttackState). */
enum EmCephadromeAttackState {
    CEPH_ATK_LAND_A = 0x00,                 /* land attack, CEPH_MOT_LAND_ATTACK_A */
    CEPH_ATK_BREATH = 0x01,                 /* ranged breath on land (entry sets unk013 1) */
    CEPH_ATK_SPIN = 0x02,                   /* turning spin on land */
    CEPH_ATK_SURFACE_BREATH_A = 0x03,       /* three-way breath from under the sand, yaw +40, 0, -40 ... */
    CEPH_ATK_SURFACE_BREATH_B = 0x04,       /* three-way breath from under the sand, yaw -40, 0, +40 ... */
    CEPH_ATK_SURFACE_BREATH_C = 0x05,       /* single breath from under the sand */
    CEPH_ATK_LEAP = 0x06,                   /* leap attack out of the sand */
    CEPH_ATK_LAND_B = 0x07,                 /* land attack, CEPH_MOT_LAND_ATTACK_B */
    CEPH_ATK_SURFACE_LUNGE = 0x08           /* lunge along the sand top */
};

/* Group 4, EM_GROUP_REACT (EmCephadrome_RunReactState). */
enum EmCephadromeReactState {
    CEPH_REACT_FLINCH = 0x00,               /* flinch, parts 0 and 7, or EM_DMG_HIT_MODE3 */
    CEPH_REACT_FLINCH_B = 0x01,             /* flinch, unbroken part 3 or 4 */
    CEPH_REACT_FLINCH_C = 0x02,             /* flinch, parts 5 and 6 */
    CEPH_REACT_FLINCH_D = 0x03,             /* flinch, parts 1 and 2 */
    CEPH_REACT_FLINCH_LYING = 0x04,         /* flinch while beached (stance EM_STANCE_FAMILY) */
    CEPH_REACT_KNOCK_DOWN = 0x05,           /* flinch of a broken part other than 3 (KnockDown variant 1) */
    CEPH_REACT_FALL = 0x06,                 /* falls out of the air (EM_DMG_STATUS_IN_AIR, a flinch in the ... */
    CEPH_REACT_STATUS4_LYING = 0x07,        /* status 4 while beached (begins status 4) */
    CEPH_REACT_EMPTY_A = 0x08,              /* empty case, nothing runs or enters it */
    CEPH_REACT_FALL_BLIND = 0x09,           /* blinded fall, no C entry */
    CEPH_REACT_FLINCH_BLIND = 0x0A,         /* blinded flinch on land, no C entry */
    CEPH_REACT_STATUS4 = 0x0B,              /* status 4 on land (begins status 4) */
    CEPH_REACT_EMPTY_B = 0x0C,              /* empty case */
    CEPH_REACT_EMPTY_C = 0x0D,              /* empty case */
    CEPH_REACT_EMPTY_D = 0x0E,              /* empty case */
    CEPH_REACT_EMPTY_E = 0x0F,              /* empty case */
    CEPH_REACT_LAND = 0x10,                 /* lands on the ground after a fall and gets up */
    CEPH_REACT_THROWN_OUT = 0x11,           /* thrown up out of the sand (a flinch, status 4 or status 2 ... */
    CEPH_REACT_THROWN_OUT_ATTACK5 = 0x12,   /* thrown out by EM_DMG_ATTACK5 under the sand (Cephadrome ... */
    CEPH_REACT_STATUS4_LYING_END = 0x13,    /* ends status 4, gets up from beached */
    CEPH_REACT_KNOCK_DOWN_PART3 = 0x14,     /* flinch of the broken part 3 (KnockDown variant 0) */
    CEPH_REACT_HOLD2 = 0x15,                /* EM_DMG_ATTACK13 (ends all statuses) */
    CEPH_REACT_STUN_A = 0x16,               /* the stun, hit from (hitAngle - rotY) above 0x8000 (begins ... */
    CEPH_REACT_STUN_B = 0x17                /* the stun, hit from the other side (begins the stun) */
};

/* Group 5, EM_GROUP_DEFEAT (EmCephadrome_RunDefeatState). */
enum EmCephadromeDefeatState {
    CEPH_DEFEAT_DIE = 0x00,                 /* killed on the ground: dies, mine point, fades */
    CEPH_DEFEAT_DIE_HELD = 0x01,            /* killed while held */
    CEPH_DEFEAT_DIE_UNDER = 0x02,           /* killed under the sand (or lying, or in the air below the ... */
    CEPH_DEFEAT_REAPPEAR = 0x04,            /* after the fade: back under the sand when the quest lets it, ... */
    CEPH_DEFEAT_DIE_FALLING = 0x05          /* killed in the air above the surface: falls, lands, dies */
};

/* Group 6, EM_GROUP_EVENT (EmCephadrome_RunEventState). */
enum EmCephadromeEventState {
    CEPH_EVENT_ENTRANCE = 0x00,             /* scripted entrance under the sand ending in a leap ashore */
    CEPH_EVENT_TO_IDLE = 0x01,              /* hidden one frame, then land idle */
    CEPH_EVENT_CAPTURED = 0x02              /* captured (begins status 1) */
};

#endif
