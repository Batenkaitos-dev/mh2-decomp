#ifndef COMBINE_H
#define COMBINE_H

#include "common.h"
#include "player.h"

/*
 * A combination recipe (8 bytes). gRecipes holds the two-item recipes, listed per first
 * ingredient by gRecipeIndex; gRecipesEx the three-item ones (RECIPE_THREE_ITEMS), listed by
 * gRecipe3First (by the smallest item) and gRecipe3Second (by the middle one).
 */
typedef struct Recipe {
    s16 item;           /* 0x00: the other ingredient (the largest of three, in gRecipesEx) */
    s16 result;         /* 0x02: the item made */
    s8 rateClass;       /* 0x04: base success rate, gRecipeRates (gRecipeRateLabels) */
    u8 yieldClass;      /* 0x05: the quantity made: gRecipeYieldBase, gRecipeYieldChances, gRecipeYieldLabels */
    u8 listBit;         /* 0x06: its bit in gSave.recipeBits, and its number (gRecipesEx's from RECIPE_COUNT: Combine_FindRecipeIndex) */
    u8 flags;           /* 0x07: RECIPE_* below */
} Recipe;
typedef char Recipe_size_check[(sizeof(Recipe) == 8) ? 1 : -1];

/* Recipe.flags */
#define RECIPE_NEEDS_SKILL      0x01    /* usable only with SKILL_RECIPE_FLAG1 or ITEM_ID_RECIPE_UNLOCK in the pouch */
#define RECIPE_SURE_WITH_SKILL  0x02    /* always succeeds with SKILL_COMBINE_SURE */
#define RECIPE_THREE_ITEMS      0x80    /* a recipe of gRecipesEx */

#define RECIPE_COUNT        0xBD    /* gRecipes: the two-item recipes (and an end record) */
#define RECIPE_EX_COUNT     0x30    /* gRecipesEx: the three-item recipes; their listBit follows
                                       the two-item ones' */
#define RECIPE_TOTAL        (RECIPE_COUNT + RECIPE_EX_COUNT) /* the bits of gSave.recipeBits */

s32 Combine_ItemHasRecipe(s16 item);
Recipe *Combine_FindRecipe(s16 itemA, s16 itemB);
s16 Combine_Do(u8 *count, Player *player, s16 itemA, s16 itemB, s32 mode);
s32 Combine_HasFixedYield(Player *player, u16 item);
s32 Combine_RecipeRate(Recipe *recipe, s32 mode);
s32 Combine_GetRate(s16 itemA, s16 itemB, s32 mode);
u8 Recipe_YieldBase(Recipe *recipe);
u8 Recipe_YieldAmount(Recipe *recipe);
void *Recipe_YieldLabel(Recipe *recipe);
s32 Combine_IsKnown(s16 itemA, s16 itemB);
s32 Combine_ItemUsable(s16 item);
s32 Combine_CanUsePair(s16 itemA, s16 itemB);
s16 Combine_Do3(u8 *count, Player *player, s32 itemA, s32 itemB, s32 itemC, s32 mode);
s32 Combine_RecipeRate3(s32 itemA, s32 itemB, s32 itemC, s32 mode);
s32 Combine_IsKnown3(s32 itemA, s32 itemB, s32 itemC);
s32 Combine_KnowsMostRecipes(void);
s32 Recipe_CheckAllKnown(void);
s32 Recipe_IsKnown(Recipe *recipe);
void Recipe_SetKnown(Recipe *recipe, s16 *unused);
s32 Combine_ItemHasRecipeIn(s16 item, u8 flag);
s32 Combine_FindRecipeIndex(Recipe *recipe, s16 *items);
s32 Recipe_IsListedFor(s16 item, s32 flags);
s32 Recipe_CanUse(Player *player, Recipe *recipe);
s32 Item_IsBait(u16 item, s32 unused1);
s32 Recipe_MatchesFlags(Recipe *recipe, s32 flags);
extern Recipe gRecipes[];
extern Recipe gRecipesEx[];

/* An entry of gRecipe3Second: the three-item recipes with this middle item, a run of
 * gRecipesEx. gRecipe3First holds per smallest item {count, first} of a run of these
 * entries, as gRecipeIndex does for the two-item recipes. */
typedef struct RecipeRun {
    s16 item;           /* 0x0: the middle ingredient; 0 ends the table */
    s8 count;           /* 0x2: the recipes in the run */
    u8 start;           /* 0x3: the first one's index in gRecipesEx */
} RecipeRun;

extern u8 gRecipe3First[];
extern RecipeRun gRecipe3Second[];

/* The filter of the recipe lists (Recipe_MatchesFlags, Combine_ListRecipesFor,
 * Recipe_IsListedFor; the menus' combination lists). */
#define RECIPE_LIST_UNKNOWN         0x01    /* the unknown recipes too (else only Recipe_IsKnown's) */
#define RECIPE_LIST_NO_SKILL        0x02    /* without the RECIPE_NEEDS_SKILL ones, instead of
                                               testing whether the player can use them */
#define RECIPE_LIST_USABLE_OR_NOT   0x04    /* whether the player can use them or not */

#endif /* COMBINE_H */
