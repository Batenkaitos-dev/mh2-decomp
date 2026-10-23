#ifndef EM_YAMATSUKAMI_H
#define EM_YAMATSUKAMI_H
#include "em.h"

/*
 * The Yama Tsukami family (EM_KIND_YAMA_TSUKAMI and EM_KIND_YAMA_TSUKAMI_B):
 * its motions, its states by group (em.h's EmStateGroup), its curves and its
 * body sizes. The functions are EmYamaTsukami_*. A giant
 * that floats over the fortress: it drifts, rises and sinks, flails and
 * slams its tentacles, spins, inhales, calls up kind 56 monsters, and
 * deflates in two steps under the hunters' hits until it falls to the
 * ground. The second kind stays at a fixed place, never sees the hunters
 * and leaves no carcass (EmYamaTsukami_Start).
 */

/* Motions: row N of the kind's motion table plays motion N on every layer,
 * so these serve Em_SetMotion's row and EM_MOTION_ID(layer, motion) alike.
 * Named from the states that play them. */
enum EmYamaTsukamiMotion {
    YAMA_MOT_IDLE = 0x01,
    YAMA_MOT_RISE = 0x02,               /* YAMA_CURVE_RISE */
    YAMA_MOT_DESCEND = 0x03,            /* YAMA_CURVE_DESCEND */
    YAMA_MOT_RISE_TO_GOAL = 0x04,       /* YAMA_CURVE_RISE_TO_GOAL, until the goal's height */
    YAMA_MOT_SURGE_FORWARD = 0x05,      /* YAMA_CURVE_SURGE_FORWARD, a speed curve */
    YAMA_MOT_SURGE_BACK = 0x06,         /* YAMA_CURVE_SURGE_BACK; two impact attacks (MotionEffects) */
    YAMA_MOT_RECOVER = 0x07,            /* swells back to its full size */
    YAMA_MOT_FALL = 0x08,
    YAMA_MOT_FALL_LAND = 0x09,          /* the landing's hit and dust (MotionEffects) */
    YAMA_MOT_SOFT_LAND = 0x0A,          /* a short fall caught before the floor (YAMA_REACT_FALL_RECOVER) */
    YAMA_MOT_RISE_TO_GOAL_END = 0x0B,
    YAMA_MOT_RECOVER_RISE = 0x0C,
    YAMA_MOT_RECOVER_PAUSE = 0x0E,      /* the ground stops shaking under it (EmYamaTsukami_IsQuakeFreeMotion) */
    YAMA_MOT_DOWNED = 0x14,
    YAMA_MOT_GET_UP = 0x15,
    YAMA_MOT_MOVE = 0x16,               /* walks, turns and drifts */
    YAMA_MOT_MOVE_CRUISE = 0x17,        /* the dash's fast drift (YAMA_FLY_DASH_TO_GOAL) */
    YAMA_MOT_MOVE_STOP = 0x18,
    YAMA_MOT_TENTACLE_SLAM_A = 0x1E,    /* each slam makes two attacks (MotionEffects) and dust where the tentacles reach the floor */
    YAMA_MOT_TENTACLE_SLAM_B = 0x1F,
    YAMA_MOT_TENTACLE_HIT_A = 0x20,     /* one attack */
    YAMA_MOT_TENTACLE_HIT_B = 0x21,
    YAMA_MOT_TENTACLE_SLAM_C = 0x22,
    YAMA_MOT_TENTACLE_SLAM_D = 0x23,
    YAMA_MOT_SMASH = 0x24,              /* held 450 frames; its hit at frame 156 */
    YAMA_MOT_SMASH_END = 0x25,
    YAMA_MOT_HOLD_POSE = 0x26,
    YAMA_MOT_HOLD_POSE_END = 0x27,
    YAMA_MOT_INHALE = 0x28,             /* the pull starts at frame 100 (YAMA_ATK_INHALE) */
    YAMA_MOT_INHALE_END = 0x29,
    YAMA_MOT_STRIKE_A = 0x2A,           /* a hit at frame 50 */
    YAMA_MOT_STRIKE_B = 0x2B,           /* a hit at frame 42 */
    YAMA_MOT_STRIKE_B_HOLD = 0x2C,
    YAMA_MOT_STRIKE_RECOVER = 0x2D,
    YAMA_MOT_SPIN_START = 0x2E,
    YAMA_MOT_SPIN = 0x2F,
    YAMA_MOT_SPIN_END = 0x30,
    YAMA_MOT_SUMMON = 0x31,
    YAMA_MOT_SUMMON_END = 0x32,
    YAMA_MOT_CRASH_FALL = 0x33,
    YAMA_MOT_DIVE_SLAM = 0x34,          /* a hit at frame 70, the impact under YAMA_BONE_BODY at frame 156 */
    YAMA_MOT_CRASH_LIE = 0x35,
    YAMA_MOT_CRASH_GET_UP = 0x36,
    YAMA_MOT_FLINCH_A = 0x3C,
    YAMA_MOT_FLINCH_B = 0x3D,
    YAMA_MOT_DOWNED_FLINCH = 0x3E,
    YAMA_MOT_DIE = 0x3F,
    YAMA_MOT_FLINCH_D = 0x40,           /* also opens YAMA_EVENT_WALK_IN */
    YAMA_MOT_EVENT_TURN = 0x41,         /* YAMA_EVENT_WALK_IN turns during its first 64 frames */
    YAMA_MOT_FLINCH_HOVER = 0x42,       /* also YAMA_EVENT_WALK_IN's walk; the ground stops shaking (EmYamaTsukami_IsQuakeFreeMotion) */
    YAMA_MOT_FLINCH_C = 0x43
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound and effect helpers read the position from). The
 * names say what the family's code shows a bone to be: the bones of the
 * hit shapes YamaTsukamiAttack_Create makes on given frames of a motion
 * (YamaTsukamiAttack_HitShapes, by the attack rows' hit lists), the bones
 * of the tentacles' floor test (EmYamaTsukami_TentacleBones), the three
 * bones the body's swelling scales (EmYamaTsukami_ScaleBones) and where
 * the summoned monsters come out (EmKind38_SpawnAtEmBone).
 * The four tentacles, A to D, are the rows of EmYamaTsukami_TentacleBones;
 * each has three spheres that narrow from the base to the tip (radius 370,
 * 220 and 160). The two small tentacles, A and B, have three each (210, 180
 * and 155). The strikes' hits take the two small tentacles (and the body); a
 * tentacle slam's one tentacle, and in YAMA_MOT_TENTACLE_SLAM_A and _B with a
 * small one beside it (small A with C, small B with D); the smash with C,
 * D and both small ones. Which side each is, the code doesn't show. Bones
 * no code explains stay numbers: 3 (the floating sounds and most others of
 * the body), 16, 23 and 29 (sounds in the tentacles' motions), and 36 (the
 * sounds that open most motions).
 */
enum EmYamaTsukamiBone {
    YAMA_BONE_BODY = 2,                 /* the body's sphere in most attacks (radius 600 to 2000); the carcass mine point 400 above it; the dive slam's impact 300 under it */
    YAMA_BONE_SWELL_B = 4,              /* grows with bodyScale[1] as the body swells (EmYamaTsukami_ScaleBones) */
    YAMA_BONE_SWELL_A = 6,              /* grows with bodyScale[0] */
    YAMA_BONE_TENTACLE_A_BASE = 7,      /* tentacle A's widest sphere */
    YAMA_BONE_TENTACLE_A_MID = 8,
    YAMA_BONE_TENTACLE_A_DUST = 9,      /* with _TIP, the floor test's bones: the dust rises where either reaches the floor */
    YAMA_BONE_TENTACLE_A_TIP = 10,      /* its narrowest sphere */
    YAMA_BONE_TENTACLE_B_BASE = 11,
    YAMA_BONE_TENTACLE_B_MID = 13,
    YAMA_BONE_TENTACLE_B_DUST = 14,
    YAMA_BONE_TENTACLE_B_TIP = 15,
    YAMA_BONE_TENTACLE_C_BASE = 22,
    YAMA_BONE_TENTACLE_C_MID = 24,
    YAMA_BONE_TENTACLE_C_DUST = 25,
    YAMA_BONE_TENTACLE_C_TIP = 26,
    YAMA_BONE_TENTACLE_D_BASE = 28,
    YAMA_BONE_TENTACLE_D_MID = 30,
    YAMA_BONE_TENTACLE_D_DUST = 31,
    YAMA_BONE_TENTACLE_D_TIP = 32,
    YAMA_BONE_SUMMON = 34,              /* where a summoned kind 56 monster comes out, facing along the bone */
    YAMA_BONE_SWELL_C = 35,             /* grows with bodyScale[2] */
    YAMA_BONE_SMALL_TENTACLE_A_BASE = 39,
    YAMA_BONE_SMALL_TENTACLE_A_MID = 41,
    YAMA_BONE_SMALL_TENTACLE_A_TIP = 42,
    YAMA_BONE_SMALL_TENTACLE_B_BASE = 43,
    YAMA_BONE_SMALL_TENTACLE_B_MID = 46,
    YAMA_BONE_SMALL_TENTACLE_B_TIP = 47
};

/*
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind; Em_PlaySeAtBone and its frame variants). A sound
 * is named where the code shows what it is: it plays on or next to the
 * frame of the thing it goes with, or with it in the same function. The
 * others stay numbers, showing only where they play: 101 to 103 at bone 3
 * through the floating motions, the pairs at the bases of tentacles A and
 * B four frames apart in the idle cycles, and the sounds that open most
 * motions at bone 36.
 */
enum EmYamaTsukamiSe {
    YAMA_SE_THUD = 6,                   /* the body striking the floor: on the frame of the landing's dust and body hit in YAMA_MOT_FALL_LAND, two and four frames before the dust in YAMA_MOT_GET_UP and _DIE */
    YAMA_SE_SMASH_HIT = 23,             /* only in YAMA_MOT_SMASH: at the tips of the four tentacles its hit (frame 156) takes, two and six frames after it */
    YAMA_SE_FORT_IMPACT_B = 68,         /* the fortress impact's second sound (EmYamaTsukami_SpawnFortImpact); in YAMA_MOT_SMASH four frames after the impact's frame */
    YAMA_SE_FORT_IMPACT_A = 69          /* the fortress impact's first sound, at its point */
};

/* Group 0, EM_GROUP_IDLE (EmYamaTsukami_RunIdleState): on the ground.
 * EmYamaTsukami_EndState returns to YAMA_IDLE. */
enum EmYamaTsukamiIdleState {
    YAMA_IDLE_HOLD = 0x00,              /* keeps the idle motion set (unless debug flag 0x4D is on) */
    YAMA_IDLE = 0x01,                   /* one idle cycle, then the script */
    YAMA_IDLE_DOWNED = 0x02,            /* after a fall: lies 5 cycles, then YAMA_IDLE_GET_UP */
    YAMA_IDLE_GET_UP = 0x03,            /* unhurt, then YAMA_IDLE_RECOVER */
    YAMA_IDLE_RECOVER = 0x04            /* swells back to YAMA_SIZE_FULL (EmYamaTsukami_SetIdleState) */
};

/* Group 1, EM_GROUP_MOVE (EmYamaTsukami_RunMoveState): on the ground. */
enum EmYamaTsukamiMoveState {
    YAMA_MOVE_FORWARD = 0x00,           /* to the goal, turning toward it */
    YAMA_MOVE_BACKWARD = 0x01,          /* backs away from the goal; EmYamaTsukami_SetMoveState turns it into YAMA_MOVE_FORWARD when the goal is ahead */
    YAMA_MOVE_TURN = 0x02               /* turns to the goal, speeding up then slowing down (EmYamaTsukami_Turn) */
};

/* Group 2, EM_GROUP_FLY (EmYamaTsukami_RunFlyState): floating, its usual
 * stance. EmYamaTsukami_EndFlyState returns to YAMA_FLY_IDLE. The _GUARDED
 * variants set EmYamaTsukamiWork.flyGuard, which keeps the hits from
 * killing or flinching it until the next state change. */
enum EmYamaTsukamiFlyState {
    YAMA_FLY_IDLE = 0x00,
    YAMA_FLY_RECOVER_PAUSE = 0x01,      /* after a short fall: pauseCycles motion cycles, then YAMA_FLY_RECOVER_RISE */
    YAMA_FLY_TURN = 0x02,               /* EmYamaTsukamiWork's turn; EmYamaTsukami_SetFlyState drops it to YAMA_FLY_IDLE with no target */
    YAMA_FLY_TURN_GUARDED = 0x03,
    YAMA_FLY_DRIFT_TO_GOAL = 0x04,      /* straight to the goal at 10 a frame, give or take 1 */
    YAMA_FLY_DRIFT_TO_GOAL_GUARDED = 0x05,
    YAMA_FLY_DASH_TO_GOAL = 0x06,       /* guarded; 30 frames, then at 20 a frame with YAMA_MOT_MOVE_CRUISE */
    YAMA_FLY_RISE_TO_GOAL_HEIGHT = 0x07,
    YAMA_FLY_RISE_TO_GOAL_HEIGHT_GUARDED = 0x08,
    YAMA_FLY_RISE = 0x09,
    YAMA_FLY_DESCEND = 0x0A,
    YAMA_FLY_SURGE_FORWARD = 0x0B,      /* until within 500 of the goal */
    YAMA_FLY_SURGE_BACK = 0x0C,
    YAMA_FLY_RISE_GUARDED = 0x0D,
    YAMA_FLY_DESCEND_GUARDED = 0x0E,
    YAMA_FLY_SURGE_FORWARD_GUARDED = 0x0F,
    YAMA_FLY_SURGE_BACK_GUARDED = 0x10,
    YAMA_FLY_RECOVER = 0x11,            /* swells back to YAMA_SIZE_FULL in the air */
    YAMA_FLY_RECOVER_RISE = 0x12,       /* the rise back after a short fall (YAMA_SIZE_FULL) */
    YAMA_FLY_CRUISE_TO_GOAL = 0x13,     /* as YAMA_FLY_DRIFT_TO_GOAL at 15 a frame */
    YAMA_FLY_WAIT_FOR_HUNTERS = 0x14,   /* idles until a hunter in the quest enters its area */
    YAMA_FLY_RISE_B = 0x15,             /* runs YAMA_FLY_RISE */
    YAMA_FLY_SINK_TO_FLOOR = 0x16
};

/* Group 3, EM_GROUP_ATTACK (EmYamaTsukami_RunAttackState). */
enum EmYamaTsukamiAttackState {
    YAMA_ATK_SUMMON = 0x00,             /* calls up kind 56 monsters, five at most alive */
    YAMA_ATK_SPIN_TO_HEIGHT = 0x01,     /* spins while it rises or sinks to the goal's height */
    YAMA_ATK_HOLD_POSE = 0x02,          /* 200 frames */
    YAMA_ATK_TENTACLE_SLAM_A = 0x03,
    YAMA_ATK_TENTACLE_SLAM_B = 0x04,
    YAMA_ATK_TENTACLE_SLAM_C = 0x05,
    YAMA_ATK_TENTACLE_SLAM_D = 0x06,
    YAMA_ATK_SMASH = 0x07,
    YAMA_ATK_DIVE_SLAM = 0x08,
    YAMA_ATK_CRASH = 0x09,              /* drops to the floor, lies 450 frames and gets up on the ground */
    YAMA_ATK_TENTACLE_HIT_A = 0x0A,
    YAMA_ATK_TENTACLE_HIT_B = 0x0B,
    YAMA_ATK_INHALE = 0x0C,             /* YAMA_MOT_STRIKE_A, then the inhale for 240 frames */
    YAMA_ATK_STRIKE_A = 0x0D,
    YAMA_ATK_STRIKE_B = 0x0E,           /* the strike held 200 frames */
    YAMA_ATK_SPIN = 0x0F,               /* spins in place */
    YAMA_ATK_SMASH_FORT = 0x10          /* the smash that hits the fortress: counts fortHits and sets gScene.fortStage */
};

/* Group 4, EM_GROUP_REACT (EmYamaTsukami_RunReactState): the part flinches
 * (EmYamaTsukami_Update). Each size takes three flinches, A, B and C; the C
 * flinch shrinks the body a step (EmYamaTsukami_SetReactState), and a hit at
 * YAMA_SIZE_SHRUNK makes it fall. The ground variants change no stage. */
enum EmYamaTsukamiReactState {
    YAMA_REACT_FALL_LAND = 0x00,        /* falls to the floor and lies down (YAMA_IDLE_DOWNED); leaves a timed mine point */
    YAMA_REACT_FALL_RECOVER = 0x01,     /* from more than 2000 above the floor: falls 1000 at most and catches itself */
    YAMA_REACT_FLINCH_A = 0x02,
    YAMA_REACT_FLINCH_B = 0x03,
    YAMA_REACT_FLINCH_C_FROM_FULL = 0x04,   /* shrinks to YAMA_SIZE_HALF */
    YAMA_REACT_FLINCH_C_FROM_HALF = 0x05,   /* shrinks to YAMA_SIZE_SHRUNK */
    YAMA_REACT_FLINCH_D = 0x06,
    YAMA_REACT_FLINCH_D_B = 0x07,       /* the same as YAMA_REACT_FLINCH_D */
    YAMA_REACT_DOWNED_FLINCH = 0x08,    /* a hit while YAMA_IDLE_DOWNED */
    YAMA_REACT_GROUND_FLINCH_A = 0x09,
    YAMA_REACT_GROUND_FLINCH_B = 0x0A,
    YAMA_REACT_GROUND_FLINCH_C_FROM_FULL = 0x0B,
    YAMA_REACT_GROUND_FLINCH_C_FROM_HALF = 0x0C,
    YAMA_REACT_FLINCH_HOVER = 0x0D      /* a hit while YAMA_FLY_RECOVER_PAUSE */
};

/* Group 5, EM_GROUP_DEFEAT (EmYamaTsukami_RunDefeatState). */
enum EmYamaTsukamiDefeatState {
    YAMA_DEFEAT_DIE = 0x00,             /* on the ground */
    YAMA_DEFEAT_DIE_FALLING = 0x01      /* in the air: falls first */
};

/* Group 6, EM_GROUP_EVENT (EmYamaTsukami_RunEventState): scripted scenes. */
enum EmYamaTsukamiEventState {
    YAMA_EVENT_TOWER_FLYBY = 0x00,      /* the tower's opening scene: three passes, then waits for event flag 0xAF */
    YAMA_EVENT_WALK_IN = 0x01           /* walks along two fixed goals, then floats (YAMA_FLY_IDLE) */
};

/* EmYamaTsukamiWork.curveKind (EmYamaTsukami_StartCurves): the curve a state
 * runs. The speed curves set velZ, the others velY. */
enum EmYamaTsukamiCurve {
    YAMA_CURVE_RISE = 0,
    YAMA_CURVE_DESCEND = 1,
    YAMA_CURVE_RISE_TO_GOAL = 2,
    YAMA_CURVE_RISE_TO_GOAL_END = 3,
    YAMA_CURVE_SURGE_FORWARD = 4,       /* a speed curve */
    YAMA_CURVE_SURGE_BACK = 5,          /* a speed curve */
    YAMA_CURVE_SPIN_START = 6,
    YAMA_CURVE_SPIN_END = 7,
    YAMA_CURVE_DIVE_SLAM = 8,
    YAMA_CURVE_9 = 9,                   /* has a table; no state starts it */
    YAMA_CURVE_DIE_FALLING = 10,
    YAMA_CURVE_SOFT_LAND = 11,
    YAMA_CURVE_RECOVER_RISE = 12
};

/* EmYamaTsukamiWork.bodySize: the row of EmYamaTsukami_SizeScales the body
 * eases to (EmYamaTsukami_EaseToSize, EmYamaTsukami_ScaleBones). */
enum EmYamaTsukamiSize {
    YAMA_SIZE_SHRUNK = 0,               /* scale 0.1 */
    YAMA_SIZE_HALF = 1,                 /* scale 0.5 */
    YAMA_SIZE_FULL = 2                  /* scale 1.0 */
};

/* EmYamaTsukamiWork.flinchStage: the next part flinch (EmYamaTsukami_Update). */
enum EmYamaTsukamiFlinchStage {
    YAMA_FLINCH_STAGE_A = 0,
    YAMA_FLINCH_STAGE_B = 1,
    YAMA_FLINCH_STAGE_C = 2             /* flinch C, or the fall at YAMA_SIZE_SHRUNK */
};

/* A key of EmYamaTsukami_TurnByKeys's tables: the yaw `v`, in degrees, at
 * frame `t`. A table ends with a key whose t is 0 (or negative: restart). */
typedef struct EmYamaTsukamiYawKey {
    s32 t;
    s32 v;
} EmYamaTsukamiYawKey;

extern void *EmYamaTsukami_Callbacks[8];
void EmYamaTsukami_StartCurves(Em *em, u8 kind);
u8 EmYamaTsukami_UpdateCurves(Em *em);
void EmYamaTsukami_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmYamaTsukami_MotionSounds(Em *em, EmWork *work);
void EmYamaTsukami_GetMinePointPos(Em *em, f32 *out);
void EmYamaTsukami_AddMinePoint(Em *em);
void EmYamaTsukami_RemoveMinePoints(Em *em);
void EmYamaTsukami_Start(Em *em);
void EmYamaTsukami_RunIdleState(Em *em);
void EmYamaTsukami_WalkForward(Em *em);
void EmYamaTsukami_WalkBackward(Em *em);
void EmYamaTsukami_Turn(Em *em, s32 variant);
void EmYamaTsukami_RecoverPause(Em *em);
void EmYamaTsukami_FlyToGoal(Em *em, s32 variant);
void EmYamaTsukami_RiseToGoalHeight(Em *em, s32 variant);
void EmYamaTsukami_Rise(Em *em, s32 variant);
void EmYamaTsukami_Descend(Em *em, s32 variant);
void EmYamaTsukami_SurgeForward(Em *em, s32 variant);
void EmYamaTsukami_SurgeBack(Em *em, s32 variant);
void EmYamaTsukami_WaitForHunters(Em *em);
void EmYamaTsukami_SinkToFloor(Em *em);
void EmYamaTsukami_Summon(Em *em);
void EmYamaTsukami_Spin(Em *em, s32 variant);
void EmYamaTsukami_Smash(Em *em, s32 variant);
void EmYamaTsukami_Die(Em *em);
void EmYamaTsukami_DieFalling(Em *em);
void EmYamaTsukami_WalkIn(Em *em);
void EmYamaTsukami_Update(Em *em);
void EmYamaTsukami_RunState(Em *em);
void EmYamaTsukami_MotionEffects(Em *em, EmWork *work);
void EmYamaTsukami_EaseToSize(Em *em, EmWork *work);
void EmYamaTsukami_ScaleBones(Em *em, EmBone *bones);
void EmYamaTsukami_SpawnFortImpact(Em *em);
void EmYamaTsukami_InitTentacleDust(Em *em);
void EmYamaTsukami_UpdateTentacleDust(Em *em);
s32 EmYamaTsukami_IsQuakeFreeMotion(Em *em);

#endif
