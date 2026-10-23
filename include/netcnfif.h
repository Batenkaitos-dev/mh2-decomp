#ifndef NETCNFIF_H
#define NETCNFIF_H

#include "common.h"

/*
 * The network configuration interface's calls that the game makes: reading
 * the network settings files (netcnfif).
 */

s32 sceNetcnfifCheck(void);
s32 sceNetcnfifGetResult(s32 *result);
void sceNetcnfifSetFNoDecode(s32 flag);
void sceNetcnfifGetCount(char *path, s32 type);
s32 sceNetcnfifAllocWorkarea(s32 size);
s32 sceNetcnfifFreeWorkarea(void);
s32 sceNetcnfifConvAuthname(char *src, char *dst, s32 size);

#endif
