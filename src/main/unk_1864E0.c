/*
 * The texture registers, which go into a static
 * packet sent with Prim_SendPacket, and the conversions from screen to GS
 * coordinates. A unit of its own: it was built at optimization level 4
 * with copy propagation, where the unit of the other GS register setters
 * was built without it.
 */
#include "common.h"
#include "dma.h"
#include "texture.h"

/* Built at optimization level 4. */
#pragma optimization_level 4

extern s32 gSysFrameMode;
extern f32 gSysZMax;
extern s32 gSysScrollX;
extern s32 gSysScrollY;
extern s32 gRenderFlags;
extern u32 D_0020E138;
extern u32 gGsTexLodL;
extern u8 gTexRegPacket[];
extern u64 gTexRegTexa;
extern u64 gTexRegTex1;
extern u64 gTexRegTex0;
extern u64 gTexRegClamp;
extern u64 gTexRegMiptbp1;
extern u64 gTexRegMiptbp2;
s32 Gs_BuildTexRegs(u32 handle, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2,
                  u32 flags);
s32 Prim_SendPacket(u32 *pkt);

/*
 * Builds the texture registers of handle with the render flags
 * (gRenderFlags) into gTexRegPacket and sends it (Prim_SendPacket).
 * Returns 1, or 0 (nothing sent) when the texture or its palette is not
 * in GS memory.
 */
s32 Gs_SetTexture(s32 handle)
{
    if (Gs_BuildTexRegs(handle, &gTexRegTexa, &gTexRegTex1, &gTexRegTex0, &gTexRegClamp, &gTexRegMiptbp1, &gTexRegMiptbp2, gRenderFlags) == 0) {
        return 0;
    }
    Prim_SendPacket((u32 *) gTexRegPacket);
    return 1;
}

s32 Gs_BuildTexRegs(u32 handle, u64 *texAlphaReg, u64 *texLodReg, u64 *texBaseReg, u64 *clampReg, u64 *mipBaseLow, u64 *mipBaseHigh, u32 flags)
{
    TexRec *texRec;
    TexRec *pal;
    s16 bufWidth;
    s32 i;
    s32 levelWidth;
    s32 *levelVramOut;
    s32 *levelBufWidthOut;
    s32 levelVram[6];
    s32 levelBufWidth[6];

    texRec = &gTexRecs[(handle & 0xFFFF) - 1];
    if ((handle & 0xFFFF) == 0) {
        return 0;
    }
    if (texRec->vram == 0) {
        return 0;
    }
    if (texRec->resident == 0) {
        return 0;
    }
    if (texRec->flags & 4) {
        if (((handle & 0xFFFF0000) >> 16) == 0) {
            return 0;
        }
        pal = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
        if (pal->vram == 0) {
            return 0;
        }
        if (pal->resident == 0) {
            return 0;
        }
    }
    if ((flags & 0xC00000) == 0 || texRec->levels == 1) {
        if ((flags & 0x10000) == 0x10000) {
            *texLodReg = 0;
        } else {
            *texLodReg = 0x60;
        }
    } else if ((flags & 0xC00000) == 0x400000) {
        if ((flags & 0x10000) == 0x10000) {
            *texLodReg = ((u64)(texRec->levels - 1) << 2) | 0x80 | ((u64)gGsTexLodL << 19) |
                    ((u64)((D_0020E138 << 4) & 0xFFF) << 32);
        } else {
            *texLodReg = ((u64)(texRec->levels - 1) << 2) | 0x120 | ((u64)gGsTexLodL << 19) |
                    ((u64)((D_0020E138 << 4) & 0xFFF) << 32);
        }
    } else {
        if ((flags & 0x10000) == 0x10000) {
            *texLodReg = ((u64)(texRec->levels - 1) << 2) | 0xC0 | ((u64)gGsTexLodL << 19) |
                    ((u64)((D_0020E138 << 4) & 0xFFF) << 32);
        } else {
            *texLodReg = ((u64)(texRec->levels - 1) << 2) | 0x160 | ((u64)gGsTexLodL << 19) |
                    ((u64)((D_0020E138 << 4) & 0xFFF) << 32);
        }
    }
    bufWidth = texRec->width / 64;
    if (bufWidth == 0) {
        bufWidth = 1;
    }
    if (texRec->flags & 4) {
        pal = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
        if (pal->fmt == 3) {
            *texAlphaReg = 0x80 | ((u64)0x80 << 32);
        } else {
            *texAlphaReg = (u64)0x80 << 32;
        }
        *texBaseReg = (u64)texRec->vram | ((u64)bufWidth << 14) | ((u64)texRec->psm << 20) | ((u64)texRec->tw << 26) |
                ((u64)texRec->th << 30) | ((u64)1 << 34) | ((u64)pal->vram << 37) |
                ((u64)pal->psm << 51) | ((u64)2 << 61);
    } else {
        if (texRec->fmt == 3) {
            *texAlphaReg = 0x80 | ((u64)0x80 << 32);
        } else {
            *texAlphaReg = (u64)0x80 << 32;
        }
        *texBaseReg = (u64)texRec->vram | ((u64)bufWidth << 14) | ((u64)texRec->psm << 20) | ((u64)texRec->tw << 26) |
                ((u64)texRec->th << 30) | ((u64)1 << 34);
    }
    switch (flags & 0x60000) {
    case 0:
        *clampReg = 0;
        break;
    case 0x20000:
    default:
        *clampReg = 2 | (2 << 2) | ((u64)texRec->width << 14) | ((u64)texRec->height << 34);
        break;
    }
    *mipBaseLow = 0;
    *mipBaseHigh = 0;
    if (texRec->levels > 1) {
        for (i = 0; i < 6; i++) {
            levelVram[i] = 0;
            levelBufWidth[i] = 0;
        }
        levelVramOut = levelVram;
        levelBufWidthOut = levelBufWidth;
        levelWidth = texRec->width >> 1;
        for (i = 0; i < texRec->levels - 1; i++) {
            *levelVramOut = Tex_LevelVram(texRec, i + 1);
            *levelBufWidthOut = levelWidth / 64;
            if (*levelBufWidthOut == 0) {
                *levelBufWidthOut = 1;
            }
            levelWidth >>= 1;
            levelVramOut++;
            levelBufWidthOut++;
        }
        *mipBaseLow = (u64)levelVram[0] | ((u64)levelBufWidth[0] << 14) | ((u64)levelVram[1] << 20) | ((u64)levelBufWidth[1] << 34) |
                ((u64)levelVram[2] << 40) | ((u64)levelBufWidth[2] << 54);
        *mipBaseHigh = (u64)levelVram[3] | ((u64)levelBufWidth[3] << 14) | ((u64)levelVram[4] << 20) | ((u64)levelBufWidth[4] << 34) |
                ((u64)levelVram[5] << 40) | ((u64)levelBufWidth[5] << 54);
    }
    return 1;
}

s16 Gs_ScreenX(s16 x)
{
    x -= gSysScrollX;
    return x;
}

/*
 * Screen y to GS y: subtracts the scroll (gSysScrollY) and halves it in
 * field mode (gSysFrameMode 0).
 */
s16 Gs_ScreenY(s16 y)
{
    y -= gSysScrollY;
    if (gSysFrameMode == 0) {
        y >>= 1;
    }
    return y;
}

f32 Gs_ScreenXf(f32 x)
{
    x -= gSysScrollX;
    return x;
}

/*
 * Gs_ScreenY for a float: subtracts gSysScrollY and halves the result in
 * field mode (gSysFrameMode 0).
 */
f32 Gs_ScreenYf(f32 y)
{
    y -= gSysScrollY;
    if (gSysFrameMode == 0) {
        y *= 0.5f;
    }
    return y;
}

/*
 * Depth z in -1..1 to a GS Z value: (1 - z) / 2 times gSysZMax, so 1 maps
 * to 0 and -1 to gSysZMax.
 */
f32 Gs_DepthToZ(f32 z)
{
    z -= 1.0f;
    z *= -0.5f;
    z *= gSysZMax;
    return z;
}

/*
 * Converts a 0xAARRGGBB colour with 0-255 channels to the GS scale where
 * 0x80 is full: 0xFF becomes 0x80, other nonzero values are halved (at
 * least 1). alphaOnly nonzero converts only the alpha. Returns the result
 * in the same layout.
 */
u32 Gs_ColorToGs(u32 color, s32 alphaOnly)
{
    u8 channels[4];
    s32 i;
    u8 *channelPtr;

    channels[0] = color >> 16;
    channels[1] = color >> 8;
    channels[2] = color;
    channels[3] = color >> 24;
    if (alphaOnly == 0) {
        for (i = 0, channelPtr = channels; i < 4; i++, channelPtr++) {
            if (*channelPtr == 0xFF) {
                *channelPtr = 0x80;
            } else if (*channelPtr != 0) {
                *channelPtr >>= 1;
                if (*channelPtr == 0) {
                    *channelPtr = 1;
                }
            }
        }
    } else {
        if (channels[3] == 0xFF) {
            channels[3] = 0x80;
        } else if (channels[3] != 0) {
            channels[3] >>= 1;
            if (channels[3] == 0) {
                channels[3] = 1;
            }
        }
    }
    return channels[2] | ((channels[1] << 8) | ((channels[3] << 24) | (channels[0] << 16)));
}

/* This file's initialised data. */
u8 gTexRegPacket[32] = {
#include "main/001BAB30.inc"
};
#pragma explicit_zero_data on
u64 gTexRegTexa = 0;
#pragma explicit_zero_data reset
u32 D_001BAB58[2] = {
#include "main/001BAB58.inc"
};
#pragma explicit_zero_data on
u64 gTexRegTex1 = 0;
#pragma explicit_zero_data reset
u32 D_001BAB68[2] = {
#include "main/001BAB68.inc"
};
#pragma explicit_zero_data on
u64 gTexRegTex0 = 0;
#pragma explicit_zero_data reset
u32 D_001BAB78[2] = {
#include "main/001BAB78.inc"
};
#pragma explicit_zero_data on
u64 gTexRegClamp = 0;
#pragma explicit_zero_data reset
u32 D_001BAB88[2] = {
#include "main/001BAB88.inc"
};
#pragma explicit_zero_data on
u64 gTexRegMiptbp1 = 0;
#pragma explicit_zero_data reset
u32 D_001BAB98[2] = {
#include "main/001BAB98.inc"
};
#pragma explicit_zero_data on
u64 gTexRegMiptbp2 = 0;
#pragma explicit_zero_data reset
u32 D_001BABA8[2] = {
#include "main/001BABA8.inc"
};
