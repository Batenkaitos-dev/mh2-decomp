#ifndef SIF_H
#define SIF_H

/*
 * Sony libkernl types and calls used by the game's SIF RPC clients (the
 * CSE sound engine in main, the AVE network wrapper in sub_main).
 */
#include "common.h"

typedef struct SifRpcData {
    void *paddr;
    u32 pid;
    s32 tid;
    u32 mode;
} SifRpcData;

typedef struct SifClientData {
    SifRpcData rpcd;
    u32 command;
    void *buff;
    void *cbuff;
    void (*end_function)(void *);
    void *end_param;
    void *serve;        /* 0x24: set once the server is bound */
} SifClientData;

typedef struct SemaParam {
    s32 currentCount;
    s32 maxCount;
    s32 initCount;
    s32 numWaitThreads;
    u32 attr;
    u32 option;
} SemaParam;

void sceSifInitRpc(u32 mode);
s32 sceSifBindRpc(SifClientData *cd, u32 id, s32 mode);
s32 sceSifCallRpc(SifClientData *cd, s32 fno, u32 mode, void *send, s32 ssize, void *recv, s32 rsize,
                  void (*end_func)(void *), void *end_param);
s32 sceSifCheckStatRpc(SifClientData *cd);
s32 CreateSema(SemaParam *p);
s32 WaitSema(s32 sema);
s32 SignalSema(s32 sema);

#endif /* SIF_H */
