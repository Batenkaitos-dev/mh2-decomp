/*
 * main 0x192ef0-0x1930e0: ADX start-up: the two stream players and the
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

void Adx_Init(void)
{
    AdxtParam *p;
    s32 i;
    u8 *work;
    ADXT *h;
    AfsPartition *pt;
    s32 r;

    ADXT_EntryErrFunc(Adx_ErrorCallback, NULL);
    for (p = gAdxtParams, i = 0, work = gAdxtWork, h = gStreamAdxt; i < STREAM_CHAN_COUNT; i++) {
        do {
            *h = ADXT_Create(p->maxnch, work, p->worksize);
        } while (*h == NULL);
        h++;
        work += 0x301E4;
        p++;
    }
    pt = gAfsPartitions;
    for (i = 0; i < 4; i++, pt++) {
    retry:
        if (i != 3) {
            if (i != 2) {
                r = ADXF_LoadPartitionNw(i, pt->fname, NULL, pt->ptinfo);
            } else {
                r = func_0015E628(i, pt->fname, NULL, pt->ptinfo);
            }
            if (r < 0) {
                goto retry;
            }
            do {
            } while (ADXF_GetPtStat(i) != ADXF_STAT_READEND);
        } else {
            /* data.bin as partition 5, its table of contents at 8 MB */
            if (func_0015E628(5, pt->fname, NULL, (void *)DATA_TOC_BUF) < 0) {
                goto retry;
            }
            do {
            } while (ADXF_GetPtStat(5) != ADXF_STAT_READEND);
        }
        if (i == 3) {
            /* partition 3 = file 0 of data.bin */
            do {
            } while (ADXF_LoadPartitionFromAfsNw(i, 5, 0, pt->ptinfo) < 0);
            do {
            } while (ADXF_GetPtStat(i) != ADXF_STAT_READEND);
        }
    }
    Afs_LoadPartition4();
    Stream_Init();
    FileQueue_Init();
}

/* unk_192EF0's .data (0x1BCCB0-0x1BCD00) */
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

/* unk_192EF0's .rodata (0x1C84F0-0x1C8530) */
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
