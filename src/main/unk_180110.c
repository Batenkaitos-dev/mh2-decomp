/*
 * The packet buffer of the frame (DMA packets are
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

/*
 * Returns size bytes of the frame's packet buffer at the next multiple
 * of align (a power of two). When the buffer has no room, waits for the
 * DMA to finish and takes the bytes from the buffer's start.
 */
void *Packet_Alloc(u32 size, s32 align)
{
    u8 *cur = gPacketCur;
    u8 *block;
    u8 *end;

    /* a check compiled out (any empty compound `if` here gives the
     * original's registers) */
    if (align == 0 || (align & (align - 1)) != 0) {
    }
    block = (u8 *)(~(align - 1) & (u32)(cur + align - 1));
    end = block + size;
    if (gPacketEnd < end) {
        Dma_WaitIdle();
        block = gPacketBuf;
        end = block + size;
    }
    gPacketCur = end;
    return block;
}

/*
 * Makes a texture (and, for 4- and 8-bit pictures, a palette) from
 * picture 0 of the TIM2 file tim2 with all its mip levels: copies the
 * pixels into a heap block (swizzled when the record asks), turns
 * 32-bit alpha into the GS range, and uploads them. mode is passed to
 * Tex_SetupRec, Tex_Upload and the palette calls. Returns the texture
 * handle in the low 16 bits and the palette handle (0 if none) in the
 * high 16; 0 when the file is not TIM2.
 */
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode)
{
    Image levels[7];
    Image pal;
    u32 texHandle;
    u32 palHandle;
    s32 lastLevel;
    TexRec *texRec;
    TexRec *palRec;
    u8 *dst;
    u8 *src;
    s32 i;
    s32 levelWidth;
    s32 levelHeight;
    u32 size;

    palHandle = 0;
    texHandle = Tex_FindFree();
    texRec = &gTexRecs[(texHandle & 0xFFFF) - 1];
    lastLevel = Tim2_LastLevel(tim2);
    if (Tim2_DescribeLevels(levels, tim2) == 0) {
        return 0;
    }
    Tex_SetupRec(levels, lastLevel + 1, texHandle, mode);
    texRec->mem = Mem_Alloc(texRec->size, 2);
    dst = Mem_GetAddr(texRec->mem);
    levelWidth = texRec->width;
    levelHeight = texRec->height;
    for (i = 0; i <= lastLevel; i++) {
        switch (levels[i].bpp) {
        case 0:
            size = (levelWidth * levelHeight) >> 1;
            src = Tim2_GetLevel(tim2, i);
            if (texRec->unk36 == 0) {
                Mem_Copy(dst, src, size);
            } else {
                Tex_Swizzle(levelWidth, levelHeight, src, dst, 0);
            }
            break;
        case 1:
            size = levelWidth * levelHeight;
            src = Tim2_GetLevel(tim2, i);
            if (texRec->unk36 == 0) {
                Mem_Copy(dst, src, size);
            } else {
                Tex_Swizzle(levelWidth, levelHeight, src, dst, 1);
            }
            break;
        case 2:
            size = levelWidth * levelHeight * 2;
            Mem_Copy(dst, Tim2_GetLevel(tim2, i), size);
            break;
        case 3:
            size = levelWidth * levelHeight * 4;
            Mem_Copy(dst, Tim2_GetLevel(tim2, i), size);
            break;
        case 4:
            size = levelWidth * levelHeight * 4;
            Mem_Copy(dst, Tim2_GetLevel(tim2, i), size);
            Tex_AlphaToGs(dst, levelWidth, levelHeight);
            break;
        }
        levelWidth >>= 1;
        levelHeight >>= 1;
        dst += size;
    }
    Tex_Upload(texHandle, mode);
    if (texRec->psm == GS_PSMT4 || texRec->psm == GS_PSMT8) {
        palHandle = Pal_FindFree();
        palRec = &gPalRecs[((palHandle & 0xFFFF0000) >> 16) - 1];
        Tim2_DescribeClut(&pal, tim2);
        Pal_SetupRec(&pal, palHandle, mode);
        palRec->mem = Mem_Alloc(palRec->size, 2);
        dst = Mem_GetAddr(palRec->mem);
        Mem_Copy(dst, Tim2_GetClut(tim2), palRec->size);
        if (pal.bpp == 4) {
            Tex_AlphaToGs(dst, palRec->width, palRec->height);
        }
        Pal_Upload(palHandle, mode);
    }
    return texHandle | palHandle;
}

/* 32-bit pixels: alpha to the GS range (0xFF -> 0x80, else halved, 1 at
 * least unless 0). */
void Tex_AlphaToGs(u8 *pixels, s32 width, s32 height)
{
    s32 x;
    s32 y;
    u8 alpha;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            alpha = pixels[3];
            if (alpha == 0xFF) {
                alpha = 0x80;
            } else if (alpha != 0) {
                alpha >>= 1;
                if (alpha == 0) {
                    alpha = 1;
                }
            }
            pixels[3] = alpha;
            pixels += 4;
        }
    }
}
