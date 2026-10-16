/*
 * Small setters (main 0x182010-0x182040): the packet buffer's mode and the
 * VBlank count of the next flip.
 */
#include "common.h"

extern s32 gPacketMode;
extern s32 gSysNextFlipAt;

/* gPacketMode is 0, 1 or 2; Packet_Reset treats them alike, and
 * Gs_ResetState sets 0 while it builds the render state, then 2. */
void Packet_SetMode0(void)
{
    gPacketMode = 0;
}

void Packet_SetMode2(void)
{
    gPacketMode = 2;
}

/* The VBlank count after which VBlankHandler shows the next frame. */
void Sys_SetNextFlipAt(s32 vblank)
{
    gSysNextFlipAt = vblank;
}
