/*
 * select: the gallery (with its .data and .rodata),
 * started from gm_sub's main menu.
 *
 * Gallery_Task runs Gallery_Update over GalWork (select.h), at 0x1000 in
 * the shared work area. The menu offers three categories (monsters, the
 * world's areas, bonus movies); six pictures move between layouts as the
 * menu opens a category's list. An unlocked entry (gGalleryEntries, its
 * event flag) opens its page: a monster's quest or an area's tour (plsel's
 * Galex tasks, after loading OVERLAY_PLSEL), a movie, or the ending. When
 * one of those ends, gSysSettings' gallery fields bring the player back to
 * the same entry.
 */
#include "common.h"
#include "card.h"
#include "window.h"
#include "sif.h"
#include "stageobj.h"
#include "stream.h"
#include "screen_fade.h"
extern s32 gVSyncCount; /* main: +1 every VBlank */
extern s32 gLoadLastVSync; /* sub_main: gVSyncCount when the loader last ran a frame */
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "pad.h"
#include "render_state.h"
#include "select.h"
#include "syssettings.h"
#include "save.h"
#include "movie.h"
#include "select/select_gallery.h"   /* the texts this file uses (tools/content_gen.py) */
#include "texture.h"
#include "plsel.h"
#include "prim.h"
#include "sound.h"
#include "text.h"
#include "display.h"
#include "engine_math.h"
#include "font.h"
#include "frame.h"
#include "game_util.h"
#include "main.h"
#include "mem_util.h"
#include "packet_tim2.h"
#include "sprite_queue.h"

/* The game compiled this file at optimization level 4 (the build's -O3,p
 * plus MWCC's level-4 passes): Gallery_EntryInList matches only there, and every
 * other function compiles the same at both levels. */
#pragma optimization_level 4



/* The gallery's work in the shared work area. */
#define GAL (&((SelWork *)gSysSettings.workArea)->gallery)

extern s32 gGallerySeen[];      /* sub_main: flags of what the gallery shows */
extern s32 gGalleryTex[];

static void Gallery_ApplyLayout(GalWork *g, s32 k);
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha); /* sub_main: w is a struct ListWin * */
char *strcpy(char *, char *);
extern s32 gTexHandles[];
struct PrimLine;

/* .data */
char *gGalleryCategoryNames[3] = {
#include "select/006CB758.inc"
};
char *gGalleryPlayHelp[2] = {
#include "select/006CB768.inc"
};
char *gGalleryNewText[2] = {
#include "select/006CB770.inc"
};
char *gGalleryTimeTexts[2] = {
#include "select/006CB778.inc"
};
char *gGalleryEntries_1[4] = {
#include "select/006CB780.inc"
};
char *gGalleryEntries_4[4] = {
#include "select/006CB790.inc"
};
char *gGalleryEntries_7[4] = {
#include "select/006CB7A0.inc"
};
char *gGalleryEntries_10[4] = {
#include "select/006CB7B0.inc"
};
char *gGalleryEntries_13[4] = {
#include "select/006CB7C0.inc"
};
char *gGalleryEntries_16[4] = {
#include "select/006CB7D0.inc"
};
char *gGalleryEntries_19[4] = {
#include "select/006CB7E0.inc"
};
char *gGalleryEntries_22[4] = {
#include "select/006CB7F0.inc"
};
char *gGalleryEntries_25[4] = {
#include "select/006CB800.inc"
};
char *gGalleryEntries_28[4] = {
#include "select/006CB810.inc"
};
char *gGalleryEntries_31[4] = {
#include "select/006CB820.inc"
};
char *gGalleryEntries_34[4] = {
#include "select/006CB830.inc"
};
char *gGalleryEntries_37[4] = {
#include "select/006CB840.inc"
};
char *gGalleryEntries_40[4] = {
#include "select/006CB850.inc"
};
char *gGalleryEntries_43[4] = {
#include "select/006CB860.inc"
};
char *gGalleryEntries_46[4] = {
#include "select/006CB870.inc"
};
char *gGalleryEntries_49[4] = {
#include "select/006CB880.inc"
};
char *gGalleryEntries_52[4] = {
#include "select/006CB890.inc"
};
char *gGalleryEntries_55[4] = {
#include "select/006CB8A0.inc"
};
char *gGalleryEntries_58[4] = {
#include "select/006CB8B0.inc"
};
char *gGalleryEntries_61[4] = {
#include "select/006CB8C0.inc"
};
char *gGalleryEntries_64[4] = {
#include "select/006CB8D0.inc"
};
char *gGalleryEntries_67[4] = {
#include "select/006CB8E0.inc"
};
char *gGalleryEntries_70[4] = {
#include "select/006CB8F0.inc"
};
char *gGalleryEntries_73[4] = {
#include "select/006CB900.inc"
};
char *gGalleryEntries_76[4] = {
#include "select/006CB910.inc"
};
char *gGalleryEntries_79[4] = {
#include "select/006CB920.inc"
};
char *gGalleryEntries_82[4] = {
#include "select/006CB930.inc"
};
char *gGalleryEntries_85[4] = {
#include "select/006CB940.inc"
};
char *gGalleryEntries_88[4] = {
#include "select/006CB950.inc"
};
char *gGalleryEntries_91[4] = {
#include "select/006CB960.inc"
};
char *gGalleryEntries_94[4] = {
#include "select/006CB970.inc"
};
char *gGalleryEntries_97[4] = {
#include "select/006CB980.inc"
};
char *gGalleryEntries_100[4] = {
#include "select/006CB990.inc"
};
char *gGalleryEntries_103[4] = {
#include "select/006CB9A0.inc"
};
char *gGalleryEntries_106[4] = {
#include "select/006CB9B0.inc"
};
char *gGalleryEntries_109[4] = {
#include "select/006CB9C0.inc"
};
char *gGalleryEntries_112[4] = {
#include "select/006CB9D0.inc"
};
char *gGalleryEntries_115[4] = {
#include "select/006CB9E0.inc"
};
char *gGalleryEntries_118[4] = {
#include "select/006CB9F0.inc"
};
char *gGalleryEntries_121[4] = {
#include "select/006CBA00.inc"
};
char *gGalleryEntries_124[4] = {
#include "select/006CBA10.inc"
};
char *gGalleryEntries_127[4] = {
#include "select/006CBA20.inc"
};
char *gGalleryEntries_130[4] = {
#include "select/006CBA30.inc"
};
char *gGalleryEntries_133[4] = {
#include "select/006CBA40.inc"
};
char *gGalleryEntries_136[4] = {
#include "select/006CBA50.inc"
};
char *gGalleryEntries_139[4] = {
#include "select/006CBA60.inc"
};
char *gGalleryEntries_142[4] = {
#include "select/006CBA70.inc"
};
char *gGalleryEntries_145[4] = {
#include "select/006CBA80.inc"
};
char *gGalleryEntries_148[4] = {
#include "select/006CBA90.inc"
};
char *gGalleryEntries_151[4] = {
#include "select/006CBAA0.inc"
};
char *gGalleryEntries_154[4] = {
#include "select/006CBAB0.inc"
};
char *gGalleryEntries_160[4] = {
#include "select/006CBAC0.inc"
};
char *gGalleryEntries_157[4] = {
#include "select/006CBAD0.inc"
};
/* The gallery's entries, by number (an entry's event flag is its number). */
GalEntry gGalleryEntries[GAL_ENTRY_COUNT] = {
#include "select/006CBAE0.inc"
};
/* The movies of the movie entries, by GalEntry.kindIndex (0xFFFF: none).
 * 16-aligned: the 8 zero bytes before it are padding. */
u16 gGalleryMovieIds[16] __attribute__((aligned(16))) = {
#include "select/006CBD70.inc"
};
/* The menu's records, by id: the pictures (GalWork.pictures) and the
 * category names' places (2, 4 and 6). */
GalRec gGalleryLayout[10] = {
#include "select/006CBD90.inc"
};
/* The list: frame, cursor, scroll bar (Gallery_DrawScrollBar, Gallery_DrawCursor).
 * 16-aligned: the 8 zero bytes before it are padding. */
GalRec gGalleryRecs[4] __attribute__((aligned(16))) = {
#include "select/006CBEB0.inc"
};
/* The scroll bar's lower end (Gallery_DrawScrollBar). */
GalRec gGalleryScrollBar = {
#include "select/006CBF20.inc"
};
/* The scroll arrow (Gallery_DrawScrollBar) and a second record
 * (Gallery_DrawEntryPage indexes the table). As a GalRec array MWCC knows [0] is
 * 16-aligned and copies it with lq/sq, as the game does (the 4 zero
 * bytes before it are padding). */
GalRec gGalleryScrollRecs[2] __attribute__((aligned(16))) = {
#include "select/006CBF40.inc"
};
/* 16-aligned: the 8 zero bytes before it are padding. */
s16 gGalleryEntryPagePos[32] __attribute__((aligned(16))) = {
#include "select/006CBF80.inc"
};
/* The six pictures' two layout ids each at the start (0xFF: none). */
u8 gGalleryStartPictures[16] = {
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
s16 gGalleryCategoryOfs[3][2] = {
#include "select/006CC0B8.inc"
};
u8 gGalleryCategoryFirst[8] = {
#include "select/006CC0C8.inc"
};
u8 gGalleryCategoryEnd[16] = {
#include "select/006CC0D0.inc"
};
/* The entries that ask day or night, as bytes ending in 255. Declared as
 * words: as a u8 array, Gallery_EntryInList's scan compiles differently. */
u32 gGalleryTimedEntries[12] = {
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
 * Marks entry n as seen (a bit of sub_main's gGallerySeen), which ends its new mark.
 */
void Gallery_SetSeen(s32 n)
{
    gGallerySeen[n / 32] |= 1 << (n % 32);
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
        w = gGallerySeen[n / 32];
        if (!(w & (1 << (n % 32)))) {
            return 1;
        }
    }
    return 0;
}

void Gallery_PlayCursorSe(void)
{
    Sound_PlaySeFlat(1, 12, 0);
}

void Gallery_PlayOpenSe(void)
{
    Sound_PlaySeFlat(1, 11, 0);
}

void Gallery_PlayCloseSe(void)
{
    Sound_PlaySeFlat(1, 11, 0);
}

void Gallery_PlayErrorSe(void)
{
    Sound_PlaySeFlat(0, 21, 0);
}

struct SndFile;
s32 Sound_LoadBankWait(struct SndFile *f, u32 slot); /* main: f is a SndFile * */
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

    Afs_LoadPartition(5, 6644, gSysSettings.readBuf, "galex");
    for (i = 0, handle = gGalleryTex; i < GAL_TEX_COUNT; i++, handle++) {
        cur = gVSyncCount;
        prev = gLoadLastVSync;
        if (prev != cur) {
            Load_AdvanceFrames(cur - prev, 0);
            gLoadLastVSync = gVSyncCount;
            Sys_FlipFrame(0);
        }
        Afs_ReadFileIdle(i, 5, gSysSettings.loadBuf);
        *handle = Tex_CreateFromTim2(gSysSettings.loadBuf, TEX_MODE_ON_DEMAND);
    }
    Mem_Compact();
}

void Gallery_FreeTextures(void)
{
    Tex_FreeRange(GAL_TEX_FIRST, GAL_TEX_COUNT);
    Mem_Compact();
}

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
    memset(gal, 0, 8780);
    memset(&gSave, 0, 14328);
    Sel_LoadCommonTextures();
    for (j = 163, i = 0; i < 16; i++, j++) {
        gal->texHandles[i] = gTexHandles[j];
    }
    Gallery_LoadTextures();
    for (i = 0, initBytes = gGalleryStartPictures; i < GAL_PICTURE_COUNT; i++) {
        gal->pictures[i][0] = *initBytes++;
        gal->pictures[i][1] = *initBytes++;
    }
    for (i = 0, layoutPos = (s16 *)gGalleryLayoutPos[0]; i < GAL_PICTURE_COUNT; i++) {
        gal->pos[i][0] = *layoutPos++;
        gal->pos[i][1] = *layoutPos++;
        gal->moveState[i] = 1;
        gal->drawOrder[i] = i;
        gal->pictureCount++;
    }
    for (rec = gGalleryLayout, i = 0, nameWalk = gal; i < 10; i++, rec++) {
        if (i == 2 || i == 4 || i == 6) {
            nameWalk->categoryPos[0][0] = rec->x;
            nameWalk->categoryPos[0][1] = rec->y;
            /* a GalWork pointer stepped one position (the match keeps the
             * work's base in the register, categoryPos's offset in the stores) */
            nameWalk = (GalWork *)((u8 *)nameWalk + sizeof(gal->categoryPos[0]));
        }
    }
    EventFlag_Set(48);
    gal->categoryCount = GAL_CATEGORY_COUNT;
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
    GalRec *layout = gGalleryLayout;

    switch (k) {
    case 0:
        gal->categoryPos[0][1] = layout[2].y;
        gal->categoryPos[1][1] = layout[4].y;
        gal->categoryPos[2][1] = layout[6].y;
        break;
    case 1:
        gal->categoryPos[2][1] = layout[6].y;
        break;
    case 2:
        gal->categoryPos[1][1] = layout[6].y;
        break;
    case 3:
        gal->categoryPos[0][1] = layout[6].y;
        break;
    }
}

/*
 * Whether the chosen entry is a quest or an area (kinds 0-2) listed in
 * gGalleryTimedEntries: those ask day or night before they start. The scan
 * matches only at optimization level 4 (the file's pragma), and the entry
 * is read through a GalEntry cast (see Gallery_Update).
 */
s32 Gallery_EntryInList(GalWork *gal)
{
    s32 idx;
    u8 *listed;
    u8 entryId;

    idx = gal->selected;
    switch (((GalEntry *)gGalleryEntries)[gal->entries[idx]].kind) {
    case GAL_KIND_MONSTER_QUEST:
    case GAL_KIND_WORLD_QUEST:
    case GAL_KIND_AREA_TOUR:
        entryId = *(u8 *)gGalleryTimedEntries;
        listed = (u8 *)gGalleryTimedEntries;
        if (entryId != 255) {
            do {
                if (entryId == gal->entries[idx]) {
                    return 1;
                }
                entryId = *++listed;
            } while (entryId != 255);
        }
        break;
    }
    return 0;
}

extern char gGalleryTitle[];     /* the title of what is shown */
/* unit's own declaration: declared s32 as Gallery_Update uses it; with the definition's u8 return, the call compiles differently */
s32 NetMenu_Update(void);

/*
 * The gallery's frame, by GalState: the fade in (0), the category menu (1),
 * the three categories' lists (2-4), the ending (5), a movie (6), the card
 * menu (7) and the fade out (8; 9 done). A quest or a tour ends the task
 * here, after setting gSysSettings' gallery fields and starting plsel's
 * task. Returns 1 when the gallery is done. Back from one of those, the
 * menu opens the same category's list at once: it jumps into the confirm
 * button's code (the goto, as the game's code does).
 *
 * gGalleryEntries is read through a GalEntry cast where the game's code
 * adds the field's offset at run time: indexed directly, MWCC folds the
 * offset into the address.
 */
s32 Gallery_Update(TaskSlot *task)
{
    GalWork *gal;
    GalEntry *entry;
    u16 pressedNow;
    u16 pressRepeat;
    s32 i;

    pressedNow = gPadState[0].pressed;
    pressRepeat = pressedNow | gPadState[0].repeat;
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
                gal->fadeFrames = 20;
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
                gal->fadeFrames = 20;
                FadeA_Start(2);
                gal->category = gSysSettings.gallerySel;
                goto open;
            }
            if (pressRepeat & PAD_UP) {
                gal->category--;
                if (gal->category < 0) {
                    gal->category += gal->categoryCount;
                }
                Gallery_PlayCursorSe();
            } else if (pressRepeat & PAD_DOWN) {
                gal->category++;
                if (gal->category >= gal->categoryCount) {
                    gal->category -= gal->categoryCount;
                }
                Gallery_PlayCursorSe();
            } else if (pressedNow & PAD_CIRCLE) {
                Gallery_PlayOpenSe();
            open:
                Gallery_FillNames(gal);
                Gallery_SetLayout(gal->category + 1);
                gal->step++;
                Gallery_StartPanel(gal, 0);
            } else if (pressedNow & PAD_CROSS) {
                NetMenu_Open(NETMENU_MODE_SAVE_SYSTEM);
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
                    gal->fadeFrames = 20;
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
            if (Gallery_ListInput(gal, pressRepeat) == 1) {
                break;
            }
            if ((u8)FadeA_GetState() == 1) {
                break;
            }
            if (pressedNow & PAD_CROSS) {
                Gallery_PlayCloseSe();
                gal->pageShown = 0;
                Gallery_SetLayoutBack(0);
                Gallery_StartPanel(gal, 1);
                gal->step++;
            } else if (pressedNow & PAD_CIRCLE) {
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
            entry = &gGalleryEntries[gal->entries[gal->selected]];
            switch (entry->kind) {
            case GAL_KIND_ENDING:
                gSysSettings.galleryPlay = 1;
                gSysSettings.gallerySel = gal->state - GAL_LIST_MONSTERS;
                gSysSettings.galleryEntry = gal->entries[gal->selected];
                gSysSettings.galleryKeep[0] = gal->listTop;
                gSysSettings.galleryKeep[1] = gal->listRow;
                gal->state = GAL_ENDING;
                gal->step = 0;
                strcpy(gGalleryTitle, gal->names[gal->listTop + gal->listRow]);
                break;
            case GAL_KIND_MOVIE:
                gSysSettings.gallerySel = gal->state - GAL_LIST_MONSTERS;
                gSysSettings.galleryEntry = gal->entries[gal->selected];
                gSysSettings.galleryKeep[0] = gal->listTop;
                gSysSettings.galleryKeep[1] = gal->listRow;
                if (gGalleryMovieIds[entry->kindIndex] != 0xFFFF) {
                    gal->state = GAL_MOVIE;
                    gal->step = 0;
                }
                break;
            case GAL_KIND_MONSTER_QUEST:
            case GAL_KIND_WORLD_QUEST:
                Game_ResetSubsystems();
                FadeA_Reset();
                gSysSettings.gallerySel = gal->state - GAL_LIST_MONSTERS;
                gSysSettings.galleryEntry = gal->entries[gal->selected];
                if (gal->night != 0) {
                    gSysSettings.galleryEntry = (gSysSettings.galleryEntry & GAL_ENTRY_MASK) | GAL_ENTRY_NIGHT;
                }
                gSysSettings.galleryKeep[0] = gal->listTop;
                gSysSettings.galleryKeep[1] = gal->listRow;
                gSysSettings.galleryPlay = 1;
                strcpy(gGalleryTitle, gal->names[gal->listTop + gal->listRow]);
                Loading_SetMessage(5);
                Scene_LoadOverlay(11);
                TaskSlot_Set(Galex_QuestTask, 6);
                TaskSlot_End(task);
                return 0;
            case GAL_KIND_AREA_TOUR:
                Game_ResetSubsystems();
                FadeA_Reset();
                gSysSettings.gallerySel = gal->state - GAL_LIST_MONSTERS;
                gSysSettings.galleryEntry = gal->entries[gal->selected];
                if (gal->night != 0) {
                    gSysSettings.galleryEntry = (gSysSettings.galleryEntry & GAL_ENTRY_MASK) | GAL_ENTRY_NIGHT;
                }
                gSysSettings.galleryKeep[0] = gal->listTop;
                gSysSettings.galleryKeep[1] = gal->listRow;
                gSysSettings.galleryPlay = 1;
                strcpy(gGalleryTitle, gal->names[gal->listTop + gal->listRow]);
                Loading_SetMessage(5);
                Scene_LoadOverlay(11);
                TaskSlot_Set(Galex_Task, 6);
                TaskSlot_End(task);
                return 0;
            }
            break;
        case 3:
            if ((pressedNow & PAD_RIGHT) || (pressedNow & PAD_LEFT)) {
                Gallery_PlayPageSe();
                gal->night ^= 1;
            } else if (pressedNow & PAD_CROSS) {
                gal->askTime = 0;
                Gallery_PlayCancelSe();
                gal->step = 0;
            } else if (pressedNow & PAD_CIRCLE) {
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
            Scene_LoadOverlay(11);
            Sel_FreeTextures();
            Gallery_FreeTextures();
            Ending_Init();
            gal->step++;
            return 0;
        case 1:
            if (Ending_Update() == 0) {
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
                gal->fadeFrames = 20;
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
            Scene_LoadOverlay(11);
            Sel_FreeTextures();
            Gallery_FreeTextures();
            Movie_PrePlay();
            Movie_Play(gGalleryMovieIds[((GalEntry *)gGalleryEntries)[gal->entries[gal->selected]].kindIndex]);
            gal->step++;
        case 1:
            Movie_Update();
            if (Movie_GetStatus() == MOVIE_STATUS_FINISHED || (gPadState[0].pressed & PAD_CROSS)) {
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
    for (i = 0; i < GAL_CATEGORY_COUNT; i++) {
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
 * need no unlock or are unlocked, then 255 for each locked one. Returns
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
    entry = gGalleryEntries + start;
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
        gal->entries[gal->entryCount++] = 255;
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
    for (j = 163, i = 0; i < 16; i++, j++) {
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
    s32 savedZbuf;
    s32 savedTexFilter;
    s32 savedAlpha;
    s32 k;

    gal = GAL;
    savedZbuf = Gs_GetState(RS_ZWRITE);
    savedTexFilter = Gs_GetState(RS_TEX_FILTER);
    savedAlpha = Gs_GetState(RS_BLEND);
    Gs_SetState(RS_ZWRITE, 0);
    for (i = 0; i < gal->pictureCount; i++) {
        k = gal->drawOrder[i];
        if (gal->pictures[k][0] != 255) {
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
    Gs_SetState(RS_ZWRITE, savedZbuf);
    Gs_SetState(RS_TEX_FILTER, savedTexFilter);
    Gs_SetState(RS_BLEND, savedAlpha);
    if (gal->askTime == 1) {
        ListPanel_DrawAlpha(&gGalleryListPanel, 0, -1, 128);
        Text_SetColor(0);
        Font_SetSpacing(18, 18);
        Font_SetPos(257, 215);
        Text_Printf(gGalleryTimeTexts[gal->night]);
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
        mode = Gs_GetState(RS_ZWRITE);
        Gs_SetState(RS_ZWRITE, 0);
        Prim_FillRect((struct PrimLine *)&rec);
        Gs_SetState(RS_ZWRITE, mode);
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
        size = 18;
        color = 0x807F7F28;
        y -= 20;
        x -= 10;
    } else {
        size = 14;
        color = 0x80605B4B;
        y -= 12;
        x -= 20;
    }
    fontSize = size;
    Font_SetSpacing(fontSize, fontSize);
    if (small == 0) {
        Font_SetPos(x + 2, y + 2);
        Font_SetColor(0x80000000);
        Font_Printf(gGalleryNewText[0]);
    }
    Font_SetPos(x, y);
    Font_SetColor(color);
    Font_Printf(gGalleryNewText[0]);
}

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

    Font_SetSpacing(24, 24);
    for (i = 0; i < gal->categoryCount; i++) {
        j = 2 - i;
        if (Gallery_PictureMoving(j + 1) != 0 && gal->category != i) {
            continue;
        }
        x = gal->categoryPos[j][0] + gGalleryCategoryOfs[i][0];
        y = gal->categoryPos[j][1] + gGalleryCategoryOfs[i][1];
        x += (s32)gal->pos[j + 1][0];
        y += (s32)gal->pos[j + 1][1];
        if (i == gal->category) {
            size = 24;
            color = 0x807F7F7F;
        } else {
            color = 0x80101010;
            size = 20;
        }
        y -= size / 2;
        fontSize = size;
        Font_SetSpacing(fontSize, fontSize);
        Font_SetPos(x, y);
        Font_SetColor(color);
        Font_Printf(gGalleryCategoryNames[i]);
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
    rec.x = x + 40;
    rec.y = y + 20;
    Gallery_DrawRecords(&rec, 1);
    n = gal->entryCounts[gal->category];
    if (n > GAL_LIST_ROWS) {
        step = 184.0f / n;
        len = 4.0f * step;
        pos = (f32)gal->listTop * step;
        rec = gGalleryRecs[3];
        rec.x = x + 44;
        top = y + 46 + pos;
        rec.y = top;
        Gallery_DrawRecords(&rec, 1);
        /* through a byte pointer: the game copies it as words, not as the
         * 16-aligned object MWCC knows it is */
        rec = *(GalRec *)(u8 *)&gGalleryScrollBar;
        rec.x = x + 44;
        rec.y = len + top;
        Gallery_DrawRecords(&rec, 1);
        box.x = x + 50;
        box.w = box.x + 10;
        box.y = y + 68 + pos;
        box.h = box.y + len - 22;
        box.color = 0xFFF0EBE7;
        Prim_FillRect((struct PrimLine *)&box);
        if (gal->listTop != 0) {
            rec = gGalleryScrollRecs[0];
            rec.x = x + 168;
            rec.y = y + 24 + gal->upArrowOfs;
            if (gal->upArrowOfs != 0) {
                rec.color = 0xFFFB7103;
            }
            rec.angle = ANGLE_TURN / 4;
            Gallery_DrawRecords(&rec, 1);
        }
        if (gal->listTop + GAL_LIST_ROWS < gal->entryCounts[gal->category]) {
            rec = gGalleryScrollRecs[0];
            rec.x = x + 168;
            rec.y = y + 244 + gal->downArrowOfs;
            if (gal->downArrowOfs != 0) {
                rec.color = 0xFFFB7103;
            }
            rec.angle = ANGLE_TURN * 3 / 4;
            Gallery_DrawRecords(&rec, 1);
        }
    }
}

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
    rec = gGalleryRecs[2];
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
    Font_SetSpacing(14, 14);
    if (blink == 1) {
        Font_SetPos((s16)(x - 2) + 2, (s16)(y - 14) + 2);
        Font_SetColor(0x80000000);
        Font_Printf(gGalleryNewText[0]);
    }
    Font_SetPos(x - 2, y - 14);
    Font_SetColor(0x807F7F28);
    Font_Printf(gGalleryNewText[0]);
}

s32 sprintf(char *, const char *, ...);    /* sprintf */
char *strchr(char *, s32);                  /* strchr */
s32 Str_Length(char *);                         /* a message's width */
/* unit's own declaration: x and y as s16, as Gallery_DrawEntryPage and Gallery_DrawList pass them; with the definition's s32, the calls compile differently */
void Hud_DrawButtonIcon(s32, s16, s16, s32, u32, f32);  /* a button icon */

/* Draws the list: four numbered entries from listTop, the selected one
 * yellow, a button code in a title drawn as a button icon. Kept for the
 * match: Gallery_DrawNewBlink takes s16 coordinates (the extensions at the
 * call); the strchr loop tests `!scan != 0` at its top (not rotated); and
 * bufStart, a copy of &buf set in the for's init, is the saved register
 * the game keeps for `scan - bufStart`. */
void Gallery_DrawList(GalWork *gal)
{
    char buf[264];
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
    y = (s32)(40.0f + gal->pos[5][1]) + 42;
    for (i = 0, bufStart = buf; i < GAL_LIST_ROWS; i++) {
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
        Font_SetSpacing(18, 18);
        Font_Printf(num);
        numberX = x + Str_Length(num) * 9;
        Font_SetSpacing(16, 16);
        Font_SetPos(numberX, y + 1);
        strcpy(buf, gal->names[gal->listTop + i]);
        for (;;) {
            scan = strchr(buf, '%');
            if (!scan != 0) {
                break;
            }
            if (scan[1] == 'r') {
                Hud_DrawButtonIcon(25, numberX + (scan - bufStart) * 8, y + 1, 341, color, 0.8f);
                scan[0] = ' ';
                scan[1] = ' ';
            }
        }
        Font_Printf(buf);
        y += 42;
    }
    Gallery_DrawCursor(gal);
    Gallery_DrawScrollBar(gal);
}

s32 strcmp(char *, char *);                 /* strcmp */
u32 strlen(char *);
/* The full-width opening double angle bracket, which opens a centred line. */
#pragma rodata on
char D_006D06F0[3] = {
#include "select/006D06F0.inc"
};
#pragma rodata off

/* An entry's page (while pageShown): its picture, the frame and its four
 * parts, the four description lines (a line starting with that bracket is centred)
 * and the two help lines. centredX holds the centred line's left edge. */
void Gallery_DrawEntryPage(GalWork *gal)
{
    Sprite spr;
    GalRec rec;
    char head[16];
    char buf[256];
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

    pagePos = gGalleryEntryPagePos;
    entry = gGalleryEntries + gal->entries[gal->selected];
    if (Gallery_IsUnlocked(gal->entries[gal->selected]) == 1) {
        tex = entry->picture + GAL_TEX_FIRST;
    } else {
        tex = GAL_TEX_LOCKED;
    }
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gs_SetState(RS_TEX_FILTER, 0);
    Tex_Use(1, (u32 *)&gTexHandles[tex]);
    Gs_SetState(RS_TEXTURE, gTexHandles[tex]);
    spr.x = pagePos[1];
    spr.y = *(pagePos += 2);
    spr.w = pagePos[1];
    spr.h = *(pagePos += 2);
    spr.u1 = 255;
    spr.v1 = 255;
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
        Font_SetSpacing(16, 16);
        line = entry->lines;
        bufStart = buf;
        y = 306;
        for (i = 0; i < 4; i++) {
            text = *line;
            head[0] = text[0];
            head[1] = text[1];
            head[2] = 0;
            if (strcmp(head, D_006D06F0) == 0) {
                x = (36 - Str_Length(*line)) * 8 / 2;
                Font_SetPos(x + 308, y);
                strcpy(buf, *line);
                centredX = x + 308;
                for (;;) {
                    scan = strchr(buf, '%');
                    if (!scan != 0) {
                        break;
                    }
                    if (scan[1] == 'r') {
                        Hud_DrawButtonIcon(25, centredX + (scan - bufStart) * 8, y, 341, 0xFFCAC8B4, 0.8f);
                        scan[0] = ' ';
                        scan[1] = ' ';
                    }
                }
                Font_Printf(buf);
            } else if (strlen(*line) != 0) {
                Font_SetPos(308, y);
                strcpy(buf, *line);
                for (;;) {
                    scan = strchr(buf, '%');
                    if (!scan != 0) {
                        break;
                    }
                    if (scan[1] == 'r') {
                        Hud_DrawButtonIcon(25, (scan - bufStart) * 8 + 308, y, 341, 0xFFCAC8B4, 0.8f);
                        scan[0] = ' ';
                        scan[1] = ' ';
                    }
                }
                Font_Printf(buf);
            }
            line++;
            y += 18;
        }
    }
    Font_SetColor(0xFF808080);
    Font_SetSpacing(14, 14);
    Font_SetPos(422, 394);
    Text_Printf(gGalleryPlayHelp[0]);
    Font_SetPos(422, 412);
    Text_Printf(gGalleryPlayHelp[1]);
}

struct TexRect;

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
        if (*ids == 255) {
            continue;
        }
        *(recPtr = &rec) = gGalleryLayout[*ids];
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
            spr.u1 = spr.u0 + 255;
        }
        spr.color = recPtr->color;
        if (spr.x < 640 && spr.x + spr.w >= 0 && spr.y < 448 && spr.y + spr.h >= 0) {
            Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
            Gs_SetState(RS_TEX_FILTER, 0);
            Tex_Use(1, (u32 *)&gal->texHandles[recPtr->tex]);
            Gs_SetState(RS_TEXTURE, gal->texHandles[recPtr->tex]);
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
        if (spr.x < 640 && spr.x + spr.w >= 0 && spr.y < 448 && spr.y + spr.h >= 0) {
            Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
            Gs_SetState(RS_TEX_FILTER, 0);
            if (rec->tex > 100) {
                Tex_Use(1, (u32 *)&gTexHandles[rec->tex]);
                Gs_SetState(RS_TEXTURE, gTexHandles[rec->tex]);
            } else {
                Tex_Use(1, (u32 *)&gal->texHandles[rec->tex]);
                Gs_SetState(RS_TEXTURE, gal->texHandles[rec->tex]);
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
    for (i = 0; i < GAL_PICTURE_COUNT; i++) {
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
                gal->movePhase[i] += 2048;
                if (gal->movePhase[i] >= 16384) {
                    gal->movePhase[i] = 16384;
                }
            }
        }
    }
    if (gal->panelSlide == 1) {
        gal->panelOffset = gal->panelOffset - (s32)(40.0f * Math_Sin(6.2831855f * gal->panelPhase / 65536.0f));
        gal->panelPhase += 2048;
        if (gal->panelPhase >= 16384) {
            gal->panelPhase = 16384;
        }
    }
    if (gal->panelSlide == 2) {
        gal->panelOffset = gal->panelOffset + (s32)(40.0f * Math_Sin(6.2831855f * gal->panelPhase / 65536.0f));
        gal->panelPhase += 2048;
        if (gal->panelPhase >= 16384) {
            gal->panelPhase = 16384;
        }
    }
}

/*
 * Starts the pictures moving to layout k (gGalleryLayoutPos): each waits
 * its delay, then moves at its speed (gGalleryLayoutSteps,
 * gGalleryLayoutSpeeds). A picture already in place is skipped through
 * the same step as a picture that stays (the goto, as the game's code).
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
    for (i = 0; i < GAL_PICTURE_COUNT; i++) {
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
 * speeds (gGalleryLayoutBackSteps); the goto as in Gallery_SetLayout.
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
    for (i = 0; i < GAL_PICTURE_COUNT; i++) {
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
    GalWork *gal = (GalWork *)((s32)gSysSettings.workArea + 4096); /* GAL changes the code here */

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
        if (gal->settleTimer == 10) {
            Gallery_ApplyLayout(gal, gal->category + 1);
        }
    } else if (gal->settleTimer == 8) {
        Gallery_ApplyLayout(gal, 0);
    }
    for (i = 0; i < GAL_PICTURE_COUNT; i++) {
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
            strcpy(gal->names[i], gGalleryEntries[gal->entries[idx]].title);
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
 * the list scrolled. The two moves end in shared tails (the gotos), as the
 * game's code does.
 */
s32 Gallery_ListInput(GalWork *gal, u16 pad)
{
    u16 buttons;

    gal->cursorPhase += 4096;
    if (gal->cursorPhase >= 16384) {
        gal->cursorPhase = 16384;
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
    buttons = pad;
    if (buttons & PAD_UP) {
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
    if (buttons & PAD_DOWN) {
        if (gal->listRow >= GAL_LIST_ROWS - 1) {
            if (gal->listTop + GAL_LIST_ROWS < gal->entryCounts[gal->category]) {
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
    Sound_PlaySeFlat(0, 19, 0);
}

void Gallery_PlayCancelSe(void)
{
    Sound_PlaySeFlat(0, 20, 0);
}

void Gallery_PlayPageSe(void)
{
    Sound_PlaySeFlat(0, 23, 0);
}

/* Draws the arrows beside category k on the menu. */
void Gallery_DrawArrow(s32 k)
{
    GalWork *gal;
    Sprite spr;
    s16 (*pos)[2];
    GalIcon *icon;

    gal = GAL;
    Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
    Gs_SetState(RS_TEX_FILTER, 0);
    Tex_Use(1, (u32 *)&gTexHandles[gGalleryArrowSprites[0].tex]);
    Gs_SetState(RS_TEXTURE, gTexHandles[gGalleryArrowSprites[0].tex]);
    if (gal->state >= GAL_LIST_MONSTERS && gal->state <= GAL_LIST_BONUS) {
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
        gal->panelOffset = -260;
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
