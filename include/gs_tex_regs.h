#ifndef GS_TEX_REGS_H
#define GS_TEX_REGS_H

#include "common.h"

/*
 * A texture's GS registers and the conversions to the GS's scales.
 * Gs_SetTexture sends a texture's registers (Gs_BuildTexRegs builds them
 * from the texture, its palette and the render flags); Gs_ScreenX and the
 * others turn screen coordinates, a depth and a colour into the GS's.
 */

s32 Gs_SetTexture(s32 handle);
s32 Gs_BuildTexRegs(u32 handle, u64 *texAlphaReg, u64 *texLodReg, u64 *texBaseReg, u64 *clampReg, u64 *mipBaseLow, u64 *mipBaseHigh, u32 flags);
s16 Gs_ScreenX(s16 x);
s16 Gs_ScreenY(s16 y);
f32 Gs_ScreenXf(f32 x);
f32 Gs_ScreenYf(f32 y);
f32 Gs_DepthToZ(f32 z);
u32 Gs_ColorToGs(u32 color, s32 alphaOnly);

#endif
