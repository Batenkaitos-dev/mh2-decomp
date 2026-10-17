/*
 * main 0x185460-0x1864e0: the render state's GS registers. Each setter
 * builds a small GIF packet (DMA END tag, VIF FLUSHA + DIRECT, a GIF tag
 * in A+D mode) with the register for drawing context 1, 2 or both, and
 * queues it. The texture registers and the coordinate conversions that
 * followed are another unit (unk_1864E0.c, built at level 4).
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

s32 Gs_SetScissor(s32 x, s32 y, s32 w, s32 h, s32 ctx)
{
    s32 x1;
    s32 y1;
    u64 *pkt;
    u64 *p;
    u32 n;
    u64 v;

    if (x < 0) {
        x = 0;
    }
    if (x + w > gDispWidth) {
        w = gDispWidth - x;
    }
    x1 = x + w - 1;
    if (y < 0) {
        y = 0;
    }
    if (y + h > gDispHeight) {
        h = gDispHeight - y;
    }
    y1 = y + h - 1;
    if (ctx != 2) {
        p = Packet_Alloc(0x30, 16);
        n = 2;
    } else {
        p = Packet_Alloc(0x40, 16);
        n = 3;
    }
    pkt = p;
    p[0] = (n + 0x70000000) | 0x80000000;
    ((u32 *)p)[2] = 0x13000000;
    ((u32 *)p)[3] = n | 0x50000000;
    p[2] = (u64)(n - 1) | 0x8000 | ((u64)1 << 60);
    p[3] = 0xE;
    p += 4;
    switch (ctx) {
    case 0:
        gScissor1X0 = x;
        gScissor1Y0 = y;
        gScissor1X1 = x1;
        gScissor1Y1 = y1;
        gGsScissor1 = (u64)x | ((u64)x1 << 16) | ((u64)y << 32) | ((u64)y1 << 48);
        p[0] = gGsScissor1;
        p[1] = GS_REG_SCISSOR_1;
        break;
    case 1:
        gScissor2X0 = x;
        gScissor2Y0 = y;
        gScissor2X1 = x1;
        gScissor2Y1 = y1;
        gGsScissor2 = (u64)x | ((u64)x1 << 16) | ((u64)y << 32) | ((u64)y1 << 48);
        p[0] = gGsScissor2;
        p[1] = GS_REG_SCISSOR_2;
        break;
    case 2:
        gScissor1X0 = x;
        gScissor1Y0 = y;
        gScissor1X1 = x1;
        gScissor1Y1 = y1;
        gScissor2X0 = x;
        gScissor2Y0 = y;
        gScissor2X1 = x1;
        gScissor2Y1 = y1;
        v = (u64)x | ((u64)x1 << 16) | ((u64)y << 32) | ((u64)y1 << 48);
        gGsScissor1 = v;
        gGsScissor2 = v;
        p[0] = v;
        p[1] = GS_REG_SCISSOR_1;
        p[2] = v;
        p[3] = GS_REG_SCISSOR_2;
        break;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

s32 Gs_SetZbuf(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *p;
    u32 n;
    u64 zmsk;
    u64 zbuf;

    zmsk = (s32)(flags & 0x8000) != 0x8000;
    zbuf = (u64)(s32)(gSysZBlock >> 5) | ((u64)gSysZPsm << 24) | (zmsk << 32);
    if (ctx != 2) {
        p = Packet_Alloc(0x30, 16);
        n = 2;
    } else {
        p = Packet_Alloc(0x40, 16);
        n = 3;
    }
    pkt = p;
    p[0] = (n + 0x70000000) | 0x80000000;
    ((u32 *)p)[2] = 0x13000000;
    ((u32 *)p)[3] = n | 0x50000000;
    p[2] = (u64)(n - 1) | 0x8000 | ((u64)1 << 60);
    p[3] = 0xE;
    p += 4;
    switch (ctx) {
    case 0:
        gGsZbuf1 = zbuf;
        p[0] = zbuf;
        p[1] = GS_REG_ZBUF_1;
        break;
    case 1:
        gGsZbuf2 = zbuf;
        p[0] = zbuf;
        p[1] = GS_REG_ZBUF_2;
        break;
    case 2:
        gGsZbuf1 = zbuf;
        gGsZbuf2 = zbuf;
        p[0] = zbuf;
        p[1] = GS_REG_ZBUF_1;
        p[2] = zbuf;
        p[3] = GS_REG_ZBUF_2;
        break;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

s32 Gs_SetTest(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *p;
    u32 n;
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
        p = Packet_Alloc(0x30, 16);
        n = 2;
    } else {
        p = Packet_Alloc(0x40, 16);
        n = 3;
    }
    pkt = p;
    p[0] = (n + 0x70000000) | 0x80000000;
    ((u32 *)p)[2] = 0x13000000;
    ((u32 *)p)[3] = n | 0x50000000;
    p[2] = (u64)(n - 1) | 0x8000 | ((u64)1 << 60);
    p[3] = 0xE;
    p += 4;
    switch (ctx) {
    case 0:
        gGsTest1 = test;
        p[0] = test;
        p[1] = GS_REG_TEST_1;
        break;
    case 1:
        gGsTest2 = test;
        p[0] = test;
        p[1] = GS_REG_TEST_2;
        break;
    case 2:
        gGsTest1 = test;
        gGsTest2 = test;
        p[0] = test;
        p[1] = GS_REG_TEST_1;
        p[2] = test;
        p[3] = GS_REG_TEST_2;
        break;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

#define ALPHA(a, b, c, d, fix) \
    ((u64)(a) | ((u64)(b) << 2) | ((u64)(c) << 4) | ((u64)(d) << 6) | ((u64)(fix) << 32))

s32 Gs_SetAlpha(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *p;
    u32 n;
    u32 mode;
    u64 alpha;

    if (ctx != 2) {
        p = Packet_Alloc(0x30, 16);
        n = 2;
    } else {
        p = Packet_Alloc(0x40, 16);
        n = 3;
    }
    pkt = p;
    p[0] = (n + 0x70000000) | 0x80000000;
    ((u32 *)p)[2] = 0x13000000;
    ((u32 *)p)[3] = n | 0x50000000;
    p[2] = (u64)(n - 1) | 0x8000 | ((u64)1 << 60);
    p[3] = 0xE;
    p += 4;
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
        p[0] = alpha;
        p[1] = GS_REG_ALPHA_1;
        break;
    case 1:
        p[0] = alpha;
        p[1] = GS_REG_ALPHA_2;
        break;
    case 2:
        p[0] = alpha;
        p[1] = GS_REG_ALPHA_1;
        p[2] = alpha;
        p[3] = GS_REG_ALPHA_2;
        break;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

s32 Gs_SetFogColor(u32 color)
{
    u64 *pkt;
    u64 *p;

    pkt = p = Packet_Alloc(0x30, 16);
    p[0] = (2 + 0x70000000) | 0x80000000;
    ((u32 *)p)[2] = 0x13000000;
    ((u32 *)p)[3] = 2 | 0x51000000;
    p[2] = (u64)(2 - 1) | 0x8000 | ((u64)1 << 60);
    p[3] = 0xE;
    p += 4;
    p[0] = (u64)((color >> 16) & 0xFF) | ((u64)((color >> 8) & 0xFF) << 8) |
           ((u64)(color & 0xFF) << 16);
    p[1] = GS_REG_FOGCOL;
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}

s32 Gs_SetTexFilter(u32 flags, s32 ctx)
{
    u64 *pkt;
    u64 *p;

    pkt = p = Packet_Alloc(0x30, 16);
    p[0] = (2 + 0x70000000) | 0x80000000;
    ((u32 *)p)[2] = 0x13000000;
    ((u32 *)p)[3] = 2 | 0x50000000;
    p[2] = (u64)(2 - 1) | 0x8000 | ((u64)1 << 60);
    p[3] = 0xE;
    p += 4;
    if ((flags & 0x10000) == 0x10000) {
        p[0] = 0;
    } else {
        p[0] = 0x60;
    }
    if (ctx == 0) {
        p[1] = GS_REG_TEX1_1;
    } else {
        p[1] = GS_REG_TEX1_2;
    }
    DmaQueue_AddChain(0, (u32)pkt & 0x0FFFFFFF, (u32 *)pkt, &gDmaQueue);
    return 1;
}
