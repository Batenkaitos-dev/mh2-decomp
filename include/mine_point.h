#ifndef MINE_POINT_H
#define MINE_POINT_H
#include "common.h"

/*
 * The gathering points placed while a quest runs (sub_main's unk_3A1070.c,
 * from 0x3A5650): a monster's carcass (carved), the items a monster drops
 * (picked up), and the special point the local player makes by using an
 * item. Up to MINE_POINT_COUNT points, in gMinePoints, with the active list
 * gMineActive (oldest first). The stage's own gathering points are
 * GatherPoint (quest.h).
 */

#define MINE_POINT_COUNT    20

/* A point placed at run time (0x20 bytes). */
typedef struct MinePoint {
    f32 x;              /* 0x00 */
    f32 y;
    f32 z;
    f32 radius;         /* 0x0C: the player must stand within it (x and z) */
    u16 table;          /* 0x10: the item table (gGatherItemTables, GATHER_TABLE_MASK), MINE_TABLE_SPECIAL, MINE_POINT_FREE */
    u16 count;          /* 0x12: gathers left, GATHER_UNLIMITED: no limit */
    u16 action;         /* 0x14: MINE_ACTION_*: the player's action there (MinePoint_FindNearPlayer) */
    u16 unk16;
    u16 area;           /* 0x18 */
    u8 flags;           /* 0x1A: MINE_POINT_KEEP */
    u8 pad1B[3];
    s16 timer;          /* 0x1E: frames left, 0 = no limit */
} MinePoint;

#define MINE_POINT_FREE     0xFFFF  /* MinePoint.table of an unused slot */
#define MINE_TABLE_SPECIAL  0x1000  /* MinePoint.table of the special point (SpecialMine_Gather) */
#define MINE_POINT_KEEP     0x01    /* MinePoint.flags: never removed to make room, nor at the quest's end */

/* MinePoint.action: what the player does there (the game's Player_TryGather). */
#define MINE_ACTION_PICK    0       /* picks up items a monster dropped */
#define MINE_ACTION_CARVE   2       /* carves a carcass */
#define MINE_ACTION_SPECIAL 5       /* gathers the special point */

/* A monster kind's carcass points (gEmMinePoints[kind], 16 bytes each). */
typedef struct EmMineEntry {
    u16 table[3];       /* 0x0: the item table by the stage's tier (StageHeader.tier) */
    u16 count;          /* 0x6: the carves */
    f32 radius;         /* 0x8: scaled by the monster's size (EmMine_Create) */
    u8 padC[2];
    u8 flags;           /* 0xE: MinePoint.flags */
    u8 bone;            /* 0xF: the bone the point follows, 0: the monster's position */
} EmMineEntry;

/* A monster kind's dropped items (gEmMinePoints2[kind], 8 bytes each). */
typedef struct EmMineEntry2 {
    u16 table[3];       /* 0x0: the item table by the stage's tier */
    u8 chance;          /* 0x6: the drop's chance, out of 32 */
    u8 pad7;
} EmMineEntry2;

/* The object that holds a monster's body (em_bulldrome.c passes its effect object; EmHolder_CreateMine). */
typedef struct EmHolder {
    u8 pad0[0x34];
    struct Em *em;      /* 0x34 */
    u8 pad38[6];
    u16 area;           /* 0x3E */
} EmHolder;

/* An entry of the special point's reward lists (gSpecialMineRewards[point], one per rank; 4 bytes). */
typedef struct SpecialMineReward {
    u16 item;
    u16 unk2;           /* not read by the code */
} SpecialMineReward;

extern MinePoint gMinePoints[MINE_POINT_COUNT]; /* 0x5D87C0 */
extern s8 gMineActive[MINE_POINT_COUNT];        /* 0x6AEDE0: the slots in use, oldest first (gQuest.mineCount of them) */

s32 MinePoint_Add(MinePoint *src, f32 *pos, u8 noBonus);
s32 MinePoint_AddUntimed(MinePoint *pt, f32 *pos, u8 noBonus); /* sub_main unk_3A1070.c */
void MinePoint_Remove(s32 slot);
void MinePoint_RemoveOldest(void);
void MinePoint_SetPos(s32 slot, f32 *pos);

#endif /* MINE_POINT_H */
