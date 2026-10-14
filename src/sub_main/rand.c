/*
 * The game's random number generator, with a few sound helpers that share
 * its address range (0x368B00-0x368F10).
 *
 * Rand_Next(stream) is a Lehmer generator on 16-bit state:
 *     x = (x ? x : 1) * 176 mod 65363 (0xFF53)
 * returning the new state (1..65362). Stream 1 is the general one (about
 * 950 call sites across the modules); stream 0 is used about 25 times, e.g.
 * by the sound code's random variations. Rand_Seed sets streams 0 and 1 to
 * the same byte; the select screen calls it with the result of
 * func_003DDBA0, which always returns 0, so both streams start from 1.
 * Another 32 streams (Rand_NextSaved) are copied into the save data.
 */
#include "common.h"
#include "scene.h"
#include "save.h"

extern u8 gSoundEffectSet[2];        /* per SPU core: effect already set */
extern u16 gBgmId;
extern u16 gRandSeeds[4];
extern u16 gRandSaved[32];
extern f32 gRandGauss[60];
extern f32 gRandGaussSum;
extern u8 D_006AE1A0[64][4];

s32 Sound_SetEffect(s32 core, s32 mode, s32 depth, s32 delay, s32 feedback); /* main */
f32 Math_Exp(f32 x);       /* expf */
f32 Math_Pow(f32 x, f32 y); /* powf */
f32 Math_Sqrtf(f32 x);       /* sqrtf (VU0) */
void *memset(void *p, s32 c, u32 n);

void Sound_SetEffectOnce(s32 core, s32 mode, u32 depth, s32 delay, s32 feedback);
u16 Rand_NextSaved(s16 i);

/* Reverb (effect mode 4) on both cores, unless an effect was already set. */
void Sound_ReverbOn(void)
{
    Sound_SetEffectOnce(0, 4, 0x1000, 0, 0);
    Sound_SetEffectOnce(1, 4, 0xA00, 0, 0);
}

void Sound_ReverbOff(void)
{
    Sound_SetEffectOnce(0, 4, 0, 0, 0);
    Sound_SetEffectOnce(1, 4, 0, 0, 0);
}

void Sound_ResetEffects(void)
{
    gSoundEffectSet[0] = 0;
    gSoundEffectSet[1] = 0;
}

void Sound_SetEffectOnce(s32 core, s32 mode, u32 depth, s32 delay, s32 feedback)
{
    u8 *set = &gSoundEffectSet[core];

    if (*set == 0) {
        *set = 1;
        Sound_SetEffect(core, mode, depth, delay, feedback);
    }
}

s32 Bgm_StopInArea87(void)
{
    if (gScene.area != 0x87) {
        return 0;
    }
    gBgmId = 0xFF;
    return 0;
}

void Debug_Message(char *unused0, s32 unused1, s32 unused2, s32 unused3)
{
}

void Rand_Seed(u8 seed)
{
    gRandSeeds[0] = seed;
    gRandSeeds[1] = seed;
}

u16 Rand_Next(s32 stream)
{
    u32 x = gRandSeeds[stream];

    if (x == 0) {
        x = 1;
    }
    return gRandSeeds[stream] = x * 176 % 0xFF53;
}

/* The saved stream is read and written through a declared byte offset o = i * 2 read through a
 * byte pointer (the form of Kbd_DrawSpecialKey's ofs): gRandSaved[i] recomputes the scaled index
 * after the if, where the original keeps it, and gSave.randSaved[i] shares it. */
u16 Rand_NextSaved(s16 i)
{
    s32 o = i * 2;
    u32 x = *(u16 *)((u8 *)gRandSaved + o);
    u16 r;

    if (x == 0) {
        x = 1;
    }
    r = x * 176 % 0xFF53;
    *(u16 *)((u8 *)gRandSaved + o) = r;
    gSave.randSaved[i] = *(u16 *)((u8 *)gRandSaved + o);
    return r;
}

void Rand_StepSaved(void)
{
    s16 i;

    for (i = 0; i < 32; i++) {
        Rand_NextSaved(i);
    }
}

/*
 * 60 samples of a bell curve, exp(-x*x/2) * sqrt(2 pi) for x = -30/9 ..
 * 29/9, and their sum. (The factor is sqrt(2 pi), not its inverse; callers
 * that divide by the sum don't see it.)
 */
void Rand_InitGauss(void)
{
    s32 i;
    f32 *p = gRandGauss;
    f32 e;
    f32 s;

    gRandGaussSum = 0.0f;
    for (i = 0; i < 60; i++) {
        e = Math_Exp(-Math_Pow((i - 30) / 9.0f, 2.0f) / 2.0f);
        s = Math_Sqrtf(6.2831855f);
        *p = 1.0f / (1.0f / s) * e;
        gRandGaussSum += *p;
        p++;
    }
}

void func_00368E80(void)
{
    memset(D_006AE1A0, 0, sizeof(D_006AE1A0));
}

void func_00368EA0(s16 i)
{
    s8 *p = (s8 *)D_006AE1A0[i];

    if (*p != 0) {
        *p = 0;
    }
}

void func_00368ED0(void)
{
    s16 i;

    for (i = 0; i < 64; i++) {
        func_00368EA0(i);
    }
}
