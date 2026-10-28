/*
 * The VU1 programs: the choice of a model's program (Vu1_SelectProgram,
 * from its render flags and mesh kind), the programs' descriptors (this
 * file's data) and their packet builders.
 *
 * Model_Draw draws a model with the VU1 program its descriptor names (Vu1ProgInfo,
 * gVu1ProgInfo00 ...): after the model's part packets it runs the descriptor's builder
 * (Vu1Pkt_*), which uploads the program's constants (the colours, the matrices, the
 * lights, the fog, the texture coordinate matrix, the bones' matrices) and starts the
 * program. The VU0 helpers the builders call (Vu1_MtxMul ...) are asm or inline asm, and
 * static, so that the builders keep their packet pointer in v0 across the calls.
 */
#include "common.h"
#include "model.h"
#include "dma.h"
#include "light.h"
#include "vu1_packets.h"
#include "packet_tim2.h"


/* The VU1 programs' descriptors (60 of them). */
extern Vu1ProgInfo gVu1ProgInfo00;
extern Vu1ProgInfo gVu1ProgInfo01;
extern Vu1ProgInfo gVu1ProgInfo02;
extern Vu1ProgInfo gVu1ProgInfo03;
extern Vu1ProgInfo gVu1ProgInfo04;
extern Vu1ProgInfo gVu1ProgInfo05;
extern Vu1ProgInfo gVu1ProgInfo06;
extern Vu1ProgInfo gVu1ProgInfo07;
extern Vu1ProgInfo gVu1ProgInfo08;
extern Vu1ProgInfo gVu1ProgInfo09;
extern Vu1ProgInfo gVu1ProgInfo10;
extern Vu1ProgInfo gVu1ProgInfo11;
extern Vu1ProgInfo gVu1ProgInfo12;
extern Vu1ProgInfo gVu1ProgInfo13;
extern Vu1ProgInfo gVu1ProgInfo14;
extern Vu1ProgInfo gVu1ProgInfo15;
extern Vu1ProgInfo gVu1ProgInfo16;
extern Vu1ProgInfo gVu1ProgInfo17;
extern Vu1ProgInfo gVu1ProgInfo18;
extern Vu1ProgInfo gVu1ProgInfo19;
extern Vu1ProgInfo gVu1ProgInfo20;
extern Vu1ProgInfo gVu1ProgInfo21;
extern Vu1ProgInfo gVu1ProgInfo22;
extern Vu1ProgInfo gVu1ProgInfo23;
extern Vu1ProgInfo gVu1ProgInfo24;
extern Vu1ProgInfo gVu1ProgInfo25;
extern Vu1ProgInfo gVu1ProgInfo26;
extern Vu1ProgInfo gVu1ProgInfo27;
extern Vu1ProgInfo gVu1ProgInfo28;
extern Vu1ProgInfo gVu1ProgInfo29;
extern Vu1ProgInfo gVu1ProgInfo30;
extern Vu1ProgInfo gVu1ProgInfo31;
extern Vu1ProgInfo gVu1ProgInfo32;
extern Vu1ProgInfo gVu1ProgInfo33;
extern Vu1ProgInfo gVu1ProgInfo34;
extern Vu1ProgInfo gVu1ProgInfo35;
extern Vu1ProgInfo gVu1ProgInfo36;
extern Vu1ProgInfo gVu1ProgInfo37;
extern Vu1ProgInfo gVu1ProgInfo38;
extern Vu1ProgInfo gVu1ProgInfo39;
extern Vu1ProgInfo gVu1ProgInfo40;
extern Vu1ProgInfo gVu1ProgInfo41;
extern Vu1ProgInfo gVu1ProgInfo42;
extern Vu1ProgInfo gVu1ProgInfo43;
extern Vu1ProgInfo gVu1ProgInfo44;
extern Vu1ProgInfo gVu1ProgInfo45;
extern Vu1ProgInfo gVu1ProgInfo46;
extern Vu1ProgInfo gVu1ProgInfo47;
extern Vu1ProgInfo gVu1ProgInfo48;
extern Vu1ProgInfo gVu1ProgInfo49;
extern Vu1ProgInfo gVu1ProgInfo50;
extern Vu1ProgInfo gVu1ProgInfo51;
extern Vu1ProgInfo gVu1ProgInfo52;
extern Vu1ProgInfo gVu1ProgInfo53;
extern Vu1ProgInfo gVu1ProgInfo54;
extern Vu1ProgInfo gVu1ProgInfo55;
extern Vu1ProgInfo gVu1ProgInfo56;
extern Vu1ProgInfo gVu1ProgInfo57;
extern Vu1ProgInfo gVu1ProgInfo58;
extern Vu1ProgInfo gVu1ProgInfo59;

/*
 * Picks entry's VU1 program: entry->prog (0..59) and its descriptor entry->progInfo
 * (gVu1ProgInfo00 ...), from the render flags the data was built with (entry->renderFlags,
 * whose top byte, render state 93's, must be 0) and the mesh key (MESH_KEY_*); -1 and
 * NULL when no program fits. mesh is not read. The render keys are bits of gRenderFlags2,
 * set by render states without names yet (render_state.h): 0x2 by state 21, 0x20 and 0x40
 * by state 0, 0x80 by RS_SCISSOR, 0x200 by state 1, 0x4000 by state 98, 0x8000 by state
 * 18, 0x20000 by state 102. The top byte is tested by a one-case switch, as the game's
 * code does (a branch over a jump to the end).
 */
void Vu1_SelectProgram(ModelEntry *entry, MeshDesc *mesh)
{
    u32 flags;
    u32 renderKey;
    s32 meshKey;

    entry->prog = -1;
    entry->progInfo = NULL;
    flags = entry->renderFlags;
    meshKey = entry->used | entry->vtxFormat;
    renderKey = flags & 0xFFFFFF;
    switch (flags & 0xFF000000) {
    case 0:
            switch (renderKey) {
            case 514:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 0;
                    entry->progInfo = &gVu1ProgInfo00;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 1;
                    entry->progInfo = &gVu1ProgInfo01;
                    break;
                }
                break;
            case 0x20202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 2;
                    entry->progInfo = &gVu1ProgInfo02;
                    break;
                }
                break;
            case 546:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 3;
                    entry->progInfo = &gVu1ProgInfo03;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 4;
                    entry->progInfo = &gVu1ProgInfo04;
                    break;
                }
                break;
            case 0x20020:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 5;
                    entry->progInfo = &gVu1ProgInfo05;
                    break;
                }
                break;
            case 128:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 6;
                    entry->progInfo = &gVu1ProgInfo06;
                    break;
                }
                break;
            case 0x20080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 7;
                    entry->progInfo = &gVu1ProgInfo07;
                    break;
                }
                break;
            case 512:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 8;
                    entry->progInfo = &gVu1ProgInfo08;
                    break;
                }
                break;
            case 0x20200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 9;
                    entry->progInfo = &gVu1ProgInfo09;
                    break;
                }
                break;
            case 0:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 10;
                    entry->progInfo = &gVu1ProgInfo10;
                    break;
                }
                break;
            case 2:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 11;
                    entry->progInfo = &gVu1ProgInfo11;
                    break;
                }
                break;
            case 0x20000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 12;
                    entry->progInfo = &gVu1ProgInfo12;
                    break;
                }
                break;
            case 0x20002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 13;
                    entry->progInfo = &gVu1ProgInfo13;
                    break;
                }
                break;
            case 0x8222:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 14;
                    entry->progInfo = &gVu1ProgInfo14;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 15;
                    entry->progInfo = &gVu1ProgInfo15;
                    break;
                }
                break;
            case 674:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 16;
                    entry->progInfo = &gVu1ProgInfo16;
                    break;
                }
                break;
            case 642:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 17;
                    entry->progInfo = &gVu1ProgInfo17;
                    break;
                }
                break;
            case 0x8202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 18;
                    entry->progInfo = &gVu1ProgInfo18;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 19;
                    entry->progInfo = &gVu1ProgInfo19;
                    break;
                }
                break;
            case 0x28202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 20;
                    entry->progInfo = &gVu1ProgInfo20;
                    break;
                }
                break;
            case 0x8200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 21;
                    entry->progInfo = &gVu1ProgInfo21;
                    break;
                }
                break;
            case 0x8000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 22;
                    entry->progInfo = &gVu1ProgInfo22;
                    break;
                }
                break;
            case 0x8002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 23;
                    entry->progInfo = &gVu1ProgInfo23;
                    break;
                }
                break;
            case 0x28200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 24;
                    entry->progInfo = &gVu1ProgInfo24;
                    break;
                }
                break;
            case 0x28000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 25;
                    entry->progInfo = &gVu1ProgInfo25;
                    break;
                }
                break;
            case 0x28002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 26;
                    entry->progInfo = &gVu1ProgInfo26;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 27;
                    entry->progInfo = &gVu1ProgInfo27;
                    break;
                }
                break;
            case 16898:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 28;
                    entry->progInfo = &gVu1ProgInfo28;
                    break;
                }
                break;
            case 0x24202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 29;
                    entry->progInfo = &gVu1ProgInfo29;
                    break;
                }
                break;
            case 16896:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 30;
                    entry->progInfo = &gVu1ProgInfo30;
                    break;
                }
                break;
            case 0x24200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 31;
                    entry->progInfo = &gVu1ProgInfo31;
                    break;
                }
                break;
            case 16386:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 32;
                    entry->progInfo = &gVu1ProgInfo32;
                    break;
                }
                break;
            case 0x24002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 33;
                    entry->progInfo = &gVu1ProgInfo33;
                    break;
                }
                break;
            case 16384:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 34;
                    entry->progInfo = &gVu1ProgInfo34;
                    break;
                }
                break;
            case 0x24000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 35;
                    entry->progInfo = &gVu1ProgInfo35;
                    break;
                }
                break;
            case 0xC202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 36;
                    entry->progInfo = &gVu1ProgInfo36;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 37;
                    entry->progInfo = &gVu1ProgInfo37;
                    break;
                }
                break;
            case 0x2C202:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 38;
                    entry->progInfo = &gVu1ProgInfo38;
                    break;
                }
                break;
            case 0xC200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 39;
                    entry->progInfo = &gVu1ProgInfo39;
                    break;
                }
                break;
            case 0x2C200:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 40;
                    entry->progInfo = &gVu1ProgInfo40;
                    break;
                }
                break;
            case 0xC002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 41;
                    entry->progInfo = &gVu1ProgInfo41;
                    break;
                }
                break;
            case 0x2C002:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 42;
                    entry->progInfo = &gVu1ProgInfo42;
                    break;
                }
                break;
            case 0xC000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 43;
                    entry->progInfo = &gVu1ProgInfo43;
                    break;
                }
                break;
            case 0x2C000:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 44;
                    entry->progInfo = &gVu1ProgInfo44;
                    break;
                }
                break;
            case 0x28080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 45;
                    entry->progInfo = &gVu1ProgInfo45;
                    break;
                }
                break;
            case 0x2C080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 46;
                    entry->progInfo = &gVu1ProgInfo46;
                    break;
                }
                break;
            case 0x2C280:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 47;
                    entry->progInfo = &gVu1ProgInfo47;
                    break;
                }
                break;
            case 0x28022:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 48;
                    entry->progInfo = &gVu1ProgInfo48;
                    break;
                }
                break;
            case 16930:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 49;
                    entry->progInfo = &gVu1ProgInfo49;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 50;
                    entry->progInfo = &gVu1ProgInfo50;
                    break;
                }
                break;
            case 0x24020:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 51;
                    entry->progInfo = &gVu1ProgInfo51;
                    break;
                case MESH_KEY_SKINNED_MATS:
                case MESH_KEY_SKINNED:
                    entry->prog = 52;
                    entry->progInfo = &gVu1ProgInfo52;
                    break;
                }
                break;
            case 0x200A0:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 53;
                    entry->progInfo = &gVu1ProgInfo53;
                    break;
                }
                break;
            case 0x28020:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 54;
                    entry->progInfo = &gVu1ProgInfo54;
                    break;
                }
                break;
            case 0x8080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 55;
                    entry->progInfo = &gVu1ProgInfo55;
                    break;
                }
                break;
            case 16512:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 56;
                    entry->progInfo = &gVu1ProgInfo56;
                    break;
                }
                break;
            case 0xC080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 57;
                    entry->progInfo = &gVu1ProgInfo57;
                    break;
                }
                break;
            case 0x24080:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 58;
                    entry->progInfo = &gVu1ProgInfo58;
                    break;
                }
                break;
            case 0x280A0:
                switch (meshKey) {
                case MESH_KEY_RIGID:
                case MESH_KEY_RIGID_MATS:
                    entry->prog = 59;
                    entry->progInfo = &gVu1ProgInfo59;
                    break;
                }
                break;
            }
        break;
    }
}

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(16), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(240), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(9), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(17), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(10), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(19), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(12), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(20), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(13), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(17), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(10), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(18), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(11), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(20), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(13), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(21), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(14), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(15), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(18), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(20), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(25), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(13), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(14), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(241), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(139), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(243), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(141), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(14), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(16), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(17), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(244), 16);

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
    u32 *pkt = Packet_Alloc(VU1_PKT_BYTES(17), 16);

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

/* This file's initialised data: the VU1 programs' descriptors. */
/* The VU1 programs (src/sub_main/vu1_microcode.vsm) the descriptors load. */
extern u32 Vu1Prog00[];
extern u32 Vu1Prog01[];
extern u32 Vu1Prog02[];
extern u32 Vu1Prog03[];
extern u32 Vu1Prog04[];
extern u32 Vu1Prog05[];
extern u32 Vu1Prog06[];
extern u32 Vu1Prog07[];
extern u32 Vu1Prog08[];
extern u32 Vu1Prog09[];
extern u32 Vu1Prog10[];
extern u32 Vu1Prog11[];
extern u32 Vu1Prog12[];
extern u32 Vu1Prog13[];
extern u32 Vu1Prog14[];
extern u32 Vu1Prog15[];
extern u32 Vu1Prog16[];
extern u32 Vu1Prog17[];
extern u32 Vu1Prog18[];
extern u32 Vu1Prog19[];
extern u32 Vu1Prog20[];
extern u32 Vu1Prog21[];
extern u32 Vu1Prog22[];
extern u32 Vu1Prog23[];
extern u32 Vu1Prog24[];
extern u32 Vu1Prog25[];
extern u32 Vu1Prog26[];
extern u32 Vu1Prog27[];
extern u32 Vu1Prog28[];
extern u32 Vu1Prog29[];
extern u32 Vu1Prog30[];
extern u32 Vu1Prog31[];
extern u32 Vu1Prog32[];
extern u32 Vu1Prog33[];
extern u32 Vu1Prog34[];
extern u32 Vu1Prog35[];
extern u32 Vu1Prog36[];
extern u32 Vu1Prog37[];
extern u32 Vu1Prog38[];
extern u32 Vu1Prog39[];
extern u32 Vu1Prog40[];
extern u32 Vu1Prog41[];
extern u32 Vu1Prog42[];
extern u32 Vu1Prog43[];
extern u32 Vu1Prog44[];
extern u32 Vu1Prog45[];
extern u32 Vu1Prog46[];
extern u32 Vu1Prog47[];
extern u32 Vu1Prog48[];
extern u32 Vu1Prog49[];
extern u32 Vu1Prog50[];
extern u32 Vu1Prog51[];
extern u32 Vu1Prog52[];
extern u32 Vu1Prog53[];
extern u32 Vu1Prog54[];
extern u32 Vu1Prog55[];
extern u32 Vu1Prog56[];
extern u32 Vu1Prog57[];
extern u32 Vu1Prog58[];
extern u32 Vu1Prog59[];
Vu1ProgInfo gVu1ProgInfo00 = {
#include "sub_main/00402580.inc"
};
Vu1BoneLayout gVu1BoneLayout01 = {
#include "sub_main/004025B0.inc"
};
Vu1ProgInfo gVu1ProgInfo01 = {
#include "sub_main/004025C0.inc"
};
Vu1ProgInfo gVu1ProgInfo02 = {
#include "sub_main/004025F0.inc"
};
Vu1ProgInfo gVu1ProgInfo03 = {
#include "sub_main/00402620.inc"
};
Vu1BoneLayout gVu1BoneLayout04 = {
#include "sub_main/00402650.inc"
};
Vu1ProgInfo gVu1ProgInfo04 = {
#include "sub_main/00402660.inc"
};
Vu1ProgInfo gVu1ProgInfo05 = {
#include "sub_main/00402690.inc"
};
Vu1ProgInfo gVu1ProgInfo06 = {
#include "sub_main/004026C0.inc"
};
Vu1ProgInfo gVu1ProgInfo07 = {
#include "sub_main/004026F0.inc"
};
Vu1ProgInfo gVu1ProgInfo08 = {
#include "sub_main/00402720.inc"
};
Vu1ProgInfo gVu1ProgInfo09 = {
#include "sub_main/00402750.inc"
};
Vu1ProgInfo gVu1ProgInfo10 = {
#include "sub_main/00402780.inc"
};
Vu1ProgInfo gVu1ProgInfo11 = {
#include "sub_main/004027B0.inc"
};
Vu1ProgInfo gVu1ProgInfo12 = {
#include "sub_main/004027E0.inc"
};
Vu1ProgInfo gVu1ProgInfo13 = {
#include "sub_main/00402810.inc"
};
Vu1ProgInfo gVu1ProgInfo14 = {
#include "sub_main/00402840.inc"
};
Vu1BoneLayout gVu1BoneLayout15 = {
#include "sub_main/00402870.inc"
};
Vu1ProgInfo gVu1ProgInfo15 = {
#include "sub_main/00402880.inc"
};
Vu1ProgInfo gVu1ProgInfo16 = {
#include "sub_main/004028B0.inc"
};
Vu1ProgInfo gVu1ProgInfo17 = {
#include "sub_main/004028E0.inc"
};
Vu1ProgInfo gVu1ProgInfo18 = {
#include "sub_main/00402910.inc"
};
Vu1BoneLayout gVu1BoneLayout19 = {
#include "sub_main/00402940.inc"
};
Vu1ProgInfo gVu1ProgInfo19 = {
#include "sub_main/00402950.inc"
};
Vu1ProgInfo gVu1ProgInfo20 = {
#include "sub_main/00402980.inc"
};
Vu1ProgInfo gVu1ProgInfo21 = {
#include "sub_main/004029B0.inc"
};
Vu1ProgInfo gVu1ProgInfo22 = {
#include "sub_main/004029E0.inc"
};
Vu1ProgInfo gVu1ProgInfo23 = {
#include "sub_main/00402A10.inc"
};
Vu1ProgInfo gVu1ProgInfo24 = {
#include "sub_main/00402A40.inc"
};
Vu1ProgInfo gVu1ProgInfo25 = {
#include "sub_main/00402A70.inc"
};
Vu1ProgInfo gVu1ProgInfo26 = {
#include "sub_main/00402AA0.inc"
};
Vu1BoneLayout gVu1BoneLayout27 = {
#include "sub_main/00402AD0.inc"
};
Vu1ProgInfo gVu1ProgInfo27 = {
#include "sub_main/00402AE0.inc"
};
Vu1ProgInfo gVu1ProgInfo28 = {
#include "sub_main/00402B10.inc"
};
Vu1ProgInfo gVu1ProgInfo29 = {
#include "sub_main/00402B40.inc"
};
Vu1ProgInfo gVu1ProgInfo30 = {
#include "sub_main/00402B70.inc"
};
Vu1ProgInfo gVu1ProgInfo31 = {
#include "sub_main/00402BA0.inc"
};
Vu1ProgInfo gVu1ProgInfo32 = {
#include "sub_main/00402BD0.inc"
};
Vu1ProgInfo gVu1ProgInfo33 = {
#include "sub_main/00402C00.inc"
};
Vu1ProgInfo gVu1ProgInfo34 = {
#include "sub_main/00402C30.inc"
};
Vu1ProgInfo gVu1ProgInfo35 = {
#include "sub_main/00402C60.inc"
};
Vu1ProgInfo gVu1ProgInfo36 = {
#include "sub_main/00402C90.inc"
};
Vu1BoneLayout gVu1BoneLayout37 = {
#include "sub_main/00402CC0.inc"
};
Vu1ProgInfo gVu1ProgInfo37 = {
#include "sub_main/00402CD0.inc"
};
Vu1ProgInfo gVu1ProgInfo38 = {
#include "sub_main/00402D00.inc"
};
Vu1ProgInfo gVu1ProgInfo39 = {
#include "sub_main/00402D30.inc"
};
Vu1ProgInfo gVu1ProgInfo40 = {
#include "sub_main/00402D60.inc"
};
Vu1ProgInfo gVu1ProgInfo41 = {
#include "sub_main/00402D90.inc"
};
Vu1ProgInfo gVu1ProgInfo42 = {
#include "sub_main/00402DC0.inc"
};
Vu1ProgInfo gVu1ProgInfo43 = {
#include "sub_main/00402DF0.inc"
};
Vu1ProgInfo gVu1ProgInfo44 = {
#include "sub_main/00402E20.inc"
};
Vu1ProgInfo gVu1ProgInfo45 = {
#include "sub_main/00402E50.inc"
};
Vu1ProgInfo gVu1ProgInfo46 = {
#include "sub_main/00402E80.inc"
};
Vu1ProgInfo gVu1ProgInfo47 = {
#include "sub_main/00402EB0.inc"
};
Vu1ProgInfo gVu1ProgInfo48 = {
#include "sub_main/00402EE0.inc"
};
Vu1ProgInfo gVu1ProgInfo49 = {
#include "sub_main/00402F10.inc"
};
Vu1BoneLayout gVu1BoneLayout50 = {
#include "sub_main/00402F40.inc"
};
Vu1ProgInfo gVu1ProgInfo50 = {
#include "sub_main/00402F50.inc"
};
Vu1ProgInfo gVu1ProgInfo51 = {
#include "sub_main/00402F80.inc"
};
Vu1BoneLayout gVu1BoneLayout52 = {
#include "sub_main/00402FB0.inc"
};
Vu1ProgInfo gVu1ProgInfo52 = {
#include "sub_main/00402FC0.inc"
};
Vu1ProgInfo gVu1ProgInfo53 = {
#include "sub_main/00402FF0.inc"
};
Vu1ProgInfo gVu1ProgInfo54 = {
#include "sub_main/00403020.inc"
};
Vu1ProgInfo gVu1ProgInfo55 = {
#include "sub_main/00403050.inc"
};
Vu1ProgInfo gVu1ProgInfo56 = {
#include "sub_main/00403080.inc"
};
Vu1ProgInfo gVu1ProgInfo57 = {
#include "sub_main/004030B0.inc"
};
Vu1ProgInfo gVu1ProgInfo58 = {
#include "sub_main/004030E0.inc"
};
Vu1ProgInfo gVu1ProgInfo59 = {
#include "sub_main/00403110.inc"
};
