#ifndef LIBKERNL_H
#define LIBKERNL_H

#include "common.h"

/*
 * The EE kernel's calls that the game makes: threads, semaphores, interrupt
 * and DMA handlers, the caches, the timers and SIF DMA (libkernl, linked
 * into the boot executable). The SIF RPC calls are sif.h's.
 */

s32 AddIntcHandler(s32 cause, s32 (*handler)(s32), s32 next);
s32 AddDmacHandler(s32 ch, s32 (*handler)(s32), s32 next);
s32 ChangeThreadPriority(s32 tid, s32 prio);
s32 GetThreadId(void);
s32 SignalSema(s32 sema);
s32 iSignalSema(s32 sema);
s32 WaitSema(s32 sema);
void FlushCache(s32 kind);
void iFlushCache(s32 mode);
void SyncDCache(void *start, void *end);
s32 EnableIntc(s32 cause);
s32 EnableDmac(s32 ch);
s32 iEnableIntc(s32 cause);
s32 iDisableIntc(s32 cause);
s32 DelayThread(s32 usec);
void scePrintf(const char *fmt, ...);
s32 sceDevctl(char *name, s32 cmd, void *arg, u32 arglen, void *buf, u32 buflen);
s32 sceSifStopModule(s32 id, s32 argc, char *argv, s32 *result); /* stop a module */
s32 sceSifUnloadModule(s32 id); /* unload a module */
s32 sceSifSearchModuleByName(char *name);
s32 sceSifLoadModule(char *path, s32 argc, char *argv);
s32 sceSifLoadStartModule(char *path, s32 argc, char *argv, void *buf);
s64 GetTimerSystemTime(void);
void TimerBusClock2USec(s64, u32 *, u32 *);

#endif
