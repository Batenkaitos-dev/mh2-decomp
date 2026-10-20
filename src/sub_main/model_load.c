/*
 * The models of the hunters, the monsters and the stage, loaded into the model pool's slots
 * and freed again.
 *
 * - A hunter's look: Player_BuildPartIds turns the equipment into one model id per part
 *   (the armour tables' male or female model, the face's colour, the hair under a head
 *   piece without a model), Look_MapPartId swaps some ids for the character type's own,
 *   and Player_LoadPartModels loads the parts whose id changed (Player_PartIdDiffers).
 *   Player_FreeModels and Player_FreeChangedPartModels free them.
 * - The equipment preview (gEquipPreview, EquipPreview): the menus' model of one piece of
 *   equipment (EquipPreview_Load, EquipPreview_Free).
 * - The monster slots (SceneState's per-slot model fields): EmSlot_LoadModels for the quest's
 *   monsters, EmSlot_LoadModel for the lobby's, EmSlot_FreeModels and EmSlot_Clear.
 * - The area's models (AreaState) and its sub-models (gAreaSubModelIds: the hub areas'
 *   extra models), and the character maker's parts model, one per gender.
 *
 * A model is taken with Model_Alloc and Model_Reserve, set up from its pack with the
 * Model_Setup* functions, and freed slot by slot with Model_Free and Model_Release.
 */
#include "common.h"
#include "syssettings.h"
#include "em.h"
#include "scene.h"
#include "model.h"
#include "save.h"
#include "player.h"
#include "task.h"
#include "area.h"
#include "item.h"
#include "equip_preview.h"
#include "overlay.h"

/* prototypes */
void Model_Free(s32 idx);
void Model_Release(s32 first, s32 count);
extern s32 gAreaSubModelId;
extern u8 gModelUsed[];
s32 Armor_LacksSexModel(s32 id, u8 female);
s32 Model_Alloc(s32 count);
Model *Model_Get(s32 idx);
void Model_Reserve(s32 first, s32 count);
void EmSlot_Clear(s16 slot);
void Motion_FreeEmSlot(s16 slot);
void Load_EmSlotModelPack(s32 kind);
/* unit's own declaration: the texture slot is declared s16, as the callers pass it; with the definition's s32 the call in EmSlot_LoadModel compiles differently */
Model *Model_SetupTexPackB(s16 model, s32 work, s16 texSlot, s32 packId, s32 setupFlags);
void Anim_LoadEmSlot(s32 kind, s32 var, s32 slot);
extern s32 gModelLoadBuf;
extern s32 gEmSlotTexFiles[];
extern s32 gEmSlotAnimFiles[];
/* Per monster slot, the kind ([0]) and the variant ([1]) of the model it holds, -1: none. */
extern s32 gEmSlotModelIds[][2];
/* The variants' column of gEmSlotModelIds (gEmSlotModelIds[0][1], a name the linker defines):
 * EmSlot_LoadModel reads the variants through it, and through gEmSlotModelIds[slot][1] it
 * compiles differently */
extern s32 gEmSlotModelVars[][2];
/* end of prototypes */

extern s32 gAreaSubModelIds[4];     /* four more model slots, -1 when unused */

/* The model id of a piece of equipment of type `type` (an EquipType) for the gender (0 male,
 * else female): an armour piece's model for that gender, 0 when the piece has none (its
 * ARMOR_FLAG_MALE or ARMOR_FLAG_FEMALE is clear); a weapon's kind. */
u16 Equip_GetModelId(u16 type, u16 id, u16 female)
{
    switch (type) {
    case EQUIP_LEGS:
        if (female == 0) {
            if (gArmorLegsData[id].flags & ARMOR_FLAG_MALE) {
                return gArmorLegsData[id].modelMale;
            }
            return 0;
        }
        if (gArmorLegsData[id].flags & ARMOR_FLAG_FEMALE) {
            return gArmorLegsData[id].modelFemale;
        }
        return 0;
    case EQUIP_ARMS:
        if (female == 0) {
            if (gArmorArmsData[id].flags & ARMOR_FLAG_MALE) {
                return gArmorArmsData[id].modelMale;
            }
            return 0;
        }
        if (gArmorArmsData[id].flags & ARMOR_FLAG_FEMALE) {
            return gArmorArmsData[id].modelFemale;
        }
        return 0;
    case EQUIP_BODY:
        if (female == 0) {
            if (gArmorBodyData[id].flags & ARMOR_FLAG_MALE) {
                return gArmorBodyData[id].modelMale;
            }
            return 0;
        }
        if (gArmorBodyData[id].flags & ARMOR_FLAG_FEMALE) {
            return gArmorBodyData[id].modelFemale;
        }
        return 0;
    case EQUIP_WAIST:
        if (female == 0) {
            if (gArmorWaistData[id].flags & ARMOR_FLAG_MALE) {
                return gArmorWaistData[id].modelMale;
            }
            return 0;
        }
        if (gArmorWaistData[id].flags & ARMOR_FLAG_FEMALE) {
            return gArmorWaistData[id].modelFemale;
        }
        return 0;
    case EQUIP_HEAD:
        if (female == 0) {
            if (gArmorHeadData[id].flags & ARMOR_FLAG_MALE) {
                return gArmorHeadData[id].modelMale;
            }
            return 0;
        }
        if (gArmorHeadData[id].flags & ARMOR_FLAG_FEMALE) {
            return gArmorHeadData[id].modelFemale;
        }
        return 0;
    case EQUIP_BLADE:
        return gBladeData[id].kind;
    case EQUIP_GUN:
        return gGunData[id].kind;
    }
    return 0;
}

extern u16 gPartIdMax[2][7];
extern u16 gArmourIdMax[];
extern u8 gFaceBodyTypeMale[];
extern u8 gFaceBodyTypeFemale[];

/* Builds the six part ids of a hunter's look (partIds, by equipment slot) from its equipment
 * (equip, by slot):
 * - an armour id over its slot's limit (gArmourIdMax) is cleared first, in equip itself;
 * - each armour slot gets the piece's model for the hunter's gender, 0 when it has none;
 * - the head without a piece, or with one that has no model for the gender, shows the hair
 *   (Player.hairStyle);
 * - EQUIP_SLOT_FACE holds the face: its id less 1, clamped to the gender's faces, and
 *   *lookVariant gets the face's body variant (gFaceBodyTypeMale, gFaceBodyTypeFemale);
 * - an id at or over the gender's limit for the slot (gPartIdMax) becomes 0.
 * The last test reads the part through a pointer to partIds[n]: the compiler then keeps n in
 * the register the game's code uses. */
void Player_BuildPartIds(Player *pl, u8 *partIds, EquipItem *equip, u8 *lookVariant)
{
    s16 i;
    u16 *idMax;
    EquipItem *item;
    u8 face;
    u16 id;
    u8 *part;

    idMax = gArmourIdMax;
    item = equip;
    for (i = 0; i < 6; i++, idMax++, item++) {
        s32 n;

        n = i;
        if (n != EQUIP_SLOT_FACE && item->id > *idMax) {
            item->id = 0;
        }
        switch (n) {
        case EQUIP_SLOT_FACE:
            face = equip[EQUIP_SLOT_FACE].id;
            if (face <= 0 || gPartIdMax[pl->gender][EQUIP_SLOT_FACE] < face) {
                face = 1;
            }
            partIds[EQUIP_SLOT_FACE] = face - 1;
            if (pl->gender == 0) {
                *lookVariant = gFaceBodyTypeMale[partIds[EQUIP_SLOT_FACE]];
            } else {
                *lookVariant = gFaceBodyTypeFemale[partIds[EQUIP_SLOT_FACE]];
            }
            break;
        case EQUIP_LEGS:
            if (pl->gender == 0) {
                if (gArmorLegsData[equip[EQUIP_LEGS].id].flags & ARMOR_FLAG_MALE) {
                    partIds[EQUIP_LEGS] = gArmorLegsData[equip[EQUIP_LEGS].id].modelMale;
                } else {
                    partIds[EQUIP_LEGS] = 0;
                }
            } else {
                if (gArmorLegsData[equip[EQUIP_LEGS].id].flags & ARMOR_FLAG_FEMALE) {
                    partIds[EQUIP_LEGS] = gArmorLegsData[equip[EQUIP_LEGS].id].modelFemale;
                } else {
                    partIds[EQUIP_LEGS] = 0;
                }
            }
            break;
        case EQUIP_BODY:
            if (pl->gender == 0) {
                if (gArmorBodyData[equip[EQUIP_BODY].id].flags & ARMOR_FLAG_MALE) {
                    partIds[EQUIP_BODY] = gArmorBodyData[equip[EQUIP_BODY].id].modelMale;
                } else {
                    partIds[EQUIP_BODY] = 0;
                }
            } else {
                if (gArmorBodyData[equip[EQUIP_BODY].id].flags & ARMOR_FLAG_FEMALE) {
                    partIds[EQUIP_BODY] = gArmorBodyData[equip[EQUIP_BODY].id].modelFemale;
                } else {
                    partIds[EQUIP_BODY] = 0;
                }
            }
            break;
        case EQUIP_ARMS:
            if (pl->gender == 0) {
                if (gArmorArmsData[equip[EQUIP_ARMS].id].flags & ARMOR_FLAG_MALE) {
                    partIds[EQUIP_ARMS] = gArmorArmsData[equip[EQUIP_ARMS].id].modelMale;
                } else {
                    partIds[EQUIP_ARMS] = 0;
                }
            } else {
                if (gArmorArmsData[equip[EQUIP_ARMS].id].flags & ARMOR_FLAG_FEMALE) {
                    partIds[EQUIP_ARMS] = gArmorArmsData[equip[EQUIP_ARMS].id].modelFemale;
                } else {
                    partIds[EQUIP_ARMS] = 0;
                }
            }
            break;
        case EQUIP_WAIST:
            if (pl->gender == 0) {
                if (gArmorWaistData[equip[EQUIP_WAIST].id].flags & ARMOR_FLAG_MALE) {
                    partIds[EQUIP_WAIST] = gArmorWaistData[equip[EQUIP_WAIST].id].modelMale;
                } else {
                    partIds[EQUIP_WAIST] = 0;
                }
            } else {
                if (gArmorWaistData[equip[EQUIP_WAIST].id].flags & ARMOR_FLAG_FEMALE) {
                    partIds[EQUIP_WAIST] = gArmorWaistData[equip[EQUIP_WAIST].id].modelFemale;
                } else {
                    partIds[EQUIP_WAIST] = 0;
                }
            }
            break;
        case EQUIP_HEAD:
            id = equip[EQUIP_HEAD].id;
            if (id == 0) {
                partIds[EQUIP_HEAD] = pl->hairStyle;
            } else if (pl->gender == 0) {
                if (gArmorHeadData[id].flags & ARMOR_FLAG_MALE) {
                    if (gArmorHeadData[id].modelMale == 0) {
                        partIds[EQUIP_HEAD] = pl->hairStyle;
                    } else {
                        partIds[EQUIP_HEAD] = gArmorHeadData[id].modelMale;
                    }
                } else {
                    partIds[EQUIP_HEAD] = pl->hairStyle;
                }
            } else {
                if (gArmorHeadData[id].flags & ARMOR_FLAG_FEMALE) {
                    if (gArmorHeadData[id].modelFemale == 0) {
                        partIds[EQUIP_HEAD] = pl->hairStyle;
                    } else {
                        partIds[EQUIP_HEAD] = gArmorHeadData[id].modelFemale;
                    }
                } else {
                    partIds[EQUIP_HEAD] = pl->hairStyle;
                }
            }
            break;
        }
        if (n != EQUIP_SLOT_FACE) {
            part = &partIds[n];
            /* gPartIdMax read as one flat array: as gPartIdMax[gender][n], the
             * address is computed in another order than the game's */
            if (*part >= ((u16 *)gPartIdMax)[pl->gender * 7 + n]) {
                *part = 0;
            }
        }
    }
}

/* 1 when head armour `id` has no model of its own for the gender: no piece (id 0), a piece
 * the gender can't wear (ARMOR_FLAG_MALE or ARMOR_FLAG_FEMALE clear), or a model id of 0; else
 * 0. The hair is then shown instead (Player_LoadPartModels). */
s32 Armor_LacksSexModel(s32 id, u8 female)
{
    u8 mask;

    id = (u16)id;
    if (id == 0) {
        return 1;
    }
    if (female == 0) {
        mask = ARMOR_FLAG_MALE;
    } else {
        mask = ARMOR_FLAG_FEMALE;
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

/* 0 when the model loaded for part `slot` (Player.lookPartId) is already part id `id`, else 1
 * (the part must be loaded again). The head compares only while it shows the same kind of
 * model as before: the armour's (headAltModel 0) while the armour has a model, or the hair
 * (headAltModel 1, compared with hairStyle) while it has none. One u8 holds the slot for the
 * switch and then the loaded id, as in the game's code. */
s32 Player_PartIdDiffers(Player *pl, s32 slot, s32 id)
{
    u8 loaded = slot;

    switch (loaded) {
    case EQUIP_HEAD:
        if (pl->headAltModel == 0) {
            if (Armor_LacksSexModel(pl->equip[EQUIP_HEAD].id, pl->gender) == 0) {
                loaded = pl->lookPartId[(u8)slot];
                if (loaded != PL_LOOK_PART_NONE && loaded == (u8)id) {
                    return 0;
                }
            }
        } else if (Armor_LacksSexModel(pl->equip[EQUIP_HEAD].id, pl->gender) == 1) {
            loaded = pl->lookPartId[(u8)slot];
            if (loaded != PL_LOOK_PART_NONE && loaded == pl->hairStyle) {
                return 0;
            }
        }
        break;
    case EQUIP_LEGS:
    case EQUIP_SLOT_FACE:
    case EQUIP_BODY:
    case EQUIP_ARMS:
    case EQUIP_WAIST:
        loaded = pl->lookPartId[(u8)slot];
        if (loaded != PL_LOOK_PART_NONE && loaded == (u8)id) {
            return 0;
        }
        break;
    }
    return 1;
}

extern s16 gLookPartMap0[];
extern s16 gLookPartMap3[];
extern s16 gLookPartMap4[];

/* The part id to load for part `slot`: the legs, the body and the arms map their id through
 * their table (gLookPartMap0, gLookPartMap3, gLookPartMap4: 16 ids per gender). An id found in
 * the gender's row becomes that row's entry charType * 4 + variant (Player.charType, the look
 * variant); any other id, and the other parts, stay as they are. */
s16 Look_MapPartId(Player *pl, s16 slot, s16 id, u8 variant)
{
    s16 i;
    s16 ret = id;

    switch (slot) {
    case EQUIP_LEGS:
        for (i = 0; i < 16; i++) {
            if (ret == gLookPartMap0[pl->gender * 16 + i]) {
                ret = gLookPartMap0[variant + pl->gender * 16 + pl->charType * 4];
                break;
            }
        }
        break;
    case EQUIP_SLOT_FACE:
    case EQUIP_HEAD:
    case EQUIP_WAIST:
        break;
    case EQUIP_BODY:
        for (i = 0; i < 16; i++) {
            if (ret == gLookPartMap3[pl->gender * 16 + i]) {
                ret = gLookPartMap3[variant + pl->gender * 16 + pl->charType * 4];
                break;
            }
        }
        break;
    case EQUIP_ARMS:
        for (i = 0; i < 16; i++) {
            if (ret == gLookPartMap4[pl->gender * 16 + i]) {
                ret = gLookPartMap4[variant + pl->gender * 16 + pl->charType * 4];
                break;
            }
        }
        break;
    }
    return ret;
}

void Load_WeaponModelPack(s32 id);
void Load_EquipModelPack(s32 id, s32 female);
/* unit's own declaration: the texture slot is declared s16, as the callers pass it; with the definition's s32 the calls in EmSlot_LoadModels and Area_LoadSubModels compile differently */
Model *Model_SetupWithPack(s16 idx, s32 modelFile, s16 texSlot, s32 packId, s32 setupFlags, s32 skelKind);
void FxBlock_Clear(EquipFxSwing *p);
void Load_PartModelPack(s32 id, s32 kind, s32 female);
/* unit's own declaration: the texture slot is declared s16, as the callers pass it; with the definition's s32 the calls in Player_LoadPartModels compile differently */
Model *Model_SetupLoadedPack(s16 idx, s32 modelFile, s16 texSlot, s32 *pack, s32 setupFlags, s32 secondId, s32 skelKind);
/* Per gender, then per look kind (the six equipment slots, then 6: the hair), the model packs
 * of each part id (two words each). */
extern s32 *gPlayerLookFiles[];
extern s32 gEquipModelTexFilesM[];
extern s32 gEquipModelTexFilesF[];
extern s32 gModelPackIds[];

/* Loads the equipment preview's model: of type `type` (an EquipType: a weapon's model by its
 * kind, or an armour piece's part model; the head without a piece shows the save's hair), or
 * with EQUIP_PREVIEW_FIXED a model of the gender's own table. The preview is drawn once its
 * model is set up (EquipPreview.active). */
void EquipPreview_Load(u16 type, u16 id, u8 female)
{
    EquipPreview *preview = &gEquipPreview;
    s16 idx;
    s32 *pack;

    gEquipPreview.active = 0;
    gEquipPreview.equipType = type;
    gEquipPreview.equipId = id;
    gEquipPreview.female = female;
    gEquipPreview.unk04[0] = 0;
    gEquipPreview.unk04[1] = 0;
    gEquipPreview.unk04[2] = 0;
    gEquipPreview.settled = 0;
    switch (type) {
    case EQUIP_PREVIEW_FIXED:
        Load_EquipModelPack(id, female);
        idx = Model_Alloc(1);
        if (idx < 0) {
            return;
        }
        preview->modelFirst = idx;
        preview->modelCount = 1;
        preview->model = Model_Get(idx);
        Model_Reserve(idx, 1);
        if (female == 0) {
            Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 0x166, gEquipModelTexFilesM[id], 0x900, 1);
        } else {
            Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 0x166, gEquipModelTexFilesF[id], 0x900, 1);
        }
        break;
    case EQUIP_HEAD:
        if (id == 0) {
            type = 6; /* the hair's look kind */
            id = gSave.hairStyle;
        }
    default:
        Load_PartModelPack(id, type, female);
        idx = Model_Alloc(1);
        if (idx < 0) {
            return;
        }
        preview->modelFirst = idx;
        preview->modelCount = 1;
        preview->model = Model_Get(idx);
        Model_Reserve(idx, 1);
        pack = gPlayerLookFiles[type + female * 7];
        pack += id * 2;
        Model_SetupLoadedPack(idx, (*(s32 *)&gSysSettings.dataBuf), 0x166, pack, 0x900,
                      (u16)(female * 4), 1);
        if (type == EQUIP_HEAD) {
            FxBlock_Clear(preview->equipFx);
        }
        break;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        Load_WeaponModelPack(id);
        idx = Model_Alloc(1);
        if (idx < 0) {
            return;
        }
        preview->modelFirst = idx;
        preview->modelCount = 1;
        preview->model = Model_Get(idx);
        Model_Reserve(idx, 1);
        Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 0x166, gModelPackIds[id], 0x900, 1);
        break;
    }
    preview->active = 1;
}

/* Frees the equipment preview's model slots and stops its drawing. */
void EquipPreview_Free(void)
{
    EquipPreview *preview = &gEquipPreview;
    s32 i;

    preview->active = 0;
    if (preview->modelCount != 0) {
        for (i = preview->modelFirst; i < preview->modelFirst + preview->modelCount; i++) {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
        }
        preview->model = NULL;
        preview->modelFirst = 0;
        preview->modelCount = 0;
    }
}

void Player_BuildPartIds(Player *pl, u8 *partIds, EquipItem *equip, u8 *lookVariant);

/* Loads the models of the hunter's six parts (Player_BuildPartIds into Player.armourKind,
 * then Look_MapPartId): each part whose loaded model differs (Player_PartIdDiffers) gets its
 * pack loaded and a model slot. The head without a model of its own loads the hair (look
 * kind 6) and sets headAltModel. Part 0 (the legs) is the skeleton's model (Player.model);
 * each part takes two texture slots of the player's twelve. */
void Player_LoadPartModels(Player *pl)
{
    s16 kind;
    s16 model;
    s16 id;
    s16 i;
    s32 *pack;

    Player_BuildPartIds(pl, pl->armourKind, pl->equip, &pl->lookVariant);
    for (i = 0; i < 6; i++) {
        id = Look_MapPartId(pl, i, pl->armourKind[i], pl->lookVariant);
        if (i == EQUIP_HEAD && Armor_LacksSexModel(pl->equip[EQUIP_HEAD].id, pl->gender) == 1) {
            kind = 6; /* the hair */
        } else {
            kind = i;
        }
        if (Player_PartIdDiffers(pl, (u8)i, (u8)id)) {
            pl->lookPartId[i] = id;
            if (kind == EQUIP_HEAD) {
                pl->headAltModel = 0;
            } else if (kind == 6) {
                pl->headAltModel = 1;
            }
            Load_PartModelPack(id, kind, pl->gender);
            model = Model_Alloc(1);
            if (model < 0) {
                return;
            }
            if (i == 0) {
                pl->modelFirst = model;
                pl->modelCount = 1;
                pl->model = Model_Get(model);
            } else {
                pl->partModelFirst[i] = model;
                pl->partModelCount[i] = 1;
                pl->partModels[i] = Model_Get(model);
            }
            Model_Reserve(model, 1);
            pack = gPlayerLookFiles[kind + pl->gender * 7];
            pack += id * 2;
            if (i == 0) {
                Model_SetupLoadedPack(model, (*(s32 *)&gSysSettings.dataBuf), pl->index * 12 + 0x2B + i * 2, pack, 0x900,
                    (u16)(pl->lookVariant + pl->gender * 4), 2);
            } else {
                Model_SetupLoadedPack(model, (*(s32 *)&gSysSettings.dataBuf), pl->index * 12 + 0x2B + i * 2, pack, 0x900,
                    (u16)(pl->lookVariant + pl->gender * 4), 1);
            }
        }
    }
}

/* Frees all of a hunter's part models and marks every part unloaded (PL_LOOK_PART_NONE). */
void Player_FreeModels(Player *pl)
{
    s32 i;
    s32 k;

    for (i = pl->modelFirst; i < pl->modelFirst + pl->modelCount; i++) {
        Model_Free(i);
        if (gModelUsed[i] != 0) {
            Model_Release(i, 1);
        }
    }
    pl->model = NULL;
    pl->modelFirst = 0;
    pl->modelCount = 0;
    pl->lookPartId[0] = PL_LOOK_PART_NONE;
    for (k = 1; k < 6; k++) {
        for (i = pl->partModelFirst[k]; i < pl->partModelFirst[k] + pl->partModelCount[k]; i++) {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
        }
        pl->partModels[k] = NULL;
        pl->partModelFirst[k] = 0;
        pl->partModelCount[k] = 0;
        pl->lookPartId[k] = PL_LOOK_PART_NONE;
    }
}

/* Frees the models of the parts that the hunter's current equipment would change: the part
 * ids are built from a copy of the equipment (Player_BuildPartIds clears ids over their limit
 * in the copy, not in the hunter's), and each part whose loaded model differs
 * (Player_PartIdDiffers) is freed and marked PL_LOOK_PART_NONE. Player_LoadPartModels then
 * loads them again. */
void Player_FreeChangedPartModels(Player *pl)
{
    EquipItem equip[6];
    u8 partIds[7];
    u8 variant;
    s32 i;
    s32 j;

    equip[0] = pl->equip[0];
    equip[1] = pl->equip[1];
    equip[2] = pl->equip[2];
    equip[3] = pl->equip[3];
    equip[4] = pl->equip[4];
    equip[5] = pl->equip[5];
    Player_BuildPartIds(pl, partIds, equip, &variant);
    if (Player_PartIdDiffers(pl, 0, (u8)Look_MapPartId(pl, 0, partIds[0], variant)) == 1) {
        for (i = pl->modelFirst; i < pl->modelFirst + pl->modelCount; i++) {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
        }
        pl->model = NULL;
        pl->modelFirst = 0;
        pl->modelCount = 0;
        pl->lookPartId[0] = PL_LOOK_PART_NONE;
    }
    for (j = 1; j < 6; j++) {
        if (Player_PartIdDiffers(pl, (u8)j, (u8)Look_MapPartId(pl, j, partIds[j], variant)) == 1) {
            for (i = pl->partModelFirst[j]; i < pl->partModelFirst[j] + pl->partModelCount[j]; i++) {
                Model_Free(i);
                if (gModelUsed[i] != 0) {
                    Model_Release(i, 1);
                }
            }
            pl->partModels[j] = NULL;
            pl->partModelFirst[j] = 0;
            pl->partModelCount[j] = 0;
            pl->lookPartId[j] = PL_LOOK_PART_NONE;
        }
    }
}

void Load_EmModelPack(s32 kind);
void Load_EmPartModelPack(s32 kind);
void Anim_LoadEm(s32 slot, s32 kind);
/* unit's own declaration: the texture slot is declared s16, as EmSlot_LoadModels passes it; with the definition's s32 the call compiles differently */
Model *Model_SetupTexPackEx(s16 idx, s32 work, s16 texSlot, s32 packId, s32 setupFlags, u8 slot);
extern s32 gEmSlotTexPacks[];
extern s32 gEmSlotModelPacks[];
extern s32 gEmPartModelPackFiles[];

/*
 * Loads the models of the quest's monster in slot `slot` (its kind in gScene.emKind, none
 * when 0): its model with its textures and motions (texture slots 20 per monster slot from
 * 0xA3), and when the kind has one (gEmPartModelPackFiles not -1) a second model set (texture
 * slot 0x10 further on); else the set's count is -1.
 * FAKEMATCH (docs/notes/nm/EmSlot_LoadModels.md): written so that the compiler keeps the
 * values in the registers the game's code uses.
 * - Each path computes its own byte offset o = slot * sizeof(s16) and writes the halfword
 *   arrays through it.
 * - The kind is reached through the integer sum slot + &gScene, plus emKind's offset (0x2A):
 *   the game's code adds them in that order.
 * - kind points first at the whole emKind array, for the test alone, and only then at the
 *   slot's entry. With one pointer to emKind[slot], the compiler gives it another register.
 */
void EmSlot_LoadModels(s32 slot)
{
    s32 work;
    s32 model;
    s32 o;
    u8 *kind;
    SceneState *scene;

    scene = &gScene;
    kind = scene->emKind;
    if (kind[slot] == 0) {
        o = slot * sizeof(s16);
        *(s16 *)((u8 *)scene->modelFirst + o) = 0;
        *(s16 *)((u8 *)scene->modelCount + o) = 0;
        scene->models[slot] = NULL;
        return;
    }
    kind = (u8 *)(slot + (s32)scene) + 0x2A; /* &gScene.emKind[slot] */
    Load_EmModelPack(*kind);
    work = gModelLoadBuf;
    model = Model_Alloc(1);
    if (model < 0) {
        return;
    }
    o = slot * sizeof(s16);
    *(s16 *)((u8 *)scene->modelFirst + o) = model;
    *(s16 *)((u8 *)scene->modelCount + o) = 1;
    scene->models[slot] = Model_Get(model);
    Model_Reserve(model, 1);
    Model_SetupTexPackEx(model, work, slot * 0x14 + 0xA3, gEmSlotTexPacks[*kind], 0x900, slot);
    Anim_LoadEm(slot, *kind);
    if (gEmPartModelPackFiles[*kind] != -1) {
        Load_EmPartModelPack(*kind);
        work = gModelLoadBuf;
        model = Model_Alloc(1);
        if (model < 0) {
            *(s16 *)((u8 *)scene->partFirst + o) = 0;
            *(s16 *)((u8 *)scene->partCount + o) = -1;
            scene->partModels[slot] = NULL;
            return;
        }
        *(s16 *)((u8 *)scene->partFirst + o) = model;
        *(s16 *)((u8 *)scene->partCount + o) = 1;
        scene->partModels[slot] = Model_Get(model);
        Model_Reserve(model, 1);
        Model_SetupWithPack(model, work, slot * 0x14 + 0xB3, gEmSlotModelPacks[*kind], 0x900, 0);
        return;
    }
    *(s16 *)((u8 *)scene->partFirst + o) = 0;
    *(s16 *)((u8 *)scene->partCount + o) = -1;
    scene->partModels[slot] = NULL;
}

/* Loads the model of monster kind `kind`, variant `var`, into monster slot `slot`, unless the
 * slot holds it already (gEmSlotModelIds): the slot's old model is freed first, and its old
 * motions unless it is slot 0. The lobby loads its creatures this way. */
void EmSlot_LoadModel(s32 kind, s32 var, s32 slot)
{
    SceneState *scene;
    s32 *cur;
    s32 *curVar;
    s32 work;
    s32 model;

    cur = &gEmSlotModelIds[slot][0];
    scene = &gScene;
    if (kind != *cur || var != gEmSlotModelVars[slot][0]) {
        if (*cur != -1) {
            EmSlot_Clear(slot);
        }
        curVar = &gEmSlotModelVars[slot][0];
        if (*curVar != -1 && slot != 0) {
            Motion_FreeEmSlot(slot);
        }
        *cur = kind;
        *curVar = var;
        Load_EmSlotModelPack(kind);
        work = (*(s32 *)&gSysSettings.dataBuf);
        model = Model_Alloc(1);
        if (model >= 0) {
            scene->modelFirst[slot] = model;
            scene->modelCount[slot] = 1;
            scene->models[slot] = Model_Get(model);
            Model_Reserve(model, 1);
            Model_SetupTexPackB(model, work, slot * 0x14 + 0xA3, gEmSlotTexFiles[kind], 0x900);
            if (gEmSlotAnimFiles[kind] != 0) {
                Anim_LoadEmSlot(kind, var, slot);
            }
        }
    }
}

/* Forgets which model monster slot `slot` holds (gEmSlotModelIds), so that EmSlot_LoadModel
 * loads it again: its kind (which 0), its variant (1) or both (2). */
void EmSlot_ForgetModel(s16 slot, s8 which)
{
    switch (which) {
    case 0:
        gEmSlotModelIds[slot][0] = -1;
        break;
    case 1:
        gEmSlotModelIds[slot][1] = -1;
        break;
    case 2:
        gEmSlotModelIds[slot][0] = -1;
        gEmSlotModelIds[slot][1] = -1;
        break;
    }
}

/* Forgets monster slot i's kind and frees its models (modelFirst[i], modelCount[i] of them).
 * The bound first + n is written in the loop's test, not kept in a local: the compiler hoists
 * it out of the loop itself, and gives it the register the game's code does. */
void EmSlot_Clear(s16 i)
{
    s16 j, first, n;

    EmSlot_ForgetModel(i, 0);
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
 * Frees the models of the quest's monster in slot i and its second model set, marks both
 * empty (count -1), frees its motions and clears its kind.
 * FAKEMATCH (docs/notes/nm/EmSlot_FreeModels.md): the game's code computes i and i * 2 in
 * an order that plain C doesn't give, and three forms here reproduce it:
 * - the (s16) cast on the index of the first count read;
 * - n set from i with an explicit cast, and the last store indexed with i itself;
 * - the empty test of first, which the compiler removes but which still splits the block
 *   after the first loop in two.
 */
void EmSlot_FreeModels(s16 i)
{
    SceneState *scene = &gScene;
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
    first = scene->partFirst[n];
    if (first < 0 || first >= 0x80) {} /* the empty test the match needs (above) */
    cnt = scene->partCount[n];
    if (cnt != -1) {
        for (j = first; j < first + cnt; j++) {
            Model_Free(j);
            if (gModelUsed[j] != 0) {
                Model_Release(j, 1);
            }
        }
    }
    scene->modelFirst[n] = 0;
    scene->modelCount[n] = -1;
    scene->models[n] = NULL;
    scene->partFirst[n] = 0;
    scene->partCount[n] = -1;
    scene->partModels[n] = NULL;
    Motion_FreeEmSlot(i);
    scene->emKind[i] = 0;
}

/* Frees the area's models (AreaState.modelSlot on, modelCount of them) and its sub-models
 * (gAreaSubModelIds). */
void Area_FreeModels(void)
{
    s16 i;
    s16 k;
    s16 id;
    AreaState *area;

    i = gAreaState.modelSlot;
    area = &gAreaState;
    if (i < i + gAreaState.modelCount) {
        do {
            Model_Free(i);
            if (gModelUsed[i] != 0) {
                Model_Release(i, 1);
            }
            i++;
        } while (i < area->modelSlot + area->modelCount);
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
u8 Village_FacilityLevel(s32 facility);
void Load_AreaSubModelPack(s32 id, s32 sub);
extern s32 gAreaSubModelFiles[];
extern s32 gAreaSubModelTexFiles[];
extern s32 gArea83SubModelTexFiles[];
extern s32 gArea83SubModelTexFileS1;
extern s32 gArea89SubModelTexFiles[];
extern s16 gAreaSubModelKinds[4];

/* Loads area `id`'s sub-models (Area_SubModelCount of them, at most four) into
 * gAreaSubModelIds and gAreaSubModels; unused entries get -1 and NULL.
 * - Sub-model 0 is the area's own (none when gAreaSubModelFiles has -1), with its texture
 *   from gAreaSubModelTexFiles; AREA_HUB_0x83 in the lobby takes the texture of the season
 *   (LobbyWorld_GetSeason).
 * - Sub-models 1-3 exist only in AREA_HUB_0x89: the textures of each facility's level
 *   (Village_FacilityLevel). */
void Area_LoadSubModels(u16 id)
{
    s32 n;
    s32 model;
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
                case AREA_HUB_0x83:
                    if (gScene.inLobby != 0) {
                        switch (LobbyWorld_GetSeason()) {
                        case 2:
                        default:
                            tex = gArea83SubModelTexFiles[0];
                            break;
                        case 0:
                            tex = gArea83SubModelTexFiles[1];
                            break;
                        case 1:
                            tex = gArea83SubModelTexFileS1;
                            break;
                        }
                    } else {
                        tex = gArea83SubModelTexFiles[0];
                    }
                    break;
                default:
                    tex = gAreaSubModelTexFiles[id];
                    break;
                }
            } else {
                switch (id) {
                case AREA_HUB_0x89:
                    tex = gArea89SubModelTexFiles[(i - 1) * 3 + (u8)Village_FacilityLevel(i)];
                    break;
                default:
                    gAreaSubModelIds[i] = -1;
                    gAreaSubModels[i] = NULL;
                    continue;
                }
            }
            Load_AreaSubModelPack(id, i);
            model = Model_Alloc(1);
            if (model < 0) {
                return;
            }
            gAreaSubModelIds[i] = model;
            gAreaSubModels[i] = Model_Get(model);
            Model_Reserve(model, 1);
            Model_SetupWithPack(model, (*(s32 *)&gSysSettings.modelPack), gAreaSubModelKinds[i], tex, 0, 0);
        } else {
            gAreaSubModelIds[i] = -1;
            gAreaSubModels[i] = NULL;
        }
    }
}

void Stage_LoadFilePair(s32 pair);
Model *Model_SetupWithPackRaw(s16 idx, s32 modelFile, s32 texSlot, s32 packId, s32 setupFlags, s32 skelKind);
extern s32 gSubModelPairTexFiles[];
extern void *gAreaSubModel;

/* Loads the character maker's parts model for the gender (its file pair, Stage_LoadFilePair)
 * into gAreaSubModelId and gAreaSubModel: one material set per part. */
void CharMake_LoadPartsModel(s32 gender)
{
    s32 buf;
    s32 model;

    Stage_LoadFilePair(gender);
    buf = (*(s32 *)&gSysSettings.dataBuf);
    model = Model_Alloc(1);
    if (model >= 0) {
        gAreaSubModelId = model;
        gAreaSubModel = Model_Get(model);
        Model_Reserve(model, 1);
        Model_SetupWithPackRaw(model, buf, 0xD, gSubModelPairTexFiles[gender], 0x900, 2);
    }
}

/* Frees the character maker's parts model. */
void CharMake_FreePartsModel(void)
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


/* The file's data: the look tables of the hunters' parts. */
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
