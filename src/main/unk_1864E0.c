/*
 * main 0x1864e0-0x186d10: the texture registers, which go into a static
 * packet sent with Prim_SendPacket, and the conversions from screen to GS
 * coordinates. A unit of its own: it was built at optimization level 4
 * with copy propagation, where unk_185460.c (the other GS register
 * setters) was built without copy propagation.
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

s32 Gs_SetTexture(s32 handle)
{
    if (Gs_BuildTexRegs(handle, &gTexRegTexa, &gTexRegTex1, &gTexRegTex0, &gTexRegClamp, &gTexRegMiptbp1, &gTexRegMiptbp2, gRenderFlags) == 0) {
        return 0;
    }
    Prim_SendPacket((u32 *) gTexRegPacket);
    return 1;
}

s32 Gs_BuildTexRegs(u32 handle, u64 *texa, u64 *tex1, u64 *tex0, u64 *clamp, u64 *mip1, u64 *mip2, u32 flags)
{
    TexRec *t;
    TexRec *pal;
    s16 bw;
    s32 i;
    s32 w;
    s32 *pv;
    s32 *pw;
    s32 vram[6];
    s32 bws[6];

    t = &gTexRecs[(handle & 0xFFFF) - 1];
    if ((handle & 0xFFFF) == 0) {
        return 0;
    }
    if (t->vram == 0) {
        return 0;
    }
    if (t->resident == 0) {
        return 0;
    }
    if (t->flags & 4) {
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
    if ((flags & 0xC00000) == 0 || t->levels == 1) {
        if ((flags & 0x10000) == 0x10000) {
            *tex1 = 0;
        } else {
            *tex1 = 0x60;
        }
    } else if ((flags & 0xC00000) == 0x400000) {
        if ((flags & 0x10000) == 0x10000) {
            *tex1 = ((u64)(t->levels - 1) << 2) | 0x80 | ((u64)gGsTexLodL << 19) |
                    ((u64)((D_0020E138 << 4) & 0xFFF) << 32);
        } else {
            *tex1 = ((u64)(t->levels - 1) << 2) | 0x120 | ((u64)gGsTexLodL << 19) |
                    ((u64)((D_0020E138 << 4) & 0xFFF) << 32);
        }
    } else {
        if ((flags & 0x10000) == 0x10000) {
            *tex1 = ((u64)(t->levels - 1) << 2) | 0xC0 | ((u64)gGsTexLodL << 19) |
                    ((u64)((D_0020E138 << 4) & 0xFFF) << 32);
        } else {
            *tex1 = ((u64)(t->levels - 1) << 2) | 0x160 | ((u64)gGsTexLodL << 19) |
                    ((u64)((D_0020E138 << 4) & 0xFFF) << 32);
        }
    }
    bw = t->width / 64;
    if (bw == 0) {
        bw = 1;
    }
    if (t->flags & 4) {
        pal = &gPalRecs[((handle & 0xFFFF0000) >> 16) - 1];
        if (pal->fmt == 3) {
            *texa = 0x80 | ((u64)0x80 << 32);
        } else {
            *texa = (u64)0x80 << 32;
        }
        *tex0 = (u64)t->vram | ((u64)bw << 14) | ((u64)t->psm << 20) | ((u64)t->tw << 26) |
                ((u64)t->th << 30) | ((u64)1 << 34) | ((u64)pal->vram << 37) |
                ((u64)pal->psm << 51) | ((u64)2 << 61);
    } else {
        if (t->fmt == 3) {
            *texa = 0x80 | ((u64)0x80 << 32);
        } else {
            *texa = (u64)0x80 << 32;
        }
        *tex0 = (u64)t->vram | ((u64)bw << 14) | ((u64)t->psm << 20) | ((u64)t->tw << 26) |
                ((u64)t->th << 30) | ((u64)1 << 34);
    }
    switch (flags & 0x60000) {
    case 0:
        *clamp = 0;
        break;
    case 0x20000:
    default:
        *clamp = 2 | (2 << 2) | ((u64)t->width << 14) | ((u64)t->height << 34);
        break;
    }
    *mip1 = 0;
    *mip2 = 0;
    if (t->levels > 1) {
        for (i = 0; i < 6; i++) {
            vram[i] = 0;
            bws[i] = 0;
        }
        pv = vram;
        pw = bws;
        w = t->width >> 1;
        for (i = 0; i < t->levels - 1; i++) {
            *pv = Tex_LevelVram(t, i + 1);
            *pw = w / 64;
            if (*pw == 0) {
                *pw = 1;
            }
            w >>= 1;
            pv++;
            pw++;
        }
        *mip1 = (u64)vram[0] | ((u64)bws[0] << 14) | ((u64)vram[1] << 20) | ((u64)bws[1] << 34) |
                ((u64)vram[2] << 40) | ((u64)bws[2] << 54);
        *mip2 = (u64)vram[3] | ((u64)bws[3] << 14) | ((u64)vram[4] << 20) | ((u64)bws[4] << 34) |
                ((u64)vram[5] << 40) | ((u64)bws[5] << 54);
    }
    return 1;
}

s16 Gs_ScreenX(s16 x)
{
    x -= gSysScrollX;
    return x;
}

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

f32 Gs_ScreenYf(f32 y)
{
    y -= gSysScrollY;
    if (gSysFrameMode == 0) {
        y *= 0.5f;
    }
    return y;
}

f32 Gs_DepthToZ(f32 z)
{
    z -= 1.0f;
    z *= -0.5f;
    z *= gSysZMax;
    return z;
}

u32 Gs_ColorToGs(u32 color, s32 alphaOnly)
{
    u8 c[4];
    s32 i;
    u8 *p;

    c[0] = color >> 16;
    c[1] = color >> 8;
    c[2] = color;
    c[3] = color >> 24;
    if (alphaOnly == 0) {
        for (i = 0, p = c; i < 4; i++, p++) {
            if (*p == 0xFF) {
                *p = 0x80;
            } else if (*p != 0) {
                *p >>= 1;
                if (*p == 0) {
                    *p = 1;
                }
            }
        }
    } else {
        if (c[3] == 0xFF) {
            c[3] = 0x80;
        } else if (c[3] != 0) {
            c[3] >>= 1;
            if (c[3] == 0) {
                c[3] = 1;
            }
        }
    }
    return c[2] | ((c[1] << 8) | ((c[3] << 24) | (c[0] << 16)));
}

/* unk_1864E0's .data (0x1BAB30-0x1BABB0) */
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
