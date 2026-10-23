#ifndef PACKS_H
#define PACKS_H

#include "common.h"

/*
 * Packed files: LzDecompress unpacks compressed data (16-bit units, literals
 * and back-references); Pack_Count, Pack_GetPart, Pack_PartSize and
 * Pack_CopyParts01 read a pack (a count, then each part's offset and size);
 * and AreaPack_ReadPart2 reads the area pack's environment block by block
 * (the fog, the lights, and the sky band through AreaPack_ReadBlock3).
 */

s32 LzDecompress(u8 *src, u8 *dst);
s32 Pack_Count(s32 *pack);
void *Pack_GetPart(u8 *pack, s32 i);
s32 Pack_PartSize(u8 *pack, s32 i);
void Pack_CopyParts01(u8 *pack, u8 *firstDst, u8 *secondDst);
u8 *AreaPack_ReadBlock3(u8 *src);
void AreaPack_ReadPart2(s32 *pack);

#endif
