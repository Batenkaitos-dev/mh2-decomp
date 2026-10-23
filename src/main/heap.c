/*
 * The memory managers: the handle heap (gHeap's kind: blocks found by
 * handle, kept in address order and compacted when a request doesn't fit)
 * and the linear heaps (allocated from both ends, released to a mark),
 * with their own byte fill and move, and Debug_Sprintf.
 */
#include "common.h"
#include "heap.h"
#include "image.h"

#include "ee_stdarg.h"
extern char gDebugMsg[];

s32 vsprintf(char *buf, const char *fmt, va_list ap); /* vsprintf */

/*
 * Sets up heap over size bytes from start, with count block records in
 * blocks (cleared) and blocks aligned to align (a power of two). upward
 * 1 places blocks from the start up (base is start aligned up), 0 from
 * the end down (base is the end aligned down). Seen: gHeap, with
 * HEAP_BLOCK_COUNT records.
 */
void Heap_Init(Heap *heap, HeapBlock *blocks, s32 count, u32 start, u32 size, s32 align, s32 upward)
{
    heap->count = count;
    heap->blocks = blocks;
    heap->size = size;
    heap->upward = upward;
    heap->align = align;
    if (upward) {
        heap->base = HEAP_ALIGN(start, align);
    } else {
        heap->base = ~(align - 1) & (start + size);
    }
    heap->cur = heap->base;
    heap->used = 0;
    heap->reserved = 0;
    heap->first = HEAP_NONE;
    Heap_Fill(blocks, 0, count * sizeof(HeapBlock));
}

/* Heap_AllocTop with the heap's own alignment. */
s32 Heap_AllocTopAligned(Heap *heap, u32 size)
{
    return Heap_AllocTop(heap, size, heap->align);
}

/*
 * Allocates size bytes where the next block goes (heap->cur): upward
 * heaps place it at cur and move cur past it, aligned up to align;
 * downward heaps place it below cur, aligned down, and move cur to it.
 * The block is recorded in a new record and linked into the address
 * list. Returns its handle (record index + 1), or 0 when size is 0, when
 * the free space is not more than size + align, or when no record is
 * free.
 * The end of the new block is computed as a pointer, so that the
 * addition has the address first, as in the game.
 */
s32 Heap_AllocTop(Heap *heap, u32 size, s32 align)
{
    s32 slot;

    if (Heap_FreeSize(heap) <= size + align) {
        return 0;
    }
    if (size == 0) {
        return 0;
    }
    slot = Heap_NewRecord(heap);
    if (slot == HEAP_NONE) {
        return 0;
    }
    heap->blocks[slot].flags = 0;
    heap->blocks[slot].size = size;
    heap->blocks[slot].align = heap->align;
    if (heap->upward) {
        heap->blocks[slot].addr = heap->cur;
        heap->cur = HEAP_ALIGN((u32)((u8 *)heap->blocks[slot].addr + size), align);
    } else {
        heap->blocks[slot].addr = ~(align - 1) & (heap->cur - size);
        heap->cur = heap->blocks[slot].addr;
    }
    heap->used += size;
    Heap_Link(heap, slot);
    return slot + 1;
}

/* A block of size bytes in the first gap between blocks that holds it
 * (upward heaps from the lowest address, downward ones from the highest),
 * else after the last block (Heap_AllocTopAligned); returns its handle. */
s32 Heap_Alloc(Heap *heap, u32 size)
{
    s32 slot;
    u32 need;
    HeapBlock *block;
    HeapBlock *blocks;
    HeapBlock *next;
    u32 addr;

    need = HEAP_ALIGN(size, heap->align);
    slot = Heap_NewRecord(heap);
    if (slot == HEAP_NONE) {
        return 0;
    }
    if (heap->first == HEAP_NONE) {
        return Heap_AllocTopAligned(heap, size);
    }
    blocks = heap->blocks;
    block = &blocks[heap->first];
    if (heap->upward) {
        while (block->next != HEAP_NONE) {
            next = &blocks[block->next];
            addr = HEAP_ALIGN(block->addr + block->size, heap->align);
            if (need <= next->addr - addr) {
                heap->blocks[slot].flags = 0;
                heap->blocks[slot].size = size;
                heap->blocks[slot].align = heap->align;
                heap->blocks[slot].addr = addr;
                heap->used += size;
                Heap_Link(heap, slot);
                return slot + 1;
            }
            block = next;
        }
        addr = HEAP_ALIGN(block->addr + block->size, heap->align);
        if (need <= heap->cur - addr) {
            heap->blocks[slot].flags = 0;
            heap->blocks[slot].size = size;
            heap->blocks[slot].align = heap->align;
            heap->blocks[slot].addr = addr;
            heap->used += size;
            Heap_Link(heap, slot);
            return slot + 1;
        }
    } else {
        while (block->next != HEAP_NONE) {
            next = &blocks[block->next];
            if (need <= block->addr - HEAP_ALIGN(next->addr + next->size, heap->align)) {
                heap->blocks[slot].flags = 0;
                heap->blocks[slot].size = size;
                heap->blocks[slot].align = heap->align;
                heap->blocks[slot].addr = block->addr - need;
                heap->used += size;
                Heap_Link(heap, slot);
                return slot + 1;
            }
            block = next;
        }
        if (need <= block->addr - heap->cur) {
            heap->blocks[slot].flags = 0;
            heap->blocks[slot].size = size;
            heap->blocks[slot].align = heap->align;
            heap->blocks[slot].addr = block->addr - need;
            heap->used += size;
            Heap_Link(heap, slot);
            return slot + 1;
        }
    }
    return Heap_AllocTopAligned(heap, size);
}

/*
 * The address of size bytes (rounded up to the alignment) at the far end
 * of heap: base + size - that for an upward heap, the low end (base -
 * size) for a downward one. When the free space is short the heap is
 * compacted first; 0 when it is still short. No block is recorded.
 * Seen: MainHeap_MaxAlloc.
 */
u32 Heap_MaxAlloc(Heap *heap, u32 size)
{
    u32 need = HEAP_ALIGN(size, heap->align);

    if (Heap_FreeSize(heap) < need) {
        Heap_Compact(heap);
        need = HEAP_ALIGN(need, heap->align);
        if (Heap_FreeSize(heap) < need) {
            return 0;
        }
    }
    if (heap->upward) {
        return heap->base + heap->size - need;
    }
    return heap->base - heap->size;
}

/* The address of handle's block; 0 for handle 0 or past the records. */
u32 Heap_GetAddr(Heap *heap, s32 handle)
{
    s32 slot = handle - 1;

    if (slot >= heap->count || handle == 0) {
        return 0;
    }
    return heap->blocks[slot].addr;
}

/*
 * Frees handle's block: takes its size off heap->used, clears its record
 * and unlinks it. Returns 1, or 0 when handle is past the records or the
 * block is already free.
 */
s32 Heap_Free(Heap *heap, s32 handle)
{
    s32 slot = handle - 1;
    u32 size;

    if (slot >= heap->count) {
        return 0;
    }
    size = heap->blocks[slot].size;
    if (size == 0) {
        return 0;
    }
    heap->used -= size;
    heap->blocks[slot].size = 0;
    heap->blocks[slot].addr = 0;
    Heap_Unlink(heap, slot);
    return 1;
}

/* Moves the blocks together (towards the start or the end of the heap), in
 * address order; returns where the next block goes. */
u32 Heap_Compact(Heap *heap)
{
    HeapBlock *block;
    HeapBlock *following;
    u32 addr;

    if (heap->first == HEAP_NONE) {
        heap->cur = heap->base;
        return heap->cur;
    }
    block = heap->blocks + heap->first;
    if (heap->upward) {
        addr = HEAP_ALIGN(heap->base, heap->align);
        if (addr != block->addr) {
            Heap_Move((u8 *)addr, (u8 *)block->addr, block->size);
            block->addr = addr;
        }
        while (block->next != HEAP_NONE) {
            following = heap->blocks + block->next;
            addr = HEAP_ALIGN(block->addr + block->size, heap->align);
            if (addr != following->addr) {
                Heap_Move((u8 *)addr, (u8 *)following->addr, following->size);
                following->addr = addr;
            }
            block = following;
        }
        heap->cur = HEAP_ALIGN(block->addr + block->size, heap->align);
    } else {
        addr = ~(heap->align - 1) & (heap->base - block->size);
        if (addr != block->addr) {
            Heap_Move((u8 *)addr, (u8 *)block->addr, block->size);
            block->addr = addr;
        }
        while (block->next != HEAP_NONE) {
            following = heap->blocks + block->next;
            addr = ~(heap->align - 1) & (block->addr - following->size);
            if (addr != following->addr) {
                Heap_Move((u8 *)addr, (u8 *)following->addr, following->size);
                following->addr = addr;
            }
            block = following;
        }
        heap->cur = block->addr;
    }
    return heap->cur;
}

/*
 * The bytes free between heap->cur and the heap's far end, less
 * heap->reserved.
 */
u32 Heap_FreeSize(Heap *heap)
{
    if (heap->upward) {
        return heap->base + heap->size - heap->cur - heap->reserved;
    }
    return heap->cur - (heap->base - heap->size) - heap->reserved;
}

/*
 * The index of heap's first free record (size 0), cleared; HEAP_NONE when
 * every record is in use.
 */
s32 Heap_NewRecord(Heap *heap)
{
    s32 i;

    for (i = 0; i < heap->count; i++) {
        if (heap->blocks[i].size == 0) {
            Heap_Fill(heap->blocks + i, 0, sizeof(HeapBlock));
            return i;
        }
    }
    return HEAP_NONE;
}

/* Accessors of the block records (inlined; see Heap_Unlink). */
static inline HeapBlock *Heap_Blocks(Heap *heap)
{
    return heap->blocks;
}

static inline HeapBlock *Heap_NextOf(HeapBlock *blocks, HeapBlock *block)
{
    return blocks + block->next;
}

/*
 * Links block slot into the address-ordered list: in front of the first
 * block above it (upward heaps) or below it (downward heaps).
 */
void Heap_Link(Heap *heap, s32 slot)
{
    s32 next;
    HeapBlock *block;
    HeapBlock *neighbor;
    s32 prev;

    block = &heap->blocks[slot];
    next = heap->first;
    prev = HEAP_NONE;

    if (next == HEAP_NONE) {
        heap->first = slot;
        block->prev = HEAP_NONE;
        block->next = HEAP_NONE;
        return;
    }
    neighbor = &heap->blocks[next];
    if (heap->upward) {
        if (neighbor->addr > block->addr) { /* compiled-out check */
        } else {
            while (neighbor->addr < block->addr) {
                prev = next;
                next = neighbor->next;
                if (next == HEAP_NONE) {
                    break;
                }
                neighbor = &heap->blocks[next];
            }
        }
    } else {
        if (block->addr > neighbor->addr) { /* compiled-out check */
        } else {
            while (neighbor->addr > block->addr) {
                prev = next;
                next = neighbor->next;
                if (next == HEAP_NONE) {
                    break;
                }
                neighbor = &heap->blocks[next];
            }
        }
    }
    block->prev = prev;
    block->next = next;
    if (prev != HEAP_NONE) {
        neighbor = heap->blocks + prev;
        neighbor->next = slot;
    }
    if (next != HEAP_NONE) {
        neighbor = heap->blocks + next;
        neighbor->prev = slot;
    }
}

/*
 * Unlinks block slot from the address list; when the list becomes empty,
 * heap->cur goes back to base. The two static inline accessors above
 * (the blocks of a heap, the next block of a block) are what give the
 * game's registers and operand order.
 */
void Heap_Unlink(Heap *heap, s32 slot)
{
    HeapBlock *blocks = Heap_Blocks(heap);
    HeapBlock *block = blocks + slot;
    HeapBlock *neighbor;

    if (block->prev != HEAP_NONE) {
        neighbor = blocks + block->prev;
        neighbor->next = block->next;
    } else {
        heap->first = block->next;
        if (heap->first == HEAP_NONE) {
            heap->cur = heap->base;
        }
    }
    if (block->next != HEAP_NONE) {
        blocks = heap->blocks;
        Heap_NextOf(blocks, block)->prev = block->prev;
    }
}

/*
 * Sets up heap over size bytes (rounded up to align) from start, both
 * ends aligned to align, with nothing allocated. Returns 1.
 */
s32 LinearHeap_Init(LinearHeap *heap, void *start, u32 size, s32 align)
{
    heap->start = (s32) start;
    heap->align = align;
    size = HEAP_ALIGN(size, align);
    heap->lo = HEAP_ALIGN((u32)heap->start, heap->align); /* the no-op cast orders the loads */
    heap->hi = HEAP_ALIGN(heap->start + size, heap->align);
    heap->pos[0] = heap->lo;
    heap->pos[1] = heap->hi;
    return 1;
}

/*
 * Allocates size bytes (rounded up to the alignment) from the low end
 * (pos[0], upwards) or, with fromTop, from the high end (pos[1],
 * downwards). Returns the block, or 0 when the two ends would cross.
 */
void *LinearHeap_Alloc(LinearHeap *heap, u32 size, s32 fromTop)
{
    u32 end;
    u32 addr;

    size = HEAP_ALIGN(size, heap->align);
    end = heap->pos[0] + size;
    if (heap->pos[1] < end) {
        return 0;
    }
    if (fromTop) {
        heap->pos[1] -= size;
        return (void *)heap->pos[1];
    }
    addr = heap->pos[0];
    heap->pos[0] = end;
    return (void *)addr;
}

/*
 * Saves heap's position on side (0 the low end, 1 the high end) in mark,
 * for LinearHeap_Release. Returns 1.
 */
s32 LinearHeap_GetMark(LinearHeap *heap, s32 side, LinearMark *mark)
{
    mark->pos = heap->pos[side];
    mark->side = side;
    return 1;
}

/*
 * Frees what was allocated on mark's side since LinearHeap_GetMark saved
 * mark, by putting that end back.
 */
void LinearHeap_Release(LinearHeap *heap, LinearMark *mark)
{
    heap->pos[mark->side] = mark->pos;
}

/*
 * Formats a message into gDebugMsg (vsprintf). Returns 1. Seen: the
 * lobby's image loaders' error messages.
 */
s32 Debug_Sprintf(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    vsprintf(gDebugMsg, fmt, args);
    return 1;
}

/* Sets size bytes from dst to value (the heap's own memset). */
void Heap_Fill(void *dst, u8 value, s32 size)
{
    s32 i;
    u8 *out = dst;

    for (i = 0; i < size; i++) {
        *out++ = value;
    }
}

/*
 * Copies size bytes from src to dst, safely when they overlap (the
 * heap's own memmove): backwards when dst lies inside the source.
 * Kept for the match: the empty if on each byte read keeps the source
 * pointer's old value in a register of its own before every step, as in
 * the game; the test of the byte compiles to nothing.
 */
void Heap_Move(void *dst, void *src, s32 size)
{
    s32 i;
    s8 *dstByte = dst;
    s8 *srcByte = src;
    s8 byte;

    if (srcByte < dstByte && dstByte < srcByte + size) {
        dstByte += size - 1;
        srcByte += size - 1;
        for (i = 0; i < size; i++) {
            if (byte = *srcByte--) {
            }
            *dstByte-- = byte;
        }
    } else {
        for (i = 0; i < size; i++) {
            if (byte = *srcByte++) {
            }
            *dstByte++ = byte;
        }
    }
}
