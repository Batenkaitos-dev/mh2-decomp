/*
 * The TIM2 picture file's readers that Tex_CreateFromTim2 uses: the header
 * check, picture 0's image levels and palette, and where each lies in the
 * file.
 */
#include "common.h"
#include "image.h"
#include "pad.h"
#include "tim2_file.h"

/* Built at optimization level 4: Tim2_GetImage needs it, and every other
 * function matches either way. */
#pragma optimization_level 4

static Tim2PictureHeader *Tim2_GetPicture(u8 *file, s32 index);

/* The number of mip levels after the first of picture 0; 0 when the file
 * is not TIM2. */
s32 Tim2_LastLevel(u8 *file)
{
    Tim2PictureHeader *pict;

    if (Tim2_CheckHeader(file) == 0) {
        return 0;
    }
    pict = Tim2_GetPicture(file, 0);
    return (u8)(pict->mipMaps - 1);
}

/* Describes the image levels of picture 0 (up to 7). */
s32 Tim2_DescribeLevels(Image *image, u8 *file)
{
    Tim2PictureHeader *pict;
    s32 i;
    s32 levelWidth;
    s32 levelHeight;

    if (Tim2_CheckHeader(file) == 0) {
        return 0;
    }
    pict = Tim2_GetPicture(file, 0);
    if (pict->mipMaps > TIM2_MAX_LEVELS) {
        return 0;
    }
    levelWidth = pict->width;
    levelHeight = pict->height;
    for (i = 0; i < pict->mipMaps; i++) {
        image->flags = 0;
        image->width = levelWidth;
        image->height = levelHeight;
        switch (pict->imageType) {
        case TIM2_TYPE_INDEX4:
            image->flags |= IMAGE_INDEXED | IMAGE_NIBBLE_HI;
            image->bpp = 0;
            image->pitch = image->width >> 1;
            image->r.shift = 0;
            image->r.bits = 0;
            image->r.mask = 0;
            image->g.shift = 0;
            image->g.bits = 0;
            image->g.mask = 0;
            image->b.shift = 0;
            image->b.bits = 0;
            image->b.mask = 0;
            image->a.shift = 0;
            image->a.bits = 0;
            image->a.mask = 0;
            break;
        case TIM2_TYPE_INDEX8:
            image->flags |= IMAGE_INDEXED;
            image->bpp = 1;
            image->pitch = image->bpp * image->width;
            image->r.shift = 0;
            image->r.bits = 0;
            image->r.mask = 0;
            image->g.shift = 0;
            image->g.bits = 0;
            image->g.mask = 0;
            image->b.shift = 0;
            image->b.bits = 0;
            image->b.mask = 0;
            image->a.shift = 0;
            image->a.bits = 0;
            image->a.mask = 0;
            break;
        case TIM2_TYPE_RGB16:
            image->bpp = 2;
            image->pitch = image->bpp * image->width;
            IMAGE_RGBA5551(image);
            image->r.shift = 0; /* the GS order, blue high */
            image->b.shift = 10;
            break;
        case TIM2_TYPE_RGB24:
            image->bpp = 3;
            image->pitch = image->bpp * image->width;
            IMAGE_RGB888(image);
            IMAGE_GS_ORDER_32(image);
            break;
        case TIM2_TYPE_RGB32:
            image->bpp = 4;
            image->pitch = image->bpp * image->width;
            IMAGE_RGBA8888(image);
            IMAGE_GS_ORDER_32(image);
            break;
        }
        image++;
        levelWidth >>= 1;
        levelHeight >>= 1;
    }
    return 1;
}

/* Describes picture 0's palette (16 or 256 colours). */
s32 Tim2_DescribeClut(Image *image, u8 *file)
{
    Tim2PictureHeader *pict;

    if (Tim2_CheckHeader(file) == 0) {
        return 0;
    }
    pict = Tim2_GetPicture(file, 0);
    if (pict->mipMaps > TIM2_MAX_LEVELS) {
        return 0;
    }
    image->flags = 0;
    switch (pict->clutType) {
    case TIM2_TYPE_RGB16:
        image->bpp = 2;
        IMAGE_RGBA5551(image);
        image->r.shift = 0; /* the GS order, blue high */
        image->b.shift = 10;
        break;
    case TIM2_TYPE_RGB24:
        image->bpp = 3;
        IMAGE_RGB888(image);
        IMAGE_GS_ORDER_32(image);
        break;
    case TIM2_TYPE_RGB32:
        image->bpp = 4;
        IMAGE_RGBA8888(image);
        IMAGE_GS_ORDER_32(image);
        break;
    default:
        return 0;
    }
    switch (pict->imageType) {
    case TIM2_TYPE_INDEX4:
        if (pict->clutColors != 16) {
            return 0;
        }
        image->width = 16;
        image->height = 1;
        break;
    case TIM2_TYPE_INDEX8:
        if (pict->clutColors != 256) {
            return 0;
        }
        image->width = 256;
        image->height = 1;
        break;
    default:
        return 0;
    }
    image->pitch = image->bpp * image->width;
    return 1;
}

/* Level `level` of picture 0's image. */
u8 *Tim2_GetLevel(u8 *file, s32 level)
{
    return Tim2_GetImage(file, 0, level);
}

/* Picture 0's palette. */
u8 *Tim2_GetClut(u8 *file)
{
    return Tim2_GetPictClut(file, 0);
}

/* 1 for a TIM2 file of version 3, or 4 with format 0 or 1, holding one
 * picture; else 0. */
s32 Tim2_CheckHeader(u8 *file)
{
    Tim2FileHeader *header = (Tim2FileHeader *)file;
    s32 kind;
    u8 fileFormat;

    if (header->id[0] == 'T' && header->id[1] == 'I' && header->id[2] == 'M' && header->id[3] == '2') {
        kind = 1;
    } else if (header->id[0] == 'C' && header->id[1] == 'L' && header->id[2] == 'T' && header->id[3] == '2') {
        /* a palette file (CLT2) */
        return 0;
    } else {
        return 0;
    }
    fileFormat = header->format;
    if (!(header->version == 3 || (header->version == 4 && fileFormat <= 1U))) {
        return 0;
    }
    if (header->pictures != 1) {
        return 0;
    }
    return kind;
}

/* size rounded up to the file's alignment. */
static u32 Tim2_Align(u8 *file, u32 size)
{
    u32 align = ((Tim2FileHeader *)file)->format == 0 ? 16 : 128;

    if (size % align != 0) {
        size += align - (size - size / align * align) % align;
    }
    return size;
}

/* Picture n's header. */
static Tim2PictureHeader *Tim2_GetPicture(u8 *file, s32 index)
{
    s32 i;
    Tim2FileHeader *header;
    Tim2PictureHeader *pict;

    header = (Tim2FileHeader *)file;
    file += Tim2_Align(file, sizeof(Tim2FileHeader));
    pict = (Tim2PictureHeader *)file;
    for (i = 0; i < index; i++) {
        if (i >= header->pictures) {
            break;
        }
        file += Tim2_Align((u8 *)header, sizeof(Tim2PictureHeader));
        file += pict->totalSize;
        pict = (Tim2PictureHeader *)file;
    }
    return pict;
}

/*
 * Level `level` of picture 0's image (pictIndex is not used): after the
 * picture header, its mip-map header and the levels before it, each
 * aligned. Two forms are kept for the match: pict is the picture header
 * as bytes, its mipMaps byte (0x11) read by index (a field read compiles
 * differently), and the level-size index is unsigned (with a signed one
 * MWCC strength-reduces the pointer).
 */
u8 *Tim2_GetImage(u8 *file, s32 pictIndex, s32 level)
{
    u32 *sizes[8];
    u8 *cursor;
    u8 *pict;
    u32 headerSize;
    u32 mipHeaderSize;
    s32 i;
    u32 offset;
    u8 *image;

    cursor = (u8 *)Tim2_GetPicture(file, 0);
    pict = cursor;
    headerSize = sizeof(Tim2PictureHeader);
    if (pict[17] > 1) {
        cursor = pict + sizeof(Tim2PictureHeader);
        for (i = 0; i < pict[17] - 1; i++) {
            sizes[i] = &((Tim2MipHeader *)cursor)->levelSize[(u32)i];
        }
        if (pict[17] <= 4) {
            mipHeaderSize = 32;
        } else {
            mipHeaderSize = 48;
        }
        headerSize += mipHeaderSize;
    }
    cursor = pict + Tim2_Align(file, headerSize);
    if (pict[17] > 1) {
        offset = 0;
        for (i = 0; i < level; i++) {
            offset += Tim2_Align(file, *sizes[i]);
        }
        image = cursor + offset;
    } else {
        image = cursor;
    }
    return image;
}

/* Picture 0's palette (pictIndex is not used): after the picture header,
 * its mip-map header and the image; NULL when it has none. */
u8 *Tim2_GetPictClut(u8 *file, s32 pictIndex)
{
    u8 *cursor;
    Tim2PictureHeader *pict;
    u32 headerSize;
    u32 mipHeaderSize;
    u8 *clut;

    cursor = (u8 *)Tim2_GetPicture(file, 0);
    pict = (Tim2PictureHeader *)cursor;
    headerSize = sizeof(Tim2PictureHeader);
    if (pict->mipMaps > 1) {
        if (pict->mipMaps <= 4) {
            mipHeaderSize = 32;
        } else {
            mipHeaderSize = 48;
        }
        headerSize += mipHeaderSize;
    }
    cursor += Tim2_Align(file, headerSize);
    if (pict->clutSize != 0) {
        clut = cursor + pict->imageSize;
    } else {
        return NULL;
    }
    return clut;
}
