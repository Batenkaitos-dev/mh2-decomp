#ifndef LOADING_SCREEN_H
#define LOADING_SCREEN_H

#include "common.h"

/* An image of the loading screen's file (gLoadingImageSizes, one per
 * image): its size in pixels and the GS memory it takes, in blocks. */
typedef struct LoadingImage {
    s32 w;
    s32 h;
    s32 blocks;
} LoadingImage;

/* A sprite of the loading screen (gLoadingSprites, ended by a tex of -1):
 * its texture, its position and size in the 640-wide screen space, and
 * its texel origin and size (a negative tw mirrors it). */
typedef struct LoadingSprite {
    s32 tex;            /* 0x00: the image it draws, -1 ends a list */
    f32 x;              /* 0x04 */
    f32 y;              /* 0x08 */
    s16 w;              /* 0x0C */
    s16 h;              /* 0x0E */
    f32 u;              /* 0x10 */
    f32 v;              /* 0x14 */
    s16 tw;             /* 0x18 */
    s16 th;             /* 0x1A */
} LoadingSprite;

/*
 * The loading screen, the animated picture drawn while the game loads:
 * Loading_Open loads its images into GS memory and builds the packets it
 * draws with, Loading_Draw sends them each frame, and Loading_Close gives
 * the GS memory back.
 */

void Loading_Open(u32 unusedBuf, s32 arg);
void Loading_Close(void);
void Loading_Draw(s32 frames);

#endif
