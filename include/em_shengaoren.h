#ifndef EM_SHENGAOREN_H
#define EM_SHENGAOREN_H

/*
 * The Shen Gaoren family (EM_KIND_SHEN_GAOREN): its motions and its states,
 * by group (em.h's EmStateGroup). The functions are EmShenGaoren_*. The
 * monster crosses the fortress areas to the fort, standing tall on its legs
 * (SHEN_STANCE_TALL) or lowered (SHEN_STANCE_LOW), and shoots at, strikes and
 * smashes the fort, counting the hits in fortHits. Its four legs are parts 3
 * to 6 (LEG_A to LEG_D in the reaction names, by part order); flinching every
 * leg twice makes it collapse. It has no flying states. Each enum lists the
 * cases of the group's dispatcher (EmShenGaoren_Run*State).
 */

/*
 * The family's two stances (Em.stance), by em.h's EmStance numbers: Init
 * starts the monster low in AREA_BATTLE_TOWN_1 (SHEN_MOT_IDLE_LOW,
 * SHEN_IDLE_LOW) and tall elsewhere, and EmShenGaoren_EndState returns to
 * the idle of the stance it is in.
 */
#define SHEN_STANCE_LOW     EM_STANCE_GROUND
#define SHEN_STANCE_TALL    EM_STANCE_FAMILY

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmShenGaorenMotion {
    SHEN_MOT_IDLE_TALL = 0x01,              /* SHEN_IDLE_HOLD, SHEN_IDLE_TALL, SHEN_IDLE_TALL_BRIEF (on ... */
    SHEN_MOT_WALK_TALL = 0x02,              /* SHEN_MOVE_WALK_TALL, SHEN_EVENT_LEAVE: stomps with impacts ... */
    SHEN_MOT_WALK_BACK_TALL = 0x03,         /* SHEN_MOVE_BACK_TALL and both event states */
    SHEN_MOT_LOWER = 0x06,                  /* SHEN_IDLE_LOWER: each leg sets down with an impact */
    SHEN_MOT_STRIKE_A = 0x0A,               /* SHEN_ATK_STRIKE_A: ShenGaorenAttack 3, a ground effect at ... */
    SHEN_MOT_STRIKE_B = 0x0B,               /* SHEN_ATK_STRIKE_B: ShenGaorenAttack 4, a ground effect at ... */
    SHEN_MOT_TRAMPLE_TALL = 0x0C,           /* SHEN_ATK_TRAMPLE_TALL, SHEN_IDLE_TRAMPLE_BRIEF: ... */
    SHEN_MOT_AIM_TALL = 0x0D,               /* SHEN_ATK_SHOT_TALL, SHEN_EVENT_CUTSCENE_SHOT: held 300 ... */
    SHEN_MOT_FIRE_TALL = 0x0E,              /* the tall shot: EMFX_SHEN_SHOT at frame 4, muzzle effect */
    SHEN_MOT_SHOT_END_TALL = 0x0F,          /* after the tall shot */
    SHEN_MOT_SMASH = 0x10,                  /* SHEN_ATK_SMASH and SHEN_ATK_SMASH_FORT: ShenGaorenAttack 2 ... */
    SHEN_MOT_FLINCH_TALL = 0x15,            /* SHEN_REACT_FLINCH_TALL and the first tall leg flinches */
    SHEN_MOT_FALL_TALL = 0x16,              /* the fall of SHEN_REACT_KNOCKDOWN_TALL: ShenGaorenAttack ... */
    SHEN_MOT_DOWN_TALL = 0x17,              /* the 300 frames lying of SHEN_REACT_KNOCKDOWN_TALL */
    SHEN_MOT_GET_UP_TALL = 0x18,            /* the getting up of SHEN_REACT_KNOCKDOWN_TALL */
    SHEN_MOT_DIE_TALL = 0x19,               /* the first motion of SHEN_DEFEAT_TALL */
    SHEN_MOT_IDLE_LOW = 0x1E,               /* SHEN_IDLE_LOW, Init in AREA_BATTLE_TOWN_1 */
    SHEN_MOT_WALK_LOW = 0x1F,               /* SHEN_MOVE_WALK_LOW: stomps with impacts and shakes */
    SHEN_MOT_WALK_BACK_LOW = 0x20,          /* SHEN_MOVE_BACK_LOW */
    SHEN_MOT_RISE = 0x23,                   /* SHEN_IDLE_RISE_A: each leg pushes off with an impact */
    SHEN_MOT_STRIKE_FORT = 0x29,            /* SHEN_ATK_STRIKE_FORT: ShenGaorenAttack 1, ground effect ... */
    SHEN_MOT_TRAMPLE_LOW = 0x2A,            /* SHEN_ATK_TRAMPLE_LOW: ShenGaorenAttacks 0xC to 0x11, ... */
    SHEN_MOT_AIM_LOW = 0x2B,                /* the 300-frame aim of the low shots, charge effect */
    SHEN_MOT_FIRE_LOW = 0x2C,               /* the firing of the low shots, muzzle effect */
    SHEN_MOT_SHOT_END_LOW = 0x2D,           /* after the low shots */
    SHEN_MOT_FLINCH_SHOT_LOW = 0x31,        /* the first motion of SHEN_REACT_LEGS_COLLAPSE_SHOT */
    SHEN_MOT_FLINCH_LOW = 0x33,             /* SHEN_REACT_FLINCH_LOW */
    SHEN_MOT_COLLAPSE = 0x34,               /* SHEN_REACT_LEGS_COLLAPSE, _SHOT and ... */
    SHEN_MOT_DOWN_LOW = 0x35,               /* the 300 frames lying of the low collapse */
    SHEN_MOT_GET_UP_LOW = 0x36,             /* the getting up of the low collapse */
    SHEN_MOT_FLINCH_LEG_C = 0x37,           /* the low flinches of part 5 (ground effect at its leg) */
    SHEN_MOT_FLINCH_LEG_A = 0x38,           /* the low flinches of part 3 */
    SHEN_MOT_FLINCH_LEG_D = 0x39,           /* the low flinches of part 6 */
    SHEN_MOT_FLINCH_LEG_B = 0x3A,           /* the low flinches of part 4 */
    SHEN_MOT_DIE_LOW = 0x3B                 /* SHEN_DEFEAT_LOW, and the second motion of SHEN_DEFEAT_TALL: ... */
};

/* Group 0, EM_GROUP_IDLE (EmShenGaoren_RunIdleState). */
enum EmShenGaorenIdleState {
    SHEN_IDLE_HOLD = 0x00,                  /* holds the tall idle, never ends */
    SHEN_IDLE_TALL = 0x01,                  /* tall idle */
    SHEN_IDLE_LOW = 0x02,                   /* low idle */
    SHEN_IDLE_LOWER = 0x03,                 /* lowers from tall to low */
    SHEN_IDLE_RISE_A = 0x04,                /* rises to tall (SHEN_MOT_RISE) */
    SHEN_IDLE_RISE_B = 0x05,                /* rises to tall (SHEN_MOT_SHOT_END_TALL) */
    SHEN_IDLE_TALL_BRIEF = 0x06,            /* tall idle, 30 frames at most */
    SHEN_IDLE_TRAMPLE_BRIEF = 0x07          /* low, the trample motion for 30 frames at most */
};

/* Group 1, EM_GROUP_MOVE (EmShenGaoren_RunMoveState). */
enum EmShenGaorenMoveState {
    SHEN_MOVE_WALK_TALL = 0x00,             /* tall walk to the goal */
    SHEN_MOVE_BACK_TALL = 0x01,             /* tall walk backward to the goal */
    SHEN_MOVE_WALK_LOW = 0x02,              /* low walk to the goal */
    SHEN_MOVE_BACK_LOW = 0x03,              /* low walk backward to the goal */
    SHEN_MOVE_TURN_LOW = 0x04,              /* low turn to the target */
    SHEN_MOVE_TURN_TALL = 0x05,             /* tall turn to the target */
    SHEN_MOVE_TURN_AWAY_TALL = 0x06,        /* tall turn away from the target */
    SHEN_MOVE_TURN_AWAY_LOW = 0x07          /* low turn away from the target */
};

/* Group 3, EM_GROUP_ATTACK (EmShenGaoren_RunAttackState). */
enum EmShenGaorenAttackState {
    SHEN_ATK_SHOT_TALL = 0x00,              /* tall aimed shot in its own area */
    SHEN_ATK_SHOT_FORT = 0x01,              /* low shot landing in AREA_FORTRESS_5, counts a fort hit */
    SHEN_ATK_STRIKE_FORT = 0x02,            /* low strike counting a fort hit */
    SHEN_ATK_SMASH = 0x03,                  /* tall smash raising emEventFlags bit 0 */
    SHEN_ATK_STRIKE_A = 0x04,               /* tall strike A */
    SHEN_ATK_STRIKE_B = 0x05,               /* tall strike B */
    SHEN_ATK_TRAMPLE_TALL = 0x06,           /* tall trample */
    SHEN_ATK_TRAMPLE_LOW = 0x07,            /* low trample */
    SHEN_ATK_SMASH_FORT = 0x08,             /* tall smash counting a fort hit */
    SHEN_ATK_SHOT_TOWN_A = 0x09,            /* low shot landing in AREA_BATTLE_TOWN_2 */
    SHEN_ATK_SHOT_TOWN_B = 0x0A             /* low shot landing in AREA_BATTLE_TOWN_3 */
};

/* Group 4, EM_GROUP_REACT (EmShenGaoren_RunReactState). */
enum EmShenGaorenReactState {
    SHEN_REACT_FLINCH_TALL = 0x00,          /* tall flinch (body parts) */
    SHEN_REACT_FLINCH_LOW = 0x01,           /* low flinch (body parts) */
    SHEN_REACT_LEG_A_LOW = 0x02,            /* first low flinch of part 3 */
    SHEN_REACT_LEG_B_LOW = 0x03,            /* first low flinch of part 4 */
    SHEN_REACT_LEG_C_LOW = 0x04,            /* first low flinch of part 5 */
    SHEN_REACT_LEG_D_LOW = 0x05,            /* first low flinch of part 6 */
    SHEN_REACT_LEGS_COLLAPSE = 0x06,        /* all legs flinched twice: falls, lies, breaks part 1 */
    SHEN_REACT_KNOCKDOWN_TALL = 0x07,       /* tall fall, lie and get up */
    SHEN_REACT_LEGS_COLLAPSE_SHOT = 0x08,   /* all legs flinched twice during SHEN_ATK_SHOT_FORT: short ... */
    SHEN_REACT_LEG_A_TALL = 0x09,           /* first tall flinch of part 3 */
    SHEN_REACT_LEG_B_TALL = 0x0A,           /* first tall flinch of part 4 */
    SHEN_REACT_LEG_C_TALL = 0x0B,           /* first tall flinch of part 5 */
    SHEN_REACT_LEG_D_TALL = 0x0C,           /* first tall flinch of part 6 */
    SHEN_REACT_LEG_A_DOWN_TALL = 0x0D,      /* later tall flinch of part 3: knockdown */
    SHEN_REACT_LEG_B_DOWN_TALL = 0x0E,      /* later tall flinch of part 4: knockdown */
    SHEN_REACT_LEG_C_DOWN_TALL = 0x0F,      /* later tall flinch of part 5: knockdown */
    SHEN_REACT_LEG_D_DOWN_TALL = 0x10,      /* later tall flinch of part 6: knockdown */
    SHEN_REACT_LEG_A_AGAIN_LOW = 0x11,      /* second low flinch of part 3 */
    SHEN_REACT_LEG_B_AGAIN_LOW = 0x12,      /* second low flinch of part 4 */
    SHEN_REACT_LEG_C_AGAIN_LOW = 0x13,      /* second low flinch of part 5 */
    SHEN_REACT_LEG_D_AGAIN_LOW = 0x14,      /* second low flinch of part 6 */
    SHEN_REACT_KNOCKDOWN_LOW = 0x15         /* low fall, lie and get up (EM_DMG_ATTACK9), no part break */
};

/* Group 5, EM_GROUP_DEFEAT (EmShenGaoren_RunDefeatState). */
enum EmShenGaorenDefeatState {
    SHEN_DEFEAT_TALL = 0x00,                /* dies from tall, becomes the carcass */
    SHEN_DEFEAT_LOW = 0x01                  /* dies from low, becomes the carcass */
};

/* Group 6, EM_GROUP_EVENT (EmShenGaoren_RunEventState). */
enum EmShenGaorenEventState {
    SHEN_EVENT_CUTSCENE_SHOT = 0x00,        /* the scene event of AREA_FORTRESS_1: scripted walk and shot */
    SHEN_EVENT_LEAVE = 0x01                 /* AREA_FORTRESS_5 once the quest goal is met: walks off */
};

#endif
