#ifndef EM_APCEROS_H
#define EM_APCEROS_H
#include "em.h"

/*
 * The Apceros family (EM_KIND_APCEROS; the per-kind hooks send the Popo,
 * EM_KIND_POPO, and kind 12 to the same functions): its motions and its
 * states, by group (em.h's EmStateGroup). The functions are EmApceros_*.
 * These herbivores graze near their spawn point, watch and turn on hunters
 * when the herd is hurt (EmApceros_CheckHerdHurt), and fade in as they
 * arrive (group 7). Each enum lists the cases of the group's
 * dispatcher (EmApceros_Run*State); where one state function serves several
 * numbers, the names say which variant the number asks for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmApcerosMotion {
    APC_MOT_STAND = 0x01,                   /* EmApceros_Init's first motion, APC_IDLE_STAND and the event stands (APC_EVENT_INTRO_WAIT, APC_EVENT_INTRO_PREY, APC_EVENT_RESHOW) */
    APC_MOT_REAR_UP = 0x02,                 /* APC_IDLE_STOMP and APC_ATK_STOMP, before APC_MOT_STOMP */
    APC_MOT_IDLE_LOOP_A = 0x03,             /* APC_IDLE_LOOP_A (360 to 570 frames) */
    APC_MOT_IDLE_LOOP_B = 0x04,             /* APC_IDLE_LOOP_B (180 to 270 frames) */
    APC_MOT_IDLE_ACTION_A = 0x05,           /* APC_IDLE_ACTION_A */
    APC_MOT_IDLE_ACTION_B = 0x06,           /* APC_IDLE_ACTION_B, and APC_ATK_B_LEAD before its attack (to frame 102, then APC_MOT_ATTACK_B) */
    APC_MOT_STOMP = 0x07,                   /* after APC_MOT_REAR_UP (APC_IDLE_STOMP, APC_ATK_STOMP): at frame 40 a ground effect under APC_BONE_FOOT_A2, a camera shake and attack 13 (its sphere on that foot) */
    APC_MOT_WALK = 0x0B,                    /* APC_IDLE_CIRCLE_HOME, APC_MOVE_WANDER_HOME, APC_MOVE_WALK, APC_DEFEAT_FADE and APC_ENTER_WALK */
    APC_MOT_RUN = 0x0D,                     /* APC_MOVE_RUN, APC_IDLE_LEAVE, APC_ENTER_RUN */
    APC_MOT_TURN_A = 0x15,                  /* APC_MOVE_WALK_TURN_A and APC_MOVE_TURN_TO_GOAL (rotY up, frames 42 to 170) */
    APC_MOT_TURN_B = 0x16,                  /* APC_MOVE_WALK_TURN_B and APC_MOVE_TURN_TO_GOAL (rotY down) */
    APC_MOT_ALERT = 0x1E,                   /* APC_IDLE_ALERT and the start of APC_MOVE_WATCH_HUNTER, before it looks for a hunter */
    APC_MOT_ATTACK_REAR = 0x1F,             /* APC_ATK_REAR: attack 9 (kind 12, Popo: 1) at frame 30 */
    APC_MOT_WATCH = 0x20,                   /* APC_IDLE_WATCH, APC_MOVE_WATCH_HUNTER (watching, turning 66 a frame to the hunter's side) and APC_MOVE_WATCH_TURN_BACK */
    APC_MOT_WATCH_TURN_A = 0x21,            /* APC_MOVE_WATCH_HUNTER and APC_MOVE_TURN_BACK: turning with rotY up (0x1D7 a frame, frames 6 to 122) */
    APC_MOT_WATCH_TURN_B = 0x22,            /* the same turning with rotY down */
    APC_MOT_ALERT_END = 0x23,               /* APC_IDLE_ALERT_END and the end of APC_MOVE_WATCH_HUNTER */
    APC_MOT_ATTACK_B = 0x24,                /* APC_ATK_B and APC_ATK_B_LEAD: attack 2 at frame 20 (Popo: attack 18 at frame 32, dust under APC_BONE_FOOT_B2 at frame 24, a ground effect at frame 52) */
    APC_MOT_SPIN_ATTACK = 0x25,             /* APC_ATK_SPIN: turns 0x1C7 a frame */
    APC_MOT_REACT_RESET = 0x3C,             /* APC_REACT_RESET_HP only */
    APC_MOT_FLINCH_B = 0x3D,                /* APC_REACT_FLINCH_B, APC_REACT_FLINCH_RAND, the start and end of APC_REACT_STATUS4, APC_EVENT_FLINCH */
    APC_MOT_FLINCH_A = 0x3E,                /* APC_REACT_FLINCH_A, APC_REACT_FLINCH_RAND, the stagger of APC_REACT_KNOCK_DOWN_A and of EmApceros_Die before APC_MOT_FALL_B */
    APC_MOT_FALL = 0x3F,                    /* APC_IDLE_STATUS2_COLLAPSE, APC_REACT_KNOCK_DOWN_B, EmApceros_Die's other fall and APC_EVENT_FALL, each pushed sideways; dust at frame 28 */
    APC_MOT_LIE = 0x40,                     /* the lying of the knock-downs, of Die and of APC_EVENT_LIE */
    APC_MOT_DIE = 0x41,                     /* Die and APC_EVENT_DIE: frozen at frame 50 as a carcass */
    APC_MOT_GET_UP = 0x42,                  /* APC_IDLE_STATUS2_END and the knock-downs */
    APC_MOT_FALL_B = 0x43,                  /* the fall after the stagger of APC_REACT_KNOCK_DOWN_A and of EmApceros_Die */
    APC_MOT_STATUS2_LIE = 0x44,             /* APC_IDLE_STATUS2_COLLAPSE: lying under status 2, with lying puffs at APC_BONE_BREATH, until Em_SetStatus2Timer's time runs out */
    APC_MOT_STATUS4 = 0x45                  /* APC_REACT_STATUS4: the status loop */
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound and effect helpers read the position from); the
 * three kinds' skeletons share the numbers the family's code uses. The names
 * say what the code shows a bone to be: the kinds' own hit shapes
 * (gEmBoneHits), the shapes of the hits VespoidAttack_Create makes on given
 * frames (VespoidAttack_HitShapes, by the attack rows' hit lists), where an
 * effect starts, and the bones the steps' sounds take turns at
 * (EmApceros_MotionSounds). "A" and "B" tell the two pairs of feet apart;
 * which pair is in front the code doesn't show. Bones no code explains stay
 * numbers: 7 (the Popo's ground effect in APC_MOT_ATTACK_B), 12 (the sphere
 * of the Apceros's attack 2), 13 (the second sphere of the kinds' own hit
 * shapes, radius 60 to 80) and 16 (most sounds but the steps play there).
 */
enum EmApcerosBone {
    APC_BONE_ROOT = 0,      /* the skeleton's first bone: the falls' sounds (APC_SE_FALL) and the ground effects of APC_MOT_LIE */
    APC_BONE_BODY = 2,      /* the body's sphere (radius 165) in the kinds' own hit shapes, and in the Popo's attack 18; the hit sparks of the event fall, lie and death (EmApceros_SpawnHitSparkAtBody); the status-4 effect (Em_SpawnStatus4Effect) */
    APC_BONE_FOOT_A1 = 6,   /* the first pair of feet: the walk's and the turns' steps play sound 1, the run's sound 3, at 6 and 10 in turn */
    APC_BONE_FOOT_A2 = 10,  /* the other foot of pair A; APC_MOT_STOMP lands under it at frame 40 (the ground effect and attack 13's sphere, radius 150) */
    APC_BONE_BREATH = 15,   /* where the lying puffs start (APC_MOT_STATUS2_LIE); the Popo's attack 18 has its first sphere here; the poison effect (Em_UpdateStatus) */
    APC_BONE_FOOT_B1 = 20,  /* the second pair of feet: the walk's steps play sound 4, the run's sound 6, at 20 and 23 in turn; the run's footstep effects (frame 52) */
    APC_BONE_FOOT_B2 = 23,  /* the other foot of pair B: the run's footstep effect at frame 24; the Popo's dust in APC_MOT_ATTACK_B (DustCloud place 44) */
    APC_BONE_TAIL_A = 25,   /* the chain of three spheres (radius 85, 65, 85) of the attack against a hunter behind it (APC_MOT_ATTACK_REAR) and of the spin (APC_MOT_SPIN_ATTACK): VespoidAttack's hit list 0 */
    APC_BONE_TAIL_B = 26,
    APC_BONE_TAIL_C = 27    /* the chain's end: sound 20 plays here in the rear attack and the spin */
};

/*
 * Sounds: a sound number indexes the kind's own sound bank (Em_PlaySeChecked
 * through EmApceros_PlaySeOnFrame). A sound is named where the code shows
 * what it is: it plays on the frame of the thing it goes with. The others
 * stay numbers.
 */
enum EmApcerosSe {
    APC_SE_FALL = 18        /* the body hitting the ground: on the frame of APC_MOT_FALL's dust cloud (28) at APC_BONE_ROOT, and in APC_MOT_FALL_B */
};
#define APC_SE_SMALL_SIZE 32    /* added to a sound's number for kind 12 at scaleX <= 0.5 and the Popo at scaleX <= 0.8: the bank's small-size copies */

/* Group 0, EM_GROUP_IDLE (EmApceros_RunIdleState). */
enum EmApcerosIdleState {
    APC_IDLE_NONE = 0x00,                   /* no function: the dispatcher does nothing */
    APC_IDLE_STAND = 0x01,                  /* stand once, then EndState */
    APC_IDLE_CIRCLE_HOME = 0x02,            /* walk in a circle near home */
    APC_IDLE_LOOP_A = 0x03,                 /* long idle loop A */
    APC_IDLE_STOMP = 0x04,                  /* rear up and stomp */
    APC_IDLE_LOOP_B = 0x05,                 /* idle loop B */
    APC_IDLE_ACTION_A = 0x06,               /* one idle motion A */
    APC_IDLE_ACTION_B = 0x07,               /* one idle motion B */
    APC_IDLE_STATUS2_COLLAPSE = 0x08,       /* falls and lies under status 2 */
    APC_IDLE_STATUS2_END = 0x09,            /* gets up after status 2 */
    APC_IDLE_ALERT = 0x0A,                  /* raises the head */
    APC_IDLE_ALERT_END = 0x0B,              /* lowers the head */
    APC_IDLE_LEAVE = 0x0C,                  /* runs off fading out and leaves the quest */
    APC_IDLE_WATCH = 0x0D                   /* watches once */
};

/* Group 1, EM_GROUP_MOVE (EmApceros_RunMoveState). */
enum EmApcerosMoveState {
    APC_MOVE_WANDER_HOME = 0x00,            /* walks in a curve around home */
    APC_MOVE_WATCH_HUNTER = 0x01,           /* watches a hunter, turns its back, attacks behind */
    APC_MOVE_WALK_TURN_A = 0x02,            /* walking turn, rotY up */
    APC_MOVE_WALK_TURN_B = 0x03,            /* walking turn, rotY down */
    APC_MOVE_TURN_TO_GOAL = 0x04,           /* walking turn toward the goal */
    APC_MOVE_RUN = 0x05,                    /* runs to the goal */
    APC_MOVE_WATCH_TURN_BACK = 0x06,        /* watching, turns its back on the goal */
    APC_MOVE_TURN_BACK = 0x07,              /* turns its back on the goal */
    APC_MOVE_WALK = 0x08,                   /* walks to the goal */
    APC_MOVE_WARP = 0x09,                   /* placed on the goal, ends at once */
    APC_MOVE_TURN_TO_TARGET = 0x0A          /* turns to the target (turn table) */
};

/* Group 3, EM_GROUP_ATTACK (EmApceros_RunAttackState). */
enum EmApcerosAttackState {
    APC_ATK_REAR = 0x00,                    /* attacks the hunter behind it */
    APC_ATK_B_LEAD = 0x01,                  /* idle action B, then attack B */
    APC_ATK_SPIN = 0x02,                    /* turning attack */
    APC_ATK_B = 0x03,                       /* attack B alone */
    APC_ATK_STOMP = 0x04                    /* rear up and stomp */
};

/* Group 4, EM_GROUP_REACT (EmApceros_RunReactState). */
enum EmApcerosReactState {
    APC_REACT_RESET_HP = 0x00,              /* one motion, then idle with hp 64 */
    APC_REACT_FLINCH_A = 0x01,              /* flinch pushed to -X */
    APC_REACT_FLINCH_B = 0x02,              /* flinch pushed to +X */
    APC_REACT_KNOCK_DOWN_A = 0x03,          /* KnockDown variant 0: stagger, fall, lie, get up */
    APC_REACT_KNOCK_DOWN_B = 0x04,          /* KnockDown variant 1: fall sideways, lie, get up */
    APC_REACT_FLINCH_RAND = 0x05,           /* flinch to a random side, request 4 */
    APC_REACT_STATUS4 = 0x06                /* status 4: flinch, status loop, flinch */
};

/* Group 5, EM_GROUP_DEFEAT (EmApceros_RunDefeatState). */
enum EmApcerosDefeatState {
    APC_DEFEAT_DIE_A = 0x00,                /* Die variant 0: stagger, fall, die, carcass */
    APC_DEFEAT_DIE_B = 0x01,                /* Die variant 1: fall sideways, die, carcass */
    APC_DEFEAT_FADE = 0x02,                 /* killed while walking off: fades out, no carcass */
    APC_DEFEAT_WAIT_REAPPEAR = 0x03         /* hidden until it may reappear */
};

/* Group 6, EM_GROUP_EVENT (EmApceros_RunEventState). */
enum EmApcerosEventState {
    APC_EVENT_FLINCH = 0x00,                /* scene: flinch, then fall */
    APC_EVENT_FALL = 0x01,                  /* scene: falls over wounded (Effect02) */
    APC_EVENT_LIE = 0x02,                   /* scene: lies until the watched object acts */
    APC_EVENT_DIE = 0x03,                   /* scene: dies */
    APC_EVENT_INTRO_WAIT = 0x04,            /* opening scene: stands until EVENT_FLAG_RATHALOS_SCENE */
    APC_EVENT_INTRO_PREY = 0x05,            /* opening scene: placed at a fixed spot (variant 0) */
    APC_EVENT_RESHOW = 0x06                 /* hidden one frame, then shown */
};

/* Group 7 (EmApceros_RunEnterState): the arrivals, fading in. */
enum EmApcerosEnterState {
    APC_ENTER_RUN = 0x00,                   /* group 7 shows arrivals: fades in while running */
    APC_ENTER_WALK = 0x01                   /* fades in while walking */
};

extern void *EmApceros_Callbacks[8];
void EmApceros_Init(Em *em);
void EmApceros_Start(Em *em);
void EmApceros_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmApceros_EndState(Em *em);
void EmApceros_IdleCircleHome(Em *em);
void EmApceros_Status2Collapse(Em *em);
void EmApceros_WanderHome(Em *em);
void EmApceros_WatchHunter(Em *em);
void EmApceros_WalkTurnA(Em *em);
void EmApceros_WalkTurnB(Em *em);
void EmApceros_TurnToGoal(Em *em);
void EmApceros_RunToGoal(Em *em);
void EmApceros_WalkToGoal(Em *em);
void EmApceros_FlinchA(Em *em);
void EmApceros_FlinchB(Em *em);
void EmApceros_KnockDown(Em *em, s32 fallKind);
void EmApceros_FlinchRandom(Em *em);
void EmApceros_Status4(Em *em);
void EmApceros_Die(Em *em, s32 fallKind);
void EmApceros_SpawnHitSparkAtBody(Em *em, f32 scale, s32 unused);
void EmApceros_EventFall(Em *em);
void EmApceros_EventIntroPrey(Em *em);
void EmApceros_EventReshow(Em *em);
void EmApceros_Update(Em *em);
void EmApceros_RunState(Em *em);
void EmApceros_MotionEffects(Em *em, EmWork *work);

#endif
