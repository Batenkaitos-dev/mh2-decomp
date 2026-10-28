#ifndef EM_KIND1D_H
#define EM_KIND1D_H
#include "em.h"

/*
 * Monster kind 29's vocabulary (EmKind1D_*, the kind in hex). No code shows
 * its species: it never moves or attacks, each of its groups has one or two
 * states that end at once or after a few frames, and its death plays its
 * variant's sound and effects. Capcom's name for kind 29 is EM_ROCK
 * (mhfo.dll's kind table), hence EM_KIND_ROCK.
 */
#define KIND1D_MOT_IDLE 0x01  /* the one motion EmKind1D_Init sets */

/* Group 0, EM_GROUP_IDLE (EmKind1D_RunIdleState); group 2 runs it too. */
enum EmKind1DIdleState {
    KIND1D_IDLE_NOP = 0x00  /* EmKind1D_IdleNop: does nothing */
};

/* Group 1, EM_GROUP_MOVE (EmKind1D_RunMoveState). */
enum EmKind1DMoveState {
    KIND1D_MOVE_END = 0x00  /* EmKind1D_MoveEnd: ends at once */
};

/* Group 3, EM_GROUP_ATTACK (EmKind1D_RunAttackState). */
enum EmKind1DAttackState {
    KIND1D_ATK_END = 0x00  /* EmKind1D_AttackEnd: ends at once */
};

/* Group 4 (and 6, 7), EM_GROUP_REACT (EmKind1D_RunReactState). */
enum EmKind1DReactState {
    KIND1D_REACT_FLINCH = 0x00,  /* EmKind1D_Flinch: 10 frames; EmKind1D_Update's pick for a part flinch or a hit of hitReactMode 3 */
    KIND1D_REACT_END = 0x01      /* EmKind1D_ReactEnd: ends at once */
};

/* Group 5, EM_GROUP_DEFEAT: EmKind1D_Die runs it whatever the state. */
enum EmKind1DDefeatState {
    KIND1D_DEFEAT_DIE = 0x00  /* EmKind1D_Update's pick when a hit takes the last hp */
};

extern void *EmKind1D_Callbacks[8];
void EmKind1D_Init(Em *em);
void EmKind1D_Die(Em *em);
void EmKind1D_Update(Em *em);
void EmKind1D_RunState(Em *em);

#endif
