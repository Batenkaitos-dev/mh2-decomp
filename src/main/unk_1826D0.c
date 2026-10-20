/*
 * The memory managers (include/heap.h): the handle heap (gHeap)
 * and the linear heaps, with their own byte fill and move.
 */
#include "common.h"
#include "heap.h"
#include "image.h"

#include "ee_stdarg.h"
extern char gDebugMsg[];
extern u8 D_00249A10[];
extern u8 D_002499F0[];
extern s32 gPacketMode;
s32 sceMcInit(void);
void Mem_Fill(void *dst, u8 c, s32 n);
s32 vsprintf(char *buf, const char *fmt, va_list ap); /* vsprintf */
void Heap_Fill(void *dst, u8 c, s32 n);
void Heap_Move(void *dst, void *src, s32 n);
u32 Heap_FreeSize(Heap *h);
s32 Heap_NewRecord(Heap *h);
void Heap_Link(Heap *h, s32 i);
void Heap_Unlink(Heap *h, s32 i);
u32 Heap_Compact(Heap *h);
s32 Heap_AllocTop(Heap *h, u32 size, s32 align);

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
    heap->first = 0xFFFF;
    Heap_Fill(blocks, 0, count * 0x10);
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
    if (slot == 0xFFFF) {
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
    if (slot == 0xFFFF) {
        return 0;
    }
    if (heap->first == 0xFFFF) {
        return Heap_AllocTopAligned(heap, size);
    }
    blocks = heap->blocks;
    block = &blocks[heap->first];
    if (heap->upward) {
        while (block->next != 0xFFFF) {
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
        while (block->next != 0xFFFF) {
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

    if (heap->first == 0xFFFF) {
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
        while (block->next != 0xFFFF) {
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
        while (block->next != 0xFFFF) {
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
 * The index of heap's first free record (size 0), cleared; 0xFFFF when
 * every record is in use.
 */
s32 Heap_NewRecord(Heap *heap)
{
    s32 i;

    for (i = 0; i < heap->count; i++) {
        if (heap->blocks[i].size == 0) {
            Heap_Fill(heap->blocks + i, 0, 0x10);
            return i;
        }
    }
    return 0xFFFF;
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
    prev = 0xFFFF;

    if (next == 0xFFFF) {
        heap->first = slot;
        block->prev = 0xFFFF;
        block->next = 0xFFFF;
        return;
    }
    neighbor = &heap->blocks[next];
    if (heap->upward) {
        if (neighbor->addr > block->addr) { /* compiled-out check */
        } else {
            while (neighbor->addr < block->addr) {
                prev = next;
                next = neighbor->next;
                if (next == 0xFFFF) {
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
                if (next == 0xFFFF) {
                    break;
                }
                neighbor = &heap->blocks[next];
            }
        }
    }
    block->prev = prev;
    block->next = next;
    if (prev != 0xFFFF) {
        neighbor = heap->blocks + prev;
        neighbor->next = slot;
    }
    if (next != 0xFFFF) {
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

    if (block->prev != 0xFFFF) {
        neighbor = blocks + block->prev;
        neighbor->next = block->next;
    } else {
        heap->first = block->next;
        if (heap->first == 0xFFFF) {
            heap->cur = heap->base;
        }
    }
    if (block->next != 0xFFFF) {
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

/* The address of pixel (x, y) of an image, 0 outside it. */
u8 *Image_PixelAddr(s32 x, s32 y, Image *img)
{
    if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return NULL;
    }
    if (img->bpp == 0) {
        if (img->flags & 0x40) {
            return img->pixels + img->pitch * y + x;
        }
        return img->pixels + img->pitch * y + x / 2;
    }
    return img->pixels + img->pitch * y + x * img->bpp;
}

/* Writes px->color at (px->x, px->y): as a palette index in an indexed
 * image (a nibble of 4-bit images), else scaled into each channel. 0 when
 * the pixel is outside the image. */
s32 Image_PutPixel(Image *img, ImagePixel *pixel)
{
    u8 *dst = Image_PixelAddr(pixel->x, pixel->y, img);
    u32 packed;
    u32 value;
    u32 keep;
    u32 put;
    s32 red;
    s32 green;
    s32 blue;
    s32 alpha;

    if (dst == NULL) {
        return 0;
    }
    if (img->flags & IMAGE_INDEXED) {
        switch (img->bpp) {
        case 4:
            *(u32 *)dst = pixel->color;
            break;
        case 2:
            *(u16 *)dst = pixel->color;
            break;
        case 1:
            *dst = pixel->color;
            break;
        case 0:
            if (img->flags & IMAGE_8BIT) {
                *dst = pixel->color;
            } else {
                value = *dst;
                put = pixel->color;
                if ((pixel->x & 1) ^ (s32)((img->flags & IMAGE_NIBBLE_HI) != 0)) {
                    keep = value & 0xF0;
                    put = put & 0xF;
                } else {
                    keep = value & 0xF;
                    put = (put & 0xF) << 4;
                }
                *dst = keep | put;
            }
            break;
        }
    } else {
        value = pixel->color;
        red = (value >> 16) & 0xFF;
        green = (value >> 8) & 0xFF;
        blue = value & 0xFF;
        alpha = (value >> 24) & 0xFF;
        packed = ((red * img->r.mask / 0xFF) << img->r.shift) | ((green * img->g.mask / 0xFF) << img->g.shift) |
            ((blue * img->b.mask / 0xFF) << img->b.shift) | ((alpha * img->a.mask / 0xFF) << img->a.shift);
        switch (img->bpp) {
        case 2:
            *(u16 *)dst = packed;
            break;
        case 3:
            dst[0] = packed;
            dst[1] = packed >> 8;
            dst[2] = packed >> 16;
            break;
        case 4:
            *(u32 *)dst = packed;
            break;
        }
    }
    return 1;
}

/*
 * Writes color (0xAARRGGBB, or a palette index) at (x, y) of img through
 * Image_PutPixel.
 */
void Image_SetPixel(Image *img, s32 x, s32 y, u32 color)
{
    ImagePixel pixel;

    pixel.x = x;
    pixel.y = y;
    pixel.color = color;
    Image_PutPixel(img, &pixel);
}

/* The colour of pixel (x, y) as 0xAARRGGBB: the palette index of an indexed
 * image, else each channel scaled to 8 bits (alpha 0xFF when absent). */
u32 Image_GetPixel(s32 x, s32 y, Image *img)
{
    u8 *src = Image_PixelAddr(x, y, img);
    u32 raw;
    u32 nibblePair;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha;

    if (src == NULL) {
        return 0;
    }
    if (img->flags & IMAGE_INDEXED) {
        switch (img->bpp) {
        case 0:
            if (img->flags & IMAGE_8BIT) {
                raw = *src;
            } else {
                nibblePair = *src;
                if ((x & 1) ^ (s32)((img->flags & IMAGE_NIBBLE_HI) != 0)) {
                    raw = nibblePair & 0xF;
                } else {
                    raw = (nibblePair >> 4) & 0xF;
                }
            }
            break;
        case 1:
            raw = *src;
            break;
        case 2:
            raw = *(u16 *)src;
            break;
        case 4:
            raw = *(u32 *)src;
            break;
        }
        return raw;
    }
    switch (img->bpp) {
    case 2:
        raw = *(u16 *)src;
        break;
    case 3:
        raw = src[0] | (src[2] << 16 | src[1] << 8);
        break;
    case 4:
        raw = *(u32 *)src;
        break;
    }
    if (img->a.bits) {
        alpha = ((img->a.mask & (raw >> img->a.shift)) * 0xFF) / (u32)img->a.mask;
    } else {
        alpha = 0xFF;
    }
    if (img->r.bits) {
        red = ((img->r.mask & (raw >> img->r.shift)) * 0xFF) / (u32)img->r.mask;
    } else {
        red = 0;
    }
    if (img->g.bits) {
        green = ((img->g.mask & (raw >> img->g.shift)) * 0xFF) / (u32)img->g.mask;
    } else {
        green = 0;
    }
    if (img->b.bits) {
        blue = ((img->b.mask & (raw >> img->b.shift)) * 0xFF) / (u32)img->b.mask;
    } else {
        blue = 0;
    }
    return (alpha << 24) | (red << 16) | (green << 8) | blue;
}

/*
 * Copies the pixels of src into dst over dst's width and height,
 * converting each through 0xAARRGGBB (Image_GetPixel, Image_SetPixel).
 * Returns 1. Seen: the texture loaders.
 */
s32 Image_Convert(Image *dst, Image *src)
{
    s32 y;
    s32 x;

    for (y = 0; y < dst->height; y++) {
        for (x = 0; x < dst->width; x++) {
            Image_SetPixel(dst, x, y, Image_GetPixel(x, y, src));
        }
    }
    return 1;
}

/*
 * Sets up the memory card library: calls sceMcInit until it returns
 * something other than -0x65, then clears a 0x64-byte work area and two
 * 0xC-byte records (no other use of either in src). Returns 1. Called at
 * the end of the system's display and DMA set-up.
 * Kept for the match: the wait is a switch on sceMcInit's result. Its
 * case makes the code after the loop a branch target, which the compiler
 * schedules as the game has it; a do-while orders the address loads of
 * the first Mem_Fill differently.
 */
s32 Sys_InitCard(void)
{
    s32 i;
    u8 *record;

    for (;;) {
        switch (sceMcInit()) {
        case -0x65:
            continue;
        }
        break;
    }
    Mem_Fill(D_00249A10, 0, 0x64);
    for (i = 0, record = D_002499F0; i < 2; i++, record += 0xC) {
        Mem_Fill(record, 0, 0xC);
    }
    return 1;
}

/* Sets gPacketMode to 1 and returns 1. */
s32 Packet_SetMode1Ok(void)
{
    gPacketMode = 1;
    return 1;
}
