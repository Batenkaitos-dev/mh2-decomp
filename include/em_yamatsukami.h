#ifndef EM_YAMATSUKAMI_H
#define EM_YAMATSUKAMI_H

/*
 * The Yama Tsukami family (EM_KIND_YAMA_TSUKAMI and EM_KIND_YAMA_TSUKAMI_B):
 * its motions, its states by group (em.h's EmStateGroup), its curves and its
 * body sizes. The functions are EmYamaTsukami_*. A giant
 * that floats over the fortress: it drifts, rises and sinks, flails and
 * slams its tentacles, spins, inhales, calls up kind 0x38 monsters, and
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
    YAMA_MOT_DIVE_SLAM = 0x34,          /* a hit at frame 70, the impact under bone 2 at frame 156 */
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
    YAMA_ATK_SUMMON = 0x00,             /* calls up kind 0x38 monsters, five at most alive */
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

#endif
