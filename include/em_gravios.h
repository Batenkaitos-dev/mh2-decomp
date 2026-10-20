#ifndef EM_GRAVIOS_H
#define EM_GRAVIOS_H

/*
 * The Gravios's vocabulary (the code in em_gravios.c), for its three kinds:
 * the Gravios, the Black Gravios and the Basarios (EM_KIND_GRAVIOS and the
 * others). They run the same code; the Basarios fires a shot where the
 * others breathe their beam.
 *
 * Motions: row N of the family's motion table plays motion N, so one number
 * serves Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion)
 * kept in Em.motionId. The names say what the states that play a motion
 * show; motions no state plays stay numbers.
 *
 * Group 2 (EM_GROUP_FLY) is the flight: the take-off, hovering, the flights
 * to a goal and the landing. Group 7 (EM_GROUP_EVENT2) runs the event
 * dispatcher.
 */
enum EmGraviosMotion {
    GRAV_MOT_IDLE = 0x01,            /* idle loop; layers 0-2 idle with EM_MOTION_ID(layer, 1); Start for Gravios kinds */
    GRAV_MOT_WALK = 0x02,            /* intro walk to the goal on layers 0 and 2 (0x3EA, 0x57A) */
    GRAV_MOT_TURN_RIGHT = 0x05,      /* intro: rotY -0x4000 at the start and -0x4000 more over half its length */
    GRAV_MOT_TURN_LEFT = 0x06,       /* motLeft of Em_TurnToTargetMotion in TURN_AND_BEAM */
    GRAV_MOT_LIMP = 0x0A,            /* GRAV_MOVE_LIMP; GraviosAttack 0x13/0x14 */
    GRAV_MOT_DESCEND = 0x0B,         /* GRAV_FLY_LAND, HOP_TO_GOAL, TRAP_ESCAPE: velY set to reach the floor in 30 frames */
    GRAV_MOT_HOVER = 0x0F,           /* every hovering FLY state; dust under it near the floor at frames 8/86/160 */
    GRAV_MOT_CHARGE = 0x11,          /* GRAV_MOVE_CHARGE: moves from frame 150, where EmGravios_MotionEffects makes GraviosAttack 0xB (0x1B Basarios) */
    GRAV_MOT_TAKE_OFF = 0x12,        /* GRAV_FLY_TAKE_OFF, HOP_*, TRAP_ESCAPE: curve 2 from frame 96 or curve 3, air from frame 30/96 */
    GRAV_MOT_TOUCH_DOWN = 0x13,      /* back on the floor (EM_STANCE_GROUND), camera shake, GraviosAttack 0xD on frame 2 */
    GRAV_MOT_UPPER_D = 0x14,         /* layer-1 gesture of GRAV_IDLE_UPPER_D */
    GRAV_MOT_UPPER_A = 0x18,         /* layer-1 gesture of GRAV_IDLE_UPPER_A */
    GRAV_MOT_UPPER_B = 0x19,         /* layer-1 gesture (id 0x4C9) the intro loops while walking and idling */
    GRAV_MOT_UPPER_E = 0x1A,         /* layer-1 gesture of GRAV_IDLE_UPPER_E */
    GRAV_MOT_UPPER_C = 0x1C,         /* layer-1 gesture of GRAV_IDLE_UPPER_C */
    GRAV_MOT_IDLE_LEAD_IN = 0x1D,    /* GRAV_IDLE plays it before the idle; sounds only */
    GRAV_MOT_LIE = 0x22,             /* EmGravios_CapturedHold2 after the collapse; effects: lying puffs at bone 0x22 every frame */
    GRAV_MOT_ATTACK_A = 0x24,        /* GRAV_ATK_TURN_AND_ATTACK_A, ATTACK_A_LOOKING: GraviosAttack 0x15, then 1 (Gravios) / 0x19 (Basarios) */
    GRAV_MOT_GET_UP = 0x25,          /* REST_END, STATUS1_END, STATUS2_END */
    GRAV_MOT_COLLAPSE = 0x26,        /* STATUS1/2_COLLAPSE (also GRAV_EVENT_CAPTURED_HOLD2) */
    GRAV_MOT_REST_LIE_DOWN = 0x27,   /* start of GRAV_IDLE_REST */
    GRAV_MOT_SPIN_BASARIOS = 0x2A,   /* GRAV_ATK_SPIN for Basarios, rotY +0x200 a frame; GraviosAttack 0x1C at frame 30 */
    GRAV_MOT_SPIN = 0x2B,            /* GRAV_ATK_SPIN for the Gravios kinds, rotY -0x200 a frame; GraviosAttack 0x10 at frame 30 */
    GRAV_MOT_TURN_THREAT = 0x36,     /* turns to frame 60 (IDLE/MOVE TURN_TO_GOAL) */
    GRAV_MOT_FLINCH_B = 0x3C,        /* GRAV_REACT_FLINCH_PART6_BROKEN; GRAV_REACT_HOLD2 start (no root motion to frame 52) */
    GRAV_MOT_FLINCH_F = 0x3D,        /* GRAV_VS_POSE_A; GRAV_REACT_FLINCH_E; effects: PartFx 0x1A at frame 30 */
    GRAV_MOT_SHAKE_OFF = 0x3E,       /* GRAV_IDLE_SHAKE_OFF and the three status shake-offs */
    GRAV_MOT_FLINCH_D = 0x3F,        /* GRAV_REACT_FLINCH_C (part 5) */
    GRAV_MOT_FLINCH_E = 0x40,        /* GRAV_REACT_FLINCH_D (parts 1, 2) */
    GRAV_MOT_PART_BREAK = 0x41,      /* GRAV_REACT_PART_BREAK: mesh 1 hidden at frame 6, a full turn from frame 52 to 122 */
    GRAV_MOT_FLINCH_C = 0x42,        /* GRAV_REACT_FLINCH_B (unbroken part 3/4) */
    GRAV_MOT_DIE = 0x44,             /* GRAV_DEFEAT_DIE and DIE_RISING; cam shake level 2 at frame 212 */
    GRAV_MOT_KNOCK_DOWN_PART3 = 0x45, /* GRAV_REACT_KNOCK_DOWN_PART3 */
    GRAV_MOT_KNOCK_DOWN = 0x4A,      /* GRAV_REACT_KNOCK_DOWN */
    GRAV_MOT_STATUS4 = 0x4C,         /* GRAV_REACT_STATUS4 (status-4 effect) and GRAV_REACT_HOLD2 until the timer */
    GRAV_MOT_FALL = 0x4D,            /* DIE_FALLING out of the air (also the react states' falls) */
    GRAV_MOT_FALL_LAND = 0x4E,       /* DIE_FALLING on the floor, until frame 60 */
    GRAV_MOT_IDLE_PAIR_A = 0x50,     /* idle PAIR_A/B/C; nothing tells it from PAIR_B */
    GRAV_MOT_IDLE_PAIR_B = 0x51,     /* idle PAIR_A/B/C */
    GRAV_MOT_DIE_FALLING = 0x52,     /* DIE_FALLING after landing (blend 6), EM_STANCE_DEAD */
    GRAV_MOT_TRAPPED = 0x5F,         /* GRAV_REACT_TRAPPED */
    GRAV_MOT_STRUGGLE = 0x60,        /* GRAV_REACT_TRAPPED_STRUGGLE, looped until trapTimer */
    GRAV_MOT_DOWNED = 0x61,          /* GRAV_REACT_STATUS4_DOWNED, GRAV_REACT_STUN_TRAPPED (and the held lying GRAV_IDLE_STATUS2_DOWNED, 0x1F) */
    GRAV_MOT_DOWNED_GET_UP = 0x62,   /* REACT_STATUS2/STATUS1_DOWNED_END, back to the struggle */
    GRAV_MOT_CHARGE_STOP = 0x64,     /* end of CHARGE, GRAV_MOVE_CHARGE_STOP; dust 2 at frame 28, PartFx 54-120 */
    GRAV_MOT_IDLE_ENRAGED = 0x65,    /* GRAV_IDLE_ENRAGED only; rage dust clouds */
    GRAV_MOT_IDLE_LOOP_B = 0x66,     /* GRAV_IDLE_TIMED_LOOP only; no sounds or effects */
    GRAV_MOT_IDLE_ACTION_A = 0x67,   /* GRAV_IDLE_ACTION_A only; sounds only */
    GRAV_MOT_EAT = 0x68,             /* intro at the wall (FallingDebris and effects at bone 0x24 on frames 68/94/204) */
    GRAV_MOT_BEAM_SHORT = 0x69,      /* GRAV_ATK_TURN_AND_BEAM: beam (bone 0x22, variant 0) held 20 frames from frame 122 */
    GRAV_MOT_ROAR = 0x6A,            /* effects: GraviosAttack 0x12 and ElementBurst type 5 (the roar burst) at frame 110 */
    GRAV_MOT_ROCK_POSE = 0x6B,       /* Basarios Start row; HIDE_AS_ROCK, REHIDE_AS_ROCK, ROCK_POSE; silent */
    GRAV_MOT_RISE_BURST = 0x6C,      /* rising out of the hiding pose GRAV_MOT_ROCK_POSE (GRAV_IDLE_ROCK_BURST, with a timed mine) */
    GRAV_MOT_ATTACK_B = 0x6D,        /* GRAV_ATK_TURN_AND_ATTACK_B: GraviosAttack 2 (Gravios) / 0x1A (Basarios) at 68 */
    GRAV_MOT_BEAM = 0x6E,            /* beam (bone 0x22, variant 1) at frame 136, held 35 frames (55 in the intro) */
    GRAV_MOT_GAS = 0x6F,             /* GRAV_ATK_GAS_A/B/C held 120 frames with the per-variant cloud */
    GRAV_MOT_REAR_ATTACK = 0x70,     /* GRAV_ATK_REAR_ATTACK: backward-facing GraviosAttack 4 at frame 46 */
    GRAV_MOT_GAS_END = 0x71,         /* ATK_GAS_* after the 120 frames, clouds to frame 50 */
    GRAV_MOT_BEAM_END = 0x72,        /* after GRAV_MOT_BEAM (Basarios skips it in the idle and attack states) */
    GRAV_MOT_FLINCH = 0x73,          /* GRAV_REACT_FLINCH (parts 0, 7, unbroken 6) */
    GRAV_MOT_STUN = 0x74,            /* GRAV_REACT_STUN with stun stars */
    GRAV_MOT_BEAM_LONG = 0x75,       /* GRAV_ATK_BEAM_LONG: beam at frame 118 held 40 */
    GRAV_MOT_GET_UP_B = 0x76,        /* after GRAV_REACT_FALL(_B) and GRAV_REACT_STUN */
    GRAV_MOT_LEAP = 0x77,            /* GRAV_ATK_LEAP_SLAM: jump at frame 70 */
    GRAV_MOT_BURROW = 0x78,          /* BURROW_TO_NEXT_AREA, GRAV_MOVE_BURROW; Basarios-only dust and PartFx */
    GRAV_MOT_SURFACE = 0x79,         /* SURFACE, SURFACE_AT_GOAL, GRAV_MOVE_SURFACE; Basarios-only AttackBurst at frame 8 */
    GRAV_MOT_LEAP_LAND = 0x7A        /* GRAV_ATK_LEAP_SLAM landing: GraviosAttack 0x1E at frame 2 */
};

/* Group 0, EM_GROUP_IDLE (EmGravios_RunIdleState): standing, turning, the beam, sleep, statuses and rage. */
enum EmGraviosIdleState {
    GRAV_IDLE_STAND = 0x00,              /* idle on all layers, em->timer, scriptPending */
    GRAV_IDLE = 0x01,                    /* EmGravios_EndState/Start target; LEAD_IN then idle; setter swaps to 0x02 when enraged */
    GRAV_IDLE_ENRAGED = 0x02,            /* the setter's GRAV_IDLE while em->rage; GRAV_MOT_IDLE_ENRAGED */
    GRAV_IDLE_TIMED_LOOP = 0x03,         /* GRAV_MOT_IDLE_LOOP_B for (stateRand & 3) * 300 frames */
    GRAV_IDLE_ACTION_A = 0x04,           /* GRAV_MOT_IDLE_ACTION_A once */
    GRAV_IDLE_UPPER_A = 0x05,            /* layer-1 gesture GRAV_MOT_UPPER_A */
    GRAV_IDLE_UPPER_B = 0x06,            /* layer-1 gesture GRAV_MOT_UPPER_B */
    GRAV_IDLE_UPPER_E = 0x07,            /* layer-1 gesture GRAV_MOT_UPPER_E */
    GRAV_IDLE_ROCK_POSE = 0x08,          /* GRAV_MOT_ROCK_POSE, blind, reaction set 1, timed */
    GRAV_IDLE_PAIR_A = 0x09,             /* PAIR_A or PAIR_B at random */
    GRAV_IDLE_TURN_TO_GOAL = 0x0A,       /* GRAV_MOT_TURN_THREAT to frame 60, rotY snapped to goalYaw */
    GRAV_IDLE_UPPER_C = 0x0B,            /* layer-1 gesture GRAV_MOT_UPPER_C */
    GRAV_IDLE_ROCK_BURST = 0x0C,         /* GRAV_MOT_RISE_BURST, timed gathering point; EmGravios_ChangeState forces it during rock hiding */
    GRAV_IDLE_UPPER_D = 0x0D,            /* layer-1 gesture GRAV_MOT_UPPER_D */
    GRAV_IDLE_TURN_AND_BEAM = 0x0E,      /* Em_SetTargetAngle; turn, GRAV_MOT_BEAM, GRAV_MOT_BEAM_END (not Basarios) */
    GRAV_IDLE_PAIR_B = 0x0F,             /* PAIR_A, idle, PAIR_B, idle */
    GRAV_IDLE_PAIR_C = 0x10,             /* PAIR_B, idle, PAIR_A, idle */
    GRAV_IDLE_THREAT = 0x11,             /* GRAV_MOT_ROAR once */
    GRAV_IDLE_REST = 0x12,               /* Em_ResetNotice; lie down, lie 1800 frames, hp regen, then REST_END */
    GRAV_IDLE_ACTION_C = 0x13,           /* NO_FLINCH, sight row 1, GRAV_MOT_IDLE_ACTION_C */
    GRAV_IDLE_STATUS2_COLLAPSE = 0x14,   /* Em_BeginStatus2; collapse, lie, then STATUS2_END */
    GRAV_IDLE_HIDE_AS_ROCK = 0x16,       /* Basarios Start; motionSpeed 0; rock pose 5400 frames, then ROCK_BURST */
    GRAV_IDLE_REST_END = 0x17,           /* Em_RefillNeed3; EmGravios_GetUp after REST */
    GRAV_IDLE_STATUS2_END = 0x18,        /* Em_EndStatus2; EmGravios_GetUp after STATUS2_COLLAPSE */
    GRAV_IDLE_SHAKE_OFF = 0x19,          /* GRAV_MOT_SHAKE_OFF after every get-up */
    GRAV_IDLE_REHIDE_AS_ROCK = 0x1A,     /* motionSpeed 0, out of combat; after SURFACE; rock pose 600 + 5400 frames */
    GRAV_IDLE_STATUS1_COLLAPSE = 0x1B,   /* Em_BeginStatus1; collapse, lie, then STATUS1_END */
    GRAV_IDLE_STATUS1_END = 0x1C,        /* Em_EndStatus1; EmGravios_GetUp after STATUS1_COLLAPSE */
    GRAV_IDLE_STATUS2_DOWNED = 0x1D,     /* Em_BeginStatus2; GRAV_MOT_DOWNED, then GRAV_REACT_STATUS2_DOWNED_END (EndStatus2) */
    GRAV_IDLE_STATUS1_DOWNED = 0x1F,     /* GRAV_MOT_DOWNED, then GRAV_REACT_STATUS1_DOWNED_END (EndStatus1) */
    GRAV_IDLE_STATUS4_SHAKE_OFF = 0x21,  /* Em_EndStatus4; EmGravios_ShakeOff */
    GRAV_IDLE_HOLD2_SHAKE_OFF = 0x22,    /* holdTimer2 = 0; EmGravios_ShakeOff */
    GRAV_IDLE_STUN_SHAKE_OFF = 0x23,     /* Em_EndStatus5; EmGravios_ShakeOff */
    GRAV_IDLE_BURROW_TO_NEXT_AREA = 0x24, /* GRAV_MOT_BURROW, area = nextArea, then SURFACE */
    GRAV_IDLE_SURFACE_AT_GOAL = 0x25,    /* setter copies the goal to the position; GRAV_MOT_SURFACE */
    GRAV_IDLE_SURFACE = 0x26,            /* Em_TypeSetup; entered from BURROW_TO_NEXT_AREA; GRAV_MOT_SURFACE */
    GRAV_IDLE_RAGE_START = 0x27,         /* Em_StartRage; GRAV_MOT_TURN_THREAT */
    GRAV_IDLE_PENDING_RAGE_START = 0x28  /* Em_StartPendingRage; GRAV_MOT_TURN_THREAT */
};

/* Group 1, EM_GROUP_MOVE (EmGravios_RunMoveState): walking, turning and digging. */
enum EmGraviosMoveState {
    GRAV_MOVE_WALK = 0x00,                   /* GRAV_MOT_WALK, Em_StepToGoal 0x80; becomes LIMP below 30% hp */
    GRAV_MOVE_END = 0x01,                    /* setter only sets scriptPending; GRAV_MOVE_END calls EmGravios_EndState */
    GRAV_MOVE_CHARGE_STOP = 0x02,            /* GRAV_MOT_CHARGE_STOP once */
    GRAV_MOVE_TURN = 0x03,                   /* EmGravios_TurnParams[0] via Em_TurnStep */
    GRAV_MOVE_CHARGE = 0x04,                 /* GRAV_MOT_CHARGE, moves from frame 150, then CHARGE_STOP; Em_SetSpeedScaled mode 2 */
    GRAV_MOVE_WALK_B = 0x05,                 /* GRAV_MOT_WALK, Em_StepToGoal 0x40; becomes LIMP below 30% hp */
    GRAV_MOVE_LIMP = 0x06,                   /* GRAV_MOT_LIMP; Em_SetSpeedScaled mode 6 */
    GRAV_MOVE_TURN_TO_GOAL = 0x07,           /* GRAV_MOT_TURN_THREAT, rotY 0x200 a frame */
    GRAV_MOVE_CHARGE_SLOW = 0x08,            /* GRAV_MOVE_CHARGE at motionSpeed 0.7 */
    GRAV_MOVE_TURN_TO_GOAL_RAGE_CHECK = 0x09, /* setter Em_CheckRageByHp; EmGravios_TurnToGoal */
    GRAV_MOVE_BURROW = 0x0A,                 /* GRAV_MOT_BURROW once */
    GRAV_MOVE_SURFACE = 0x0B,                /* GRAV_MOT_SURFACE, no exit */
    GRAV_MOVE_TURN_NO_HIT = 0x0C,            /* Turn variant 1 (setter and state set noReact/senseDelay/noHitTimer/noPushTimer) */
    GRAV_MOVE_WALK_NO_HIT = 0x0D,            /* Walk variant 1, unhittable flags; setter skips the limp check */
    GRAV_MOVE_WALK_FAST = 0x0E               /* Walk variant 2, speedBonus 150 */
};

/* Group 2, EM_GROUP_FLY (EmGravios_RunFlyState): the flight. */
enum EmGraviosFlyState {
    GRAV_FLY_TAKE_OFF = 0x00,        /* take-off motion, curve 2, air from frame 96, then GRAV_FLY_HOVER_ONCE */
    GRAV_FLY_LAND = 0x01,            /* hover down, descent, touch-down; area entry in the air, end of GRAV_FLY_HOVER */
    GRAV_FLY_HOVER = 0x02,           /* hover turning to the goal until the timer, then GRAV_FLY_LAND */
    GRAV_FLY_HOP_TO_GOAL = 0x03,     /* take off, turn to goal, descend, land */
    GRAV_FLY_HOP_FORWARD = 0x04,     /* curve 3, velZ 20, touch-down */
    GRAV_FLY_HOP_BACK = 0x05,        /* curve 3, velZ -20, touch-down */
    GRAV_FLY_HOVER_TO_GOAL = 0x07,   /* setter velZ 20, timer 3x travel; turn and pitch to goal */
    GRAV_FLY_TO_AREA_POINT = 0x09,   /* the setter aims at an area point (timer 500), but no state function runs it */
    GRAV_FLY_HOVER_ONCE = 0x0E,      /* the state of EmGravios_EndStateInAir; hover once */
    GRAV_FLY_HOVER_TURN = 0x10,      /* turn to goal for 150 frames */
    GRAV_FLY_RISE = 0x14,            /* velY 20 up to goal height + 1000 */
    GRAV_FLY_STRAIGHT_TO_GOAL = 0x15, /* yaw straight at the goal, 3D distance */
    GRAV_FLY_TRAP_ESCAPE = 0x17      /* from GRAV_REACT_TRAPPED_STRUGGLE; clears holdTimer and trapHit */
};

/* Group 3, EM_GROUP_ATTACK (EmGravios_RunAttackState): the beam, the charges, the spin and the slams. */
enum EmGraviosAttackState {
    GRAV_ATK_TURN_AND_ATTACK_A = 0x00, /* turn, GRAV_MOT_ATTACK_A; setter Em_SetTargetAngle */
    GRAV_ATK_TURN_AND_BEAM = 0x01,    /* turn, GRAV_MOT_BEAM_SHORT, beam held 20 */
    GRAV_ATK_LEAP_SLAM = 0x02,        /* velY 73 jump, landing GRAV_MOT_LEAP_LAND with an attack */
    GRAV_ATK_ATTACK_A_LOOKING = 0x03, /* GRAV_MOT_ATTACK_A with lookAtTarget 1, no turn */
    GRAV_ATK_BEAM_OR_SHOT_A = 0x04,   /* EmGravios_TurnAndBeamOrShot variant 0 (Basarios shot row 0) */
    GRAV_ATK_GAS_A = 0x05,            /* EmGravios_Gas variant 0: GraviosAttack 0xE, cloud 0 */
    GRAV_ATK_SPIN = 0x06,             /* 0x200 a frame, twice; motion by kind */
    GRAV_ATK_GAS_B = 0x07,            /* variant 1: GraviosAttack 0xC, cloud 1 */
    GRAV_ATK_TURN_AND_ATTACK_B = 0x08, /* turn, GRAV_MOT_ATTACK_B */
    GRAV_ATK_REAR_ATTACK = 0x09,      /* GRAV_MOT_REAR_ATTACK, backward-facing GraviosAttack 4 */
    GRAV_ATK_GAS_C = 0x0A,            /* variant 2: GraviosAttack 0x1F, cloud 2 at 1.1x */
    GRAV_ATK_BEAM_OR_SHOT_B = 0x0B,   /* variant 1 (Basarios shot row 1; same as _A for Gravios) */
    GRAV_ATK_BEAM_LONG = 0x0C         /* GRAV_MOT_BEAM_LONG, beam held 40, no turn */
};

/* Group 4, EM_GROUP_REACT (EmGravios_RunReactState): what EmGravios_Update picks from Em_UpdateDamage's result. */
enum EmGraviosReactState {
    GRAV_REACT_FLINCH_PART6_BROKEN = 0x00,  /* EmGravios_StartPart6Flinch with parts[6].state > 1; GRAV_MOT_FLINCH_B */
    GRAV_REACT_FLINCH_B = 0x01,             /* unbroken part 3/4/other; GRAV_MOT_FLINCH_C */
    GRAV_REACT_FLINCH_C = 0x02,             /* part 5; GRAV_MOT_FLINCH_D */
    GRAV_REACT_FLINCH_D = 0x03,             /* parts 1, 2; GRAV_MOT_FLINCH_E */
    GRAV_REACT_PART_BREAK = 0x04,           /* EM_DMG_PART8_DOWN; partBreakCount++; GRAV_MOT_PART_BREAK, full turn */
    GRAV_REACT_KNOCK_DOWN = 0x05,           /* broken part other than 3; GRAV_MOT_KNOCK_DOWN */
    GRAV_REACT_KNOCK_DOWN_PART3 = 0x06,     /* broken part 3; GRAV_MOT_KNOCK_DOWN_PART3 */
    GRAV_REACT_AIR_FLINCH = 0x07,           /* fall motion to frame 14, back to GRAV_FLY_HOVER_ONCE; no caller here */
    GRAV_REACT_FALL = 0x08,                 /* EM_DMG_STATUS_IN_AIR or a part flinch in the air */
    GRAV_REACT_FALL_B = 0x09,               /* as FALL with blind 2; no caller here */
    GRAV_REACT_FLINCH_E = 0x0A,             /* GRAV_MOT_FLINCH_F; no caller here */
    GRAV_REACT_STATUS4 = 0x0B,              /* EM_DMG_STATUS4 not held; Em_BeginStatus4; to GRAV_IDLE_STATUS4_SHAKE_OFF */
    GRAV_REACT_TRAPPED = 0x0C,              /* EM_DMG_ATTACK9 / ATTACK6 not held; GRAV_MOT_TRAPPED */
    GRAV_REACT_TRAPPED_STRUGGLE = 0x0D,     /* GRAV_MOT_STRUGGLE until trapTimer; to GRAV_FLY_TRAP_ESCAPE */
    GRAV_REACT_STATUS4_DOWNED = 0x0E,       /* EM_DMG_STATUS4 with holdTimer; Em_BeginStatus4; EmGravios_DownedWhileHeld variant 0 */
    GRAV_REACT_FLINCH = 0x0F,               /* parts 0, 7, unbroken 6; GRAV_MOT_FLINCH */
    GRAV_REACT_STUN = 0x10,                 /* EM_DMG_STATUS5 not held; Em_BeginStatus5; stun stars */
    GRAV_REACT_PART_BREAK_END = 0x11,       /* Em_ResetToBasePose, Em_BreakPart(em, 2) on entry */
    GRAV_REACT_STATUS4_DOWNED_END = 0x12,   /* Em_EndStatus4 on entry; runs EmGravios_TrapStruggle */
    GRAV_REACT_STATUS2_DOWNED_END = 0x13,   /* from GRAV_IDLE_STATUS2_DOWNED; Em_EndStatus2 on entry; GRAV_MOT_DOWNED_GET_UP */
    GRAV_REACT_STATUS1_DOWNED_END = 0x14,   /* from GRAV_IDLE_STATUS1_DOWNED; Em_EndStatus1 on entry; GRAV_MOT_DOWNED_GET_UP */
    GRAV_REACT_HOLD2 = 0x15,                /* EM_DMG_ATTACK13; Em_SetHold2; Em_EndAllStatus on entry */
    GRAV_REACT_STUN_TRAPPED = 0x16,         /* EM_DMG_STATUS5 with holdTimer; Em_BeginStatus5; EmGravios_DownedWhileHeld variant 1 */
    GRAV_REACT_STUN_TRAPPED_STRUGGLE = 0x17 /* Em_EndStatus5 on entry; runs EmGravios_TrapStruggle */
};

/* Group 5, EM_GROUP_DEFEAT (EmGravios_RunDefeatState): hunted, captured, and appearing again. */
enum EmGraviosDefeatState {
    GRAV_DEFEAT_DIE = 0x00,        /* GRAV_MOT_DIE, EM_STANCE_DEAD, QuestEm_Hunted, carve point */
    GRAV_DEFEAT_DIE_HELD = 0x01,   /* killed with holdTimer; GRAV_MOT_DOWNED */
    GRAV_DEFEAT_DIE_FALLING = 0x02, /* killed in EM_STANCE_AIR; FALL, FALL_LAND, DIE_FALLING */
    GRAV_DEFEAT_DIE_RISING = 0x03, /* killed while hiding or in GRAV_IDLE_ROCK_POSE or ROCK_BURST; GRAV_MOT_RISE_BURST then GRAV_MOT_DIE */
    GRAV_DEFEAT_REAPPEAR = 0x04    /* QuestEm_Appear in EmGravios_ChangeState; arena spots or a fly-in landing */
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmGravios_RunEventState): the volcano's opening scene and the scripted events. */
enum EmGraviosEventState {
    GRAV_EVENT_INTRO = 0x00,         /* Update, in the volcano quest whose opening scene the family plays, in AREA_VOLCANO_6 while the scene's flag is clear; eat, beam, wait for the flag */
    GRAV_EVENT_INTRO_B = 0x01,       /* same function as 0; nothing in the file enters it */
    GRAV_EVENT_INTRO_C = 0x02,       /* same function as 0; nothing in the file enters it */
    GRAV_EVENT_INTRO_D = 0x03,       /* same function as 0; nothing in the file enters it */
    GRAV_EVENT_CAPTURED = 0x04,      /* EM_DMG_CAPTURED with holdTimer; status 1 begun by EmGravios_ChangeState */
    GRAV_EVENT_CAPTURED_HOLD2 = 0x05 /* EM_DMG_CAPTURED with holdTimer2 */
};

/* Group 8, EM_GROUP_VERSUS (EmGravios_RunVersusState): the versus quests' bait and poses. */
enum EmGraviosVersusState {
    GRAV_VS_EAT_BAIT = 0x00, /* VersusBait_Create, GRAV_MOT_EAT, GRAV_MOT_ROAR, Versus_RouteEmByTeam */
    GRAV_VS_POSE_A = 0x01,   /* GRAV_MOT_FLINCH_F, then GRAV_VS_WAIT_BOTH */
    GRAV_VS_POSE_B = 0x02,   /* GRAV_MOT_ROAR, then GRAV_VS_WAIT_BOTH */
    GRAV_VS_POSE_C = 0x03,   /* GRAV_MOT_BEAM_SHORT, GRAV_MOT_BEAM_END, then GRAV_VS_WAIT_BOTH */
    GRAV_VS_WAIT_BOTH = 0x98, /* Versus_MarkDefeat on entry; waits Versus_BothDefeated */
    GRAV_VS_END = 0x99       /* 90 frames then EmGravios_EndState */
};


/* The curves the flight states start (EmGravios_StartCurves: the velY and
 * speed tables' row, EmFlyerWork.curveKind) and the one the hovering states
 * bob by (EmGravios_MoveByVelYCurve's row of EmGravios_MoveVelYCurves). */
enum EmGraviosCurve {
    GRAV_CURVE_HOVER = 1,           /* EmGravios_MoveByVelYCurve: every hovering GRAV_FLY_ state */
    GRAV_CURVE_TAKE_OFF = 2,        /* the take-off's climb: GRAV_FLY_TAKE_OFF, HOP_TO_GOAL, TRAP_ESCAPE */
    GRAV_CURVE_HOP = 3              /* GRAV_FLY_HOP_FORWARD and HOP_BACK */
};
/* The family's own meaning of bytes of the monster's work area (Em.work) that
 * its effects and sounds use; its flight uses EmFlyerWork's fields. */
typedef struct EmGraviosWork {
    u8 fxStep;                  /* 0x00: EmGravios_UpdateMotionEffects: 0 on the first frame, then 1 */
    u8 unk01[0x02 - 0x01];
    s16 sndMotion;              /* 0x02: the layer-0 motion id EmGravios_MotionEffects keeps */
    u8 unk04[0x1B - 0x04];
    u8 onFlaggedFloor;          /* 0x1B: AreaGrid2_OnFlaggedFloor at the position, each frame; while 1 the footsteps (bones 0x14 and 0x1A) play their other sound (se + 0x35) */
} EmGraviosWork;

#endif
