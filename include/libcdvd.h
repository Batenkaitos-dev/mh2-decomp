#ifndef LIBCDVD_H
#define LIBCDVD_H

#include "common.h"

/*
 * The disc drive library's calls that the game makes: the drive's set-up,
 * its state, stopping it and the power-off (libcdvd).
 */

s32 sceCdSync(s32 a, s32 b);
s32 sceCdInit(s32 mode);
s32 sceCdDiskReady(s32 a, s32 b);
s32 sceCdStandby(s32 a, s32 b);
s32 sceCdPowerOff(volatile s32 *stat);
s32 sceCdMmode(s32 media);

#endif
