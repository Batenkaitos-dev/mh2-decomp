#ifndef GM_EM_H
#define GM_EM_H

#include "common.h"
#include "em.h"
#include "mine_point.h"
#include "quest.h"

/*
 * The monsters in the quest overlay: the monster task (GmEm_Task) and the
 * set-up, update and sizes every kind shares, the shared motions' sounds,
 * and what Em_DrawParts asks before it draws a mesh set (texture scrolls,
 * materials). The tables are gm_sub's data that the game overlay reads: the
 * per-kind monster tables (hit points, hit zones, levels, sizes), the
 * areas' ambient sounds, the gathering, mining and reward lists and the
 * start areas.
 */

/* One row of a monster kind's level table (gEmScaleRows), selected by
 * Em.level. */
typedef struct EmScaleRow {
    f32 hpScale;        /* the base hp's factor (Em_ComputeMaxHp: hpMax) */
    f32 attackFactor;   /* Em.attackFactor's */
    f32 damageFactor;   /* Em.damageFactor's */
} EmScaleRow;

/*
 * A monster kind's area tables (Em.tables): the game's gEmKindTables[type]
 * (em_common.c) holds the record's address as a number. Each record is an
 * array of table pointers, `void *gEmKindTables_<Kind>[7]`:
 * - [0] to [5]: EmAreaPoints lists, one entry per area (Em_AreaEntryValue's
 *   goal argument selects the list; [0] the entry points, [1] the loop
 *   points EmScript reads through Em_FindAreaEntry, [2] the swimmers' area
 *   points);
 * - [6]: the route sets (EmRouteSet; Em_FindAreaEntry), or none.
 * Some kinds' records end after [5] (`[6]`): the word after them is the
 * first entry of their area value list (EmAreaValue).
 */

/* The entries of a per-kind table (indexed by Em.type, EM_KIND_*): kinds 0
 * to 75. */
#define EM_KIND_SLOTS 76

/* A monster kind's breakable parts (gEmPartHpTables[kind]). Em_InitParts and
 * Em_ResetPartHp step an s16 pointer through the row, so the table's entries
 * are s16 pointers. Several kinds can share a row. */
typedef struct EmPartHpRow {
    s16 hp[9];          /* 0x00: each part's hit points (EmPart.hp); 0 or less: the kind has no such part */
    s16 partDamageMode; /* 0x12: Em.partDamageMode, how a hit's damage is taken off the parts (Em_UpdateDamage) */
    s16 unk14[6];       /* 0x14: no reader seen */
} EmPartHpRow;

/* The entries of a gEmBaseHp row: a monster kind's base hp by the index
 * Em_ComputeMaxHp is passed (0 for most kinds; some pass the monster's
 * variant). */
#define EM_HP_VARIANTS 4

void GmEm_MotionSounds(Em *em, EmWork *work);
void GmEm_SetupKind(Em *em);
void GmEm_ResetSize(Em *em);
void GmEm_ApplySize(Em *em);
f32 GmEm_ScaleBySize(Em *em, f32 value);
void GmEm_ResetState(Em *em);
void GmEm_Setup(Em *em);
void GmEm_Update(Em *em);
s32 GmEm_Task(Em *em);
void GmEm_KhezuScrollUv(Em *em, s32 meshSet);
void GmEm_PlesiothScrollUv(Em *em, s32 meshSet);
void GmEm_YamaTsukamiScrollUv(Em *em, s32 meshSet);
s32 GmEm_ChameleosMeshUv(Em *em, s32 meshSet);
void GmEm_FatalisCPulseUv(Em *em, s32 meshSet);
s32 GmEm_SetMeshSetUv(Em *em, s32 meshSet);
void GmEm_PrepareMeshSet(Em *em, s32 set, ModelMatSet *sets);
void GmEm_SpawnBulldromeRageAura(Em *em, s16 motion);
extern s16 *gEmPartHpTables[EM_KIND_SLOTS];
extern EmHitzone **gEmHitzoneTables[EM_KIND_SLOTS];
extern f32 gEmLowHpRatios[EM_KIND_SLOTS];
extern EmScaleRow *gEmScaleRows[EM_KIND_CEANATAUR + 1];
extern s8 *gEmLevelTables[];
extern s16 *gEmBaseHp[EM_KIND_CEANATAUR + 1];
/* By monster note index (MonsterNote_Index): the factor of a versus quest's
 * hp (Em_ComputeMaxHp). */
#define EM_VERSUS_HP_FACTORS 28
extern f32 *gEmVersusHpFactors[EM_VERSUS_HP_FACTORS];
/* By Em.level: the versus hp's factor (Em_ComputeMaxHp). */
#define EM_SIZE_LEVEL_FACTORS 28
extern f32 gEmSizeLevelFactors[EM_SIZE_LEVEL_FACTORS];
extern u8 gEmMotionGroupCounts[80]; /* by kind (EmKind_MotionGroupCount); the object runs to the next one */
extern Vec3f gPlayerResetPos[];
extern s32 gPlayerResetRotY[];
extern s32 gAmbSeIdsSwampNight7[];
extern s16 gAmbSePeriodsSwampNight7[];
extern s16 gAmbSeKindsSwampNight7[];
extern s32 gAmbSeIdsSwampNight9[];
extern s16 gAmbSePeriodsSwampNight9[];
extern s16 gAmbSeKindsSwampNight9[];
extern s32 gAmbSeIdsJungle0[];
extern s16 gAmbSePeriodsJungle0[];
extern s16 gAmbSeKindsJungle0[];
extern s32 gAmbSeIdsJungle1[];
extern s16 gAmbSePeriodsJungle1[];
extern s16 gAmbSeKindsJungle1[];
extern s32 gAmbSeIdsJungle4[];
extern s16 gAmbSePeriodsJungle4[];
extern s16 gAmbSeKindsJungle4[];
extern s32 gAmbSeIdsJungle7[];
extern s16 gAmbSePeriodsJungle7[];
extern s16 gAmbSeKindsJungle7[];
extern s32 gAmbSeIdsJungle8[];
extern s16 gAmbSePeriodsJungle8[];
extern s16 gAmbSeKindsJungle8[];
extern s32 gAmbSeIdsTower0[];
extern s16 gAmbSePeriodsTower0[];
extern s16 gAmbSeKindsTower0[];
extern s32 gAmbSeIdsTower1[];
extern s16 gAmbSePeriodsTower1[];
extern s16 gAmbSeKindsTower1[];
extern s32 gAmbSeIdsVolcano7[];
extern s16 gAmbSePeriodsVolcano7[];
extern s16 gAmbSeKindsVolcano7[];
extern s32 gAmbSeIdsVolcano8[];
extern s16 gAmbSePeriodsVolcano8[];
extern s16 gAmbSeKindsVolcano8[];
/* The gathering item tables of the points placed at run time, by
 * MinePoint.table: each a GatherEntry list ended by weight GATHER_END
 * (Gather_RollMineItem). */
#define GATHER_ITEM_TABLES 380
extern GatherEntry *gGatherItemTables[GATHER_ITEM_TABLES];
extern EmMineEntry *gEmMinePoints[EM_KIND_CEANATAUR + 1];
extern EmMineEntry2 *gEmMinePoints2[EM_KIND_SLOTS];
/* The reward lists, by PartReward.table: each a u16 list of {weight, item,
 * count} triples ended by weight REWARD_END (Quest_RollRewards). */
#define REWARD_ITEM_TABLES 344
extern u16 *gRewardItemTables[REWARD_ITEM_TABLES];
/* The broken parts' and captures' rewards, ended by kind 255
 * (Quest_RollRewards). */
#define PART_REWARD_RECS 115
extern PartReward gPartRewardRecs[PART_REWARD_RECS];
extern u16 **gStartAreaTables[];

/*
 * The default stage's entry records (gm_em.c; gDefaultStageHeader's
 * emSpawns and stageEntry, used with no stage file): a StageEntry whose
 * parts are pointers, which StageEntry_GetData gives as they are when
 * gQuest.stage is 0. Id 0 ends a list.
 */
typedef struct DefaultStageEntry {
    s32 id;             /* 0x0: an area for the monster spawns */
    s32 unk4;
    s32 *ids;           /* 0x8: part 0: the ids (StageEntry.ids) */
    void *records;      /* 0xC: part 1: the records (StageEntry.records: QuestEm spawns) */
} DefaultStageEntry;

#endif
