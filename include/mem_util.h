#ifndef MEM_UTIL_H
#define MEM_UTIL_H

#include "common.h"

/*
 * Memory helpers: a byte fill and a byte copy (Mem_Fill, Mem_Copy),
 * wrappers over the system heap gSysHeap (SysHeap_*), and the main heap's
 * blocks the movie player uses (Mem_Alloc, Mem_GetAddr, Mem_Free,
 * Mem_Compact).
 */

void Mem_Fill(void *dst, u8 value, s32 n);
void Mem_Copy(void *dst, void *src, s32 n);
void *SysHeap_Alloc(u32 size);
s32 SysHeap_GetMark(void *mark);
void SysHeap_Release(void *mark);
void *SysHeap_AllocTop(u32 size);
void *Mem_Alloc(u32 size, s32 flag);
void Mem_Free(void *handle);
void *Mem_GetAddr(void *handle);
void Mem_Compact(void);

#endif
