#ifndef EM_APCEROS_H
#define EM_APCEROS_H

/*
 * The Apceros family (EM_KIND_APCEROS; the per-kind hooks send the Popo,
 * EM_KIND_POPO, and kind 0x0C to the same functions): its motions and its
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
    APC_MOT_STAND = 0x01,                   /* APC_IDLE_STAND, the event stands (APC_EVENT_INTRO_WAIT, ... */
    APC_MOT_REAR_UP = 0x02,                 /* APC_IDLE_STOMP and APC_ATK_STOMP, before APC_MOT_STOMP */
    APC_MOT_IDLE_LOOP_A = 0x03,             /* APC_IDLE_LOOP_A (360 to 570 frames) */
    APC_MOT_IDLE_LOOP_B = 0x04,             /* APC_IDLE_LOOP_B (180 to 270 frames) */
    APC_MOT_IDLE_ACTION_A = 0x05,           /* APC_IDLE_ACTION_A */
    APC_MOT_IDLE_ACTION_B = 0x06,           /* APC_IDLE_ACTION_B and the lead of APC_ATK_B_LEAD (to frame ... */
    APC_MOT_STOMP = 0x07,                   /* after APC_MOT_REAR_UP: ground effect at bone 0xA, camera ... */
    APC_MOT_WALK = 0x0B,                    /* APC_IDLE_CIRCLE_HOME, APC_MOVE_WANDER_HOME, APC_MOVE_WALK, ... */
    APC_MOT_RUN = 0x0D,                     /* APC_MOVE_RUN, APC_IDLE_LEAVE, APC_ENTER_RUN */
    APC_MOT_TURN_A = 0x15,                  /* APC_MOVE_WALK_TURN_A and APC_MOVE_TURN_TO_GOAL (rotY up ... */
    APC_MOT_TURN_B = 0x16,                  /* APC_MOVE_WALK_TURN_B and APC_MOVE_TURN_TO_GOAL (rotY down) */
    APC_MOT_ALERT = 0x1E,                   /* APC_IDLE_ALERT and the start of APC_MOVE_WATCH_HUNTER: ... */
    APC_MOT_ATTACK_REAR = 0x1F,             /* APC_ATK_REAR: attack 9 (kind 0x0C, Popo: 1) at frame 30 */
    APC_MOT_WATCH = 0x20,                   /* APC_IDLE_WATCH, APC_MOVE_WATCH_HUNTER, ... */
    APC_MOT_WATCH_TURN_A = 0x21,            /* APC_MOVE_WATCH_HUNTER and APC_MOVE_TURN_BACK: turning with ... */
    APC_MOT_WATCH_TURN_B = 0x22,            /* the same turning with rotY down */
    APC_MOT_ALERT_END = 0x23,               /* APC_IDLE_ALERT_END and the end of APC_MOVE_WATCH_HUNTER */
    APC_MOT_ATTACK_B = 0x24,                /* APC_ATK_B and APC_ATK_B_LEAD: attack 2 at frame 20 (Popo: ... */
    APC_MOT_SPIN_ATTACK = 0x25,             /* APC_ATK_SPIN: turns 0x1C7 a frame */
    APC_MOT_REACT_RESET = 0x3C,             /* APC_REACT_RESET_HP only */
    APC_MOT_FLINCH_B = 0x3D,                /* APC_REACT_FLINCH_B, APC_REACT_FLINCH_RAND, ... */
    APC_MOT_FLINCH_A = 0x3E,                /* APC_REACT_FLINCH_A, APC_REACT_FLINCH_RAND, the stagger of ... */
    APC_MOT_FALL = 0x3F,                    /* APC_IDLE_STATUS2_COLLAPSE, APC_REACT_KNOCK_DOWN_B, ... */
    APC_MOT_LIE = 0x40,                     /* the lying of the knock-downs, of Die and of APC_EVENT_LIE */
    APC_MOT_DIE = 0x41,                     /* Die and APC_EVENT_DIE: frozen at frame 50 as a carcass */
    APC_MOT_GET_UP = 0x42,                  /* APC_IDLE_STATUS2_END and the knock-downs */
    APC_MOT_FALL_B = 0x43,                  /* the fall after the stagger of APC_REACT_KNOCK_DOWN_A and ... */
    APC_MOT_STATUS2_LIE = 0x44,             /* APC_IDLE_STATUS2_COLLAPSE: lying under status 2 with lying ... */
    APC_MOT_STATUS4 = 0x45                  /* APC_REACT_STATUS4: the status loop */
};

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
    APC_REACT_RESET_HP = 0x00,              /* one motion, then idle with hp 0x40 */
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

#endif
