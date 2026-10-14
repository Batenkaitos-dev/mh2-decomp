/*
 * A small heap: blocks {size, addr, next} kept in address order between
 * a head and a tail record, optionally guarded by one semaphore.
 */
#include "common.h"
#include "sif.h"
#include "arena.h"


extern s32 gNetSema;

s32 WaitSema(s32 sema);
s32 SignalSema(s32 sema);
void *memmove(void *dst, void *src, u32 n);
void Arena_Lock(Arena *h);
void Arena_Unlock(Arena *h);
static s32 Arena_AllocBlock(Arena *h, s32 size);
static void Arena_FreeBlock(Arena *h, s32 addr);

/* Turns every '/' of a path into a backslash. */
s32 Path_ToBackslashes(char *path)
{
    while (*path != 0) {
        if (*path == '/') {
            *path = 0x5C;
        }
        path++;
    }
    return 0;
}

void Net_DebugLog(s32 a, s32 b, s32 c, char *d, ...)
{
}

/* Swaps the two bytes of a u16. */
u16 Swap16(s32 x)
{
    u32 v = x & 0xFFFF;

    return ((v << 8 & 0xFF00) | (v >> 8 & 0xFF)) & 0xFFFF;
}



void *memset(void *dst, s32 c, u32 n);
void Arena_InitSema(void *h)
{
    s32 sema[6];

    if ((*(u16 *)((u8 *)h + 0x14) != 0) && (gNetSema < 0)) {
        memset(sema, 0, 0x18U);
        sema[2] = 1;
        sema[1] = 1;
        gNetSema = CreateSema((SemaParam *)sema);
    }
}

void Arena_Lock(Arena *h)
{
    if (h->locked != 0) {
        WaitSema(gNetSema);
    }
}

void Arena_Unlock(Arena *h)
{
    if (h->locked != 0) {
        SignalSema(gNetSema);
    }
}

/* Sets up an arena: its block records cleared, a head and a tail record around the whole block, the memory cleared. */
void Arena_Init(Arena *h)
{
    if (h->align > 0 && h->base != 0 && h->size > 0 && h->recs != NULL && h->nrecs >= 2 && h->locked <= 1) {
        Arena_InitSema(h);
        memset(h->recs, 0, h->nrecs * sizeof(ArenaBlk));
        h->head = h->recs;
        h->tail = h->recs + 1;
        h->head->size = h->align;
        h->head->addr = h->base - h->align;
        h->head->next = h->tail;
        h->tail->size = h->align;
        h->tail->addr = h->base + h->size;
        h->tail->next = NULL;
        memset((void *)h->base, 0, (u32)h->size);
        h->unk20 = 0;
        h->unk24 = 0;
    }
}

/* A free block record. */
static ArenaBlk *Arena_NewRecord(Arena *h)
{
    s32 i;

    for (i = 0; i < h->nrecs; i++) {
        if (h->recs[i].size == 0) {
            return &h->recs[i];
        }
    }
    return NULL;
}

static ArenaBlk *Arena_FindBlock(Arena *h, s32 addr)
{
    ArenaBlk *b = h->head;

    for (;;) {
        if (b->addr == addr) {
            return b;
        }
        b = b->next;
        if (b == h->tail) {
            return NULL;
        }
    }
}

static s32 Arena_AllocBlock(Arena *h, s32 size)
{
    ArenaBlk *b;
    ArenaBlk *n;
    ArenaBlk *r;
    s32 rem;

    if (size == 0) {
        return 0;
    }
    rem = size % h->align;
    if (rem != 0) {
        size += h->align - rem;
    }
    b = h->head;
    for (;;) {
        n = b->next;
        if (size <= n->addr - (b->addr + b->size)) {
            r = Arena_NewRecord(h);
            if (r == NULL) {
                return 0;
            }
            r->size = size;
            r->addr = b->addr + b->size;
            r->next = b->next;
            b->next = r;
            return r->addr;
        }
        if (n == h->tail) {
            return 0;
        }
        b = n;
    }
}

void *Arena_Alloc(Arena *h, s32 size)
{
    s32 addr;

    Arena_Lock(h);
    addr = Arena_AllocBlock(h, size);
    Arena_Unlock(h);
    return (void *) addr;
}

static void Arena_FreeBlock(Arena *h, s32 addr)
{
    ArenaBlk *prev;
    ArenaBlk *b;
    ArenaBlk *tail;

    prev = h->head;
    tail = h->tail;
    b = prev;
    for (;;) {
        if (b->next == tail) {
            return;
        }
        b = prev->next;
        if (b->addr == addr) {
            b->size = 0;
            b->addr = 0;
            prev->next = b->next;
            b->next = NULL;
            return;
        }
        prev = b;
    }
}

void Arena_Free(Arena *h, void *addr)
{
    Arena_Lock(h);
    Arena_FreeBlock(h, (s32) addr);
    Arena_Unlock(h);
}

/* Resizes the block at addr (a new block, the contents copied): addr 0 allocates, size 0 frees. */
void *Arena_Realloc(Arena *h, void *addr, s32 size)
{
    s32 rem;
    ArenaBlk *b;
    s32 old;
    void *naddr;

    rem = size % h->align;
    if (rem != 0) {
        size += h->align - rem;
    }
    if (addr == 0) {
        return Arena_Alloc(h, size);
    }
    if (size == 0) {
        Arena_Free(h, addr);
        return NULL;
    }
    Arena_Lock(h);
    b = Arena_FindBlock(h, (s32) addr);
    if (b == NULL) {
        Arena_Unlock(h);
        return NULL;
    }
    old = b->size;
    naddr = (void *)Arena_AllocBlock(h, size);
    if (naddr == NULL) {
        Arena_Unlock(h);
        return NULL;
    }
    Arena_FreeBlock(h, (s32) addr);
    if (old < size) {
        memmove(naddr, addr, old);
    } else {
        memmove(naddr, addr, size);
    }
    Arena_Unlock(h);
    return naddr;
}
