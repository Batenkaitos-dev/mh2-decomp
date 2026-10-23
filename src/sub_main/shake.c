/*
 * The HUD's shake: Shake_Start starts it and Shake_Update runs it, a frame at a
 * time. It moves one element at a time, owned by an id (gShakeId; SHAKE_NONE
 * when no shake runs), by an offset along x or y; the queries after them tell
 * the menus and the HUD whether their id shakes and by how much.
 */
#include "common.h"
#include "shake.h"
#include "hud.h"
#include "render_state.h"
#include "model.h"
#include "text.h"
#include "engine_math.h"

extern ShakeOffset gShakeOffset;
extern u8 gShakeId;
extern u8 gShakeFlags;
extern s8 gShakeOn;
extern u8 gShakeZeroOffset[];
extern f32 gShakeAmp;
extern s8 gShakeTimer;

/* Starts a shake of the HUD element id (gShakeOffset) for SHAKE_FRAMES
 * frames along the axis in flags (SHAKE_AXIS_*): it eases from amp back
 * to 0, or with SHAKE_SLIDE it slides from 0 out to amp and stays. */
void Shake_Start(u8 id, u8 flags, f32 amp)
{
    gShakeId = id;
    *(s32 *)&gShakeOffset = 0; /* both halves as one word, as the game clears it */
    gShakeOn = 1;
    gShakeTimer = SHAKE_FRAMES;
    gShakeFlags = flags;
    gShakeAmp = amp;
    if (flags & SHAKE_SLIDE) {
        amp = 0.0f;
    }
    switch (flags & SHAKE_AXIS_MASK) {
    case SHAKE_AXIS_X:
        gShakeOffset.x = amp;
        break;
    case SHAKE_AXIS_Y:
        gShakeOffset.y = amp;
        break;
    }
}

/* Steps the shake of element id: returns 1 while it moves. When it ends,
 * the offset goes back to 0 (or, after a slide, the shake is let go). */
s32 Shake_Update(u8 id)
{
    f32 progress;

    if (gShakeId == id) {
        if (gShakeTimer != 0) {
            progress = 0.16666667f * gShakeTimer /* 1 / SHAKE_FRAMES */;
            if ((gShakeFlags & SHAKE_SLIDE) == 0) {
                progress = Math_Pow(progress, 2.25f);
            } else {
                progress = 1.0f - progress;
            }
            switch (gShakeFlags & SHAKE_AXIS_MASK) {
            case SHAKE_AXIS_X:
                gShakeOffset.x = gShakeAmp * progress;
                break;
            case SHAKE_AXIS_Y:
                gShakeOffset.y = gShakeAmp * progress;
                break;
            }
            gShakeTimer--;
            return 1;
        }
        if (gShakeOn != 0) {
            if ((gShakeFlags & SHAKE_SLIDE) == 0) {
                switch (gShakeFlags & SHAKE_AXIS_MASK) {
                case SHAKE_AXIS_X:
                    gShakeOffset.x = 0;
                    break;
                case SHAKE_AXIS_Y:
                    gShakeOffset.y = 0;
                    break;
                }
            } else {
                gShakeId = SHAKE_NONE;
            }
            gShakeOn = 0;
        }
    }
    return 0;
}

void Shake_Stop(void)
{
    gShakeId = SHAKE_NONE;
}

/* Whether id's shake runs (gShakeOn), 0 when another id has it. */
s32 Shake_Value(u8 id)
{
    if (gShakeId == id) {
        return gShakeOn;
    }
    return 0;
}

/* id's offset, or a zero offset when another id has the shake. */
void *Shake_GetOffset(u8 id)
{
    if (gShakeId == id) {
        return &gShakeOffset;
    }
    return gShakeZeroOffset;
}

/* Copies id's offset to out and returns 1, or zeroes out and returns 0
 * when another id has the shake. */
s32 Shake_CopyOffset(u8 id, ShakeOffset *out)
{
    if (gShakeId == id) {
        *out = gShakeOffset;
        return 1;
    }
    out->y = 0;
    out->x = 0;
    return 0;
}

/* id's shake flags (Shake_Start's: the axis in the low bits), 0 when
 * another id has the shake. */
u8 Shake_Flags(u8 id)
{
    if (gShakeId == id) {
        return gShakeFlags;
    }
    return 0;
}
