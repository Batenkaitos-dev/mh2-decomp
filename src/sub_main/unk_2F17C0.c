#include "common.h"
#include "em.h"
#include "motion.h"
#include "dma.h"

extern s32 gAnimCurveBuf;          /* the base of the curves' keys */
extern u8 gAnimKeyHeap[];         /* the heap of the keys */
struct LinearHeap;
void *LinearHeap_Alloc(struct LinearHeap *heap, u32 size, s32 flag);

/* The 16-bit keys as the motion file stores them: KeyS2, KeyS4 and KeySM
 * (motion.h) read as unsigned halves. Anim_ConvertKeysS2/S4/SM copy them
 * through these, which the match needs: a signed half loads with lh, these
 * copies load with lhu. */
typedef struct FileKeyS2 { u16 v, t; } FileKeyS2;
typedef struct FileKeyS4 { u16 v, t, in, out; } FileKeyS4;
typedef struct FileKeySM { u32 mode; u16 v, t, in, out; } FileKeySM;

/* prototypes */
void Anim_ConvertKeysF2(void *rec, AnimCurve *c);
void Anim_ConvertKeysF4(void *rec, AnimCurve *c);
void Anim_ConvertKeysFM(void *rec, AnimCurve *c);
void Anim_ConvertKeysS2(void *rec, AnimCurve *c);
void Anim_ConvertKeysS4(void *rec, AnimCurve *c);
void Anim_ConvertKeysSM(void *rec, AnimCurve *c);
/* dst = a * b (4x4), as Mtx_Mul. */
static asm void Mtx_Mul2(f32 *dst, f32 *a, f32 *b)
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
/* ab = a * b, dst = ab * c (4x4). */
static asm void Mtx_Mul3(f32 *dst, f32 *ab, f32 *a, f32 *b, f32 *c)
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
/* The rows of b * transpose(a) (3x3), each normalised, transposed into dst. */
static asm void Vu1_PutLightDirs(f32 *dst, f32 *a, f32 *b)
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

/* A 16-byte vector (4-aligned: the lights' vectors sit at 0x..4 addresses). */
typedef struct Vec4 {
    f32 x, y, z, w;
} Vec4;

/* The three lights (0x68 bytes apart, main's data): a colour at +0x00, a second colour
 * (ambient?) at +0x20, the direction at +0x30. */
extern f32 D_20CBD4[], D_20CBF4[], D_20CC3C[], D_20CC5C[], D_20CCA4[], D_20CCC4[];

/*
 * The light colours for the VU1 program: dst[0..2] = the three lights' colours with w = 0,
 * dst[3] = min(k * the sum of their second colours, k) with w = 128.0f. Compiled C around
 * an inline VU0 block (k moved to the I register). Its callers, the packet builders
 * Vu1Pkt_L etc., keep the packet pointer in v0 across the call, so it is static and
 * defined before them.
 */
static void Vu1_PutLightColors(Vec4 *dst, f32 k)
{
    Vec4 l[6];

    l[0] = *(Vec4 *)D_20CBD4;
    l[1] = *(Vec4 *)D_20CBF4;
    l[2] = *(Vec4 *)D_20CC3C;
    l[3] = *(Vec4 *)D_20CC5C;
    l[4] = *(Vec4 *)D_20CCA4;
    l[5] = *(Vec4 *)D_20CCC4;
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
        : : "r"(l), "r"(dst), "f"(k) : "t0", "memory");
    dst[3].w = 128.0f;
}
extern u8 gMtxRs19[]; /* RsMat (main) */
/* Rows 0, 1 and 3 of the matrix at 0x20E0F0 into rows 0-2 of m. */
static asm void Vu1_CopyRowsE(f32 *m)
{
    .set noreorder
    la v1, gMtxRs19
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
extern f32 gGsColor67F[];
extern f32 gGsColor0EF[];
extern f32 gGsFogFar;
extern f32 gGsFogNear;
extern f32 gMtxProj[];

typedef struct Mtx44 {
    f32 m[16];
} Mtx44;

/*
 * dst[i] = matrix slot i of gMtxSlots for the slots whose bit is set in mask (32 slots).
 * The copy is an inline asm block (lq/sq through a2, a3, t0, t1, fixed registers where
 * MWCC's own struct copy picks others); the asm is also why the loop is a do-while (a
 * for loop gets a branch to its test). Static: its callers keep the packet pointer in v0
 * across the call.
 */
static void Vu1_CopyMatrixSlots(Mtx44 *dst, u32 mask)
{
    Mtx44 *src;
    u32 bit;

    bit = 1;
    src = (Mtx44 *)gMtxSlots;
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
 * The packet builders with lighting: as below, plus the three lights' directions (negated,
 * Vu1_PutLightDirs with the matrix gMtxSlots) and colours (Vu1_PutLightColors, scaled by e's
 * program's +0x24).
 */
void Vu1Pkt_L(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x130, 0x10);

    p[0] = 0x20000012;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C100000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[44], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    p[72] = prog | 0x15000000;
    p[73] = 0;
    p[74] = 0;
    p[75] = 0;
}

void Vu1Pkt_SL(u8 *e, u32 prog, u32 next)
{
    u32 *p = Packet_Alloc(0xF30, 0x10);

    p[0] = 0x200000F2;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6CF00000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul2((f32 *)&p[12], gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], gMtxView, gViewportMtx);
    ((f32 *)p)[44] = -D_20CBD4[12 + 0];
    ((f32 *)p)[45] = -D_20CBD4[12 + 1];
    ((f32 *)p)[46] = -D_20CBD4[12 + 2];
    ((f32 *)p)[48] = -D_20CC3C[12 + 0];
    ((f32 *)p)[49] = -D_20CC3C[12 + 1];
    ((f32 *)p)[50] = -D_20CC3C[12 + 2];
    ((f32 *)p)[52] = -D_20CCA4[12 + 0];
    ((f32 *)p)[53] = -D_20CCA4[12 + 1];
    ((f32 *)p)[54] = -D_20CCA4[12 + 2];
    Vu1_CopyMatrixSlots((Mtx44 *)&p[72], *(u32 *)(e + 0x54));
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    p[968] = prog | 0x15000000;
    p[969] = 0;
    p[970] = 0;
    p[971] = 0;
}

/*
 * The VU1 packet builders, one per program kind (model e, microprogram address prog, the
 * DMA chain to continue with next): a DMA tag (NEXT), MSKPATH3, STCYCL and an UNPACK of
 * the program's data: the light colour at 0x20D820 (main) scaled by e's program's +0x24,
 * maybe the vector at 0x20D810, the matrices (Mtx_Mul3, Mtx_Mul2 by gViewportMtx), maybe
 * rows of 0x20E0F0 (Vu1_CopyRowsE), the matrix gMtxProj and the depth range at 0x219100
 * and 0x219108 (main); then MSCALF prog.
 * Named Vu1Pkt_ by what they pack: S the matrix slots (Vu1_CopyMatrixSlots, e's +0x54 mask),
 * L the lights (Vu1_PutLightDirs, Vu1_PutLightColors), D the depth range, V the vector at
 * 0x20D810, E the rows of 0x20E0F0, P the matrix gMtxProj; Plain none of these.
 */
void Vu1Pkt_Plain(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0xC0, 0x10);

    p[0] = 0x2000000B;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C090000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    p[44] = prog | 0x15000000;
    p[45] = 0;
    p[46] = 0;
    p[47] = 0;
}

void Vu1Pkt_LD(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x140, 0x10);

    p[0] = 0x20000013;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C110000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[44], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    ((f32 *)p)[72] = gGsFogFar;
    ((f32 *)p)[73] = 1.0f / (gGsFogFar - gGsFogNear);
    p[74] = 0;
    p[75] = 0;
    p[76] = prog | 0x15000000;
    p[77] = 0;
    p[78] = 0;
    p[79] = 0;
}

void Vu1Pkt_D(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0xD0, 0x10);

    p[0] = 0x2000000C;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0A0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    ((f32 *)p)[44] = gGsFogFar;
    ((f32 *)p)[45] = 1.0f / (gGsFogFar - gGsFogNear);
    p[46] = 0;
    p[47] = 0;
    p[48] = prog | 0x15000000;
    p[49] = 0;
    p[50] = 0;
    p[51] = 0;
}

void Vu1Pkt_LE(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x160, 0x10);

    p[0] = 0x20000015;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C130000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[44], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    Vu1_CopyRowsE((f32 *)&p[72]);
    p[84] = prog | 0x15000000;
    p[85] = 0;
    p[86] = 0;
    p[87] = 0;
}

void Vu1Pkt_E(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0xF0, 0x10);

    p[0] = 0x2000000E;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0C0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    Vu1_CopyRowsE((f32 *)&p[44]);
    p[56] = prog | 0x15000000;
    p[57] = 0;
    p[58] = 0;
    p[59] = 0;
}

void Vu1Pkt_LDE(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x170, 0x10);

    p[0] = 0x20000016;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C140000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[44], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    Vu1_CopyRowsE((f32 *)&p[72]);
    ((f32 *)p)[84] = gGsFogFar;
    ((f32 *)p)[85] = 1.0f / (gGsFogFar - gGsFogNear);
    p[86] = 0;
    p[87] = 0;
    p[88] = prog | 0x15000000;
    p[89] = 0;
    p[90] = 0;
    p[91] = 0;
}

void Vu1Pkt_DE(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x100, 0x10);

    p[0] = 0x2000000F;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0D0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    Vu1_CopyRowsE((f32 *)&p[44]);
    ((f32 *)p)[56] = gGsFogFar;
    ((f32 *)p)[57] = 1.0f / (gGsFogFar - gGsFogNear);
    p[58] = 0;
    p[59] = 0;
    p[60] = prog | 0x15000000;
    p[61] = 0;
    p[62] = 0;
    p[63] = 0;
}

void Vu1Pkt_LV(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x140, 0x10);

    p[0] = 0x20000013;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C110000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[48], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[60], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    p[76] = prog | 0x15000000;
    p[77] = 0;
    p[78] = 0;
    p[79] = 0;
}

void Vu1Pkt_V(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0xD0, 0x10);

    p[0] = 0x2000000C;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0A0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    p[48] = prog | 0x15000000;
    p[49] = 0;
    p[50] = 0;
    p[51] = 0;
}

void Vu1Pkt_LDV(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x150, 0x10);

    p[0] = 0x20000014;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C120000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[48], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[60], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    ((f32 *)p)[76] = gGsFogFar;
    ((f32 *)p)[77] = 1.0f / (gGsFogFar - gGsFogNear);
    p[78] = 0;
    p[79] = 0;
    p[80] = prog | 0x15000000;
    p[81] = 0;
    p[82] = 0;
    p[83] = 0;
}

void Vu1Pkt_DV(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0xE0, 0x10);

    p[0] = 0x2000000D;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0B0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    ((f32 *)p)[48] = gGsFogFar;
    ((f32 *)p)[49] = 1.0f / (gGsFogFar - gGsFogNear);
    p[50] = 0;
    p[51] = 0;
    p[52] = prog | 0x15000000;
    p[53] = 0;
    p[54] = 0;
    p[55] = 0;
}

void Vu1Pkt_LVE(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x170, 0x10);

    p[0] = 0x20000016;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C140000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[48], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[60], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    Vu1_CopyRowsE((f32 *)&p[76]);
    p[88] = prog | 0x15000000;
    p[89] = 0;
    p[90] = 0;
    p[91] = 0;
}

void Vu1Pkt_VE(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x100, 0x10);

    p[0] = 0x2000000F;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0D0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    Vu1_CopyRowsE((f32 *)&p[48]);
    p[60] = prog | 0x15000000;
    p[61] = 0;
    p[62] = 0;
    p[63] = 0;
}

void Vu1Pkt_LDVE(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x180, 0x10);

    p[0] = 0x20000017;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C150000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[48], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[60], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    Vu1_CopyRowsE((f32 *)&p[76]);
    ((f32 *)p)[88] = gGsFogFar;
    ((f32 *)p)[89] = 1.0f / (gGsFogFar - gGsFogNear);
    p[90] = 0;
    p[91] = 0;
    p[92] = prog | 0x15000000;
    p[93] = 0;
    p[94] = 0;
    p[95] = 0;
}

void Vu1Pkt_DVE(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x110, 0x10);

    p[0] = 0x20000010;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0E0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    Vu1_CopyRowsE((f32 *)&p[48]);
    ((f32 *)p)[60] = gGsFogFar;
    ((f32 *)p)[61] = 1.0f / (gGsFogFar - gGsFogNear);
    p[62] = 0;
    p[63] = 0;
    p[64] = prog | 0x15000000;
    p[65] = 0;
    p[66] = 0;
    p[67] = 0;
}

void Vu1Pkt_DVP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x120, 0x10);

    p[0] = 0x20000011;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0F0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    *(Mtx44 *)&p[52] = *(Mtx44 *)gMtxProj;
    ((f32 *)p)[48] = gGsFogFar;
    ((f32 *)p)[49] = 1.0f / (gGsFogFar - gGsFogNear);
    p[50] = 0;
    p[51] = 0;
    p[68] = prog | 0x15000000;
    p[69] = 0;
    p[70] = 0;
    p[71] = 0;
}

void Vu1Pkt_DVEP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x150, 0x10);

    p[0] = 0x20000014;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C120000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    Vu1_CopyRowsE((f32 *)&p[48]);
    *(Mtx44 *)&p[64] = *(Mtx44 *)gMtxProj;
    ((f32 *)p)[60] = gGsFogFar;
    ((f32 *)p)[61] = 1.0f / (gGsFogFar - gGsFogNear);
    p[62] = 0;
    p[63] = 0;
    p[80] = prog | 0x15000000;
    p[81] = 0;
    p[82] = 0;
    p[83] = 0;
}

void Vu1Pkt_LP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x170, 0x10);

    p[0] = 0x20000016;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C140000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[44], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    *(Mtx44 *)&p[72] = *(Mtx44 *)gMtxProj;
    p[88] = prog | 0x15000000;
    p[89] = 0;
    p[90] = 0;
    p[91] = 0;
}

void Vu1Pkt_LDVEP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    f32 l[12];
    u32 *p = Packet_Alloc(0x1C0, 0x10);

    p[0] = 0x2000001B;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C190000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    l[0] = -D_20CBD4[12 + 0];
    l[1] = -D_20CBD4[12 + 1];
    l[2] = -D_20CBD4[12 + 2];
    l[4] = -D_20CC3C[12 + 0];
    l[5] = -D_20CC3C[12 + 1];
    l[6] = -D_20CC3C[12 + 2];
    l[8] = -D_20CCA4[12 + 0];
    l[9] = -D_20CCA4[12 + 1];
    l[10] = -D_20CCA4[12 + 2];
    Vu1_PutLightDirs((f32 *)&p[48], gMtxSlots, l);
    Vu1_PutLightColors((Vec4 *)&p[60], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    Vu1_CopyRowsE((f32 *)&p[76]);
    ((f32 *)p)[88] = gGsFogFar;
    ((f32 *)p)[89] = 1.0f / (gGsFogFar - gGsFogNear);
    p[90] = 0;
    p[91] = 0;
    *(Mtx44 *)&p[92] = *(Mtx44 *)gMtxProj;
    p[108] = prog | 0x15000000;
    p[109] = 0;
    p[110] = 0;
    p[111] = 0;
}

void Vu1Pkt_P(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x100, 0x10);

    p[0] = 0x2000000F;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0D0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    *(Mtx44 *)&p[44] = *(Mtx44 *)gMtxProj;
    p[60] = prog | 0x15000000;
    p[61] = 0;
    p[62] = 0;
    p[63] = 0;
}

void Vu1Pkt_VP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x110, 0x10);

    p[0] = 0x20000010;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0E0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    *(Mtx44 *)&p[48] = *(Mtx44 *)gMtxProj;
    p[64] = prog | 0x15000000;
    p[65] = 0;
    p[66] = 0;
    p[67] = 0;
}

void Vu1Pkt_SLD(u8 *e, u32 prog, u32 next)
{
    u32 *p = Packet_Alloc(0xF40, 0x10);

    p[0] = 0x200000F3;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6CF10000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul2((f32 *)&p[12], gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], gMtxView, gViewportMtx);
    ((f32 *)p)[44] = -D_20CBD4[12 + 0];
    ((f32 *)p)[45] = -D_20CBD4[12 + 1];
    ((f32 *)p)[46] = -D_20CBD4[12 + 2];
    ((f32 *)p)[48] = -D_20CC3C[12 + 0];
    ((f32 *)p)[49] = -D_20CC3C[12 + 1];
    ((f32 *)p)[50] = -D_20CC3C[12 + 2];
    ((f32 *)p)[52] = -D_20CCA4[12 + 0];
    ((f32 *)p)[53] = -D_20CCA4[12 + 1];
    ((f32 *)p)[54] = -D_20CCA4[12 + 2];
    Vu1_CopyMatrixSlots((Mtx44 *)&p[76], *(u32 *)(e + 0x54));
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    ((f32 *)p)[72] = gGsFogFar;
    ((f32 *)p)[73] = 1.0f / (gGsFogFar - gGsFogNear);
    p[74] = 0;
    p[75] = 0;
    p[972] = prog | 0x15000000;
    p[973] = 0;
    p[974] = 0;
    p[975] = 0;
}

void Vu1Pkt_SDV(u8 *e, u32 prog, u32 next)
{
    u32 *p = Packet_Alloc(0x8E0, 0x10);

    p[0] = 0x2000008D;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C8B0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul2((f32 *)&p[20], gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[36], gMtxView, gViewportMtx);
    Vu1_CopyMatrixSlots((Mtx44 *)&p[52], *(u32 *)(e + 0x54));
    ((f32 *)p)[16] = gGsFogFar;
    ((f32 *)p)[17] = 1.0f / (gGsFogFar - gGsFogNear);
    p[18] = 0;
    p[19] = 0;
    p[564] = prog | 0x15000000;
    p[565] = 0;
    p[566] = 0;
    p[567] = 0;
}

void Vu1Pkt_SLE(u8 *e, u32 prog, u32 next)
{
    u32 *p = Packet_Alloc(0xF60, 0x10);

    p[0] = 0x200000F5;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6CF30000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul2((f32 *)&p[12], gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], gMtxView, gViewportMtx);
    Vu1_CopyRowsE((f32 *)&p[72]);
    ((f32 *)p)[44] = -D_20CBD4[12 + 0];
    ((f32 *)p)[45] = -D_20CBD4[12 + 1];
    ((f32 *)p)[46] = -D_20CBD4[12 + 2];
    ((f32 *)p)[48] = -D_20CC3C[12 + 0];
    ((f32 *)p)[49] = -D_20CC3C[12 + 1];
    ((f32 *)p)[50] = -D_20CC3C[12 + 2];
    ((f32 *)p)[52] = -D_20CCA4[12 + 0];
    ((f32 *)p)[53] = -D_20CCA4[12 + 1];
    ((f32 *)p)[54] = -D_20CCA4[12 + 2];
    Vu1_CopyMatrixSlots((Mtx44 *)&p[84], *(u32 *)(e + 0x54));
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    p[980] = prog | 0x15000000;
    p[981] = 0;
    p[982] = 0;
    p[983] = 0;
}

void Vu1Pkt_SVE(u8 *e, u32 prog, u32 next)
{
    u32 *p = Packet_Alloc(0x900, 0x10);

    p[0] = 0x2000008F;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C8D0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Vu1_CopyRowsE((f32 *)&p[48]);
    Mtx_Mul2((f32 *)&p[16], gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], gMtxView, gViewportMtx);
    Vu1_CopyMatrixSlots((Mtx44 *)&p[60], *(u32 *)(e + 0x54));
    p[572] = prog | 0x15000000;
    p[573] = 0;
    p[574] = 0;
    p[575] = 0;
}

void Vu1Pkt_DP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x110, 0x10);

    p[0] = 0x20000010;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C0E0000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsFogFar;
    ((f32 *)p)[13] = 1.0f / (gGsFogFar - gGsFogNear);
    p[14] = 0;
    p[15] = 0;
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    *(Mtx44 *)&p[48] = *(Mtx44 *)gMtxProj;
    p[64] = prog | 0x15000000;
    p[65] = 0;
    p[66] = 0;
    p[67] = 0;
}

void Vu1Pkt_EP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x130, 0x10);

    p[0] = 0x20000012;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C100000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Vu1_CopyRowsE((f32 *)&p[44]);
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    *(Mtx44 *)&p[56] = *(Mtx44 *)gMtxProj;
    p[72] = prog | 0x15000000;
    p[73] = 0;
    p[74] = 0;
    p[75] = 0;
}

void Vu1Pkt_DEP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x140, 0x10);

    p[0] = 0x20000013;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C110000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Vu1_CopyRowsE((f32 *)&p[48]);
    ((f32 *)p)[44] = gGsFogFar;
    ((f32 *)p)[45] = 1.0f / (gGsFogFar - gGsFogNear);
    p[46] = 0;
    p[47] = 0;
    Mtx_Mul3((f32 *)&p[12], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], m, gViewportMtx);
    *(Mtx44 *)&p[60] = *(Mtx44 *)gMtxProj;
    p[76] = prog | 0x15000000;
    p[77] = 0;
    p[78] = 0;
    p[79] = 0;
}

void Vu1Pkt_SLDE(u8 *e, u32 prog, u32 next)
{
    u32 *p = Packet_Alloc(0xF70, 0x10);

    p[0] = 0x200000F6;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6CF40000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    Mtx_Mul2((f32 *)&p[12], gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[28], gMtxView, gViewportMtx);
    ((f32 *)p)[44] = -D_20CBD4[12 + 0];
    ((f32 *)p)[45] = -D_20CBD4[12 + 1];
    ((f32 *)p)[46] = -D_20CBD4[12 + 2];
    ((f32 *)p)[48] = -D_20CC3C[12 + 0];
    ((f32 *)p)[49] = -D_20CC3C[12 + 1];
    ((f32 *)p)[50] = -D_20CC3C[12 + 2];
    ((f32 *)p)[52] = -D_20CCA4[12 + 0];
    ((f32 *)p)[53] = -D_20CCA4[12 + 1];
    ((f32 *)p)[54] = -D_20CCA4[12 + 2];
    Vu1_CopyMatrixSlots((Mtx44 *)&p[88], *(u32 *)(e + 0x54));
    Vu1_PutLightColors((Vec4 *)&p[56], *(f32 *)(*(u8 **)(e + 0x4C) + 0x24));
    ((f32 *)p)[72] = gGsFogFar;
    ((f32 *)p)[73] = 1.0f / (gGsFogFar - gGsFogNear);
    p[74] = 0;
    p[75] = 0;
    Vu1_CopyRowsE((f32 *)&p[76]);
    p[984] = prog | 0x15000000;
    p[985] = 0;
    p[986] = 0;
    p[987] = 0;
}

void Vu1Pkt_VEP(u8 *e, u32 prog, u32 next)
{
    f32 m[16];
    u32 *p = Packet_Alloc(0x140, 0x10);

    p[0] = 0x20000013;
    p[1] = next;
    *(u64 *)&p[2] = 0;
    p[4] = 0x13000000;
    p[5] = 0;
    p[6] = 0x01000404;
    p[7] = 0x6C110000;
    ((f32 *)p)[8] = gGsColor0EF[0] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[9] = gGsColor0EF[1] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[10] = gGsColor0EF[2] * *(f32 *)(*(u8 **)(e + 0x4C) + 0x24);
    ((f32 *)p)[11] = gGsColor0EF[3];
    ((f32 *)p)[12] = gGsColor67F[0];
    ((f32 *)p)[13] = gGsColor67F[1];
    ((f32 *)p)[14] = gGsColor67F[2];
    ((f32 *)p)[15] = gGsColor67F[3];
    Mtx_Mul3((f32 *)&p[16], m, gMtxSlots, gMtxView, gMtxProjClip);
    Mtx_Mul2((f32 *)&p[32], m, gViewportMtx);
    Vu1_CopyRowsE((f32 *)&p[48]);
    *(Mtx44 *)&p[60] = *(Mtx44 *)gMtxProj;
    p[76] = prog | 0x15000000;
    p[77] = 0;
    p[78] = 0;
    p[79] = 0;
}

static void *AnimFile_Header(void *f)
{
    return f;
}

/* A chunk of the file after its 0x14-byte header; size is the whole chunk's. */
typedef struct Chunk {
    s32 unk0;
    s32 unk4;
    s32 size;
} Chunk;

/* The n-th chunk after f's header (p and c both walk it: two registers in the original). */
u8 *AnimFile_GetChunk(void *f, s32 n)
{
    s32 i;
    u8 *h;
    u8 *p;
    Chunk *c;

    h = AnimFile_Header(f);
    p = h + 0x14;
    c = (Chunk *)(h + 0x14);
    for (i = 0; i < n; i++) {
        p += c->size;
        c = (Chunk *)p;
    }
    return (u8 *)c;
}

s32 AnimFile_GetLoopTime(void *f, f32 *out)
{
    u8 *h = AnimFile_Header(f);
    s32 r;

    if (*(s32 *)(h + 0xC) != 0) {
        r = 1;
        *out = *(f32 *)(h + 0x10);
    } else {
        *out = 0.0f;
        r = 0;
    }
    return r;
}

s32 AnimFile_Count(void *f);
u8 *AnimFile_GetChunk(void *f, s32 n);
s32 Anim_ChunkKeySize(u8 *c);

s32 Anim_ConvertedSize(void *f)
{
    s32 i;
    s32 n;
    s32 size;
    u8 *c;

    n = AnimFile_Count(f);
    size = n * 4 + 0x20;

    for (i = 0; i < n; i++) {
        c = AnimFile_GetChunk(f, i);
        size += 8;
        size += *(s32 *)(c + 4) * 8;
        size += Anim_ChunkKeySize(c);
    }
    return size;
}

/* The bytes of the keys of the n curve records of chunk c, once converted (Anim_ConvertKeys). */
s32 Anim_ChunkKeySize(u8 *c)
{
    u8 *rec;
    s32 n;
    s32 size;
    s32 i;

    n = *(s32 *)(c + 4);
    rec = c + 0xC;
    size = 0;

    for (i = 0; i < n; i++) {
        switch ((*(u32 *)rec & 0xFF0000) >> 16) {
        case ANIM_KEY_F32_LINEAR:
            size += *(s32 *)(rec + 4) * 8;
            break;
        case ANIM_KEY_F32_CUBIC:
            size += *(s32 *)(rec + 4) * 16;
            break;
        case ANIM_KEY_F32_MIXED:
            size += *(s32 *)(rec + 4) * 20;
            break;
        case ANIM_KEY_S16_LINEAR:
            size += *(s32 *)(rec + 4) * 4;
            break;
        case ANIM_KEY_S16_CUBIC:
            size += *(s32 *)(rec + 4) * 8;
            break;
        case ANIM_KEY_S16_MIXED:
            size += *(s32 *)(rec + 4) * 12;
            break;
        }
        rec += *(s32 *)(rec + 8);
    }
    return size;
}

/* An animation set (0x20 bytes): the animations of one file, converted. */
typedef struct AnimSet {
    u16 flags;                  /* 0x00: 0x8000 when it has a loop time */
    s16 count;                  /* 0x02: animations */
    f32 start;                  /* 0x04: the earliest key */
    f32 end;                    /* 0x08: the latest key */
    f32 loop;                   /* 0x0C */
    s32 anims;                  /* 0x10: AnimHeader pointers, as an offset from gAnimCurveBuf */
    s32 base;                   /* 0x14: the buffer (gAnimCurveBuf) */
    s32 size;                   /* 0x18: bytes used */
    u8 pad1C[4];
} AnimSet;

void Anim_SetKeyBase(void *buf);
struct LinearHeap;
s32 LinearHeap_Init(struct LinearHeap *h, void *start, u32 size, s32 align);
s32 AnimFile_Count(void *f);
void Anim_LoadChunk(AnimHeader *h, void *f, s32 i);
f32 Anim_GetEndTime(AnimHeader *h);
f32 Anim_GetStartTime(AnimHeader *h);

/* Converts animation file f into buf (its keys, curves and headers); out = the set. */
s32 Anim_LoadSet(AnimSet *out, u8 *buf, void *f)
{
    s32 size;
    s32 i;
    s32 n;
    AnimSet *set;
    AnimHeader **list;
    f32 min;
    f32 max;
    f32 t;
    f32 loop;

    size = Anim_ConvertedSize(f);
    gAnimCurveBuf = (s32)buf;
    Anim_SetKeyBase(buf);
    LinearHeap_Init((struct LinearHeap *)gAnimKeyHeap, buf, size, 1);
    set = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, 0x20, 0);
    n = AnimFile_Count(f);
    set->flags = 0;
    set->count = n;
    set->anims = (s32)LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, n * 4, 0);
    max = 0.0f;
    set->base = 0;
    min = 1e10f;
    set->size = size;
    list = (AnimHeader **)set->anims;
    for (i = 0; i < n; i++, list++) {
        *list = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, 8, 0);
        Anim_LoadChunk(*list, f, i);
        if ((t = Anim_GetEndTime(*list)) > max) {
            max = t;
        }
        if ((t = Anim_GetStartTime(*list)) < min) {
            min = t;
        }
    }
    set->start = min;
    set->end = max;
    if (AnimFile_GetLoopTime(f, &loop)) {
        set->flags |= 0x8000;
        set->loop = loop;
    } else {
        set->flags &= 0x7FFF;
        set->loop = 0.0f;
    }
    for (i = 0; i < n; i++) {
        ((s32 *)set->anims)[i] -= gAnimCurveBuf;
    }
    set->anims -= gAnimCurveBuf;
    *out = *set;
    out->base = (s32)buf;
    return 1;
}

s32 AnimFile_Count(void *f);
void Anim_ConvertChunk(void *f, AnimHeader *h, u8 *ch, s32 unused);

/* Animation header h from chunk i of file f, its format from the file's first byte. */
void Anim_LoadChunk(AnimHeader *h, void *f, s32 i)
{
    u8 *ch;
    u8 *hd;

    if (AnimFile_Count(f) < i) {
        return;
    }
    if ((ch = AnimFile_GetChunk(f, i)) == NULL) {
        return;
    }
    hd = AnimFile_Header(f);
    if (hd == NULL) {
        return;
    }
    switch (*hd) {
    case 1:
        h->flags = 0x1000;
        break;
    case 2:
        h->flags = 0x2000;
        break;
    }
    switch (h->flags & ANIM_TYPE_MASK) {
    case 0x1000:
    case 0x2000:
        Anim_ConvertChunk(f, h, ch, 0);
        break;
    }
}

void Anim_ConvertKeys(s32 *arg0, s32 arg1);

/* Animation header h from chunk ch of file f: its curves converted (Anim_ConvertKeys), each
 * driving pose value 0-8 by the bit in the record's low bits. */
void Anim_ConvertChunk(void *f, AnimHeader *h, u8 *ch, s32 unused)
{
    AnimCurve *c;
    u8 *rec;
    s32 n;
    s32 i;
    u32 w;

    n = *(s32 *)(ch + 4);
    h->count = n;
    c = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, n * sizeof(AnimCurve), 0);
    h->curves = (s32)c;
    for (i = 0; i < n; i++) {
        c[i].type = ANIM_KEY_NONE;
        c[i].count = 0;
    }
    rec = ch + 0xC;
    for (i = 0; i < n; i++, c++) {
        w = *(u32 *)rec;
        if (w & 0x80000000) {
            Anim_ConvertKeys((s32 *)rec, (s32)c);
            switch (w & 0x1FF) {
            case 0x1:
                c->idx = 0;
                break;
            case 0x2:
                c->idx = 1;
                break;
            case 0x4:
                c->idx = 2;
                break;
            case 0x8:
                c->idx = 3;
                break;
            case 0x10:
                c->idx = 4;
                break;
            case 0x20:
                c->idx = 5;
                break;
            case 0x40:
                c->idx = 6;
                break;
            case 0x80:
                c->idx = 7;
                break;
            case 0x100:
                c->idx = 8;
                break;
            }
        }
        rec += *(s32 *)(rec + 8);
    }
    h->curves -= gAnimCurveBuf;
}

void Anim_ConvertKeys(s32 *rec, s32 arg1)
{
    u32 temp_a2;

    temp_a2 = (u32) (*rec & 0xFF0000) >> 0x10;
    switch (temp_a2) {
    case 33:
        Anim_ConvertKeysF2(rec, (void *) arg1);
        break;
    case 34:
        Anim_ConvertKeysF4(rec, (void *) arg1);
        break;
    case 35:
        Anim_ConvertKeysFM(rec, (void *) arg1);
        break;
    case 17:
        Anim_ConvertKeysS2(rec, (void *) arg1);
        break;
    case 18:
        Anim_ConvertKeysS4(rec, (void *) arg1);
        break;
    case 19:
        Anim_ConvertKeysSM(rec, (void *) arg1);
        break;
    }
}

/* Curve c in format 0x21 from the keys of record rec (their count at +4, the keys at +0xC). */
void Anim_ConvertKeysF2(void *rec, AnimCurve *c)
{
    KeyF2 *s = (KeyF2 *)((u8 *)rec + 0xC);
    KeyF2 *d;
    s32 i;

    c->type = ANIM_KEY_F32_LINEAR;
    c->idx = 0;
    c->count = *(s32 *)((u8 *)rec + 4);
    d = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, c->count * sizeof(KeyF2), 0);
    c->keys = (s32)d;
    for (i = 0; i < c->count; i++) {
        d->v = s->v;
        d->t = s->t;
        s++;
        d++;
    }
    c->keys -= gAnimCurveBuf;
}

/* Curve c in format 0x22 from the keys of record rec (their count at +4, the keys at +0xC). */
void Anim_ConvertKeysF4(void *rec, AnimCurve *c)
{
    KeyF4 *s = (KeyF4 *)((u8 *)rec + 0xC);
    KeyF4 *d;
    s32 i;

    c->type = ANIM_KEY_F32_CUBIC;
    c->idx = 0;
    c->count = *(s32 *)((u8 *)rec + 4);
    d = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, c->count * sizeof(KeyF4), 0);
    c->keys = (s32)d;
    for (i = 0; i < c->count; i++) {
        d->v = s->v;
        d->t = s->t;
        d->in = s->in;
        d->out = s->out;
        s++;
        d++;
    }
    c->keys -= gAnimCurveBuf;
}

/* Curve c in format 0x23 from the keys of record rec (their count at +4, the keys at +0xC). */
void Anim_ConvertKeysFM(void *rec, AnimCurve *c)
{
    KeyFM *s = (KeyFM *)((u8 *)rec + 0xC);
    KeyFM *d;
    s32 i;

    c->type = ANIM_KEY_F32_MIXED;
    c->idx = 0;
    c->count = *(s32 *)((u8 *)rec + 4);
    d = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, c->count * sizeof(KeyFM), 0);
    c->keys = (s32)d;
    for (i = 0; i < c->count; i++) {
        d->mode = s->mode;
        d->v = s->v;
        d->t = s->t;
        d->in = s->in;
        d->out = s->out;
        s++;
        d++;
    }
    c->keys -= gAnimCurveBuf;
}

/* Curve c in format 0x11 from the keys of record rec (their count at +4, the keys at +0xC). */
void Anim_ConvertKeysS2(void *rec, AnimCurve *c)
{
    FileKeyS2 *s = (FileKeyS2 *)((u8 *)rec + 0xC);
    KeyS2 *d;
    s32 i;

    c->type = ANIM_KEY_S16_LINEAR;
    c->idx = 0;
    c->count = *(s32 *)((u8 *)rec + 4);
    d = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, c->count * sizeof(KeyS2), 0);
    c->keys = (s32)d;
    for (i = 0; i < c->count; i++) {
        d->v = s->v;
        d->t = s->t;
        s++;
        d++;
    }
    c->keys -= gAnimCurveBuf;
}

/* Curve c in format 0x12 from the keys of record rec (their count at +4, the keys at +0xC). */
void Anim_ConvertKeysS4(void *rec, AnimCurve *c)
{
    FileKeyS4 *s = (FileKeyS4 *)((u8 *)rec + 0xC);
    KeyS4 *d;
    s32 i;

    c->type = ANIM_KEY_S16_CUBIC;
    c->idx = 0;
    c->count = *(s32 *)((u8 *)rec + 4);
    d = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, c->count * sizeof(KeyS4), 0);
    c->keys = (s32)d;
    for (i = 0; i < c->count; i++) {
        d->v = s->v;
        d->t = s->t;
        d->in = s->in;
        d->out = s->out;
        s++;
        d++;
    }
    c->keys -= gAnimCurveBuf;
}

/* Curve c in format 0x13 from the keys of record rec (their count at +4, the keys at +0xC). */
void Anim_ConvertKeysSM(void *rec, AnimCurve *c)
{
    FileKeySM *s = (FileKeySM *)((u8 *)rec + 0xC);
    KeySM *d;
    s32 i;

    c->type = ANIM_KEY_S16_MIXED;
    c->idx = 0;
    c->count = *(s32 *)((u8 *)rec + 4);
    d = LinearHeap_Alloc((struct LinearHeap *)gAnimKeyHeap, c->count * sizeof(KeySM), 0);
    c->keys = (s32)d;
    for (i = 0; i < c->count; i++) {
        d->mode = s->mode;
        d->v = s->v;
        d->t = s->t;
        d->in = s->in;
        d->out = s->out;
        s++;
        d++;
    }
    c->keys -= gAnimCurveBuf;
}

f32 AnimCurve_GetEndTime(AnimCurve *c);

/* The latest end time (AnimCurve_GetEndTime) of the curves of animation h, at least 0. */
f32 Anim_GetEndTime(AnimHeader *h)
{
    f32 max = 0.0f;
    s32 i;
    f32 t;

    for (i = 0; i < h->count; i++) {
        t = AnimCurve_GetEndTime(&((AnimCurve *)(h->curves + gAnimCurveBuf))[i]);
        if (t > max) {
            max = t;
        }
    }
    return max;
}

f32 AnimCurve_StartTime(AnimCurve *c);

/* The earliest start time (AnimCurve_StartTime) of the curves of animation h (1e10 when none). */
f32 Anim_GetStartTime(AnimHeader *h)
{
    f32 min = 1e10f;
    s32 i;
    f32 t;

    for (i = 0; i < h->count; i++) {
        t = AnimCurve_StartTime(&((AnimCurve *)(h->curves + gAnimCurveBuf))[i]);
        if (t < min) {
            min = t;
        }
    }
    return min;
}
