#ifndef EM_KUTKU_H
#define EM_KUTKU_H

/*
 * The Kut-Ku family's vocabulary (src/game/em_kutku.c): Yian Kut-Ku, Blue
 * Yian Kut-Ku, Gypceros and Purple Gypceros (EM_KIND_YIAN_KUT_KU,
 * EM_KIND_BLUE_YIAN_KUT_KU, EM_KIND_GYPCEROS, EM_KIND_PURPLE_GYPCEROS) share
 * one set of state functions and one numbering of motions and states; the
 * code tests the kind where they differ (the Gypceros feigns death and
 * flashes, the Kut-Ku flinches from attack kind 5 and rages).
 *
 * Motions: row N of the family's motion table plays motion N, so one number
 * serves Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion)
 * kept in Em.motionId. The names say what the states that play a motion
 * show; motions no state plays stay numbers.
 */
enum EmKutkuMotion {
    KUTKU_MOT_IDLE = 0x01,          /* the idle loop; layers 0-2 idle with EM_MOTION_ID(layer, 1) */
    KUTKU_MOT_WALK = 0x03,          /* KUTKU_MOVE_WALK; forward motion of Em_TurnToTargetMotion in group 3 */
    KUTKU_MOT_TURN_LEFT = 0x06,     /* intro: rotY offset -0x4000 and turned back over half its length */
    KUTKU_MOT_LIMP = 0x0A,          /* KUTKU_MOVE_LIMP (below 20% hp) */
    KUTKU_MOT_DESCEND = 0x0B,       /* the last stretch of a landing (KUTKU_FLY_LAND, HOP_TO_GOAL) */
    KUTKU_MOT_GLIDE = 0x0C,         /* entering a flight (FlyRoute, GlideToGoal*, GlideOnce, the swoops start) */
    KUTKU_MOT_FLY = 0x0E,           /* flying forward after the glide (FlyRoute, FlyRouteLoop) and away after a swoop */
    KUTKU_MOT_HOVER = 0x0F,         /* hovering (HoverToGoal, HoverOnce, HoverTurn, HoverRise, OutOfArea, ThroughArea, LevelOutAndLand) */
    KUTKU_MOT_RUN = 0x11,           /* ChargeTrip run to the goal (also KUTKU_MOVE_RUN) */
    KUTKU_MOT_TAKE_OFF = 0x12,      /* KUTKU_FLY_TAKE_OFF / HOP_TO_GOAL: EM_STANCE_AIR from frame 30 or 38 */
    KUTKU_MOT_TOUCH_DOWN = 0x13,    /* intro: EM_STANCE_GROUND when below the floor */
    KUTKU_MOT_UPPER_D = 0x14,       /* layer-1 gesture of KUTKU_IDLE_UPPER_D */
    KUTKU_MOT_UPPER_A = 0x18,       /* layer-1 gesture of KUTKU_IDLE_UPPER_A */
    KUTKU_MOT_UPPER_B = 0x19,       /* layer-1 gesture of KUTKU_IDLE_UPPER_B / _QUIET */
    KUTKU_MOT_VS_BAIT = 0x1A,       /* only VS_EAT_BAIT: the bait appears at its frame 10 */
    KUTKU_MOT_UPPER_C = 0x1C,       /* layer-1 gesture of KUTKU_IDLE_UPPER_C */
    KUTKU_MOT_IDLE_ACTION_C = 0x1D, /* KUTKU_IDLE_ACTION_C only */
    KUTKU_MOT_REST = 0x1F,          /* the rest loop of KUTKU_IDLE_REST (hp regen) */
    KUTKU_MOT_REST_LIE_DOWN = 0x20, /* start of KUTKU_IDLE_REST */
    KUTKU_MOT_REST_GET_UP = 0x21,   /* KUTKU_IDLE_REST_END */
    KUTKU_MOT_LIE = 0x22,           /* lying under statuses 1/2 (STATUS1/2_COLLAPSE, STATUS2_LIE_DOWN) */
    KUTKU_MOT_STARE = 0x24,         /* TurnAndStare, Stare; effects KutkuAttack 0x21 (f46) and 1 (f50) */
    KUTKU_MOT_GET_UP = 0x25,        /* STATUS1_END, STATUS2_END */
    KUTKU_MOT_COLLAPSE = 0x26,      /* CapturedHold2 start (status 1 begun) */
    KUTKU_MOT_LIE_DOWN = 0x27,      /* STATUS2_LIE_DOWN */
    KUTKU_MOT_LEAP = 0x28,          /* Leap, airborne from frame 60; KutkuAttack 0x1C at frame 60 */
    KUTKU_MOT_LEAP_LAND = 0x29,     /* Leap and SwoopThenLand landings; KutkuAttack 0x1D, dust */
    KUTKU_MOT_TAIL_SPIN = 0x2A,     /* TailSpin, rotY + 0x200; KutkuAttack 0x11 at frame 30 */
    KUTKU_MOT_SWOOP_DIVE = 0x2C,    /* the swoops start, aimed at the target */
    KUTKU_MOT_SWOOP_CLAW = 0x2D,    /* EmKutku_SwoopClawIfNear near the target; KutkuAttack 0x14 and 0x15 at frame 22 */
    KUTKU_MOT_SPIT = 0x2F,          /* Spit and SpitAfterFlinch; projectile at the table frame, KutkuAttack 0x22 at 30 */
    KUTKU_MOT_ALERT_IDLE = 0x33,    /* intro, VS_EAT_BAIT end, KUTKU_IDLE_ALERT (combat idle) */
    KUTKU_MOT_BACK_HOP = 0x34,      /* BackHop, airborne from frame 76 */
    KUTKU_MOT_TURN_IN_PLACE = 0x36, /* KUTKU_MOVE_TURN_TO_GOAL (rotY 0x200 a frame to frame 60); also KUTKU_VS_POSE_C */
    KUTKU_MOT_BACK_HOP_LAND = 0x39, /* BackHop landing; KutkuAttack 0x1F, 0x1B, 0x20 */
    KUTKU_MOT_FEED = 0x3B,          /* after the forage motion in the eat states; +5% hp, need gauges refilled */
    KUTKU_MOT_FLINCH = 0x3C,        /* REACT_HOLD2 start (no root motion to frame 52), KUTKU_REACT_FLINCH */
    KUTKU_MOT_FLINCH_E = 0x3D,      /* FlinchE */
    KUTKU_MOT_SHAKE_OFF = 0x3E,     /* KUTKU_IDLE_SHAKE_OFF and the three status shake-offs */
    KUTKU_MOT_FLINCH_C = 0x3F,      /* FlinchC and Part6Break */
    KUTKU_MOT_FLINCH_D = 0x40,      /* FlinchD */
    KUTKU_MOT_FLINCH_B = 0x42,      /* FlinchB */
    KUTKU_MOT_DIE = 0x44,           /* falling dead (KUTKU_DEFEAT_DIE) */
    KUTKU_MOT_KNOCK_DOWN = 0x45,    /* REACT_STUN start, stun stars */
    KUTKU_MOT_DOWN = 0x46,          /* REACT_STUN until the timer */
    KUTKU_MOT_DOWN_GET_UP = 0x47,   /* after KUTKU_MOT_DOWN and after a fall (FallFromAir) */
    KUTKU_MOT_DOWN_GET_UP_B = 0x48, /* after KUTKU_MOT_DOWN_B (KnockDown) */
    KUTKU_MOT_KNOCK_DOWN_B = 0x4A,  /* KnockDown (broken part other than 3) */
    KUTKU_MOT_DOWN_B = 0x4B,        /* lying 120 frames after KUTKU_MOT_KNOCK_DOWN_B */
    KUTKU_MOT_STATUS4 = 0x4C,       /* KUTKU_REACT_STATUS4 (status 4 effect) and REACT_HOLD2 until the timer */
    KUTKU_MOT_FALL = 0x4D,          /* falling out of the air (AirFlinch, FallFromAir*) */
    KUTKU_MOT_FALL_LAND = 0x4E,     /* DEFEAT_DIE_FALLING on the floor */
    KUTKU_MOT_FORAGE = 0x4F,        /* first motion of the eat states: dust clouds (DustCloud_Create kinds 6-0x10) at its frames */
    KUTKU_MOT_IDLE_PAIR_A = 0x50,   /* idle PAIR_A/B/C, ACTION_D; nothing tells it from KUTKU_MOT_IDLE_PAIR_B */
    KUTKU_MOT_IDLE_PAIR_B = 0x51,   /* idle PAIR_A/B/C */
    KUTKU_MOT_DIE_FALLING = 0x52,   /* DEFEAT_DIE_FALLING after landing */
    KUTKU_MOT_HOP_IN_PLACE = 0x54,  /* HopInPlace, airborne from frame 36 */
    KUTKU_MOT_SWOOP = 0x57,         /* the swoops descent after KUTKU_MOT_SWOOP_DIVE, velZ 100 */
    KUTKU_MOT_LEVEL_OUT = 0x5A,     /* LevelOut*, rotX and rotZ stepped to 0 */
    KUTKU_MOT_CHARGE_TRIP = 0x5B,   /* ChargeTrip end with KutkuAttack 0x24 */
    KUTKU_MOT_TRAPPED = 0x5F,       /* KUTKU_REACT_TRAPPED (pitfall) */
    KUTKU_MOT_STRUGGLE = 0x60,      /* TrapStruggle, Status4DownedEnd, looped until trapTimer ends */
    KUTKU_MOT_DOWNED = 0x61,        /* STATUS1/2_DOWNED with lying puffs and a periodic sound; also group-4 and group-8 states */
    KUTKU_MOT_DOWNED_GET_UP = 0x62, /* Status2/1DownedEnd */
    KUTKU_MOT_THREAT = 0x64,        /* IDLE_DISPLAY (and its rage / pending-rage forms), KUTKU_REACT_ATTACK5 after KUTKU_MOT_ATTACK5_FLINCH */
    KUTKU_MOT_TURN_TO_GOAL = 0x65,  /* KUTKU_IDLE_TURN_TO_GOAL (aimAngle to the goal, snaps rotY), end of MOVE_WALK_B_AND_TURN */
    KUTKU_MOT_PECK_COMBO = 0x66,    /* PeckCombo, PeckSteal (also KUTKU_IDLE_ACTION_B); four KutkuAttack 0x18 */
    KUTKU_MOT_WALK_B = 0x68,        /* KUTKU_MOVE_WALK_B / WALK_B_AND_TURN, Em_StepToGoal */
    KUTKU_MOT_SPIT_VOLLEY = 0x69,   /* MotionEffects: four ranged shots from bone 0x22 (frames 78/150/240/296) */
    KUTKU_MOT_PECK_COMBO_B = 0x6A,  /* PeckComboB (also KUTKU_IDLE_ACTION_A); KutkuAttack 0x12 then four 0x18 */
    KUTKU_MOT_FLASH = 0x6B,         /* Gypceros: MotionEffects: sets crestGlow */
    KUTKU_MOT_ATTACK5_FLINCH = 0x6C, /* Kut-Ku KUTKU_REACT_ATTACK5 (EM_DMG_ATTACK5, not Gypceros, then enrage); Kut-Ku VS_POSE_A */
    KUTKU_MOT_FEIGN_DEATH_RISE = 0x6D, /* KUTKU_ATK_FEIGN_DEATH_RISE/KUTKU_MOT_REST_LIE_DOWN after feigned death */
    KUTKU_MOT_FEIGN_DEATH_LIE = 0x6E /* FeignDeathLie, lying still; no effects case */
};

/* Group 0, EM_GROUP_IDLE (EmKutku_RunIdleState): standing, idle actions, eating, resting and lying under a status. */
enum EmKutkuIdleState {
    KUTKU_IDLE_STAND = 0x00,               /* idle on all layers, waits em->timer */
    KUTKU_IDLE = 0x01,                     /* idle motion once; Start/ToIdle go here; becomes KUTKU_IDLE_ALERT in combat */
    KUTKU_IDLE_ACTION_A = 0x02,            /* KUTKU_MOT_PECK_COMBO_B once */
    KUTKU_IDLE_ACTION_B = 0x03,            /* KUTKU_MOT_PECK_COMBO once */
    KUTKU_IDLE_ONE_LOOP = 0x04,            /* one idle loop on layer 0, layers 1/2 kept idle */
    KUTKU_IDLE_UPPER_A = 0x05,             /* layer-1 gesture KUTKU_MOT_UPPER_A */
    KUTKU_IDLE_UPPER_B = 0x06,             /* layer-1 gesture KUTKU_MOT_UPPER_B with sounds */
    KUTKU_IDLE_UPPER_B_QUIET = 0x07,       /* layer-1 gesture KUTKU_MOT_UPPER_B, no sounds, lookAtTarget kept */
    KUTKU_IDLE_HOLD2_SHAKE_OFF = 0x08,     /* EmKutku_SetIdleState clears holdTimer2 */
    KUTKU_IDLE_PAIR_A = 0x09,              /* KUTKU_MOT_IDLE_PAIR_A or KUTKU_MOT_IDLE_PAIR_B at random */
    KUTKU_IDLE_TURN_TO_GOAL = 0x0A,        /* KUTKU_MOT_TURN_TO_GOAL aimed at the goal */
    KUTKU_IDLE_UPPER_C = 0x0B,             /* layer-1 gesture KUTKU_MOT_UPPER_C */
    KUTKU_IDLE_ACTION_C = 0x0C,            /* KUTKU_MOT_IDLE_ACTION_C once */
    KUTKU_IDLE_UPPER_D = 0x0D,             /* layer-1 gesture KUTKU_MOT_UPPER_D */
    KUTKU_IDLE_EAT_NEED2 = 0x0E,           /* forage + feed, need2Gauge raised, +5% hp */
    KUTKU_IDLE_PAIR_B = 0x0F,              /* KUTKU_MOT_IDLE_PAIR_A then KUTKU_MOT_IDLE_PAIR_B */
    KUTKU_IDLE_PAIR_C = 0x10,              /* KUTKU_MOT_IDLE_PAIR_B then KUTKU_MOT_IDLE_PAIR_A */
    KUTKU_IDLE_ALERT = 0x11,               /* combat idle, KUTKU_MOT_ALERT_IDLE */
    KUTKU_IDLE_REST = 0x12,                /* lie down KUTKU_MOT_REST_LIE_DOWN, rest KUTKU_MOT_REST, hp regen */
    KUTKU_IDLE_EAT_START = 0x13,           /* forage KUTKU_MOT_FORAGE to frame 200, then KUTKU_IDLE_EAT */
    KUTKU_IDLE_STATUS2_COLLAPSE = 0x14,    /* Em_BeginStatus2; EM_DMG_STATUS2 (case 8) and holdTimer2 */
    KUTKU_IDLE_STATUS2_LIE_DOWN = 0x15,    /* Em_BeginStatus2; lie down KUTKU_MOT_LIE_DOWN, hp regen like the rest */
    KUTKU_IDLE_ACTION_D = 0x16,            /* KUTKU_MOT_IDLE_PAIR_A once */
    KUTKU_IDLE_REST_END = 0x17,            /* get up KUTKU_MOT_REST_GET_UP after the rest; Em_RefillNeed3 */
    KUTKU_IDLE_STATUS2_END = 0x18,         /* Em_EndStatus2; get up KUTKU_MOT_GET_UP */
    KUTKU_IDLE_SHAKE_OFF = 0x19,           /* shake-off KUTKU_MOT_SHAKE_OFF after every get-up */
    KUTKU_IDLE_EAT_NEED1 = 0x1A,           /* forage + feed, need1Gauge refilled */
    KUTKU_IDLE_STATUS1_COLLAPSE = 0x1B,    /* Em_BeginStatus1; EM_DMG_CAPTURED path (case 7) when not held */
    KUTKU_IDLE_STATUS1_END = 0x1C,         /* Em_EndStatus1; get up KUTKU_MOT_GET_UP */
    KUTKU_IDLE_STATUS2_DOWNED = 0x1D,      /* Em_BeginStatus2 with holdTimer set */
    KUTKU_IDLE_STATUS1_DOWNED = 0x1F,      /* Em_BeginStatus1; downed KUTKU_MOT_DOWNED, then KUTKU_REACT_STATUS1_DOWNED_END */
    KUTKU_IDLE_DISPLAY = 0x21,             /* KUTKU_MOT_THREAT once */
    KUTKU_IDLE_STATUS4_SHAKE_OFF = 0x22,   /* Em_EndStatus4; entered from KUTKU_REACT_STATUS4 */
    KUTKU_IDLE_DISPLAY_RAGE = 0x23,        /* Em_StartRage; KUTKU_MOT_THREAT */
    KUTKU_IDLE_STUN_SHAKE_OFF = 0x24,      /* Em_EndStatus5 (the stun); entered from KUTKU_REACT_STUN */
    KUTKU_IDLE_DISPLAY_PENDING_RAGE = 0x25, /* Em_StartPendingRage; KUTKU_MOT_THREAT */
    KUTKU_IDLE_EAT = 0x28                  /* Em_StartGoalAction(0x2710); feed KUTKU_MOT_FEED after EAT_START */
};

/* Group 1, EM_GROUP_MOVE (EmKutku_RunMoveState): walking, running, limping and turning. */
enum EmKutkuMoveState {
    KUTKU_MOVE_WALK = 0x00,           /* walk KUTKU_MOT_WALK; becomes KUTKU_MOVE_LIMP below 20% hp */
    KUTKU_MOVE_END = 0x01,            /* EmKutku_SetMoveState only sets scriptPending and returns */
    KUTKU_MOVE_WALK_B = 0x02,         /* walk KUTKU_MOT_WALK_B */
    KUTKU_MOVE_TURN = 0x03,           /* EmKutku_TurnParams[0]; becomes KUTKU_MOVE_TURN_IN_COMBAT in combat */
    KUTKU_MOVE_RUN = 0x04,            /* run KUTKU_MOT_RUN */
    KUTKU_MOVE_TURN_IN_COMBAT = 0x05, /* EmKutku_TurnParams[1] */
    KUTKU_MOVE_LIMP = 0x06,           /* limp KUTKU_MOT_LIMP, timer capped at 300 */
    KUTKU_MOVE_TURN_TO_GOAL = 0x07,   /* KUTKU_MOT_TURN_IN_PLACE, 0x200 a frame */
    KUTKU_MOVE_RUN_SLOW = 0x08,       /* KUTKU_MOVE_RUN at motionSpeed 0.8 */
    KUTKU_MOVE_NOP = 0x09,            /* empty function */
    KUTKU_MOVE_WALK_B_AND_TURN = 0x0A /* walk KUTKU_MOT_WALK_B then turn KUTKU_MOT_TURN_TO_GOAL */
};

/* Group 2, EM_GROUP_FLY (EmKutku_RunFlyState): take-offs, hops, flight, hovering and landings. */
enum EmKutkuFlyState {
    KUTKU_FLY_TAKE_OFF = 0x00,              /* KUTKU_MOT_TAKE_OFF, EM_STANCE_AIR from frame 38, then KUTKU_FLY_HOVER_ONCE */
    KUTKU_FLY_LAND = 0x01,                  /* hover, descend, touch down */
    KUTKU_FLY_HOVER = 0x02,                 /* hover KUTKU_MOT_HOVER turning to the goal; the air state of EmKutku_ToIdle */
    KUTKU_FLY_HOP_TO_GOAL = 0x03,           /* take off, turn to goal, land again */
    KUTKU_FLY_HOP_FORWARD = 0x04,           /* KUTKU_MOT_TAKE_OFF then KUTKU_MOT_TOUCH_DOWN, velZ 20 */
    KUTKU_FLY_HOP_BACK = 0x05,              /* KUTKU_MOT_TAKE_OFF then KUTKU_MOT_TOUCH_DOWN, velZ -20 */
    KUTKU_FLY_ROUTE = 0x06,                 /* legsLeft 4 goals, then the glide */
    KUTKU_FLY_HOVER_TO_GOAL = 0x07,         /* hover, turn and pitch to the goal */
    KUTKU_FLY_OUT_OF_AREA = 0x08,           /* rise, glide out, Em_EnterNextArea */
    KUTKU_FLY_TO_GOAL_AND_LAND = 0x09,      /* velZ 80 to the goal, then KUTKU_FLY_LAND */
    KUTKU_FLY_END_TO_GLIDE = 0x0A,          /* EmKutku_EndStateInAir(em, 1) only */
    KUTKU_FLY_GLIDE_TO_GOAL_AND_LAND = 0x0B, /* glide to the goal, then KUTKU_FLY_LAND */
    KUTKU_FLY_BACK_HOP = 0x0C,              /* KUTKU_MOT_BACK_HOP, KUTKU_MOT_BACK_HOP_LAND with dust */
    KUTKU_FLY_THROUGH_AREA = 0x0D,          /* entry point, exit point, next area */
    KUTKU_FLY_HOVER_ONCE = 0x0E,            /* KUTKU_MOT_HOVER once, EmKutku_EndStateInAir(em, 0) */
    KUTKU_FLY_GLIDE_ONCE = 0x0F,            /* KUTKU_MOT_GLIDE once, EmKutku_EndStateInAir(em, 1) */
    KUTKU_FLY_HOVER_TURN = 0x10,            /* Em_TurnToGoal up to 150 frames */
    KUTKU_FLY_ROUTE_LOOP = 0x11,            /* restarts itself at each goal, no leg count */
    KUTKU_FLY_LEVEL_OUT_AND_LAND = 0x12,    /* KUTKU_MOT_LEVEL_OUT, then KUTKU_FLY_LAND */
    KUTKU_FLY_GLIDE_TO_GOAL = 0x13,         /* one leg, then KUTKU_FLY_GLIDE_ONCE */
    KUTKU_FLY_HOP_IN_PLACE = 0x14,          /* KUTKU_MOT_HOP_IN_PLACE, velY-only curve 0xC */
    KUTKU_FLY_HOVER_STRAIGHT_TO_GOAL = 0x15, /* yaw straight at the goal, 3D distance */
    KUTKU_FLY_LEVEL_OUT = 0x16,             /* KUTKU_MOT_LEVEL_OUT, then the hover */
    KUTKU_FLY_TRAP_ESCAPE = 0x17,           /* from the trap struggles; clears holdTimer */
    KUTKU_FLY_HOVER_RISE = 0x18             /* velY 20 up to goal height + 1000 */
};

/* Group 3, EM_GROUP_ATTACK (EmKutku_RunAttackState): the attacks, and the Gypceros's feigned death. */
enum EmKutkuAttackState {
    KUTKU_ATK_TURN_AND_STARE = 0x00,          /* turn, KUTKU_MOT_STARE */
    KUTKU_ATK_LEAP = 0x02,                    /* KUTKU_MOT_LEAP, runDist, KUTKU_MOT_LEAP_LAND */
    KUTKU_ATK_STARE = 0x03,                   /* KUTKU_MOT_STARE with lookAtTarget */
    KUTKU_ATK_FEIGN_DEATH = 0x04,             /* KUTKU_MOT_DIE, then KUTKU_ATK_FEIGN_DEATH_LIE */
    KUTKU_ATK_TAIL_SPIN = 0x06,               /* KUTKU_MOT_TAIL_SPIN twice, rotY + 0x200 */
    KUTKU_ATK_FEIGN_DEATH_RISE = 0x07,        /* KUTKU_MOT_FEIGN_DEATH_RISE */
    KUTKU_ATK_SWOOP_THEN_LEAVE = 0x08,        /* swoop, climb, fly away */
    KUTKU_ATK_FEIGN_DEATH_LIE = 0x09,         /* KUTKU_MOT_FEIGN_DEATH_LIE, out of combat, gathering point */
    KUTKU_ATK_PECK_COMBO = 0x0A,              /* turn, KUTKU_MOT_PECK_COMBO */
    KUTKU_ATK_PECK_STEAL = 0x0B,              /* KUTKU_MOT_PECK_COMBO, EmKutku_StealItem on contact */
    KUTKU_ATK_CHARGE_TRIP = 0x12,             /* KUTKU_MOT_RUN run, KUTKU_MOT_CHARGE_TRIP with KutkuAttack 0x24 */
    KUTKU_ATK_SWOOP_THEN_LAND = 0x15,         /* swoop, drop, KUTKU_MOT_LEAP_LAND */
    KUTKU_ATK_SPIT_B = 0x17,                  /* EmKutku_Spit variant 1 */
    KUTKU_ATK_SPIT_C = 0x18,                  /* variant 2 */
    KUTKU_ATK_SPIT_D = 0x19,                  /* variant 3 */
    KUTKU_ATK_SPIT = 0x1A,                    /* variant 0 (the row KUTKU_ATK_SPIT_AFTER_FLINCH uses) */
    KUTKU_ATK_SPIT_VOLLEY = 0x1B,             /* turn, KUTKU_MOT_SPIT_VOLLEY */
    KUTKU_ATK_PECK_COMBO_B = 0x1C,            /* turn, KUTKU_MOT_PECK_COMBO_B */
    KUTKU_ATK_FLASH = 0x1D,                   /* KUTKU_MOT_FLASH, the flash while partWhole */
    KUTKU_ATK_SPIT_AFTER_FLINCH = 0x1E,       /* EmKutku_EndStateOrSpit */
    KUTKU_ATK_SPIT_VOLLEY_AFTER_FLINCH = 0x1F, /* same, enraged Kut-Ku */
    KUTKU_ATK_FEIGN_DEATH_RISE_B = 0x20       /* EmKutku_FeignDeathRise */
};

/* Group 4, EM_GROUP_REACT (EmKutku_RunReactState): what EmKutku_Update picks from Em_UpdateDamage's result. */
enum EmKutkuReactState {
    KUTKU_REACT_FLINCH = 0x00,               /* parts 0, 7 or EM_DMG_HIT_MODE3; KUTKU_MOT_FLINCH */
    KUTKU_REACT_FLINCH_B = 0x01,             /* unbroken part 3/4; KUTKU_MOT_FLINCH_B */
    KUTKU_REACT_FLINCH_C = 0x02,             /* parts 5, 6; KUTKU_MOT_FLINCH_C */
    KUTKU_REACT_FLINCH_D = 0x03,             /* parts 1, 2; KUTKU_MOT_FLINCH_D */
    KUTKU_REACT_PART_BREAK = 0x04,           /* EM_DMG_PART8_DOWN; empty function */
    KUTKU_REACT_KNOCK_DOWN = 0x05,           /* broken part other than 3 */
    KUTKU_REACT_KNOCK_DOWN_PART3 = 0x06,     /* broken part 3 */
    KUTKU_REACT_AIR_FLINCH = 0x07,           /* KUTKU_MOT_FALL, recovers into the glide */
    KUTKU_REACT_FALL = 0x08,                 /* EM_DMG_STATUS_IN_AIR or a hit in the air */
    KUTKU_REACT_FALL_B = 0x09,               /* as FALL with blind 2 */
    KUTKU_REACT_FLINCH_E = 0x0A,             /* KUTKU_MOT_FLINCH_E */
    KUTKU_REACT_STATUS4 = 0x0B,              /* EM_DMG_STATUS4 not held; Em_BeginStatus4 */
    KUTKU_REACT_TRAPPED = 0x0C,              /* EM_DMG_ATTACK9 / ATTACK6 */
    KUTKU_REACT_TRAPPED_STRUGGLE = 0x0D,     /* KUTKU_MOT_STRUGGLE until trapTimer ends */
    KUTKU_REACT_STATUS4_DOWNED = 0x0E,       /* EM_DMG_STATUS4 while held */
    KUTKU_REACT_ATTACK5 = 0x0F,              /* EM_DMG_ATTACK5(_FLINCH), Kut-Ku only */
    KUTKU_REACT_FEIGN_DEATH = 0x10,          /* Gypceros low hp, then KUTKU_ATK_FEIGN_DEATH_LIE */
    KUTKU_REACT_STATUS4_DOWNED_END = 0x11,   /* Em_EndStatus4, struggle */
    KUTKU_REACT_STATUS2_DOWNED_END = 0x12,   /* Em_EndStatus2, KUTKU_MOT_DOWNED_GET_UP */
    KUTKU_REACT_STATUS1_DOWNED_END = 0x13,   /* Em_EndStatus1, KUTKU_MOT_DOWNED_GET_UP */
    KUTKU_REACT_PART6_BREAK = 0x14,          /* Gypceros, Em_BreakPart(em, 1) */
    KUTKU_REACT_STUN = 0x15,                 /* EM_DMG_STATUS5 not held; stun stars; Em_BeginStatus5 */
    KUTKU_REACT_STUN_TRAPPED = 0x16,         /* EM_DMG_STATUS5 with holdTimer; downed KUTKU_MOT_DOWNED; then 0x17 */
    KUTKU_REACT_STUN_TRAPPED_STRUGGLE = 0x17, /* KUTKU_MOT_STRUGGLE until trapTimer; Em_EndStatus5; to KUTKU_FLY_TRAP_ESCAPE */
    KUTKU_REACT_HOLD2 = 0x18,                /* EM_DMG_ATTACK13; Em_SetHold2; */
    KUTKU_REACT_FEIGN_DEATH_CARVABLE = 0x19  /* Gypceros level 11-17; KUTKU_MOT_DIE, EmKutku_AddMinePoint, to KUTKU_ATK_FEIGN_DEATH_RISE_B */
};

/* Group 5, EM_GROUP_DEFEAT (EmKutku_RunDefeatState): death and appearing again. */
enum EmKutkuDefeatState {
    KUTKU_DEFEAT_DIE = 0x00,        /* KUTKU_MOT_DIE, EM_STANCE_DEAD, QuestEm_Hunted */
    KUTKU_DEFEAT_DIE_HELD = 0x01,   /* killed with holdTimer; KUTKU_MOT_DOWNED */
    KUTKU_DEFEAT_DIE_FALLING = 0x02, /* killed in EM_STANCE_AIR; KUTKU_MOT_FALL/KUTKU_MOT_FALL_LAND/KUTKU_MOT_DIE_FALLING */
    KUTKU_DEFEAT_REAPPEAR = 0x03    /* QuestEm_Appear in EmKutku_ChangeState; fixed spots or fly-in */
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmKutku_RunEventState): the jungle story scene and the captures. */
enum EmKutkuEventState {
    KUTKU_EVENT_INTRO = 0x00,         /* AREA_JUNGLE_9, quest stages QUEST_ID_KUTKU_SCENE_1/QUEST_ID_KUTKU_SCENE_2, EVENT_FLAG_KUTKU_SCENE */
    KUTKU_EVENT_CAPTURED = 0x04,      /* EM_DMG_CAPTURED with holdTimer */
    KUTKU_EVENT_CAPTURED_HOLD2 = 0x05 /* EM_DMG_CAPTURED with holdTimer2 */
};

/* Group 8, EM_GROUP_VERSUS (EmKutku_RunVersusState): the versus quests' bait and poses. */
enum EmKutkuVersusState {
    KUTKU_VS_EAT_BAIT = 0x00, /* VersusBait_Create, head-down, feed */
    KUTKU_VS_POSE_A = 0x01,   /* per-kind motions, then 0x98 */
    KUTKU_VS_POSE_B = 0x02,   /* per-kind motions, then 0x98 */
    KUTKU_VS_POSE_C = 0x03,   /* per-kind motions, then 0x98 */
    KUTKU_VS_WAIT_BOTH = 0x98, /* Versus_MarkDefeat on entry; waits Versus_BothDefeated */
    KUTKU_VS_END = 0x99       /* 90 frames then EndState */
};

#endif
