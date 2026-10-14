/*
 * 0x17D250: textures in GS memory: making handles resident
 * (Tex_MakeResident), the GS memory they need, converting an Image to the
 * GS layout (flPS2ConvertTextureFromContext: channel order, alpha 0x80 =
 * 1.0, 4/8-bit swizzling), and the VramSlot list.
 */
#include "common.h"
#include "dma.h"
#include "texture.h"

/* This file was built at optimization level 4: Tex_MakeResident and the
 * 8-times unrolled level loop of Tex_VramBlocks only match so, every
 * other function matches either way. */
#pragma optimization_level 4

/* Swizzle tables of the 4- and 8-bit layouts */
extern u8 gClutCsm1Order[32];
extern s32 gSwizzleBlockCol[32];
extern s32 gSwizzleBlockRow[32];
extern s32 gSwizzle4BlockOrder[32];
extern s32 gSwizzle8BlockOrder[32];
extern s32 *gTexSwizzle4Tbls[4];
extern s32 *gTexSwizzle8Tbls[4];
extern s32 D_002191A8;
extern char D_001C7BF0[];       /* "Not supported texture bit depth @flPS2ConvertTextureFromContext" */

void Mem_Fill(void *dst, u8 c, s32 n);
void *memcpy(void *dst, const void *src, u32 n);
void Mem_Copy(void *dst, void *src, s32 n);
s32 Tex_UploadPacketSize(u32 unused0);
void *Packet_Alloc(u32 size, s32 align);
void *Mem_Alloc(u32 size, s32 flag);
void *Mem_GetAddr(void *handle);
u8 *MainHeap_MaxAlloc(u32 size);
s32 Debug_Printf(const char *fmt, ...);
void Dma_BuildEndPacket(u128 *pkt, s32 irq);
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *q);
extern DmaQueue gDmaQueue;
s32 Tex_ConvertPixels(Image *src, Image *dst, s32 dir, s32 clut);
s32 Tex_Swizzle(s32 w, s32 h, u8 *src, u8 *dst, s32 bits8);
void Tex_Swizzle4(s32 w, s32 h, u8 *src, u8 *dst);
void Tex_Swizzle4Block(u8 *block, u8 *dst, s32 bw);
void Tex_Swizzle4Kernel(u8 *src, u8 *dst, s32 bw);
void Tex_Swizzle8(s32 w, s32 h, u8 *src, u8 *dst);
void Tex_Swizzle8Block(u8 *block, u8 *dst, s32 bw);
void Tex_Swizzle8Kernel(u8 *src, u8 *dst, s32 bw);

/* Makes the textures and palettes of n handles resident in GS memory: in a
 * gap, or else in slots whose textures are not in use (asked again until
 * one is found), and sends their pixels; returns how many were sent. */
s32 Tex_MakeResident(s32 n, u32 *handles)
{
    TexRec *last = NULL;
    s32 i;
    TexRec *tt;
    TexRec *pt;
    VramSlot *s;

    TexRec *t;
    s32 k;
    s32 count = 0;
    u32 th;
    u32 ph;
    u128 *p;

    for (i = 0; i < n; i++) {
        th = handles[i] & 0xFFFF;
        ph = (handles[i] & 0xFFFF0000) >> 16;
        tt = &gTexRecs[th - 1];
        pt = &gPalRecs[ph - 1];
        for (k = 0; k < 2; k++) {
            if (k == 0) {
                if (th == 0 || th >= 0x100) {
                    continue;
                }
                t = tt;
            } else {
                if (ph == 0 || ph >= 0x100) {
                    continue;
                }
                t = pt;
            }
            if (t->resident != 0) {
                continue;
            }
            s = VramSlot_FindGap(t->blocks, t->align);
            if (s != (VramSlot *)-1) {
                VramSlot_Assign(s, t);
            } else {
                do {
                    s = VramSlot_FindVictim(t, handles, n);
                } while (s == (VramSlot *)-1);
                VramSlot_Replace(s, t);
            }
            Tex_SendLevels(t);
            last = t;
            count++;
        }
    }
    if (last != NULL) {
        p = Packet_Alloc(Tex_UploadPacketSize(0), 16);
        Dma_BuildEndPacket(p, 1);
        DmaQueue_AddChain(0, (u32)p & 0x0FFFFFFF, (u32 *)p, &gDmaQueue);
    }
    return count;
}

/* log2 of a size (1..10) */
s16 Tex_Log2(s16 size)
{
    s32 i;

    for (i = 1; i <= 10; i++) {
        if ((1 << i) >= size) {
            return i;
        }
    }
    return 1;
}

/* Bytes of all mip levels. */
s32 Tex_Size(u32 psm, s32 w, s32 h, s32 levels)
{
    s32 size = 0;
    s32 i;

    for (i = 0; i < levels; i++) {
        switch (psm) {
        case GS_PSMCT32:
        case GS_PSMCT24:
            size += w * h * 4;
            break;
        case GS_PSMCT16:
        case 10:
            size += w * h * 2;
            break;
        case GS_PSMT8:
            size += w * h;
            break;
        case GS_PSMT4:
            size += (w * h) >> 1;
            break;
        }
        w >>= 1;
        h >>= 1;
    }
    return size;
}

/* GS memory blocks of a texture: whole pages, the height rounded up to a
 * power of two (32 at least), all mip levels. */
s16 Tex_VramBlocks(TexRec *t)
{
    s16 pw;
    s16 ph;
    s16 w;
    s16 h;
    s16 n;
    u32 i;
    s32 hh;

    switch (t->psm) {
    case GS_PSMCT32:
    case GS_PSMCT24:
        pw = 64;
        ph = 32;
        break;
    case GS_PSMCT16:
    case 10:
        pw = 64;
        ph = 64;
        break;
    case GS_PSMT8:
        pw = 128;
        ph = 64;
        break;
    case GS_PSMT4:
        pw = 128;
        ph = 128;
        break;
    }
    w = t->width;
    h = t->height;
    for (hh = 32; hh <= 0x400; hh <<= 1) {
        if (h <= hh) {
            h = hh;
            break;
        }
    }
    n = 0;
    for (i = 0; i < t->levels; i++) {
        n += ((w + pw - 1) / pw) * ((h + ph - 1) / ph) * 32;
        w >>= 1;
        h >>= 1;
    }
    return n;
}

/* GS memory blocks of a palette. */
s16 Pal_VramBlocks(TexRec *t)
{
    s16 n;

    if (t->height == 1) {
        n = 2;
    } else {
        switch (t->psm) {
        case GS_PSMCT32:
        case GS_PSMCT24:
            n = 4;
            break;
        case GS_PSMCT16:
            n = 4;
            break;
        }
    }
    return n;
}

s32 flPS2ConvertTextureFromContext(Image *img, TexRec *t, s32 pal)
{
    s32 w;
    s32 h;
    u8 *dst;
    s32 size;
    u32 i;
    Image gs;

    t->mem = Mem_Alloc(t->size, 2);
    dst = Mem_GetAddr(t->mem);
    gs.bpp = img->bpp;
    i = 0;
    gs.flags = img->flags;
    w = t->width;
    h = t->height;
    for (; i < t->levels; i++, img++) {
        gs.width = w;
        gs.height = h;
        gs.pitch = gs.width * gs.bpp;
        switch (t->psm) {
        default:
            Debug_Printf(D_001C7BF0);
            break;
        case GS_PSMT4:
            size = (w * h) >> 1;
            if (t->unk36 == 0) {
                Mem_Copy(dst, img->pixels, t->size);
            } else {
                gs.pixels = MainHeap_MaxAlloc(size);
                Mem_Copy(gs.pixels, img->pixels, size);
                Tex_Swizzle(w, h, gs.pixels, dst, 0);
            }
            break;
        case GS_PSMT8:
            size = w * h;
            if (t->unk36 == 0) {
                Mem_Copy(dst, img->pixels, t->size);
            } else {
                gs.pixels = MainHeap_MaxAlloc(size);
                Mem_Copy(gs.pixels, img->pixels, size);
                Tex_Swizzle(w, h, gs.pixels, dst, 1);
            }
            break;
        case GS_PSMCT16:
            gs.pixels = dst;
            size = w * h * 2;
            IMAGE_RGBA5551(&gs);
            IMAGE_GS_ORDER_16(&gs);
            Tex_ConvertPixels(img, &gs, 0, pal);
            break;
        case GS_PSMCT24:
            gs.pixels = dst;
            size = w * h * 4;
            IMAGE_RGB888(&gs);
            IMAGE_GS_ORDER_32(&gs);
            Tex_ConvertPixels(img, &gs, 0, pal);
            break;
        case GS_PSMCT32:
            gs.pixels = dst;
            size = w * h * 4;
            IMAGE_RGBA8888(&gs);
            IMAGE_GS_ORDER_32(&gs);
            Tex_ConvertPixels(img, &gs, 0, pal);
            break;
        }
        dst += size;
        w >>= 1;
        h >>= 1;
    }
    return 1;
}

s32 Tex_ConvertPixels(Image *src, Image *dst, s32 dir, s32 clut)
{
    s32 x;
    s32 y = 0;
    u32 r;
    u32 g;
    u32 b;
    u32 a;
    s32 n = 0;

    s32 off = 0;
    u8 *sp = src->pixels;
    u8 *dp = dst->pixels;
    u8 *d;
    s32 bpp;
    u8 *s;
    u32 c;

    for (; y < dst->height; y++) {
        for (x = 0; x < dst->width; x++, n++) {
            if (clut == 1 && dir == 1) {
                bpp = src->bpp;
                s = sp + bpp * ((n & 0xE0) + gClutCsm1Order[n & 0x1F]);
            } else {
                bpp = src->bpp;
                s = sp + off;
                off += bpp;
            }
            switch (bpp) {
            case 2:
                c = *(u16 *)s;
                break;
            case 3:
                c = (s[2] << 16) | (s[1] << 8) | s[0];
                break;
            case 4:
                c = *(u32 *)s;
                break;
            }
            r = (src->r.mask & (c >> src->r.shift)) << (8 - src->r.bits);
            g = (src->g.mask & (c >> src->g.shift)) << (8 - src->g.bits);
            b = (src->b.mask & (c >> src->b.shift)) << (8 - src->b.bits);
            a = (src->a.mask & (c >> src->a.shift)) << (8 - src->a.bits);
            if (clut == 1 && dir == 0) {
                d = dp + dst->bpp * ((n & 0xE0) + gClutCsm1Order[n & 0x1F]);
            } else {
                d = dp + dst->pitch * y + dst->bpp * x;
            }
            if (bpp == 4) {
                if (dir == 0) {
                    if (a == 0xFF) {
                        a = 0x80;
                    } else if (a != 0) {
                        a >>= 1;
                        if (a == 0) {
                            a = 1;
                        }
                    }
                } else if (a == 0x80) {
                    a = 0xFF;
                } else {
                    a <<= 1;
                }
            }
            c = ((dst->r.mask & (r >> (8 - dst->r.bits))) << dst->r.shift) |
                ((dst->g.mask & (g >> (8 - dst->g.bits))) << dst->g.shift) |
                ((dst->b.mask & (b >> (8 - dst->b.bits))) << dst->b.shift) |
                ((dst->a.mask & (a >> (8 - dst->a.bits))) << dst->a.shift);
            switch (bpp) {
            case 2:
                *(u16 *)d = c;
                break;
            case 3:
                d[0] = r;
                d[1] = g;
                d[2] = b;
                break;
            case 4:
                *(u32 *)d = c;
                break;
            }
        }
    }
    return 1;
}

s32 Tex_Swizzle(s32 w, s32 h, u8 *src, u8 *dst, s32 bits8)
{
    switch (bits8) {
    case 0:
        Tex_Swizzle4(w, h, src, dst);
        return 0;
    case 1:
        Tex_Swizzle8(w, h, src, dst);
        return 0;
    default:
        return -1;
    }
}

void Tex_Swizzle4(s32 w, s32 h, u8 *src, u8 *dst)
{
    s32 bw;
    s32 bh;
    s32 y;
    u8 block[0x2000];
    s32 i;
    s32 x;
    u8 *q;
    u8 *p;

    bw = (w - 1) / 128 + 1;
    bh = (h - 1) / 128 + 1;
    for (y = 0; y < bh; y++) {
        for (x = 0; x < bw; x++) {
            for (q = src + bw * (y * 0x2000) + x * 0x40, p = block, i = 0; i < 0x80; i++, q += bw << 6, p += 0x40) {
                memcpy(p, q, 0x40);
            }
            Tex_Swizzle4Block(block, dst + bw * (y * 0x2000) + x * 0x100, bw);
        }
    }
}

/* A 128 x 128 block of 4-bit pixels (rows of 64 bytes) into GS memory,
 * as 32 swizzled 32 x 16 pieces. */
void Tex_Swizzle4Block(u8 *block, u8 *dst, s32 bw)
{
    s32 y;
    s32 x;
    s32 *idx = gSwizzle4BlockOrder;
    s32 k;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 4; x++) {
            k = *idx++;
            Tex_Swizzle4Kernel(block + ((y * 16) << 6) + x * 16, dst + bw * (gSwizzleBlockRow[k] << 11) + (gSwizzleBlockCol[k] << 5), bw);
        }
    }
}

void Tex_Swizzle4Kernel(u8 *src, u8 *dst, s32 bw)
{
    s32 j;
    s32 k;
    s32 i;
    s32 **tab = gTexSwizzle4Tbls;
    s32 *idx;
    s32 n;
    u8 lo;
    u8 hi;

    for (i = 0; i < 4; i++, src += 0x100) {
        idx = *tab++;
        for (j = 0; j < 2; j++, dst += (bw << 8) - 0x20) {
            for (k = 0; k < 32; k++) {
                n = idx[0];
                if ((n & 1) == 0) {
                    lo = src[n / 2] & 0xF;
                } else {
                    lo = (src[n / 2] & 0xF0) >> 4;
                }
                n = idx[1];
                idx += 2;
                if ((n & 1) == 0) {
                    hi = (src[n / 2] & 0xF) << 4;
                } else {
                    hi = src[n / 2] & 0xF0;
                }
                *dst++ = lo | hi;
            }
        }
    }
}

/* 8-bit pixels into the GS layout, in blocks of 128 x 64. */
/* 8-bit pixels into the GS layout, a block of 128 x 64 at a time
 * (copied out of the rows of src first). */
void Tex_Swizzle8(s32 w, s32 h, u8 *src, u8 *dst)
{
    s32 bw;
    s32 bh;
    s32 y;
    u8 block[0x2000];
    s32 i;
    s32 x;
    u8 *q;
    u8 *p;

    bw = (w - 1) / 128 + 1;
    bh = (h - 1) / 64 + 1;
    for (y = 0; y < bh; y++) {
        for (x = 0; x < bw; x++) {
            for (q = src + bw * (y * 0x2000) + x * 0x80, p = block, i = 0; i < 0x40; i++, q += bw << 7, p += 0x80) {
                memcpy(p, q, 0x80);
            }
            Tex_Swizzle8Block(block, dst + bw * (y * 0x2000) + x * 0x100, bw);
        }
    }
}

/* A 128 x 64 block of 8-bit pixels into GS memory, as 32 swizzled
 * 16 x 16 pieces. */
void Tex_Swizzle8Block(u8 *block, u8 *dst, s32 bw)
{
    s32 y;
    s32 x;
    s32 *idx = gSwizzle8BlockOrder;
    s32 k;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 8; x++) {
            k = *idx++;
            Tex_Swizzle8Kernel(block + ((y * 16) << 7) + x * 16, dst + bw * (gSwizzleBlockRow[k] << 11) + (gSwizzleBlockCol[k] << 5), bw);
        }
    }
}

void Tex_Swizzle8Kernel(u8 *src, u8 *dst, s32 bw)
{
    s32 j;
    s32 k;
    s32 i;
    s32 **tab = gTexSwizzle8Tbls;
    s32 *idx;

    for (i = 0; i < 4; i++, src += 0x200) {
        idx = *tab++;
        for (j = 0; j < 2; j++, dst += (bw << 8) - 0x20) {
            for (k = 0; k < 32; k += 8) {
                *dst++ = src[*idx++];
                *dst++ = src[*idx++];
                *dst++ = src[*idx++];
                *dst++ = src[*idx++];
                *dst++ = src[*idx++];
                *dst++ = src[*idx++];
                *dst++ = src[*idx++];
                *dst++ = src[*idx++];
            }
        }
    }
}

void Vram_Init(void)
{
    s32 i;
    VramBlock *b;
    VramSlot *s;

    gVramBlockCount = 0;
    D_002191A8 = 1;
    for (i = 0, b = gVramBlocks; i < 4; i++, b++) {
        Mem_Fill(b, 0, sizeof(VramBlock));
    }
    gVramSlotCount = 0;
    gVramSlotHead = NULL;
    for (i = 0, s = gVramSlots; i < VRAM_SLOT_MAX; i++, s++) {
        Mem_Fill(s, 0, sizeof(VramSlot));
    }
}

VramSlot *VramSlot_Alloc(VramSlot *table, s32 n)
{
    s32 i;
    VramSlot *s;

    for (i = 0, s = gVramSlots; i < VRAM_SLOT_MAX; i++, s++) {
        if (s->addr == 0) {
            gVramSlotCount++;
            return &gVramSlots[i];
        }
    }
    return NULL;
}

/* Unlinks and clears a slot; its texture loses its GS memory. */
void VramSlot_Free(VramSlot *s)
{
    VramSlot *prev = s->prev;
    VramSlot *next = s->next;

    if (prev != NULL) {
        prev->next = next;
    } else if (next != NULL) {
        gVramSlotHead = next;
    } else {
        gVramSlotHead = NULL;
    }
    if (next != NULL) {
        next->prev = prev;
    }
    if (s->tex != NULL) {
        s->tex->resident = 0;
        s->tex->mode = 4;
    }
    Mem_Fill(s, 0, sizeof(VramSlot));
    gVramSlotCount--;
}

/* Links s after pos (at the head when pos is NULL). */
void VramSlot_Insert(VramSlot *pos, VramSlot *s)
{
    VramSlot *next = NULL;

    if (pos != NULL) {
        next = pos->next;
        pos->next = s;
    } else if (gVramSlotHead == NULL) {
        gVramSlotHead = s;
    } else {
        next = gVramSlotHead;
        gVramSlotHead = s;
    }
    s->prev = pos;
    s->next = next;
    if (next != NULL) {
        next->prev = s;
    }
}

/* texconv's .data (0x1BA150-0x1BA990) */
u8 gClutCsm1Order[32] = {
#include "main/001BA150.inc"
};
s32 gSwizzleBlockCol[32] = {
#include "main/001BA170.inc"
};
s32 gSwizzleBlockRow[32] = {
#include "main/001BA1F0.inc"
};
s32 gSwizzle4BlockOrder[32] = {
#include "main/001BA270.inc"
};
s32 gSwizzle8BlockOrder[32] = {
#include "main/001BA2F0.inc"
};
s32 gTexSwizzle4Tbls_0[128] = {
#include "main/001BA370.inc"
};
s32 gTexSwizzle4Tbls_1[128] = {
#include "main/001BA570.inc"
};
s32 *gTexSwizzle4Tbls[4] = {
    gTexSwizzle4Tbls_0, gTexSwizzle4Tbls_1, gTexSwizzle4Tbls_0, gTexSwizzle4Tbls_1,
};
s32 gTexSwizzle8Tbls_0[64] = {
#include "main/001BA780.inc"
};
s32 gTexSwizzle8Tbls_1[64] = {
#include "main/001BA880.inc"
};
s32 *gTexSwizzle8Tbls[4] = {
    gTexSwizzle8Tbls_0, gTexSwizzle8Tbls_1, gTexSwizzle8Tbls_0, gTexSwizzle8Tbls_1,
};

/* texconv's .rodata (0x1C7BF0-0x1C7C30) */
#pragma rodata on
char D_001C7BF0[0x40] = {
#include "main/001C7BF0.inc"
};
#pragma rodata off
