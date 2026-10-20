/*
 * DMA tags, image uploads, the DMA queue and handlers
 */
#include "common.h"
#include "dma.h"
#include "hw_regs.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

extern DmaQueue gDmaQueue;
extern s32 gPacketBufIdx;
extern s32 gDmaGsIntcId;
extern u32 gDmaRecvDst;
extern u32 gDmaRecvQwc;
extern u64 gDmaSavedImr;
extern u128 D_001BA110;
extern u32 *gDmaChcrRegs[10];

void *Mem_Alloc(u32 size, s32 flag);
void *Mem_GetAddr(void *handle);
void *Packet_Alloc(u32 size, s32 align);
s32 AddDmacHandler(s32 ch, s32 (*handler)(s32), s32 next);
s32 AddIntcHandler(s32 cause, s32 (*handler)(s32), s32 next);
s32 EnableDmac(s32 ch);
s32 EnableIntc(s32 cause);
s32 iEnableIntc(s32 cause);
s32 iDisableIntc(s32 cause);
s32 sceDmaRecvN(DmaChan *chan, void *addr, s32 size);
s32 sceDmaSend(DmaChan *chan, void *tag);
s32 sceDmaSync(DmaChan *chan, s32 mode, s32 timeout);
s32 sceGsSyncPath(s32 mode, u16 timeout);
void FlushCache(s32 op);
s32 Dma_GsFinishHandler(s32 cause);
s32 DmaQueue_Add(u32 tag, DmaQueue *q);
s32 Dma_WaitIdle(void);
s32 Dma_StoreImagePacketSize(u32 size);

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
    u32 physAddr = addr & 0x0FFFFFFF;
    u32 scratchpadBit = 0;

    if ((addr & 0x70000000) == 0x70000000) {
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
    u32 physAddr = addr & 0x0FFFFFFF;
    u32 scratchpadBit = 0;

    if ((addr & 0x70000000) == 0x70000000) {
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
    u32 physAddr = addr & 0x0FFFFFFF;
    u32 scratchpadBit = 0;

    if ((addr & 0x70000000) == 0x70000000) {
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
    u32 physAddr = addr & 0x0FFFFFFF;
    u32 scratchpadBit = 0;

    if ((addr & 0x70000000) == 0x70000000) {
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
 * of 0x70000 bytes (0x80 each). */
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
 * 0x40, whatever the size passed.
 */
s32 Tex_UploadPacketSize(u32 unused0)
{
    return 0x40;
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
        words[3] = 0xE;
        words[4] = (((u64)dbp << 32) | ((u64)dbw << 48)) | ((u64)psm << 56);
        words[5] = 0x50;
        words[6] = ((u64)dstX << 32) | ((u64)dstY << 48);
        words[7] = 0x51;
        words[8] = width | ((u64)rows << 32);
        words[9] = 0x52;
        words[10] = 0;
        words[11] = 0x53;
        if (k >= chunkCount - 1) {
            qwc = size >> 4;
            words[12] = (u64)qwc | 0x0800000000008000;
            last = (u128 *)(words + 14);
            words[13] = 0;
            DmaTag_RefE(last, qwc, src, irq, 0);
            ((u32 *)words)[0x1E] = 0;
            ((u32 *)words)[0x1F] = qwc | 0x51000000;
        } else {
            words[12] = 0x080000000000F000;
            words[13] = 0;
            DmaTag_Ref((u128 *)(words + 14), 0x7000, src, 0, 0);
            ((u32 *)words)[0x1E] = 0;
            ((u32 *)words)[0x1F] = 0x51007000;
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

/* An END tag (qwc 3) and a GIF packet that writes nothing (A+D to 0x3F).
 * The tags are written through the u64 view (`(u128 *)p`, which orders the
 * argument set-up as the original). */
void Dma_BuildEndPacket(u128 *pkt, s32 irq)
{
    u64 *words = (u64 *)pkt;

    DmaTag_End((u128 *)words, 3, irq, 0);
    Vif_DirectHL((u128 *)words + 1, 2);
    words[4] = 0x1000000000008001;
    words[5] = 0xE;
    words[6] = 0;
    words[7] = 0x3F;
}

/*
 * Bytes of the packets Dma_QueueStoreImage builds for an image of size
 * bytes: 0x90 per chunk of at most 0x70000 bytes.
 */
s32 Dma_StoreImagePacketSize(u32 size)
{
    u32 chunkCount = size / 0x70000;

    if (size % 0x70000) {
        chunkCount++;
    }
    return chunkCount * 0x90;
}

/*
 * Queues the download of a GS memory rectangle (BITBLTBUF sbp, sbw, psm;
 * srcX, srcY; width x height) to dst, size bytes, in chunks of at most
 * 0x70000 bytes: each chunk is a queue entry of kind 2 whose packet ends
 * with its dst and quadword count, which Dma_GsFinishHandler reads. irq
 * sets the IRQ bit of each END tag.
 */
void Dma_QueueStoreImage(void *dst, u32 size, s16 sbp, s16 sbw, s16 psm, s16 srcX, s16 srcY, s16 width, s16 height,
                         u32 irq)
{
    u64 *words;
    u32 chunkCount;
    s32 bytes;
    u32 k;
    s16 rows;

    chunkCount = size / 0x70000;
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
    if (sbw == 0) {
        sbw = 1;
    }
    words = Packet_Alloc(Dma_StoreImagePacketSize(size), 16);
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
        DmaTag_End((u128 *)words, 7, irq, 0);
        ((u32 *)words)[2] = 0;
        ((u32 *)words)[3] = 0;
        ((u32 *)words)[4] = 0;
        ((u32 *)words)[5] = 0x06008000;
        ((u32 *)words)[6] = 0x13000000;
        ((u32 *)words)[7] = 0x50000006;
        words[4] = 0x1000000000008005;
        words[5] = 0xE;
        words[6] = sbp | ((u64)sbw << 16) | ((u64)psm << 24);
        words[7] = 0x50;
        words[8] = srcX | ((u64)srcY << 16);
        words[9] = 0x51;
        words[10] = width | ((u64)rows << 32);
        words[11] = 0x52;
        words[12] = 0;
        words[13] = 0x61;
        words[14] = 1;
        words[15] = 0x53;
        ((u32 *)words)[0x20] = (u32)dst;
        ((u32 *)words)[0x21] = (u32)bytes >> 4;
        ((u32 *)words)[0x22] = 0;
        ((u32 *)words)[0x23] = 0;
        DmaQueue_Add(((u32)words & 0x0FFFFFFF) | 0x20000000, &gDmaQueue);
        words += 18;
        dst = (u8 *)dst + bytes;
        srcY += rows;
        if (height < srcY + rows) {
            rows = height - srcY;
        }
    }
}

/*
 * Downloads a GS memory rectangle (BITBLTBUF sbp, sbw, psm; srcX, srcY;
 * width x height) to dst, size bytes, at once: waits for the DMA queue,
 * then sends each chunk of at most 0x70000 bytes and waits until
 * Dma_GsFinishHandler has received it. Tex_Lock uses it to read back a
 * texture that has no copy in main memory.
 */
void Dma_StoreImage(u32 dst, u32 size, s16 sbp, s16 sbw, s16 psm, s16 srcX, s16 srcY, s16 width, s16 height)
{
    u32 chunkCount;
    u32 k;
    DmaChan *chan;
    s16 rows;
    s32 bytes;
    u64 *words;

    Dma_WaitIdle();
    chunkCount = size / 0x70000;
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
    if (sbw == 0) {
        sbw = 1;
    }
    chan = gDmaChan[gDmaQueue.chan];
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
        words = Packet_Alloc(0x90, 16);
        DmaTag_End((u128 *)words, 7, 0, 0);
        ((u32 *)words)[2] = 0;
        ((u32 *)words)[3] = 0;
        ((u32 *)words)[4] = 0;
        ((u32 *)words)[5] = 0x06008000;
        ((u32 *)words)[6] = 0x13000000;
        ((u32 *)words)[7] = 0x50000006;
        words[4] = 0x1000000000008005;
        words[5] = 0xE;
        words[6] = sbp | ((u64)sbw << 16) | ((u64)psm << 24);
        words[7] = 0x50;
        words[8] = srcX | ((u64)srcY << 16);
        words[9] = 0x51;
        words[10] = width | ((u64)rows << 32);
        words[11] = 0x52;
        words[12] = 0;
        words[13] = 0x61;
        words[14] = 1;
        words[15] = 0x53;
        gDmaRecvQwc = (u32)bytes >> 4;
        gDmaRecvDst = dst;
        GS_CSR = 2;
        gDmaSavedImr = GS_IMR;
        GS_IMR = ~0x200LL;
        EnableIntc(0);
        chan->chcr.tte = 0;
        chan->chcr.tie = 0;
        FlushCache(0);
        sceDmaSend(chan, words);
        sceGsSyncPath(0, 0);
        while (gDmaRecvQwc != 0) {
        }
        sceGsSyncPath(0, 0);
        VIF1_STAT = 0;
        srcY += rows;
        GS_BUSDIR = 0;
        GS_IMR = gDmaSavedImr;
        VIF1_FIFO = D_001BA110;
        dst += bytes;
        if (height < srcY + rows) {
            rows = height - srcY;
        }
    }
}

/* GS interrupt handler: when a download's FINISH arrives, starts the VIF1
 * transfer that receives the data. */
s32 Dma_GsFinishHandler(s32 cause)
{
    DmaChan *chan;

    if (gDmaRecvQwc != 0) {
        chan = gDmaChan[1];
        VIF1_STAT = 0x800000;
        GS_BUSDIR = 1;
        iDisableIntc(0);
        chan->chcr.tte = 1;
        chan->chcr.tie = 0;
        sceDmaRecvN(chan, (void *)gDmaRecvDst, gDmaRecvQwc);
        gDmaRecvDst = 0;
        gDmaRecvQwc = 0;
    }
    __asm__ volatile ("sync.l\n\tei");
    return 0;
}

/*
 * Sets up the DMA queue: two entry buffers of capacity entries each (one
 * filled while the other is sent), handler as the DMAC handler of the
 * channel of the queue, which it enables, and Dma_GsFinishHandler as the
 * GS interrupt handler. bufSlot walks the fields of the two buffers (buf,
 * count, write, read) by stepping the queue pointer 4 bytes.
 */
void DmaQueue_Init(DmaQueue *queue, u32 capacity, s32 (*handler)(s32))
{
    s32 i;
    DmaQueue *bufSlot;

    queue->capacity = capacity;
    for (i = 0, bufSlot = queue; i < 2; i++, bufSlot = (DmaQueue *)((u8 *)bufSlot + 4)) {
        bufSlot->buf[0] = (s32)Mem_Alloc(capacity * 4, 1);
        bufSlot->count[0] = 0;
        bufSlot->write[0] = 0;
        bufSlot->read[0] = 0;
    }
    queue->handler = AddDmacHandler(queue->chan, handler, 0);
    queue->unk30 = 0;
    EnableDmac(queue->chan);
    gDmaGsIntcId = AddIntcHandler(0, Dma_GsFinishHandler, 0);
}

/* Queues a chain's tag in the current buffer; -1 when it is full. The
 * counter's address is integer arithmetic (the offset first, as the original
 * adds it). */
s32 DmaQueue_Add(u32 tag, DmaQueue *queue)
{
    s32 bufIdx = gPacketBufIdx;
    u32 byteOffset = bufIdx * 4;
    volatile u32 *countPtr = (volatile u32 *)(byteOffset + (u32)queue + 0x14);
    u32 *buf;

    if (*countPtr <= queue->capacity - 1) {
        buf = Mem_GetAddr((void *) queue->buf[bufIdx]);
        (*countPtr)++;
        buf[queue->write[bufIdx]++] = tag;
        if (queue->write[bufIdx] == queue->capacity) {
            queue->write[bufIdx] = 0;
        }
        queue->chainStart = 0;
        queue->chainEnd = 0;
        return 0;
    }
    return -1;
}

/*
 * Queues a DMA chain: entry is its first tag's address (with a kind in bits
 * 28-31), last its last tag. Unless mode is 2, the chain is linked to the
 * previous one when that ended with an END tag (made a NEXT to this chain)
 * or, for kind 1, a REFE (made a REF); otherwise it becomes a queue entry.
 * Mode 1 ends the linking after this chain.
 */
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *queue)
{
    u32 *prev;
    u32 tagWord;
    s32 bufIdx;
    u32 byteOffset;
    u32 *buf;
    volatile u32 *countPtr;

    if (mode == 2) {
    add:
        bufIdx = gPacketBufIdx;
        byteOffset = bufIdx * 4;
        countPtr = (volatile u32 *)(byteOffset + (u32)queue + 0x14);
        if (*countPtr <= queue->capacity - 1) {
            buf = Mem_GetAddr((void *) queue->buf[bufIdx]);
            (*countPtr)++;
            buf[queue->write[bufIdx]++] = entry;
            if (queue->write[bufIdx] == queue->capacity) {
                queue->write[bufIdx] = 0;
            }
            queue->chainStart = entry;
            queue->chainEnd = last;
            return 0;
        }
        goto fail;
    }
    if (queue->chainStart == 0) {
        goto add;
    }
    prev = queue->chainEnd;
    if (prev == NULL) {
        goto add;
    }
    switch (((u32)queue->chainStart & 0xF0000000) >> 28) {
    case 0:
    case 3:
    case 4:
    case 5:
        tagWord = prev[0];
        switch (tagWord & 0x70000000) {
        case 0x70000000:
            prev[0] = (tagWord & 0xFFFF) + 0x20000000;
            prev[1] = entry & 0x0FFFFFFF;
            goto linked;
        }
        goto add;
    case 1:
        tagWord = prev[0];
        switch (tagWord & 0x70000000) {
        case 0:
            prev[0] = (tagWord & 0xFFFF) + 0x30000000;
            goto linked;
        }
        goto add;
    case 2:
    default:
        goto add;
    }
linked:
    if (mode == 1) {
        queue->chainStart = 0;
        queue->chainEnd = 0;
    } else {
        queue->chainStart = entry;
        queue->chainEnd = last;
    }
    return 0;
fail:
    queue->chainStart = 0;
    queue->chainEnd = 0;
    return -1;
}

/*
 * DMAC interrupt handler of the queue (DmaQueue_Init installs it). For
 * channel 1, when the running chain has ended (or a download has been
 * received), drops that entry from the buffer being sent and starts the
 * next one: kinds 0, 1, 3, 4 and 5 are sent as chains, kind 2 (a
 * download) arms the GS interrupt for Dma_GsFinishHandler first. Returns 0.
 */
s32 DmaQueue_Handler(s32 channel)
{
    DmaQueue *queue = &gDmaQueue;
    u32 chcr;
    u32 entry;
    s32 bufIdx;
    u32 *buf;

    switch (channel) {
    case 1:
        chcr = *gDmaChcrRegs[queue->chan];
        if (chcr & 0x100) {
            break;
        }
        if ((chcr & 0xC) == 4) {
            if (!(chcr & 0x80)) {
                break;
            }
            if (!(chcr & 0x80000000)) {
                break;
            }
        } else if ((chcr & 0xC) != 0 || !(queue->unk30 & 1)) {
            break;
        }
        bufIdx = gPacketBufIdx ^ 1;
        buf = Mem_GetAddr((void *) queue->buf[bufIdx]);
        queue->count[bufIdx]--;
        entry = buf[queue->read[bufIdx]++];
        if (queue->read[bufIdx] == queue->capacity) {
            queue->read[bufIdx] = 0;
        }
        switch ((entry & 0xF0000000) >> 28) {
        case 2:
            queue->unk30 = 0;
            VIF1_STAT = 0;
            GS_BUSDIR = 0;
            GS_IMR = gDmaSavedImr;
            VIF1_FIFO = D_001BA110;
            break;
        }
        if (queue->write[bufIdx] != queue->read[bufIdx]) {
            entry = buf[queue->read[bufIdx]];
            queue->unk2C = entry;
            chcr = (entry & 0xF0000000) >> 28;
            entry &= 0x0FFFFFFF;
            if (entry != 0) {
                switch (chcr) {
                case 0:
                case 1:
                case 3:
                case 4:
                case 5:
                    gDmaChan[queue->chan]->chcr.tte = 1;
                    gDmaChan[queue->chan]->chcr.tie = 1;
                    sceDmaSend(gDmaChan[queue->chan], (void *)entry);
                    break;
                case 2:
                    gDmaRecvDst = ((u32 *)entry)[0x20];
                    gDmaRecvQwc = ((u32 *)entry)[0x21];
                    GS_CSR = 2;
                    gDmaSavedImr = GS_IMR;
                    GS_IMR = ~0x200LL;
                    iEnableIntc(0);
                    queue->unk30 |= 1;
                    gDmaChan[queue->chan]->chcr.tte = 0;
                    gDmaChan[queue->chan]->chcr.tie = 0;
                    sceDmaSend(gDmaChan[queue->chan], (void *)entry);
                    break;
                }
            }
        }
        break;
    }
    __asm__ volatile ("sync.l\n\tei");
    return 0;
}

/* Swaps the queue buffers and starts the first entry of the one just
 * filled. */
void SendFrameDmaChain(void)
{
    DmaQueue *queue = &gDmaQueue;
    s32 bufIdx;
    DmaChan *chan;
    u32 entry;
    u32 addr;

    queue->chainStart = 0;
    queue->chainEnd = 0;
    bufIdx = gPacketBufIdx;
    gPacketBufIdx = bufIdx ^ 1;
    if (queue->count[bufIdx] != 0) {
        chan = gDmaChan[queue->chan];
        entry = ((u32 *)Mem_GetAddr((void *) queue->buf[bufIdx]))[queue->read[bufIdx]];
        queue->unk2C = entry;
        switch ((entry & 0xF0000000) >> 28) {
        case 0:
        case 1:
        case 3:
        case 4:
        case 5:
            chan->chcr.tte = 1;
            chan->chcr.tie = 1;
            FlushCache(0);
            sceDmaSend(chan, (void *)(entry & 0x0FFFFFFF));
            break;
        case 2:
            addr = entry & 0x0FFFFFFF;
            gDmaRecvDst = ((u32 *)addr)[0x20];
            gDmaRecvQwc = ((u32 *)addr)[0x21];
            GS_CSR = 2;
            gDmaSavedImr = GS_IMR;
            GS_IMR = ~0x200LL;
            EnableIntc(0);
            queue->unk30 |= 1;
            chan->chcr.tte = 0;
            chan->chcr.tie = 0;
            FlushCache(0);
            sceDmaSend(chan, (void *)addr);
            break;
        }
    }
}

/*
 * Waits until the buffer of the DMA queue being sent is empty and channel 1
 * (VIF1) is idle. Returns 1.
 */
s32 Dma_WaitIdle(void)
{
    while (gDmaQueue.count[gPacketBufIdx ^ 1] != 0) {
    }
    while (sceDmaSync(gDmaChan[1], 1, 0) != 0) {
    }
    return 1;
}

/* This file's initialised data. */
u64 gVif1Path3Unmask[2] = {
#include "main/001BA110.inc"
};
u32 *gDmaChcrRegs[10] = {
#include "main/001BA120.inc"
};
