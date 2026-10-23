#ifndef EM_MOSSWINE_H
#define EM_MOSSWINE_H
#include "em.h"

/*
 * The Mosswine family's vocabulary (EmMosswine_*):
 * Mosswine and Bullfango (EM_KIND_MOSSWINE, EM_KIND_BULLFANGO) share one
 * set of state functions and one numbering of motions and states. The
 * Felyne cart (EM_KIND_FELYNE_CART, EmFelyneCart_*) runs copies
 * of some of them.
 *
 * Motions: the kinds' motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion). A turn "right" raises rotY. Sides A and B
 * are the two sides a hit comes from (by the hit angle's half); which one
 * is the left is not settled.
 */
enum EmMosswineMotion {
    MOSS_MOT_IDLE = 0x01,               /* the standing loop; Init's row */
    MOSS_MOT_WALK = 0x02,
    MOSS_MOT_RUN = 0x03,
    MOSS_MOT_TURN_RIGHT = 0x04,
    MOSS_MOT_TURN_LEFT = 0x05,
    MOSS_MOT_IDLE_ACTION_B = 0x07,      /* held 240 frames */
    MOSS_MOT_WALK_B = 0x08,
    MOSS_MOT_IDLE_ACTION_A = 0x0A,      /* looped 30 to 89 frames */
    MOSS_MOT_RECOVER = 0x0B,            /* after status 2's get-up */
    MOSS_MOT_IDLE_ACTION_D = 0x0C,
    MOSS_MOT_IDLE_ACTION_A_END = 0x0D,
    MOSS_MOT_IDLE_ACTION_C = 0x0E,      /* repeated, turning to the goal */
    MOSS_MOT_CHARGE = 0x0F,             /* the charge, with a contact attack */
    MOSS_MOT_CHARGE_END = 0x10,
    MOSS_MOT_ALERT_BULLFANGO = 0x11,    /* the Bullfango's combat idle */
    MOSS_MOT_ALERT_MOSSWINE = 0x13,     /* the Mosswine's combat idle */
    MOSS_MOT_FLINCH_FRONT = 0x14,       /* a hit from the front */
    MOSS_MOT_FLINCH_A = 0x3C,
    MOSS_MOT_FLINCH_B = 0x3D,
    MOSS_MOT_BLOWN_A = 0x3E,            /* thrown away by a hit from side A */
    MOSS_MOT_BLOWN_LAND = 0x3F,         /* the landing, then lying */
    MOSS_MOT_DIE = 0x40,                /* the death after the landing */
    MOSS_MOT_BLOWN_GET_UP = 0x42,
    MOSS_MOT_BLOWN_B = 0x43,            /* thrown away by a hit from side B */
    MOSS_MOT_LIE = 0x46,                /* lying under status 2 */
    MOSS_MOT_DOWN = 0x47,               /* down under status 4 and the stun, the body turned half round */
    MOSS_MOT_DOWN_GET_UP = 0x48,        /* back up, turning back */
    MOSS_MOT_STATUS2_GET_UP = 0x49
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the effect helpers read the position from); the Mosswine's,
 * the Bullfango's and the Bulldrome's skeletons share the numbers this
 * code uses (em_bulldrome.h's BULL_BONE_* are the same bones). The names
 * say what the code shows a bone to be: the kinds' own hit shapes
 * (gEmBoneHits), the shapes of the charge's hits (EmContactAttack_Create,
 * EmContactAttack_HitShapes) and where an effect starts.
 *
 * Sounds: the family's code plays none of its own (no Em_PlaySe* call), so
 * there is no sound vocabulary here.
 */
enum EmMosswineBone {
    MOSS_BONE_ROOT = 0,     /* the skeleton's first bone: the stun stars circle above it (StunStars_CreateEm) */
    MOSS_BONE_BODY = 2,     /* the body's capsule (part 0) in the kinds' own hit shapes, the sphere of every charge's hit (EmContactAttack 3, 8 to 10, 14, 15) and the status-4 effect (Em_SpawnStatus4Effect) */
    MOSS_BONE_BREATH = 11   /* where the lying puffs start (MOSS_MOT_LIE) and the poison effect (Em_UpdateStatus) */
};

/* Group 0, EM_GROUP_IDLE (EmMosswine_RunIdleState). */
enum EmMosswineIdleState {
    MOSS_IDLE_NONE = 0x00,              /* an empty function */
    MOSS_IDLE = 0x01,                   /* EmMosswine_EndState's state; in combat the kind's alert idle */
    MOSS_IDLE_ACTION_A = 0x02,
    MOSS_IDLE_ACTION_B = 0x03,
    MOSS_IDLE_ACTION_C = 0x04,          /* MOSS_MOT_IDLE_ACTION_C once */
    MOSS_IDLE_ACTION_D = 0x05,
    MOSS_IDLE_ALERT_BULLFANGO = 0x06,
    MOSS_IDLE_ACTION_C_TWICE = 0x07,
    MOSS_IDLE_ACTION_C_4 = 0x08,        /* four times */
    MOSS_IDLE_RECOVER = 0x09,           /* after MOSS_IDLE_STATUS2_END */
    MOSS_IDLE_STATUS2 = 0x0A,           /* EM_DMG_STATUS2: lies until the status timer ends */
    MOSS_IDLE_STATUS2_END = 0x0B,
    MOSS_IDLE_ALERT_MOSSWINE = 0x0C,
    MOSS_IDLE_STUN_END = 0x0D           /* from MOSS_REACT_STUN: ends status 5 */
};

/* Group 1, EM_GROUP_MOVE (EmMosswine_RunMoveState). */
enum EmMosswineMoveState {
    MOSS_MOVE_TURN = 0x00,              /* turns to the target (EmMosswine_TurnParams[0]) */
    MOSS_MOVE_WALK = 0x01,              /* to the goal, within 200, for at most 1800 frames */
    MOSS_MOVE_RUN = 0x02,               /* to the goal, within 100 */
    MOSS_MOVE_WALK_B = 0x03,
    MOSS_MOVE_WALK_SCRIPTED = 0x04,     /* from a fixed start to a fixed goal by Em.variant (EmMosswine_ScriptedWalks) */
    MOSS_MOVE_WANDER_HOME = 0x05,       /* walks a while, turning toward its home (EmMosswineWork.home) */
    MOSS_MOVE_TURN_QUARTER = 0x06
};

/* Group 3, EM_GROUP_ATTACK (EmMosswine_RunAttackState): the charges, by their contact attack and length. */
enum EmMosswineAttackState {
    MOSS_ATK_CHARGE_A = 0x00,
    MOSS_ATK_CHARGE_B = 0x01,
    MOSS_ATK_CHARGE_C = 0x02,
    MOSS_ATK_CHARGE_D = 0x03,
    MOSS_ATK_CHARGE_E = 0x04,
    MOSS_ATK_CHARGE_F = 0x05
};

/* Group 4, EM_GROUP_REACT (EmMosswine_RunReactState): what EmMosswine_Update picks. */
enum EmMosswineReactState {
    MOSS_REACT_FLINCH = 0x00,
    MOSS_REACT_BLOWN_AWAY = 0x01,       /* thrown from the motion's frame 10 */
    MOSS_REACT_BLOWN_AWAY_QUICK = 0x02, /* thrown at once (in the air, or captured) */
    MOSS_REACT_STATUS4 = 0x03,
    MOSS_REACT_DOWN = 0x04,             /* as MOSS_REACT_STATUS4 without the status; EmMosswine_Update never picks it */
    MOSS_REACT_STUN = 0x05              /* status 5 (EM_STATUS_STUN), the Bullfango only */
};

/* Group 5, EM_GROUP_DEFEAT (EmMosswine_RunDefeatState). */
enum EmMosswineDefeatState {
    MOSS_DEFEAT_DIE = 0x00,             /* thrown from frame 10, then a carcass to carve */
    MOSS_DEFEAT_DIE_QUICK = 0x01,       /* thrown at once */
    MOSS_DEFEAT_REAPPEAR = 0x02
};

/* Group 6, EM_GROUP_EVENT (EmMosswine_RunEventState). */
enum EmMosswineEventState {
    MOSS_EVENT_HIDE_A = 0x00,
    MOSS_EVENT_HIDE_B = 0x01
};

extern void *EmMosswine_Callbacks[8];
void EmMosswine_Init(Em *em);
void EmMosswine_Start(Em *em);
void EmMosswine_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmMosswine_Status2Lie(Em *em);
void EmMosswine_Turn(Em *em, s32 quarterTurn);
void EmMosswine_Walk(Em *em);
void EmMosswine_Run(Em *em);
void EmMosswine_WalkB(Em *em);
void EmMosswine_WalkScripted(Em *em);
void EmMosswine_WanderHome(Em *em);
void EmMosswine_Charge(Em *em, u32 charge);
void EmMosswine_BlownAway(Em *em);
void EmMosswine_BlownAwayQuick(Em *em);
void EmMosswine_Die(Em *em);
void EmMosswine_DieQuick(Em *em);
void EmMosswine_Update(Em *em);
void EmMosswine_RunState(Em *em);

#endif /* EM_MOSSWINE_H */
