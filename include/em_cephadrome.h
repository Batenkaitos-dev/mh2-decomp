#ifndef EM_CEPHADROME_H
#define EM_CEPHADROME_H
#include "em.h"

/*
 * The Cephadrome family (EM_KIND_CEPHADROME, EM_KIND_CEPHALOS): its motions
 * and its states, by group (em.h's EmStateGroup). The functions are
 * EmCephadrome_*. The family swims under the sand (Em.stance
 * EM_STANCE_UNDER), surfaces to breathe sand at hunters, leaps out of the
 * sand and back, and lies beached on its side after a leap ashore
 * (EM_STANCE_FAMILY). Its group 2, the flying families' group, holds the
 * sand-swimming states. Each enum lists the cases of the group's dispatcher
 * (EmCephadrome_Run*State); where one state function serves several numbers,
 * the names say which variant the number asks for.
 */

/*
 * Motions: the family's motion table (gEmMotionRows[kind]) has row N play
 * motion N, so one number serves Em_SetMotion's row and
 * EM_MOTION_ID(layer, motion) kept in Em.motionId. The names say what the
 * states that play a motion show; motions only the motion sounds and
 * effects name stay numbers.
 */
enum EmCephadromeMotion {
    CEPH_MOT_IDLE = 0x01,                   /* the standing loop on all three layers (CEPH_IDLE_STAND, CEPH_IDLE); also after CEPH_MOT_IDLE_ACTION, in CEPH_REACT_FLINCH when layer 1 or 2 stops, the entrance and CEPH_EVENT_TO_IDLE */
    CEPH_MOT_WALK = 0x03,                   /* CEPH_MOVE_WALK */
    CEPH_MOT_RUN = 0x04,                    /* CEPH_MOVE_RUN, followed by CEPH_MOT_RUN_STOP */
    CEPH_MOT_TURN_RIGHT = 0x05,             /* the land turn row (motRight of EmCephadrome_TurnParams[0]), CEPH_MOVE_TURN (attack row 5 at frame 2, 6 at 30; dust from 2 to 78 while enraged) */
    CEPH_MOT_TURN_LEFT = 0x06,              /* the land turn row (motLeft of EmCephadrome_TurnParams[0]), CEPH_MOVE_TURN (attack row 8 at frame 2, 7 at 30; dust from 2 to 78 while enraged) */
    CEPH_MOT_LIMP = 0x0A,                   /* CEPH_MOVE_LIMP */
    CEPH_MOT_TOUCH_DOWN = 0x13,             /* played by no state: a landing on both feet, as the wyverns' touch-down (motion 19 too). Attack row 13 (both feet's spheres, hit list 9) at frame 2, the landing dust (EmCephadrome_SpawnLandingDust) at frame 16 */
    CEPH_MOT_IDLE_ACTION = 0x19,            /* CEPH_IDLE_ACTION */
    CEPH_MOT_REST = 0x1F,                   /* the rest loop of CEPH_IDLE_REST */
    CEPH_MOT_REST_START = 0x20,             /* lying down to rest (CEPH_IDLE_REST) */
    CEPH_MOT_REST_END = 0x21,               /* getting up from the rest (CEPH_IDLE_REST_END) */
    CEPH_MOT_STATUS_LIE = 0x22,             /* lying under status 1 or 2 on the ground until the timer runs out (StatusCollapse), and in CEPH_EVENT_CAPTURED; lying puffs */
    CEPH_MOT_LAND_ATTACK_A = 0x24,          /* CEPH_ATK_LAND_A (attack row 1 at frame 50) */
    CEPH_MOT_STATUS_GET_UP = 0x25,          /* getting up after a status (CEPH_IDLE_STATUS2_END, CEPH_IDLE_STATUS1_END), kept blind, then CEPH_IDLE_SHAKE_OFF */
    CEPH_MOT_STATUS_COLLAPSE = 0x26,        /* collapsing under status 1 or 2 on the ground (StatusCollapse) and in CEPH_EVENT_CAPTURED (attack row 15 at frame 148, lying puffs, dust at 156) */
    CEPH_MOT_SPIN = 0x2B,                   /* CEPH_ATK_SPIN, played twice (attack row 16 at frame 30, dust from 6 to 38 while enraged) */
    CEPH_MOT_BREATH = 0x2F,                 /* CEPH_ATK_BREATH (attack row 25 at 66, ranged attack at 78) */
    CEPH_MOT_FEED = 0x3B,                   /* feeding after CEPH_MOT_HEAD_DOWN (CEPH_IDLE_EAT_NEED2, which adds 5% of max hp, and CEPH_IDLE_EAT), then land idle */
    CEPH_MOT_FLINCH = 0x3C,                 /* CEPH_REACT_FLINCH and the start of CEPH_REACT_HOLD2 (to frame 52, no root motion); effects at 44 and 62, dust at 80 */
    CEPH_MOT_FLINCH_BLIND = 0x3D,           /* CEPH_REACT_FLINCH_BLIND (effect at 24) */
    CEPH_MOT_SHAKE_OFF = 0x3E,              /* CEPH_IDLE_SHAKE_OFF and the STATUS4, HOLD2 and STUN shake-offs (ShakeOff), then land idle */
    CEPH_MOT_FLINCH_C = 0x3F,               /* CEPH_REACT_FLINCH_C */
    CEPH_MOT_FLINCH_D = 0x40,               /* CEPH_REACT_FLINCH_D */
    CEPH_MOT_FLINCH_B = 0x42,               /* CEPH_REACT_FLINCH_B (dust at 268) */
    CEPH_MOT_DIE = 0x44,                    /* CEPH_DEFEAT_DIE, the end of DIE_UNDER and DIE_FALLING after CEPH_MOT_FLOP_A (dust at 50, camera shake at 212), then the carve mine point */
    CEPH_MOT_FALL_A = 0x45,                 /* KnockDown odd variants (KNOCK_DOWN, STUN_A): falls over (dust at 26), then CEPH_MOT_LIE_A */
    CEPH_MOT_LIE_A = 0x46,                  /* KnockDown odd variants: lies flapping (effects at 16 and 66) */
    CEPH_MOT_GET_UP_A = 0x47,               /* KnockDown odd variants and ThrownOutOfSand: gets up */
    CEPH_MOT_FALL_B = 0x4A,                 /* KnockDown even variants (KNOCK_DOWN_PART3, STUN_B): falls over (dust at 26), then CEPH_MOT_LIE_DOWN */
    CEPH_MOT_LIE_DOWN = 0x4B,               /* lying down at the start of the _BEACHED status collapses */
    CEPH_MOT_STAND_STILL = 0x4C,            /* CEPH_REACT_STATUS4 and CEPH_REACT_HOLD2: stands still until the timer runs out, then CEPH_IDLE_STATUS4_SHAKE_OFF */
    CEPH_MOT_HEAD_DOWN = 0x4F,              /* lowering the head to eat, dust clouds and a sound on given frames (CEPH_IDLE_EAT_NEED2); CEPH_IDLE_EAT_START, which goes on to CEPH_IDLE_EAT at frame 200 */
    CEPH_MOT_DIE_HELD = 0x61,               /* CEPH_DEFEAT_DIE_HELD only (Plesioth plays its motion 97 as the held status lie, PLES_MOT_TRAPPED_LIE); no motion sounds or effects */
    CEPH_MOT_SAND_IDLE = 0x64,              /* CEPH_SAND_IDLE */
    CEPH_MOT_SAND_SWIM_START = 0x65,        /* the start of CEPH_SAND_FORWARD */
    CEPH_MOT_SAND_SWIM = 0x66,              /* the swim loop after SWIM_START (CEPH_SAND_FORWARD) */
    CEPH_MOT_SAND_SWIM_FAST = 0x67,         /* fast swim (CEPH_SAND_DASH, ROUND_NEXT/PREV and their LEGs) and the glide of 1000 on re-entering the sand after SAND_LEAP_IN, SAND_LEAP and CEPH_ATK_LEAP (attack row 26 at frame 10) */
    CEPH_MOT_SAND_RISE = 0x68,              /* CEPH_SAND_RISE */
    CEPH_MOT_SAND_DIVE = 0x69,              /* CEPH_SAND_DIVE */
    CEPH_MOT_SAND_TURN_LEFT = 0x6A,         /* the in-place turn of CEPH_EVENT_ENTRANCE (second turn) */
    CEPH_MOT_SAND_TURN_RIGHT = 0x6B,        /* the in-place turn of CEPH_EVENT_ENTRANCE (first turn) */
    CEPH_MOT_LEAP_ATTACK = 0x6C,            /* CEPH_ATK_LEAP (attack row 14 at frame 10) */
    CEPH_MOT_SAND_IDLE_B = 0x6D,            /* CEPH_SAND_IDLE_B */
    CEPH_MOT_SAND_CURVE = 0x6E,             /* CEPH_SAND_CURVE only (curve set 0) */
    CEPH_MOT_SURFACE_BREATH_C = 0x70,       /* CEPH_ATK_SURFACE_BREATH_C (attack row 29 and pad vibration at frame 26 with a camera shake; the shot at 58, splashes at 26 and 98) */
    CEPH_MOT_SURFACE_LUNGE = 0x71,          /* CEPH_ATK_SURFACE_LUNGE (attack row 27 at 28) */
    CEPH_MOT_SAND_CIRCLE_RIGHT = 0x72,      /* CEPH_SAND_CIRCLE_RIGHT and CIRCLE_RIGHT_B */
    CEPH_MOT_SAND_CIRCLE_LEFT = 0x73,       /* CEPH_SAND_CIRCLE_LEFT */
    CEPH_MOT_SURFACE_BREATH_A = 0x74,       /* CEPH_ATK_SURFACE_BREATH_A */
    CEPH_MOT_SURFACE_BREATH_B = 0x75,       /* CEPH_ATK_SURFACE_BREATH_B */
    CEPH_MOT_LEAP = 0x77,                   /* airborne in a leap (SAND_LEAP_ASHORE, SAND_LEAP_IN, SAND_LEAP, SAND_LEAP_ON_LAND, SAND_LEAP_TO_FIXED_POINT, the entrance; attack row 24 at frame 2) */
    CEPH_MOT_LEAP_CROUCH = 0x78,            /* the crouch on land before SAND_LEAP_IN and SAND_LEAP_ON_LAND */
    CEPH_MOT_FLOP_A = 0x79,                 /* landing flop of CEPH_IDLE_LAND_FLOP_A */
    CEPH_MOT_BEACHED = 0x7A,                /* lying beached on its side (stance EM_STANCE_FAMILY) in the land flops (LandFlop) and the entrance, then CEPH_MOT_GET_UP (attack row 23 at frame 2, sand sprays) */
    CEPH_MOT_FLOP_B = 0x7B,                 /* landing flop of CEPH_IDLE_LAND_FLOP_B */
    CEPH_MOT_GET_UP = 0x7C,                 /* getting up from its side: the end of the land flops, the _BEACHED status ends, FlinchLying, KnockDown even variants, LandAfterFall, Status4LyingEnd, the entrance */
    CEPH_MOT_TUMBLE = 0x7E,                 /* FallFromAir, FallBlinded, ThrownOutOfSand: tumbling in the air or through the sand; also DieUnder and DieFalling */
    CEPH_MOT_LIE_FLAP = 0x80,               /* after CEPH_MOT_LIE_DOWN in the _BEACHED status collapses */
    CEPH_MOT_LIE_FLAP_B = 0x81,             /* KnockDown even variants after LIE_DOWN (effect at 46) */
    CEPH_MOT_SAND_RECOVER = 0x83,           /* recovering under the sand (SAND_STATUS4_RECOVER, SAND_STATUS2_END, SAND_STATUS1_END, the last two blind), then the sand idle */
    CEPH_MOT_RUN_STOP = 0x85,               /* after the run (CEPH_MOVE_RUN) */
    CEPH_MOT_SAND_SURFACED = 0x86,          /* the end of CEPH_SAND_RISE (attack row 28 at frame 2) */
    CEPH_MOT_SAND_BOTTOMED = 0x87,          /* the end of CEPH_SAND_DIVE on the floor */
    CEPH_MOT_LAND_ATTACK_B = 0x88,          /* CEPH_ATK_LAND_B (attack row 22 at frame 60) */
    CEPH_MOT_SIDE_LIE = 0x89,               /* lying on its side under a status until the timer, sand puffs at CEPH_BONE_BREATH and a sound every 135 frames (the _BEACHED status collapses) */
    CEPH_MOT_STATUS4_LYING = 0x8A           /* Status4Lying: lies under status 4 */
};

/*
 * Bones: a bone number indexes the model's skeleton (Em.model->bones, the
 * world matrix the sound and effect helpers read the position from). The
 * names say what the family's code and its hit shapes show a bone to be:
 * the shapes of the hits CephadromeAttack_Create and CephalosAttack_Create
 * make on given frames of a motion (their HitShapes lists, by the attack
 * rows' hit lists), the monster's own hit shapes (gEmBoneHits), where the
 * shots and the lying puffs start, and the bones the steps' sounds and the
 * run's dust take turns at. "A" and "B" are the two sides; which is left
 * the code doesn't show. The four fins (bones 37, 38, 39 and 16) are folded
 * and waved by Em_AdjustKindBones, with the Plesioth's. Bones no code
 * explains stay numbers: 6 and 12 (a pair: several motions' paired sounds
 * alternate between them, and the sand-surface splash's sound plays at 6)
 * and 27 (dust in CEPH_MOT_FLINCH).
 */
enum EmCephadromeBone {
    CEPH_BONE_ROOT = 0,     /* the skeleton's first bone: the falls', the flops' and the sand motions' sounds play here */
    CEPH_BONE_BODY = 2,     /* the body's wide sphere in the leaps (CEPH_MOT_LEAP, CEPH_MOT_LEAP_ATTACK), CEPH_MOT_LAND_ATTACK_B, CEPH_MOT_BEACHED, CEPH_MOT_STATUS_COLLAPSE, the fast swim and the surfacing (hit lists 3, 8, 10, 11, 13 and 14); dust */
    CEPH_BONE_LEG_A = 19,   /* leg A's upper sphere in the walk's, the turns' and the run's steps (hit lists 1 and 6, with CEPH_BONE_FOOT_A's) */
    CEPH_BONE_STEP_A = 20,  /* the steps' sounds and the run's dust at leg A, in turn with CEPH_BONE_STEP_B (CEPH_MOT_WALK, CEPH_MOT_RUN, the turns) */
    CEPH_BONE_FOOT_A = 22,  /* leg A's foot: the steps' larger sphere; both feet's spheres in CEPH_MOT_TOUCH_DOWN's hit (hit list 9) */
    CEPH_BONE_LEG_B = 25,   /* the same for leg B (hit lists 2 and 7) */
    CEPH_BONE_STEP_B = 26,
    CEPH_BONE_FOOT_B = 28,
    CEPH_BONE_BREATH = 34,  /* where the sand breath's shots (EMRANGED_CEPHADROME_SHOT) and bursts, the lying puffs and the stun stars start; the sphere of CEPH_MOT_SURFACE_LUNGE's hit (hit list 12) and part 6's */
    CEPH_BONE_HEAD = 35,    /* the head's sphere in CEPH_MOT_LAND_ATTACK_A and CEPH_MOT_BREATH (hit list 0); most of the family's sounds play here */
    CEPH_BONE_TAIL_A = 41,  /* the tail's capsule, to CEPH_BONE_TAIL_C, in CEPH_MOT_SPIN and CEPH_MOT_RUN (hit lists 4 and 5); part 7's spheres on 41 and 42 */
    CEPH_BONE_TAIL_B = 42,  /* the tail's middle: the spin's, the sand swims' and the dives' sounds play here */
    CEPH_BONE_TAIL_C = 44
};

/*
 * Sounds: a sound number indexes the kind's own sound bank (gSeBankByType
 * gives the bank by kind; Em_PlaySeAtBone and Em_PlaySeAtBoneOnFrame). A
 * sound is named where the code shows what it is: it plays on the frame of
 * the thing it goes with. The others stay numbers.
 */
enum EmCephadromeSe {
    CEPH_SE_BREATH = 24,    /* the sand breath: in CEPH_MOT_SURFACE_BREATH_A and _B on the frames of the three shots (122, 162, 192); in CEPH_MOT_BREATH among the bursts (78-82), in CEPH_MOT_SURFACE_BREATH_C before the shot */
    CEPH_SE_SPLASH = 26     /* the burst where it breaks the sand's surface (EmCephadrome_SpawnSurfaceSplash) */
};

/* Group 0, EM_GROUP_IDLE (EmCephadrome_RunIdleState). */
enum EmCephadromeIdleState {
    CEPH_IDLE_STAND = 0x00,                 /* layered idle until the timer, then the script decides */
    CEPH_IDLE = 0x01,                       /* idle on land */
    CEPH_IDLE_ACTION = 0x02,                /* CEPH_MOT_IDLE_ACTION then the idle motion */
    CEPH_IDLE_EAT_NEED2 = 0x03,             /* eats in one go, +5% hp, refills need 2 */
    CEPH_IDLE_LAND_FLOP_A = 0x04,           /* landed from a leap: flop A, beached, gets up (entered by SAND_LEAP_ASHORE, SAND_LEAP_ON_LAND, and SAND_LEAP_IN, SAND_LEAP, CEPH_ATK_LEAP over hard ground) */
    CEPH_IDLE_STATUS2_COLLAPSE = 0x05,      /* entry begins status 2 */
    CEPH_IDLE_STATUS2_END = 0x06,           /* entry ends status 2 */
    CEPH_IDLE_SHAKE_OFF = 0x07,             /* CEPH_MOT_SHAKE_OFF after getting up */
    CEPH_IDLE_LAND_FLOP_B = 0x09,           /* as LAND_FLOP_A with flop B (not entered by this file) */
    CEPH_IDLE_STATUS2_COLLAPSE_BEACHED = 0x0E,/* entry begins status 2 */
    CEPH_IDLE_STATUS2_END_BEACHED = 0x0F,   /* entry ends status 2 */
    CEPH_IDLE_STATUS1_COLLAPSE_BEACHED = 0x10,/* entry begins status 1 */
    CEPH_IDLE_STATUS1_END_BEACHED = 0x11,   /* entry ends status 1 */
    CEPH_IDLE_REST = 0x12,                  /* entry resets the notice */
    CEPH_IDLE_REST_END = 0x13,              /* entry refills need 3 */
    CEPH_IDLE_EAT_START = 0x14,             /* lowers its head, then CEPH_IDLE_EAT */
    CEPH_IDLE_STATUS4_SHAKE_OFF = 0x15,     /* entry ends status 4 */
    CEPH_IDLE_HOLD2_SHAKE_OFF = 0x16,       /* entry clears holdTimer2 */
    CEPH_IDLE_STUN_SHAKE_OFF = 0x17,        /* entry ends status 5 (the stun) */
    CEPH_IDLE_EAT = 0x28,                   /* entry starts the goal action */
    CEPH_IDLE_STATUS1_COLLAPSE = 0x29,      /* entry begins status 1 */
    CEPH_IDLE_STATUS1_END = 0x2A            /* entry ends status 1 */
};

/* Group 1, EM_GROUP_MOVE (EmCephadrome_RunMoveState). */
enum EmCephadromeMoveState {
    CEPH_MOVE_WALK = 0x00,                  /* walk to the goal (CEPH_MOVE_LIMP instead below a tenth of hpMax, by SetMoveState) */
    CEPH_MOVE_RUN = 0x01,                   /* run to the goal, then stop */
    CEPH_MOVE_TURN = 0x02,                  /* turn toward the target on land */
    CEPH_MOVE_LIMP = 0x03                   /* limp to the goal (low hp) */
};

/*
 * Group 2, EM_GROUP_FLY (EmCephadrome_RunSandState): for this family,
 * swimming under the sand, surfacing and leaping.
 */
enum EmCephadromeSandState {
    CEPH_SAND_IDLE = 0x00,                  /* lies under the sand */
    CEPH_SAND_IDLE_B = 0x01,                /* sand idle, other motion */
    CEPH_SAND_CIRCLE_RIGHT_B = 0x02,        /* circle right, heading not wrapped */
    CEPH_SAND_TURN = 0x03,                  /* turn toward the target under the sand */
    CEPH_SAND_FORWARD = 0x04,               /* swim to the goal (speed scaled on entry) */
    CEPH_SAND_DASH = 0x05,                  /* fast swim to the goal with wakes (speed scaled on entry) */
    CEPH_SAND_CURVE = 0x06,                 /* curve set 0 */
    CEPH_SAND_RISE = 0x07,                  /* rise to the swim height under the sand top */
    CEPH_SAND_DIVE = 0x08,                  /* sink to the floor */
    CEPH_SAND_ROUND_TURN = 0x09,            /* turn to the next area point, then ROUND_NEXT */
    CEPH_SAND_ROUND_NEXT = 0x0A,            /* 4 legs round the area points, forward order (entry sets roundLegsLeft 4 and the nearest point as goal; ROUND_PREV if the following point is behind, ROUND_TURN if the goal is not ahead) */
    CEPH_SAND_ROUND_PREV = 0x0B,            /* 4 legs round the area points, backward order (entry sets roundLegsLeft 4 and the nearest point as goal; ROUND_TURN, then ROUND_NEXT, if the goal is not ahead) */
    CEPH_SAND_CIRCLE_RIGHT = 0x0C,          /* swims a circle turning right */
    CEPH_SAND_CIRCLE_LEFT = 0x0D,           /* swims a circle turning left */
    CEPH_SAND_LEAP_ASHORE = 0x0E,           /* leap out of the sand, always landing on the ground */
    CEPH_SAND_SURFACE_FLOP = 0x0F,          /* flop under the sand top */
    CEPH_SAND_LEAP_IN = 0x10,               /* leap from land into the sand (or onto land) */
    CEPH_SAND_LEAP = 0x11,                  /* leap out of the sand, back in or onto land */
    CEPH_SAND_LEAP_ON_LAND = 0x12,          /* crouch and leap on land, landing on the ground */
    CEPH_SAND_ROUND_NEXT_LEG = 0x13,        /* the following legs of ROUND_NEXT (no entry work, the previous leg sets the goal and timer) */
    CEPH_SAND_ROUND_PREV_LEG = 0x14,        /* the following legs of ROUND_PREV */
    CEPH_SAND_STATUS4_RECOVER = 0x15,       /* entry ends status 4 */
    CEPH_SAND_LEAP_TO_FIXED_POINT = 0x16,   /* leap to the fixed point (6000, -1000, 10000) */
    CEPH_SAND_EMPTY = 0x17,                 /* empty case, nothing runs */
    CEPH_SAND_STATUS2_END = 0x18,           /* entry ends status 2 */
    CEPH_SAND_STATUS1_END = 0x19,           /* entry ends status 1 */
    CEPH_SAND_TURN_NO_REACT = 0x1A,         /* SandTurn with noReact and the sense/hit/push timers */
    CEPH_SAND_DASH_NO_REACT = 0x1B          /* SandDash with noReact and the timers (also set on entry) */
};

/* Group 3, EM_GROUP_ATTACK (EmCephadrome_RunAttackState). */
enum EmCephadromeAttackState {
    CEPH_ATK_LAND_A = 0x00,                 /* land attack, CEPH_MOT_LAND_ATTACK_A */
    CEPH_ATK_BREATH = 0x01,                 /* ranged breath on land (entry sets unk013 1) */
    CEPH_ATK_SPIN = 0x02,                   /* turning spin on land */
    CEPH_ATK_SURFACE_BREATH_A = 0x03,       /* three-way breath from under the sand, yaw +40, 0, -40 degrees at frames 122, 162, 192 (CEPH_MOT_SURFACE_BREATH_A) */
    CEPH_ATK_SURFACE_BREATH_B = 0x04,       /* three-way breath from under the sand, yaw -40, 0, +40 degrees at frames 122, 162, 192 (CEPH_MOT_SURFACE_BREATH_B) */
    CEPH_ATK_SURFACE_BREATH_C = 0x05,       /* single breath from under the sand */
    CEPH_ATK_LEAP = 0x06,                   /* leap attack out of the sand */
    CEPH_ATK_LAND_B = 0x07,                 /* land attack, CEPH_MOT_LAND_ATTACK_B */
    CEPH_ATK_SURFACE_LUNGE = 0x08           /* lunge along the sand top */
};

/* Group 4, EM_GROUP_REACT (EmCephadrome_RunReactState). */
enum EmCephadromeReactState {
    CEPH_REACT_FLINCH = 0x00,               /* flinch, parts 0 and 7, or EM_DMG_HIT_MODE3 */
    CEPH_REACT_FLINCH_B = 0x01,             /* flinch, unbroken part 3 or 4 */
    CEPH_REACT_FLINCH_C = 0x02,             /* flinch, parts 5 and 6 */
    CEPH_REACT_FLINCH_D = 0x03,             /* flinch, parts 1 and 2 */
    CEPH_REACT_FLINCH_LYING = 0x04,         /* flinch while beached (stance EM_STANCE_FAMILY) */
    CEPH_REACT_KNOCK_DOWN = 0x05,           /* flinch of a broken part other than 3 (KnockDown variant 1) */
    CEPH_REACT_FALL = 0x06,                 /* falls out of the air (EM_DMG_STATUS_IN_AIR, a flinch in the air): CEPH_MOT_TUMBLE down to floorY, then CEPH_REACT_LAND */
    CEPH_REACT_STATUS4_LYING = 0x07,        /* status 4 while beached (begins status 4) */
    CEPH_REACT_EMPTY_A = 0x08,              /* empty case, nothing runs or enters it */
    CEPH_REACT_FALL_BLIND = 0x09,           /* blinded fall, no C entry */
    CEPH_REACT_FLINCH_BLIND = 0x0A,         /* blinded flinch on land, no C entry */
    CEPH_REACT_STATUS4 = 0x0B,              /* status 4 on land (begins status 4) */
    CEPH_REACT_EMPTY_B = 0x0C,              /* empty case */
    CEPH_REACT_EMPTY_C = 0x0D,              /* empty case */
    CEPH_REACT_EMPTY_D = 0x0E,              /* empty case */
    CEPH_REACT_EMPTY_E = 0x0F,              /* empty case */
    CEPH_REACT_LAND = 0x10,                 /* lands on the ground after a fall and gets up */
    CEPH_REACT_THROWN_OUT = 0x11,           /* thrown up out of the sand (a flinch, status 4 or status 2 under the sand): tumbles, lands; FLOP_A, LIE_DOWN, GET_UP_A, then land idle */
    CEPH_REACT_THROWN_OUT_ATTACK5 = 0x12,   /* thrown out by EM_DMG_ATTACK5 under the sand (Cephadrome leaves a timed mine point at the sand top, EmMine_CreateTimed0); otherwise as THROWN_OUT */
    CEPH_REACT_STATUS4_LYING_END = 0x13,    /* ends status 4, gets up from beached */
    CEPH_REACT_KNOCK_DOWN_PART3 = 0x14,     /* flinch of the broken part 3 (KnockDown variant 0) */
    CEPH_REACT_HOLD2 = 0x15,                /* EM_DMG_ATTACK13 (ends all statuses) */
    CEPH_REACT_STUN_A = 0x16,               /* the stun, hit from (hitAngle - rotY) above ANGLE_TURN / 2 (begins the stun; KnockDown variant 3, then CEPH_IDLE_STUN_SHAKE_OFF) */
    CEPH_REACT_STUN_B = 0x17                /* the stun, hit from the other side (begins the stun) */
};

/* Group 5, EM_GROUP_DEFEAT (EmCephadrome_RunDefeatState). */
enum EmCephadromeDefeatState {
    CEPH_DEFEAT_DIE = 0x00,                 /* killed on the ground: dies, mine point, fades */
    CEPH_DEFEAT_DIE_HELD = 0x01,            /* killed while held */
    CEPH_DEFEAT_DIE_UNDER = 0x02,           /* killed under the sand (or lying, or in the air below the surface): thrown up, falls, CEPH_MOT_FLOP_A, then as CEPH_DEFEAT_DIE */
    CEPH_DEFEAT_REAPPEAR = 0x04,            /* after the fade: back under the sand when the quest lets it, in CEPH_SAND_IDLE; otherwise its task is finished (taskPhase) */
    CEPH_DEFEAT_DIE_FALLING = 0x05          /* killed in the air above the surface: falls, lands, dies */
};

/* Group 6, EM_GROUP_EVENT (EmCephadrome_RunEventState). */
enum EmCephadromeEventState {
    CEPH_EVENT_ENTRANCE = 0x00,             /* scripted entrance under the sand ending in a leap ashore */
    CEPH_EVENT_TO_IDLE = 0x01,              /* hidden one frame, then land idle */
    CEPH_EVENT_CAPTURED = 0x02              /* captured (begins status 1) */
};

extern void *EmCephadrome_Callbacks[8];
void EmCephadrome_MotionSounds(Em *em, EmWork *work);
void EmCephadrome_Init(Em *em);
void EmCephadrome_Start(Em *em);
void EmCephadrome_LandFlop(Em *em, EmWork *work, u8 variant);
void EmCephadrome_Status2CollapseBeached(Em *em, EmWork *work);
void EmCephadrome_Status1CollapseBeached(Em *em, EmWork *work);
void EmCephadrome_SandRoundNext(Em *em, EmWork *work);
void EmCephadrome_SandRoundPrev(Em *em, EmWork *work);
void EmCephadrome_LeapIntoSand(Em *em, EmWork *work);
void EmCephadrome_LeapFromSand(Em *em, EmWork *work);
void EmCephadrome_LeapAttack(Em *em, EmWork *work);
void EmCephadrome_KnockDown(Em *em, EmWork *work, s16 variant);
void EmCephadrome_FallFromAir(Em *em, EmWork *work);
void EmCephadrome_FallBlinded(Em *em, EmWork *work);
void EmCephadrome_ThrownOutOfSand(Em *em, EmWork *work, s32 variant);
void EmCephadrome_EntranceScene(Em *em, EmWork *work);
void EmCephadrome_Die(Em *em, EmWork *work);
void EmCephadrome_DieHeld(Em *em, EmWork *work);
void EmCephadrome_DieUnder(Em *em, EmWork *work);
void EmCephadrome_DieFalling(Em *em, EmWork *work);
void EmCephadrome_RunIdleState(Em *em, EmWork *work);
void EmCephadrome_RunSandState(Em *em, EmWork *work);
void EmCephadrome_RunReactState(Em *em, EmWork *work);
void EmCephadrome_Update(Em *em);
void EmCephadrome_RunState(Em *em, s32 work);
void EmCephadrome_MotionEffects(Em *em, EmWork *work);
void EmCephadrome_UpdateFins(Em *em, EmWork *work);
void EmCephadrome_SetFinMode(Em *em, EmWork *work, s32 mode);
void EmCephadrome_SpawnLandingDust(Em *em);
void EmCephadrome_SpawnWakeEvery4(Em *em, f32 scale);
void EmCephadrome_SpawnSurfaceSplash(Em *em, f32 scale);
void EmCephadrome_CreateAttack(Em *em, s32 kind);
void EmCephadrome_VibrateNearby(Em *em);
void EmCephadrome_ChangeState(Em *em, u16 group, u16 state, u8 mode);
void EmCephadrome_StartCurves(Em *em, u8 kind);
u8 EmCephadrome_UpdateCurves(Em *em);
u8 EmCephadrome_NextAreaPoint(Em *em, f32 *out);
s32 EmCephadrome_IsUnderOffSand(Em *em);
void EmCephadrome_StepAreaPoint(Em *em, u8 dir);

#endif
