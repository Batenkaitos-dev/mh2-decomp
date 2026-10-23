#ifndef LIBUSBKB_H
#define LIBUSBKB_H

#include "common.h"

/*
 * The USB keyboard library's calls that the game makes: the keyboard's set-
 * up, its input and its lights (libusbkb).
 */

void sceUsbKbSetLEDStatus(s32 a, s32 b);
void sceUsbKbSetRepeat(s32 a, s32 b, s32 c);
void sceUsbKbClearRbuf(s32);

#endif
