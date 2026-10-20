/*
 * ADX start-up: the two stream players and the
 * AFS partitions 0..4 (see include/stream.h).
 */
#include "common.h"
#include "stream.h"
#include "ram_areas.h"

typedef struct AfsPartition {
    char *fname;        /* afs00.afs, afs01.afs, afs02.afs, data.bin */
    void *ptinfo;       /* table of contents buffer */
    s32 ptinfoSize;
} AfsPartition;

typedef struct AdxtParam {
    s32 maxnch;
    s32 worksize;
} AdxtParam;

extern AfsPartition gAfsPartitions[4];
extern AdxtParam gAdxtParams[];
extern u8 gAdxtWork[];

void Adx_ErrorCallback(void *obj, char *msg);
/* partition loads by file name; which of the ADX variants each is (the
 * third argument is the directory) is not known */
s32 ADXF_LoadPartitionNw(s32 ptid, char *fname, void *dir, void *ptinfo);
s32 func_0015E628(s32 ptid, char *fname, void *dir, void *ptinfo);

/*
 * Starts ADX: sets Adx_ErrorCallback as the ADXT error function, creates
 * one ADXT player per stream channel into gStreamAdxt (each retried
 * until it works, each with 0x301E4 bytes of gAdxtWork and its
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
        work += 0x301E4;
        adxtParam++;
    }
    partition = gAfsPartitions;
    for (i = 0; i < 4; i++, partition++) {
    retry:
        if (i != 3) {
            if (i != 2) {
                loadResult = ADXF_LoadPartitionNw(i, partition->fname, NULL, partition->ptinfo);
            } else {
                loadResult = func_0015E628(i, partition->fname, NULL, partition->ptinfo);
            }
            if (loadResult < 0) {
                goto retry;
            }
            do {
            } while (ADXF_GetPtStat(i) != ADXF_STAT_READEND);
        } else {
            /* data.bin as partition 5, its table of contents at 8 MB */
            if (func_0015E628(5, partition->fname, NULL, (void *)DATA_TOC_BUF) < 0) {
                goto retry;
            }
            do {
            } while (ADXF_GetPtStat(5) != ADXF_STAT_READEND);
        }
        if (i == 3) {
            /* partition 3 = file 0 of data.bin */
            do {
            } while (ADXF_LoadPartitionFromAfsNw(i, 5, 0, partition->ptinfo) < 0);
            do {
            } while (ADXF_GetPtStat(i) != ADXF_STAT_READEND);
        }
    }
    Afs_LoadPartition4();
    Stream_Init();
    FileQueue_Init();
}

/* This file's initialised data. */
extern u32 D_001C84F0[];
extern u32 D_001C8500[];
extern u32 D_001C8510[];
extern u32 D_001C8520[];
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

/* This file's read-only data. */
#pragma rodata on
u32 D_001C84F0[4] = {
#include "main/001C84F0.inc"
};
u32 D_001C8500[4] = {
#include "main/001C8500.inc"
};
u32 D_001C8510[4] = {
#include "main/001C8510.inc"
};
u32 D_001C8520[4] = {
#include "main/001C8520.inc"
};
#pragma rodata off
