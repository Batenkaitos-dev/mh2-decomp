#ifndef EM_VESPOID_H
#define EM_VESPOID_H

#include "em.h"

/*
 * The Vespoid's vocabulary (the code in em_vespoid.c), for its two kinds,
 * EM_KIND_VESPOID and EM_KIND_HORNETAUR: both run the same functions, and the
 * code tests the kind only in EmVespoid_Walk (the Hornetaur walks faster).
 *
 * Motions: row N of the family's motion table plays motion N, so one number
 * serves Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion)
 * kept in Em.motionId. The names say what the states that play a motion
 * show; motions no state plays stay numbers.
 *
 * Group 2 (EM_GROUP_FLY) is the flight: take-off, hovering, flying forward
 * and back, darts, flying to the goal or home, landing. Group 7
 * (EM_GROUP_EVENT2) is the fly-in after a reappearance.
 */
enum EmVespoidMotion {
    VESP_MOT_HOVER = 0x01,      /* VESP_FLY_HOVER, VESP_FLY_TO_GOAL, VESP_FLY_LAND, end of TAKE_OFF, event states */
    VESP_MOT_FLY_FORWARD = 0x02, /* VESP_FLY_FORWARD (forward curve), VESP_FLY_HOME, VESP_FLY_DART when velZ >= 0 */
    VESP_MOT_FLY_BACK = 0x03,   /* VESP_FLY_BACK / DASH_BACK (backward curves), VESP_FLY_DART when velZ < 0 */
    VESP_MOT_IDLE = 0x05,       /* VESP_IDLE on the ground */
    VESP_MOT_HOP = 0x06,        /* HOP_TURN, TAKE_OFF, the hop attacks: airborne from frame 6 */
    VESP_MOT_HOP_LAND = 0x07,   /* after VESP_MOT_HOP (frame 16 or 32), back on the ground */
    VESP_MOT_WALK = 0x08,       /* VESP_MOVE_WALK, Em_StepToGoal */
    VESP_MOT_RUN = 0x09,        /* VESP_MOVE_RUN and VESP_ATK_CHARGE (1.3 speed, VespoidAttack 11) */
    VESP_MOT_LUNGE = 0x0A,      /* VESP_ATK_LUNGE; EmVespoid_MotionEffects makes VespoidAttack 6 at frame 12 */
    VESP_MOT_STING = 0x0B,      /* VESP_ATK_STING_A/B, attack at frame 34 */
    VESP_MOT_AIR_FLINCH = 0x0C, /* VESP_REACT_AIR_FLINCH */
    VESP_MOT_FALL = 0x0D,       /* VESP_REACT_FALL, FALL_AND_LIE, DIE_CARVABLE in the air, VESP_CURVE_FALL */
    VESP_MOT_FALL_LAND = 0x0E,  /* on reaching the floor after VESP_MOT_FALL; the lying states skip their collapse after it */
    VESP_MOT_DIE_AIR = 0x0F,    /* VESP_DEFEAT_DIE in the air, fade from frame 100 */
    VESP_MOT_LIE = 0x10,        /* lying loop of STATUS2_LIE, STATUS4, FALL_AND_LIE, COLLAPSE_AND_LIE, DIE_CARVABLE */
    VESP_MOT_LAND = 0x11,       /* VESP_FLY_LAND touch-down */
    VESP_MOT_FLINCH = 0x12,     /* VESP_REACT_FLINCH on the ground */
    VESP_MOT_IDLE_ACTION = 0x13, /* VESP_IDLE_ACTION */
    VESP_MOT_DIE = 0x14,        /* VESP_DEFEAT_DIE on the ground, fade from frame 50 */
    VESP_MOT_COLLAPSE = 0x15,   /* start of STATUS2_LIE, STATUS4, COLLAPSE_AND_LIE, DIE_CARVABLE on the ground */
    VESP_MOT_GET_UP = 0x16      /* STATUS2_END and the end of the lying react states */
};

/* Group 0, EM_GROUP_IDLE (EmVespoid_RunIdleState): standing, the idle action, and lying under status 2. */
enum EmVespoidIdleState {
    VESP_IDLE_NOP = 0x00,        /* empty function */
    VESP_IDLE = 0x01,            /* ground idle VESP_MOT_IDLE; Start and EmVespoid_EndStateOnGround go here */
    VESP_IDLE_ACTION = 0x02,     /* VESP_MOT_IDLE_ACTION once */
    VESP_IDLE_STATUS2_LIE = 0x03, /* EM_DMG_STATUS2; Em_BeginStatus2, collapse and lie with puffs */
    VESP_IDLE_STATUS2_END = 0x04 /* Em_EndStatus2, VESP_MOT_GET_UP; EM_DMG_HIT_LYING */
};

/* Group 1, EM_GROUP_MOVE (EmVespoid_RunMoveState): walking, running and the hop that turns it. */
enum EmVespoidMoveState {
    VESP_MOVE_WALK = 0x00,    /* VESP_MOT_WALK to the goal or home */
    VESP_MOVE_RUN = 0x01,     /* VESP_MOT_RUN, speed scaled to goalDist */
    VESP_MOVE_HOP_TURN = 0x02 /* hop in place turning to the goal */
};

/* Group 2, EM_GROUP_FLY (EmVespoid_RunFlyState): the flight. */
enum EmVespoidFlyState {
    VESP_FLY_FORWARD = 0x00,   /* VESP_MOT_FLY_FORWARD, forward speed curve, 100 frames */
    VESP_FLY_BACK = 0x01,      /* VESP_MOT_FLY_BACK, backward speed curve, 100 frames */
    VESP_FLY_HOP_ATTACK = 0x02, /* hop with VespoidAttack 8 (as VESP_ATK_HOP) */
    VESP_FLY_HOVER = 0x03,     /* VESP_MOT_HOVER; EmVespoid_EndStateInAir goes here */
    VESP_FLY_TAKE_OFF = 0x04,  /* VESP_MOT_HOP into VESP_MOT_HOVER, airborne from frame 6 */
    VESP_FLY_TO_GOAL = 0x05,   /* hover on a velocity aimed at the goal */
    VESP_FLY_LAND = 0x06,      /* sink at velY -5, VESP_MOT_LAND on the floor */
    VESP_FLY_DASH_BACK = 0x07, /* EmVespoid_FlyBack variant 1: motionSpeed 8, short backward curve, 10 frames */
    VESP_FLY_DART = 0x08,      /* random sideways dart out and back */
    VESP_FLY_HOME = 0x09       /* to 20 above home, EmVespoid_MoveByVelocity */
};

/* Group 3, EM_GROUP_ATTACK (EmVespoid_RunAttackState): the lunge, the stings, the hop and the charge. */
enum EmVespoidAttackState {
    VESP_ATK_LUNGE = 0x00,  /* VESP_MOT_LUNGE, fast forward curve, attack 6 at frame 12 */
    VESP_ATK_STING_A = 0x01, /* VESP_MOT_STING, attack 7/12/16 by variant at frame 34 */
    VESP_ATK_HOP = 0x02,    /* VESP_MOT_HOP with VespoidAttack 8, lands */
    VESP_ATK_CHARGE = 0x03, /* VESP_MOT_RUN at 1.3 with VespoidAttack 11 */
    VESP_ATK_STING_B = 0x04 /* VESP_MOT_STING, attack 14/15/17 by variant */
};

/* Group 4, EM_GROUP_REACT (EmVespoid_RunReactState): what EmVespoid_Update picks from Em_UpdateDamage's result, and two falls only the scripts ask for. */
enum EmVespoidReactState {
    VESP_REACT_AIR_FLINCH = 0x00,      /* flinch/hit in the air, VESP_MOT_AIR_FLINCH */
    VESP_REACT_FALL = 0x01,            /* EM_DMG_STATUS_IN_AIR; VESP_MOT_FALL, VESP_MOT_FALL_LAND */
    VESP_REACT_STATUS4 = 0x02,         /* EM_DMG_STATUS4; Em_BeginStatus4/EndStatus4, lie */
    VESP_REACT_FLINCH = 0x03,          /* flinch/hit on the ground, VESP_MOT_FLINCH */
    VESP_REACT_FALL_AND_LIE = 0x04,    /* fall, land, lie 200 frames, get up (scripts only) */
    VESP_REACT_COLLAPSE_AND_LIE = 0x05 /* collapse, lie 200 frames, get up (scripts only) */
};

/* Group 5, EM_GROUP_DEFEAT (EmVespoid_RunDefeatState): death, with or without a carving point, and appearing again. */
enum EmVespoidDefeatState {
    VESP_DEFEAT_DIE = 0x00,         /* QuestEm_Hunted, VESP_MOT_DIE_AIR / VESP_MOT_DIE, fade out */
    VESP_DEFEAT_DIE_CARVABLE = 0x01, /* QuestEm_Hunted, fall/collapse, EmMine carve point 900 frames */
    VESP_DEFEAT_REAPPEAR = 0x02     /* QuestEm_Appear in EmVespoid_ChangeState; to VESP_ENTER_FLY_IN */
};

/* Group 6, EM_GROUP_EVENT (EmVespoid_RunEventState): the scripted appearances. */
enum EmVespoidEventState {
    VESP_EVENT_SHOW_A = 0x00, /* one frame hidden, then visible and idle */
    VESP_EVENT_SHOW_B = 0x01 /* same code as VESP_EVENT_SHOW_A */
};

/* Group 7, EM_GROUP_EVENT2 (EmVespoid_RunEnterState): the fly-in after a reappearance. */
enum EmVespoidEnterState {
    VESP_ENTER_FLY_IN = 0x00 /* wait 150, sink 30 frames fading in, then hover */
};

/* The curve sets EmVespoid_StartCurves picks (EmVespoidWork.curveKind): the velY and speed
 * curves that drive a flight, a lunge, a hop or a fall. */
enum EmVespoidCurve {
    VESP_CURVE_FLY_FORWARD = 0,     /* VESP_FLY_FORWARD */
    VESP_CURVE_FLY_BACK = 1,        /* VESP_FLY_BACK */
    VESP_CURVE_LUNGE = 2,           /* VESP_ATK_LUNGE: fast forward */
    VESP_CURVE_HOP = 3,             /* the take-off and the hop attacks */
    VESP_CURVE_FALL = 4,            /* a fall from the air */
    VESP_CURVE_UNUSED = 5,          /* set up, but no state starts it */
    VESP_CURVE_DASH_BACK = 6        /* VESP_FLY_DASH_BACK: a short dash back */
};

/* Vespoid's own meanings of the work area's bytes (EmWork, include/em.h, has the shared ones). */
typedef struct EmVespoidWork {
    u8 fxStep;                  /* 0x000: EmVespoid_UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x00C - 0x001];
    s16 unk00C;                 /* 0x00C: Init clears it; nothing in the family reads it */
    s16 unk00E;                 /* 0x00E: Init clears it; nothing in the family reads it */
    u8 unk010[0x014 - 0x010];
    f32 home[3];                /* 0x014: Init: the spawn position (posX, 0, posZ); EmVespoid_ChangeState copies it to the goal */
    s32 accelAngX;              /* 0x020: angles X, Y, Z for Em_MoveRotatedAccel (EmVespoid_UpdateCurves: 0, rotY, 0) */
    s32 accelAngY;              /* 0x024 */
    s32 accelAngZ;              /* 0x028 */
    u8 curveNone;               /* 0x02C: the three curves' run flags (EmVespoid_StartCurves, EmVespoid_UpdateCurves) */
    u8 curveVelY;               /* 0x02D */
    u8 curveSpeed;              /* 0x02E */
    u8 curveKind;               /* 0x02F: the curve tables' index (EmVespoid_StartCurves) */
    s16 curveFrame;             /* 0x030: the curve frame (EmVespoid_StartCurves, EmVespoid_UpdateCurves) */
    u8 unk032[0x034 - 0x032];
    f32 homeDist;               /* 0x034: EmVespoid_ChangeState for VESP_FLY_HOME: the distance to the goal set 20 above home; nothing reads it */
    u8 unk038[0x03C - 0x038];
    s16 dartLegFrames;          /* 0x03C: EmVespoid_Dart: its first leg's frames (3); nothing reads it */
    u8 unk03E[0x100 - 0x03E];
} EmVespoidWork;
typedef char EmVespoidWork_size_check[(sizeof(EmVespoidWork) == sizeof(EmWork)) ? 1 : -1];

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound and effect helpers read the position from). The
 * names say what the family's code shows a bone to be: the bones of the
 * attack hit shapes VespoidAttack_Create makes on given frames of a motion
 * (VespoidAttack_HitShapes, by the attack rows' hit lists) and where an
 * effect starts. The hit lists' other bones (10, 12, 13, 15, 25-27) belong
 * to rows this family's code doesn't create.
 */
enum EmVespoidBone {
    VESP_BONE_ROOT = 0,     /* the skeleton's first bone: every sound of the family plays here */
    VESP_BONE_BODY = 2,     /* the body's sphere in the hops' attack (VESP_MOT_HOP) and VESP_MOT_LUNGE */
    VESP_BONE_HEAD = 8,     /* the sphere of the charge (VESP_MOT_RUN), and where the lying puffs start */
    VESP_BONE_STING = 23    /* the sting's sphere in VESP_MOT_STING (from frame 34) */
};

/*
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind). No sound plays on the frame of a thing the code
 * shows (the sting's sound, 7, plays at frame 28, its hit at 34), so they
 * stay numbers.
 */

extern void *EmVespoid_Callbacks[8];
void EmVespoid_Init(Em *em);
void EmVespoid_Start(Em *em);
void EmVespoid_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmVespoid_Status2Lie(Em *em);
void EmVespoid_RunMoveState(Em *em);
void EmVespoid_FlyForward(Em *em, s32 unused);
void EmVespoid_FlyBack(Em *em, s16 dashBack);
void EmVespoid_Hover(Em *em);
void EmVespoid_TakeOff(Em *em);
void EmVespoid_Land(Em *em);
void EmVespoid_Dart(Em *em);
void EmVespoid_Lunge(Em *em);
void EmVespoid_StingA(Em *em);
void EmVespoid_StingB(Em *em);
void EmVespoid_FallFromAir(Em *em);
void EmVespoid_FallAndLie(Em *em);
void EmVespoid_Die(Em *em);
void EmVespoid_DieCarvable(Em *em);
void EmVespoid_Reappear(Em *em);
void EmVespoid_FlyIn(Em *em);
void EmVespoid_Update(Em *em);
void EmVespoid_RunState(Em *em);
void EmVespoid_StartCurves(Em *em, u8 kind);
u8 EmVespoid_UpdateCurves(Em *em);
void EmVespoid_MoveByVelocity(Em *em);

#endif
