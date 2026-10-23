#ifndef DISPLAY_UTIL_H
#define DISPLAY_UTIL_H

#include "common.h"

/*
 * Display helpers: the display's offset (Display_SetOffset), the CRI
 * libraries' call at each VBlank (Display_AdxVint), and the end-of-frame
 * wait (Sys_WaitFlip), which runs the sound's update and requests the flip.
 */

void Display_SetOffset(s32 x, s32 y);
void Display_AdxVint(void);
void Sys_WaitFlip(void);

#endif
