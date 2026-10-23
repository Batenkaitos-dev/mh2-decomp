#ifndef EM_FATALIS_H
#define EM_FATALIS_H
#include "em.h"

/*
 * The Fatalis family's vocabulary (EmFatalis_*: its states, motions, bones
 * and sounds). Kinds EM_KIND_FATALIS, EM_KIND_FATALIS_B and
 * EM_KIND_FATALIS_C share one set of states; the first has a motion table of
 * its own, the other two share one.
 *
 * Motions: both motion tables have row N play motion N, so one number serves
 * Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion) kept in
 * Em.motionId. The names say what the states that play a motion show;
 * motions no state explains stay numbers (18, 19, 30, 41).
 */
enum EmFatalisMotion {
    FAT_MOT_IDLE = 0x01,                /* the standing loop; layers 0-2 idle with EM_MOTION_ID(layer, 1) */
    FAT_MOT_WALK = 0x02,                /* Em_StepToGoal walking */
    FAT_MOT_TURN_LEFT = 0x03,           /* turning to the goal, rotY falling 372 a frame up to frame 89 */
    FAT_MOT_TURN_RIGHT = 0x04,          /* turning to the goal, rotY rising 372 a frame up to frame 89 */
    FAT_MOT_CHARGE_START = 0x05,        /* before a charge */
    FAT_MOT_TURN_SMALL = 0x06,          /* turning to a goal less than an eighth of a turn away, 91 a frame */
    FAT_MOT_CHARGE = 0x07,              /* running at the goal (Em_StepToGoal), with FatalisAttack 2 */
    FAT_MOT_RECOVER = 0x08,             /* back to standing after a landing, an attack, a status or a reaction */
    FAT_MOT_TAKE_OFF = 0x09,            /* airborne (stance 2) from frame 134, along curve 0 or 6 */
    FAT_MOT_FLY = 0x0A,                 /* flying and hovering; layers 0-2 hover with EM_MOTION_ID(layer, FAT_MOT_FLY) */
    FAT_MOT_DESCEND = 0x0B,             /* the last stretch of a landing, down to the floor */
    FAT_MOT_TOUCH_DOWN = 0x0C,          /* on the floor again (stance 0), with a camera shake */
    FAT_MOT_FIREBALL = 0x0D,            /* a fireball on the ground at frame 102 (EmFatalis_SpawnFireball) */
    FAT_MOT_FIREBALL_B = 0x0E,          /* a fireball on the ground at frame 98 */
    FAT_MOT_SWEEP = 0x0F,               /* five effects at frames 80 to 320 (EmFatalis_SpawnSweepEffect) */
    FAT_MOT_AIR_FIREBALL = 0x10,        /* a fireball from the hover at frame 30 */
    FAT_MOT_FLINCH = 0x14,
    FAT_MOT_FLINCH_B = 0x15,
    FAT_MOT_FLINCH_TURN_RIGHT = 0x16,   /* rotY rising 0x28E a frame up to frame 50 */
    FAT_MOT_FLINCH_TURN_LEFT = 0x17,    /* rotY falling 0x28E a frame up to frame 50 */
    FAT_MOT_DIE = 0x18,                 /* falling dead (QuestEm_Hunted) */
    FAT_MOT_FALL = 0x19,                /* falling out of the air */
    FAT_MOT_FALL_LAND = 0x1A,           /* hitting the floor after FAT_MOT_FALL */
    FAT_MOT_DOWN = 0x1B,                /* lying knocked down (120 frames), or under status 4 */
    FAT_MOT_DOWN_GET_UP = 0x1C,         /* getting up from FAT_MOT_DOWN or FAT_MOT_DIE_B's pose */
    FAT_MOT_DIE_B = 0x1D,               /* falling dead after an attack of kind 9 (Em.lastAttackKind) */
    FAT_MOT_AIR_FIREBALL_AIM = 0x1F,    /* hovering 90 frames, turning to the goal, before FAT_MOT_AIR_FIREBALL */
    FAT_MOT_ROAR = 0x20,                /* the scenes start the rage with it (Em.rage 1, rageTimer 1800) */
    FAT_MOT_HOP_FIREBALL = 0x21,        /* airborne from frame 70, a fireball at frame 94 */
    FAT_MOT_LIE_DOWN = 0x22,            /* collapsing to lie under status 2 or 4 (lying from frame 296) */
    FAT_MOT_GLIDE_START = 0x23,         /* on the floor, before FAT_MOT_GLIDE */
    FAT_MOT_LAND_SLIDE = 0x24,          /* back on the floor after a glide or a dive */
    FAT_MOT_DIVE_RISE = 0x25,           /* rising and pitching up 0x1A8 a frame before a dive */
    FAT_MOT_DIVE = 0x26,                /* diving at the floor, velZ 80 and accelZ 9 */
    FAT_MOT_DIVE_PULL_UP = 0x27,        /* pulling out of the dive, pitch falling 0x6A2 a frame */
    FAT_MOT_GLIDE = 0x28                /* flying low to the goal along curve 4 (speed only) */
};

/* Group 0, EM_GROUP_IDLE (EmFatalis_RunIdleState). */
enum EmFatalisIdleState {
    FAT_IDLE_STAND = 0x00,              /* the idle loop on every layer until the timer ends */
    FAT_IDLE = 0x01,                    /* EmFatalis_SetIdleState picks FAT_IDLE or FAT_IDLE_ACTION at random for either */
    FAT_IDLE_ACTION = 0x02,
    FAT_IDLE_ROAR = 0x03,
    FAT_IDLE_STATUS2 = 0x04,            /* lying under status 2 (EM_DMG_STATUS2) until EM_DMG_HIT_LYING */
    FAT_IDLE_STATUS2_END = 0x05,
    FAT_IDLE_STATUS4_END = 0x06         /* after FAT_REACT_STATUS4 */
};

/* Group 1, EM_GROUP_MOVE (EmFatalis_RunMoveState). */
enum EmFatalisMoveState {
    FAT_MOVE_WALK = 0x00,
    FAT_MOVE_TURN_TO_TARGET = 0x01,
    FAT_MOVE_TURN_TO_GOAL = 0x02
};

/* Group 2, EM_GROUP_FLY (EmFatalis_RunFlyState). */
enum EmFatalisFlyState {
    FAT_FLY_TAKE_OFF = 0x00,
    FAT_FLY_LAND = 0x01,
    FAT_FLY_HOVER = 0x02,
    FAT_FLY_HOVER_TURN = 0x03,
    FAT_FLY_TO_GOAL = 0x04,
    FAT_FLY_RISE = 0x05,
    FAT_FLY_STRAIGHT_TO_GOAL = 0x06,
    FAT_FLY_DESCEND = 0x07,
    FAT_FLY_RISE_HIGH = 0x08,
    FAT_FLY_LEVEL_TO_GOAL = 0x09,
    FAT_FLY_CLIMB_TO_GOAL = 0x0A,
    FAT_FLY_TAKE_OFF_AND_RISE = 0x0B,
    FAT_FLY_CLIMB_TO_GOAL_NEAR = 0x0C,  /* FAT_FLY_CLIMB_TO_GOAL, ending 10 frames' flight from the goal instead of 100 */
    FAT_FLY_HOVER_TURN_STOP = 0x0D,     /* FAT_FLY_HOVER_TURN, stopping first */
    FAT_FLY_LAND_AT_HEIGHT = 0x0E,      /* lands at the height 3584, then FAT_ATK_BLASTS */
    FAT_FLY_TAKE_OFF_HIGH = 0x0F        /* FAT_FLY_TAKE_OFF counted airborne from the start (after FAT_ATK_BLASTS) */
};

/* Group 3, EM_GROUP_ATTACK (EmFatalis_RunAttackState). */
enum EmFatalisAttackState {
    FAT_ATK_SINGLE_A = 0x00,            /* motion 18 */
    FAT_ATK_SINGLE_B = 0x01,            /* motion 19 */
    FAT_ATK_FIREBALL = 0x02,
    FAT_ATK_FIREBALL_B = 0x03,
    FAT_ATK_SWEEP_NO_EFFECT = 0x04,     /* the sweep's motion with no effects */
    FAT_ATK_AIR_FIREBALL = 0x05,        /* the fireball's pitch 14564 */
    FAT_ATK_CHARGE = 0x06,
    FAT_ATK_HOP_FIREBALL = 0x07,
    FAT_ATK_CHARGE_START = 0x08,        /* the charge's first motion only */
    FAT_ATK_AIR_FIREBALL_TRIPLE = 0x09, /* three fireballs, pitched 7828, 10923 and 14564 */
    FAT_ATK_AIR_FIREBALL_B = 0x0A,      /* pitch 10923 */
    FAT_ATK_AIR_FIREBALL_C = 0x0B,      /* pitch 7828 */
    FAT_ATK_GLIDE = 0x0C,
    FAT_ATK_DIVE = 0x0D,                /* pulls out 850 above the floor */
    FAT_ATK_SWEEP_A = 0x0E,             /* the sweeps A-D take rows 1-4 of EmFatalis_SweepSpots */
    FAT_ATK_SWEEP_B = 0x0F,
    FAT_ATK_SWEEP_C = 0x10,
    FAT_ATK_SWEEP_D = 0x11,
    FAT_ATK_SWEEP_DOUBLE = 0x12,        /* two sweeps along row 5 */
    FAT_ATK_SINGLE_C = 0x13,            /* FAT_ATK_SINGLE_A's state with motion 41 */
    FAT_ATK_DIVE_B = 0x14,              /* pulls out at the height 1050 */
    FAT_ATK_BLASTS = 0x15               /* after FAT_FLY_LAND_AT_HEIGHT: 24 effects along row 6 or 7 */
};

/* Group 4, EM_GROUP_REACT (EmFatalis_RunReactState): what EmFatalis_Update picks from Em_UpdateDamage's result. */
enum EmFatalisReactState {
    FAT_REACT_FLINCH = 0x00,            /* a flinch of parts 0, 1, 4 and 5 (EM_DMG_PART_FLINCH), or EM_DMG_HIT_MODE3 */
    FAT_REACT_FLINCH_B = 0x01,          /* a flinch of part 6 */
    FAT_REACT_FLINCH_TURN_RIGHT = 0x02, /* a flinch of the other parts but 3 */
    FAT_REACT_FALL = 0x03,              /* EM_DMG_STATUS_IN_AIR, or a flinch in the air */
    FAT_REACT_KNOCK_DOWN = 0x04,        /* EM_DMG_ATTACK9 and EM_DMG_PART8_DOWN */
    FAT_REACT_STATUS4 = 0x05,           /* EM_DMG_STATUS4 */
    FAT_REACT_FLINCH_TURN_LEFT = 0x06,  /* a flinch of part 3 */
    FAT_REACT_FLINCH_C = 0x07,
    FAT_REACT_FALL_B = 0x08
};

/* Group 5, EM_GROUP_DEFEAT (EmFatalis_RunDefeatState): EM_DMG_KILLED. */
enum EmFatalisDefeatState {
    FAT_DEFEAT_DIE = 0x00,
    FAT_DEFEAT_DIE_B = 0x01,            /* the last hit an attack of kind 9 (Em.lastAttackKind) */
    FAT_DEFEAT_DIE_FALLING = 0x02       /* in the air */
};

/*
 * Group 6 (and 7), EM_GROUP_EVENT (EmFatalis_RunEventState): EmFatalis_Update
 * starts them in three areas (by Area_DayId), each kind in its own, while an
 * event runs and until the kind's event flag is set.
 */
enum EmFatalisEventState {
    FAT_EVENT_LEAVE = 0x00,             /* EM_KIND_FATALIS in AREA_CASTLE_1 once the quest's goal is met: flies up and away */
    FAT_EVENT_FLY_IN = 0x01,            /* EM_KIND_FATALIS's scene in AREA_CASTLE_1 (EVENT_FLAG_FATALIS_CASTLE_SCENE) */
    FAT_EVENT_LEAVE_FROM_AIR = 0x02,    /* FAT_EVENT_LEAVE from the hover */
    FAT_EVENT_GET_UP = 0x03,            /* EM_KIND_FATALIS_B's scene in AREA_BATTLEFIELD_1 (EVENT_FLAG_FATALIS_BATTLEFIELD_SCENE) */
    FAT_EVENT_DESCEND = 0x04            /* EM_KIND_FATALIS_C's scene in AREA_TOWER_0x7C (EVENT_FLAG_FATALIS_TOWER_SCENE) */
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound and effect helpers read the position from). The
 * three kinds share one skeleton's numbers: they share the attack table and
 * the sound and effect code. The names say what the family's code shows a
 * bone to be: the bones of the attack hit shapes FatalisAttack_Create makes
 * on given frames of a motion (FatalisAttack_HitShapes, by the attack rows'
 * hit lists), where the fireballs and the sweeps start, the bones the wing
 * beats take turns at, and the four legs, whose step sounds and ground
 * effects take turns in FAT_MOT_CHARGE (leg C, B, D, then A). Each leg has
 * two bones: the step sounds play at the upper one, and the ground effects
 * (PartFx_SpawnForEmScaled) rise at the lower one, the foot. "A" to "D" tell
 * the legs apart; which is left, right, front or hind the code doesn't show.
 * Bones no code explains stay numbers:
 * - 2: a large sphere with FAT_BONE_BREATH's in the charge (FatalisAttack 2)
 *   and the sweep (FatalisAttack 3);
 * - 11 to 15: a chain of five spheres (FatalisAttack 19 to 27) in the flight
 *   motions and on frame 60 of the turns, when sound 26 plays at bone 17;
 * - 17: sounds only (as at 11);
 * - 20 and 55: the glide's spheres (FatalisAttack 30); 22 and 27: spheres
 *   with FAT_BONE_LEG_C's and FAT_BONE_LEG_D's in motions 18 and 41;
 * - 31: a large ground effect as FAT_MOT_HOP_FIREBALL leaves the floor;
 * - the carcass's mine points (EmFatalis_MinePtBones, data).
 */
enum EmFatalisBone {
    FAT_BONE_ROOT = 0,          /* the skeleton's first bone: the sweeps' fire starts here (EMRANGED_FATALIS_SWEEP) */
    FAT_BONE_BODY = 1,          /* the body's sphere in FAT_MOT_CHARGE_START and FAT_MOT_TOUCH_DOWN (FatalisAttack 12 to 14), and the roar's wide sphere in FAT_MOT_ROAR (FatalisAttack 16) */
    FAT_BONE_LEG_A = 5,         /* the walk's and the turns' step sounds take turns here and at FAT_BONE_LEG_B, as do their step spheres (FatalisAttack 5 to 11) */
    FAT_BONE_FOOT_A = 6,        /* leg A's foot: its ground effects in the turns and the charge, its sphere in FAT_MOT_TURN_SMALL (FatalisAttack 33) */
    FAT_BONE_LEG_B = 9,
    FAT_BONE_FOOT_B = 10,       /* the same for leg B (FatalisAttack 34) */
    FAT_BONE_LEG_C = 24,        /* leg C's step sounds in the charge; its sphere in motions 18 and 41 (FatalisAttack 28, 32) */
    FAT_BONE_FOOT_C = 25,       /* leg C's foot: its ground effects in the charge, the flinches, lying down and getting up */
    FAT_BONE_LEG_D = 29,        /* the same for leg D (FatalisAttack 4, 29, 31) */
    FAT_BONE_FOOT_D = 30,
    FAT_BONE_WING_A = 34,       /* the wing beats play here and at FAT_BONE_WING_B, four frames apart, above all in the flight motions */
    FAT_BONE_WING_B = 40,
    FAT_BONE_BREATH = 51,       /* where the fireballs start (EMRANGED_FATALIS_FIREBALL and the two bursts) and the lying puffs; a sphere in motion 19 (FatalisAttack 1) and the ground and air fireballs' motions (FatalisAttack 17, 18) */
    FAT_BONE_HEAD = 52          /* the voice sounds play here; the head's sphere in EM_KIND_FATALIS_C's charge start (FatalisAttack 35) */
};

/*
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind; Em_PlaySeAtBoneOnFrame and the other helpers). A
 * sound is named where the code shows what it is: it plays on the frame of
 * the thing it goes with. The others stay numbers: the fireballs' sound 27
 * plays on the fireball's frame in FAT_MOT_HOP_FIREBALL only (4 to 10 frames
 * before it in the others), and sound 85, played only in FAT_MOT_ROAR, plays
 * at frame 4, long before the roar's rings (frame 66).
 */
enum EmFatalisSe {
    FAT_SE_STEP = 1             /* a step: only at FAT_BONE_LEG_A and _B; on the frame of the camera shake in FAT_MOT_TURN_RIGHT (94), FAT_MOT_FLINCH_B (26) and FAT_MOT_DIE (130), two frames after each of FAT_MOT_WALK's (14, 84) */
};

extern void *EmFatalis_Callbacks[8];
void EmFatalis_Init(Em *em);
void EmFatalis_UpdateRageTimer(Em *em);
void EmFatalis_AddMinePoints(Em *em);
void EmFatalis_UpdateMinePoints(Em *em);
void EmFatalis_RemoveMinePoints(Em *em);
void EmFatalis_EndAirState(Em *em, u8 next);
void EmFatalis_Update(Em *em);
void EmFatalis_Status2Lie(Em *em, EmWork *work);
void EmFatalis_TurnToGoal(Em *em, EmWork *work);
void EmFatalis_Hover(Em *em, EmWork *work);
void EmFatalis_Sweep(Em *em, EmWork *work, s32 variant);
void EmFatalis_Dive(Em *em, EmWork *work);
void EmFatalis_SweepDouble(Em *em, EmWork *work, s32 variant);
void EmFatalis_DiveB(Em *em, EmWork *work);
void EmFatalis_Blasts(Em *em);
void EmFatalis_Fall(Em *em, EmWork *work);
void EmFatalis_KnockDown(Em *em, EmWork *work);
void EmFatalis_FallB(Em *em, EmWork *work);
void EmFatalis_Die(Em *em, EmWork *work);
void EmFatalis_DieB(Em *em, EmWork *work);
void EmFatalis_DieFalling(Em *em, EmWork *work);
void EmFatalis_EventLeave(Em *em, EmWork *work);
void EmFatalis_EventFlyIn(Em *em, EmWork *work);
void EmFatalis_EventGetUp(Em *em, EmWork *work);
void EmFatalis_EventDescend(Em *em, EmWork *work);
void EmFatalis_RunState(Em *em, EmWork *work);
void EmFatalis_MotionEffects(Em *em, EmFatalisWork *work);
void EmFatalis_ShakeOnFrame(void *em, s32 frame, s32 strength);

void EmFatalis_SetIdleState(Em *em, u16 state, u8 mode);
void EmFatalis_SetMoveState(Em *em, u16 state, u8 mode);
void EmFatalis_SetFlyState(Em *em, u16 state, u8 mode);
void EmFatalis_SetAttackState(Em *em, u16 state, u8 mode);
void EmFatalis_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmFatalis_MoveByVelYCurve(Em *em, s32 kind);
void EmFatalis_StartCurves(Em *em, u8 kind);

/*
 * The block that holds EmFatalis_SweepSpots' row 7 and the table's rows 0 to
 * 4 (EmFatalis_SweepSpots_7): the linker's EmFatalis_SweepSpots names the
 * second field (config/ld/game_aliases.ld); rows 5 to 8 are
 * EmFatalis_SweepSpotsTail, the next object.
 */
typedef struct EmFatalisSweepBlock {
    s8 row7[28];        /* 0x00: row 7's spots (EmFatalis_SweepSpotsTail's third entry) */
    s8 *rows[5];        /* 0x1C: EmFatalis_SweepSpots' rows 0 to 4 (row 0 is NULL) */
} EmFatalisSweepBlock;

#endif /* EM_FATALIS_H */
