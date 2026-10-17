/*
 * The quest: its state and everything that runs it while it lasts.
 *
 * - gQuest (QuestState, quest.h; cleared by Quest_Clear, set up from the
 *   stage file by Quest_Init): the stage header (StageHeader) and its
 *   tables, the time left, the reward and the faint penalty, the quest
 *   phase (gScene.questPhase, QUEST_PHASE_*).
 * - The quest script (QuestCmd, QUEST_OP_*): Quest_Start, Quest_Update and
 *   Quest_StepScript run it; it waits, shows notices, adds goals and jumps
 *   to its clear and end marks.
 * - The goals (QuestGoal, QUEST_GOAL_*): hunts, deliveries, parts to break
 *   and damage to deal, checked against the clear type (QUEST_CLEAR_*).
 * - The quest's monsters (QuestEm): the lists in the stage file, their
 *   levels and sizes, when they appear, leave and come back, and the counts
 *   of the monsters hunted and captured.
 * - The deliveries (Player.delivered) and the gathering points placed at
 *   run time (MinePoint, mine_point.h).
 * - The rewards: the items Melynxes stole (gQuest.rewards, Quest_GiveRewards),
 *   and the rewards rolled at the end into gScene.supply (Quest_RollRewards).
 * - The players' quest messages (QMSG_*, sent by QSync_SendState with kind
 *   6, handled by Quest_HandleNetMsg).
 * - The player's state changes (Player_SetState and the flags), which the
 *   game overlay's state code calls.
 *
 * The scene (unk_3240E0.c) starts and ends the quest; unk_39C290.c holds
 * the quest's queries, the versus code and the records; the game overlay
 * runs the monsters, the players and the stage objects that call in here.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "scene.h"
#include "save.h"
#include "quest.h"
#include "syssettings.h"
#include "mine_point.h"
#include "objhead.h"
#include "area.h"
#include "qsync.h"

/* One of Capcom's units, 0x3A1070-0x3AC3A0 (it calls the static
 * Quest_DeliveredCount, defined further down), built at optimization level 4
 * (docs/mwcc-matching.md, "Optimization level 4 files"), except
 * Quest_RollRewards (level 3). */
#pragma optimization_level 4

extern u32 gStageFile;          /* base address of the stage file */
extern u16 **gStartAreaTables[];
extern u16 gEmAppearMax[];
extern PartBreakEffect gPartBreakEffects[];
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
void QuestEm_CapturedStub(); /* unit's own declaration: defined below as (void); its caller passes q and em in a0 and a1, which it ignores */
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
s32 QuestGoal_InitialFlags(u16 i, s32 mask);
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
extern EmSizeRange gEmSizeRanges[5];
extern EmSizeRange gEmSizeRangeDefaults[5];
extern s8 gTraderVariantRolls[16];
extern s16 *gStageFileTbls[];
extern s16 gQuestGroupLastIds[];
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
    s16 *tbl = gQuestGroupLastIds;
    s32 i;
    u8 season;
    u8 daytimeRow;
    StageHeader *stageHeader;
    u32 *mapInfo;
    s32 file;
    u16 rolled;

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
            rolled = Rand_Next(0);
            gQuest.seed[i] = rolled;
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
    for (i = 0; i < SCENE_PLAYERS; i++) {
        gQuest.seed[i] = Quest_Random();
    }
    gQuest.acks = 0;
    gQuest.netMsgArgs = 0;
    gQuest.eventTriggered = 0;
    gQuest.eventLabel = 0;
    gQuest.goalLabel = 0;
    for (i = 0; i < SCENE_PLAYERS; i++) {
        gQuest.hunterRank[i][0] = 0;
        gQuest.hunterRank[i][1] = 0;
    }
    gQuest.emLeft = 0;
    gQuest.savedEmParts = 0;
    gQuest.savedEmHp = 0;
    gQuest.anyFainted = 0;
    gQuest.hunterRank[gScene.localPlayer][1] = Save_RankFromPoints(&gSave);
    gQuest.unk096 = 1;
    for (i = 0; i < SCENE_EM_KINDS; i++) {
        gScene.emKind[i] = 0;
    }
    for (i = 0; i < QUEST_REWARD_SLOTS; i++) {
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
                        season = SEASON_BREEDING;
                    } else if (gNextQuest.flags & QUEST_FLAG_SET_WARM) {
                        season = SEASON_WARM;
                    } else {
                        season = SEASON_COLD;
                    }
                } else {
                    season = Season_Get();
                }
                /* these maps are always played by day */
                switch (gNextQuest.map) {
                case MAP_CASTLE:
                case MAP_BATTLEFIELD:
                case MAP_TOWER:
                case MAP_TOWER_B:
                case MAP_TOWER_C:
                    daytimeRow = DAYTIME_DAY * 3;
                    Daytime_Set(DAYTIME_SET_DAY);
                    break;
                default:
                    daytimeRow = Daytime_Get() * 3;
                    break;
                }
                file += daytimeRow + season;
            } else {
                /* versus: the warm night's file */
                season = SEASON_WARM;
                file += DAYTIME_NIGHT * 3;
            }
            switch (season) {
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
                season = SEASON_BREEDING;
            } else if (gQuest.info->flags & QUEST_FLAG_COLD) {
                season = SEASON_COLD;
            } else {
                season = SEASON_WARM;
            }
            Quest_SetSeason(season);
            Daytime_Set((gQuest.info->flags & QUEST_FLAG_DAY) ? DAYTIME_SET_DAY : DAYTIME_SET_NIGHT);
        }
        stageHeader = (StageHeader *)gStageFile;
        Quest_SetCurrent(gQuest.stage, stageHeader, 0);
        gQuest.info = (QuestInfo *)(stageHeader->info + gStageFile);
        gQuest.header = stageHeader;
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
        gQuest.script = (void *)(stageHeader->script + gStageFile);
        gQuest.loopPc = 0;
        gQuest.scriptPc = 0;
        gQuest.areaData = (void *)(stageHeader->areaData + gStageFile);
        gQuest.noticeTexts = (void *)(stageHeader->noticeTexts + gStageFile);
        gQuest.texts = (void *)(gQuest.info->texts + gStageFile);
        gQuest.emSpawns = (void *)(stageHeader->emSpawns + gStageFile);
        gQuest.stageEntry = (void *)(stageHeader->stageEntry + gStageFile);
        gQuest.areaRecords = (void *)(stageHeader->areaRecords + gStageFile);
        gQuest.gatherPoints = (void *)(stageHeader->gatherPoints + gStageFile);
        gQuest.actionSpots = (void *)(stageHeader->actionSpots + gStageFile);
        gQuest.dataTable = (void *)(stageHeader->dataTable + gStageFile);
        gQuest.seasonalData = (void *)(stageHeader->seasonalData + gStageFile);
        gQuest.reward = gQuest.info->reward;
        gQuest.penalty = gQuest.info->penalty;
        gQuest.rewardTable = (void *)(stageHeader->rewardTable + gStageFile);
        gQuest.flags = gQuest.info->flags;
        gQuest.emLevel = stageHeader->emLevel;
        gQuest.header->tables += gStageFile;
        mapInfo = (u32 *)(stageHeader->stageInfo + gStageFile);
        gScene.map = mapInfo[0];
        gScene.campArea = mapInfo[1];
        gQuest.timedEvent = stageHeader->timedEvent;
        gQuest.eventKind = stageHeader->eventKind;
        gQuest.eventCount = stageHeader->eventCount;
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
        gScene.map = MAP_FOREST_HILLS;
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
    QuestCmd *cmd = gQuest.script;
    s16 i = 0;

    switch (gScene.questPhase) {
    case QUEST_PHASE_NONE:
    case 1:
    case QUEST_PHASE_READY:
        for (;;) {
            if (cmd->op == QUEST_OP_START || cmd->op == QUEST_OP_START_ALT) {
                break;
            }
            i++;
            cmd++;
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
    u16 *entry;
    u16 *weights;
    s32 sum;
    s32 roll;
    StartArea *start;
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
        weights = tbl[gQuest.header->randomStart - 1];
        sum = 0;
        for (entry = weights; *entry != 0xFFFF; entry += 2) {
            sum += *entry;
        }
        if (who == START_AREA_SCENE) {
            roll = gQuest.seed[gScene.localPlayer] % sum;
        } else {
            roll = gQuest.seed[who] % sum;
        }
        sum = 0;
        while (*weights != 0xFFFF) {
            if (roll < (sum += *weights++)) {
                break;
            }
            weights++;
        }
        if (who == START_AREA_SCENE) {
            area = *weights;
            gScene.area = area;
            gScene.startArea = area;
            return;
        }
        gPlayers[who].area = *weights;
        return;
    }
    start = (StartArea *)(((StageHeader *)gStageFile)->startAreas + gStageFile);
    if (who == START_AREA_SCENE) {
        start += gScene.localPlayer;
        area = start->area;
        gScene.area = area;
        gScene.startArea = area;
        return;
    }
    gPlayers[who].area = start[who].area;
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
    s32 left;

    switch (which) {
    case QUEST_TIME_LEFT:
        left = gQuest.timeLeft;
        return (left < 0) ? 0 : left;
    case QUEST_TIME_LIMIT:
        return gQuest.info->timeLimit;
    default:
        left = gQuest.timeLeft;
        if (left < 0) {
            return 0;
        }
        return left;
    }
}

/* On entering an area: loads the stage's part and spawns the quest's
 * monsters and those of the area. */
void Quest_EnterArea(s32 area)
{
    GmLoad_LoadStagePartition();
    QuestEm_SpawnAll();
    QuestEm_SpawnArea(area);
}

/* Moves em to a random spawn point of its area (StageHeader.areaSpawns):
 * 0, or -1 when the area has none. */
s32 QuestEm_PlaceAtRandomPoint(Em *em)
{
    AreaSpawns *spawnList = (AreaSpawns *)(gQuest.header->areaSpawns + gStageFile);
    EmSpawnPoint *spawn;

    for (;;) {
        if (em->area == Area_IdForCurrentTime(spawnList->area)) {
            break;
        }
        spawnList++;
        if (spawnList->area == AREA_NONE) {
            return -1;
        }
    }
    spawn = (EmSpawnPoint *)(spawnList->offset + gStageFile);
    spawn += Rand_Next(0) % spawnList->count;
    em->rotY = spawn->rotY;
    em->posX = spawn->x;
    em->posY = spawn->y;
    em->posZ = spawn->z;
    return 0;
}

/* 1 when em's quest monster may appear again: the quest not running, the
 * entry not done and some appearances left. */
s32 QuestEm_CanAppear(Em *em)
{
    QuestEm *qem;

    if (Quest_IsRunning(0) != 0) {
        return 0;
    }
    qem = QuestEm_FindForEm(em);
    if (qem != NULL) {
        if (qem->flags & QUEST_EM_DONE) {
            return 0;
        }
        if (qem->noReappear == 1) {
            return 0;
        }
        if (qem->count > 0) {
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
    QuestEm *qem;
    u16 max;

    if ((qem = QuestEm_FindForEm(em)) != NULL) {
        if (qem->flags & QUEST_EM_DONE) {
            return 0;
        }
        qem->flags = QUEST_EM_ACTIVE;
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
        qem->hp = em->hp;
        switch (qem->spawn) {
        case 1:
            if (QuestEm_PlaceAtRandomPoint(em) == 0) {
                break;
            }
        case 0:
        default:
            em->posX = qem->x;
            em->posY = qem->y;
            em->posZ = qem->z;
            em->rotY = qem->rotY;
            break;
        }
        em->area = qem->area;
        Em_InitParts(em);
        EmMine_RemoveTimed(em);
        GmEm_ApplySize(em);
        GmEm_ResetState(em);
        Em_RunAppearHook(em);
        return qem->count;
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
    QuestEm *qem;

    if ((qem = QuestEm_FindForEm(em)) != NULL) {
        if (qem->flags & (QUEST_EM_DONE | QUEST_EM_HUNTED)) {
            Quest_NoteTargetDown(em);
            return;
        }
        if (qem->large != 0) {
            gQuest.emLeft--;
        }
        qem->count--;
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
        if (qem->count > 0) {
            qem->flags = QUEST_EM_HUNTED;
        } else {
            qem->emIndex = -1;
            qem->flags = QUEST_EM_DONE;
        }
        if (em->remote == 0) {
            Quest_SendKill(qem, (Player *)em);
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
    QuestEm *qem = QuestEm_FindForEm(em);

    if (qem != NULL && !(qem->flags & QUEST_EM_DONE)) {
        qem->hp = em->hp;
        qem->emIndex = -1;
    }
}

/*
 * em was captured: as QuestEm_Hunted (once per entry), and the kind, size
 * and level (level - 3, 1 to 12) of the capture are kept for the records.
 */
void QuestEm_Captured(Em *em)
{
    QuestEm *qem;

    if ((qem = QuestEm_FindForEm(em)) != NULL) {
        if (qem->flags & QUEST_EM_CAPTURED) {
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
        if (!(qem->flags & (QUEST_EM_DONE | QUEST_EM_HUNTED))) {
            QuestEm_CapturedStub(qem, em);
            if (qem->large != 0) {
                gQuest.emLeft--;
            }
            qem->count--;
            if (qem->count > 0) {
                qem->flags = QUEST_EM_HUNTED;
            } else {
                qem->emIndex = -1;
                qem->flags = QUEST_EM_DONE;
            }
        }
        qem->flags |= QUEST_EM_CAPTURED;
    }
}

/* Shows the part break effect at the bone of em's part, the first time it breaks
 * (gPartBreakEffects lists the kinds' parts). */
void Em_PartBreakEffect(Em *em, s32 part)
{
    f32 pos[3];
    f32 vec[4];
    f32 mtx[4][4];
    PartBreakEffect *effect = gPartBreakEffects;
    u8 kind = gPartBreakEffects[0].kind;

    if (kind != 0xFF) {
        do {
            if (Em_KindMatches(em->type, kind) != 0 && effect->part == part && Em_PartBroken(em, part) == 0) {
                Mtx_Copy((f32 *) mtx, (f32 *) (Em_Bone(em, effect->bone)));
                Vec_Copy(vec, effect->offset);
                Vec_RotateSelf((f32 *) vec, (f32 *) mtx);
                pos[0] = mtx[3][0] + vec[0];
                pos[1] = mtx[3][1] + vec[1];
                pos[2] = mtx[3][2] + vec[2];
                Effect02_SpawnForOwner(em, 0, 10, 0, pos, 1.0f);
                return;
            }
            effect++;
        } while ((kind = effect->kind) != 0xFF);
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
    s32 value;
    u8 idx;
    s32 stepOut;

    for (i = 0; i < SCENE_PLAYERS; i++) {
        if (gScene.inQuest[i] != IN_QUEST_YES) {
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
                if (gScene.inQuest[i] != IN_QUEST_YES && gPlayers[i].side < 3) {
                    Quest_FinishIfIdle();
                }
            }
        }
        QuestGoal_Update();
        Quest_CheckClear(1);
        Quest_UpdateCountdown();
        gQuest.scriptPc = Quest_StepScript(pl, gQuest.scriptPc, &stepOut);
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
            value = Quest_GetTime(QUEST_TIME_LIMIT);
            gQuest.elapsed = value - Quest_GetTime(QUEST_TIME_LEFT);
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
            value = gQuest.reward /= gScene.playerCount;
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
            for (i = 0; i < SCENE_PLAYERS; i++) {
                if (gScene.inQuest[i] != IN_QUEST_YES) {
                    gQuest.acks |= 1 << i;
                }
            }
            if ((gQuest.acks & QUEST_ACKS_RANK) != QUEST_ACKS_RANK&& gQuest.ackTimer > 0 && gScene.inQuest[gScene.localPlayer] == IN_QUEST_YES) {
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
            for (i = 0; i < SCENE_PLAYERS; i++) {
                if (gScene.inQuest[i] != IN_QUEST_YES) {
                    gQuest.acks |= 1 << (i + QUEST_ACK_REWARD_SHIFT);
                }
            }
            if ((gQuest.acks & (QUEST_ACKS_RANK | QUEST_ACKS_REWARD)) != (QUEST_ACKS_RANK | QUEST_ACKS_REWARD)&& gQuest.ackTimer > 0 && gScene.inQuest[gScene.localPlayer] == IN_QUEST_YES) {
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
            for (i = 0; i < SCENE_PLAYERS; i++) {
                if (gScene.inQuest[i] != IN_QUEST_YES) {
                    gQuest.acks |= 1 << i;
                }
            }
            if ((gQuest.acks & QUEST_ACKS_RANK) != QUEST_ACKS_RANK&& gQuest.ackTimer > 0 && gScene.inQuest[gScene.localPlayer] == IN_QUEST_YES) {
                break;
            }
            gQuest.step++;
            value = Quest_GetTime(QUEST_TIME_LIMIT);
            gQuest.elapsed = value - Quest_GetTime(QUEST_TIME_LEFT);
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
    TimedSupply *step = (TimedSupply *)gQuestTimedEventTbls[gQuest.timedEventSet];

    switch (gQuest.timedEvent) {
    case QUEST_EVENT_TIMED:
        step += gQuest.timedEventStep;
        if (step->minutes == 0) {
            return;
        }
        if (step->minutes * 1800 /* frames per minute */ < gQuest.timeLeft) {
            return;
        }
        gQuest.timedEventStep++;
        if ((Rand_Next(0) & 0x7F) < step->chance) {
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
    QuestTarget *target = gQuest.targets;
    s32 i;

    for (i = 0; i < 2; i++, target++) {
        switch (target->state) {
        case TARGET_DONE:
        case TARGET_KILLS:
            break;
        case TARGET_ITEMS:
            if (Pouch_Count(&gPlayers[gScene.localPlayer], target->item) >= target->count) {
                if (target->idx > gQuest.progress) {
                    gQuest.progress = target->idx;
                    Quest_TargetMessage();
                }
                target->state = TARGET_DONE;
            }
            break;
        }
    }
}

/* A kill of monster kind `type` counts for the kill targets of that kind
 * (their `item`); the last one moves the progress on. */
void Quest_CountTarget(u16 type)
{
    QuestTarget *target = gQuest.targets;
    s32 i;

    for (i = 0; i < 2; i++, target++) {
        switch (target->state) {
        case TARGET_KILLS:
            if (target->item == type) {
                if (--target->count <= 0) {
                    if (target->idx > gQuest.progress) {
                        gQuest.progress = target->idx;
                        Quest_TargetMessage();
                    }
                    target->state = TARGET_DONE;
                }
            }
            break;
        }
    }
}

/* Clears the quest's two targets. */
void Quest_ClearTargets(void)
{
    QuestTarget *target = gQuest.targets;
    s32 i;

    for (i = 0; i < 2; i++) {
        target[i].state = 0;
        target[i].item = 0;
        target[i].count = 0;
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
Em *QuestEm_Spawn(QuestEm *qem)
{
    Em *em;
    s32 i;

    for (i = 0; i < SCENE_EM_KINDS; i++) {
        if (qem->kind == gScene.emKind[i]) {
            break;
        }
    }
    if (i >= 4) {
        return NULL;
    }
    if ((em = EmSlot_Alloc()) != NULL) {
        em->modelSlot = i;
        em->type = qem->kind;
        em->variant = qem->variant;
        em->questIndex = qem->index;
        if (gEmIsLarge[em->type] != 0) {
            em->area = qem->area;
        } else {
            em->area = gScene.area;
        }
        em->need1Gauge = qem->need1Gauge;
        em->need2Gauge = qem->need2Gauge;
        em->need3Gauge = qem->need3Gauge;
        em->spawnParam = qem->spawnParam;
        em->questKey = qem->questKey;
        em->posX = qem->x;
        em->posY = qem->y;
        em->posZ = qem->z;
        em->rotY = qem->rotY;
        em->sizePct = qem->size;
        em->level = qem->level;
        qem->emIndex = em->index;
        GmEm_Task(em);
        GmEm_ApplySize(em);
        Em_UpdateMatrix(em);
        if (qem->hp == -1) {
            qem->hp = em->hp;
            if (qem->large != 0) {
                gQuest.spawnedEm = em;
                gQuest.savedEmHp = em->hp;
            }
        } else {
            em->hp = qem->hp;
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
    QuestEm *qem;
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
        if ((qem = QuestEm_FindForEm(em)) != NULL && qem->flags != QUEST_EM_DONE && !keep) {
            if (em->type == EM_KIND_MELYNX) {
                EmMelynx_DeductReward(em);
            }
            if (em->taskPhase >= 2 || em->stateGroup == EM_GROUP_DEFEAT) {
                QuestEm_Hunted(em);
                if (qem->flags & QUEST_EM_HUNTED) {
                    qem->flags = QUEST_EM_ACTIVE;
                    qem->hp = -1;
                }
            } else {
                qem->hp = em->hp;
            }
            qem->emIndex = -1;
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
    for (i = 0; i < SCENE_EM_KINDS; i++) {
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
        searched: /* the search exits by gotos, as in the game's code */
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
    QuestEm *qem;
    s32 i;
    s32 *ids;
    s32 addYama;
    u8 *kindPtr;
    s32 k;
    s32 id;

    Effect17_SpawnArea(area);
    if (area == gScene.campArea) {
        for (i = 0; i < SCENE_EM_KINDS; i++) {
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
        for (i = 0; i < SCENE_EM_KINDS; i++) {
            if (gScene.emKind[i] <= 0) {
                gScene.emKind[i] = EM_KIND_YAMA_TSUKAMI_B;
                GmLoad_LoadEmData(gScene.map, gScene.emKind[i]);
                EmSlot_LoadModels(i);
                break;
            }
        }
    }
    ids = Stage_FindEntryData(gQuest.emSpawns, area, 0, gQuest.progress);
    qem = Stage_FindEntryData(gQuest.emSpawns, area, 1, gQuest.progress);
    if (ids != NULL) {
        for (;;) {
            id = *ids;
            if (id <= 0) {
                break;
            }
            /* the kind's model slot, if loaded (gScene.emKind read through a byte pointer, as the game does) */
            kindPtr = (u8 *)&gScene;
            for (k = 0; k < 4; k++, kindPtr++) {
                if (((SceneState *)kindPtr)->emKind[0] == id) {
                    break;
                }
            }
            if (k < 4) {
                ids++;
                continue;
            }
            for (i = 0; i < SCENE_EM_KINDS; i++) {
                if (gScene.emKind[i] <= 0) {
                    gScene.emKind[i] = id;
                    GmLoad_LoadEmData(gScene.map, gScene.emKind[i]);
                    EmSlot_LoadModels(i);
                    break;
                }
            }
            ids++;
        }
        for (;; qem++) {
            if (qem->kind <= 0) {
                break;
            }
            if (qem->flags & QUEST_EM_DONE) {
                continue;
            }
            if (qem->kind == 10) { /* the trader: only the entries of gQuest.traderVariant */
                if (gQuest.traderVariant == 0) {
                    if (qem->variant != 0) {
                        continue;
                    }
                } else {
                    if (qem->variant == 0) {
                        continue;
                    }
                }
            }
            if (qem->kind < 100) {
                QuestEm_Spawn(qem);
                qem->area = gScene.area;
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
    void *text;
    u32 *offsets;

    switch (gScene.online) {
    default:
        text = Quest_SelectedText(i);
        break;
    case 0:
        offsets = gQuest.texts;
        offsets += i;
        text = (void *)(*offsets + gStageFile);
        break;
    }
    return text;
}

/* Copies the stage-wide monster kinds (StageEntry part 0, up to 4) to
 * kinds and returns their count; 0 when the stage has none. */
s32 Quest_GetStageEmKinds(u8 *kinds)
{
    s32 *kindList;
    s32 *countList;
    s32 n = 0;
    s32 i;

    kindList = StageEntry_GetData(gQuest.stageEntry, 0);
    countList = StageEntry_GetData(gQuest.stageEntry, 1);
    if (kindList == NULL) {
        return 0;
    }
    if (countList == NULL) {
        return 0;
    }
    for (i = 0; i < SCENE_EM_KINDS; i++) {
        if (*kindList <= 0) {
            break;
        }
        *kinds = *kindList;
        n++;
        kinds++;
        kindList++;
    }
    return n;
}

/* The count of item id in the quest's item list (gQuest.rewards), 0 when absent. */
static s16 Quest_FindReward(u16 id)
{
    s16 i;

    for (i = 0; i < QUEST_REWARD_SLOTS; i++) {
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
u16 Quest_AddReward(u16 item, s16 count)
{
    s16 i;
    u16 result;
    s8 max;

    if (Quest_FindReward(item) == 0) {
        result = 5;
        for (i = 0; i < QUEST_REWARD_SLOTS; i++) {
            if (gQuest.rewards[i].id == 0 && count > 0) {
                gQuest.rewards[i].id = item;
                gQuest.rewards[i].val = count;
                result = 0;
                break;
            }
        }
        if (result == 5) {
            gQuest.rewards[gQuest.rewardNext].id = item;
            gQuest.rewards[gQuest.rewardNext].val = count;
            gQuest.rewardNext++;
            if (gQuest.rewardNext >= QUEST_REWARD_SLOTS) {
                gQuest.rewardNext = 0;
            }
        }
    } else {
        for (i = 0; i < QUEST_REWARD_SLOTS; i++) {
            if (gQuest.rewards[i].id == item) {
                max = gItemData[item].max;
                if (count > 0 && gQuest.rewards[i].val >= max) {
                    gQuest.rewards[i].val = max;
                    result = 3;
                    break;
                }
                gQuest.rewards[i].val += (s8)count;
                if (gQuest.rewards[i].val <= 0) {
                    result = 4;
                    gQuest.rewards[i].id = 0;
                    gQuest.rewards[i].val = 0;
                } else if (gQuest.rewards[i].val > max) {
                    gQuest.rewards[i].val = max;
                    result = 2;
                } else {
                    result = 1;
                }
                break;
            }
        }
    }
    /* r is not set when the second loop finds no slot of item: the game
     * then returns what its register held (a2, the caller's) */
    return result;
}

/* An item stolen from player p (the game's Melynx, em_cephadrome.c): n of
 * item off the quest's item list, noted for the scene (deductedItem,
 * deductedCount), and the list is to be given (rewardsPending). */
void Quest_DeductReward(Player *pl, u16 item, s16 n)
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
            for (i = 0; i < QUEST_REWARD_SLOTS; i++) {
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
        for (i = 0; i < QUEST_REWARD_SLOTS; i++) {
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

/*
 * Delivers up to `count` of `item` from pl's pouch for the delivery goals:
 * as many as a goal still wants, recorded in pl->delivered (a free slot for
 * an item the local player has not delivered yet). Returns the count
 * delivered: 0 when no goal wants the item.
 */
u16 Quest_Deliver(Player *pl, u16 item, s32 count)
{
    s16 i;
    s16 max = 0;
    s16 left;
    s16 delivered;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        left = (s16)QuestGoal_DeliverCountFor(item, i);
        if (left >= 0) {
            delivered = (s16)Quest_DeliveredCount(item);
            left -= delivered;
            if (left > max) {
                max = left;
            }
        }
    }
    if (max <= 0) {
        return 0;
    }
    if (Quest_CountDelivered(item, DELIVERED_LOCAL) == 0) {
        for (i = 0; i < DELIVERED_SLOTS; i++) {
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
        for (i = 0; i < DELIVERED_SLOTS; i++) {
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

/*
 * Records that player pl has delivered `count` of `item` in all (another
 * player's QMSG_DELIVERED). Returns 0 for a new entry, 1 for an updated one,
 * 2 when the count reaches what a goal still wants (or none wants it).
 * `have` holds what a goal still wants, then what pl had delivered.
 */
u8 Quest_SetDelivered(Player *pl, u16 item, s32 count)
{
    s16 i;
    s16 have;
    s16 max = 0;
    s16 delivered;
    u8 result;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        have = (s16)QuestGoal_DeliverCountFor(item, i);
        if (have >= 0) {
            delivered = (s16)Quest_DeliveredCount(item);
            have -= delivered;
            if (have > max) {
                max = have;
            }
        }
    }
    if (max <= 0) {
        return 2;
    }
    for (have = 0, i = 0; i < DELIVERED_SLOTS; i++) {
        if (pl->delivered[i].item == item) {
            have = pl->delivered[i].count;
        }
    }
    if (have == 0) {
        result = 0;
        for (i = 0; i < DELIVERED_SLOTS; i++) {
            if (pl->delivered[i].item == 0) {
                pl->delivered[i].item = item;
                if ((s16)count >= max) {
                    result = 2;
                }
                pl->delivered[i].count = (s8)count;
                break;
            }
        }
    } else {
        result = 1;
        for (i = 0; i < DELIVERED_SLOTS; i++) {
            if (pl->delivered[i].item == item) {
                if ((s16)count >= max) {
                    result = 2;
                }
                pl->delivered[i].count = (s8)count;
                break;
            }
        }
    }
    return result;
}

/* Returns how many of `item` were delivered by `who` (DELIVERED_*). */
s16 Quest_CountDelivered(u16 item, s32 who)
{
    s16 n = 0;
    Player *pl;
    s32 i;
    s32 j;

    switch (who) {
    case DELIVERED_ALL:
        pl = gPlayers;
        for (j = 0; j < gScene.playerCount; j++, pl++) {
            for (i = 0; i < DELIVERED_SLOTS; i++) {
                if (pl->delivered[i].item == item) {
                    n += pl->delivered[i].count;
                }
            }
        }
        break;
    case DELIVERED_LOCAL:
        pl = &gPlayers[gScene.localPlayer];
        for (i = 0; i < DELIVERED_SLOTS; i++) {
            if (pl->delivered[i].item == item) {
                return pl->delivered[i].count;
            }
        }
        break;
    default:
        pl = &gPlayers[who - DELIVERED_PLAYER0];
        for (i = 0; i < DELIVERED_SLOTS; i++) {
            if (pl->delivered[i].item == item) {
                return pl->delivered[i].count;
            }
        }
        break;
    }
    return n;
}

/* Tells the other players how many of `item` the local player has delivered (QMSG_DELIVERED), with the notice. */
void Quest_DeliveryMessage(u16 item)
{
    gQuest.msgArg0 = 0;
    gQuest.msgArg1 = item;
    gQuest.msgType = QMSG_DELIVERED;
    gQuest.msgArg2 = Quest_CountDelivered(item, DELIVERED_LOCAL);
    QSync_SendState(6, gScene.localPlayer);
    Notice_Push(1, 8, item);
}

/*
 * Delivers from pl's pouch every item a delivery goal of the way `mode`
 * wants (0: QUEST_GOAL_DELIVER_BOX, 1: _SET, 2: _HOW2), with the messages and
 * a jingle; only while the quest runs. The game's trap-setting state calls it
 * with mode 1.
 */
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
    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        item = QuestGoal_DeliverItem(i);
        if (item == 0) {
            continue;
        }
        switch (mode) {
        case 0:
            if (QuestGoal_InitialFlags(i, QUEST_GOAL_DELIVER_HOW) != QUEST_GOAL_DELIVER_BOX) {
                continue;
            }
            break;
        case 1:
            if (QuestGoal_InitialFlags(i, QUEST_GOAL_DELIVER_HOW) != QUEST_GOAL_DELIVER_SET) {
                continue;
            }
            break;
        case 2:
            if (QuestGoal_InitialFlags(i, QUEST_GOAL_DELIVER_HOW) != QUEST_GOAL_DELIVER_HOW2) {
                continue;
            }
            break;
        }
        for (j = 0; j < POUCH_SLOTS; j++) {
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

/*
 * For the delivery spot's menu: sets *out to the item goal `idx` wants
 * handed in (QUEST_GOAL_DELIVER_BOX) and the count it still wants from all
 * the players. Returns 0 when there is no such goal (or the quest is not
 * running), 1 when it has all it wants, 2 when it wants more.
 */
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
        if (QuestGoal_InitialFlags(i, QUEST_GOAL_DELIVER_HOW) != QUEST_GOAL_DELIVER_BOX) {
            return 0;
        }
        i = QuestGoal_DeliverCount(i);
        if (i >= 0) {
            out->item = item;
            i -= Quest_CountDelivered(item, DELIVERED_ALL);
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

/* Quest_Deliver, with the message to the others and a jingle when anything was delivered; returns the count delivered. */
u16 Quest_DeliverItem(Player *pl, u16 item, s16 count)
{
    s32 n = Quest_Deliver(pl, item, count);

    if (n > 0) {
        Quest_DeliveryMessage(item);
        Jingle_Play(pl, 0xB7);
    }
    return n;
}

/*
 * The quest's tables: the trader's variant rolls, the special point's age
 * ranks, the monster sizes' default ranges and appearance caps, the reward
 * bands and draw chances, and the timed supply events' tables.
 */
extern u32 D_006AEDB8[];
s8 gTraderVariantRolls[16] = {
#include "sub_main/0044B450.inc"
};
u16 gSpecialMineAgeRanks[8] = {
#include "sub_main/0044B460.inc"
};
EmSizeRange gEmSizeRangeDefaults[5] = {
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
u32 gRewardRatesMain[16] = {
#include "sub_main/0044B550.inc"
};
u32 gRewardRatesMainGoal[12] = {
#include "sub_main/0044B590.inc"
};
u32 gRewardRatesSub[8] = {
#include "sub_main/0044B5C0.inc"
};
u32 gRewardRatesOther[2] = {
#include "sub_main/0044B5E0.inc"
};
u32 gRewardRatesDrill[3] = {
#include "sub_main/0044B5E8.inc"
};
u32 gRewardRatesPart[2] = {
#include "sub_main/0044B5F8.inc"
};
u32 gRewardRatesCapture[2] = {
#include "sub_main/0044B600.inc"
};
u32 gRewardRatesTraining[3] = {
#include "sub_main/0044B608.inc"
};
#pragma explicit_zero_data on
u32 D_0044B618[2] = {
    0, 0,
};
#pragma explicit_zero_data reset
s8 *gRewardRateTables[8] = {
    (s8 *)gRewardRatesMain, (s8 *)gRewardRatesMainGoal, (s8 *)gRewardRatesSub, (s8 *)gRewardRatesOther,
    (s8 *)gRewardRatesDrill, (s8 *)gRewardRatesPart, (s8 *)gRewardRatesCapture, (s8 *)gRewardRatesTraining,
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

/*
 * ---- The gathering points placed at run time (mine_point.h) ----
 *
 * Up to MINE_POINT_COUNT points, placed by the monsters' carcasses and
 * drops and by the special point, kept in gMinePoints with the active list
 * gMineActive (oldest first, gQuest.mineCount entries). A point runs out
 * when its count or its timer reaches 0; with every slot in use, the oldest
 * point without MINE_POINT_KEEP makes room for a new one. Gather_AtMinePoint
 * rolls the item from the game overlay's gGatherItemTables
 * (Gather_RollMineItem, random stream 0).
 */

u16 Gather_RollMineItem(Player *p, u16 table);


extern GatherEntry *gGatherItemTables[];

s32 QuestGoal_InitialFlags(u16 i, s32 mask);
s32 QuestGoal_IsDone(s32 i);
s32 Player_HasSkill(Player *p, u8 skill);
s32 SpecialMine_Gather(Player *p);
void Effect0D_SpawnAt(f32 *pos, s32 kind, f32 scale);
f32 Math_Sqrtf(f32 x);   /* sqrtf */
u16 Rand_Next(s32 stream);
void MineActive_RemoveAt(s8 idx);
void MineActive_Remove(s8 slot);
void MineActive_Append(s8 slot);

/*
 * Returns nonzero once a set-down delivery goal (QUEST_GOAL_DELIVER_SET) is
 * done: 2 under quest flag 0x40000000, else 1; the game's stage objects
 * pick their animation by it.
 */
s32 QuestGoal_DeliveryDone(void)
{
    s32 i;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if (QuestGoal_DeliverItem(i) != 0 && QuestGoal_InitialFlags(i, QUEST_GOAL_DELIVER_HOW) == QUEST_GOAL_DELIVER_SET && QuestGoal_IsDone(i) == 1) {
            return (gQuest.flags & 0x40000000) ? 2 : 1;
        }
    }
    return 0;
}

/*
 * Rolls an item from gathering table `tableIdx` (weights, random stream 0);
 * item 0x290 in pl's pouch moves the roll one step up the table. Returns
 * GATHER_NO_ITEM when the roll falls past the table.
 */
u16 Gather_RollMineItem(Player *pl, u16 tableIdx)
{
    u16 sum;
    GatherEntry *entry;
    u16 roll;
    GatherEntry **table;

    sum = 0;
    table = &gGatherItemTables[tableIdx];
    for (entry = *table; entry->weight != GATHER_END; entry++) {
        sum += entry->weight;
    }
    roll = Rand_Next(0) % sum;
    if (Pouch_Count(pl, 0x290) > 0 && roll < sum - 1) {
        roll++;
    }
    sum = 0;
    for (entry = *table; entry->weight != GATHER_END; entry++) {
        sum += entry->weight;
        if (roll < sum) {
            return entry->item;
        }
    }
    return GATHER_NO_ITEM;
}

/* Frees every mining point and empties the active list. */
void MinePoint_ResetAll(void)
{
    MinePoint *point;
    s32 i;

    gQuest.mineCount = 0;
    point = gMinePoints;
    for (i = 0; i < MINE_POINT_COUNT; i++, point++) {
        point->table = MINE_POINT_FREE;
        gMineActive[i] = -1;
    }
}

/*
 * Counts down the timer of each timed point and removes the spent or
 * expired ones; every 64 frames the others spawn their glint (effect 0x15)
 * 20 units above them, at scale 0.5, shrinking to 0.2 over the last 900
 * frames.
 */
void MinePoint_UpdateAll(void)
{
    s32 i;
    MinePoint *point;
    f32 pos[3];

    for (i = 0; i < gQuest.mineCount; i++) {
        point = &gMinePoints[gMineActive[i]];
        if (point->table == MINE_POINT_FREE) {
            continue;
        }
        if (point->timer == 0) {
            continue;
        }
        if (point->count <= 0 || --point->timer <= 0) {
            MinePoint_Remove(gMineActive[i]);
            continue;
        }
        if ((point->timer & 0x3F) != 0) {
            continue;
        }
        pos[0] = point->x;
        pos[1] = point->y + 20.0f;
        pos[2] = point->z;
        Effect0D_SpawnAt(pos, 0x15, point->timer < 900 ? 0.2f + 0.3f * (point->timer / 900.0f) : 0.5f);
    }
}

/*
 * Adds a point like `src` (at `pos` if given, else at src's position),
 * first making room (MinePoint_RemoveOldest); returns its slot, or -1 when
 * none is free. Unless noBonus, the local player's extra-carve skill gives
 * one more gather.
 */
s32 MinePoint_Add(MinePoint *src, f32 *pos, u8 noBonus)
{
    MinePoint *point;
    s32 i;

    point = gMinePoints;
    MinePoint_RemoveOldest();
    for (i = 0; i < MINE_POINT_COUNT; i++, point++) {
        if (point->table != 0xFFFF) {
            continue;
        }
        if (pos == NULL) {
            point->x = src->x;
            point->y = src->y;
            point->z = src->z;
        } else {
            point->x = pos[0];
            point->y = pos[1];
            point->z = pos[2];
        }
        point->radius = src->radius;
        point->table = src->table;
        point->count = src->count;
        if (noBonus == 0 && Player_HasSkill(&gPlayers[gScene.localPlayer], SKILL_EXTRA_CARVE) == 1) {
            point->count++;
        }
        point->action = src->action;
        point->area = src->area;
        point->flags = src->flags;
        point->timer = src->timer;
        MineActive_Append(i);
        return i;
    }
    return -1;
}

/* Returns the gathers left at point `slot`. */
s32 MinePoint_Count(s32 slot)
{
    return gMinePoints[slot].count;
}

/* Frees point `slot` (-1: none) and takes it off the active list. */
void MinePoint_Remove(s32 slot)
{
    if (slot != -1) {
        gMinePoints[slot].table = MINE_POINT_FREE;
        MineActive_Remove(slot);
    }
}

/* Moves point `slot` to pos. */
void MinePoint_SetPos(s32 slot, f32 *pos)
{
    gMinePoints[slot].x = pos[0];
    gMinePoints[slot].y = pos[1];
    gMinePoints[slot].z = pos[2];
}

/* Moves point `slot` to `area`. */
void MinePoint_SetArea(s32 slot, u16 area)
{
    gMinePoints[slot].area = area;
}

/*
 * Returns the action (MINE_ACTION_*) of the point the object stands at, or
 * 0xFFFF when there is none: in its area, from 200 below to 100 above the
 * point, and within its radius.
 */
u16 MinePoint_FindNearPlayer(void *obj)
{
    ObjHead *head = obj;
    MinePoint *point;
    s32 i;
    f32 dx;
    f32 dz;

    point = gMinePoints;
    for (i = 0; i < MINE_POINT_COUNT; i++, point++) {
        if (point->table == MINE_POINT_FREE) {
            continue;
        }
        if (point->area != head->area) {
            continue;
        }
        if (head->pos[1] < point->y - 200.0f) {
            continue;
        }
        if (!(head->pos[1] < point->y + 100.0f)) {
            continue;
        }
        dx = head->pos[0] - point->x;
        dz = head->pos[2] - point->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= point->radius)) {
            continue;
        }
        return point->action;
    }
    return 0xFFFF;
}

/*
 * Gathers at the point pl stands at (as MinePoint_FindNearPlayer): returns
 * the item (the special point's reward from SpecialMine_Gather),
 * GATHER_USED_UP when the point has no gathers left, 0 when there is none.
 */
u16 Gather_AtMinePoint(Player *pl)
{
    MinePoint *point;
    s32 i;
    u16 item;
    f32 dx;
    f32 dz;

    point = gMinePoints;
    for (i = 0; i < MINE_POINT_COUNT; i++, point++) {
        if (point->table == MINE_POINT_FREE) {
            continue;
        }
        if (point->area != pl->area) {
            continue;
        }
        if (pl->pos[1] < point->y - 200.0f) {
            continue;
        }
        if (!(pl->pos[1] < point->y + 100.0f)) {
            continue;
        }
        dx = pl->pos[0] - point->x;
        dz = pl->pos[2] - point->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= point->radius)) {
            continue;
        }
        if (point->count > 0) {
            if (point->table == MINE_TABLE_SPECIAL) {
                return SpecialMine_Gather(pl);
            }
            item = Gather_RollMineItem(pl, point->table & GATHER_TABLE_MASK);
            if (point->count != GATHER_UNLIMITED) {
                point->count--;
            }
        } else {
            item = GATHER_USED_UP;
        }
        return item;
    }
    return 0;
}

/* With every point in use, frees the oldest one without MINE_POINT_KEEP (or the newest). */
void MinePoint_RemoveOldest(void)
{
    s32 i;

    if (gQuest.mineCount >= MINE_POINT_COUNT) {
        for (i = 0; i < gQuest.mineCount - 1; i++) {
            if (!(gMinePoints[gMineActive[i]].flags & MINE_POINT_KEEP)) {
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

/* Takes point `slot` off the active list. */
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

/* Frees the monsters' dropped items and every point without MINE_POINT_KEEP. */
void MinePoint_RemoveTemporary(void)
{
    s32 i;
    Em *em;

    em = gEms;
    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0 && em->isEm != 0 && em->mineSlot2 != -1) {
            EmMine_RemoveTimed(em);
        }
    }
    for (i = gQuest.mineCount - 1; i >= 0; i--) {
        if (!(gMinePoints[gMineActive[i]].flags & MINE_POINT_KEEP)) {
            MinePoint_Remove(gMineActive[i]);
        }
    }
}

/* Puts point `slot` at the end of the active list. */
void MineActive_Append(s8 slot)
{
    gMineActive[gQuest.mineCount] = slot;
    gQuest.mineCount++;
}

/* Empty: called on a filled-in point before it is added (EmMine_Create, the monsters carcass points). */
void MinePoint_PrepareStub(MinePoint *point)
{
}

/*
 * ---- The drill's statistics, the quest's monsters, timers and goals ----
 */

#include "ee_stdarg.h"

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
extern SpecialMineReward *gSpecialMineRewards[];
extern u16 gSpecialMinePoints[];
extern u8 gEmShowsOnMap[];
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
void MinePoint_PrepareStub(MinePoint *pt);
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

/*
 * Adds em's carcass point from entry idx of its kind's list
 * (gEmMinePoints), its radius scaled by the monster's size; it follows the
 * entry's bone, or the monster. Returns the slot (Em.mineSlot), -1 when
 * there is none (and in a must-order or versus quest). A Melynx's point is
 * picked up, every other one carved.
 */
s8 EmMine_Create(Em *em, s32 idx)
{
    MinePoint point;
    f32 pos[3];
    EmMineEntry *entry;

    if (Quest_IsMustOrderOrVersus(em) == 1) {
        em->mineSlot = -1;
        return em->mineSlot;
    }
    entry = gEmMinePoints[em->type];
    if (entry == NULL) {
        em->mineSlot = -1;
    } else {
        entry += idx;
        if (entry->table[gStageHeader->tier] == GATHER_NO_ITEM) {
            em->mineSlot = -1;
        } else {
            point.radius = em->sizePct * (entry->radius * em->bodyScale) / 100.0f;
            point.table = entry->table[gStageHeader->tier];
            point.count = entry->count;
            if (em->type == EM_KIND_MELYNX) {
                point.action = MINE_ACTION_PICK;
            } else {
                point.action = MINE_ACTION_CARVE;
            }
            MinePoint_PrepareStub(&point);
            point.area = em->area;
            point.timer = 0;
            point.flags = entry->flags;
            em->mineBone = entry->bone;
            if (em->mineBone != 0) {
                Em_BonePos(em, em->mineBone, pos);
                pos[1] = em->floorY;
                em->mineSlot = MinePoint_Add(&point, pos, 0);
            } else {
                em->mineSlot = MinePoint_Add(&point, &em->posX, 0);
            }
        }
    }
    return em->mineSlot;
}

/* Adds a copy of point without a time limit; -1 in a must-order or versus quest. */
s32 MinePoint_AddUntimed(MinePoint *point, f32 *pos, u8 noBonus)
{
    if (Quest_IsMustOrderOrVersus(point) == 1) {
        return -1;
    }
    point->timer = 0;
    return MinePoint_Add(point, pos, noBonus);
}

/*
 * Drops em's items at pos: a point to pick up, from entry idx of its
 * kind's list (gEmMinePoints2), with Em.mineCount2 gathers, for 1800
 * frames (EmMine_UpdateTimed counts them), once at a time. Blangonga and
 * Congalala always drop; the others by the entry's chance out of 32.
 * Returns the slot (Em.mineSlot2), -1 when none; only for a monster in the
 * local player's area.
 */
s8 EmMine_CreateTimed(Em *em, f32 *pos, s32 idx)
{
    MinePoint point;
    EmMineEntry2 *entry;

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
    entry = gEmMinePoints2[em->type];
    if (entry == NULL) {
        em->mineSlot2 = -1;
    } else {
        entry += idx;
        if (em->type != EM_KIND_BLANGONGA && em->type != EM_KIND_CONGALALA && (Rand_Next(0) & 0x1F) > entry->chance) {
            em->mineSlot2 = -1;
            return em->mineSlot2;
        }
        if (entry->table[gStageHeader->tier] == GATHER_NO_ITEM) {
            em->mineSlot2 = -1;
        } else {
            point.radius = 150.0f;
            point.table = entry->table[gStageHeader->tier];
            point.count = em->mineCount2;
            point.action = MINE_ACTION_PICK;
            point.area = em->area;
            point.timer = 0;
            point.flags = 0;
            em->mineSlot2 = MinePoint_Add(&point, pos, 1);
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

/*
 * EmMine_CreateTimed's drop from entry 0, as a point of its own: timed by
 * the point itself (MinePoint.timer), and not kept in Em.mineSlot2.
 */
void EmMine_Drop(Em *em, f32 *pos)
{
    MinePoint point;
    EmMineEntry2 *entry;

    if (Quest_IsMustOrderOrVersus(em) == 1) {
        return;
    }
    if (em->area != gScene.area) {
        return;
    }
    entry = gEmMinePoints2[em->type];
    if (entry == NULL) {
        return;
    }
    if (em->type != EM_KIND_BLANGONGA && em->type != EM_KIND_CONGALALA && (Rand_Next(0) & 0x1F) > entry->chance) {
        return;
    }
    if (entry->table[gStageHeader->tier] == GATHER_NO_ITEM) {
        return;
    }
    point.radius = 150.0f;
    point.table = entry->table[gStageHeader->tier];
    point.count = em->mineCount2;
    point.action = MINE_ACTION_PICK;
    point.area = em->area;
    point.timer = 1800;
    point.flags = 0;
    MinePoint_Add(&point, pos, 1);
    Sound_PlaySeAtPosChecked(1, 0x8C, 0, pos, 1, 0);
}

/* Frees em's dropped items (Em.mineSlot2). */
void EmMine_RemoveTimed(Em *em)
{
    MinePoint_Remove(em->mineSlot2);
    em->mineSlot2 = -1;
}

/*
 * Keeps em's dropped items in step: copies the gathers left back to
 * Em.mineCount2, counts the time down (freeing the point at 0 or when
 * empty), and every 64 frames spawns the glint, as MinePoint_UpdateAll.
 */
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

/* Adds a carcass point from entry 1 of the held monster's kind list at pos, in the holder's area; returns its slot or -1. */
s32 EmHolder_CreateMine(EmHolder *holder, s32 unused, f32 *pos)
{
    Em *em = holder->em;
    MinePoint point;
    EmMineEntry *entry;

    if (Quest_IsMustOrderOrVersus(holder) == 1) {
        return -1;
    }
    entry = gEmMinePoints[em->type];
    if (entry == NULL) {
        return -1;
    }
    if (entry[1].table[gStageHeader->tier] == GATHER_NO_ITEM) {
        return -1;
    }
    point.radius = entry[1].radius;
    point.table = entry[1].table[gStageHeader->tier];
    point.count = entry[1].count;
    point.action = MINE_ACTION_CARVE;
    point.area = holder->area;
    point.timer = 0;
    point.flags = entry[1].flags;
    return MinePoint_Add(&point, pos, 0);
}

/* Moves em's carcass point with it (bone Em.mineBone, or its position); returns the carves left, -1 when none (an empty point is freed). */
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

/* Frees em's carcass point. */
void EmMine_Remove(Em *em)
{
    MinePoint_Remove(em->mineSlot);
    em->mineSlot = -1;
}

/*
 * The special point: the local player makes it by using an item where it
 * stands (gSpecialMinePoint holds the item, 0: none). Gathered later, it
 * gives item 0x2F7 and a reward that improves with the point's age
 * (gSpecialMineAge, ranked by gSpecialMineAgeRanks).
 */

/* Clears the special point. */
void SpecialMine_Clear(void)
{
    gSpecialMinePoint = 0;
}

/* Returns 1 when there is no special point. */
s32 SpecialMine_IsFree(Player *unused0)
{
    return gSpecialMinePoint == 0;
}

/*
 * Makes the special point where the local player pl stands, from the item
 * it uses (one gather, kept to the end, radius 200), with its marker
 * object; returns 1 when made, 0 for another player or when one exists.
 */
s32 SpecialMine_Create(Player *pl)
{
    MinePoint point;

    if (!Player_IsLocal(pl)) {
        return 0;
    }
    if (gSpecialMinePoint != 0) {
        return 0;
    }
    point.radius = 200.0f;
    point.table = MINE_TABLE_SPECIAL;
    point.count = 1;
    point.action = MINE_ACTION_SPECIAL;
    point.area = (u8)pl->area;
    point.timer = 0;
    point.flags = MINE_POINT_KEEP;
    gQuest.specialMinePos[0] = pl->pos[0];
    gQuest.specialMinePos[1] = pl->pos[1];
    gQuest.specialMinePos[2] = pl->pos[2];
    gQuest.specialMineSlot = MinePoint_Add(&point, gQuest.specialMinePos, 1);
    gSpecialMineAge = 0;
    gQuest.specialMineArea = pl->area;
    gSpecialMinePoint = pl->useItem;
    StageObj_K0E_CreateAt(gQuest.specialMinePos, 10);
    return 1;
}

/*
 * Gathers the special point for the local player pl: gives item 0x2F7 and
 * returns the reward for the point's item at its rank (the list of
 * gSpecialMineRewards found by the item in gSpecialMinePoints), and frees
 * the point. Returns GATHER_NO_ITEM when there is none, the pouch is full,
 * or the item has no list.
 */
s32 SpecialMine_Gather(Player *pl)
{
    s32 i;
    s32 rank;
    SpecialMineReward *reward;

    if (!Player_IsLocal(pl)) {
        return GATHER_NO_ITEM;
    }
    if (gSpecialMinePoint == 0) {
        return GATHER_NO_ITEM;
    }
    if (Pouch_Space(pl, 0x2F7) <= 0) {
        return GATHER_NO_ITEM;
    }
    Pouch_Add(pl, 0x2F7, 1);
    Notice_Push(1, 0, 0x2F7);
    rank = SpecialMine_Rank();
    for (i = 0;; i++) {
        if (gSpecialMinePoints[i] == 0xFFFF /* the list end */) {
            return GATHER_NO_ITEM;
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

/* Returns the special point's rank, 0 to 5, from its age (gSpecialMineAge against gSpecialMineAgeRanks). */
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
    va_list args;

    va_start(args, fmt);
    vsprintf(buf, fmt, args);
}

/* The quest monsters of area a hunted with appearances left come back
 * active, at full health (area entries of the current progress). */
void QuestEm_ResetArea(s32 area)
{
    QuestEm *qem;

    if (Stage_FindEntryData((s32 *) gQuestEmAreaSets, area, 0, gQuest.progress) == 0) {
        return;
    }
    qem = (QuestEm *)Stage_FindEntryData((s32 *) gQuestEmAreaSets, area, 1, gQuest.progress);
    for (;; qem++) {
        if (qem->kind <= 0) {
            break;
        }
        if (qem->flags & QUEST_EM_HUNTED) {
            qem->flags = QUEST_EM_ACTIVE;
            qem->hp = -1;
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
    u16 size;

    tbl = gEmSizeRollTbls[gEmSizeRollPicks[Quest_Random() & 0x1F]];
    size = tbl[Quest_Random() & 0x1F];
    if (size < gEmSizeRanges[gStageHeader->sizeRange].min) {
        size = gEmSizeRanges[gStageHeader->sizeRange].min;
    }
    if (size > gEmSizeRanges[gStageHeader->sizeRange].max) {
        size = gEmSizeRanges[gStageHeader->sizeRange].max;
    }
    return size;
}

/* Rolls the level of quest monster q (weighted table by kind), clamped to 0..16. */
void QuestEm_RollLevel(QuestEm *qem)
{
    u16 sum;
    s8 *entry;
    u16 roll;
    u16 running;
    s8 levelAdd;

    if (gQuest.flags & QUEST_FLAG_FIXED_LEVEL) {
        qem->level = gQuest.emLevel;
        return;
    }
    entry = gEmLevelTables[gEmLevelTableIdx[qem->kind][Daytime_Get()]];
    sum = 0;
    {
        s8 *weight;

        for (weight = entry; *weight != -1; weight += 2) {
            sum += *weight;
        }
    }
    roll = Quest_Random() % sum;
    running = 0;
    while (*entry != -1) {
        if (roll < (running += *entry++)) {
            break;
        }
        entry++;
    }
    levelAdd = *entry + gEmLevelSeasonAdd[qem->kind][(u8)Quest_GetSeason()];
    if (levelAdd > 2) {
        levelAdd = 2;
    }
    qem->level = gQuest.emLevel + levelAdd;
    if (qem->level > 16) {
        qem->level = 16;
    }
    if (qem->level < 0) {
        qem->level = 0;
    }
}

/* Initialises quest monster q with index *n and counts it. */
void QuestEm_Init(QuestEm *qem, s32 *n)
{
    qem->flags = QUEST_EM_ACTIVE;
    qem->hp = -1;
    qem->emIndex = -1;
    qem->index = *n;
    QuestEm_RollLevel(qem);
    if ((qem->large = gEmShowsOnMap[qem->kind]) != 0) {
        gQuest.emLeft += qem->count;
    }
    (*n)++;
}

/* Returns 1 for the monster kinds whose size is rolled. */
s32 Em_HasRandomSize(s16 kind)
{
    switch (kind) {
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_CEPHADROME:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
    case EM_KIND_KIRIN:
    case EM_KIND_BULLDROME:
    case EM_KIND_YIAN_GARUGA:
    case EM_KIND_DAIMYO_HERMITAUR:
    case EM_KIND_SHOGUN_CEANATAUR:
    case EM_KIND_CONGALALA:
    case EM_KIND_BLANGONGA:
    case EM_KIND_RAJANG:
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_CHAMELEOS:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        return 1;
    }
    return 0;
}

/* Same list as Em_HasRandomSize. */
s32 Em_HasRandomSize2(s16 kind)
{
    switch (kind) {
    case EM_KIND_RATHIAN:
    case EM_KIND_GOLD_RATHIAN:
    case EM_KIND_PINK_RATHIAN:
    case EM_KIND_YIAN_KUT_KU:
    case EM_KIND_BLUE_YIAN_KUT_KU:
    case EM_KIND_CEPHADROME:
    case EM_KIND_RATHALOS:
    case EM_KIND_SILVER_RATHALOS:
    case EM_KIND_AZURE_RATHALOS:
    case EM_KIND_DIABLOS:
    case EM_KIND_BLACK_DIABLOS:
    case EM_KIND_KHEZU:
    case EM_KIND_RED_KHEZU:
    case EM_KIND_GRAVIOS:
    case EM_KIND_BLACK_GRAVIOS:
    case EM_KIND_GYPCEROS:
    case EM_KIND_PURPLE_GYPCEROS:
    case EM_KIND_PLESIOTH:
    case EM_KIND_GREEN_PLESIOTH:
    case EM_KIND_BASARIOS:
    case EM_KIND_MONOBLOS:
    case EM_KIND_WHITE_MONOBLOS:
    case EM_KIND_VELOCIDROME:
    case EM_KIND_GENDROME:
    case EM_KIND_IODROME:
    case EM_KIND_KIRIN:
    case EM_KIND_BULLDROME:
    case EM_KIND_YIAN_GARUGA:
    case EM_KIND_DAIMYO_HERMITAUR:
    case EM_KIND_SHOGUN_CEANATAUR:
    case EM_KIND_CONGALALA:
    case EM_KIND_BLANGONGA:
    case EM_KIND_RAJANG:
    case EM_KIND_KUSHALA_DAORA:
    case EM_KIND_RUSTED_KUSHALA_DAORA:
    case EM_KIND_CHAMELEOS:
    case EM_KIND_LUNASTRA:
    case EM_KIND_TEOSTRA:
        return 1;
    }
    return 0;
}

/* Initialises the monster sets at set (0x10 bytes each, until one is empty) and rolls their sizes. */
void QuestEm_InitSets(u8 *set, s32 *n)
{
    s32 count;
    QuestEm *qem;

    for (;; set += 0x10) {
        count = (s32)StageEntry_GetData((struct StageEntry *) set, 0);
        qem = (QuestEm *)StageEntry_GetData((struct StageEntry *) set, 1);
        if (count == 0) {
            break;
        }
        if (qem == NULL) {
            continue;
        }
        for (;; qem++) {
            if (qem->kind <= 0) {
                break;
            }
            QuestEm_Init(qem, n);
            if (Em_HasRandomSize(qem->kind) == 1) {
                qem->size = gStageHeader->emSize;
                qem->size = qem->size * QuestEm_RollSize() / 100;
                qem->size += gEmSizeSeasonAdd[qem->kind][(u8)Quest_GetSeason()];
            } else {
                qem->size = 100;
            }
        }
    }
}

/* Initialises the monster sets of each area list (size 100). */
void QuestEm_InitAreaSets(u8 **list, s32 *n)
{
    QuestEm *qem;
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
            qem = (QuestEm *)StageEntry_GetData((struct StageEntry *) set, 1);
            set += 0x10;
            if (count == 0) {
                break;
            }
            if (qem == NULL) {
                continue;
            }
            for (;; qem++) {
                if (qem->kind <= 0) {
                    break;
                }
                QuestEm_Init(qem, n);
                qem->size = 100;
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
    QuestEm *qem;
    s32 *ids;
    s32 i;
    void *em;

    ids = (s32 *)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 0);
    qem = (QuestEm *)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 1);
    if (Quest_IsVersus() == 1) {
        for (i = 0; i < 2; i++) {
            if (gNextQuest.teams[i].emKind != 0) {
                ids[i] = gNextQuest.teams[i].emKind;
                qem[i].kind = gNextQuest.teams[i].emKind;
                qem[i].count = 1;
                qem[i].rotY = gVersusEmAngles[i];
                qem[i].x = gVersusEmPos[i][0];
                qem[i].y = gVersusEmPos[i][1];
                qem[i].z = gVersusEmPos[i][2];
            }
        }
    }
    if (ids != NULL && qem != NULL) {
        while (1) {
            if (*ids <= 0) {
                break;
            }
            for (i = 0; i < SCENE_EM_KINDS; i++) {
                if (gScene.emKind[i] == *ids) {
                    break;
                }
            }
            if (i < 4) {
                ids++;
                continue;
            }
            for (j = 0; j < SCENE_EM_KINDS; j++) {
                if (gScene.emKind[j] <= 0) {
                    gScene.emKind[j] = *ids;
                    GmLoad_LoadEmData(gScene.map, gScene.emKind[j]);
                    EmSlot_LoadModels(j);
                    break;
                }
            }
            ids++;
        }
        for (;; qem++) {
            if (qem->kind <= 0) {
                break;
            }
            if ((em = QuestEm_Spawn(qem)) != NULL && Quest_IsVersus() == 1) {
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
    QuestEm *qem;

    for (i = 0; i <= gQuest.progress; i++) {
        count = (s32)Stage_FindEntryData((s32 *) gQuestEmAreaSets, em->area, 0, i);
        qem = (QuestEm *)Stage_FindEntryData((s32 *) gQuestEmAreaSets, em->area, 1, i);
        if (count == 0) {
            continue;
        }
        for (;; qem++) {
            if (qem->kind <= 0) {
                break;
            }
            if (em->index == qem->emIndex) {
                return qem;
            }
        }
    }
    count = (s32)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 0);
    qem = (QuestEm *)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 1);
    if (count == 0) {
        return NULL;
    }
    if (qem == NULL) {
        return NULL;
    }
    for (;; qem++) {
        if (qem->kind <= 0) {
            break;
        }
        if (em->index == qem->emIndex) {
            return qem;
        }
    }
    return NULL;
}

/* Finds the quest monster with index idx in area; NULL when none. */
QuestEm *QuestEm_FindByIndex(s16 idx, u16 area)
{
    s32 i;
    s32 count;
    QuestEm *qem;

    for (i = 0; i <= gQuest.progress; i++) {
        count = (s32)Stage_FindEntryData((s32 *) gQuestEmAreaSets, area, 0, i);
        qem = (QuestEm *)Stage_FindEntryData((s32 *) gQuestEmAreaSets, area, 1, i);
        if (count == 0) {
            continue;
        }
        for (;; qem++) {
            if (qem->kind <= 0) {
                break;
            }
            if (idx == qem->index) {
                return qem;
            }
        }
    }
    count = (s32)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 0);
    qem = (QuestEm *)StageEntry_GetData((struct StageEntry *) gQuestEmSets, 1);
    if (count == 0) {
        return NULL;
    }
    if (qem == NULL) {
        return NULL;
    }
    for (;; qem++) {
        if (qem->kind <= 0) {
            break;
        }
        if (idx == qem->index) {
            return qem;
        }
    }
    return NULL;
}

/*
 * Counts the quest's timers by `ticks`, in 1/30 s units, except while an event
 * plays or the input is locked: the time left and the countdown go down to
 * 0, and the special mining point's age goes up to 90000 (50 minutes).
 */
void Quest_UpdateTimers(s32 ticks)
{
    if (gScene.eventRunning == 0 && gQuest.inputLock <= 0) {
        if (gQuestTimeLeft > 0) {
            gQuestTimeLeft -= ticks;
            if (gQuestTimeLeft < 0) {
                gQuestTimeLeft = 0;
            }
        }
        if (gSpecialMinePoint != 0) {
            gSpecialMineAge += ticks;
            if (gSpecialMineAge > 90000) {
                gSpecialMineAge = 90000;
            }
        }
        if (gQuestCountdown > 0) {
            gQuestCountdown -= ticks;
            if (gQuestCountdown < 0) {
                gQuestCountdown = 0;
            }
        }
    }
}

/* Sets the time left back to the quest's time limit. */
void Quest_ResetTimer(void)
{
    gQuestTimeLeft = Quest_GetTime(QUEST_TIME_LIMIT);
}

/* Clears the goals, as they stand and as set. */
void QuestGoal_ClearAll(void)
{
    s32 i;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        gQuest.goals[i].target = 0;
        gQuest.goalsSet[i].target = 0;
        gQuest.goals[i].count = 0;
        gQuest.goalsSet[i].count = 0;
        gQuest.goals[i].flags = 0;
        gQuest.goalsSet[i].flags = 0;
    }
}

/* Sets the goals from the quest's information block, with its clear type. */
void QuestGoal_InitFromDef(void)
{
    QuestGoal *goal;
    s32 i;

    goal = gQuestInfo->goals;
    gQuest.clearType = gQuestInfo->clearType;
    for (i = 0; i < QUEST_GOAL_COUNT; i++, goal++) {
        if (goal->flags != 0) {
            QuestGoal_Add(goal->flags, (s16)goal->target, goal->count);
        }
        gQuest.goalIdx[i] = QUEST_GOAL_NONE;
    }
}

/* Adds a goal (QUEST_GOAL_* kind bits, target, count) in the first free slot, if one is free. */
void QuestGoal_Add(s32 flags, s16 target, s16 count)
{
    s32 i;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if (gQuest.goals[i].flags == 0) {
            break;
        }
    }
    if (i < QUEST_GOAL_COUNT) {
        gQuest.goalsSet[i].flags = gQuest.goals[i].flags = flags | QUEST_GOAL_USED;
        gQuest.goalsSet[i].target = gQuest.goals[i].target = target;
        gQuest.goalsSet[i].count = gQuest.goals[i].count = count;
    }
}

/*
 * Marks goal i done; `how` is the way it was met (GOAL_MET_*). Outside
 * versus it notes the order (goalIdx), starts the message event of the way
 * it was met, shows the notice the clear type calls for and, unless the
 * quest is now cleared, a sub-quest's supplies notice and the goal's jingle.
 * The other players get QMSG_GOAL_DONE.
 */
void QuestGoal_Complete(u16 i, u32 how)
{
    s32 j;

    if (QuestGoal_IsDone(i) == 1) {
        return;
    }
    gQuest.goals[i].flags |= QUEST_GOAL_DONE;
    if (Quest_IsVersus() == 1) {
        return;
    }
    for (j = 0; j < QUEST_GOAL_COUNT; j++) {
        if (gQuest.goalIdx[j] == QUEST_GOAL_NONE) {
            gQuest.goalIdx[j] = i;
            break;
        }
    }
    /* message events (MsgEvent_Check): 0x11 killed, 0x12 captured, 0xF delivered */
    if (how == GOAL_MET_KILLED) {
        MsgEvent_Check(0x11);
    }
    if (how == GOAL_MET_CAPTURED) {
        MsgEvent_Check(0x12);
    }
    if (how == GOAL_MET_DELIVERED) {
        MsgEvent_Check(0xF);
    }
    switch (gQuest.clearType) {
    case QUEST_CLEAR_ANY:
        Notice_PushItem(gMustOrderAllText[0]);
        break;
    case QUEST_CLEAR_MAIN:
        if (Quest_IsMustOrder() == 1) {
            Notice_PushItem(gMustOrderAllText[0]);
        } else {
            Notice_PushItem(gGoalDoneTexts[(u32)i]);
        }
        break;
    case QUEST_CLEAR_FIRST_TWO:
    case QUEST_CLEAR_ALL:
        if (Quest_CheckClear(0) == 1) {
            Notice_PushItem(gMustOrderAllText[0]);
        } else {
            Notice_PushItem(gMustOrderOneText);
        }
        break;
    }
    if (Quest_CheckClear(0) == 0) {
        u16 n = i;

        /* a sub-quest's supplies: the supply box's 8 slots from 24 for goal 1, from 32 for goal 2 */
        if (n != 0) {
            s32 end;
            s32 k;

            for (k = (n - 1) * 8 + 24, end = k + 8; k < end; k++) {
                if (gScene.supply[k].item != 0) {
                    if (Quest_IsMustOrder() == 1) {
                        Notice_PushItem(gQuestNoticeTexts[3]);
                    } else if (n == 1) {
                        Notice_PushItem(gQuestNoticeTexts[7]);
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
    gQuest.msgType = QMSG_GOAL_DONE;
    gQuest.msgArg1 = how >> 16;
    gQuest.msgArg2 = how;
    QSync_SendState(6, gScene.localPlayer);
}

/* QuestGoal_Complete, while the quest is running (Quest_IsRunning(1) gives 0). */
void QuestGoal_CompleteIfRunning(u16 i, s32 how)
{
    if (Quest_IsRunning(1) == 0) {
        QuestGoal_Complete(i, how);
    }
}

/*
 * Returns 1 when the done goals clear the quest (QUEST_CLEAR_*); never in
 * versus. A goal that has just failed sends the script to its end mark
 * (once) and returns 0. With notify 1 a clear sends the script to its start
 * mark for the clear sequence (Quest_JumpToStart).
 */
s32 Quest_CheckClear(s32 notify)
{
    s32 i;
    s32 done = 0;       /* a bit per goal done */
    s32 unused = 0;     /* a bit per empty slot */

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if (gQuest.goals[i].flags & QUEST_GOAL_USED) {
            if (gQuest.goals[i].flags & QUEST_GOAL_DONE) {
                done |= 1 << i;
            }
            if ((gQuest.goals[i].flags & (QUEST_GOAL_FAILED | QUEST_GOAL_FAIL_SEEN)) == QUEST_GOAL_FAILED) {
                gQuest.goals[i].flags |= QUEST_GOAL_FAIL_SEEN;
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
    case QUEST_CLEAR_ANY:
        if (done) {
            if (notify == 1) {
                Quest_JumpToStart(0);
            }
            return 1;
        }
        break;
    case QUEST_CLEAR_MAIN:
        if ((done & 1) == 1) {
            if (notify == 1) {
                Quest_JumpToStart(0);
            }
            return 1;
        }
        break;
    case QUEST_CLEAR_FIRST_TWO:
        if ((done & 3) == 3) {
            if (notify == 1) {
                Quest_JumpToStart(0);
            }
            return 1;
        }
        break;
    case QUEST_CLEAR_ALL:
        if ((done | unused) == 7 /* the three goals */) {
            if (notify == 1) {
                Quest_JumpToStart(0);
            }
            return 1;
        }
        break;
    }
    return 0;
}

/*
 * Returns 1 when a sub-quest is done (goal 1 or 2 under QUEST_CLEAR_MAIN,
 * goal 2 under QUEST_CLEAR_FIRST_TWO). A failed goal ends the script as in
 * Quest_CheckClear.
 */
s32 Quest_CheckSubClear(void)
{
    s32 i;
    s32 done = 0;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if (gQuest.goals[i].flags & QUEST_GOAL_USED) {
            if (gQuest.goals[i].flags & QUEST_GOAL_DONE) {
                done |= 1 << i;
            }
            if ((gQuest.goals[i].flags & (QUEST_GOAL_FAILED | QUEST_GOAL_FAIL_SEEN)) == QUEST_GOAL_FAILED) {
                gQuest.goals[i].flags |= QUEST_GOAL_FAIL_SEEN;
                Quest_JumpToEnd(0);
                return 0;
            }
        }
    }
    switch (gQuest.clearType) {
    case QUEST_CLEAR_ANY:
        break;
    case QUEST_CLEAR_MAIN:
        if (done & 6) {
            return 1;
        }
        break;
    case QUEST_CLEAR_FIRST_TWO:
        if (done & 4) {
            return 1;
        }
        break;
    case QUEST_CLEAR_ALL:
        break;
    }
    return 0;
}

/* Quest_CheckSubClear, but 0 in a drill. */
s32 Quest_CheckSubClearMode(void)
{
    if (Quest_IsDrill() == 1) {
        return 0;
    }
    return Quest_CheckSubClear();
}

/* Returns 1 when goal i is done (and has not failed). */
s32 QuestGoal_IsDone(s32 i)
{
    s32 flags = gQuest.goals[i].flags;

    if (flags & QUEST_GOAL_USED) {
        if ((flags & (QUEST_GOAL_FAILED | QUEST_GOAL_FAIL_SEEN)) == QUEST_GOAL_FAILED) {
            return 0;
        }
        if (flags & QUEST_GOAL_DONE) {
            return 1;
        }
    }
    return 0;
}

/* Returns 1 when goal slot i holds a goal. */
s32 QuestGoal_IsUsed(s32 i)
{
    return (gQuest.goals[i].flags & QUEST_GOAL_USED) != 0;
}

/* Returns 1 when goals 1 and 2 are done or empty. */
s32 QuestGoal_AllSubDone(void)
{
    s32 i;

    for (i = 1; i < QUEST_GOAL_COUNT; i++) {
        if (QuestGoal_IsUsed(i) == 1 && QuestGoal_IsDone(i) == 0) {
            return 0;
        }
    }
    return 1;
}

/*
 * Returns 1 when a done hunt or monster goal on kind `target` is one the
 * clear type counts (Quest_NoteTargetDown: the camera turns to it).
 */
s32 QuestGoal_IsClearTarget(u16 target)
{
    s32 i;
    u8 type = gQuest.clearType;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if (((gQuest.goals[i].flags & QUEST_GOAL_HUNT_DONE) == QUEST_GOAL_HUNT_DONE || (gQuest.goals[i].flags & QUEST_GOAL_EM_DONE) == QUEST_GOAL_EM_DONE) && gQuest.goals[i].target == target) {
            switch (type) {
            case QUEST_CLEAR_ANY:
                return 1;
            case QUEST_CLEAR_MAIN:
                if (i == 0) {
                    return 1;
                }
                break;
            case QUEST_CLEAR_FIRST_TWO:
                if (i == 0 || i == 1) {
                    return 1;
                }
                break;
            case QUEST_CLEAR_ALL:
                return 1;
            }
        }
    }
    return 0;
}

/* Returns 1 when goal i is a done hunt (versus: which side's goal was met). */
static s32 QuestGoal_IsHuntDone(u16 i)
{
    return (gQuest.goals[i].flags & QUEST_GOAL_HUNT_DONE) == QUEST_GOAL_HUNT_DONE;
}

/* Returns the count still wanted by the hunt of kind `target`; -1 when there is none. */
s16 QuestGoal_HuntCountFor(u16 target)
{
    s32 i;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if ((gQuest.goals[i].flags & QUEST_GOAL_HUNT_USED) == QUEST_GOAL_HUNT_USED && gQuest.goals[i].target == target) {
            return gQuest.goals[i].count;
        }
    }
    return -1;
}

/* Returns the count still wanted by hunt goal i; -1 when it is not a hunt. */
static s32 QuestGoal_HuntCount(u16 i)
{
    return (gQuest.goals[i].flags & QUEST_GOAL_HUNT_USED) == QUEST_GOAL_HUNT_USED ? gQuest.goals[i].count : -1;
}

/* Returns the count delivery goal i wants in all; -1 when it is not a delivery. */
s32 QuestGoal_DeliverCount(u16 i)
{
    return (gQuest.goalsSet[i].flags & QUEST_GOAL_DELIVER_USED) == QUEST_GOAL_DELIVER_USED ? gQuest.goalsSet[i].count : -1;
}

/* Returns the count delivery goal i wants of item `target` in all; -1 otherwise. */
s32 QuestGoal_DeliverCountFor(u16 target, u16 i)
{
    if ((gQuest.goalsSet[i].flags & QUEST_GOAL_DELIVER_USED) == QUEST_GOAL_DELIVER_USED && gQuest.goalsSet[i].target == target) {
        return gQuest.goalsSet[i].count;
    }
    return -1;
}

/* Returns the item delivery goal i wants; 0 when it is not a delivery. */
u16 QuestGoal_DeliverItem(u16 i)
{
    if ((gQuest.goalsSet[i].flags & QUEST_GOAL_DELIVER_USED) == QUEST_GOAL_DELIVER_USED) {
        return gQuest.goalsSet[i].target;
    }
    return 0;
}

/* Returns goal i's flags as set, masked by `mask`. */
s32 QuestGoal_InitialFlags(u16 i, s32 mask)
{
    return gQuest.goalsSet[i].flags & mask;
}

/*
 * Returns how many of `item` were delivered for its delivery goal: by the
 * local player under QUEST_GOAL_LOCAL, else by all the players; 0 when no
 * goal wants it.
 */
static s32 Quest_DeliveredCount(u16 item)
{
    Player *pl;
    s32 i;
    QuestGoal *goal;
    s16 n = 0;
    s32 k;

    for (goal = gQuest.goals, i = 0; i < QUEST_GOAL_COUNT; i++, goal++) {
        if ((goal->flags & QUEST_GOAL_DELIVER_USED) == QUEST_GOAL_DELIVER_USED && goal->target == item) {
            break;
        }
    }
    if (i >= QUEST_GOAL_COUNT) {
        return 0;
    }
    if (goal->flags & QUEST_GOAL_LOCAL) {
        pl = &gPlayers[gScene.localPlayer];
        for (i = 0; i < DELIVERED_SLOTS; i++) {
            if (pl->delivered[i].item == item) {
                return pl->delivered[i].count;
            }
        }
    } else {
        pl = gPlayers;
        for (k = 0; k < gScene.playerCount; k++, pl++) {
            for (i = 0; i < DELIVERED_SLOTS; i++) {
                if (pl->delivered[i].item == item) {
                    n += pl->delivered[i].count;
                }
            }
        }
    }
    return n;
}

/*
 * The goals' checks each frame: a delivery goal's count left is brought up
 * to date and the goal met at 0; a part goal is met once the part is broken;
 * a damage goal once the monster lost count * 100 health since it was saved
 * (with HALF_TIME, only in the second half of the time, and the monster
 * flees wounded).
 */
void QuestGoal_Update(void)
{
    s32 i;
    Em *em;
    s32 halfTime;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if ((gQuest.goals[i].flags & (QUEST_GOAL_DELIVER_USED | QUEST_GOAL_DONE)) == QUEST_GOAL_DELIVER_USED) {
            if ((gQuest.goals[i].count = QuestGoal_DeliverCountFor(gQuest.goals[i].target, i) - Quest_DeliveredCount(gQuest.goals[i].target)) <= 0) {
                QuestGoal_CompleteIfRunning(i, GOAL_MET_DELIVERED);
            }
        }
    }
    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if ((gQuest.goals[i].flags & (QUEST_GOAL_PART_USED | QUEST_GOAL_DONE)) == QUEST_GOAL_PART_USED &&
            Quest_IsPartBroken(gQuest.goals[i].target, gQuest.goals[i].count) == 1) {
            QuestGoal_CompleteIfRunning(i, GOAL_MET_PART);
        }
    }
    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        s32 flags = gQuest.goals[i].flags;

        if ((flags & (QUEST_GOAL_DAMAGE_USED | QUEST_GOAL_DONE)) != QUEST_GOAL_DAMAGE_USED) {
            continue;
        }
        em = gQuest.spawnedEm;
        if (em == NULL) {
            continue;
        }
        if (flags & QUEST_GOAL_HALF_TIME) {
            halfTime = Quest_GetTime(QUEST_TIME_LIMIT) / 2;
            if (Quest_GetTime(QUEST_TIME_LEFT) > halfTime) {
                continue;
            }
        }
        if (gQuest.savedEmHp - em->hp < gQuest.goals[i].count * 100) {
            continue;
        }
        if (gQuest.goals[i].flags & QUEST_GOAL_HALF_TIME) {
            Notice_PushItem(gElderFledText[0]);
        }
        QuestGoal_CompleteIfRunning(i, GOAL_MET_DAMAGE);
    }
}

/*
 * Counts a capture of kind `target` for the hunt goals; a kill-only goal
 * fails instead (and returns 0). Returns 1 when a goal is met.
 */
s32 QuestGoal_OnCapture(u16 target)
{
    s32 i;
    s32 ret = 0;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        s32 flags = gQuest.goals[i].flags;

        if ((flags & QUEST_GOAL_ENDED) == 0 && (flags & QUEST_GOAL_HUNT_USED) == QUEST_GOAL_HUNT_USED && gQuest.goals[i].target == target) {
            if (flags & QUEST_GOAL_SLAY) {
                gQuest.goals[i].flags |= QUEST_GOAL_FAILED;
                return 0;
            }
            if (--gQuest.goals[i].count <= 0) {
                QuestGoal_CompleteIfRunning(i, GOAL_MET_CAPTURED);
                ret = 1;
            }
        }
    }
    return ret;
}

/* Returns 1 when a capture-only hunt goal is done (Quest_StepScript then jumps to gQuest.goalLabel). */
s32 QuestGoal_CaptureHuntDone(void)
{
    s32 i;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        if ((gQuest.goals[i].flags & QUEST_GOAL_CAPTURE) && (gQuest.goals[i].flags & QUEST_GOAL_HUNT_DONE) == QUEST_GOAL_HUNT_DONE) {
            return 1;
        }
    }
    return 0;
}

/*
 * Meets the part goals on kind `kind` whose part is now broken (a bit of
 * gQuest.parts[kind]); returns 1 when one is met. Em_BreakPart and
 * Em_BreakParts pass the part (or the bits) too, unused.
 */
s32 QuestGoal_OnPartBroken(u16 kind, u16 part)
{
    s32 i;
    s32 ret = 0;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        s32 flags = gQuest.goals[i].flags;

        if ((flags & QUEST_GOAL_ENDED) == 0 && (flags & QUEST_GOAL_PART_USED) == QUEST_GOAL_PART_USED && gQuest.goals[i].target == kind &&
            (gQuest.parts[kind] & (1 << gQuest.goals[i].count))) {
            QuestGoal_CompleteIfRunning(i, GOAL_MET_PART);
            ret = 1;
        }
    }
    return ret;
}

/*
 * Counts a kill of kind `target` for the hunt goals (a capture-only goal
 * fails instead, and returns 0), and meets the damage goals on it. Returns 1
 * when a goal is met.
 */
s32 QuestGoal_OnKill(u16 target)
{
    s32 i;
    s32 ret = 0;

    for (i = 0; i < QUEST_GOAL_COUNT; i++) {
        s32 flags = gQuest.goals[i].flags;

        if (flags & QUEST_GOAL_ENDED) {
            continue;
        }
        if ((flags & QUEST_GOAL_HUNT_USED) == QUEST_GOAL_HUNT_USED && gQuest.goals[i].target == target) {
            if (flags & QUEST_GOAL_CAPTURE) {
                gQuest.goals[i].flags |= QUEST_GOAL_FAILED;
                return 0;
            }
            if (--gQuest.goals[i].count <= 0) {
                QuestGoal_CompleteIfRunning(i, GOAL_MET_KILLED);
                ret = 1;
            }
        }
        if ((gQuest.goals[i].flags & QUEST_GOAL_DAMAGE_USED) == QUEST_GOAL_DAMAGE_USED && gQuest.goals[i].target == target) {
            QuestGoal_CompleteIfRunning(i, GOAL_MET_DAMAGE);
            ret = 1;
        }
    }
    return ret;
}

/* QuestGoal_OnKill's hunt count for goal i alone. */
s32 QuestGoal_OnKillSlot(u16 target, u16 i)
{
    if ((gQuest.goals[i].flags & QUEST_GOAL_ENDED) == 0 && (gQuest.goals[i].flags & QUEST_GOAL_HUNT_USED) == QUEST_GOAL_HUNT_USED && gQuest.goals[i].target == target) {
        if (gQuest.goals[i].flags & QUEST_GOAL_CAPTURE) {
            gQuest.goals[i].flags |= QUEST_GOAL_FAILED;
            return 0;
        }
        if (--gQuest.goals[i].count <= 0) {
            QuestGoal_CompleteIfRunning(i, GOAL_MET_KILLED);
            return 1;
        }
        return 0;
    }
    return 0;
}

extern QuestTimeNotice gQuestTimeNotices[];
extern char *gTimeLeftMinFmt[];
extern char *gQuestNoticeTexts[];
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
s16 Quest_StepScript(Player *pl, s16 cmdIndex, s32 *out)
{
    char buf[32];
    QuestCmd *cmd;
    s16 mark;
    Em *em;
    s16 lbl;

    mark = -1;
    if (gScene.questPhase == QUEST_PHASE_READY && gQuestTimeLeft >= 0) {
        if (gQuest.timeNoticeIdx > 0 && gQuestTimeNotices[gQuest.timeNoticeIdx].time >= gQuestTimeLeft) {
            Quest_Sprintf(buf, gTimeLeftMinFmt[0], gQuestTimeNotices[gQuest.timeNoticeIdx].time / 1800 /* frames per minute */);
            buf[31] = 0;
            Notice_PushText(buf);
            gQuest.timeNoticeIdx--;
        }
        Quest_UpdateTimers(1);
        if (gQuestTimeLeft <= 0) {
            gQuestTimeLeft = 0;
            cmd = ((QuestCmd *)gQuest.script);
            cmdIndex = 0;
            Quest_SendTime();
            gQuestTimeLeft = -1;
            for (;;) {
                if (cmd->op == QUEST_OP_LABEL && gQuest.timeoutLabel == cmd->arg) {
                    cmdIndex++;
                    break;
                }
                cmdIndex++;
                cmd++;
            }
        } else if (gQuestTimeLeft % QUEST_TIME_NOTICE_STEP == 0) {
            Quest_SendTime();
        }
    }
    for (;;) {
        if (gQuest.goalLabel != 0 && QuestGoal_CaptureHuntDone() == 1) {
            cmdIndex = 0;
            for (cmd = ((QuestCmd *)gQuest.script);; cmd++) {
                if (cmd->op == QUEST_OP_LABEL && gQuest.goalLabel == cmd->arg) {
                    cmdIndex++;
                    break;
                }
                cmdIndex++;
            }
            gQuest.goalLabel = 0;
        }
        if (gQuest.eventLabel != 0 && gQuest.eventTriggered != 0) {
            lbl = gQuest.eventLabel;
            cmd = ((QuestCmd *)gQuest.script);
            cmdIndex = 0;
            for (;;) {
                if (cmd->op == QUEST_OP_LABEL && lbl == cmd->arg) {
                    cmdIndex++;
                    break;
                }
                cmdIndex++;
                cmd++;
            }
            gQuest.eventLabel = 0;
        }
        cmd = &((QuestCmd *)gQuest.script)[cmdIndex];
        switch (cmd->op) {
        case QUEST_OP_CLEAR_TYPE:
            gQuest.clearType = cmd->arg;
            cmdIndex++;
            continue;
        case QUEST_OP_ADD_GOAL:
            QuestGoal_Add(cmd->arg, cmd->arg2, cmd->arg3);
            cmdIndex++;
            continue;
        case QUEST_OP_HALT:
            break;
        case QUEST_OP_SET_WAIT:
            gQuest.scriptWait = cmd->arg;
            cmdIndex++;
            break;
        case QUEST_OP_WAIT:
            if (--gQuest.scriptWait <= 0) {
                cmdIndex++;
            }
            break;
        case QUEST_OP_NOTICE: {
            s32 *ofs = gQuest.noticeTexts;

            ofs += cmd->arg;
            Notice_PushItem((char *)(*ofs + gSysSettings.stageFile));
            cmdIndex++;
            break;
        }
        case QUEST_OP_NOP:
            cmdIndex++;
            continue;
        case QUEST_OP_IF_CLEAR:
            if (Quest_CheckClear(0) == 1 || Quest_CheckSubClear() == 1) {
                cmdIndex++;
                continue;
            }
            cmdIndex++;
            cmd++;
            for (;;) {
                if (cmd->op == QUEST_OP_ENDIF) {
                    cmdIndex++;
                    break;
                }
                cmdIndex++;
                cmd++;
            }
            continue;
        case QUEST_OP_WAIT_HUNTED:
            if (QuestGoal_HuntCountFor(cmd->arg) <= cmd->arg2) {
                cmdIndex++;
                continue;
            }
            break;
        case QUEST_OP_WAIT_AREA:
            if (pl->area == cmd->arg) {
                cmdIndex++;
                continue;
            }
            break;
        case QUEST_OP_ADD_TIME:
            gQuestTimeLeft += (u16)cmd->arg;
            cmdIndex++;
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
            cmdIndex++;
            gScene.questPhase = QUEST_PHASE_RUNNING;
            gQuest.msgArg2 = 0;
            gQuest.acks |= 1 << (gScene.localPlayer + QUEST_ACK_STARTED_SHIFT);
            gQuest.msgType = QMSG_STARTED;
            QSync_SendState(6, gScene.localPlayer);
            continue;
        case QUEST_OP_WAIT_ALL_START:
            if ((gQuest.acks & QUEST_ACKS_STARTED) == QUEST_ACKS_STARTED || gScene.inQuest[gScene.localPlayer] != IN_QUEST_YES) {
                cmdIndex++;
            }
            break;
        case QUEST_OP_CLEARED:
            gQuest.step = 0;
            cmdIndex = -1;
            gQuest.state++; /* to QSTATE_CLEAR */
            break;
        case QUEST_OP_LOOP:
            cmdIndex = gQuest.loopPc;
            gQuest.scriptPc = cmdIndex;
            continue;
        case QUEST_OP_WAIT_STATE4:
            if (Player_IsState(pl, PL_GROUP_EVENT, PL_EVENT_CART) != 0) {
                cmdIndex++;
                continue;
            }
            break;
        case QUEST_OP_END:
            gScene.questPhase = QUEST_PHASE_ENDED;
            gQuest.result = QUEST_PHASE_ENDED;
            gQuest.msgArg0 = 0;
            cmdIndex++;
            gQuest.msgArg1 = 0;
            gQuest.msgArg2 = 0;
            gQuest.msgType = QMSG_ENDED;
            QSync_SendState(6, gScene.localPlayer);
            continue;
        case QUEST_OP_FAIL:
            gQuest.result = QUEST_PHASE_FAILED;
            cmdIndex = -1;
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
            gQuest.timeoutLabel = cmd->arg;
            Quest_Sprintf(buf, gQuestNoticeTexts[1], Quest_GetTime(QUEST_TIME_LIMIT) / 1800 /* frames per minute */);
            buf[31] = 0;
            Notice_PushText(buf);
            if (Quest_IsVersus() == 0) {
                SeasonBanner_Create();
            }
            cmdIndex++;
            break;
        case QUEST_OP_PROGRESS:
            gQuest.progress = (u16)cmd->arg;
            Quest_TargetMessage();
            cmdIndex++;
            continue;
        case QUEST_OP_WAIT_ITEMS:
            if (Cond_HasItems(&gQuest, (u16 *)cmd) != 0) {
                cmdIndex++;
                continue;
            }
            break;
        case QUEST_OP_GOAL_LABEL:
            gQuest.goalLabel = cmd->arg;
            cmdIndex++;
            break;
        case QUEST_OP_EVENT_LABEL:
            gQuest.eventLabel = cmd->arg;
            cmdIndex++;
            break;
        case QUEST_OP_END_ALT:
            gScene.questPhase = QUEST_PHASE_ENDED;
            cmdIndex++;
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
            cmdIndex++;
            gQuest.msgArg1 = 0;
            gScene.questPhase = QUEST_PHASE_RUNNING;
            gQuest.msgArg2 = 0;
            gQuest.acks |= 1 << (gScene.localPlayer + QUEST_ACK_STARTED_SHIFT);
            gQuest.msgType = QMSG_STARTED_ALT;
            QSync_SendState(6, gScene.localPlayer);
            continue;
        case QUEST_OP_IF_EM_HURT:
            if (gQuest.spawnedEm->hp <= gQuest.savedEmHp - cmd->arg) {
                gQuest.savedEmHp = gQuest.spawnedEm->hp;
                cmdIndex++;
                continue;
            }
            cmdIndex++;
            cmd++;
            for (;;) {
                if (cmd->op == QUEST_OP_ENDIF_EM_HURT) {
                    cmdIndex++;
                    break;
                }
                cmdIndex++;
                cmd++;
            }
            continue;
        case QUEST_OP_IF_EM_WEAK:
            if (EmList_AnyBelowHalfHp() == 1) {
                QuestGoal_CompleteIfRunning(0, 0x200);
                cmdIndex++;
                continue;
            }
            cmdIndex++;
            cmd++;
            for (;;) {
                if (cmd->op == QUEST_OP_ENDIF_EM_WEAK) {
                    cmdIndex++;
                    break;
                }
                cmdIndex++;
                cmd++;
            }
            continue;
        case QUEST_OP_GOTO:
            lbl = cmd->arg;
            cmd = ((QuestCmd *)gQuest.script);
            cmdIndex = 0;
            for (;;) {
                if (cmd->op == QUEST_OP_LABEL && lbl == cmd->arg) {
                    cmdIndex++;
                    break;
                }
                cmdIndex++;
                cmd++;
            }
            continue;
        case QUEST_OP_HALT2:
            break;
        case QUEST_OP_LABEL:
            cmdIndex++;
            continue;
        case QUEST_OP_TIMEOUT_LABEL:
            gQuest.timeoutLabel = cmd->arg;
            cmdIndex++;
            break;
        case QUEST_OP_IF_HUNTED_GOTO:
            if (QuestGoal_HuntCountFor(cmd->arg) <= cmd->arg2) {
                lbl = cmd->arg3;
                cmd = ((QuestCmd *)gQuest.script);
                cmdIndex = 0;
                for (;;) {
                    if (cmd->op == QUEST_OP_LABEL && lbl == cmd->arg) {
                        cmdIndex++;
                        break;
                    }
                    cmdIndex++;
                    cmd++;
                }
                continue;
            }
            cmdIndex++;
            continue;
        case QUEST_OP_MARK:
            mark = cmdIndex;
            cmdIndex++;
            continue;
        case QUEST_OP_UNMARK:
            mark = -1;
            cmdIndex++;
            continue;
        case QUEST_OP_VERSUS_END:
            if (gNextQuest.unk5E == 0) {
                if (QuestGoal_IsHuntDone(0) == 1) {
                    Versus_End(2);
                    cmdIndex = -1;
                } else if (QuestGoal_IsHuntDone(1) == 1) {
                    Versus_End(1);
                    cmdIndex = -1;
                }
            } else if (gQuest.sideEmLeft[0] <= 0) {
                Versus_End(2);
                cmdIndex = -1;
            } else if (gQuest.sideEmLeft[1] <= 0) {
                Versus_End(1);
                cmdIndex = -1;
            }
            break;
        case QUEST_OP_COMPLETE_GOAL:
            QuestGoal_CompleteIfRunning(0, 0);
            cmdIndex++;
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
        cmdIndex = mark;
    }
    return cmdIndex;
}

extern s32 gDrillPayoutThresholds[];
extern PartReward gPartRewardRecs[];
extern u16 *gRewardItemTables[];
u16 Dojo_GetParMinutes(u16 id);

s32 Plsel_DrillPayout(void);

#pragma optimization_level 3
/*
 * Rolls the quest's rewards into gScene.supply (REWARD_SLOT_*): for each
 * reward entry whose condition is met (REWARD_*), draws from its item list
 * while each draw's chance holds (gRewardRateTables: by the entry's kind,
 * the main ones by the local player's luck skill too); then, outside a
 * must-order or versus quest, the rewards of the broken parts and the
 * captures (gPartRewardRecs); then sorts each group by item.
 *
 * The locals are shared as the game's code shares them: `i` clears the
 * slots, then holds the luck skill's row, then sorts; `matched` marks that
 * a ranged condition was taken, then counts the part draws and sorts; `k`
 * holds a band's count, then counts the draws. The function is compiled at
 * optimisation level 3, unlike the rest of the file: at level 4 the draw
 * loop loads the list's first weight through a register of its own (9
 * words differ).
 */
void Quest_RollRewards(void)
{
    u16 roll;
    s32 subACount;
    u16 *entry;
    u16 sum;
    s32 slot;
    u16 swapItem;
    u16 *drop;
    Player *pl;
    u16 *dropList;
    u16 *range;
    PartReward *partRec;
    s32 i;
    s32 chance;
    s32 matched;
    u16 partSum;
    s8 *rate;
    s16 swapCount;
    s32 k;
    s32 subBCount;
    u16 tableIdx;

    gScene.supplyTaken[0] = 0;
    matched = 0;
    gScene.supplyTaken[1] = 0;
    pl = &gPlayers[gScene.localPlayer];
    for (i = 0; i < REWARD_SLOT_END; i++) {
        gScene.supply[i].item = 0;
        gScene.supply[i].count = 0;
    }
    slot = 0;
    subACount = subBCount = 0;
    if (Player_HasSkill(pl, SKILL_LUCK_UP) == 1) {
        i = 2;
    } else if (Player_HasSkill(pl, SKILL_LUCK_DOWN) == 1) {
        i = 3;
    } else if (Player_HasSkill(pl, SKILL_LUCK_UP2) == 1) {
        i = 1;
    } else if (Player_HasSkill(pl, SKILL_LUCK_DOWN2) == 1) {
        i = 4;
    } else {
        i = 0;
    }
    for (entry = ((u16 *)gQuest.rewardTable); *entry != REWARD_END; entry += 4) {
        if (*entry >= REWARD_HUNTED_20 && *entry < REWARD_HUNTED_20 + 21) {
            if (matched != 0 || (u16)(20 - QuestGoal_HuntCount(0)) < *entry - REWARD_HUNTED_20) {
                goto next;
            }
            matched = 1;
        } else if (*entry >= REWARD_HUNTED_99 && *entry < REWARD_DELIVERED) {
            if (matched != 0 || (u16)(99 - QuestGoal_HuntCount(0)) < *entry - REWARD_HUNTED_99) {
                goto next;
            }
            matched = 1;
        } else if (*entry >= REWARD_DELIVERED && *entry < REWARD_HUNTED_20) {
            if (matched != 0 || (u16)Quest_DeliveredCount(QuestGoal_DeliverItem(0)) < *entry - REWARD_DELIVERED) {
                goto next;
            }
            matched = 1;
        } else if (*entry >= REWARD_DRILL_PAYOUT && *entry < REWARD_DOJO_PAR) {
            if (matched != 0 || Plsel_DrillPayout() < gDrillPayoutThresholds[*entry - REWARD_DRILL_PAYOUT]) {
                goto next;
            }
            matched = 1;
        } else {
            switch (*entry) {
            case REWARD_ALWAYS:
                break;
            case REWARD_MAIN_GOAL:
                if (QuestGoal_IsDone(0) == 0) {
                    goto next;
                }
                break;
            case REWARD_SUB_A:
                if (QuestGoal_IsDone(1) == 0) {
                    goto next;
                }
                break;
            case REWARD_SUB_B:
                if (QuestGoal_IsDone(2) == 0) {
                    goto next;
                }
                break;
            case REWARD_HP_BELOW:
                if (gScene.unk13A <= 0x6400 || gQuest.savedEmHp > 0x6400) {
                    goto next;
                }
                break;
            case REWARD_HP_BELOW + 1:
                if (gScene.unk13A <= 0x4B00 || gQuest.savedEmHp > 0x4B00) {
                    goto next;
                }
                break;
            case REWARD_HP_BELOW + 2:
                if (gScene.unk13A <= 0x3200 || gQuest.savedEmHp > 0x3200) {
                    goto next;
                }
                break;
            case REWARD_HP_BELOW + 3:
                if (gScene.unk13A <= 0x1900 || gQuest.savedEmHp > 0x1900) {
                    goto next;
                }
                break;
            case REWARD_HP_BELOW + 4:
                if (gScene.unk13A <= 0x1900 || gQuest.savedEmHp > 0x1900) {
                    goto next;
                }
                break;
            case REWARD_DELIVER_BAND:
            case REWARD_DELIVER_BAND + 1:
            case REWARD_DELIVER_BAND + 2:
            case REWARD_DELIVER_BAND + 3:
            case REWARD_DELIVER_BAND + 4:
                range = &gRewardDeliverBands[*entry - REWARD_DELIVER_BAND];
                k = (u16)Quest_DeliveredCount(QuestGoal_DeliverItem(0));
                if (k >= range[0] && k < range[1]) {
                    break;
                }
                goto next;
            case REWARD_HUNT_BAND:
            case REWARD_HUNT_BAND + 1:
            case REWARD_HUNT_BAND + 2:
            case REWARD_HUNT_BAND + 3:
            case REWARD_HUNT_BAND + 4:
            case REWARD_HUNT_BAND + 5:
            case REWARD_HUNT_BAND + 6:
            case REWARD_HUNT_BAND + 7:
            case REWARD_HUNT_BAND + 8:
            case REWARD_HUNT_BAND + 9:
                range = &gRewardHuntBands[*entry - REWARD_HUNT_BAND];
                k = (u16)(20 - QuestGoal_HuntCount(0));
                if (k >= range[0] && k < range[1]) {
                    break;
                }
                goto next;
            case REWARD_DOJO_PAR:
                if (gQuest.clearTime > Dojo_GetParMinutes(gQuest.stage) * 1800 /* frames per minute */) {
                    goto next;
                }
                break;
            case REWARD_ALL_HUNTED:
                if (gQuest.emLeft > 0) {
                    goto next;
                }
                break;
            default:
                goto next;
            }
        }
        dropList = (u16 *)(*(s32 *)(entry + 2) + gSysSettings.stageFile);
        for (k = 0;; k++) {
            /* gRewardRateTables: 0 main, 1 main goal, 2 sub-quests, 3 other, 4 drill, 5 parts, 6 captures, 7 training */
            switch (*entry) {
            case REWARD_ALWAYS:
                if (Quest_IsDrill() == 1) {
                    rate = gRewardRateTables[4];
                } else if (Quest_IsTraining() == 1) {
                    rate = gRewardRateTables[7];
                } else {
                    rate = gRewardRateTables[0] + i * 11;
                }
                break;
            case REWARD_MAIN_GOAL:
                rate = gRewardRateTables[1] + i * 9;
                break;
            case REWARD_SUB_A:
            case REWARD_SUB_B:
                rate = gRewardRateTables[2];
                rate += i * 6;
                break;
            case REWARD_ALL_HUNTED:
                rate = gRewardRateTables[6];
                break;
            default:
                rate = gRewardRateTables[3];
                break;
            }
            chance = rate[k];
            if ((Rand_Next(0) & 0x1F) >= chance) {
                break;
            }
            sum = 0;
            for (drop = dropList; *drop != REWARD_END; drop += 3) {
                sum += *drop;
            }
            roll = Rand_Next(0) % sum;
            if (*entry == REWARD_ALWAYS && k == 0) {
                roll = 0;
            }
            if (*entry == REWARD_MAIN_GOAL && k == 0) {
                roll = 0;
            }
            sum = 0;
            for (drop = dropList; *drop != REWARD_END; drop += 2) {
                sum += *drop++;
                if (roll < sum) {
                    break;
                }
            }
            if (*drop != 0) {
                switch (*entry) {
                case REWARD_SUB_A:
                    gScene.supply[REWARD_SLOT_SUB_A + subACount].item = drop[0];
                    gScene.supply[REWARD_SLOT_SUB_A + subACount].count = drop[1];
                    if (subACount < 3) {
                        subACount++;
                    }
                    break;
                case REWARD_SUB_B:
                    gScene.supply[REWARD_SLOT_SUB_B + subBCount].item = drop[0];
                    gScene.supply[REWARD_SLOT_SUB_B + subBCount].count = drop[1];
                    if (subBCount < 3) {
                        subBCount++;
                    }
                    break;
                default:
                    gScene.supply[slot].item = drop[0];
                    gScene.supply[slot].count = drop[1];
                    slot++;
                    break;
                }
            }
            if (slot >= REWARD_SLOT_SUB_A) {
                break;
            }
        }
    next: /* an entry whose condition fails comes here, and the game's code tests the slots again (a continue would skip the test) */
        if (slot >= REWARD_SLOT_SUB_A) {
            break;
        }
    }
    if (Quest_IsMustOrderOrVersus() == 0) {
        slot = REWARD_SLOT_PARTS;
        for (partRec = gPartRewardRecs; partRec->kind != 0xFF; partRec++) {
            if (partRec->part == PART_REWARD_CAPTURE) {
                if (gQuest.captured[partRec->kind] == 0) {
                    continue;
                }
            } else if (Quest_IsPartBroken(partRec->kind, partRec->part) == 0) {
                continue;
            }
            tableIdx = partRec->table[gStageHeader->tier];
            if (tableIdx == 0xFFFF /* none at this tier */) {
                continue;
            }
            dropList = gRewardItemTables[tableIdx];
            for (matched = 0;; matched++) {
                rate = partRec->part == PART_REWARD_CAPTURE ? gRewardRateTables[6] : gRewardRateTables[5];
                chance = rate[matched];
                if ((Rand_Next(0) & 0x1F) >= chance) {
                    break;
                }
                sum = 0;
                for (drop = dropList; *drop != REWARD_END; drop += 3) {
                    sum += *drop;
                }
                roll = Rand_Next(0) % sum;
                partSum = 0;
                for (drop = dropList; *drop != REWARD_END; drop += 2) {
                    partSum += *drop++;
                    if (roll < partSum) {
                        break;
                    }
                }
                if (*drop != 0) {
                    gScene.supply[slot].item = drop[0];
                    gScene.supply[slot].count = drop[1];
                    slot++;
                }
                if (slot >= REWARD_SLOT_END) {
                    break;
                }
            }
        }
    }
    for (i = REWARD_SLOT_MAIN; i < REWARD_SLOT_SUB_A; i++) {
        for (matched = i + 1; matched < REWARD_SLOT_SUB_A; matched++) {
            if (gScene.supply[i].item > gScene.supply[matched].item && gScene.supply[matched].item != 0) {
                swapItem = gScene.supply[i].item;
                swapCount = gScene.supply[i].count;
                gScene.supply[i].item = gScene.supply[matched].item;
                gScene.supply[i].count = gScene.supply[matched].count;
                gScene.supply[matched].item = swapItem;
                gScene.supply[matched].count = swapCount;
            }
        }
    }
    for (i = REWARD_SLOT_SUB_A; i < REWARD_SLOT_SUB_B; i++) {
        for (matched = i + 1; matched < REWARD_SLOT_SUB_B; matched++) {
            if (gScene.supply[i].item > gScene.supply[matched].item && gScene.supply[matched].item != 0) {
                swapItem = gScene.supply[i].item;
                swapCount = gScene.supply[i].count;
                gScene.supply[i].item = gScene.supply[matched].item;
                gScene.supply[i].count = gScene.supply[matched].count;
                gScene.supply[matched].item = swapItem;
                gScene.supply[matched].count = swapCount;
            }
        }
    }
    for (i = REWARD_SLOT_SUB_B; i < REWARD_SLOT_PARTS; i++) {
        for (matched = i + 1; matched < REWARD_SLOT_PARTS; matched++) {
            if (gScene.supply[i].item > gScene.supply[matched].item && gScene.supply[matched].item != 0) {
                swapItem = gScene.supply[i].item;
                swapCount = gScene.supply[i].count;
                gScene.supply[i].item = gScene.supply[matched].item;
                gScene.supply[i].count = gScene.supply[matched].count;
                gScene.supply[matched].item = swapItem;
                gScene.supply[matched].count = swapCount;
            }
        }
    }
    for (i = REWARD_SLOT_PARTS; i < REWARD_SLOT_END; i++) {
        for (matched = i + 1; matched < REWARD_SLOT_END; matched++) {
            if (gScene.supply[i].item > gScene.supply[matched].item && gScene.supply[matched].item != 0) {
                swapItem = gScene.supply[i].item;
                swapCount = gScene.supply[i].count;
                gScene.supply[i].item = gScene.supply[matched].item;
                gScene.supply[i].count = gScene.supply[matched].count;
                gScene.supply[matched].item = swapItem;
                gScene.supply[matched].count = swapCount;
            }
        }
    }
}
#pragma optimization_level reset

/*
 * The monsters' size roll tables, the special point's reward lists, the
 * time-left notices, versus's start positions and the stage file tables.
 */
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
SpecialMineReward gSpecialMineRewards_0[8] = {
#include "sub_main/0044B8B0.inc"
};
SpecialMineReward gSpecialMineRewards_1[8] = {
#include "sub_main/0044B8D0.inc"
};
SpecialMineReward gSpecialMineRewards_2[8] = {
#include "sub_main/0044B8F0.inc"
};
SpecialMineReward gSpecialMineRewards_3[8] = {
#include "sub_main/0044B910.inc"
};
SpecialMineReward gSpecialMineRewards_4[8] = {
#include "sub_main/0044B930.inc"
};
SpecialMineReward gSpecialMineRewards_5[8] = {
#include "sub_main/0044B950.inc"
};
SpecialMineReward gSpecialMineRewards_6[8] = {
#include "sub_main/0044B970.inc"
};
SpecialMineReward gSpecialMineRewards_7[8] = {
#include "sub_main/0044B990.inc"
};
SpecialMineReward gSpecialMineRewards_8[8] = {
#include "sub_main/0044B9B0.inc"
};
SpecialMineReward gSpecialMineRewards_9[8] = {
#include "sub_main/0044B9D0.inc"
};
SpecialMineReward gSpecialMineRewards_10[8] = {
#include "sub_main/0044B9F0.inc"
};
SpecialMineReward gSpecialMineRewards_11[8] = {
#include "sub_main/0044BA10.inc"
};
SpecialMineReward gSpecialMineRewards_12[8] = {
#include "sub_main/0044BA30.inc"
};
SpecialMineReward gSpecialMineRewards_13[8] = {
#include "sub_main/0044BA50.inc"
};
SpecialMineReward gSpecialMineRewards_14[8] = {
#include "sub_main/0044BA70.inc"
};
SpecialMineReward gSpecialMineRewards_15[8] = {
#include "sub_main/0044BA90.inc"
};
SpecialMineReward gSpecialMineRewards_16[8] = {
#include "sub_main/0044BAB0.inc"
};
SpecialMineReward gSpecialMineRewards_17[8] = {
#include "sub_main/0044BAD0.inc"
};
SpecialMineReward gSpecialMineRewards_18[8] = {
#include "sub_main/0044BAF0.inc"
};
SpecialMineReward gSpecialMineRewards_19[8] = {
#include "sub_main/0044BB10.inc"
};
SpecialMineReward gSpecialMineRewards_20[8] = {
#include "sub_main/0044BB30.inc"
};
SpecialMineReward gSpecialMineRewards_21[8] = {
#include "sub_main/0044BB50.inc"
};
SpecialMineReward gSpecialMineRewards_22[8] = {
#include "sub_main/0044BB70.inc"
};
SpecialMineReward gSpecialMineRewards_23[8] = {
#include "sub_main/0044BB90.inc"
};
SpecialMineReward gSpecialMineRewards_24[8] = {
#include "sub_main/0044BBB0.inc"
};
SpecialMineReward gSpecialMineRewards_25[8] = {
#include "sub_main/0044BBD0.inc"
};
SpecialMineReward gSpecialMineRewards_26[8] = {
#include "sub_main/0044BBF0.inc"
};
SpecialMineReward gSpecialMineRewards_27[8] = {
#include "sub_main/0044BC10.inc"
};
SpecialMineReward gSpecialMineRewards_28[8] = {
#include "sub_main/0044BC30.inc"
};
SpecialMineReward gSpecialMineRewards_29[8] = {
#include "sub_main/0044BC50.inc"
};
SpecialMineReward gSpecialMineRewards_30[8] = {
#include "sub_main/0044BC70.inc"
};
SpecialMineReward gSpecialMineRewards_31[8] = {
#include "sub_main/0044BC90.inc"
};
SpecialMineReward gSpecialMineRewards_32[8] = {
#include "sub_main/0044BCB0.inc"
};
SpecialMineReward *gSpecialMineRewards[36] = {
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
QuestTimeNotice gQuestTimeNotices[3] = {
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


/*
 * ---- The script's jumps, the players' quest messages, the quest records
 *      and the player's state ----
 *
 * The player's state: stateGroup and state (the previous ones in
 * prevStateGroup and prevState) and their change requests, the two flag
 * words (flags and stateFlags: flag bit 31 selects the second), and the idle
 * motions by the area's temperature.
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
void Player_ReturnToIdle(Player *p, s32 action, s32 a2, s32 a3);
void Player_ReturnToIdleKeepParts(Player *p, s32 action, s32 a2, s32 a3);
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
    QuestCmd *cmd = (QuestCmd *)gQuest.script;
    s16 i = 0;
    s16 mark;

    gScene.questPhase = QUEST_PHASE_ENDED;
    mark = alt == 0 ? QUEST_OP_END : QUEST_OP_END_ALT;
    for (;;) {
        if (cmd->op == mark) {
            break;
        }
        i++;
        cmd++;
    }
    gQuest.scriptPc = i;
}

/* Another player started the quest: unless it runs already, the script
 * jumps to its start mark (QUEST_OP_START, or with `alt`
 * QUEST_OP_START_ALT) and the quest is RUNNING. */
static void Quest_JumpToStart(s32 alt)
{
    QuestCmd *cmd = (QuestCmd *)gQuest.script;
    s16 i = 0;
    s16 mark;

    switch (gScene.questPhase) {
    case QUEST_PHASE_NONE:
    case 1:
    case QUEST_PHASE_READY:
        mark = alt == 0 ? QUEST_OP_START : QUEST_OP_START_ALT;
        for (;;) {
            if (cmd->op == mark) {
                break;
            }
            i++;
            cmd++;
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
        if (Area_DayId(area) == AREA_JUNGLE_0 && (Rand_Next(0) & 0xF) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Spawns Yama Tsukami's second kind at the area's origin, at its full
 * health and the normal size. */
void QuestEm_SpawnYamaTsukamiB(void)
{
    SpawnRequest request;

    request.kind = EM_KIND_YAMA_TSUKAMI_B;
    request.variant = 0;
    request.hp = -1;
    request.x = 0;
    request.size = 100;
    request.y = 0;
    request.z = 0;
    request.rotY = 0;
    QuestEm_Spawn((QuestEm *)&request);
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
    s32 value;

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
        value = (gQuest.msgArg2 & 0xFFFF) | ((gQuest.msgArg1 << 16) & 0xFFFF0000);
        if (value < (s32)gQuestTimeLeft) {
            gQuestTimeLeft = value;
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
        value = (gQuest.msgArg2 & 0xFFFF) | ((gQuest.msgArg1 << 16) & 0xFFFF0000);
        if (gQuestPartyClearTime < value) {
            gQuestPartyClearTime = value;
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
    QuestEmView *qem;
    Em *em;
    u16 countLeft;
    s16 flags;
    s8 emIndex;
    s32 i;

    countLeft = gQuest.msgArg2;
    em = gEms;
    if ((qem = (QuestEmView *)QuestEm_FindByIndex(gQuest.msgArg1, gQuest.msgArea)) != NULL) {
        flags = qem->flags;
        if (flags & (QUEST_EM_ACTIVE | QUEST_EM_HUNTED)) {
            emIndex = qem->emIndex;
            for (i = 0; i < 20 /* gEms */; i++, em++) {
                if (em->index == emIndex && em->questIndex == qem->index) {
                    if (em->type != qem->kind) {
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
                    } else if (qem->count <= countLeft) {
                        return;
                    }
                    if (em->taskPhase >= 2 || em->stateGroup == EM_GROUP_DEFEAT) {
                        EmMine_Remove(em);
                    }
                    Em_ChangeState(em, EM_GROUP_DEFEAT, 0, 0);
                    em->alpha = 1.0f;
                    qem->flags = QUEST_EM_ACTIVE;
                    em->hp = 0;
                    if (QuestGoal_IsClearTarget(em->type) == 1) {
                        gQuest.downEm = em;
                    }
                    return;
                }
            }
            if (qem->count > countLeft) {
                QuestGoal_OnKill(qem->kind);
                Quest_CountEventKill(qem->kind);
                Quest_CountTarget(qem->kind);
                if (Quest_IsVersus() == 0) {
                    QuestEm_CountHunted(qem->kind);
                    gQuest.hunted[qem->kind]++;
                    if (gQuest.hunted[qem->kind] > QUEST_COUNT_MAX) {
                        gQuest.hunted[qem->kind] = QUEST_COUNT_MAX;
                    }
                }
                if (qem->large != 0) {
                    gQuest.emLeft--;
                }
                qem->count--;
                if (qem->count > 0) {
                    qem->flags = QUEST_EM_ACTIVE;
                    return;
                }
                qem->flags = QUEST_EM_DONE;
                qem->emIndex = -1;
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

/* Tells the others that quest monster qem is down (QMSG_KILL); `monster`
 * is its Em, read as a Player for its area. */
void Quest_SendKill(QuestEm *qem, Player *monster)
{
    gQuest.msgArg0 = 0;
    gQuest.msgType = QMSG_KILL;
    gQuest.msgArg1 = qem->index;
    gQuest.msgArg2 = qem->count;
    gQuest.msgArea = monster->area;
    QSync_SendState(6, gScene.localPlayer);
}

/* Empty: QuestEm_Captured calls it with the quest monster and its Em (both ignored). */
void QuestEm_CapturedStub(void)
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

/*
 * The player's state code (sub_main's half of it; the states themselves are
 * the game overlay's, include/player.h). The state calls take a Player or an
 * Em (both start with ObjHead).
 */

/* Turns off the part motion overrides of the four motion layers. */
void Player_ClearPartOverrides(Player *pl)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        pl->partAnimOn[i] = 0;
        pl->partAnimMotion[i] = 0;
        pl->partAnimWeight[i] = 0;
    }
}

/*
 * Sets the state group and state of a player or a monster (obj), as
 * Player_SetState but without its checks or flags. A monster keeps no
 * previous state nor stateRand, and keeps its part overrides in weapon-group
 * states 0, 4 and 5 when the byte at Player.weaponType is 1, 5 or 10. For a
 * player with unk01E 0, the local player sends the change; another player
 * put in the idle state returns to the idle motion.
 */
void Player_ForceState(void *obj, s32 state, s32 sub)
{
    Player *pl = obj;
    if (pl->isEm == 0) {
        pl->prevStateGroup = pl->stateGroup;
        pl->prevState = pl->state;
    }
    pl->stateGroup = state;
    pl->state = sub;
    pl->step = 0;
    pl->stateVar0 = 0;
    pl->stateVar1 = 0;
    if (pl->isEm == 0) {
        pl->stateRand = Rand_Next(1);
    }
    if (pl->isEm == 0) {
        Player_ClearPartOverrides(pl);
    } else {
        switch (pl->weaponType) {
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_LIGHT_BOWGUN:
        case WEAPON_BOW:
            if (pl->stateGroup == PL_GROUP_WEAPON) {
                switch (pl->state) {
                case 0:
                case 4:
                case 5:
                    break;
                default:
                    Player_ClearPartOverrides(pl);
                    break;
                }
            } else {
                Player_ClearPartOverrides(pl);
            }
            break;
        default:
            Player_ClearPartOverrides(pl);
            break;
        }
    }
    if (pl->isEm == 0 && pl->stateGroup == PL_GROUP_REACT) {
        pl->unk417 = 0;
    }
    Player_UpdateMotionTime(pl, 0);
    pl->flags &= 0x100100;
    pl->stateFlags = 0;
    if (pl->isEm == 0 && pl->ownTask == 0) {
        if (Player_IsLocal(pl) == 1) {
            QSync_SendPlayer(pl, 1, 0);
            return;
        }
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_IDLE) != 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
    }
}

/*
 * Changes pl's state to group `state`, state `sub` (PL_GROUP_*, and the
 * group's enum in player.h): the old one goes to prevStateGroup/prevState,
 * the step, the scratch bytes, the motion speed and the flags not kept are
 * reset, and stateRand is drawn again. `flags` are PL_SET_*. The local
 * player sends the change (QSync_SendPlayer); another player put in the idle
 * state returns to the idle motion. While the local player moves to another
 * area, only an event state is taken.
 */
void Player_SetState(Player *pl, u16 state, u8 sub, u16 flags)
{
    u16 low;

    if (Player_IsLocal(pl) == 1 && pl->areaMoveState != 0 && state != PL_GROUP_EVENT) {
        return;
    }
    pl->prevStateGroup = pl->stateGroup;
    pl->prevState = pl->state;
    pl->stateGroup = state;
    pl->state = sub;
    pl->step = 0;
    pl->stateVar0 = 0;
    pl->stateVar1 = 0;
    if (flags & PL_SET_KEEP_RAND) {
        low = (u8)pl->stateRand;
        pl->stateRand = Rand_Next(1) & 0xFF00;
        pl->stateRand |= low;
    } else {
        pl->stateRand = Rand_Next(1);
    }
    pl->stateRequest = 0;
    pl->motionSpeed = 1.0f;
    if (!(flags & PL_SET_KEEP_PARTS)) {
        Player_ClearPartOverrides(pl);
        pl->aimWeight = 0;
    }
    Player_ResetAttackPercent(pl);
    if (pl->stateGroup == PL_GROUP_REACT) {
        pl->unk417 = 0;
    }
    Player_UpdateMotionTime(pl, 0);
    pl->flags &= 0x80100;
    pl->stateFlags = 0;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
    if (Player_IsLocal(pl) == 1) {
        if (!(flags & PL_SET_NO_SYNC)) {
            if (flags & PL_SET_SYNC_OFFER) {
                QSync_SendPlayer(pl, 6, flags);
            } else if (flags & PL_SET_SYNC_ALT) {
                QSync_SendPlayer(pl, 9, flags);
            } else {
                QSync_SendPlayer(pl, 1, flags);
            }
        }
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_IDLE) != 0) {
        if (!(flags & PL_SET_KEEP_PARTS)) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else {
            Player_ReturnToIdleKeepParts(pl, 0, 4, 0);
        }
    }
}

/* Player_SetState, marked as a request (stateRequest); refused as there while the local player changes areas. */
void Player_RequestState(Player *pl, u16 state, u8 sub, u16 flags)
{
    if (Player_IsLocal(pl) != 1 || pl->areaMoveState == 0 || state == PL_GROUP_EVENT) {
        Player_SetState(pl, state, sub, flags);
        pl->stateRequest = 1;
    }
}

/*
 * Player_RequestState with PL_SET_KEEP_RAND: the low byte of stateRand
 * carries gQuest.drillCharm for the local player (0 for another).
 */
void Player_RequestStateSynced(Player *pl, u16 state, u8 sub, u16 flags)
{
    if (Player_IsLocal(pl) != 1 || pl->areaMoveState == 0 || state == PL_GROUP_EVENT) {
        if (Player_IsLocal(pl) == 1) {
            pl->stateRand = gQuest.drillCharm;
        } else {
            pl->stateRand = 0;
        }
        Player_SetState(pl, state, sub, flags | PL_SET_KEEP_RAND);
        pl->stateRequest = 1;
    }
}

/* Returns 1 when the player or monster obj is in group `state`, state `sub`. */
s16 Player_IsState(void *obj, u8 state, u8 sub)
{
    ObjHead *head = obj;
    if (head->stateGroup == state && head->state == sub) {
        return 1;
    }
    return 0;
}

/* Returns obj's flags masked by `flag`; with bit 31 set, its stateFlags (cleared at each state change) instead. */
u32 Player_TestFlag(void *obj, u32 flag)
{
    ObjHead *head = obj;
    if ((flag & 0x80000000) == 0) {
        return head->flags & flag;
    }
    return head->stateFlags & (flag & 0x7FFFFFFF);
}

/* Sets `flag` in obj's flags, or in its stateFlags with bit 31 set. */
void Player_SetFlag(void *obj, u32 flag)
{
    ObjHead *head = obj;
    if (!(flag & 0x80000000)) {
        head->flags |= flag;
    } else {
        head->stateFlags |= flag & 0x7FFFFFFF;
    }
}

/* Clears `flag` in obj's flags, or in its stateFlags with bit 31 set. */
void Player_ClearFlag(void *obj, u32 flag)
{
    ObjHead *head = obj;
    if (!(flag & 0x80000000)) {
        head->flags &= ~flag;
    } else {
        head->stateFlags &= ~(flag & 0x7FFFFFFF);
    }
}

void Player_SetAction(Player *pl, u8 action)
{
    pl->action = action;
}

/* Returns obj's ObjHead.attackFlags bits (ATKF_*) masked by `mask`. */
s32 Player_TestFlags(void *obj, s32 mask)
{
    ObjHead *head = obj;
    return head->attackFlags & mask;
}

/* Returns obj's ObjHead.attackFlags bits (ATKF_*) masked by `mask`, then sets them. */
s32 Player_TestSetFlags(void *obj, s32 mask)
{
    ObjHead *head = obj;
    s32 ret = head->attackFlags & mask;

    head->attackFlags |= (u16)mask;
    return ret;
}

/*
 * The idle motion in the air (Player_ReturnToIdle with move mode 2): motion
 * 0x3F9 from frame 8, blended over `blend` frames; `variant` picks unk2B4,
 * 2 or 6. Sets flag 1.
 */
void Player_SetAirIdle(Player *pl, s16 blend, s32 variant)
{
    pl->motionId[0] = 0x3F9;
    if (variant == 0) {
        pl->unk2B4 = 2;
    } else {
        pl->unk2B4 = 6;
    }
    pl->motionBlend[0] = blend / 2;
    pl->motionBlend[1] = blend / 2;
    pl->motionFrame[0] = 8;
    pl->motionFrame[1] = 8;
    Player_SetFlag(pl, 1);
}

/*
 * Starts pl's idle motion pair (lower and upper layers), blended over
 * `blend` frames from `startFrame`: riding, the weapon drawn, tired (the
 * maximum stamina below 76), or by the area's temperature (AREA_TEMP_*)
 * unless a resistance skill covers it.
 */
void Player_StartIdleMotion(Player *pl, s32 blend, s32 startFrame)
{
    u16 lowerMotion;
    u16 upperMotion;

    if (pl->rideState != 0) {
        lowerMotion = 0x18;
        upperMotion = 0x7C;
    } else if (pl->weaponOut != 0) {
        lowerMotion = 0x3E9;
        upperMotion = 0x44D;
    } else if (pl->staminaMax < 76 /* tired */) {
        lowerMotion = 0x193;
        upperMotion = 0x1F7;
    } else {
        switch (Area_Temperature(pl->area)) {
        case AREA_TEMP_NORMAL:
        default:
            lowerMotion = 1;
            upperMotion = 0x65;
            break;
        case AREA_TEMP_HOT:
            if (Player_HasSkill(pl, SKILL_HEAT_RES) == 1 || Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
                lowerMotion = 1;
                upperMotion = 0x65;
            } else {
                lowerMotion = 0x15;
                upperMotion = 0x79;
            }
            break;
        case AREA_TEMP_VERY_HOT:
            if (Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
                lowerMotion = 1;
                upperMotion = 0x65;
            } else {
                lowerMotion = 0x15;
                upperMotion = 0x79;
            }
            break;
        case AREA_TEMP_COLD:
            if (Player_HasSkill(pl, SKILL_COLD_RES) == 1 || Player_HasSkill(pl, SKILL_COLD_RES2) == 1) {
                lowerMotion = 1;
                upperMotion = 0x65;
            } else {
                lowerMotion = 0x1AF;
                upperMotion = 0x213;
            }
            break;
        case AREA_TEMP_VERY_COLD:
            if (Player_HasSkill(pl, SKILL_COLD_RES2) == 1) {
                lowerMotion = 1;
                upperMotion = 0x65;
            } else {
                lowerMotion = 0x1AF;
                upperMotion = 0x213;
            }
            break;
        }
    }
    Player_SetMotion(pl, lowerMotion, blend, startFrame, 0);
    Player_SetMotion(pl, upperMotion, blend, startFrame, 1);
}

/*
 * Puts pl in the idle state (group 0, state 0) with the idle motions of
 * Player_StartIdleMotion, set to start at the next motion update; the local
 * player sends the change unless `quiet`.
 */
void Player_SetIdle(Player *pl, s32 blend, s32 startFrame, s32 quiet)
{
    if (pl->rideState != 0) {
        pl->motionId[0] = 0x18;
        pl->motionId[1] = 0x7C;
    } else if (pl->weaponOut != 0) {
        pl->motionId[0] = 0x3E9;
        pl->motionId[1] = 0x44D;
    } else if (pl->staminaMax < 76 /* tired */) {
        pl->motionId[0] = 0x193;
        pl->motionId[1] = 0x1F7;
    } else {
        switch (Area_Temperature(pl->area)) {
        case AREA_TEMP_NORMAL:
        default:
            pl->motionId[0] = 1;
            pl->motionId[1] = 0x65;
            break;
        case AREA_TEMP_HOT:
            if (Player_HasSkill(pl, SKILL_HEAT_RES) == 1 || Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
                pl->motionId[0] = 1;
                pl->motionId[1] = 0x65;
            } else {
                pl->motionId[0] = 0x15;
                pl->motionId[1] = 0x79;
            }
            break;
        case AREA_TEMP_VERY_HOT:
            if (Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
                pl->motionId[0] = 1;
                pl->motionId[1] = 0x65;
            } else {
                pl->motionId[0] = 0x15;
                pl->motionId[1] = 0x79;
            }
            break;
        case AREA_TEMP_COLD:
            if (Player_HasSkill(pl, SKILL_COLD_RES) == 1 || Player_HasSkill(pl, SKILL_COLD_RES2) == 1) {
                pl->motionId[0] = 1;
                pl->motionId[1] = 0x65;
            } else {
                pl->motionId[0] = 0x1AF;
                pl->motionId[1] = 0x213;
            }
            break;
        case AREA_TEMP_VERY_COLD:
            if (Player_HasSkill(pl, SKILL_COLD_RES2) == 1) {
                pl->motionId[0] = 1;
                pl->motionId[1] = 0x65;
            } else {
                pl->motionId[0] = 0x1AF;
                pl->motionId[1] = 0x213;
            }
            break;
        }
    }
    pl->motionBlend[0] = blend / 2;
    pl->motionBlend[1] = blend / 2;
    pl->motionFrame[0] = startFrame;
    pl->motionFrame[1] = startFrame;
    pl->stateGroup = PL_GROUP_ACTION;
    pl->state = PL_ACT_IDLE;
    if (Player_IsLocal(pl) == 1 && quiet == 0) {
        QSync_SendPlayer(pl, 1, 0);
    }
}
