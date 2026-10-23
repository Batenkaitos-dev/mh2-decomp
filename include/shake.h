#ifndef SHAKE_H
#define SHAKE_H

#include "common.h"

/*
 * The HUD's shake: one element at a time, owned by an id (gShakeId), is
 * moved by an offset along x or y (gShakeOffset). Shake_Start and
 * Shake_Update start and run it; the queries
 * tell a menu or a HUD element whether its id shakes and by
 * how much (Shake_CopyOffset). The camera's shake is another thing
 * (camera.h's CamShake).
 */

#define SHAKE_NONE 0xFF     /* gShakeId: no shake */

/* The shake's offset, in pixels (gShakeOffset; Shake_CopyOffset's copy). */
typedef struct ShakeOffset {
    s16 x;
    s16 y;
} ShakeOffset;

void Shake_Start(u8 id, u8 flags, f32 amp);
s32 Shake_Update(u8 id);
void Shake_Stop(void);
s32 Shake_Value(u8 id);
void *Shake_GetOffset(u8 id);
s32 Shake_CopyOffset(u8 id, ShakeOffset *out);
u8 Shake_Flags(u8 id);

#endif
