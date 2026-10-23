/*
 * Starting, pausing and stopping the two ADXT streams (channel 0 plays the
 * music from afs00.afs), and their volumes and fades: the volume options
 * give each channel a ceiling (Stream_UpdateMaxVol), and Stream_UpdateFades
 * steps the fades once a frame.
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"
#include "sound.h"
#include "libc.h"

extern s32 gPadDebugSwitch;

/* sub_main debug switch (0 in the retail game): no streams are started */
#define SYS_DEBUG (gPadDebugSwitch)

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

void Stream_Reset(void)
{
    Stream_Init();
    Stream_UpdateMaxVol(0);
}

/* Takes the ceilings from the volume options; apply: 1 also sets the
 * volumes to them. */
void Stream_UpdateMaxVol(s32 apply)
{
    s32 i;
    StreamChan *chan = gStreamChan;
    u8 *levels;

    if (gSysSettings.soundMode == 2) {
        levels = gVolumeMaxSurround;
    } else {
        levels = gVolumeMax;
    }
    for (i = 0; i < STREAM_CHAN_COUNT; i++, chan++) {
        if (i == 0) {
            chan->maxVol = levels[gSysSettings.musicVolume];
        } else {
            chan->maxVol = levels[gSysSettings.seVolume];
        }
        if (apply == 1) {
            Stream_SetVol(i, chan->maxVol);
        }
    }
}

/*
 * Sets stream channel chanIndex to volume step vol (0-127), capped at the
 * channel's ceiling (maxVol), and sends the matching output volume from
 * gVolumeDb (0.1 dB, no lower than -999) to its ADXT handle.
 */
void Stream_SetVol(s32 chanIndex, s32 vol)
{
    StreamChan *chan = &gStreamChan[chanIndex];
    s32 volDb;

    if (chan->maxVol < vol) {
        vol = chan->maxVol;
    }
    chan->vol = vol;
    volDb = (f32)gVolumeDb[chan->vol];
    if (volDb < -999) {
        volDb = -999;
    }
    ADXT_SetOutVol(gStreamAdxt[chanIndex], volDb);
}

/*
 * Applies the sound option: ADX streams go mono when
 * gSysSettings.soundMode is 0, and the sound engine's output is set from
 * mode (Sound_SetOutputMode). seen: callers pass the saved soundMode.
 */
void Sound_ApplyOutputMode(s32 mode)
{
    if (gSysSettings.soundMode == 0) {
        ADXT_SetOutputMono(1);
    } else {
        ADXT_SetOutputMono(0);
    }
    Sound_SetOutputMode(mode);
}

/*
 * Reads the ADXT status of stream channel chanIndex, keeps it in the
 * channel's stat and returns it.
 */
s32 Stream_GetStat(s32 chanIndex)
{
    StreamChan *chan = &gStreamChan[chanIndex];

    chan->stat = ADXT_GetStat(gStreamAdxt[chanIndex]);
    return chan->stat;
}

/*
 * Starts a fade of stream channel chanIndex from its volume to 0 over
 * frames frames, stepped by Stream_UpdateFades.
 */
void Stream_FadeOut(s32 chanIndex, s32 frames)
{
    StreamChan *chan = &gStreamChan[chanIndex];

    chan->fadeFrom = chan->vol;
    chan->fadeTo = 0;
    chan->fadeLeft = frames;
    chan->fadeLen = frames;
}

/*
 * Starts a fade of stream channel chanIndex from its volume to its
 * ceiling (maxVol) over frames frames, stepped by Stream_UpdateFades.
 */
void Stream_FadeToMax(s32 chanIndex, s16 frames)
{
    StreamChan *chan = &gStreamChan[chanIndex];

    chan->fadeFrom = chan->vol;
    chan->fadeTo = chan->maxVol;
    chan->fadeLeft = frames;
    chan->fadeLen = frames;
}

/*
 * Starts a fade of stream channel chanIndex from its volume to level
 * (0-127, a fraction of the channel's ceiling) over frames frames,
 * stepped by Stream_UpdateFades.
 */
void Stream_FadeTo(s32 chanIndex, s16 frames, s16 level)
{
    StreamChan *chan = &gStreamChan[chanIndex];

    chan->fadeFrom = chan->vol;
    chan->fadeTo = level * chan->maxVol / 127;
    chan->fadeLeft = frames;
    chan->fadeLen = frames;
}

/* Per frame: steps the fades of the playing channels. */
void Stream_UpdateFades(void)
{
    s32 i;
    StreamChan *chan = gStreamChan;
    ADXT *adxtHandle = gStreamAdxt;

    for (i = 0; i < STREAM_CHAN_COUNT; i++, adxtHandle++, chan++) {
        if (chan->fadeLeft > 0 && chan->stat == ADXT_STAT_PLAYING) {
            chan->fadeLeft--;
            Stream_SetVol(i, chan->fadeFrom + (chan->fadeTo - chan->fadeFrom) * (chan->fadeLen - chan->fadeLeft) / chan->fadeLen);
        }
        chan->stat = ADXT_GetStat(*adxtHandle);
    }
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
