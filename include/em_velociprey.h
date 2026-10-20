#ifndef EM_VELOCIPREY_H
#define EM_VELOCIPREY_H

/*
 * The Velociprey family's vocabulary (EmVelociprey_*):
 * Velociprey, Genprey, Ioprey and Giaprey (EM_KIND_VELOCIPREY,
 * EM_KIND_GENPREY, EM_KIND_IOPREY, EM_KIND_GIAPREY) share one set of state
 * functions and one numbering of motions and states.
 *
 * Motions: the four kinds and the dromes (EmDrome_*) share one
 * motion table (gEmMotionRows[kind]), whose row N plays motion N, so one
 * number serves Em_SetMotion's row and EM_MOTION_ID(layer, motion). The
 * names say what the states that play a motion show. Sides A and B are the
 * two side hits (by the hit angle's quarter); which one is the left is not
 * settled.
 */
enum EmVelocipreyMotion {
    VPREY_MOT_IDLE = 0x01,              /* the standing loop; Init's row */
    VPREY_MOT_WALK = 0x02,              /* Em_StepToGoal walking */
    VPREY_MOT_HOP = 0x03,               /* a hop forward: airborne from frame 10 */
    VPREY_MOT_HOP_UP = 0x04,            /* a hop on the spot: airborne from frame 22 */
    VPREY_MOT_HOP_LAND = 0x05,          /* the landing of every hop */
    VPREY_MOT_TURN_LEFT = 0x06,         /* the turn rows' motLeft (EmVelociprey_TurnParams) */
    VPREY_MOT_TURN_RIGHT = 0x07,        /* their motRight */
    VPREY_MOT_MOVE_ACTION_A = 0x08,     /* dust at frame 12 */
    VPREY_MOT_MOVE_ACTION_B = 0x09,     /* dust at frame 14 */
    VPREY_MOT_MOVE_ACTION_C = 0x0A,     /* dust at frame 14 */
    VPREY_MOT_ALERT = 0x0B,             /* the combat idle (VPREY_IDLE_ALERT), also after status 4 and the opening scene */
    VPREY_MOT_IDLE_B = 0x0C,            /* VPREY_IDLE_PACK_WAIT and VPREY_IDLE_SPAWN */
    VPREY_MOT_LEAP = 0x14,              /* the leap attacks: airborne from frame 22 */
    VPREY_MOT_LEAP_LAND = 0x15,
    VPREY_MOT_STRIKE = 0x16,            /* a melee hit at frame 40 */
    VPREY_MOT_POUNCE = 0x17,            /* played by the dromes only (EmDrome_*): a leap attack on curve kind 1 */
    VPREY_MOT_POUNCE_LAND = 0x18,       /* its landing */
    VPREY_MOT_QUICK_STRIKE = 0x19,      /* a melee hit at frame 16 */
    VPREY_MOT_SHOOT = 0x1A,             /* an EmRangedAttack shot at frame 80 */
    VPREY_MOT_FLINCH = 0x1E,
    VPREY_MOT_KNOCKED_BACK = 0x1F,      /* thrown backward by a hit from the front */
    VPREY_MOT_KNOCKED_BACK_LAND = 0x20,
    VPREY_MOT_KNOCKED_FORWARD = 0x21,   /* thrown forward by a hit from behind */
    VPREY_MOT_KNOCKED_FORWARD_LAND = 0x22,
    VPREY_MOT_KNOCKED_SIDE_B = 0x23,
    VPREY_MOT_KNOCKED_SIDE_B_LAND = 0x24,
    VPREY_MOT_KNOCKED_SIDE_A = 0x25,
    VPREY_MOT_KNOCKED_SIDE_A_LAND = 0x26,
    VPREY_MOT_GET_UP = 0x27,            /* after a knock-back and after status 2 */
    VPREY_MOT_DEAD = 0x29,              /* the carcass lying on the ground */
    VPREY_MOT_DAZED = 0x2A,             /* blinded, dazed, stunned */
    VPREY_MOT_COLLAPSE = 0x2B,          /* the fall under status 2 */
    VPREY_MOT_LIE = 0x2C,               /* lying under status 2 */
    VPREY_MOT_DIE_IN_AIR = 0x2D,        /* killed in the air: flung away, fading */
    VPREY_MOT_DIE = 0x2E,               /* the death throes after a knock-back's landing */
    VPREY_MOT_STATUS4 = 0x2F,
    VPREY_MOT_IDLE_ACTION_A = 0x32,
    VPREY_MOT_CALL = 0x33,              /* the pack call: the sense object at frame 40 */
    VPREY_MOT_SHAKE_OFF = 0x34,         /* also the end of the stun */
    VPREY_MOT_EVENT_IDLE = 0x35         /* held in the opening scene (VPREY_EVENT_LOOK_AROUND) */
};

/* Group 0, EM_GROUP_IDLE (EmVelociprey_RunIdleState). */
enum EmVelocipreyIdleState {
    VPREY_IDLE_STAND = 0x00,            /* idle for 30 to 120 frames, then the script */
    VPREY_IDLE = 0x01,                  /* EmVelociprey_EndState's state: the script picks the next */
    VPREY_IDLE_ACTION_A = 0x02,
    VPREY_IDLE_PACK_CALL = 0x03,        /* calls the pack: a sense object the other raptors notice */
    VPREY_IDLE_SHAKE_OFF = 0x04,
    VPREY_IDLE_ALERT = 0x05,            /* the combat idle: EmVelociprey_SetIdleState puts it for VPREY_IDLE and VPREY_IDLE_WAIT */
    VPREY_IDLE_GET_UP = 0x06,           /* after the four knock-backs */
    VPREY_IDLE_STATUS2 = 0x07,          /* EM_DMG_STATUS2: collapses and lies until the status timer ends */
    VPREY_IDLE_STATUS2_END = 0x08,
    VPREY_IDLE_BLINDED = 0x09,          /* dazed until Em.blindTimer runs out */
    VPREY_IDLE_PACK_WAIT = 0x0A,        /* a pack member waiting for its leader (Em.spawnParam 1) */
    VPREY_IDLE_STATUS4_END = 0x0B,      /* from VPREY_REACT_STATUS4 when its timer ends */
    VPREY_IDLE_SPAWN = 0x0C,            /* after EmVelociprey_Start, EmVelociprey_Respawn and a reappearance */
    VPREY_IDLE_WAIT = 0x0D,             /* runs as VPREY_IDLE */
    VPREY_IDLE_STUN_END = 0x10          /* from VPREY_REACT_STUN: ends status 5 */
};

/* Group 1, EM_GROUP_MOVE (EmVelociprey_RunMoveState). */
enum EmVelocipreyMoveState {
    VPREY_MOVE_TURN = 0x00,             /* turns to the target; VPREY_MOVE_TURN_ALERT in combat */
    VPREY_MOVE_WALK = 0x01,
    VPREY_MOVE_ACTION_A = 0x02,
    VPREY_MOVE_ACTION_B = 0x03,
    VPREY_MOVE_ACTION_C = 0x04,
    VPREY_MOVE_TURN_ALERT = 0x05        /* the turn with EmVelociprey_TurnParams[1] */
};

/* Group 2, EM_GROUP_FLY (EmVelociprey_RunHopState): the hops. */
enum EmVelocipreyHopState {
    VPREY_HOP_FORWARD = 0x00,           /* on curve kind 2: up and forward */
    VPREY_HOP_UP = 0x01,                /* on curve kind 3: up only */
    VPREY_HOP_ARC = 0x02                /* on a set velocity and gravity */
};

/* Group 3, EM_GROUP_ATTACK (EmVelociprey_RunAttackState): picked by the script only. */
enum EmVelocipreyAttackState {
    VPREY_ATK_STRIKE_A = 0x00,
    VPREY_ATK_NONE = 0x01,              /* ends at once */
    VPREY_ATK_LEAP_MEDIUM_A = 0x02,
    VPREY_ATK_QUICK_STRIKE_A = 0x03,
    VPREY_ATK_SHOT_A = 0x04,
    VPREY_ATK_LEAP_LONG_A = 0x05,
    VPREY_ATK_LEAP_HIGH_A = 0x06,
    VPREY_ATK_STRIKE_B = 0x07,
    VPREY_ATK_LEAP_MEDIUM_B = 0x08,
    VPREY_ATK_QUICK_STRIKE_B = 0x09,
    VPREY_ATK_LEAP_LONG_B = 0x0A,
    VPREY_ATK_LEAP_HIGH_B = 0x0B,
    VPREY_ATK_STRIKE_C = 0x0C,
    VPREY_ATK_SHOT_A_FAST = 0x0D,       /* VPREY_ATK_SHOT_A with the double-speed shot */
    VPREY_ATK_SHOT_B = 0x0E,
    VPREY_ATK_SHOT_B_FAST = 0x0F
};

/* Group 4, EM_GROUP_REACT (EmVelociprey_RunReactState): what EmVelociprey_Update picks. */
enum EmVelocipreyReactState {
    VPREY_REACT_FLINCH = 0x00,
    VPREY_REACT_KNOCKED_BACK = 0x01,    /* a hit from the front, or a status in the air */
    VPREY_REACT_KNOCKED_FORWARD = 0x02, /* a hit from behind */
    VPREY_REACT_KNOCKED_SIDE_A = 0x03,
    VPREY_REACT_KNOCKED_SIDE_B = 0x04,
    VPREY_REACT_DAZE = 0x05,
    VPREY_REACT_STATUS4 = 0x06,
    VPREY_REACT_STUN = 0x08             /* status 5 (EM_STATUS_STUN) */
};

/* Group 5, EM_GROUP_DEFEAT (EmVelociprey_RunDefeatState). */
enum EmVelocipreyDefeatState {
    VPREY_DEFEAT_DIE_BACK = 0x00,       /* killed by a hit from the front: a carcass to carve */
    VPREY_DEFEAT_DIE_FORWARD = 0x01,
    VPREY_DEFEAT_DIE_SIDE_A = 0x02,
    VPREY_DEFEAT_DIE_SIDE_B = 0x03,
    VPREY_DEFEAT_REAPPEAR = 0x04,       /* QuestEm_Appear: back in the area at one of its appear points */
    VPREY_DEFEAT_FADE_RESPAWN = 0x05,
    VPREY_DEFEAT_DIE_IN_AIR_BACK = 0x06, /* killed in the air: no carcass */
    VPREY_DEFEAT_DIE_IN_AIR_FORWARD = 0x07,
    VPREY_DEFEAT_DIE_IN_AIR_SIDE_A = 0x08,
    VPREY_DEFEAT_DIE_IN_AIR_SIDE_B = 0x09
};

/* Group 6, EM_GROUP_EVENT (EmVelociprey_RunEventState). */
enum EmVelocipreyEventState {
    VPREY_EVENT_LOOK_AROUND = 0x00,
    VPREY_EVENT_WALK_TO_GOAL_A = 0x01,
    VPREY_EVENT_WALK_TO_GOAL_B = 0x02,
    VPREY_EVENT_BY_VARIANT = 0x03,      /* one of the three above, by Em.variant */
    VPREY_EVENT_HIDE_A = 0x04,
    VPREY_EVENT_HIDE_B = 0x05,
    VPREY_EVENT_HIDE_C = 0x06
};

/* Group 7, EM_GROUP_EVENT2 (EmVelociprey_RunEvent2State): the hops into the area after VPREY_DEFEAT_REAPPEAR. */
enum EmVelocipreyEvent2State {
    VPREY_EVENT2_HOP_IN = 0x00,
    VPREY_EVENT2_HOP_IN_LOW = 0x01,
    VPREY_EVENT2_HOP_IN_HIGH = 0x02
};

/*
 * EmVelociprey_StartCurves' kinds: rows of the curve tables
 * (EmVelociprey_VelYCurves, EmVelociprey_SpeedCurves). Kinds 0 and 2 run
 * both curves, 1 and 3 the vertical one only; the family starts 2 and 3.
 */
enum EmVelocipreyCurveKind {
    VPREY_CURVE_HOP_FORWARD = 2,        /* VPREY_HOP_FORWARD: up and forward */
    VPREY_CURVE_HOP_UP = 3              /* VPREY_HOP_UP: up only */
};

/*
 * The family's own meanings of the monster's work area (Em.work; EmWork,
 * include/em.h, has the shared ones).
 */
typedef struct EmVelocipreyWork {
    u8 fxStep;                  /* 0x000: EmVelociprey_UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x008 - 0x001];
    s32 accelAngX;              /* 0x008: angles X, Y, Z for Em_MoveRotatedAccel (UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x00C */
    s32 accelAngZ;              /* 0x010 */
    u8 curveNone;               /* 0x014: the three curves' run flags (StartCurves, UpdateCurves) */
    u8 curveVelY;               /* 0x015 */
    u8 curveSpeed;              /* 0x016 */
    u8 curveKind;               /* 0x017: the curve tables' index (StartCurves) */
    s16 curveFrame;             /* 0x018: the curve frame (StartCurves, UpdateCurves) */
    u8 callActive;              /* 0x01A: 1 while callObj is in the sense list (EmVelociprey_PackCall); ChangeState removes it */
    u8 unk01B[0x020 - 0x01B];
    EmObj callObj;              /* 0x020: the pack call, the object other raptors notice (Em_SenseObjects) */
    u8 unk03C[0x044 - 0x03C];
    f32 wallVelZ;               /* 0x044: the leap attacks: velZ kept while a wall stops the leap */
    u8 unk048[0x050 - 0x048];
    s32 unk050;                 /* 0x050: the leap attacks clear it at the start; no reader */
    f32 spawnPos[3];            /* 0x054: Init: the position; Respawn puts it back */
    s32 spawnRotY;              /* 0x060: Init: rotY; Respawn puts it back */
    u8 unk064[0x06E - 0x064];
    u8 hunted;                  /* 0x06E: 1 once hunted (the group 5 deaths); Init and Respawn clear it */
    u8 unk06F[0x100 - 0x06F];
} EmVelocipreyWork;
typedef char EmVelocipreyWork_size_check[(sizeof(EmVelocipreyWork) == sizeof(EmWork)) ? 1 : -1];

/*
 * Velociprey's appear points (EmVelociprey_Reappear), by area: a position and
 * the point it faces, or a position and a heading. Four tables are read
 * field by field, each field through its own symbol (<table>_PosY ...,
 * aliases in config/ld/game_aliases.ld), which the match needs.
 */
typedef struct EmVelocipreyAppearPoint {
    f32 pos[3];
    f32 target[3];
} EmVelocipreyAppearPoint;
typedef struct EmVelocipreyAppearAt {
    f32 pos[3];
    u16 angle;
} EmVelocipreyAppearAt;

#endif /* EM_VELOCIPREY_H */
