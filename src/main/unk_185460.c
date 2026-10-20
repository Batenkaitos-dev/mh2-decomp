/*
 * The render state's GS registers. Each setter
 * builds a small GIF packet (DMA END tag, VIF FLUSHA + DIRECT, a GIF tag
 * in A+D mode) with the register for drawing context 1, 2 or both, and
 * queues it. The texture registers and the coordinate conversions that
 * followed are another unit (built at level 4).
 */
#include "common.h"
#include "dma.h"
#include "texture.h"
#include "hw_regs.h"

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

void *Packet_Alloc(u32 size, s32 align);
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *q);

/* The render state's unit was built without copy propagation: a global is
 * reloaded after each store, and a packet pointer's step is not folded into
 * the offsets of the stores that follow it. */
#pragma opt_propagation off

/*
 * Sets the scissor rectangle at x, y, width by height, clipped to the
 * display (gDispWidth, gDispHeight), for drawing context 1 (ctx 0), 2
 * (ctx 1) or both (ctx 2). Keeps the corners in gScissor1X0.. and
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
    if (ctx != 2) {
        pktCursor = Packet_Alloc(0x30, 16);
        qwCount = 2;
    } else {
        pktCursor = Packet_Alloc(0x40, 16);
        qwCount = 3;
    }
    pkt = pktCursor;
    pktCursor[0] = (qwCount + 0x70000000) | 0x80000000;
    ((u32 *)pktCursor)[2] = 0x13000000;
    ((u32 *)pktCursor)[3] = qwCount | 0x50000000;
    pktCursor[2] = (u64)(qwCount - 1) | 0x8000 | ((u64)1 << 60);
    pktCursor[3] = 0xE;
    pktCursor += 4;
    switch (ctx) {
    case 0:
        gScissor1X0 = x;
        gScissor1Y0 = y;
        gScissor1X1 = lastX;
        gScissor1Y1 = lastY;
        gGsScissor1 = (u64)x | ((u64)lastX << 16) | ((u64)y << 32) | ((u64)lastY << 48);
        pktCursor[0] = gGsScissor1;
        pktCursor[1] = GS_REG_SCISSOR_1;
        break;
    case 1:
        gScissor2X0 = x;
        gScissor2Y0 = y;
        gScissor2X1 = lastX;
        gScissor2Y1 = lastY;
        gGsScissor2 = (u64)x | ((u64)lastX << 16) | ((u64)y << 32) | ((u64)lastY << 48);
        pktCursor[0] = gGsScissor2;
        pktCursor[1] = GS_REG_SCISSOR_2;
        break;
    case 2:
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
 * Sets ZBUF for context 1 (ctx 0), 2 (ctx 1) or both (ctx 2): the Z buffer
 * at gSysZBlock in format gSysZPsm, with Z writes masked unless flags has
 * 0x8000. Keeps it in gGsZbuf1/2 and queues the packet. Returns 1.
 */
s32 Gs_SetZbuf(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *pktCursor;
    u32 qwCount;
    u64 zmsk;
    u64 zbuf;

    zmsk = (s32)(flags & 0x8000) != 0x8000;
    zbuf = (u64)(s32)(gSysZBlock >> 5) | ((u64)gSysZPsm << 24) | (zmsk << 32);
    if (ctx != 2) {
        pktCursor = Packet_Alloc(0x30, 16);
        qwCount = 2;
    } else {
        pktCursor = Packet_Alloc(0x40, 16);
        qwCount = 3;
    }
    pkt = pktCursor;
    pktCursor[0] = (qwCount + 0x70000000) | 0x80000000;
    ((u32 *)pktCursor)[2] = 0x13000000;
    ((u32 *)pktCursor)[3] = qwCount | 0x50000000;
    pktCursor[2] = (u64)(qwCount - 1) | 0x8000 | ((u64)1 << 60);
    pktCursor[3] = 0xE;
    pktCursor += 4;
    switch (ctx) {
    case 0:
        gGsZbuf1 = zbuf;
        pktCursor[0] = zbuf;
        pktCursor[1] = GS_REG_ZBUF_1;
        break;
    case 1:
        gGsZbuf2 = zbuf;
        pktCursor[0] = zbuf;
        pktCursor[1] = GS_REG_ZBUF_2;
        break;
    case 2:
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
 * Sets TEST for context 1 (ctx 0), 2 (ctx 1) or both (ctx 2), kept in
 * gGsTest1/2. The alpha test is always on, its method from bits 19-21
 * of flags and its reference gGsAlphaRef (0-255, scaled to the GS's
 * 0-0x80). The depth test is always on: flags & 0x7000 of 0 never
 * passes, 0x1000 greater, 0x3000 greater or equal, anything else always.
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
    if (aref == 0xFF) {
        aref = 0x80;
    } else if (aref != 0) {
        aref >>= 1;
        if (aref == 0) {
            aref = 1;
        }
    }
    switch (flags & 0x380000) {
    case 0:
        atst = 0;
        break;
    case 0x380000:
        atst = 1;
        break;
    case 0x80000:
        atst = 2;
        break;
    case 0x180000:
        atst = 3;
        break;
    case 0x100000:
        atst = 4;
        break;
    case 0x300000:
        atst = 5;
        break;
    case 0x200000:
        atst = 6;
        break;
    case 0x280000:
        atst = 7;
        break;
    }
    switch (flags & 0x7000) {
    case 0x1000:
        ztst = 3;
        break;
    case 0x3000:
        ztst = 2;
        break;
    case 0x7000:
        ztst = 1;
        break;
    case 0:
        ztst = 0;
        break;
    case 0x2000:
    case 0x5000:
    case 0x4000:
    case 0x6000:
        ztst = 1;
        break;
    }
    test = ((u64)aref << 4) | (1 | ((u64)atst << 1)) | 0x10000 | ((u64)ztst << 17);
    if (ctx != 2) {
        pktCursor = Packet_Alloc(0x30, 16);
        qwCount = 2;
    } else {
        pktCursor = Packet_Alloc(0x40, 16);
        qwCount = 3;
    }
    pkt = pktCursor;
    pktCursor[0] = (qwCount + 0x70000000) | 0x80000000;
    ((u32 *)pktCursor)[2] = 0x13000000;
    ((u32 *)pktCursor)[3] = qwCount | 0x50000000;
    pktCursor[2] = (u64)(qwCount - 1) | 0x8000 | ((u64)1 << 60);
    pktCursor[3] = 0xE;
    pktCursor += 4;
    switch (ctx) {
    case 0:
        gGsTest1 = test;
        pktCursor[0] = test;
        pktCursor[1] = GS_REG_TEST_1;
        break;
    case 1:
        gGsTest2 = test;
        pktCursor[0] = test;
        pktCursor[1] = GS_REG_TEST_2;
        break;
    case 2:
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

#define ALPHA(a, b, c, d, fix) \
    ((u64)(a) | ((u64)(b) << 2) | ((u64)(c) << 4) | ((u64)(d) << 6) | ((u64)(fix) << 32))

/*
 * Sets the ALPHA blend equation for context 1 (ctx 0), 2 (ctx 1) or both
 * (ctx 2). flags & 0xC00 picks the family, flags & 0xF and flags & 0xF0
 * the equation; without flag 0x1000000 blending is off (the source colour
 * as is). Keeps nothing in globals. Returns 0 for a combination with no
 * equation (nothing queued), else 1.
 */
s32 Gs_SetAlpha(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *pktCursor;
    u32 qwCount;
    u32 mode;
    u64 alpha;

    if (ctx != 2) {
        pktCursor = Packet_Alloc(0x30, 16);
        qwCount = 2;
    } else {
        pktCursor = Packet_Alloc(0x40, 16);
        qwCount = 3;
    }
    pkt = pktCursor;
    pktCursor[0] = (qwCount + 0x70000000) | 0x80000000;
    ((u32 *)pktCursor)[2] = 0x13000000;
    ((u32 *)pktCursor)[3] = qwCount | 0x50000000;
    pktCursor[2] = (u64)(qwCount - 1) | 0x8000 | ((u64)1 << 60);
    pktCursor[3] = 0xE;
    pktCursor += 4;
    mode = flags & 0xC00;
    if (mode == 0) {
        switch (flags & 0xF) {
        case 1:
            switch (flags & 0xF0) {
            case 0x10:
                alpha = ALPHA(0, 2, 2, 1, 0x80);
                break;
            case 0:
                alpha = ALPHA(0, 2, 2, 2, 0x80);
                break;
            case 0x20:
                alpha = ALPHA(1, 2, 0, 0, 0);
                break;
            case 0x30:
                alpha = ALPHA(2, 1, 0, 0, 0);
                break;
            case 0x40:
                alpha = ALPHA(1, 2, 1, 0, 0);
                break;
            case 0x50:
                alpha = ALPHA(2, 1, 1, 0, 0);
                break;
            default:
                return 0;
            }
            break;
        case 0:
            switch (flags & 0xF0) {
            case 0x10:
                alpha = ALPHA(2, 2, 2, 1, 0x80);
                break;
            case 0:
                alpha = ALPHA(2, 2, 2, 2, 0x80);
                break;
            case 0x20:
                alpha = ALPHA(1, 2, 0, 2, 0);
                break;
            case 0x30:
                alpha = ALPHA(2, 1, 0, 1, 0);
                break;
            case 0x40:
                alpha = ALPHA(1, 2, 1, 2, 0);
                break;
            case 0x50:
                alpha = ALPHA(2, 1, 1, 1, 0);
                break;
            default:
                return 0;
            }
            break;
        case 2:
            switch (flags & 0xF0) {
            case 0x10:
                alpha = ALPHA(0, 2, 0, 1, 0);
                break;
            case 0:
                alpha = ALPHA(0, 2, 0, 2, 0);
                break;
            case 0x30:
                alpha = ALPHA(0, 1, 0, 1, 0);
                break;
            default:
                return 0;
            }
            break;
        case 3:
            switch (flags & 0xF0) {
            case 0:
                alpha = ALPHA(2, 0, 0, 0, 0);
                break;
            case 0x20:
                alpha = ALPHA(1, 0, 0, 0, 0);
                break;
            default:
                return 0;
            }
            break;
        case 4:
            switch (flags & 0xF0) {
            case 0x10:
                alpha = ALPHA(0, 2, 1, 1, 0);
                break;
            case 0:
                alpha = ALPHA(0, 2, 1, 2, 0);
                break;
            case 0x50:
                alpha = ALPHA(0, 1, 1, 1, 0);
                break;
            default:
                return 0;
            }
            break;
        case 5:
            switch (flags & 0xF0) {
            case 0:
                alpha = ALPHA(2, 0, 1, 0, 0);
                break;
            case 0x40:
                alpha = ALPHA(1, 0, 1, 0, 0);
                break;
            default:
                return 0;
            }
            break;
        case 8:
            switch (flags & 0xF0) {
            case 0x20:
                alpha = ALPHA(1, 2, 0, 1, 0);
                break;
            default:
                return 0;
            }
            break;
        default:
            return 0;
        }
    } else if (mode == 0x400) {
        switch (flags & 0xF) {
        case 1:
            switch (flags & 0xF0) {
            case 0x10:
                alpha = ALPHA(0, 1, 2, 2, 0x80);
                break;
            case 0:
                alpha = ALPHA(2, 2, 2, 0, 0x80);
                break;
            default:
                return 0;
            }
            break;
        case 0:
            switch (flags & 0xF0) {
            case 0x10:
                alpha = ALPHA(2, 1, 2, 2, 0x80);
                break;
            case 0:
                alpha = ALPHA(2, 2, 2, 2, 0x80);
                break;
            case 0x20:
                alpha = ALPHA(2, 1, 0, 2, 0);
                break;
            case 0x40:
                alpha = ALPHA(2, 1, 1, 2, 0);
                break;
            default:
                return 0;
            }
            break;
        case 2:
            switch (flags & 0xF0) {
            case 0:
                alpha = ALPHA(0, 2, 0, 2, 0);
                break;
            case 0x20:
                alpha = ALPHA(0, 1, 0, 2, 0);
                break;
            default:
                return 0;
            }
            break;
        case 3:
            switch (flags & 0xF0) {
            case 0:
                alpha = ALPHA(2, 0, 0, 0, 0);
                break;
            default:
                return 0;
            }
            break;
        case 4:
            switch (flags & 0xF0) {
            case 0:
                alpha = ALPHA(0, 2, 1, 2, 0);
                break;
            case 0x40:
                alpha = ALPHA(0, 1, 1, 2, 0);
                break;
            default:
                return 0;
            }
            break;
        case 5:
            switch (flags & 0xF0) {
            case 0:
                alpha = ALPHA(2, 1, 1, 1, 0);
                break;
            default:
                return 0;
            }
            break;
        default:
            return 0;
        }
    } else if (mode == 0x800) {
        switch (flags & 0xF) {
        case 1:
            switch (flags & 0xF0) {
            case 0x10:
                alpha = ALPHA(1, 0, 2, 2, 0x80);
                break;
            case 0:
                alpha = ALPHA(2, 0, 2, 2, 0x80);
                break;
            default:
                return 0;
            }
            break;
        case 0:
            switch (flags & 0xF0) {
            case 0x10:
                alpha = ALPHA(1, 2, 2, 2, 0x80);
                break;
            case 0:
                alpha = ALPHA(2, 2, 2, 2, 0x80);
                break;
            case 0x20:
                alpha = ALPHA(1, 2, 0, 2, 0);
                break;
            case 0x30:
                alpha = ALPHA(2, 1, 0, 1, 0);
                break;
            case 0x40:
                alpha = ALPHA(1, 2, 1, 2, 0);
                break;
            case 0x50:
                alpha = ALPHA(2, 1, 1, 1, 0);
                break;
            default:
                return 0;
            }
            break;
        case 2:
            switch (flags & 0xF0) {
            case 0:
                alpha = ALPHA(2, 0, 0, 2, 0);
                break;
            case 0x20:
                alpha = ALPHA(1, 0, 0, 2, 0);
                break;
            default:
                return 0;
            }
            break;
        case 4:
            switch (flags & 0xF0) {
            case 0:
                alpha = ALPHA(2, 0, 1, 2, 0);
                break;
            case 0x40:
                alpha = ALPHA(1, 0, 1, 2, 0);
                break;
            default:
                return 0;
            }
            break;
        default:
            return 0;
        }
    }
    if (!(flags & 0x1000000)) {
        alpha = ALPHA(0, 2, 2, 2, 0x80);
    }
    switch (ctx) {
    case 0:
        pktCursor[0] = alpha;
        pktCursor[1] = GS_REG_ALPHA_1;
        break;
    case 1:
        pktCursor[0] = alpha;
        pktCursor[1] = GS_REG_ALPHA_2;
        break;
    case 2:
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

    pkt = pktCursor = Packet_Alloc(0x30, 16);
    pktCursor[0] = (2 + 0x70000000) | 0x80000000;
    ((u32 *)pktCursor)[2] = 0x13000000;
    ((u32 *)pktCursor)[3] = 2 | 0x51000000;
    pktCursor[2] = (u64)(2 - 1) | 0x8000 | ((u64)1 << 60);
    pktCursor[3] = 0xE;
    pktCursor += 4;
    pktCursor[0] = (u64)((color >> 16) & 0xFF) | ((u64)((color >> 8) & 0xFF) << 8) |
           ((u64)(color & 0xFF) << 16);
    pktCursor[1] = GS_REG_FOGCOL;
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

/*
 * Sets TEX1 for context 1 (ctx 0) or 2 (any other ctx): point sampling
 * when flags has 0x10000, else bilinear, with no mipmapping. Returns 1.
 */
s32 Gs_SetTexFilter(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *pktCursor;

    pkt = pktCursor = Packet_Alloc(0x30, 16);
    pktCursor[0] = (2 + 0x70000000) | 0x80000000;
    ((u32 *)pktCursor)[2] = 0x13000000;
    ((u32 *)pktCursor)[3] = 2 | 0x50000000;
    pktCursor[2] = (u64)(2 - 1) | 0x8000 | ((u64)1 << 60);
    pktCursor[3] = 0xE;
    pktCursor += 4;
    if ((flags & 0x10000) == 0x10000) {
        pktCursor[0] = 0;
    } else {
        pktCursor[0] = 0x60;
    }
    if (ctx == 0) {
        pktCursor[1] = GS_REG_TEX1_1;
    } else {
        pktCursor[1] = GS_REG_TEX1_2;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}
