#ifndef EM_DROME_H
#define EM_DROME_H

#include "em.h"

/*
 * The dromes' vocabulary (the code in em_drome.c), for the Velocidrome, the
 * Gendrome and the Iodrome (EM_KIND_VELOCIDROME, EM_KIND_GENDROME,
 * EM_KIND_IODROME), the leaders of the raptor packs. They share the
 * Velociprey's motion table, so these motions line up with em_velociprey.h's.
 *
 * Motions: row N of the motion table plays motion N, so one number serves
 * Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion) kept in
 * Em.motionId. The names say what the states that play a motion show;
 * motions no state plays stay numbers.
 *
 * Group 2 (EM_GROUP_FLY's slot) holds the two hops on the curve tables, as
 * the Velociprey's does. Group 7 (EM_GROUP_EVENT2) has its own dispatcher:
 * the hops into the area after a reappearance.
 */
enum EmDromeMotion {
    DROME_MOT_IDLE = 0x01,                /* DROME_IDLE, the Init row, the scenes and VS waits (tested in Em.motionId) */
    DROME_MOT_WALK = 0x02,                /* DROME_MOVE_WALK, the Jungle scene walk */
    DROME_MOT_HOP = 0x03,                 /* HOP_FORWARD, EVENT2_HOP_IN/LEAP_DOWN_IN; airborne from frame 10; dust at 18 */
    DROME_MOT_HOP_UP = 0x04,              /* HOP_UP; airborne from frame 22; dust at 20 */
    DROME_MOT_HOP_LAND = 0x05,            /* the landing of every hop */
    DROME_MOT_TURN_LEFT = 0x06,           /* EmDrome_TurnParams motLeft (both rows) */
    DROME_MOT_TURN_RIGHT = 0x07,          /* EmDrome_TurnParams motRight */
    DROME_MOT_MOVE_ACTION_A = 0x08,       /* DROME_MOVE_ACTION_A; dust at frame 12 */
    DROME_MOT_MOVE_ACTION_B = 0x09,       /* DROME_MOVE_ACTION_B; dust at frame 14 */
    DROME_MOT_MOVE_ACTION_C = 0x0A,       /* DROME_MOVE_ACTION_C; dust at frame 14 */
    DROME_MOT_ALERT = 0x0B,               /* DROME_IDLE_ALERT, STATUS4_END, the Jungle scene */
    DROME_MOT_IDLE_B = 0x0C,              /* DROME_IDLE_REAPPEAR only */
    DROME_MOT_LEAP = 0x14,                /* the LEAP_* attacks; airborne from frame 22 */
    DROME_MOT_LEAP_LAND = 0x15,           /* their landing */
    DROME_MOT_STRIKE = 0x16,              /* STRIKE_A/B/C; DromeAttack at frame 40 */
    DROME_MOT_POUNCE = 0x17,              /* DROME_ATK_POUNCE; DromeAttack 2 at 32, PartFx at DROME_BONE_POUNCE_DUST at 22 (EmDrome_MotionEffects) */
    DROME_MOT_POUNCE_LAND = 0x18,         /* its landing */
    DROME_MOT_QUICK_STRIKE = 0x19,        /* QUICK_STRIKE_A/B; DromeAttack at frame 12 */
    DROME_MOT_SPIT = 0x1A,                /* DROME_ATK_SPIT(_FAST); spit from DROME_BONE_MOUTH at frame 80 */
    DROME_MOT_FLINCH = 0x1E,              /* DROME_REACT_FLINCH, DROME_REACT_HOLD2 start */
    DROME_MOT_KNOCKED_BACK = 0x1F,        /* DROME_REACT_KNOCKED_BACK, DIE_BACK */
    DROME_MOT_KNOCKED_BACK_LAND = 0x20,   /* their landing */
    DROME_MOT_KNOCKED_FORWARD = 0x21,     /* DROME_REACT_KNOCKED_FORWARD, DIE_FORWARD */
    DROME_MOT_KNOCKED_FORWARD_LAND = 0x22, /* their landing (turning to frame 26) */
    DROME_MOT_KNOCKED_SIDE_B = 0x23,      /* DROME_REACT_KNOCKED_SIDE_B, DIE_SIDE_B */
    DROME_MOT_KNOCKED_SIDE_B_LAND = 0x24, /* their landing */
    DROME_MOT_KNOCKED_SIDE_A = 0x25,      /* DROME_REACT_KNOCKED_SIDE_A, DIE_SIDE_A */
    DROME_MOT_KNOCKED_SIDE_A_LAND = 0x26, /* their landing */
    DROME_MOT_GET_UP = 0x27,              /* DROME_IDLE_GET_UP, STATUS1/2_END */
    DROME_MOT_DEAD = 0x29,                /* the carcass of DIE_* with the carve point */
    DROME_MOT_DAZED = 0x2A,               /* DROME_IDLE_BLINDED (tested in Em.motionId), DROME_REACT_DAZE, DROME_REACT_STUN, DROME_VS_POSE_A */
    DROME_MOT_COLLAPSE = 0x2B,            /* DROME_IDLE_STATUS1/2, DROME_EVENT_CAPTURED */
    DROME_MOT_LIE = 0x2C,                 /* lying after the collapse; puffs at DROME_BONE_MOUTH each frame */
    DROME_MOT_DIE = 0x2E,                 /* DIE_* after the landing; field collision off */
    DROME_MOT_STATUS4 = 0x2F,             /* DROME_REACT_STATUS4, DROME_REACT_HOLD2 until the timer */
    DROME_MOT_THREAT = 0x32,              /* DROME_IDLE_THREAT, DROME_IDLE_RAGE (rage start), the Jungle scene, DROME_VS_POSE_C */
    DROME_MOT_CALL = 0x33,                /* DROME_IDLE_PACK_CALL (sense object at 40), DROME_IDLE_PENDING_RAGE, DROME_VS_POSE_B */
    DROME_MOT_SHAKE_OFF = 0x34,           /* DROME_IDLE_SHAKE_OFF, HOLD2_SHAKE_OFF, STUN_END */
    DROME_MOT_VS_BAIT = 0x35              /* DROME_VS_EAT_BAIT only, bait at frame 10 (em_velociprey.h names the shared row VPREY_MOT_EVENT_IDLE) */
};

/* Group 0, EM_GROUP_IDLE (EmDrome_RunIdleState): standing, threats, the pack call, statuses and rage. */
enum EmDromeIdleState {
    DROME_IDLE_NONE = 0x00,           /* EmDrome_IdleNone does nothing; EVENT2_TO_IDLE_A/B go here */
    DROME_IDLE = 0x01,                /* EmDrome_EndState/Start; idle motion, scriptPending; ALERT in combat with hp >= 30% */
    DROME_IDLE_THREAT = 0x02,         /* DROME_MOT_THREAT once */
    DROME_IDLE_PACK_CALL = 0x03,      /* call object in the sense list at frame 40 */
    DROME_IDLE_SHAKE_OFF = 0x04,      /* DROME_MOT_SHAKE_OFF once */
    DROME_IDLE_ALERT = 0x05,          /* combat idle DROME_MOT_ALERT; the setter pick for IDLE and DROME_IDLE_WAIT */
    DROME_IDLE_GET_UP = 0x06,         /* after REACT_KNOCKED_*; noReact set */
    DROME_IDLE_STATUS2 = 0x07,        /* EM_DMG_STATUS2; Em_BeginStatus2; collapse and lie until the timer */
    DROME_IDLE_STATUS2_END = 0x08,    /* Em_EndStatus2; get up; also EM_DMG_HIT_LYING */
    DROME_IDLE_BLINDED = 0x09,        /* DROME_MOT_DAZED until blindTimer 0 */
    DROME_IDLE_STATUS4_END = 0x0A,    /* from DROME_REACT_STATUS4; Em_EndStatus4 */
    DROME_IDLE_REAPPEAR = 0x0B,       /* DROME_DEFEAT_REAPPEAR outside the hop-in areas; DROME_MOT_IDLE_B */
    DROME_IDLE_WAIT = 0x0C,           /* runs EmDrome_Idle; the setter may pick ALERT against a far player */
    DROME_IDLE_STATUS1 = 0x0D,        /* Em_BeginStatus1; collapse and lie (variant 1); HIT_LYING ends it */
    DROME_IDLE_STATUS1_END = 0x0E,    /* Em_EndStatus1; get up */
    DROME_IDLE_HOLD2_SHAKE_OFF = 0x0F, /* from DROME_REACT_HOLD2; clears holdTimer2 */
    DROME_IDLE_STUN_END = 0x10,       /* from DROME_REACT_STUN; Em_EndStatus5 */
    DROME_IDLE_RAGE = 0x11,           /* Em_StartRage; DROME_MOT_THREAT */
    DROME_IDLE_PENDING_RAGE = 0x12    /* Em_StartPendingRage; DROME_MOT_CALL */
};

/* Group 1, EM_GROUP_MOVE (EmDrome_RunMoveState): turning, walking and the moves aside. */
enum EmDromeMoveState {
    DROME_MOVE_TURN = 0x00,      /* EmDrome_TurnParams[0]; TURN_ALERT in combat */
    DROME_MOVE_WALK = 0x01,      /* Em_StepToGoal; stuck: HOP_FORWARD */
    DROME_MOVE_ACTION_A = 0x02,  /* DROME_MOT_MOVE_ACTION_A */
    DROME_MOVE_ACTION_B = 0x03,  /* DROME_MOT_MOVE_ACTION_B */
    DROME_MOVE_ACTION_C = 0x04,  /* DROME_MOT_MOVE_ACTION_C */
    DROME_MOVE_TURN_ALERT = 0x05 /* EmDrome_TurnParams[1] */
};

/* Group 2, EM_GROUP_FLY (EmDrome_RunHopState): the hops. */
enum EmDromeHopState {
    DROME_HOP_FORWARD = 0x00, /* DROME_CURVE_HOP_FORWARD, DROME_MOT_HOP */
    DROME_HOP_UP = 0x01      /* DROME_CURVE_HOP_UP, DROME_MOT_HOP_UP */
};

/* Group 3, EM_GROUP_ATTACK (EmDrome_RunAttackState): the strikes, the pounce, the leaps and the spit. */
enum EmDromeAttackState {
    DROME_ATK_STRIKE_A = 0x00,      /* the kind's DromeAttack 1/13/7 at frame 40 */
    DROME_ATK_POUNCE = 0x01,        /* DROME_CURVE_POUNCE, DROME_MOT_POUNCE */
    DROME_ATK_LEAP_MEDIUM_A = 0x02, /* velY 75 velZ 50; the kind's 3/8/9 */
    DROME_ATK_QUICK_STRIKE_A = 0x03, /* frame 12; the kind's 5/10/11 */
    DROME_ATK_LEAP_LONG_A = 0x04,   /* velZ 100; the kind's 3/8/9 */
    DROME_ATK_LEAP_HIGH_A = 0x05,   /* velY 150; the kind's 3/8/9 */
    DROME_ATK_SPIT = 0x06,          /* prey spit variant 1 */
    DROME_ATK_STRIKE_B = 0x07,      /* DromeAttack 6 */
    DROME_ATK_LEAP_MEDIUM_B = 0x08, /* DromeAttack 3 */
    DROME_ATK_QUICK_STRIKE_B = 0x09, /* DromeAttack 5 */
    DROME_ATK_LEAP_LONG_B = 0x0A,   /* DromeAttack 3 */
    DROME_ATK_LEAP_HIGH_B = 0x0B,   /* DromeAttack 3 */
    DROME_ATK_LEAP_TO_GOAL = 0x0C,  /* velY aimed above a higher goal */
    DROME_ATK_STRIKE_C = 0x0D,      /* DromeAttack 12 */
    DROME_ATK_SPIT_FAST = 0x0E      /* prey spit variant 3 (double speed) */
};

/* Group 4, EM_GROUP_REACT (EmDrome_RunReactState): what EmDrome_Update picks from Em_UpdateDamage's result. */
enum EmDromeReactState {
    DROME_REACT_FLINCH = 0x00,         /* DROME_MOT_FLINCH; PART_FLINCH/HIT on the ground */
    DROME_REACT_KNOCKED_BACK = 0x01,   /* hit from three to five eighths of a turn off the heading (the back) */
    DROME_REACT_KNOCKED_FORWARD = 0x02, /* hit angle near the heading */
    DROME_REACT_KNOCKED_SIDE_A = 0x03, /* hit from one to three eighths of a turn off the heading */
    DROME_REACT_KNOCKED_SIDE_B = 0x04, /* hit from five to seven eighths of a turn off the heading */
    DROME_REACT_DAZE = 0x05,           /* DROME_MOT_DAZED once; not picked in this file */
    DROME_REACT_STATUS4 = 0x06,        /* EM_DMG_STATUS4; Em_BeginStatus4 */
    DROME_REACT_HOLD2 = 0x07,          /* EM_DMG_ATTACK13; Em_EndAllStatus, Em_SetHold2 */
    DROME_REACT_STUN = 0x08            /* EM_DMG_STATUS5; Em_BeginStatus5, stun stars */
};

/* Group 5, EM_GROUP_DEFEAT (EmDrome_RunDefeatState): death, by the side it falls on, and appearing again. */
enum EmDromeDefeatState {
    DROME_DEFEAT_DIE_BACK = 0x00,   /* killed, hit from three to five eighths of a turn off the heading; carcass to carve */
    DROME_DEFEAT_DIE_FORWARD = 0x01, /* killed by a hit near the heading */
    DROME_DEFEAT_DIE_SIDE_A = 0x02, /* killed, hit from one to three eighths of a turn off the heading */
    DROME_DEFEAT_DIE_SIDE_B = 0x03, /* killed, hit from five to seven eighths of a turn off the heading */
    DROME_DEFEAT_REAPPEAR = 0x04    /* QuestEm_Appear; appear point by area */
};

/* Group 6, EM_GROUP_EVENT (EmDrome_RunEventState): the capture and the jungle scene. */
enum EmDromeEventState {
    DROME_EVENT_CAPTURED = 0x01,    /* EM_DMG_CAPTURED; QuestEm_Captured, fades, reappears */
    DROME_EVENT_JUNGLE_SCENE = 0x02 /* Update in AREA_JUNGLE_7 while gScene.eventRunning */
};

/* Group 7, EM_GROUP_EVENT2 (EmDrome_RunEvent2State): the hops into the area after a reappearance. */
enum EmDromeEvent2State {
    DROME_EVENT2_HOP_IN = 0x00,      /* AREA_FOREST_HILLS_10, _2 or _1 after DROME_DEFEAT_REAPPEAR; DROME_CURVE_HOP_FORWARD */
    DROME_EVENT2_LEAP_DOWN_IN = 0x01, /* AREA_JUNGLE_7 after DROME_DEFEAT_REAPPEAR, from 1300 up */
    DROME_EVENT2_TO_IDLE_A = 0x02,   /* straight to DROME_IDLE_NONE; not reached from this file */
    DROME_EVENT2_TO_IDLE_B = 0x03    /* the same; not reached from this file */
};

/* Group 8, EM_GROUP_VERSUS (EmDrome_RunVersusState): the versus quests' bait and poses. */
enum EmDromeVersusState {
    DROME_VS_EAT_BAIT = 0x00, /* Versus_DefeatCamera, VersusBait at frame 10 */
    DROME_VS_POSE_A = 0x01,   /* DROME_MOT_DAZED, then WAIT_BOTH */
    DROME_VS_POSE_B = 0x02,   /* DROME_MOT_CALL, then WAIT_BOTH */
    DROME_VS_POSE_C = 0x03,   /* DROME_MOT_THREAT, then WAIT_BOTH */
    DROME_VS_WAIT_BOTH = 0x98, /* Versus_MarkDefeat on entry; waits Versus_BothDefeated */
    DROME_VS_END = 0x99       /* 90 frames then EmDrome_EndState */
};

/*
 * EmDrome_StartCurves' kinds: rows of the curve tables (EmDrome_VelYCurves,
 * EmDrome_SpeedCurves). Kinds 0 and 2 run both curves, 1 and 3 the
 * vertical one only.
 */
enum EmDromeCurveKind {
    DROME_CURVE_UNUSED = 0,             /* no state starts it */
    DROME_CURVE_POUNCE = 1,             /* DROME_ATK_POUNCE */
    DROME_CURVE_HOP_FORWARD = 2,        /* DROME_HOP_FORWARD and DROME_EVENT2_HOP_IN: up and forward */
    DROME_CURVE_HOP_UP = 3              /* DROME_HOP_UP: up only */
};

/* The dromes' own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmDromeWork {
    u8 fxStep;                  /* 0x000: EmDrome_UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x008 - 0x001];
    s32 accelAngX;              /* 0x008: angles X, Y, Z for Em_MoveRotatedAccel (EmDrome_UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x00C */
    s32 accelAngZ;              /* 0x010 */
    u8 curveNone;               /* 0x014: the three curves' run flags (EmDrome_StartCurves, EmDrome_UpdateCurves) */
    u8 curveVelY;               /* 0x015 */
    u8 curveSpeed;              /* 0x016 */
    u8 curveKind;               /* 0x017: the curve tables' index (EmDrome_StartCurves) */
    s16 curveFrame;             /* 0x018: the curve frame (EmDrome_StartCurves, EmDrome_UpdateCurves) */
    s8 countdown;               /* 0x01A: 1 while the pack call (callObj) is in the sense list: EmDrome_PackCall sets it, it and EmDrome_ChangeState clear it */
    u8 unk01B[0x020 - 0x01B];
    EmObj callObj;              /* 0x020: the pack call, an object the other raptors sense (EmDrome_PackCall fills it; EmDrome_ChangeState removes it from the list) */
    u8 unk03C[0x044 - 0x03C];
    f32 wallVelZ;               /* 0x044: the attacks: the velZ kept while a wall stops the jump */
    u8 unk048[0x050 - 0x048];
    s32 unk050;                 /* 0x050: the leap attacks clear it; nothing in the family reads it */
    u8 unk054[0x05E - 0x054];
    u8 hunted;                  /* 0x05E: as EmWork's hunted */
    u8 unk05F[0x100 - 0x05F];
} EmDromeWork;
typedef char EmDromeWork_size_check[(sizeof(EmDromeWork) == sizeof(EmWork)) ? 1 : -1];

/* A variant's row (indexed by Em.variant) in the dromes' layout; EmKind1D_Variants is a
 * table of them. */
typedef struct EmDromeKindEntry {
    s16 part;           /* 0x00: the one mesh shown (Em.meshShown) */
    s16 hp;             /* 0x02: 0 or less: 4000 */
    f32 scale;          /* 0x04 */
    s32 se;             /* 0x08: the sound at death */
} EmDromeKindEntry;

/*
 * Bones: a bone number indexes the model's skeleton (the bone argument of
 * Em_PlaySeAtBoneOnFrame, EmRangedAttack_CreateAtBone, PartFx_SpawnForEm and
 * Em_SpawnLyingPuffs). The names say what the family's code shows a bone to
 * be. The preys share the skeleton, and the Velociprey's code uses these
 * names the same way: its spits and its lying puffs start at
 * DROME_BONE_MOUTH, its stun stars circle DROME_BONE_STUN_STARS, its hops,
 * its moves aside and its pounce raise their dust at
 * DROME_BONE_HOP_DUST and DROME_BONE_POUNCE_DUST, and its sounds play at
 * DROME_BONE_ROOT. Bones no code explains stay numbers: 2 (the idle's random call plays
 * there), 26 (the sphere of DromeAttack's row 4, which no state makes), and
 * 7 and 10 (the spheres of hit list 3, which no row uses).
 *
 * Sounds: a sound number indexes the kind's own sound bank. None is named:
 * the code shows only where each plays (all but the idle's call at
 * DROME_BONE_ROOT), not what it is.
 */
enum EmDromeBone {
    DROME_BONE_ROOT = 0,            /* the skeleton's first bone: the sounds play there, and every attack's sphere is offset from it (DromeAttack_HitShapes' lists 0 and 1) */
    DROME_BONE_HOP_DUST = 1,        /* the dust in the hops and the moves aside (PartFx_SpawnForEm) */
    DROME_BONE_STUN_STARS = 14,     /* where the stun stars circle (StunStars_CreateEm, EmDrome_Stun) */
    DROME_BONE_MOUTH = 15,          /* where the spit starts (EMRANGED_PREY_SPIT, DROME_MOT_SPIT's frame 80) and the lying puffs (DROME_MOT_LIE) */
    DROME_BONE_POUNCE_DUST = 27     /* the pounce's dust (DROME_MOT_POUNCE's frame 22, ten frames before its hit) */
};

extern void *EmDrome_Callbacks[8];
void EmDrome_MotionSounds(Em *em, EmWork *work);
void EmDrome_Init(Em *em);
void EmDrome_Start(Em *em);
void EmDrome_RunIdleState(Em *em, EmWork *work);
void EmDrome_Walk(Em *em, EmWork *work);
void EmDrome_RunAttackState(Em *em, EmWork *work);
void EmDrome_DieBack(Em *em, EmWork *work);
void EmDrome_DieForward(Em *em, EmWork *work);
void EmDrome_DieSideA(Em *em, EmWork *work);
void EmDrome_DieSideB(Em *em, EmWork *work);
void EmDrome_Reappear(Em *em, EmWork *work);
void EmDrome_EventJungleScene(Em *em, EmWork *work);
void EmDrome_VsEatBait(Em *em, EmWork *work);
void EmDrome_Update(Em *em);
void EmDrome_RunState(Em *em, s32 work);
void EmDrome_MotionEffects(Em *em, EmWork *work);
void EmDrome_SetIdleState(Em *em, u16 state, u8 mode);
void EmDrome_SetMoveState(Em *em, u16 state, u8 mode);
void EmDrome_SetHopState(Em *em, u16 state, u8 mode);
void EmDrome_SetAttackState(Em *em, u16 state, u8 mode);
void EmDrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmDrome_StartCurves(Em *em, u8 kind);
u8 EmDrome_UpdateCurves(Em *em);

#endif
