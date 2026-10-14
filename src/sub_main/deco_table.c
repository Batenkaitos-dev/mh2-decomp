/*
 * The decoration table (gDecoTable: 125 records of 16 bytes, indexed by
 * ItemData.deco), Capcom's data in the .data between ACCESS's kana-kanji
 * engine's units (0x403DF0-0x4045C0). The engine doesn't use it; the item
 * code does (unk_356380.c, unk_3B4C80.c).
 */
#include "common.h"

u32 gDecoTable[500] = {
#include "sub_main/00403DF0.inc"
};
