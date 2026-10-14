/*
 * main 0x180110-0x180640: the packet buffer of the frame (DMA packets are
 * allocated from it: Packet_Alloc), and texture handles made from a TIM2
 * file (Tex_CreateFromTim2).
 */
#include "common.h"
#include "texture.h"

extern s32 gPacketMode;          /* gSysWork: packet buffer in use when 0..2 */
extern s32 gPacketBufIdx;          /* gSysWork: frame buffer index (0/1) */
extern u8 *gPacketBuf;          /* gSysWork: packet buffer of this frame */
extern u8 *gPacketEnd;          /* gSysWork: its end */
extern u8 *gPacketCur;          /* gSysWork: next free byte */
extern void *gPacketBlocks[2];     /* gSysWork: the two buffers (heap blocks) */

void *Mem_Alloc(u32 size, s32 flag);
void *Mem_GetAddr(void *handle);
s32 Dma_WaitIdle(void);
void Mem_Copy(void *dst, void *src, s32 n);
s32 Tex_Swizzle(s32 w, s32 h, u8 *src, u8 *dst, s32 bits8);
s32 Tim2_LastLevel(u8 *tim2);
s32 Tim2_DescribeLevels(Image *img, u8 *tim2);
s32 Tim2_DescribeClut(Image *img, u8 *tim2);
u8 *Tim2_GetLevel(u8 *tim2, s32 level);
u8 *Tim2_GetClut(u8 *tim2);
void Tex_AlphaToGs(u8 *pixels, s32 w, s32 h);

/* Starts the frame's packet buffer (the one of gPacketBufIdx). */
void Packet_Reset(void)
{
    switch (gPacketMode) {
    case 0:
    case 2:
    case 1:
        gPacketBuf = Mem_GetAddr(gPacketBlocks[gPacketBufIdx]);
        gPacketCur = gPacketBuf;
        gPacketEnd = (u8 *)((u32)gPacketBuf + 0xA0000);
        break;
    }
}

/* size bytes of the packet buffer, aligned; when it is full, waits for the
 * DMA to finish and starts it again. */
void *Packet_Alloc(u32 size, s32 align)
{
    u8 *cur = gPacketCur;
    u8 *p;
    u8 *end;

    /* a check compiled out (any empty compound `if` here gives the
     * original's registers) */
    if (align == 0 || (align & (align - 1)) != 0) {
    }
    p = (u8 *)(~(align - 1) & (u32)(cur + align - 1));
    end = p + size;
    if (gPacketEnd < end) {
        Dma_WaitIdle();
        p = gPacketBuf;
        end = p + size;
    }
    gPacketCur = end;
    return p;
}

/* Texture handle (and palette handle, for 4- and 8-bit pictures) made from
 * picture 0 of a TIM2 file, with its mip levels; 0 when the file is not
 * TIM2. */
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode)
{
    Image lv[7];
    Image pal;
    u32 th;
    u32 ph;
    s32 n;
    TexRec *t;
    TexRec *pt;
    u8 *dst;
    u8 *src;
    s32 i;
    s32 w;
    s32 h;
    u32 size;

    ph = 0;
    th = Tex_FindFree();
    t = &gTexRecs[(th & 0xFFFF) - 1];
    n = Tim2_LastLevel(tim2);
    if (Tim2_DescribeLevels(lv, tim2) == 0) {
        return 0;
    }
    Tex_SetupRec(lv, n + 1, th, mode);
    t->mem = Mem_Alloc(t->size, 2);
    dst = Mem_GetAddr(t->mem);
    w = t->width;
    h = t->height;
    for (i = 0; i <= n; i++) {
        switch (lv[i].bpp) {
        case 0:
            size = (w * h) >> 1;
            src = Tim2_GetLevel(tim2, i);
            if (t->unk36 == 0) {
                Mem_Copy(dst, src, size);
            } else {
                Tex_Swizzle(w, h, src, dst, 0);
            }
            break;
        case 1:
            size = w * h;
            src = Tim2_GetLevel(tim2, i);
            if (t->unk36 == 0) {
                Mem_Copy(dst, src, size);
            } else {
                Tex_Swizzle(w, h, src, dst, 1);
            }
            break;
        case 2:
            size = w * h * 2;
            Mem_Copy(dst, Tim2_GetLevel(tim2, i), size);
            break;
        case 3:
            size = w * h * 4;
            Mem_Copy(dst, Tim2_GetLevel(tim2, i), size);
            break;
        case 4:
            size = w * h * 4;
            Mem_Copy(dst, Tim2_GetLevel(tim2, i), size);
            Tex_AlphaToGs(dst, w, h);
            break;
        }
        w >>= 1;
        h >>= 1;
        dst += size;
    }
    Tex_Upload(th, mode);
    if (t->psm == GS_PSMT4 || t->psm == GS_PSMT8) {
        ph = Pal_FindFree();
        pt = &gPalRecs[((ph & 0xFFFF0000) >> 16) - 1];
        Tim2_DescribeClut(&pal, tim2);
        Pal_SetupRec(&pal, ph, mode);
        pt->mem = Mem_Alloc(pt->size, 2);
        dst = Mem_GetAddr(pt->mem);
        Mem_Copy(dst, Tim2_GetClut(tim2), pt->size);
        if (pal.bpp == 4) {
            Tex_AlphaToGs(dst, pt->width, pt->height);
        }
        Pal_Upload(ph, mode);
    }
    return th | ph;
}

/* 32-bit pixels: alpha to the GS range (0xFF -> 0x80, else halved, 1 at
 * least unless 0). */
void Tex_AlphaToGs(u8 *pixels, s32 w, s32 h)
{
    s32 x;
    s32 y;
    u8 a;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            a = pixels[3];
            if (a == 0xFF) {
                a = 0x80;
            } else if (a != 0) {
                a >>= 1;
                if (a == 0) {
                    a = 1;
                }
            }
            pixels[3] = a;
            pixels += 4;
        }
    }
}
