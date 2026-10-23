/*
 * The quest results screen (Plsel_ResultsUpdate runs its steps): the tally
 * of the rewards and the hunter rank points, the monsters' size records, the
 * awards and the quests and notices they unlock, the reward pages and their
 * text, the item box menu that takes the rewards, the arena record menu
 * that keeps a captured monster, and the drawing of each page. Its state is
 * PlselWork, the plsel work area gPlselWork (sub_main's shared overlay work
 * pointer), with the file's own gPlselBonus (PlselBonus: the records and
 * items that pay extra) and gPlselItemBox (the item box menu). Plsel_ResultsUpdate steps through
 * enum PlselResStep, one page after another. The pages' lines are formatted
 * from sub_main's text tables (gPlselTexts, gHelpTexts_6), indexed by
 * number where each entry is used once.
 *
 * The parts share one file because their data does: the .data tables lie
 * in another order than the code that reads them (the item box menu's
 * windows after the awards' tables), so no cut between two parts splits
 * the file's data in two.
 */
#include "common.h"
#include "sprite.h"
#include "screen_fade.h"
extern u8 gHelpShow;
extern u8 gHelpPage;
extern s8 gHelpColumn;
extern s16 gHelpLine;
#include "syssettings.h"
extern u16 gFrameTick; /* sub_main: the frame counter */
#include "pad.h"
#include "m2c_macros.h"
#include "plsel.h"
#include "player.h"
#include "scene.h"
#include "save.h"
#include "quest.h"
#include "area.h"
#include "qsync.h"
#include "sound.h"
#include "render_state.h"
#include "prim.h"
#include "hud.h"
#include "plsel/quest_results.h"   /* the texts this file uses (tools/content_gen.py) */
#include "texture.h"
#include "area_draw.h"
#include "fishing.h"
#include "model.h"
#include "shake.h"
#include "stream.h"
#include "draw_helpers.h"
#include "draw_slots.h"
#include "file_load.h"
#include "font.h"
#include "frame.h"
#include "game_util.h"
#include "menu_util.h"
#include "packet_tim2.h"
#include "rand.h"
#include "libc.h"
#include "sprite_queue.h"

/* The game compiled this file at optimization level 4, as the other plsel
 * files: Plsel_UpdateSizeRecords matches only there. */
#pragma optimization_level 4

struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha); /* sub_main: w is a struct ListWin * */
void Gfx_SetBlend(s32, s32);
void Text_SetSize(s32 size);
void Text_Printf(s8 *fmt, ...);
void Text_SetColor(s16 color);

extern u8 gPouchBackup[];

/* unit's own declaration: declared here taking a u16; with the definition's s32 parameter the award checks compile differently */
s32 Award_Grant(u16);







void Gfx_SetTexFilter(s32);
void Tex_Bind(s32);
struct TexRect;

extern NextQuest gNextQuest;

void Text_PrintfColored(const char *fmt, ...);
extern char gPlselArrowUp[];
extern char gPlselArrowDown[];

/* unit's own declaration: declared here returning u16; with the definition's u8 Plsel_ItemBoxMenu compiles differently */
u16 Pouch_Add(Player *, u16, s16);
extern Player gPlayers[];

extern const char gPlselStrFmt[];

void Text_Draw(char *);

char *fptodp(f32);

struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);

extern PlselKind gRecordCountEntries[];

s32 Str_Length(char *);

extern char gPlselPageFmt[];

/* The file's .bss, declared from the highest address down: the compiler
 * places them last-declared first. */
s32 gPlselItemBoxSale[4];
PlselItemBoxMenu gPlselItemBox;
PlselBonus gPlselBonus;

/* The file's .data: the results screen's panels and tables. */
ListWin gPlselResPanels_0 = {
#include "plsel/00D78B10.inc"
};
ListWin gPlselResPanels_1 = {
#include "plsel/00D78B30.inc"
};
ListWin gPlselResPanels_2 = {
#include "plsel/00D78B50.inc"
};
ListWin gPlselResPanels_3 = {
#include "plsel/00D78B70.inc"
};
ListWin gPlselResPanels_4 = {
#include "plsel/00D78B90.inc"
};
ListWin gPlselResPanels_5 = {
#include "plsel/00D78BB0.inc"
};
ListWin gPlselResPanels_6 = {
#include "plsel/00D78BD0.inc"
};
ListWin gPlselResPanels_7 = {
#include "plsel/00D78BF0.inc"
};
ListWin *gPlselResPanels[8] = {
    &gPlselResPanels_0, &gPlselResPanels_1, &gPlselResPanels_2, &gPlselResPanels_3,
    &gPlselResPanels_4, &gPlselResPanels_5, &gPlselResPanels_6, &gPlselResPanels_7,
};
/* The reward per count of each hunting category (Plsel_ComputeRewards). */
s16 gPlselRewardTbl[17] = {
#include "plsel/00D78C30.inc"
};
/* The quest statistic each category counts (-1: none). */
s8 gPlselRewardSteps[17] = {
#include "plsel/00D78C60.inc"
};
u16 gPlselFlagUnlocks[72] = {
#include "plsel/00D78C80.inc"
};
u16 gPlselArenaRecordChances[56] = {
#include "plsel/00D78D10.inc"
};

PlselHuntAward gPlselHuntAwards[60] = {
#include "plsel/00D78D80.inc"
};
u16 gPlselDrillNoticeStages[16] = {
#include "plsel/00D78E70.inc"
};
ListPanel gPlselItemBoxWin = {
#include "plsel/00D78E90.inc"
};
Panel gPlselItemBoxTimeWin = {
#include "plsel/00D78EB0.inc"
};
Icon gPlselItemBoxHintIcon[1] = {
#include "plsel/00D78EC8.inc"
};
Icon gPlselItemBoxNextIcon[1] = {
#include "plsel/00D78ED0.inc"
};
Icon gPlselResultsBgIcon[1] = {
#include "plsel/00D78ED8.inc"
};
Frame gPlselItemBoxFrame = {
#include "plsel/00D78EE0.inc"
};
u8 gPlselRankAwardRanks[32] = {
#include "plsel/00D78F00.inc"
};
u16 gPlselRankAwards[32] = {
#include "plsel/00D78F20.inc"
};
u16 gPlselCountAwards[8] = {
#include "plsel/00D78F60.inc"
};
u16 gPlselAwardTblA[16] = {
#include "plsel/00D78F70.inc"
};
u16 gPlselAwardTblB[16] = {
#include "plsel/00D78F90.inc"
};
u16 gPlselAwardTblC[16] = {
#include "plsel/00D78FB0.inc"
};
u16 gPlselTrainingAwards[20] = {
#include "plsel/00D78FD0.inc"
};
u16 gPlselOfflineAwardsA[4] = {
#include "plsel/00D78FF8.inc"
};
u16 gPlselOfflineAwardsB[8] = {
#include "plsel/00D79000.inc"
};
u16 gPlselOfflineAwardsC[96] = {
#include "plsel/00D79010.inc"
};
u16 gPlselOfflineAwardsD[88] = {
#include "plsel/00D790D0.inc"
};
u16 gPlselOfflineAwardsE[88] = {
#include "plsel/00D79180.inc"
};
u16 gPlselAwardTblD[16] = {
#include "plsel/00D79230.inc"
};
f32 gPlselResultsBgDepth[4][2] = {
#include "plsel/00D79250.inc"
};
ListPanel gPlselSaveRecPanel = {
#include "plsel/00D79270.inc"
};
Panel gPlselSaveRecText = {
#include "plsel/00D79290.inc"
};
ListPanel gPlselSaveRecWinA = {
#include "plsel/00D792B0.inc"
};
ListPanel gPlselSaveRecWinB = {
#include "plsel/00D792D0.inc"
};
ListWin gPlselSaveRecPanel2 = {
#include "plsel/00D792F0.inc"
};
u16 gPlselQuestUnlocks0[8] = {
#include "plsel/00D79310.inc"
};
u16 gPlselQuestUnlocks1[16] = {
#include "plsel/00D79320.inc"
};
u16 gPlselQuestUnlocks2[20] = {
#include "plsel/00D79340.inc"
};
u16 gPlselQuestUnlockIds[4] = {
#include "plsel/00D79368.inc"
};

extern s32 gRewardMoney;
/*
 * Pays the reward money still held in gRewardMoney into the save
 * (Save_AddMoney) and clears it.
 */
void Plsel_FlushMoney(void)
{
    if (gRewardMoney != 0) {
        Save_AddMoney(gRewardMoney);
        gRewardMoney = 0;
    }
}

/*
 * Draw hook of the steps from the arena record menu to the item box menu:
 * the captured screen, then the arena record menu (PLRES_ARENA_MENU and
 * PLRES_ARENA_OUT) or the item box menu.
 */
void PlselRes_DrawMenus(void)
{
    Text_SetSize(4);
    ScreenCapture_Draw();
    if (gScene.step == PLRES_ARENA_MENU || gScene.step == PLRES_ARENA_OUT) {
        Plsel_DrawSaveRecordMenu();
    } else {
        Plsel_DrawItemBoxMenu();
    }
}

/*
 * Draw hook of the quit notice (after QUEST_PHASE_QUIT): the results panel,
 * opaque.
 */
void PlselRes_DrawPanel(void)
{
    Text_SetSize(4);
    Gs_SetState(RS_ALPHA_REF, 0);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    ListPanel_DrawAlpha(gPlselResPanels[0], 0, -1, 255);
}

void Plsel_FadeOutMusic(void)
{
    Stream_FadeTo(0, 30, 0);
}

void Plsel_RestorePouch(void)
{
    memcpy(gSave.pouch, gPouchBackup, 80);
}

void PlselRes_PlayDecideSe(void)
{
    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
}

void PlselRes_PlayPageSe(void)
{
    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
}

void PlselRes_PlayCancelSe(void)
{
    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
}

/* Static in the original: Plsel_UpdateSizeRecords keeps t1-t3 across calls to it,
 * which MWCC only does for a static function it has already compiled. */
static void Plsel_AddRecordGroup(s32 group)
{
    PlselWork *w;
    s32 i;

    w = gPlselWork;
    for (i = 0; i < w->recordGroupCount; i++) {
        if (w->recordGroups[i] == group) {
            return;
        }
    }
    w->recordGroups[w->recordGroupCount] = group;
    w->recordGroupCount++;
}

/* Updates the monster size records with this quest's largest and
 * smallest monsters, keeping the earlier records of each monster group for
 * the results pages, and clears the quest's sizes.
 * newRecord is set before Plsel_AddRecordGroup in the second largest test
 * (the game's code stores it in the call's delay slot), and the third loop
 * has its own record variable (rec) and counts with j. Three compares are
 * written with the j entry first (`a[j] > a[g]`): the compiler loads a
 * comparison's operands in source order, and this order gives the game's
 * registers. */
void Plsel_UpdateSizeRecords(void)
{
    PlselWork *w;
    s32 i;
    s32 j;
    s32 group;
    u16 savedSize;
    u16 rec;

    w = gPlselWork;
    for (j = 1; j < PLSEL_GROUP_COUNT; j++) {
        group = MonsterGroup_First(j);
        w->largestBefore[group] = Record_GroupLargest(&gSave, j, NULL);
        w->smallestBefore[group] = Record_GroupSmallest(&gSave, j, NULL);
    }
    for (i = 1; i < PLSEL_GROUP_COUNT; i++) {
        if (gQuest.sizeMax[i] > 0) {
            w->questLargest[i] = gQuest.sizeMax[i];
        }
        if (gQuest.sizeMin[i] > 0) {
            w->questSmallest[i] = gQuest.sizeMin[i];
        }
        savedSize = Record_GetLargest(&gSave, i);
        if (savedSize < gQuest.sizeMax[i]) {
            Record_SetLargest(&gSave, i, gQuest.sizeMax[i]);
        }
        savedSize = Record_GetSmallest(&gSave, i);
        if (gQuest.sizeMin[i] > 0) {
            if (savedSize == 0) {
                Record_SetSmallest(&gSave, i, gQuest.sizeMin[i]);
            } else if (gQuest.sizeMin[i] < savedSize) {
                Record_SetSmallest(&gSave, i, gQuest.sizeMin[i]);
            }
        }
        gQuest.sizeMax[i] = 0;
        gQuest.sizeMin[i] = 0;
    }
    for (j = 1; j < PLSEL_GROUP_COUNT; j++) {
        group = MonsterGroup_First(j);
        if (w->questLargest[j] > 0) {
            rec = Record_GroupLargest(&gSave, j, NULL);
            if (rec > 0) {
                if (w->largestBefore[group] == 0) {
                    if (w->largestNew[group] != 0) {
                        if (w->questLargest[j] > w->questLargest[group]) {
                            w->questLargest[group] = w->questLargest[j];
                            w->questLargest[j] = 0;
                            w->largestNew[group] = 1;
                            w->newRecord = 1;
                            Plsel_AddRecordGroup(group);
                        }
                    } else {
                        w->largestNew[group] = 1;
                        w->newRecord = 1;
                        w->questLargest[group] = rec;
                        Plsel_AddRecordGroup(group);
                    }
                }
                if (w->largestBefore[group] < rec) {
                    if (w->largestNew[group] != 0) {
                        if (w->questLargest[j] > w->questLargest[group]) {
                            w->questLargest[group] = w->questLargest[j];
                            w->questLargest[j] = 0;
                            w->largestNew[group] = 2;
                            w->newRecord = 1;
                            Plsel_AddRecordGroup(group);
                        }
                    } else {
                        w->largestNew[group] = 2;
                        w->newRecord = 1;
                        w->questLargest[group] = rec;
                        Plsel_AddRecordGroup(group);
                    }
                } else {
                    if (j != group && w->questLargest[j] > w->questLargest[group]) {
                        w->questLargest[group] = w->questLargest[j];
                        w->questLargest[j] = 0;
                    }
                    Plsel_AddRecordGroup(group);
                }
            }
        }
        if (w->questSmallest[j] > 0) {
            savedSize = Record_GroupSmallest(&gSave, j, NULL);
            if (savedSize > 0) {
                if (w->smallestBefore[group] == 0) {
                    if (w->smallestNew[group] != 0) {
                        if (w->questSmallest[j] < w->questSmallest[group]) {
                            w->questSmallest[group] = w->questSmallest[j];
                            w->questSmallest[j] = 0;
                            w->smallestNew[group] = 1;
                            w->newRecord = 1;
                            Plsel_AddRecordGroup(group);
                        }
                    } else {
                        w->smallestNew[group] = 1;
                        w->newRecord = 1;
                        w->questSmallest[group] = savedSize;
                        Plsel_AddRecordGroup(group);
                    }
                }
                if (savedSize < w->smallestBefore[group]) {
                    if (w->smallestNew[group] != 0) {
                        if (w->questSmallest[j] < w->questSmallest[group]) {
                            w->questSmallest[group] = w->questSmallest[j];
                            w->questSmallest[j] = 0;
                            w->smallestNew[group] = 2;
                            w->newRecord = 1;
                            Plsel_AddRecordGroup(group);
                        }
                    } else {
                        w->smallestNew[group] = 2;
                        w->newRecord = 1;
                        w->questSmallest[group] = savedSize;
                        Plsel_AddRecordGroup(group);
                    }
                } else {
                    if (j != group) {
                        if (w->questSmallest[group] == 0) {
                            w->questSmallest[group] = w->questSmallest[j];
                        } else if (w->questSmallest[j] < w->questSmallest[group]) {
                            w->questSmallest[group] = w->questSmallest[j];
                        }
                        w->questSmallest[j] = 0;
                    }
                    Plsel_AddRecordGroup(group);
                }
            }
        }
    }
}

/*
 * Returns 1 when a size of this quest (gQuest.sizeMax and sizeMin, for the
 * groups after gPlselBonus.sizeCheckAfter) is short of its group's record,
 * else 0. Neither parameter is read. The drill results call it after
 * Plsel_UpdateSizeRecords has cleared the sizes, and drop the result.
 */
s32 Plsel_CheckSizeRecords(SceneState *scene, s32 unused)
{
    s32 i;
    u16 rec;

    for (i = gPlselBonus.sizeCheckAfter + 1; i < PLSEL_GROUP_COUNT; i++) {
        if (gQuest.sizeMax[i] != 0 || gQuest.sizeMin[i] != 0) {
            if (gQuest.sizeMax[i] != 0) {
                rec = Record_GroupLargest(&gSave, i, NULL);
            }
            if (gQuest.sizeMin[i] != 0) {
                rec = Record_GroupSmallest(&gSave, i, NULL);
            }
            if (gQuest.sizeMax[i] != 0 && gQuest.sizeMax[i] < rec) {
                return 1;
            }
            if (gQuest.sizeMin[i] != 0 && gQuest.sizeMax[i] >= rec) {
                return 1;
            }
        }
    }
    return 0;
}

/* Fills the reward records: the quest targets first, then the monsters hunted. */
void Plsel_CheckTargetBonus(PlselWork *w)
{
    Player *pl;
    s32 n;
    s32 i;
    s32 j;
    s32 k;

    gPlselBonus.hasBonusRecs = 0;
    n = 0;
    pl = &gPlayers[gScene.localPlayer];
    memset(gPlselBonus.recs, 0, sizeof(gPlselBonus.recs));
    for (i = 0; i < 2; i++) {
        if (gQuest.huntGoals[i].type != 0 && gQuest.huntGoals[i].kind != 0) {
            k = MonsterGroup_First(gQuest.huntGoals[i].kind);
            if (gQuest.huntGoals[i].type == 1) {
                if (w->huntedLeft[k] >= gQuest.huntGoals[i].need) {
                    gPlselBonus.recs[n].type = 1;
                    gPlselBonus.recs[n].id = k;
                    if (i == 0) {
                        gPlselBonus.recs[n].count = w->huntedLeft[k] / gQuest.huntGoals[i].need;
                        w->huntedLeft[k] -= (u16)(gPlselBonus.recs[n].count * gQuest.huntGoals[i].need);
                    } else {
                        gPlselBonus.recs[n].count = 1;
                        w->huntedLeft[k] = 0;
                    }
                    gPlselBonus.recs[n].reward = gQuest.huntGoals[i].reward;
                    n++;
                    gPlselBonus.hasBonusRecs = 1;
                }
            } else if (gQuest.huntGoals[i].type == 2) {
                if (w->capturedLeft[k] >= gQuest.huntGoals[i].need) {
                    gPlselBonus.recs[n].type = 2;
                    gPlselBonus.recs[n].id = k;
                    if (i == 0) {
                        gPlselBonus.recs[n].count = w->capturedLeft[k] / gQuest.huntGoals[i].need;
                        w->capturedLeft[k] -= (u16)(gPlselBonus.recs[n].count * gQuest.huntGoals[i].need);
                    } else {
                        gPlselBonus.recs[n].count = 1;
                        w->capturedLeft[k] = 0;
                    }
                    gPlselBonus.recs[n].reward = gQuest.huntGoals[i].reward;
                    n++;
                    gPlselBonus.hasBonusRecs = 1;
                }
            }
        }
    }
    for (i = 0; i < PLSEL_GROUP_COUNT; i++) {
        if (n >= 8) {
            break;
        }
        k = MonsterGroup_First(i);
        if (Rate_GetSnapshot(k) != 0 && w->huntedLeft[k] + w->capturedLeft[k] > 0) {
            gPlselBonus.recs[n].type = 0;
            gPlselBonus.recs[n].id = i;
            gPlselBonus.recs[n].count = w->huntedLeft[k] + w->capturedLeft[k];
            gPlselBonus.recs[n].reward = Rate_GetSnapshot(k);
            gPlselBonus.hasBonusRecs = 1;
            n++;
            w->huntedLeft[k] = 0;
            w->capturedLeft[k] = 0;
        }
    }
    if (n < 7 && gQuest.huntGoals[2].type == 3 && gQuest.huntGoals[2].kind != 0) {

        for (j = 0; j < 20; j++) {
            if (pl->pouch[j].item == gQuest.huntGoals[2].kind && pl->pouch[j].count >= gQuest.huntGoals[2].need) {
                gPlselBonus.recs[n].type = 3;
                gPlselBonus.recs[n].id = gQuest.huntGoals[2].kind;
                gPlselBonus.recs[n].count = gQuest.huntGoals[2].need;
                gPlselBonus.recs[n].reward = gQuest.huntGoals[2].reward;
                Pouch_Add(pl, pl->pouch[j].item, -gQuest.huntGoals[2].need);
                gPlselBonus.hasBonusRecs = 1;
            }
        }
    }
}

/*
 * Moves the local player's pouch stacks whose item is sold at the quest's
 * end (ITEM_FLAG2_SALE) into gPlselBonus.items, for the money tally to pay
 * at value x count. Returns the number of stacks taken.
 */
s32 Plsel_TakeSaleItems(void)
{
    Player *pl;
    s32 j;
    s32 n;
    ItemData *items;
    u16 id;
    s32 i;

    n = 0;
    gPlselBonus.itemsTaken = 0;
    pl = &gPlayers[gScene.localPlayer];
    for (i = 0; i < 20; i++) {
        gPlselBonus.items[i].item = 0;
        gPlselBonus.items[i].count = 0;
    }
    items = gItemData;
    for (j = 0; j < 20; j++) {
        id = pl->pouch[j].item;
        if (id != 0 && (items[id].flags2 & 2)) {
            gPlselBonus.items[n].item = id;
            gPlselBonus.items[n].count = pl->pouch[j].count;
            Pouch_Add(pl, pl->pouch[j].item, -pl->pouch[j].count);
            gPlselBonus.itemsTaken = 1;
            n++;
        }
    }
    return n;
}

extern u16 gQuestCapturedKind;
/*
 * Rolls whether the captured monster may be kept as an arena record: never
 * in a drill, a training quest or a quest without a fee, nor before save
 * event flag EVENT_FLAG_ARENA_CUP_OPEN. gPlselArenaRecordChances gives each kind's chance in 32;
 * one kind also needs hunter title 141. Returns 1 to open the arena record
 * menu (PLRES_ARENA_MENU), else 0.
 */
s32 Plsel_RollArenaRecord(void)
{
    u16 *entry;
    u16 capturedKind;
    u16 entryKind;

    if (Quest_IsDrill() == 1) {
        return 0;
    }
    if (Quest_IsTraining() == 1) {
        return 0;
    }
    if (gQuestCapturedKind == 0) {
        return 0;
    }
    if (gScene.questPaid == 0) {
        return 0;
    }
    if (Save_TestEventFlag(EVENT_FLAG_ARENA_CUP_OPEN) == 0) {
        return 0;
    }
    capturedKind = gQuestCapturedKind;
    entry = gPlselArenaRecordChances;
    entryKind = *entry;
    while (entryKind != capturedKind) {
        if (entryKind == 0xFFFF) {
            return 0;
        }
        entry += 2;
        entryKind = *entry;
    }
    if (capturedKind == 53 && Save_HasTitle(141) == 0) {
        return 0;
    }
    if ((Rand_Next(1) & 0x1F) < entry[1]) {
        return 1;
    }
    return 0;
}

/*
 * Whether an offline clear goes on to the opening movie (SCENE_OPENING,
 * OPENING_SUB_MOVIE): save event flag 388 set, 391 and 389 (which the
 * movie sets) clear, and gSave.awayFromVillage 0. Returns 1 or 0.
 */
s32 Plsel_ShouldPlayOpening(void)
{
    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (Quest_GetOutcome(1) != 1) {
        return 0;
    }
    if (Save_TestEventFlag(388) == 0) {
        return 0;
    }
    if (Save_TestEventFlag(391) == 1) {
        return 0;
    }
    if (Save_TestEventFlag(389) == 1) {
        return 0;
    }
    if (gSave.awayFromVillage == 0) {
        return 1;
    }
    return 0;
}

/*
 * Whether an offline clear of quest QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE goes on to the ending
 * (SCENE_OPENING, OPENING_SUB_ENDING: its movie, then the ending overlay)
 * while save event flag 0x58 is clear. Returns 1 or 0.
 */
s32 Plsel_ShouldPlayEnding(void)
{
    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (Quest_GetOutcome(1) != 1) {
        return 0;
    }
    if (gQuest.stage != QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE) {
        return 0;
    }
    if (Save_TestEventFlag(88) == 1) {
        return 0;
    }
    return 1;
}

void Rate_ListAdd(s16 kind, s8 n); /* unit's own declaration: the definition (main) takes s32; Plsel_ResultsUpdate is 842 words off with it */
void Player_BackupPouch(Player *, s32);
extern DrawSlot gNetMenuDrawHooks[2];

/* The quest results screen: its steps (SceneState.state) from the tally at
 * the quest's end through the rewards, items and awards pages to the next
 * scene. */
void Plsel_ResultsUpdate(void)
{
    SceneState *sceneState;
    PlselWork *w;
    Player *pl;
    s32 i;

    w = gPlselWork;
    sceneState = &gScene;
    Text_SetSize(4);
    switch (gScene.step) {
    case PLRES_START:
        memset(&gPlselBonus, 0, 196);
        Sound_Silence();
        sceneState->stateSyncFrame = 0;
        gAreaState.drawSky = 0;
        gPlselBonus.sizeCheckAfter = 0;
        for (i = 0; i < 2; i++) {
            memset(&gNetMenuDrawHooks[i], 0, sizeof(DrawSlot));
        }
        QuestRecord_Fill();
        QuestStats_Tally();
        memset(w, 0, sizeof(PlselWork));
        Plsel_CountHunts(w);
        if (Quest_IsDrill() == 1) {
            w->clearedBefore = Save_IsQuestCleared(gQuest.stage);
            Plsel_ComputeRewards(&gPlayers[sceneState->localPlayer]);
            Save_RestorePouch(&gPlayers[sceneState->localPlayer]);
        }
        if (Quest_IsTraining() == 1) {
            Save_RestorePouch(&gPlayers[sceneState->localPlayer]);
        }
        if (gScene.questPhase == QUEST_PHASE_QUIT) {
            sceneState->step = PLRES_QUIT_NOTICE;
            sceneState->subStep = 0;
            sceneState->timer = 150;
            sceneState->timer2 = 1;
            Plsel_FlushMoney();
            FadeA_Start(FADE_IN_BLACK);
            gNetMenuDrawHooks[0].draw = (DrawSlotFn)PlselRes_DrawPanel;
            Plsel_RestorePouch();
            break;
        }
        if (gScene.questPhase == QUEST_PHASE_ABORTED) {
            Plsel_QueueFlagUnlocks();
            Plsel_FlushMoney();
            Plsel_RestorePouch();
            if (Plsel_HasNotices() == 0) {
                sceneState->step = PLRES_EXIT;
                w->timeout = PLRES_PAGE_TIMEOUT;
                sceneState->timer = 30;
                FadeA_Start(FADE_BLACK);
                Plsel_FadeOutMusic();
            } else {
                Plsel_LoadResultsTextures();
                sceneState->step = PLRES_NOTICES_START;
                sceneState->subStep = 0;
                gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
                FadeA_Start(FADE_IN_BLACK);
            }
            return;
        }
        Plsel_QueueFlagUnlocks();
        Rand_StepSaved();
        i = gRewardMoney;
        /* gNetMenuDrawHooks[0].draw, stored through its address:
         * the store through the array takes other registers for the
         * money copied around it than the game's code does. */
        *(void (**)(void))0x6A3A24 = PlselRes_DrawMenus;
        gPlselBonus.rewardMoney = i;
        gRewardMoney = 0;
        for (i = 0; i < 20; i++) {
            pl = &gPlayers[sceneState->localPlayer];
            if (gItemData[pl->pouch[i].item].flags2 & 4) {
                Pouch_Add(pl, pl->pouch[i].item, -pl->pouch[i].count);
            }
        }
        Plsel_CheckTargetBonus(w);
        Plsel_TakeSaleItems();
        gNetMenuDrawHooks[0].draw = (DrawSlotFn)PlselRes_DrawMenus;
        if (Quest_IsTraining() != 1 && Quest_IsDrill() != 1) {
            for (i = 0; i < PLSEL_GROUP_COUNT; i++) {
                if (w->huntedByGroup[i] + w->capturedByGroup[i] > 0) {
                    Rate_ListAdd(i, w->huntedByGroup[i] + w->capturedByGroup[i]);
                }
            }
        }
        Plsel_UpdateSizeRecords();
        if (Quest_IsVersus() == 1) {
            Plsel_SnapshotVersusMonster();
            Player_BackupPouch(&gPlayers[gScene.localPlayer], 1);
            Versus_ConsumeTeamItem();
        }
        if (Quest_GetOutcome(1) == 1) {
            Quest_UpdateSaveRecords(0);
            if (Game_IsOnline() == 1) {
                if (Quest_IsMustOrder() == 0 || gScene.questPaid != 0) {
                    Save_MarkQuestCleared(gQuest.stage);
                }
            } else {
                Save_MarkQuestCleared(gQuest.stage);
            }
            if (Quest_IsTraining() == 1) {
                Plsel_CheckTrainingAwards();
            }
            if (Quest_IsDrill() == 1) {
                Plsel_CheckDrillAwards();
                Plsel_CheckSizeRecords(&gScene, 2);
                Plsel_CheckMonsterAwards();
            }
            Plsel_AddCounterPoints();
            if (QuestGoal_IsDone(0) == 1) {
                Plsel_QueueQuestUnlocks();
                Plsel_QueueDrillClearNotice();
            }
            QuestEm_SavePersistentHp();
            Quest_RollRewards();
            if (Game_IsOnline() == 1 && Plsel_RollArenaRecord() == 1) {
                sceneState->step = PLRES_ARENA_MENU;
                sceneState->subStep = 0;
                Plsel_InitSaveRecordMenu();
            } else {
                sceneState->step = PLRES_ITEMBOX;
                sceneState->subStep = 0;
                Plsel_InitItemBoxMenu();
            }
            sceneState->timer = 30;
            FadeA_Start(FADE_IN_BLACK);
        } else {
            Quest_UpdateSaveRecords(1);
            Plsel_AddCounterPoints();
            if (Quest_IsDrill() == 1) {
                Plsel_LoadResultsTextures();
                Plsel_CheckDrillAwards();
                Plsel_CheckSizeRecords(&gScene, 2);
                Plsel_CheckMonsterAwards();
                sceneState->step = PLRES_TITLES_START;
                sceneState->subStep = 0;
                w->timeout = PLRES_PAGE_TIMEOUT;
                w->closeState = PLRES_OPEN;
                gNetMenuDrawHooks[0].draw = NULL;
                break;
            }
            if (Game_IsOnline() == 1) {
                if (Quest_IsVersus() == 1) {
                    Plsel_LoadResultsTextures();
                    Save_StorePouch(&gPlayers[sceneState->localPlayer]);
                    Plsel_CheckVersusAwards();
                    PlselRes_InitMonsterPage();
                    if (gNextQuest.arenaRecord == -1) {
                        sceneState->step = PLRES_TITLES_START;
                    } else {
                        sceneState->step = PLRES_MONSTER;
                    }
                    sceneState->subStep = 0;
                    w->timeout = PLRES_PAGE_TIMEOUT;
                    w->closeState = PLRES_OPEN;
                    gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
                    sceneState->timer = 30;
                    FadeA_Start(FADE_IN_BLACK);
                    break;
                }
                if (Plsel_RollArenaRecord() == 1) {
                    sceneState->step = PLRES_ARENA_MENU;
                    sceneState->subStep = 0;
                    Plsel_InitSaveRecordMenu();
                    sceneState->timer = 30;
                    FadeA_Start(FADE_IN_BLACK);
                    break;
                }
            }
            Plsel_LoadResultsTextures();
            sceneState->step = PLRES_MONEY;
            Save_StorePouch(&gPlayers[sceneState->localPlayer]);
            Plsel_ComputeAllRewards();
            gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
            sceneState->timer = 30;
            FadeA_Start(FADE_IN_BLACK);
        }
        break;
    case PLRES_ITEMBOX:
        if (Plsel_ItemBoxMenu() <= 0) {
            sceneState->step++;
            FadeA_Start(FADE_OUT_BLACK);
            sceneState->timer = 30;
            Stream_FadeOut(0, 30);
            Save_StorePouch(&gPlayers[sceneState->localPlayer]);
        }
        break;
    case PLRES_ITEMBOX_OUT:
        if (--sceneState->timer <= 0) {
            Plsel_LoadResultsTextures();
            Stream_FadeToMax(0, 30);
            if (Quest_IsDrill() == 1) {
                sceneState->step = PLRES_REWARD_LIST;
                sceneState->subStep = 0;
                FadeA_Start(FADE_IN_BLACK);
                sceneState->timer = 30;
            } else {
                if (Quest_IsVersus() == 1) {
                    Plsel_CheckQuestAwards();
                    if (gNextQuest.arenaRecord == -1) {
                        sceneState->step = PLRES_TITLES_START;
                        sceneState->subStep = 0;
                        w->timeout = PLRES_PAGE_TIMEOUT;
                        w->closeState = PLRES_OPEN;
                    } else {
                        sceneState->step = PLRES_MONSTER;
                        sceneState->subStep = 0;
                        PlselRes_InitMonsterPage();
                    }
                } else {
                    sceneState->step = PLRES_MONEY;
                    sceneState->subStep = 0;
                    Plsel_ComputeAllRewards();
                }
                gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
                FadeA_Start(FADE_IN_BLACK);
                sceneState->timer = 30;
            }
        }
        break;
    case PLRES_MONEY:
        i = PlselRes_RewardPageInput(sceneState);
        if (sceneState->timer != 0) {
            sceneState->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sceneState->step++;
                FadeA_Start(FADE_OUT_BLACK);
                sceneState->timer = 30;
            }
        }
        Plsel_DrawResultPage(0);
        break;
    case PLRES_MONEY_OUT:
        if (--sceneState->timer <= 0) {
            w->timeout = PLRES_PAGE_TIMEOUT;
            sceneState->step = PLRES_POINTS_START;
            sceneState->subStep = 0;
        } else {
            Plsel_DrawResultPage(0);
        }
        break;
    case PLRES_POINTS_START:
        if (Game_IsOnline() != 1) {
            sceneState->step = PLRES_TITLES_START;
        } else {
            sceneState->step = PLRES_POINTS;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
            PlselRes_InitPointsPage();
            FadeA_Start(FADE_IN_BLACK);
            sceneState->timer = 30;
            gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
            if (w->rankChange == 1) {
                Stream_Play(1, 29);
            }
        }
        break;
    case PLRES_POINTS:
        i = PlselRes_PointsPageInput(sceneState);
        if (sceneState->timer != 0) {
            sceneState->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sceneState->step++;
                FadeA_Start(FADE_OUT_BLACK);
                sceneState->timer = 30;
            }
        }
        Plsel_DrawResultPage(0);
        break;
    case PLRES_POINTS_OUT:
        if (--sceneState->timer <= 0) {
            sceneState->step = PLRES_TITLES_START;
            sceneState->subStep = 0;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
        } else {
            Plsel_DrawResultPage(0);
        }
        break;
    case PLRES_TITLES_START:
        if (w->awardCount == 0) {
            sceneState->step = PLRES_RECORDS_START;
            sceneState->subStep = 0;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
            gNetMenuDrawHooks[0].draw = NULL;
            FadeA_Start(FADE_IN_BLACK);
        } else {
            sceneState->step = PLRES_TITLES;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
            Plsel_BuildTitleText();
            gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
            FadeA_Start(FADE_IN_BLACK);
            sceneState->timer = 30;
            Stream_Play(1, 29);
        }
        break;
    case PLRES_TITLES:
        i = PlselRes_TitlePageInput();
        if (sceneState->timer != 0) {
            sceneState->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sceneState->step++;
                FadeA_Start(FADE_OUT_BLACK);
                sceneState->timer = 30;
            }
        }
        Plsel_DrawTitleText();
        break;
    case PLRES_TITLES_OUT:
        if (--sceneState->timer <= 0) {
            sceneState->step = PLRES_RECORDS_START;
            sceneState->subStep = 0;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
            gNetMenuDrawHooks[0].draw = NULL;
        } else {
            Plsel_DrawTitleText();
        }
        break;
    case PLRES_RECORDS_START:
        if (w->recordGroupCount == 0) {
            sceneState->step = PLRES_NOTICES_START;
            sceneState->subStep = 0;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
            FadeA_Start(FADE_IN_BLACK);
        } else {
            PlselRes_InitRecordPage();
            sceneState->step = PLRES_RECORDS;
            sceneState->subStep = 0;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
            FadeA_Start(FADE_IN_BLACK);
            sceneState->timer = 30;
            gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
        }
        break;
    case PLRES_RECORDS:
        i = PlselRes_RecordPageInput(sceneState);
        if (sceneState->timer != 0) {
            sceneState->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sceneState->step++;
                FadeA_Start(FADE_OUT_BLACK);
                sceneState->timer = 30;
            }
        }
        Plsel_DrawResultPage(0);
        break;
    case PLRES_RECORDS_OUT:
        if (--sceneState->timer <= 0) {
            sceneState->step = PLRES_NOTICES_START;
            sceneState->subStep = 0;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
        } else {
            Plsel_DrawResultPage(0);
        }
        break;
    case PLRES_REWARD_LIST_START:
        sceneState->step = PLRES_REWARD_LIST;
        break;
    case PLRES_REWARD_LIST:
        if (Plsel_RewardListTask(sceneState) != 0) {
            sceneState->step = PLRES_TITLES_START;
            sceneState->subStep = 0;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
        } else {
            Plsel_RewardListDrawText(sceneState);
        }
        break;
    case PLRES_NOTICES_START:
        if (Plsel_HasNotices() == 0) {
            w->timeout = PLRES_PAGE_TIMEOUT;
            sceneState->timer = 30;
            Plsel_FadeOutMusic();
            sceneState->step = PLRES_EXIT;
            gNetMenuDrawHooks[0].draw = NULL;
        } else {
            Plsel_BuildNoticeText();
            sceneState->step = PLRES_NOTICES;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
            sceneState->timer = 30;
            FadeA_Start(FADE_IN_BLACK);
            gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
        }
        break;
    case PLRES_NOTICES:
        i = PlselRes_NoticePageInput();
        if (sceneState->timer != 0) {
            sceneState->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sceneState->step = PLRES_NOTICES_OUT;
                sceneState->timer = 30;
                FadeA_Start(FADE_BLACK);
            }
        }
        Plsel_DrawNoticeText();
        break;
    case PLRES_NOTICES_OUT:
        if (sceneState->timer != 0) {
            sceneState->timer--;
            Plsel_DrawNoticeText();
        } else {
            sceneState->timer = 30;
            w->timeout = PLRES_PAGE_TIMEOUT;
            Plsel_FadeOutMusic();
            sceneState->step = PLRES_EXIT;
        }
        break;
    case PLRES_ARENA_MENU:
        if (Plsel_SaveRecordMenu() <= 0) {
            sceneState->step++;
            FadeA_Start(FADE_OUT_BLACK);
            sceneState->timer = 30;
        }
        break;
    case PLRES_ARENA_OUT:
        if (--sceneState->timer <= 0) {
            if (Quest_GetOutcome(1) == 1) {
                Plsel_InitItemBoxMenu();
                sceneState->step = PLRES_ITEMBOX;
                sceneState->subStep = 0;
                FadeA_Start(FADE_IN_BLACK);
                sceneState->timer = 30;
            } else {
                Plsel_LoadResultsTextures();
                sceneState->step = PLRES_MONEY;
                sceneState->subStep = 0;
                Save_StorePouch(&gPlayers[sceneState->localPlayer]);
                Plsel_ComputeAllRewards();
                gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_DrawResultsBg;
                sceneState->timer = 30;
                FadeA_Start(FADE_IN_BLACK);
            }
        }
        break;
    case PLRES_MONSTER:
        i = PlselRes_MonsterPageInput(sceneState);
        if (sceneState->timer != 0) {
            sceneState->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sceneState->step++;
                FadeA_Start(FADE_OUT_BLACK);
                sceneState->timer = 30;
            }
        }
        Plsel_DrawResultPage(0);
        break;
    case PLRES_MONSTER_OUT:
        if (--sceneState->timer <= 0) {
            sceneState->step = PLRES_TITLES_START;
            sceneState->subStep = 0;
            w->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
        } else {
            Plsel_DrawResultPage(0);
        }
        break;
    case PLRES_QUIT_NOTICE:
        if (--sceneState->timer <= 0) {
            sceneState->step++;
            FadeA_Start(FADE_OUT_BLACK);
        }
        PlselRes_DrawQuitNotice();
        break;
    case PLRES_QUIT_OUT:
        if (--sceneState->timer <= 0) {
            sceneState->scene = SCENE_EXIT;
            sceneState->step = PLRES_START;
            Game_ResetSubsystems();
            return;
        }
        PlselRes_DrawQuitNotice();
        break;
    case PLRES_EXIT:
        if (--sceneState->timer <= 0) {
            gSysSettings.openingAfterResults = 0;
            if (Plsel_ShouldPlayOpening() == 1) {
                gSysSettings.openingAfterResults = 1;
                sceneState->scene = SCENE_OPENING;
                sceneState->step = PLRES_START;
                sceneState->subStep = 0;
                Game_ResetSubsystems();
                return;
            }
            if (Plsel_ShouldPlayEnding() == 1) {
                sceneState->scene = SCENE_OPENING;
                sceneState->step = PLRES_START;
                sceneState->subStep = 1;
                Game_ResetSubsystems();
                return;
            }
            sceneState->scene = SCENE_EXIT;
            sceneState->step = PLRES_START;
            Game_ResetSubsystems();
            return;
        }
        break;
    }
    if (gNetMenuDrawHooks[0].draw != NULL) {
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
    }
}

/*
 * Draws the five lines of the notice shown when the quest ended with
 * QUEST_PHASE_QUIT.
 */
void PlselRes_DrawQuitNotice(void)
{
    Text_SetColor(0);
    Font_SetSpacing(18, 18);
    Font_SetPos(275, 97);
    Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_QUIT_NOTICE + 0]);
    Font_SetPos(176, 133);
    Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_QUIT_NOTICE + 1]);
    Font_SetPos(176, 151);
    Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_QUIT_NOTICE + 2]);
    Font_SetPos(176, 169);
    Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_QUIT_NOTICE + 3]);
    Font_SetPos(176, 187);
    Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_QUIT_NOTICE + 4]);
}

/*
 * Opens the item box menu: its state cleared, IBOX_TIME_LIMIT frames on its
 * clock, the help of its page hidden, and its window sliding in.
 */
void Plsel_InitItemBoxMenu(void)
{
    *(s32 *)&gPlselItemBox = 0;
    gPlselItemBox.sold = 0;
    gPlselItemBox.entryCount = 2;
    gPlselItemBox.noRoom = -1;
    gPlselItemBox.blinkTimer = 255;
    gPlselItemBox.timeLeft = IBOX_TIME_LIMIT;
    gPlselItemBox.boxCursor = 0;
    gHelpShow = 0;
    gHelpPage = HELP_PAGE_RESULTS_MENU;
    gHelpColumn = 0;
    Shake_Start(SHAKE_RESULTS_MENU, 0, -180.0f);
    Hud_ResetMenuState();
}

/* The item box menu after a quest (the box items to the pouch, selling). */
s16 Plsel_ItemBoxMenu(void)
{
    PlselShop *menu;
    Player *pl;
    u16 padLeft;
    u16 item;
    u16 pad;
    s16 room;
    ItemStack boxStack;

    pl = &((Player *)gPlayers)[gScene.localPlayer];
    menu = (PlselShop *)&gPlselItemBox;
    gPlselItemBox.timeLeft--;
    pad = gPadState[0].pressed | (gPadState[0].repeat & PAD_DPAD);
    pad |= Menu_ReadDpad(0);
    switch (gPlselItemBox.state) {
    case 0:
        if (Shake_Update(SHAKE_RESULTS_MENU) == 1) {
            pad &= 0x3C40;
        }
        Menu_CursorUpDown(&menu->t.cursor, pad, menu->t.entryCount);
        if (pad & PAD_CIRCLE) {
            switch (menu->t.cursor) {
            case 0:
                gHelpShow = 1;
                gHelpLine = 0;
                break;
            case 1:
                if (Quest_IsDrill() == 1) {
                    PlselRes_PlayDecideSe();
                    menu->t.state = IBOX_DONE;
                    return 0;
                }
                if (menu->t.sold == 0) {
                    gHelpShow = 1;
                    gHelpLine = 3;
                    menu->t.sellChoice = 1;
                    break;
                }
                menu->t.state = IBOX_DONE;
                return 0;
            case 2:
                if (menu->t.timeLeft < 300) {
                    menu->t.timeLeft = 300;
                }
                break;
            }
            menu->t.state++;
            menu->t.infoWindow = IBOX_INFO_NONE;
            menu->t.subStep = 0;
            PlselRes_PlayDecideSe();
        }
        break;
    case 1:
        switch (menu->t.cursor) {
        case 0:
            switch (menu->t.subStep) {
            case 0:
                gHelpLine = 0;
                padLeft = Plsel_ItemBoxHandlePad(&menu->t, pad);
                if (padLeft & PAD_CROSS) {
                    break;
                }
                item = gScene.supply[menu->t.boxCursor].item;
                room = Pouch_Space(pl, item);
                if (room == 255 || room == 0) {
                    menu->t.noRoom = 0;
                } else {
                    menu->t.noRoom = -1;
                }
                if (padLeft & PAD_CIRCLE) {
                    if (item != 0 && menu->t.noRoom < 0) {
                        switch (Pouch_Add(pl, item, gScene.supply[menu->t.boxCursor].count)) {
                        case 0:
                        case 1:
                            gScene.supply[menu->t.boxCursor].item = 0;
                            gScene.supply[menu->t.boxCursor].count = 0;
                            Sound_PlaySeFlat(0, SE_SYS_DONE, 0);
                            break;
                        case 2:
                            gScene.supply[menu->t.boxCursor].count -= room;
                            Sound_PlaySeFlat(0, SE_SYS_DONE, 0);
                            break;
                        case 3:
                            Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                            break;
                        case 5:
                            gHelpLine = 1;
                            menu->t.pouchCursor = 0;
                            menu->t.subStep = 1;
                            menu->t.infoWindow = IBOX_INFO_NONE;
                            Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                            break;
                        }
                    } else {
                        Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                    }
                }
                break;
            case 1:
                if (menu->t.infoWindow != IBOX_INFO_NONE) {
                    if (pad & (PAD_SQUARE | PAD_CROSS)) {
                        menu->t.infoWindow = IBOX_INFO_NONE;
                        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                    }
                    pad &= ~0x40;
                } else {
                    if (pad & PAD_SQUARE) {
                        menu->t.infoWindow = IBOX_INFO_POUCH;
                        Sound_PlaySeFlat(0, 9, 0);
                    }
                    if (pad & PAD_CROSS) {
                        gHelpLine = 0;
                        menu->t.subStep = 0;
                        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                        break;
                    }
                }
                gHelpLine = 1;
                Pouch_MoveCursor(&menu->t.pouchCursor, pad);
                if (pad & PAD_CIRCLE) {
                    /* the box as u16 pairs, as the plsel loops read it: &gScene.supply[k]
                     * folds the offset 0x140 into the loads and stores */
                    ItemStack *boxSlot = (ItemStack *)((u16 *)gScene.supply + menu->t.boxCursor * 2);

                    boxStack = *boxSlot;
                    *boxSlot = pl->pouch[menu->t.pouchCursor];
                    Pouch_ClearSlot(pl, menu->t.pouchCursor);
                    Pouch_Add(pl, boxStack.item, boxStack.count);
                    gHelpLine = 2;
                    menu->t.subStep++;
                    menu->t.infoWindow = IBOX_INFO_NONE;
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                }
                break;
            case 2:
                if (pad & PAD_CIRCLE) {
                    gHelpLine = 0;
                    menu->t.blinkTimer = 255;
                    menu->t.subStep = 0;
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                } else {
                    gHelpLine = 2;
                    menu->t.blinkTimer++;
                }
                break;
            }
            break;
        case 1:
            switch (menu->t.subStep) {
            case 0:
                gHelpLine = 3;
                if ((menu->t.sellChoice == 0 && (pad & PAD_RIGHT)) || (menu->t.sellChoice == 1 && (pad & PAD_LEFT))) {
                    menu->t.sellChoice ^= 1;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    break;
                }
                if (pad & PAD_CROSS) {
                /* The cross button's code, which the sale's "no" shares:
                 * the game's code has it once and jumps to it. */
                cancel:
                    gHelpShow = 0;
                    menu->t.state = IBOX_LIST;
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else if (pad & PAD_CIRCLE) {
                    if (menu->t.sellChoice == 1) {
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        goto cancel;
                    }
                    if (menu->t.sellChoice == 0) {
                        if (Save_HasTitle(30) == 0) {
                            menu->flag = 1;
                        } else {
                            menu->flag = 0;
                        }
                        Save_AddMoney(menu->money);
                        if (Save_HasTitle(30) == 0) {
                            menu->flag = 0;
                        }
                        menu->t.subStep = 1;
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    }
                }
                break;
            case 1:
                menu->t.timeLeft = 0;
                break;
            }
            break;
        case 2:
            menu->t.state = IBOX_DONE;
            break;
        }
        break;
    case 2:
        menu->t.timeLeft = 0;
        break;
    }
    return menu->t.timeLeft;
}

/*
 * Moves an item box cursor (column | row << 3: 8 columns, 6 rows) with the
 * D-pad, wrapping both ways; plays SE_SYS_SCROLL when it moved.
 */
void Plsel_MoveGridCursor(u8 *cursor, u16 pad)
{
    u8 old;
    u8 x;
    u8 y;
    u8 next;

    old = *cursor;
    x = old;
    y = old >> 3;
    if (pad & PAD_LEFT) {
        x--;
    }
    if (pad & PAD_RIGHT) {
        x++;
    }
    if (pad & PAD_UP) {
        y--;
    }
    if (pad & PAD_DOWN) {
        y++;
    }
    if (y == 255) {
        y = 5;
    }
    next = (x & 7) | ((y % 6) << 3);
    if (old != next) {
        *cursor = next;
        Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
    }
}

/*
 * The buttons on the box grid: square opens the description of the box
 * item, and square or cross closes it; otherwise cross leaves the grid and
 * returns PAD_CROSS. Else moves the box cursor and returns `pad`.
 */
u16 Plsel_ItemBoxHandlePad(PlselItemBoxMenu *menu, u16 pad)
{
    if (menu->infoWindow != IBOX_INFO_NONE) {
        if (pad & (PAD_SQUARE | PAD_CROSS)) {
            menu->infoWindow = IBOX_INFO_NONE;
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        }
        pad &= ~PAD_CROSS;
    } else if (pad & PAD_SQUARE) {
        menu->infoWindow = IBOX_INFO_BOX;
        Sound_PlaySeFlat(0, 9, 0);
    }
    if (pad & PAD_CROSS) {
        gHelpShow = 0;
        menu->state = 0;
        Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
        return PAD_CROSS;
    }
    Plsel_MoveGridCursor(&menu->boxCursor, pad);
    return pad;
}

/*
 * Draws the item box menu: the help or description window, the box grid and
 * the pouch as the step needs them, the entries, the time left in seconds
 * and, outside a drill, the hunter's money.
 */
void Plsel_DrawItemBoxMenu(void)
{
    PlselItemBoxMenu *menu;
    Player *pl;
    s32 id;
    char buf[64];
    ListPanel win;

    menu = &gPlselItemBox;
    pl = &((Player *)gPlayers)[gScene.localPlayer];
    id = (s32) Shake_GetOffset(SHAKE_RESULTS_MENU);
    win = gPlselItemBoxWin;
    if (Quest_IsDrill() == 1) {
        win.items = &gHelpTexts_6[IBOX_TEXT_DRILL_ENTRIES];
    }
    switch (menu->infoWindow) {
    case 0:
        if (gHelpShow != 0) {
            HelpWindow_DrawCurrent();
            if ((menu->blinkTimer & 0x1F) < 20) {
                Draw_Icons(gPlselItemBoxNextIcon, 1);
            }
        }
        break;
    case 1:
        HelpWindow_Draw(1, gScene.supply[menu->boxCursor].item + 24, gHelpColumn);
        break;
    case 2:
        HelpWindow_Draw(1, pl->pouch[menu->pouchCursor].item + 24, gHelpColumn);
        break;
    }
    switch (menu->state) {
    case 0:
        win.cursorColor = 0xA9182;
        break;
    case 1:
        win.cursorColor = 0x808080;
        switch (menu->cursor) {
        case 0:
            switch (menu->subStep) {
            case 0:
                Plsel_DrawItemBox(menu->boxCursor, 0, menu->noRoom);
                if (menu->infoWindow == 0) {
                    Draw_Icons(gPlselItemBoxHintIcon, 1);
                }
                break;
            case 1:
                Plsel_DrawItemBox(menu->boxCursor, 1, menu->noRoom);
                ItemList_DrawPouch(menu->pouchCursor, 0xA9182, 8);
                if (menu->infoWindow == 0) {
                    Draw_Icons(gPlselItemBoxHintIcon, 1);
                }
                break;
            case 2:
                Plsel_DrawItemBox(menu->boxCursor, 1, menu->noRoom);
                ItemList_DrawPouch(menu->pouchCursor, 0x808080, 0);
                break;
            }
            break;
        case 1:
            Plsel_DrawItemBox(-1, 0, -1);
            break;
        case 2:
            break;
        }
        break;
    case 2:
        break;
    }
    ListPanel_Draw(&win, (s16 *)id, (char *)0, menu->cursor);
    sprintf(buf, gPlselTexts[PLSEL_TEXT_ITEMBOX_TIME], menu->timeLeft / 30);
    Window_DrawText(&gPlselItemBoxTimeWin, (s16 *)id, (u8 *)buf);
    if (Quest_IsDrill() == 0) {
        Plsel_DrawMoney();
    }
}

/*
 * Sums the sell value (value x count) of the supply box's slots into
 * gPlselItemBoxSale[0], what the sale pays.
 */
void Plsel_ComputeItemBoxValue(void)
{
    s32 i;
    u16 *slot;

    gPlselItemBoxSale[0] = 0;
    slot = (u16 *)gScene.supply;
    for (i = 0; i < 48; i++, slot += 2) {
        if (slot[0] != 0) {
            if (slot[0] != 0) {
                gPlselItemBoxSale[0] += gItemData[slot[0]].value * (s16)slot[1];
            }
        }
    }
}

/*
 * Draws the hunter's money (gSave.money) at the top right, over its strip,
 * with the coin icon.
 */
void Plsel_DrawMoney(void)
{
    char num[32];
    char txt[32];
    Sprite icon;
    s32 i;
    s16 x;

    sprintf(num, gHelpTexts_6[IBOX_TEXT_MONEY_FMT], gSave.money);
    Sjis_Widen(num, txt);
    Font_SetSpacing(18, 18);
    Text_SetColor(1);
    Font_SetPos(442, 18);
    Text_Draw(txt);
    Text_SetColor(0);
    Font_SetPos(440, 16);
    Text_Draw(txt);
    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 341);
    Tex_Bind(341);
    icon.u0 = 155;
    icon.v0 = 132;
    icon.u1 = 178;
    icon.v1 = 139;
    icon.w = 24;
    icon.h = 8;
    for (x = 400, i = 0; i < 8; i++, x += 24) {
        icon.x = x + 3;
        icon.y = 32;
        icon.color = 0xFF000000;
        Sprite_Draw(&icon);
        icon.x = x;
        icon.y = 30;
        icon.color = 0xFF806000;
        Sprite_Draw(&icon);
    }
    icon.x = 410;
    icon.y = 10;
    icon.w = 24;
    icon.h = 24;
    icon.color = 0xFFE0E000;
    icon.u0 = 103;
    icon.v0 = 171;
    icon.u1 = 126;
    icon.v1 = 194;
    Sprite_Draw(&icon);
}

/* Returns the sell value of a stack (value x count), or -1 for an empty one. */
s32 Plsel_ItemStackValue(u16 *stack)
{
    if (stack[0] == 0) {
        return -1;
    }
    if (stack[0] == 0) {
        return -1;
    }
    return gItemData[stack[0]].value * (s16)stack[1];
}


struct Frame;
struct PrimLine;
extern u32 gItemColors[];
extern char *gItemNames[];

/* Draws the item box grid (48 slots), the item under the cursor with its
 * name and count, and the selling window. */
void Plsel_DrawItemBox(s16 sel, u8 dim, s8 flag)
{
    PlselShop *menu;
    PrimLine cell;
    Sprite iconSprite;
    char buf[48];
    char num[32];
    ItemStack *stack;
    s16 i;
    u16 *slot;
    s32 x;
    u8 alpha;
    ItemData *itemData;
    s32 k;
    s32 value;

    menu = (PlselShop *)&gPlselItemBox;
    x = dim == 0 ? 255 : 96;
    alpha = x;
    WindowFrame_Draw(&gPlselItemBoxFrame, NULL);
    cell.color = 0xFF200000;
    for (i = 0; i < 48; i++) {
        cell.x0 = (s16)(313.0f + 36.0f * (i & 7)) + 4;
        cell.x1 = cell.x0 + 32;
        cell.y0 = (i >> 3) * 32 + 66;
        cell.y1 = cell.y0 + 28;
        Prim_FillRect(&cell);
    }
    Gfx_SetTexFilter(GFX_TEX_NEAREST);
    Tex_Prepare(1, 324);
    Tex_Bind(324);
    iconSprite.w = 40;
    slot = (u16 *)gScene.supply;
    iconSprite.h = 32;
    for (i = 0; i < 48; i++, slot += 2) {
        iconSprite.x = 313.0f + 36.0f * (i & 7);
        iconSprite.y = (i >> 3) * 32 + 64;
        if (slot[0] != 0) {
            itemData = &gItemData[slot[0]];
            if (itemData->icon != 255) {
                k = itemData->icon + 1;
                iconSprite.u0 = (k & 7) * 32 + 1;
                iconSprite.v0 = (k >> 3) * 32 + 1;
                iconSprite.u1 = iconSprite.u0 + 30;
                iconSprite.v1 = iconSprite.v0 + 30;
                iconSprite.color = gItemColors[itemData->color];
                iconSprite.color = ((u32)iconSprite.color & 0xFFFFFF) | (alpha << 24);
                Sprite_Draw(&iconSprite);
            }
        }
    }
    if (sel >= 0) {
        iconSprite.x = 313.0f + 36.0f * (sel & 7);
        iconSprite.y = (sel >> 3) * 32 + 64;
        *(s32 *)&iconSprite.u0 = 0;
        *(s32 *)&iconSprite.u1 = 0x200020;
        iconSprite.color = (alpha << 24) | 0xFFFFFF;
        Sprite_Draw(&iconSprite);
        Font_SetSpacing(18, 18);
        if (dim == 0) {
            Text_SetColor(0);
        } else {
            Text_SetColor(10);
        }
        stack = &gScene.supply[sel];
        if (stack->item == 0) {
            sprintf(buf, gItemNames[0]);
        } else {
            switch (gItemData[stack->item].max) {
            case 1:
                sprintf(buf, gItemNames[stack->item]);
                break;
            case 255:
                sprintf(buf, gPlselTexts[PLSEL_TEXT_STACK_NAME], gItemNames[stack->item]);
                break;
            default:
                sprintf(buf, gPlselTexts[PLSEL_TEXT_STACK_COUNT], gItemNames[stack->item], stack->count);
                break;
            }
        }
        Font_SetPos(459 - Str_Length(buf) * 9 / 2, 288);
        Text_Draw(buf);
        if (Quest_IsDrill() == 0 && menu->t.state == IBOX_ENTRY && menu->t.cursor == IBOX_ENTRY_BOX) {
            value = Plsel_ItemStackValue((u16 *)stack);
            if (value >= 0) {
                Text_SetColor(0);
                sprintf(buf, gHelpTexts_6[IBOX_TEXT_VALUE_FMT], value);
                Sjis_Widen(buf, num);
                strcpy(buf, gHelpTexts_6[IBOX_TEXT_STACK_VALUE]);
                strcat(buf, num);
                Font_SetPos((s32)((306.0f - 9.0f * (Str_Length(buf) - 8)) / 2.0f) + 306, 312);
                Text_Printf((s8 *) buf);
            }
        }
        if (flag >= 0) {
            Text_SetColor(2);
            Font_SetPos(315, 264);
            Text_Draw(gPlselTexts[PLSEL_TEXT_NO_ROOM]);
        }
    }
    if (Quest_IsDrill() != 1 && menu->t.state == IBOX_ENTRY && menu->t.cursor == IBOX_ENTRY_SELL) {
        Text_SetColor(0);
        Plsel_ComputeItemBoxValue();
        sprintf(buf, gHelpTexts_6[IBOX_TEXT_VALUE_FMT], menu->money);
        Sjis_Widen(buf, num);
        strcpy(buf, gHelpTexts_6[IBOX_TEXT_SALE_TOTAL]);
        strcat(buf, num);
        Font_SetPos((s32)((306.0f - 9.0f * Str_Length(buf)) / 2.0f) + 306, 264);
        Text_Draw(buf);
        strcpy(buf, gHelpTexts_6[IBOX_TEXT_SELL_YES + menu->t.sellChoice]);
        Font_SetPos(316, 396);
        Text_Printf((s8 *) buf);
    }
}

/* "%s" comes here in the game's .rodata, before Plsel_CategoryIsShared's jump table,
 * although its first user is Plsel_BuildRewardLines: probably first used by a function
 * the linker dead-stripped. */
INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselStrFmt);

/* Whether reward category i is shared among the players (divided by
 * gScene.playerCount). */
static s32 Plsel_CategoryIsShared(u32 i)
{
    switch (i) {
    case 0:
    case 1:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return 1;
    default:
        return 0;
    }
}

/*
 * Returns the drill's payout (rewList.payout), or -1 when the quest isn't a
 * drill.
 */
s32 Plsel_DrillPayout(void)
{
    PlselWork *w;

    w = gPlselWork;
    if (Quest_IsDrill() == 0) {
        return -1;
    }
    return w->rewList.payout;
}


extern u8 gQuestStats[];
extern s16 gPlselKillRewards[];
/* unit's own declaration: declared here with u16 and u32 parameters; with the definition's (s32, s32) Plsel_ComputeRewards compiles differently */
void Dojo_SetBestTime(u16, u32);

/* Works out the quest rewards into the rewards list: the pouch sold, the
 * goals, the hunting categories and monsters, the time bonus and the pages
 * the list will show. The first loop over the 17 categories walks
 * gPlselRewardSteps and the rates through pointers (statId, catRate) and the
 * values by index: written so, the compiler advances its walkers in the
 * order the game's code does. */
void Plsel_ComputeRewards(Player *pl)
{
    s32 *catWalk;
    PlselList *list;
    u8 *questInfo;
    s32 i;
    s32 *values;
    u32 bestTime;
    s32 k;
    u8 *stats;
    PlselWork *w;
    s32 *goalReward;
    s16 *catRate;
    s16 *killRate;
    u16 *hunted;
    s8 *statId;

    w = gPlselWork;
    questInfo = (u8 *)gQuest.info;
    if (Quest_GetOutcome(1) != 1) {
        return;
    }
    w->rewList.pageCount[1] = 1;
    list = &w->rewList;
    w->rewList.pageCount[2] = 1;
    memcpy(list->pouch, pl->pouch, 80);
    values = list->val;
    for (i = 0; i < 20; i++) {
        if (list->pouch[i].item != 0 && list->pouch[i].count != 0 && !(gItemData[list->pouch[i].item].flags2 & 4)) {
            values[i] = list->pouch[i].count * gItemData[list->pouch[i].item].value;
            values[i] = (f32)values[i] / 4.0f;
            list->itemTotal += values[i];
        }
    }
    stats = gQuestStats;
    goalReward = list->goal;
    if (QuestGoal_IsDone(0) == 1) {
        *goalReward = gQuest.reward;
    }
    goalReward++;
    if (QuestGoal_IsDone(1) == 1) {
        *goalReward = *(s32 *)(questInfo + 20) / gScene.playerCount;
    }
    goalReward++;
    if (QuestGoal_IsDone(2) == 1) {
        *goalReward = *(s32 *)(questInfo + 24) / gScene.playerCount;
    }
    list->clearTime = *(s32 *)(stats + 20);
    list->parTime = Dojo_GetParMinutes(gQuest.stage) * 1800;
    if (list->clearTime <= list->parTime) {
        list->fastClear = 1;
    }
    bestTime = Dojo_GetBestTime(gQuest.stage);
    if (bestTime > (u32)list->clearTime || bestTime == 0) {
        list->newBestTime = 1;
        list->newBestTime2 = 1;
        Dojo_SetBestTime(gQuest.stage, list->clearTime);
    }
    values = list->catReward;
    catRate = gPlselRewardTbl;
    for (i = 0, statId = gPlselRewardSteps; i < 17; i++, catRate++, statId++) {
        if (*statId != -1) {
            k = stats[*statId];
            values[i] = k * *catRate;
            if (Plsel_CategoryIsShared(i) != 0) {
                values[i] /= gScene.playerCount;
            }
        }
    }
    hunted = w->huntedByGroup;
    values = list->catReward;
    killRate = gPlselKillRewards;
    for (i = 0; i < PLSEL_GROUP_COUNT; i++) {
        list->kill[i] = *hunted++ * *killRate++;
        list->kill[i] /= gScene.playerCount;
        *values += list->kill[i];
        list->killTotal += list->kill[i];
    }
    /* w->huntedByGroup[group of the marked monster], written as an offset
     * from w: indexing the array adds the two registers the other way
     * round, which the game's code doesn't. */
    if (*(u16 *)((u8 *)w + MonsterGroup_First(stats[16]) * 2 + 1124) != 0) {
        list->catReward[12] = 1;
    }
    k = *(u16 *)(stats + 18);
    if (k >= 13) {
        k++;
    }
    if (list->catReward[k] != 0) {
        list->catReward[13] = 1;
    }
    catWalk = list->catReward;
    for (i = 0; i < 17; i++) {
        if (i == 12 || i == 13) {
            catWalk++;
        } else {
            list->huntReward += *catWalk++;
        }
    }
    if (list->huntReward > 0) {
        if (list->catReward[12] != 0) {
            list->huntReward = list->huntReward * 200 / 100;
        }
        if (list->catReward[13] != 0) {
            list->huntReward = list->huntReward * 150 / 100;
        }
    }
    list->rewardTotal += list->goal[0];
    list->rewardTotal += list->goal[1];
    list->rewardTotal += list->goal[2];
    list->rewardTotal += list->huntReward;
    goalReward = &list->rewardTotal;
    if (list->fastClear != 0 && *goalReward > 0) {
        *goalReward *= 2;
    }
    list->payout = list->itemTotal + list->rewardTotal;
    list->payout = list->payout * gQuestSync.pointRatePct / 100;
    if (list->payout < 0) {
        list->payout = 0;
    }
    list->payoutCopy = list->payout;
    if (Dojo_GetBestScore(gQuest.stage) < list->payout) {
        list->newBestScore = 1;
        Dojo_SetBestScore(gQuest.stage, list->payout);
    }
    list->linesShown = 12;
    list->screen = 0;
    list->zenny = Save_GetPoints();
    list->pageCount[0] = 1;
    list->pages[list->pageCount[0] - 1] = 0;
    if (list->huntReward != 0) {
        list->pageCount[0]++;
        list->pages[list->pageCount[0] - 1] = 1;
    }
    if (list->catReward[0] != 0) {
        list->pageCount[0]++;
        list->pages[list->pageCount[0] - 1] = 2;
    }
    if (list->itemTotal != 0) {
        list->pageCount[0]++;
        list->pages[list->pageCount[0] - 1] = 3;
    }
}

/* Asks the page to close: PLRES_CLOSE_ASKED, with 30 frames of closing. */
void PlselRes_BeginClose(void)
{
    PlselWork *w;

    w = gPlselWork;
    PlselRes_EmptyCloseHook();
    w->closeState = PLRES_CLOSE_ASKED;
    w->unk06C2 = 0;
    w->closeTimer = 30;
}

extern char gPlselPadSpace[];
extern char gPlselValueSep[];
extern char gPlselNameCountFmt[];
extern char gPlselPointsFmt[];
extern char gPlselPercentMark[];

#define LINE(l) ((char *)&(l)->ent[(l)->lineCount])

/* Builds the text lines of the rewards list's current page. Case 2 reuses
 * case 1's table pointer and padding counter (tbl, padLen): with locals of its
 * own, its kill walker and counter took each other's saved registers. */
void Plsel_BuildRewardLines(PlselList *list)
{
    char buf[128];
    char wide[128];
    char **tbl;
    s32 k;
    char **str;
    s32 padLen;
    s32 *reward;
    s32 i;
    s32 n;
    s8 *statId;
    s32 group;
    PlselWork *w;
    char **texts;
    s32 itemPadLen;
    s32 totalSecs;
    s32 minutes;
    s32 secs;

    w = gPlselWork;
    for (i = 0; i < 80; i++) {
        list->ent[i].flag = 0;
    }
    list->unk1C4 = 0;
    list->lineCount = 0;
    list->scrollable = 0;
    switch (list->screen) {
    case 0:
        list->linesShown = 8;
        switch (list->pages[list->pageCur]) {
        case 0:
            tbl = &gPlselTexts[PLSEL_TEXT_REWARD_LIST];
            str = &gResultRuleText;
            for (i = 0; i < 4; i++) {
                switch (i) {
                case 0:
                    if (Quest_GetOutcome(0) != 0) {
                        sprintf(buf, tbl[2], list->goal[i]);
                        goto widen;
                    }
                    break;
                case 1:
                    if (gQuest.goals[1].flags & QUEST_GOAL_DONE) {
                        sprintf(buf, tbl[3], list->goal[i]);
                        goto widen;
                    }
                    break;
                case 2:
                    if (gQuest.goals[2].flags & QUEST_GOAL_DONE) {
                        sprintf(buf, tbl[4], list->goal[i]);
                    /* The line's widening, which the three goals share: the
                     * game's code has it once and jumps to it. */
                    widen:
                        Sjis_Widen(buf, LINE(list));
                        list->lineCount++;
                    }
                    break;
                case 3:
                    if (list->goal[i] != 0) {
                        sprintf(LINE(list), tbl[5]);
                        sprintf(buf, TXT(0x00D7BB70) /* the goal, then a full-width P (points) */, list->goal[i]);
                        Sjis_Widen(buf, wide);
                        strcat(LINE(list), wide);
                        list->lineCount++;
                    }
                    break;
                }
            }
            if (list->goal[i] != 0) {
                sprintf(LINE(list), tbl[6]);
                list->lineCount++;
            }
            list->lineCount++;
            sprintf(buf, tbl[7], list->itemTotal);
            Sjis_Widen(buf, LINE(list));
            list->lineCount++;
            for (n = list->lineCount; n < 10; n++) {
                list->lineCount++;
            }
            sprintf((char *)&list->ent[list->lineCount++], gPlselStrFmt, *str);
            sprintf(buf, tbl[8], list->itemTotal + list->rewardTotal);
            Sjis_Widen(buf, LINE(list));
            break;
        case 1:
            list->linesShown = 8;
            list->scrollable = 1;
            tbl = &gPlselTexts[PLSEL_TEXT_REWARD_CATEGORIES];
            list->markMonsterLine = -1;
            reward = list->catReward;
            k = 0;
            list->markHuntLine = -1;
            for (n = 0, str = tbl, statId = gPlselRewardSteps; n < 17; n++) {
                if (n != 13) {
                    if (*(u16 *)&gQuestStats[18] == k && *reward != 0) {
                        list->markHuntLine = list->lineCount;
                    }
                    k++;
                }
                switch (n) {
                case 0:
                    if (list->killTotal != 0) {
                        strcpy(LINE(list), *str);
                        sprintf(buf, "%d", list->killTotal);
                        Sjis_Widen(buf, wide);
                        if (list->killTotal > 0) {
                            strcpy(buf, tbl[20]);
                        } else {
                            strcpy(buf, tbl[19]);
                        }
                        strcat(buf, wide);
                        strcat(buf, tbl[21]);
                        strcat(buf, TXT(0x00D7BB80) /* a full-width P (points) */);
                        strcat(LINE(list), buf);
                        list->lineCount++;
                    }
                    break;
                case 1:
                case 10:
                    if (*reward != 0) {
                        strcpy(LINE(list), *str);
                        sprintf(buf, "%d", *reward);
                        Sjis_Widen(buf, wide);
                        strcpy(buf, tbl[20]);
                        strcat(LINE(list), buf);
                        sprintf(wide, "%d", *reward);
                        Sjis_Widen(wide, buf);
                        strcat(LINE(list), buf);
                        strcat(LINE(list), tbl[21]);
                        strcat(LINE(list), TXT(0x00D7BB80) /* a full-width P (points) */);
                        list->lineCount++;
                    }
                    break;
                case 2:
                case 14:
                case 15:
                case 16:
                case 3:
                    if (*reward != 0) {
                        sprintf(buf, *str, gQuest.stats[*statId]);
                        Sjis_Widen(buf, wide);
                        if ((padLen = Str_Length(wide)) < 28) {
                            do {
                                strcat(wide, gPlselPadSpace);
                            } while (++padLen < 28);
                        }
                        strcpy(LINE(list), wide);
                        strcat(LINE(list), gPlselValueSep);
                        if (n == 2 || n == 3) {
                            strcpy(buf, tbl[19]);
                        } else {
                            strcpy(buf, tbl[20]);
                        }
                        strcat(LINE(list), buf);
                        sprintf(wide, "%d", *reward);
                        Sjis_Widen(wide, buf);
                        strcat(LINE(list), buf);
                        strcat(LINE(list), tbl[21]);
                        strcat(LINE(list), TXT(0x00D7BB80) /* a full-width P (points) */);
                        list->lineCount++;
                    }
                    break;
                case 12:
                case 13:
                    break;
                default:
                    if (*reward != 0) {
                        strcpy(LINE(list), *str);
                        sprintf(buf, "%d", *reward);
                        Sjis_Widen(buf, wide);
                        if (n == 11) {
                            strcpy(buf, tbl[19]);
                        } else if (n >= 4 && n < 10) {
                            strcpy(buf, tbl[20]);
                        } else {
                            buf[0] = 0;
                        }
                        strcat(buf, wide);
                        if (n == 11 || (n >= 4 && n < 10)) {
                            strcat(buf, tbl[21]);
                        }
                        strcat(LINE(list), buf);
                        strcat(LINE(list), TXT(0x00D7BB80) /* a full-width P (points) */);
                        list->lineCount++;
                    }
                    break;
                }
                reward++;
                str++;
                statId++;
            }
            if (list->catReward[12] != 0 && list->huntReward > 0) {
                list->lineCount++;
                sprintf(LINE(list), tbl[12]);
                list->lineCount++;
            }
            if (list->catReward[13] != 0 && list->huntReward > 0) {
                if (list->catReward[12] == 0) {
                    list->lineCount++;
                }
                sprintf(LINE(list), tbl[13]);
                list->lineCount++;
            }
            break;
        case 2:
            list->scrollable = 1;
            list->linesShown = 8;
            tbl = &gPlselTexts[PLSEL_TEXT_REWARD_CATEGORIES];
            list->markMonsterLine = -1;
            for (group = 0; group < PLSEL_GROUP_COUNT; group++) {
                if (list->kill[group] != 0) {
                    if (*(u16 *)&gQuestStats[16] == (u16)group) {
                        list->markMonsterLine = list->lineCount;
                    }
                    sprintf(buf, gPlselNameCountFmt, gMonsterNames[group], w->huntedByGroup[group]);
                    Sjis_Widen(buf, wide);
                    if ((padLen = Str_Length(wide)) < 30) {
                        do {
                            strcat(wide, gPlselPadSpace);
                        } while (++padLen < 30);
                    }
                    strcpy(buf, wide);
                    strcpy(LINE(list), buf);
                    strcat(LINE(list), gPlselValueSep);
                    if (list->kill[group] < 0) {
                        strcat(LINE(list), tbl[19]);
                    } else {
                        strcat(LINE(list), tbl[20]);
                    }
                    sprintf(wide, "%d", list->kill[group]);
                    Sjis_Widen(wide, buf);
                    strcat(LINE(list), buf);
                    strcat(LINE(list), tbl[21]);
                    strcat(LINE(list), TXT(0x00D7BB80) /* a full-width P (points) */);
                    list->lineCount++;
                }
            }
            break;
        case 3:
            list->scrollable = 1;
            list->linesShown = 8;
            for (i = 0; i < 20; i++) {
                if (list->val[i] != 0 && list->pouch[i].item != 0 && list->pouch[i].count != 0) {
                    sprintf(buf, gPlselNameCountFmt, gItemNames[list->pouch[i].item], list->pouch[i].count);
                    Sjis_Widen(buf, wide);
                    if ((itemPadLen = Str_Length(wide)) < 30) {
                        do {
                            strcat(wide, gPlselPadSpace);
                        } while (++itemPadLen < 30);
                    }
                    strcpy(LINE(list), wide);
                    sprintf(wide, gPlselPointsFmt, list->val[i]);
                    Sjis_Widen(wide, buf);
                    strcat(LINE(list), buf);
                    list->lineCount++;
                }
            }
            break;
        }
        break;
    case 2:
        list->linesShown = 8;
        list->scrollable = 0;
        texts = &gPlselTexts[PLSEL_TEXT_REWARD_LIST];
        if (list->clearTime == 0) {
            minutes = 0;
            secs = 0;
        } else {
            totalSecs = list->clearTime / 30;
            minutes = totalSecs / 60;
            secs = totalSecs;
            secs -= minutes * 60;
        }
        sprintf(buf, texts[12], minutes, secs);
        Sjis_Widen(buf, LINE(list));
        if (list->newBestTime != 0) {
            strcat(LINE(list), texts[0]);
        }
        list->lineCount++;
        strcpy((char *)&list->ent[list->lineCount++], gResultRuleText);
        sprintf(buf, texts[9], list->itemTotal + list->rewardTotal);
        Sjis_Widen(buf, LINE(list));
        list->lineCount++;
        sprintf(buf, texts[13], gQuestSync.pointRatePct);
        Sjis_Widen(buf, LINE(list));
        strcat(LINE(list), gPlselPercentMark);
        list->lineCount += 4;
        sprintf(buf, texts[14], list->payout);
        Sjis_Widen(buf, LINE(list));
        if (list->newBestScore != 0) {
            strcat(LINE(list), texts[0]);
        }
        break;
    }
}

INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselPadSpace);

INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselValueSep);

INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselNameCountFmt);

INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselPointsFmt);

INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselPercentMark);

/*
 * One frame of the drill's rewards list (PLRES_REWARD_LIST; task is gScene):
 * left and right turn the pages, up and down scroll them; circle, or the
 * timeout of 3600 frames (2 minutes), pays the points (Save_AddPoints) and
 * shows the total, and the next circle closes it. Returns 1 once it has
 * faded out.
 */
s32 Plsel_RewardListTask(SceneState *task)
{
    PlselList *list;
    u16 padRepeat;
    u16 trg;
    PlselWork *w;
    u16 pad;

    w = gPlselWork;
    trg = gPadState[0].held;
    list = &w->rewList;
    pad = gPadState[0].pressed;
    padRepeat = gPadState[0].repeat | pad;
    if (w->closeState == PLRES_CLOSE_ASKED) {
        PlselRes_Close();
    }
    switch (task->subStep) {
    case 0:
        task->timeout = PLRES_PAGE_TIMEOUT;
        gNetMenuDrawHooks[0].draw = (DrawSlotFn)Plsel_RewardListDraw;
        list->pageCur = 0;
        list->topLine = 0;
        Plsel_BuildRewardLines(list);
        list->unk1A44 = 10;
        list->revealLine = 12;
        task->timer2 = 20;
        task->subStep++;
    case 1:
        if (task->timer2 != 0) {
            task->timer2--;
            break;
        }
    /* The frame count and the arrows' bob, which the reveal shares once it
     * is done: the game's code has it once and jumps to it. */
    common:
        list->frame++;
        if (!(list->frame & 3)) {
            list->arrowBob ^= 1;
        }
        if (task->timeout != 0) {
            task->timeout--;
        }
        if (task->subStep < 4 && ((pad & PAD_CIRCLE) || task->timeout == 0)) {
            PlselRes_PlayDecideSe();
            PlselRes_BeginClose();
            task->subStep = 10;
            break;
        }
        if (list->pageCount[0] >= 2) {
            if (list->pageCount[0] > 1) {
                if (pad & PAD_RIGHT) {
                    PlselRes_PlayPageSe();
                    if (list->pageCur >= list->pageCount[0] - 1) {
                        list->pageCur = 0;
                    } else {
                        list->pageCur++;
                    }
                    list->topLine = 0;
                    Plsel_BuildRewardLines(list);
                    break;
                }
                if (pad & PAD_LEFT) {
                    PlselRes_PlayPageSe();
                    if (list->pageCur > 0) {
                        list->pageCur--;
                    } else {
                        list->pageCur = list->pageCount[0] - 1;
                    }
                    list->topLine = 0;
                    Plsel_BuildRewardLines(list);
                    break;
                }
            }
            if (list->pageCur == 0) {
                break;
            }
        }
        if ((padRepeat & PAD_DOWN) && list->lineCount > list->topLine + list->linesShown) {
            PlselRes_PlayPageSe();
            list->topLine++;
        } else if ((padRepeat & PAD_UP) && list->topLine > 0) {
            PlselRes_PlayPageSe();
            list->topLine--;
        }
        break;
    case 2:
        if (task->timer2 == 0) {
            if (list->pageCur != 0 || list->revealLine == list->lineCount) {
                goto common;
            }
            list->revealLine++;
            while (list->ent[list->revealLine].flag == 0) {
                if (list->revealLine == list->lineCount) {
                    break;
                }
                list->revealLine++;
            }
            task->timer2 = 20;
        } else {
            task->timer2--;
        }
        break;
    case 3:
        list->doneShown = 1;
        if (task->timer2 > 0) {
            task->timer2--;
            break;
        }
        if (task->timeout != 0) {
            task->timeout--;
        }
        if ((trg & PAD_CIRCLE) || task->timeout == 0) {
            PlselRes_PlayDecideSe();
            PlselRes_BeginClose();
            task->subStep++;
        }
        break;
    case 4:
        if (--w->closeTimer <= 0) {
            FadeA_Start(FADE_OUT_BLACK);
            task->timer2 = 30;
            task->subStep++;
        }
        break;
    case 5:
        if (--task->timer2 <= 0) {
            return 1;
        }
        break;
    case 10:
        if (--w->closeTimer <= 0) {
            task->subStep = 3;
            list->screen = 2;
            list->pageCur = 0;
            list->revealLine = 0;
            list->zenny += list->payout;
            Save_AddPoints(list->payout);
            Plsel_BuildRewardLines(list);
            task->timer2 = 20;
            task->timeout = PLRES_PAGE_TIMEOUT;
            w->closeState = PLRES_OPEN;
        }
        break;
    }
    return 0;
}

INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselArrowUp);

INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselArrowDown);

/*
 * Draw hook of the rewards list: the results background and, on the reward
 * pages, the scroll marks and the page arrows.
 */
void Plsel_RewardListDraw(void)
{
    PlselList *list;
    s32 y;

    list = &gPlselWork->rewList;
    Plsel_DrawResultsBg();
    switch (list->screen) {
    case 0:
        Text_SetSize(4);
        if (list->scrollable == 1) {
            if (list->topLine > 0) {
                y = 128 - list->arrowBob;
                Font_SetSpacing(28, 28);
                Font_SetPos(500, y);
                Text_PrintfColored(gPlselArrowUp);
            }
            if (list->lineCount > list->topLine + list->linesShown) {
                y = list->arrowBob + 270;
                Font_SetSpacing(28, 28);
                Font_SetPos(500, y);
                Text_PrintfColored(gPlselArrowDown);
            }
        }
        if (list->pageCount[list->screen] > 1) {
            PlselRes_DrawArrow(1, 552, 88, 1);
            PlselRes_DrawArrow(0, 486, 88, 1);
        }
        break;
    }
}

INCLUDE_RODATA("asm/plsel/nonmatchings/quest_results", gPlselPageFmt);


/* The rewards screen text: the page title, the lines shown, the page's
 * total, the page number and the countdown. */
void Plsel_RewardListDrawText(SceneState *task)
{
    s32 w;
    s32 x;
    s32 y;
    s32 i;
    char **tbl;
    PlselList *list;
    char buf[128];
    char num[128];

    list = &gPlselWork->rewList;
    Text_SetColor(0);
    Font_SetSpacing(18, 18);
    w = Str_Length(gResultRuleText);
    w *= 9.0f; /* `w = len * 9.0f` puts the literal first */
    x = (s32)(640.0f - w) / 2.0f;
    y = 88;
    Text_SetColor(49);
    Font_SetSpacing(18, 18);
    tbl = &gPlselTexts[PLSEL_TEXT_REWARD_LIST];
    switch (list->screen) {
    case 0:
        switch (list->pages[list->pageCur]) {
        case 0:
            Font_SetSpacing(24, 24);
            Font_SetPos(x, 82);
            Text_Printf((s8 *) gPlselStrFmt, tbl[1]);
            Font_SetSpacing(18, 18);
            Font_SetPos(x, 110);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            y += 44;
            for (i = 0; i < list->linesShown; y += 22, i++) {
                Font_SetPos(x, y);
                if (list->ent[i].flag) {
                    Text_Printf((s8 *) &list->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 22;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            Font_SetPos(x, y + 22);
            sprintf(buf, tbl[8], list->itemTotal + list->rewardTotal);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) gPlselStrFmt, num);
            break;
        case 1:
            Font_SetPos(x, y);
            Text_Printf((s8 *) gPlselStrFmt, tbl[16]);
            Font_SetPos(x, 110);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            y += 44;
            for (i = list->topLine; i < list->topLine + list->linesShown; y += 22, i++) {
                if (i == list->markHuntLine && list->markHuntLine >= 0) {
                    Font_SetPos(x - 30, y);
                    Text_Printf((s8 *) gPlselTexts[162]);
                }
                Font_SetPos(x, y);
                if (list->ent[i].flag) {
                    Text_Printf((s8 *) &list->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 22;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            Font_SetPos(x, y + 22);
            sprintf(buf, tbl[21], list->huntReward);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) gPlselStrFmt, num);
            break;
        case 2:
            Font_SetPos(x, y);
            Text_Printf((s8 *) gPlselStrFmt, tbl[19]);
            Font_SetPos(x, 110);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            y += 44;
            for (i = list->topLine; i < list->topLine + list->linesShown; y += 22, i++) {
                if (i == list->markMonsterLine && list->markMonsterLine >= 0) {
                    Font_SetPos(x - 30, y);
                    Text_Printf((s8 *) gPlselTexts[161]);
                }
                Font_SetPos(x, y);
                if (list->ent[i].flag) {
                    Text_Printf((s8 *) &list->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 22;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            Font_SetPos(x, y + 22);
            sprintf(buf, tbl[22], list->killTotal);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) gPlselStrFmt, num);
            break;
        case 3:
            Font_SetPos(x, y);
            Text_Printf((s8 *) gPlselStrFmt, tbl[20]);
            Font_SetPos(x, 110);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            y += 44;
            for (i = list->topLine; i < list->topLine + list->linesShown; y += 22, i++) {
                Font_SetPos(x, y);
                if (list->ent[i].flag) {
                    Text_Printf((s8 *) &list->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 22;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            Font_SetPos(x, y + 22);
            sprintf(buf, tbl[23], list->itemTotal);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) gPlselStrFmt, num);
            break;
        }
        Font_SetPos(514, 88);
        sprintf(buf, gPlselPageFmt, list->pageCur + 1, list->pageCount[0]);
        Text_PrintfColored(buf);
        break;
    case 2:
            Font_SetSpacing(24, 24);
            Font_SetPos(x, 82);
            Text_Printf((s8 *) gPlselStrFmt, tbl[11]);
            Font_SetSpacing(18, 18);
            Font_SetPos(x, 110);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            y += 44;
            for (i = 0; i < list->linesShown; y += 22, i++) {
                Font_SetPos(x, y);
                if (list->ent[i].flag) {
                    Text_Printf((s8 *) &list->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 22;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) gPlselStrFmt, gResultRuleText);
            Font_SetPos(x, y + 22);
            sprintf(buf, tbl[15], list->zenny);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) gPlselStrFmt, num);
            break;
    }
    Font_SetPos(490, 356);
    Text_Printf((s8 *) gPlselTexts[PLSEL_TEXT_PROMPT]);
    Font_SetPos(440, 380);
    sprintf(num, gPlselTexts[PLSEL_TEXT_COUNTDOWN], task->timeout / 30);
    Text_Printf((s8 *) num);
}

/* The line being written of the results screen's text. */
#define WLINE(w) ((w)->lines[(w)->lineCount])

/* Stripped_Sub: a stand-in for a function of this file that the game's
 * linker dead-stripped (tools/mwcc_cc.py compiles it and drops its code). Its
 * body is unknown: what shows is that the file compiled a two-argument runtime
 * call before Plsel_BuildResultText, which changes that function's code
 * (docs/notes/nm/Plsel_BuildResultText.md). The "%s" before
 * Plsel_CategoryIsShared's jump table, first used only later, is another
 * sign of a stripped function here. */
static f64 Stripped_Sub(f64 lhs, f64 rhs)
{
    return lhs - rhs;
}

/* Builds the text lines (w->lines, count w->lineCount) of the results
 * screen's page `kind` (w->page): 0 the quest's summary, 1 and 2 the
 * rewards (the goals, then per monster), 3 the items sold (seven from line
 * w->firstLine), 4 the hunter rank, 5 a monster group's size records, 7 the
 * guild card's record.
 * The game's code reuses the parameter as a counter and as the monster
 * group (in one register), and one counter for two loops.
 * FAKEMATCH: it matches only with Stripped_Sub before it. Once a function
 * earlier in the file has compiled a two-argument runtime call (dpsub,
 * dpadd, __divdi3...), MWCC models its own fptodp calls (the floats promoted
 * for sprintf) as reading a1 too. So in case 5, a1 stays busy from the
 * format of the sprintf before each size test to the fptodp after it: the
 * size stays in v0, is copied to a1 after the test, and the copy can't fill
 * the test's delay slot, as in the game. Without it MWCC loads the size into
 * a1 and fills the slot with kind's andi (2 words short; the old block
 * stayed at 24 words). Nothing left in plsel calls such a helper. */
void Plsel_BuildResultText(s32 kind)
{
    char **fmts;
    char bufA[128];
    char bufB[128];
    char **str;
    s32 rec;
    s32 goal;
    PlselWork *w;
    s32 goalNeed;
    s32 nameLen;
    s32 shownCount;
    s32 shownMoney;
    s32 index;
    s32 itemValue;
    f32 sizeCm;
    u16 savedSize;
    ArenaEm *arenaEm;

    w = gPlselWork;
    memset(w->lines, 0, sizeof(w->lines));
    w->unk0748 = 0;
    w->scroll = 0;
    w->lineCount = 0;
    str = &gResultRuleText;
    fmts = &gPlselTexts[PLSEL_TEXT_RESULT_FMTS];
    switch (kind) {
    case 0:
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[0]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(bufA, fmts[1], w->questMoney);
        Sjis_Widen(bufA, bufB);
        sprintf(w->lines[w->lineCount++], bufB);
        sprintf(bufA, fmts[2], w->monsterMoney);
        Sjis_Widen(bufA, bufB);
        sprintf(w->lines[w->lineCount++], bufB);
        sprintf(bufA, fmts[3], w->itemMoney);
        Sjis_Widen(bufA, bufB);
        sprintf(w->lines[w->lineCount++], bufB);
        w->lineCount += 4;
        sprintf(bufA, fmts[4], w->moneyTotal);
        Sjis_Widen(bufA, bufB);
        sprintf(w->lines[w->lineCount++], bufB);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(bufA, fmts[5], gSave.money);
        Sjis_Widen(bufA, bufB);
        sprintf(w->lines[w->lineCount++], bufB);
        break;
    case 2:
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[6]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[7]);
        for (rec = 0; rec < 8; rec++) {
            if (gPlselBonus.recs[rec].id != 0) {
                switch (gPlselBonus.recs[rec].type) {
                case 0:
                case 1:
                case 2:
                    for (goal = 0; goal < 3; goal++) {
                        if (gQuest.huntGoals[goal].type == gPlselBonus.recs[rec].type && gQuest.huntGoals[goal].kind == gPlselBonus.recs[rec].id) {
                            break;
                        }
                    }
                    if (goal != 1) {
                        strcpy(bufA, gMonsterNames[gPlselBonus.recs[rec].id]);
                        if ((kind = Str_Length(bufA)) < 20) {
                            do {
                                strcat(bufA, gPlselPadSpace);
                            } while (++kind < 20);
                        }
                        sprintf(WLINE(w), gPlselStrFmt, bufA);
                        goalNeed = gQuest.huntGoals[goal].need;
                        if (goalNeed == 0) {
                            goalNeed = 1;
                        }
                        sprintf(bufA, fmts[8], goalNeed, gPlselBonus.recs[rec].reward);
                        Sjis_Widen(bufA, bufB);
                        strcat(WLINE(w), bufB);
                        if (goal == 0) {
                            strcat(WLINE(w), (&gResultRuleText)[goal + 2]);
                        }
                        w->lineCount++;
                    }
                    break;
                }
            }
        }
        w->lineCount++;
        if (gScene.playerCount == 1) {
            sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[9]);
        } else {
            sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[10]);
        }
        for (kind = 0; kind < 8; kind++) {
            if (gPlselBonus.recs[kind].id != 0) {
                switch (gPlselBonus.recs[kind].type) {
                case 0:
                case 1:
                case 2:
                    strcpy(bufA, gMonsterNames[gPlselBonus.recs[kind].id]);
                    if ((nameLen = Str_Length(bufA)) < 20) {
                        do {
                            strcat(bufA, gPlselPadSpace);
                        } while (++nameLen < 20);
                    }
                    sprintf(WLINE(w), gPlselStrFmt, bufA);
                    for (index = 0; index < 3; index++) {
                        if (gQuest.huntGoals[index].type == gPlselBonus.recs[kind].type && gQuest.huntGoals[index].kind == gPlselBonus.recs[kind].id) {
                            break;
                        }
                    }
                    switch (index) {
                    case 0:
                        shownCount = gQuest.huntGoals[index].need * gPlselBonus.recs[kind].count;
                        shownMoney = gQuest.huntGoals[index].reward;
                        shownMoney *= shownCount;
                        shownMoney /= gScene.playerCount;
                        if (w->lowRankCut != 0) {
                            shownMoney = shownMoney * 60 / 100;
                        }
                        break;
                    case 1:
                        shownMoney = gQuest.huntGoals[index].reward;
                        shownMoney /= gScene.playerCount;
                        shownCount = gQuest.huntGoals[index].need;
                        if (w->lowRankCut != 0) {
                            shownMoney = shownMoney * 60 / 100;
                        }
                        break;
                    default:
                        shownCount = gPlselBonus.recs[kind].count;
                        shownMoney = gPlselBonus.recs[kind].reward;
                        shownMoney *= shownCount;
                        shownMoney /= gScene.playerCount;
                        if (w->lowRankCut != 0) {
                            shownMoney = shownMoney * 60 / 100;
                        }
                        break;
                    }
                    sprintf(bufA, fmts[8], shownCount, shownMoney);
                    Sjis_Widen(bufA, bufB);
                    strcat(WLINE(w), bufB);
                    if (index < 3) {
                        strcat(WLINE(w), (&gResultRuleText)[index + 2]);
                    }
                    w->lineCount++;
                    break;
                }
            }
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(bufA, fmts[11], w->monsterMoney);
        Sjis_Widen(bufA, bufB);
        sprintf(w->lines[w->lineCount++], bufB);
        break;
    case 1:
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[12]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        if (gScene.playerCount == 1) {
            sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[13]);
        } else {
            sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[14]);
        }
        sprintf(bufA, fmts[15], w->fee);
        Sjis_Widen(bufA, bufB);
        sprintf(WLINE(w), bufB);
        if (w->feeRate != 1.0f && w->fee != 0) {
            strcat(WLINE(w), str[1]);
        }
        w->lineCount++;
        if (w->wtdFee > 0) {
            sprintf(bufA, fmts[16], w->wtdFee);
            Sjis_Widen(bufA, bufB);
            sprintf(WLINE(w), bufB);
            if (w->wtdFeeRate != 1.0f && w->wtdFee != 0) {
                strcat(WLINE(w), str[1]);
            }
            w->lineCount++;
        }
        if (w->mainDone != 0) {
            if (w->mustOrder == 1) {
                sprintf(bufA, fmts[52], w->mainMoney);
            } else {
                sprintf(bufA, fmts[17], w->mainMoney);
            }
            Sjis_Widen(bufA, bufB);
            sprintf(WLINE(w), bufB);
            w->lineCount++;
        }
        if (w->mustOrder == 0) {
            if (w->subADone != 0) {
                sprintf(bufA, fmts[18], w->subAMoney);
                Sjis_Widen(bufA, bufB);
                sprintf(WLINE(w), bufB);
                w->lineCount++;
            }
            if (w->subBDone != 0) {
                sprintf(bufA, fmts[19], w->subBMoney);
                Sjis_Widen(bufA, bufB);
                sprintf(WLINE(w), bufB);
                w->lineCount++;
            }
        }
        if ((u8)gQuest.anyFainted != 0) { /* lbu: the game reads it unsigned */
            sprintf(WLINE(w), gDeathPenaltyText);
            w->lineCount++;
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(bufA, fmts[20], w->questMoney);
        Sjis_Widen(bufA, bufB);
        sprintf(w->lines[w->lineCount++], bufB);
        break;
    case 3:
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[21]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[22]);
        for (index = w->firstLine; index < w->firstLine + 7; index++) {
            if (gPlselBonus.items[index].item != 0) {
                strcpy(bufA, gItemNames[gPlselBonus.items[index].item]);
                if ((kind = Str_Length(bufA)) < 20) {
                    do {
                        strcat(bufA, gPlselPadSpace);
                    } while (++kind < 20);
                }
                sprintf(WLINE(w), gPlselStrFmt, bufA);
                itemValue = gItemData[gPlselBonus.items[index].item].value;
                sprintf(bufA, fmts[23], gPlselBonus.items[index].count, itemValue * gPlselBonus.items[index].count);
                Sjis_Widen(bufA, bufB);
                strcat(w->lines[w->lineCount++], bufB);
            } else if (gPlselBonus.deliveredItem != 0 && gPlselBonus.deliveredCount != 0) {
                strcpy(bufA, gItemNames[gPlselBonus.deliveredItem]);
                if ((kind = Str_Length(bufA)) < 20) {
                    do {
                        strcat(bufA, gPlselPadSpace);
                    } while (++kind < 20);
                }
                sprintf(WLINE(w), gPlselStrFmt, bufA);
                sprintf(bufA, fmts[23], gQuest.huntGoals[2].need, gQuest.huntGoals[2].reward);
                Sjis_Widen(bufA, bufB);
                strcat(WLINE(w), bufB);
                strcat(WLINE(w), gWtdClassCText);
                w->lineCount++;
                break;
            }
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(bufA, fmts[24], w->itemMoney);
        Sjis_Widen(bufA, bufB);
        sprintf(w->lines[w->lineCount++], bufB);
        break;
    case 4:
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[43]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        if (Quest_GetOutcome(1) == 1) {
            if (w->mainDone != 0) {
                if (w->mustOrder == 1) {
                    sprintf(bufB, fmts[53], w->hrpMain);
                } else {
                    sprintf(bufB, fmts[44], w->hrpMain);
                }
                Sjis_Widen(bufB, bufA);
                strcpy(w->lines[w->lineCount++], bufA);
            }
            if (w->mustOrder == 0) {
                if (w->subADone != 0) {
                    sprintf(bufB, fmts[45], w->hrpSubA);
                    Sjis_Widen(bufB, bufA);
                    strcpy(w->lines[w->lineCount++], bufA);
                }
                if (w->subBDone != 0) {
                    sprintf(bufB, fmts[46], w->hrpSubB);
                    Sjis_Widen(bufB, bufA);
                    strcpy(w->lines[w->lineCount++], bufA);
                }
            }
        } else if (w->hrpMain != 0) {
            sprintf(bufB, fmts[51], w->hrpMain);
            Sjis_Widen(bufB, bufA);
            strcpy(w->lines[w->lineCount++], bufA);
        }
        if (w->deaths > 0) {
            sprintf(bufB, fmts[47], w->deaths, w->hrpDeaths);
            Sjis_Widen(bufB, bufA);
            sprintf(w->lines[w->lineCount++], "~C18%s~C17", bufA);
        }
        if (w->lineCount < 8) {
            w->lineCount = 8;
        }
        sprintf(bufB, fmts[48], w->hrpTotal);
        Sjis_Widen(bufB, bufA);
        sprintf(w->lines[w->lineCount++], bufA);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(bufB, fmts[49], w->hrPoints);
        Sjis_Widen(bufB, bufA);
        sprintf(w->lines[w->lineCount++], bufA);
        if (w->hrpResult == 0) {
            if (w->rankChange == 1) {
                sprintf(w->lines[w->lineCount++], gHrUpText);
            }
        } else if (w->hrpResult == 1) {
            sprintf(w->lines[w->lineCount++], gHrpNoLossText);
        } else if (w->hrpResult == 2) {
            sprintf(w->lines[w->lineCount++], gHrMaxText);
        } else {
            sprintf(w->lines[w->lineCount++], gHrpNotMetText);
        }
        sprintf(bufB, fmts[50], w->rankAfter);
        Sjis_Widen(bufB, bufA);
        sprintf(w->lines[w->lineCount++], bufA);
        break;
    case 5:
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[26]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        kind = w->recordGroups[w->recordGroupCur];
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[27]);
        strcpy(bufA, gMonsterNames[MonsterGroup_First(kind)]);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, bufA);
        w->unk0212 = 0;
        w->unk0213 = 0;
        w->oldLargest = w->largestBefore[kind];
        if ((savedSize = w->oldLargest) > 0) {
            sizeCm = Monster_SizeFromPercent(kind, savedSize);
        } else {
            sizeCm = 0.0f;
        }
        w->oldLargestCm = sizeCm;
        w->shownGroup = kind;
        if (sizeCm == 0.0f) {
            strcpy(bufB, fmts[31]);
        } else {
            sprintf(bufB, fmts[54], sizeCm);
        }
        Sjis_Widen(bufB, bufA);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, bufA);
        w->oldSmallest = w->smallestBefore[kind];
        if ((savedSize = w->oldSmallest) > 0) {
            sizeCm = Monster_SizeFromPercent(kind, savedSize);
        } else {
            sizeCm = 0.0f;
        }
        w->oldSmallestCm = sizeCm;
        if (sizeCm == 0.0f) {
            strcpy(bufB, fmts[32]);
        } else {
            sprintf(bufB, fmts[55], sizeCm);
        }
        Sjis_Widen(bufB, bufA);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, bufA);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[28]);
        w->newLargest = w->questLargest[w->recordGroups[w->recordGroupCur]];
        if (w->newLargest == 0) {
            sizeCm = 0.0f;
        } else {
            sizeCm = Monster_SizeFromPercent(kind, w->questLargest[w->recordGroups[w->recordGroupCur]]);
        }
        w->newLargestCm = sizeCm;
        w->newSmallest = w->questSmallest[w->recordGroups[w->recordGroupCur]];
        if (w->newSmallest == 0) {
            sizeCm = 0.0f;
        } else {
            sizeCm = Monster_SizeFromPercent(kind, w->questSmallest[w->recordGroups[w->recordGroupCur]]);
        }
        w->newSmallestCm = sizeCm;
        strcpy(bufA, gMonsterNames[MonsterGroup_First(kind)]);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, bufA);
        if (w->newLargest == 0) {
            strcpy(bufB, fmts[56]);
            Sjis_Widen(bufB, bufA);
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, bufA);
        } else {
            sprintf(bufB, fmts[29], w->newLargestCm);
            Sjis_Widen(bufB, bufA);
            if (w->largestNew[w->recordGroups[w->recordGroupCur]] != 0) {
                sprintf(w->lines[w->lineCount++], TXT(0x00D7BC10) /* colour 18, eight full-width spaces, the line, colour 17 */, bufA);
            } else {
                sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, bufA);
            }
        }
        if (w->newSmallest == 0) {
            strcpy(bufB, fmts[57]);
            Sjis_Widen(bufB, bufA);
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, bufA);
        } else {
            sprintf(bufB, fmts[30], w->newSmallestCm);
            Sjis_Widen(bufB, bufA);
            if (w->smallestNew[w->recordGroups[w->recordGroupCur]] != 0) {
                sprintf(w->lines[w->lineCount++], TXT(0x00D7BC10) /* colour 18, eight full-width spaces, the line, colour 17 */, bufA);
            } else {
                sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, bufA);
            }
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        break;
    case 7:
        if (w->arenaBefore.name[0] == 0) {
            sprintf(bufB, fmts[36], gRecordDashesText);
        } else {
            sprintf(bufB, fmts[36], w->arenaBefore.name);
        }
        Sjis_Widen(bufB, bufA);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, bufA);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, fmts[37]);
        sprintf(bufA, fmts[39], gArenaEmTameNames[w->arenaBefore.gaugeLevel]);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, bufA);
        sprintf(bufB, fmts[40], gArenaEmRankNames[(s8)SaveRecord_KindIndex_Unk((u8 *)&w->arenaBefore)]);
        Sjis_Widen(bufB, bufA);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, bufA);
        sprintf(bufB, fmts[41], Monster_SizeFromPercent(w->arenaBefore.emKind, w->arenaBefore.sizePct));
        Sjis_Widen(bufB, bufA);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, bufA);
        sprintf(WLINE(w), gPlselStrFmt, fmts[38]);
        if (w->friendChanged != 0 || w->strengthChanged != 0 || w->sizeChanged != 0) {
            strcat(WLINE(w), fmts[42]);
        }
        w->lineCount++;
        arenaEm = &gSave.arenaEms[gNextQuest.arenaRecord];
        sprintf(bufA, fmts[39], gArenaEmTameNames[arenaEm->gaugeLevel]);
        if (w->friendChanged != 0) {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BC30) /* colour 18, six full-width spaces, the line, colour 17 */, bufA);
        } else {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, bufA);
        }
        sprintf(bufB, fmts[40], gArenaEmRankNames[(s8)SaveRecord_KindIndex_Unk((u8 *)arenaEm)]);
        Sjis_Widen(bufB, bufA);
        if (w->strengthChanged != 0) {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BC30) /* colour 18, six full-width spaces, the line, colour 17 */, bufA);
        } else {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, bufA);
        }
        sprintf(bufB, fmts[41], Monster_SizeFromPercent(w->arenaBefore.emKind, arenaEm->sizePct));
        Sjis_Widen(bufB, bufA);
        if (w->sizeChanged != 0) {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BC30) /* colour 18, six full-width spaces, the line, colour 17 */, bufA);
        } else {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, bufA);
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], gPlselStrFmt, str[0]);
        break;
    }
}

void Plsel_ClearWork(void)
{
    memset(gPlselWork, 0, sizeof(PlselWork));
}

/*
 * Adds the quest's own money to questMoney when the quest was cleared: the
 * fee back (doubled online when paid), the rest of the reward money
 * (rewardMoney less the fee, doubled with wtdDouble), and the main and sub
 * goals' rewards (the subs split between the players, folded into the main
 * one for a must-order quest, and cut to 60% online with lowRankCut).
 */
void Plsel_SumQuestRewards(void)
{
    PlselWork *w;
    u8 *questInfo;

    w = gPlselWork;
    questInfo = (u8 *)gQuest.info;
    if (Quest_GetOutcome(1) == 1) {
        if (Quest_GetOutcome(1) == 1) {
            if (Game_IsOnline() == 1) {
                if (gScene.questPaid != 0) {
                    w->fee = *(s32 *)(questInfo + 8);
                    w->feeRate = 2.0f;
                    w->wtdFee = gPlselBonus.rewardMoney - w->fee;
                    if (w->wtdFee < 0) {
                        w->wtdFee = 0;
                    }
                    if (w->wtdDouble != 0) {
                        w->wtdFeeRate = 2.0f;
                    } else {
                        w->wtdFeeRate = 1.0f;
                    }
                } else {
                    w->fee = 0;
                    w->feeRate = 2.0f;
                    w->wtdFee = gPlselBonus.rewardMoney;
                    if (w->wtdFee < 0) {
                        w->wtdFee = 0;
                    }
                    if (w->wtdDouble != 0) {
                        w->wtdFeeRate = 2.0f;
                    } else {
                        w->wtdFeeRate = 1.0f;
                    }
                }
            } else {
                w->fee = *(s32 *)(questInfo + 8);
                w->feeRate = 1.0f;
                w->wtdFee = gPlselBonus.rewardMoney - w->fee;
                if (w->wtdFee < 0) {
                    w->wtdFee = 0;
                }
                w->wtdFeeRate = 1.0f;
            }
        }
        if (w->fee > 0) {
            w->questMoney += (s32)(w->fee * w->feeRate);
        }
        if (w->wtdFee > 0) {
            w->questMoney += (s32)(w->wtdFee * w->wtdFeeRate);
        }
        if (QuestGoal_IsDone(0) == 1) {
            w->mainDone = 1;
            if (gQuest.reward != *(s32 *)(questInfo + 12)) {
                w->questMoneyCut = 1;
            }
            w->mainMoney = gQuest.reward;
        }
        if (Quest_IsMustOrder() == 1) {
            w->mustOrder = 1;
        }
        if (QuestGoal_IsUsed(1) == 1) {
            if (QuestGoal_IsDone(1) == 1) {
                if (w->mustOrder == 0) {
                    w->subADone = 1;
                }
                w->subAMoney = *(s32 *)(questInfo + 20);
                w->subAMoney = w->subAMoney / gScene.playerCount;
            } else {
                w->subAMoney = 0;
            }
        }
        if (QuestGoal_IsUsed(2) == 1) {
            if (QuestGoal_IsDone(2) == 1) {
                if (w->mustOrder == 0) {
                    w->subBDone = 1;
                }
                w->subBMoney = *(s32 *)(questInfo + 24);
                w->subBMoney = w->subBMoney / gScene.playerCount;
            } else {
                w->subBMoney = 0;
            }
        }
        if (Game_IsOnline() == 1 && w->lowRankCut != 0) {
            w->questMoneyCut = 1;
            w->mainMoney = w->mainMoney * 60 / 100;
            w->subAMoney = w->subAMoney * 60 / 100;
            w->subBMoney = w->subBMoney * 60 / 100;
        }
        if (w->mustOrder == 1) {
            w->mainMoney += w->subAMoney;
            w->mainMoney += w->subBMoney;
            w->subAMoney = 0;
            w->subBMoney = 0;
        }
        w->questMoney += w->mainMoney;
        w->questMoney += w->subAMoney;
        w->questMoney += w->subBMoney;
    }
}

/*
 * Adds the hunt, capture and rate records of gPlselBonus to monsterMoney
 * (count x reward / players), and sets wtdDouble when a record is the main
 * hunt goal.
 */
void Plsel_SumRecordRewards(void)
{
    s32 i;
    PlselWork *w;
    s32 recType;

    w = gPlselWork;
    for (i = 0; i < 8; i++) {
        if (gPlselBonus.recs[i].id != 0) {
            recType = gPlselBonus.recs[i].type;
            if (recType == 0 || recType == 1 || recType == 2) {
                if (gQuest.huntGoals[0].type == recType && gQuest.huntGoals[0].kind == gPlselBonus.recs[i].id) {
                    w->wtdDouble = 1;
                }
                w->monsterMoney += gPlselBonus.recs[i].count * gPlselBonus.recs[i].reward / gScene.playerCount;
            }
        }
    }
}

/*
 * Counts the item lines, adds the items sold (gPlselBonus.items) and the
 * delivery record to itemMoney, and keeps the delivered item and its count
 * for the items page.
 */
void Plsel_TallyRewards(void)
{
    PlselWork *w;
    s32 i;

    w = gPlselWork;
    w->itemLines = 0;
    for (i = 0; i < 20; i++) {
        if (gPlselBonus.items[i].item != 0) {
            w->itemLines++;
            w->itemMoney += gPlselBonus.items[i].count * gItemData[gPlselBonus.items[i].item].value;
        }
    }
    for (i = 0; i < 8; i++) {
        if (gPlselBonus.recs[i].type == 3) {
            w->itemLines++;
            w->itemMoney += gPlselBonus.recs[i].reward;
            gPlselBonus.deliveredItem = gPlselBonus.recs[i].id;
            gPlselBonus.deliveredCount = gPlselBonus.recs[i].count;
        }
    }
}

/* Queues award `id` for the hunter titles page (w->awards, at most 32). */
void Plsel_QueueAward(s32 id)
{
    PlselWork *w;

    w = gPlselWork;
    if (w->awardCount < 32) {
        w->awards[w->awardCount] = id;
        w->awardCount++;
    }
}

/* Queues the rank awards (gPlselRankAwardRanks: the rank that earns each, 255 ends;
 * gPlselRankAwards: the award) up to rank arg0. */
void Plsel_CheckRankAwards(s16 rankAfter)
{
    s32 i;
    u8 rank;
    u8 *rankList = gPlselRankAwardRanks;

    for (i = 0, rank = *rankList; rank != 255; i++, rank = *++rankList) {
        if ((s16)(rank & 0xFF) > rankAfter) {
            break;
        }
        if ((s16)(rank & 0xFF) <= rankAfter && Award_Grant(gPlselRankAwards[i]) == 1) {
            Plsel_QueueAward(gPlselRankAwards[i]);
        }
    }
}

void Plsel_ResetPointRate(PlselWork *w)
{
    w->pointRate = 100;
}

/*
 * Online only: works out this quest's hunter rank points (the goals', or the
 * stage's points when it wasn't cleared; 60% with lowRankCut; 20 less per
 * faint; pointRate percent), adds them (Hr_AddPoints, into hrpResult), keeps
 * the rank before and after, and queues the rank awards.
 */
void Plsel_ComputeReward(void)
{
    PlselWork *w;

    w = gPlselWork;
    if (Game_IsOnline() == 0) {
        return;
    }
    Plsel_ResetPointRate(w);
    if (Quest_IsMustOrder() == 1) {
        w->mustOrder = 1;
    }
    w->rankBefore = Save_RankFromPoints(&gSave);
    if (Quest_GetOutcome(1) == 1) {
        if (QuestGoal_IsDone(0) == 1) {
            w->hrpMain = gQuest.header->hrpMain;
        }
        if (QuestGoal_IsDone(1) == 1) {
            w->hrpSubA = gQuest.header->hrpSubA;
        }
        if (QuestGoal_IsDone(2) == 1) {
            w->hrpSubB = gQuest.header->hrpSubB;
        }
    } else {
        w->hrpMain = gQuest.header->unk50;
    }
    if (gQuest.stage < 50000) {
        w->lowRankCut = 0;
        if (w->rankBefore > 30 && gQuest.info->stars < 5) {
            w->hrpMain = w->hrpMain * 60 / 100;
            w->hrpSubA = w->hrpSubA * 60 / 100;
            w->hrpSubB = w->hrpSubB * 60 / 100;
            w->lowRankCut = 1;
        }
    }
    w->deaths = gPlayers[gScene.localPlayer].faints;
    if (w->deaths > 0) {
        w->hrpDeaths = w->deaths * -20;
    }
    if (w->mustOrder == 1) {
        w->hrpMain += w->hrpSubA;
        w->hrpMain += w->hrpSubB;
        w->hrpSubA = 0;
        w->hrpSubB = 0;
    }
    w->hrpMain = w->hrpMain * w->pointRate / 100;
    w->hrpSubA = w->hrpSubA * w->pointRate / 100;
    w->hrpSubB = w->hrpSubB * w->pointRate / 100;
    w->hrpTotal = w->hrpMain;
    w->hrpTotal += w->hrpSubA;
    w->hrpTotal += w->hrpSubB;
    w->hrpTotal += w->hrpDeaths;
    w->hrpResult = Hr_AddPoints(w->hrpTotal);
    w->hrPoints = gSave.hrPoints;
    w->rankAfter = Save_RankFromPoints(&gSave);
    if (w->rankAfter != w->rankBefore) {
        if (w->rankAfter > w->rankBefore) {
            w->rankChange = 1;
        } else {
            w->rankChange = 2;
        }
    }
    Plsel_CheckRankAwards(w->rankAfter);
}

/*
 * Outside a drill: queues the awards for money, versus, the delivery
 * counters and the quest lists.
 */
void Plsel_CheckQuestAwards(void)
{
    PlselWork *w;
    s32 k;
    s32 entryId;
    u16 stageId;
    u16 award;
    s32 j;
    PlselKind *entry;
    s32 sum;
    s32 flag;
    s32 n;
    s32 totalMoney;
    u16 *list;

    w = gPlselWork;
    if (Quest_IsDrill() == 1) {
        return;
    }
    totalMoney = w->questMoney;
    totalMoney += w->extraMoney;
    totalMoney += w->monsterMoney;
    totalMoney += w->itemMoney;
    if (gSave.money + totalMoney >= 2000000 && (Award_Grant(30) == 1 || gPlselItemBoxSale[1] == 1)) {
        Plsel_QueueAward(30);
    }
    Plsel_CheckVersusAwards();
    entry = gRecordCountEntries;
    entryId = entry->id;
    sum = 0;
    flag = 0;
    j = 0;
    while (entryId != 0) {
        k = entry->kind;
        if (k == 2 || k == 3 || k == 5) {
            if (k == 3) {
                if (flag == 0) {
                    flag = 1;
                    sum += gSave.recordCounts[k];
                }
            } else {
                sum += gSave.recordCounts[k];
            }
        }
        if (gSave.recordCounts[k] >= 30 && Award_Grant(gPlselCountAwards[j]) == 1) {
            Plsel_QueueAward(gPlselCountAwards[j]);
        }
        entry++;
        j++;
        entryId = entry->id;
    }
    if (sum >= 30 && Award_Grant(129) == 1) {
        Plsel_QueueAward(129);
    }
    list = gPlselAwardTblA;
    entryId = *list;
    while (entryId != 0xFFFF) {
        award = list[1];
        list += 2;
        if (gSave.recordCounts[entryId] >= 100 && Award_Grant(award) == 1) {
            Plsel_QueueAward(award);
        }
        entryId = *list;
    }
    list = gPlselOfflineAwardsA;
    stageId = *list;
    while (stageId != 0xFFFF) {
        if (stageId == gQuest.stage) {
            if (gSave.recordCounts[14] >= 30 && Award_Grant(143) == 1) {
                Plsel_QueueAward(143);
            }
            return;
        }
        list++;
        stageId = *list;
    }
    list = gPlselOfflineAwardsB;
    stageId = *list;
    while (stageId != 0xFFFF) {
        if (stageId == gQuest.stage) {
            if (gSave.recordCounts[15] >= 30 && Award_Grant(144) == 1) {
                Plsel_QueueAward(144);
            }
            return;
        }
        list++;
        stageId = *list;
    }
    list = gPlselOfflineAwardsC;
    stageId = *list;
    while (stageId != 0xFFFF) {
        if (Save_IsQuestCleared(stageId) == 0) {
            break;
        }
        list++;
        stageId = *list;
    }
    if (*list == 0xFFFF && Award_Grant(145) == 1) {
        Plsel_QueueAward(145);
    }
    n = 0;
    list = gPlselOfflineAwardsD;
    stageId = *list;
    while (stageId != 0xFFFF) {
        if (Save_IsQuestCleared(stageId) == 0) {
            break;
        }
        list++;
        stageId = *list;
    }
    if (*list == 0xFFFF) {
        n++;
        if (Award_Grant(146) == 1) {
            Plsel_QueueAward(146);
        }
    }
    list = gPlselOfflineAwardsE;
    stageId = *list;
    while (stageId != 0xFFFF) {
        if (Save_IsQuestCleared(stageId) == 0) {
            break;
        }
        list++;
        stageId = *list;
    }
    if (*list == 0xFFFF && ++n == 2 && Award_Grant(147) == 1) {
        Plsel_QueueAward(147);
    }
}

/*
 * After a versus quest: the awards for 50 wins (with its rate flag) and 50
 * losses.
 */
void Plsel_CheckVersusAwards(void)
{
    if (Quest_IsVersus() == 1) {
        if (((s32) gSave.versusWins >= 50) && (Award_Grant(141) == 1)) {
            Plsel_QueueAward(141);
            if (RateFlag_SetQuiet(7) == 1) {
                Plsel_CheckCounterNotices();
            }
        }
        if (((s32) gSave.versusLosses >= 50) && (Award_Grant(142) == 1)) {
            Plsel_QueueAward(142);
        }
    }
}

/*
 * A drill only: the awards for the delivery counters, the drill's money and
 * payout, the items owned and the quest lists.
 */
void Plsel_CheckDrillAwards(void)
{
    PlselWork *w;
    s32 k;
    s32 entryId;
    u16 stage;
    u16 award;
    s32 i;
    s32 j;
    PlselKind *entry;
    s32 flag;
    s32 sum;
    PlselList *rewards;
    u16 *list;

    w = gPlselWork;
    if (Quest_IsDrill() != 1) {
        return;
    }
    entry = gRecordCountEntries;
    entryId = entry->id;
    rewards = &w->rewList;
    sum = 0;
    flag = 0;
    j = 0;
    while (entryId != 0) {
        k = entry->kind;
        if (k == 2 || k == 3 || k == 5) {
            if (k == 3) {
                if (flag == 0) {
                    flag = 1;
                    sum += gSave.recordCounts[k];
                }
            } else {
                sum += gSave.recordCounts[k];
            }
        }
        if (gSave.recordCounts[k] >= 30 && Award_Grant(gPlselCountAwards[j]) == 1) {
            Plsel_QueueAward(gPlselCountAwards[j]);
        }
        entry++;
        j++;
        entryId = entry->id;
    }
    if (sum >= 30 && Award_Grant(129) == 1) {
        Plsel_QueueAward(129);
    }
    list = gPlselAwardTblA;
    entryId = *list;
    while (entryId != 0xFFFF) {
        award = list[1];
        list += 2;
        if (gSave.recordCounts[entryId] >= 100 && Award_Grant(award) == 1) {
            Plsel_QueueAward(award);
        }
        entryId = *list;
    }
    if (rewards->zenny + rewards->payout >= 200000 && Award_Grant(136) == 1) {
        Plsel_QueueAward(136);
    }
    for (list = gPlselAwardTblD, i = 0; i < 12; i++, list++) {
        if (Save_IsItemObtained(&gSave, *list) == 0) {
            break;
        }
    }
    if (i == 12 && Award_Grant(137) == 1) {
        Plsel_QueueAward(137);
    }
    if (rewards->zenny + rewards->payout >= 15000 && Award_Grant(138) == 1) {
        Plsel_QueueAward(138);
    }
    list = gPlselAwardTblB;
    stage = *list;
    while (stage != 0xFFFF) {
        if (Save_IsQuestCleared(stage) == 0) {
            break;
        }
        list++;
        stage = *list;
    }
    if (*list == 0xFFFF && Award_Grant(139) == 1) {
        Plsel_QueueAward(139);
    }
    list = gPlselAwardTblC;
    stage = *list;
    while (stage != 0xFFFF) {
        if (Save_IsQuestCleared(stage) == 0) {
            break;
        }
        list++;
        stage = *list;
    }
    if (*list == 0xFFFF && Award_Grant(139) == 1) {
        Plsel_QueueAward(139);
    }
    list = gPlselTrainingAwards;
    stage = *list;
    while (stage != 0xFFFF) {
        if (Save_IsQuestCleared(stage) == 0) {
            break;
        }
        list++;
        stage = *list;
    }
    if (*list == 0xFFFF && Award_Grant(140) == 1) {
        Plsel_QueueAward(140);
    }
}

/*
 * After a training quest: the award once every quest of
 * gPlselTrainingAwards is cleared.
 */
void Plsel_CheckTrainingAwards(void)
{
    u16 *stage;
    u16 id;

    if (Quest_IsTraining() == 1) {
        stage = gPlselTrainingAwards;
        id = *stage;
        while (id != 0xFFFF) {
            if (Save_IsQuestCleared(id) == 0) {
                break;
            }
            stage++;
            id = *stage;
        }
        if (*stage == 0xFFFF && Award_Grant(140) == 1) {
            Plsel_QueueAward(140);
        }
    }
}

/*
 * Queues the hunt-count awards (gPlselHuntAwards), the award for the large
 * monsters captured, and the gold, big and small crown awards at 5, 10 and
 * 15 monster groups.
 */
void Plsel_CheckMonsterAwards(void)
{
    PlselHuntAward *entry;
    u8 byMonster;
    s32 k;
    s32 i;
    s32 big;
    s32 small;
    s32 crown;
    MonsterNoteDef *note;
    u16 largest;

    entry = gPlselHuntAwards;
    byMonster = entry->byMonster;
    while (byMonster != 255) {
        k = entry->award;
        /* `+ 0` makes the last argument an int expression, which the game
         * loads before entry->monster (as a call or macro there would). */
        if (Records_HuntedAtLeast((s32)byMonster, &gSave, entry->monster, entry->count + 0) == 1 && Award_Grant(k) == 1) {
            Plsel_QueueAward(k);
        }
        entry++;
        byMonster = entry->byMonster;
    }
    if (gSave.recordCounts[16] >= 100 && Award_Grant(99) == 1) {
        Plsel_QueueAward(99);
    }
    big = 0;
    small = 0;
    crown = 0;
    for (i = 1; i < 73; i++) {
        note = MonsterNote_FindKind(MonsterGroup_First(i));
        if (note != NULL) {
            largest = gSave.sizeMax[i];
            if (largest >= note->goldCrown && note->bigCrown != -1) {
                big++;
            }
            if (largest >= note->bigCrown && note->bigCrown != -1) {
                crown++;
            }
            if (gSave.sizeMin[i] <= note->smallCrown && gSave.sizeMin[i] > 0 && note->smallCrown != -1) {
                small++;
            }
        }
    }
    if (big >= 5 && Award_Grant(100) == 1) {
        Plsel_QueueAward(100);
    }
    if (big >= 10 && Award_Grant(101) == 1) {
        Plsel_QueueAward(101);
    }
    if (big >= 15) {
        if (Award_Grant(102) == 1) {
            Plsel_QueueAward(102);
        }
        if (RateFlag_SetQuiet(9) == 1) {
            Plsel_CheckCounterNotices();
        }
    }
    if (crown >= 5 && Award_Grant(103) == 1) {
        Plsel_QueueAward(103);
    }
    if (crown >= 10 && Award_Grant(104) == 1) {
        Plsel_QueueAward(104);
    }
    if (crown >= 15 && Award_Grant(105) == 1) {
        Plsel_QueueAward(105);
    }
    if (small >= 5 && Award_Grant(106) == 1) {
        Plsel_QueueAward(106);
    }
    if (small >= 10 && Award_Grant(107) == 1) {
        Plsel_QueueAward(107);
    }
    if (small >= 15 && Award_Grant(108) == 1) {
        Plsel_QueueAward(108);
    }
}

/*
 * Works out the money and points of the results, the awards and the size
 * records, pays the total into the save, and opens the money pages (four,
 * with a timeout of 3600 frames) at w->page.
 */
void Plsel_ComputeAllRewards(void)
{
    PlselWork *w;

    w = gPlselWork;
    Plsel_ComputeReward();
    Plsel_SumRecordRewards();
    Plsel_SumQuestRewards();
    if (Game_IsOnline() == 1) {
        if (gScene.playerCount > 1) {
            w->monsterMoneyCut = 1;
        }
        if (w->lowRankCut != 0) {
            w->monsterMoneyCut = 1;
            w->monsterMoney = w->monsterMoney * 60 / 100;
        }
    }
    Plsel_TallyRewards();
    Plsel_CheckQuestAwards();
    gPlselBonus.sizeCheckAfter = 0;
    Plsel_CheckSizeRecords(&gScene, 2);
    Plsel_CheckMonsterAwards();
    w->moneyTotal = w->questMoney;
    w->moneyTotal += w->extraMoney;
    w->moneyTotal += w->monsterMoney;
    w->moneyTotal += w->itemMoney;
    Save_AddMoney(w->moneyTotal);
    w->timeout = PLRES_PAGE_TIMEOUT;
    w->resMode = RES_MODE_MONEY;
    w->pageCount = RES_MONEY_PAGES;
    Plsel_BuildResultText(w->page);
}

extern s32 gResultsBgTex;
extern s32 gResultsIconTex;
extern s32 gPlselResultsTex;

/* The results screen background: the panel, the monster icons of pages 5
 * and 7, the page arrows, the closing zoom, and the size records of page 5. */
void Plsel_DrawResultsBg(void)
{
    PlselWork *w;
    s32 id;
    s32 savedZWrite;
    s32 savedAlphaTest;
    s32 savedBlend;
    s32 savedTexFilter;
    Sprite sprite;
    s32 zoomStep;
    f32 *zoomKey;
    f32 size;
    f32 alpha;
    MonsterNoteDef *def;

    w = gPlselWork;
    savedZWrite = Gs_GetState(RS_ZWRITE);
    savedAlphaTest = Gs_GetState(RS_ALPHA_TEST);
    savedBlend = Gs_GetState(RS_BLEND);
    savedTexFilter = Gs_GetState(RS_TEX_FILTER);
    Gs_SetState(RS_ZWRITE, 0);
    Gs_SetState(RS_ALPHA_TEST, RS_CMP_ALWAYS);
    Gs_SetState(RS_BLEND, RS_BLEND_OPAQUE);
    Gs_SetState(RS_TEX_FILTER, 0);
    Tex_Use(1, (u32 *)&gResultsBgTex);
    Gs_SetState(RS_TEXTURE, gResultsBgTex);
    sprite.x = 0;
    sprite.y = 0;
    sprite.w = 640;
    sprite.h = 448;
    sprite.u0 = 0;
    sprite.v0 = 0;
    sprite.u1 = 255;
    sprite.v1 = 255;
    sprite.color = -1;
    Sprite_Draw(&sprite);
    if (gScene.step < PLRES_NOTICES_START || gScene.step > PLRES_NOTICES_OUT) {
        if (w->page == RES_PAGE_RECORDS || w->page == RES_PAGE_ARENA) {
            if (w->page == RES_PAGE_RECORDS) {
                id = w->recordGroups[w->recordGroupCur];
            } else {
                id = w->arenaBefore.emKind;
            }
            Gs_SetState(RS_TEX_FILTER, 0x10000);
            Tex_Use(1, (u32 *)&gResultsIconTex);
            Gs_SetState(RS_TEXTURE, gResultsIconTex);
            sprite.x = 110;
            sprite.y = 154;
            sprite.w = 64;
            sprite.h = 64;
            EntryPicture_GetUV((s32 *)&sprite.u0, (s32 *)&sprite.u1, id);
            sprite.color = -1;
            Sprite_Draw(&sprite);
            sprite.y = 242;
            Sprite_Draw(&sprite);
        }
        if (Quest_IsDrill() != 1) {
            if ((u32)w->page <= 2 || w->page == RES_PAGE_ITEMS) {
                Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
                if (w->pageCount > 1) {
                    PlselRes_DrawArrow(1, 552, 88, 1);
                    PlselRes_DrawArrow(0, 486, 88, 1);
                }
            }
        }
    }
    if (w->closeState == PLRES_CLOSING) {
        Gs_SetState(RS_BLEND, RS_BLEND_ALPHA);
        Gs_SetState(RS_TEX_FILTER, 0);
        Tex_Use(1, (u32 *)&gPlselResultsTex);
        Gs_SetState(RS_TEXTURE, gPlselResultsTex);
        zoomStep = 29 - w->closeTimer;
        if (zoomStep > 2) {
            zoomStep = 2;
        }
        zoomKey = (f32 *)gPlselResultsBgDepth + zoomStep * 2;
        size = (s32)(150.0f * zoomKey[0]);
        alpha = 160.0f * zoomKey[1];
        sprite.x = 360.0f + (150.0f - size) / 2.0f;
        sprite.y = 180.0f + (150.0f - size) / 2.0f;
        sprite.w = size;
        sprite.h = size;
        sprite.u0 = 0;
        sprite.v0 = 0;
        sprite.u1 = 79;
        sprite.v1 = 79;
        sprite.color = ((u32)alpha << 24) | 0x50140A;
        Sprite_Draw(&sprite);
    }
    Gs_SetState(RS_ZWRITE, savedZWrite);
    Gs_SetState(RS_ALPHA_TEST, savedAlphaTest);
    Gs_SetState(RS_BLEND, savedBlend);
    Gs_SetState(RS_TEX_FILTER, savedTexFilter);
    if (gScene.step >= PLRES_RECORDS_START && gScene.step < PLRES_NOTICES_START && w->page == RES_PAGE_RECORDS) {
        def = MonsterNote_FindKind(MonsterGroup_First((u8)w->shownGroup));
        if (w->oldLargest > 0) {
            MonsterNote_DrawLargeCrown(def->note, 201, 177, (s16)w->oldLargest);
        }
        if (w->oldSmallest > 0) {
            MonsterNote_DrawSmallCrown(def->note, 201, 197, (s16)w->oldSmallest);
        }
        if (w->oldLargest < w->newLargest && w->newLargest > 0) {
            MonsterNote_DrawLargeCrown(def->note, 201, 264, (s16)w->newLargest);
        }
        if (w->oldSmallest == 0) {
            MonsterNote_DrawSmallCrown(def->note, 201, 284, (s16)w->newSmallest);
        } else if (w->newSmallest < w->oldSmallest && w->newSmallest > 0) {
            MonsterNote_DrawSmallCrown(def->note, 201, 284, (s16)w->newSmallest);
        }
    }
    Draw_Icons(gPlselResultsBgIcon, 1);
}

/*
 * Draws the current page's lines (the first one larger), the prompt and the
 * countdown, and what the mode adds: the page counter and the high rank
 * note, the item list's arrows, the new-record marks and the group arrows.
 * `unused` is not read.
 */
void Plsel_DrawResultPage(s32 unused)
{
    s32 i;
    s32 size;
    s32 x;
    s32 y;
    PlselWork *w;
    s32 width;
    char buf[128];

    w = gPlselWork;
    size = 18;
    width = Str_Length(gResultRuleText);
    width *= 9.0f;
    width = 640.0f - width;
    x = width / 2.0f;
    y = 88;
    Text_SetColor(49);
    Font_SetSpacing(size, size);
    for (i = 0; i < w->lineCount; i++) {
        if (i == 0) {
            Font_SetSpacing(24, 24);
            Font_SetPos(x, y - 6);
            Text_Printf((s8 *) w->lines[i]);
            size = 18;
            Font_SetSpacing(size, size);
        } else {
            Font_SetPos(x, y);
            if (w->lines[i][0] != 0) {
                Text_Printf((s8 *) w->lines[i]);
            }
        }
        y += size + 4;
    }
    Font_SetPos(490, 356);
    Text_Printf((s8 *) gPlselTexts[PLSEL_TEXT_PROMPT]);
    Font_SetPos(440, 380);
    sprintf(buf, gPlselTexts[PLSEL_TEXT_COUNTDOWN], w->timeout / 30);
    Text_Printf((s8 *) buf);
    if (w->resMode == RES_MODE_MONEY) {
        switch (w->page) {
        case 0:
            if (w->moneyTotal != 0 && w->questMoneyCut != 0 && w->lowRankCut != 0) {
                Font_SetPos(300, 88);
                Text_Printf((s8 *) gHighClassBonusText);
            }
            break;
        case 2:
            if (w->monsterMoney != 0 && w->questMoneyCut != 0 && w->lowRankCut != 0) {
                Font_SetPos(300, 88);
                Text_Printf((s8 *) gHighClassBonusText);
                Font_SetPos(514, 88);
                sprintf(buf, gPlselPageFmt, w->page + 1, w->pageCount);
                Text_PrintfColored(buf);
                return;
            }
            break;
        case 1:
            if (w->questMoney != 0 && w->questMoneyCut != 0 && w->lowRankCut != 0) {
                Font_SetPos(300, 88);
                Text_Printf((s8 *) gHighClassBonusText);
            }
            break;
        }
        Font_SetPos(514, 88);
        sprintf(buf, gPlselPageFmt, w->page + 1, w->pageCount);
        Text_PrintfColored(buf);
    }
    if (w->resMode == RES_MODE_POINTS && w->hrpTotal != 0 && w->lowRankCut != 0) {
        Font_SetPos(410, 88);
        Text_Printf((s8 *) gHighClassBonusText);
    }
    if (w->resMode == RES_MODE_MONEY && w->page == RES_PAGE_ITEMS) {
        if (w->firstLine > 0) {
            Font_SetSpacing(28, 28);
            Font_SetPos(500, 120);
            if (gFrameTick & 0x10) {
                Text_PrintfColored(gPlselArrowUp);
            }
        }
        if (w->firstLine + 7 < w->itemLines) {
            Font_SetSpacing(28, 28);
            Font_SetPos(500, 290);
            if (gFrameTick & 0x10) {
                Text_PrintfColored(gPlselArrowDown);
            }
        }
    }
    Font_SetSpacing(18, 18);
    if (w->resMode == RES_MODE_RECORDS && w->page == RES_PAGE_RECORDS) {
        if (w->newLargest > 0 && ((w->oldLargest == 0 && w->newLargest > 0) || (w->oldLargest > 0 && w->oldLargest < w->newLargest))) {
            Font_SetPos(472, 176);
            Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_NEW_RECORD]);
        }
        if (w->newSmallest > 0 && ((w->oldSmallest == 0 && w->newSmallest > 0) || (w->oldSmallest > 0 && w->newSmallest < w->oldSmallest))) {
            Font_SetPos(472, 198);
            Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_NEW_RECORD]);
        }
        if (w->recordGroupCount != 1) {
            if (w->recordGroupCur > 0) {
                Font_SetSpacing(28, 28);
                Font_SetPos(500, 120);
                if (gFrameTick & 0x10) {
                    Text_PrintfColored(gPlselArrowUp);
                }
            }
            if (w->recordGroupCur < w->recordGroupCount - 1) {
                Font_SetSpacing(28, 28);
                Font_SetPos(500, 290);
                if (gFrameTick & 0x10) {
                    Text_PrintfColored(gPlselArrowDown);
                }
            }
            return;
        }
    }
}

/*
 * Input of the money pages: left and right turn the page (wrapping), up and
 * down scroll the items page, circle starts the closing. Returns 1 once the
 * closing has run, else 0 (also while scene->timer runs).
 */
s32 PlselRes_RewardPageInput(SceneState *scene)
{
    s32 ret;
    PlselWork *w;
    u16 padRepeat;
    u16 pad;

    w = gPlselWork;
    if (scene->timer != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    padRepeat = gPadState[0].repeat | pad;
    if (w->closeState == PLRES_CLOSE_ASKED) {
        PlselRes_Close();
    }
    if (w->closeState == PLRES_CLOSING) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    w->resMode = RES_MODE_MONEY;
    w->pageCount = RES_MONEY_PAGES;
    Plsel_BuildResultText(w->page);
    if (pad & PAD_RIGHT) {
        PlselRes_PlayPageSe();
        w->page++;
        if (w->page > w->pageCount - 1) {
            w->page = RES_PAGE_SUMMARY;
        }
        w->firstLine = 0;
        Plsel_BuildResultText(w->page);
    } else if (pad & PAD_LEFT) {
        PlselRes_PlayPageSe();
        w->page--;
        if (w->page < 0) {
            w->page = w->pageCount - 1;
        }
        w->firstLine = 0;
        Plsel_BuildResultText(w->page);
    } else {
        if (w->page == RES_PAGE_ITEMS) {
            if ((padRepeat & PAD_UP) && w->firstLine != 0) {
                PlselRes_PlayPageSe();
                w->firstLine--;
                goto end;
            }
            if ((padRepeat & PAD_DOWN) && w->firstLine + 7 < w->itemLines) {
                PlselRes_PlayPageSe();
                w->firstLine++;
                goto end;
            }
        }
        if ((pad & PAD_CIRCLE) && w->closeState == PLRES_OPEN) {
            PlselRes_PlayDecideSe();
            PlselRes_BeginClose();
        }
    }
    /* A scroll ends the frame here, past the circle button's test, as the
     * game's code jumps. */
end:
    return 0;
}

/*
 * Opens the size records page (RES_PAGE_RECORDS) and plays the fanfare when
 * a record is new.
 */
void PlselRes_InitRecordPage(void)
{
    PlselWork *w;

    w = gPlselWork;
    w->resMode = RES_MODE_RECORDS;
    w->pageCount = 1;
    w->page = RES_PAGE_RECORDS;
    Plsel_BuildResultText(w->page);
    if (w->newRecord == 1) {
        Stream_Play(1, 29);
    }
}

/*
 * Input of the size records page: up and down pick the monster group,
 * circle starts the closing. Returns 1 once the closing has run.
 */
s32 PlselRes_RecordPageInput(SceneState *scene)
{
    s32 ret;
    PlselWork *w;
    u16 padRepeat;
    u16 pad;

    w = gPlselWork;
    if (scene->timer != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    padRepeat = gPadState[0].repeat | pad;
    if (w->closeState == PLRES_CLOSE_ASKED) {
        PlselRes_Close();
    }
    if (w->closeState == PLRES_CLOSING) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    if (padRepeat & PAD_UP) {
        if (w->recordGroupCur > 0) {
            w->recordGroupCur--;
            Plsel_BuildResultText(w->page);
            return 0;
        }
    }
    if (padRepeat & PAD_DOWN) {
        if (w->recordGroupCur < w->recordGroupCount - 1) {
            w->recordGroupCur++;
            Plsel_BuildResultText(w->page);
            return 0;
        }
    }
    if ((pad & PAD_CIRCLE) && w->closeState == PLRES_OPEN) {
        PlselRes_PlayDecideSe();
        PlselRes_BeginClose();
    }
    return 0;
}

/* Opens the hunter rank points page (RES_PAGE_POINTS). */
void PlselRes_InitPointsPage(void)
{
    PlselWork *w;

    w = gPlselWork;
    w->resMode = RES_MODE_POINTS;
    w->pageCount = 1;
    w->page = RES_PAGE_POINTS;
    Plsel_BuildResultText(w->page);
}

/*
 * Input of the hunter rank points page: circle starts the closing. Returns
 * 1 once the closing has run.
 */
s32 PlselRes_PointsPageInput(SceneState *scene)
{
    u16 pad;
    s32 ret;
    PlselWork *w;

    w = gPlselWork;
    if (scene->timer != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    if (w->closeState == PLRES_CLOSE_ASKED) {
        PlselRes_Close();
    }
    if (w->closeState == PLRES_CLOSING) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    if ((pad & PAD_CIRCLE) && w->closeState == PLRES_OPEN) {
        PlselRes_PlayDecideSe();
        PlselRes_BeginClose();
    }
    return 0;
}

/*
 * Opens the arena monster page (RES_PAGE_ARENA): marks which of its
 * friendliness, strength and size the versus quest changed (against
 * arenaBefore), and plays the fanfare if any did.
 */
void PlselRes_InitMonsterPage(void)
{
    PlselWork *w;
    ArenaEm *arenaEm;

    w = gPlselWork;
    w->resMode = RES_MODE_ARENA;
    w->pageCount = 1;
    w->page = RES_PAGE_ARENA;
    w->timeout = PLRES_PAGE_TIMEOUT;
    arenaEm = &gSave.arenaEms[gNextQuest.arenaRecord];
    if (arenaEm->gaugeLevel != w->arenaBefore.gaugeLevel) {
        w->friendChanged = 1;
    }
    if (arenaEm->level != w->arenaBefore.level) {
        w->strengthChanged = 1;
    }
    if (arenaEm->sizePct != w->arenaBefore.sizePct) {
        w->sizeChanged = 1;
    }
    Plsel_BuildResultText(w->page);
    if (w->friendChanged == 1 || w->strengthChanged == 1 || w->sizeChanged == 1) {
        Stream_Play(1, 29);
    }
}

/*
 * Input of the arena monster page: returns 1 when circle is pressed, once
 * scene->timer is 0.
 */
s32 PlselRes_MonsterPageInput(SceneState *scene)
{
    s32 ret;

    if (scene->timer != 0) {
        return 0;
    }
    ret = 0;
    if (gPadState[0].pressed & PAD_CIRCLE) {
        PlselRes_PlayDecideSe();
        ret = 1;
    }
    return ret;
}

/*
 * Opens the menu that keeps the captured monster as an arena record: its
 * cursor, the help lines, the slide-in, a timeout of 3600 frames, and the
 * count of arena slots in use.
 */
void Plsel_InitSaveRecordMenu(void)
{
    PlselWork *w;
    s32 i;

    w = gPlselWork;
    w->page = ARENA_CHOICE_KEEP;
    w->timeout = PLRES_PAGE_TIMEOUT;
    w->menuStep = ARENA_MENU_ASK;
    w->help1 = 0;
    w->help2 = 0;
    gHelpShow = 0;
    gHelpPage = HELP_PAGE_RESULTS_MENU;
    gHelpColumn = 0;
    Shake_Start(SHAKE_RESULTS_MENU, 0, -180.0f);
    w->recordCount = 0;
    for (i = 0; i < 3; i++) {
        if (gSave.arenaEms[i].emKind != 0) {
            w->recordCount++;
        }
    }
}

/*
 * Runs the arena record menu: keep or skip, pick a slot when all three are
 * in use, confirm the overwrite, then write the record (the default name,
 * kind, level and size, and a new monster's condition). Returns 0 once it is
 * done or timed out, 1 while it runs.
 */
s32 Plsel_SaveRecordMenu(void)
{
    PlselWork *w;
    u16 pad;
    u16 padRepeat;
    s32 ret;
    s32 i;

    ret = 1;
    w = gPlselWork;
    pad = gPadState[0].pressed;
    padRepeat = gPadState[0].repeat | pad;
    w->timeout--;
    if (w->timeout == 0) {
        if (w->menuStep >= ARENA_MENU_SAVED) {
            return 0;
        }
        if (w->menuStep < ARENA_MENU_SAVED) {
            w->timeout = 90;
            w->menuStep = ARENA_MENU_TIMED_OUT;
            PlselRes_PlayCancelSe();
        }
    }
    switch (w->menuStep) {
    case ARENA_MENU_ASK:
        if ((padRepeat & PAD_UP) && w->page != ARENA_CHOICE_KEEP) {
            PlselRes_PlayPageSe();
            w->page = ARENA_CHOICE_KEEP;
        } else if ((padRepeat & PAD_DOWN) && w->page == ARENA_CHOICE_KEEP) {
            PlselRes_PlayPageSe();
            w->page = ARENA_CHOICE_SKIP;
        } else if (pad & PAD_CIRCLE) {
            PlselRes_PlayDecideSe();
            if (w->page == ARENA_CHOICE_SKIP) {
                ret = 0;
            } else if (w->page == ARENA_CHOICE_KEEP) {
                w->menuStep = ARENA_MENU_PICK_SLOT;
                if (w->recordCount == 3) {
                    w->recordSlot = 0;
                } else {
                    w->recordSlot = w->recordCount;
                }
            }
        }
        break;
    case ARENA_MENU_PICK_SLOT:
        w->help1 = 3;
        w->help2 = 4;
        if (w->recordCount == 3) {
            w->help1 = 1;
            if ((padRepeat & PAD_UP) && w->recordSlot != 0) {
                PlselRes_PlayPageSe();
                w->recordSlot--;
                break;
            }
            if ((padRepeat & PAD_DOWN) && w->recordSlot != 2) {
                PlselRes_PlayPageSe();
                w->recordSlot++;
                break;
            }
        }
        if (pad & PAD_CROSS) {
            PlselRes_PlayCancelSe();
            w->menuStep = ARENA_MENU_ASK;
        } else if (pad & PAD_CIRCLE) {
            PlselRes_PlayDecideSe();
            if (w->recordCount == 3) {
                w->menuStep = ARENA_MENU_CONFIRM;
            } else {
                goto save;
            }
        }
        break;
    case ARENA_MENU_CONFIRM:
        w->help1 = 2;
        w->help2 = 4;
        if (pad & PAD_CROSS) {
            PlselRes_PlayCancelSe();
            w->menuStep = ARENA_MENU_PICK_SLOT;
        } else if (pad & PAD_CIRCLE) {
            PlselRes_PlayDecideSe();
        /* The record's writing, which a pick from fewer than three records
         * shares: the game's code has it once and jumps to it. */
        save:
            i = w->recordSlot;
            w->menuStep = ARENA_MENU_SAVED;
            memset(&gSave.arenaEms[i], 0, sizeof(ArenaEm));
            strcpy((char *)gSave.arenaEms[i].name, TXT(0x00D7BC70) /* the default name of an arena record's monster */);
            gSave.arenaEms[i].emKind = gQuestCapturedKind;
            gSave.arenaEms[i].level = gQuest.capturedLevel;
            gSave.arenaEms[i].conditionPoints = 100;
            gSave.arenaEms[i].condition = 2;
            gSave.arenaEms[i].sizePct = gQuest.capturedSize;
        }
        break;
    case ARENA_MENU_SAVED:
        w->help1 = 3;
        w->help2 = 6;
        if (pad & PAD_CIRCLE) {
            PlselRes_PlayDecideSe();
            ret = 0;
        }
        break;
    case ARENA_MENU_TIMED_OUT:
        break;
    }
    return ret;
}

/*
 * Writes line `line` of arena record `arenaEm` (ARENA_LINE_*: the name, the
 * monster, the rank or the size) into `out`, widened; a blank line for an
 * empty slot.
 */
void Plsel_FormatRecord(s32 line, ArenaEm *arenaEm, char *out)
{
    char buf[128];

    if (arenaEm->emKind == 0) {
        strcpy(out, gArenaEmBlankLine[0]);
        return;
    }
    buf[0] = 0;
    switch (line) {
    case 0:
        if (*(s8 *)arenaEm->name == 0) {
            sprintf(buf, gArenaEmLineFmts[line], gRecordDashesText);
        } else {
            sprintf(buf, gArenaEmLineFmts[line], arenaEm->name);
        }
        break;
    case 1:
        sprintf(buf, gArenaEmLineFmts[line], gMonsterNames[arenaEm->emKind]);
        break;
    case 2:
        sprintf(buf, gArenaEmLineFmts[line], gArenaEmRankNames[(s8)SaveRecord_KindIndex_Unk((u8 *)arenaEm)]);
        break;
    case 3:
        sprintf(buf, gArenaEmLineFmts[line], fptodp(Monster_SizeFromPercent(arenaEm->emKind, arenaEm->sizePct)));
        break;
    }
    Sjis_Widen(buf, out);
}

/*
 * Draws the arena record menu: the countdown, the captured monster's four
 * lines, the three slots with the cursor, and the help lines; after a
 * timeout, only the message that the monster was released.
 */
void Plsel_DrawSaveRecordMenu(void)
{
    PlselWork *w;
    s32 i;
    s32 j;
    s32 y;
    s32 lineY;
    char **tbl;
    ArenaEm *slotEm;
    char buf[128];
    char text[128];
    ListPanel win;
    ArenaEm rec;
    s8 emLevel;

    w = gPlselWork;
    y = 40;
    if (w->menuStep == ARENA_MENU_TIMED_OUT) {
        ListPanel_DrawAlpha(&gPlselSaveRecPanel2, 0, -1, 160);
        Text_SetColor(0);
        Font_SetSpacing(18, 18);
        Font_SetPos(203, 215);
        Text_Printf((s8 *) gArenaEmReleasedText);
        return;
    }
    if (w->menuStep < ARENA_MENU_SAVED) {
        win = gPlselSaveRecWinA;
        win.x = 32;
        win.y = y;
        ListPanel_Draw(&win, (s16 *)0, (char *)0, -1);
    }
    ListPanel_Draw(&gPlselSaveRecPanel, (s16 *)0, (char *)0, w->page);
    if (w->menuStep == ARENA_MENU_TIMED_OUT) {
        sprintf(buf, gPlselTexts[PLSEL_TEXT_ITEMBOX_TIME], 0);
    } else {
        sprintf(buf, gPlselTexts[PLSEL_TEXT_ITEMBOX_TIME], w->timeout / 30);
    }
    Window_DrawText(&gPlselSaveRecText, (s16 *)0, (u8 *)buf);
    Text_SetColor(32);
    Font_SetSpacing(18, 18);
    if (w->menuStep < ARENA_MENU_SAVED) {
        rec.emKind = gQuestCapturedKind;
        rec.conditionPoints = 100;
        emLevel = gQuest.capturedLevel;
        rec.condition = 2;
        rec.level = emLevel;
        rec.sizePct = gQuest.capturedSize;
        for (i = 0, tbl = gArenaEmLineFmts; i < 4; i++, y += 20, tbl++) {
            Font_SetPos(32, y);
            switch (i) {
            case 0:
                sprintf(buf, *tbl, gRecordDashesText);
                break;
            case 1:
                sprintf(buf, *tbl, gMonsterNames[rec.emKind]);
                break;
            case 2:
                sprintf(buf, *tbl, gArenaEmRankNames[(s8)SaveRecord_KindIndex_Unk((u8 *)&rec)]);
                break;
            case 3:
                sprintf(buf, *tbl, fptodp(Monster_SizeFromPercent(gQuestCapturedKind, gQuest.capturedSize)));
                break;
            }
            Sjis_Widen(buf, text);
            Text_PrintfColored(text);
        }
    }
    if (w->menuStep != ARENA_MENU_ASK) {
        y = 40;
        slotEm = gSave.arenaEms;
        for (i = 0; i < 3; i++) {
            win = gPlselSaveRecWinA;
            win.x = 362;
            win.y = y;
            ListPanel_Draw(&win, (s16 *)0, (char *)0, -1);
            if (w->menuStep != ARENA_MENU_ASK && i == w->recordSlot && w->recordCount == 3) {
                Menu_DrawGlowBar(472, y, 220, 80, 0xFF20C0C0);
            }
            for (lineY = y, j = 0; j < 4; j++, lineY += 20) {
                Plsel_FormatRecord(j, slotEm, text);
                Font_SetPos(362, lineY);
                Text_PrintfColored(text);
            }
            y += 100;
            slotEm++;
        }
        win = gPlselSaveRecWinB;
        win.x = 360;
        win.y = 350;
        ListPanel_Draw(&win, (s16 *)0, (char *)0, -1);
        if (w->help1 > 0) {
            Font_SetPos(360, 350);
            Text_PrintfColored(gArenaEmPenHelp[w->help1]);
        }
        if (w->help2 > 0) {
            Font_SetPos(360, 390);
            Text_Printf((s8 *) gArenaEmPenHelp[w->help2]);
        }
    }
}

/*
 * After a versus quest: clears PlselWork, keeps the arena record the quest
 * used as it was (arenaBefore), and notes the local side's item in the
 * monster notes under that side's monster.
 */
void Plsel_SnapshotVersusMonster(void)
{
    PlselWork *w;
    s16 idx;
    s32 team;
    u16 teamItem;

    Plsel_ClearWork();
    w = gPlselWork;
    idx = gNextQuest.arenaRecord;
    if (idx != -1) {
        w->arenaBefore = gSave.arenaEms[idx];
        team = gPlayers[gScene.localPlayer].side - 1;
        teamItem = gQuest.teams[team].item;
        if (teamItem != 0) {
            MonsterNote_Record(gNextQuest.teams[team].emKind, teamItem);
        }
    }
}

/*
 * Draws a page arrow at x, y: pointing right when `facesRight` is 1, left
 * (mirrored) when 0; small and flat when `isSmall` is 1, large and turned a
 * quarter when 0 (no caller passes 0).
 */
void PlselRes_DrawArrow(s32 facesRight, s16 x, s16 y, s32 isSmall)
{
    Sprite sprite;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Tex_Prepare(1, 341);
    Tex_Bind(341);
    sprite.x = x;
    sprite.y = y;
    if (isSmall == 0) {
        sprite.w = 30;
        sprite.h = 38;
    } else {
        sprite.w = 18;
        sprite.h = 20;
    }
    sprite.color = 0xFF32BC64;
    if (facesRight == 0) {
        sprite.u0 = 255;
        sprite.u1 = 240;
    } else {
        sprite.u0 = 240;
        sprite.u1 = 255;
    }
    sprite.v0 = 236;
    sprite.v1 = 255;
    if (isSmall == 0) {
        TexRect_DrawRotated((struct TexRect *)&sprite, 49152);
        return;
    }
    Sprite_Draw(&sprite);
}

extern s32 gPlselResultsSubTex;
/*
 * Loads the results screen's textures (compressed files 38 and 34, once
 * each) and reloads sound bank 7.
 */
void Plsel_LoadResultsTextures(void)
{
    Tex_LoadQuestEndFile();
    Tex_SetQuestEndImages();
    if (gPlselResultsTex == 0) {
        LoadCompressedFile(gSysSettings.loadBuf, 38);
        gPlselResultsTex = Tex_CreateFromTim2(gSysSettings.loadBuf, TEX_MODE_ON_DEMAND);
    }
    if (gPlselResultsSubTex == 0) {
        LoadCompressedFile(gSysSettings.loadBuf, 34);
        gPlselResultsSubTex = Tex_CreateFromTim2(gSysSettings.loadBuf, TEX_MODE_ON_DEMAND);
    }
    Sound_LoadResultsBank();
}

void PlselRes_EmptyCloseHook(void)
{

}

/*
 * When a close was asked, starts the closing (PLRES_CLOSING) and plays its
 * sound.
 */
void PlselRes_Close(void)
{
    PlselWork *w;

    w = gPlselWork;
    if (w->closeState != PLRES_OPEN && w->closeState != PLRES_CLOSING) {
        w->closeState = PLRES_CLOSING;
        Sound_PlaySeFlat(7, 8, 0);
    }
}

/*
 * Clears the notices, then queues the notice of each gPlselFlagUnlocks row
 * whose save event flag is set and whose system flag (the notice's low
 * byte) is not yet, and sets that flag.
 */
void Plsel_QueueFlagUnlocks(void)
{
    s32 item;
    s32 id;
    s32 i;
    u16 *entry;
    PlselWork *w;

    w = gPlselWork;
    memset(w->notices, 0, sizeof(w->notices));
    for (i = 0, entry = gPlselFlagUnlocks; i < 57; i++, entry += 2) {
        if (entry[0] == 0xFFFF) {
            break;
        }
        id = entry[0] & 0xFF;
        item = entry[1];
        if (EventFlag_Test(id) != 1 && Save_TestEventFlag(item) == 1) {
            EventFlag_Set(id);
            w->notices[w->noticeCount] = entry[0];
            w->noticeCount++;
        }
    }
}

/*
 * Queues the notice that the rates are open, once (save event flag 0xB6),
 * and grants AWARD_RATE_FLAGS once every rate flag is set.
 */
void Plsel_CheckCounterNotices(void)
{
    PlselWork *w;

    w = gPlselWork;
    if (Save_TestEventFlag(182) == 1 && w->rateNoticeQueued == 0) {
        w->notices[w->noticeCount] = 1280;
        w->noticeCount++;
        w->rateNoticeQueued = 1;
    }
    if ((gSave.rateFlags & 0x800) && Save_HasTitle(122) == 0) {
        Save_SetTitle(122);
        Plsel_QueueAward(122);
    }
}

/*
 * On a drill's first clear: queues the notice of the quest's entry in
 * gPlselDrillNoticeStages, and a second one for quest 10103.
 */
void Plsel_QueueDrillClearNotice(void)
{
    PlselWork *w;
    u16 *stageList;
    s32 i;
    u16 stageId;

    w = gPlselWork;
    if (Quest_IsDrill() == 1 && w->clearedBefore == 0) {
        stageList = gPlselDrillNoticeStages;
        i = 0;
        stageId = *stageList;
        while (stageId != 0) {
            if (stageId == gQuest.stage) {
                w->notices[w->noticeCount] = i | 0x300;
                w->noticeCount++;
                if (gQuest.stage == 10103) {
                    w->notices[w->noticeCount] = 1024;
                    w->noticeCount++;
                }
                return;
            }
            stageList++;
            stageId = *stageList;
            i++;
        }
    }
}

/*
 * Adds the quest's play style points (Save_AddCounterPoints); when that
 * earns a rate, queues the same notices as Plsel_CheckCounterNotices.
 */
void Plsel_AddCounterPoints(void)
{
    PlselWork *w;

    w = gPlselWork;
    if (Save_AddCounterPoints() == 1) {
        if (Save_TestEventFlag(182) != 0 && w->rateNoticeQueued == 0) {
            w->notices[w->noticeCount] = 1280;
            w->noticeCount++;
            w->rateNoticeQueued = 1;
        }
        if ((gSave.rateFlags & 0x800) && Save_HasTitle(122) == 0) {
            Save_SetTitle(122);
            Plsel_QueueAward(122);
        }
    }
}

/* Returns 1 if a notice is queued, else 0. */
s32 Plsel_HasNotices(void)
{
    s32 i;
    PlselWork *w;

    w = gPlselWork;
    for (i = 0; i < 57; i++) {
        if (w->notices[i] != 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * Builds the notices page's lines: each notice's rows of gAwardNoticeLines,
 * with the gallery or dojo title put in place of its placeholder; plays the
 * fanfare.
 */
void Plsel_BuildNoticeText(void)
{
    s32 noticeKind;
    s32 titleIndex;
    char **tbl;
    char *title;
    char *pct;
    s32 i;
    PlselWork *w;
    char buf[128];
    char spliced[128];

    w = gPlselWork;
    memset(w->lines, 0, sizeof(w->lines));
    w->unk0748 = 0;
    w->scroll = 0;
    w->lineCount = 0;
    for (i = 0; i < 57; i++) {
        if (w->notices[i] != 0) {
            noticeKind = (w->notices[i] & 0xFF00) >> 8;
            titleIndex = w->notices[i] & 0xFF;
            if (noticeKind == 2) {
                titleIndex -= 9;
            }
            if (noticeKind > 0) {
                for (tbl = gAwardNoticeLines[noticeKind - 1]; *tbl != NULL; tbl++) {
                    strcpy(buf, *tbl);
                    pct = strchr(buf, '%');
                    if (pct != NULL) {
                        strcpy(spliced, buf);
                        switch (pct[1]) {
                        case 'g':
                            title = gAwardGalleryTitles[titleIndex];
                            break;
                        case 'm':
                            title = gAwardDojoTitles[titleIndex];
                            break;
                        }
                        *strchr(spliced, '%') = 0;
                        strcat(spliced, title);
                        strcat(spliced, pct + 2);
                        strcpy(buf, spliced);
                    }
                    sprintf(w->lines[w->lineCount++], gPlselStrFmt, buf);
                }
            }
        }
    }
    Stream_Play(1, 29);
}

/*
 * Input of the notices page: up and down page through them (three lines a
 * page), circle starts the closing. Returns 1 once the closing has run.
 */
s32 PlselRes_NoticePageInput(void)
{
    s32 ret;
    PlselWork *w;
    u16 padRepeat;
    u16 pad;

    w = gPlselWork;
    if (gScene.timer != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    padRepeat = gPadState[0].repeat | pad;
    if (w->closeState == PLRES_CLOSE_ASKED) {
        PlselRes_Close();
    }
    if (w->closeState == PLRES_CLOSING) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    if ((padRepeat & PAD_UP) && w->noticePage != 0) {
        PlselRes_PlayPageSe();
        w->noticePage--;
    }
    if ((padRepeat & PAD_DOWN) && w->noticePage < w->noticeCount - 1) {
        PlselRes_PlayPageSe();
        w->noticePage++;
    }
    if ((pad & PAD_CIRCLE) && w->closeState == PLRES_OPEN) {
        PlselRes_PlayDecideSe();
        PlselRes_BeginClose();
    }
    return 0;
}

/*
 * Draws the notices page: the heading, the rule lines, the page's three
 * lines centred, the arrows, the prompt and the countdown.
 */
void Plsel_DrawNoticeText(void)
{
    char **ruleText;
    s32 i;
    PlselWork *w;
    s32 end;
    s32 x;
    s32 y;
    s32 width;
    char buf[128];

    w = gPlselWork;
    ruleText = &gResultRuleText;
    width = Str_Length(gResultRuleText);
    width *= 9.0f;
    width = 640.0f - width;
    x = width / 2.0f;
    y = 88;
    Text_SetColor(49);
    Font_SetSpacing(18, 18);
    Font_SetSpacing(24, 24);
    Font_SetPos(x, 82);
    Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_NOTICES_HEADING]);
    Font_SetSpacing(18, 18);
    Font_SetPos(x, 110);
    Text_Printf((s8 *) *(s32 *)&gResultRuleText);
    y += 88;
    end = w->noticePage * 3 + 3;
    for (i = w->noticePage * 3; i < end; i++) {
        if (w->lines[i][0] != 0) {
            width = Str_Length(w->lines[i]);
            width *= 9.0f;
            width = 640.0f - width;
            width /= 2.0f;
            Font_SetPos(width, y);
            Text_Printf((s8 *) w->lines[i]);
        }
        y += 22;
    }
    Font_SetPos(x, 308);
    Text_Printf((s8 *) *ruleText);
    if (w->noticePage > 0) {
        Font_SetSpacing(28, 28);
        Font_SetPos(306, 120);
        if (gFrameTick & 0x10) {
            Text_PrintfColored(gPlselArrowUp);
        }
    }
    if (w->noticePage * 3 + 3 < w->lineCount) {
        Font_SetSpacing(28, 28);
        Font_SetPos(306, 290);
        if (gFrameTick & 0x10) {
            Text_PrintfColored(gPlselArrowDown);
        }
    }
    Font_SetSpacing(18, 18);
    Font_SetPos(490, 356);
    Text_Printf((s8 *) gPlselTexts[PLSEL_TEXT_PROMPT]);
    Font_SetPos(440, 380);
    sprintf(buf, gPlselTexts[PLSEL_TEXT_COUNTDOWN], w->timeout / 30);
    Text_Printf((s8 *) buf);
}

/*
 * Builds the hunter titles page from the queued awards: each title's name
 * and up to three lines of its description, four lines a title; lineCount
 * ends as the number of titles.
 */
void Plsel_BuildTitleText(void)
{
    char *desc;
    char *secondLine;
    s32 i;
    u16 *award;
    PlselWork *w;
    HunterTitle *titles;
    char *newline;

    w = gPlselWork;
    memset(w->lines, 0, sizeof(w->lines));
    w->unk0748 = 0;
    w->scroll = 0;
    w->lineCount = 0;
    w->page = RES_PAGE_POINTS;
    award = w->awards;
    titles = gHunterTitles;
    for (i = 0; i < w->awardCount; i++, award++) {
        if (w->lineCount + 4 >= 80) {
            break;
        }
        strcpy(w->lines[w->lineCount++], titles[*award].text);
        desc = gHunterTitleDescs[*award];
        strcpy(w->lines[w->lineCount], desc);
        newline = strchr(w->lines[w->lineCount], '\n');
        if (newline != NULL) {
            *newline = 0;
        }
        w->lineCount++;
        secondLine = strchr(desc, '\n');
        if (secondLine == NULL) {
            w->lineCount++;
            w->lineCount++;
            continue;
        }
        strcpy(w->lines[w->lineCount], secondLine + 1);
        newline = strchr(w->lines[w->lineCount], '\n');
        if (newline != NULL) {
            *newline = 0;
        }
        w->lineCount++;
        newline = strchr(secondLine + 1, '\n');
        if (newline == NULL) {
            w->lineCount++;
            continue;
        }
        strcpy(w->lines[w->lineCount], newline + 1);
        w->lineCount++;
    }
    w->lineCount /= 4;
}

/*
 * Input of the hunter titles page: up and down show the previous or next
 * title, circle starts the closing. Returns 1 once the closing has run.
 */
s32 PlselRes_TitlePageInput(void)
{
    s32 ret;
    PlselWork *w;
    u16 padRepeat;
    u16 pad;

    w = gPlselWork;
    if (gScene.timer != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    padRepeat = gPadState[0].repeat | pad;
    if (w->closeState == PLRES_CLOSE_ASKED) {
        PlselRes_Close();
    }
    if (w->closeState == PLRES_CLOSING) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    if ((padRepeat & PAD_UP) && w->scroll != 0) {
        PlselRes_PlayPageSe();
        w->scroll--;
    }
    if ((padRepeat & PAD_DOWN) && w->scroll + 1 < w->lineCount) {
        PlselRes_PlayPageSe();
        w->scroll++;
    }
    if ((pad & PAD_CIRCLE) && w->closeState == PLRES_OPEN) {
        PlselRes_PlayDecideSe();
        PlselRes_BeginClose();
    }
    return 0;
}

/*
 * Draws the hunter titles page: the heading, the current title (its name
 * larger), the arrows, the prompt and the countdown.
 */
void Plsel_DrawTitleText(void)
{
    char **ruleText;
    PlselWork *w;
    s32 i;
    s32 size;
    s32 end;
    s32 x;
    s32 y;
    s32 width;
    char buf[128];

    w = gPlselWork;
    ruleText = &gResultRuleText;
    width = Str_Length(gResultRuleText);
    width *= 9.0f;
    width = 640.0f - width;
    x = width / 2.0f;
    y = 88;
    Text_SetColor(49);
    Font_SetSpacing(18, 18);
    Font_SetSpacing(24, 24);
    Font_SetPos(x, 82);
    Text_Printf((s8 *)gPlselTexts[PLSEL_TEXT_TITLES_HEADING]);
    Font_SetSpacing(18, 18);
    Font_SetPos(x, 110);
    Text_Printf((s8 *) *(s32 *)&gResultRuleText);
    size = 24;
    y += 88;
    i = w->scroll * 4;
    end = i + 4;
    Font_SetSpacing(size, size);
    for (; i < end; i++) {
        if (w->lines[i][0] != 0) {
            width = Str_Length(w->lines[i]);
            width *= size / 2.0f;
            width = 640.0f - width;
            width /= 2.0f;
            Font_SetPos(width, y);
            Text_Printf((s8 *) w->lines[i]);
        }
        y += size + 4;
        size = 18;
        Font_SetSpacing(size, size);
    }
    y = 88;
    y += (size + 4) * 10;
    Font_SetPos(x, y);
    Text_Printf((s8 *) *ruleText);
    if (w->scroll > 0) {
        Font_SetSpacing(28, 28);
        Font_SetPos(306, 120);
        if (gFrameTick & 0x10) {
            Text_PrintfColored(gPlselArrowUp);
        }
    }
    if (w->scroll + 1 < w->lineCount) {
        Font_SetSpacing(28, 28);
        Font_SetPos(306, 290);
        if (gFrameTick & 0x10) {
            Text_PrintfColored(gPlselArrowDown);
        }
    }
    Font_SetSpacing(size, size);
    Font_SetPos(490, 356);
    Text_Printf((s8 *) gPlselTexts[PLSEL_TEXT_PROMPT]);
    Font_SetPos(440, 380);
    sprintf(buf, gPlselTexts[PLSEL_TEXT_COUNTDOWN], w->timeout / 30);
    Text_Printf((s8 *) buf);
}

/*
 * Counts this quest's hunts and captures by monster group (into the group's
 * first kind) and copies them to huntedLeft and capturedLeft, for the
 * bonuses to use up.
 */
void Plsel_CountHunts(PlselWork *w)
{
    s32 i;
    u8 k;

    for (i = 0; i < PLSEL_GROUP_COUNT; i++) {
        w->huntedByGroup[i] = 0;
        w->capturedByGroup[i] = 0;
    }
    for (i = 0; i < PLSEL_GROUP_COUNT; i++) {
        if (gQuest.hunted[i] != 0) {
            k = MonsterGroup_First(i);
            w->huntedByGroup[k] += gQuest.hunted[i];
        }
    }
    for (i = 0; i < PLSEL_GROUP_COUNT; i++) {
        if (gQuest.captured[i] != 0) {
            k = MonsterGroup_First(i);
            w->capturedByGroup[k] += gQuest.captured[i];
        }
    }
    for (i = 0; i < PLSEL_GROUP_COUNT; i++) {
        w->huntedLeft[i] = w->huntedByGroup[i];
        w->capturedLeft[i] = w->capturedByGroup[i];
    }
}

/*
 * If this quest is in `list` (a header word, the system flag, then quest ids
 * up to PLSEL_LIST_END) and the flag isn't set: sets it and queues notice
 * gPlselQuestUnlockIds[idx].
 */
void Plsel_QueueQuestUnlock(u16 *list, s32 idx)
{
    PlselWork *w;
    s32 id;

    w = gPlselWork;
    id = list[1];
    list += 2;
    while (*list != 0xFFFF) {
        if (gQuest.stage == *list) {
            if (EventFlag_Test(id) != 0) {
                return;
            }
            EventFlag_Set(id);
            w->notices[w->noticeCount] = gPlselQuestUnlockIds[idx];
            w->noticeCount++;
            return;
        }
        list++;
    }
}

/*
 * Checks the three quest unlock lists; online, the first clear of quest
 * 15017 sets four system flags and queues four notices.
 */
void Plsel_QueueQuestUnlocks(void)
{
    PlselWork *w;

    w = gPlselWork;
    Plsel_QueueQuestUnlock(gPlselQuestUnlocks0, 0);
    Plsel_QueueQuestUnlock(gPlselQuestUnlocks1, 1);
    Plsel_QueueQuestUnlock(gPlselQuestUnlocks2, 2);
    if (Game_IsOnline() != 0 && gQuest.stage == 15017 && EventFlag_Test(49) != 1) {
        EventFlag_Set(49);
        EventFlag_Set(50);
        EventFlag_Set(51);
        EventFlag_Set(52);
        w->notices[w->noticeCount] = 1792;
        w->noticeCount++;
        w->notices[w->noticeCount] = 2048;
        w->noticeCount++;
        w->notices[w->noticeCount] = 2304;
        w->noticeCount++;
        w->notices[w->noticeCount] = 2560;
        w->noticeCount++;
    }
}
