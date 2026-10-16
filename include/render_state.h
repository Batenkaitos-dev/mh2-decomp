#ifndef RENDER_STATE_H
#define RENDER_STATE_H

/*
 * The render states: the first argument of main's Gs_SetState
 * (unk_183D20.c), which keeps each in the render flags (gRenderFlags,
 * gRenderFlags2) or a global and writes the GS register it feeds. The
 * names below come from what Gs_SetState does with each. The states it
 * keeps only as bits of the render flags have no name yet (seen, the
 * render flag bits each sets: 0 -> gRenderFlags2 0x60; 1 -> 0xF00; 2 ->
 * 0x1C; 0xD -> gRenderFlags 0xC00, 0x5E -> 0xFF and 0x61 -> 0x1000000,
 * each through Gs_SetAlpha; 0x12 -> gRenderFlags2 0x8000; 0x15 -> 2;
 * 0x5D -> the top byte; 0x5F -> gRenderFlags 0x380000 and 0x6D -> 0x7000,
 * through Gs_SetTest; 0x62 -> gRenderFlags2 0x4000; 0x64 -> gRenderFlags
 * 0x20000; 0x66 -> gRenderFlags2 0x20000; 0x6B -> gRenderFlags 0xC00000;
 * 0x6C -> 0x8000, through Gs_SetZbuf). States 3, 8-0xB, 0x13, 0x65 and
 * 0x68 do nothing.
 */
#define RS_TEXTURE 4                /* a texture handle: bound (Gs_SetTexture) and marked in use */
#define RS_TEXTURE_1 5              /* 5-7: texture handles only kept (gGsStateTextures[1..3]) */
#define RS_TEXTURE_2 6
#define RS_TEXTURE_3 7
#define RS_SCISSOR 0xC              /* 1: scissor to the whole screen (Gs_SetScissor) */
#define RS_COLOR 0xE                /* a colour, 0xAARRGGBB (gGsColor0E; also kept as floats) */
#define RS_FOG_COLOR 0xF            /* the fog's colour (Gs_SetFogColor) */
#define RS_FOG_NEAR 0x10            /* the fog's near distance, an f32 passed as its bits */
#define RS_FOG_FAR 0x11             /* the fog's far distance, an f32 passed as its bits */
#define RS_BG_COLOR 0x14            /* the background colour (Gs_SetBgColor) */
#define RS_VIEW_MTX 0x16            /* the address of the view matrix to copy (gMtxView) */
#define RS_PROJ_MTX 0x17            /* the address of the projection matrix to copy (gMtxCamProj) */
#define RS_ORTHO 0x18               /* the address of an orthographic view: 4 s32, 2 f32 (Camera_SetOrtho) */
#define RS_MTX_SLOT(i) (0x1A + (i)) /* 0x1A-0x39: the address of a matrix for gMtxSlots[i] */
#define RS_MATERIAL(i) (0x3A + (i)) /* 0x3A-0x59: the address of a ModelMat for gRsMaterials[i] */
#define RS_LIGHT(i) (0x5A + (i))    /* 0x5A-0x5C: the address of a Light for gRsLights[i] */
#define RS_ALPHA_REF 0x60           /* the alpha test's reference value (gGsAlphaRef; Gs_SetTest) */
#define RS_TEX_FILTER 0x63          /* 0x10000: filtered textures, 0: not (Gs_SetTexFilter; Gfx_SetState63) */
#define RS_COLOR2 0x67              /* a second colour, 0xAARRGGBB (gGsColor67; also kept as floats) */
#define RS_TEX_LOD_L 0x69           /* the textures' LOD parameter L (gGsTexLodL) */

#endif /* RENDER_STATE_H */
