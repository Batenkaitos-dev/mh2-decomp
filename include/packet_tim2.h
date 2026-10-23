#ifndef PACKET_TIM2_H
#define PACKET_TIM2_H

#include "common.h"

/*
 * The packet buffers, one per frame, from which the frame's DMA packets are
 * allocated (Packet_InitBuffers, Packet_Reset, Packet_Alloc), and a texture
 * handle made from a TIM2 file (Tex_CreateFromTim2).
 */

void Packet_InitBuffers(void);
void Packet_Reset(void);
void *Packet_Alloc(u32 size, s32 align);
u32 Tex_CreateFromTim2(u8 *file, s32 mode);
void Tex_AlphaToGs(u8 *pixels, s32 width, s32 height);

#endif
