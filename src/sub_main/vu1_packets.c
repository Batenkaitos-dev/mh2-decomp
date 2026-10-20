/*
 * The VU1 programs' packet builders, and the motion file reader.
 *
 * Model_Draw draws a model with the VU1 program its descriptor names (Vu1ProgInfo,
 * gVu1ProgInfo00 ...): after the model's part packets it runs the descriptor's builder
 * (Vu1Pkt_*), which uploads the program's constants (the colours, the matrices, the
 * lights, the fog, the texture coordinate matrix, the bones' matrices) and starts the
 * program. The VU0 helpers the builders call (Vu1_MtxMul ...) are asm or inline asm, and
 * static, so that the builders keep their packet pointer in v0 across the calls.
 *
 * The motion file reader (AnimFile_*, Anim_LoadSet, Anim_ConvertChunk, Anim_ConvertKeys*)
 * converts a motion file (AnimFileHeader) into an AnimSet: per bone an AnimHeader with
 * its curves and their keys, in the forms the Anim_* evaluators read.
 */
#include "common.h"
#include "model.h"
#include "motion.h"
#include "dma.h"
#include "heap.h"
#include "light.h"

extern s32 gAnimCurveBuf;          /* the base of the curves' keys */
extern LinearHeap gAnimKeyHeap;   /* the heap of the keys */
void *LinearHeap_Alloc(struct LinearHeap *heap, u32 size, s32 flag);

/* The 16-bit keys as the motion file stores them: KeyS2, KeyS4 and KeySM
 * (motion.h) read as unsigned halves. Anim_ConvertKeysS2/S4/SM copy them
 * through these, which the match needs: a signed half loads with lh, these
 * copies load with lhu. */
typedef struct FileKeyS2 { u16 v, t; } FileKeyS2;
typedef struct FileKeyS4 { u16 v, t, in, out; } FileKeyS4;
typedef struct FileKeySM { u32 mode; u16 v, t, in, out; } FileKeySM;

void Anim_ConvertKeysF2(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysF4(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysFM(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysS2(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysS4(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysSM(AnimFileCurve *rec, AnimCurve *curve);

/* dst = a * b (4x4), as Mtx_Mul. */
static asm void Vu1_MtxMul(f32 *dst, f32 *a, f32 *b)
{
    .set noreorder
    lqc2 vf4, 0x0(a2)
    lqc2 vf5, 0x10(a2)
    lqc2 vf6, 0x20(a2)
    lqc2 vf7, 0x30(a2)
    lqc2 vf8, 0x0(a1)
    lqc2 vf9, 0x10(a1)
    lqc2 vf10, 0x20(a1)
    lqc2 vf11, 0x30(a1)
    vmulax.xyzw ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw vf8, vf7, vf8w
    vmulax.xyzw ACC, vf4, vf9x
    vmadday.xyzw ACC, vf5, vf9y
    vmaddaz.xyzw ACC, vf6, vf9z
    vmaddw.xyzw vf9, vf7, vf9w
    vmulax.xyzw ACC, vf4, vf10x
    vmadday.xyzw ACC, vf5, vf10y
    vmaddaz.xyzw ACC, vf6, vf10z
    vmaddw.xyzw vf10, vf7, vf10w
    vmulax.xyzw ACC, vf4, vf11x
    vmadday.xyzw ACC, vf5, vf11y
    vmaddaz.xyzw ACC, vf6, vf11z
    vmaddw.xyzw vf11, vf7, vf11w
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    sqc2 vf10, 0x20(a0)
    sqc2 vf11, 0x30(a0)
    jr ra
    nop
}

/* ab = a * b, dst = ab * c (4x4): the model-view matrix and model to clip space in one. */
static asm void Vu1_MtxMulChain(f32 *dst, f32 *ab, f32 *a, f32 *b, f32 *c)
{
    .set noreorder
    lqc2 vf4, 0x0(a3)
    lqc2 vf5, 0x10(a3)
    lqc2 vf6, 0x20(a3)
    lqc2 vf7, 0x30(a3)
    lqc2 vf8, 0x0(a2)
    lqc2 vf9, 0x10(a2)
    lqc2 vf10, 0x20(a2)
    lqc2 vf11, 0x30(a2)
    vmulax.xyzw ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw vf8, vf7, vf8w
    vmulax.xyzw ACC, vf4, vf9x
    vmadday.xyzw ACC, vf5, vf9y
    vmaddaz.xyzw ACC, vf6, vf9z
    vmaddw.xyzw vf9, vf7, vf9w
    vmulax.xyzw ACC, vf4, vf10x
    vmadday.xyzw ACC, vf5, vf10y
    vmaddaz.xyzw ACC, vf6, vf10z
    vmaddw.xyzw vf10, vf7, vf10w
    vmulax.xyzw ACC, vf4, vf11x
    vmadday.xyzw ACC, vf5, vf11y
    vmaddaz.xyzw ACC, vf6, vf11z
    vmaddw.xyzw vf11, vf7, vf11w
    sqc2 vf8, 0x0(a1)
    sqc2 vf9, 0x10(a1)
    sqc2 vf10, 0x20(a1)
    sqc2 vf11, 0x30(a1)
    lqc2 vf4, 0x0(t0)
    lqc2 vf5, 0x10(t0)
    lqc2 vf6, 0x20(t0)
    lqc2 vf7, 0x30(t0)
    vmulax.xyzw ACC, vf4, vf8x
    vmadday.xyzw ACC, vf5, vf8y
    vmaddaz.xyzw ACC, vf6, vf8z
    vmaddw.xyzw vf8, vf7, vf8w
    vmulax.xyzw ACC, vf4, vf9x
    vmadday.xyzw ACC, vf5, vf9y
    vmaddaz.xyzw ACC, vf6, vf9z
    vmaddw.xyzw vf9, vf7, vf9w
    vmulax.xyzw ACC, vf4, vf10x
    vmadday.xyzw ACC, vf5, vf10y
    vmaddaz.xyzw ACC, vf6, vf10z
    vmaddw.xyzw vf10, vf7, vf10w
    vmulax.xyzw ACC, vf4, vf11x
    vmadday.xyzw ACC, vf5, vf11y
    vmaddaz.xyzw ACC, vf6, vf11z
    vmaddw.xyzw vf11, vf7, vf11w
    sqc2 vf8, 0x0(a0)
    sqc2 vf9, 0x10(a0)
    sqc2 vf10, 0x20(a0)
    sqc2 vf11, 0x30(a0)
    jr ra
    nop
}

/* Takes the three light directions dirs[0..2] into model space (each times the transpose
 * of the 3x3 part of model, the model's matrix, which undoes its rotation), normalises them
 * and stores them transposed in dst, so that the VU1 program gets the three N.L with
 * one matrix product. */
static asm void Vu1_PutLightDirs(f32 *dst, f32 *model, f32 *dirs)
{
    .set noreorder
    lqc2 vf7, 0x0(a1)
    lqc2 vf8, 0x10(a1)
    lqc2 vf9, 0x20(a1)
    vaddx.x vf4, vf0, vf7x
    vaddy.x vf5, vf0, vf7y
    vaddz.x vf6, vf0, vf7z
    vaddx.y vf4, vf0, vf8x
    vaddy.y vf5, vf0, vf8y
    vaddz.y vf6, vf0, vf8z
    vaddx.z vf4, vf0, vf9x
    vaddy.z vf5, vf0, vf9y
    vaddz.z vf6, vf0, vf9z
    lqc2 vf7, 0x0(a2)
    lqc2 vf8, 0x10(a2)
    lqc2 vf9, 0x20(a2)
    vmulax.xyz ACC, vf4, vf7x
    vmadday.xyz ACC, vf5, vf7y
    vmaddz.xyz vf7, vf6, vf7z
    vmulax.xyz ACC, vf4, vf8x
    vmadday.xyz ACC, vf5, vf8y
    vmaddz.xyz vf8, vf6, vf8z
    vmulax.xyz ACC, vf4, vf9x
    vmadday.xyz ACC, vf5, vf9y
    vmaddz.xyz vf9, vf6, vf9z
    vmul.xyz vf10, vf7, vf7
    vaddy.x vf10, vf10, vf10y
    vaddz.x vf10, vf10, vf10z
    vrsqrt Q, vf0w, vf10x
    vwaitq
    vmulq.xyz vf7, vf7, Q
    vmul.xyz vf10, vf8, vf8
    vaddy.x vf10, vf10, vf10y
    vaddz.x vf10, vf10, vf10z
    vrsqrt Q, vf0w, vf10x
    vwaitq
    vmulq.xyz vf8, vf8, Q
    vmul.xyz vf10, vf9, vf9
    vaddy.x vf10, vf10, vf10y
    vaddz.x vf10, vf10, vf10z
    vrsqrt Q, vf0w, vf10x
    vwaitq
    vmulq.xyz vf9, vf9, Q
    vaddx.x vf10, vf0, vf7x
    vaddy.x vf11, vf0, vf7y
    vaddz.x vf12, vf0, vf7z
    vaddx.y vf10, vf0, vf8x
    vaddy.y vf11, vf0, vf8y
    vaddz.y vf12, vf0, vf8z
    vaddx.z vf10, vf0, vf9x
    vaddy.z vf11, vf0, vf9y
    vaddz.z vf12, vf0, vf9z
    sqc2 vf10, 0x0(a0)
    sqc2 vf11, 0x10(a0)
    sqc2 vf12, 0x20(a0)
    jr ra
    nop
}

/* A 16-byte vector, copied as one object. The lights' vectors are only 4-aligned, so
 * the copies go through the stack, where the VU0 block can load them. */
typedef struct Vec4 {
    f32 x, y, z, w;
} Vec4;

/* The render state's three lights (Gs_SetState's RS_LIGHT states set them). */
extern Light gRsLights[3];

/*
 * The light colours for the VU1 program: dst[0..2] = the three lights' colours with w = 0,
 * dst[3], the ambient colour, = min(scale * the sum of their ambient parts, scale) with
 * w = 128.0f. C around an inline VU0 block (scale goes to the I register). It is static
 * and defined before its callers, the packet builders, so that the compiler knows it
 * leaves v0 alone and keeps their packet pointer there across the call, as the game's
 * code does.
 */
static void Vu1_PutLightColors(Vec4 *dst, f32 scale)
{
    Vec4 lights[6];

    lights[0] = *(Vec4 *)gRsLights[0].color;
    lights[1] = *(Vec4 *)gRsLights[0].ambient;
    lights[2] = *(Vec4 *)gRsLights[1].color;
    lights[3] = *(Vec4 *)gRsLights[1].ambient;
    lights[4] = *(Vec4 *)gRsLights[2].color;
    lights[5] = *(Vec4 *)gRsLights[2].ambient;
    __asm__ volatile (
        ".set noreorder\n"
        "lqc2 vf4, 0x0(%0)\n"
        "lqc2 vf5, 0x20(%0)\n"
        "lqc2 vf6, 0x40(%0)\n"
        "lqc2 vf7, 0x10(%0)\n"
        "lqc2 vf8, 0x30(%0)\n"
        "lqc2 vf9, 0x50(%0)\n"
        "vsub.w vf4, vf0, vf0\n"
        "vsub.w vf5, vf0, vf0\n"
        "vsub.w vf6, vf0, vf0\n"
        "mfc1 t0, %2\n"
        "ctc2.ni t0, vi21\n"
        "vadda.xyz ACC, vf7, vf8\n"
        "vmaddw.xyz vf10, vf9, vf0w\n"
        "sqc2 vf4, 0x0(%1)\n"
        "sqc2 vf5, 0x10(%1)\n"
        "sqc2 vf6, 0x20(%1)\n"
        "vmuli.xyz vf10, vf10, I\n"
        "vminii.xyz vf10, vf10, I\n"
        "sqc2 vf10, 0x30(%1)\n"
        : : "r"(lights), "r"(dst), "f"(scale) : "t0", "memory");
    dst[3].w = 128.0f;
}

extern u8 gMtxUv[];

/* The texture coordinate matrix for the VU1 program: rows 0, 1 and 3 of gMtxUv (render
 * state RS_UV_MTX) into rows 0-2 of dst. */
static asm void Vu1_PutUvMatrix(f32 *dst)
{
    .set noreorder
    la v1, gMtxUv
    lqc2 vf4, 0x0(v1)
    lqc2 vf5, 0x10(v1)
    lqc2 vf6, 0x30(v1)
    sqc2 vf4, 0x0(a0)
    sqc2 vf5, 0x10(a0)
    sqc2 vf6, 0x20(a0)
    jr ra
    nop
}

void *Packet_Alloc(u32 size, s32 align);
extern f32 gMtxSlots[];
extern f32 gMtxView[];
extern f32 gMtxProjClip[];
extern f32 gViewportMtx[];
extern f32 gGsColor2F[];
extern f32 gGsColorF[];
extern f32 gGsFogFar;
extern f32 gGsFogNear;
extern f32 gMtxProj[];

/* A packet of n qwords of program data: a DMA tag (NEXT), a qword of VIF codes, the
 * data, and the qword that starts the program (MSCALF). */
#define VU1_PKT_BYTES(n) (((n) + 3) * 16)
#define VU1_PKT_QWC(n) ((n) + 2)    /* the DMA tag's count: what follows it */

/* The packet's first two qwords: the DMA tag (NEXT: what follows it, then the chain
 * next) and the VIF codes FLUSHA, STCYCL 4/4 and an UNPACK V4-32 of the n qwords of
 * data to VU1 address 0. The data follows from pkt[8]: qword q is pkt[8 + 4 * q]. */
#define VU1_PKT_HEAD(pkt, n, next)                  \
    (pkt)[0] = DMA_TAG_NEXT | VU1_PKT_QWC(n);       \
    (pkt)[1] = (u32)(next);                         \
    *(u64 *)&(pkt)[2] = 0;                          \
    (pkt)[4] = VIF_FLUSHA;                          \
    (pkt)[5] = 0;                                   \
    (pkt)[6] = VIF_STCYCL(4, 4);                    \
    (pkt)[7] = VIF_UNPACK_V4_32(n, 0)

/* The packet's last qword, after the n qwords of data: MSCALF, which starts the
 * program at start once the data is in. */
#define VU1_PKT_END(pkt, n, start)                  \
    (pkt)[8 + 4 * (n)] = VIF_MSCALF(start);         \
    (pkt)[9 + 4 * (n)] = 0;                         \
    (pkt)[10 + 4 * (n)] = 0;                        \
    (pkt)[11 + 4 * (n)] = 0

/*
 * dst[i] = matrix slot i of gMtxSlots for the slots whose bit is set in mask (32 slots).
 * The copy is an inline asm block: the game's code copies through a2, a3, t0 and t1,
 * where the compiler's own struct copy picks other registers. The loop is a do-while
 * because a for loop compiles with an extra branch to its test. Static, like
 * Vu1_PutLightColors, so that its callers keep their packet pointer in v0.
 */
static void Vu1_CopyMatrixSlots(MtxSlot *dst, u32 mask)
{
    MtxSlot *src;
    u32 bit;

    bit = 1;
    src = (MtxSlot *)gMtxSlots;
    do {
        if (mask & bit) {
            __asm__ volatile (
                "lq a2, 0x0(%0)\n"
                "lq a3, 0x10(%0)\n"
                "lq t0, 0x20(%0)\n"
                "lq t1, 0x30(%0)\n"
                "sq a2, 0x0(%1)\n"
                "sq a3, 0x10(%1)\n"
                "sq t0, 0x20(%1)\n"
                "sq t1, 0x30(%1)\n"
                : : "r"(src), "r"(dst) : "a2", "a3", "t0", "t1", "memory");
        }
        bit <<= 1;
        dst++;
        src++;
    } while (bit != 0);
}

/*
 * The packet builders (Vu1ProgInfo.build), one per kind of VU1 program. Model_Draw calls
 * the model's builder after building its part packets: the builder allocates a packet
 * (Packet_Alloc), fills it with the program's constants (VU1_PKT_HEAD, the data,
 * VU1_PKT_END) and returns it; the packet goes on with the chain next, the part
 * packets. Each builder is named by what it packs beyond the colour and the two
 * matrices that every one starts with:
 * - L the lights (Vu1_PutLightDirs, Vu1_PutLightColors);
 * - D the fog;
 * - V the second colour gGsColor2F;
 * - E the texture coordinate matrix (Vu1_PutUvMatrix);
 * - P the projection gMtxProj;
 * - S a skinned model: the matrices and the lights in world space, then the matrix
 *   slots of its bones (Vu1_CopyMatrixSlots, the slots in ModelEntry.boneMask).
 * Plain packs none of these. Each one's comment lists its data by qword, which is the
 * VU1 address the program reads it from.
 */

/*
 * L: 16 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9-11 the light directions (model space); 12-15 the
 * light colours and the ambient.
 */
u8 *Vu1Pkt_L(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(16), 0x10);

    VU1_PKT_HEAD(pkt, 16, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[44], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    VU1_PKT_END(pkt, 16, start);
    return (u8 *)pkt;
}

/*
 * SL: 240 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 world
 * to screen; 5-8 world to the clip test; 9-11 the light directions (world space); 12-15
 * the light colours and the ambient; 16-143 the matrix slots of the model's bones.
 */
u8 *Vu1Pkt_SL(ModelEntry *entry, u32 start, u8 *next)
{
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(240), 0x10);

    VU1_PKT_HEAD(pkt, 240, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMul((f32 *)&pkt[12], gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], gMtxView, gViewportMtx);
    ((f32 *)pkt)[44] = -gRsLights[0].dir[0];
    ((f32 *)pkt)[45] = -gRsLights[0].dir[1];
    ((f32 *)pkt)[46] = -gRsLights[0].dir[2];
    ((f32 *)pkt)[48] = -gRsLights[1].dir[0];
    ((f32 *)pkt)[49] = -gRsLights[1].dir[1];
    ((f32 *)pkt)[50] = -gRsLights[1].dir[2];
    ((f32 *)pkt)[52] = -gRsLights[2].dir[0];
    ((f32 *)pkt)[53] = -gRsLights[2].dir[1];
    ((f32 *)pkt)[54] = -gRsLights[2].dir[2];
    Vu1_CopyMatrixSlots((MtxSlot *)&pkt[72], entry->boneMask);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    VU1_PKT_END(pkt, 240, start);
    return (u8 *)pkt;
}

/*
 * Plain: 9 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model
 * to screen; 5-8 model to the clip test.
 */
u8 *Vu1Pkt_Plain(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(9), 0x10);

    VU1_PKT_HEAD(pkt, 9, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    VU1_PKT_END(pkt, 9, start);
    return (u8 *)pkt;
}

/*
 * LD: 17 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9-11 the light directions (model space); 12-15 the
 * light colours and the ambient; 16 the fog: far and 1 / (far - near).
 */
u8 *Vu1Pkt_LD(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(17), 0x10);

    VU1_PKT_HEAD(pkt, 17, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[44], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    ((f32 *)pkt)[72] = gGsFogFar;
    ((f32 *)pkt)[73] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[74] = 0;
    pkt[75] = 0;
    VU1_PKT_END(pkt, 17, start);
    return (u8 *)pkt;
}

/*
 * D: 10 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9 the fog: far and 1 / (far - near).
 */
u8 *Vu1Pkt_D(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(10), 0x10);

    VU1_PKT_HEAD(pkt, 10, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    ((f32 *)pkt)[44] = gGsFogFar;
    ((f32 *)pkt)[45] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[46] = 0;
    pkt[47] = 0;
    VU1_PKT_END(pkt, 10, start);
    return (u8 *)pkt;
}

/*
 * LE: 19 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9-11 the light directions (model space); 12-15 the
 * light colours and the ambient; 16-18 the texture coordinate matrix.
 */
u8 *Vu1Pkt_LE(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(19), 0x10);

    VU1_PKT_HEAD(pkt, 19, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[44], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    Vu1_PutUvMatrix((f32 *)&pkt[72]);
    VU1_PKT_END(pkt, 19, start);
    return (u8 *)pkt;
}

/*
 * E: 12 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9-11 the texture coordinate matrix.
 */
u8 *Vu1Pkt_E(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(12), 0x10);

    VU1_PKT_HEAD(pkt, 12, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    Vu1_PutUvMatrix((f32 *)&pkt[44]);
    VU1_PKT_END(pkt, 12, start);
    return (u8 *)pkt;
}

/*
 * LDE: 20 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model
 * to screen; 5-8 model to the clip test; 9-11 the light directions (model space); 12-15
 * the light colours and the ambient; 16-18 the texture coordinate matrix; 19 the fog: far
 * and 1 / (far - near).
 */
u8 *Vu1Pkt_LDE(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(20), 0x10);

    VU1_PKT_HEAD(pkt, 20, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[44], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    Vu1_PutUvMatrix((f32 *)&pkt[72]);
    ((f32 *)pkt)[84] = gGsFogFar;
    ((f32 *)pkt)[85] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[86] = 0;
    pkt[87] = 0;
    VU1_PKT_END(pkt, 20, start);
    return (u8 *)pkt;
}

/*
 * DE: 13 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9-11 the texture coordinate matrix; 12 the fog: far
 * and 1 / (far - near).
 */
u8 *Vu1Pkt_DE(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(13), 0x10);

    VU1_PKT_HEAD(pkt, 13, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    Vu1_PutUvMatrix((f32 *)&pkt[44]);
    ((f32 *)pkt)[56] = gGsFogFar;
    ((f32 *)pkt)[57] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[58] = 0;
    pkt[59] = 0;
    VU1_PKT_END(pkt, 13, start);
    return (u8 *)pkt;
}

/*
 * LV: 17 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the second
 * colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the light
 * directions (model space); 13-16 the light colours and the ambient.
 */
u8 *Vu1Pkt_LV(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(17), 0x10);

    VU1_PKT_HEAD(pkt, 17, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[48], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[60], entry->progInfo->colorScale);
    VU1_PKT_END(pkt, 17, start);
    return (u8 *)pkt;
}

/*
 * V: 10 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the second
 * colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test.
 */
u8 *Vu1Pkt_V(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(10), 0x10);

    VU1_PKT_HEAD(pkt, 10, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    VU1_PKT_END(pkt, 10, start);
    return (u8 *)pkt;
}

/*
 * LDV: 18 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the
 * light directions (model space); 13-16 the light colours and the ambient; 17 the fog: far
 * and 1 / (far - near).
 */
u8 *Vu1Pkt_LDV(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(18), 0x10);

    VU1_PKT_HEAD(pkt, 18, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[48], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[60], entry->progInfo->colorScale);
    ((f32 *)pkt)[76] = gGsFogFar;
    ((f32 *)pkt)[77] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[78] = 0;
    pkt[79] = 0;
    VU1_PKT_END(pkt, 18, start);
    return (u8 *)pkt;
}

/*
 * DV: 11 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the second
 * colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10 the fog: far and
 * 1 / (far - near).
 */
u8 *Vu1Pkt_DV(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(11), 0x10);

    VU1_PKT_HEAD(pkt, 11, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    ((f32 *)pkt)[48] = gGsFogFar;
    ((f32 *)pkt)[49] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[50] = 0;
    pkt[51] = 0;
    VU1_PKT_END(pkt, 11, start);
    return (u8 *)pkt;
}

/*
 * LVE: 20 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the
 * light directions (model space); 13-16 the light colours and the ambient; 17-19 the
 * texture coordinate matrix.
 */
u8 *Vu1Pkt_LVE(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(20), 0x10);

    VU1_PKT_HEAD(pkt, 20, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[48], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[60], entry->progInfo->colorScale);
    Vu1_PutUvMatrix((f32 *)&pkt[76]);
    VU1_PKT_END(pkt, 20, start);
    return (u8 *)pkt;
}

/*
 * VE: 13 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the second
 * colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the texture
 * coordinate matrix.
 */
u8 *Vu1Pkt_VE(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(13), 0x10);

    VU1_PKT_HEAD(pkt, 13, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    Vu1_PutUvMatrix((f32 *)&pkt[48]);
    VU1_PKT_END(pkt, 13, start);
    return (u8 *)pkt;
}

/*
 * LDVE: 21 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the
 * light directions (model space); 13-16 the light colours and the ambient; 17-19 the
 * texture coordinate matrix; 20 the fog: far and 1 / (far - near).
 */
u8 *Vu1Pkt_LDVE(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(21), 0x10);

    VU1_PKT_HEAD(pkt, 21, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[48], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[60], entry->progInfo->colorScale);
    Vu1_PutUvMatrix((f32 *)&pkt[76]);
    ((f32 *)pkt)[88] = gGsFogFar;
    ((f32 *)pkt)[89] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[90] = 0;
    pkt[91] = 0;
    VU1_PKT_END(pkt, 21, start);
    return (u8 *)pkt;
}

/*
 * DVE: 14 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the
 * texture coordinate matrix; 13 the fog: far and 1 / (far - near).
 */
u8 *Vu1Pkt_DVE(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(14), 0x10);

    VU1_PKT_HEAD(pkt, 14, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    Vu1_PutUvMatrix((f32 *)&pkt[48]);
    ((f32 *)pkt)[60] = gGsFogFar;
    ((f32 *)pkt)[61] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[62] = 0;
    pkt[63] = 0;
    VU1_PKT_END(pkt, 14, start);
    return (u8 *)pkt;
}

/*
 * DVP: 15 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10 the fog:
 * far and 1 / (far - near); 11-14 the projection gMtxProj.
 */
u8 *Vu1Pkt_DVP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(15), 0x10);

    VU1_PKT_HEAD(pkt, 15, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    *(MtxSlot *)&pkt[52] = *(MtxSlot *)gMtxProj;
    ((f32 *)pkt)[48] = gGsFogFar;
    ((f32 *)pkt)[49] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[50] = 0;
    pkt[51] = 0;
    VU1_PKT_END(pkt, 15, start);
    return (u8 *)pkt;
}

/*
 * DVEP: 18 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the
 * texture coordinate matrix; 13 the fog: far and 1 / (far - near); 14-17 the projection
 * gMtxProj.
 */
u8 *Vu1Pkt_DVEP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(18), 0x10);

    VU1_PKT_HEAD(pkt, 18, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    Vu1_PutUvMatrix((f32 *)&pkt[48]);
    *(MtxSlot *)&pkt[64] = *(MtxSlot *)gMtxProj;
    ((f32 *)pkt)[60] = gGsFogFar;
    ((f32 *)pkt)[61] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[62] = 0;
    pkt[63] = 0;
    VU1_PKT_END(pkt, 18, start);
    return (u8 *)pkt;
}

/*
 * LP: 20 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9-11 the light directions (model space); 12-15 the
 * light colours and the ambient; 16-19 the projection gMtxProj.
 */
u8 *Vu1Pkt_LP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(20), 0x10);

    VU1_PKT_HEAD(pkt, 20, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[44], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    *(MtxSlot *)&pkt[72] = *(MtxSlot *)gMtxProj;
    VU1_PKT_END(pkt, 20, start);
    return (u8 *)pkt;
}

/*
 * LDVEP: 25 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the
 * light directions (model space); 13-16 the light colours and the ambient; 17-19 the
 * texture coordinate matrix; 20 the fog: far and 1 / (far - near); 21-24 the projection
 * gMtxProj.
 */
u8 *Vu1Pkt_LDVEP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    f32 lightDirs[12];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(25), 0x10);

    VU1_PKT_HEAD(pkt, 25, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    lightDirs[0] = -gRsLights[0].dir[0];
    lightDirs[1] = -gRsLights[0].dir[1];
    lightDirs[2] = -gRsLights[0].dir[2];
    lightDirs[4] = -gRsLights[1].dir[0];
    lightDirs[5] = -gRsLights[1].dir[1];
    lightDirs[6] = -gRsLights[1].dir[2];
    lightDirs[8] = -gRsLights[2].dir[0];
    lightDirs[9] = -gRsLights[2].dir[1];
    lightDirs[10] = -gRsLights[2].dir[2];
    Vu1_PutLightDirs((f32 *)&pkt[48], gMtxSlots, lightDirs);
    Vu1_PutLightColors((Vec4 *)&pkt[60], entry->progInfo->colorScale);
    Vu1_PutUvMatrix((f32 *)&pkt[76]);
    ((f32 *)pkt)[88] = gGsFogFar;
    ((f32 *)pkt)[89] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[90] = 0;
    pkt[91] = 0;
    *(MtxSlot *)&pkt[92] = *(MtxSlot *)gMtxProj;
    VU1_PKT_END(pkt, 25, start);
    return (u8 *)pkt;
}

/*
 * P: 13 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9-12 the projection gMtxProj.
 */
u8 *Vu1Pkt_P(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(13), 0x10);

    VU1_PKT_HEAD(pkt, 13, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    *(MtxSlot *)&pkt[44] = *(MtxSlot *)gMtxProj;
    VU1_PKT_END(pkt, 13, start);
    return (u8 *)pkt;
}

/*
 * VP: 14 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the second
 * colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-13 the projection
 * gMtxProj.
 */
u8 *Vu1Pkt_VP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(14), 0x10);

    VU1_PKT_HEAD(pkt, 14, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    *(MtxSlot *)&pkt[48] = *(MtxSlot *)gMtxProj;
    VU1_PKT_END(pkt, 14, start);
    return (u8 *)pkt;
}

/*
 * SLD: 241 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 world
 * to screen; 5-8 world to the clip test; 9-11 the light directions (world space); 12-15
 * the light colours and the ambient; 16 the fog: far and 1 / (far - near); 17-144 the
 * matrix slots of the model's bones.
 */
u8 *Vu1Pkt_SLD(ModelEntry *entry, u32 start, u8 *next)
{
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(241), 0x10);

    VU1_PKT_HEAD(pkt, 241, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMul((f32 *)&pkt[12], gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], gMtxView, gViewportMtx);
    ((f32 *)pkt)[44] = -gRsLights[0].dir[0];
    ((f32 *)pkt)[45] = -gRsLights[0].dir[1];
    ((f32 *)pkt)[46] = -gRsLights[0].dir[2];
    ((f32 *)pkt)[48] = -gRsLights[1].dir[0];
    ((f32 *)pkt)[49] = -gRsLights[1].dir[1];
    ((f32 *)pkt)[50] = -gRsLights[1].dir[2];
    ((f32 *)pkt)[52] = -gRsLights[2].dir[0];
    ((f32 *)pkt)[53] = -gRsLights[2].dir[1];
    ((f32 *)pkt)[54] = -gRsLights[2].dir[2];
    Vu1_CopyMatrixSlots((MtxSlot *)&pkt[76], entry->boneMask);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    ((f32 *)pkt)[72] = gGsFogFar;
    ((f32 *)pkt)[73] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[74] = 0;
    pkt[75] = 0;
    VU1_PKT_END(pkt, 241, start);
    return (u8 *)pkt;
}

/*
 * SDV: 139 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2 the fog: far and 1 / (far - near); 3-6 world to screen; 7-10
 * world to the clip test; 11-138 the matrix slots of the model's bones.
 */
u8 *Vu1Pkt_SDV(ModelEntry *entry, u32 start, u8 *next)
{
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(139), 0x10);

    VU1_PKT_HEAD(pkt, 139, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMul((f32 *)&pkt[20], gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[36], gMtxView, gViewportMtx);
    Vu1_CopyMatrixSlots((MtxSlot *)&pkt[52], entry->boneMask);
    ((f32 *)pkt)[16] = gGsFogFar;
    ((f32 *)pkt)[17] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[18] = 0;
    pkt[19] = 0;
    VU1_PKT_END(pkt, 139, start);
    return (u8 *)pkt;
}

/*
 * SLE: 243 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 world
 * to screen; 5-8 world to the clip test; 9-11 the light directions (world space); 12-15
 * the light colours and the ambient; 16-18 the texture coordinate matrix; 19-146 the
 * matrix slots of the model's bones.
 */
u8 *Vu1Pkt_SLE(ModelEntry *entry, u32 start, u8 *next)
{
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(243), 0x10);

    VU1_PKT_HEAD(pkt, 243, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMul((f32 *)&pkt[12], gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], gMtxView, gViewportMtx);
    Vu1_PutUvMatrix((f32 *)&pkt[72]);
    ((f32 *)pkt)[44] = -gRsLights[0].dir[0];
    ((f32 *)pkt)[45] = -gRsLights[0].dir[1];
    ((f32 *)pkt)[46] = -gRsLights[0].dir[2];
    ((f32 *)pkt)[48] = -gRsLights[1].dir[0];
    ((f32 *)pkt)[49] = -gRsLights[1].dir[1];
    ((f32 *)pkt)[50] = -gRsLights[1].dir[2];
    ((f32 *)pkt)[52] = -gRsLights[2].dir[0];
    ((f32 *)pkt)[53] = -gRsLights[2].dir[1];
    ((f32 *)pkt)[54] = -gRsLights[2].dir[2];
    Vu1_CopyMatrixSlots((MtxSlot *)&pkt[84], entry->boneMask);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    VU1_PKT_END(pkt, 243, start);
    return (u8 *)pkt;
}

/*
 * SVE: 141 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 world to screen; 6-9 world to the clip test; 10-12 the
 * texture coordinate matrix; 13-140 the matrix slots of the model's bones.
 */
u8 *Vu1Pkt_SVE(ModelEntry *entry, u32 start, u8 *next)
{
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(141), 0x10);

    VU1_PKT_HEAD(pkt, 141, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_PutUvMatrix((f32 *)&pkt[48]);
    Vu1_MtxMul((f32 *)&pkt[16], gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], gMtxView, gViewportMtx);
    Vu1_CopyMatrixSlots((MtxSlot *)&pkt[60], entry->boneMask);
    VU1_PKT_END(pkt, 141, start);
    return (u8 *)pkt;
}

/*
 * DP: 14 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the fog:
 * far and 1 / (far - near); 2-5 model to screen; 6-9 model to the clip test; 10-13 the
 * projection gMtxProj.
 */
u8 *Vu1Pkt_DP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(14), 0x10);

    VU1_PKT_HEAD(pkt, 14, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsFogFar;
    ((f32 *)pkt)[13] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[14] = 0;
    pkt[15] = 0;
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    *(MtxSlot *)&pkt[48] = *(MtxSlot *)gMtxProj;
    VU1_PKT_END(pkt, 14, start);
    return (u8 *)pkt;
}

/*
 * EP: 16 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model to
 * screen; 5-8 model to the clip test; 9-11 the texture coordinate matrix; 12-15 the
 * projection gMtxProj.
 */
u8 *Vu1Pkt_EP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(16), 0x10);

    VU1_PKT_HEAD(pkt, 16, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_PutUvMatrix((f32 *)&pkt[44]);
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    *(MtxSlot *)&pkt[56] = *(MtxSlot *)gMtxProj;
    VU1_PKT_END(pkt, 16, start);
    return (u8 *)pkt;
}

/*
 * DEP: 17 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 model
 * to screen; 5-8 model to the clip test; 9 the fog: far and 1 / (far - near); 10-12 the
 * texture coordinate matrix; 13-16 the projection gMtxProj.
 */
u8 *Vu1Pkt_DEP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(17), 0x10);

    VU1_PKT_HEAD(pkt, 17, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_PutUvMatrix((f32 *)&pkt[48]);
    ((f32 *)pkt)[44] = gGsFogFar;
    ((f32 *)pkt)[45] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[46] = 0;
    pkt[47] = 0;
    Vu1_MtxMulChain((f32 *)&pkt[12], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], modelView, gViewportMtx);
    *(MtxSlot *)&pkt[60] = *(MtxSlot *)gMtxProj;
    VU1_PKT_END(pkt, 17, start);
    return (u8 *)pkt;
}

/*
 * SLDE: 244 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1-4 world
 * to screen; 5-8 world to the clip test; 9-11 the light directions (world space); 12-15
 * the light colours and the ambient; 16 the fog: far and 1 / (far - near); 17-19 the
 * texture coordinate matrix; 20-147 the matrix slots of the model's bones.
 */
u8 *Vu1Pkt_SLDE(ModelEntry *entry, u32 start, u8 *next)
{
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(244), 0x10);

    VU1_PKT_HEAD(pkt, 244, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    Vu1_MtxMul((f32 *)&pkt[12], gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[28], gMtxView, gViewportMtx);
    ((f32 *)pkt)[44] = -gRsLights[0].dir[0];
    ((f32 *)pkt)[45] = -gRsLights[0].dir[1];
    ((f32 *)pkt)[46] = -gRsLights[0].dir[2];
    ((f32 *)pkt)[48] = -gRsLights[1].dir[0];
    ((f32 *)pkt)[49] = -gRsLights[1].dir[1];
    ((f32 *)pkt)[50] = -gRsLights[1].dir[2];
    ((f32 *)pkt)[52] = -gRsLights[2].dir[0];
    ((f32 *)pkt)[53] = -gRsLights[2].dir[1];
    ((f32 *)pkt)[54] = -gRsLights[2].dir[2];
    Vu1_CopyMatrixSlots((MtxSlot *)&pkt[88], entry->boneMask);
    Vu1_PutLightColors((Vec4 *)&pkt[56], entry->progInfo->colorScale);
    ((f32 *)pkt)[72] = gGsFogFar;
    ((f32 *)pkt)[73] = 1.0f / (gGsFogFar - gGsFogNear);
    pkt[74] = 0;
    pkt[75] = 0;
    Vu1_PutUvMatrix((f32 *)&pkt[76]);
    VU1_PKT_END(pkt, 244, start);
    return (u8 *)pkt;
}

/*
 * VEP: 17 qwords of data: 0 the colour gGsColorF (RGB times the colour scale); 1 the
 * second colour gGsColor2F; 2-5 model to screen; 6-9 model to the clip test; 10-12 the
 * texture coordinate matrix; 13-16 the projection gMtxProj.
 */
u8 *Vu1Pkt_VEP(ModelEntry *entry, u32 start, u8 *next)
{
    f32 modelView[16];
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(17), 0x10);

    VU1_PKT_HEAD(pkt, 17, next);
    ((f32 *)pkt)[8] = gGsColorF[0] * entry->progInfo->colorScale;
    ((f32 *)pkt)[9] = gGsColorF[1] * entry->progInfo->colorScale;
    ((f32 *)pkt)[10] = gGsColorF[2] * entry->progInfo->colorScale;
    ((f32 *)pkt)[11] = gGsColorF[3];
    ((f32 *)pkt)[12] = gGsColor2F[0];
    ((f32 *)pkt)[13] = gGsColor2F[1];
    ((f32 *)pkt)[14] = gGsColor2F[2];
    ((f32 *)pkt)[15] = gGsColor2F[3];
    Vu1_MtxMulChain((f32 *)&pkt[16], modelView, gMtxSlots, gMtxView, gMtxProjClip);
    Vu1_MtxMul((f32 *)&pkt[32], modelView, gViewportMtx);
    Vu1_PutUvMatrix((f32 *)&pkt[48]);
    *(MtxSlot *)&pkt[60] = *(MtxSlot *)gMtxProj;
    VU1_PKT_END(pkt, 17, start);
    return (u8 *)pkt;
}

/* The motion file's header: the file itself. */
static void *AnimFile_Header(void *file)
{
    return file;
}

/* Chunk n of motion file file (the chunks follow the header, each `size` bytes long).
 * cur and chunk walk the chunks together: the compiler keeps the two in separate
 * registers, as the game's code does. */
u8 *AnimFile_GetChunk(void *file, s32 n)
{
    s32 i;
    u8 *header;
    u8 *cur;
    AnimFileChunk *chunk;

    header = AnimFile_Header(file);
    cur = header + sizeof(AnimFileHeader);
    chunk = (AnimFileChunk *)(header + sizeof(AnimFileHeader));
    for (i = 0; i < n; i++) {
        cur += chunk->size;
        chunk = (AnimFileChunk *)cur;
    }
    return (u8 *)chunk;
}

/* 1 and *loopTime = the file's loop time when it gives one, else 0 and *loopTime = 0. */
s32 AnimFile_GetLoopTime(void *file, f32 *loopTime)
{
    AnimFileHeader *header = AnimFile_Header(file);
    s32 loops;

    if (header->loops != 0) {
        loops = 1;
        *loopTime = header->loopTime;
    } else {
        *loopTime = 0.0f;
        loops = 0;
    }
    return loops;
}

s32 AnimFile_Count(void *file);
u8 *AnimFile_GetChunk(void *file, s32 n);
s32 Anim_ChunkKeySize(AnimFileChunk *chunk);

/* The bytes motion file file takes once converted (Anim_LoadSet): the set, the list of
 * its animations, and per chunk an AnimHeader, its curves and their keys. */
s32 Anim_ConvertedSize(void *file)
{
    s32 i;
    s32 count;
    s32 size;
    AnimFileChunk *chunk;

    count = AnimFile_Count(file);
    size = count * sizeof(AnimHeader *) + sizeof(AnimSet);

    for (i = 0; i < count; i++) {
        chunk = (AnimFileChunk *)AnimFile_GetChunk(file, i);
        size += sizeof(AnimHeader);
        size += chunk->count * sizeof(AnimCurve);
        size += Anim_ChunkKeySize(chunk);
    }
    return size;
}

/* The bytes the keys of chunk's curve records take once converted (Anim_ConvertKeys). */
s32 Anim_ChunkKeySize(AnimFileChunk *chunk)
{
    AnimFileCurve *rec;
    s32 count;
    s32 size;
    s32 i;

    count = chunk->count;
    rec = (AnimFileCurve *)(chunk + 1);
    size = 0;

    for (i = 0; i < count; i++) {
        switch (ANIM_FILE_KEY_FORMAT(rec->info)) {
        case ANIM_KEY_F32_LINEAR:
            size += rec->count * sizeof(KeyF2);
            break;
        case ANIM_KEY_F32_CUBIC:
            size += rec->count * sizeof(KeyF4);
            break;
        case ANIM_KEY_F32_MIXED:
            size += rec->count * sizeof(KeyFM);
            break;
        case ANIM_KEY_S16_LINEAR:
            size += rec->count * sizeof(KeyS2);
            break;
        case ANIM_KEY_S16_CUBIC:
            size += rec->count * sizeof(KeyS4);
            break;
        case ANIM_KEY_S16_MIXED:
            size += rec->count * sizeof(KeySM);
            break;
        }
        rec = (AnimFileCurve *)((u8 *)rec + rec->size);
    }
    return size;
}

void Anim_SetKeyBase(void *buf);
s32 LinearHeap_Init(struct LinearHeap *h, void *start, u32 size, s32 align);
s32 AnimFile_Count(void *file);
void Anim_LoadChunk(AnimHeader *anim, void *file, s32 i);
f32 Anim_GetEndTime(AnimHeader *anim);
f32 Anim_GetStartTime(AnimHeader *anim);

/*
 * Converts motion file file into buf: the set first, then the list of its animations,
 * one AnimHeader per chunk with its curves and keys, all from the heap gAnimKeyHeap over
 * buf. The set's start and end are the earliest and latest keys of all the curves. The
 * offsets in the set are made relative to buf (gAnimCurveBuf), and *out gets a copy of
 * the set with base = buf. Returns 1.
 */
s32 Anim_LoadSet(AnimSet *out, u8 *buf, void *file)
{
    s32 size;
    s32 i;
    s32 count;
    AnimSet *set;
    AnimHeader **list;
    f32 min;
    f32 max;
    f32 time;
    f32 loopTime;

    size = Anim_ConvertedSize(file);
    gAnimCurveBuf = (s32)buf;
    Anim_SetKeyBase(buf);
    LinearHeap_Init(&gAnimKeyHeap, buf, size, 1);
    set = LinearHeap_Alloc(&gAnimKeyHeap, sizeof(AnimSet), 0);
    count = AnimFile_Count(file);
    set->flags = 0;
    set->count = count;
    set->anims = (s32)LinearHeap_Alloc(&gAnimKeyHeap, count * sizeof(AnimHeader *), 0);
    max = 0.0f;
    set->base = 0;
    min = 1e10f;
    set->size = size;
    list = (AnimHeader **)set->anims;
    for (i = 0; i < count; i++, list++) {
        *list = LinearHeap_Alloc(&gAnimKeyHeap, sizeof(AnimHeader), 0);
        Anim_LoadChunk(*list, file, i);
        if ((time = Anim_GetEndTime(*list)) > max) {
            max = time;
        }
        if ((time = Anim_GetStartTime(*list)) < min) {
            min = time;
        }
    }
    set->start = min;
    set->end = max;
    if (AnimFile_GetLoopTime(file, &loopTime)) {
        set->flags |= ANIM_SET_LOOPS;
        set->loop = loopTime;
    } else {
        set->flags &= ~ANIM_SET_LOOPS;
        set->loop = 0.0f;
    }
    for (i = 0; i < count; i++) {
        ((s32 *)set->anims)[i] -= gAnimCurveBuf;
    }
    set->anims -= gAnimCurveBuf;
    *out = *set;
    out->base = (s32)buf;
    return 1;
}

s32 AnimFile_Count(void *file);
void Anim_ConvertChunk(void *file, AnimHeader *anim, AnimFileChunk *chunk, s32 unused);

/* Fills animation anim from chunk i of motion file file, its type from the file's
 * header; nothing when there is no such chunk. */
void Anim_LoadChunk(AnimHeader *anim, void *file, s32 i)
{
    u8 *chunk;
    AnimFileHeader *header;

    if (AnimFile_Count(file) < i) {
        return;
    }
    if ((chunk = AnimFile_GetChunk(file, i)) == NULL) {
        return;
    }
    header = AnimFile_Header(file);
    if (header == NULL) {
        return;
    }
    switch (header->type) {
    case ANIM_FILE_FLOAT:
        anim->flags = ANIM_TYPE_FLOAT;
        break;
    case ANIM_FILE_FIXED:
        anim->flags = ANIM_TYPE_FIXED;
        break;
    }
    switch (anim->flags & ANIM_TYPE_MASK) {
    case ANIM_TYPE_FLOAT:
    case ANIM_TYPE_FIXED:
        Anim_ConvertChunk(file, anim, (AnimFileChunk *)chunk, 0);
        break;
    }
}

void Anim_ConvertKeys(AnimFileCurve *rec, AnimCurve *curve);

/* Fills animation anim from chunk of motion file file: one AnimCurve per curve record,
 * the used ones (ANIM_FILE_CURVE_USED) converted (Anim_ConvertKeys), each driving the
 * pose value n given by its record's bit 1 << n. The others keep no keys. */
void Anim_ConvertChunk(void *file, AnimHeader *anim, AnimFileChunk *chunk, s32 unused)
{
    AnimCurve *curve;
    AnimFileCurve *rec;
    s32 count;
    s32 i;
    u32 info;

    count = chunk->count;
    anim->count = count;
    curve = LinearHeap_Alloc(&gAnimKeyHeap, count * sizeof(AnimCurve), 0);
    anim->curves = (s32)curve;
    for (i = 0; i < count; i++) {
        curve[i].type = ANIM_KEY_NONE;
        curve[i].count = 0;
    }
    rec = (AnimFileCurve *)(chunk + 1);
    for (i = 0; i < count; i++, curve++) {
        info = rec->info;
        if (info & ANIM_FILE_CURVE_USED) {
            Anim_ConvertKeys(rec, curve);
            switch (info & ANIM_FILE_POSE_BITS) {
            case 1 << 0:
                curve->idx = 0;
                break;
            case 1 << 1:
                curve->idx = 1;
                break;
            case 1 << 2:
                curve->idx = 2;
                break;
            case 1 << 3:
                curve->idx = 3;
                break;
            case 1 << 4:
                curve->idx = 4;
                break;
            case 1 << 5:
                curve->idx = 5;
                break;
            case 1 << 6:
                curve->idx = 6;
                break;
            case 1 << 7:
                curve->idx = 7;
                break;
            case 1 << 8:
                curve->idx = 8;
                break;
            }
        }
        rec = (AnimFileCurve *)((u8 *)rec + rec->size);
    }
    anim->curves -= gAnimCurveBuf;
}

/* Converts curve record rec's keys into curve, by the record's key format. */
void Anim_ConvertKeys(AnimFileCurve *rec, AnimCurve *curve)
{
    u32 format;

    format = ANIM_FILE_KEY_FORMAT((u32)rec->info);
    switch (format) {
    case ANIM_KEY_F32_LINEAR:
        Anim_ConvertKeysF2(rec, curve);
        break;
    case ANIM_KEY_F32_CUBIC:
        Anim_ConvertKeysF4(rec, curve);
        break;
    case ANIM_KEY_F32_MIXED:
        Anim_ConvertKeysFM(rec, curve);
        break;
    case ANIM_KEY_S16_LINEAR:
        Anim_ConvertKeysS2(rec, curve);
        break;
    case ANIM_KEY_S16_CUBIC:
        Anim_ConvertKeysS4(rec, curve);
        break;
    case ANIM_KEY_S16_MIXED:
        Anim_ConvertKeysSM(rec, curve);
        break;
    }
}

/* Curve curve, ANIM_KEY_F32_LINEAR, from record rec's keys (KeyF2), copied to gAnimKeyHeap. */
void Anim_ConvertKeysF2(AnimFileCurve *rec, AnimCurve *curve)
{
    KeyF2 *src = (KeyF2 *)(rec + 1);
    KeyF2 *dst;
    s32 i;

    curve->type = ANIM_KEY_F32_LINEAR;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyF2), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->v = src->v;
        dst->t = src->t;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_F32_CUBIC, from record rec's keys (KeyF4), copied to gAnimKeyHeap. */
void Anim_ConvertKeysF4(AnimFileCurve *rec, AnimCurve *curve)
{
    KeyF4 *src = (KeyF4 *)(rec + 1);
    KeyF4 *dst;
    s32 i;

    curve->type = ANIM_KEY_F32_CUBIC;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyF4), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->v = src->v;
        dst->t = src->t;
        dst->in = src->in;
        dst->out = src->out;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_F32_MIXED, from record rec's keys (KeyFM), copied to gAnimKeyHeap. */
void Anim_ConvertKeysFM(AnimFileCurve *rec, AnimCurve *curve)
{
    KeyFM *src = (KeyFM *)(rec + 1);
    KeyFM *dst;
    s32 i;

    curve->type = ANIM_KEY_F32_MIXED;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyFM), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->mode = src->mode;
        dst->v = src->v;
        dst->t = src->t;
        dst->in = src->in;
        dst->out = src->out;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_S16_LINEAR, from record rec's keys (KeyS2), copied to gAnimKeyHeap. */
void Anim_ConvertKeysS2(AnimFileCurve *rec, AnimCurve *curve)
{
    FileKeyS2 *src = (FileKeyS2 *)(rec + 1);
    KeyS2 *dst;
    s32 i;

    curve->type = ANIM_KEY_S16_LINEAR;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyS2), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->v = src->v;
        dst->t = src->t;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_S16_CUBIC, from record rec's keys (KeyS4), copied to gAnimKeyHeap. */
void Anim_ConvertKeysS4(AnimFileCurve *rec, AnimCurve *curve)
{
    FileKeyS4 *src = (FileKeyS4 *)(rec + 1);
    KeyS4 *dst;
    s32 i;

    curve->type = ANIM_KEY_S16_CUBIC;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeyS4), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->v = src->v;
        dst->t = src->t;
        dst->in = src->in;
        dst->out = src->out;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

/* Curve curve, ANIM_KEY_S16_MIXED, from record rec's keys (KeySM), copied to gAnimKeyHeap. */
void Anim_ConvertKeysSM(AnimFileCurve *rec, AnimCurve *curve)
{
    FileKeySM *src = (FileKeySM *)(rec + 1);
    KeySM *dst;
    s32 i;

    curve->type = ANIM_KEY_S16_MIXED;
    curve->idx = 0;
    curve->count = rec->count;
    dst = LinearHeap_Alloc(&gAnimKeyHeap, curve->count * sizeof(KeySM), 0);
    curve->keys = (s32)dst;
    for (i = 0; i < curve->count; i++) {
        dst->mode = src->mode;
        dst->v = src->v;
        dst->t = src->t;
        dst->in = src->in;
        dst->out = src->out;
        src++;
        dst++;
    }
    curve->keys -= gAnimCurveBuf;
}

f32 AnimCurve_GetEndTime(AnimCurve *curve);

/* The latest end time (AnimCurve_GetEndTime) of animation anim's curves, at least 0. */
f32 Anim_GetEndTime(AnimHeader *anim)
{
    f32 max = 0.0f;
    s32 i;
    f32 time;

    for (i = 0; i < anim->count; i++) {
        time = AnimCurve_GetEndTime(&((AnimCurve *)(anim->curves + gAnimCurveBuf))[i]);
        if (time > max) {
            max = time;
        }
    }
    return max;
}

f32 AnimCurve_StartTime(AnimCurve *curve);

/* The earliest start time (AnimCurve_StartTime) of animation anim's curves (1e10 when none). */
f32 Anim_GetStartTime(AnimHeader *anim)
{
    f32 min = 1e10f;
    s32 i;
    f32 time;

    for (i = 0; i < anim->count; i++) {
        time = AnimCurve_StartTime(&((AnimCurve *)(anim->curves + gAnimCurveBuf))[i]);
        if (time < min) {
            min = time;
        }
    }
    return min;
}
