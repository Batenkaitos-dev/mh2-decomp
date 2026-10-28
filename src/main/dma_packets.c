/*
 * Writing DMA packets: the DMA tags (CNT, NEXT, REF, REFE, CALL, RET,
 * END), the VIF codes the packets use (Vif_MsCnt, Vif_UnpackV4,
 * Vif_DirectHL), and the chains that upload an image to GS memory, with
 * their sizes. Each function writes at the address it is given and
 * returns; the DMA queue sends what they build.
 */
#include "common.h"
#include "dma.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

/*
 * Writes a CNT tag (qwc quadwords of data follow the tag) at tag and
 * returns the quadword after it. irq 1 sets the IRQ bit of the tag;
 * unused is ignored (every caller passes 0).
 */
u128 *DmaTag_Cnt(u128 *tag, s32 qwc, s32 irq, s32 unused)
{
    u32 *words = (u32 *)tag;

    *tag = 0;
    words[0] = qwc + 0x10000000;
    if (irq == 1) {
        words[0] |= 0x80000000;
    }
    return tag + 1;
}

/*
 * Writes a NEXT tag at tag: qwc quadwords follow it, then the chain goes on
 * at addr. An addr in scratchpad memory (top nibble 7) gets the SPR bit.
 * irq 1 sets the IRQ bit; unused is ignored. Returns the next quadword.
 */
u128 *DmaTag_Next(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused)
{
    u32 *words = (u32 *)tag;
    u32 physAddr = DMA_TAG_ADDR(addr);
    u32 scratchpadBit = 0;

    if (DMA_ADDR_IS_SPR(addr)) {
        scratchpadBit = 0x80000000;
    }
    *tag = 0;
    words[0] = qwc + 0x20000000;
    words[1] = physAddr | scratchpadBit;
    if (irq == 1) {
        words[0] |= 0x80000000;
    }
    return tag + 1;
}

/*
 * Writes a REF tag at tag: qwc quadwords are sent from addr, then the chain
 * goes on after the tag. An addr in scratchpad memory (top nibble 7) gets
 * the SPR bit. irq 1 sets the IRQ bit; unused is ignored. Returns the next
 * quadword.
 */
u128 *DmaTag_Ref(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused)
{
    u32 *words = (u32 *)tag;
    u32 physAddr = DMA_TAG_ADDR(addr);
    u32 scratchpadBit = 0;

    if (DMA_ADDR_IS_SPR(addr)) {
        scratchpadBit = 0x80000000;
    }
    *tag = 0;
    words[0] = qwc + 0x30000000;
    words[1] = physAddr | scratchpadBit;
    if (irq == 1) {
        words[0] |= 0x80000000;
    }
    return tag + 1;
}

/*
 * Writes a REFE tag at tag: qwc quadwords are sent from addr and the chain
 * ends. An addr in scratchpad memory (top nibble 7) gets the SPR bit.
 * irq 1 sets the IRQ bit; unused is ignored. Returns the next quadword.
 */
u128 *DmaTag_RefE(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused)
{
    u32 *words = (u32 *)tag;
    u32 physAddr = DMA_TAG_ADDR(addr);
    u32 scratchpadBit = 0;

    if (DMA_ADDR_IS_SPR(addr)) {
        scratchpadBit = 0x80000000;
    }
    *tag = 0;
    words[0] = qwc;
    words[1] = physAddr | scratchpadBit;
    if (irq == 1) {
        words[0] |= 0x80000000;
    }
    return tag + 1;
}

/*
 * Writes a CALL tag at tag: qwc quadwords follow it, then the chain runs
 * the one at addr until its RET and comes back after this data. An addr in
 * scratchpad memory (top nibble 7) gets the SPR bit. irq 1 sets the IRQ
 * bit; unused is ignored. Returns the next quadword.
 */
u128 *DmaTag_Call(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused)
{
    u32 *words = (u32 *)tag;
    u32 physAddr = DMA_TAG_ADDR(addr);
    u32 scratchpadBit = 0;

    if (DMA_ADDR_IS_SPR(addr)) {
        scratchpadBit = 0x80000000;
    }
    *tag = 0;
    words[0] = qwc + 0x50000000;
    words[1] = physAddr | scratchpadBit;
    if (irq == 1) {
        words[0] |= 0x80000000;
    }
    return tag + 1;
}

/*
 * Writes a RET tag at tag: qwc quadwords follow it, then the chain returns
 * to the CALL that ran it. irq 1 sets the IRQ bit; unused is ignored.
 * Returns the next quadword.
 */
u128 *DmaTag_Ret(u128 *tag, s32 qwc, s32 irq, s32 unused)
{
    u32 *words = (u32 *)tag;

    *tag = 0;
    words[0] = qwc + 0x60000000;
    if (irq == 1) {
        words[0] |= 0x80000000;
    }
    return tag + 1;
}

/*
 * Writes an END tag at tag: qwc quadwords follow it, then the chain ends.
 * irq 1 sets the IRQ bit; unused is ignored. Returns the next quadword.
 */
u128 *DmaTag_End(u128 *tag, s32 qwc, s32 irq, s32 unused)
{
    u32 *words = (u32 *)tag;

    *tag = 0;
    words[0] = qwc + 0x70000000;
    if (irq == 1) {
        words[0] |= 0x80000000;
    }
    return tag + 1;
}

/*
 * Writes a VIF MSCNT (run the VU1 microprogram from where it stopped) and
 * three NOPs at pos; returns the next quadword.
 */
u128 *Vif_MsCnt(u128 *pos)
{
    u32 *words = (u32 *)pos;

    words[0] = 0x17000000;
    words[1] = 0;
    words[2] = 0;
    words[3] = 0;
    return pos + 1;
}

/*
 * Writes STCYCL 4,4 and an UNPACK V4-32 of qwc quadwords to VU memory
 * address addr (relative to TOPS) at pos; returns the next quadword.
 */
u128 *Vif_UnpackV4(u128 *pos, s32 qwc, s32 addr)
{
    u32 *words = (u32 *)pos;

    words[0] = 0;
    words[1] = 0;
    words[2] = 0x01000404;
    words[3] = (addr | 0x8000) | (qwc << 16) | 0x6C000000;
    return pos + 1;
}

/*
 * Writes three NOPs and a DIRECTHL of qwc quadwords (passed straight to the
 * GIF) at pos; returns the next quadword.
 */
u128 *Vif_DirectHL(u128 *pos, s32 qwc)
{
    u32 *words = (u32 *)pos;

    words[0] = 0;
    words[1] = 0;
    words[2] = 0;
    words[3] = qwc | 0x51000000;
    return pos + 1;
}

/* Bytes of image-transfer packet for an image of size bytes, sent in chunks
 * of 0x70000 bytes (128 each). */
s32 Dma_ImagePacketSize(u32 size)
{
    u32 chunkCount = size / 0x70000;

    if (size % 0x70000) {
        chunkCount++;
    }
    return chunkCount << 7;
}

/*
 * Bytes of the packet that ends a texture upload (Dma_BuildEndPacket):
 * 64, whatever the size passed.
 */
s32 Dma_EndPacketSize(u32 unused0)
{
    return 64;
}

/*
 * Builds at pkt the DMA chain that uploads an image of size bytes at src to
 * GS memory: BITBLTBUF (dbp, dbw, psm), TRXPOS (dstX, dstY), TRXREG
 * (width x rows) and TRXDIR, then a REF of the pixels, in chunks of at most
 * 0x70000 bytes (a whole number of rows each). The last chunk goes by a
 * REFE whose IRQ bit is irq. Returns that REFE tag, the last tag of the
 * chain. irq is u32 (as s32, the stack copy of it the original keeps is
 * missing).
 */
u128 *Dma_BuildLoadImage(u128 *pkt, u32 irq, u32 src, u32 size, s16 dbp, s16 dbw, s16 psm, s16 dstX,
                         s16 dstY, s16 width, s16 height)
{
    u32 chunkCount;
    u64 *words;
    u128 *last;
    s32 bytes;
    u32 k;
    u32 qwc;
    s16 rows;

    chunkCount = size / 0x70000;
    last = NULL;
    if (size % 0x70000) {
        chunkCount++;
    }
    if (chunkCount == 1) {
        rows = height;
    } else {
        switch (psm) {
        case 0:
        case 1:
            rows = 0x70000 / (width * 4);
            break;
        case 2:
        case 10:
            rows = 0x70000 / (width * 2);
            break;
        case 19:
            rows = 0x70000 / width;
            break;
        case 20:
            rows = 0x70000 / (width >> 1);
            break;
        }
    }
    if (dbw == 0) {
        dbw = 1;
    }
    words = (u64 *)pkt;
    for (k = 0; k < chunkCount; k++) {
        switch (psm) {
        case 0:
        case 1:
            bytes = width * rows * 4;
            break;
        case 2:
        case 10:
            bytes = width * rows * 2;
            break;
        case 19:
            bytes = width * rows;
            break;
        case 20:
            bytes = width * rows / 2;
            break;
        }
        DmaTag_Cnt((u128 *)words, 6, 0, 0);
        ((u32 *)words)[2] = 0x13000000;
        ((u32 *)words)[3] = 0x51000006;
        words[2] = 0x1000000000000004;
        words[3] = 14;
        words[4] = (((u64)dbp << 32) | ((u64)dbw << 48)) | ((u64)psm << 56);
        words[5] = 80;
        words[6] = ((u64)dstX << 32) | ((u64)dstY << 48);
        words[7] = 81;
        words[8] = width | ((u64)rows << 32);
        words[9] = 82;
        words[10] = 0;
        words[11] = 83;
        if (k >= chunkCount - 1) {
            qwc = size >> 4;
            words[12] = (u64)qwc | 0x0800000000008000;
            last = (u128 *)(words + 14);
            words[13] = 0;
            DmaTag_RefE(last, qwc, src, irq, 0);
            ((u32 *)words)[30] = 0;
            ((u32 *)words)[31] = qwc | 0x51000000;
        } else {
            words[12] = 0x080000000000F000;
            words[13] = 0;
            DmaTag_Ref((u128 *)(words + 14), 28672, src, 0, 0);
            ((u32 *)words)[30] = 0;
            ((u32 *)words)[31] = 0x51007000;
            src += bytes;
            dstY += rows;
            words += 16;
            if (height < dstY + rows) {
                rows = height - dstY;
            }
            size -= bytes;
        }
    }
    return last;
}

/* An END tag (qwc 3) and a GIF packet whose one A+D write is 0 to GS_REG_TEXFLUSH.
 * The tags are written through the u64 view (`(u128 *)p`, which orders the
 * argument set-up as the original). */
void Dma_BuildEndPacket(u128 *pkt, s32 irq)
{
    u64 *words = (u64 *)pkt;

    DmaTag_End((u128 *)words, 3, irq, 0);
    Vif_DirectHL((u128 *)words + 1, 2);
    words[4] = 0x1000000000008001;
    words[5] = 14;
    words[6] = 0;
    words[7] = 63;
}
