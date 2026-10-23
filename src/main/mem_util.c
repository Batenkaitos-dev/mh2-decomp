/*
 * Capcom memory helpers in the boot ELF: byte fill and copy, wrappers over the
 * heap object gSysHeap, and the heap-block calls the movie player uses
 * (allocate, address of, free).
 */
#include "common.h"
#include "syswork.h"
#include "heap.h"
#include "mem_util.h"
#include "model.h"

extern u8 gSysHeap[];
extern s32 gFlipCountdown;

/* Sets n bytes at dst to value. */
void Mem_Fill(void *dst, u8 value, s32 n)
{
    u8 *cursor = dst;
    s32 i;

    for (i = 0; i < n; i++) {
        *cursor++ = value;
    }
}

/*
 * Byte copy. The empty `if` on each byte read keeps the source pointer's
 * old value in a register of its own before every step (`move a3, a1; lb
 * t1, 0(a3); addiu a1, a3, 1`), as in Heap_Move.
 */
void Mem_Copy(void *dst, void *src, s32 n)
{
    s32 i;
    s8 *dstCursor = dst;
    s8 *srcCursor = src;
    s8 value;

    for (i = 0; i < n; i++) {
        if (value = *srcCursor++) {
        }
        *dstCursor++ = value;
    }
}

void *SysHeap_Alloc(u32 size)
{
    return LinearHeap_Alloc((struct LinearHeap *)gSysHeap, size, 0);
}

s32 SysHeap_GetMark(void *mark)
{
    return LinearHeap_GetMark((struct LinearHeap *)gSysHeap, 0, (struct LinearMark *)mark);
}

void SysHeap_Release(void *mark)
{
    LinearHeap_Release((LinearHeap *) gSysHeap, mark);
}

void *SysHeap_AllocTop(u32 size)
{
    return LinearHeap_Alloc((struct LinearHeap *)gSysHeap, size, 1);
}

/*
 * Allocates a block of size bytes from the main heap; when none is free,
 * compacts memory (Mem_Compact) and allocates from the heap's top. flag
 * is not used. Returns the block's handle (Mem_GetAddr gives its
 * address), NULL when it still fails.
 */
void *Mem_Alloc(u32 size, s32 flag)
{
    void *handle = MainHeap_Alloc(size);

    if (handle == NULL) {
        Mem_Compact();
        handle = MainHeap_AllocTop(size);
    }
    return handle;
}

void Mem_Free(void *handle)
{
    MainHeap_Free(handle);
}

void *Mem_GetAddr(void *handle)
{
    return MainHeap_GetAddr(handle);
}

/*
 * Compacts the main heap: holds the flip for 2 frames (gFlipCountdown),
 * sends and waits for the queued DMA (Dma_FlushQueue), moves the heap's
 * blocks together (MainHeap_Compact), then fixes the models' pointers to
 * their moved blocks (Model_RelocateAll).
 */
void Mem_Compact(void)
{
    gFlipCountdown = 2;
    Dma_FlushQueue();
    MainHeap_Compact();
    Model_RelocateAll();
}

