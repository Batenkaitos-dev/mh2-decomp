/*
 * Starting, pausing and stopping the two ADXT
 * streams (channel 0 plays the music from afs00.afs).
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"

extern s32 gPadDebugSwitch;

/* sub_main debug switch (0 in the retail game): no streams are started */
#define SYS_DEBUG (gPadDebugSwitch)

extern s16 gVolumeDb[128];
extern u8 gVolumeMax[8];
extern u8 gVolumeMaxSurround[8];

void *memset(void *dst, s32 c, u32 n);

/*
 * Stops and unpauses every stream channel and clears its StreamChan
 * state.
 */
void Stream_Init(void)
{
    s32 i;
    StreamChan *chan = gStreamChan;

    for (i = 0; i < STREAM_CHAN_COUNT; i++, chan++) {
        Stream_Stop(i);
        Stream_Pause(i, 0);
        memset(chan, 0, sizeof(StreamChan));
    }
}

/* Plays file flid of afs00.afs (a negative flid: the next seamless entry)
 * at the channel's ceiling. */
void Stream_Play(s32 chanIndex, s32 flid)
{
    Stream_PlayVol(chanIndex, flid, gStreamChan[chanIndex].maxVol, 0);
}

/*
 * Plays file flid of afs00.afs (negative: the next seamless entry) on
 * channel chanIndex at the music channel's ceiling, whatever chanIndex
 * (seen: called for channel 1). Any fade under way is dropped. Nothing
 * starts while SYS_DEBUG is set.
 */
void Stream_PlayMusicVol(s32 chanIndex, s32 flid)
{
    StreamChan *chan = &gStreamChan[chanIndex];
    s32 volDb;

    Stream_Pause(chanIndex, 0);
    chan->vol = gStreamChan[0].maxVol;
    volDb = (f32)gVolumeDb[chan->vol];
    if (volDb < -999) {
        volDb = -999;
    }
    ADXT_SetOutVol(gStreamAdxt[chanIndex], volDb);
    chan->fadeLeft = 0;
    if (SYS_DEBUG == 0) {
        if (flid >= 0) {
            ADXT_StartAfs(gStreamAdxt[chanIndex], 0, flid);
        } else {
            ADXT_StartSeamless(gStreamAdxt[chanIndex]);
        }
    }
}

/*
 * Plays file flid on channel chanIndex at level (0-127, a fraction of the
 * channel's ceiling).
 */
void Stream_PlayLevel(s32 chanIndex, s32 flid, s32 level)
{
    StreamChan *chan = &gStreamChan[chanIndex];

    Stream_PlayVol(chanIndex, flid, level * chan->maxVol / 127, 0);
}

/* Starts silent and fades in to level over frames. */
void Stream_PlayFadeIn(s32 chanIndex, s32 flid, s16 frames, s16 level)
{
    Stream_PlayVol(chanIndex, flid, 0, 0);
    Stream_FadeTo(chanIndex, frames, level);
}

/*
 * Plays file flid of afs00.afs (negative: the next seamless entry) on
 * channel chanIndex at volume step vol, paused when pause is nonzero. Any
 * fade under way is dropped. Nothing starts while SYS_DEBUG is set.
 */
void Stream_PlayVol(s32 chanIndex, s32 flid, s32 vol, s32 pause)
{
    StreamChan *chan;

    Stream_Pause(chanIndex, pause);
    Stream_SetVol(chanIndex, vol);
    chan = &gStreamChan[chanIndex];
    chan->fadeLeft = 0;
    if (SYS_DEBUG == 0) {
        if (flid >= 0) {
            ADXT_StartAfs(gStreamAdxt[chanIndex], 0, flid);
        } else {
            ADXT_StartSeamless(gStreamAdxt[chanIndex]);
        }
    }
}

/* Starts decoding but holds the output until Stream_StartPrepared. */
void Stream_Prepare(s32 chanIndex, s32 flid, s32 vol, s32 pause)
{
    StreamChan *chan;

    Stream_Pause(chanIndex, pause);
    Stream_SetVol(chanIndex, vol);
    chan = &gStreamChan[chanIndex];
    chan->fadeLeft = 0;
    if (SYS_DEBUG == 0) {
        ADXT_SetWaitPlayStart(gStreamAdxt[chanIndex], 1);
        ADXT_StartAfs(gStreamAdxt[chanIndex], 0, flid);
    }
}

s32 Stream_IsReady(s32 chanIndex)
{
    return ADXT_IsReadyPlayStart(gStreamAdxt[chanIndex]) != 0;
}

/*
 * Releases a stream started with Stream_Prepare at the ceiling of the
 * volume option (music volume for channel 0, SE volume otherwise). The
 * ceiling is stored in channel 0 whatever chanIndex (the loop body of
 * Stream_UpdateMaxVol without the loop).
 */
void Stream_StartPrepared(s32 chanIndex)
{
    StreamChan *musicChan = gStreamChan;
    u8 *levels;

    if (gSysSettings.soundMode == 2) {
        levels = gVolumeMaxSurround;
    } else {
        levels = gVolumeMax;
    }
    if (chanIndex == 0) {
        musicChan->maxVol = levels[gSysSettings.musicVolume];
    } else {
        musicChan->maxVol = levels[gSysSettings.seVolume];
    }
    Stream_SetVol(chanIndex, musicChan->maxVol);
    ADXT_SetWaitPlayStart(gStreamAdxt[chanIndex], 0);
}

/*
 * Pauses (pause nonzero) or resumes stream channel chanIndex; resuming
 * also clears its wait for Stream_StartPrepared.
 */
void Stream_Pause(s32 chanIndex, s32 pause)
{
    ADXT *adxtHandle = &gStreamAdxt[chanIndex];

    ADXT_Pause(*adxtHandle, pause);
    if (pause == 0) {
        ADXT_SetWaitPlayStart(*adxtHandle, 0);
    }
}

void Stream_Stop(s32 chanIndex)
{
    ADXT_Stop(gStreamAdxt[chanIndex]);
}

/* This file's initialised data. */
s16 gVolumeDb[128] = {
#include "main/001BCD00.inc"
};
u8 gVolumeMax[8] = {
#include "main/001BCE00.inc"
};
u8 gVolumeMaxSurround[8] = {
#include "main/001BCE08.inc"
};
