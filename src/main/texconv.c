/*
 * Textures in GS memory: the sizes and GS memory a texture needs, and
 * converting an Image to the GS layout (flPS2ConvertTextureFromContext:
 * channel order, alpha 128 = 1.0, 4/8-bit swizzling).
 */
#include "common.h"
#include "dma.h"
#include "texture.h"
#include "heap.h"
#include "gs_setup.h"
#include "mem_util.h"
#include "libc.h"

/* This file was built at optimization level 4: the 8-times unrolled
 * level loop of Tex_VramBlocks only matches so, every other function
 * matches either way. */
#pragma optimization_level 4

/* Swizzle tables of the 4- and 8-bit layouts */
extern u8 gClutCsm1Order[32];
extern s32 gSwizzleBlockCol[32];
extern s32 gSwizzleBlockRow[32];
extern s32 gSwizzle4BlockOrder[32];
extern s32 gSwizzle8BlockOrder[32];
extern s32 *gTexSwizzle4Tbls[4];
extern s32 *gTexSwizzle8Tbls[4];
extern char D_001C7BF0[];       /* "Not supported texture bit depth @flPS2ConvertTextureFromContext" */

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

/*
 * Bytes of a texture of psm, width x height at the top level, over levels
 * mip levels (each half the width and height of the one before).
 */
s32 Tex_Size(u32 psm, s32 width, s32 height, s32 levels)
{
    s32 size = 0;
    s32 i;

    for (i = 0; i < levels; i++) {
        switch (psm) {
        case GS_PSMCT32:
        case GS_PSMCT24:
            size += width * height * 4;
            break;
        case GS_PSMCT16:
        case 10:
            size += width * height * 2;
            break;
        case GS_PSMT8:
            size += width * height;
            break;
        case GS_PSMT4:
            size += (width * height) >> 1;
            break;
        }
        width >>= 1;
        height >>= 1;
    }
    return size;
}

/* GS memory blocks of a texture: whole pages, the height rounded up to a
 * power of two (32 at least), all mip levels. */
s16 Tex_VramBlocks(TexRec *rec)
{
    s16 pageWidth;
    s16 pageHeight;
    s16 levelWidth;
    s16 levelHeight;
    s16 blockCount;
    u32 i;
    s32 pow2Height;

    switch (rec->psm) {
    case GS_PSMCT32:
    case GS_PSMCT24:
        pageWidth = 64;
        pageHeight = 32;
        break;
    case GS_PSMCT16:
    case 10:
        pageWidth = 64;
        pageHeight = 64;
        break;
    case GS_PSMT8:
        pageWidth = 128;
        pageHeight = 64;
        break;
    case GS_PSMT4:
        pageWidth = 128;
        pageHeight = 128;
        break;
    }
    levelWidth = rec->width;
    levelHeight = rec->height;
    for (pow2Height = 32; pow2Height <= 1024; pow2Height <<= 1) {
        if (levelHeight <= pow2Height) {
            levelHeight = pow2Height;
            break;
        }
    }
    blockCount = 0;
    for (i = 0; i < rec->levels; i++) {
        blockCount += ((levelWidth + pageWidth - 1) / pageWidth) * ((levelHeight + pageHeight - 1) / pageHeight) * 32;
        levelWidth >>= 1;
        levelHeight >>= 1;
    }
    return blockCount;
}

/* GS memory blocks of a palette. */
s16 Pal_VramBlocks(TexRec *palRec)
{
    s16 blockCount;

    if (palRec->height == 1) {
        blockCount = 2;
    } else {
        switch (palRec->psm) {
        case GS_PSMCT32:
        case GS_PSMCT24:
            blockCount = 4;
            break;
        case GS_PSMCT16:
            blockCount = 4;
            break;
        }
    }
    return blockCount;
}

/*
 * Converts the levels of img (one Image per mip level) into the GS layout
 * in a new heap block (rec->mem): 4- and 8-bit pixels are copied, or
 * swizzled by Tex_Swizzle when rec->swizzled is set; 16-, 24- and 32-bit
 * pixels get the GS channel order and alpha (Tex_ConvertPixels).
 * reorderClut 1 puts the entries of a 256-colour palette in the GS CSM1
 * order. Returns 1.
 */
s32 flPS2ConvertTextureFromContext(Image *img, TexRec *rec, s32 reorderClut)
{
    s32 levelWidth;
    s32 levelHeight;
    u8 *dst;
    s32 size;
    u32 i;
    Image gsImage;

    rec->mem = Mem_Alloc(rec->size, 2);
    dst = Mem_GetAddr(rec->mem);
    gsImage.bpp = img->bpp;
    i = 0;
    gsImage.flags = img->flags;
    levelWidth = rec->width;
    levelHeight = rec->height;
    for (; i < rec->levels; i++, img++) {
        gsImage.width = levelWidth;
        gsImage.height = levelHeight;
        gsImage.pitch = gsImage.width * gsImage.bpp;
        switch (rec->psm) {
        default:
            Debug_Printf(D_001C7BF0);
            break;
        case GS_PSMT4:
            size = (levelWidth * levelHeight) >> 1;
            if (rec->swizzled == 0) {
                Mem_Copy(dst, img->pixels, rec->size);
            } else {
                gsImage.pixels = MainHeap_MaxAlloc(size);
                Mem_Copy(gsImage.pixels, img->pixels, size);
                Tex_Swizzle(levelWidth, levelHeight, gsImage.pixels, dst, 0);
            }
            break;
        case GS_PSMT8:
            size = levelWidth * levelHeight;
            if (rec->swizzled == 0) {
                Mem_Copy(dst, img->pixels, rec->size);
            } else {
                gsImage.pixels = MainHeap_MaxAlloc(size);
                Mem_Copy(gsImage.pixels, img->pixels, size);
                Tex_Swizzle(levelWidth, levelHeight, gsImage.pixels, dst, 1);
            }
            break;
        case GS_PSMCT16:
            gsImage.pixels = dst;
            size = levelWidth * levelHeight * 2;
            IMAGE_RGBA5551(&gsImage);
            IMAGE_GS_ORDER_16(&gsImage);
            Tex_ConvertPixels(img, &gsImage, 0, reorderClut);
            break;
        case GS_PSMCT24:
            gsImage.pixels = dst;
            size = levelWidth * levelHeight * 4;
            IMAGE_RGB888(&gsImage);
            IMAGE_GS_ORDER_32(&gsImage);
            Tex_ConvertPixels(img, &gsImage, 0, reorderClut);
            break;
        case GS_PSMCT32:
            gsImage.pixels = dst;
            size = levelWidth * levelHeight * 4;
            IMAGE_RGBA8888(&gsImage);
            IMAGE_GS_ORDER_32(&gsImage);
            Tex_ConvertPixels(img, &gsImage, 0, reorderClut);
            break;
        }
        dst += size;
        levelWidth >>= 1;
        levelHeight >>= 1;
    }
    return 1;
}

/*
 * Converts the pixels of src into dst (the size and channel layout of
 * dst), pixel by pixel: each channel is scaled through 8 bits. For 32-bit
 * pixels, fromGs 0 maps alpha 255 to 128 (1.0 on the GS) and halves the
 * rest, fromGs 1 does the reverse. reorderClut 1 shuffles the pixel order
 * by gClutCsm1Order (the GS CSM1 palette layout): on the source side when
 * fromGs is 1, on the destination side when it is 0. Returns 1.
 */
s32 Tex_ConvertPixels(Image *src, Image *dst, s32 fromGs, s32 reorderClut)
{
    s32 x;
    s32 y = 0;
    u32 red;
    u32 green;
    u32 blue;
    u32 alpha;
    s32 pixelIndex = 0;

    s32 srcOffset = 0;
    u8 *srcBase = src->pixels;
    u8 *dstBase = dst->pixels;
    u8 *dstPixel;
    s32 bpp;
    u8 *srcPixel;
    u32 color;

    for (; y < dst->height; y++) {
        for (x = 0; x < dst->width; x++, pixelIndex++) {
            if (reorderClut == 1 && fromGs == 1) {
                bpp = src->bpp;
                srcPixel = srcBase + bpp * ((pixelIndex & 0xE0) + gClutCsm1Order[pixelIndex & 0x1F]);
            } else {
                bpp = src->bpp;
                srcPixel = srcBase + srcOffset;
                srcOffset += bpp;
            }
            switch (bpp) {
            case 2:
                color = *(u16 *)srcPixel;
                break;
            case 3:
                color = (srcPixel[2] << 16) | (srcPixel[1] << 8) | srcPixel[0];
                break;
            case 4:
                color = *(u32 *)srcPixel;
                break;
            }
            red = (src->r.mask & (color >> src->r.shift)) << (8 - src->r.bits);
            green = (src->g.mask & (color >> src->g.shift)) << (8 - src->g.bits);
            blue = (src->b.mask & (color >> src->b.shift)) << (8 - src->b.bits);
            alpha = (src->a.mask & (color >> src->a.shift)) << (8 - src->a.bits);
            if (reorderClut == 1 && fromGs == 0) {
                dstPixel = dstBase + dst->bpp * ((pixelIndex & 0xE0) + gClutCsm1Order[pixelIndex & 0x1F]);
            } else {
                dstPixel = dstBase + dst->pitch * y + dst->bpp * x;
            }
            if (bpp == 4) {
                if (fromGs == 0) {
                    if (alpha == 255) {
                        alpha = 128;
                    } else if (alpha != 0) {
                        alpha >>= 1;
                        if (alpha == 0) {
                            alpha = 1;
                        }
                    }
                } else if (alpha == 128) {
                    alpha = 255;
                } else {
                    alpha <<= 1;
                }
            }
            color = ((dst->r.mask & (red >> (8 - dst->r.bits))) << dst->r.shift) |
                ((dst->g.mask & (green >> (8 - dst->g.bits))) << dst->g.shift) |
                ((dst->b.mask & (blue >> (8 - dst->b.bits))) << dst->b.shift) |
                ((dst->a.mask & (alpha >> (8 - dst->a.bits))) << dst->a.shift);
            switch (bpp) {
            case 2:
                *(u16 *)dstPixel = color;
                break;
            case 3:
                dstPixel[0] = red;
                dstPixel[1] = green;
                dstPixel[2] = blue;
                break;
            case 4:
                *(u32 *)dstPixel = color;
                break;
            }
        }
    }
    return 1;
}

/*
 * Swizzles width x height pixels at src into the GS memory layout at dst:
 * is8Bit 0 for 4-bit pixels (Tex_Swizzle4), 1 for 8-bit (Tex_Swizzle8).
 * Returns 0, -1 for another is8Bit.
 */
s32 Tex_Swizzle(s32 width, s32 height, u8 *src, u8 *dst, s32 is8Bit)
{
    switch (is8Bit) {
    case 0:
        Tex_Swizzle4(width, height, src, dst);
        return 0;
    case 1:
        Tex_Swizzle8(width, height, src, dst);
        return 0;
    default:
        return -1;
    }
}

/*
 * 4-bit pixels into the GS layout, a block of 128 x 128 at a time: each
 * block is copied out of the rows of src (widthBlocks blocks across) into
 * a local buffer, then swizzled into dst by Tex_Swizzle4Block.
 */
void Tex_Swizzle4(s32 width, s32 height, u8 *src, u8 *dst)
{
    s32 widthBlocks;
    s32 heightBlocks;
    s32 y;
    u8 block[8192];
    s32 i;
    s32 x;
    u8 *srcRow;
    u8 *blockRow;

    widthBlocks = (width - 1) / 128 + 1;
    heightBlocks = (height - 1) / 128 + 1;
    for (y = 0; y < heightBlocks; y++) {
        for (x = 0; x < widthBlocks; x++) {
            for (srcRow = src + widthBlocks * (y * 8192) + x * 64, blockRow = block, i = 0; i < 128; i++, srcRow += widthBlocks << 6, blockRow += 64) {
                memcpy(blockRow, srcRow, 64);
            }
            Tex_Swizzle4Block(block, dst + widthBlocks * (y * 8192) + x * 256, widthBlocks);
        }
    }
}

/* A 128 x 128 block of 4-bit pixels (rows of 64 bytes) into GS memory,
 * as 32 swizzled 32 x 16 pieces. */
void Tex_Swizzle4Block(u8 *block, u8 *dst, s32 widthBlocks)
{
    s32 y;
    s32 x;
    s32 *pieceOrder = gSwizzle4BlockOrder;
    s32 piece;

    for (y = 0; y < 8; y++) {
        for (x = 0; x < 4; x++) {
            piece = *pieceOrder++;
            Tex_Swizzle4Kernel(block + ((y * 16) << 6) + x * 16, dst + widthBlocks * (gSwizzleBlockRow[piece] << 11) + (gSwizzleBlockCol[piece] << 5), widthBlocks);
        }
    }
}

/*
 * Swizzles one 32 x 16 piece of 4-bit pixels at src into dst: four
 * column groups, each by its table of gTexSwizzle4Tbls giving the source
 * nibble of every destination nibble. widthBlocks sets the destination
 * row stride.
 */
void Tex_Swizzle4Kernel(u8 *src, u8 *dst, s32 widthBlocks)
{
    s32 j;
    s32 k;
    s32 i;
    s32 **tables = gTexSwizzle4Tbls;
    s32 *nibbleOrder;
    s32 nibbleIdx;
    u8 lowNibble;
    u8 highNibble;

    for (i = 0; i < 4; i++, src += 256) {
        nibbleOrder = *tables++;
        for (j = 0; j < 2; j++, dst += (widthBlocks << 8) - 32) {
            for (k = 0; k < 32; k++) {
                nibbleIdx = nibbleOrder[0];
                if ((nibbleIdx & 1) == 0) {
                    lowNibble = src[nibbleIdx / 2] & 0xF;
                } else {
                    lowNibble = (src[nibbleIdx / 2] & 0xF0) >> 4;
                }
                nibbleIdx = nibbleOrder[1];
                nibbleOrder += 2;
                if ((nibbleIdx & 1) == 0) {
                    highNibble = (src[nibbleIdx / 2] & 0xF) << 4;
                } else {
                    highNibble = src[nibbleIdx / 2] & 0xF0;
                }
                *dst++ = lowNibble | highNibble;
            }
        }
    }
}

/* 8-bit pixels into the GS layout, in blocks of 128 x 64. */
/*
 * 8-bit pixels into the GS layout, a block of 128 x 64 at a time: each
 * block is copied out of the rows of src (widthBlocks blocks across) into
 * a local buffer, then swizzled into dst by Tex_Swizzle8Block. This one
 * comment replaces both above it (the first repeats the second).
 */
void Tex_Swizzle8(s32 width, s32 height, u8 *src, u8 *dst)
{
    s32 widthBlocks;
    s32 heightBlocks;
    s32 y;
    u8 block[8192];
    s32 i;
    s32 x;
    u8 *srcRow;
    u8 *blockRow;

    widthBlocks = (width - 1) / 128 + 1;
    heightBlocks = (height - 1) / 64 + 1;
    for (y = 0; y < heightBlocks; y++) {
        for (x = 0; x < widthBlocks; x++) {
            for (srcRow = src + widthBlocks * (y * 8192) + x * 128, blockRow = block, i = 0; i < 64; i++, srcRow += widthBlocks << 7, blockRow += 128) {
                memcpy(blockRow, srcRow, 128);
            }
            Tex_Swizzle8Block(block, dst + widthBlocks * (y * 8192) + x * 256, widthBlocks);
        }
    }
}

/* A 128 x 64 block of 8-bit pixels into GS memory, as 32 swizzled
 * 16 x 16 pieces. */
void Tex_Swizzle8Block(u8 *block, u8 *dst, s32 widthBlocks)
{
    s32 y;
    s32 x;
    s32 *pieceOrder = gSwizzle8BlockOrder;
    s32 piece;

    for (y = 0; y < 4; y++) {
        for (x = 0; x < 8; x++) {
            piece = *pieceOrder++;
            Tex_Swizzle8Kernel(block + ((y * 16) << 7) + x * 16, dst + widthBlocks * (gSwizzleBlockRow[piece] << 11) + (gSwizzleBlockCol[piece] << 5), widthBlocks);
        }
    }
}

/*
 * Swizzles one 16 x 16 piece of 8-bit pixels at src into dst: four column
 * groups, each by its table of gTexSwizzle8Tbls giving the source byte of
 * every destination byte. widthBlocks sets the destination row stride.
 */
void Tex_Swizzle8Kernel(u8 *src, u8 *dst, s32 widthBlocks)
{
    s32 j;
    s32 k;
    s32 i;
    s32 **tables = gTexSwizzle8Tbls;
    s32 *byteOrder;

    for (i = 0; i < 4; i++, src += 512) {
        byteOrder = *tables++;
        for (j = 0; j < 2; j++, dst += (widthBlocks << 8) - 32) {
            for (k = 0; k < 32; k += 8) {
                *dst++ = src[*byteOrder++];
                *dst++ = src[*byteOrder++];
                *dst++ = src[*byteOrder++];
                *dst++ = src[*byteOrder++];
                *dst++ = src[*byteOrder++];
                *dst++ = src[*byteOrder++];
                *dst++ = src[*byteOrder++];
                *dst++ = src[*byteOrder++];
            }
        }
    }
}

/* This file's initialised data. */
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

/* This file's read-only data. */
#pragma rodata on
char D_001C7BF0[64] = {
#include "main/001C7BF0.inc"
};
#pragma rodata off
