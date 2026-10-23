#ifndef TOWN_LOCK_ON_H
#define TOWN_LOCK_ON_H

#include "common.h"
#include "player.h"

/*
 * The town's lock-on: LockOn_Update picks, each frame, the local player's
 * target among the townspeople and the other shown players in reach (the
 * d-pad cycles through them), with the action spot the player stands at as
 * the fallback. LockOn is the view of an actor's ObjHead it reads.
 */

/* A lock-on candidate (a player or a townsperson) as the lobby's lock-on
 * reads it: it keeps its own values in fields the game uses otherwise
 * (ObjHead.hp, ObjHead.target). */
typedef struct LockOn {
    /* 0x000 */ u8 pad0[172];
    /* 0x0AC */ f32 pos[3];         /* ObjHead.pos */
    /* 0x0B8 */ u8 padB8[706 - 184];
    /* 0x2C2 */ s16 sector;         /* ObjHead.hp: by the angle from the player, 0 to 5 (LockOn_SetSector); -1 out of reach; orders the list (LockOn_Insert) */
    /* 0x2C4 */ u8 pad2C4[948 - 708];
    /* 0x3B4 */ struct LockOn *next; /* ObjHead.target: the next candidate (LockOn_Insert); a player's: its lock-on target */
    /* 0x3B8 */ u8 pad3B8[982 - 952];
    /* 0x3D6 */ u8 targeted;
    /* 0x3D7 */ u8 pad3D7[1192 - 983];
    /* 0x4A8 */ f32 dist;           /* from the player (LockOn_TestCandidate) */
} LockOn;

void LockOn_SetSector(Player *pl, LockOn *cand, u32 angle, u32 halfArc, u8 cycleDir);
void LockOn_Update(void);

#endif
