/*
 * main 0x1937a0-0x193cb0: starting, pausing and stopping the two ADXT
 * streams (channel 0 plays the music from afs00.afs).
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"

extern s32 gPadDebugSwitch;
/* sub_main system settings (0x6ADF90) */
/* sub_main debug switch (0 in the retail game): no streams are started */
#define SYS_DEBUG (gPadDebugSwitch)

extern s16 gVolumeDb[128];
extern u8 gVolumeMax[8];
extern u8 gVolumeMaxSurround[8];

void *memset(void *dst, s32 c, u32 n);

void Stream_Init(void)
{
    s32 i;
    StreamChan *c = gStreamChan;

    for (i = 0; i < STREAM_CHAN_COUNT; i++, c++) {
        Stream_Stop(i);
        Stream_Pause(i, 0);
        memset(c, 0, sizeof(StreamChan));
    }
}

/* Plays file flid of afs00.afs (a negative flid: the next seamless entry)
 * at the channel's ceiling. */
void Stream_Play(s32 ch, s32 flid)
{
    Stream_PlayVol(ch, flid, gStreamChan[ch].maxVol, 0);
}

/* Plays at the music channel's ceiling, whatever ch (called for ch 1). */
void Stream_PlayMusicVol(s32 ch, s32 flid)
{
    StreamChan *c = &gStreamChan[ch];
    s32 db;

    Stream_Pause(ch, 0);
    c->vol = gStreamChan[0].maxVol;
    db = (f32)gVolumeDb[c->vol];
    if (db < -999) {
        db = -999;
    }
    ADXT_SetOutVol(gStreamAdxt[ch], db);
    c->fadeLeft = 0;
    if (SYS_DEBUG == 0) {
        if (flid >= 0) {
            ADXT_StartAfs(gStreamAdxt[ch], 0, flid);
        } else {
            ADXT_StartSeamless(gStreamAdxt[ch]);
        }
    }
}

/* level: 0..127 of the channel's ceiling */
void Stream_PlayLevel(s32 ch, s32 flid, s32 level)
{
    StreamChan *c = &gStreamChan[ch];

    Stream_PlayVol(ch, flid, level * c->maxVol / 127, 0);
}

/* Starts silent and fades in to level over frames. */
void Stream_PlayFadeIn(s32 ch, s32 flid, s16 frames, s16 level)
{
    Stream_PlayVol(ch, flid, 0, 0);
    Stream_FadeTo(ch, frames, level);
}

void Stream_PlayVol(s32 ch, s32 flid, s32 vol, s32 pause)
{
    StreamChan *c;

    Stream_Pause(ch, pause);
    Stream_SetVol(ch, vol);
    c = &gStreamChan[ch];
    c->fadeLeft = 0;
    if (SYS_DEBUG == 0) {
        if (flid >= 0) {
            ADXT_StartAfs(gStreamAdxt[ch], 0, flid);
        } else {
            ADXT_StartSeamless(gStreamAdxt[ch]);
        }
    }
}

/* Starts decoding but holds the output until Stream_StartPrepared. */
void Stream_Prepare(s32 ch, s32 flid, s32 vol, s32 pause)
{
    StreamChan *c;

    Stream_Pause(ch, pause);
    Stream_SetVol(ch, vol);
    c = &gStreamChan[ch];
    c->fadeLeft = 0;
    if (SYS_DEBUG == 0) {
        ADXT_SetWaitPlayStart(gStreamAdxt[ch], 1);
        ADXT_StartAfs(gStreamAdxt[ch], 0, flid);
    }
}

s32 Stream_IsReady(s32 ch)
{
    return ADXT_IsReadyPlayStart(gStreamAdxt[ch]) != 0;
}

/* Releases a prepared stream at the ceiling of the volume option. The
 * ceiling is stored in channel 0 whatever ch (the loop body of
 * Stream_UpdateMaxVol without the loop). */
void Stream_StartPrepared(s32 ch)
{
    StreamChan *c = gStreamChan;
    u8 *levels;

    if (gSysSettings.soundMode == 2) {
        levels = gVolumeMaxSurround;
    } else {
        levels = gVolumeMax;
    }
    if (ch == 0) {
        c->maxVol = levels[gSysSettings.musicVolume];
    } else {
        c->maxVol = levels[gSysSettings.seVolume];
    }
    Stream_SetVol(ch, c->maxVol);
    ADXT_SetWaitPlayStart(gStreamAdxt[ch], 0);
}

void Stream_Pause(s32 ch, s32 pause)
{
    ADXT *h = &gStreamAdxt[ch];

    ADXT_Pause(*h, pause);
    if (pause == 0) {
        ADXT_SetWaitPlayStart(*h, 0);
    }
}

void Stream_Stop(s32 ch)
{
    ADXT_Stop(gStreamAdxt[ch]);
}

/* unk_1937A0's .data (0x1BCD00-0x1BCE10) */
s16 gVolumeDb[128] = {
#include "main/001BCD00.inc"
};
u8 gVolumeMax[8] = {
#include "main/001BCE00.inc"
};
u8 gVolumeMaxSurround[8] = {
#include "main/001BCE08.inc"
};
