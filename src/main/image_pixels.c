/*
 * The image pixel helpers the texture loaders convert with: a pixel's
 * address (Image_PixelAddr), writing one as a palette index or a colour
 * (Image_PutPixel, Image_SetPixel), reading one as 0xAARRGGBB
 * (Image_GetPixel), and copying an image into another's format
 * (Image_Convert). The file ends with two small set-ups the system's start
 * calls: the memory card library's (Sys_InitCard) and Packet_SetMode1Ok.
 */
#include "common.h"
#include "image.h"
#include "mem_util.h"
#include "libmc.h"

extern u8 gCardWork[];
extern u8 gCardRecords[];
extern s32 gPacketMode;
/* The memory card work Sys_InitCard clears: its sizes in bytes. */
#define CARD_WORK_SIZE 100

#define CARD_RECORDS 2
#define CARD_RECORD_SIZE 12

/* The address of pixel (x, y) of an image, 0 outside it. */
u8 *Image_PixelAddr(s32 x, s32 y, Image *img)
{
    if (x < 0 || y < 0 || x >= img->width || y >= img->height) {
        return NULL;
    }
    if (img->bpp == 0) {
        if (img->flags & IMAGE_8BIT) {
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
        packed = ((red * img->r.mask / 255) << img->r.shift) | ((green * img->g.mask / 255) << img->g.shift) |
            ((blue * img->b.mask / 255) << img->b.shift) | ((alpha * img->a.mask / 255) << img->a.shift);
        switch (img->bpp) {
        case 2:
            *(u16 *)dst = packed;
            break;
        case 3:
            /* 24-bit pixels are three bytes, written one at a time */
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
 * image, else each channel scaled to 8 bits (alpha 255 when absent). */
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
        /* 24-bit pixels are three bytes, read one at a time */
        raw = src[0] | (src[2] << 16 | src[1] << 8);
        break;
    case 4:
        raw = *(u32 *)src;
        break;
    }
    if (img->a.bits) {
        alpha = ((img->a.mask & (raw >> img->a.shift)) * 255) / (u32)img->a.mask;
    } else {
        alpha = 255;
    }
    if (img->r.bits) {
        red = ((img->r.mask & (raw >> img->r.shift)) * 255) / (u32)img->r.mask;
    } else {
        red = 0;
    }
    if (img->g.bits) {
        green = ((img->g.mask & (raw >> img->g.shift)) * 255) / (u32)img->g.mask;
    } else {
        green = 0;
    }
    if (img->b.bits) {
        blue = ((img->b.mask & (raw >> img->b.shift)) * 255) / (u32)img->b.mask;
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
 * Sets up the memory card library: calls sceMcInit again while it
 * returns -101, then clears gCardWork and the two records of
 * gCardRecords (no code reads either). Returns 1. Called at the end of
 * the system's display and DMA set-up.
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
        case -101:
            continue;
        }
        break;
    }
    Mem_Fill(gCardWork, 0, CARD_WORK_SIZE);
    for (i = 0, record = gCardRecords; i < CARD_RECORDS; i++, record += CARD_RECORD_SIZE) {
        Mem_Fill(record, 0, CARD_RECORD_SIZE);
    }
    return 1;
}

/* Sets gPacketMode to 1 and returns 1. */
s32 Packet_SetMode1Ok(void)
{
    gPacketMode = 1;
    return 1;
}
