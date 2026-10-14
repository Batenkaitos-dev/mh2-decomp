/*
 * 0x1826D0: the memory managers (include/heap.h): the handle heap (gHeap)
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

void Heap_Init(Heap *h, HeapBlock *blocks, s32 count, u32 start, u32 size, s32 align, s32 upward)
{
    h->count = count;
    h->blocks = blocks;
    h->size = size;
    h->upward = upward;
    h->align = align;
    if (upward) {
        h->base = HEAP_ALIGN(start, align);
    } else {
        h->base = ~(align - 1) & (start + size);
    }
    h->cur = h->base;
    h->used = 0;
    h->reserved = 0;
    h->first = 0xFFFF;
    Heap_Fill(blocks, 0, count * 0x10);
}

s32 Heap_AllocTopAligned(Heap *h, u32 size)
{
    return Heap_AllocTop(h, size, h->align);
}

/* The end of the new block is computed as a pointer (the addition then has
 * the address first). */
s32 Heap_AllocTop(Heap *h, u32 size, s32 align)
{
    s32 i;

    if (Heap_FreeSize(h) <= size + align) {
        return 0;
    }
    if (size == 0) {
        return 0;
    }
    i = Heap_NewRecord(h);
    if (i == 0xFFFF) {
        return 0;
    }
    h->blocks[i].flags = 0;
    h->blocks[i].size = size;
    h->blocks[i].align = h->align;
    if (h->upward) {
        h->blocks[i].addr = h->cur;
        h->cur = HEAP_ALIGN((u32)((u8 *)h->blocks[i].addr + size), align);
    } else {
        h->blocks[i].addr = ~(align - 1) & (h->cur - size);
        h->cur = h->blocks[i].addr;
    }
    h->used += size;
    Heap_Link(h, i);
    return i + 1;
}

/* A block of size bytes in the first gap between blocks that holds it
 * (upward heaps from the lowest address, downward ones from the highest),
 * else after the last block (Heap_AllocTopAligned); returns its handle. */
s32 Heap_Alloc(Heap *h, u32 size)
{
    s32 i;
    u32 need;
    HeapBlock *b;
    HeapBlock *blocks;
    HeapBlock *next;
    u32 addr;

    need = HEAP_ALIGN(size, h->align);
    i = Heap_NewRecord(h);
    if (i == 0xFFFF) {
        return 0;
    }
    if (h->first == 0xFFFF) {
        return Heap_AllocTopAligned(h, size);
    }
    blocks = h->blocks;
    b = &blocks[h->first];
    if (h->upward) {
        while (b->next != 0xFFFF) {
            next = &blocks[b->next];
            addr = HEAP_ALIGN(b->addr + b->size, h->align);
            if (need <= next->addr - addr) {
                h->blocks[i].flags = 0;
                h->blocks[i].size = size;
                h->blocks[i].align = h->align;
                h->blocks[i].addr = addr;
                h->used += size;
                Heap_Link(h, i);
                return i + 1;
            }
            b = next;
        }
        addr = HEAP_ALIGN(b->addr + b->size, h->align);
        if (need <= h->cur - addr) {
            h->blocks[i].flags = 0;
            h->blocks[i].size = size;
            h->blocks[i].align = h->align;
            h->blocks[i].addr = addr;
            h->used += size;
            Heap_Link(h, i);
            return i + 1;
        }
    } else {
        while (b->next != 0xFFFF) {
            next = &blocks[b->next];
            if (need <= b->addr - HEAP_ALIGN(next->addr + next->size, h->align)) {
                h->blocks[i].flags = 0;
                h->blocks[i].size = size;
                h->blocks[i].align = h->align;
                h->blocks[i].addr = b->addr - need;
                h->used += size;
                Heap_Link(h, i);
                return i + 1;
            }
            b = next;
        }
        if (need <= b->addr - h->cur) {
            h->blocks[i].flags = 0;
            h->blocks[i].size = size;
            h->blocks[i].align = h->align;
            h->blocks[i].addr = b->addr - need;
            h->used += size;
            Heap_Link(h, i);
            return i + 1;
        }
    }
    return Heap_AllocTopAligned(h, size);
}

u32 Heap_MaxAlloc(Heap *h, u32 size)
{
    u32 need = HEAP_ALIGN(size, h->align);

    if (Heap_FreeSize(h) < need) {
        Heap_Compact(h);
        need = HEAP_ALIGN(need, h->align);
        if (Heap_FreeSize(h) < need) {
            return 0;
        }
    }
    if (h->upward) {
        return h->base + h->size - need;
    }
    return h->base - h->size;
}

u32 Heap_GetAddr(Heap *h, s32 handle)
{
    s32 i = handle - 1;

    if (i >= h->count || handle == 0) {
        return 0;
    }
    return h->blocks[i].addr;
}

s32 Heap_Free(Heap *h, s32 handle)
{
    s32 i = handle - 1;
    u32 size;

    if (i >= h->count) {
        return 0;
    }
    size = h->blocks[i].size;
    if (size == 0) {
        return 0;
    }
    h->used -= size;
    h->blocks[i].size = 0;
    h->blocks[i].addr = 0;
    Heap_Unlink(h, i);
    return 1;
}

/* Moves the blocks together (towards the start or the end of the heap), in
 * address order; returns where the next block goes. */
u32 Heap_Compact(Heap *h)
{
    HeapBlock *b;
    HeapBlock *n;
    u32 addr;

    if (h->first == 0xFFFF) {
        h->cur = h->base;
        return h->cur;
    }
    b = h->blocks + h->first;
    if (h->upward) {
        addr = HEAP_ALIGN(h->base, h->align);
        if (addr != b->addr) {
            Heap_Move((u8 *)addr, (u8 *)b->addr, b->size);
            b->addr = addr;
        }
        while (b->next != 0xFFFF) {
            n = h->blocks + b->next;
            addr = HEAP_ALIGN(b->addr + b->size, h->align);
            if (addr != n->addr) {
                Heap_Move((u8 *)addr, (u8 *)n->addr, n->size);
                n->addr = addr;
            }
            b = n;
        }
        h->cur = HEAP_ALIGN(b->addr + b->size, h->align);
    } else {
        addr = ~(h->align - 1) & (h->base - b->size);
        if (addr != b->addr) {
            Heap_Move((u8 *)addr, (u8 *)b->addr, b->size);
            b->addr = addr;
        }
        while (b->next != 0xFFFF) {
            n = h->blocks + b->next;
            addr = ~(h->align - 1) & (b->addr - n->size);
            if (addr != n->addr) {
                Heap_Move((u8 *)addr, (u8 *)n->addr, n->size);
                n->addr = addr;
            }
            b = n;
        }
        h->cur = b->addr;
    }
    return h->cur;
}

u32 Heap_FreeSize(Heap *h)
{
    if (h->upward) {
        return h->base + h->size - h->cur - h->reserved;
    }
    return h->cur - (h->base - h->size) - h->reserved;
}

s32 Heap_NewRecord(Heap *h)
{
    s32 i;

    for (i = 0; i < h->count; i++) {
        if (h->blocks[i].size == 0) {
            Heap_Fill(h->blocks + i, 0, 0x10);
            return i;
        }
    }
    return 0xFFFF;
}

/* Accessors of the block records (inlined; see Heap_Unlink). */
static inline HeapBlock *Heap_Blocks(Heap *h)
{
    return h->blocks;
}

static inline HeapBlock *Heap_NextOf(HeapBlock *blocks, HeapBlock *b)
{
    return blocks + b->next;
}

/* Links block i into the address-ordered list: in front of the first block
 * above it (upward heaps) or below it (downward heaps). */
void Heap_Link(Heap *h, s32 i)
{
    s32 next;
    HeapBlock *b;
    HeapBlock *n;
    s32 prev;

    b = &h->blocks[i];
    next = h->first;
    prev = 0xFFFF;

    if (next == 0xFFFF) {
        h->first = i;
        b->prev = 0xFFFF;
        b->next = 0xFFFF;
        return;
    }
    n = &h->blocks[next];
    if (h->upward) {
        if (n->addr > b->addr) { /* compiled-out check */
        } else {
            while (n->addr < b->addr) {
                prev = next;
                next = n->next;
                if (next == 0xFFFF) {
                    break;
                }
                n = &h->blocks[next];
            }
        }
    } else {
        if (b->addr > n->addr) { /* compiled-out check */
        } else {
            while (n->addr > b->addr) {
                prev = next;
                next = n->next;
                if (next == 0xFFFF) {
                    break;
                }
                n = &h->blocks[next];
            }
        }
    }
    b->prev = prev;
    b->next = next;
    if (prev != 0xFFFF) {
        n = h->blocks + prev;
        n->next = i;
    }
    if (next != 0xFFFF) {
        n = h->blocks + next;
        n->prev = i;
    }
}

/* Unlinks block i from the address list. The two static inline accessors
 * above (the blocks of a heap, the next block of a block) are what give the
 * original its registers and operand order. */
void Heap_Unlink(Heap *h, s32 i)
{
    HeapBlock *blocks = Heap_Blocks(h);
    HeapBlock *b = blocks + i;
    HeapBlock *n;

    if (b->prev != 0xFFFF) {
        n = blocks + b->prev;
        n->next = b->next;
    } else {
        h->first = b->next;
        if (h->first == 0xFFFF) {
            h->cur = h->base;
        }
    }
    if (b->next != 0xFFFF) {
        blocks = h->blocks;
        Heap_NextOf(blocks, b)->prev = b->prev;
    }
}

s32 LinearHeap_Init(LinearHeap *h, void *start, u32 size, s32 align)
{
    h->start = (s32) start;
    h->align = align;
    size = HEAP_ALIGN(size, align);
    h->lo = HEAP_ALIGN((u32)h->start, h->align); /* the no-op cast orders the loads */
    h->hi = HEAP_ALIGN(h->start + size, h->align);
    h->pos[0] = h->lo;
    h->pos[1] = h->hi;
    return 1;
}

void *LinearHeap_Alloc(LinearHeap *h, u32 size, s32 fromTop)
{
    u32 end;
    u32 ret;

    size = HEAP_ALIGN(size, h->align);
    end = h->pos[0] + size;
    if (h->pos[1] < end) {
        return 0;
    }
    if (fromTop) {
        h->pos[1] -= size;
        return (void *)h->pos[1];
    }
    ret = h->pos[0];
    h->pos[0] = end;
    return (void *)ret;
}

s32 LinearHeap_GetMark(LinearHeap *h, s32 side, LinearMark *mark)
{
    mark->pos = h->pos[side];
    mark->side = side;
    return 1;
}

void LinearHeap_Release(LinearHeap *h, LinearMark *mark)
{
    h->pos[mark->side] = mark->pos;
}

s32 Debug_Sprintf(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(gDebugMsg, fmt, ap);
    return 1;
}

void Heap_Fill(void *dst, u8 c, s32 n)
{
    s32 i;
    u8 *p = dst;

    for (i = 0; i < n; i++) {
        *p++ = c;
    }
}

/*
 * memmove. The empty `if` on each byte read is what keeps the source
 * pointer's old value in a register of its own before every step (`move
 * a3, a1; lb t1, 0(a3); addiu a1, a3, 1`): a test of the byte compiled out.
 */
void Heap_Move(void *dst, void *src, s32 n)
{
    s32 i;
    s8 *d = dst;
    s8 *s = src;
    s8 c;

    if (s < d && d < s + n) {
        d += n - 1;
        s += n - 1;
        for (i = 0; i < n; i++) {
            if (c = *s--) {
            }
            *d-- = c;
        }
    } else {
        for (i = 0; i < n; i++) {
            if (c = *s++) {
            }
            *d++ = c;
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
s32 Image_PutPixel(Image *img, ImagePixel *px)
{
    u8 *p = Image_PixelAddr(px->x, px->y, img);
    u32 v;
    u32 c;
    u32 keep;
    u32 put;
    s32 r;
    s32 g;
    s32 b;
    s32 a;

    if (p == NULL) {
        return 0;
    }
    if (img->flags & IMAGE_INDEXED) {
        switch (img->bpp) {
        case 4:
            *(u32 *)p = px->color;
            break;
        case 2:
            *(u16 *)p = px->color;
            break;
        case 1:
            *p = px->color;
            break;
        case 0:
            if (img->flags & IMAGE_8BIT) {
                *p = px->color;
            } else {
                c = *p;
                put = px->color;
                if ((px->x & 1) ^ (s32)((img->flags & IMAGE_NIBBLE_HI) != 0)) {
                    keep = c & 0xF0;
                    put = put & 0xF;
                } else {
                    keep = c & 0xF;
                    put = (put & 0xF) << 4;
                }
                *p = keep | put;
            }
            break;
        }
    } else {
        c = px->color;
        r = (c >> 16) & 0xFF;
        g = (c >> 8) & 0xFF;
        b = c & 0xFF;
        a = (c >> 24) & 0xFF;
        v = ((r * img->r.mask / 0xFF) << img->r.shift) | ((g * img->g.mask / 0xFF) << img->g.shift) |
            ((b * img->b.mask / 0xFF) << img->b.shift) | ((a * img->a.mask / 0xFF) << img->a.shift);
        switch (img->bpp) {
        case 2:
            *(u16 *)p = v;
            break;
        case 3:
            p[0] = v;
            p[1] = v >> 8;
            p[2] = v >> 16;
            break;
        case 4:
            *(u32 *)p = v;
            break;
        }
    }
    return 1;
}

void Image_SetPixel(Image *img, s32 x, s32 y, u32 color)
{
    ImagePixel px;

    px.x = x;
    px.y = y;
    px.color = color;
    Image_PutPixel(img, &px);
}

/* The colour of pixel (x, y) as 0xAARRGGBB: the palette index of an indexed
 * image, else each channel scaled to 8 bits (alpha 0xFF when absent). */
u32 Image_GetPixel(s32 x, s32 y, Image *img)
{
    u8 *p = Image_PixelAddr(x, y, img);
    u32 v;
    u32 c;
    u32 r;
    u32 g;
    u32 b;
    u32 a;

    if (p == NULL) {
        return 0;
    }
    if (img->flags & IMAGE_INDEXED) {
        switch (img->bpp) {
        case 0:
            if (img->flags & IMAGE_8BIT) {
                v = *p;
            } else {
                c = *p;
                if ((x & 1) ^ (s32)((img->flags & IMAGE_NIBBLE_HI) != 0)) {
                    v = c & 0xF;
                } else {
                    v = (c >> 4) & 0xF;
                }
            }
            break;
        case 1:
            v = *p;
            break;
        case 2:
            v = *(u16 *)p;
            break;
        case 4:
            v = *(u32 *)p;
            break;
        }
        return v;
    }
    switch (img->bpp) {
    case 2:
        v = *(u16 *)p;
        break;
    case 3:
        v = p[0] | (p[2] << 16 | p[1] << 8);
        break;
    case 4:
        v = *(u32 *)p;
        break;
    }
    if (img->a.bits) {
        a = ((img->a.mask & (v >> img->a.shift)) * 0xFF) / (u32)img->a.mask;
    } else {
        a = 0xFF;
    }
    if (img->r.bits) {
        r = ((img->r.mask & (v >> img->r.shift)) * 0xFF) / (u32)img->r.mask;
    } else {
        r = 0;
    }
    if (img->g.bits) {
        g = ((img->g.mask & (v >> img->g.shift)) * 0xFF) / (u32)img->g.mask;
    } else {
        g = 0;
    }
    if (img->b.bits) {
        b = ((img->b.mask & (v >> img->b.shift)) * 0xFF) / (u32)img->b.mask;
    } else {
        b = 0;
    }
    return (a << 24) | (r << 16) | (g << 8) | b;
}

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

/* The wait is a switch on sceMcInit's result: its case makes the block after
 * the loop a branch target, which MWCC schedules as the game has it (a0's lui
 * and addiu together before a1 = 0); a do-while gives lui, a1, addiu. */
s32 Sys_InitCard(void)
{
    s32 i;
    u8 *p;

    for (;;) {
        switch (sceMcInit()) {
        case -0x65:
            continue;
        }
        break;
    }
    Mem_Fill(D_00249A10, 0, 0x64);
    for (i = 0, p = D_002499F0; i < 2; i++, p += 0xC) {
        Mem_Fill(p, 0, 0xC);
    }
    return 1;
}

s32 Packet_SetMode1Ok(void)
{
    gPacketMode = 1;
    return 1;
}
