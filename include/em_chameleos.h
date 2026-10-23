#ifndef EM_CHAMELEOS_H
#define EM_CHAMELEOS_H
#include "em.h"

/*
 * The Chameleos family (EM_KIND_CHAMELEOS): its motions and its states, by
 * group (em.h's EmStateGroup). The functions are EmChameleos_*. The monster
 * walks, flies and hovers (stance EM_STANCE_AIR in the air), attacks with its
 * tongue, which can steal an item, spits balls and breath, and cloaks itself:
 * with Em.fullCloak set it is invisible but for a shimmer every 900 frames,
 * without it it shimmers all the time; after its part break it can only
 * shimmer, and once part 0 is broken it cannot cloak at all. Each
 * enum lists the cases of the group's dispatcher (EmChameleos_Run*State); a
 * name ending in NONE is a number the dispatcher accepts but runs nothing
 * for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmChameleosMotion {
    CHAM_MOT_IDLE = 0x01,                   /* CHAM_IDLE, CHAM_EVENT_FOREST's pauses */
    CHAM_MOT_WALK = 0x02,                   /* CHAM_MOVE_WALK (Em_StepToGoal), CHAM_EVENT_FOREST */
    CHAM_MOT_RUN = 0x03,                    /* CHAM_MOVE_RUN and CHAM_MOVE_RUN_NO_HIT, to the goal */
    CHAM_MOT_RUN_STOP = 0x04,               /* the end of a run */
    CHAM_MOT_SPIN_TURN = 0x07,              /* CHAM_MOVE_SPIN_TURN, rotY falling 0x590 a frame from frame 44 to 90; a ground effect under bone 1 at 110 */
    CHAM_MOT_BACK_HOP = 0x09,               /* CHAM_MOVE_BACK_HOP, airborne from 44 to 82 */
    CHAM_MOT_TAKE_OFF = 0x0A,               /* CHAM_FLY_TAKE_OFF and CHAM_EVENT_LEAVE */
    CHAM_MOT_FLY = 0x0C,                    /* the forward flight of the route, area and goal flights: CHAM_FLY_FORWARD, _ROUTE, _ROUTE_LOOP, _OUT_OF_AREA, _TO_GOAL and _TO_GOAL_AND_LAND */
    CHAM_MOT_FLY_START = 0x0D,              /* going into forward flight from slow (CHAM_FLY_ROUTE; CHAM_FLY_ROUTE_LOOP and CHAM_FLY_TO_GOAL unless already flying) */
    CHAM_MOT_HOVER_SETTLE = 0x0F,           /* CHAM_FLY_BRAKE_AND_LAND after braking, before the landing */
    CHAM_MOT_BRAKE = 0x10,                  /* CHAM_FLY_BRAKE and CHAM_FLY_BRAKE_AND_LAND: slowing out of fast flight (curve 2) with rotX and rotZ levelling; near the floor, dust at frame 22 and a ground effect at 92 */
    CHAM_MOT_LAND = 0x11,                   /* CHAM_FLY_LAND: touching down (stance ground, camera shake) */
    CHAM_MOT_HOVER = 0x12,                  /* hovering: CHAM_FLY_HOVER, LAND, HOVER_TO_GOAL, INTO_AREA, THROUGH_AREA, ASCEND, TURN_TO_GOAL, BRAKE_AND_LAND, STRAFE_TO_GOAL and RISE, CHAM_EVENT_LEAVE and _LEAVE_AIR; near the floor, dust and an impact at frame 36 */
    CHAM_MOT_TAKE_OFF_END = 0x13,           /* the end of a take-off, 600 above the floor */
    CHAM_MOT_IDLE_ALERT = 0x1F,             /* CHAM_IDLE_ALERT */
    CHAM_MOT_TURN_TO_GOAL = 0x20,           /* CHAM_IDLE_FACE_GOAL */
    CHAM_MOT_IDLE_ACTION_A = 0x21,          /* CHAM_IDLE_ACTION_A */
    CHAM_MOT_CLOAK = 0x22,                  /* CHAM_IDLE_CLOAK_SHIMMER and CHAM_IDLE_CLOAK_FULL */
    CHAM_MOT_LIMP = 0x23,                   /* CHAM_MOVE_LIMP, moving on its root motion */
    CHAM_MOT_REST = 0x24,                   /* CHAM_IDLE_REST, lying with periodic puffs */
    CHAM_MOT_REST_END = 0x25,               /* CHAM_IDLE_REST_END */
    CHAM_MOT_TONGUE_LOW = 0x29,             /* CHAM_ATK_TONGUE_LOW: ChameleosAttack types 2 and 8 (type 2, the tongue's two spheres, at frame 66; type 8 at 68), dust at 74 and 102 */
    CHAM_MOT_SPIT_START = 0x2A,             /* the wind-up of the ball and breath attacks (ChameleosAttack type 9 at frame 86) */
    CHAM_MOT_SPIT_A = 0x2B,                 /* CHAM_ATK_BALL_PITCHED / CHAM_ATK_BREATH_PITCHED, the attack pitched 1820 from frame 10 */
    CHAM_MOT_SPIT_A_END = 0x2C,             /* after CHAM_MOT_SPIT_A */
    CHAM_MOT_STOMP = 0x2D,                  /* CHAM_ATK_STOMP: ChameleosAttack type 3 at frame 42, three ground effects under CHAM_BONE_FOOT_D (frames 44, 48 and 52), dust at 50 */
    CHAM_MOT_TAIL_SLAM = 0x2E,              /* CHAM_ATK_TAIL_SLAM: ChameleosAttack type 4 (backward) at frame 72; at 84 a ground effect, an attack burst and an impact under CHAM_BONE_TAIL_C */
    CHAM_MOT_BACK_HOP_BALL = 0x2F,          /* CHAM_ATK_BACK_HOP_BALL, airborne from frame 88 */
    CHAM_MOT_TAIL_POUNDS = 0x30,            /* CHAM_ATK_TAIL_POUNDS and the start of CHAM_ATK_TONGUE and CHAM_ATK_TONGUE_B: four pounds, ChameleosAttack type 7 at frames 64, 160, 256 and 354, and two frames later a ground effect, an attack burst and an impact under CHAM_BONE_TAIL_C */
    CHAM_MOT_TONGUE = 0x31,                 /* CHAM_ATK_TONGUE, CHAM_ATK_TONGUE_QUICK and the eating (CHAM_IDLE_EAT_NEED1, _NEED2): tongue (type 1) at frame 28, a ground effect 200 ahead of CHAM_BONE_TONGUE at 38 */
    CHAM_MOT_TONGUE_B = 0x32,               /* CHAM_ATK_TONGUE_B: ChameleosAttack types 5 and 1 */
    CHAM_MOT_TONGUE_HIGH = 0x33,            /* CHAM_ATK_TONGUE_HIGH: tongue (type 1) at frame 26, a ground effect 350 ahead of CHAM_BONE_TONGUE at 30 */
    CHAM_MOT_SPIT_B = 0x34,                 /* CHAM_ATK_BALL_LEVEL / CHAM_ATK_BREATH_LEVEL, the attack level from frame 6 */
    CHAM_MOT_SPIT_B_END = 0x35,             /* after CHAM_MOT_SPIT_B */
    CHAM_MOT_FLINCH_B = 0x3D,               /* CHAM_REACT_FLINCH_B and CHAM_REACT_FLINCH_C */
    CHAM_MOT_RECOVER = 0x3E,                /* after CHAM_MOT_COLLAPSE from frame 96 (CHAM_REACT_STAGGER, CHAM_REACT_DECLOAK, CHAM_EVENT_LEAVE); a ground effect under bone 1 at 64 */
    CHAM_MOT_FLINCH_A = 0x3F,               /* CHAM_REACT_FLINCH_A */
    CHAM_MOT_FALL = 0x40,                   /* falling out of the air (CHAM_REACT_FALL*, CHAM_REACT_DECLOAK_FALL, CHAM_DEFEAT_DIE_FALLING) */
    CHAM_MOT_FALL_LAND = 0x41,              /* hitting the floor after CHAM_MOT_FALL */
    CHAM_MOT_DOWN = 0x42,                   /* lying knocked down (CHAM_REACT_KNOCK_DOWN, STUN, FALL*) */
    CHAM_MOT_GET_UP = 0x43,                 /* getting up from a knock-down, a fall or statuses 1 and 2 */
    CHAM_MOT_DIE = 0x44,                    /* CHAM_DEFEAT_DIE */
    CHAM_MOT_STATUS4 = 0x45,                /* CHAM_REACT_STATUS4 */
    CHAM_MOT_SHAKE_OFF = 0x46,              /* CHAM_IDLE_SHAKE_OFF, STUN_SHAKE_OFF, STATUS4_SHAKE_OFF */
    CHAM_MOT_STATUS_DOWN = 0x47,            /* lying under statuses 1 and 2 (CHAM_IDLE_STATUS1_DOWN, CHAM_IDLE_STATUS2_DOWN); dust at frame 182 */
    CHAM_MOT_COLLAPSE = 0x49,               /* falling over: CHAM_REACT_STAGGER, DECLOAK, KNOCK_DOWN, STUN, CHAM_EVENT_LEAVE; dust at frame 166 */
    CHAM_MOT_PART_BREAK = 0x4A,             /* CHAM_REACT_PART_BREAK's start */
    CHAM_MOT_PART_BREAK_SPIN = 0x4B         /* CHAM_REACT_PART_BREAK, turning 630 a frame to frame 104 */
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound and effect helpers read the position from). The
 * names say what the family's code shows a bone to be: the bones of the
 * hit shapes ChameleosAttack_Create makes on given frames of a motion
 * (ChameleosAttack_HitShapes, by the attack rows' hit lists), where the
 * ball, the breath and the lying puffs start, where the tongue's and the
 * tail's blows strike the floor, and the feet that take turns in the walk
 * (the step sounds 3 to 5 at one pair, 0 to 2 at the other) and leave the
 * run's ground effects. "A" to "D" tell the parts of a kind apart: which
 * side or end each is, the code doesn't show. Bones no code explains stay
 * numbers: 1 (the ground effects under the body as it stops, turns, hops
 * and lands), 2 (the falls' sounds), 21 and 25 (spheres of the spit's
 * wind-up only), 38 (a sphere of CHAM_MOT_TONGUE_LOW's second hit; most
 * of the voice-like sounds), 46 and 50 (sounds only).
 */
enum EmChameleosBone {
    CHAM_BONE_BODY = 3,                     /* the running charge's widest sphere (radius 460, CHAM_MOVE_RUN) */
    CHAM_BONE_FOOT_A = 8,                   /* the walk's step sounds 3 to 5 take turns here and at CHAM_BONE_FOOT_B; a ground effect in CHAM_MOT_RUN */
    CHAM_BONE_FOOT_B = 13,
    CHAM_BONE_TAIL_A = 14,                  /* the tail's widest sphere (radius 235) in the tail slam and the pounds */
    CHAM_BONE_TAIL_B = 15,
    CHAM_BONE_TAIL_C = 16,                  /* where the tail slam and the pounds strike the floor (ground effect, attack burst, impact) */
    CHAM_BONE_TAIL_D = 17,                  /* the pounds' sounds (CHAM_SE_POUND) and the tail slam's (CHAM_SE_TAIL_SLAM) play here */
    CHAM_BONE_FOOT_C = 26,                  /* the walk's step sounds 0 to 2 take turns here and at CHAM_BONE_FOOT_D */
    CHAM_BONE_STOMP_A = 29,                 /* the stomp's two spheres (ChameleosAttack types 3 and 5) */
    CHAM_BONE_STOMP_B = 30,
    CHAM_BONE_FOOT_D = 31,                  /* also where the stomp's ground effects rise */
    CHAM_BONE_BREATH = 37,                  /* where the ball and the breath start (EmEffectAttack_Bones) and the lying puffs; with CHAM_BONE_BODY, the charge's sphere */
    CHAM_BONE_TONGUE_B = 40,                /* CHAM_MOT_TONGUE_LOW's second tongue sphere */
    CHAM_BONE_TONGUE = 41                   /* the tongue's sphere in every tongue attack; the tongue's ground effect rises ahead of it */
};

/*
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind; Em_PlaySeAtBone and its frame variants). A sound
 * is named where the code shows what it is: it plays on or next to the
 * frame of the thing it goes with. The others stay numbers; the steps'
 * sounds show only where they play.
 */
enum EmChameleosSe {
    CHAM_SE_TAIL_SLAM = 6,                  /* two frames after the tail slam's impact (CHAM_MOT_TAIL_SLAM) */
    CHAM_SE_THUD = 7,                       /* the body meeting the floor: two frames after the body's dust in CHAM_MOT_STATUS_DOWN, _COLLAPSE and _PART_BREAK, four in _DIE, and on the frame of CHAM_MOT_SPIN_TURN's ground effect under bone 1; also in CHAM_MOT_LAND (frame 58), _FALL_LAND (24) and _RECOVER (74), frames where the code shows no event */
    CHAM_SE_POUND = 8,                      /* on the frames of the tail's pounds in CHAM_MOT_TAIL_POUNDS (four after the first); also at bone 50 at frame 16 of CHAM_MOT_SPIT_A_END and _SPIT_B_END, where the code shows no event */
    CHAM_SE_STOMP = 21,                     /* at CHAM_BONE_FOOT_D on the frame of the stomp's first ground effect, and two frames before CHAM_MOT_TONGUE_B's stomp hit; also at bone 2 at frame 50 of CHAM_MOT_TAKE_OFF, where the code shows no event */
    CHAM_SE_TONGUE_STRIKE = 36              /* at CHAM_BONE_TONGUE on the frame the tongue's ground effect or dust rises in every tongue motion, and four frames after CHAM_MOT_TONGUE_LOW's second dust */
};

/* Group 0, EM_GROUP_IDLE (EmChameleos_RunIdleState). */
enum EmChameleosIdleState {
    CHAM_IDLE_STAND = 0x00,                 /* layered idle until the timer runs out */
    CHAM_IDLE = 0x01,                       /* idle once */
    CHAM_IDLE_ALERT = 0x02,                 /* idle in combat */
    CHAM_IDLE_FACE_GOAL = 0x03,             /* turn motion facing the goal */
    CHAM_IDLE_CLOAK_FULL = 0x04,            /* allows the full cloak (EmChameleos_SetFullCloak) */
    CHAM_IDLE_CLOAK_SHIMMER = 0x05,         /* only the shimmering cloak (EmChameleos_ClearFullCloak) */
    CHAM_IDLE_ACTION_A = 0x09,              /* one idle motion */
    CHAM_IDLE_EAT_NEED1 = 0x0A,             /* eats with the tongue, refills need 1 */
    CHAM_IDLE_EAT_NEED2 = 0x0B,             /* eats three times, refills need 2 */
    CHAM_IDLE_REST = 0x12,                  /* lies regenerating hp */
    CHAM_IDLE_STATUS2_DOWN = 0x14,          /* lies under status 2 */
    CHAM_IDLE_REST_END = 0x17,              /* gets up from rest */
    CHAM_IDLE_STATUS2_END = 0x18,           /* gets up from status 2 */
    CHAM_IDLE_SHAKE_OFF = 0x19,             /* shakes itself */
    CHAM_IDLE_STATUS1_DOWN = 0x1B,          /* lies under status 1 */
    CHAM_IDLE_STATUS1_END = 0x1C,           /* gets up from status 1 */
    CHAM_IDLE_STUN_SHAKE_OFF = 0x20,        /* shakes off the stun (Em_EndStatus5) */
    CHAM_IDLE_STATUS4_SHAKE_OFF = 0x21      /* shakes off status 4 (Em_EndStatus4) */
};

/* Group 1, EM_GROUP_MOVE (EmChameleos_RunMoveState). */
enum EmChameleosMoveState {
    CHAM_MOVE_WALK = 0x00,                  /* walk to the goal */
    CHAM_MOVE_LIMP = 0x01,                  /* slow walk below 15% hp */
    CHAM_MOVE_TURN = 0x02,                  /* turn to the target */
    CHAM_MOVE_NONE = 0x03,                  /* the dispatcher runs nothing */
    CHAM_MOVE_RUN = 0x04,                   /* run to the goal with a hit */
    CHAM_MOVE_SPIN_TURN = 0x05,             /* spinning turn */
    CHAM_MOVE_BACK_HOP = 0x06,              /* hop backward */
    CHAM_MOVE_RUN_NO_HIT = 0x07,            /* run without the hit */
    CHAM_MOVE_TURN_RANDOM = 0x08            /* turn to a random point 500 away */
};

/* Group 2, EM_GROUP_FLY (EmChameleos_RunFlyState). */
enum EmChameleosFlyState {
    CHAM_FLY_TAKE_OFF = 0x00,               /* take off to hover */
    CHAM_FLY_LAND = 0x01,                   /* sink and land */
    CHAM_FLY_HOVER = 0x02,                  /* hover once */
    CHAM_FLY_NONE_A = 0x03,                 /* empty (EndAirState 2) */
    CHAM_FLY_NONE_B = 0x04,                 /* empty */
    CHAM_FLY_NONE_C = 0x05,                 /* empty */
    CHAM_FLY_ROUTE = 0x06,                  /* four legs between area points */
    CHAM_FLY_HOVER_TO_GOAL = 0x07,          /* hover to the goal turning */
    CHAM_FLY_OUT_OF_AREA = 0x08,            /* fly out to the next area */
    CHAM_FLY_INTO_AREA = 0x09,              /* fly to the area point and land */
    CHAM_FLY_NONE_D = 0x0A,                 /* empty */
    CHAM_FLY_TO_GOAL_AND_LAND = 0x0B,       /* fly to the goal and land */
    CHAM_FLY_NONE_E = 0x0C,                 /* empty */
    CHAM_FLY_THROUGH_AREA = 0x0D,           /* cross an area on the way */
    CHAM_FLY_ASCEND = 0x0E,                 /* rise to the area's height */
    CHAM_FLY_FORWARD = 0x0F,                /* fly straight on once */
    CHAM_FLY_TURN_TO_GOAL = 0x10,           /* hover turning to the goal */
    CHAM_FLY_ROUTE_LOOP = 0x11,             /* endless flight between area points */
    CHAM_FLY_BRAKE_AND_LAND = 0x12,         /* brake from fast flight, then land */
    CHAM_FLY_TO_GOAL = 0x13,                /* fly to the goal */
    CHAM_FLY_NONE_F = 0x14,                 /* empty */
    CHAM_FLY_STRAFE_TO_GOAL = 0x15,         /* hover straight at the goal without turning */
    CHAM_FLY_BRAKE = 0x16,                  /* brake to a hover */
    CHAM_FLY_NONE_G = 0x17,                 /* empty */
    CHAM_FLY_RISE = 0x18                    /* rise 1000 above the goal */
};

/* Group 3, EM_GROUP_ATTACK (EmChameleos_RunAttackState). */
enum EmChameleosAttackState {
    CHAM_ATK_TONGUE = 0x00,                 /* tail thumps then tongue lash */
    CHAM_ATK_TONGUE_LOW = 0x01,             /* tongue at a target on its level */
    CHAM_ATK_STOMP = 0x02,                  /* ChameleosAttack 3, ground effects under CHAM_BONE_FOOT_D */
    CHAM_ATK_TAIL_SLAM = 0x03,              /* backward slam with impact */
    CHAM_ATK_TONGUE_B = 0x04,               /* tail thumps then the second tongue lash */
    CHAM_ATK_TONGUE_HIGH = 0x05,            /* tongue at a raised target */
    CHAM_ATK_BALL_PITCHED = 0x06,           /* poison ball, pitched */
    CHAM_ATK_BALL_LEVEL = 0x07,             /* poison ball, level */
    CHAM_ATK_BACK_HOP_BALL = 0x08,          /* leap back into the air with a ball */
    CHAM_ATK_BREATH_PITCHED = 0x09,         /* mist breath, pitched */
    CHAM_ATK_BREATH_LEVEL = 0x0A,           /* mist breath, level */
    CHAM_ATK_TONGUE_QUICK = 0x0B,           /* tongue lash alone */
    CHAM_ATK_TAIL_POUNDS = 0x0C             /* tail thumps alone */
};

/* Group 4, EM_GROUP_REACT (EmChameleos_RunReactState). */
enum EmChameleosReactState {
    CHAM_REACT_FLINCH_A = 0x00,             /* flinch of part 0 or 6 */
    CHAM_REACT_FLINCH_B = 0x01,             /* flinch of parts 1 to 5 */
    CHAM_REACT_STAGGER = 0x02,              /* broken part's flinch */
    CHAM_REACT_KNOCK_DOWN = 0x03,           /* broken part 3/4's flinch, lies */
    CHAM_REACT_PART_BREAK = 0x04,           /* horn broken */
    CHAM_REACT_STUN = 0x05,                 /* stunned (EM_DMG_STATUS5) */
    CHAM_REACT_NONE_A = 0x06,               /* empty */
    CHAM_REACT_NONE_B = 0x07,               /* empty */
    CHAM_REACT_FALL = 0x08,                 /* falls out of the air */
    CHAM_REACT_FALL_BLIND = 0x09,           /* falls, blind */
    CHAM_REACT_FLINCH_C = 0x0A,             /* copy of FLINCH_B */
    CHAM_REACT_STATUS4 = 0x0B,              /* under status 4 */
    CHAM_REACT_DECLOAK = 0x0C,              /* attack-kind-5 hit while visible: cloaks, drops a mine point */
    CHAM_REACT_DECLOAK_FALL = 0x0D          /* the same in the air */
};

/* Group 5, EM_GROUP_DEFEAT (EmChameleos_RunDefeatState). */
enum EmChameleosDefeatState {
    CHAM_DEFEAT_DIE = 0x00,                 /* dies on the ground */
    CHAM_DEFEAT_NONE = 0x01,                /* empty */
    CHAM_DEFEAT_DIE_FALLING = 0x02,         /* dies in the air */
    CHAM_DEFEAT_REAPPEAR = 0x03             /* appears again */
};

/* Group 6, EM_GROUP_EVENT (EmChameleos_RunEventState). */
enum EmChameleosEventState {
    CHAM_EVENT_FOREST = 0x00,               /* Forest and Hills area 9 event walk */
    CHAM_EVENT_LEAVE = 0x01,                /* quest over: flies off */
    CHAM_EVENT_LEAVE_AIR = 0x02             /* quest over in the air: flies off */
};

extern void *EmChameleos_Callbacks[8];
void EmChameleos_MotionSounds(Em *em, EmWork *work);
void EmChameleos_Init(Em *em);
void EmChameleos_Start(Em *em);
void EmChameleos_Eat(Em *em, EmWork *work, u8 variant);
void EmChameleos_Limp(Em *em, EmWork *work);
void EmChameleos_Turn(Em *em, EmWork *work, u8 variant);
void EmChameleos_BackHop(Em *em, EmWork *work);
void EmChameleos_Land(Em *em, EmWork *work);
void EmChameleos_FlyOutOfArea(Em *em, EmWork *work);
void EmChameleos_FlyIntoArea(Em *em, EmWork *work);
void EmChameleos_FlyThroughArea(Em *em, EmWork *work);
void EmChameleos_FlyRouteLoop(Em *em, EmWork *work);
void EmChameleos_MistPitched(Em *em, s32 unused, u8 variant);
void EmChameleos_MistLevel(Em *em, s32 unused, u8 variant);
void EmChameleos_KnockDown(Em *em, EmWork *work, u8 variant);
void EmChameleos_ForestEvent(Em *em, EmWork *work);
void EmChameleos_LeaveQuest(Em *em, EmWork *work);
void EmChameleos_LeaveQuestFromAir(Em *em, EmWork *work);
void EmChameleos_Die(Em *em, EmWork *work);
void EmChameleos_DieFalling(Em *em, EmWork *work);
void EmChameleos_RunIdleState(Em *em, EmWork *work);
void EmChameleos_RunMoveState(Em *em, EmWork *work);
void EmChameleos_RunReactState(Em *em, EmWork *work);
void EmChameleos_Update(Em *em);
void EmChameleos_RunState(Em *em, s32 work);
void EmChameleos_BreakParts(Em *em, s32 apply);
void EmChameleos_SpawnGroundEffect(Em *em, s32 bone, s32 kind, f32 scale);
void EmChameleos_MotionEffects(Em *em, EmChameleosWork *work);

void EmChameleos_SetIdleState(Em *em, u16 state, u8 mode);
void EmChameleos_SetMoveState(Em *em, u16 state, u8 mode);
void EmChameleos_SetFlyState(Em *em, u16 state, u8 mode);
void EmChameleos_SetAttackState(Em *em, u16 state, u8 mode);
void EmChameleos_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmChameleos_StartCurves(Em *em, u8 kind);
u8 EmChameleos_UpdateCurves(Em *em);
void EmChameleos_SetFullCloak(Em *em);
void EmChameleos_ClearFullCloak(Em *em);
s32 EmChameleos_IsInvisible(Em *em);
s32 EmChameleos_HasFullCloak(Em *em);
void EmChameleos_UpdateInvisibility(Em *em);
s32 EmChameleos_WantsInvisible(Em *em);
s32 EmChameleos_InvisibilityBlocked(Em *em);
void EmChameleos_SpawnLyingPuffs(Em *em);
s32 EmChameleos_TongueHitPlayer(Em *em);

#endif
