/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "heap.h"

s32 Heap_Alloc(Heap *h, u32 size);
extern M2C_UNK gHeap;
s32 Heap_AllocTopAligned(Heap *h, u32 size);
u32 Heap_MaxAlloc(Heap *h, u32 size);
u32 Heap_GetAddr(Heap *h, s32 handle);
s32 Heap_Free(Heap *h, s32 handle);
u32 Heap_Compact(Heap *h);

void *MainHeap_Alloc(u32 size) {
    return (void *) Heap_Alloc((Heap *) &gHeap, size);
}

void *MainHeap_AllocTop(u32 size) {
    return (void *)Heap_AllocTopAligned((Heap *) &gHeap, size);
}

u8 *MainHeap_MaxAlloc(u32 size) {
    return (u8 *)Heap_MaxAlloc((Heap *) &gHeap, size);
}

void *MainHeap_GetAddr(void *handle) {
    return (void *)Heap_GetAddr((Heap *) &gHeap, (s32) handle);
}

void MainHeap_Free(void *handle) {
    Heap_Free((Heap *) &gHeap, (s32) handle);
}

void MainHeap_Compact(void) {
    Heap_Compact((Heap *) &gHeap);
}
