#ifndef LIBMC_H
#define LIBMC_H

#include "common.h"

/*
 * The memory card library's calls that the game makes: opening, reading,
 * writing and listing files, formatting a card (libmc).
 */

s32 sceMcInit(void);
s32 sceMcOpen(s32 port, s32 slot, char *name, s32 mode);
s32 sceMcMkdir(s32 a, s32 b, char *name);
s32 sceMcClose(s32 fd);
s32 sceMcRead(s32 fd, u8 *buf, s32 size);
s32 sceMcWrite(s32 fd, u8 *buf, s32 size);
s32 sceMcSync(s32 port, s32 *a, s32 *b);
s32 sceMcGetInfo(s32 port, s32 slot, s32 *type, s32 *free, s32 *format);
s32 sceMcFormat(s32 a, s32 b);
s32 sceMcDelete(s32 a, s32 b, char *name);

#endif
