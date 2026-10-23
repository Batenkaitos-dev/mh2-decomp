#ifndef GS_SETUP_H
#define GS_SETUP_H

#include "common.h"

/*
 * The GS's set-up and the frame's clears. Gs_SetupDisplay places the frame
 * buffers and the Z buffer in GS memory and builds their draw environments,
 * Gs_ShowFrame shows a frame buffer, Gs_ClearFrame and Gs_ClearColor fill
 * the frame, and Gs_ClearVram clears GS memory at boot. Debug_Printf writes
 * the debug output file.
 */

void Gs_ClearVram(void);
void Gs_SetupDisplay(s32 fbDepth, s32 zDepth, s32 frameMode, s32 tvMode, s32 narrow);
void Gs_ShowFrame(s32 buf, s32 boot);
void Gs_ClearFrame(void);
s32 Debug_Printf(const char *fmt, ...);
s32 Gs_ClearColor(u8 mode, u32 color);

#endif
