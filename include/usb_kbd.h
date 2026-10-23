#ifndef USB_KBD_H
#define USB_KBD_H

#include "common.h"

/*
 * The USB keyboard, through libusbkb: UsbKb_RunInitStep runs one step of the
 * keyboard's set-up (the boot calls it until it returns 0), UsbKb_Poll goes
 * on with the set-up until the keyboard is ready and then reads keyboard 0,
 * returning its key codes as a 9-byte report (NULL when it has none), and
 * UsbKb_IsConnected says whether a port's keyboard is connected (0, else
 * -1).
 */

s32 UsbKb_RunInitStep(void);
u8 *UsbKb_Poll(void);
s32 UsbKb_IsConnected(s32 port);

#endif
