#include "common.h"
#include "syssettings.h"
#include "em.h"
#include "scene.h"
#include "model.h"
#include "save.h"
#include "player.h"
#include "task.h"
#include "area.h"

/* prototypes */
void Model_Free(s32 idx);
void Model_Release(s32 i, s32 a);
extern s32 gAreaSubModelId;
extern u8 gModelUsed[];
s32 Armor_LacksSexModel(s32 id, u8 female);
s32 Model_Alloc(s32 n);
Model *Model_Get(s32 i);
void Model_Reserve(s32 i, s32 a);
void EmSlot_Clear(s16 i);
void Motion_FreeEmSlot(s16 i);
void Load_EmSlotModelPack(s32 kind);
/* unit's own declaration: with the definition's Model *(s16 idx, s32 arg1, s32 arg2, s32 arg3, s32 arg4) (sub_main/unk_339EC0.c), EmSlot_LoadModel: 27 of 101 words differ */
Model *Model_SetupTexPackB(s16 model, s32 work, s16 a, s32 b, s32 c);
void Anim_LoadEmSlot(s32 kind, s32 var, s32 slot);
extern s32 gModelLoadBuf;
extern s32 gEmSlotTexFiles[];
extern s32 gEmSlotAnimFiles[];
/* Per monster slot, the kind and the variant of its model (the two halves carry separate labels). */
extern s32 gEmSlotModelIds[][2];
extern s32 D_005B3624[][2];
/* end of prototypes */


/* The models loaded for the stage: a flag, then a range of model slots. */
typedef struct StageModels {
    u8 loaded;                  /* 0x0 */
    u8 pad1[7];
    s16 count;                  /* 0x8 */
    s16 first;                  /* 0xA */
    s32 unkC;                   /* 0xC */
} StageModels;
extern StageModels gEquipPreview;

extern s32 gAreaSubModelIds[4];     /* four more model slots, -1 when unused */

/* Equip_GetModelId: the model id of an equipment part (kind: 0 head, 2-5 the other
 * armour parts, 6 blade, 7 gun) for the sex (0 male, else female). */

u16 Equip_GetModelId(u16 kind, u16 id, u16 female)
{
    switch (kind) {
    case 0:
        if (female == 0) {
            if (gArmorLegsData[id].flags & 1) {
                return gArmorLegsData[id].modelMale;
            }
            return 0;
        }
        if (gArmorLegsData[id].flags & 2) {
                return gArmorLegsData[id].modelFemale;
            }
            return 0;
    case 4:
        if (female == 0) {
            if (gArmorArmsData[id].flags & 1) {
                return gArmorArmsData[id].modelMale;
            }
            return 0;
        }
        if (gArmorArmsData[id].flags & 2) {
                return gArmorArmsData[id].modelFemale;
            }
            return 0;
    case 3:
        if (female == 0) {
            if (gArmorBodyData[id].flags & 1) {
                return gArmorBodyData[id].modelMale;
            }
            return 0;
        }
        if (gArmorBodyData[id].flags & 2) {
                return gArmorBodyData[id].modelFemale;
            }
            return 0;
    case 5:
        if (female == 0) {
            if (gArmorWaistData[id].flags & 1) {
                return gArmorWaistData[id].modelMale;
            }
            return 0;
        }
        if (gArmorWaistData[id].flags & 2) {
                return gArmorWaistData[id].modelFemale;
            }
            return 0;
    case 2:
        if (female == 0) {
            if (gArmorHeadData[id].flags & 1) {
                return gArmorHeadData[id].modelMale;
            }
            return 0;
        }
        if (gArmorHeadData[id].flags & 2) {
                return gArmorHeadData[id].modelFemale;
            }
            return 0;
    case 6:
        return gBladeData[id].kind;
    case 7:
        return gGunData[id].kind;
    }
    return 0;
}

extern u16 gPartIdMax[2][7];
extern u16 gArmourIdMax[];
extern u8 gFaceBodyTypeMale[];
extern u8 gFaceBodyTypeFemale[];

/* Builds the six armour part ids of a look (ids) from the equipment records a (12 bytes each): an
 * id over its slot's gArmourIdMax limit is cleared, then each part's model id is taken from its
 * armour table by the gender byte p->gender (case 1: the colour, clamped by gPartIdMax, and *b from
 * gFaceBodyTypeMale/gFaceBodyTypeFemale), and an id at or over the gPartIdMax limit becomes 0. Round 23: the
 * last test through a pointer q = &ids[n] (a declared local) gives the game's n in s0. */
void Player_BuildPartIds(Player *p, u8 *ids, EquipItem *a, u8 *b)
{
    s16 i;
    u16 *lim;
    EquipItem *e;
    u8 v;
    u16 id;
    u8 *q;

    lim = gArmourIdMax;
    e = a;
    for (i = 0; i < 6; i++, lim++, e++) {
        s32 n;

        n = i;
        if (n != 1 && e->id > *lim) {
            e->id = 0;
        }
        switch (n) {
        case 1:
            v = a[1].id;
            if (v <= 0 || gPartIdMax[p->gender][1] < v) {
                v = 1;
            }
            ids[1] = v - 1;
            if (p->gender == 0) {
                *b = gFaceBodyTypeMale[ids[1]];
            } else {
                *b = gFaceBodyTypeFemale[ids[1]];
            }
            break;
        case 0:
            if (p->gender == 0) {
                if (gArmorLegsData[a[0].id].flags & 1) {
                    ids[0] = gArmorLegsData[a[0].id].modelMale;
                } else {
                    ids[0] = 0;
                }
            } else {
                if (gArmorLegsData[a[0].id].flags & 2) {
                    ids[0] = gArmorLegsData[a[0].id].modelFemale;
                } else {
                    ids[0] = 0;
                }
            }
            break;
        case 3:
            if (p->gender == 0) {
                if (gArmorBodyData[a[3].id].flags & 1) {
                    ids[3] = gArmorBodyData[a[3].id].modelMale;
                } else {
                    ids[3] = 0;
                }
            } else {
                if (gArmorBodyData[a[3].id].flags & 2) {
                    ids[3] = gArmorBodyData[a[3].id].modelFemale;
                } else {
                    ids[3] = 0;
                }
            }
            break;
        case 4:
            if (p->gender == 0) {
                if (gArmorArmsData[a[4].id].flags & 1) {
                    ids[4] = gArmorArmsData[a[4].id].modelMale;
                } else {
                    ids[4] = 0;
                }
            } else {
                if (gArmorArmsData[a[4].id].flags & 2) {
                    ids[4] = gArmorArmsData[a[4].id].modelFemale;
                } else {
                    ids[4] = 0;
                }
            }
            break;
        case 5:
            if (p->gender == 0) {
                if (gArmorWaistData[a[5].id].flags & 1) {
                    ids[5] = gArmorWaistData[a[5].id].modelMale;
                } else {
                    ids[5] = 0;
                }
            } else {
                if (gArmorWaistData[a[5].id].flags & 2) {
                    ids[5] = gArmorWaistData[a[5].id].modelFemale;
                } else {
                    ids[5] = 0;
                }
            }
            break;
        case 2:
            id = a[2].id;
            if (id == 0) {
                ids[2] = p->hairStyle;
            } else if (p->gender == 0) {
                if (gArmorHeadData[id].flags & 1) {
                    if (gArmorHeadData[id].modelMale == 0) {
                        ids[2] = p->hairStyle;
                    } else {
                        ids[2] = gArmorHeadData[id].modelMale;
                    }
                } else {
                    ids[2] = p->hairStyle;
                }
            } else {
                if (gArmorHeadData[id].flags & 2) {
                    if (gArmorHeadData[id].modelFemale == 0) {
                        ids[2] = p->hairStyle;
                    } else {
                        ids[2] = gArmorHeadData[id].modelFemale;
                    }
                } else {
                    ids[2] = p->hairStyle;
                }
            }
            break;
        }
        if (n != 1) {
            q = &ids[n];
            if (*q >= ((u16 *)gPartIdMax)[p->gender * 7 + n]) {
                *q = 0;
            }
        }
    }
}

/* 0 when armour id (gArmorHeadData, 0x28-byte records) has the flag of the sex (1 male,
 * 2 female) and its byte for that sex (+0 male, +1 female) is set; else 1. */
s32 Armor_LacksSexModel(s32 id, u8 female)
{
    u8 mask;

    id = (u16)id;
    if (id == 0) {
        return 1;
    }
    if (female == 0) {
        mask = 1;
    } else {
        mask = 2;
    }
    if (mask & gArmorHeadData[id].flags) {
        if (female == 0) {
            if (gArmorHeadData[id].modelMale == 0) {
                return 1;
            }
        } else if (gArmorHeadData[id].modelFemale == 0) {
            return 1;
        }
    } else {
        return 1;
    }
    return 0;
}

/* 0 when the byte at +0x8A8 + slot (slots 0..5) is id; slot 2 also
 * depends on +0x8B0 and Armor_LacksSexModel (then compares with +0x30E). The
 * u8 of the switch is reused for the table byte (permuter). */
s32 Player_PartIdDiffers(Player *p, s32 slot, s32 id)
{
    u8 v = slot;

    switch (v) {
    case 2:
        if (p->torsoAltModel == 0) {
            if (Armor_LacksSexModel(p->equip[2].id, p->gender) == 0) {
                v = p->lookPartId[(u8)slot];
                if (v != 0xFF && v == (u8)id) {
                    return 0;
                }
            }
        } else if (Armor_LacksSexModel(p->equip[2].id, p->gender) == 1) {
            v = p->lookPartId[(u8)slot];
            if (v != 0xFF && v == p->hairStyle) {
                return 0;
            }
        }
        break;
    case 0:
    case 1:
    case 3:
    case 4:
    case 5:
        v = p->lookPartId[(u8)slot];
        if (v != 0xFF && v == (u8)id) {
            return 0;
        }
        break;
    }
    return 1;
}

extern s16 gLookPartMap0[];
extern s16 gLookPartMap3[];
extern s16 gLookPartMap4[];

/* Maps id through the table of kind 0, 3 or 4 (rows of 16 per sex, +0x11):
 * when id is in the row, the entry at +0x8BC * 4 + k of that row replaces it. */
s16 Look_MapPartId(Player *p, s16 kind, s16 id, u8 k)
{
    s16 i;
    s16 ret = id;

    switch (kind) {
    case 0:
        for (i = 0; i < 16; i++) {
            if (ret == gLookPartMap0[p->gender * 16 + i]) {
                ret = gLookPartMap0[k + p->gender * 16 + p->charType * 4];
                break;
            }
        }
        break;
    case 1:
    case 2:
    case 5:
        break;
    case 3:
        for (i = 0; i < 16; i++) {
            if (ret == gLookPartMap3[p->gender * 16 + i]) {
                ret = gLookPartMap3[k + p->gender * 16 + p->charType * 4];
                break;
            }
        }
        break;
    case 4:
        for (i = 0; i < 16; i++) {
            if (ret == gLookPartMap4[p->gender * 16 + i]) {
                ret = gLookPartMap4[k + p->gender * 16 + p->charType * 4];
                break;
            }
        }
        break;
    }
    return ret;
}

void Load_WeaponModelPack(s32 id);
void Load_EquipModelPack(s32 id, s32 female);
Model *Model_SetupWithPack(s16 idx, s32 a, s16 slot, s32 tex, s32 b, s32 c); /* unit's own declaration: with the definition's Model *(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c) (sub_main/unk_335040.c), EmSlot_LoadModels 36 of 138 words, Area_LoadSubModels 2 of 158 words differ */
void FxBlock_Clear(EquipFxSwing *p);
void Load_PartModelPack(s32 i, s32 j, s32 k);
/* unit's own declaration: with the definition's Model *(s16 idx, s32 a, s32 slot, s32 *pack, s32 b, s32 d, s32 c) (sub_main/unk_335040.c), Player_LoadPartModels: 48 of 164 words differ */
Model *Model_SetupLoadedPack(s16 idx, s32 a, s16 slot, s32 *pack, s32 b, s32 d, s32 c);
extern s32 *gPlayerLookFiles[];   /* per sex (7 kinds each), the model packs of each id (8 bytes apart) */
extern s16 D_005BAE08;
extern s16 D_005BAE0A;
extern u8 D_005BAD64;
extern u8 D_005BAD65;
extern u8 D_005BAD66;
extern u8 D_005BAD67;
extern u8 D_005BAD70;
extern s32 gEquipModelTexFilesM[];
extern s32 gEquipModelTexFilesF[];
extern s32 gModelPackIds[];

/* Loads the model of an equipment piece for the stage's preview: kind 0xFF (sex's
 * table), 6 and 7, or a part kind (2 with id 0: kind 6 of gSave.hairStyle). */
void StageModels_LoadEquip(u16 kind, u16 id, u8 female)
{
    StageModels *m = &gEquipPreview;
    s16 idx;
    s32 *pack;

    gEquipPreview.loaded = 0;
    D_005BAE08 = kind;
    D_005BAE0A = id;
    D_005BAD70 = female;
    D_005BAD64 = 0;
    D_005BAD65 = 0;
    D_005BAD66 = 0;
    D_005BAD67 = 0;
    switch (kind) {
    case 0xFF:
        Load_EquipModelPack(id, female);
        idx = Model_Alloc(1);
        if (idx < 0) {
            return;
        }
        m->first = idx;
        m->count = 1;
        m->unkC = (s32)Model_Get(idx);
        Model_Reserve(idx, 1);
        if (female == 0) {
            Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 0x166, gEquipModelTexFilesM[id], 0x900, 1);
        } else {
            Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 0x166, gEquipModelTexFilesF[id], 0x900, 1);
        }
        break;
    case 2:
        if (id == 0) {
            kind = 6;
            id = gSave.hairStyle;
        }
    default:
        Load_PartModelPack(id, kind, female);
        idx = Model_Alloc(1);
        if (idx < 0) {
            return;
        }
        m->first = idx;
        m->count = 1;
        m->unkC = (s32)Model_Get(idx);
        Model_Reserve(idx, 1);
        pack = gPlayerLookFiles[kind + female * 7];
        pack += id * 2;
        Model_SetupLoadedPack(idx, (*(s32 *)&gSysSettings.dataBuf), 0x166, pack, 0x900,
                      (u16)(female * 4), 1);
        if (kind == 2) {
            FxBlock_Clear((EquipFxSwing *) ((u8 *)m + 0x28));
        }
        break;
    case 6:
    case 7:
        Load_WeaponModelPack(id);
        idx = Model_Alloc(1);
        if (idx < 0) {
            return;
        }
        m->first = idx;
        m->count = 1;
        m->unkC = (s32)Model_Get(idx);
        Model_Reserve(idx, 1);
        Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 0x166, gModelPackIds[id], 0x900, 1);
        break;
    }
    m->loaded = 1;
}

/* Frees the stage's models (the slots first .. first + count). */
void StageModels_Free(void)
{
    StageModels *m = &gEquipPreview;
    s32 i;

    m->loaded = 0;
    if (m->count != 0) {
        for (i = m->first; i < m->first + m->count; i++) {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
        }
        m->unkC = 0;
        m->first = 0;
        m->count = 0;
    }
}

void Player_BuildPartIds(Player *p, u8 *ids, EquipItem *a, u8 *b);

/* Loads the models of the player's six parts (the body, then the equipment): maps each id
 * (+0x366) through Look_MapPartId into +0x8A8, and when Player_PartIdDiffers says it changed, loads its
 * pack and allocates a model slot for it. */
void Player_LoadPartModels(Player *p)
{
    s16 kind;
    s16 m;
    s16 id;
    s16 i;
    s32 *pack;

    Player_BuildPartIds(p, p->armourKind, p->equip, &p->lookVariant);
    for (i = 0; i < 6; i++) {
        id = Look_MapPartId(p, i, p->armourKind[i], p->lookVariant);
        if (i == 2 && Armor_LacksSexModel(p->equip[2].id, p->gender) == 1) {
            kind = 6;
        } else {
            kind = i;
        }
        if (Player_PartIdDiffers(p, (u8)i, (u8)id)) {
            p->lookPartId[i] = id;
            if (kind == 2) {
                p->torsoAltModel = 0;
            } else if (kind == 6) {
                p->torsoAltModel = 1;
            }
            Load_PartModelPack(id, kind, p->gender);
            m = Model_Alloc(1);
            if (m < 0) {
                return;
            }
            if (i == 0) {
                p->modelFirst = m;
                p->modelCount = 1;
                p->model = Model_Get(m);
            } else {
                p->partModelFirst[i] = m;
                p->partModelCount[i] = 1;
                p->partModels[i] = Model_Get(m);
            }
            Model_Reserve(m, 1);
            pack = gPlayerLookFiles[kind + p->gender * 7];
            pack += id * 2;
            if (i == 0) {
                Model_SetupLoadedPack(m, (*(s32 *)&gSysSettings.dataBuf), p->index * 12 + 0x2B + i * 2, pack, 0x900,
                (u16)(p->lookVariant + p->gender * 4), 2);
            } else {
                Model_SetupLoadedPack(m, (*(s32 *)&gSysSettings.dataBuf), p->index * 12 + 0x2B + i * 2, pack, 0x900,
                (u16)(p->lookVariant + p->gender * 4), 1);
            }
        }
    }
}

/* Frees a player's models: the body's slots and those of parts 1-5. */
void Player_FreeModels(Player *p)
{
    s32 i;
    s32 k;

    for (i = p->modelFirst; i < p->modelFirst + p->modelCount; i++) {
        Model_Free(i);
        if (gModelUsed[i] != 0) {
            Model_Release(i, 1);
        }
    }
    p->model = NULL;
    p->modelFirst = 0;
    p->modelCount = 0;
    p->lookPartId[0] = 0xFF;
    for (k = 1; k < 6; k++) {
        for (i = p->partModelFirst[k]; i < p->partModelFirst[k] + p->partModelCount[k]; i++) {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
        }
        p->partModels[k] = NULL;
        p->partModelFirst[k] = 0;
        p->partModelCount[k] = 0;
        p->lookPartId[k] = 0xFF;
    }
}

/* The player's six equipment records at +0x312 (0xC bytes each). */
typedef struct EquipRec {
    s16 v[6];
} EquipRec;

/* Frees the models of the player's parts whose id Look_MapPartId maps to something new
 * (Player_PartIdDiffers): the ids come from a copy of the equipment records. */
void Player_FreeChangedPartModels(Player *p)
{
    EquipRec recs[6];
    u8 ids[7];
    u8 k;
    s32 i;
    s32 j;

    recs[0] = ((EquipRec *)p->equip)[0];
    recs[1] = ((EquipRec *)p->equip)[1];
    recs[2] = ((EquipRec *)p->equip)[2];
    recs[3] = ((EquipRec *)p->equip)[3];
    recs[4] = ((EquipRec *)p->equip)[4];
    recs[5] = ((EquipRec *)p->equip)[5];
    Player_BuildPartIds(p, ids, (EquipItem *)recs, &k);
    if (Player_PartIdDiffers(p, 0, (u8)Look_MapPartId(p, 0, ids[0], k)) == 1) {
        for (i = p->modelFirst; i < p->modelFirst + p->modelCount; i++) {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
        }
        p->model = NULL;
        p->modelFirst = 0;
        p->modelCount = 0;
        p->lookPartId[0] = 0xFF;
    }
    for (j = 1; j < 6; j++) {
        if (Player_PartIdDiffers(p, (u8)j, (u8)Look_MapPartId(p, j, ids[j], k)) == 1) {
            for (i = p->partModelFirst[j]; i < p->partModelFirst[j] + p->partModelCount[j]; i++) {
                Model_Free(i);
                if (gModelUsed[i] != 0) {
                    Model_Release(i, 1);
                }
            }
            p->partModels[j] = NULL;
            p->partModelFirst[j] = 0;
            p->partModelCount[j] = 0;
            p->lookPartId[j] = 0xFF;
        }
    }
}

void Load_EmModelPack(s32 kind);
void Load_EmModelPack2(s32 kind);
void Anim_LoadEm(s32 arg0, s32 arg1);
Model *Model_SetupTexPackEx(s16 idx, s32 work, s16 id, s32 tex, s32 flags, u8 slot); /* unit's own declaration:
 * the definition takes s32 id and slot; EmSlot_LoadModels matches with s16 id (NON_MATCHING in round 29: 24 words with s16, 97 with s32) */
extern s32 gEmSlotTexPacks[];
extern s32 gEmSlotModelPacks[];
extern s32 gEmModelPackFiles[];

/*
 * Loads monster slot's models for the kind in gScene.emKind[slot] (none when 0): one
 * model (Model_SetupTexPackEx, id slot * 20 + 0xA3), and a second set (Model_SetupWithPack,
 * id + 0x10) when gEmModelPackFiles[kind] isn't -1 (else partCount -1). work and model are
 * reused for the second set.
 * The game keeps slot * 2 (s0) and the &emKind[slot] pointer (s1) across every block and
 * numbers both after MWCC's common subexpressions slot * 4 and slot * 20 (s3, s2), so slot
 * gets s5. Each path computes its own byte offset o = slot * sizeof(s16) (the byte-offset
 * form, as Kbd_DrawSpecialKey's), and emKind[slot]'s address is the integer sum slot + s
 * (as Font_ClearPage writes it), which gives the game's operand order (slot, s).
 * FAKEMATCH: kind is first set to the emKind array, for the test only, so that the slot
 * pointer is a later web, numbered after the common subexpressions as the game numbers it;
 * natural C numbers kind at its declaration (24 words). See docs/notes/nm/EmSlot_LoadModels.md.
 */
void EmSlot_LoadModels(s32 slot)
{
    s32 work;
    s32 model;
    s32 o;
    u8 *kind;
    SceneState *s;

    s = &gScene;
    kind = s->emKind;
    if (kind[slot] == 0) {
        o = slot * sizeof(s16);
        *(s16 *)((u8 *)s->modelFirst + o) = 0;
        *(s16 *)((u8 *)s->modelCount + o) = 0;
        s->models[slot] = NULL;
        return;
    }
    kind = (u8 *)(slot + (s32)s) + 0x2A;
    Load_EmModelPack(*kind);
    work = gModelLoadBuf;
    model = Model_Alloc(1);
    if (model < 0) {
        return;
    }
    o = slot * sizeof(s16);
    *(s16 *)((u8 *)s->modelFirst + o) = model;
    *(s16 *)((u8 *)s->modelCount + o) = 1;
    s->models[slot] = Model_Get(model);
    Model_Reserve(model, 1);
    Model_SetupTexPackEx(model, work, slot * 0x14 + 0xA3, gEmSlotTexPacks[*kind], 0x900, slot);
    Anim_LoadEm(slot, *kind);
    if (gEmModelPackFiles[*kind] != -1) {
        Load_EmModelPack2(*kind);
        work = gModelLoadBuf;
        model = Model_Alloc(1);
        if (model < 0) {
            *(s16 *)((u8 *)s->partFirst + o) = 0;
            *(s16 *)((u8 *)s->partCount + o) = -1;
            s->partModels[slot] = NULL;
            return;
        }
        *(s16 *)((u8 *)s->partFirst + o) = model;
        *(s16 *)((u8 *)s->partCount + o) = 1;
        s->partModels[slot] = Model_Get(model);
        Model_Reserve(model, 1);
        Model_SetupWithPack(model, work, slot * 0x14 + 0xB3, gEmSlotModelPacks[*kind], 0x900, 0);
        return;
    }
    *(s16 *)((u8 *)s->partFirst + o) = 0;
    *(s16 *)((u8 *)s->partCount + o) = -1;
    s->partModels[slot] = NULL;
}

/* Loads the model of kind (variant var) into monster slot, unless the slot holds it. */
void EmSlot_LoadModel(s32 kind, s32 var, s32 slot)
{
    SceneState *s;
    s32 *cur;
    s32 *curVar;
    s32 work;
    s32 model;

    cur = &gEmSlotModelIds[slot][0];
    s = &gScene;
    if (kind != *cur || var != D_005B3624[slot][0]) {
        if (*cur != -1) {
            EmSlot_Clear(slot);
        }
        curVar = &D_005B3624[slot][0];
        if (*curVar != -1 && slot != 0) {
            Motion_FreeEmSlot(slot);
        }
        *cur = kind;
        *curVar = var;
        Load_EmSlotModelPack(kind);
        work = (*(s32 *)&gSysSettings.dataBuf);
        model = Model_Alloc(1);
        if (model >= 0) {
            s->modelFirst[slot] = model;
            s->modelCount[slot] = 1;
            s->models[slot] = Model_Get(model);
            Model_Reserve(model, 1);
            Model_SetupTexPackB(model, work, slot * 0x14 + 0xA3, gEmSlotTexFiles[kind], 0x900);
            if (gEmSlotAnimFiles[kind] != 0) {
                Anim_LoadEmSlot(kind, var, slot);
            }
        }
    }
}

/* Clears (to -1) the first (which 0), second (1) or both (2) values of
 * entry i. */
void EmSlotModel_Clear(s16 i, s8 which)
{
    switch (which) {
    case 0:
        gEmSlotModelIds[i][0] = -1;
        break;
    case 1:
        D_005B3624[i][0] = -1;
        break;
    case 2:
        gEmSlotModelIds[i][0] = -1;
        D_005B3624[i][0] = -1;
        break;
    }
}

/* Clears slot i's kind and frees its models (modelFirst[i], modelCount[i] of them).
 * The bound first + n is not a local: MWCC hoists it out of the loop, and as a
 * value it creates it takes its saved register after the walker's. */
void EmSlot_Clear(s16 i)
{
    s16 j, first, n;

    EmSlotModel_Clear(i, 0);
    first = gScene.modelFirst[i];
    n = gScene.modelCount[i];
    for (j = first; j < first + n; j++) {
        Model_Free(j);
        if (gModelUsed[j] != 0) {
            Model_Release(j, 1);
        }
    }
}

/*
 * Frees monster slot i's models and its second model set, and marks both empty.
 * FAKEMATCH: the game numbers n (s1) and n * 2 (s0) after MWCC's other common
 * subexpressions: n * 2 after the first loop's extended j, n after the part pointers.
 * Contrived here: the (s16) cast on the count read's index keeps the first block's two
 * n * 2 apart, so the common n * 2 is created where the block after the loop reuses it.
 * Also needed: n set with an explicit cast and the last store indexed with i (n's
 * extension is then a common subexpression reused by that store; `s32 n = i;` numbers n
 * at its declaration), and the compiled-out check, which splits the block after the loop
 * so that each of its two reads uses n * 2 once and keeps the first block's value.
 * See docs/notes/nm/EmSlot_FreeModels.md.
 */
void EmSlot_FreeModels(s16 i)
{
    SceneState *s = &gScene;
    s16 j, first, cnt;
    s32 n = (s32)i;

    first = gScene.modelFirst[n];
    cnt = gScene.modelCount[(s16)n];
    for (j = first; j < first + cnt; j++) {
        Model_Free(j);
        if (gModelUsed[j] != 0) {
            Model_Release(j, 1);
        }
    }
    first = s->partFirst[n];
    if (first < 0 || first >= 0x80) {} /* compiled-out check */
    cnt = s->partCount[n];
    if (cnt != -1) {
        for (j = first; j < first + cnt; j++) {
            Model_Free(j);
            if (gModelUsed[j] != 0) {
                Model_Release(j, 1);
            }
        }
    }
    s->modelFirst[n] = 0;
    s->modelCount[n] = -1;
    s->models[n] = NULL;
    s->partFirst[n] = 0;
    s->partCount[n] = -1;
    s->partModels[n] = NULL;
    Motion_FreeEmSlot(i);
    s->emKind[i] = 0;
}

/* Frees the area models (slots first .. first + count) and the four in gAreaSubModelIds. */
void Area_FreeModels(void)
{
    s16 i;
    s16 k;
    s16 id;
    AreaState *a;

    i = gAreaState.modelSlot;
    a = &gAreaState;
    if (i < i + gAreaState.modelCount) {
        do {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
            i++;
        } while (i < a->modelSlot + a->modelCount);
    }
    for (k = 0; k < 4; k++) {
        if (gAreaSubModelIds[k] != -1) {
            id = gAreaSubModelIds[k];
            Model_Free(id);
            if (gModelUsed[id] != 0) {
                Model_Release(id, 1);
            }
        }
    }
}

s16 Area_SubModelCount(u16 area);
s32 LobbyWorld_GetSeason(void);
u8 Village_FacilityLevel(s32 k);
void Load_AreaSubModelPack(s32 id, s32 sub);
Model *Model_SetupWithPack(s16 idx, s32 a, s16 slot, s32 tex, s32 b, s32 c); /* unit's own declaration: with the definition's Model *(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c) (sub_main/unk_335040.c), EmSlot_LoadModels 36 of 138 words, Area_LoadSubModels 2 of 158 words differ */
extern s32 gAreaSubModelFiles[];
extern s32 gAreaSubModelTexFiles[];
extern s32 gArea83SubModelTexFiles;
extern s32 D_0042F15C;
extern s32 gArea83SubModelTexFileS1;
extern s32 gArea89SubModelTexFiles[];
extern s16 gAreaSubModelKinds[4];

/* Loads the models of place id into the four slots of gAreaSubModelIds (the pack of each sub-area
 * by Load_AreaSubModelPack, its texture from the tables as there); unused slots get -1. */
void Area_LoadSubModels(u16 id)
{
    s32 n;
    s32 m;
    s32 i;
    s32 tex;

    n = Area_SubModelCount(id);
    for (i = 0; i < 4; i++) {
        if (i < n) {
            if (i == 0) {
                if (gAreaSubModelFiles[id] == -1) {
                    gAreaSubModelIds[i] = -1;
                    gAreaSubModels[i] = NULL;
                    continue;
                }
                switch (id) {
                case 0x83:
                    if (gScene.online != 0) {
                        switch (LobbyWorld_GetSeason()) {
                        case 2:
                        default:
                            tex = gArea83SubModelTexFiles;
                            break;
                        case 0:
                            tex = D_0042F15C;
                            break;
                        case 1:
                            tex = gArea83SubModelTexFileS1;
                            break;
                        }
                    } else {
                        tex = gArea83SubModelTexFiles;
                    }
                    break;
                default:
                    tex = gAreaSubModelTexFiles[id];
                    break;
                }
            } else {
                switch (id) {
                case 0x89:
                    tex = gArea89SubModelTexFiles[(i - 1) * 3 + (u8)Village_FacilityLevel(i)];
                    break;
                default:
                    gAreaSubModelIds[i] = -1;
                    gAreaSubModels[i] = NULL;
                    continue;
                }
            }
            Load_AreaSubModelPack(id, i);
            m = Model_Alloc(1);
            if (m < 0) {
                return;
            }
            gAreaSubModelIds[i] = m;
            gAreaSubModels[i] = Model_Get(m);
            Model_Reserve(m, 1);
            Model_SetupWithPack(m, (*(s32 *)&gSysSettings.modelPack), gAreaSubModelKinds[i], tex, 0, 0);
        } else {
            gAreaSubModelIds[i] = -1;
            gAreaSubModels[i] = NULL;
        }
    }
}

void Stage_LoadFilePair(s32);
Model *Model_SetupWithPackRaw(s16 idx, s32 a, s32 slot, s32 tex, s32 b, s32 c);
extern s32 gSubModelPairTexFiles[];
extern void *gAreaSubModel;

void Area_LoadSubModel(s32 i)
{
    s32 buf;
    s32 model;

    Stage_LoadFilePair(i);
    buf = (*(s32 *)&gSysSettings.dataBuf);
    model = Model_Alloc(1);
    if (model >= 0) {
        gAreaSubModelId = model;
        gAreaSubModel = Model_Get(model);
        Model_Reserve(model, 1);
        Model_SetupWithPackRaw(model, buf, 0xD, gSubModelPairTexFiles[i], 0x900, 2);
    }
}

void Area_FreeSubModel(void)
{
    s32 id;

    id = gAreaSubModelId;
    Model_Free(id);
    if (gModelUsed[id] != 0) {
        Model_Release(id, 1);
    }
}

extern TaskSlot gTaskSlots[];

void *memset(void *dst, s32 c, u32 n);
void TaskSlot_InitAll(void)
{
    memset(gTaskSlots, 0, 0x200);
}

void TaskSlot_Clear9(void)
{
    s32 i;
    TaskSlot *p = gTaskSlots;

    for (i = 0; i < 9; i++) {
        memset(p, 0, sizeof(TaskSlot));
        p++;
    }
}

extern u16 gFrameTick;
extern s16 gPrimCount;
extern u16 gPadDebugMode;
extern u8 D_7C90C0[];
extern u8 D_7CA500[];
void D_6B5230(TaskSlot *t); /* select Boot_Task */
u16 Rand_Next(s32 stream);
void Callbacks_Clear(void);
void LoadOverlay(s32 id);
void TaskSlot_Set(void *func, s16 slot);
void Scene_QuestTask(u8 *obj);

/* The per-frame task step: runs the 16 task slots (slots 2-11 only when gPadDebugMode or
 * gSysSettings.unk07[0] is not set), then starts the task gSysSettings.taskRequest asks for. */
void Task_RunFrame(void)
{
    TaskSlot *t;
    s32 i;
    s32 busy;

    gFrameTick++;
    Rand_Next(1);
    busy = gPadDebugMode != 0;
    if (!busy) {
        Callbacks_Clear();
        gPrimCount = 0;
    }
    if (gSysSettings.unk07[0] != 0) {
        busy = 1;
    }
    for (i = 0, t = gTaskSlots; i < 16; i++, t++) {
        if (!busy || i <= 1 || i > 11) {
            switch (t->state) {
            case 0:
                break;
            case 4:
                t->state = 8;
                break;
            case 12:
                t->state = 8;
                break;
            case 8:
                t->func(t);
                break;
            case 1:
                break;
            case 2:
                t->timer = 0;
                t->state = 4;
                break;
            case 16:
                t->timer--;
                if (t->timer < 0) {
                    t->timer = 0;
                } else if (t->timer == 0) {
                    t->state = 4;
                }
                break;
            }
        }
    }
    switch (gSysSettings.taskRequest) {
    case 0:
        return;
    case 1:
        LoadOverlay(4);
        TaskSlot_Set(D_7C90C0, 5);
        gSysSettings.taskRequest = 0;
        return;
    case 2:
        LoadOverlay(4);
        TaskSlot_Set(D_7CA500, 5);
        gSysSettings.taskRequest = 0;
        return;
    case 3:
        LoadOverlay(3);
        TaskSlot_Set(Scene_QuestTask, 5);
        gSysSettings.taskRequest = 0;
        return;
    case 4:
        LoadOverlay(2);
        TaskSlot_Set(D_6B5230, 3);
        gSysSettings.taskRequest = 0;
        return;
    }
}

/* unk_33B080's .data (0x42FC30-0x4306C0) */
extern u32 gPlayerLookFiles_0[];
extern u32 gPlayerLookFiles_1[];
extern u32 gPlayerLookFiles_2[];
extern u32 gPlayerLookFiles_3[];
extern u32 gPlayerLookFiles_4[];
extern u32 gPlayerLookFiles_5[];
extern u32 gPlayerLookFiles_6[];
extern u32 gPlayerLookFiles_7[];
extern u32 gPlayerLookFiles_8[];
extern u32 gPlayerLookFiles_9[];
extern u32 gPlayerLookFiles_10[];
extern u32 gPlayerLookFiles_11[];
extern u32 gPlayerLookFiles_12[];
extern u32 gPlayerLookFiles_13[];
s16 gLookPartMap0[32] = {
#include "sub_main/0042FC30.inc"
};
s16 gLookPartMap3[32] = {
#include "sub_main/0042FC70.inc"
};
s16 gLookPartMap4[32] = {
#include "sub_main/0042FCB0.inc"
};
s32 *gPlayerLookFiles[16] = {
    (s32 *)gPlayerLookFiles_0, (s32 *)gPlayerLookFiles_1, (s32 *)gPlayerLookFiles_2, (s32 *)gPlayerLookFiles_3,
    (s32 *)gPlayerLookFiles_4, (s32 *)gPlayerLookFiles_5, (s32 *)gPlayerLookFiles_6, (s32 *)gPlayerLookFiles_7,
    (s32 *)gPlayerLookFiles_8, (s32 *)gPlayerLookFiles_9, (s32 *)gPlayerLookFiles_10, (s32 *)gPlayerLookFiles_11,
    (s32 *)gPlayerLookFiles_12, (s32 *)gPlayerLookFiles_13, NULL, NULL,
};
u32 gPlayerPartFlagTbls_0[60] = {
#include "sub_main/0042FD30.inc"
};
u32 gPlayerPartFlagTbls_2[60] = {
#include "sub_main/0042FE20.inc"
};
u32 gPlayerPartFlagTbls_3[60] = {
#include "sub_main/0042FF10.inc"
};
u32 gPlayerPartFlagTbls_4[60] = {
#include "sub_main/00430000.inc"
};
u32 gPlayerPartFlagTbls_5[60] = {
#include "sub_main/004300F0.inc"
};
u32 gPlayerPartFlagTbls_6[60] = {
#include "sub_main/004301E0.inc"
};
u32 gPlayerPartFlagTbls_8[60] = {
#include "sub_main/004302D0.inc"
};
u32 gPlayerPartFlagTbls_9[60] = {
#include "sub_main/004303C0.inc"
};
u32 gPlayerPartFlagTbls_10[60] = {
#include "sub_main/004304B0.inc"
};
u32 gPlayerPartFlagTbls_11[60] = {
#include "sub_main/004305A0.inc"
};
u32 gPlayerPartFlagTbls[12] = {
    (u32)gPlayerPartFlagTbls_0, 0, (u32)gPlayerPartFlagTbls_2, (u32)gPlayerPartFlagTbls_3,
    (u32)gPlayerPartFlagTbls_4, (u32)gPlayerPartFlagTbls_5, (u32)gPlayerPartFlagTbls_6, 0,
    (u32)gPlayerPartFlagTbls_8, (u32)gPlayerPartFlagTbls_9, (u32)gPlayerPartFlagTbls_10, (u32)gPlayerPartFlagTbls_11,
};
