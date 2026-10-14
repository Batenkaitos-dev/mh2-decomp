/*
 * Capcom memory helpers in the boot ELF: byte fill and copy, wrappers over the
 * heap object at 0x219110, and the heap-block calls the movie player uses
 * (src/sub_main/movie.c: allocate, address of, free).
 */
#include "common.h"
#include "syswork.h"
#include "heap.h"

extern u8 gSysHeap[];
extern s32 gFlipCountdown;

void *LinearHeap_Alloc(struct LinearHeap *heap, u32 size, s32 flag);
s32 LinearHeap_GetMark(struct LinearHeap *h, s32 side, struct LinearMark *mark);
void LinearHeap_Release(LinearHeap *h, LinearMark *mark);
void *MainHeap_Alloc(u32 size);
void *MainHeap_AllocTop(u32 size);
void *MainHeap_GetAddr(void *handle);
void MainHeap_Free(void *handle);
void MainHeap_Compact(void);
s32 Dma_FlushQueue(void);
void Model_RelocateAll(void);
void Mem_Compact(void);
void *Mem_Alloc(u32 size, s32 flag);
void Packet_Reset(void);

void Mem_Fill(void *dst, u8 c, s32 n)
{
    u8 *d = dst;
    s32 i;

    for (i = 0; i < n; i++) {
        *d++ = c;
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
    s8 *d = dst;
    s8 *s = src;
    s8 c;

    for (i = 0; i < n; i++) {
        if (c = *s++) {
        }
        *d++ = c;
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

void SysHeap_Release(void *p)
{
    LinearHeap_Release((LinearHeap *) gSysHeap, p);
}

void *SysHeap_AllocTop(u32 size)
{
    return LinearHeap_Alloc((struct LinearHeap *)gSysHeap, size, 1);
}

/* Allocates a heap block; on failure, calls Mem_Compact and retries. */
void *Mem_Alloc(u32 size, s32 flag)
{
    void *p = MainHeap_Alloc(size);

    if (p == NULL) {
        Mem_Compact();
        p = MainHeap_AllocTop(size);
    }
    return p;
}

void Mem_Free(void *handle)
{
    MainHeap_Free(handle);
}

void *Mem_GetAddr(void *handle)
{
    return MainHeap_GetAddr(handle);
}

void Mem_Compact(void)
{
    gFlipCountdown = 2;
    Dma_FlushQueue();
    MainHeap_Compact();
    Model_RelocateAll();
}

void Packet_InitBuffers(void)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        gSysWork.buf[i] = Mem_Alloc(0xA0000, 1);
    }
    Packet_Reset();
}
