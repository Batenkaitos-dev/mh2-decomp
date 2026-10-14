#ifndef MINE_POINT_H
#define MINE_POINT_H
#include "common.h"

/* A mining point (sub_main's unk_3A1070.c, 0x3A5650; the monsters that leave one behind,
 * such as Kut-Ku, Rathalos, Lao-Shan Lung and Shen Gaoren, add them). */
typedef struct MinePoint {
    f32 x;              /* 0x00 */
    f32 y;
    f32 z;
    f32 radius;         /* 0x0C */
    u16 table;          /* 0x10: 0xFFFF = free slot */
    u16 count;          /* 0x12: gathers left, 0xFF = unlimited */
    u16 unk14;
    u16 unk16;
    u16 area;           /* 0x18 */
    u8 flags;           /* 0x1A: bit 0 = never removed to make room */
    u8 pad1B[3];
    s16 timer;          /* 0x1E: frames left, 0 = no limit */
} MinePoint;

extern MinePoint gMinePoints[20];   /* 0x5D87C0 */
extern s8 gMineActive[20];          /* 0x6AEDE0: the slots in use (gQuest.mineCount of them) */

s32 MinePoint_Add(MinePoint *src, f32 *pos, u8 noBonus);
s32 MinePoint_AddUntimed(MinePoint *pt, f32 *pos, u8 noBonus); /* sub_main unk_3A1070.c */
void MinePoint_Remove(s32 slot);
void MinePoint_RemoveOldest(void);
void MinePoint_SetPos(s32 slot, f32 *pos);

#endif /* MINE_POINT_H */
