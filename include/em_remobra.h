#ifndef EM_REMOBRA_H
#define EM_REMOBRA_H
#include "em.h"

/*
 * The Remobra family (EM_KIND_REMOBRA): its motions and its states, by group
 * (em.h's EmStateGroup). The functions are EmRemobra_*. These flying serpents
 * spend most of their time in the air (stance EM_STANCE_AIR): they hover,
 * bank, swoop and glide to their goal, spit from the air
 * (EMRANGED_REMOBRA_SPIT) and strike, and fall to the ground when hit hard.
 * Once hunted, a quest's Remobra waits to appear again
 * (REMOBRA_DEFEAT_WAIT_APPEAR, then the reappearance in EM_GROUP_EVENT2). Each enum
 * lists the cases of the group's dispatcher (EmRemobra_Run*State); a name
 * ending in NONE is a number the dispatcher accepts but runs nothing for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmRemobraMotion {
    REMOBRA_MOT_FLY = 0x01,                 /* the flying loop (motion row 1) with the hops of EmRemobra_UpdateHop: HOVER, the turns, height changes, glides, RISE_CRY, RETURN_HOME, the shifts, EVENT2_REAPPEAR */
    REMOBRA_MOT_SWOOP_END = 0x03,           /* REMOBRA_FLY_SWOOP_END, the pull-up after a swoop */
    REMOBRA_MOT_SWOOP = 0x04,               /* REMOBRA_FLY_SWOOP, and the slowing end of SWOOP and SWOOP_SPIT (speed x 0.8) before REMOBRA_FLY_SWOOP_END */
    REMOBRA_MOT_BANK_SLOW_A = 0x06,         /* REMOBRA_FLY_BANK_SLOW_A */
    REMOBRA_MOT_BANK_SLOW_B = 0x07,         /* REMOBRA_FLY_BANK_SLOW_B */
    REMOBRA_MOT_BANK_FAST_A = 0x08,         /* REMOBRA_FLY_BANK_FAST_A */
    REMOBRA_MOT_BANK_FAST_B = 0x09,         /* REMOBRA_FLY_BANK_FAST_B */
    REMOBRA_MOT_BACK_OFF = 0x0A,            /* REMOBRA_FLY_BACK_OFF_A/_B: the leap up and back from frame 40 (_B higher and farther), then REMOBRA_FLY_HOVER */
    REMOBRA_MOT_TAKE_OFF = 0x0B,            /* REMOBRA_FLY_TAKE_OFF_* (airborne at frame 196) */
    REMOBRA_MOT_SPIT_DOWN = 0x0C,           /* REMOBRA_ATK_SPIT_DOWN and REMOBRA_FLY_SWOOP_SPIT: a steep spit at frame 24 (EMRANGED_REMOBRA_SPIT from REMOBRA_BONE_SPIT), replayed through the swoop */
    REMOBRA_MOT_SPIT_AIM = 0x0E,            /* REMOBRA_ATK_SPIT_A/B/C: held 40, 50 or 60 frames before the spit (REMOBRA_MOT_SPIT) */
    REMOBRA_MOT_SPIT = 0x0F,                /* REMOBRA_ATK_SPIT_A/B/C: the spit at frame 8 */
    REMOBRA_MOT_STRIKE = 0x10,              /* REMOBRA_ATK_STRIKE (RemobraAttack type 1 at frame 46) */
    REMOBRA_MOT_SWOOP_STRIKE = 0x11,        /* REMOBRA_ATK_SWOOP_STRIKE (curve kind 0, RemobraAttack types 2 and 3 at frames 104 and 168) */
    REMOBRA_MOT_FLINCH = 0x12,              /* REMOBRA_REACT_FLINCH */
    REMOBRA_MOT_FALL = 0x13,                /* REMOBRA_REACT_FALL and REMOBRA_DEFEAT_FALL: falling from the air to the floor, 10 a frame, 40 from frame 66; then REMOBRA_MOT_FALL_LAND */
    REMOBRA_MOT_FALL_LAND = 0x14,           /* hitting the ground at the end of REMOBRA_REACT_FALL and REMOBRA_DEFEAT_FALL (ground effect at 14), then REACT_DOWNED or DEFEAT_HUNTED */
    REMOBRA_MOT_DOWNED = 0x15,              /* REMOBRA_REACT_DOWNED: lying on the ground for 150 frames */
    REMOBRA_MOT_STATUS4 = 0x16,             /* REMOBRA_REACT_STATUS4, with the status-4 effect */
    REMOBRA_MOT_DEATH = 0x19,               /* REMOBRA_DEFEAT_HUNTED (carve point at frame 158) */
    REMOBRA_MOT_HOVER_CRY = 0x1A,           /* REMOBRA_FLY_RISE_CRY: a cry while hovering (voice at the start, sound 12 at bone 21 on frame 14), played to its end, then REMOBRA_FLY_HOVER */
    REMOBRA_MOT_IDLE_GROUND = 0x1B          /* REMOBRA_IDLE_GROUND: standing on the ground */
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound and effect helpers read the position from). The
 * names say what the family's code and its hit shapes show a bone to be:
 * the monster's own hit shapes (gEmBoneHits: each shape's bone and the part
 * it belongs to), the shapes of the hits RemobraAttack_Create makes on given
 * frames of a motion (RemobraAttack_HitShapes, by the attack rows' hit
 * lists), and where the spit starts. "A" and "B" are the two sides; which
 * is left the code doesn't show. Bones no code explains stay numbers: 21
 * (most of the family's sounds, the cry's among them, play there), 1 and 3
 * (the ground effects under the flying motions and the landings; 3 also
 * the landings' sounds), 25 and 29 (sounds of the take-offs).
 */
enum EmRemobraBone {
    REMOBRA_BONE_WING_A = 8,    /* the middle of one of part 1's two matching chains of spheres (bones 5 to 9, and 11 to 15 for REMOBRA_BONE_WING_B), the only paired limbs of this flier; in REMOBRA_MOT_FLY their two sounds play together three times a loop (72/76, 148/152, 222/224) */
    REMOBRA_BONE_WING_B = 14,
    REMOBRA_BONE_SPIT = 19,     /* where the spit starts (EMRANGED_REMOBRA_SPIT); part 0's sphere, with bone 17's, and the second sphere of the swoop strike's first hit (hit list 1) */
    REMOBRA_BONE_STRIKE = 32    /* the middle of the strike's three spheres (31, 32 and 33; hit list 0: REMOBRA_MOT_STRIKE at frame 46, the swoop strike's second hit at 168); a sound plays here two frames after the strike's hit starts */
};

/* Group 0, EM_GROUP_IDLE (EmRemobra_RunIdleState). */
enum EmRemobraIdleState {
    REMOBRA_IDLE_NONE = 0x00,               /* does nothing */
    REMOBRA_IDLE_GROUND = 0x01              /* stands on the ground, loops */
};

/* Group 1, EM_GROUP_MOVE (EmRemobra_RunMoveState). */
enum EmRemobraMoveState {
    REMOBRA_MOVE_NONE = 0x00                /* ends at once to REMOBRA_IDLE_GROUND */
};

/* Group 2, EM_GROUP_FLY (EmRemobra_RunFlyState). */
enum EmRemobraFlyState {
    REMOBRA_FLY_HOVER = 0x00,               /* flies in place, rising to 400 over the floor */
    REMOBRA_FLY_SWOOP_END = 0x01,           /* pull-up after a swoop */
    REMOBRA_FLY_SWOOP = 0x02,               /* swoop toward the goal */
    REMOBRA_FLY_BANK_SLOW_A = 0x03,         /* slow turning curve, one side, then swoop */
    REMOBRA_FLY_BANK_SLOW_B = 0x04,         /* slow turning curve, other side, then swoop */
    REMOBRA_FLY_BANK_FAST_A = 0x05,         /* fast turning curve, one side, then swoop */
    REMOBRA_FLY_BANK_FAST_B = 0x06,         /* fast turning curve, other side, then swoop */
    REMOBRA_FLY_BACK_OFF_A = 0x07,          /* leap up and back */
    REMOBRA_FLY_BACK_OFF_B = 0x08,          /* higher, farther leap up and back */
    REMOBRA_FLY_TAKE_OFF_START = 0x09,      /* take-off from frame 90 (EmRemobra_Start) */
    REMOBRA_FLY_TURN_TO_GOAL = 0x0A,        /* turn in the air to the goal */
    REMOBRA_FLY_TAKE_OFF = 0x0B,            /* take-off from the ground */
    REMOBRA_FLY_CHANGE_HEIGHT_RANDOM = 0x0C,/* climb or sink to a random height */
    REMOBRA_FLY_CHANGE_HEIGHT_GOAL = 0x0D,  /* climb or sink to the goal height */
    REMOBRA_FLY_TAKE_OFF_STATUS4 = 0x0E,    /* take-off ending status 4 */
    REMOBRA_FLY_GLIDE_TO_GOAL = 0x0F,       /* fly forward to the goal, stopping 300 short */
    REMOBRA_FLY_RISE_CRY = 0x10,            /* rise, then the hovering cry */
    REMOBRA_FLY_RETURN_HOME = 0x11,         /* fly back to the home position */
    REMOBRA_FLY_TURN_RANDOM = 0x12,         /* turn in the air by a random angle */
    REMOBRA_FLY_GLIDE_FIXED = 0x13,         /* fly straight 600 without steering */
    REMOBRA_FLY_SWOOP_SPIT = 0x14,          /* swoop toward the goal while spitting */
    REMOBRA_FLY_SHIFT_FORWARD = 0x15,       /* drift forward */
    REMOBRA_FLY_SHIFT_BACK = 0x16,          /* drift back */
    REMOBRA_FLY_SHIFT_SIDE_A = 0x17,        /* drift sideways (+X) */
    REMOBRA_FLY_SHIFT_SIDE_B = 0x18,        /* drift sideways (-X) */
    REMOBRA_FLY_SHIFT_BACK_UP = 0x19,       /* drift back and up */
    REMOBRA_FLY_RISE_HIGH = 0x1A            /* climb to 500 over the floor */
};

/* Group 3, EM_GROUP_ATTACK (EmRemobra_RunAttackState). */
enum EmRemobraAttackState {
    REMOBRA_ATK_STRIKE = 0x00,              /* RemobraAttack type 1 */
    REMOBRA_ATK_SPIT_A = 0x01,              /* aimed spit, 40-frame hold */
    REMOBRA_ATK_SPIT_B = 0x02,              /* aimed spit, 50-frame hold */
    REMOBRA_ATK_SPIT_C = 0x03,              /* aimed spit, 60-frame hold */
    REMOBRA_ATK_SPIT_DOWN = 0x04,           /* quick steep spit */
    REMOBRA_ATK_SWOOP_STRIKE = 0x05         /* curve-driven pass with RemobraAttack types 2 and 3 */
};

/* Group 4, EM_GROUP_REACT (EmRemobra_RunReactState). */
enum EmRemobraReactState {
    REMOBRA_REACT_FLINCH = 0x00,            /* flinch in the air */
    REMOBRA_REACT_FALL = 0x01,              /* fall to the ground */
    REMOBRA_REACT_DOWNED = 0x02,            /* lie on the ground, get up */
    REMOBRA_REACT_STATUS4 = 0x03            /* status 4 */
};

/* Group 5, EM_GROUP_DEFEAT (EmRemobra_RunDefeatState). */
enum EmRemobraDefeatState {
    REMOBRA_DEFEAT_HUNTED = 0x00,           /* death, carve point, fade out */
    REMOBRA_DEFEAT_WAIT_APPEAR = 0x01,      /* wait to appear again */
    REMOBRA_DEFEAT_FALL = 0x02              /* killed in the air: fall, then hunted */
};

/* Group 6, EM_GROUP_EVENT (EmRemobra_RunEventState). */
enum EmRemobraEventState {
    REMOBRA_EVENT_WAIT = 0x00               /* scripted scene, senses off */
};

/* Group 7, EM_GROUP_EVENT2 (EmRemobra_RunEvent2State). */
enum EmRemobraEvent2State {
    REMOBRA_EVENT2_REAPPEAR = 0x00          /* fly in after appearing again */
};

extern void *EmRemobra_Callbacks[8];
void EmRemobra_MotionSounds(Em *em, EmWork *work);
void EmRemobra_Init(Em *em);
void EmRemobra_Start(Em *em);
void EmRemobra_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmRemobra_UpdateHop(Em *em);
void EmRemobra_Hover(Em *em);
void EmRemobra_Swoop(Em *em, s32 variant);
void EmRemobra_Bank(Em *em, s32 fast, s32 side);
void EmRemobra_TakeOff(Em *em, s32 variant);
void EmRemobra_TurnInAir(Em *em);
void EmRemobra_ChangeHeight(Em *em, s32 variant);
void EmRemobra_Glide(Em *em, s32 variant);
void EmRemobra_Shift(Em *em, s32 variant);
void EmRemobra_SpitAimed(Em *em, s32 variant);
void EmRemobra_Hunted(Em *em);
void EmRemobra_WaitAppear(Em *em);
void EmRemobra_Update(Em *em);
void EmRemobra_RunState(Em *em);
void EmRemobra_SpawnGroundEffect(Em *em, s32 bone, s32 effectType, f32 scale);
void EmRemobra_MoveByVelocity(Em *em);
void EmRemobra_StartCurves(Em *em, u8 kind);
u8 EmRemobra_UpdateCurves(Em *em);
extern EmMotionRow gEmMotionRows_Remobra[101];

#endif
