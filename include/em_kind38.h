#ifndef EM_KIND38_H
#define EM_KIND38_H
#include "em.h"

/*
 * Kind 56's vocabulary (EmKind38_*, the kind in hex): a small hovering
 * monster with one contact attack (the charge). Nothing in the code names
 * the monster. It is placed by the quest or spawned by Yama Tsukami
 * (EmKind38_SpawnAtEmBone; then it bursts out, stays 1800 frames and dies).
 * Its C sets only the idle motion; the script plays the others. Capcom's
 * name for kind 56 is EM_RAIKOUCHUU, the Thunderbug (mhfo.dll's kind
 * table), hence EM_KIND_THUNDERBUG.
 */
enum EmKind38Motion {
    K38_MOT_IDLE = 0x01
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones). The
 * code uses one: bone 2 is its body.
 *
 * Sounds: a sound number indexes the kind's own sound bank
 * (Em_PlaySeAtBoneVol, at K38_BONE_BODY). Each plays on the frame of the
 * thing it goes with, or loops while a state runs (EmKind38_UpdateLoopSe,
 * by EmKind38Work.seKind).
 */
enum EmKind38Bone {
    K38_BONE_BODY = 2       /* its one hit sphere (part 1, radius 40), its charge's sphere (BulldromeAttack, which it borrows), where its sounds play and where the self-destruct explodes */
};
enum EmKind38Se {
    K38_SE_FLY_LOOP = 0,    /* every 10 frames while it flies, hovers, sidesteps or bursts out (seKind 1) */
    K38_SE_CHARGE_LOOP = 1, /* every 11 frames while it charges or flies at the target to explode (seKind 2) */
    K38_SE_BURST_OUT = 2,   /* on K38_FLY_BURST_OUT's first frame, as it is flung out */
    K38_SE_VANISH = 3,      /* when K38_DEFEAT_DIE's 60-frame hold ends and the fade starts */
    K38_SE_ICE_KILL = 4,    /* on the frame an ice hit kills it (EmKind38_Update) */
    K38_SE_FLINCH = 5       /* on the frame a hit makes it flinch (EmKind38_Update) */
};

/* Group 0, EM_GROUP_IDLE (EmKind38_RunIdleState). */
enum EmKind38IdleState {
    K38_IDLE_NONE = 0x00,               /* an empty state */
    K38_IDLE = 0x01                     /* waits 30-75 frames on the ground, then asks the script */
};

/* Group 1, EM_GROUP_MOVE (EmKind38_RunMoveState). */
enum EmKind38MoveState {
    K38_MOVE_NONE = 0x00
};

/* Group 2, EM_GROUP_FLY (EmKind38_RunFlyState). */
enum EmKind38FlyState {
    K38_FLY_FORWARD_SHORT = 0x00,       /* at most 800 toward the goal */
    K38_FLY_BACKWARD = 0x01,            /* backs 800 away, facing the goal */
    K38_FLY_FORWARD_LONG = 0x02,        /* at most 1200, faster */
    K38_FLY_HOVER = 0x03,               /* the idle in the air */
    K38_FLY_CLIMB = 0x04,
    K38_FLY_DESCEND = 0x05,             /* lands if it touches the floor */
    K38_FLY_SIDESTEP_A = 0x06,          /* heading 14336 off the goal's direction, one way */
    K38_FLY_SIDESTEP_B = 0x07,          /* the other way */
    K38_FLY_DRIFT = 0x08,               /* along +x, -x, +z or -z (stateRand) */
    K38_FLY_NONE = 0x09,
    K38_FLY_BURST_OUT = 0x0A            /* flung out on being spawned, growing to its size */
};

/* Group 3, EM_GROUP_ATTACK (EmKind38_RunAttackState). */
enum EmKind38AttackState {
    K38_ATK_CHARGE_SHORT = 0x00,
    K38_ATK_CHARGE_LONG = 0x01,
    K38_ATK_SELF_DESTRUCT = 0x02        /* flies at the target and explodes */
};

/* Group 4, EM_GROUP_REACT (EmKind38_RunReactState). */
enum EmKind38ReactState {
    K38_REACT_FLINCH_AIR = 0x00,
    K38_REACT_STATUS_FLINCH = 0x01,     /* the same function, for a status in the air */
    K38_REACT_NONE = 0x02,
    K38_REACT_FLINCH_GROUND = 0x03
};

/* Group 5, EM_GROUP_DEFEAT (EmKind38_RunDefeatState). */
enum EmKind38DefeatState {
    K38_DEFEAT_DIE = 0x00,              /* killed, hit by ice, or out of time */
    K38_DEFEAT_VANISH = 0x01,           /* after the self-destruct: no hold, no fade */
    K38_DEFEAT_REAPPEAR = 0x02
};

/* Group 6, EM_GROUP_EVENT (EmKind38_RunEventState). */
enum EmKind38EventState {
    K38_EVENT_NONE = 0x00
};

/* Group 7, EM_GROUP_EVENT2 (EmKind38_RunEvent2State): this kind has a dispatcher of its own for it. */
enum EmKind38Event2State {
    K38_EVENT_FADE_IN = 0x00            /* sinks into view after reappearing */
};

/* Kind 56's own meanings of the work area's bytes (em.h's EmWork has the shared ones). */
typedef struct EmKind38Work {
    u8 fxStep;                  /* 0x000: UpdateMotionEffects: 0, then 1 */
    u8 unk001[0x006 - 0x001];
    s16 stayTimer;              /* 0x006: Start: 1800 when spawned with spawnParam bit 0, else -1 (none); Update counts it down, and at 0 a state of groups 0 to 2 changes to group 5 */
    f32 bobPos;                 /* 0x008: the hover's bob: offset (InitHover: 15 to 24, its sign by the height above the floor), speed, step (SteerHover, ApplyHover) */
    f32 bobVel;                 /* 0x00C */
    f32 bobAccel;               /* 0x010 */
    f32 bobGainGoal;            /* 0x014: the gain ApplyHover eases bobGain to while stance is 2 (the states: 1.0, 0.6 or 0) */
    f32 bobGain;                /* 0x018: ApplyHover: posY += bobVel * bobGain */
    f32 baseScale;              /* 0x01C: Init: bodyScale; EmKind38_BurstOut grows the scale to it, EmKind38_SelfDestruct pulses the scale around it */
    u8 unk020[0x038 - 0x020];
    s16 seTimer;                /* 0x038: UpdateLoopSe: frames of the looped sound (played at 0, wraps at 10 or 11) */
    u8 seKind;                  /* 0x03A: UpdateLoopSe: the looped sound (1, 2; else none); Init: 0 */
    u8 dashing;                 /* 0x03B: EmKind38_Charge: 1 from the attack's creation to the dash's end (no C reads) */
    u8 charging;                /* 0x03C: EmKind38_Charge, EmKind38_SelfDestruct: 1 (RunState clears it each frame; EmKind38_IsCharging returns it) */
    u8 unk03D[0x03E - 0x03D];
    s16 scaleFrames;            /* 0x03E: EmKind38_BurstOut: the timer's first value (the scale grows over it) */
    s16 pulseAng;               /* 0x040: EmKind38_SelfDestruct: an angle stepped by 0x777, the scale's sine */
    u8 unk042[0x100 - 0x042];
} EmKind38Work;
typedef char EmKind38Work_size_check[(sizeof(EmKind38Work) == sizeof(EmWork)) ? 1 : -1];

extern void *EmKind38_Callbacks[8];
void EmKind38_UpdateLoopSe(Em *em, EmWork *work);
void EmKind38_Init(Em *em);
void EmKind38_Start(Em *em);
void EmKind38_FlyForward(Em *em, s16 variant);
void EmKind38_FlyBackward(Em *em);
void EmKind38_Climb(Em *em);
void EmKind38_Descend(Em *em);
void EmKind38_Sidestep(Em *em, s32 dir);
void EmKind38_Drift(Em *em);
void EmKind38_BurstOut(Em *em);
void EmKind38_Charge(Em *em, u8 k);
void *EmKind38_CreateChargeAttack(Em *em);
void EmKind38_SelfDestruct(Em *em);
void EmKind38_Die(Em *em, u8 variant);
void EmKind38_Reappear(Em *em);
void EmKind38_Update(Em *em);
void EmKind38_RunState(Em *em);
void EmKind38_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmKind38_InitHover(Em *em, EmWork *work);
void EmKind38_SteerHover(Em *em, EmWork *work);
void EmKind38_ApplyHover(Em *em, EmWork *work);
s32 EmKind38_MoveByVelocity(Em *em);
void EmKind38_ClampVelocity(Em *em, f32 *max);
void EmKind38_KeepHeight(Em *em, u8 k);
s32 EmKind38_HitByIce(Em *em);
void EmKind38_SpawnAtEmBone(Em *em);
u8 EmKind38_IsCharging(Em *em);
void EmKind38_PlaySe(Em *em, s32 soundId);

#endif
