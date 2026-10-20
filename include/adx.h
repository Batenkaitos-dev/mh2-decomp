#ifndef ADX_H
#define ADX_H

#include "common.h"

/*
 * CRI ADX, EE side: the GCC-built library in main.
 * Only the entry points the Capcom core calls. The names come from the
 * library's own error strings, which name the function that reports
 * them, and from the shape of the ADX API.
 */

typedef void *ADXT;     /* player: 0xC4-byte object, 16 of them at 0x19DF20 */
typedef void *ADXF;     /* file: 0x44-byte object */

/* ADXF_GetStat, ADXF_GetPtStat */
#define ADXF_STAT_STOP 1
#define ADXF_STAT_READING 2
#define ADXF_STAT_READEND 3
#define ADXF_STAT_ERROR 4

/* ADXT_GetStat (ADXT_IsCompleted tests PLAYEND) */
#define ADXT_STAT_STOP 0
#define ADXT_STAT_DECINFO 1
#define ADXT_STAT_PREP 2
#define ADXT_STAT_PLAYING 3
#define ADXT_STAT_DECEND 4
#define ADXT_STAT_PLAYEND 5
#define ADXT_STAT_ERROR 6

/* ADXERR_CallErrFunc1 calls func(obj, message): a0 = obj, a1 = its copy of the message. */
void ADXERR_EntryErrFunc(void (*func)(void *obj, char *msg), void *obj);
void ADXERR_CallErrFunc1(char *msg);

ADXF ADXF_Open(char *fname, void *atr);
ADXF ADXF_OpenAfs(s32 ptid, s32 flid);
void ADXF_Close(ADXF adxf);
s32 ADXF_ReadNw(ADXF adxf, s32 nsct, void *buf);
s32 ADXF_Stop(ADXF adxf);
s32 ADXF_GetStat(ADXF adxf);
s32 ADXF_GetFsizeSct(ADXF adxf);
/* Partition ptid = the AFS archive stored as file rd_flid of partition
 * rd_ptid; ptinfo receives its table of contents. */
s32 ADXF_LoadPartitionFromAfsNw(s32 ptid, s32 rd_ptid, s32 rd_flid, void *ptinfo);
s32 ADXF_GetPtStat(s32 ptid);

void ADXT_EntryErrFunc(void (*func)(void *obj, char *msg), void *obj); /* forwards to ADXERR_EntryErrFunc */
ADXT ADXT_Create(s32 maxnch, void *work, s32 worksize);
void ADXT_StartAfs(ADXT adxt, s32 ptid, s32 flid);
void ADXT_StartSeamless(ADXT adxt);
void ADXT_Stop(ADXT adxt);
s32 ADXT_GetStat(ADXT adxt);
void ADXT_SetOutVol(ADXT adxt, s32 vol);    /* 0.1 dB, -999..0 */
void ADXT_SetWaitPlayStart(ADXT adxt, s32 flag);
s32 ADXT_IsReadyPlayStart(ADXT adxt);
void ADXT_Pause(ADXT adxt, s32 sw);
void ADXT_SetOutputMono(s32 flag);

#endif /* ADX_H */
