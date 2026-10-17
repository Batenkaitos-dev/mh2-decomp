#ifndef IMAGE_H
#define IMAGE_H

/*
 * An image in EE memory as the texture converter sees it (the pixel
 * accessors at 0x1835E0-0x183BD0, used by src/main/texconv.c): indexed
 * (4 or 8 bits) or direct colour (2, 3 or 4 bytes per pixel, each channel
 * described by its bit count, shift and mask). Colours are passed as
 * 0xAARRGGBB.
 */
#include "common.h"

#define IMAGE_INDEXED 0x04   /* pixels are palette indices */
#define IMAGE_NIBBLE_HI 0x10 /* 4-bit: even pixels in the low nibble when clear */
#define IMAGE_8BIT 0x40      /* indexed: 8 bits per pixel (else 4) */

typedef struct ImageChannel {
    s32 bits;           /* 0 = channel absent */
    s32 shift;
    s32 mask;
} ImageChannel;

typedef struct Image {
    s32 flags;          /* 0x00 IMAGE_* */
    s32 width;          /* 0x04 */
    s32 height;         /* 0x08 */
    s32 pitch;          /* 0x0C bytes per row */
    u8 *pixels;         /* 0x10 */
    s32 bpp;            /* 0x14 bytes per pixel; 0 = 4 bits */
    ImageChannel r;     /* 0x18 */
    ImageChannel g;     /* 0x24 */
    ImageChannel b;     /* 0x30 */
    ImageChannel a;     /* 0x3C */
} Image;

typedef struct ImagePixel {
    s16 x;
    s16 y;
    u32 color;
} ImagePixel;

u8 *Image_PixelAddr(s32 x, s32 y, Image *img);
s32 Image_PutPixel(Image *img, ImagePixel *px);
void Image_SetPixel(Image *img, s32 x, s32 y, u32 color);
u32 Image_GetPixel(s32 x, s32 y, Image *img);
s32 Image_Convert(Image *dst, Image *src);

/*
 * Channel layouts of direct-colour pixels, as the texture code sets them
 * (field by field, in this order): the usual order has red high
 * (0xAARRGGBB; 16 bits: A1 R5 G5 B5); GS memory has red and blue swapped
 * (IMAGE_GS_ORDER_*).
 */
#define IMAGE_RGBA5551(img)                                         \
    (img)->r.bits = 5;                                              \
    (img)->r.shift = 10;                                            \
    (img)->r.mask = 0x1F;                                           \
    (img)->g.bits = 5;                                              \
    (img)->g.shift = 5;                                             \
    (img)->g.mask = 0x1F;                                           \
    (img)->b.bits = 5;                                              \
    (img)->b.shift = 0;                                             \
    (img)->b.mask = 0x1F;                                           \
    (img)->a.bits = 1;                                              \
    (img)->a.shift = 15;                                            \
    (img)->a.mask = 1
#define IMAGE_RGB888(img)                                           \
    (img)->r.bits = 8;                                              \
    (img)->r.shift = 16;                                            \
    (img)->r.mask = 0xFF;                                           \
    (img)->g.bits = 8;                                              \
    (img)->g.shift = 8;                                             \
    (img)->g.mask = 0xFF;                                           \
    (img)->b.bits = 8;                                              \
    (img)->b.shift = 0;                                             \
    (img)->b.mask = 0xFF;                                           \
    (img)->a.bits = 0;                                              \
    (img)->a.shift = 0;                                             \
    (img)->a.mask = 0
#define IMAGE_RGBA8888(img)                                         \
    (img)->r.bits = 8;                                              \
    (img)->r.shift = 16;                                            \
    (img)->r.mask = 0xFF;                                           \
    (img)->g.bits = 8;                                              \
    (img)->g.shift = 8;                                             \
    (img)->g.mask = 0xFF;                                           \
    (img)->b.bits = 8;                                              \
    (img)->b.shift = 0;                                             \
    (img)->b.mask = 0xFF;                                           \
    (img)->a.bits = 8;                                              \
    (img)->a.shift = 24;                                            \
    (img)->a.mask = 0xFF
#define IMAGE_GS_ORDER_16(img)                                      \
    (img)->r.shift = 0;                                             \
    (img)->b.shift = 10;                                            \
    (img)->g.bits = 5;                                              \
    (img)->g.mask = 0x1F
#define IMAGE_GS_ORDER_32(img)                                      \
    (img)->r.shift = 0;                                             \
    (img)->b.shift = 16
#define IMAGE_SET_PITCH(img) (img)->pitch = (img)->width * (img)->bpp

/* A BMP file's header, as Bmp_GetPixels (sub_main unk_2F6540.c) reads it: the
 * signature, then 32-bit fields at 2-byte alignment, read as halves. */
typedef struct BmpFileHeader {
    u16 type;                   /* 0x0: BMP_SIGNATURE */
    u16 size[2];                /* 0x2: the file's bytes */
    u16 reserved[2];            /* 0x6 */
    u16 offBits[2];             /* 0xA: the pixels' offset from the file's start, low half first */
} BmpFileHeader;
#define BMP_SIGNATURE 0x4D42    /* 'BM' */

/*
 * TIM2 pictures, as main's Tim2_* helpers (src/main/unk_18AFE0.c) read them
 * for Tex_CreateFromTim2: a 16-byte file header, then per picture a
 * header, its mip-map header when it has more than one level, the image
 * levels and the palette (CLUT), each part aligned to 16 bytes (format 0)
 * or 128 (format 1).
 */
typedef struct Tim2FileHeader {
    u8 id[4];                   /* 0x00: 'TIM2' ('CLT2' for a palette file) */
    u8 version;                 /* 0x04: 3 or 4 */
    u8 format;                  /* 0x05: 0 = 16-byte alignment, 1 = 128 */
    u16 pictures;               /* 0x06 */
    u8 pad[8];                  /* 0x08 */
} Tim2FileHeader;

/* Tim2PictureHeader.clutType and imageType. */
#define TIM2_TYPE_RGB16 1       /* 16-bit colours (RGBA 5551) */
#define TIM2_TYPE_RGB24 2       /* 24-bit colours */
#define TIM2_TYPE_RGB32 3       /* 32-bit colours */
#define TIM2_TYPE_INDEX4 4      /* imageType only: 4-bit indices (16 colours) */
#define TIM2_TYPE_INDEX8 5      /* imageType only: 8-bit indices (256 colours) */

#define TIM2_MAX_LEVELS 7       /* the most image levels Tim2_DescribeLevels takes */

typedef struct Tim2PictureHeader {
    u32 totalSize;              /* 0x00 */
    u32 clutSize;               /* 0x04 */
    u32 imageSize;              /* 0x08 */
    u16 headerSize;             /* 0x0C */
    u16 clutColors;             /* 0x0E */
    u8 pictFormat;              /* 0x10 */
    u8 mipMaps;                 /* 0x11: image levels, 1 to TIM2_MAX_LEVELS */
    u8 clutType;                /* 0x12: TIM2_TYPE_RGB16 to _RGB32 */
    u8 imageType;               /* 0x13: TIM2_TYPE_* */
    u16 width;                  /* 0x14 */
    u16 height;                 /* 0x16 */
    u64 gsTex0;                 /* 0x18 */
    u64 gsTex1;                 /* 0x20 */
    u32 gsTexaFbaPabe;          /* 0x28 */
    u32 gsTexClut;              /* 0x2C */
} Tim2PictureHeader;

/* After the picture header when there is more than one level: 0x20 bytes
 * for up to 4 levels, 0x30 for more. */
typedef struct Tim2MipHeader {
    u64 gsMiptbp1;              /* 0x00 */
    u64 gsMiptbp2;              /* 0x08 */
    u32 levelSize[8];           /* 0x10 */
} Tim2MipHeader;

#endif /* IMAGE_H */
