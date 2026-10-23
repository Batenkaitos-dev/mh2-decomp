#ifndef PACKET_MODE_H
#define PACKET_MODE_H

#include "common.h"

/*
 * Two small setters: the packet buffer's mode (gPacketMode:
 * Packet_SetMode0, Packet_SetMode2) and the VBlank count after which the
 * next frame is shown (Sys_SetNextFlipAt).
 */

void Packet_SetMode0(void);
void Packet_SetMode2(void);
void Sys_SetNextFlipAt(s32 vblank);

#endif
