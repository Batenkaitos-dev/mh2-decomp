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

#endif /* IMAGE_H */
