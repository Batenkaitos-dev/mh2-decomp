/*
 * 0xD67530-0xD74450: the menus on the work area at 0x6AE018, dispatched by
 * Plsel_ResultsUpdate.
 */
#include "common.h"
#include "sprite.h"
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
#include "plsel/unk_D67530.h"   /* the texts this file uses (tools/content_gen.py) */

/* sub_main's result-page texts and the point rate (named in round 22) */
extern char *gDeathPenaltyText;
extern char *gWtdClassCText;
extern char *gHrUpText;
extern char *gHrpNoLossText;
extern char *gHrMaxText;
extern char *gHrpNotMetText;

/* Optimization level 4 (as ending.c and plsel.c): every function that
 * matches at level 3 still matches, and Plsel_CheckTargetBonus and Plsel_UpdateSizeRecords
 * come closer (65 -> 10 and 156 -> 84 words; Plsel_UpdateSizeRecords matches since round 28). */
#pragma optimization_level 4

s32 Gs_SetState(u8 reg, u32 v);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n); /* sub_main: table is a u32 * */
void Plsel_UpdateSizeRecords(void);
void Plsel_CheckTargetBonus(PlselWork *w);
struct ListWin;
void ListPanel_DrawAlpha(struct ListWin *w, s8 *title, s32 sel, u8 alpha); /* sub_main: w is a struct ListWin * */
void Gfx_SetBlend(s32, s32);
void Text_SetSize(s32 size);
void Font_SetSpacing(u8 a, u8 b);
void Font_SetPos(s16, s16);
void Text_Printf(s8 *fmt, ...);
void Text_SetColor(s16 color);
s32 Quest_IsDrill(void);
void func_00D72F10(void);
void Stream_Play(s32 ch, s32 flid);
void Plsel_BuildResultText(s32);
void PlselRes_PlayDecideSe(void);

void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void Stream_FadeTo(s32 ch, s16 frames, s16 level);
void *memcpy(void *, const void *, u32);
void *memset(void *, s32, u32);
extern u8 gPouchBackup[];

void Save_AddMoney(s32 n);
void ScreenCapture_Draw(void);
void Plsel_DrawSaveRecordMenu(void);
void Plsel_DrawTimerWindow(void);
s32 Game_IsOnline(void);
s32 Quest_IsRunning(s32 ending);
s32 Save_TestEventFlag(s32);
s32 Quest_IsTraining(void);
s32 Save_TestListBit(u16);
/* unit's own declaration: with the definition's s32 (s32 arg0) (main/unk_18DA50.c), Plsel_CheckAwards: 160 of 252 words differ, Plsel_CheckAwardsOffline: 162 of 285 words differ ... */
s32 Award_Grant(u16);

void Tex_PatchClut_Unk(void);
void Tex_LoadResultTex_Unk(void);
s32 LoadCompressedFile(u8 *dst, s32 id);
u32 Tex_CreateFromTim2(u8 *tim2, s32 mode);
void Sound_ReloadBank7(void);
s32 Save_AddCounterPoints(void);
s32 SaveFlag_Test(s32);
void SaveFlag_Set(s32);

/* The .bss object at 0xD7BD90 (0xD0 bytes). */
typedef struct PlselRec {
    s16 type;
    s16 id;
    s16 unk4;
    s16 unk6;
} PlselRec;

typedef struct PlselState {
    u8 unk00[0x08];
    s32 unk08;                  /* 0xD7BD98 */
    u8 unk0C[0x04];
    u8 unk10;                   /* 0xD7BDA0 */
    u8 unk11;                   /* 0xD7BDA1 */
    u8 unk12;                   /* 0xD7BDA2 */
    u8 unk13[0x16 - 0x13];
    struct {
        u16 item;
        s16 count;
    } items[20];                /* 0xD7BDA6 */
    u16 unk66;                  /* 0xD7BDF6 */
    s16 unk68;                  /* 0xD7BDF8 */
    PlselRec recs[8];           /* 0xD7BDFA */
    u8 unkAA[0xD0 - 0xAA];
} PlselState;

/* The .bss object at 0xD7BE60 (a countdown and its flags). */
typedef struct PlselTimer {
    u8 unk0;                    /* 0x0: state */
    u8 unk1;
    u8 unk2;
    u8 unk3;
    u8 unk4;
    u8 unk5;
    u8 unk6;
    u8 unk7;
    s16 unk8;
    s8 unkA;
    u8 unkB;
    u8 unkC[4];
} PlselTimer;


s32 EventFlag_Test(s32);
void EventFlag_Set(s32);

void Shake_Start(u8 id, u8 flags, f32 amp);
void Hud_ResetMenuState(void);





/* An entry of sub_main's gRecordCountEntries (4 bytes, ends with id 0). */
typedef struct PlselKind {
    u16 id;
    u16 kind;
} PlselKind;

u16 Record_GroupLargest(SaveData *save, u8 group, u8 *out);
u16 Record_GroupSmallest(SaveData *save, u8 group, u8 *out);

void PlselRes_Close(void);
void PlselRes_BeginClose(void);

void Gfx_SetState63(s32);
void Tex_Prepare(s32, s32);
void Tex_Bind(s32);
s32 Sprite_Draw(Sprite *s);
struct TexRect;
void TexRect_DrawRotated(struct TexRect *r, s32 angle);
void Plsel_QueueQuestUnlock(u16 *, s32);

u16 Rand_Next(s32);
s32 MonsterNote_Record(u16 id, u16 val);
extern u16 D_6ADADE[];

void PlselRes_PlayPageSe(void);
void Plsel_ComputeReward(void);
void Plsel_SumRecordRewards(void);
void Plsel_SumQuestRewards(void);
void Plsel_TallyRewards(void);
void Plsel_CheckAwardsOffline(void);
s32 Plsel_CheckSizeRecords(SceneState *scene, s32 arg1);
void Plsel_CheckSizeAwards(void);

void Plsel_DrawResultsBg(void);
void Text_PrintfColored(const char *fmt, ...);
void PlselRes_DrawArrow(s32, s16, s16, s32);
extern char D_00D7BBB0[];
extern char D_00D7BBB8[];

/* unit's own declaration: with the definition's u8 (Player *p, u16 item, s16 n) (sub_main/pouch.c), Plsel_ItemBoxMenu: 1 of 464 words differ */
u16 Pouch_Add(Player *, u16, s16);
extern Player gPlayers[];

u8 MonsterGroup_First(u8);

char *strcpy(char *dst, const char *src);
char *strcat(char *dst, const char *src);
s32 sprintf(char *, const char *, ...);
extern const char D_00D7BB38[];
extern char **gAwardNoticeLines[];
extern char *gAwardGalleryTitles[];
extern char *gAwardDojoTitles[];
char *strchr(char *, s32);

void Sjis_Widen(char *, char *);
void Text_Draw(char *);

f32 Monster_SizeFromPercent(u8 kind, u16 x);
char *fptodp(f32);
s8 SaveRecord_KindIndex_Unk(u8 *);
extern char *gArenaEmLineFmts[];
extern char *gMonsterNames[];
extern char *gArenaEmRankNames[];

extern char *gHunterTitleDescs[];

void Plsel_DrawMoney(void);
void *Shake_GetOffset(u8 id);
void HelpWindow_DrawCurrent(void);
void Draw_Icons(Icon *icons, u8 count);
void HelpWindow_Draw(u8 page, u16 idx, s8 c);
void Plsel_DrawItemBox(s16 sel, u8 dim, s8 flag);
void ItemList_DrawPouch(s32 a, s32 b, s32 flags);
struct ListPanel;
void ListPanel_Draw(struct ListPanel *b, s16 *pos, char *title, s16 sel);
void Window_DrawText(void *p, s16 *pos, u8 *text);
extern char *D_46A0C8[];

s32 Quest_IsMustOrder(void);
u16 Save_RankFromPoints(SaveData *s);
s32 QuestGoal_IsDone(s32);
s32 AddHrPoints(s32);

void Plsel_CheckRankAwards(s16 arg0);

extern PlselKind gRecordCountEntries[];
s32 Save_IsItemObtained(SaveData *save, u16 item);

void Plsel_CheckVersusAwards(void);


MonsterNoteDef *MonsterNote_FindKind(s32 kind);

s32 Records_HuntedAtLeast(f32 f, SaveData *save, u8 m, u16 n);

s32 QuestGoal_IsUsed(s32);

extern char *gArenaEmPenHelp[];
void Menu_DrawGlowBar(s16 x, s16 y, s16 w, s16 h, u32 color);

u16 Record_GetLargest(SaveData *save, u16 monster);
u16 Record_GetSmallest(SaveData *save, u16 monster);
void Record_SetLargest(SaveData *save, u16 monster, u16 size);
void Record_SetSmallest(SaveData *save, u16 monster, u16 size);

extern char *gResultRuleText;
s32 Str_Length(char *);

extern char D_00D7BBC0[];

s32 Rate_GetSnapshot(s32);

/* .bss (0xD7BD90-0xD7BE80), declared from the highest address down (MWCC
 * emits them last-declared first). */
s32 gPlselShopMoney[4];
PlselTimer gPlselTimer;
PlselState gPlselBonus;

/* .data (0xD78B10-0xD79370) */
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
u16 gPlselBonusTbl[56] = {
#include "plsel/00D78D10.inc"
};
u8 gPlselSizeAwards[240] = {
#include "plsel/00D78D80.inc"
};
u16 gPlselOnlineQuestNotices[16] = {
#include "plsel/00D78E70.inc"
};
extern char *D_0046A0C0[];
ListPanel gPlselTimerWin = {
#include "plsel/00D78E90.inc"
};
Panel gPlselTimerText = {
#include "plsel/00D78EB0.inc"
};
u32 gPlselTimerIconA[2] = {
#include "plsel/00D78EC8.inc"
};
u32 gPlselTimerIconB[2] = {
#include "plsel/00D78ED0.inc"
};
u32 gPlselResultsBgIcon[2] = {
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
u32 gPlselResultsBgDepth[8] = {
#include "plsel/00D79250.inc"
};
extern char *gArenaEmPenMenuTexts[];
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
u32 gPlselQuestUnlocks0[4] = {
#include "plsel/00D79310.inc"
};
u32 gPlselQuestUnlocks1[8] = {
#include "plsel/00D79320.inc"
};
u32 gPlselQuestUnlocks2[10] = {
#include "plsel/00D79340.inc"
};
u16 gPlselQuestUnlockIds[4] = {
#include "plsel/00D79368.inc"
};

extern s32 gRewardMoney;
void Plsel_FlushMoney(void) {
    if (gRewardMoney != 0) {
        Save_AddMoney(gRewardMoney);
        gRewardMoney = 0;
    }
}

void PlselRes_DrawMenus(void) {
    Text_SetSize(4);
    ScreenCapture_Draw();
    if (gScene.step == 1 || gScene.step == 2) {
        Plsel_DrawSaveRecordMenu();
    } else {
        Plsel_DrawTimerWindow();
    }
}

void PlselRes_DrawPanel(void) {
    Text_SetSize(4);
    Gs_SetState(0x60, 0);
    Gfx_SetBlend(4, 5);
    ListPanel_DrawAlpha(gPlselResPanels[0], 0, -1, 0xFF);
}

void Plsel_FadeOutMusic(void) {
    Stream_FadeTo(0, 0x1E, 0);
}

void Plsel_RestorePouch(void) {
    memcpy(gSave.pouch, gPouchBackup, 0x50);
}

void PlselRes_PlayDecideSe(void) {
    Sound_PlaySeFlat(0, 0x13, 0);
}

void PlselRes_PlayPageSe(void) {
    Sound_PlaySeFlat(0, 0x17, 0);
}

void PlselRes_PlayCancelSe(void) {
    Sound_PlaySeFlat(0, 0x14, 0);
}

/* Static in the original: Plsel_UpdateSizeRecords keeps t1-t3 across calls to it,
 * which MWCC only does for a static function it has already compiled. */
static void Plsel_AddRecordGroup(s32 g) {
    PlselWork *w;
    s32 i;

    w = gPlselWork;
    for (i = 0; i < w->recordGroupCount; i++) {
        if (w->recordGroups[i] == g) {
            return;
        }
    }
    w->recordGroups[w->recordGroupCount] = g;
    w->recordGroupCount++;
}

/* Round 23: newRecord is set before Plsel_AddRecordGroup in the second largest
 * test (the game's store is in the jal slot), and the third loop has its own
 * record variable (rec) and counts with j. Round 28: three compares are written
 * with the j entry first (`a[j] > a[g]`): MWCC creates a relational compare's
 * operands in source order, which gives the game's t1/t2 (docs/mwcc-matching.md, "How
 * MWCC colours temporaries"). */
void Plsel_UpdateSizeRecords(void) {
    PlselWork *w;
    s32 i;
    s32 j;
    s32 g;
    u16 v;
    u16 rec;

    w = gPlselWork;
    for (j = 1; j < 0x4A; j++) {
        g = MonsterGroup_First(j);
        w->largestBefore[g] = Record_GroupLargest(&gSave, j, NULL);
        w->smallestBefore[g] = Record_GroupSmallest(&gSave, j, NULL);
    }
    for (i = 1; i < 0x4A; i++) {
        if (gQuest.sizeMax[i] > 0) {
            w->questLargest[i] = gQuest.sizeMax[i];
        }
        if (gQuest.sizeMin[i] > 0) {
            w->questSmallest[i] = gQuest.sizeMin[i];
        }
        v = Record_GetLargest(&gSave, i);
        if (v < gQuest.sizeMax[i]) {
            Record_SetLargest(&gSave, i, gQuest.sizeMax[i]);
        }
        v = Record_GetSmallest(&gSave, i);
        if (gQuest.sizeMin[i] > 0) {
            if (v == 0) {
                Record_SetSmallest(&gSave, i, gQuest.sizeMin[i]);
            } else if (gQuest.sizeMin[i] < v) {
                Record_SetSmallest(&gSave, i, gQuest.sizeMin[i]);
            }
        }
        gQuest.sizeMax[i] = 0;
        gQuest.sizeMin[i] = 0;
    }
    for (j = 1; j < 0x4A; j++) {
        g = MonsterGroup_First(j);
        if (w->questLargest[j] > 0) {
            rec = Record_GroupLargest(&gSave, j, NULL);
            if (rec > 0) {
                if (w->largestBefore[g] == 0) {
                    if (w->largestNew[g] != 0) {
                        if (w->questLargest[j] > w->questLargest[g]) {
                            w->questLargest[g] = w->questLargest[j];
                            w->questLargest[j] = 0;
                            w->largestNew[g] = 1;
                            w->newRecord = 1;
                            Plsel_AddRecordGroup(g);
                        }
                    } else {
                        w->largestNew[g] = 1;
                        w->newRecord = 1;
                        w->questLargest[g] = rec;
                        Plsel_AddRecordGroup(g);
                    }
                }
                if (w->largestBefore[g] < rec) {
                    if (w->largestNew[g] != 0) {
                        if (w->questLargest[j] > w->questLargest[g]) {
                            w->questLargest[g] = w->questLargest[j];
                            w->questLargest[j] = 0;
                            w->largestNew[g] = 2;
                            w->newRecord = 1;
                            Plsel_AddRecordGroup(g);
                        }
                    } else {
                        w->largestNew[g] = 2;
                        w->newRecord = 1;
                        w->questLargest[g] = rec;
                        Plsel_AddRecordGroup(g);
                    }
                } else {
                    if (j != g && w->questLargest[j] > w->questLargest[g]) {
                        w->questLargest[g] = w->questLargest[j];
                        w->questLargest[j] = 0;
                    }
                    Plsel_AddRecordGroup(g);
                }
            }
        }
        if (w->questSmallest[j] > 0) {
            v = Record_GroupSmallest(&gSave, j, NULL);
            if (v > 0) {
                if (w->smallestBefore[g] == 0) {
                    if (w->smallestNew[g] != 0) {
                        if (w->questSmallest[j] < w->questSmallest[g]) {
                            w->questSmallest[g] = w->questSmallest[j];
                            w->questSmallest[j] = 0;
                            w->smallestNew[g] = 1;
                            w->newRecord = 1;
                            Plsel_AddRecordGroup(g);
                        }
                    } else {
                        w->smallestNew[g] = 1;
                        w->newRecord = 1;
                        w->questSmallest[g] = v;
                        Plsel_AddRecordGroup(g);
                    }
                }
                if (v < w->smallestBefore[g]) {
                    if (w->smallestNew[g] != 0) {
                        if (w->questSmallest[j] < w->questSmallest[g]) {
                            w->questSmallest[g] = w->questSmallest[j];
                            w->questSmallest[j] = 0;
                            w->smallestNew[g] = 2;
                            w->newRecord = 1;
                            Plsel_AddRecordGroup(g);
                        }
                    } else {
                        w->smallestNew[g] = 2;
                        w->newRecord = 1;
                        w->questSmallest[g] = v;
                        Plsel_AddRecordGroup(g);
                    }
                } else {
                    if (j != g) {
                        if (w->questSmallest[g] == 0) {
                            w->questSmallest[g] = w->questSmallest[j];
                        } else if (w->questSmallest[j] < w->questSmallest[g]) {
                            w->questSmallest[g] = w->questSmallest[j];
                        }
                        w->questSmallest[j] = 0;
                    }
                    Plsel_AddRecordGroup(g);
                }
            }
        }
    }
}

s32 Plsel_CheckSizeRecords(SceneState *scene, s32 unused) {
    s32 i;
    u16 rec;

    for (i = gPlselBonus.unk12 + 1; i < 0x4A; i++) {
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
void Plsel_CheckTargetBonus(PlselWork *w) {
    Player *pl;
    s32 n;
    s32 i;
    s32 j;
    s32 k;

    gPlselBonus.unk11 = 0;
    n = 0;
    pl = &gPlayers[gScene.localPlayer];
    memset(gPlselBonus.recs, 0, 0x40);
    for (i = 0; i < 2; i++) {
        if (gQuest.huntGoals[i].type != 0 && gQuest.huntGoals[i].kind != 0) {
            k = MonsterGroup_First(gQuest.huntGoals[i].kind);
            if (gQuest.huntGoals[i].type == 1) {
                if (w->huntedLeft[k] >= gQuest.huntGoals[i].need) {
                    gPlselBonus.recs[n].type = 1;
                    gPlselBonus.recs[n].id = k;
                    if (i == 0) {
                        gPlselBonus.recs[n].unk4 = w->huntedLeft[k] / gQuest.huntGoals[i].need;
                        w->huntedLeft[k] -= (u16)(gPlselBonus.recs[n].unk4 * gQuest.huntGoals[i].need);
                    } else {
                        gPlselBonus.recs[n].unk4 = 1;
                        w->huntedLeft[k] = 0;
                    }
                    gPlselBonus.recs[n].unk6 = gQuest.huntGoals[i].reward;
                    n++;
                    gPlselBonus.unk11 = 1;
                }
            } else if (gQuest.huntGoals[i].type == 2) {
                if (w->capturedLeft[k] >= gQuest.huntGoals[i].need) {
                    gPlselBonus.recs[n].type = 2;
                    gPlselBonus.recs[n].id = k;
                    if (i == 0) {
                        gPlselBonus.recs[n].unk4 = w->capturedLeft[k] / gQuest.huntGoals[i].need;
                        w->capturedLeft[k] -= (u16)(gPlselBonus.recs[n].unk4 * gQuest.huntGoals[i].need);
                    } else {
                        gPlselBonus.recs[n].unk4 = 1;
                        w->capturedLeft[k] = 0;
                    }
                    gPlselBonus.recs[n].unk6 = gQuest.huntGoals[i].reward;
                    n++;
                    gPlselBonus.unk11 = 1;
                }
            }
        }
    }
    for (i = 0; i < 0x4A; i++) {
        if (n >= 8) {
            break;
        }
        k = MonsterGroup_First(i);
        if (Rate_GetSnapshot(k) != 0 && w->huntedLeft[k] + w->capturedLeft[k] > 0) {
            gPlselBonus.recs[n].type = 0;
            gPlselBonus.recs[n].id = i;
            gPlselBonus.recs[n].unk4 = w->huntedLeft[k] + w->capturedLeft[k];
            gPlselBonus.recs[n].unk6 = Rate_GetSnapshot(k);
            gPlselBonus.unk11 = 1;
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
                gPlselBonus.recs[n].unk4 = gQuest.huntGoals[2].need;
                gPlselBonus.recs[n].unk6 = gQuest.huntGoals[2].reward;
                Pouch_Add(pl, pl->pouch[j].item, -gQuest.huntGoals[2].need);
                gPlselBonus.unk11 = 1;
            }
        }
    }
}

s32 Plsel_TakeQuestItems(void) {
    Player *pl;
    s32 j;
    s32 n;
    ItemData *items;
    u16 id;
    s32 i;

    n = 0;
    gPlselBonus.unk10 = 0;
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
            gPlselBonus.unk10 = 1;
            n++;
        }
    }
    return n;
}

extern u16 gQuestEmSizePct;
s32 Plsel_RollBonus(void) {
    u16 *p;
    u16 cur;
    u16 v;

    if (Quest_IsDrill() == 1) {
        return 0;
    }
    if (Quest_IsTraining() == 1) {
        return 0;
    }
    if (gQuestEmSizePct == 0) {
        return 0;
    }
    if (gScene.questPaid == 0) {
        return 0;
    }
    if (Save_TestEventFlag(0xC4) == 0) {
        return 0;
    }
    cur = gQuestEmSizePct;
    p = gPlselBonusTbl;
    v = *p;
    while (v != cur) {
        if (v == 0xFFFF) {
            return 0;
        }
        p += 2;
        v = *p;
    }
    if (cur == 0x35 && SaveFlag_Test(0x8D) == 0) {
        return 0;
    }
    if ((Rand_Next(1) & 0x1F) < p[1]) {
        return 1;
    }
    return 0;
}

s32 Plsel_CheckStoryScene(void) {
    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (Quest_IsRunning(1) != 1) {
        return 0;
    }
    if (Save_TestEventFlag(0x184) == 0) {
        return 0;
    }
    if (Save_TestEventFlag(0x187) == 1) {
        return 0;
    }
    if (Save_TestEventFlag(0x185) == 1) {
        return 0;
    }
    if (gSave.unk36E8 == 0) {
        return 1;
    }
    return 0;
}

s32 Plsel_CheckStoryScene2(void) {
    if (Game_IsOnline() == 1) {
        return 0;
    }
    if (Quest_IsRunning(1) != 1) {
        return 0;
    }
    if (gQuest.stage != 0x138F) {
        return 0;
    }
    if (Save_TestEventFlag(0x58) == 1) {
        return 0;
    }
    return 1;
}

void FadeA_Start(s32);
void Sound_Silence(void);
void QuestRecord_Fill(void);
void QuestStats_Tally(void);
void Save_RestorePouch(Player *);
void Save_StorePouch(Player *);
void Rand_StepSaved(void);
void Rate_ListAdd(s16 kind, s8 n); /* unit's own declaration: the definition (main) takes s32; Plsel_ResultsUpdate is 842 words off with it */
void Player_BackupPouch(Player *, s32);
void Versus_ConsumeTeamItem(void);
void Quest_UpdateSaveRecords(s32);
void Save_SetListBit(u16);
void QuestEm_SavePersistentHp(void);
void Quest_RollRewards(void);
void Stream_FadeOut(s32, s32);
void Stream_FadeToMax(s32, s16);
void Game_ResetSubsystems(void);
void Plsel_QueueFlagUnlocks(void);
void Plsel_LoadResultsTextures(void);
void Plsel_SnapshotVersusMonster(void);
void Plsel_CheckTrainingAwards(void);
void Plsel_CheckAwards(void);
void Plsel_AddCounterPoints(void);
void Plsel_QueueQuestUnlocks(void);
void Plsel_QueueOnlineQuestNotice(void);
void Plsel_InitSaveRecordMenu(void);
void Plsel_InitItemBoxMenu(void);
void PlselRes_InitMonsterPage(void);
void Plsel_ComputeAllRewards(void);
s32 Quest_IsVersus(void);
void PlselRes_InitPointsPage(void);
void Plsel_BuildItemText(void);
void Plsel_DrawItemText(void);
void PlselRes_InitRecordPage(void);
void Plsel_BuildAwardText(void);
void Plsel_DrawAwardText(void);
void Plsel_DrawResultPage(s32);
s32 PlselRes_RewardPageInput(u8 *);
s32 PlselRes_RecordPageInput(u8 *);
s32 PlselRes_PointsPageInput(u8 *);
s32 PlselRes_MonsterPageInput(SceneState *sc);
s32 Plsel_SaveRecordMenu(void);
s32 Plsel_HasQueuedAwards(void);
s32 PlselRes_AwardPageInput(void);
s32 PlselRes_ItemPageInput(void);
struct PlselRewTask;
s32 Plsel_RewardListTask(struct PlselRewTask *);
void Plsel_RewardListDrawText(struct PlselRewTask *);
void Plsel_ComputeRewards(Player *);
extern u8 gNetMenuDrawHooks[];

void PlselRes_DrawCommError(void);
s16 Plsel_ItemBoxMenu(void);
void Plsel_CountHunts(PlselWork *);

extern s16 D_6ADB14;
extern char *gRecordDashesText;
extern void *D_006A3A24;
/* The quest results screen: its steps (SceneState.state) from the tally at
 * the quest's end through the rewards, items and awards pages to the next
 * scene. */
void Plsel_ResultsUpdate(void) {
    SceneState *sc;
    PlselWork *w;
    Player *pl;
    s32 i;

    w = gPlselWork;
    sc = &gScene;
    Text_SetSize(4);
    switch (gScene.step) {
    case 0:
        memset(&gPlselBonus, 0, 0xC4);
        Sound_Silence();
        sc->stateSyncFrame = 0;
        gAreaState.drawSky = 0;
        gPlselBonus.unk12 = 0;
        for (i = 0; i < 2; i++) {
            memset(&gNetMenuDrawHooks[i * 0x20], 0, 0x20);
        }
        QuestRecord_Fill();
        QuestStats_Tally();
        memset(w, 0, 0x3B88);
        Plsel_CountHunts(w);
        if (Quest_IsDrill() == 1) {
            w->unk3B83 = Save_TestListBit(gQuest.stage);
            Plsel_ComputeRewards(&gPlayers[sc->localPlayer]);
            Save_RestorePouch(&gPlayers[sc->localPlayer]);
        }
        if (Quest_IsTraining() == 1) {
            Save_RestorePouch(&gPlayers[sc->localPlayer]);
        }
        if (gScene.questPhase == 8) {
            sc->step = 0x17;
            sc->subStep = 0;
            sc->timer = 0x96;
            sc->timer2 = 1;
            Plsel_FlushMoney();
            FadeA_Start(2);
            *(void (**)(void))0x6A3A24 = PlselRes_DrawPanel;
            Plsel_RestorePouch();
            break;
        }
        if (gScene.questPhase == 7) {
            Plsel_QueueFlagUnlocks();
            Plsel_FlushMoney();
            Plsel_RestorePouch();
            if (Plsel_HasQueuedAwards() == 0) {
                sc->step = 0x19;
                w->timeout = 0xE10;
                sc->timer = 0x1E;
                FadeA_Start(0xA);
                Plsel_FadeOutMusic();
            } else {
                Plsel_LoadResultsTextures();
                sc->step = 0x10;
                sc->subStep = 0;
                *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
                FadeA_Start(2);
            }
            return;
        }
        Plsel_QueueFlagUnlocks();
        Rand_StepSaved();
        i = gRewardMoney;
        *(void (**)(void))0x6A3A24 = PlselRes_DrawMenus;
        gPlselBonus.unk08 = i;
        gRewardMoney = 0;
        for (i = 0; i < 20; i++) {
            pl = &gPlayers[sc->localPlayer];
            if (gItemData[pl->pouch[i].item].flags2 & 4) {
                Pouch_Add(pl, pl->pouch[i].item, -pl->pouch[i].count);
            }
        }
        Plsel_CheckTargetBonus(w);
        Plsel_TakeQuestItems();
        *(void (**)(void))0x6A3A24 = PlselRes_DrawMenus;
        if (Quest_IsTraining() != 1 && Quest_IsDrill() != 1) {
            for (i = 0; i < 0x4A; i++) {
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
        if (Quest_IsRunning(1) == 1) {
            Quest_UpdateSaveRecords(0);
            if (Game_IsOnline() == 1) {
                if (Quest_IsMustOrder() == 0 || gScene.questPaid != 0) {
                    Save_SetListBit(gQuest.stage);
                }
            } else {
                Save_SetListBit(gQuest.stage);
            }
            if (Quest_IsTraining() == 1) {
                Plsel_CheckTrainingAwards();
            }
            if (Quest_IsDrill() == 1) {
                Plsel_CheckAwards();
                Plsel_CheckSizeRecords(&gScene, 2);
                Plsel_CheckSizeAwards();
            }
            Plsel_AddCounterPoints();
            if (QuestGoal_IsDone(0) == 1) {
                Plsel_QueueQuestUnlocks();
                Plsel_QueueOnlineQuestNotice();
            }
            QuestEm_SavePersistentHp();
            Quest_RollRewards();
            if (Game_IsOnline() == 1 && Plsel_RollBonus() == 1) {
                sc->step = 1;
                sc->subStep = 0;
                Plsel_InitSaveRecordMenu();
            } else {
                sc->step = 3;
                sc->subStep = 0;
                Plsel_InitItemBoxMenu();
            }
            sc->timer = 0x1E;
            FadeA_Start(2);
        } else {
            Quest_UpdateSaveRecords(1);
            Plsel_AddCounterPoints();
            if (Quest_IsDrill() == 1) {
                Plsel_LoadResultsTextures();
                Plsel_CheckAwards();
                Plsel_CheckSizeRecords(&gScene, 2);
                Plsel_CheckSizeAwards();
                sc->step = 0xA;
                sc->subStep = 0;
                w->timeout = 0xE10;
                w->closeState = 0;
                *(void (**)(void))0x6A3A24 = NULL;
                break;
            }
            if (Game_IsOnline() == 1) {
                if (Quest_IsVersus() == 1) {
                    Plsel_LoadResultsTextures();
                    Save_StorePouch(&gPlayers[sc->localPlayer]);
                    Plsel_CheckVersusAwards();
                    PlselRes_InitMonsterPage();
                    if (D_6ADB14 == -1) {
                        sc->step = 0xA;
                    } else {
                        sc->step = 0x15;
                    }
                    sc->subStep = 0;
                    w->timeout = 0xE10;
                    w->closeState = 0;
                    *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
                    sc->timer = 0x1E;
                    FadeA_Start(2);
                    break;
                }
                if (Plsel_RollBonus() == 1) {
                    sc->step = 1;
                    sc->subStep = 0;
                    Plsel_InitSaveRecordMenu();
                    sc->timer = 0x1E;
                    FadeA_Start(2);
                    break;
                }
            }
            Plsel_LoadResultsTextures();
            sc->step = 5;
            Save_StorePouch(&gPlayers[sc->localPlayer]);
            Plsel_ComputeAllRewards();
            *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
            sc->timer = 0x1E;
            FadeA_Start(2);
        }
        break;
    case 3:
        if (Plsel_ItemBoxMenu() <= 0) {
            sc->step++;
            FadeA_Start(1);
            sc->timer = 0x1E;
            Stream_FadeOut(0, 0x1E);
            Save_StorePouch(&gPlayers[sc->localPlayer]);
        }
        break;
    case 4:
        if (--sc->timer <= 0) {
            Plsel_LoadResultsTextures();
            Stream_FadeToMax(0, 0x1E);
            if (Quest_IsDrill() == 1) {
                sc->step = 0x14;
                sc->subStep = 0;
                FadeA_Start(2);
                sc->timer = 0x1E;
            } else {
                if (Quest_IsVersus() == 1) {
                    Plsel_CheckAwardsOffline();
                    if (D_6ADB14 == -1) {
                        sc->step = 0xA;
                        sc->subStep = 0;
                        w->timeout = 0xE10;
                        w->closeState = 0;
                    } else {
                        sc->step = 0x15;
                        sc->subStep = 0;
                        PlselRes_InitMonsterPage();
                    }
                } else {
                    sc->step = 5;
                    sc->subStep = 0;
                    Plsel_ComputeAllRewards();
                }
                *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
                FadeA_Start(2);
                sc->timer = 0x1E;
            }
        }
        break;
    case 5:
        i = PlselRes_RewardPageInput(&sc->scene);
        if (sc->timer != 0) {
            sc->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sc->step++;
                FadeA_Start(1);
                sc->timer = 0x1E;
            }
        }
        Plsel_DrawResultPage(0);
        break;
    case 6:
        if (--sc->timer <= 0) {
            w->timeout = 0xE10;
            sc->step = 7;
            sc->subStep = 0;
        } else {
            Plsel_DrawResultPage(0);
        }
        break;
    case 7:
        if (Game_IsOnline() != 1) {
            sc->step = 0xA;
        } else {
            sc->step = 8;
            w->timeout = 0xE10;
            w->closeState = 0;
            PlselRes_InitPointsPage();
            FadeA_Start(2);
            sc->timer = 0x1E;
            *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
            if (w->rankChange == 1) {
                Stream_Play(1, 0x1D);
            }
        }
        break;
    case 8:
        i = PlselRes_PointsPageInput(&sc->scene);
        if (sc->timer != 0) {
            sc->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sc->step++;
                FadeA_Start(1);
                sc->timer = 0x1E;
            }
        }
        Plsel_DrawResultPage(0);
        break;
    case 9:
        if (--sc->timer <= 0) {
            sc->step = 0xA;
            sc->subStep = 0;
            w->timeout = 0xE10;
            w->closeState = 0;
        } else {
            Plsel_DrawResultPage(0);
        }
        break;
    case 10:
        if (w->awardCount == 0) {
            sc->step = 0xD;
            sc->subStep = 0;
            w->timeout = 0xE10;
            w->closeState = 0;
            *(void (**)(void))0x6A3A24 = NULL;
            FadeA_Start(2);
        } else {
            sc->step = 0xB;
            w->timeout = 0xE10;
            w->closeState = 0;
            Plsel_BuildItemText();
            *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
            FadeA_Start(2);
            sc->timer = 0x1E;
            Stream_Play(1, 0x1D);
        }
        break;
    case 11:
        i = PlselRes_ItemPageInput();
        if (sc->timer != 0) {
            sc->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sc->step++;
                FadeA_Start(1);
                sc->timer = 0x1E;
            }
        }
        Plsel_DrawItemText();
        break;
    case 12:
        if (--sc->timer <= 0) {
            sc->step = 0xD;
            sc->subStep = 0;
            w->timeout = 0xE10;
            w->closeState = 0;
            *(void (**)(void))0x6A3A24 = NULL;
        } else {
            Plsel_DrawItemText();
        }
        break;
    case 13:
        if (w->recordGroupCount == 0) {
            sc->step = 0x10;
            sc->subStep = 0;
            w->timeout = 0xE10;
            w->closeState = 0;
            FadeA_Start(2);
        } else {
            PlselRes_InitRecordPage();
            sc->step = 0xE;
            sc->subStep = 0;
            w->timeout = 0xE10;
            w->closeState = 0;
            FadeA_Start(2);
            sc->timer = 0x1E;
            *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
        }
        break;
    case 14:
        i = PlselRes_RecordPageInput(&sc->scene);
        if (sc->timer != 0) {
            sc->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sc->step++;
                FadeA_Start(1);
                sc->timer = 0x1E;
            }
        }
        Plsel_DrawResultPage(0);
        break;
    case 15:
        if (--sc->timer <= 0) {
            sc->step = 0x10;
            sc->subStep = 0;
            w->timeout = 0xE10;
            w->closeState = 0;
        } else {
            Plsel_DrawResultPage(0);
        }
        break;
    case 19:
        sc->step = 0x14;
        break;
    case 20:
        if (Plsel_RewardListTask((struct PlselRewTask *)sc) != 0) {
            sc->step = 0xA;
            sc->subStep = 0;
            w->timeout = 0xE10;
            w->closeState = 0;
        } else {
            Plsel_RewardListDrawText((struct PlselRewTask *)sc);
        }
        break;
    case 16:
        if (Plsel_HasQueuedAwards() == 0) {
            w->timeout = 0xE10;
            sc->timer = 0x1E;
            Plsel_FadeOutMusic();
            sc->step = 0x19;
            *(void (**)(void))0x6A3A24 = NULL;
        } else {
            Plsel_BuildAwardText();
            sc->step = 0x11;
            w->timeout = 0xE10;
            w->closeState = 0;
            sc->timer = 0x1E;
            FadeA_Start(2);
            *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
        }
        break;
    case 17:
        i = PlselRes_AwardPageInput();
        if (sc->timer != 0) {
            sc->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sc->step = 0x12;
                sc->timer = 0x1E;
                FadeA_Start(0xA);
            }
        }
        Plsel_DrawAwardText();
        break;
    case 18:
        if (sc->timer != 0) {
            sc->timer--;
            Plsel_DrawAwardText();
        } else {
            sc->timer = 0x1E;
            w->timeout = 0xE10;
            Plsel_FadeOutMusic();
            sc->step = 0x19;
        }
        break;
    case 1:
        if (Plsel_SaveRecordMenu() <= 0) {
            sc->step++;
            FadeA_Start(1);
            sc->timer = 0x1E;
        }
        break;
    case 2:
        if (--sc->timer <= 0) {
            if (Quest_IsRunning(1) == 1) {
                Plsel_InitItemBoxMenu();
                sc->step = 3;
                sc->subStep = 0;
                FadeA_Start(2);
                sc->timer = 0x1E;
            } else {
                Plsel_LoadResultsTextures();
                sc->step = 5;
                sc->subStep = 0;
                Save_StorePouch(&gPlayers[sc->localPlayer]);
                Plsel_ComputeAllRewards();
                *(void (**)(void))0x6A3A24 = Plsel_DrawResultsBg;
                sc->timer = 0x1E;
                FadeA_Start(2);
            }
        }
        break;
    case 21:
        i = PlselRes_MonsterPageInput(sc);
        if (sc->timer != 0) {
            sc->timer--;
        } else {
            w->timeout--;
            if (w->timeout == 0 || i != 0) {
                sc->step++;
                FadeA_Start(1);
                sc->timer = 0x1E;
            }
        }
        Plsel_DrawResultPage(0);
        break;
    case 22:
        if (--sc->timer <= 0) {
            sc->step = 0xA;
            sc->subStep = 0;
            w->timeout = 0xE10;
            w->closeState = 0;
        } else {
            Plsel_DrawResultPage(0);
        }
        break;
    case 23:
        if (--sc->timer <= 0) {
            sc->step++;
            FadeA_Start(1);
        }
        PlselRes_DrawCommError();
        break;
    case 24:
        if (--sc->timer <= 0) {
            sc->scene = 6;
            sc->step = 0;
            Game_ResetSubsystems();
            return;
        }
        PlselRes_DrawCommError();
        break;
    case 25:
        if (--sc->timer <= 0) {
            gSysSettings.unkE0 = 0;
            if (Plsel_CheckStoryScene() == 1) {
                gSysSettings.unkE0 = 1;
                sc->scene = 7;
                sc->step = 0;
                sc->subStep = 0;
                Game_ResetSubsystems();
                return;
            }
            if (Plsel_CheckStoryScene2() == 1) {
                sc->scene = 7;
                sc->step = 0;
                sc->subStep = 1;
                Game_ResetSubsystems();
                return;
            }
            sc->scene = 6;
            sc->step = 0;
            Game_ResetSubsystems();
            return;
        }
        break;
    }
    if (D_006A3A24 != NULL) {
        OT_AddAt(gDrawListText4, gNetMenuDrawHooks, 0, 1);
    }
}

extern s32 D_00469E68;
extern s32 D_00469E6C;
extern s32 D_00469E70;
extern s32 D_00469E74;
extern s32 D_00469E78;
void PlselRes_DrawCommError(void) {
    Text_SetColor(0);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(0x113, 0x61);
    Text_Printf((s8 *) D_00469E68);
    Font_SetPos(0xB0, 0x85);
    Text_Printf((s8 *) D_00469E6C);
    Font_SetPos(0xB0, 0x97);
    Text_Printf((s8 *) D_00469E70);
    Font_SetPos(0xB0, 0xA9);
    Text_Printf((s8 *) D_00469E74);
    Font_SetPos(0xB0, 0xBB);
    Text_Printf((s8 *) D_00469E78);
}

void Plsel_InitItemBoxMenu(void) {
    *(s32 *)&gPlselTimer = 0;
    gPlselTimer.unk4 = 0;
    gPlselTimer.unk5 = 2;
    gPlselTimer.unkA = -1;
    gPlselTimer.unkB = 0xFF;
    gPlselTimer.unk8 = 0xE10;
    gPlselTimer.unk6 = 0;
    gHelpShow = 0;
    gHelpPage = 6;
    gHelpColumn = 0;
    Shake_Start(6, 0, -180.0f);
    Hud_ResetMenuState();
}

/* gPlselTimer with the object after it (gPlselShopMoney), as the shop menu uses
 * them: one 0x20-byte work area. */
typedef struct PlselShop {
    PlselTimer t;               /* 0x00 */
    s32 money;                  /* 0x10 */
    s32 flag;                   /* 0x14 */
} PlselShop;

u16 Menu_ReadDpad(u8 pad);
s32 Shake_Update(u8 id);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
s16 Pouch_Space(Player *, u16);
u16 Plsel_ItemBoxHandlePad(u8 *p, u16 pad);
void Pouch_MoveCursor(u8 *, u16);
s16 Pouch_ClearSlot(Player *p, s16 slot);

/* The item box menu after a quest (the box items to the pouch, selling). */
s16 Plsel_ItemBoxMenu(void) {
    PlselShop *t;
    Player *pl;
    u16 r;
    u16 item;
    u16 pad;
    s16 sp;
    ItemStack tmp;

    pl = &((Player *)gPlayers)[gScene.localPlayer];
    t = (PlselShop *)&gPlselTimer;
    gPlselTimer.unk8--;
    pad = gPadState[0].pressed | (gPadState[0].repeat & 0x3C00);
    pad |= Menu_ReadDpad(0);
    switch (gPlselTimer.unk0) {
    case 0:
        if (Shake_Update(6) == 1) {
            pad &= 0x3C40;
        }
        Menu_CursorUpDown(&t->t.unk1, pad, t->t.unk5);
        if (pad & 0x20) {
            switch (t->t.unk1) {
            case 0:
                gHelpShow = 1;
                gHelpLine = 0;
                break;
            case 1:
                if (Quest_IsDrill() == 1) {
                    PlselRes_PlayDecideSe();
                    t->t.unk0 = 2;
                    return 0;
                }
                if (t->t.unk4 == 0) {
                    gHelpShow = 1;
                    gHelpLine = 3;
                    t->t.unkC[0] = 1;
                    break;
                }
                t->t.unk0 = 2;
                return 0;
            case 2:
                if (t->t.unk8 < 300) {
                    t->t.unk8 = 300;
                }
                break;
            }
            t->t.unk0++;
            t->t.unk3 = 0;
            t->t.unk2 = 0;
            PlselRes_PlayDecideSe();
        }
        break;
    case 1:
        switch (t->t.unk1) {
        case 0:
            switch (t->t.unk2) {
            case 0:
                gHelpLine = 0;
                r = Plsel_ItemBoxHandlePad((u8 *)t, pad);
                if (r & 0x40) {
                    break;
                }
                item = gScene.supply[t->t.unk6].item;
                sp = Pouch_Space(pl, item);
                if (sp == 0xFF || sp == 0) {
                    t->t.unkA = 0;
                } else {
                    t->t.unkA = -1;
                }
                if (r & 0x20) {
                    if (item != 0 && t->t.unkA < 0) {
                        switch (Pouch_Add(pl, item, gScene.supply[t->t.unk6].count)) {
                        case 0:
                        case 1:
                            gScene.supply[t->t.unk6].item = 0;
                            gScene.supply[t->t.unk6].count = 0;
                            Sound_PlaySeFlat(0, 0x19, 0);
                            break;
                        case 2:
                            gScene.supply[t->t.unk6].count -= sp;
                            Sound_PlaySeFlat(0, 0x19, 0);
                            break;
                        case 3:
                            Sound_PlaySeFlat(0, 0x15, 0);
                            break;
                        case 5:
                            gHelpLine = 1;
                            t->t.unk7 = 0;
                            t->t.unk2 = 1;
                            t->t.unk3 = 0;
                            Sound_PlaySeFlat(0, 0x13, 0);
                            break;
                        }
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                }
                break;
            case 1:
                if (t->t.unk3 != 0) {
                    if (pad & 0x240) {
                        t->t.unk3 = 0;
                        Sound_PlaySeFlat(0, 0x14, 0);
                    }
                    pad &= ~0x40;
                } else {
                    if (pad & 0x200) {
                        t->t.unk3 = 2;
                        Sound_PlaySeFlat(0, 9, 0);
                    }
                    if (pad & 0x40) {
                        gHelpLine = 0;
                        t->t.unk2 = 0;
                        Sound_PlaySeFlat(0, 0x14, 0);
                        break;
                    }
                }
                gHelpLine = 1;
                Pouch_MoveCursor(&t->t.unk7, pad);
                if (pad & 0x20) {
                    /* the box as u16 pairs, as the plsel loops read it: &gScene.supply[k]
                     * folds the 0x140 into the loads and stores */
                    ItemStack *e = (ItemStack *)((u16 *)gScene.supply + t->t.unk6 * 2);

                    tmp = *e;
                    *e = pl->pouch[t->t.unk7];
                    Pouch_ClearSlot(pl, t->t.unk7);
                    Pouch_Add(pl, tmp.item, tmp.count);
                    gHelpLine = 2;
                    t->t.unk2++;
                    t->t.unk3 = 0;
                    Sound_PlaySeFlat(0, 0x13, 0);
                }
                break;
            case 2:
                if (pad & 0x20) {
                    gHelpLine = 0;
                    t->t.unkB = 0xFF;
                    t->t.unk2 = 0;
                    Sound_PlaySeFlat(0, 0x13, 0);
                } else {
                    gHelpLine = 2;
                    t->t.unkB++;
                }
                break;
            }
            break;
        case 1:
            switch (t->t.unk2) {
            case 0:
                gHelpLine = 3;
                if ((t->t.unkC[0] == 0 && (pad & 0x400)) || (t->t.unkC[0] == 1 && (pad & 0x800))) {
                    t->t.unkC[0] ^= 1;
                    Sound_PlaySeFlat(0, 0x17, 0);
                    break;
                }
                if (pad & 0x40) {
                cancel:
                    gHelpShow = 0;
                    t->t.unk0 = 0;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else if (pad & 0x20) {
                    if (t->t.unkC[0] == 1) {
                        Sound_PlaySeFlat(0, 0x13, 0);
                        goto cancel;
                    }
                    if (t->t.unkC[0] == 0) {
                        if (SaveFlag_Test(0x1E) == 0) {
                            t->flag = 1;
                        } else {
                            t->flag = 0;
                        }
                        Save_AddMoney(t->money);
                        if (SaveFlag_Test(0x1E) == 0) {
                            t->flag = 0;
                        }
                        t->t.unk2 = 1;
                        Sound_PlaySeFlat(0, 0x13, 0);
                    }
                }
                break;
            case 1:
                t->t.unk8 = 0;
                break;
            }
            break;
        case 2:
            t->t.unk0 = 2;
            break;
        }
        break;
    case 2:
        t->t.unk8 = 0;
        break;
    }
    return t->t.unk8;
}

void Plsel_MoveGridCursor(u8 *p, u16 pad) {
    u8 old;
    u8 x;
    u8 y;
    u8 v;

    old = *p;
    x = old;
    y = old >> 3;
    if (pad & 0x800) {
        x--;
    }
    if (pad & 0x400) {
        x++;
    }
    if (pad & 0x2000) {
        y--;
    }
    if (pad & 0x1000) {
        y++;
    }
    if (y == 0xFF) {
        y = 5;
    }
    v = (x & 7) | ((y % 6) << 3);
    if (old != v) {
        *p = v;
        Sound_PlaySeFlat(0, 0x17, 0);
    }
}

u16 Plsel_ItemBoxHandlePad(u8 *p, u16 pad) {
    if (p[3] != 0) {
        if (pad & 0x240) {
            p[3] = 0;
            Sound_PlaySeFlat(0, 0x14, 0);
        }
        pad &= ~0x40;
    } else if (pad & 0x200) {
        p[3] = 1;
        Sound_PlaySeFlat(0, 9, 0);
    }
    if (pad & 0x40) {
        gHelpShow = 0;
        p[0] = 0;
        Sound_PlaySeFlat(0, 0x14, 0);
        return 0x40;
    }
    Plsel_MoveGridCursor(p + 6, pad);
    return pad;
}

extern char*gPlselTexts[];
void Plsel_DrawTimerWindow(void) {
    PlselTimer *t;
    Player *pl;
    s32 id;
    char buf[0x40];
    ListPanel win;

    t = &gPlselTimer;
    pl = &((Player *)gPlayers)[gScene.localPlayer];
    id = (s32) Shake_GetOffset(6);
    win = gPlselTimerWin;
    if (Quest_IsDrill() == 1) {
        win.items = D_46A0C8;
    }
    switch (t->unk3) {
    case 0:
        if (gHelpShow != 0) {
            HelpWindow_DrawCurrent();
            if ((t->unkB & 0x1F) < 0x14) {
                Draw_Icons((Icon *)gPlselTimerIconB, 1);
            }
        }
        break;
    case 1:
        HelpWindow_Draw(1, gScene.supply[t->unk6].item + 0x18, gHelpColumn);
        break;
    case 2:
        HelpWindow_Draw(1, pl->pouch[t->unk7].item + 0x18, gHelpColumn);
        break;
    }
    switch (t->unk0) {
    case 0:
        win.cursorColor = 0xA9182;
        break;
    case 1:
        win.cursorColor = 0x808080;
        switch (t->unk1) {
        case 0:
            switch (t->unk2) {
            case 0:
                Plsel_DrawItemBox(t->unk6, 0, t->unkA);
                if (t->unk3 == 0) {
                    Draw_Icons((Icon *)gPlselTimerIconA, 1);
                }
                break;
            case 1:
                Plsel_DrawItemBox(t->unk6, 1, t->unkA);
                ItemList_DrawPouch(t->unk7, 0xA9182, 8);
                if (t->unk3 == 0) {
                    Draw_Icons((Icon *)gPlselTimerIconA, 1);
                }
                break;
            case 2:
                Plsel_DrawItemBox(t->unk6, 1, t->unkA);
                ItemList_DrawPouch(t->unk7, 0x808080, 0);
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
    ListPanel_Draw(&win, (s16 *)id, (char *)0, t->unk1);
    sprintf(buf, gPlselTexts[31], t->unk8 / 30);
    Window_DrawText(&gPlselTimerText, (s16 *)id, (u8 *)buf);
    if (Quest_IsDrill() == 0) {
        Plsel_DrawMoney();
    }
}

void Plsel_ComputeItemBoxValue(void) {
    s32 i;
    u16 *p;

    gPlselShopMoney[0] = 0;
    p = (u16 *)gScene.supply;
    for (i = 0; i < 0x30; i++, p += 2) {
        if (p[0] != 0) {
            if (p[0] != 0) {
                gPlselShopMoney[0] += gItemData[p[0]].value * (s16)p[1];
            }
        }
    }
}

extern char *D_0046A0EC;
void Plsel_DrawMoney(void) {
    char num[0x20];
    char txt[0x20];
    Sprite s;
    s32 i;
    s16 x;

    sprintf(num, D_0046A0EC, gSave.money);
    Sjis_Widen(num, txt);
    Font_SetSpacing(0x12, 0x12);
    Text_SetColor(1);
    Font_SetPos(0x1BA, 0x12);
    Text_Draw(txt);
    Text_SetColor(0);
    Font_SetPos(0x1B8, 0x10);
    Text_Draw(txt);
    Gfx_SetState63(1);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s.u0 = 0x9B;
    s.v0 = 0x84;
    s.u1 = 0xB2;
    s.v1 = 0x8B;
    s.w = 0x18;
    s.h = 8;
    for (x = 400, i = 0; i < 8; i++, x += 0x18) {
        s.x = x + 3;
        s.y = 0x20;
        s.color = 0xFF000000;
        Sprite_Draw(&s);
        s.x = x;
        s.y = 0x1E;
        s.color = 0xFF806000;
        Sprite_Draw(&s);
    }
    s.x = 0x19A;
    s.y = 10;
    s.w = 0x18;
    s.h = 0x18;
    s.color = 0xFFE0E000;
    s.u0 = 0x67;
    s.v0 = 0xAB;
    s.u1 = 0x7E;
    s.v1 = 0xC2;
    Sprite_Draw(&s);
}

s32 Plsel_ItemStackValue(u16 *p) {
    if (p[0] == 0) {
        return -1;
    }
    if (p[0] == 0) {
        return -1;
    }
    return gItemData[p[0]].value * (s16)p[1];
}

/* Rect for Prim_FillRect. */
typedef struct PlselRect {
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    u32 color;
} PlselRect;

struct Frame;
void WindowFrame_Draw(struct Frame *f, s16 *pos);
struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);
extern u32 gItemColors[];
extern char *gItemNames[];
extern char *D_46A0B0[];
s32 Plsel_ItemStackValue(u16 *p);

extern char *D_0046A0E0;
extern char *D_0046A0E4;
extern char *D_0046A0E8;
extern char*gPlselTexts[];
/* Draws the item box grid (48 slots), the item under the cursor with its
 * name and count, and the selling window. */
void Plsel_DrawItemBox(s16 sel, u8 dim, s8 flag) {
    PlselShop *t;
    PlselRect rc;
    Sprite s;
    char buf[0x30];
    char num[0x20];
    ItemStack *st;
    s16 i;
    u16 *p;
    s32 x;
    u8 alpha;
    ItemData *d;
    s32 k;
    s32 v;

    t = (PlselShop *)&gPlselTimer;
    x = dim == 0 ? 0xFF : 0x60;
    alpha = x;
    WindowFrame_Draw(&gPlselItemBoxFrame, NULL);
    rc.color = 0xFF200000;
    for (i = 0; i < 0x30; i++) {
        rc.x0 = (s16)(313.0f + 36.0f * (i & 7)) + 4;
        rc.x1 = rc.x0 + 0x20;
        rc.y0 = (i >> 3) * 32 + 0x42;
        rc.y1 = rc.y0 + 0x1C;
        Prim_FillRect((struct PrimLine *)&rc);
    }
    Gfx_SetState63(0);
    Tex_Prepare(1, 0x144);
    Tex_Bind(0x144);
    s.w = 0x28;
    p = (u16 *)gScene.supply;
    s.h = 0x20;
    for (i = 0; i < 0x30; i++, p += 2) {
        s.x = 313.0f + 36.0f * (i & 7);
        s.y = (i >> 3) * 32 + 0x40;
        if (p[0] != 0) {
            d = &gItemData[p[0]];
            if (d->icon != 0xFF) {
                k = d->icon + 1;
                s.u0 = (k & 7) * 32 + 1;
                s.v0 = (k >> 3) * 32 + 1;
                s.u1 = s.u0 + 0x1E;
                s.v1 = s.v0 + 0x1E;
                s.color = gItemColors[d->color];
                s.color = ((u32)s.color & 0xFFFFFF) | (alpha << 24);
                Sprite_Draw(&s);
            }
        }
    }
    if (sel >= 0) {
        s.x = 313.0f + 36.0f * (sel & 7);
        s.y = (sel >> 3) * 32 + 0x40;
        *(s32 *)&s.u0 = 0;
        *(s32 *)&s.u1 = 0x200020;
        s.color = (alpha << 24) | 0xFFFFFF;
        Sprite_Draw(&s);
        Font_SetSpacing(0x12, 0x12);
        if (dim == 0) {
            Text_SetColor(0);
        } else {
            Text_SetColor(10);
        }
        st = &gScene.supply[sel];
        if (st->item == 0) {
            sprintf(buf, gItemNames[0]);
        } else {
            switch (gItemData[st->item].max) {
            case 1:
                sprintf(buf, gItemNames[st->item]);
                break;
            case 0xFF:
                sprintf(buf, gPlselTexts[32], gItemNames[st->item]);
                break;
            default:
                sprintf(buf, gPlselTexts[33], gItemNames[st->item], st->count);
                break;
            }
        }
        Font_SetPos(0x1CB - Str_Length(buf) * 9 / 2, 0x120);
        Text_Draw(buf);
        if (Quest_IsDrill() == 0 && t->t.unk0 == 1 && t->t.unk1 == 0) {
            v = Plsel_ItemStackValue((u16 *)st);
            if (v >= 0) {
                Text_SetColor(0);
                sprintf(buf, D_0046A0E8, v);
                Sjis_Widen(buf, num);
                strcpy(buf, D_0046A0E0);
                strcat(buf, num);
                Font_SetPos((s32)((306.0f - 9.0f * (Str_Length(buf) - 8)) / 2.0f) + 0x132, 0x138);
                Text_Printf((s8 *) buf);
            }
        }
        if (flag >= 0) {
            Text_SetColor(2);
            Font_SetPos(0x13B, 0x108);
            Text_Draw(gPlselTexts[34]);
        }
    }
    if (Quest_IsDrill() != 1 && t->t.unk0 == 1 && t->t.unk1 == 1) {
        Text_SetColor(0);
        Plsel_ComputeItemBoxValue();
        sprintf(buf, D_0046A0E8, t->money);
        Sjis_Widen(buf, num);
        strcpy(buf, D_0046A0E4);
        strcat(buf, num);
        Font_SetPos((s32)((306.0f - 9.0f * Str_Length(buf)) / 2.0f) + 0x132, 0x108);
        Text_Draw(buf);
        strcpy(buf, D_46A0B0[t->t.unkC[0]]);
        Font_SetPos(0x13C, 0x18C);
        Text_Printf((s8 *) buf);
    }
}

/* "%s" comes here in the game's .rodata, before Plsel_CategoryIsShared's jump table,
 * although its first user is Plsel_BuildRewardLines: probably first used by a function
 * the linker dead-stripped. */
INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BB38);

/* Whether hunting category arg0's reward is shared among the players (divided by
 * their count, 0x6ADCEE). */
static s32 Plsel_CategoryIsShared(u32 i) {
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

s32 Plsel_DrillPayout(void) {
    PlselWork *w;

    w = gPlselWork;
    if (Quest_IsDrill() == 0) {
        return -1;
    }
    return w->rewList.payout;
}

/* The task running the rewards screen. */
typedef struct PlselRewTask {
    u8 unk00[2];
    u8 state;                   /* 0x02 */
    u8 unk03[3];
    s16 wait;                   /* 0x06 */
    s16 timeout;                /* 0x08 */
} PlselRewTask;

extern u8 gQuestStats[];
extern s16 D_44B2F0[];
u16 Dojo_GetParMinutes(u16);
s32 Dojo_GetBestTime(u16 id);
s32 Dojo_GetBestScore(u16);
/* unit's own declaration: with the definition's void (s32 a, s32 value) (sub_main/unk_3B4C80.c), Plsel_ComputeRewards: 2 of 590 words differ */
void Dojo_SetBestTime(u16, u32);
void Dojo_SetBestScore(s32 a, s32 value);
s32 Save_GetZenny(void);

/* Works out the quest rewards into the rewards list: the pouch sold, the
 * goals, the hunting categories and monsters, the time bonus and the pages
 * the list will show. The first 17-loop walks gPlselRewardSteps and tb by explicit
 * pointers (st, tb) and v by index: MWCC bumps its own walker (v) first. */
void Plsel_ComputeRewards(Player *pl) {
    s32 *c3;
    PlselList *l;
    u8 *q;
    s32 i;
    ItemStack *e;
    s32 *v;
    u32 t;
    s32 k;
    u8 *qs;
    PlselWork *w;
    s32 *g;
    s16 *tb;
    s16 *tb2;
    u16 *kc;
    s8 *st;

    w = gPlselWork;
    q = (u8 *)gQuest.info;
    if (Quest_IsRunning(1) != 1) {
        return;
    }
    w->rewList.pageCount[1] = 1;
    l = &w->rewList;
    w->rewList.pageCount[2] = 1;
    memcpy(l->pouch, pl->pouch, 0x50);
    v = l->val;
    for (i = 0; i < 20; i++) {
        if (l->pouch[i].item != 0 && l->pouch[i].count != 0 && !(gItemData[l->pouch[i].item].flags2 & 4)) {
            v[i] = l->pouch[i].count * gItemData[l->pouch[i].item].value;
            v[i] = (f32)v[i] / 4.0f;
            l->itemTotal += v[i];
        }
    }
    qs = gQuestStats;
    g = l->goal;
    if (QuestGoal_IsDone(0) == 1) {
        *g = gQuest.reward;
    }
    g++;
    if (QuestGoal_IsDone(1) == 1) {
        *g = *(s32 *)(q + 0x14) / gScene.playerCount;
    }
    g++;
    if (QuestGoal_IsDone(2) == 1) {
        *g = *(s32 *)(q + 0x18) / gScene.playerCount;
    }
    l->clearTime = *(s32 *)(qs + 0x14);
    l->parTime = Dojo_GetParMinutes(gQuest.stage) * 1800;
    if (l->clearTime <= l->parTime) {
        l->fastClear = 1;
    }
    t = Dojo_GetBestTime(gQuest.stage);
    if (t > (u32)l->clearTime || t == 0) {
        l->newBestTime = 1;
        l->newBestTime2 = 1;
        Dojo_SetBestTime(gQuest.stage, l->clearTime);
    }
    v = l->catReward;
    tb = gPlselRewardTbl;
    for (i = 0, st = gPlselRewardSteps; i < 17; i++, tb++, st++) {
        if (*st != -1) {
            k = qs[*st];
            v[i] = k * *tb;
            if (Plsel_CategoryIsShared(i) != 0) {
                v[i] /= gScene.playerCount;
            }
        }
    }
    kc = w->huntedByGroup;
    v = l->catReward;
    tb2 = D_44B2F0;
    for (i = 0; i < 0x4A; i++) {
        l->kill[i] = *kc++ * *tb2++;
        l->kill[i] /= gScene.playerCount;
        *v += l->kill[i];
        l->killTotal += l->kill[i];
    }
    if (*(u16 *)((u8 *)w + MonsterGroup_First(qs[0x10]) * 2 + 0x464) != 0) {
        l->catReward[12] = 1;
    }
    k = *(u16 *)(qs + 0x12);
    if (k >= 13) {
        k++;
    }
    if (l->catReward[k] != 0) {
        l->catReward[13] = 1;
    }
    c3 = l->catReward;
    for (i = 0; i < 17; i++) {
        if (i == 12 || i == 13) {
            c3++;
        } else {
            l->huntReward += *c3++;
        }
    }
    if (l->huntReward > 0) {
        if (l->catReward[12] != 0) {
            l->huntReward = l->huntReward * 200 / 100;
        }
        if (l->catReward[13] != 0) {
            l->huntReward = l->huntReward * 150 / 100;
        }
    }
    l->rewardTotal += l->goal[0];
    l->rewardTotal += l->goal[1];
    l->rewardTotal += l->goal[2];
    l->rewardTotal += l->huntReward;
    g = &l->rewardTotal;
    if (l->fastClear != 0 && *g > 0) {
        *g *= 2;
    }
    l->payout = l->itemTotal + l->rewardTotal;
    l->payout = l->payout * gQuestSync.pointRatePct / 100;
    if (l->payout < 0) {
        l->payout = 0;
    }
    l->payoutCopy = l->payout;
    if (Dojo_GetBestScore(gQuest.stage) < l->payout) {
        l->newBestScore = 1;
        Dojo_SetBestScore(gQuest.stage, l->payout);
    }
    l->linesShown = 12;
    l->screen = 0;
    l->zenny = Save_GetZenny();
    l->pageCount[0] = 1;
    l->pages[l->pageCount[0] - 1] = 0;
    if (l->huntReward != 0) {
        l->pageCount[0]++;
        l->pages[l->pageCount[0] - 1] = 1;
    }
    if (l->catReward[0] != 0) {
        l->pageCount[0]++;
        l->pages[l->pageCount[0] - 1] = 2;
    }
    if (l->itemTotal != 0) {
        l->pageCount[0]++;
        l->pages[l->pageCount[0] - 1] = 3;
    }
}

void PlselRes_BeginClose(void) {
    PlselWork *w;

    w = gPlselWork;
    func_00D72F10();
    w->closeState = 1;
    w->unk06C2 = 0;
    w->closeTimer = 0x1E;
}

extern char *D_469FE0[];
extern char *D_46A040[];
extern char D_00D7BB88[];
extern char D_00D7BB90[];
extern char D_00D7BB98[];
extern char D_00D7BBA0[];
extern char D_00D7BBA8[];

#define LINE(l) ((char *)&(l)->ent[(l)->lineCount])

/* Builds the text lines of the rewards list's current page. Case 2 reuses
 * case 1's table pointer and padding counter (tbl, n1): with locals of its
 * own, its kill walker and counter took each other's saved registers. */
void Plsel_BuildRewardLines(PlselList *l) {
    char buf[0x80];
    char buf2[0x80];
    char **tbl;
    s32 k;
    char **str;
    s32 n1;
    s32 *c;
    s32 i;
    s32 n;
    s8 *st;
    s32 i2;
    PlselWork *w;
    char **t4;
    s32 i3;
    s32 n3;
    s32 n4;
    s32 i1;
    s32 m;
    s32 s;

    w = gPlselWork;
    for (i = 0; i < 80; i++) {
        l->ent[i].flag = 0;
    }
    l->unk1C4 = 0;
    l->lineCount = 0;
    l->scrollable = 0;
    switch (l->screen) {
    case 0:
        l->linesShown = 8;
        switch (l->pages[l->pageCur]) {
        case 0:
            tbl = D_469FE0;
            str = &gResultRuleText;
            for (i = 0; i < 4; i++) {
                switch (i) {
                case 0:
                    if (Quest_IsRunning(0) != 0) {
                        sprintf(buf, tbl[2], l->goal[i]);
                        goto widen;
                    }
                    break;
                case 1:
                    if (gQuest.goals[1].flags & 0x20) {
                        sprintf(buf, tbl[3], l->goal[i]);
                        goto widen;
                    }
                    break;
                case 2:
                    if (gQuest.goals[2].flags & 0x20) {
                        sprintf(buf, tbl[4], l->goal[i]);
                    widen:
                        Sjis_Widen(buf, LINE(l));
                        l->lineCount++;
                    }
                    break;
                case 3:
                    if (l->goal[i] != 0) {
                        sprintf(LINE(l), tbl[5]);
                        sprintf(buf, TXT(0x00D7BB70) /* the goal, then a full-width P (points) */, l->goal[i]);
                        Sjis_Widen(buf, buf2);
                        strcat(LINE(l), buf2);
                        l->lineCount++;
                    }
                    break;
                }
            }
            if (l->goal[i] != 0) {
                sprintf(LINE(l), tbl[6]);
                l->lineCount++;
            }
            l->lineCount++;
            sprintf(buf, tbl[7], l->itemTotal);
            Sjis_Widen(buf, LINE(l));
            l->lineCount++;
            for (n = l->lineCount; n < 10; n++) {
                l->lineCount++;
            }
            sprintf((char *)&l->ent[l->lineCount++], D_00D7BB38, *str);
            sprintf(buf, tbl[8], l->itemTotal + l->rewardTotal);
            Sjis_Widen(buf, LINE(l));
            break;
        case 1:
            l->linesShown = 8;
            l->scrollable = 1;
            tbl = D_46A040;
            l->markMonsterLine = -1;
            c = l->catReward;
            k = 0;
            l->markHuntLine = -1;
            for (n = 0, str = tbl, st = gPlselRewardSteps; n < 17; n++) {
                if (n != 13) {
                    if (*(u16 *)&gQuestStats[0x12] == k && *c != 0) {
                        l->markHuntLine = l->lineCount;
                    }
                    k++;
                }
                switch (n) {
                case 0:
                    if (l->killTotal != 0) {
                        strcpy(LINE(l), *str);
                        sprintf(buf, "%d", l->killTotal);
                        Sjis_Widen(buf, buf2);
                        if (l->killTotal > 0) {
                            strcpy(buf, tbl[20]);
                        } else {
                            strcpy(buf, tbl[19]);
                        }
                        strcat(buf, buf2);
                        strcat(buf, tbl[21]);
                        strcat(buf, TXT(0x00D7BB80) /* a full-width P (points) */);
                        strcat(LINE(l), buf);
                        l->lineCount++;
                    }
                    break;
                case 1:
                case 10:
                    if (*c != 0) {
                        strcpy(LINE(l), *str);
                        sprintf(buf, "%d", *c);
                        Sjis_Widen(buf, buf2);
                        strcpy(buf, tbl[20]);
                        strcat(LINE(l), buf);
                        sprintf(buf2, "%d", *c);
                        Sjis_Widen(buf2, buf);
                        strcat(LINE(l), buf);
                        strcat(LINE(l), tbl[21]);
                        strcat(LINE(l), TXT(0x00D7BB80) /* a full-width P (points) */);
                        l->lineCount++;
                    }
                    break;
                case 2:
                case 14:
                case 15:
                case 16:
                case 3:
                    if (*c != 0) {
                        sprintf(buf, *str, gQuest.stats[*st]);
                        Sjis_Widen(buf, buf2);
                        if ((n1 = Str_Length(buf2)) < 0x1C) {
                            do {
                                strcat(buf2, D_00D7BB88);
                            } while (++n1 < 0x1C);
                        }
                        strcpy(LINE(l), buf2);
                        strcat(LINE(l), D_00D7BB90);
                        if (n == 2 || n == 3) {
                            strcpy(buf, tbl[19]);
                        } else {
                            strcpy(buf, tbl[20]);
                        }
                        strcat(LINE(l), buf);
                        sprintf(buf2, "%d", *c);
                        Sjis_Widen(buf2, buf);
                        strcat(LINE(l), buf);
                        strcat(LINE(l), tbl[21]);
                        strcat(LINE(l), TXT(0x00D7BB80) /* a full-width P (points) */);
                        l->lineCount++;
                    }
                    break;
                case 12:
                case 13:
                    break;
                default:
                    if (*c != 0) {
                        strcpy(LINE(l), *str);
                        sprintf(buf, "%d", *c);
                        Sjis_Widen(buf, buf2);
                        if (n == 11) {
                            strcpy(buf, tbl[19]);
                        } else if (n >= 4 && n < 10) {
                            strcpy(buf, tbl[20]);
                        } else {
                            buf[0] = 0;
                        }
                        strcat(buf, buf2);
                        if (n == 11 || (n >= 4 && n < 10)) {
                            strcat(buf, tbl[21]);
                        }
                        strcat(LINE(l), buf);
                        strcat(LINE(l), TXT(0x00D7BB80) /* a full-width P (points) */);
                        l->lineCount++;
                    }
                    break;
                }
                c++;
                str++;
                st++;
            }
            if (l->catReward[12] != 0 && l->huntReward > 0) {
                l->lineCount++;
                sprintf(LINE(l), tbl[12]);
                l->lineCount++;
            }
            if (l->catReward[13] != 0 && l->huntReward > 0) {
                if (l->catReward[12] == 0) {
                    l->lineCount++;
                }
                sprintf(LINE(l), tbl[13]);
                l->lineCount++;
            }
            break;
        case 2:
            l->scrollable = 1;
            l->linesShown = 8;
            tbl = D_46A040;
            l->markMonsterLine = -1;
            for (i2 = 0; i2 < 0x4A; i2++) {
                if (l->kill[i2] != 0) {
                    if (*(u16 *)&gQuestStats[0x10] == (u16)i2) {
                        l->markMonsterLine = l->lineCount;
                    }
                    sprintf(buf, D_00D7BB98, gMonsterNames[i2], w->huntedByGroup[i2]);
                    Sjis_Widen(buf, buf2);
                    if ((n1 = Str_Length(buf2)) < 0x1E) {
                        do {
                            strcat(buf2, D_00D7BB88);
                        } while (++n1 < 0x1E);
                    }
                    strcpy(buf, buf2);
                    strcpy(LINE(l), buf);
                    strcat(LINE(l), D_00D7BB90);
                    if (l->kill[i2] < 0) {
                        strcat(LINE(l), tbl[19]);
                    } else {
                        strcat(LINE(l), tbl[20]);
                    }
                    sprintf(buf2, "%d", l->kill[i2]);
                    Sjis_Widen(buf2, buf);
                    strcat(LINE(l), buf);
                    strcat(LINE(l), tbl[21]);
                    strcat(LINE(l), TXT(0x00D7BB80) /* a full-width P (points) */);
                    l->lineCount++;
                }
            }
            break;
        case 3:
            l->scrollable = 1;
            l->linesShown = 8;
            for (i = 0; i < 20; i++) {
                if (l->val[i] != 0 && l->pouch[i].item != 0 && l->pouch[i].count != 0) {
                    sprintf(buf, D_00D7BB98, gItemNames[l->pouch[i].item], l->pouch[i].count);
                    Sjis_Widen(buf, buf2);
                    if ((n3 = Str_Length(buf2)) < 0x1E) {
                        do {
                            strcat(buf2, D_00D7BB88);
                        } while (++n3 < 0x1E);
                    }
                    strcpy(LINE(l), buf2);
                    sprintf(buf2, D_00D7BBA0, l->val[i]);
                    Sjis_Widen(buf2, buf);
                    strcat(LINE(l), buf);
                    l->lineCount++;
                }
            }
            break;
        }
        break;
    case 2:
        l->linesShown = 8;
        l->scrollable = 0;
        t4 = D_469FE0;
        if (l->clearTime == 0) {
            m = 0;
            s = 0;
        } else {
            n4 = l->clearTime / 30;
            m = n4 / 60;
            s = n4;
            s -= m * 60;
        }
        sprintf(buf, t4[12], m, s);
        Sjis_Widen(buf, LINE(l));
        if (l->newBestTime != 0) {
            strcat(LINE(l), t4[0]);
        }
        l->lineCount++;
        strcpy((char *)&l->ent[l->lineCount++], gResultRuleText);
        sprintf(buf, t4[9], l->itemTotal + l->rewardTotal);
        Sjis_Widen(buf, LINE(l));
        l->lineCount++;
        sprintf(buf, t4[13], gQuestSync.pointRatePct);
        Sjis_Widen(buf, LINE(l));
        strcat(LINE(l), D_00D7BBA8);
        l->lineCount += 4;
        sprintf(buf, t4[14], l->payout);
        Sjis_Widen(buf, LINE(l));
        if (l->newBestScore != 0) {
            strcat(LINE(l), t4[0]);
        }
        break;
    }
}

INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BB88);

INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BB90);

INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BB98);

INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BBA0);

INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BBA8);


void Plsel_RewardListDraw(void);
void Plsel_BuildRewardLines(PlselList *l);
void PlselRes_BeginClose(void);
void Save_AddZenny(s32);
void FadeA_Start(s32);

s32 Plsel_RewardListTask(PlselRewTask *t) {
    PlselList *l;
    u16 pad2;
    u16 trg;
    PlselWork *w;
    u16 pad;

    w = gPlselWork;
    trg = gPadState[0].held;
    l = &w->rewList;
    pad = gPadState[0].pressed;
    pad2 = gPadState[0].repeat | pad;
    if (w->closeState == 1) {
        PlselRes_Close();
    }
    switch (t->state) {
    case 0:
        t->timeout = 3600;
        *(void (**)(void))0x6A3A24 = Plsel_RewardListDraw;
        l->pageCur = 0;
        l->topLine = 0;
        Plsel_BuildRewardLines(l);
        l->unk1A44 = 10;
        l->revealLine = 12;
        t->wait = 20;
        t->state++;
    case 1:
        if (t->wait != 0) {
            t->wait--;
            break;
        }
    common:
        l->frame++;
        if (!(l->frame & 3)) {
            l->arrowBob ^= 1;
        }
        if (t->timeout != 0) {
            t->timeout--;
        }
        if (t->state < 4 && ((pad & 0x20) || t->timeout == 0)) {
            PlselRes_PlayDecideSe();
            PlselRes_BeginClose();
            t->state = 10;
            break;
        }
        if (l->pageCount[0] >= 2) {
            if (l->pageCount[0] > 1) {
                if (pad & 0x400) {
                    PlselRes_PlayPageSe();
                    if (l->pageCur >= l->pageCount[0] - 1) {
                        l->pageCur = 0;
                    } else {
                        l->pageCur++;
                    }
                    l->topLine = 0;
                    Plsel_BuildRewardLines(l);
                    break;
                }
                if (pad & 0x800) {
                    PlselRes_PlayPageSe();
                    if (l->pageCur > 0) {
                        l->pageCur--;
                    } else {
                        l->pageCur = l->pageCount[0] - 1;
                    }
                    l->topLine = 0;
                    Plsel_BuildRewardLines(l);
                    break;
                }
            }
            if (l->pageCur == 0) {
                break;
            }
        }
        if ((pad2 & 0x1000) && l->lineCount > l->topLine + l->linesShown) {
            PlselRes_PlayPageSe();
            l->topLine++;
        } else if ((pad2 & 0x2000) && l->topLine > 0) {
            PlselRes_PlayPageSe();
            l->topLine--;
        }
        break;
    case 2:
        if (t->wait == 0) {
            if (l->pageCur != 0 || l->revealLine == l->lineCount) {
                goto common;
            }
            l->revealLine++;
            while (l->ent[l->revealLine].flag == 0) {
                if (l->revealLine == l->lineCount) {
                    break;
                }
                l->revealLine++;
            }
            t->wait = 20;
        } else {
            t->wait--;
        }
        break;
    case 3:
        l->doneShown = 1;
        if (t->wait > 0) {
            t->wait--;
            break;
        }
        if (t->timeout != 0) {
            t->timeout--;
        }
        if ((trg & 0x20) || t->timeout == 0) {
            PlselRes_PlayDecideSe();
            PlselRes_BeginClose();
            t->state++;
        }
        break;
    case 4:
        if (--w->closeTimer <= 0) {
            FadeA_Start(1);
            t->wait = 30;
            t->state++;
        }
        break;
    case 5:
        if (--t->wait <= 0) {
            return 1;
        }
        break;
    case 10:
        if (--w->closeTimer <= 0) {
            t->state = 3;
            l->screen = 2;
            l->pageCur = 0;
            l->revealLine = 0;
            l->zenny += l->payout;
            Save_AddZenny(l->payout);
            Plsel_BuildRewardLines(l);
            t->wait = 20;
            t->timeout = 3600;
            w->closeState = 0;
        }
        break;
    }
    return 0;
}

INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BBB0);

INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BBB8);

void Plsel_RewardListDraw(void) {
    PlselList *l;
    s32 y;

    l = &gPlselWork->rewList;
    Plsel_DrawResultsBg();
    switch (l->screen) {
    case 0:
        Text_SetSize(4);
        if (l->scrollable == 1) {
            if (l->topLine > 0) {
                y = 0x80 - l->arrowBob;
                Font_SetSpacing(0x1C, 0x1C);
                Font_SetPos(0x1F4, y);
                Text_PrintfColored(D_00D7BBB0);
            }
            if (l->lineCount > l->topLine + l->linesShown) {
                y = l->arrowBob + 0x10E;
                Font_SetSpacing(0x1C, 0x1C);
                Font_SetPos(0x1F4, y);
                Text_PrintfColored(D_00D7BBB8);
            }
        }
        if (l->pageCount[l->screen] > 1) {
            PlselRes_DrawArrow(1, 0x228, 0x58, 1);
            PlselRes_DrawArrow(0, 0x1E6, 0x58, 1);
        }
        break;
    }
}

INCLUDE_RODATA("asm/plsel/nonmatchings/unk_D67530", D_00D7BBC0);

extern char *D_469FE0[];

extern char*gPlselTexts[];
/* The rewards screen text: the page title, the lines shown, the page's
 * total, the page number and the countdown. */
void Plsel_RewardListDrawText(PlselRewTask *t) {
    s32 w;
    s32 x;
    s32 y;
    s32 i;
    char **tbl;
    PlselList *l;
    char buf[0x80];
    char num[0x80];

    l = &gPlselWork->rewList;
    Text_SetColor(0);
    Font_SetSpacing(0x12, 0x12);
    w = Str_Length(gResultRuleText);
    w *= 9.0f; /* `w = len * 9.0f` puts the literal first */
    x = (s32)(640.0f - w) / 2.0f;
    y = 0x58;
    Text_SetColor(0x31);
    Font_SetSpacing(0x12, 0x12);
    tbl = D_469FE0;
    switch (l->screen) {
    case 0:
        switch (l->pages[l->pageCur]) {
        case 0:
            Font_SetSpacing(0x18, 0x18);
            Font_SetPos(x, 0x52);
            Text_Printf((s8 *) D_00D7BB38, tbl[1]);
            Font_SetSpacing(0x12, 0x12);
            Font_SetPos(x, 0x6E);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            y += 0x2C;
            for (i = 0; i < l->linesShown; y += 0x16, i++) {
                Font_SetPos(x, y);
                if (l->ent[i].flag) {
                    Text_Printf((s8 *) &l->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 0x16;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            Font_SetPos(x, y + 0x16);
            sprintf(buf, tbl[8], l->itemTotal + l->rewardTotal);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) D_00D7BB38, num);
            break;
        case 1:
            Font_SetPos(x, y);
            Text_Printf((s8 *) D_00D7BB38, tbl[16]);
            Font_SetPos(x, 0x6E);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            y += 0x2C;
            for (i = l->topLine; i < l->topLine + l->linesShown; y += 0x16, i++) {
                if (i == l->markHuntLine && l->markHuntLine >= 0) {
                    Font_SetPos(x - 0x1E, y);
                    Text_Printf((s8 *) gPlselTexts[162]);
                }
                Font_SetPos(x, y);
                if (l->ent[i].flag) {
                    Text_Printf((s8 *) &l->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 0x16;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            Font_SetPos(x, y + 0x16);
            sprintf(buf, tbl[21], l->huntReward);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) D_00D7BB38, num);
            break;
        case 2:
            Font_SetPos(x, y);
            Text_Printf((s8 *) D_00D7BB38, tbl[19]);
            Font_SetPos(x, 0x6E);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            y += 0x2C;
            for (i = l->topLine; i < l->topLine + l->linesShown; y += 0x16, i++) {
                if (i == l->markMonsterLine && l->markMonsterLine >= 0) {
                    Font_SetPos(x - 0x1E, y);
                    Text_Printf((s8 *) gPlselTexts[161]);
                }
                Font_SetPos(x, y);
                if (l->ent[i].flag) {
                    Text_Printf((s8 *) &l->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 0x16;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            Font_SetPos(x, y + 0x16);
            sprintf(buf, tbl[22], l->killTotal);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) D_00D7BB38, num);
            break;
        case 3:
            Font_SetPos(x, y);
            Text_Printf((s8 *) D_00D7BB38, tbl[20]);
            Font_SetPos(x, 0x6E);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            y += 0x2C;
            for (i = l->topLine; i < l->topLine + l->linesShown; y += 0x16, i++) {
                Font_SetPos(x, y);
                if (l->ent[i].flag) {
                    Text_Printf((s8 *) &l->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 0x16;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            Font_SetPos(x, y + 0x16);
            sprintf(buf, tbl[23], l->itemTotal);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) D_00D7BB38, num);
            break;
        }
        Font_SetPos(0x202, 0x58);
        sprintf(buf, D_00D7BBC0, l->pageCur + 1, l->pageCount[0]);
        Text_PrintfColored(buf);
        break;
    case 2:
            Font_SetSpacing(0x18, 0x18);
            Font_SetPos(x, 0x52);
            Text_Printf((s8 *) D_00D7BB38, tbl[11]);
            Font_SetSpacing(0x12, 0x12);
            Font_SetPos(x, 0x6E);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            y += 0x2C;
            for (i = 0; i < l->linesShown; y += 0x16, i++) {
                Font_SetPos(x, y);
                if (l->ent[i].flag) {
                    Text_Printf((s8 *) &l->ent[i]);
                }
            }
            while (i < 8) {
                i++;
                y += 0x16;
            }
            Font_SetPos(x, y);
            Text_Printf((s8 *) D_00D7BB38, gResultRuleText);
            Font_SetPos(x, y + 0x16);
            sprintf(buf, tbl[15], l->zenny);
            Sjis_Widen(buf, num);
            Text_Printf((s8 *) D_00D7BB38, num);
            break;
    }
    Font_SetPos(0x1EA, 0x164);
    Text_Printf((s8 *) gPlselTexts[111]);
    Font_SetPos(0x1B8, 0x17C);
    sprintf(num, gPlselTexts[112], t->timeout / 30);
    Text_Printf((s8 *) num);
}

extern char *D_469EB0[];
extern char *gArenaEmTameNames[];

/* The line being written of the results screen's text. */
#define WLINE(w) ((w)->lines[(w)->lineCount])

/* Stripped_Sub: a stand-in for a function of this file that the game's
 * linker dead-stripped (tools/mwcc_cc.py compiles it and drops its code). Its
 * body is unknown: what shows is that the file compiled a two-argument runtime
 * call before Plsel_BuildResultText, which changes that function's code
 * (docs/notes/nm/Plsel_BuildResultText.md). The "%s" before 0xD6ABB0's jump
 * table, first used only later, is another sign of a stripped function here. */
static f64 Stripped_Sub(f64 a, f64 b)
{
    return a - b;
}

/* Builds the text lines (w->lines, count w->lineCount) of the results
 * screen's page `kind` (w->page): 0 the quest's summary, 1 and 2 the
 * rewards (the goals, then per monster), 3 the items sold (seven from line
 * w->firstLine), 4 the hunter rank, 5 a monster group's size records, 7 the
 * guild card's record.
 * The game reuses the parameter as a counter and as the monster group
 * (s1), and one counter for two loops.
 * FAKEMATCH: it matches only with Stripped_Sub before it. Once a function
 * earlier in the file has compiled a two-argument runtime call (dpsub,
 * dpadd, __divdi3...), MWCC models its own fptodp calls (the floats promoted
 * for sprintf) as reading a1 too. So in case 5, a1 stays busy from the
 * format of the sprintf before each size test to the fptodp after it: the
 * size stays in v0, is copied to a1 after the test, and the copy can't fill
 * the test's delay slot, as in the game. Without it MWCC loads the size into
 * a1 and fills the slot with kind's andi (2 words short; the old block
 * stayed at 24 words). Nothing left in plsel calls such a helper. */
void Plsel_BuildResultText(s32 kind) {
    char **t;
    char buf[0x80];
    char buf2[0x80];
    char **str;
    s32 i1;
    s32 j1;
    PlselWork *w;
    s32 need1;
    s32 n2;
    s32 need2;
    s32 val2;
    s32 j2;
    s32 v;
    f32 f;
    u16 sz;
    ArenaEm *e;

    w = gPlselWork;
    memset(w->lines, 0, 0x17C0);
    w->unk0748 = 0;
    w->scroll = 0;
    w->lineCount = 0;
    str = &gResultRuleText;
    t = D_469EB0;
    switch (kind) {
    case 0:
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[0]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(buf, t[1], w->questMoney);
        Sjis_Widen(buf, buf2);
        sprintf(w->lines[w->lineCount++], buf2);
        sprintf(buf, t[2], w->monsterMoney);
        Sjis_Widen(buf, buf2);
        sprintf(w->lines[w->lineCount++], buf2);
        sprintf(buf, t[3], w->itemMoney);
        Sjis_Widen(buf, buf2);
        sprintf(w->lines[w->lineCount++], buf2);
        w->lineCount += 4;
        sprintf(buf, t[4], w->moneyTotal);
        Sjis_Widen(buf, buf2);
        sprintf(w->lines[w->lineCount++], buf2);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(buf, t[5], gSave.money);
        Sjis_Widen(buf, buf2);
        sprintf(w->lines[w->lineCount++], buf2);
        break;
    case 2:
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[6]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[7]);
        for (i1 = 0; i1 < 8; i1++) {
            if (gPlselBonus.recs[i1].id != 0) {
                switch (gPlselBonus.recs[i1].type) {
                case 0:
                case 1:
                case 2:
                    for (j1 = 0; j1 < 3; j1++) {
                        if (gQuest.huntGoals[j1].type == gPlselBonus.recs[i1].type && gQuest.huntGoals[j1].kind == gPlselBonus.recs[i1].id) {
                            break;
                        }
                    }
                    if (j1 != 1) {
                        strcpy(buf, gMonsterNames[gPlselBonus.recs[i1].id]);
                        if ((kind = Str_Length(buf)) < 20) {
                            do {
                                strcat(buf, D_00D7BB88);
                            } while (++kind < 20);
                        }
                        sprintf(WLINE(w), D_00D7BB38, buf);
                        need1 = gQuest.huntGoals[j1].need;
                        if (need1 == 0) {
                            need1 = 1;
                        }
                        sprintf(buf, t[8], need1, gPlselBonus.recs[i1].unk6);
                        Sjis_Widen(buf, buf2);
                        strcat(WLINE(w), buf2);
                        if (j1 == 0) {
                            strcat(WLINE(w), (&gResultRuleText)[j1 + 2]);
                        }
                        w->lineCount++;
                    }
                    break;
                }
            }
        }
        w->lineCount++;
        if (gScene.playerCount == 1) {
            sprintf(w->lines[w->lineCount++], D_00D7BB38, t[9]);
        } else {
            sprintf(w->lines[w->lineCount++], D_00D7BB38, t[10]);
        }
        for (kind = 0; kind < 8; kind++) {
            if (gPlselBonus.recs[kind].id != 0) {
                switch (gPlselBonus.recs[kind].type) {
                case 0:
                case 1:
                case 2:
                    strcpy(buf, gMonsterNames[gPlselBonus.recs[kind].id]);
                    if ((n2 = Str_Length(buf)) < 20) {
                        do {
                            strcat(buf, D_00D7BB88);
                        } while (++n2 < 20);
                    }
                    sprintf(WLINE(w), D_00D7BB38, buf);
                    for (j2 = 0; j2 < 3; j2++) {
                        if (gQuest.huntGoals[j2].type == gPlselBonus.recs[kind].type && gQuest.huntGoals[j2].kind == gPlselBonus.recs[kind].id) {
                            break;
                        }
                    }
                    switch (j2) {
                    case 0:
                        need2 = gQuest.huntGoals[j2].need * gPlselBonus.recs[kind].unk4;
                        val2 = gQuest.huntGoals[j2].reward;
                        val2 *= need2;
                        val2 /= gScene.playerCount;
                        if (w->lowRankCut != 0) {
                            val2 = val2 * 60 / 100;
                        }
                        break;
                    case 1:
                        val2 = gQuest.huntGoals[j2].reward;
                        val2 /= gScene.playerCount;
                        need2 = gQuest.huntGoals[j2].need;
                        if (w->lowRankCut != 0) {
                            val2 = val2 * 60 / 100;
                        }
                        break;
                    default:
                        need2 = gPlselBonus.recs[kind].unk4;
                        val2 = gPlselBonus.recs[kind].unk6;
                        val2 *= need2;
                        val2 /= gScene.playerCount;
                        if (w->lowRankCut != 0) {
                            val2 = val2 * 60 / 100;
                        }
                        break;
                    }
                    sprintf(buf, t[8], need2, val2);
                    Sjis_Widen(buf, buf2);
                    strcat(WLINE(w), buf2);
                    if (j2 < 3) {
                        strcat(WLINE(w), (&gResultRuleText)[j2 + 2]);
                    }
                    w->lineCount++;
                    break;
                }
            }
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(buf, t[11], w->monsterMoney);
        Sjis_Widen(buf, buf2);
        sprintf(w->lines[w->lineCount++], buf2);
        break;
    case 1:
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[12]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        if (gScene.playerCount == 1) {
            sprintf(w->lines[w->lineCount++], D_00D7BB38, t[13]);
        } else {
            sprintf(w->lines[w->lineCount++], D_00D7BB38, t[14]);
        }
        sprintf(buf, t[15], w->fee);
        Sjis_Widen(buf, buf2);
        sprintf(WLINE(w), buf2);
        if (w->feeRate != 1.0f && w->fee != 0) {
            strcat(WLINE(w), str[1]);
        }
        w->lineCount++;
        if (w->wtdFee > 0) {
            sprintf(buf, t[16], w->wtdFee);
            Sjis_Widen(buf, buf2);
            sprintf(WLINE(w), buf2);
            if (w->wtdFeeRate != 1.0f && w->wtdFee != 0) {
                strcat(WLINE(w), str[1]);
            }
            w->lineCount++;
        }
        if (w->mainDone != 0) {
            if (w->mustOrder == 1) {
                sprintf(buf, t[52], w->mainMoney);
            } else {
                sprintf(buf, t[17], w->mainMoney);
            }
            Sjis_Widen(buf, buf2);
            sprintf(WLINE(w), buf2);
            w->lineCount++;
        }
        if (w->mustOrder == 0) {
            if (w->subADone != 0) {
                sprintf(buf, t[18], w->subAMoney);
                Sjis_Widen(buf, buf2);
                sprintf(WLINE(w), buf2);
                w->lineCount++;
            }
            if (w->subBDone != 0) {
                sprintf(buf, t[19], w->subBMoney);
                Sjis_Widen(buf, buf2);
                sprintf(WLINE(w), buf2);
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
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(buf, t[20], w->questMoney);
        Sjis_Widen(buf, buf2);
        sprintf(w->lines[w->lineCount++], buf2);
        break;
    case 3:
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[21]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[22]);
        for (j2 = w->firstLine; j2 < w->firstLine + 7; j2++) {
            if (gPlselBonus.items[j2].item != 0) {
                strcpy(buf, gItemNames[gPlselBonus.items[j2].item]);
                if ((kind = Str_Length(buf)) < 20) {
                    do {
                        strcat(buf, D_00D7BB88);
                    } while (++kind < 20);
                }
                sprintf(WLINE(w), D_00D7BB38, buf);
                v = gItemData[gPlselBonus.items[j2].item].value;
                sprintf(buf, t[23], gPlselBonus.items[j2].count, v * gPlselBonus.items[j2].count);
                Sjis_Widen(buf, buf2);
                strcat(w->lines[w->lineCount++], buf2);
            } else if (gPlselBonus.unk66 != 0 && gPlselBonus.unk68 != 0) {
                strcpy(buf, gItemNames[gPlselBonus.unk66]);
                if ((kind = Str_Length(buf)) < 20) {
                    do {
                        strcat(buf, D_00D7BB88);
                    } while (++kind < 20);
                }
                sprintf(WLINE(w), D_00D7BB38, buf);
                sprintf(buf, t[23], gQuest.huntGoals[2].need, gQuest.huntGoals[2].reward);
                Sjis_Widen(buf, buf2);
                strcat(WLINE(w), buf2);
                strcat(WLINE(w), gWtdClassCText);
                w->lineCount++;
                break;
            }
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(buf, t[24], w->itemMoney);
        Sjis_Widen(buf, buf2);
        sprintf(w->lines[w->lineCount++], buf2);
        break;
    case 4:
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[43]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        if (Quest_IsRunning(1) == 1) {
            if (w->mainDone != 0) {
                if (w->mustOrder == 1) {
                    sprintf(buf2, t[53], w->hrpMain);
                } else {
                    sprintf(buf2, t[44], w->hrpMain);
                }
                Sjis_Widen(buf2, buf);
                strcpy(w->lines[w->lineCount++], buf);
            }
            if (w->mustOrder == 0) {
                if (w->subADone != 0) {
                    sprintf(buf2, t[45], w->hrpSubA);
                    Sjis_Widen(buf2, buf);
                    strcpy(w->lines[w->lineCount++], buf);
                }
                if (w->subBDone != 0) {
                    sprintf(buf2, t[46], w->hrpSubB);
                    Sjis_Widen(buf2, buf);
                    strcpy(w->lines[w->lineCount++], buf);
                }
            }
        } else if (w->hrpMain != 0) {
            sprintf(buf2, t[51], w->hrpMain);
            Sjis_Widen(buf2, buf);
            strcpy(w->lines[w->lineCount++], buf);
        }
        if (w->deaths > 0) {
            sprintf(buf2, t[47], w->deaths, w->hrpDeaths);
            Sjis_Widen(buf2, buf);
            sprintf(w->lines[w->lineCount++], "~C18%s~C17", buf);
        }
        if (w->lineCount < 8) {
            w->lineCount = 8;
        }
        sprintf(buf2, t[48], w->hrpTotal);
        Sjis_Widen(buf2, buf);
        sprintf(w->lines[w->lineCount++], buf);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(buf2, t[49], w->hrPoints);
        Sjis_Widen(buf2, buf);
        sprintf(w->lines[w->lineCount++], buf);
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
        sprintf(buf2, t[50], w->rankAfter);
        Sjis_Widen(buf2, buf);
        sprintf(w->lines[w->lineCount++], buf);
        break;
    case 5:
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[26]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        kind = w->recordGroups[w->recordGroupCur];
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[27]);
        strcpy(buf, gMonsterNames[MonsterGroup_First(kind)]);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, buf);
        w->unk0212 = 0;
        w->unk0213 = 0;
        w->oldLargest = w->largestBefore[kind];
        if ((sz = w->oldLargest) > 0) {
            f = Monster_SizeFromPercent(kind, sz);
        } else {
            f = 0.0f;
        }
        w->oldLargestCm = f;
        w->shownGroup = kind;
        if (f == 0.0f) {
            strcpy(buf2, t[31]);
        } else {
            sprintf(buf2, t[54], f);
        }
        Sjis_Widen(buf2, buf);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, buf);
        w->oldSmallest = w->smallestBefore[kind];
        if ((sz = w->oldSmallest) > 0) {
            f = Monster_SizeFromPercent(kind, sz);
        } else {
            f = 0.0f;
        }
        w->oldSmallestCm = f;
        if (f == 0.0f) {
            strcpy(buf2, t[32]);
        } else {
            sprintf(buf2, t[55], f);
        }
        Sjis_Widen(buf2, buf);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, buf);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[28]);
        w->newLargest = w->questLargest[w->recordGroups[w->recordGroupCur]];
        if (w->newLargest == 0) {
            f = 0.0f;
        } else {
            f = Monster_SizeFromPercent(kind, w->questLargest[w->recordGroups[w->recordGroupCur]]);
        }
        w->newLargestCm = f;
        w->newSmallest = w->questSmallest[w->recordGroups[w->recordGroupCur]];
        if (w->newSmallest == 0) {
            f = 0.0f;
        } else {
            f = Monster_SizeFromPercent(kind, w->questSmallest[w->recordGroups[w->recordGroupCur]]);
        }
        w->newSmallestCm = f;
        strcpy(buf, gMonsterNames[MonsterGroup_First(kind)]);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, buf);
        if (w->newLargest == 0) {
            strcpy(buf2, t[56]);
            Sjis_Widen(buf2, buf);
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, buf);
        } else {
            sprintf(buf2, t[29], w->newLargestCm);
            Sjis_Widen(buf2, buf);
            if (w->largestNew[w->recordGroups[w->recordGroupCur]] != 0) {
                sprintf(w->lines[w->lineCount++], TXT(0x00D7BC10) /* colour 18, eight full-width spaces, the line, colour 17 */, buf);
            } else {
                sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, buf);
            }
        }
        if (w->newSmallest == 0) {
            strcpy(buf2, t[57]);
            Sjis_Widen(buf2, buf);
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, buf);
        } else {
            sprintf(buf2, t[30], w->newSmallestCm);
            Sjis_Widen(buf2, buf);
            if (w->smallestNew[w->recordGroups[w->recordGroupCur]] != 0) {
                sprintf(w->lines[w->lineCount++], TXT(0x00D7BC10) /* colour 18, eight full-width spaces, the line, colour 17 */, buf);
            } else {
                sprintf(w->lines[w->lineCount++], TXT(0x00D7BBF0) /* eight full-width spaces, then the line */, buf);
            }
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        break;
    case 7:
        if (w->arenaBefore.name[0] == 0) {
            sprintf(buf2, t[36], gRecordDashesText);
        } else {
            sprintf(buf2, t[36], w->arenaBefore.name);
        }
        Sjis_Widen(buf2, buf);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, buf);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        sprintf(w->lines[w->lineCount++], D_00D7BB38, t[37]);
        sprintf(buf, t[39], gArenaEmTameNames[w->arenaBefore.gaugeLevel]);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, buf);
        sprintf(buf2, t[40], gArenaEmRankNames[(s8)SaveRecord_KindIndex_Unk((u8 *)&w->arenaBefore)]);
        Sjis_Widen(buf2, buf);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, buf);
        sprintf(buf2, t[41], Monster_SizeFromPercent(w->arenaBefore.emKind, w->arenaBefore.sizePct));
        Sjis_Widen(buf2, buf);
        sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, buf);
        sprintf(WLINE(w), D_00D7BB38, t[38]);
        if (w->friendChanged != 0 || w->strengthChanged != 0 || w->sizeChanged != 0) {
            strcat(WLINE(w), t[42]);
        }
        w->lineCount++;
        e = &gSave.arenaEms[D_6ADB14];
        sprintf(buf, t[39], gArenaEmTameNames[e->gaugeLevel]);
        if (w->friendChanged != 0) {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BC30) /* colour 18, six full-width spaces, the line, colour 17 */, buf);
        } else {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, buf);
        }
        sprintf(buf2, t[40], gArenaEmRankNames[(s8)SaveRecord_KindIndex_Unk((u8 *)e)]);
        Sjis_Widen(buf2, buf);
        if (w->strengthChanged != 0) {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BC30) /* colour 18, six full-width spaces, the line, colour 17 */, buf);
        } else {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, buf);
        }
        sprintf(buf2, t[41], Monster_SizeFromPercent(w->arenaBefore.emKind, e->sizePct));
        Sjis_Widen(buf2, buf);
        if (w->sizeChanged != 0) {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BC30) /* colour 18, six full-width spaces, the line, colour 17 */, buf);
        } else {
            sprintf(w->lines[w->lineCount++], TXT(0x00D7BBD8) /* six full-width spaces, then the line */, buf);
        }
        if (w->lineCount < 10) {
            w->lineCount = 10;
        }
        sprintf(w->lines[w->lineCount++], D_00D7BB38, str[0]);
        break;
    }
}

void Plsel_ClearWork(void) {
    memset(gPlselWork, 0, 0x3B88);
}

void Plsel_SumQuestRewards(void) {
    PlselWork *w;
    u8 *q;

    w = gPlselWork;
    q = (u8 *)gQuest.info;
    if (Quest_IsRunning(1) == 1) {
        if (Quest_IsRunning(1) == 1) {
            if (Game_IsOnline() == 1) {
                if (gScene.questPaid != 0) {
                    w->fee = *(s32 *)(q + 8);
                    w->feeRate = 2.0f;
                    w->wtdFee = gPlselBonus.unk08 - w->fee;
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
                    w->wtdFee = gPlselBonus.unk08;
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
                w->fee = *(s32 *)(q + 8);
                w->feeRate = 1.0f;
                w->wtdFee = gPlselBonus.unk08 - w->fee;
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
            if (gQuest.reward != *(s32 *)(q + 0xC)) {
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
                w->subAMoney = *(s32 *)(q + 0x14);
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
                w->subBMoney = *(s32 *)(q + 0x18);
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

void Plsel_SumRecordRewards(void) {
    s32 i;
    PlselWork *w;
    s32 t;

    w = gPlselWork;
    for (i = 0; i < 8; i++) {
        if (gPlselBonus.recs[i].id != 0) {
            t = gPlselBonus.recs[i].type;
            if (t == 0 || t == 1 || t == 2) {
                if (gQuest.huntGoals[0].type == t && gQuest.huntGoals[0].kind == gPlselBonus.recs[i].id) {
                    w->wtdDouble = 1;
                }
                w->monsterMoney += gPlselBonus.recs[i].unk4 * gPlselBonus.recs[i].unk6 / gScene.playerCount;
            }
        }
    }
}

void Plsel_TallyRewards(void) {
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
            w->itemMoney += gPlselBonus.recs[i].unk6;
            gPlselBonus.unk66 = gPlselBonus.recs[i].id;
            gPlselBonus.unk68 = gPlselBonus.recs[i].unk4;
        }
    }
}

void Plsel_QueueAward(s32 id) {
    PlselWork *w;

    w = gPlselWork;
    if (w->awardCount < 0x20) {
        w->awards[w->awardCount] = id;
        w->awardCount++;
    }
}

/* Queues the rank awards (gPlselRankAwardRanks: the rank that earns each, 0xFF ends;
 * gPlselRankAwards: the award) up to rank arg0. */
void Plsel_CheckRankAwards(s16 rankAfter) {
    s32 i;
    u8 v;
    u8 *p = gPlselRankAwardRanks;

    for (i = 0, v = *p; v != 0xFF; i++, v = *++p) {
        if ((s16)(v & 0xFF) > rankAfter) {
            break;
        }
        if ((s16)(v & 0xFF) <= rankAfter && Award_Grant(gPlselRankAwards[i]) == 1) {
            Plsel_QueueAward(gPlselRankAwards[i]);
        }
    }
}

void Plsel_ResetPointRate(PlselWork *w) {
    w->pointRate = 0x64;
}

void Plsel_ComputeReward(void) {
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
    if (Quest_IsRunning(1) == 1) {
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
    w->hrpResult = AddHrPoints(w->hrpTotal);
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

void Plsel_CheckAwardsOffline(void) {
    PlselWork *w;
    s32 k;
    s32 v;
    u16 b;
    u16 a;
    s32 j;
    PlselKind *e;
    s32 sum;
    s32 flag;
    s32 n;
    s32 t;
    u16 *p;

    w = gPlselWork;
    if (Quest_IsDrill() == 1) {
        return;
    }
    t = w->questMoney;
    t += w->extraMoney;
    t += w->monsterMoney;
    t += w->itemMoney;
    if (gSave.money + t >= 2000000 && (Award_Grant(0x1E) == 1 || gPlselShopMoney[1] == 1)) {
        Plsel_QueueAward(0x1E);
    }
    Plsel_CheckVersusAwards();
    e = gRecordCountEntries;
    v = e->id;
    sum = 0;
    flag = 0;
    j = 0;
    while (v != 0) {
        k = e->kind;
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
        if (gSave.recordCounts[k] >= 0x1E && Award_Grant(gPlselCountAwards[j]) == 1) {
            Plsel_QueueAward(gPlselCountAwards[j]);
        }
        e++;
        j++;
        v = e->id;
    }
    if (sum >= 0x1E && Award_Grant(0x81) == 1) {
        Plsel_QueueAward(0x81);
    }
    p = gPlselAwardTblA;
    v = *p;
    while (v != 0xFFFF) {
        a = p[1];
        p += 2;
        if (gSave.recordCounts[v] >= 100 && Award_Grant(a) == 1) {
            Plsel_QueueAward(a);
        }
        v = *p;
    }
    p = gPlselOfflineAwardsA;
    b = *p;
    while (b != 0xFFFF) {
        if (b == gQuest.stage) {
            if (gSave.recordCounts[14] >= 0x1E && Award_Grant(0x8F) == 1) {
                Plsel_QueueAward(0x8F);
            }
            return;
        }
        p++;
        b = *p;
    }
    p = gPlselOfflineAwardsB;
    b = *p;
    while (b != 0xFFFF) {
        if (b == gQuest.stage) {
            if (gSave.recordCounts[15] >= 0x1E && Award_Grant(0x90) == 1) {
                Plsel_QueueAward(0x90);
            }
            return;
        }
        p++;
        b = *p;
    }
    p = gPlselOfflineAwardsC;
    b = *p;
    while (b != 0xFFFF) {
        if (Save_TestListBit(b) == 0) {
            break;
        }
        p++;
        b = *p;
    }
    if (*p == 0xFFFF && Award_Grant(0x91) == 1) {
        Plsel_QueueAward(0x91);
    }
    n = 0;
    p = gPlselOfflineAwardsD;
    b = *p;
    while (b != 0xFFFF) {
        if (Save_TestListBit(b) == 0) {
            break;
        }
        p++;
        b = *p;
    }
    if (*p == 0xFFFF) {
        n++;
        if (Award_Grant(0x92) == 1) {
            Plsel_QueueAward(0x92);
        }
    }
    p = gPlselOfflineAwardsE;
    b = *p;
    while (b != 0xFFFF) {
        if (Save_TestListBit(b) == 0) {
            break;
        }
        p++;
        b = *p;
    }
    if (*p == 0xFFFF && ++n == 2 && Award_Grant(0x93) == 1) {
        Plsel_QueueAward(0x93);
    }
}

void Plsel_QueueAward(s32 arg0);
void Plsel_CheckCounterNotices(void);
s32 RateFlag_SetQuiet(u8 bit);
s32 Quest_IsVersus(void);
void Plsel_CheckVersusAwards(void) {
    if (Quest_IsVersus() == 1) {
        if (((s32) gSave.versusWins >= 0x32) && (Award_Grant(0x8D) == 1)) {
            Plsel_QueueAward(0x8D);
            if (RateFlag_SetQuiet(7) == 1) {
                Plsel_CheckCounterNotices();
            }
        }
        if (((s32) gSave.versusLosses >= 0x32) && (Award_Grant(0x8E) == 1)) {
            Plsel_QueueAward(0x8E);
        }
    }
}

void Plsel_CheckAwards(void) {
    PlselWork *w;
    s32 k;
    s32 v;
    u16 b;
    u16 a;
    s32 i;
    s32 j;
    PlselKind *e;
    s32 flag;
    s32 sum;
    PlselList *l;
    u16 *p;

    w = gPlselWork;
    if (Quest_IsDrill() != 1) {
        return;
    }
    e = gRecordCountEntries;
    v = e->id;
    l = &w->rewList;
    sum = 0;
    flag = 0;
    j = 0;
    while (v != 0) {
        k = e->kind;
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
        if (gSave.recordCounts[k] >= 0x1E && Award_Grant(gPlselCountAwards[j]) == 1) {
            Plsel_QueueAward(gPlselCountAwards[j]);
        }
        e++;
        j++;
        v = e->id;
    }
    if (sum >= 0x1E && Award_Grant(0x81) == 1) {
        Plsel_QueueAward(0x81);
    }
    p = gPlselAwardTblA;
    v = *p;
    while (v != 0xFFFF) {
        a = p[1];
        p += 2;
        if (gSave.recordCounts[v] >= 100 && Award_Grant(a) == 1) {
            Plsel_QueueAward(a);
        }
        v = *p;
    }
    if (l->zenny + l->payout >= 200000 && Award_Grant(0x88) == 1) {
        Plsel_QueueAward(0x88);
    }
    for (p = gPlselAwardTblD, i = 0; i < 12; i++, p++) {
        if (Save_IsItemObtained(&gSave, *p) == 0) {
            break;
        }
    }
    if (i == 12 && Award_Grant(0x89) == 1) {
        Plsel_QueueAward(0x89);
    }
    if (l->zenny + l->payout >= 15000 && Award_Grant(0x8A) == 1) {
        Plsel_QueueAward(0x8A);
    }
    p = gPlselAwardTblB;
    b = *p;
    while (b != 0xFFFF) {
        if (Save_TestListBit(b) == 0) {
            break;
        }
        p++;
        b = *p;
    }
    if (*p == 0xFFFF && Award_Grant(0x8B) == 1) {
        Plsel_QueueAward(0x8B);
    }
    p = gPlselAwardTblC;
    b = *p;
    while (b != 0xFFFF) {
        if (Save_TestListBit(b) == 0) {
            break;
        }
        p++;
        b = *p;
    }
    if (*p == 0xFFFF && Award_Grant(0x8B) == 1) {
        Plsel_QueueAward(0x8B);
    }
    p = gPlselTrainingAwards;
    b = *p;
    while (b != 0xFFFF) {
        if (Save_TestListBit(b) == 0) {
            break;
        }
        p++;
        b = *p;
    }
    if (*p == 0xFFFF && Award_Grant(0x8C) == 1) {
        Plsel_QueueAward(0x8C);
    }
}

void Plsel_CheckTrainingAwards(void) {
    u16 *p;
    u16 id;

    if (Quest_IsTraining() == 1) {
        p = gPlselTrainingAwards;
        id = *p;
        while (id != 0xFFFF) {
            if (Save_TestListBit(id) == 0) {
                break;
            }
            p++;
            id = *p;
        }
        if (*p == 0xFFFF && Award_Grant(0x8C) == 1) {
            Plsel_QueueAward(0x8C);
        }
    }
}

void Plsel_CheckSizeAwards(void) {
    u8 *p;
    u8 v;
    s32 k;
    s32 i;
    s32 big;
    s32 small;
    s32 crown;
    MonsterNoteDef *d;
    u16 m;

    p = gPlselSizeAwards;
    v = *p;
    while (v != 0xFF) {
        k = p[3];
        /* `+ 0` makes the last argument an int expression, which the game
         * loads before p[1] (as a call or macro there would). */
        if (Records_HuntedAtLeast((s32)v, &gSave, p[1], p[2] + 0) == 1 && Award_Grant(k) == 1) {
            Plsel_QueueAward(k);
        }
        p += 4;
        v = *p;
    }
    if (gSave.recordCounts[16] >= 100 && Award_Grant(0x63) == 1) {
        Plsel_QueueAward(0x63);
    }
    big = 0;
    small = 0;
    crown = 0;
    for (i = 1; i < 0x49; i++) {
        d = MonsterNote_FindKind(MonsterGroup_First(i));
        if (d != NULL) {
            m = gSave.sizeMax[i];
            if (m >= d->goldCrown && d->bigCrown != -1) {
                big++;
            }
            if (m >= d->bigCrown && d->bigCrown != -1) {
                crown++;
            }
            if (gSave.sizeMin[i] <= d->smallCrown && gSave.sizeMin[i] > 0 && d->smallCrown != -1) {
                small++;
            }
        }
    }
    if (big >= 5 && Award_Grant(0x64) == 1) {
        Plsel_QueueAward(0x64);
    }
    if (big >= 10 && Award_Grant(0x65) == 1) {
        Plsel_QueueAward(0x65);
    }
    if (big >= 15) {
        if (Award_Grant(0x66) == 1) {
            Plsel_QueueAward(0x66);
        }
        if (RateFlag_SetQuiet(9) == 1) {
            Plsel_CheckCounterNotices();
        }
    }
    if (crown >= 5 && Award_Grant(0x67) == 1) {
        Plsel_QueueAward(0x67);
    }
    if (crown >= 10 && Award_Grant(0x68) == 1) {
        Plsel_QueueAward(0x68);
    }
    if (crown >= 15 && Award_Grant(0x69) == 1) {
        Plsel_QueueAward(0x69);
    }
    if (small >= 5 && Award_Grant(0x6A) == 1) {
        Plsel_QueueAward(0x6A);
    }
    if (small >= 10 && Award_Grant(0x6B) == 1) {
        Plsel_QueueAward(0x6B);
    }
    if (small >= 15 && Award_Grant(0x6C) == 1) {
        Plsel_QueueAward(0x6C);
    }
}

void Plsel_ComputeAllRewards(void) {
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
    Plsel_CheckAwardsOffline();
    gPlselBonus.unk12 = 0;
    Plsel_CheckSizeRecords(&gScene, 2);
    Plsel_CheckSizeAwards();
    w->moneyTotal = w->questMoney;
    w->moneyTotal += w->extraMoney;
    w->moneyTotal += w->monsterMoney;
    w->moneyTotal += w->itemMoney;
    Save_AddMoney(w->moneyTotal);
    w->timeout = 0xE10;
    w->unk0000 = 0;
    w->pageCount = 4;
    Plsel_BuildResultText(w->page);
}

s32 Gs_GetState(u8 reg);
s32 Tex_Use(s32 n, u32 *handles);
s32 EntryPicture_GetUV(s32 *uv0, s32 *uv1, u16 id);
s32 MonsterNote_DrawLargeCrown(u16 i, s16 x, s16 y, s32 v);
s32 MonsterNote_DrawSmallCrown(u16 i, s16 x, s16 y, s32 v);
extern s32 gResultsBgTex;
extern s32 gResultsIconTex;
extern s32 gPlselResultsTex;

/* The results screen background: the panel, the monster icons of pages 5
 * and 7, the page arrows, the closing zoom, and the size records of page 5. */
void Plsel_DrawResultsBg(void) {
    PlselWork *w;
    s32 id;
    s32 s6C;
    s32 s5F;
    s32 s5E;
    s32 s63;
    Sprite s;
    s32 k;
    f32 *z;
    f32 d;
    f32 sc;
    MonsterNoteDef *def;

    w = gPlselWork;
    s6C = Gs_GetState(0x6C);
    s5F = Gs_GetState(0x5F);
    s5E = Gs_GetState(0x5E);
    s63 = Gs_GetState(0x63);
    Gs_SetState(0x6C, 0);
    Gs_SetState(0x5F, 7);
    Gs_SetState(0x5E, 1);
    Gs_SetState(0x63, 0);
    Tex_Use(1, (u32 *)&gResultsBgTex);
    Gs_SetState(4, gResultsBgTex);
    s.x = 0;
    s.y = 0;
    s.w = 0x280;
    s.h = 0x1C0;
    s.u0 = 0;
    s.v0 = 0;
    s.u1 = 0xFF;
    s.v1 = 0xFF;
    s.color = -1;
    Sprite_Draw(&s);
    if (gScene.step < 0x10 || gScene.step > 0x12) {
        if (w->page == 5 || w->page == 7) {
            if (w->page == 5) {
                id = w->recordGroups[w->recordGroupCur];
            } else {
                id = *(s16 *)((u8 *)w + 0x6E2);
            }
            Gs_SetState(0x63, 0x10000);
            Tex_Use(1, (u32 *)&gResultsIconTex);
            Gs_SetState(4, gResultsIconTex);
            s.x = 0x6E;
            s.y = 0x9A;
            s.w = 0x40;
            s.h = 0x40;
            EntryPicture_GetUV((s32 *)&s.u0, (s32 *)&s.u1, id);
            s.color = -1;
            Sprite_Draw(&s);
            s.y = 0xF2;
            Sprite_Draw(&s);
        }
        if (Quest_IsDrill() != 1) {
            if ((u32)w->page <= 2 || w->page == 3) {
                Gs_SetState(0x5E, 0x32);
                if (w->pageCount > 1) {
                    PlselRes_DrawArrow(1, 0x228, 0x58, 1);
                    PlselRes_DrawArrow(0, 0x1E6, 0x58, 1);
                }
            }
        }
    }
    if (w->closeState == 2) {
        Gs_SetState(0x5E, 0x32);
        Gs_SetState(0x63, 0);
        Tex_Use(1, (u32 *)&gPlselResultsTex);
        Gs_SetState(4, gPlselResultsTex);
        k = 0x1D - w->closeTimer;
        if (k > 2) {
            k = 2;
        }
        z = (f32 *)gPlselResultsBgDepth + k * 2;
        d = (s32)(150.0f * z[0]);
        sc = 160.0f * z[1];
        s.x = 360.0f + (150.0f - d) / 2.0f;
        s.y = 180.0f + (150.0f - d) / 2.0f;
        s.w = d;
        s.h = d;
        s.u0 = 0;
        s.v0 = 0;
        s.u1 = 0x4F;
        s.v1 = 0x4F;
        s.color = ((u32)sc << 24) | 0x50140A;
        Sprite_Draw(&s);
    }
    Gs_SetState(0x6C, s6C);
    Gs_SetState(0x5F, s5F);
    Gs_SetState(0x5E, s5E);
    Gs_SetState(0x63, s63);
    if (gScene.step >= 0xD && gScene.step < 0x10 && w->page == 5) {
        def = MonsterNote_FindKind(MonsterGroup_First(*((u8 *)w + 0x210)));
        if (w->oldLargest > 0) {
            MonsterNote_DrawLargeCrown(def->note, 0xC9, 0xB1, (s16)w->oldLargest);
        }
        if (w->oldSmallest > 0) {
            MonsterNote_DrawSmallCrown(def->note, 0xC9, 0xC5, (s16)w->oldSmallest);
        }
        if (w->oldLargest < w->newLargest && w->newLargest > 0) {
            MonsterNote_DrawLargeCrown(def->note, 0xC9, 0x108, (s16)w->newLargest);
        }
        if (w->oldSmallest == 0) {
            MonsterNote_DrawSmallCrown(def->note, 0xC9, 0x11C, (s16)w->newSmallest);
        } else if (w->newSmallest < w->oldSmallest && w->newSmallest > 0) {
            MonsterNote_DrawSmallCrown(def->note, 0xC9, 0x11C, (s16)w->newSmallest);
        }
    }
    Draw_Icons((Icon *)gPlselResultsBgIcon, 1);
}

extern s32 D_00469F38;
extern s32 gHighClassBonusText;
void Plsel_DrawResultPage(s32 unused) {
    s32 i;
    s32 size;
    s32 x;
    s32 y;
    PlselWork *w;
    s32 c;
    char buf[0x80];

    w = gPlselWork;
    size = 0x12;
    c = Str_Length(gResultRuleText);
    c *= 9.0f;
    c = 640.0f - c;
    x = c / 2.0f;
    y = 0x58;
    Text_SetColor(0x31);
    Font_SetSpacing(size, size);
    for (i = 0; i < w->lineCount; i++) {
        if (i == 0) {
            Font_SetSpacing(0x18, 0x18);
            Font_SetPos(x, y - 6);
            Text_Printf((s8 *) w->lines[i]);
            size = 0x12;
            Font_SetSpacing(size, size);
        } else {
            Font_SetPos(x, y);
            if (w->lines[i][0] != 0) {
                Text_Printf((s8 *) w->lines[i]);
            }
        }
        y += size + 4;
    }
    Font_SetPos(0x1EA, 0x164);
    Text_Printf((s8 *) gPlselTexts[111]);
    Font_SetPos(0x1B8, 0x17C);
    sprintf(buf, gPlselTexts[112], w->timeout / 30);
    Text_Printf((s8 *) buf);
    if (w->unk0000 == 0) {
        switch (w->page) {
        case 0:
            if (w->moneyTotal != 0 && w->questMoneyCut != 0 && w->lowRankCut != 0) {
                Font_SetPos(0x12C, 0x58);
                Text_Printf((s8 *) gHighClassBonusText);
            }
            break;
        case 2:
            if (w->monsterMoney != 0 && w->questMoneyCut != 0 && w->lowRankCut != 0) {
                Font_SetPos(0x12C, 0x58);
                Text_Printf((s8 *) gHighClassBonusText);
                Font_SetPos(0x202, 0x58);
                sprintf(buf, D_00D7BBC0, w->page + 1, w->pageCount);
                Text_PrintfColored(buf);
                return;
            }
            break;
        case 1:
            if (w->questMoney != 0 && w->questMoneyCut != 0 && w->lowRankCut != 0) {
                Font_SetPos(0x12C, 0x58);
                Text_Printf((s8 *) gHighClassBonusText);
            }
            break;
        }
        Font_SetPos(0x202, 0x58);
        sprintf(buf, D_00D7BBC0, w->page + 1, w->pageCount);
        Text_PrintfColored(buf);
    }
    if (w->unk0000 == 1 && w->hrpTotal != 0 && w->lowRankCut != 0) {
        Font_SetPos(0x19A, 0x58);
        Text_Printf((s8 *) gHighClassBonusText);
    }
    if (w->unk0000 == 0 && w->page == 3) {
        if (w->firstLine > 0) {
            Font_SetSpacing(0x1C, 0x1C);
            Font_SetPos(0x1F4, 0x78);
            if (gFrameTick & 0x10) {
                Text_PrintfColored(D_00D7BBB0);
            }
        }
        if (w->firstLine + 7 < w->itemLines) {
            Font_SetSpacing(0x1C, 0x1C);
            Font_SetPos(0x1F4, 0x122);
            if (gFrameTick & 0x10) {
                Text_PrintfColored(D_00D7BBB8);
            }
        }
    }
    Font_SetSpacing(0x12, 0x12);
    if (w->unk0000 == 3 && w->page == 5) {
        if (w->newLargest > 0 && ((w->oldLargest == 0 && w->newLargest > 0) || (w->oldLargest > 0 && w->oldLargest < w->newLargest))) {
            Font_SetPos(0x1D8, 0xB0);
            Text_Printf((s8 *) D_00469F38);
        }
        if (w->newSmallest > 0 && ((w->oldSmallest == 0 && w->newSmallest > 0) || (w->oldSmallest > 0 && w->newSmallest < w->oldSmallest))) {
            Font_SetPos(0x1D8, 0xC6);
            Text_Printf((s8 *) D_00469F38);
        }
        if (w->recordGroupCount != 1) {
            if (w->recordGroupCur > 0) {
                Font_SetSpacing(0x1C, 0x1C);
                Font_SetPos(0x1F4, 0x78);
                if (gFrameTick & 0x10) {
                    Text_PrintfColored(D_00D7BBB0);
                }
            }
            if (w->recordGroupCur < w->recordGroupCount - 1) {
                Font_SetSpacing(0x1C, 0x1C);
                Font_SetPos(0x1F4, 0x122);
                if (gFrameTick & 0x10) {
                    Text_PrintfColored(D_00D7BBB8);
                }
            }
            return;
        }
    }
}

s32 PlselRes_RewardPageInput(u8 *o) {
    s32 ret;
    PlselWork *w;
    u16 pad2;
    u16 pad;

    w = gPlselWork;
    if (*(s16 *)(o + 4) != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    pad2 = gPadState[0].repeat | pad;
    if (w->closeState == 1) {
        PlselRes_Close();
    }
    if (w->closeState == 2) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    w->unk0000 = 0;
    w->pageCount = 4;
    Plsel_BuildResultText(w->page);
    if (pad & 0x400) {
        PlselRes_PlayPageSe();
        w->page++;
        if (w->page > w->pageCount - 1) {
            w->page = 0;
        }
        w->firstLine = 0;
        Plsel_BuildResultText(w->page);
    } else if (pad & 0x800) {
        PlselRes_PlayPageSe();
        w->page--;
        if (w->page < 0) {
            w->page = w->pageCount - 1;
        }
        w->firstLine = 0;
        Plsel_BuildResultText(w->page);
    } else {
        if (w->page == 3) {
            if ((pad2 & 0x2000) && w->firstLine != 0) {
                PlselRes_PlayPageSe();
                w->firstLine--;
                goto end;
            }
            if ((pad2 & 0x1000) && w->firstLine + 7 < w->itemLines) {
                PlselRes_PlayPageSe();
                w->firstLine++;
                goto end;
            }
        }
        if ((pad & 0x20) && w->closeState == 0) {
            PlselRes_PlayDecideSe();
            PlselRes_BeginClose();
        }
    }
end:
    return 0;
}

void PlselRes_InitRecordPage(void) {
    PlselWork *w;

    w = gPlselWork;
    w->unk0000 = 3;
    w->pageCount = 1;
    w->page = 5;
    Plsel_BuildResultText(w->page);
    if (w->newRecord == 1) {
        Stream_Play(1, 0x1D);
    }
}

s32 PlselRes_RecordPageInput(u8 *o) {
    s32 ret;
    PlselWork *w;
    u16 pad2;
    u16 pad;

    w = gPlselWork;
    if (*(s16 *)(o + 4) != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    pad2 = gPadState[0].repeat | pad;
    if (w->closeState == 1) {
        PlselRes_Close();
    }
    if (w->closeState == 2) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    if (pad2 & 0x2000) {
        if (w->recordGroupCur > 0) {
            w->recordGroupCur--;
            Plsel_BuildResultText(w->page);
            return 0;
        }
    }
    if (pad2 & 0x1000) {
        if (w->recordGroupCur < w->recordGroupCount - 1) {
            w->recordGroupCur++;
            Plsel_BuildResultText(w->page);
            return 0;
        }
    }
    if ((pad & 0x20) && w->closeState == 0) {
        PlselRes_PlayDecideSe();
        PlselRes_BeginClose();
    }
    return 0;
}

void PlselRes_InitPointsPage(void) {
    PlselWork *w;

    w = gPlselWork;
    w->unk0000 = 1;
    w->pageCount = 1;
    w->page = 4;
    Plsel_BuildResultText(w->page);
}

s32 PlselRes_PointsPageInput(u8 *o) {
    u16 pad;
    s32 ret;
    PlselWork *w;

    w = gPlselWork;
    if (*(s16 *)(o + 4) != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    if (w->closeState == 1) {
        PlselRes_Close();
    }
    if (w->closeState == 2) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    if ((pad & 0x20) && w->closeState == 0) {
        PlselRes_PlayDecideSe();
        PlselRes_BeginClose();
    }
    return 0;
}

void PlselRes_InitMonsterPage(void) {
    PlselWork *w;
    ArenaEm *e;

    w = gPlselWork;
    w->unk0000 = 4;
    w->pageCount = 1;
    w->page = 7;
    w->timeout = 0xE10;
    e = &gSave.arenaEms[D_6ADB14];
    if (e->gaugeLevel != w->arenaBefore.gaugeLevel) {
        w->friendChanged = 1;
    }
    if (e->level != w->arenaBefore.level) {
        w->strengthChanged = 1;
    }
    if (e->sizePct != w->arenaBefore.sizePct) {
        w->sizeChanged = 1;
    }
    Plsel_BuildResultText(w->page);
    if (w->friendChanged == 1 || w->strengthChanged == 1 || w->sizeChanged == 1) {
        Stream_Play(1, 0x1D);
    }
}

s32 PlselRes_MonsterPageInput(SceneState *sc) {
    s32 ret;

    if (sc->timer != 0) {
        return 0;
    }
    ret = 0;
    if (gPadState[0].pressed & 0x20) {
        PlselRes_PlayDecideSe();
        ret = 1;
    }
    return ret;
}

void Plsel_InitSaveRecordMenu(void) {
    PlselWork *w;
    s32 i;

    w = gPlselWork;
    w->page = 0;
    w->timeout = 0xE10;
    w->menuStep = 0;
    w->help1 = 0;
    w->help2 = 0;
    gHelpShow = 0;
    gHelpPage = 6;
    gHelpColumn = 0;
    Shake_Start(6, 0, -180.0f);
    w->recordCount = 0;
    for (i = 0; i < 3; i++) {
        if (gSave.arenaEms[i].emKind != 0) {
            w->recordCount++;
        }
    }
}

s32 Plsel_SaveRecordMenu(void) {
    PlselWork *w;
    u16 pad;
    u16 pad2;
    s32 ret;
    s32 i;

    ret = 1;
    w = gPlselWork;
    pad = gPadState[0].pressed;
    pad2 = gPadState[0].repeat | pad;
    w->timeout--;
    if (w->timeout == 0) {
        if (w->menuStep >= 3) {
            return 0;
        }
        if (w->menuStep < 3) {
            w->timeout = 0x5A;
            w->menuStep = 4;
            PlselRes_PlayCancelSe();
        }
    }
    switch (w->menuStep) {
    case 0:
        if ((pad2 & 0x2000) && w->page != 0) {
            PlselRes_PlayPageSe();
            w->page = 0;
        } else if ((pad2 & 0x1000) && w->page == 0) {
            PlselRes_PlayPageSe();
            w->page = 1;
        } else if (pad & 0x20) {
            PlselRes_PlayDecideSe();
            if (w->page == 1) {
                ret = 0;
            } else if (w->page == 0) {
                w->menuStep = 1;
                if (w->recordCount == 3) {
                    w->recordSlot = 0;
                } else {
                    w->recordSlot = w->recordCount;
                }
            }
        }
        break;
    case 1:
        w->help1 = 3;
        w->help2 = 4;
        if (w->recordCount == 3) {
            w->help1 = 1;
            if ((pad2 & 0x2000) && w->recordSlot != 0) {
                PlselRes_PlayPageSe();
                w->recordSlot--;
                break;
            }
            if ((pad2 & 0x1000) && w->recordSlot != 2) {
                PlselRes_PlayPageSe();
                w->recordSlot++;
                break;
            }
        }
        if (pad & 0x40) {
            PlselRes_PlayCancelSe();
            w->menuStep = 0;
        } else if (pad & 0x20) {
            PlselRes_PlayDecideSe();
            if (w->recordCount == 3) {
                w->menuStep = 2;
            } else {
                goto save;
            }
        }
        break;
    case 2:
        w->help1 = 2;
        w->help2 = 4;
        if (pad & 0x40) {
            PlselRes_PlayCancelSe();
            w->menuStep = 1;
        } else if (pad & 0x20) {
            PlselRes_PlayDecideSe();
        save:
            i = w->recordSlot;
            w->menuStep = 3;
            memset(&gSave.arenaEms[i], 0, 0x1C);
            strcpy((char *)gSave.arenaEms[i].name, TXT(0x00D7BC70) /* the default name of an arena record's monster */);
            gSave.arenaEms[i].emKind = gQuestEmSizePct;
            gSave.arenaEms[i].level = gQuest.capturedLevel;
            gSave.arenaEms[i].unk8 = 100;
            gSave.arenaEms[i].unk18 = 2;
            gSave.arenaEms[i].sizePct = gQuest.capturedSize;
        }
        break;
    case 3:
        w->help1 = 3;
        w->help2 = 6;
        if (pad & 0x20) {
            PlselRes_PlayDecideSe();
            ret = 0;
        }
        break;
    case 4:
        break;
    }
    return ret;
}

extern char *gArenaEmBlankLine[];
void Plsel_FormatRecord(s32 type, ArenaEm *e, char *out) {
    char buf[0x80];

    if (e->emKind == 0) {
        strcpy(out, gArenaEmBlankLine[0]);
        return;
    }
    buf[0] = 0;
    switch (type) {
    case 0:
        if (*(s8 *)e->name == 0) {
            sprintf(buf, gArenaEmLineFmts[type], gRecordDashesText);
        } else {
            sprintf(buf, gArenaEmLineFmts[type], e->name);
        }
        break;
    case 1:
        sprintf(buf, gArenaEmLineFmts[type], gMonsterNames[e->emKind]);
        break;
    case 2:
        sprintf(buf, gArenaEmLineFmts[type], gArenaEmRankNames[(s8)SaveRecord_KindIndex_Unk((u8 *)e)]);
        break;
    case 3:
        sprintf(buf, gArenaEmLineFmts[type], fptodp(Monster_SizeFromPercent(e->emKind, e->sizePct)));
        break;
    }
    Sjis_Widen(buf, out);
}

extern char *gArenaEmReleasedText;
void Plsel_DrawSaveRecordMenu(void) {
    PlselWork *w;
    s32 i;
    s32 j;
    s32 y;
    s32 y2;
    char **tbl;
    ArenaEm *e;
    char buf[0x80];
    char text[0x80];
    ListPanel win;
    ArenaEm rec;
    s8 t;

    w = gPlselWork;
    y = 0x28;
    if (w->menuStep == 4) {
        ListPanel_DrawAlpha(&gPlselSaveRecPanel2, 0, -1, 0xA0);
        Text_SetColor(0);
        Font_SetSpacing(0x12, 0x12);
        Font_SetPos(0xCB, 0xD7);
        Text_Printf((s8 *) gArenaEmReleasedText);
        return;
    }
    if (w->menuStep < 3) {
        win = gPlselSaveRecWinA;
        win.x = 0x20;
        win.y = y;
        ListPanel_Draw(&win, (s16 *)0, (char *)0, -1);
    }
    ListPanel_Draw(&gPlselSaveRecPanel, (s16 *)0, (char *)0, w->page);
    if (w->menuStep == 4) {
        sprintf(buf, gPlselTexts[31], 0);
    } else {
        sprintf(buf, gPlselTexts[31], w->timeout / 30);
    }
    Window_DrawText(&gPlselSaveRecText, (s16 *)0, (u8 *)buf);
    Text_SetColor(0x20);
    Font_SetSpacing(0x12, 0x12);
    if (w->menuStep < 3) {
        rec.emKind = gQuestEmSizePct;
        rec.unk8 = 100;
        t = gQuest.capturedLevel;
        rec.unk18 = 2;
        rec.level = t;
        rec.sizePct = gQuest.capturedSize;
        for (i = 0, tbl = gArenaEmLineFmts; i < 4; i++, y += 0x14, tbl++) {
            Font_SetPos(0x20, y);
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
                sprintf(buf, *tbl, fptodp(Monster_SizeFromPercent(gQuestEmSizePct, gQuest.capturedSize)));
                break;
            }
            Sjis_Widen(buf, text);
            Text_PrintfColored(text);
        }
    }
    if (w->menuStep != 0) {
        y = 0x28;
        e = gSave.arenaEms;
        for (i = 0; i < 3; i++) {
            win = gPlselSaveRecWinA;
            win.x = 0x16A;
            win.y = y;
            ListPanel_Draw(&win, (s16 *)0, (char *)0, -1);
            if (w->menuStep != 0 && i == w->recordSlot && w->recordCount == 3) {
                Menu_DrawGlowBar(0x1D8, y, 0xDC, 0x50, 0xFF20C0C0);
            }
            for (y2 = y, j = 0; j < 4; j++, y2 += 0x14) {
                Plsel_FormatRecord(j, e, text);
                Font_SetPos(0x16A, y2);
                Text_PrintfColored(text);
            }
            y += 0x64;
            e++;
        }
        win = gPlselSaveRecWinB;
        win.x = 0x168;
        win.y = 0x15E;
        ListPanel_Draw(&win, (s16 *)0, (char *)0, -1);
        if (w->help1 > 0) {
            Font_SetPos(0x168, 0x15E);
            Text_PrintfColored(gArenaEmPenHelp[w->help1]);
        }
        if (w->help2 > 0) {
            Font_SetPos(0x168, 0x186);
            Text_Printf((s8 *) gArenaEmPenHelp[w->help2]);
        }
    }
}

void Plsel_SnapshotVersusMonster(void) {
    PlselWork *w;
    s16 idx;
    s32 k;
    u16 v;

    Plsel_ClearWork();
    w = gPlselWork;
    idx = D_6ADB14;
    if (idx != -1) {
        w->arenaBefore = gSave.arenaEms[idx];
        k = gPlayers[gScene.localPlayer].side - 1;
        v = gQuest.teams[k].item;
        if (v != 0) {
            MonsterNote_Record(D_6ADADE[k * 14], v);
        }
    }
}

void PlselRes_DrawArrow(s32 flag, s16 x, s16 y, s32 mode) {
    Sprite s;

    Gfx_SetState63(1);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
    s.x = x;
    s.y = y;
    if (mode == 0) {
        s.w = 0x1E;
        s.h = 0x26;
    } else {
        s.w = 0x12;
        s.h = 0x14;
    }
    s.color = 0xFF32BC64;
    if (flag == 0) {
        s.u0 = 0xFF;
        s.u1 = 0xF0;
    } else {
        s.u0 = 0xF0;
        s.u1 = 0xFF;
    }
    s.v0 = 0xEC;
    s.v1 = 0xFF;
    if (mode == 0) {
        TexRect_DrawRotated((struct TexRect *)&s, 0xC000);
        return;
    }
    Sprite_Draw(&s);
}

extern s32 D_006AE974;
void Plsel_LoadResultsTextures(void) {
    Tex_PatchClut_Unk();
    Tex_LoadResultTex_Unk();
    if (gPlselResultsTex == 0) {
        LoadCompressedFile(gSysSettings.loadBuf, 0x26);
        gPlselResultsTex = Tex_CreateFromTim2(gSysSettings.loadBuf, 4);
    }
    if (D_006AE974 == 0) {
        LoadCompressedFile(gSysSettings.loadBuf, 0x22);
        D_006AE974 = Tex_CreateFromTim2(gSysSettings.loadBuf, 4);
    }
    Sound_ReloadBank7();
}

void func_00D72F10(void) {

}

void PlselRes_Close(void) {
    PlselWork *w;

    w = gPlselWork;
    if (w->closeState != 0 && w->closeState != 2) {
        w->closeState = 2;
        Sound_PlaySeFlat(7, 8, 0);
    }
}

void Plsel_QueueFlagUnlocks(void) {
    s32 item;
    s32 id;
    s32 i;
    u16 *e;
    PlselWork *w;

    w = gPlselWork;
    memset(w->notices, 0, 0x72);
    for (i = 0, e = gPlselFlagUnlocks; i < 0x39; i++, e += 2) {
        if (e[0] == 0xFFFF) {
            break;
        }
        id = e[0] & 0xFF;
        item = e[1];
        if (EventFlag_Test(id) != 1 && Save_TestEventFlag(item) == 1) {
            EventFlag_Set(id);
            w->notices[w->noticeCount] = e[0];
            w->noticeCount++;
        }
    }
}

void Plsel_CheckCounterNotices(void) {
    PlselWork *w;

    w = gPlselWork;
    if (Save_TestEventFlag(0xB6) == 1 && w->unk3B82 == 0) {
        w->notices[w->noticeCount] = 0x500;
        w->noticeCount++;
        w->unk3B82 = 1;
    }
    if ((gSave.rateFlags & 0x800) && SaveFlag_Test(0x7A) == 0) {
        SaveFlag_Set(0x7A);
        Plsel_QueueAward(0x7A);
    }
}

void Plsel_QueueOnlineQuestNotice(void) {
    PlselWork *w;
    u16 *p;
    s32 i;
    u16 v;

    w = gPlselWork;
    if (Quest_IsDrill() == 1 && w->unk3B83 == 0) {
        p = gPlselOnlineQuestNotices;
        i = 0;
        v = *p;
        while (v != 0) {
            if (v == gQuest.stage) {
                w->notices[w->noticeCount] = i | 0x300;
                w->noticeCount++;
                if (gQuest.stage == 0x2777) {
                    w->notices[w->noticeCount] = 0x400;
                    w->noticeCount++;
                }
                return;
            }
            p++;
            v = *p;
            i++;
        }
    }
}

void Plsel_AddCounterPoints(void) {
    PlselWork *w;

    w = gPlselWork;
    if (Save_AddCounterPoints() == 1) {
        if (Save_TestEventFlag(0xB6) != 0 && w->unk3B82 == 0) {
            w->notices[w->noticeCount] = 0x500;
            w->noticeCount++;
            w->unk3B82 = 1;
        }
        if ((gSave.rateFlags & 0x800) && SaveFlag_Test(0x7A) == 0) {
            SaveFlag_Set(0x7A);
            Plsel_QueueAward(0x7A);
        }
    }
}

s32 Plsel_HasQueuedAwards(void) {
    s32 i;
    PlselWork *w;

    w = gPlselWork;
    for (i = 0; i < 0x39; i++) {
        if (w->notices[i] != 0) {
            return 1;
        }
    }
    return 0;
}

void Plsel_BuildAwardText(void) {
    s32 t;
    s32 k;
    char **tbl;
    char *s;
    char *pct;
    s32 i;
    PlselWork *w;
    char buf[0x80];
    char tmp[0x80];

    w = gPlselWork;
    memset(w->lines, 0, sizeof(w->lines));
    w->unk0748 = 0;
    w->scroll = 0;
    w->lineCount = 0;
    for (i = 0; i < 0x39; i++) {
        if (w->notices[i] != 0) {
            t = (w->notices[i] & 0xFF00) >> 8;
            k = w->notices[i] & 0xFF;
            if (t == 2) {
                k -= 9;
            }
            if (t > 0) {
                for (tbl = gAwardNoticeLines[t - 1]; *tbl != NULL; tbl++) {
                    strcpy(buf, *tbl);
                    pct = strchr(buf, '%');
                    if (pct != NULL) {
                        strcpy(tmp, buf);
                        switch (pct[1]) {
                        case 'g':
                            s = gAwardGalleryTitles[k];
                            break;
                        case 'm':
                            s = gAwardDojoTitles[k];
                            break;
                        }
                        *strchr(tmp, '%') = 0;
                        strcat(tmp, s);
                        strcat(tmp, pct + 2);
                        strcpy(buf, tmp);
                    }
                    sprintf(w->lines[w->lineCount++], D_00D7BB38, buf);
                }
            }
        }
    }
    Stream_Play(1, 0x1D);
}

s32 PlselRes_AwardPageInput(void) {
    s32 ret;
    PlselWork *w;
    u16 pad2;
    u16 pad;

    w = gPlselWork;
    if (gScene.timer != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    pad2 = gPadState[0].repeat | pad;
    if (w->closeState == 1) {
        PlselRes_Close();
    }
    if (w->closeState == 2) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    if ((pad2 & 0x2000) && w->noticePage != 0) {
        PlselRes_PlayPageSe();
        w->noticePage--;
    }
    if ((pad2 & 0x1000) && w->noticePage < w->noticeCount - 1) {
        PlselRes_PlayPageSe();
        w->noticePage++;
    }
    if ((pad & 0x20) && w->closeState == 0) {
        PlselRes_PlayDecideSe();
        PlselRes_BeginClose();
    }
    return 0;
}

extern s32 D_00469F3C;
void Plsel_DrawAwardText(void) {
    char **t;
    s32 i;
    PlselWork *w;
    s32 end;
    s32 x;
    s32 y;
    s32 c;
    char buf[0x80];

    w = gPlselWork;
    t = &gResultRuleText;
    c = Str_Length(gResultRuleText);
    c *= 9.0f;
    c = 640.0f - c;
    x = c / 2.0f;
    y = 0x58;
    Text_SetColor(0x31);
    Font_SetSpacing(0x12, 0x12);
    Font_SetSpacing(0x18, 0x18);
    Font_SetPos(x, 0x52);
    Text_Printf((s8 *) D_00469F3C);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x, 0x6E);
    Text_Printf((s8 *) *(s32 *)&gResultRuleText);
    y += 0x58;
    end = w->noticePage * 3 + 3;
    for (i = w->noticePage * 3; i < end; i++) {
        if (w->lines[i][0] != 0) {
            c = Str_Length(w->lines[i]);
            c *= 9.0f;
            c = 640.0f - c;
            c /= 2.0f;
            Font_SetPos(c, y);
            Text_Printf((s8 *) w->lines[i]);
        }
        y += 0x16;
    }
    Font_SetPos(x, 0x134);
    Text_Printf((s8 *) *t);
    if (w->noticePage > 0) {
        Font_SetSpacing(0x1C, 0x1C);
        Font_SetPos(0x132, 0x78);
        if (gFrameTick & 0x10) {
            Text_PrintfColored(D_00D7BBB0);
        }
    }
    if (w->noticePage * 3 + 3 < w->lineCount) {
        Font_SetSpacing(0x1C, 0x1C);
        Font_SetPos(0x132, 0x122);
        if (gFrameTick & 0x10) {
            Text_PrintfColored(D_00D7BBB8);
        }
    }
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(0x1EA, 0x164);
    Text_Printf((s8 *) gPlselTexts[111]);
    Font_SetPos(0x1B8, 0x17C);
    sprintf(buf, gPlselTexts[112], w->timeout / 30);
    Text_Printf((s8 *) buf);
}

void Plsel_BuildItemText(void) {
    char *s;
    char *q;
    s32 i;
    u16 *id;
    PlselWork *w;
    HunterTitle *tbl;
    char *p;

    w = gPlselWork;
    memset(w->lines, 0, sizeof(w->lines));
    w->unk0748 = 0;
    w->scroll = 0;
    w->lineCount = 0;
    w->page = 4;
    id = w->awards;
    tbl = gHunterTitles;
    for (i = 0; i < w->awardCount; i++, id++) {
        if (w->lineCount + 4 >= 0x50) {
            break;
        }
        strcpy(w->lines[w->lineCount++], tbl[*id].text);
        s = gHunterTitleDescs[*id];
        strcpy(w->lines[w->lineCount], s);
        p = strchr(w->lines[w->lineCount], '\n');
        if (p != NULL) {
            *p = 0;
        }
        w->lineCount++;
        q = strchr(s, '\n');
        if (q == NULL) {
            w->lineCount++;
            w->lineCount++;
            continue;
        }
        strcpy(w->lines[w->lineCount], q + 1);
        p = strchr(w->lines[w->lineCount], '\n');
        if (p != NULL) {
            *p = 0;
        }
        w->lineCount++;
        p = strchr(q + 1, '\n');
        if (p == NULL) {
            w->lineCount++;
            continue;
        }
        strcpy(w->lines[w->lineCount], p + 1);
        w->lineCount++;
    }
    w->lineCount /= 4;
}

s32 PlselRes_ItemPageInput(void) {
    s32 ret;
    PlselWork *w;
    u16 pad2;
    u16 pad;

    w = gPlselWork;
    if (gScene.timer != 0) {
        return 0;
    }
    pad = gPadState[0].pressed;
    ret = 0;
    pad2 = gPadState[0].repeat | pad;
    if (w->closeState == 1) {
        PlselRes_Close();
    }
    if (w->closeState == 2) {
        if (--w->closeTimer <= 0) {
            ret = 1;
        }
        return ret;
    }
    if ((pad2 & 0x2000) && w->scroll != 0) {
        PlselRes_PlayPageSe();
        w->scroll--;
    }
    if ((pad2 & 0x1000) && w->scroll + 1 < w->lineCount) {
        PlselRes_PlayPageSe();
        w->scroll++;
    }
    if ((pad & 0x20) && w->closeState == 0) {
        PlselRes_PlayDecideSe();
        PlselRes_BeginClose();
    }
    return 0;
}

extern s32 D_00469F14;
void Plsel_DrawItemText(void) {
    char **t;
    PlselWork *w;
    s32 i;
    s32 size;
    s32 end;
    s32 x;
    s32 y;
    s32 c;
    char buf[0x80];

    w = gPlselWork;
    t = &gResultRuleText;
    c = Str_Length(gResultRuleText);
    c *= 9.0f;
    c = 640.0f - c;
    x = c / 2.0f;
    y = 0x58;
    Text_SetColor(0x31);
    Font_SetSpacing(0x12, 0x12);
    Font_SetSpacing(0x18, 0x18);
    Font_SetPos(x, 0x52);
    Text_Printf((s8 *) D_00469F14);
    Font_SetSpacing(0x12, 0x12);
    Font_SetPos(x, 0x6E);
    Text_Printf((s8 *) *(s32 *)&gResultRuleText);
    size = 0x18;
    y += 0x58;
    i = w->scroll * 4;
    end = i + 4;
    Font_SetSpacing(size, size);
    for (; i < end; i++) {
        if (w->lines[i][0] != 0) {
            c = Str_Length(w->lines[i]);
            c *= size / 2.0f;
            c = 640.0f - c;
            c /= 2.0f;
            Font_SetPos(c, y);
            Text_Printf((s8 *) w->lines[i]);
        }
        y += size + 4;
        size = 0x12;
        Font_SetSpacing(size, size);
    }
    y = 0x58;
    y += (size + 4) * 10;
    Font_SetPos(x, y);
    Text_Printf((s8 *) *t);
    if (w->scroll > 0) {
        Font_SetSpacing(0x1C, 0x1C);
        Font_SetPos(0x132, 0x78);
        if (gFrameTick & 0x10) {
            Text_PrintfColored(D_00D7BBB0);
        }
    }
    if (w->scroll + 1 < w->lineCount) {
        Font_SetSpacing(0x1C, 0x1C);
        Font_SetPos(0x132, 0x122);
        if (gFrameTick & 0x10) {
            Text_PrintfColored(D_00D7BBB8);
        }
    }
    Font_SetSpacing(size, size);
    Font_SetPos(0x1EA, 0x164);
    Text_Printf((s8 *) gPlselTexts[111]);
    Font_SetPos(0x1B8, 0x17C);
    sprintf(buf, gPlselTexts[112], w->timeout / 30);
    Text_Printf((s8 *) buf);
}

void Plsel_CountHunts(PlselWork *w) {
    s32 i;
    u8 k;

    for (i = 0; i < 0x4A; i++) {
        w->huntedByGroup[i] = 0;
        w->capturedByGroup[i] = 0;
    }
    for (i = 0; i < 0x4A; i++) {
        if (gQuest.hunted[i] != 0) {
            k = MonsterGroup_First(i);
            w->huntedByGroup[k] += gQuest.hunted[i];
        }
    }
    for (i = 0; i < 0x4A; i++) {
        if (gQuest.captured[i] != 0) {
            k = MonsterGroup_First(i);
            w->capturedByGroup[k] += gQuest.captured[i];
        }
    }
    for (i = 0; i < 0x4A; i++) {
        w->huntedLeft[i] = w->huntedByGroup[i];
        w->capturedLeft[i] = w->capturedByGroup[i];
    }
}

void Plsel_QueueQuestUnlock(u16 *list, s32 idx) {
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

void Plsel_QueueQuestUnlocks(void) {
    PlselWork *w;

    w = gPlselWork;
    Plsel_QueueQuestUnlock((u16 *)gPlselQuestUnlocks0, 0);
    Plsel_QueueQuestUnlock((u16 *)gPlselQuestUnlocks1, 1);
    Plsel_QueueQuestUnlock((u16 *)gPlselQuestUnlocks2, 2);
    if (Game_IsOnline() != 0 && gQuest.stage == 0x3AA9 && EventFlag_Test(0x31) != 1) {
        EventFlag_Set(0x31);
        EventFlag_Set(0x32);
        EventFlag_Set(0x33);
        EventFlag_Set(0x34);
        w->notices[w->noticeCount] = 0x700;
        w->noticeCount++;
        w->notices[w->noticeCount] = 0x800;
        w->noticeCount++;
        w->notices[w->noticeCount] = 0x900;
        w->noticeCount++;
        w->notices[w->noticeCount] = 0xA00;
        w->noticeCount++;
    }
}
