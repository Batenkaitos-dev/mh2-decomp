#ifndef POWER_OFF_H
#define POWER_OFF_H

#include "common.h"

/*
 * The power-off handling: PowerOff_Init starts a thread (PowerOff_Thread)
 * that the console's power button wakes (PowerOff_Callback); it marks the
 * game as exiting, shuts the drives down and switches the console off.
 */

s32 PowerOff_Init(void);
void PowerOff_Thread(s32 sema);
void PowerOff_Callback(s32 sema);

#endif
