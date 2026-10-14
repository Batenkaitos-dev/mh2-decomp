#include "common.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "scene.h"
#include "area.h"
#include "hud.h"
#include "combine.h"
#include "light.h"

/* prototypes */
s32 Combine_RecipeRate(Recipe *r, s32 mode);
Recipe *Combine_FindRecipe3(s32, s32, s32); /* unit's own declaration: the K&R definition's narrow parameters arrive promoted, as this prototype says (a narrow prototype is incompatible with it) */
s32 Recipe_CanUse(Player *player, Recipe *r);
s32 Gs_SetState(u8 reg, u32 v);
extern u8 gLightSets;
s32 SaveFlag_Test(s32 n);
s32 Recipe_MatchesFlags(Recipe *o, s32 flags);
s32 Award_Grant(s32 n);
/* end of prototypes */

/* A second-level entry (gRecipe3Second, by the middle item): a run of gRecipesEx recipes. */
typedef struct RecipeIdx2 {
    s16 item;
    s8 n;
    u8 start;
} RecipeIdx2;

/* The recipe index by the smallest item: pairs {n, start} of runs of gRecipe3Second. */
extern u8 gRecipe3First[];
extern RecipeIdx2 gRecipe3Second[];

/* 1 if the player can use (Recipe_CanUse) a recipe that combines item with others.
 * it is one widening copy of the parameter, made at the top and used by every read (an
 * ordinary local copy); with the declaration order it
 * gives the game's saved registers. The second loop nest has its own q2, n2, r2 and m2. */
s32 Combine_ItemUsable(s16 item)
{
    u8 *e;
    Player *p;
    RecipeIdx2 *q;
    s32 n;
    Recipe *r;
    s32 m2;
    s32 it = item;
    s32 n2;
    RecipeIdx2 *q2;
    s32 i;
    Recipe *r2;
    s32 m;

    p = &gPlayers[gScene.localPlayer];
    e = &gRecipe3First[it + it];
    if (e[0] != 0) {
        for (n = e[0], q = &gRecipe3Second[e[1]]; n > 0; n--, q++) {
            for (r = &gRecipesEx[q->start], m = q->n; m > 0; m--, r++) {
                if (Recipe_CanUse(p, r) == 1) {
                    return 1;
                }
            }
        }
    }
    for (i = 0; i < it; i++) {
        e = &gRecipe3First[i + i];
        if (e[0] == 0) {
            continue;
        }
        for (q2 = &gRecipe3Second[e[1]], n2 = e[0]; n2 > 0; n2--, q2++) {
            r2 = &gRecipesEx[q2->start];
            m2 = q2->n;
            if (q2->item == it) {
                for (; m2 > 0; m2--, r2++) {
                    if (Recipe_CanUse(p, r2) == 1) {
                        return 1;
                    }
                }
            } else {
                for (; m2 > 0; m2--, r2++) {
                    if (r2->item == it && Recipe_CanUse(p, r2) == 1) {
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

/* 1 if the player can use (Recipe_CanUse) a recipe that combines items a and b. The second
 * search reuses the first one's n (as its item counter) and r (as its recipe walker). */
s32 Combine_CanUsePair(s16 a, s16 b)
{
    s32 m2;
    u8 *e;
    s16 t;
    Player *p;
    Recipe *r;
    s32 n2;
    RecipeIdx2 *q;
    s32 n;
    s32 m;
    RecipeIdx2 *q2;

    p = &gPlayers[gScene.localPlayer];
    if (a > b) {
        t = a;
        a = b;
        b = t;
    }
    e = &gRecipe3First[a + a];
    if (e[0] != 0) {
        for (q = &gRecipe3Second[e[1]], n = e[0]; n > 0; n--, q++) {
            for (r = &gRecipesEx[q->start], m = q->n; m > 0; m--, r++) {
                if ((q->item == b || r->item == b) && Recipe_CanUse(p, r) == 1) {
                    return 1;
                }
            }
        }
    }
    for (n = 0; n < a; n++) {
        e = &gRecipe3First[n + n];
        if (e[0] == 0) {
            continue;
        }
        for (q2 = &gRecipe3Second[e[1]], n2 = e[0]; n2 > 0; n2--, q2++) {
            if (q2->item == a) {
                for (r = &gRecipesEx[q2->start], m2 = q2->n; m2 > 0; m2--, r++) {
                    if (r->item == b && Recipe_CanUse(p, r) == 1) {
                        return 1;
                    }
                }
                break;
            }
        }
    }
    return 0;
}


/* The recipe (gRecipesEx) combining the three items a, b and c (in any order) that the player can use
 * (Recipe_CanUse), or NULL. K&R definition: the callers in this file see the (s32, s32, s32)
 * declaration and the body uses a, b, c as s16 without narrowing them (an s16 prototype or s32
 * parameters narrowed in the body: 3 functions differ). */
Recipe *Combine_FindRecipe3(a, b, c)
    s16 a;
    s16 b;
    s16 c;
{
    s16 t;
    u8 *e;
    RecipeIdx2 *p;
    Recipe *r;
    s32 n;

    if (c < b) {
        t = c;
        c = b;
        b = t;
    }
    if (b < a) {
        t = b;
        b = a;
        a = t;
    }
    if (c < b) {
        t = c;
        c = b;
        b = t;
    }
    e = &gRecipe3First[a + a];
    if (e[0] == 0) {
        return NULL;
    }
    for (p = &gRecipe3Second[e[1]], n = e[0]; n != 0; n--, p++) {
        if (p->item == b) {
            for (r = &gRecipesEx[p->start], n = p->n; n != 0; n--, r++) {
                if (r->item == c) {
                    if (Recipe_CanUse(&gPlayers[gScene.localPlayer], r) == 1) {
                        return r;
                    }
                    r = NULL;
                    return r;
                }
            }
            break;
        }
    }
    return NULL;
}

u8 Pouch_Add(Player *p, u16 item, s16 n);
s32 SavePouch_Count(u16 item);
s16 ItemBox_Remove(u16 item, s16 count);
u8 SavePouch_Add(u16 item, s16 n);
u16 Rand_Next(s32 stream);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Combine_RecipeRate3(s32 a, s32 b, s32 c, s32 mode);
extern u8 gRecipeYieldBase[];
extern u8 *gRecipeYieldChances[];

/* Uses up one of the item from the saved pouch, or from the box when the pouch has none. (The u16 copy
 * is masked once for the three calls.) */
static inline void Combine_UseFromBox(s32 n)
{
    u16 item = n;

    if (SavePouch_Count(item) == 0) {
        ItemBox_Remove(item, 1);
    } else {
        SavePouch_Add(item, -1);
    }
}

/* Combine_Do for three items a, b and c: the item made (or -1 on failure) with the quantity in *count;
 * the ingredients are used up either way (from the pouch in mode 0, else from the box). r is the
 * recipe, then its yield chances: one variable (a Recipe * and a u8 * swap s5 and s6). */
s16 Combine_Do3(u8 *count, Player *player, s32 a, s32 b, s32 c, s32 mode)
{
    u8 *r;
    s16 result = -1;
    s32 x;

    r = (u8 *)Combine_FindRecipe3(a, b, c);
    if (r == NULL) {
        return result;
    }
    result = (s8)Combine_RecipeRate3(a, b, c, mode);
    if ((u16)Rand_Next(1) % 100 < result) {
        result = ((Recipe *)r)->result;
        if (Player_HasSkill(player, 0xAA) == 1) {
            *count = gRecipeYieldBase[((Recipe *)r)->yieldClass];
        } else {
            r = gRecipeYieldChances[((Recipe *)r)->yieldClass];
            *count = *r++;
            x = (u16)Rand_Next(1) % 100;
            for (;;) {
                if (x < *r) {
                    break;
                }
                (*count)++;
                x -= *r;
                r++;
            }
        }
    } else {
        *count = 0;
        result = -1;
    }
    if (mode == 0) {
        Pouch_Add(player, a, -1);
        Pouch_Add(player, b, -1);
        Pouch_Add(player, c, -1);
    } else {
        Combine_UseFromBox(a);
        Combine_UseFromBox(b);
        Combine_UseFromBox(c);
    }
    return result;
}

s32 Combine_RecipeRate3(s32 arg0, s32 arg1, s32 arg2, s32 mode)
{
    Recipe *r;

    r = Combine_FindRecipe3(arg0, arg1, arg2);
    if (r == NULL) {
        return -1;
    }
    return Combine_RecipeRate(r, mode);
}

s32 Combine_IsKnown3(s32 arg0, s32 arg1, s32 arg2)
{
    Recipe *r;
    u8 bit;

    r = Combine_FindRecipe3(arg0, arg1, arg2);
    if (r == NULL) {
        return 0;
    }
    bit = r->listBit;
    return (gSave.recipeBits[(s32) bit >> 3] & (1 << (bit & 7))) != 0;
}

s32 Recipe_IsKnown(Recipe *r);

/* 1 when at least 90% of the recipes the player can see (flag bit 0 clear) are known (Recipe_IsKnown). */
s32 Combine_KnowsMostRecipes(void)
{
    u16 total;
    u16 known;
    Recipe *r;
    s32 i;

    total = known = 0;
    for (r = gRecipes, i = 0xBD; i > 0; i--, r++) {
        if (!(r->flags & 1)) {
            total++;
            if (Recipe_IsKnown(r) == 1) {
                known++;
            }
        }
    }
    for (r = gRecipesEx, i = 0x30; i > 0; i--, r++) {
        if (!(r->flags & 1)) {
            total++;
            if (Recipe_IsKnown(r) == 1) {
                known++;
            }
        }
    }
    return known >= (s16)(0.9f * total);
}

/* When 0xD5 of the 0xED combination recipes are known, sets flag 0x20
 * (Award_Grant) once and returns 1. */
s32 Recipe_CheckAllKnown(void)
{
    SaveData *s = (SaveData *)&gSave;
    s32 i;
    u16 n;

    if (SaveFlag_Test(0x20) == 1) {
        return 0;
    }
    n = 0;
    for (i = 0; i < 0xED; i++) {
        if (s->recipeBits[i >> 3] & (1 << (i & 7))) {
            n++;
        }
    }
    if (n >= 0xD5) {
        Award_Grant(0x20);
        return 1;
    }
    return 0;
}

s32 Recipe_IsKnown(Recipe *r)
{
    u8 bit;

    if (Recipe_CanUse(&gPlayers[gScene.localPlayer], r) != 1) {
        return 0;
    }
    bit = r->listBit;
    return (gSave.recipeBits[(s32) bit >> 3] & (1 << (bit & 7))) != 0;
}

/* gSave.recipeBits as its own symbol: &gSave.recipeBits[k] folds 0x20DB
 * into the load and store. */

void Recipe_SetKnown(Recipe *r, s16 *unused)
{
    u8 *p;
    u8 bit;

    bit = r->listBit;
    p = (u8 *)gSave.recipeBits + ((s32) bit >> 3);
    *p |= (1 << (bit & 7)) & 0xFF;
}

s32 Combine_ItemHasRecipe(s16 item);
s32 Combine_ItemUsable(s16 item);

s32 Combine_ItemHasRecipeIn(s16 item, u8 flag)
{
    if (flag == 0) {
        return Combine_ItemHasRecipe(item);
    }
    return Combine_ItemUsable(item);
}

extern u8 gRecipeIndex[];

/* Finds where recipe r sits in the combination indexes: for a two-item recipe (flag 0x80 clear) the first
 * item's entry in gRecipeIndex, for a three-item one the entries in gRecipe3First and gRecipe3Second; out gets
 * {first item, second item, third item} as ids of those runs. 0 when not found. */
s32 Combine_FindRecipeIndex(Recipe *r, s16 *out)
{
    u16 k;
    u32 i;
    u8 *e;
    u32 j;
    RecipeIdx2 *q;
    u32 i2;

    if (!(r->flags & 0x80)) {
        for (e = gRecipeIndex, i = 0; i < r->item; i++, e += 2) {
            if (e[0] == 0) {
                continue;
            }
            if (e[1] > r->listBit) {
                continue;
            }
            if (e[1] + e[0] > r->listBit) {
                out[0] = i;
                out[1] = r->item;
                out[2] = 0;
                return 1;
            }
        }
    } else {
        k = r->listBit - 0xBD;
        for (j = 0, q = gRecipe3Second;; j++) {
        if (!(q->start > k) && q->start + q->n > k) {
            for (e = gRecipe3First, i2 = 0; i2 < r->item; i2++, e += 2) {
                if (e[0] == 0) {
                    continue;
                }
                if (e[1] > j) {
                    continue;
                }
                if (e[1] + e[0] > j) {
                    out[0] = i2;
                    i2 = q->item;
                    out[1] = i2;
                    out[2] = r->item;
                    return 1;
                }
            }
        }
        q++;
        if (q->item == 0) {
            break;
        }
        }
    }
    return 0;
}

/* A combination found for an item (Combine_ListRecipesFor): the recipe and what Combine_FindRecipeIndex fills in. */

s32 Combine_FindRecipeIndex(Recipe *r, s16 *info);

/* Lists in out (up to 3) the listed recipes (gRecipes, then the 0x30 at gRecipesEx) that make item (Recipe_MatchesFlags
 * with flags); returns how many. */
u8 Combine_ListRecipesFor(RecipeHit *out, s16 item, s32 flags)
{
    Recipe *r;
    u8 n = 0;
    s32 i;

    out[0].recipe = out[1].recipe = out[2].recipe = NULL;
    for (r = gRecipes, i = 0xBD; i != 0; i--, r++) {
        if (r->result == item && Recipe_MatchesFlags(r, flags) == 1) {
            out->recipe = r;
            Combine_FindRecipeIndex(r, out->info);
            n++;
            out++;
            if (n >= 3) {
                return n;
            }
        }
    }
    for (r = gRecipesEx, i = 0x30; i != 0; i--, r++) {
        if (r->result == item && Recipe_MatchesFlags(r, flags) == 1) {
            out->recipe = r;
            Combine_FindRecipeIndex(r, out->info);
            n++;
            out++;
            if (n >= 3) {
                return n;
            }
        }
    }
    return n;
}

/* 1 when a listed recipe (gRecipes, then the 0x30 at gRecipesEx) makes
 * item (Recipe_MatchesFlags with flags). */
s32 Recipe_IsListedFor(s16 item, s32 flags)
{
    Recipe *r;
    s32 i;

    for (r = gRecipes, i = 0xBD; i != 0; i--, r++) {
        if (r->result == item && Recipe_MatchesFlags(r, flags) == 1) {
            return 1;
        }
    }
    for (r = gRecipesEx, i = 0x30; i != 0; i--, r++) {
        if (r->result == item && Recipe_MatchesFlags(r, flags) == 1) {
            return 1;
        }
    }
    return 0;
}

s32 Player_HasSkill(Player *p, u8 skill);
s16 Pouch_Count(Player *p, u16 item);

s32 Recipe_CanUse(Player *player, Recipe *r)
{
    if ((r != NULL) && !(r->flags & 1)) {
        return 1;
    }
    if (Player_HasSkill(player, 0xABU) == 1) {
        return 1;
    }
    return (s16)Pouch_Count(player, 6U) != 0;
}

/* A (key, value) pair sorted by Sort_PairsDesc. */
typedef struct SortPair {
    u16 key;
    s16 val;
} SortPair;

/* Shell-sorts n pairs by key descending, then value ascending (gaps 1, 4, 13, ...). */
void Sort_PairsDesc(SortPair *a, u32 n)
{
    SortPair t;
    u32 i;
    s32 h;
    s32 j;

    for (h = 1; h < n; h = h * 3 + 1) {
    }
    for (h /= 3; h > 0; h /= 3) {
        for (i = h; i < n; i++) {
            t = a[i];
            for (j = i - h; j >= 0; j -= h) {
                if (a[j].key > t.key || (a[j].key == t.key && a[j].val < t.val)) {
                    a[j + h] = a[j];
                } else {
                    break;
                }
            }
            a[j + h] = t;
        }
    }
}

s32 Item_IsBait(u16 item, s32 unused1)
{
    switch (item) {
    case 0x8E:
    case 0x8F:
    case 0x90:
    case 0x91:
    case 0x92:
    case 0x93:
    case 0x94:
    case 0x95:
        return 1;
    }
    return 0;
}

s32 Recipe_CanUse(Player *p, Recipe *o);
s32 Recipe_IsKnown(Recipe *o);
s32 Recipe_MatchesFlags(Recipe *o, s32 flags)
{
    Player *p = &gPlayers[gScene.localPlayer];

    if (!(flags & 4)) {
        if (!(flags & 2)) {
            if (Recipe_CanUse(p, o) != 1) {
                return 0;
            }
        } else if (o->flags & 1) {
            return 0;
        }
    }
    if (!(flags & 1) && Recipe_IsKnown(o) != 1) {
        return 0;
    }
    return 1;
}


extern f32 gLightDefaultDirs[];
extern f32 D_0041EC04[];
extern f32 D_0041EC08[];
extern f32 gLightDirsArea56[];
extern f32 D_0041EC34[];
extern f32 D_0041EC38[];
extern f32 gLightDefaultAmbs[];
extern f32 D_0041EC64[];
extern f32 D_0041EC68[];
extern f32 gLightAmbsArea56[];
extern f32 D_0041ECC4[];
extern f32 D_0041ECC8[];
extern f32 gLightDefaultCols[];
extern f32 D_0041ECF4[];
extern f32 D_0041ECF8[];
extern f32 gLightColsArea56[];
extern f32 D_0041ED24[];
extern f32 D_0041ED28[];

/* Sets light i of l: its direction and colour (mode 0) and its ambient (modes 0 and 1), from the area's
 * own lights when it has them (kind not 0), else the defaults (kind 0x56 has its own defaults first). */
static void Light_SetDefault(Light *l, u16 kind, s32 i, u16 mode)
{
    AreaState *area = &gAreaState;

    switch (kind) {
    case 0x56:
        switch (mode) {
        case 0:
            l->dir[0] = gLightDirsArea56[(s16)i * 3];
            l->dir[1] = D_0041EC34[(s16)i * 3];
            l->dir[2] = D_0041EC38[(s16)i * 3];
            l->col[0] = gLightColsArea56[(s16)i * 4];
            l->col[1] = D_0041ED24[(s16)i * 4];
            l->col[2] = D_0041ED28[(s16)i * 4];
        }
        switch (mode) {
        case 0:
        case 1:
            l->amb[0] = gLightAmbsArea56[(s16)i * 4];
            l->amb[1] = D_0041ECC4[(s16)i * 4];
            l->amb[2] = D_0041ECC8[(s16)i * 4];
        }
    }
    if (kind != 0 && area->envBlocks > 0) {
        switch (mode) {
        case 0:
            l->dir[0] = area->lightDir[(s16)i][0];
            l->dir[1] = area->lightDir[(s16)i][1];
            l->dir[2] = area->lightDir[(s16)i][2];
            l->col[0] = area->lightCol[(s16)i][0];
            l->col[1] = area->lightCol[(s16)i][1];
            l->col[2] = area->lightCol[(s16)i][2];
        }
        switch (mode) {
        case 0:
        case 1:
            l->amb[0] = area->lightAmb[(s16)i][0];
            l->amb[1] = area->lightAmb[(s16)i][1];
            l->amb[2] = area->lightAmb[(s16)i][2];
        }
    } else {
        switch (mode) {
        case 0:
            l->dir[0] = gLightDefaultDirs[(s16)i * 3];
            l->dir[1] = D_0041EC04[(s16)i * 3];
            l->dir[2] = D_0041EC08[(s16)i * 3];
            l->col[0] = gLightDefaultCols[(s16)i * 4];
            l->col[1] = D_0041ECF4[(s16)i * 4];
            l->col[2] = D_0041ECF8[(s16)i * 4];
        }
        switch (mode) {
        case 0:
        case 1:
            l->amb[0] = gLightDefaultAmbs[(s16)i * 4];
            l->amb[1] = D_0041EC64[(s16)i * 4];
            l->amb[2] = D_0041EC68[(s16)i * 4];
        }
    }
}

extern f32 (*gLightInitPosDir[])[3];
extern f32 (*D_0041EBE4)[3];
extern f32 (*gLightInitAmbCol[])[4];
extern f32 (*D_0041EBEC)[4];
extern f32 (*D_0041EBF0)[4];
void *memset(void *dst, s32 c, u32 n);

/* Clears the light sets (gLightSets): set 0 from the default tables (ambient times 10), set 1 from the
 * area (Light_SetDefault mode 0) with unk14 1.0. */
/* The check below needs the separate symbol: with gScene.area the load moves after the stores. */
extern u16 gSceneArea;

void Light_InitAll(s32 unused0)
{
    u8 *q;
    u8 *p;
    u8 *base;
    f32 *s;
    s32 i;
    s32 k;
    Light *l;

    base = &gLightSets;
    memset(base, 0, 0x290);
    for (i = 0, p = base + 0x10; i < 3; i++, p += 0x68) {
        l = (Light *)(p + 8);
        s = (*(f32 (**)[3])gLightInitPosDir)[i];
        for (k = 0; k < 3; k++) {
            l->pos[k] = *s++;
        }
        s = D_0041EBE4[i];
        for (k = 0; k < 3; k++) {
            l->dir[k] = *s++;
        }
        s = (*(f32 (**)[4])gLightInitAmbCol)[i];
        for (k = 0; k < 3; k++) {
            l->amb[k] = 10.0f * *s++;
        }
        s = D_0041EBEC[i];
        for (k = 0; k < 3; k++) {
            l->col[k] = *s++;
        }
        s = D_0041EBF0[i];
        for (k = 0; k < 3; k++) {
            l->unk14[k] = *s++;
        }
        l->unk00 = 0;
    }
    p = base + 0x140;
    for (i = 0, q = p + 0x10; i < 3; i++, q += 0x68) {
        l = (Light *)(q + 8);
        /* An empty check of the area's light kind (Light_SetDefault's 0 and 0x56), compiled
         * out (docs/mwcc-matching.md, "Statements scheduled as if a check were still there"): it
         * loads gSceneArea before the pos stores, as the original does; a one-term test, a
         * (void) read or a local read first leave the load after them. */
        if (gSceneArea != 0 && gSceneArea != 0x56) { /* compiled-out check */
        }
        l->pos[0] = 0.0f;
        l->pos[1] = 0.0f;
        l->pos[2] = 0.0f;
        Light_SetDefault(l, gSceneArea, (s16)i, 0);
        l->unk14[0] = 1.0f;
        l->unk14[1] = 1.0f;
        l->unk14[2] = 1.0f;
        l->unk00 = 0;
    }
}

void LightSet_UpdateFade(LightSet *set);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
extern f32 gCamMatrix[];

/* Sets v to (x, y, z). Inlined: its pointer parameter makes MWCC keep &v[1] and &v[2]
 * in saved registers across the light loop, as the game does (round 25; it replaced
 * round 18's pointer locals pv1, pv2 and pr1). */
static inline void Vec3_Set(f32 *v, f32 x, f32 y, f32 z)
{
    v[0] = x;
    v[1] = y;
    v[2] = z;
}

/* Updates the two light sets (gLightSets, 0x140 bytes each; mode 1: LightSet_UpdateFade), then points set 1's
 * third light down through the camera unless the scene is 0x11. */
void LightSet_UpdateAll(void)
{
    u8 *e;
    Light *q;
    f32 v[3];
    f32 r[3];
    LightSet *set;
    s32 i;

    for (i = 0, e = &gLightSets; i < 2; i++, e += 0x140) {
        set = (LightSet *)(e + 0x10);
        switch (set->mode) {
        case 0:
            break;
        case 1:
            LightSet_UpdateFade(set);
            break;
        }
        if (i == 1 && gScene.area != 0x11) {
            Vec3_Set(v, 0.0f, 0.0f, -1.0f);
            q = &set->lights[2];
            Vec_Rotate(r, v, gCamMatrix);
            q->dir[0] = r[0];
            q->dir[1] = r[1];
            q->dir[2] = r[2];
        }
    }
}

extern f32 *D_0041EAF8[];
extern f32 *D_0041EAFC[];

/* Mode 1 of a light set (LightSet_UpdateAll): state 0 starts a 30-frame fade; state 1 sets light 0 to the
 * D_0041EAF8[0] ambient and D_0041EAFC[0] colour plus (frames left / 30) and counts down; state 2 ends the mode. */
void LightSet_UpdateFade(LightSet *set)
{
    f32 *amb = D_0041EAF8[0];
    f32 *col = D_0041EAFC[0];
    Light *l = set->lights;
    s32 k;

    switch (set->state) {
    case 0:
        set->state++;
        set->left = 30;
        set->frames = 30;
        break;
    case 1:
        for (k = 0; k < 3; k++) {
            l->amb[k] = *amb++;
        }
        for (k = 0; k < 3; k++) {
            l->col[k] = *col++;
        }
        if (--set->left != 0) {
            l->amb[0] += (1.0f / set->frames) * set->left;
            l->amb[1] += (1.0f / set->frames) * set->left;
            l->amb[2] += (1.0f / set->frames) * set->left;
            l->col[0] += (1.0f / set->frames) * set->left;
            l->col[1] += (1.0f / set->frames) * set->left;
            l->col[2] += (1.0f / set->frames) * set->left;
        } else {
            set->state++;
        }
        break;
    case 2:
        set->mode = 0;
        break;
    }
}

extern f32 gLightFixedAmbs[3][4];

/* Sets the ambient colours of the three lights of light set n (gLightSets) for object o: in areas 0x1E,
 * 0x1C, 0xE, 0xD and 0xC the fixed gLightFixedAmbs ones while o[0x597] is set, else o's own (+0x664, the
 * first two lights) or the area's (Light_SetDefault). */
void LightSet_SetAmbient(void *obj, s32 n)
{
    u8 *o = obj;
    u8 *p;
    s32 i;
    Light *l;
    f32 (*own)[4];

    p = &gLightSets + n * 0x140 + 0x10;
    for (i = 0; i < 3; i++) {
        l = (Light *)(p + i * 0x68 + 8);
        switch (gScene.area) {
        case 0xC:
        case 0xD:
        case 0xE:
        case 0x1C:
        case 0x1E:
            if (o[0x597] != 0) {
                l->amb[0] = gLightFixedAmbs[i][0];
                l->amb[1] = gLightFixedAmbs[i][1];
                l->amb[2] = gLightFixedAmbs[i][2];
                continue;
            }
        }
        own = *(f32 (**)[4])(o + 0x664);
        if (own == NULL || i == 2) {
            Light_SetDefault(l, gScene.area, (s16)i, 1);
        } else {
            l->amb[0] = own[i][0];
            l->amb[1] = own[i][1];
            l->amb[2] = own[i][2];
        }
    }
}

/* Sets the three lights of light set kind (gLightSets) to its area's ambient colours (mode 1). */
void LightSet_ResetAmbient(s32 kind)
{
    u8 *p;
    s32 j;

    p = &gLightSets + kind * 0x140 + 0x10;
    for (j = 0; j < 3; j++, p += 0x68) {
        Light_SetDefault((Light *)(p + 8), kind, (s16)j, 1);
    }
}

void LightSet_Apply(s32 a)
{
    u8 *set;

    set = &gLightSets + (a * 0x140) + 0x10;
    Gs_SetState(0x5A, (s32) (set + 8));
    Gs_SetState(0x5B, (s32) (set + 0x70));
    Gs_SetState(0x5C, (s32) (set + 0xD8));
    Gs_SetState(1, 1);
}

void Player_ClearUnk540(Player *p)
{
    *(s32 *)((u8 *)p + 0x540) = 0;
    *(s32 *)((u8 *)p + 0x544) = 0;
    *(s32 *)((u8 *)p + 0x548) = 0;
}

extern u8 D_005EC320[];
void *memcpy(void *dst, const void *src, u32 n);

/* Loads the three lights of light set 1 (D_005EC320) with their ambient colours faded a fifth of the way
 * from o's colours (+0x540, one per light) and keeps the result in o. */
void LightSet_FadeAmbient(void *obj)
{
    u8 *o = obj;
    f32 r;
    u8 *p;
    f32 g;
    Light *src;
    s16 i;
    Light l;
    u32 c;
    f32 b;

    p = D_005EC320;
    for (i = 0; i < 3; p += 0x68, i++) {
        src = (Light *)(p + 8);
        memcpy(&l, src, sizeof(Light));
        c = *(u32 *)(o + i * 4 + 0x540);
        if (c == 0) {
            r = src->amb[0];
            g = src->amb[1];
            b = src->amb[2];
        } else {
            r = ((c >> 16) & 0xFF) / 255.0f;
            g = ((c >> 8) & 0xFF) / 255.0f;
            b = (c & 0xFF) / 255.0f;
            r += (src->amb[0] - r) / 5.0f;
            g += (src->amb[1] - g) / 5.0f;
            b += (src->amb[2] - b) / 5.0f;
            l.amb[0] = r;
            l.amb[1] = g;
            l.amb[2] = b;
        }
        *(u32 *)(o + i * 4 + 0x540) = 0xFF000000 | ((u32)(r * 255.0f) & 0xFF) << 16 | ((u32)(g * 255.0f) & 0xFF) << 8 | ((u32)(b * 255.0f) & 0xFF) | 0xFF000000;
        Gs_SetState((u8)(i + 0x5A), (s32)&l);
    }
    Gs_SetState(1, 1);
}

extern u32 gTexHandles[];
s32 Tex_IsIdSlot(s32 h);
s32 flReleasePaletteHandle(u32 id);
s32 flReleaseTextureHandle(u32 id);
void Tex_ReleaseIdSlots(void);

/* Releases the texture and palette handles of all 0x173 texture slots but 0, 0x148 and 0x155 (a slot
 * whose handle Tex_IsIdSlot frees is just cleared). */
void Tex_ReleaseAll(void)
{
    s32 i;
    u32 *h;
    u32 pal;

    for (i = 0, h = gTexHandles; i < 0x173; i++, h++) {
        if (*h == 0) {
            continue;
        }
        if (Tex_IsIdSlot(*h) == 1) {
            *h = 0;
            continue;
        }
        switch (i) {
        case 0:
        case 0x148:
        case 0x155:
            continue;
        }
        if ((pal = *h & 0xFFFF0000) != 0 && flReleasePaletteHandle(pal >> 16) == 1) {
            *h = *(u16 *)h;
        }
        if (*(u16 *)h != 0 && flReleaseTextureHandle(*(u16 *)h) == 1) {
            *h &= 0xFFFF0000;
        }
    }
    Tex_ReleaseIdSlots();
}

/* Releases the texture and palette handles of the n texture slots from start (as Tex_ReleaseAll). */
void Tex_ReleaseRange(s32 start, s32 n)
{
    s32 i;
    u32 *h;
    u32 pal;

    i = 0;
    if (i < n) {
        h = &gTexHandles[start];
        do {
            if (*h != 0) {
                if (Tex_IsIdSlot(*h) == 1) {
                    *h = 0;
                } else {
                    if ((pal = *h & 0xFFFF0000) != 0 && flReleasePaletteHandle(pal >> 16) == 1) {
                        *h = *(u16 *)h;
                    }
                    if (*(u16 *)h != 0 && flReleaseTextureHandle(*(u16 *)h) == 1) {
                        *h &= 0xFFFF0000;
                    }
                }
            }
            i++;
            h++;
        } while (i < n);
    }
}

/* unk_331A40's .data (0x41E020-0x41ED50) */
extern u32 D_005B2400[];
extern u32 D_005B2430[];
extern u32 D_0041EAC0[];
Recipe gRecipesEx[48] = {
#include "sub_main/0041E020.inc"
};
RecipeIdx2 gRecipe3Second[44] = {
#include "sub_main/0041E1A0.inc"
};
u8 gRecipe3First[1848] = {
#include "sub_main/0041E250.inc"
};
u32 gRecipeRates[2] = {
#include "sub_main/0041E988.inc"
};
u32 gCombineBookBonus[2] = {
#include "sub_main/0041E990.inc"
};
u32 gCombineBookItems[3] = {
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
u32 D_0041EA30[12] = {
#include "sub_main/0041EA30.inc"
};
u32 D_0041EA60[12] = {
#include "sub_main/0041EA60.inc"
};
u32 D_0041EA90[12] = {
#include "sub_main/0041EA90.inc"
};
u32 D_0041EAC0[24] = {
#include "sub_main/0041EAC0.inc"
};
u32 D_0041EB20[12] = {
#include "sub_main/0041EB20.inc"
};
u32 D_0041EB50[12] = {
#include "sub_main/0041EB50.inc"
};
u32 D_0041EB80[12] = {
#include "sub_main/0041EB80.inc"
};
u32 D_0041EBB0[12] = {
#include "sub_main/0041EBB0.inc"
};
f32 (*gLightInitPosDir[2])[3] = {
    (void *)D_005B2430, (void *)D_0041EB20,
};
f32 (*gLightInitAmbCol[2])[4] = {
    (void *)D_0041EB50, (void *)D_0041EB80,
};
f32 (*D_0041EBF0)[4] = (void *)D_0041EBB0;
f32 gLightDefaultDirs[12] = {
#include "sub_main/0041EC00.inc"
};
f32 gLightDirsArea56[12] = {
#include "sub_main/0041EC30.inc"
};
f32 gLightDefaultAmbs[12] = {
#include "sub_main/0041EC60.inc"
};
f32 gLightFixedAmbs[3][4] = {
#include "sub_main/0041EC90.inc"
};
f32 gLightAmbsArea56[12] = {
#include "sub_main/0041ECC0.inc"
};
f32 gLightDefaultCols[12] = {
#include "sub_main/0041ECF0.inc"
};
f32 gLightColsArea56[12] = {
#include "sub_main/0041ED20.inc"
};
