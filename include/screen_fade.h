#ifndef SCREEN_FADE_H
#define SCREEN_FADE_H

#include "common.h"

/*
 * The two screen fades and the brightness option. FadeA_Start and
 * FadeB_Start run a fade from one of its keys (FADE_OUT_BLACK and the
 * others), FadeA_GetState and FadeB_GetState say whether it runs or holds
 * (FADE_STATE_*), ScreenFade_Task steps both each frame, and FadeA_Draw,
 * FadeB_Draw and Brightness_Draw draw them.
 */

/*
 * Each fade, gFadeA and gFadeB, runs one key of gFadeKeys at a time
 * (FadeA_Start(id), ids from 1). A key
 * is a coloured rectangle whose alpha (256 opaque) moves from alphaFrom to
 * alphaTo by alphaStep a frame, after `delay` frames; the fade then holds the
 * key, or ends after a key marked last.
 */
typedef struct FadeKey {
    u8 last;            /* 0x00: nonzero: the fade ends after this key */
    u8 color;           /* 0x01: an index into gFadeAColors / gFadeBColors (0 black, 7 white) */
    u8 unk02[2];        /* 0x02: the same two bytes in every key; nothing reads them */
    f32 x;              /* 0x04: the rectangle's corner */
    f32 y;              /* 0x08 */
    f32 z;              /* 0x0C */
    s16 w;              /* 0x10 */
    s16 h;              /* 0x12 */
    s16 alphaFrom;      /* 0x14 */
    s16 alphaTo;        /* 0x16 */
    s16 alphaStep;      /* 0x18: added each frame */
    s16 delay;          /* 0x1A: frames before the alpha moves */
} FadeKey;

/* A fade player (gFadeA, gFadeB). */
typedef struct FadeState {
    u8 active;          /* 0x00: FADE_STATE_* */
    u8 starting;        /* 0x01: set by FadeX_Start, cleared when the alpha reaches the key's end */
    u8 unk02[2];
    s16 id;             /* 0x04: the key asked for (from 1), 0 none */
    s16 cur;            /* 0x06: the key running */
    s16 alpha;          /* 0x08 */
    s16 prevAlpha;      /* 0x0A: the alpha of the frame before */
    FadeKey *key;       /* 0x0C: the key running, NULL none */
    s16 delay;          /* 0x10: frames left before the alpha moves */
} FadeState;

/* FadeState.active, and what FadeA_GetState and FadeB_GetState return. */
#define FADE_STATE_IDLE     0   /* no fade */
#define FADE_STATE_RUNNING  1   /* the alpha moving */
#define FADE_STATE_HELD     2   /* holding the key's end */

/* The rectangle FadeA_Draw gives Prim_FillRectF: two corners (x, y, z, 1)
 * and a colour. */
typedef struct FadeQuad {
    f32 p0[4];
    f32 p1[4];
    u32 color;
} FadeQuad;

/*
 * The keys of gFadeKeys (FadeA_Start, FadeB_Start), from the table: "out"
 * keys go to opaque and hold, "in" keys go back to clear and end the fade.
 * The frame counts are the ramp's length.
 */
#define FADE_OUT_BLACK          1   /* 16 frames */
#define FADE_IN_BLACK           2   /* 16 frames */
#define FADE_OUT_WHITE          3   /* 16 frames */
#define FADE_IN_WHITE           4   /* 16 frames */
#define FADE_OUT_BLACK_SLOW     5   /* 32 frames */
#define FADE_IN_BLACK_SLOW      6   /* 32 frames */
#define FADE_OUT_HALF_BLACK     7   /* to half opaque, 16 frames */
#define FADE_IN_HALF_BLACK      8   /* from half opaque, 16 frames */
#define FADE_OUT_WHITE_SLOW     9   /* 128 frames */
#define FADE_BLACK              10  /* opaque black at once, held */
#define FADE_OUT_BLACK_FAST     11  /* 8 frames */
#define FADE_IN_BLACK_FAST      12  /* 8 frames */
#define FADE_IN_WHITE_FAST      13  /* 4 frames */
#define FADE_IN_BAND            14  /* a black band 32 pixels high at y 246, 16 frames */
#define FADE_IN_BLACK_DELAYED   15  /* 16 frames, after 4 */

void ScreenFade_Task(u8 *task); /* runs as a task: steps both fades each frame */
void FadeA_Start(s32 id);
void FadeA_Reset(void);
u8 FadeA_GetState(void);
void FadeA_Draw(void);
void FadeB_Start(s32 id);
void FadeB_Reset(void);
u8 FadeB_GetState(void);
void FadeB_Draw(void);
void Brightness_Draw(void);

#endif
