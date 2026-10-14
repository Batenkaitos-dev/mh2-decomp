/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"
#include "model.h"
#include "light.h"

extern s32 gPacketMode;
extern s32 gSysBgColor;
extern u32 gRenderFlags2;
extern u32 gGsColor0E;
extern u32 gGsAlphaRef;
extern u32 gGsStateTextures[];
extern u32 D_0020E138;
extern u32 gGsTexLodL;
s32 Gs_GetBgColor(void);
void Gs_SetBgColor(s32 color);
typedef struct RsMat {
    f32 m[16];
} RsMat;


extern Light gRsLights[3];

extern RsMat gMtxSlots[];       /* registers 0x1A-0x39 */
extern ModelMat gRsMaterials[];     /* registers 0x3A-0x59 */
extern f32 D_0020CBE0;
extern f32 D_0020CBF0;
extern f32 D_0020CC20;
extern f32 D_0020CBC0[];
extern u32 D_002191A8;
extern f32 gGsFogNear;
extern f32 gGsFogFar;
extern u32 gGsColor67;
extern f32 gGsColor67F;
extern f32 D_0020D814;
extern f32 D_0020D818;
extern f32 D_0020D81C;
extern f32 gGsColor0EF;
extern f32 D_0020D824;
extern f32 D_0020D828;
extern f32 D_0020D82C;
extern RsMat gMtxView;
extern RsMat gMtxCamProj;
extern RsMat gMtxRs19;
extern f32 gMtxProjClip[];
extern f32 gMtxClip[];
extern f32 gMtxProjScreen[];
extern f32 gMtxScreen[];
s32 Tex_Use(s32 n, u32 *handles);
s32 Gs_SetTexture(s32 arg0);
void Vec_Normalize(f32 *v);
void Camera_SetOrtho(s32 i, s32 x, s32 y, s32 w, s32 h, f32 n, f32 f);
void Mtx_Mul(f32 *dst, f32 *a, f32 *b);
extern u32 gRenderFlags;
extern s32 gDispHeight;          /* display height */
extern s32 gDispWidth;          /* display width */
extern u32 gGsFogColor;
extern s32 gGsFba;
extern f32 gDispTexScaleU;
extern f32 gDispTexScaleV;
extern f32 gMtxIdentity[16];

void Packet_SetMode0(void);
void Packet_SetMode2(void);
s32 Gs_SetAlpha(u32 flags, s32 ctx);
s32 Gs_SetTest(u32 flags, s32 ctx);
s32 Gs_SetZbuf(u32 flags, s32 ctx);
s32 Gs_SetScissor(s32 x, s32 y, s32 w, s32 h, s32 ctx);
s32 Gs_SetFogColor(u32 color);
s32 Gs_SetTexFilter(u32 flags, s32 ctx);

s32 Packet_SetMode2Ok(void) {
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
    s32 w;
    s32 h;
    s32 j;

    Packet_SetMode0();
    Gs_SetAlpha(gRenderFlags, 2);
    Gs_SetTest(gRenderFlags, 2);
    Gs_SetZbuf(gRenderFlags, 2);
    Gs_SetScissor(0, 0, gDispWidth, gDispHeight, 2);
    Gs_SetFogColor(gGsFogColor);
    Gs_SetTexFilter(gRenderFlags, 0);
    gGsFba = 0;
    Packet_SetMode2();
    w = gDispWidth;
    i = 0;
    if (w > 1) {
        do {
            i++;
        } while (w > (1 << i));
    }
    gDispTexScaleU = (f32)w / (f32)(1 << i);
    h = gDispHeight;
    j = 0;
    if (h > 1) {
        do {
            j++;
        } while (h > (1 << j));
    }
    gDispTexScaleV = (f32)h / (f32)(1 << j);
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

/* Sets a render-state value (the register numbers Gs_GetState reads):
 * matrices and light records (0x1A-0x5C), test, blending and Z bits,
 * colours, textures. Returns 1. */
s32 Gs_SetState(u8 reg, u32 v)
{
    u32 tex;
    u32 c;
    s32 k;

    if (reg >= 0x1A && reg <= 0x39) {
        if (v != 0) {
            gMtxSlots[reg - 0x1AU] = *(RsMat *)v;
        }
        return 1;
    }
    if (reg >= 0x3A && reg <= 0x59) {
        gRsMaterials[reg - 0x3AU] = *(ModelMat *)v;
        return 1;
    }
    if (reg >= 0x5A && reg <= 0x5C) {
        k = reg - 0x5A;
        gRsLights[k] = *(Light *)v;
        Vec_Normalize(gRsLights[k].dir);
        gRsLights[k].amb[3] = 0.0f;
        gRsLights[k].unk14[3] = 0.0f;
        D_0020CBC0[k] = 1.0f / gRsLights[k].unk50;
        return 1;
    }
    switch (reg) {
    case 0:
        gRenderFlags2 &= ~0x60;
        gRenderFlags2 |= v;
        break;
    case 3:
        break;
    case 1:
        gRenderFlags2 &= ~0xF00;
        switch (v) {
        case 0x200:
            gRenderFlags2 |= 0x200;
            break;
        case 0x100:
            gRenderFlags2 |= 0x100;
            break;
        case 0x300:
            gRenderFlags2 |= 0x300;
            break;
        case 0x400:
            gRenderFlags2 |= 0x400;
            break;
        case 0x500:
            gRenderFlags2 |= 0x500;
            break;
        case 0x600:
            gRenderFlags2 |= 0x600;
            break;
        case 0x700:
            gRenderFlags2 |= 0x700;
            break;
        case 0x800:
            gRenderFlags2 |= 0x800;
            break;
        case 0:
            break;
        }
        break;
    case 2:
        gRenderFlags2 &= ~0x1C;
        switch (v) {
        case 0x4:
        case 0x8:
        case 0xC:
        case 0x10:
        case 0x14:
        case 0x18:
            gRenderFlags2 |= v;
            break;
        case 0:
            break;
        }
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        tex = v;
        if (reg == 4) {
            Tex_Use(1, &tex);
            Gs_SetTexture(tex);
            D_002191A8 = tex;
        }
        gGsStateTextures[reg - 4U] = tex;
        break;
    case 8:
    case 9:
    case 0xA:
    case 0xB:
        break;
    case 0xC:
        gRenderFlags2 &= ~0x80;
        if (v == 1) {
            gRenderFlags2 |= 0x80;
            Gs_SetScissor(0, 0, gDispWidth, gDispHeight, 1);
        }
        break;
    case 0xD:
        if ((gRenderFlags & 0xC00) != v) {
            gRenderFlags &= ~0xC00;
            gRenderFlags |= v;
            Gs_SetAlpha(gRenderFlags, 0);
        }
        break;
    case 0x5E:
        if ((gRenderFlags & 0xFF) != v) {
            gRenderFlags &= ~0xFF;
            gRenderFlags |= v;
            Gs_SetAlpha(gRenderFlags, 0);
        }
        break;
    case 0x61:
        if ((gRenderFlags & 0x1000000) != v) {
            gRenderFlags &= ~0x1000000;
            gRenderFlags |= v;
            Gs_SetAlpha(gRenderFlags, 0);
        }
        break;
    case 0xE:
        gGsColor0E = v;
        c = *(volatile u32 *)&gGsColor0E;
        gGsColor0EF = ((c >> 16) & 0xFF) / 255.0f;
        D_0020D824 = ((c >> 8) & 0xFF) / 255.0f;
        D_0020D828 = (c & 0xFF) / 255.0f;
        {
            u32 a = (c >> 24) & 0xFF;

            if (a == 0xFF) {
                D_0020D82C = 128.0f;
            } else if (a != 0) {
                a >>= 1;
                if (a == 0) {
                    a = 1;
                }
                D_0020D82C = a;
            } else {
                D_0020D82C = 0.0f;
            }
        }
        break;
    case 0xF:
        if (gGsFogColor != v) {
            gGsFogColor = v;
            Gs_SetFogColor(v);
        }
        break;
    case 0x10:
        gGsFogNear = *(f32 *)&v;
        break;
    case 0x11:
        gGsFogFar = *(f32 *)&v;
        break;
    case 0x12:
        gRenderFlags2 &= ~0x18000;
        if (v == 0x8000) {
            gRenderFlags2 |= 0x8000;
        }
        break;
    case 0x13:
        break;
    case 0x14:
        Gs_SetBgColor(v);
        break;
    case 0x15:
        gRenderFlags2 &= ~3;
        if (v != 0 && v == 2) {
            gRenderFlags2 |= 2;
        }
        break;
    case 0x16:
        gMtxView = *(RsMat *)v;
        break;
    case 0x17:
        gMtxCamProj = *(RsMat *)v;
        Mtx_Mul(gMtxProjClip, gMtxCamProj.m, gMtxClip);
        Mtx_Mul(gMtxProjScreen, gMtxCamProj.m, gMtxScreen);
        break;
    case 0x18:
        Camera_SetOrtho(0x22, ((s32 *)v)[0], ((s32 *)v)[1], ((s32 *)v)[2], ((s32 *)v)[3], ((f32 *)v)[4], ((f32 *)v)[5]);
        break;
    case 0x19:
        gMtxRs19 = *(RsMat *)v;
        break;
    case 0x5D:
        gRenderFlags2 &= 0xFFFFFF;
        gRenderFlags2 |= v;
        break;
    case 0x5F:
        switch (v) {
        case 2:
            v = 0x100000;
            break;
        case 5:
            v = 0x280000;
            break;
        case 4:
            v = 0x200000;
            break;
        case 6:
            v = 0x300000;
            break;
        case 0:
            v = 0;
            break;
        case 7:
            v = 0x380000;
            break;
        case 1:
            v = 0x80000;
            break;
        case 3:
            v = 0x180000;
            break;
        }
        if ((gRenderFlags & 0x380000) != v) {
            gRenderFlags &= ~0x380000;
            gRenderFlags |= v;
            Gs_SetTest(gRenderFlags, 0);
        }
        break;
    case 0x60:
        if (v != gGsAlphaRef) {
            gGsAlphaRef = v;
            Gs_SetTest(gRenderFlags, 0);
        }
        break;
    case 0x62:
        gRenderFlags2 &= ~0x4000;
        if (v != 0) {
            gRenderFlags2 |= 0x4000;
        }
        break;
    case 0x63:
        gRenderFlags &= ~0x10000;
        if (v == 0x10000) {
            gRenderFlags |= 0x10000;
        }
        Gs_SetTexFilter(gRenderFlags, 0);
        break;
    case 0x64:
        gRenderFlags &= ~0x60000;
        if (v != 0 && v == 0x20000) {
            gRenderFlags |= 0x20000;
        }
        break;
    case 0x65:
        break;
    case 0x66:
        gRenderFlags2 &= ~0x20000;
        if (v == 1) {
            gRenderFlags2 |= 0x20000;
        }
        break;
    case 0x67:
        gGsColor67 = v;
        gGsColor67F = ((v >> 16) & 0xFF) / 255.0f;
        D_0020D814 = ((v >> 8) & 0xFF) / 255.0f;
        D_0020D818 = (v & 0xFF) / 255.0f;
        D_0020D81C = ((v >> 24) & 0xFF) / 255.0f;
        break;
    case 0x68:
        break;
    case 0x69:
        gGsTexLodL = v;
        break;
    case 0x6A:
        D_0020E138 = v;
        break;
    case 0x6B:
        if ((gRenderFlags & 0xC00000) != v) {
            gRenderFlags &= ~0xC00000;
            gRenderFlags |= v;
        }
        break;
    case 0x6C:
        v = (v == 1) ? 0x8000 : 0;
        if ((gRenderFlags & 0x8000) != v) {
            gRenderFlags &= ~0x8000;
            gRenderFlags |= v;
            Gs_SetZbuf(gRenderFlags, 0);
        }
        break;
    case 0x6D:
        switch (v) {
        case 0:
            v = 0;
            break;
        case 7:
            v = 0x7000;
            break;
        case 1:
            v = 0x1000;
            break;
        case 3:
            v = 0x3000;
            break;
        case 2:
        case 4:
        case 5:
        case 6:
            break;
        }
        if ((gRenderFlags & 0x7000) != v) {
            gRenderFlags &= ~0x7000;
            gRenderFlags |= v;
            Gs_SetTest(gRenderFlags, 0);
        }
        break;
    }
    return 1;
}

/* Reads back a render-state value (the register numbers of Gs_SetState's
 * setter): the test, blending and Z bits as flags or small codes, the
 * colours, the matrices' records. */
s32 Gs_GetState(u8 reg)
{
    s32 r = 0;

    switch (reg) {
    case 0:
    case 3:
        break;
    case 1:
        r = gRenderFlags2 & 0xF00;
        break;
    case 2:
        r = gRenderFlags2 & 0x1C;
        break;
    case 4:
    case 5:
    case 6:
    case 7:
        r = gGsStateTextures[reg - 4U];
        break;
    case 8:
    case 9:
    case 0xA:
    case 0xB:
        break;
    case 0xC:
        if ((gRenderFlags2 & 0x80) == 0x80) {
            r = 1;
        } else {
            r = 0;
        }
        break;
    case 0xD:
        r = gRenderFlags & 0xC00;
        break;
    case 0xE:
        r = gGsColor0E;
        break;
    case 0xF:
        r = gGsFogColor;
        break;
    case 0x10:
    case 0x11:
        break;
    case 0x12:
        if ((gRenderFlags2 & 0x18000) == 0x8000) {
            r = 1;
        } else {
            r = 0;
        }
        break;
    case 0x13:
        break;
    case 0x14:
        r = Gs_GetBgColor();
        break;
    case 0x15:
        r = gRenderFlags2 & 3;
        if (r == 0) {
            r = 0;
        } else if (r == 2) {
            r = 2;
        }
        break;
    case 0x5A:
    case 0x5B:
    case 0x5C:
        r = (s32)&gRsLights[reg - 0x5AU];
        break;
    case 0x5D:
        r = gRenderFlags2 & 0xFF000000;
        break;
    case 0x5E:
        r = (u8)gRenderFlags;
        break;
    case 0x5F:
        switch (gRenderFlags & 0x380000) {
        case 0:
            r = 0;
            break;
        case 0x80000:
            r = 1;
            break;
        case 0x100000:
            r = 2;
            break;
        case 0x180000:
            r = 3;
            break;
        case 0x200000:
            r = 4;
            break;
        case 0x280000:
            r = 5;
            break;
        case 0x300000:
            r = 6;
            break;
        case 0x380000:
            r = 7;
            break;
        }
        break;
    case 0x60:
        r = gGsAlphaRef;
        break;
    case 0x61:
        if ((gRenderFlags & 0x1000000) == 0x1000000) {
            r = 1;
        } else {
            r = 0;
        }
        break;
    case 0x62:
        if ((gRenderFlags2 & 0x4000) == 0x4000) {
            r = 1;
        } else {
            r = 0;
        }
        break;
    case 0x63:
        r = gRenderFlags & 0x10000;
        break;
    case 0x64:
        r = gRenderFlags & 0x60000;
        break;
    case 0x65:
        break;
    case 0x66:
        if ((gRenderFlags2 & 0x20000) == 0x20000) {
            r = 1;
        } else {
            r = 0;
        }
        break;
    case 0x68:
        break;
    case 0x69:
        r = gGsTexLodL;
        break;
    case 0x6A:
        r = D_0020E138;
        break;
    case 0x6B:
        r = gRenderFlags & 0xC00000;
        break;
    case 0x6C:
        if ((gRenderFlags & 0x8000) == 0x8000) {
            r = 1;
        } else {
            r = 0;
        }
        break;
    case 0x6D:
        switch (gRenderFlags & 0x7000) {
        case 0:
            r = 0;
            break;
        case 0x1000:
            r = 1;
            break;
        case 0x2000:
            r = 2;
            break;
        case 0x3000:
            r = 3;
            break;
        case 0x4000:
            r = 4;
            break;
        case 0x5000:
            r = 5;
            break;
        case 0x6000:
            r = 6;
            break;
        case 0x7000:
            r = 7;
            break;
        }
        break;
    }
    return r;
}

void Gs_SetBgColor(s32 color) {
    gSysBgColor = color;
}

s32 Gs_GetBgColor(void) {
    return gSysBgColor;
}
