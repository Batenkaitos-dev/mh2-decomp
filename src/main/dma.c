/*
 * 0x179590: DMA tags, image uploads, the DMA queue and handlers
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

u128 *DmaTag_Cnt(u128 *tag, s32 qwc, s32 irq, s32 unused)
{
    u32 *p = (u32 *)tag;

    *tag = 0;
    p[0] = qwc + 0x10000000;
    if (irq == 1) {
        p[0] |= 0x80000000;
    }
    return tag + 1;
}

u128 *DmaTag_Next(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused)
{
    u32 *p = (u32 *)tag;
    u32 a = addr & 0x0FFFFFFF;
    u32 spr = 0;

    if ((addr & 0x70000000) == 0x70000000) {
        spr = 0x80000000;
    }
    *tag = 0;
    p[0] = qwc + 0x20000000;
    p[1] = a | spr;
    if (irq == 1) {
        p[0] |= 0x80000000;
    }
    return tag + 1;
}

u128 *DmaTag_Ref(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused)
{
    u32 *p = (u32 *)tag;
    u32 a = addr & 0x0FFFFFFF;
    u32 spr = 0;

    if ((addr & 0x70000000) == 0x70000000) {
        spr = 0x80000000;
    }
    *tag = 0;
    p[0] = qwc + 0x30000000;
    p[1] = a | spr;
    if (irq == 1) {
        p[0] |= 0x80000000;
    }
    return tag + 1;
}

u128 *DmaTag_RefE(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused)
{
    u32 *p = (u32 *)tag;
    u32 a = addr & 0x0FFFFFFF;
    u32 spr = 0;

    if ((addr & 0x70000000) == 0x70000000) {
        spr = 0x80000000;
    }
    *tag = 0;
    p[0] = qwc;
    p[1] = a | spr;
    if (irq == 1) {
        p[0] |= 0x80000000;
    }
    return tag + 1;
}

u128 *DmaTag_Call(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused)
{
    u32 *p = (u32 *)tag;
    u32 a = addr & 0x0FFFFFFF;
    u32 spr = 0;

    if ((addr & 0x70000000) == 0x70000000) {
        spr = 0x80000000;
    }
    *tag = 0;
    p[0] = qwc + 0x50000000;
    p[1] = a | spr;
    if (irq == 1) {
        p[0] |= 0x80000000;
    }
    return tag + 1;
}

u128 *DmaTag_Ret(u128 *tag, s32 qwc, s32 irq, s32 unused)
{
    u32 *p = (u32 *)tag;

    *tag = 0;
    p[0] = qwc + 0x60000000;
    if (irq == 1) {
        p[0] |= 0x80000000;
    }
    return tag + 1;
}

u128 *DmaTag_End(u128 *tag, s32 qwc, s32 irq, s32 unused)
{
    u32 *p = (u32 *)tag;

    *tag = 0;
    p[0] = qwc + 0x70000000;
    if (irq == 1) {
        p[0] |= 0x80000000;
    }
    return tag + 1;
}

/* VIF MSCNT, then three NOPs. */
u128 *Vif_MsCnt(u128 *q)
{
    u32 *p = (u32 *)q;

    p[0] = 0x17000000;
    p[1] = 0;
    p[2] = 0;
    p[3] = 0;
    return q + 1;
}

/* STCYCL 4,4 and UNPACK V4-32 of n quadwords to VU address addr (TOPS-relative). */
u128 *Vif_UnpackV4(u128 *q, s32 n, s32 addr)
{
    u32 *p = (u32 *)q;

    p[0] = 0;
    p[1] = 0;
    p[2] = 0x01000404;
    p[3] = (addr | 0x8000) | (n << 16) | 0x6C000000;
    return q + 1;
}

/* NOPs, then DIRECTHL of qwc quadwords to the GIF. */
u128 *Vif_DirectHL(u128 *q, s32 qwc)
{
    u32 *p = (u32 *)q;

    p[0] = 0;
    p[1] = 0;
    p[2] = 0;
    p[3] = qwc | 0x51000000;
    return q + 1;
}

/* Bytes of image-transfer packet for an image of size bytes, sent in chunks
 * of 0x70000 bytes (0x80 each). */
s32 Dma_ImagePacketSize(u32 size)
{
    u32 n = size / 0x70000;

    if (size % 0x70000) {
        n++;
    }
    return n << 7;
}

s32 Tex_UploadPacketSize(u32 unused0)
{
    return 0x40;
}

/*
 * Builds the DMA packet that uploads an image of size bytes at src to GS
 * memory (BITBLTBUF dbp/dbw/psm, rectangle dx, dy, w x h), in chunks of at
 * most 0x70000 bytes; returns the last chunk's REFE tag.
 * irq is u32 (as s32, the stack copy of it the original keeps is missing).
 */
u128 *Dma_BuildLoadImage(u128 *pkt, u32 irq, u32 src, u32 size, s16 dbp, s16 dbw, s16 psm, s16 dx,
                         s16 dy, s16 w, s16 h)
{
    u32 n;
    u64 *p;
    u128 *last;
    s32 bytes;
    u32 k;
    u32 qwc;
    s16 rows;

    n = size / 0x70000;
    last = NULL;
    if (size % 0x70000) {
        n++;
    }
    if (n == 1) {
        rows = h;
    } else {
        switch (psm) {
        case 0:
        case 1:
            rows = 0x70000 / (w * 4);
            break;
        case 2:
        case 10:
            rows = 0x70000 / (w * 2);
            break;
        case 19:
            rows = 0x70000 / w;
            break;
        case 20:
            rows = 0x70000 / (w >> 1);
            break;
        }
    }
    if (dbw == 0) {
        dbw = 1;
    }
    p = (u64 *)pkt;
    for (k = 0; k < n; k++) {
        switch (psm) {
        case 0:
        case 1:
            bytes = w * rows * 4;
            break;
        case 2:
        case 10:
            bytes = w * rows * 2;
            break;
        case 19:
            bytes = w * rows;
            break;
        case 20:
            bytes = w * rows / 2;
            break;
        }
        DmaTag_Cnt((u128 *)p, 6, 0, 0);
        ((u32 *)p)[2] = 0x13000000;
        ((u32 *)p)[3] = 0x51000006;
        p[2] = 0x1000000000000004;
        p[3] = 0xE;
        p[4] = (((u64)dbp << 32) | ((u64)dbw << 48)) | ((u64)psm << 56);
        p[5] = 0x50;
        p[6] = ((u64)dx << 32) | ((u64)dy << 48);
        p[7] = 0x51;
        p[8] = w | ((u64)rows << 32);
        p[9] = 0x52;
        p[10] = 0;
        p[11] = 0x53;
        if (k >= n - 1) {
            qwc = size >> 4;
            p[12] = (u64)qwc | 0x0800000000008000;
            last = (u128 *)(p + 14);
            p[13] = 0;
            DmaTag_RefE(last, qwc, src, irq, 0);
            ((u32 *)p)[0x1E] = 0;
            ((u32 *)p)[0x1F] = qwc | 0x51000000;
        } else {
            p[12] = 0x080000000000F000;
            p[13] = 0;
            DmaTag_Ref((u128 *)(p + 14), 0x7000, src, 0, 0);
            ((u32 *)p)[0x1E] = 0;
            ((u32 *)p)[0x1F] = 0x51007000;
            src += bytes;
            dy += rows;
            p += 16;
            if (h < dy + rows) {
                rows = h - dy;
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
    u64 *p = (u64 *)pkt;

    DmaTag_End((u128 *)p, 3, irq, 0);
    Vif_DirectHL((u128 *)p + 1, 2);
    p[4] = 0x1000000000008001;
    p[5] = 0xE;
    p[6] = 0;
    p[7] = 0x3F;
}

s32 Dma_StoreImagePacketSize(u32 size)
{
    u32 n = size / 0x70000;

    if (size % 0x70000) {
        n++;
    }
    return n * 0x90;
}

/*
 * Queues the download of a GS memory rectangle (BITBLTBUF sbp/sbw/psm,
 * sx, sy, w x h) to dst, size bytes, in chunks of at most 0x70000 bytes:
 * each chunk is a queue entry of kind 2, whose packet ends with dst and the
 * quadword count for the GS interrupt handler (Dma_GsFinishHandler).
 */
void Dma_QueueStoreImage(void *dst, u32 size, s16 sbp, s16 sbw, s16 psm, s16 sx, s16 sy, s16 w, s16 h,
                         u32 irq)
{
    u64 *p;
    u32 n;
    s32 bytes;
    u32 k;
    s16 rows;

    n = size / 0x70000;
    if (size % 0x70000) {
        n++;
    }
    if (n == 1) {
        rows = h;
    } else {
        switch (psm) {
        case 0:
        case 1:
            rows = 0x70000 / (w * 4);
            break;
        case 2:
        case 10:
            rows = 0x70000 / (w * 2);
            break;
        case 19:
            rows = 0x70000 / w;
            break;
        case 20:
            rows = 0x70000 / (w >> 1);
            break;
        }
    }
    if (sbw == 0) {
        sbw = 1;
    }
    p = Packet_Alloc(Dma_StoreImagePacketSize(size), 16);
    for (k = 0; k < n; k++) {
        switch (psm) {
        case 0:
        case 1:
            bytes = w * rows * 4;
            break;
        case 2:
        case 10:
            bytes = w * rows * 2;
            break;
        case 19:
            bytes = w * rows;
            break;
        case 20:
            bytes = w * rows / 2;
            break;
        }
        DmaTag_End((u128 *)p, 7, irq, 0);
        ((u32 *)p)[2] = 0;
        ((u32 *)p)[3] = 0;
        ((u32 *)p)[4] = 0;
        ((u32 *)p)[5] = 0x06008000;
        ((u32 *)p)[6] = 0x13000000;
        ((u32 *)p)[7] = 0x50000006;
        p[4] = 0x1000000000008005;
        p[5] = 0xE;
        p[6] = sbp | ((u64)sbw << 16) | ((u64)psm << 24);
        p[7] = 0x50;
        p[8] = sx | ((u64)sy << 16);
        p[9] = 0x51;
        p[10] = w | ((u64)rows << 32);
        p[11] = 0x52;
        p[12] = 0;
        p[13] = 0x61;
        p[14] = 1;
        p[15] = 0x53;
        ((u32 *)p)[0x20] = (u32)dst;
        ((u32 *)p)[0x21] = (u32)bytes >> 4;
        ((u32 *)p)[0x22] = 0;
        ((u32 *)p)[0x23] = 0;
        DmaQueue_Add(((u32)p & 0x0FFFFFFF) | 0x20000000, &gDmaQueue);
        p += 18;
        dst = (u8 *)dst + bytes;
        sy += rows;
        if (h < sy + rows) {
            rows = h - sy;
        }
    }
}

/* The same download done at once: each chunk is sent and waited for. */
void Dma_StoreImage(u32 dst, u32 size, s16 sbp, s16 sbw, s16 psm, s16 sx, s16 sy, s16 w, s16 h)
{
    u32 n;
    u32 k;
    DmaChan *chan;
    s16 rows;
    s32 bytes;
    u64 *p;

    Dma_WaitIdle();
    n = size / 0x70000;
    if (size % 0x70000) {
        n++;
    }
    if (n == 1) {
        rows = h;
    } else {
        switch (psm) {
        case 0:
        case 1:
            rows = 0x70000 / (w * 4);
            break;
        case 2:
        case 10:
            rows = 0x70000 / (w * 2);
            break;
        case 19:
            rows = 0x70000 / w;
            break;
        case 20:
            rows = 0x70000 / (w >> 1);
            break;
        }
    }
    if (sbw == 0) {
        sbw = 1;
    }
    chan = gDmaChan[gDmaQueue.chan];
    for (k = 0; k < n; k++) {
        switch (psm) {
        case 0:
        case 1:
            bytes = w * rows * 4;
            break;
        case 2:
        case 10:
            bytes = w * rows * 2;
            break;
        case 19:
            bytes = w * rows;
            break;
        case 20:
            bytes = w * rows / 2;
            break;
        }
        p = Packet_Alloc(0x90, 16);
        DmaTag_End((u128 *)p, 7, 0, 0);
        ((u32 *)p)[2] = 0;
        ((u32 *)p)[3] = 0;
        ((u32 *)p)[4] = 0;
        ((u32 *)p)[5] = 0x06008000;
        ((u32 *)p)[6] = 0x13000000;
        ((u32 *)p)[7] = 0x50000006;
        p[4] = 0x1000000000008005;
        p[5] = 0xE;
        p[6] = sbp | ((u64)sbw << 16) | ((u64)psm << 24);
        p[7] = 0x50;
        p[8] = sx | ((u64)sy << 16);
        p[9] = 0x51;
        p[10] = w | ((u64)rows << 32);
        p[11] = 0x52;
        p[12] = 0;
        p[13] = 0x61;
        p[14] = 1;
        p[15] = 0x53;
        gDmaRecvQwc = (u32)bytes >> 4;
        gDmaRecvDst = dst;
        GS_CSR = 2;
        gDmaSavedImr = GS_IMR;
        GS_IMR = ~0x200LL;
        EnableIntc(0);
        chan->chcr.tte = 0;
        chan->chcr.tie = 0;
        FlushCache(0);
        sceDmaSend(chan, p);
        sceGsSyncPath(0, 0);
        while (gDmaRecvQwc != 0) {
        }
        sceGsSyncPath(0, 0);
        VIF1_STAT = 0;
        sy += rows;
        GS_BUSDIR = 0;
        GS_IMR = gDmaSavedImr;
        VIF1_FIFO = D_001BA110;
        dst += bytes;
        if (h < sy + rows) {
            rows = h - sy;
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

void DmaQueue_Init(DmaQueue *q, u32 capacity, s32 (*handler)(s32))
{
    s32 i;
    DmaQueue *b;

    q->capacity = capacity;
    for (i = 0, b = q; i < 2; i++, b = (DmaQueue *)((u8 *)b + 4)) {
        b->buf[0] = (s32)Mem_Alloc(capacity * 4, 1);
        b->count[0] = 0;
        b->write[0] = 0;
        b->read[0] = 0;
    }
    q->handler = AddDmacHandler(q->chan, handler, 0);
    q->unk30 = 0;
    EnableDmac(q->chan);
    gDmaGsIntcId = AddIntcHandler(0, Dma_GsFinishHandler, 0);
}

/* Queues a chain's tag in the current buffer; -1 when it is full. The
 * counter's address is integer arithmetic (the offset first, as the original
 * adds it). */
s32 DmaQueue_Add(u32 tag, DmaQueue *q)
{
    s32 i = gPacketBufIdx;
    u32 off = i * 4;
    volatile u32 *cnt = (volatile u32 *)(off + (u32)q + 0x14);
    u32 *buf;

    if (*cnt <= q->capacity - 1) {
        buf = Mem_GetAddr((void *) q->buf[i]);
        (*cnt)++;
        buf[q->write[i]++] = tag;
        if (q->write[i] == q->capacity) {
            q->write[i] = 0;
        }
        q->chainStart = 0;
        q->chainEnd = 0;
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
s32 DmaQueue_AddChain(s32 mode, u32 entry, u32 *last, DmaQueue *q)
{
    u32 *prev;
    u32 t;
    s32 i;
    u32 off;
    u32 *buf;
    volatile u32 *cnt;

    if (mode == 2) {
    add:
        i = gPacketBufIdx;
        off = i * 4;
        cnt = (volatile u32 *)(off + (u32)q + 0x14);
        if (*cnt <= q->capacity - 1) {
            buf = Mem_GetAddr((void *) q->buf[i]);
            (*cnt)++;
            buf[q->write[i]++] = entry;
            if (q->write[i] == q->capacity) {
                q->write[i] = 0;
            }
            q->chainStart = entry;
            q->chainEnd = last;
            return 0;
        }
        goto fail;
    }
    if (q->chainStart == 0) {
        goto add;
    }
    prev = q->chainEnd;
    if (prev == NULL) {
        goto add;
    }
    switch (((u32)q->chainStart & 0xF0000000) >> 28) {
    case 0:
    case 3:
    case 4:
    case 5:
        t = prev[0];
        switch (t & 0x70000000) {
        case 0x70000000:
            prev[0] = (t & 0xFFFF) + 0x20000000;
            prev[1] = entry & 0x0FFFFFFF;
            goto linked;
        }
        goto add;
    case 1:
        t = prev[0];
        switch (t & 0x70000000) {
        case 0:
            prev[0] = (t & 0xFFFF) + 0x30000000;
            goto linked;
        }
        goto add;
    case 2:
    default:
        goto add;
    }
linked:
    if (mode == 1) {
        q->chainStart = 0;
        q->chainEnd = 0;
    } else {
        q->chainStart = entry;
        q->chainEnd = last;
    }
    return 0;
fail:
    q->chainStart = 0;
    q->chainEnd = 0;
    return -1;
}

/* The DMAC handler of the queue's channel: when a chain ends, the next queue
 * entry is started (a download is set up for the GS interrupt handler). */
s32 DmaQueue_Handler(s32 ch)
{
    DmaQueue *q = &gDmaQueue;
    u32 chcr;
    u32 e;
    s32 i;
    u32 *buf;

    switch (ch) {
    case 1:
        chcr = *gDmaChcrRegs[q->chan];
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
        } else if ((chcr & 0xC) != 0 || !(q->unk30 & 1)) {
            break;
        }
        i = gPacketBufIdx ^ 1;
        buf = Mem_GetAddr((void *) q->buf[i]);
        q->count[i]--;
        e = buf[q->read[i]++];
        if (q->read[i] == q->capacity) {
            q->read[i] = 0;
        }
        switch ((e & 0xF0000000) >> 28) {
        case 2:
            q->unk30 = 0;
            VIF1_STAT = 0;
            GS_BUSDIR = 0;
            GS_IMR = gDmaSavedImr;
            VIF1_FIFO = D_001BA110;
            break;
        }
        if (q->write[i] != q->read[i]) {
            e = buf[q->read[i]];
            q->unk2C = e;
            chcr = (e & 0xF0000000) >> 28;
            e &= 0x0FFFFFFF;
            if (e != 0) {
                switch (chcr) {
                case 0:
                case 1:
                case 3:
                case 4:
                case 5:
                    gDmaChan[q->chan]->chcr.tte = 1;
                    gDmaChan[q->chan]->chcr.tie = 1;
                    sceDmaSend(gDmaChan[q->chan], (void *)e);
                    break;
                case 2:
                    gDmaRecvDst = ((u32 *)e)[0x20];
                    gDmaRecvQwc = ((u32 *)e)[0x21];
                    GS_CSR = 2;
                    gDmaSavedImr = GS_IMR;
                    GS_IMR = ~0x200LL;
                    iEnableIntc(0);
                    q->unk30 |= 1;
                    gDmaChan[q->chan]->chcr.tte = 0;
                    gDmaChan[q->chan]->chcr.tie = 0;
                    sceDmaSend(gDmaChan[q->chan], (void *)e);
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
    DmaQueue *q = &gDmaQueue;
    s32 i;
    DmaChan *chan;
    u32 e;
    u32 addr;

    q->chainStart = 0;
    q->chainEnd = 0;
    i = gPacketBufIdx;
    gPacketBufIdx = i ^ 1;
    if (q->count[i] != 0) {
        chan = gDmaChan[q->chan];
        e = ((u32 *)Mem_GetAddr((void *) q->buf[i]))[q->read[i]];
        q->unk2C = e;
        switch ((e & 0xF0000000) >> 28) {
        case 0:
        case 1:
        case 3:
        case 4:
        case 5:
            chan->chcr.tte = 1;
            chan->chcr.tie = 1;
            FlushCache(0);
            sceDmaSend(chan, (void *)(e & 0x0FFFFFFF));
            break;
        case 2:
            addr = e & 0x0FFFFFFF;
            gDmaRecvDst = ((u32 *)addr)[0x20];
            gDmaRecvQwc = ((u32 *)addr)[0x21];
            GS_CSR = 2;
            gDmaSavedImr = GS_IMR;
            GS_IMR = ~0x200LL;
            EnableIntc(0);
            q->unk30 |= 1;
            chan->chcr.tte = 0;
            chan->chcr.tie = 0;
            FlushCache(0);
            sceDmaSend(chan, (void *)addr);
            break;
        }
    }
}

s32 Dma_WaitIdle(void)
{
    while (gDmaQueue.count[gPacketBufIdx ^ 1] != 0) {
    }
    while (sceDmaSync(gDmaChan[1], 1, 0) != 0) {
    }
    return 1;
}

/* dma's .data (0x1BA110-0x1BA150) */
u64 gVif1Path3Unmask[2] = {
#include "main/001BA110.inc"
};
u32 *gDmaChcrRegs[10] = {
#include "main/001BA120.inc"
};
