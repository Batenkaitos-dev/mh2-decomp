/*
 * Accessors for the loaded stage (map) data. When gQuest.stage is set, the
 * data came from a file and its tables hold offsets from the file's base
 * gStageFile; otherwise they hold plain pointers. Per-area tables are
 * indexed through Map_AreaIndex(map, area) (0xFFFF: no such area). The
 * gathering tables (Gather_GetTable) and gathering points
 * (Gather_GetPoints) come from here.
 */
#include "common.h"
#include "scene.h"
#include "quest.h"
#include "area.h"

extern u32 gStageFile;      /* base address of the stage file */
extern IdOffset gOnlineStageData[];
extern IdOffset gOnlineSeasonalData[];

/* unit's own declaration: with the definition's u16 (u16 area) (sub_main/unk_376C00.c), Stage_GetAreaRecord: 1 of 27 words differ */
u16 Area_IdForCurrentTime(s32 x);
u16 Map_AreaIndex(u16 map, u16 area);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Quest_GetSeason(void);
u8 Daytime_Get(void);
s32 LobbyTime_IsAm(void);

/*
 * The stage data of id `id`: offline, from the stage file's id/offset list
 * (gQuest.dataTable; NULL with no stage file); online, from the built-in
 * list gOnlineStageData. NULL when the id is not listed.
 */
void *Stage_FindData(u16 id)
{
    IdOffset *entry;

    if (gScene.online != 1) {
        if (gQuest.stage == 0) {
            return NULL;
        }
        for (entry = (*(IdOffset * *)&gQuest.dataTable);; entry++) {
            if (entry->id == 0) {
                return NULL;
            }
            if (entry->id == id) {
                return (void *)(entry->offset + gStageFile);
            }
        }
    }
    for (entry = gOnlineStageData;; entry++) {
        if (entry->id == 0) {
            return NULL;
        }
        if (entry->id == id) {
            return (void *)entry->offset;
        }
    }
}

/*
 * Data that changes with the season and the time of day: entry `index` of
 * gQuest.seasonalData (two offsets per entry, the second for stage tier 1)
 * leads to a table of six id/offset pairs, by season (0 to 2) plus 3 at the
 * daytime Daytime_Get gives 1. Returns the data (the pair's first word, an
 * offset in the stage file) and stores the second word in *out. Online, the
 * built-in gOnlineSeasonalData by morning or afternoon (LobbyTime_IsAm).
 */
void *Stage_GetSeasonalData(s32 index, s32 *out)
{
    s32 *offsets;
    IdOffset *table;
    s32 season;
    s32 n;

    *out = 0;
    if (gScene.online != 1) {
        if (gQuest.stage == 0) {
            return NULL;
        }
        offsets = (s32 *)gQuest.seasonalData;
        offsets += index * 2;
        if (gQuest.header->tier == 1) {
            offsets++;
        }
        table = (IdOffset *)(*offsets + gStageFile);
        season = (u8)Quest_GetSeason();
        n = season + Daytime_Get() * 3;
        *out = table[n].offset;
        return (u8 *)gStageFile + table[n].id;
    }
    offsets = (s32 *)gOnlineSeasonalData;
    offsets += index * 2;
    table = (IdOffset *)*offsets;
    n = LobbyTime_IsAm() ^ 1;
    *out = table[n].offset;
    return (void *)table[n].id;
}

/*
 * An area's data block (gQuest.areaData, by Map_AreaIndex): NULL when the
 * map has no such area or the area has none. Without a stage file, the
 * table's first entry.
 */
void *Stage_GetAreaData(s32 area)
{
    s32 *offsets;
    u16 i;

    if (gQuest.stage == 0) {
        offsets = (*(s32 * *)&gQuest.areaData);
        return (void *)*offsets;
    }
    i = Map_AreaIndex(gScene.map, area);
    if (i != AREA_INDEX_NONE) {
        offsets = (*(s32 * *)&gQuest.areaData);
        offsets += i;
        return *offsets == 0 ? NULL : (void *)(*offsets + gStageFile);
    }
    return NULL;
}

/*
 * The AreaRecord of area `area` as it is at this time of day
 * (Area_IdForCurrentTime); the first record without a stage file or when
 * the map has no such area.
 */
AreaRecord *Stage_GetAreaRecord(u16 area)
{
    AreaRecord *record = gQuest.areaRecords;
    u16 i;

    if (gQuest.stage != 0) {
        i = Map_AreaIndex(gScene.map, Area_IdForCurrentTime(area));
        if (i != AREA_INDEX_NONE) {
            record += i;
        }
    }
    return record;
}

/* An area's gathering points (gQuest.gatherPoints), NULL when it has none
 * or there is no stage file. */
void *Gather_GetPoints(s32 area)
{
    s32 *offsets;
    u16 i;

    if (gQuest.stage == 0) {
        return NULL;
    }
    offsets = (*(s32 * *)&gQuest.gatherPoints);
    i = Map_AreaIndex(gScene.map, area);
    if (i == AREA_INDEX_NONE) {
        return NULL;
    }
    offsets += i;
    return *offsets == 0 ? NULL : (void *)(*offsets + gStageFile);
}

/* An area's ActionSpot list (gQuest.actionSpots), NULL when it has none;
 * without a stage file, the table's first entry. */
void *Stage_GetActionSpots(s32 area)
{
    s32 *offsets = (*(s32 * *)&gQuest.actionSpots);
    u16 i;

    if (gQuest.stage == 0) {
        return (void *)*offsets;
    }
    i = Map_AreaIndex(gScene.map, area);
    if (i == AREA_INDEX_NONE) {
        return NULL;
    }
    offsets += i;
    return *offsets == 0 ? NULL : (void *)(*offsets + gStageFile);
}

/* Gathering table `table` of the stage header's list (StageHeader.tables). */
void *Gather_GetTable(s32 table)
{
    if (gQuest.stage == 0) {
        return (void *)((s32 *)gQuest.header->tables)[table];
    }
    return (void *)(gStageFile + ((s32 *)gQuest.header->tables)[table]);
}

/*
 * A part of a stage entry: `which` 0 its ids, 1 its records. NULL when the
 * part is 0 or -1 in a stage file; without one, the field as it is.
 */
void *StageEntry_GetData(StageEntry *entry, s32 which)
{
    s32 off;

    if (which == 0) {
        off = entry->ids;
    } else {
        off = entry->records;
    }
    if (gQuest.stage == 0) {
        return (void *)off;
    }
    if (off == 0 || off == -1) {
        return NULL;
    }
    return (void *)(off + gStageFile);
}

/*
 * In a table of StageEntry lists by quest progress (gQuest.emSpawns, the
 * monsters' area sets), list `idx`'s entry `id` (an area): its part `which`
 * as StageEntry_GetData gives it, except that 0 and -1 come back as they are.
 * NULL without a stage file, an empty list or no such entry.
 */
void *Stage_FindEntryData(s32 *tbl, s32 id, s32 which, s8 idx)
{
    StageEntry *entry;
    u32 base;
    s32 off;

    if (gQuest.stage == 0) {
        return NULL;
    }
    tbl += idx;
    if (*tbl == 0) {
        return NULL;
    }
    base = gStageFile;
    for (entry = (StageEntry *)(*tbl + base);; entry++) {
        if (entry->id == 0) {
            return NULL;
        }
        if (entry->id == id) {
            break;
        }
    }
    if (which == 0) {
        off = entry->ids;
    } else {
        off = entry->records;
    }
    if (off == 0 || off == -1) {
        return (void *)off;
    }
    return (void *)(off + base);
}

/* The supply box's items: the stage file header's list, or the default
 * header's. */
void *Stage_GetSupplyItems(void)
{
    u32 base = gStageFile;

    if (gQuest.stage == 0) {
        return (void *)gDefaultStageHeader.supply;
    }
    return (void *)(((StageHeader *)base)->supply + base);
}
