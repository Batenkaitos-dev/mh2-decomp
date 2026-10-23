#ifndef RENDER_STATE_H
#define RENDER_STATE_H

/*
 * The render states: the first argument of main's Gs_SetState
 *, which keeps each in the render flags (gRenderFlags,
 * gRenderFlags2) or a global and writes the GS register it feeds. The
 * names below come from what Gs_SetState does with each. The states it
 * keeps only as bits of the render flags have no name yet (seen, the
 * render flag bits each sets: 0 -> gRenderFlags2 0x60; 1 -> 0xF00; 2 ->
 * 0x1C; 18 -> gRenderFlags2 0x8000; 21 -> 2; 93 -> the top byte;
 * 98 -> gRenderFlags2 0x4000; 102 -> gRenderFlags2 0x20000).
 * gRenderFlags2's bits are the keys that pick a model's VU1 program
 * (Vu1_SelectProgram). States 3, 8-11, 19, 101 and 104 do nothing.
 */
#define RS_TEXTURE 4                /* a texture handle: bound (Gs_SetTexture) and marked in use */
#define RS_TEXTURE_1 5              /* 5-7: texture handles only kept (gGsStateTextures[1..3]) */
#define RS_TEXTURE_2 6
#define RS_TEXTURE_3 7
#define RS_SCISSOR 0xC              /* 1: scissor to the whole screen (Gs_SetScissor) */
#define RS_COLOR 0xE                /* a colour, 0xAARRGGBB (gGsColor; also kept as floats) */
#define RS_FOG_COLOR 0xF            /* the fog's colour (Gs_SetFogColor) */
#define RS_FOG_NEAR 0x10            /* the fog's near distance, an f32 passed as its bits */
#define RS_FOG_FAR 0x11             /* the fog's far distance, an f32 passed as its bits */
#define RS_BG_COLOR 0x14            /* the background colour (Gs_SetBgColor) */
#define RS_VIEW_MTX 0x16            /* the address of the view matrix to copy (gMtxView) */
#define RS_PROJ_MTX 0x17            /* the address of the projection matrix to copy (gMtxCamProj) */
#define RS_ORTHO 0x18               /* the address of an orthographic view: 4 s32, 2 f32 (Camera_SetOrtho) */
#define RS_UV_MTX 0x19              /* the address of a texture coordinate matrix to copy (gMtxUv): the VU1 programs map ST through rows 0, 1 and 3 */
#define RS_MTX_SLOT(i) (0x1A + (i)) /* states 26-57: the address of a matrix for gMtxSlots[i] */
#define RS_MATERIAL(i) (0x3A + (i)) /* states 58-89: the address of a ModelMat for gRsMaterials[i] */
#define RS_LIGHT(i) (0x5A + (i))    /* states 90-92: the address of a Light for gRsLights[i] */
#define RS_BLEND 0x5E               /* the blend equation, RS_BLEND_* (gRenderFlags 0xFF; Gs_SetAlpha) */
#define RS_ALPHA_TEST 0x5F          /* the alpha test's comparison, RS_CMP_* (gRenderFlags 0x380000; Gs_SetTest) */
#define RS_ALPHA_REF 0x60           /* the alpha test's reference value (gGsAlphaRef; Gs_SetTest) */
#define RS_TEX_FILTER 0x63          /* RF_TEX_NEAREST: point-sampled textures; 0: bilinear (Gs_SetTexFilter; Gfx_SetTexFilter) */
#define RS_COLOR2 0x67              /* a second colour, 0xAARRGGBB (gGsColor2; also kept as floats) */
#define RS_TEX_LOD_L 0x69           /* the textures' LOD parameter L (gGsTexLodL) */
#define RS_ZWRITE 0x6C              /* 1: write the depth buffer, 0: mask it (gRenderFlags 0x8000; Gs_SetZbuf) */
#define RS_ZTEST 0x6D               /* the depth test's comparison, RS_CMP_* (gRenderFlags 0x7000; Gs_SetTest) */
#define RS_BLEND_OP 0xD             /* the blend's family (gRenderFlags 0xC00; Gs_SetAlpha): Gfx_SetBlendOp passes gBlendOps' values */
#define RS_BLEND_ON 0x61            /* RF_BLEND_ON: blend; 0: draw the source colour as is (Gs_SetAlpha) */
#define RS_TEX_CLAMP 0x64           /* RF_TEX_CLAMP: clamp the screen texture to its size; 0: repeat it (ScreenTex_SetRegs) */
#define RS_TEX_LOD_K 0x6A           /* the textures' LOD parameter K, in 16ths (Gs_BuildTexRegs): kept in RS_TEXTURE_2's word of gGsStateTextures */
#define RS_MIPMAP 0x6B              /* how the textures' mipmap levels are sampled, RF_MIPMAP_*; 0: no mipmaps (Gs_BuildTexRegs) */

/*
 * gRenderFlags: the render states Gs_SetState keeps for the GS register
 * setters (Gs_SetAlpha, Gs_SetTest, Gs_SetZbuf, Gs_SetTexFilter) and the
 * texture registers (Gs_BuildTexRegs, ScreenTex_SetRegs).
 */
#define RF_BLEND_MASK 0xFF          /* RS_BLEND's equation */
#define RF_BLEND_OP_MASK 0xC00      /* RS_BLEND_OP's family */
#define RF_ZTEST(cmp) ((cmp) << 12) /* RS_ZTEST's comparison, an RS_CMP_* */
#define RF_ZTEST_MASK 0x7000
#define RF_ZWRITE 0x8000            /* RS_ZWRITE */
#define RF_TEX_NEAREST 0x10000      /* RS_TEX_FILTER: point sampling; without it, bilinear */
#define RF_TEX_CLAMP 0x20000        /* RS_TEX_CLAMP */
#define RF_TEX_CLAMP_MASK 0x60000
#define RF_ATEST(cmp) ((cmp) << 19) /* RS_ALPHA_TEST's comparison, an RS_CMP_* */
#define RF_ATEST_MASK 0x380000
#define RF_MIPMAP_NEAREST 0x400000  /* RS_MIPMAP: the nearest level */
#define RF_MIPMAP_LINEAR 0x800000   /* RS_MIPMAP: between the two nearest levels */
#define RF_MIPMAP_MASK 0xC00000
#define RF_BLEND_ON 0x1000000       /* RS_BLEND_ON */

/*
 * RS_BLEND's values, with RS_BLEND_OP at RF_BLEND_OP_ADD (Gs_SetAlpha's
 * first table). The low nibble and the high nibble pick the GS ALPHA
 * register's (A - B) * C + D; these are the ones the game sets whole.
 */
#define RS_BLEND_OPAQUE 0x01        /* Cs: the source colour only */
#define RS_BLEND_ALPHA 0x32         /* (Cs - Cd) * As + Cd: the usual alpha blend */
#define RS_BLEND_LIGHTEN 0x28       /* (Cd - 0) * As + Cd: the destination brightened by the source's alpha (source nibble 8) */

/* RS_BLEND's two nibbles: the source's factor (low) and the destination's
 * (high, RS_BLEND_DST), as Gs_SetAlpha reads them. RS_BLEND_OPAQUE is
 * (ONE, ZERO) and RS_BLEND_ALPHA (SRC_ALPHA, INV_SRC_ALPHA). Gs_SetAlpha's
 * equations multiply by the destination's alpha (the GS's Ad) for 4 and 5,
 * as either nibble; the GS has no colour-by-colour product, so no factor
 * here is a colour. 6, 7 and 9 have no equation, and 8 only one (source 8,
 * destination SRC_ALPHA: Cd * As + Cd). */
#define RS_FACTOR_ZERO 0
#define RS_FACTOR_ONE 1
#define RS_FACTOR_SRC_ALPHA 2
#define RS_FACTOR_INV_SRC_ALPHA 3       /* 1 - the source's alpha */
#define RS_FACTOR_DST_ALPHA 4
#define RS_FACTOR_INV_DST_ALPHA 5
#define RS_BLEND_SRC_MASK 0xF
#define RS_BLEND_DST_MASK 0xF0
#define RS_BLEND_DST(f) ((f) << 4)

/* The GS's drawing contexts, the ctx Gs_SetAlpha, Gs_SetTest, Gs_SetZbuf,
 * Gs_SetScissor and Gs_SetTexFilter write. */
#define GS_CTX_1 0
#define GS_CTX_2 1
#define GS_CTX_BOTH 2

/* RF_BLEND_OP_MASK's families (RS_BLEND_OP): how Gs_SetAlpha combines the
 * source and the destination, each times its factor. */
#define RF_BLEND_OP_ADD 0               /* source + destination */
#define RF_BLEND_OP_SUB 0x400           /* source - destination */
#define RF_BLEND_OP_REV_SUB 0x800       /* destination - source */

/*
 * The comparisons of RS_ALPHA_TEST and RS_ZTEST, in OpenGL's order:
 * Gs_SetState maps them to the GS's ATST and ZTST codes. The GS's depth
 * grows toward the viewer, so RS_ZTEST's LESS and LEQUAL keep the nearer
 * pixel (ZTST GREATER and GEQUAL); RS_ZTEST takes only NEVER, LESS,
 * LEQUAL and ALWAYS (the GS has no other depth tests).
 */
#define RS_CMP_NEVER 0
#define RS_CMP_LESS 1
#define RS_CMP_EQUAL 2
#define RS_CMP_LEQUAL 3
#define RS_CMP_GREATER 4
#define RS_CMP_NOTEQUAL 5
#define RS_CMP_GEQUAL 6
#define RS_CMP_ALWAYS 7

/* RS_ALPHA_REF's value for an opaque draw: texels with less alpha are cut
 * out. A faded draw sets 0 instead, so that every texel passes. */
#define RS_ALPHA_REF_OPAQUE 0xC0

/*
 * Gfx_SetBlend's two factors, the source's and the destination's
 * (gBlendModes and gBlendModes2 turn them into RS_BLEND's nibbles). 0, 1,
 * 4 and 5 are OpenGL's ZERO, ONE, SRC_ALPHA and ONE_MINUS_SRC_ALPHA; the
 * tables send 2 and 3 to RS_FACTOR_DST_ALPHA and RS_FACTOR_INV_DST_ALPHA,
 * 6 and 7 to RS_FACTOR 8 and 9, and 8 and 9 to RS_FACTOR 6 and 7. A model's
 * factors are stored in RS_FACTOR's order: Draw_ApplyStates passes them
 * through gDrawBlendFactors, the same exchange, which gBlendModes undoes.
 * The pairs the game's calls pass by value: (GFX_BLEND_SRC_ALPHA,
 * GFX_BLEND_INV_SRC_ALPHA) is RS_BLEND_ALPHA, (GFX_BLEND_SRC_ALPHA,
 * GFX_BLEND_ONE) adds the source by its alpha, (GFX_BLEND_ONE,
 * GFX_BLEND_ONE) adds it whole.
 */
#define GFX_BLEND_ZERO 0
#define GFX_BLEND_ONE 1
#define GFX_BLEND_SRC_ALPHA 4
#define GFX_BLEND_INV_SRC_ALPHA 5       /* 1 - the source's alpha */

/* Gfx_SetTexFilter's argument, gTexFilterModes' index: RS_TEX_FILTER's
 * RF_TEX_NEAREST (point sampling) or 0 (bilinear). */
#define GFX_TEX_NEAREST 0
#define GFX_TEX_BILINEAR 1

/* Gfx_SetBlendOp's argument, gBlendOps' index: RS_BLEND_OP's
 * RF_BLEND_OP_ADD, RF_BLEND_OP_SUB and RF_BLEND_OP_REV_SUB. */
#define GFX_BLEND_OP_ADD 0
#define GFX_BLEND_OP_SUB 1
#define GFX_BLEND_OP_REV_SUB 2

/* RS_ORTHO's argument: an orthographic view (Camera_SetOrtho). */
typedef struct GsViewport {
    s32 x;                      /* 0x00 */
    s32 y;                      /* 0x04 */
    s32 width;                  /* 0x08 */
    s32 height;                 /* 0x0C */
    f32 nearZ;                  /* 0x10: the depth range (Camera.viewport writes its near as an int 0) */
    f32 farZ;                   /* 0x14 */
} GsViewport;

s32 Packet_SetMode2Ok(void);
s32 Gs_ResetState(void);
s32 Gs_SetState(u8 reg, u32 value);
s32 Gs_GetState(u8 reg);
void Gs_SetBgColor(s32 color);
s32 Gs_GetBgColor(void);

#endif /* RENDER_STATE_H */
