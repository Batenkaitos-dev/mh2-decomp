/*
 * The render state's store. Gs_ResetState sets the defaults; Gs_SetState
 * sets one render state (render_state.h's RS_*): the matrices, the
 * material and light records, the colours, the textures, and the blend,
 * test, depth and texture bits it keeps in gRenderFlags (RF_*), writing
 * the GS register each feeds; and Gs_GetState reads one back. The states
 * render_state.h doesn't name set bits of gRenderFlags2, the keys that
 * pick a model's VU1 program. Gs_SetBgColor and Gs_GetBgColor keep the
 * background colour.
 */
#include "common.h"
#include "model.h"
#include "light.h"
#include "render_state.h"
#include "texture.h"
#include "engine_math.h"
#include "gs_regs.h"
#include "gs_tex_regs.h"
#include "packet_mode.h"

extern s32 gPacketMode;
extern s32 gSysBgColor;
extern u32 gRenderFlags2;
extern u32 gGsColor;
extern u32 gGsAlphaRef;
extern u32 gGsStateTextures[];
extern u32 gGsTexLodL;
typedef struct RsMat {
    f32 m[16];
} RsMat;


extern Light gRsLights[3];

extern RsMat gMtxSlots[];       /* states RS_MTX_SLOT(0) to RS_MTX_SLOT(31) */
extern ModelMat gRsMaterials[];     /* states RS_MATERIAL(0) to RS_MATERIAL(31) */
extern f32 gRsLightReciprocals[];  /* per light, 1 / its unk50 (no reader seen) */
extern u32 gGsBoundTexture;     /* the texture RS_TEXTURE bound last */
extern f32 gGsFogNear;
extern f32 gGsFogFar;
extern u32 gGsColor2;
extern f32 gGsColor2F[4];       /* RS_COLOR2 as floats: red, green, blue, alpha */
extern f32 gGsColorF[4];        /* RS_COLOR as floats: red, green, blue, alpha */
extern RsMat gMtxView;
extern RsMat gMtxCamProj;
extern RsMat gMtxUv;
extern f32 gMtxProjClip[];
extern f32 gMtxClip[];
extern f32 gMtxProjScreen[];
extern f32 gMtxScreen[];
extern u32 gRenderFlags;
extern s32 gDispHeight;          /* display height */
extern s32 gDispWidth;          /* display width */
extern u32 gGsFogColor;
extern s32 gGsFba;
extern f32 gDispTexScaleU;
extern f32 gDispTexScaleV;
extern f32 gMtxIdentity[16];

/* Sets the packet mode to 2; returns 1. */
s32 Packet_SetMode2Ok(void)
{
    gPacketMode = 2;
    return 1;
}

/* Sets the default render state (both contexts), the scissor to the
 * display, and the texture scale factors: the display size over the next
 * power of two. Returns 1 (the constant 1 of the last shift stays in v0
 * for it). */
s32 Gs_ResetState(void)
{
    s32 i;
    s32 width;
    s32 height;
    s32 j;

    Packet_SetMode0();
    Gs_SetAlpha(gRenderFlags, GS_CTX_BOTH);
    Gs_SetTest(gRenderFlags, GS_CTX_BOTH);
    Gs_SetZbuf(gRenderFlags, GS_CTX_BOTH);
    Gs_SetScissor(0, 0, gDispWidth, gDispHeight, GS_CTX_BOTH);
    Gs_SetFogColor(gGsFogColor);
    Gs_SetTexFilter(gRenderFlags, GS_CTX_1);
    gGsFba = 0;
    Packet_SetMode2();
    width = gDispWidth;
    i = 0;
    if (width > 1) {
        do {
            i++;
        } while (width > (1 << i));
    }
    gDispTexScaleU = (f32)width / (f32)(1 << i);
    height = gDispHeight;
    j = 0;
    if (height > 1) {
        do {
            j++;
        } while (height > (1 << j));
    }
    gDispTexScaleV = (f32)height / (f32)(1 << j);
    gMtxIdentity[1] = 0.0f;
    gMtxIdentity[2] = 0.0f;
    gMtxIdentity[3] = 0.0f;
    gMtxIdentity[4] = 0.0f;
    gMtxIdentity[0] = 1.0f;
    gMtxIdentity[5] = 1.0f;
    gMtxIdentity[6] = 0.0f;
    gMtxIdentity[7] = 0.0f;
    gMtxIdentity[8] = 0.0f;
    gMtxIdentity[9] = 0.0f;
    gMtxIdentity[10] = 1.0f;
    gMtxIdentity[15] = 1.0f;
    gMtxIdentity[11] = 0.0f;
    gMtxIdentity[12] = 0.0f;
    gMtxIdentity[13] = 0.0f;
    gMtxIdentity[14] = 0.0f;
    return 1;
}

/*
 * Sets render state reg (RS_*) to value: a matrix, material or light
 * record is copied from the address value holds (a light's direction is
 * normalized), a colour is also kept as floats, and the blend, test, depth
 * and texture states go into gRenderFlags and the GS register they feed.
 * A state already at the value isn't written again. Returns 1.
 */
s32 Gs_SetState(u8 reg, u32 value)
{
    u32 tex;
    u32 color;
    s32 k;

    if (reg >= RS_MTX_SLOT(0) && reg <= RS_MTX_SLOT(31)) {
        if (value != 0) {
            gMtxSlots[reg - (u32)RS_MTX_SLOT(0)] = *(RsMat *)value;
        }
        return 1;
    }
    if (reg >= RS_MATERIAL(0) && reg <= RS_MATERIAL(31)) {
        gRsMaterials[reg - (u32)RS_MATERIAL(0)] = *(ModelMat *)value;
        return 1;
    }
    if (reg >= RS_LIGHT(0) && reg <= RS_LIGHT(2)) {
        k = reg - RS_LIGHT(0);
        gRsLights[k] = *(Light *)value;
        Vec_Normalize(gRsLights[k].dir);
        gRsLights[k].color[3] = 0.0f;
        gRsLights[k].unk14[3] = 0.0f;
        gRsLightReciprocals[k] = 1.0f / gRsLights[k].unk50;
        return 1;
    }
    switch (reg) {
    case 0:
        gRenderFlags2 &= ~0x60;
        gRenderFlags2 |= value;
        break;
    case 3:
        break;
    case 1:
        gRenderFlags2 &= ~0xF00;
        switch (value) {
        case 512:
            gRenderFlags2 |= 0x200;
            break;
        case 256:
            gRenderFlags2 |= 0x100;
            break;
        case 768:
            gRenderFlags2 |= 0x300;
            break;
        case 1024:
            gRenderFlags2 |= 0x400;
            break;
        case 1280:
            gRenderFlags2 |= 0x500;
            break;
        case 1536:
            gRenderFlags2 |= 0x600;
            break;
        case 1792:
            gRenderFlags2 |= 0x700;
            break;
        case 2048:
            gRenderFlags2 |= 0x800;
            break;
        case 0:
            break;
        }
        break;
    case 2:
        gRenderFlags2 &= ~0x1C;
        switch (value) {
        case 4:
        case 8:
        case 12:
        case 16:
        case 20:
        case 24:
            gRenderFlags2 |= value;
            break;
        case 0:
            break;
        }
        break;
    case RS_TEXTURE:
    case RS_TEXTURE_1:
    case RS_TEXTURE_2:
    case RS_TEXTURE_3:
        tex = value;
        if (reg == RS_TEXTURE) {
            Tex_Use(1, &tex);
            Gs_SetTexture(tex);
            gGsBoundTexture = tex;
        }
        gGsStateTextures[reg - (u32)RS_TEXTURE] = tex;
        break;
    case 8:
    case 9:
    case 10:
    case 11:
        break;
    case RS_SCISSOR:
        gRenderFlags2 &= ~0x80;
        if (value == 1) {
            gRenderFlags2 |= 0x80;
            Gs_SetScissor(0, 0, gDispWidth, gDispHeight, GS_CTX_2);
        }
        break;
    case RS_BLEND_OP:
        if ((gRenderFlags & RF_BLEND_OP_MASK) != value) {
            gRenderFlags &= ~RF_BLEND_OP_MASK;
            gRenderFlags |= value;
            Gs_SetAlpha(gRenderFlags, GS_CTX_1);
        }
        break;
    case RS_BLEND:
        if ((gRenderFlags & RF_BLEND_MASK) != value) {
            gRenderFlags &= ~RF_BLEND_MASK;
            gRenderFlags |= value;
            Gs_SetAlpha(gRenderFlags, GS_CTX_1);
        }
        break;
    case RS_BLEND_ON:
        if ((gRenderFlags & RF_BLEND_ON) != value) {
            gRenderFlags &= ~RF_BLEND_ON;
            gRenderFlags |= value;
            Gs_SetAlpha(gRenderFlags, GS_CTX_1);
        }
        break;
    case RS_COLOR:
        /* the colour is read back through a volatile load, as the game's
         * code does: a plain read keeps value's register instead */
        gGsColor = value;
        color = *(volatile u32 *)&gGsColor;
        gGsColorF[0] = ((color >> 16) & 0xFF) / 255.0f;
        gGsColorF[1] = ((color >> 8) & 0xFF) / 255.0f;
        gGsColorF[2] = (color & 0xFF) / 255.0f;
        {
            /* the alpha in the GS's range: 255 is 128, the rest halved
             * (at least 1 unless 0) */
            u32 alpha = (color >> 24) & 0xFF;

            if (alpha == 255) {
                gGsColorF[3] = 128.0f;
            } else if (alpha != 0) {
                alpha >>= 1;
                if (alpha == 0) {
                    alpha = 1;
                }
                gGsColorF[3] = alpha;
            } else {
                gGsColorF[3] = 0.0f;
            }
        }
        break;
    case RS_FOG_COLOR:
        if (gGsFogColor != value) {
            gGsFogColor = value;
            Gs_SetFogColor(value);
        }
        break;
    case RS_FOG_NEAR:
        gGsFogNear = *(f32 *)&value;
        break;
    case RS_FOG_FAR:
        gGsFogFar = *(f32 *)&value;
        break;
    case 18:
        gRenderFlags2 &= ~0x18000;
        if (value == 0x8000) {
            gRenderFlags2 |= 0x8000;
        }
        break;
    case 19:
        break;
    case RS_BG_COLOR:
        Gs_SetBgColor(value);
        break;
    case 21:
        gRenderFlags2 &= ~3;
        if (value != 0 && value == 2) {
            gRenderFlags2 |= 2;
        }
        break;
    case RS_VIEW_MTX:
        gMtxView = *(RsMat *)value;
        break;
    case RS_PROJ_MTX:
        gMtxCamProj = *(RsMat *)value;
        Mtx_Mul(gMtxProjClip, gMtxCamProj.m, gMtxClip);
        Mtx_Mul(gMtxProjScreen, gMtxCamProj.m, gMtxScreen);
        break;
    case RS_ORTHO:
        Camera_SetOrtho(MTX_SLOT_ORTHO, ((GsViewport *)value)->x, ((GsViewport *)value)->y, ((GsViewport *)value)->width, ((GsViewport *)value)->height,
                        ((GsViewport *)value)->nearZ, ((GsViewport *)value)->farZ);
        break;
    case RS_UV_MTX:
        gMtxUv = *(RsMat *)value;
        break;
    case 93:
        gRenderFlags2 &= 0xFFFFFF;
        gRenderFlags2 |= value;
        break;
    case RS_ALPHA_TEST:
        switch (value) {
        case RS_CMP_EQUAL:
            value = RF_ATEST(RS_CMP_EQUAL);
            break;
        case RS_CMP_NOTEQUAL:
            value = RF_ATEST(RS_CMP_NOTEQUAL);
            break;
        case RS_CMP_GREATER:
            value = RF_ATEST(RS_CMP_GREATER);
            break;
        case RS_CMP_GEQUAL:
            value = RF_ATEST(RS_CMP_GEQUAL);
            break;
        case RS_CMP_NEVER:
            value = RF_ATEST(RS_CMP_NEVER);
            break;
        case RS_CMP_ALWAYS:
            value = RF_ATEST(RS_CMP_ALWAYS);
            break;
        case RS_CMP_LESS:
            value = RF_ATEST(RS_CMP_LESS);
            break;
        case RS_CMP_LEQUAL:
            value = RF_ATEST(RS_CMP_LEQUAL);
            break;
        }
        if ((gRenderFlags & RF_ATEST_MASK) != value) {
            gRenderFlags &= ~RF_ATEST_MASK;
            gRenderFlags |= value;
            Gs_SetTest(gRenderFlags, GS_CTX_1);
        }
        break;
    case RS_ALPHA_REF:
        if (value != gGsAlphaRef) {
            gGsAlphaRef = value;
            Gs_SetTest(gRenderFlags, GS_CTX_1);
        }
        break;
    case 98:
        gRenderFlags2 &= ~0x4000;
        if (value != 0) {
            gRenderFlags2 |= 0x4000;
        }
        break;
    case RS_TEX_FILTER:
        gRenderFlags &= ~RF_TEX_NEAREST;
        if (value == RF_TEX_NEAREST) {
            gRenderFlags |= RF_TEX_NEAREST;
        }
        Gs_SetTexFilter(gRenderFlags, GS_CTX_1);
        break;
    case RS_TEX_CLAMP:
        gRenderFlags &= ~RF_TEX_CLAMP_MASK;
        if (value != 0 && value == RF_TEX_CLAMP) {
            gRenderFlags |= RF_TEX_CLAMP;
        }
        break;
    case 101:
        break;
    case 102:
        gRenderFlags2 &= ~0x20000;
        if (value == 1) {
            gRenderFlags2 |= 0x20000;
        }
        break;
    case RS_COLOR2:
        gGsColor2 = value;
        gGsColor2F[0] = ((value >> 16) & 0xFF) / 255.0f;
        gGsColor2F[1] = ((value >> 8) & 0xFF) / 255.0f;
        gGsColor2F[2] = (value & 0xFF) / 255.0f;
        gGsColor2F[3] = ((value >> 24) & 0xFF) / 255.0f;
        break;
    case 104:
        break;
    case RS_TEX_LOD_L:
        gGsTexLodL = value;
        break;
    case RS_TEX_LOD_K:
        /* kept in RS_TEXTURE_2's word */
        gGsStateTextures[RS_TEXTURE_2 - RS_TEXTURE] = value;
        break;
    case RS_MIPMAP:
        if ((gRenderFlags & RF_MIPMAP_MASK) != value) {
            gRenderFlags &= ~RF_MIPMAP_MASK;
            gRenderFlags |= value;
        }
        break;
    case RS_ZWRITE:
        value = (value == 1) ? RF_ZWRITE : 0;
        if ((gRenderFlags & RF_ZWRITE) != value) {
            gRenderFlags &= ~RF_ZWRITE;
            gRenderFlags |= value;
            Gs_SetZbuf(gRenderFlags, GS_CTX_1);
        }
        break;
    case RS_ZTEST:
        /* the GS tests depth only these four ways; the other comparisons
         * are kept as passed */
        switch (value) {
        case RS_CMP_NEVER:
            value = RF_ZTEST(RS_CMP_NEVER);
            break;
        case RS_CMP_ALWAYS:
            value = RF_ZTEST(RS_CMP_ALWAYS);
            break;
        case RS_CMP_LESS:
            value = RF_ZTEST(RS_CMP_LESS);
            break;
        case RS_CMP_LEQUAL:
            value = RF_ZTEST(RS_CMP_LEQUAL);
            break;
        case RS_CMP_EQUAL:
        case RS_CMP_GREATER:
        case RS_CMP_NOTEQUAL:
        case RS_CMP_GEQUAL:
            break;
        }
        if ((gRenderFlags & RF_ZTEST_MASK) != value) {
            gRenderFlags &= ~RF_ZTEST_MASK;
            gRenderFlags |= value;
            Gs_SetTest(gRenderFlags, GS_CTX_1);
        }
        break;
    }
    return 1;
}

/*
 * Reads render state reg (RS_*) back, as Gs_SetState was given it: the
 * comparisons as RS_CMP_*, the depth write and the on/off states as 1 or
 * 0, the light records as their addresses. The states Gs_SetState only
 * copies (the matrices, the materials) and those that do nothing read 0.
 */
s32 Gs_GetState(u8 reg)
{
    s32 value = 0;

    switch (reg) {
    case 0:
    case 3:
        break;
    case 1:
        value = gRenderFlags2 & 0xF00;
        break;
    case 2:
        value = gRenderFlags2 & 0x1C;
        break;
    case RS_TEXTURE:
    case RS_TEXTURE_1:
    case RS_TEXTURE_2:
    case RS_TEXTURE_3:
        value = gGsStateTextures[reg - (u32)RS_TEXTURE];
        break;
    case 8:
    case 9:
    case 10:
    case 11:
        break;
    case RS_SCISSOR:
        if ((gRenderFlags2 & 0x80) == 128) {
            value = 1;
        } else {
            value = 0;
        }
        break;
    case RS_BLEND_OP:
        value = gRenderFlags & RF_BLEND_OP_MASK;
        break;
    case RS_COLOR:
        value = gGsColor;
        break;
    case RS_FOG_COLOR:
        value = gGsFogColor;
        break;
    case RS_FOG_NEAR:
    case RS_FOG_FAR:
        break;
    case 18:
        if ((gRenderFlags2 & 0x18000) == 0x8000) {
            value = 1;
        } else {
            value = 0;
        }
        break;
    case 19:
        break;
    case RS_BG_COLOR:
        value = Gs_GetBgColor();
        break;
    case 21:
        value = gRenderFlags2 & 3;
        if (value == 0) {
            value = 0;
        } else if (value == 2) {
            value = 2;
        }
        break;
    case RS_LIGHT(0):
    case RS_LIGHT(1):
    case RS_LIGHT(2):
        value = (s32)&gRsLights[reg - (u32)RS_LIGHT(0)];
        break;
    case 93:
        value = gRenderFlags2 & 0xFF000000;
        break;
    case RS_BLEND:
        value = (u8)gRenderFlags;
        break;
    case RS_ALPHA_TEST:
        switch (gRenderFlags & RF_ATEST_MASK) {
        case RF_ATEST(RS_CMP_NEVER):
            value = RS_CMP_NEVER;
            break;
        case RF_ATEST(RS_CMP_LESS):
            value = RS_CMP_LESS;
            break;
        case RF_ATEST(RS_CMP_EQUAL):
            value = RS_CMP_EQUAL;
            break;
        case RF_ATEST(RS_CMP_LEQUAL):
            value = RS_CMP_LEQUAL;
            break;
        case RF_ATEST(RS_CMP_GREATER):
            value = RS_CMP_GREATER;
            break;
        case RF_ATEST(RS_CMP_NOTEQUAL):
            value = RS_CMP_NOTEQUAL;
            break;
        case RF_ATEST(RS_CMP_GEQUAL):
            value = RS_CMP_GEQUAL;
            break;
        case RF_ATEST(RS_CMP_ALWAYS):
            value = RS_CMP_ALWAYS;
            break;
        }
        break;
    case RS_ALPHA_REF:
        value = gGsAlphaRef;
        break;
    case RS_BLEND_ON:
        if ((gRenderFlags & RF_BLEND_ON) == RF_BLEND_ON) {
            value = 1;
        } else {
            value = 0;
        }
        break;
    case 98:
        if ((gRenderFlags2 & 0x4000) == 16384) {
            value = 1;
        } else {
            value = 0;
        }
        break;
    case RS_TEX_FILTER:
        value = gRenderFlags & RF_TEX_NEAREST;
        break;
    case RS_TEX_CLAMP:
        value = gRenderFlags & RF_TEX_CLAMP_MASK;
        break;
    case 101:
        break;
    case 102:
        if ((gRenderFlags2 & 0x20000) == 0x20000) {
            value = 1;
        } else {
            value = 0;
        }
        break;
    case 104:
        break;
    case RS_TEX_LOD_L:
        value = gGsTexLodL;
        break;
    case RS_TEX_LOD_K:
        value = gGsStateTextures[RS_TEXTURE_2 - RS_TEXTURE];
        break;
    case RS_MIPMAP:
        value = gRenderFlags & RF_MIPMAP_MASK;
        break;
    case RS_ZWRITE:
        if ((gRenderFlags & RF_ZWRITE) == RF_ZWRITE) {
            value = 1;
        } else {
            value = 0;
        }
        break;
    case RS_ZTEST:
        switch (gRenderFlags & RF_ZTEST_MASK) {
        case RF_ZTEST(RS_CMP_NEVER):
            value = RS_CMP_NEVER;
            break;
        case RF_ZTEST(RS_CMP_LESS):
            value = RS_CMP_LESS;
            break;
        case RF_ZTEST(RS_CMP_EQUAL):
            value = RS_CMP_EQUAL;
            break;
        case RF_ZTEST(RS_CMP_LEQUAL):
            value = RS_CMP_LEQUAL;
            break;
        case RF_ZTEST(RS_CMP_GREATER):
            value = RS_CMP_GREATER;
            break;
        case RF_ZTEST(RS_CMP_NOTEQUAL):
            value = RS_CMP_NOTEQUAL;
            break;
        case RF_ZTEST(RS_CMP_GEQUAL):
            value = RS_CMP_GEQUAL;
            break;
        case RF_ZTEST(RS_CMP_ALWAYS):
            value = RS_CMP_ALWAYS;
            break;
        }
        break;
    }
    return value;
}

/* Sets the background colour (RS_BG_COLOR). */
void Gs_SetBgColor(s32 color)
{
    gSysBgColor = color;
}

/* Returns the background colour. */
s32 Gs_GetBgColor(void)
{
    return gSysBgColor;
}
