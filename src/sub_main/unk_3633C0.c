/*
 * The shake's queries (sub_main 0x3633C0-0x363550). A shake (Shake_Start
 * and Shake_Update in unk_361570.c) moves one element at a time, owned by
 * an id (gShakeId; SHAKE_NONE when no shake runs), by an offset along x or
 * y; the menus and the HUD ask whether their id shakes and by how much.
 * Then a few small helpers: an empty function of the keyboard's start,
 * binding texture 0x148, a blend setting and the keyboard's start flag.
 */
#include "common.h"
#include "shake.h"

extern ShakeOffset gShakeOffset;
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

/* An empty function Kbd_InitState calls. */
void Kbd_EmptyInit(void)
{
}

/* Prepares and binds texture 0x148. */
void Tex_Bind148(void)
{
    Tex_Prepare(1, 0x148);
    Tex_Bind(0x148);
}

/* Sets the blend: Gfx_SetBlend(4, 1) when mode is 1, else (4, 5). */
void Gfx_SetBlendAlphaAdd(s32 mode)
{
    if (mode != 1) {
        Gfx_SetBlend(4, 5);
    } else {
        Gfx_SetBlend(4, 1);
    }
}

/* When set, any key makes the software keyboard active (unk_314E00.c). */
void Kbd_SetStartActive(u8 active)
{
    gKbdStartActive = active;
}

s32 Kbd_GetStartActive(void)
{
    return gKbdStartActive;
}
