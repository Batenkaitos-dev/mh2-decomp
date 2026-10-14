/*
 * Item combination: recipe lookup, the success rate and the
 * combination itself, which draws from random stream 1 twice (success,
 * then the quantity made).
 *
 * Recipes are 8-byte records (gRecipes), listed per first ingredient
 * (gRecipeIndex: count and first record, by the smaller item id).
 */
#include "common.h"
#include "scene.h"
#include "player.h"
#include "item.h"
#include "save.h"
#include "combine.h"


extern u8 gRecipeIndex[];       /* per item: count, first recipe */
extern u8 gRecipeRates[];       /* base success rate (percent) by class */
extern u8 gCombineBookBonus[];         /* rate bonus by combination books owned */
extern u16 gCombineBookItems[5];       /* the combination books (items) */
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
    u8 *ix = &gRecipeIndex[item + item];
    Recipe *r;
    s32 n;
    s32 i;

    n = ix[0];
    if (n != 0) {
        r = &gRecipes[ix[1]];
        for (; n > 0; n--, r++) {
            if (Recipe_CanUse(&gPlayers[gScene.localPlayer], r) == 1) {
                return 1;
            }
        }
    }
    for (i = 0; i < item; i++) {
        ix = &gRecipeIndex[i + i];
        n = ix[0];
        if (n != 0) {
            r = &gRecipes[ix[1]];
            for (; n > 0; n--, r++) {
                if (r->item == item && Recipe_CanUse(&gPlayers[gScene.localPlayer], r) == 1) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

/* The recipe for items a and b (in any order), or NULL. */
Recipe *Combine_FindRecipe(s16 a, s16 b)
{
    Recipe *r;
    u8 *ix;
    u32 n;
    s16 t;

    if (a > b) {
        t = b;
        b = a;
        a = t;
    }
    ix = &gRecipeIndex[a + a];
    n = ix[0];
    if (n == 0) {
        return NULL;
    }
    r = &gRecipes[ix[1]];
    for (; n != 0; n--, r++) {
        if (r->item == b) {
            if (Recipe_CanUse(&gPlayers[gScene.localPlayer], r) == 1) {
                return r;
            }
            break;
        }
    }
    return NULL;
}

s32 Combine_GetRate(s16 a, s16 b, s32 mode);

/*
 * Combines items a and b. Returns the item made, or -1 on failure, and
 * the quantity in *count. The ingredients are used up either way (from the
 * pouch in mode 0, else from the box).
 */
s16 Combine_Do(u8 *count, Player *player, s16 a, s16 b, s32 mode)
{
    Recipe *r;
    s16 result;
    u8 *chance;
    s32 x;

    r = Combine_FindRecipe(a, b);
    if (r == NULL) {
        return -1;
    }
    result = (s8)Combine_GetRate(a, b, mode);
    if ((u16)Rand_Next(1) % 100 < result) {
        result = r->result;
        if (Combine_HasFixedYield(player, result) == 1) {
            *count = gRecipeYieldBase[r->yieldClass];
        } else {
            chance = gRecipeYieldChances[r->yieldClass];
            *count = *chance++;
            x = (u16)Rand_Next(1) % 100;
            for (;;) {
                if (x < *chance) {
                    break;
                }
                (*count)++;
                x -= *chance;
                chance++;
            }
        }
    } else {
        *count = 0;
        result = -1;
    }
    if (mode == 0) {
        Pouch_Add(player, a, -1);
        Pouch_Add(player, b, -1);
    } else {
        if (SavePouch_Count(a) == 0) {
            ItemBox_Remove(a, 1);
        } else {
            SavePouch_Add(a, -1);
        }
        if (SavePouch_Count(b) == 0) {
            ItemBox_Remove(b, 1);
        } else {
            SavePouch_Add(b, -1);
        }
    }
    return result;
}

/* Nonzero for items of type 2 or 3 when the player has 0xAA (from
 * Player_HasSkill); such results are made in the fixed quantity. */
s32 Combine_HasFixedYield(Player *player, u16 item)
{
    u8 type = gItemData[item].type;

    if (type != 2 && type != 3) {
        return 0;
    }
    return Player_HasSkill(player, 0xAA);
}

/*
 * Success rate in percent (mode != 0: always 100): the recipe's base rate,
 * 100 for flagged recipes with item 0x48, then the combination skill
 * (-15..+20), +5 for some recipes (else 95 and up round to 100), plus a
 * bonus for each combination book owned (in order), at most 100.
 */
s32 Combine_RecipeRate(Recipe *r, s32 mode)
{
    u8 rate;
    Player *player;
    u32 i;

    if (mode == 0) {
        player = &gPlayers[gScene.localPlayer];
        if (Recipe_CanUse(player, r) != 1) {
            return -1;
        }
        rate = gRecipeRates[r->rateClass];
        if ((r->flags & 2) && Player_HasSkill(player, 0x48) == 1) {
            return 100;
        }
        switch (Player_GetSkill(player, 0x25)) {
        case 0xA7:
            rate += 20;
            break;
        case 0xA6:
            rate += 10;
            break;
        case 0xA5:
            rate += 5;
            break;
        case 0xA8:
            rate -= 5;
            break;
        case 0xA9:
            rate -= 15;
            break;
        }
        if (Recipe_IsKnown(r) == 1) {
            rate += 5;
        } else if (rate >= 95) {
            rate = 100;
        }
        for (i = 0; i < 5; i++) {
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

s32 Combine_GetRate(s16 a, s16 b, s32 mode)
{
    Recipe *r = Combine_FindRecipe(a, b);

    if (r == NULL) {
        return -1;
    }
    return Combine_RecipeRate(r, mode);
}

u8 Recipe_YieldBase(Recipe *r)
{
    return gRecipeYieldBase[r->yieldClass];
}

u8 Recipe_YieldAmount(Recipe *r)
{
    return *gRecipeYieldChances[r->yieldClass];
}

void *Recipe_YieldLabel(Recipe *r)
{
    return gRecipeYieldLabels[r->yieldClass];
}

/* 1 if the recipe for a and b is in the player's combination list. */
s32 Combine_IsKnown(s16 a, s16 b)
{
    Recipe *r = Combine_FindRecipe(a, b);

    if (r == NULL) {
        return 0;
    }
    return (gSave.recipeBits[r->listBit >> 3] & (1 << (r->listBit & 7))) != 0;
}

/* combine's .data (0x41D2F0-0x41E020) */
Recipe gRecipes[190] = {
#include "sub_main/0041D2F0.inc"
};
u8 gRecipeIndex[1856] = {
#include "sub_main/0041D8E0.inc"
};
