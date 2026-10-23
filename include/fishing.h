#ifndef FISHING_H
#define FISHING_H

#include "common.h"
#include "fxobj.h"

/*
 * The fishing's vocabulary: the steps of a fish and of a lure (FxObj.step of
 * effect objects of those kinds), how a reel ends (LureWork.mode), and the
 * lure's type and flag. FxObj, FishWork and LureWork are in fxobj.h.
 */

/* A fish's FxObj.step (Fish_Update). */
#define FISH_STEP_SPAWN     0   /* picks its kind and grows to its size near home */
#define FISH_STEP_SWIM      1   /* swims around home, looking for a lure */
#define FISH_STEP_BITE      2   /* goes for a lure: nibbles, then bites */
#define FISH_STEP_HOOKED    3   /* struck: follows the lure out */
#define FISH_STEP_WAIT      4   /* the lure is gone: waits at home, then spawns again */

/* A lure's FxObj.step (Lure_Update). */
#define LURE_STEP_FLY       0   /* flies from the cast until it lands on the water */
#define LURE_STEP_LANDED    1   /* on the water, until the player's PL_MOT_FISH_WAIT */
#define LURE_STEP_BITE      2   /* waiting: a fish's bites pull it under */
#define LURE_STEP_REEL      3   /* reeled in (PL_MOT_FISH_REEL), kept within 100 of its target */
#define LURE_STEP_PULL_OUT  4   /* pulled out of the water toward the player (Lure_Cast) */
#define LURE_STEP_IN_HAND   5   /* held at the player's hand until the motion ends */

/* LureWork.mode: how the reel ended, by the player's motion. */
#define LURE_MODE_CATCH     0   /* PL_MOT_FISH_CATCH */
#define LURE_MODE_EMPTY     1   /* PL_MOT_FISH_REEL_EMPTY */
#define LURE_MODE_PULLED    2   /* PL_MOT_FISH_PULLED_DOWN */

/* FxObj.type of a lure: 1 for ITEM_ID_FISH_BAIT_LURE_ALT's (its own model), else 0. */
#define LURE_TYPE_ALT       1

/* FxObj.ownerIndex's high bit on a lure: set while a bite pulls it under. */
#define LURE_PULLED_UNDER   0x80

/* A fishing spot of the stage data (Stage_FindData(area); 0x18 bytes, a negative kind ends the
 * list): Effect17_Spawn starts Stage_GetSeasonalData(kind)'s count of fish there. */
typedef struct FishSpot {
    f32 circle[4];              /* 0x00: the fish's home x, y, z and its radius (Effect17_Spawn) */
    s32 kind;                   /* 0x10: the entry of Stage_GetSeasonalData: the fish table and count */
    s32 unk14;                  /* 0x14: not read by the fishing */
} FishSpot;

/*
 * The town's built-in stage data (gScene.inLobby): the stage file's lists,
 * with pointers where the file holds offsets (stage_data.c).
 * - LobbyFishArea: an entry of gLobbyStageData's list for Stage_FindData:
 *   an area id and its fishing spots (FishSpot list); id 0 ends the list.
 * - LobbyFishTable: an entry of a seasonal table (gLobbySeasonalData_N) for
 *   Stage_GetSeasonalData: the fish table it returns and the count of fish
 *   it stores in *out.
 * - LobbyStageData (gLobbyStageData): the area list, then the fish table
 *   the seasonal tables point to.
 */
typedef struct LobbyFishArea {
    s32 area;
    FishSpot *spots;
} LobbyFishArea;
typedef struct LobbyFishTable {
    FishTableEntry *table;
    s32 count;
} LobbyFishTable;
typedef struct LobbyStageData {
    LobbyFishArea areas[3];
    FishTableEntry fish[8];
} LobbyStageData;
typedef char FishSpot_size_check[(sizeof(FishSpot) == 0x18) ? 1 : -1];

void Fish_Run(FxObj *fish);
void Fish_Start(FxObj *fish);
void Fish_StepSpawn(FxObj *fish, FishWork *w);
void Fish_StepSwim(FxObj *fish, FishWork *w);
void Fish_StepBite(FxObj *fish, FishWork *w);
void Fish_StepHooked(FxObj *fish, FishWork *w);
void Fish_StepWait(FxObj *fish, FishWork *w);
void Fish_Update(FxObj *fish);
void Fish_End(FxObj *fish);
void Fish_Free(FxObj *fish);
void Fish_Draw(DrawSlot *slot);
s8 Fish_PickKind(FxObj *fish);
void Effect17_Spawn(s32 spotKind, f32 *src, FishTableEntry *table);
void Effect17_SpawnArea(s32 area);
f32 Stage_GetDataPoint(s32 area, f32 *pos);
void Lure_Spawn(Player *pl, u8 lureType);
void Lure_Run(FxObj *lure);
void Lure_Start(FxObj *lure);
void Lure_Update(FxObj *lure);
void Lure_End(FxObj *lure);
void Lure_Free(FxObj *lure);
void Lure_Draw(DrawSlot *slot);
f32 Lure_GetWaterHeight(FxObj *lure);
void Lure_Cast(FxObj *lure, LureWork *w);
void Fish_StepState_Unk(FxObj *lure, LureWork *w);
void Player_GetRodTip(f32 *out, Player *pl);
void Fishing_PlaySe(FxObj *lure, f32 *pos, s16 sound);
void CountdownFx_Update(FxObj *countdown);
void CountdownFx_Start(FxObj *countdown);
void CountdownFx_Run(FxObj *countdown);
void CountdownFx_End(FxObj *countdown);
void CountdownFx_Free(FxObj *countdown);
void CountdownFx_Draw(DrawSlot *slot);
void Effect2C_Spawn(s16 frames);
void Countdown_Draw(s16 framesLeft);
void Countdown_DrawDigit(u8 digit, u32 color, f32 scale);
void Letterbox_Draw(s8 size);
extern char *gPlselTexts[168];
extern char *gResultRuleText;
extern char *gWtdClassCText;
extern s32 gHighClassBonusText;
extern char *gHrpNotMetText;
extern char *gHrpNoLossText;
extern char *gHrMaxText;
extern char *gHrUpText;
extern char *gDeathPenaltyText;
extern char *gHelpTexts_6[24];

#endif /* FISHING_H */
