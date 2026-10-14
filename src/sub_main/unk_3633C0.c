/*
 * Queries on a record owned by one id (gShakeId, 0xFF = none), and a few
 * small helpers after it. Semantics not yet known.
 */
#include "common.h"

typedef struct Pair {
    s16 x;
    s16 y;
} Pair;

extern Pair gShakeOffset;
extern u8 gShakeId;
extern u8 gShakeFlags;
extern s8 gShakeOn;
extern u8 gShakeZeroOffset[];
extern s8 gKbdStartActive;

void Tex_Prepare(s32 a, s32 b);
void Tex_Bind(s32 a);
void Gfx_SetBlend(s32 a, s32 b);

void Shake_Stop(void)
{
    gShakeId = 0xFF;
}

s32 Shake_Value(u8 id)
{
    if (gShakeId == id) {
        return gShakeOn;
    }
    return 0;
}

void *Shake_GetOffset(u8 id)
{
    if (gShakeId == id) {
        return &gShakeOffset;
    }
    return gShakeZeroOffset;
}

s32 Shake_CopyOffset(u8 id, Pair *out)
{
    if (gShakeId == id) {
        *out = gShakeOffset;
        return 1;
    }
    out->y = 0;
    out->x = 0;
    return 0;
}

u8 Shake_Flags(u8 id)
{
    if (gShakeId == id) {
        return gShakeFlags;
    }
    return 0;
}

void func_003634B0(void)
{
}

void Tex_Bind148(void)
{
    Tex_Prepare(1, 0x148);
    Tex_Bind(0x148);
}

void Gfx_SetBlendAlphaAdd(s32 a)
{
    if (a != 1) {
        Gfx_SetBlend(4, 5);
    } else {
        Gfx_SetBlend(4, 1);
    }
}

void Kbd_SetStartActive(u8 v)
{
    gKbdStartActive = v;
}

s32 Kbd_GetStartActive(void)
{
    return gKbdStartActive;
}
