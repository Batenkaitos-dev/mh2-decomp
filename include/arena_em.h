#ifndef ARENA_EM_H
#define ARENA_EM_H

#include "common.h"

/*
 * A monster kept for the arena (0x1C bytes): SaveData.arenaEms[3] (save.h)
 * and the plsel work's copy (PlselWork.unk06D8, plsel.h).
 */
typedef struct ArenaEm {
    u32 minutes;            /* at most 59999; grows by the quest time / 1800 */
    u16 wins;               /* at most 9999 */
    u16 losses;             /* at most 9999 */
    s16 unk8;               /* 50..300, x1.08 or x0.85 after a quest */
    s16 emKind;             /* the monster kind (gMonsterNames, MonsterNote_Index); 0: the entry is unused */
    u16 sizePct;            /* its size in percent (Monster_SizeFromPercent; gQuest.capturedSize) */
    char name[9];           /* its name (read with lb; "MONSTER" by default, Plsel_SaveRecordMenu) */
    s8 gaugeLevel;          /* 0..4, rises by chance (Versus_UpdateRecords; set from gaugeLevel) */
    u8 unk18;               /* 0-4 */
    s8 level;               /* the base level (Versus_UpdateRecords), the captured level (Plsel_SaveRecordMenu) */
    u8 unk1A[2];
} ArenaEm;
typedef char ArenaEm_size_check[(sizeof(ArenaEm) == 0x1C) ? 1 : -1];

#endif /* ARENA_EM_H */
