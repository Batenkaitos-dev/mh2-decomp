#ifndef STAGE_DATA_H
#define STAGE_DATA_H

#include "common.h"
#include "quest.h"

/*
 * Accessors for the loaded stage (map) data: its entries by id, the
 * per-area records (Stage_GetAreaRecord), the gathering tables and points
 * (Gather_GetTable, Gather_GetPoints), the action spots and the supply
 * box's items.
 */

void *Stage_FindData(u16 id);
void *Stage_GetSeasonalData(s32 index, s32 *out);
AreaRecord *Stage_GetAreaRecord(u16 area);
void *Gather_GetPoints(s32 area);
void *Stage_GetActionSpots(s32 area);
void *Gather_GetTable(s32 table);
void *StageEntry_GetData(StageEntry *entry, s32 which);
void *Stage_FindEntryData(s32 *tbl, s32 id, s32 which, s8 idx);
void *Stage_GetSupplyItems(void);

#endif
