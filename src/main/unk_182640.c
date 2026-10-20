/*
 * Sys_InitHeap: sets up the main handle heap gHeap
 * (include/heap.h) over a memory area,
 * with gHeapBlocks' HEAP_BLOCK_COUNT blocks.
 */
#include "common.h"
#include "heap.h"

extern HeapBlock gHeapBlocks[];
extern Heap gHeap;

void Heap_Init(Heap *h, HeapBlock *blocks, s32 count, u32 start, u32 size, s32 align, s32 upward);

/* Display's Sys_InitWork gives it the system heap's area, aligned to
 * 0x40; the heap allocates upward. */
void Sys_InitHeap(void *start, s32 size, s32 align)
{
    Heap_Init(&gHeap, gHeapBlocks, HEAP_BLOCK_COUNT, (u32)start, size, align, 1);
}
