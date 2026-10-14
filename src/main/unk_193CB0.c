/*
 * main 0x193cb0-0x194830: stream volumes and fades (the volume options
 * give each channel a ceiling), and the AFS loaders: partitions made from
 * files of partition 3, whole-file reads, and a small cache of "dmcam"
 * files.
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"

/* sub_main system settings (0x6ADF90) */
/* sub_main work buffer (also the table of contents of partitions 5, 6) */

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
    StreamChan *c = gStreamChan;
    u8 *levels;

    if (gSysSettings.soundMode == 2) {
        levels = gVolumeMaxSurround;
    } else {
        levels = gVolumeMax;
    }
    for (i = 0; i < STREAM_CHAN_COUNT; i++, c++) {
        if (i == 0) {
            c->maxVol = levels[gSysSettings.musicVolume];
        } else {
            c->maxVol = levels[gSysSettings.seVolume];
        }
        if (apply == 1) {
            Stream_SetVol(i, c->maxVol);
        }
    }
}

void Stream_SetVol(s32 ch, s32 vol)
{
    StreamChan *c = &gStreamChan[ch];
    s32 db;

    if (c->maxVol < vol) {
        vol = c->maxVol;
    }
    c->vol = vol;
    db = (f32)gVolumeDb[c->vol];
    if (db < -999) {
        db = -999;
    }
    ADXT_SetOutVol(gStreamAdxt[ch], db);
}

void Sound_ApplyOutputMode(s32 mode)
{
    if (gSysSettings.soundMode == 0) {
        ADXT_SetOutputMono(1);
    } else {
        ADXT_SetOutputMono(0);
    }
    Sound_SetOutputMode(mode);
}

s32 Stream_GetStat(s32 ch)
{
    StreamChan *c = &gStreamChan[ch];

    c->stat = ADXT_GetStat(gStreamAdxt[ch]);
    return c->stat;
}

void Stream_FadeOut(s32 ch, s32 frames)
{
    StreamChan *c = &gStreamChan[ch];

    c->fadeFrom = c->vol;
    c->fadeTo = 0;
    c->fadeLeft = frames;
    c->fadeLen = frames;
}

void Stream_FadeToMax(s32 ch, s16 frames)
{
    StreamChan *c = &gStreamChan[ch];

    c->fadeFrom = c->vol;
    c->fadeTo = c->maxVol;
    c->fadeLeft = frames;
    c->fadeLen = frames;
}

/* level: 0..127 of the ceiling */
void Stream_FadeTo(s32 ch, s16 frames, s16 level)
{
    StreamChan *c = &gStreamChan[ch];

    c->fadeFrom = c->vol;
    c->fadeTo = level * c->maxVol / 127;
    c->fadeLeft = frames;
    c->fadeLen = frames;
}

/* Per frame: steps the fades of the playing channels. */
void Stream_UpdateFades(void)
{
    s32 i;
    StreamChan *c = gStreamChan;
    ADXT *h = gStreamAdxt;

    for (i = 0; i < STREAM_CHAN_COUNT; i++, h++, c++) {
        if (c->fadeLeft > 0 && c->stat == ADXT_STAT_PLAYING) {
            c->fadeLeft--;
            Stream_SetVol(i, c->fadeFrom + (c->fadeTo - c->fadeFrom) * (c->fadeLen - c->fadeLeft) / c->fadeLen);
        }
        c->stat = ADXT_GetStat(*h);
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
 * one (from D_8B4D00 when alt is 1, else D_5FE840) if needed; NULL when
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

/* unk_193CB0's .data (0x1BCE10-0x1BCE20) */
u8 gDmcamGroupAlt[2] = {
#include "main/001BCE10.inc"
};
u8 gDmcamGroup[2] = {
#include "main/001BCE18.inc"
};

/* unk_193CB0's .rodata (0x1C8530-0x1C8600) */
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
