/*
 * Stream volumes and fades (the volume options
 * give each channel a ceiling), and the AFS loaders: partitions made from
 * files of partition 3, whole-file reads, and a small cache of "dmcam"
 * files.
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"


/* 0.1 dB output volume for each volume step 0..127 (-999 = silent) */
extern s16 gVolumeDb[128];
/* ceiling (volume step) for each option level, stereo and surround */
extern u8 gVolumeMax[8];
extern u8 gVolumeMaxSurround[8];
/* groups of the dmcam cache preloads: first index, count */
extern u8 gDmcamGroupAlt[2];
extern u8 gDmcamGroup[2];
extern char D_001C8530[];   /* "mib" */
extern char D_001C8538[];   /* "emcmd" */
extern char D_001C8540[];   /* "brdata" */
extern char D_001C8548[];   /* "dmcam" */
extern u8 gAfsPtInfo4[];

/* dmcam cache: 16 files of partition 6, packed one after the other */
extern s32 gDmcamCount;
extern u8 gDmcamId[16];     /* file id, 0xFF: free */
extern s32 gDmcamSize[16];  /* bytes, rounded up to 64 */
extern u8 *gDmcamAddr[16];
extern u8 D_5FE840[];       /* sub_main buffer for the cache */
extern u8 D_8B4D00[];       /* its alternative (overlay area) */
extern s32 gDmcamFileIds[];      /* sub_main: dmcam file of each index, >= 0x80: none */

void *memset(void *dst, s32 c, u32 n);
void Sound_InitBuffers(void);
void Sound_LoadSpuMap(void);
void Sound_SetOutputMode(s32 mode);

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

/* Partition ptid = the AFS archive in file flid of partition 3 (name is
 * only for the reader). */
void Afs_LoadPartition(s32 ptid, s32 flid, void *ptinfo, char *name)
{
    do {
    } while (ADXF_LoadPartitionFromAfsNw(ptid, 3, flid, ptinfo) < 0);
    do {
    } while (ADXF_GetPtStat(ptid) != ADXF_STAT_READEND);
}

/* Reads a whole file, running Load_Idle while waiting; returns the size
 * read (whole sectors). */
s32 Afs_ReadFileIdle(s32 flid, s32 ptid, void *buf)
{
    ADXF adxf;
    s32 nsct;
    s32 size;

    do {
        adxf = ADXF_OpenAfs(ptid, flid);
    } while (adxf == NULL);
    nsct = ADXF_GetFsizeSct(adxf);
    size = nsct * 0x800;
    memset(buf, 0, size);
    ADXF_ReadNw(adxf, nsct, buf);
    while (ADXF_GetStat(adxf) != ADXF_STAT_READEND) {
        Load_Idle();
    }
    ADXF_Close(adxf);
    return size;
}

/*
 * Reads the whole of file flid of AFS partition ptid into buf (cleared
 * first), waiting in a busy loop where Afs_ReadFileIdle runs Load_Idle.
 * Returns the size read (whole 0x800-byte sectors).
 */
s32 Afs_ReadFile(s32 flid, s32 ptid, void *buf)
{
    ADXF adxf;
    s32 nsct;
    s32 size;

    do {
        adxf = ADXF_OpenAfs(ptid, flid);
    } while (adxf == NULL);
    nsct = ADXF_GetFsizeSct(adxf);
    size = nsct * 0x800;
    memset(buf, 0, size);
    ADXF_ReadNw(adxf, nsct, buf);
    do {
    } while (ADXF_GetStat(adxf) != ADXF_STAT_READEND);
    ADXF_Close(adxf);
    return size;
}

s32 Mib_Read(s32 flid, void *buf)
{
    Afs_LoadPartition(5, 0x19BA, gSysSettings.loadBuf, D_001C8530);
    return Afs_ReadFileIdle(flid, 5, buf);
}

void Emcmd_LoadPartition(void *ptinfo)
{
    Afs_LoadPartition(6, 0x19BB, ptinfo, D_001C8538);
}

s32 Emcmd_Read(s32 flid, void *buf)
{
    memset(buf, 0, 0x10000);
    return Afs_ReadFileIdle(flid, 6, buf);
}

void Brdata_LoadPartition(void *ptinfo)
{
    Afs_LoadPartition(5, 0x19F0, ptinfo, D_001C8540);
}

s32 Brdata_Read(void *buf, s32 flid)
{
    return Afs_ReadFileIdle(flid, 5, buf);
}

/* Makes "dmcam" partition 6; keep: 1 keeps the cache. */
void Dmcam_Init(void *ptinfo, s32 keep)
{
    s32 i;

    Afs_LoadPartition(6, 0x19F5, gSysSettings.loadBuf, D_001C8548);
    if (keep != 1) {
        gDmcamCount = 0;
        for (i = 0; i < 16; i++) {
            gDmcamAddr[i] = NULL;
            gDmcamSize[i] = 0;
            gDmcamId[i] = 0xFF;
        }
    }
}

/* Returns the cached dmcam file of index idx, reading it after the last
 * one (in the overlay area when alt is 1, else in sub_main's cache buffer) if needed; NULL when
 * the index has no file or the cache is full. */
u8 *Dmcam_Load(s32 idx, s32 alt)
{
    s32 i;
    u8 *addr;
    s32 flid = gDmcamFileIds[idx];
    s32 size;

    if (flid >= 0x80) {
        return NULL;
    }
    for (i = 0; i < 16; i++) {
        if (gDmcamId[i] == (u8)flid) {
            return gDmcamAddr[i];
        }
    }
    if (gDmcamCount == 16) {
        return NULL;
    }
    if (alt == 1) {
        addr = D_8B4D00;
    } else {
        addr = D_5FE840;
    }
    if (gDmcamCount == 0) {
        i = 0;
    } else {
        for (i = 0; i < 16; i++) {
            if (gDmcamId[i] == 0xFF) {
                break;
            }
        }
        addr = gDmcamAddr[i - 1] + gDmcamSize[i - 1];
    }
    size = Afs_ReadFileIdle(flid, 6, addr);
    gDmcamAddr[i] = addr;
    gDmcamSize[i] = (size + 0x3F) & ~0x3F;
    gDmcamId[i] = flid;
    gDmcamCount++;
    return gDmcamAddr[i];
}

/*
 * The cached dmcam file of index idx (by gDmcamFileIds), NULL when it is
 * not in the cache. seen: a camera script's program counter is set to it.
 */
u8 *Dmcam_Find(s32 idx)
{
    s32 i;
    s32 flid = gDmcamFileIds[idx];

    for (i = 0; i < 16; i++) {
        if (gDmcamId[i] == (u8)flid) {
            return gDmcamAddr[i];
        }
    }
    return NULL;
}

/* Fresh cache with the 18 files from index 68, in the alternative
 * buffer (lobby, player select). */
void Dmcam_PreloadAlt(void)
{
    s32 i;
    s32 first;
    s32 count;

    Dmcam_Init(gSysSettings.loadBuf, 0);
    first = gDmcamGroupAlt[0];
    count = gDmcamGroupAlt[1];
    for (i = 0; i < count; i++) {
        Dmcam_Load(first + i, 1);
    }
}

/* Fresh cache with the 4 files from index 98. */
void Dmcam_Preload(void)
{
    s32 i;
    s32 first;
    s32 count;

    Dmcam_Init(gSysSettings.loadBuf, 0);
    first = gDmcamGroup[0];
    count = gDmcamGroup[1];
    for (i = 0; i < count; i++) {
        Dmcam_Load(first + i, 0);
    }
}

/* Partition 4 = file 1 of data.bin (partition 5 while booting). */
void Afs_LoadPartition4(void)
{
    do {
    } while (ADXF_LoadPartitionFromAfsNw(4, 5, 1, gAfsPtInfo4) < 0);
    do {
    } while (ADXF_GetPtStat(4) != ADXF_STAT_READEND);
}

s32 Afs_ReadOverlay(s32 flid, void *buf)
{
    return Afs_ReadFile(flid, 4, buf);
}

/*
 * Startup sound setup: the bank slot buffers (Sound_InitBuffers), the ADX
 * library (Adx_Init) and the SPU map (Sound_LoadSpuMap).
 */
void Sound_Boot(void)
{
    Sound_InitBuffers();
    Adx_Init();
    Sound_LoadSpuMap();
}

void Stream_Server(void)
{
    Stream_Update();
}

/* This file's initialised data. */
u8 gDmcamGroupAlt[2] = {
#include "main/001BCE10.inc"
};
u8 gDmcamGroup[2] = {
#include "main/001BCE18.inc"
};

/* This file's read-only data. */
#pragma rodata on
char D_001C8530[0x8] = {
#include "main/001C8530.inc"
};
char D_001C8538[0x8] = {
#include "main/001C8538.inc"
};
char D_001C8540[0x40] = {
#include "main/001C8540.inc"
};
u32 gOverlayWindowBase[32] = {
#include "main/001C8580.inc"
};
#pragma rodata off
