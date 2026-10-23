#ifndef TIM2_FILE_H
#define TIM2_FILE_H

#include "common.h"
#include "image.h"

/*
 * The TIM2 picture file's readers that Tex_CreateFromTim2 uses: picture 0's
 * image levels (Tim2_DescribeLevels) and palette (Tim2_DescribeClut),
 * described as Images that point into the file.
 */

s32 Tim2_LastLevel(u8 *file);
s32 Tim2_DescribeLevels(Image *image, u8 *file);
s32 Tim2_DescribeClut(Image *image, u8 *file);
u8 *Tim2_GetLevel(u8 *file, s32 level);
u8 *Tim2_GetClut(u8 *file);
s32 Tim2_CheckHeader(u8 *file);
u8 *Tim2_GetImage(u8 *file, s32 pictIndex, s32 level);
u8 *Tim2_GetPictClut(u8 *file, s32 pictIndex);

#endif
