/*
 * The DMA queue and the transfers from GS memory.
 *
 * The queue (gDmaQueue) holds the frame's DMA chains in two buffers: one
 * is filled while the other is sent. SendFrameDmaChain swaps them and
 * starts the first entry, and the queue's DMAC handler starts each next
 * one. Dma_WaitIdle waits for the buffer being sent, and Dma_FlushQueue
 * sends the queued chains at once.
 *
 * An image goes from GS memory to main memory either as a queue entry
 * (Dma_QueueStoreImage) or at once (Dma_StoreImage); the GS FINISH
 * interrupt (Dma_GsFinishHandler) then starts the VIF1 transfer that
 * receives it.
 */
#include "common.h"
#include "dma.h"
#include "hw_regs.h"
#include "mem_util.h"
#include "packet_tim2.h"
#include "libgraph.h"
#include "libkernl.h"

/* Built at optimization level 4: Dma_QueueStoreImage and Dma_StoreImage
 * only match so. */
#pragma optimization_level 4

extern DmaQueue gDmaQueue;
extern s32 gPacketBufIdx;
extern s32 gDmaGsIntcId;
extern u32 gDmaRecvDst;
extern u32 gDmaRecvQwc;
extern u64 gDmaSavedImr;
extern u128 D_001BA110;
extern u32 *gDmaChcrRegs[10];
s32 sceDmaRecvN(DmaChan *chan, void *addr, s32 size);
s32 sceDmaSend(DmaChan *chan, void *tag);
s32 sceDmaSync(DmaChan *chan, s32 mode, s32 timeout);
extern s32 gFlipCountdown;

/*
 * Bytes of the packets Dma_QueueStoreImage builds for an image of size
 * bytes: 144 per chunk of at most 0x70000 bytes.
 */
s32 Dma_StoreImagePacketSize(u32 size)
{
    u32 chunkCount = size / 0x70000;

    if (size % 0x70000) {
        chunkCount++;
    }
    return chunkCount * 144;
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
        words[5] = 14;
        words[6] = sbp | ((u64)sbw << 16) | ((u64)psm << 24);
        words[7] = 80;
        words[8] = srcX | ((u64)srcY << 16);
        words[9] = 81;
        words[10] = width | ((u64)rows << 32);
        words[11] = 82;
        words[12] = 0;
        words[13] = 97;
        words[14] = 1;
        words[15] = 83;
        ((u32 *)words)[32] = (u32)dst;
        ((u32 *)words)[33] = (u32)bytes >> 4;
        ((u32 *)words)[34] = 0;
        ((u32 *)words)[35] = 0;
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
        words = Packet_Alloc(144, 16);
        DmaTag_End((u128 *)words, 7, 0, 0);
        ((u32 *)words)[2] = 0;
        ((u32 *)words)[3] = 0;
        ((u32 *)words)[4] = 0;
        ((u32 *)words)[5] = 0x06008000;
        ((u32 *)words)[6] = 0x13000000;
        ((u32 *)words)[7] = 0x50000006;
        words[4] = 0x1000000000008005;
        words[5] = 14;
        words[6] = sbp | ((u64)sbw << 16) | ((u64)psm << 24);
        words[7] = 80;
        words[8] = srcX | ((u64)srcY << 16);
        words[9] = 81;
        words[10] = width | ((u64)rows << 32);
        words[11] = 82;
        words[12] = 0;
        words[13] = 97;
        words[14] = 1;
        words[15] = 83;
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
    volatile u32 *countPtr = (volatile u32 *)(byteOffset + (u32)queue + 20);
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
        countPtr = (volatile u32 *)(byteOffset + (u32)queue + 20);
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
                    gDmaRecvDst = ((u32 *)entry)[32];
                    gDmaRecvQwc = ((u32 *)entry)[33];
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
            gDmaRecvDst = ((u32 *)addr)[32];
            gDmaRecvQwc = ((u32 *)addr)[33];
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

/* Sends the current buffer's queued chain and waits for it, then resets
 * the packet buffer and holds the flip for 2 frames (gFlipCountdown).
 * Returns 1 if anything was sent, else 0. */
s32 Dma_FlushQueue(void)
{
    if ((gDmaQueue.count[0] == 0) && (gDmaQueue.count[1] == 0)) {
        return 0;
    }
    Dma_WaitIdle();
    if (gDmaQueue.count[gPacketBufIdx] == 0) {
        return 0;
    }
    SendFrameDmaChain();
    Dma_WaitIdle();
    Packet_Reset();
    gFlipCountdown = 2;
    return 1;
}

/* This file's initialised data. */
u64 gVif1Path3Unmask[2] = {
#include "main/001BA110.inc"
};
u32 *gDmaChcrRegs[10] = {
#include "main/001BA120.inc"
};
