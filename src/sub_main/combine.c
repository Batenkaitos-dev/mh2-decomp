/*
 * Item combination: recipe lookup, the success rate and the
 * combination itself, which draws from random stream 1 twice (success,
 * then the quantity made).
 *
 * Recipes are 8-byte records (gRecipes, include/combine.h), listed per first
 * ingredient (gRecipeIndex: two bytes per item id, the smaller of the two:
 * the count and the first record). The three-item recipes (gRecipesEx) have
 * their own functions (Combine_*3) and a two-level index: gRecipe3First by
 * the smallest item, then gRecipe3Second's runs by the middle one.
 *
 * Then the combination list: which recipes the player knows (gSave.recipeBits,
 * Recipe_IsKnown, Recipe_SetKnown, AWARD_RECIPES) and the menus' recipe lists
 * for an item (Combine_ListRecipesFor, Recipe_MatchesFlags with RECIPE_LIST_*).
 * Two helpers of the item menus close the code: Sort_PairsDesc, a shell sort
 * of (key, value) pairs, and Item_IsBait.
 */
#include "common.h"
#include "scene.h"
#include "player.h"
#include "item.h"
#include "save.h"
#include "combine.h"
#include "award.h"
#include "hud.h"
#include "area_draw.h"
#include "rand.h"


extern u8 gRecipeIndex[];       /* per item: count, first recipe */
/* The two bytes of an item's gRecipeIndex entry. Read as bytes: the game
 * computes the entry's address as item + item, which a struct index would
 * not give. */
#define RECIPE_INDEX_COUNT  0
#define RECIPE_INDEX_FIRST  1
extern u8 gRecipeRates[];       /* base success rate (percent) by class */
extern u8 gCombineBookBonus[];         /* rate bonus by combination books owned */
#define COMBINE_BOOK_COUNT 5
extern u16 gCombineBookItems[];  /* the combination books (items), COMBINE_BOOK_COUNT of them */
extern u8 gRecipeYieldBase[];         /* quantity made, by yield class */
extern u8 *gRecipeYieldChances[];        /* quantity: base, then chances of one more */

u8 Player_GetSkill(Player *p, u8 group);
u8 Pouch_Add(Player *p, u16 item, s16 n);
s32 SavePouch_Count(u16 item);
u8 SavePouch_Add(u16 item, s16 n);

Recipe *Combine_FindRecipe3(s32, s32, s32); /* unit's own declaration: the K&R definition's narrow parameters arrive promoted, as this prototype says (a narrow prototype is incompatible with it) */
s32 Save_HasTitle(s32 n);
s32 Award_Grant(s32 n);

/* 1 if some recipe that uses item can be made now. */
s32 Combine_ItemHasRecipe(s16 item)
{
    u8 *index = &gRecipeIndex[item + item];
    Recipe *recipe;
    s32 count;
    s32 i;

    count = index[RECIPE_INDEX_COUNT];
    if (count != 0) {
        recipe = &gRecipes[index[RECIPE_INDEX_FIRST]];
        for (; count > 0; count--, recipe++) {
            if (Recipe_CanUse(&gPlayers[gScene.localPlayer], recipe) == 1) {
                return 1;
            }
        }
    }
    for (i = 0; i < item; i++) {
        index = &gRecipeIndex[i + i];
        count = index[RECIPE_INDEX_COUNT];
        if (count != 0) {
            recipe = &gRecipes[index[RECIPE_INDEX_FIRST]];
            for (; count > 0; count--, recipe++) {
                if (recipe->item == item && Recipe_CanUse(&gPlayers[gScene.localPlayer], recipe) == 1) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* The recipe for items itemA and itemB (in any order), or NULL, also when
 * the local player can't use it. */
Recipe *Combine_FindRecipe(s16 itemA, s16 itemB)
{
    Recipe *recipe;
    u8 *index;
    u32 count;
    s16 swap;

    if (itemA > itemB) {
        swap = itemB;
        itemB = itemA;
        itemA = swap;
    }
    index = &gRecipeIndex[itemA + itemA];
    count = index[RECIPE_INDEX_COUNT];
    if (count == 0) {
        return NULL;
    }
    recipe = &gRecipes[index[RECIPE_INDEX_FIRST]];
    for (; count != 0; count--, recipe++) {
        if (recipe->item == itemB) {
            if (Recipe_CanUse(&gPlayers[gScene.localPlayer], recipe) == 1) {
                return recipe;
            }
            break;
        }
    }
    return NULL;
}

/*
 * Combines items itemA and itemB. Returns the item made, or -1 on failure,
 * and the quantity in *count: the recipe's yield class gives a base, then
 * each roll under the next chance adds one. The ingredients are used up
 * either way (from the pouch in mode 0, else from the save's pouch, or the
 * box when the pouch has none).
 */
s16 Combine_Do(u8 *count, Player *player, s16 itemA, s16 itemB, s32 mode)
{
    Recipe *recipe;
    s16 result;
    u8 *chance;
    s32 roll;

    recipe = Combine_FindRecipe(itemA, itemB);
    if (recipe == NULL) {
        return -1;
    }
    result = (s8)Combine_GetRate(itemA, itemB, mode);
    if ((u16)Rand_Next(1) % 100 < result) {
        result = recipe->result;
        if (Combine_HasFixedYield(player, result) == 1) {
            *count = gRecipeYieldBase[recipe->yieldClass];
        } else {
            chance = gRecipeYieldChances[recipe->yieldClass];
            *count = *chance++;
            roll = (u16)Rand_Next(1) % 100;
            for (;;) {
                if (roll < *chance) {
                    break;
                }
                (*count)++;
                roll -= *chance;
                chance++;
            }
        }
    } else {
        *count = 0;
        result = -1;
    }
    if (mode == 0) {
        Pouch_Add(player, itemA, -1);
        Pouch_Add(player, itemB, -1);
    } else {
        if (SavePouch_Count(itemA) == 0) {
            ItemBox_Remove(itemA, 1);
        } else {
            SavePouch_Add(itemA, -1);
        }
        if (SavePouch_Count(itemB) == 0) {
            ItemBox_Remove(itemB, 1);
        } else {
            SavePouch_Add(itemB, -1);
        }
    }
    return result;
}

/* Nonzero for ammo and coatings when the player has SKILL_COMBINE_AMMO_FIXED:
 * such results are made in the yield class's base quantity. */
s32 Combine_HasFixedYield(Player *player, u16 item)
{
    u8 type = gItemData[item].type;

    if (type != ITEM_TYPE_AMMO && type != ITEM_TYPE_COATING) {
        return 0;
    }
    return Player_HasSkill(player, SKILL_COMBINE_AMMO_FIXED);
}

/*
 * Success rate in percent (mode != 0: always 100; -1 when the local player
 * can't use the recipe): the recipe's base rate, 100 for recipes flagged 2
 * with SKILL_COMBINE_SURE, then the combination skill (-15 to +20), +5 for a
 * known recipe (else 95 and up round to 100), plus the bonus for the
 * combination books held (counted in order, up to the first missing), at
 * most 100.
 */
s32 Combine_RecipeRate(Recipe *recipe, s32 mode)
{
    u8 rate;
    Player *player;
    u32 i;

    if (mode == 0) {
        player = &gPlayers[gScene.localPlayer];
        if (Recipe_CanUse(player, recipe) != 1) {
            return -1;
        }
        rate = gRecipeRates[recipe->rateClass];
        if ((recipe->flags & RECIPE_SURE_WITH_SKILL) && Player_HasSkill(player, SKILL_COMBINE_SURE) == 1) {
            return 100;
        }
        switch (Player_GetSkill(player, SKILL_GROUP_COMBINE)) {
        case SKILL_COMBINE_PLUS20:
            rate += 20;
            break;
        case SKILL_COMBINE_PLUS10:
            rate += 10;
            break;
        case SKILL_COMBINE_PLUS5:
            rate += 5;
            break;
        case SKILL_COMBINE_MINUS5:
            rate -= 5;
            break;
        case SKILL_COMBINE_MINUS15:
            rate -= 15;
            break;
        }
        if (Recipe_IsKnown(recipe) == 1) {
            rate += 5;
        } else if (rate >= 95) {
            rate = 100;
        }
        for (i = 0; i < COMBINE_BOOK_COUNT; i++) {
            if (Pouch_Count(player, gCombineBookItems[i]) == 0) {
                break;
            }
        }
        rate += gCombineBookBonus[i];
    } else {
        return 100;
    }
    if (rate > 100) {
        rate = 100;
    }
    return (s8)rate;
}

/* The success rate of combining itemA and itemB (Combine_RecipeRate), -1
 * without a recipe. */
s32 Combine_GetRate(s16 itemA, s16 itemB, s32 mode)
{
    Recipe *recipe = Combine_FindRecipe(itemA, itemB);

    if (recipe == NULL) {
        return -1;
    }
    return Combine_RecipeRate(recipe, mode);
}

/* The quantity a recipe makes with SKILL_COMBINE_AMMO_FIXED. */
u8 Recipe_YieldBase(Recipe *recipe)
{
    return gRecipeYieldBase[recipe->yieldClass];
}

/* The quantity a recipe makes at least (its yield chances' first byte). */
u8 Recipe_YieldAmount(Recipe *recipe)
{
    return *gRecipeYieldChances[recipe->yieldClass];
}

/* The text naming a recipe's yield. */
void *Recipe_YieldLabel(Recipe *recipe)
{
    return gRecipeYieldLabels[recipe->yieldClass];
}

/* 1 if the recipe for itemA and itemB is in the player's combination list. */
s32 Combine_IsKnown(s16 itemA, s16 itemB)
{
    Recipe *recipe = Combine_FindRecipe(itemA, itemB);

    if (recipe == NULL) {
        return 0;
    }
    return (gSave.recipeBits[recipe->listBit >> 3] & (1 << (recipe->listBit & 7))) != 0;
}

/* 1 if the player can use (Recipe_CanUse) a three-item recipe with `item` among its
 * ingredients: first the recipes listed under it as the smallest ingredient, then under each
 * smaller item, those with it as the middle or the largest one. itemId is one widening copy of
 * the parameter, made at the top and used by every read; with the locals declared in this
 * order, and the second search with its own copies of them (later*), the compiler gives each the
 * register the game's code uses. */
s32 Combine_ItemUsable(s16 item)
{
    u8 *index;
    Player *pl;
    RecipeRun *run;
    s32 runCount;
    Recipe *recipe;
    s32 laterRecipeCount;
    s32 itemId = item;
    s32 laterRunCount;
    RecipeRun *laterRun;
    s32 i;
    Recipe *laterRecipe;
    s32 recipeCount;

    pl = &gPlayers[gScene.localPlayer];
    index = &gRecipe3First[itemId + itemId];
    if (index[RECIPE_INDEX_COUNT] != 0) {
        for (runCount = index[RECIPE_INDEX_COUNT], run = &gRecipe3Second[index[RECIPE_INDEX_FIRST]]; runCount > 0; runCount--, run++) {
            for (recipe = &gRecipesEx[run->start], recipeCount = run->count; recipeCount > 0; recipeCount--, recipe++) {
                if (Recipe_CanUse(pl, recipe) == 1) {
                    return 1;
                }
            }
        }
    }
    for (i = 0; i < itemId; i++) {
        index = &gRecipe3First[i + i];
        if (index[RECIPE_INDEX_COUNT] == 0) {
            continue;
        }
        for (laterRun = &gRecipe3Second[index[RECIPE_INDEX_FIRST]], laterRunCount = index[RECIPE_INDEX_COUNT]; laterRunCount > 0; laterRunCount--, laterRun++) {
            laterRecipe = &gRecipesEx[laterRun->start];
            laterRecipeCount = laterRun->count;
            if (laterRun->item == itemId) {
                for (; laterRecipeCount > 0; laterRecipeCount--, laterRecipe++) {
                    if (Recipe_CanUse(pl, laterRecipe) == 1) {
                        return 1;
                    }
                }
            } else {
                for (; laterRecipeCount > 0; laterRecipeCount--, laterRecipe++) {
                    if (laterRecipe->item == itemId && Recipe_CanUse(pl, laterRecipe) == 1) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* 1 if the player can use (Recipe_CanUse) a three-item recipe with both itemA and itemB among
 * its ingredients. The second search reuses the first one's runCount (as its item counter)
 * and recipe, as the game's code does. */
s32 Combine_CanUsePair(s16 itemA, s16 itemB)
{
    s32 laterRecipeCount;
    u8 *index;
    s16 swap;
    Player *pl;
    Recipe *recipe;
    s32 laterRunCount;
    RecipeRun *run;
    s32 runCount;
    s32 recipeCount;
    RecipeRun *laterRun;

    pl = &gPlayers[gScene.localPlayer];
    if (itemA > itemB) {
        swap = itemA;
        itemA = itemB;
        itemB = swap;
    }
    index = &gRecipe3First[itemA + itemA];
    if (index[RECIPE_INDEX_COUNT] != 0) {
        for (run = &gRecipe3Second[index[RECIPE_INDEX_FIRST]], runCount = index[RECIPE_INDEX_COUNT]; runCount > 0; runCount--, run++) {
            for (recipe = &gRecipesEx[run->start], recipeCount = run->count; recipeCount > 0; recipeCount--, recipe++) {
                if ((run->item == itemB || recipe->item == itemB) && Recipe_CanUse(pl, recipe) == 1) {
                    return 1;
                }
            }
        }
    }
    for (runCount = 0; runCount < itemA; runCount++) {
        index = &gRecipe3First[runCount + runCount];
        if (index[RECIPE_INDEX_COUNT] == 0) {
            continue;
        }
        for (laterRun = &gRecipe3Second[index[RECIPE_INDEX_FIRST]], laterRunCount = index[RECIPE_INDEX_COUNT]; laterRunCount > 0; laterRunCount--, laterRun++) {
            if (laterRun->item == itemA) {
                for (recipe = &gRecipesEx[laterRun->start], laterRecipeCount = laterRun->count; laterRecipeCount > 0; laterRecipeCount--, recipe++) {
                    if (recipe->item == itemB && Recipe_CanUse(pl, recipe) == 1) {
                        return 1;
                    }
                }
                break;
            }
        }
    }
    return 0;
}


/* The three-item recipe (gRecipesEx) of itemA, itemB and itemC, in any order, if the player
 * can use it (Recipe_CanUse); else NULL. It is defined in the old (K&R) form: the callers in
 * this file see the (s32, s32, s32) declaration, and the body uses the three as s16 without
 * narrowing them. With an s16 prototype, or s32 parameters narrowed in the body, three
 * functions compile differently. */
Recipe *Combine_FindRecipe3(itemA, itemB, itemC)
    s16 itemA;
    s16 itemB;
    s16 itemC;
{
    s16 swap;
    u8 *index;
    RecipeRun *run;
    Recipe *recipe;
    s32 count;

    if (itemC < itemB) {
        swap = itemC;
        itemC = itemB;
        itemB = swap;
    }
    if (itemB < itemA) {
        swap = itemB;
        itemB = itemA;
        itemA = swap;
    }
    if (itemC < itemB) {
        swap = itemC;
        itemC = itemB;
        itemB = swap;
    }
    index = &gRecipe3First[itemA + itemA];
    if (index[RECIPE_INDEX_COUNT] == 0) {
        return NULL;
    }
    for (run = &gRecipe3Second[index[RECIPE_INDEX_FIRST]], count = index[RECIPE_INDEX_COUNT]; count != 0; count--, run++) {
        if (run->item == itemB) {
            for (recipe = &gRecipesEx[run->start], count = run->count; count != 0; count--, recipe++) {
                if (recipe->item == itemC) {
                    if (Recipe_CanUse(&gPlayers[gScene.localPlayer], recipe) == 1) {
                        return recipe;
                    }
                    recipe = NULL;
                    return recipe;
                }
            }
            break;
        }
    }
    return NULL;
}

extern u8 gRecipeYieldBase[];
extern u8 *gRecipeYieldChances[];

/* Uses up one of the item from the saved pouch, or from the box when the pouch has none.
 * The u16 copy is masked once for the three calls. */
static inline void Combine_UseFromBox(s32 itemId)
{
    u16 item = itemId;

    if (SavePouch_Count(item) == 0) {
        ItemBox_Remove(item, 1);
    } else {
        SavePouch_Add(item, -1);
    }
}

/* Combine_Do for three items: the item made, with the quantity in *count, or -1 when the
 * roll fails or there is no such recipe. The ingredients are used up either way: from the
 * pouch in mode 0, else from the saved pouch or the box. One variable holds the recipe and
 * then its yield chances: with a Recipe * and a u8 * the compiler swaps two registers. */
s16 Combine_Do3(u8 *count, Player *player, s32 itemA, s32 itemB, s32 itemC, s32 mode)
{
    u8 *recipe;
    s16 result = -1;
    s32 roll;

    recipe = (u8 *)Combine_FindRecipe3(itemA, itemB, itemC);
    if (recipe == NULL) {
        return result;
    }
    result = (s8)Combine_RecipeRate3(itemA, itemB, itemC, mode);
    if ((u16)Rand_Next(1) % 100 < result) {
        result = ((Recipe *)recipe)->result;
        if (Player_HasSkill(player, SKILL_COMBINE_AMMO_FIXED) == 1) {
            *count = gRecipeYieldBase[((Recipe *)recipe)->yieldClass];
        } else {
            recipe = gRecipeYieldChances[((Recipe *)recipe)->yieldClass];
            *count = *recipe++;
            roll = (u16)Rand_Next(1) % 100;
            for (;;) {
                if (roll < *recipe) {
                    break;
                }
                (*count)++;
                roll -= *recipe;
                recipe++;
            }
        }
    } else {
        *count = 0;
        result = -1;
    }
    if (mode == 0) {
        Pouch_Add(player, itemA, -1);
        Pouch_Add(player, itemB, -1);
        Pouch_Add(player, itemC, -1);
    } else {
        Combine_UseFromBox(itemA);
        Combine_UseFromBox(itemB);
        Combine_UseFromBox(itemC);
    }
    return result;
}

/* The success rate of the three-item recipe of itemA, itemB and itemC (Combine_RecipeRate),
 * -1 when there is none the player can use. */
s32 Combine_RecipeRate3(s32 itemA, s32 itemB, s32 itemC, s32 mode)
{
    Recipe *recipe;

    recipe = Combine_FindRecipe3(itemA, itemB, itemC);
    if (recipe == NULL) {
        return -1;
    }
    return Combine_RecipeRate(recipe, mode);
}

/* 1 if the three-item recipe of itemA, itemB and itemC is in the player's combination list
 * (gSave.recipeBits). */
s32 Combine_IsKnown3(s32 itemA, s32 itemB, s32 itemC)
{
    Recipe *recipe;
    u8 bit;

    recipe = Combine_FindRecipe3(itemA, itemB, itemC);
    if (recipe == NULL) {
        return 0;
    }
    bit = recipe->listBit;
    return (gSave.recipeBits[(s32) bit >> 3] & (1 << (bit & 7))) != 0;
}


/* 1 when the player knows (Recipe_IsKnown) at least 90% of the recipes that need no skill
 * (RECIPE_NEEDS_SKILL clear), of both tables. */
s32 Combine_KnowsMostRecipes(void)
{
    u16 total;
    u16 known;
    Recipe *recipe;
    s32 i;

    total = known = 0;
    for (recipe = gRecipes, i = RECIPE_COUNT; i > 0; i--, recipe++) {
        if (!(recipe->flags & RECIPE_NEEDS_SKILL)) {
            total++;
            if (Recipe_IsKnown(recipe) == 1) {
                known++;
            }
        }
    }
    for (recipe = gRecipesEx, i = RECIPE_EX_COUNT; i > 0; i--, recipe++) {
        if (!(recipe->flags & RECIPE_NEEDS_SKILL)) {
            total++;
            if (Recipe_IsKnown(recipe) == 1) {
                known++;
            }
        }
    }
    return known >= (s16)(0.9f * total);
}

/* Grants AWARD_RECIPES (Award_Grant) once 213 of the RECIPE_TOTAL recipes are known, and
 * then returns 1; 0 otherwise, and once it is granted. */
s32 Recipe_CheckAllKnown(void)
{
    SaveData *save = (SaveData *)&gSave;
    s32 i;
    u16 known;

    if (Save_HasTitle(AWARD_RECIPES) == 1) {
        return 0;
    }
    known = 0;
    for (i = 0; i < RECIPE_TOTAL; i++) {
        if (save->recipeBits[i >> 3] & (1 << (i & 7))) {
            known++;
        }
    }
    if (known >= 213) {
        Award_Grant(AWARD_RECIPES);
        return 1;
    }
    return 0;
}

/* 1 if the player can use the recipe (Recipe_CanUse) and it is in the combination list
 * (its listBit in gSave.recipeBits). */
s32 Recipe_IsKnown(Recipe *recipe)
{
    u8 bit;

    if (Recipe_CanUse(&gPlayers[gScene.localPlayer], recipe) != 1) {
        return 0;
    }
    bit = recipe->listBit;
    return (gSave.recipeBits[(s32) bit >> 3] & (1 << (bit & 7))) != 0;
}

/* Adds the recipe to the combination list (its listBit in gSave.recipeBits). */
void Recipe_SetKnown(Recipe *recipe, s16 *unused)
{
    u8 *bits;
    u8 bit;

    bit = recipe->listBit;
    bits = (u8 *)gSave.recipeBits + ((s32) bit >> 3);
    *bits |= (1 << (bit & 7)) & 0xFF;
}

/* 1 if the player can use a recipe with the item: a two-item one (flag 0,
 * Combine_ItemHasRecipe) or a three-item one (else, Combine_ItemUsable). */
s32 Combine_ItemHasRecipeIn(s16 item, u8 flag)
{
    if (flag == 0) {
        return Combine_ItemHasRecipe(item);
    }
    return Combine_ItemUsable(item);
}

extern u8 gRecipeIndex[];

/* The ingredients of a recipe, from where it sits in the combination indexes: for a two-item
 * recipe (RECIPE_THREE_ITEMS clear) the item whose gRecipeIndex run holds it, for a three-item
 * one the items of its gRecipe3First and gRecipe3Second runs. items gets {first, second,
 * third} (third 0 for two items), and it returns 1; 0 when not found. */
s32 Combine_FindRecipeIndex(Recipe *recipe, s16 *items)
{
    u16 exIndex;
    u32 i;
    u8 *index;
    u32 j;
    RecipeRun *run;
    u32 smallest;

    if (!(recipe->flags & RECIPE_THREE_ITEMS)) {
        for (index = gRecipeIndex, i = 0; i < recipe->item; i++, index += 2) {
            if (index[RECIPE_INDEX_COUNT] == 0) {
                continue;
            }
            if (index[RECIPE_INDEX_FIRST] > recipe->listBit) {
                continue;
            }
            if (index[RECIPE_INDEX_FIRST] + index[RECIPE_INDEX_COUNT] > recipe->listBit) {
                items[0] = i;
                items[1] = recipe->item;
                items[2] = 0;
                return 1;
            }
        }
    } else {
        exIndex = recipe->listBit - RECIPE_COUNT;
        for (j = 0, run = gRecipe3Second;; j++) {
        if (!(run->start > exIndex) && run->start + run->count > exIndex) {
            for (index = gRecipe3First, smallest = 0; smallest < recipe->item; smallest++, index += 2) {
                if (index[RECIPE_INDEX_COUNT] == 0) {
                    continue;
                }
                if (index[RECIPE_INDEX_FIRST] > j) {
                    continue;
                }
                if (index[RECIPE_INDEX_FIRST] + index[RECIPE_INDEX_COUNT] > j) {
                    items[0] = smallest;
                    smallest = run->item;
                    items[1] = smallest;
                    items[2] = recipe->item;
                    return 1;
                }
            }
        }
        run++;
        if (run->item == 0) {
            break;
        }
        }
    }
    return 0;
}

/* Lists in out (at most 3) the recipes, two-item then three-item, that make `item` and pass
 * Recipe_MatchesFlags with flags (RECIPE_LIST_*), each with its ingredients
 * (Combine_FindRecipeIndex); returns how many. */
u8 Combine_ListRecipesFor(RecipeHit *out, s16 item, s32 flags)
{
    Recipe *candidate;
    u8 found = 0;
    s32 i;

    out[0].recipe = out[1].recipe = out[2].recipe = NULL;
    for (candidate = gRecipes, i = RECIPE_COUNT; i != 0; i--, candidate++) {
        if (candidate->result == item && Recipe_MatchesFlags(candidate, flags) == 1) {
            out->recipe = candidate;
            Combine_FindRecipeIndex(candidate, out->info);
            found++;
            out++;
            if (found >= 3) {
                return found;
            }
        }
    }
    for (candidate = gRecipesEx, i = RECIPE_EX_COUNT; i != 0; i--, candidate++) {
        if (candidate->result == item && Recipe_MatchesFlags(candidate, flags) == 1) {
            out->recipe = candidate;
            Combine_FindRecipeIndex(candidate, out->info);
            found++;
            out++;
            if (found >= 3) {
                return found;
            }
        }
    }
    return found;
}

/* 1 when a recipe, two-item or three-item, makes `item` and passes Recipe_MatchesFlags with
 * flags (RECIPE_LIST_*). */
s32 Recipe_IsListedFor(s16 item, s32 flags)
{
    Recipe *recipe;
    s32 i;

    for (recipe = gRecipes, i = RECIPE_COUNT; i != 0; i--, recipe++) {
        if (recipe->result == item && Recipe_MatchesFlags(recipe, flags) == 1) {
            return 1;
        }
    }
    for (recipe = gRecipesEx, i = RECIPE_EX_COUNT; i != 0; i--, recipe++) {
        if (recipe->result == item && Recipe_MatchesFlags(recipe, flags) == 1) {
            return 1;
        }
    }
    return 0;
}

/* 1 if the player can use the recipe: one without RECIPE_NEEDS_SKILL always; else with
 * SKILL_RECIPE_FLAG1, or with ITEM_ID_RECIPE_UNLOCK in the pouch. */
s32 Recipe_CanUse(Player *player, Recipe *recipe)
{
    if ((recipe != NULL) && !(recipe->flags & RECIPE_NEEDS_SKILL)) {
        return 1;
    }
    if (Player_HasSkill(player, SKILL_RECIPE_FLAG1) == 1) {
        return 1;
    }
    return (s16)Pouch_Count(player, ITEM_ID_RECIPE_UNLOCK) != 0;
}

/* A (key, value) pair sorted by Sort_PairsDesc. */
typedef struct SortPair {
    u16 key;
    s16 val;
} SortPair;

/* Shell-sorts n pairs by key descending, then value ascending (gaps 1, 4, 13, ...). */
void Sort_PairsDesc(SortPair *pairs, u32 n)
{
    SortPair pair;
    u32 i;
    s32 gap;
    s32 j;

    for (gap = 1; gap < n; gap = gap * 3 + 1) {
    }
    for (gap /= 3; gap > 0; gap /= 3) {
        for (i = gap; i < n; i++) {
            pair = pairs[i];
            for (j = i - gap; j >= 0; j -= gap) {
                if (pairs[j].key > pair.key || (pairs[j].key == pair.key && pairs[j].val < pair.val)) {
                    pairs[j + gap] = pairs[j];
                } else {
                    break;
                }
            }
            pairs[j + gap] = pair;
        }
    }
}

/* 1 if the item is a fishing bait (the lobby's fishing lists them). */
s32 Item_IsBait(u16 item, s32 unused1)
{
    switch (item) {
    case ITEM_ID_FISH_BAIT_A:
    case ITEM_ID_FISH_BAIT_B:
    case ITEM_ID_FISH_BAIT_C:
    case ITEM_ID_FISH_BAIT_LURE_ALT:
    case ITEM_ID_FISH_BAIT_D:
    case ITEM_ID_FISH_BAIT_E:
    case ITEM_ID_FISH_BAIT_F:
    case ITEM_ID_FISH_BAIT_G:
        return 1;
    }
    return 0;
}

/* 1 if the recipe passes the list's filter (RECIPE_LIST_*): usable by the local player
 * (Recipe_CanUse) unless RECIPE_LIST_NO_SKILL or RECIPE_LIST_USABLE_OR_NOT says otherwise, and
 * known (Recipe_IsKnown) unless RECIPE_LIST_UNKNOWN. */
s32 Recipe_MatchesFlags(Recipe *recipe, s32 flags)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    if (!(flags & RECIPE_LIST_USABLE_OR_NOT)) {
        if (!(flags & RECIPE_LIST_NO_SKILL)) {
            if (Recipe_CanUse(pl, recipe) != 1) {
                return 0;
            }
        } else if (recipe->flags & RECIPE_NEEDS_SKILL) {
            return 0;
        }
    }
    if (!(flags & RECIPE_LIST_UNKNOWN) && Recipe_IsKnown(recipe) != 1) {
        return 0;
    }
    return 1;
}

/* The recipes. */
Recipe gRecipes[190] = {
#include "sub_main/0041D2F0.inc"
};
u8 gRecipeIndex[1856] = {
#include "sub_main/0041D8E0.inc"
};
Recipe gRecipesEx[48] = {
#include "sub_main/0041E020.inc"
};
RecipeRun gRecipe3Second[44] = {
#include "sub_main/0041E1A0.inc"
};
u8 gRecipe3First[1848] = {
#include "sub_main/0041E250.inc"
};
u8 gRecipeRates[8] = {
#include "sub_main/0041E988.inc"
};
u8 gCombineBookBonus[8] = {
#include "sub_main/0041E990.inc"
};
u16 gCombineBookItems[COMBINE_BOOK_COUNT + 1] = {
#include "sub_main/0041E998.inc"
};
u8 gRecipeYieldChances_0[8] = {
#include "sub_main/0041E9A8.inc"
};
u8 gRecipeYieldChances_1[8] = {
#include "sub_main/0041E9B0.inc"
};
u8 gRecipeYieldChances_2[8] = {
#include "sub_main/0041E9B8.inc"
};
u8 gRecipeYieldChances_3[8] = {
#include "sub_main/0041E9C0.inc"
};
u8 gRecipeYieldChances_4[8] = {
#include "sub_main/0041E9C8.inc"
};
u8 gRecipeYieldChances_5[8] = {
#include "sub_main/0041E9D0.inc"
};
u8 gRecipeYieldChances_6[8] = {
#include "sub_main/0041E9D8.inc"
};
u8 gRecipeYieldChances_7[8] = {
#include "sub_main/0041E9E0.inc"
};
u8 gRecipeYieldChances_8[8] = {
#include "sub_main/0041E9E8.inc"
};
u8 *gRecipeYieldChances[10] = {
    gRecipeYieldChances_0, gRecipeYieldChances_1, gRecipeYieldChances_2, gRecipeYieldChances_3,
    gRecipeYieldChances_4, gRecipeYieldChances_5, gRecipeYieldChances_6, gRecipeYieldChances_7,
    gRecipeYieldChances_8, NULL,
};
u8 gRecipeYieldBase[12] = {
#include "sub_main/0041EA18.inc"
};
