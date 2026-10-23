/*
 * The boot executable's main and the services every overlay uses.
 * - Sys_EmptyHook: an empty function main and two overlays call.
 * - main: sets up the display, the sound and CRI's file systems, loads
 *   sub_main (include/overlay.h) and runs it; sub_main_entry never returns.
 * - Afs_ReadId: reads one file of the disc's AFS archives, by an id that
 *   holds the archive and the file, while the loading screen goes on.
 * - LoadOverlay and its helpers: read an overlay into its window
 *   (gOverlayWindow, gOverlayWindowBase), relocate it, and record what each
 *   window holds in gSysSettings.
 */
#include "common.h"
#include "syssettings.h"
#include "quest.h"
#include "overlay.h"
#include "card.h"
#include "cri_setup.h"
#include "sound.h"
#include "stream.h"
#include "main.h"
#include "display.h"
#include "sys_main.h"
#include "libc.h"

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

extern u8 gSeasonFlags;         /* sub_main 0x5D4910 */
extern s32 gLoadLastVSync;      /* sub_main 0x5D4A50: gVSyncCount when the loader last ran a frame */
extern u8 gQuestSeasonFlags;    /* sub_main 0x5D87A9: the quest's season (Quest_GetSeason) */
extern u8 gOverlayWindow[];
#define ARGV_KEPT 3 /* the command-line arguments main keeps */
extern char gArgv[ARGV_KEPT][64];
extern u8 gAdxCacheWork[];
extern s32 gVSyncCount;
extern s32 gLoadingScreenOn;
extern QuestRecord gQuestRecord;
u16 Map_Group(u16, s32); /* unit's own declaration: the definition (sub_main) is u16 Map_Group(u16 map), which ignores a1; QuestRecord_Fill passes a1 = v, and its call compiles as the original's only with that second argument */
s32 Quest_GetOutcome(s32 withLoss); /* the definition (sub_main) is s32 Quest_GetOutcome(s32 withLoss): the call here passes a second argument */

void func_001657B8(s32);
void func_00165400(AdxDvdFsParam *);
void func_00165500(AdxCacheParam *);
void func_00162220(s32);
void func_00128550(void *addr, s32 size);

/* An empty function. main calls it once, after loading the first
 * overlay and before entering sub_main; the lobby's NetMode_RunSetup and
 * select's PSP link call it too. */
void Sys_EmptyHook(void)
{
}

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
    while (InitDisplay(512, 448) == 0) {
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
 * file list (32 files, 533 sectors) and Sofdec. */
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
    cache.nfile = 32;
    cache.nsct = 533;
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

/* This file's initialised data. */
u8 gOverlayWindow[16] = {
#include "main/001BB130.inc"
};
