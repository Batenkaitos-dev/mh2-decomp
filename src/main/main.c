/*
 * The boot executable's main and the services every overlay uses.
 * - main: sets up the display, the sound and CRI's file systems, loads
 *   sub_main (include/overlay.h) and runs it; sub_main_entry never returns.
 * - Afs_ReadId: reads one file of the disc's AFS archives, by an id that
 *   holds the archive and the file, while the loading screen goes on.
 * - LoadOverlay and its helpers: read an overlay into its window
 *   (gOverlayWindow, gOverlayWindowBase), relocate it, and record what each
 *   window holds in gSysSettings.
 * - Small accessors for sub_main's and the lobby's state that main keeps:
 *   the HR gates, the training quests' par times and records, the monster
 *   rates' snapshot, the quest record (gQuestRecord, sent to the lobby
 *   server after a quest) and the season.
 */
#include "common.h"
#include "save.h"
#include "syssettings.h"
#include "quest.h"
#include "scene.h"
#include "overlay.h"

/* gDojoParTimes' entry: a training quest and its target time. */
typedef struct DojoParTime {
    u16 questId;        /* 0 ends the table */
    u16 minutes;
} DojoParTime;

/* CRI ADX's parameter blocks for the CD/DVD file system and its cache
 * (0flist.dir) set-up; only Adx_InitFileSystems fills them. */
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

/* The monster rates main copies out (Rate_TakeSnapshot): the first of
 * SaveData.rateBase. */
#define RATE_SNAPSHOT_COUNT 0x4A

extern u8 gSeasonFlags;         /* sub_main 0x5D4910 */
extern s32 gLoadLastVSync;      /* sub_main 0x5D4A50: gVSyncCount when the loader last ran a frame */
extern u8 gQuestSeasonFlags;    /* sub_main 0x5D87A9: the quest's season (Quest_GetSeason) */
extern u8 gOverlayWindow[];
extern u32 gOverlayWindowBase[];
extern u16 *gHrGateQuests[];
extern DojoParTime gDojoParTimes[];
#define ARGV_KEPT 3 /* the command-line arguments main keeps */
extern char gArgv[ARGV_KEPT][0x40];
extern u8 gAdxCacheWork[];
extern s32 gVSyncCount;
extern s32 gLoadingScreenOn;
extern QuestRecord gQuestRecord;
extern u8 gLobbyRateStates[];
extern u16 gQuestRecordIds[];
u16 Map_Group(u16, s32); /* unit's own declaration: the definition (sub_main) is u16 Map_Group(u16 map), which ignores a1; QuestRecord_Fill passes a1 = v, and its call compiles as the original's only with that second argument */
s32 Quest_GetOutcome(s32 withLoss); /* the definition (sub_main) is s32 Quest_GetOutcome(s32 withLoss): the call here passes a second argument */
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
s32 Save_IsQuestCleared(u16 item);

/* Keeps the first three arguments in gArgv, sets up the display (512 x 448),
 * the sound and the file systems, then loads and runs sub_main. */
s32 main(s32 argc, char **argv)
{
    s32 i;

    if (argc > 0) {
        for (i = 0; i < argc; i++) {
            if (i >= ARGV_KEPT) {
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
    LoadOverlay(OVERLAY_SUB_MAIN);
    Sys_EmptyHook();
    sub_main_entry();
    return 0;
}

/* Sets up CRI's host and CD/DVD file systems, the CD/DVD cache of the
 * file list (32 files, 0x215 sectors) and Sofdec. */
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

/*
 * Reads a whole file of the disc's AFS archives to dst: id's high half is
 * the archive (CRI's partition id), its low half the file. Waits for
 * sub_main's file queue first. Retries the open until it succeeds, and the
 * read until ADXF reports it complete (status 3; 2 while it reads); while
 * it reads, calls sub_main's Load_Idle and flips a frame of the loading
 * screen once per vsync. Returns the size read in bytes (whole sectors).
 */
s32 Afs_ReadId(u32 id, void *dst)
{
    void *adxf;
    s32 ptid;
    s32 flid;
    s32 nsct;
    s32 stat;

    if (Overlay_IsLoaded(OVERLAY_SUB_MAIN) == 1) {
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
            if (Overlay_IsLoaded(OVERLAY_SUB_MAIN) == 1) {
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
    if ((hdr->magic & OVERLAY_MAGIC_MASK) != OVERLAY_MAGIC) {
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

/* Loads gm_sub into its window unless it is already there. */
void Overlay_LoadGmSub(void)
{
    u32 addr;
    s32 bss;

    if (gSysSettings.gmSubLoaded == 0) {
        FileQueue_Flush();
        addr = gOverlayWindowBase[gOverlayWindow[OVERLAY_GM_SUB - 1]];
        Overlay_ReadFile(OVERLAY_GM_SUB - 1, (s32)gSysSettings.loadBuf, addr);
        Overlay_Relocate((OverlayHeader *)addr, &bss);
        gSysSettings.gmSubAddr = addr;
        gSysSettings.gmSubLoaded = 1;
        Sys_EnableGameVSync();
    }
}

/*
 * Loads overlay id (an OverlayId) into its window and relocates it. The
 * scene window's overlays (select and game, not the lobby) need gm_sub,
 * which is loaded first; an overlay already in the scene window is not
 * read again. Every load but sub_main's waits for sub_main's file queue.
 * Then records what the window holds (gSysSettings.loaded, or the flags of
 * sub_main, gm_sub and plsel) and runs the overlay's patch hook
 * (PatchExecCS) where it has one.
 */
void LoadOverlay(s32 id)
{
    s32 window;
    s32 idx;
    SysSettings *settings;
    u32 addr;
    s32 bss;

    settings = &gSysSettings;
    if (id > OVERLAY_MAIN) {
        idx = id - 1;
        window = gOverlayWindow[idx];
        if (window == OVERLAY_WINDOW_SCENE) {
            if (id != OVERLAY_LOBBY) {
                Overlay_LoadGmSub();
            }
            if (settings->loaded[window - OVERLAY_WINDOW_SCENE] == id) {
                return;
            }
        }
        if (id != OVERLAY_SUB_MAIN) {
            FileQueue_Flush();
        }
        addr = gOverlayWindowBase[window];
        if (id == OVERLAY_LBGUILD) {
            gSysSettings.lbguildFree = addr;
            gSysSettings.lbguildFree += 0x30000;
            gSysSettings.lbguildFreeSize = gSysSettings.patchArea;
            gSysSettings.lbguildFreeSize -= gSysSettings.lbguildFree;
        }
        if (id == OVERLAY_SUB_MAIN) {
            Overlay_ReadFile(idx, 0x800000, addr);
        } else {
            Overlay_ReadFile(idx, (s32)gSysSettings.loadBuf, addr);
        }
        Overlay_Relocate((OverlayHeader *)addr, &bss);
        if (window == OVERLAY_WINDOW_GM_SUB || window == OVERLAY_WINDOW_LBGUILD) {
            settings->gmSubLoaded = 0;
        }
        if (id != OVERLAY_SUB_MAIN && id != OVERLAY_LBGUILD && id != OVERLAY_PLSEL) {
            settings->loaded[window - OVERLAY_WINDOW_SCENE] = id;
        } else {
            if (id == OVERLAY_SUB_MAIN) {
                settings->subMainLoaded = 1;
                return;
            }
            if (id == OVERLAY_LBGUILD || id == OVERLAY_NETHTTP) {
                gSysSettings.gmSubLoaded = 0;
            }
            if (id == OVERLAY_PLSEL) {
                gSysSettings.plselLoaded = 1;
            }
        }
        switch (id) {
        case OVERLAY_SELECT:
            PatchExecCS(2, 1);
            break;
        case OVERLAY_GAME:
            PatchExecCS(2, 2);
            break;
        case OVERLAY_LOBBY:
            PatchExecCS(2, 3);
            break;
        case OVERLAY_PLSEL:
            PatchExecCS(6, 1);
            break;
        case OVERLAY_DNAS_INS:
        case OVERLAY_DNAS_NET:
            return;
        }
        if (gSysSettings.gmSubLoaded == 1) {
            PatchExecCS(3, 2);
        }
    }
}

/* 1 when overlay id (an OverlayId) is in its window, else 0; main always is. */
s32 Overlay_IsLoaded(s32 id)
{
    if (id < 0) {
        return 0U;
    }
    switch (id) {
    case OVERLAY_MAIN:
        return 1U;
    case OVERLAY_SUB_MAIN:
        return gSysSettings.subMainLoaded;
    default:
        return gSysSettings.loaded[gOverlayWindow[id - 1] - OVERLAY_WINDOW_SCENE] == id;
    }
}

/* 1 when every quest of HR gate n (0-2: HR 11, 17 and 31) is cleared. */
s32 Save_HrGateCleared(s32 n)
{
    u16 *questId = gHrGateQuests[n];

    while (*questId != 0) {
        if (Save_IsQuestCleared(*questId) == 0) {
            return 0;
        }
        questId++;
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

    for (i = 0; gDojoParTimes[i].questId != 0; i++) {
        if (id == gDojoParTimes[i].questId) {
            return gDojoParTimes[i].minutes;
        }
    }
    return 0;
}

/* The index of training quest id in gDojoParTimes, or -1. */
s32 Dojo_FindQuest(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].questId != 0; i++) {
        if (id == gDojoParTimes[i].questId) {
            return i;
        }
    }
    return -1;
}

/* The best score of training quest id, 0 if it isn't one. */
s32 Dojo_GetBestScore(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].questId != 0; i++) {
        if (id == gDojoParTimes[i].questId) {
            return gSave.dojoScore[i];
        }
    }
    return 0;
}

/* The best time of training quest id in frames, 0 if it isn't one. */
s32 Dojo_GetBestTime(u16 id)
{
    s16 i;

    for (i = 0; gDojoParTimes[i].questId != 0; i++) {
        if (id == gDojoParTimes[i].questId) {
            return gSave.dojoTime[i];
        }
    }
    return 0;
}

/* Clears the monster rates' snapshot and the season flags. */
void Rate_ClearSnapshot(void)
{
    s32 i;

    for (i = 0; i < RATE_SNAPSHOT_COUNT; i++) {
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

/* Copies the monster rates of the save data out: the first
 * RATE_SNAPSHOT_COUNT values (as s32) and every kind's trend and level
 * bits. */
void Rate_TakeSnapshot(void)
{
    SaveData *save = &gSave;
    s32 i;

    for (i = 0; i < RATE_SNAPSHOT_COUNT; i++) {
        gRateSnapshot[i] = save->rateBase[i];
    }
    memcpy(gLobbyRateStates, save->rateState, sizeof(save->rateState));
}

/* Clears gQuestRecord; returns it (memset's result). */
void *QuestRecord_Clear(void)
{
    return memset(&gQuestRecord, 0, sizeof(QuestRecord));
}

/*
 * Fills gQuestRecord for the results from sub_main's state: the quest, its
 * phase, map group, flyer, tier and time. The counts and the hunted and
 * captured tables (kept up during the quest) are cleared unless the first
 * player in the quest is the local one and the quest's goal is met.
 * Kept for the match:
 * - the pointer to the quest's map (info + 0x20, QuestInfo.map read as
 *   a u16) taken before the stores, and the empty check of the tier (0 to
 *   3; any two-test condition gives the same code), give the game's
 *   registers and load order;
 * - the time is stored through a cast of the field (`*(s32 *)&...elapsed`,
 *   which aliases like a raw address): stored as the field, the stores are
 *   scheduled differently.
 */
void QuestRecord_Fill(void)
{
    QuestRecord *rec = &gQuestRecord;
    s32 elapsed;
    u16 questId;
    u8 phase;
    u8 *info;
    s8 tier;
    s8 flyer;
    u8 *map;
    u8 playerCount;
    s32 i;
    s32 j;

    elapsed = gQuest.elapsed;
    questId = gQuest.stage;
    phase = gScene.questPhase;
    info = (u8 *)gQuest.info;
    map = info + 0x20;
    *(s32 *)&gQuestRecord.elapsed = elapsed;
    gQuestRecord.questId = questId;
    gQuestRecord.phase = phase;
    gQuestRecord.mapGroup = Map_Group(*(u16 *)map, elapsed);
    tier = gQuest.header->tier;
    flyer = gQuest.flyer;
    if (tier < 0 || tier > 3) { /* empty: for the match */
    }
    playerCount = gScene.playerCount;
    gQuestRecord.tier = tier;
    gQuestRecord.flyer = flyer;
    for (i = 0; i < playerCount; i++) {
        if (gScene.inQuest[i] == 1) {
            break;
        }
    }
    if (i != gScene.localPlayer || Quest_GetOutcome(1) != 1) {
        for (j = 0; j < QUEST_RECORD_IDS; j++) {
            rec->counts[j] = 0;
        }
        for (i = 0; i < QUEST_RECORD_KINDS; i++) {
            rec->hunted[i] = 0;
            rec->captured[i] = 0;
        }
    }
}

/* Counts a clear of quest id in gQuestRecord.counts, if gQuestRecordIds
 * lists it (the count stops at 255). */
void QuestRecord_Count(u16 id)
{
    s32 i;
    u16 *listed = gQuestRecordIds;

    for (i = 0; i < QUEST_RECORD_IDS; i++, listed++) {
        if (*listed == 0) {
            break;
        }
        if (*listed == id) {
            if (gQuestRecord.counts[i] < 0xFF) {
                gQuestRecord.counts[i]++;
            }
            break;
        }
    }
}

/* The season (SEASON_*), from sub_main's season bits (Season_Set sets one). */
s32 Season_Get(void)
{
    u8 flags = gSeasonFlags;
    s32 season;

    if (flags & (1 << SEASON_WARM)) {
        season = SEASON_WARM;
    } else if (flags & (1 << SEASON_COLD)) {
        season = SEASON_COLD;
    } else {
        season = SEASON_BREEDING;
    }
    return season;
}

/* Sets the season bits to season (SEASON_*): bit season of the low three. */
void Season_Set(s32 season)
{
    gSeasonFlags = (u8) (gSeasonFlags & ~7);
    gSeasonFlags = (u8) (gSeasonFlags | ((1 << (season & 0xFF)) & 0xFF));
}

/* The current quest's season (SEASON_*; the same bits in the quest's data). */
s32 Quest_GetSeason(void)
{
    u8 flags = gQuestSeasonFlags;
    s32 season;

    if (flags & (1 << SEASON_WARM)) {
        season = SEASON_WARM;
    } else if (flags & (1 << SEASON_COLD)) {
        season = SEASON_COLD;
    } else {
        season = SEASON_BREEDING;
    }
    return season;
}

/* This file's initialised data. */
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
DojoParTime gDojoParTimes[24] = {
#include "main/001BB200.inc"
};
u32 D_001BB260[8] = {
#include "main/001BB260.inc"
};
u16 gQuestRecordIds[16] = {
#include "main/001BB280.inc"
};
