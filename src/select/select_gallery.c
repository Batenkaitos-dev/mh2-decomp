/*
 * select: the gallery (0x6BF3B0-0x6C2DB0, with its .data and .rodata),
 * started from gm_sub's main menu.
 *
 * Gallery_Task runs Gallery_Update over GalWork (select.h), at 0x1000 in
 * the shared work area. The menu offers three categories (monsters, the
 * world's areas, bonus movies); six pictures move between layouts as the
 * menu opens a category's list. An unlocked entry (gGalleryEntries, its
 * event flag) opens its page: a monster's quest or an area's tour (plsel's
 * Galex tasks, after loading overlay 0xB), a movie, or the ending. When
 * one of those ends, gSysSettings' gallery fields bring the player back to
 * the same entry.
 */
#include "common.h"
#include "window.h"
extern s32 gVSyncCount; /* main: +1 every VBlank */
extern s32 gLoadLastVSync; /* sub_main: gVSyncCount when the loader last ran a frame */
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "pad.h"
#include "m2c_macros.h"
#include "select.h"
#include "syssettings.h"
#include "save.h"
#include "select/select_gallery.h"   /* the texts this file uses (tools/content_gen.py) */

/* The game compiled this file at optimization level 4 (the build's -O3,p
 * plus MWCC's level-4 passes): Gallery_EntryInList matches only there, and every
 * other function compiles the same at both levels. */
#pragma optimization_level 4


/* gGalleryEntries's records: the gallery's entries. */
typedef struct GalEntry {
    char *title;            /* 0x00 */
    void *unk4;             /* 0x04 */
    u8 unk8;
    u8 kind;                /* 0x09 */
    u8 movie;               /* 0x0A: kind 3: its index in gGalleryMovieIds */
    u8 needsUnlock;         /* 0x0B: listed only once its event flag (the entry number) is set */
} GalEntry;

/* gGalleryArrowSprites's records: the list's scroll arrows. */
typedef struct GalIcon {
    s32 tex;                /* 0x00: index into gTexHandles */
    s32 unk4;
    s16 w, h;               /* 0x08 */
    s16 u0, v0, u1, v1;     /* 0x0C */
    u32 color;              /* 0x14 */
    s32 unk18;
    f32 unk1C;              /* 0x1C */
} GalIcon;

/* gGalleryRecs's records (Gallery_DrawRecords draws them). */
typedef struct GalRec {
    s32 tex;                /* 0x00: > 100: gTexHandles, else the gallery's own */
    s16 x, y, w, h;         /* 0x04 */
    s16 u0, v0, u1, v1;     /* 0x0C */
    u32 color;              /* 0x14 */
    u16 angle;              /* 0x18 */
    u8 unk1A[2];
} GalRec;

/* A filled screen rectangle (Prim_FillRect). */
typedef struct GalRect {
    s16 x, y, w, h;
    u32 color;
} GalRect;

/* The gallery's work in the shared work area. */
#define GAL (&((SelWork *)gSysSettings.workArea)->gallery)

s32 Gallery_PictureMoving(s32 i);
extern s32 D_6ADABC[];      /* sub_main: flags of what the gallery shows */
extern s32 gGalleryTex[];

void Gallery_Init(TaskSlot *t);
static void Gallery_ApplyLayout(GalWork *g, s32 k);
s32 Gallery_Update(TaskSlot *t);
void Gallery_Exit(TaskSlot *t);
void Sel_FreeTextures(void);
void Afs_LoadPartition(s32 ptid, s32 flid, void *ptinfo, char *name);
s32 Afs_ReadFileIdle(s32 flid, s32 ptid, void *buf);
s32 func_36BDF0(s32, s32);
s32 Sys_FlipFrame(s32 unused0);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
void LoadOverlay(s32 id);
void Task_StartGmSub(void);
s32 Gs_GetState(u8 reg);
void Sel_DrawHeader(s16 kind);
void Gallery_DrawCategories(GalWork *);
void Gallery_DrawList(GalWork *);
void Gallery_DrawEntryPage(GalWork *);
void Gallery_DrawPicture(s32 *, f32 *);
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha); /* sub_main: w is a struct ListWin * */
void Text_SetColor(s16 color);
void Text_Printf(const char *fmt, ...);
s32 Font_GetPage(void);
char *strcpy(char *, char *);
s32 Tex_Use(s32 n, u32 *handles);
void Gallery_PlayPageSe(void);
void Loading_Start(void);
void Loading_Stop(void);
void Sel_LoadCommonTextures(void);
extern s32 gTexHandles[];
struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);


s32 EventFlag_Test(s32 n);
void Mem_Compact(void);
void Tex_FreeRange(s32 first, s32 count);

/* .data */
u32 gGalleryCategoryNames[3] = {
#include "select/006CB758.inc"
};
u32 gGalleryPlayHelp[2] = {
#include "select/006CB768.inc"
};
u32 gGalleryNewText[2] = {
#include "select/006CB770.inc"
};
u32 gGalleryTitleTexts[2] = {
#include "select/006CB778.inc"
};
u32 gGalleryEntries_1[4] = {
#include "select/006CB780.inc"
};
u32 gGalleryEntries_4[4] = {
#include "select/006CB790.inc"
};
u32 gGalleryEntries_7[4] = {
#include "select/006CB7A0.inc"
};
u32 gGalleryEntries_10[4] = {
#include "select/006CB7B0.inc"
};
u32 gGalleryEntries_13[4] = {
#include "select/006CB7C0.inc"
};
u32 gGalleryEntries_16[4] = {
#include "select/006CB7D0.inc"
};
u32 gGalleryEntries_19[4] = {
#include "select/006CB7E0.inc"
};
u32 gGalleryEntries_22[4] = {
#include "select/006CB7F0.inc"
};
u32 gGalleryEntries_25[4] = {
#include "select/006CB800.inc"
};
u32 gGalleryEntries_28[4] = {
#include "select/006CB810.inc"
};
u32 gGalleryEntries_31[4] = {
#include "select/006CB820.inc"
};
u32 gGalleryEntries_34[4] = {
#include "select/006CB830.inc"
};
u32 gGalleryEntries_37[4] = {
#include "select/006CB840.inc"
};
u32 gGalleryEntries_40[4] = {
#include "select/006CB850.inc"
};
u32 gGalleryEntries_43[4] = {
#include "select/006CB860.inc"
};
u32 gGalleryEntries_46[4] = {
#include "select/006CB870.inc"
};
u32 gGalleryEntries_49[4] = {
#include "select/006CB880.inc"
};
u32 gGalleryEntries_52[4] = {
#include "select/006CB890.inc"
};
u32 gGalleryEntries_55[4] = {
#include "select/006CB8A0.inc"
};
u32 gGalleryEntries_58[4] = {
#include "select/006CB8B0.inc"
};
u32 gGalleryEntries_61[4] = {
#include "select/006CB8C0.inc"
};
u32 gGalleryEntries_64[4] = {
#include "select/006CB8D0.inc"
};
u32 gGalleryEntries_67[4] = {
#include "select/006CB8E0.inc"
};
u32 gGalleryEntries_70[4] = {
#include "select/006CB8F0.inc"
};
u32 gGalleryEntries_73[4] = {
#include "select/006CB900.inc"
};
u32 gGalleryEntries_76[4] = {
#include "select/006CB910.inc"
};
u32 gGalleryEntries_79[4] = {
#include "select/006CB920.inc"
};
u32 gGalleryEntries_82[4] = {
#include "select/006CB930.inc"
};
u32 gGalleryEntries_85[4] = {
#include "select/006CB940.inc"
};
u32 gGalleryEntries_88[4] = {
#include "select/006CB950.inc"
};
u32 gGalleryEntries_91[4] = {
#include "select/006CB960.inc"
};
u32 gGalleryEntries_94[4] = {
#include "select/006CB970.inc"
};
u32 gGalleryEntries_97[4] = {
#include "select/006CB980.inc"
};
u32 gGalleryEntries_100[4] = {
#include "select/006CB990.inc"
};
u32 gGalleryEntries_103[4] = {
#include "select/006CB9A0.inc"
};
u32 gGalleryEntries_106[4] = {
#include "select/006CB9B0.inc"
};
u32 gGalleryEntries_109[4] = {
#include "select/006CB9C0.inc"
};
u32 gGalleryEntries_112[4] = {
#include "select/006CB9D0.inc"
};
u32 gGalleryEntries_115[4] = {
#include "select/006CB9E0.inc"
};
u32 gGalleryEntries_118[4] = {
#include "select/006CB9F0.inc"
};
u32 gGalleryEntries_121[4] = {
#include "select/006CBA00.inc"
};
u32 gGalleryEntries_124[4] = {
#include "select/006CBA10.inc"
};
u32 gGalleryEntries_127[4] = {
#include "select/006CBA20.inc"
};
u32 gGalleryEntries_130[4] = {
#include "select/006CBA30.inc"
};
u32 gGalleryEntries_133[4] = {
#include "select/006CBA40.inc"
};
u32 gGalleryEntries_136[4] = {
#include "select/006CBA50.inc"
};
u32 gGalleryEntries_139[4] = {
#include "select/006CBA60.inc"
};
u32 gGalleryEntries_142[4] = {
#include "select/006CBA70.inc"
};
u32 gGalleryEntries_145[4] = {
#include "select/006CBA80.inc"
};
u32 gGalleryEntries_148[4] = {
#include "select/006CBA90.inc"
};
u32 gGalleryEntries_151[4] = {
#include "select/006CBAA0.inc"
};
u32 gGalleryEntries_154[4] = {
#include "select/006CBAB0.inc"
};
u32 gGalleryEntries_160[4] = {
#include "select/006CBAC0.inc"
};
u32 gGalleryEntries_157[4] = {
#include "select/006CBAD0.inc"
};
u32 gGalleryEntries[164] = {
#include "select/006CBAE0.inc"
};
u32 gGalleryMovieIds[8] = {
#include "select/006CBD70.inc"
};
u32 gGalleryLayout[72] = {
#include "select/006CBD90.inc"
};
/* The list: frame, cursor, scroll bar (Gallery_DrawScrollBar, Gallery_DrawCursor). */
GalRec gGalleryRecs[4] = {
#include "select/006CBEB0.inc"
};
u32 gGalleryScrollBar[8] = {
#include "select/006CBF20.inc"
};
/* The scroll arrow (Gallery_DrawScrollBar) and a second record
 * (Gallery_DrawEntryPage indexes the table). As a GalRec array MWCC knows [0] is
 * 16-aligned and copies it with lq/sq, as the game does. */
GalRec gGalleryScrollRecs[2] = {
#include "select/006CBF40.inc"
};
/* 16-aligned: the 8 zero bytes before it (0x6CBF78) are padding. */
u32 gGalleryEntryPagePos[16] __attribute__((aligned(16))) = {
#include "select/006CBF80.inc"
};
u32 gGalleryInitBytes[4] = {
#include "select/006CBFC0.inc"
};
/* The six pictures' target positions, per layout (gGalleryLayoutPos). */
s16 gGalleryLayoutPos0[8][2] = {
#include "select/006CBFD0.inc"
};
s16 gGalleryLayoutPos1[8][2] = {
#include "select/006CBFF0.inc"
};
s16 gGalleryLayoutPos2[8][2] = {
#include "select/006CC010.inc"
};
s16 gGalleryLayoutPos3[8][2] = {
#include "select/006CC030.inc"
};
s16 (*gGalleryLayoutPos[4])[2] = {
    gGalleryLayoutPos0, gGalleryLayoutPos1, gGalleryLayoutPos2, gGalleryLayoutPos3,
};
s32 gGalleryLayoutSteps[8] = {
#include "select/006CC060.inc"
};
s32 gGalleryLayoutBackSteps[8] = {
#include "select/006CC080.inc"
};
f32 gGalleryLayoutSpeeds[6] = {
#include "select/006CC0A0.inc"
};
u32 gGalleryCategoryOfs[3] = {
#include "select/006CC0B8.inc"
};
u8 gGalleryCategoryFirst[8] = {
#include "select/006CC0C8.inc"
};
u8 gGalleryCategoryEnd[16] = {
#include "select/006CC0D0.inc"
};
u32 gGalleryEntryList[12] = {
#include "select/006CC0E0.inc"
};
ListWin gGalleryListPanel = {
#include "select/006CC110.inc"
};
GalRect gGalleryFadeRect = {
#include "select/006CC130.inc"
};
GalIcon gGalleryArrowSprites[4] = {
#include "select/006CC140.inc"
};
s16 gGalleryArrowPosA[4][2] = {
#include "select/006CC1C0.inc"
};
s16 gGalleryArrowPosB[4][2] = {
#include "select/006CC1D0.inc"
};


/*
 * Marks entry n as seen (a bit of sub_main's D_6ADABC), which ends its new mark.
 */
void Gallery_SetSeen(s32 n)
{
    D_6ADABC[n / 32] |= 1 << (n % 32);
}

/* Whether entry n is unlocked: its event flag is set. */
s32 Gallery_IsUnlocked(s32 n)
{
    return EventFlag_Test(n);
}

/* Whether entry n is new: unlocked and not seen yet. */
s32 Gallery_IsNew(s32 n)
{
    s32 w;

    if (EventFlag_Test(n) == 1) {
        w = D_6ADABC[n / 32];
        if (!(w & (1 << (n % 32)))) {
            return 1;
        }
    }
    return 0;
}

void Gallery_PlayCursorSe(void)
{
    Sound_PlaySeFlat(1, 0xC, 0);
}

void Gallery_PlayOpenSe(void)
{
    Sound_PlaySeFlat(1, 0xB, 0);
}

void Gallery_PlayCloseSe(void)
{
    Sound_PlaySeFlat(1, 0xB, 0);
}

void Gallery_PlayErrorSe(void)
{
    Sound_PlaySeFlat(0, 0x15, 0);
}

void FlushCache(s32);
struct SndFile;
s32 Sound_LoadBankWait(struct SndFile *f, u32 slot); /* main: f is a SndFile * */
s32 Sound_ReadBank(u32 id, void *buf);
/*
 * Loads the menu's four sound banks back after a quest or tour of the
 * gallery (slots 0, 1, 6 and 7).
 */
void Gallery_LoadSounds(void)
{
    u8 *loadBuf;

    loadBuf = gSysSettings.loadBuf;
    FlushCache(0);
    Sound_ReadBank(0x10037, loadBuf);
    Sound_LoadBankWait((struct SndFile *)loadBuf, 0);
    FlushCache(0);
    Sound_ReadBank(0x10002, loadBuf);
    Sound_LoadBankWait((struct SndFile *)loadBuf, 1);
    FlushCache(0);
    Sound_ReadBank(0x10007, loadBuf);
    Sound_LoadBankWait((struct SndFile *)loadBuf, 6);
    FlushCache(0);
    Sound_ReadBank(0x10008, loadBuf);
    Sound_LoadBankWait((struct SndFile *)loadBuf, 7);
}

/* The gallery's task: starts it, runs it until it ends, then leaves. */
void Gallery_Task(TaskSlot *task)
{
    switch (task->mode) {
    case 0:
        Gallery_Init(task);
        task->mode++;
        break;
    case 1:
        if (Gallery_Update(task) == 0) {
            break;
        }
        task->mode++;
    case 2:
        Gallery_Exit(task);
        break;
    }
}

/*
 * Loads the gallery's 55 textures from the galex partition, flipping the
 * loading screen's frame as time passes.
 */
void Gallery_LoadTextures(void)
{
    s32 i;
    s32 *handle;
    s32 cur;
    s32 prev;

    Afs_LoadPartition(5, 0x19F4, gSysSettings.readBuf, "galex");
    for (i = 0, handle = gGalleryTex; i < 0x37; i++, handle++) {
        cur = gVSyncCount;
        prev = gLoadLastVSync;
        if (prev != cur) {
            func_36BDF0(cur - prev, 0);
            gLoadLastVSync = gVSyncCount;
            Sys_FlipFrame(0);
        }
        Afs_ReadFileIdle(i, 5, gSysSettings.loadBuf);
        *handle = Tex_CreateFromTim2(gSysSettings.loadBuf, 4);
    }
    Mem_Compact();
}

void Gallery_FreeTextures(void)
{
    Tex_FreeRange(0x2B, 0x37);
    Mem_Compact();
}

void EventFlag_Set(s32);
void Gallery_UpdateNewFlags(void);
s32 Gallery_BuildList(s32 cat);
void Gallery_LoadSounds(void);
void Gallery_LoadTextures(void);
extern char gGalleryTitle[];

/* The gallery's start: clears its work and the save buffer, takes the
 * texture handles, places the six pictures (layout 0) and the category
 * names, and builds the three lists. */
void Gallery_Init(TaskSlot *task)
{
    GalRec *rec;
    u8 *initBytes;
    s16 *layoutPos;
    GalWork *gal;
    s32 i;
    s32 j;
    GalWork *nameWalk;

    gGalleryTitle[0] = 0;
    Loading_Start();
    gal = GAL;
    memset(gal, 0, 0x224C);
    memset(&gSave, 0, 0x37F8);
    Sel_LoadCommonTextures();
    for (j = 0xA3, i = 0; i < 16; i++, j++) {
        gal->texHandles[i] = gTexHandles[j];
    }
    Gallery_LoadTextures();
    for (i = 0, initBytes = (u8 *)gGalleryInitBytes; i < 6; i++) {
        gal->pictures[i][0] = *initBytes++;
        gal->pictures[i][1] = *initBytes++;
    }
    for (i = 0, layoutPos = (s16 *)gGalleryLayoutPos[0]; i < 6; i++) {
        gal->pos[i][0] = *layoutPos++;
        gal->pos[i][1] = *layoutPos++;
        gal->moveState[i] = 1;
        gal->drawOrder[i] = i;
        gal->pictureCount++;
    }
    for (rec = (GalRec *)gGalleryLayout, i = 0, nameWalk = gal; i < 10; i++, rec++) {
        if (i == 2 || i == 4 || i == 6) {
            nameWalk->categoryPos[0][0] = rec->x;
            nameWalk->categoryPos[0][1] = rec->y;
            nameWalk = (GalWork *)((u8 *)nameWalk + 8);
        }
    }
    EventFlag_Set(0x30);
    gal->categoryCount = 3;
    gal->entryCounts[0] = Gallery_BuildList(0);
    gal->entryCounts[1] = Gallery_BuildList(1);
    gal->entryCounts[2] = Gallery_BuildList(2);
    Loading_Stop();
    Gallery_UpdateNewFlags();
    if (gSysSettings.galleryPlay == 1) {
        Gallery_LoadSounds();
    }
}

/*
 * Moves the category names that a layout (k: 0 the menu, 1-3 a category's
 * list) places at another height.
 */
static void Gallery_ApplyLayout(GalWork *gal, s32 k)
{
    s16 *tbl = (s16 *)gGalleryLayout;

    switch (k) {
    case 0:
        gal->categoryPos[0][1] = tbl[0x3E / 2];
        gal->categoryPos[1][1] = tbl[0x76 / 2];
        gal->categoryPos[2][1] = tbl[0xAE / 2];
        break;
    case 1:
        gal->categoryPos[2][1] = tbl[0xAE / 2];
        break;
    case 2:
        gal->categoryPos[1][1] = tbl[0xAE / 2];
        break;
    case 3:
        gal->categoryPos[0][1] = tbl[0xAE / 2];
        break;
    }
}

/*
 * Whether the chosen entry is a quest or an area (kinds 0-2) listed in
 * gGalleryEntryList: those ask day or night before they start. The scan
 * matches only at optimization level 4 (the file's pragma).
 */
s32 Gallery_EntryInList(GalWork *gal)
{
    s32 idx;
    u8 *listed;
    u8 entryId;

    idx = gal->selected;
    switch (((GalEntry *)gGalleryEntries)[gal->entries[idx]].kind) {
    case 0:
    case 1:
    case 2:
        entryId = *(u8 *)gGalleryEntryList;
        listed = (u8 *)gGalleryEntryList;
        if (entryId != 0xFF) {
            do {
                if (entryId == gal->entries[idx]) {
                    return 1;
                }
                entryId = *++listed;
            } while (entryId != 0xFF);
        }
        break;
    }
    return 0;
}

void Gallery_UpdatePictures(void);
void Gallery_Draw(void);
void Frame_Draw(void);
void Gallery_DrawFade(void);
void Gallery_ResetPanel(GalWork *g);
u8 FadeA_GetState(void);
void Gallery_FillNames(GalWork *g);
void Gallery_SetLayout(s32 k);
void Gallery_StartPanel(GalWork *g, s32 back);
void NetMenu_Open(u8 mode);
void Gallery_PlayCancelSe(void);
s32 Gallery_Settled(void);
s32 Gallery_ListInput(GalWork *g, u16 pad);
void Gallery_SetLayoutBack(s32 unused);
void Gallery_PlayDecideSe(void);
void Game_ResetSubsystems(void);
void FadeA_Reset(void);
void Loading_SetMessage(u32 mode);
void Scene_LoadOverlay(s32);
void TaskSlot_Set(void *func, s16 slot);
void Galex_QuestTask(TaskSlot *t); /* plsel (overlay 0xB) */
extern char gGalleryTitle[];     /* the title of what is shown */
void Galex_Task(TaskSlot *t); /* plsel */
void Ending_Init(void);
s32 func_D64D40(void);
void Gallery_GetTextures(GalWork *g);
void Movie_PrePlay(void);
s32 Movie_Play(s32 id);
s32 Movie_Update(void);
s32 Movie_GetStatus(void);
void Movie_RequestStop(void);
void Movie_Stop(void);
/* unit's own declaration: with the definition's u8 (void) (sub_main/unk_3D54B0.c), Gallery_Update: 77 of 791 words differ */
s32 NetMenu_Update(void);

/* The gallery's frame (GalState): the category menu (1), the lists of the
 * three categories (2-4), a picture shown by the game (5), a movie (6),
 * the network menu (7) and the fade out (8, 9: done). */
s32 Gallery_Update(TaskSlot *task)
{
    GalWork *gal;
    GalEntry *entry;
    u16 trig;
    u16 rep;
    s32 i;

    trig = gPadState[0].pressed;
    rep = trig | gPadState[0].repeat;
    gal = GAL;
    switch (gal->state) {
    case GAL_OPEN:
        gal->state++;
        gal->step = 0;
        FadeA_Start(2);
        break;
    case GAL_CATEGORIES:
        switch (gal->step) {
        case 0:
            if (gSysSettings.galleryPlay == 1) {
                gal->fadeFrames = 0x14;
                FadeA_Start(2);
                Gallery_ResetPanel(gal);
                gal->category = gSysSettings.gallerySel;
                gal->step++;
            } else {
                Gallery_ResetPanel(gal);
                if ((u8)FadeA_GetState() == 1) {
                    break;
                }
                gal->step++;
            }
        case 1:
            if (gSysSettings.galleryPlay == 1) {
                gal->fadeFrames = 0x14;
                FadeA_Start(2);
                gal->category = gSysSettings.gallerySel;
                goto open;
            }
            if (rep & 0x2000) {
                gal->category--;
                if (gal->category < 0) {
                    gal->category += gal->categoryCount;
                }
                Gallery_PlayCursorSe();
            } else if (rep & 0x1000) {
                gal->category++;
                if (gal->category >= gal->categoryCount) {
                    gal->category -= gal->categoryCount;
                }
                Gallery_PlayCursorSe();
            } else if (trig & 0x20) {
                Gallery_PlayOpenSe();
            open:
                Gallery_FillNames(gal);
                Gallery_SetLayout(gal->category + 1);
                gal->step++;
                Gallery_StartPanel(gal, 0);
            } else if (trig & 0x40) {
                NetMenu_Open(1);
                gal->state = GAL_NET_MENU;
                gal->step = 0;
                Gallery_PlayCancelSe();
            } else {
                gal->iconTimer++;
                if (!(gal->iconTimer & 3)) {
                    gal->iconFrame++;
                    gal->iconFrame %= 3;
                }
                gal->animTimer++;
                if (!(gal->animTimer & 3)) {
                    gal->animFrame++;
                    gal->animFrame %= 3;
                }
            }
            break;
        case 2:
            if (gSysSettings.galleryPlay == 1) {
                FadeA_Start(2);
            }
            if (Gallery_Settled() != 0) {
                if (gSysSettings.galleryPlay == 1) {
                    gSysSettings.galleryPlay = 0;
                    gal->fadeFrames = 0x14;
                    gal->state = gSysSettings.gallerySel + GAL_LIST_MONSTERS;
                    gal->step = 0;
                    gal->selected = 0;
                    for (i = 0; i < gal->entryCount; i++) {
                        if (gal->entries[i] == gSysSettings.galleryEntry) {
                            gal->selected = i;
                            break;
                        }
                    }
                    gal->listTop = gSysSettings.galleryKeep[0];
                    gal->listRow = gSysSettings.galleryKeep[1];
                    FadeA_Start(2);
                } else {
                    gal->state = gal->category + GAL_LIST_MONSTERS;
                    gal->step = 0;
                }
                Gallery_ResetPanel(gal);
            }
            break;
        }
        break;
    case GAL_LIST_MONSTERS:
    case GAL_LIST_WORLD:
    case GAL_LIST_BONUS:
        switch (gal->step) {
        case 0:
            gal->pageShown = 1;
            if (Gallery_ListInput(gal, rep) == 1) {
                break;
            }
            if ((u8)FadeA_GetState() == 1) {
                break;
            }
            if (trig & 0x40) {
                Gallery_PlayCloseSe();
                gal->pageShown = 0;
                Gallery_SetLayoutBack(0);
                Gallery_StartPanel(gal, 1);
                gal->step++;
            } else if (trig & 0x20) {
                if (Gallery_IsUnlocked(gal->entries[gal->selected]) == 1) {
                    Gallery_SetSeen(gal->entries[gal->selected]);
                    Gallery_PlayDecideSe();
                    gal->night = 0;
                    if (Gallery_EntryInList(gal) == 1) {
                        gal->askTime = 1;
                        gal->step = 3;
                    } else {
                        gal->step = 2;
                        FadeA_Start(1);
                    }
                } else {
                    Gallery_PlayErrorSe();
                }
            } else {
                gal->animTimer++;
                if (!(gal->animTimer & 3)) {
                    gal->animFrame++;
                    gal->animFrame %= 3;
                }
            }
            break;
        case 1:
            if (Gallery_Settled() != 0) {
                gal->listShown = 0;
                gal->state = GAL_CATEGORIES;
                gal->step = 0;
            }
            break;
        case 2:
            if ((u8)FadeA_GetState() == 1) {
                break;
            }
            entry = &((GalEntry *)gGalleryEntries)[gal->entries[gal->selected]];
            switch (entry->kind) {
            case 4:
                gSysSettings.galleryPlay = 1;
                gSysSettings.gallerySel = gal->state - GAL_LIST_MONSTERS;
                gSysSettings.galleryEntry = gal->entries[gal->selected];
                gSysSettings.galleryKeep[0] = gal->listTop;
                gSysSettings.galleryKeep[1] = gal->listRow;
                gal->state = GAL_ENDING;
                gal->step = 0;
                strcpy(gGalleryTitle, gal->names[gal->listTop + gal->listRow]);
                break;
            case 3:
                gSysSettings.gallerySel = gal->state - GAL_LIST_MONSTERS;
                gSysSettings.galleryEntry = gal->entries[gal->selected];
                gSysSettings.galleryKeep[0] = gal->listTop;
                gSysSettings.galleryKeep[1] = gal->listRow;
                if (((u16 *)gGalleryMovieIds)[entry->movie] != 0xFFFF) {
                    gal->state = GAL_MOVIE;
                    gal->step = 0;
                }
                break;
            case 0:
            case 1:
                Game_ResetSubsystems();
                FadeA_Reset();
                gSysSettings.gallerySel = gal->state - GAL_LIST_MONSTERS;
                gSysSettings.galleryEntry = gal->entries[gal->selected];
                if (gal->night != 0) {
                    gSysSettings.galleryEntry = (gSysSettings.galleryEntry & 0x7F) | 0x80;
                }
                gSysSettings.galleryKeep[0] = gal->listTop;
                gSysSettings.galleryKeep[1] = gal->listRow;
                gSysSettings.galleryPlay = 1;
                strcpy(gGalleryTitle, gal->names[gal->listTop + gal->listRow]);
                Loading_SetMessage(5);
                Scene_LoadOverlay(0xB);
                TaskSlot_Set(Galex_QuestTask, 6);
                TaskSlot_End(task);
                return 0;
            case 2:
                Game_ResetSubsystems();
                FadeA_Reset();
                gSysSettings.gallerySel = gal->state - GAL_LIST_MONSTERS;
                gSysSettings.galleryEntry = gal->entries[gal->selected];
                if (gal->night != 0) {
                    gSysSettings.galleryEntry = (gSysSettings.galleryEntry & 0x7F) | 0x80;
                }
                gSysSettings.galleryKeep[0] = gal->listTop;
                gSysSettings.galleryKeep[1] = gal->listRow;
                gSysSettings.galleryPlay = 1;
                strcpy(gGalleryTitle, gal->names[gal->listTop + gal->listRow]);
                Loading_SetMessage(5);
                Scene_LoadOverlay(0xB);
                TaskSlot_Set(Galex_Task, 6);
                TaskSlot_End(task);
                return 0;
            }
            break;
        case 3:
            if ((trig & 0x400) || (trig & 0x800)) {
                Gallery_PlayPageSe();
                gal->night ^= 1;
            } else if (trig & 0x40) {
                gal->askTime = 0;
                Gallery_PlayCancelSe();
                gal->step = 0;
            } else if (trig & 0x20) {
                gal->askTime = 0;
                Gallery_PlayDecideSe();
                gal->step = 2;
                FadeA_Start(1);
            }
            break;
        }
        break;
    case GAL_ENDING:
        switch (gal->step) {
        case 0:
            FadeA_Reset();
            Scene_LoadOverlay(0xB);
            Sel_FreeTextures();
            Gallery_FreeTextures();
            Ending_Init();
            gal->step++;
            return 0;
        case 1:
            if (func_D64D40() == 0) {
                gal->step++;
                FadeA_Start(2);
            }
            return 0;
        case 2:
            if ((u8)FadeA_GetState() != 1) {
                Gallery_GetTextures(gal);
                Gallery_FillNames(gal);
                Gallery_UpdateNewFlags();
                gal->state = gSysSettings.gallerySel + GAL_LIST_MONSTERS;
                gal->step = 0;
                gal->selected = gSysSettings.galleryEntry;
                gal->listTop = gSysSettings.galleryKeep[0];
                gal->listRow = gSysSettings.galleryKeep[1];
                FadeA_Start(2);
                gal->fadeFrames = 0x14;
                gSysSettings.plselLoaded = 0;
                gSysSettings.galleryPlay = 0;
            }
            return 0;
        }
        break;
    case GAL_MOVIE:
        switch (gal->step) {
        case 0:
            FadeA_Reset();
            Scene_LoadOverlay(0xB);
            Sel_FreeTextures();
            Gallery_FreeTextures();
            Movie_PrePlay();
            Movie_Play(((u16 *)gGalleryMovieIds)[((GalEntry *)gGalleryEntries)[gal->entries[gal->selected]].movie]);
            gal->step++;
        case 1:
            Movie_Update();
            if (Movie_GetStatus() == 3 || (gPadState[0].pressed & 0x40)) {
                Movie_RequestStop();
                gal->step++;
                FadeA_Start(2);
            }
            return 0;
        case 2:
            if ((u8)FadeA_GetState() != 1) {
                Movie_Stop();
                Gallery_GetTextures(gal);
                Gallery_FillNames(gal);
                Gallery_UpdateNewFlags();
                gal->state = gSysSettings.gallerySel + GAL_LIST_MONSTERS;
                gal->step = 0;
                gal->selected = gSysSettings.galleryEntry;
                gal->listTop = gSysSettings.galleryKeep[0];
                gal->listRow = gSysSettings.galleryKeep[1];
                FadeA_Start(2);
                gSysSettings.plselLoaded = 0;
            }
            return 0;
        }
        break;
    case GAL_NET_MENU:
        if ((u8)NetMenu_Update()) {
            gal->state = GAL_FADE_OUT;
            FadeA_Start(1);
        }
        break;
    case GAL_FADE_OUT:
        if ((u8)FadeA_GetState() != 1) {
            gal->state = GAL_DONE;
            gal->step = 0;
        }
        break;
    case GAL_DONE:
        gSysSettings.galleryPlay = 0;
        return 1;
    }
    Gallery_UpdatePictures();
    Gallery_Draw();
    Frame_Draw();
    Gallery_DrawFade();
    return 0;
}

/* Sets each category's new mark: whether one of its entries is new. */
void Gallery_UpdateNewFlags(void)
{
    s32 i;
    s32 j;
    s32 end;
    GalWork *gal;

    gal = GAL;
    for (i = 0; i < 3; i++) {
        j = gGalleryCategoryFirst[i];
        end = gGalleryCategoryEnd[i];
        gal->categoryNew[i] = 0;
        for (; j < end; j++) {
            if (Gallery_IsNew(j) == 1) {
                gal->categoryNew[i] = 1;
                break;
            }
        }
    }
}

/*
 * Appends category cat's entries to the list (GalWork.entries): those that
 * need no unlock or are unlocked, then 0xFF for each locked one. Returns
 * the count listed.
 */
s32 Gallery_BuildList(s32 cat)
{
    s32 i;
    s32 start;
    s32 end;
    s32 n;
    GalEntry *entry;
    GalWork *gal;

    gal = GAL;
    start = gGalleryCategoryFirst[cat];
    end = gGalleryCategoryEnd[cat];
    entry = (GalEntry *)gGalleryEntries + start;
    n = 0;
    for (i = start; i < end; i++, entry++) {
        if (entry->needsUnlock == 0) {
            n++;
            gal->entries[gal->entryCount++] = i;
        } else if (EventFlag_Test(i) == 1) {
            n++;
            gal->entries[gal->entryCount++] = i;
        }
    }
    for (i = start + n; i < end; i++) {
        gal->entries[gal->entryCount++] = 0xFF;
    }
    return n;
}

/* Entry n of category i, as an index of gGalleryEntries. */
static s32 Gallery_EntryIndex(s32 i, s32 n)
{
    return gGalleryCategoryFirst[i] + n;
}

/*
 * Leaves the gallery: frees its textures, ends the task, loads the select
 * overlay back and returns to gm_sub's menu.
 */
void Gallery_Exit(TaskSlot *task)
{
    Sel_FreeTextures();
    Gallery_FreeTextures();
    TaskSlot_End(task);
    LoadOverlay(2);
    Task_StartGmSub();
    FadeA_Start(2);
}

/*
 * Takes the common textures' handles and loads the gallery's own again
 * (after a movie or the ending).
 */
void Gallery_GetTextures(GalWork *gal)
{
    s32 j;
    s32 i;

    Loading_Start();
    Sel_LoadCommonTextures();
    for (j = 0xA3, i = 0; i < 16; i++, j++) {
        gal->texHandles[i] = gTexHandles[j];
    }
    Gallery_LoadTextures();
    Loading_Stop();
}

/*
 * Draws the gallery: the pictures in their order, the header, the category
 * names, the list and the entry page when shown, and the day-or-night
 * question.
 */
void Gallery_Draw(void)
{
    GalWork *gal;
    s32 i;
    s32 m6C;
    s32 m63;
    s32 m5E;
    s32 k;

    gal = GAL;
    m6C = Gs_GetState(0x6C);
    m63 = Gs_GetState(0x63);
    m5E = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    for (i = 0; i < gal->pictureCount; i++) {
        k = gal->drawOrder[i];
        if (gal->pictures[k][0] != 0xFF) {
            Gallery_DrawPicture(gal->pictures[k], gal->pos[k]);
        }
    }
    Sel_DrawHeader(6);
    Gallery_DrawCategories(gal);
    if (gal->listShown != 0) {
        Gallery_DrawList(gal);
    }
    if (gal->pageShown != 0) {
        Gallery_DrawEntryPage(gal);
    }
    Gs_SetState(0x6C, m6C);
    Gs_SetState(0x63, m63);
    Gs_SetState(0x5E, m5E);
    if (gal->askTime == 1) {
        ListPanel_DrawAlpha(&gGalleryListPanel, 0, -1, 0x80);
        Text_SetColor(0);
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x101, 0xD7);
        Text_Printf((char *)gGalleryTitleTexts[gal->night]);
    }
    Font_GetPage();
}

/*
 * While fadeFrames counts down, covers the screen in black and restarts
 * the fade in.
 */
void Gallery_DrawFade(void)
{
    GalRect rec;
    GalWork *gal;
    s32 mode;

    rec = gGalleryFadeRect;
    gal = GAL;
    if (gal->fadeFrames != 0) {
        gal->fadeFrames--;
        mode = Gs_GetState(0x6C);
        Gs_SetState(0x6C, 0);
        Prim_FillRect((struct PrimLine *)&rec);
        Gs_SetState(0x6C, mode);
        FadeA_Start(2);
    }
}

/*
 * Draws the new mark beside a category name: large with a shadow (small
 * 0) or small. size is overwritten.
 */
void Gallery_DrawNew(s32 size, s16 x, s16 y, s32 small)
{
    u32 color;
    u8 fontSize;

    if (small == 0) {
        size = 0x12;
        color = 0x807F7F28;
        y -= 0x14;
        x -= 0xA;
    } else {
        size = 0xE;
        color = 0x80605B4B;
        y -= 0xC;
        x -= 0x14;
    }
    fontSize = size;
    Font_SetSpacing(fontSize, fontSize);
    if (small == 0) {
        Font_SetPos(x + 2, y + 2);
        Font_SetColor(0x80000000);
        Font_Printf((char *)gGalleryNewText[0]);
    }
    Font_SetPos(x, y);
    Font_SetColor(color);
    Font_Printf((char *)gGalleryNewText[0]);
}

void Gallery_DrawArrow(s32 k);

/* Draws the category names, the chosen one larger, with their new marks. */
void Gallery_DrawCategories(GalWork *gal)
{
    s32 i;
    s32 j;
    s32 x;
    s32 y;
    s32 size;
    u32 color;
    u8 fontSize;

    Font_SetSpacing(0x18, 0x18);
    for (i = 0; i < gal->categoryCount; i++) {
        j = 2 - i;
        if (Gallery_PictureMoving(j + 1) != 0 && gal->category != i) {
            continue;
        }
        x = gal->categoryPos[j][0] + ((s16 (*)[2])gGalleryCategoryOfs)[i][0];
        y = gal->categoryPos[j][1] + ((s16 (*)[2])gGalleryCategoryOfs)[i][1];
        x += (s32)gal->pos[j + 1][0];
        y += (s32)gal->pos[j + 1][1];
        if (i == gal->category) {
            size = 0x18;
            color = 0x807F7F7F;
        } else {
            color = 0x80101010;
            size = 0x14;
        }
        y -= size / 2;
        fontSize = size;
        Font_SetSpacing(fontSize, fontSize);
        Font_SetPos(x, y);
        Font_SetColor(color);
        Font_Printf((char *)gGalleryCategoryNames[i]);
        if (gal->categoryNew[i] == 1) {
            if (i == gal->category) {
                Gallery_DrawNew((s32)gal, x, y, 0);
            } else {
                Gallery_DrawNew((s32)gal, x, y, 1);
            }
        }
        if (i == gal->category) {
            Gallery_DrawArrow(i);
        }
    }
}

void Gallery_DrawRecords(GalRec *r, s32 n);

/* The list's frame, scroll bar and arrows (the bar's length and position
 * computed right after the step; box.h before the colour). */
void Gallery_DrawScrollBar(GalWork *gal)
{
    GalRec rec;
    GalRect box;
    s32 x;
    s32 y;
    s32 n;
    f32 step;
    s32 pos;
    s32 len;
    s32 top;

    rec = gGalleryRecs[0];
    x = gal->pos[5][0];
    y = gal->pos[5][1];
    rec.x = x + 0x28;
    rec.y = y + 0x14;
    Gallery_DrawRecords(&rec, 1);
    n = gal->entryCounts[gal->category];
    if (n > 4) {
        step = 184.0f / n;
        len = 4.0f * step;
        pos = (f32)gal->listTop * step;
        rec = gGalleryRecs[3];
        rec.x = x + 0x2C;
        top = y + 0x2E + pos;
        rec.y = top;
        Gallery_DrawRecords(&rec, 1);
        rec = *(GalRec *)((u8 *)gGalleryScrollBar + 0);
        rec.x = x + 0x2C;
        rec.y = len + top;
        Gallery_DrawRecords(&rec, 1);
        box.x = x + 0x32;
        box.w = box.x + 0xA;
        box.y = y + 0x44 + pos;
        box.h = box.y + len - 0x16;
        box.color = 0xFFF0EBE7;
        Prim_FillRect((struct PrimLine *)&box);
        if (gal->listTop != 0) {
            rec = gGalleryScrollRecs[0];
            rec.x = x + 0xA8;
            rec.y = y + 0x18 + gal->upArrowOfs;
            if (gal->upArrowOfs != 0) {
                rec.color = 0xFFFB7103;
            }
            rec.angle = 0x4000;
            Gallery_DrawRecords(&rec, 1);
        }
        if (gal->listTop + 4 < gal->entryCounts[gal->category]) {
            rec = gGalleryScrollRecs[0];
            rec.x = x + 0xA8;
            rec.y = y + 0xF4 + gal->downArrowOfs;
            if (gal->downArrowOfs != 0) {
                rec.color = 0xFFFB7103;
            }
            rec.angle = 0xC000;
            Gallery_DrawRecords(&rec, 1);
        }
    }
}

f32 Math_Sin(f32);
void Gallery_DrawRecords(GalRec *r, s32 n);

/*
 * Draws the list's cursor on row listRow, its width opening by the sine of
 * cursorPhase.
 */
void Gallery_DrawCursor(GalWork *gal)
{
    GalRec rec;
    s16 x;
    s16 y;
    x = 72.0f + gal->pos[5][0];
    y = (s32)(18.0f + (42.0f + (40.0f + gal->pos[5][1]))) + gal->listRow * 42;
    rec = *(GalRec *)((u8 *)gGalleryRecs + 0x38);
    rec.x = x;
    rec.y = y;
    rec.w = 224.0f * Math_Sin(6.2831855f * gal->cursorPhase / 65536.0f);
    Gallery_DrawRecords(&rec, 1);
}

/*
 * Draws the new mark of a list entry; with blink 1 it has a shadow and
 * blinks every 16 frames.
 */
void Gallery_DrawNewBlink(s32 unused, s16 x, s16 y, s32 blink)
{
    if (blink == 1 && (gFrameTick & 0x10)) {
        return;
    }
    Font_SetSpacing(0xE, 0xE);
    if (blink == 1) {
        Font_SetPos((s16)(x - 2) + 2, (s16)(y - 0xE) + 2);
        Font_SetColor(0x80000000);
        Font_Printf((char *)gGalleryNewText[0]);
    }
    Font_SetPos(x - 2, y - 0xE);
    Font_SetColor(0x807F7F28);
    Font_Printf((char *)gGalleryNewText[0]);
}

s32 sprintf(char *, const char *, ...);    /* sprintf */
char *strchr(char *, s32);                  /* strchr */
s32 Str_Length(char *);                         /* a message's width */
/* unit's own declaration: with the definition's void (s32 id, s32 x, s32 y, s32 depth, u32 color, f32 scale) (sub_main/unk_36BE70.c), Gallery_DrawEntryPage: 104 of 276 words differ, Gallery_DrawList: 38 of 137 words differ */
void Hud_DrawButtonIcon(s32, s16, s16, s32, u32, f32);  /* a button icon */
void Gallery_DrawScrollBar(GalWork *g);

/* Draws the list: four numbered entries from listTop, the selected one
 * yellow, a button code in a title drawn as a button icon. Kept for the
 * match: Gallery_DrawNewBlink takes s16 coordinates (the extensions at the
 * call); the strchr loop tests `!scan != 0` at its top (not rotated); and
 * bufStart, a copy of &buf set in the for's init, is the saved register
 * the game keeps for `scan - bufStart`. */
void Gallery_DrawList(GalWork *gal)
{
    char buf[0x108];
    char num[8];
    s32 i;
    s32 numberX;
    s32 y;
    s32 sel;
    u32 color;
    s32 x;
    char *scan;
    char *bufStart;

    x = 72.0f + gal->pos[5][0];
    y = (s32)(40.0f + gal->pos[5][1]) + 0x2A;
    for (i = 0, bufStart = buf; i < 4; i++) {
        sel = i == gal->listRow;
        if (gal->isNew[gal->listTop + i] == 1) {
            Gallery_DrawNewBlink((s32)gal, x, y, sel);
        }
        color = 0xFF000000;
        if (sel == 1) {
            color = 0xFFFFEA00;
        }
        Font_SetColor(color);
        sprintf(num, TXT(0x006D06E8) /* the entry's number (two digits), then a full-width colon */, gal->listTop + i + 1);
        Font_SetPos(x, y);
        Font_SetSpacing(0x12, 0x12);
        Font_Printf(num);
        numberX = x + Str_Length(num) * 9;
        Font_SetSpacing(0x10, 0x10);
        Font_SetPos(numberX, y + 1);
        strcpy(buf, gal->names[gal->listTop + i]);
        for (;;) {
            scan = strchr(buf, '%');
            if (!scan != 0) {
                break;
            }
            if (scan[1] == 'r') {
                Hud_DrawButtonIcon(0x19, numberX + (scan - bufStart) * 8, y + 1, 0x155, color, 0.8f);
                scan[0] = ' ';
                scan[1] = ' ';
            }
        }
        Font_Printf(buf);
        y += 0x2A;
    }
    Gallery_DrawCursor(gal);
    Gallery_DrawScrollBar(gal);
}

s32 strcmp(char *, char *);                 /* strcmp */
u32 strlen(char *);
/* The full-width opening double angle bracket, which opens a centred line,
 * with its padding (16 bytes; a literal would take 8). */
INCLUDE_RODATA("asm/select/nonmatchings/select_gallery", D_006D06F0);
extern char D_006D06F0[];

/* An entry's page (while pageShown): its picture, the frame and its four
 * parts, the four description lines (a line starting with that bracket is centred)
 * and the two help lines. centredX holds the centred line's left edge. */
void Gallery_DrawEntryPage(GalWork *gal)
{
    Sprite spr;
    GalRec rec;
    char head[0x10];
    char buf[0x100];
    s32 i;
    s32 tex;
    s16 *pagePos;
    GalEntry *entry;
    s32 y;
    char *scan;
    char **line;
    char *bufStart;
    s32 x;
    s32 centredX;
    char *text;

    pagePos = (s16 *)gGalleryEntryPagePos;
    entry = (GalEntry *)gGalleryEntries + gal->entries[gal->selected];
    if (Gallery_IsUnlocked(gal->entries[gal->selected]) == 1) {
        tex = entry->unk8 + 0x2B;
    } else {
        tex = 0x61;
    }
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x63, 0);
    Tex_Use(1, (u32 *)&gTexHandles[tex]);
    Gs_SetState(4, gTexHandles[tex]);
    spr.x = pagePos[1];
    spr.y = *(pagePos += 2);
    spr.w = pagePos[1];
    spr.h = *(pagePos += 2);
    spr.u1 = 0xFF;
    spr.v1 = 0xFF;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    Sprite_Draw(&spr);
    pagePos += 2;
    for (i = 0; i < 4; i++) {
        rec = gGalleryScrollRecs[pagePos[0]];
        rec.x = pagePos[1];
        rec.y = *(pagePos += 2);
        rec.w = pagePos[1];
        rec.h = *(pagePos += 2);
        rec.angle = pagePos[1];
        pagePos += 2;
        Gallery_DrawRecords(&rec, 1);
    }
    if (Gallery_IsUnlocked(gal->entries[gal->selected]) != 0) {
        Font_SetColor(0xFF65645A);
        Font_SetSpacing(0x10, 0x10);
        line = (char **)entry->unk4;
        bufStart = buf;
        y = 0x132;
        for (i = 0; i < 4; i++) {
            text = *line;
            head[0] = text[0];
            head[1] = text[1];
            head[2] = 0;
            if (strcmp(head, D_006D06F0) == 0) {
                x = (0x24 - Str_Length(*line)) * 8 / 2;
                Font_SetPos(x + 0x134, y);
                strcpy(buf, *line);
                centredX = x + 0x134;
                for (;;) {
                    scan = strchr(buf, '%');
                    if (!scan != 0) {
                        break;
                    }
                    if (scan[1] == 'r') {
                        Hud_DrawButtonIcon(0x19, centredX + (scan - bufStart) * 8, y, 0x155, 0xFFCAC8B4, 0.8f);
                        scan[0] = ' ';
                        scan[1] = ' ';
                    }
                }
                Font_Printf(buf);
            } else if (strlen(*line) != 0) {
                Font_SetPos(0x134, y);
                strcpy(buf, *line);
                for (;;) {
                    scan = strchr(buf, '%');
                    if (!scan != 0) {
                        break;
                    }
                    if (scan[1] == 'r') {
                        Hud_DrawButtonIcon(0x19, (scan - bufStart) * 8 + 0x134, y, 0x155, 0xFFCAC8B4, 0.8f);
                        scan[0] = ' ';
                        scan[1] = ' ';
                    }
                }
                Font_Printf(buf);
            }
            line++;
            y += 0x12;
        }
    }
    Font_SetColor(0xFF808080);
    Font_SetSpacing(0xE, 0xE);
    Font_SetPos(0x1A6, 0x18A);
    Text_Printf((char *)gGalleryPlayHelp[0]);
    Font_SetPos(0x1A6, 0x19C);
    Text_Printf((char *)gGalleryPlayHelp[1]);
}

struct TexRect;
void TexRect_DrawRotated(struct TexRect *r, s32 angle);

/* Draws a gallery picture's two parts (ids[i], 0xFF: none) at pos: parts
 * 2, 4 and 6 at their category name's place, part 8 with the texture
 * column of iconFrame. */
void Gallery_DrawPicture(s32 *ids, f32 *pos)
{
    Sprite spr;
    GalRec rec;
    GalWork *gal;
    s32 i;
    s32 k;
    GalRec *recPtr;

    gal = GAL;
    for (i = 0; i < 2; i++, ids++) {
        if (*ids == 0xFF) {
            continue;
        }
        *(recPtr = &rec) = *(GalRec *)((u8 *)gGalleryLayout + *ids * sizeof(GalRec));
        k = -1;
        if (*ids == 2) {
            k = 0;
        }
        if (*ids == 4) {
            k = 1;
        }
        if (*ids == 6) {
            k = 2;
        }
        if (k < 0) {
            spr.x = recPtr->x + (s16)pos[0];
            spr.y = recPtr->y + (s16)pos[1];
        } else {
            spr.x = (s16)gal->categoryPos[k][0] + (s16)pos[0];
            spr.y = (s16)gal->categoryPos[k][1] + (s16)pos[1];
        }
        spr.w = recPtr->w;
        spr.h = recPtr->h;
        spr.u0 = recPtr->u0;
        spr.v0 = recPtr->v0;
        spr.u1 = recPtr->u1;
        spr.v1 = recPtr->v1;
        if (*ids == 8) {
            spr.u0 = gal->iconFrame << 8;
            spr.u1 = spr.u0 + 0xFF;
        }
        spr.color = recPtr->color;
        if (spr.x < 0x280 && spr.x + spr.w >= 0 && spr.y < 0x1C0 && spr.y + spr.h >= 0) {
            Gs_SetState(0x5E, 0x32);
            Gs_SetState(0x63, 0);
            Tex_Use(1, (u32 *)&gal->texHandles[recPtr->tex]);
            Gs_SetState(4, gal->texHandles[recPtr->tex]);
            if (recPtr->angle != 0) {
                TexRect_DrawRotated((struct TexRect *)&spr, recPtr->angle);
            } else {
                Sprite_Draw(&spr);
            }
        }
    }
}

/* Draws n records (GalRec): a texture rectangle each, with its angle. */
void Gallery_DrawRecords(GalRec *rec, s32 n)
{
    GalWork *gal;
    Sprite spr;
    s32 i;

    gal = GAL;
    for (i = 0; i < n; i++, rec++) {
        spr.x = rec->x;
        spr.y = rec->y;
        spr.w = rec->w;
        spr.h = rec->h;
        spr.u0 = rec->u0;
        spr.v0 = rec->v0;
        spr.u1 = rec->u1;
        spr.v1 = rec->v1;
        spr.color = rec->color;
        if (spr.x < 0x280 && spr.x + spr.w >= 0 && spr.y < 0x1C0 && spr.y + spr.h >= 0) {
            Gs_SetState(0x5E, 0x32);
            Gs_SetState(0x63, 0);
            if (rec->tex > 100) {
                Tex_Use(1, (u32 *)&gTexHandles[rec->tex]);
                Gs_SetState(4, gTexHandles[rec->tex]);
            } else {
                Tex_Use(1, (u32 *)&gal->texHandles[rec->tex]);
                Gs_SetState(4, gal->texHandles[rec->tex]);
            }
            if (rec->angle != 0) {
                TexRect_DrawRotated((struct TexRect *)&spr, rec->angle);
            } else {
                Sprite_Draw(&spr);
            }
        }
    }
}

/* The pictures sliding to their places, and the panel sliding in (1) or
 * out (2). */
void Gallery_UpdatePictures(void)
{
    GalWork *gal;
    s32 i;

    gal = GAL;
    for (i = 0; i < 6; i++) {
        if (gal->moveState[i] >= 2) {
            if (gal->moveState[i] == 2) {
                if (gal->moveDelay[i] > 0) {
                    gal->moveDelay[i]--;
                    continue;
                }
                gal->moveState[i] = 3;
            }
            if (gal->moveState[i] == 3) {
                gal->pos[i][0] += gal->speed[i][0] * Math_Sin(6.2831855f * gal->movePhase[i] / 65536.0f);
                if (gal->speed[i][0] < 0.0f) {
                    if (!(gal->targetPos[i][0] < gal->pos[i][0])) {
                        gal->pos[i][0] = gal->targetPos[i][0];
                        gal->moveState[i] = 1;
                    }
                } else if (gal->targetPos[i][0] <= gal->pos[i][0]) {
                    gal->pos[i][0] = gal->targetPos[i][0];
                    gal->moveState[i] = 1;
                }
                gal->movePhase[i] += 0x800;
                if (gal->movePhase[i] >= 0x4000) {
                    gal->movePhase[i] = 0x4000;
                }
            }
        }
    }
    if (gal->panelSlide == 1) {
        gal->panelOffset = gal->panelOffset - (s32)(40.0f * Math_Sin(6.2831855f * gal->panelPhase / 65536.0f));
        gal->panelPhase += 0x800;
        if (gal->panelPhase >= 0x4000) {
            gal->panelPhase = 0x4000;
        }
    }
    if (gal->panelSlide == 2) {
        gal->panelOffset = gal->panelOffset + (s32)(40.0f * Math_Sin(6.2831855f * gal->panelPhase / 65536.0f));
        gal->panelPhase += 0x800;
        if (gal->panelPhase >= 0x4000) {
            gal->panelPhase = 0x4000;
        }
    }
}

/*
 * Starts the pictures moving to layout k (gGalleryLayoutPos): each waits
 * its delay, then moves at its speed (gGalleryLayoutSteps,
 * gGalleryLayoutSpeeds).
 */
void Gallery_SetLayout(s32 k)
{
    GalWork *gal;
    s32 *steps;
    f32 *speeds;
    s16 (*pos)[2];
    s32 i;

    gal = GAL;
    pos = gGalleryLayoutPos[k];
    steps = gGalleryLayoutSteps;
    speeds = gGalleryLayoutSpeeds;
    for (i = 0; i < 6; i++) {
        if (*steps == -1) {
        skip:
            pos++;
            steps++;
            speeds++;
        } else if ((f32)(*pos)[0] == gal->pos[i][0]) {
            goto skip;
        } else {
            gal->targetPos[i][0] = (*pos)[0];
            gal->targetPos[i][1] = (*pos)[1];
            pos++;
            gal->moveDelay[i] = *steps++;
            gal->speed[i][0] = *speeds++;
            gal->speed[i][1] = 0.0f;
            gal->movePhase[i] = 0;
            gal->moveState[i] = 2;
        }
    }
    gal->settleTimer = 0;
}

/*
 * Starts the pictures moving back to the menu's layout, at the reversed
 * speeds (gGalleryLayoutBackSteps).
 */
void Gallery_SetLayoutBack(s32 unused)
{
    GalWork *gal;
    s32 *steps;
    f32 *speeds;
    s16 (*pos)[2];
    s32 i;

    gal = GAL;
    pos = gGalleryLayoutPos[0];
    steps = gGalleryLayoutBackSteps;
    speeds = gGalleryLayoutSpeeds;
    for (i = 0; i < 6; i++) {
        if (*steps == -1) {
        skip:
            pos++;
            steps++;
            speeds++;
        } else if ((f32)(*pos)[0] == gal->pos[i][0]) {
            goto skip;
        } else {
            gal->targetPos[i][0] = (*pos)[0];
            gal->targetPos[i][1] = (*pos)[1];
            pos++;
            gal->moveDelay[i] = *steps++;
            gal->speed[i][0] = *speeds++;
            gal->speed[i][0] = gal->speed[i][0] * -1.0f;
            gal->speed[i][1] = 0.0f;
            gal->movePhase[i] = 0;
            gal->moveState[i] = 2;
        }
    }
    gal->settleTimer = 0;
    gal->iconTimer = 0;
    gal->iconFrame = 0;
}

/* Whether picture i waits or moves. */
s32 Gallery_PictureMoving(s32 i)
{
    GalWork *gal = (GalWork *)((s32)gSysSettings.workArea + 0x1000); /* GAL changes the code here */

    return gal->moveState[i] >= 2;
}

/*
 * Counts a frame of the layout's move (the category names follow at frame
 * 10 on the menu, 8 otherwise) and returns whether every picture rests.
 */
s32 Gallery_Settled(void)
{
    SelWork *base;
    GalWork *gal;
    s32 i;

    base = (SelWork *)gSysSettings.workArea;
    base->gallery.settleTimer++;
    gal = &base->gallery;
    if (base->gallery.state == GAL_CATEGORIES) {
        if (gal->settleTimer == 0xA) {
            Gallery_ApplyLayout(gal, gal->category + 1);
        }
    } else if (gal->settleTimer == 8) {
        Gallery_ApplyLayout(gal, 0);
    }
    for (i = 0; i < 6; i++) {
        if (gal->moveState[i] >= 2) {
            return 0;
        }
    }
    return 1;
}

/* Fills the names and new marks of the category's listed entries. */
void Gallery_FillNames(GalWork *gal)
{
    s32 i;
    s32 idx;

    gal->selected = Gallery_EntryIndex(gal->category, 0);
    for (i = 0; i < 32; i++) {
        gal->names[i][0] = 0;
    }
    for (i = 0; i < gal->entryCounts[gal->category]; i++) {
        idx = Gallery_EntryIndex(gal->category, i);
        if (Gallery_IsUnlocked(gal->entries[idx]) == 0) {
            strcpy(gal->names[i], TXT(0x006D0700) /* a row of full-width question marks (an entry not unlocked yet) */);
        } else {
            strcpy(gal->names[i], ((GalEntry *)gGalleryEntries)[gal->entries[idx]].title);
        }
        if (Gallery_IsNew(gal->entries[idx]) == 1) {
            gal->isNew[i] = 1;
        } else {
            gal->isNew[i] = 0;
        }
    }
    gal->listTop = 0;
    gal->listRow = 0;
    gal->listShown = 1;
}

/*
 * The list's input: up and down move the cursor's row, scrolling the list
 * at its ends with its arrow bouncing; opens the cursor. Returns 1 when
 * the list scrolled.
 */
s32 Gallery_ListInput(GalWork *gal, u16 pad)
{
    u16 b;

    gal->cursorPhase += 0x1000;
    if (gal->cursorPhase >= 0x4000) {
        gal->cursorPhase = 0x4000;
    }
    if (gal->upArrowBounce != 0) {
        if (gal->upArrowBounce == 1) {
            gal->upArrowOfs--;
            if (gal->upArrowOfs < -4) {
                gal->upArrowBounce = 2;
            }
        }
        if (gal->upArrowBounce == 2) {
            gal->upArrowOfs++;
            if (gal->upArrowOfs >= 0) {
                gal->upArrowBounce = 0;
            }
        }
    }
    if (gal->downArrowBounce != 0) {
        if (gal->downArrowBounce == 1) {
            gal->downArrowOfs++;
            if (gal->downArrowOfs >= 5) {
                gal->downArrowBounce = 2;
            }
        }
        if (gal->downArrowBounce == 2) {
            gal->downArrowOfs--;
            if (gal->downArrowOfs <= 0) {
                gal->downArrowBounce = 0;
            }
        }
    }
    b = pad;
    if (b & 0x2000) {
        if (gal->listRow == 0) {
            if (gal->listTop > 0) {
                gal->listTop--;
                goto up;
            }
        } else {
            gal->listRow--;
            goto up;
        }
    }
    if (b & 0x1000) {
        if (gal->listRow >= 3) {
            if (gal->listTop + 4 < gal->entryCounts[gal->category]) {
                gal->listTop++;
                goto down;
            }
        } else {
            gal->listRow++;
            goto down;
        }
    }
    return 0;
up:
    gal->selected = Gallery_EntryIndex(gal->category, gal->listTop + gal->listRow);
    gal->cursorPhase = 0;
    Gallery_PlayPageSe();
    gal->upArrowBounce = 1;
    return 1;
down:
    gal->selected = Gallery_EntryIndex(gal->category, gal->listTop + gal->listRow);
    gal->cursorPhase = 0;
    Gallery_PlayPageSe();
    gal->downArrowBounce = 1;
    return 1;
}

void Gallery_PlayDecideSe(void)
{
    Sound_PlaySeFlat(0, 0x13, 0);
}

void Gallery_PlayCancelSe(void)
{
    Sound_PlaySeFlat(0, 0x14, 0);
}

void Gallery_PlayPageSe(void)
{
    Sound_PlaySeFlat(0, 0x17, 0);
}

/* Draws the arrows beside category k on the menu. */
void Gallery_DrawArrow(s32 k)
{
    GalWork *gal;
    Sprite spr;
    s16 (*pos)[2];
    GalIcon *icon;

    gal = GAL;
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x63, 0);
    Tex_Use(1, (u32 *)&gTexHandles[gGalleryArrowSprites[0].tex]);
    Gs_SetState(4, gTexHandles[gGalleryArrowSprites[0].tex]);
    if (gal->state >= 2 && gal->state < 5) {
        pos = &gGalleryArrowPosB[k];
    } else {
        pos = &gGalleryArrowPosA[k];
    }
    icon = &gGalleryArrowSprites[gal->animFrame];
    spr.w = icon->w;
    spr.h = icon->h;
    spr.color = 0xFFFF6000;
    spr.x = (*pos)[0];
    spr.y = (*pos)[1];
    if (gal->panelSlide != 0) {
        spr.y += gal->panelOffset;
        if (gal->panelSlide == 2 && spr.y >= ((s16 *)(gGalleryArrowPosA + k))[1]) {
            spr.y = ((s16 *)(gGalleryArrowPosA + k))[1];
        }
    }
    spr.u0 = icon->u0;
    spr.v0 = icon->v0;
    spr.u1 = icon->u1;
    spr.v1 = icon->v1;
    Sprite_Draw(&spr);
}

/* Starts the list panel sliding in (back 0) or out. */
void Gallery_StartPanel(GalWork *gal, s32 back)
{
    gal->panelSlide = (s8) (back + 1);
    if (back == 0) {
        gal->panelOffset = 0;
    } else {
        gal->panelOffset = -0x104;
    }
    gal->panelPhase = 0;
    gal->animFrame = 3;
}

/* Stops the list panel's slide and its animation. */
void Gallery_ResetPanel(GalWork *gal)
{
    gal->animTimer = 0;
    gal->animFrame = 0;
    gal->panelSlide = 0;
    gal->panelPhase = 0;
}
