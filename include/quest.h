#ifndef QUEST_H
#define QUEST_H

#include "common.h"
#include "em.h"
#include "player.h"
#include "mine_point.h"

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
 * QuestGoal.flags (quest.c's QuestGoal_*). The kind bits come from the
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
 * gQuest.info points to it), 80 bytes; also the start of each guild quest
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
    s32 rewardSubA;             /* 0x14: the reward for sub goal A (Quest_DrawTerms: gQuestSheetTexts[QUEST_SHEET_TEXT_SUB_A_POINTS]) */
    s32 rewardSubB;             /* 0x18: the reward for sub goal B (gQuestSheetTexts[QUEST_SHEET_TEXT_SUB_B_POINTS]) */
    s32 timeLimit;              /* 0x1C: frames */
    u32 map;                    /* 0x20: the quest map (Map_Group(map): ElderForecast_DrawPanel, QuestList_DropGroup; gStartAreaTables[map]: Quest_SetStartArea); u16 readers load it with lhu */
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
#define QUEST_FLAG_MUSIC_WAIT       0x2         /* once the goal is met, the music waits 100 frames before
                                                   BGM_QUEST_RUNNING (Bgm_FieldUpdate); one of the bits
                                                   0-2 the quest list shows */
#define QUEST_FLAG_NO_FIGHT_MUSIC   0x200       /* no fight in any area turns on the battle music (Bgm_IsBattle) */
#define QUEST_FLAG_BATTLE_MUSIC     0x400       /* the battle music plays from the area's entry (Bgm_EnterArea);
                                                   Bgm_IsBattle answers no fight */
#define QUEST_FLAG_THEME_MUSIC      0x800       /* BGM_QUEST_THEME plays all through the quest (Bgm_EnterArea) */
#define QUEST_FLAG_FIXED_LEVEL      0x4000      /* the monsters take gQuest.emLevel as it is (QuestEm_RollLevel) */
#define QUEST_FLAG_NO_LEVEL_UP      0x8000      /* a monster that appears again keeps its level (QuestEm_Appear) */
#define QUEST_FLAG_BREEDING         0x10000     /* a downloaded quest's season (Quest_Init): breeding, */
#define QUEST_FLAG_COLD             0x40000     /* cold, or else warm */
#define QUEST_FLAG_DAY              0x80000     /* a downloaded quest's time of day: day, or else night */
#define QUEST_FLAG_DAY_AND_NIGHT    0x100000    /* with QUEST_FLAG_DAY: day and night alike (the elder forecast's habitats panel) */
#define QUEST_FLAG_DRILL            0x200000    /* a drill: the charm (Quest_DrillTakeCharm), Quest_HasDrillFlag */
#define QUEST_FLAG_MUST_ORDER       0x400000    /* Quest_IsMustOrder */
#define QUEST_FLAG_SET_BREEDING     0x800000    /* with QUEST_FLAGS_FIXED_SEASON: the breeding season, */
#define QUEST_FLAG_SET_WARM         0x1000000   /* the warm season, or else the cold one (Quest_Init) */
#define QUEST_FLAG_TRAINING         0x4000000   /* Quest_IsTraining */
#define QUEST_FLAG_ADVICE           0x8000000   /* with TRAINING, Quest_HasAdvice: one more entry in the offline field menu (FieldMenu_Open) */
/* A built-in quest with one of these flags has its season set by the flags
 * above instead of the current one (Quest_Init). */
#define QUEST_FLAGS_FIXED_SEASON    (QUEST_FLAG_TRAINING | QUEST_FLAG_MUST_ORDER | QUEST_FLAG_DRILL)

/* The kinds of a quest's entry requirement (Player_CheckRequirement): each
 * stat has an "at least value" kind and an "at most value" kind. The
 * resistances are Player.resist's. */
#define QUEST_REQ_ATTACK_MIN        0   /* attack, times the weapon class's multiplier */
#define QUEST_REQ_ATTACK_MAX        1
#define QUEST_REQ_DEFENSE_MIN       2
#define QUEST_REQ_DEFENSE_MAX       3
#define QUEST_REQ_FIRE_RES_MIN      4
#define QUEST_REQ_FIRE_RES_MAX      5
#define QUEST_REQ_WATER_RES_MIN     6
#define QUEST_REQ_WATER_RES_MAX     7
#define QUEST_REQ_THUNDER_RES_MIN   8
#define QUEST_REQ_THUNDER_RES_MAX   9
#define QUEST_REQ_ICE_RES_MIN       10
#define QUEST_REQ_ICE_RES_MAX       11
#define QUEST_REQ_NO_ARMOR          12  /* no armour worn */
#define QUEST_REQ_EMPTY_POUCH       13  /* nothing in the pouch */
#define QUEST_REQ_DRAGON_RES_MIN    14
#define QUEST_REQ_DRAGON_RES_MAX    15
#define QUEST_REQ_AMMO_ONLY         16  /* nothing in the pouch but items with ItemData.flags ITEM_FLAG_AMMO or 4 */
#define QUEST_REQ_NO_BARRED_ITEMS   17  /* none of the items 39, ITEM_ID_LOBBED_NO_VERSUS, ITEM_ID_THROWN_NO_VERSUS and 130 in the pouch */

/*
 * The season and the time of day (the boot ELF's Season_Get, Daytime_Get;
 * the season banner names them in this order, field_objects.c). A stage file
 * has six variants, file + daytime * 3 + season (Quest_Init).
 */
#define SEASON_WARM         0
#define SEASON_COLD         1
#define SEASON_BREEDING     2
#define SEASON_COUNT        3   /* the seasons a year has */
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
 * A monster of the quest, 60 bytes, in the stage file's monster lists
 * (gQuestEmSets, and per area and progress gQuestEmAreaSets; a list ends
 * with kind 0). QuestEm_Init numbers them and QuestEm_Spawn creates the
 * monster from it; the game overlay's code for monster kind 56 keeps
 * a copy of its own.
 */
typedef struct QuestEm {
    s16 kind;                   /* 0x00: the monster kind; 0 ends the list */
    s16 variant;                /* 0x02: -> Em.variant; for EM_KIND_TRADER, gQuest.unk15A picks the entries with variant 0 or not (QuestEm_SpawnArea) */
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
 * A side of a versus quest as the lobby sends it (NextQuest.teams, 28
 * bytes; the lobby copies both sides in one 56-byte block). Versus_Init
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
 * The quest to start next (gNextQuest, 0x6ADAD0, 128 bytes): what the
 * lobby, the quest menus and the network set before Quest_Init reads it.
 * The select and plsel overlays clear it whole. gNextQuestId is its
 * field `stage`.
 */
typedef struct NextQuest {
    u32 flags;                  /* 0x00: the quest's QuestInfo.flags (QUEST_FLAG_*: the lobby copies them) */
    NextQuestTeam teams[2];     /* 0x04: versus: the two sides */
    u32 teamItems[2];           /* 0x3C: versus: per side, an item (QuestTeam.item) */
    s16 arenaRecord;            /* 0x44: the arena record the versus quest uses (gSave.arenaEms index; Versus_ConsumeTeamItem, Versus_UpdateRecords); the lobby sets it from the arena role (-1: none) */
    u8 sideIds[4][6];           /* 0x46: versus: a player whose user id (its first 6 bytes) is entry j is on side j + 1 (Players_UpdateSides); the lobby copies them in */
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

/* An entry of gRecordCountEntries: a delivered item and the
 * save's record counter (SaveData.recordCounts) it raises once a delivery
 * goal for it is done (Records_UpdateCounters; plsel's results read the
 * table too). A key of 0 ends the table. Only the type is here: plsel
 * declares the table with its own view. */
typedef struct RecordCountEntry {
    u16 key;                    /* 0x0: the item (QuestState.goals[].target) */
    u16 counter;                /* 0x2: the index into SaveData.recordCounts */
} RecordCountEntry;

/*
 * A stage file's header (gStageHeader = gQuest.header). In the file the u32
 * fields are offsets from the file's base gStageFile; Quest_LoadStage
 * (quest.c) copies most of them into gQuest as pointers and relocates
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
    u32 areaSpawns; /* 0x34: AreaSpawns, ended by area AREA_NONE */
    u32 tables;                 /* 0x38: the gathering tables (s32 offsets; relocated to a pointer on load) */
    u32 dataTable;              /* 0x3C: id and offset pairs (gQuest.dataTable; Stage_FindData) */
    u32 seasonalData;           /* 0x40: by season and daytime (gQuest.seasonalData; Stage_GetSeasonalData) */
    s16 emSize; /* 0x44: the base size of the monsters that have a random size (Em_HasRandomSize): QuestEm.size is it times QuestEm_RollSize's percent, plus the season's step (plsel's gallery uses 100 percent: Galex_RollEmSize) */
    s16 sizeRange;              /* 0x46: the row of gEmSizeRanges the rolled monster sizes stay in (QuestEm_RollSize) */
    u32 emLevel;                /* 0x48: the monsters' starting level (gQuest.emLevel) */
    s32 hrpMain; /* 0x4C: the HR points for the main goal (Plsel_SumQuestRewards) */
    s32 unk50;                  /* 0x50: plsel's reward screen gives it as hrpMain when Quest_GetOutcome(1) is not 1 */
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

/* A downloaded quest's stage file (gQuestFiles holds three): the first word
 * is StageHeader.info, the QuestInfo's offset in the file. */
typedef struct QuestFile {
    s32 info;                   /* 0x000 */
    u8 data[0x2FC];             /* 0x004 */
} QuestFile;
typedef char QuestFile_size_check[(sizeof(QuestFile) == 0x300) ? 1 : -1];

/* An area's place in the map (32 bytes per area index: gQuest.areaRecords, the stage
 * file's StageHeader.areaRecords; Stage_GetAreaRecord). Positions in an area are local:
 * the map position is origin + local (player_stats.c), the area's centre origin + size / 2. */
typedef struct AreaRecord {
    f32 originX;    /* 0x00 */
    f32 originZ;    /* 0x04 */
    f32 unk08[2];   /* 0x08 */
    f32 sizeX;      /* 0x10: positions stay in 8 .. sizeX - 8 (the stage's position clamps) */
    f32 sizeZ;      /* 0x14 */
    f32 bottom;     /* 0x18: the floor's y */
    f32 top;        /* 0x1C */
} AreaRecord;
typedef char AreaRecord_size_check[(sizeof(AreaRecord) == 0x20) ? 1 : -1];

/*
 * gQuest (0x5D8260, 1376 bytes): the running quest's state. Some sub_main
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
    s8 goalLabel;               /* 0x00B: a label (QUEST_OP_LABEL) the script jumps to once the goal has flag 0x100 (quest.c) */
    s32 scriptWait;             /* 0x00C: a countdown the quest script sets (Quest_StepScript: the next step when it reaches 0) */
    s32 timeLeft;               /* 0x010: frames (gQuestTimeLeft) */
    s32 reward;                 /* 0x014: quest money; faints subtract penalty (gQuestReward) */
    s32 penalty;                /* 0x018: the reward lost per faint */
    s16 emLeft;                 /* 0x01C: the monsters still to hunt or capture: QuestEm_Hunted, QuestEm_Captured and Quest_OnRemoteKill decrement it; CamReq_QuestTarget at 0 */
    s16 scriptPc;               /* 0x01E: the quest script's position (Quest_StepScript(pl, pc, out) steps it; Quest_ClearBySubTarget finds the goal-met mark); -1 stopped (Quest_Clear, Quest_Abort, Quest_Finish) */
    s16 loopPc;                 /* 0x020: the script position QUEST_OP_LOOP jumps back to */
    s8 progress;                /* 0x022: the highest target index reached (Quest_CountTarget, Quest_CheckItemTargets); selects the spawn entries (Stage_FindEntryData) */
    s8 mineCount;               /* 0x023: the mining points in use (gMineActive) */
    struct Em *downEm;          /* 0x024: the monster that went down last (Quest_NoteTargetDown, Quest_OnRemoteKill): the camera shows it (CamReq_QuestTarget) */
    u32 flags;                  /* 0x028: quest flags */
    u8 unk02C[0x04C - 0x02C];
    struct StageHeader *header; /* 0x04C: gStageHeader */
    u8 unk050[0x054 - 0x050];
    void *script;               /* 0x054: the quest script, 8-byte ops (StageHeader.script; Quest_ClearBySubTarget, Quest_JumpToEnd, Quest_JumpToGoalMet) */
    void *noticeTexts;          /* 0x058: the stage file's notice text offsets (header +0x30; Notice_PushItem adds gSysSettings.stageFile) */
    void *emSpawns;             /* 0x05C: the monster spawn entries per area and progress (StageHeader.emSpawns; Stage_FindEntryData) */
    void *stageEntry;           /* 0x060: the stage entry records: header unk18 + gStageFile (Quest_Init; StageEntry_GetData, Galex_InitEmSets) */
    void *areaData;             /* 0x064: per area index (Map_AreaIndex), an offset into the stage file (StageHeader.areaData; Stage_GetAreaData) */
    AreaRecord *areaRecords;    /* 0x068: 32 bytes per area index (StageHeader.areaRecords; Stage_GetAreaRecord) */
    void *texts;                /* 0x06C: the quest texts (QuestInfo.texts; QuestInfo_Get) */
    void *rewardTable;          /* 0x070: the stage reward entries (u16 x4 each, REWARD_END ends; Quest_RollRewards) */
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
    s8 emLevel;                 /* 0x136: the monsters' level: StageHeader.emLevel, +1 per appearance up to 15 (QuestEm_Appear, Galex_RollEmLevel) */
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
    u16 traderWantItem;         /* 0x158: the item the trader (EM_KIND_TRADER) asks for in this quest, 0: none yet (EmTrader_RunTalk picks it, clears it when traded) */
    s8 traderVariant;           /* 0x15A: which entries of EM_KIND_TRADER appear: those with QuestEm.variant nonzero, or 0 (rolled from gTraderVariantRolls; 0 in a drill; QuestEm_SpawnArea) */
    u8 unk15B;                  /* 0x15B */
    s8 flyer;                   /* 0x15C: nonzero: the area's flyer object appears (AreaFlyer_Create); from NextQuest.flyer (Quest_Init); kept in the quest record */
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
    u8 drillCharm;              /* 0x1F0: the drill's charm: 0 not taken, 1 taken (Quest_DrillTakeCharm), 255 used up by a faint (Quest_PlayerFainted) */
    u8 unk1F1[0x1F4 - 0x1F1];
    QuestHuntGoal huntGoals[3]; /* 0x1F4: the hunting goals, copied from gNextQuest.huntGoals (Quest_Init); plsel's rewards read them */
    u16 hunted[80];             /* 0x20C: per monster kind, this quest */
    u16 captured[80];           /* 0x2AC */
    u16 sizeMax[80];            /* 0x34C */
    u16 sizeMin[80];            /* 0x3EC */
    u16 parts[80];              /* 0x48C: broken parts, one bit each */
    u32 elapsed;                /* 0x52C: the quest time: Quest_GetTime(1) - Quest_GetTime(0) (Quest_Update, Quest_StoreElapsed, Versus_UpdateRecords) */
    void *dataTable;            /* 0x530: id and offset pairs, 0-terminated (StageHeader.dataTable; Stage_FindData) */
    void *seasonalData;         /* 0x534: per entry two offsets of id/offset tables by season and daytime (StageHeader.seasonalData; Stage_GetSeasonalData) */
    u8 unk538[0x53A - 0x538];
    u16 savedEmParts;           /* 0x53A: its broken parts */
    s16 capturedType;           /* 0x53C: the captured monster's kind (QuestEm_Captured) */
    u16 capturedSize;           /* 0x53E: its size (em->sizePct; plsel's record menu) */
    s8 capturedLevel;           /* 0x540: its level - 3, 1 to 12 (QuestEm_Captured; saved by Plsel_SaveRecordMenu) */
    u8 goalIdx[3];              /* 0x541: the goals in the order they were done, QUEST_GOAL_NONE: none yet (QuestGoal_InitFromDef, QuestGoal_Complete, CamReq_QuestTarget) */
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
    u16 action;         /* 0x14: what the player does there, MINE_ACTION_PICK, _MINE or _BUG_NET (mine_point.h) */
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
    u16 type;           /* 0x00: in the lobby, the spot's id (the seat's index, the Poogie's dig spot) */
    u16 kind;           /* 0x02: seen 2 and 11 in the field; in the town, enum TownSpotKind */
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
 * that waits keeps the script at it until the next frame. The script runs
 * the hunt while the quest is ACTIVE, from its first command; the goal met
 * sends it to its goal-met mark (Quest_JumpToGoalMet; the first player there
 * tells the others), and a failure to its end mark (Quest_JumpToEnd).
 */
#define QUEST_OP_GOAL_MET       -1      /* goal-met mark: the quest GOAL_MET, the special monster's health kept; tells the others (QMSG_GOAL_MET) */
#define QUEST_OP_END            -2      /* end mark: the quest ENDED; tells the others (QMSG_ENDED) */
#define QUEST_OP_CLEAR_TYPE     0x01    /* gQuest.clearType = arg */
#define QUEST_OP_ADD_GOAL       0x02    /* QuestGoal_Add(arg, arg2, arg3) */
#define QUEST_OP_HALT           0x03    /* waits for good (also QUEST_OP_HALT2) */
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
#define QUEST_OP_GOAL_MET_ALT   0x26    /* a second goal-met mark (Quest_ClearBySubTarget, Quest_JumpToGoalMet(1); QMSG_GOAL_MET_ALT) */
#define QUEST_OP_HALT2          0x27
#define QUEST_OP_IF_EM_HURT     0x28    /* unless the last spawned monster lost arg health since the last time, skips to after QUEST_OP_ENDIF_EM_HURT */
#define QUEST_OP_ENDIF_EM_HURT  0x29
#define QUEST_OP_IF_EM_WEAK     0x2A    /* unless a monster is below half health, skips to after QUEST_OP_ENDIF_EM_WEAK; else completes goal 0 with flag 0x200 */
#define QUEST_OP_ENDIF_EM_WEAK  0x2B
#define QUEST_OP_EVENT_LABEL    0x2C    /* gQuest.eventLabel = arg; waits a frame */
#define QUEST_OP_WAIT_ALL_GOAL_MET 0x2D /* waits until every player passed the goal-met mark */
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
#define QMSG_GOAL_MET           2       /* the sender passed the goal-met mark */
#define QMSG_ENDED              4       /* the sender passed the end mark */
#define QMSG_TIME_LEFT          5       /* the sender's time left (the lower one counts) */
#define QMSG_PROGRESS           6       /* gQuest.progress (the higher one counts) */
#define QMSG_KILL               7       /* a quest monster down: its index and appearances left, the area */
#define QMSG_DELIVERED          9       /* an item delivered: item and count */
#define QMSG_ENDED_ALT          10      /* the second end mark */
#define QMSG_GOAL_MET_ALT       11      /* the second goal-met mark */
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
#define QUEST_ACKS_GOAL_MET     0xF00   /* the goal-met mark passed (QMSG_GOAL_MET) */
#define QUEST_ACK_REWARD_SHIFT  4
#define QUEST_ACK_GOAL_MET_SHIFT 8

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

/* The last id of the standard quests (QuestId_IsStandard): the lobby's
 * dojo lists the quests up to it apart from the others. */
#define QUEST_ID_STANDARD_MAX 10000

/* The arena cup's quest, the versus quest: the hunters' teams fight with the
 * monsters they keep (Quest_IsVersus, online only; the lobby's arena cup desk
 * posts it). */
#define QUEST_ID_ARENA_CUP  25001

/* The village's starting quests: offline, each is offered with no village
 * condition (Quest_IsOffered; 1001 is the village's first quest id). They
 * share one advice message set (MsgEvent_Init), and clearing any of them
 * sets event flag 5. */
#define QUEST_ID_STARTER_A  1001
#define QUEST_ID_STARTER_B  1086
#define QUEST_ID_STARTER_C  1087

/* A story quest in the fortress: the offer menu's story offer posts it, it
 * has an advice message set of its own (MsgEvent_Init), and entering the
 * fortress's area 0 in it shows one of them (MSGEV_FORTRESS_0). */
#define QUEST_ID_FORTRESS_STORY 6002

/*
 * Quests (gQuest.stage) whose first meeting with a monster plays a story
 * scene: in them the monster waits in its scene's area, by
 * day AREA_JUNGLE_3 for the Congalala and AREA_SNOWY_MOUNTAINS_6 for the
 * Blangonga, until the scene's event flag is set, and switches to its scene
 * state while a scene runs there (gScene.eventRunning).
 */
#define QUEST_ID_CONGALALA_SCENE_1      0x3F3
#define QUEST_ID_CONGALALA_SCENE_2      0x3F4
#define QUEST_ID_BLANGONGA_SCENE_1      0x423
#define QUEST_ID_BLANGONGA_SCENE_2      0x424

/* The Kirin's scene quests, by the same rule: in them, by day in
 * AREA_SNOWY_MOUNTAINS_8, the Kirin waits while EVENT_FLAG_KIRIN_SCENE is
 * clear. */
#define QUEST_ID_KIRIN_SCENE_1          0x425
#define QUEST_ID_KIRIN_SCENE_2          0x426

/* The other monsters' scene quests: ScEvent_Init starts the monster's
 * sighting scene in them, and the monster waits in its scene's area while
 * its EVENT_FLAG_*_SCENE (below) is clear. Where one monster has scenes in
 * two areas, the area's name is in the quest's. */
#define QUEST_ID_VELOCIDROME_SCENE_1    0x12
#define QUEST_ID_VELOCIDROME_SCENE_2    0x3EA
#define QUEST_ID_VELOCIDROME_SCENE_3    0x3EB
#define QUEST_ID_BULLDROME_SCENE_1      0x3EC
#define QUEST_ID_BULLDROME_SCENE_2      0x3ED
#define QUEST_ID_KUTKU_SCENE_1          0x3EE
#define QUEST_ID_KUTKU_SCENE_2          0x3F0
#define QUEST_ID_RATHIAN_SCENE_1        0x3F6
#define QUEST_ID_RATHIAN_SCENE_2        0x3F8
#define QUEST_ID_DAIMYO_HERMITAUR_SCENE_1 0x400
#define QUEST_ID_DAIMYO_HERMITAUR_SCENE_2 0x1389
#define QUEST_ID_DIABLOS_SCENE          0x406
#define QUEST_ID_KHEZU_SCENE            0x422
#define QUEST_ID_GRAVIOS_SCENE          0x42C
#define QUEST_ID_SHOGUN_CEANATAUR_SCENE_1 0x42E
#define QUEST_ID_SHOGUN_CEANATAUR_SCENE_2 0x42F
#define QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_1 0x430
#define QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE_2 0x431
#define QUEST_ID_RATHALOS_SCENE_1       0x434
#define QUEST_ID_RATHALOS_SCENE_2       0x436
#define QUEST_ID_CHAMELEOS_SCENE_1      0x437
#define QUEST_ID_CHAMELEOS_SCENE_2      0x438
#define QUEST_ID_KUSHALA_JUNGLE_SCENE   0x138A
#define QUEST_ID_KUSHALA_SNOW_SCENE     0x138C
#define QUEST_ID_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE 0x138D
#define QUEST_ID_LUNASTRA_TEOSTRA_TOWER_SCENE 0x138F
#define QUEST_ID_MONOBLOS_SCENE         0x1390
#define QUEST_ID_GARUGA_SCENE           0x1771

/* The save's event flags (Save_TestEventFlag) those monsters test: while
 * clear, they wait for their scene. No C code sets them (seen: no
 * Save_SetEventFlag call with these numbers). */
#define EVENT_FLAG_CONGALALA_SCENE      0x9B
#define EVENT_FLAG_BLANGONGA_SCENE      0xA4

/* Yama Tsukami's tower scene: while clear, a Yama Tsukami in AREA_TOWER_0
 * flies by when the scene's event runs (YAMA_EVENT_TOWER_FLYBY), and the
 * fly-by moves it on to AREA_TOWER_3 once the flag is set
 * (EmYamaTsukami_TowerFlyby). ScEvent_LoadDemoCams clears it
 * with the other monsters' scene flags. */
#define EVENT_FLAG_YAMA_TSUKAMI_TOWER_SCENE 0xAF

/* More monsters' first-meeting scenes, by the same rule: while clear, the
 * monster waits for its scene (its EVENT state) and the area's update starts
 * the scene. ScEvent_LoadDemoCams clears them with
 * EVENT_FLAG_YAMA_TSUKAMI_TOWER_SCENE. */
#define EVENT_FLAG_LAO_SHAN_SCENE       0xAD    /* the Lao-Shan Lung's */
#define EVENT_FLAG_ASHEN_LAO_SHAN_SCENE 0xB5    /* the Ashen Lao-Shan Lung's */
#define EVENT_FLAG_SHEN_GAOREN_SCENE    0xAE    /* in AREA_FORTRESS_1 */
#define EVENT_FLAG_CHAMELEOS_FOREST_SCENE 0xA2  /* in AREA_FOREST_HILLS_9, QUEST_ID_CHAMELEOS_SCENE_1 and QUEST_ID_CHAMELEOS_SCENE_2 */
#define EVENT_FLAG_KIRIN_SCENE          0xAC    /* the Kirin's, in AREA_SNOWY_MOUNTAINS_8 */
#define EVENT_FLAG_VELOCIDROME_SCENE    0x98    /* in AREA_JUNGLE_7 */
#define EVENT_FLAG_BULLDROME_SCENE      0x99    /* in AREA_JUNGLE_5 */
#define EVENT_FLAG_DAIMYO_HERMITAUR_SCENE 0x9A  /* in AREA_DESERT_9 */
#define EVENT_FLAG_RATHIAN_SCENE        0x9C    /* in AREA_JUNGLE_3 */
#define EVENT_FLAG_KUSHALA_JUNGLE_SCENE 0x9D    /* in AREA_JUNGLE_3 */
#define EVENT_FLAG_KUSHALA_SNOW_SCENE   0x9E    /* in AREA_SNOWY_MOUNTAINS_6 */
#define EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_8_SCENE 0x9F
#define EVENT_FLAG_LUNASTRA_TEOSTRA_TOWER_SCENE 0xA0 /* in AREA_TOWER_0x7C */
#define EVENT_FLAG_GARUGA_SCENE         0xA1    /* in AREA_JUNGLE_9 */
#define EVENT_FLAG_LUNASTRA_TEOSTRA_VOLCANO_7_SCENE 0xA3
#define EVENT_FLAG_SHOGUN_CEANATAUR_SCENE 0xA5  /* in AREA_VOLCANO_5 */
#define EVENT_FLAG_KUTKU_SCENE          0xA6    /* in AREA_JUNGLE_9 */
#define EVENT_FLAG_KHEZU_SCENE          0xA7    /* in AREA_SNOWY_MOUNTAINS_3 */
#define EVENT_FLAG_DIABLOS_SCENE        0xA8    /* in AREA_DESERT_2 */
#define EVENT_FLAG_GRAVIOS_SCENE        0xA9    /* in AREA_VOLCANO_6 */
#define EVENT_FLAG_MONOBLOS_SCENE       0xAA    /* in AREA_DESERT_2 */
#define EVENT_FLAG_RATHALOS_SCENE       0xAB    /* in AREA_FOREST_HILLS_3; the Apceros there wait for it too */
#define EVENT_FLAG_FATALIS_CASTLE_SCENE 0xB0    /* EM_KIND_FATALIS in AREA_CASTLE_1 */
#define EVENT_FLAG_FATALIS_BATTLEFIELD_SCENE 0xB1 /* EM_KIND_FATALIS_B in AREA_BATTLEFIELD_1 */
#define EVENT_FLAG_FATALIS_TOWER_SCENE  0xB2    /* EM_KIND_FATALIS_C in AREA_TOWER_0x7C */

/* Set the first time the local player is in the area of a visible Kushala
 * Daora (Player_CheckKushalaSeen); the PSP link's send mask tests it. */
#define EVENT_FLAG_KUSHALA_SEEN         0x81

/* Set once the link's gift is given (PspGift_Give); PspGift_Pick gives
 * none while it is set, and each new link clears it. */
#define EVENT_FLAG_PSP_GIFT_GIVEN       0x31

/* Set once HR gate N is cleared (Save_HrGateCleared(N); Village_UpdateProgress). */
#define EVENT_FLAG_HR_GATE_0            0x5A
#define EVENT_FLAG_HR_GATE_1            0x5B
#define EVENT_FLAG_HR_GATE_2            0x5C
#define EVENT_FLAG_HR_GATE_3            0x5D
#define EVENT_FLAG_HR_GATE_4            0x5E

/* The Poogie's four outfits: PoogieMenu_GetOutfits lists outfit N as
 * owned once its flag is set. Outfit 1's flag is set the first time a
 * shooting gallery round scores GALLERY_BONUS_POINTS (3500) or more
 * (Gallery_Run), with a notice. */
#define EVENT_FLAG_POOGIE_OUTFIT_0      0x74
#define EVENT_FLAG_POOGIE_OUTFIT_1      0x75
#define EVENT_FLAG_POOGIE_OUTFIT_2      0x76
#define EVENT_FLAG_POOGIE_OUTFIT_3      0x77

/* The event flags the PSP link sets for its bonuses, once the MH Portable
 * data shows bonus N (PspLink_HasFlag; PspLink_GrantBonuses sets them,
 * and Psp_SelectFeatures lists the bonuses not yet given). Bonus 6's flag
 * is cleared with the link flags (PspLink_ClearFlags), and without it the
 * lobby counts no linked data. Bonuses 7 to 9 each let equipment be forged
 * (item.h's FORGE_COND_PSP_LINK_*). */
#define EVENT_FLAG_PSP_LINK_0           0x78
#define EVENT_FLAG_PSP_LINK_1           0x79
#define EVENT_FLAG_PSP_LINK_2           0x7A
#define EVENT_FLAG_PSP_LINK_3           0x7B    /* also needed for the link gifts (PspGift_Pick) */
#define EVENT_FLAG_PSP_LINK_4           0x7C
#define EVENT_FLAG_PSP_LINK_5           0x7D
#define EVENT_FLAG_PSP_LINK_6           0x7E
#define EVENT_FLAG_PSP_LINK_7           0x7F
#define EVENT_FLAG_PSP_LINK_8           0x82
#define EVENT_FLAG_PSP_LINK_9           0x83

/* Set by the bed's sleep event in AREA_HUB_0xAC, due once Lao-Shan Lung is
 * hunted and the mine (VILLAGE_FACILITY_MINE) is at level 6
 * (Town_SleepEventPending); the event also puts a blade in the box. Once it
 * is set, the mine's strong pickaxe draws from its own table of finds
 * (Mine_Dig); the mine's model objects test it too. */
#define EVENT_FLAG_MINE_STRONG_PICK     0x89

/* Set by the bed's sleep event in AREA_HUB_0x57, due once condition 81
 * holds (Town_SleepEventPending); the event puts a blade in the box. */
#define EVENT_FLAG_HOUSE_SLEEP_EVENT    0x8A

/* Set the first time the house's owner enters AREA_ONLINE_HOUSE_1 or
 * AREA_ONLINE_HOUSE_2 (Town_AreaEffects), which places the house's prop
 * (LobbyProp_Spawn) that once. */
#define EVENT_FLAG_ONLINE_HOUSE_1_ENTERED 0xB3
#define EVENT_FLAG_ONLINE_HOUSE_2_ENTERED 0xB4

/* Set once the three village quests 10101 to 10103 are cleared
 * (Village_UpdateProgress): the arena cup's desk opens its lists, and some
 * of the town's talk changes. */
#define EVENT_FLAG_ARENA_CUP_OPEN       0xC4

/* Set the first time a hunter in the town requests the song menu's track 3
 * (SongMenu_OnAccepted, which also shows its notice). Online, it opens the
 * quest desk's seventh map list (QuestDesk_CountMapLists) and some of the
 * town's quests and offers test it. */
#define EVENT_FLAG_SONG_TRACK_3         0x122

/* The town's one-time scenes: each is set once its scene or tour has played
 * (LobbyArea_StartEvent plays one while its flag is clear). */
#define EVENT_FLAG_HUB_0x57_SCENE       0x123   /* the first entry to AREA_HUB_0x57 (CAM_REQ_LOBBY_EVENT) */
#define EVENT_FLAG_TOUR_TAVERN_1        0x124   /* TOWN_TOUR_TAVERN_1's */
#define EVENT_FLAG_TOUR_TAVERN_2        0x125
#define EVENT_FLAG_TOUR_SMITHY_1        0x126
#define EVENT_FLAG_TOUR_SMITHY_2        0x127
#define EVENT_FLAG_TOUR_PORT_1          0x128
#define EVENT_FLAG_TOUR_PORT_2          0x129
#define EVENT_FLAG_TOUR_VILLAGE_LEVEL_2 0x12A
#define EVENT_FLAG_TOUR_HUB_0xAA        0x12B
#define EVENT_FLAG_TOUR_HUB_0xAB        0x12C
#define EVENT_FLAG_TOUR_HUB_0xAC        0x12D
#define EVENT_FLAG_TOUR_HOUSE_1         0x13B   /* also Village_HouseTourPending's */
#define EVENT_FLAG_TOUR_HOUSE_2         0x13C
#define EVENT_FLAG_VILLAGE_LEVEL_2      0x13F   /* set once the village's total level reaches 2 */
#define EVENT_FLAG_CONTEST_TOP_PRIZE    0xBF    /* the arm-wrestling contest's top prize was given (Contest_Update); Contest_IsTopPrize gives it once */
#define EVENT_FLAG_TOUR_VILLAGE_RETURN  0x1CE   /* cleared again on going back to the village from AREA_HUB_0x57, and by village milestone 0 */

/* Quest_SetStartArea's `who` for the scene's own area (the local player). */
#define START_AREA_SCENE    0xFF
/* Ends a gStartAreaTables list of (weight, area) pairs (Quest_SetStartArea). */
#define START_AREA_END      0xFFFF

/* The highest count of a monster kind hunted or captured in a quest. */
#define QUEST_COUNT_MAX     9999

/*
 * The quest's reward entries (StageHeader.rewardTable, gQuest.rewardTable;
 * Quest_RollRewards): four u16 each, the condition, then the offset in the
 * stage file of an item list (u16 triples: weight, item, count; weight
 * REWARD_END ends). A list is drawn from again and again while the draw's
 * chance holds (gRewardRateTables). The ranged conditions take only the
 * first entry of their kind that is met.
 */
#define REWARD_HP_BELOW         0x0B    /* 11..15: the monster's health went from above a threshold to at most it */
#define REWARD_DELIVER_BAND     0x15    /* 21..25: the delivered count is in band n of gRewardDeliverBands */
#define REWARD_HUNT_BAND        0x1A    /* 26..35: the hunted count (of 20) is in band n of gRewardHuntBands */
#define REWARD_DRILL_PAYOUT     0x29    /* 41..45: the drill's payout reaches gDrillPayoutThresholds[n] */
#define REWARD_DOJO_PAR         0x2E    /* cleared within the dojo's par time */
#define REWARD_ALL_HUNTED       0x2F    /* no monster left */
#define REWARD_HUNTED_99        0x30    /* 48..147: at least n of the main goal's 99 hunted */
#define REWARD_DELIVERED        0x94    /* 148..247: at least n of the main goal's item delivered */
#define REWARD_HUNTED_20        0xF8    /* 248..268: at least n of the main goal's 20 hunted */
#define REWARD_ALWAYS           0x8000  /* the main rewards (the first draw always gives the list's first item) */
#define REWARD_MAIN_GOAL        0x8001  /* goal 0 done (the first draw likewise) */
#define REWARD_SUB_A            0x8002  /* goal 1 done: sub-quest A's rewards */
#define REWARD_SUB_B            0x8003  /* goal 2 done: sub-quest B's rewards */
#define REWARD_END              0xFFFF
#define REWARD_TABLE_NONE       0xFFFF  /* PartReward.table: no gRewardItemTables list at this tier */

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

/* A reward for a broken part or a capture (gm_sub's gPartRewardRecs, 8 bytes; kind 255 ends). */
typedef struct PartReward {
    u8 kind;                    /* the monster kind */
    u8 part;                    /* the part (Quest_IsPartBroken), PART_REWARD_CAPTURE: the capture */
    u16 table[3];               /* by the stage's tier: the gRewardItemTables list, REWARD_TABLE_NONE: none */
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
 * gScene.questPhase (SceneState): where the quest is, set by the quest code.
 * Quest_GetOutcome gives 1 once the goal is met (GOAL_MET and CLEARED), and
 * -1 (when asked) once it is lost (ENDED, FAILED and ABORTED).
 *
 * The hunt itself is ACTIVE: the quest clock counts down only in that
 * phase (Quest_StepScript). The goal met moves it to GOAL_MET: the clock
 * stops, the return countdown runs (Quest_Update's QSTATE_CLEAR), the
 * hunters can still act, and several monsters leave their area (their
 * code tests Quest_GetOutcome(0) == 1). Once the
 * reward is agreed it is CLEARED, and each hunter plays the clear pose
 * before the results. ENDED and FAILED send the hunters to the quest's
 * end at once.
 */
#define QUEST_PHASE_NONE        0   /* no quest set up (the scene's cleared state); no code sets 1 */
#define QUEST_PHASE_ACTIVE      2   /* set up (Quest_Init) and under way: the hunt, with the clock running */
#define QUEST_PHASE_GOAL_MET    3   /* the goal met (Quest_CheckClear), or the sub-target chosen at the tent (Quest_ClearBySubTarget): the script's goal-met section and the return countdown */
#define QUEST_PHASE_CLEARED     4   /* the countdown over and the reward agreed (Quest_Update; gQuest.result 4) */
#define QUEST_PHASE_ENDED       5   /* the script's end section: a goal failed, the reward lost to faints (Quest_JumpToEnd), or the time up (QUEST_OP_END_ALT); result 5 */
#define QUEST_PHASE_FAILED      6   /* failed (QUEST_OP_FAIL; result 6) */
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
    u16 area;                   /* AREA_NONE ends the list */
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

/* An effect shown when a monster part breaks (gPartBreakEffects, 255-terminated). */
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

/* The record of the last quest (gQuestRecord, 0x5D4920, 220 bytes): cleared by
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

void Quest_Clear(void);
void Quest_Init(void);
void Quest_Abort(void);
void Quest_ClearBySubTarget(void);
void Quest_Finish(void);
void Quest_FinishIfIdle(void);
void Quest_SetStartArea(u16 who);
void Quest_ResetMusicIfIdle(void);
s32 Quest_GetOutcome(s32 withLoss);
s32 Quest_GetTime(s32 which);
void Quest_EnterArea(s32 area);
s32 QuestEm_PlaceAtRandomPoint(Em *em);
s32 QuestEm_CanAppear(Em *em);
s8 QuestEm_Appear(Em *em);
void Em_RunAppearHook(Em *em);
void Quest_NoteTargetDown(Em *em);
void QuestEm_Hunted(Em *em);
void QuestEm_Leave(Em *em);
void QuestEm_Captured(Em *em);
void Em_BreakPart(Em *em, s32 part);
void Em_BreakParts(Em *em, u16 bits);
s32 Quest_IsPartBroken(u16 type, u16 part);
void Em_ClearPart(Em *em, u16 part);
s32 Quest_PlayerFainted(Player *pl, s32 noPenalty);
s32 Quest_DrillTakeCharm(void);
s32 Quest_DrillHasCharm(void);
s32 Quest_Update(void);
void Quest_SeedRandom(void);
u16 Quest_Random(void);
u8 Quest_GetSeedByte(u8 i);
void Quest_UpdateTimedEvent(void);
void Quest_CountEventKill(u16 type);
void Quest_TargetMessage(void);
void Quest_CheckItemTargets(void);
void Quest_CountTarget(u16 type);
void Quest_ClearTargets(void);
void Quest_LoadTargets(void);
Em *QuestEm_Spawn(QuestEm *qem);
void QuestEm_LeaveArea(s32 fromArea, s32 toArea);
void QuestEm_SpawnArea(s32 area);
void *QuestInfo_Get(s32 i);
s32 Quest_GetStageEmKinds(u8 *kinds);
u16 Quest_AddReward(u16 item, s16 count);
void Quest_DeductReward(Player *pl, u16 item, s16 n);
void Quest_GiveRewards(Player *pl);
u16 Quest_Deliver(Player *pl, u16 item, s32 count);
u8 Quest_SetDelivered(Player *pl, u16 item, s32 count);
s16 Quest_CountDelivered(u16 item, s32 who);
void Quest_DeliveryMessage(u16 item);
void Quest_DeliverFromPouch(Player *pl, s32 mode);
s32 Quest_GetDeliveryTarget(Player *unused, s32 idx, ItemStack *out);
u16 Quest_DeliverItem(Player *pl, u16 item, s16 count);
s32 QuestGoal_DeliveryDone(void);
u16 Gather_RollMineItem(Player *pl, u16 tableIdx);
void MinePoint_ResetAll(void);
void MinePoint_UpdateAll(void);
s32 MinePoint_Count(s32 slot);
void MinePoint_Remove(s32 slot);
void MinePoint_SetPos(s32 slot, f32 *pos);
void MinePoint_SetArea(s32 slot, u16 area);
u16 MinePoint_FindNearPlayer(void *obj);
u16 Gather_AtMinePoint(Player *pl);
void MinePoint_RemoveOldest(void);
void MineActive_RemoveAt(s8 idx);
void MineActive_Remove(s8 slot);
void MinePoint_RemoveTemporary(void);
void MineActive_Append(s8 slot);
void MinePoint_PrepareStub(MinePoint *point);
s8 EmMine_Create(Em *em, s32 idx);
s8 EmMine_CreateTimed(Em *em, f32 *pos, s32 idx);
s8 EmMine_CreateTimed0(Em *em, f32 *pos);
void EmMine_Drop(Em *em, f32 *pos);
void EmMine_RemoveTimed(Em *em);
void EmMine_UpdateTimed(Em *em);
s32 EmHolder_CreateMine(EmHolder *holder, s32 unused, f32 *pos);
s32 EmMine_Update(Em *em);
void EmMine_Remove(Em *em);
void SpecialMine_Clear(void);
s32 SpecialMine_IsFree(Player *unused0);
s32 SpecialMine_Create(Player *pl);
s32 SpecialMine_Gather(Player *pl);
s32 SpecialMine_Rank(void);
void QuestStats_Init(void);
void QuestStats_Add(s32 id, s32 n);
void QuestStats_Tally(void);
void Quest_ShowValue(void);
void Quest_Sprintf(char *buf, char *fmt, ...);
void QuestEm_ResetArea(s32 area);
void QuestEm_InitAll(void);
u16 QuestEm_RollSize(void);
void QuestEm_RollLevel(QuestEm *qem);
void QuestEm_Init(QuestEm *qem, s32 *n);
s32 Em_HasRandomSize(s16 kind);
s32 Em_HasRandomSize2(s16 kind);
void QuestEm_InitSets(u8 *set, s32 *n);
void QuestEm_InitAreaSets(u8 **list, s32 *n);
void QuestEm_SpawnAll(void);
struct QuestEm *QuestEm_FindForEm(struct Em *em);
QuestEm *QuestEm_FindByIndex(s16 idx, u16 area);
void Quest_UpdateTimers(s32 ticks);
void Quest_ResetTimer(void);
void QuestGoal_ClearAll(void);
void QuestGoal_InitFromDef(void);
void QuestGoal_Add(s32 flags, s16 target, s16 count);
void QuestGoal_Complete(u16 i, u32 how);
void QuestGoal_CompleteIfRunning(u16 i, s32 how);
s32 Quest_CheckClear(s32 notify);
s32 Quest_CheckSubClear(void);
s32 Quest_CheckSubClearMode(void);
s32 QuestGoal_IsDone(s32 i);
s32 QuestGoal_IsUsed(s32 i);
s32 QuestGoal_AllSubDone(void);
s32 QuestGoal_IsClearTarget(u16 target);
s32 QuestGoal_DeliverCount(u16 i);
s32 QuestGoal_DeliverCountFor(u16 target, u16 i);
u16 QuestGoal_DeliverItem(u16 i);
s32 QuestGoal_InitialFlags(u16 i, s32 mask);
void QuestGoal_Update(void);
s32 QuestGoal_OnCapture(u16 target);
s32 QuestGoal_OnPartBroken(u16 kind, u16 part);
s32 QuestGoal_OnKill(u16 target);
s32 QuestGoal_OnKillSlot(u16 target, u16 i);
s16 Quest_StepScript(Player *pl, s16 cmdIndex, s32 *out);
void Quest_RollRewards(void);
void Quest_JumpToEnd(s32 alt);
s32 QuestEm_RollYamaTsukamiB(s32 area);
void QuestEm_SpawnYamaTsukamiB(void);
void Quest_HandleNetMsg(void);
void Quest_OnRemoteKill(void);
void Quest_SendTime(void);
void Quest_SendKill(QuestEm *qem, Player *monster);
void QuestEm_CountHunted(u16 kind);
void QuestEm_CountCaptured(u16 kind);
void Quest_StoreElapsed(void);
extern u16 gEmAppearMax[76];
extern u16 gDefaultStageHeader_3[8];
extern s16 gStageFileTbls_0[24];
extern s16 gStageFileTbls_1[12];
extern s16 gStageFileTbls_2[6];
extern s16 gStageFileTbls_3[88];

s32 Quest_IsVersus(void);
s8 Versus_Team(s8 side);
void Versus_MonsterDown(Em *em);
void Versus_ApplyTeamBits(QuestTeam *team, u32 bits, u16 kind);
void Versus_Init(void);
void Versus_ConsumeTeamItem(void);
void Versus_UpdateRecords(s32 lost);
s32 Versus_AddGauge(Em *em, u8 useKind);
s32 Versus_TeamHasFlag(Em *em, s32 bit);
void Versus_AssignEmTeam(Em *em);
Em *Em_FindOnSide(u8 side);
void Versus_End(s8 side);
void Versus_InitEm(Em *em);
void Versus_RouteEmByTeam(Em *em);
s32 Versus_DefeatCamera(Em *em);
void Versus_MarkDefeat(Em *em);
s32 Versus_BothDefeated(void);
s32 Quest_KeepsRecords(void);
s32 Quest_IsTraining(void);
s32 Quest_HasAdvice(void);
s32 Quest_HasDrillFlag(void);
s32 Quest_IsMustOrder(void);
void Records_UpdateMonsterFlags(void);
void Records_CountQuest(void);
void Records_CountFlagged(void);
void Records_AddQuestKills(void);
void Records_UpdateCounters(s32 failed);
void Quest_UpdateSaveRecords(s32 failed);
void Save_PickBestCounter(EquipItem *piece, u8 gender);
s32 Save_AddCounterPoints(void);
s32 Item_UsableOutsideCamp(void);
void Quest_StartCountdown(Player *pl);
void Quest_UpdateCountdown(void);
void Quest_TriggerEvent(Em *em);
u8 Quest_FindSpecialMonster(u8 emKind);
void QuestEm_LoadPersistentHp(Em *em);
void QuestEm_SavePersistentHp(void);

QuestInfo *Quest_GetDiscInfo(u16 id);
QuestInfo *Quest_GetInfoAny(u16 id);
QuestInfo *Quest_FindDownloaded(u16 id);
QuestInfo *Quest_CurrentInfo(void);
s8 Quest_DownloadedSlot(u16 id);
char *Quest_GetText(u16 id, s32 which);
char *Quest_SelectedText(s32 which);
QuestInfo *Quest_GetInfo(u16 id);
u8 Quest_GetStars(u16 id);

s32 LobbyConn_Run(void);
void LobbyConn_Next(void);
void LobbyConn_Fail(void);
s32 LobbyNet_DisconnectStep(void);
s32 LobbyNet_AbortSocket(void);

#endif /* QUEST_H */
