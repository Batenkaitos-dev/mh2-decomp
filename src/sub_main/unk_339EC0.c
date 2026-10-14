#include "common.h"
#include "syssettings.h"
#include "player.h"
#include "model.h"


typedef struct Obj50 {
    u8 unk00[0x50];
} Obj50;


extern u8 gBoneUsed[0x800];    /* one per bone slot: in use */
extern u8 gMaterialSlotUsed[0x400];    /* one per 0x50-byte slot: in use */
extern u8 gMatSetUsed[0x180];    /* one per 0x8C-byte slot: in use */
extern u8 gModelUsed[0x80];     /* one per model slot: in use */
extern PlRec gPlayerRecs[4][32];
extern ModelMatSet *gMatSets;
extern Obj50 *gMaterials;
extern EmBone *gBones;
extern Model *gModels;
extern s32 gModelPackIds[];

void *memset(void *p, s32 c, u32 n);
s32 Model_FreeHandle(u32 h);
s32 Model_FreeBlock(void *p);
/* unit's own declaration: with the definition's void (s32 a, s32 b) (sub_main/unk_333D70.c), Model_Free: 21 of 49 words differ */
void Tex_FreeRange(s32 a, s16 b);
/* unit's own declaration: s16 b (the definition, unk_333D70.c, takes s32); with s32 Model_SetupTexPack, B and Ex differ by 15 words each */
s32 Tex_LoadPack(s32 a, s16 b, s32 c);
void Load_WeaponModelPack(s32 kind);
void Model_InitMatSets(Model *m, s32 a, s32 b, s32 c);
/* unit's own declaration: with the definition's void (Model *m, s32 arg1, u8 slot, s32 kind) (sub_main/unk_335040.c), Model_SetupTexPackEx: 2 of 50 words differ */
void Model_InitMatSetsSlot(Model *m, s32 a, s32 b, s32 c);
s32 Model_LoadMaterials(Model *m, s32 a);
/* unit's own declaration: with the definition's Model *(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c) (sub_main/unk_335040.c), Player_LoadWeaponModel: 23 of 59 words differ */
Model *Model_SetupWithPack(s16 id, s32 a, s16 b, s32 c, s32 d, s32 e);
/* unit's own declaration: with the definition's Model *(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c, s32 d) (sub_main/unk_335040.c), Player_AllocWeaponModel: 24 of 57 words differ */
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

Model *Model_SetupTexPack(s16 idx, s32 f, s32 slot, s32 id, s32 flags)
{
    Model *m = Model_Get(idx);

    m->active = 1;
    m->unk75 = 0;
    m->unk01 = 0;
    m->file = (u8 *)f;
    m->texSlot = slot;
    m->texCount = Tex_LoadPack(id, slot, 0);
    if (Model_LoadMaterials(m, f) == -1) {
        return NULL;
    }
    Model_InitMatSets(m, flags, 0, 0);
    return m;
}

Model *Model_SetupTexPackB(s16 idx, s32 f, s32 slot, s32 id, s32 flags)
{
    Model *m = Model_Get(idx);

    m->active = 1;
    m->unk75 = 0;
    m->unk01 = 0;
    m->file = (u8 *)f;
    m->texSlot = slot;
    m->texCount = Tex_LoadPack(id, slot, 0);
    if (Model_LoadMaterials(m, f) == -1) {
        return NULL;
    }
    Model_InitMatSets(m, flags, 2, 0);
    return m;
}

Model *Model_SetupTexPackEx(s16 idx, s32 f, s32 slot, s32 id, s32 flags, s32 matSlot)
{
    Model *m = Model_Get(idx);

    m->active = 1;
    m->unk75 = 0;
    m->unk01 = 0;
    m->file = (u8 *)f;
    m->texSlot = slot;
    m->texCount = Tex_LoadPack(id, slot, 0);
    if (Model_LoadMaterials(m, f) == -1) {
        return NULL;
    }
    Model_InitMatSetsSlot(m, flags, matSlot, 2);
    return m;
}

void MatSet_FreeHandles(ModelMatSet *p, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        if (p[i].id != -1) {
            Model_FreeHandle(p[i].id);
        }
    }
}

void Model_FreePartCopies(Model *m, s32 *p)
{
    s32 i;

    for (i = 0; i < m->partCount; i++, p++) {
        if (*p != 0) {
            Model_FreeBlock((void *)*p);
        }
    }
}

void Model_Free(s32 idx)
{
    Model *m = Model_Get(idx);
    s32 t;

    if (m->unk75 != 0) {
        Model_FreeCopy(idx);
    } else if (m->active != 0) {
        Material_Release(m->matFirst, m->matCount);
        Bone_Release(m->boneFirst, m->boneCount);
        MatSet_Release(m->matSetFirst, m->matSetCount);
        MatSet_FreeHandles(m->matSets, m->matSetCount);
        t = m->texCount;
        if (t != 0) {
            Tex_FreeRange(m->texSlot, t);
        }
        Model_FreePartCopies(m, m->partSkels);
        memset(m, 0, 0x80);
    }
}

void Model_FreeCopy(s32 idx)
{
    Model *m = Model_Get(idx);

    if (m->active != 0) {
        Bone_Release(m->boneFirst, m->boneCount);
        memset(m, 0, 0x80);
    }
}

void Model_FreeAll(void)
{
    s32 i;

    for (i = 0; i < 0x80; i++) {
        Model_Free(i);
    }
}

void func_0033A360(void)
{
}

/* Finds n free slots in a row (0x400 slots of 0x50 bytes): the first free
 * slot, then the sum of the n flags from there; skips ahead by n while any is
 * in use. `for (i = i; ...)` makes MWCC rotate the search loop as the game
 * does, and `j = sum = 0` keeps the sum loop rolled with j not folded to 0, as
 * in the game; so does reusing j for the flag tested. */
s32 Material_FindFree(s32 n)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (n == 0) {
        return 0;
    }
    while (1) {
        for (i = i; i < 0x400; i++) {
            j = gMaterialSlotUsed[i];
            if (j == 0) {
                break;
            }
        }
        if (i >= 0x400) {
            return -1;
        }
        if (i + n > 0x400) {
            return -1;
        }
        for (j = sum = 0; j < n; j++) {
            sum += gMaterialSlotUsed[i + j];
        }
        if (sum == 0) {
            break;
        }
        i += n;
    }
    return i;
}

Obj50 *Material_Get(s32 i)
{
    return (Obj50 *)((s32)gMaterials + i * 0x50);
}

void Material_Reserve(s32 first, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gMaterialSlotUsed[first + i] = 1;
    }
}

void Material_Release(s32 first, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gMaterialSlotUsed[first + i] = 0;
    }
}

/* As Material_FindFree (0x800 bone slots). */
s32 Bone_Alloc(s32 n)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (n == 0) {
        return 0;
    }
    while (1) {
        for (i = i; i < 0x800; i++) {
            j = gBoneUsed[i];
            if (j == 0) {
                break;
            }
        }
        if (i >= 0x800) {
            return -1;
        }
        if (i + n > 0x800) {
            return -1;
        }
        for (j = sum = 0; j < n; j++) {
            sum += gBoneUsed[i + j];
        }
        if (sum == 0) {
            break;
        }
        i += n;
    }
    return i;
}

EmBone *Bone_Get(s32 i)
{
    return (EmBone *)((s32)gBones + i * 0x190);
}

void Bone_Reserve(s32 first, s32 n)
{
    s32 i;

    if (n != 0) {
        for (i = 0; i < n; i++) {
            gBoneUsed[first + i] = 1;
        }
    }
}

void Bone_Release(s32 first, s32 n)
{
    s32 i;

    if (n != 0) {
        for (i = 0; i < n; i++) {
            gBoneUsed[first + i] = 0;
        }
    }
}

/* As Material_FindFree (0x180 slots of 0x8C bytes); j holds i + n for the bound test. */
s32 MatSet_FindFree(s32 n)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (n == 0) {
        return 0;
    }
    while (1) {
        for (i = i; i < 0x180; i++) {
            if (gMatSetUsed[i] == 0) {
                break;
            }
        }
        if (i >= 0x180) {
            return -1;
        }
        j = i + n;
        if (j > 0x180) {
            return -1;
        }
        for (j = sum = 0; j < n; j++) {
            sum += gMatSetUsed[i + j];
        }
        if (sum == 0) {
            break;
        }
        i += n;
    }
    return i;
}

ModelMatSet *MatSet_Get(s32 i)
{
    return (ModelMatSet *)((s32)gMatSets + i * 0x8C);
}

void MatSet_Reserve(s32 first, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gMatSetUsed[first + i] = 1;
    }
}

void MatSet_Release(s32 first, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gMatSetUsed[first + i] = 0;
    }
}

/* As Material_FindFree (0x80 model slots); j holds i + n for the bound test. */
s32 Model_Alloc(s32 n)
{
    s32 i;
    s32 sum;
    s32 j;

    i = 0;
    if (n == 0) {
        return 0;
    }
    while (1) {
        for (i = i; i < 0x80; i++) {
            if (gModelUsed[i] == 0) {
                break;
            }
        }
        if (i >= 0x80) {
            return -1;
        }
        j = i + n;
        if (j > 0x80) {
            return -1;
        }
        for (j = sum = 0; j < n; j++) {
            sum += gModelUsed[i + j];
        }
        if (sum == 0) {
            break;
        }
        i += n;
    }
    return i;
}

Model *Model_Get(s32 i)
{
    return (Model *)((s32)gModels + (i << 7));
}

void Model_Reserve(s32 first, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gModelUsed[first + i] = 1;
    }
}

void Model_Release(s32 first, s32 n)
{
    s32 i;

    for (i = 0; i < n; i++) {
        gModelUsed[first + i] = 0;
    }
}

void Player_InitPartTable(s32 idx)
{
    Player *p = &gPlayers[idx];
    PlRec *r = gPlayerRecs[p->index];
    s32 i;

    for (i = 0; i < 32; i++) {
        p->partRecs[i] = r++;
    }
    p->lookPartId[0] = 0xFF;
    p->lookPartId[1] = 0xFF;
    p->lookPartId[2] = 0xFF;
    p->lookPartId[3] = 0xFF;
    p->lookPartId[4] = 0xFF;
    p->lookPartId[5] = 0xFF;
    p->torsoAltModel = 0;
    p->weaponModelKind = 0xFFFF;
    p->index = idx;
    p->unk4C6 = 0;
}

void Player_LoadWeaponModel(s32 kind, s32 pl)
{
    Player *p = &gPlayers[pl];
    s32 id;

    if (p->weaponModelKind == 0xFFFF || p->weaponModelKind != (u16)kind) {
        Load_WeaponModelPack(kind);
        p->weaponModelKind = kind;
        id = Model_Alloc(1);
        if (id >= 0) {
            p->weaponModelSlot = id;
            p->weaponModelSlotCount = 1;
            p->weaponModel = Model_Get(id);
            Model_Reserve(id, 1);
            Model_SetupWithPack(id, (*(s32 *)&gSysSettings.dataBuf), pl + 0x21, gModelPackIds[kind], 0x900, 1);
        }
    }
}

void Player_AllocWeaponModel(s32 kind, s32 pl, u8 *buf, u8 *d)
{
    Player *p = &gPlayers[pl];
    s32 id;

    p->weaponModelKind = kind;
    id = Model_Alloc(1);
    if (id >= 0) {
        p->weaponModelSlot = id;
        p->weaponModelSlotCount = 1;
        p->weaponModel = Model_Get(id);
        Model_Reserve(id, 1);
        Model_SetupWithPackEx(id, (s32) buf, pl + 0x21, gModelPackIds[kind], 0x900, 1, (s32) d);
    }
}

void Player_FreeWeaponModel(Player *p)
{
    s32 i;

    for (i = p->weaponModelSlot; i < p->weaponModelSlot + p->weaponModelSlotCount; i++) {
        Model_Free(i);
        if (gModelUsed[i] != 0) {
            Model_Release(i, 1);
        }
    }
    p->weaponModelKind = 0xFFFF;
    p->weaponModel = NULL;
    p->weaponModelSlot = 0;
    p->weaponModelSlotCount = 0;
}

void Player_FreeWeaponModelIfChanged(Player *p)
{
    s32 i;

    if (p->weaponModelKind != p->weaponKind) {
        for (i = p->weaponModelSlot; i < p->weaponModelSlot + p->weaponModelSlotCount; i++) {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
        }
        p->weaponModelKind = 0xFFFF;
        p->weaponModel = NULL;
        p->weaponModelSlot = 0;
        p->weaponModelSlotCount = 0;
    }
}
