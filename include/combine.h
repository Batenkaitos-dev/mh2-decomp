#ifndef COMBINE_H
#define COMBINE_H

#include "common.h"

/*
 * A combination recipe (8 bytes; sub_main combine.c and unk_331A40.c). gRecipes
 * (combine.c) holds the two-item recipes, listed per first ingredient by gRecipeIndex;
 * gRecipesEx (unk_331A40.c) the three-item ones (flags 0x80), listed by gRecipe3First
 * (by the smallest item) and gRecipe3Second (by the middle one).
 */
typedef struct Recipe {
    s16 item;           /* 0x00: the other ingredient (the largest of three, in gRecipesEx) */
    s16 result;         /* 0x02: the item made */
    s8 rateClass;       /* 0x04: base success rate, gRecipeRates (gRecipeRateLabels) */
    u8 yieldClass;      /* 0x05: the quantity made: gRecipeYieldBase, gRecipeYieldChances, gRecipeYieldLabels */
    u8 listBit;         /* 0x06: its bit in gSave.recipeBits, and its number (gRecipesEx's from 0xBD: Combine_FindRecipeIndex) */
    u8 flags;           /* 0x07: 1: usable only with skill 0xAB or item 6 in the pouch (Recipe_CanUse); 2: always succeeds with skill 0x48 (Combine_RecipeRate); 0x80: three items */
} Recipe;
typedef char Recipe_size_check[(sizeof(Recipe) == 8) ? 1 : -1];

/* Recipe.flags */
#define RECIPE_NEEDS_SKILL      0x01    /* usable only with SKILL_RECIPE_FLAG1 or item 6 in the pouch */
#define RECIPE_SURE_WITH_SKILL  0x02    /* always succeeds with SKILL_COMBINE_SURE */
#define RECIPE_THREE_ITEMS      0x80    /* a recipe of gRecipesEx */

extern Recipe gRecipes[];       /* 0xBD recipes of two items (and an end record) */
extern Recipe gRecipesEx[];     /* 0x30 recipes of three items */

#endif /* COMBINE_H */
