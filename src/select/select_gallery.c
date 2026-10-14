/*
 * select: GALLERY (the monster list, world exploration, bonus movies)
 * (0x6BF3B0-0x6C2DB0). The file owns its .data (0x6CB758-0x6CC1E0: the
 * monster descriptions and movie tables) and .rodata (0x6CE188-0x6D0720).
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

/* GALLERY's work, at 0x1000 in the shared work area. */
typedef struct GalWork {
    s8 unk0;                /* 0x000 */
    s8 unk1;                /* 0x001 */
    u8 unk2[0xA];
    s32 unkC[16];           /* 0x00C: texture handles */
    s32 unk4C;              /* 0x04C */
    s32 unk50[6];           /* 0x050 */
    s32 unk68;              /* 0x068 */
    s32 unk6C;              /* 0x06C: categories */
    s32 unk70[6];           /* 0x070 */
    s32 unk88[6][2];        /* 0x088: the six pictures (0xFF: none) */
    s32 unkB8[6];           /* 0x0B8: their drawing order */
    f32 unkD0[6][2];        /* 0x0D0: the six pictures' target positions */
    f32 unk100[6][2];       /* 0x100: their speeds */
    f32 unk130[6][2];       /* 0x130: their positions */
    u32 unk160[6];          /* 0x160: the pictures' move phases */
    s32 unk178;             /* 0x178: pictures to draw */
    s32 unk17C[3][2];       /* 0x17C: the category names' positions */
    u8 unk194[0xC];
    s8 unk1A0;              /* 0x1A0 */
    s8 unk1A1;              /* 0x1A1 */
    s8 unk1A2;              /* 0x1A2 */
    u8 unk1A3;
    s16 unk1A4;             /* 0x1A4 */
    u16 unk1A6;             /* 0x1A6 */
    s32 unk1A8;             /* 0x1A8 */
    s32 unk1AC;             /* 0x1AC */
    s32 unk1B0[3];          /* 0x1B0: entries per category */
    s32 unk1BC;             /* 0x1BC */
    s32 unk1C0;             /* 0x1C0 */
    s32 unk1C4;             /* 0x1C4: the list is shown */
    u32 unk1C8;             /* 0x1C8: cursor blink */
    s32 unk1CC;             /* 0x1CC */
    s32 unk1D0;             /* 0x1D0: frames of the closing fade */
    s32 unk1D4;             /* 0x1D4: up arrow offset */
    s32 unk1D8;             /* 0x1D8: down arrow offset */
    s32 unk1DC;             /* 0x1DC: up arrow bounce (1 out, 2 back) */
    s32 unk1E0;             /* 0x1E0: down arrow bounce */
    char unk1E4[32][0x100]; /* 0x1E4: the listed entries' names */
    s8 unk21E4[32];         /* 0x21E4: ... and whether they are new */
    u8 unk2204[4];          /* 0x2204: per category: something new to see */
    s32 unk2208;            /* 0x2208 */
    u8 unk220C[0x36];       /* 0x220C: the entries listed (0xFF: none) */
    u8 unk2242;             /* 0x2242: how many */
    u8 unk2243;
    s32 unk2244;            /* 0x2244: day or night */
    s32 unk2248;            /* 0x2248 */
} GalWork;

/* gGalleryEntries's records: the gallery's entries. */
typedef struct GalEntry {
    char *title;            /* 0x00 */
    void *unk4;             /* 0x04 */
    u8 unk8;
    u8 kind;                /* 0x09 */
    u8 unkA;
    u8 unkB;                /* 0x0B: shown only once unlocked */
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

/* The shared work area as GALLERY sees it. */
typedef struct GalBase {
    u8 unk0[0x1000];
    GalWork gal;            /* 0x1000 */
} GalBase;

#define GAL (&((GalBase *)gSysSettings.workArea)->gal)

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
void Task_SetGmSub(void);
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
u32 gGalleryNextEntry[8] = {
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


void Gallery_SetSeen(s32 n) {
    D_6ADABC[n / 32] |= 1 << (n % 32);
}

s32 Gallery_IsUnlocked(s32 n) {
    return EventFlag_Test(n);
}

s32 Gallery_IsNew(s32 n) {
    s32 w;

    if (EventFlag_Test(n) == 1) {
        w = D_6ADABC[n / 32];
        if (!(w & (1 << (n % 32)))) {
            return 1;
        }
    }
    return 0;
}

void Gallery_PlayCursorSe(void) {
    Sound_PlaySeFlat(1, 0xC, 0);
}

void Gallery_PlayOpenSe(void) {
    Sound_PlaySeFlat(1, 0xB, 0);
}

void Gallery_PlayCloseSe(void) {
    Sound_PlaySeFlat(1, 0xB, 0);
}

void Gallery_PlayErrorSe(void) {
    Sound_PlaySeFlat(0, 0x15, 0);
}

void FlushCache(s32);
struct SndFile;
s32 Sound_LoadBankWait(struct SndFile *f, u32 slot); /* main: f is a SndFile * */
s32 Sound_ReadBank(u32 id, void *buf);
void Gallery_LoadSounds(void) {
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

void Gallery_Task(TaskSlot *t) {
    switch (t->mode) {
    case 0:
        Gallery_Init(t);
        t->mode++;
        break;
    case 1:
        if (Gallery_Update(t) == 0) {
            break;
        }
        t->mode++;
    case 2:
        Gallery_Exit(t);
        break;
    }
}

void Gallery_LoadTextures(void) {
    s32 i;
    s32 *p;
    s32 cur;
    s32 prev;

    Afs_LoadPartition(5, 0x19F4, gSysSettings.readBuf, "galex");
    for (i = 0, p = gGalleryTex; i < 0x37; i++, p++) {
        cur = gVSyncCount;
        prev = gLoadLastVSync;
        if (prev != cur) {
            func_36BDF0(cur - prev, 0);
            gLoadLastVSync = gVSyncCount;
            Sys_FlipFrame(0);
        }
        Afs_ReadFileIdle(i, 5, gSysSettings.loadBuf);
        *p = Tex_CreateFromTim2(gSysSettings.loadBuf, 4);
    }
    Mem_Compact();
}

void Gallery_FreeTextures(void) {
    Tex_FreeRange(0x2B, 0x37);
    Mem_Compact();
}

void EventFlag_Set(s32);
void Gallery_UpdateNewFlags(void);
s32 Gallery_BuildList(s32 cat);
void Gallery_LoadSounds(void);
void Gallery_LoadTextures(void);
extern char gGalleryTitle[];

/* GALLERY's start: clears its work and the save buffer, takes the
 * texture handles, places the six pictures (layout 0) and the category
 * names, and builds the three lists. */
void Gallery_Init(TaskSlot *t) {
    GalRec *r;
    u8 *p;
    s16 *ps;
    GalWork *g;
    s32 i;
    s32 j;
    GalWork *gp;

    gGalleryTitle[0] = 0;
    Loading_Start();
    g = GAL;
    memset(g, 0, 0x224C);
    memset(&gSave, 0, 0x37F8);
    Sel_LoadCommonTextures();
    for (j = 0xA3, i = 0; i < 16; i++, j++) {
        g->unkC[i] = gTexHandles[j];
    }
    Gallery_LoadTextures();
    for (i = 0, p = (u8 *)gGalleryInitBytes; i < 6; i++) {
        g->unk88[i][0] = *p++;
        g->unk88[i][1] = *p++;
    }
    for (i = 0, ps = (s16 *)gGalleryLayoutPos[0]; i < 6; i++) {
        g->unk130[i][0] = *ps++;
        g->unk130[i][1] = *ps++;
        g->unk70[i] = 1;
        g->unkB8[i] = i;
        g->unk178++;
    }
    for (r = (GalRec *)gGalleryLayout, i = 0, gp = g; i < 10; i++, r++) {
        if (i == 2 || i == 4 || i == 6) {
            gp->unk17C[0][0] = r->x;
            gp->unk17C[0][1] = r->y;
            gp = (GalWork *)((u8 *)gp + 8);
        }
    }
    EventFlag_Set(0x30);
    g->unk6C = 3;
    g->unk1B0[0] = Gallery_BuildList(0);
    g->unk1B0[1] = Gallery_BuildList(1);
    g->unk1B0[2] = Gallery_BuildList(2);
    Loading_Stop();
    Gallery_UpdateNewFlags();
    if (gSysSettings.galleryPlay == 1) {
        Gallery_LoadSounds();
    }
}

static void Gallery_ApplyLayout(GalWork *g, s32 k) {
    s16 *tbl = (s16 *)gGalleryLayout;

    switch (k) {
    case 0:
        g->unk17C[0][1] = tbl[0x3E / 2];
        g->unk17C[1][1] = tbl[0x76 / 2];
        g->unk17C[2][1] = tbl[0xAE / 2];
        break;
    case 1:
        g->unk17C[2][1] = tbl[0xAE / 2];
        break;
    case 2:
        g->unk17C[1][1] = tbl[0xAE / 2];
        break;
    case 3:
        g->unk17C[0][1] = tbl[0xAE / 2];
        break;
    }
}

s32 Gallery_EntryInList(GalWork *g) {
    s32 idx;
    u8 *p;
    u8 c;

    idx = g->unk2208;
    switch (((GalEntry *)gGalleryEntries)[g->unk220C[idx]].kind) {
    case 0:
    case 1:
    case 2:
        c = *(u8 *)gGalleryEntryList;
        p = (u8 *)gGalleryEntryList;
        if (c != 0xFF) {
            do {
                if (c == g->unk220C[idx]) {
                    return 1;
                }
                c = *++p;
            } while (c != 0xFF);
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
u8 SeqA_GetState(void);
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
void SeqA_Reset(void);
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

/* GALLERY's task, every frame: the category menu (1), the lists of the
 * three categories (2-4), a picture shown by the game (5), a movie (6),
 * the network menu (7) and the fade out (8, 9: done). */
s32 Gallery_Update(TaskSlot *t) {
    GalWork *g;
    GalEntry *e;
    u16 trig;
    u16 rep;
    s32 i;

    trig = gPadState[0].pressed;
    rep = trig | gPadState[0].repeat;
    g = GAL;
    switch (g->unk0) {
    case 0:
        g->unk0++;
        g->unk1 = 0;
        SeqA_Start(2);
        break;
    case 1:
        switch (g->unk1) {
        case 0:
            if (gSysSettings.galleryPlay == 1) {
                g->unk1D0 = 0x14;
                SeqA_Start(2);
                Gallery_ResetPanel(g);
                g->unk68 = gSysSettings.gallerySel;
                g->unk1++;
            } else {
                Gallery_ResetPanel(g);
                if ((u8)SeqA_GetState() == 1) {
                    break;
                }
                g->unk1++;
            }
        case 1:
            if (gSysSettings.galleryPlay == 1) {
                g->unk1D0 = 0x14;
                SeqA_Start(2);
                g->unk68 = gSysSettings.gallerySel;
                goto open;
            }
            if (rep & 0x2000) {
                g->unk68--;
                if (g->unk68 < 0) {
                    g->unk68 += g->unk6C;
                }
                Gallery_PlayCursorSe();
            } else if (rep & 0x1000) {
                g->unk68++;
                if (g->unk68 >= g->unk6C) {
                    g->unk68 -= g->unk6C;
                }
                Gallery_PlayCursorSe();
            } else if (trig & 0x20) {
                Gallery_PlayOpenSe();
            open:
                Gallery_FillNames(g);
                Gallery_SetLayout(g->unk68 + 1);
                g->unk1++;
                Gallery_StartPanel(g, 0);
            } else if (trig & 0x40) {
                NetMenu_Open(1);
                g->unk0 = 7;
                g->unk1 = 0;
                Gallery_PlayCancelSe();
            } else {
                g->unk1A8++;
                if (!(g->unk1A8 & 3)) {
                    g->unk1AC++;
                    g->unk1AC %= 3;
                }
                g->unk1A0++;
                if (!(g->unk1A0 & 3)) {
                    g->unk1A1++;
                    g->unk1A1 %= 3;
                }
            }
            break;
        case 2:
            if (gSysSettings.galleryPlay == 1) {
                SeqA_Start(2);
            }
            if (Gallery_Settled() != 0) {
                if (gSysSettings.galleryPlay == 1) {
                    gSysSettings.galleryPlay = 0;
                    g->unk1D0 = 0x14;
                    g->unk0 = gSysSettings.gallerySel + 2;
                    g->unk1 = 0;
                    g->unk2208 = 0;
                    for (i = 0; i < g->unk2242; i++) {
                        if (g->unk220C[i] == gSysSettings.galleryEntry) {
                            g->unk2208 = i;
                            break;
                        }
                    }
                    g->unk1BC = gSysSettings.galleryKeep[0];
                    g->unk1C0 = gSysSettings.galleryKeep[1];
                    SeqA_Start(2);
                } else {
                    g->unk0 = g->unk68 + 2;
                    g->unk1 = 0;
                }
                Gallery_ResetPanel(g);
            }
            break;
        }
        break;
    case 2:
    case 3:
    case 4:
        switch (g->unk1) {
        case 0:
            g->unk1CC = 1;
            if (Gallery_ListInput(g, rep) == 1) {
                break;
            }
            if ((u8)SeqA_GetState() == 1) {
                break;
            }
            if (trig & 0x40) {
                Gallery_PlayCloseSe();
                g->unk1CC = 0;
                Gallery_SetLayoutBack(0);
                Gallery_StartPanel(g, 1);
                g->unk1++;
            } else if (trig & 0x20) {
                if (Gallery_IsUnlocked(g->unk220C[g->unk2208]) == 1) {
                    Gallery_SetSeen(g->unk220C[g->unk2208]);
                    Gallery_PlayDecideSe();
                    g->unk2244 = 0;
                    if (Gallery_EntryInList(g) == 1) {
                        g->unk2248 = 1;
                        g->unk1 = 3;
                    } else {
                        g->unk1 = 2;
                        SeqA_Start(1);
                    }
                } else {
                    Gallery_PlayErrorSe();
                }
            } else {
                g->unk1A0++;
                if (!(g->unk1A0 & 3)) {
                    g->unk1A1++;
                    g->unk1A1 %= 3;
                }
            }
            break;
        case 1:
            if (Gallery_Settled() != 0) {
                g->unk1C4 = 0;
                g->unk0 = 1;
                g->unk1 = 0;
            }
            break;
        case 2:
            if ((u8)SeqA_GetState() == 1) {
                break;
            }
            e = &((GalEntry *)gGalleryEntries)[g->unk220C[g->unk2208]];
            switch (e->kind) {
            case 4:
                gSysSettings.galleryPlay = 1;
                gSysSettings.gallerySel = g->unk0 - 2;
                gSysSettings.galleryEntry = g->unk220C[g->unk2208];
                gSysSettings.galleryKeep[0] = g->unk1BC;
                gSysSettings.galleryKeep[1] = g->unk1C0;
                g->unk0 = 5;
                g->unk1 = 0;
                strcpy(gGalleryTitle, g->unk1E4[g->unk1BC + g->unk1C0]);
                break;
            case 3:
                gSysSettings.gallerySel = g->unk0 - 2;
                gSysSettings.galleryEntry = g->unk220C[g->unk2208];
                gSysSettings.galleryKeep[0] = g->unk1BC;
                gSysSettings.galleryKeep[1] = g->unk1C0;
                if (((u16 *)gGalleryNextEntry)[e->unkA] != 0xFFFF) {
                    g->unk0 = 6;
                    g->unk1 = 0;
                }
                break;
            case 0:
            case 1:
                Game_ResetSubsystems();
                SeqA_Reset();
                gSysSettings.gallerySel = g->unk0 - 2;
                gSysSettings.galleryEntry = g->unk220C[g->unk2208];
                if (g->unk2244 != 0) {
                    gSysSettings.galleryEntry = (gSysSettings.galleryEntry & 0x7F) | 0x80;
                }
                gSysSettings.galleryKeep[0] = g->unk1BC;
                gSysSettings.galleryKeep[1] = g->unk1C0;
                gSysSettings.galleryPlay = 1;
                strcpy(gGalleryTitle, g->unk1E4[g->unk1BC + g->unk1C0]);
                Loading_SetMessage(5);
                Scene_LoadOverlay(0xB);
                TaskSlot_Set(Galex_QuestTask, 6);
                TaskSlot_End(t);
                return 0;
            case 2:
                Game_ResetSubsystems();
                SeqA_Reset();
                gSysSettings.gallerySel = g->unk0 - 2;
                gSysSettings.galleryEntry = g->unk220C[g->unk2208];
                if (g->unk2244 != 0) {
                    gSysSettings.galleryEntry = (gSysSettings.galleryEntry & 0x7F) | 0x80;
                }
                gSysSettings.galleryKeep[0] = g->unk1BC;
                gSysSettings.galleryKeep[1] = g->unk1C0;
                gSysSettings.galleryPlay = 1;
                strcpy(gGalleryTitle, g->unk1E4[g->unk1BC + g->unk1C0]);
                Loading_SetMessage(5);
                Scene_LoadOverlay(0xB);
                TaskSlot_Set(Galex_Task, 6);
                TaskSlot_End(t);
                return 0;
            }
            break;
        case 3:
            if ((trig & 0x400) || (trig & 0x800)) {
                Gallery_PlayPageSe();
                g->unk2244 ^= 1;
            } else if (trig & 0x40) {
                g->unk2248 = 0;
                Gallery_PlayCancelSe();
                g->unk1 = 0;
            } else if (trig & 0x20) {
                g->unk2248 = 0;
                Gallery_PlayDecideSe();
                g->unk1 = 2;
                SeqA_Start(1);
            }
            break;
        }
        break;
    case 5:
        switch (g->unk1) {
        case 0:
            SeqA_Reset();
            Scene_LoadOverlay(0xB);
            Sel_FreeTextures();
            Gallery_FreeTextures();
            Ending_Init();
            g->unk1++;
            return 0;
        case 1:
            if (func_D64D40() == 0) {
                g->unk1++;
                SeqA_Start(2);
            }
            return 0;
        case 2:
            if ((u8)SeqA_GetState() != 1) {
                Gallery_GetTextures(g);
                Gallery_FillNames(g);
                Gallery_UpdateNewFlags();
                g->unk0 = gSysSettings.gallerySel + 2;
                g->unk1 = 0;
                g->unk2208 = gSysSettings.galleryEntry;
                g->unk1BC = gSysSettings.galleryKeep[0];
                g->unk1C0 = gSysSettings.galleryKeep[1];
                SeqA_Start(2);
                g->unk1D0 = 0x14;
                gSysSettings.plselLoaded = 0;
                gSysSettings.galleryPlay = 0;
            }
            return 0;
        }
        break;
    case 6:
        switch (g->unk1) {
        case 0:
            SeqA_Reset();
            Scene_LoadOverlay(0xB);
            Sel_FreeTextures();
            Gallery_FreeTextures();
            Movie_PrePlay();
            Movie_Play(((u16 *)gGalleryNextEntry)[((GalEntry *)gGalleryEntries)[g->unk220C[g->unk2208]].unkA]);
            g->unk1++;
        case 1:
            Movie_Update();
            if (Movie_GetStatus() == 3 || (gPadState[0].pressed & 0x40)) {
                Movie_RequestStop();
                g->unk1++;
                SeqA_Start(2);
            }
            return 0;
        case 2:
            if ((u8)SeqA_GetState() != 1) {
                Movie_Stop();
                Gallery_GetTextures(g);
                Gallery_FillNames(g);
                Gallery_UpdateNewFlags();
                g->unk0 = gSysSettings.gallerySel + 2;
                g->unk1 = 0;
                g->unk2208 = gSysSettings.galleryEntry;
                g->unk1BC = gSysSettings.galleryKeep[0];
                g->unk1C0 = gSysSettings.galleryKeep[1];
                SeqA_Start(2);
                gSysSettings.plselLoaded = 0;
            }
            return 0;
        }
        break;
    case 7:
        if ((u8)NetMenu_Update()) {
            g->unk0 = 8;
            SeqA_Start(1);
        }
        break;
    case 8:
        if ((u8)SeqA_GetState() != 1) {
            g->unk0 = 9;
            g->unk1 = 0;
        }
        break;
    case 9:
        gSysSettings.galleryPlay = 0;
        return 1;
    }
    Gallery_UpdatePictures();
    Gallery_Draw();
    Frame_Draw();
    Gallery_DrawFade();
    return 0;
}

void Gallery_UpdateNewFlags(void) {
    s32 i;
    s32 j;
    s32 end;
    GalWork *g;

    g = GAL;
    for (i = 0; i < 3; i++) {
        j = gGalleryCategoryFirst[i];
        end = gGalleryCategoryEnd[i];
        g->unk2204[i] = 0;
        for (; j < end; j++) {
            if (Gallery_IsNew(j) == 1) {
                g->unk2204[i] = 1;
                break;
            }
        }
    }
}

s32 Gallery_BuildList(s32 cat) {
    s32 i;
    s32 start;
    s32 end;
    s32 n;
    GalEntry *e;
    GalWork *g;

    g = GAL;
    start = gGalleryCategoryFirst[cat];
    end = gGalleryCategoryEnd[cat];
    e = (GalEntry *)gGalleryEntries + start;
    n = 0;
    for (i = start; i < end; i++, e++) {
        if (e->unkB == 0) {
            n++;
            g->unk220C[g->unk2242++] = i;
        } else if (EventFlag_Test(i) == 1) {
            n++;
            g->unk220C[g->unk2242++] = i;
        }
    }
    for (i = start + n; i < end; i++) {
        g->unk220C[g->unk2242++] = 0xFF;
    }
    return n;
}

static s32 Gallery_EntryIndex(s32 i, s32 n) {
    return gGalleryCategoryFirst[i] + n;
}

void Gallery_Exit(TaskSlot *t) {
    Sel_FreeTextures();
    Gallery_FreeTextures();
    TaskSlot_End(t);
    LoadOverlay(2);
    Task_SetGmSub();
    SeqA_Start(2);
}

void Gallery_GetTextures(GalWork *g) {
    s32 j;
    s32 i;

    Loading_Start();
    Sel_LoadCommonTextures();
    for (j = 0xA3, i = 0; i < 16; i++, j++) {
        g->unkC[i] = gTexHandles[j];
    }
    Gallery_LoadTextures();
    Loading_Stop();
}

void Gallery_Draw(void) {
    GalWork *g;
    s32 i;
    s32 m6C;
    s32 m63;
    s32 m5E;
    s32 k;

    g = GAL;
    m6C = Gs_GetState(0x6C);
    m63 = Gs_GetState(0x63);
    m5E = Gs_GetState(0x5E);
    Gs_SetState(0x6C, 0);
    for (i = 0; i < g->unk178; i++) {
        k = g->unkB8[i];
        if (g->unk88[k][0] != 0xFF) {
            Gallery_DrawPicture(g->unk88[k], g->unk130[k]);
        }
    }
    Sel_DrawHeader(6);
    Gallery_DrawCategories(g);
    if (g->unk1C4 != 0) {
        Gallery_DrawList(g);
    }
    if (g->unk1CC != 0) {
        Gallery_DrawEntryPage(g);
    }
    Gs_SetState(0x6C, m6C);
    Gs_SetState(0x63, m63);
    Gs_SetState(0x5E, m5E);
    if (g->unk2248 == 1) {
        ListPanel_DrawAlpha(&gGalleryListPanel, 0, -1, 0x80);
        Text_SetColor(0);
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0x101, 0xD7);
        Text_Printf((char *)gGalleryTitleTexts[g->unk2244]);
    }
    Font_GetPage();
}

void Gallery_DrawFade(void) {
    GalRect r;
    GalWork *g;
    s32 mode;

    r = gGalleryFadeRect;
    g = GAL;
    if (g->unk1D0 != 0) {
        g->unk1D0--;
        mode = Gs_GetState(0x6C);
        Gs_SetState(0x6C, 0);
        Prim_FillRect((struct PrimLine *)&r);
        Gs_SetState(0x6C, mode);
        SeqA_Start(2);
    }
}

void Gallery_DrawNew(s32 size, s16 x, s16 y, s32 small) {
    u32 color;
    u8 s;

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
    s = size;
    Font_SetSpacing(s, s);
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

/* The category names (MONSTERS, WORLD, BONUS). */
void Gallery_DrawCategories(GalWork *g) {
    s32 i;
    s32 j;
    s32 x;
    s32 y;
    s32 size;
    u32 color;
    u8 s;

    Font_SetSpacing(0x18, 0x18);
    for (i = 0; i < g->unk6C; i++) {
        j = 2 - i;
        if (Gallery_PictureMoving(j + 1) != 0 && g->unk68 != i) {
            continue;
        }
        x = g->unk17C[j][0] + ((s16 (*)[2])gGalleryCategoryOfs)[i][0];
        y = g->unk17C[j][1] + ((s16 (*)[2])gGalleryCategoryOfs)[i][1];
        x += (s32)g->unk130[j + 1][0];
        y += (s32)g->unk130[j + 1][1];
        if (i == g->unk68) {
            size = 0x18;
            color = 0x807F7F7F;
        } else {
            color = 0x80101010;
            size = 0x14;
        }
        y -= size / 2;
        s = size;
        Font_SetSpacing(s, s);
        Font_SetPos(x, y);
        Font_SetColor(color);
        Font_Printf((char *)gGalleryCategoryNames[i]);
        if (g->unk2204[i] == 1) {
            if (i == g->unk68) {
                Gallery_DrawNew((s32)g, x, y, 0);
            } else {
                Gallery_DrawNew((s32)g, x, y, 1);
            }
        }
        if (i == g->unk68) {
            Gallery_DrawArrow(i);
        }
    }
}

void Gallery_DrawRecords(GalRec *r, s32 n);

/* The list's frame, scroll bar and arrows (the bar's length and position
 * computed right after the step; box.h before the colour). */
void Gallery_DrawScrollBar(GalWork *g) {
    GalRec r;
    GalRect box;
    s32 x;
    s32 y;
    s32 n;
    f32 step;
    s32 pos;
    s32 len;
    s32 top;

    r = gGalleryRecs[0];
    x = g->unk130[5][0];
    y = g->unk130[5][1];
    r.x = x + 0x28;
    r.y = y + 0x14;
    Gallery_DrawRecords(&r, 1);
    n = g->unk1B0[g->unk68];
    if (n > 4) {
        step = 184.0f / n;
        len = 4.0f * step;
        pos = (f32)g->unk1BC * step;
        r = gGalleryRecs[3];
        r.x = x + 0x2C;
        top = y + 0x2E + pos;
        r.y = top;
        Gallery_DrawRecords(&r, 1);
        r = *(GalRec *)((u8 *)gGalleryScrollBar + 0);
        r.x = x + 0x2C;
        r.y = len + top;
        Gallery_DrawRecords(&r, 1);
        box.x = x + 0x32;
        box.w = box.x + 0xA;
        box.y = y + 0x44 + pos;
        box.h = box.y + len - 0x16;
        box.color = 0xFFF0EBE7;
        Prim_FillRect((struct PrimLine *)&box);
        if (g->unk1BC != 0) {
            r = gGalleryScrollRecs[0];
            r.x = x + 0xA8;
            r.y = y + 0x18 + g->unk1D4;
            if (g->unk1D4 != 0) {
                r.color = 0xFFFB7103;
            }
            r.angle = 0x4000;
            Gallery_DrawRecords(&r, 1);
        }
        if (g->unk1BC + 4 < g->unk1B0[g->unk68]) {
            r = gGalleryScrollRecs[0];
            r.x = x + 0xA8;
            r.y = y + 0xF4 + g->unk1D8;
            if (g->unk1D8 != 0) {
                r.color = 0xFFFB7103;
            }
            r.angle = 0xC000;
            Gallery_DrawRecords(&r, 1);
        }
    }
}

f32 Math_Sin(f32);
void Gallery_DrawRecords(GalRec *r, s32 n);

void Gallery_DrawCursor(GalWork *g) {
    GalRec r;
    s16 x;
    s16 y;
    x = 72.0f + g->unk130[5][0];
    y = (s32)(18.0f + (42.0f + (40.0f + g->unk130[5][1]))) + g->unk1C0 * 42;
    r = *(GalRec *)((u8 *)gGalleryRecs + 0x38);
    r.x = x;
    r.y = y;
    r.w = 224.0f * Math_Sin(6.2831855f * g->unk1C8 / 65536.0f);
    Gallery_DrawRecords(&r, 1);
}

void Gallery_DrawNewBlink(s32 unused, s16 x, s16 y, s32 blink) {
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

/* The gallery's list: four numbered entries from unk1BC, the selected one
 * yellow, each "%r" in a title drawn as a button icon. Gallery_DrawNewBlink
 * takes s16 coordinates (the extensions at the call); the strchr loop tests
 * `!p != 0` at the top (sltu/xori, not rotated); b, a copy of &buf set in
 * the for's init, is the saved register the game keeps for `p - b`. */
void Gallery_DrawList(GalWork *g) {
    char buf[0x108];
    char num[8];
    s32 i;
    s32 nx;
    s32 y;
    s32 sel;
    u32 color;
    s32 x;
    char *p;
    char *b;

    x = 72.0f + g->unk130[5][0];
    y = (s32)(40.0f + g->unk130[5][1]) + 0x2A;
    for (i = 0, b = buf; i < 4; i++) {
        sel = i == g->unk1C0;
        if (g->unk21E4[g->unk1BC + i] == 1) {
            Gallery_DrawNewBlink((s32)g, x, y, sel);
        }
        color = 0xFF000000;
        if (sel == 1) {
            color = 0xFFFFEA00;
        }
        Font_SetColor(color);
        sprintf(num, TXT(0x006D06E8) /* the entry's number (two digits), then a full-width colon */, g->unk1BC + i + 1);
        Font_SetPos(x, y);
        Font_SetSpacing(0x12, 0x12);
        Font_Printf(num);
        nx = x + Str_Length(num) * 9;
        Font_SetSpacing(0x10, 0x10);
        Font_SetPos(nx, y + 1);
        strcpy(buf, g->unk1E4[g->unk1BC + i]);
        for (;;) {
            p = strchr(buf, '%');
            if (!p != 0) {
                break;
            }
            if (p[1] == 'r') {
                Hud_DrawButtonIcon(0x19, nx + (p - b) * 8, y + 1, 0x155, color, 0.8f);
                p[0] = ' ';
                p[1] = ' ';
            }
        }
        Font_Printf(buf);
        y += 0x2A;
    }
    Gallery_DrawCursor(g);
    Gallery_DrawScrollBar(g);
}

s32 strcmp(char *, char *);                 /* strcmp */
u32 strlen(char *);
/* The full-width opening double angle bracket, which opens a centred line,
 * with its padding (16 bytes; a literal would take 8). */
INCLUDE_RODATA("asm/select/nonmatchings/select_gallery", D_006D06F0);
extern char D_006D06F0[];

/* An entry's page (while unk1CC): its picture, the frame and its four
 * parts, the four description lines (a line starting with that bracket is centred)
 * and the two help lines. x2 holds the centred line's left edge. */
void Gallery_DrawEntryPage(GalWork *g) {
    Sprite spr;
    GalRec r;
    char head[0x10];
    char buf[0x100];
    s32 i;
    s32 tex;
    s16 *p;
    GalEntry *e;
    s32 y;
    char *q;
    char **line;
    char *b;
    s32 x;
    s32 x2;
    char *l;

    p = (s16 *)gGalleryEntryPagePos;
    e = (GalEntry *)gGalleryEntries + g->unk220C[g->unk2208];
    if (Gallery_IsUnlocked(g->unk220C[g->unk2208]) == 1) {
        tex = e->unk8 + 0x2B;
    } else {
        tex = 0x61;
    }
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x63, 0);
    Tex_Use(1, (u32 *)&gTexHandles[tex]);
    Gs_SetState(4, gTexHandles[tex]);
    spr.x = p[1];
    spr.y = *(p += 2);
    spr.w = p[1];
    spr.h = *(p += 2);
    spr.u1 = 0xFF;
    spr.v1 = 0xFF;
    spr.color = -1;
    spr.u0 = 0;
    spr.v0 = 0;
    Sprite_Draw(&spr);
    p += 2;
    for (i = 0; i < 4; i++) {
        r = gGalleryScrollRecs[p[0]];
        r.x = p[1];
        r.y = *(p += 2);
        r.w = p[1];
        r.h = *(p += 2);
        r.angle = p[1];
        p += 2;
        Gallery_DrawRecords(&r, 1);
    }
    if (Gallery_IsUnlocked(g->unk220C[g->unk2208]) != 0) {
        Font_SetColor(0xFF65645A);
        Font_SetSpacing(0x10, 0x10);
        line = (char **)e->unk4;
        b = buf;
        y = 0x132;
        for (i = 0; i < 4; i++) {
            l = *line;
            head[0] = l[0];
            head[1] = l[1];
            head[2] = 0;
            if (strcmp(head, D_006D06F0) == 0) {
                x = (0x24 - Str_Length(*line)) * 8 / 2;
                Font_SetPos(x + 0x134, y);
                strcpy(buf, *line);
                x2 = x + 0x134;
                for (;;) {
                    q = strchr(buf, '%');
                    if (!q != 0) {
                        break;
                    }
                    if (q[1] == 'r') {
                        Hud_DrawButtonIcon(0x19, x2 + (q - b) * 8, y, 0x155, 0xFFCAC8B4, 0.8f);
                        q[0] = ' ';
                        q[1] = ' ';
                    }
                }
                Font_Printf(buf);
            } else if (strlen(*line) != 0) {
                Font_SetPos(0x134, y);
                strcpy(buf, *line);
                for (;;) {
                    q = strchr(buf, '%');
                    if (!q != 0) {
                        break;
                    }
                    if (q[1] == 'r') {
                        Hud_DrawButtonIcon(0x19, (q - b) * 8 + 0x134, y, 0x155, 0xFFCAC8B4, 0.8f);
                        q[0] = ' ';
                        q[1] = ' ';
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
 * column of unk1AC. */
void Gallery_DrawPicture(s32 *ids, f32 *pos) {
    Sprite spr;
    GalRec r;
    GalWork *g;
    s32 i;
    s32 k;
    GalRec *rp;

    g = GAL;
    for (i = 0; i < 2; i++, ids++) {
        if (*ids == 0xFF) {
            continue;
        }
        *(rp = &r) = *(GalRec *)((u8 *)gGalleryLayout + *ids * sizeof(GalRec));
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
            spr.x = rp->x + (s16)pos[0];
            spr.y = rp->y + (s16)pos[1];
        } else {
            spr.x = (s16)g->unk17C[k][0] + (s16)pos[0];
            spr.y = (s16)g->unk17C[k][1] + (s16)pos[1];
        }
        spr.w = rp->w;
        spr.h = rp->h;
        spr.u0 = rp->u0;
        spr.v0 = rp->v0;
        spr.u1 = rp->u1;
        spr.v1 = rp->v1;
        if (*ids == 8) {
            spr.u0 = g->unk1AC << 8;
            spr.u1 = spr.u0 + 0xFF;
        }
        spr.color = rp->color;
        if (spr.x < 0x280 && spr.x + spr.w >= 0 && spr.y < 0x1C0 && spr.y + spr.h >= 0) {
            Gs_SetState(0x5E, 0x32);
            Gs_SetState(0x63, 0);
            Tex_Use(1, (u32 *)&g->unkC[rp->tex]);
            Gs_SetState(4, g->unkC[rp->tex]);
            if (rp->angle != 0) {
                TexRect_DrawRotated((struct TexRect *)&spr, rp->angle);
            } else {
                Sprite_Draw(&spr);
            }
        }
    }
}

void Gallery_DrawRecords(GalRec *r, s32 n) {
    GalWork *g;
    Sprite spr;
    s32 i;

    g = GAL;
    for (i = 0; i < n; i++, r++) {
        spr.x = r->x;
        spr.y = r->y;
        spr.w = r->w;
        spr.h = r->h;
        spr.u0 = r->u0;
        spr.v0 = r->v0;
        spr.u1 = r->u1;
        spr.v1 = r->v1;
        spr.color = r->color;
        if (spr.x < 0x280 && spr.x + spr.w >= 0 && spr.y < 0x1C0 && spr.y + spr.h >= 0) {
            Gs_SetState(0x5E, 0x32);
            Gs_SetState(0x63, 0);
            if (r->tex > 100) {
                Tex_Use(1, (u32 *)&gTexHandles[r->tex]);
                Gs_SetState(4, gTexHandles[r->tex]);
            } else {
                Tex_Use(1, (u32 *)&g->unkC[r->tex]);
                Gs_SetState(4, g->unkC[r->tex]);
            }
            if (r->angle != 0) {
                TexRect_DrawRotated((struct TexRect *)&spr, r->angle);
            } else {
                Sprite_Draw(&spr);
            }
        }
    }
}

/* The pictures sliding to their places, and the panel sliding in (1) or
 * out (2). */
void Gallery_UpdatePictures(void) {
    GalWork *g;
    s32 i;

    g = GAL;
    for (i = 0; i < 6; i++) {
        if (g->unk70[i] >= 2) {
            if (g->unk70[i] == 2) {
                if (g->unk50[i] > 0) {
                    g->unk50[i]--;
                    continue;
                }
                g->unk70[i] = 3;
            }
            if (g->unk70[i] == 3) {
                g->unk130[i][0] += g->unk100[i][0] * Math_Sin(6.2831855f * g->unk160[i] / 65536.0f);
                if (g->unk100[i][0] < 0.0f) {
                    if (!(g->unkD0[i][0] < g->unk130[i][0])) {
                        g->unk130[i][0] = g->unkD0[i][0];
                        g->unk70[i] = 1;
                    }
                } else if (g->unkD0[i][0] <= g->unk130[i][0]) {
                    g->unk130[i][0] = g->unkD0[i][0];
                    g->unk70[i] = 1;
                }
                g->unk160[i] += 0x800;
                if (g->unk160[i] >= 0x4000) {
                    g->unk160[i] = 0x4000;
                }
            }
        }
    }
    if (g->unk1A2 == 1) {
        g->unk1A4 = g->unk1A4 - (s32)(40.0f * Math_Sin(6.2831855f * g->unk1A6 / 65536.0f));
        g->unk1A6 += 0x800;
        if (g->unk1A6 >= 0x4000) {
            g->unk1A6 = 0x4000;
        }
    }
    if (g->unk1A2 == 2) {
        g->unk1A4 = g->unk1A4 + (s32)(40.0f * Math_Sin(6.2831855f * g->unk1A6 / 65536.0f));
        g->unk1A6 += 0x800;
        if (g->unk1A6 >= 0x4000) {
            g->unk1A6 = 0x4000;
        }
    }
}

void Gallery_SetLayout(s32 k) {
    GalWork *g;
    s32 *st;
    f32 *sp;
    s16 (*pos)[2];
    s32 i;

    g = GAL;
    pos = gGalleryLayoutPos[k];
    st = gGalleryLayoutSteps;
    sp = gGalleryLayoutSpeeds;
    for (i = 0; i < 6; i++) {
        if (*st == -1) {
        skip:
            pos++;
            st++;
            sp++;
        } else if ((f32)(*pos)[0] == g->unk130[i][0]) {
            goto skip;
        } else {
            g->unkD0[i][0] = (*pos)[0];
            g->unkD0[i][1] = (*pos)[1];
            pos++;
            g->unk50[i] = *st++;
            g->unk100[i][0] = *sp++;
            g->unk100[i][1] = 0.0f;
            g->unk160[i] = 0;
            g->unk70[i] = 2;
        }
    }
    g->unk4C = 0;
}

void Gallery_SetLayoutBack(s32 unused) {
    GalWork *g;
    s32 *st;
    f32 *sp;
    s16 (*pos)[2];
    s32 i;

    g = GAL;
    pos = gGalleryLayoutPos[0];
    st = gGalleryLayoutBackSteps;
    sp = gGalleryLayoutSpeeds;
    for (i = 0; i < 6; i++) {
        if (*st == -1) {
        skip:
            pos++;
            st++;
            sp++;
        } else if ((f32)(*pos)[0] == g->unk130[i][0]) {
            goto skip;
        } else {
            g->unkD0[i][0] = (*pos)[0];
            g->unkD0[i][1] = (*pos)[1];
            pos++;
            g->unk50[i] = *st++;
            g->unk100[i][0] = *sp++;
            g->unk100[i][0] = g->unk100[i][0] * -1.0f;
            g->unk100[i][1] = 0.0f;
            g->unk160[i] = 0;
            g->unk70[i] = 2;
        }
    }
    g->unk4C = 0;
    g->unk1A8 = 0;
    g->unk1AC = 0;
}

s32 Gallery_PictureMoving(s32 i) {
    GalWork *g = (GalWork *)((s32)gSysSettings.workArea + 0x1000); /* GAL changes the code here */

    return g->unk70[i] >= 2;
}

s32 Gallery_Settled(void) {
    GalBase *base;
    GalWork *g;
    s32 i;

    base = (GalBase *)gSysSettings.workArea;
    base->gal.unk4C++;
    g = &base->gal;
    if (base->gal.unk0 == 1) {
        if (g->unk4C == 0xA) {
            Gallery_ApplyLayout(g, g->unk68 + 1);
        }
    } else if (g->unk4C == 8) {
        Gallery_ApplyLayout(g, 0);
    }
    for (i = 0; i < 6; i++) {
        if (g->unk70[i] >= 2) {
            return 0;
        }
    }
    return 1;
}

void Gallery_FillNames(GalWork *g) {
    s32 i;
    s32 idx;

    g->unk2208 = Gallery_EntryIndex(g->unk68, 0);
    for (i = 0; i < 32; i++) {
        g->unk1E4[i][0] = 0;
    }
    for (i = 0; i < g->unk1B0[g->unk68]; i++) {
        idx = Gallery_EntryIndex(g->unk68, i);
        if (Gallery_IsUnlocked(g->unk220C[idx]) == 0) {
            strcpy(g->unk1E4[i], TXT(0x006D0700) /* a row of full-width question marks (an entry not unlocked yet) */);
        } else {
            strcpy(g->unk1E4[i], ((GalEntry *)gGalleryEntries)[g->unk220C[idx]].title);
        }
        if (Gallery_IsNew(g->unk220C[idx]) == 1) {
            g->unk21E4[i] = 1;
        } else {
            g->unk21E4[i] = 0;
        }
    }
    g->unk1BC = 0;
    g->unk1C0 = 0;
    g->unk1C4 = 1;
}

s32 Gallery_ListInput(GalWork *g, u16 pad) {
    u16 b;

    g->unk1C8 += 0x1000;
    if (g->unk1C8 >= 0x4000) {
        g->unk1C8 = 0x4000;
    }
    if (g->unk1DC != 0) {
        if (g->unk1DC == 1) {
            g->unk1D4--;
            if (g->unk1D4 < -4) {
                g->unk1DC = 2;
            }
        }
        if (g->unk1DC == 2) {
            g->unk1D4++;
            if (g->unk1D4 >= 0) {
                g->unk1DC = 0;
            }
        }
    }
    if (g->unk1E0 != 0) {
        if (g->unk1E0 == 1) {
            g->unk1D8++;
            if (g->unk1D8 >= 5) {
                g->unk1E0 = 2;
            }
        }
        if (g->unk1E0 == 2) {
            g->unk1D8--;
            if (g->unk1D8 <= 0) {
                g->unk1E0 = 0;
            }
        }
    }
    b = pad;
    if (b & 0x2000) {
        if (g->unk1C0 == 0) {
            if (g->unk1BC > 0) {
                g->unk1BC--;
                goto up;
            }
        } else {
            g->unk1C0--;
            goto up;
        }
    }
    if (b & 0x1000) {
        if (g->unk1C0 >= 3) {
            if (g->unk1BC + 4 < g->unk1B0[g->unk68]) {
                g->unk1BC++;
                goto down;
            }
        } else {
            g->unk1C0++;
            goto down;
        }
    }
    return 0;
up:
    g->unk2208 = Gallery_EntryIndex(g->unk68, g->unk1BC + g->unk1C0);
    g->unk1C8 = 0;
    Gallery_PlayPageSe();
    g->unk1DC = 1;
    return 1;
down:
    g->unk2208 = Gallery_EntryIndex(g->unk68, g->unk1BC + g->unk1C0);
    g->unk1C8 = 0;
    Gallery_PlayPageSe();
    g->unk1E0 = 1;
    return 1;
}

void Gallery_PlayDecideSe(void) {
    Sound_PlaySeFlat(0, 0x13, 0);
}

void Gallery_PlayCancelSe(void) {
    Sound_PlaySeFlat(0, 0x14, 0);
}

void Gallery_PlayPageSe(void) {
    Sound_PlaySeFlat(0, 0x17, 0);
}

void Gallery_DrawArrow(s32 k) {
    GalWork *g;
    Sprite spr;
    s16 (*pos)[2];
    GalIcon *ic;

    g = GAL;
    Gs_SetState(0x5E, 0x32);
    Gs_SetState(0x63, 0);
    Tex_Use(1, (u32 *)&gTexHandles[gGalleryArrowSprites[0].tex]);
    Gs_SetState(4, gTexHandles[gGalleryArrowSprites[0].tex]);
    if (g->unk0 >= 2 && g->unk0 < 5) {
        pos = &gGalleryArrowPosB[k];
    } else {
        pos = &gGalleryArrowPosA[k];
    }
    ic = &gGalleryArrowSprites[g->unk1A1];
    spr.w = ic->w;
    spr.h = ic->h;
    spr.color = 0xFFFF6000;
    spr.x = (*pos)[0];
    spr.y = (*pos)[1];
    if (g->unk1A2 != 0) {
        spr.y += g->unk1A4;
        if (g->unk1A2 == 2 && spr.y >= ((s16 *)(gGalleryArrowPosA + k))[1]) {
            spr.y = ((s16 *)(gGalleryArrowPosA + k))[1];
        }
    }
    spr.u0 = ic->u0;
    spr.v0 = ic->v0;
    spr.u1 = ic->u1;
    spr.v1 = ic->v1;
    Sprite_Draw(&spr);
}

void Gallery_StartPanel(GalWork *g, s32 back) {
    g->unk1A2 = (s8) (back + 1);
    if (back == 0) {
        g->unk1A4 = 0;
    } else {
        g->unk1A4 = -0x104;
    }
    g->unk1A6 = 0;
    g->unk1A1 = 3;
}

void Gallery_ResetPanel(GalWork *g) {
    g->unk1A0 = 0;
    g->unk1A1 = 0;
    g->unk1A2 = 0;
    g->unk1A6 = 0;
}
