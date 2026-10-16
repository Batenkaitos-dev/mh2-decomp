/*
 * 0x18CC20: main, the file systems, file loading by id, LoadOverlay, save-data accessors
 */
#include "common.h"
#include "save.h"
#include "syssettings.h"
#include "quest.h"
#include "scene.h"
/* The MWo3 header of an overlay file. */
typedef struct OverlayHeader {
    u32 magic;          /* "MWo" and the overlay id */
    u32 id;
    u32 load;           /* 0x08 load address (the header is loaded too) */
    u32 textSize;
    u32 dataSize;
    u32 bssSize;        /* 0x14 */
    u32 end;            /* 0x18 load + file size */
    u32 end2;
    char name[0x20];
} OverlayHeader;

typedef struct IdPair {
    u16 id;
    u16 val;
} IdPair;

/* CRI ADX: the CD/DVD file system and its cache (0flist.dir) setup. */
typedef struct AdxDvdFsParam {
    char *root;
    s32 unk4[4];
} AdxDvdFsParam;

typedef struct AdxCacheParam {
    char *list;
    s32 nfile;
    void *buf;
    s32 nsct;
} AdxCacheParam;

/* The record of the last quest (sub_main 0x5D4920, 0xDC bytes): cleared by
 * QuestRecord_Clear when a quest starts or ends, filled by QuestRecord_Fill
 * for the results. */
typedef struct QuestRecord {
    u16 questId;        /* 0x00 (0x5D8268) */
    u8 pad02[2];
    u8 unk04;           /* 0x04 (0x6ADCF0) */
    s8 unk05;           /* 0x05: Area_MapGroup of the quest's target */
    s8 unk06;           /* 0x06 (0x5D83BC) */
    s8 unk07;           /* 0x07 */
    s32 unk08;          /* 0x08 (0x5D878C) */
    u8 counts[0x10];    /* 0x0C: per id of gQuestRecordIds (QuestRecord_Count) */
    u8 kindA[0x60];     /* 0x1C: per monster kind */
    u8 kindB[0x60];     /* 0x7C: per monster kind */
} QuestRecord;

extern u8 gSeasonFlags;         /* sub_main 0x5D4910 */
extern s32 gLoadLastVSync;      /* sub_main 0x5D4A50: gVSyncCount when the loader last ran a frame */
extern u8 gQuestSeasonFlags;    /* sub_main 0x5D87A9: the quest's season (Quest_GetSeason) */
extern u8 gOverlayWindow[];
extern u32 gOverlayWindowBase[];
extern u16 *gHrGateQuests[];
extern IdPair gDojoParTimes[];
extern char gArgv[3][0x40];
extern u8 gAdxCacheWork[];
extern s32 gVSyncCount;
extern s32 gLoadingScreenOn;
extern QuestRecord gQuestRecord;
extern u8 gLobbyRateStates[];
extern u16 gQuestRecordIds[];
u16 Area_MapGroup(u16, s32); /* unit's own declaration: the definition (sub_main) is u16 Area_MapGroup(u16 area), which ignores a1; QuestRecord_Fill passes a1 = v, and without it changes by 8 of 98 words */
s32 Quest_IsRunning(s32 ending); /* the definition (sub_main) is s32 Quest_IsRunning(s32 ending): the call here passes a second argument */
extern s32 gRateSnapshot[];

s32 Afs_ReadOverlay(s32 flid, void *buf);
void *memset(void *dst, s32 c, u32 n);
void *memcpy(void *dst, const void *src, u32 n);
s32 printf(const char *fmt, ...);          /* printf */
char *strcpy(char *dst, const char *src);  /* strcpy */
s32 InitDisplay(s32 w, s32 h);
void Sound_Init(void);
void Adx_InitFileSystems(void);
void Sound_Boot(void);
void LoadOverlay(s32 id);
void Sys_EmptyHook(void);
void sub_main_entry(void);
void func_001657B8(s32);
void func_00165400(AdxDvdFsParam *);
void func_00165500(AdxCacheParam *);
void func_00162220(s32);
void Movie_InitSfd(s32);
s32 Overlay_IsLoaded(s32 id);
void FileQueue_Flush(void);
void Load_Idle(void);
void *ADXF_OpenAfs(s32 ptid, s32 flid);
s32 ADXF_GetFsizeSct(void *adxf);
s32 ADXF_ReadNw(void *adxf, s32 nsct, void *buf);
s32 ADXF_GetStat(void *adxf);
void ADXF_Close(void *adxf);
s32 Sys_FlipFrame(s32 unused0);
void func_00128550(void *addr, s32 size);
s32 Overlay_ReadFile(s32 idx, s32 base, s32 dst);
s32 Overlay_Relocate(void *data, s32 *bss);
void Sys_EnableGameVSync(void);
void Overlay_LoadGmSub(void);
void PatchExecCS(u8 mode, u8 sub);
s32 Save_TestListBit(u16 item);

s32 main(s32 argc, char **argv)
{
    s32 i;

    if (argc > 0) {
        for (i = 0; i < argc; i++) {
            if (i >= 3) {
                break;
            }
            strcpy(gArgv[i], argv[i]);
        }
    }
    while (InitDisplay(0x200, 0x1C0) == 0) {
    }
    Sound_Init();
    Adx_InitFileSystems();
    Sound_Boot();
    LoadOverlay(1);
    Sys_EmptyHook();
    sub_main_entry();
    return 0;
}

void Adx_InitFileSystems(void)
{
    AdxDvdFsParam dvd;
    AdxCacheParam cache;

    printf("Setup HOST file system.\n");
    func_001657B8(0);
    printf("Setup CD/DVD file system.\n");
    memset(&dvd, 0, sizeof(dvd));
    dvd.root = "\\";
    func_00165400(&dvd);
    printf("Setup CD/DVD cache system.\n");
    memset(&cache, 0, sizeof(cache));
    cache.list = "0flist.dir";
    cache.nfile = 0x20;
    cache.nsct = 0x215;
    cache.buf = gAdxCacheWork;
    func_00165500(&cache);
    func_00162220(0);
    Movie_InitSfd(0);
}

s32 Afs_ReadId(u32 id, void *dst)
{
    void *adxf;
    s32 ptid;
    s32 flid;
    s32 nsct;
    s32 stat;

    if (Overlay_IsLoaded(1) == 1) {
        FileQueue_Flush();
    }
    ptid = (id >> 16) & 0xFFFF;
    flid = id & 0xFFFF;
    do {
        do {
            adxf = ADXF_OpenAfs(ptid, flid);
        } while (adxf == NULL);
        nsct = ADXF_GetFsizeSct(adxf);
        ADXF_ReadNw(adxf, nsct, dst);
        for (;;) {
            stat = ADXF_GetStat(adxf);
            if (stat != 2) {
                break;
            }
            if (Overlay_IsLoaded(1) == 1) {
                Load_Idle();
            }
            if (gLoadLastVSync != gVSyncCount) {
                gLoadLastVSync = gVSyncCount;
                if (gLoadingScreenOn != 0) {
                    Sys_FlipFrame(0);
                }
            }
        }
        ADXF_Close(adxf);
    } while (stat != 3);
    return nsct << 11;
}

/*
 * Puts an overlay file read at data where its MWo header says it goes (the
 * header is part of the image) and flushes the data cache over it; returns
 * its size including .bss (0 when data is not an overlay).
 */
s32 Overlay_Relocate(void *data, s32 *bss)
{
    s32 size;
    void *dst;
    OverlayHeader *hdr;

    *bss = 0;
    hdr = data;
    if ((hdr->magic & 0xFFFFFF) != 0x6F574D) {
        return 0;
    }
    size = hdr->end - hdr->load;
    if (size <= 0) {
        return 0;
    }
    *bss = hdr->bssSize;
    dst = (void *)hdr->load;
    memcpy(dst, hdr, size);
    func_00128550(dst, size);
    return size + *bss;
}

/* Reads overlay file idx to dst; returns the end of what was read (base is unused). */
s32 Overlay_ReadFile(s32 flid, s32 unused, s32 buf)
{
    return buf + Afs_ReadOverlay(flid, (void *)buf);
}

/* Loads gm_sub (overlay 6) into its window unless it is already there. */
void Overlay_LoadGmSub(void)
{
    u32 addr;
    s32 bss;

    if (gSysSettings.gmSubLoaded == 0) {
        FileQueue_Flush();
        addr = gOverlayWindowBase[gOverlayWindow[5]];
        Overlay_ReadFile(5, (s32)gSysSettings.loadBuf, addr);
        Overlay_Relocate((OverlayHeader *)addr, &bss);
        gSysSettings.gmSubAddr = addr;
        gSysSettings.gmSubLoaded = 1;
        Sys_EnableGameVSync();
    }
}

void LoadOverlay(s32 id)
{
    s32 region;
    s32 idx;
    SysSettings *st;
    u32 addr;
    s32 bss;

    st = &gSysSettings;
    if (id > 0) {
        idx = id - 1;
        region = gOverlayWindow[idx];
        if (region == 2) {
            if (id != 4) {
                Overlay_LoadGmSub();
            }
            if (st->loaded[region - 2] == id) {
                return;
            }
        }
        if (id != 1) {
            FileQueue_Flush();
        }
        addr = gOverlayWindowBase[region];
        if (id == 10) {
            gSysSettings.lbguildFree = addr;
            gSysSettings.lbguildFree += 0x30000;
            gSysSettings.lbguildFreeSize = gSysSettings.patchArea;
            gSysSettings.lbguildFreeSize -= gSysSettings.lbguildFree;
        }
        if (id == 1) {
            Overlay_ReadFile(idx, 0x800000, addr);
        } else {
            Overlay_ReadFile(idx, (s32)gSysSettings.loadBuf, addr);
        }
        Overlay_Relocate((OverlayHeader *)addr, &bss);
        if (region == 3 || region == 5) {
            st->gmSubLoaded = 0;
        }
        if (id != 1 && id != 10 && id != 11) {
            st->loaded[region - 2] = id;
        } else {
            if (id == 1) {
                st->subMainLoaded = 1;
                return;
            }
            if (id == 10 || id == 5) {
                gSysSettings.gmSubLoaded = 0;
            }
            if (id == 11) {
                gSysSettings.plselLoaded = 1;
            }
        }
        switch (id) {
        case 2:
            PatchExecCS(2, 1);
            break;
        case 3:
            PatchExecCS(2, 2);
            break;
        case 4:
            PatchExecCS(2, 3);
            break;
        case 11:
            PatchExecCS(6, 1);
            break;
        case 8:
        case 7:
            return;
        }
        if (gSysSettings.gmSubLoaded == 1) {
            PatchExecCS(3, 2);
        }
    }
}

s32 Overlay_IsLoaded(s32 id)
{
    if (id < 0) {
        return 0U;
    }
    switch (id) {
    case 0:
        return 1U;
    case 1:
        return gSysSettings.subMainLoaded;
    default:
        return gSysSettings.loaded[gOverlayWindow[id - 1] - 2] == id;
    }
}

/* 1 when every quest of HR gate n (0-2: HR 11, 17 and 31) is cleared. */
s32 Save_HrGateCleared(s32 n)
{
    u16 *p = gHrGateQuests[n];

    while (*p != 0) {
        if (Save_TestListBit(*p) == 0) {
            return 0;
        }
        p++;
    }
    return 1;
}

/*
 * The training quests: gDojoParTimes pairs a quest id with a target time in
 * minutes; the best score and time (in frames) of each are in sub_main.
 */
u16 Dojo_GetParMinutes(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].id != 0; i++) {
        if (id == gDojoParTimes[i].id) {
            return gDojoParTimes[i].val;
        }
    }
    return 0;
}

s32 Dojo_FindQuest(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].id != 0; i++) {
        if (id == gDojoParTimes[i].id) {
            return i;
        }
    }
    return -1;
}

s32 Dojo_GetBestScore(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].id != 0; i++) {
        if (id == gDojoParTimes[i].id) {
            return gSave.dojoScore[i];
        }
    }
    return 0;
}

s32 Dojo_GetBestTime(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].id != 0; i++) {
        if (id == gDojoParTimes[i].id) {
            return gSave.dojoTime[i];
        }
    }
    return 0;
}

void Rate_ClearSnapshot(void)
{
    s32 i;

    for (i = 0; i < 0x4A; i++) {
        gRateSnapshot[i] = 0;
    }
    gSeasonFlags = 0;
}

s32 Rate_GetSnapshot(s32 i)
{
    return gRateSnapshot[i];
}

s32 Rate_Get(s32 i)
{
    return gSave.rateBase[i];
}

/* Copies the monster rates of the save data out: the first 0x4A values
 * (as s32) and the trend and level bytes. */
void Rate_TakeSnapshot(void)
{
    SaveData *save = &gSave;
    s32 i;

    for (i = 0; i < 0x4A; i++) {
        gRateSnapshot[i] = save->rateBase[i];
    }
    memcpy(gLobbyRateStates, save->rateState, 0x60);
}

void *QuestRecord_Clear(void)
{
    return memset(&gQuestRecord, 0, sizeof(QuestRecord));
}

/* Fills the record at gQuestRecord (0xDC bytes, cleared by QuestRecord_Clear)
 * from sub_main's state; its three tables are cleared unless the player's
 * slot and Quest_IsRunning agree. */
/* The pointer to the u16 at q + 0x20 taken before the stores and the
 * compiled-out check of the tier (0 to 3; any two-test condition gives the
 * same code) give the game's registers and load order (found by the
 * permuter, as `c && d`). The record's s32 is stored
 * through a cast of the field (`*(s32 *)&gQuestRecord.unk08`, which aliases
 * like a raw address): stored as the field, the stores are scheduled
 * differently. */
void QuestRecord_Fill(void)
{
    QuestRecord *rec = &gQuestRecord;
    s32 v;
    u16 a;
    u8 b;
    u8 * q;
    s8 c;
    s8 d;
    u8 *qs;
    u8 n;
    s32 i;
    s32 j;

    v = gQuest.elapsed;
    a = gQuest.stage;
    b = gScene.questPhase;
    q = (u8 *)gQuest.info;
    qs = q + 0x20;
    *(s32 *)&gQuestRecord.unk08 = v;
    gQuestRecord.questId = a;
    gQuestRecord.unk04 = b;
    gQuestRecord.unk05 = Area_MapGroup(*(u16 *)qs, v);
    c = gQuest.header->tier;
    d = gQuest.flyer;
    if (c < 0 || c > 3) { /* compiled-out check */
    }
    n = gScene.playerCount;
    gQuestRecord.unk07 = c;
    gQuestRecord.unk06 = d;
    for (i = 0; i < n; i++) {
        if (gScene.inQuest[i] == 1) {
            break;
        }
    }
    if (i != gScene.localPlayer || Quest_IsRunning(1) != 1) {
        for (j = 0; j < 0x10; j++) {
            rec->counts[j] = 0;
        }
        for (i = 0; i < 0x60; i++) {
            rec->kindA[i] = 0;
            rec->kindB[i] = 0;
        }
    }
}

void QuestRecord_Count(u16 id)
{
    s32 i;
    u16 *p = gQuestRecordIds;

    for (i = 0; i < 0x10; i++, p++) {
        if (*p == 0) {
            break;
        }
        if (*p == id) {
            if (gQuestRecord.counts[i] < 0xFF) {
                gQuestRecord.counts[i]++;
            }
            break;
        }
    }
}

/* The season, 0-2, from sub_main's season bits (Season_Set sets one). */
s32 Season_Get(void)
{
    u8 f = gSeasonFlags;
    s32 r;

    if (f & 1) {
        r = 0;
    } else if (f & 2) {
        r = 1;
    } else {
        r = 2;
    }
    return r;
}

void Season_Set(s32 season)
{
    gSeasonFlags = (u8) (gSeasonFlags & ~7);
    gSeasonFlags = (u8) (gSeasonFlags | ((1 << (season & 0xFF)) & 0xFF));
}

/* The current quest's season, 0-2 (the same bits in the quest's data). */
s32 Quest_GetSeason(void)
{
    u8 f = gQuestSeasonFlags;
    s32 r;

    if (f & 1) {
        r = 0;
    } else if (f & 2) {
        r = 1;
    } else {
        r = 2;
    }
    return r;
}

/* main's .data (0x1BB130-0x1BB2A0) */
u8 gOverlayWindow[16] = {
#include "main/001BB130.inc"
};
u32 gPresetColors[16] = {
#include "main/001BB140.inc"
};
u16 D_001BB180[8] = {
#include "main/001BB180.inc"
};
u16 D_001BB190[8] = {
#include "main/001BB190.inc"
};
u16 D_001BB1A0[8] = {
#include "main/001BB1A0.inc"
};
u16 D_001BB1B0[8] = {
#include "main/001BB1B0.inc"
};
u16 D_001BB1C0[8] = {
#include "main/001BB1C0.inc"
};
u16 D_001BB1D0[8] = {
#include "main/001BB1D0.inc"
};
u16 *gHrGateQuests[6] = {
    D_001BB180, D_001BB190, D_001BB1A0, D_001BB1B0,
    D_001BB1C0, D_001BB1D0,
};
#pragma explicit_zero_data on
u32 D_001BB1F8[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
IdPair gDojoParTimes[24] = {
#include "main/001BB200.inc"
};
u32 D_001BB260[8] = {
#include "main/001BB260.inc"
};
u16 gQuestRecordIds[16] = {
#include "main/001BB280.inc"
};
