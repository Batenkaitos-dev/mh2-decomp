/*
 * The model pools of sub_main: four pools of slots, each with an in-use
 * flag per slot (gModelUsed, gMaterialSlotUsed, gBoneUsed, gMatSetUsed): MODEL_POOL_COUNT models (Model), and the
 * materials, bones and material sets the models take in runs of slots. A
 * pool's FindFree or Alloc finds a free run, Reserve and Release mark it,
 * Get gives a slot's address. A model is set up from its file and texture
 * pack (Model_SetupTexPack and its variants) and freed with all its slots
 * (Model_Free). Quest_EmptySetupHook, an empty function, sits between the
 * frees and the pools.
 */
#include "common.h"
#include "model.h"
#include "libc.h"

/* A slot of the material pool, 80 bytes: a model's ModelMat array
 * (Model.mats) starts at its first slot and runs on through the others. */
typedef struct MaterialSlot {
    u8 data[80];
} MaterialSlot;

extern u8 gBoneUsed[BONE_POOL_COUNT];            /* one per bone slot: in use */
extern u8 gMaterialSlotUsed[MATERIAL_POOL_COUNT]; /* one per material slot: in use */
extern u8 gMatSetUsed[MATSET_POOL_COUNT];        /* one per material set slot: in use */
extern u8 gModelUsed[MODEL_POOL_COUNT];          /* one per model slot: in use */
extern ModelMatSet *gMatSets;
extern MaterialSlot *gMaterials;
extern EmBone *gBones;
extern Model *gModels;

/* unit's own declaration: declared as this file's callers use it; with the definition's void (s32 a, s32 b), Model_Free compiles differently */
void Tex_FreeRange(s32 a, s16 b);
/* unit's own declaration: s16 b (the definition, takes s32); with s32 Model_SetupTexPack, B and Ex differ by 15 words each */
s32 Tex_LoadPack(s32 a, s16 b, s32 c);
/* unit's own declaration: declared as this file's callers use it; with the definition's void (Model *m, s32 arg1, u8 slot, s32 kind), Model_SetupTexPackEx compiles differently */
void Model_InitMatSetsSlot(Model *m, s32 a, s32 b, s32 c);

/*
 * Sets up model slot idx from modelFile, with the texture pack packId
 * loaded to texture slot slot, and its material sets with flags (mode 0).
 * Returns the model, or NULL when its materials don't load.
 */
Model *Model_SetupTexPack(s16 idx, s32 modelFile, s32 slot, s32 packId, s32 flags)
{
    Model *model = Model_Get(idx);

    model->active = 1;
    model->isCopy = 0;
    model->unk01 = 0;
    model->file = (u8 *)modelFile;
    model->texSlot = slot;
    model->texCount = Tex_LoadPack(packId, slot, 0);
    if (Model_LoadMaterials(model, modelFile) == -1) {
        return NULL;
    }
    Model_InitMatSets(model, flags, 0, 0);
    return model;
}

/* As Model_SetupTexPack, with the material sets in mode 2. */
Model *Model_SetupTexPackB(s16 idx, s32 modelFile, s32 slot, s32 packId, s32 flags)
{
    Model *model = Model_Get(idx);

    model->active = 1;
    model->isCopy = 0;
    model->unk01 = 0;
    model->file = (u8 *)modelFile;
    model->texSlot = slot;
    model->texCount = Tex_LoadPack(packId, slot, 0);
    if (Model_LoadMaterials(model, modelFile) == -1) {
        return NULL;
    }
    Model_InitMatSets(model, flags, 2, 0);
    return model;
}

/*
 * As Model_SetupTexPack, with the material sets placed from slot matSlot
 * (Model_InitMatSetsSlot).
 */
Model *Model_SetupTexPackEx(s16 idx, s32 modelFile, s32 slot, s32 packId, s32 flags, s32 matSlot)
{
    Model *model = Model_Get(idx);

    model->active = 1;
    model->isCopy = 0;
    model->unk01 = 0;
    model->file = (u8 *)modelFile;
    model->texSlot = slot;
    model->texCount = Tex_LoadPack(packId, slot, 0);
    if (Model_LoadMaterials(model, modelFile) == -1) {
        return NULL;
    }
    Model_InitMatSetsSlot(model, flags, matSlot, 2);
    return model;
}

/*
 * Frees the handle of each of count material sets that has one (id not
 * -1).
 */
void MatSet_FreeHandles(ModelMatSet *sets, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        if (sets[i].id != -1) {
            Model_FreeHandle(sets[i].id);
        }
    }
}

/*
 * Frees each part's skeleton copy that a model has (copies: one per part,
 * 0 none).
 */
void Model_FreePartCopies(Model *model, s32 *copies)
{
    s32 i;

    for (i = 0; i < model->partCount; i++, copies++) {
        if (*copies != 0) {
            Model_FreeBlock((void *)*copies);
        }
    }
}

/*
 * Frees model slot idx: a copy releases only its bones (Model_FreeCopy);
 * an active model releases its materials, bones and material sets, the
 * sets' handles, its textures and its parts' skeleton copies, and is
 * cleared.
 */
void Model_Free(s32 idx)
{
    Model *model = Model_Get(idx);
    s32 textures;

    if (model->isCopy != 0) {
        Model_FreeCopy(idx);
    } else if (model->active != 0) {
        Material_Release(model->matFirst, model->matCount);
        Bone_Release(model->boneFirst, model->boneCount);
        MatSet_Release(model->matSetFirst, model->matSetCount);
        MatSet_FreeHandles(model->matSets, model->matSetCount);
        textures = model->texCount;
        if (textures != 0) {
            Tex_FreeRange(model->texSlot, textures);
        }
        Model_FreePartCopies(model, model->partSkels);
        memset(model, 0, sizeof(Model));
    }
}

/* Frees a model copy in slot idx: releases its bones and clears it. */
void Model_FreeCopy(s32 idx)
{
    Model *model = Model_Get(idx);

    if (model->active != 0) {
        Bone_Release(model->boneFirst, model->boneCount);
        memset(model, 0, sizeof(Model));
    }
}

/* Frees every model slot. */
void Model_FreeAll(void)
{
    s32 i;

    for (i = 0; i < MODEL_POOL_COUNT; i++) {
        Model_Free(i);
    }
}

void Quest_EmptySetupHook(void)
{
}

/*
 * Finds count free material slots in a row: the first free slot, then
 * the sum of the count flags from there; skips ahead by count while any is
 * in use. Returns the first slot, or -1 when none is left.
 * The forms are kept for the match: `for (i = i; ...)` makes the compiler
 * rotate the search loop as the game's code does, and `j = sum = 0` keeps
 * the sum loop rolled with j not folded to 0; so does reusing j for the
 * flag tested.
 */
s32 Material_FindFree(s32 count)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (count == 0) {
        return 0;
    }
    while (1) {
        for (i = i; i < MATERIAL_POOL_COUNT; i++) {
            j = gMaterialSlotUsed[i];
            if (j == 0) {
                break;
            }
        }
        if (i >= MATERIAL_POOL_COUNT) {
            return -1;
        }
        if (i + count > MATERIAL_POOL_COUNT) {
            return -1;
        }
        for (j = sum = 0; j < count; j++) {
            sum += gMaterialSlotUsed[i + j];
        }
        if (sum == 0) {
            break;
        }
        i += count;
    }
    return i;
}

/* The address of material slot i. */
MaterialSlot *Material_Get(s32 i)
{
    return (MaterialSlot *)((s32)gMaterials + i * 80);
}

/* Marks count material slots from first in use. */
void Material_Reserve(s32 first, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        gMaterialSlotUsed[first + i] = 1;
    }
}

/* Marks count material slots from first free. */
void Material_Release(s32 first, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        gMaterialSlotUsed[first + i] = 0;
    }
}

/* As Material_FindFree, for the bone pool. */
s32 Bone_Alloc(s32 count)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (count == 0) {
        return 0;
    }
    while (1) {
        for (i = i; i < BONE_POOL_COUNT; i++) {
            j = gBoneUsed[i];
            if (j == 0) {
                break;
            }
        }
        if (i >= BONE_POOL_COUNT) {
            return -1;
        }
        if (i + count > BONE_POOL_COUNT) {
            return -1;
        }
        for (j = sum = 0; j < count; j++) {
            sum += gBoneUsed[i + j];
        }
        if (sum == 0) {
            break;
        }
        i += count;
    }
    return i;
}

/* The address of bone slot i. */
EmBone *Bone_Get(s32 i)
{
    return (EmBone *)((s32)gBones + i * 400);
}

/* Marks count bone slots from first in use. */
void Bone_Reserve(s32 first, s32 count)
{
    s32 i;

    if (count != 0) {
        for (i = 0; i < count; i++) {
            gBoneUsed[first + i] = 1;
        }
    }
}

/* Marks count bone slots from first free. */
void Bone_Release(s32 first, s32 count)
{
    s32 i;

    if (count != 0) {
        for (i = 0; i < count; i++) {
            gBoneUsed[first + i] = 0;
        }
    }
}

/*
 * As Material_FindFree, for the material set pool; j holds i + count for
 * the bound test.
 */
s32 MatSet_FindFree(s32 count)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (count == 0) {
        return 0;
    }
    while (1) {
        for (i = i; i < MATSET_POOL_COUNT; i++) {
            if (gMatSetUsed[i] == 0) {
                break;
            }
        }
        if (i >= MATSET_POOL_COUNT) {
            return -1;
        }
        j = i + count;
        if (j > MATSET_POOL_COUNT) {
            return -1;
        }
        for (j = sum = 0; j < count; j++) {
            sum += gMatSetUsed[i + j];
        }
        if (sum == 0) {
            break;
        }
        i += count;
    }
    return i;
}

/* The address of material set slot i. */
ModelMatSet *MatSet_Get(s32 i)
{
    return (ModelMatSet *)((s32)gMatSets + i * 140);
}

/* Marks count material set slots from first in use. */
void MatSet_Reserve(s32 first, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        gMatSetUsed[first + i] = 1;
    }
}

/* Marks count material set slots from first free. */
void MatSet_Release(s32 first, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        gMatSetUsed[first + i] = 0;
    }
}

/*
 * As Material_FindFree, for the model pool; j holds i + count for the
 * bound test.
 */
s32 Model_Alloc(s32 count)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (count == 0) {
        return 0;
    }
    while (1) {
        for (i = i; i < MODEL_POOL_COUNT; i++) {
            if (gModelUsed[i] == 0) {
                break;
            }
        }
        if (i >= MODEL_POOL_COUNT) {
            return -1;
        }
        j = i + count;
        if (j > MODEL_POOL_COUNT) {
            return -1;
        }
        for (j = sum = 0; j < count; j++) {
            sum += gModelUsed[i + j];
        }
        if (sum == 0) {
            break;
        }
        i += count;
    }
    return i;
}

Model *Model_Get(s32 i)
{
    return (Model *)((s32)gModels + (i << 7));
}

/* Marks count model slots from first in use. */
void Model_Reserve(s32 first, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        gModelUsed[first + i] = 1;
    }
}

/* Marks count model slots from first free. */
void Model_Release(s32 first, s32 count)
{
    s32 i;

    for (i = 0; i < count; i++) {
        gModelUsed[first + i] = 0;
    }
}
