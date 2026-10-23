/*
 * The render state's GS registers: SCISSOR, ZBUF, TEST, ALPHA, FOGCOL and
 * TEX1. The render state's store (Gs_SetState, Gs_ResetState) calls these
 * setters with gRenderFlags when a state changes. Each one builds a small
 * packet, an A+D head (AD_PACKET_HEAD) and the register for drawing
 * context 1, 2 or both (GS_CTX_*), and queues it in the frame's chain;
 * SCISSOR, ZBUF and TEST are also kept in globals (gGsScissor1 ...), which
 * the frame's clear puts back after it draws. The texture registers that
 * follow in the game are another unit (built at another optimisation
 * level).
 */
#include "common.h"
#include "dma.h"
#include "texture.h"
#include "hw_regs.h"
#include "render_state.h"
#include "gs_regs.h"
#include "packet_tim2.h"

extern DmaQueue gDmaQueue;
extern s32 gSysZPsm;
extern u32 gSysZBlock;
extern u64 gGsTest1;
extern u64 gGsTest2;
extern u64 gGsZbuf1;
extern u64 gGsZbuf2;
extern u64 gGsScissor1;
extern s32 gScissor1X0;
extern s32 gScissor1Y0;
extern s32 gScissor1X1;
extern s32 gScissor1Y1;
extern u64 gGsScissor2;
extern s32 gScissor2X0;
extern s32 gScissor2Y0;
extern s32 gScissor2X1;
extern s32 gScissor2Y1;
extern s32 gGsAlphaRef;
extern s32 gDispHeight;
extern s32 gDispWidth;

/* The render state's unit was built without copy propagation: a global is
 * reloaded after each store, and a packet pointer's step is not folded into
 * the offsets of the stores that follow it. */
#pragma opt_propagation off

/*
 * Writes the head of an A+D packet of qwc quadwords at p (u64 *): a DMA
 * END tag, the VIF's FLUSHA and `direct` (VIF_DIRECT or VIF_DIRECTHL) for
 * the qwc quadwords, and a GIF tag of qwc - 1 A+D register writes. The
 * writes follow it, from p + 4. AD_PACKET_SIZE is the packet's size for
 * a number of writes.
 */
#define AD_PACKET_SIZE(writes) ((2 + (writes)) * 16)  /* bytes, with its head */
#define AD_PACKET_HEAD(p, qwc, direct) \
    (p)[0] = ((qwc) + DMA_TAG_END) | DMA_TAG_IRQ; \
    ((u32 *)(p))[2] = VIF_FLUSHA; \
    ((u32 *)(p))[3] = (qwc) | (direct); \
    (p)[2] = (u64)((qwc) - 1) | GIF_TAG_EOP | GIF_TAG_NREG(1); \
    (p)[3] = GIF_REG_AD

/*
 * Sets the scissor rectangle at x, y, width by height, clipped to the
 * display (gDispWidth, gDispHeight), for the drawing context ctx
 * (GS_CTX_1, GS_CTX_2 or GS_CTX_BOTH). Keeps the corners in gScissor1X0.. and
 * gScissor2X0.., the register in gGsScissor1/2, and queues the packet.
 * Returns 1.
 */
s32 Gs_SetScissor(s32 x, s32 y, s32 width, s32 height, s32 ctx)
{
    s32 lastX;
    s32 lastY;
    u64 *pkt;
    u64 *pktCursor;
    u32 qwCount;
    u64 scissorReg;

    if (x < 0) {
        x = 0;
    }
    if (x + width > gDispWidth) {
        width = gDispWidth - x;
    }
    lastX = x + width - 1;
    if (y < 0) {
        y = 0;
    }
    if (y + height > gDispHeight) {
        height = gDispHeight - y;
    }
    lastY = y + height - 1;
    if (ctx != GS_CTX_BOTH) {
        pktCursor = Packet_Alloc(AD_PACKET_SIZE(1), 16);
        qwCount = 2;
    } else {
        pktCursor = Packet_Alloc(AD_PACKET_SIZE(2), 16);
        qwCount = 3;
    }
    pkt = pktCursor;
    AD_PACKET_HEAD(pktCursor, qwCount, VIF_DIRECT);
    pktCursor += 4;
    switch (ctx) {
    case GS_CTX_1:
        gScissor1X0 = x;
        gScissor1Y0 = y;
        gScissor1X1 = lastX;
        gScissor1Y1 = lastY;
        gGsScissor1 = (u64)x | ((u64)lastX << 16) | ((u64)y << 32) | ((u64)lastY << 48);
        pktCursor[0] = gGsScissor1;
        pktCursor[1] = GS_REG_SCISSOR_1;
        break;
    case GS_CTX_2:
        gScissor2X0 = x;
        gScissor2Y0 = y;
        gScissor2X1 = lastX;
        gScissor2Y1 = lastY;
        gGsScissor2 = (u64)x | ((u64)lastX << 16) | ((u64)y << 32) | ((u64)lastY << 48);
        pktCursor[0] = gGsScissor2;
        pktCursor[1] = GS_REG_SCISSOR_2;
        break;
    case GS_CTX_BOTH:
        gScissor1X0 = x;
        gScissor1Y0 = y;
        gScissor1X1 = lastX;
        gScissor1Y1 = lastY;
        gScissor2X0 = x;
        gScissor2Y0 = y;
        gScissor2X1 = lastX;
        gScissor2Y1 = lastY;
        scissorReg = (u64)x | ((u64)lastX << 16) | ((u64)y << 32) | ((u64)lastY << 48);
        gGsScissor1 = scissorReg;
        gGsScissor2 = scissorReg;
        pktCursor[0] = scissorReg;
        pktCursor[1] = GS_REG_SCISSOR_1;
        pktCursor[2] = scissorReg;
        pktCursor[3] = GS_REG_SCISSOR_2;
        break;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

/*
 * Sets ZBUF for the drawing context ctx (GS_CTX_*): the Z buffer at
 * gSysZBlock in format gSysZPsm, with Z writes masked unless flags has
 * RF_ZWRITE. Keeps it in gGsZbuf1/2 and queues the packet. Returns 1.
 */
s32 Gs_SetZbuf(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *pktCursor;
    u32 qwCount;
    u64 zmsk;
    u64 zbuf;

    zmsk = (s32)(flags & RF_ZWRITE) != RF_ZWRITE;
    zbuf = (u64)(s32)(gSysZBlock >> 5) | ((u64)gSysZPsm << 24) | (zmsk << 32);
    if (ctx != GS_CTX_BOTH) {
        pktCursor = Packet_Alloc(AD_PACKET_SIZE(1), 16);
        qwCount = 2;
    } else {
        pktCursor = Packet_Alloc(AD_PACKET_SIZE(2), 16);
        qwCount = 3;
    }
    pkt = pktCursor;
    AD_PACKET_HEAD(pktCursor, qwCount, VIF_DIRECT);
    pktCursor += 4;
    switch (ctx) {
    case GS_CTX_1:
        gGsZbuf1 = zbuf;
        pktCursor[0] = zbuf;
        pktCursor[1] = GS_REG_ZBUF_1;
        break;
    case GS_CTX_2:
        gGsZbuf2 = zbuf;
        pktCursor[0] = zbuf;
        pktCursor[1] = GS_REG_ZBUF_2;
        break;
    case GS_CTX_BOTH:
        gGsZbuf1 = zbuf;
        gGsZbuf2 = zbuf;
        pktCursor[0] = zbuf;
        pktCursor[1] = GS_REG_ZBUF_1;
        pktCursor[2] = zbuf;
        pktCursor[3] = GS_REG_ZBUF_2;
        break;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

/*
 * Sets TEST for the drawing context ctx (GS_CTX_*), kept in gGsTest1/2.
 * The alpha test is always on, its comparison flags' RF_ATEST and its
 * reference gGsAlphaRef (0-255, scaled to the GS's 0-128). The depth test
 * is always on, its comparison flags' RF_ZTEST: the GS's depth grows
 * toward the viewer, so RS_CMP_LESS and RS_CMP_LEQUAL become GREATER and
 * GEQUAL; NEVER stays, and the comparisons the GS lacks pass always.
 * Returns 1.
 */
s32 Gs_SetTest(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *pktCursor;
    u32 qwCount;
    s32 atst;
    s32 ztst;
    s32 aref;
    u64 test;

    aref = gGsAlphaRef;
    if (aref == 255) {
        aref = 128;
    } else if (aref != 0) {
        aref >>= 1;
        if (aref == 0) {
            aref = 1;
        }
    }
    switch (flags & RF_ATEST_MASK) {
    case RF_ATEST(RS_CMP_NEVER):
        atst = GS_ATST_NEVER;
        break;
    case RF_ATEST(RS_CMP_ALWAYS):
        atst = GS_ATST_ALWAYS;
        break;
    case RF_ATEST(RS_CMP_LESS):
        atst = GS_ATST_LESS;
        break;
    case RF_ATEST(RS_CMP_LEQUAL):
        atst = GS_ATST_LEQUAL;
        break;
    case RF_ATEST(RS_CMP_EQUAL):
        atst = GS_ATST_EQUAL;
        break;
    case RF_ATEST(RS_CMP_GEQUAL):
        atst = GS_ATST_GEQUAL;
        break;
    case RF_ATEST(RS_CMP_GREATER):
        atst = GS_ATST_GREATER;
        break;
    case RF_ATEST(RS_CMP_NOTEQUAL):
        atst = GS_ATST_NOTEQUAL;
        break;
    }
    switch (flags & RF_ZTEST_MASK) {
    case RF_ZTEST(RS_CMP_LESS):
        ztst = GS_ZTST_GREATER;
        break;
    case RF_ZTEST(RS_CMP_LEQUAL):
        ztst = GS_ZTST_GEQUAL;
        break;
    case RF_ZTEST(RS_CMP_ALWAYS):
        ztst = GS_ZTST_ALWAYS;
        break;
    case RF_ZTEST(RS_CMP_NEVER):
        ztst = GS_ZTST_NEVER;
        break;
    case RF_ZTEST(RS_CMP_EQUAL):
    case RF_ZTEST(RS_CMP_NOTEQUAL):
    case RF_ZTEST(RS_CMP_GREATER):
    case RF_ZTEST(RS_CMP_GEQUAL):
        ztst = GS_ZTST_ALWAYS;
        break;
    }
    test = ((u64)aref << 4) | (GS_TEST_ATE | ((u64)atst << 1)) | GS_TEST_ZTE | ((u64)ztst << 17);
    if (ctx != GS_CTX_BOTH) {
        pktCursor = Packet_Alloc(AD_PACKET_SIZE(1), 16);
        qwCount = 2;
    } else {
        pktCursor = Packet_Alloc(AD_PACKET_SIZE(2), 16);
        qwCount = 3;
    }
    pkt = pktCursor;
    AD_PACKET_HEAD(pktCursor, qwCount, VIF_DIRECT);
    pktCursor += 4;
    switch (ctx) {
    case GS_CTX_1:
        gGsTest1 = test;
        pktCursor[0] = test;
        pktCursor[1] = GS_REG_TEST_1;
        break;
    case GS_CTX_2:
        gGsTest2 = test;
        pktCursor[0] = test;
        pktCursor[1] = GS_REG_TEST_2;
        break;
    case GS_CTX_BOTH:
        gGsTest1 = test;
        gGsTest2 = test;
        pktCursor[0] = test;
        pktCursor[1] = GS_REG_TEST_1;
        pktCursor[2] = test;
        pktCursor[3] = GS_REG_TEST_2;
        break;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

/*
 * ALPHA's value: the GS blends (A - B) * C + D. A, B and D each pick the
 * source colour, the destination's or zero; C the source's alpha, the
 * destination's or FIX (ALPHA_FIX_ONE is 1.0).
 */
#define ALPHA(a, b, c, d, fix) \
    ((u64)(a) | ((u64)(b) << 2) | ((u64)(c) << 4) | ((u64)(d) << 6) | ((u64)(fix) << 32))
#define ALPHA_CS 0
#define ALPHA_CD 1
#define ALPHA_ZERO 2
#define ALPHA_AS 0
#define ALPHA_AD 1
#define ALPHA_FIX 2
#define ALPHA_FIX_ONE 128

/*
 * Sets the ALPHA blend equation for the drawing context ctx (GS_CTX_*):
 * flags' RF_BLEND_OP_MASK picks the family (add, subtract, reverse
 * subtract) and its RS_BLEND nibbles the source's and the destination's
 * factors (RS_FACTOR_*), each pair an equation of the GS's (A - B) * C +
 * D form; without RF_BLEND_ON blending is off (the source colour as is). Keeps nothing in globals. Returns 0 for a combination with no
 * equation (nothing queued), else 1.
 */
s32 Gs_SetAlpha(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *pktCursor;
    u32 qwCount;
    u32 mode;
    u64 alpha;

    if (ctx != GS_CTX_BOTH) {
        pktCursor = Packet_Alloc(AD_PACKET_SIZE(1), 16);
        qwCount = 2;
    } else {
        pktCursor = Packet_Alloc(AD_PACKET_SIZE(2), 16);
        qwCount = 3;
    }
    pkt = pktCursor;
    AD_PACKET_HEAD(pktCursor, qwCount, VIF_DIRECT);
    pktCursor += 4;
    mode = flags & RF_BLEND_OP_MASK;
    if (mode == RF_BLEND_OP_ADD) {
        switch (flags & RS_BLEND_SRC_MASK) {
        case RS_FACTOR_ONE:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ONE):
                alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_FIX, ALPHA_CD, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_SRC_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_ZERO, ALPHA_AS, ALPHA_CS, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_INV_SRC_ALPHA):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AS, ALPHA_CS, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_DST_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_ZERO, ALPHA_AD, ALPHA_CS, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_INV_DST_ALPHA):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AD, ALPHA_CS, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_ZERO:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ONE):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_ZERO, ALPHA_FIX, ALPHA_CD, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_ZERO, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_SRC_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_ZERO, ALPHA_AS, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_INV_SRC_ALPHA):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AS, ALPHA_CD, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_DST_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_ZERO, ALPHA_AD, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_INV_DST_ALPHA):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AD, ALPHA_CD, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_SRC_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ONE):
                alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_AS, ALPHA_CD, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_AS, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_INV_SRC_ALPHA):
                alpha = ALPHA(ALPHA_CS, ALPHA_CD, ALPHA_AS, ALPHA_CD, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_INV_SRC_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CS, ALPHA_AS, ALPHA_CS, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_SRC_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_CS, ALPHA_AS, ALPHA_CS, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_DST_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ONE):
                alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_AD, ALPHA_CD, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_AD, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_INV_DST_ALPHA):
                alpha = ALPHA(ALPHA_CS, ALPHA_CD, ALPHA_AD, ALPHA_CD, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_INV_DST_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CS, ALPHA_AD, ALPHA_CS, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_DST_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_CS, ALPHA_AD, ALPHA_CS, 0);
                break;
            default:
                return 0;
            }
            break;
        case 8:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_SRC_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_ZERO, ALPHA_AS, ALPHA_CD, 0);
                break;
            default:
                return 0;
            }
            break;
        default:
            return 0;
        }
    } else if (mode == RF_BLEND_OP_SUB) {
        switch (flags & RS_BLEND_SRC_MASK) {
        case RS_FACTOR_ONE:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ONE):
                alpha = ALPHA(ALPHA_CS, ALPHA_CD, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_ZERO, ALPHA_FIX, ALPHA_CS, ALPHA_FIX_ONE);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_ZERO:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ONE):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_ZERO, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_SRC_ALPHA):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AS, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_DST_ALPHA):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AD, ALPHA_ZERO, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_SRC_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_AS, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_SRC_ALPHA):
                alpha = ALPHA(ALPHA_CS, ALPHA_CD, ALPHA_AS, ALPHA_ZERO, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_INV_SRC_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CS, ALPHA_AS, ALPHA_CS, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_DST_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_AD, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_DST_ALPHA):
                alpha = ALPHA(ALPHA_CS, ALPHA_CD, ALPHA_AD, ALPHA_ZERO, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_INV_DST_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AD, ALPHA_CD, 0);
                break;
            default:
                return 0;
            }
            break;
        default:
            return 0;
        }
    } else if (mode == RF_BLEND_OP_REV_SUB) {
        switch (flags & RS_BLEND_SRC_MASK) {
        case RS_FACTOR_ONE:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ONE):
                alpha = ALPHA(ALPHA_CD, ALPHA_CS, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CS, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_ZERO:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ONE):
                alpha = ALPHA(ALPHA_CD, ALPHA_ZERO, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_ZERO, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
                break;
            case RS_BLEND_DST(RS_FACTOR_SRC_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_ZERO, ALPHA_AS, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_INV_SRC_ALPHA):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AS, ALPHA_CD, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_DST_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_ZERO, ALPHA_AD, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_INV_DST_ALPHA):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CD, ALPHA_AD, ALPHA_CD, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_SRC_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CS, ALPHA_AS, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_SRC_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_CS, ALPHA_AS, ALPHA_ZERO, 0);
                break;
            default:
                return 0;
            }
            break;
        case RS_FACTOR_DST_ALPHA:
            switch (flags & RS_BLEND_DST_MASK) {
            case RS_BLEND_DST(RS_FACTOR_ZERO):
                alpha = ALPHA(ALPHA_ZERO, ALPHA_CS, ALPHA_AD, ALPHA_ZERO, 0);
                break;
            case RS_BLEND_DST(RS_FACTOR_DST_ALPHA):
                alpha = ALPHA(ALPHA_CD, ALPHA_CS, ALPHA_AD, ALPHA_ZERO, 0);
                break;
            default:
                return 0;
            }
            break;
        default:
            return 0;
        }
    }
    if (!(flags & RF_BLEND_ON)) {
        alpha = ALPHA(ALPHA_CS, ALPHA_ZERO, ALPHA_FIX, ALPHA_ZERO, ALPHA_FIX_ONE);
    }
    switch (ctx) {
    case GS_CTX_1:
        pktCursor[0] = alpha;
        pktCursor[1] = GS_REG_ALPHA_1;
        break;
    case GS_CTX_2:
        pktCursor[0] = alpha;
        pktCursor[1] = GS_REG_ALPHA_2;
        break;
    case GS_CTX_BOTH:
        pktCursor[0] = alpha;
        pktCursor[1] = GS_REG_ALPHA_1;
        pktCursor[2] = alpha;
        pktCursor[3] = GS_REG_ALPHA_2;
        break;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

/* Queues the fog colour (FOGCOL) from color, 0xRRGGBB. Returns 1. */
s32 Gs_SetFogColor(u32 color)
{
    u64 *pkt;
    u64 *pktCursor;

    pkt = pktCursor = Packet_Alloc(AD_PACKET_SIZE(1), 16);
    AD_PACKET_HEAD(pktCursor, 2, VIF_DIRECTHL);
    pktCursor += 4;
    pktCursor[0] = (u64)((color >> 16) & 0xFF) | ((u64)((color >> 8) & 0xFF) << 8) |
           ((u64)(color & 0xFF) << 16);
    pktCursor[1] = GS_REG_FOGCOL;
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

/*
 * Sets TEX1 for context 1 (GS_CTX_1) or 2 (any other ctx): point sampling
 * when flags has RF_TEX_NEAREST, else bilinear, with no mipmapping. Returns 1.
 */
s32 Gs_SetTexFilter(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *pktCursor;

    pkt = pktCursor = Packet_Alloc(AD_PACKET_SIZE(1), 16);
    AD_PACKET_HEAD(pktCursor, 2, VIF_DIRECT);
    pktCursor += 4;
    if ((flags & RF_TEX_NEAREST) == RF_TEX_NEAREST) {
        pktCursor[0] = 0;
    } else {
        pktCursor[0] = TEX1_MMAG_LINEAR | TEX1_MMIN(TEX1_MIN_LINEAR);
    }
    if (ctx == GS_CTX_1) {
        pktCursor[1] = GS_REG_TEX1_1;
    } else {
        pktCursor[1] = GS_REG_TEX1_2;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}
