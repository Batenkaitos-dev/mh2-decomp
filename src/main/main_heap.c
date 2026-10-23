/*
 * The main heap: Sys_InitHeap sets it up (gHeap over a memory area, with
 * gHeapBlocks' HEAP_BLOCK_COUNT blocks), and its calls each pass gHeap to
 * the handle heap's function of the same name (include/heap.h). The
 * handles go out as pointers.
 */
#include "common.h"
#include "heap.h"

extern Heap gHeap;
extern HeapBlock gHeapBlocks[];

/* Sets the main heap up over size bytes at start. Sys_InitWork gives it
 * the system heap's area, aligned to 64 bytes; the heap allocates upward. */
void Sys_InitHeap(void *start, s32 size, s32 align)
{
    Heap_Init(&gHeap, gHeapBlocks, HEAP_BLOCK_COUNT, (u32)start, size, align, 1);
}

void *MainHeap_Alloc(u32 size)
{
    return (void *)Heap_Alloc(&gHeap, size);
}

/* Allocates from the top of the heap. */
void *MainHeap_AllocTop(u32 size)
{
    return (void *)Heap_AllocTopAligned(&gHeap, size);
}

u8 *MainHeap_MaxAlloc(u32 size)
{
    return (u8 *)Heap_MaxAlloc(&gHeap, size);
}

/* The address of a handle's block. */
void *MainHeap_GetAddr(void *handle)
{
    return (void *)Heap_GetAddr(&gHeap, (s32)handle);
}

void MainHeap_Free(void *handle)
{
    Heap_Free(&gHeap, (s32)handle);
}

void MainHeap_Compact(void)
{
    Heap_Compact(&gHeap);
}
