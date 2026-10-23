#ifndef LIGHT_H
#define LIGHT_H

#include "common.h"
#include "player.h"

/*
 * A light (104 bytes). Gs_SetState's states RS_LIGHT(0) to RS_LIGHT(2) copy one to gRsLights;
 * gLightSets holds two sets of three (LightSet). The VU1 programs light a vertex
 * with the three lights' colours, each scaled by its N.L, plus an ambient colour,
 * the sum of the three lights' ambient parts (Vu1_PutLightColors).
 */
typedef struct Light {
    s32 unk00;          /* 0x00: 0 (Light_InitAll) */
    f32 color[4];       /* 0x04: the light's colour, which the VU1 programs scale by N.L; Gs_SetState zeroes [3] */
    f32 unk14[4];       /* 0x14: 1.0 in set 1 (Light_InitAll); Gs_SetState zeroes [3] */
    f32 ambient[3];     /* 0x24: the light's part of the ambient colour */
    u8 unk30[0x34 - 0x30];
    f32 dir[3];         /* 0x34: the direction, normalized by Gs_SetState */
    f32 pos[3];         /* 0x40 */
    u8 unk4C[0x50 - 0x4C];
    f32 unk50;          /* 0x50: Gs_SetState keeps its reciprocal */
    u8 unk54[0x68 - 0x54];
} Light;
typedef char Light_size_check[(sizeof(Light) == 0x68) ? 1 : -1];

/* The lights of a LightSet. */
#define LIGHT_SET_LIGHTS 3

/* The length of a light set's fade (LightSet_UpdateFade), in frames. */
#define LIGHT_FADE_FRAMES 30

/* LightSet.mode: what LightSet_UpdateAll runs for the set each frame. */
enum LightSetMode {
    LIGHT_MODE_NONE = 0,    /* nothing */
    LIGHT_MODE_FADE = 1     /* a fade (LightSet_UpdateFade), whose end sets LIGHT_MODE_NONE again */
};

/* LightSet.state in LIGHT_MODE_FADE: LightSet_UpdateFade's steps. */
enum LightFadeStep {
    LIGHT_FADE_START = 0,   /* sets the fade's length */
    LIGHT_FADE_RUN = 1,     /* fades light 0 down to its target, one frame a call */
    LIGHT_FADE_END = 2      /* ends the mode */
};

/* A light set (320 bytes; gLightSets.sets[n], n 0 or 1): LightSet_Apply hands its
 * three lights to Gs_SetState. */
typedef struct LightSet {
    u8 state;           /* 0x00: the mode's step (enum LightFadeStep) */
    u8 mode;            /* 0x01: enum LightSetMode */
    u8 frames;          /* 0x02: the fade's length (LIGHT_FADE_FRAMES) */
    u8 left;            /* 0x03: the fade's frames left */
    u8 pad04[4];
    Light lights[LIGHT_SET_LIGHTS]; /* 0x08 */
} LightSet;
typedef char LightSet_size_check[(sizeof(LightSet) == 0x140) ? 1 : -1];

/* The two light sets (656 bytes), after a head of 16 bytes that nothing reads; Light_InitAll and
 * the scene's start clear the whole. */
typedef struct LightSets {
    u8 unk00[0x10];
    LightSet sets[2];   /* 0x10: set 0 and set 1 */
} LightSets;
typedef char LightSets_size_check[(sizeof(LightSets) == 0x290) ? 1 : -1];

/* The tables a light set is loaded from: for each of its three lights, the position, the
 * direction, the colour, the ambient part and the value of Light.unk14 (Light_InitAll;
 * LightSet_UpdateFade reads the colour and the ambient part of the first). */
typedef struct LightSources {
    f32 (*pos)[3];
    f32 (*dir)[3];
    f32 (*color)[4];
    f32 (*ambient)[4];
    f32 (*unk14)[4];
} LightSources;

void Light_InitAll(s32 unused0);
void LightSet_UpdateAll(void);
void LightSet_UpdateFade(LightSet *set);
void LightSet_SetColors(void *obj, s32 n);
void LightSet_ResetColors(s32 setIndex);
void LightSet_Apply(s32 setIndex);
void Player_ClearLightColors(Player *pl);
void LightSet_FadeColors(void *obj);

#endif /* LIGHT_H */
