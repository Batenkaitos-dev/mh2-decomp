#ifndef FISHING_H
#define FISHING_H

#include "common.h"

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
typedef char FishSpot_size_check[(sizeof(FishSpot) == 0x18) ? 1 : -1];

#endif /* FISHING_H */
