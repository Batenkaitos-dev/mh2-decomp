#ifndef EM_BULLDROME_H
#define EM_BULLDROME_H

#include "em.h"

/*
 * The Bulldrome's vocabulary (the code in em_bulldrome.c, EM_KIND_BULLDROME).
 *
 * Motions: row N of the family's motion table plays motion N, so one number
 * serves Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion)
 * kept in Em.motionId. The names say what the states that play a motion
 * show; motions no state plays stay numbers.
 *
 * Group 2 (EM_GROUP_FLY's slot) holds the travel between the areas' goals
 * (BULL_TRAVEL_*). Group 7 (EM_GROUP_EVENT2) runs the reactions' dispatcher.
 */
enum EmBulldromeMotion {
    BULL_MOT_IDLE = 0x01,          /* EmBulldrome_Init, BULL_IDLE, the opening scene's wait, the versus idles */
    BULL_MOT_WALK = 0x02,          /* BULL_MOVE_WALK, BULL_MOVE_WANDER_HOME */
    BULL_MOT_WALK_B = 0x03,        /* BULL_MOVE_WALK_B; the opening scene's walk-in to its goal */
    BULL_MOT_QUARTER_TURN_A = 0x04, /* the turn table's right-hand motion; BULL_MOVE_QUARTER_TURN: rotY - 309 a frame (about a quarter turn in all) */
    BULL_MOT_QUARTER_TURN_B = 0x05, /* the turn table's left-hand motion; BULL_MOVE_QUARTER_TURN: rotY + 309 a frame */
    BULL_MOT_FORAGE = 0x07,        /* BULL_IDLE_FORAGE to motionTime 240; the versus bait appears at its frame 10 */
    BULL_MOT_SHAKE_OFF = 0x0B,     /* BULL_IDLE_SHAKE_OFF after BULL_MOT_LIE_GET_UP */
    BULL_MOT_AIM = 0x0E,           /* BULL_IDLE_AIM_* turning to the goal, the opening scene before the charge, BULL_VS_POSE_B looped */
    BULL_MOT_CHARGE = 0x0F,        /* the charges with BulldromeAttack 3-5 and Em_StepToGoal; the opening scene's run-off */
    BULL_MOT_CHARGE_STOP = 0x10,   /* end of the charges */
    BULL_MOT_THREAT = 0x11,        /* BULL_IDLE_DISPLAY and _DISPLAY_B, the rage displays, BULL_VS_POSE_C */
    BULL_MOT_SPIN_A = 0x12,        /* BULL_ATK_SPIN, goal up to half a turn off rotY, rotY increasing */
    BULL_MOT_SPIN_B = 0x13,        /* BULL_ATK_SPIN, goal half a turn to a full turn off rotY, rotY decreasing */
    BULL_MOT_FLINCH = 0x14,        /* BULL_REACT_FLINCH when hit within three eighths to five eighths of a turn of rotY; BULL_REACT_HOLD2's start; BULL_VS_POSE_A */
    BULL_MOT_HALF_TURN = 0x15,     /* BULL_MOVE_HALF_TURN (618 a frame, about half a turn in all), the opening scene's turn */
    BULL_MOT_TRAVEL_START = 0x16,  /* BULL_TRAVEL_START */
    BULL_MOT_TRAVEL_END = 0x17,    /* BULL_TRAVEL_END */
    BULL_MOT_TRAVEL = 0x18,        /* BULL_TRAVEL_TURN and _MOVE (80 a frame) */
    BULL_MOT_FLINCH_SIDE_A = 0x3C, /* BULL_REACT_FLINCH, hitAngle - rotY under half a turn */
    BULL_MOT_FLINCH_SIDE_B = 0x3D, /* BULL_REACT_FLINCH, the other side */
    BULL_MOT_KNOCKED_SIDE_A = 0x3E, /* EmBulldrome_SetKnockedMotion, hitAngle - rotY under half a turn; launched at frame 10 */
    BULL_MOT_KNOCKED_LAND = 0x3F,  /* after Player_MoveFall lands: 150 frames (react) or once before BULL_MOT_DEAD */
    BULL_MOT_DEAD = 0x40,          /* BULL_DEFEAT_DIE and _DIE_IN_AIR, 2700 frames with the carve point */
    BULL_MOT_KNOCKED_GET_UP = 0x42, /* the end of BULL_REACT_KNOCKED_AWAY and _KNOCKED_AWAY_IN_AIR */
    BULL_MOT_KNOCKED_SIDE_B = 0x43, /* EmBulldrome_SetKnockedMotion, the other side */
    BULL_MOT_LIE = 0x46,           /* BULL_IDLE_STATUS2_LIE, _STATUS1_LIE, _REST, lying puffs at BULL_BONE_BREATH */
    BULL_MOT_DOWN = 0x47,          /* BULL_REACT_STATUS4, _STUN, _HOLD2, BULL_EVENT_CAPTURED (frozen at its end) */
    BULL_MOT_DOWN_GET_UP = 0x48,   /* BULL_IDLE_*_GET_UP after BULL_MOT_DOWN, rotY + half a turn at its start */
    BULL_MOT_LIE_GET_UP = 0x49     /* BULL_IDLE_STATUS2_END, _STATUS1_END, _REST_END */
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the effect helpers read the position from). The names say
 * what the code shows a bone to be: the Bulldrome's own hit shapes
 * (gEmBoneHits[EM_KIND_BULLDROME]), the shapes of the hits
 * BulldromeAttack_Create makes (BulldromeAttack_HitShapes) and where an
 * effect starts. Bone 1 stays a number: its small sphere (radius 32) is hit
 * only by ATTACK_HOLD (HITSHAPE2_HOLD_HIT | HITSHAPE2_HOLD_ONLY), and no
 * code says what part it is.
 *
 * Sounds: the Bulldrome's code plays none of its own (no Em_PlaySe* call in
 * the family), so there is no sound vocabulary here.
 */
enum EmBulldromeBone {
    BULL_BONE_ROOT = 0,     /* the skeleton's first bone: the stun stars circle above it (StunStars_CreateEm, BULL_REACT_STUN) */
    BULL_BONE_BODY = 2,     /* the body's capsule (part 0) and the sphere of every hit BulldromeAttack makes (its lists 0 to 2: the charges and the spin); the status-4 effect (Em_SpawnStatus4Effect) */
    BULL_BONE_FOOT_A = 6,   /* with BULL_BONE_FOOT_B, the feet that take turns in the gm_sub overlay's effects of the Bulldrome's motion 22 (GmEm_MotionEffects): its dust rises under FOOT_B at frames 24, 94 and 154 and under FOOT_A at 62, 118 and 178 (DustCloud places 46 and 45) */
    BULL_BONE_FOOT_B = 9,
    BULL_BONE_BREATH = 11   /* where the lying puffs start (Em_SpawnLyingPuffs in BULL_MOT_LIE's states), and MonsterFx type 13's place for this kind */
};

/* Group 0, EM_GROUP_IDLE (EmBulldrome_RunIdleState): standing, foraging, aiming at the goal, the display, resting, lying under a status and getting up. */
enum EmBulldromeIdleState {
    BULL_IDLE_NOP_A = 0x00,               /* empty function */
    BULL_IDLE = 0x01,                     /* BULL_MOT_IDLE once; EmBulldrome_Start, EmBulldrome_EndState and BULL_DEFEAT_REAPPEAR end in it */
    BULL_IDLE_NOP_B = 0x02,               /* empty function */
    BULL_IDLE_FORAGE = 0x03,              /* BULL_MOT_FORAGE to motionTime 240 */
    BULL_IDLE_AIM_SHORT = 0x04,           /* BULL_MOT_AIM, Em_TurnToGoal, count 1 */
    BULL_IDLE_NOP_C = 0x05,               /* empty function */
    BULL_IDLE_DISPLAY = 0x06,             /* BULL_MOT_THREAT once (argument 1, unused) */
    BULL_IDLE_AIM_MEDIUM = 0x07,          /* BULL_MOT_AIM, count 2 */
    BULL_IDLE_AIM_LONG = 0x08,            /* BULL_MOT_AIM, count 4 */
    BULL_IDLE_SHAKE_OFF = 0x09,           /* BULL_MOT_SHAKE_OFF; after every BULL_MOT_LIE_GET_UP */
    BULL_IDLE_DISPLAY_B = 0x0A,           /* BULL_MOT_THREAT once; nothing tells it from BULL_IDLE_DISPLAY */
    BULL_IDLE_STATUS2_LIE = 0x0B,         /* Em_BeginStatus2; EM_DMG_STATUS2 with Em_SetStatus2Timer */
    BULL_IDLE_STATUS2_END = 0x0C,         /* Em_EndStatus2; BULL_MOT_LIE_GET_UP; also EM_DMG_HIT_LYING */
    BULL_IDLE_HOLD2_GET_UP = 0x0D,        /* holdTimer2 = 0; from BULL_REACT_HOLD2 */
    BULL_IDLE_STATUS1_LIE = 0x0E,         /* Em_BeginStatus1; BULL_MOT_LIE (script only) */
    BULL_IDLE_STATUS1_END = 0x0F,         /* Em_EndStatus1; a hit while in BULL_IDLE_STATUS1_LIE */
    BULL_IDLE_STATUS4_GET_UP = 0x10,      /* Em_EndStatus4, noReact; from BULL_REACT_STATUS4 */
    BULL_IDLE_STUN_GET_UP = 0x11,         /* Em_EndStatus5, noReact; from BULL_REACT_STUN */
    BULL_IDLE_REST = 0x12,                /* Em_ResetNotice; 5400 frames lying, hp regen to 60% */
    BULL_IDLE_REST_END = 0x13,            /* Em_RefillNeed3; BULL_MOT_LIE_GET_UP */
    BULL_IDLE_DISPLAY_RAGE = 0x14,        /* Em_StartRage; BULL_MOT_THREAT */
    BULL_IDLE_DISPLAY_PENDING_RAGE = 0x15 /* Em_StartPendingRage; BULL_MOT_THREAT */
};

/* Group 1, EM_GROUP_MOVE (EmBulldrome_RunMoveState): walking and turning. */
enum EmBulldromeMoveState {
    BULL_MOVE_TURN_TO_TARGET = 0x00,       /* Em_StartTurnToTarget, Em_TurnStep */
    BULL_MOVE_WALK = 0x01,                 /* BULL_MOT_WALK, Em_StepToGoal, goal and speed from EmBulldrome_SetMoveState */
    BULL_MOVE_WALK_B = 0x02,               /* BULL_MOT_WALK_B, Em_StepToGoal; the setter's timer of 1800 is never read */
    BULL_MOVE_NOP_A = 0x03,                /* empty function */
    BULL_MOVE_NOP_B = 0x04,                /* empty function */
    BULL_MOVE_WANDER_HOME = 0x05,          /* BULL_MOT_WALK turning toward home, 120-247 frames */
    BULL_MOVE_QUARTER_TURN_FIXED = 0x06,   /* aimAngle rotY + ANGLE_TURN / 4, Em_TurnStep */
    BULL_MOVE_QUARTER_TURN = 0x07,         /* to the goal's side, 309 a frame */
    BULL_MOVE_HALF_TURN = 0x08,            /* BULL_MOT_HALF_TURN, 618 a frame */
    BULL_MOVE_QUARTER_TURN_NEW_GOAL = 0x09, /* EmBulldrome_SetMoveState recomputes the goal; as BULL_MOVE_QUARTER_TURN */
    BULL_MOVE_TURN_TO_NEW_GOAL = 0x0A      /* EmBulldrome_SetMoveState recomputes the goal; step from the clamped goal angle */
};

/* Group 2, EM_GROUP_FLY (EmBulldrome_RunTravelState): the travel to a goal. */
enum EmBulldromeTravelState {
    BULL_TRAVEL_START = 0x00, /* BULL_MOT_TRAVEL_START, unhittable, shadow hidden */
    BULL_TRAVEL_TURN = 0x01, /* a 64th of a turn a frame to the goal heading */
    BULL_TRAVEL_MOVE = 0x02, /* 80 a frame until distLeft is 0 */
    BULL_TRAVEL_END = 0x03   /* BULL_MOT_TRAVEL_END, then EmBulldrome_EndState */
};

/* Group 3, EM_GROUP_ATTACK (EmBulldrome_RunAttackState): the charges and the spin. */
enum EmBulldromeAttackState {
    BULL_ATK_CHARGE_A = 0x00,           /* BulldromeAttack 3, timer 90 */
    BULL_ATK_CHARGE_B = 0x01,           /* BulldromeAttack 4, timer 90 */
    BULL_ATK_CHARGE_SHORT = 0x02,       /* BulldromeAttack 5, timer 30 */
    BULL_ATK_CHARGE_SHORT_KEEP_A = 0x03, /* as BULL_ATK_CHARGE_SHORT; EmBulldrome_SetAttackState sets no goal or timer */
    BULL_ATK_CHARGE_SHORT_KEEP_B = 0x04, /* same; nothing tells it from _KEEP_A */
    BULL_ATK_CHARGE_SHORT_KEEP_C = 0x05, /* same */
    BULL_ATK_SPIN = 0x06                /* BulldromeAttack 1 (spin), BULL_MOT_SPIN_A/_B */
};

/* Group 4 (and 7), EM_GROUP_REACT (EmBulldrome_RunReactState): what EmBulldrome_Update picks from Em_UpdateDamage's result. */
enum EmBulldromeReactState {
    BULL_REACT_FLINCH = 0x00,             /* EM_DMG_PART_FLINCH with part 0 state bits */
    BULL_REACT_KNOCKED_AWAY = 0x01,       /* EM_DMG_PART_FLINCH otherwise, EM_DMG_HIT_MODE3; launch */
    BULL_REACT_KNOCKED_AWAY_IN_AIR = 0x02, /* EM_DMG_STATUS_IN_AIR; launch at once */
    BULL_REACT_STATUS4 = 0x03,            /* EM_DMG_STATUS4; Em_BeginStatus4, BULL_MOT_DOWN */
    BULL_REACT_STUN = 0x04,               /* EM_DMG_STATUS5; Em_BeginStatus5, stun stars */
    BULL_REACT_HOLD2 = 0x05               /* EM_DMG_ATTACK13; Em_SetHold2, Em_EndAllStatus */
};

/* Group 5, EM_GROUP_DEFEAT (EmBulldrome_RunDefeatState): death on the ground and in the air, and appearing again. */
enum EmBulldromeDefeatState {
    BULL_DEFEAT_DIE = 0x00,       /* EM_DMG_KILLED/DEAD; QuestEm_Hunted, launch, carve point */
    BULL_DEFEAT_DIE_IN_AIR = 0x01, /* as BULL_DEFEAT_DIE, launch at once (no setter in this file) */
    BULL_DEFEAT_REAPPEAR = 0x02   /* QuestEm_Appear in EmBulldrome_ChangeState; else task ends */
};

/* Group 6, EM_GROUP_EVENT (EmBulldrome_RunEventState): the capture and the opening scene. */
enum EmBulldromeEventState {
    BULL_EVENT_CAPTURED = 0x00, /* EM_DMG_CAPTURED; Em_SetCaptured, QuestEm_Captured */
    BULL_EVENT_INTRO = 0x01    /* EmBulldrome_Update: quest stages QUEST_ID_BULLDROME_SCENE_1/QUEST_ID_BULLDROME_SCENE_2, AREA_JUNGLE_5, EVENT_FLAG_BULLDROME_SCENE */
};

/* Group 8, EM_GROUP_VERSUS (EmBulldrome_RunVersusState): the versus quests' bait and poses. */
enum EmBulldromeVersusState {
    BULL_VS_EAT_BAIT = 0x00, /* VersusBait_Create at frame 10 of BULL_MOT_FORAGE */
    BULL_VS_POSE_A = 0x01,   /* BULL_MOT_FLINCH, then BULL_VS_WAIT_BOTH */
    BULL_VS_POSE_B = 0x02,   /* BULL_MOT_AIM x5, then BULL_VS_WAIT_BOTH */
    BULL_VS_POSE_C = 0x03,   /* BULL_MOT_THREAT, then BULL_VS_WAIT_BOTH */
    BULL_VS_WAIT_BOTH = 0x98, /* Versus_MarkDefeat on entry; waits Versus_BothDefeated */
    BULL_VS_END = 0x99       /* 90 frames then EmBulldrome_EndState */
};

/* The Bulldrome's own meanings of the work area's bytes (Em.work; EmWork has
 * the shared ones). */
typedef struct EmBulldromeWork {
    u8 unk000[0x00C - 0x000];
    u16 countdown;              /* 0x00C: EmBulldrome_Init: 0; EmBulldrome_Update takes 1 a frame while it is not 0 (nothing here sets it) */
    s16 unk00E;                 /* 0x00E: EmBulldrome_Init: 0 (no reader seen) */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: EmBulldrome_Init: the position; EmBulldrome_WanderHome turns toward it */
    u8 unk020[0x024 - 0x020];
    f32 distLeft;               /* 0x024: EmBulldrome_SetTravelState: the distance to the goal; EmBulldrome_TravelToGoal takes each step from it */
    u16 travelYaw;              /* 0x028: EmBulldrome_TravelTurn: the heading to the goal, which it turns to */
    u8 unk02A[0x02B - 0x02A];
    u8 steerToGoal;             /* 0x02B: EmBulldrome_SetTravelState (BULL_TRAVEL_MOVE): 1; EmBulldrome_TravelToGoal steers 64 a frame while set */
    u8 turnDir;                 /* 0x02C: EmBulldrome_QuarterTurn: the side (0 or 1); EmBulldrome_Spin: bit 0 the side, bit 1 a rear quadrant */
    u8 unk02D[0x02E - 0x02D];
    s16 turnStep;               /* 0x02E: EmBulldrome_HalfTurn: added to rotY each frame (EmBulldrome_QuarterTurn sets it too) */
    u8 unk030[0x100 - 0x030];
} EmBulldromeWork;
typedef char EmBulldromeWork_size_check[(sizeof(EmBulldromeWork) == sizeof(EmWork)) ? 1 : -1];

extern void *EmBulldrome_Callbacks[8];
void EmBulldrome_Init(Em *em);
void EmBulldrome_Start(Em *em);
void EmBulldrome_EndState(Em *em);
void EmBulldrome_LieDown(Em *em, s32 lieKind);
void EmBulldrome_TurnInPlace(Em *em, s32 variant);
void EmBulldrome_WanderHome(Em *em);
void EmBulldrome_QuarterTurn(Em *em, u8 variant);
void EmBulldrome_HalfTurn(Em *em, u8 variant);
void EmBulldrome_TravelTurn(Em *em);
void EmBulldrome_TravelToGoal(Em *em);
void EmBulldrome_Spin(Em *em);
void EmBulldrome_Die(Em *em);
void EmBulldrome_DieInAir(Em *em);
void EmBulldrome_Captured(Em *em);
void EmBulldrome_IntroScene(Em *em);
void EmBulldrome_VsEatBait(Em *em, EmWork *work);
void EmBulldrome_RunVersusState(Em *em);
void EmBulldrome_Update(Em *em);
void EmBulldrome_RunState(Em *em);
void EmBulldrome_SpawnShadow(u8 *em);
void EmBulldrome_TableNop(void);
void EmBulldrome_SetMoveState(Em *em, u16 state, u8 mode);
void EmBulldrome_SetTravelState(Em *em, u16 state, u8 mode);
void EmBulldrome_SetAttackState(Em *em, u16 state, u8 mode);
void EmBulldrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmBulldrome_LaunchFromHit(Em *em);
void EmBulldrome_SetKnockedMotion(Em *em, s16 frame);
extern EmTurnParams EmBulldrome_TurnParams[1];
extern f32 EmBulldrome_LiePuffsOfs[3];
extern EmMotionRow gEmMotionRows_68[101];
extern EmAreaValue gEmAreaTimer1Values_68[4];
extern EmAreaValue gEmAreaTimer2Values_68[16];

#endif
