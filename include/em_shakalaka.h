#ifndef EM_SHAKALAKA_H
#define EM_SHAKALAKA_H
#include "em.h"

/*
 * The Shakalaka family (EM_KIND_SHAKALAKA): its motions and its states, by
 * group (em.h's EmStateGroup). The functions are EmShakalaka_*. A small masked creature that walks and runs in a pack, hides
 * in the ground and springs out at a player who comes near, strikes, leaps,
 * and throws bombs (the bomb families of the attack objects ask
 * EmShakalaka_BombState whether the bomb is still in its hand).
 */

/* Motions: row N of the kind's motion table plays motion N, so these serve
 * Em_SetMotion's row and EM_MOTION_ID(layer, motion) alike. Named from the
 * states that play them; the idle and move actions nothing tells apart have
 * letters. */
enum EmShakalakaMotion {
    SHAK_MOT_IDLE = 0x01,
    SHAK_MOT_IDLE_ACTION_A = 0x02,
    SHAK_MOT_IDLE_ACTION_B = 0x03,
    SHAK_MOT_WALK_B = 0x04,             /* the second walk (SHAK_MOVE_WALK_B, SHAK_MOVE_WANDER_HOME_B) */
    SHAK_MOT_IDLE_ACTION_C = 0x06,
    SHAK_MOT_MOVE_ACTION_A = 0x07,      /* SHAK_MOVE_ACTION picks this or _B at random */
    SHAK_MOT_MOVE_ACTION_B = 0x08,
    SHAK_MOT_IDLE_ACTION_D = 0x09,
    SHAK_MOT_IDLE_ACTION_E = 0x0A,      /* also the crouch that opens the jump slam */
    SHAK_MOT_IDLE_ACTION_F = 0x0B,
    SHAK_MOT_WALK = 0x0C,
    SHAK_MOT_RUN = 0x0D,                /* carries a contact attack (MotionEffects) */
    SHAK_MOT_AMBUSH = 0x0E,             /* springs out of the ground, with a contact attack */
    SHAK_MOT_HIDDEN = 0x0F,             /* hidden in the ground, waiting */
    SHAK_MOT_STRIKE = 0x15,
    SHAK_MOT_JUMP_WINDUP = 0x16,
    SHAK_MOT_LEAP = 0x17,               /* leaves the ground at frame 8 */
    SHAK_MOT_LEAP_LAND = 0x18,          /* also the stop that ends SHAK_MOVE_DASH_PAST_TARGET */
    SHAK_MOT_BOMB_WINDUP = 0x19,
    SHAK_MOT_BOMB_THROW = 0x1A,         /* the bomb appears at frame 45 and leaves the hand at frame 94 */
    SHAK_MOT_JUMP_UP = 0x1B,
    SHAK_MOT_JUMP_FALL = 0x1C,
    SHAK_MOT_JUMP_LAND = 0x1D,          /* the landing's shockwave (MotionEffects) */
    SHAK_MOT_FLINCH = 0x3C,
    SHAK_MOT_KNOCK_DOWN = 0x3D,
    SHAK_MOT_LIE = 0x3E,
    SHAK_MOT_GET_UP = 0x3F,
    SHAK_MOT_RECOVER = 0x40,            /* settles back to the idle after a fall, a shake-off or a dash */
    SHAK_MOT_STATUS4 = 0x41,            /* held while status 4 lasts */
    SHAK_MOT_SHAKE_OFF = 0x42,
    SHAK_MOT_COLLAPSE = 0x43,           /* the status 2 and stun collapse, followed by SHAK_MOT_LIE */
    SHAK_MOT_DIE = 0x44,
    SHAK_MOT_FALL = 0x45,               /* falling out of the air */
    SHAK_MOT_FALL_LAND = 0x46
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix Em_BonePos reads). The names say what the family's code
 * shows a bone to be: where the ground effects (EmShakalaka_SpawnGroundEffect
 * and _Offset, by EmShakalaka_MotionEffects) and the lying puffs go. Which
 * part of the body a bone is the code doesn't show. Every attack's sphere
 * (ShakalakaAttack's one hit list: radius 30 on bone 9) is in the attack's
 * data only.
 *
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind; Em_PlaySeOnFrame plays it at the monster).
 * None is named: none plays only on the frame of the event it goes with
 * (sound 5 plays as SHAK_MOT_STRIKE's attack starts at frame 24, but also
 * in the run, the leap and the jump on frames where no attack starts; sound
 * 16 on SHAK_MOT_FALL_LAND's dust at frame 2, and 4 to 10 frames from the
 * knock-down's and the collapse's).
 */
enum EmShakalakaBone {
    SHAK_BONE_DUST = 1,             /* the ground effects of the move actions, the ambush, the knock-down and the death (55 ahead of it), and SHAK_MOT_JUMP_LAND's impact (55 ahead of it at frame 4) */
    SHAK_BONE_FALL_DUST = 3,        /* SHAK_MOT_FALL_LAND's ground effects (frames 2 and 48) */
    SHAK_BONE_BREATH = 10,          /* where the lying puffs start (SHAK_MOT_LIE in SHAK_IDLE_STATUS2); the ground effects of SHAK_MOT_LEAP_LAND, the shake-off and the ends of the collapse and the death */
    SHAK_BONE_PROP = 11             /* variant 3's prop (EMBODYFX_SHAKALAKA_PROP) is drawn on this bone, and its flame (FireBlast type 12) burns at a small offset from it, following it every frame; the hiding variant's cover (EMBODYFX_SHAKALAKA_COVER) starts here */
};

/* Group 0, EM_GROUP_IDLE (EmShakalaka_RunIdleState). Most are the script's
 * choices; EmShakalaka_EndState returns to SHAK_IDLE. */
enum EmShakalakaIdleState {
    SHAK_IDLE_WAIT = 0x00,              /* the idle motion, then a turn of the script every frame after 5 to 20 frames; group 2 runs it too */
    SHAK_IDLE = 0x01,                   /* one idle cycle, then the script */
    SHAK_IDLE_ACTION_A = 0x02,
    SHAK_IDLE_ACTION_B = 0x03,
    SHAK_IDLE_ACTION_C = 0x04,          /* held for 90 frames */
    SHAK_IDLE_ACTION_D = 0x05,
    SHAK_IDLE_ACTION_E = 0x06,
    SHAK_IDLE_ACTION_F = 0x07,
    SHAK_IDLE_AMBUSH = 0x08,            /* hidden in the ground until a player comes near (EmShakalaka_Start, EmShakalaka_Reappear) */
    SHAK_IDLE_STATUS2 = 0x09,           /* status 2: collapses and lies until its timer ends */
    SHAK_IDLE_STATUS2_END = 0x0A,       /* ends status 2 and gets up */
    SHAK_IDLE_STUN_END = 0x0B           /* entering it ends the stun (EmShakalaka_ChangeState); gets up */
};

/* Group 1, EM_GROUP_MOVE (EmShakalaka_RunMoveState). Entering one computes
 * the goal (Em_ComputeGoal2); the script picks them. */
enum EmShakalakaMoveState {
    SHAK_MOVE_WALK = 0x00,              /* walks to the goal, stops within 200 */
    SHAK_MOVE_WALK_B = 0x01,            /* the same with SHAK_MOT_WALK_B */
    SHAK_MOVE_RUN = 0x02,               /* runs to the goal, stops within 200 */
    SHAK_MOVE_ACTION_A = 0x03,          /* a move action four times, turning to the goal */
    SHAK_MOVE_ACTION_B = 0x04,          /* the same code */
    SHAK_MOVE_TURN_IN_PLACE = 0x05,
    SHAK_MOVE_WALK_STRAIGHT = 0x06,     /* walks on the heading the goal had at the start */
    SHAK_MOVE_RUN_STRAIGHT = 0x07,
    SHAK_MOVE_WANDER_HOME = 0x08,       /* wanders, turning slowly toward its spawn point */
    SHAK_MOVE_WANDER_HOME_B = 0x09,     /* the same with SHAK_MOT_WALK_B */
    SHAK_MOVE_RUN_FAST = 0x0A,          /* runs half again as fast, stops within 200 */
    SHAK_MOVE_RUN_STOP_AT_300 = 0x0B,
    SHAK_MOVE_RUN_STOP_AT_600 = 0x0C,
    SHAK_MOVE_FLANK_BACK = 0x0D,        /* runs to a point 600 along the target's -Z axis */
    SHAK_MOVE_FLANK_BACK_FAST = 0x0E,
    SHAK_MOVE_FLANK_SIDE_A = 0x0F,      /* 600 along the target's +X axis */
    SHAK_MOVE_FLANK_SIDE_A_FAST = 0x10,
    SHAK_MOVE_FLANK_SIDE_B = 0x11,      /* 600 along the target's -X axis */
    SHAK_MOVE_FLANK_SIDE_B_FAST = 0x12,
    SHAK_MOVE_DASH_PAST_TARGET = 0x13,  /* runs at the target and 400 beyond it */
    SHAK_MOVE_WALK_STOP_AT_700 = 0x14
};

/* Group 3, EM_GROUP_ATTACK (EmShakalaka_RunAttackState). The script picks
 * them, apart from SHAK_ATK_AMBUSH. */
enum EmShakalakaAttackState {
    SHAK_ATK_STRIKE = 0x00,
    SHAK_ATK_BOMB_LOB = 0x01,           /* the bomb on a high arc (EmBomb type 4) */
    SHAK_ATK_JUMP_SLAM = 0x02,          /* jumps up and lands with a shockwave */
    SHAK_ATK_LEAP_SHORT = 0x03,         /* the leaps, by their row of the leap table */
    SHAK_ATK_LEAP_MEDIUM = 0x04,
    SHAK_ATK_LEAP_LONG = 0x05,
    SHAK_ATK_AMBUSH = 0x06,             /* springs out of the ground (from SHAK_IDLE_AMBUSH) */
    SHAK_ATK_BOMB_FLAT = 0x07           /* the bomb thrown flat and fast, bursting into a cloud (EmBomb type 5) */
};

/* Group 4, EM_GROUP_REACT (EmShakalaka_RunReactState): what
 * EmShakalaka_Update picks from Em_UpdateDamage's result. */
enum EmShakalakaReactState {
    SHAK_REACT_FLINCH = 0x00,           /* a hit on the ground: turns to the hit */
    SHAK_REACT_KNOCK_DOWN = 0x01,       /* a part flinch, a hit while lying or in the air */
    SHAK_REACT_FLINCH_NO_TURN = 0x02,   /* the script's only */
    SHAK_REACT_STATUS4 = 0x03,
    SHAK_REACT_FALL = 0x04,             /* a status begun in the air: falls and lands */
    SHAK_REACT_STUN = 0x05              /* entering it begins the stun (status 5) */
};

/* Group 5, EM_GROUP_DEFEAT (EmShakalaka_RunDefeatState). */
enum EmShakalakaDefeatState {
    SHAK_DEFEAT_DIE = 0x00,
    SHAK_DEFEAT_DIE_IN_AIR = 0x01,
    SHAK_DEFEAT_REAPPEAR = 0x02         /* after the fade: the quest may bring it back */
};

/* Group 6, EM_GROUP_EVENT (EmShakalaka_RunEventState). */
enum EmShakalakaEventState {
    SHAK_EVENT_NONE = 0x00              /* does nothing */
};

/* Shakalaka's own meanings of the work area's bytes (em.h's EmWork has the shared ones). */
typedef struct EmShakalakaWork {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x00C - 0x001];
    u16 unk00C;                 /* 0x00C: Init: 0; Update counts it down; nothing sets it above 0 */
    u16 bombHeld;               /* 0x00E: EmShakalaka_ThrowBomb: 1 while the bomb is in the hand (frames 45 to 94), then 0; EmShakalaka_BombState reads it */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Init: the spawn position (posX, 0, posZ); State1_08 turns to it */
    u8 unk020[0x050 - 0x020];
    s32 accelAngX;              /* 0x050: angles X, Y, Z for Em_MoveRotatedAccel (State4_04, State5_01_Hunted: 0, rotY, 0) */
    f32 accelAngY;              /* 0x054: rotY converted to a float (the game's own store; Em_MoveRotatedAccel reads the word as an angle) */
    s32 accelAngZ;              /* 0x058 */
    u8 unk05C[0x05D - 0x05C];
    u8 unk05D;                  /* 0x05D: Init: 0 */
    u8 unk05E[0x100 - 0x05E];
} EmShakalakaWork;
typedef char EmShakalakaWork_size_check[(sizeof(EmShakalakaWork) == sizeof(EmWork)) ? 1 : -1];

extern void *EmShakalaka_Callbacks[8];
void EmShakalaka_Init(Em *em);
void EmShakalaka_Start(Em *em);
s32 EmShakalaka_BombState(Em *em);
void EmShakalaka_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmShakalaka_IdleAction(Em *em, s32 variant);
void EmShakalaka_Ambush(Em *em);
void EmShakalaka_Status2Collapse(Em *em);
void EmShakalaka_MoveAction(Em *em, s32 variant);
void EmShakalaka_MoveStraight(Em *em, s32 variant);
void EmShakalaka_WanderHome(Em *em, s32 variant);
void EmShakalaka_RunToFlank(Em *em, s8 variant);
void EmShakalaka_Leap(Em *em, s32 variant);
void EmShakalaka_KnockDown(Em *em);
void EmShakalaka_Die(Em *em);
void EmShakalaka_DieInAir(Em *em);
void EmShakalaka_Reappear(Em *em);
void EmShakalaka_Update(Em *em);
void EmShakalaka_RunState(Em *em);
void EmShakalaka_MotionEffects(Em *em, EmWork *work);

#endif /* EM_SHAKALAKA_H */
