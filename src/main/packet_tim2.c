/*
 * The packet buffers (Packet_InitBuffers allocates the two; the frame's
 * DMA packets are allocated from one of them: Packet_Alloc), and texture
 * handles made from a TIM2 file (Tex_CreateFromTim2).
 */
#include "common.h"
#include "texture.h"
#include "dma.h"
#include "pad.h"
#include "syswork.h"
#include "packet_tim2.h"
#include "mem_util.h"
#include "tim2_file.h"

extern s32 gPacketMode;          /* gSysWork: packet buffer in use when 0..2 */
extern s32 gPacketBufIdx;          /* gSysWork: frame buffer index (0/1) */
extern u8 *gPacketBuf;          /* gSysWork: packet buffer of this frame */
extern u8 *gPacketEnd;          /* gSysWork: its end */
extern u8 *gPacketCur;          /* gSysWork: next free byte */
extern void *gPacketBlocks[2];     /* gSysWork: the two buffers (heap blocks) */

/*
 * Allocates the two packet buffers (gSysWork.buf, PACKET_BUF_SIZE each), one per
 * frame, and starts the current one (Packet_Reset).
 */
void Packet_InitBuffers(void)
{
    s32 i;

    for (i = 0; i < 2; i++) {
        gSysWork.buf[i] = Mem_Alloc(PACKET_BUF_SIZE, 1);
    }
    Packet_Reset();
}

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

    /* an empty check that align is a power of two: the game's code was
     * built with one here (a check compiled to nothing), and without it the
     * compiler gives the values other registers */
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
u32 Tex_CreateFromTim2(u8 *file, s32 mode)
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
    lastLevel = Tim2_LastLevel(file);
    if (Tim2_DescribeLevels(levels, file) == 0) {
        return 0;
    }
    Tex_SetupRec(levels, lastLevel + 1, texHandle, mode);
    texRec->mem = Mem_Alloc(texRec->size, 2);
    dst = Mem_GetAddr(texRec->mem);
    levelWidth = texRec->width;
    levelHeight = texRec->height;
    for (i = 0; i <= lastLevel; i++) {
        switch (levels[i].bpp) { /* bytes per pixel */
        case 0: /* 4 bits */
            size = (levelWidth * levelHeight) >> 1;
            src = Tim2_GetLevel(file, i);
            if (texRec->swizzled == 0) {
                Mem_Copy(dst, src, size);
            } else {
                Tex_Swizzle(levelWidth, levelHeight, src, dst, 0);
            }
            break;
        case 1:
            size = levelWidth * levelHeight;
            src = Tim2_GetLevel(file, i);
            if (texRec->swizzled == 0) {
                Mem_Copy(dst, src, size);
            } else {
                Tex_Swizzle(levelWidth, levelHeight, src, dst, 1);
            }
            break;
        case 2:
            size = levelWidth * levelHeight * 2;
            Mem_Copy(dst, Tim2_GetLevel(file, i), size);
            break;
        case 3:
            size = levelWidth * levelHeight * 4;
            Mem_Copy(dst, Tim2_GetLevel(file, i), size);
            break;
        case 4:
            size = levelWidth * levelHeight * 4;
            Mem_Copy(dst, Tim2_GetLevel(file, i), size);
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
        Tim2_DescribeClut(&pal, file);
        Pal_SetupRec(&pal, palHandle, mode);
        palRec->mem = Mem_Alloc(palRec->size, 2);
        dst = Mem_GetAddr(palRec->mem);
        Mem_Copy(dst, Tim2_GetClut(file), palRec->size);
        if (pal.bpp == 4) {
            Tex_AlphaToGs(dst, palRec->width, palRec->height);
        }
        Pal_Upload(palHandle, mode);
    }
    return texHandle | palHandle;
}

/* 32-bit pixels: alpha to the GS range (255 -> 128, else halved, 1 at
 * least unless 0). */
void Tex_AlphaToGs(u8 *pixels, s32 width, s32 height)
{
    s32 x;
    s32 y;
    u8 alpha;

    for (y = 0; y < height; y++) {
        for (x = 0; x < width; x++) {
            alpha = pixels[3]; /* the pixel's fourth byte */
            if (alpha == 255) {
                alpha = 128;
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
