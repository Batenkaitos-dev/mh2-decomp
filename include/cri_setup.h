#ifndef CRI_SETUP_H
#define CRI_SETUP_H

#include "common.h"

/*
 * The CRI libraries' hooks in the boot executable: the VBlank hook
 * Display_AdxVint calls each frame (AdxWrap_VintHook), and the start-up of
 * the Sofdec movie library for the display's rate (Movie_InitSfd, with the
 * empty callback it hands the library).
 */

void AdxWrap_VintHook(s32 param);
void Movie_NullCallback(void);
void Movie_InitSfd(s32 pal);

#endif
