#ifndef AREA_DRAW_H
#define AREA_DRAW_H

/*
 * The drawing of an area's models (sub_main): the area model with its
 * animated parts (Area_DrawModels, gAreaAnimTbls), its props (Area_DrawProp,
 * gAreaProps, and the village's gVillagePropChoices), the sky's gradient
 * (Sky_DrawGradient) and the buffers of the area atmosphere objects
 * (AreaAtmos_*). The animations' parameter blocks are area_anim.h's.
 *
 * Both lists carry an alpha test reference and flags per entry: the flags
 * pick when the entry shows (AREA_DRAW_*) and how its animation runs
 * (area_anim.h's AREA_ANIM_REVERSE, AREA_ANIM_ROT_X, AREA_ANIM_ROT_Z).
 */

#include "common.h"

/* AreaAnim.flags and PropRec.flags. */
#define AREA_DRAW_NOT_IN_EVENT  0x01    /* AreaAnim: skipped while an event scene runs */
#define AREA_DRAW_NO_ZWRITE     0x02    /* drawn without writing the depth buffer */
#define AREA_DRAW_DAY           0x08    /* fades out as night falls (alpha 0xFF less the night level) */
#define AREA_DRAW_NIGHT         0x10    /* fades in as night falls (alpha the night level) */

/* An animated part of the area model (gAreaAnimTbls, one list per area):
 * 8 bytes, sorted by model; the entries of one model follow each other. */
typedef struct AreaAnim {
    u8 model;                   /* 0x00: its entry in the area model's set */
    u8 alphaRef;                /* 0x01: the alpha test's reference (RS_ALPHA_REF) */
    u8 type;                    /* 0x02: AREA_MODEL_ANIM_* */
    u8 flags;                   /* 0x03: AREA_DRAW_*, and area_anim.h's AREA_ANIM_* */
    void *arg;                  /* 0x04: the animation's parameters (area_anim.h's blocks) */
} AreaAnim;

/* AreaAnim.type: how Area_DrawModels animates the part. */
#define AREA_MODEL_ANIM_NONE            0x00    /* drawn in place */
#define AREA_MODEL_ANIM_SPIN            0x01    /* AreaAnim_TranslateSpin */
#define AREA_MODEL_ANIM_TRANSLATE       0x02    /* AreaAnim_Translate */
#define AREA_MODEL_ANIM_HIDDEN          0x03    /* not drawn */
#define AREA_MODEL_ANIM_UV_SCROLL       0x04    /* AreaAnim_UvScroll */
#define AREA_MODEL_ANIM_UV_WAVE         0x05    /* AreaAnim_UvWave */
#define AREA_MODEL_ANIM_UV_SCROLL_SPIN  0x06    /* AreaAnim_UvScrollSpin */
#define AREA_MODEL_ANIM_UV_STEP         0x07    /* AreaAnim_UvStep */
#define AREA_MODEL_ANIM_UV_SCROLL_MOVE  0x08    /* AreaAnim_UvScrollTranslate */
#define AREA_MODEL_ANIM_SPIN_SCROLL     0x09    /* AreaAnim_SpinScroll */
#define AREA_MODEL_ANIM_MAT_ALPHAS      0x64    /* its first material opaque, its second clear */
#define AREA_MODEL_ANIM_UV_SCROLL_BLINK 0x65    /* scrolled, shown for 64 frames of each 256 */
#define AREA_MODEL_ANIM_NIGHT_SCROLL    0x66    /* scrolled, darkened by night (also 0x67) */
#define AREA_MODEL_ANIM_NIGHT_SCROLL_2  0x67
#define AREA_MODEL_ANIM_SPIN_TURN       0x68    /* spun, then turned by the angle after its AnimSpin (to 0x6B) */
#define AREA_MODEL_ANIM_SPIN_TURN_BIG   0x69    /* the same, scaled by 1.25 */
#define AREA_MODEL_ANIM_SPIN_TURN_2     0x6A
#define AREA_MODEL_ANIM_SPIN_TURN_3     0x6B
#define AREA_MODEL_ANIM_PULSE           0x6C    /* its colour pulses (green and blue) */

/* A prop of an area (gAreaProps, one list per area; the village's from
 * gVillagePropChoices): 12 bytes, the list ended by model PROP_END. */
typedef struct PropRec {
    u8 model;                   /* 0x00: its entry in the props' model set; PROP_END ends the list */
    u8 alphaRef;                /* 0x01: the alpha test's reference (RS_ALPHA_REF) */
    u8 type;                    /* 0x02: its animation: area_anim.h's AREA_ANIM_CMD_*, or PROP_ANIM_* */
    u8 flags;                   /* 0x03: AREA_DRAW_*, and area_anim.h's AREA_ANIM_* */
    void *data;                 /* 0x04: the animation's parameters, one block per copy */
    s16 count;                  /* 0x08: the copies drawn */
    u16 late;                   /* 0x0A: drawn after the others (two at most; more are drawn in turn) */
} PropRec;

#define PROP_END 0xFF

/* PropRec.type, besides area_anim.h's AREA_ANIM_CMD_* (1 to 12). The parameters
 * named are the f32 words of PropRec.data, per copy. */
#define PROP_ANIM_NONE          0x00    /* drawn in place */
#define PROP_ANIM_MOVING        0x0D    /* at pos[3] plus gAreaState.propOfs, rocking while propState runs */
#define PROP_ANIM_FLIPBOOK      0x64    /* its texture steps through 4 x 8 cells, forwards then back */
#define PROP_ANIM_PLACE_YZ      0x65    /* at pos[3], turned about y and z by two angles */
#define PROP_ANIM_NIGHT_GLOW    0x66    /* alpha 0.875 of the night level, plus a fast pulse */
#define PROP_ANIM_NIGHT_SPIN    0x67    /* darkened by night (Area_NightTint's colour), and spun (AnimSpin) */
#define PROP_ANIM_PULSE         0x68    /* alpha pulses between 0xA0 and 0xFF */
#define PROP_ANIM_SLIDE         0x69    /* at pos[3], sliding 2600 along z and back, holding between */
#define PROP_ANIM_SWAY          0x6A    /* at pos[3], swaying about z, turned about y */
#define PROP_ANIM_WHEEL         0x6B    /* copies spaced around y, turning with the frame counter; those
                                         * between 100 and 250 degrees are not drawn */
#define PROP_ANIM_PLACE_XYZ     0x6C    /* at pos[3], turned about x, y and z */
#define PROP_ANIM_FLICKER       0x6D    /* at pos[3], stretched along z by a sum of sines, swaying about y */
#define PROP_ANIM_FLICKER_2     0x6E    /* the same, with other frequencies */
#define PROP_ANIM_DAY_SCROLL    0x6F    /* AreaAnim_UvScrollTranslate, half faded by night */
#define PROP_ANIM_NIGHT_GLOW_2  0x70    /* 0x70 to 0x72: as PROP_ANIM_NIGHT_GLOW */
#define PROP_ANIM_NIGHT_GLOW_3  0x71
#define PROP_ANIM_NIGHT_GLOW_4  0x72
#define PROP_ANIM_NIGHT_GLOW_5  0x73    /* 0x73 and 0x74: alpha 0.81 of the night level, plus a slower pulse */
#define PROP_ANIM_NIGHT_GLOW_6  0x74
#define PROP_ANIM_FACILITY      0x75    /* drawn when the village's facility level 0 is the s16 in data */
#define PROP_ANIM_VARIANT       0x76    /* drawn when the area's variant (gAreaState.look[1]) is the byte in data */
#define PROP_ANIM_KUSHALA_WIND  0x77    /* AreaAnim_UvScroll, drawn only while a Kushala Daora is in the field */

/* The village's props by its facilities (gVillagePropChoices's lists). */
typedef struct PropChoice {
    u16 mask;                   /* 0x00: bit n: for model set n; bit 8 + l: for facility level l */
    u8 pad02[2];
    PropRec *props;             /* 0x04 */
} PropChoice;

/* An area's prop choices (gVillagePropChoices, ended by area 0xFFFF). */
typedef struct AreaPropChoices {
    u16 area;                   /* 0x00: the area id; 0xFFFF ends the table */
    s16 count;                  /* 0x02: the choices */
    PropChoice *choices;        /* 0x04 */
} AreaPropChoices;

/* The props held back to be drawn last (Area_DrawModels). */
typedef struct PropDefer {
    PropRec *prop[2];
    u8 set[2];                  /* their model sets (gAreaSubModels) */
} PropDefer;

/* A step of an area's day table (Sky_GetGradient): 8 steps. */
typedef struct DayStep {
    s32 type;                   /* 0x0: a row of gDayStepTimes (below DAY_STEP_TIMES), or DAY_STEP_BEFORE_NEXT */
    u32 top;                    /* 0x4: the sky's top colour */
    u32 bottom;                 /* 0x8: its bottom colour */
} DayStep;

#define DAY_STEP_COUNT      8
#define DAY_STEP_TIMES      4   /* the rows of gDayStepTimes: types 0-3 */
#define DAY_STEP_BEFORE_NEXT 5  /* a moment before the next step's time; other types: a moment after
                                 * the previous step's (DayTable_GetTime) */

/* A scrolled layer of an area atmosphere object (types 8 and 14). */
typedef struct AtmosLayer {
    f32 scroll[3];              /* 0x00: [1] the texture's v offset, kept in 0-1; [0] and [2] stay 0 */
    f32 speed;                  /* 0x0C: its v per frame, eased to AreaAtmos_LayerSpeed */
    f32 alpha;                  /* 0x10: eased to AreaAtmos_LayerAlpha */
} AtmosLayer;

/* A texture offset of an area atmosphere object's layer (AtmosScroll). */
typedef struct AtmosUv {
    f32 u;                      /* 0x0 */
    f32 v;                      /* 0x4 */
    f32 alpha;                  /* 0x8: eased to AreaAtmos_LayerAlpha (types 10 and 13) */
} AtmosUv;

/* The buffer of the area atmosphere objects of types 10, 11 and 13: their
 * layers' textures follow the camera's turns and climbs (AtmosScroll_Update). */
typedef struct AtmosScroll {
    u16 pitch;                  /* 0x00: the camera's last pitch (Vec_DirToAngles) */
    u16 yaw;                    /* 0x02: and its last yaw */
    f32 camPos[3];              /* 0x04: and its last position */
    AtmosUv layer[1];           /* 0x10: one per layer (gAreaAtmosLayerCounts) */
} AtmosScroll;

#endif /* AREA_DRAW_H */
