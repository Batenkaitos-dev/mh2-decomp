/*
 * The two screen fades and the brightness option. ScreenFade_Task steps both
 * fades, gFadeA and gFadeB, every frame: each runs through its keys in
 * gFadeKeys from the one FadeA_Start or FadeB_Start gave it, and its state
 * (FADE_STATE_*) tells the lobby and the game when it is done.
 * FadeA_Draw and FadeB_Draw draw each fade's rectangle in its colour
 * (gFadeAColors, gFadeBColors); Brightness_Draw lightens or darkens the
 * whole screen by the brightness option.
 */
#include "common.h"
#include "syssettings.h"
#include "em.h"
#include "scene.h"
#include "prim.h"
#include "task.h"
#include "render_state.h"
#include "screen_fade.h"
#include "text.h"
#include "libc.h"
struct PrimLine;
/* A filled screen rectangle (Prim_FillRect). */
typedef struct ScreenRect {
    s16 x, y, w, h;
    u32 color;
} ScreenRect;
extern FadeKey gFadeKeys[];
extern FadeState gFadeB;
extern FadeState gFadeA;
/* end of prototypes */

/*
 * The task (started by select's boot) that steps the two screen fades, gFadeA
 * and gFadeB, every frame: the task's mode is gFadeA's step, its step
 * gFadeB's (0 clears the fade, 1 runs it). A fade starts at its key `id` (set
 * by FadeX_Start), waits the key's delay, moves its alpha to the key's
 * alphaTo, then goes on to the next key or, after the last, stops. A new id
 * restarts it from that key. The goto keeps the restart inside the switch,
 * as the game's code has it.
 */
void ScreenFade_Task(u8 *task)
{
    TaskSlot *slot = (TaskSlot *)task;
    FadeState *fade;
    FadeKey *key;

    fade = &gFadeA;
    switch (slot->mode) {
    case 0:
        memset(fade, 0, sizeof(FadeState));
        fade->key = NULL;
        slot->mode++;
    case 1:
        switch (fade->active) {
        case FADE_STATE_IDLE:
            if (fade->id == 0) {
                break;
            }
        start:
            fade->active = FADE_STATE_RUNNING;
            fade->cur = fade->id;
            fade->alpha = gFadeKeys[fade->cur - 1].alphaFrom;
            fade->delay = gFadeKeys[fade->cur - 1].delay;
        case FADE_STATE_RUNNING:
        case FADE_STATE_HELD:
            if (fade->id == 0) {
                fade->active = FADE_STATE_IDLE;
                fade->key = NULL;
                break;
            }
            if (fade->id != fade->cur) {
                goto start;
            }
            key = &gFadeKeys[fade->cur - 1];
            fade->key = key;
            fade->prevAlpha = fade->alpha;
            if (fade->alpha != key->alphaTo) {
                if (fade->delay == 0) {
                    fade->alpha += key->alphaStep;
                } else {
                    fade->delay--;
                }
                break;
            }
            fade->starting = 0;
            if (key->last != 0) {
                fade->active = FADE_STATE_IDLE;
                fade->id = 0;
                fade->key = NULL;
                break;
            }
            if (fade->active == FADE_STATE_RUNNING) {
                fade->active++; /* to FADE_STATE_HELD */
            }
            break;
        }
        break;
    }
    fade = &gFadeB;
    switch (slot->step) {
    case 0:
        memset(fade, 0, sizeof(FadeState));
        fade->key = NULL;
        slot->step++;
    case 1:
        switch (fade->active) {
        case FADE_STATE_IDLE:
            if (fade->id == 0) {
                break;
            }
        start2:
            fade->active = FADE_STATE_RUNNING;
            fade->cur = fade->id;
            fade->alpha = gFadeKeys[fade->cur - 1].alphaFrom;
            fade->delay = gFadeKeys[fade->cur - 1].delay;
        case FADE_STATE_RUNNING:
        case FADE_STATE_HELD:
            if (fade->id == 0) {
                fade->active = FADE_STATE_IDLE;
                fade->key = NULL;
                break;
            }
            if (fade->id != fade->cur) {
                goto start2;
            }
            key = &gFadeKeys[fade->cur - 1];
            fade->key = key;
            fade->prevAlpha = fade->alpha;
            if (fade->alpha != key->alphaTo) {
                if (fade->delay == 0) {
                    fade->alpha += key->alphaStep;
                } else {
                    fade->delay--;
                }
                break;
            }
            fade->starting = 0;
            if (key->last != 0) {
                fade->active = FADE_STATE_IDLE;
                fade->id = 0;
                fade->key = NULL;
                break;
            }
            if (fade->active == FADE_STATE_RUNNING) {
                fade->active++; /* to FADE_STATE_HELD */
            }
            break;
        }
        break;
    }
}


extern u32 gFadeAColors[];
extern u32 gFadeBColors[];
struct PrimLineF;

/*
 * gFadeA's state: FADE_STATE_IDLE, FADE_STATE_RUNNING while its alpha moves
 * (or before a key that isn't the last ends its first pass), FADE_STATE_HELD
 * when it holds its last alpha. The lobby and the game wait for it to leave
 * FADE_STATE_RUNNING.
 */
u8 FadeA_GetState(void)
{
    FadeState *fade = &gFadeA;
    FadeKey *key;

    if ((fade->active == FADE_STATE_IDLE && fade->starting == 0) || fade->id == 0) {
        return FADE_STATE_IDLE;
    }
    key = &gFadeKeys[fade->cur - 1];
    if (fade->alpha != key->alphaTo || (key->last == 0 && fade->active == FADE_STATE_RUNNING)) {
        return FADE_STATE_RUNNING;
    }
    return FADE_STATE_HELD;
}

/* The same for gFadeB. */
u8 FadeB_GetState(void)
{
    FadeState *fade = &gFadeB;
    FadeKey *key;

    if ((fade->active == FADE_STATE_IDLE && fade->starting == 0) || fade->id == 0) {
        return FADE_STATE_IDLE;
    }
    key = &gFadeKeys[fade->cur - 1];
    if (fade->alpha != key->alphaTo || (key->last == 0 && fade->active == FADE_STATE_RUNNING)) {
        return FADE_STATE_RUNNING;
    }
    return FADE_STATE_HELD;
}

/* Draws gFadeA's current key: its rectangle in its colour (gFadeAColors) at
 * the fade's alpha. */
void FadeA_Draw(void)
{
    FadeState *fade = &gFadeA;
    f32 alpha;
    FadeQuad quad;

    if (fade->id == 0) {
        return;
    }
    if (fade->key == NULL) {
        return;
    }
    alpha = (f32)fade->alpha / 256.0f;
    if (alpha <= 0.0f) {
        return;
    }
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
    quad.p0[0] = fade->key->x;
    quad.p0[1] = fade->key->y;
    quad.p0[2] = fade->key->z;
    quad.p0[3] = 1.0f;
    quad.p1[0] = quad.p0[0] + fade->key->w;
    quad.p1[1] = quad.p0[1] + fade->key->h;
    quad.p1[2] = quad.p0[2];
    quad.p1[3] = 1.0f;
    quad.color = ((u32)(255.0f * alpha) << 24 & 0xFF000000) | (gFadeAColors[fade->key->color] & 0xFFFFFF);
    Prim_FillRectF((struct PrimLineF *)&quad);
    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
}

/* The same for gFadeB (gFadeBColors). */
void FadeB_Draw(void)
{
    FadeState *fade = &gFadeB;
    f32 alpha;
    FadeQuad quad;

    if (fade->id == 0) {
        return;
    }
    if (fade->key == NULL) {
        return;
    }
    alpha = (f32)fade->alpha / 256.0f;
    if (alpha <= 0.0f) {
        return;
    }
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Gs_SetState(RS_ZTEST, RS_CMP_ALWAYS);
    quad.p0[0] = fade->key->x;
    quad.p0[1] = fade->key->y;
    quad.p0[2] = fade->key->z;
    quad.p0[3] = 1.0f;
    quad.p1[0] = quad.p0[0] + fade->key->w;
    quad.p1[1] = quad.p0[1] + fade->key->h;
    quad.p1[2] = quad.p0[2];
    quad.p1[3] = 1.0f;
    quad.color = ((u32)(255.0f * alpha) << 24 & 0xFF000000) | (gFadeBColors[fade->key->color] & 0xFFFFFF);
    Prim_FillRectF((struct PrimLineF *)&quad);
    Gs_SetState(RS_ZTEST, RS_CMP_LEQUAL);
}

/*
 * Starts gFadeA at key `id` (FADE_*). prevAlpha holds the first
 * ramp's length in frames until Fade_Task's first step overwrites it.
 */
void FadeA_Start(s32 id)
{
    FadeState *fade = &gFadeA;
    FadeKey *key;
    s32 range;
    s32 step;

    fade->id = id;
    key = &gFadeKeys[fade->id - 1];
    range = key->alphaTo - key->alphaFrom;
    if (range < 0) {
        range = -range;
    }
    step = key->alphaStep;
    if (step < 0) {
        step = -step;
    }
    fade->prevAlpha = range / step;
    fade->cur = fade->id;
    fade->alpha = gFadeKeys[fade->cur - 1].alphaFrom;
    fade->delay = gFadeKeys[fade->cur - 1].delay;
    fade->starting = 1;
}

/* Stops gFadeA at once. */
void FadeA_Reset(void)
{
    gFadeA.active = FADE_STATE_IDLE;
    gFadeA.starting = 0;
    gFadeA.id = 0;
    gFadeA.key = NULL;
    gFadeA.prevAlpha = 0;
}

/* The same as FadeA_Start, for gFadeB. */
void FadeB_Start(s32 id)
{
    FadeState *fade = &gFadeB;
    FadeKey *key;
    s32 range;
    s32 step;

    fade->id = id;
    key = &gFadeKeys[fade->id - 1];
    range = key->alphaTo - key->alphaFrom;
    if (range < 0) {
        range = -range;
    }
    step = key->alphaStep;
    if (step < 0) {
        step = -step;
    }
    fade->prevAlpha = range / step;
    fade->cur = fade->id;
    fade->alpha = gFadeKeys[fade->cur - 1].alphaFrom;
    fade->delay = gFadeKeys[fade->cur - 1].delay;
    fade->starting = 1;
}

/* Stops gFadeB at once. */
void FadeB_Reset(void)
{
    gFadeB.active = FADE_STATE_IDLE;
    gFadeB.starting = 0;
    gFadeB.id = 0;
    gFadeB.key = NULL;
    gFadeB.prevAlpha = 0;
}

/*
 * Draws the brightness option over the whole screen (640 x 448): above the
 * neutral 128 a white rectangle of alpha brightness - 128, below it a black
 * one of alpha (128 - brightness) * 2, each with its own GS blending
 * (RS_BLEND); RS_BLEND_OP and RS_BLEND are put back after.
 */
void Brightness_Draw(void)
{
    s32 dark;
    s32 alphaLevel;
    s32 level;
    s32 savedAlpha;
    s32 savedTest;
    ScreenRect rect;

    if (gSysSettings.brightness == 128) {
        return;
    }
    if (gSysSettings.brightness > 128U) {
        alphaLevel = gSysSettings.brightness - 128;
        dark = 0;
        level = 255;
    } else {
        alphaLevel = 128 - gSysSettings.brightness;
        dark = 1;
        alphaLevel *= 2;
        level = 0;
    }
    if (alphaLevel == 0) {
        return;
    }
    rect.w = 640;
    rect.y = 0;
    rect.h = 448;
    rect.x = 0;
    rect.color = (alphaLevel << 24) | (level << 16) | (level << 8) | level;
    savedAlpha = Gs_GetState(RS_BLEND_OP);
    savedTest = Gs_GetState(RS_BLEND);
    Gs_SetState(RS_BLEND_OP, RF_BLEND_OP_ADD);
    if (dark == 0) {
        Gs_SetState(RS_BLEND, RS_FACTOR_SRC_ALPHA | RS_BLEND_DST(RS_FACTOR_ONE));
    } else {
        Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    }
    Prim_FillRect((struct PrimLine *)&rect);
    Gs_SetState(RS_BLEND_OP, savedAlpha);
    Gs_SetState(RS_BLEND, savedTest);
}

/* The file's initialised data, in the game's order */
#pragma explicit_zero_data on
u32 D_0041CFD8[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
FadeKey gFadeKeys[15] = {
#include "sub_main/0041CFE0.inc"
};
u32 gFadeAColors[12] = {
#include "sub_main/0041D190.inc"
};
u32 gFadeBColors[12] = {
#include "sub_main/0041D1C0.inc"
};
