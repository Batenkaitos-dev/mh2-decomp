#ifndef QUEST_H
#define QUEST_H

#include "common.h"

/* One quest objective (8 bytes; QuestState.goals). */
typedef struct QuestGoal {
    s32 flags;                  /* 0x10: in use; 0x01/0x02: kind; 0x20: done (Records_AddQuestKills tests 0x30) */
    u16 target;                 /* monster kind or item id */
    s16 count;
} QuestGoal;

/*
 * A quest's information block in the stage file (gStageFile + header word 0;
 * gQuest.info points to it), 0x50 bytes; also the start of each guild quest
 * (lbguild.h: GuildQuest). Fields from sub_main's quest code and the lobby's
 * quest board (Quest_GetStars, Quest_GetText, Quest_FindDownloaded,
 * Quest_DrawOverview, Quest_DrawTerms, QuestDesk_BuildLists).
 */
typedef struct QuestInfo {
    u32 flags;                  /* 0x00: bits 0-2 shown in the quest list; 0x80000, 0x100000 */
    u16 stars;                  /* 0x04: the difficulty in bits 0-3 */
    u16 type;                   /* 0x06: the quest type, 1-6 (QuestDesk_BuildLists lists by it) */
    s32 fee;                    /* 0x08: the contract fee, paid from gSave.money (Quest_DrawTerms) */
    s32 reward;                 /* 0x0C */
    s32 penalty;                /* 0x10 */
    s32 rewardSubA;             /* 0x14: the reward for sub goal A (Quest_DrawTerms: gSubAPointsTexts) */
    s32 rewardSubB;             /* 0x18: the reward for sub goal B (gSubBPointsTexts) */
    s32 timeLimit;              /* 0x1C: frames */
    u32 map;                    /* 0x20: the quest map (Area_MapGroup(map): ElderForecast_DrawPanel, QuestList_DropKind; gStartAreaTables[map]: Quest_SetStartArea); u16 readers load it with lhu */
    u32 texts;                  /* 0x24: the texts (char **: the lobby's Quest_GetText; gQuest.texts); the stage file and a downloaded quest hold offsets in the file */
    u8 terms;                   /* 0x28: the order condition (Quest_CheckOrderTerms) */
    u8 unk29;
    u16 id;                     /* 0x2A: the quest id (Quest_FindDownloaded) */
    QuestGoal goals[3];         /* 0x2C: goals[0].target is the main monster (Monster_IsAvailable) */
    u8 clearType;               /* 0x44: copied to gQuest.clearType (QuestGoal_InitFromDef) */
    u8 pad45[3];
    u32 unk48;
    u32 unk4C;
} QuestInfo;

/* A reward item given at the end of the quest: id and count (QuestState.rewards). */
typedef struct StageFlag {
    u16 id;
    s8 val;
    u8 pad;
} StageFlag;

/* An item to deliver or gather during the quest (QuestState.targets). */
typedef struct QuestTarget {
    s32 state;                  /* 0x0: 0 done, 1 counted by Quest_CountTarget, 2 by Quest_CheckItemTargets */
    u16 item;                   /* 0x4 */
    s8 count;                   /* 0x6 */
    s8 idx;                     /* 0x7 */
} QuestTarget;

/* A hunting objective of the quest board (QuestState.huntGoals). */
typedef struct QuestHuntGoal {
    s16 type;                   /* 1: hunt, 2: capture */
    s16 kind;                   /* monster kind */
    s16 need;
    s16 reward;
} QuestHuntGoal;

/* Per side (Versus_Team of Player.side): QuestState.teams. */
typedef struct QuestTeam {
    u8 unk0;
    s8 emLevel;                 /* 0x1: the level of the side's monsters: baseLevel + D_0044B448[...], 0 to 15 (Versus_Init; Versus_AssignEmTeam sets em->level) */
    u8 gaugeLevel;              /* 0x2: the versus gauge gain level, 0 to 4 (from gQuestExtra team unk1B; + 1 on most hits; +10, +15 ... to Em.versusGauge) */
    s8 baseLevel;               /* 0x3: from gQuestExtra (Versus_Init) */
    u16 rageScale1;             /* 0x4: percent, 100 = normal (Em_RageScale1; Versus_ApplyTeamBits) */
    u16 rageScale2;             /* 0x6: percent (Em_RageScale2) */
    u16 sizeScale;              /* 0x8: percent (Em_ComputeSize) */
    u16 sizePct;                /* 0xA: the side's monsters' em->sizePct (Versus_AssignEmTeam) */
    u16 flags;                  /* 0xC: bits 0x01-0x20 (Versus_TeamHasFlag) */
    u16 item;                   /* 0xE: an item (Versus_ConsumeTeamItem) */
} QuestTeam;

/*
 * A stage file's header (gStageHeader = gQuest.header). In the file the u32
 * fields are offsets from the file's base gStageFile; Quest_LoadStage
 * (unk_3A1070.c) copies most of them into gQuest as pointers and relocates
 * `tables` in place. gDefaultStageHeader, the header used with no stage
 * file, holds pointers.
 */
typedef struct StageHeader {
    u32 info;                   /* 0x00: QuestInfo */
    u32 unk04;
    u32 supply;                 /* 0x08: the supply box items (Stage_GetSupplyItems) */
    u32 unk0C;
    u32 script;                 /* 0x10: the quest script, 8-byte ops (gQuest.script) */
    u32 emSpawns;               /* 0x14: the monster spawn entries per area and progress (gQuest.emSpawns) */
    u32 unk18;
    u32 areaData;               /* 0x1C: per area index, an offset (gQuest.areaData) */
    u32 areaRecords;            /* 0x20: 32 bytes per area index (gQuest.areaRecords) */
    u32 stageInfo; /* 0x24: two words, the map and its base camp (Quest_LoadStage: gScene.map, gScene.campArea) */
    u32 gatherPoints;           /* 0x28: the gathering points per area index (gQuest.gatherPoints) */
    u32 actionSpots; /* 0x2C: per area index, its ActionSpot list (gQuest.actionSpots) */
    u32 unk30;
    u32 areaSpawns; /* 0x34: AreaSpawns, 0xFFFF-terminated */
    u32 tables;                 /* 0x38: the gathering tables (s32 offsets; relocated to a pointer on load) */
    u32 dataTable;              /* 0x3C: id and offset pairs (gQuest.dataTable; Stage_FindData) */
    u32 seasonalData;           /* 0x40: by season and daytime (gQuest.seasonalData; Stage_GetSeasonalData) */
    s16 emSize; /* 0x44: the monsters' base size (QuestEm spawns q->size; Galex scales it by a random percentage) */
    s16 unk46;                  /* 0x46: row of gQuestTimerRanges */
    u32 emLevel;                /* 0x48: the monsters' starting level (gQuest.emLevel) */
    s32 hrpMain; /* 0x4C: the HR points for the main goal (Plsel_SumQuestRewards) */
    s32 unk50;                  /* 0x50: plsel's reward screen gives it as hrpMain when Quest_IsRunning(1) is not 1 */
    s32 hrpSubA; /* 0x54: the HR points for sub goal A */
    s32 hrpSubB; /* 0x58: the HR points for sub goal B */
    s8 tier; /* 0x5C: the stage tier, 0-3: the column of the per-tier tables (Stage_FindData, the spawn points, the fortress monsters); 1 and 2 count a hunt twice (QuestEm_CountHunted) */
    s8 unk5D;
    s8 timedEvent;              /* 0x5E: the timed event kind (gQuest.timedEvent) */
    s8 eventKind;               /* 0x5F: the monster kind the event counts (gQuest.eventKind) */
    s8 eventCount;              /* 0x60: the kills it needs (gQuest.eventCount) */
    u8 unk61[0x64 - 0x61];
    QuestTarget targets[2];     /* 0x64 */
} StageHeader;

/* An area's place in the map (32 bytes per area index: gQuest.areaRecords, the stage
 * file's StageHeader.areaRecords; Stage_GetAreaRecord). Positions in an area are local:
 * the map position is origin + local (unk_3AC9C0.c), the area's centre origin + size / 2. */
typedef struct AreaRecord {
    f32 originX;    /* 0x00 */
    f32 originZ;    /* 0x04 */
    f32 unk08[2];   /* 0x08 */
    f32 sizeX;      /* 0x10: positions stay in 8 .. sizeX - 8 (unk_328CD0.c) */
    f32 sizeZ;      /* 0x14 */
    f32 bottom;     /* 0x18: the floor's y */
    f32 top;        /* 0x1C */
} AreaRecord;
typedef char AreaRecord_size_check[(sizeof(AreaRecord) == 0x20) ? 1 : -1];

/*
 * gQuest (0x5D8260, 0x560 bytes): the running quest's state. Some sub_main
 * scalars alias its fields (gQuestTimeLeft = timeLeft, gQuestReward = reward,
 * gStageHeader = header, gQuestInfo = info, gQuestStats = stats).
 */
typedef struct QuestState {
    s8 state;                   /* 0x000: 0 running, 1 the clear sequence, 2 the reward and acknowledgement steps, 3... (Quest_Update's switch) */
    s8 step;                    /* 0x001: the step inside state (Quest_Update) */
    u8 unk002[0x003 - 0x002];
    s8 unk003;                  /* 0x003 */
    u8 savedEmType;             /* 0x004: the monster whose health carries over (QuestEm_SavePersistentHp, QuestEm_LoadPersistentHp) */
    u8 unk005[0x006 - 0x005];
    s8 result;                  /* 0x006: 4 cleared (Quest_Update, Versus_End), 6 failed (Versus_End), 7 aborted (Quest_Abort), 8 finished (Quest_Finish) */
    u8 unk007[0x008 - 0x007];
    u16 stage;                  /* 0x008: stage/quest id, 0: none */
    s8 unk00A;                  /* 0x00A */
    s8 goalLabel;               /* 0x00B: a label (op 0x1A) the script jumps to once the goal has flag 0x100 (unk_3A1070.c) */
    s32 scriptWait;             /* 0x00C: a countdown the quest script sets (Quest_StepScript: the next step when it reaches 0) */
    s32 timeLeft;               /* 0x010: frames (gQuestTimeLeft) */
    s32 reward;                 /* 0x014: quest money; faints subtract penalty (gQuestReward) */
    s32 penalty;                /* 0x018: the reward lost per faint */
    s16 emLeft;                 /* 0x01C: the monsters still to hunt or capture: QuestEm_Hunted, QuestEm_Captured and Quest_OnRemoteKill decrement it; CamReq_QuestTarget at 0 */
    s16 scriptPc;               /* 0x01E: the quest script's position (Quest_StepScript(pl, pc, out) steps it; Quest_Start finds op 0x26); -1 stopped (Quest_Clear, Quest_Abort, Quest_Finish) */
    s16 loopPc;                 /* 0x020: the script position op 0x17 jumps back to */
    s8 progress;                /* 0x022: the highest target index reached (Quest_CountTarget, Quest_CheckItemTargets); selects the spawn entries (Stage_FindEntryData) */
    s8 mineCount;               /* 0x023: the mining points in use (gMineActive) */
    struct Em *downEm;          /* 0x024: the monster that went down last (Quest_NoteTargetDown, Quest_OnRemoteKill): the camera shows it (CamReq_QuestTarget) */
    u32 flags;                  /* 0x028: quest flags */
    u8 unk02C[0x04C - 0x02C];
    struct StageHeader *header; /* 0x04C: gStageHeader */
    u8 unk050[0x054 - 0x050];
    void *script;               /* 0x054: the quest script, 8-byte ops (StageHeader.script; Quest_Start, Quest_SetFailed, Quest_SetCleared) */
    void *noticeTexts;          /* 0x058: the stage file's notice text offsets (header +0x30; Notice_PushItem adds gSysSettings.stageFile) */
    void *emSpawns;             /* 0x05C: the monster spawn entries per area and progress (StageHeader.emSpawns; Stage_FindEntryData) */
    void *stageEntry;           /* 0x060: the stage entry records: header unk18 + gStageFile (Quest_Init; StageEntry_GetData, Galex_PlaceRecords) */
    void *areaData;             /* 0x064: per area index (Map_AreaIndex), an offset into the stage file (StageHeader.areaData; Stage_GetAreaData64) */
    AreaRecord *areaRecords;    /* 0x068: 32 bytes per area index (StageHeader.areaRecords; Stage_GetAreaRecord) */
    void *texts;                /* 0x06C: the quest texts (QuestInfo.texts; QuestInfo_Get) */
    void *rewardTable;          /* 0x070: the stage reward entries (u16 x4 each, 0xFFFF ends; Quest_RollRewards) */
    void *gatherPoints;         /* 0x074: per area index, the gathering points (StageHeader.gatherPoints; Gather_GetPoints) */
    void *actionSpots; /* 0x078: per area index, its ActionSpot list (Stage_GetActionSpots; StageHeader.actionSpots relocated) */
    QuestInfo *info;            /* 0x07C: gQuestInfo */
    StageFlag rewards[5];       /* 0x080: items to give at the end: id and count (Quest_AddReward, Quest_FindReward, Quest_GiveRewards) */
    s8 rewardNext;              /* 0x094: next rewards slot to overwrite (Quest_AddReward, 0 to 4) */
    s8 rewardsPending;          /* 0x095: rewards holds items to give (Quest_DeductReward sets it, Quest_GiveRewards) */
    s8 unk096;                  /* 0x096 */
    s8 timeNoticeIdx;           /* 0x097: the next time-left notice: gQuestTimeNotices[it].time, counted down as each is shown (Quest_StepScript; Quest_Init from timeLeft) */
    struct Em *spawnedEm;       /* 0x098: the last monster spawned (QuestEm_Spawn); the camera's fallback target (CamReq_QuestTarget) */
    s8 unk09C;                  /* 0x09C */
    s8 timedEventSet;           /* 0x09D: timeLeft / 9000: the gQuestTimedEventTbls entry (Quest_UpdateTimedEvent) */
    u16 random;                 /* 0x09E: a random stream (Quest_Random), seeded from seed (Quest_SeedRandom) */
    u8 hunterRank[4][2]; /* 0x0A0: per player: [0] the hunter rank after the quest, [1] at its start (Save_RankFromPoints) */
    u8 unk0A8[0x128 - 0x0A8];
    u32 netMsgArgs;             /* 0x128: msgArg1 (high half) and msgArg2 (low half) of a message from the network (Quest_HandleNetMsg, Quest_Update) */
    u32 acks;                   /* 0x12C: a bit per player and step: bits 0-3, 4-7, 8-11 (Quest_Update waits for all 4 or for ackTimer) */
    s32 ackTimer;               /* 0x130: the wait for acks in frames: 5400, 1800 (Quest_Update, Versus_End) */
    s16 savedEmHp;              /* 0x134: its health */
    s8 emLevel;                 /* 0x136: the monsters' level: StageHeader.emLevel, +1 per appearance up to 15 (QuestEm_Appear, Galex_RollLevel) */
    s8 unk137;                  /* 0x137 */
    s8 unk138;                  /* 0x138 */
    u8 seed[15];                /* 0x139: random seed bytes (Quest_SeedRandom, Quest_GetSeedByte, Quest_SetStartArea) */
    s8 nextSide; /* 0x148: the side the next monster gets (Em.side = nextSide++, 1 and 2 in turn) */
    s8 timedEvent;              /* 0x149: the timed event kind (StageHeader.timedEvent; Quest_UpdateTimedEvent, Quest_CountEventKill); 0 none; the supply box tests it (SupplyBox_IsOpen) */
    s8 eventKind;               /* 0x14A: the monster kind the event counts (StageHeader.eventKind; Quest_CountEventKill) */
    s8 eventCount;              /* 0x14B: the kills it still needs (StageHeader.eventCount; Quest_CountEventKill) */
    u8 unk14C[0x152 - 0x14C];
    s8 sideEmLeft[2];           /* 0x152: per side, the monsters left: 3 (Versus_Init, Versus_MonsterDown) */
    u8 clearType;               /* 0x154: 1..4 (Records_AddQuestKills) */
    u8 versusDefeat;            /* 0x155: bits 0/1: side 0/1 defeated (Versus_MarkDefeat, Versus_BothDefeated); bits 2, 3 (Versus_DefeatCamera); kept by Quest_Clear */
    s16 inputLock;              /* 0x156: frames the pad and the field menu stay locked: 60 at the start, 90 in versus (Quest_Update counts down; Player_CopyPadInput, FieldMenu_Requested) */
    u16 traderWantItem;         /* 0x158: the item the trader kind 0x0A asks for in this quest, 0: none yet (EmKind0A_Trade picks it, clears it when traded) */
    s8 unk15A;                  /* 0x15A */
    u8 unk15B;                  /* 0x15B */
    s8 unk15C;                  /* 0x15C */
    u8 unk15D[0x160 - 0x15D];
    QuestGoal goals[3];         /* 0x160 */
    QuestGoal goals2[3];        /* 0x178: a copy with the sub-quest flags */
    QuestTeam teams[2];         /* 0x190: per side (Versus_Team) */
    QuestTarget targets[2];     /* 0x1B0 */
    u16 specialMineArea;        /* 0x1C0: the special mining point's area (SpecialMine_Create: p->area) */
    u8 unk1C2[0x1C4 - 0x1C2];
    u16 specialMinePoint;       /* 0x1C4: gSpecialMinePoint (0: none; SpecialMine_Create) */
    u8 unk1C6;                  /* 0x1C6 */
    s8 specialMineSlot;         /* 0x1C7: MinePoint_Add's result for it (SpecialMine_Create) */
    s32 specialMineAge;         /* 0x1C8: gSpecialMineAge */
    f32 specialMinePos[3];      /* 0x1CC: its position (SpecialMine_Create; func_86BBC0) */
    u8 stats[0x10];             /* 0x1D8: the drill statistics by id (QuestStats_Add; [10] all goals done, [11] the charm kept), gQuestStats (QuestStats_Add, QuestStats_Tally; plsel's rewards read them by gPlselRewardSteps) */
    u16 markMonster;            /* 0x1E8: a monster kind rolled from gMarkMonsterCands (QuestStats_Init): plsel marks its line in the kill list */
    u16 markHunt;               /* 0x1EA: rolled 0..15 (QuestStats_Init): plsel marks the reward line of that category */
    s32 clearTime;              /* 0x1EC: the quest time in frames (QuestStats_Tally: gQuestPartyClearTime; the dojo's par time test) */
    u8 drillCharm;              /* 0x1F0: the drill's charm: 0 not taken, 1 taken (Quest_DrillTakeCharm), 0xFF used up by a faint (Quest_PlayerFainted) */
    u8 unk1F1[0x1F4 - 0x1F1];
    QuestHuntGoal huntGoals[3]; /* 0x1F4: the hunting goals, copied from D_006ADB30 (Quest_Init); plsel's rewards read them */
    u16 hunted[80];             /* 0x20C: per monster kind, this quest */
    u16 captured[80];           /* 0x2AC */
    u16 sizeMax[80];            /* 0x34C */
    u16 sizeMin[80];            /* 0x3EC */
    u16 parts[80];              /* 0x48C: broken parts, one bit each */
    u32 elapsed;                /* 0x52C: the quest time: Quest_GetTime(1) - Quest_GetTime(0) (Quest_Update, Quest_StoreElapsed, Versus_UpdateRecords) */
    void *dataTable;            /* 0x530: id and offset pairs, 0-terminated (StageHeader.dataTable; Stage_FindData) */
    void *seasonalData;         /* 0x534: per entry two offsets of id/offset tables by season and daytime (StageHeader.seasonalData; Stage_GetSeasonalData through D_005D8794) */
    u8 unk538[0x53A - 0x538];
    u16 savedEmParts;           /* 0x53A: its broken parts */
    s16 capturedType;           /* 0x53C: the captured monster's kind (QuestEm_Captured) */
    u16 capturedSize;           /* 0x53E: its size (em->sizePct; plsel's record menu) */
    s8 capturedLevel;           /* 0x540: its level - 3, 1 to 12 (QuestEm_Captured; saved by Plsel_SaveRecordMenu) */
    u8 goalIdx[3];              /* 0x541: per goal: its index, 0xFF none (QuestGoal_InitFromDef, QuestGoal_Complete, CamReq_QuestTarget) */
    s32 countdown;              /* 0x544: frames: 18000 (Quest_StartCountdown, Quest_UpdateCountdown; gScene.countdownOn) */
    s8 countdownMsg;            /* 0x548: the countdown's message counter (Quest_UpdateCountdown) */
    u8 unk549[0x54A - 0x549];
    u8 anyFainted;              /* 0x54A: a player fainted and cost the reward (Quest_PlayerFainted) */
    u8 unk54B[0x550 - 0x54B];
    u8 msgFrom;                 /* 0x550: the quest net message (10 bytes): the sending player (QSync_OnState, Quest_HandleNetMsg) */
    u8 msgType;                 /* 0x551: its type (Quest_HandleNetMsg's switch; Quest_Update sets 12, 13, 15) */
    s16 msgArg0;                /* 0x552: its arguments */
    s16 msgArg1;                /* 0x554 */
    s16 msgArg2;                /* 0x556 */
    u16 msgArea;                /* 0x558: the area of a kill (Quest_SendKill), the goal of QuestGoal_Complete: read as u16 */
    u8 unk55A[0x560 - 0x55A];
} QuestState;

extern QuestState gQuest;
extern StageHeader *gStageHeader;     /* gQuest.header */
extern StageHeader gDefaultStageHeader;

#endif /* QUEST_H */
