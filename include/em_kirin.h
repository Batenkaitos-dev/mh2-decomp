#ifndef EM_KIRIN_H
#define EM_KIRIN_H

/*
 * The Kirin's vocabulary (the code in em_kirin.c, EM_KIND_KIRIN).
 *
 * Motions: row N of the family's motion table plays motion N, so one number
 * serves Em_SetMotion's row and the motion id EM_MOTION_ID(layer, motion)
 * kept in Em.motionId. The names say what the states that play a motion
 * show; motions no state plays stay numbers. A turn to the LEFT raises rotY.
 *
 * The Kirin doesn't fly: group 2 (EM_GROUP_FLY) runs the idle dispatcher and
 * group 7 (EM_GROUP_EVENT2) the event one. The lightning attacks
 * (KIRIN_ATK_*BOLTS*) pick their strike spots and attack row by the state:
 * the _ALT states strike the same spots with other rows.
 */
enum EmKirinMotion {
    KIRIN_MOT_IDLE = 0x01,              /* KIRIN_IDLE, motion row of Init, KIRIN_IDLE_THREAT_THEN_IDLE, the scene */
    KIRIN_MOT_IDLE_ACTION_A = 0x02,     /* KIRIN_IDLE_ACTION_A only; seven head sounds */
    KIRIN_MOT_IDLE_ACTION_B = 0x03,     /* KIRIN_IDLE_ACTION_B; the scene twice */
    KIRIN_MOT_IDLE_ACTION_C = 0x04,     /* KIRIN_IDLE_ACTION_C; the scene on reaching its goal */
    KIRIN_MOT_IDLE_ACTION_D = 0x05,     /* KIRIN_IDLE_ACTION_D; the scene before the threat; random voice */
    KIRIN_MOT_IDLE_ACTION_E = 0x06,     /* KIRIN_IDLE_ACTION_E only */
    KIRIN_MOT_TURN_TO_GOAL_LEFT = 0x07, /* KIRIN_IDLE_TURN_TO_GOAL, goal on the rotY-rising side */
    KIRIN_MOT_TURN_TO_GOAL_RIGHT = 0x08, /* KIRIN_IDLE_TURN_TO_GOAL, goal on the falling side */
    KIRIN_MOT_WALK = 0x09,              /* KIRIN_MOVE_WALK, WALK_TURN_TO_GOAL, the walks of the scene; footsteps */
    KIRIN_MOT_CHARGE = 0x0A,            /* KIRIN_MOVE_CHARGE(_WIDE) with contact attack 0x10; dust at f4 and f28 */
    KIRIN_MOT_GALLOP = 0x0B,            /* KIRIN_MOVE_GALLOP(_WIDE), restarted by hand on layers 0 and 1; dust */
    KIRIN_MOT_STRIDE_A = 0x0C,          /* KIRIN_MOVE_STRIDE_A, STRIDE_RAMPAGE; dust at f12 */
    KIRIN_MOT_STRIDE_B = 0x0D,          /* KIRIN_MOVE_STRIDE_B, STRIDE_RAMPAGE; same sounds and dust as STRIDE_A */
    KIRIN_MOT_STOP = 0x0E,              /* after charge, gallop (layers 0 and 1), strides, lunge, jump landing; dust at f12/f16 */
    KIRIN_MOT_TURN_AROUND_LEFT = 0x0F,  /* KIRIN_MOVE_TURN_AROUND_LEFT and FACE_GOAL, rotY rising 0x888 a frame */
    KIRIN_MOT_LUNGE = 0x11,             /* KIRIN_ATK_LUNGE; contact KIRIN_ATK_BOLTS_B_ALT at f20, dust at f24, then STOP */
    KIRIN_MOT_HOP = 0x12,               /* KIRIN_MOVE_HOP, FACE_GOAL (up to 0x4000 of turn to f26), the scene; dust at f26 */
    KIRIN_MOT_JUMP = 0x14,              /* KIRIN_MOVE_JUMP_TO_GOAL (frozen at f36 in the air), KIRIN_ATK_JUMP_STRIKE */
    KIRIN_MOT_TURN_AROUND_RIGHT = 0x16, /* KIRIN_MOVE_TURN_AROUND_RIGHT and FACE_GOAL, rotY falling; sounds as 0x0F */
    KIRIN_MOT_STRIKE = 0x18,            /* KIRIN_ATK_STRIKE_BOLTS (lightning at f22), KIRIN_ATK_JUMP_STRIKE; contact KIRIN_ATK_BOLTS_C_ALT at f14 */
    KIRIN_MOT_THREAT = 0x26,            /* KIRIN_IDLE_THREAT(_THEN_IDLE), KIRIN_ATK_THREAT, the scene before the lightning */
    KIRIN_MOT_CALL_LIGHTNING = 0x27,    /* ATK_BOLTS_*, spark type 8 at start, lightning at f90 */
    KIRIN_MOT_FLINCH_A = 0x3C,          /* KIRIN_REACT_FLINCH_A */
    KIRIN_MOT_FLINCH_B = 0x3D,          /* KIRIN_REACT_FLINCH_B; sounds as FLINCH_A */
    KIRIN_MOT_KNOCKBACK = 0x3E,         /* KIRIN_REACT_KNOCKBACK, WAKE_KNOCKBACK, KIRIN_DEFEAT_HUNTED: thrown back airborne */
    KIRIN_MOT_KNOCKBACK_LAND = 0x3F,    /* landing after KNOCKBACK (lies 90 frames; the hunted carcass pose); dust at f44 */
    KIRIN_MOT_GET_UP = 0x40,            /* KIRIN_IDLE_STATUS2_GET_UP, end of KIRIN_REACT_KNOCKBACK, KIRIN_REACT_COLLAPSE_AND_RISE */
    KIRIN_MOT_COLLAPSE = 0x41,          /* start of KIRIN_IDLE_STATUS2_COLLAPSE, KIRIN_REACT_COLLAPSE_AND_RISE */
    KIRIN_MOT_STATUS2_LIE = 0x43,       /* lying under status 2 until the timer; lying puffs at bone 0x17 */
    KIRIN_MOT_STATUS4 = 0x44,           /* KIRIN_REACT_STATUS4 with the status 4 effect */
    KIRIN_MOT_STATUS4_SHAKE_OFF = 0x45  /* KIRIN_IDLE_STATUS4_SHAKE_OFF; random voice */
};

/* Group 0 (and 2), EM_GROUP_IDLE (EmKirin_RunIdleState): standing, the idle actions, threats, and the statuses. */
enum EmKirinIdleState {
    KIRIN_IDLE_NOP = 0x00,              /* empty function */
    KIRIN_IDLE = 0x01,                  /* idle motion once; Init and EmKirin_EndState go here */
    KIRIN_IDLE_ACTION_A = 0x02,         /* KIRIN_MOT_IDLE_ACTION_A once */
    KIRIN_IDLE_ACTION_B = 0x03,         /* KIRIN_MOT_IDLE_ACTION_B once */
    KIRIN_IDLE_ACTION_C = 0x04,         /* KIRIN_MOT_IDLE_ACTION_C once */
    KIRIN_IDLE_ACTION_D = 0x05,         /* KIRIN_MOT_IDLE_ACTION_D once */
    KIRIN_IDLE_ACTION_E = 0x06,         /* KIRIN_MOT_IDLE_ACTION_E once */
    KIRIN_IDLE_TURN_TO_GOAL = 0x07,     /* turn motion picked by the side of the goal */
    KIRIN_IDLE_STATUS2_COLLAPSE = 0x08, /* Em_BeginStatus2; EM_DMG_STATUS2; collapse, lie until the timer */
    KIRIN_IDLE_STATUS2_GET_UP = 0x09,   /* Em_EndStatus2; KIRIN_MOT_GET_UP */
    KIRIN_IDLE_STATUS4_SHAKE_OFF = 0x0A, /* Em_EndStatus4; from KIRIN_REACT_STATUS4 */
    KIRIN_IDLE_THREAT = 0x0B,           /* KIRIN_MOT_THREAT once */
    KIRIN_IDLE_THREAT_THEN_IDLE = 0x0C  /* KIRIN_MOT_THREAT then KIRIN_MOT_IDLE */
};

/* Group 1, EM_GROUP_MOVE (EmKirin_RunMoveState): walking, charging, galloping, strides, turns and jumps. */
enum EmKirinMoveState {
    KIRIN_MOVE_WALK_TURN_TO_GOAL = 0x00, /* walk motion, rotY 0x100 a frame to the goal */
    KIRIN_MOVE_WALK = 0x01,             /* walk, Em_StepToGoal, stop margin 100 */
    KIRIN_MOVE_CHARGE = 0x02,           /* KIRIN_MOT_CHARGE + contact attack 0x10, stop margin 100 */
    KIRIN_MOVE_GALLOP = 0x03,           /* looped KIRIN_MOT_GALLOP + contact attack 0x10, stop margin 100 */
    KIRIN_MOVE_STRIDE_A = 0x04,         /* KIRIN_MOT_STRIDE_A then stop */
    KIRIN_MOVE_STRIDE_B = 0x05,         /* KIRIN_MOT_STRIDE_B then stop */
    KIRIN_MOVE_TURN_AROUND_LEFT = 0x06, /* half turn, rotY rising */
    KIRIN_MOVE_TURN_AROUND_RIGHT = 0x07, /* half turn, rotY falling */
    KIRIN_MOVE_HOP = 0x08,              /* KIRIN_MOT_HOP once */
    KIRIN_MOVE_CHARGE_WIDE = 0x09,      /* as CHARGE, stop margin 300 */
    KIRIN_MOVE_GALLOP_WIDE = 0x0A,      /* as GALLOP, stop margin 300, goalDist cut by 300 */
    KIRIN_MOVE_STRIDE_RAMPAGE = 0x0B,   /* four strides, contact KIRIN_ATK_BOLTS_D_ALT each */
    KIRIN_MOVE_FACE_GOAL = 0x0C,        /* hop or turn-around, rotY snapped to the goal */
    KIRIN_MOVE_JUMP_TO_GOAL = 0x0D      /* KIRIN_MOT_JUMP ballistic arc to the goal */
};

/* Group 3, EM_GROUP_ATTACK (EmKirin_RunAttackState): the threat, the lunge, the strikes and the lightning. */
enum EmKirinAttackState {
    KIRIN_ATK_THREAT = 0x00,          /* KIRIN_MOT_THREAT once */
    KIRIN_ATK_LUNGE = 0x01,           /* KIRIN_MOT_LUNGE (contact KIRIN_ATK_BOLTS_B_ALT), stop */
    KIRIN_ATK_JUMP_STRIKE = 0x02,     /* jump then KIRIN_MOT_STRIKE (contact KIRIN_ATK_BOLTS_C_ALT) */
    KIRIN_ATK_STRIKE_BOLTS = 0x03,    /* KIRIN_MOT_STRIKE, lightning variant 0 at f22 */
    KIRIN_ATK_BOLTS_A = 0x04,         /* KIRIN_MOT_CALL_LIGHTNING, lightning variant 1 at f90 */
    KIRIN_ATK_BOLTS_B = 0x05,         /* lightning variant 2 */
    KIRIN_ATK_BOLTS_C = 0x06,         /* lightning variant 3 */
    KIRIN_ATK_BOLTS_D = 0x07,         /* lightning variant 4 (also the one of the scene) */
    KIRIN_ATK_BOLTS_E = 0x08,         /* lightning variant 5 */
    KIRIN_ATK_STRIKE_BOLTS_ALT = 0x09, /* variant 6: the spots of STRIKE_BOLTS, attack row 6 */
    KIRIN_ATK_BOLTS_A_ALT = 0x0A,     /* variant 7: the spots of BOLTS_A, attack row 7 */
    KIRIN_ATK_BOLTS_B_ALT = 0x0B,     /* variant 8 */
    KIRIN_ATK_BOLTS_C_ALT = 0x0C,     /* variant 9 */
    KIRIN_ATK_BOLTS_D_ALT = 0x0D,     /* variant 0xA */
    KIRIN_ATK_BOLTS_E_ALT = 0x0E      /* variant 0xB */
};

/* Group 4, EM_GROUP_REACT (EmKirin_RunReactState): what EmKirin_Update picks from Em_UpdateDamage's result. */
enum EmKirinReactState {
    KIRIN_REACT_FLINCH_A = 0x00,         /* front hit, hitAngle - rotY in 0x4000..0x7FFF; KIRIN_MOT_FLINCH_A */
    KIRIN_REACT_FLINCH_B = 0x01,         /* front hit, hitAngle - rotY in 0x8000..0xBFFF; KIRIN_MOT_FLINCH_B */
    KIRIN_REACT_KNOCKBACK = 0x02,        /* hit from behind, part 0 broken or not grounded */
    KIRIN_REACT_STATUS4 = 0x03,          /* EM_DMG_STATUS4, Em_BeginStatus4 */
    KIRIN_REACT_COLLAPSE_AND_RISE = 0x04, /* collapse then get up; set by no code here */
    KIRIN_REACT_WAKE_KNOCKBACK = 0x05    /* EM_DMG_HIT_LYING, Em_EndStatus2, blind cleared, knockback */
};

/* Group 5, EM_GROUP_DEFEAT (EmKirin_RunDefeatState): hunted. */
enum EmKirinDefeatState {
    KIRIN_DEFEAT_HUNTED = 0x00 /* QuestEm_Hunted, carcass point */
};

/* Group 6 (and 7), EM_GROUP_EVENT (EmKirin_RunEventState): its scene in the snowy mountains. */
enum EmKirinEventState {
    KIRIN_EVENT_SNOWY_MOUNTAINS_SCENE = 0x00 /* Update starts it in quests QUEST_ID_KIRIN_SCENE_1 and QUEST_ID_KIRIN_SCENE_2, in AREA_SNOWY_MOUNTAINS_8 */
};

#endif
