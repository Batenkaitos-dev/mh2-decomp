/*
 * The main heap's calls (main 0x182670-0x1826D0): each passes gHeap to the
 * handle heap's function of the same name (unk_1826D0.c, include/heap.h).
 * The handles go out as pointers.
 */
#include "common.h"
#include "heap.h"

s32 Heap_Alloc(Heap *h, u32 size);
extern Heap gHeap;
s32 Heap_AllocTopAligned(Heap *h, u32 size);
u32 Heap_MaxAlloc(Heap *h, u32 size);
u32 Heap_GetAddr(Heap *h, s32 handle);
s32 Heap_Free(Heap *h, s32 handle);
u32 Heap_Compact(Heap *h);

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
