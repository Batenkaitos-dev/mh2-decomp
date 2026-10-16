#ifndef SHAKE_H
#define SHAKE_H

#include "common.h"

/*
 * The HUD's shake: one element at a time, owned by an id (gShakeId), is
 * moved by an offset along x or y (gShakeOffset). Shake_Start and
 * Shake_Update (unk_361570.c) start and run it; the queries in
 * unk_3633C0.c tell a menu or a HUD element whether its id shakes and by
 * how much (Shake_CopyOffset). The camera's shake is another thing
 * (camera.h's CamShake).
 */

#define SHAKE_NONE 0xFF     /* gShakeId: no shake */

/* The shake's offset, in pixels (gShakeOffset; Shake_CopyOffset's copy). */
typedef struct ShakeOffset {
    s16 x;
    s16 y;
} ShakeOffset;

#endif
