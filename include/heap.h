#ifndef HEAP_H
#define HEAP_H

/*
 * The boot ELF's memory managers (src/main/unk_1826D0.c).
 *
 * Heap: blocks addressed by handles (1-based indices into a table of
 * HeapBlock records), kept in a list ordered by address so that they can be
 * compacted (moved together) when a request doesn't fit. The main heap is
 * gHeap (0x2491B0), with 0x3000 records at 0x2191B0.
 *
 * LinearHeap: a range allocated from both ends, released back to a saved
 * position (a LinearMark).
 */
#include "common.h"

typedef struct HeapBlock {
    u32 addr;           /* 0x0 */
    u32 size;           /* 0x4; 0 = free record */
    s16 align;          /* 0x8 */
    s16 flags;          /* 0xA */
    u16 prev;           /* 0xC; 0xFFFF = none */
    u16 next;           /* 0xE */
} HeapBlock;

typedef struct Heap {
    s32 count;          /* 0x00 records in blocks */
    u32 size;           /* 0x04 bytes managed */
    HeapBlock *blocks;  /* 0x08 */
    s32 upward;         /* 0x0C 1: blocks from the start up; 0: from the end down */
    u32 cur;            /* 0x10 where the next block goes */
    u32 base;           /* 0x14 the aligned start (upward) or end */
    s32 align;          /* 0x18 */
    u32 used;           /* 0x1C bytes in blocks */
    u32 reserved;       /* 0x20 */
    s32 first;          /* 0x24 first block of the address list; 0xFFFF = none */
} Heap;

typedef struct LinearHeap {
    u32 start;          /* 0x00 */
    u32 lo;             /* 0x04 aligned start */
    u32 hi;             /* 0x08 aligned end */
    u32 pos[2];         /* 0x0C [0] allocated up to here from lo; [1] down to here from hi */
    s32 align;          /* 0x14 */
} LinearHeap;

typedef struct LinearMark {
    u32 pos;
    s32 side;
} LinearMark;

#define HEAP_ALIGN(x, a) (~((a) - 1) & ((x) + (a) - 1))

#endif /* HEAP_H */
