#ifndef GS_REGS_H
#define GS_REGS_H

#include "common.h"

/*
 * The GS register setters of the render state: SCISSOR, ZBUF, TEST, ALPHA,
 * FOGCOL and TEX1. Each queues a small A+D packet for drawing context 1, 2
 * or both (GS_CTX_*); the render state's store calls them when a state
 * changes.
 */

s32 Gs_SetScissor(s32 x, s32 y, s32 width, s32 height, s32 ctx);
s32 Gs_SetZbuf(u32 flags, s32 ctx);
s32 Gs_SetTest(u32 flags, s32 ctx);
s32 Gs_SetAlpha(u32 flags, s32 ctx);
s32 Gs_SetFogColor(u32 color);
s32 Gs_SetTexFilter(u32 flags, s32 ctx);

#endif
