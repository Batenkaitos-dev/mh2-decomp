/*
 * The hunter's equipment and save data (gSave, SaveData in save.h): the
 * equipment tables' accessors (Equip_GetData and the price, name, rarity,
 * kind and class), the bowgun options (GUN_OPT_*), the players' versus
 * sides, the long sword's gauge, loading a player from the save or a
 * preset, the hunter rank (gHrRankPoints, gHrPointSteps and the rank
 * gates), the save's pouch, the item box and the pouch backup, the event
 * flags, the weapon and equipment stats, the equipment flags (EQUIP_FLAG_*),
 * the equipment box and the equipped items (SaveData.box, equip,
 * equipIdx), the forge's recipes and conditions, the hunting records by
 * monster group, the quest requirements, the village facilities, the
 * items obtained, the armour upgrade values, then the save's points,
 * titles and MH Portable link, and the decorations. The middle of the
 * file is data: the equipment tables (BladeData, GunData, ArmorData by
 * type) and their names, and the tables this code reads.
 *
 * The menus that show all this are the equipment menus, the status pages
 * and the lobby's forge and box menus; the pouch has its own code.
 *
 * The file is one of the game's compilation units: Deco_FitsEquip matches
 * only with Armor_GetData static, defined earlier in the same unit. The
 * "----" lines mark where its last two parts begin. The code is compiled
 * at optimization level 4 up to the second part and at the default (3)
 * from there (Equip_CompactDecorations needs 3). Its data interleave through it as well: no
 * cut before its last four functions keeps each side's data together.
 */
#include "common.h"
#include "player.h"
#include "item.h"
#include "save.h"
#include "em.h"
#include "scene.h"
#include "syssave.h"
#include "syssettings.h"
#include "lobby_state.h"
#include "quest.h"
#include "area.h"
#include "award.h"
#include "bone_effects.h"
#include "engine_math.h"
#include "game_util.h"
#include "hud.h"
#include "menu_steps.h"
#include "window.h"

#pragma optimization_level 4

extern u8 D_0044BF3B[];         /* class of the ids from 901 */
extern u8 gWeaponClassInfo[][2];      /* per weapon class: gunner flag, unk774 */
extern u8 gWeaponClassAttack[][2];      /* per weapon class: unk8A2, unk8A3 */
extern SaveData *gCardSlots;    /* the save slots read from the memory card */

extern u16 gRandSeeds[];

/* As Equip_SetFlag on save. */
s32 Player_HasSongEffect(Player *pl, s16 kind);
char *strcpy(char *dst, char *src);    /* strcpy */
s32 Award_Grant(s32 arg0);

/* A band of hunter ranks above 50 (gHrPointSteps, Hr_PointsForRank): the band's last rank and
 * the points each of its ranks adds. The list ends with a limit of 65535; the ranks up to 50
 * have their points in gHrRankPoints. */
typedef struct HrStep {
    s32 limit;                  /* the band's last rank */
    s32 step;                   /* the points per rank */
} HrStep;

extern u32 gHrRankPoints[];
extern HrStep gHrPointSteps[];
extern char gMyName[];


/* gNextQuest seen from its teams (+4), for the user ids of the versus sides at NextQuest +0x46
 * (sideIds: the lobby copies 6 bytes per side there). Players_UpdateSides reads them from a base
 * at +4 with the offset 0x42, so the view keeps that base for the match. */
typedef struct SideIdsView {
    u8 teams[66];             /* NextQuest.teams, teamItems, unk44 */
    s8 ids[4][6];               /* NextQuest.sideIds */
} SideIdsView;

extern ItemStack gPouchBackup[20];
extern UserText gPlayerCardTexts[];     /* the party's hunter card texts, one a player, found by the id (Preset_Find) */
extern char gMemberNames[][17];   /* the preset names */
extern s8 gPlayerIds[][8];      /* the preset ids */
extern NextQuest gNextQuest;
extern s8 gMyUserId[8];
/* gScene.userIds as its own symbol: with gScene.userIds[i] MWCC folds the field's offset 0x276 into the stores. */
extern s8 D_006ADF26[][8];

void *memcpy(void *dst, void *src, u32 n);
s32 SavePouch_Count(u16 item);
void Em_BonePos(void *obj, s16 bone, f32 *out);

extern Em gEms[];

extern u8 gAmmoLevels[][4];      /* per ammo kind: +0, +1 compared with the gun */
extern EquipItem gEquipShown[7]; /* the equipment shown: [0] the weapon, [1 + type] each armour piece (EquipType 0-5) */
extern s16 gSilencerBonus;
extern s16 gBarrelBonusHeavy[];
extern s16 gBarrelBonusLight[];
extern u16 gClearListQuests[];        /* item ids, ended by 60000 (bits in gSave unk1FFC) */


/* The data record of a piece of equipment (its BladeData, GunData or ArmorData, by its type), NULL
 * for an unknown type. */
EquipData *Equip_GetData(EquipItem *item)
{
    switch (item->type) {
    case EQUIP_LEGS:
        return (EquipData *)&gArmorLegsData[item->id];
    case EQUIP_HEAD:
        return (EquipData *)&gArmorHeadData[item->id];
    case EQUIP_BODY:
        return (EquipData *)&gArmorBodyData[item->id];
    case EQUIP_ARMS:
        return (EquipData *)&gArmorArmsData[item->id];
    case EQUIP_WAIST:
        return (EquipData *)&gArmorWaistData[item->id];
    case EQUIP_BLADE:
        return (EquipData *)&gBladeData[item->id];
    case EQUIP_GUN:
        return (EquipData *)&gGunData[item->id];
    }
    return NULL;
}

/* The weapon kind of a weapon (BladeData.kind or GunData.kind, a row of gWeaponKindClass); 0 for
 * armour. */
u16 Equip_GetKind(EquipItem *item)
{
    if (item->type == EQUIP_BLADE) {
        return gBladeData[item->id].kind;
    }
    if (item->type == EQUIP_GUN) {
        return gGunData[item->id].kind;
    }
    return 0;
}

/* The weapon class of a weapon. */
u8 Equip_GetClass(EquipItem *item)
{
    return gWeaponKindClass[Equip_GetKind(item)];
}

/* A blade weapon's special value (BladeData.special: a gunlance's shelling type, a horn's notes); 0
 * for no item or a gun. */
u8 Equip_GetBladeSpecial(EquipItem *item)
{
    if (item->id == 0) {
        return 0;
    }
    if (item->type == EQUIP_BLADE) {
        return gBladeData[item->id].special;
    }
    return 0;
}

/* The weapon class (WEAPON_*) of a weapon by its type and id. Ids above 900 read the byte table
 * that starts 11 bytes into gStageFileTbls_4, at the id: no weapon table goes that far.
 */
u8 Weapon_GetClass(u8 type, u16 id)
{
    if (id > 900) {
        return D_0044BF3B[id];
    }
    if (type == EQUIP_BLADE) {
        return gWeaponKindClass[gBladeData[id].kind];
    }
    if (type == EQUIP_GUN) {
        return gWeaponKindClass[gGunData[id].kind];
    }
    return 0;
}

/* 1 for the gunner weapon classes (gWeaponClassInfo[class][0]). */
u8 WeaponClass_IsGunner(u8 cls)
{
    return gWeaponClassInfo[cls][0];
}

/* The flags of a weapon class (gWeaponClassInfo[class][1]), kept in Player.weaponFlags. */
u8 WeaponType_Flags(u8 cls)
{
    return gWeaponClassInfo[cls][1];
}

/* The price of a piece of equipment by its type and id; 9,999,999 for an unknown type. */
u32 Equip_GetPrice(u8 type, u16 id)
{
    switch (type) {
    case EQUIP_LEGS:
        return gArmorLegsData[id].price;
    case EQUIP_HEAD:
        return gArmorHeadData[id].price;
    case EQUIP_BODY:
        return gArmorBodyData[id].price;
    case EQUIP_ARMS:
        return gArmorArmsData[id].price;
    case EQUIP_WAIST:
        return gArmorWaistData[id].price;
    case EQUIP_BLADE:
        return gBladeData[id].price;
    case EQUIP_GUN:
        return gGunData[id].price;
    }
    return 9999999;
}

/* The name of a piece of equipment by its type and id (the name tables at the end of this file);
 * NULL for an unknown type. */
char *Equip_GetName(u8 type, u16 id)
{
    switch (type) {
    case EQUIP_LEGS:
        return gArmorLegsNames[id];
    case EQUIP_HEAD:
        return gArmorHeadNames[id];
    case EQUIP_BODY:
        return gArmorBodyNames[id];
    case EQUIP_ARMS:
        return gArmorArmsNames[id];
    case EQUIP_WAIST:
        return gArmorWaistNames[id];
    case EQUIP_BLADE:
        return gBladeNames[id];
    case EQUIP_GUN:
        return gGunNames[id];
    }
    return 0;
}

/* The rarity of a piece of equipment by its type and id (the low 4 bits of its rarity byte); 0 for
 * an unknown type. */
u8 Equip_GetRarity(u8 type, u16 id)
{
    switch (type) {
    case EQUIP_LEGS:
        return gArmorLegsData[id].rarity & 0xF;
    case EQUIP_HEAD:
        return gArmorHeadData[id].rarity & 0xF;
    case EQUIP_BODY:
        return gArmorBodyData[id].rarity & 0xF;
    case EQUIP_ARMS:
        return gArmorArmsData[id].rarity & 0xF;
    case EQUIP_WAIST:
        return gArmorWaistData[id].rarity & 0xF;
    case EQUIP_BLADE:
        return gBladeData[id].rarity & 0xF;
    case EQUIP_GUN:
        return gGunData[id].rarity & 0xF;
    }
    return 0;
}

/* The carried item in the player's pouch (ITEM_ID_CANNON_AMMO, the items 670-674 or ITEM_ID_DELIVERY_CARRIED, which Item_CarryKind
 * knows), 65535 if none; with more than one, the last slot's. */
u16 Pouch_FindCarryItem(Player *pl)
{
    u16 found = 65535;
    s16 i;

    for (i = 0; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].count != 0) {
            switch (pl->pouch[i].item) {
            case ITEM_ID_CANNON_AMMO:
            case 670:
            case 671:
            case 672:
            case 673:
            case 674:
            case ITEM_ID_DELIVERY_CARRIED:
                found = pl->pouch[i].item;
                break;
            }
        }
    }
    return found;
}

/* The kind of a carried item (Player.carry: 1-7 for ITEM_ID_CANNON_AMMO, the items 670-674 and ITEM_ID_DELIVERY_CARRIED, in the order the
 * switch gives), 0 for any other item. */
u8 Item_CarryKind(u16 item)
{
    switch (item) {
    case 670:
        return 1;
    case 671:
        return 2;
    case ITEM_ID_CANNON_AMMO:
        return 3;
    case 674:
        return 4;
    case 673:
        return 5;
    case ITEM_ID_DELIVERY_CARRIED:
        return 6;
    case 672:
        return 7;
    }
    return 0;
}

/* 1 when the player carries well: with SKILL_CARRY_BONUS, or carrying an item of kind 6 (Player.carry's
 * high nibble). Seen: the carry walk takes a longer stride, and a fall must last longer before the
 * item drops (the game's carry states). */
s32 Player_HasCarryBonus(Player *pl)
{
    if (Player_HasSkill(pl, SKILL_CARRY_BONUS) == 1) {
        return 1;
    }
    if ((u8)Byte_HighNibble(pl->carry) == 6) {
        return 1;
    }
    return 0;
}

/* The high 4 bits of a byte. */
s32 Byte_HighNibble(u8 x)
{
    return (u8)((x & 0xF0) >> 4);
}

/* 1 when the player's weapon is a gun with GUN_OPT_SCOPE (the aim camera then zooms). */
s32 Bowgun_HasScope(Player *pl)
{
    if (pl->equip[EQUIP_SLOT_WEAPON].type != EQUIP_GUN) {
        return 0;
    }
    if (pl->equip[EQUIP_SLOT_WEAPON].level & GUN_OPT_SCOPE) {
        return 1;
    }
    return 0;
}

/* 1 when the player's weapon is a light bowgun with GUN_OPT_SILENCER. */
s32 LightBowgun_HasSilencer(Player *pl)
{
    s32 ret;

    if (pl->equip[EQUIP_SLOT_WEAPON].type != EQUIP_GUN) {
        ret = 0;
    } else if (pl->weaponType != WEAPON_LIGHT_BOWGUN) {
        ret = 0;
    } else {
        ret = (pl->equip[EQUIP_SLOT_WEAPON].level & GUN_OPT_SILENCER) != 0;
    }
    return ret;
}

/* 1 when the player's weapon is a heavy bowgun with GUN_OPT_SHIELD (it then guards as a shield
 * skill would, in the hit tests). */
s32 HeavyBowgun_HasShield(Player *pl)
{
    s32 ret;

    if (pl->equip[EQUIP_SLOT_WEAPON].type != EQUIP_GUN) {
        ret = 0;
    } else if (pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        ret = 0;
    } else {
        ret = (pl->equip[EQUIP_SLOT_WEAPON].level & GUN_OPT_SHIELD) != 0;
    }
    return ret;
}

/* 1 when the player's weapon is a gun with GUN_OPT_BARREL. */
s32 Bowgun_HasBarrel(Player *pl)
{
    if (pl->equip[EQUIP_SLOT_WEAPON].type != EQUIP_GUN) {
        return 0;
    }
    if (pl->equip[EQUIP_SLOT_WEAPON].level & GUN_OPT_BARREL) {
        return 1;
    }
    return 0;
}

/* How slowly the gun reloads an ammo kind: 0, 1 or 2 (the reload text's row). The ammo's own value
 * (gAmmoLevels[ammo][1]) less the gun's GunData.reload, less 1, 2 or 3 for SKILL_RELOAD_PLUS1, _PLUS2 and _PLUS3, and
 * plus 1 or 3 for SKILL_RELOAD_MINUS1 and _MINUS3; up to 4 gives 0, up to 7 gives 1. */
u8 Bowgun_ReloadLevel(Player *pl, u8 ammo)
{
    s16 level = gAmmoLevels[ammo][1] - gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].reload;

    if (Player_HasSkill(pl, SKILL_RELOAD_PLUS1) == 1) {
        level -= 1;
    }
    if (Player_HasSkill(pl, SKILL_RELOAD_PLUS2) == 1) {
        level -= 2;
    }
    if (Player_HasSkill(pl, SKILL_RELOAD_PLUS3) == 1) {
        level -= 3;
    }
    if (Player_HasSkill(pl, SKILL_RELOAD_MINUS1) == 1) {
        level += 1;
    }
    if (Player_HasSkill(pl, SKILL_RELOAD_MINUS3) == 1) {
        level += 3;
    }
    if (level <= 4) {
        return 0;
    } else if (level <= 7) {
        return 1;
    }
    return 2;
}

/* How hard an ammo kind kicks back with the player's gun: 0, 1 or 2 (the recoil text's row); 2
 * without a gun. The ammo's own value (gAmmoLevels[ammo][0]), and unless SKILL_FREE_SHOTS cancels it,
 * less the gun's GunData.recoil and less 2 or 4 for SKILL_RECOIL_DOWN1 or _DOWN2; up to 8 gives 0, up to
 * 10 gives 1. */
s16 Bowgun_RecoilLevel(Player *pl, u8 ammo)
{
    s16 level;

    if (pl->equip[EQUIP_SLOT_WEAPON].type != EQUIP_GUN) {
        return 2;
    }
    level = gAmmoLevels[ammo][0];
    if (Player_HasSkill(pl, SKILL_FREE_SHOTS) == 0) {
        level -= gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].recoil;
        if (Player_HasSkill(pl, SKILL_RECOIL_DOWN1) == 1) {
            level -= 2;
        } else if (Player_HasSkill(pl, SKILL_RECOIL_DOWN2) == 1) {
            level -= 4;
        }
    }
    if (level <= 0) {
        level = 0;
    }
    if (level <= 8) {
        return 0;
    } else if (level <= 10) {
        return 1;
    }
    return 2;
}

/* Leaves the bowgun's aim: clears the aim mode, its lock timer and the pitch. */
void Player_ResetAim(Player *pl)
{
    pl->aimMode = 0;
    pl->aimLockTimer = 0;
    pl->aimPitch = 0;
}

/* Which horn item a player on side sideWanted is playing: 1-7 for ITEM_ID_HORN_ITEM_A to ITEM_ID_HORN_ITEM_LONG_B, from the
 * first of the four players on that side (and not flagged by unk69E[0]) whose state is
 * PL_ACT_HORN_ITEM_PLAY (the group isn't checked); 0 if none. Players with no side don't count. */
u16 Side_HornItemPlaying(u8 sideWanted)
{
    u16 played = 0;
    s16 i;
    Player *pl;

    for (i = 0, pl = gPlayers; i < SCENE_PLAYERS; i++, pl++) {
        if (pl->active == 0) {
            continue;
        }
        if (Side_Match(pl->side, sideWanted, 1) == 0) {
            continue;
        }
        if (pl->unk69E[0] != 0) {
            continue;
        }
        switch (pl->state) {
        case PL_ACT_HORN_ITEM_PLAY:
            switch (pl->useItem) {
            case ITEM_ID_HORN_ITEM_A:
                played = 1;
                break;
            case ITEM_ID_HORN_ITEM_B:
                played = 2;
                break;
            case ITEM_ID_HORN_ITEM_SHORT:
                played = 3;
                break;
            case ITEM_ID_HORN_ITEM_LONG:
                played = 4;
                break;
            case ITEM_ID_HORN_ITEM_C:
                played = 5;
                break;
            case ITEM_ID_HORN_ITEM_D:
                played = 6;
                break;
            case ITEM_ID_HORN_ITEM_LONG_B:
                played = 7;
                break;
            }
            return played;
        }
    }
    return 0;
}

/* 1 when b is on a's side: a has no side, or both have the same bit 0 of Player.side. */
s32 Player_OnSameSide(Player *pl, Player *other)
{
    if (pl->side == 0) {
        return 1;
    }
    if ((pl->side & 1) == (other->side & 1)) {
        return 1;
    }
    return 0;
}

/* 1 when one of the four players has a side (Player.side nonzero). */
s32 Players_AnySide(void)
{
    Player *pl = gPlayers;
    s16 i;

    for (i = 0; i < SCENE_PLAYERS; i++, pl++) {
        if (pl->side != 0) {
            return 1;
        }
    }
    return 0;
}

/* 1 when two sides match (the same bit 0). When either is 0 (no side), the result is noSideFails's
 * opposite: 1 when it is 0, else 0. */
s32 Side_Match(u8 sideA, u8 sideB, u8 noSideFails)
{
    s32 ret;

    if (sideA == 0 || sideB == 0) {
        if (noSideFails == 0) {
            ret = 1;
        } else {
            ret = 0;
        }
    } else {
        ret = (sideA & 1) == (sideB & 1);
    }
    return ret;
}

/* A long sword's spirit gauge (Player.swordGauge): adds amount, within 0 to PL_SWORD_GAUGE_MAX. Filling it starts
 * the full gauge's effect at bone 10 (the local player only, once), keeps it full for 900 frames
 * (swordGaugeTimer), and gives an attack buff of 25 for as long unless a larger one is on. */
void LongSword_AddGauge(Player *pl, s16 amount)
{
    if (pl->weaponType == WEAPON_LONG_SWORD) {
        pl->swordGauge += amount;
        if (amount >= 0) {
            if (pl->swordGauge >= PL_SWORD_GAUGE_MAX) {
                pl->swordGauge = PL_SWORD_GAUGE_MAX;
                if (Player_IsLocal(pl) == 1 && pl->swordGaugeTimer == 0) {
                    Effect06_SpawnAtBone(pl, 0, 2, 10, 4.0f);
                }
                pl->swordGaugeTimer = 900;
                if (pl->attackBuff < 26) {
                    pl->attackBuff = 25;
                    pl->attackBuffTimer = pl->swordGaugeTimer;
                }
            }
        } else if (pl->swordGauge < 0) {
            pl->swordGauge = 0;
        }
    }
}

/* 1 when the player wields a long sword whose spirit gauge is at least amount. */
s32 LongSword_GaugeAtLeast(Player *pl, s16 amount)
{
    if (pl->weaponType != WEAPON_LONG_SWORD) {
        return 0;
    }
    if (pl->swordGauge >= amount) {
        return 1;
    }
    return 0;
}

/* Sets the player's attack percent and its base from the weapon class (gWeaponClassAttack). */
void Player_ResetAttackPercent(Player *pl)
{
    pl->attackPercent = gWeaponClassAttack[pl->weaponType][0];
    pl->attackPercentBase = gWeaponClassAttack[pl->weaponType][1];
}

/* How far to raise the player's name tag: 40 when the head armour's model (male or female, by the
 * player's gender) is one of the tall ones the switch lists, else 0. */
f32 Player_NameTagRaise(Player *pl)
{
    ArmorData *head = gArmorTables[EQUIP_HEAD];
    f32 raise = 0.0f;

    head += pl->equip[EQUIP_HEAD].id;

    if (pl->gender == 0) {
        switch (head->modelMale) {
        case 2:
        case 20:
        case 32:
        case 36:
        case 78:
        case 88:
        case 99:
        case 100:
        case 107:
        case 108:
        case 119:
        case 139:
        case 171:
        case 185:
        case 191:
        case 201:
        case 203:
        case 216:
        case 217:
        case 218:
            raise = 40.0f;
            break;
        }
    } else {
        switch (head->modelFemale) {
        case 32:
        case 42:
        case 107:
        case 108:
        case 122:
        case 124:
        case 127:
        case 139:
        case 140:
        case 144:
        case 149:
        case 201:
        case 203:
        case 216:
        case 217:
        case 218:
            raise = 40.0f;
            break;
        }
    }
    return raise;
}

/* 1 when the local player's stamina doesn't drain: an item's effect is on (staminaLockTimer), or
 * the song effect 5 (PL_SONG_STAMINA). Always 0 for the other players. */
s32 Player_StaminaLocked(Player *pl)
{
    s32 ret;

    if (Player_IsLocal(pl) == 0) {
        ret = 0;
    } else if (pl->staminaLockTimer > 0) {
        ret = 1;
    } else {
        ret = Player_HasSongEffect(pl, 5) == 1;
    }
    return ret;
}

/* 1 when a Lunastra or Teostra in the player's area, visible, with its aura on (Em.windTimer), has
 * its bone 2 within 750 times its scale of the player. The callers then count the player as in
 * hazard 2 (Player.floorHazard). */
s32 Player_InEmAura(Player *pl)
{
    Em *em = gEms;
    s16 i;
    f32 bonePos[4];

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active != 0 && em->visible != 0 && em->area == pl->area && em->windTimer != 0) {
            switch (em->type) {
            case EM_KIND_TEOSTRA:
            case EM_KIND_LUNASTRA:
                Em_BonePos(em, 2, bonePos);
                if (Vec_Distance(pl->pos, bonePos) <= 750.0f * em->scaleX) {
                    return 1;
                }
                break;
            }
        }
    }
    return 0;
}

/* 1 while the player is in a poisoning area (Player.inPoisonArea), outside the town only. The HUD shows the
 * poison icon for it. */
s32 Player_InPoisonArea(Player *pl)
{
    if (gScene.inLobby != 0) {
        return 0;
    }
    return pl->inPoisonArea != 0;
}

/* Loads save slot `slot` into the copy gSysSave.save and the save data. */
void Save_LoadSlot(u8 slot)
{
    SaveData *slotData;

    slotData = gCardSlots;
    slotData += slot;
    gSysSave.save = *slotData;
    gSave = gSysSave.save;
    gRandSeeds[2] = gSave.randSaved[0];
    PspLink_ClearFlags();
}

/* Sets up a player from save data: the equipment and what follows from
 * the weapon (class, gunner flag). */
void Player_SetupFromSave(Player *pl, SaveData *save)
{
    Equip_Refresh(save);
    pl->gender = save->gender;
    pl->equip[EQUIP_SLOT_FACE].id = save->face + 1;
    pl->hairStyle = save->hairStyle;
    pl->hairColor = save->hairColor;
    Player_SetColorWord(pl, save->symbolColor);
    pl->voice = save->voice;
    pl->charType = save->charType;
    pl->weaponKind = Equip_GetKind(&save->equip[SAVE_EQUIP_WEAPON]);
    pl->equip[EQUIP_SLOT_WEAPON] = save->equip[SAVE_EQUIP_WEAPON];
    pl->weaponType = gWeaponKindClass[pl->weaponKind];
    pl->isGunner = WeaponClass_IsGunner(pl->weaponType);
    pl->weaponFlags = WeaponType_Flags(pl->weaponType);
    pl->bladeSpecial = Equip_GetBladeSpecial(&pl->equip[EQUIP_SLOT_WEAPON]);
    pl->equip[EQUIP_LEGS] = save->equip[SAVE_EQUIP_LEGS];
    pl->equip[EQUIP_HEAD] = save->equip[SAVE_EQUIP_HEAD];
    pl->equip[EQUIP_BODY] = save->equip[SAVE_EQUIP_BODY];
    pl->equip[EQUIP_ARMS] = save->equip[SAVE_EQUIP_ARMS];
    pl->equip[EQUIP_WAIST] = save->equip[SAVE_EQUIP_WAIST];
}

/* Loads the local hunter from gSave: the name, the pouch (and its copy gPouchBackup, for the local
 * player), the voice and the weapon, then Player_SetupFromSave. */
void Player_LoadFromSave(Player *pl)
{
    SaveData *save = &gSave;
    s16 i;

    for (i = 0; i < CHARA_NAME_LEN; i++) {
        pl->name[i] = save->name[i];
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        pl->pouch[i].item = save->pouch[i].item;
        pl->pouch[i].count = save->pouch[i].count;
        if (Player_IsLocal(pl) == 1) {
            gPouchBackup[i].item = save->pouch[i].item;
            gPouchBackup[i].count = save->pouch[i].count;
        }
    }
    pl->voice = save->voice;
    pl->weaponKind = Equip_GetKind(&save->equip[SAVE_EQUIP_WEAPON]);
    pl->equip[EQUIP_SLOT_WEAPON] = save->equip[SAVE_EQUIP_WEAPON];
    Player_SetupFromSave(pl, save);
}

/* Sets the player's name: the online name gMyName when online, else the save's. */
void Player_LoadName(Player *pl)
{
    SaveData *save = &gSave;
    s16 i;

    if (Game_IsOnline() == 1) {
        strcpy((char *)pl->name, gMyName);
        return;
    }
    for (i = 0; i < CHARA_NAME_LEN; i++) {
        pl->name[i] = save->name[i];
    }
}

/* Copies the player's pouch into gSave. */
void Save_StorePouch(Player *pl)
{
    memcpy(gSave.pouch, pl->pouch, sizeof(gSave.pouch));
}

/* Copies gSave's pouch into the player's. */
void Save_RestorePouch(Player *pl)
{
    memcpy(pl->pouch, gSave.pouch, sizeof(gSave.pouch));
}

/* Adds money (or takes it, with a negative amount), within 0 to 9,999,999. Having 2,000,000 or more
 * grants AWARD_MONEY. */
void Save_AddMoney(s32 n)
{
    gSave.money += n;
    if (gSave.money > MONEY_MAX) {
        gSave.money = MONEY_MAX;
    }
    if (gSave.money < 0) {
        gSave.money = 0;
    } else if (gSave.money >= 2000000) {
        Award_Grant(AWARD_MONEY);
    }
}

/* The hunter rank points at which a rank begins: from gHrRankPoints up to rank 50, then by
 * gHrPointSteps (bands of 50 ranks, each with its points per rank); 10,000,000 above rank 999. */
u32 Hr_PointsForRank(u16 rank)
{
    u32 points;
    u16 bandStart;
    HrStep *band;
    u32 result;

    if (rank == 0) {
        return 0;
    }
    if (rank > HR_MAX) {
        return 10000000;
    }
    points = gHrRankPoints[HR_TABLE_MAX];
    band = gHrPointSteps;
    result = 10000000;
    if (rank <= HR_TABLE_MAX) {
        result = gHrRankPoints[rank];
    } else {
        bandStart = HR_TABLE_MAX;
        while (band->limit != 0xFFFF) {
            if (rank <= (u16)band->limit) {
                result = points + band->step * (rank - bandStart);
                break;
            }
            points += band->step * HR_BAND_RANKS;
            bandStart += HR_BAND_RANKS;
            band++;
        }
    }
    return result;
}

/* The hunter rank the save's points reach (HR_MAX at most); 0 for a save whose rank is still 0. */
u16 Save_RankFromPoints(SaveData *save)
{
    s16 i;
    u32 maxPoints;

    if (save->hr == 0) {
        return 0;
    }
    maxPoints = Hr_PointsForRank(HR_MAX);
    if (save->hrPoints >= maxPoints) {
        return HR_MAX;
    }
    for (i = 0; i < HR_MAX + 1; i++) {
        if (Hr_PointsForRank(i) > save->hrPoints) {
            return i - 1;
        }
    }
    return 0;
}

/* Gives the save's rank, its points and the points of the next rank (0 at rank HR_MAX). A save whose
 * rank is 0 reads as rank 1. */
void Save_GetRankInfo(SaveData *save, u16 *rank, u32 *points, u32 *nextPoints)
{
    s16 i;
    u32 maxPoints;

    if (save->hr == 0) {
        *rank = 1;
        *points = save->hrPoints;
        *nextPoints = 0;
        return;
    }
    *points = save->hrPoints;
    maxPoints = Hr_PointsForRank(HR_MAX);
    if (save->hrPoints >= maxPoints) {
        *rank = HR_MAX;
        *nextPoints = 0;
        return;
    }
    for (i = 0; i < HR_MAX + 1; i++) {
        if (Hr_PointsForRank((u16)i) > *points) {
            *rank = (u16)i - 1;
            *nextPoints = Hr_PointsForRank((u16)i);
            return;
        }
    }
}

/* Sets the save's points to one short of the given rank: the rank gates stop there. */
void Save_SetRankCapPoints(SaveData *save, s32 rank)
{
    save->hrPoints = Hr_PointsForRank(rank) - 1;
}

/* Adds hunter rank points to a save (or takes them), within 0 and 9,999,999, and updates its rank.
 * Ranks 11, 17, 31, 41 and 51 are gates (Save_HrGateCleared 0-4): until a gate's quest is cleared,
 * the points stop one short of it. Returns 0, 1 when the points fell below the rank's start (or
 * would have gone below 0), 2 at rank 999, or 3, 4, 5, 6 or 8 for the gate that stopped them. */
s32 Save_AddHrPoints(SaveData *save, s32 points)
{
    u16 rank = save->hr;
    u8 result = 0;
    u32 minPoints;

    if (points < 0 && save->hrPoints < (u32)Math_Absf(points)) {
        save->hrPoints = 0;
        result = 1;
    } else {
        save->hrPoints += points;
    }
    if (save->hrPoints > HR_POINTS_MAX) {
        save->hrPoints = HR_POINTS_MAX;
    }
    minPoints = Hr_PointsForRank(rank);
    if (save->hrPoints < minPoints) {
        save->hrPoints = minPoints;
        save->hr = Save_RankFromPoints(&gSave);
        return 1;
    }
    if (rank >= HR_MAX) {
        return 2;
    }
    do {
        if (Hr_PointsForRank(rank + 1) > save->hrPoints) {
            save->hr = Save_RankFromPoints(&gSave);
            return result;
        }
        switch ((s32)rank + 1) {
        case 11:
            if (Save_HrGateCleared(0) == 1) {
                rank++;
            } else {
                Save_SetRankCapPoints(save, 11);
                result = 3;
            }
            break;
        case 17:
            if (Save_HrGateCleared(1) == 1) {
                rank++;
            } else {
                Save_SetRankCapPoints(save, 17);
                result = 4;
            }
            break;
        case 31:
            if (Save_HrGateCleared(2) == 1) {
                rank++;
            } else {
                Save_SetRankCapPoints(save, 31);
                result = 5;
            }
            break;
        case 41:
            if (Save_HrGateCleared(3) == 1) {
                rank++;
            } else {
                Save_SetRankCapPoints(save, 41);
                result = 6;
            }
            break;
        case 51:
            if (Save_HrGateCleared(4) == 1) {
                rank++;
            } else {
                Save_SetRankCapPoints(save, 51);
                result = 8;
            }
            break;
        default:
            rank++;
            break;
        }
    } while (result == 0);
    save->hr = Save_RankFromPoints(&gSave);
    return result;
}

/* Save_AddHrPoints on gSave. */
s32 Hr_AddPoints(s32 points)
{
    return Save_AddHrPoints(&gSave, points);
}

/* 1 for the items 625-654, which are never carried: getting one only marks it obtained
 * (SavePouch_Add). */
s32 Item_IsRecordOnly(u16 item)
{
    if (item >= 625 && item < 655) {
        return 1;
    }
    return 0;
}

/* Adds amount of an item to gSave's pouch (or takes it, with a negative amount), within the item's
 * stack size. A record-only item is only marked obtained (with the monster notes, and a notice
 * in the town). Returns 0 added to an empty slot (or a record-only item), 1 changed, 2 capped at the
 * stack size, 3 the stack was already full (or a second special item), 4 the stack ran out and its
 * slot was cleared, 5 no free slot. */
u8 SavePouch_Add(u16 item, s16 amount)
{
    s16 i;
    u8 result;
    s16 stackMax;
    SaveData *save = &gSave;

    if (Item_IsRecordOnly(item) == 1) {
        if (Save_IsItemObtained(save, item) == 0) {
            MonsterNote_OnItemGet(item);
            Save_MarkItemObtained(save, item);
            if (gScene.inLobby != 0) {
                Menu_PushNoticeOnce(4);
            }
        }
        return 0;
    }
    if (gItemData[item].max == 255) {
        amount = 255;
    }
    if (SavePouch_Count(item) == 0) {
        result = 5;
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (save->pouch[i].item == 0 && amount > 0) {
                stackMax = gItemData[item].max;
                save->pouch[i].item = item;
                if (amount > stackMax) {
                    amount = stackMax;
                }
                save->pouch[i].count = amount;
                Save_MarkItemObtained(save, item);
                result = 0;
                break;
            }
        }
    } else if (amount > 0 && Item_IsOnePerPouch(item) == 1) {
        result = 3;
    } else {
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (save->pouch[i].item == item) {
                stackMax = gItemData[item].max;

                if (amount > 0 && save->pouch[i].count >= stackMax) {
                    save->pouch[i].count = stackMax;
                    Save_MarkItemObtained(save, item);
                    result = 3;
                    break;
                }
                if (amount < 0 && gItemData[item].max == 255) {
                    result = 1;
                    break;
                }
                save->pouch[i].count += amount;
                if (save->pouch[i].count <= 0) {
                    SavePouch_ClearSlot(i);
                    result = 4;
                    save->pouch[i].item = 0;
                    save->pouch[i].count = 0;
                    break;
                }
                if (stackMax < save->pouch[i].count) {
                    save->pouch[i].count = stackMax;
                    result = 2;
                } else {
                    result = 1;
                }
                Save_MarkItemObtained(save, item);
                break;
            }
        }
    }
    /* the game reads a register it never set: when the second loop finds no slot holding item (SavePouch_Count said there is one), result is whatever the caller left in that register */
    return result;
}

/* 1 when the save's pouch holds a special item (Item_IsOnePerPouch: only one may be carried). */
s32 SavePouch_HasSpecial(SaveData *save)
{
    s16 i;

    for (i = 0; i < POUCH_SLOTS; i++) {
        if (Item_IsOnePerPouch(save->pouch[i].item) == 1) {
            return 1;
        }
    }
    return 0;
}

/* How many of an item gSave's pouch holds; for a special item, 1 when the pouch holds any special
 * item. */
s32 SavePouch_Count(u16 item)
{
    SaveData *save = &gSave;
    s32 i;

    if (Item_IsOnePerPouch(item) == 1) {
        return SavePouch_HasSpecial(save) == 1;
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (save->pouch[i].item == item) {
            return save->pouch[i].count;
        }
    }
    return 0;
}

/* How many more of an item fit in gSave's pouch's stack of it (the full stack size when there is
 * none; ITEM_MAX_UNLIMITED for an item with no limit); for a special item, 1 when the pouch holds none. */
s32 SavePouch_Room(u16 item)
{
    SaveData *save = &gSave;
    s32 i;

    if (Item_IsOnePerPouch(item) == 1) {
        return SavePouch_HasSpecial(save) != 1;
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (save->pouch[i].item == item) {
            if (gItemData[item].max == ITEM_MAX_UNLIMITED) {
                return ITEM_MAX_UNLIMITED;
            }
            /* A return in both branches, as the game has it (each jumps to the
             * epilogue): the first probably held a print compiled out. */
            if (save->pouch[i].count > gItemData[item].max) {
                return gItemData[item].max - save->pouch[i].count;
            }
            return gItemData[item].max - save->pouch[i].count;
        }
    }
    return gItemData[item].max;
}

/* As SavePouch_Room, and -1 when the pouch has neither a stack of the item nor a free slot; 0 for a
 * special item when one is held, and for a record-only item 1 until it is obtained. */
s32 SavePouch_Space(u16 item)
{
    SaveData *save = &gSave;
    s32 i;
    s32 empty;

    if (Item_IsOnePerPouch(item) == 1 && SavePouch_HasSpecial(save) == 1) {
        return 0;
    }
    if (Item_IsRecordOnly(item) == 1) {
        return Save_IsItemObtained(save, item) == 0;
    }
    empty = 0;
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (save->pouch[i].item == item) {
            if (gItemData[item].max == 255) {
                return 255;
            }
            /* A return in both branches, as the game has it (each jumps to the
             * epilogue): the first probably held a print compiled out. */
            if (save->pouch[i].count > gItemData[item].max) {
                return gItemData[item].max - save->pouch[i].count;
            }
            return gItemData[item].max - save->pouch[i].count;
        }
        if (save->pouch[i].item == 0) {
            empty++;
        }
    }
    if (empty == 0) {
        return -1;
    }
    return gItemData[item].max;
}

/* 1 when gSave's pouch has an empty slot. */
s16 SavePouch_HasFreeSlot(void)
{
    s16 i;
    SaveData *save = &gSave;

    for (i = 0; i < POUCH_SLOTS; i++) {
        if (save->pouch[i].item == 0) {
            return 1;
        }
    }
    return 0;
}

/* The number of empty slots in gSave's pouch. */
s16 SavePouch_CountFree(void)
{
    s16 i;
    s16 count = 0;
    SaveData *save = &gSave;

    for (i = 0; i < POUCH_SLOTS; i++) {
        if (save->pouch[i].item == 0) {
            count++;
        }
    }
    return count;
}

/* Empties a slot of gSave's pouch. Returns 1 when it held an item, 0 when it was empty, 2 for a
 * slot out of range. */
s32 SavePouch_ClearSlot(s16 slot)
{
    SaveData *save = &gSave;

    if (slot >= POUCH_SLOTS) {
        return 2;
    }
    save->pouch[slot].count = 0;
    if (save->pouch[slot].item == 0) {
        return 0;
    }
    save->pouch[slot].item = 0;
    return 1;
}

/* How many of an item the item box's first stack of it holds (999 for an item with no limit), 0 if
 * none. */
s32 ItemBox_Count(u16 item)
{
    SaveData *save = &gSave;
    s16 capacity = Box_GetCapacity(save);
    s32 i;

    for (i = 0; i < capacity; i++) {
        if (save->itemBox[i].item == item) {
            return gItemData[item].max == 255 ? 999 : save->itemBox[i].count;
        }
    }
    return 0;
}

/* How many of an item the item box holds in all its stacks (999 for an item with no limit). */
s32 ItemBox_Total(u16 item)
{
    SaveData *save = &gSave;
    s16 i;
    s16 capacity = Box_GetCapacity(save);
    s32 total = 0;

    for (i = 0; i < capacity; i++) {
        if (save->itemBox[i].item == item) {
            if (gItemData[item].max == 255) {
                return 999;
            }
            total += save->itemBox[i].count;
        }
    }
    return total;
}

/* How many more of an item the item box can take: the room left in its stacks and a full stack per
 * empty slot. 0 for no item or an item with no limit. */
s32 ItemBox_Room(u16 item)
{
    SaveData *save = &gSave;
    s32 i;
    s16 capacity;
    s32 room;

    if (item == 0 || gItemData[item].max == 255) {
        return 0;
    }
    capacity = Box_GetCapacity(save);
    room = 0;
    for (i = 0; i < capacity; i++) {
        if (save->itemBox[i].item == item) {
            room += (s16)gItemData[item].max - save->itemBox[i].count;
        } else if (save->itemBox[i].item == 0) {
            room += (s16)gItemData[item].max;
        }
    }
    return room;
}

/* How much of a full stack of an item the item box can take: the full stack size when there is an
 * empty slot or the room in its stacks reaches it, else the room found. -1 for no item. */
s32 ItemBox_Fits(u16 item)
{
    SaveData *save = &gSave;
    ItemStack *stack;
    s32 left;
    s32 max;
    s32 wanted;

    if (item == 0) {
        return -1;
    }
    left = Box_GetCapacity(save);
    stack = save->itemBox;
    max = gItemData[item].max;
    wanted = max;
    for (; left > 0; left--, stack++) {
        if (stack->item == 0) {
            return max;
        }
        if (stack->item == item && max != 255) {
            wanted -= max - stack->count;
            if (wanted <= 0) {
                return max;
            }
        }
    }
    return max - wanted;
}

/* Takes count of an item from the item box's stacks, first to last, clearing the stacks that run
 * out. Returns how many could not be taken (0 when all were). */
s16 ItemBox_Remove(u16 item, s16 count)
{
    s16 left;
    s16 i;
    s16 capacity;
    SaveData *save = &gSave;

    if (count < 0) {
        return 0;
    }
    if (item == 0 || gItemData[item].max == 255) {
        return 0;
    }
    capacity = Box_GetCapacity(save);
    left = count;
    for (i = 0; i < capacity; i++) {
        if (save->itemBox[i].item == item) {
            if (save->itemBox[i].count >= left) {
                save->itemBox[i].count -= left;
                left = 0;
            } else {
                left -= save->itemBox[i].count;
                save->itemBox[i].count = 0;
            }
            if (save->itemBox[i].count <= 0) {
                save->itemBox[i].item = 0;
            }
            if (left <= 0) {
                return 0;
            }
        }
    }
    return left;
}

/* Adds count of an item to a save's item box (at most a stack's worth): first onto its stacks that
 * have room, then the rest into the first empty slot, marking the item obtained. Returns how many
 * found no place (0 when all did). */
u32 ItemBox_AddTo(SaveData *save, u16 item, u32 count)
{
    ItemStack *stack;
    u32 i;
    u32 max;
    u32 room;
    u32 capacity;

    capacity = Box_GetCapacity(save);
    max = gItemData[item].max;
    if (count > max) {
        count = max;
    }
    if (max != 255) {
        stack = save->itemBox;
        for (i = capacity; i != 0; i--, stack++) {
            if (stack->item == item && stack->count < max) {
                room = max - stack->count;
                if (room >= count) {
                    stack->count += count;
                    Save_MarkItemObtained(save, item);
                    return 0;
                }
                count -= room;
                stack->count = max;
            }
        }
    }
    stack = save->itemBox;
    for (i = capacity; i != 0; i--, stack++) {
        if (stack->item == 0) {
            stack->item = item;
            stack->count = count;
            Save_MarkItemObtained(save, item);
            return 0;
        }
    }
    return count;
}

/* ItemBox_AddTo on gSave. */
u32 ItemBox_Add(u16 item, u32 count)
{
    return ItemBox_AddTo(&gSave, item, count);
}

/* As ItemBox_AddTo, on the item list given (as many slots as gSave's item box has), marking the
 * item obtained in gSave. Returns how many found no place. */
u16 ItemList_Add(ItemStack *list, u16 item, u16 count)
{
    ItemStack *stack;
    u32 i;
    u32 max;
    u32 room;
    u32 capacity;

    capacity = Box_GetCapacity(&gSave);
    max = gItemData[item].max;
    if (count > max) {
        count = max;
    }
    if (max != 255) {
        stack = list;
        for (i = capacity; i != 0; i--, stack++) {
            if (stack->item == item && stack->count < max) {
                room = max - stack->count;
                if (room >= count) {
                    stack->count += count;
                    Save_MarkItemObtained(&gSave, item);
                    return 0;
                }
                count -= room;
                stack->count = max;
            }
        }
    }
    for (i = capacity; i != 0; i--, list++) {
        if (list->item == 0) {
            list->item = item;
            list->count = count;
            Save_MarkItemObtained(&gSave, item);
            return 0;
        }
    }
    return count;
}

/* Adds count of an item to gSave's item box: first onto its stacks that have room, then into the
 * empty slots, a full stack each. An item with no limit takes one empty slot per unit, each with
 * the count 255. Returns how many found no place (0 when all did). */
u32 ItemBox_AddStacks(u16 item, u32 count)
{
    SaveData *save = &gSave;
    s16 stackMax;
    s16 capacity;
    ItemStack *stack;
    u32 i;

    if (count == 0) {
        return 0;
    }
    stackMax = gItemData[item].max;
    capacity = Box_GetCapacity(save);
    stack = save->itemBox;
    for (i = 0; i < capacity; i++, stack++) {
        if (stack->item == item && stack->count < stackMax && stackMax != 255) {
            u32 room = stackMax - stack->count;

            if (room >= count) {
                stack->count += count;
                Save_MarkItemObtained(save, item);
                return 0;
            }
            count -= room;
            stack->count = stackMax;
        }
    }
    stack = save->itemBox;
    for (i = 0; i < capacity; i++, stack++) {
        if (stack->item == 0) {
            Save_MarkItemObtained(save, item);
            stack->item = item;
            if (stackMax == 255) {
                stack->count = 255;
                count--;
            } else if (count <= stackMax) {
                stack->count = count;
                return 0;
            } else {
                stack->count = stackMax;
                count -= stackMax;
            }
        }
    }
    return count;
}

/* As SavePouch_Count, on gPouchBackup (the pouch as the quest found it). */
s16 PouchBackup_Count(u16 item)
{
    s16 i;

    if (Item_IsOnePerPouch(item) == 1) {
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (Item_IsOnePerPouch(gPouchBackup[i].item) == 1) {
                return 1;
            }
        }
        return 0;
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (item == gPouchBackup[i].item) {
            return gPouchBackup[i].count;
        }
    }
    return 0;
}

/* Takes amount (zero or less) of an item from gPouchBackup when the pouch uses it up. When the
 * pouch holds more than the backup, that extra goes first: 5 when it covers it all. Returns 4 when
 * the backup's stack ran out, 1 when some is left, and 255 for a positive amount, an item with no
 * limit, or an item neither holds. */
u8 PouchBackup_Use(Player *pl, u16 item, s16 amount)
{
    s16 backupCount;
    s16 pouchCount;
    s16 i;
    u8 result;

    if (amount > 0) {
        return 255;
    }
    if (gItemData[item].max == 255) {
        return 255;
    }
    backupCount = PouchBackup_Count(item);
    pouchCount = Pouch_Count(pl, item);
    if (backupCount == 0 && pouchCount == 0) {
        return 255;
    }
    if (pouchCount > backupCount) {
        s16 extra = pouchCount - backupCount;

        if (extra >= -amount) {
            return 5;
        }
        amount += extra;
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (item == gPouchBackup[i].item) {
            gPouchBackup[i].count += amount;
            if (gPouchBackup[i].count <= 0) {
                result = 4;
                gPouchBackup[i].item = 0;
                gPouchBackup[i].count = 0;
            } else {
                result = 1;
            }
            break;
        }
    }
    /* the game reads a register it never set: when no backup slot holds item (backupCount 0, the pouch holding more than -amount), result is whatever the caller left in that register */
    return result;
}

/* Sets event flag n. */
void Save_SetEventFlag(s32 n)
{
    gSave.flags[n / 16] |= (u16)(1 << (n & 0xF));
}

/* Clears event flag n. */
void Save_ClearEventFlag(s32 n)
{
    gSave.flags[n / 16] &= (u16)~(1 << (n & 0xF));
}

/* Tests event flag n (none while gSysSettings.galleryPlay is 1). */
s32 Save_TestEventFlag(s32 n)
{
    if (gSysSettings.galleryPlay == 1) {
        return 0;
    }
    if (gSave.flags[n / 16] & (1 << (n & 0xF))) {
        return 1;
    }
    return 0;
}

/* Sets up a player as a preset hunter (found by its id, Preset_Find): its hair, colour, gender,
 * voice, equipment, name (gMemberNames) and character type, from gPlayerCardTexts. */
void Player_LoadPreset(s8 *id, Player *pl)
{
    UserText *preset;
    s8 presetIndex;

    presetIndex = Preset_Find(id);
    preset = &gPlayerCardTexts[presetIndex];

    pl->hairColor = preset->card.hairColor;
    Player_SetColorWord(pl, preset->card.symbolColor);
    pl->gender = preset->card.gender;
    pl->hairStyle = preset->card.hairStyle;
    pl->equip[EQUIP_SLOT_WEAPON] = preset->card.weapon;
    pl->voice = preset->card.voice;
    pl->weaponKind = Equip_GetKind(&preset->card.weapon);
    pl->equip[EQUIP_LEGS] = preset->card.armour[0];
    pl->equip[EQUIP_SLOT_FACE] = preset->card.armour[1];
    pl->equip[EQUIP_HEAD] = preset->card.armour[2];
    pl->equip[EQUIP_BODY] = preset->card.armour[3];
    pl->equip[EQUIP_ARMS] = preset->card.armour[4];
    pl->equip[EQUIP_WAIST] = preset->card.armour[5];
    strcpy((char *)pl->name, gMemberNames[presetIndex]);
    pl->charType = preset->card.charType;
}

/* The index of the preset hunter (of four) whose 6-byte id in gPlayerIds matches id; 0 if none. */
s8 Preset_Find(s8 *id)
{
    s8 j;
    s8 i;

    for (j = 0; j < SCENE_PLAYERS; j++) {
        for (i = 0; i < 6; i++) {
            if (gPlayerIds[j][i] != id[i]) {
                break;
            }
        }
        if (i == 6) {
            return j;
        }
    }
    return 0;
}

/* Sets the four players' sides: in a versus quest, a player whose id (gPlayerIds) matches side j's
 * user id in gNextQuest is on side j + 1; else, and outside versus quests, on none (0). */
void Players_UpdateSides(void)
{
    s16 i;
    Player *pl = gPlayers;
    SideIdsView *view = (SideIdsView *)gNextQuest.teams;
    s16 j;
    s16 k;
    s16 differ;

    for (i = 0; i < SCENE_PLAYERS; i++, pl++) {
        if (Quest_IsVersus() == 1) {
            pl->side = 0;
            for (j = 0; j < SCENE_PLAYERS; j++) {
                differ = 0;
                for (k = 0; k < 6; k++) {
                    if (view->ids[j][k] != gPlayerIds[i][k]) {
                        differ = 1;
                        break;
                    }
                }
                if (differ == 0) {
                    pl->side = j + 1;
                    break;
                }
            }
        } else {
            pl->side = 0;
        }
    }
}

/* Copies the local user's id (gMyUserId) into gScene.userIds[slot]. */
void Player_SetMyUserId(u8 slot)
{
    s8 *dst = D_006ADF26[slot];

    dst[0] = gMyUserId[0];
    dst[1] = gMyUserId[1];
    dst[2] = gMyUserId[2];
    dst[3] = gMyUserId[3];
    dst[4] = gMyUserId[4];
    dst[5] = gMyUserId[5];
    dst[6] = gMyUserId[6];
    dst[7] = gMyUserId[7];
}

/* A blade weapon's element value when its element is the one given; 0 for another element or a gun.
 */
s32 Weapon_ElementValue(u16 type, u16 id, u16 element)
{
    switch (type) {
    case EQUIP_BLADE:
        if (element == gBladeData[id].elementType) {
            return gBladeData[id].elementValue;
        }
        break;
    }
    return 0;
}

/* A blade weapon's status value when its status is the one given; 0 for another status or a gun. */
s32 Weapon_StatusValue(u16 type, u16 id, u16 status)
{
    switch (type) {
    case EQUIP_BLADE:
        if (status == gBladeData[id].statusType) {
            return gBladeData[id].statusValue;
        }
        break;
    }
    return 0;
}

/* A stat of the player's weapon by kind (WSTAT_*): an element's value when the weapon has that
 * element (BladeData/GunData.elementType 1-4 for WSTAT_FIRE to WSTAT_ICE, 5 for WSTAT_DRAGON), a
 * status's when a blade has that status (statusType 1-3), else 0; or the affinity, plus 5, 10 or 15
 * for SKILL_AFFINITY_PLUS5, _PLUS10_NO_BOUNCE and _PLUS15_NO_BOUNCE, and for a blade 5 at sharpness level 4 or 10 above it when positive.
 * SKILL_STATUS_PLUS raises the statuses and the dragon element (WSTAT_POISON to WSTAT_DRAGON) by an
 * eighth. */
s16 Player_GetWeaponStat(Player *pl, u8 kind)
{
    s16 result;
    u8 etype;
    u8 stype;
    u8 eval;
    u8 sval;
    s8 aff;
    u8 equipType;

    result = 0;
    equipType = pl->equip[EQUIP_SLOT_WEAPON].type;
    if (equipType == EQUIP_BLADE) {
        etype = gBladeData[pl->equip[EQUIP_SLOT_WEAPON].id].elementType;
        stype = gBladeData[pl->equip[EQUIP_SLOT_WEAPON].id].statusType;
        eval = gBladeData[pl->equip[EQUIP_SLOT_WEAPON].id].elementValue;
        sval = gBladeData[pl->equip[EQUIP_SLOT_WEAPON].id].statusValue;
        aff = gBladeData[pl->equip[EQUIP_SLOT_WEAPON].id].affinity;
    } else {
        stype = 0;
        sval = 0;
        etype = gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].elementType;
        eval = gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].elementValue;
        aff = gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].affinity;
    }
    switch (kind) {
    case WSTAT_FIRE:
        if (etype == 1) {
            result = eval;
        }
        break;
    case WSTAT_WATER:
        if (etype == 2) {
            result = eval;
        }
        break;
    case WSTAT_THUNDER:
        if (etype == 3) {
            result = eval;
        }
        break;
    case WSTAT_ICE:
        if (etype == 4) {
            result = eval;
        }
        break;
    case WSTAT_POISON:
        if (equipType == EQUIP_BLADE && stype == 1) {
            result = sval;
        }
        break;
    case WSTAT_STATUS_TYPE2:
        if (equipType == EQUIP_BLADE && stype == 2) {
            result = sval;
        }
        break;
    case WSTAT_STATUS_TYPE3:
        if (equipType == EQUIP_BLADE && stype == 3) {
            result = sval;
        }
        break;
    case WSTAT_DRAGON:
        if (etype == 5) {
            result = eval;
        }
        break;
    case WSTAT_AFFINITY:
        result = aff;
        if (Player_HasSkill(pl, SKILL_AFFINITY_PLUS5) == 1) {
            result += 5;
        } else if (Player_HasSkill(pl, SKILL_AFFINITY_PLUS10_NO_BOUNCE) == 1) {
            result += 10;
        } else if (Player_HasSkill(pl, SKILL_AFFINITY_PLUS15_NO_BOUNCE) == 1) {
            result += 15;
        }
        if (result > 0) {
            if (pl->equip[EQUIP_SLOT_WEAPON].type == EQUIP_BLADE) {
                switch (pl->sharpnessLevel) {
                case 0:
                case 1:
                case 2:
                case 3:
                    break;
                case 4:
                    result += 5;
                    break;
                case 5:
                default:
                    result += 10;
                    break;
                }
            }
            return result;
        }
        return result;
    }
    if (Player_HasSkill(pl, SKILL_STATUS_PLUS) == 1 && result > 0 && kind >= WSTAT_POISON && kind < WSTAT_AFFINITY) {
        result = 1.125f * result;
    }
    return result;
}

/* A stat of the equipment shown (gEquipShown) by kind (EQUIP_STAT_*, save.h): the attack (a gun's
 * with its level row and options), the defense, a resistance summed over the armour, or the
 * weapon's element or status value (Weapon_ElementValue, Weapon_StatusValue: element types 1-4,
 * status types 1-3). */
s16 EquipView_GetStat(s32 kind)
{
    EquipItem *item;
    s16 value;

    item = gEquipShown;
    value = 0;
    switch ((u8)kind) {
    case EQUIP_STAT_ATTACK:
        if (item->type == EQUIP_GUN) {
            u16 gunLevel = item->level;

            value += gGunData[item->id].attack;
            value += gGunLevelRows[gGunData[item->id].levelRow].levels[(gunLevel & GUN_LEVEL_MASK) * 12];
            if (gunLevel & GUN_OPT_SILENCER) {
                value += gSilencerBonus;
            }
            if (gunLevel & GUN_OPT_BARREL) {
                if (Weapon_GetClass(EQUIP_GUN, item->id) == WEAPON_HEAVY_BOWGUN) {
                    value += gBarrelBonusHeavy[0];
                } else {
                    value += gBarrelBonusLight[0];
                }
            }
        } else {
            value = gBladeData[item->id].attack;
        }
        break;
    case EQUIP_STAT_DEFENSE:
        value += (u8)Armor_GetDefense(&item[1 + EQUIP_HEAD]);
        value += (u8)Armor_GetDefense(&item[1 + EQUIP_BODY]);
        value += (u8)Armor_GetDefense(&item[1 + EQUIP_ARMS]);
        value += (u8)Armor_GetDefense(&item[1 + EQUIP_WAIST]);
        value += (u8)Armor_GetDefense(&item[1 + EQUIP_LEGS]);
        if (item->type == EQUIP_GUN) {
            value += gGunData[item->id].defense;
        } else {
            value += gBladeData[item->id].defense;
        }
        break;
    case EQUIP_STAT_RES_FIRE:
        value += gArmorHeadData[item[1 + EQUIP_HEAD].id].resist[0];
        value += gArmorBodyData[item[1 + EQUIP_BODY].id].resist[0];
        value += gArmorArmsData[item[1 + EQUIP_ARMS].id].resist[0];
        value += gArmorWaistData[item[1 + EQUIP_WAIST].id].resist[0];
        value += gArmorLegsData[item[1 + EQUIP_LEGS].id].resist[0];
        break;
    case EQUIP_STAT_RES_WATER:
        value += gArmorHeadData[item[1 + EQUIP_HEAD].id].resist[1];
        value += gArmorBodyData[item[1 + EQUIP_BODY].id].resist[1];
        value += gArmorArmsData[item[1 + EQUIP_ARMS].id].resist[1];
        value += gArmorWaistData[item[1 + EQUIP_WAIST].id].resist[1];
        value += gArmorLegsData[item[1 + EQUIP_LEGS].id].resist[1];
        break;
    case EQUIP_STAT_RES_THUNDER:
        value += gArmorHeadData[item[1 + EQUIP_HEAD].id].resist[2];
        value += gArmorBodyData[item[1 + EQUIP_BODY].id].resist[2];
        value += gArmorArmsData[item[1 + EQUIP_ARMS].id].resist[2];
        value += gArmorWaistData[item[1 + EQUIP_WAIST].id].resist[2];
        value += gArmorLegsData[item[1 + EQUIP_LEGS].id].resist[2];
        break;
    case EQUIP_STAT_RES_ICE:
        value += gArmorHeadData[item[1 + EQUIP_HEAD].id].resist[3];
        value += gArmorBodyData[item[1 + EQUIP_BODY].id].resist[3];
        value += gArmorArmsData[item[1 + EQUIP_ARMS].id].resist[3];
        value += gArmorWaistData[item[1 + EQUIP_WAIST].id].resist[3];
        value += gArmorLegsData[item[1 + EQUIP_LEGS].id].resist[3];
        break;
    case EQUIP_STAT_FIRE:
        value = Weapon_ElementValue(item->type, item->id, 1);
        break;
    case EQUIP_STAT_WATER:
        value = Weapon_ElementValue(item->type, item->id, 2);
        break;
    case EQUIP_STAT_THUNDER:
        value = Weapon_ElementValue(item->type, item->id, 3);
        break;
    case EQUIP_STAT_ICE:
        value = Weapon_ElementValue(item->type, item->id, 4);
        break;
    case EQUIP_STAT_POISON:
        value = Weapon_StatusValue(item->type, item->id, 1);
        break;
    case EQUIP_STAT_STATUS_TYPE2:
        value = Weapon_StatusValue(item->type, item->id, 2);
        break;
    case EQUIP_STAT_STATUS_TYPE3:
        value = Weapon_StatusValue(item->type, item->id, 3);
        break;
    case EQUIP_STAT_RES_DRAGON:
        value += gArmorHeadData[item[1 + EQUIP_HEAD].id].resist[4];
        value += gArmorBodyData[item[1 + EQUIP_BODY].id].resist[4];
        value += gArmorArmsData[item[1 + EQUIP_ARMS].id].resist[4];
        value += gArmorWaistData[item[1 + EQUIP_WAIST].id].resist[4];
        value += gArmorLegsData[item[1 + EQUIP_LEGS].id].resist[4];
        break;
    }
    return value;
}

/* A stat of a player's equipment (EQUIP_STAT_*, EquipView_GetStat): from the local hunter's save in a
 * normal quest, else (a drill, the training, another player) from the player's own equipment. */
s16 Player_GetEquipStat(Player *pl, s32 kind)
{
    EquipItem *shown = gEquipShown;
    SaveData *save = &gSave;

    if (Quest_IsDrill() == 1 || Quest_IsTraining() == 1 || Player_IsLocal(pl) == 0) {
        EquipView_FromPlayer(pl);
    } else {
        shown[0] = save->equip[SAVE_EQUIP_WEAPON];
        shown[1 + EQUIP_HEAD] = save->equip[SAVE_EQUIP_HEAD];
        shown[1 + EQUIP_BODY] = save->equip[SAVE_EQUIP_BODY];
        shown[1 + EQUIP_ARMS] = save->equip[SAVE_EQUIP_ARMS];
        shown[1 + EQUIP_WAIST] = save->equip[SAVE_EQUIP_WAIST];
        shown[1 + EQUIP_LEGS] = save->equip[SAVE_EQUIP_LEGS];
    }
    return EquipView_GetStat(kind);
}

/* Fills gEquipShown from a player's equipment: the weapon first, then Player.equip[0-5]. */
void EquipView_FromPlayer(Player *pl)
{
    EquipItem *shown = gEquipShown;
    s32 i;

    shown[0] = pl->equip[EQUIP_SLOT_WEAPON];
    for (i = 0; i < EQUIP_SLOT_WEAPON; i++) {
        shown[i + 1] = pl->equip[i];
    }
}

/* A gun's attack: GunData.attack, plus for bowguns the level's attack (gGunLevelRows) and the
 * options' (gSilencerBonus, and gBarrelBonusHeavy or gBarrelBonusLight); -1 for another item. */
s16 Gun_GetAttack(EquipItem *item)
{
    s16 total;
    u16 weaponClass;

    if (item->type != EQUIP_GUN) {
        return -1;
    }
    total = gGunData[item->id].attack;
    weaponClass = Equip_GetClass(item);
    if (weaponClass != WEAPON_BOW) {
        u16 gunLevel = item->level;

        total += gGunLevelRows[gGunData[item->id].levelRow].levels[(gunLevel & GUN_LEVEL_MASK) * 12];
        if (gunLevel & GUN_OPT_SILENCER) {
            total += gSilencerBonus;
        }
        if (gunLevel & GUN_OPT_BARREL) {
            if (weaponClass == WEAPON_HEAVY_BOWGUN) {
                total += gBarrelBonusHeavy[0];
            } else {
                total += gBarrelBonusLight[0];
            }
        }
    }
    return total;
}

/* The index of a quest id in gClearListQuests (its bit in SaveData.clearedQuests), 65535 if it
 * isn't listed. */
u16 ClearList_IndexOf(u16 quest)
{
    u16 i = 0;
    u16 *entry = gClearListQuests;
    u16 key = gClearListQuests[0];

    while (key != 0xEA60) {
        if (key == quest) {
            return i;
        }
        entry++;
        i++;
        key = *entry;
    }
    return 65535;
}

/* Records a quest as cleared in gSave (its bit in clearedQuests) when the quest's main goal is done
 * and the quest is listed. */
void Save_MarkQuestCleared(u16 quest)
{
    SaveData *save = &gSave;
    u16 index;

    if (QuestGoal_IsDone(0) != 0) {
        index = ClearList_IndexOf(quest);
        if (index != 0xFFFF) {
            save->clearedQuests[(u16)(index / 32)] |= 1LL << (u16)(index % 32);
        }
    }
}

/* 1 when gSave records a listed quest as cleared (the menus draw its clear mark). */
s32 Save_IsQuestCleared(u16 quest)
{
    SaveData *save = &gSave;
    u16 index = ClearList_IndexOf(quest);

    if (index != 0xFFFF) {
        if (save->clearedQuests[(u16)(index / 32)] & (1LL << (u16)(index % 32))) {
            return 1;
        }
        return 0;
    }
    return 0;
}

/* The flag bytes of item's equipment type in save. Static: its callers (Equip_SetFlag and the
 * next two) keep their arguments in the argument registers across the call, as the game's code
 * does; the compiler does that only for a static function defined earlier in the file. */
static u8 *Save_GetEquipFlags(SaveData *save, EquipItem *item)
{
    u8 *flags;

    switch (item->type) {
    case EQUIP_LEGS:
        flags = save->armorFlags[4];
        break;
    case EQUIP_HEAD:
        flags = save->armorFlags[0];
        break;
    case EQUIP_BODY:
        flags = save->armorFlags[1];
        break;
    case EQUIP_ARMS:
        flags = save->armorFlags[2];
        break;
    case EQUIP_WAIST:
        flags = save->armorFlags[3];
        break;
    case EQUIP_BLADE:
        flags = save->bladeFlags;
        break;
    case EQUIP_GUN:
        flags = save->gunFlags;
        break;
    }
    return flags;
}

/* Clears bit 2 of every equipment flag byte in the save (blades, guns, then the five
 * armour types, as far as each table's entries go). */
void Save_ClearNewMarks(void)
{
    s16 i;
    SaveData *save = &gSave;

    for (i = 0; i < BLADE_ENTRY_COUNT; i++) {
        save->bladeFlags[i] &= ~(1 << EQUIP_FLAG_NEW);
    }
    for (i = 0; i < GUN_ENTRY_COUNT; i++) {
        save->gunFlags[i] &= ~(1 << EQUIP_FLAG_NEW);
    }
    for (i = 0; i < HEAD_ENTRY_COUNT; i++) {
        save->armorFlags[0][i] &= ~(1 << EQUIP_FLAG_NEW);
    }
    for (i = 0; i < BODY_ENTRY_COUNT; i++) {
        save->armorFlags[1][i] &= ~(1 << EQUIP_FLAG_NEW);
    }
    for (i = 0; i < ARMS_ENTRY_COUNT; i++) {
        save->armorFlags[2][i] &= ~(1 << EQUIP_FLAG_NEW);
    }
    for (i = 0; i < WAIST_ENTRY_COUNT; i++) {
        save->armorFlags[3][i] &= ~(1 << EQUIP_FLAG_NEW);
    }
    for (i = 0; i < LEGS_ENTRY_COUNT; i++) {
        save->armorFlags[4][i] &= ~(1 << EQUIP_FLAG_NEW);
    }
}

/* 1 when an item of equipment has an id its type's flag bytes cover (1 to ARMOR_ID_COUNT,
 * BLADE_ID_COUNT or GUN_ID_COUNT less one); 0 for id 0 or an unknown type. */
static s32 Equip_IsValidId(EquipItem *item)
{
    u16 max;

    if (item->id == 0) {
        return 0;
    }
    switch (item->type) {
    case EQUIP_LEGS:
    case EQUIP_HEAD:
    case EQUIP_BODY:
    case EQUIP_ARMS:
    case EQUIP_WAIST:
        max = ARMOR_ID_COUNT;
        break;
    case EQUIP_BLADE:
        max = BLADE_ID_COUNT;
        break;
    case EQUIP_GUN:
        max = GUN_ID_COUNT;
        break;
    default:
        return 0;
    }
    return item->id < max;
}

/*
 * From here: the equipment box (12-byte entries, 100 per box page owned)
 * and the six equipped items, gun levels and options, the per-monster
 * hunting records (sizes, hunted, captured), the items-obtained bits, and
 * armour values computed from the armour tables (defense, upgrade cost,
 * sell price).
 */

/* Armor data (40 bytes), one table per armor type (gArmorTables). */

/* A forging recipe (24 bytes): the tables gArmorRecipes and gWeaponRecipes are outside
 * sub_main (Forge_CheckRecipe). The lobby's forge menus view it as their own ForgeRecipe. */
typedef struct ForgeRecipe {
    u8 type;                    /* 0x0: gWeaponRecipes: EQUIP_BLADE or EQUIP_GUN */
    u8 alwaysShown;             /* 0x1: nonzero: Forge_CheckRecipe gives 1 even while locked */
    u16 id;
    ItemStack mat[4];           /* 0x4: materials */
    u8 cond[4];                 /* 0x14: per material, the condition that unlocks the recipe (Forge_CheckRecipe) */
} ForgeRecipe;

/* One page of the box (25 entries), built by Save_BuildSummary (412 bytes). */
typedef struct BoxPage {
    u16 id[SUMMARY_BOX_SLOTS];                 /* 0x000 */
    u16 level[SUMMARY_BOX_SLOTS];              /* 0x032 */
    u16 deco0[SUMMARY_BOX_SLOTS];              /* 0x064: EquipItem.deco[0] */
    u16 deco1[SUMMARY_BOX_SLOTS];              /* 0x096 */
    u16 deco2[SUMMARY_BOX_SLOTS];              /* 0x0C8 */
    s32 hrPoints;               /* 0x0FC */
    u32 itemBits[ITEM_BITS_WORDS];           /* 0x100: SaveData.itemBits */
    u16 hr;                     /* 0x180 */
    u8 type[SUMMARY_BOX_SLOTS];                /* 0x182: EquipItem.type, 255 for an empty slot */
} BoxPage;

/* The equipped items, built by Save_BuildSummary (196 bytes). */
typedef struct EquipSummary {
    u8 weaponType;              /* 0x00 */
    u8 headId;                  /* 0x01 */
    u16 weaponId;               /* 0x02 */
    u8 bodyId;                  /* 0x04 */
    u8 armsId;                  /* 0x05 */
    u8 waistId;                 /* 0x06 */
    u8 legsId;                  /* 0x07 */
    u16 level[6];               /* 0x08: the weapon, head, body, arms, waist, legs */
    u16 deco0[6];               /* 0x14: EquipItem.deco[0], in the same order */
    u16 deco1[6];               /* 0x20 */
    u16 deco2[6];               /* 0x2C */
    s32 playTime;               /* 0x38 */
    s32 hrPoints;               /* 0x3C */
    u32 itemBits[ITEM_BITS_WORDS];           /* 0x40: SaveData.itemBits */
    u16 hr;                     /* 0xC0 */
} EquipSummary;

extern EquipSummary gSaveSummaryEquip;
extern BoxPage gSaveSummaryBox;
extern f32 gMonsterBaseSize[];
extern u16 gObtainFlagItems[];
extern u32 gPlayerColorWords[];
extern u8 *gPlayerPartFlagTbls[];
extern f32 gArmorLevelDefense[];
extern f32 gArmorLevelCost[];
extern u8 gArmorLevelSlots[];
extern ForgeRecipe gArmorRecipes[];
extern ForgeRecipe gWeaponRecipes[];

/* Sets bit bit of item's flag byte in gSave, then the equipment collection awards. */
void Equip_SetFlag(EquipItem *item, s16 bit)
{
    u8 *flags;

    if (Equip_IsValidId(item)) {
        flags = Save_GetEquipFlags(&gSave, item);
        flags[item->id] |= 1 << bit;
        EquipCollect_CheckWeapons();
        EquipCollect_CheckArmor();
    }
}

/* As Equip_SetFlag, on the save given. */
void Save_SetEquipFlag(SaveData *save, EquipItem *item, s16 bit)
{
    u8 *flags;

    if (Equip_IsValidId(item)) {
        flags = Save_GetEquipFlags(save, item);
        flags[item->id] |= 1 << bit;
        EquipCollect_CheckWeapons();
        EquipCollect_CheckArmor();
    }
}

/* Packs bit 0 of n bytes into (n + 7) / 8 bytes. */
void Bits_Pack(u8 *dst, u8 *src, s32 bitCount)
{
    s32 count;
    s32 i;
    s32 j;
    u8 packed;

    if (bitCount % 8 != 0) {
        count = bitCount / 8 + 1;
    } else {
        count = bitCount / 8;
    }
    for (i = 0; i < count; i++) {
        packed = 0;
        for (j = 0; j < 8; j++) {
            packed |= (u8)((*src++ & 1) << j);
        }
        *dst++ = packed;
    }
}

/* Bit n of a byte array. */
s32 Bits_Test(u8 *bits, u16 bit)
{
    return (bits[bit / 8] & (1 << (bit & 7))) != 0;
}

/* 1 when bit `bit` (EQUIP_FLAG_*) of the item's byte in the flag bytes given is set; 0 for an
 * invalid id. */
s32 EquipFlags_Test(u8 *bits, EquipItem *item, s16 bit)
{
    if (Equip_IsValidId(item) == 0) {
        return 0;
    }
    if (bits[item->id] & (1 << bit)) {
        return 1;
    }
    return 0;
}

/* Bit bit of item's flag byte in gSave (0 for an invalid id). */
s32 Equip_TestFlag(EquipItem *item, s16 bit)
{
    u8 *flags;

    if (Equip_IsValidId(item) == 0) {
        return 0;
    }
    flags = Save_GetEquipFlags(&gSave, item);
    return EquipFlags_Test(flags, item, bit);
}

/* The first free box slot of gSave, 65535 when full. */
u16 Box_FindFreeSlot(void *unused0)
{
    SaveData *save = &gSave;
    s16 i;
    s16 capacity = Box_GetCapacity(save);

    for (i = 0; i < capacity; i++) {
        if (save->box[i].used == 0) {
            return i;
        }
    }
    return BOX_NONE;
}

/* Puts an item in the first free box slot and flags it seen; its index or 65535. */
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level)
{
    s16 capacity = Box_GetCapacity(save);
    s16 i;

    for (i = 0; i < capacity; i++) {
        if (save->box[i].used == 0) {
            save->box[i].used = 1;
            save->box[i].type = type;
            save->box[i].id = id;
            save->box[i].level = level;
            save->box[i].deco[0] = 0;
            save->box[i].deco[1] = 0;
            save->box[i].deco[2] = 0;
            Save_PickBestCounter(&save->box[i], save->gender);
            Save_SetEquipFlag(save, &save->box[i], EQUIP_FLAG_OWNED);
            return i;
        }
    }
    return BOX_NONE;
}

/* Puts a copy of item in box slot idx. */
void Box_SetSlot(SaveData *save, u16 idx, EquipItem *item)
{
    s16 capacity = Box_GetCapacity(save);

    if (idx < capacity) {
        save->box[idx] = *item;
        save->box[idx].used = 1;
        item = &save->box[idx];
        Save_PickBestCounter(item, save->gender);
        Save_SetEquipFlag(save, item, EQUIP_FLAG_OWNED);
    }
}

/* Empties box slot idx unless it is equipped; 1 when emptied. */
s32 Box_ClearSlot(SaveData *save, u16 idx)
{
    if (Equip_IsSlotEquipped(save, idx) == 0) {
        save->box[idx].used = 0;
        save->box[idx].type = 0;
        save->box[idx].id = 0;
        save->box[idx].level = 0;
        save->box[idx].deco[0] = 0;
        save->box[idx].deco[1] = 0;
        save->box[idx].deco[2] = 0;
        return 1;
    }
    return 0;
}

/* Unequips the armor the character cannot wear (gender, blademaster or gunner). */
static void Save_UnequipUnwearable(SaveData *save)
{
    u8 mask = 0;
    s32 wanted;

    if (save->gender == 0) {
        mask |= 1;
    } else {
        mask |= 2;
    }
    if (save->equip[SAVE_EQUIP_WEAPON].type == EQUIP_GUN) {
        mask |= 8;
    } else {
        mask |= 4;
    }
    wanted = mask;
    if ((wanted & gArmorHeadData[save->equip[SAVE_EQUIP_HEAD].id].flags) != wanted) {
        save->equipIdx[SAVE_EQUIP_HEAD] = BOX_NONE;
    }
    if ((wanted & gArmorBodyData[save->equip[SAVE_EQUIP_BODY].id].flags) != wanted) {
        save->equipIdx[SAVE_EQUIP_BODY] = BOX_NONE;
    }
    if ((wanted & gArmorWaistData[save->equip[SAVE_EQUIP_WAIST].id].flags) != wanted) {
        save->equipIdx[SAVE_EQUIP_WAIST] = BOX_NONE;
    }
    if ((wanted & gArmorArmsData[save->equip[SAVE_EQUIP_ARMS].id].flags) != wanted) {
        save->equipIdx[SAVE_EQUIP_ARMS] = BOX_NONE;
    }
    if ((wanted & gArmorLegsData[save->equip[SAVE_EQUIP_LEGS].id].flags) != wanted) {
        save->equipIdx[SAVE_EQUIP_LEGS] = BOX_NONE;
    }
}

/* Equips box slot idx of the save in the place its type takes (SaveData.equipIdx). Returns 1, or 0
 * for an empty slot, an unknown type, or a weapon with idx BOX_NONE. */
s32 Equip_EquipSlot(SaveData *save, u16 idx)
{
    if (save->box[idx].used == 0) {
        return 0;
    }
    switch (save->box[idx].type) {
    case EQUIP_LEGS:
        save->equipIdx[SAVE_EQUIP_LEGS] = idx;
        break;
    case EQUIP_HEAD:
        save->equipIdx[SAVE_EQUIP_HEAD] = idx;
        break;
    case EQUIP_BODY:
        save->equipIdx[SAVE_EQUIP_BODY] = idx;
        break;
    case EQUIP_ARMS:
        save->equipIdx[SAVE_EQUIP_ARMS] = idx;
        break;
    case EQUIP_WAIST:
        save->equipIdx[SAVE_EQUIP_WAIST] = idx;
        break;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        if (idx == BOX_NONE) {
            return 0;
        }
        save->equipIdx[SAVE_EQUIP_WEAPON] = idx;
        break;
    default:
        return 0;
    }
    return 1;
}

/* Takes off the save's armour piece of a type (its equipIdx to BOX_NONE). Returns 1, or 0 for a
 * weapon type (one is always equipped) or an unknown type. */
s32 Equip_UnequipType(SaveData *save, s32 type)
{
    switch (type) {
    case EQUIP_LEGS:
        save->equipIdx[SAVE_EQUIP_LEGS] = BOX_NONE;
        break;
    case EQUIP_HEAD:
        save->equipIdx[SAVE_EQUIP_HEAD] = BOX_NONE;
        break;
    case EQUIP_BODY:
        save->equipIdx[SAVE_EQUIP_BODY] = BOX_NONE;
        break;
    case EQUIP_ARMS:
        save->equipIdx[SAVE_EQUIP_ARMS] = BOX_NONE;
        break;
    case EQUIP_WAIST:
        save->equipIdx[SAVE_EQUIP_WAIST] = BOX_NONE;
        break;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        return 0;
    default:
        return 0;
    }
    return 1;
}

/* Box slot idx is used and holds an item of type type1 or type2. */
static s32 Box_SlotHasType(SaveData *save, u16 idx, u8 typeA, u8 typeB)
{
    if (idx == BOX_NONE) {
        return 0;
    }
    if (save->box[idx].used == 0
        || (save->box[idx].type != typeA && save->box[idx].type != typeB)) {
        return 0;
    }
    return 1;
}

/* Copies the equipped box slots into save->equip (no weapon: blade weapon 155). */
void Equip_Refresh(SaveData *save)
{
    if (Box_SlotHasType(save, save->equipIdx[SAVE_EQUIP_WEAPON], EQUIP_BLADE, EQUIP_GUN) == 1) {
        save->equip[SAVE_EQUIP_WEAPON] = save->box[save->equipIdx[SAVE_EQUIP_WEAPON]];
    } else {
        save->equip[SAVE_EQUIP_WEAPON].used = save->box[save->equipIdx[SAVE_EQUIP_WEAPON]].used;
        save->equip[SAVE_EQUIP_WEAPON].type = EQUIP_BLADE;
        save->equip[SAVE_EQUIP_WEAPON].id = 155;
        save->equip[SAVE_EQUIP_WEAPON].level = 0;
        save->equip[SAVE_EQUIP_WEAPON].deco[0] = 0;
        save->equip[SAVE_EQUIP_WEAPON].deco[1] = 0;
        save->equip[SAVE_EQUIP_WEAPON].deco[2] = 0;
    }
    Save_UnequipUnwearable(save);
    if (Box_SlotHasType(save, save->equipIdx[SAVE_EQUIP_LEGS], EQUIP_LEGS, EQUIP_LEGS) == 1) {
        save->equip[SAVE_EQUIP_LEGS] = save->box[save->equipIdx[SAVE_EQUIP_LEGS]];
    } else {
        save->equip[SAVE_EQUIP_LEGS].id = 0;
        save->equip[SAVE_EQUIP_LEGS].type = EQUIP_LEGS;
        save->equip[SAVE_EQUIP_LEGS].level = 0;
        save->equip[SAVE_EQUIP_LEGS].deco[0] = 0;
        save->equip[SAVE_EQUIP_LEGS].deco[1] = 0;
        save->equip[SAVE_EQUIP_LEGS].deco[2] = 0;
    }
    if (Box_SlotHasType(save, save->equipIdx[SAVE_EQUIP_HEAD], EQUIP_HEAD, EQUIP_HEAD) == 1) {
        save->equip[SAVE_EQUIP_HEAD] = save->box[save->equipIdx[SAVE_EQUIP_HEAD]];
    } else {
        save->equip[SAVE_EQUIP_HEAD].id = 0;
        save->equip[SAVE_EQUIP_HEAD].type = EQUIP_HEAD;
        save->equip[SAVE_EQUIP_HEAD].level = 0;
        save->equip[SAVE_EQUIP_HEAD].deco[0] = 0;
        save->equip[SAVE_EQUIP_HEAD].deco[1] = 0;
        save->equip[SAVE_EQUIP_HEAD].deco[2] = 0;
    }
    if (Box_SlotHasType(save, save->equipIdx[SAVE_EQUIP_BODY], EQUIP_BODY, EQUIP_BODY) == 1) {
        save->equip[SAVE_EQUIP_BODY] = save->box[save->equipIdx[SAVE_EQUIP_BODY]];
    } else {
        save->equip[SAVE_EQUIP_BODY].id = 0;
        save->equip[SAVE_EQUIP_BODY].type = EQUIP_BODY;
        save->equip[SAVE_EQUIP_BODY].level = 0;
        save->equip[SAVE_EQUIP_BODY].deco[0] = 0;
        save->equip[SAVE_EQUIP_BODY].deco[1] = 0;
        save->equip[SAVE_EQUIP_BODY].deco[2] = 0;
    }
    if (Box_SlotHasType(save, save->equipIdx[SAVE_EQUIP_ARMS], EQUIP_ARMS, EQUIP_ARMS) == 1) {
        save->equip[SAVE_EQUIP_ARMS] = save->box[save->equipIdx[SAVE_EQUIP_ARMS]];
    } else {
        save->equip[SAVE_EQUIP_ARMS].id = 0;
        save->equip[SAVE_EQUIP_ARMS].type = EQUIP_ARMS;
        save->equip[SAVE_EQUIP_ARMS].level = 0;
        save->equip[SAVE_EQUIP_ARMS].deco[0] = 0;
        save->equip[SAVE_EQUIP_ARMS].deco[1] = 0;
        save->equip[SAVE_EQUIP_ARMS].deco[2] = 0;
    }
    if (Box_SlotHasType(save, save->equipIdx[SAVE_EQUIP_WAIST], EQUIP_WAIST, EQUIP_WAIST) == 1) {
        save->equip[SAVE_EQUIP_WAIST] = save->box[save->equipIdx[SAVE_EQUIP_WAIST]];
    } else {
        save->equip[SAVE_EQUIP_WAIST].id = 0;
        save->equip[SAVE_EQUIP_WAIST].type = EQUIP_WAIST;
        save->equip[SAVE_EQUIP_WAIST].level = 0;
        save->equip[SAVE_EQUIP_WAIST].deco[0] = 0;
        save->equip[SAVE_EQUIP_WAIST].deco[1] = 0;
        save->equip[SAVE_EQUIP_WAIST].deco[2] = 0;
    }
}

/* One of six indices equals n. */
s32 List6_Contains(u16 *list, s32 value)
{
    s32 ret;

    if (list[0] == value) {
        ret = 1;
    } else if (list[1] == value) {
        ret = 1;
    } else if (list[2] == value) {
        ret = 1;
    } else if (list[3] == value) {
        ret = 1;
    } else if (list[4] == value) {
        ret = 1;
    } else {
        ret = list[5] == value;
    }
    return ret;
}

/* Box slot idx is equipped. */
s32 Equip_IsSlotEquipped(SaveData *save, s32 idx)
{
    s32 ret;

    if (save->equipIdx[SAVE_EQUIP_WEAPON] == idx) {
        ret = 1;
    } else if (save->equipIdx[SAVE_EQUIP_LEGS] == idx) {
        ret = 1;
    } else if (save->equipIdx[SAVE_EQUIP_HEAD] == idx) {
        ret = 1;
    } else if (save->equipIdx[SAVE_EQUIP_BODY] == idx) {
        ret = 1;
    } else if (save->equipIdx[SAVE_EQUIP_ARMS] == idx) {
        ret = 1;
    } else {
        ret = save->equipIdx[SAVE_EQUIP_WAIST] == idx;
    }
    return ret;
}

/* Box slot idx is free. */
s32 Box_IsSlotFree(SaveData *save, s32 idx)
{
    return save->box[idx].used == 0;
}

/* 1 when a forging condition is met (BladeData.forgeCond, GunData.forgeCond; the same values as
 * ForgeRecipe.cond): 0 and 1 always; 2 a hunter rank above 0; 3, 4 and 5 EVENT_FLAG_PSP_LINK_7, _8
 * and _9; 6 a monster of group 11 hunted or captured; 7 both 2 and 6. */
s32 Forge_CheckCondition(s16 cond)
{
    u8 ok = 0;

    switch (cond) {
    case 0:
        ok = 1;
        break;
    case 1:
        ok = 1;
        break;
    case 2:
        if (gSave.hr > 0) {
            ok = 1;
        }
        break;
    case 3:
        if (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_7)) {
            ok = 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_8)) {
            ok = 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(EVENT_FLAG_PSP_LINK_9)) {
            ok = 1;
        }
        break;
    case 6:
        if (Record_GroupHunted(&gSave, 11) > 0
            || Record_GroupCaptured(&gSave, 11) > 0) {
            ok = 1;
        }
        break;
    case 7:
        if (gSave.hr > 0
            && (Record_GroupHunted(&gSave, 11) > 0
                || Record_GroupCaptured(&gSave, 11) > 0)) {
            ok = 1;
        }
        break;
    }
    return ok != 0;
}

/* Whether a recipe can be forged: list 0 the armour recipes (gArmorRecipes), else the weapon
 * recipes, entry idx. A weapon whose own condition fails (Forge_CheckCondition) gives 0. The recipe
 * is unlocked when none of its materials has a condition, or one material's is met (as
 * Forge_CheckCondition's, with 1, 2, 6 and 7 also needing the material obtained). The materials are
 * counted where `where` says: 0 the pouch and the box, 1 the pouch, 2 the box's first stack.
 * Returns 2 unlocked with every material at hand, 1 unlocked (or ForgeRecipe.alwaysShown), else 0.
 */
s32 Forge_CheckRecipe(u16 list, s16 idx, s32 where)
{
    SaveData *save = &gSave;
    s32 total;
    u8 numCond;
    ForgeRecipe *recipe;
    u8 isWeapon;
    s32 i;
    u8 okMask;
    u8 unlocked;
    u8 obtained;
    s32 pouchCount;
    s32 boxCount;

    switch (list) {
    case 0:
        recipe = &gArmorRecipes[idx];
        isWeapon = 0;
        break;
    case 1:
    default:
        recipe = &gWeaponRecipes[idx];
        isWeapon = 1;
        break;
    }
    unlocked = 0;
    numCond = 0;
    okMask = 0;
    if (isWeapon != 0) {
        switch (recipe->type) {
        case EQUIP_BLADE:
            if (recipe->id < 900) {
                BladeData *blade = &gBladeData[recipe->id];

                if (Forge_CheckCondition(blade->forgeCond) == 0) {
                    return 0;
                }
            }
            break;
        case EQUIP_GUN:
            if (recipe->id < 900) {
                GunData *gun = &gGunData[recipe->id];

                if (Forge_CheckCondition(gun->forgeCond) == 0) {
                    return 0;
                }
            }
            break;
        }
    }
    for (i = 0; i < 4; i++) {
        if (recipe->mat[i].item == 0 || recipe->mat[i].count == 0) {
            okMask |= 1 << i;
            continue;
        }
        obtained = Save_IsItemObtained(save, recipe->mat[i].item) == 1;
        switch (where) {
        case 0:
            pouchCount = SavePouch_Count(recipe->mat[i].item);
            boxCount = ItemBox_Total(recipe->mat[i].item);
            break;
        case 1:
            pouchCount = SavePouch_Count(recipe->mat[i].item);
            boxCount = 0;
            break;
        case 2:
            pouchCount = 0;
            boxCount = ItemBox_Count(recipe->mat[i].item);
            break;
        }
        total = pouchCount + boxCount;
        if (recipe->cond[i] != 0) {
            numCond++;
            switch (recipe->cond[i]) {
            case 1:
                if (obtained > 0) {
                    unlocked = 1;
                }
                break;
            case 2:
                if (obtained > 0 && gSave.hr > 0) {
                    unlocked = 1;
                }
                break;
            case 3:
                if (Save_TestEventFlag(127)) {
                    unlocked = 1;
                }
                break;
            case 4:
                if (Save_TestEventFlag(130)) {
                    unlocked = 1;
                }
                break;
            case 5:
                if (Save_TestEventFlag(131)) {
                    unlocked = 1;
                }
                break;
            case 6:
                if (obtained > 0
                    && (Record_GroupHunted(&gSave, 11) > 0
                        || Record_GroupCaptured(&gSave, 11) > 0)) {
                    unlocked = 1;
                }
                break;
            case 7:
                if (obtained > 0 && gSave.hr > 0
                    && (Record_GroupHunted(&gSave, 11) > 0
                        || Record_GroupCaptured(&gSave, 11) > 0)) {
                    unlocked = 1;
                }
                break;
            }
        }
        if (total >= recipe->mat[i].count) {
            okMask |= 1 << i;
        }
    }
    if (numCond == 0 || unlocked != 0) {
        if (okMask == 15) {
            return 2;
        }
        if (unlocked != 0) {
            return 1;
        }
    }
    if (recipe->alwaysShown != 0) {
        return 1;
    }
    return 0;
}

/* 1 when box slot idx holds a gun. */
static s32 Box_SlotIsGun(SaveData *save, s16 idx)
{
    return save->box[idx].type == EQUIP_GUN;
}

/* Guns: raises the level in bits 0-3 of box slot idx, up to 4. */
void Box_GunLevelUp(SaveData *save, s16 idx)
{
    u16 level;
    u16 gunLevel;

    if (Box_SlotIsGun(save, idx)) {
        level = save->box[idx].level;
        gunLevel = level & GUN_LEVEL_MASK;
        if (gunLevel >= 4) {
            gunLevel = 4;
        } else {
            gunLevel++;
        }
        save->box[idx].level = (level & GUN_OPT_MASK) | gunLevel;
    }
}

/* A gun's level in box slot idx (GUN_LEVEL_MASK); 0 when the slot holds no gun. */
s32 Box_GetGunLevel(SaveData *save, s16 idx)
{
    if (Box_SlotIsGun(save, idx) == 0) {
        return 0;
    }
    return save->box[idx].level & GUN_LEVEL_MASK;
}

/* 1 when the gun in box slot idx has the option bits in mask (GUN_OPT_*), 0 when not; 255 when the
 * slot holds no gun. */
s32 Box_TestGunOption(SaveData *save, s16 idx, s32 mask)
{
    if (Box_SlotIsGun(save, idx) == 0) {
        return 255;
    }
    if (save->box[idx].level & mask) {
        return 1;
    }
    return 0;
}

/* Fits the barrel to the gun in box slot idx, taking off the silencer and the shield (off 0), or
 * takes the barrel off (off nonzero). */
void Box_SetGunBarrel(SaveData *save, s16 idx, s32 off)
{
    if (Box_SlotIsGun(save, idx)) {
        if (off == 0) {
            save->box[idx].level |= GUN_OPT_BARREL;
            save->box[idx].level &= ~GUN_OPT_SILENCER;
            save->box[idx].level &= ~GUN_OPT_SHIELD;
        } else {
            save->box[idx].level &= ~GUN_OPT_BARREL;
        }
    }
}

/* Fits the silencer to the gun in box slot idx, taking off the barrel (off 0), or takes the
 * silencer off (off nonzero). */
void Box_SetGunSilencer(SaveData *save, s16 idx, s32 off)
{
    if (Box_SlotIsGun(save, idx)) {
        if (off == 0) {
            save->box[idx].level |= GUN_OPT_SILENCER;
            save->box[idx].level &= ~GUN_OPT_BARREL;
        } else {
            save->box[idx].level &= ~GUN_OPT_SILENCER;
        }
    }
}

/* Fits the scope to the gun in box slot idx (off 0), or takes it off (off nonzero). */
void Box_SetGunScope(SaveData *save, s16 idx, s32 off)
{
    if (Box_SlotIsGun(save, idx)) {
        if (off == 0) {
            save->box[idx].level |= GUN_OPT_SCOPE;
        } else {
            save->box[idx].level &= ~GUN_OPT_SCOPE;
        }
    }
}

/* Fits the shield to the gun in box slot idx, taking off the barrel (off 0), or takes the shield
 * off (off nonzero). */
void Box_SetGunShield(SaveData *save, s16 idx, s32 off)
{
    if (Box_SlotIsGun(save, idx)) {
        if (off == 0) {
            save->box[idx].level |= GUN_OPT_SHIELD;
            save->box[idx].level &= ~GUN_OPT_BARREL;
        } else {
            save->box[idx].level &= ~GUN_OPT_SHIELD;
        }
    }
}

/* The data of armor type/id. */
static ArmorData *Armor_GetData(u8 type, s16 id)
{
    return gArmorTables[type] + id;
}

/* 1 when the save's hunter can wear an armour piece: its ArmorData.flags must hold the hunter's
 * gender (1 male, 2 female) and the weapon's side (4 blademaster, 8 gunner). Always 1 for a weapon.
 */
s32 Equip_CanWear(SaveData *save, EquipItem *item)
{
    u8 mask = 0;

    if (save->gender == 0) {
        mask |= 1;
    } else {
        mask |= 2;
    }
    if (save->equip[SAVE_EQUIP_WEAPON].type == EQUIP_GUN) {
        mask |= 8;
    } else {
        mask |= 4;
    }
    switch (item->type) {
    case EQUIP_HEAD:
        if ((mask & gArmorHeadData[item->id].flags) != mask) {
            return 0;
        }
        break;
    case EQUIP_BODY:
        if ((mask & gArmorBodyData[item->id].flags) != mask) {
            return 0;
        }
        break;
    case EQUIP_WAIST:
        if ((mask & gArmorWaistData[item->id].flags) != mask) {
            return 0;
        }
        break;
    case EQUIP_ARMS:
        if ((mask & gArmorArmsData[item->id].flags) != mask) {
            return 0;
        }
        break;
    case EQUIP_LEGS:
        if ((mask & gArmorLegsData[item->id].flags) != mask) {
            return 0;
        }
        break;
    case 1:
    case EQUIP_BLADE:
    case EQUIP_GUN:
        break;
    }
    return 1;
}

/* Who can wear an item of equipment: its armour piece's ArmorData.flags (1 male, 2 female, 4
 * blademaster, 8 gunner); the whole mask 0xF for a weapon, 0 for an unknown type. */
u8 Equip_GetWearMask(SaveData *save, EquipItem *item)
{
    switch (item->type) {
    case EQUIP_HEAD:
        return gArmorHeadData[item->id].flags;
    case EQUIP_BODY:
        return gArmorBodyData[item->id].flags;
    case EQUIP_WAIST:
        return gArmorWaistData[item->id].flags;
    case EQUIP_ARMS:
        return gArmorArmsData[item->id].flags;
    case EQUIP_LEGS:
        return gArmorLegsData[item->id].flags;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        return 15;
    }
    return 0;
}

/* Adds n to the monster's hunted count (at most 9999). */
void Record_AddHunted(SaveData *save, u8 monster, u16 n)
{
    save->hunted[monster] += n;
    if (save->hunted[monster] > RECORD_COUNT_MAX) {
        save->hunted[monster] = RECORD_COUNT_MAX;
    }
}

/* Keeps the largest size of a monster. */
void Record_SetLargest(SaveData *save, u16 monster, u16 size)
{
    if (save->sizeMax[monster] < size) {
        save->sizeMax[monster] = size;
        return;
    }
}

/* Keeps the smallest (nonzero) size of a monster. */
void Record_SetSmallest(SaveData *save, u16 monster, u16 size)
{
    if (size != 0 && (save->sizeMin[monster] > size || save->sizeMin[monster] == 0)) {
        save->sizeMin[monster] = size;
    }
}

/* How many of a monster the save has hunted. */
u16 Record_GetHunted(SaveData *save, u8 monster)
{
    return save->hunted[monster];
}

/* The largest size recorded for a monster. */
u16 Record_GetLargest(SaveData *save, u16 monster)
{
    return save->sizeMax[monster];
}

/* The smallest size recorded for a monster (0: none). */
u16 Record_GetSmallest(SaveData *save, u16 monster)
{
    return save->sizeMin[monster];
}

/* How many of a monster the save has captured. */
u16 Record_GetCaptured(SaveData *save, u8 monster)
{
    return save->captured[monster];
}

/* A size in percent times the monster's base size (gMonsterBaseSize), divided by 100. */
f32 Monster_SizeFromPercent(u8 kind, u16 x)
{
    return x * gMonsterBaseSize[kind] / 100.0f;
}

/*
 * The largest size in a monster group (gMonsterGroups: the monster, then its
 * variants, ended by 255), and which monster it was.
 */
u16 Record_GroupLargest(SaveData *save, u8 group, u8 *out)
{
    u16 best = 0;
    u8 *member = gMonsterGroups[group];
    u16 size;

    for (; *member != 255; member++) {
        size = Record_GetLargest(save, *member);
        if (best < size) {
            best = size;
            if (out != NULL) {
                *out = *member;
            }
        }
    }
    return best;
}

/* As Record_GroupLargest, on the size list given (by monster kind) instead of a save. */
u16 MonsterGroup_Max(u16 *sizes, u8 group, u8 *out)
{
    u16 best = 0;
    u8 *member = gMonsterGroups[group];
    u16 size;

    for (; *member != 255; member++) {
        size = sizes[*member];
        if (best < size) {
            best = size;
            if (out != NULL) {
                *out = *member;
            }
        }
    }
    return best;
}

/* The smallest size in a monster group, 0 when none was recorded. */
u16 Record_GroupSmallest(SaveData *save, u8 group, u8 *out)
{
    u16 best;
    u8 *member;
    u16 size;

    member = gMonsterGroups[group];
    best = Record_GroupLargest(save, group, out);
    if (best == 0) {
        return 0;
    }
    for (; *member != 255; member++) {
        size = Record_GetSmallest(save, *member);
        if (size != 0) {
            if (best == 0) {
                best = size;
                if (out != NULL) {
                    *out = *member;
                }
            } else if (size < best) {
                best = size;
                if (out != NULL) {
                    *out = *member;
                }
            }
        }
    }
    return best;
}

/* As Record_GroupSmallest, on the size list given (by monster kind): the smallest nonzero size in
 * the group, 0 when none; out gets which monster it was. */
u16 MonsterGroup_MinNonzero(u16 *sizes, u8 group, u8 *out)
{
    u16 best = 0;
    u8 *member = gMonsterGroups[group];
    u16 size;

    for (; *member != 255; member++) {
        size = sizes[*member];
        if (size != 0) {
            if (best == 0) {
                best = size;
                if (out != NULL) {
                    *out = *member;
                }
            } else if (size < best) {
                best = size;
                if (out != NULL) {
                    *out = *member;
                }
            }
        }
    }
    return best;
}

/* The lobby's caller passes a third argument (0), which it does not read. */
u16 MonsterGroup_Sum(u16 *counts, u8 group, s32 unused)
{
    u16 sum = 0;
    u8 *member = gMonsterGroups[group];

    for (; *member != 255; member++) {
        sum += counts[*member];
    }
    if (sum > RECORD_COUNT_MAX) {
        sum = RECORD_COUNT_MAX;
    }
    return sum;
}

/* Hunted (and captured, when captured == 1) total of a monster group, at most RECORD_COUNT_MAX. */
u16 Records_HuntedTotal(SaveData *save, u8 group, u8 captured)
{
    u16 sum = 0;
    u8 *member = gMonsterGroups[group];

    for (; *member != 255; member++) {
        sum += save->hunted[*member];
        if (captured == 1) {
            sum += save->captured[*member];
        }
    }
    if (sum > RECORD_COUNT_MAX) {
        sum = RECORD_COUNT_MAX;
    }
    return sum;
}

/* How many of a monster group's monsters the save has hunted, at most RECORD_COUNT_MAX. */
u16 Record_GroupHunted(SaveData *save, u8 group)
{
    u16 sum = 0;
    u8 *member = gMonsterGroups[group];

    for (; *member != 255; member++) {
        sum += Record_GetHunted(save, *member);
    }
    if (sum > RECORD_COUNT_MAX) {
        sum = RECORD_COUNT_MAX;
    }
    return sum;
}

/* How many of a monster group's monsters the save has captured, at most RECORD_COUNT_MAX. */
u16 Record_GroupCaptured(SaveData *save, u8 group)
{
    u16 sum = 0;
    u8 *member = gMonsterGroups[group];

    for (; *member != 255; member++) {
        sum += Record_GetCaptured(save, *member);
    }
    if (sum > RECORD_COUNT_MAX) {
        sum = RECORD_COUNT_MAX;
    }
    return sum;
}

/* The first monster kind of a monster group. */
u8 MonsterGroup_First(u8 group)
{
    return *gMonsterGroups[group];
}

/* 1 when the save's weapon is a bowgun (a gun-type weapon of another class than the bow) and the
 * save's pouch holds no ammo it loads (ItemData.flags 2, of an ammo kind in Bowgun_GetAmmoMask);
 * else 0. */
s32 Equip_GunHasNoAmmo(Player *pl, SaveData *save)
{
    s32 mask;
    s16 i;
    ItemData *itemData;

    if (save->equip[SAVE_EQUIP_WEAPON].type != EQUIP_GUN) {
        return 0;
    }
    if (gWeaponKindClass[gGunData[save->equip[SAVE_EQUIP_WEAPON].id].kind] == WEAPON_BOW) {
        return 0;
    }
    Player_CalcSkills(pl);
    mask = Bowgun_GetAmmoMask(pl);
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (save->pouch[i].item != 0 && save->pouch[i].count > 0) {
            itemData = &gItemData[save->pouch[i].item];
            if ((itemData->flags & 2) && (mask & (1 << itemData->ammoKind))) {
                return 0;
            }
        }
    }
    return 1;
}

/* Called when the player gets an item (pouch code). */
void Save_MarkItemObtained(SaveData *save, u16 item)
{
    Save_SetItemObtained(save, item);
}

/* The size of the buffer Save_BuildSummary builds for a mode. */
s32 Save_SummarySize(s32 mode)
{
    if (mode == SAVE_SUMMARY_BOX) {
        return sizeof(BoxPage);
    } else if (mode == SAVE_SUMMARY_EQUIPPED) {
        return sizeof(EquipSummary);
    }
    return 0;
}

/* Builds a summary of the save data for the lobby and returns it: in mode
 * SAVE_SUMMARY_BOX, SUMMARY_BOX_SLOTS slots of the box from page * SUMMARY_BOX_SLOTS (type 255 for
 * an empty one); in SAVE_SUMMARY_EQUIPPED, the equipped items and the play time. Both carry the item
 * bits, the hunter rank and its points. NULL for another mode. */
void *Save_BuildSummary(s32 mode, s32 page)
{
    s16 capacity;
    s16 i;
    s16 slot;
    SaveData *save = &gSave;
    BoxPage *boxPage = &gSaveSummaryBox;
    EquipSummary *equipSummary = &gSaveSummaryEquip;

    capacity = Box_GetCapacity(save);
    if (mode == SAVE_SUMMARY_BOX) {
        slot = page * SUMMARY_BOX_SLOTS;
        for (i = 0; i < SUMMARY_BOX_SLOTS; i++, slot++) {
            if (slot >= capacity || save->box[slot].used == 0) {
                boxPage->type[i] = 255;
                boxPage->id[i] = 0;
                boxPage->level[i] = 0;
                boxPage->deco0[i] = 0;
                boxPage->deco1[i] = 0;
                boxPage->deco2[i] = 0;
            } else {
                boxPage->type[i] = save->box[slot].type;
                boxPage->id[i] = save->box[slot].id;
                boxPage->level[i] = save->box[slot].level;
                boxPage->deco0[i] = save->box[slot].deco[0];
                boxPage->deco1[i] = save->box[slot].deco[1];
                boxPage->deco2[i] = save->box[slot].deco[2];
            }
        }
        for (i = 0; i < ITEM_BITS_WORDS; i++) {
            boxPage->itemBits[i] = save->itemBits[i];
        }
        boxPage->hrPoints = save->hrPoints;
        boxPage->hr = save->hr;
        return &gSaveSummaryBox;
    }
    if (mode == SAVE_SUMMARY_EQUIPPED) {
        equipSummary->weaponType = save->equip[SAVE_EQUIP_WEAPON].type;
        equipSummary->weaponId = save->equip[SAVE_EQUIP_WEAPON].id;
        equipSummary->level[0] = save->equip[SAVE_EQUIP_WEAPON].level;
        equipSummary->deco0[0] = save->equip[SAVE_EQUIP_WEAPON].deco[0];
        equipSummary->deco1[0] = save->equip[SAVE_EQUIP_WEAPON].deco[1];
        equipSummary->deco2[0] = save->equip[SAVE_EQUIP_WEAPON].deco[2];
        equipSummary->headId = save->equip[SAVE_EQUIP_HEAD].id;
        equipSummary->level[1] = save->equip[SAVE_EQUIP_HEAD].level;
        equipSummary->deco0[1] = save->equip[SAVE_EQUIP_HEAD].deco[0];
        equipSummary->deco1[1] = save->equip[SAVE_EQUIP_HEAD].deco[1];
        equipSummary->deco2[1] = save->equip[SAVE_EQUIP_HEAD].deco[2];
        equipSummary->bodyId = save->equip[SAVE_EQUIP_BODY].id;
        equipSummary->level[2] = save->equip[SAVE_EQUIP_BODY].level;
        equipSummary->deco0[2] = save->equip[SAVE_EQUIP_BODY].deco[0];
        equipSummary->deco1[2] = save->equip[SAVE_EQUIP_BODY].deco[1];
        equipSummary->deco2[2] = save->equip[SAVE_EQUIP_BODY].deco[2];
        equipSummary->armsId = save->equip[SAVE_EQUIP_ARMS].id;
        equipSummary->level[3] = save->equip[SAVE_EQUIP_ARMS].level;
        equipSummary->deco0[3] = save->equip[SAVE_EQUIP_ARMS].deco[0];
        equipSummary->deco1[3] = save->equip[SAVE_EQUIP_ARMS].deco[1];
        equipSummary->deco2[3] = save->equip[SAVE_EQUIP_ARMS].deco[2];
        equipSummary->waistId = save->equip[SAVE_EQUIP_WAIST].id;
        equipSummary->level[4] = save->equip[SAVE_EQUIP_WAIST].level;
        equipSummary->deco0[4] = save->equip[SAVE_EQUIP_WAIST].deco[0];
        equipSummary->deco1[4] = save->equip[SAVE_EQUIP_WAIST].deco[1];
        equipSummary->deco2[4] = save->equip[SAVE_EQUIP_WAIST].deco[2];
        equipSummary->legsId = save->equip[SAVE_EQUIP_LEGS].id;
        equipSummary->level[5] = save->equip[SAVE_EQUIP_LEGS].level;
        equipSummary->deco0[5] = save->equip[SAVE_EQUIP_LEGS].deco[0];
        equipSummary->deco1[5] = save->equip[SAVE_EQUIP_LEGS].deco[1];
        equipSummary->deco2[5] = save->equip[SAVE_EQUIP_LEGS].deco[2];
        equipSummary->hrPoints = save->hrPoints;
        equipSummary->hr = save->hr;
        equipSummary->playTime = save->playTime;
        for (i = 0; i < ITEM_BITS_WORDS; i++) {
            equipSummary->itemBits[i] = save->itemBits[i];
        }
        return &gSaveSummaryEquip;
    }
    return NULL;
}

/* Copies the player's pouch to gPouchBackup (load == 0) or back. */
void Player_BackupPouch(Player *pl, s32 load)
{
    if (load == 0) {
        memcpy(gPouchBackup, pl->pouch, sizeof(pl->pouch));
    } else {
        memcpy(pl->pouch, gPouchBackup, sizeof(pl->pouch));
    }
}

/* Box pages owned: 1 + how many of the items ITEM_ID_BOX_UPGRADE_1 to ITEM_ID_HOUSE_UPGRADE_2 are obtained (reads gSave, not save). */
s16 Box_GetPageCount(SaveData *save)
{
    s32 n = 1;
    s32 i;

    for (i = ITEM_ID_BOX_UPGRADE_1; i <= ITEM_ID_HOUSE_UPGRADE_2; i++) {
        if (Save_IsItemObtained(&gSave, i) == 1) {
            n++;
        }
    }
    return n;
}

/* Box capacity: BOX_PAGE_SLOTS slots per page. */
s16 Box_GetCapacity(SaveData *save)
{
    return Box_GetPageCount(save) * BOX_PAGE_SLOTS;
}

/* Wraps a box index: negative ones up by pages of BOX_PAGE_SLOTS, others into the pages owned. */
s16 Box_WrapIndex(SaveData *save, s16 idx)
{
    u8 last;
    s16 page;

    if (idx <= 0) {
        while (idx < 0) {
            idx += BOX_PAGE_SLOTS;
        }
        return idx;
    }
    last = Box_GetPageCount(save) - 1;
    page = idx / BOX_PAGE_SLOTS;
    if (last < page) {
        page = last;
    }
    return idx % BOX_PAGE_SLOTS + page * BOX_PAGE_SLOTS;
}

/* Whether the player meets a quest requirement of a kind: 0 and 1 the attack (times the weapon
 * class's multiplier) at least or at most value; 2 and 3 the defense; 4/5, 6/7, 8/9, 10/11 and
 * 14/15 the resistances 0, 1, 2, 3 and 4 (the even kind: at least; the odd: at most); 12 no armour
 * worn; 13 an empty pouch; 16 nothing in the pouch but items with ItemData.flags 2 or 4; 17 none of
 * the items 39, ITEM_ID_LOBBED_NO_VERSUS, ITEM_ID_THROWN_NO_VERSUS and 130. Returns 1 when met, 0 when not or for another kind. */
s32 Player_CheckRequirement(Player *pl, u16 kind, s32 value)
{
    s32 stat;
    s32 i;

    Player_CalcSkills(pl);
    switch (kind) {
    case 0:
    case 1:
        Player_CalcAttack(pl);
        stat = pl->attack * gWeaponClassMultipliers[Weapon_GetClass(pl->equip[EQUIP_SLOT_WEAPON].type, pl->equip[EQUIP_SLOT_WEAPON].id)] / 100;
        if (kind == 0) {
            if (stat >= value) {
                return 1;
            }
        } else {
            if (stat <= value) {
                return 1;
            }
        }
        break;
    case 2:
    case 3:
        Player_CalcDefense(pl);
        stat = pl->defense;
        if (kind == 2) {
            if (stat >= value) {
                return 1;
            }
        } else {
            if (stat <= value) {
                return 1;
            }
        }
        break;
    case 4:
    case 5:
        Player_CalcResists(pl);
        stat = pl->resist[0];
        goto check;
    case 6:
    case 7:
        Player_CalcResists(pl);
        stat = pl->resist[1];
        goto check;
    case 8:
    case 9:
        Player_CalcResists(pl);
        stat = pl->resist[2];
        goto check;
    case 14:
    case 15:
        Player_CalcResists(pl);
        stat = pl->resist[4];
        goto check;
    case 10:
    case 11:
        Player_CalcResists(pl);
        stat = pl->resist[3];
    check:
        if ((kind == 4 || kind == 6 || kind == 8 || kind == 10 || kind == 14)) {
            if (stat >= value) {
                return 1;
            }
        } else {
            if (stat <= value) {
                return 1;
            }
        }
        break;
    case 12:
        for (i = 0; i < 6; i++) {
            if (i != 1 && pl->equip[i].id != 0) {
                return 0;
            }
        }
        return 1;
    case 13:
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (pl->pouch[i].count > 0) {
                return 0;
            }
        }
        return 1;
    case 16:
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (pl->pouch[i].item != 0 && pl->pouch[i].count > 0
                && (gItemData[pl->pouch[i].item].flags & 6) == 0) {
                return 0;
            }
        }
        return 1;
    case 17:
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (pl->pouch[i].item != 0 && pl->pouch[i].count > 0) {
                switch (pl->pouch[i].item) {
                case ITEM_ID_THROWN_NO_VERSUS:
                case ITEM_ID_LOBBED_NO_VERSUS:
                case 130:
                case 39:
                    return 0;
                }
            }
        }
        return 1;
    }
    return 0;
}

u8 Village_FacilityLevel(s32 k);

/* The average level of the village facilities 1-3. */
u8 Village_AverageLevel(void)
{
    u8 sum = Village_FacilityLevel(1) + Village_FacilityLevel(2) + Village_FacilityLevel(3);

    return sum / 3;
}

/* The sum of the levels of the village facilities 1-3. */
u8 Village_TotalLevel(void)
{
    return Village_FacilityLevel(1) + Village_FacilityLevel(2) + Village_FacilityLevel(3);
}

/* A village facility's level (SaveData.facilityLevel): facility 4 reads at most 2, and facility 5
 * is the box pages bought (Box_GetPageCount less 1). */
u8 Village_FacilityLevel(s32 facility)
{
    if (facility == 5) {
        return Box_GetPageCount(&gSave) - 1;
    }
    if (facility == 4 && gSave.facilityLevel[facility] > 2) {
        return 2;
    }
    return gSave.facilityLevel[facility];
}

/* Sets a village facility's level. */
void Village_SetFacilityLevel(s32 facility, u8 level)
{
    gSave.facilityLevel[facility] = level;
}

/* Sets gSave's symbol colour. */
void Save_SetSymbolColor(u8 color)
{
    gSave.symbolColor = color;
}

/* Sets the player's style (the symbol colour) and its colour word from gPlayerColorWords. */
void Player_SetColorWord(Player *pl, u8 idx)
{
    pl->style = idx;
    pl->styleColor = gPlayerColorWords[idx];
}

/* 1 when bit 0 of the part flags of an armour piece's model is set (gPlayerPartFlagTbls, by the
 * armour type, for the male model (gender 0) or the female one); 0 for a weapon or no piece.
 * Save_PickBestCounter goes on only for such pieces. */
s32 Armor_HasPartFlag(EquipItem *item, u8 gender)
{
    u8 type;
    u16 id;
    ArmorData *rec;
    u8 model;

    type = item->type;
    if (type == EQUIP_BLADE || type == EQUIP_GUN) {
        return 0;
    }
    id = item->id;
    if (id == 0) {
        return 0;
    }
    rec = gArmorTables[type];
    rec += id;
    if (gender == 0) {
        model = rec->modelMale;
    } else {
        model = rec->modelFemale;
    }
    if (gPlayerPartFlagTbls[type + gender * 6][model] & 1) {
        return 1;
    }
    return 0;
}

/* Marks an item obtained; flag 0x27 once 15 of the items at gObtainFlagItems are. */
void Save_SetItemObtained(SaveData *save, u16 item)
{
    u16 *entry;
    s16 count = 0;

    if (item != 0xFFFF && item < ITEM_ID_COUNT) {
        save->itemBits[item / 32] |= 1 << (item & 0x1F);
    }
    if (Save_HasTitle(39) != 1) {
        entry = gObtainFlagItems;
        if (gObtainFlagItems[0] != 0xFFFF) {
            do {
                if (Save_IsItemObtained(save, *entry++) == 1) {
                    count++;
                }
            } while (*entry != 0xFFFF);
        }
        if (count >= 15) {
            Award_Grant(39);
        }
    }
}

/* 1 when item's bit is set in the bits given (one per item id), 0 when not; 65535 for an id out of
 * range (65535 or ITEM_ID_COUNT and above). */
s32 ItemBits_Test(u32 *bits, u16 item)
{
    if (item == 0xFFFF || item >= ITEM_ID_COUNT) {
        return 65535;
    }
    return (bits[item / 32] & (1 << (item & 0x1F))) != 0;
}

/* 1 when the save has obtained an item (SaveData.itemBits); 65535 for an id out of range. */
s32 Save_IsItemObtained(SaveData *save, u16 item)
{
    if (item == 0xFFFF || item >= ITEM_ID_COUNT) {
        return 65535;
    }
    return ItemBits_Test(save->itemBits, item);
}

/* The defense factor of an armour upgrade level, by the armour's defense row (gArmorLevelDefense, 7
 * levels a row). */
f32 ArmorLevel_Defense(u8 row, u8 level)
{
    return gArmorLevelDefense[row * 7 + level];
}

/* The cost factor of an armour upgrade level (at most 6), by the armour's cost row
 * (gArmorLevelCost); type isn't read. */
f32 ArmorLevel_CostFactor(u8 row, u8 level, u8 type)
{
    if (level > 6) {
        level = 6;
    }
    return gArmorLevelCost[row * 7 + level];
}

/* The decoration slots an armour upgrade level (at most 6) adds, by the armour's slot row
 * (gArmorLevelSlots). */
u8 ArmorLevel_SlotBonus(u8 row, u8 level)
{
    if (level > 6) {
        level = 6;
    }
    return gArmorLevelSlots[row * 7 + level];
}

/* Armor defense: the base times one factor per upgrade level, rounded up each time. */
u32 Armor_GetDefense(EquipItem *item)
{
    ArmorData *rec;
    f32 def;
    s16 i;
    s16 n;

    if (item->id == 0) {
        return 0;
    }
    rec = gArmorTables[item->type];
    rec += item->id;
    def = rec->defense;
    n = (u8)item->level;
    for (i = 0; i < n; i++) {
        def *= ArmorLevel_Defense(rec->defenseRow, (u8)i + 1);
        if (def > Math_Floor(def)) {
            def = Math_Floor(def) + 1.0f;
        }
    }
    return def;
}

/* Weapons: a byte of their data; armor: unk10 plus a per-level bonus, at most unk11. */
u8 Equip_GetSlotCount(EquipItem *item)
{
    ArmorData *rec;
    u8 slotCount;

    if (item->type == EQUIP_BLADE || item->type == EQUIP_GUN) {
        if (item->type == EQUIP_BLADE) {
            return gBladeData[item->id].slots;
        } else {
            return gGunData[item->id].slots;
        }
    }
    rec = gArmorTables[item->type];
    rec += item->id;
    slotCount = rec->slots + ArmorLevel_SlotBonus(rec->slotRow, item->level);
    if (slotCount >= rec->slotsMax) {
        slotCount = rec->slotsMax;
    }
    return slotCount;
}

/* Armor upgrade cost: price times the factor of the current level (0 at the maximum). */
s32 Armor_GetUpgradeCost(EquipItem *item)
{
    u8 type = item->type;
    ArmorData *rec = gArmorTables[type];
    u8 level;
    f32 price;

    rec += item->id;
    level = item->level;
    if (level >= rec->maxLevel || level >= 7) {
        return 0;
    }
    price = rec->price;
    price *= ArmorLevel_CostFactor(rec->costRow, level, type);
    return price;
}

/* Armor sell price: (price + price times the level factor) / 2. */
s32 Armor_GetSellPrice(EquipItem *item)
{
    u8 type = item->type;
    ArmorData *rec = gArmorTables[type];
    u8 level;
    f32 price;

    rec += item->id;
    level = item->level;
    price = rec->price;
    if (level > 0) {
        price += price * ArmorLevel_CostFactor(rec->costRow, level, type);
    }
    price /= 2.0f;
    return price;
}

/* The equipment's initialised data: the stage file lists, the equipment tables and their names. */
extern char D_004A00E8[];
extern char gBladeNames_1[];
extern char gBladeNames_2[];
extern char gBladeNames_3[];
extern char gBladeNames_4[];
extern char gBladeNames_5[];
extern char gBladeNames_6[];
extern char gBladeNames_7[];
extern char gBladeNames_8[];
extern char gBladeNames_9[];
extern char gBladeNames_10[];
extern char gBladeNames_11[];
extern char gBladeNames_12[];
extern char gBladeNames_13[];
extern char gBladeNames_14[];
extern char gBladeNames_15[];
extern char gBladeNames_16[];
extern char gBladeNames_17[];
extern char gBladeNames_18[];
extern char gBladeNames_19[];
extern char gBladeNames_20[];
extern char gBladeNames_21[];
extern char gBladeNames_22[];
extern char gBladeNames_23[];
extern char gBladeNames_24[];
extern char gBladeNames_25[];
extern char gBladeNames_26[];
extern char gBladeNames_27[];
extern char gBladeNames_28[];
extern char gBladeNames_29[];
extern char gBladeNames_30[];
extern char gBladeNames_31[];
extern char gBladeNames_32[];
extern char gBladeNames_33[];
extern char gBladeNames_34[];
extern char gBladeNames_35[];
extern char gBladeNames_36[];
extern char gBladeNames_37[];
extern char gBladeNames_38[];
extern char gBladeNames_39[];
extern char gBladeNames_40[];
extern char gBladeNames_41[];
extern char gBladeNames_42[];
extern char gBladeNames_43[];
extern char gBladeNames_44[];
extern char gBladeNames_45[];
extern char gBladeNames_46[];
extern char gBladeNames_47[];
extern char gBladeNames_48[];
extern char gBladeNames_49[];
extern char gBladeNames_50[];
extern char gBladeNames_51[];
extern char gBladeNames_52[];
extern char gBladeNames_53[];
extern char gBladeNames_54[];
extern char gBladeNames_55[];
extern char gBladeNames_56[];
extern char gBladeNames_57[];
extern char gBladeNames_58[];
extern char gBladeNames_59[];
extern char gBladeNames_60[];
extern char gBladeNames_61[];
extern char gBladeNames_62[];
extern char gBladeNames_63[];
extern char gBladeNames_64[];
extern char gBladeNames_65[];
extern char gBladeNames_66[];
extern char gBladeNames_67[];
extern char gBladeNames_68[];
extern char gBladeNames_69[];
extern char gBladeNames_70[];
extern char gBladeNames_71[];
extern char gBladeNames_72[];
extern char gBladeNames_73[];
extern char gBladeNames_74[];
extern char gBladeNames_75[];
extern char gBladeNames_76[];
extern char gBladeNames_77[];
extern char gBladeNames_78[];
extern char gBladeNames_79[];
extern char gBladeNames_80[];
extern char gBladeNames_81[];
extern char gBladeNames_82[];
extern char gBladeNames_83[];
extern char gBladeNames_84[];
extern char gBladeNames_85[];
extern char gBladeNames_86[];
extern char gBladeNames_87[];
extern char gBladeNames_88[];
extern char gBladeNames_89[];
extern char gBladeNames_90[];
extern char gBladeNames_91[];
extern char gBladeNames_92[];
extern char gBladeNames_93[];
extern char gBladeNames_94[];
extern char gBladeNames_95[];
extern char gBladeNames_96[];
extern char gBladeNames_97[];
extern char gBladeNames_98[];
extern char gBladeNames_99[];
extern char gBladeNames_100[];
extern char gBladeNames_101[];
extern char gBladeNames_102[];
extern char gBladeNames_103[];
extern char gBladeNames_104[];
extern char gBladeNames_105[];
extern char gBladeNames_106[];
extern char gBladeNames_107[];
extern char gBladeNames_108[];
extern char gBladeNames_109[];
extern char gBladeNames_110[];
extern char gBladeNames_111[];
extern char gBladeNames_112[];
extern char gBladeNames_113[];
extern char gBladeNames_114[];
extern char gBladeNames_115[];
extern char gBladeNames_116[];
extern char gBladeNames_117[];
extern char gBladeNames_118[];
extern char gBladeNames_119[];
extern char gBladeNames_120[];
extern char gBladeNames_121[];
extern char gBladeNames_122[];
extern char gBladeNames_123[];
extern char gBladeNames_124[];
extern char gBladeNames_125[];
extern char gBladeNames_126[];
extern char gBladeNames_127[];
extern char gBladeNames_128[];
extern char gBladeNames_129[];
extern char gBladeNames_130[];
extern char gBladeNames_131[];
extern char gBladeNames_132[];
extern char gBladeNames_133[];
extern char gBladeNames_134[];
extern char gBladeNames_135[];
extern char gBladeNames_136[];
extern char gBladeNames_137[];
extern char gBladeNames_138[];
extern char gBladeNames_139[];
extern char gBladeNames_140[];
extern char gBladeNames_141[];
extern char gBladeNames_142[];
extern char gBladeNames_143[];
extern char gBladeNames_144[];
extern char gBladeNames_145[];
extern char gBladeNames_146[];
extern char gBladeNames_147[];
extern char gBladeNames_148[];
extern char gBladeNames_149[];
extern char gBladeNames_150[];
extern char gBladeNames_151[];
extern char gBladeNames_152[];
extern char gBladeNames_153[];
extern char gBladeNames_154[];
extern char gBladeNames_155[];
extern char gBladeNames_156[];
extern char gBladeNames_157[];
extern char gBladeNames_158[];
extern char gBladeNames_159[];
extern char gBladeNames_160[];
extern char gBladeNames_161[];
extern char gBladeNames_162[];
extern char gBladeNames_163[];
extern char gBladeNames_164[];
extern char gBladeNames_165[];
extern char gBladeNames_166[];
extern char gBladeNames_167[];
extern char gBladeNames_168[];
extern char gBladeNames_169[];
extern char gBladeNames_170[];
extern char gBladeNames_171[];
extern char gBladeNames_172[];
extern char gBladeNames_173[];
extern char gBladeNames_174[];
extern char gBladeNames_175[];
extern char gBladeNames_176[];
extern char gBladeNames_177[];
extern char gBladeNames_178[];
extern char gBladeNames_179[];
extern char gBladeNames_180[];
extern char gBladeNames_181[];
extern char gBladeNames_182[];
extern char gBladeNames_183[];
extern char gBladeNames_184[];
extern char gBladeNames_185[];
extern char gBladeNames_186[];
extern char gBladeNames_187[];
extern char gBladeNames_188[];
extern char gBladeNames_189[];
extern char gBladeNames_190[];
extern char gBladeNames_191[];
extern char gBladeNames_192[];
extern char gBladeNames_193[];
extern char gBladeNames_194[];
extern char gBladeNames_195[];
extern char gBladeNames_196[];
extern char gBladeNames_197[];
extern char gBladeNames_198[];
extern char gBladeNames_199[];
extern char gBladeNames_200[];
extern char gBladeNames_201[];
extern char gBladeNames_202[];
extern char gBladeNames_203[];
extern char gBladeNames_204[];
extern char gBladeNames_205[];
extern char gBladeNames_206[];
extern char gBladeNames_207[];
extern char gBladeNames_208[];
extern char gBladeNames_209[];
extern char gBladeNames_210[];
extern char gBladeNames_211[];
extern char gBladeNames_212[];
extern char gBladeNames_213[];
extern char gBladeNames_214[];
extern char gBladeNames_215[];
extern char gBladeNames_216[];
extern char gBladeNames_217[];
extern char gBladeNames_218[];
extern char gBladeNames_219[];
extern char gBladeNames_220[];
extern char gBladeNames_221[];
extern char gBladeNames_222[];
extern char gBladeNames_223[];
extern char gBladeNames_224[];
extern char gBladeNames_225[];
extern char gBladeNames_226[];
extern char gBladeNames_227[];
extern char gBladeNames_228[];
extern char gBladeNames_229[];
extern char gBladeNames_230[];
extern char gBladeNames_231[];
extern char gBladeNames_232[];
extern char gBladeNames_233[];
extern char gBladeNames_234[];
extern char gBladeNames_235[];
extern char gBladeNames_236[];
extern char gBladeNames_237[];
extern char gBladeNames_238[];
extern char gBladeNames_239[];
extern char gBladeNames_240[];
extern char gBladeNames_241[];
extern char gBladeNames_242[];
extern char gBladeNames_243[];
extern char gBladeNames_244[];
extern char gBladeNames_245[];
extern char gBladeNames_246[];
extern char gBladeNames_247[];
extern char gBladeNames_248[];
extern char gBladeNames_249[];
extern char gBladeNames_250[];
extern char gBladeNames_251[];
extern char gBladeNames_252[];
extern char gBladeNames_253[];
extern char gBladeNames_254[];
extern char gBladeNames_255[];
extern char gBladeNames_256[];
extern char gBladeNames_257[];
extern char gBladeNames_258[];
extern char gBladeNames_259[];
extern char gBladeNames_260[];
extern char gBladeNames_261[];
extern char gBladeNames_262[];
extern char gBladeNames_263[];
extern char gBladeNames_264[];
extern char gBladeNames_265[];
extern char gBladeNames_266[];
extern char gBladeNames_267[];
extern char gBladeNames_268[];
extern char gBladeNames_269[];
extern char gBladeNames_270[];
extern char gBladeNames_271[];
extern char gBladeNames_272[];
extern char gBladeNames_273[];
extern char gBladeNames_274[];
extern char gBladeNames_275[];
extern char gBladeNames_276[];
extern char gBladeNames_277[];
extern char gBladeNames_278[];
extern char gBladeNames_279[];
extern char gBladeNames_280[];
extern char gBladeNames_281[];
extern char gBladeNames_282[];
extern char gBladeNames_283[];
extern char gBladeNames_284[];
extern char gBladeNames_285[];
extern char gBladeNames_286[];
extern char gBladeNames_287[];
extern char gBladeNames_288[];
extern char gBladeNames_289[];
extern char gBladeNames_290[];
extern char gBladeNames_291[];
extern char gBladeNames_292[];
extern char gBladeNames_293[];
extern char gBladeNames_294[];
extern char gBladeNames_295[];
extern char gBladeNames_296[];
extern char gBladeNames_297[];
extern char gBladeNames_298[];
extern char gBladeNames_299[];
extern char gBladeNames_300[];
extern char gBladeNames_301[];
extern char gBladeNames_302[];
extern char gBladeNames_303[];
extern char gBladeNames_304[];
extern char gBladeNames_305[];
extern char gBladeNames_306[];
extern char gBladeNames_307[];
extern char gBladeNames_308[];
extern char gBladeNames_309[];
extern char gBladeNames_310[];
extern char gBladeNames_311[];
extern char gBladeNames_312[];
extern char gBladeNames_313[];
extern char gBladeNames_314[];
extern char gBladeNames_315[];
extern char gBladeNames_316[];
extern char gBladeNames_317[];
extern char gBladeNames_318[];
extern char gBladeNames_319[];
extern char gBladeNames_320[];
extern char gBladeNames_321[];
extern char gBladeNames_322[];
extern char gBladeNames_323[];
extern char gBladeNames_324[];
extern char gBladeNames_325[];
extern char gBladeNames_326[];
extern char gBladeNames_327[];
extern char gBladeNames_328[];
extern char gBladeNames_329[];
extern char gBladeNames_330[];
extern char gBladeNames_331[];
extern char gBladeNames_332[];
extern char gBladeNames_333[];
extern char gBladeNames_334[];
extern char gBladeNames_335[];
extern char gBladeNames_336[];
extern char gBladeNames_337[];
extern char gBladeNames_338[];
extern char gBladeNames_339[];
extern char gBladeNames_340[];
extern char gBladeNames_341[];
extern char gBladeNames_342[];
extern char gBladeNames_343[];
extern char gBladeNames_344[];
extern char gBladeNames_345[];
extern char gBladeNames_346[];
extern char gBladeNames_347[];
extern char gBladeNames_348[];
extern char gBladeNames_349[];
extern char gBladeNames_350[];
extern char gBladeNames_351[];
extern char gBladeNames_352[];
extern char gBladeNames_353[];
extern char gBladeNames_354[];
extern char gBladeNames_355[];
extern char gBladeNames_356[];
extern char gBladeNames_357[];
extern char gBladeNames_358[];
extern char gBladeNames_359[];
extern char gBladeNames_360[];
extern char gBladeNames_361[];
extern char gBladeNames_362[];
extern char gBladeNames_363[];
extern char gBladeNames_364[];
extern char gBladeNames_365[];
extern char gBladeNames_366[];
extern char gBladeNames_367[];
extern char gBladeNames_368[];
extern char gBladeNames_369[];
extern char gBladeNames_370[];
extern char gBladeNames_371[];
extern char gBladeNames_372[];
extern char gBladeNames_373[];
extern char gBladeNames_374[];
extern char gBladeNames_375[];
extern char gBladeNames_376[];
extern char gBladeNames_377[];
extern char gBladeNames_378[];
extern char gBladeNames_379[];
extern char gBladeNames_380[];
extern char gBladeNames_381[];
extern char gBladeNames_382[];
extern char gBladeNames_383[];
extern char gBladeNames_384[];
extern char gBladeNames_385[];
extern char gBladeNames_386[];
extern char gBladeNames_387[];
extern char gBladeNames_388[];
extern char gBladeNames_389[];
extern char gBladeNames_390[];
extern char gBladeNames_391[];
extern char gBladeNames_392[];
extern char gBladeNames_393[];
extern char gBladeNames_394[];
extern char gBladeNames_395[];
extern char gBladeNames_396[];
extern char gBladeNames_397[];
extern char gBladeNames_398[];
extern char gBladeNames_399[];
extern char gBladeNames_400[];
extern char gBladeNames_401[];
extern char gBladeNames_402[];
extern char gBladeNames_403[];
extern char gBladeNames_404[];
extern char gBladeNames_405[];
extern char gBladeNames_406[];
extern char gBladeNames_407[];
extern char gBladeNames_408[];
extern char gBladeNames_409[];
extern char gBladeNames_410[];
extern char gBladeNames_411[];
extern char gBladeNames_412[];
extern char gBladeNames_413[];
extern char gBladeNames_414[];
extern char gBladeNames_415[];
extern char gBladeNames_416[];
extern char gBladeNames_417[];
extern char gBladeNames_418[];
extern char gBladeNames_419[];
extern char gBladeNames_420[];
extern char gBladeNames_421[];
extern char gBladeNames_422[];
extern char gBladeNames_423[];
extern char gBladeNames_424[];
extern char gBladeNames_425[];
extern char gBladeNames_426[];
extern char gBladeNames_427[];
extern char gBladeNames_428[];
extern char gBladeNames_429[];
extern char gBladeNames_430[];
extern char gBladeNames_431[];
extern char gBladeNames_432[];
extern char gBladeNames_433[];
extern char gBladeNames_434[];
extern char gBladeNames_435[];
extern char gBladeNames_436[];
extern char gBladeNames_437[];
extern char gBladeNames_438[];
extern char gBladeNames_439[];
extern char gBladeNames_440[];
extern char gBladeNames_441[];
extern char gBladeNames_442[];
extern char gBladeNames_443[];
extern char gBladeNames_444[];
extern char gBladeNames_445[];
extern char gBladeNames_446[];
extern char gBladeNames_447[];
extern char gBladeNames_448[];
extern char gBladeNames_449[];
extern char gBladeNames_450[];
extern char gBladeNames_451[];
extern char gBladeNames_452[];
extern char gBladeNames_453[];
extern char gBladeNames_454[];
extern char gBladeNames_455[];
extern char gBladeNames_456[];
extern char gBladeNames_457[];
extern char gBladeNames_458[];
extern char gBladeNames_459[];
extern char gBladeNames_460[];
extern char gBladeNames_461[];
extern char gBladeNames_462[];
extern char gBladeNames_463[];
extern char gBladeNames_464[];
extern char gBladeNames_465[];
extern char gBladeNames_466[];
extern char gBladeNames_467[];
extern char gBladeNames_468[];
extern char gBladeNames_469[];
extern char gBladeNames_470[];
extern char gBladeNames_471[];
extern char gBladeNames_472[];
extern char gBladeNames_473[];
extern char gBladeNames_474[];
extern char gBladeNames_475[];
extern char gBladeNames_476[];
extern char gBladeNames_477[];
extern char gBladeNames_478[];
extern char gBladeNames_479[];
extern char gBladeNames_480[];
extern char gBladeNames_481[];
extern char gBladeNames_482[];
extern char gBladeNames_483[];
extern char gBladeNames_484[];
extern char gBladeNames_485[];
extern char gGunNames_1[];
extern char gGunNames_2[];
extern char gGunNames_3[];
extern char gGunNames_4[];
extern char gGunNames_5[];
extern char gGunNames_6[];
extern char gGunNames_7[];
extern char gGunNames_8[];
extern char gGunNames_9[];
extern char gGunNames_10[];
extern char gGunNames_11[];
extern char gGunNames_12[];
extern char gGunNames_13[];
extern char gGunNames_14[];
extern char gGunNames_15[];
extern char gGunNames_16[];
extern char gGunNames_17[];
extern char gGunNames_18[];
extern char gGunNames_19[];
extern char gGunNames_20[];
extern char gGunNames_21[];
extern char gGunNames_22[];
extern char gGunNames_23[];
extern char gGunNames_24[];
extern char gGunNames_25[];
extern char gGunNames_26[];
extern char gGunNames_27[];
extern char gGunNames_28[];
extern char gGunNames_29[];
extern char gGunNames_30[];
extern char gGunNames_31[];
extern char gGunNames_32[];
extern char gGunNames_33[];
extern char gGunNames_34[];
extern char gGunNames_35[];
extern char gGunNames_36[];
extern char gGunNames_37[];
extern char gGunNames_38[];
extern char gGunNames_39[];
extern char gGunNames_40[];
extern char gGunNames_41[];
extern char gGunNames_42[];
extern char gGunNames_43[];
extern char gGunNames_44[];
extern char gGunNames_45[];
extern char gGunNames_46[];
extern char gGunNames_47[];
extern char gGunNames_48[];
extern char gGunNames_49[];
extern char gGunNames_50[];
extern char gGunNames_51[];
extern char gGunNames_52[];
extern char gGunNames_53[];
extern char gGunNames_54[];
extern char gGunNames_55[];
extern char gGunNames_56[];
extern char gGunNames_57[];
extern char gGunNames_58[];
extern char gGunNames_59[];
extern char gGunNames_60[];
extern char gGunNames_61[];
extern char gGunNames_62[];
extern char gGunNames_63[];
extern char gGunNames_64[];
extern char gGunNames_65[];
extern char gGunNames_66[];
extern char gGunNames_67[];
extern char gGunNames_68[];
extern char gGunNames_69[];
extern char gGunNames_70[];
extern char gGunNames_71[];
extern char gGunNames_72[];
extern char gGunNames_73[];
extern char gGunNames_74[];
extern char gGunNames_75[];
extern char gGunNames_76[];
extern u32 D_004A3378[];
extern u32 D_004A3388[];
extern u32 D_004A3398[];
extern char gGunNames_80[];
extern char gGunNames_81[];
extern char gGunNames_82[];
extern char gGunNames_83[];
extern u32 D_004A3428[];
extern char gGunNames_85[];
extern char gGunNames_86[];
extern char gGunNames_87[];
extern char gGunNames_88[];
extern char gGunNames_89[];
extern char gGunNames_90[];
extern char gGunNames_91[];
extern char gGunNames_92[];
extern char gGunNames_93[];
extern char gGunNames_94[];
extern char gGunNames_95[];
extern char gGunNames_96[];
extern char gGunNames_97[];
extern char gGunNames_98[];
extern char gGunNames_99[];
extern char gGunNames_100[];
extern char gGunNames_101[];
extern char gGunNames_102[];
extern char gGunNames_103[];
extern char gGunNames_104[];
extern char gGunNames_105[];
extern char gGunNames_106[];
extern char gGunNames_107[];
extern u32 D_004A3638[];
extern u32 D_004A3648[];
extern u32 D_004A3658[];
extern u32 D_004A3668[];
extern char gGunNames_112[];
extern char gGunNames_113[];
extern char gGunNames_114[];
extern char gGunNames_115[];
extern u32 D_004A36F8[];
extern u32 D_004A3708[];
extern char gGunNames_118[];
extern char gGunNames_119[];
extern char gGunNames_120[];
extern char gGunNames_121[];
extern char gGunNames_122[];
extern char gGunNames_123[];
extern char gGunNames_124[];
extern char gGunNames_125[];
extern char gGunNames_126[];
extern char gGunNames_127[];
extern char gArmorHeadNames_1[];
extern char gArmorHeadNames_2[];
extern char gArmorHeadNames_3[];
extern char gArmorHeadNames_4[];
extern char gArmorHeadNames_5[];
extern char gArmorHeadNames_6[];
extern char gArmorHeadNames_7[];
extern char gArmorHeadNames_8[];
extern char gArmorHeadNames_9[];
extern char gArmorHeadNames_10[];
extern char gArmorHeadNames_11[];
extern char gArmorHeadNames_12[];
extern char gArmorHeadNames_13[];
extern char gArmorHeadNames_14[];
extern char gArmorHeadNames_15[];
extern char gArmorHeadNames_16[];
extern char gArmorHeadNames_17[];
extern char gArmorHeadNames_18[];
extern char gArmorHeadNames_19[];
extern char gArmorHeadNames_20[];
extern char gArmorHeadNames_21[];
extern char gArmorHeadNames_22[];
extern char gArmorHeadNames_23[];
extern char gArmorHeadNames_24[];
extern char gArmorHeadNames_25[];
extern char gArmorHeadNames_26[];
extern char gArmorHeadNames_27[];
extern char gArmorHeadNames_28[];
extern char gArmorHeadNames_29[];
extern char gArmorHeadNames_30[];
extern char gArmorHeadNames_31[];
extern char gArmorHeadNames_32[];
extern char gArmorHeadNames_33[];
extern char gArmorHeadNames_34[];
extern char gArmorHeadNames_35[];
extern char gArmorHeadNames_36[];
extern char gArmorHeadNames_37[];
extern char gArmorHeadNames_38[];
extern char gArmorHeadNames_39[];
extern char gArmorHeadNames_40[];
extern char gArmorHeadNames_41[];
extern char gArmorHeadNames_42[];
extern char gArmorHeadNames_43[];
extern char gArmorHeadNames_44[];
extern char gArmorHeadNames_45[];
extern char gArmorHeadNames_46[];
extern char gArmorHeadNames_47[];
extern char gArmorHeadNames_48[];
extern char gArmorHeadNames_49[];
extern char gArmorHeadNames_50[];
extern char gArmorHeadNames_51[];
extern char gArmorHeadNames_52[];
extern char gArmorHeadNames_53[];
extern char gArmorHeadNames_54[];
extern char gArmorHeadNames_55[];
extern char gArmorHeadNames_56[];
extern char gArmorHeadNames_57[];
extern char gArmorHeadNames_58[];
extern char gArmorHeadNames_59[];
extern char gArmorHeadNames_60[];
extern char gArmorHeadNames_61[];
extern char gArmorHeadNames_62[];
extern char gArmorHeadNames_63[];
extern char gArmorHeadNames_64[];
extern char gArmorHeadNames_65[];
extern char gArmorHeadNames_66[];
extern char gArmorHeadNames_67[];
extern char gArmorHeadNames_68[];
extern char gArmorHeadNames_69[];
extern char gArmorHeadNames_70[];
extern char gArmorHeadNames_71[];
extern char gArmorHeadNames_72[];
extern char gArmorHeadNames_73[];
extern char gArmorHeadNames_74[];
extern char gArmorHeadNames_75[];
extern char gArmorHeadNames_76[];
extern char gArmorHeadNames_77[];
extern char gArmorHeadNames_78[];
extern char gArmorHeadNames_79[];
extern char gArmorHeadNames_80[];
extern char gArmorHeadNames_81[];
extern char gArmorHeadNames_82[];
extern char gArmorHeadNames_83[];
extern char gArmorHeadNames_84[];
extern char gArmorHeadNames_85[];
extern char gArmorHeadNames_86[];
extern char gArmorHeadNames_87[];
extern char gArmorHeadNames_88[];
extern char gArmorHeadNames_89[];
extern char gArmorHeadNames_90[];
extern char gArmorHeadNames_91[];
extern char gArmorHeadNames_92[];
extern char gArmorHeadNames_93[];
extern char gArmorHeadNames_94[];
extern char gArmorHeadNames_95[];
extern char gArmorHeadNames_96[];
extern char gArmorHeadNames_97[];
extern char gArmorHeadNames_98[];
extern char gArmorHeadNames_99[];
extern char gArmorHeadNames_100[];
extern char gArmorHeadNames_101[];
extern char gArmorHeadNames_102[];
extern char gArmorHeadNames_103[];
extern char gArmorHeadNames_104[];
extern char gArmorHeadNames_105[];
extern char gArmorHeadNames_106[];
extern char gArmorHeadNames_107[];
extern char gArmorHeadNames_108[];
extern char gArmorHeadNames_109[];
extern char gArmorHeadNames_110[];
extern char gArmorHeadNames_111[];
extern char gArmorHeadNames_112[];
extern char gArmorHeadNames_113[];
extern char gArmorHeadNames_114[];
extern char gArmorHeadNames_115[];
extern char gArmorHeadNames_116[];
extern char gArmorHeadNames_117[];
extern char gArmorHeadNames_118[];
extern char gArmorHeadNames_119[];
extern char gArmorHeadNames_120[];
extern char gArmorHeadNames_121[];
extern char gArmorHeadNames_122[];
extern char gArmorHeadNames_123[];
extern char gArmorHeadNames_124[];
extern char gArmorHeadNames_125[];
extern char gArmorHeadNames_126[];
extern char gArmorHeadNames_127[];
extern char gArmorHeadNames_128[];
extern char gArmorHeadNames_129[];
extern char gArmorHeadNames_130[];
extern char gArmorHeadNames_131[];
extern char gArmorHeadNames_132[];
extern char gArmorHeadNames_133[];
extern char gArmorHeadNames_134[];
extern char gArmorHeadNames_135[];
extern char gArmorHeadNames_136[];
extern char gArmorHeadNames_137[];
extern char gArmorHeadNames_138[];
extern char gArmorHeadNames_139[];
extern char gArmorHeadNames_140[];
extern char gArmorHeadNames_141[];
extern char gArmorHeadNames_142[];
extern char gArmorHeadNames_143[];
extern char gArmorHeadNames_144[];
extern char gArmorHeadNames_145[];
extern char gArmorHeadNames_146[];
extern char gArmorHeadNames_147[];
extern char gArmorHeadNames_148[];
extern char gArmorHeadNames_149[];
extern char gArmorHeadNames_150[];
extern char gArmorHeadNames_151[];
extern char gArmorHeadNames_152[];
extern char gArmorHeadNames_153[];
extern char gArmorHeadNames_154[];
extern char gArmorHeadNames_155[];
extern char gArmorHeadNames_156[];
extern char gArmorHeadNames_157[];
extern char gArmorHeadNames_158[];
extern char gArmorHeadNames_159[];
extern char gArmorHeadNames_160[];
extern char gArmorHeadNames_161[];
extern char gArmorHeadNames_162[];
extern char gArmorHeadNames_163[];
extern char gArmorHeadNames_164[];
extern char gArmorHeadNames_165[];
extern char gArmorHeadNames_166[];
extern char gArmorHeadNames_167[];
extern char gArmorHeadNames_168[];
extern char gArmorHeadNames_169[];
extern char gArmorHeadNames_170[];
extern char gArmorHeadNames_171[];
extern char gArmorHeadNames_172[];
extern char gArmorHeadNames_173[];
extern char gArmorHeadNames_174[];
extern char gArmorHeadNames_175[];
extern char gArmorHeadNames_176[];
extern char gArmorHeadNames_177[];
extern char gArmorHeadNames_178[];
extern char gArmorHeadNames_179[];
extern char gArmorHeadNames_180[];
extern char gArmorHeadNames_181[];
extern char gArmorHeadNames_182[];
extern char gArmorHeadNames_183[];
extern char gArmorHeadNames_184[];
extern char gArmorHeadNames_185[];
extern char gArmorHeadNames_186[];
extern char gArmorHeadNames_187[];
extern char gArmorHeadNames_188[];
extern char gArmorHeadNames_189[];
extern char gArmorHeadNames_190[];
extern char gArmorHeadNames_191[];
extern char gArmorHeadNames_192[];
extern char gArmorHeadNames_193[];
extern char gArmorHeadNames_194[];
extern char gArmorHeadNames_195[];
extern char gArmorHeadNames_196[];
extern char gArmorHeadNames_197[];
extern char gArmorHeadNames_198[];
extern char gArmorHeadNames_199[];
extern char gArmorHeadNames_200[];
extern char gArmorHeadNames_201[];
extern char gArmorHeadNames_202[];
extern char gArmorHeadNames_203[];
extern char gArmorHeadNames_204[];
extern char gArmorHeadNames_205[];
extern char gArmorHeadNames_206[];
extern char gArmorHeadNames_207[];
extern char gArmorHeadNames_208[];
extern char gArmorHeadNames_209[];
extern char gArmorHeadNames_210[];
extern char gArmorHeadNames_211[];
extern char gArmorHeadNames_212[];
extern char gArmorHeadNames_213[];
extern char gArmorHeadNames_214[];
extern char gArmorHeadNames_215[];
extern char gArmorHeadNames_216[];
extern char gArmorHeadNames_217[];
extern char gArmorHeadNames_218[];
extern char gArmorHeadNames_219[];
extern char gArmorHeadNames_220[];
extern char gArmorHeadNames_221[];
extern char gArmorHeadNames_222[];
extern char gArmorHeadNames_223[];
extern char gArmorHeadNames_224[];
extern char gArmorHeadNames_225[];
extern char gArmorHeadNames_226[];
extern char gArmorHeadNames_227[];
extern char gArmorHeadNames_228[];
extern char gArmorHeadNames_229[];
extern char gArmorHeadNames_230[];
extern char gArmorHeadNames_231[];
extern char gArmorHeadNames_232[];
extern char gArmorHeadNames_233[];
extern char gArmorHeadNames_234[];
extern char gArmorHeadNames_235[];
extern char gArmorHeadNames_236[];
extern char gArmorHeadNames_237[];
extern char gArmorHeadNames_238[];
extern char gArmorHeadNames_239[];
extern char gArmorHeadNames_240[];
extern char gArmorHeadNames_241[];
extern char gArmorHeadNames_242[];
extern char gArmorHeadNames_243[];
extern char gArmorHeadNames_244[];
extern char gArmorHeadNames_245[];
extern char gArmorHeadNames_246[];
extern char gArmorHeadNames_247[];
extern char gArmorHeadNames_248[];
extern char gArmorHeadNames_249[];
extern char gArmorHeadNames_250[];
extern char gArmorHeadNames_251[];
extern char gArmorHeadNames_252[];
extern char gArmorBodyNames_1[];
extern char gArmorBodyNames_2[];
extern char gArmorBodyNames_3[];
extern char gArmorBodyNames_4[];
extern char gArmorBodyNames_5[];
extern char gArmorBodyNames_6[];
extern char gArmorBodyNames_7[];
extern char gArmorBodyNames_8[];
extern char gArmorBodyNames_9[];
extern char gArmorBodyNames_10[];
extern char gArmorBodyNames_11[];
extern char gArmorBodyNames_12[];
extern char gArmorBodyNames_13[];
extern char gArmorBodyNames_14[];
extern char gArmorBodyNames_15[];
extern char gArmorBodyNames_16[];
extern char gArmorBodyNames_17[];
extern char gArmorBodyNames_18[];
extern char gArmorBodyNames_19[];
extern char gArmorBodyNames_20[];
extern char gArmorBodyNames_21[];
extern char gArmorBodyNames_22[];
extern char gArmorBodyNames_23[];
extern char gArmorBodyNames_24[];
extern char gArmorBodyNames_25[];
extern char gArmorBodyNames_26[];
extern char gArmorBodyNames_27[];
extern char gArmorBodyNames_28[];
extern char gArmorBodyNames_29[];
extern char gArmorBodyNames_30[];
extern char gArmorBodyNames_31[];
extern char gArmorBodyNames_32[];
extern char gArmorBodyNames_33[];
extern char gArmorBodyNames_34[];
extern char gArmorBodyNames_35[];
extern char gArmorBodyNames_36[];
extern char gArmorBodyNames_37[];
extern char gArmorBodyNames_38[];
extern char gArmorBodyNames_39[];
extern char gArmorBodyNames_40[];
extern char gArmorBodyNames_41[];
extern char gArmorBodyNames_42[];
extern char gArmorBodyNames_43[];
extern char gArmorBodyNames_44[];
extern char gArmorBodyNames_45[];
extern char gArmorBodyNames_46[];
extern char gArmorBodyNames_47[];
extern char gArmorBodyNames_48[];
extern char gArmorBodyNames_49[];
extern char gArmorBodyNames_50[];
extern char gArmorBodyNames_51[];
extern char gArmorBodyNames_52[];
extern char gArmorBodyNames_53[];
extern char gArmorBodyNames_54[];
extern char gArmorBodyNames_55[];
extern char gArmorBodyNames_56[];
extern char gArmorBodyNames_57[];
extern char gArmorBodyNames_58[];
extern char gArmorBodyNames_59[];
extern char gArmorBodyNames_60[];
extern char gArmorBodyNames_61[];
extern char gArmorBodyNames_62[];
extern char gArmorBodyNames_63[];
extern char gArmorBodyNames_64[];
extern char gArmorBodyNames_65[];
extern char gArmorBodyNames_66[];
extern char gArmorBodyNames_67[];
extern char gArmorBodyNames_68[];
extern char gArmorBodyNames_69[];
extern char gArmorBodyNames_70[];
extern char gArmorBodyNames_71[];
extern char gArmorBodyNames_72[];
extern char gArmorBodyNames_73[];
extern char gArmorBodyNames_74[];
extern char gArmorBodyNames_75[];
extern char gArmorBodyNames_76[];
extern char gArmorBodyNames_77[];
extern char gArmorBodyNames_78[];
extern char gArmorBodyNames_79[];
extern char gArmorBodyNames_80[];
extern char gArmorBodyNames_81[];
extern char gArmorBodyNames_82[];
extern char gArmorBodyNames_83[];
extern char gArmorBodyNames_84[];
extern char gArmorBodyNames_85[];
extern char gArmorBodyNames_86[];
extern char gArmorBodyNames_87[];
extern char gArmorBodyNames_88[];
extern char gArmorBodyNames_89[];
extern char gArmorBodyNames_90[];
extern char gArmorBodyNames_91[];
extern char gArmorBodyNames_92[];
extern char gArmorBodyNames_93[];
extern char gArmorBodyNames_94[];
extern char gArmorBodyNames_95[];
extern char gArmorBodyNames_96[];
extern char gArmorBodyNames_97[];
extern char gArmorBodyNames_98[];
extern char gArmorBodyNames_99[];
extern char gArmorBodyNames_100[];
extern char gArmorBodyNames_101[];
extern char gArmorBodyNames_102[];
extern char gArmorBodyNames_103[];
extern char gArmorBodyNames_104[];
extern char gArmorBodyNames_105[];
extern char gArmorBodyNames_106[];
extern char gArmorBodyNames_107[];
extern char gArmorBodyNames_108[];
extern char gArmorBodyNames_109[];
extern char gArmorBodyNames_110[];
extern char gArmorBodyNames_111[];
extern char gArmorBodyNames_112[];
extern char gArmorBodyNames_113[];
extern char gArmorBodyNames_114[];
extern char gArmorBodyNames_115[];
extern char gArmorBodyNames_116[];
extern char gArmorBodyNames_117[];
extern char gArmorBodyNames_118[];
extern char gArmorBodyNames_119[];
extern char gArmorBodyNames_120[];
extern char gArmorBodyNames_121[];
extern char gArmorBodyNames_122[];
extern char gArmorBodyNames_123[];
extern char gArmorBodyNames_124[];
extern char gArmorBodyNames_125[];
extern char gArmorBodyNames_126[];
extern char gArmorBodyNames_127[];
extern char gArmorBodyNames_128[];
extern char gArmorBodyNames_129[];
extern char gArmorBodyNames_130[];
extern char gArmorBodyNames_131[];
extern char gArmorBodyNames_132[];
extern char gArmorBodyNames_133[];
extern char gArmorBodyNames_134[];
extern char gArmorBodyNames_135[];
extern char gArmorBodyNames_136[];
extern char gArmorBodyNames_137[];
extern char gArmorBodyNames_138[];
extern char gArmorBodyNames_139[];
extern char gArmorBodyNames_140[];
extern char gArmorBodyNames_141[];
extern char gArmorBodyNames_142[];
extern char gArmorBodyNames_143[];
extern char gArmorBodyNames_144[];
extern char gArmorBodyNames_145[];
extern char gArmorBodyNames_146[];
extern char gArmorBodyNames_147[];
extern char gArmorBodyNames_148[];
extern char gArmorBodyNames_149[];
extern char gArmorBodyNames_150[];
extern char gArmorBodyNames_151[];
extern char gArmorBodyNames_152[];
extern char gArmorBodyNames_153[];
extern char gArmorBodyNames_154[];
extern char gArmorBodyNames_155[];
extern char gArmorBodyNames_156[];
extern char gArmorBodyNames_157[];
extern char gArmorBodyNames_158[];
extern char gArmorBodyNames_159[];
extern char gArmorBodyNames_160[];
extern char gArmorBodyNames_161[];
extern char gArmorBodyNames_162[];
extern char gArmorBodyNames_163[];
extern char gArmorBodyNames_164[];
extern char gArmorBodyNames_165[];
extern char gArmorBodyNames_166[];
extern char gArmorBodyNames_167[];
extern char gArmorBodyNames_168[];
extern char gArmorBodyNames_169[];
extern char gArmorBodyNames_170[];
extern char gArmorBodyNames_171[];
extern char D_004A6220[];
extern char D_004A6240[];
extern char gArmorBodyNames_176[];
extern char gArmorBodyNames_177[];
extern char gArmorBodyNames_178[];
extern char gArmorBodyNames_179[];
extern char gArmorBodyNames_180[];
extern char gArmorBodyNames_181[];
extern char gArmorBodyNames_182[];
extern char gArmorBodyNames_183[];
extern char gArmorBodyNames_184[];
extern char gArmorBodyNames_185[];
extern char gArmorBodyNames_186[];
extern char gArmorBodyNames_187[];
extern char gArmorBodyNames_188[];
extern char gArmorBodyNames_189[];
extern char gArmorBodyNames_190[];
extern char gArmorBodyNames_191[];
extern char gArmorBodyNames_192[];
extern char gArmorBodyNames_193[];
extern char gArmorBodyNames_194[];
extern char gArmorBodyNames_195[];
extern char gArmorBodyNames_196[];
extern char gArmorBodyNames_197[];
extern char gArmorBodyNames_198[];
extern char gArmorBodyNames_199[];
extern char gArmorBodyNames_200[];
extern char gArmorBodyNames_201[];
extern char gArmorBodyNames_202[];
extern char gArmorBodyNames_203[];
extern char gArmorBodyNames_204[];
extern char gArmorBodyNames_205[];
extern char gArmorBodyNames_206[];
extern char gArmorBodyNames_207[];
extern char gArmorBodyNames_208[];
extern char gArmorBodyNames_209[];
extern char gArmorBodyNames_210[];
extern char gArmorBodyNames_211[];
extern char gArmorBodyNames_212[];
extern char gArmorBodyNames_213[];
extern char gArmorBodyNames_214[];
extern char gArmorBodyNames_215[];
extern char gArmorBodyNames_216[];
extern char gArmorBodyNames_217[];
extern char gArmorBodyNames_218[];
extern char gArmorBodyNames_219[];
extern char gArmorBodyNames_220[];
extern char gArmorBodyNames_221[];
extern char gArmorBodyNames_222[];
extern char gArmorBodyNames_223[];
extern char gArmorBodyNames_224[];
extern char gArmorBodyNames_225[];
extern char gArmorBodyNames_226[];
extern char gArmorBodyNames_227[];
extern char gArmorBodyNames_228[];
extern char gArmorBodyNames_229[];
extern char gArmorBodyNames_230[];
extern char gArmorBodyNames_231[];
extern char gArmorBodyNames_232[];
extern char gArmorBodyNames_233[];
extern char gArmorBodyNames_234[];
extern char gArmorBodyNames_235[];
extern char D_004A6830[];
extern char D_004A6850[];
extern char gArmorBodyNames_240[];
extern char gArmorBodyNames_241[];
extern char gArmorBodyNames_242[];
extern char gArmorArmsNames_1[];
extern char gArmorArmsNames_2[];
extern char gArmorArmsNames_3[];
extern char gArmorArmsNames_4[];
extern char gArmorArmsNames_5[];
extern char gArmorArmsNames_6[];
extern char gArmorArmsNames_7[];
extern char gArmorArmsNames_8[];
extern char gArmorArmsNames_9[];
extern char gArmorArmsNames_10[];
extern char gArmorArmsNames_11[];
extern char gArmorArmsNames_12[];
extern char gArmorArmsNames_13[];
extern char gArmorArmsNames_14[];
extern char gArmorArmsNames_15[];
extern char gArmorArmsNames_16[];
extern char gArmorArmsNames_17[];
extern char gArmorArmsNames_18[];
extern char gArmorArmsNames_19[];
extern char gArmorArmsNames_20[];
extern char gArmorArmsNames_21[];
extern char gArmorArmsNames_22[];
extern char gArmorArmsNames_23[];
extern char gArmorArmsNames_24[];
extern char gArmorArmsNames_25[];
extern char gArmorArmsNames_26[];
extern char gArmorArmsNames_27[];
extern char gArmorArmsNames_28[];
extern char gArmorArmsNames_29[];
extern char gArmorArmsNames_30[];
extern char gArmorArmsNames_31[];
extern char gArmorArmsNames_32[];
extern char gArmorArmsNames_33[];
extern char gArmorArmsNames_34[];
extern char gArmorArmsNames_35[];
extern char gArmorArmsNames_36[];
extern char gArmorArmsNames_37[];
extern char gArmorArmsNames_38[];
extern char gArmorArmsNames_39[];
extern char gArmorArmsNames_40[];
extern char gArmorArmsNames_41[];
extern char gArmorArmsNames_42[];
extern char gArmorArmsNames_43[];
extern char gArmorArmsNames_44[];
extern char gArmorArmsNames_45[];
extern char gArmorArmsNames_46[];
extern char gArmorArmsNames_47[];
extern char gArmorArmsNames_48[];
extern char gArmorArmsNames_49[];
extern char gArmorArmsNames_50[];
extern char gArmorArmsNames_51[];
extern char gArmorArmsNames_52[];
extern char gArmorArmsNames_53[];
extern char gArmorArmsNames_54[];
extern char gArmorArmsNames_55[];
extern char gArmorArmsNames_56[];
extern char gArmorArmsNames_57[];
extern char gArmorArmsNames_58[];
extern char gArmorArmsNames_59[];
extern char gArmorArmsNames_60[];
extern char gArmorArmsNames_61[];
extern char gArmorArmsNames_62[];
extern char gArmorArmsNames_63[];
extern char gArmorArmsNames_64[];
extern char gArmorArmsNames_65[];
extern char gArmorArmsNames_66[];
extern char gArmorArmsNames_67[];
extern char gArmorArmsNames_68[];
extern char gArmorArmsNames_69[];
extern char gArmorArmsNames_70[];
extern char gArmorArmsNames_71[];
extern char gArmorArmsNames_72[];
extern char gArmorArmsNames_73[];
extern char gArmorArmsNames_74[];
extern char gArmorArmsNames_75[];
extern char gArmorArmsNames_76[];
extern char gArmorArmsNames_77[];
extern char gArmorArmsNames_78[];
extern char gArmorArmsNames_79[];
extern char gArmorArmsNames_80[];
extern char gArmorArmsNames_81[];
extern char gArmorArmsNames_82[];
extern char gArmorArmsNames_83[];
extern char gArmorArmsNames_84[];
extern char gArmorArmsNames_85[];
extern char gArmorArmsNames_86[];
extern char gArmorArmsNames_87[];
extern char gArmorArmsNames_88[];
extern char gArmorArmsNames_89[];
extern char gArmorArmsNames_90[];
extern char gArmorArmsNames_91[];
extern char gArmorArmsNames_92[];
extern char gArmorArmsNames_93[];
extern char gArmorArmsNames_94[];
extern char gArmorArmsNames_95[];
extern char gArmorArmsNames_96[];
extern char gArmorArmsNames_97[];
extern char gArmorArmsNames_98[];
extern char gArmorArmsNames_99[];
extern char gArmorArmsNames_100[];
extern char gArmorArmsNames_101[];
extern char gArmorArmsNames_102[];
extern char gArmorArmsNames_103[];
extern char gArmorArmsNames_104[];
extern char gArmorArmsNames_105[];
extern char gArmorArmsNames_106[];
extern char gArmorArmsNames_107[];
extern char gArmorArmsNames_108[];
extern char gArmorArmsNames_109[];
extern char gArmorArmsNames_110[];
extern char gArmorArmsNames_111[];
extern char gArmorArmsNames_112[];
extern char gArmorArmsNames_113[];
extern char gArmorArmsNames_114[];
extern char gArmorArmsNames_115[];
extern char gArmorArmsNames_116[];
extern char gArmorArmsNames_117[];
extern char gArmorArmsNames_118[];
extern char gArmorArmsNames_119[];
extern char gArmorArmsNames_120[];
extern char gArmorArmsNames_121[];
extern char gArmorArmsNames_122[];
extern char gArmorArmsNames_123[];
extern char gArmorArmsNames_124[];
extern char gArmorArmsNames_125[];
extern char gArmorArmsNames_126[];
extern char gArmorArmsNames_127[];
extern char gArmorArmsNames_128[];
extern char gArmorArmsNames_129[];
extern char gArmorArmsNames_130[];
extern char gArmorArmsNames_131[];
extern char gArmorArmsNames_132[];
extern char gArmorArmsNames_133[];
extern char gArmorArmsNames_134[];
extern char gArmorArmsNames_135[];
extern char gArmorArmsNames_136[];
extern char gArmorArmsNames_137[];
extern char gArmorArmsNames_138[];
extern char gArmorArmsNames_139[];
extern char gArmorArmsNames_140[];
extern char gArmorArmsNames_141[];
extern char gArmorArmsNames_142[];
extern char gArmorArmsNames_143[];
extern char gArmorArmsNames_144[];
extern char gArmorArmsNames_145[];
extern char gArmorArmsNames_146[];
extern char gArmorArmsNames_147[];
extern char gArmorArmsNames_148[];
extern char gArmorArmsNames_149[];
extern char gArmorArmsNames_150[];
extern char gArmorArmsNames_151[];
extern char gArmorArmsNames_152[];
extern char gArmorArmsNames_153[];
extern char gArmorArmsNames_154[];
extern char gArmorArmsNames_155[];
extern char gArmorArmsNames_156[];
extern char gArmorArmsNames_157[];
extern char gArmorArmsNames_158[];
extern char gArmorArmsNames_159[];
extern char gArmorArmsNames_160[];
extern char gArmorArmsNames_161[];
extern char gArmorArmsNames_162[];
extern char gArmorArmsNames_163[];
extern char gArmorArmsNames_164[];
extern char gArmorArmsNames_165[];
extern char gArmorArmsNames_166[];
extern char gArmorArmsNames_167[];
extern char gArmorArmsNames_168[];
extern char gArmorArmsNames_169[];
extern char D_004A7850[];
extern char D_004A7870[];
extern char gArmorArmsNames_174[];
extern char gArmorArmsNames_175[];
extern char gArmorArmsNames_176[];
extern char gArmorArmsNames_177[];
extern char gArmorArmsNames_178[];
extern char gArmorArmsNames_179[];
extern char gArmorArmsNames_180[];
extern char gArmorArmsNames_181[];
extern char gArmorArmsNames_182[];
extern char gArmorArmsNames_183[];
extern char gArmorArmsNames_184[];
extern char gArmorArmsNames_185[];
extern char gArmorArmsNames_186[];
extern char gArmorArmsNames_187[];
extern char gArmorArmsNames_188[];
extern char gArmorArmsNames_189[];
extern char gArmorArmsNames_190[];
extern char gArmorArmsNames_191[];
extern char gArmorArmsNames_192[];
extern char gArmorArmsNames_193[];
extern char gArmorArmsNames_194[];
extern char gArmorArmsNames_195[];
extern char gArmorArmsNames_196[];
extern char gArmorArmsNames_197[];
extern char gArmorArmsNames_198[];
extern char gArmorArmsNames_199[];
extern char gArmorArmsNames_200[];
extern char gArmorArmsNames_201[];
extern char gArmorArmsNames_202[];
extern char gArmorArmsNames_203[];
extern char gArmorArmsNames_204[];
extern char gArmorArmsNames_205[];
extern char gArmorArmsNames_206[];
extern char gArmorArmsNames_207[];
extern char gArmorArmsNames_208[];
extern char gArmorArmsNames_209[];
extern char gArmorArmsNames_210[];
extern char gArmorArmsNames_211[];
extern char gArmorArmsNames_212[];
extern char gArmorArmsNames_213[];
extern char gArmorArmsNames_214[];
extern char gArmorArmsNames_215[];
extern char gArmorArmsNames_216[];
extern char gArmorArmsNames_217[];
extern char gArmorArmsNames_218[];
extern char gArmorArmsNames_219[];
extern char gArmorArmsNames_220[];
extern char gArmorArmsNames_221[];
extern char gArmorArmsNames_222[];
extern char gArmorArmsNames_223[];
extern char gArmorArmsNames_224[];
extern char gArmorArmsNames_225[];
extern char gArmorArmsNames_226[];
extern char gArmorArmsNames_227[];
extern char gArmorArmsNames_228[];
extern char gArmorArmsNames_229[];
extern char gArmorArmsNames_230[];
extern char D_004A7DA0[];
extern char D_004A7DC0[];
extern char gArmorArmsNames_235[];
extern char gArmorArmsNames_236[];
extern char gArmorArmsNames_237[];
extern char gArmorWaistNames_1[];
extern char gArmorWaistNames_2[];
extern char gArmorWaistNames_3[];
extern char gArmorWaistNames_4[];
extern char gArmorWaistNames_5[];
extern char gArmorWaistNames_6[];
extern char gArmorWaistNames_7[];
extern char gArmorWaistNames_8[];
extern char gArmorWaistNames_9[];
extern char gArmorWaistNames_10[];
extern char gArmorWaistNames_11[];
extern char gArmorWaistNames_12[];
extern char gArmorWaistNames_13[];
extern char gArmorWaistNames_14[];
extern char gArmorWaistNames_15[];
extern char gArmorWaistNames_16[];
extern char gArmorWaistNames_17[];
extern char gArmorWaistNames_18[];
extern char gArmorWaistNames_19[];
extern char gArmorWaistNames_20[];
extern char gArmorWaistNames_21[];
extern char gArmorWaistNames_22[];
extern char gArmorWaistNames_23[];
extern char gArmorWaistNames_24[];
extern char gArmorWaistNames_25[];
extern char gArmorWaistNames_26[];
extern char gArmorWaistNames_27[];
extern char gArmorWaistNames_28[];
extern char gArmorWaistNames_29[];
extern char gArmorWaistNames_30[];
extern char gArmorWaistNames_31[];
extern char gArmorWaistNames_32[];
extern char gArmorWaistNames_33[];
extern char gArmorWaistNames_34[];
extern char gArmorWaistNames_35[];
extern char gArmorWaistNames_36[];
extern char gArmorWaistNames_37[];
extern char gArmorWaistNames_38[];
extern char gArmorWaistNames_39[];
extern char gArmorWaistNames_40[];
extern char gArmorWaistNames_41[];
extern char gArmorWaistNames_42[];
extern char gArmorWaistNames_43[];
extern char gArmorWaistNames_44[];
extern char gArmorWaistNames_45[];
extern char gArmorWaistNames_46[];
extern char gArmorWaistNames_47[];
extern char gArmorWaistNames_48[];
extern char gArmorWaistNames_49[];
extern char gArmorWaistNames_50[];
extern char gArmorWaistNames_51[];
extern char gArmorWaistNames_52[];
extern char gArmorWaistNames_53[];
extern char gArmorWaistNames_54[];
extern char gArmorWaistNames_55[];
extern char gArmorWaistNames_56[];
extern char gArmorWaistNames_57[];
extern char gArmorWaistNames_58[];
extern char gArmorWaistNames_59[];
extern char gArmorWaistNames_60[];
extern char gArmorWaistNames_61[];
extern char gArmorWaistNames_62[];
extern char gArmorWaistNames_63[];
extern char gArmorWaistNames_64[];
extern char gArmorWaistNames_65[];
extern char gArmorWaistNames_66[];
extern char gArmorWaistNames_67[];
extern char gArmorWaistNames_68[];
extern char gArmorWaistNames_69[];
extern char gArmorWaistNames_70[];
extern char gArmorWaistNames_71[];
extern char gArmorWaistNames_72[];
extern char gArmorWaistNames_73[];
extern char gArmorWaistNames_74[];
extern char gArmorWaistNames_75[];
extern char gArmorWaistNames_76[];
extern char gArmorWaistNames_77[];
extern char gArmorWaistNames_78[];
extern char gArmorWaistNames_79[];
extern char gArmorWaistNames_80[];
extern char gArmorWaistNames_81[];
extern char gArmorWaistNames_82[];
extern char gArmorWaistNames_83[];
extern char gArmorWaistNames_84[];
extern char gArmorWaistNames_85[];
extern char gArmorWaistNames_86[];
extern char gArmorWaistNames_87[];
extern char gArmorWaistNames_88[];
extern char gArmorWaistNames_89[];
extern char gArmorWaistNames_90[];
extern char gArmorWaistNames_91[];
extern char gArmorWaistNames_92[];
extern char gArmorWaistNames_93[];
extern char gArmorWaistNames_94[];
extern char gArmorWaistNames_95[];
extern char gArmorWaistNames_96[];
extern char gArmorWaistNames_97[];
extern char gArmorWaistNames_98[];
extern char gArmorWaistNames_99[];
extern char gArmorWaistNames_100[];
extern char gArmorWaistNames_101[];
extern char gArmorWaistNames_102[];
extern char gArmorWaistNames_103[];
extern char gArmorWaistNames_104[];
extern char gArmorWaistNames_105[];
extern char gArmorWaistNames_106[];
extern char gArmorWaistNames_107[];
extern char gArmorWaistNames_108[];
extern char gArmorWaistNames_109[];
extern char gArmorWaistNames_110[];
extern char gArmorWaistNames_111[];
extern char gArmorWaistNames_112[];
extern char gArmorWaistNames_113[];
extern char gArmorWaistNames_114[];
extern char gArmorWaistNames_115[];
extern char gArmorWaistNames_116[];
extern char gArmorWaistNames_117[];
extern char gArmorWaistNames_118[];
extern char gArmorWaistNames_119[];
extern char gArmorWaistNames_120[];
extern char gArmorWaistNames_121[];
extern char gArmorWaistNames_122[];
extern char gArmorWaistNames_123[];
extern char gArmorWaistNames_124[];
extern char gArmorWaistNames_125[];
extern char gArmorWaistNames_126[];
extern char gArmorWaistNames_127[];
extern char gArmorWaistNames_128[];
extern char gArmorWaistNames_129[];
extern char gArmorWaistNames_130[];
extern char gArmorWaistNames_131[];
extern char gArmorWaistNames_132[];
extern char gArmorWaistNames_133[];
extern char gArmorWaistNames_134[];
extern char gArmorWaistNames_135[];
extern char gArmorWaistNames_136[];
extern char gArmorWaistNames_137[];
extern char gArmorWaistNames_138[];
extern char gArmorWaistNames_139[];
extern char gArmorWaistNames_140[];
extern char gArmorWaistNames_141[];
extern char gArmorWaistNames_142[];
extern char gArmorWaistNames_143[];
extern char gArmorWaistNames_144[];
extern char gArmorWaistNames_145[];
extern char gArmorWaistNames_146[];
extern char gArmorWaistNames_147[];
extern char gArmorWaistNames_148[];
extern char gArmorWaistNames_149[];
extern char gArmorWaistNames_150[];
extern char gArmorWaistNames_151[];
extern char gArmorWaistNames_152[];
extern char gArmorWaistNames_153[];
extern char gArmorWaistNames_154[];
extern char gArmorWaistNames_155[];
extern char gArmorWaistNames_156[];
extern char gArmorWaistNames_157[];
extern char gArmorWaistNames_158[];
extern char gArmorWaistNames_159[];
extern char gArmorWaistNames_160[];
extern char gArmorWaistNames_161[];
extern char gArmorWaistNames_162[];
extern char gArmorWaistNames_163[];
extern char gArmorWaistNames_164[];
extern char gArmorWaistNames_165[];
extern char D_004A8F90[];
extern char D_004A8FB0[];
extern char gArmorWaistNames_170[];
extern char gArmorWaistNames_171[];
extern char gArmorWaistNames_172[];
extern char gArmorWaistNames_173[];
extern char gArmorWaistNames_174[];
extern char gArmorWaistNames_175[];
extern char gArmorWaistNames_176[];
extern char gArmorWaistNames_177[];
extern char gArmorWaistNames_178[];
extern char gArmorWaistNames_179[];
extern char gArmorWaistNames_180[];
extern char gArmorWaistNames_181[];
extern char gArmorWaistNames_182[];
extern char gArmorWaistNames_183[];
extern char gArmorWaistNames_184[];
extern char gArmorWaistNames_185[];
extern char gArmorWaistNames_186[];
extern char gArmorWaistNames_187[];
extern char gArmorWaistNames_188[];
extern char gArmorWaistNames_189[];
extern char gArmorWaistNames_190[];
extern char gArmorWaistNames_191[];
extern char gArmorWaistNames_192[];
extern char gArmorWaistNames_193[];
extern char gArmorWaistNames_194[];
extern char gArmorWaistNames_195[];
extern char gArmorWaistNames_196[];
extern char gArmorWaistNames_197[];
extern char gArmorWaistNames_198[];
extern char gArmorWaistNames_199[];
extern char gArmorWaistNames_200[];
extern char gArmorWaistNames_201[];
extern char gArmorWaistNames_202[];
extern char gArmorWaistNames_203[];
extern char gArmorWaistNames_204[];
extern char gArmorWaistNames_205[];
extern char gArmorWaistNames_206[];
extern char gArmorWaistNames_207[];
extern char gArmorWaistNames_208[];
extern char gArmorWaistNames_209[];
extern char gArmorWaistNames_210[];
extern char gArmorWaistNames_211[];
extern char gArmorWaistNames_212[];
extern char gArmorWaistNames_213[];
extern char gArmorWaistNames_214[];
extern char gArmorWaistNames_215[];
extern char gArmorWaistNames_216[];
extern char gArmorWaistNames_217[];
extern char gArmorWaistNames_218[];
extern char gArmorWaistNames_219[];
extern char gArmorWaistNames_220[];
extern char gArmorWaistNames_221[];
extern char gArmorWaistNames_222[];
extern char gArmorWaistNames_223[];
extern char gArmorWaistNames_224[];
extern char gArmorWaistNames_225[];
extern char gArmorWaistNames_226[];
extern char gArmorWaistNames_227[];
extern char D_004A9580[];
extern char D_004A95A0[];
extern char gArmorWaistNames_232[];
extern char gArmorWaistNames_233[];
extern char gArmorWaistNames_234[];
extern char gArmorWaistNames_235[];
extern char gArmorWaistNames_236[];
extern char gArmorLegsNames_1[];
extern char gArmorLegsNames_2[];
extern char gArmorLegsNames_3[];
extern char gArmorLegsNames_4[];
extern char gArmorLegsNames_5[];
extern char gArmorLegsNames_6[];
extern char gArmorLegsNames_7[];
extern char gArmorLegsNames_8[];
extern char gArmorLegsNames_9[];
extern char gArmorLegsNames_10[];
extern char gArmorLegsNames_11[];
extern char gArmorLegsNames_12[];
extern char gArmorLegsNames_13[];
extern char gArmorLegsNames_14[];
extern char gArmorLegsNames_15[];
extern char gArmorLegsNames_16[];
extern char gArmorLegsNames_17[];
extern char gArmorLegsNames_18[];
extern char gArmorLegsNames_19[];
extern char gArmorLegsNames_20[];
extern char gArmorLegsNames_21[];
extern char gArmorLegsNames_22[];
extern char gArmorLegsNames_23[];
extern char gArmorLegsNames_24[];
extern char gArmorLegsNames_25[];
extern char gArmorLegsNames_26[];
extern char gArmorLegsNames_27[];
extern char gArmorLegsNames_28[];
extern char gArmorLegsNames_29[];
extern char gArmorLegsNames_30[];
extern char gArmorLegsNames_31[];
extern char gArmorLegsNames_32[];
extern char gArmorLegsNames_33[];
extern char gArmorLegsNames_34[];
extern char gArmorLegsNames_35[];
extern char gArmorLegsNames_36[];
extern char gArmorLegsNames_37[];
extern char gArmorLegsNames_38[];
extern char gArmorLegsNames_39[];
extern char gArmorLegsNames_40[];
extern char gArmorLegsNames_41[];
extern char gArmorLegsNames_42[];
extern char gArmorLegsNames_43[];
extern char gArmorLegsNames_44[];
extern char gArmorLegsNames_45[];
extern char gArmorLegsNames_46[];
extern char gArmorLegsNames_47[];
extern char gArmorLegsNames_48[];
extern char gArmorLegsNames_49[];
extern char gArmorLegsNames_50[];
extern char gArmorLegsNames_51[];
extern char gArmorLegsNames_52[];
extern char gArmorLegsNames_53[];
extern char gArmorLegsNames_54[];
extern char gArmorLegsNames_55[];
extern char gArmorLegsNames_56[];
extern char gArmorLegsNames_57[];
extern char gArmorLegsNames_58[];
extern char gArmorLegsNames_59[];
extern char gArmorLegsNames_60[];
extern char gArmorLegsNames_61[];
extern char gArmorLegsNames_62[];
extern char gArmorLegsNames_63[];
extern char gArmorLegsNames_64[];
extern char gArmorLegsNames_65[];
extern char gArmorLegsNames_66[];
extern char gArmorLegsNames_67[];
extern char gArmorLegsNames_68[];
extern char gArmorLegsNames_69[];
extern char gArmorLegsNames_70[];
extern char gArmorLegsNames_71[];
extern char gArmorLegsNames_72[];
extern char gArmorLegsNames_73[];
extern char gArmorLegsNames_74[];
extern char gArmorLegsNames_75[];
extern char gArmorLegsNames_76[];
extern char gArmorLegsNames_77[];
extern char gArmorLegsNames_78[];
extern char gArmorLegsNames_79[];
extern char gArmorLegsNames_80[];
extern char gArmorLegsNames_81[];
extern char gArmorLegsNames_82[];
extern char gArmorLegsNames_83[];
extern char gArmorLegsNames_84[];
extern char gArmorLegsNames_85[];
extern char gArmorLegsNames_86[];
extern char gArmorLegsNames_87[];
extern char gArmorLegsNames_88[];
extern char gArmorLegsNames_89[];
extern char gArmorLegsNames_90[];
extern char gArmorLegsNames_91[];
extern char gArmorLegsNames_92[];
extern char gArmorLegsNames_93[];
extern char gArmorLegsNames_94[];
extern char gArmorLegsNames_95[];
extern char gArmorLegsNames_96[];
extern char gArmorLegsNames_97[];
extern char gArmorLegsNames_98[];
extern char gArmorLegsNames_99[];
extern char gArmorLegsNames_100[];
extern char gArmorLegsNames_101[];
extern char gArmorLegsNames_102[];
extern char gArmorLegsNames_103[];
extern char gArmorLegsNames_104[];
extern char gArmorLegsNames_105[];
extern char gArmorLegsNames_106[];
extern char gArmorLegsNames_107[];
extern char gArmorLegsNames_108[];
extern char gArmorLegsNames_109[];
extern char gArmorLegsNames_110[];
extern char gArmorLegsNames_111[];
extern char gArmorLegsNames_112[];
extern char gArmorLegsNames_113[];
extern char gArmorLegsNames_114[];
extern char gArmorLegsNames_115[];
extern char gArmorLegsNames_116[];
extern char gArmorLegsNames_117[];
extern char gArmorLegsNames_118[];
extern char gArmorLegsNames_119[];
extern char gArmorLegsNames_120[];
extern char gArmorLegsNames_121[];
extern char gArmorLegsNames_122[];
extern char gArmorLegsNames_123[];
extern char gArmorLegsNames_124[];
extern char gArmorLegsNames_125[];
extern char gArmorLegsNames_126[];
extern char gArmorLegsNames_127[];
extern char gArmorLegsNames_128[];
extern char gArmorLegsNames_129[];
extern char gArmorLegsNames_130[];
extern char gArmorLegsNames_131[];
extern char gArmorLegsNames_132[];
extern char gArmorLegsNames_133[];
extern char gArmorLegsNames_134[];
extern char gArmorLegsNames_135[];
extern char gArmorLegsNames_136[];
extern char gArmorLegsNames_137[];
extern char gArmorLegsNames_138[];
extern char gArmorLegsNames_139[];
extern char gArmorLegsNames_140[];
extern char gArmorLegsNames_141[];
extern char gArmorLegsNames_142[];
extern char gArmorLegsNames_143[];
extern char gArmorLegsNames_144[];
extern char gArmorLegsNames_145[];
extern char gArmorLegsNames_146[];
extern char gArmorLegsNames_147[];
extern char gArmorLegsNames_148[];
extern char gArmorLegsNames_149[];
extern char gArmorLegsNames_150[];
extern char gArmorLegsNames_151[];
extern char gArmorLegsNames_152[];
extern char gArmorLegsNames_153[];
extern char gArmorLegsNames_154[];
extern char gArmorLegsNames_155[];
extern char gArmorLegsNames_156[];
extern char gArmorLegsNames_157[];
extern char gArmorLegsNames_158[];
extern char gArmorLegsNames_159[];
extern char gArmorLegsNames_160[];
extern char gArmorLegsNames_161[];
extern char gArmorLegsNames_162[];
extern char gArmorLegsNames_163[];
extern char gArmorLegsNames_164[];
extern char gArmorLegsNames_165[];
extern char D_004AA8D0[];
extern char D_004AA8F0[];
extern char gArmorLegsNames_170[];
extern char gArmorLegsNames_171[];
extern char gArmorLegsNames_172[];
extern char gArmorLegsNames_173[];
extern char gArmorLegsNames_174[];
extern char gArmorLegsNames_175[];
extern char gArmorLegsNames_176[];
extern char gArmorLegsNames_177[];
extern char gArmorLegsNames_178[];
extern char gArmorLegsNames_179[];
extern char gArmorLegsNames_180[];
extern char gArmorLegsNames_181[];
extern char gArmorLegsNames_182[];
extern char gArmorLegsNames_183[];
extern char gArmorLegsNames_184[];
extern char gArmorLegsNames_185[];
extern char gArmorLegsNames_186[];
extern char gArmorLegsNames_187[];
extern char gArmorLegsNames_188[];
extern char gArmorLegsNames_189[];
extern char gArmorLegsNames_190[];
extern char gArmorLegsNames_191[];
extern char gArmorLegsNames_192[];
extern char gArmorLegsNames_193[];
extern char gArmorLegsNames_194[];
extern char gArmorLegsNames_195[];
extern char gArmorLegsNames_196[];
extern char gArmorLegsNames_197[];
extern char gArmorLegsNames_198[];
extern char gArmorLegsNames_199[];
extern char gArmorLegsNames_200[];
extern char gArmorLegsNames_201[];
extern char gArmorLegsNames_202[];
extern char gArmorLegsNames_203[];
extern char gArmorLegsNames_204[];
extern char gArmorLegsNames_205[];
extern char gArmorLegsNames_206[];
extern char gArmorLegsNames_207[];
extern char gArmorLegsNames_208[];
extern char gArmorLegsNames_209[];
extern char gArmorLegsNames_210[];
extern char gArmorLegsNames_211[];
extern char gArmorLegsNames_212[];
extern char gArmorLegsNames_213[];
extern char gArmorLegsNames_214[];
extern char gArmorLegsNames_215[];
extern char gArmorLegsNames_216[];
extern char gArmorLegsNames_217[];
extern char gArmorLegsNames_218[];
extern char gArmorLegsNames_219[];
extern char gArmorLegsNames_220[];
extern char gArmorLegsNames_221[];
extern char gArmorLegsNames_222[];
extern char gArmorLegsNames_223[];
extern char gArmorLegsNames_224[];
extern char gArmorLegsNames_225[];
extern char gArmorLegsNames_226[];
extern char D_004AAEF0[];
extern char D_004AAF10[];
extern char gArmorLegsNames_231[];
extern char gArmorLegsNames_232[];
extern char gArmorLegsNames_233[];
extern char gArmorLegsNames_234[];
extern char gArmorLegsNames_235[];
extern u32 D_004AB180[];
extern u32 D_004AB190[];
extern u32 D_004AB1A0[];
extern u32 D_004AB1B0[];
extern u32 D_004AB1C0[];
extern u32 D_004AB1D0[];
extern u32 D_004AB1E0[];
extern u32 D_004AB1F8[];
extern u32 D_004AB200[];
extern u32 D_004AB210[];
extern u32 D_004AB218[];
extern u32 D_004AB228[];
extern u32 D_004AB230[];
extern u32 D_004AB238[];
extern u32 D_004AB240[];
extern u32 D_004AB250[];
extern u32 D_004AB258[];
extern u32 D_004AB268[];
extern u32 D_004AB278[];
extern u32 D_004AB280[];
extern u32 D_004AB290[];
extern u32 D_004AB2A0[];
extern u32 D_004AB2B8[];
extern u32 D_004AB2C8[];
extern u32 D_004AB2E0[];
extern u32 D_004AB300[];
extern u32 D_004AB318[];
extern u32 D_004AB320[];
extern u32 D_004AB328[];
extern u32 D_004AB338[];
extern u32 D_004AB340[];
extern u32 D_004AB350[];
extern u32 D_004AB360[];
extern u32 D_004AB368[];
extern u32 D_004AB378[];
extern u32 D_004AB388[];
extern u32 D_004AB398[];
extern u32 D_004AB3A0[];
extern u32 D_004AB3B0[];
extern u32 D_004AB3C0[];
extern u32 D_004AB3D0[];
extern u32 D_004AB3D8[];
extern u32 D_004AB3E0[];
extern u32 D_004AB3F0[];
extern u32 D_004AB400[];
extern u32 D_004AB408[];
extern u32 D_004AB418[];
extern u32 D_004AB428[];
extern u32 D_004AB438[];
extern u32 D_004AB448[];
extern u32 D_004AB458[];
extern u32 D_004AB460[];
extern u32 D_004AB470[];
extern u32 D_004AB480[];
extern u32 D_004AB490[];
extern u32 D_004AB4A8[];
extern u32 D_004AB4B8[];
extern u32 D_004AB4C8[];
extern u32 D_004AB4D8[];
extern u32 D_004AB4E8[];
extern u32 D_004AB4F8[];
extern u32 D_004AB510[];
extern u32 D_004AB528[];
extern u32 D_004AB530[];
extern u32 D_004AB540[];
extern u32 D_004AB550[];
extern u32 D_004AB560[];
extern u32 D_004AB570[];
extern u32 D_004AB580[];
extern u32 D_004AB598[];
extern u32 D_004AB5A0[];
extern u32 D_004AB5B0[];
extern u32 D_004AB5B8[];
extern u32 D_004AB5C0[];
extern u32 D_004AB5D8[];
extern u32 D_004AB5E8[];
extern u32 D_004AB5F8[];
extern u32 D_004AB608[];
extern u32 D_004AB618[];
extern u32 D_004AB620[];
extern u32 D_004AB630[];
extern u32 D_004AB640[];
extern u32 D_004AB648[];
extern u32 D_004AB658[];
extern u32 D_004AB668[];
extern u32 D_004AB678[];
extern u32 D_004AB680[];
extern u32 D_004AB690[];
extern u32 D_004AB698[];
extern u32 D_004AB6A0[];
extern u32 D_004AB6B0[];
extern u32 D_004AB6C0[];
extern u32 D_004AB6D0[];
extern u32 D_004AB6E0[];
extern u32 D_004AB6F0[];
extern u32 D_004AB708[];
extern u32 D_004AB718[];
extern u32 D_004AB728[];
extern u32 D_004AB738[];
extern u32 D_004AB748[];
extern u32 D_004AB758[];
extern u32 D_004AB768[];
extern u32 D_004AB778[];
extern u32 D_004AB788[];
extern u32 D_004AB798[];
extern u32 D_004AB7A8[];
extern u32 D_004AB7B8[];
extern u32 D_004AB7C8[];
extern u32 D_004AB7D8[];
extern u32 D_004AB7E8[];
extern u32 D_004AB7F8[];
extern u32 D_004AB808[];
extern u32 D_004AB818[];
extern u32 D_004AB828[];
extern u32 D_004AB838[];
extern u32 D_004AB848[];
extern u32 D_004AB850[];
extern u32 D_004AB858[];
extern u32 D_004AB860[];
extern u32 D_004AB868[];
extern u32 D_004AB870[];
extern u32 D_004AB880[];
extern u32 D_004AB890[];
extern u32 D_004AB8A0[];
extern u32 D_004AB8B0[];
extern u32 D_004AB8C0[];
extern u32 D_004AB8D0[];
extern u32 D_004AB8E0[];
extern u32 D_004AB8F0[];
extern u32 D_004AB900[];
extern u32 D_004AB910[];
extern u32 D_004AB918[];
extern u32 D_004AB920[];
extern u32 D_004AB930[];
extern u32 D_004AB940[];
extern u32 D_004AB950[];
extern u32 D_004AB958[];
extern u32 D_004AB968[];
extern u32 D_004AB978[];
extern u32 D_004AB988[];
extern u32 D_004AB998[];
extern u32 D_004AB9A8[];
extern u32 D_004AB9B8[];
extern u32 D_004AB9C8[];
extern u32 D_004AB9D8[];
extern u32 D_004AB9E8[];
extern u32 D_004AB9F8[];
extern u32 D_004ABA08[];
extern u32 D_004ABA20[];
extern u32 D_004ABA38[];
extern u32 D_004ABA48[];
extern u32 D_004ABA50[];
extern u32 D_004ABA60[];
extern u32 D_004ABA70[];
extern u32 D_004ABA90[];
extern u32 D_004ABAB0[];
extern u32 D_004ABAD0[];
extern u32 D_004ABAF0[];
extern u32 D_004ABB10[];
extern u32 D_004ABB28[];
extern u32 D_004ABB38[];
extern u32 D_004ABB48[];
extern u32 D_004ABB50[];
extern u32 D_004ABB58[];
extern u32 D_004ABB68[];
extern u32 D_004ABB70[];
extern u32 D_004ABB78[];
extern u32 D_004ABB80[];
extern u32 D_004ABB88[];
extern u32 D_004ABB90[];
extern u32 D_004ABBA0[];
extern u32 D_004ABBA8[];
extern u32 D_004ABBB0[];
extern u32 D_004ABBB8[];
extern u32 D_004ABBC8[];
extern u32 D_004ABBD0[];
extern u32 D_004ABBE0[];
extern u32 D_004ABBF0[];
extern u32 D_004ABC00[];
extern u32 D_004ABC10[];
extern u32 D_004ABC20[];
extern u32 D_004ABC30[];
extern u32 D_004ABC40[];
extern u32 D_004ABC50[];
extern u32 D_004ABC60[];
extern u32 D_004ABC70[];
extern u32 D_004ABC80[];
extern u32 D_004ABC90[];
extern u32 D_004ABCA0[];
extern u32 D_004ABCB0[];
extern u32 D_004ABCC0[];
extern u32 D_004ABCD0[];
extern u32 D_004ABCF0[];
extern u32 D_004ABD08[];
extern u32 D_004ABD18[];
extern u32 D_004ABD28[];
extern u32 D_004ABD38[];
extern u32 D_004ABD48[];
extern u32 D_004ABD58[];
extern u32 D_004ABD68[];
extern u32 D_004ABD78[];
extern u32 D_004ABD88[];
extern u32 D_004ABD98[];
extern u32 D_004ABDA0[];
extern u32 D_004ABDA8[];
extern u32 D_004ABDB0[];
extern u32 D_004ABDB8[];
extern u32 D_004ABDC8[];
extern u32 D_004ABDE0[];
extern u32 D_004ABE00[];
extern u32 D_004ABE18[];
extern u32 D_004ABE28[];
extern u32 D_004ABE30[];
extern u32 D_004ABE40[];
extern u32 D_004ABE60[];
extern u32 D_004ABE78[];
extern u32 D_004ABE80[];
extern u32 D_004ABE88[];
extern u32 D_004ABE98[];
extern u32 D_004ABEA8[];
extern u32 D_004ABEB8[];
extern u32 D_004ABEC8[];
extern u32 D_004ABED8[];
extern u32 D_004ABEE8[];
extern u32 D_004ABEF0[];
extern u32 D_004ABF00[];
extern u32 D_004ABF10[];
extern u32 D_004ABF28[];
extern u32 D_004ABF40[];
extern u32 D_004ABF58[];
extern u32 D_004ABF68[];
extern u32 D_004ABF78[];
extern u32 D_004ABF90[];
extern u32 D_004ABFA8[];
extern u32 D_004ABFB8[];
extern u32 D_004ABFD0[];
extern u32 D_004ABFE8[];
extern u32 D_004ABFF0[];
extern u32 D_004AC000[];
extern u32 D_004AC008[];
extern u32 D_004AC010[];
extern u32 D_004AC018[];
extern u32 D_004AC028[];
extern u32 D_004AC038[];
extern u32 D_004AC048[];
extern u32 D_004AC058[];
extern u32 D_004AC068[];
extern u32 D_004AC070[];
extern u32 D_004AC078[];
extern u32 D_004AC080[];
extern u32 D_004AC088[];
extern u32 D_004AC098[];
extern u32 D_004AC0A8[];
extern u32 D_004AC0B8[];
extern u32 D_004AC0C8[];
extern u32 D_004AC0D8[];
extern u32 D_004AC0F0[];
extern u32 D_004AC108[];
extern u32 D_004AC118[];
extern u32 D_004AC128[];
extern u32 D_004AC138[];
extern u32 D_004AC150[];
extern u32 D_004AC168[];
extern u32 D_004AC178[];
extern u32 D_004AC188[];
extern u32 D_004AC190[];
extern u32 D_004AC1A0[];
extern u32 D_004AC1B0[];
extern u32 D_004AC1C0[];
extern u32 D_004AC1D0[];
extern u32 D_004AC1E0[];
extern u32 D_004AC1F0[];
extern u32 D_004AC208[];
extern u32 D_004AC210[];
extern u32 D_004AC220[];
extern u32 D_004AC230[];
extern u32 D_004AC240[];
extern u32 D_004AC250[];
extern u32 D_004AC260[];
extern u32 D_004AC270[];
extern u32 D_004AC288[];
extern u32 D_004AC298[];
extern u32 D_004AC2A8[];
extern u32 D_004AC2B0[];
extern u32 D_004AC2C0[];
extern u32 D_004AC2D0[];
extern u32 D_004AC2E8[];
extern u32 D_004AC2F8[];
extern u32 D_004AC300[];
extern u32 D_004AC308[];
extern u32 D_004AC310[];
extern u32 D_004AC318[];
extern u32 D_004AC320[];
extern u32 D_004AC328[];
extern u32 D_004AC338[];
extern u32 D_004AC348[];
extern u32 D_004AC358[];
extern u32 D_004AC368[];
extern u32 D_004AC378[];
extern u32 D_004AC380[];
extern u32 D_004AC388[];
extern u32 D_004AC398[];
extern u32 D_004AC3A8[];
extern u32 D_004AC3B8[];
extern u32 D_004AC3C8[];
extern u32 D_004AC3D8[];
extern u32 D_004AC3E0[];
extern u32 D_004AC3E8[];
extern u32 D_004AC3F0[];
extern u32 D_004AC400[];
extern u32 D_004AC410[];
extern u32 D_004AC420[];
extern u32 D_004AC428[];
extern u32 D_004AC438[];
extern u32 D_004AC440[];
extern u32 D_004AC450[];
extern u32 D_004AC460[];
extern u32 D_004AC470[];
extern u32 D_004AC480[];
extern u32 D_004AC490[];
extern u32 D_004AC4A0[];
extern u32 D_004AC4B0[];
extern u32 D_004AC4C0[];
extern u32 D_004AC4D0[];
extern u32 D_004AC4E0[];
extern u32 D_004AC4F0[];
extern u32 D_004AC500[];
extern u32 D_004AC510[];
extern u32 D_004AC520[];
extern u32 D_004AC540[];
extern u32 D_004AC558[];
extern u32 D_004AC570[];
extern u32 D_004AC588[];
extern u32 D_004AC598[];
extern u32 D_004AC5B0[];
extern u32 D_004AC5C8[];
extern u32 D_004AC5D8[];
extern u32 D_004AC5E8[];
extern u32 D_004AC600[];
extern u32 D_004AC620[];
extern u32 D_004AC638[];
extern u32 D_004AC648[];
extern u32 D_004AC658[];
extern u32 D_004AC668[];
extern u32 D_004AC680[];
extern u32 D_004AC6A0[];
extern u32 D_004AC6B8[];
extern u32 D_004AC6C8[];
extern u32 D_004AC6D8[];
extern u32 D_004AC6E8[];
extern u32 D_004AC6F8[];
extern u32 D_004AC708[];
extern u32 D_004AC718[];
extern u32 D_004AC728[];
extern u32 D_004AC738[];
extern u32 D_004AC748[];
extern u32 D_004AC758[];
extern u32 D_004AC768[];
extern u32 D_004AC778[];
extern u32 D_004AC788[];
extern u32 D_004AC798[];
extern u32 D_004AC7A8[];
extern u32 D_004AC7B8[];
extern u32 D_004AC7C8[];
extern u32 D_004AC7D8[];
extern u32 D_004AC7E8[];
extern u32 D_004AC7F8[];
extern u32 D_004AC808[];
extern u32 D_004AC818[];
extern u32 D_004AC828[];
extern u32 D_004AC838[];
extern u32 D_004AC848[];
extern u32 D_004AC858[];
extern u32 D_004AC868[];
extern u32 D_004AC878[];
extern u32 D_004AC888[];
extern u32 D_004AC898[];
extern u32 D_004AC8A8[];
extern u32 D_004AC8B8[];
extern u32 D_004AC8C8[];
extern u32 D_004AC8D8[];
extern u32 D_004AC8E8[];
extern u32 D_004AC8F8[];
extern u32 D_004AC908[];
extern u32 D_004AC918[];
extern u32 D_004AC928[];
extern u32 D_004AC938[];
extern u32 D_004AC948[];
extern u32 D_004AC958[];
extern u32 D_004AC968[];
extern u32 D_004AC978[];
extern u32 D_004AC988[];
extern u32 D_004AC998[];
extern u32 D_004AC9A8[];
extern u32 D_004AC9B8[];
extern u32 D_004AC9C8[];
extern u32 D_004AC9D8[];
extern u32 D_004AC9E8[];
extern u32 D_004AC9F8[];
extern u32 D_004ACA08[];
extern u32 D_004ACA18[];
extern u32 D_004ACA28[];
extern u32 D_004ACA38[];
extern u32 D_004ACA48[];
extern u32 D_004ACA58[];
extern u32 D_004ACA68[];
extern u32 D_004ACA78[];
extern u32 D_004ACA88[];
extern u32 D_004ACA98[];
extern u32 D_004ACAA8[];
extern u32 D_004ACAB8[];
extern u32 D_004ACAC8[];
extern u32 D_004ACAD8[];
extern u32 D_004ACAF0[];
extern u32 D_004ACB10[];
extern u32 D_004ACB28[];
extern u32 D_004ACB38[];
extern u32 D_004ACB48[];
extern u32 D_004ACB58[];
extern u32 D_004ACB68[];
extern u32 D_004ACB78[];
extern u32 D_004ACB88[];
extern u32 D_004ACB98[];
extern u32 D_004ACBA8[];
extern u32 D_004ACBB8[];
extern u32 D_004ACBC8[];
extern u32 D_004ACBD8[];
extern u32 D_004ACBE8[];
extern u32 D_004ACC00[];
extern u32 D_004ACC18[];
extern u32 D_004ACC28[];
extern u32 D_004ACC38[];
extern u32 D_004ACC48[];
extern u32 D_004ACC58[];
extern u32 D_004ACC68[];
extern u32 D_004ACC78[];
extern u32 D_004ACC88[];
extern u32 D_004ACC98[];
extern u32 D_004ACCA8[];
extern u32 D_004ACCB8[];
extern u32 D_004ACCC8[];
extern u32 D_004ACCE0[];
extern u32 D_004ACD00[];
extern u32 D_004ACD18[];
extern u32 D_004ACD30[];
extern u32 D_004ACD50[];
extern u32 D_004ACD68[];
extern u32 D_004ACD78[];
extern u32 D_004ACD90[];
extern u32 D_004ACDA8[];
extern u32 D_004ACDB8[];
extern u32 D_004ACDC8[];
extern u32 D_004ACDD8[];
extern u32 D_004ACDE8[];
extern u32 D_004ACDF8[];
extern u32 D_004ACE08[];
extern u32 D_004ACE20[];
extern u32 D_004ACE38[];
extern u32 D_004ACE48[];
extern u32 D_004ACE58[];
extern u32 D_004ACE68[];
extern u32 D_004ACE78[];
extern u32 D_004ACE88[];
extern u32 D_004ACEA0[];
extern u32 D_004ACEB8[];
extern u32 D_004ACEC8[];
extern u32 D_004ACED8[];
extern u32 D_004ACEE8[];
extern u32 D_004ACEF8[];
extern u32 D_004ACF08[];
extern u32 D_004ACF18[];
extern u32 D_004ACF28[];
extern u32 D_004ACF38[];
extern u32 D_004ACF48[];
extern u32 D_004ACF58[];
extern u32 D_004ACF68[];
extern u32 D_004ACF78[];
extern u32 D_004ACF88[];
extern u32 D_004ACF98[];
extern u32 D_004ACFA8[];
extern u32 D_004ACFB8[];
extern u32 D_004ACFC8[];
extern u32 D_004ACFD8[];
extern u32 D_004ACFE8[];
extern u32 D_004ACFF8[];
extern u32 D_004AD008[];
extern u32 D_004AD018[];
extern u32 D_004AD028[];
extern u32 D_004AD038[];
extern u32 D_004AD048[];
extern u32 D_004AD058[];
extern u32 D_004AD068[];
extern u32 D_004AD078[];
extern u32 D_004AD088[];
extern u32 D_004AD098[];
extern u32 D_004AD0A8[];
extern u32 D_004AD0B8[];
extern u32 D_004AD0C8[];
extern u32 D_004AD0D8[];
extern u32 D_004AD0E8[];
extern u32 D_004AD0F8[];
extern u32 D_004AD108[];
extern u32 D_004AD118[];
extern u32 D_004AD128[];
extern u32 D_004AD138[];
extern u32 D_004AD148[];
extern u32 D_004AD158[];
extern u32 D_004AD168[];
extern u32 D_004AD178[];
extern u32 D_004AD180[];
extern u32 D_004AD190[];
extern u32 D_004AD1A0[];
extern u32 D_004AD1B0[];
extern u32 D_004AD1C0[];
extern u32 D_004AD1D0[];
extern u32 D_004AD1E0[];
extern u32 D_004AD1F0[];
extern u32 D_004AD200[];
extern u32 D_004AD210[];
extern u32 D_004AD220[];
extern u32 D_004AD230[];
extern u32 D_004AD240[];
extern u32 D_004AD250[];
extern u32 D_004AD260[];
extern u32 D_004AD270[];
extern u32 D_004AD280[];
extern u32 D_004AD290[];
extern u32 D_004AD2A0[];
extern u32 D_004AD2B0[];
extern u32 D_004AD2C0[];
extern u32 D_004AD2D0[];
extern u32 D_004AD2E0[];
extern u32 D_004AD2F0[];
extern u32 D_004AD300[];
extern u32 D_004AD310[];
extern u32 D_004AD320[];
extern u32 D_004AD330[];
extern u32 D_004AD340[];
extern u32 D_004AD350[];
extern u32 D_004AD360[];
extern u32 D_004AD370[];
extern u32 D_004AD380[];
extern u32 D_004AD390[];
extern u32 D_004AD3A0[];
extern u32 D_004AD3B0[];
extern u32 D_004AD3C0[];
extern u32 D_004AD3D0[];
extern u32 D_004AD3E0[];
extern u32 D_004AD3F0[];
extern u32 D_004AD400[];
extern u32 D_004AD410[];
extern u32 D_004AD420[];
extern u32 D_004AD430[];
extern u32 D_004AD448[];
extern u32 D_004AD458[];
extern u32 D_004AD468[];
extern u32 D_004AD478[];
extern u32 D_004AD488[];
extern u32 D_004AD4A0[];
extern u32 D_004AD4B8[];
extern u32 D_004AD4C8[];
extern u32 D_004AD4D8[];
extern u32 D_004AD4E8[];
extern u32 D_004AD4F8[];
extern u32 D_004AD508[];
extern u32 D_004AD518[];
extern u32 D_004AD528[];
extern u32 D_004AD530[];
extern u32 D_004AD538[];
extern u32 D_004AD548[];
extern u32 D_004AD558[];
extern u32 D_004AD568[];
extern u32 D_004AD578[];
extern u32 D_004AD588[];
extern u32 D_004AD598[];
extern u32 D_004AD5A8[];
extern u32 D_004AD5B8[];
extern u32 D_004AD5C8[];
extern u32 D_004AD5E0[];
extern u32 D_004AD5F8[];
extern u32 D_004AD608[];
extern u32 D_004AD618[];
extern u32 D_004AD628[];
extern u32 D_004AD638[];
extern u32 D_004AD648[];
extern u32 D_004AD658[];
extern u32 D_004AD668[];
extern u32 D_004AD678[];
extern u32 D_004AD688[];
extern u32 D_004AD698[];
extern u32 D_004AD6A8[];
extern u32 D_004AD6B8[];
extern u32 D_004AD6C8[];
extern u32 D_004AD6D8[];
extern u32 D_004AD6E8[];
extern u32 D_004AD700[];
extern u32 D_004AD720[];
extern u32 D_004AD740[];
extern u32 D_004AD760[];
extern u32 D_004AD778[];
extern u32 D_004AD790[];
extern u32 D_004AD7A8[];
extern u32 D_004AD7B8[];
extern u32 D_004AD7C8[];
extern u32 D_004AD7D8[];
extern u32 D_004AD7F0[];
extern u32 D_004AD810[];
extern u32 D_004AD830[];
extern u32 D_004AD848[];
extern u32 D_004AD858[];
extern u32 D_004AD868[];
extern u32 D_004AD880[];
extern u32 D_004AD8A0[];
extern u32 D_004AD8C0[];
extern u32 D_004AD8D8[];
extern u32 D_004AD8F0[];
extern u32 D_004AD908[];
extern u32 D_004AD918[];
extern u32 D_004AD930[];
extern u32 D_004AD948[];
extern u32 D_004AD960[];
extern u32 D_004AD980[];
extern u32 D_004AD9A0[];
extern u32 D_004AD9B8[];
extern u32 D_004AD9D0[];
extern u32 D_004AD9E8[];
extern u32 D_004AD9F8[];
extern u32 D_004ADA08[];
extern u32 D_004ADA10[];
extern u32 D_004ADA18[];
extern u32 D_004ADA30[];
extern u32 D_004ADA48[];
extern u32 D_004ADA58[];
extern u32 D_004ADA68[];
extern u32 D_004ADA78[];
extern u32 D_004ADA90[];
extern u32 D_004ADAB0[];
extern u32 D_004ADAC8[];
extern u32 D_004ADAD8[];
extern u32 D_004ADAE8[];
extern u32 D_004ADAF8[];
extern u32 D_004ADB08[];
extern u32 D_004ADB18[];
extern u32 D_004ADB28[];
extern u32 D_004ADB38[];
extern u32 D_004ADB48[];
extern u32 D_004ADB50[];
extern u32 D_004ADB60[];
extern u32 D_004ADB70[];
extern u32 D_004ADB80[];
extern u32 D_004ADB90[];
extern u32 D_004ADBA0[];
extern u32 D_004ADBB0[];
extern u32 D_004ADBC0[];
extern u32 D_004ADBD0[];
extern u32 D_004ADBE0[];
extern u32 D_004ADBF0[];
extern u32 D_004ADC00[];
extern u32 D_004ADC10[];
extern u32 D_004ADC18[];
extern u32 D_004ADC20[];
extern u32 D_004ADC28[];
extern u32 D_004ADC30[];
extern u32 D_004ADC38[];
extern u32 D_004ADC40[];
extern u32 D_004ADC48[];
extern u32 D_004ADC58[];
extern u32 D_004ADC68[];
extern u32 D_004ADC78[];
extern u32 D_004ADC80[];
extern u32 D_004ADC88[];
extern u32 D_004ADC98[];
extern u32 D_004ADCA8[];
extern u32 D_004ADCB8[];
extern u32 D_004ADCC8[];
extern u32 D_004ADCE0[];
extern u32 D_004ADCF8[];
extern u32 D_004ADD08[];
extern u32 D_004ADD18[];
extern u32 D_004ADD28[];
extern u32 D_004ADD38[];
extern u32 D_004ADD48[];
extern u32 D_004ADD58[];
extern u32 D_004ADD68[];
extern u32 D_004ADD78[];
extern u32 D_004ADD88[];
extern u32 D_004ADD98[];
extern u32 D_004ADDA8[];
extern u32 D_004ADDB8[];
extern u32 D_004ADDD0[];
extern u32 D_004ADDE8[];
extern u32 D_004ADE00[];
extern u32 D_004ADE18[];
extern u32 D_004ADE28[];
extern u32 D_004ADE38[];
extern u32 D_004ADE48[];
extern u32 D_004ADE58[];
extern u32 D_004ADE70[];
extern u32 D_004ADE90[];
extern u32 D_004ADEB0[];
extern u32 D_004ADEC8[];
extern u32 D_004ADED8[];
extern u32 D_004ADEE8[];
extern u32 D_004ADEF8[];
extern u32 D_004ADF08[];
extern u32 D_004ADF18[];
extern u32 D_004ADF20[];
extern u32 D_004ADF30[];
extern u32 D_004ADF40[];
extern u32 D_004ADF50[];
extern u32 D_004ADF60[];
extern u32 D_004ADF70[];
extern u32 D_004ADF80[];
extern u32 D_004ADF88[];
extern u32 D_004ADF98[];
extern u32 D_004ADFA8[];
extern u32 D_004ADFB8[];
extern u32 D_004ADFD0[];
extern u32 D_004ADFE8[];
extern u32 D_004AE000[];
extern u32 D_004AE018[];
extern u32 D_004AE028[];
extern u32 D_004AE040[];
extern u32 D_004AE058[];
extern u32 D_004AE070[];
extern u32 D_004AE090[];
extern u32 D_004AE0B0[];
extern u32 D_004AE0C8[];
extern u32 D_004AE0D8[];
extern u32 D_004AE0F0[];
extern u32 D_004AE110[];
extern u32 D_004AE130[];
extern u32 D_004AE150[];
extern u32 D_004AE168[];
extern u32 D_004AE178[];
extern u32 D_004AE188[];
extern u32 D_004AE198[];
extern u32 D_004AE1A8[];
extern u32 D_004AE1B0[];
extern u32 D_004AE1C0[];
extern u32 D_004AE1C8[];
extern u32 D_004AE1D0[];
extern u32 D_004AE1D8[];
extern u32 D_004AE1E0[];
extern u32 D_004AE1E8[];
extern u32 D_004AE1F0[];
extern u32 D_004AE1F8[];
extern u32 D_004AE200[];
extern u32 D_004AE208[];
extern u32 D_004AE210[];
extern u32 D_004AE218[];
extern u32 D_004AE220[];
extern u32 D_004AE228[];
extern u32 D_004AE230[];
extern u32 D_004AE238[];
extern u32 D_004AE240[];
extern u32 D_004AE248[];
extern u32 D_004AE250[];
extern u32 D_004AE258[];
extern u32 D_004AE260[];
extern u32 D_004AE268[];
extern u32 D_004AE270[];
extern u32 D_004AE278[];
extern u32 D_004AE280[];
extern u32 D_004AE288[];
extern u32 D_004AE290[];
extern u32 D_004AE298[];
extern u32 D_004AE2A0[];
extern u32 D_004AE2A8[];
extern u32 D_004AE2B0[];
extern u32 D_004AE2B8[];
extern u32 D_004AE2C0[];
extern u32 D_004AE2C8[];
extern u32 D_004AE2D0[];
extern u32 D_004AE2D8[];
extern u32 D_004AE2E0[];
extern u32 D_004AE2E8[];
extern u32 D_004AE2F0[];
extern u32 D_004AE2F8[];
extern u32 D_004AE300[];
extern u32 D_004AE308[];
extern u32 D_004AE310[];
extern u32 D_004AE318[];
extern u32 D_004AE320[];
extern u32 D_004AE328[];
extern u32 D_004AE330[];
extern u32 D_004AE338[];
extern u32 D_004AE340[];
extern u32 D_004AE348[];
extern u32 D_004AE350[];
extern u32 D_004AE358[];
extern u32 D_004AE360[];
extern u32 D_004AE368[];
extern u32 D_004AE370[];
extern u32 D_004AE378[];
extern u32 D_004AE380[];
extern u32 D_004AE388[];
extern u32 D_004AE390[];
extern u32 D_004AE398[];
extern u32 D_004AE3A0[];
extern u32 D_004AE3A8[];
extern u32 D_004AE3B0[];
extern u32 D_004AE3B8[];
extern u32 D_004AE3C0[];
extern u32 D_004AE3C8[];
extern u32 D_004AE3D0[];
extern u32 D_004AE3D8[];
extern u32 D_004AE3E0[];
extern u32 D_004AE3E8[];
extern u32 D_004AE3F0[];
extern u32 D_004AE3F8[];
extern u32 D_004AE400[];
extern u32 D_004AE408[];
extern u32 D_004AE410[];
extern u32 D_004AE420[];
extern u32 D_004AE428[];
extern u32 D_004AE430[];
extern u32 D_004AE438[];
extern u32 D_004AE440[];
extern u32 D_004AE448[];
extern u32 D_004AE450[];
extern u32 D_004AE458[];
extern u32 D_004AE460[];
extern u32 D_004AE468[];
extern u32 D_004AE470[];
extern u32 D_004AE478[];
extern u32 D_004AE480[];
extern u32 D_004AE488[];
extern u32 D_004AE490[];
extern u32 D_004AE498[];
extern u32 D_004AE4A0[];
extern u32 D_004AE4A8[];
extern u32 D_004AE4B0[];
extern u32 D_004AE4B8[];
extern u32 D_004AE4C0[];
extern u32 D_004AE4C8[];
extern u32 D_004AE4D0[];
extern u32 D_004AE4D8[];
extern u32 D_004AE4E0[];
extern u32 D_004AE4E8[];
extern u32 D_004AE4F0[];
extern u32 D_004AE4F8[];
extern u32 D_004AE500[];
extern u32 D_004AE508[];
extern u32 D_004AE510[];
extern u32 D_004AE518[];
extern u32 D_004AE520[];
extern u32 D_004AE528[];
extern u32 D_004AE530[];
extern u32 D_004AE538[];
extern u32 D_004AE540[];
extern u32 D_004AE548[];
extern u32 D_004AE550[];
extern u32 D_004AE558[];
extern u32 D_004AE560[];
extern u32 D_004AE568[];
extern u32 D_004AE570[];
extern u32 D_004AE578[];
extern u32 D_004AE580[];
extern u32 D_004AE588[];
s16 gStageFileTbls_4[12] = {
#include "sub_main/0044BF30.inc"
};
s16 gStageFileTbls_5[4] = {
#include "sub_main/0044BF48.inc"
};
s16 gStageFileTbls_6[12] = {
#include "sub_main/0044BF50.inc"
};
s16 gStageFileTbls_7[6] = {
#include "sub_main/0044BF68.inc"
};
s16 gStageFileTbls_8[72] = {
#include "sub_main/0044BF80.inc"
};
s16 gStageFileTbls_9[88] = {
#include "sub_main/0044C010.inc"
};
s16 gStageFileTbls_10[32] = {
#include "sub_main/0044C0C0.inc"
};
s16 gStageFileTbls_11[12] = {
#include "sub_main/0044C100.inc"
};
s16 gStageFileTbls_12[4] = {
#include "sub_main/0044C118.inc"
};
s16 gStageFileTbls_13[56] = {
#include "sub_main/0044C120.inc"
};
s16 *gStageFileTbls[16] = {
    gStageFileTbls_0, gStageFileTbls_1, gStageFileTbls_2, gStageFileTbls_3,
    gStageFileTbls_4, gStageFileTbls_5, gStageFileTbls_6, gStageFileTbls_7,
    gStageFileTbls_8, gStageFileTbls_9, gStageFileTbls_10, gStageFileTbls_11,
    gStageFileTbls_12, gStageFileTbls_13, NULL, NULL,
};
MonsterBit gMonsterFlagBits[28] = {
#include "sub_main/0044C1D0.inc"
};
PartBreakEffect gPartBreakEffects[6] = {
#include "sub_main/0044C240.inc"
};
s16 gResistSongEffects[5] = {
#include "sub_main/0044C2A0.inc"
};
/* gGestureStates holds two tables: its own 28 bytes, then the hunter rank points, read
 * as gHrRankPoints (an alias at +0x1C). Defined as two arrays, they no longer match: sub_main
 * grows by 16 bytes; one record of the two (GestureHrTables) keeps the layout. */
GestureHrTables gGestureStates = {
#include "sub_main/0044C2B0.inc"
};
u16 gClearListQuests[328] = {
#include "sub_main/0044C3A0.inc"
};
u8 gMonsterGroups_0[8] = {
#include "sub_main/0044C630.inc"
};
u8 gMonsterGroups_1[8] = {
#include "sub_main/0044C638.inc"
};
u8 gMonsterGroups_2[8] = {
#include "sub_main/0044C640.inc"
};
u8 gMonsterGroups_3[8] = {
#include "sub_main/0044C648.inc"
};
u8 gMonsterGroups_4[8] = {
#include "sub_main/0044C650.inc"
};
u8 gMonsterGroups_5[8] = {
#include "sub_main/0044C658.inc"
};
u8 gMonsterGroups_6[8] = {
#include "sub_main/0044C660.inc"
};
u8 gMonsterGroups_7[8] = {
#include "sub_main/0044C668.inc"
};
u8 gMonsterGroups_8[8] = {
#include "sub_main/0044C670.inc"
};
u8 gMonsterGroups_9[8] = {
#include "sub_main/0044C678.inc"
};
u8 gMonsterGroups_10[8] = {
#include "sub_main/0044C680.inc"
};
u8 gMonsterGroups_11[8] = {
#include "sub_main/0044C688.inc"
};
u8 gMonsterGroups_12[8] = {
#include "sub_main/0044C690.inc"
};
u8 gMonsterGroups_13[8] = {
#include "sub_main/0044C698.inc"
};
u8 gMonsterGroups_14[8] = {
#include "sub_main/0044C6A0.inc"
};
u8 gMonsterGroups_15[8] = {
#include "sub_main/0044C6A8.inc"
};
u8 gMonsterGroups_16[8] = {
#include "sub_main/0044C6B0.inc"
};
u8 gMonsterGroups_17[8] = {
#include "sub_main/0044C6B8.inc"
};
u8 gMonsterGroups_18[8] = {
#include "sub_main/0044C6C0.inc"
};
u8 gMonsterGroups_19[8] = {
#include "sub_main/0044C6C8.inc"
};
u8 gMonsterGroups_20[8] = {
#include "sub_main/0044C6D0.inc"
};
u8 gMonsterGroups_21[8] = {
#include "sub_main/0044C6D8.inc"
};
u8 gMonsterGroups_22[8] = {
#include "sub_main/0044C6E0.inc"
};
u8 gMonsterGroups_23[8] = {
#include "sub_main/0044C6E8.inc"
};
u8 gMonsterGroups_24[8] = {
#include "sub_main/0044C6F0.inc"
};
u8 gMonsterGroups_25[8] = {
#include "sub_main/0044C6F8.inc"
};
u8 gMonsterGroups_26[8] = {
#include "sub_main/0044C700.inc"
};
u8 gMonsterGroups_27[8] = {
#include "sub_main/0044C708.inc"
};
u8 gMonsterGroups_28[8] = {
#include "sub_main/0044C710.inc"
};
u8 gMonsterGroups_29[8] = {
#include "sub_main/0044C718.inc"
};
u8 gMonsterGroups_30[8] = {
#include "sub_main/0044C720.inc"
};
u8 gMonsterGroups_31[8] = {
#include "sub_main/0044C728.inc"
};
u8 gMonsterGroups_32[8] = {
#include "sub_main/0044C730.inc"
};
u8 gMonsterGroups_33[8] = {
#include "sub_main/0044C738.inc"
};
u8 gMonsterGroups_34[8] = {
#include "sub_main/0044C740.inc"
};
u8 gMonsterGroups_40[8] = {
#include "sub_main/0044C748.inc"
};
u8 gMonsterGroups_48[8] = {
#include "sub_main/0044C750.inc"
};
u8 gMonsterGroups_51[8] = {
#include "sub_main/0044C758.inc"
};
u8 gMonsterGroups_52[8] = {
#include "sub_main/0044C760.inc"
};
u8 gMonsterGroups_53[8] = {
#include "sub_main/0044C768.inc"
};
u8 gMonsterGroups_54[8] = {
#include "sub_main/0044C770.inc"
};
u8 gMonsterGroups_55[8] = {
#include "sub_main/0044C778.inc"
};
u8 gMonsterGroups_56[8] = {
#include "sub_main/0044C780.inc"
};
u8 gMonsterGroups_57[8] = {
#include "sub_main/0044C788.inc"
};
u8 gMonsterGroups_58[8] = {
#include "sub_main/0044C790.inc"
};
u8 gMonsterGroups_59[8] = {
#include "sub_main/0044C798.inc"
};
u8 gMonsterGroups_61[8] = {
#include "sub_main/0044C7A0.inc"
};
u8 gMonsterGroups_62[8] = {
#include "sub_main/0044C7A8.inc"
};
u8 gMonsterGroups_63[8] = {
#include "sub_main/0044C7B0.inc"
};
u8 gMonsterGroups_64[8] = {
#include "sub_main/0044C7B8.inc"
};
u8 gMonsterGroups_65[8] = {
#include "sub_main/0044C7C0.inc"
};
u8 gMonsterGroups_66[8] = {
#include "sub_main/0044C7C8.inc"
};
u8 gMonsterGroups_67[8] = {
#include "sub_main/0044C7D0.inc"
};
u8 gMonsterGroups_68[8] = {
#include "sub_main/0044C7D8.inc"
};
u8 gMonsterGroups_69[8] = {
#include "sub_main/0044C7E0.inc"
};
u8 gMonsterGroups_70[8] = {
#include "sub_main/0044C7E8.inc"
};
u8 gMonsterGroups_73[16] = {
#include "sub_main/0044C7F0.inc"
};
u8 *gMonsterGroups[76] = {
    gMonsterGroups_0, gMonsterGroups_1, gMonsterGroups_2, gMonsterGroups_3,
    gMonsterGroups_4, gMonsterGroups_5, gMonsterGroups_6, gMonsterGroups_7,
    gMonsterGroups_8, gMonsterGroups_9, gMonsterGroups_10, gMonsterGroups_11,
    gMonsterGroups_12, gMonsterGroups_13, gMonsterGroups_14, gMonsterGroups_15,
    gMonsterGroups_16, gMonsterGroups_17, gMonsterGroups_18, gMonsterGroups_19,
    gMonsterGroups_20, gMonsterGroups_21, gMonsterGroups_22, gMonsterGroups_23,
    gMonsterGroups_24, gMonsterGroups_25, gMonsterGroups_26, gMonsterGroups_27,
    gMonsterGroups_28, gMonsterGroups_29, gMonsterGroups_30, gMonsterGroups_31,
    gMonsterGroups_32, gMonsterGroups_33, gMonsterGroups_34, gMonsterGroups_16,
    gMonsterGroups_2, gMonsterGroups_1, gMonsterGroups_6, gMonsterGroups_20,
    gMonsterGroups_40, gMonsterGroups_11, gMonsterGroups_1, gMonsterGroups_14,
    gMonsterGroups_26, gMonsterGroups_15, gMonsterGroups_21, gMonsterGroups_17,
    gMonsterGroups_48, gMonsterGroups_11, gMonsterGroups_7, gMonsterGroups_51,
    gMonsterGroups_52, gMonsterGroups_53, gMonsterGroups_54, gMonsterGroups_55,
    gMonsterGroups_56, gMonsterGroups_57, gMonsterGroups_58, gMonsterGroups_59,
    gMonsterGroups_54, gMonsterGroups_61, gMonsterGroups_62, gMonsterGroups_63,
    gMonsterGroups_64, gMonsterGroups_65, gMonsterGroups_66, gMonsterGroups_67,
    gMonsterGroups_68, gMonsterGroups_69, gMonsterGroups_70, gMonsterGroups_2,
    gMonsterGroups_58, gMonsterGroups_73, NULL, NULL,
};
f32 gMonsterBaseSize[76] = {
#include "sub_main/0044C930.inc"
};
HrStep gHrPointSteps[20] = {
#include "sub_main/0044CA60.inc"
};
u16 gObtainFlagItems[24] = {
#include "sub_main/0044CB00.inc"
};
/* equipment ids of type 6, ending 9999 */
u16 gEquipCollectType6[48] = {
#include "sub_main/0044CB30.inc"
};
/* of type 7 */
u16 gEquipCollectType7[16] = {
#include "sub_main/0044CB90.inc"
};
/* per class, the number Award_Grant gets when all are owned */
u16 gEquipCollectClassAwards[16] = {
#include "sub_main/0044CBB0.inc"
};
u16 gEquipCollectLists_0[16] = {
#include "sub_main/0044CBD0.inc"
};
u16 gEquipCollectLists_1[16] = {
#include "sub_main/0044CBF0.inc"
};
u16 gEquipCollectLists_2[16] = {
#include "sub_main/0044CC10.inc"
};
u16 gEquipCollectLists_3[16] = {
#include "sub_main/0044CC30.inc"
};
u16 gEquipCollectLists_4[16] = {
#include "sub_main/0044CC50.inc"
};
u16 *gEquipCollectLists[6] = {
    gEquipCollectLists_0, gEquipCollectLists_1, gEquipCollectLists_2, gEquipCollectLists_3, gEquipCollectLists_4, 0,
};
u16 gEquipCollectKinds[6] = {
#include "sub_main/0044CC88.inc"
};
BladeData gBladeData[486] = {
#include "sub_main/0044CCA0.inc"
};
GunData gGunData[128] = {
#include "sub_main/0044FA30.inc"
};
ArmorData gArmorHeadData[253] = {
#include "sub_main/00450630.inc"
};
ArmorData gArmorBodyData[243] = {
#include "sub_main/00452DC0.inc"
};
ArmorData gArmorArmsData[238] = {
#include "sub_main/004553C0.inc"
};
ArmorData gArmorWaistData[237] = {
#include "sub_main/004578F0.inc"
};
ArmorData gArmorLegsData[236] = {
#include "sub_main/00459E00.inc"
};
char *gBladeNames[488] = {
    D_004A00E8, gBladeNames_1, gBladeNames_2, gBladeNames_3,
    gBladeNames_4, gBladeNames_5, gBladeNames_6, gBladeNames_7,
    gBladeNames_8, gBladeNames_9, gBladeNames_10, gBladeNames_11,
    gBladeNames_12, gBladeNames_13, gBladeNames_14, gBladeNames_15,
    gBladeNames_16, gBladeNames_17, gBladeNames_18, gBladeNames_19,
    gBladeNames_20, gBladeNames_21, gBladeNames_22, gBladeNames_23,
    gBladeNames_24, gBladeNames_25, gBladeNames_26, gBladeNames_27,
    gBladeNames_28, gBladeNames_29, gBladeNames_30, gBladeNames_31,
    gBladeNames_32, gBladeNames_33, gBladeNames_34, gBladeNames_35,
    gBladeNames_36, gBladeNames_37, gBladeNames_38, gBladeNames_39,
    gBladeNames_40, gBladeNames_41, gBladeNames_42, gBladeNames_43,
    gBladeNames_44, gBladeNames_45, gBladeNames_46, gBladeNames_47,
    gBladeNames_48, gBladeNames_49, gBladeNames_50, gBladeNames_51,
    gBladeNames_52, gBladeNames_53, gBladeNames_54, gBladeNames_55,
    gBladeNames_56, gBladeNames_57, gBladeNames_58, gBladeNames_59,
    gBladeNames_60, gBladeNames_61, gBladeNames_62, gBladeNames_63,
    gBladeNames_64, gBladeNames_65, gBladeNames_66, gBladeNames_67,
    gBladeNames_68, gBladeNames_69, gBladeNames_70, gBladeNames_71,
    gBladeNames_72, gBladeNames_73, gBladeNames_74, gBladeNames_75,
    gBladeNames_76, gBladeNames_77, gBladeNames_78, gBladeNames_79,
    gBladeNames_80, gBladeNames_81, gBladeNames_82, gBladeNames_83,
    gBladeNames_84, gBladeNames_85, gBladeNames_86, gBladeNames_87,
    gBladeNames_88, gBladeNames_89, gBladeNames_90, gBladeNames_91,
    gBladeNames_92, gBladeNames_93, gBladeNames_94, gBladeNames_95,
    gBladeNames_96, gBladeNames_97, gBladeNames_98, gBladeNames_99,
    gBladeNames_100, gBladeNames_101, gBladeNames_102, gBladeNames_103,
    gBladeNames_104, gBladeNames_105, gBladeNames_106, gBladeNames_107,
    gBladeNames_108, gBladeNames_109, gBladeNames_110, gBladeNames_111,
    gBladeNames_112, gBladeNames_113, gBladeNames_114, gBladeNames_115,
    gBladeNames_116, gBladeNames_117, gBladeNames_118, gBladeNames_119,
    gBladeNames_120, gBladeNames_121, gBladeNames_122, gBladeNames_123,
    gBladeNames_124, gBladeNames_125, gBladeNames_126, gBladeNames_127,
    gBladeNames_128, gBladeNames_129, gBladeNames_130, gBladeNames_131,
    gBladeNames_132, gBladeNames_133, gBladeNames_134, gBladeNames_135,
    gBladeNames_136, gBladeNames_137, gBladeNames_138, gBladeNames_139,
    gBladeNames_140, gBladeNames_141, gBladeNames_142, gBladeNames_143,
    gBladeNames_144, gBladeNames_145, gBladeNames_146, gBladeNames_147,
    gBladeNames_148, gBladeNames_149, gBladeNames_150, gBladeNames_151,
    gBladeNames_152, gBladeNames_153, gBladeNames_154, gBladeNames_155,
    gBladeNames_156, gBladeNames_157, gBladeNames_158, gBladeNames_159,
    gBladeNames_160, gBladeNames_161, gBladeNames_162, gBladeNames_163,
    gBladeNames_164, gBladeNames_165, gBladeNames_166, gBladeNames_167,
    gBladeNames_168, gBladeNames_169, gBladeNames_170, gBladeNames_171,
    gBladeNames_172, gBladeNames_173, gBladeNames_174, gBladeNames_175,
    gBladeNames_176, gBladeNames_177, gBladeNames_178, gBladeNames_179,
    gBladeNames_180, gBladeNames_181, gBladeNames_182, gBladeNames_183,
    gBladeNames_184, gBladeNames_185, gBladeNames_186, gBladeNames_187,
    gBladeNames_188, gBladeNames_189, gBladeNames_190, gBladeNames_191,
    gBladeNames_192, gBladeNames_193, gBladeNames_194, gBladeNames_195,
    gBladeNames_196, gBladeNames_197, gBladeNames_198, gBladeNames_199,
    gBladeNames_200, gBladeNames_201, gBladeNames_202, gBladeNames_203,
    gBladeNames_204, gBladeNames_205, gBladeNames_206, gBladeNames_207,
    gBladeNames_208, gBladeNames_209, gBladeNames_210, gBladeNames_211,
    gBladeNames_212, gBladeNames_213, gBladeNames_214, gBladeNames_215,
    gBladeNames_216, gBladeNames_217, gBladeNames_218, gBladeNames_219,
    gBladeNames_220, gBladeNames_221, gBladeNames_222, gBladeNames_223,
    gBladeNames_224, gBladeNames_225, gBladeNames_226, gBladeNames_227,
    gBladeNames_228, gBladeNames_229, gBladeNames_230, gBladeNames_231,
    gBladeNames_232, gBladeNames_233, gBladeNames_234, gBladeNames_235,
    gBladeNames_236, gBladeNames_237, gBladeNames_238, gBladeNames_239,
    gBladeNames_240, gBladeNames_241, gBladeNames_242, gBladeNames_243,
    gBladeNames_244, gBladeNames_245, gBladeNames_246, gBladeNames_247,
    gBladeNames_248, gBladeNames_249, gBladeNames_250, gBladeNames_251,
    gBladeNames_252, gBladeNames_253, gBladeNames_254, gBladeNames_255,
    gBladeNames_256, gBladeNames_257, gBladeNames_258, gBladeNames_259,
    gBladeNames_260, gBladeNames_261, gBladeNames_262, gBladeNames_263,
    gBladeNames_264, gBladeNames_265, gBladeNames_266, gBladeNames_267,
    gBladeNames_268, gBladeNames_269, gBladeNames_270, gBladeNames_271,
    gBladeNames_272, gBladeNames_273, gBladeNames_274, gBladeNames_275,
    gBladeNames_276, gBladeNames_277, gBladeNames_278, gBladeNames_279,
    gBladeNames_280, gBladeNames_281, gBladeNames_282, gBladeNames_283,
    gBladeNames_284, gBladeNames_285, gBladeNames_286, gBladeNames_287,
    gBladeNames_288, gBladeNames_289, gBladeNames_290, gBladeNames_291,
    gBladeNames_292, gBladeNames_293, gBladeNames_294, gBladeNames_295,
    gBladeNames_296, gBladeNames_297, gBladeNames_298, gBladeNames_299,
    gBladeNames_300, gBladeNames_301, gBladeNames_302, gBladeNames_303,
    gBladeNames_304, gBladeNames_305, gBladeNames_306, gBladeNames_307,
    gBladeNames_308, gBladeNames_309, gBladeNames_310, gBladeNames_311,
    gBladeNames_312, gBladeNames_313, gBladeNames_314, gBladeNames_315,
    gBladeNames_316, gBladeNames_317, gBladeNames_318, gBladeNames_319,
    gBladeNames_320, gBladeNames_321, gBladeNames_322, gBladeNames_323,
    gBladeNames_324, gBladeNames_325, gBladeNames_326, gBladeNames_327,
    gBladeNames_328, gBladeNames_329, gBladeNames_330, gBladeNames_331,
    gBladeNames_332, gBladeNames_333, gBladeNames_334, gBladeNames_335,
    gBladeNames_336, gBladeNames_337, gBladeNames_338, gBladeNames_339,
    gBladeNames_340, gBladeNames_341, gBladeNames_342, gBladeNames_343,
    gBladeNames_344, gBladeNames_345, gBladeNames_346, gBladeNames_347,
    gBladeNames_348, gBladeNames_349, gBladeNames_350, gBladeNames_351,
    gBladeNames_352, gBladeNames_353, gBladeNames_354, gBladeNames_355,
    gBladeNames_356, gBladeNames_357, gBladeNames_358, gBladeNames_359,
    gBladeNames_360, gBladeNames_361, gBladeNames_362, gBladeNames_363,
    gBladeNames_364, gBladeNames_365, gBladeNames_366, gBladeNames_367,
    gBladeNames_368, gBladeNames_369, gBladeNames_370, gBladeNames_371,
    gBladeNames_372, gBladeNames_373, gBladeNames_374, gBladeNames_375,
    gBladeNames_376, gBladeNames_377, gBladeNames_378, gBladeNames_379,
    gBladeNames_380, gBladeNames_381, gBladeNames_382, gBladeNames_383,
    gBladeNames_384, gBladeNames_385, gBladeNames_386, gBladeNames_387,
    gBladeNames_388, gBladeNames_389, gBladeNames_390, gBladeNames_391,
    gBladeNames_392, gBladeNames_393, gBladeNames_394, gBladeNames_395,
    gBladeNames_396, gBladeNames_397, gBladeNames_398, gBladeNames_399,
    gBladeNames_400, gBladeNames_401, gBladeNames_402, gBladeNames_403,
    gBladeNames_404, gBladeNames_405, gBladeNames_406, gBladeNames_407,
    gBladeNames_408, gBladeNames_409, gBladeNames_410, gBladeNames_411,
    gBladeNames_412, gBladeNames_413, gBladeNames_414, gBladeNames_415,
    gBladeNames_416, gBladeNames_417, gBladeNames_418, gBladeNames_419,
    gBladeNames_420, gBladeNames_421, gBladeNames_422, gBladeNames_423,
    gBladeNames_424, gBladeNames_425, gBladeNames_426, gBladeNames_427,
    gBladeNames_428, gBladeNames_429, gBladeNames_430, gBladeNames_431,
    gBladeNames_432, gBladeNames_433, gBladeNames_434, gBladeNames_435,
    gBladeNames_436, gBladeNames_437, gBladeNames_438, gBladeNames_439,
    gBladeNames_440, gBladeNames_441, gBladeNames_442, gBladeNames_443,
    gBladeNames_444, gBladeNames_445, gBladeNames_446, gBladeNames_447,
    gBladeNames_448, gBladeNames_449, gBladeNames_450, gBladeNames_451,
    gBladeNames_452, gBladeNames_453, gBladeNames_454, gBladeNames_455,
    gBladeNames_456, gBladeNames_457, gBladeNames_458, gBladeNames_459,
    gBladeNames_460, gBladeNames_461, gBladeNames_462, gBladeNames_463,
    gBladeNames_464, gBladeNames_465, gBladeNames_466, gBladeNames_467,
    gBladeNames_468, gBladeNames_469, gBladeNames_470, gBladeNames_471,
    gBladeNames_472, gBladeNames_473, gBladeNames_474, gBladeNames_475,
    gBladeNames_476, gBladeNames_477, gBladeNames_478, gBladeNames_479,
    gBladeNames_480, gBladeNames_481, gBladeNames_482, gBladeNames_483,
    gBladeNames_484, gBladeNames_485, 0, 0,
};
char *gGunNames[128] = {
    D_004A00E8, gGunNames_1, gGunNames_2, gGunNames_3,
    gGunNames_4, gGunNames_5, gGunNames_6, gGunNames_7,
    gGunNames_8, gGunNames_9, gGunNames_10, gGunNames_11,
    gGunNames_12, gGunNames_13, gGunNames_14, gGunNames_15,
    gGunNames_16, gGunNames_17, gGunNames_18, gGunNames_19,
    gGunNames_20, gGunNames_21, gGunNames_22, gGunNames_23,
    gGunNames_24, gGunNames_25, gGunNames_26, gGunNames_27,
    gGunNames_28, gGunNames_29, gGunNames_30, gGunNames_31,
    gGunNames_32, gGunNames_33, gGunNames_34, gGunNames_35,
    gGunNames_36, gGunNames_37, gGunNames_38, gGunNames_39,
    gGunNames_40, gGunNames_41, gGunNames_42, gGunNames_43,
    gGunNames_44, gGunNames_45, gGunNames_46, gGunNames_47,
    gGunNames_48, gGunNames_49, gGunNames_50, gGunNames_51,
    gGunNames_52, gGunNames_53, gGunNames_54, gGunNames_55,
    gGunNames_56, gGunNames_57, gGunNames_58, gGunNames_59,
    gGunNames_60, gGunNames_61, gGunNames_62, gGunNames_63,
    gGunNames_64, gGunNames_65, gGunNames_66, gGunNames_67,
    gGunNames_68, gGunNames_69, gGunNames_70, gGunNames_71,
    gGunNames_72, gGunNames_73, gGunNames_74, gGunNames_75,
    gGunNames_76, (char *)D_004A3378, (char *)D_004A3388, (char *)D_004A3398,
    gGunNames_80, gGunNames_81, gGunNames_82, gGunNames_83,
    (char *)D_004A3428, gGunNames_85, gGunNames_86, gGunNames_87,
    gGunNames_88, gGunNames_89, gGunNames_90, gGunNames_91,
    gGunNames_92, gGunNames_93, gGunNames_94, gGunNames_95,
    gGunNames_96, gGunNames_97, gGunNames_98, gGunNames_99,
    gGunNames_100, gGunNames_101, gGunNames_102, gGunNames_103,
    gGunNames_104, gGunNames_105, gGunNames_106, gGunNames_107,
    (char *)D_004A3638, (char *)D_004A3648, (char *)D_004A3658, (char *)D_004A3668,
    gGunNames_112, gGunNames_113, gGunNames_114, gGunNames_115,
    (char *)D_004A36F8, (char *)D_004A3708, gGunNames_118, gGunNames_119,
    gGunNames_120, gGunNames_121, gGunNames_122, gGunNames_123,
    gGunNames_124, gGunNames_125, gGunNames_126, gGunNames_127,
};
char *gArmorHeadNames[256] = {
    D_004A00E8, gArmorHeadNames_1, gArmorHeadNames_2, gArmorHeadNames_3,
    gArmorHeadNames_4, gArmorHeadNames_5, gArmorHeadNames_6, gArmorHeadNames_7,
    gArmorHeadNames_8, gArmorHeadNames_9, gArmorHeadNames_10, gArmorHeadNames_11,
    gArmorHeadNames_12, gArmorHeadNames_13, gArmorHeadNames_14, gArmorHeadNames_15,
    gArmorHeadNames_16, gArmorHeadNames_17, gArmorHeadNames_18, gArmorHeadNames_19,
    gArmorHeadNames_20, gArmorHeadNames_21, gArmorHeadNames_22, gArmorHeadNames_23,
    gArmorHeadNames_24, gArmorHeadNames_25, gArmorHeadNames_26, gArmorHeadNames_27,
    gArmorHeadNames_28, gArmorHeadNames_29, gArmorHeadNames_30, gArmorHeadNames_31,
    gArmorHeadNames_32, gArmorHeadNames_33, gArmorHeadNames_34, gArmorHeadNames_35,
    gArmorHeadNames_36, gArmorHeadNames_37, gArmorHeadNames_38, gArmorHeadNames_39,
    gArmorHeadNames_40, gArmorHeadNames_41, gArmorHeadNames_42, gArmorHeadNames_43,
    gArmorHeadNames_44, gArmorHeadNames_45, gArmorHeadNames_46, gArmorHeadNames_47,
    gArmorHeadNames_48, gArmorHeadNames_49, gArmorHeadNames_50, gArmorHeadNames_51,
    gArmorHeadNames_52, gArmorHeadNames_53, gArmorHeadNames_54, gArmorHeadNames_55,
    gArmorHeadNames_56, gArmorHeadNames_57, gArmorHeadNames_58, gArmorHeadNames_59,
    gArmorHeadNames_60, gArmorHeadNames_61, gArmorHeadNames_62, gArmorHeadNames_63,
    gArmorHeadNames_64, gArmorHeadNames_65, gArmorHeadNames_66, gArmorHeadNames_67,
    gArmorHeadNames_68, gArmorHeadNames_69, gArmorHeadNames_70, gArmorHeadNames_71,
    gArmorHeadNames_72, gArmorHeadNames_73, gArmorHeadNames_74, gArmorHeadNames_75,
    gArmorHeadNames_76, gArmorHeadNames_77, gArmorHeadNames_78, gArmorHeadNames_79,
    gArmorHeadNames_80, gArmorHeadNames_81, gArmorHeadNames_82, gArmorHeadNames_83,
    gArmorHeadNames_84, gArmorHeadNames_85, gArmorHeadNames_86, gArmorHeadNames_87,
    gArmorHeadNames_88, gArmorHeadNames_89, gArmorHeadNames_90, gArmorHeadNames_91,
    gArmorHeadNames_92, gArmorHeadNames_93, gArmorHeadNames_94, gArmorHeadNames_95,
    gArmorHeadNames_96, gArmorHeadNames_97, gArmorHeadNames_98, gArmorHeadNames_99,
    gArmorHeadNames_100, gArmorHeadNames_101, gArmorHeadNames_102, gArmorHeadNames_103,
    gArmorHeadNames_104, gArmorHeadNames_105, gArmorHeadNames_106, gArmorHeadNames_107,
    gArmorHeadNames_108, gArmorHeadNames_109, gArmorHeadNames_110, gArmorHeadNames_111,
    gArmorHeadNames_112, gArmorHeadNames_113, gArmorHeadNames_114, gArmorHeadNames_115,
    gArmorHeadNames_116, gArmorHeadNames_117, gArmorHeadNames_118, gArmorHeadNames_119,
    gArmorHeadNames_120, gArmorHeadNames_121, gArmorHeadNames_122, gArmorHeadNames_123,
    gArmorHeadNames_124, gArmorHeadNames_125, gArmorHeadNames_126, gArmorHeadNames_127,
    gArmorHeadNames_128, gArmorHeadNames_129, gArmorHeadNames_130, gArmorHeadNames_131,
    gArmorHeadNames_132, gArmorHeadNames_133, gArmorHeadNames_134, gArmorHeadNames_135,
    gArmorHeadNames_136, gArmorHeadNames_137, gArmorHeadNames_138, gArmorHeadNames_139,
    gArmorHeadNames_140, gArmorHeadNames_141, gArmorHeadNames_142, gArmorHeadNames_143,
    gArmorHeadNames_144, gArmorHeadNames_145, gArmorHeadNames_146, gArmorHeadNames_147,
    gArmorHeadNames_148, gArmorHeadNames_149, gArmorHeadNames_150, gArmorHeadNames_151,
    gArmorHeadNames_152, gArmorHeadNames_153, gArmorHeadNames_154, gArmorHeadNames_155,
    gArmorHeadNames_156, gArmorHeadNames_157, gArmorHeadNames_158, gArmorHeadNames_159,
    gArmorHeadNames_160, gArmorHeadNames_161, gArmorHeadNames_162, gArmorHeadNames_163,
    gArmorHeadNames_164, gArmorHeadNames_165, gArmorHeadNames_166, gArmorHeadNames_167,
    gArmorHeadNames_168, gArmorHeadNames_169, gArmorHeadNames_170, gArmorHeadNames_171,
    gArmorHeadNames_172, gArmorHeadNames_173, gArmorHeadNames_174, gArmorHeadNames_175,
    gArmorHeadNames_176, gArmorHeadNames_177, gArmorHeadNames_178, gArmorHeadNames_179,
    gArmorHeadNames_180, gArmorHeadNames_181, gArmorHeadNames_182, gArmorHeadNames_183,
    gArmorHeadNames_184, gArmorHeadNames_185, gArmorHeadNames_186, gArmorHeadNames_187,
    gArmorHeadNames_188, gArmorHeadNames_189, gArmorHeadNames_190, gArmorHeadNames_191,
    gArmorHeadNames_192, gArmorHeadNames_193, gArmorHeadNames_194, gArmorHeadNames_195,
    gArmorHeadNames_196, gArmorHeadNames_197, gArmorHeadNames_198, gArmorHeadNames_199,
    gArmorHeadNames_200, gArmorHeadNames_201, gArmorHeadNames_202, gArmorHeadNames_203,
    gArmorHeadNames_204, gArmorHeadNames_205, gArmorHeadNames_206, gArmorHeadNames_207,
    gArmorHeadNames_208, gArmorHeadNames_209, gArmorHeadNames_210, gArmorHeadNames_211,
    gArmorHeadNames_212, gArmorHeadNames_213, gArmorHeadNames_214, gArmorHeadNames_215,
    gArmorHeadNames_216, gArmorHeadNames_217, gArmorHeadNames_218, gArmorHeadNames_219,
    gArmorHeadNames_220, gArmorHeadNames_221, gArmorHeadNames_222, gArmorHeadNames_223,
    gArmorHeadNames_224, gArmorHeadNames_225, gArmorHeadNames_226, gArmorHeadNames_227,
    gArmorHeadNames_228, gArmorHeadNames_229, gArmorHeadNames_230, gArmorHeadNames_231,
    gArmorHeadNames_232, gArmorHeadNames_233, gArmorHeadNames_234, gArmorHeadNames_235,
    gArmorHeadNames_236, gArmorHeadNames_237, gArmorHeadNames_238, gArmorHeadNames_239,
    gArmorHeadNames_240, gArmorHeadNames_241, gArmorHeadNames_242, gArmorHeadNames_243,
    gArmorHeadNames_244, gArmorHeadNames_245, gArmorHeadNames_246, gArmorHeadNames_247,
    gArmorHeadNames_248, gArmorHeadNames_249, gArmorHeadNames_250, gArmorHeadNames_251,
    gArmorHeadNames_252, 0, 0, 0,
};
char *gArmorBodyNames[244] = {
    D_004A00E8, gArmorBodyNames_1, gArmorBodyNames_2, gArmorBodyNames_3,
    gArmorBodyNames_4, gArmorBodyNames_5, gArmorBodyNames_6, gArmorBodyNames_7,
    gArmorBodyNames_8, gArmorBodyNames_9, gArmorBodyNames_10, gArmorBodyNames_11,
    gArmorBodyNames_12, gArmorBodyNames_13, gArmorBodyNames_14, gArmorBodyNames_15,
    gArmorBodyNames_16, gArmorBodyNames_17, gArmorBodyNames_18, gArmorBodyNames_19,
    gArmorBodyNames_20, gArmorBodyNames_21, gArmorBodyNames_22, gArmorBodyNames_23,
    gArmorBodyNames_24, gArmorBodyNames_25, gArmorBodyNames_26, gArmorBodyNames_27,
    gArmorBodyNames_28, gArmorBodyNames_29, gArmorBodyNames_30, gArmorBodyNames_31,
    gArmorBodyNames_32, gArmorBodyNames_33, gArmorBodyNames_34, gArmorBodyNames_35,
    gArmorBodyNames_36, gArmorBodyNames_37, gArmorBodyNames_38, gArmorBodyNames_39,
    gArmorBodyNames_40, gArmorBodyNames_41, gArmorBodyNames_42, gArmorBodyNames_43,
    gArmorBodyNames_44, gArmorBodyNames_45, gArmorBodyNames_46, gArmorBodyNames_47,
    gArmorBodyNames_48, gArmorBodyNames_49, gArmorBodyNames_50, gArmorBodyNames_51,
    gArmorBodyNames_52, gArmorBodyNames_53, gArmorBodyNames_54, gArmorBodyNames_55,
    gArmorBodyNames_56, gArmorBodyNames_57, gArmorBodyNames_58, gArmorBodyNames_59,
    gArmorBodyNames_60, gArmorBodyNames_61, gArmorBodyNames_62, gArmorBodyNames_63,
    gArmorBodyNames_64, gArmorBodyNames_65, gArmorBodyNames_66, gArmorBodyNames_67,
    gArmorBodyNames_68, gArmorBodyNames_69, gArmorBodyNames_70, gArmorBodyNames_71,
    gArmorBodyNames_72, gArmorBodyNames_73, gArmorBodyNames_74, gArmorBodyNames_75,
    gArmorBodyNames_76, gArmorBodyNames_77, gArmorBodyNames_78, gArmorBodyNames_79,
    gArmorBodyNames_80, gArmorBodyNames_81, gArmorBodyNames_82, gArmorBodyNames_83,
    gArmorBodyNames_84, gArmorBodyNames_85, gArmorBodyNames_86, gArmorBodyNames_87,
    gArmorBodyNames_88, gArmorBodyNames_89, gArmorBodyNames_90, gArmorBodyNames_91,
    gArmorBodyNames_92, gArmorBodyNames_93, gArmorBodyNames_94, gArmorBodyNames_95,
    gArmorBodyNames_96, gArmorBodyNames_97, gArmorBodyNames_98, gArmorBodyNames_99,
    gArmorBodyNames_100, gArmorBodyNames_101, gArmorBodyNames_102, gArmorBodyNames_103,
    gArmorBodyNames_104, gArmorBodyNames_105, gArmorBodyNames_106, gArmorBodyNames_107,
    gArmorBodyNames_108, gArmorBodyNames_109, gArmorBodyNames_110, gArmorBodyNames_111,
    gArmorBodyNames_112, gArmorBodyNames_113, gArmorBodyNames_114, gArmorBodyNames_115,
    gArmorBodyNames_116, gArmorBodyNames_117, gArmorBodyNames_118, gArmorBodyNames_119,
    gArmorBodyNames_120, gArmorBodyNames_121, gArmorBodyNames_122, gArmorBodyNames_123,
    gArmorBodyNames_124, gArmorBodyNames_125, gArmorBodyNames_126, gArmorBodyNames_127,
    gArmorBodyNames_128, gArmorBodyNames_129, gArmorBodyNames_130, gArmorBodyNames_131,
    gArmorBodyNames_132, gArmorBodyNames_133, gArmorBodyNames_134, gArmorBodyNames_135,
    gArmorBodyNames_136, gArmorBodyNames_137, gArmorBodyNames_138, gArmorBodyNames_139,
    gArmorBodyNames_140, gArmorBodyNames_141, gArmorBodyNames_142, gArmorBodyNames_143,
    gArmorBodyNames_144, gArmorBodyNames_145, gArmorBodyNames_146, gArmorBodyNames_147,
    gArmorBodyNames_148, gArmorBodyNames_149, gArmorBodyNames_150, gArmorBodyNames_151,
    gArmorBodyNames_152, gArmorBodyNames_153, gArmorBodyNames_154, gArmorBodyNames_155,
    gArmorBodyNames_156, gArmorBodyNames_157, gArmorBodyNames_158, gArmorBodyNames_159,
    gArmorBodyNames_160, gArmorBodyNames_161, gArmorBodyNames_162, gArmorBodyNames_163,
    gArmorBodyNames_164, gArmorBodyNames_165, gArmorBodyNames_166, gArmorBodyNames_167,
    gArmorBodyNames_168, gArmorBodyNames_169, gArmorBodyNames_170, gArmorBodyNames_171,
    D_004A6220, D_004A6240, D_004A6220, D_004A6240,
    gArmorBodyNames_176, gArmorBodyNames_177, gArmorBodyNames_178, gArmorBodyNames_179,
    gArmorBodyNames_180, gArmorBodyNames_181, gArmorBodyNames_182, gArmorBodyNames_183,
    gArmorBodyNames_184, gArmorBodyNames_185, gArmorBodyNames_186, gArmorBodyNames_187,
    gArmorBodyNames_188, gArmorBodyNames_189, gArmorBodyNames_190, gArmorBodyNames_191,
    gArmorBodyNames_192, gArmorBodyNames_193, gArmorBodyNames_194, gArmorBodyNames_195,
    gArmorBodyNames_196, gArmorBodyNames_197, gArmorBodyNames_198, gArmorBodyNames_199,
    gArmorBodyNames_200, gArmorBodyNames_201, gArmorBodyNames_202, gArmorBodyNames_203,
    gArmorBodyNames_204, gArmorBodyNames_205, gArmorBodyNames_206, gArmorBodyNames_207,
    gArmorBodyNames_208, gArmorBodyNames_209, gArmorBodyNames_210, gArmorBodyNames_211,
    gArmorBodyNames_212, gArmorBodyNames_213, gArmorBodyNames_214, gArmorBodyNames_215,
    gArmorBodyNames_216, gArmorBodyNames_217, gArmorBodyNames_218, gArmorBodyNames_219,
    gArmorBodyNames_220, gArmorBodyNames_221, gArmorBodyNames_222, gArmorBodyNames_223,
    gArmorBodyNames_224, gArmorBodyNames_225, gArmorBodyNames_226, gArmorBodyNames_227,
    gArmorBodyNames_228, gArmorBodyNames_229, gArmorBodyNames_230, gArmorBodyNames_231,
    gArmorBodyNames_232, gArmorBodyNames_233, gArmorBodyNames_234, gArmorBodyNames_235,
    D_004A6830, D_004A6850, D_004A6830, D_004A6850,
    gArmorBodyNames_240, gArmorBodyNames_241, gArmorBodyNames_242, 0,
};
char *gArmorArmsNames[240] = {
    D_004A00E8, gArmorArmsNames_1, gArmorArmsNames_2, gArmorArmsNames_3,
    gArmorArmsNames_4, gArmorArmsNames_5, gArmorArmsNames_6, gArmorArmsNames_7,
    gArmorArmsNames_8, gArmorArmsNames_9, gArmorArmsNames_10, gArmorArmsNames_11,
    gArmorArmsNames_12, gArmorArmsNames_13, gArmorArmsNames_14, gArmorArmsNames_15,
    gArmorArmsNames_16, gArmorArmsNames_17, gArmorArmsNames_18, gArmorArmsNames_19,
    gArmorArmsNames_20, gArmorArmsNames_21, gArmorArmsNames_22, gArmorArmsNames_23,
    gArmorArmsNames_24, gArmorArmsNames_25, gArmorArmsNames_26, gArmorArmsNames_27,
    gArmorArmsNames_28, gArmorArmsNames_29, gArmorArmsNames_30, gArmorArmsNames_31,
    gArmorArmsNames_32, gArmorArmsNames_33, gArmorArmsNames_34, gArmorArmsNames_35,
    gArmorArmsNames_36, gArmorArmsNames_37, gArmorArmsNames_38, gArmorArmsNames_39,
    gArmorArmsNames_40, gArmorArmsNames_41, gArmorArmsNames_42, gArmorArmsNames_43,
    gArmorArmsNames_44, gArmorArmsNames_45, gArmorArmsNames_46, gArmorArmsNames_47,
    gArmorArmsNames_48, gArmorArmsNames_49, gArmorArmsNames_50, gArmorArmsNames_51,
    gArmorArmsNames_52, gArmorArmsNames_53, gArmorArmsNames_54, gArmorArmsNames_55,
    gArmorArmsNames_56, gArmorArmsNames_57, gArmorArmsNames_58, gArmorArmsNames_59,
    gArmorArmsNames_60, gArmorArmsNames_61, gArmorArmsNames_62, gArmorArmsNames_63,
    gArmorArmsNames_64, gArmorArmsNames_65, gArmorArmsNames_66, gArmorArmsNames_67,
    gArmorArmsNames_68, gArmorArmsNames_69, gArmorArmsNames_70, gArmorArmsNames_71,
    gArmorArmsNames_72, gArmorArmsNames_73, gArmorArmsNames_74, gArmorArmsNames_75,
    gArmorArmsNames_76, gArmorArmsNames_77, gArmorArmsNames_78, gArmorArmsNames_79,
    gArmorArmsNames_80, gArmorArmsNames_81, gArmorArmsNames_82, gArmorArmsNames_83,
    gArmorArmsNames_84, gArmorArmsNames_85, gArmorArmsNames_86, gArmorArmsNames_87,
    gArmorArmsNames_88, gArmorArmsNames_89, gArmorArmsNames_90, gArmorArmsNames_91,
    gArmorArmsNames_92, gArmorArmsNames_93, gArmorArmsNames_94, gArmorArmsNames_95,
    gArmorArmsNames_96, gArmorArmsNames_97, gArmorArmsNames_98, gArmorArmsNames_99,
    gArmorArmsNames_100, gArmorArmsNames_101, gArmorArmsNames_102, gArmorArmsNames_103,
    gArmorArmsNames_104, gArmorArmsNames_105, gArmorArmsNames_106, gArmorArmsNames_107,
    gArmorArmsNames_108, gArmorArmsNames_109, gArmorArmsNames_110, gArmorArmsNames_111,
    gArmorArmsNames_112, gArmorArmsNames_113, gArmorArmsNames_114, gArmorArmsNames_115,
    gArmorArmsNames_116, gArmorArmsNames_117, gArmorArmsNames_118, gArmorArmsNames_119,
    gArmorArmsNames_120, gArmorArmsNames_121, gArmorArmsNames_122, gArmorArmsNames_123,
    gArmorArmsNames_124, gArmorArmsNames_125, gArmorArmsNames_126, gArmorArmsNames_127,
    gArmorArmsNames_128, gArmorArmsNames_129, gArmorArmsNames_130, gArmorArmsNames_131,
    gArmorArmsNames_132, gArmorArmsNames_133, gArmorArmsNames_134, gArmorArmsNames_135,
    gArmorArmsNames_136, gArmorArmsNames_137, gArmorArmsNames_138, gArmorArmsNames_139,
    gArmorArmsNames_140, gArmorArmsNames_141, gArmorArmsNames_142, gArmorArmsNames_143,
    gArmorArmsNames_144, gArmorArmsNames_145, gArmorArmsNames_146, gArmorArmsNames_147,
    gArmorArmsNames_148, gArmorArmsNames_149, gArmorArmsNames_150, gArmorArmsNames_151,
    gArmorArmsNames_152, gArmorArmsNames_153, gArmorArmsNames_154, gArmorArmsNames_155,
    gArmorArmsNames_156, gArmorArmsNames_157, gArmorArmsNames_158, gArmorArmsNames_159,
    gArmorArmsNames_160, gArmorArmsNames_161, gArmorArmsNames_162, gArmorArmsNames_163,
    gArmorArmsNames_164, gArmorArmsNames_165, gArmorArmsNames_166, gArmorArmsNames_167,
    gArmorArmsNames_168, gArmorArmsNames_169, D_004A7850, D_004A7870,
    D_004A7850, D_004A7870, gArmorArmsNames_174, gArmorArmsNames_175,
    gArmorArmsNames_176, gArmorArmsNames_177, gArmorArmsNames_178, gArmorArmsNames_179,
    gArmorArmsNames_180, gArmorArmsNames_181, gArmorArmsNames_182, gArmorArmsNames_183,
    gArmorArmsNames_184, gArmorArmsNames_185, gArmorArmsNames_186, gArmorArmsNames_187,
    gArmorArmsNames_188, gArmorArmsNames_189, gArmorArmsNames_190, gArmorArmsNames_191,
    gArmorArmsNames_192, gArmorArmsNames_193, gArmorArmsNames_194, gArmorArmsNames_195,
    gArmorArmsNames_196, gArmorArmsNames_197, gArmorArmsNames_198, gArmorArmsNames_199,
    gArmorArmsNames_200, gArmorArmsNames_201, gArmorArmsNames_202, gArmorArmsNames_203,
    gArmorArmsNames_204, gArmorArmsNames_205, gArmorArmsNames_206, gArmorArmsNames_207,
    gArmorArmsNames_208, gArmorArmsNames_209, gArmorArmsNames_210, gArmorArmsNames_211,
    gArmorArmsNames_212, gArmorArmsNames_213, gArmorArmsNames_214, gArmorArmsNames_215,
    gArmorArmsNames_216, gArmorArmsNames_217, gArmorArmsNames_218, gArmorArmsNames_219,
    gArmorArmsNames_220, gArmorArmsNames_221, gArmorArmsNames_222, gArmorArmsNames_223,
    gArmorArmsNames_224, gArmorArmsNames_225, gArmorArmsNames_226, gArmorArmsNames_227,
    gArmorArmsNames_228, gArmorArmsNames_229, gArmorArmsNames_230, D_004A7DA0,
    D_004A7DC0, D_004A7DA0, D_004A7DC0, gArmorArmsNames_235,
    gArmorArmsNames_236, gArmorArmsNames_237, 0, 0,
};
char *gArmorWaistNames[240] = {
    D_004A00E8, gArmorWaistNames_1, gArmorWaistNames_2, gArmorWaistNames_3,
    gArmorWaistNames_4, gArmorWaistNames_5, gArmorWaistNames_6, gArmorWaistNames_7,
    gArmorWaistNames_8, gArmorWaistNames_9, gArmorWaistNames_10, gArmorWaistNames_11,
    gArmorWaistNames_12, gArmorWaistNames_13, gArmorWaistNames_14, gArmorWaistNames_15,
    gArmorWaistNames_16, gArmorWaistNames_17, gArmorWaistNames_18, gArmorWaistNames_19,
    gArmorWaistNames_20, gArmorWaistNames_21, gArmorWaistNames_22, gArmorWaistNames_23,
    gArmorWaistNames_24, gArmorWaistNames_25, gArmorWaistNames_26, gArmorWaistNames_27,
    gArmorWaistNames_28, gArmorWaistNames_29, gArmorWaistNames_30, gArmorWaistNames_31,
    gArmorWaistNames_32, gArmorWaistNames_33, gArmorWaistNames_34, gArmorWaistNames_35,
    gArmorWaistNames_36, gArmorWaistNames_37, gArmorWaistNames_38, gArmorWaistNames_39,
    gArmorWaistNames_40, gArmorWaistNames_41, gArmorWaistNames_42, gArmorWaistNames_43,
    gArmorWaistNames_44, gArmorWaistNames_45, gArmorWaistNames_46, gArmorWaistNames_47,
    gArmorWaistNames_48, gArmorWaistNames_49, gArmorWaistNames_50, gArmorWaistNames_51,
    gArmorWaistNames_52, gArmorWaistNames_53, gArmorWaistNames_54, gArmorWaistNames_55,
    gArmorWaistNames_56, gArmorWaistNames_57, gArmorWaistNames_58, gArmorWaistNames_59,
    gArmorWaistNames_60, gArmorWaistNames_61, gArmorWaistNames_62, gArmorWaistNames_63,
    gArmorWaistNames_64, gArmorWaistNames_65, gArmorWaistNames_66, gArmorWaistNames_67,
    gArmorWaistNames_68, gArmorWaistNames_69, gArmorWaistNames_70, gArmorWaistNames_71,
    gArmorWaistNames_72, gArmorWaistNames_73, gArmorWaistNames_74, gArmorWaistNames_75,
    gArmorWaistNames_76, gArmorWaistNames_77, gArmorWaistNames_78, gArmorWaistNames_79,
    gArmorWaistNames_80, gArmorWaistNames_81, gArmorWaistNames_82, gArmorWaistNames_83,
    gArmorWaistNames_84, gArmorWaistNames_85, gArmorWaistNames_86, gArmorWaistNames_87,
    gArmorWaistNames_88, gArmorWaistNames_89, gArmorWaistNames_90, gArmorWaistNames_91,
    gArmorWaistNames_92, gArmorWaistNames_93, gArmorWaistNames_94, gArmorWaistNames_95,
    gArmorWaistNames_96, gArmorWaistNames_97, gArmorWaistNames_98, gArmorWaistNames_99,
    gArmorWaistNames_100, gArmorWaistNames_101, gArmorWaistNames_102, gArmorWaistNames_103,
    gArmorWaistNames_104, gArmorWaistNames_105, gArmorWaistNames_106, gArmorWaistNames_107,
    gArmorWaistNames_108, gArmorWaistNames_109, gArmorWaistNames_110, gArmorWaistNames_111,
    gArmorWaistNames_112, gArmorWaistNames_113, gArmorWaistNames_114, gArmorWaistNames_115,
    gArmorWaistNames_116, gArmorWaistNames_117, gArmorWaistNames_118, gArmorWaistNames_119,
    gArmorWaistNames_120, gArmorWaistNames_121, gArmorWaistNames_122, gArmorWaistNames_123,
    gArmorWaistNames_124, gArmorWaistNames_125, gArmorWaistNames_126, gArmorWaistNames_127,
    gArmorWaistNames_128, gArmorWaistNames_129, gArmorWaistNames_130, gArmorWaistNames_131,
    gArmorWaistNames_132, gArmorWaistNames_133, gArmorWaistNames_134, gArmorWaistNames_135,
    gArmorWaistNames_136, gArmorWaistNames_137, gArmorWaistNames_138, gArmorWaistNames_139,
    gArmorWaistNames_140, gArmorWaistNames_141, gArmorWaistNames_142, gArmorWaistNames_143,
    gArmorWaistNames_144, gArmorWaistNames_145, gArmorWaistNames_146, gArmorWaistNames_147,
    gArmorWaistNames_148, gArmorWaistNames_149, gArmorWaistNames_150, gArmorWaistNames_151,
    gArmorWaistNames_152, gArmorWaistNames_153, gArmorWaistNames_154, gArmorWaistNames_155,
    gArmorWaistNames_156, gArmorWaistNames_157, gArmorWaistNames_158, gArmorWaistNames_159,
    gArmorWaistNames_160, gArmorWaistNames_161, gArmorWaistNames_162, gArmorWaistNames_163,
    gArmorWaistNames_164, gArmorWaistNames_165, D_004A8F90, D_004A8FB0,
    D_004A8F90, D_004A8FB0, gArmorWaistNames_170, gArmorWaistNames_171,
    gArmorWaistNames_172, gArmorWaistNames_173, gArmorWaistNames_174, gArmorWaistNames_175,
    gArmorWaistNames_176, gArmorWaistNames_177, gArmorWaistNames_178, gArmorWaistNames_179,
    gArmorWaistNames_180, gArmorWaistNames_181, gArmorWaistNames_182, gArmorWaistNames_183,
    gArmorWaistNames_184, gArmorWaistNames_185, gArmorWaistNames_186, gArmorWaistNames_187,
    gArmorWaistNames_188, gArmorWaistNames_189, gArmorWaistNames_190, gArmorWaistNames_191,
    gArmorWaistNames_192, gArmorWaistNames_193, gArmorWaistNames_194, gArmorWaistNames_195,
    gArmorWaistNames_196, gArmorWaistNames_197, gArmorWaistNames_198, gArmorWaistNames_199,
    gArmorWaistNames_200, gArmorWaistNames_201, gArmorWaistNames_202, gArmorWaistNames_203,
    gArmorWaistNames_204, gArmorWaistNames_205, gArmorWaistNames_206, gArmorWaistNames_207,
    gArmorWaistNames_208, gArmorWaistNames_209, gArmorWaistNames_210, gArmorWaistNames_211,
    gArmorWaistNames_212, gArmorWaistNames_213, gArmorWaistNames_214, gArmorWaistNames_215,
    gArmorWaistNames_216, gArmorWaistNames_217, gArmorWaistNames_218, gArmorWaistNames_219,
    gArmorWaistNames_220, gArmorWaistNames_221, gArmorWaistNames_222, gArmorWaistNames_223,
    gArmorWaistNames_224, gArmorWaistNames_225, gArmorWaistNames_226, gArmorWaistNames_227,
    D_004A9580, D_004A95A0, D_004A9580, D_004A95A0,
    gArmorWaistNames_232, gArmorWaistNames_233, gArmorWaistNames_234, gArmorWaistNames_235,
    gArmorWaistNames_236, 0, 0, 0,
};
char *gArmorLegsNames[236] = {
    D_004A00E8, gArmorLegsNames_1, gArmorLegsNames_2, gArmorLegsNames_3,
    gArmorLegsNames_4, gArmorLegsNames_5, gArmorLegsNames_6, gArmorLegsNames_7,
    gArmorLegsNames_8, gArmorLegsNames_9, gArmorLegsNames_10, gArmorLegsNames_11,
    gArmorLegsNames_12, gArmorLegsNames_13, gArmorLegsNames_14, gArmorLegsNames_15,
    gArmorLegsNames_16, gArmorLegsNames_17, gArmorLegsNames_18, gArmorLegsNames_19,
    gArmorLegsNames_20, gArmorLegsNames_21, gArmorLegsNames_22, gArmorLegsNames_23,
    gArmorLegsNames_24, gArmorLegsNames_25, gArmorLegsNames_26, gArmorLegsNames_27,
    gArmorLegsNames_28, gArmorLegsNames_29, gArmorLegsNames_30, gArmorLegsNames_31,
    gArmorLegsNames_32, gArmorLegsNames_33, gArmorLegsNames_34, gArmorLegsNames_35,
    gArmorLegsNames_36, gArmorLegsNames_37, gArmorLegsNames_38, gArmorLegsNames_39,
    gArmorLegsNames_40, gArmorLegsNames_41, gArmorLegsNames_42, gArmorLegsNames_43,
    gArmorLegsNames_44, gArmorLegsNames_45, gArmorLegsNames_46, gArmorLegsNames_47,
    gArmorLegsNames_48, gArmorLegsNames_49, gArmorLegsNames_50, gArmorLegsNames_51,
    gArmorLegsNames_52, gArmorLegsNames_53, gArmorLegsNames_54, gArmorLegsNames_55,
    gArmorLegsNames_56, gArmorLegsNames_57, gArmorLegsNames_58, gArmorLegsNames_59,
    gArmorLegsNames_60, gArmorLegsNames_61, gArmorLegsNames_62, gArmorLegsNames_63,
    gArmorLegsNames_64, gArmorLegsNames_65, gArmorLegsNames_66, gArmorLegsNames_67,
    gArmorLegsNames_68, gArmorLegsNames_69, gArmorLegsNames_70, gArmorLegsNames_71,
    gArmorLegsNames_72, gArmorLegsNames_73, gArmorLegsNames_74, gArmorLegsNames_75,
    gArmorLegsNames_76, gArmorLegsNames_77, gArmorLegsNames_78, gArmorLegsNames_79,
    gArmorLegsNames_80, gArmorLegsNames_81, gArmorLegsNames_82, gArmorLegsNames_83,
    gArmorLegsNames_84, gArmorLegsNames_85, gArmorLegsNames_86, gArmorLegsNames_87,
    gArmorLegsNames_88, gArmorLegsNames_89, gArmorLegsNames_90, gArmorLegsNames_91,
    gArmorLegsNames_92, gArmorLegsNames_93, gArmorLegsNames_94, gArmorLegsNames_95,
    gArmorLegsNames_96, gArmorLegsNames_97, gArmorLegsNames_98, gArmorLegsNames_99,
    gArmorLegsNames_100, gArmorLegsNames_101, gArmorLegsNames_102, gArmorLegsNames_103,
    gArmorLegsNames_104, gArmorLegsNames_105, gArmorLegsNames_106, gArmorLegsNames_107,
    gArmorLegsNames_108, gArmorLegsNames_109, gArmorLegsNames_110, gArmorLegsNames_111,
    gArmorLegsNames_112, gArmorLegsNames_113, gArmorLegsNames_114, gArmorLegsNames_115,
    gArmorLegsNames_116, gArmorLegsNames_117, gArmorLegsNames_118, gArmorLegsNames_119,
    gArmorLegsNames_120, gArmorLegsNames_121, gArmorLegsNames_122, gArmorLegsNames_123,
    gArmorLegsNames_124, gArmorLegsNames_125, gArmorLegsNames_126, gArmorLegsNames_127,
    gArmorLegsNames_128, gArmorLegsNames_129, gArmorLegsNames_130, gArmorLegsNames_131,
    gArmorLegsNames_132, gArmorLegsNames_133, gArmorLegsNames_134, gArmorLegsNames_135,
    gArmorLegsNames_136, gArmorLegsNames_137, gArmorLegsNames_138, gArmorLegsNames_139,
    gArmorLegsNames_140, gArmorLegsNames_141, gArmorLegsNames_142, gArmorLegsNames_143,
    gArmorLegsNames_144, gArmorLegsNames_145, gArmorLegsNames_146, gArmorLegsNames_147,
    gArmorLegsNames_148, gArmorLegsNames_149, gArmorLegsNames_150, gArmorLegsNames_151,
    gArmorLegsNames_152, gArmorLegsNames_153, gArmorLegsNames_154, gArmorLegsNames_155,
    gArmorLegsNames_156, gArmorLegsNames_157, gArmorLegsNames_158, gArmorLegsNames_159,
    gArmorLegsNames_160, gArmorLegsNames_161, gArmorLegsNames_162, gArmorLegsNames_163,
    gArmorLegsNames_164, gArmorLegsNames_165, D_004AA8D0, D_004AA8F0,
    D_004AA8D0, D_004AA8F0, gArmorLegsNames_170, gArmorLegsNames_171,
    gArmorLegsNames_172, gArmorLegsNames_173, gArmorLegsNames_174, gArmorLegsNames_175,
    gArmorLegsNames_176, gArmorLegsNames_177, gArmorLegsNames_178, gArmorLegsNames_179,
    gArmorLegsNames_180, gArmorLegsNames_181, gArmorLegsNames_182, gArmorLegsNames_183,
    gArmorLegsNames_184, gArmorLegsNames_185, gArmorLegsNames_186, gArmorLegsNames_187,
    gArmorLegsNames_188, gArmorLegsNames_189, gArmorLegsNames_190, gArmorLegsNames_191,
    gArmorLegsNames_192, gArmorLegsNames_193, gArmorLegsNames_194, gArmorLegsNames_195,
    gArmorLegsNames_196, gArmorLegsNames_197, gArmorLegsNames_198, gArmorLegsNames_199,
    gArmorLegsNames_200, gArmorLegsNames_201, gArmorLegsNames_202, gArmorLegsNames_203,
    gArmorLegsNames_204, gArmorLegsNames_205, gArmorLegsNames_206, gArmorLegsNames_207,
    gArmorLegsNames_208, gArmorLegsNames_209, gArmorLegsNames_210, gArmorLegsNames_211,
    gArmorLegsNames_212, gArmorLegsNames_213, gArmorLegsNames_214, gArmorLegsNames_215,
    gArmorLegsNames_216, gArmorLegsNames_217, gArmorLegsNames_218, gArmorLegsNames_219,
    gArmorLegsNames_220, gArmorLegsNames_221, gArmorLegsNames_222, gArmorLegsNames_223,
    gArmorLegsNames_224, gArmorLegsNames_225, gArmorLegsNames_226, D_004AAEF0,
    D_004AAF10, D_004AAEF0, D_004AAF10, gArmorLegsNames_231,
    gArmorLegsNames_232, gArmorLegsNames_233, gArmorLegsNames_234, gArmorLegsNames_235,
};
u8 gWeaponKindClass[352] = {
#include "sub_main/0045DF80.inc"
};
u8 gAmmoLevels[36][4] = {
#include "sub_main/0045E0E0.inc"
};
u8 gBowgunClipSizes[33][32] = {
#include "sub_main/0045E170.inc"
};
s16 gBowgunClipShots[30][4] = {
#include "sub_main/0045E590.inc"
};
s16 gSharpnessTables_0[8][6] = {
#include "sub_main/0045E680.inc"
};
s16 gSharpnessTables_2[8][6] = {
#include "sub_main/0045E6E0.inc"
};
s16 gSharpnessTables_3[8][6] = {
#include "sub_main/0045E740.inc"
};
s16 gSharpnessTables_4[8][6] = {
#include "sub_main/0045E7A0.inc"
};
s16 gSharpnessTables_6[8][6] = {
#include "sub_main/0045E800.inc"
};
SharpRow *gSharpnessTables[12] = {
    gSharpnessTables_0, gSharpnessTables_0, gSharpnessTables_2, gSharpnessTables_3,
    gSharpnessTables_4, gSharpnessTables_0, gSharpnessTables_6, gSharpnessTables_0,
    gSharpnessTables_2, gSharpnessTables_3, gSharpnessTables_0, NULL,
};
ArmorData *gArmorTables[16] = {
    gArmorLegsData, gArmorHeadData, gArmorHeadData, gArmorBodyData,
    gArmorArmsData, gArmorWaistData, NULL, NULL,
    (ArmorData *)gArmorLegsNames, NULL, (ArmorData *)gArmorHeadNames, (ArmorData *)gArmorBodyNames,
    (ArmorData *)gArmorArmsNames, (ArmorData *)gArmorWaistNames, (ArmorData *)gBladeNames, (ArmorData *)gGunNames,
};
u32 gArmorNameTables[8] = {
    (u32)gArmorHeadNames, (u32)gArmorBodyNames, (u32)gArmorArmsNames, (u32)gArmorWaistNames,
    (u32)gArmorLegsNames, 0, 0, 0,
};
s16 D_0045E8F0[64] = {
#include "sub_main/0045E8F0.inc"
};
s16 D_0045E970[64] = {
#include "sub_main/0045E970.inc"
};
s16 D_0045E9F0[64] = {
#include "sub_main/0045E9F0.inc"
};
s16 D_0045EA70[64] = {
#include "sub_main/0045EA70.inc"
};
GunLevelRow gGunLevelRows[4] = {
    {D_0045E8F0}, {D_0045E970}, {D_0045E9F0}, {D_0045EA70},
};
s16 gBarrelBonusLight[16] = {
#include "sub_main/0045EB00.inc"
};
s16 gBarrelBonusHeavy[16] = {
#include "sub_main/0045EB20.inc"
};
f32 gArmorLevelDefense[80] = {
#include "sub_main/0045EB40.inc"
};
f32 gArmorLevelCost[80] = {
#include "sub_main/0045EC80.inc"
};
u8 gArmorLevelSlots[64] = {
#include "sub_main/0045EDC0.inc"
};
u8 gWeaponClassAttack[136][2] = {
#include "sub_main/0045EE00.inc"
};
s16 gHitSeIds[88] = {
#include "sub_main/0045EF10.inc"
};
f32 gHitKindMvFactors[24] = {
#include "sub_main/0045EFC0.inc"
};
f32 gSwordGaugeHitFactors[4] = {
#include "sub_main/0045F020.inc"
};
f32 gSharpnessRawMods[12] = {
#include "sub_main/0045F030.inc"
};
s16 gSharpnessLossByHit[8] = {
#include "sub_main/0045F060.inc"
};
AreaKindRow gAreaKindFlags_170[2] = {
#include "sub_main/0045F070.inc"
};
AreaLightZone gAreaLightZones_170[2] = {
#include "sub_main/0045F080.inc"
};
f32 gAreaLightColors_170[6][4] = {
#include "sub_main/0045F0B0.inc"
};
ItemData gItemData[921] = {
#include "sub_main/0045F110.inc"
};
u32 gItemNames[924] = {
    (u32)D_004AB180, (u32)D_004AB190, (u32)D_004AB1A0, (u32)D_004AB1B0,
    (u32)D_004AB1C0, (u32)D_004AB1D0, (u32)D_004AB1E0, (u32)D_004AB1F8,
    (u32)D_004AB200, (u32)D_004AB210, (u32)D_004AB218, (u32)D_004AB228,
    (u32)D_004AB230, (u32)D_004AB238, (u32)D_004AB240, (u32)D_004AB250,
    (u32)D_004AB258, (u32)D_004AB268, (u32)D_004AB278, (u32)D_004AB280,
    (u32)D_004AB290, (u32)D_004AB2A0, (u32)D_004AB2B8, (u32)D_004AB2C8,
    (u32)D_004AB2E0, (u32)D_004AB300, (u32)D_004AB318, (u32)D_004AB320,
    (u32)D_004AB328, (u32)D_004AB338, (u32)D_004AB340, (u32)D_004AB350,
    (u32)D_004AB360, (u32)D_004AB368, (u32)D_004AB378, (u32)D_004AB388,
    (u32)D_004AB398, (u32)D_004AB3A0, (u32)D_004AB3B0, (u32)D_004AB3C0,
    (u32)D_004AB3D0, (u32)D_004AB3D8, (u32)D_004AB3E0, (u32)D_004AB3F0,
    (u32)D_004AB400, (u32)D_004AB408, (u32)D_004AB418, (u32)D_004AB428,
    (u32)D_004AB438, (u32)D_004AB448, (u32)D_004AB458, (u32)D_004AB460,
    (u32)D_004AB470, (u32)D_004AB480, (u32)D_004AB490, (u32)D_004AB4A8,
    (u32)D_004AB4B8, (u32)D_004AB4C8, (u32)D_004AB4D8, (u32)D_004AB4E8,
    (u32)D_004AB4F8, (u32)D_004AB510, (u32)D_004AB528, (u32)D_004AB350,
    (u32)D_004AB530, (u32)D_004AB540, (u32)D_004AB550, (u32)D_004AB560,
    (u32)D_004AB570, (u32)D_004AB580, (u32)D_004AB598, (u32)D_004AB5A0,
    (u32)D_004AB5B0, (u32)D_004AB5B8, (u32)D_004AB5C0, (u32)D_004AB5D8,
    (u32)D_004AB5E8, (u32)D_004AB5F8, (u32)D_004AB608, (u32)D_004AB618,
    (u32)D_004AB620, (u32)D_004AB630, (u32)D_004AB640, (u32)D_004AB648,
    (u32)D_004AB658, (u32)D_004AB668, (u32)D_004AB678, (u32)D_004AB350,
    (u32)D_004AB680, (u32)D_004AB690, (u32)D_004AB698, (u32)D_004AB6A0,
    (u32)D_004AB6B0, (u32)D_004AB6C0, (u32)D_004AB6D0, (u32)D_004AB6E0,
    (u32)D_004AB6F0, (u32)D_004AB708, (u32)D_004AB718, (u32)D_004AB728,
    (u32)D_004AB738, (u32)D_004AB748, (u32)D_004AB758, (u32)D_004AB768,
    (u32)D_004AB778, (u32)D_004AB788, (u32)D_004AB798, (u32)D_004AB7A8,
    (u32)D_004AB7B8, (u32)D_004AB7C8, (u32)D_004AB7D8, (u32)D_004AB7E8,
    (u32)D_004AB7F8, (u32)D_004AB808, (u32)D_004AB818, (u32)D_004AB828,
    (u32)D_004AB838, (u32)D_004AB848, (u32)D_004AB850, (u32)D_004AB858,
    (u32)D_004AB860, (u32)D_004AB868, (u32)D_004AB870, (u32)D_004AB880,
    (u32)D_004AB890, (u32)D_004AB8A0, (u32)D_004AB8B0, (u32)D_004AB8C0,
    (u32)D_004AB8D0, (u32)D_004AB8E0, (u32)D_004AB8F0, (u32)D_004AB900,
    (u32)D_004AB910, (u32)D_004AB918, (u32)D_004AB350, (u32)D_004AB920,
    (u32)D_004AB930, (u32)D_004AB940, (u32)D_004AB950, (u32)D_004AB958,
    (u32)D_004AB968, (u32)D_004AB350, (u32)D_004AB978, (u32)D_004AB988,
    (u32)D_004AB998, (u32)D_004AB9A8, (u32)D_004AB9B8, (u32)D_004AB9C8,
    (u32)D_004AB9D8, (u32)D_004AB9E8, (u32)D_004AB9F8, (u32)D_004ABA08,
    (u32)D_004ABA20, (u32)D_004ABA38, (u32)D_004ABA48, (u32)D_004ABA50,
    (u32)D_004ABA60, (u32)D_004ABA70, (u32)D_004ABA90, (u32)D_004ABAB0,
    (u32)D_004ABAD0, (u32)D_004ABAF0, (u32)D_004ABB10, (u32)D_004ABB28,
    (u32)D_004ABB28, (u32)D_004ABB28, (u32)D_004ABB28, (u32)D_004ABB28,
    (u32)D_004ABB38, (u32)D_004ABB38, (u32)D_004AB350, (u32)D_004AB350,
    (u32)D_004ABB48, (u32)D_004ABB50, (u32)D_004ABB58, (u32)D_004ABB68,
    (u32)D_004ABB70, (u32)D_004ABB78, (u32)D_004ABB80, (u32)D_004ABB88,
    (u32)D_004ABB90, (u32)D_004ABBA0, (u32)D_004ABBA8, (u32)D_004ABBB0,
    (u32)D_004ABBB8, (u32)D_004ABBC8, (u32)D_004ABBD0, (u32)D_004ABBE0,
    (u32)D_004ABBF0, (u32)D_004ABC00, (u32)D_004ABC10, (u32)D_004ABC20,
    (u32)D_004ABC30, (u32)D_004ABC40, (u32)D_004ABC50, (u32)D_004ABC60,
    (u32)D_004ABC70, (u32)D_004ABC80, (u32)D_004ABC90, (u32)D_004ABCA0,
    (u32)D_004ABCB0, (u32)D_004ABCC0, (u32)D_004ABCD0, (u32)D_004ABCF0,
    (u32)D_004ABD08, (u32)D_004ABD18, (u32)D_004ABD28, (u32)D_004ABD38,
    (u32)D_004ABD48, (u32)D_004ABD58, (u32)D_004ABD68, (u32)D_004ABD78,
    (u32)D_004ABD88, (u32)D_004ABD98, (u32)D_004ABDA0, (u32)D_004ABDA8,
    (u32)D_004ABDB0, (u32)D_004ABDB8, (u32)D_004ABDC8, (u32)D_004ABDE0,
    (u32)D_004ABE00, (u32)D_004ABE18, (u32)D_004ABE28, (u32)D_004AB350,
    (u32)D_004ABE30, (u32)D_004ABE40, (u32)D_004ABE60, (u32)D_004ABE78,
    (u32)D_004ABE80, (u32)D_004AB350, (u32)D_004ABE88, (u32)D_004ABE98,
    (u32)D_004ABEA8, (u32)D_004ABEB8, (u32)D_004ABEC8, (u32)D_004ABED8,
    (u32)D_004ABEE8, (u32)D_004ABEF0, (u32)D_004ABF00, (u32)D_004ABF10,
    (u32)D_004ABF28, (u32)D_004ABF40, (u32)D_004ABF58, (u32)D_004ABF68,
    (u32)D_004ABF78, (u32)D_004ABF90, (u32)D_004ABFA8, (u32)D_004ABFB8,
    (u32)D_004ABFD0, (u32)D_004ABFE8, (u32)D_004ABFF0, (u32)D_004AC000,
    (u32)D_004AC008, (u32)D_004AC010, (u32)D_004AC018, (u32)D_004AC028,
    (u32)D_004AC038, (u32)D_004AC048, (u32)D_004AC058, (u32)D_004AC068,
    (u32)D_004AC070, (u32)D_004AC078, (u32)D_004AC080, (u32)D_004AC088,
    (u32)D_004AC098, (u32)D_004AB350, (u32)D_004AC0A8, (u32)D_004AC0B8,
    (u32)D_004AC0C8, (u32)D_004AC0D8, (u32)D_004AC0F0, (u32)D_004AC108,
    (u32)D_004AC118, (u32)D_004AC128, (u32)D_004AC138, (u32)D_004AC150,
    (u32)D_004AC168, (u32)D_004AC178, (u32)D_004AC188, (u32)D_004AC190,
    (u32)D_004AC1A0, (u32)D_004AC1B0, (u32)D_004AC1C0, (u32)D_004AC1D0,
    (u32)D_004AC1E0, (u32)D_004AC1F0, (u32)D_004AC208, (u32)D_004AC210,
    (u32)D_004AC220, (u32)D_004AC230, (u32)D_004AC240, (u32)D_004AC250,
    (u32)D_004AC260, (u32)D_004AC270, (u32)D_004AC288, (u32)D_004AC298,
    (u32)D_004AC2A8, (u32)D_004AC2B0, (u32)D_004AC2C0, (u32)D_004AC2D0,
    (u32)D_004AC2E8, (u32)D_004AB350, (u32)D_004AC2F8, (u32)D_004AC300,
    (u32)D_004AC308, (u32)D_004AB350, (u32)D_004AC310, (u32)D_004AB350,
    (u32)D_004AC318, (u32)D_004AC320, (u32)D_004AC328, (u32)D_004AC338,
    (u32)D_004AC348, (u32)D_004AC358, (u32)D_004AC368, (u32)D_004AC378,
    (u32)D_004AC380, (u32)D_004AC388, (u32)D_004AC398, (u32)D_004AC3A8,
    (u32)D_004AC3B8, (u32)D_004AC3C8, (u32)D_004AC3D8, (u32)D_004AC3E0,
    (u32)D_004AC3E8, (u32)D_004AC3F0, (u32)D_004AC400, (u32)D_004AC410,
    (u32)D_004AC420, (u32)D_004AC428, (u32)D_004AC438, (u32)D_004AC440,
    (u32)D_004AC450, (u32)D_004AC460, (u32)D_004AC470, (u32)D_004AC480,
    (u32)D_004AC490, (u32)D_004AC4A0, (u32)D_004AC4B0, (u32)D_004AC4C0,
    (u32)D_004AC4D0, (u32)D_004AC4E0, (u32)D_004AC4F0, (u32)D_004AC500,
    (u32)D_004AC510, (u32)D_004AC520, (u32)D_004AC540, (u32)D_004AC558,
    (u32)D_004AC570, (u32)D_004AC588, (u32)D_004AC598, (u32)D_004AC5B0,
    (u32)D_004AC5C8, (u32)D_004AC5D8, (u32)D_004AC5E8, (u32)D_004AC600,
    (u32)D_004AC620, (u32)D_004AC638, (u32)D_004AC648, (u32)D_004AC658,
    (u32)D_004AC668, (u32)D_004AC680, (u32)D_004AC6A0, (u32)D_004AC6B8,
    (u32)D_004AC6C8, (u32)D_004AC6D8, (u32)D_004AC6E8, (u32)D_004AC6F8,
    (u32)D_004AC708, (u32)D_004AC718, (u32)D_004AC728, (u32)D_004AC738,
    (u32)D_004AC748, (u32)D_004AC758, (u32)D_004AC768, (u32)D_004AC778,
    (u32)D_004AC788, (u32)D_004AC798, (u32)D_004AC7A8, (u32)D_004AC7B8,
    (u32)D_004AC7C8, (u32)D_004AC7D8, (u32)D_004AC7E8, (u32)D_004AC7F8,
    (u32)D_004AC808, (u32)D_004AC818, (u32)D_004AC828, (u32)D_004AC838,
    (u32)D_004AC848, (u32)D_004AC858, (u32)D_004AC868, (u32)D_004AC878,
    (u32)D_004AC888, (u32)D_004AC898, (u32)D_004AC8A8, (u32)D_004AC8B8,
    (u32)D_004AC8C8, (u32)D_004AC8D8, (u32)D_004AC8E8, (u32)D_004AC8F8,
    (u32)D_004AC908, (u32)D_004AC918, (u32)D_004AC928, (u32)D_004AC938,
    (u32)D_004AC948, (u32)D_004AC958, (u32)D_004AC968, (u32)D_004AC978,
    (u32)D_004AC988, (u32)D_004AC998, (u32)D_004AC9A8, (u32)D_004AC9B8,
    (u32)D_004AC9C8, (u32)D_004AC9D8, (u32)D_004AC9E8, (u32)D_004AC9F8,
    (u32)D_004ACA08, (u32)D_004ACA18, (u32)D_004ACA28, (u32)D_004ACA38,
    (u32)D_004ACA48, (u32)D_004ACA58, (u32)D_004ACA68, (u32)D_004ACA78,
    (u32)D_004ACA88, (u32)D_004ACA98, (u32)D_004ACAA8, (u32)D_004ACAB8,
    (u32)D_004ACAC8, (u32)D_004ACAD8, (u32)D_004ACAF0, (u32)D_004ACB10,
    (u32)D_004ACB28, (u32)D_004ACB38, (u32)D_004ACB48, (u32)D_004ACB58,
    (u32)D_004ACB68, (u32)D_004ACB78, (u32)D_004ACB88, (u32)D_004ACB98,
    (u32)D_004ACBA8, (u32)D_004ACBB8, (u32)D_004ACBC8, (u32)D_004ACBD8,
    (u32)D_004ACBE8, (u32)D_004ACC00, (u32)D_004ACC18, (u32)D_004ACC28,
    (u32)D_004ACC38, (u32)D_004ACC48, (u32)D_004ACC58, (u32)D_004ACC68,
    (u32)D_004ACC78, (u32)D_004ACC88, (u32)D_004ACC98, (u32)D_004ACCA8,
    (u32)D_004ACCB8, (u32)D_004ACCC8, (u32)D_004ACCE0, (u32)D_004ACD00,
    (u32)D_004ACD18, (u32)D_004ACD30, (u32)D_004ACD50, (u32)D_004ACD68,
    (u32)D_004ACD78, (u32)D_004ACD90, (u32)D_004ACDA8, (u32)D_004ACDB8,
    (u32)D_004ACDC8, (u32)D_004ACDD8, (u32)D_004ACDE8, (u32)D_004ACDF8,
    (u32)D_004ACE08, (u32)D_004ACE20, (u32)D_004ACE38, (u32)D_004ACE48,
    (u32)D_004ACE58, (u32)D_004ACE68, (u32)D_004ACE78, (u32)D_004ACE88,
    (u32)D_004ACEA0, (u32)D_004ACEB8, (u32)D_004ACEC8, (u32)D_004ACED8,
    (u32)D_004ACEE8, (u32)D_004ACEF8, (u32)D_004ACF08, (u32)D_004ACF18,
    (u32)D_004ACF28, (u32)D_004ACF38, (u32)D_004ACF48, (u32)D_004ACF58,
    (u32)D_004ACF68, (u32)D_004ACF78, (u32)D_004ACF88, (u32)D_004ACF98,
    (u32)D_004ACFA8, (u32)D_004ACFB8, (u32)D_004ACFC8, (u32)D_004ACFD8,
    (u32)D_004ACFE8, (u32)D_004ACFF8, (u32)D_004AD008, (u32)D_004AD018,
    (u32)D_004AD028, (u32)D_004AB350, (u32)D_004AB350, (u32)D_004AD038,
    (u32)D_004AD048, (u32)D_004AD058, (u32)D_004AD068, (u32)D_004AD078,
    (u32)D_004AD088, (u32)D_004AD098, (u32)D_004AD0A8, (u32)D_004AD0B8,
    (u32)D_004AD0C8, (u32)D_004AD0D8, (u32)D_004AD0E8, (u32)D_004AD0F8,
    (u32)D_004AB350, (u32)D_004AD108, (u32)D_004AD118, (u32)D_004AD128,
    (u32)D_004AD138, (u32)D_004AD148, (u32)D_004AD158, (u32)D_004AD168,
    (u32)D_004AD178, (u32)D_004AD180, (u32)D_004AD190, (u32)D_004AD1A0,
    (u32)D_004AD1B0, (u32)D_004AD1C0, (u32)D_004AB350, (u32)D_004AD1D0,
    (u32)D_004AD1E0, (u32)D_004AD1F0, (u32)D_004AD200, (u32)D_004AD210,
    (u32)D_004AD220, (u32)D_004AD230, (u32)D_004AB350, (u32)D_004AD240,
    (u32)D_004AD250, (u32)D_004AD260, (u32)D_004AD270, (u32)D_004AB350,
    (u32)D_004AD280, (u32)D_004AD290, (u32)D_004AD2A0, (u32)D_004AD2B0,
    (u32)D_004AD2C0, (u32)D_004AD2D0, (u32)D_004AD2E0, (u32)D_004AD2F0,
    (u32)D_004AD300, (u32)D_004AD310, (u32)D_004AD320, (u32)D_004AD330,
    (u32)D_004AD340, (u32)D_004AD350, (u32)D_004AD360, (u32)D_004AD370,
    (u32)D_004AD380, (u32)D_004AD390, (u32)D_004AD3A0, (u32)D_004AD3B0,
    (u32)D_004AB350, (u32)D_004AD3C0, (u32)D_004AD3D0, (u32)D_004AD3E0,
    (u32)D_004AD3F0, (u32)D_004AD400, (u32)D_004AD410, (u32)D_004AD420,
    (u32)D_004AD430, (u32)D_004AD448, (u32)D_004AD458, (u32)D_004AD468,
    (u32)D_004AB350, (u32)D_004AD478, (u32)D_004AD488, (u32)D_004AD4A0,
    (u32)D_004AD4B8, (u32)D_004AD4C8, (u32)D_004AD4D8, (u32)D_004AD4E8,
    (u32)D_004AD4F8, (u32)D_004AD508, (u32)D_004AD518, (u32)D_004AD528,
    (u32)D_004AD530, (u32)D_004AB350, (u32)D_004AD538, (u32)D_004AD548,
    (u32)D_004AD558, (u32)D_004AD568, (u32)D_004AD578, (u32)D_004AD588,
    (u32)D_004AD598, (u32)D_004AD5A8, (u32)D_004AD5B8, (u32)D_004AD5C8,
    (u32)D_004AD5E0, (u32)D_004AD5F8, (u32)D_004AD608, (u32)D_004AD618,
    (u32)D_004AD628, (u32)D_004AD638, (u32)D_004AD648, (u32)D_004AD658,
    (u32)D_004AD668, (u32)D_004AD678, (u32)D_004AD688, (u32)D_004AD698,
    (u32)D_004AD6A8, (u32)D_004AD6B8, (u32)D_004AD6C8, (u32)D_004AD6D8,
    (u32)D_004AD6E8, (u32)D_004AD700, (u32)D_004AD720, (u32)D_004AD740,
    (u32)D_004AD760, (u32)D_004AD778, (u32)D_004AD790, (u32)D_004AD7A8,
    (u32)D_004AD7B8, (u32)D_004AD7C8, (u32)D_004AD7D8, (u32)D_004AD7F0,
    (u32)D_004AD810, (u32)D_004AD830, (u32)D_004AD848, (u32)D_004AD858,
    (u32)D_004AD868, (u32)D_004AD880, (u32)D_004AD8A0, (u32)D_004AD8C0,
    (u32)D_004AD8D8, (u32)D_004AD8F0, (u32)D_004AD908, (u32)D_004AD918,
    (u32)D_004AD930, (u32)D_004AD948, (u32)D_004AD960, (u32)D_004AD980,
    (u32)D_004AD9A0, (u32)D_004AD9B8, (u32)D_004AD9D0, (u32)D_004AB350,
    (u32)D_004AB350, (u32)D_004AB350, (u32)D_004AB350, (u32)D_004AD9E8,
    (u32)D_004AD9F8, (u32)D_004ADA08, (u32)D_004ADA10, (u32)D_004ADA18,
    (u32)D_004ADA30, (u32)D_004ADA48, (u32)D_004ADA58, (u32)D_004ADA68,
    (u32)D_004ADA78, (u32)D_004ADA90, (u32)D_004ADAB0, (u32)D_004ADAC8,
    (u32)D_004ADAD8, (u32)D_004ADAE8, (u32)D_004ADAF8, (u32)D_004ADB08,
    (u32)D_004ADB18, (u32)D_004ADB28, (u32)D_004ADB38, (u32)D_004ADB48,
    (u32)D_004ADB50, (u32)D_004ADB60, (u32)D_004ADB70, (u32)D_004ADB80,
    (u32)D_004ADB90, (u32)D_004ADBA0, (u32)D_004ADBB0, (u32)D_004ADBC0,
    (u32)D_004ADBD0, (u32)D_004ADBE0, (u32)D_004ADBF0, (u32)D_004ADC00,
    (u32)D_004ADC10, (u32)D_004ADC18, (u32)D_004ADC20, (u32)D_004ADC28,
    (u32)D_004ADC30, (u32)D_004ADC38, (u32)D_004ADC40, (u32)D_004ADC48,
    (u32)D_004ADC58, (u32)D_004AB350, (u32)D_004ADC68, (u32)D_004ADC78,
    (u32)D_004ADC80, (u32)D_004ADC88, (u32)D_004ADC98, (u32)D_004ADCA8,
    (u32)D_004ADCB8, (u32)D_004ADCC8, (u32)D_004ADCE0, (u32)D_004ADCF8,
    (u32)D_004ADD08, (u32)D_004ADD18, (u32)D_004ADD28, (u32)D_004ADD38,
    (u32)D_004ADD48, (u32)D_004ADD58, (u32)D_004ADD68, (u32)D_004ADD78,
    (u32)D_004ADD88, (u32)D_004ADD98, (u32)D_004ADDA8, (u32)D_004ADDB8,
    (u32)D_004ADDD0, (u32)D_004ADDE8, (u32)D_004ADE00, (u32)D_004ADE18,
    (u32)D_004ADE28, (u32)D_004ADE38, (u32)D_004ADE48, (u32)D_004ADE58,
    (u32)D_004ADE70, (u32)D_004ADE90, (u32)D_004ADEB0, (u32)D_004ADEC8,
    (u32)D_004ADED8, (u32)D_004ADEE8, (u32)D_004ADEF8, (u32)D_004ADF08,
    (u32)D_004ADF18, (u32)D_004ADF20, (u32)D_004ADF30, (u32)D_004ADF40,
    (u32)D_004ADF50, (u32)D_004ADF60, (u32)D_004ADF70, (u32)D_004ADF80,
    (u32)D_004ADF88, (u32)D_004ADF98, (u32)D_004ADFA8, (u32)D_004ADFB8,
    (u32)D_004ADFD0, (u32)D_004ADFE8, (u32)D_004AE000, (u32)D_004AE018,
    (u32)D_004AE028, (u32)D_004AE040, (u32)D_004AE058, (u32)D_004AE070,
    (u32)D_004AE090, (u32)D_004AE0B0, (u32)D_004AE0C8, (u32)D_004AE0D8,
    (u32)D_004AE0F0, (u32)D_004AE0F0, (u32)D_004AE0F0, (u32)D_004AE0F0,
    (u32)D_004AE0F0, (u32)D_004AE0F0, (u32)D_004AE0F0, (u32)D_004AE0F0,
    (u32)D_004AE0F0, (u32)D_004AE0F0, (u32)D_004AE0F0, (u32)D_004AE0F0,
    (u32)D_004AE0F0, (u32)D_004AE0F0, (u32)D_004AE110, (u32)D_004AE110,
    (u32)D_004AE110, (u32)D_004AE110, (u32)D_004AE130, (u32)D_004AE130,
    (u32)D_004AE130, (u32)D_004AE130, (u32)D_004AE150, (u32)D_004AE130,
    (u32)D_004AE130, (u32)D_004AE130, (u32)D_004AE130, (u32)D_004AE130,
    (u32)D_004AE130, (u32)D_004AE130, (u32)D_004AE130, (u32)D_004AE130,
    (u32)D_004AE130, (u32)D_004AE130, (u32)D_004AE168, (u32)D_004AE178,
    (u32)D_004AE188, (u32)D_004AE198, (u32)D_004AE1A8, (u32)D_004AE1B0,
    (u32)D_004AE1C0, (u32)D_004AE1C8, (u32)D_004AE1D0, (u32)D_004AE1D8,
    (u32)D_004AE1E0, (u32)D_004AE1E8, (u32)D_004AE1F0, (u32)D_004AE1F8,
    (u32)D_004AE200, (u32)D_004AE208, (u32)D_004AE210, (u32)D_004AE218,
    (u32)D_004AE220, (u32)D_004AE228, (u32)D_004AE230, (u32)D_004AE238,
    (u32)D_004AE240, (u32)D_004AE248, (u32)D_004AE250, (u32)D_004AE258,
    (u32)D_004AE260, (u32)D_004AE268, (u32)D_004AE270, (u32)D_004AE278,
    (u32)D_004AE280, (u32)D_004AE288, (u32)D_004AE290, (u32)D_004AE298,
    (u32)D_004AE2A0, (u32)D_004AE2A8, (u32)D_004AE2B0, (u32)D_004AE2B8,
    (u32)D_004AE2C0, (u32)D_004AE2C8, (u32)D_004AE2D0, (u32)D_004AE2D8,
    (u32)D_004AE2E0, (u32)D_004AE2E8, (u32)D_004AE2F0, (u32)D_004AE2F8,
    (u32)D_004AE300, (u32)D_004AE308, (u32)D_004AE310, (u32)D_004AE318,
    (u32)D_004AE320, (u32)D_004AE328, (u32)D_004AE330, (u32)D_004AE338,
    (u32)D_004AE340, (u32)D_004AE348, (u32)D_004AE350, (u32)D_004AE358,
    (u32)D_004AE360, (u32)D_004AE368, (u32)D_004AE370, (u32)D_004AE378,
    (u32)D_004AE380, (u32)D_004AE388, (u32)D_004AE390, (u32)D_004AE398,
    (u32)D_004AE3A0, (u32)D_004AE3A8, (u32)D_004AE3B0, (u32)D_004AE3B8,
    (u32)D_004AE3C0, (u32)D_004AE3C8, (u32)D_004AE3D0, (u32)D_004AE3D8,
    (u32)D_004AE3E0, (u32)D_004AE3E8, (u32)D_004AE3F0, (u32)D_004AE3F8,
    (u32)D_004AE400, (u32)D_004AE408, (u32)D_004AE410, (u32)D_004AE420,
    (u32)D_004AE428, (u32)D_004AE430, (u32)D_004AE438, (u32)D_004AE440,
    (u32)D_004AE448, (u32)D_004AE450, (u32)D_004AE458, (u32)D_004AE460,
    (u32)D_004AE468, (u32)D_004AE470, (u32)D_004AE478, (u32)D_004AE480,
    (u32)D_004AE488, (u32)D_004AE490, (u32)D_004AE498, (u32)D_004AE4A0,
    (u32)D_004AE4A8, (u32)D_004AE4B0, (u32)D_004AE4B8, (u32)D_004AE4C0,
    (u32)D_004AE4C8, (u32)D_004AE4D0, (u32)D_004AE4D8, (u32)D_004AE4E0,
    (u32)D_004AE4E8, (u32)D_004AE4F0, (u32)D_004AE4F8, (u32)D_004AE500,
    (u32)D_004AE508, (u32)D_004AE510, (u32)D_004AE518, (u32)D_004AE520,
    (u32)D_004AE528, (u32)D_004AE530, (u32)D_004AE538, (u32)D_004AE540,
    (u32)D_004AE548, (u32)D_004AE550, (u32)D_004AE558, (u32)D_004AE560,
    (u32)D_004AE568, (u32)D_004AE570, (u32)D_004AE578, (u32)D_004AE580,
    (u32)D_004AE588, 0, 0, 0,
};

/* The equipment's texts and constants, read-only. */
#pragma rodata on
char D_004A00E8[12] = {
#include "sub_main/004A00E8.inc"
};
char gBladeNames_1[8] = {
#include "sub_main/004A00F8.inc"
};
char D_004A0100[16] = {
#include "sub_main/004A0100.inc"
};
char gBladeNames_2[24] = {
#include "sub_main/004A0110.inc"
};
char gBladeNames_3[15] = {
#include "sub_main/004A0128.inc"
};
char gBladeNames_4[24] = {
#include "sub_main/004A0140.inc"
};
char gBladeNames_5[12] = {
#include "sub_main/004A0158.inc"
};
char gBladeNames_6[24] = {
#include "sub_main/004A0170.inc"
};
char gBladeNames_7[15] = {
#include "sub_main/004A0188.inc"
};
char gBladeNames_8[32] = {
#include "sub_main/004A01A0.inc"
};
char gBladeNames_9[24] = {
#include "sub_main/004A01C0.inc"
};
char gBladeNames_10[8] = {
#include "sub_main/004A01D8.inc"
};
char gBladeNames_11[16] = {
#include "sub_main/004A01E0.inc"
};
char gBladeNames_12[16] = {
#include "sub_main/004A01F0.inc"
};
char gBladeNames_13[16] = {
#include "sub_main/004A0200.inc"
};
char gBladeNames_14[16] = {
#include "sub_main/004A0210.inc"
};
char gBladeNames_15[16] = {
#include "sub_main/004A0220.inc"
};
char gBladeNames_16[16] = {
#include "sub_main/004A0230.inc"
};
char gBladeNames_17[16] = {
#include "sub_main/004A0240.inc"
};
char gBladeNames_18[16] = {
#include "sub_main/004A0250.inc"
};
char gBladeNames_19[16] = {
#include "sub_main/004A0260.inc"
};
char gBladeNames_20[16] = {
#include "sub_main/004A0270.inc"
};
char gBladeNames_21[16] = {
#include "sub_main/004A0280.inc"
};
char gBladeNames_22[16] = {
#include "sub_main/004A0290.inc"
};
char gBladeNames_23[16] = {
#include "sub_main/004A02A0.inc"
};
char gBladeNames_24[32] = {
#include "sub_main/004A02B0.inc"
};
char gBladeNames_25[24] = {
#include "sub_main/004A02D0.inc"
};
char gBladeNames_26[13] = {
#include "sub_main/004A02E8.inc"
};
char gBladeNames_27[24] = {
#include "sub_main/004A0300.inc"
};
char gBladeNames_28[15] = {
#include "sub_main/004A0318.inc"
};
char gBladeNames_29[24] = {
#include "sub_main/004A0330.inc"
};
char gBladeNames_30[8] = {
#include "sub_main/004A0348.inc"
};
char gBladeNames_31[16] = {
#include "sub_main/004A0350.inc"
};
char gBladeNames_32[16] = {
#include "sub_main/004A0360.inc"
};
char gBladeNames_33[32] = {
#include "sub_main/004A0370.inc"
};
char gBladeNames_34[32] = {
#include "sub_main/004A0390.inc"
};
char gBladeNames_35[24] = {
#include "sub_main/004A03B0.inc"
};
char gBladeNames_36[13] = {
#include "sub_main/004A03C8.inc"
};
char gBladeNames_37[15] = {
#include "sub_main/004A03D8.inc"
};
char gBladeNames_38[13] = {
#include "sub_main/004A03E8.inc"
};
char gBladeNames_39[15] = {
#include "sub_main/004A03F8.inc"
};
char gBladeNames_40[15] = {
#include "sub_main/004A0408.inc"
};
char gBladeNames_41[15] = {
#include "sub_main/004A0418.inc"
};
char gBladeNames_42[15] = {
#include "sub_main/004A0428.inc"
};
char gBladeNames_43[15] = {
#include "sub_main/004A0438.inc"
};
char gBladeNames_44[15] = {
#include "sub_main/004A0448.inc"
};
char gBladeNames_45[12] = {
#include "sub_main/004A0458.inc"
};
char gBladeNames_46[32] = {
#include "sub_main/004A0470.inc"
};
char gBladeNames_47[24] = {
#include "sub_main/004A0490.inc"
};
char gBladeNames_48[8] = {
#include "sub_main/004A04A8.inc"
};
char gBladeNames_49[16] = {
#include "sub_main/004A04B0.inc"
};
char gBladeNames_50[16] = {
#include "sub_main/004A04C0.inc"
};
char gBladeNames_51[16] = {
#include "sub_main/004A04D0.inc"
};
char gBladeNames_52[16] = {
#include "sub_main/004A04E0.inc"
};
char gBladeNames_53[24] = {
#include "sub_main/004A04F0.inc"
};
char gBladeNames_54[13] = {
#include "sub_main/004A0508.inc"
};
char gBladeNames_55[32] = {
#include "sub_main/004A0520.inc"
};
char gBladeNames_56[24] = {
#include "sub_main/004A0540.inc"
};
char gBladeNames_57[15] = {
#include "sub_main/004A0558.inc"
};
char gBladeNames_58[8] = {
#include "sub_main/004A0568.inc"
};
char gBladeNames_59[16] = {
#include "sub_main/004A0570.inc"
};
char gBladeNames_60[32] = {
#include "sub_main/004A0580.inc"
};
char gBladeNames_61[32] = {
#include "sub_main/004A05A0.inc"
};
char gBladeNames_62[32] = {
#include "sub_main/004A05C0.inc"
};
char gBladeNames_63[32] = {
#include "sub_main/004A05E0.inc"
};
char gBladeNames_64[32] = {
#include "sub_main/004A0600.inc"
};
char gBladeNames_65[32] = {
#include "sub_main/004A0620.inc"
};
char gBladeNames_66[32] = {
#include "sub_main/004A0640.inc"
};
char gBladeNames_67[32] = {
#include "sub_main/004A0660.inc"
};
char gBladeNames_68[32] = {
#include "sub_main/004A0680.inc"
};
char gBladeNames_69[24] = {
#include "sub_main/004A06A0.inc"
};
char gBladeNames_70[13] = {
#include "sub_main/004A06B8.inc"
};
char gBladeNames_71[13] = {
#include "sub_main/004A06C8.inc"
};
char gBladeNames_72[32] = {
#include "sub_main/004A06E0.inc"
};
char gBladeNames_73[24] = {
#include "sub_main/004A0700.inc"
};
char gBladeNames_74[15] = {
#include "sub_main/004A0718.inc"
};
char gBladeNames_75[24] = {
#include "sub_main/004A0730.inc"
};
char gBladeNames_76[12] = {
#include "sub_main/004A0748.inc"
};
char gBladeNames_77[24] = {
#include "sub_main/004A0760.inc"
};
char gBladeNames_78[15] = {
#include "sub_main/004A0778.inc"
};
char gBladeNames_79[32] = {
#include "sub_main/004A0790.inc"
};
char gBladeNames_80[24] = {
#include "sub_main/004A07B0.inc"
};
char gBladeNames_81[12] = {
#include "sub_main/004A07C8.inc"
};
char gBladeNames_82[32] = {
#include "sub_main/004A07E0.inc"
};
char gBladeNames_83[32] = {
#include "sub_main/004A0800.inc"
};
char gBladeNames_84[24] = {
#include "sub_main/004A0820.inc"
};
char gBladeNames_85[13] = {
#include "sub_main/004A0838.inc"
};
char gBladeNames_86[15] = {
#include "sub_main/004A0848.inc"
};
char gBladeNames_87[24] = {
#include "sub_main/004A0860.inc"
};
char gBladeNames_88[13] = {
#include "sub_main/004A0878.inc"
};
char gBladeNames_89[13] = {
#include "sub_main/004A0888.inc"
};
char gBladeNames_90[32] = {
#include "sub_main/004A08A0.inc"
};
char gBladeNames_91[24] = {
#include "sub_main/004A08C0.inc"
};
char gBladeNames_92[15] = {
#include "sub_main/004A08D8.inc"
};
char gBladeNames_93[32] = {
#include "sub_main/004A08F0.inc"
};
char gBladeNames_94[24] = {
#include "sub_main/004A0910.inc"
};
char gBladeNames_95[13] = {
#include "sub_main/004A0928.inc"
};
char gBladeNames_96[12] = {
#include "sub_main/004A0938.inc"
};
char gBladeNames_97[8] = {
#include "sub_main/004A0948.inc"
};
char gBladeNames_98[16] = {
#include "sub_main/004A0950.inc"
};
char gBladeNames_99[8] = {
#include "sub_main/004A0960.inc"
};
char gBladeNames_100[8] = {
#include "sub_main/004A0968.inc"
};
char gBladeNames_101[32] = {
#include "sub_main/004A0970.inc"
};
char gBladeNames_102[24] = {
#include "sub_main/004A0990.inc"
};
char gBladeNames_103[15] = {
#include "sub_main/004A09A8.inc"
};
char gBladeNames_104[24] = {
#include "sub_main/004A09C0.inc"
};
char gBladeNames_105[13] = {
#include "sub_main/004A09D8.inc"
};
char gBladeNames_106[13] = {
#include "sub_main/004A09E8.inc"
};
char gBladeNames_107[32] = {
#include "sub_main/004A0A00.inc"
};
char gBladeNames_108[24] = {
#include "sub_main/004A0A20.inc"
};
char gBladeNames_109[12] = {
#include "sub_main/004A0A38.inc"
};
char gBladeNames_110[24] = {
#include "sub_main/004A0A50.inc"
};
char gBladeNames_111[8] = {
#include "sub_main/004A0A68.inc"
};
char gBladeNames_112[16] = {
#include "sub_main/004A0A70.inc"
};
char gBladeNames_113[24] = {
#include "sub_main/004A0A80.inc"
};
char gBladeNames_114[15] = {
#include "sub_main/004A0A98.inc"
};
char gBladeNames_115[13] = {
#include "sub_main/004A0AA8.inc"
};
char gBladeNames_116[15] = {
#include "sub_main/004A0AB8.inc"
};
char gBladeNames_117[24] = {
#include "sub_main/004A0AD0.inc"
};
char gBladeNames_118[13] = {
#include "sub_main/004A0AE8.inc"
};
char gBladeNames_119[13] = {
#include "sub_main/004A0AF8.inc"
};
char gBladeNames_120[24] = {
#include "sub_main/004A0B10.inc"
};
char gBladeNames_121[13] = {
#include "sub_main/004A0B28.inc"
};
char gBladeNames_122[15] = {
#include "sub_main/004A0B38.inc"
};
char gBladeNames_123[15] = {
#include "sub_main/004A0B48.inc"
};
char gBladeNames_124[12] = {
#include "sub_main/004A0B58.inc"
};
char gBladeNames_125[32] = {
#include "sub_main/004A0B70.inc"
};
char gBladeNames_126[24] = {
#include "sub_main/004A0B90.inc"
};
char gBladeNames_127[13] = {
#include "sub_main/004A0BA8.inc"
};
char gBladeNames_128[15] = {
#include "sub_main/004A0BB8.inc"
};
char gBladeNames_129[13] = {
#include "sub_main/004A0BC8.inc"
};
char gBladeNames_130[24] = {
#include "sub_main/004A0BE0.inc"
};
char gBladeNames_131[13] = {
#include "sub_main/004A0BF8.inc"
};
char gBladeNames_132[24] = {
#include "sub_main/004A0C10.inc"
};
char gBladeNames_133[15] = {
#include "sub_main/004A0C28.inc"
};
char gBladeNames_134[15] = {
#include "sub_main/004A0C38.inc"
};
char gBladeNames_135[15] = {
#include "sub_main/004A0C48.inc"
};
char gBladeNames_136[13] = {
#include "sub_main/004A0C58.inc"
};
char gBladeNames_137[24] = {
#include "sub_main/004A0C70.inc"
};
char gBladeNames_138[12] = {
#include "sub_main/004A0C88.inc"
};
char gBladeNames_139[13] = {
#include "sub_main/004A0C98.inc"
};
char gBladeNames_140[32] = {
#include "sub_main/004A0CB0.inc"
};
char gBladeNames_141[32] = {
#include "sub_main/004A0CD0.inc"
};
char gBladeNames_142[32] = {
#include "sub_main/004A0CF0.inc"
};
char gBladeNames_143[24] = {
#include "sub_main/004A0D10.inc"
};
char gBladeNames_144[15] = {
#include "sub_main/004A0D28.inc"
};
char gBladeNames_145[32] = {
#include "sub_main/004A0D40.inc"
};
char gBladeNames_146[24] = {
#include "sub_main/004A0D60.inc"
};
char gBladeNames_147[15] = {
#include "sub_main/004A0D78.inc"
};
char gBladeNames_148[32] = {
#include "sub_main/004A0D90.inc"
};
char gBladeNames_149[24] = {
#include "sub_main/004A0DB0.inc"
};
char gBladeNames_150[13] = {
#include "sub_main/004A0DC8.inc"
};
char gBladeNames_151[12] = {
#include "sub_main/004A0DD8.inc"
};
char gBladeNames_152[24] = {
#include "sub_main/004A0DF0.inc"
};
char gBladeNames_153[15] = {
#include "sub_main/004A0E08.inc"
};
char gBladeNames_154[15] = {
#include "sub_main/004A0E18.inc"
};
char gBladeNames_155[15] = {
#include "sub_main/004A0E28.inc"
};
char gBladeNames_156[15] = {
#include "sub_main/004A0E38.inc"
};
char gBladeNames_157[32] = {
#include "sub_main/004A0E50.inc"
};
char gBladeNames_158[32] = {
#include "sub_main/004A0E70.inc"
};
char gBladeNames_159[32] = {
#include "sub_main/004A0E90.inc"
};
char gBladeNames_160[24] = {
#include "sub_main/004A0EB0.inc"
};
char gBladeNames_161[8] = {
#include "sub_main/004A0EC8.inc"
};
char gBladeNames_162[8] = {
#include "sub_main/004A0ED0.inc"
};
char gBladeNames_163[12] = {
#include "sub_main/004A0ED8.inc"
};
char gBladeNames_164[13] = {
#include "sub_main/004A0EE8.inc"
};
char gBladeNames_165[32] = {
#include "sub_main/004A0F00.inc"
};
char gBladeNames_166[32] = {
#include "sub_main/004A0F20.inc"
};
char gBladeNames_167[24] = {
#include "sub_main/004A0F40.inc"
};
char gBladeNames_168[12] = {
#include "sub_main/004A0F58.inc"
};
char gBladeNames_169[13] = {
#include "sub_main/004A0F68.inc"
};
char gBladeNames_170[15] = {
#include "sub_main/004A0F78.inc"
};
char gBladeNames_171[32] = {
#include "sub_main/004A0F90.inc"
};
char gBladeNames_172[24] = {
#include "sub_main/004A0FB0.inc"
};
char gBladeNames_173[15] = {
#include "sub_main/004A0FC8.inc"
};
char gBladeNames_174[32] = {
#include "sub_main/004A0FE0.inc"
};
char gBladeNames_175[32] = {
#include "sub_main/004A1000.inc"
};
char gBladeNames_176[32] = {
#include "sub_main/004A1020.inc"
};
char gBladeNames_177[24] = {
#include "sub_main/004A1040.inc"
};
char gBladeNames_178[13] = {
#include "sub_main/004A1058.inc"
};
char gBladeNames_179[15] = {
#include "sub_main/004A1068.inc"
};
char gBladeNames_180[13] = {
#include "sub_main/004A1078.inc"
};
char gBladeNames_181[15] = {
#include "sub_main/004A1088.inc"
};
char gBladeNames_182[13] = {
#include "sub_main/004A1098.inc"
};
char gBladeNames_183[13] = {
#include "sub_main/004A10A8.inc"
};
char gBladeNames_184[24] = {
#include "sub_main/004A10C0.inc"
};
char gBladeNames_185[15] = {
#include "sub_main/004A10D8.inc"
};
char gBladeNames_186[32] = {
#include "sub_main/004A10F0.inc"
};
char gBladeNames_187[24] = {
#include "sub_main/004A1110.inc"
};
char gBladeNames_188[8] = {
#include "sub_main/004A1128.inc"
};
char gBladeNames_189[32] = {
#include "sub_main/004A1130.inc"
};
char gBladeNames_190[24] = {
#include "sub_main/004A1150.inc"
};
char gBladeNames_191[15] = {
#include "sub_main/004A1168.inc"
};
char gBladeNames_192[15] = {
#include "sub_main/004A1178.inc"
};
char gBladeNames_193[24] = {
#include "sub_main/004A1190.inc"
};
char gBladeNames_194[12] = {
#include "sub_main/004A11A8.inc"
};
char gBladeNames_195[32] = {
#include "sub_main/004A11C0.inc"
};
char gBladeNames_196[24] = {
#include "sub_main/004A11E0.inc"
};
char gBladeNames_197[15] = {
#include "sub_main/004A11F8.inc"
};
char gBladeNames_198[12] = {
#include "sub_main/004A1208.inc"
};
char gBladeNames_199[15] = {
#include "sub_main/004A1218.inc"
};
char gBladeNames_200[24] = {
#include "sub_main/004A1230.inc"
};
char gBladeNames_201[8] = {
#include "sub_main/004A1248.inc"
};
char gBladeNames_202[16] = {
#include "sub_main/004A1250.inc"
};
char gBladeNames_203[24] = {
#include "sub_main/004A1260.inc"
};
char gBladeNames_204[13] = {
#include "sub_main/004A1278.inc"
};
char gBladeNames_205[13] = {
#include "sub_main/004A1288.inc"
};
char gBladeNames_206[24] = {
#include "sub_main/004A12A0.inc"
};
char gBladeNames_207[13] = {
#include "sub_main/004A12B8.inc"
};
char gBladeNames_208[8] = {
#include "sub_main/004A12C8.inc"
};
char gBladeNames_209[16] = {
#include "sub_main/004A12D0.inc"
};
char gBladeNames_210[32] = {
#include "sub_main/004A12E0.inc"
};
char gBladeNames_211[24] = {
#include "sub_main/004A1300.inc"
};
char gBladeNames_212[15] = {
#include "sub_main/004A1318.inc"
};
char gBladeNames_213[32] = {
#include "sub_main/004A1330.inc"
};
char gBladeNames_214[24] = {
#include "sub_main/004A1350.inc"
};
char gBladeNames_215[12] = {
#include "sub_main/004A1368.inc"
};
char gBladeNames_216[32] = {
#include "sub_main/004A1380.inc"
};
char gBladeNames_217[32] = {
#include "sub_main/004A13A0.inc"
};
char gBladeNames_218[24] = {
#include "sub_main/004A13C0.inc"
};
char gBladeNames_219[8] = {
#include "sub_main/004A13D8.inc"
};
char gBladeNames_220[16] = {
#include "sub_main/004A13E0.inc"
};
char gBladeNames_221[8] = {
#include "sub_main/004A13F0.inc"
};
char gBladeNames_222[8] = {
#include "sub_main/004A13F8.inc"
};
char gBladeNames_223[16] = {
#include "sub_main/004A1400.inc"
};
char gBladeNames_224[16] = {
#include "sub_main/004A1410.inc"
};
char gBladeNames_225[16] = {
#include "sub_main/004A1420.inc"
};
char gBladeNames_226[16] = {
#include "sub_main/004A1430.inc"
};
char gBladeNames_227[8] = {
#include "sub_main/004A1440.inc"
};
char gBladeNames_228[13] = {
#include "sub_main/004A1448.inc"
};
char gBladeNames_229[15] = {
#include "sub_main/004A1458.inc"
};
char gBladeNames_230[24] = {
#include "sub_main/004A1470.inc"
};
char gBladeNames_231[12] = {
#include "sub_main/004A1488.inc"
};
char gBladeNames_232[24] = {
#include "sub_main/004A14A0.inc"
};
char gBladeNames_233[12] = {
#include "sub_main/004A14B8.inc"
};
char gBladeNames_234[13] = {
#include "sub_main/004A14C8.inc"
};
char gBladeNames_235[24] = {
#include "sub_main/004A14E0.inc"
};
char gBladeNames_236[12] = {
#include "sub_main/004A14F8.inc"
};
char gBladeNames_237[15] = {
#include "sub_main/004A1508.inc"
};
char gBladeNames_238[12] = {
#include "sub_main/004A1518.inc"
};
char gBladeNames_239[32] = {
#include "sub_main/004A1530.inc"
};
char gBladeNames_240[32] = {
#include "sub_main/004A1550.inc"
};
char gBladeNames_241[24] = {
#include "sub_main/004A1570.inc"
};
char gBladeNames_242[15] = {
#include "sub_main/004A1588.inc"
};
char gBladeNames_243[32] = {
#include "sub_main/004A15A0.inc"
};
char gBladeNames_244[24] = {
#include "sub_main/004A15C0.inc"
};
char gBladeNames_245[15] = {
#include "sub_main/004A15D8.inc"
};
char gBladeNames_246[15] = {
#include "sub_main/004A15E8.inc"
};
char gBladeNames_247[24] = {
#include "sub_main/004A1600.inc"
};
char gBladeNames_248[15] = {
#include "sub_main/004A1618.inc"
};
char gBladeNames_249[24] = {
#include "sub_main/004A1630.inc"
};
char gBladeNames_250[13] = {
#include "sub_main/004A1648.inc"
};
char gBladeNames_251[15] = {
#include "sub_main/004A1658.inc"
};
char gBladeNames_252[32] = {
#include "sub_main/004A1670.inc"
};
char gBladeNames_253[24] = {
#include "sub_main/004A1690.inc"
};
char gBladeNames_254[15] = {
#include "sub_main/004A16A8.inc"
};
char gBladeNames_255[32] = {
#include "sub_main/004A16C0.inc"
};
char gBladeNames_256[24] = {
#include "sub_main/004A16E0.inc"
};
char gBladeNames_257[8] = {
#include "sub_main/004A16F8.inc"
};
char gBladeNames_258[16] = {
#include "sub_main/004A1700.inc"
};
char gBladeNames_259[32] = {
#include "sub_main/004A1710.inc"
};
char gBladeNames_260[24] = {
#include "sub_main/004A1730.inc"
};
char gBladeNames_261[15] = {
#include "sub_main/004A1748.inc"
};
char gBladeNames_262[15] = {
#include "sub_main/004A1758.inc"
};
char gBladeNames_263[32] = {
#include "sub_main/004A1770.inc"
};
char gBladeNames_264[32] = {
#include "sub_main/004A1790.inc"
};
char gBladeNames_265[32] = {
#include "sub_main/004A17B0.inc"
};
char gBladeNames_266[32] = {
#include "sub_main/004A17D0.inc"
};
char gBladeNames_267[32] = {
#include "sub_main/004A17F0.inc"
};
char gBladeNames_268[24] = {
#include "sub_main/004A1810.inc"
};
char gBladeNames_269[15] = {
#include "sub_main/004A1828.inc"
};
char gBladeNames_270[12] = {
#include "sub_main/004A1838.inc"
};
char gBladeNames_271[8] = {
#include "sub_main/004A1848.inc"
};
char gBladeNames_272[16] = {
#include "sub_main/004A1850.inc"
};
char gBladeNames_273[24] = {
#include "sub_main/004A1860.inc"
};
char gBladeNames_274[12] = {
#include "sub_main/004A1878.inc"
};
char gBladeNames_275[13] = {
#include "sub_main/004A1888.inc"
};
char gBladeNames_276[15] = {
#include "sub_main/004A1898.inc"
};
char gBladeNames_277[12] = {
#include "sub_main/004A18A8.inc"
};
char gBladeNames_278[13] = {
#include "sub_main/004A18B8.inc"
};
char gBladeNames_279[24] = {
#include "sub_main/004A18D0.inc"
};
char gBladeNames_280[13] = {
#include "sub_main/004A18E8.inc"
};
char gBladeNames_281[24] = {
#include "sub_main/004A1900.inc"
};
char gBladeNames_282[15] = {
#include "sub_main/004A1918.inc"
};
char gBladeNames_283[12] = {
#include "sub_main/004A1928.inc"
};
char gBladeNames_284[13] = {
#include "sub_main/004A1938.inc"
};
char gBladeNames_285[8] = {
#include "sub_main/004A1948.inc"
};
char gBladeNames_286[32] = {
#include "sub_main/004A1950.inc"
};
char gBladeNames_287[32] = {
#include "sub_main/004A1970.inc"
};
char gBladeNames_288[32] = {
#include "sub_main/004A1990.inc"
};
char gBladeNames_289[32] = {
#include "sub_main/004A19B0.inc"
};
char gBladeNames_290[24] = {
#include "sub_main/004A19D0.inc"
};
char gBladeNames_291[13] = {
#include "sub_main/004A19E8.inc"
};
char gBladeNames_292[15] = {
#include "sub_main/004A19F8.inc"
};
char gBladeNames_293[24] = {
#include "sub_main/004A1A10.inc"
};
char gBladeNames_294[13] = {
#include "sub_main/004A1A28.inc"
};
char gBladeNames_295[13] = {
#include "sub_main/004A1A38.inc"
};
char gBladeNames_296[32] = {
#include "sub_main/004A1A50.inc"
};
char gBladeNames_297[24] = {
#include "sub_main/004A1A70.inc"
};
char gBladeNames_298[8] = {
#include "sub_main/004A1A88.inc"
};
char gBladeNames_299[16] = {
#include "sub_main/004A1A90.inc"
};
char gBladeNames_300[16] = {
#include "sub_main/004A1AA0.inc"
};
char gBladeNames_301[16] = {
#include "sub_main/004A1AB0.inc"
};
char gBladeNames_302[24] = {
#include "sub_main/004A1AC0.inc"
};
char gBladeNames_303[8] = {
#include "sub_main/004A1AD8.inc"
};
char gBladeNames_304[16] = {
#include "sub_main/004A1AE0.inc"
};
char gBladeNames_305[16] = {
#include "sub_main/004A1AF0.inc"
};
char gBladeNames_306[16] = {
#include "sub_main/004A1B00.inc"
};
char gBladeNames_307[32] = {
#include "sub_main/004A1B10.inc"
};
char gBladeNames_308[32] = {
#include "sub_main/004A1B30.inc"
};
char gBladeNames_309[24] = {
#include "sub_main/004A1B50.inc"
};
char gBladeNames_310[15] = {
#include "sub_main/004A1B68.inc"
};
char gBladeNames_311[12] = {
#include "sub_main/004A1B78.inc"
};
char gBladeNames_312[32] = {
#include "sub_main/004A1B90.inc"
};
char gBladeNames_313[24] = {
#include "sub_main/004A1BB0.inc"
};
char gBladeNames_314[13] = {
#include "sub_main/004A1BC8.inc"
};
char gBladeNames_315[15] = {
#include "sub_main/004A1BD8.inc"
};
char gBladeNames_316[13] = {
#include "sub_main/004A1BE8.inc"
};
char gBladeNames_317[12] = {
#include "sub_main/004A1BF8.inc"
};
char gBladeNames_318[8] = {
#include "sub_main/004A1C08.inc"
};
char gBladeNames_319[16] = {
#include "sub_main/004A1C10.inc"
};
char gBladeNames_320[32] = {
#include "sub_main/004A1C20.inc"
};
char gBladeNames_321[24] = {
#include "sub_main/004A1C40.inc"
};
char gBladeNames_322[12] = {
#include "sub_main/004A1C58.inc"
};
char gBladeNames_323[24] = {
#include "sub_main/004A1C70.inc"
};
char gBladeNames_324[8] = {
#include "sub_main/004A1C88.inc"
};
char gBladeNames_325[16] = {
#include "sub_main/004A1C90.inc"
};
char gBladeNames_326[16] = {
#include "sub_main/004A1CA0.inc"
};
char gBladeNames_327[32] = {
#include "sub_main/004A1CB0.inc"
};
char gBladeNames_328[24] = {
#include "sub_main/004A1CD0.inc"
};
char gBladeNames_329[15] = {
#include "sub_main/004A1CE8.inc"
};
char gBladeNames_330[15] = {
#include "sub_main/004A1CF8.inc"
};
char gBladeNames_331[15] = {
#include "sub_main/004A1D08.inc"
};
char gBladeNames_332[15] = {
#include "sub_main/004A1D18.inc"
};
char gBladeNames_333[12] = {
#include "sub_main/004A1D28.inc"
};
char gBladeNames_334[13] = {
#include "sub_main/004A1D38.inc"
};
char gBladeNames_335[12] = {
#include "sub_main/004A1D48.inc"
};
char gBladeNames_336[32] = {
#include "sub_main/004A1D60.inc"
};
char gBladeNames_337[24] = {
#include "sub_main/004A1D80.inc"
};
char gBladeNames_338[15] = {
#include "sub_main/004A1D98.inc"
};
char gBladeNames_339[32] = {
#include "sub_main/004A1DB0.inc"
};
char gBladeNames_340[32] = {
#include "sub_main/004A1DD0.inc"
};
char gBladeNames_341[24] = {
#include "sub_main/004A1DF0.inc"
};
char gBladeNames_342[15] = {
#include "sub_main/004A1E08.inc"
};
char gBladeNames_343[8] = {
#include "sub_main/004A1E18.inc"
};
char gBladeNames_344[16] = {
#include "sub_main/004A1E20.inc"
};
char gBladeNames_345[8] = {
#include "sub_main/004A1E30.inc"
};
char gBladeNames_346[15] = {
#include "sub_main/004A1E38.inc"
};
char gBladeNames_347[32] = {
#include "sub_main/004A1E50.inc"
};
char gBladeNames_348[24] = {
#include "sub_main/004A1E70.inc"
};
char gBladeNames_349[15] = {
#include "sub_main/004A1E88.inc"
};
char gBladeNames_350[32] = {
#include "sub_main/004A1EA0.inc"
};
char gBladeNames_351[24] = {
#include "sub_main/004A1EC0.inc"
};
char gBladeNames_352[15] = {
#include "sub_main/004A1ED8.inc"
};
char gBladeNames_353[15] = {
#include "sub_main/004A1EE8.inc"
};
char gBladeNames_354[15] = {
#include "sub_main/004A1EF8.inc"
};
char gBladeNames_355[24] = {
#include "sub_main/004A1F10.inc"
};
char gBladeNames_356[15] = {
#include "sub_main/004A1F28.inc"
};
char gBladeNames_357[15] = {
#include "sub_main/004A1F38.inc"
};
char gBladeNames_358[13] = {
#include "sub_main/004A1F48.inc"
};
char gBladeNames_359[32] = {
#include "sub_main/004A1F60.inc"
};
char gBladeNames_360[32] = {
#include "sub_main/004A1F80.inc"
};
char gBladeNames_361[32] = {
#include "sub_main/004A1FA0.inc"
};
char gBladeNames_362[24] = {
#include "sub_main/004A1FC0.inc"
};
char gBladeNames_363[13] = {
#include "sub_main/004A1FD8.inc"
};
char gBladeNames_364[32] = {
#include "sub_main/004A1FF0.inc"
};
char gBladeNames_365[24] = {
#include "sub_main/004A2010.inc"
};
char gBladeNames_366[12] = {
#include "sub_main/004A2028.inc"
};
char gBladeNames_367[13] = {
#include "sub_main/004A2038.inc"
};
char gBladeNames_368[32] = {
#include "sub_main/004A2050.inc"
};
char gBladeNames_369[24] = {
#include "sub_main/004A2070.inc"
};
char gBladeNames_370[12] = {
#include "sub_main/004A2088.inc"
};
char gBladeNames_371[8] = {
#include "sub_main/004A2098.inc"
};
char gBladeNames_372[32] = {
#include "sub_main/004A20A0.inc"
};
char gBladeNames_373[24] = {
#include "sub_main/004A20C0.inc"
};
char gBladeNames_374[12] = {
#include "sub_main/004A20D8.inc"
};
char gBladeNames_375[24] = {
#include "sub_main/004A20F0.inc"
};
char gBladeNames_376[12] = {
#include "sub_main/004A2108.inc"
};
char gBladeNames_377[15] = {
#include "sub_main/004A2118.inc"
};
char gBladeNames_378[13] = {
#include "sub_main/004A2128.inc"
};
char gBladeNames_379[13] = {
#include "sub_main/004A2138.inc"
};
char gBladeNames_380[32] = {
#include "sub_main/004A2150.inc"
};
char gBladeNames_381[32] = {
#include "sub_main/004A2170.inc"
};
char gBladeNames_382[24] = {
#include "sub_main/004A2190.inc"
};
char gBladeNames_383[13] = {
#include "sub_main/004A21A8.inc"
};
char gBladeNames_384[15] = {
#include "sub_main/004A21B8.inc"
};
char gBladeNames_385[32] = {
#include "sub_main/004A21D0.inc"
};
char gBladeNames_386[32] = {
#include "sub_main/004A21F0.inc"
};
char gBladeNames_387[32] = {
#include "sub_main/004A2210.inc"
};
char gBladeNames_388[24] = {
#include "sub_main/004A2230.inc"
};
char gBladeNames_389[13] = {
#include "sub_main/004A2248.inc"
};
char gBladeNames_390[12] = {
#include "sub_main/004A2258.inc"
};
char gBladeNames_391[13] = {
#include "sub_main/004A2268.inc"
};
char gBladeNames_392[15] = {
#include "sub_main/004A2278.inc"
};
char gBladeNames_393[15] = {
#include "sub_main/004A2288.inc"
};
char gBladeNames_394[32] = {
#include "sub_main/004A22A0.inc"
};
char gBladeNames_395[32] = {
#include "sub_main/004A22C0.inc"
};
char gBladeNames_396[32] = {
#include "sub_main/004A22E0.inc"
};
char gBladeNames_397[24] = {
#include "sub_main/004A2300.inc"
};
char gBladeNames_398[13] = {
#include "sub_main/004A2318.inc"
};
char gBladeNames_399[24] = {
#include "sub_main/004A2330.inc"
};
char gBladeNames_400[15] = {
#include "sub_main/004A2348.inc"
};
char gBladeNames_401[32] = {
#include "sub_main/004A2360.inc"
};
char gBladeNames_402[32] = {
#include "sub_main/004A2380.inc"
};
char gBladeNames_403[32] = {
#include "sub_main/004A23A0.inc"
};
char gBladeNames_404[32] = {
#include "sub_main/004A23C0.inc"
};
char gBladeNames_405[24] = {
#include "sub_main/004A23E0.inc"
};
char gBladeNames_406[13] = {
#include "sub_main/004A23F8.inc"
};
char gBladeNames_407[15] = {
#include "sub_main/004A2408.inc"
};
char gBladeNames_408[32] = {
#include "sub_main/004A2420.inc"
};
char gBladeNames_409[32] = {
#include "sub_main/004A2440.inc"
};
char gBladeNames_410[24] = {
#include "sub_main/004A2460.inc"
};
char gBladeNames_411[15] = {
#include "sub_main/004A2478.inc"
};
char gBladeNames_412[15] = {
#include "sub_main/004A2488.inc"
};
char gBladeNames_413[32] = {
#include "sub_main/004A24A0.inc"
};
char gBladeNames_414[24] = {
#include "sub_main/004A24C0.inc"
};
char gBladeNames_415[12] = {
#include "sub_main/004A24D8.inc"
};
char gBladeNames_416[32] = {
#include "sub_main/004A24F0.inc"
};
char gBladeNames_417[32] = {
#include "sub_main/004A2510.inc"
};
char gBladeNames_418[32] = {
#include "sub_main/004A2530.inc"
};
char gBladeNames_419[32] = {
#include "sub_main/004A2550.inc"
};
char gBladeNames_420[32] = {
#include "sub_main/004A2570.inc"
};
char gBladeNames_421[32] = {
#include "sub_main/004A2590.inc"
};
char gBladeNames_422[32] = {
#include "sub_main/004A25B0.inc"
};
char gBladeNames_423[32] = {
#include "sub_main/004A25D0.inc"
};
char gBladeNames_424[32] = {
#include "sub_main/004A25F0.inc"
};
char gBladeNames_425[24] = {
#include "sub_main/004A2610.inc"
};
char gBladeNames_426[12] = {
#include "sub_main/004A2628.inc"
};
char gBladeNames_427[24] = {
#include "sub_main/004A2640.inc"
};
char gBladeNames_428[12] = {
#include "sub_main/004A2658.inc"
};
char gBladeNames_429[32] = {
#include "sub_main/004A2670.inc"
};
char gBladeNames_430[32] = {
#include "sub_main/004A2690.inc"
};
char gBladeNames_431[24] = {
#include "sub_main/004A26B0.inc"
};
char gBladeNames_432[15] = {
#include "sub_main/004A26C8.inc"
};
char gBladeNames_433[32] = {
#include "sub_main/004A26E0.inc"
};
char gBladeNames_434[32] = {
#include "sub_main/004A2700.inc"
};
char gBladeNames_435[32] = {
#include "sub_main/004A2720.inc"
};
char gBladeNames_436[32] = {
#include "sub_main/004A2740.inc"
};
char gBladeNames_437[32] = {
#include "sub_main/004A2760.inc"
};
char gBladeNames_438[32] = {
#include "sub_main/004A2780.inc"
};
char gBladeNames_439[32] = {
#include "sub_main/004A27A0.inc"
};
char gBladeNames_440[24] = {
#include "sub_main/004A27C0.inc"
};
char gBladeNames_441[12] = {
#include "sub_main/004A27D8.inc"
};
char gBladeNames_442[24] = {
#include "sub_main/004A27F0.inc"
};
char gBladeNames_443[13] = {
#include "sub_main/004A2808.inc"
};
char gBladeNames_444[24] = {
#include "sub_main/004A2820.inc"
};
char gBladeNames_445[12] = {
#include "sub_main/004A2838.inc"
};
char gBladeNames_446[15] = {
#include "sub_main/004A2848.inc"
};
char gBladeNames_447[24] = {
#include "sub_main/004A2860.inc"
};
char gBladeNames_448[13] = {
#include "sub_main/004A2878.inc"
};
char gBladeNames_449[12] = {
#include "sub_main/004A2888.inc"
};
char gBladeNames_450[32] = {
#include "sub_main/004A28A0.inc"
};
char gBladeNames_451[32] = {
#include "sub_main/004A28C0.inc"
};
char gBladeNames_452[24] = {
#include "sub_main/004A28E0.inc"
};
char gBladeNames_453[15] = {
#include "sub_main/004A28F8.inc"
};
char gBladeNames_454[13] = {
#include "sub_main/004A2908.inc"
};
char gBladeNames_455[32] = {
#include "sub_main/004A2920.inc"
};
char gBladeNames_456[24] = {
#include "sub_main/004A2940.inc"
};
char gBladeNames_457[12] = {
#include "sub_main/004A2958.inc"
};
char gBladeNames_458[15] = {
#include "sub_main/004A2968.inc"
};
char gBladeNames_459[12] = {
#include "sub_main/004A2978.inc"
};
char gBladeNames_460[15] = {
#include "sub_main/004A2988.inc"
};
char gBladeNames_461[32] = {
#include "sub_main/004A29A0.inc"
};
char gBladeNames_462[24] = {
#include "sub_main/004A29C0.inc"
};
char gBladeNames_463[8] = {
#include "sub_main/004A29D8.inc"
};
char gBladeNames_464[16] = {
#include "sub_main/004A29E0.inc"
};
char gBladeNames_465[32] = {
#include "sub_main/004A29F0.inc"
};
char gBladeNames_466[24] = {
#include "sub_main/004A2A10.inc"
};
char gBladeNames_467[12] = {
#include "sub_main/004A2A28.inc"
};
char gBladeNames_468[32] = {
#include "sub_main/004A2A40.inc"
};
char gBladeNames_469[32] = {
#include "sub_main/004A2A60.inc"
};
char gBladeNames_470[24] = {
#include "sub_main/004A2A80.inc"
};
char gBladeNames_471[8] = {
#include "sub_main/004A2A98.inc"
};
char gBladeNames_472[24] = {
#include "sub_main/004A2AA0.inc"
};
char gBladeNames_473[13] = {
#include "sub_main/004A2AB8.inc"
};
char gBladeNames_474[13] = {
#include "sub_main/004A2AC8.inc"
};
char gBladeNames_475[32] = {
#include "sub_main/004A2AE0.inc"
};
char gBladeNames_476[24] = {
#include "sub_main/004A2B00.inc"
};
char gBladeNames_477[15] = {
#include "sub_main/004A2B18.inc"
};
char gBladeNames_478[13] = {
#include "sub_main/004A2B28.inc"
};
char gBladeNames_479[8] = {
#include "sub_main/004A2B38.inc"
};
char gBladeNames_480[16] = {
#include "sub_main/004A2B40.inc"
};
char gBladeNames_481[32] = {
#include "sub_main/004A2B50.inc"
};
char gBladeNames_482[32] = {
#include "sub_main/004A2B70.inc"
};
char gBladeNames_483[24] = {
#include "sub_main/004A2B90.inc"
};
char gBladeNames_484[13] = {
#include "sub_main/004A2BA8.inc"
};
char gBladeNames_485[13] = {
#include "sub_main/004A2BB8.inc"
};
char gGunNames_1[15] = {
#include "sub_main/004A2BC8.inc"
};
char gGunNames_2[24] = {
#include "sub_main/004A2BE0.inc"
};
char gGunNames_3[15] = {
#include "sub_main/004A2BF8.inc"
};
char gGunNames_4[32] = {
#include "sub_main/004A2C10.inc"
};
char gGunNames_5[32] = {
#include "sub_main/004A2C30.inc"
};
char gGunNames_6[32] = {
#include "sub_main/004A2C50.inc"
};
char gGunNames_7[32] = {
#include "sub_main/004A2C70.inc"
};
char gGunNames_8[32] = {
#include "sub_main/004A2C90.inc"
};
char gGunNames_9[32] = {
#include "sub_main/004A2CB0.inc"
};
char gGunNames_10[32] = {
#include "sub_main/004A2CD0.inc"
};
char gGunNames_11[32] = {
#include "sub_main/004A2CF0.inc"
};
char gGunNames_12[24] = {
#include "sub_main/004A2D10.inc"
};
char gGunNames_13[13] = {
#include "sub_main/004A2D28.inc"
};
char gGunNames_14[15] = {
#include "sub_main/004A2D38.inc"
};
char gGunNames_15[13] = {
#include "sub_main/004A2D48.inc"
};
char gGunNames_16[32] = {
#include "sub_main/004A2D60.inc"
};
char gGunNames_17[24] = {
#include "sub_main/004A2D80.inc"
};
char gGunNames_18[15] = {
#include "sub_main/004A2D98.inc"
};
char gGunNames_19[24] = {
#include "sub_main/004A2DB0.inc"
};
char gGunNames_20[13] = {
#include "sub_main/004A2DC8.inc"
};
char gGunNames_21[12] = {
#include "sub_main/004A2DD8.inc"
};
char gGunNames_22[24] = {
#include "sub_main/004A2DF0.inc"
};
char gGunNames_23[15] = {
#include "sub_main/004A2E08.inc"
};
char gGunNames_24[8] = {
#include "sub_main/004A2E18.inc"
};
char gGunNames_25[24] = {
#include "sub_main/004A2E20.inc"
};
char gGunNames_26[15] = {
#include "sub_main/004A2E38.inc"
};
char gGunNames_27[32] = {
#include "sub_main/004A2E50.inc"
};
char gGunNames_28[32] = {
#include "sub_main/004A2E70.inc"
};
char gGunNames_29[32] = {
#include "sub_main/004A2E90.inc"
};
char gGunNames_30[32] = {
#include "sub_main/004A2EB0.inc"
};
char gGunNames_31[32] = {
#include "sub_main/004A2ED0.inc"
};
char gGunNames_32[32] = {
#include "sub_main/004A2EF0.inc"
};
char gGunNames_33[32] = {
#include "sub_main/004A2F10.inc"
};
char gGunNames_34[32] = {
#include "sub_main/004A2F30.inc"
};
char gGunNames_35[32] = {
#include "sub_main/004A2F50.inc"
};
char gGunNames_36[24] = {
#include "sub_main/004A2F70.inc"
};
char gGunNames_37[12] = {
#include "sub_main/004A2F88.inc"
};
char gGunNames_38[12] = {
#include "sub_main/004A2F98.inc"
};
char gGunNames_39[24] = {
#include "sub_main/004A2FB0.inc"
};
char gGunNames_40[15] = {
#include "sub_main/004A2FC8.inc"
};
char gGunNames_41[12] = {
#include "sub_main/004A2FD8.inc"
};
char gGunNames_42[32] = {
#include "sub_main/004A2FF0.inc"
};
char gGunNames_43[32] = {
#include "sub_main/004A3010.inc"
};
char gGunNames_44[32] = {
#include "sub_main/004A3030.inc"
};
char gGunNames_45[32] = {
#include "sub_main/004A3050.inc"
};
char gGunNames_46[24] = {
#include "sub_main/004A3070.inc"
};
char gGunNames_47[15] = {
#include "sub_main/004A3088.inc"
};
char gGunNames_48[32] = {
#include "sub_main/004A30A0.inc"
};
char gGunNames_49[32] = {
#include "sub_main/004A30C0.inc"
};
char gGunNames_50[32] = {
#include "sub_main/004A30E0.inc"
};
char gGunNames_51[32] = {
#include "sub_main/004A3100.inc"
};
char gGunNames_52[24] = {
#include "sub_main/004A3120.inc"
};
char gGunNames_53[15] = {
#include "sub_main/004A3138.inc"
};
char gGunNames_54[13] = {
#include "sub_main/004A3148.inc"
};
char gGunNames_55[32] = {
#include "sub_main/004A3160.inc"
};
char gGunNames_56[24] = {
#include "sub_main/004A3180.inc"
};
char gGunNames_57[8] = {
#include "sub_main/004A3198.inc"
};
char gGunNames_58[32] = {
#include "sub_main/004A31A0.inc"
};
char gGunNames_59[32] = {
#include "sub_main/004A31C0.inc"
};
char gGunNames_60[32] = {
#include "sub_main/004A31E0.inc"
};
char gGunNames_61[24] = {
#include "sub_main/004A3200.inc"
};
char gGunNames_62[15] = {
#include "sub_main/004A3218.inc"
};
char gGunNames_63[15] = {
#include "sub_main/004A3228.inc"
};
char gGunNames_64[32] = {
#include "sub_main/004A3240.inc"
};
char gGunNames_65[32] = {
#include "sub_main/004A3260.inc"
};
char gGunNames_66[32] = {
#include "sub_main/004A3280.inc"
};
char gGunNames_67[32] = {
#include "sub_main/004A32A0.inc"
};
char gGunNames_68[24] = {
#include "sub_main/004A32C0.inc"
};
char gGunNames_69[15] = {
#include "sub_main/004A32D8.inc"
};
char gGunNames_70[15] = {
#include "sub_main/004A32E8.inc"
};
char gGunNames_71[24] = {
#include "sub_main/004A3300.inc"
};
char gGunNames_72[15] = {
#include "sub_main/004A3318.inc"
};
char gGunNames_73[13] = {
#include "sub_main/004A3328.inc"
};
char gGunNames_74[13] = {
#include "sub_main/004A3338.inc"
};
char gGunNames_75[15] = {
#include "sub_main/004A3348.inc"
};
char gGunNames_76[80] = {
#include "sub_main/004A3360.inc"
};
char gGunNames_80[32] = {
#include "sub_main/004A33B0.inc"
};
char gGunNames_81[32] = {
#include "sub_main/004A33D0.inc"
};
char gGunNames_82[32] = {
#include "sub_main/004A33F0.inc"
};
char gGunNames_83[48] = {
#include "sub_main/004A3410.inc"
};
char gGunNames_85[24] = {
#include "sub_main/004A3440.inc"
};
char gGunNames_86[13] = {
#include "sub_main/004A3458.inc"
};
char gGunNames_87[24] = {
#include "sub_main/004A3470.inc"
};
char gGunNames_88[13] = {
#include "sub_main/004A3488.inc"
};
char gGunNames_89[32] = {
#include "sub_main/004A34A0.inc"
};
char gGunNames_90[24] = {
#include "sub_main/004A34C0.inc"
};
char gGunNames_91[15] = {
#include "sub_main/004A34D8.inc"
};
char gGunNames_92[12] = {
#include "sub_main/004A34E8.inc"
};
char gGunNames_93[24] = {
#include "sub_main/004A3500.inc"
};
char gGunNames_94[15] = {
#include "sub_main/004A3518.inc"
};
char gGunNames_95[8] = {
#include "sub_main/004A3528.inc"
};
char gGunNames_96[16] = {
#include "sub_main/004A3530.inc"
};
char gGunNames_97[24] = {
#include "sub_main/004A3540.inc"
};
char gGunNames_98[15] = {
#include "sub_main/004A3558.inc"
};
char gGunNames_99[13] = {
#include "sub_main/004A3568.inc"
};
char gGunNames_100[15] = {
#include "sub_main/004A3578.inc"
};
char gGunNames_101[24] = {
#include "sub_main/004A3590.inc"
};
char gGunNames_102[15] = {
#include "sub_main/004A35A8.inc"
};
char gGunNames_103[32] = {
#include "sub_main/004A35C0.inc"
};
char gGunNames_104[24] = {
#include "sub_main/004A35E0.inc"
};
char gGunNames_105[12] = {
#include "sub_main/004A35F8.inc"
};
char gGunNames_106[13] = {
#include "sub_main/004A3608.inc"
};
char gGunNames_107[96] = {
#include "sub_main/004A3620.inc"
};
char gGunNames_112[32] = {
#include "sub_main/004A3680.inc"
};
char gGunNames_113[32] = {
#include "sub_main/004A36A0.inc"
};
char gGunNames_114[32] = {
#include "sub_main/004A36C0.inc"
};
char gGunNames_115[64] = {
#include "sub_main/004A36E0.inc"
};
char gGunNames_118[24] = {
#include "sub_main/004A3720.inc"
};
char gGunNames_119[13] = {
#include "sub_main/004A3738.inc"
};
char gGunNames_120[32] = {
#include "sub_main/004A3750.inc"
};
char gGunNames_121[24] = {
#include "sub_main/004A3770.inc"
};
char gGunNames_122[8] = {
#include "sub_main/004A3788.inc"
};
char gGunNames_123[16] = {
#include "sub_main/004A3790.inc"
};
char gGunNames_124[16] = {
#include "sub_main/004A37A0.inc"
};
char gGunNames_125[16] = {
#include "sub_main/004A37B0.inc"
};
char gGunNames_126[24] = {
#include "sub_main/004A37C0.inc"
};
char gGunNames_127[15] = {
#include "sub_main/004A37D8.inc"
};
char gArmorHeadNames_1[24] = {
#include "sub_main/004A37F0.inc"
};
char gArmorHeadNames_2[15] = {
#include "sub_main/004A3808.inc"
};
char gArmorHeadNames_3[15] = {
#include "sub_main/004A3818.inc"
};
char gArmorHeadNames_4[24] = {
#include "sub_main/004A3830.inc"
};
char gArmorHeadNames_5[13] = {
#include "sub_main/004A3848.inc"
};
char gArmorHeadNames_6[15] = {
#include "sub_main/004A3858.inc"
};
char gArmorHeadNames_7[15] = {
#include "sub_main/004A3868.inc"
};
char gArmorHeadNames_8[24] = {
#include "sub_main/004A3880.inc"
};
char gArmorHeadNames_9[13] = {
#include "sub_main/004A3898.inc"
};
char gArmorHeadNames_10[15] = {
#include "sub_main/004A38A8.inc"
};
char gArmorHeadNames_11[13] = {
#include "sub_main/004A38B8.inc"
};
char gArmorHeadNames_12[15] = {
#include "sub_main/004A38C8.inc"
};
char gArmorHeadNames_13[13] = {
#include "sub_main/004A38D8.inc"
};
char gArmorHeadNames_14[15] = {
#include "sub_main/004A38E8.inc"
};
char gArmorHeadNames_15[15] = {
#include "sub_main/004A38F8.inc"
};
char gArmorHeadNames_16[24] = {
#include "sub_main/004A3910.inc"
};
char gArmorHeadNames_17[15] = {
#include "sub_main/004A3928.inc"
};
char gArmorHeadNames_18[24] = {
#include "sub_main/004A3940.inc"
};
char gArmorHeadNames_19[15] = {
#include "sub_main/004A3958.inc"
};
char gArmorHeadNames_20[24] = {
#include "sub_main/004A3970.inc"
};
char gArmorHeadNames_21[15] = {
#include "sub_main/004A3988.inc"
};
char gArmorHeadNames_22[24] = {
#include "sub_main/004A39A0.inc"
};
char gArmorHeadNames_23[15] = {
#include "sub_main/004A39B8.inc"
};
char gArmorHeadNames_24[24] = {
#include "sub_main/004A39D0.inc"
};
char gArmorHeadNames_25[13] = {
#include "sub_main/004A39E8.inc"
};
char gArmorHeadNames_26[15] = {
#include "sub_main/004A39F8.inc"
};
char gArmorHeadNames_27[15] = {
#include "sub_main/004A3A08.inc"
};
char gArmorHeadNames_28[24] = {
#include "sub_main/004A3A20.inc"
};
char gArmorHeadNames_29[13] = {
#include "sub_main/004A3A38.inc"
};
char gArmorHeadNames_30[15] = {
#include "sub_main/004A3A48.inc"
};
char gArmorHeadNames_31[13] = {
#include "sub_main/004A3A58.inc"
};
char gArmorHeadNames_32[15] = {
#include "sub_main/004A3A68.inc"
};
char gArmorHeadNames_33[24] = {
#include "sub_main/004A3A80.inc"
};
char gArmorHeadNames_34[13] = {
#include "sub_main/004A3A98.inc"
};
char gArmorHeadNames_35[15] = {
#include "sub_main/004A3AA8.inc"
};
char gArmorHeadNames_36[32] = {
#include "sub_main/004A3AC0.inc"
};
char gArmorHeadNames_37[32] = {
#include "sub_main/004A3AE0.inc"
};
char gArmorHeadNames_38[32] = {
#include "sub_main/004A3B00.inc"
};
char gArmorHeadNames_39[24] = {
#include "sub_main/004A3B20.inc"
};
char gArmorHeadNames_40[15] = {
#include "sub_main/004A3B38.inc"
};
char gArmorHeadNames_41[24] = {
#include "sub_main/004A3B50.inc"
};
char gArmorHeadNames_42[15] = {
#include "sub_main/004A3B68.inc"
};
char gArmorHeadNames_43[13] = {
#include "sub_main/004A3B78.inc"
};
char gArmorHeadNames_44[24] = {
#include "sub_main/004A3B90.inc"
};
char gArmorHeadNames_45[12] = {
#include "sub_main/004A3BA8.inc"
};
char gArmorHeadNames_46[24] = {
#include "sub_main/004A3BC0.inc"
};
char gArmorHeadNames_47[15] = {
#include "sub_main/004A3BD8.inc"
};
char gArmorHeadNames_48[32] = {
#include "sub_main/004A3BF0.inc"
};
char gArmorHeadNames_49[32] = {
#include "sub_main/004A3C10.inc"
};
char gArmorHeadNames_50[32] = {
#include "sub_main/004A3C30.inc"
};
char gArmorHeadNames_51[32] = {
#include "sub_main/004A3C50.inc"
};
char gArmorHeadNames_52[24] = {
#include "sub_main/004A3C70.inc"
};
char gArmorHeadNames_53[13] = {
#include "sub_main/004A3C88.inc"
};
char gArmorHeadNames_54[13] = {
#include "sub_main/004A3C98.inc"
};
char gArmorHeadNames_55[15] = {
#include "sub_main/004A3CA8.inc"
};
char gArmorHeadNames_56[15] = {
#include "sub_main/004A3CB8.inc"
};
char gArmorHeadNames_57[12] = {
#include "sub_main/004A3CC8.inc"
};
char gArmorHeadNames_58[12] = {
#include "sub_main/004A3CD8.inc"
};
char gArmorHeadNames_59[32] = {
#include "sub_main/004A3CF0.inc"
};
char gArmorHeadNames_60[24] = {
#include "sub_main/004A3D10.inc"
};
char gArmorHeadNames_61[15] = {
#include "sub_main/004A3D28.inc"
};
char gArmorHeadNames_62[15] = {
#include "sub_main/004A3D38.inc"
};
char gArmorHeadNames_63[15] = {
#include "sub_main/004A3D48.inc"
};
char gArmorHeadNames_64[15] = {
#include "sub_main/004A3D58.inc"
};
char gArmorHeadNames_65[32] = {
#include "sub_main/004A3D70.inc"
};
char gArmorHeadNames_66[24] = {
#include "sub_main/004A3D90.inc"
};
char gArmorHeadNames_67[12] = {
#include "sub_main/004A3DA8.inc"
};
char gArmorHeadNames_68[15] = {
#include "sub_main/004A3DB8.inc"
};
char gArmorHeadNames_69[12] = {
#include "sub_main/004A3DC8.inc"
};
char gArmorHeadNames_70[12] = {
#include "sub_main/004A3DD8.inc"
};
char gArmorHeadNames_71[15] = {
#include "sub_main/004A3DE8.inc"
};
char gArmorHeadNames_72[32] = {
#include "sub_main/004A3E00.inc"
};
char gArmorHeadNames_73[32] = {
#include "sub_main/004A3E20.inc"
};
char gArmorHeadNames_74[32] = {
#include "sub_main/004A3E40.inc"
};
char gArmorHeadNames_75[32] = {
#include "sub_main/004A3E60.inc"
};
char gArmorHeadNames_76[24] = {
#include "sub_main/004A3E80.inc"
};
char gArmorHeadNames_77[15] = {
#include "sub_main/004A3E98.inc"
};
char gArmorHeadNames_78[32] = {
#include "sub_main/004A3EB0.inc"
};
char gArmorHeadNames_79[32] = {
#include "sub_main/004A3ED0.inc"
};
char gArmorHeadNames_80[24] = {
#include "sub_main/004A3EF0.inc"
};
char gArmorHeadNames_81[15] = {
#include "sub_main/004A3F08.inc"
};
char gArmorHeadNames_82[24] = {
#include "sub_main/004A3F20.inc"
};
char gArmorHeadNames_83[15] = {
#include "sub_main/004A3F38.inc"
};
char gArmorHeadNames_84[24] = {
#include "sub_main/004A3F50.inc"
};
char gArmorHeadNames_85[15] = {
#include "sub_main/004A3F68.inc"
};
char gArmorHeadNames_86[32] = {
#include "sub_main/004A3F80.inc"
};
char gArmorHeadNames_87[32] = {
#include "sub_main/004A3FA0.inc"
};
char gArmorHeadNames_88[32] = {
#include "sub_main/004A3FC0.inc"
};
char gArmorHeadNames_89[32] = {
#include "sub_main/004A3FE0.inc"
};
char gArmorHeadNames_90[32] = {
#include "sub_main/004A4000.inc"
};
char gArmorHeadNames_91[32] = {
#include "sub_main/004A4020.inc"
};
char gArmorHeadNames_92[32] = {
#include "sub_main/004A4040.inc"
};
char gArmorHeadNames_93[32] = {
#include "sub_main/004A4060.inc"
};
char gArmorHeadNames_94[32] = {
#include "sub_main/004A4080.inc"
};
char gArmorHeadNames_95[32] = {
#include "sub_main/004A40A0.inc"
};
char gArmorHeadNames_96[32] = {
#include "sub_main/004A40C0.inc"
};
char gArmorHeadNames_97[32] = {
#include "sub_main/004A40E0.inc"
};
char gArmorHeadNames_98[32] = {
#include "sub_main/004A4100.inc"
};
char gArmorHeadNames_99[32] = {
#include "sub_main/004A4120.inc"
};
char gArmorHeadNames_100[32] = {
#include "sub_main/004A4140.inc"
};
char gArmorHeadNames_101[32] = {
#include "sub_main/004A4160.inc"
};
char gArmorHeadNames_102[24] = {
#include "sub_main/004A4180.inc"
};
char gArmorHeadNames_103[15] = {
#include "sub_main/004A4198.inc"
};
char gArmorHeadNames_104[24] = {
#include "sub_main/004A41B0.inc"
};
char gArmorHeadNames_105[15] = {
#include "sub_main/004A41C8.inc"
};
char gArmorHeadNames_106[32] = {
#include "sub_main/004A41E0.inc"
};
char gArmorHeadNames_107[32] = {
#include "sub_main/004A4200.inc"
};
char gArmorHeadNames_108[32] = {
#include "sub_main/004A4220.inc"
};
char gArmorHeadNames_109[32] = {
#include "sub_main/004A4240.inc"
};
char gArmorHeadNames_110[32] = {
#include "sub_main/004A4260.inc"
};
char gArmorHeadNames_111[32] = {
#include "sub_main/004A4280.inc"
};
char gArmorHeadNames_112[24] = {
#include "sub_main/004A42A0.inc"
};
char gArmorHeadNames_113[12] = {
#include "sub_main/004A42B8.inc"
};
char gArmorHeadNames_114[12] = {
#include "sub_main/004A42C8.inc"
};
char gArmorHeadNames_115[32] = {
#include "sub_main/004A42E0.inc"
};
char gArmorHeadNames_116[32] = {
#include "sub_main/004A4300.inc"
};
char gArmorHeadNames_117[32] = {
#include "sub_main/004A4320.inc"
};
char gArmorHeadNames_118[24] = {
#include "sub_main/004A4340.inc"
};
char gArmorHeadNames_119[15] = {
#include "sub_main/004A4358.inc"
};
char gArmorHeadNames_120[24] = {
#include "sub_main/004A4370.inc"
};
char gArmorHeadNames_121[15] = {
#include "sub_main/004A4388.inc"
};
char gArmorHeadNames_122[15] = {
#include "sub_main/004A4398.inc"
};
char gArmorHeadNames_123[32] = {
#include "sub_main/004A43B0.inc"
};
char gArmorHeadNames_124[32] = {
#include "sub_main/004A43D0.inc"
};
char gArmorHeadNames_125[32] = {
#include "sub_main/004A43F0.inc"
};
char gArmorHeadNames_126[32] = {
#include "sub_main/004A4410.inc"
};
char gArmorHeadNames_127[32] = {
#include "sub_main/004A4430.inc"
};
char gArmorHeadNames_128[32] = {
#include "sub_main/004A4450.inc"
};
char gArmorHeadNames_129[32] = {
#include "sub_main/004A4470.inc"
};
char gArmorHeadNames_130[32] = {
#include "sub_main/004A4490.inc"
};
char gArmorHeadNames_131[32] = {
#include "sub_main/004A44B0.inc"
};
char gArmorHeadNames_132[24] = {
#include "sub_main/004A44D0.inc"
};
char gArmorHeadNames_133[15] = {
#include "sub_main/004A44E8.inc"
};
char gArmorHeadNames_134[32] = {
#include "sub_main/004A4500.inc"
};
char gArmorHeadNames_135[32] = {
#include "sub_main/004A4520.inc"
};
char gArmorHeadNames_136[24] = {
#include "sub_main/004A4540.inc"
};
char gArmorHeadNames_137[15] = {
#include "sub_main/004A4558.inc"
};
char gArmorHeadNames_138[24] = {
#include "sub_main/004A4570.inc"
};
char gArmorHeadNames_139[15] = {
#include "sub_main/004A4588.inc"
};
char gArmorHeadNames_140[24] = {
#include "sub_main/004A45A0.inc"
};
char gArmorHeadNames_141[15] = {
#include "sub_main/004A45B8.inc"
};
char gArmorHeadNames_142[32] = {
#include "sub_main/004A45D0.inc"
};
char gArmorHeadNames_143[32] = {
#include "sub_main/004A45F0.inc"
};
char gArmorHeadNames_144[32] = {
#include "sub_main/004A4610.inc"
};
char gArmorHeadNames_145[32] = {
#include "sub_main/004A4630.inc"
};
char gArmorHeadNames_146[24] = {
#include "sub_main/004A4650.inc"
};
char gArmorHeadNames_147[15] = {
#include "sub_main/004A4668.inc"
};
char gArmorHeadNames_148[32] = {
#include "sub_main/004A4680.inc"
};
char gArmorHeadNames_149[32] = {
#include "sub_main/004A46A0.inc"
};
char gArmorHeadNames_150[32] = {
#include "sub_main/004A46C0.inc"
};
char gArmorHeadNames_151[32] = {
#include "sub_main/004A46E0.inc"
};
char gArmorHeadNames_152[32] = {
#include "sub_main/004A4700.inc"
};
char gArmorHeadNames_153[32] = {
#include "sub_main/004A4720.inc"
};
char gArmorHeadNames_154[24] = {
#include "sub_main/004A4740.inc"
};
char gArmorHeadNames_155[15] = {
#include "sub_main/004A4758.inc"
};
char gArmorHeadNames_156[32] = {
#include "sub_main/004A4770.inc"
};
char gArmorHeadNames_157[32] = {
#include "sub_main/004A4790.inc"
};
char gArmorHeadNames_158[24] = {
#include "sub_main/004A47B0.inc"
};
char gArmorHeadNames_159[15] = {
#include "sub_main/004A47C8.inc"
};
char gArmorHeadNames_160[24] = {
#include "sub_main/004A47E0.inc"
};
char gArmorHeadNames_161[15] = {
#include "sub_main/004A47F8.inc"
};
char gArmorHeadNames_162[24] = {
#include "sub_main/004A4810.inc"
};
char gArmorHeadNames_163[15] = {
#include "sub_main/004A4828.inc"
};
char gArmorHeadNames_164[32] = {
#include "sub_main/004A4840.inc"
};
char gArmorHeadNames_165[32] = {
#include "sub_main/004A4860.inc"
};
char gArmorHeadNames_166[32] = {
#include "sub_main/004A4880.inc"
};
char gArmorHeadNames_167[32] = {
#include "sub_main/004A48A0.inc"
};
char gArmorHeadNames_168[32] = {
#include "sub_main/004A48C0.inc"
};
char gArmorHeadNames_169[32] = {
#include "sub_main/004A48E0.inc"
};
char gArmorHeadNames_170[24] = {
#include "sub_main/004A4900.inc"
};
char gArmorHeadNames_171[12] = {
#include "sub_main/004A4918.inc"
};
char gArmorHeadNames_172[12] = {
#include "sub_main/004A4928.inc"
};
char gArmorHeadNames_173[32] = {
#include "sub_main/004A4940.inc"
};
char gArmorHeadNames_174[32] = {
#include "sub_main/004A4960.inc"
};
char gArmorHeadNames_175[32] = {
#include "sub_main/004A4980.inc"
};
char gArmorHeadNames_176[24] = {
#include "sub_main/004A49A0.inc"
};
char gArmorHeadNames_177[15] = {
#include "sub_main/004A49B8.inc"
};
char gArmorHeadNames_178[24] = {
#include "sub_main/004A49D0.inc"
};
char gArmorHeadNames_179[15] = {
#include "sub_main/004A49E8.inc"
};
char gArmorHeadNames_180[15] = {
#include "sub_main/004A49F8.inc"
};
char gArmorHeadNames_181[32] = {
#include "sub_main/004A4A10.inc"
};
char gArmorHeadNames_182[24] = {
#include "sub_main/004A4A30.inc"
};
char gArmorHeadNames_183[13] = {
#include "sub_main/004A4A48.inc"
};
char gArmorHeadNames_184[15] = {
#include "sub_main/004A4A58.inc"
};
char gArmorHeadNames_185[15] = {
#include "sub_main/004A4A68.inc"
};
char gArmorHeadNames_186[24] = {
#include "sub_main/004A4A80.inc"
};
char gArmorHeadNames_187[15] = {
#include "sub_main/004A4A98.inc"
};
char gArmorHeadNames_188[24] = {
#include "sub_main/004A4AB0.inc"
};
char gArmorHeadNames_189[13] = {
#include "sub_main/004A4AC8.inc"
};
char gArmorHeadNames_190[15] = {
#include "sub_main/004A4AD8.inc"
};
char gArmorHeadNames_191[15] = {
#include "sub_main/004A4AE8.inc"
};
char gArmorHeadNames_192[24] = {
#include "sub_main/004A4B00.inc"
};
char gArmorHeadNames_193[15] = {
#include "sub_main/004A4B18.inc"
};
char gArmorHeadNames_194[24] = {
#include "sub_main/004A4B30.inc"
};
char gArmorHeadNames_195[15] = {
#include "sub_main/004A4B48.inc"
};
char gArmorHeadNames_196[32] = {
#include "sub_main/004A4B60.inc"
};
char gArmorHeadNames_197[32] = {
#include "sub_main/004A4B80.inc"
};
char gArmorHeadNames_198[32] = {
#include "sub_main/004A4BA0.inc"
};
char gArmorHeadNames_199[32] = {
#include "sub_main/004A4BC0.inc"
};
char gArmorHeadNames_200[24] = {
#include "sub_main/004A4BE0.inc"
};
char gArmorHeadNames_201[13] = {
#include "sub_main/004A4BF8.inc"
};
char gArmorHeadNames_202[15] = {
#include "sub_main/004A4C08.inc"
};
char gArmorHeadNames_203[15] = {
#include "sub_main/004A4C18.inc"
};
char gArmorHeadNames_204[24] = {
#include "sub_main/004A4C30.inc"
};
char gArmorHeadNames_205[15] = {
#include "sub_main/004A4C48.inc"
};
char gArmorHeadNames_206[24] = {
#include "sub_main/004A4C60.inc"
};
char gArmorHeadNames_207[13] = {
#include "sub_main/004A4C78.inc"
};
char gArmorHeadNames_208[13] = {
#include "sub_main/004A4C88.inc"
};
char gArmorHeadNames_209[15] = {
#include "sub_main/004A4C98.inc"
};
char gArmorHeadNames_210[15] = {
#include "sub_main/004A4CA8.inc"
};
char gArmorHeadNames_211[15] = {
#include "sub_main/004A4CB8.inc"
};
char gArmorHeadNames_212[15] = {
#include "sub_main/004A4CC8.inc"
};
char gArmorHeadNames_213[32] = {
#include "sub_main/004A4CE0.inc"
};
char gArmorHeadNames_214[32] = {
#include "sub_main/004A4D00.inc"
};
char gArmorHeadNames_215[32] = {
#include "sub_main/004A4D20.inc"
};
char gArmorHeadNames_216[24] = {
#include "sub_main/004A4D40.inc"
};
char gArmorHeadNames_217[12] = {
#include "sub_main/004A4D58.inc"
};
char gArmorHeadNames_218[12] = {
#include "sub_main/004A4D68.inc"
};
char gArmorHeadNames_219[24] = {
#include "sub_main/004A4D80.inc"
};
char gArmorHeadNames_220[15] = {
#include "sub_main/004A4D98.inc"
};
char gArmorHeadNames_221[13] = {
#include "sub_main/004A4DA8.inc"
};
char gArmorHeadNames_222[13] = {
#include "sub_main/004A4DB8.inc"
};
char gArmorHeadNames_223[12] = {
#include "sub_main/004A4DC8.inc"
};
char gArmorHeadNames_224[12] = {
#include "sub_main/004A4DD8.inc"
};
char gArmorHeadNames_225[32] = {
#include "sub_main/004A4DF0.inc"
};
char gArmorHeadNames_226[32] = {
#include "sub_main/004A4E10.inc"
};
char gArmorHeadNames_227[32] = {
#include "sub_main/004A4E30.inc"
};
char gArmorHeadNames_228[32] = {
#include "sub_main/004A4E50.inc"
};
char gArmorHeadNames_229[32] = {
#include "sub_main/004A4E70.inc"
};
char gArmorHeadNames_230[32] = {
#include "sub_main/004A4E90.inc"
};
char gArmorHeadNames_231[32] = {
#include "sub_main/004A4EB0.inc"
};
char gArmorHeadNames_232[32] = {
#include "sub_main/004A4ED0.inc"
};
char gArmorHeadNames_233[24] = {
#include "sub_main/004A4EF0.inc"
};
char gArmorHeadNames_234[15] = {
#include "sub_main/004A4F08.inc"
};
char gArmorHeadNames_235[32] = {
#include "sub_main/004A4F20.inc"
};
char gArmorHeadNames_236[32] = {
#include "sub_main/004A4F40.inc"
};
char gArmorHeadNames_237[32] = {
#include "sub_main/004A4F60.inc"
};
char gArmorHeadNames_238[32] = {
#include "sub_main/004A4F80.inc"
};
char gArmorHeadNames_239[24] = {
#include "sub_main/004A4FA0.inc"
};
char gArmorHeadNames_240[12] = {
#include "sub_main/004A4FB8.inc"
};
char gArmorHeadNames_241[12] = {
#include "sub_main/004A4FC8.inc"
};
char gArmorHeadNames_242[12] = {
#include "sub_main/004A4FD8.inc"
};
char gArmorHeadNames_243[13] = {
#include "sub_main/004A4FE8.inc"
};
char gArmorHeadNames_244[13] = {
#include "sub_main/004A4FF8.inc"
};
char gArmorHeadNames_245[13] = {
#include "sub_main/004A5008.inc"
};
char gArmorHeadNames_246[13] = {
#include "sub_main/004A5018.inc"
};
char gArmorHeadNames_247[13] = {
#include "sub_main/004A5028.inc"
};
char gArmorHeadNames_248[13] = {
#include "sub_main/004A5038.inc"
};
char gArmorHeadNames_249[13] = {
#include "sub_main/004A5048.inc"
};
char gArmorHeadNames_250[15] = {
#include "sub_main/004A5058.inc"
};
char gArmorHeadNames_251[24] = {
#include "sub_main/004A5070.inc"
};
char gArmorHeadNames_252[15] = {
#include "sub_main/004A5088.inc"
};
char gArmorBodyNames_1[24] = {
#include "sub_main/004A50A0.inc"
};
char gArmorBodyNames_2[15] = {
#include "sub_main/004A50B8.inc"
};
char gArmorBodyNames_3[15] = {
#include "sub_main/004A50C8.inc"
};
char gArmorBodyNames_4[24] = {
#include "sub_main/004A50E0.inc"
};
char gArmorBodyNames_5[13] = {
#include "sub_main/004A50F8.inc"
};
char gArmorBodyNames_6[15] = {
#include "sub_main/004A5108.inc"
};
char gArmorBodyNames_7[15] = {
#include "sub_main/004A5118.inc"
};
char gArmorBodyNames_8[24] = {
#include "sub_main/004A5130.inc"
};
char gArmorBodyNames_9[13] = {
#include "sub_main/004A5148.inc"
};
char gArmorBodyNames_10[15] = {
#include "sub_main/004A5158.inc"
};
char gArmorBodyNames_11[13] = {
#include "sub_main/004A5168.inc"
};
char gArmorBodyNames_12[15] = {
#include "sub_main/004A5178.inc"
};
char gArmorBodyNames_13[13] = {
#include "sub_main/004A5188.inc"
};
char gArmorBodyNames_14[15] = {
#include "sub_main/004A5198.inc"
};
char gArmorBodyNames_15[15] = {
#include "sub_main/004A51A8.inc"
};
char gArmorBodyNames_16[24] = {
#include "sub_main/004A51C0.inc"
};
char gArmorBodyNames_17[15] = {
#include "sub_main/004A51D8.inc"
};
char gArmorBodyNames_18[24] = {
#include "sub_main/004A51F0.inc"
};
char gArmorBodyNames_19[13] = {
#include "sub_main/004A5208.inc"
};
char gArmorBodyNames_20[15] = {
#include "sub_main/004A5218.inc"
};
char gArmorBodyNames_21[15] = {
#include "sub_main/004A5228.inc"
};
char gArmorBodyNames_22[24] = {
#include "sub_main/004A5240.inc"
};
char gArmorBodyNames_23[15] = {
#include "sub_main/004A5258.inc"
};
char gArmorBodyNames_24[24] = {
#include "sub_main/004A5270.inc"
};
char gArmorBodyNames_25[15] = {
#include "sub_main/004A5288.inc"
};
char gArmorBodyNames_26[32] = {
#include "sub_main/004A52A0.inc"
};
char gArmorBodyNames_27[24] = {
#include "sub_main/004A52C0.inc"
};
char gArmorBodyNames_28[13] = {
#include "sub_main/004A52D8.inc"
};
char gArmorBodyNames_29[15] = {
#include "sub_main/004A52E8.inc"
};
char gArmorBodyNames_30[15] = {
#include "sub_main/004A52F8.inc"
};
char gArmorBodyNames_31[24] = {
#include "sub_main/004A5310.inc"
};
char gArmorBodyNames_32[13] = {
#include "sub_main/004A5328.inc"
};
char gArmorBodyNames_33[15] = {
#include "sub_main/004A5338.inc"
};
char gArmorBodyNames_34[13] = {
#include "sub_main/004A5348.inc"
};
char gArmorBodyNames_35[15] = {
#include "sub_main/004A5358.inc"
};
char gArmorBodyNames_36[32] = {
#include "sub_main/004A5370.inc"
};
char gArmorBodyNames_37[24] = {
#include "sub_main/004A5390.inc"
};
char gArmorBodyNames_38[13] = {
#include "sub_main/004A53A8.inc"
};
char gArmorBodyNames_39[15] = {
#include "sub_main/004A53B8.inc"
};
char gArmorBodyNames_40[32] = {
#include "sub_main/004A53D0.inc"
};
char gArmorBodyNames_41[32] = {
#include "sub_main/004A53F0.inc"
};
char gArmorBodyNames_42[32] = {
#include "sub_main/004A5410.inc"
};
char gArmorBodyNames_43[24] = {
#include "sub_main/004A5430.inc"
};
char gArmorBodyNames_44[15] = {
#include "sub_main/004A5448.inc"
};
char gArmorBodyNames_45[24] = {
#include "sub_main/004A5460.inc"
};
char gArmorBodyNames_46[15] = {
#include "sub_main/004A5478.inc"
};
char gArmorBodyNames_47[13] = {
#include "sub_main/004A5488.inc"
};
char gArmorBodyNames_48[32] = {
#include "sub_main/004A54A0.inc"
};
char gArmorBodyNames_49[24] = {
#include "sub_main/004A54C0.inc"
};
char gArmorBodyNames_50[15] = {
#include "sub_main/004A54D8.inc"
};
char gArmorBodyNames_51[32] = {
#include "sub_main/004A54F0.inc"
};
char gArmorBodyNames_52[32] = {
#include "sub_main/004A5510.inc"
};
char gArmorBodyNames_53[24] = {
#include "sub_main/004A5530.inc"
};
char gArmorBodyNames_54[13] = {
#include "sub_main/004A5548.inc"
};
char gArmorBodyNames_55[13] = {
#include "sub_main/004A5558.inc"
};
char gArmorBodyNames_56[32] = {
#include "sub_main/004A5570.inc"
};
char gArmorBodyNames_57[24] = {
#include "sub_main/004A5590.inc"
};
char gArmorBodyNames_58[15] = {
#include "sub_main/004A55A8.inc"
};
char gArmorBodyNames_59[15] = {
#include "sub_main/004A55B8.inc"
};
char gArmorBodyNames_60[15] = {
#include "sub_main/004A55C8.inc"
};
char gArmorBodyNames_61[24] = {
#include "sub_main/004A55E0.inc"
};
char gArmorBodyNames_62[15] = {
#include "sub_main/004A55F8.inc"
};
char gArmorBodyNames_63[15] = {
#include "sub_main/004A5608.inc"
};
char gArmorBodyNames_64[13] = {
#include "sub_main/004A5618.inc"
};
char gArmorBodyNames_65[13] = {
#include "sub_main/004A5628.inc"
};
char gArmorBodyNames_66[15] = {
#include "sub_main/004A5638.inc"
};
char gArmorBodyNames_67[15] = {
#include "sub_main/004A5648.inc"
};
char gArmorBodyNames_68[32] = {
#include "sub_main/004A5660.inc"
};
char gArmorBodyNames_69[32] = {
#include "sub_main/004A5680.inc"
};
char gArmorBodyNames_70[32] = {
#include "sub_main/004A56A0.inc"
};
char gArmorBodyNames_71[24] = {
#include "sub_main/004A56C0.inc"
};
char gArmorBodyNames_72[15] = {
#include "sub_main/004A56D8.inc"
};
char gArmorBodyNames_73[32] = {
#include "sub_main/004A56F0.inc"
};
char gArmorBodyNames_74[32] = {
#include "sub_main/004A5710.inc"
};
char gArmorBodyNames_75[24] = {
#include "sub_main/004A5730.inc"
};
char gArmorBodyNames_76[15] = {
#include "sub_main/004A5748.inc"
};
char gArmorBodyNames_77[24] = {
#include "sub_main/004A5760.inc"
};
char gArmorBodyNames_78[15] = {
#include "sub_main/004A5778.inc"
};
char gArmorBodyNames_79[24] = {
#include "sub_main/004A5790.inc"
};
char gArmorBodyNames_80[15] = {
#include "sub_main/004A57A8.inc"
};
char gArmorBodyNames_81[32] = {
#include "sub_main/004A57C0.inc"
};
char gArmorBodyNames_82[32] = {
#include "sub_main/004A57E0.inc"
};
char gArmorBodyNames_83[32] = {
#include "sub_main/004A5800.inc"
};
char gArmorBodyNames_84[32] = {
#include "sub_main/004A5820.inc"
};
char gArmorBodyNames_85[24] = {
#include "sub_main/004A5840.inc"
};
char gArmorBodyNames_86[15] = {
#include "sub_main/004A5858.inc"
};
char gArmorBodyNames_87[32] = {
#include "sub_main/004A5870.inc"
};
char gArmorBodyNames_88[32] = {
#include "sub_main/004A5890.inc"
};
char gArmorBodyNames_89[32] = {
#include "sub_main/004A58B0.inc"
};
char gArmorBodyNames_90[32] = {
#include "sub_main/004A58D0.inc"
};
char gArmorBodyNames_91[32] = {
#include "sub_main/004A58F0.inc"
};
char gArmorBodyNames_92[32] = {
#include "sub_main/004A5910.inc"
};
char gArmorBodyNames_93[32] = {
#include "sub_main/004A5930.inc"
};
char gArmorBodyNames_94[32] = {
#include "sub_main/004A5950.inc"
};
char gArmorBodyNames_95[32] = {
#include "sub_main/004A5970.inc"
};
char gArmorBodyNames_96[32] = {
#include "sub_main/004A5990.inc"
};
char gArmorBodyNames_97[32] = {
#include "sub_main/004A59B0.inc"
};
char gArmorBodyNames_98[32] = {
#include "sub_main/004A59D0.inc"
};
char gArmorBodyNames_99[32] = {
#include "sub_main/004A59F0.inc"
};
char gArmorBodyNames_100[24] = {
#include "sub_main/004A5A10.inc"
};
char gArmorBodyNames_101[15] = {
#include "sub_main/004A5A28.inc"
};
char gArmorBodyNames_102[24] = {
#include "sub_main/004A5A40.inc"
};
char gArmorBodyNames_103[15] = {
#include "sub_main/004A5A58.inc"
};
char gArmorBodyNames_104[32] = {
#include "sub_main/004A5A70.inc"
};
char gArmorBodyNames_105[32] = {
#include "sub_main/004A5A90.inc"
};
char gArmorBodyNames_106[32] = {
#include "sub_main/004A5AB0.inc"
};
char gArmorBodyNames_107[32] = {
#include "sub_main/004A5AD0.inc"
};
char gArmorBodyNames_108[32] = {
#include "sub_main/004A5AF0.inc"
};
char gArmorBodyNames_109[32] = {
#include "sub_main/004A5B10.inc"
};
char gArmorBodyNames_110[24] = {
#include "sub_main/004A5B30.inc"
};
char gArmorBodyNames_111[13] = {
#include "sub_main/004A5B48.inc"
};
char gArmorBodyNames_112[13] = {
#include "sub_main/004A5B58.inc"
};
char gArmorBodyNames_113[32] = {
#include "sub_main/004A5B70.inc"
};
char gArmorBodyNames_114[32] = {
#include "sub_main/004A5B90.inc"
};
char gArmorBodyNames_115[32] = {
#include "sub_main/004A5BB0.inc"
};
char gArmorBodyNames_116[32] = {
#include "sub_main/004A5BD0.inc"
};
char gArmorBodyNames_117[32] = {
#include "sub_main/004A5BF0.inc"
};
char gArmorBodyNames_118[32] = {
#include "sub_main/004A5C10.inc"
};
char gArmorBodyNames_119[32] = {
#include "sub_main/004A5C30.inc"
};
char gArmorBodyNames_120[32] = {
#include "sub_main/004A5C50.inc"
};
char gArmorBodyNames_121[32] = {
#include "sub_main/004A5C70.inc"
};
char gArmorBodyNames_122[32] = {
#include "sub_main/004A5C90.inc"
};
char gArmorBodyNames_123[32] = {
#include "sub_main/004A5CB0.inc"
};
char gArmorBodyNames_124[32] = {
#include "sub_main/004A5CD0.inc"
};
char gArmorBodyNames_125[32] = {
#include "sub_main/004A5CF0.inc"
};
char gArmorBodyNames_126[32] = {
#include "sub_main/004A5D10.inc"
};
char gArmorBodyNames_127[32] = {
#include "sub_main/004A5D30.inc"
};
char gArmorBodyNames_128[32] = {
#include "sub_main/004A5D50.inc"
};
char gArmorBodyNames_129[32] = {
#include "sub_main/004A5D70.inc"
};
char gArmorBodyNames_130[24] = {
#include "sub_main/004A5D90.inc"
};
char gArmorBodyNames_131[15] = {
#include "sub_main/004A5DA8.inc"
};
char gArmorBodyNames_132[32] = {
#include "sub_main/004A5DC0.inc"
};
char gArmorBodyNames_133[32] = {
#include "sub_main/004A5DE0.inc"
};
char gArmorBodyNames_134[24] = {
#include "sub_main/004A5E00.inc"
};
char gArmorBodyNames_135[15] = {
#include "sub_main/004A5E18.inc"
};
char gArmorBodyNames_136[24] = {
#include "sub_main/004A5E30.inc"
};
char gArmorBodyNames_137[15] = {
#include "sub_main/004A5E48.inc"
};
char gArmorBodyNames_138[24] = {
#include "sub_main/004A5E60.inc"
};
char gArmorBodyNames_139[15] = {
#include "sub_main/004A5E78.inc"
};
char gArmorBodyNames_140[32] = {
#include "sub_main/004A5E90.inc"
};
char gArmorBodyNames_141[32] = {
#include "sub_main/004A5EB0.inc"
};
char gArmorBodyNames_142[32] = {
#include "sub_main/004A5ED0.inc"
};
char gArmorBodyNames_143[32] = {
#include "sub_main/004A5EF0.inc"
};
char gArmorBodyNames_144[24] = {
#include "sub_main/004A5F10.inc"
};
char gArmorBodyNames_145[15] = {
#include "sub_main/004A5F28.inc"
};
char gArmorBodyNames_146[32] = {
#include "sub_main/004A5F40.inc"
};
char gArmorBodyNames_147[32] = {
#include "sub_main/004A5F60.inc"
};
char gArmorBodyNames_148[32] = {
#include "sub_main/004A5F80.inc"
};
char gArmorBodyNames_149[32] = {
#include "sub_main/004A5FA0.inc"
};
char gArmorBodyNames_150[32] = {
#include "sub_main/004A5FC0.inc"
};
char gArmorBodyNames_151[32] = {
#include "sub_main/004A5FE0.inc"
};
char gArmorBodyNames_152[32] = {
#include "sub_main/004A6000.inc"
};
char gArmorBodyNames_153[24] = {
#include "sub_main/004A6020.inc"
};
char gArmorBodyNames_154[15] = {
#include "sub_main/004A6038.inc"
};
char gArmorBodyNames_155[32] = {
#include "sub_main/004A6050.inc"
};
char gArmorBodyNames_156[32] = {
#include "sub_main/004A6070.inc"
};
char gArmorBodyNames_157[24] = {
#include "sub_main/004A6090.inc"
};
char gArmorBodyNames_158[15] = {
#include "sub_main/004A60A8.inc"
};
char gArmorBodyNames_159[24] = {
#include "sub_main/004A60C0.inc"
};
char gArmorBodyNames_160[15] = {
#include "sub_main/004A60D8.inc"
};
char gArmorBodyNames_161[24] = {
#include "sub_main/004A60F0.inc"
};
char gArmorBodyNames_162[15] = {
#include "sub_main/004A6108.inc"
};
char gArmorBodyNames_163[32] = {
#include "sub_main/004A6120.inc"
};
char gArmorBodyNames_164[32] = {
#include "sub_main/004A6140.inc"
};
char gArmorBodyNames_165[32] = {
#include "sub_main/004A6160.inc"
};
char gArmorBodyNames_166[32] = {
#include "sub_main/004A6180.inc"
};
char gArmorBodyNames_167[32] = {
#include "sub_main/004A61A0.inc"
};
char gArmorBodyNames_168[32] = {
#include "sub_main/004A61C0.inc"
};
char gArmorBodyNames_169[24] = {
#include "sub_main/004A61E0.inc"
};
char gArmorBodyNames_170[13] = {
#include "sub_main/004A61F8.inc"
};
char gArmorBodyNames_171[13] = {
#include "sub_main/004A6208.inc"
};
char D_004A6220[32] = {
#include "sub_main/004A6220.inc"
};
char D_004A6240[32] = {
#include "sub_main/004A6240.inc"
};
char gArmorBodyNames_176[32] = {
#include "sub_main/004A6260.inc"
};
char gArmorBodyNames_177[32] = {
#include "sub_main/004A6280.inc"
};
char gArmorBodyNames_178[32] = {
#include "sub_main/004A62A0.inc"
};
char gArmorBodyNames_179[32] = {
#include "sub_main/004A62C0.inc"
};
char gArmorBodyNames_180[32] = {
#include "sub_main/004A62E0.inc"
};
char gArmorBodyNames_181[24] = {
#include "sub_main/004A6300.inc"
};
char gArmorBodyNames_182[13] = {
#include "sub_main/004A6318.inc"
};
char gArmorBodyNames_183[15] = {
#include "sub_main/004A6328.inc"
};
char gArmorBodyNames_184[15] = {
#include "sub_main/004A6338.inc"
};
char gArmorBodyNames_185[24] = {
#include "sub_main/004A6350.inc"
};
char gArmorBodyNames_186[15] = {
#include "sub_main/004A6368.inc"
};
char gArmorBodyNames_187[24] = {
#include "sub_main/004A6380.inc"
};
char gArmorBodyNames_188[13] = {
#include "sub_main/004A6398.inc"
};
char gArmorBodyNames_189[15] = {
#include "sub_main/004A63A8.inc"
};
char gArmorBodyNames_190[15] = {
#include "sub_main/004A63B8.inc"
};
char gArmorBodyNames_191[24] = {
#include "sub_main/004A63D0.inc"
};
char gArmorBodyNames_192[15] = {
#include "sub_main/004A63E8.inc"
};
char gArmorBodyNames_193[24] = {
#include "sub_main/004A6400.inc"
};
char gArmorBodyNames_194[15] = {
#include "sub_main/004A6418.inc"
};
char gArmorBodyNames_195[32] = {
#include "sub_main/004A6430.inc"
};
char gArmorBodyNames_196[32] = {
#include "sub_main/004A6450.inc"
};
char gArmorBodyNames_197[32] = {
#include "sub_main/004A6470.inc"
};
char gArmorBodyNames_198[32] = {
#include "sub_main/004A6490.inc"
};
char gArmorBodyNames_199[24] = {
#include "sub_main/004A64B0.inc"
};
char gArmorBodyNames_200[13] = {
#include "sub_main/004A64C8.inc"
};
char gArmorBodyNames_201[15] = {
#include "sub_main/004A64D8.inc"
};
char gArmorBodyNames_202[15] = {
#include "sub_main/004A64E8.inc"
};
char gArmorBodyNames_203[24] = {
#include "sub_main/004A6500.inc"
};
char gArmorBodyNames_204[15] = {
#include "sub_main/004A6518.inc"
};
char gArmorBodyNames_205[24] = {
#include "sub_main/004A6530.inc"
};
char gArmorBodyNames_206[13] = {
#include "sub_main/004A6548.inc"
};
char gArmorBodyNames_207[13] = {
#include "sub_main/004A6558.inc"
};
char gArmorBodyNames_208[15] = {
#include "sub_main/004A6568.inc"
};
char gArmorBodyNames_209[15] = {
#include "sub_main/004A6578.inc"
};
char gArmorBodyNames_210[24] = {
#include "sub_main/004A6590.inc"
};
char gArmorBodyNames_211[15] = {
#include "sub_main/004A65A8.inc"
};
char gArmorBodyNames_212[32] = {
#include "sub_main/004A65C0.inc"
};
char gArmorBodyNames_213[32] = {
#include "sub_main/004A65E0.inc"
};
char gArmorBodyNames_214[32] = {
#include "sub_main/004A6600.inc"
};
char gArmorBodyNames_215[24] = {
#include "sub_main/004A6620.inc"
};
char gArmorBodyNames_216[15] = {
#include "sub_main/004A6638.inc"
};
char gArmorBodyNames_217[15] = {
#include "sub_main/004A6648.inc"
};
char gArmorBodyNames_218[15] = {
#include "sub_main/004A6658.inc"
};
char gArmorBodyNames_219[24] = {
#include "sub_main/004A6670.inc"
};
char gArmorBodyNames_220[13] = {
#include "sub_main/004A6688.inc"
};
char gArmorBodyNames_221[13] = {
#include "sub_main/004A6698.inc"
};
char gArmorBodyNames_222[12] = {
#include "sub_main/004A66A8.inc"
};
char gArmorBodyNames_223[12] = {
#include "sub_main/004A66B8.inc"
};
char gArmorBodyNames_224[32] = {
#include "sub_main/004A66D0.inc"
};
char gArmorBodyNames_225[32] = {
#include "sub_main/004A66F0.inc"
};
char gArmorBodyNames_226[32] = {
#include "sub_main/004A6710.inc"
};
char gArmorBodyNames_227[32] = {
#include "sub_main/004A6730.inc"
};
char gArmorBodyNames_228[32] = {
#include "sub_main/004A6750.inc"
};
char gArmorBodyNames_229[32] = {
#include "sub_main/004A6770.inc"
};
char gArmorBodyNames_230[32] = {
#include "sub_main/004A6790.inc"
};
char gArmorBodyNames_231[24] = {
#include "sub_main/004A67B0.inc"
};
char gArmorBodyNames_232[13] = {
#include "sub_main/004A67C8.inc"
};
char gArmorBodyNames_233[24] = {
#include "sub_main/004A67E0.inc"
};
char gArmorBodyNames_234[13] = {
#include "sub_main/004A67F8.inc"
};
char gArmorBodyNames_235[32] = {
#include "sub_main/004A6810.inc"
};
char D_004A6830[32] = {
#include "sub_main/004A6830.inc"
};
char D_004A6850[24] = {
#include "sub_main/004A6850.inc"
};
char gArmorBodyNames_240[15] = {
#include "sub_main/004A6868.inc"
};
char gArmorBodyNames_241[15] = {
#include "sub_main/004A6878.inc"
};
char gArmorBodyNames_242[32] = {
#include "sub_main/004A6890.inc"
};
char gArmorArmsNames_1[24] = {
#include "sub_main/004A68B0.inc"
};
char gArmorArmsNames_2[15] = {
#include "sub_main/004A68C8.inc"
};
char gArmorArmsNames_3[15] = {
#include "sub_main/004A68D8.inc"
};
char gArmorArmsNames_4[15] = {
#include "sub_main/004A68E8.inc"
};
char gArmorArmsNames_5[13] = {
#include "sub_main/004A68F8.inc"
};
char gArmorArmsNames_6[13] = {
#include "sub_main/004A6908.inc"
};
char gArmorArmsNames_7[15] = {
#include "sub_main/004A6918.inc"
};
char gArmorArmsNames_8[15] = {
#include "sub_main/004A6928.inc"
};
char gArmorArmsNames_9[13] = {
#include "sub_main/004A6938.inc"
};
char gArmorArmsNames_10[13] = {
#include "sub_main/004A6948.inc"
};
char gArmorArmsNames_11[13] = {
#include "sub_main/004A6958.inc"
};
char gArmorArmsNames_12[13] = {
#include "sub_main/004A6968.inc"
};
char gArmorArmsNames_13[13] = {
#include "sub_main/004A6978.inc"
};
char gArmorArmsNames_14[13] = {
#include "sub_main/004A6988.inc"
};
char gArmorArmsNames_15[15] = {
#include "sub_main/004A6998.inc"
};
char gArmorArmsNames_16[15] = {
#include "sub_main/004A69A8.inc"
};
char gArmorArmsNames_17[15] = {
#include "sub_main/004A69B8.inc"
};
char gArmorArmsNames_18[15] = {
#include "sub_main/004A69C8.inc"
};
char gArmorArmsNames_19[13] = {
#include "sub_main/004A69D8.inc"
};
char gArmorArmsNames_20[13] = {
#include "sub_main/004A69E8.inc"
};
char gArmorArmsNames_21[15] = {
#include "sub_main/004A69F8.inc"
};
char gArmorArmsNames_22[15] = {
#include "sub_main/004A6A08.inc"
};
char gArmorArmsNames_23[15] = {
#include "sub_main/004A6A18.inc"
};
char gArmorArmsNames_24[15] = {
#include "sub_main/004A6A28.inc"
};
char gArmorArmsNames_25[15] = {
#include "sub_main/004A6A38.inc"
};
char gArmorArmsNames_26[15] = {
#include "sub_main/004A6A48.inc"
};
char gArmorArmsNames_27[24] = {
#include "sub_main/004A6A60.inc"
};
char gArmorArmsNames_28[13] = {
#include "sub_main/004A6A78.inc"
};
char gArmorArmsNames_29[13] = {
#include "sub_main/004A6A88.inc"
};
char gArmorArmsNames_30[15] = {
#include "sub_main/004A6A98.inc"
};
char gArmorArmsNames_31[15] = {
#include "sub_main/004A6AA8.inc"
};
char gArmorArmsNames_32[13] = {
#include "sub_main/004A6AB8.inc"
};
char gArmorArmsNames_33[13] = {
#include "sub_main/004A6AC8.inc"
};
char gArmorArmsNames_34[13] = {
#include "sub_main/004A6AD8.inc"
};
char gArmorArmsNames_35[13] = {
#include "sub_main/004A6AE8.inc"
};
char gArmorArmsNames_36[32] = {
#include "sub_main/004A6B00.inc"
};
char gArmorArmsNames_37[24] = {
#include "sub_main/004A6B20.inc"
};
char gArmorArmsNames_38[13] = {
#include "sub_main/004A6B38.inc"
};
char gArmorArmsNames_39[13] = {
#include "sub_main/004A6B48.inc"
};
char gArmorArmsNames_40[32] = {
#include "sub_main/004A6B60.inc"
};
char gArmorArmsNames_41[32] = {
#include "sub_main/004A6B80.inc"
};
char gArmorArmsNames_42[32] = {
#include "sub_main/004A6BA0.inc"
};
char gArmorArmsNames_43[24] = {
#include "sub_main/004A6BC0.inc"
};
char gArmorArmsNames_44[15] = {
#include "sub_main/004A6BD8.inc"
};
char gArmorArmsNames_45[15] = {
#include "sub_main/004A6BE8.inc"
};
char gArmorArmsNames_46[15] = {
#include "sub_main/004A6BF8.inc"
};
char gArmorArmsNames_47[12] = {
#include "sub_main/004A6C08.inc"
};
char gArmorArmsNames_48[32] = {
#include "sub_main/004A6C20.inc"
};
char gArmorArmsNames_49[32] = {
#include "sub_main/004A6C40.inc"
};
char gArmorArmsNames_50[32] = {
#include "sub_main/004A6C60.inc"
};
char gArmorArmsNames_51[24] = {
#include "sub_main/004A6C80.inc"
};
char gArmorArmsNames_52[13] = {
#include "sub_main/004A6C98.inc"
};
char gArmorArmsNames_53[13] = {
#include "sub_main/004A6CA8.inc"
};
char gArmorArmsNames_54[32] = {
#include "sub_main/004A6CC0.inc"
};
char gArmorArmsNames_55[24] = {
#include "sub_main/004A6CE0.inc"
};
char gArmorArmsNames_56[15] = {
#include "sub_main/004A6CF8.inc"
};
char gArmorArmsNames_57[15] = {
#include "sub_main/004A6D08.inc"
};
char gArmorArmsNames_58[15] = {
#include "sub_main/004A6D18.inc"
};
char gArmorArmsNames_59[15] = {
#include "sub_main/004A6D28.inc"
};
char gArmorArmsNames_60[13] = {
#include "sub_main/004A6D38.inc"
};
char gArmorArmsNames_61[13] = {
#include "sub_main/004A6D48.inc"
};
char gArmorArmsNames_62[12] = {
#include "sub_main/004A6D58.inc"
};
char gArmorArmsNames_63[12] = {
#include "sub_main/004A6D68.inc"
};
char gArmorArmsNames_64[15] = {
#include "sub_main/004A6D78.inc"
};
char gArmorArmsNames_65[32] = {
#include "sub_main/004A6D90.inc"
};
char gArmorArmsNames_66[32] = {
#include "sub_main/004A6DB0.inc"
};
char gArmorArmsNames_67[32] = {
#include "sub_main/004A6DD0.inc"
};
char gArmorArmsNames_68[32] = {
#include "sub_main/004A6DF0.inc"
};
char gArmorArmsNames_69[24] = {
#include "sub_main/004A6E10.inc"
};
char gArmorArmsNames_70[15] = {
#include "sub_main/004A6E28.inc"
};
char gArmorArmsNames_71[15] = {
#include "sub_main/004A6E38.inc"
};
char gArmorArmsNames_72[32] = {
#include "sub_main/004A6E50.inc"
};
char gArmorArmsNames_73[24] = {
#include "sub_main/004A6E70.inc"
};
char gArmorArmsNames_74[15] = {
#include "sub_main/004A6E88.inc"
};
char gArmorArmsNames_75[15] = {
#include "sub_main/004A6E98.inc"
};
char gArmorArmsNames_76[15] = {
#include "sub_main/004A6EA8.inc"
};
char gArmorArmsNames_77[15] = {
#include "sub_main/004A6EB8.inc"
};
char gArmorArmsNames_78[15] = {
#include "sub_main/004A6EC8.inc"
};
char gArmorArmsNames_79[15] = {
#include "sub_main/004A6ED8.inc"
};
char gArmorArmsNames_80[32] = {
#include "sub_main/004A6EF0.inc"
};
char gArmorArmsNames_81[32] = {
#include "sub_main/004A6F10.inc"
};
char gArmorArmsNames_82[32] = {
#include "sub_main/004A6F30.inc"
};
char gArmorArmsNames_83[24] = {
#include "sub_main/004A6F50.inc"
};
char gArmorArmsNames_84[15] = {
#include "sub_main/004A6F68.inc"
};
char gArmorArmsNames_85[15] = {
#include "sub_main/004A6F78.inc"
};
char gArmorArmsNames_86[32] = {
#include "sub_main/004A6F90.inc"
};
char gArmorArmsNames_87[32] = {
#include "sub_main/004A6FB0.inc"
};
char gArmorArmsNames_88[32] = {
#include "sub_main/004A6FD0.inc"
};
char gArmorArmsNames_89[32] = {
#include "sub_main/004A6FF0.inc"
};
char gArmorArmsNames_90[32] = {
#include "sub_main/004A7010.inc"
};
char gArmorArmsNames_91[32] = {
#include "sub_main/004A7030.inc"
};
char gArmorArmsNames_92[32] = {
#include "sub_main/004A7050.inc"
};
char gArmorArmsNames_93[32] = {
#include "sub_main/004A7070.inc"
};
char gArmorArmsNames_94[32] = {
#include "sub_main/004A7090.inc"
};
char gArmorArmsNames_95[32] = {
#include "sub_main/004A70B0.inc"
};
char gArmorArmsNames_96[32] = {
#include "sub_main/004A70D0.inc"
};
char gArmorArmsNames_97[32] = {
#include "sub_main/004A70F0.inc"
};
char gArmorArmsNames_98[24] = {
#include "sub_main/004A7110.inc"
};
char gArmorArmsNames_99[15] = {
#include "sub_main/004A7128.inc"
};
char gArmorArmsNames_100[15] = {
#include "sub_main/004A7138.inc"
};
char gArmorArmsNames_101[15] = {
#include "sub_main/004A7148.inc"
};
char gArmorArmsNames_102[15] = {
#include "sub_main/004A7158.inc"
};
char gArmorArmsNames_103[32] = {
#include "sub_main/004A7170.inc"
};
char gArmorArmsNames_104[32] = {
#include "sub_main/004A7190.inc"
};
char gArmorArmsNames_105[32] = {
#include "sub_main/004A71B0.inc"
};
char gArmorArmsNames_106[32] = {
#include "sub_main/004A71D0.inc"
};
char gArmorArmsNames_107[32] = {
#include "sub_main/004A71F0.inc"
};
char gArmorArmsNames_108[24] = {
#include "sub_main/004A7210.inc"
};
char gArmorArmsNames_109[13] = {
#include "sub_main/004A7228.inc"
};
char gArmorArmsNames_110[13] = {
#include "sub_main/004A7238.inc"
};
char gArmorArmsNames_111[32] = {
#include "sub_main/004A7250.inc"
};
char gArmorArmsNames_112[32] = {
#include "sub_main/004A7270.inc"
};
char gArmorArmsNames_113[32] = {
#include "sub_main/004A7290.inc"
};
char gArmorArmsNames_114[32] = {
#include "sub_main/004A72B0.inc"
};
char gArmorArmsNames_115[32] = {
#include "sub_main/004A72D0.inc"
};
char gArmorArmsNames_116[24] = {
#include "sub_main/004A72F0.inc"
};
char gArmorArmsNames_117[15] = {
#include "sub_main/004A7308.inc"
};
char gArmorArmsNames_118[15] = {
#include "sub_main/004A7318.inc"
};
char gArmorArmsNames_119[32] = {
#include "sub_main/004A7330.inc"
};
char gArmorArmsNames_120[32] = {
#include "sub_main/004A7350.inc"
};
char gArmorArmsNames_121[32] = {
#include "sub_main/004A7370.inc"
};
char gArmorArmsNames_122[32] = {
#include "sub_main/004A7390.inc"
};
char gArmorArmsNames_123[32] = {
#include "sub_main/004A73B0.inc"
};
char gArmorArmsNames_124[32] = {
#include "sub_main/004A73D0.inc"
};
char gArmorArmsNames_125[32] = {
#include "sub_main/004A73F0.inc"
};
char gArmorArmsNames_126[32] = {
#include "sub_main/004A7410.inc"
};
char gArmorArmsNames_127[32] = {
#include "sub_main/004A7430.inc"
};
char gArmorArmsNames_128[24] = {
#include "sub_main/004A7450.inc"
};
char gArmorArmsNames_129[15] = {
#include "sub_main/004A7468.inc"
};
char gArmorArmsNames_130[15] = {
#include "sub_main/004A7478.inc"
};
char gArmorArmsNames_131[32] = {
#include "sub_main/004A7490.inc"
};
char gArmorArmsNames_132[24] = {
#include "sub_main/004A74B0.inc"
};
char gArmorArmsNames_133[15] = {
#include "sub_main/004A74C8.inc"
};
char gArmorArmsNames_134[15] = {
#include "sub_main/004A74D8.inc"
};
char gArmorArmsNames_135[15] = {
#include "sub_main/004A74E8.inc"
};
char gArmorArmsNames_136[15] = {
#include "sub_main/004A74F8.inc"
};
char gArmorArmsNames_137[15] = {
#include "sub_main/004A7508.inc"
};
char gArmorArmsNames_138[15] = {
#include "sub_main/004A7518.inc"
};
char gArmorArmsNames_139[32] = {
#include "sub_main/004A7530.inc"
};
char gArmorArmsNames_140[32] = {
#include "sub_main/004A7550.inc"
};
char gArmorArmsNames_141[32] = {
#include "sub_main/004A7570.inc"
};
char gArmorArmsNames_142[24] = {
#include "sub_main/004A7590.inc"
};
char gArmorArmsNames_143[15] = {
#include "sub_main/004A75A8.inc"
};
char gArmorArmsNames_144[15] = {
#include "sub_main/004A75B8.inc"
};
char gArmorArmsNames_145[32] = {
#include "sub_main/004A75D0.inc"
};
char gArmorArmsNames_146[32] = {
#include "sub_main/004A75F0.inc"
};
char gArmorArmsNames_147[32] = {
#include "sub_main/004A7610.inc"
};
char gArmorArmsNames_148[32] = {
#include "sub_main/004A7630.inc"
};
char gArmorArmsNames_149[32] = {
#include "sub_main/004A7650.inc"
};
char gArmorArmsNames_150[32] = {
#include "sub_main/004A7670.inc"
};
char gArmorArmsNames_151[24] = {
#include "sub_main/004A7690.inc"
};
char gArmorArmsNames_152[15] = {
#include "sub_main/004A76A8.inc"
};
char gArmorArmsNames_153[15] = {
#include "sub_main/004A76B8.inc"
};
char gArmorArmsNames_154[32] = {
#include "sub_main/004A76D0.inc"
};
char gArmorArmsNames_155[24] = {
#include "sub_main/004A76F0.inc"
};
char gArmorArmsNames_156[15] = {
#include "sub_main/004A7708.inc"
};
char gArmorArmsNames_157[15] = {
#include "sub_main/004A7718.inc"
};
char gArmorArmsNames_158[15] = {
#include "sub_main/004A7728.inc"
};
char gArmorArmsNames_159[15] = {
#include "sub_main/004A7738.inc"
};
char gArmorArmsNames_160[15] = {
#include "sub_main/004A7748.inc"
};
char gArmorArmsNames_161[15] = {
#include "sub_main/004A7758.inc"
};
char gArmorArmsNames_162[32] = {
#include "sub_main/004A7770.inc"
};
char gArmorArmsNames_163[32] = {
#include "sub_main/004A7790.inc"
};
char gArmorArmsNames_164[32] = {
#include "sub_main/004A77B0.inc"
};
char gArmorArmsNames_165[32] = {
#include "sub_main/004A77D0.inc"
};
char gArmorArmsNames_166[32] = {
#include "sub_main/004A77F0.inc"
};
char gArmorArmsNames_167[24] = {
#include "sub_main/004A7810.inc"
};
char gArmorArmsNames_168[13] = {
#include "sub_main/004A7828.inc"
};
char gArmorArmsNames_169[13] = {
#include "sub_main/004A7838.inc"
};
char D_004A7850[32] = {
#include "sub_main/004A7850.inc"
};
char D_004A7870[32] = {
#include "sub_main/004A7870.inc"
};
char gArmorArmsNames_174[32] = {
#include "sub_main/004A7890.inc"
};
char gArmorArmsNames_175[24] = {
#include "sub_main/004A78B0.inc"
};
char gArmorArmsNames_176[15] = {
#include "sub_main/004A78C8.inc"
};
char gArmorArmsNames_177[15] = {
#include "sub_main/004A78D8.inc"
};
char gArmorArmsNames_178[32] = {
#include "sub_main/004A78F0.inc"
};
char gArmorArmsNames_179[24] = {
#include "sub_main/004A7910.inc"
};
char gArmorArmsNames_180[13] = {
#include "sub_main/004A7928.inc"
};
char gArmorArmsNames_181[13] = {
#include "sub_main/004A7938.inc"
};
char gArmorArmsNames_182[15] = {
#include "sub_main/004A7948.inc"
};
char gArmorArmsNames_183[15] = {
#include "sub_main/004A7958.inc"
};
char gArmorArmsNames_184[15] = {
#include "sub_main/004A7968.inc"
};
char gArmorArmsNames_185[15] = {
#include "sub_main/004A7978.inc"
};
char gArmorArmsNames_186[13] = {
#include "sub_main/004A7988.inc"
};
char gArmorArmsNames_187[13] = {
#include "sub_main/004A7998.inc"
};
char gArmorArmsNames_188[15] = {
#include "sub_main/004A79A8.inc"
};
char gArmorArmsNames_189[15] = {
#include "sub_main/004A79B8.inc"
};
char gArmorArmsNames_190[15] = {
#include "sub_main/004A79C8.inc"
};
char gArmorArmsNames_191[15] = {
#include "sub_main/004A79D8.inc"
};
char gArmorArmsNames_192[15] = {
#include "sub_main/004A79E8.inc"
};
char gArmorArmsNames_193[15] = {
#include "sub_main/004A79F8.inc"
};
char gArmorArmsNames_194[32] = {
#include "sub_main/004A7A10.inc"
};
char gArmorArmsNames_195[32] = {
#include "sub_main/004A7A30.inc"
};
char gArmorArmsNames_196[32] = {
#include "sub_main/004A7A50.inc"
};
char gArmorArmsNames_197[24] = {
#include "sub_main/004A7A70.inc"
};
char gArmorArmsNames_198[13] = {
#include "sub_main/004A7A88.inc"
};
char gArmorArmsNames_199[13] = {
#include "sub_main/004A7A98.inc"
};
char gArmorArmsNames_200[15] = {
#include "sub_main/004A7AA8.inc"
};
char gArmorArmsNames_201[15] = {
#include "sub_main/004A7AB8.inc"
};
char gArmorArmsNames_202[15] = {
#include "sub_main/004A7AC8.inc"
};
char gArmorArmsNames_203[15] = {
#include "sub_main/004A7AD8.inc"
};
char gArmorArmsNames_204[24] = {
#include "sub_main/004A7AF0.inc"
};
char gArmorArmsNames_205[15] = {
#include "sub_main/004A7B08.inc"
};
char gArmorArmsNames_206[32] = {
#include "sub_main/004A7B20.inc"
};
char gArmorArmsNames_207[32] = {
#include "sub_main/004A7B40.inc"
};
char gArmorArmsNames_208[32] = {
#include "sub_main/004A7B60.inc"
};
char gArmorArmsNames_209[24] = {
#include "sub_main/004A7B80.inc"
};
char gArmorArmsNames_210[15] = {
#include "sub_main/004A7B98.inc"
};
char gArmorArmsNames_211[32] = {
#include "sub_main/004A7BB0.inc"
};
char gArmorArmsNames_212[32] = {
#include "sub_main/004A7BD0.inc"
};
char gArmorArmsNames_213[24] = {
#include "sub_main/004A7BF0.inc"
};
char gArmorArmsNames_214[13] = {
#include "sub_main/004A7C08.inc"
};
char gArmorArmsNames_215[13] = {
#include "sub_main/004A7C18.inc"
};
char gArmorArmsNames_216[15] = {
#include "sub_main/004A7C28.inc"
};
char gArmorArmsNames_217[15] = {
#include "sub_main/004A7C38.inc"
};
char gArmorArmsNames_218[13] = {
#include "sub_main/004A7C48.inc"
};
char gArmorArmsNames_219[13] = {
#include "sub_main/004A7C58.inc"
};
char gArmorArmsNames_220[12] = {
#include "sub_main/004A7C68.inc"
};
char gArmorArmsNames_221[12] = {
#include "sub_main/004A7C78.inc"
};
char gArmorArmsNames_222[32] = {
#include "sub_main/004A7C90.inc"
};
char gArmorArmsNames_223[32] = {
#include "sub_main/004A7CB0.inc"
};
char gArmorArmsNames_224[32] = {
#include "sub_main/004A7CD0.inc"
};
char gArmorArmsNames_225[32] = {
#include "sub_main/004A7CF0.inc"
};
char gArmorArmsNames_226[32] = {
#include "sub_main/004A7D10.inc"
};
char gArmorArmsNames_227[32] = {
#include "sub_main/004A7D30.inc"
};
char gArmorArmsNames_228[24] = {
#include "sub_main/004A7D50.inc"
};
char gArmorArmsNames_229[13] = {
#include "sub_main/004A7D68.inc"
};
char gArmorArmsNames_230[32] = {
#include "sub_main/004A7D80.inc"
};
char D_004A7DA0[32] = {
#include "sub_main/004A7DA0.inc"
};
char D_004A7DC0[24] = {
#include "sub_main/004A7DC0.inc"
};
char gArmorArmsNames_235[15] = {
#include "sub_main/004A7DD8.inc"
};
char gArmorArmsNames_236[15] = {
#include "sub_main/004A7DE8.inc"
};
char gArmorArmsNames_237[12] = {
#include "sub_main/004A7DF8.inc"
};
char gArmorWaistNames_1[32] = {
#include "sub_main/004A7E10.inc"
};
char gArmorWaistNames_2[32] = {
#include "sub_main/004A7E30.inc"
};
char gArmorWaistNames_3[24] = {
#include "sub_main/004A7E50.inc"
};
char gArmorWaistNames_4[15] = {
#include "sub_main/004A7E68.inc"
};
char gArmorWaistNames_5[24] = {
#include "sub_main/004A7E80.inc"
};
char gArmorWaistNames_6[15] = {
#include "sub_main/004A7E98.inc"
};
char gArmorWaistNames_7[24] = {
#include "sub_main/004A7EB0.inc"
};
char gArmorWaistNames_8[13] = {
#include "sub_main/004A7EC8.inc"
};
char gArmorWaistNames_9[24] = {
#include "sub_main/004A7EE0.inc"
};
char gArmorWaistNames_10[15] = {
#include "sub_main/004A7EF8.inc"
};
char gArmorWaistNames_11[24] = {
#include "sub_main/004A7F10.inc"
};
char gArmorWaistNames_12[13] = {
#include "sub_main/004A7F28.inc"
};
char gArmorWaistNames_13[13] = {
#include "sub_main/004A7F38.inc"
};
char gArmorWaistNames_14[24] = {
#include "sub_main/004A7F50.inc"
};
char gArmorWaistNames_15[13] = {
#include "sub_main/004A7F68.inc"
};
char gArmorWaistNames_16[24] = {
#include "sub_main/004A7F80.inc"
};
char gArmorWaistNames_17[13] = {
#include "sub_main/004A7F98.inc"
};
char gArmorWaistNames_18[24] = {
#include "sub_main/004A7FB0.inc"
};
char gArmorWaistNames_19[15] = {
#include "sub_main/004A7FC8.inc"
};
char gArmorWaistNames_20[24] = {
#include "sub_main/004A7FE0.inc"
};
char gArmorWaistNames_21[15] = {
#include "sub_main/004A7FF8.inc"
};
char gArmorWaistNames_22[32] = {
#include "sub_main/004A8010.inc"
};
char gArmorWaistNames_23[24] = {
#include "sub_main/004A8030.inc"
};
char gArmorWaistNames_24[13] = {
#include "sub_main/004A8048.inc"
};
char gArmorWaistNames_25[24] = {
#include "sub_main/004A8060.inc"
};
char gArmorWaistNames_26[15] = {
#include "sub_main/004A8078.inc"
};
char gArmorWaistNames_27[24] = {
#include "sub_main/004A8090.inc"
};
char gArmorWaistNames_28[15] = {
#include "sub_main/004A80A8.inc"
};
char gArmorWaistNames_29[24] = {
#include "sub_main/004A80C0.inc"
};
char gArmorWaistNames_30[15] = {
#include "sub_main/004A80D8.inc"
};
char gArmorWaistNames_31[32] = {
#include "sub_main/004A80F0.inc"
};
char gArmorWaistNames_32[24] = {
#include "sub_main/004A8110.inc"
};
char gArmorWaistNames_33[13] = {
#include "sub_main/004A8128.inc"
};
char gArmorWaistNames_34[24] = {
#include "sub_main/004A8140.inc"
};
char gArmorWaistNames_35[15] = {
#include "sub_main/004A8158.inc"
};
char gArmorWaistNames_36[24] = {
#include "sub_main/004A8170.inc"
};
char gArmorWaistNames_37[13] = {
#include "sub_main/004A8188.inc"
};
char gArmorWaistNames_38[24] = {
#include "sub_main/004A81A0.inc"
};
char gArmorWaistNames_39[13] = {
#include "sub_main/004A81B8.inc"
};
char gArmorWaistNames_40[32] = {
#include "sub_main/004A81D0.inc"
};
char gArmorWaistNames_41[32] = {
#include "sub_main/004A81F0.inc"
};
char gArmorWaistNames_42[32] = {
#include "sub_main/004A8210.inc"
};
char gArmorWaistNames_43[32] = {
#include "sub_main/004A8230.inc"
};
char gArmorWaistNames_44[24] = {
#include "sub_main/004A8250.inc"
};
char gArmorWaistNames_45[15] = {
#include "sub_main/004A8268.inc"
};
char gArmorWaistNames_46[24] = {
#include "sub_main/004A8280.inc"
};
char gArmorWaistNames_47[12] = {
#include "sub_main/004A8298.inc"
};
char gArmorWaistNames_48[15] = {
#include "sub_main/004A82A8.inc"
};
char gArmorWaistNames_49[24] = {
#include "sub_main/004A82C0.inc"
};
char gArmorWaistNames_50[12] = {
#include "sub_main/004A82D8.inc"
};
char gArmorWaistNames_51[12] = {
#include "sub_main/004A82E8.inc"
};
char gArmorWaistNames_52[32] = {
#include "sub_main/004A8300.inc"
};
char gArmorWaistNames_53[32] = {
#include "sub_main/004A8320.inc"
};
char gArmorWaistNames_54[24] = {
#include "sub_main/004A8340.inc"
};
char gArmorWaistNames_55[15] = {
#include "sub_main/004A8358.inc"
};
char gArmorWaistNames_56[24] = {
#include "sub_main/004A8370.inc"
};
char gArmorWaistNames_57[15] = {
#include "sub_main/004A8388.inc"
};
char gArmorWaistNames_58[15] = {
#include "sub_main/004A8398.inc"
};
char gArmorWaistNames_59[15] = {
#include "sub_main/004A83A8.inc"
};
char gArmorWaistNames_60[13] = {
#include "sub_main/004A83B8.inc"
};
char gArmorWaistNames_61[13] = {
#include "sub_main/004A83C8.inc"
};
char gArmorWaistNames_62[15] = {
#include "sub_main/004A83D8.inc"
};
char gArmorWaistNames_63[15] = {
#include "sub_main/004A83E8.inc"
};
char gArmorWaistNames_64[24] = {
#include "sub_main/004A8400.inc"
};
char gArmorWaistNames_65[15] = {
#include "sub_main/004A8418.inc"
};
char gArmorWaistNames_66[32] = {
#include "sub_main/004A8430.inc"
};
char gArmorWaistNames_67[32] = {
#include "sub_main/004A8450.inc"
};
char gArmorWaistNames_68[32] = {
#include "sub_main/004A8470.inc"
};
char gArmorWaistNames_69[32] = {
#include "sub_main/004A8490.inc"
};
char gArmorWaistNames_70[24] = {
#include "sub_main/004A84B0.inc"
};
char gArmorWaistNames_71[15] = {
#include "sub_main/004A84C8.inc"
};
char gArmorWaistNames_72[32] = {
#include "sub_main/004A84E0.inc"
};
char gArmorWaistNames_73[32] = {
#include "sub_main/004A8500.inc"
};
char gArmorWaistNames_74[24] = {
#include "sub_main/004A8520.inc"
};
char gArmorWaistNames_75[15] = {
#include "sub_main/004A8538.inc"
};
char gArmorWaistNames_76[24] = {
#include "sub_main/004A8550.inc"
};
char gArmorWaistNames_77[15] = {
#include "sub_main/004A8568.inc"
};
char gArmorWaistNames_78[24] = {
#include "sub_main/004A8580.inc"
};
char gArmorWaistNames_79[15] = {
#include "sub_main/004A8598.inc"
};
char gArmorWaistNames_80[32] = {
#include "sub_main/004A85B0.inc"
};
char gArmorWaistNames_81[32] = {
#include "sub_main/004A85D0.inc"
};
char gArmorWaistNames_82[32] = {
#include "sub_main/004A85F0.inc"
};
char gArmorWaistNames_83[32] = {
#include "sub_main/004A8610.inc"
};
char gArmorWaistNames_84[24] = {
#include "sub_main/004A8630.inc"
};
char gArmorWaistNames_85[15] = {
#include "sub_main/004A8648.inc"
};
char gArmorWaistNames_86[32] = {
#include "sub_main/004A8660.inc"
};
char gArmorWaistNames_87[32] = {
#include "sub_main/004A8680.inc"
};
char gArmorWaistNames_88[32] = {
#include "sub_main/004A86A0.inc"
};
char gArmorWaistNames_89[32] = {
#include "sub_main/004A86C0.inc"
};
char gArmorWaistNames_90[32] = {
#include "sub_main/004A86E0.inc"
};
char gArmorWaistNames_91[32] = {
#include "sub_main/004A8700.inc"
};
char gArmorWaistNames_92[32] = {
#include "sub_main/004A8720.inc"
};
char gArmorWaistNames_93[32] = {
#include "sub_main/004A8740.inc"
};
char gArmorWaistNames_94[32] = {
#include "sub_main/004A8760.inc"
};
char gArmorWaistNames_95[32] = {
#include "sub_main/004A8780.inc"
};
char gArmorWaistNames_96[32] = {
#include "sub_main/004A87A0.inc"
};
char gArmorWaistNames_97[32] = {
#include "sub_main/004A87C0.inc"
};
char gArmorWaistNames_98[32] = {
#include "sub_main/004A87E0.inc"
};
char gArmorWaistNames_99[24] = {
#include "sub_main/004A8800.inc"
};
char gArmorWaistNames_100[15] = {
#include "sub_main/004A8818.inc"
};
char gArmorWaistNames_101[32] = {
#include "sub_main/004A8830.inc"
};
char gArmorWaistNames_102[32] = {
#include "sub_main/004A8850.inc"
};
char gArmorWaistNames_103[32] = {
#include "sub_main/004A8870.inc"
};
char gArmorWaistNames_104[32] = {
#include "sub_main/004A8890.inc"
};
char gArmorWaistNames_105[32] = {
#include "sub_main/004A88B0.inc"
};
char gArmorWaistNames_106[24] = {
#include "sub_main/004A88D0.inc"
};
char gArmorWaistNames_107[12] = {
#include "sub_main/004A88E8.inc"
};
char gArmorWaistNames_108[12] = {
#include "sub_main/004A88F8.inc"
};
char gArmorWaistNames_109[32] = {
#include "sub_main/004A8910.inc"
};
char gArmorWaistNames_110[32] = {
#include "sub_main/004A8930.inc"
};
char gArmorWaistNames_111[32] = {
#include "sub_main/004A8950.inc"
};
char gArmorWaistNames_112[32] = {
#include "sub_main/004A8970.inc"
};
char gArmorWaistNames_113[32] = {
#include "sub_main/004A8990.inc"
};
char gArmorWaistNames_114[32] = {
#include "sub_main/004A89B0.inc"
};
char gArmorWaistNames_115[32] = {
#include "sub_main/004A89D0.inc"
};
char gArmorWaistNames_116[32] = {
#include "sub_main/004A89F0.inc"
};
char gArmorWaistNames_117[32] = {
#include "sub_main/004A8A10.inc"
};
char gArmorWaistNames_118[32] = {
#include "sub_main/004A8A30.inc"
};
char gArmorWaistNames_119[32] = {
#include "sub_main/004A8A50.inc"
};
char gArmorWaistNames_120[32] = {
#include "sub_main/004A8A70.inc"
};
char gArmorWaistNames_121[32] = {
#include "sub_main/004A8A90.inc"
};
char gArmorWaistNames_122[32] = {
#include "sub_main/004A8AB0.inc"
};
char gArmorWaistNames_123[32] = {
#include "sub_main/004A8AD0.inc"
};
char gArmorWaistNames_124[32] = {
#include "sub_main/004A8AF0.inc"
};
char gArmorWaistNames_125[32] = {
#include "sub_main/004A8B10.inc"
};
char gArmorWaistNames_126[32] = {
#include "sub_main/004A8B30.inc"
};
char gArmorWaistNames_127[24] = {
#include "sub_main/004A8B50.inc"
};
char gArmorWaistNames_128[15] = {
#include "sub_main/004A8B68.inc"
};
char gArmorWaistNames_129[32] = {
#include "sub_main/004A8B80.inc"
};
char gArmorWaistNames_130[32] = {
#include "sub_main/004A8BA0.inc"
};
char gArmorWaistNames_131[24] = {
#include "sub_main/004A8BC0.inc"
};
char gArmorWaistNames_132[15] = {
#include "sub_main/004A8BD8.inc"
};
char gArmorWaistNames_133[24] = {
#include "sub_main/004A8BF0.inc"
};
char gArmorWaistNames_134[15] = {
#include "sub_main/004A8C08.inc"
};
char gArmorWaistNames_135[24] = {
#include "sub_main/004A8C20.inc"
};
char gArmorWaistNames_136[15] = {
#include "sub_main/004A8C38.inc"
};
char gArmorWaistNames_137[32] = {
#include "sub_main/004A8C50.inc"
};
char gArmorWaistNames_138[32] = {
#include "sub_main/004A8C70.inc"
};
char gArmorWaistNames_139[32] = {
#include "sub_main/004A8C90.inc"
};
char gArmorWaistNames_140[32] = {
#include "sub_main/004A8CB0.inc"
};
char gArmorWaistNames_141[24] = {
#include "sub_main/004A8CD0.inc"
};
char gArmorWaistNames_142[15] = {
#include "sub_main/004A8CE8.inc"
};
char gArmorWaistNames_143[32] = {
#include "sub_main/004A8D00.inc"
};
char gArmorWaistNames_144[32] = {
#include "sub_main/004A8D20.inc"
};
char gArmorWaistNames_145[32] = {
#include "sub_main/004A8D40.inc"
};
char gArmorWaistNames_146[32] = {
#include "sub_main/004A8D60.inc"
};
char gArmorWaistNames_147[32] = {
#include "sub_main/004A8D80.inc"
};
char gArmorWaistNames_148[32] = {
#include "sub_main/004A8DA0.inc"
};
char gArmorWaistNames_149[32] = {
#include "sub_main/004A8DC0.inc"
};
char gArmorWaistNames_150[24] = {
#include "sub_main/004A8DE0.inc"
};
char gArmorWaistNames_151[15] = {
#include "sub_main/004A8DF8.inc"
};
char gArmorWaistNames_152[32] = {
#include "sub_main/004A8E10.inc"
};
char gArmorWaistNames_153[32] = {
#include "sub_main/004A8E30.inc"
};
char gArmorWaistNames_154[24] = {
#include "sub_main/004A8E50.inc"
};
char gArmorWaistNames_155[15] = {
#include "sub_main/004A8E68.inc"
};
char gArmorWaistNames_156[24] = {
#include "sub_main/004A8E80.inc"
};
char gArmorWaistNames_157[15] = {
#include "sub_main/004A8E98.inc"
};
char gArmorWaistNames_158[32] = {
#include "sub_main/004A8EB0.inc"
};
char gArmorWaistNames_159[32] = {
#include "sub_main/004A8ED0.inc"
};
char gArmorWaistNames_160[32] = {
#include "sub_main/004A8EF0.inc"
};
char gArmorWaistNames_161[32] = {
#include "sub_main/004A8F10.inc"
};
char gArmorWaistNames_162[32] = {
#include "sub_main/004A8F30.inc"
};
char gArmorWaistNames_163[24] = {
#include "sub_main/004A8F50.inc"
};
char gArmorWaistNames_164[12] = {
#include "sub_main/004A8F68.inc"
};
char gArmorWaistNames_165[12] = {
#include "sub_main/004A8F78.inc"
};
char D_004A8F90[32] = {
#include "sub_main/004A8F90.inc"
};
char D_004A8FB0[32] = {
#include "sub_main/004A8FB0.inc"
};
char gArmorWaistNames_170[32] = {
#include "sub_main/004A8FD0.inc"
};
char gArmorWaistNames_171[32] = {
#include "sub_main/004A8FF0.inc"
};
char gArmorWaistNames_172[32] = {
#include "sub_main/004A9010.inc"
};
char gArmorWaistNames_173[32] = {
#include "sub_main/004A9030.inc"
};
char gArmorWaistNames_174[32] = {
#include "sub_main/004A9050.inc"
};
char gArmorWaistNames_175[32] = {
#include "sub_main/004A9070.inc"
};
char gArmorWaistNames_176[24] = {
#include "sub_main/004A9090.inc"
};
char gArmorWaistNames_177[13] = {
#include "sub_main/004A90A8.inc"
};
char gArmorWaistNames_178[24] = {
#include "sub_main/004A90C0.inc"
};
char gArmorWaistNames_179[15] = {
#include "sub_main/004A90D8.inc"
};
char gArmorWaistNames_180[24] = {
#include "sub_main/004A90F0.inc"
};
char gArmorWaistNames_181[15] = {
#include "sub_main/004A9108.inc"
};
char gArmorWaistNames_182[24] = {
#include "sub_main/004A9120.inc"
};
char gArmorWaistNames_183[13] = {
#include "sub_main/004A9138.inc"
};
char gArmorWaistNames_184[24] = {
#include "sub_main/004A9150.inc"
};
char gArmorWaistNames_185[15] = {
#include "sub_main/004A9168.inc"
};
char gArmorWaistNames_186[24] = {
#include "sub_main/004A9180.inc"
};
char gArmorWaistNames_187[15] = {
#include "sub_main/004A9198.inc"
};
char gArmorWaistNames_188[24] = {
#include "sub_main/004A91B0.inc"
};
char gArmorWaistNames_189[15] = {
#include "sub_main/004A91C8.inc"
};
char gArmorWaistNames_190[32] = {
#include "sub_main/004A91E0.inc"
};
char gArmorWaistNames_191[32] = {
#include "sub_main/004A9200.inc"
};
char gArmorWaistNames_192[32] = {
#include "sub_main/004A9220.inc"
};
char gArmorWaistNames_193[32] = {
#include "sub_main/004A9240.inc"
};
char gArmorWaistNames_194[24] = {
#include "sub_main/004A9260.inc"
};
char gArmorWaistNames_195[13] = {
#include "sub_main/004A9278.inc"
};
char gArmorWaistNames_196[24] = {
#include "sub_main/004A9290.inc"
};
char gArmorWaistNames_197[15] = {
#include "sub_main/004A92A8.inc"
};
char gArmorWaistNames_198[24] = {
#include "sub_main/004A92C0.inc"
};
char gArmorWaistNames_199[15] = {
#include "sub_main/004A92D8.inc"
};
char gArmorWaistNames_200[13] = {
#include "sub_main/004A92E8.inc"
};
char gArmorWaistNames_201[15] = {
#include "sub_main/004A92F8.inc"
};
char gArmorWaistNames_202[15] = {
#include "sub_main/004A9308.inc"
};
char gArmorWaistNames_203[24] = {
#include "sub_main/004A9320.inc"
};
char gArmorWaistNames_204[15] = {
#include "sub_main/004A9338.inc"
};
char gArmorWaistNames_205[15] = {
#include "sub_main/004A9348.inc"
};
char gArmorWaistNames_206[15] = {
#include "sub_main/004A9358.inc"
};
char gArmorWaistNames_207[32] = {
#include "sub_main/004A9370.inc"
};
char gArmorWaistNames_208[32] = {
#include "sub_main/004A9390.inc"
};
char gArmorWaistNames_209[24] = {
#include "sub_main/004A93B0.inc"
};
char gArmorWaistNames_210[15] = {
#include "sub_main/004A93C8.inc"
};
char gArmorWaistNames_211[15] = {
#include "sub_main/004A93D8.inc"
};
char gArmorWaistNames_212[24] = {
#include "sub_main/004A93F0.inc"
};
char gArmorWaistNames_213[15] = {
#include "sub_main/004A9408.inc"
};
char gArmorWaistNames_214[13] = {
#include "sub_main/004A9418.inc"
};
char gArmorWaistNames_215[13] = {
#include "sub_main/004A9428.inc"
};
char gArmorWaistNames_216[12] = {
#include "sub_main/004A9438.inc"
};
char gArmorWaistNames_217[12] = {
#include "sub_main/004A9448.inc"
};
char gArmorWaistNames_218[32] = {
#include "sub_main/004A9460.inc"
};
char gArmorWaistNames_219[32] = {
#include "sub_main/004A9480.inc"
};
char gArmorWaistNames_220[24] = {
#include "sub_main/004A94A0.inc"
};
char gArmorWaistNames_221[15] = {
#include "sub_main/004A94B8.inc"
};
char gArmorWaistNames_222[32] = {
#include "sub_main/004A94D0.inc"
};
char gArmorWaistNames_223[32] = {
#include "sub_main/004A94F0.inc"
};
char gArmorWaistNames_224[32] = {
#include "sub_main/004A9510.inc"
};
char gArmorWaistNames_225[24] = {
#include "sub_main/004A9530.inc"
};
char gArmorWaistNames_226[15] = {
#include "sub_main/004A9548.inc"
};
char gArmorWaistNames_227[32] = {
#include "sub_main/004A9560.inc"
};
char D_004A9580[32] = {
#include "sub_main/004A9580.inc"
};
char D_004A95A0[32] = {
#include "sub_main/004A95A0.inc"
};
char gArmorWaistNames_232[32] = {
#include "sub_main/004A95C0.inc"
};
char gArmorWaistNames_233[32] = {
#include "sub_main/004A95E0.inc"
};
char gArmorWaistNames_234[24] = {
#include "sub_main/004A9600.inc"
};
char gArmorWaistNames_235[15] = {
#include "sub_main/004A9618.inc"
};
char gArmorWaistNames_236[15] = {
#include "sub_main/004A9628.inc"
};
char gArmorLegsNames_1[32] = {
#include "sub_main/004A9640.inc"
};
char gArmorLegsNames_2[32] = {
#include "sub_main/004A9660.inc"
};
char gArmorLegsNames_3[32] = {
#include "sub_main/004A9680.inc"
};
char gArmorLegsNames_4[24] = {
#include "sub_main/004A96A0.inc"
};
char gArmorLegsNames_5[15] = {
#include "sub_main/004A96B8.inc"
};
char gArmorLegsNames_6[15] = {
#include "sub_main/004A96C8.inc"
};
char gArmorLegsNames_7[32] = {
#include "sub_main/004A96E0.inc"
};
char gArmorLegsNames_8[24] = {
#include "sub_main/004A9700.inc"
};
char gArmorLegsNames_9[15] = {
#include "sub_main/004A9718.inc"
};
char gArmorLegsNames_10[15] = {
#include "sub_main/004A9728.inc"
};
char gArmorLegsNames_11[24] = {
#include "sub_main/004A9740.inc"
};
char gArmorLegsNames_12[15] = {
#include "sub_main/004A9758.inc"
};
char gArmorLegsNames_13[15] = {
#include "sub_main/004A9768.inc"
};
char gArmorLegsNames_14[15] = {
#include "sub_main/004A9778.inc"
};
char gArmorLegsNames_15[15] = {
#include "sub_main/004A9788.inc"
};
char gArmorLegsNames_16[32] = {
#include "sub_main/004A97A0.inc"
};
char gArmorLegsNames_17[32] = {
#include "sub_main/004A97C0.inc"
};
char gArmorLegsNames_18[32] = {
#include "sub_main/004A97E0.inc"
};
char gArmorLegsNames_19[32] = {
#include "sub_main/004A9800.inc"
};
char gArmorLegsNames_20[32] = {
#include "sub_main/004A9820.inc"
};
char gArmorLegsNames_21[32] = {
#include "sub_main/004A9840.inc"
};
char gArmorLegsNames_22[32] = {
#include "sub_main/004A9860.inc"
};
char gArmorLegsNames_23[32] = {
#include "sub_main/004A9880.inc"
};
char gArmorLegsNames_24[32] = {
#include "sub_main/004A98A0.inc"
};
char gArmorLegsNames_25[32] = {
#include "sub_main/004A98C0.inc"
};
char gArmorLegsNames_26[32] = {
#include "sub_main/004A98E0.inc"
};
char gArmorLegsNames_27[32] = {
#include "sub_main/004A9900.inc"
};
char gArmorLegsNames_28[24] = {
#include "sub_main/004A9920.inc"
};
char gArmorLegsNames_29[15] = {
#include "sub_main/004A9938.inc"
};
char gArmorLegsNames_30[15] = {
#include "sub_main/004A9948.inc"
};
char gArmorLegsNames_31[32] = {
#include "sub_main/004A9960.inc"
};
char gArmorLegsNames_32[24] = {
#include "sub_main/004A9980.inc"
};
char gArmorLegsNames_33[15] = {
#include "sub_main/004A9998.inc"
};
char gArmorLegsNames_34[15] = {
#include "sub_main/004A99A8.inc"
};
char gArmorLegsNames_35[15] = {
#include "sub_main/004A99B8.inc"
};
char gArmorLegsNames_36[15] = {
#include "sub_main/004A99C8.inc"
};
char gArmorLegsNames_37[15] = {
#include "sub_main/004A99D8.inc"
};
char gArmorLegsNames_38[15] = {
#include "sub_main/004A99E8.inc"
};
char gArmorLegsNames_39[32] = {
#include "sub_main/004A9A00.inc"
};
char gArmorLegsNames_40[32] = {
#include "sub_main/004A9A20.inc"
};
char gArmorLegsNames_41[32] = {
#include "sub_main/004A9A40.inc"
};
char gArmorLegsNames_42[32] = {
#include "sub_main/004A9A60.inc"
};
char gArmorLegsNames_43[32] = {
#include "sub_main/004A9A80.inc"
};
char gArmorLegsNames_44[32] = {
#include "sub_main/004A9AA0.inc"
};
char gArmorLegsNames_45[24] = {
#include "sub_main/004A9AC0.inc"
};
char gArmorLegsNames_46[12] = {
#include "sub_main/004A9AD8.inc"
};
char gArmorLegsNames_47[32] = {
#include "sub_main/004A9AF0.inc"
};
char gArmorLegsNames_48[32] = {
#include "sub_main/004A9B10.inc"
};
char gArmorLegsNames_49[24] = {
#include "sub_main/004A9B30.inc"
};
char gArmorLegsNames_50[13] = {
#include "sub_main/004A9B48.inc"
};
char gArmorLegsNames_51[13] = {
#include "sub_main/004A9B58.inc"
};
char gArmorLegsNames_52[32] = {
#include "sub_main/004A9B70.inc"
};
char gArmorLegsNames_53[32] = {
#include "sub_main/004A9B90.inc"
};
char gArmorLegsNames_54[32] = {
#include "sub_main/004A9BB0.inc"
};
char gArmorLegsNames_55[32] = {
#include "sub_main/004A9BD0.inc"
};
char gArmorLegsNames_56[32] = {
#include "sub_main/004A9BF0.inc"
};
char gArmorLegsNames_57[24] = {
#include "sub_main/004A9C10.inc"
};
char gArmorLegsNames_58[13] = {
#include "sub_main/004A9C28.inc"
};
char gArmorLegsNames_59[13] = {
#include "sub_main/004A9C38.inc"
};
char gArmorLegsNames_60[12] = {
#include "sub_main/004A9C48.inc"
};
char gArmorLegsNames_61[12] = {
#include "sub_main/004A9C58.inc"
};
char gArmorLegsNames_62[12] = {
#include "sub_main/004A9C68.inc"
};
char gArmorLegsNames_63[12] = {
#include "sub_main/004A9C78.inc"
};
char gArmorLegsNames_64[15] = {
#include "sub_main/004A9C88.inc"
};
char gArmorLegsNames_65[15] = {
#include "sub_main/004A9C98.inc"
};
char gArmorLegsNames_66[32] = {
#include "sub_main/004A9CB0.inc"
};
char gArmorLegsNames_67[32] = {
#include "sub_main/004A9CD0.inc"
};
char gArmorLegsNames_68[32] = {
#include "sub_main/004A9CF0.inc"
};
char gArmorLegsNames_69[32] = {
#include "sub_main/004A9D10.inc"
};
char gArmorLegsNames_70[32] = {
#include "sub_main/004A9D30.inc"
};
char gArmorLegsNames_71[32] = {
#include "sub_main/004A9D50.inc"
};
char gArmorLegsNames_72[32] = {
#include "sub_main/004A9D70.inc"
};
char gArmorLegsNames_73[32] = {
#include "sub_main/004A9D90.inc"
};
char gArmorLegsNames_74[32] = {
#include "sub_main/004A9DB0.inc"
};
char gArmorLegsNames_75[32] = {
#include "sub_main/004A9DD0.inc"
};
char gArmorLegsNames_76[32] = {
#include "sub_main/004A9DF0.inc"
};
char gArmorLegsNames_77[32] = {
#include "sub_main/004A9E10.inc"
};
char gArmorLegsNames_78[32] = {
#include "sub_main/004A9E30.inc"
};
char gArmorLegsNames_79[32] = {
#include "sub_main/004A9E50.inc"
};
char gArmorLegsNames_80[32] = {
#include "sub_main/004A9E70.inc"
};
char gArmorLegsNames_81[32] = {
#include "sub_main/004A9E90.inc"
};
char gArmorLegsNames_82[32] = {
#include "sub_main/004A9EB0.inc"
};
char gArmorLegsNames_83[32] = {
#include "sub_main/004A9ED0.inc"
};
char gArmorLegsNames_84[32] = {
#include "sub_main/004A9EF0.inc"
};
char gArmorLegsNames_85[32] = {
#include "sub_main/004A9F10.inc"
};
char gArmorLegsNames_86[32] = {
#include "sub_main/004A9F30.inc"
};
char gArmorLegsNames_87[32] = {
#include "sub_main/004A9F50.inc"
};
char gArmorLegsNames_88[32] = {
#include "sub_main/004A9F70.inc"
};
char gArmorLegsNames_89[32] = {
#include "sub_main/004A9F90.inc"
};
char gArmorLegsNames_90[32] = {
#include "sub_main/004A9FB0.inc"
};
char gArmorLegsNames_91[32] = {
#include "sub_main/004A9FD0.inc"
};
char gArmorLegsNames_92[32] = {
#include "sub_main/004A9FF0.inc"
};
char gArmorLegsNames_93[32] = {
#include "sub_main/004AA010.inc"
};
char gArmorLegsNames_94[32] = {
#include "sub_main/004AA030.inc"
};
char gArmorLegsNames_95[32] = {
#include "sub_main/004AA050.inc"
};
char gArmorLegsNames_96[32] = {
#include "sub_main/004AA070.inc"
};
char gArmorLegsNames_97[32] = {
#include "sub_main/004AA090.inc"
};
char gArmorLegsNames_98[32] = {
#include "sub_main/004AA0B0.inc"
};
char gArmorLegsNames_99[32] = {
#include "sub_main/004AA0D0.inc"
};
char gArmorLegsNames_100[32] = {
#include "sub_main/004AA0F0.inc"
};
char gArmorLegsNames_101[32] = {
#include "sub_main/004AA110.inc"
};
char gArmorLegsNames_102[32] = {
#include "sub_main/004AA130.inc"
};
char gArmorLegsNames_103[32] = {
#include "sub_main/004AA150.inc"
};
char gArmorLegsNames_104[32] = {
#include "sub_main/004AA170.inc"
};
char gArmorLegsNames_105[24] = {
#include "sub_main/004AA190.inc"
};
char gArmorLegsNames_106[13] = {
#include "sub_main/004AA1A8.inc"
};
char gArmorLegsNames_107[13] = {
#include "sub_main/004AA1B8.inc"
};
char gArmorLegsNames_108[32] = {
#include "sub_main/004AA1D0.inc"
};
char gArmorLegsNames_109[32] = {
#include "sub_main/004AA1F0.inc"
};
char gArmorLegsNames_110[32] = {
#include "sub_main/004AA210.inc"
};
char gArmorLegsNames_111[32] = {
#include "sub_main/004AA230.inc"
};
char gArmorLegsNames_112[32] = {
#include "sub_main/004AA250.inc"
};
char gArmorLegsNames_113[24] = {
#include "sub_main/004AA270.inc"
};
char gArmorLegsNames_114[13] = {
#include "sub_main/004AA288.inc"
};
char gArmorLegsNames_115[13] = {
#include "sub_main/004AA298.inc"
};
char gArmorLegsNames_116[32] = {
#include "sub_main/004AA2B0.inc"
};
char gArmorLegsNames_117[32] = {
#include "sub_main/004AA2D0.inc"
};
char gArmorLegsNames_118[32] = {
#include "sub_main/004AA2F0.inc"
};
char gArmorLegsNames_119[32] = {
#include "sub_main/004AA310.inc"
};
char gArmorLegsNames_120[32] = {
#include "sub_main/004AA330.inc"
};
char gArmorLegsNames_121[32] = {
#include "sub_main/004AA350.inc"
};
char gArmorLegsNames_122[32] = {
#include "sub_main/004AA370.inc"
};
char gArmorLegsNames_123[32] = {
#include "sub_main/004AA390.inc"
};
char gArmorLegsNames_124[32] = {
#include "sub_main/004AA3B0.inc"
};
char gArmorLegsNames_125[32] = {
#include "sub_main/004AA3D0.inc"
};
char gArmorLegsNames_126[32] = {
#include "sub_main/004AA3F0.inc"
};
char gArmorLegsNames_127[32] = {
#include "sub_main/004AA410.inc"
};
char gArmorLegsNames_128[32] = {
#include "sub_main/004AA430.inc"
};
char gArmorLegsNames_129[32] = {
#include "sub_main/004AA450.inc"
};
char gArmorLegsNames_130[32] = {
#include "sub_main/004AA470.inc"
};
char gArmorLegsNames_131[32] = {
#include "sub_main/004AA490.inc"
};
char gArmorLegsNames_132[32] = {
#include "sub_main/004AA4B0.inc"
};
char gArmorLegsNames_133[32] = {
#include "sub_main/004AA4D0.inc"
};
char gArmorLegsNames_134[32] = {
#include "sub_main/004AA4F0.inc"
};
char gArmorLegsNames_135[32] = {
#include "sub_main/004AA510.inc"
};
char gArmorLegsNames_136[32] = {
#include "sub_main/004AA530.inc"
};
char gArmorLegsNames_137[32] = {
#include "sub_main/004AA550.inc"
};
char gArmorLegsNames_138[32] = {
#include "sub_main/004AA570.inc"
};
char gArmorLegsNames_139[32] = {
#include "sub_main/004AA590.inc"
};
char gArmorLegsNames_140[32] = {
#include "sub_main/004AA5B0.inc"
};
char gArmorLegsNames_141[32] = {
#include "sub_main/004AA5D0.inc"
};
char gArmorLegsNames_142[32] = {
#include "sub_main/004AA5F0.inc"
};
char gArmorLegsNames_143[32] = {
#include "sub_main/004AA610.inc"
};
char gArmorLegsNames_144[32] = {
#include "sub_main/004AA630.inc"
};
char gArmorLegsNames_145[32] = {
#include "sub_main/004AA650.inc"
};
char gArmorLegsNames_146[32] = {
#include "sub_main/004AA670.inc"
};
char gArmorLegsNames_147[32] = {
#include "sub_main/004AA690.inc"
};
char gArmorLegsNames_148[32] = {
#include "sub_main/004AA6B0.inc"
};
char gArmorLegsNames_149[32] = {
#include "sub_main/004AA6D0.inc"
};
char gArmorLegsNames_150[32] = {
#include "sub_main/004AA6F0.inc"
};
char gArmorLegsNames_151[32] = {
#include "sub_main/004AA710.inc"
};
char gArmorLegsNames_152[32] = {
#include "sub_main/004AA730.inc"
};
char gArmorLegsNames_153[32] = {
#include "sub_main/004AA750.inc"
};
char gArmorLegsNames_154[32] = {
#include "sub_main/004AA770.inc"
};
char gArmorLegsNames_155[32] = {
#include "sub_main/004AA790.inc"
};
char gArmorLegsNames_156[32] = {
#include "sub_main/004AA7B0.inc"
};
char gArmorLegsNames_157[32] = {
#include "sub_main/004AA7D0.inc"
};
char gArmorLegsNames_158[32] = {
#include "sub_main/004AA7F0.inc"
};
char gArmorLegsNames_159[32] = {
#include "sub_main/004AA810.inc"
};
char gArmorLegsNames_160[32] = {
#include "sub_main/004AA830.inc"
};
char gArmorLegsNames_161[32] = {
#include "sub_main/004AA850.inc"
};
char gArmorLegsNames_162[32] = {
#include "sub_main/004AA870.inc"
};
char gArmorLegsNames_163[24] = {
#include "sub_main/004AA890.inc"
};
char gArmorLegsNames_164[13] = {
#include "sub_main/004AA8A8.inc"
};
char gArmorLegsNames_165[13] = {
#include "sub_main/004AA8B8.inc"
};
char D_004AA8D0[32] = {
#include "sub_main/004AA8D0.inc"
};
char D_004AA8F0[32] = {
#include "sub_main/004AA8F0.inc"
};
char gArmorLegsNames_170[32] = {
#include "sub_main/004AA910.inc"
};
char gArmorLegsNames_171[24] = {
#include "sub_main/004AA930.inc"
};
char gArmorLegsNames_172[13] = {
#include "sub_main/004AA948.inc"
};
char gArmorLegsNames_173[13] = {
#include "sub_main/004AA958.inc"
};
char gArmorLegsNames_174[32] = {
#include "sub_main/004AA970.inc"
};
char gArmorLegsNames_175[24] = {
#include "sub_main/004AA990.inc"
};
char gArmorLegsNames_176[15] = {
#include "sub_main/004AA9A8.inc"
};
char gArmorLegsNames_177[15] = {
#include "sub_main/004AA9B8.inc"
};
char gArmorLegsNames_178[32] = {
#include "sub_main/004AA9D0.inc"
};
char gArmorLegsNames_179[32] = {
#include "sub_main/004AA9F0.inc"
};
char gArmorLegsNames_180[32] = {
#include "sub_main/004AAA10.inc"
};
char gArmorLegsNames_181[24] = {
#include "sub_main/004AAA30.inc"
};
char gArmorLegsNames_182[15] = {
#include "sub_main/004AAA48.inc"
};
char gArmorLegsNames_183[15] = {
#include "sub_main/004AAA58.inc"
};
char gArmorLegsNames_184[32] = {
#include "sub_main/004AAA70.inc"
};
char gArmorLegsNames_185[32] = {
#include "sub_main/004AAA90.inc"
};
char gArmorLegsNames_186[32] = {
#include "sub_main/004AAAB0.inc"
};
char gArmorLegsNames_187[32] = {
#include "sub_main/004AAAD0.inc"
};
char gArmorLegsNames_188[32] = {
#include "sub_main/004AAAF0.inc"
};
char gArmorLegsNames_189[32] = {
#include "sub_main/004AAB10.inc"
};
char gArmorLegsNames_190[32] = {
#include "sub_main/004AAB30.inc"
};
char gArmorLegsNames_191[32] = {
#include "sub_main/004AAB50.inc"
};
char gArmorLegsNames_192[32] = {
#include "sub_main/004AAB70.inc"
};
char gArmorLegsNames_193[24] = {
#include "sub_main/004AAB90.inc"
};
char gArmorLegsNames_194[15] = {
#include "sub_main/004AABA8.inc"
};
char gArmorLegsNames_195[15] = {
#include "sub_main/004AABB8.inc"
};
char gArmorLegsNames_196[32] = {
#include "sub_main/004AABD0.inc"
};
char gArmorLegsNames_197[32] = {
#include "sub_main/004AABF0.inc"
};
char gArmorLegsNames_198[32] = {
#include "sub_main/004AAC10.inc"
};
char gArmorLegsNames_199[24] = {
#include "sub_main/004AAC30.inc"
};
char gArmorLegsNames_200[15] = {
#include "sub_main/004AAC48.inc"
};
char gArmorLegsNames_201[13] = {
#include "sub_main/004AAC58.inc"
};
char gArmorLegsNames_202[24] = {
#include "sub_main/004AAC70.inc"
};
char gArmorLegsNames_203[15] = {
#include "sub_main/004AAC88.inc"
};
char gArmorLegsNames_204[15] = {
#include "sub_main/004AAC98.inc"
};
char gArmorLegsNames_205[15] = {
#include "sub_main/004AACA8.inc"
};
char gArmorLegsNames_206[15] = {
#include "sub_main/004AACB8.inc"
};
char gArmorLegsNames_207[32] = {
#include "sub_main/004AACD0.inc"
};
char gArmorLegsNames_208[32] = {
#include "sub_main/004AACF0.inc"
};
char gArmorLegsNames_209[24] = {
#include "sub_main/004AAD10.inc"
};
char gArmorLegsNames_210[13] = {
#include "sub_main/004AAD28.inc"
};
char gArmorLegsNames_211[13] = {
#include "sub_main/004AAD38.inc"
};
char gArmorLegsNames_212[32] = {
#include "sub_main/004AAD50.inc"
};
char gArmorLegsNames_213[24] = {
#include "sub_main/004AAD70.inc"
};
char gArmorLegsNames_214[13] = {
#include "sub_main/004AAD88.inc"
};
char gArmorLegsNames_215[13] = {
#include "sub_main/004AAD98.inc"
};
char gArmorLegsNames_216[12] = {
#include "sub_main/004AADA8.inc"
};
char gArmorLegsNames_217[12] = {
#include "sub_main/004AADB8.inc"
};
char gArmorLegsNames_218[32] = {
#include "sub_main/004AADD0.inc"
};
char gArmorLegsNames_219[32] = {
#include "sub_main/004AADF0.inc"
};
char gArmorLegsNames_220[32] = {
#include "sub_main/004AAE10.inc"
};
char gArmorLegsNames_221[32] = {
#include "sub_main/004AAE30.inc"
};
char gArmorLegsNames_222[32] = {
#include "sub_main/004AAE50.inc"
};
char gArmorLegsNames_223[32] = {
#include "sub_main/004AAE70.inc"
};
char gArmorLegsNames_224[32] = {
#include "sub_main/004AAE90.inc"
};
char gArmorLegsNames_225[32] = {
#include "sub_main/004AAEB0.inc"
};
char gArmorLegsNames_226[32] = {
#include "sub_main/004AAED0.inc"
};
char D_004AAEF0[32] = {
#include "sub_main/004AAEF0.inc"
};
char D_004AAF10[32] = {
#include "sub_main/004AAF10.inc"
};
char gArmorLegsNames_231[32] = {
#include "sub_main/004AAF30.inc"
};
char gArmorLegsNames_232[32] = {
#include "sub_main/004AAF50.inc"
};
char gArmorLegsNames_233[32] = {
#include "sub_main/004AAF70.inc"
};
char gArmorLegsNames_234[32] = {
#include "sub_main/004AAF90.inc"
};
char gArmorLegsNames_235[32] = {
#include "sub_main/004AAFB0.inc"
};
#pragma rodata off

#pragma optimization_level reset

/* ---- The unit's second part ---- */
/*
 * gSave's points (the lobby's point shop), the training courses' best
 * scores and times, the MH Portable link's count and flags, the hunter
 * titles' bits, and the decorations' count.
 */


/* unit's own declaration: declared here as these callers use it; with the definition's s32 (u16 id), Dojo_SetBestScore and Dojo_SetBestTime compile differently. */
s32 Dojo_FindQuest(s32 a);

/* Adds to gSave's points (SaveData.points, which the lobby's point shop spends), within 0 and
 * 9,999,999. */
void Save_AddPoints(s32 amount)
{
    gSave.points += amount;
    if (POINTS_MAX < gSave.points) {
        gSave.points = POINTS_MAX;
    }
    if (gSave.points < 0) {
        gSave.points = 0;
    }
}

/* gSave's points. */
s32 Save_GetPoints(void)
{
    return gSave.points;
}

/* Records a score as the best for a training course (gSave.dojoScore), by the course's quest id
 * (Dojo_FindQuest); nothing for a quest that isn't a course. */
void Dojo_SetBestScore(s32 quest, s32 value)
{
    s32 course = Dojo_FindQuest(quest);

    if (course >= 0) {
        gSave.dojoScore[course] = value;
    }
}

/* Records a time as the best for a training course (gSave.dojoTime), as Dojo_SetBestScore. */
void Dojo_SetBestTime(s32 quest, s32 value)
{
    s32 course = Dojo_FindQuest(quest);

    if (course >= 0) {
        gSave.dojoTime[course] = value;
    }
}

/* Counts one more link with MH Portable (at most 9,999,999). */
void PspLink_AddCount(void)
{
    gSave.pspLinkCount++;
    if (9999999 < gSave.pspLinkCount) {
        gSave.pspLinkCount = 9999999;
    }
}

/* How many times the save has linked with MH Portable. */
s32 PspLink_Count(void)
{
    return gSave.pspLinkCount;
}

/* Sets what the MH Portable data had reached (SaveData.pspLinkFlags). */
void PspLink_SetFlags(u32 flags)
{
    gSave.pspLinkFlags = flags;
}

/* Clears the MH Portable link flags and event flag 0x7E. */
void PspLink_ClearFlags(void)
{
    gSave.pspLinkFlags = 0;
    Save_ClearEventFlag(126);
}

/* 1 when bit `bit` of the MH Portable link flags is set. */
s32 PspLink_HasFlag(s32 bit)
{
    return (gSave.pspLinkFlags & (1 << bit)) != 0;
}

/* Marks a hunter title earned in the save given (SaveData.titles). */
void Save_SetTitleIn(SaveData *save, s32 title)
{
    ((u32 *)save->titles)[title / 32] |= 1 << (title & 0x1F);
}

/* Save_SetTitleIn on gSave. */
void Save_SetTitle(s32 title)
{
    Save_SetTitleIn(&gSave, title);
}

/* 1 when gSave has earned a hunter title. */
s32 Save_HasTitle(s32 title)
{
    return Bits32_Test((u32 *)gSave.titles, title);
}

/* 1 when bit `bit` of a bit array of words is set. */
s32 Bits32_Test(u32 *bits, s32 bit)
{
    return (bits[bit / 32] & (1 << (bit & 0x1F))) != 0;
}

/* The number of decorations set in an item of equipment. */
s32 Equip_CountDecos(EquipItem *item)
{
    s32 count = 0;
    s32 i;

    for (i = 0; i < 3; i++) {
        if (item->deco[i] != 0) {
            count++;
        }
    }
    return count;
}

/* ---- The unit's third part ---- */
/*
 * Decorations in the equipment slots (EquipItem.deco), the system data's
 * event flags (gSysSave.eventFlags), and quest-condition helpers (hunting
 * records, the equipment box).
 */

/* Only the flags byte of the armour records (ArmorData). */

static void Equip_CompactDecorations(EquipItem *e);

/* The free decoration slots of a piece of equipment. Kept for the match: the record is read as
 * its halfwords from &item->used (deco[i + 3] is item->deco[i]). The game reads each decoration
 * at its offset from the record itself; written as item->deco[i], or through a pointer to the
 * decorations, the compiler keeps other registers or adds the offset before the loop. */
s32 Equip_FreeSlots(EquipItem *item)
{
    s32 i;
    s32 used;
    s32 n;
    u16 *deco;
    u8 slots;

    deco = (u16 *)&item->used;
    slots = Equip_GetSlotCount(item);
    if (slots <= 0) {
        return 0;
    }
    if (slots > 3) {
        slots = 3;
    }
    n = slots;
    used = 0;
    for (i = 0; i < n; i++) {
        if (deco[i + 3] != 0) {
            u8 n = Item_GetDecoration(deco[i + 3])->size;

            if (n == 0 || n > 3) {
                n = 1;
            }
            used += n;
        }
    }
    if (used > slots) {
        return 0;
    }
    return slots - used;
}

/* The decoration record of an item, NULL for other items. */
DecoData *Item_GetDecoration(u16 item)
{
    u16 k = gItemData[item].deco;

    if (k == 0) {
        return NULL;
    }
    return &gDecoTable[k];
}

/* 1 if a decoration fits a piece of equipment. */
/* Matches only with Armor_GetData static (this file's opening comment). */
s32 Deco_FitsEquip(EquipItem *equip, u16 item)
{
    DecoData *deco = Item_GetDecoration(item);

    if (deco == NULL) {
        return 0;
    }
    switch (equip->type) {
    case EQUIP_BLADE:
        if (deco->flags & 1) {
            return 1;
        }
        break;
    case EQUIP_GUN:
        if (deco->flags & 2) {
            return 1;
        }
        break;
    default:
        if (equip->type > EQUIP_WAIST) {
            return 0;
        }
        if (deco->flags & (u8)(Armor_GetData(equip->type, equip->id)->flags & 0xC)) {
            return 1;
        }
        break;
    }
    return 0;
}

/* Puts a decoration in the first empty slot. */
void Equip_AddDecoration(EquipItem *equip, u16 item)
{
    s32 i;

    for (i = 0; i < 3; i++) {
        if (equip->deco[i] == 0) {
            equip->deco[i] = item;
            return;
        }
    }
}

/* Moves the decorations to the first slots. */
static void Equip_CompactDecorations(EquipItem *item)
{
    u16 secondDeco = item->deco[1];
    u16 firstDeco = item->deco[0];
    u16 thirdDeco = item->deco[2];

    if (firstDeco + secondDeco + thirdDeco == 0) {
        return;
    }
    if (firstDeco == 0) {
        if (secondDeco != 0) {
            item->deco[0] = secondDeco;
            item->deco[1] = 0;
        } else if (thirdDeco != 0) {
            item->deco[0] = thirdDeco;
            item->deco[2] = 0;
        }
    }
    if (item->deco[1] == 0 && item->deco[2] != 0) {
        item->deco[1] = item->deco[2];
        item->deco[2] = 0;
    }
}

/* Takes the decoration out of slot i. */
void Equip_RemoveDecoration(EquipItem *item, s32 i)
{
    item->deco[i] = 0;
    Equip_CompactDecorations(item);
}

/* The decoration in each slot it takes (3 entries, 0: empty). */
void Equip_ListSlots(EquipItem *item, u16 *out)
{
    s32 i;
    s32 k;

    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    for (i = 0; i < 3; i++) {
        if (item->deco[i] != 0) {
            DecoData *decoData = Item_GetDecoration(item->deco[i]);

            for (k = 0; k < decoData->size; k++) {
                *out++ = item->deco[i];
            }
        }
    }
}

/* Sets event flag n of the system data (gSysSave.eventFlags), which every hunter shares. */
void EventFlag_Set(s32 n)
{
    ((u32 *)gSysSave.eventFlags)[n / 32] |= 1 << (n % 32);
}

/* 1 when event flag n of the system data (gSysSave.eventFlags) is set. */
s32 EventFlag_Test(s32 n)
{
    u32 *word = &((u32 *)gSysSave.eventFlags)[n / 32];

    return (*word & (1 << (n % 32))) != 0;
}

/* Sets gSysSave.bonusUnlocked (the character editor's last hair style, and what the lobby and the
 * select overlay test with event flag 0x7C). */
void SysSave_UnlockBonus(void)
{
    gSysSave.bonusUnlocked = 1;
}

u8 Equip_GetClass(EquipItem *item);

/* Counts the equipment of the lists gEquipCollectType6 (type 6) and gEquipCollectType7 (type 7) by class
 * (Equip_GetClass) and how many the player owns (Equip_TestFlag): each class all owned gives
 * Award_Grant(gEquipCollectClassAwards[class]); at least 80% of all, Award_Grant(AWARD_WEAPONS) and RateFlag_SetAndNotify(RATE_FLAG_WEAPONS). */
void EquipCollect_CheckWeapons(void)
{
    s16 owned[11];
    s16 total[11];
    EquipItem item;
    s16 i;
    s16 have;
    s16 all;
    u16 *id;
    u16 weaponId;
    u8 weaponClass;
    u8 *type;

    have = 0;
    all = 0;
    for (i = 0; i < 11; i++) {
        owned[i] = 0;
        total[i] = 0;
    }
    id = gEquipCollectType6;
    type = &item.type;
    item.used = 1;
    *type = 6;
    weaponId = *id;
    while (weaponId != EQUIP_LIST_END) {
        item.id = weaponId;
        id++;
        weaponClass = Equip_GetClass(&item);
        all++;
        total[weaponClass]++;
        if (Equip_TestFlag(&item, 0) == 1) {
            have++;
            owned[weaponClass]++;
        }
        weaponId = *id;
    }
    id = gEquipCollectType7;
    *type = 7;
    weaponId = *id;
    while (weaponId != EQUIP_LIST_END) {
        item.id = weaponId;
        id++;
        weaponClass = Equip_GetClass(&item);
        all++;
        total[weaponClass]++;
        if (Equip_TestFlag(&item, 0) == 1) {
            have++;
            owned[weaponClass]++;
        }
        weaponId = *id;
    }
    for (i = 0; i < 11; i++) {
        if (owned[i] == total[i]) {
            Award_Grant(gEquipCollectClassAwards[i]);
        }
    }
    if (have >= (s16)(all * 0.8f)) {
        Award_Grant(AWARD_WEAPONS);
        RateFlag_SetAndNotify(RATE_FLAG_WEAPONS);
    }
}


/* Counts the equipment of the five lists gEquipCollectLists (kind gEquipCollectKinds, ending EQUIP_LIST_END) the
 * player owns (Equip_TestFlag); with at least half of them, Award_Grant(AWARD_ARMOR) and
 * RateFlag_SetAndNotify(RATE_FLAG_ARMOR). */
void EquipCollect_CheckArmor(void)
{
    EquipItem item;
    s16 owned;
    s16 total;
    s16 list;
    u16 *id;

    item.used = 1;
    owned = 0;
    total = 0;
    for (list = 0; list < 5; list++) {
        id = gEquipCollectLists[list];
        item.type = gEquipCollectKinds[list];
        while (*id != EQUIP_LIST_END) {
            item.id = *id;
            total++;
            id++;
            if (Equip_TestFlag(&item, 0) == 1) {
                owned++;
            }
        }
    }
    if (owned >= (s16)(total * 0.5f)) {
        Award_Grant(AWARD_ARMOR);
        RateFlag_SetAndNotify(RATE_FLAG_ARMOR);
    }
}

/* 1 if both ids of an equipment item pass Armor_LacksSexModel (type 2 only). */
s32 Equip_IsNoModelArmor(EquipItem *item)
{
    if (item->type != 2 || item->id == 0) {
        return 0;
    }
    if (Armor_LacksSexModel(item->id, 0) == 1 && Armor_LacksSexModel(item->id, 1) == 1) {
        return 1;
    }
    return 0;
}

/* 1 while n does not exceed gSave.playTime / 3600 (minutes of play?). */
s32 Save_PlayTimeReached(s16 n)
{
    return !(gSave.playTime / 3600 < n);
}

/* 1 if the monster (or, with byMonster 0, the monster group) was hunted and captured count
 * times in all. */
s32 Records_HuntedAtLeast(f32 byMonster, SaveData *save, u8 monster, u16 count)
{
    u16 total;

    if (byMonster == 0.0f) {
        total = Record_GroupHunted(save, monster);
        total += Record_GroupCaptured(save, monster);
    } else {
        total = Record_GetHunted(save, monster);
        total += Record_GetCaptured(save, monster);
    }
    if (total >= count) {
        return 1;
    }
    return 0;
}

/* 1 if the equipment box holds a piece of that type and id. */
s32 Box_HasEquip(EquipItem *item)
{
    SaveData *save = &gSave;
    s16 i;
    s16 n = Box_GetCapacity(save);

    for (i = 0; i < n; i++) {
        if (save->box[i].used != 0 && save->box[i].type == item->type && save->box[i].id == item->id) {
            return 1;
        }
    }
    return 0;
}
