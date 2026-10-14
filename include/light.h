#ifndef LIGHT_H
#define LIGHT_H

#include "common.h"

/*
 * A light (0x68 bytes). Gs_SetState's registers 0x5A-0x5C copy one to gRsLights (main
 * unk_183D20.c); gLightSets holds two sets of three (sub_main unk_331A40.c: LightSet).
 */
typedef struct Light {
    s32 unk00;          /* 0x00: 0 (Light_InitAll) */
    f32 amb[4];         /* 0x04: the ambient colour; Gs_SetState zeroes [3] */
    f32 unk14[4];       /* 0x14: 1.0 in set 1 (Light_InitAll); Gs_SetState zeroes [3] */
    f32 col[3];         /* 0x24: the diffuse colour */
    u8 unk30[0x34 - 0x30];
    f32 dir[3];         /* 0x34: the direction, normalized by Gs_SetState */
    f32 pos[3];         /* 0x40 */
    u8 unk4C[0x50 - 0x4C];
    f32 unk50;          /* 0x50: Gs_SetState keeps its reciprocal */
    u8 unk54[0x68 - 0x54];
} Light;
typedef char Light_size_check[(sizeof(Light) == 0x68) ? 1 : -1];

/* A light set (0x140 bytes; gLightSets + 0x10 + n * 0x140, n 0 or 1): LightSet_Apply hands its
 * three lights to Gs_SetState. */
typedef struct LightSet {
    u8 state;           /* 0x00: the mode's step (LightSet_UpdateFade: 0 start, 1 fading, 2 end) */
    u8 mode;            /* 0x01: 1 a fade (LightSet_UpdateAll runs LightSet_UpdateFade); the fade's end clears it */
    u8 frames;          /* 0x02: the fade's length (30) */
    u8 left;            /* 0x03: the fade's frames left */
    u8 pad04[4];
    Light lights[3];    /* 0x08 */
} LightSet;
typedef char LightSet_size_check[(sizeof(LightSet) == 0x140) ? 1 : -1];

#endif /* LIGHT_H */
