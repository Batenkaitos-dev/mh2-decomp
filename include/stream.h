#ifndef STREAM_H
#define STREAM_H

#include "common.h"
#include "adx.h"

/*
 * Streaming of the Capcom core: the two ADXT
 * stream channels (the music manager of sub_main, Bgm_*, plays through
 * channel 0), the AFS partitions and the file loaders, and the queue of
 * background file reads.
 *
 * AFS partitions (Adx_Init, Afs_LoadPartition):
 *   0 afs00.afs (the streams, Stream_Play), 1 afs01.afs, 2 afs02.afs,
 *   3 file 0 of data.bin, 4 file 1 of data.bin (the overlays),
 *   5 data.bin while booting, then "mib" (file 6586 of 3) or
 *     "brdata" (6640),
 *   6 "emcmd" (6587) or "dmcam" (6645).
 */
#define AFS_PT_AFS02 2          /* afs02.afs: Adx_Init loads it with the other load call */
#define AFS_PT_MAIN 3           /* file 0 of data.bin */
#define AFS_PT_BOOT 5           /* data.bin itself, while booting */
#define AFS_PT_OVERLAYS 4       /* file 1 of data.bin: the overlays (Afs_LoadPartition4) */
#define AFS_PT_INNER 5          /* after booting, an archive of AFS_PT_MAIN: the mib or brdata files */
#define AFS_PT_INNER_ALT 6      /* another: the emcmd or dmcam files */

/* The archives of AFS_PT_MAIN that Afs_LoadPartition makes partitions of. */
#define AFS_FILE_MIB 6586
#define AFS_FILE_EMCMD 6587
#define AFS_FILE_BRDATA 6640
#define AFS_FILE_DMCAM 6645
#define AFS_BOOT_PARTITIONS 4   /* the partitions Adx_Init loads (gAfsPartitions) */

/* Stream channel: 0 plays the music (music volume option), 1 a second
 * stream at the SE volume option. */
typedef struct StreamChan {
    /* 0x00 */ s32 stat;        /* ADXT_GetStat() at the last update */
    /* 0x04 */ s16 maxVol;      /* ceiling from the volume option, 0..98 */
    /* 0x06 */ s16 vol;         /* current volume, 0..127 */
    /* 0x08 */ s16 unk08;
    /* 0x0A */ s16 fadeFrom;
    /* 0x0C */ s16 fadeTo;
    /* 0x0E */ s16 fadeLen;     /* frames */
    /* 0x10 */ s16 fadeLeft;    /* frames still to go */
    /* 0x12 */ s16 unk12;
} StreamChan; /* size 20 */

#define STREAM_CHAN_COUNT 2

extern StreamChan gStreamChan[STREAM_CHAN_COUNT];
extern ADXT gStreamAdxt[STREAM_CHAN_COUNT];

/* ADX's set-up, and the ADXT error callback it sets, which ignores the error. */
void Adx_Init(void);
void Adx_ErrorCallback(void *obj, char *msg);

/* Background file reads. */
void Stream_Update(void);
void Load_Idle(void);
void FileQueue_Init(void);
void FileQueue_AddFile(u32 id, void *dst);
void FileQueue_AddCall(void (*func)(void), s32 nargs, s32 firstArg, s32 secondArg, s32 thirdArg, s32 fourthArg);
void FileQueue_Update(void);
void FileQueue_Cancel(void);
s32 FileQueue_IsBusy(void);
s32 Lobby_IsLookPending(s32 playerIndex);
void FileQueue_Flush(void);

/* Starting and stopping the streams. */
void Stream_Init(void);
void Stream_Play(s32 ch, s32 flid);
void Stream_PlayMusicVol(s32 ch, s32 flid);
void Stream_PlayLevel(s32 ch, s32 flid, s32 level);
void Stream_PlayFadeIn(s32 ch, s32 flid, s16 frames, s16 level);
void Stream_PlayVol(s32 ch, s32 flid, s32 vol, s32 pause);
void Stream_Prepare(s32 ch, s32 flid, s32 vol, s32 pause);
s32 Stream_IsReady(s32 ch);
void Stream_StartPrepared(s32 ch);
void Stream_Pause(s32 ch, s32 pause);
void Stream_Stop(s32 ch);
extern s16 gVolumeDb[128];
extern u8 gVolumeMax[8];
extern u8 gVolumeMaxSurround[8];

/* Volumes and fades, AFS loaders. */
void Stream_Reset(void);
void Stream_UpdateMaxVol(s32 apply);
void Stream_SetVol(s32 ch, s32 vol);
void Sound_ApplyOutputMode(s32 mode);
s32 Stream_GetStat(s32 ch);
void Stream_FadeOut(s32 ch, s32 frames);
void Stream_FadeToMax(s32 ch, s16 frames);
void Stream_FadeTo(s32 ch, s16 frames, s16 level);
void Stream_UpdateFades(void);
void Afs_LoadPartition(s32 ptid, s32 flid, void *ptinfo, char *name);
s32 Afs_ReadFileIdle(s32 flid, s32 ptid, void *buf);
s32 Afs_ReadFile(s32 flid, s32 ptid, void *buf);
s32 Mib_Read(s32 flid, void *buf);
void Emcmd_LoadPartition(void *ptinfo);
s32 Emcmd_Read(s32 flid, void *buf);
void Brdata_LoadPartition(void *ptinfo);
s32 Brdata_Read(void *buf, s32 flid);
void Dmcam_Init(void *ptinfo, s32 keep);
u8 *Dmcam_Load(s32 idx, s32 alt);
u8 *Dmcam_Find(s32 idx);
void Dmcam_PreloadAlt(void);
void Dmcam_Preload(void);
void Afs_LoadPartition4(void);
s32 Afs_ReadOverlay(s32 flid, void *buf);
void Sound_Boot(void);
void Stream_Server(void);
extern u32 gOverlayWindowBase[32];

#endif /* STREAM_H */
