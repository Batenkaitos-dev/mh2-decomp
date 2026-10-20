/*
 * Dma_FlushQueue, the DMA queue's flush: the queued packet
 * chain is sent at once instead of at the frame's end. The queue
 * (gDmaQueue, include/dma.h) and the rest of its code are in dma.c.
 */
#include "common.h"
#include "dma.h"

void Packet_Reset(void);
void SendFrameDmaChain(void);
s32 Dma_WaitIdle(void);
extern DmaQueue gDmaQueue;
extern s32 gPacketBufIdx;
extern s32 gFlipCountdown;

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
