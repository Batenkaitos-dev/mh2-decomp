#ifndef QUEST_H
#define QUEST_H

#include "common.h"

/*
 * One quest goal (8 bytes; QuestInfo.goals, QuestState.goals and goalsSet).
 * Goal 0 is the main goal; goals 1 and 2 are the sub-quests, or more main
 * goals, as the clear type says (QUEST_CLEAR_*).
 */
typedef struct QuestGoal {
    s32 flags;                  /* QUEST_GOAL_* */
    u16 target;                 /* the monster kind, or the item to deliver */
    s16 count;                  /* the monsters or items still wanted; a part goal's part index; a damage goal's damage / 100 */
} QuestGoal;

#define QUEST_GOAL_COUNT        3       /* the goal slots */
#define QUEST_REWARD_SLOTS      5       /* QuestState.rewards: the items Melynxes stole, given back at the end */
#define DELIVERED_SLOTS         4       /* Player.delivered: the items a player delivered for the quest */

/* Quest_CountDelivered's `who`: */
#define DELIVERED_ALL           0       /* all the players */
#define DELIVERED_LOCAL         1       /* the local player */
#define DELIVERED_PLAYER0       2       /* + a player's index: that player */

/*
 * QuestGoal.flags (unk_3A1070.c's QuestGoal_*). The kind bits come from the
 * quest's data; DONE, FAILED and FAIL_SEEN are set while it runs.
 */
#define QUEST_GOAL_HUNT         0x01    /* hunt `count` monsters of kind `target` (killed or captured) */
#define QUEST_GOAL_DELIVER      0x02    /* deliver `count` of item `target` */
#define QUEST_GOAL_EM           0x04    /* a goal on the monster `target` itself: with PART or DAMAGE */
#define QUEST_GOAL_USED         0x10    /* the slot holds a goal (QuestGoal_Add) */
#define QUEST_GOAL_DONE         0x20
#define QUEST_GOAL_FAILED       0x40    /* a capture-only target was killed, or a kill-only one captured */
#define QUEST_GOAL_LOCAL        0x80    /* the delivery counts the local player's items only (Quest_DeliveredCount) */
#define QUEST_GOAL_CAPTURE      0x100   /* capture only: killing the target fails the goal */
#define QUEST_GOAL_SLAY         0x200   /* kill only: capturing the target fails the goal */
#define QUEST_GOAL_FAIL_SEEN    0x400   /* FAILED has sent the script to its end (Quest_CheckClear) */
#define QUEST_GOAL_DELIVER_HOW  0x3000  /* how a delivery is made (QuestState.goalsSet flags): */
#define QUEST_GOAL_DELIVER_BOX  0x0000  /* handed in at the delivery spot (Quest_GetDeliveryTarget) */
#define QUEST_GOAL_DELIVER_SET  0x1000  /* set down where it is used (Quest_DeliverFromPouch mode 1, from the game's trap setting); QuestGoal_DeliveryDone */
#define QUEST_GOAL_DELIVER_HOW2 0x2000  /* Quest_DeliverFromPouch mode 2: no caller seen */
#define QUEST_GOAL_PART         0x4000  /* with EM: break the part `count` of the monster `target` */
#define QUEST_GOAL_DAMAGE       0x8000  /* with EM: deal it `count` * 100 damage (since gQuest.savedEmHp) */
#define QUEST_GOAL_HALF_TIME    0x10000 /* with DAMAGE: only once half the time is gone; the monster then leaves wounded */
#define QUEST_GOAL_MONSTER      (QUEST_GOAL_HUNT | QUEST_GOAL_EM)  /* a goal on a monster (the lobby and the camera test it) */
#define QUEST_GOAL_ENDED        (QUEST_GOAL_DONE | QUEST_GOAL_FAILED)

/* The combinations the goal code tests. */
#define QUEST_GOAL_HUNT_USED    (QUEST_GOAL_USED | QUEST_GOAL_HUNT)
#define QUEST_GOAL_HUNT_DONE    (QUEST_GOAL_USED | QUEST_GOAL_DONE | QUEST_GOAL_HUNT)
#define QUEST_GOAL_DELIVER_DONE (QUEST_GOAL_USED | QUEST_GOAL_DONE | QUEST_GOAL_DELIVER)
#define QUEST_GOAL_USED_DONE    (QUEST_GOAL_USED | QUEST_GOAL_DONE)    /* any goal in use and met */
#define QUEST_GOAL_EM_DONE      (QUEST_GOAL_USED | QUEST_GOAL_DONE | QUEST_GOAL_EM)
#define QUEST_GOAL_DELIVER_USED (QUEST_GOAL_USED | QUEST_GOAL_DELIVER)
#define QUEST_GOAL_PART_USED    (QUEST_GOAL_USED | QUEST_GOAL_EM | QUEST_GOAL_PART)
#define QUEST_GOAL_DAMAGE_USED  (QUEST_GOAL_USED | QUEST_GOAL_EM | QUEST_GOAL_DAMAGE)

#define QUEST_GOAL_NONE         0xFF    /* an empty QuestState.goalIdx entry */

/*
 * QuestGoal_Complete's reason: the flag of the way the goal was met, which
 * also picks the message event (MsgEvent_Check).
 */
#define GOAL_MET_DELIVERED      QUEST_GOAL_DELIVER
#define GOAL_MET_CAPTURED       QUEST_GOAL_CAPTURE
#define GOAL_MET_KILLED         QUEST_GOAL_SLAY
#define GOAL_MET_PART           QUEST_GOAL_PART
#define GOAL_MET_DAMAGE         QUEST_GOAL_DAMAGE

/*
 * QuestState.clearType (QuestInfo.clearType, QUEST_OP_CLEAR_TYPE): which
 * done goals clear the quest (Quest_CheckClear), and which are sub-quests
 * (Quest_CheckSubClear).
 */
#define QUEST_CLEAR_ANY         1       /* any goal */
#define QUEST_CLEAR_MAIN        2       /* goal 0; goals 1 and 2 are sub-quests */
#define QUEST_CLEAR_FIRST_TWO   3       /* goals 0 and 1; goal 2 is a sub-quest */
#define QUEST_CLEAR_ALL         4       /* every goal in use */

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
    u32 map;                    /* 0x20: the quest map (Map_Group(map): ElderForecast_DrawPanel, QuestList_DropKind; gStartAreaTables[map]: Quest_SetStartArea); u16 readers load it with lhu */
    u32 texts;                  /* 0x24: the texts (char **: the lobby's Quest_GetText; gQuest.texts); the stage file and a downloaded quest hold offsets in the file */
    u8 terms;                   /* 0x28: the order condition (Quest_CheckOrderTerms) */
    u8 unk29;
    u16 id;                     /* 0x2A: the quest id (Quest_FindDownloaded) */
    QuestGoal goals[QUEST_GOAL_COUNT]; /* 0x2C: goals[0].target is the main monster (Monster_IsAvailable) */
    u8 clearType;               /* 0x44: copied to gQuest.clearType (QuestGoal_InitFromDef) */
    u8 pad45[3];
    u32 unk48;
    u32 unk4C;
} QuestInfo;

/*
 * QuestInfo.flags, copied to gQuest.flags (Quest_Init). Online, the lobby
 * also copies a quest's flags to NextQuest.flags before it starts.
 */
#define QUEST_FLAG_FIXED_LEVEL      0x4000      /* the monsters take gQuest.emLevel as it is (QuestEm_RollLevel) */
#define QUEST_FLAG_NO_LEVEL_UP      0x8000      /* a monster that appears again keeps its level (QuestEm_Appear) */
#define QUEST_FLAG_BREEDING         0x10000     /* a downloaded quest's season (Quest_Init): breeding, */
#define QUEST_FLAG_COLD             0x40000     /* cold, or else warm */
#define QUEST_FLAG_DAY              0x80000     /* a downloaded quest's time of day: day, or else night */
#define QUEST_FLAG_DRILL            0x200000    /* a drill: the charm (Quest_DrillTakeCharm), Quest_HasDrillFlag */
#define QUEST_FLAG_MUST_ORDER       0x400000    /* Quest_IsMustOrder */
#define QUEST_FLAG_SET_BREEDING     0x800000    /* with QUEST_FLAGS_FIXED_SEASON: the breeding season, */
#define QUEST_FLAG_SET_WARM         0x1000000   /* the warm season, or else the cold one (Quest_Init) */
#define QUEST_FLAG_TRAINING         0x4000000   /* Quest_IsTraining */
#define QUEST_FLAG_ADVICE           0x8000000   /* with TRAINING, Quest_HasAdvice: one more entry in the offline field menu (FieldMenu_Open) */
/* A built-in quest with one of these flags has its season set by the flags
 * above instead of the current one (Quest_Init). */
#define QUEST_FLAGS_FIXED_SEASON    (QUEST_FLAG_TRAINING | QUEST_FLAG_MUST_ORDER | QUEST_FLAG_DRILL)

/*
 * The season and the time of day (the boot ELF's Season_Get, Daytime_Get;
 * the season banner names them in this order, unk_386160.c). A stage file
 * has six variants, file + daytime * 3 + season (Quest_Init).
 */
#define SEASON_WARM         0
#define SEASON_COLD         1
#define SEASON_BREEDING     2
#define DAYTIME_DAY         0
#define DAYTIME_NIGHT       1
/* Daytime_Set's argument: the bit of the season flags it sets (3 gives
 * Daytime_Get 0, 4 gives 1). */
#define DAYTIME_SET_DAY     3
#define DAYTIME_SET_NIGHT   4

/* A reward item given at the end of the quest: id and count (QuestState.rewards). */
typedef struct StageFlag {
    u16 id;
    s8 val;
    u8 pad;
} StageFlag;

/* A target that moves the quest's progress on (QuestState.targets): monsters
 * to hunt or items to hold. */
typedef struct QuestTarget {
    s32 state;                  /* 0x0: TARGET_* */
    u16 item;                   /* 0x4: the item, or for TARGET_KILLS the monster kind */
    s8 count;                   /* 0x6 */
    s8 idx;                     /* 0x7 */
} QuestTarget;

#define TARGET_DONE     0
#define TARGET_KILLS    1       /* counted by Quest_CountTarget */
#define TARGET_ITEMS    2       /* checked by Quest_CheckItemTargets */

/* A step of a timed supply event's table (gQuestTimedEventTbls[gQuest.timedEventSet], Quest_UpdateTimedEvent). */
typedef struct TimedSupply {
    u8 minutes;                 /* the time left, in minutes, the step rolls at; 0 ends the table */
    u8 chance;                  /* out of 128: the supplies arrive */
} TimedSupply;

/* gQuest.timedEvent: how the supplies arrive (the supply box opens with them). */
#define QUEST_EVENT_NONE    0   /* none, or they have arrived */
#define QUEST_EVENT_TIMED   1   /* rolled at times of the time left (Quest_UpdateTimedEvent) */
#define QUEST_EVENT_KILLS   2   /* after eventCount kills of eventKind (Quest_CountEventKill) */

/*
 * A monster of the quest, 0x3C bytes, in the stage file's monster lists
 * (gQuestEmSets, and per area and progress gQuestEmAreaSets; a list ends
 * with kind 0). QuestEm_Init numbers them and QuestEm_Spawn creates the
 * monster from it; the game overlay's em_kushala.c and em_bulldrome.c
 * keep their own copy of it.
 */
typedef struct QuestEm {
    s16 kind;                   /* 0x00: the monster kind; 0 ends the list */
    s16 variant;                /* 0x02: -> Em.variant; for kind 0x0A, gQuest.unk15A picks the entries with variant 0 or not (QuestEm_SpawnArea) */
    s8 count;                   /* 0x04: how many more times it appears */
    u8 unk05;
    u16 spawnParam;             /* 0x06: -> Em.spawnParam */
    u16 area;                   /* 0x08 */
    s8 noReappear;              /* 0x0A: 1: QuestEm_CanAppear refuses it */
    s8 spawn;                   /* 0x0B: 0 at (x, y, z), 1 at a random point of the area (QuestEm_Appear) */
    s32 need1Gauge;             /* 0x0C: -> Em.need1Gauge */
    s32 need2Gauge;             /* 0x10: -> Em.need2Gauge */
    s32 need3Gauge;             /* 0x14: -> Em.need3Gauge */
    s32 questKey;               /* 0x18: -> Em.questKey */
    s32 rotY;                   /* 0x1C: -> Em.rotY */
    f32 x;                      /* 0x20 */
    f32 y;
    f32 z;
    s16 index;                  /* 0x2C: its number in the quest (QuestEm_Init; Em.questIndex, QuestEm_FindByIndex) */
    s16 flags;                  /* 0x2E: QUEST_EM_* */
    u16 size;                   /* 0x30: in percent (Em.sizePct) */
    s16 hp;                     /* 0x32: the health it comes back with; -1: the monster's own */
    s8 emIndex;                 /* 0x34: Em.index of its monster, -1: none (QuestEm_FindForEm) */
    s8 large;                   /* 0x35: gEmShowsOnMap[kind]: it counts in gQuest.emLeft */
    s8 level;                   /* 0x36: 0..16 */
    u8 unk37[0x3C - 0x37];
} QuestEm;

/* QuestEm.flags (one value at a time, but for QUEST_EM_CAPTURED). */
#define QUEST_EM_DONE       0x1     /* hunted or captured with no appearance left */
#define QUEST_EM_ACTIVE     0x2     /* may appear (QuestEm_Init, QuestEm_Appear) */
#define QUEST_EM_HUNTED     0x4     /* down, to appear again (QuestEm_ResetArea makes it active) */
#define QUEST_EM_CAPTURED   0x8     /* added once captured (QuestEm_Captured) */

/* A hunting objective of the quest board (QuestState.huntGoals). */
typedef struct QuestHuntGoal {
    s16 type;                   /* 1: hunt, 2: capture */
    s16 kind;                   /* monster kind */
    s16 need;
    s16 reward;
} QuestHuntGoal;

/*
 * A side of a versus quest as the lobby sends it (NextQuest.teams, 0x1C
 * bytes; the lobby copies both sides in one 0x38-byte block). Versus_Init
 * turns it into the side's QuestTeam.
 */
typedef struct NextQuestTeam {
    u8 unk00[0x0A];
    s16 emKind;                 /* 0x0A: the side's monster kind (QuestEm_SpawnAll, Versus_Init) */
    u16 sizePct;                /* 0x0C: -> QuestTeam.sizePct */
    u8 name[0x17 - 0x0E];       /* 0x0E: drawn with the side's health bar (Hud_DrawVersusMonsterBars) */
    s8 gaugeLevel;              /* 0x17: -> QuestTeam.gaugeLevel */
    s8 levelRoll;               /* 0x18: Versus_Init adds gVersusLevelOffsets[levelRoll] to baseLevel */
    s8 baseLevel;               /* 0x19: -> QuestTeam.baseLevel */
    u8 unk1A[0x1C - 0x1A];
} NextQuestTeam;

/*
 * The quest to start next (gNextQuest, 0x6ADAD0, 0x80 bytes): what the
 * lobby, the quest menus and the network set before Quest_Init reads it.
 * The select and plsel overlays clear it whole. gNextQuestId is its
 * field `stage`.
 */
typedef struct NextQuest {
    u32 flags;                  /* 0x00: the quest's QuestInfo.flags (QUEST_FLAG_*: the lobby copies them) */
    NextQuestTeam teams[2];     /* 0x04: versus: the two sides */
    u32 teamItems[2];           /* 0x3C: versus: per side, an item (QuestTeam.item) */
    s16 arenaRecord;            /* 0x44: the arena record the versus quest uses (gSave.arenaEms index; Versus_ConsumeTeamItem, Versus_UpdateRecords); the lobby sets it from the arena role (-1: none) */
    u8 unk46[0x5E - 0x46];      /* 0x46: the lobby copies 6 bytes per arena role here */
    s8 unk5E;                   /* 0x5E: versus (QUEST_OP_VERSUS_END): 0, a side wins with its hunt goal (goal 0 or 1); else when the other side has no monster left */
    u8 timeLimit;               /* 0x5F: versus: the time limit in minutes */
    QuestHuntGoal huntGoals[3]; /* 0x60: copied to gQuest.huntGoals (Quest_Init) */
    u16 stage;                  /* 0x78: the quest id (gNextQuestId) */
    u16 map;                    /* 0x7A: the quest's map (the lobby sets QuestInfo.map) */
    s8 flyer;                   /* 0x7C: copied to gQuest.flyer */
    u8 pad7D[0x80 - 0x7D];
} NextQuest;

/* Per side (Versus_Team of Player.side): QuestState.teams. */
typedef struct QuestTeam {
    u8 pose;                    /* 0x0: the side's monster pose, 0-2 (Versus_RouteEmByTeam: versus state 1-3), from the side's item (Versus_Init: gTeamItemBits) */
    s8 emLevel;                 /* 0x1: the level of the side's monsters: baseLevel + gVersusLevelOffsets[...], 0 to 15 (Versus_Init; Versus_AssignEmTeam sets em->level) */
    u8 gaugeLevel;              /* 0x2: the versus gauge gain level, 0 to 4 (from gNextQuest team unk1B; + 1 on most hits; +10, +15 ... to Em.versusGauge) */
    s8 baseLevel;               /* 0x3: from gNextQuest (Versus_Init) */
    u16 rageScale1;             /* 0x4: percent, 100 = normal (Em_LevelAttackFactor; Versus_ApplyTeamBits) */
    u16 rageScale2;             /* 0x6: percent (Em_LevelDamageFactor) */
    u16 sizeScale;              /* 0x8: percent (Em_ComputeMaxHp) */
    u16 sizePct;                /* 0xA: the side's monsters' em->sizePct (Versus_AssignEmTeam) */
    u16 flags;                  /* 0xC: bits 0x01-0x20 (Versus_TeamHasFlag) */
    u16 item;                   /* 0xE: an item (Versus_ConsumeTeamItem) */
} QuestTeam;

/* An entry of gTeamItemBits and of gMonsterNoteMarks' lists: what a side's item gives its
 * QuestTeam (Versus_Init; the lobby's arena feed lists the items). */
typedef struct TeamEntry {
    u16 key;                    /* 0x0: an item; 0 ends the table */
    u16 pose;                   /* 0x2: -> QuestTeam.pose */
    u32 bits;                   /* 0x4: the team's bits (Versus_ApplyTeamBits) */
} TeamEntry;

/*
 * A stage file's header (gStageHeader = gQuest.header). In the file the u32
 * fields are offsets from the file's base gStageFile; Quest_LoadStage
 * (unk_3A1070.c) copies most of them into gQuest as pointers and relocates
 * `tables` in place. gDefaultStageHeader, the header used with no stage
 * file, holds pointers.
 */
typedef struct StageHeader {
    u32 info;                   /* 0x00: QuestInfo */
    u32 startAreas;             /* 0x04: StartArea[4], one per player (Quest_SetStartArea) */
    u32 supply;                 /* 0x08: the supply box items (Stage_GetSupplyItems) */
    u32 rewardTable;            /* 0x0C: the reward entries (gQuest.rewardTable) */
    u32 script;                 /* 0x10: the quest script, 8-byte ops (gQuest.script) */
    u32 emSpawns;               /* 0x14: the monster spawn entries per area and progress (gQuest.emSpawns) */
    u32 stageEntry;             /* 0x18: the stage entry records (gQuest.stageEntry) */
    u32 areaData;               /* 0x1C: per area index, an offset (gQuest.areaData) */
    u32 areaRecords;            /* 0x20: 32 bytes per area index (gQuest.areaRecords) */
    u32 stageInfo; /* 0x24: two words, the map and its base camp (Quest_LoadStage: gScene.map, gScene.campArea) */
    u32 gatherPoints;           /* 0x28: the gathering points per area index (gQuest.gatherPoints) */
    u32 actionSpots; /* 0x2C: per area index, its ActionSpot list (gQuest.actionSpots) */
    u32 noticeTexts;            /* 0x30: the notice text offsets (gQuest.noticeTexts) */
    u32 areaSpawns; /* 0x34: AreaSpawns, 0xFFFF-terminated */
    u32 tables;                 /* 0x38: the gathering tables (s32 offsets; relocated to a pointer on load) */
    u32 dataTable;              /* 0x3C: id and offset pairs (gQuest.dataTable; Stage_FindData) */
    u32 seasonalData;           /* 0x40: by season and daytime (gQuest.seasonalData; Stage_GetSeasonalData) */
    s16 emSize; /* 0x44: the base size of the monsters that have a random size (Em_HasRandomSize): QuestEm.size is it times QuestEm_RollSize's percent, plus the season's step (plsel's gallery uses 100 percent: Galex_RollEmSize) */
    s16 sizeRange;              /* 0x46: the row of gEmSizeRanges the rolled monster sizes stay in (QuestEm_RollSize) */
    u32 emLevel;                /* 0x48: the monsters' starting level (gQuest.emLevel) */
    s32 hrpMain; /* 0x4C: the HR points for the main goal (Plsel_SumQuestRewards) */
    s32 unk50;                  /* 0x50: plsel's reward screen gives it as hrpMain when Quest_IsRunning(1) is not 1 */
    s32 hrpSubA; /* 0x54: the HR points for sub goal A */
    s32 hrpSubB; /* 0x58: the HR points for sub goal B */
    s8 tier; /* 0x5C: the stage tier, 0-3: the column of the per-tier tables (Stage_FindData, the spawn points, the fortress monsters); 1 and 2 count a hunt twice (QuestEm_CountHunted) */
    s8 randomStart;             /* 0x5D: 0: each player starts in the stage file's start area; n: in an area rolled from gStartAreaTables[map][n - 1] (Quest_SetStartArea), with the area change jingle (Jingle_OnAreaChange) */
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
    s8 timeoutLabel;            /* 0x00A: the script's label to jump to when the time runs out (QUEST_OP_BEGIN, QUEST_OP_TIMEOUT_LABEL) */
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
    void *script;               /* 0x054: the quest script, 8-byte ops (StageHeader.script; Quest_Start, Quest_JumpToEnd, Quest_JumpToStart) */
    void *noticeTexts;          /* 0x058: the stage file's notice text offsets (header +0x30; Notice_PushItem adds gSysSettings.stageFile) */
    void *emSpawns;             /* 0x05C: the monster spawn entries per area and progress (StageHeader.emSpawns; Stage_FindEntryData) */
    void *stageEntry;           /* 0x060: the stage entry records: header unk18 + gStageFile (Quest_Init; StageEntry_GetData, Galex_PlaceRecords) */
    void *areaData;             /* 0x064: per area index (Map_AreaIndex), an offset into the stage file (StageHeader.areaData; Stage_GetAreaData) */
    AreaRecord *areaRecords;    /* 0x068: 32 bytes per area index (StageHeader.areaRecords; Stage_GetAreaRecord) */
    void *texts;                /* 0x06C: the quest texts (QuestInfo.texts; QuestInfo_Get) */
    void *rewardTable;          /* 0x070: the stage reward entries (u16 x4 each, 0xFFFF ends; Quest_RollRewards) */
    void *gatherPoints;         /* 0x074: per area index, the gathering points (StageHeader.gatherPoints; Gather_GetPoints) */
    void *actionSpots; /* 0x078: per area index, its ActionSpot list (Stage_GetActionSpots; StageHeader.actionSpots relocated) */
    QuestInfo *info;            /* 0x07C: gQuestInfo */
    StageFlag rewards[QUEST_REWARD_SLOTS]; /* 0x080: items to give at the end: id and count (Quest_AddReward, Quest_FindReward, Quest_GiveRewards) */
    s8 rewardNext;              /* 0x094: next rewards slot to overwrite (Quest_AddReward, 0 to 4) */
    s8 rewardsPending;          /* 0x095: rewards holds items to give (Quest_DeductReward sets it, Quest_GiveRewards) */
    s8 unk096;                  /* 0x096 */
    s8 timeNoticeIdx;           /* 0x097: the next time-left notice: gQuestTimeNotices[it].time, counted down as each is shown (Quest_StepScript; Quest_Init from timeLeft) */
    struct Em *spawnedEm;       /* 0x098: the last monster spawned (QuestEm_Spawn); the camera's fallback target (CamReq_QuestTarget) */
    s8 timedEventStep;          /* 0x09C: the next entry of the timed event's table (Quest_UpdateTimedEvent) */
    s8 timedEventSet;           /* 0x09D: timeLeft / 9000: the gQuestTimedEventTbls entry (Quest_UpdateTimedEvent) */
    u16 random;                 /* 0x09E: a random stream (Quest_Random), seeded from seed (Quest_SeedRandom) */
    u8 hunterRank[4][2]; /* 0x0A0: per player: [0] the hunter rank after the quest, [1] at its start (Save_RankFromPoints) */
    u8 unk0A8[0x128 - 0x0A8];
    u32 netMsgArgs;             /* 0x128: msgArg1 (high half) and msgArg2 (low half) of a message from the network (Quest_HandleNetMsg, Quest_Update) */
    u32 acks;                   /* 0x12C: a bit per player and step: bits 0-3, 4-7, 8-11 (Quest_Update waits for all 4 or for ackTimer) */
    s32 ackTimer;               /* 0x130: the wait for acks in frames: 5400, 1800 (Quest_Update, Versus_End) */
    s16 savedEmHp;              /* 0x134: its health */
    s8 emLevel;                 /* 0x136: the monsters' level: StageHeader.emLevel, +1 per appearance up to 15 (QuestEm_Appear, Galex_RollLevel) */
    s8 eventLabel;              /* 0x137: a label (QUEST_OP_EVENT_LABEL) the script jumps to once eventTriggered is set */
    s8 eventTriggered;          /* 0x138: set by Quest_TriggerEvent */
    u8 seed[15];                /* 0x139: random seed bytes (Quest_SeedRandom, Quest_GetSeedByte, Quest_SetStartArea) */
    s8 nextSide; /* 0x148: the side the next monster gets (Em.side = nextSide++, 1 and 2 in turn) */
    s8 timedEvent;              /* 0x149: the timed event kind (StageHeader.timedEvent; Quest_UpdateTimedEvent, Quest_CountEventKill); 0 none; the supply box tests it (SupplyBox_IsOpen) */
    s8 eventKind;               /* 0x14A: the monster kind the event counts (StageHeader.eventKind; Quest_CountEventKill) */
    s8 eventCount;              /* 0x14B: the kills it still needs (StageHeader.eventCount; Quest_CountEventKill) */
    u8 unk14C[0x152 - 0x14C];
    s8 sideEmLeft[2];           /* 0x152: per side, the monsters left: 3 (Versus_Init, Versus_MonsterDown) */
    u8 clearType;               /* 0x154: QUEST_CLEAR_* (Records_AddQuestKills) */
    u8 versusDefeat;            /* 0x155: bits 0/1: side 0/1 defeated (Versus_MarkDefeat, Versus_BothDefeated); bits 2, 3 (Versus_DefeatCamera); kept by Quest_Clear */
    s16 inputLock;              /* 0x156: frames the pad and the field menu stay locked: 60 at the start, 90 in versus (Quest_Update counts down; Player_CopyPadInput, FieldMenu_Requested) */
    u16 traderWantItem;         /* 0x158: the item the trader kind 0x0A asks for in this quest, 0: none yet (EmKind0A_Trade picks it, clears it when traded) */
    s8 traderVariant;           /* 0x15A: which entries of monster kind 0x0A appear: those with QuestEm.variant nonzero, or 0 (rolled from gTraderVariantRolls; 0 in a drill; QuestEm_SpawnArea) */
    u8 unk15B;                  /* 0x15B */
    s8 flyer;                   /* 0x15C: nonzero: the area's flyer object appears (StageObj_AreaFlyer_Create); from NextQuest.flyer (Quest_Init); kept in the quest record */
    u8 unk15D[0x160 - 0x15D];
    QuestGoal goals[QUEST_GOAL_COUNT]; /* 0x160: the goals as they stand: the counts still wanted, DONE and FAILED */
    QuestGoal goalsSet[QUEST_GOAL_COUNT]; /* 0x178: the goals as set: the counts wanted at the start (QuestGoal_Add) */
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
    u8 goalIdx[3];              /* 0x541: the goals in the order they were done, 0xFF: none yet (QuestGoal_InitFromDef, QuestGoal_Complete, CamReq_QuestTarget) */
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

/* An id and an offset in the stage file (StageHeader.dataTable, the seasonal
 * tables; Stage_FindData, Stage_GetSeasonalData in stage_data.c); online, the
 * built-in lists hold a pointer as the offset. Id 0 ends a list. */
typedef struct IdOffset {
    s32 id;
    s32 offset;
} IdOffset;

/* An entry of a stage table by id (an area for the monster spawns; gQuest.stageEntry,
 * gQuest.emSpawns): two parts, each an offset in the stage file
 * (StageEntry_GetData, Stage_FindEntryData). Id 0 ends a list. */
typedef struct StageEntry {
    s32 id;
    s32 unk4;
    s32 ids;            /* 0x8: part 0: the ids (some tables: a count) */
    s32 records;        /* 0xC: part 1: the records (QuestEm spawns) */
} StageEntry;

/* A gathering point of an area (Gather_GetPoints; gather.c), 24 bytes; a
 * list ends with x == -1.0f. */
typedef struct GatherPoint {
    f32 x;              /* 0x00 */
    f32 y;
    f32 z;
    f32 radius;         /* 0x0C */
    u16 table;          /* 0x10: the item table (GATHER_TABLE_MASK; bit 15 is a flag) */
    u16 count;          /* 0x12: gathers left, GATHER_UNLIMITED */
    u16 unk14;
    u16 bonus;          /* 0x16: gathers before the point can run out */
} GatherPoint;

/* An item of a gathering table: {weight, item}, ended by weight GATHER_END
 * (Gather_RollItem, Gather_RollMineItem). */
typedef struct GatherEntry {
    u16 weight;
    u16 item;
} GatherEntry;

#define GATHER_TABLE_MASK   0x7FFF
#define GATHER_UNLIMITED    0xFF    /* GatherPoint.count: never runs out */
#define GATHER_END          0xFFFF  /* GatherEntry.weight: the table's end */
#define GATHER_NO_ITEM      0xFFFF  /* Gather_RollItem, Gather_AtPoint: nothing */
#define GATHER_USED_UP      0xFFFE  /* Gather_AtPoint: the point is used up */

/* A place in an area where the player can act (Stage_GetActionSpots;
 * Player_FindActionSpot in gather.c); a list ends with x == -1.0f. */
typedef struct ActionSpot {
    u16 type;           /* 0x00 */
    u16 kind;           /* 0x02: seen 2 and 0xB (the game's unk_70A290.c) */
    f32 x;              /* 0x04 */
    f32 y;
    f32 z;
    f32 radius;         /* 0x10 */
    u16 angle;          /* 0x14: the direction the player takes there (Player_FindActionSpot) */
} ActionSpot;

/* A command of the quest script (gQuest.script, Quest_StepScript), 8 bytes. */
typedef struct QuestCmd {
    s16 op;                     /* QUEST_OP_* */
    s16 arg;
    s16 arg2;
    s16 arg3;
} QuestCmd;

/*
 * QuestCmd.op, the quest script's commands (Quest_StepScript). A command
 * that waits keeps the script at it until the next frame. The script has
 * a setup part, run while the quest is READY, then the play from a start
 * mark (Quest_Start; the first player there tells the others), and its end
 * sections from an end mark (Quest_JumpToEnd).
 */
#define QUEST_OP_START          -1      /* start mark: the quest RUNNING; tells the others (QMSG_STARTED) */
#define QUEST_OP_END            -2      /* end mark: the quest ENDED; tells the others (QMSG_ENDED) */
#define QUEST_OP_CLEAR_TYPE     0x01    /* gQuest.clearType = arg */
#define QUEST_OP_ADD_GOAL       0x02    /* QuestGoal_Add(arg, arg2, arg3) */
#define QUEST_OP_HALT           0x03    /* waits for good (also 0x27) */
#define QUEST_OP_ADD_TIME       0x04    /* adds arg frames to the time left; waits a frame */
#define QUEST_OP_NOP            0x05
#define QUEST_OP_IF_CLEAR       0x06    /* unless the goal or a sub goal is met, skips to after ENDIF */
#define QUEST_OP_ENDIF          0x07    /* (reached in order, it halts) */
#define QUEST_OP_SET_WAIT       0x09    /* gQuest.scriptWait = arg; waits a frame */
#define QUEST_OP_WAIT           0x0A    /* waits until scriptWait counts down to 0 */
#define QUEST_OP_NOTICE         0x0B    /* shows the stage file's notice text arg; waits a frame */
#define QUEST_OP_WAIT_AREA      0x0C    /* waits for the player to be in area arg */
#define QUEST_OP_LOOP           0x17    /* goes back to gQuest.loopPc */
#define QUEST_OP_WAIT_STATE4    0x18    /* waits for the player to be in state 4 */
#define QUEST_OP_LABEL          0x1A    /* label arg: the target of the jumps */
#define QUEST_OP_BEGIN          0x1B    /* the time-out label (arg), the time limit notice and the season banner; waits a frame */
#define QUEST_OP_GOTO           0x1C    /* jumps to label arg */
#define QUEST_OP_CLEARED        0x1E    /* stops the script; gQuest.state goes on to the clear sequence (Quest_Update) */
#define QUEST_OP_FAIL           0x1F    /* stops the script: the quest FAILED, with its notice */
#define QUEST_OP_PROGRESS       0x20    /* gQuest.progress = arg: the next spawn entries and targets */
#define QUEST_OP_WAIT_ITEMS     0x21    /* waits until the local player holds arg3 of item arg2 */
#define QUEST_OP_GOAL_LABEL     0x23    /* gQuest.goalLabel = arg; waits a frame */
#define QUEST_OP_WAIT_HUNTED    0x24    /* waits until at most arg2 of monster kind arg are left to hunt */
#define QUEST_OP_END_ALT        0x25    /* a second end mark (Quest_JumpToEnd(1); QMSG_ENDED_ALT), with the time-up notice */
#define QUEST_OP_START_ALT      0x26    /* a second start mark (Quest_Start, Quest_JumpToStart(1); QMSG_STARTED_ALT) */
#define QUEST_OP_HALT2          0x27
#define QUEST_OP_IF_EM_HURT     0x28    /* unless the last spawned monster lost arg health since the last time, skips to after 0x29 */
#define QUEST_OP_ENDIF_EM_HURT  0x29
#define QUEST_OP_IF_EM_WEAK     0x2A    /* unless a monster is below half health, skips to after 0x2B; else completes goal 0 with flag 0x200 */
#define QUEST_OP_ENDIF_EM_WEAK  0x2B
#define QUEST_OP_EVENT_LABEL    0x2C    /* gQuest.eventLabel = arg; waits a frame */
#define QUEST_OP_WAIT_ALL_START 0x2D    /* waits until every player passed the start mark */
#define QUEST_OP_TIMEOUT_LABEL  0x2F    /* the time-out label (arg) alone; waits a frame */
#define QUEST_OP_IF_HUNTED_GOTO 0x30    /* when at most arg2 of kind arg are left, jumps to label arg3 */
#define QUEST_OP_MARK           0x31    /* the next frame's step starts here again */
#define QUEST_OP_UNMARK         0x32
#define QUEST_OP_VERSUS_END     0x33    /* versus: ends the match once a side has won */
#define QUEST_OP_COMPLETE_GOAL  0x34    /* completes goal 0 (QuestGoal_CompleteIfRunning) */

/*
 * gQuest.msgType: the quest's messages between the players (sent with
 * QSync_SendState(6); Quest_HandleNetMsg applies them). The arguments are
 * msgArg0..2 and msgArea; a 32-bit value goes as msgArg1 (high half) and
 * msgArg2 (low half).
 */
#define QMSG_STARTED            2       /* the sender passed the start mark */
#define QMSG_ENDED              4       /* the sender passed the end mark */
#define QMSG_TIME_LEFT          5       /* the sender's time left (the lower one counts) */
#define QMSG_PROGRESS           6       /* gQuest.progress (the higher one counts) */
#define QMSG_KILL               7       /* a quest monster down: its index and appearances left, the area */
#define QMSG_DELIVERED          9       /* an item delivered: item and count */
#define QMSG_ENDED_ALT          10      /* the second end mark */
#define QMSG_STARTED_ALT        11      /* the second start mark */
#define QMSG_RANK               12      /* the clear sequence: the sender's hunter rank (acks bits 0-3) */
#define QMSG_REWARD_ACK         13      /* the clear sequence: the second acknowledgement (acks bits 4-7) */
#define QMSG_EVENT_OVER         14      /* the timed event is over */
#define QMSG_CLEAR_TIME         15      /* the sender's quest time (the higher one counts) */
#define QMSG_GOAL_DONE          16      /* QuestGoal_Complete(msgArea, the value) */
#define QMSG_VERSUS_END         17      /* versus: the winning side (msgArg1) */
#define QMSG_COUNTDOWN_MSG      18      /* the countdown's message counter */
#define QMSG_COUNTDOWN_START    19      /* the countdown starts */
#define QMSG_VERSUS_DEFEAT      20      /* versus: gQuest.versusDefeat bits */

/* gQuest.acks: a bit per player for each of the three waits of the clear
 * sequence (Quest_Update); a player out of the quest counts as acknowledged. */
#define QUEST_ACKS_RANK         0x00F   /* QMSG_RANK received */
#define QUEST_ACKS_REWARD       0x0F0   /* QMSG_REWARD_ACK received */
#define QUEST_ACKS_STARTED      0xF00   /* the start mark passed (QMSG_STARTED) */
#define QUEST_ACK_REWARD_SHIFT  4
#define QUEST_ACK_STARTED_SHIFT 8

/* gQuest.state, the quest's own sequence (Quest_Update). */
#define QSTATE_PLAY             0       /* the script runs */
#define QSTATE_CLEAR            1       /* the goal met: the return countdown */
#define QSTATE_REWARD           2       /* the reward and the players' acknowledgements */
#define QSTATE_VERSUS_END       3       /* versus: waits for the players, then won or lost */

/* Quest_GetTime's argument. */
#define QUEST_TIME_LEFT     0
#define QUEST_TIME_LIMIT    1
/* frames: 5 minutes, the step of the time-left notices and of the timed
 * events (gQuest.timeNoticeIdx, timedEventSet) */
#define QUEST_TIME_NOTICE_STEP  9000

/* From this quest id the quest was downloaded: its stage file is loaded
 * already (Quest_Init). */
#define QUEST_ID_DOWNLOADED 50000

/* The versus quest's id (Quest_IsVersus: online only; the lobby's arena menu offers it). */
#define QUEST_ID_VERSUS     25001

/*
 * Quests (gQuest.stage) whose first meeting with a monster plays a story
 * scene (em_blangonga.c): in them the monster waits in its scene's area, by
 * day AREA_JUNGLE_3 for the Congalala and AREA_SNOWY_MOUNTAINS_6 for the
 * Blangonga, until the scene's event flag is set, and switches to its scene
 * state while a scene runs there (gScene.eventRunning).
 */
#define QUEST_ID_CONGALALA_SCENE_1      0x3F3
#define QUEST_ID_CONGALALA_SCENE_2      0x3F4
#define QUEST_ID_BLANGONGA_SCENE_1      0x423
#define QUEST_ID_BLANGONGA_SCENE_2      0x424

/* The save's event flags (Save_TestEventFlag) those monsters test: while
 * clear, they wait for their scene. No C code sets them (seen: no
 * Save_SetEventFlag call with these numbers). */
#define EVENT_FLAG_CONGALALA_SCENE      0x9B
#define EVENT_FLAG_BLANGONGA_SCENE      0xA4

/* The event flags the PSP link sets, for the link's bonuses 7, 8 and 9 (the lobby's
 * unk_75E3B0.c sets them; select_psp.c lists the bonuses not yet given). Each lets equipment
 * be forged (item.h's FORGE_COND_PSP_LINK_*). */
#define EVENT_FLAG_PSP_LINK_7           0x7F
#define EVENT_FLAG_PSP_LINK_8           0x82
#define EVENT_FLAG_PSP_LINK_9           0x83

/* Quest_SetStartArea's `who` for the scene's own area (the local player). */
#define START_AREA_SCENE    0xFF

/* The highest count of a monster kind hunted or captured in a quest. */
#define QUEST_COUNT_MAX     9999

/*
 * The quest's reward entries (StageHeader.rewardTable, gQuest.rewardTable;
 * Quest_RollRewards): four u16 each, the condition, then the offset in the
 * stage file of an item list (u16 triples: weight, item, count; weight
 * 0xFFFF ends). A list is drawn from again and again while the draw's
 * chance holds (gRewardRateTables). The ranged conditions take only the
 * first entry of their kind that is met.
 */
#define REWARD_HP_BELOW         0x0B    /* 0x0B..0x0F: the monster's health went from above a threshold to at most it */
#define REWARD_DELIVER_BAND     0x15    /* 0x15..0x19: the delivered count is in band n of gRewardDeliverBands */
#define REWARD_HUNT_BAND        0x1A    /* 0x1A..0x23: the hunted count (of 20) is in band n of gRewardHuntBands */
#define REWARD_DRILL_PAYOUT     0x29    /* 0x29..0x2D: the drill's payout reaches gDrillPayoutThresholds[n] */
#define REWARD_DOJO_PAR         0x2E    /* cleared within the dojo's par time */
#define REWARD_ALL_HUNTED       0x2F    /* no monster left */
#define REWARD_HUNTED_99        0x30    /* 0x30..0x93: at least n of the main goal's 99 hunted */
#define REWARD_DELIVERED        0x94    /* 0x94..0xF7: at least n of the main goal's item delivered */
#define REWARD_HUNTED_20        0xF8    /* 0xF8..0x10C: at least n of the main goal's 20 hunted */
#define REWARD_ALWAYS           0x8000  /* the main rewards (the first draw always gives the list's first item) */
#define REWARD_MAIN_GOAL        0x8001  /* goal 0 done (the first draw likewise) */
#define REWARD_SUB_A            0x8002  /* goal 1 done: sub-quest A's rewards */
#define REWARD_SUB_B            0x8003  /* goal 2 done: sub-quest B's rewards */
#define REWARD_END              0xFFFF

/*
 * After the quest, gScene.supply holds the rewards (Quest_RollRewards): the
 * main ones, sub-quest A's, sub-quest B's, then those of the broken parts
 * and the captures, each group sorted by item.
 */
#define REWARD_SLOT_MAIN        0
#define REWARD_SLOT_SUB_A       24
#define REWARD_SLOT_SUB_B       28
#define REWARD_SLOT_PARTS       32
#define REWARD_SLOT_END         48

/* A reward for a broken part or a capture (gm_sub's gPartRewardRecs, 8 bytes; kind 0xFF ends). */
typedef struct PartReward {
    u8 kind;                    /* the monster kind */
    u8 part;                    /* the part (Quest_IsPartBroken), PART_REWARD_CAPTURE: the capture */
    u16 table[3];               /* by the stage's tier: the gRewardItemTables list, 0xFFFF: none */
} PartReward;

#define PART_REWARD_CAPTURE     0x80

/* gQuest.stats (QuestStats_Add), in a drill: the ids seen. */
#define QSTAT_PART_BROKEN   1       /* a part broken (counted once) */
#define QSTAT_HEAL          2       /* a heal with an item (pouch.c) */
#define QSTAT_FAINT         3       /* a faint */
#define QSTAT_HUNTED_FIRST  4       /* 4 to 9: the hunts by gEmTallyClass 1 to 6 */
#define QSTAT_ALL_GOALS     10      /* every goal done (QuestStats_Tally) */
#define QSTAT_CHARM         11      /* the drill's charm was taken */

/* gQuest.drillCharm. */
#define DRILL_CHARM_NONE    0
#define DRILL_CHARM_TAKEN   1       /* Quest_DrillTakeCharm */
#define DRILL_CHARM_USED    0xFF    /* it took a faint (Quest_PlayerFainted) */

/* Quest_PlayerFainted's result. */
#define FAINT_PENALTY       0       /* the reward lost the penalty */
#define FAINT_FAILED        1       /* the penalty took the last of the reward: the quest failed */
#define FAINT_QUEST_OVER    2       /* no reward was left, or a drill or versus quest ends */
#define FAINT_FREE          3       /* no penalty (the drill's charm, or a free faint) */

/*
 * gScene.questPhase (SceneState, include/scene.h): where the quest is. Set by
 * the quest code (unk_3A1070.c); Quest_IsRunning gives 1 for RUNNING and
 * CLEARED, and -1 (when asked) for ENDED, FAILED and ABORTED.
 */
#define QUEST_PHASE_NONE        0   /* no quest set up (the scene's cleared state); no code sets 1 */
#define QUEST_PHASE_READY       2   /* set up (Quest_Init) */
#define QUEST_PHASE_RUNNING     3   /* started (Quest_Start; the script's restart ops) */
#define QUEST_PHASE_CLEARED     4   /* the goal met (gQuest.result 4) */
#define QUEST_PHASE_ENDED       5   /* ended by the script (its op 0x25, or its end mark -2; result 5) */
#define QUEST_PHASE_FAILED      6   /* failed (script op 0x1F; result 6) */
#define QUEST_PHASE_ABORTED     7   /* abandoned (Quest_Abort) */
#define QUEST_PHASE_QUIT        8   /* left before or without an end (Quest_Finish): straight to the results */

extern QuestState gQuest;
extern StageHeader *gStageHeader;     /* gQuest.header */
extern StageHeader gDefaultStageHeader;

/* A player's start in the stage file (StageHeader.startAreas, one per player). */
typedef struct StartArea {
    s32 area;
    u8 unk04[0x10 - 0x04];
} StartArea;

/* Where a monster can appear in an area (StageHeader.areaSpawns): count points at offset. */
typedef struct AreaSpawns {
    u16 area;                   /* 0xFFFF ends the list */
    u16 count;
    u32 offset;                 /* EmSpawnPoint[count], from gStageFile */
} AreaSpawns;

/* A monster's spawn point in an area (AreaSpawns; QuestEm_PlaceAtRandomPoint). */
typedef struct EmSpawnPoint {
    s32 rotY;
    f32 x;
    f32 y;
    f32 z;
} EmSpawnPoint;

/* A range of monster sizes in percent (gEmSizeRanges, by StageHeader.sizeRange). */
typedef struct EmSizeRange {
    u8 min;
    u8 max;
} EmSizeRange;

/* An effect shown when a monster part breaks (gPartBreakEffects, 0xFF-terminated). */
typedef struct PartBreakEffect {
    u8 kind;                    /* 0x0: Em_KindMatches(type, kind) */
    s8 bone;                    /* 0x1 */
    u16 part;                   /* 0x2 */
    f32 offset[3];              /* 0x4: from the bone */
} PartBreakEffect;

/* A time-left notice (gQuestTimeNotices, Quest_StepScript). */
typedef struct QuestTimeNotice {
    s32 time;                   /* 1/30 s: the notice shows once the time left falls to it */
    s32 unk4;                   /* not read by the code */
} QuestTimeNotice;

/* The number of quest ids gQuestRecordIds lists, and of monster kinds the
 * record counts. */
#define QUEST_RECORD_IDS 0x10
#define QUEST_RECORD_KINDS 0x60

/* The record of the last quest (sub_main 0x5D4920, 0xDC bytes): cleared by
 * QuestRecord_Clear when a quest starts, filled by QuestRecord_Fill for the
 * results; the lobby reads its phase and sends the whole record to the
 * server. */
typedef struct QuestRecord {
    u16 questId;        /* 0x00: gQuest.stage */
    u8 pad02[2];
    u8 phase;           /* 0x04: gScene.questPhase at the end (QUEST_PHASE_*) */
    s8 mapGroup;        /* 0x05: Map_Group of the quest's map */
    s8 flyer;           /* 0x06: gQuest.flyer */
    s8 tier;            /* 0x07: the stage's tier (gQuest.header->tier) */
    s32 elapsed;        /* 0x08: the quest's time (gQuest.elapsed) */
    u8 counts[QUEST_RECORD_IDS]; /* 0x0C: per id of gQuestRecordIds, its clears (QuestRecord_Count) */
    u8 hunted[QUEST_RECORD_KINDS];   /* 0x1C: per monster kind (sub_main's QuestEm_CountHunted) */
    u8 captured[QUEST_RECORD_KINDS]; /* 0x7C: per monster kind (QuestEm_CountCaptured) */
} QuestRecord;

#endif /* QUEST_H */
