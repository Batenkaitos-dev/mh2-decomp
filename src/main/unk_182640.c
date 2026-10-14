/*
 * main 0x182640-0x182670
 */
#include "common.h"
#include "em.h"

/* prototypes */
#include "heap.h"
extern HeapBlock gHeapBlocks[];
extern Heap gHeap;
/* end of prototypes */

void Heap_Init(Heap *h, HeapBlock *blocks, s32 count, u32 start, u32 size, s32 align, s32 upward);
void Sys_InitHeap(void *start, s32 size, s32 align)
{
    Heap_Init(&gHeap, gHeapBlocks, 0x3000, (u32)start, size, align, 0x1);
}
