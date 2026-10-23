/*
 * ADX's start-up (Adx_Init, with the error callback it sets): the two
 * ADXT stream players and the AFS partitions loaded at boot: afs00.afs,
 * afs01.afs, afs02.afs and data.bin's file 0, with data.bin itself as a
 * partition of its own while booting.
 */
#include "common.h"
#include "stream.h"
#include "ram_areas.h"

typedef struct AfsPartition {
    char *fname;        /* the file's name */
    void *ptinfo;       /* its table of contents' buffer */
    s32 ptinfoSize;     /* that buffer's size */
} AfsPartition;

typedef struct AdxtParam {
    s32 maxnch;
    s32 worksize;
} AdxtParam;

extern AfsPartition gAfsPartitions[4];
extern AdxtParam gAdxtParams[];
extern u8 gAdxtWork[];

/* Partition loads by file name (the third argument is the directory).
 * The second is a CRI library call that no evidence names yet. */
s32 func_0015E628(s32 ptid, char *fname, void *dir, void *ptinfo);

/* CRI ADX error callback (ADXT_EntryErrFunc): ignores the object and the message. */
void Adx_ErrorCallback(void *obj, char *msg)
{
}

/*
 * Starts ADX: sets Adx_ErrorCallback as the ADXT error function, creates
 * one ADXT player per stream channel into gStreamAdxt (each retried
 * until it works, each with 197092 bytes of gAdxtWork and its
 * gAdxtParams), then loads the AFS partitions of gAfsPartitions,
 * retrying a failed load and waiting for each read to end. Partitions 0
 * and 1 load with ADXF_LoadPartitionNw, partition 2 with the other load
 * call; data.bin is loaded as partition 5 (table of contents at
 * DATA_TOC_BUF) and partition 3 is file 0 inside it. Last come
 * Afs_LoadPartition4, Stream_Init and FileQueue_Init.
 */
void Adx_Init(void)
{
    AdxtParam *adxtParam;
    s32 i;
    u8 *work;
    ADXT *adxtSlot;
    AfsPartition *partition;
    s32 loadResult;

    ADXT_EntryErrFunc(Adx_ErrorCallback, NULL);
    for (adxtParam = gAdxtParams, i = 0, work = gAdxtWork, adxtSlot = gStreamAdxt; i < STREAM_CHAN_COUNT; i++) {
        do {
            *adxtSlot = ADXT_Create(adxtParam->maxnch, work, adxtParam->worksize);
        } while (*adxtSlot == NULL);
        adxtSlot++;
        work += 197092; /* each player's work area in gAdxtWork */
        adxtParam++;
    }
    partition = gAfsPartitions;
    for (i = 0; i < AFS_BOOT_PARTITIONS; i++, partition++) {
    retry:
        if (i != AFS_PT_MAIN) {
            if (i != AFS_PT_AFS02) {
                loadResult = ADXF_LoadPartitionNw(i, partition->fname, NULL, partition->ptinfo);
            } else {
                loadResult = func_0015E628(i, partition->fname, NULL, partition->ptinfo);
            }
            if (loadResult < 0) {
                goto retry; /* the same partition again, without the loop's step */
            }
            do {
            } while (ADXF_GetPtStat(i) != ADXF_STAT_READEND);
        } else {
            /* data.bin as a partition of its own, its table of contents at 8 MB */
            if (func_0015E628(AFS_PT_BOOT, partition->fname, NULL, (void *)DATA_TOC_BUF) < 0) {
                goto retry; /* as above */
            }
            do {
            } while (ADXF_GetPtStat(AFS_PT_BOOT) != ADXF_STAT_READEND);
        }
        if (i == AFS_PT_MAIN) {
            /* then the main partition: file 0 of data.bin */
            do {
            } while (ADXF_LoadPartitionFromAfsNw(i, AFS_PT_BOOT, 0, partition->ptinfo) < 0);
            do {
            } while (ADXF_GetPtStat(i) != ADXF_STAT_READEND);
        }
    }
    Afs_LoadPartition4();
    Stream_Init();
    FileQueue_Init();
}

/* The boot partitions, and the ADXT players' parameters. The tables of
 * contents' buffers keep their addresses for now. */
extern char gAfs00Name[];
extern char gAfs01Name[];
extern char gAfs02Name[];
extern char gDataBinName[];
extern u32 D_002DF850[];
extern u32 D_002E2D70[];
extern u32 D_002E2ED0[];
extern u32 D_002E3340[];
AfsPartition gAfsPartitions[4] = {
#include "main/001BCCB0.inc"
};
AdxtParam gAdxtParams[4] = {
#include "main/001BCCE0.inc"
};

/* The partitions' file names. */
#pragma rodata on
char gAfs00Name[16] = {
#include "main/001C84F0.inc"
};
char gAfs01Name[16] = {
#include "main/001C8500.inc"
};
char gAfs02Name[16] = {
#include "main/001C8510.inc"
};
char gDataBinName[16] = {
#include "main/001C8520.inc"
};
#pragma rodata off
