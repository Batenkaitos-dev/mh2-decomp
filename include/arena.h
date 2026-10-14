#ifndef ARENA_H
#define ARENA_H

#include "common.h"

/*
 * A small heap of sub_main (src/sub_main/unk_30E420.c: Arena_Init sets one
 * up, Arena_Alloc, Arena_Free and Arena_Realloc use it): block records
 * {size, addr, next} kept in address order between a head and a tail record,
 * optionally guarded by one semaphore. The browser's page heap
 * (BrowserWork.heap) and the lobby's image heap (gImgHeap) are Arenas.
 * Not heap.h's Heap.
 */
typedef struct ArenaBlk {
    s32 size;                   /* 0x0: 0 for a free record */
    s32 addr;                   /* 0x4 */
    struct ArenaBlk *next;      /* 0x8 */
} ArenaBlk;

typedef char ArenaBlk_size_check[(sizeof(ArenaBlk) == 0xC) ? 1 : -1];

typedef struct Arena {
    s32 align;                  /* 0x00 */
    s32 base;                   /* 0x04: the memory's address (an integer: the blocks' addresses are computed from it) */
    s32 size;                   /* 0x08 */
    ArenaBlk *recs;             /* 0x0C: the block records */
    s32 nrecs;                  /* 0x10: at least 2 (the head and the tail) */
    u16 locked;                 /* 0x14: use the semaphore (0 or 1) */
    u16 unk16;                  /* 0x16: Img_InitHeap sets 2; Arena_Init does not read it */
    ArenaBlk *head;             /* 0x18 */
    ArenaBlk *tail;             /* 0x1C */
    s32 unk20;                  /* 0x20: Arena_Init clears it */
    s32 unk24;                  /* 0x24: Arena_Init clears it */
} Arena;

typedef char Arena_size_check[(sizeof(Arena) == 0x28) ? 1 : -1];

#endif
