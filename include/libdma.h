#ifndef LIBDMA_H
#define LIBDMA_H

#include "common.h"

/*
 * The DMA library's calls that the game makes: the channels, sends, receives
 * and syncs (libdma). The game's own DMA queue is dma.h's.
 */

void sceDmaReset(s32 mode);

#endif
