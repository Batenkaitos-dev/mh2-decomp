/*
 * The model pools of sub_main and the player's weapon model: four pools
 * of slots, each with an in-use flag per slot (gModelUsed, gMaterialSlotUsed,
 * gBoneUsed, gMatSetUsed): MODEL_POOL_COUNT models (Model), and the
 * materials, bones and material sets the models take in runs of slots. A
 * pool's FindFree or Alloc finds a free run, Reserve and Release mark it,
 * Get gives a slot's address. A model is set up from its file and texture
 * pack (Model_SetupTexPack and its variants) and freed with all its slots
 * (Model_Free). The player's weapon model is loaded and freed by weapon
 * kind.
 */
#include "common.h"
#include "syssettings.h"
#include "player.h"
#include "model.h"


/* A slot of the material pool, 0x50 bytes: a model's ModelMat array
 * (Model.mats) starts at its first slot and runs on through the others. */
typedef struct MaterialSlot {
    u8 data[0x50];
} MaterialSlot;


extern u8 gBoneUsed[BONE_POOL_COUNT];            /* one per bone slot: in use */
extern u8 gMaterialSlotUsed[MATERIAL_POOL_COUNT]; /* one per material slot: in use */
extern u8 gMatSetUsed[MATSET_POOL_COUNT];        /* one per material set slot: in use */
extern u8 gModelUsed[MODEL_POOL_COUNT];          /* one per model slot: in use */
extern PlRec gPlayerRecs[4][32];
extern ModelMatSet *gMatSets;
extern MaterialSlot *gMaterials;
extern EmBone *gBones;
extern Model *gModels;
extern s32 gModelPackIds[];

void *memset(void *p, s32 c, u32 n);
s32 Model_FreeHandle(u32 h);
s32 Model_FreeBlock(void *p);
/* unit's own declaration: declared as this file's callers use it; with the definition's void (s32 a, s32 b), Model_Free compiles differently */
void Tex_FreeRange(s32 a, s16 b);
/* unit's own declaration: s16 b (the definition, takes s32); with s32 Model_SetupTexPack, B and Ex differ by 15 words each */
s32 Tex_LoadPack(s32 a, s16 b, s32 c);
void Load_WeaponModelPack(s32 kind);
void Model_InitMatSets(Model *m, s32 a, s32 b, s32 c);
/* unit's own declaration: declared as this file's callers use it; with the definition's void (Model *m, s32 arg1, u8 slot, s32 kind), Model_SetupTexPackEx compiles differently */
void Model_InitMatSetsSlot(Model *m, s32 a, s32 b, s32 c);
s32 Model_LoadMaterials(Model *m, s32 a);
/* unit's own declaration: declared as this file's callers use it; with the definition's Model *(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c), Player_LoadWeaponModel compiles differently */
Model *Model_SetupWithPack(s16 id, s32 a, s16 b, s32 c, s32 d, s32 e);
/* unit's own declaration: declared as this file's callers use it; with the definition's Model *(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c, s32 d), Player_AllocWeaponModel compiles differently */
Model *Model_SetupWithPackEx(s16 id, s32 a, s16 b, s32 c, s32 d, s32 e, s32 f);
Model *Model_Get(s32 i);
void Model_FreeCopy(s32 i);
void Material_Release(s32 first, s32 n);
void Bone_Release(s32 first, s32 n);
void MatSet_Release(s32 first, s32 n);
void MatSet_FreeHandles(ModelMatSet *p, s32 n);
void Model_FreePartCopies(Model *m, s32 *p);
s32 Model_Alloc(s32 n);
void Model_Reserve(s32 first, s32 n);
void Model_Release(s32 first, s32 n);

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
    return (MaterialSlot *)((s32)gMaterials + i * 0x50);
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
    return (EmBone *)((s32)gBones + i * 0x190);
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
    return (ModelMatSet *)((s32)gMatSets + i * 0x8C);
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

/*
 * Sets up player playerIdx: its 32 part records (gPlayerRecs, by its
 * index), no look parts (0xFF), no torso alternative, no weapon model
 * (0xFFFF).
 */
void Player_InitPartTable(s32 playerIdx)
{
    Player *pl = &gPlayers[playerIdx];
    PlRec *rec = gPlayerRecs[pl->index];
    s32 i;

    for (i = 0; i < 32; i++) {
        pl->partRecs[i] = rec++;
    }
    pl->lookPartId[0] = 0xFF;
    pl->lookPartId[1] = 0xFF;
    pl->lookPartId[2] = 0xFF;
    pl->lookPartId[3] = 0xFF;
    pl->lookPartId[4] = 0xFF;
    pl->lookPartId[5] = 0xFF;
    pl->headAltModel = 0;
    pl->weaponModelKind = 0xFFFF;
    pl->index = playerIdx;
    pl->unk4C6 = 0;
}

/*
 * Loads the weapon model of kind for player playerIdx, unless already
 * loaded: the kind's model pack, then a model slot set up from the data
 * buffer, with its textures in slot playerIdx + 0x21.
 */
void Player_LoadWeaponModel(s32 kind, s32 playerIdx)
{
    Player *pl = &gPlayers[playerIdx];
    s32 modelIdx;

    if (pl->weaponModelKind == 0xFFFF || pl->weaponModelKind != (u16)kind) {
        Load_WeaponModelPack(kind);
        pl->weaponModelKind = kind;
        modelIdx = Model_Alloc(1);
        if (modelIdx >= 0) {
            pl->weaponModelSlot = modelIdx;
            pl->weaponModelSlotCount = 1;
            pl->weaponModel = Model_Get(modelIdx);
            Model_Reserve(modelIdx, 1);
            Model_SetupWithPack(modelIdx, (*(s32 *)&gSysSettings.dataBuf), playerIdx + 0x21, gModelPackIds[kind], 0x900, 1);
        }
    }
}

/*
 * As Player_LoadWeaponModel without the check and the pack's loading: the
 * model is set up from modelFile, with extra passed on to
 * Model_SetupWithPackEx.
 */
void Player_AllocWeaponModel(s32 kind, s32 playerIdx, u8 *modelFile, u8 *extra)
{
    Player *pl = &gPlayers[playerIdx];
    s32 modelIdx;

    pl->weaponModelKind = kind;
    modelIdx = Model_Alloc(1);
    if (modelIdx >= 0) {
        pl->weaponModelSlot = modelIdx;
        pl->weaponModelSlotCount = 1;
        pl->weaponModel = Model_Get(modelIdx);
        Model_Reserve(modelIdx, 1);
        Model_SetupWithPackEx(modelIdx, (s32) modelFile, playerIdx + 0x21, gModelPackIds[kind], 0x900, 1, (s32) extra);
    }
}

/* Frees the player's weapon model slots and forgets its kind. */
void Player_FreeWeaponModel(Player *pl)
{
    s32 i;

    for (i = pl->weaponModelSlot; i < pl->weaponModelSlot + pl->weaponModelSlotCount; i++) {
        Model_Free(i);
        if (gModelUsed[i] != 0) {
            Model_Release(i, 1);
        }
    }
    pl->weaponModelKind = 0xFFFF;
    pl->weaponModel = NULL;
    pl->weaponModelSlot = 0;
    pl->weaponModelSlotCount = 0;
}

/*
 * Player_FreeWeaponModel when the loaded weapon model is not of the
 * player's weapon kind.
 */
void Player_FreeWeaponModelIfChanged(Player *pl)
{
    s32 i;

    if (pl->weaponModelKind != pl->weaponKind) {
        for (i = pl->weaponModelSlot; i < pl->weaponModelSlot + pl->weaponModelSlotCount; i++) {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
        }
        pl->weaponModelKind = 0xFFFF;
        pl->weaponModel = NULL;
        pl->weaponModelSlot = 0;
        pl->weaponModelSlotCount = 0;
    }
}
