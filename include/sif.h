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

/* Sony's kernel thread parameters (CreateThread). */
struct ThreadParam {
    s32 status;
    void *entry;
    void *stack;
    s32 stackSize;
    void *gpReg;
    s32 initPriority;
    s32 currentPriority;
    u32 attr;
    u32 option;
    s32 waitType;
    s32 waitId;
    s32 wakeupCount;
};

void sceSifInitRpc(u32 mode);
s32 sceSifBindRpc(SifClientData *cd, u32 id, s32 mode);
s32 sceSifCallRpc(SifClientData *cd, s32 fno, u32 mode, void *send, s32 ssize, void *recv, s32 rsize,
                  void (*end_func)(void *), void *end_param);
s32 sceSifCheckStatRpc(SifClientData *cd);
s32 sceOpen(void *, s32, s32); /* open */
s32 sceClose(s32); /* close */
s32 sceRead(s32, void *, s32);
s32 sceWrite(s32, void *, s32);
s32 RemoveIntcHandler(s32 cause, s32 id);
s32 RemoveDmacHandler(s32 channel, s32 id);
s32 CreateThread(struct ThreadParam *);
s32 DeleteThread(s32);
s32 TerminateThread(s32);
s32 sceSifUnloadModule(s32 id); /* unload a module */
s32 sceSifSearchModuleByName(char *name); /* find a loaded module by name */
s32 sceSifLoadModule(char *path, s32 argc, char *argv); /* load a module */
void DIntr(void);
void EIntr(void);
s32 CreateSema(SemaParam *p);
s32 WaitSema(s32 sema);
void FlushCache(s32 mode);
void DisableIntc(s32 a);
void DisableDmac(s32 a);
s32 DelayThread(s32);
void sceSifExitCmd(void);
s32 SignalSema(s32 sema);

#endif /* SIF_H */
