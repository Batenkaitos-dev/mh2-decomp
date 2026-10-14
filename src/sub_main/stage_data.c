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

typedef struct IdOffset {
    s32 id;             /* 0 ends the list */
    s32 offset;
} IdOffset;

typedef struct StageEntry {
    s32 id;             /* 0 ends the list */
    s32 unk4;
    s32 offset8;
    s32 offsetC;
} StageEntry;

extern s32 *D_005D8794;
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

void *Stage_FindData(u16 id)
{
    IdOffset *e;

    if (gScene.online != 1) {
        if (gQuest.stage == 0) {
            return NULL;
        }
        for (e = (*(IdOffset * *)&gQuest.dataTable);; e++) {
            if (e->id == 0) {
                return NULL;
            }
            if (e->id == id) {
                return (void *)(e->offset + gStageFile);
            }
        }
    }
    for (e = gOnlineStageData;; e++) {
        if (e->id == 0) {
            return NULL;
        }
        if (e->id == id) {
            return (void *)e->offset;
        }
    }
}

void *Stage_GetSeasonalData(s32 i, s32 *out)
{
    s32 *p;
    IdOffset *e;
    s32 a;
    s32 n;

    *out = 0;
    if (gScene.online != 1) {
        if (gQuest.stage == 0) {
            return NULL;
        }
        p = D_005D8794;
        p += i * 2;
        if (gQuest.header->tier == 1) {
            p++;
        }
        e = (IdOffset *)(*p + gStageFile);
        a = (u8)Quest_GetSeason();
        n = a + Daytime_Get() * 3;
        *out = e[n].offset;
        return (u8 *)gStageFile + e[n].id;
    }
    p = (s32 *)gOnlineSeasonalData;
    p += i * 2;
    e = (IdOffset *)*p;
    n = LobbyTime_IsAm() ^ 1;
    *out = e[n].offset;
    return (void *)e[n].id;
}

void *Stage_GetAreaData64(s32 area)
{
    s32 *p;
    u16 i;

    if (gQuest.stage == 0) {
        p = (*(s32 * *)&gQuest.areaData);
        return (void *)*p;
    }
    i = Map_AreaIndex(gScene.map, area);
    if (i != 0xFFFF) {
        p = (*(s32 * *)&gQuest.areaData);
        p += i;
        return *p == 0 ? NULL : (void *)(*p + gStageFile);
    }
    return NULL;
}

AreaRecord *Stage_GetAreaRecord(u16 x)
{
    AreaRecord *p = gQuest.areaRecords;
    u16 i;

    if (gQuest.stage != 0) {
        i = Map_AreaIndex(gScene.map, Area_IdForCurrentTime(x));
        if (i != 0xFFFF) {
            p += i;
        }
    }
    return p;
}

void *Gather_GetPoints(s32 area)
{
    s32 *p;
    u16 i;

    if (gQuest.stage == 0) {
        return NULL;
    }
    p = (*(s32 * *)&gQuest.gatherPoints);
    i = Map_AreaIndex(gScene.map, area);
    if (i == 0xFFFF) {
        return NULL;
    }
    p += i;
    return *p == 0 ? NULL : (void *)(*p + gStageFile);
}

void *Stage_GetActionSpots(s32 area)
{
    s32 *p = (*(s32 * *)&gQuest.actionSpots);
    u16 i;

    if (gQuest.stage == 0) {
        return (void *)*p;
    }
    i = Map_AreaIndex(gScene.map, area);
    if (i == 0xFFFF) {
        return NULL;
    }
    p += i;
    return *p == 0 ? NULL : (void *)(*p + gStageFile);
}

void *Gather_GetTable(s32 table)
{
    if (gQuest.stage == 0) {
        return (void *)((s32 *)gQuest.header->tables)[table];
    }
    return (void *)(gStageFile + ((s32 *)gQuest.header->tables)[table]);
}

void *StageEntry_GetData(StageEntry *e, s32 which)
{
    s32 off;

    if (which == 0) {
        off = e->offset8;
    } else {
        off = e->offsetC;
    }
    if (gQuest.stage == 0) {
        return (void *)off;
    }
    if (off == 0 || off == -1) {
        return NULL;
    }
    return (void *)(off + gStageFile);
}

void *Stage_FindEntryData(s32 *tbl, s32 id, s32 which, s8 idx)
{
    StageEntry *e;
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
    for (e = (StageEntry *)(*tbl + base);; e++) {
        if (e->id == 0) {
            return NULL;
        }
        if (e->id == id) {
            break;
        }
    }
    if (which == 0) {
        off = e->offset8;
    } else {
        off = e->offsetC;
    }
    if (off == 0 || off == -1) {
        return (void *)off;
    }
    return (void *)(off + base);
}

void *Stage_GetSupplyItems(void)
{
    u32 base = gStageFile;

    if (gQuest.stage == 0) {
        return (void *)gDefaultStageHeader.supply;
    }
    return (void *)(((StageHeader *)base)->supply + base);
}
