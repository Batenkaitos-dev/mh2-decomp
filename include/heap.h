#ifndef HEAP_H
#define HEAP_H

/*
 * The boot ELF's memory managers.
 *
 * Heap: blocks addressed by handles (1-based indices into a table of
 * HeapBlock records), kept in a list ordered by address so that they can be
 * compacted (moved together) when a request doesn't fit. The main heap is
 * gHeap (0x2491B0), with 12288 records (gHeapBlocks).
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
    u16 prev;           /* 0xC; HEAP_NONE = none */
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
    s32 first;          /* 0x24 first block of the address list; HEAP_NONE = none */
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

#define HEAP_BLOCK_COUNT 0x3000  /* gHeapBlocks' entries, gHeap's blocks (Sys_InitHeap) */
/* No record: the end of the address list (prev, next, first), or
 * Heap_NewRecord finding every record in use. */
#define HEAP_NONE 0xFFFF
#define HEAP_ALIGN(x, a) (~((a) - 1) & ((x) + (a) - 1))

void Sys_InitHeap(void *start, s32 size, s32 align);
void *MainHeap_Alloc(u32 size);
void *MainHeap_AllocTop(u32 size);
u8 *MainHeap_MaxAlloc(u32 size);
void *MainHeap_GetAddr(void *handle);
void MainHeap_Free(void *handle);
void MainHeap_Compact(void);
void Heap_Init(Heap *heap, HeapBlock *blocks, s32 count, u32 start, u32 size, s32 align, s32 upward);
s32 Heap_AllocTopAligned(Heap *heap, u32 size);
s32 Heap_AllocTop(Heap *heap, u32 size, s32 align);
s32 Heap_Alloc(Heap *heap, u32 size);
u32 Heap_MaxAlloc(Heap *heap, u32 size);
u32 Heap_GetAddr(Heap *heap, s32 handle);
s32 Heap_Free(Heap *heap, s32 handle);
u32 Heap_Compact(Heap *heap);
u32 Heap_FreeSize(Heap *heap);
s32 Heap_NewRecord(Heap *heap);
void Heap_Link(Heap *heap, s32 slot);
void Heap_Unlink(Heap *heap, s32 slot);
s32 LinearHeap_Init(LinearHeap *heap, void *start, u32 size, s32 align);
void *LinearHeap_Alloc(LinearHeap *heap, u32 size, s32 fromTop);
s32 LinearHeap_GetMark(LinearHeap *heap, s32 side, LinearMark *mark);
void LinearHeap_Release(LinearHeap *heap, LinearMark *mark);
s32 Debug_Sprintf(const char *fmt, ...);
void Heap_Fill(void *dst, u8 value, s32 size);
void Heap_Move(void *dst, void *src, s32 size);

#endif /* HEAP_H */
