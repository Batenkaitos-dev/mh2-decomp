/*
 * The AFS loaders: partitions made from archives in AFS_PT_MAIN (the mib,
 * emcmd, brdata and dmcam files) and the overlays' partition, whole-file
 * reads, and a small cache of dmcam files (the camera scripts' files) read
 * one after another into a buffer. The file ends with the sound's start-up
 * (Sound_Boot) and the stream server's step (Stream_Server).
 */
#include "common.h"
#include "syssettings.h"
#include "stream.h"
#include "sound.h"
#include "libc.h"

/* groups of the dmcam cache preloads: first index, count */
extern u8 gDmcamGroupAlt[2];
extern u8 gDmcamGroup[2];
extern char gMibPtName[];      /* the AFS partitions' names */
extern char gEmcmdPtName[];
extern char gBrdataPtName[];
extern char gDmcamPtName[];
extern u8 gAfsPtInfo4[];

/* dmcam cache: 16 files of partition 6, packed one after the other */
/* The dmcam cache: DMCAM_CACHE_MAX files, read one after another into a
 * buffer. */
#define DMCAM_CACHE_MAX 16
#define DMCAM_FREE 255                  /* gDmcamId of a free entry */
#define DMCAM_NO_FILE 128               /* gDmcamFileIds from here on: no file */
#define EMCMD_FILE_MAX 65536            /* bytes Emcmd_Read clears */

extern s32 gDmcamCount;
extern u8 gDmcamId[DMCAM_CACHE_MAX];     /* file id, DMCAM_FREE: free */
extern s32 gDmcamSize[DMCAM_CACHE_MAX];  /* bytes, rounded up to 64 */
extern u8 *gDmcamAddr[DMCAM_CACHE_MAX];
extern u8 D_5FE840[];       /* sub_main buffer for the cache */
extern u8 D_8B4D00[];       /* its alternative (overlay area) */
extern u32 *gDmcamFileIds[];     /* sub_main: dmcam file of each index; DMCAM_NO_FILE and up (a built-in script's address): none */

/* Makes partition ptid of the AFS archive in file flid of AFS_PT_MAIN
 * (name is only for the reader). */
void Afs_LoadPartition(s32 ptid, s32 flid, void *ptinfo, char *name)
{
    do {
    } while (ADXF_LoadPartitionFromAfsNw(ptid, AFS_PT_MAIN, flid, ptinfo) < 0);
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
    size = nsct * 2048;
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
 * Returns the size read (whole 2048-byte sectors).
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
    size = nsct * 2048;
    memset(buf, 0, size);
    ADXF_ReadNw(adxf, nsct, buf);
    do {
    } while (ADXF_GetStat(adxf) != ADXF_STAT_READEND);
    ADXF_Close(adxf);
    return size;
}

/* Makes the mib files' partition and reads mib file flid into buf;
 * returns the size read. */
s32 Mib_Read(s32 flid, void *buf)
{
    Afs_LoadPartition(AFS_PT_INNER, AFS_FILE_MIB, gSysSettings.loadBuf, gMibPtName);
    return Afs_ReadFileIdle(flid, AFS_PT_INNER, buf);
}

/* Makes the emcmd files' partition (its table in ptinfo). */
void Emcmd_LoadPartition(void *ptinfo)
{
    Afs_LoadPartition(AFS_PT_INNER_ALT, AFS_FILE_EMCMD, ptinfo, gEmcmdPtName);
}

/* Reads emcmd file flid into buf, cleared first; returns the size read. */
s32 Emcmd_Read(s32 flid, void *buf)
{
    memset(buf, 0, EMCMD_FILE_MAX);
    return Afs_ReadFileIdle(flid, AFS_PT_INNER_ALT, buf);
}

/* Makes the brdata files' partition (its table in ptinfo). */
void Brdata_LoadPartition(void *ptinfo)
{
    Afs_LoadPartition(AFS_PT_INNER, AFS_FILE_BRDATA, ptinfo, gBrdataPtName);
}

/* Reads brdata file flid into buf; returns the size read. */
s32 Brdata_Read(void *buf, s32 flid)
{
    return Afs_ReadFileIdle(flid, AFS_PT_INNER, buf);
}

/* Makes the dmcam files' partition (AFS_PT_INNER_ALT); keep 1 keeps the
 * cache, anything else empties it. */
void Dmcam_Init(void *ptinfo, s32 keep)
{
    s32 i;

    Afs_LoadPartition(AFS_PT_INNER_ALT, AFS_FILE_DMCAM, gSysSettings.loadBuf, gDmcamPtName);
    if (keep != 1) {
        gDmcamCount = 0;
        for (i = 0; i < DMCAM_CACHE_MAX; i++) {
            gDmcamAddr[i] = NULL;
            gDmcamSize[i] = 0;
            gDmcamId[i] = DMCAM_FREE;
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
    s32 flid = (s32)gDmcamFileIds[idx];
    s32 size;

    if (flid >= DMCAM_NO_FILE) {
        return NULL;
    }
    for (i = 0; i < DMCAM_CACHE_MAX; i++) {
        if (gDmcamId[i] == (u8)flid) {
            return gDmcamAddr[i];
        }
    }
    if (gDmcamCount == DMCAM_CACHE_MAX) {
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
        for (i = 0; i < DMCAM_CACHE_MAX; i++) {
            if (gDmcamId[i] == DMCAM_FREE) {
                break;
            }
        }
        addr = gDmcamAddr[i - 1] + gDmcamSize[i - 1];
    }
    size = Afs_ReadFileIdle(flid, AFS_PT_INNER_ALT, addr);
    gDmcamAddr[i] = addr;
    gDmcamSize[i] = (size + 63) & ~0x3F;
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
    s32 flid = (s32)gDmcamFileIds[idx];

    for (i = 0; i < DMCAM_CACHE_MAX; i++) {
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

/* Makes AFS_PT_OVERLAYS from file 1 of data.bin (AFS_PT_BOOT). */
void Afs_LoadPartition4(void)
{
    do {
    } while (ADXF_LoadPartitionFromAfsNw(AFS_PT_OVERLAYS, AFS_PT_BOOT, 1, gAfsPtInfo4) < 0);
    do {
    } while (ADXF_GetPtStat(AFS_PT_OVERLAYS) != ADXF_STAT_READEND);
}

/* Reads overlay file flid of AFS_PT_OVERLAYS into buf; returns the size. */
s32 Afs_ReadOverlay(s32 flid, void *buf)
{
    return Afs_ReadFile(flid, AFS_PT_OVERLAYS, buf);
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

/* The stream server's step: Stream_Update. */
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
char gMibPtName[8] = {
#include "main/001C8530.inc"
};
char gEmcmdPtName[8] = {
#include "main/001C8538.inc"
};
char gBrdataPtName[64] = {
#include "main/001C8540.inc"
};
u32 gOverlayWindowBase[32] = {
#include "main/001C8580.inc"
};
#pragma rodata off
