#ifndef EM_ANTEKA_H
#define EM_ANTEKA_H

/*
 * The Anteka family's vocabulary (EmAnteka_*): the
 * Anteka (EM_KIND_ANTEKA) and kind 3, likely the Kelbi (EM_KIND_KELBI: the
 * name is inferred, the code doesn't show which monster it is).
 * Both run the same functions. Only the Kelbi has Em.gender: a smaller body,
 * a second hp row, and a layer-1 copy of some motions (EmAnteka_SetMotion).
 *
 * Motions: the kinds' motion table has row N play motion N, so one number
 * serves Em_SetMotion's row and EM_MOTION_ID(layer, motion). A turn "right"
 * raises rotY (the TurnParams naming).
 */
enum EmAntekaMotion {
    ANTEKA_MOT_IDLE = 0x01,             /* the standing loop; Init's row */
    ANTEKA_MOT_IDLE_ACTION_A = 0x02,    /* ANTEKA_IDLE_ACTION_A; the first motion of ANTEKA_EVENT_SCENE */
    ANTEKA_MOT_IDLE_ACTION_B = 0x03,
    ANTEKA_MOT_IDLE_ACTION_C = 0x04,
    ANTEKA_MOT_IDLE_ACTION_D = 0x05,
    ANTEKA_MOT_IDLE_ACTION_E = 0x06,
    ANTEKA_MOT_TURN_RIGHT = 0x07,       /* a quarter turn, the goal less than half a turn away */
    ANTEKA_MOT_TURN_LEFT = 0x08,        /* its mirror */
    ANTEKA_MOT_WALK = 0x09,             /* Em_StepToGoal walking */
    ANTEKA_MOT_RUN = 0x0A,
    ANTEKA_MOT_CHARGE = 0x0B,           /* the run with EM_KIND_ANTEKA's contact attack at frame 20 */
    ANTEKA_MOT_MOVE_ACTION_A = 0x0C,
    ANTEKA_MOT_MOVE_ACTION_B = 0x0D,
    ANTEKA_MOT_RECOVER = 0x0E,          /* back to standing after a run, an action, a leap or an attack */
    ANTEKA_MOT_TURN_AROUND_RIGHT = 0x0F, /* a half turn, 30 frames */
    ANTEKA_MOT_ATK_COMBO = 0x10,        /* three contact attacks (EmAnteka_MotionEffects) */
    ANTEKA_MOT_ATK_SLAM = 0x11,         /* one contact attack and a ground impact */
    ANTEKA_MOT_TURN_QUICK = 0x12,       /* a quarter turn in 26 frames; the Kelbi's small turn */
    ANTEKA_MOT_LEAP = 0x14,             /* airborne from frame 26 (ANTEKA_MOVE_LEAP) */
    ANTEKA_MOT_TURN_AROUND_LEFT = 0x16,
    ANTEKA_MOT_ATK_STRIKE = 0x18,       /* one contact attack */
    ANTEKA_MOT_IDLE_ACTION_F = 0x26,
    ANTEKA_MOT_FLINCH_A = 0x3C,
    ANTEKA_MOT_FLINCH_B = 0x3D,
    ANTEKA_MOT_BLOWN_BACK = 0x3E,       /* thrown up and backward (stance EM_STANCE_AIR) */
    ANTEKA_MOT_BLOWN_BACK_LAND = 0x3F,  /* the landing; held as the carcass's pose */
    ANTEKA_MOT_GET_UP = 0x40,
    ANTEKA_MOT_COLLAPSE = 0x41,
    ANTEKA_MOT_LIE = 0x43,              /* lying under status 2 */
    ANTEKA_MOT_STATUS4 = 0x44,
    ANTEKA_MOT_STATUS4_END = 0x45
};

/* Group 0, EM_GROUP_IDLE (EmAnteka_RunIdleState; also run for group 2). */
enum EmAntekaIdleState {
    ANTEKA_IDLE_NONE = 0x00,            /* an empty function */
    ANTEKA_IDLE = 0x01,                 /* EmAnteka_EndState's state: the script picks the next */
    ANTEKA_IDLE_ACTION_A = 0x02,
    ANTEKA_IDLE_ACTION_B = 0x03,
    ANTEKA_IDLE_ACTION_C = 0x04,
    ANTEKA_IDLE_ACTION_D = 0x05,
    ANTEKA_IDLE_ACTION_E = 0x06,
    ANTEKA_IDLE_FACE_GOAL = 0x07,       /* the turn motion toward the goal's side; rotY unchanged */
    ANTEKA_IDLE_STATUS2 = 0x08,         /* EM_DMG_STATUS2: collapses and lies until the status timer ends */
    ANTEKA_IDLE_STATUS2_END = 0x09,
    ANTEKA_IDLE_STATUS4_END = 0x0A,     /* from ANTEKA_REACT_STATUS4 when its timer ends */
    ANTEKA_IDLE_ACTION_F = 0x0B
};

/* Group 1, EM_GROUP_MOVE (EmAnteka_RunMoveState). */
enum EmAntekaMoveState {
    ANTEKA_MOVE_WALK_TURN = 0x00,       /* walks while turning to the goal's angle */
    ANTEKA_MOVE_WALK = 0x01,
    ANTEKA_MOVE_RUN = 0x02,             /* stops 100 short of the goal (EmAnteka_ChangeState) */
    ANTEKA_MOVE_CHARGE = 0x03,
    ANTEKA_MOVE_ACTION_A = 0x04,
    ANTEKA_MOVE_ACTION_B = 0x05,
    ANTEKA_MOVE_TURN_AROUND_RIGHT = 0x06,
    ANTEKA_MOVE_TURN_AROUND_LEFT = 0x07,
    ANTEKA_MOVE_TURN_QUICK = 0x08,
    ANTEKA_MOVE_RUN_FAR = 0x09,         /* ANTEKA_MOVE_RUN stopping 300 short */
    ANTEKA_MOVE_CHARGE_FAR = 0x0A,      /* ANTEKA_MOVE_CHARGE stopping 300 short */
    ANTEKA_MOVE_ACTION_SERIES = 0x0B,   /* actions A, B, A, B */
    ANTEKA_MOVE_TURN_TO_GOAL = 0x0C,
    ANTEKA_MOVE_LEAP = 0x0D
};

/* Group 3, EM_GROUP_ATTACK (EmAnteka_RunAttackState): picked by the script only. */
enum EmAntekaAttackState {
    ANTEKA_ATK_COMBO = 0x00,
    ANTEKA_ATK_SLAM = 0x01,
    ANTEKA_ATK_STRIKE = 0x02,           /* the leap's motion in place, then the strike */
    ANTEKA_ATK_NONE_A = 0x03,           /* 0x03-0x08 end at once */
    ANTEKA_ATK_NONE_B = 0x04,
    ANTEKA_ATK_NONE_C = 0x05,
    ANTEKA_ATK_NONE_D = 0x06,
    ANTEKA_ATK_NONE_E = 0x07,
    ANTEKA_ATK_NONE_F = 0x08
};

/* Group 4, EM_GROUP_REACT (EmAnteka_RunReactState): what EmAnteka_Update picks. */
enum EmAntekaReactState {
    ANTEKA_REACT_FLINCH_A = 0x00,       /* a hit, (hitAngle - rotY) below half a turn */
    ANTEKA_REACT_FLINCH_B = 0x01,       /* the other half */
    ANTEKA_REACT_BLOWN_BACK = 0x02,     /* EM_DMG_PART_FLINCH or EM_DMG_HIT_MODE3 when no flinch applies */
    ANTEKA_REACT_STATUS4 = 0x03,        /* EM_DMG_STATUS4 */
    ANTEKA_REACT_COLLAPSE = 0x04,       /* collapses and gets up; Update never picks it */
    ANTEKA_REACT_BLOWN_BACK_LYING = 0x05 /* EM_DMG_HIT_LYING: ANTEKA_REACT_BLOWN_BACK ending status 2 */
};

/* Group 5, EM_GROUP_DEFEAT (EmAnteka_RunDefeatState). */
enum EmAntekaDefeatState {
    ANTEKA_DEFEAT_DIE = 0x00,           /* EM_DMG_KILLED, EM_DMG_DEAD: thrown back, a carcass, faded out */
    ANTEKA_DEFEAT_REAPPEAR = 0x04,
    ANTEKA_DEFEAT_LEAVE = 0x05          /* a 100-frame fade, then QuestEm_Leave */
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmAnteka_RunEventState). */
enum EmAntekaEventState {
    ANTEKA_EVENT_LEAVE = 0x00,          /* a 60-frame fade, then QuestEm_Leave */
    ANTEKA_EVENT_SCENE = 0x01,          /* at a fixed spot, three motions */
    ANTEKA_EVENT_IDLE = 0x02
};

#endif
