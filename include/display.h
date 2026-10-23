#ifndef DISPLAY_H
#define DISPLAY_H

#include "common.h"

/*
 * The display's start-up and the end of a frame. InitDisplay sets up the
 * system (Sys_Init: the IOP, the memory, DMA and the memory card), the
 * packet buffers, the GS's display and the pads. VBlankHandler counts the
 * VBlanks and shows a finished frame; Sys_FlipFrame ends a frame and starts
 * the next, and Sys_EnableGameVSync turns on the game's VSync counter.
 */

void Sys_EnableGameVSync(void);
s32 InitDisplay(s32 w, s32 h);
s32 Sys_Init(void);
void VBlankHandler(void);
s32 Sys_FlipFrame(s32 unused);

#endif
