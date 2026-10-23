#ifndef LIBGRAPH_H
#define LIBGRAPH_H

#include "common.h"

/*
 * The GS library's calls that the game makes: resetting and syncing the GS
 * and its paths, image loads (libgraph).
 */

void sceGsResetGraph(s16 mode, s16 inter, s16 omode, s16 ffmode);
s32 sceGsResetPath(void);
s32 sceGsSyncV(s32);
s32 sceGsSyncPath(s32 mode, u16 timeout);
void sceGsSetDefLoadImage(void *lp, s16 dbp, s16 dbw, s16 dpsm, s16 x, s16 y, s16 w, s16 h);
s32 sceGsExecLoadImage(void *lp, void *src);
s32 sceGsSyncVCallback(void (*func)(void), s32 arg);

#endif
