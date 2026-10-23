/*
 * The network code's small helpers and a small heap.
 *
 * The heap (Arena): Arena_Init sets one up over a block of memory, and Arena_Alloc,
 * Arena_Free and Arena_Realloc use it. Its allocations are block records {size, addr,
 * next} kept in address order between a head and a tail record; a new block goes in the
 * first gap that fits. An arena may be guarded by the network's semaphore (gNetSema).
 * The browser's page heap and the lobby's image heap are arenas.
 */
#include "common.h"
#include "sif.h"
#include "arena.h"
#include "libkernl.h"


extern s32 gNetSema;

void *memmove(void *dst, void *src, u32 n);
static s32 Arena_AllocBlock(Arena *arena, s32 size);
static void Arena_FreeBlock(Arena *arena, s32 addr);

/* Turns every '/' of a path into a backslash. */
s32 Path_ToBackslashes(char *path)
{
    while (*path != 0) {
        if (*path == '/') {
            *path = '\\';
        }
        path++;
    }
    return 0;
}

/* The network code's log, compiled out: it does nothing. Its callers pass three numbers
 * before the format and its arguments (seen: 1, 26 or 27, 1). */
void Net_DebugLog(s32 level, s32 id, s32 flags, char *format, ...)
{
}

/* Swaps the two bytes of a u16. */
u16 Swap16(s32 x)
{
    u32 value = x & 0xFFFF;

    return ((value << 8 & 0xFF00) | (value >> 8 & 0xFF)) & 0xFFFF;
}



void *memset(void *dst, s32 c, u32 n);

/* Creates the network's semaphore (gNetSema: one count) for an arena that locks, unless
 * it exists already. */
void Arena_InitSema(Arena *arena)
{
    SemaParam param;

    if ((arena->locked != 0) && (gNetSema < 0)) {
        memset(&param, 0, sizeof(param));
        param.initCount = 1;
        param.maxCount = 1;
        gNetSema = CreateSema(&param);
    }
}

/* Takes the network's semaphore when the arena locks. */
void Arena_Lock(Arena *arena)
{
    if (arena->locked != 0) {
        WaitSema(gNetSema);
    }
}

/* Gives the network's semaphore back when the arena locks. */
void Arena_Unlock(Arena *arena)
{
    if (arena->locked != 0) {
        SignalSema(gNetSema);
    }
}

/* Sets up an arena: its block records cleared, a head and a tail record around the whole
 * block, the memory cleared. Nothing when its fields are not valid. */
void Arena_Init(Arena *arena)
{
    if (arena->align > 0 && arena->base != 0 && arena->size > 0 && arena->recs != NULL && arena->nrecs >= 2 && arena->locked <= 1) {
        Arena_InitSema(arena);
        memset(arena->recs, 0, arena->nrecs * sizeof(ArenaBlk));
        arena->head = arena->recs;
        arena->tail = arena->recs + 1;
        arena->head->size = arena->align;
        arena->head->addr = arena->base - arena->align;
        arena->head->next = arena->tail;
        arena->tail->size = arena->align;
        arena->tail->addr = arena->base + arena->size;
        arena->tail->next = NULL;
        memset((void *)arena->base, 0, (u32)arena->size);
        arena->unk20 = 0;
        arena->unk24 = 0;
    }
}

/* A free block record (size 0), or NULL when all are in use. */
static ArenaBlk *Arena_NewRecord(Arena *arena)
{
    s32 i;

    for (i = 0; i < arena->nrecs; i++) {
        if (arena->recs[i].size == 0) {
            return &arena->recs[i];
        }
    }
    return NULL;
}

/* The record of the block at addr, or NULL. */
static ArenaBlk *Arena_FindBlock(Arena *arena, s32 addr)
{
    ArenaBlk *blk = arena->head;

    for (;;) {
        if (blk->addr == addr) {
            return blk;
        }
        blk = blk->next;
        if (blk == arena->tail) {
            return NULL;
        }
    }
}

/* Allocates size bytes (rounded up to the arena's alignment) in the first gap between
 * two blocks that fits them, without locking. Returns the address, or 0 when size is 0,
 * no gap fits or no record is free. */
static s32 Arena_AllocBlock(Arena *arena, s32 size)
{
    ArenaBlk *blk;
    ArenaBlk *next;
    ArenaBlk *rec;
    s32 rem;

    if (size == 0) {
        return 0;
    }
    rem = size % arena->align;
    if (rem != 0) {
        size += arena->align - rem;
    }
    blk = arena->head;
    for (;;) {
        next = blk->next;
        if (size <= next->addr - (blk->addr + blk->size)) {
            rec = Arena_NewRecord(arena);
            if (rec == NULL) {
                return 0;
            }
            rec->size = size;
            rec->addr = blk->addr + blk->size;
            rec->next = blk->next;
            blk->next = rec;
            return rec->addr;
        }
        if (next == arena->tail) {
            return 0;
        }
        blk = next;
    }
}

/* Allocates size bytes (Arena_AllocBlock) under the arena's lock; NULL when it can't. */
void *Arena_Alloc(Arena *arena, s32 size)
{
    s32 addr;

    Arena_Lock(arena);
    addr = Arena_AllocBlock(arena, size);
    Arena_Unlock(arena);
    return (void *) addr;
}

/* Frees the block at addr without locking: its record is cleared and unlinked. Nothing
 * when no block is there. */
static void Arena_FreeBlock(Arena *arena, s32 addr)
{
    ArenaBlk *prev;
    ArenaBlk *blk;
    ArenaBlk *tail;

    prev = arena->head;
    tail = arena->tail;
    blk = prev;
    for (;;) {
        if (blk->next == tail) {
            return;
        }
        blk = prev->next;
        if (blk->addr == addr) {
            blk->size = 0;
            blk->addr = 0;
            prev->next = blk->next;
            blk->next = NULL;
            return;
        }
        prev = blk;
    }
}

/* Frees the block at addr (Arena_FreeBlock) under the arena's lock. */
void Arena_Free(Arena *arena, void *addr)
{
    Arena_Lock(arena);
    Arena_FreeBlock(arena, (s32) addr);
    Arena_Unlock(arena);
}

/* Resizes the block at addr (a new block, the contents copied): addr 0 allocates, size 0
 * frees. Returns the new address, or NULL when there is no block at addr or no room. */
void *Arena_Realloc(Arena *arena, void *addr, s32 size)
{
    s32 rem;
    ArenaBlk *blk;
    s32 oldSize;
    void *newAddr;

    rem = size % arena->align;
    if (rem != 0) {
        size += arena->align - rem;
    }
    if (addr == 0) {
        return Arena_Alloc(arena, size);
    }
    if (size == 0) {
        Arena_Free(arena, addr);
        return NULL;
    }
    Arena_Lock(arena);
    blk = Arena_FindBlock(arena, (s32) addr);
    if (blk == NULL) {
        Arena_Unlock(arena);
        return NULL;
    }
    oldSize = blk->size;
    newAddr = (void *)Arena_AllocBlock(arena, size);
    if (newAddr == NULL) {
        Arena_Unlock(arena);
        return NULL;
    }
    Arena_FreeBlock(arena, (s32) addr);
    if (oldSize < size) {
        memmove(newAddr, addr, oldSize);
    } else {
        memmove(newAddr, addr, size);
    }
    Arena_Unlock(arena);
    return newAddr;
}
