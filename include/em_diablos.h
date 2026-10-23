#ifndef EM_DIABLOS_H
#define EM_DIABLOS_H

#include "em.h"

/*
 * The Diablos family's vocabulary (the code in em_diablos.c): Diablos, Black
 * Diablos, Monoblos and White Monoblos (EM_KIND_DIABLOS, EM_KIND_BLACK_DIABLOS,
 * EM_KIND_MONOBLOS, EM_KIND_WHITE_MONOBLOS) share one set of state functions
 * and one numbering of motions and states; the code tests the kind where they
 * differ (each has its own desert scene).
 *
 * Motions: row N of the family's motion table plays motion N, so one number
 * serves Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion)
 * kept in Em.motionId. The names say what the states that play a motion
 * show; motions no state plays stay numbers.
 *
 * Group 2 (EM_GROUP_FLY's slot) holds the burrowing: digging in, travelling
 * under the sand (EM_STANCE_UNDER) and emerging, and the leap out of the sand
 * with its states in the air (EM_STANCE_AIR).
 */
enum EmDiablosMotion {
    DIAB_MOT_IDLE = 0x01,          /* EmDiablos_Init, IDLE, STAND, ONE_LOOP */
    DIAB_MOT_WALK = 0x03,          /* DIAB_MOVE_WALK, and DIAB_BURROW_WALK under the sand */
    DIAB_MOT_TURN_RIGHT = 0x05,    /* right motion of Em_TurnToTargetMotion; DiablosAttack 5/6 and dust */
    DIAB_MOT_TURN_LEFT = 0x06,     /* left motion of Em_TurnToTargetMotion; DiablosAttack 7/8 and dust */
    DIAB_MOT_LIMP = 0x0A,          /* DIAB_MOVE_LIMP (below 30% hp) */
    DIAB_MOT_DESCEND = 0x0B,       /* the last 30 frames down (AIR_LAND, SAND_ESCAPE) */
    DIAB_MOT_AIR = 0x0F,           /* the airborne loop of the copied air states; dust when within 1000 of the floor */
    DIAB_MOT_RUN = 0x11,           /* runs and the charge (DIAB_MOVE_RUN, DIAB_MOVE_RUN_AND_STOP, the DIAB_ATK_CHARGE states; under the sand DIAB_BURROW_TRAVEL) */
    DIAB_MOT_LEAP = 0x12,          /* EM_STANCE_AIR from frame 70 with velY curve 2 (DIAB_BURROW_LEAP, _SAND_ESCAPE); dust near the floor */
    DIAB_MOT_LAND = 0x13,          /* touch-down, stance ground, camera shake; DiablosAttack 13 at frame 2 */
    DIAB_MOT_UPPER_D = 0x14,       /* layer-1 gesture of DIAB_IDLE_UPPER_D */
    DIAB_MOT_UPPER_A = 0x18,       /* layer-1 gesture of DIAB_IDLE_UPPER_A */
    DIAB_MOT_UPPER_B = 0x19,       /* layer-1 gesture of DIAB_IDLE_UPPER_B */
    DIAB_MOT_UPPER_E = 0x1A,       /* layer-1 gesture of DIAB_IDLE_UPPER_E (no sounds) */
    DIAB_MOT_UPPER_C = 0x1C,       /* layer-1 gesture of DIAB_IDLE_UPPER_C */
    DIAB_MOT_REST = 0x1F,          /* rest loop of DIAB_IDLE_REST (hp regen) */
    DIAB_MOT_REST_LIE_DOWN = 0x20, /* start of DIAB_IDLE_REST */
    DIAB_MOT_REST_GET_UP = 0x21,   /* DIAB_IDLE_REST_END */
    DIAB_MOT_LIE = 0x22,           /* lying under statuses 1/2 (STATUS1/2_COLLAPSE, STATUS2_LIE_DOWN) */
    DIAB_MOT_STARE = 0x24,         /* DIAB_ATK_TURN_AND_STARE, DIAB_ATK_STARE, DIAB_VS_BAIT */
    DIAB_MOT_GET_UP = 0x25,        /* STATUS1_END, STATUS2_END */
    DIAB_MOT_COLLAPSE = 0x26,      /* STATUS1/2_COLLAPSE start */
    DIAB_MOT_LIE_DOWN = 0x27,      /* DIAB_IDLE_STATUS2_LIE_DOWN */
    DIAB_MOT_TAIL_SPIN = 0x2A,     /* DIAB_ATK_TAIL_SPIN, rotY + 0x200; DiablosAttack 17 at frame 30 */
    DIAB_MOT_ALERT_IDLE = 0x33,    /* combat idle (DIAB_IDLE_ALERT), the Diablos scene, DIAB_VS_BAIT; DiablosAttack 27/28 */
    DIAB_MOT_TURN_IN_PLACE = 0x36, /* DIAB_MOVE_TURN_TO_GOAL (rotY 0x200 a frame to frame 60), DIAB_VS_POSE_B */
    DIAB_MOT_FLINCH = 0x3C,        /* DIAB_REACT_FLINCH, start of DIAB_REACT_HOLD2 (no root motion to frame 52) */
    DIAB_MOT_FLINCH_E = 0x3D,      /* DIAB_REACT_FLINCH_E, DIAB_VS_POSE_A */
    DIAB_MOT_SHAKE_OFF = 0x3E,     /* DIAB_IDLE_SHAKE_OFF and the three status shake-offs */
    DIAB_MOT_FLINCH_C = 0x3F,      /* DIAB_REACT_FLINCH_C */
    DIAB_MOT_FLINCH_D = 0x40,      /* DIAB_REACT_FLINCH_D */
    DIAB_MOT_PART_BREAK = 0x41,    /* DIAB_REACT_PART_BREAK: mesh 1 hidden at frame 6, turning in frames 52-122 */
    DIAB_MOT_FLINCH_B = 0x42,      /* DIAB_REACT_FLINCH_B */
    DIAB_MOT_DIE = 0x44,           /* EmDiablos_Die with QuestEm_Hunted; camera shake at 212, dust at 268 */
    DIAB_MOT_KNOCK_DOWN = 0x45,    /* EmDiablos_KnockDown even variants (KNOCK_DOWN, STUN) */
    DIAB_MOT_DOWN = 0x46,          /* lying after KNOCK_DOWN (120 frames or the stun timer) */
    DIAB_MOT_DOWN_GET_UP = 0x47,   /* after DOWN and after the falls (DIAB_REACT_FALL, FALL_B) */
    DIAB_MOT_DOWN_GET_UP_B = 0x48, /* after DOWN_B */
    DIAB_MOT_KNOCK_DOWN_B = 0x4A,  /* EmDiablos_KnockDown odd variants (KNOCK_DOWN_PART3, STUN_B) */
    DIAB_MOT_DOWN_B = 0x4B,        /* lying after KNOCK_DOWN_B */
    DIAB_MOT_STATUS4 = 0x4C,       /* DIAB_REACT_STATUS4 and DIAB_REACT_HOLD2 until the timer */
    DIAB_MOT_FALL = 0x4D,          /* DIAB_DEFEAT_DIE_FALLING, DIAB_REACT_FALL and _FALL_B: falling out of the air */
    DIAB_MOT_FALL_LAND = 0x4E,     /* hitting the floor after DIAB_MOT_FALL (velocity cleared); dust at frame 8 */
    DIAB_MOT_IDLE_PAIR_A = 0x50,   /* IDLE PAIR_A/B/C/D; nothing tells it apart from DIAB_IDLE_PAIR_B */
    DIAB_MOT_IDLE_PAIR_B = 0x51,   /* IDLE PAIR_A/B/C */
    DIAB_MOT_DIE_FALLING = 0x52,   /* dead after the fall (EM_STANCE_DEAD, QuestEm_Hunted) */
    DIAB_MOT_SAND_STRUGGLE = 0x60, /* stuck in the sand, looped for trapTimer frames (DIAB_REACT_POP_OUT, the sand struggles), then DIAB_BURROW_SAND_ESCAPE */
    DIAB_MOT_DOWNED = 0x61,        /* SAND_DOWNED (lying puffs, periodic sound) and group-4 stun/status states (stun stars) */
    DIAB_MOT_DOWNED_GET_UP = 0x62, /* DIAB_REACT_STATUS2_SAND_GET_UP (EmDiablos_Update: a hit while lying in DIAB_IDLE_SAND_DOWNED) */
    DIAB_MOT_CHARGE_STOP = 0x64,   /* the end of DIAB_MOVE_RUN_AND_STOP and of the DIAB_ATK_CHARGE states */
    DIAB_MOT_BURROW = 0x67,        /* DIAB_BURROW_DIG (EM_STANCE_GROUND, shadow hidden from frame 100, guarded) */
    DIAB_MOT_EMERGE = 0x68,        /* DIAB_BURROW_EMERGE (EM_STANCE_UNDER while blending, then EM_STANCE_GROUND) */
    DIAB_MOT_POP_OUT = 0x69,       /* forced out of the sand (DIAB_REACT_POP_OUT, the downs in the sand, DIAB_DEFEAT_DIE_IN_SAND); EM_STANCE_UNDER while it blends in */
    DIAB_MOT_ATTACK_A = 0x6A,      /* DIAB_ATK_CHARGE_ATTACK_A, DIAB_ATK_TURN_ATTACK_A; DiablosAttack 24 at frame 48 */
    DIAB_MOT_ATTACK_B = 0x6B,      /* DIAB_ATK_CHARGE_ATTACK_B, DIAB_ATK_TURN_ATTACK_B; DiablosAttack 16 at frame 50 */
    DIAB_MOT_ATTACK_C = 0x6C,      /* DIAB_ATK_ATTACK_C */
    DIAB_MOT_ATTACK_D = 0x6D,      /* DIAB_ATK_TURN_ATTACK_D; DiablosAttack 30 at frame 54 */
    DIAB_MOT_ROAR = 0x6F,          /* DIAB_ATK_ROAR, both scenes, DIAB_VS_ROAR */
    DIAB_MOT_PART6_BREAK = 0x71,   /* DIAB_REACT_PART6_BREAK only; no effects case */
    DIAB_MOT_HORN_STUCK = 0x72,    /* DIAB_IDLE_HORN_STUCK after EmDiablos_ChargeHitWall, 240 frames */
    DIAB_MOT_HORN_PULL_FREE = 0x73 /* end of DIAB_IDLE_HORN_STUCK; falling debris at frame 68 */
};

/* Group 0, EM_GROUP_IDLE (EmDiablos_RunIdleState): standing, idle actions, resting, the horn stuck in a wall, lying under a status and shaking it off. */
enum EmDiablosIdleState {
    DIAB_IDLE_STAND = 0x00,             /* idle on all three layers, scriptPending when em->timer ends */
    DIAB_IDLE = 0x01,                   /* DIAB_MOT_IDLE once; EmDiablos_EndState's target */
    DIAB_IDLE_HORN_STUCK = 0x02,        /* entered after a charge's wall hit; HORN_STUCK then HORN_PULL_FREE */
    DIAB_IDLE_ALERT_BRIEF = 0x03,       /* ALERT_IDLE for 10 frames; burrow refused on hard floor */
    DIAB_IDLE_ONE_LOOP = 0x04,          /* one idle loop on layer 0 */
    DIAB_IDLE_UPPER_A = 0x05,           /* layer-1 gesture DIAB_MOT_UPPER_A */
    DIAB_IDLE_UPPER_B = 0x06,           /* layer-1 gesture DIAB_MOT_UPPER_B */
    DIAB_IDLE_UPPER_E = 0x07,           /* layer-1 gesture DIAB_MOT_UPPER_E */
    DIAB_IDLE_NOP_A = 0x08,             /* empty function */
    DIAB_IDLE_PAIR_A = 0x09,            /* DIAB_IDLE_PAIR_A or _B at random */
    DIAB_IDLE_TURN_TO_GOAL = 0x0A,      /* TURN_IN_PLACE, rotY snapped to goalYaw */
    DIAB_IDLE_UPPER_C = 0x0B,           /* layer-1 gesture DIAB_MOT_UPPER_C */
    DIAB_IDLE_NOP_B = 0x0C,             /* empty function */
    DIAB_IDLE_UPPER_D = 0x0D,           /* layer-1 gesture DIAB_MOT_UPPER_D */
    DIAB_IDLE_NOP_C = 0x0E,             /* empty function */
    DIAB_IDLE_PAIR_B = 0x0F,            /* PAIR_A then PAIR_B */
    DIAB_IDLE_PAIR_C = 0x10,            /* PAIR_B then PAIR_A */
    DIAB_IDLE_ALERT = 0x11,             /* combat idle DIAB_MOT_ALERT_IDLE once */
    DIAB_IDLE_REST = 0x12,              /* lie down, rest 1800 frames, hp regen; Em_ResetNotice */
    DIAB_IDLE_NOP_D = 0x13,             /* empty function */
    DIAB_IDLE_STATUS2_COLLAPSE = 0x14,  /* Em_BeginStatus2; EM_DMG_STATUS2 on the ground */
    DIAB_IDLE_STATUS2_LIE_DOWN = 0x15,  /* Em_BeginStatus2; LIE_DOWN, 600 frames regen */
    DIAB_IDLE_PAIR_D = 0x16,            /* DIAB_IDLE_PAIR_A once */
    DIAB_IDLE_REST_END = 0x17,          /* REST_GET_UP after the rest; Em_RefillNeed3 */
    DIAB_IDLE_STATUS2_END = 0x18,       /* Em_EndStatus2; GET_UP */
    DIAB_IDLE_SHAKE_OFF = 0x19,         /* SHAKE_OFF after every get-up */
    DIAB_IDLE_NOP_E = 0x1A,             /* empty function */
    DIAB_IDLE_STATUS1_COLLAPSE = 0x1B,  /* Em_BeginStatus1; EM_DMG_CAPTURED (not held) on the ground */
    DIAB_IDLE_STATUS1_END = 0x1C,       /* Em_EndStatus1; GET_UP (DIAB_IDLE_STATUS2_END) */
    DIAB_IDLE_SAND_DOWNED = 0x1D,       /* status 1/2 under the sand: DIAB_MOT_POP_OUT then DIAB_MOT_DOWNED */
    DIAB_IDLE_STATUS4_SHAKE_OFF = 0x21, /* Em_EndStatus4; from DIAB_REACT_STATUS4 (status 4) */
    DIAB_IDLE_HOLD2_SHAKE_OFF = 0x22,   /* holdTimer2 = 0; from DIAB_REACT_HOLD2 (Em_SetHold2) */
    DIAB_IDLE_STUN_SHAKE_OFF = 0x23,    /* Em_EndStatus5; from DIAB_REACT_KNOCK_DOWN (stun stars) */
    DIAB_IDLE_ALERT_RAGE = 0x24,        /* Em_StartRage; ALERT_IDLE */
    DIAB_IDLE_ALERT_PENDING_RAGE = 0x25 /* Em_StartPendingRage; ALERT_IDLE */
};

/* Group 1, EM_GROUP_MOVE (EmDiablos_RunMoveState): walking, running, limping and turning. */
enum EmDiablosMoveState {
    DIAB_MOVE_WALK = 0x00,             /* DIAB_MOT_WALK; LIMP below 30% hp */
    DIAB_MOVE_END = 0x01,              /* setter sets scriptPending and returns */
    DIAB_MOVE_RUN = 0x02,              /* DIAB_MOT_RUN; RUN_AND_STOP in combat; a wall hit gives DIAB_IDLE_HORN_STUCK */
    DIAB_MOVE_TURN = 0x03,             /* EmDiablos_TurnParams[0]; TURN_IN_COMBAT in combat */
    DIAB_MOVE_RUN_AND_STOP = 0x04,     /* RUN then CHARGE_STOP */
    DIAB_MOVE_TURN_IN_COMBAT = 0x05,   /* EmDiablos_TurnParams[1] */
    DIAB_MOVE_LIMP = 0x06,             /* DIAB_MOT_LIMP, timer capped at 300 */
    DIAB_MOVE_TURN_TO_GOAL = 0x07,     /* TURN_IN_PLACE, 0x200 a frame toward the goal */
    DIAB_MOVE_RUN_AND_STOP_SLOW = 0x08 /* DIAB_MOVE_RUN_AND_STOP at motionSpeed 0.7 */
};

/* Group 2, EM_GROUP_FLY (EmDiablos_RunBurrowState): under the sand, emerging, and the leap and its states in the air. */
enum EmDiablosBurrowState {
    DIAB_BURROW_DIG = 0x00,          /* DIAB_MOT_BURROW, shadow hidden from frame 100, ends under the sand */
    DIAB_BURROW_IDLE = 0x01,         /* idle motion in EM_STANCE_UNDER; EmDiablos_Start and EmDiablos_EndStateUnderground end in it */
    DIAB_BURROW_EMERGE = 0x02,       /* DIAB_MOT_EMERGE, EM_STANCE_UNDER to EM_STANCE_GROUND, then EmDiablos_EndState */
    DIAB_BURROW_TURN = 0x03,         /* EmDiablos_TurnParams[2] turn in EM_STANCE_UNDER */
    DIAB_BURROW_TRAVEL_RUMBLE = 0x04, /* DIAB_MOT_RUN under the sand, with camera shake and pad vibration */
    DIAB_BURROW_WALK = 0x05,         /* WALK in EM_STANCE_UNDER, Em_StepToGoal */
    DIAB_BURROW_LEAP = 0x06,         /* DIAB_MOT_LEAP, curve kind 2, EM_STANCE_AIR from frame 70 */
    DIAB_BURROW_AIR_TURN = 0x07,     /* AIR, Em_TurnToGoal up to 150 frames */
    DIAB_BURROW_TRAVEL = 0x08,       /* DIAB_MOT_RUN under the sand to the goal, camera shake only */
    DIAB_BURROW_AIR_RISE = 0x09,     /* AIR, velY 20 to goal height + 1000 */
    DIAB_BURROW_WAIT = 0x0A,         /* idle in EM_STANCE_UNDER, 90 frames */
    DIAB_BURROW_AIR_TO_GOAL = 0x0B,  /* AIR, turn and pitch to the goal */
    DIAB_BURROW_AIR_LAND = 0x0C,     /* AIR, DESCEND, LAND; DIAB_DEFEAT_REAPPEAR enters it */
    DIAB_BURROW_AIR_LOOP = 0x0E,     /* AIR once; the target of EmDiablos_EndStateInAir */
    DIAB_BURROW_SAND_ESCAPE = 0x17   /* LEAP out after the sand struggles, LAND */
};

/* Group 3, EM_GROUP_ATTACK (EmDiablos_RunAttackState): the stare, the charges, the tail spin, the turning attacks and the roar. */
enum EmDiablosAttackState {
    DIAB_ATK_TURN_AND_STARE = 0x00, /* turn, STARE; EmDiablos_SetAttackState sets the target angle */
    DIAB_ATK_STARE = 0x03,          /* STARE with lookAtTarget */
    DIAB_ATK_TAIL_SPIN = 0x06,      /* TAIL_SPIN twice, rotY + 0x200 */
    DIAB_ATK_CHARGE = 0x0F,         /* DIAB_MOT_RUN, DIAB_MOT_CHARGE_STOP; a wall hit gives DIAB_IDLE_HORN_STUCK */
    DIAB_ATK_CHARGE_ATTACK_A = 0x10, /* the charge, then DIAB_MOT_ATTACK_A */
    DIAB_ATK_CHARGE_ATTACK_B = 0x11, /* the charge, then DIAB_MOT_ATTACK_B */
    DIAB_ATK_TURN_ATTACK_A = 0x1A,  /* turn, ATTACK_A */
    DIAB_ATK_TURN_ATTACK_B = 0x1B,  /* turn, ATTACK_B */
    DIAB_ATK_ATTACK_C = 0x1C,       /* ATTACK_C, no turn */
    DIAB_ATK_TURN_ATTACK_D = 0x1D,  /* turn, ATTACK_D */
    DIAB_ATK_ROAR = 0x1E,           /* ROAR */
    DIAB_ATK_ATTACK_C_B = 0x1F      /* runs DIAB_ATK_ATTACK_C's function; EmDiablos_SetAttackState has no case for it */
};

/* Group 4, EM_GROUP_REACT (EmDiablos_RunReactState): what EmDiablos_Update picks from Em_UpdateDamage's result. */
enum EmDiablosReactState {
    DIAB_REACT_FLINCH = 0x00,               /* parts 0/7, EM_DMG_HIT_MODE3 */
    DIAB_REACT_FLINCH_B = 0x01,             /* unbroken part 3/4 */
    DIAB_REACT_FLINCH_C = 0x02,             /* part 5, unbroken part 6 */
    DIAB_REACT_FLINCH_D = 0x03,             /* parts 1/2 */
    DIAB_REACT_PART_BREAK = 0x04,           /* EM_DMG_PART8_DOWN, partBreakCount++ */
    DIAB_REACT_KNOCK_DOWN = 0x05,           /* broken part other than 3; EmDiablos_KnockDown variant 0 */
    DIAB_REACT_POP_OUT = 0x06,              /* EM_DMG_ATTACK5/_FLINCH or part flinch in EM_STANCE_UNDER */
    DIAB_REACT_PART6_BREAK = 0x07,          /* part 6 flinch with part 6 broken (per kind) */
    DIAB_REACT_FALL = 0x08,                 /* EM_DMG_STATUS_IN_AIR, flinch in the air */
    DIAB_REACT_FALL_B = 0x09,               /* FALL with blind 2; no caller here */
    DIAB_REACT_FLINCH_E = 0x0A,             /* FLINCH_E; no caller here */
    DIAB_REACT_STATUS4 = 0x0B,              /* EM_DMG_STATUS4 out of the sand, Em_BeginStatus4 */
    DIAB_REACT_SAND_STRUGGLE = 0x0D,        /* after STATUS2_SAND_GET_UP; clears trapHit */
    DIAB_REACT_STATUS4_IN_SAND = 0x0E,      /* EM_DMG_STATUS4 in EM_STANCE_UNDER or a sand state; EmDiablos_HitDownedInSand variant 0 */
    DIAB_REACT_PART_BREAK_END = 0x0F,       /* EmDiablos_ChangeState breaks part 2; after PART_BREAK */
    DIAB_REACT_KNOCK_DOWN_PART3 = 0x10,     /* broken part 3; EmDiablos_KnockDown variant 1 */
    DIAB_REACT_STATUS4_SAND_STRUGGLE = 0x11, /* Em_EndStatus4 on entry; after STATUS4_IN_SAND */
    DIAB_REACT_STATUS2_SAND_GET_UP = 0x12,  /* Em_EndStatus2 on entry; DOWNED_GET_UP */
    DIAB_REACT_HOLD2 = 0x13,                /* EM_DMG_ATTACK13, Em_SetHold2 */
    DIAB_REACT_STUN = 0x14,                 /* EM_DMG_STATUS5 with (hitAngle - rotY) > 0x8000; EmDiablos_KnockDown variant 2 */
    DIAB_REACT_STUN_B = 0x15,               /* EM_DMG_STATUS5 from the other side; EmDiablos_KnockDown variant 3 */
    DIAB_REACT_STUN_IN_SAND = 0x16,         /* EM_DMG_STATUS5 in EM_STANCE_UNDER or a sand state; EmDiablos_HitDownedInSand variant 1 */
    DIAB_REACT_STUN_SAND_STRUGGLE = 0x17    /* Em_EndStatus5 on entry; after STUN_IN_SAND */
};

/* Group 5, EM_GROUP_DEFEAT (EmDiablos_RunDefeatState): death on the ground, in the sand and falling, and appearing again. */
enum EmDiablosDefeatState {
    DIAB_DEFEAT_DIE = 0x00,        /* DIAB_MOT_DIE, QuestEm_Hunted, carve point, fade; EmDiablos_Update's pick for a death on the ground */
    DIAB_DEFEAT_DIE_IN_SAND = 0x01, /* pushed out of the sand then DIAB_MOT_DOWNED */
    DIAB_DEFEAT_DIE_FALLING = 0x02, /* fall, land, DIAB_MOT_DIE_FALLING; EmDiablos_Update's pick in EM_STANCE_AIR */
    DIAB_DEFEAT_REAPPEAR = 0x03    /* QuestEm_Appear (EmDiablos_ChangeState), back in DIAB_BURROW_AIR_LAND or the task ends */
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmDiablos_RunEventState): the desert scenes and the capture. */
enum EmDiablosEventState {
    DIAB_EVENT_DIABLOS_SCENE = 0x00, /* quest stage QUEST_ID_DIABLOS_SCENE, EVENT_FLAG_DIABLOS_SCENE, Diablos kinds */
    DIAB_EVENT_MONOBLOS_SCENE = 0x01, /* quest stage QUEST_ID_MONOBLOS_SCENE, EVENT_FLAG_MONOBLOS_SCENE, Monoblos kinds */
    DIAB_EVENT_CAPTURED = 0x02       /* EM_DMG_CAPTURED with holdTimer2 */
};

/* Group 8, EM_GROUP_VERSUS (EmDiablos_RunVersusState): the versus quests' bait and poses. */
enum EmDiablosVersusState {
    DIAB_VS_BAIT = 0x00,     /* VersusBait_Create, STARE, Versus_RouteEmByTeam */
    DIAB_VS_POSE_A = 0x01,   /* DIAB_MOT_FLINCH_E, then DIAB_VS_WAIT_BOTH */
    DIAB_VS_POSE_B = 0x02,   /* DIAB_MOT_TURN_IN_PLACE, then DIAB_VS_WAIT_BOTH */
    DIAB_VS_ROAR = 0x03,     /* DIAB_MOT_ROAR, then DIAB_VS_WAIT_BOTH */
    DIAB_VS_WAIT_BOTH = 0x98, /* Versus_MarkDefeat, Versus_BothDefeated */
    DIAB_VS_END = 0x99       /* 90 frames, then EmDiablos_EndState */
};

/* Diablos's own meanings of the work area's bytes (Em.work; EmWork has the
 * shared ones). */
typedef struct EmDiablosWork {
    u8 fxStep;                  /* 0x000: EmDiablos_UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x006 - 0x001];
    s16 landTimer;              /* 0x006: frames since the landing after DIAB_BURROW_SAND_ESCAPE (150; EmDiablos_Update counts it down) */
    u8 unk008[0x014 - 0x008];
    u16 goalYaw;                /* 0x014: as EmWork's (EmDiablos_Init: ANGLE_TURN / 4) */
    u8 unk016[0x01B - 0x016];
    s8 rageFade;                /* 0x01B: EmDiablos_Update: counts up to 60 while enraged (then rageStep 1), down to 0 otherwise */
    s32 moveAngBX;              /* 0x01C: as EmWork's */
    s32 moveAngBY;              /* 0x020 */
    u8 unk024[0x030 - 0x024];
    s32 moveAngX;               /* 0x030 */
    s32 moveAngY;               /* 0x034 */
    s32 moveAngZ;               /* 0x038 */
    s32 pitchStep;              /* 0x03C */
    s32 turnSpeed;              /* 0x040 */
    s32 rollSpeed;              /* 0x044 */
    u8 curveNone;               /* 0x048: the three curves' run flags (EmDiablos_StartCurves, EmDiablos_UpdateCurves) */
    u8 curveVelY;               /* 0x049 */
    u8 curveSpeed;              /* 0x04A */
    u8 curveKind;               /* 0x04B: the curve tables' index (EmDiablos_StartCurves) */
    s16 curveFrame;             /* 0x04C: the curve frame (EmDiablos_StartCurves, EmDiablos_UpdateCurves, EmDiablos_CurveVelY, EmDiablos_CurveSpeed) */
    s16 quakeTimer;             /* 0x04E: the travels under the sand: frames to the next camera shake */
    u8 unk050[0x100 - 0x050];
} EmDiablosWork;
typedef char EmDiablosWork_size_check[(sizeof(EmDiablosWork) == sizeof(EmWork)) ? 1 : -1];

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound and effect helpers read the position from). The
 * names say what the family's code shows a bone to be: the bones of the
 * attack hit shapes DiablosAttack_Create makes on given frames of a motion
 * (DiablosAttack_HitShapes, by the attack rows' hit lists), where the dust,
 * the sparks or the rings start, and the bones the steps' sounds take
 * turns at. "A" and "B" are the two sides; which is left the code doesn't
 * show. Bones no code explains stay numbers: 1 (the mine ahead of it while
 * the horn is stuck), 3 (the charge's wide sphere), 9, 15 and 27 (the
 * dust of the burrowing and the flinches), 24 (a sphere in
 * DIAB_MOT_CHARGE_STOP), 29 and 30 (the burrowing's spheres) and 43 (dust
 * where the tail strikes in DIAB_MOT_ATTACK_C).
 */
enum EmDiablosBone {
    DIAB_BONE_ROOT = 0,     /* the skeleton's first bone: the falls' and the burrowing's sounds play here */
    DIAB_BONE_BODY = 2,     /* the body's wide sphere in DIAB_MOT_COLLAPSE and DIAB_MOT_EMERGE; the dust and bursts of the descent, the charge's stop and the pop out of the sand */
    DIAB_BONE_WING_A = 6,   /* the flight's (DIAB_MOT_AIR, DIAB_MOT_DESCEND, DIAB_MOT_LEAP) paired sounds play here and at DIAB_BONE_WING_B, four frames apart */
    DIAB_BONE_WING_B = 12,
    DIAB_BONE_LEG_A = 19,   /* leg A's upper sphere in the walk's and the turns' steps (with DIAB_BONE_FOOT_A's) */
    DIAB_BONE_STEP_A = 20,  /* leg A's step sounds (DIAB_SE_STEP in the walk and the turns); DustCloud type 3 rises under it */
    DIAB_BONE_FOOT_A = 22,  /* leg A's foot: the steps' larger sphere, and a landing on both feet (DIAB_MOT_LAND) */
    DIAB_BONE_LEG_B = 25,   /* the same for leg B */
    DIAB_BONE_STEP_B = 26,  /* (DustCloud type 2 rises under it) */
    DIAB_BONE_FOOT_B = 28,
    DIAB_BONE_BREATH = 34,  /* the lying puffs start here and the roar's rings follow it; the head's sphere in the horn attacks, the charge and the stare */
    DIAB_BONE_HEAD = 35,    /* every voice sound (Em_PlaySeType*) plays here */
    DIAB_BONE_HORN = 39,    /* the sparks where the horn (part 6) sticks in a wall, in DIAB_MOT_HORN_STUCK */
    DIAB_BONE_TAIL_A = 41,  /* the tail's capsule, to DIAB_BONE_TAIL_C, in DIAB_MOT_TAIL_SPIN, DIAB_MOT_ATTACK_C, DIAB_MOT_STARE and DIAB_MOT_ALERT_IDLE */
    DIAB_BONE_TAIL_B = 42,  /* the tail spin's swing sound plays here as the tail's hit starts (frame 30) */
    DIAB_BONE_TAIL_C = 44   /* the capsule's other end; the tail strike's sounds in DIAB_MOT_ATTACK_C */
};

/*
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind; Em_PlaySeAtBone and Em_PlaySeType). A
 * sound is named where the code shows what it is: it plays on the frame of
 * the thing it goes with. The others stay numbers.
 */
enum EmDiablosSe {
    DIAB_SE_STEP = 1,       /* a step: in DIAB_MOT_WALK on the frames of its camera shakes (52 and 116), at the step bones */
    DIAB_SE_ROAR = 91       /* the roar: DIAB_MOT_ROAR's only voice from frame 100, repeated every 4 frames to frame 320, over the rings (frame 112) and the roar's wide sphere */
};

extern void *EmDiablos_Callbacks[8];
void EmDiablos_MotionSounds(Em *em, EmWork *work);
void EmDiablos_Init(Em *em);
void EmDiablos_Start(Em *em);
void EmDiablos_EndStateInAir(Em *em, u8 variant);
void EmDiablos_HornStuck(Em *em, EmWork *work);
void EmDiablos_StatusDownedInSand(Em *em, EmWork *work);
void EmDiablos_TurnToGoal(Em *em, EmWork *work);
void EmDiablos_BurrowTravelRumble(Em *em, EmWork *work);
void EmDiablos_BurrowTravel(Em *em, EmWork *work);
void EmDiablos_DiablosDesertScene(Em *em, EmWork *work);
void EmDiablos_MonoblosDesertScene(Em *em, EmWork *work);
void EmDiablos_VsBait(Em *em, EmWork *work);
void EmDiablos_Die(Em *em, EmWork *work);
void EmDiablos_DieInSand(Em *em, EmWork *work);
void EmDiablos_DieFalling(Em *em, EmWork *work);
void EmDiablos_RunReactState(Em *em, EmWork *work);
void EmDiablos_Update(Em *em);
void EmDiablos_RunState(Em *em, EmWork *work);
void EmDiablos_MotionEffects(Em *em, EmWork *work);
void EmDiablos_EndAttack(Em *em, EmWork *work);
void EmDiablos_SetIdleState(Em *em, u16 state, u8 mode);
void EmDiablos_SetMoveState(Em *em, u16 state, u8 mode);
void EmDiablos_SetBurrowState(Em *em, u16 state, u8 mode);
void EmDiablos_SetAttackState(Em *em, u16 state, u8 mode);
void EmDiablos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmDiablos_MoveByVelYCurve(Em *em, s32 kind);
void EmDiablos_StartCurves(Em *em, u8 kind);
u8 EmDiablos_UpdateCurves(Em *em);
s32 EmDiablos_IsBuriedOffSand(Em *em);
s32 EmDiablos_ChargeHitWall(Em *em);
s32 EmDiablos_InSandStuckMotion(Em *em);
s32 EmDiablos_InSandReactState(Em *em);

#endif
