#ifndef EM_HERMITAUR_H
#define EM_HERMITAUR_H

/*
 * The Hermitaur family's vocabulary (src/game/em_hermitaur.c): Daimyo
 * Hermitaur, Shogun Ceanataur, Hermitaur and Ceanataur
 * (EM_KIND_DAIMYO_HERMITAUR, EM_KIND_SHOGUN_CEANATAUR, EM_KIND_HERMITAUR,
 * EM_KIND_CEANATAUR) share one set of state functions and one numbering of
 * motions and states. The two small kinds skip the ceiling, the needs and
 * the low-hp work and start buried (HERM_ATK_AMBUSH_WAIT).
 *
 * Motions: the four kinds share one motion table (gEmMotionRows[kind]),
 * whose row N plays motion N, so one number serves Em_SetMotion's row and
 * the motion id EM_MOTION_ID(layer, motion) kept in Em.motionId. The names
 * say what the states that play a motion show; motions no state plays
 * (0x0D-0x10, 0x12, 0x18, which have sounds only) stay numbers.
 */
enum EmHermitaurMotion {
    HERM_MOT_IDLE = 0x01,               /* standing; the versus poses end with it */
    HERM_MOT_WALK = 0x02,               /* walking to the goal on the root motion */
    HERM_MOT_WALK_BACK = 0x03,          /* walking backward to the goal */
    HERM_MOT_TURN_RIGHT = 0x05,         /* the turns in place (rotY falling) */
    HERM_MOT_TURN_LEFT = 0x06,          /* (rotY rising); also HERM_IDLE_ACTION_B alone */
    HERM_MOT_TURN_RIGHT_ALERT = 0x07,   /* the combat turns in place */
    HERM_MOT_TURN_LEFT_ALERT = 0x08,
    HERM_MOT_CEIL_HANG = 0x09,          /* the Shogun hanging from the ceiling (stance 2) */
    HERM_MOT_LEAP = 0x0A,               /* a leap: airborne at frame 98 (the Daimyo's leap press), or up to the ceiling */
    HERM_MOT_LEAP_LAND = 0x0B,          /* landing from a leap or a ceiling drop: impact attacks at frame 8 */
    HERM_MOT_CEIL_CRAWL = 0x0C,         /* moving along the ceiling to the goal (Em_StepToGoal) */
    HERM_MOT_STRIKE_C = 0x13,           /* a claw strike: the hit at frame 14 */
    HERM_MOT_SIDESTEP_A = 0x14,         /* a sidestep on the root motion (to the +X side) */
    HERM_MOT_STRIKE_B = 0x15,           /* a claw strike: the hit at frame 14 */
    HERM_MOT_SIDESTEP_B = 0x16,         /* (to the -X side) */
    HERM_MOT_STRIKE_B_MIRROR = 0x17,    /* HERM_MOT_STRIKE_B with the other claw */
    HERM_MOT_STRIKE_C_MIRROR = 0x19,    /* HERM_MOT_STRIKE_C with the other claw */
    HERM_MOT_LIE = 0x1F,                /* lying (the rest, statuses 1 and 2, captured) */
    HERM_MOT_GET_UP = 0x20,             /* getting up after HERM_MOT_LIE */
    HERM_MOT_LIE_DOWN = 0x21,           /* lying down: the rest, a status, the capture */
    HERM_MOT_EMERGE = 0x22,             /* coming up out of the ground */
    HERM_MOT_BURIED = 0x23,             /* under the ground: the shadow is hidden */
    HERM_MOT_CEIL_JET = 0x24,           /* the Shogun's jet from the ceiling (frame 50) */
    HERM_MOT_BACK_ATTACK_WALK = 0x25,   /* walking backward into the goal with a hit (HermitaurAttack 0x1B) */
    HERM_MOT_WATER_JET_END = 0x26,
    HERM_MOT_WATER_JET = 0x27,          /* the jet at frame 82, its hit after frame 86 */
    HERM_MOT_GUARD = 0x28,              /* the Daimyo's guard; for the Shogun a turn (EmHermitaur_TurnToGoal) */
    HERM_MOT_GUARD_END = 0x29,          /* the Daimyo's guard's end; for the Shogun the other turn */
    HERM_MOT_STRIKE_A = 0x2A,           /* a strike: the hit at frame 44 */
    HERM_MOT_WINDUP_STRIKE = 0x2B,      /* the strike after HERM_MOT_WINDUP: the hit at frame 18 */
    HERM_MOT_WINDUP = 0x2C,             /* looped before HERM_MOT_WINDUP_STRIKE */
    HERM_MOT_WALK_ALERT = 0x2D,
    HERM_MOT_WALK_BACK_ALERT = 0x2E,    /* with a hit behind (HermitaurAttack 9) */
    HERM_MOT_SPIN_A = 0x2F,             /* a full turn over frames 46-86, rotY falling: the hit at frame 76 */
    HERM_MOT_UNDER_THRUST = 0x30,       /* a thrust up out of the ground */
    HERM_MOT_SPIN_B = 0x31,             /* HERM_MOT_SPIN_A the other way */
    HERM_MOT_SHELL_SWAP = 0x32,         /* the Shogun changing its shell under the ground */
    HERM_MOT_IDLE_ALERT = 0x33,         /* standing alert, in combat */
    HERM_MOT_THREAT = 0x34,             /* the rage starts and the shake-offs */
    HERM_MOT_IDLE_ACTION_A = 0x35,      /* HERM_IDLE_ACTION's motions, by stateRand */
    HERM_MOT_IDLE_ACTION_B = 0x36,
    HERM_MOT_EAT = 0x37,                /* eating */
    HERM_MOT_BURROW = 0x39,             /* digging in: the shadow hidden from frame 130 */
    HERM_MOT_CEIL_GRAB = 0x3A,          /* reaching the ceiling (Em.surfaceY) */
    HERM_MOT_CEIL_DROP = 0x3B,          /* letting go of the ceiling: 400 lower at frame 108 */
    HERM_MOT_FLINCH = 0x3C,
    HERM_MOT_FLINCH_CLAW = 0x3D,        /* a claw's flinch */
    HERM_MOT_KNOCK_DOWN = 0x3F,         /* knocked down on side A */
    HERM_MOT_DOWN = 0x40,               /* lying on side A */
    HERM_MOT_HELD = 0x41,               /* held: status 4 and the second hold, until the timer runs out */
    HERM_MOT_FALL = 0x42,               /* falling off the ceiling */
    HERM_MOT_FALL_LAND = 0x43,
    HERM_MOT_DIE = 0x44,
    HERM_MOT_SPIN_FLINCH = 0x45,        /* a full turn over frames 82-156: the shell's flinch and the part break */
    HERM_MOT_DOWN_GET_UP = 0x46,        /* getting up from side A */
    HERM_MOT_KNOCK_DOWN_B = 0x47,       /* knocked down on side B */
    HERM_MOT_DOWN_B = 0x48,
    HERM_MOT_DOWN_B_GET_UP = 0x49,
    HERM_MOT_RECOIL = 0x4A,             /* thrown back: a guarded wind hit, a broken claw's flinch */
    HERM_MOT_RECOIL_END = 0x4B,
    HERM_MOT_FLINCH_END = 0x4C          /* after HERM_MOT_FLINCH */
};

/* Group 0, EM_GROUP_IDLE (EmHermitaur_RunIdleState). In combat,
 * EmHermitaur_ChangeState turns HERM_IDLE into HERM_IDLE_ALERT and
 * HERM_IDLE_WAIT into HERM_IDLE_WAIT_ALERT. */
enum EmHermitaurIdleState {
    HERM_IDLE_STAND = 0x00,
    HERM_IDLE = 0x01,                   /* where EmHermitaur_EndToIdle goes */
    HERM_IDLE_ALERT = 0x02,             /* where EmHermitaur_EndToIdleAlert goes */
    HERM_IDLE_FEED = 0x03,              /* eating with no gain (the Daimyo leaves a mine point) */
    HERM_IDLE_THREAT = 0x04,
    HERM_IDLE_ACTION = 0x05,            /* HERM_MOT_IDLE_ACTION_A or _B */
    HERM_IDLE_ACTION_B = 0x06,          /* HERM_MOT_TURN_LEFT played once, no turn */
    HERM_IDLE_TURN_TO_GOAL = 0x07,
    HERM_IDLE_STATUS2_COLLAPSE = 0x08,  /* Em_BeginStatus2 */
    HERM_IDLE_STATUS2_END = 0x09,       /* Em_EndStatus2 */
    HERM_IDLE_STATUS4_SHAKE_OFF = 0x0A, /* Em_EndStatus4 */
    HERM_IDLE_STUN_SHAKE_OFF = 0x0B,    /* Em_EndStatus5 */
    HERM_IDLE_HOLD2_SHAKE_OFF = 0x0C,   /* holdTimer2 cleared */
    HERM_IDLE_REST = 0x0D,              /* lying 5400 frames, hp coming back */
    HERM_IDLE_STATUS1_COLLAPSE = 0x0E,  /* Em_BeginStatus1 */
    HERM_IDLE_STATUS1_END = 0x0F,       /* Em_EndStatus1 */
    HERM_IDLE_REST_END = 0x10,          /* Em_RefillNeed3 */
    HERM_IDLE_THREAT_RAGE = 0x11,       /* Em_StartRage */
    HERM_IDLE_THREAT_PENDING_RAGE = 0x12, /* Em_StartPendingRage */
    HERM_IDLE_WAIT = 0x13,              /* 30 frames; also HERM_DIG_CEIL_CLIMB's fallback with no ceiling */
    HERM_IDLE_WAIT_ALERT = 0x14,
    HERM_IDLE_EAT_START = 0x15,         /* the eating motion to frame 476, with sight row and reaction set 1 */
    HERM_IDLE_EAT_END = 0x16,           /* Em_StartGoalAction on entry; the rest of the motion */
    HERM_IDLE_FEED_REFILL = 0x17        /* HERM_IDLE_FEED with 5% hp and need 1 filled */
};

/* Group 1, EM_GROUP_MOVE (EmHermitaur_RunMoveState). In combat,
 * EmHermitaur_ChangeState turns the walks and turns into their _ALERT forms
 * (TURN_TO_GOAL, WALK, TURN_AWAY, WALK_BACK, TURN_SIDE_A and _B). Sides A
 * and B are the monster's +X and -X sides. */
enum EmHermitaurMoveState {
    HERM_MOVE_TURN_TO_GOAL = 0x00,
    HERM_MOVE_WALK = 0x01,              /* stops 300 short of the goal */
    HERM_MOVE_WALK_ALERT_FAST = 0x02,
    HERM_MOVE_TURN_AWAY = 0x03,         /* to the goal's angle plus a half turn */
    HERM_MOVE_WALK_BACK = 0x04,         /* backward, 400 past the goal */
    HERM_MOVE_WALK_BACK_ALERT_FAST = 0x05,
    HERM_MOVE_SIDESTEP_A = 0x06,        /* stops 450 short (200 for the small kinds) */
    HERM_MOVE_SIDESTEP_B = 0x07,
    HERM_MOVE_TURN_SIDE_A = 0x08,       /* to the goal's angle plus a quarter turn */
    HERM_MOVE_TURN_SIDE_B = 0x09,       /* plus three quarters */
    HERM_MOVE_SIDESTEP_FAST_A = 0x0A,
    HERM_MOVE_SIDESTEP_FAST_B = 0x0B,
    HERM_MOVE_SIDESTEP_SHORT_A = 0x0C,  /* to a point 300 to side A */
    HERM_MOVE_SIDESTEP_SHORT_B = 0x0D,
    HERM_MOVE_SIDESTEP_LONG_A = 0x0E,   /* 600 */
    HERM_MOVE_SIDESTEP_LONG_B = 0x0F,
    HERM_MOVE_SIDESTEP_SHORT_C = 0x10,  /* as HERM_MOVE_SIDESTEP_SHORT_A */
    HERM_MOVE_SIDESTEP_SHORT_D = 0x11,  /* as HERM_MOVE_SIDESTEP_SHORT_B */
    HERM_MOVE_WALK_ALERT = 0x12,
    HERM_MOVE_WALK_BACK_ALERT = 0x13,
    HERM_MOVE_TURN_TO_GOAL_ALERT = 0x14,
    HERM_MOVE_TURN_AWAY_ALERT = 0x15,
    HERM_MOVE_TURN_SIDE_A_ALERT = 0x16,
    HERM_MOVE_TURN_SIDE_B_ALERT = 0x17,
    HERM_MOVE_BACK_ATTACK = 0x18        /* HERM_MOT_BACK_ATTACK_WALK */
};

/* Group 2, EM_GROUP_FLY (EmHermitaur_RunDigState): under the ground (the
 * buried states and the Shogun's shell swap) and the Shogun's ceiling
 * (stance 2 at Em.surfaceY with Em.ceilingTimer kept at 5). */
enum EmHermitaurDigState {
    HERM_DIG_BURROW = 0x00,
    HERM_DIG_TURN_TO_GOAL = 0x01,
    HERM_DIG_MOVE_TO_GOAL = 0x02,
    HERM_DIG_BURIED = 0x03,             /* where EmHermitaur_EndToBuried goes */
    HERM_DIG_CEIL_CLIMB = 0x04,         /* HERM_IDLE_WAIT where there is no ceiling */
    HERM_DIG_CEIL_HANG = 0x05,          /* where EmHermitaur_EndToCeiling goes */
    HERM_DIG_CEIL_DROP = 0x06,
    HERM_DIG_CEIL_TURN = 0x07,
    HERM_DIG_CEIL_CRAWL = 0x08,
    HERM_DIG_SHELL_SWAP = 0x09,         /* HERM_DIG_BURIED with the shell whole, else one of _A, _B, _C */
    HERM_DIG_MOVE_THROUGH = 0x0A,       /* HERM_DIG_MOVE_TO_GOAL without the field's collision, unhittable */
    HERM_DIG_CEIL_CRAWL_FAST = 0x0B,
    HERM_DIG_SHELL_SWAP_A = 0x0C,       /* shell (Em.variant) 0 */
    HERM_DIG_SHELL_SWAP_B = 0x0D,       /* 1 */
    HERM_DIG_SHELL_SWAP_C = 0x0E        /* 2 */
};

/* Group 3, EM_GROUP_ATTACK (EmHermitaur_RunAttackState): picked by the script. */
enum EmHermitaurAttackState {
    HERM_ATK_STRIKE_A = 0x00,
    HERM_ATK_WINDUP_STRIKE = 0x01,      /* one wind-up */
    HERM_ATK_WINDUP_STRIKE_MID = 0x02,  /* five */
    HERM_ATK_WATER_JET = 0x03,          /* the Daimyo and Ceanataur */
    HERM_ATK_GUARD = 0x04,              /* the Daimyo, 360 frames (Em.hermitTimer) */
    HERM_ATK_GUARD_END = 0x05,
    HERM_ATK_WINDUP_STRIKE_LONG = 0x06, /* ten wind-ups */
    HERM_ATK_BURROW = 0x07,             /* then HERM_ATK_AMBUSH_WAIT */
    HERM_ATK_AMBUSH_THRUST = 0x08,      /* a thrust from under the ground, then HERM_ATK_EMERGE */
    HERM_ATK_EMERGE = 0x09,
    HERM_ATK_LEAP_LOW = 0x0A,           /* the Daimyo's leap press */
    HERM_ATK_LEAP_MID = 0x0B,
    HERM_ATK_LEAP_HIGH = 0x0C,
    HERM_ATK_STRIKE_B = 0x0D,
    HERM_ATK_STRIKE_B_MIRROR = 0x0E,
    HERM_ATK_STRIKE_C = 0x0F,
    HERM_ATK_STRIKE_C_MIRROR = 0x10,
    HERM_ATK_AMBUSH_WAIT = 0x11,        /* buried until a player comes within 200 or the timer runs out */
    HERM_ATK_THRUST_SERIES_A = 0x12,    /* thrusts at EmHermitaur_ThrustOffsets row 0 */
    HERM_ATK_THRUST_RETURN = 0x13,      /* one thrust, back under the ground */
    HERM_ATK_THRUST_SERIES_B = 0x14,    /* row 1 */
    HERM_ATK_THRUST_ONCE = 0x15,        /* row 2: one thrust in place */
    HERM_ATK_CEIL_JET = 0x16,           /* the Shogun, on the ceiling */
    HERM_ATK_TURN_TO_GOAL = 0x17,       /* the Shogun */
    HERM_ATK_TURN_TO_GOAL_B = 0x18,     /* the same */
    HERM_ATK_GUARD_SHORT = 0x19,        /* the Daimyo, 180 frames */
    HERM_ATK_SPIN_A = 0x1A,             /* the Shogun */
    HERM_ATK_SPIN_B = 0x1B
};

/* Group 4, EM_GROUP_REACT (EmHermitaur_RunReactState): what EmHermitaur_Update
 * picks from Em_UpdateDamage's result. A flinch by part: 0, 1, 2 and 7 the
 * body, 3 and 4 the knock-downs, 5 and 6 the claws (Em.flinchPart). */
enum EmHermitaurReactState {
    HERM_REACT_FLINCH = 0x00,
    HERM_REACT_FLINCH_CLAW = 0x01,      /* a claw not broken */
    HERM_REACT_KNOCK_DOWN_A = 0x02,     /* part 3 */
    HERM_REACT_KNOCK_DOWN_B = 0x03,     /* part 4 */
    HERM_REACT_STATUS4 = 0x04,
    HERM_REACT_SPIN_FLINCH = 0x05,      /* the Shogun's broken shell */
    HERM_REACT_GUARD_RECOIL = 0x06,     /* EM_DMG_ATTACK5 on the Daimyo's guard */
    HERM_REACT_WIND_FLINCH = 0x07,      /* EM_DMG_ATTACK5 on the small kinds */
    HERM_REACT_HOLD2 = 0x08,            /* EM_DMG_ATTACK13 */
    HERM_REACT_STUN_A = 0x09,           /* hit on one side */
    HERM_REACT_STUN_B = 0x0A,           /* the other */
    HERM_REACT_RECOIL_CLAW_A = 0x0B,    /* the Shogun's damaged claw (part 5) */
    HERM_REACT_FALL = 0x0C,             /* off the ceiling */
    HERM_REACT_PART_BREAK = 0x0D,       /* EM_DMG_PART8_DOWN */
    HERM_REACT_RECOIL_CLAW_B = 0x0E,    /* part 6 */
    HERM_REACT_FLINCH_CLAW_BROKEN_A = 0x0F, /* the Daimyo's broken claw (part 5) */
    HERM_REACT_FLINCH_CLAW_BROKEN_B = 0x10  /* part 6 */
};

/* Group 5, EM_GROUP_DEFEAT (EmHermitaur_RunDefeatState). */
enum EmHermitaurDefeatState {
    HERM_DEFEAT_DIE = 0x00,
    HERM_DEFEAT_REAPPEAR = 0x01         /* QuestEm_Appear, then HERM_EVENT2_EMERGE */
};

/* Group 6, EM_GROUP_EVENT (EmHermitaur_RunEventState). */
enum EmHermitaurEventState {
    HERM_EVENT_LEAVE = 0x00,
    HERM_EVENT_END = 0x01,
    HERM_EVENT_DAIMYO_INTRO = 0x02,     /* the Daimyo's first sighting (ScEvent_DaimyoHermitaurSighting) */
    HERM_EVENT_SHOGUN_INTRO = 0x03,     /* the Shogun's first sighting */
    HERM_EVENT_CAPTURED = 0x04          /* EM_DMG_CAPTURED */
};

/* Group 7, EM_GROUP_EVENT2 (EmHermitaur_RunEvent2State). */
enum EmHermitaurEvent2State {
    HERM_EVENT2_EMERGE = 0x00           /* after HERM_DEFEAT_REAPPEAR */
};

/* Group 8, EM_GROUP_VERSUS (EmHermitaur_RunVersusState). */
enum EmHermitaurVersusState {
    HERM_VS_EAT_BAIT = 0x00,
    HERM_VS_POSE_A = 0x01,
    HERM_VS_POSE_B = 0x02,
    HERM_VS_POSE_C = 0x03,
    HERM_VS_WAIT_BOTH = 0x98,           /* Versus_MarkDefeat */
    HERM_VS_END = 0x99
};

#endif /* EM_HERMITAUR_H */
