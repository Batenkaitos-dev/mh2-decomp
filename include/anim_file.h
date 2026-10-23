#ifndef ANIM_FILE_H
#define ANIM_FILE_H

#include "common.h"
#include "motion.h"

/*
 * The motion file reader. Anim_LoadSet converts a motion file into an
 * animation set: one AnimHeader per chunk (Anim_LoadChunk,
 * Anim_ConvertChunk), with its curves and their keys converted into the
 * forms the curve evaluators read (Anim_ConvertKeys*). AnimFile_GetChunk
 * gives a file's chunk, and Anim_GetStartTime and Anim_GetEndTime an
 * animation's time range.
 */

u8 *AnimFile_GetChunk(void *file, s32 n);
s32 Anim_ChunkKeySize(AnimFileChunk *chunk);
s32 Anim_LoadSet(AnimSet *out, u8 *buf, void *file);
void Anim_LoadChunk(AnimHeader *anim, void *file, s32 i);
void Anim_ConvertChunk(void *file, AnimHeader *anim, AnimFileChunk *chunk, s32 unused);
void Anim_ConvertKeys(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysF2(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysF4(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysFM(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysS2(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysS4(AnimFileCurve *rec, AnimCurve *curve);
void Anim_ConvertKeysSM(AnimFileCurve *rec, AnimCurve *curve);
f32 Anim_GetEndTime(AnimHeader *anim);
f32 Anim_GetStartTime(AnimHeader *anim);
s32 AnimFile_Count(void *file);

#endif
