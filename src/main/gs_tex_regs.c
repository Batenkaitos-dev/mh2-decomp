/*
 * The texture registers and the conversions to the GS's scales.
 * Gs_SetTexture builds a texture's registers (Gs_BuildTexRegs: TEXA, TEX1,
 * TEX0, CLAMP, MIPTBP1 and MIPTBP2, from its TexRec, its palette and the
 * render flags) into gTexRegPacket, a static packet of A+D pairs, and
 * sends it with Prim_SendPacket. Then screen coordinates to GS
 * coordinates, a depth to a GS Z value, and a colour to the GS's
 * 0-128 scale. A unit of its own: it was built at optimization level 4
 * with copy propagation, where the unit of the other GS register setters
 * was built without it.
 */
#include "common.h"
#include "syswork.h"
#include "dma.h"
#include "texture.h"
#include "render_state.h"
#include "hw_regs.h"
#include "prim.h"
#include "gs_tex_regs.h"

/* Built at optimization level 4. */
#pragma optimization_level 4

extern s32 gSysFrameMode;
extern f32 gSysZMax;
extern s32 gSysScrollX;
extern s32 gSysScrollY;
extern s32 gRenderFlags;
extern u32 gGsStateTextures[];
/* RS_TEX_LOD_K's value, which Gs_SetState keeps in RS_TEXTURE_2's word */
#define gGsTexLodK (gGsStateTextures[RS_TEXTURE_2 - RS_TEXTURE])
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

/* TEXA's alpha for a texel whose own alpha bit is 0 (AEM 0) and 1 (TA1),
 * both full (128), and the A+D pairs' TCC (the texture's alpha used),
 * CLD (the palette loaded) and CLAMP's region clamp. */
#define TEXA_TA0_FULL 0x80
#define TEXA_TA1_FULL ((u64)128 << 32)
#define TEX0_TCC_RGBA ((u64)1 << 34)
#define TEX0_CLD_LOAD ((u64)2 << 61)
#define CLAMP_REGION 2                  /* WMS and WMT: REGION_CLAMP */
#define MIP_LEVELS_MAX 6                /* the levels after the first that MIPTBP1/2 hold */

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

/*
 * Builds the GS texture registers of handle (its TexRec in the low 16
 * bits, its palette's in the high 16, each 1-based) with the render flags
 * (RF_*): TEXA, TEX1 (the sampling and the mipmaps, RF_TEX_NEAREST and
 * RF_MIPMAP_*, with RS_TEX_LOD_L and RS_TEX_LOD_K), TEX0 (the address,
 * size and format, and the palette's), CLAMP (repeat, or RF_TEX_CLAMP's
 * region clamp) and MIPTBP1/2 (the levels' addresses). Returns 0 when the
 * texture or its palette is not in GS memory, else 1.
 */
s32 Gs_BuildTexRegs(u32 handle, u64 *texAlphaReg, u64 *texLodReg, u64 *texBaseReg, u64 *clampReg, u64 *mipBaseLow, u64 *mipBaseHigh, u32 flags)
{
    TexRec *texRec;
    TexRec *pal;
    s16 bufWidth;
    s32 i;
    s32 levelWidth;
    s32 *levelVramOut;
    s32 *levelBufWidthOut;
    s32 levelVram[MIP_LEVELS_MAX];
    s32 levelBufWidth[MIP_LEVELS_MAX];

    texRec = &gTexRecs[TEX_HANDLE_TEX(handle) - 1];
    if (TEX_HANDLE_TEX(handle) == 0) {
        return 0;
    }
    if (texRec->vram == 0) {
        return 0;
    }
    if (texRec->resident == 0) {
        return 0;
    }
    if (texRec->flags & TEX_FLAG_PALETTE) {
        if (TEX_HANDLE_PAL(handle) == 0) {
            return 0;
        }
        pal = &gPalRecs[TEX_HANDLE_PAL(handle) - 1];
        if (pal->vram == 0) {
            return 0;
        }
        if (pal->resident == 0) {
            return 0;
        }
    }
    if ((flags & RF_MIPMAP_MASK) == 0 || texRec->levels == 1) {
        if ((flags & RF_TEX_NEAREST) == RF_TEX_NEAREST) {
            *texLodReg = 0;
        } else {
            *texLodReg = TEX1_MMAG_LINEAR | TEX1_MMIN(TEX1_MIN_LINEAR);
        }
    } else if ((flags & RF_MIPMAP_MASK) == RF_MIPMAP_NEAREST) {
        if ((flags & RF_TEX_NEAREST) == RF_TEX_NEAREST) {
            *texLodReg = TEX1_MXL(texRec->levels - 1) | TEX1_MMIN(TEX1_MIN_NEAREST_MIP_NEAREST) |
                    TEX1_L(gGsTexLodL) | TEX1_K(gGsTexLodK);
        } else {
            *texLodReg = TEX1_MXL(texRec->levels - 1) | (TEX1_MMAG_LINEAR | TEX1_MMIN(TEX1_MIN_LINEAR_MIP_NEAREST)) |
                    TEX1_L(gGsTexLodL) | TEX1_K(gGsTexLodK);
        }
    } else {
        if ((flags & RF_TEX_NEAREST) == RF_TEX_NEAREST) {
            *texLodReg = TEX1_MXL(texRec->levels - 1) | TEX1_MMIN(TEX1_MIN_NEAREST_MIP_LINEAR) |
                    TEX1_L(gGsTexLodL) | TEX1_K(gGsTexLodK);
        } else {
            *texLodReg = TEX1_MXL(texRec->levels - 1) | (TEX1_MMAG_LINEAR | TEX1_MMIN(TEX1_MIN_LINEAR_MIP_LINEAR)) |
                    TEX1_L(gGsTexLodL) | TEX1_K(gGsTexLodK);
        }
    }
    bufWidth = texRec->width / 64;
    if (bufWidth == 0) {
        bufWidth = 1;
    }
    if (texRec->flags & TEX_FLAG_PALETTE) {
        pal = &gPalRecs[TEX_HANDLE_PAL(handle) - 1];
        if (pal->fmt == TEX_FMT_NO_ALPHA) {
            *texAlphaReg = TEXA_TA0_FULL | TEXA_TA1_FULL;
        } else {
            *texAlphaReg = TEXA_TA1_FULL;
        }
        *texBaseReg = (u64)texRec->vram | ((u64)bufWidth << 14) | ((u64)texRec->psm << 20) | ((u64)texRec->tw << 26) |
                ((u64)texRec->th << 30) | TEX0_TCC_RGBA | ((u64)pal->vram << 37) |
                ((u64)pal->psm << 51) | TEX0_CLD_LOAD;
    } else {
        if (texRec->fmt == TEX_FMT_NO_ALPHA) {
            *texAlphaReg = TEXA_TA0_FULL | TEXA_TA1_FULL;
        } else {
            *texAlphaReg = TEXA_TA1_FULL;
        }
        *texBaseReg = (u64)texRec->vram | ((u64)bufWidth << 14) | ((u64)texRec->psm << 20) | ((u64)texRec->tw << 26) |
                ((u64)texRec->th << 30) | TEX0_TCC_RGBA;
    }
    switch (flags & RF_TEX_CLAMP_MASK) {
    case 0:
        *clampReg = 0;
        break;
    case RF_TEX_CLAMP:
    default:
        *clampReg = CLAMP_REGION | (CLAMP_REGION << 2) | ((u64)texRec->width << 14) | ((u64)texRec->height << 34);
        break;
    }
    *mipBaseLow = 0;
    *mipBaseHigh = 0;
    if (texRec->levels > 1) {
        for (i = 0; i < MIP_LEVELS_MAX; i++) {
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

/* Screen x to GS x: subtracts the scroll (gSysScrollX). */
s16 Gs_ScreenX(s16 x)
{
    x -= gSysScrollX;
    return x;
}

/*
 * Screen y to GS y: subtracts the scroll (gSysScrollY) and halves it in
 * field mode (gSysFrameMode FRAME_MODE_FIELD).
 */
s16 Gs_ScreenY(s16 y)
{
    y -= gSysScrollY;
    if (gSysFrameMode == FRAME_MODE_FIELD) {
        y >>= 1;
    }
    return y;
}

/* Gs_ScreenX for a float. */
f32 Gs_ScreenXf(f32 x)
{
    x -= gSysScrollX;
    return x;
}

/*
 * Gs_ScreenY for a float: subtracts gSysScrollY and halves the result in
 * field mode (gSysFrameMode FRAME_MODE_FIELD).
 */
f32 Gs_ScreenYf(f32 y)
{
    y -= gSysScrollY;
    if (gSysFrameMode == FRAME_MODE_FIELD) {
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
 * 128 is full: 255 becomes 128, other nonzero values are halved (at
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
            if (*channelPtr == 255) {
                *channelPtr = 128;
            } else if (*channelPtr != 0) {
                *channelPtr >>= 1;
                if (*channelPtr == 0) {
                    *channelPtr = 1;
                }
            }
        }
    } else {
        if (channels[3] == 255) {
            channels[3] = 128;
        } else if (channels[3] != 0) {
            channels[3] >>= 1;
            if (channels[3] == 0) {
                channels[3] = 1;
            }
        }
    }
    return channels[2] | ((channels[1] << 8) | ((channels[3] << 24) | (channels[0] << 16)));
}

/* gTexRegPacket: the DMA and GIF tags, then the A+D pairs Gs_BuildTexRegs
 * fills, each a register's value and its register word (gTexReg*Addr, a u64). */
u8 gTexRegPacket[32] = {
#include "main/001BAB30.inc"
};
#pragma explicit_zero_data on
u64 gTexRegTexa = 0;
#pragma explicit_zero_data reset
u64 gTexRegTexaAddr[1] = {
#include "main/001BAB58.inc"
};
#pragma explicit_zero_data on
u64 gTexRegTex1 = 0;
#pragma explicit_zero_data reset
u64 gTexRegTex1Addr[1] = {
#include "main/001BAB68.inc"
};
#pragma explicit_zero_data on
u64 gTexRegTex0 = 0;
#pragma explicit_zero_data reset
u64 gTexRegTex0Addr[1] = {
#include "main/001BAB78.inc"
};
#pragma explicit_zero_data on
u64 gTexRegClamp = 0;
#pragma explicit_zero_data reset
u64 gTexRegClampAddr[1] = {
#include "main/001BAB88.inc"
};
#pragma explicit_zero_data on
u64 gTexRegMiptbp1 = 0;
#pragma explicit_zero_data reset
u64 gTexRegMiptbp1Addr[1] = {
#include "main/001BAB98.inc"
};
#pragma explicit_zero_data on
u64 gTexRegMiptbp2 = 0;
#pragma explicit_zero_data reset
u64 gTexRegMiptbp2Addr[1] = {
#include "main/001BABA8.inc"
};
