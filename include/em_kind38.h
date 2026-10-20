#ifndef EM_KIND38_H
#define EM_KIND38_H

/*
 * Kind 0x38's vocabulary (EmKind38_*): a small hovering
 * monster with one contact attack (the charge). Nothing in the code names
 * the monster. It is placed by the quest or spawned by Yama Tsukami
 * (EmKind38_SpawnAtEmBone; then it bursts out, stays 1800 frames and dies).
 * Its C sets only the idle motion; the script plays the others.
 */
enum EmKind38Motion {
    K38_MOT_IDLE = 0x01
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
    K38_FLY_SIDESTEP_A = 0x06,          /* heading 0x3800 off the goal's direction, one way */
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

/* Kind 0x38's own meanings of the work area's bytes (em.h's EmWork has the shared ones). */
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

#endif
