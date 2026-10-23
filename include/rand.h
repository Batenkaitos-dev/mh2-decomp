#ifndef RAND_H
#define RAND_H

#include "common.h"

/*
 * The game's random number generator: Rand_Next(stream), a Lehmer generator
 * on 16-bit state, Rand_Seed for its two streams, the streams copied into
 * the save data (Rand_NextSaved, Rand_StepSaved) and Rand_InitGauss's bell
 * curve. With it, a table of flags no code reads (UnreadFlags_*), and
 * Debug_Message, the debug message call the game's build left empty.
 */

void Debug_Message(char *unused0, s32 unused1, s32 unused2, s32 unused3);
void Rand_Seed(u8 seed);
u16 Rand_Next(s32 stream);
u16 Rand_NextSaved(s16 i);
void Rand_StepSaved(void);
void Rand_InitGauss(void);
void UnreadFlags_Init(void);
void UnreadFlags_ClearAll(void);

#endif
