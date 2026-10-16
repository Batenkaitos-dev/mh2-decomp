/*
 * The quest state (gQuest, 0x560 bytes, cleared by Quest_Clear and set
 * up from the stage file by Quest_Init): the stage header and its tables,
 * the time left, the reward and the faint penalty, the quest phase
 * (SceneState.questPhase), and the monsters hunted, captured, their sizes and
 * broken parts during the quest.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "scene.h"
#include "save.h"
#include "quest.h"
#include "syssettings.h"
#include "mine_point.h"
#include "syssettings.h"
#include "objhead.h"
#include "qsync.h"

/* One of Capcom's units, 0x3A1070-0x3AC3A0 (it calls the static
 * Quest_DeliveredCount, defined further down), built at optimization level 4
 * throughout (docs/mwcc-matching.md, "Optimization level 4 files"). */
#pragma optimization_level 4

/* A player's start in the stage file (StageHeader.startAreas, one per player). */
typedef struct StartArea {
    s32 area;
    u8 unk04[0x10 - 0x04];
} StartArea;

/* Where a monster can appear in an area: count points at offset. */
typedef struct AreaSpawns {
    u16 area;                   /* 0xFFFF ends the list */
    u16 count;
    u32 offset;                 /* SpawnPoint[count], from gStageFile */
} AreaSpawns;

typedef struct SpawnPoint {
    s32 rotY;
    f32 x;
    f32 y;
    f32 z;
} SpawnPoint;

/* A range of monster sizes in percent (gEmSizeRanges). */
typedef struct RangeU8 {
    u8 min;
    u8 max;
} RangeU8;

/* An effect shown when a monster part breaks (gPartBreakEffects, 0xFF-terminated). */
typedef struct PartEffect {
    u8 kind;                    /* 0x0: Em_KindMatches(type, kind) */
    s8 bone;                    /* 0x1 */
    u16 part;                   /* 0x2 */
    f32 offset[3];              /* 0x4: from the bone */
} PartEffect;


extern u32 gStageFile;          /* base address of the stage file */
extern u16 **gStartAreaTables[];
extern u16 gEmAppearMax[];
extern PartEffect gPartBreakEffects[];
extern char *gDrillScoreTexts[];
extern char *gQuestNoticeTexts[];
extern u8 *gQuestTimedEventTbls[];

void *memset(void *p, s32 c, u32 n);
void *QuestRecord_Clear(void);

u16 Area_IdForCurrentTime(u16 area);
u16 Rand_Next(s32 stream);
s32 Player_IsLocal(void *obj);

/* Prototypes shared by the parts of this unit. */
void CamReq_TargetDown(); /* unit's own declaration: the definition (unk_323C90.c) takes no parameter; Quest_Update's call passes gQuest.result in a0, which it ignores, and the other call none */
s32 Cond_HasItems(void *unused, u16 *r);
s32 Em_HasRandomSize(s16 id);
s32 Em_HasRandomSize2(s16 id);
u8 Notice_Push(u8 kind, s16 a, s16 b);
u8 Notice_PushItem(char *item);
s16 Player_IsState(void *obj, u8 state, u8 sub);
/* unit's own declaration: with the definition's u8 (Player *p, u16 item, s16 n) (sub_main/pouch.c), Quest_GiveRewards: 2 of 189 words differ */
u16 Pouch_Add(Player *pl, u16 item, s16 n);
void QuestEm_CountCaptured(u16 i);
void QuestEm_CountHunted(u16 i);
QuestEm * QuestEm_FindByIndex(s16 idx, u16 area);
s32 QuestEm_RollYamaTsukamiB(s32 a);
Em * QuestEm_Spawn(QuestEm *q);
s32 QuestGoal_DeliverCountFor(u16 target, u16 i);
u16 QuestGoal_DeliverItem(u16 i);
s32 QuestGoal_IsClearTarget(u16 target);
s32 QuestGoal_OnCapture(u16 target);
s32 QuestGoal_OnKill(u16 target);
s32 QuestGoal_OnKillSlot(u16 target, u16 i);
s32 QuestGoal_OnPartBroken(u16 id, u16 part);
s32 Quest_CheckClear(s32 notify);
static s32 Quest_DeliveredCount(u16 item);
void Quest_SendKill(QuestEm *q, Player *p);
struct StageEntry;
void *StageEntry_GetData(struct StageEntry *e, s32 which);
void *Stage_FindEntryData(s32 *tbl, s32 id, s32 which, s8 idx);
void func_003AB680(); /* unit's own declaration: defined below as (void); its caller passes q and em in a0 and a1, which it ignores */
void StageObj_K0E_CreateAt(f32 *pos, s16 k);
s32 Game_IsOnline(void);
void QuestEm_SpawnArea(s32 a);
void EmMine_RemoveTimed(Em *em);
void QuestEm_SpawnAll(void);
QuestEm *QuestEm_FindForEm(Em *em);
void Quest_StoreElapsed(void);
u8 MonsterGroup_First(u8 group);
void Em_InitParts(Em *em);
void QNetIf_Leave(void);
void GmEm_ApplySize(Em *em);
void GmEm_ResetState(Em *em);
void GmLoad_LoadStagePartition(void);
s32 Quest_IsRunning(s32 ending);
s32 QuestEm_PlaceAtRandomPoint(Em *em);
void Em_RunAppearHook(Em *em);
void Quest_NoteTargetDown(Em *em);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_Copy(f32 *dst, f32 *src);

s8 Versus_Team(s8 side);
s32 Quest_HasDrillFlag(void);
void Quest_CountEventKill(u16 type);
void Quest_CountTarget(u16 type);
void QuestStats_Add(s32 a, s32 b);
void Quest_JumpToEnd(s32 a);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
EmBone *Em_Bone(void *obj, s16 bone);
s32 MonsterNote_Index(u16 id);
s32 Em_KindMatches(u8 type, u8 kind);
s32 Em_PartBroken(Em *em, u16 part); /* unit's own declaration: the definition (game) is s32 Em_PartBroken(Em *em, s32 arg1); the calls here match only with this one */
void Em_PartBreakEffect(Em *em, s32 part);
s32 Quest_IsVersus(void);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
void QSync_SendState(u8 kind, u8 who);
s16 Pouch_Count(Player *p, u16 item);
void Quest_TargetMessage(void);
char *Quest_SelectedText(s32 k);
s16 Quest_CountDelivered(u16 item, s32 who);
void Quest_DeliveryMessage(u16 item);
s32 QuestGoal_CopyFlags(u16 i, s32 mask);
s32 QuestGoal_DeliverCount(u16 i);
u16 Quest_Deliver(Player *pl, u16 item, s32 count);
void Jingle_Play(void *obj, s32 a);
static s16 Quest_FindReward(u16 id);
u16 Quest_AddReward(u16 item, s16 n);
extern s8 gRewardGiveRolls[];
void Effect17_SpawnArea(s32 area);
void QuestEm_SpawnYamaTsukamiB(void);
void GmLoad_LoadEmData(s32 kind, s32 idx);
void EmSlot_LoadModels(s32 i);
Em *EmSlot_Alloc(void);
s32 GmEm_Task(Em *em);
void Em_UpdateMatrix(Em *em);
extern u8 gEmIsLarge[]; /* the game overlay's gEmIsLarge: 1 for the large monsters, by kind */
extern Em gEms[];
extern char *gReturn20sTownText[];
extern char *gReturn20sVillageText;
extern char *gReturn1mTownText[];
extern char *gReturn1mVillageText;
extern char *gQuestClearText[];
extern char *gReturn10sTownText[];
extern char *gReturn10sVillageText;
void MinePoint_UpdateAll(void);
void QuestGoal_Update(void);
void Quest_UpdateCountdown(void);
s16 Quest_StepScript(Player *pl, s16 a, s32 *out);
s32 Quest_HasAdvice(void);
void Quest_CheckCampAdvice(void);
void Quest_UpdateTimers(s32 a);
s32 QuestGoal_IsDone(s32 a);
s32 Quest_IsDrill(void);
s32 Quest_IsTraining(void);
u16 Save_RankFromPoints(SaveData *s);
s32 Side_Match(u8 a, u8 b, u8 c);
void Quest_UpdateTimedEvent(void);
void Quest_CheckItemTargets(void);
extern u16 gNextQuestId;
extern NextQuest gNextQuest;
extern u8 gNetQuestSeeds[15];
extern RangeU8 gEmSizeRanges[5];
extern RangeU8 gEmSizeRangeDefaults[5];
extern s8 gTraderVariantRolls[16];
extern s16 *gStageFileTbls[];
extern s16 D_1BB3D0[];
void *memcpy(void *dst, const void *src, u32 n);
void MinePoint_ResetAll(void);
void QuestGoal_ClearAll(void);
void Bgm_Init(void);
void QuestStats_Init(void);
void MsgEvent_Init(void);
s32 Season_Get(void);
void Daytime_Set(s32 a);
u8 Daytime_Get(void);
void Quest_SetSeason(u8 a);
s32 Mib_Read(s32 flid, void *buf);
void Quest_SetCurrent(u16 stage, void *b, s32 a);
void Versus_Init(void);
void QuestGoal_InitFromDef(void);
void QuestEm_InitAll(void);
void SpecialMine_Clear(void);
s32 MsgEvent_Check(u16 kind);
void GmLoad_Reset(void);
u16 Quest_Random(void);
void Quest_SeedRandom(void);
void Quest_ClearTargets(void);
void Quest_LoadTargets(void);
void Quest_SetStartArea(u16 who);
s32 Quest_GetTime(s32 which);
void EmMelynx_DeductReward(Em *em);
void EmSlot_Free(Em *em);
void QuestEm_ResetArea(s32 a);
void EmSlot_FreeModels(s16 i);
void QuestEm_Hunted(Em *em);

/*
 * Clears gQuest for no quest: the default stage header's tables, the script
 * stopped. Only the versus defeat bits are kept.
 */
void Quest_Clear(void)
{
    u8 keep = gQuest.versusDefeat;

    memset(&gQuest, 0, sizeof(QuestState));
    gQuest.versusDefeat = keep;
    gQuest.header = &gDefaultStageHeader;
    gQuest.loopPc = -1;
    gQuest.scriptPc = -1;
    gQuest.script = (void *)gDefaultStageHeader.script;
    gQuest.noticeTexts = (void *)gDefaultStageHeader.noticeTexts;
    gQuest.texts = (void *)gDefaultStageHeader.noticeTexts;
    gQuest.emSpawns = (void *)gDefaultStageHeader.emSpawns;
    gQuest.stageEntry = (void *)gDefaultStageHeader.stageEntry;
    gQuest.areaData = (void *)gDefaultStageHeader.areaData;
    gQuest.areaRecords = (void *)gDefaultStageHeader.areaRecords;
    gQuest.rewardTable = (void *)gDefaultStageHeader.rewardTable;
    gQuest.gatherPoints = (void *)gDefaultStageHeader.gatherPoints;
    gQuest.actionSpots = (void *)gDefaultStageHeader.actionSpots;
    gQuest.info = (QuestInfo *)gDefaultStageHeader.info;
    gQuest.dataTable = (void *)gDefaultStageHeader.dataTable;
    gQuest.seasonalData = (void *)gDefaultStageHeader.seasonalData;
    QuestRecord_Clear();
}

/*
 * Sets up the quest gNextQuestId (gNextQuest): clears the quest's records,
 * seeds its random stream (online, from the seeds the host sent), loads the
 * stage file of the season and time of day (for a quest id below 50000; from
 * 50000 the quest was downloaded and its file is already loaded), points
 * gQuest at the file's tables and sets up the goals, the targets, the
 * monsters and the mining points. With no quest (stage 0) only the area is
 * set.
 */
void Quest_Init(void)
{
    s16 *tbl = D_1BB3D0;
    s32 i;
    u8 a;
    u8 b;
    StageHeader *h;
    u32 *p;
    s32 file;
    u16 r;

    gScene.questId = gNextQuestId;
    gQuest.stage = gNextQuestId;
    gQuest.flyer = gNextQuest.flyer;
    memcpy(gQuest.huntGoals, gNextQuest.huntGoals, sizeof(gQuest.huntGoals));
    QuestRecord_Clear();
    gScene.rewardDeducted = 0;
    gScene.questPhase = QUEST_PHASE_READY;
    MinePoint_ResetAll();
    QuestGoal_ClearAll();
    Bgm_Init();
    Quest_ClearTargets();
    if (Game_IsOnline() == 0) {
        for (i = 0; i < (s32)sizeof(gQuest.seed); i++) {
            r = Rand_Next(0);
            gQuest.seed[i] = r;
        }
        gEmSizeRanges[0].min = gEmSizeRangeDefaults[0].min;
        gEmSizeRanges[0].max = gEmSizeRangeDefaults[0].max;
        gEmSizeRanges[1].min = gEmSizeRangeDefaults[1].min;
        gEmSizeRanges[1].max = gEmSizeRangeDefaults[1].max;
        gEmSizeRanges[2].min = gEmSizeRangeDefaults[2].min;
        gEmSizeRanges[2].max = gEmSizeRangeDefaults[2].max;
        gEmSizeRanges[3].min = gEmSizeRangeDefaults[3].min;
        gEmSizeRanges[3].max = gEmSizeRangeDefaults[3].max;
        gEmSizeRanges[4].min = gEmSizeRangeDefaults[4].min;
        gEmSizeRanges[4].max = gEmSizeRangeDefaults[4].max;
    } else {
        for (i = 0; i < (s32)sizeof(gQuest.seed); i++) {
            gQuest.seed[i] = gNetQuestSeeds[i];
        }
    }
    Quest_SeedRandom();
    Quest_Random();
    /* only the first 74 kinds; sizeMin, captured and parts stay */
    for (i = 0; i < 74; i++) {
        gQuest.sizeMax[i] = 0;
        gQuest.hunted[i] = 0;
    }
    gQuest.capturedType = 0;
    gQuest.capturedSize = 0;
    gQuest.capturedLevel = 0;
    QuestStats_Init();
    for (i = 0; i < 4; i++) {
        gQuest.seed[i] = Quest_Random();
    }
    gQuest.acks = 0;
    gQuest.netMsgArgs = 0;
    gQuest.eventTriggered = 0;
    gQuest.eventLabel = 0;
    gQuest.goalLabel = 0;
    for (i = 0; i < 4; i++) {
        gQuest.hunterRank[i][0] = 0;
        gQuest.hunterRank[i][1] = 0;
    }
    gQuest.emLeft = 0;
    gQuest.savedEmParts = 0;
    gQuest.savedEmHp = 0;
    gQuest.anyFainted = 0;
    gQuest.hunterRank[gScene.localPlayer][1] = Save_RankFromPoints(&gSave);
    gQuest.unk096 = 1;
    for (i = 0; i < 4; i++) {
        gScene.emKind[i] = 0;
    }
    for (i = 0; i < 5; i++) {
        gQuest.rewards[i].id = 0;
        gQuest.rewards[i].val = 0;
    }
    gQuest.rewardNext = 0;
    gQuest.rewardsPending = 0;
    gQuest.unk003 = 0;
    gQuest.state = 0;
    gQuest.step = 0;
    gQuest.progress = 0;
    gQuest.downEm = NULL;
    gQuest.timeLeft = 0;
    gQuest.savedEmType = 0;
    gQuest.unk15B = 0;
    gQuest.traderWantItem = 0;
    MsgEvent_Init();
    if (gQuest.stage != 0) {
        if (gQuest.stage < QUEST_ID_DOWNLOADED) {
            /* the group of stage ids the quest falls in (each bound is the
             * group's last id), then its file in the group's table */
            for (i = 0; *tbl != 0; i++) {
                if (*tbl >= gQuest.stage) {
                    break;
                }
                tbl++;
            }
            tbl = gStageFileTbls[i];
            tbl += gQuest.stage % 100;
            file = *tbl;
            if (Quest_IsVersus() == 0) {
                if (gNextQuest.flags & QUEST_FLAGS_FIXED_SEASON) {
                    if (gNextQuest.flags & QUEST_FLAG_SET_BREEDING) {
                        a = SEASON_BREEDING;
                    } else if (gNextQuest.flags & QUEST_FLAG_SET_WARM) {
                        a = SEASON_WARM;
                    } else {
                        a = SEASON_COLD;
                    }
                } else {
                    a = Season_Get();
                }
                /* these maps are always played by day */
                switch (gNextQuest.map) {
                case 7:
                case 8:
                case 13:
                case 14:
                case 15:
                    b = DAYTIME_DAY * 3;
                    Daytime_Set(DAYTIME_SET_DAY);
                    break;
                default:
                    b = Daytime_Get() * 3;
                    break;
                }
                file += b + a;
            } else {
                /* versus: the warm night's file */
                a = SEASON_WARM;
                file += DAYTIME_NIGHT * 3;
            }
            switch (a) {
            case SEASON_WARM:
                Quest_SetSeason(SEASON_WARM);
                break;
            case SEASON_COLD:
                Quest_SetSeason(SEASON_COLD);
                break;
            case SEASON_BREEDING:
            default:
                Quest_SetSeason(SEASON_BREEDING);
                break;
            }
            Mib_Read(file, (void *)gStageFile);
        } else {
            gQuest.info = (QuestInfo *)(((StageHeader *)gStageFile)->info + gStageFile);
            if (gQuest.info->flags & QUEST_FLAG_BREEDING) {
                a = SEASON_BREEDING;
            } else if (gQuest.info->flags & QUEST_FLAG_COLD) {
                a = SEASON_COLD;
            } else {
                a = SEASON_WARM;
            }
            Quest_SetSeason(a);
            Daytime_Set((gQuest.info->flags & QUEST_FLAG_DAY) ? DAYTIME_SET_DAY : DAYTIME_SET_NIGHT);
        }
        h = (StageHeader *)gStageFile;
        Quest_SetCurrent(gQuest.stage, h, 0);
        gQuest.info = (QuestInfo *)(h->info + gStageFile);
        gQuest.header = h;
        if (Quest_IsVersus() == 1) {
            Versus_Init();
        } else {
            gQuest.nextSide = 0;
            gQuest.inputLock = 60; /* frames: 2 s */
        }
        Quest_SetStartArea(START_AREA_SCENE);
        gQuest.timeLeft = Quest_GetTime(QUEST_TIME_LIMIT);
        gQuest.timeNoticeIdx = (gQuest.timeLeft - 1) / QUEST_TIME_NOTICE_STEP;
        if (gQuest.timeNoticeIdx > 2) {
            gQuest.timeNoticeIdx = 2;
        }
        gQuest.script = (void *)(h->script + gStageFile);
        gQuest.loopPc = 0;
        gQuest.scriptPc = 0;
        gQuest.areaData = (void *)(h->areaData + gStageFile);
        gQuest.noticeTexts = (void *)(h->noticeTexts + gStageFile);
        gQuest.texts = (void *)(gQuest.info->texts + gStageFile);
        gQuest.emSpawns = (void *)(h->emSpawns + gStageFile);
        gQuest.stageEntry = (void *)(h->stageEntry + gStageFile);
        gQuest.areaRecords = (void *)(h->areaRecords + gStageFile);
        gQuest.gatherPoints = (void *)(h->gatherPoints + gStageFile);
        gQuest.actionSpots = (void *)(h->actionSpots + gStageFile);
        gQuest.dataTable = (void *)(h->dataTable + gStageFile);
        gQuest.seasonalData = (void *)(h->seasonalData + gStageFile);
        gQuest.reward = gQuest.info->reward;
        gQuest.penalty = gQuest.info->penalty;
        gQuest.rewardTable = (void *)(h->rewardTable + gStageFile);
        gQuest.flags = gQuest.info->flags;
        gQuest.emLevel = h->emLevel;
        gQuest.header->tables += gStageFile;
        p = (u32 *)(h->stageInfo + gStageFile);
        gScene.map = p[0];
        gScene.campArea = p[1];
        gQuest.timedEvent = h->timedEvent;
        gQuest.eventKind = h->eventKind;
        gQuest.eventCount = h->eventCount;
        gQuest.timedEventSet = gQuest.timeLeft / QUEST_TIME_NOTICE_STEP;
        gQuest.timedEventStep = 0;
        gQuest.traderVariant = gTraderVariantRolls[Quest_Random() & 0xF];
        if (Quest_IsDrill() == 1) {
            gQuest.traderVariant = 0;
        }
        QuestGoal_InitFromDef();
        Quest_LoadTargets();
        QuestEm_InitAll();
        SpecialMine_Clear();
        MsgEvent_Check(0);
    } else {
        gScene.area = gQuestSync.area;
        gScene.startArea = gQuestSync.area;
        gQuest.header = &gDefaultStageHeader;
        Quest_SetSeason(SEASON_WARM);
        gScene.map = 2;
    }
    GmLoad_Reset();
}

/* Abandons the quest unless it is running or already ending (online, also
 * leaves the network quest). */
void Quest_Abort(void)
{
    if (Quest_IsRunning(1) == 0) {
        gScene.questPhase = QUEST_PHASE_ABORTED;
        gQuest.scriptPc = -1;
        gQuest.result = QUEST_PHASE_ABORTED;
        Quest_StoreElapsed();
        if (Game_IsOnline() != 0) {
            QNetIf_Leave();
        }
    }
}

/*
 * Starts a quest that is set up: the script goes on from its first start
 * mark (QUEST_OP_START or QUEST_OP_START_ALT) and the quest is RUNNING.
 */
void Quest_Start(void)
{
    s16 *p = gQuest.script;
    s16 i = 0;

    switch (gScene.questPhase) {
    case 0:
    case 1:
    case QUEST_PHASE_READY:
        for (;;) {
            if (*p == QUEST_OP_START || *p == QUEST_OP_START_ALT) {
                break;
            }
            i++;
            p += sizeof(QuestCmd) / sizeof(s16);
        }
        gQuest.scriptPc = i;
        gScene.questPhase = QUEST_PHASE_RUNNING;
        break;
    }
}

/* Ends the quest at once for the results (QUEST_PHASE_QUIT) and stops the script. */
void Quest_Finish(void)
{
    gScene.questPhase = QUEST_PHASE_QUIT;
    gQuest.result = QUEST_PHASE_QUIT;
    gQuest.scriptPc = -1;
}

/* Quest_Finish, unless the quest is running or already ending. */
void Quest_FinishIfIdle(void)
{
    if (Quest_IsRunning(1) == 0) {
        Quest_Finish();
    }
}

/*
 * Sets the start area of player `who`, or with START_AREA_SCENE the scene's
 * area (the local player's). Without a quest it is gQuestSync.area; with
 * the stage's randomStart a weighted roll on the player's seed byte;
 * otherwise the stage file's start area of the player.
 */
void Quest_SetStartArea(u16 who)
{
    u16 **tbl = gStartAreaTables[gQuest.info->map];
    u16 *p;
    u16 *w;
    s32 sum;
    s32 r;
    StartArea *s;
    u16 area;

    if (gQuest.stage == 0) {
        if (who == START_AREA_SCENE) {
            gScene.area = gQuestSync.area;
            gScene.startArea = gQuestSync.area;
        } else {
            gPlayers[who].area = gScene.area;
        }
        return;
    }
    if (gQuest.header->randomStart != 0) {
        /* (weight, area) pairs, 0xFFFF-terminated */
        w = tbl[gQuest.header->randomStart - 1];
        sum = 0;
        for (p = w; *p != 0xFFFF; p += 2) {
            sum += *p;
        }
        if (who == START_AREA_SCENE) {
            r = gQuest.seed[gScene.localPlayer] % sum;
        } else {
            r = gQuest.seed[who] % sum;
        }
        sum = 0;
        while (*w != 0xFFFF) {
            if (r < (sum += *w++)) {
                break;
            }
            w++;
        }
        if (who == START_AREA_SCENE) {
            area = *w;
            gScene.area = area;
            gScene.startArea = area;
            return;
        }
        gPlayers[who].area = *w;
        return;
    }
    s = (StartArea *)(((StageHeader *)gStageFile)->startAreas + gStageFile);
    if (who == START_AREA_SCENE) {
        s += gScene.localPlayer;
        area = s->area;
        gScene.area = area;
        gScene.startArea = area;
        return;
    }
    gPlayers[who].area = s[who].area;
}

/* Restarts the BGM state machine, unless the quest is running or ending. */
void Quest_ResetMusicIfIdle(void)
{
    if (Quest_IsRunning(1) == 0) {
        gScene.musicState = 0;
    }
}

/*
 * 1 while the quest runs (RUNNING or CLEARED); -1 when it is ending (ENDED,
 * FAILED or ABORTED) and `ending` is nonzero; otherwise 0.
 */
s32 Quest_IsRunning(s32 ending)
{
    if (gScene.questPhase == QUEST_PHASE_RUNNING || gScene.questPhase == QUEST_PHASE_CLEARED) {
        return 1;
    }
    /* ENDED or FAILED */
    if ((u32)(gScene.questPhase - QUEST_PHASE_ENDED) <= 1 || gScene.questPhase == QUEST_PHASE_ABORTED) {
        if (ending != 0) {
            return -1;
        }
    }
    return 0;
}

/* The time left (QUEST_TIME_LEFT, never below 0) or the time limit
 * (QUEST_TIME_LIMIT), in frames. */
s32 Quest_GetTime(s32 which)
{
    s32 t;

    switch (which) {
    case QUEST_TIME_LEFT:
        t = gQuest.timeLeft;
        return (t < 0) ? 0 : t;
    case QUEST_TIME_LIMIT:
        return gQuest.info->timeLimit;
    default:
        t = gQuest.timeLeft;
        if (t < 0) {
            return 0;
        }
        return t;
    }
}

/* On entering an area: loads the stage's part and spawns the quest's
 * monsters and those of the area. */
void Quest_EnterArea(s32 a)
{
    GmLoad_LoadStagePartition();
    QuestEm_SpawnAll();
    QuestEm_SpawnArea(a);
}

/* Moves em to a random spawn point of its area (StageHeader.areaSpawns):
 * 0, or -1 when the area has none. */
s32 QuestEm_PlaceAtRandomPoint(Em *em)
{
    AreaSpawns *p = (AreaSpawns *)(gQuest.header->areaSpawns + gStageFile);
    SpawnPoint *sp;

    for (;;) {
        if (em->area == Area_IdForCurrentTime(p->area)) {
            break;
        }
        p++;
        if (p->area == 0xFFFF) {
            return -1;
        }
    }
    sp = (SpawnPoint *)(p->offset + gStageFile);
    sp += Rand_Next(0) % p->count;
    em->rotY = sp->rotY;
    em->posX = sp->x;
    em->posY = sp->y;
    em->posZ = sp->z;
    return 0;
}

/* 1 when em's quest monster may appear again: the quest not running, the
 * entry not done and some appearances left. */
s32 QuestEm_CanAppear(Em *em)
{
    QuestEm *q;

    if (Quest_IsRunning(0) != 0) {
        return 0;
    }
    q = QuestEm_FindForEm(em);
    if (q != NULL) {
        if (q->flags & QUEST_EM_DONE) {
            return 0;
        }
        if (q->noReappear == 1) {
            return 0;
        }
        if (q->count > 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * Brings em's quest monster back: a monster with a random size grows by 5
 * percent (up to the kind's gEmAppearMax) and, unless the quest says not,
 * gains a level, as does the quest's level (up to 15). It takes its place
 * (a random point for spawn 1, else the entry's), its parts and state are
 * reset and its appear handler runs. Returns its appearances left, 0 when
 * none.
 */
s8 QuestEm_Appear(Em *em)
{
    QuestEm *q;
    u16 max;

    if ((q = QuestEm_FindForEm(em)) != NULL) {
        if (q->flags & QUEST_EM_DONE) {
            return 0;
        }
        q->flags = QUEST_EM_ACTIVE;
        if (Em_HasRandomSize2(em->type) == 1) {
            em->sizePct += 5;
            max = gEmAppearMax[MonsterGroup_First(em->type)];
            if (em->sizePct > max) {
                em->sizePct = max;
            }
            if (!(gQuest.flags & QUEST_FLAG_NO_LEVEL_UP)) {
                if (gQuest.emLevel < 15) {
                    gQuest.emLevel++;
                }
                if (gQuest.emLevel > 15) {
                    gQuest.emLevel = 15;
                }
                if (em->level < 15) {
                    em->level++;
                }
                if (em->level > 15) {
                    em->level = 15;
                }
            }
        }
        q->hp = em->hp;
        switch (q->spawn) {
        case 1:
            if (QuestEm_PlaceAtRandomPoint(em) == 0) {
                break;
            }
        case 0:
        default:
            em->posX = q->x;
            em->posY = q->y;
            em->posZ = q->z;
            em->rotY = q->rotY;
            break;
        }
        em->area = q->area;
        Em_InitParts(em);
        EmMine_RemoveTimed(em);
        GmEm_ApplySize(em);
        GmEm_ResetState(em);
        Em_RunAppearHook(em);
        return q->count;
    }
    return 0;
}

/* Runs em's appear handler and clears its pending script. */
void Em_RunAppearHook(Em *em)
{
    em->handlers->appear(em);
    em->scriptPending = 0;
}

/* Notes em as the target that went down last when its kind is a clear
 * target; once the quest is cleared, the camera turns to it. */
void Quest_NoteTargetDown(Em *em)
{
    if (QuestGoal_IsClearTarget(em->type) == 1) {
        gQuest.downEm = em;
        if (Quest_IsRunning(0) != 0 && gQuest.result == QUEST_PHASE_CLEARED && gSysSettings.clearCapture == 0) {
            CamReq_TargetDown(gQuest.result);
        }
    }
}

/*
 * em was hunted: counts it (the quest's targets, events and records, the
 * largest and smallest sizes, outside versus), uses up one appearance of its
 * entry, tells the other players when the kill is ours, and counts the
 * goal.
 */
void QuestEm_Hunted(Em *em)
{
    QuestEm *q;

    if ((q = QuestEm_FindForEm(em)) != NULL) {
        if (q->flags & (QUEST_EM_DONE | QUEST_EM_HUNTED)) {
            Quest_NoteTargetDown(em);
            return;
        }
        if (q->large != 0) {
            gQuest.emLeft--;
        }
        q->count--;
        Quest_CountEventKill(em->type);
        Quest_CountTarget(em->type);
        if (Quest_IsVersus() == 0) {
            QuestEm_CountHunted(em->type);
            gQuest.hunted[em->type]++;
            if (gQuest.hunted[em->type] > QUEST_COUNT_MAX) {
                gQuest.hunted[em->type] = QUEST_COUNT_MAX;
            }
            if (Em_HasRandomSize(em->type) == 1) {
                if (em->sizePct > gQuest.sizeMax[em->type]) {
                    gQuest.sizeMax[em->type] = em->sizePct;
                }
                if (gQuest.sizeMin[em->type] == 0) {
                    gQuest.sizeMin[em->type] = em->sizePct;
                } else if (gQuest.sizeMin[em->type] > em->sizePct) {
                    gQuest.sizeMin[em->type] = em->sizePct;
                }
            }
        }
        if (q->count > 0) {
            q->flags = QUEST_EM_HUNTED;
        } else {
            q->emIndex = -1;
            q->flags = QUEST_EM_DONE;
        }
        if (em->remote == 0) {
            Quest_SendKill(q, (Player *)em);
        }
        if (Quest_IsVersus() == 1) {
            QuestGoal_OnKillSlot(em->type, (s8)Versus_Team(em->side));
            return;
        }
        QuestGoal_OnKill(em->type);
        Quest_NoteTargetDown(em);
    }
}

/* em leaves the area: its entry keeps its health for the next appearance. */
void QuestEm_Leave(Em *em)
{
    QuestEm *q = QuestEm_FindForEm(em);

    if (q != NULL && !(q->flags & QUEST_EM_DONE)) {
        q->hp = em->hp;
        q->emIndex = -1;
    }
}

/*
 * em was captured: as QuestEm_Hunted (once per entry), and the kind, size
 * and level (level - 3, 1 to 12) of the capture are kept for the records.
 */
void QuestEm_Captured(Em *em)
{
    QuestEm *q;

    if ((q = QuestEm_FindForEm(em)) != NULL) {
        if (q->flags & QUEST_EM_CAPTURED) {
            Quest_NoteTargetDown(em);
            return;
        }
        gSysSettings.clearCapture = 0x81;
        if (Quest_IsVersus() == 0) {
            QuestEm_CountCaptured(em->type);
            gQuest.captured[em->type]++;
            if (gQuest.captured[em->type] > QUEST_COUNT_MAX) {
                gQuest.captured[em->type] = QUEST_COUNT_MAX;
            }
            if (Em_HasRandomSize(em->type) == 1) {
                if (em->sizePct > gQuest.sizeMax[em->type]) {
                    gQuest.sizeMax[em->type] = em->sizePct;
                }
                if (gQuest.sizeMin[em->type] == 0) {
                    gQuest.sizeMin[em->type] = em->sizePct;
                } else if (gQuest.sizeMin[em->type] > em->sizePct) {
                    gQuest.sizeMin[em->type] = em->sizePct;
                }
            }
        }
        if (MonsterNote_Index(em->type) != 0xFF) {
            gQuest.capturedType = em->type;
            gQuest.capturedSize = em->sizePct;
            gQuest.capturedLevel = em->level - 3;
            if (gQuest.capturedLevel > 12) {
                gQuest.capturedLevel = 12;
            }
            if (gQuest.capturedLevel <= 0) {
                gQuest.capturedLevel = 1;
            }
        }
        QuestGoal_OnCapture(em->type);
        Quest_NoteTargetDown(em);
        if (!(q->flags & (QUEST_EM_DONE | QUEST_EM_HUNTED))) {
            func_003AB680(q, em);
            if (q->large != 0) {
                gQuest.emLeft--;
            }
            q->count--;
            if (q->count > 0) {
                q->flags = QUEST_EM_HUNTED;
            } else {
                q->emIndex = -1;
                q->flags = QUEST_EM_DONE;
            }
        }
        q->flags |= QUEST_EM_CAPTURED;
    }
}

/* Shows the part break effect at the bone of em's part, the first time it breaks
 * (gPartBreakEffects lists the kinds' parts). */
void Em_PartBreakEffect(Em *em, s32 part)
{
    f32 pos[3];
    f32 vec[4];
    f32 mtx[4][4];
    PartEffect *p = gPartBreakEffects;
    u8 kind = gPartBreakEffects[0].kind;

    if (kind != 0xFF) {
        do {
            if (Em_KindMatches(em->type, kind) != 0 && p->part == part && Em_PartBroken(em, part) == 0) {
                Mtx_Copy((f32 *) mtx, (f32 *) (Em_Bone(em, p->bone)));
                Vec_Copy(vec, p->offset);
                Vec_RotateSelf((f32 *) vec, (f32 *) mtx);
                pos[0] = mtx[3][0] + vec[0];
                pos[1] = mtx[3][1] + vec[1];
                pos[2] = mtx[3][2] + vec[2];
                Effect02_SpawnForOwner(em, 0, 10, 0, pos, 1.0f);
                return;
            }
            p++;
        } while ((kind = p->kind) != 0xFF);
    }
}

/* Breaks part `part` of em (with its effect the first time) and counts it
 * for the quest, its goals and the drill statistics. */
void Em_BreakPart(Em *em, s32 part)
{
    s32 bit = 1 << part;

    if (!(em->partsBroken & bit)) {
        Em_PartBreakEffect(em, part);
    }
    em->partsBroken |= (u16)bit;
    gQuest.parts[em->type] |= (u16)bit;
    QuestGoal_OnPartBroken(em->type, part);
    QuestStats_Add(QSTAT_PART_BROKEN, 1);
}

/* Breaks the parts in `bits` at once (Em_BreakPart for a set; the statistic
 * counts once when any of them is new to the quest). */
void Em_BreakParts(Em *em, u16 bits)
{
    s32 i;

    if (bits != 0 && (bits & gQuest.parts[em->type]) != bits) {
        QuestStats_Add(QSTAT_PART_BROKEN, 1);
    }
    for (i = 0; i < 16; i++) {
        if ((bits & (1 << i)) && !(em->partsBroken & (1 << i))) {
            Em_PartBreakEffect(em, i);
        }
    }
    em->partsBroken |= bits;
    gQuest.parts[em->type] |= em->partsBroken;
    QuestGoal_OnPartBroken(em->type, bits);
}

/* 1 when part `part` of monster kind `type` was broken during the quest. */
s32 Quest_IsPartBroken(u16 type, u16 part)
{
    return (gQuest.parts[type] & (1 << part)) != 0;
}

void Em_ClearPart(Em *em, u16 part)
{
    em->partsBroken &= (u16)~(u16)(1 << part);
}

/*
 * A player fainted: the reward loses the penalty, unless the faint is
 * free (`noPenalty`, or the local player's drill charm takes it). Returns
 * FAINT_* (quest.h).
 */
s32 Quest_PlayerFainted(Player *pl, s32 noPenalty)
{
    if (gQuest.reward <= 0) {
        return FAINT_QUEST_OVER;
    }
    QuestStats_Add(QSTAT_FAINT, 1);
    if (Player_IsLocal(pl) == 1 && (gQuest.flags & QUEST_FLAG_DRILL) && gQuest.drillCharm == DRILL_CHARM_TAKEN) {
        gQuest.drillCharm = DRILL_CHARM_USED;
        return FAINT_FREE;
    }
    if (noPenalty == 0) {
        gQuest.anyFainted = 1;
        if ((gQuest.reward -= gQuest.penalty) <= 0) {
            Quest_JumpToEnd(0);
            if (gQuest.reward <= 0) {
                gQuest.reward = 0;
            }
            if (Quest_HasDrillFlag() == 1 || Quest_IsVersus() == 1) {
                return FAINT_QUEST_OVER;
            }
            return FAINT_FAILED;
        }
    } else {
        return FAINT_FREE;
    }
    if (Quest_HasDrillFlag() == 1 || Quest_IsVersus() == 1) {
        return FAINT_QUEST_OVER;
    }
    return FAINT_PENALTY;
}

/* In a drill, takes the charm the first time (with its notice): 1 when taken. */
s32 Quest_DrillTakeCharm(void)
{
    if ((gQuest.flags & QUEST_FLAG_DRILL) && gQuest.drillCharm == DRILL_CHARM_NONE) {
        gQuest.drillCharm = DRILL_CHARM_TAKEN;
        Notice_PushItem(gDrillScoreTexts[QSTAT_CHARM]);
        return 1;
    }
    return 0;
}

/* 1 when a drill's charm is taken and not yet used up. */
s32 Quest_DrillHasCharm(void)
{
    if ((gQuest.flags & QUEST_FLAG_DRILL) && gQuest.drillCharm == DRILL_CHARM_TAKEN) {
        return 1;
    }
    return 0;
}

/*
 * The quest's frame (gQuest.state): while it plays, the goals, the clear
 * check, the countdown, a step of the script, the timed event and the
 * item targets; once cleared, the return countdown with its notice; then
 * the reward: the reward split between the players and two rounds of
 * acknowledgements between them; in versus, the wait before won or lost.
 * Returns 1 once the quest is cleared or the script stopped, -1 when it
 * failed, -2 when abandoned or quit, otherwise 0.
 */
s32 Quest_Update(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    s32 i;
    s32 t;
    u8 idx;
    s32 tmp;

    for (i = 0; i < 4; i++) {
        if (gScene.inQuest[i] != 1) {
            gQuest.acks |= 1 << (i + QUEST_ACK_STARTED_SHIFT);
        }
    }
    if (gQuest.inputLock > 0) {
        gQuest.inputLock--;
    }
    MinePoint_UpdateAll();
    if (gQuest.stage == 0) {
        return 0;
    }
    if (gQuest.result == QUEST_PHASE_ABORTED) {
        return -2;
    }
    if (gQuest.result == QUEST_PHASE_QUIT) {
        return -2;
    }
    switch (gQuest.state) {
    case QSTATE_PLAY:
        if (gQuest.result == QUEST_PHASE_CLEARED) {
            return 1;
        }
        if (gQuest.result == QUEST_PHASE_FAILED) {
            return -1;
        }
        if (gQuest.scriptPc == -1) {
            return 1;
        }
        if (Quest_IsVersus() == 1) {
            for (i = 0; i < gScene.playerCount; i++) {
                if (gScene.inQuest[i] != 1 && gPlayers[i].side < 3) {
                    Quest_FinishIfIdle();
                }
            }
        }
        QuestGoal_Update();
        Quest_CheckClear(1);
        Quest_UpdateCountdown();
        gQuest.scriptPc = Quest_StepScript(pl, gQuest.scriptPc, &tmp);
        Quest_UpdateTimedEvent();
        Quest_CheckItemTargets();
        if (Quest_HasAdvice() == 1) {
            Quest_CheckCampAdvice();
        }
        return 0;
    case QSTATE_CLEAR:
        switch (gQuest.step) {
        case 0:
            gQuest.step++;
            t = Quest_GetTime(QUEST_TIME_LIMIT);
            gQuest.elapsed = t - Quest_GetTime(QUEST_TIME_LEFT);
            gQuest.msgArg0 = 0;
            gQuest.msgType = QMSG_CLEAR_TIME;
            gQuest.msgArg2 = gQuest.elapsed;
            gQuest.msgArg1 = gQuest.elapsed >> 16;
            QSync_SendState(6, gScene.localPlayer);
            gQuest.timeLeft = 150; /* frames: 5 s */
            break;
        case 1:
            Quest_UpdateTimers(1);
            if (gQuest.timeLeft > 0) {
                break;
            }
            gQuest.step++;
            if (QuestGoal_IsDone(0) == 1) {
                /* the return time after the main goal: 20 s for these
                 * quests, else a minute; 10 s after a sub goal */
                if (gQuest.flags & 0x20000006) {
                    gQuest.timeLeft = 600; /* frames: 20 s */
                    if (Game_IsOnline() == 1) {
                        Notice_PushItem(gReturn20sTownText[0]);
                    } else {
                        Notice_PushItem(gReturn20sVillageText);
                    }
                } else {
                    gQuest.timeLeft = 1800; /* frames: 1 min */
                    if (Game_IsOnline() == 1) {
                        Notice_PushItem(gReturn1mTownText[0]);
                    } else {
                        Notice_PushItem(gReturn1mVillageText);
                    }
                }
            } else {
                gQuest.timeLeft = 300; /* frames: 10 s */
                if (Game_IsOnline() == 1) {
                    Notice_PushItem(gReturn10sTownText[0]);
                } else {
                    Notice_PushItem(gReturn10sVillageText);
                }
            }
            break;
        case 2:
            Quest_UpdateTimers(1);
            if (gQuest.timeLeft > 0) {
                break;
            }
            gQuest.step = 0;
            gQuest.state++;
            Notice_PushItem(gQuestClearText[0]);
            if (Quest_IsVersus() != 0) {
                break;
            }
            if (Quest_IsDrill() != 0) {
                break;
            }
            if (Quest_IsTraining() != 0) {
                break;
            }
            Notice_Push(0, 15, 0);
            break;
        }
        break;
    case QSTATE_REWARD:
        switch (gQuest.step) {
        case 0:
            gQuest.step++;
            gQuest.result = QUEST_PHASE_CLEARED;
            t = gQuest.reward /= gScene.playerCount;
            idx = gScene.localPlayer;
            gQuest.hunterRank[idx][0] = Save_RankFromPoints(&gSave);
            gQuest.msgArg0 = 0;
            gQuest.msgType = QMSG_RANK;
            gQuest.msgArg1 = gQuest.hunterRank[idx][0];
            gQuest.msgArg2 = 0;
            QSync_SendState(6, idx);
            gQuest.acks |= 1 << gScene.localPlayer;
            gQuest.ackTimer = 5400; /* frames: 3 min */
            break;
        case 1:
            if (gQuest.ackTimer > 0) {
                gQuest.ackTimer--;
            }
            for (i = 0; i < 4; i++) {
                if (gScene.inQuest[i] != 1) {
                    gQuest.acks |= 1 << i;
                }
            }
            if ((gQuest.acks & QUEST_ACKS_RANK) != QUEST_ACKS_RANK&& gQuest.ackTimer > 0 && gScene.inQuest[gScene.localPlayer] == 1) {
                break;
            }
            gQuest.msgArg0 = 0;
            gQuest.msgType = QMSG_REWARD_ACK;
            gQuest.step++;
            gQuest.msgArg2 = gQuest.netMsgArgs;
            gQuest.msgArg1 = gQuest.netMsgArgs >> 16;
            QSync_SendState(6, gScene.localPlayer);
            gQuest.acks |= 1 << (gScene.localPlayer + QUEST_ACK_REWARD_SHIFT);
            gQuest.ackTimer = 1800; /* frames: 1 min */
            break;
        case 2:
            if (gQuest.ackTimer > 0) {
                gQuest.ackTimer--;
            }
            for (i = 0; i < 4; i++) {
                if (gScene.inQuest[i] != 1) {
                    gQuest.acks |= 1 << (i + QUEST_ACK_REWARD_SHIFT);
                }
            }
            if ((gQuest.acks & (QUEST_ACKS_RANK | QUEST_ACKS_REWARD)) != (QUEST_ACKS_RANK | QUEST_ACKS_REWARD)&& gQuest.ackTimer > 0 && gScene.inQuest[gScene.localPlayer] == 1) {
                break;
            }
            gQuest.step++;
            gScene.questPhase = QUEST_PHASE_CLEARED;
            break;
        case 3:
            break;
        }
        break;
    case QSTATE_VERSUS_END:
        switch (gQuest.step) {
        case 0:
            if (gQuest.ackTimer > 0) {
                gQuest.ackTimer--;
            }
            for (i = 0; i < 4; i++) {
                if (gScene.inQuest[i] != 1) {
                    gQuest.acks |= 1 << i;
                }
            }
            if ((gQuest.acks & QUEST_ACKS_RANK) != QUEST_ACKS_RANK&& gQuest.ackTimer > 0 && gScene.inQuest[gScene.localPlayer] == 1) {
                break;
            }
            gQuest.step++;
            t = Quest_GetTime(QUEST_TIME_LIMIT);
            gQuest.elapsed = t - Quest_GetTime(QUEST_TIME_LEFT);
            if (gQuest.result == QUEST_PHASE_CLEARED) {
                gScene.questPhase = QUEST_PHASE_CLEARED;
                for (i = 0; i < gScene.playerCount; i++) {
                    if (gPlayers[i].side < 3 && Side_Match(gPlayers[i].side, gPlayers[gScene.localPlayer].side, 1) == 1) {
                        break;
                    }
                }
                Notice_Push(3, 3, i);
            } else {
                gScene.questPhase = QUEST_PHASE_FAILED;
                for (i = 0; i < gScene.playerCount; i++) {
                    if (gPlayers[i].side < 3 && Side_Match(gPlayers[i].side, gPlayers[gScene.localPlayer].side, 1) == 0) {
                        break;
                    }
                }
                Notice_Push(3, 3, i);
            }
            break;
        }
        break;
    }
    return 0;
}

/* Seeds the quest's random stream from the seed bytes (0x1C3 for a zero sum). */
void Quest_SeedRandom(void)
{
    s32 i;

    gQuest.random = 0;
    for (i = 0; i < 14; i += 2) {
        gQuest.random += (u16)(gQuest.seed[i] + (u16)(gQuest.seed[i + 1] << 8));
    }
    if (gQuest.random == 0) {
        gQuest.random = 0x1C3;
    }
}

/* The quest's next random number (gQuest.random * 176 mod 0xFF53): the
 * same on every player's machine, as the seed is the host's. */
u16 Quest_Random(void)
{
    u32 x = gQuest.random;

    if (x == 0) {
        x = 1;
    }
    return gQuest.random = x * 176 % 0xFF53;
}

u8 Quest_GetSeedByte(u8 i)
{
    return gQuest.seed[i];
}

/*
 * The timed supply event: each (minutes left, chance in 128) pair of the
 * event's table rolls once when the time left comes down to it; on
 * success the supplies arrive (the supply notice and its sound) and the
 * others are told.
 */
void Quest_UpdateTimedEvent(void)
{
    u8 *tbl = gQuestTimedEventTbls[gQuest.timedEventSet];

    switch (gQuest.timedEvent) {
    case QUEST_EVENT_TIMED:
        tbl += gQuest.timedEventStep * 2;
        if (tbl[0] == 0) {
            return;
        }
        if (tbl[0] * 1800 /* frames per minute */ < gQuest.timeLeft) {
            return;
        }
        gQuest.timedEventStep++;
        if ((Rand_Next(0) & 0x7F) < tbl[1]) {
            gQuest.timedEvent = QUEST_EVENT_NONE;
            Notice_PushItem(gQuestNoticeTexts[3]);
            Sound_PlaySeFlat(1, 0x77, 0);
            gQuest.msgArg0 = 0;
            gQuest.msgType = QMSG_EVENT_OVER;
            gQuest.msgArg1 = 0;
            gQuest.msgArg2 = 0;
            QSync_SendState(6, gScene.localPlayer);
        }
        break;
    case QUEST_EVENT_NONE:
        break;
    }
}

/* The kill supply event: a kill of the event's monster kind counts, and
 * the supplies arrive after the last one. */
void Quest_CountEventKill(u16 type)
{
    switch (gQuest.timedEvent) {
    case QUEST_EVENT_KILLS:
        if (gQuest.eventKind == type) {
            if (--gQuest.eventCount <= 0) {
                gQuest.timedEvent = QUEST_EVENT_NONE;
                Notice_PushItem(gQuestNoticeTexts[3]);
                Sound_PlaySeFlat(1, 0x77, 0);
            }
        }
        break;
    case QUEST_EVENT_NONE:
        break;
    }
}

/* Tells the others the quest's progress (QMSG_PROGRESS). */
void Quest_TargetMessage(void)
{
    gQuest.msgArg0 = 0;
    gQuest.msgArg2 = 0;
    gQuest.msgType = QMSG_PROGRESS;
    gQuest.msgArg1 = gQuest.progress;
    QSync_SendState(6, gScene.localPlayer);
}

/* An item target is met once the local player holds its count: the
 * progress moves on to the target's index. */
void Quest_CheckItemTargets(void)
{
    QuestTarget *t = gQuest.targets;
    s32 i;

    for (i = 0; i < 2; i++, t++) {
        switch (t->state) {
        case TARGET_DONE:
        case TARGET_KILLS:
            break;
        case TARGET_ITEMS:
            if (Pouch_Count(&gPlayers[gScene.localPlayer], t->item) >= t->count) {
                if (t->idx > gQuest.progress) {
                    gQuest.progress = t->idx;
                    Quest_TargetMessage();
                }
                t->state = TARGET_DONE;
            }
            break;
        }
    }
}

/* A kill of monster kind `type` counts for the kill targets of that kind
 * (their `item`); the last one moves the progress on. */
void Quest_CountTarget(u16 type)
{
    QuestTarget *t = gQuest.targets;
    s32 i;

    for (i = 0; i < 2; i++, t++) {
        switch (t->state) {
        case TARGET_KILLS:
            if (t->item == type) {
                if (--t->count <= 0) {
                    if (t->idx > gQuest.progress) {
                        gQuest.progress = t->idx;
                        Quest_TargetMessage();
                    }
                    t->state = TARGET_DONE;
                }
            }
            break;
        }
    }
}

/* Clears the quest's two targets. */
void Quest_ClearTargets(void)
{
    QuestTarget *t = gQuest.targets;
    s32 i;

    for (i = 0; i < 2; i++) {
        t[i].state = 0;
        t[i].item = 0;
        t[i].count = 0;
    }
}

/* Copies the stage's targets (StageHeader.targets) into gQuest. */
void Quest_LoadTargets(void)
{
    QuestTarget *dst = gQuest.targets;
    QuestTarget *src = gQuest.header->targets;
    s32 i;

    for (i = 0; i < 2; i++, dst++, src++) {
        if (src->state != 0) {
            dst->state = src->state;
            dst->item = src->item;
            dst->count = src->count;
            dst->idx = i + 1;
        }
    }
}

/* Creates the monster of quest entry q, when its kind has a model slot
 * (gScene.emKind): its place, size, level and needs from the entry; a
 * large monster in its own area, a small one in the scene's. Returns it,
 * or NULL. */
Em *QuestEm_Spawn(QuestEm *q)
{
    Em *em;
    s32 i;

    for (i = 0; i < 4; i++) {
        if (q->kind == gScene.emKind[i]) {
            break;
        }
    }
    if (i >= 4) {
        return NULL;
    }
    if ((em = EmSlot_Alloc()) != NULL) {
        em->modelSlot = i;
        em->type = q->kind;
        em->variant = q->variant;
        em->questIndex = q->index;
        if (gEmIsLarge[em->type] != 0) {
            em->area = q->area;
        } else {
            em->area = gScene.area;
        }
        em->need1Gauge = q->need1Gauge;
        em->need2Gauge = q->need2Gauge;
        em->need3Gauge = q->need3Gauge;
        em->spawnParam = q->spawnParam;
        em->questKey = q->questKey;
        em->posX = q->x;
        em->posY = q->y;
        em->posZ = q->z;
        em->rotY = q->rotY;
        em->sizePct = q->size;
        em->level = q->level;
        q->emIndex = em->index;
        GmEm_Task(em);
        GmEm_ApplySize(em);
        Em_UpdateMatrix(em);
        if (q->hp == -1) {
            q->hp = em->hp;
            if (q->large != 0) {
                gQuest.spawnedEm = em;
                gQuest.savedEmHp = em->hp;
            }
        } else {
            em->hp = q->hp;
        }
    }
    return em;
}

/*
 * Leaving area fromArea for toArea: the monsters that stay are the large
 * ones and the stage-wide kinds (StageEntry part 0); the others are freed,
 * their quest entries keeping their health, or counted as hunted when
 * they were down (a Melynx gives back what it stole; a player's felyne
 * cart is let go). The area's entries are reset, and the model slots of
 * kinds that neither the stage nor toArea's spawns need are freed.
 */
void QuestEm_LeaveArea(s32 fromArea, s32 toArea)
{
    s32 *ids;
    Em *em;
    s32 i;
    s32 keep;
    s32 j;
    QuestEm *q;
    Player *pl;
    s32 found;

    em = gEms;
    ids = StageEntry_GetData(gQuest.stageEntry, 0);

    for (i = 0; i < 20 /* gEms */; i++, em++) {
        if (em->active == 0) {
            continue;
        }
        if (ids != NULL) {
            for (j = 0; j < 4; j++) {
                if (ids[j] <= 0) {
                    keep = 0;
                    goto checked;
                }
                if (gEmIsLarge[em->type] != 0) {
                    keep = 1;
                    goto checked;
                }
                if (em->type == ids[j]) {
                    keep = 1;
                    goto checked;
                }
            }
            keep = 0;
        } else {
            keep = 0;
        }
    checked: /* the search's exits are gotos in the game's code */
        if ((q = QuestEm_FindForEm(em)) != NULL && q->flags != QUEST_EM_DONE && !keep) {
            if (em->type == EM_KIND_MELYNX) {
                EmMelynx_DeductReward(em);
            }
            if (em->taskPhase >= 2 || em->stateGroup == EM_GROUP_DEFEAT) {
                QuestEm_Hunted(em);
                if (q->flags & QUEST_EM_HUNTED) {
                    q->flags = QUEST_EM_ACTIVE;
                    q->hp = -1;
                }
            } else {
                q->hp = em->hp;
            }
            q->emIndex = -1;
        }
        if (!keep) {
            if (em->type == EM_KIND_FELYNE_CART) {
                pl = gPlayers;
                for (j = 0; j < gScene.playerCount; j++, pl++) {
                    if (pl->active && pl->cart == em) {
                        pl->cart = NULL;
                    }
                }
            }
            EmSlot_Free(em);
        }
    }
    QuestEm_ResetArea(fromArea);
    for (i = 0; i < 4; i++) {
        if (gScene.emKind[i] <= 0) {
            continue;
        }
        ids = StageEntry_GetData(gQuest.stageEntry, 0);
        if (ids != NULL) {
            for (j = 0; j < 4; j++) {
                if (ids[j] <= 0) {
                    found = 0;
                    goto searched;
                }
                if (gScene.emKind[i] == ids[j]) {
                    found = 1;
                    goto searched;
                }
            }
            found = 0;
        searched:
            if (found) {
                continue;
            }
        }
        ids = Stage_FindEntryData(gQuest.emSpawns, toArea, 0, gQuest.progress);
        if (ids != NULL) {
            j = 0;
            for (;;) {
                if (*ids <= 0) {
                    found = 0;
                    break;
                }
                if (gScene.emKind[i] == *ids) {
                    found = 1;
                    break;
                }
                if (++j >= 4) {
                    found = 0;
                    break;
                }
                ids++;
            }
            if (found) {
                continue;
            }
        }
        EmSlot_FreeModels(i);
        gScene.emKind[i] = 0;
    }
}

/*
 * Entering an area: its effects; at the base camp the felyne cart's
 * model; the models of the area's spawn kinds for the current progress
 * and its quest monsters (not those done; the trader's by
 * gQuest.traderVariant); Yama Tsukami's second kind when rolled; and the
 * special mining point's object when it lies here.
 */
void QuestEm_SpawnArea(s32 area)
{
    QuestEm *q;
    s32 i;
    s32 *ids;
    s32 addYama;
    u8 *p;
    s32 k;
    s32 id;

    Effect17_SpawnArea(area);
    if (area == gScene.campArea) {
        for (i = 0; i < 4; i++) {
            if (gScene.emKind[i] <= 0) {
                gScene.emKind[i] = EM_KIND_FELYNE_CART;
                GmLoad_LoadEmData(gScene.map, gScene.emKind[i]);
                EmSlot_LoadModels(i);
                break;
            }
        }
    }
    addYama = (s8)QuestEm_RollYamaTsukamiB(area);
    if (addYama != 0) {
        for (i = 0; i < 4; i++) {
            if (gScene.emKind[i] <= 0) {
                gScene.emKind[i] = EM_KIND_YAMA_TSUKAMI_B;
                GmLoad_LoadEmData(gScene.map, gScene.emKind[i]);
                EmSlot_LoadModels(i);
                break;
            }
        }
    }
    ids = Stage_FindEntryData(gQuest.emSpawns, area, 0, gQuest.progress);
    q = Stage_FindEntryData(gQuest.emSpawns, area, 1, gQuest.progress);
    if (ids != NULL) {
        for (;;) {
            id = *ids;
            if (id <= 0) {
                break;
            }
            /* the kind's model slot, if loaded (gScene.emKind read through a byte pointer, as the game does) */
            p = (u8 *)&gScene;
            for (k = 0; k < 4; k++, p++) {
                if (((SceneState *)p)->emKind[0] == id) {
                    break;
                }
            }
            if (k < 4) {
                ids++;
                continue;
            }
            for (i = 0; i < 4; i++) {
                if (gScene.emKind[i] <= 0) {
                    gScene.emKind[i] = id;
                    GmLoad_LoadEmData(gScene.map, gScene.emKind[i]);
                    EmSlot_LoadModels(i);
                    break;
                }
            }
            ids++;
        }
        for (;; q++) {
            if (q->kind <= 0) {
                break;
            }
            if (q->flags & QUEST_EM_DONE) {
                continue;
            }
            if (q->kind == 10) { /* the trader: only the entries of gQuest.traderVariant */
                if (gQuest.traderVariant == 0) {
                    if (q->variant != 0) {
                        continue;
                    }
                } else {
                    if (q->variant == 0) {
                        continue;
                    }
                }
            }
            if (q->kind < 100) {
                QuestEm_Spawn(q);
                q->area = gScene.area;
            }
        }
    }
    if ((s8)addYama != 0) {
        QuestEm_SpawnYamaTsukamiB();
    }
    if (gQuest.specialMineArea == area) {
        StageObj_K0E_CreateAt(gQuest.specialMinePos, 10);
    }
}

/* Quest text i: offline from the stage file's texts, online the selected quest's. */
void *QuestInfo_Get(s32 i)
{
    void *p;
    u32 *q;

    switch (gScene.online) {
    default:
        p = Quest_SelectedText(i);
        break;
    case 0:
        q = gQuest.texts;
        q += i;
        p = (void *)(*q + gStageFile);
        break;
    }
    return p;
}

/* Copies the stage-wide monster kinds (StageEntry part 0, up to 4) to
 * kinds and returns their count; 0 when the stage has none. */
s32 Quest_GetStageEmKinds(u8 *kinds)
{
    s32 *a;
    s32 *b;
    s32 n = 0;
    s32 i;

    a = StageEntry_GetData(gQuest.stageEntry, 0);
    b = StageEntry_GetData(gQuest.stageEntry, 1);
    if (a == NULL) {
        return 0;
    }
    if (b == NULL) {
        return 0;
    }
    for (i = 0; i < 4; i++) {
        if (*a <= 0) {
            break;
        }
        *kinds = *a;
        n++;
        kinds++;
        a++;
    }
    return n;
}

/* The count of item id in the quest's item list (gQuest.rewards), 0 when absent. */
static s16 Quest_FindReward(u16 id)
{
    s16 i;

    for (i = 0; i < 5; i++) {
        if (gQuest.rewards[i].id == id) {
            return gQuest.rewards[i].val;
        }
    }
    return 0;
}

/*
 * Adds n of item to the quest's item list (gQuest.rewards, five slots). A
 * new item takes a free slot, or else overwrites the next slot in turn
 * (rewardNext); a listed one is capped at the item's maximum and leaves the
 * list at 0. Returns 0 new, 1 added, 2 capped, 3 already full, 4 gone, 5
 * overwritten. Quest_FindReward is static, which lets the compiler keep
 * this function's registers across the call.
 */
u16 Quest_AddReward(u16 item, s16 n)
{
    s16 i;
    u16 r;
    s8 max;

    if (Quest_FindReward(item) == 0) {
        r = 5;
        for (i = 0; i < 5; i++) {
            if (gQuest.rewards[i].id == 0 && n > 0) {
                gQuest.rewards[i].id = item;
                gQuest.rewards[i].val = n;
                r = 0;
                break;
            }
        }
        if (r == 5) {
            gQuest.rewards[gQuest.rewardNext].id = item;
            gQuest.rewards[gQuest.rewardNext].val = n;
            gQuest.rewardNext++;
            if (gQuest.rewardNext >= 5) {
                gQuest.rewardNext = 0;
            }
        }
    } else {
        for (i = 0; i < 5; i++) {
            if (gQuest.rewards[i].id == item) {
                max = gItemData[item].max;
                if (n > 0 && gQuest.rewards[i].val >= max) {
                    gQuest.rewards[i].val = max;
                    r = 3;
                    break;
                }
                gQuest.rewards[i].val += (s8)n;
                if (gQuest.rewards[i].val <= 0) {
                    r = 4;
                    gQuest.rewards[i].id = 0;
                    gQuest.rewards[i].val = 0;
                } else if (gQuest.rewards[i].val > max) {
                    gQuest.rewards[i].val = max;
                    r = 2;
                } else {
                    r = 1;
                }
                break;
            }
        }
    }
    /* r is not set when the second loop finds no slot of item: the game
     * then returns what its register held (a2, the caller's) */
    return r;
}

/* An item stolen from player p (the game's Melynx, em_cephadrome.c): n of
 * item off the quest's item list, noted for the scene (deductedItem,
 * deductedCount), and the list is to be given (rewardsPending). */
void Quest_DeductReward(Player *p, u16 item, s16 n)
{
    gScene.deductedItem = item;
    gScene.deductedCount = n;
    gScene.rewardDeducted = 1;
    Quest_AddReward(item, -n);
    gQuest.rewardsPending = 1;
}

/*
 * Gives pl the quest's item list (unk_70A290.c), as rolled from
 * gRewardGiveRolls: the listed items (0), item 0xFF instead (1), or
 * nothing (2), each with its notice; then the list is cleared.
 */
void Quest_GiveRewards(Player *pl)
{
    s32 i;
    s32 any = 0;

    if (gQuest.rewardsPending != 0) {
        /* only entries 0 and 8 can be rolled */
        switch (gRewardGiveRolls[Rand_Next(0) & 8]) {
        default:
            for (i = 0; i < 5; i++) {
                if (gQuest.rewards[i].id != 0) {
                    switch (Pouch_Add(pl, gQuest.rewards[i].id, gQuest.rewards[i].val)) {
                    case 0:
                    case 1:
                    case 2:
                        if (Player_IsLocal(pl) == 1) {
                            Notice_Push(1, 0, gQuest.rewards[i].id);
                        }
                        any = 1;
                        break;
                    case 3:
                        if (Player_IsLocal(pl) == 1) {
                            Notice_Push(1, 3, gQuest.rewards[i].id);
                        }
                        any = 1;
                        break;
                    }
                }
            }
            if (any == 0 && Player_IsLocal(pl) == 1) {
                Notice_Push(0, 0, 0);
            }
            break;
        case 1:
            switch (Pouch_Add(pl, 0xFF, 1)) {
            case 0:
            case 1:
            case 2:
                if (Player_IsLocal(pl) == 1) {
                    Notice_Push(1, 0, 0xFF);
                }
                break;
            case 3:
                if (Player_IsLocal(pl) == 1) {
                    Notice_Push(1, 3, 0xFF);
                }
            default:
                if (Player_IsLocal(pl) == 1) {
                    Notice_Push(0, 0, 0);
                }
                break;
            }
            break;
        case 2:
            if (Player_IsLocal(pl) == 1) {
                Notice_Push(0, 0, 0);
            }
            break;
        }
        for (i = 0; i < 5; i++) {
            gQuest.rewards[i].id = 0;
            gQuest.rewards[i].val = 0;
        }
        gQuest.rewardNext = 0;
        gQuest.rewardsPending = 0;
    } else {
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 0, 0);
        }
    }
}

u16 Quest_Deliver(Player *pl, u16 item, s32 count)
{
    s16 i;
    s16 max = 0;
    s16 left;
    s16 t;

    for (i = 0; i < 3; i++) {
        left = (s16)QuestGoal_DeliverCountFor(item, i);
        if (left >= 0) {
            t = (s16)Quest_DeliveredCount(item);
            left -= t;
            if (left > max) {
                max = left;
            }
        }
    }
    if (max <= 0) {
        return 0;
    }
    if (Quest_CountDelivered(item, 1) == 0) {
        for (i = 0; i < 4; i++) {
            if (pl->delivered[i].item == 0) {
                pl->delivered[i].item = item;
                if ((s16)count >= max) {
                    count = max;
                }
                pl->delivered[i].count = (s8)count;
                Pouch_Add(pl, item, -(s16)count);
                return count;
            }
        }
    } else {
        for (i = 0; i < 4; i++) {
            if (pl->delivered[i].item == item) {
                if ((s16)count >= max) {
                    count = max;
                }
                pl->delivered[i].count += (s8)count;
                Pouch_Add(pl, item, -(s16)count);
                return count;
            }
        }
    }
    return count;
}

u8 Quest_SetDelivered(Player *pl, u16 item, s32 count)
{
    s16 i;
    s16 have;
    s16 max = 0;
    s16 t;
    u8 r;

    for (i = 0; i < 3; i++) {
        have = (s16)QuestGoal_DeliverCountFor(item, i);
        if (have >= 0) {
            t = (s16)Quest_DeliveredCount(item);
            have -= t;
            if (have > max) {
                max = have;
            }
        }
    }
    if (max <= 0) {
        return 2;
    }
    for (have = 0, i = 0; i < 4; i++) {
        if (pl->delivered[i].item == item) {
            have = pl->delivered[i].count;
        }
    }
    if (have == 0) {
        r = 0;
        for (i = 0; i < 4; i++) {
            if (pl->delivered[i].item == 0) {
                pl->delivered[i].item = item;
                if ((s16)count >= max) {
                    r = 2;
                }
                pl->delivered[i].count = (s8)count;
                break;
            }
        }
    } else {
        r = 1;
        for (i = 0; i < 4; i++) {
            if (pl->delivered[i].item == item) {
                if ((s16)count >= max) {
                    r = 2;
                }
                pl->delivered[i].count = (s8)count;
                break;
            }
        }
    }
    return r;
}

s16 Quest_CountDelivered(u16 item, s32 who)
{
    s16 n = 0;
    Player *pl;
    s32 i;
    s32 j;

    switch (who) {
    case 0:
        pl = gPlayers;
        for (j = 0; j < gScene.playerCount; j++, pl++) {
            for (i = 0; i < 4; i++) {
                if (pl->delivered[i].item == item) {
                    n += pl->delivered[i].count;
                }
            }
        }
        break;
    case 1:
        pl = &gPlayers[gScene.localPlayer];
        for (i = 0; i < 4; i++) {
            if (pl->delivered[i].item == item) {
                return pl->delivered[i].count;
            }
        }
        break;
    default:
        pl = &gPlayers[who - 2];
        for (i = 0; i < 4; i++) {
            if (pl->delivered[i].item == item) {
                return pl->delivered[i].count;
            }
        }
        break;
    }
    return n;
}

void Quest_DeliveryMessage(u16 item)
{
    gQuest.msgArg0 = 0;
    gQuest.msgArg1 = item;
    gQuest.msgType = 9;
    gQuest.msgArg2 = Quest_CountDelivered(item, 1);
    QSync_SendState(6, gScene.localPlayer);
    Notice_Push(1, 8, item);
}

void Quest_DeliverFromPouch(Player *pl, s32 mode)
{
    s32 i;
    s32 j;
    s32 found;
    u16 item;

    if (Quest_IsRunning(1) != 0) {
        return;
    }
    found = 0;
    for (i = 0; i < 3; i++) {
        item = QuestGoal_DeliverItem(i);
        if (item == 0) {
            continue;
        }
        switch (mode) {
        case 0:
            if (QuestGoal_CopyFlags(i, 0x3000) != 0) {
                continue;
            }
            break;
        case 1:
            if (QuestGoal_CopyFlags(i, 0x3000) != 0x1000) {
                continue;
            }
            break;
        case 2:
            if (QuestGoal_CopyFlags(i, 0x3000) != 0x2000) {
                continue;
            }
            break;
        }
        for (j = 0; j < 20; j++) {
            if (pl->pouch[j].item == (u16)item) {
                if (Quest_Deliver(pl, pl->pouch[j].item, pl->pouch[j].count) > 0) {
                    found = 1;
                    Quest_DeliveryMessage(item);
                }
            }
        }
    }
    if (found) {
        Jingle_Play(pl, 0xB7);
    }
}

s32 Quest_GetDeliveryTarget(Player *unused, s32 idx, ItemStack *out)
{
    s32 i = idx;
    u16 item;

    out->item = 0;
    out->count = 0;
    if (Quest_IsRunning(1) != 0) {
        return 0;
    }
    item = QuestGoal_DeliverItem(i);
    if (item != 0) {
        if (QuestGoal_CopyFlags(i, 0x3000) != 0) {
            return 0;
        }
        i = QuestGoal_DeliverCount(i);
        if (i >= 0) {
            out->item = item;
            i -= Quest_CountDelivered(item, 0);
            if (i >= 0) {
                out->count = i;
            }
        }
    }
    if (out->item == 0) {
        return 0;
    }
    return out->count == 0 ? 1 : 2;
}

u16 Quest_DeliverItem(Player *pl, u16 item, s16 count)
{
    s32 n = Quest_Deliver(pl, item, count);

    if (n > 0) {
        Quest_DeliveryMessage(item);
        Jingle_Play(pl, 0xB7);
    }
    return n;
}

/* unk_3A1070's .data (0x44B450-0x44B790) */
extern u32 D_006AEDB8[];
s8 gTraderVariantRolls[16] = {
#include "sub_main/0044B450.inc"
};
u16 gSpecialMineAgeRanks[8] = {
#include "sub_main/0044B460.inc"
};
RangeU8 gEmSizeRangeDefaults[5] = {
#include "sub_main/0044B470.inc"
};
u16 gEmAppearMax[76] = {
#include "sub_main/0044B480.inc"
};
u16 gRewardDeliverBands[6] = {
#include "sub_main/0044B518.inc"
};
u16 gRewardHuntBands[16] = {
#include "sub_main/0044B530.inc"
};
u32 D_0044B550[16] = {
#include "sub_main/0044B550.inc"
};
u32 D_0044B590[12] = {
#include "sub_main/0044B590.inc"
};
u32 D_0044B5C0[8] = {
#include "sub_main/0044B5C0.inc"
};
u32 D_0044B5E0[2] = {
#include "sub_main/0044B5E0.inc"
};
u32 D_0044B5E8[3] = {
#include "sub_main/0044B5E8.inc"
};
u32 D_0044B5F8[2] = {
#include "sub_main/0044B5F8.inc"
};
u32 D_0044B600[2] = {
#include "sub_main/0044B600.inc"
};
u32 D_0044B608[3] = {
#include "sub_main/0044B608.inc"
};
#pragma explicit_zero_data on
u32 D_0044B618[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
s8 *gRewardRateTables[8] = {
    (s8 *)D_0044B550, (s8 *)D_0044B590, (s8 *)D_0044B5C0, (s8 *)D_0044B5E0,
    (s8 *)D_0044B5E8, (s8 *)D_0044B5F8, (s8 *)D_0044B600, (s8 *)D_0044B608,
};
u8 gQuestTimedEventTbls_2[8] = {
#include "sub_main/0044B640.inc"
};
u8 gQuestTimedEventTbls_3[12] = {
#include "sub_main/0044B648.inc"
};
u8 gQuestTimedEventTbls_4[12] = {
#include "sub_main/0044B658.inc"
};
u8 gQuestTimedEventTbls_5[16] = {
#include "sub_main/0044B670.inc"
};
u8 gQuestTimedEventTbls_6[32] = {
#include "sub_main/0044B680.inc"
};
u8 gQuestTimedEventTbls_7[32] = {
#include "sub_main/0044B6A0.inc"
};
u8 gQuestTimedEventTbls_8[32] = {
#include "sub_main/0044B6C0.inc"
};
u8 gQuestTimedEventTbls_9[32] = {
#include "sub_main/0044B6E0.inc"
};
u8 gQuestTimedEventTbls_10[32] = {
#include "sub_main/0044B700.inc"
};
u8 gQuestTimedEventTbls_11[48] = {
#include "sub_main/0044B720.inc"
};
u8 *gQuestTimedEventTbls[16] = {
    (u8 *)D_006AEDB8, (u8 *)D_006AEDB8, gQuestTimedEventTbls_2, gQuestTimedEventTbls_3,
    gQuestTimedEventTbls_4, gQuestTimedEventTbls_5, gQuestTimedEventTbls_6, gQuestTimedEventTbls_7,
    gQuestTimedEventTbls_8, gQuestTimedEventTbls_9, gQuestTimedEventTbls_10, gQuestTimedEventTbls_11,
    (u8 *)D_006AEDB8, NULL, NULL, NULL,
};

/* ---- 0x3A5650 (was mine_point.c) ---- */
/*
 * Mining and bug-catching points: up to 20 points placed at run time
 * (MinePoint_Add, e.g. from a stage object), kept in gMinePoints with an
 * active list gMineActive (oldest first, gQuest.mineCount entries). A point runs
 * out when its count or its timer reaches 0; with 20 active, the oldest
 * point without flag 1 makes room for a new one. Gather_AtMinePoint rolls
 * the item from the tables at gGatherItemTables in the game overlay
 * (Gather_RollMineItem, random stream 0); table 0x1000 is special
 * (SpecialMine_Gather).
 */

u16 Gather_RollMineItem(Player *p, u16 table);


extern GatherEntry *gGatherItemTables[];

s32 QuestGoal_CopyFlags(u16 i, s32 mask);
s32 QuestGoal_IsDone(s32 i);
s32 Player_HasSkill(Player *p, u8 skill);
s32 SpecialMine_Gather(Player *p);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
f32 Math_Sqrtf(f32 x);   /* sqrtf */
u16 Rand_Next(s32 stream);
void MineActive_RemoveAt(s8 idx);
void MineActive_Remove(s8 slot);
void MineActive_Append(s8 slot);

s32 QuestGoal_DeliveryDone(void)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (QuestGoal_DeliverItem(i) != 0 && QuestGoal_CopyFlags(i, 0x3000) == 0x1000 && QuestGoal_IsDone(i) == 1) {
            return (gQuest.flags & 0x40000000) ? 2 : 1;
        }
    }
    return 0;
}

/* Rolls an item from mining/bug table `table`; the item 0x290 in the pouch
 * moves the roll one entry up the table. */
u16 Gather_RollMineItem(Player *p, u16 table)
{
    u16 sum;
    GatherEntry *e;
    u16 r;
    GatherEntry **t;

    sum = 0;
    t = &gGatherItemTables[table];
    for (e = *t; e->weight != 0xFFFF; e++) {
        sum += e->weight;
    }
    r = Rand_Next(0) % sum;
    if (Pouch_Count(p, 0x290) > 0 && r < sum - 1) {
        r++;
    }
    sum = 0;
    for (e = *t; e->weight != 0xFFFF; e++) {
        sum += e->weight;
        if (r < sum) {
            return e->item;
        }
    }
    return 0xFFFF;
}

/* Clears all 20 mine points (table 0xFFFF) and the active list. */
void MinePoint_ResetAll(void)
{
    MinePoint *pt;
    s32 i;

    gQuest.mineCount = 0;
    pt = gMinePoints;
    for (i = 0; i < 20; i++, pt++) {
        pt->table = 0xFFFF;
        gMineActive[i] = -1;
    }
}

/* Counts the active points down and shows their effect every 64 frames. */
/* Counts down the timer of each active mine point and removes the spent or expired ones;
 * every 64 frames the others spawn effect 0x15 (Effect0D_SpawnAt) 20 units above them, with
 * 0.5, or 0.2 to 0.5 over the last 900 frames. */
void MinePoint_UpdateAll(void)
{
    s32 i;
    MinePoint *pt;
    f32 pos[3];

    for (i = 0; i < gQuest.mineCount; i++) {
        pt = &gMinePoints[gMineActive[i]];
        if (pt->table == 0xFFFF) {
            continue;
        }
        if (pt->timer == 0) {
            continue;
        }
        if (pt->count <= 0 || --pt->timer <= 0) {
            MinePoint_Remove(gMineActive[i]);
            continue;
        }
        if ((pt->timer & 0x3F) != 0) {
            continue;
        }
        pos[0] = pt->x;
        pos[1] = pt->y + 20.0f;
        pos[2] = pt->z;
        Effect0D_SpawnAt(pos, 0x15, pt->timer < 900 ? 0.2f + 0.3f * (pt->timer / 900.0f) : 0.5f);
    }
}

/* Adds a point like `src` (at `pos` if given); returns its slot or -1. */
s32 MinePoint_Add(MinePoint *src, f32 *pos, u8 noBonus)
{
    MinePoint *pt;
    s32 i;

    pt = gMinePoints;
    MinePoint_RemoveOldest();
    for (i = 0; i < 20; i++, pt++) {
        if (pt->table != 0xFFFF) {
            continue;
        }
        if (pos == NULL) {
            pt->x = src->x;
            pt->y = src->y;
            pt->z = src->z;
        } else {
            pt->x = pos[0];
            pt->y = pos[1];
            pt->z = pos[2];
        }
        pt->radius = src->radius;
        pt->table = src->table;
        pt->count = src->count;
        if (noBonus == 0 && Player_HasSkill(&gPlayers[gScene.localPlayer], 0xBE) == 1) {
            pt->count++;
        }
        pt->unk14 = src->unk14;
        pt->area = src->area;
        pt->flags = src->flags;
        pt->timer = src->timer;
        MineActive_Append(i);
        return i;
    }
    return -1;
}

s32 MinePoint_Count(s32 slot)
{
    return gMinePoints[slot].count;
}

void MinePoint_Remove(s32 slot)
{
    if (slot != -1) {
        gMinePoints[slot].table = 0xFFFF;
        MineActive_Remove(slot);
    }
}

void MinePoint_SetPos(s32 slot, f32 *pos)
{
    gMinePoints[slot].x = pos[0];
    gMinePoints[slot].y = pos[1];
    gMinePoints[slot].z = pos[2];
}

void MinePoint_SetArea(s32 slot, u16 area)
{
    gMinePoints[slot].area = area;
}

/* unk14 of the point the player stands at, or 0xFFFF. */
u16 MinePoint_FindNearPlayer(void *obj)
{
    ObjHead *o = obj;
    MinePoint *pt;
    s32 i;
    f32 dx;
    f32 dz;

    pt = gMinePoints;
    for (i = 0; i < 20; i++, pt++) {
        if (pt->table == 0xFFFF) {
            continue;
        }
        if (pt->area != o->area) {
            continue;
        }
        if (o->pos[1] < pt->y - 200.0f) {
            continue;
        }
        if (!(o->pos[1] < pt->y + 100.0f)) {
            continue;
        }
        dx = o->pos[0] - pt->x;
        dz = o->pos[2] - pt->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= pt->radius)) {
            continue;
        }
        return pt->unk14;
    }
    return 0xFFFF;
}

/* Gathers at the mining/bug point the player stands at: the item, 0xFFFE
 * if the point is used up, 0 if there is none. */
u16 Gather_AtMinePoint(Player *p)
{
    MinePoint *pt;
    s32 i;
    u16 item;
    f32 dx;
    f32 dz;

    pt = gMinePoints;
    for (i = 0; i < 20; i++, pt++) {
        if (pt->table == 0xFFFF) {
            continue;
        }
        if (pt->area != p->area) {
            continue;
        }
        if (p->pos[1] < pt->y - 200.0f) {
            continue;
        }
        if (!(p->pos[1] < pt->y + 100.0f)) {
            continue;
        }
        dx = p->pos[0] - pt->x;
        dz = p->pos[2] - pt->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= pt->radius)) {
            continue;
        }
        if (pt->count > 0) {
            if (pt->table == 0x1000) {
                return SpecialMine_Gather(p);
            }
            item = Gather_RollMineItem(p, pt->table & 0x7FFF);
            if (pt->count != 0xFF) {
                pt->count--;
            }
        } else {
            item = 0xFFFE;
        }
        return item;
    }
    return 0;
}

/* With 20 points active, removes the oldest one without flag 1. */
void MinePoint_RemoveOldest(void)
{
    s32 i;

    if (gQuest.mineCount >= 20) {
        for (i = 0; i < gQuest.mineCount - 1; i++) {
            if (!(gMinePoints[gMineActive[i]].flags & 1)) {
                break;
            }
        }
        MinePoint_Remove(gMineActive[i]);
    }
}

/* Removes entry idx of the active list gMineActive (the later ones move down; the last becomes -1). */
void MineActive_RemoveAt(s8 idx)
{
    s32 i;

    for (i = idx; i < gQuest.mineCount - 1; i++) {
        gMineActive[i] = gMineActive[i + 1];
    }
    gMineActive[i] = -1;
    gQuest.mineCount--;
}

void MineActive_Remove(s8 slot)
{
    s32 i;

    for (i = 0; i < gQuest.mineCount; i++) {
        if (slot == gMineActive[i]) {
            MineActive_RemoveAt(i);
            return;
        }
    }
}

void MinePoint_RemoveTemporary(void)
{
    s32 i;
    u8 *obj;

    obj = (u8 *)gEms;
    for (i = 0; i < 20; i++, obj += 0x9D0) {
        if (obj[0] != 0 && obj[0x10] != 0 && (s8)obj[0x9A8] != -1) {
            EmMine_RemoveTimed((Em *)obj);
        }
    }
    for (i = gQuest.mineCount - 1; i >= 0; i--) {
        if (!(gMinePoints[gMineActive[i]].flags & 1)) {
            MinePoint_Remove(gMineActive[i]);
        }
    }
}

void MineActive_Append(s8 slot)
{
    gMineActive[gQuest.mineCount] = slot;
    gQuest.mineCount++;
}

void func_003A6120(MinePoint *pt)
{
}

/* ---- 0x3A6130 (was unk_3A6130.c) ---- */
/*
 * Quest state (gQuest): the quest's objectives, its timers and the
 * per-quest monster and mine-point bookkeeping.
 */


#include "ee_stdarg.h"


/* A monster of the quest (0x3C bytes, lists end with id <= 0). */

/* A reward of the special mine point (4 bytes). */
typedef struct Reward {
    u16 item;
    u16 unk2;
} Reward;




/* A carcass mine point of a monster kind (gEmMinePoints, 16 bytes). */
typedef struct EmMineEntry {
    u16 table[3];       /* 0x0: by the stage's column (StageHeader.unk5C) */
    u16 count;          /* 0x6 */
    f32 radius;         /* 0x8: scaled by the monster's size */
    u8 padC[2];
    u8 flags;           /* 0xE */
    u8 bone;            /* 0xF */
} EmMineEntry;

/* A second mine point of a monster kind (gEmMinePoints2, 8 bytes). */
typedef struct EmMineEntry2 {
    u16 table[3];       /* 0x0: by the stage's column */
    u8 chance;          /* 0x6: out of 32 */
    u8 pad7;
} EmMineEntry2;

/* An object that carries a monster (EmHolder_CreateMine). */
typedef struct EmHolder {
    u8 pad0[0x34];
    Em *em;             /* 0x34 */
    u8 pad38[6];
    u16 area;           /* 0x3E */
} EmHolder;

extern u8 **gQuestEmAreaSets;
extern u8 *gQuestEmSets;
extern QuestInfo *gQuestInfo;
extern u8 gQuestStats[];
extern s32 gQuestPartyClearTime;
extern s8 gEmTallyClass[0x4A];
extern s32 gVersusEmAngles[2];
extern f32 gVersusEmPos[2][3];
extern char *gRewardCutFmt;
extern char *gDrillScoreTexts[];
extern u16 gMarkMonsterCands[];
extern u8 *gEmSizeRollTbls[];
extern u8 gEmSizeRollPicks[];
extern Reward *gSpecialMineRewards[];
extern u16 gSpecialMinePoints[];
extern u8 gEmIsLarge2[];
extern s8 *gEmLevelTables[];
extern s8 gEmLevelTableIdx[][2];
extern s8 gEmLevelSeasonAdd[][3];
extern u16 gEmSizeSeasonAdd[][3];
extern s32 gQuestTimeLeft;
extern u16 gSpecialMinePoint;
extern s32 gSpecialMineAge;
extern s32 gQuestCountdown;
extern char *gElderFledText[];
extern char *D_0044557C;
extern char *gSubBSupplyText[];
extern char *gGoalDoneTexts[];
extern char *gMustOrderAllText[];
extern char *gMustOrderOneText;
extern EmMineEntry *gEmMinePoints[];
extern EmMineEntry2 *gEmMinePoints2[];

void *memset(void *p, s32 c, u32 n);
s32 Quest_IsMustOrderOrVersus(); /* unit's own declaration: the definition (unk_39C290.c) takes no parameter; most callers here pass the object in a0, which it ignores */
s32 Quest_IsVersus(void);
s32 Quest_GetTime(s32 which);
s32 Quest_IsPartBroken(u16 type, u16 part);
void Versus_InitEm(Em *em);
void EmSlot_LoadModels(s32 slot);
void GmLoad_LoadEmData(s32 kind, s32 idx);
s32 Quest_IsMustOrder(void);
void Bgm_PlayEvent(s32 a);
s32 MsgEvent_Check(u16 kind);
void QSync_SendState(u8 kind, u8 who);
u16 Quest_Random(void);
u8 Notice_PushText(char *s);
void vsprintf(char *buf, char *fmt, va_list ap);
/* the calls cast to u8: this unit's code takes the value as u8 */
s32 Quest_GetSeason(void);
u8 Daytime_Get(void);
s16 Pouch_Space(Player *p, u16 item);
void Quest_Sprintf(char *buf, char *fmt, ...);
void QuestEm_RollLevel(QuestEm *q);
void QuestEm_Init(QuestEm *q, s32 *n);
u16 QuestEm_RollSize(void);
void QuestEm_InitSets(u8 *set, s32 *n);
void QuestEm_InitAreaSets(u8 **list, s32 *n);
void QuestGoal_Add(s32 flags, s16 target, s16 count);
s32 Quest_IsRunning(s32 ending);
s32 Quest_GetTime(s32 a);
void func_003A6120(MinePoint *pt);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
void Em_BonePos(void *obj, s16 bone, f32 *out);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Quest_IsDrill(void);
s32 Player_IsLocal(void *obj);
u16 Rand_Next(s32 stream);
s8 EmMine_CreateTimed(Em *em, f32 *pos, s32 idx);
void EmMine_RemoveTimed(Em *em);
void EmMine_Remove(Em *em);
s32 Quest_CheckSubClear(void);
void QuestGoal_Complete(u16 i, u32 flag);
void QuestGoal_CompleteIfRunning(u16 i, s32 flag);
void Quest_JumpToEnd(s32 a);
static void Quest_JumpToStart(s32 a);
s32 QuestGoal_IsDone(s32 i);
s32 QuestGoal_IsUsed(s32 i);
s32 SpecialMine_Rank(void);

/* Adds the carcass mine point of entry idx of the monster kind (gEmMinePoints), scaled by the monster size; returns the slot (Em.mineSlot). */
s8 EmMine_Create(Em *em, s32 idx)
{
    MinePoint pt;
    f32 pos[3];
    EmMineEntry *e;

    if (Quest_IsMustOrderOrVersus(em) == 1) {
        em->mineSlot = -1;
        return em->mineSlot;
    }
    e = gEmMinePoints[em->type];
    if (e == NULL) {
        em->mineSlot = -1;
    } else {
        e += idx;
        if (e->table[gStageHeader->tier] == 0xFFFF) {
            em->mineSlot = -1;
        } else {
            pt.radius = em->sizePct * (e->radius * em->bodyScale) / 100.0f;
            pt.table = e->table[gStageHeader->tier];
            pt.count = e->count;
            if (em->type == 0x17) {
                pt.unk14 = 0;
            } else {
                pt.unk14 = 2;
            }
            func_003A6120(&pt);
            pt.area = em->area;
            pt.timer = 0;
            pt.flags = e->flags;
            em->mineBone = e->bone;
            if (em->mineBone != 0) {
                Em_BonePos(em, em->mineBone, pos);
                pos[1] = em->floorY;
                em->mineSlot = MinePoint_Add(&pt, pos, 0);
            } else {
                em->mineSlot = MinePoint_Add(&pt, &em->posX, 0);
            }
        }
    }
    return em->mineSlot;
}

/* Adds a copy of pt without a time limit; -1 when Quest_IsMustOrderOrVersus refuses. */
s32 MinePoint_AddUntimed(MinePoint *pt, f32 *pos, u8 noBonus)
{
    if (Quest_IsMustOrderOrVersus(pt) == 1) {
        return -1;
    }
    pt->timer = 0;
    return MinePoint_Add(pt, pos, noBonus);
}

/* Adds the second (timed, 1800 frames) mine point of the monster (gEmMinePoints2) at pos, once; returns Em.mineSlot2. */
s8 EmMine_CreateTimed(Em *em, f32 *pos, s32 idx)
{
    MinePoint pt;
    EmMineEntry2 *e;

    if (Quest_IsMustOrderOrVersus(em) == 1) {
        em->mineSlot2 = -1;
        return em->mineSlot2;
    }
    if (em->mineSlot2 != -1) {
        return em->mineSlot2;
    }
    if (em->mineCount2 <= 0 || em->area != gScene.area) {
        em->mineSlot2 = -1;
        return em->mineSlot2;
    }
    e = gEmMinePoints2[em->type];
    if (e == NULL) {
        em->mineSlot2 = -1;
    } else {
        e += idx;
        if (em->type != 0x33 && em->type != 0x34 && (Rand_Next(0) & 0x1F) > e->chance) {
            em->mineSlot2 = -1;
            return em->mineSlot2;
        }
        if (e->table[gStageHeader->tier] == 0xFFFF) {
            em->mineSlot2 = -1;
        } else {
            pt.radius = 150.0f;
            pt.table = e->table[gStageHeader->tier];
            pt.count = em->mineCount2;
            pt.unk14 = 0;
            pt.area = em->area;
            pt.timer = 0;
            pt.flags = 0;
            em->mineSlot2 = MinePoint_Add(&pt, pos, 1);
            em->mineTimer2 = 1800;
            Sound_PlaySeAtPosChecked(1, 0x8C, 0, pos, 1, 0);
        }
    }
    return em->mineSlot2;
}

/* EmMine_CreateTimed with entry 0. */
s8 EmMine_CreateTimed0(Em *em, f32 *pos)
{
    return EmMine_CreateTimed(em, pos, 0);
}

/* Drops a one-off timed mine point of the monster kind at pos (monster in the current area only). */
void EmMine_Drop(Em *em, f32 *pos)
{
    MinePoint pt;
    EmMineEntry2 *e;

    if (Quest_IsMustOrderOrVersus(em) == 1) {
        return;
    }
    if (em->area != gScene.area) {
        return;
    }
    e = gEmMinePoints2[em->type];
    if (e == NULL) {
        return;
    }
    if (em->type != 0x33 && em->type != 0x34 && (Rand_Next(0) & 0x1F) > e->chance) {
        return;
    }
    if (e->table[gStageHeader->tier] == 0xFFFF) {
        return;
    }
    pt.radius = 150.0f;
    pt.table = e->table[gStageHeader->tier];
    pt.count = em->mineCount2;
    pt.unk14 = 0;
    pt.area = em->area;
    pt.timer = 1800;
    pt.flags = 0;
    MinePoint_Add(&pt, pos, 1);
    Sound_PlaySeAtPosChecked(1, 0x8C, 0, pos, 1, 0);
}

/* Removes the second mine point of the monster. */
void EmMine_RemoveTimed(Em *em)
{
    MinePoint_Remove(em->mineSlot2);
    em->mineSlot2 = -1;
}

/* Keeps the second mine point in step: copies its count back, counts the timer down (removing it at 0) and shows the effect every 64 frames. */
void EmMine_UpdateTimed(Em *em)
{
    f32 pos[3];

    if (em->mineSlot2 == -1) {
        return;
    }
    if (gMinePoints[em->mineSlot2].count < em->mineCount2) {
        em->mineCount2 = gMinePoints[em->mineSlot2].count;
        if (em->mineCount2 <= 0) {
            EmMine_RemoveTimed(em);
            return;
        }
    }
    if (--em->mineTimer2 <= 0) {
        EmMine_RemoveTimed(em);
        return;
    }
    if ((em->mineTimer2 & 0x3F) != 0) {
        return;
    }
    pos[0] = gMinePoints[em->mineSlot2].x;
    pos[1] = gMinePoints[em->mineSlot2].y + 20.0f;
    pos[2] = gMinePoints[em->mineSlot2].z;
    Effect0D_SpawnAt(pos, 0x15, em->mineTimer2 < 900 ? 0.2f + 0.3f * (em->mineTimer2 / 900.0f) : 0.5f);
}

/* Adds the mine point of entry 1 of the held monster's kind at pos. */
s32 EmHolder_CreateMine(EmHolder *h, s32 unused, f32 *pos)
{
    Em *em = h->em;
    MinePoint pt;
    EmMineEntry *e;

    if (Quest_IsMustOrderOrVersus(h) == 1) {
        return -1;
    }
    e = gEmMinePoints[em->type];
    if (e == NULL) {
        return -1;
    }
    if (e[1].table[gStageHeader->tier] == 0xFFFF) {
        return -1;
    }
    pt.radius = e[1].radius;
    pt.table = e[1].table[gStageHeader->tier];
    pt.count = e[1].count;
    pt.unk14 = 2;
    pt.area = h->area;
    pt.timer = 0;
    pt.flags = e[1].flags;
    return MinePoint_Add(&pt, pos, 0);
}

/* Moves the carcass mine point with the monster (bone Em.mineBone, or its position); returns the gathers left, -1 when none. */
s32 EmMine_Update(Em *em)
{
    f32 pos[3];

    if (em->mineSlot != -1) {
        if (gMinePoints[em->mineSlot].count <= 0) {
            EmMine_Remove(em);
            return -1;
        }
        if (em->mineBone != 0) {
            Em_BonePos(em, em->mineBone, pos);
            pos[1] = em->floorY;
            MinePoint_SetPos(em->mineSlot, pos);
        } else {
            MinePoint_SetPos(em->mineSlot, &em->posX);
        }
        return gMinePoints[em->mineSlot].count;
    }
    return -1;
}

/* Removes the carcass mine point of the monster. */
void EmMine_Remove(Em *em)
{
    MinePoint_Remove(em->mineSlot);
    em->mineSlot = -1;
}

/* Clears the special mine point (gSpecialMinePoint). */
void SpecialMine_Clear(void)
{
    gSpecialMinePoint = 0;
}

/* Returns 1 when there is no special mine point. */
s32 SpecialMine_IsFree(Player *unused0)
{
    return gSpecialMinePoint == 0;
}

/* Creates the special mine point (table 0x1000) where the local player stands; returns 1 when created. */
s32 SpecialMine_Create(Player *p)
{
    MinePoint pt;

    if (!Player_IsLocal(p)) {
        return 0;
    }
    if (gSpecialMinePoint != 0) {
        return 0;
    }
    pt.radius = 200.0f;
    pt.table = 0x1000;
    pt.count = 1;
    pt.unk14 = 5;
    pt.area = (u8)p->area;
    pt.timer = 0;
    pt.flags = 1;
    gQuest.specialMinePos[0] = p->pos[0];
    gQuest.specialMinePos[1] = p->pos[1];
    gQuest.specialMinePos[2] = p->pos[2];
    gQuest.specialMineSlot = MinePoint_Add(&pt, gQuest.specialMinePos, 1);
    gSpecialMineAge = 0;
    gQuest.specialMineArea = p->area;
    gSpecialMinePoint = p->useItem;
    StageObj_K0E_CreateAt(gQuest.specialMinePos, 10);
    return 1;
}

/* Gathers the special mine point: gives item 0x2F7 and returns the reward for its rank (0xFFFF: none). */
s32 SpecialMine_Gather(Player *p)
{
    s32 i;
    s32 rank;
    Reward *reward;

    if (!Player_IsLocal(p)) {
        return 0xFFFF;
    }
    if (gSpecialMinePoint == 0) {
        return 0xFFFF;
    }
    if (Pouch_Space(p, 0x2F7) <= 0) {
        return 0xFFFF;
    }
    Pouch_Add(p, 0x2F7, 1);
    Notice_Push(1, 0, 0x2F7);
    rank = SpecialMine_Rank();
    for (i = 0;; i++) {
        if (gSpecialMinePoints[i] == 0xFFFF) {
            return 0xFFFF;
        }
        if (gSpecialMinePoint == gSpecialMinePoints[i]) {
            break;
        }
    }
    reward = gSpecialMineRewards[i];
    reward += rank;
    gSpecialMinePoint = 0;
    MinePoint_Remove(gQuest.specialMineSlot);
    return reward->item;
}

/* Rank of the special mine point from its age (gSpecialMineAge against gSpecialMineAgeRanks). */
s32 SpecialMine_Rank(void)
{
    s32 i;

    for (i = 0; i < 5; i++) {
        if (gSpecialMineAge < gSpecialMineAgeRanks[i]) {
            break;
        }
    }
    return i;
}

/* Clears the quest statistics (gQuestStats) and rolls gQuest.markMonster/gQuest.markHunt. */
void QuestStats_Init(void)
{
    s32 n;

    memset(gQuestStats, 0, 0x1C);
    for (n = 0; n < 0x49; n++) {
        if (gMarkMonsterCands[n] == 0) {
            break;
        }
    }
    gQuest.markMonster = gMarkMonsterCands[Quest_Random() % n];
    gQuest.markHunt = Quest_Random() & 0xF;
}

/* In a drill, adds n to statistic id (QSTAT_*; QSTAT_PART_BROKEN only
 * once) and shows the notice of some of them. */
void QuestStats_Add(s32 id, s32 n)
{
    if (Quest_IsDrill() == 0) {
        return;
    }
    switch (id) {
    case QSTAT_PART_BROKEN:
        if (gQuestStats[id] != 0) {
            return;
        }
        gQuestStats[id] = 1;
        break;
    default:
        gQuestStats[id] += (u8)n;
        break;
    }
    switch (id) {
    case QSTAT_PART_BROKEN:
    case QSTAT_HEAL:
    case QSTAT_CHARM:
    case 13:
    case 14:
    case 15:
        Notice_PushItem(gDrillScoreTexts[id]);
        break;
    }
}

/* At a drill's end: the quest time, the hunts by class (gEmTallyClass) into
 * the statistics, and the all-goals and charm marks. */
void QuestStats_Tally(void)
{
    s32 i;

    if (Quest_IsDrill() == 0) {
        return;
    }
    gQuest.clearTime = gQuestPartyClearTime;
    for (i = 0; i < 0x4A; i++) {
        u16 n = gQuest.hunted[i];

        if (n <= 0) {
            continue;
        }
        switch (gEmTallyClass[i]) {
        case 1:
            gQuestStats[QSTAT_HUNTED_FIRST] += n;
            break;
        case 2:
            gQuestStats[QSTAT_HUNTED_FIRST + 1] += n;
            break;
        case 3:
            gQuestStats[QSTAT_HUNTED_FIRST + 2] += n;
            break;
        case 4:
            gQuestStats[QSTAT_HUNTED_FIRST + 3] += n;
            break;
        case 5:
            gQuestStats[QSTAT_HUNTED_FIRST + 4] += n;
            break;
        case 6:
            gQuestStats[QSTAT_HUNTED_FIRST + 5] += n;
            break;
        }
    }
    if (QuestGoal_IsDone(0) == 1 && QuestGoal_IsDone(1) == 1 && QuestGoal_IsDone(2) == 1) {
        gQuest.stats[QSTAT_ALL_GOALS] = 1;
    }
    if (gQuest.drillCharm == DRILL_CHARM_TAKEN || gQuest.drillCharm == DRILL_CHARM_USED) {
        gQuest.stats[QSTAT_CHARM] = 1;
    }
}

/* Formats gQuest.penalty with gRewardCutFmt and shows it. */
void Quest_ShowValue(void)
{
    char buf[0x20];

    Quest_Sprintf(buf, gRewardCutFmt, gQuest.penalty);
    buf[0x1F] = 0;
    Notice_PushText(buf);
}

/* vsprintf into buf (vsprintf). */
void Quest_Sprintf(char *buf, char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
}

/* The quest monsters of area a hunted with appearances left come back
 * active, at full health (area entries of the current progress). */
void QuestEm_ResetArea(s32 a)
{
    QuestEm *q;

    if (Stage_FindEntryData((s32 *) gQuestEmAreaSets, a, 0, gQuest.progress) == 0) {
        return;
    }
    q = (QuestEm *)Stage_FindEntryData((s32 *) gQuestEmAreaSets, a, 1, gQuest.progress);
    for (;; q++) {
        if (q->kind <= 0) {
            break;
        }
        if (q->flags & QUEST_EM_HUNTED) {
            q->flags = QUEST_EM_ACTIVE;
            q->hp = -1;
        }
    }
}

/* Initialises all quest monster lists (numbering them from 0). */
void QuestEm_InitAll(void)
{
    s32 n = 0;

    QuestEm_InitSets(gQuestEmSets, &n);
    QuestEm_InitAreaSets(gQuestEmAreaSets, &n);
}

/* Rolls a monster size (percent) from the tables at gEmSizeRollTbls, clamped to the stage's range. */
u16 QuestEm_RollSize(void)
{
    u8 *tbl;
    u16 v;

    tbl = gEmSizeRollTbls[gEmSizeRollPicks[Quest_Random() & 0x1F]];
    v = tbl[Quest_Random() & 0x1F];
    if (v < gEmSizeRanges[gStageHeader->sizeRange].min) {
        v = gEmSizeRanges[gStageHeader->sizeRange].min;
    }
    if (v > gEmSizeRanges[gStageHeader->sizeRange].max) {
        v = gEmSizeRanges[gStageHeader->sizeRange].max;
    }
    return v;
}

/* Rolls the level of quest monster q (weighted table by kind), clamped to 0..16. */
void QuestEm_RollLevel(QuestEm *q)
{
    u16 sum;
    s8 *p;
    u16 r;
    u16 acc;
    s8 lv;

    if (gQuest.flags & QUEST_FLAG_FIXED_LEVEL) {
        q->level = gQuest.emLevel;
        return;
    }
    p = gEmLevelTables[gEmLevelTableIdx[q->kind][Daytime_Get()]];
    sum = 0;
    {
        s8 *e;

        for (e = p; *e != -1; e += 2) {
            sum += *e;
        }
    }
    r = Quest_Random() % sum;
    acc = 0;
    while (*p != -1) {
        if (r < (acc += *p++)) {
            break;
        }
        p++;
    }
    lv = *p + gEmLevelSeasonAdd[q->kind][(u8)Quest_GetSeason()];
    if (lv > 2) {
        lv = 2;
    }
    q->level = gQuest.emLevel + lv;
    if (q->level > 16) {
        q->level = 16;
    }
    if (q->level < 0) {
        q->level = 0;
    }
}

/* Initialises quest monster q with index *n and counts it. */
void QuestEm_Init(QuestEm *q, s32 *n)
{
    q->flags = QUEST_EM_ACTIVE;
    q->hp = -1;
    q->emIndex = -1;
    q->index = *n;
    QuestEm_RollLevel(q);
    if ((q->large = gEmIsLarge2[q->kind]) != 0) {
        gQuest.emLeft += q->count;
    }
    (*n)++;
}

/* Returns 1 for the monster kinds whose size is rolled. */
s32 Em_HasRandomSize(s16 id)
{
    switch (id) {
    case 0x1:
    case 0x2A:
    case 0x25:
    case 0x6:
    case 0x26:
    case 0x8:
    case 0xB:
    case 0x29:
    case 0x31:
    case 0xE:
    case 0x2B:
    case 0xF:
    case 0x2D:
    case 0x11:
    case 0x2F:
    case 0x14:
    case 0x27:
    case 0x15:
    case 0x2E:
    case 0x16:
    case 0x1A:
    case 0x2C:
    case 0x1B:
    case 0x1C:
    case 0x1F:
    case 0x21:
    case 0x44:
    case 0x28:
    case 0x30:
    case 0x43:
    case 0x34:
    case 0x33:
    case 0x35:
    case 0x36:
    case 0x3C:
    case 0x3B:
    case 0x40:
    case 0x41:
        return 1;
    }
    return 0;
}

/* Same list as Em_HasRandomSize. */
s32 Em_HasRandomSize2(s16 id)
{
    switch (id) {
    case 0x1:
    case 0x2A:
    case 0x25:
    case 0x6:
    case 0x26:
    case 0x8:
    case 0xB:
    case 0x29:
    case 0x31:
    case 0xE:
    case 0x2B:
    case 0xF:
    case 0x2D:
    case 0x11:
    case 0x2F:
    case 0x14:
    case 0x27:
    case 0x15:
    case 0x2E:
    case 0x16:
    case 0x1A:
    case 0x2C:
    case 0x1B:
    case 0x1C:
    case 0x1F:
    case 0x21:
    case 0x44:
    case 0x28:
    case 0x30:
    case 0x43:
    case 0x34:
    case 0x33:
    case 0x35:
    case 0x36:
    case 0x3C:
    case 0x3B:
    case 0x40:
    case 0x41:
        return 1;
    }
    return 0;
}

/* Initialises the monster sets at set (0x10 bytes each, until one is empty) and rolls their sizes. */
void QuestEm_InitSets(u8 *set, s32 *n)
{
    s32 count;
    QuestEm *q;

    for (;; set += 0x10) {
        count = (s32)StageEntry_GetData((struct StageEntry *) set, 0);
        q = (QuestEm *)StageEntry_GetData((struct StageEntry *) set, 1);
        if (count == 0) {
            break;
        }
        if (q == NULL) {
            continue;
        }
        for (;; q++) {
            if (q->kind <= 0) {
                break;
            }
            QuestEm_Init(q, n);
            if (Em_HasRandomSize(q->kind) == 1) {
                q->size = gStageHeader->emSize;
                q->size = q->size * QuestEm_RollSize() / 100;
                q->size += gEmSizeSeasonAdd[q->kind][(u8)Quest_GetSeason()];
            } else {
                q->size = 100;
            }
        }
    }
}

/* Initialises the monster sets of each area list (size 100). */
void QuestEm_InitAreaSets(u8 **list, s32 *n)
{
    QuestEm *q;
    s32 count;
    u8 *set;

    for (;; list++) {
        if (*list == NULL) {
            break;
        }
        if (gQuest.stage == 0) {
            set = *list;
        } else {
            set = *list + (s32)gStageFile;
        }
        for (;;) {
            count = (s32)StageEntry_GetData((struct StageEntry *) set, 0);
            q = (QuestEm *)StageEntry_GetData((struct StageEntry *) set, 1);
            set += 0x10;
            if (count == 0) {
                break;
            }
            if (q == NULL) {
                continue;
            }
            for (;; q++) {
                if (q->kind <= 0) {
                    break;
                }
                QuestEm_Init(q, n);
                q->size = 100;
            }
        }
    }
}

/* In versus, writes the two sides' monsters (gNextQuest.teams) into the
 * stage's monster list; then loads the models of the stage-wide kinds
 * (gScene.emKind) and spawns the stage-wide monsters. */
void QuestEm_SpawnAll(void)
{
    s32 j;
    QuestEm *q;
    s32 *ids;
    s32 i;
    void *em;

    ids = (s32 *)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 0);
    q = (QuestEm *)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 1);
    if (Quest_IsVersus() == 1) {
        for (i = 0; i < 2; i++) {
            if (gNextQuest.teams[i].emKind != 0) {
                ids[i] = gNextQuest.teams[i].emKind;
                q[i].kind = gNextQuest.teams[i].emKind;
                q[i].count = 1;
                q[i].rotY = gVersusEmAngles[i];
                q[i].x = gVersusEmPos[i][0];
                q[i].y = gVersusEmPos[i][1];
                q[i].z = gVersusEmPos[i][2];
            }
        }
    }
    if (ids != NULL && q != NULL) {
        while (1) {
            if (*ids <= 0) {
                break;
            }
            for (i = 0; i < 4; i++) {
                if (gScene.emKind[i] == *ids) {
                    break;
                }
            }
            if (i < 4) {
                ids++;
                continue;
            }
            for (j = 0; j < 4; j++) {
                if (gScene.emKind[j] <= 0) {
                    gScene.emKind[j] = *ids;
                    GmLoad_LoadEmData(gScene.map, gScene.emKind[j]);
                    EmSlot_LoadModels(j);
                    break;
                }
            }
            ids++;
        }
        for (;; q++) {
            if (q->kind <= 0) {
                break;
            }
            if ((em = QuestEm_Spawn(q)) != NULL && Quest_IsVersus() == 1) {
                Versus_InitEm(em);
            }
        }
    }
}

/* Finds the quest monster record of em (by Em.index); NULL when none. */
QuestEm *QuestEm_FindForEm(Em *em)
{
    s32 i;
    s32 count;
    QuestEm *q;

    for (i = 0; i <= gQuest.progress; i++) {
        count = (s32)Stage_FindEntryData((s32 *) gQuestEmAreaSets, em->area, 0, i);
        q = (QuestEm *)Stage_FindEntryData((s32 *) gQuestEmAreaSets, em->area, 1, i);
        if (count == 0) {
            continue;
        }
        for (;; q++) {
            if (q->kind <= 0) {
                break;
            }
            if (em->index == q->emIndex) {
                return q;
            }
        }
    }
    count = (s32)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 0);
    q = (QuestEm *)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 1);
    if (count == 0) {
        return NULL;
    }
    if (q == NULL) {
        return NULL;
    }
    for (;; q++) {
        if (q->kind <= 0) {
            break;
        }
        if (em->index == q->emIndex) {
            return q;
        }
    }
    return NULL;
}

/* Finds the quest monster with index idx in area; NULL when none. */
QuestEm *QuestEm_FindByIndex(s16 idx, u16 area)
{
    s32 i;
    s32 count;
    QuestEm *q;

    for (i = 0; i <= gQuest.progress; i++) {
        count = (s32)Stage_FindEntryData((s32 *) gQuestEmAreaSets, area, 0, i);
        q = (QuestEm *)Stage_FindEntryData((s32 *) gQuestEmAreaSets, area, 1, i);
        if (count == 0) {
            continue;
        }
        for (;; q++) {
            if (q->kind <= 0) {
                break;
            }
            if (idx == q->index) {
                return q;
            }
        }
    }
    count = (s32)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 0);
    q = (QuestEm *)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 1);
    if (count == 0) {
        return NULL;
    }
    if (q == NULL) {
        return NULL;
    }
    for (;; q++) {
        if (q->kind <= 0) {
            break;
        }
        if (idx == q->index) {
            return q;
        }
    }
    return NULL;
}

/* Counts the quest timers by dt (not while paused). */
void Quest_UpdateTimers(s32 dt)
{
    if (gScene.eventRunning == 0 && gQuest.inputLock <= 0) {
        if (gQuestTimeLeft > 0) {
            gQuestTimeLeft -= dt;
            if (gQuestTimeLeft < 0) {
                gQuestTimeLeft = 0;
            }
        }
        if (gSpecialMinePoint != 0) {
            gSpecialMineAge += dt;
            if (gSpecialMineAge > 90000) {
                gSpecialMineAge = 90000;
            }
        }
        if (gQuestCountdown > 0) {
            gQuestCountdown -= dt;
            if (gQuestCountdown < 0) {
                gQuestCountdown = 0;
            }
        }
    }
}

/* Restarts the quest timer. */
void Quest_ResetTimer(void)
{
    gQuestTimeLeft = Quest_GetTime(1);
}

/* Clears the three objectives and their copies. */
void QuestGoal_ClearAll(void)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        gQuest.goals[i].target = 0;
        gQuest.goals2[i].target = 0;
        gQuest.goals[i].count = 0;
        gQuest.goals2[i].count = 0;
        gQuest.goals[i].flags = 0;
        gQuest.goals2[i].flags = 0;
    }
}

/* Sets the objectives from the quest definition. */
void QuestGoal_InitFromDef(void)
{
    QuestGoal *g;
    s32 i;

    g = gQuestInfo->goals;
    gQuest.clearType = gQuestInfo->clearType;
    for (i = 0; i < 3; i++, g++) {
        if (g->flags != 0) {
            QuestGoal_Add(g->flags, (s16)g->target, g->count);
        }
        gQuest.goalIdx[i] = 0xFF;
    }
}

/* Adds an objective in the first free slot. */
void QuestGoal_Add(s32 flags, s16 target, s16 count)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (gQuest.goals[i].flags == 0) {
            break;
        }
    }
    if (i < 3) {
        gQuest.goals2[i].flags = gQuest.goals[i].flags = flags | 0x10;
        gQuest.goals2[i].target = gQuest.goals[i].target = target;
        gQuest.goals2[i].count = gQuest.goals[i].count = count;
    }
}

/* Marks objective i done (reason flag) and shows the messages. */
void QuestGoal_Complete(u16 i, u32 flag)
{
    s32 j;

    if (QuestGoal_IsDone(i) == 1) {
        return;
    }
    gQuest.goals[i].flags |= 0x20;
    if (Quest_IsVersus() == 1) {
        return;
    }
    for (j = 0; j < 3; j++) {
        if (gQuest.goalIdx[j] == 0xFF) {
            gQuest.goalIdx[j] = i;
            break;
        }
    }
    if (flag == 0x200) {
        MsgEvent_Check(0x11);
    }
    if (flag == 0x100) {
        MsgEvent_Check(0x12);
    }
    if (flag == 2) {
        MsgEvent_Check(0xF);
    }
    switch (gQuest.clearType) {
    case 1:
        Notice_PushItem(gMustOrderAllText[0]);
        break;
    case 2:
        if (Quest_IsMustOrder() == 1) {
            Notice_PushItem(gMustOrderAllText[0]);
        } else {
            Notice_PushItem(gGoalDoneTexts[(u32)i]);
        }
        break;
    case 3:
    case 4:
        if (Quest_CheckClear(0) == 1) {
            Notice_PushItem(gMustOrderAllText[0]);
        } else {
            Notice_PushItem(gMustOrderOneText);
        }
        break;
    }
    if (Quest_CheckClear(0) == 0) {
        u16 n = i;

        if (n != 0) {
            s32 end;
            s32 k;

            for (k = (n - 1) * 8 + 0x18, end = k + 8; k < end; k++) {
                if (gScene.supply[k].item != 0) {
                    if (Quest_IsMustOrder() == 1) {
                        Notice_PushItem(gQuestNoticeTexts[3]);
                    } else if (n == 1) {
                        Notice_PushItem(D_0044557C);
                    } else {
                        Notice_PushItem(gSubBSupplyText[0]);
                    }
                    break;
                }
            }
        }
        Bgm_PlayEvent(0xB7);
    }
    gQuest.msgArea = i;
    gQuest.msgArg0 = 0;
    gQuest.msgType = 0x10;
    gQuest.msgArg1 = flag >> 16;
    gQuest.msgArg2 = flag;
    QSync_SendState(6, gScene.localPlayer);
}

/* QuestGoal_Complete while the quest is running. */
void QuestGoal_CompleteIfRunning(u16 a, s32 b)
{
    if (Quest_IsRunning(1) == 0) {
        QuestGoal_Complete(a, b);
    }
}

/* Checks the clear condition; notify 1 also announces it. Returns 1 when cleared. */
s32 Quest_CheckClear(s32 notify)
{
    s32 i;
    s32 done = 0;
    s32 unused = 0;

    for (i = 0; i < 3; i++) {
        if (gQuest.goals[i].flags & 0x10) {
            if (gQuest.goals[i].flags & 0x20) {
                done |= 1 << i;
            }
            if ((gQuest.goals[i].flags & 0x440) == 0x40) {
                gQuest.goals[i].flags |= 0x400;
                if (notify == 1) {
                    Quest_JumpToEnd(0);
                }
                return 0;
            }
        } else {
            unused |= 1 << i;
        }
    }
    if (Quest_IsVersus() == 1) {
        return 0;
    }
    switch (gQuest.clearType) {
    case 1:
        if (done) {
            if (notify == 1) {
                Quest_JumpToStart(0);
            }
            return 1;
        }
        break;
    case 2:
        if ((done & 1) == 1) {
            if (notify == 1) {
                Quest_JumpToStart(0);
            }
            return 1;
        }
        break;
    case 3:
        if ((done & 3) == 3) {
            if (notify == 1) {
                Quest_JumpToStart(0);
            }
            return 1;
        }
        break;
    case 4:
        if ((done | unused) == 7) {
            if (notify == 1) {
                Quest_JumpToStart(0);
            }
            return 1;
        }
        break;
    }
    return 0;
}

/* Returns 1 when the sub-objectives needed by the clear type are done. */
s32 Quest_CheckSubClear(void)
{
    s32 i;
    s32 done = 0;

    for (i = 0; i < 3; i++) {
        if (gQuest.goals[i].flags & 0x10) {
            if (gQuest.goals[i].flags & 0x20) {
                done |= 1 << i;
            }
            if ((gQuest.goals[i].flags & 0x440) == 0x40) {
                gQuest.goals[i].flags |= 0x400;
                Quest_JumpToEnd(0);
                return 0;
            }
        }
    }
    switch (gQuest.clearType) {
    case 1:
        break;
    case 2:
        if (done & 6) {
            return 1;
        }
        break;
    case 3:
        if (done & 4) {
            return 1;
        }
        break;
    case 4:
        break;
    }
    return 0;
}

/* Quest_CheckSubClear, 0 in some modes (Quest_IsDrill). */
s32 Quest_CheckSubClearMode(void)
{
    if (Quest_IsDrill() == 1) {
        return 0;
    }
    return Quest_CheckSubClear();
}

/* Returns 1 when objective i is done (and not failed). */
s32 QuestGoal_IsDone(s32 i)
{
    s32 flags = gQuest.goals[i].flags;

    if (flags & 0x10) {
        if ((flags & 0x440) == 0x40) {
            return 0;
        }
        if (flags & 0x20) {
            return 1;
        }
    }
    return 0;
}

/* Returns 1 when objective i is in use. */
s32 QuestGoal_IsUsed(s32 i)
{
    return (gQuest.goals[i].flags & 0x10) != 0;
}

/* Returns 1 when all sub-objectives in use are done. */
s32 QuestGoal_AllSubDone(void)
{
    s32 i;

    for (i = 1; i < 3; i++) {
        if (QuestGoal_IsUsed(i) == 1 && QuestGoal_IsDone(i) == 0) {
            return 0;
        }
    }
    return 1;
}

/* Returns 1 when monster kind target is a hunt target that counts for the clear type. */
s32 QuestGoal_IsClearTarget(u16 target)
{
    s32 i;
    u8 type = gQuest.clearType;

    for (i = 0; i < 3; i++) {
        if (((gQuest.goals[i].flags & 0x31) == 0x31 || (gQuest.goals[i].flags & 0x34) == 0x34) && gQuest.goals[i].target == target) {
            switch (type) {
            case 1:
                return 1;
            case 2:
                if (i == 0) {
                    return 1;
                }
                break;
            case 3:
                if (i == 0 || i == 1) {
                    return 1;
                }
                break;
            case 4:
                return 1;
            }
        }
    }
    return 0;
}

/* Returns 1 when objective i is a hunt. */
static s32 QuestGoal_IsHunt(u16 i)
{
    return (gQuest.goals[i].flags & 0x31) == 0x31;
}

/* Count left for the hunt of kind target; -1 when none. */
s16 QuestGoal_HuntCountFor(u16 target)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if ((gQuest.goals[i].flags & 0x11) == 0x11 && gQuest.goals[i].target == target) {
            return gQuest.goals[i].count;
        }
    }
    return -1;
}

/* Count of hunt objective i; -1 when not a hunt. */
static s32 QuestGoal_HuntCount(u16 i)
{
    return (gQuest.goals[i].flags & 0x11) == 0x11 ? gQuest.goals[i].count : -1;
}

/* Count of delivery objective i; -1 when not a delivery. */
s32 QuestGoal_DeliverCount(u16 i)
{
    return (gQuest.goals2[i].flags & 0x12) == 0x12 ? gQuest.goals2[i].count : -1;
}

/* Count of delivery objective i for target; -1 otherwise. */
s32 QuestGoal_DeliverCountFor(u16 target, u16 i)
{
    if ((gQuest.goals2[i].flags & 0x12) == 0x12 && gQuest.goals2[i].target == target) {
        return gQuest.goals2[i].count;
    }
    return -1;
}

/* Item of delivery objective i; 0 when not a delivery. */
u16 QuestGoal_DeliverItem(u16 i)
{
    if ((gQuest.goals2[i].flags & 0x12) == 0x12) {
        return gQuest.goals2[i].target;
    }
    return 0;
}

/* Flags mask of objective copy i. */
s32 QuestGoal_CopyFlags(u16 i, s32 mask)
{
    return gQuest.goals2[i].flags & mask;
}

/* Number of item delivered (by the local player for flag 0x80, else by all players). */
static s32 Quest_DeliveredCount(u16 item)
{
    Player *p;
    s32 i;
    QuestGoal *g;
    s16 n = 0;
    s32 k;

    for (g = gQuest.goals, i = 0; i < 3; i++, g++) {
        if ((g->flags & 0x12) == 0x12 && g->target == item) {
            break;
        }
    }
    if (i >= 3) {
        return 0;
    }
    if (g->flags & 0x80) {
        p = &gPlayers[gScene.localPlayer];
        for (i = 0; i < 4; i++) {
            if (p->delivered[i].item == item) {
                return p->delivered[i].count;
            }
        }
    } else {
        p = gPlayers;
        for (k = 0; k < gScene.playerCount; k++, p++) {
            for (i = 0; i < 4; i++) {
                if (p->delivered[i].item == item) {
                    n += p->delivered[i].count;
                }
            }
        }
    }
    return n;
}

/* Per-frame objective checks: deliveries, broken parts and damage. */
void QuestGoal_Update(void)
{
    s32 i;
    Em *em;
    s32 half;

    for (i = 0; i < 3; i++) {
        if ((gQuest.goals[i].flags & 0x32) == 0x12) {
            if ((gQuest.goals[i].count = QuestGoal_DeliverCountFor(gQuest.goals[i].target, i) - Quest_DeliveredCount(gQuest.goals[i].target)) <= 0) {
                QuestGoal_CompleteIfRunning(i, 2);
            }
        }
    }
    for (i = 0; i < 3; i++) {
        if ((gQuest.goals[i].flags & 0x4034) == 0x4014 &&
            Quest_IsPartBroken(gQuest.goals[i].target, gQuest.goals[i].count) == 1) {
            QuestGoal_CompleteIfRunning(i, 0x4000);
        }
    }
    for (i = 0; i < 3; i++) {
        s32 flags = gQuest.goals[i].flags;

        if ((flags & 0x8034) != 0x8014) {
            continue;
        }
        em = gQuest.spawnedEm;
        if (em == NULL) {
            continue;
        }
        if (flags & 0x10000) {
            half = Quest_GetTime(1) / 2;
            if (Quest_GetTime(0) > half) {
                continue;
            }
        }
        if (gQuest.savedEmHp - em->hp < gQuest.goals[i].count * 100) {
            continue;
        }
        if (gQuest.goals[i].flags & 0x10000) {
            Notice_PushItem(gElderFledText[0]);
        }
        QuestGoal_CompleteIfRunning(i, 0x8000);
    }
}

/* Counts a capture of kind target; returns 1 when an objective is completed. */
s32 QuestGoal_OnCapture(u16 target)
{
    s32 i;
    s32 ret = 0;

    for (i = 0; i < 3; i++) {
        s32 flags = gQuest.goals[i].flags;

        if ((flags & 0x60) == 0 && (flags & 0x11) == 0x11 && gQuest.goals[i].target == target) {
            if (flags & 0x200) {
                gQuest.goals[i].flags |= 0x40;
                return 0;
            }
            if (--gQuest.goals[i].count <= 0) {
                QuestGoal_CompleteIfRunning(i, 0x100);
                ret = 1;
            }
        }
    }
    return ret;
}

/* Returns 1 when a hunt objective has flag 0x100. */
s32 QuestGoal_HasFlag100(void)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if ((gQuest.goals[i].flags & 0x100) && (gQuest.goals[i].flags & 0x31) == 0x31) {
            return 1;
        }
    }
    return 0;
}

/* Checks the part objectives of kind id (bit masks gQuest.parts). */
/* Em_BreakPart and Em_BreakParts pass the part (or the bits) too. */
s32 QuestGoal_OnPartBroken(u16 id, u16 part)
{
    s32 i;
    s32 ret = 0;

    for (i = 0; i < 3; i++) {
        s32 flags = gQuest.goals[i].flags;

        if ((flags & 0x60) == 0 && (flags & 0x4014) == 0x4014 && gQuest.goals[i].target == id &&
            (gQuest.parts[id] & (1 << gQuest.goals[i].count))) {
            QuestGoal_CompleteIfRunning(i, 0x4000);
            ret = 1;
        }
    }
    return ret;
}

/* Counts a kill of kind target; returns 1 when an objective is completed. */
s32 QuestGoal_OnKill(u16 target)
{
    s32 i;
    s32 ret = 0;

    for (i = 0; i < 3; i++) {
        s32 flags = gQuest.goals[i].flags;

        if (flags & 0x60) {
            continue;
        }
        if ((flags & 0x11) == 0x11 && gQuest.goals[i].target == target) {
            if (flags & 0x100) {
                gQuest.goals[i].flags |= 0x40;
                return 0;
            }
            if (--gQuest.goals[i].count <= 0) {
                QuestGoal_CompleteIfRunning(i, 0x200);
                ret = 1;
            }
        }
        if ((gQuest.goals[i].flags & 0x8014) == 0x8014 && gQuest.goals[i].target == target) {
            QuestGoal_CompleteIfRunning(i, 0x8000);
            ret = 1;
        }
    }
    return ret;
}

/* Counts a kill for objective i only. */
s32 QuestGoal_OnKillSlot(u16 target, u16 i)
{
    if ((gQuest.goals[i].flags & 0x60) == 0 && (gQuest.goals[i].flags & 0x11) == 0x11 && gQuest.goals[i].target == target) {
        if (gQuest.goals[i].flags & 0x100) {
            gQuest.goals[i].flags |= 0x40;
            return 0;
        }
        if (--gQuest.goals[i].count <= 0) {
            QuestGoal_CompleteIfRunning(i, 0x200);
            return 1;
        }
        return 0;
    }
    return 0;
}

typedef struct QuestTimeMsg {
    s32 time;
    s32 unk4;
} QuestTimeMsg;
extern QuestTimeMsg gQuestTimeNotices[];
extern char *gTimeLeftMinFmt[];
extern char *gQuestNoticeTexts[];
extern char *D_00445564;
extern char *D_00445568;
extern s8 D_006ADB2E;
void Quest_SendTime(void);
u8 Quest_FindSpecialMonster(u8 type);
void Quest_StoreElapsed(void);
s32 Quest_IsTraining(void);
void SeasonBanner_Create(void);
void Quest_TargetMessage(void);
u8 EmList_AnyBelowHalfHp(void); /* unit's own declaration: the definition (game) returns s32; Quest_StepScript masks the result with andi 0xFF (7 words with this one, 17 with s32) */
void Versus_End(s8 side);

/*
 * Steps the quest script (gQuest.script, QUEST_OP_* in quest.h) from
 * command pc until a command waits, and returns where it stopped (or the
 * QUEST_OP_MARK position). Before the quest runs it also counts the time
 * down, with the time-left notices and a QMSG_TIME_LEFT every 5 minutes;
 * when the time is up it jumps to the time-out label. A goal with flag
 * 0x100 (goalLabel) or a triggered event (eventLabel) jumps to its label.
 * QUEST_OP_NOTICE's text pointer is a local advanced by the index, the
 * form that gives the game's register order.
 */
s16 Quest_StepScript(Player *pl, s16 pc, s32 *out)
{
    char buf[32];
    QuestCmd *e;
    s16 mark;
    Em *em;
    s16 lbl;

    mark = -1;
    if (gScene.questPhase == QUEST_PHASE_READY&& gQuestTimeLeft >= 0) {
        if (gQuest.timeNoticeIdx > 0 && gQuestTimeNotices[gQuest.timeNoticeIdx].time >= gQuestTimeLeft) {
            Quest_Sprintf(buf, gTimeLeftMinFmt[0], gQuestTimeNotices[gQuest.timeNoticeIdx].time / 1800 /* frames per minute */);
            buf[31] = 0;
            Notice_PushText(buf);
            gQuest.timeNoticeIdx--;
        }
        Quest_UpdateTimers(1);
        if (gQuestTimeLeft <= 0) {
            gQuestTimeLeft = 0;
            e = ((QuestCmd *)gQuest.script);
            pc = 0;
            Quest_SendTime();
            gQuestTimeLeft = -1;
            for (;;) {
                if (e->op == QUEST_OP_LABEL && gQuest.timeoutLabel == e->arg) {
                    pc++;
                    break;
                }
                pc++;
                e++;
            }
        } else if (gQuestTimeLeft % QUEST_TIME_NOTICE_STEP == 0) {
            Quest_SendTime();
        }
    }
    for (;;) {
        if (gQuest.goalLabel != 0 && QuestGoal_HasFlag100() == 1) {
            pc = 0;
            for (e = ((QuestCmd *)gQuest.script);; e++) {
                if (e->op == QUEST_OP_LABEL && gQuest.goalLabel == e->arg) {
                    pc++;
                    break;
                }
                pc++;
            }
            gQuest.goalLabel = 0;
        }
        if (gQuest.eventLabel != 0 && gQuest.eventTriggered != 0) {
            lbl = gQuest.eventLabel;
            e = ((QuestCmd *)gQuest.script);
            pc = 0;
            for (;;) {
                if (e->op == QUEST_OP_LABEL && lbl == e->arg) {
                    pc++;
                    break;
                }
                pc++;
                e++;
            }
            gQuest.eventLabel = 0;
        }
        e = &((QuestCmd *)gQuest.script)[pc];
        switch (e->op) {
        case QUEST_OP_CLEAR_TYPE:
            gQuest.clearType = e->arg;
            pc++;
            continue;
        case QUEST_OP_ADD_GOAL:
            QuestGoal_Add(e->arg, e->arg2, e->arg3);
            pc++;
            continue;
        case QUEST_OP_HALT:
            break;
        case QUEST_OP_SET_WAIT:
            gQuest.scriptWait = e->arg;
            pc++;
            break;
        case QUEST_OP_WAIT:
            if (--gQuest.scriptWait <= 0) {
                pc++;
            }
            break;
        case QUEST_OP_NOTICE: {
            s32 *ofs = gQuest.noticeTexts;

            ofs += e->arg;
            Notice_PushItem((char *)(*ofs + gSysSettings.stageFile));
            pc++;
            break;
        }
        case QUEST_OP_NOP:
            pc++;
            continue;
        case QUEST_OP_IF_CLEAR:
            if (Quest_CheckClear(0) == 1 || Quest_CheckSubClear() == 1) {
                pc++;
                continue;
            }
            pc++;
            e++;
            for (;;) {
                if (e->op == QUEST_OP_ENDIF) {
                    pc++;
                    break;
                }
                pc++;
                e++;
            }
            continue;
        case QUEST_OP_WAIT_HUNTED:
            if (QuestGoal_HuntCountFor(e->arg) <= e->arg2) {
                pc++;
                continue;
            }
            break;
        case QUEST_OP_WAIT_AREA:
            if (pl->area == e->arg) {
                pc++;
                continue;
            }
            break;
        case QUEST_OP_ADD_TIME:
            gQuestTimeLeft += (u16)e->arg;
            pc++;
            break;
        case QUEST_OP_START:
            em = gQuest.spawnedEm;
            if (em != NULL && Quest_FindSpecialMonster(em->type) != 0xFF) {
                gQuest.savedEmHp = em->hp;
                gQuest.parts[em->type] = em->partsBroken;
            }
            gQuest.msgArg0 = 0;
            gQuest.msgArg1 = 0;
            gSysSettings.clearCapture = 0x81;
            gQuest.result = QUEST_PHASE_RUNNING;
            pc++;
            gScene.questPhase = QUEST_PHASE_RUNNING;
            gQuest.msgArg2 = 0;
            gQuest.acks |= 1 << (gScene.localPlayer + QUEST_ACK_STARTED_SHIFT);
            gQuest.msgType = QMSG_STARTED;
            QSync_SendState(6, gScene.localPlayer);
            continue;
        case QUEST_OP_WAIT_ALL_START:
            if ((gQuest.acks & QUEST_ACKS_STARTED) == QUEST_ACKS_STARTED || gScene.inQuest[gScene.localPlayer] != 1) {
                pc++;
            }
            break;
        case QUEST_OP_CLEARED:
            gQuest.step = 0;
            pc = -1;
            (*(s8 *)&gQuest)++; /* gQuest.state, to QSTATE_CLEAR */
            break;
        case QUEST_OP_LOOP:
            pc = gQuest.loopPc;
            gQuest.scriptPc = pc;
            continue;
        case QUEST_OP_WAIT_STATE4:
            if (Player_IsState(pl, 4, 0) != 0) {
                pc++;
                continue;
            }
            break;
        case QUEST_OP_END:
            gScene.questPhase = QUEST_PHASE_ENDED;
            gQuest.result = QUEST_PHASE_ENDED;
            gQuest.msgArg0 = 0;
            pc++;
            gQuest.msgArg1 = 0;
            gQuest.msgArg2 = 0;
            gQuest.msgType = QMSG_ENDED;
            QSync_SendState(6, gScene.localPlayer);
            continue;
        case QUEST_OP_FAIL:
            gQuest.result = QUEST_PHASE_FAILED;
            pc = -1;
            gScene.questPhase = QUEST_PHASE_FAILED;
            Quest_StoreElapsed();
            if (Quest_IsVersus() == 0) {
                Notice_PushItem(gQuestNoticeTexts[0]);
            }
            if (Quest_IsVersus() == 0 && Quest_IsDrill() == 0 && Quest_IsTraining() == 0) {
                Notice_Push(0, 0xF, 0);
            }
            break;
        case QUEST_OP_BEGIN:
            gQuest.timeoutLabel = e->arg;
            Quest_Sprintf(buf, gQuestNoticeTexts[1], Quest_GetTime(QUEST_TIME_LIMIT) / 1800 /* frames per minute */);
            buf[31] = 0;
            Notice_PushText(buf);
            if (Quest_IsVersus() == 0) {
                SeasonBanner_Create();
            }
            pc++;
            break;
        case QUEST_OP_PROGRESS:
            gQuest.progress = (u16)e->arg;
            Quest_TargetMessage();
            pc++;
            continue;
        case QUEST_OP_WAIT_ITEMS:
            if (Cond_HasItems(&gQuest, (u16 *)e) != 0) {
                pc++;
                continue;
            }
            break;
        case QUEST_OP_GOAL_LABEL:
            gQuest.goalLabel = e->arg;
            pc++;
            break;
        case QUEST_OP_EVENT_LABEL:
            gQuest.eventLabel = e->arg;
            pc++;
            break;
        case QUEST_OP_END_ALT:
            gScene.questPhase = QUEST_PHASE_ENDED;
            pc++;
            gQuest.result = QUEST_PHASE_ENDED;
            Notice_PushItem(gQuestNoticeTexts[2]);
            gQuest.msgArg0 = 0;
            gQuest.msgType = QMSG_ENDED_ALT;
            gQuest.msgArg1 = 0;
            gQuest.msgArg2 = 0;
            QSync_SendState(6, gScene.localPlayer);
            continue;
        case QUEST_OP_START_ALT:
            em = gQuest.spawnedEm;
            if (em != NULL && Quest_FindSpecialMonster(em->type) != 0xFF) {
                gQuest.savedEmHp = em->hp;
                gQuest.parts[em->type] = em->partsBroken;
            }
            gQuest.msgArg0 = 0;
            gQuest.result = QUEST_PHASE_RUNNING;
            pc++;
            gQuest.msgArg1 = 0;
            gScene.questPhase = QUEST_PHASE_RUNNING;
            gQuest.msgArg2 = 0;
            gQuest.acks |= 1 << (gScene.localPlayer + QUEST_ACK_STARTED_SHIFT);
            gQuest.msgType = QMSG_STARTED_ALT;
            QSync_SendState(6, gScene.localPlayer);
            continue;
        case QUEST_OP_IF_EM_HURT:
            if (gQuest.spawnedEm->hp <= gQuest.savedEmHp - e->arg) {
                gQuest.savedEmHp = gQuest.spawnedEm->hp;
                pc++;
                continue;
            }
            pc++;
            e++;
            for (;;) {
                if (e->op == QUEST_OP_ENDIF_EM_HURT) {
                    pc++;
                    break;
                }
                pc++;
                e++;
            }
            continue;
        case QUEST_OP_IF_EM_WEAK:
            if (EmList_AnyBelowHalfHp() == 1) {
                QuestGoal_CompleteIfRunning(0, 0x200);
                pc++;
                continue;
            }
            pc++;
            e++;
            for (;;) {
                if (e->op == QUEST_OP_ENDIF_EM_WEAK) {
                    pc++;
                    break;
                }
                pc++;
                e++;
            }
            continue;
        case QUEST_OP_GOTO:
            lbl = e->arg;
            e = ((QuestCmd *)gQuest.script);
            pc = 0;
            for (;;) {
                if (e->op == QUEST_OP_LABEL && lbl == e->arg) {
                    pc++;
                    break;
                }
                pc++;
                e++;
            }
            continue;
        case QUEST_OP_HALT2:
            break;
        case QUEST_OP_LABEL:
            pc++;
            continue;
        case QUEST_OP_TIMEOUT_LABEL:
            gQuest.timeoutLabel = e->arg;
            pc++;
            break;
        case QUEST_OP_IF_HUNTED_GOTO:
            if (QuestGoal_HuntCountFor(e->arg) <= e->arg2) {
                lbl = e->arg3;
                e = ((QuestCmd *)gQuest.script);
                pc = 0;
                for (;;) {
                    if (e->op == QUEST_OP_LABEL && lbl == e->arg) {
                        pc++;
                        break;
                    }
                    pc++;
                    e++;
                }
                continue;
            }
            pc++;
            continue;
        case QUEST_OP_MARK:
            mark = pc;
            pc++;
            continue;
        case QUEST_OP_UNMARK:
            mark = -1;
            pc++;
            continue;
        case QUEST_OP_VERSUS_END:
            if (gNextQuest.unk5E == 0) {
                if (QuestGoal_IsHunt(0) == 1) {
                    Versus_End(2);
                    pc = -1;
                } else if (QuestGoal_IsHunt(1) == 1) {
                    Versus_End(1);
                    pc = -1;
                }
            } else if (gQuest.sideEmLeft[0] <= 0) {
                Versus_End(2);
                pc = -1;
            } else if (gQuest.sideEmLeft[1] <= 0) {
                Versus_End(1);
                pc = -1;
            }
            break;
        case QUEST_OP_COMPLETE_GOAL:
            QuestGoal_CompleteIfRunning(0, 0);
            pc++;
            continue;
        }
        break;
    }
    if (gSysSettings.clearCapture == 0x80) {
        gSysSettings.clearCapture = 0;
        if (QuestGoal_IsDone(0) == 1 && Quest_IsRunning(0) != 0) {
            CamReq_TargetDown();
        }
    }
    if (mark != -1) {
        pc = mark;
    }
    return pc;
}

extern s32 gDrillPayoutThresholds[];
extern u8 gPartRewardRecs[];
extern u16 *gRewardItemTables[];
u16 Dojo_GetParMinutes(u16 id);

/* The quest's rewards into gScene.supply: 24 main, 4 and 4 for the subquests, 16 for broken
 * parts (sorted by item within each group). */
/* Matched in round 29. Locals are reused as the game's unit seems to have: one each for sum and
 * acc, the two supply counts n and m; got also counts the part rewards and the sorts; i clears,
 * holds the skill level and sorts. k holds a goal's count in the range cases and then counts the
 * drop rolls: as a later web of k, the counter is numbered after the walkers MWCC creates and takes
 * s0, while the drop table pointer lim, whose first web this is (the range cases have their own
 * range pointer), is numbered as declared. The function is compiled at level 3 in this level-4
 * file, as its code shows: the drop table loop loads its first byte through the address register
 * it keeps (lui s5; addiu s5; lbu a0,0(s5)), where level 4 loads it again through its own
 * lui at / lbu %lo(at) and forms s5 later, in the branch delay slot (9 words differ at level 4). */
s32 func_D6ABF0(void); /* plsel's func_00D6ABF0 */
#pragma optimization_level 3
void Quest_RollRewards(void)
{
    ItemStack *t;
    u16 r;
    s32 na;
    u16 *e;
    u16 sum;
    s32 n;
    ItemStack *s;
    u16 it;
    u16 *p;
    Player *pl;
    u16 *lim;
    u16 *range;
    u8 *b;
    s32 i;
    s32 ch;
    s32 got;
    u16 acc2;
    s8 *rate;
    s16 c;
    s32 k;
    s32 nb;
    u16 idx;

    gScene.supplyTaken[0] = 0;
    got = 0;
    gScene.supplyTaken[1] = 0;
    pl = &gPlayers[gScene.localPlayer];
    for (i = 0; i < 48; i++) {
        gScene.supply[i].item = 0;
        gScene.supply[i].count = 0;
    }
    n = 0;
    na = nb = 0;
    if (Player_HasSkill(pl, 0x9C) == 1) {
        i = 2;
    } else if (Player_HasSkill(pl, 0x9E) == 1) {
        i = 3;
    } else if (Player_HasSkill(pl, 0x9D) == 1) {
        i = 1;
    } else if (Player_HasSkill(pl, 0x9F) == 1) {
        i = 4;
    } else {
        i = 0;
    }
    for (e = ((u16 *)gQuest.rewardTable); *e != 0xFFFF; e += 4) {
        if (*e >= 0xF8 && *e < 0x10D) {
            if (got != 0 || (u16)(20 - QuestGoal_HuntCount(0)) < *e - 0xF8) {
                goto next;
            }
            got = 1;
        } else if (*e >= 0x30 && *e < 0x94) {
            if (got != 0 || (u16)(99 - QuestGoal_HuntCount(0)) < *e - 0x30) {
                goto next;
            }
            got = 1;
        } else if (*e >= 0x94 && *e < 0xF8) {
            if (got != 0 || (u16)Quest_DeliveredCount(QuestGoal_DeliverItem(0)) < *e - 0x94) {
                goto next;
            }
            got = 1;
        } else if (*e >= 0x29 && *e < 0x2E) {
            if (got != 0 || func_D6ABF0() < gDrillPayoutThresholds[*e - 0x29]) {
                goto next;
            }
            got = 1;
        } else {
            switch (*e) {
            case 0x8000:
                break;
            case 0x8001:
                if (QuestGoal_IsDone(0) == 0) {
                    goto next;
                }
                break;
            case 0x8002:
                if (QuestGoal_IsDone(1) == 0) {
                    goto next;
                }
                break;
            case 0x8003:
                if (QuestGoal_IsDone(2) == 0) {
                    goto next;
                }
                break;
            case 0xB:
                if (gScene.unk13A <= 0x6400 || gQuest.savedEmHp > 0x6400) {
                    goto next;
                }
                break;
            case 0xC:
                if (gScene.unk13A <= 0x4B00 || gQuest.savedEmHp > 0x4B00) {
                    goto next;
                }
                break;
            case 0xD:
                if (gScene.unk13A <= 0x3200 || gQuest.savedEmHp > 0x3200) {
                    goto next;
                }
                break;
            case 0xE:
                if (gScene.unk13A <= 0x1900 || gQuest.savedEmHp > 0x1900) {
                    goto next;
                }
                break;
            case 0xF:
                if (gScene.unk13A <= 0x1900 || gQuest.savedEmHp > 0x1900) {
                    goto next;
                }
                break;
            case 0x15:
            case 0x16:
            case 0x17:
            case 0x18:
            case 0x19:
                range = &gRewardDeliverBands[*e - 0x15];
                k = (u16)Quest_DeliveredCount(QuestGoal_DeliverItem(0));
                if (k >= range[0] && k < range[1]) {
                    break;
                }
                goto next;
            case 0x1A:
            case 0x1B:
            case 0x1C:
            case 0x1D:
            case 0x1E:
            case 0x1F:
            case 0x20:
            case 0x21:
            case 0x22:
            case 0x23:
                range = &gRewardHuntBands[*e - 0x1A];
                k = (u16)(20 - QuestGoal_HuntCount(0));
                if (k >= range[0] && k < range[1]) {
                    break;
                }
                goto next;
            case 0x2E:
                if (gQuest.clearTime > Dojo_GetParMinutes(gQuest.stage) * 1800) {
                    goto next;
                }
                break;
            case 0x2F:
                if (gQuest.emLeft > 0) {
                    goto next;
                }
                break;
            default:
                goto next;
            }
        }
        lim = (u16 *)(*(s32 *)(e + 2) + gSysSettings.stageFile);
        for (k = 0;; k++) {
            switch (*e) {
            case 0x8000:
                if (Quest_IsDrill() == 1) {
                    rate = gRewardRateTables[4];
                } else if (Quest_IsTraining() == 1) {
                    rate = gRewardRateTables[7];
                } else {
                    rate = gRewardRateTables[0] + i * 11;
                }
                break;
            case 0x8001:
                rate = gRewardRateTables[1] + i * 9;
                break;
            case 0x8002:
            case 0x8003:
                rate = gRewardRateTables[2];
                rate += i * 6;
                break;
            case 0x2F:
                rate = gRewardRateTables[6];
                break;
            default:
                rate = gRewardRateTables[3];
                break;
            }
            ch = rate[k];
            if ((Rand_Next(0) & 0x1F) >= ch) {
                break;
            }
            sum = 0;
            for (p = lim; *p != 0xFFFF; p += 3) {
                sum += *p;
            }
            r = Rand_Next(0) % sum;
            if (*e == 0x8000 && k == 0) {
                r = 0;
            }
            if (*e == 0x8001 && k == 0) {
                r = 0;
            }
            sum = 0;
            for (p = lim; *p != 0xFFFF; p += 2) {
                sum += *p++;
                if (r < sum) {
                    break;
                }
            }
            if (*p != 0) {
                switch (*e) {
                case 0x8002:
                    gScene.supply[24 + na].item = p[0];
                    gScene.supply[24 + na].count = p[1];
                    if (na < 3) {
                        na++;
                    }
                    break;
                case 0x8003:
                    gScene.supply[28 + nb].item = p[0];
                    gScene.supply[28 + nb].count = p[1];
                    if (nb < 3) {
                        nb++;
                    }
                    break;
                default:
                    gScene.supply[n].item = p[0];
                    gScene.supply[n].count = p[1];
                    n++;
                    break;
                }
            }
            if (n >= 24) {
                break;
            }
        }
    next:
        if (n >= 24) {
            break;
        }
    }
    if (Quest_IsMustOrderOrVersus() == 0) {
        n = 32;
        for (b = gPartRewardRecs; b[0] != 0xFF; b += 8) {
            if (b[1] == 0x80) {
                if (gQuest.captured[b[0]] == 0) {
                    continue;
                }
            } else if (Quest_IsPartBroken(b[0], b[1]) == 0) {
                continue;
            }
            idx = *(u16 *)(b + 2 + *(s8 *)((u8 *)gStageHeader + 0x5C) * 2);
            if (idx == 0xFFFF) {
                continue;
            }
            lim = gRewardItemTables[idx];
            for (got = 0;; got++) {
                rate = b[1] == 0x80 ? gRewardRateTables[6] : gRewardRateTables[5];
                ch = rate[got];
                if ((Rand_Next(0) & 0x1F) >= ch) {
                    break;
                }
                sum = 0;
                for (p = lim; *p != 0xFFFF; p += 3) {
                    sum += *p;
                }
                r = Rand_Next(0) % sum;
                acc2 = 0;
                for (p = lim; *p != 0xFFFF; p += 2) {
                    acc2 += *p++;
                    if (r < acc2) {
                        break;
                    }
                }
                if (*p != 0) {
                    gScene.supply[n].item = p[0];
                    gScene.supply[n].count = p[1];
                    n++;
                }
                if (n >= 48) {
                    break;
                }
            }
        }
    }
    for (i = 0; i < 24; i++) {
        for (got = i + 1; got < 24; got++) {
            if (gScene.supply[i].item > gScene.supply[got].item && gScene.supply[got].item != 0) {
                it = gScene.supply[i].item;
                c = gScene.supply[i].count;
                gScene.supply[i].item = gScene.supply[got].item;
                gScene.supply[i].count = gScene.supply[got].count;
                gScene.supply[got].item = it;
                gScene.supply[got].count = c;
            }
        }
    }
    for (i = 24; i < 28; i++) {
        for (got = i + 1; got < 28; got++) {
            if (gScene.supply[i].item > gScene.supply[got].item && gScene.supply[got].item != 0) {
                it = gScene.supply[i].item;
                c = gScene.supply[i].count;
                gScene.supply[i].item = gScene.supply[got].item;
                gScene.supply[i].count = gScene.supply[got].count;
                gScene.supply[got].item = it;
                gScene.supply[got].count = c;
            }
        }
    }
    for (i = 28; i < 32; i++) {
        for (got = i + 1; got < 32; got++) {
            if (gScene.supply[i].item > gScene.supply[got].item && gScene.supply[got].item != 0) {
                it = gScene.supply[i].item;
                c = gScene.supply[i].count;
                gScene.supply[i].item = gScene.supply[got].item;
                gScene.supply[i].count = gScene.supply[got].count;
                gScene.supply[got].item = it;
                gScene.supply[got].count = c;
            }
        }
    }
    for (i = 32; i < 48; i++) {
        for (got = i + 1; got < 48; got++) {
            if (gScene.supply[i].item > gScene.supply[got].item && gScene.supply[got].item != 0) {
                it = gScene.supply[i].item;
                c = gScene.supply[i].count;
                gScene.supply[i].item = gScene.supply[got].item;
                gScene.supply[i].count = gScene.supply[got].count;
                gScene.supply[got].item = it;
                gScene.supply[got].count = c;
            }
        }
    }
}
#pragma optimization_level reset

/* unk_3A6130's .data (0x44B790-0x44BF30) */
extern u32 gHeapBlocks[];
u8 gEmSizeRollTbls_0[32] = {
#include "sub_main/0044B790.inc"
};
u8 gEmSizeRollTbls_1[32] = {
#include "sub_main/0044B7B0.inc"
};
u8 gEmSizeRollTbls_2[32] = {
#include "sub_main/0044B7D0.inc"
};
u8 gEmSizeRollTbls_3[32] = {
#include "sub_main/0044B7F0.inc"
};
u8 gEmSizeRollTbls_4[32] = {
#include "sub_main/0044B810.inc"
};
u8 gEmSizeRollTbls_5[32] = {
#include "sub_main/0044B830.inc"
};
u8 gEmSizeRollTbls_6[32] = {
#include "sub_main/0044B850.inc"
};
u8 *gEmSizeRollTbls[8] = {
    gEmSizeRollTbls_0, gEmSizeRollTbls_1, gEmSizeRollTbls_2, gEmSizeRollTbls_3,
    gEmSizeRollTbls_4, gEmSizeRollTbls_5, gEmSizeRollTbls_6, NULL,
};
u8 gEmSizeRollPicks[32] = {
#include "sub_main/0044B890.inc"
};
Reward gSpecialMineRewards_0[8] = {
#include "sub_main/0044B8B0.inc"
};
Reward gSpecialMineRewards_1[8] = {
#include "sub_main/0044B8D0.inc"
};
Reward gSpecialMineRewards_2[8] = {
#include "sub_main/0044B8F0.inc"
};
Reward gSpecialMineRewards_3[8] = {
#include "sub_main/0044B910.inc"
};
Reward gSpecialMineRewards_4[8] = {
#include "sub_main/0044B930.inc"
};
Reward gSpecialMineRewards_5[8] = {
#include "sub_main/0044B950.inc"
};
Reward gSpecialMineRewards_6[8] = {
#include "sub_main/0044B970.inc"
};
Reward gSpecialMineRewards_7[8] = {
#include "sub_main/0044B990.inc"
};
Reward gSpecialMineRewards_8[8] = {
#include "sub_main/0044B9B0.inc"
};
Reward gSpecialMineRewards_9[8] = {
#include "sub_main/0044B9D0.inc"
};
Reward gSpecialMineRewards_10[8] = {
#include "sub_main/0044B9F0.inc"
};
Reward gSpecialMineRewards_11[8] = {
#include "sub_main/0044BA10.inc"
};
Reward gSpecialMineRewards_12[8] = {
#include "sub_main/0044BA30.inc"
};
Reward gSpecialMineRewards_13[8] = {
#include "sub_main/0044BA50.inc"
};
Reward gSpecialMineRewards_14[8] = {
#include "sub_main/0044BA70.inc"
};
Reward gSpecialMineRewards_15[8] = {
#include "sub_main/0044BA90.inc"
};
Reward gSpecialMineRewards_16[8] = {
#include "sub_main/0044BAB0.inc"
};
Reward gSpecialMineRewards_17[8] = {
#include "sub_main/0044BAD0.inc"
};
Reward gSpecialMineRewards_18[8] = {
#include "sub_main/0044BAF0.inc"
};
Reward gSpecialMineRewards_19[8] = {
#include "sub_main/0044BB10.inc"
};
Reward gSpecialMineRewards_20[8] = {
#include "sub_main/0044BB30.inc"
};
Reward gSpecialMineRewards_21[8] = {
#include "sub_main/0044BB50.inc"
};
Reward gSpecialMineRewards_22[8] = {
#include "sub_main/0044BB70.inc"
};
Reward gSpecialMineRewards_23[8] = {
#include "sub_main/0044BB90.inc"
};
Reward gSpecialMineRewards_24[8] = {
#include "sub_main/0044BBB0.inc"
};
Reward gSpecialMineRewards_25[8] = {
#include "sub_main/0044BBD0.inc"
};
Reward gSpecialMineRewards_26[8] = {
#include "sub_main/0044BBF0.inc"
};
Reward gSpecialMineRewards_27[8] = {
#include "sub_main/0044BC10.inc"
};
Reward gSpecialMineRewards_28[8] = {
#include "sub_main/0044BC30.inc"
};
Reward gSpecialMineRewards_29[8] = {
#include "sub_main/0044BC50.inc"
};
Reward gSpecialMineRewards_30[8] = {
#include "sub_main/0044BC70.inc"
};
Reward gSpecialMineRewards_31[8] = {
#include "sub_main/0044BC90.inc"
};
Reward gSpecialMineRewards_32[8] = {
#include "sub_main/0044BCB0.inc"
};
Reward *gSpecialMineRewards[36] = {
    gSpecialMineRewards_0, gSpecialMineRewards_1, gSpecialMineRewards_2, gSpecialMineRewards_3,
    gSpecialMineRewards_4, gSpecialMineRewards_5, gSpecialMineRewards_6, gSpecialMineRewards_7,
    gSpecialMineRewards_8, gSpecialMineRewards_9, gSpecialMineRewards_10, gSpecialMineRewards_11,
    gSpecialMineRewards_12, gSpecialMineRewards_13, gSpecialMineRewards_14, gSpecialMineRewards_15,
    gSpecialMineRewards_16, gSpecialMineRewards_17, gSpecialMineRewards_18, gSpecialMineRewards_19,
    gSpecialMineRewards_20, gSpecialMineRewards_21, gSpecialMineRewards_22, gSpecialMineRewards_23,
    gSpecialMineRewards_24, gSpecialMineRewards_25, gSpecialMineRewards_26, gSpecialMineRewards_27,
    gSpecialMineRewards_28, gSpecialMineRewards_29, gSpecialMineRewards_30, gSpecialMineRewards_31,
    gSpecialMineRewards_32, NULL, NULL, NULL,
};
u16 gSpecialMinePoints[40] = {
#include "sub_main/0044BD60.inc"
};
u32 gDefaultStageHeader_3[4] = {
#include "sub_main/0044BDB0.inc"
};
s8 gRewardGiveRolls[16] = {
#include "sub_main/0044BDC0.inc"
};
QuestTimeMsg gQuestTimeNotices[3] = {
#include "sub_main/0044BDD0.inc"
};
u32 gStyleSymbolColors[2] = {
#include "sub_main/0044BDE8.inc"
};
s32 gVersusEmAngles[2] = {
#include "sub_main/0044BDF0.inc"
};
f32 gVersusEmPos[2][3] = {
#include "sub_main/0044BE00.inc"
};
u32 gStageFileTbls_0[12] = {
#include "sub_main/0044BE20.inc"
};
u32 gStageFileTbls_1[6] = {
#include "sub_main/0044BE50.inc"
};
u32 gStageFileTbls_2[3] = {
#include "sub_main/0044BE68.inc"
};
u32 gStageFileTbls_3[44] = {
#include "sub_main/0044BE80.inc"
};


/* ---- 0x3AAD20 (was unk_3AAD20.c) ---- */
/*
 * Player state: the state and sub-state bytes (+0x14/+0x15, the previous
 * ones at +0x16/+0x17) and their change requests, the two flag words
 * (+0x398, +0x39C: flag bit 31 selects the second), the idle motions by
 * the area's climate, and a few quest helpers.
 */

/* The monster QuestEm_SpawnYamaTsukamiB builds for QuestEm_Spawn: a
 * QuestEm, but 0x40 bytes, the stack frame the game's function has. The
 * positions are written as integers (0). */
typedef struct SpawnRequest {
    s16 kind;
    s16 variant;
    u8 unk04[0x1C - 0x04];
    s32 rotY;
    s32 x;
    s32 y;
    s32 z;
    u8 unk2C[0x30 - 0x2C];
    s16 size;
    s16 hp;
    u8 unk34[0x40 - 0x34];
} SpawnRequest;

/* The quest record's counts per monster kind (QuestRecord in the boot
 * ELF's main.c: kindA and kindB). */
extern u8 gQuestRecordHunted[];
extern u8 gQuestRecordCaptured[];
extern s32 gQuestPartyClearTime;


s16 Pouch_Count(Player *p, u16 item);
u16 Rand_Next(s32 stream);
u16 Area_DayId(u16 a);
void QSync_SendState(u8 kind, u8 who);
s32 Quest_GetTime(s32 a);
s32 Player_IsLocal(void *obj);
void Player_UpdateMotionTime(Player *p, s16 step);
void QSync_SendPlayer(Player *p, s32 kind, u16 mode);
void Player_BeginState(Player *p, s32 action, s32 a2, s32 a3);
void Player_BeginStateKeepParts(Player *p, s32 action, s32 a2, s32 a3);
void Player_ResetAttackPercent(Player *p);
s16 Area_Temperature(u16 area);
s32 Player_HasSkill(Player *p, u8 skill);
/* unit's own declaration: with the definition's void (Player *p, s32 motion, s32 a, s16 b, s32 layer) (sub_main/unk_3AC9C0.c), Player_StartIdleMotion: 21 of 135 words differ */
void Player_SetMotion(Player *p, s32 motion, s32 a, s32 b, s32 c);

void Player_ClearPartOverrides(Player *p);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_SetFlag(void *obj, u32 flag);

/* Nonzero when the local player holds the items of a QUEST_OP_WAIT_ITEMS
 * command (cmd: the command read as halfwords: its item, then its count). */
s32 Cond_HasItems(void *unused, u16 *cmd)
{
    u16 item;
    s16 count;

    cmd++;
    item = *cmd++;
    count = *cmd;
    return count <= Pouch_Count(&gPlayers[gScene.localPlayer], item);
}

/* The quest is ENDED: the script jumps to its end mark (QUEST_OP_END, or
 * with `alt` QUEST_OP_END_ALT). */
void Quest_JumpToEnd(s32 alt)
{
    QuestCmd *e = (QuestCmd *)gQuest.script;
    s16 i = 0;
    s16 op;

    gScene.questPhase = QUEST_PHASE_ENDED;
    op = alt == 0 ? QUEST_OP_END : QUEST_OP_END_ALT;
    for (;;) {
        if (e->op == op) {
            break;
        }
        i++;
        e++;
    }
    gQuest.scriptPc = i;
}

/* Another player started the quest: unless it runs already, the script
 * jumps to its start mark (QUEST_OP_START, or with `alt`
 * QUEST_OP_START_ALT) and the quest is RUNNING. */
static void Quest_JumpToStart(s32 alt)
{
    QuestCmd *e = (QuestCmd *)gQuest.script;
    s16 i = 0;
    s16 op;

    switch (gScene.questPhase) {
    case 0:
    case 1:
    case QUEST_PHASE_READY:
        op = alt == 0 ? QUEST_OP_START : QUEST_OP_START_ALT;
        for (;;) {
            if (e->op == op) {
                break;
            }
            i++;
            e++;
        }
        gQuest.scriptPc = i;
        gScene.questPhase = QUEST_PHASE_RUNNING;
        break;
    }
}

/* 1, one time in 16, on the quests 0x2B04 to 0x2B06 in the area of day id
 * 0x6E (QuestEm_SpawnArea then adds Yama Tsukami's second kind). */
s32 QuestEm_RollYamaTsukamiB(s32 area)
{
    switch (gQuest.stage) {
    case 0x2B04:
    case 0x2B05:
    case 0x2B06:
        if (Area_DayId(area) == 0x6E && (Rand_Next(0) & 0xF) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Spawns Yama Tsukami's second kind at the area's origin, at its full
 * health and the normal size. */
void QuestEm_SpawnYamaTsukamiB(void)
{
    SpawnRequest r;

    r.kind = EM_KIND_YAMA_TSUKAMI_B;
    r.variant = 0;
    r.hp = -1;
    r.x = 0;
    r.size = 100;
    r.y = 0;
    r.z = 0;
    r.rotY = 0;
    QuestEm_Spawn((QuestEm *)&r);
}

extern s32 gQuestCountdown;
void Quest_JumpToEnd(s32 a);
void Quest_OnRemoteKill(void);
u8 Quest_SetDelivered(Player *pl, u16 item, s32 count);
void QuestGoal_Complete(u16 i, u32 flag);
void Versus_End(s8 side);
s32 Side_Match(u8 a, u8 b, u8 c);

/* Applies the quest message another player sent (gQuest.msgType, QMSG_*);
 * Quest_JumpToStart is static in this unit. */
void Quest_HandleNetMsg(void)
{
    s32 v;

    switch (gQuest.msgType) {
    case QMSG_STARTED:
        gQuest.acks |= 1 << (gQuest.msgFrom + QUEST_ACK_STARTED_SHIFT);
        Quest_JumpToStart(0);
        break;
    case QMSG_ENDED:
        if (gScene.questPhase != QUEST_PHASE_ENDED && gScene.questPhase != QUEST_PHASE_FAILED) {
            Quest_JumpToEnd(0);
        }
        break;
    case QMSG_TIME_LEFT:
        v = (gQuest.msgArg2 & 0xFFFF) | ((gQuest.msgArg1 << 16) & 0xFFFF0000);
        if (v < (s32)gQuestTimeLeft) {
            gQuestTimeLeft = v;
        }
        break;
    case QMSG_PROGRESS:
        if (gQuest.progress < gQuest.msgArg1) {
            gQuest.progress = gQuest.msgArg1;
        }
        break;
    case QMSG_KILL:
        Quest_OnRemoteKill();
        break;
    case QMSG_DELIVERED:
        Quest_SetDelivered(&gPlayers[gQuest.msgFrom], gQuest.msgArg1, gQuest.msgArg2);
        break;
    case QMSG_ENDED_ALT:
        if (gScene.questPhase != QUEST_PHASE_ENDED && gScene.questPhase != QUEST_PHASE_FAILED) {
            Quest_JumpToEnd(1);
        }
        break;
    case QMSG_STARTED_ALT:
        gQuest.acks |= 1 << (gQuest.msgFrom + QUEST_ACK_STARTED_SHIFT);
        Quest_JumpToStart(1);
        break;
    case QMSG_RANK:
        gQuest.acks |= 1 << gQuest.msgFrom;
        gQuest.hunterRank[gQuest.msgFrom][0] = gQuest.msgArg1;
        break;
    case QMSG_REWARD_ACK:
        gQuest.acks |= 1 << (gQuest.msgFrom + QUEST_ACK_REWARD_SHIFT);
        gQuest.netMsgArgs |= gQuest.msgArg1 << 16;
        gQuest.netMsgArgs |= gQuest.msgArg2;
        break;
    case QMSG_EVENT_OVER:
        if (gQuest.timedEvent != 0) {
            gQuest.timedEvent = 0;
            Notice_PushItem(gQuestNoticeTexts[3]);
        }
        break;
    case QMSG_CLEAR_TIME:
        v = (gQuest.msgArg2 & 0xFFFF) | ((gQuest.msgArg1 << 16) & 0xFFFF0000);
        if (gQuestPartyClearTime < v) {
            gQuestPartyClearTime = v;
        }
        break;
    case QMSG_GOAL_DONE:
        QuestGoal_Complete(gQuest.msgArea, (gQuest.msgArg2 & 0xFFFF) | ((gQuest.msgArg1 << 16) & 0xFFFF0000));
        break;
    case QMSG_VERSUS_END:
        gQuest.acks |= 1 << gQuest.msgFrom;
        if (gQuest.state < QSTATE_VERSUS_END) {
            Versus_End(gQuest.msgArg1);
        }
        if (Side_Match(gPlayers[gScene.localPlayer].side, gQuest.msgArg1, 1) == 0) {
            gQuest.result = QUEST_PHASE_FAILED;
        }
        break;
    case QMSG_COUNTDOWN_MSG:
        if (gQuest.countdownMsg < gQuest.msgArg1) {
            gQuest.countdownMsg = gQuest.msgArg1;
            gQuestCountdown = -1;
        }
        break;
    case QMSG_COUNTDOWN_START:
        gScene.countdownOn = 1;
        gQuestCountdown = 18000; /* frames: 10 min */
        break;
    case QMSG_VERSUS_DEFEAT:
        gQuest.versusDefeat |= (u8)gQuest.msgArg1;
        break;
    }
}

/* Quest_OnRemoteKill's view of a QuestEm (quest.h): the game reads the
 * index's low byte only (lbu), which this view gives. */
typedef struct QuestEmView {
    s16 kind;           /* 0x00 */
    u8 pad2[2];
    s8 count;           /* 0x04 */
    u8 pad5[0x2C - 0x5];
    u8 index;           /* 0x2C: QuestEm.index's low byte */
    u8 pad2D;
    s16 flags;          /* 0x2E */
    u8 pad30[0x34 - 0x30];
    s8 emIndex;         /* 0x34 */
    s8 large;           /* 0x35 */
} QuestEmView;

void EmMine_Remove(Em *em);
void Em_ChangeState(Em *em, s32 group, s32 state, s32 mode);
void Quest_CountEventKill(u16 type);
void Quest_CountTarget(u16 type);
s32 Quest_IsVersus(void);
void QuestEm_CountHunted(u16 i);
extern Em gEms[];

/*
 * Another player's QMSG_KILL (Quest_SendKill): the quest monster it names
 * (by index, in msgArea) is down for them. When its monster is here and
 * alive, it is put down too (state group 5, no health; its mining point
 * goes if it was already down). When it isn't here, the kill is counted
 * as ours would be, unless this player has already counted it (the
 * appearances left are no more than the sender's).
 */
void Quest_OnRemoteKill(void)
{
    QuestEmView *q;
    Em *em;
    u16 countLeft;
    s16 flags;
    s8 emIndex;
    s32 i;

    countLeft = gQuest.msgArg2;
    em = gEms;
    if ((q = (QuestEmView *)QuestEm_FindByIndex(gQuest.msgArg1, gQuest.msgArea)) != NULL) {
        flags = q->flags;
        if (flags & (QUEST_EM_ACTIVE | QUEST_EM_HUNTED)) {
            emIndex = q->emIndex;
            for (i = 0; i < 20 /* gEms */; i++, em++) {
                if (em->index == emIndex && em->questIndex == q->index) {
                    if (em->type != q->kind) {
                        return;
                    }
                    if (em->active == 0) {
                        return;
                    }
                    if (!(flags & QUEST_EM_HUNTED)) {
                        if (em->taskPhase >= 2) {
                            return;
                        }
                        if (em->stateGroup == EM_GROUP_DEFEAT) {
                            return;
                        }
                    } else if (q->count <= countLeft) {
                        return;
                    }
                    if (em->taskPhase >= 2 || em->stateGroup == EM_GROUP_DEFEAT) {
                        EmMine_Remove(em);
                    }
                    Em_ChangeState(em, EM_GROUP_DEFEAT, 0, 0);
                    em->alpha = 1.0f;
                    q->flags = QUEST_EM_ACTIVE;
                    em->hp = 0;
                    if (QuestGoal_IsClearTarget(em->type) == 1) {
                        (*(Em * *)&gQuest.downEm) = em;
                    }
                    return;
                }
            }
            if (q->count > countLeft) {
                QuestGoal_OnKill(q->kind);
                Quest_CountEventKill(q->kind);
                Quest_CountTarget(q->kind);
                if (Quest_IsVersus() == 0) {
                    QuestEm_CountHunted(q->kind);
                    gQuest.hunted[q->kind]++;
                    if (gQuest.hunted[q->kind] > QUEST_COUNT_MAX) {
                        gQuest.hunted[q->kind] = QUEST_COUNT_MAX;
                    }
                }
                if (q->large != 0) {
                    gQuest.emLeft--;
                }
                q->count--;
                if (q->count > 0) {
                    q->flags = QUEST_EM_ACTIVE;
                    return;
                }
                q->flags = QUEST_EM_DONE;
                q->emIndex = -1;
            }
        }
    }
}

/* Sends the local time left to the others (not during an event scene). */
void Quest_SendTime(void)
{
    if (gScene.eventRunning == 0) {
        gQuest.msgArg0 = 0;
        gQuest.msgType = QMSG_TIME_LEFT;
        gQuest.msgArg2 = gQuestTimeLeft;
        gQuest.msgArg1 = (u32)gQuestTimeLeft >> 16;
        QSync_SendState(6, gScene.localPlayer);
    }
}

/* Tells the others that quest monster q is down (QMSG_KILL); p is the
 * monster, read as a player for its area. */
void Quest_SendKill(QuestEm *q, Player *p)
{
    gQuest.msgArg0 = 0;
    gQuest.msgType = QMSG_KILL;
    gQuest.msgArg1 = q->index;
    gQuest.msgArg2 = q->count;
    gQuest.msgArea = p->area;
    QSync_SendState(6, gScene.localPlayer);
}

void func_003AB680(void)
{
}

/* Counts a hunt of monster kind `kind` in the quest record: twice on the
 * stage tiers 1 and 2. */
void QuestEm_CountHunted(u16 kind)
{
    switch (gStageHeader->tier) {
    case 1:
    case 2:
        gQuestRecordHunted[kind] += 2;
        break;
    default:
        gQuestRecordHunted[kind] += 1;
        break;
    }
}

/* Counts a capture of monster kind `kind` in the quest record, as QuestEm_CountHunted. */
void QuestEm_CountCaptured(u16 kind)
{
    switch (gStageHeader->tier) {
    case 1:
    case 2:
        gQuestRecordCaptured[kind] += 2;
        break;
    default:
        gQuestRecordCaptured[kind] += 1;
        break;
    }
}

/* Stores the quest time so far (gQuest.elapsed). */
void Quest_StoreElapsed(void)
{
    s32 limit = Quest_GetTime(QUEST_TIME_LIMIT);

    gQuest.elapsed = limit - Quest_GetTime(QUEST_TIME_LEFT);
}

void Player_ClearPartOverrides(Player *p)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        p->partAnimOn[i] = 0;
        p->partAnimMotion[i] = 0;
        p->partAnimWeight[i] = 0;
    }
}

/* Sets the state (see Player_SetState) without the checks and the network
 * message. */
void Player_ForceState(void *obj, s32 state, s32 sub)
{
    Player *p = obj;
    if (p->isEm == 0) {
        p->prevStateGroup = p->stateGroup;
        p->prevState = p->state;
    }
    p->stateGroup = state;
    p->state = sub;
    p->step = 0;
    p->stateVar0 = 0;
    p->stateVar1 = 0;
    if (p->isEm == 0) {
        p->stateRand = Rand_Next(1);
    }
    if (p->isEm == 0) {
        Player_ClearPartOverrides(p);
    } else {
        switch (p->weaponType) {
        case 1:
        case 5:
        case 10:
            if (p->stateGroup == 1) {
                switch (p->state) {
                case 0:
                case 4:
                case 5:
                    break;
                default:
                    Player_ClearPartOverrides(p);
                    break;
                }
            } else {
                Player_ClearPartOverrides(p);
            }
            break;
        default:
            Player_ClearPartOverrides(p);
            break;
        }
    }
    if (p->isEm == 0 && p->stateGroup == 2) {
        p->unk417 = 0;
    }
    Player_UpdateMotionTime(p, 0);
    p->flags &= 0x100100;
    p->stateFlags = 0;
    if (p->isEm == 0 && p->unk01E == 0) {
        if (Player_IsLocal(p) == 1) {
            QSync_SendPlayer(p, 1, 0);
            return;
        }
        if (Player_IsState(p, 0, 0) != 0) {
            Player_BeginState(p, 0, 4, 0);
        }
    }
}

/*
 * Changes the player's state: the old one is kept at +0x16/+0x17, the
 * state timers are reset, and the local player sends it with QSync_SendPlayer
 * (flags 0x10: kind 6, 0x40: kind 9, else 1; none with flag 2). A player
 * whose state is locked (+0x682) only accepts state 4. Flag 0x20 keeps the
 * low byte of the random value at +0x3A2, flag 1 keeps unk66C-unk67E.
 */
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags)
{
    u16 low;

    if (Player_IsLocal(p) == 1 && p->areaMoveState != 0 && state != 4) {
        return;
    }
    p->prevStateGroup = p->stateGroup;
    p->prevState = p->state;
    p->stateGroup = state;
    p->state = sub;
    p->step = 0;
    p->stateVar0 = 0;
    p->stateVar1 = 0;
    if (flags & 0x20) {
        low = (u8)p->stateRand;
        p->stateRand = Rand_Next(1) & 0xFF00;
        p->stateRand |= low;
    } else {
        p->stateRand = Rand_Next(1);
    }
    p->stateRequest = 0;
    p->motionSpeed = 1.0f;
    if (!(flags & 1)) {
        Player_ClearPartOverrides(p);
        p->aimWeight = 0;
    }
    Player_ResetAttackPercent(p);
    if (p->stateGroup == 2) {
        p->unk417 = 0;
    }
    Player_UpdateMotionTime(p, 0);
    p->flags &= 0x80100;
    p->stateFlags = 0;
    p->motion[0].step = 2.0f;
    p->motion[1].step = 2.0f;
    if (Player_IsLocal(p) == 1) {
        if (!(flags & 2)) {
            if (flags & 0x10) {
                QSync_SendPlayer(p, 6, flags);
            } else if (flags & 0x40) {
                QSync_SendPlayer(p, 9, flags);
            } else {
                QSync_SendPlayer(p, 1, flags);
            }
        }
    } else if (Player_IsState(p, 0, 0) != 0) {
        if (!(flags & 1)) {
            Player_BeginState(p, 0, 4, 0);
        } else {
            Player_BeginStateKeepParts(p, 0, 4, 0);
        }
    }
}

void Player_RequestState(Player *p, u16 state, u8 sub, u16 flags)
{
    if (Player_IsLocal(p) != 1 || p->areaMoveState == 0 || state == 4) {
        Player_SetState(p, state, sub, flags);
        p->stateRequest = 1;
    }
}

void Player_RequestStateSynced(Player *p, u16 state, u8 sub, u16 flags)
{
    if (Player_IsLocal(p) != 1 || p->areaMoveState == 0 || state == 4) {
        if (Player_IsLocal(p) == 1) {
            p->stateRand = gQuest.drillCharm;
        } else {
            p->stateRand = 0;
        }
        Player_SetState(p, state, sub, flags | 0x20);
        p->stateRequest = 1;
    }
}

/* Nonzero when the player is in this state and sub-state. */
s16 Player_IsState(void *obj, u8 state, u8 sub)
{
    ObjHead *o = obj;
    if (o->stateGroup == state && o->state == sub) {
        return 1;
    }
    return 0;
}

u32 Player_TestFlag(void *obj, u32 flag)
{
    ObjHead *o = obj;
    if ((flag & 0x80000000) == 0) {
        return o->flags & flag;
    }
    return o->stateFlags & (flag & 0x7FFFFFFF);
}

void Player_SetFlag(void *obj, u32 flag)
{
    ObjHead *o = obj;
    if (!(flag & 0x80000000)) {
        o->flags |= flag;
    } else {
        o->stateFlags |= flag & 0x7FFFFFFF;
    }
}

void Player_ClearFlag(void *obj, u32 flag)
{
    ObjHead *o = obj;
    if (!(flag & 0x80000000)) {
        o->flags &= ~flag;
    } else {
        o->stateFlags &= ~(flag & 0x7FFFFFFF);
    }
}

void Player_SetAction(Player *p, u8 action)
{
    p->action = action;
}

s32 Player_TestFlags(void *obj, s32 mask)
{
    ObjHead *o = obj;
    return o->unk078 & mask;
}

s32 Player_TestSetFlags(void *obj, s32 mask)
{
    ObjHead *o = obj;
    s32 ret = o->unk078 & mask;

    o->unk078 |= (u16)mask;
    return ret;
}

void func_003ABE90(Player *p, s16 a, s32 b)
{
    p->motionId[0] = 0x3F9;
    if (b == 0) {
        p->unk2B4 = 2;
    } else {
        p->unk2B4 = 6;
    }
    p->motionBlend[0] = a / 2;
    p->motionBlend[1] = a / 2;
    p->motionFrame[0] = 8;
    p->motionFrame[1] = 8;
    Player_SetFlag(p, 1);
}

/* Starts the idle motions: tired (unk588), unk012, hungry (maximum stamina
 * below 76), or by the area's climate (1 and 3: skills 0x85/0x86; 2 hot:
 * 0x8A/0x8B; 4 cold: 0x8B). */
void Player_StartIdleMotion(Player *p, s32 a, s32 b)
{
    u16 m1;
    u16 m2;

    if (p->rideState != 0) {
        m1 = 0x18;
        m2 = 0x7C;
    } else if (p->weaponOut != 0) {
        m1 = 0x3E9;
        m2 = 0x44D;
    } else if (p->staminaMax < 76) {
        m1 = 0x193;
        m2 = 0x1F7;
    } else {
        switch (Area_Temperature(p->area)) {
        case 0:
        default:
            m1 = 1;
            m2 = 0x65;
            break;
        case 1:
            if (Player_HasSkill(p, 0x85) == 1 || Player_HasSkill(p, 0x86) == 1) {
                m1 = 1;
                m2 = 0x65;
            } else {
                m1 = 0x15;
                m2 = 0x79;
            }
            break;
        case 3:
            if (Player_HasSkill(p, 0x86) == 1) {
                m1 = 1;
                m2 = 0x65;
            } else {
                m1 = 0x15;
                m2 = 0x79;
            }
            break;
        case 2:
            if (Player_HasSkill(p, 0x8A) == 1 || Player_HasSkill(p, 0x8B) == 1) {
                m1 = 1;
                m2 = 0x65;
            } else {
                m1 = 0x1AF;
                m2 = 0x213;
            }
            break;
        case 4:
            if (Player_HasSkill(p, 0x8B) == 1) {
                m1 = 1;
                m2 = 0x65;
            } else {
                m1 = 0x1AF;
                m2 = 0x213;
            }
            break;
        }
    }
    Player_SetMotion(p, m1, a, b, 0);
    Player_SetMotion(p, m2, a, b, 1);
}

void Player_SetIdle(Player *p, s32 a, s32 b, s32 quiet)
{
    if (p->rideState != 0) {
        p->motionId[0] = 0x18;
        p->motionId[1] = 0x7C;
    } else if (p->weaponOut != 0) {
        p->motionId[0] = 0x3E9;
        p->motionId[1] = 0x44D;
    } else if (p->staminaMax < 76) {
        p->motionId[0] = 0x193;
        p->motionId[1] = 0x1F7;
    } else {
        switch (Area_Temperature(p->area)) {
        case 0:
        default:
            p->motionId[0] = 1;
            p->motionId[1] = 0x65;
            break;
        case 1:
            if (Player_HasSkill(p, 0x85) == 1 || Player_HasSkill(p, 0x86) == 1) {
                p->motionId[0] = 1;
                p->motionId[1] = 0x65;
            } else {
                p->motionId[0] = 0x15;
                p->motionId[1] = 0x79;
            }
            break;
        case 3:
            if (Player_HasSkill(p, 0x86) == 1) {
                p->motionId[0] = 1;
                p->motionId[1] = 0x65;
            } else {
                p->motionId[0] = 0x15;
                p->motionId[1] = 0x79;
            }
            break;
        case 2:
            if (Player_HasSkill(p, 0x8A) == 1 || Player_HasSkill(p, 0x8B) == 1) {
                p->motionId[0] = 1;
                p->motionId[1] = 0x65;
            } else {
                p->motionId[0] = 0x1AF;
                p->motionId[1] = 0x213;
            }
            break;
        case 4:
            if (Player_HasSkill(p, 0x8B) == 1) {
                p->motionId[0] = 1;
                p->motionId[1] = 0x65;
            } else {
                p->motionId[0] = 0x1AF;
                p->motionId[1] = 0x213;
            }
            break;
        }
    }
    p->motionBlend[0] = a / 2;
    p->motionBlend[1] = a / 2;
    p->motionFrame[0] = b;
    p->motionFrame[1] = b;
    p->stateGroup = 0;
    p->state = 0;
    if (Player_IsLocal(p) == 1 && quiet == 0) {
        QSync_SendPlayer(p, 1, 0);
    }
}
