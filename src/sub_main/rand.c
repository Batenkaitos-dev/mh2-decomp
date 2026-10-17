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
 * Another RAND_SAVED_STREAMS streams (Rand_NextSaved) are copied into the
 * save data. Rand_InitGauss tabulates a bell curve.
 *
 * At the range's end, gUnreadFlags: a table of 64 flags that the boot,
 * the quest's end and the text code clear, and no code reads.
 */
#include "common.h"
#include "scene.h"
#include "save.h"
#include "sound.h"
#include "area.h"

extern u8 gSoundEffectSet[2];        /* per SPU core: effect already set */
extern u16 gBgmId;
extern u16 gRandSeeds[4];
#define RAND_SAVED_STREAMS 32
#define RAND_GAUSS_SAMPLES 60
#define UNREAD_FLAG_COUNT 64

extern u16 gRandSaved[RAND_SAVED_STREAMS];
extern f32 gRandGauss[RAND_GAUSS_SAMPLES];
extern f32 gRandGaussSum;
extern u8 gUnreadFlags[UNREAD_FLAG_COUNT][4];

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

/* Sets an SPU core's effect (Sound_SetEffect), unless one was set already
 * since Sound_ResetEffects. */
void Sound_SetEffectOnce(s32 core, s32 mode, u32 depth, s32 delay, s32 feedback)
{
    u8 *set = &gSoundEffectSet[core];

    if (*set == 0) {
        *set = 1;
        Sound_SetEffect(core, mode, depth, delay, feedback);
    }
}

/* In AREA_HUB_0x87, clears gBgmId to BGM_NONE; the lobby calls it.
 * Returns 0. */
s32 Bgm_StopInArea87(void)
{
    if (gScene.area != AREA_HUB_0x87) {
        return 0;
    }
    gBgmId = BGM_NONE;
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

/* Steps a stream (0 or 1) and returns its new state, 1..65362. */
u16 Rand_Next(s32 stream)
{
    u32 x = gRandSeeds[stream];

    if (x == 0) {
        x = 1;
    }
    return gRandSeeds[stream] = x * 176 % 0xFF53;
}

/*
 * Steps saved stream i like Rand_Next, copies it into the save data
 * (gSave.randSaved) and returns it. The stream is reached through a byte
 * offset for the match: the original keeps i * 2 in one register for both
 * arrays, where gRandSaved[i] would compute it again after the if.
 */
u16 Rand_NextSaved(s16 i)
{
    s32 offset = i * 2;
    u32 x = *(u16 *)((u8 *)gRandSaved + offset);
    u16 next;

    if (x == 0) {
        x = 1;
    }
    next = x * 176 % 0xFF53;
    *(u16 *)((u8 *)gRandSaved + offset) = next;
    gSave.randSaved[i] = *(u16 *)((u8 *)gRandSaved + offset);
    return next;
}

/* Steps every saved stream once. */
void Rand_StepSaved(void)
{
    s16 i;

    for (i = 0; i < RAND_SAVED_STREAMS; i++) {
        Rand_NextSaved(i);
    }
}

/*
 * RAND_GAUSS_SAMPLES samples of a bell curve, exp(-x*x/2) * sqrt(2 pi) for x = -30/9 ..
 * 29/9, and their sum. (The factor is sqrt(2 pi), not its inverse; callers
 * that divide by the sum don't see it.)
 */
void Rand_InitGauss(void)
{
    s32 i;
    f32 *sample = gRandGauss;
    f32 bell;
    f32 scale;

    gRandGaussSum = 0.0f;
    for (i = 0; i < RAND_GAUSS_SAMPLES; i++) {
        bell = Math_Exp(-Math_Pow((i - 30) / 9.0f, 2.0f) / 2.0f);
        scale = Math_Sqrtf(6.2831855f);
        *sample = 1.0f / (1.0f / scale) * bell;
        gRandGaussSum += *sample;
        sample++;
    }
}

/* Clears the whole table (select's boot). */
void UnreadFlags_Init(void)
{
    memset(gUnreadFlags, 0, sizeof(gUnreadFlags));
}

/* Clears flag i (the first byte of its entry) if it is set. */
void UnreadFlags_Clear(s16 i)
{
    s8 *flag = (s8 *)gUnreadFlags[i];

    if (*flag != 0) {
        *flag = 0;
    }
}

/* Clears every flag (the quest's end, the text code). */
void UnreadFlags_ClearAll(void)
{
    s16 i;

    for (i = 0; i < UNREAD_FLAG_COUNT; i++) {
        UnreadFlags_Clear(i);
    }
}
