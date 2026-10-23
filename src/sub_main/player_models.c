/*
 * The hunters' models, loaded into the model pool's slots and freed again.
 *
 * - A hunter's look: Player_BuildPartIds turns the equipment into one model id per part
 *   (the armour tables' male or female model, the face's colour, the hair under a head
 *   piece without a model), Look_MapPartId swaps some ids for the character type's own,
 *   and Player_LoadPartModels loads the parts whose id changed (Player_PartIdDiffers).
 *   Player_FreeModels and Player_FreeChangedPartModels free them.
 * - A hunter's part records (Player_InitPartTable) and weapon model, by weapon kind
 *   (Player_LoadWeaponModel, Player_FreeWeaponModel).
 * - The equipment preview (gEquipPreview, EquipPreview): the menus' model of one piece of
 *   equipment (EquipPreview_Load, EquipPreview_Free).
 *
 * A model is taken with Model_Alloc and Model_Reserve, set up from its pack with the
 * Model_Setup* functions, and freed slot by slot with Model_Free and Model_Release.
 */
#include "common.h"
#include "syssettings.h"
#include "model.h"
#include "texture.h"
#include "save.h"
#include "item.h"
#include "player.h"
#include "equip_preview.h"
#include "file_load.h"

extern u8 gModelUsed[];
extern PlRec gPlayerRecs[4][32];
/* unit's own declaration: the texture slot is declared s16, as the callers pass it; with the definition's s32 the call in Player_LoadWeaponModel compiles differently */
Model *Model_SetupWithPack(s16 idx, s32 modelFile, s16 texSlot, s32 packId, s32 setupFlags, s32 skelKind);
/* unit's own declaration: the texture slot is declared s16, as Player_AllocWeaponModel passes it; with the definition's s32 the call compiles differently */
Model *Model_SetupWithPackEx(s16 idx, s32 modelFile, s16 texSlot, s32 packId, s32 setupFlags, s32 skelKind, s32 src);

/*
 * Sets up player playerIdx: its 32 part records (gPlayerRecs, by its
 * index), no look parts (PL_LOOK_PART_NONE), no torso alternative, no
 * weapon model (PL_WEAPON_MODEL_NONE).
 */
void Player_InitPartTable(s32 playerIdx)
{
    Player *pl = &gPlayers[playerIdx];
    PlRec *rec = gPlayerRecs[pl->index];
    s32 i;

    for (i = 0; i < 32; i++) {
        pl->partRecs[i] = rec++;
    }
    pl->lookPartId[0] = PL_LOOK_PART_NONE;
    pl->lookPartId[1] = PL_LOOK_PART_NONE;
    pl->lookPartId[2] = PL_LOOK_PART_NONE;
    pl->lookPartId[3] = PL_LOOK_PART_NONE;
    pl->lookPartId[4] = PL_LOOK_PART_NONE;
    pl->lookPartId[5] = PL_LOOK_PART_NONE;
    pl->headAltModel = 0;
    pl->weaponModelKind = PL_WEAPON_MODEL_NONE;
    pl->index = playerIdx;
    pl->unk4C6 = 0;
}

/*
 * Loads the weapon model of kind for player playerIdx, unless already
 * loaded: the kind's model pack, then a model slot set up from the data
 * buffer, with its textures in slot playerIdx + 33.
 */
void Player_LoadWeaponModel(s32 kind, s32 playerIdx)
{
    Player *pl = &gPlayers[playerIdx];
    s32 modelIdx;

    if (pl->weaponModelKind == PL_WEAPON_MODEL_NONE || pl->weaponModelKind != (u16)kind) {
        Load_WeaponModelPack(kind);
        pl->weaponModelKind = kind;
        modelIdx = Model_Alloc(1);
        if (modelIdx >= 0) {
            pl->weaponModelSlot = modelIdx;
            pl->weaponModelSlotCount = 1;
            pl->weaponModel = Model_Get(modelIdx);
            Model_Reserve(modelIdx, 1);
            Model_SetupWithPack(modelIdx, (*(s32 *)&gSysSettings.dataBuf), playerIdx + 33, gModelPackIds[kind], 2304, 1);
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
        Model_SetupWithPackEx(modelIdx, (s32) modelFile, playerIdx + 33, gModelPackIds[kind], 2304, 1, (s32) extra);
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
    pl->weaponModelKind = PL_WEAPON_MODEL_NONE;
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
        pl->weaponModelKind = PL_WEAPON_MODEL_NONE;
        pl->weaponModel = NULL;
        pl->weaponModelSlot = 0;
        pl->weaponModelSlotCount = 0;
    }
}

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

/* unit's own declaration: the texture slot is declared s16, as the callers pass it; with the definition's s32 the calls in Player_LoadPartModels compile differently */
Model *Model_SetupLoadedPack(s16 idx, s32 modelFile, s16 texSlot, s32 *pack, s32 setupFlags, s32 secondId, s32 skelKind);
/* Per gender, then per look kind (the six equipment slots, then 6: the hair), the texture pack
 * of each part id (read here as two words each). */
extern TexPackHead *gPlayerLookFiles[];
extern s32 gEquipModelTexFilesM[];
extern s32 gEquipModelTexFilesF[];

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
            Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 358, gEquipModelTexFilesM[id], 2304, 1);
        } else {
            Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 358, gEquipModelTexFilesF[id], 2304, 1);
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
        pack = (s32 *)gPlayerLookFiles[type + female * 7];
        pack += id * 2;
        Model_SetupLoadedPack(idx, (*(s32 *)&gSysSettings.dataBuf), 358, pack, 2304,
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
        Model_SetupWithPack(idx, (*(s32 *)&gSysSettings.dataBuf), 358, gModelPackIds[id], 2304, 1);
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
            pack = (s32 *)gPlayerLookFiles[kind + pl->gender * 7];
            pack += id * 2;
            if (i == 0) {
                Model_SetupLoadedPack(model, (*(s32 *)&gSysSettings.dataBuf), pl->index * 12 + 43 + i * 2, pack, 2304,
                    (u16)(pl->lookVariant + pl->gender * 4), 2);
            } else {
                Model_SetupLoadedPack(model, (*(s32 *)&gSysSettings.dataBuf), pl->index * 12 + 43 + i * 2, pack, 2304,
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

/* The file's data: the look tables of the hunters' parts. */
s16 gLookPartMap0[32] = {
#include "sub_main/0042FC30.inc"
};
s16 gLookPartMap3[32] = {
#include "sub_main/0042FC70.inc"
};
s16 gLookPartMap4[32] = {
#include "sub_main/0042FCB0.inc"
};
TexPackHead *gPlayerLookFiles[16] = {
    gPlayerLookFiles_0, gPlayerLookFiles_1, gPlayerLookFiles_2, gPlayerLookFiles_3,
    gPlayerLookFiles_4, gPlayerLookFiles_5, gPlayerLookFiles_6, gPlayerLookFiles_7,
    gPlayerLookFiles_8, gPlayerLookFiles_9, gPlayerLookFiles_10, gPlayerLookFiles_11,
    gPlayerLookFiles_12, gPlayerLookFiles_13, NULL, NULL,
};
u8 gPlayerPartFlagTbls_0[240] = {
#include "sub_main/0042FD30.inc"
};
u8 gPlayerPartFlagTbls_2[240] = {
#include "sub_main/0042FE20.inc"
};
u8 gPlayerPartFlagTbls_3[240] = {
#include "sub_main/0042FF10.inc"
};
u8 gPlayerPartFlagTbls_4[240] = {
#include "sub_main/00430000.inc"
};
u8 gPlayerPartFlagTbls_5[240] = {
#include "sub_main/004300F0.inc"
};
u8 gPlayerPartFlagTbls_6[240] = {
#include "sub_main/004301E0.inc"
};
u8 gPlayerPartFlagTbls_8[240] = {
#include "sub_main/004302D0.inc"
};
u8 gPlayerPartFlagTbls_9[240] = {
#include "sub_main/004303C0.inc"
};
u8 gPlayerPartFlagTbls_10[240] = {
#include "sub_main/004304B0.inc"
};
u8 gPlayerPartFlagTbls_11[240] = {
#include "sub_main/004305A0.inc"
};
u8 *gPlayerPartFlagTbls[12] = {
    gPlayerPartFlagTbls_0, NULL, gPlayerPartFlagTbls_2, gPlayerPartFlagTbls_3,
    gPlayerPartFlagTbls_4, gPlayerPartFlagTbls_5, gPlayerPartFlagTbls_6, NULL,
    gPlayerPartFlagTbls_8, gPlayerPartFlagTbls_9, gPlayerPartFlagTbls_10, gPlayerPartFlagTbls_11,
};
