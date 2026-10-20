/*
 * Sys_EnableGameVSync: turns on the game's VSync counter,
 * which VBlankHandler (display.c) advances while it is on.
 */
#include "common.h"

extern s32 gGameVSyncOn;

void Sys_EnableGameVSync(void)
{
    gGameVSyncOn = 1;
}
