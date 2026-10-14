#ifndef DMA_H
#define DMA_H

/*
 * The boot ELF's DMA layer (src/main/dma.c): DMA tags and VIF codes, image
 * transfers to and from GS memory, and the double-buffered queue of DMA
 * chains sent to the GIF each frame.
 */
#include "common.h"

/* A quadword. MWCC accepts GCC's mode attribute and stores one with sq. */
typedef unsigned int u128 __attribute__((mode(TI)));

/* EE DMAC channel registers (Sony libdma's sceDmaChan). */
typedef struct DmaChcr {
    u32 dir : 1;
    u32 pad0 : 1;
    u32 mod : 2;
    u32 asp : 2;
    u32 tte : 1;
    u32 tie : 1;
    u32 str : 1;
    u32 pad1 : 7;
    u32 tag : 16;
} DmaChcr;

typedef struct DmaChan {
    DmaChcr chcr;   u32 p0[3];  /* 0x00 */
    void *madr;     u32 p1[3];  /* 0x10 */
    u32 qwc;        u32 p2[3];  /* 0x20 */
    void *tadr;     u32 p3[3];  /* 0x30 */
    void *as0;      u32 p4[3];  /* 0x40 */
    void *as1;      u32 p5[3];  /* 0x50 */
    u32 p6[4];                  /* 0x60 */
    u32 p7[4];                  /* 0x70 */
    void *sadr;     u32 p8[3];  /* 0x80 */
} DmaChan;

/* The DMA chains queued for the GIF, double-buffered by gDmaBufIndex. */
typedef struct DmaQueue {
    s32 chan;           /* 0x00 DMAC channel number */
    u32 capacity;       /* 0x04 entries per buffer */
    s32 handler;        /* 0x08 AddDmacHandler id */
    s32 buf[2];         /* 0x0C heap handles of the two entry buffers */
    volatile u32 count[2]; /* 0x14 entries queued in each buffer */
    u32 write[2];       /* 0x1C */
    u32 read[2];        /* 0x24 */
    s32 unk2C;
    s32 unk30;
    s32 chainStart;     /* 0x34 the first tag address of the queued chain (its top bits select the memory) */
    u32 *chainEnd;      /* 0x38 the last tag of the queued chain: the next chain links to it */
    s32 unk3C;
} DmaQueue;
typedef char DmaQueue_size_check[(sizeof(DmaQueue) == 0x40) ? 1 : -1];

extern DmaChan *gDmaChan[10];

/* DMA tags (id in bits 28-30, IRQ in bit 31): each writes one quadword and
 * returns the next. Every caller passes a last argument that the tags ignore. */
u128 *DmaTag_Cnt(u128 *tag, s32 qwc, s32 irq, s32 unused);
u128 *DmaTag_Next(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused);
u128 *DmaTag_Ref(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused);
u128 *DmaTag_RefE(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused);
u128 *DmaTag_Call(u128 *tag, s32 qwc, u32 addr, s32 irq, s32 unused);
u128 *DmaTag_Ret(u128 *tag, s32 qwc, s32 irq, s32 unused);
u128 *DmaTag_End(u128 *tag, s32 qwc, s32 irq, s32 unused);

u128 *Vif_MsCnt(u128 *q);
u128 *Vif_UnpackV4(u128 *q, s32 n, s32 addr);
u128 *Vif_DirectHL(u128 *q, s32 qwc);
s32 Dma_ImagePacketSize(u32 size);

#endif /* DMA_H */
