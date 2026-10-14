/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "m2c_macros.h"

void Packet_Reset(void);
void SendFrameDmaChain(void);
s32 Dma_WaitIdle(void);
extern s32 D_002189C4;
extern s32 D_002189C8;
extern s32 gPacketBufIdx;
extern s32 gFlipCountdown;

s32 Dma_FlushQueue(void) {
    if ((D_002189C4 == 0) && (D_002189C8 == 0)) {
        return 0;
    }
    Dma_WaitIdle();
    if ((&D_002189C4)[gPacketBufIdx] == 0) {
        return 0;
    }
    SendFrameDmaChain();
    Dma_WaitIdle();
    Packet_Reset();
    gFlipCountdown = 2;
    return 1;
}
