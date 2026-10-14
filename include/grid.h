#ifndef GRID_H
#define GRID_H

#include "common.h"

/* A paged list cursor (0xC bytes; sub_main's Menu_GridCursor moves it):
 * rows per column, the row and column, the columns and the rows of the last. */
typedef struct Grid {
    /* 0x00 */ s32 unk0;
    /* 0x04 */ u16 index;           /* row + rows * column */
    /* 0x06 */ u8 rows;             /* rows per column (10) */
    /* 0x07 */ u8 row;
    /* 0x08 */ u8 col;
    /* 0x09 */ u8 cols;             /* (count + 9) / 10 */
    /* 0x0A */ u8 lastRows;         /* rows in the last column */
} Grid;
typedef char Grid_size_check[(sizeof(Grid) == 0xC) ? 1 : -1];

#endif
