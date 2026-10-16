/*
 * Item combination: recipe lookup, the success rate and the
 * combination itself, which draws from random stream 1 twice (success,
 * then the quantity made).
 *
 * Recipes are 8-byte records (gRecipes, include/combine.h), listed per first
 * ingredient (gRecipeIndex: two bytes per item id, the smaller of the two:
 * the count and the first record). The three-item recipes are in
 * unk_331A40.c.
 */
#include "common.h"
#include "scene.h"
#include "player.h"
#include "item.h"
#include "save.h"
#include "combine.h"


extern u8 gRecipeIndex[];       /* per item: count, first recipe */
/* The two bytes of an item's gRecipeIndex entry. Read as bytes: the game
 * computes the entry's address as item + item, which a struct index would
 * not give. */
#define RECIPE_INDEX_COUNT  0
#define RECIPE_INDEX_FIRST  1
extern u8 gRecipeRates[];       /* base success rate (percent) by class */
extern u8 gCombineBookBonus[];         /* rate bonus by combination books owned */
#define COMBINE_BOOK_COUNT 5
extern u16 gCombineBookItems[COMBINE_BOOK_COUNT];  /* the combination books (items) */
extern u8 gRecipeYieldBase[];         /* quantity made, by yield class */
extern u8 *gRecipeYieldChances[];        /* quantity: base, then chances of one more */
extern char *gRecipeYieldLabels[];

s32 Recipe_CanUse(Player *player, Recipe *r);
s32 Player_HasSkill(Player *p, u8 skill);
u8 Player_GetSkill(Player *p, u8 group);
s32 Recipe_IsKnown(Recipe *r);
s16 Pouch_Count(Player *player, u16 item);
u8 Pouch_Add(Player *p, u16 item, s16 n);
s32 SavePouch_Count(u16 item);
s16 ItemBox_Remove(u16 item, s16 count);
u8 SavePouch_Add(u16 item, s16 n);
u16 Rand_Next(s32 stream);
s32 Combine_HasFixedYield(Player *player, u16 item);

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

s32 Combine_GetRate(s16 a, s16 b, s32 mode);

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

/* combine's .data (0x41D2F0-0x41E020) */
Recipe gRecipes[190] = {
#include "sub_main/0041D2F0.inc"
};
u8 gRecipeIndex[1856] = {
#include "sub_main/0041D8E0.inc"
};
