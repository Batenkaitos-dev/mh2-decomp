#ifndef AREA_ANIM_H
#define AREA_ANIM_H

/*
 * The parameter blocks of the area animations. The AreaAnim_* functions (sub_main
 * area_anim.c) build a model or texture matrix from one; the area tables of area_draw.c
 * point to them by animation type.
 * - PropRec.data (Area_DrawProp, one block per copy): 1 AnimSpin, 2 UvScroll, 3 UvWave[2],
 *   4 f32 pos[3], 5 AnimRot, 6 AnimRotScale, 7 f32 x and z, 8 and 0x6F AnimScrollPos,
 *   9 AnimScrollSpin, 10 AnimScrollRot, 11 AnimSpin2, 12 UvStep[2].
 * - AreaAnim.arg (Area_DrawModels): 1 AnimSpin, 2 f32 pos[3], 4 UvScroll, 5 UvWave[2],
 *   6 AnimScrollSpin, 7 UvStep[2], 8 AnimScrollPos, 9 AnimSpin2.
 */

#include "common.h"

/* A position and a fixed rotation angle (radians). */
typedef struct AnimRot {
    f32 pos[3];                 /* 0x0 */
    f32 angle;                  /* 0xC */
} AnimRot;
typedef char AnimRot_size_check[(sizeof(AnimRot) == 0x10) ? 1 : -1];

/* A position and a rotation by the frame counter: the angle is the counter
 * masked to 16 - shift bits, shifted up (so a turn takes 0x10000 >> shift
 * frames). */
typedef struct AnimSpin {
    f32 pos[3];                 /* 0x0 */
    s16 shift;                  /* 0xC */
} AnimSpin;
typedef char AnimSpin_size_check[(sizeof(AnimSpin) == 0x10) ? 1 : -1];

/* The same for AreaAnim_SpinScroll, which uses the counter at +0xA. */
typedef struct AnimSpin2 {
    f32 pos[3];                 /* 0x0 */
    u16 shift;                  /* 0xC */
} AnimSpin2;
typedef char AnimSpin2_size_check[(sizeof(AnimSpin2) == 0x10) ? 1 : -1];

typedef struct AnimRotScale {
    AnimRot *rot;               /* 0x0 */
    f32 scale[3];               /* 0x4 */
} AnimRotScale;
typedef char AnimRotScale_size_check[(sizeof(AnimRotScale) == 0x10) ? 1 : -1];

/* A texture scroll: the period of u and v in frames (powers of two; 0:
 * none). */
typedef struct UvScroll {
    u16 period[2];              /* 0x0 */
} UvScroll;
typedef char UvScroll_size_check[(sizeof(UvScroll) == 0x4) ? 1 : -1];

/* A texture wave (one per coordinate): base + amp * sin(counter). */
typedef struct UvWave {
    u16 period;                 /* 0x0: frames, a power of two (0: none) */
    u16 speed;                  /* 0x2: angle units (0x10000 a turn) per frame */
    f32 base;                   /* 0x4 */
    f32 amp;                    /* 0x8 */
} UvWave;
typedef char UvWave_size_check[(sizeof(UvWave) == 0xC) ? 1 : -1];

/* A texture step (one per coordinate): step * ((counter >> shift) & mask),
 * a flip-book. */
typedef struct UvStep {
    u16 shift;                  /* 0x0 */
    u16 mask;                   /* 0x2 */
    f32 step;                   /* 0x4 */
} UvStep;
typedef char UvStep_size_check[(sizeof(UvStep) == 0x8) ? 1 : -1];

typedef struct AnimScrollSpin {
    UvScroll *uv;               /* 0x0 */
    AnimSpin *spin;             /* 0x4 */
} AnimScrollSpin;
typedef char AnimScrollSpin_size_check[(sizeof(AnimScrollSpin) == 0x8) ? 1 : -1];

typedef struct AnimScrollPos {
    UvScroll *uv;               /* 0x0 */
    f32 *pos;                   /* 0x4 */
} AnimScrollPos;
typedef char AnimScrollPos_size_check[(sizeof(AnimScrollPos) == 0x8) ? 1 : -1];

typedef struct AnimScrollRot {
    UvScroll *uv;               /* 0x0 */
    AnimRot *rot;               /* 0x4 */
} AnimScrollRot;
typedef char AnimScrollRot_size_check[(sizeof(AnimScrollRot) == 0x8) ? 1 : -1];

/* An animated part's command byte (Area_DrawProp, record +2): the function
 * that builds its matrix (area_anim.c). */
#define AREA_ANIM_CMD_SPIN              1   /* AreaAnim_TranslateSpin */
#define AREA_ANIM_CMD_UV_SCROLL         2   /* AreaAnim_UvScroll */
#define AREA_ANIM_CMD_UV_WAVE           3   /* AreaAnim_UvWave */
#define AREA_ANIM_CMD_TRANSLATE         4   /* AreaAnim_Translate */
#define AREA_ANIM_CMD_ROTATE            5   /* AreaAnim_TranslateRotate */
#define AREA_ANIM_CMD_SCALE_ROTATE      6   /* AreaAnim_ScaleTranslateRotate */
#define AREA_ANIM_CMD_TRANSLATE_XZ      7   /* AreaAnim_TranslateXZ */
#define AREA_ANIM_CMD_UV_SCROLL_MOVE    8   /* AreaAnim_UvScrollTranslate */
#define AREA_ANIM_CMD_UV_SCROLL_SPIN    9   /* AreaAnim_UvScrollSpin */
#define AREA_ANIM_CMD_UV_SCROLL_ROTATE  10  /* AreaAnim_UvScrollRotate */
#define AREA_ANIM_CMD_SPIN_SCROLL       11  /* AreaAnim_SpinScroll */
#define AREA_ANIM_CMD_UV_STEP           12  /* AreaAnim_UvStep */

/* An animated part's flags byte (record +3). */
#define AREA_ANIM_REVERSE   0x04    /* the motion runs backwards */
#define AREA_ANIM_ROT_X     0x40    /* turns about x (else y) */
#define AREA_ANIM_ROT_Z     0x80    /* turns about z */

#endif /* AREA_ANIM_H */
