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
 * from there (Equip_CompactDecorations needs 3).
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
#include "award.h"

#pragma optimization_level 4

extern ArmorData *gArmorTables[];

extern u8 gWeaponKindClass[];         /* weapon kind -> weapon class */
extern u8 D_0044BF3B[];         /* class of the ids from 0x385 */
extern u8 gWeaponClassInfo[][2];      /* per weapon class: gunner flag, unk774 */
extern u8 gWeaponClassAttack[][2];      /* per weapon class: unk8A2, unk8A3 */
extern SaveData *gCardSlots;    /* the save slots read from the memory card */

extern u16 gRandSeeds[];

s32 Player_HasSkill(Player *p, u8 skill);
s32 Player_IsLocal(void *obj);
/* As Equip_SetFlag on save. */
void Save_SetEquipFlag(SaveData *save, EquipItem *item, s16 bit);
s32 Player_HasSongEffect(Player *pl, s16 kind);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);
void PspLink_ClearFlags(void);
void Equip_Refresh(SaveData *save);
void Player_SetColorWord(Player *p, u8 i);
s32 Byte_HighNibble(u8 x);
s32 Game_IsOnline(void);
char *strcpy(char *dst, char *src);    /* strcpy */
s32 Award_Grant(s32 arg0);
s32 Save_HrGateCleared(s32 n);
f32 Math_Absf(f32 x);
s32 Item_IsOnePerPouch(u16 item);
s16 Box_GetCapacity(SaveData *save);
void Save_MarkItemObtained(SaveData *save, u16 item);
s32 Save_IsItemObtained(SaveData *save, u16 item);
s32 Item_IsRecordOnly(u16 item);
u32 Hr_PointsForRank(u16 hr);
u16 Save_RankFromPoints(SaveData *s);
void Save_SetRankCapPoints(SaveData *s, s32 hr);
s32 Save_AddHrPoints(SaveData *s, s32 n);
s32 SavePouch_HasSpecial(SaveData *s);
u32 ItemBox_AddTo(SaveData *s, u16 item, u32 count);

/* A band of hunter ranks above 50 (gHrPointSteps, Hr_PointsForRank): the band's last rank and
 * the points each of its ranks adds. The list ends with a limit of 0xFFFF; the ranks up to 50
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
    u8 teams[0x42];             /* NextQuest.teams, teamItems, unk44 */
    s8 ids[4][6];               /* NextQuest.sideIds */
} SideIdsView;

extern ItemStack gPouchBackup[20];
extern UserText gPlayerCardTexts[];     /* the party's hunter card texts, one a player, found by the id (Preset_Find) */
extern char gMemberNames[][17];   /* the preset names */
extern s8 gPlayerIds[][8];      /* the preset ids */
extern NextQuest gNextQuest;
extern s8 gMyUserId[8];
/* gScene.userIds as its own symbol: with gScene.userIds[i] MWCC folds 0x276 into the stores. */
extern s8 D_006ADF26[][8];

void *memcpy(void *dst, void *src, u32 n);
void MonsterNote_OnItemGet(u16 item);
s32 Menu_PushNoticeOnce(s32 n);
s32 Quest_IsVersus(void);
s16 Pouch_Count(Player *p, u16 item);
s32 SavePouch_ClearSlot(s16 slot);
s32 SavePouch_Count(u16 item);
s16 PouchBackup_Count(u16 item);
s8 Preset_Find(s8 *id);
s32 Quest_IsDrill(void);
s32 Quest_IsTraining(void);
s16 EquipView_GetStat(s32 kind);
void EquipView_FromPlayer(Player *p);
s32 QuestGoal_IsDone(s32 n);
u16 ClearList_IndexOf(u16 item);
void Em_BonePos(void *obj, s16 bone, f32 *out);
f32 Vec_Distance(f32 *a, f32 *b);

extern Em gEms[];

extern u8 gAmmoLevels[][4];      /* per ammo kind: +0, +1 compared with the gun */
extern EquipItem gEquipShown[7]; /* the equipment shown: [0] the weapon, [1 + type] each armour piece (EquipType 0-5) */
extern s16 gSilencerBonus;
extern s16 gBarrelBonusHeavy[];
extern s16 gBarrelBonusLight[];
extern u16 gClearListQuests[];        /* item ids, ended by 0xEA60 (bits in gSave unk1FFC) */


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

/* The weapon class (WEAPON_*) of a weapon by its type and id. Ids above 0x384 read the byte table
 * that starts 0xB bytes into gStageFileTbls_4, at the id: no weapon table goes that far.
 */
u8 Weapon_GetClass(u8 type, u16 id)
{
    if (id > 0x384) {
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

extern char *gArmorLegsNames[];
extern char *gArmorHeadNames[];
extern char *gArmorBodyNames[];
extern char *gArmorArmsNames[];
extern char *gArmorWaistNames[];
extern char *gBladeNames[];
extern char *gGunNames[];

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

/* The carried item in the player's pouch (one of the items 0x29C, 0x29E-0x2A3, which Item_CarryKind
 * knows), 0xFFFF if none; with more than one, the last slot's. */
u16 Pouch_FindCarryItem(Player *pl)
{
    u16 found = 0xFFFF;
    s16 i;

    for (i = 0; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].count != 0) {
            switch (pl->pouch[i].item) {
            case 0x29C:
            case 0x29E:
            case 0x29F:
            case 0x2A0:
            case 0x2A1:
            case 0x2A2:
            case 0x2A3:
                found = pl->pouch[i].item;
                break;
            }
        }
    }
    return found;
}

/* The kind of a carried item (Player.carry: 1-7 for the items 0x29C, 0x29E-0x2A3, in the order the
 * switch gives), 0 for any other item. */
u8 Item_CarryKind(u16 item)
{
    switch (item) {
    case 0x29E:
        return 1;
    case 0x29F:
        return 2;
    case 0x29C:
        return 3;
    case 0x2A2:
        return 4;
    case 0x2A1:
        return 5;
    case 0x2A3:
        return 6;
    case 0x2A0:
        return 7;
    }
    return 0;
}

/* 1 when the player carries well: with skill 0x5F, or carrying an item of kind 6 (Player.carry's
 * high nibble). Seen: the carry walk takes a longer stride, and a fall must last longer before the
 * item drops (the game's carry states). */
s32 Player_HasCarryBonus(Player *pl)
{
    if (Player_HasSkill(pl, 0x5F) == 1) {
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
 * (gAmmoLevels[ammo][1]) less the gun's GunData.reload, less 1, 2 or 3 for the skills 0x2E-0x30 and
 * plus 1 or 3 for 0x31 and 0x32; up to 4 gives 0, up to 7 gives 1. */
u8 Bowgun_ReloadLevel(Player *pl, u8 ammo)
{
    s16 level = gAmmoLevels[ammo][1] - gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].reload;

    if (Player_HasSkill(pl, 0x2E) == 1) {
        level -= 1;
    }
    if (Player_HasSkill(pl, 0x2F) == 1) {
        level -= 2;
    }
    if (Player_HasSkill(pl, 0x30) == 1) {
        level -= 3;
    }
    if (Player_HasSkill(pl, 0x31) == 1) {
        level += 1;
    }
    if (Player_HasSkill(pl, 0x32) == 1) {
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
 * without a gun. The ammo's own value (gAmmoLevels[ammo][0]), and unless skill 0x33 cancels it,
 * less the gun's GunData.recoil and less 2 or 4 for the skills 0x34 or 0x35; up to 8 gives 0, up to
 * 10 gives 1. */
s16 Bowgun_RecoilLevel(Player *pl, u8 ammo)
{
    s16 level;

    if (pl->equip[EQUIP_SLOT_WEAPON].type != EQUIP_GUN) {
        return 2;
    }
    level = gAmmoLevels[ammo][0];
    if (Player_HasSkill(pl, 0x33) == 0) {
        level -= gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].recoil;
        if (Player_HasSkill(pl, 0x34) == 1) {
            level -= 2;
        } else if (Player_HasSkill(pl, 0x35) == 1) {
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

s32 Side_Match(u8 a, u8 b, u8 none);

/* Which horn item a player on side sideWanted is playing: 1-7 for the items 0x2B0-0x2B6, from the
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
            case 0x2B0:
                played = 1;
                break;
            case 0x2B1:
                played = 2;
                break;
            case 0x2B2:
                played = 3;
                break;
            case 0x2B3:
                played = 4;
                break;
            case 0x2B4:
                played = 5;
                break;
            case 0x2B5:
                played = 6;
                break;
            case 0x2B6:
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
        case 0x02:
        case 0x14:
        case 0x20:
        case 0x24:
        case 0x4E:
        case 0x58:
        case 0x63:
        case 0x64:
        case 0x6B:
        case 0x6C:
        case 0x77:
        case 0x8B:
        case 0xAB:
        case 0xB9:
        case 0xBF:
        case 0xC9:
        case 0xCB:
        case 0xD8:
        case 0xD9:
        case 0xDA:
            raise = 40.0f;
            break;
        }
    } else {
        switch (head->modelFemale) {
        case 0x20:
        case 0x2A:
        case 0x6B:
        case 0x6C:
        case 0x7A:
        case 0x7C:
        case 0x7F:
        case 0x8B:
        case 0x8C:
        case 0x90:
        case 0x95:
        case 0xC9:
        case 0xCB:
        case 0xD8:
        case 0xD9:
        case 0xDA:
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
 * grants award 0x1E. */
void Save_AddMoney(s32 n)
{
    gSave.money += n;
    if (gSave.money > MONEY_MAX) {
        gSave.money = MONEY_MAX;
    }
    if (gSave.money < 0) {
        gSave.money = 0;
    } else if (gSave.money >= 2000000) {
        Award_Grant(0x1E);
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

/* 1 for the items 0x271-0x28E, which are never carried: getting one only marks it obtained
 * (SavePouch_Add). */
s32 Item_IsRecordOnly(u16 item)
{
    if (item >= 0x271 && item < 0x28F) {
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
    if (gItemData[item].max == 0xFF) {
        amount = 0xFF;
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
                if (amount < 0 && gItemData[item].max == 0xFF) {
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
 * none; 0xFF for an item with no limit); for a special item, 1 when the pouch holds none. */
s32 SavePouch_Room(u16 item)
{
    SaveData *save = &gSave;
    s32 i;

    if (Item_IsOnePerPouch(item) == 1) {
        return SavePouch_HasSpecial(save) != 1;
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (save->pouch[i].item == item) {
            if (gItemData[item].max == 0xFF) {
                return 0xFF;
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
            if (gItemData[item].max == 0xFF) {
                return 0xFF;
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
            return gItemData[item].max == 0xFF ? 999 : save->itemBox[i].count;
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
            if (gItemData[item].max == 0xFF) {
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

    if (item == 0 || gItemData[item].max == 0xFF) {
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
        if (stack->item == item && max != 0xFF) {
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
    if (item == 0 || gItemData[item].max == 0xFF) {
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
    if (max != 0xFF) {
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
    if (max != 0xFF) {
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
 * the count 0xFF. Returns how many found no place (0 when all did). */
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
        if (stack->item == item && stack->count < stackMax && stackMax != 0xFF) {
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
            if (stackMax == 0xFF) {
                stack->count = 0xFF;
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
 * the backup's stack ran out, 1 when some is left, and 0xFF for a positive amount, an item with no
 * limit, or an item neither holds. */
u8 PouchBackup_Use(Player *pl, u16 item, s16 amount)
{
    s16 backupCount;
    s16 pouchCount;
    s16 i;
    u8 result;

    if (amount > 0) {
        return 0xFF;
    }
    if (gItemData[item].max == 0xFF) {
        return 0xFF;
    }
    backupCount = PouchBackup_Count(item);
    pouchCount = Pouch_Count(pl, item);
    if (backupCount == 0 && pouchCount == 0) {
        return 0xFF;
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
 * for the skills 0x23-0x25, and for a blade 5 at sharpness level 4 or 10 above it when positive.
 * The skill 0x47 raises the statuses and the dragon element (WSTAT_POISON to WSTAT_DRAGON) by an
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
        if (Player_HasSkill(pl, 0x23) == 1) {
            result += 5;
        } else if (Player_HasSkill(pl, 0x24) == 1) {
            result += 10;
        } else if (Player_HasSkill(pl, 0x25) == 1) {
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
    if (Player_HasSkill(pl, 0x47) == 1 && result > 0 && kind >= WSTAT_POISON && kind < WSTAT_AFFINITY) {
        result = 1.125f * result;
    }
    return result;
}

u32 Armor_GetDefense(EquipItem *item);

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

/* The index of a quest id in gClearListQuests (its bit in SaveData.clearedQuests), 0xFFFF if it
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
    return 0xFFFF;
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

/* Armor data (0x28 bytes), one table per armor type (gArmorTables). */

/* A forging recipe (0x18 bytes): the tables gArmorRecipes and gWeaponRecipes are outside
 * sub_main (Forge_CheckRecipe). The lobby's forge menus view it as their own ForgeRecipe. */
typedef struct ForgeRecipe {
    u8 type;                    /* 0x0: gWeaponRecipes: EQUIP_BLADE or EQUIP_GUN */
    u8 alwaysShown;             /* 0x1: nonzero: Forge_CheckRecipe gives 1 even while locked */
    u16 id;
    ItemStack mat[4];           /* 0x4: materials */
    u8 cond[4];                 /* 0x14: per material, the condition that unlocks the recipe (Forge_CheckRecipe) */
} ForgeRecipe;

/* One page of the box (25 entries), built by Save_BuildSummary (0x19C bytes). */
typedef struct BoxPage {
    u16 id[SUMMARY_BOX_SLOTS];                 /* 0x000 */
    u16 level[SUMMARY_BOX_SLOTS];              /* 0x032 */
    u16 deco0[SUMMARY_BOX_SLOTS];              /* 0x064: EquipItem.deco[0] */
    u16 deco1[SUMMARY_BOX_SLOTS];              /* 0x096 */
    u16 deco2[SUMMARY_BOX_SLOTS];              /* 0x0C8 */
    s32 hrPoints;               /* 0x0FC */
    u32 itemBits[ITEM_BITS_WORDS];           /* 0x100: SaveData.itemBits */
    u16 hr;                     /* 0x180 */
    u8 type[SUMMARY_BOX_SLOTS];                /* 0x182: EquipItem.type, 0xFF for an empty slot */
} BoxPage;

/* The equipped items, built by Save_BuildSummary (0xC4 bytes). */
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

extern ItemStack gPouchBackup[20];
extern EquipSummary gSaveSummaryEquip;
extern BoxPage gSaveSummaryBox;
extern u8 *gMonsterGroups[];
extern f32 gMonsterBaseSize[];
extern u16 gObtainFlagItems[];
extern u32 gPlayerColorWords[];
extern u8 *gPlayerPartFlagTbls[];
extern u8 gWeaponKindClass[];
extern f32 gArmorLevelDefense[];
extern f32 gArmorLevelCost[];
extern u8 gArmorLevelSlots[];
extern s32 gWeaponClassMultipliers[];        /* per weapon class: attack multiplier x 100 */
extern ForgeRecipe gArmorRecipes[];
extern ForgeRecipe gWeaponRecipes[];

void *memcpy(void *dst, void *src, u32 n);
s32 Award_Grant(s32 arg0);
f32 Math_Floor(f32 x);
void Save_PickBestCounter(EquipItem *unused0, u8 unused1);
void Player_CalcAttack(Player *pl);
void Player_CalcDefense(Player *pl);
void Player_CalcResists(Player *pl);
void Player_CalcSkills(Player *pl);
u32 Bowgun_GetAmmoMask(Player *p);
u8 Weapon_GetClass(u8 a, u16 weapon);
s32 SavePouch_Count(u16 item);
s32 ItemBox_Count(u16 item);
s32 ItemBox_Total(u16 item);
s32 Save_TestEventFlag(s32 n);
s32 Equip_IsSlotEquipped(SaveData *save, s32 idx);
u16 Record_GroupHunted(SaveData *save, u8 group);
u16 Record_GroupCaptured(SaveData *save, u8 group);
s16 Box_GetPageCount(SaveData *save);
s16 Box_GetCapacity(SaveData *save);
void Save_SetItemObtained(SaveData *save, u16 item);
s32 Save_IsItemObtained(SaveData *save, u16 item);
s32 Save_HasTitle(s32 n);
void EquipCollect_CheckWeapons(void);
void EquipCollect_CheckArmor(void);

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

/* The first free box slot of gSave, 0xFFFF when full. */
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

/* Puts an item in the first free box slot and flags it seen; its index or 0xFFFF. */
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

/* Copies the equipped box slots into save->equip (no weapon: blade weapon 0x9B). */
void Equip_Refresh(SaveData *save)
{
    if (Box_SlotHasType(save, save->equipIdx[SAVE_EQUIP_WEAPON], EQUIP_BLADE, EQUIP_GUN) == 1) {
        save->equip[SAVE_EQUIP_WEAPON] = save->box[save->equipIdx[SAVE_EQUIP_WEAPON]];
    } else {
        save->equip[SAVE_EQUIP_WEAPON].used = save->box[save->equipIdx[SAVE_EQUIP_WEAPON]].used;
        save->equip[SAVE_EQUIP_WEAPON].type = EQUIP_BLADE;
        save->equip[SAVE_EQUIP_WEAPON].id = 0x9B;
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
 * ForgeRecipe.cond): 0 and 1 always; 2 a hunter rank above 0; 3, 4 and 5 the event flags 0x7F, 0x82
 * and 0x83; 6 a monster of group 0xB hunted or captured; 7 both 2 and 6. */
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
        if (Save_TestEventFlag(0x7F)) {
            ok = 1;
        }
        break;
    case 4:
        if (Save_TestEventFlag(0x82)) {
            ok = 1;
        }
        break;
    case 5:
        if (Save_TestEventFlag(0x83)) {
            ok = 1;
        }
        break;
    case 6:
        if (Record_GroupHunted(&gSave, 0xB) > 0
            || Record_GroupCaptured(&gSave, 0xB) > 0) {
            ok = 1;
        }
        break;
    case 7:
        if (gSave.hr > 0
            && (Record_GroupHunted(&gSave, 0xB) > 0
                || Record_GroupCaptured(&gSave, 0xB) > 0)) {
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
                if (Save_TestEventFlag(0x7F)) {
                    unlocked = 1;
                }
                break;
            case 4:
                if (Save_TestEventFlag(0x82)) {
                    unlocked = 1;
                }
                break;
            case 5:
                if (Save_TestEventFlag(0x83)) {
                    unlocked = 1;
                }
                break;
            case 6:
                if (obtained > 0
                    && (Record_GroupHunted(&gSave, 0xB) > 0
                        || Record_GroupCaptured(&gSave, 0xB) > 0)) {
                    unlocked = 1;
                }
                break;
            case 7:
                if (obtained > 0 && gSave.hr > 0
                    && (Record_GroupHunted(&gSave, 0xB) > 0
                        || Record_GroupCaptured(&gSave, 0xB) > 0)) {
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
        if (okMask == 0xF) {
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

/* 1 when the gun in box slot idx has the option bits in mask (GUN_OPT_*), 0 when not; 0xFF when the
 * slot holds no gun. */
s32 Box_TestGunOption(SaveData *save, s16 idx, s32 mask)
{
    if (Box_SlotIsGun(save, idx) == 0) {
        return 0xFF;
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
 * blademaster, 8 gunner); 0xF for a weapon, 0 for an unknown type. */
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
        return 0xF;
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
 * variants, 0xFF-terminated), and which monster it was.
 */
u16 Record_GroupLargest(SaveData *save, u8 group, u8 *out)
{
    u16 best = 0;
    u8 *member = gMonsterGroups[group];
    u16 size;

    for (; *member != 0xFF; member++) {
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

    for (; *member != 0xFF; member++) {
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
    for (; *member != 0xFF; member++) {
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

    for (; *member != 0xFF; member++) {
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

    for (; *member != 0xFF; member++) {
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

    for (; *member != 0xFF; member++) {
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

    for (; *member != 0xFF; member++) {
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

    for (; *member != 0xFF; member++) {
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
 * SAVE_SUMMARY_BOX, SUMMARY_BOX_SLOTS slots of the box from page * SUMMARY_BOX_SLOTS (type 0xFF for
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
                boxPage->type[i] = 0xFF;
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

/* Box pages owned: 1 + items 0x26D-0x270 obtained (reads gSave, not save). */
s16 Box_GetPageCount(SaveData *save)
{
    s32 n = 1;
    s32 i;

    for (i = 0x26D; i <= 0x270; i++) {
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
 * the items 0x27, 0x45, 0x50 and 0x82. Returns 1 when met, 0 when not or for another kind. */
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
                case 0x50:
                case 0x45:
                case 0x82:
                case 0x27:
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
    if (Save_HasTitle(0x27) != 1) {
        entry = gObtainFlagItems;
        if (gObtainFlagItems[0] != 0xFFFF) {
            do {
                if (Save_IsItemObtained(save, *entry++) == 1) {
                    count++;
                }
            } while (*entry != 0xFFFF);
        }
        if (count >= 15) {
            Award_Grant(0x27);
        }
    }
}

/* 1 when item's bit is set in the bits given (one per item id), 0 when not; 0xFFFF for an id out of
 * range (0xFFFF or ITEM_ID_COUNT and above). */
s32 ItemBits_Test(u32 *bits, u16 item)
{
    if (item == 0xFFFF || item >= ITEM_ID_COUNT) {
        return 0xFFFF;
    }
    return (bits[item / 32] & (1 << (item & 0x1F))) != 0;
}

/* 1 when the save has obtained an item (SaveData.itemBits); 0xFFFF for an id out of range. */
s32 Save_IsItemObtained(SaveData *save, u16 item)
{
    if (item == 0xFFFF || item >= ITEM_ID_COUNT) {
        return 0xFFFF;
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
extern u32 gStageFileTbls_0[];
extern u32 gStageFileTbls_1[];
extern u32 gStageFileTbls_2[];
extern u32 gStageFileTbls_3[];
extern char D_004A00E8[];
extern char gBladeNames_1[];
extern char D_004A0110[];
extern char D_004A0128[];
extern char D_004A0140[];
extern char D_004A0158[];
extern char D_004A0170[];
extern char D_004A0188[];
extern char D_004A01A0[];
extern char D_004A01C0[];
extern char D_004A01D8[];
extern char D_004A01E0[];
extern char D_004A01F0[];
extern char D_004A0200[];
extern char D_004A0210[];
extern char D_004A0220[];
extern char D_004A0230[];
extern char D_004A0240[];
extern char D_004A0250[];
extern char D_004A0260[];
extern char D_004A0270[];
extern char D_004A0280[];
extern char D_004A0290[];
extern char D_004A02A0[];
extern char D_004A02B0[];
extern char D_004A02D0[];
extern char D_004A02E8[];
extern char D_004A0300[];
extern char D_004A0318[];
extern char D_004A0330[];
extern char D_004A0348[];
extern char D_004A0350[];
extern char D_004A0360[];
extern char D_004A0370[];
extern char D_004A0390[];
extern char D_004A03B0[];
extern char D_004A03C8[];
extern char D_004A03D8[];
extern char D_004A03E8[];
extern char D_004A03F8[];
extern char D_004A0408[];
extern char D_004A0418[];
extern char D_004A0428[];
extern char D_004A0438[];
extern char D_004A0448[];
extern char D_004A0458[];
extern char D_004A0470[];
extern char D_004A0490[];
extern char D_004A04A8[];
extern char D_004A04B0[];
extern char D_004A04C0[];
extern char D_004A04D0[];
extern char D_004A04E0[];
extern char D_004A04F0[];
extern char D_004A0508[];
extern char D_004A0520[];
extern char D_004A0540[];
extern char D_004A0558[];
extern char D_004A0568[];
extern char D_004A0570[];
extern char D_004A0580[];
extern char D_004A05A0[];
extern char D_004A05C0[];
extern char D_004A05E0[];
extern char D_004A0600[];
extern char D_004A0620[];
extern char D_004A0640[];
extern char D_004A0660[];
extern char D_004A0680[];
extern char D_004A06A0[];
extern char D_004A06B8[];
extern char D_004A06C8[];
extern char D_004A06E0[];
extern char D_004A0700[];
extern char D_004A0718[];
extern char D_004A0730[];
extern char D_004A0748[];
extern char D_004A0760[];
extern char D_004A0778[];
extern char D_004A0790[];
extern char D_004A07B0[];
extern char D_004A07C8[];
extern char D_004A07E0[];
extern char D_004A0800[];
extern char D_004A0820[];
extern char D_004A0838[];
extern char D_004A0848[];
extern char D_004A0860[];
extern char D_004A0878[];
extern char D_004A0888[];
extern char D_004A08A0[];
extern char D_004A08C0[];
extern char D_004A08D8[];
extern char D_004A08F0[];
extern char D_004A0910[];
extern char D_004A0928[];
extern char D_004A0938[];
extern char D_004A0948[];
extern char D_004A0950[];
extern char D_004A0960[];
extern char D_004A0968[];
extern char D_004A0970[];
extern char D_004A0990[];
extern char D_004A09A8[];
extern char D_004A09C0[];
extern char D_004A09D8[];
extern char D_004A09E8[];
extern char D_004A0A00[];
extern char D_004A0A20[];
extern char D_004A0A38[];
extern char D_004A0A50[];
extern char D_004A0A68[];
extern char D_004A0A70[];
extern char D_004A0A80[];
extern char D_004A0A98[];
extern char D_004A0AA8[];
extern char D_004A0AB8[];
extern char D_004A0AD0[];
extern char D_004A0AE8[];
extern char D_004A0AF8[];
extern char D_004A0B10[];
extern char D_004A0B28[];
extern char D_004A0B38[];
extern char D_004A0B48[];
extern char D_004A0B58[];
extern char D_004A0B70[];
extern char D_004A0B90[];
extern char D_004A0BA8[];
extern char D_004A0BB8[];
extern char D_004A0BC8[];
extern char D_004A0BE0[];
extern char D_004A0BF8[];
extern char D_004A0C10[];
extern char D_004A0C28[];
extern char D_004A0C38[];
extern char D_004A0C48[];
extern char D_004A0C58[];
extern char D_004A0C70[];
extern char D_004A0C88[];
extern char D_004A0C98[];
extern char D_004A0CB0[];
extern char D_004A0CD0[];
extern char D_004A0CF0[];
extern char D_004A0D10[];
extern char D_004A0D28[];
extern char D_004A0D40[];
extern char D_004A0D60[];
extern char D_004A0D78[];
extern char D_004A0D90[];
extern char D_004A0DB0[];
extern char D_004A0DC8[];
extern char D_004A0DD8[];
extern char D_004A0DF0[];
extern char D_004A0E08[];
extern char D_004A0E18[];
extern char D_004A0E28[];
extern char D_004A0E38[];
extern char D_004A0E50[];
extern char D_004A0E70[];
extern char D_004A0E90[];
extern char D_004A0EB0[];
extern char D_004A0EC8[];
extern char D_004A0ED0[];
extern char D_004A0ED8[];
extern char D_004A0EE8[];
extern char D_004A0F00[];
extern char D_004A0F20[];
extern char D_004A0F40[];
extern char D_004A0F58[];
extern char D_004A0F68[];
extern char D_004A0F78[];
extern char D_004A0F90[];
extern char D_004A0FB0[];
extern char D_004A0FC8[];
extern char D_004A0FE0[];
extern char D_004A1000[];
extern char D_004A1020[];
extern char D_004A1040[];
extern char D_004A1058[];
extern char D_004A1068[];
extern char D_004A1078[];
extern char D_004A1088[];
extern char D_004A1098[];
extern char D_004A10A8[];
extern char D_004A10C0[];
extern char D_004A10D8[];
extern char D_004A10F0[];
extern char D_004A1110[];
extern char D_004A1128[];
extern char D_004A1130[];
extern char D_004A1150[];
extern char D_004A1168[];
extern char D_004A1178[];
extern char D_004A1190[];
extern char D_004A11A8[];
extern char D_004A11C0[];
extern char D_004A11E0[];
extern char D_004A11F8[];
extern char D_004A1208[];
extern char D_004A1218[];
extern char D_004A1230[];
extern char D_004A1248[];
extern char D_004A1250[];
extern char D_004A1260[];
extern char D_004A1278[];
extern char D_004A1288[];
extern char D_004A12A0[];
extern char D_004A12B8[];
extern char D_004A12C8[];
extern char D_004A12D0[];
extern char D_004A12E0[];
extern char D_004A1300[];
extern char D_004A1318[];
extern char D_004A1330[];
extern char D_004A1350[];
extern char D_004A1368[];
extern char D_004A1380[];
extern char D_004A13A0[];
extern char D_004A13C0[];
extern char D_004A13D8[];
extern char D_004A13E0[];
extern char D_004A13F0[];
extern char D_004A13F8[];
extern char D_004A1400[];
extern char D_004A1410[];
extern char D_004A1420[];
extern char D_004A1430[];
extern char D_004A1440[];
extern char D_004A1448[];
extern char D_004A1458[];
extern char D_004A1470[];
extern char D_004A1488[];
extern char D_004A14A0[];
extern char D_004A14B8[];
extern char D_004A14C8[];
extern char D_004A14E0[];
extern char D_004A14F8[];
extern char D_004A1508[];
extern char D_004A1518[];
extern char D_004A1530[];
extern char D_004A1550[];
extern char D_004A1570[];
extern char D_004A1588[];
extern char D_004A15A0[];
extern char D_004A15C0[];
extern char D_004A15D8[];
extern char D_004A15E8[];
extern char D_004A1600[];
extern char D_004A1618[];
extern char D_004A1630[];
extern char D_004A1648[];
extern char D_004A1658[];
extern char D_004A1670[];
extern char D_004A1690[];
extern char D_004A16A8[];
extern char D_004A16C0[];
extern char D_004A16E0[];
extern char D_004A16F8[];
extern char D_004A1700[];
extern char D_004A1710[];
extern char D_004A1730[];
extern char D_004A1748[];
extern char D_004A1758[];
extern char D_004A1770[];
extern char D_004A1790[];
extern char D_004A17B0[];
extern char D_004A17D0[];
extern char D_004A17F0[];
extern char D_004A1810[];
extern char D_004A1828[];
extern char D_004A1838[];
extern char D_004A1848[];
extern char D_004A1850[];
extern char D_004A1860[];
extern char D_004A1878[];
extern char D_004A1888[];
extern char D_004A1898[];
extern char D_004A18A8[];
extern char D_004A18B8[];
extern char D_004A18D0[];
extern char D_004A18E8[];
extern char D_004A1900[];
extern char D_004A1918[];
extern char D_004A1928[];
extern char D_004A1938[];
extern char D_004A1948[];
extern char D_004A1950[];
extern char D_004A1970[];
extern char D_004A1990[];
extern char D_004A19B0[];
extern char D_004A19D0[];
extern char D_004A19E8[];
extern char D_004A19F8[];
extern char D_004A1A10[];
extern char D_004A1A28[];
extern char D_004A1A38[];
extern char D_004A1A50[];
extern char D_004A1A70[];
extern char D_004A1A88[];
extern char D_004A1A90[];
extern char D_004A1AA0[];
extern char D_004A1AB0[];
extern char D_004A1AC0[];
extern char D_004A1AD8[];
extern char D_004A1AE0[];
extern char D_004A1AF0[];
extern char D_004A1B00[];
extern char D_004A1B10[];
extern char D_004A1B30[];
extern char D_004A1B50[];
extern char D_004A1B68[];
extern char D_004A1B78[];
extern char D_004A1B90[];
extern char D_004A1BB0[];
extern char D_004A1BC8[];
extern char D_004A1BD8[];
extern char D_004A1BE8[];
extern char D_004A1BF8[];
extern char D_004A1C08[];
extern char D_004A1C10[];
extern char D_004A1C20[];
extern char D_004A1C40[];
extern char D_004A1C58[];
extern char D_004A1C70[];
extern char D_004A1C88[];
extern char D_004A1C90[];
extern char D_004A1CA0[];
extern char D_004A1CB0[];
extern char D_004A1CD0[];
extern char D_004A1CE8[];
extern char D_004A1CF8[];
extern char D_004A1D08[];
extern char D_004A1D18[];
extern char D_004A1D28[];
extern char D_004A1D38[];
extern char D_004A1D48[];
extern char D_004A1D60[];
extern char D_004A1D80[];
extern char D_004A1D98[];
extern char D_004A1DB0[];
extern char D_004A1DD0[];
extern char D_004A1DF0[];
extern char D_004A1E08[];
extern char D_004A1E18[];
extern char D_004A1E20[];
extern char D_004A1E30[];
extern char D_004A1E38[];
extern char D_004A1E50[];
extern char D_004A1E70[];
extern char D_004A1E88[];
extern char D_004A1EA0[];
extern char D_004A1EC0[];
extern char D_004A1ED8[];
extern char D_004A1EE8[];
extern char D_004A1EF8[];
extern char D_004A1F10[];
extern char D_004A1F28[];
extern char D_004A1F38[];
extern char D_004A1F48[];
extern char D_004A1F60[];
extern char D_004A1F80[];
extern char D_004A1FA0[];
extern char D_004A1FC0[];
extern char D_004A1FD8[];
extern char D_004A1FF0[];
extern char D_004A2010[];
extern char D_004A2028[];
extern char D_004A2038[];
extern char D_004A2050[];
extern char D_004A2070[];
extern char D_004A2088[];
extern char D_004A2098[];
extern char D_004A20A0[];
extern char D_004A20C0[];
extern char D_004A20D8[];
extern char D_004A20F0[];
extern char D_004A2108[];
extern char D_004A2118[];
extern char D_004A2128[];
extern char D_004A2138[];
extern char D_004A2150[];
extern char D_004A2170[];
extern char D_004A2190[];
extern char D_004A21A8[];
extern char D_004A21B8[];
extern char D_004A21D0[];
extern char D_004A21F0[];
extern char D_004A2210[];
extern char D_004A2230[];
extern char D_004A2248[];
extern char D_004A2258[];
extern char D_004A2268[];
extern char D_004A2278[];
extern char D_004A2288[];
extern char D_004A22A0[];
extern char D_004A22C0[];
extern char D_004A22E0[];
extern char D_004A2300[];
extern char D_004A2318[];
extern char D_004A2330[];
extern char D_004A2348[];
extern char D_004A2360[];
extern char D_004A2380[];
extern char D_004A23A0[];
extern char D_004A23C0[];
extern char D_004A23E0[];
extern char D_004A23F8[];
extern char D_004A2408[];
extern char D_004A2420[];
extern char D_004A2440[];
extern char D_004A2460[];
extern char D_004A2478[];
extern char D_004A2488[];
extern char D_004A24A0[];
extern char D_004A24C0[];
extern char D_004A24D8[];
extern char D_004A24F0[];
extern char D_004A2510[];
extern char D_004A2530[];
extern char D_004A2550[];
extern char D_004A2570[];
extern char D_004A2590[];
extern char D_004A25B0[];
extern char D_004A25D0[];
extern char D_004A25F0[];
extern char D_004A2610[];
extern char D_004A2628[];
extern char D_004A2640[];
extern char D_004A2658[];
extern char D_004A2670[];
extern char D_004A2690[];
extern char D_004A26B0[];
extern char D_004A26C8[];
extern char D_004A26E0[];
extern char D_004A2700[];
extern char D_004A2720[];
extern char D_004A2740[];
extern char D_004A2760[];
extern char D_004A2780[];
extern char D_004A27A0[];
extern char D_004A27C0[];
extern char D_004A27D8[];
extern char D_004A27F0[];
extern char D_004A2808[];
extern char D_004A2820[];
extern char D_004A2838[];
extern char D_004A2848[];
extern char D_004A2860[];
extern char D_004A2878[];
extern char D_004A2888[];
extern char D_004A28A0[];
extern char D_004A28C0[];
extern char D_004A28E0[];
extern char D_004A28F8[];
extern char D_004A2908[];
extern char D_004A2920[];
extern char D_004A2940[];
extern char D_004A2958[];
extern char D_004A2968[];
extern char D_004A2978[];
extern char D_004A2988[];
extern char D_004A29A0[];
extern char D_004A29C0[];
extern char D_004A29D8[];
extern char D_004A29E0[];
extern char D_004A29F0[];
extern char D_004A2A10[];
extern char D_004A2A28[];
extern char D_004A2A40[];
extern char D_004A2A60[];
extern char D_004A2A80[];
extern char D_004A2A98[];
extern char D_004A2AA0[];
extern char D_004A2AB8[];
extern char D_004A2AC8[];
extern char D_004A2AE0[];
extern char D_004A2B00[];
extern char D_004A2B18[];
extern char D_004A2B28[];
extern char D_004A2B38[];
extern char D_004A2B40[];
extern char D_004A2B50[];
extern char D_004A2B70[];
extern char D_004A2B90[];
extern char D_004A2BA8[];
extern char D_004A2BB8[];
extern char D_004A2BC8[];
extern char D_004A2BE0[];
extern char D_004A2BF8[];
extern char D_004A2C10[];
extern char D_004A2C30[];
extern char D_004A2C50[];
extern char D_004A2C70[];
extern char D_004A2C90[];
extern char D_004A2CB0[];
extern char D_004A2CD0[];
extern char D_004A2CF0[];
extern char D_004A2D10[];
extern char D_004A2D28[];
extern char D_004A2D38[];
extern char D_004A2D48[];
extern char D_004A2D60[];
extern char D_004A2D80[];
extern char D_004A2D98[];
extern char D_004A2DB0[];
extern char D_004A2DC8[];
extern char D_004A2DD8[];
extern char D_004A2DF0[];
extern char D_004A2E08[];
extern char D_004A2E18[];
extern char D_004A2E20[];
extern char D_004A2E38[];
extern char D_004A2E50[];
extern char D_004A2E70[];
extern char D_004A2E90[];
extern char D_004A2EB0[];
extern char D_004A2ED0[];
extern char D_004A2EF0[];
extern char D_004A2F10[];
extern char D_004A2F30[];
extern char D_004A2F50[];
extern char D_004A2F70[];
extern char D_004A2F88[];
extern char D_004A2F98[];
extern char D_004A2FB0[];
extern char D_004A2FC8[];
extern char D_004A2FD8[];
extern char D_004A2FF0[];
extern char D_004A3010[];
extern char D_004A3030[];
extern char D_004A3050[];
extern char D_004A3070[];
extern char D_004A3088[];
extern char D_004A30A0[];
extern char D_004A30C0[];
extern char D_004A30E0[];
extern char D_004A3100[];
extern char D_004A3120[];
extern char D_004A3138[];
extern char D_004A3148[];
extern char D_004A3160[];
extern char D_004A3180[];
extern char D_004A3198[];
extern char D_004A31A0[];
extern char D_004A31C0[];
extern char D_004A31E0[];
extern char D_004A3200[];
extern char D_004A3218[];
extern char D_004A3228[];
extern char D_004A3240[];
extern char D_004A3260[];
extern char D_004A3280[];
extern char D_004A32A0[];
extern char D_004A32C0[];
extern char D_004A32D8[];
extern char D_004A32E8[];
extern char D_004A3300[];
extern char D_004A3318[];
extern char D_004A3328[];
extern char D_004A3338[];
extern char D_004A3348[];
extern char D_004A3360[];
extern u32 D_004A3378[];
extern u32 D_004A3388[];
extern u32 D_004A3398[];
extern char D_004A33B0[];
extern char D_004A33D0[];
extern char D_004A33F0[];
extern char D_004A3410[];
extern u32 D_004A3428[];
extern char D_004A3440[];
extern char D_004A3458[];
extern char D_004A3470[];
extern char D_004A3488[];
extern char D_004A34A0[];
extern char D_004A34C0[];
extern char D_004A34D8[];
extern char D_004A34E8[];
extern char D_004A3500[];
extern char D_004A3518[];
extern char D_004A3528[];
extern char D_004A3530[];
extern char D_004A3540[];
extern char D_004A3558[];
extern char D_004A3568[];
extern char D_004A3578[];
extern char D_004A3590[];
extern char D_004A35A8[];
extern char D_004A35C0[];
extern char D_004A35E0[];
extern char D_004A35F8[];
extern char D_004A3608[];
extern char D_004A3620[];
extern u32 D_004A3638[];
extern u32 D_004A3648[];
extern u32 D_004A3658[];
extern u32 D_004A3668[];
extern char D_004A3680[];
extern char D_004A36A0[];
extern char D_004A36C0[];
extern char D_004A36E0[];
extern u32 D_004A36F8[];
extern u32 D_004A3708[];
extern char D_004A3720[];
extern char D_004A3738[];
extern char D_004A3750[];
extern char D_004A3770[];
extern char D_004A3788[];
extern char D_004A3790[];
extern char D_004A37A0[];
extern char D_004A37B0[];
extern char D_004A37C0[];
extern char D_004A37D8[];
extern char D_004A37F0[];
extern char D_004A3808[];
extern char D_004A3818[];
extern char D_004A3830[];
extern char D_004A3848[];
extern char D_004A3858[];
extern char D_004A3868[];
extern char D_004A3880[];
extern char D_004A3898[];
extern char D_004A38A8[];
extern char D_004A38B8[];
extern char D_004A38C8[];
extern char D_004A38D8[];
extern char D_004A38E8[];
extern char D_004A38F8[];
extern char D_004A3910[];
extern char D_004A3928[];
extern char D_004A3940[];
extern char D_004A3958[];
extern char D_004A3970[];
extern char D_004A3988[];
extern char D_004A39A0[];
extern char D_004A39B8[];
extern char D_004A39D0[];
extern char D_004A39E8[];
extern char D_004A39F8[];
extern char D_004A3A08[];
extern char D_004A3A20[];
extern char D_004A3A38[];
extern char D_004A3A48[];
extern char D_004A3A58[];
extern char D_004A3A68[];
extern char D_004A3A80[];
extern char D_004A3A98[];
extern char D_004A3AA8[];
extern char D_004A3AC0[];
extern char D_004A3AE0[];
extern char D_004A3B00[];
extern char D_004A3B20[];
extern char D_004A3B38[];
extern char D_004A3B50[];
extern char D_004A3B68[];
extern char D_004A3B78[];
extern char D_004A3B90[];
extern char D_004A3BA8[];
extern char D_004A3BC0[];
extern char D_004A3BD8[];
extern char D_004A3BF0[];
extern char D_004A3C10[];
extern char D_004A3C30[];
extern char D_004A3C50[];
extern char D_004A3C70[];
extern char D_004A3C88[];
extern char D_004A3C98[];
extern char D_004A3CA8[];
extern char D_004A3CB8[];
extern char D_004A3CC8[];
extern char D_004A3CD8[];
extern char D_004A3CF0[];
extern char D_004A3D10[];
extern char D_004A3D28[];
extern char D_004A3D38[];
extern char D_004A3D48[];
extern char D_004A3D58[];
extern char D_004A3D70[];
extern char D_004A3D90[];
extern char D_004A3DA8[];
extern char D_004A3DB8[];
extern char D_004A3DC8[];
extern char D_004A3DD8[];
extern char D_004A3DE8[];
extern char D_004A3E00[];
extern char D_004A3E20[];
extern char D_004A3E40[];
extern char D_004A3E60[];
extern char D_004A3E80[];
extern char D_004A3E98[];
extern char D_004A3EB0[];
extern char D_004A3ED0[];
extern char D_004A3EF0[];
extern char D_004A3F08[];
extern char D_004A3F20[];
extern char D_004A3F38[];
extern char D_004A3F50[];
extern char D_004A3F68[];
extern char D_004A3F80[];
extern char D_004A3FA0[];
extern char D_004A3FC0[];
extern char D_004A3FE0[];
extern char D_004A4000[];
extern char D_004A4020[];
extern char D_004A4040[];
extern char D_004A4060[];
extern char D_004A4080[];
extern char D_004A40A0[];
extern char D_004A40C0[];
extern char D_004A40E0[];
extern char D_004A4100[];
extern char D_004A4120[];
extern char D_004A4140[];
extern char D_004A4160[];
extern char D_004A4180[];
extern char D_004A4198[];
extern char D_004A41B0[];
extern char D_004A41C8[];
extern char D_004A41E0[];
extern char D_004A4200[];
extern char D_004A4220[];
extern char D_004A4240[];
extern char D_004A4260[];
extern char D_004A4280[];
extern char D_004A42A0[];
extern char D_004A42B8[];
extern char D_004A42C8[];
extern char D_004A42E0[];
extern char D_004A4300[];
extern char D_004A4320[];
extern char D_004A4340[];
extern char D_004A4358[];
extern char D_004A4370[];
extern char D_004A4388[];
extern char D_004A4398[];
extern char D_004A43B0[];
extern char D_004A43D0[];
extern char D_004A43F0[];
extern char D_004A4410[];
extern char D_004A4430[];
extern char D_004A4450[];
extern char D_004A4470[];
extern char D_004A4490[];
extern char D_004A44B0[];
extern char D_004A44D0[];
extern char D_004A44E8[];
extern char D_004A4500[];
extern char D_004A4520[];
extern char D_004A4540[];
extern char D_004A4558[];
extern char D_004A4570[];
extern char D_004A4588[];
extern char D_004A45A0[];
extern char D_004A45B8[];
extern char D_004A45D0[];
extern char D_004A45F0[];
extern char D_004A4610[];
extern char D_004A4630[];
extern char D_004A4650[];
extern char D_004A4668[];
extern char D_004A4680[];
extern char D_004A46A0[];
extern char D_004A46C0[];
extern char D_004A46E0[];
extern char D_004A4700[];
extern char D_004A4720[];
extern char D_004A4740[];
extern char D_004A4758[];
extern char D_004A4770[];
extern char D_004A4790[];
extern char D_004A47B0[];
extern char D_004A47C8[];
extern char D_004A47E0[];
extern char D_004A47F8[];
extern char D_004A4810[];
extern char D_004A4828[];
extern char D_004A4840[];
extern char D_004A4860[];
extern char D_004A4880[];
extern char D_004A48A0[];
extern char D_004A48C0[];
extern char D_004A48E0[];
extern char D_004A4900[];
extern char D_004A4918[];
extern char D_004A4928[];
extern char D_004A4940[];
extern char D_004A4960[];
extern char D_004A4980[];
extern char D_004A49A0[];
extern char D_004A49B8[];
extern char D_004A49D0[];
extern char D_004A49E8[];
extern char D_004A49F8[];
extern char D_004A4A10[];
extern char D_004A4A30[];
extern char D_004A4A48[];
extern char D_004A4A58[];
extern char D_004A4A68[];
extern char D_004A4A80[];
extern char D_004A4A98[];
extern char D_004A4AB0[];
extern char D_004A4AC8[];
extern char D_004A4AD8[];
extern char D_004A4AE8[];
extern char D_004A4B00[];
extern char D_004A4B18[];
extern char D_004A4B30[];
extern char D_004A4B48[];
extern char D_004A4B60[];
extern char D_004A4B80[];
extern char D_004A4BA0[];
extern char D_004A4BC0[];
extern char D_004A4BE0[];
extern char D_004A4BF8[];
extern char D_004A4C08[];
extern char D_004A4C18[];
extern char D_004A4C30[];
extern char D_004A4C48[];
extern char D_004A4C60[];
extern char D_004A4C78[];
extern char D_004A4C88[];
extern char D_004A4C98[];
extern char D_004A4CA8[];
extern char D_004A4CB8[];
extern char D_004A4CC8[];
extern char D_004A4CE0[];
extern char D_004A4D00[];
extern char D_004A4D20[];
extern char D_004A4D40[];
extern char D_004A4D58[];
extern char D_004A4D68[];
extern char D_004A4D80[];
extern char D_004A4D98[];
extern char D_004A4DA8[];
extern char D_004A4DB8[];
extern char D_004A4DC8[];
extern char D_004A4DD8[];
extern char D_004A4DF0[];
extern char D_004A4E10[];
extern char D_004A4E30[];
extern char D_004A4E50[];
extern char D_004A4E70[];
extern char D_004A4E90[];
extern char D_004A4EB0[];
extern char D_004A4ED0[];
extern char D_004A4EF0[];
extern char D_004A4F08[];
extern char D_004A4F20[];
extern char D_004A4F40[];
extern char D_004A4F60[];
extern char D_004A4F80[];
extern char D_004A4FA0[];
extern char D_004A4FB8[];
extern char D_004A4FC8[];
extern char D_004A4FD8[];
extern char D_004A4FE8[];
extern char D_004A4FF8[];
extern char D_004A5008[];
extern char D_004A5018[];
extern char D_004A5028[];
extern char D_004A5038[];
extern char D_004A5048[];
extern char D_004A5058[];
extern char D_004A5070[];
extern char D_004A5088[];
extern char D_004A50A0[];
extern char D_004A50B8[];
extern char D_004A50C8[];
extern char D_004A50E0[];
extern char D_004A50F8[];
extern char D_004A5108[];
extern char D_004A5118[];
extern char D_004A5130[];
extern char D_004A5148[];
extern char D_004A5158[];
extern char D_004A5168[];
extern char D_004A5178[];
extern char D_004A5188[];
extern char D_004A5198[];
extern char D_004A51A8[];
extern char D_004A51C0[];
extern char D_004A51D8[];
extern char D_004A51F0[];
extern char D_004A5208[];
extern char D_004A5218[];
extern char D_004A5228[];
extern char D_004A5240[];
extern char D_004A5258[];
extern char D_004A5270[];
extern char D_004A5288[];
extern char D_004A52A0[];
extern char D_004A52C0[];
extern char D_004A52D8[];
extern char D_004A52E8[];
extern char D_004A52F8[];
extern char D_004A5310[];
extern char D_004A5328[];
extern char D_004A5338[];
extern char D_004A5348[];
extern char D_004A5358[];
extern char D_004A5370[];
extern char D_004A5390[];
extern char D_004A53A8[];
extern char D_004A53B8[];
extern char D_004A53D0[];
extern char D_004A53F0[];
extern char D_004A5410[];
extern char D_004A5430[];
extern char D_004A5448[];
extern char D_004A5460[];
extern char D_004A5478[];
extern char D_004A5488[];
extern char D_004A54A0[];
extern char D_004A54C0[];
extern char D_004A54D8[];
extern char D_004A54F0[];
extern char D_004A5510[];
extern char D_004A5530[];
extern char D_004A5548[];
extern char D_004A5558[];
extern char D_004A5570[];
extern char D_004A5590[];
extern char D_004A55A8[];
extern char D_004A55B8[];
extern char D_004A55C8[];
extern char D_004A55E0[];
extern char D_004A55F8[];
extern char D_004A5608[];
extern char D_004A5618[];
extern char D_004A5628[];
extern char D_004A5638[];
extern char D_004A5648[];
extern char D_004A5660[];
extern char D_004A5680[];
extern char D_004A56A0[];
extern char D_004A56C0[];
extern char D_004A56D8[];
extern char D_004A56F0[];
extern char D_004A5710[];
extern char D_004A5730[];
extern char D_004A5748[];
extern char D_004A5760[];
extern char D_004A5778[];
extern char D_004A5790[];
extern char D_004A57A8[];
extern char D_004A57C0[];
extern char D_004A57E0[];
extern char D_004A5800[];
extern char D_004A5820[];
extern char D_004A5840[];
extern char D_004A5858[];
extern char D_004A5870[];
extern char D_004A5890[];
extern char D_004A58B0[];
extern char D_004A58D0[];
extern char D_004A58F0[];
extern char D_004A5910[];
extern char D_004A5930[];
extern char D_004A5950[];
extern char D_004A5970[];
extern char D_004A5990[];
extern char D_004A59B0[];
extern char D_004A59D0[];
extern char D_004A59F0[];
extern char D_004A5A10[];
extern char D_004A5A28[];
extern char D_004A5A40[];
extern char D_004A5A58[];
extern char D_004A5A70[];
extern char D_004A5A90[];
extern char D_004A5AB0[];
extern char D_004A5AD0[];
extern char D_004A5AF0[];
extern char D_004A5B10[];
extern char D_004A5B30[];
extern char D_004A5B48[];
extern char D_004A5B58[];
extern char D_004A5B70[];
extern char D_004A5B90[];
extern char D_004A5BB0[];
extern char D_004A5BD0[];
extern char D_004A5BF0[];
extern char D_004A5C10[];
extern char D_004A5C30[];
extern char D_004A5C50[];
extern char D_004A5C70[];
extern char D_004A5C90[];
extern char D_004A5CB0[];
extern char D_004A5CD0[];
extern char D_004A5CF0[];
extern char D_004A5D10[];
extern char D_004A5D30[];
extern char D_004A5D50[];
extern char D_004A5D70[];
extern char D_004A5D90[];
extern char D_004A5DA8[];
extern char D_004A5DC0[];
extern char D_004A5DE0[];
extern char D_004A5E00[];
extern char D_004A5E18[];
extern char D_004A5E30[];
extern char D_004A5E48[];
extern char D_004A5E60[];
extern char D_004A5E78[];
extern char D_004A5E90[];
extern char D_004A5EB0[];
extern char D_004A5ED0[];
extern char D_004A5EF0[];
extern char D_004A5F10[];
extern char D_004A5F28[];
extern char D_004A5F40[];
extern char D_004A5F60[];
extern char D_004A5F80[];
extern char D_004A5FA0[];
extern char D_004A5FC0[];
extern char D_004A5FE0[];
extern char D_004A6000[];
extern char D_004A6020[];
extern char D_004A6038[];
extern char D_004A6050[];
extern char D_004A6070[];
extern char D_004A6090[];
extern char D_004A60A8[];
extern char D_004A60C0[];
extern char D_004A60D8[];
extern char D_004A60F0[];
extern char D_004A6108[];
extern char D_004A6120[];
extern char D_004A6140[];
extern char D_004A6160[];
extern char D_004A6180[];
extern char D_004A61A0[];
extern char D_004A61C0[];
extern char D_004A61E0[];
extern char D_004A61F8[];
extern char D_004A6208[];
extern char D_004A6220[];
extern char D_004A6240[];
extern char D_004A6260[];
extern char D_004A6280[];
extern char D_004A62A0[];
extern char D_004A62C0[];
extern char D_004A62E0[];
extern char D_004A6300[];
extern char D_004A6318[];
extern char D_004A6328[];
extern char D_004A6338[];
extern char D_004A6350[];
extern char D_004A6368[];
extern char D_004A6380[];
extern char D_004A6398[];
extern char D_004A63A8[];
extern char D_004A63B8[];
extern char D_004A63D0[];
extern char D_004A63E8[];
extern char D_004A6400[];
extern char D_004A6418[];
extern char D_004A6430[];
extern char D_004A6450[];
extern char D_004A6470[];
extern char D_004A6490[];
extern char D_004A64B0[];
extern char D_004A64C8[];
extern char D_004A64D8[];
extern char D_004A64E8[];
extern char D_004A6500[];
extern char D_004A6518[];
extern char D_004A6530[];
extern char D_004A6548[];
extern char D_004A6558[];
extern char D_004A6568[];
extern char D_004A6578[];
extern char D_004A6590[];
extern char D_004A65A8[];
extern char D_004A65C0[];
extern char D_004A65E0[];
extern char D_004A6600[];
extern char D_004A6620[];
extern char D_004A6638[];
extern char D_004A6648[];
extern char D_004A6658[];
extern char D_004A6670[];
extern char D_004A6688[];
extern char D_004A6698[];
extern char D_004A66A8[];
extern char D_004A66B8[];
extern char D_004A66D0[];
extern char D_004A66F0[];
extern char D_004A6710[];
extern char D_004A6730[];
extern char D_004A6750[];
extern char D_004A6770[];
extern char D_004A6790[];
extern char D_004A67B0[];
extern char D_004A67C8[];
extern char D_004A67E0[];
extern char D_004A67F8[];
extern char D_004A6810[];
extern char D_004A6830[];
extern char D_004A6850[];
extern char D_004A6868[];
extern char D_004A6878[];
extern char D_004A6890[];
extern char D_004A68B0[];
extern char D_004A68C8[];
extern char D_004A68D8[];
extern char D_004A68E8[];
extern char D_004A68F8[];
extern char D_004A6908[];
extern char D_004A6918[];
extern char D_004A6928[];
extern char D_004A6938[];
extern char D_004A6948[];
extern char D_004A6958[];
extern char D_004A6968[];
extern char D_004A6978[];
extern char D_004A6988[];
extern char D_004A6998[];
extern char D_004A69A8[];
extern char D_004A69B8[];
extern char D_004A69C8[];
extern char D_004A69D8[];
extern char D_004A69E8[];
extern char D_004A69F8[];
extern char D_004A6A08[];
extern char D_004A6A18[];
extern char D_004A6A28[];
extern char D_004A6A38[];
extern char D_004A6A48[];
extern char D_004A6A60[];
extern char D_004A6A78[];
extern char D_004A6A88[];
extern char D_004A6A98[];
extern char D_004A6AA8[];
extern char D_004A6AB8[];
extern char D_004A6AC8[];
extern char D_004A6AD8[];
extern char D_004A6AE8[];
extern char D_004A6B00[];
extern char D_004A6B20[];
extern char D_004A6B38[];
extern char D_004A6B48[];
extern char D_004A6B60[];
extern char D_004A6B80[];
extern char D_004A6BA0[];
extern char D_004A6BC0[];
extern char D_004A6BD8[];
extern char D_004A6BE8[];
extern char D_004A6BF8[];
extern char D_004A6C08[];
extern char D_004A6C20[];
extern char D_004A6C40[];
extern char D_004A6C60[];
extern char D_004A6C80[];
extern char D_004A6C98[];
extern char D_004A6CA8[];
extern char D_004A6CC0[];
extern char D_004A6CE0[];
extern char D_004A6CF8[];
extern char D_004A6D08[];
extern char D_004A6D18[];
extern char D_004A6D28[];
extern char D_004A6D38[];
extern char D_004A6D48[];
extern char D_004A6D58[];
extern char D_004A6D68[];
extern char D_004A6D78[];
extern char D_004A6D90[];
extern char D_004A6DB0[];
extern char D_004A6DD0[];
extern char D_004A6DF0[];
extern char D_004A6E10[];
extern char D_004A6E28[];
extern char D_004A6E38[];
extern char D_004A6E50[];
extern char D_004A6E70[];
extern char D_004A6E88[];
extern char D_004A6E98[];
extern char D_004A6EA8[];
extern char D_004A6EB8[];
extern char D_004A6EC8[];
extern char D_004A6ED8[];
extern char D_004A6EF0[];
extern char D_004A6F10[];
extern char D_004A6F30[];
extern char D_004A6F50[];
extern char D_004A6F68[];
extern char D_004A6F78[];
extern char D_004A6F90[];
extern char D_004A6FB0[];
extern char D_004A6FD0[];
extern char D_004A6FF0[];
extern char D_004A7010[];
extern char D_004A7030[];
extern char D_004A7050[];
extern char D_004A7070[];
extern char D_004A7090[];
extern char D_004A70B0[];
extern char D_004A70D0[];
extern char D_004A70F0[];
extern char D_004A7110[];
extern char D_004A7128[];
extern char D_004A7138[];
extern char D_004A7148[];
extern char D_004A7158[];
extern char D_004A7170[];
extern char D_004A7190[];
extern char D_004A71B0[];
extern char D_004A71D0[];
extern char D_004A71F0[];
extern char D_004A7210[];
extern char D_004A7228[];
extern char D_004A7238[];
extern char D_004A7250[];
extern char D_004A7270[];
extern char D_004A7290[];
extern char D_004A72B0[];
extern char D_004A72D0[];
extern char D_004A72F0[];
extern char D_004A7308[];
extern char D_004A7318[];
extern char D_004A7330[];
extern char D_004A7350[];
extern char D_004A7370[];
extern char D_004A7390[];
extern char D_004A73B0[];
extern char D_004A73D0[];
extern char D_004A73F0[];
extern char D_004A7410[];
extern char D_004A7430[];
extern char D_004A7450[];
extern char D_004A7468[];
extern char D_004A7478[];
extern char D_004A7490[];
extern char D_004A74B0[];
extern char D_004A74C8[];
extern char D_004A74D8[];
extern char D_004A74E8[];
extern char D_004A74F8[];
extern char D_004A7508[];
extern char D_004A7518[];
extern char D_004A7530[];
extern char D_004A7550[];
extern char D_004A7570[];
extern char D_004A7590[];
extern char D_004A75A8[];
extern char D_004A75B8[];
extern char D_004A75D0[];
extern char D_004A75F0[];
extern char D_004A7610[];
extern char D_004A7630[];
extern char D_004A7650[];
extern char D_004A7670[];
extern char D_004A7690[];
extern char D_004A76A8[];
extern char D_004A76B8[];
extern char D_004A76D0[];
extern char D_004A76F0[];
extern char D_004A7708[];
extern char D_004A7718[];
extern char D_004A7728[];
extern char D_004A7738[];
extern char D_004A7748[];
extern char D_004A7758[];
extern char D_004A7770[];
extern char D_004A7790[];
extern char D_004A77B0[];
extern char D_004A77D0[];
extern char D_004A77F0[];
extern char D_004A7810[];
extern char D_004A7828[];
extern char D_004A7838[];
extern char D_004A7850[];
extern char D_004A7870[];
extern char D_004A7890[];
extern char D_004A78B0[];
extern char D_004A78C8[];
extern char D_004A78D8[];
extern char D_004A78F0[];
extern char D_004A7910[];
extern char D_004A7928[];
extern char D_004A7938[];
extern char D_004A7948[];
extern char D_004A7958[];
extern char D_004A7968[];
extern char D_004A7978[];
extern char D_004A7988[];
extern char D_004A7998[];
extern char D_004A79A8[];
extern char D_004A79B8[];
extern char D_004A79C8[];
extern char D_004A79D8[];
extern char D_004A79E8[];
extern char D_004A79F8[];
extern char D_004A7A10[];
extern char D_004A7A30[];
extern char D_004A7A50[];
extern char D_004A7A70[];
extern char D_004A7A88[];
extern char D_004A7A98[];
extern char D_004A7AA8[];
extern char D_004A7AB8[];
extern char D_004A7AC8[];
extern char D_004A7AD8[];
extern char D_004A7AF0[];
extern char D_004A7B08[];
extern char D_004A7B20[];
extern char D_004A7B40[];
extern char D_004A7B60[];
extern char D_004A7B80[];
extern char D_004A7B98[];
extern char D_004A7BB0[];
extern char D_004A7BD0[];
extern char D_004A7BF0[];
extern char D_004A7C08[];
extern char D_004A7C18[];
extern char D_004A7C28[];
extern char D_004A7C38[];
extern char D_004A7C48[];
extern char D_004A7C58[];
extern char D_004A7C68[];
extern char D_004A7C78[];
extern char D_004A7C90[];
extern char D_004A7CB0[];
extern char D_004A7CD0[];
extern char D_004A7CF0[];
extern char D_004A7D10[];
extern char D_004A7D30[];
extern char D_004A7D50[];
extern char D_004A7D68[];
extern char D_004A7D80[];
extern char D_004A7DA0[];
extern char D_004A7DC0[];
extern char D_004A7DD8[];
extern char D_004A7DE8[];
extern char D_004A7DF8[];
extern char D_004A7E10[];
extern char D_004A7E30[];
extern char D_004A7E50[];
extern char D_004A7E68[];
extern char D_004A7E80[];
extern char D_004A7E98[];
extern char D_004A7EB0[];
extern char D_004A7EC8[];
extern char D_004A7EE0[];
extern char D_004A7EF8[];
extern char D_004A7F10[];
extern char D_004A7F28[];
extern char D_004A7F38[];
extern char D_004A7F50[];
extern char D_004A7F68[];
extern char D_004A7F80[];
extern char D_004A7F98[];
extern char D_004A7FB0[];
extern char D_004A7FC8[];
extern char D_004A7FE0[];
extern char D_004A7FF8[];
extern char D_004A8010[];
extern char D_004A8030[];
extern char D_004A8048[];
extern char D_004A8060[];
extern char D_004A8078[];
extern char D_004A8090[];
extern char D_004A80A8[];
extern char D_004A80C0[];
extern char D_004A80D8[];
extern char D_004A80F0[];
extern char D_004A8110[];
extern char D_004A8128[];
extern char D_004A8140[];
extern char D_004A8158[];
extern char D_004A8170[];
extern char D_004A8188[];
extern char D_004A81A0[];
extern char D_004A81B8[];
extern char D_004A81D0[];
extern char D_004A81F0[];
extern char D_004A8210[];
extern char D_004A8230[];
extern char D_004A8250[];
extern char D_004A8268[];
extern char D_004A8280[];
extern char D_004A8298[];
extern char D_004A82A8[];
extern char D_004A82C0[];
extern char D_004A82D8[];
extern char D_004A82E8[];
extern char D_004A8300[];
extern char D_004A8320[];
extern char D_004A8340[];
extern char D_004A8358[];
extern char D_004A8370[];
extern char D_004A8388[];
extern char D_004A8398[];
extern char D_004A83A8[];
extern char D_004A83B8[];
extern char D_004A83C8[];
extern char D_004A83D8[];
extern char D_004A83E8[];
extern char D_004A8400[];
extern char D_004A8418[];
extern char D_004A8430[];
extern char D_004A8450[];
extern char D_004A8470[];
extern char D_004A8490[];
extern char D_004A84B0[];
extern char D_004A84C8[];
extern char D_004A84E0[];
extern char D_004A8500[];
extern char D_004A8520[];
extern char D_004A8538[];
extern char D_004A8550[];
extern char D_004A8568[];
extern char D_004A8580[];
extern char D_004A8598[];
extern char D_004A85B0[];
extern char D_004A85D0[];
extern char D_004A85F0[];
extern char D_004A8610[];
extern char D_004A8630[];
extern char D_004A8648[];
extern char D_004A8660[];
extern char D_004A8680[];
extern char D_004A86A0[];
extern char D_004A86C0[];
extern char D_004A86E0[];
extern char D_004A8700[];
extern char D_004A8720[];
extern char D_004A8740[];
extern char D_004A8760[];
extern char D_004A8780[];
extern char D_004A87A0[];
extern char D_004A87C0[];
extern char D_004A87E0[];
extern char D_004A8800[];
extern char D_004A8818[];
extern char D_004A8830[];
extern char D_004A8850[];
extern char D_004A8870[];
extern char D_004A8890[];
extern char D_004A88B0[];
extern char D_004A88D0[];
extern char D_004A88E8[];
extern char D_004A88F8[];
extern char D_004A8910[];
extern char D_004A8930[];
extern char D_004A8950[];
extern char D_004A8970[];
extern char D_004A8990[];
extern char D_004A89B0[];
extern char D_004A89D0[];
extern char D_004A89F0[];
extern char D_004A8A10[];
extern char D_004A8A30[];
extern char D_004A8A50[];
extern char D_004A8A70[];
extern char D_004A8A90[];
extern char D_004A8AB0[];
extern char D_004A8AD0[];
extern char D_004A8AF0[];
extern char D_004A8B10[];
extern char D_004A8B30[];
extern char D_004A8B50[];
extern char D_004A8B68[];
extern char D_004A8B80[];
extern char D_004A8BA0[];
extern char D_004A8BC0[];
extern char D_004A8BD8[];
extern char D_004A8BF0[];
extern char D_004A8C08[];
extern char D_004A8C20[];
extern char D_004A8C38[];
extern char D_004A8C50[];
extern char D_004A8C70[];
extern char D_004A8C90[];
extern char D_004A8CB0[];
extern char D_004A8CD0[];
extern char D_004A8CE8[];
extern char D_004A8D00[];
extern char D_004A8D20[];
extern char D_004A8D40[];
extern char D_004A8D60[];
extern char D_004A8D80[];
extern char D_004A8DA0[];
extern char D_004A8DC0[];
extern char D_004A8DE0[];
extern char D_004A8DF8[];
extern char D_004A8E10[];
extern char D_004A8E30[];
extern char D_004A8E50[];
extern char D_004A8E68[];
extern char D_004A8E80[];
extern char D_004A8E98[];
extern char D_004A8EB0[];
extern char D_004A8ED0[];
extern char D_004A8EF0[];
extern char D_004A8F10[];
extern char D_004A8F30[];
extern char D_004A8F50[];
extern char D_004A8F68[];
extern char D_004A8F78[];
extern char D_004A8F90[];
extern char D_004A8FB0[];
extern char D_004A8FD0[];
extern char D_004A8FF0[];
extern char D_004A9010[];
extern char D_004A9030[];
extern char D_004A9050[];
extern char D_004A9070[];
extern char D_004A9090[];
extern char D_004A90A8[];
extern char D_004A90C0[];
extern char D_004A90D8[];
extern char D_004A90F0[];
extern char D_004A9108[];
extern char D_004A9120[];
extern char D_004A9138[];
extern char D_004A9150[];
extern char D_004A9168[];
extern char D_004A9180[];
extern char D_004A9198[];
extern char D_004A91B0[];
extern char D_004A91C8[];
extern char D_004A91E0[];
extern char D_004A9200[];
extern char D_004A9220[];
extern char D_004A9240[];
extern char D_004A9260[];
extern char D_004A9278[];
extern char D_004A9290[];
extern char D_004A92A8[];
extern char D_004A92C0[];
extern char D_004A92D8[];
extern char D_004A92E8[];
extern char D_004A92F8[];
extern char D_004A9308[];
extern char D_004A9320[];
extern char D_004A9338[];
extern char D_004A9348[];
extern char D_004A9358[];
extern char D_004A9370[];
extern char D_004A9390[];
extern char D_004A93B0[];
extern char D_004A93C8[];
extern char D_004A93D8[];
extern char D_004A93F0[];
extern char D_004A9408[];
extern char D_004A9418[];
extern char D_004A9428[];
extern char D_004A9438[];
extern char D_004A9448[];
extern char D_004A9460[];
extern char D_004A9480[];
extern char D_004A94A0[];
extern char D_004A94B8[];
extern char D_004A94D0[];
extern char D_004A94F0[];
extern char D_004A9510[];
extern char D_004A9530[];
extern char D_004A9548[];
extern char D_004A9560[];
extern char D_004A9580[];
extern char D_004A95A0[];
extern char D_004A95C0[];
extern char D_004A95E0[];
extern char D_004A9600[];
extern char D_004A9618[];
extern char D_004A9628[];
extern char D_004A9640[];
extern char D_004A9660[];
extern char D_004A9680[];
extern char D_004A96A0[];
extern char D_004A96B8[];
extern char D_004A96C8[];
extern char D_004A96E0[];
extern char D_004A9700[];
extern char D_004A9718[];
extern char D_004A9728[];
extern char D_004A9740[];
extern char D_004A9758[];
extern char D_004A9768[];
extern char D_004A9778[];
extern char D_004A9788[];
extern char D_004A97A0[];
extern char D_004A97C0[];
extern char D_004A97E0[];
extern char D_004A9800[];
extern char D_004A9820[];
extern char D_004A9840[];
extern char D_004A9860[];
extern char D_004A9880[];
extern char D_004A98A0[];
extern char D_004A98C0[];
extern char D_004A98E0[];
extern char D_004A9900[];
extern char D_004A9920[];
extern char D_004A9938[];
extern char D_004A9948[];
extern char D_004A9960[];
extern char D_004A9980[];
extern char D_004A9998[];
extern char D_004A99A8[];
extern char D_004A99B8[];
extern char D_004A99C8[];
extern char D_004A99D8[];
extern char D_004A99E8[];
extern char D_004A9A00[];
extern char D_004A9A20[];
extern char D_004A9A40[];
extern char D_004A9A60[];
extern char D_004A9A80[];
extern char D_004A9AA0[];
extern char D_004A9AC0[];
extern char D_004A9AD8[];
extern char D_004A9AF0[];
extern char D_004A9B10[];
extern char D_004A9B30[];
extern char D_004A9B48[];
extern char D_004A9B58[];
extern char D_004A9B70[];
extern char D_004A9B90[];
extern char D_004A9BB0[];
extern char D_004A9BD0[];
extern char D_004A9BF0[];
extern char D_004A9C10[];
extern char D_004A9C28[];
extern char D_004A9C38[];
extern char D_004A9C48[];
extern char D_004A9C58[];
extern char D_004A9C68[];
extern char D_004A9C78[];
extern char D_004A9C88[];
extern char D_004A9C98[];
extern char D_004A9CB0[];
extern char D_004A9CD0[];
extern char D_004A9CF0[];
extern char D_004A9D10[];
extern char D_004A9D30[];
extern char D_004A9D50[];
extern char D_004A9D70[];
extern char D_004A9D90[];
extern char D_004A9DB0[];
extern char D_004A9DD0[];
extern char D_004A9DF0[];
extern char D_004A9E10[];
extern char D_004A9E30[];
extern char D_004A9E50[];
extern char D_004A9E70[];
extern char D_004A9E90[];
extern char D_004A9EB0[];
extern char D_004A9ED0[];
extern char D_004A9EF0[];
extern char D_004A9F10[];
extern char D_004A9F30[];
extern char D_004A9F50[];
extern char D_004A9F70[];
extern char D_004A9F90[];
extern char D_004A9FB0[];
extern char D_004A9FD0[];
extern char D_004A9FF0[];
extern char D_004AA010[];
extern char D_004AA030[];
extern char D_004AA050[];
extern char D_004AA070[];
extern char D_004AA090[];
extern char D_004AA0B0[];
extern char D_004AA0D0[];
extern char D_004AA0F0[];
extern char D_004AA110[];
extern char D_004AA130[];
extern char D_004AA150[];
extern char D_004AA170[];
extern char D_004AA190[];
extern char D_004AA1A8[];
extern char D_004AA1B8[];
extern char D_004AA1D0[];
extern char D_004AA1F0[];
extern char D_004AA210[];
extern char D_004AA230[];
extern char D_004AA250[];
extern char D_004AA270[];
extern char D_004AA288[];
extern char D_004AA298[];
extern char D_004AA2B0[];
extern char D_004AA2D0[];
extern char D_004AA2F0[];
extern char D_004AA310[];
extern char D_004AA330[];
extern char D_004AA350[];
extern char D_004AA370[];
extern char D_004AA390[];
extern char D_004AA3B0[];
extern char D_004AA3D0[];
extern char D_004AA3F0[];
extern char D_004AA410[];
extern char D_004AA430[];
extern char D_004AA450[];
extern char D_004AA470[];
extern char D_004AA490[];
extern char D_004AA4B0[];
extern char D_004AA4D0[];
extern char D_004AA4F0[];
extern char D_004AA510[];
extern char D_004AA530[];
extern char D_004AA550[];
extern char D_004AA570[];
extern char D_004AA590[];
extern char D_004AA5B0[];
extern char D_004AA5D0[];
extern char D_004AA5F0[];
extern char D_004AA610[];
extern char D_004AA630[];
extern char D_004AA650[];
extern char D_004AA670[];
extern char D_004AA690[];
extern char D_004AA6B0[];
extern char D_004AA6D0[];
extern char D_004AA6F0[];
extern char D_004AA710[];
extern char D_004AA730[];
extern char D_004AA750[];
extern char D_004AA770[];
extern char D_004AA790[];
extern char D_004AA7B0[];
extern char D_004AA7D0[];
extern char D_004AA7F0[];
extern char D_004AA810[];
extern char D_004AA830[];
extern char D_004AA850[];
extern char D_004AA870[];
extern char D_004AA890[];
extern char D_004AA8A8[];
extern char D_004AA8B8[];
extern char D_004AA8D0[];
extern char D_004AA8F0[];
extern char D_004AA910[];
extern char D_004AA930[];
extern char D_004AA948[];
extern char D_004AA958[];
extern char D_004AA970[];
extern char D_004AA990[];
extern char D_004AA9A8[];
extern char D_004AA9B8[];
extern char D_004AA9D0[];
extern char D_004AA9F0[];
extern char D_004AAA10[];
extern char D_004AAA30[];
extern char D_004AAA48[];
extern char D_004AAA58[];
extern char D_004AAA70[];
extern char D_004AAA90[];
extern char D_004AAAB0[];
extern char D_004AAAD0[];
extern char D_004AAAF0[];
extern char D_004AAB10[];
extern char D_004AAB30[];
extern char D_004AAB50[];
extern char D_004AAB70[];
extern char D_004AAB90[];
extern char D_004AABA8[];
extern char D_004AABB8[];
extern char D_004AABD0[];
extern char D_004AABF0[];
extern char D_004AAC10[];
extern char D_004AAC30[];
extern char D_004AAC48[];
extern char D_004AAC58[];
extern char D_004AAC70[];
extern char D_004AAC88[];
extern char D_004AAC98[];
extern char D_004AACA8[];
extern char D_004AACB8[];
extern char D_004AACD0[];
extern char D_004AACF0[];
extern char D_004AAD10[];
extern char D_004AAD28[];
extern char D_004AAD38[];
extern char D_004AAD50[];
extern char D_004AAD70[];
extern char D_004AAD88[];
extern char D_004AAD98[];
extern char D_004AADA8[];
extern char D_004AADB8[];
extern char D_004AADD0[];
extern char D_004AADF0[];
extern char D_004AAE10[];
extern char D_004AAE30[];
extern char D_004AAE50[];
extern char D_004AAE70[];
extern char D_004AAE90[];
extern char D_004AAEB0[];
extern char D_004AAED0[];
extern char D_004AAEF0[];
extern char D_004AAF10[];
extern char D_004AAF30[];
extern char D_004AAF50[];
extern char D_004AAF70[];
extern char D_004AAF90[];
extern char D_004AAFB0[];
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
extern ItemData gItemData[];
u32 gStageFileTbls_4[6] = {
#include "sub_main/0044BF30.inc"
};
u32 gStageFileTbls_5[2] = {
#include "sub_main/0044BF48.inc"
};
u32 gStageFileTbls_6[6] = {
#include "sub_main/0044BF50.inc"
};
u32 gStageFileTbls_7[3] = {
#include "sub_main/0044BF68.inc"
};
u32 gStageFileTbls_8[36] = {
#include "sub_main/0044BF80.inc"
};
u32 gStageFileTbls_9[44] = {
#include "sub_main/0044C010.inc"
};
u32 gStageFileTbls_10[16] = {
#include "sub_main/0044C0C0.inc"
};
u32 gStageFileTbls_11[6] = {
#include "sub_main/0044C100.inc"
};
u32 gStageFileTbls_12[2] = {
#include "sub_main/0044C118.inc"
};
u32 gStageFileTbls_13[28] = {
#include "sub_main/0044C120.inc"
};
u32 gStageFileTbls[16] = {
    (u32)gStageFileTbls_0, (u32)gStageFileTbls_1, (u32)gStageFileTbls_2, (u32)gStageFileTbls_3,
    (u32)gStageFileTbls_4, (u32)gStageFileTbls_5, (u32)gStageFileTbls_6, (u32)gStageFileTbls_7,
    (u32)gStageFileTbls_8, (u32)gStageFileTbls_9, (u32)gStageFileTbls_10, (u32)gStageFileTbls_11,
    (u32)gStageFileTbls_12, (u32)gStageFileTbls_13, 0, 0,
};
u32 gMonsterFlagBits[28] = {
#include "sub_main/0044C1D0.inc"
};
u32 gPartBreakEffects[24] = {
#include "sub_main/0044C240.inc"
};
u32 gResistSongEffects[4] = {
#include "sub_main/0044C2A0.inc"
};
/* gGestureStates holds two tables: its own bytes (0x1C), then the hunter rank points, read
 * as gHrRankPoints (an alias at +0x1C). Defined as two arrays, they no longer match: sub_main
 * grows by 16 bytes. */
u32 gGestureStates[60] = {
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
GunData gGunData[62] = {
#include "sub_main/0044FA30.inc"
};
u32 D_00450000[64] = {
#include "sub_main/00450000.inc"
};
u32 D_00450100[128] = {
#include "sub_main/00450100.inc"
};
u32 D_00450300[204] = {
#include "sub_main/00450300.inc"
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
    D_004A00E8, gBladeNames_1, D_004A0110, D_004A0128,
    D_004A0140, D_004A0158, D_004A0170, D_004A0188,
    D_004A01A0, D_004A01C0, D_004A01D8, D_004A01E0,
    D_004A01F0, D_004A0200, D_004A0210, D_004A0220,
    D_004A0230, D_004A0240, D_004A0250, D_004A0260,
    D_004A0270, D_004A0280, D_004A0290, D_004A02A0,
    D_004A02B0, D_004A02D0, D_004A02E8, D_004A0300,
    D_004A0318, D_004A0330, D_004A0348, D_004A0350,
    D_004A0360, D_004A0370, D_004A0390, D_004A03B0,
    D_004A03C8, D_004A03D8, D_004A03E8, D_004A03F8,
    D_004A0408, D_004A0418, D_004A0428, D_004A0438,
    D_004A0448, D_004A0458, D_004A0470, D_004A0490,
    D_004A04A8, D_004A04B0, D_004A04C0, D_004A04D0,
    D_004A04E0, D_004A04F0, D_004A0508, D_004A0520,
    D_004A0540, D_004A0558, D_004A0568, D_004A0570,
    D_004A0580, D_004A05A0, D_004A05C0, D_004A05E0,
    D_004A0600, D_004A0620, D_004A0640, D_004A0660,
    D_004A0680, D_004A06A0, D_004A06B8, D_004A06C8,
    D_004A06E0, D_004A0700, D_004A0718, D_004A0730,
    D_004A0748, D_004A0760, D_004A0778, D_004A0790,
    D_004A07B0, D_004A07C8, D_004A07E0, D_004A0800,
    D_004A0820, D_004A0838, D_004A0848, D_004A0860,
    D_004A0878, D_004A0888, D_004A08A0, D_004A08C0,
    D_004A08D8, D_004A08F0, D_004A0910, D_004A0928,
    D_004A0938, D_004A0948, D_004A0950, D_004A0960,
    D_004A0968, D_004A0970, D_004A0990, D_004A09A8,
    D_004A09C0, D_004A09D8, D_004A09E8, D_004A0A00,
    D_004A0A20, D_004A0A38, D_004A0A50, D_004A0A68,
    D_004A0A70, D_004A0A80, D_004A0A98, D_004A0AA8,
    D_004A0AB8, D_004A0AD0, D_004A0AE8, D_004A0AF8,
    D_004A0B10, D_004A0B28, D_004A0B38, D_004A0B48,
    D_004A0B58, D_004A0B70, D_004A0B90, D_004A0BA8,
    D_004A0BB8, D_004A0BC8, D_004A0BE0, D_004A0BF8,
    D_004A0C10, D_004A0C28, D_004A0C38, D_004A0C48,
    D_004A0C58, D_004A0C70, D_004A0C88, D_004A0C98,
    D_004A0CB0, D_004A0CD0, D_004A0CF0, D_004A0D10,
    D_004A0D28, D_004A0D40, D_004A0D60, D_004A0D78,
    D_004A0D90, D_004A0DB0, D_004A0DC8, D_004A0DD8,
    D_004A0DF0, D_004A0E08, D_004A0E18, D_004A0E28,
    D_004A0E38, D_004A0E50, D_004A0E70, D_004A0E90,
    D_004A0EB0, D_004A0EC8, D_004A0ED0, D_004A0ED8,
    D_004A0EE8, D_004A0F00, D_004A0F20, D_004A0F40,
    D_004A0F58, D_004A0F68, D_004A0F78, D_004A0F90,
    D_004A0FB0, D_004A0FC8, D_004A0FE0, D_004A1000,
    D_004A1020, D_004A1040, D_004A1058, D_004A1068,
    D_004A1078, D_004A1088, D_004A1098, D_004A10A8,
    D_004A10C0, D_004A10D8, D_004A10F0, D_004A1110,
    D_004A1128, D_004A1130, D_004A1150, D_004A1168,
    D_004A1178, D_004A1190, D_004A11A8, D_004A11C0,
    D_004A11E0, D_004A11F8, D_004A1208, D_004A1218,
    D_004A1230, D_004A1248, D_004A1250, D_004A1260,
    D_004A1278, D_004A1288, D_004A12A0, D_004A12B8,
    D_004A12C8, D_004A12D0, D_004A12E0, D_004A1300,
    D_004A1318, D_004A1330, D_004A1350, D_004A1368,
    D_004A1380, D_004A13A0, D_004A13C0, D_004A13D8,
    D_004A13E0, D_004A13F0, D_004A13F8, D_004A1400,
    D_004A1410, D_004A1420, D_004A1430, D_004A1440,
    D_004A1448, D_004A1458, D_004A1470, D_004A1488,
    D_004A14A0, D_004A14B8, D_004A14C8, D_004A14E0,
    D_004A14F8, D_004A1508, D_004A1518, D_004A1530,
    D_004A1550, D_004A1570, D_004A1588, D_004A15A0,
    D_004A15C0, D_004A15D8, D_004A15E8, D_004A1600,
    D_004A1618, D_004A1630, D_004A1648, D_004A1658,
    D_004A1670, D_004A1690, D_004A16A8, D_004A16C0,
    D_004A16E0, D_004A16F8, D_004A1700, D_004A1710,
    D_004A1730, D_004A1748, D_004A1758, D_004A1770,
    D_004A1790, D_004A17B0, D_004A17D0, D_004A17F0,
    D_004A1810, D_004A1828, D_004A1838, D_004A1848,
    D_004A1850, D_004A1860, D_004A1878, D_004A1888,
    D_004A1898, D_004A18A8, D_004A18B8, D_004A18D0,
    D_004A18E8, D_004A1900, D_004A1918, D_004A1928,
    D_004A1938, D_004A1948, D_004A1950, D_004A1970,
    D_004A1990, D_004A19B0, D_004A19D0, D_004A19E8,
    D_004A19F8, D_004A1A10, D_004A1A28, D_004A1A38,
    D_004A1A50, D_004A1A70, D_004A1A88, D_004A1A90,
    D_004A1AA0, D_004A1AB0, D_004A1AC0, D_004A1AD8,
    D_004A1AE0, D_004A1AF0, D_004A1B00, D_004A1B10,
    D_004A1B30, D_004A1B50, D_004A1B68, D_004A1B78,
    D_004A1B90, D_004A1BB0, D_004A1BC8, D_004A1BD8,
    D_004A1BE8, D_004A1BF8, D_004A1C08, D_004A1C10,
    D_004A1C20, D_004A1C40, D_004A1C58, D_004A1C70,
    D_004A1C88, D_004A1C90, D_004A1CA0, D_004A1CB0,
    D_004A1CD0, D_004A1CE8, D_004A1CF8, D_004A1D08,
    D_004A1D18, D_004A1D28, D_004A1D38, D_004A1D48,
    D_004A1D60, D_004A1D80, D_004A1D98, D_004A1DB0,
    D_004A1DD0, D_004A1DF0, D_004A1E08, D_004A1E18,
    D_004A1E20, D_004A1E30, D_004A1E38, D_004A1E50,
    D_004A1E70, D_004A1E88, D_004A1EA0, D_004A1EC0,
    D_004A1ED8, D_004A1EE8, D_004A1EF8, D_004A1F10,
    D_004A1F28, D_004A1F38, D_004A1F48, D_004A1F60,
    D_004A1F80, D_004A1FA0, D_004A1FC0, D_004A1FD8,
    D_004A1FF0, D_004A2010, D_004A2028, D_004A2038,
    D_004A2050, D_004A2070, D_004A2088, D_004A2098,
    D_004A20A0, D_004A20C0, D_004A20D8, D_004A20F0,
    D_004A2108, D_004A2118, D_004A2128, D_004A2138,
    D_004A2150, D_004A2170, D_004A2190, D_004A21A8,
    D_004A21B8, D_004A21D0, D_004A21F0, D_004A2210,
    D_004A2230, D_004A2248, D_004A2258, D_004A2268,
    D_004A2278, D_004A2288, D_004A22A0, D_004A22C0,
    D_004A22E0, D_004A2300, D_004A2318, D_004A2330,
    D_004A2348, D_004A2360, D_004A2380, D_004A23A0,
    D_004A23C0, D_004A23E0, D_004A23F8, D_004A2408,
    D_004A2420, D_004A2440, D_004A2460, D_004A2478,
    D_004A2488, D_004A24A0, D_004A24C0, D_004A24D8,
    D_004A24F0, D_004A2510, D_004A2530, D_004A2550,
    D_004A2570, D_004A2590, D_004A25B0, D_004A25D0,
    D_004A25F0, D_004A2610, D_004A2628, D_004A2640,
    D_004A2658, D_004A2670, D_004A2690, D_004A26B0,
    D_004A26C8, D_004A26E0, D_004A2700, D_004A2720,
    D_004A2740, D_004A2760, D_004A2780, D_004A27A0,
    D_004A27C0, D_004A27D8, D_004A27F0, D_004A2808,
    D_004A2820, D_004A2838, D_004A2848, D_004A2860,
    D_004A2878, D_004A2888, D_004A28A0, D_004A28C0,
    D_004A28E0, D_004A28F8, D_004A2908, D_004A2920,
    D_004A2940, D_004A2958, D_004A2968, D_004A2978,
    D_004A2988, D_004A29A0, D_004A29C0, D_004A29D8,
    D_004A29E0, D_004A29F0, D_004A2A10, D_004A2A28,
    D_004A2A40, D_004A2A60, D_004A2A80, D_004A2A98,
    D_004A2AA0, D_004A2AB8, D_004A2AC8, D_004A2AE0,
    D_004A2B00, D_004A2B18, D_004A2B28, D_004A2B38,
    D_004A2B40, D_004A2B50, D_004A2B70, D_004A2B90,
    D_004A2BA8, D_004A2BB8, 0, 0,
};
char *gGunNames[128] = {
    D_004A00E8, D_004A2BC8, D_004A2BE0, D_004A2BF8,
    D_004A2C10, D_004A2C30, D_004A2C50, D_004A2C70,
    D_004A2C90, D_004A2CB0, D_004A2CD0, D_004A2CF0,
    D_004A2D10, D_004A2D28, D_004A2D38, D_004A2D48,
    D_004A2D60, D_004A2D80, D_004A2D98, D_004A2DB0,
    D_004A2DC8, D_004A2DD8, D_004A2DF0, D_004A2E08,
    D_004A2E18, D_004A2E20, D_004A2E38, D_004A2E50,
    D_004A2E70, D_004A2E90, D_004A2EB0, D_004A2ED0,
    D_004A2EF0, D_004A2F10, D_004A2F30, D_004A2F50,
    D_004A2F70, D_004A2F88, D_004A2F98, D_004A2FB0,
    D_004A2FC8, D_004A2FD8, D_004A2FF0, D_004A3010,
    D_004A3030, D_004A3050, D_004A3070, D_004A3088,
    D_004A30A0, D_004A30C0, D_004A30E0, D_004A3100,
    D_004A3120, D_004A3138, D_004A3148, D_004A3160,
    D_004A3180, D_004A3198, D_004A31A0, D_004A31C0,
    D_004A31E0, D_004A3200, D_004A3218, D_004A3228,
    D_004A3240, D_004A3260, D_004A3280, D_004A32A0,
    D_004A32C0, D_004A32D8, D_004A32E8, D_004A3300,
    D_004A3318, D_004A3328, D_004A3338, D_004A3348,
    D_004A3360, (char *)D_004A3378, (char *)D_004A3388, (char *)D_004A3398,
    D_004A33B0, D_004A33D0, D_004A33F0, D_004A3410,
    (char *)D_004A3428, D_004A3440, D_004A3458, D_004A3470,
    D_004A3488, D_004A34A0, D_004A34C0, D_004A34D8,
    D_004A34E8, D_004A3500, D_004A3518, D_004A3528,
    D_004A3530, D_004A3540, D_004A3558, D_004A3568,
    D_004A3578, D_004A3590, D_004A35A8, D_004A35C0,
    D_004A35E0, D_004A35F8, D_004A3608, D_004A3620,
    (char *)D_004A3638, (char *)D_004A3648, (char *)D_004A3658, (char *)D_004A3668,
    D_004A3680, D_004A36A0, D_004A36C0, D_004A36E0,
    (char *)D_004A36F8, (char *)D_004A3708, D_004A3720, D_004A3738,
    D_004A3750, D_004A3770, D_004A3788, D_004A3790,
    D_004A37A0, D_004A37B0, D_004A37C0, D_004A37D8,
};
char *gArmorHeadNames[256] = {
    D_004A00E8, D_004A37F0, D_004A3808, D_004A3818,
    D_004A3830, D_004A3848, D_004A3858, D_004A3868,
    D_004A3880, D_004A3898, D_004A38A8, D_004A38B8,
    D_004A38C8, D_004A38D8, D_004A38E8, D_004A38F8,
    D_004A3910, D_004A3928, D_004A3940, D_004A3958,
    D_004A3970, D_004A3988, D_004A39A0, D_004A39B8,
    D_004A39D0, D_004A39E8, D_004A39F8, D_004A3A08,
    D_004A3A20, D_004A3A38, D_004A3A48, D_004A3A58,
    D_004A3A68, D_004A3A80, D_004A3A98, D_004A3AA8,
    D_004A3AC0, D_004A3AE0, D_004A3B00, D_004A3B20,
    D_004A3B38, D_004A3B50, D_004A3B68, D_004A3B78,
    D_004A3B90, D_004A3BA8, D_004A3BC0, D_004A3BD8,
    D_004A3BF0, D_004A3C10, D_004A3C30, D_004A3C50,
    D_004A3C70, D_004A3C88, D_004A3C98, D_004A3CA8,
    D_004A3CB8, D_004A3CC8, D_004A3CD8, D_004A3CF0,
    D_004A3D10, D_004A3D28, D_004A3D38, D_004A3D48,
    D_004A3D58, D_004A3D70, D_004A3D90, D_004A3DA8,
    D_004A3DB8, D_004A3DC8, D_004A3DD8, D_004A3DE8,
    D_004A3E00, D_004A3E20, D_004A3E40, D_004A3E60,
    D_004A3E80, D_004A3E98, D_004A3EB0, D_004A3ED0,
    D_004A3EF0, D_004A3F08, D_004A3F20, D_004A3F38,
    D_004A3F50, D_004A3F68, D_004A3F80, D_004A3FA0,
    D_004A3FC0, D_004A3FE0, D_004A4000, D_004A4020,
    D_004A4040, D_004A4060, D_004A4080, D_004A40A0,
    D_004A40C0, D_004A40E0, D_004A4100, D_004A4120,
    D_004A4140, D_004A4160, D_004A4180, D_004A4198,
    D_004A41B0, D_004A41C8, D_004A41E0, D_004A4200,
    D_004A4220, D_004A4240, D_004A4260, D_004A4280,
    D_004A42A0, D_004A42B8, D_004A42C8, D_004A42E0,
    D_004A4300, D_004A4320, D_004A4340, D_004A4358,
    D_004A4370, D_004A4388, D_004A4398, D_004A43B0,
    D_004A43D0, D_004A43F0, D_004A4410, D_004A4430,
    D_004A4450, D_004A4470, D_004A4490, D_004A44B0,
    D_004A44D0, D_004A44E8, D_004A4500, D_004A4520,
    D_004A4540, D_004A4558, D_004A4570, D_004A4588,
    D_004A45A0, D_004A45B8, D_004A45D0, D_004A45F0,
    D_004A4610, D_004A4630, D_004A4650, D_004A4668,
    D_004A4680, D_004A46A0, D_004A46C0, D_004A46E0,
    D_004A4700, D_004A4720, D_004A4740, D_004A4758,
    D_004A4770, D_004A4790, D_004A47B0, D_004A47C8,
    D_004A47E0, D_004A47F8, D_004A4810, D_004A4828,
    D_004A4840, D_004A4860, D_004A4880, D_004A48A0,
    D_004A48C0, D_004A48E0, D_004A4900, D_004A4918,
    D_004A4928, D_004A4940, D_004A4960, D_004A4980,
    D_004A49A0, D_004A49B8, D_004A49D0, D_004A49E8,
    D_004A49F8, D_004A4A10, D_004A4A30, D_004A4A48,
    D_004A4A58, D_004A4A68, D_004A4A80, D_004A4A98,
    D_004A4AB0, D_004A4AC8, D_004A4AD8, D_004A4AE8,
    D_004A4B00, D_004A4B18, D_004A4B30, D_004A4B48,
    D_004A4B60, D_004A4B80, D_004A4BA0, D_004A4BC0,
    D_004A4BE0, D_004A4BF8, D_004A4C08, D_004A4C18,
    D_004A4C30, D_004A4C48, D_004A4C60, D_004A4C78,
    D_004A4C88, D_004A4C98, D_004A4CA8, D_004A4CB8,
    D_004A4CC8, D_004A4CE0, D_004A4D00, D_004A4D20,
    D_004A4D40, D_004A4D58, D_004A4D68, D_004A4D80,
    D_004A4D98, D_004A4DA8, D_004A4DB8, D_004A4DC8,
    D_004A4DD8, D_004A4DF0, D_004A4E10, D_004A4E30,
    D_004A4E50, D_004A4E70, D_004A4E90, D_004A4EB0,
    D_004A4ED0, D_004A4EF0, D_004A4F08, D_004A4F20,
    D_004A4F40, D_004A4F60, D_004A4F80, D_004A4FA0,
    D_004A4FB8, D_004A4FC8, D_004A4FD8, D_004A4FE8,
    D_004A4FF8, D_004A5008, D_004A5018, D_004A5028,
    D_004A5038, D_004A5048, D_004A5058, D_004A5070,
    D_004A5088, 0, 0, 0,
};
char *gArmorBodyNames[244] = {
    D_004A00E8, D_004A50A0, D_004A50B8, D_004A50C8,
    D_004A50E0, D_004A50F8, D_004A5108, D_004A5118,
    D_004A5130, D_004A5148, D_004A5158, D_004A5168,
    D_004A5178, D_004A5188, D_004A5198, D_004A51A8,
    D_004A51C0, D_004A51D8, D_004A51F0, D_004A5208,
    D_004A5218, D_004A5228, D_004A5240, D_004A5258,
    D_004A5270, D_004A5288, D_004A52A0, D_004A52C0,
    D_004A52D8, D_004A52E8, D_004A52F8, D_004A5310,
    D_004A5328, D_004A5338, D_004A5348, D_004A5358,
    D_004A5370, D_004A5390, D_004A53A8, D_004A53B8,
    D_004A53D0, D_004A53F0, D_004A5410, D_004A5430,
    D_004A5448, D_004A5460, D_004A5478, D_004A5488,
    D_004A54A0, D_004A54C0, D_004A54D8, D_004A54F0,
    D_004A5510, D_004A5530, D_004A5548, D_004A5558,
    D_004A5570, D_004A5590, D_004A55A8, D_004A55B8,
    D_004A55C8, D_004A55E0, D_004A55F8, D_004A5608,
    D_004A5618, D_004A5628, D_004A5638, D_004A5648,
    D_004A5660, D_004A5680, D_004A56A0, D_004A56C0,
    D_004A56D8, D_004A56F0, D_004A5710, D_004A5730,
    D_004A5748, D_004A5760, D_004A5778, D_004A5790,
    D_004A57A8, D_004A57C0, D_004A57E0, D_004A5800,
    D_004A5820, D_004A5840, D_004A5858, D_004A5870,
    D_004A5890, D_004A58B0, D_004A58D0, D_004A58F0,
    D_004A5910, D_004A5930, D_004A5950, D_004A5970,
    D_004A5990, D_004A59B0, D_004A59D0, D_004A59F0,
    D_004A5A10, D_004A5A28, D_004A5A40, D_004A5A58,
    D_004A5A70, D_004A5A90, D_004A5AB0, D_004A5AD0,
    D_004A5AF0, D_004A5B10, D_004A5B30, D_004A5B48,
    D_004A5B58, D_004A5B70, D_004A5B90, D_004A5BB0,
    D_004A5BD0, D_004A5BF0, D_004A5C10, D_004A5C30,
    D_004A5C50, D_004A5C70, D_004A5C90, D_004A5CB0,
    D_004A5CD0, D_004A5CF0, D_004A5D10, D_004A5D30,
    D_004A5D50, D_004A5D70, D_004A5D90, D_004A5DA8,
    D_004A5DC0, D_004A5DE0, D_004A5E00, D_004A5E18,
    D_004A5E30, D_004A5E48, D_004A5E60, D_004A5E78,
    D_004A5E90, D_004A5EB0, D_004A5ED0, D_004A5EF0,
    D_004A5F10, D_004A5F28, D_004A5F40, D_004A5F60,
    D_004A5F80, D_004A5FA0, D_004A5FC0, D_004A5FE0,
    D_004A6000, D_004A6020, D_004A6038, D_004A6050,
    D_004A6070, D_004A6090, D_004A60A8, D_004A60C0,
    D_004A60D8, D_004A60F0, D_004A6108, D_004A6120,
    D_004A6140, D_004A6160, D_004A6180, D_004A61A0,
    D_004A61C0, D_004A61E0, D_004A61F8, D_004A6208,
    D_004A6220, D_004A6240, D_004A6220, D_004A6240,
    D_004A6260, D_004A6280, D_004A62A0, D_004A62C0,
    D_004A62E0, D_004A6300, D_004A6318, D_004A6328,
    D_004A6338, D_004A6350, D_004A6368, D_004A6380,
    D_004A6398, D_004A63A8, D_004A63B8, D_004A63D0,
    D_004A63E8, D_004A6400, D_004A6418, D_004A6430,
    D_004A6450, D_004A6470, D_004A6490, D_004A64B0,
    D_004A64C8, D_004A64D8, D_004A64E8, D_004A6500,
    D_004A6518, D_004A6530, D_004A6548, D_004A6558,
    D_004A6568, D_004A6578, D_004A6590, D_004A65A8,
    D_004A65C0, D_004A65E0, D_004A6600, D_004A6620,
    D_004A6638, D_004A6648, D_004A6658, D_004A6670,
    D_004A6688, D_004A6698, D_004A66A8, D_004A66B8,
    D_004A66D0, D_004A66F0, D_004A6710, D_004A6730,
    D_004A6750, D_004A6770, D_004A6790, D_004A67B0,
    D_004A67C8, D_004A67E0, D_004A67F8, D_004A6810,
    D_004A6830, D_004A6850, D_004A6830, D_004A6850,
    D_004A6868, D_004A6878, D_004A6890, 0,
};
char *gArmorArmsNames[240] = {
    D_004A00E8, D_004A68B0, D_004A68C8, D_004A68D8,
    D_004A68E8, D_004A68F8, D_004A6908, D_004A6918,
    D_004A6928, D_004A6938, D_004A6948, D_004A6958,
    D_004A6968, D_004A6978, D_004A6988, D_004A6998,
    D_004A69A8, D_004A69B8, D_004A69C8, D_004A69D8,
    D_004A69E8, D_004A69F8, D_004A6A08, D_004A6A18,
    D_004A6A28, D_004A6A38, D_004A6A48, D_004A6A60,
    D_004A6A78, D_004A6A88, D_004A6A98, D_004A6AA8,
    D_004A6AB8, D_004A6AC8, D_004A6AD8, D_004A6AE8,
    D_004A6B00, D_004A6B20, D_004A6B38, D_004A6B48,
    D_004A6B60, D_004A6B80, D_004A6BA0, D_004A6BC0,
    D_004A6BD8, D_004A6BE8, D_004A6BF8, D_004A6C08,
    D_004A6C20, D_004A6C40, D_004A6C60, D_004A6C80,
    D_004A6C98, D_004A6CA8, D_004A6CC0, D_004A6CE0,
    D_004A6CF8, D_004A6D08, D_004A6D18, D_004A6D28,
    D_004A6D38, D_004A6D48, D_004A6D58, D_004A6D68,
    D_004A6D78, D_004A6D90, D_004A6DB0, D_004A6DD0,
    D_004A6DF0, D_004A6E10, D_004A6E28, D_004A6E38,
    D_004A6E50, D_004A6E70, D_004A6E88, D_004A6E98,
    D_004A6EA8, D_004A6EB8, D_004A6EC8, D_004A6ED8,
    D_004A6EF0, D_004A6F10, D_004A6F30, D_004A6F50,
    D_004A6F68, D_004A6F78, D_004A6F90, D_004A6FB0,
    D_004A6FD0, D_004A6FF0, D_004A7010, D_004A7030,
    D_004A7050, D_004A7070, D_004A7090, D_004A70B0,
    D_004A70D0, D_004A70F0, D_004A7110, D_004A7128,
    D_004A7138, D_004A7148, D_004A7158, D_004A7170,
    D_004A7190, D_004A71B0, D_004A71D0, D_004A71F0,
    D_004A7210, D_004A7228, D_004A7238, D_004A7250,
    D_004A7270, D_004A7290, D_004A72B0, D_004A72D0,
    D_004A72F0, D_004A7308, D_004A7318, D_004A7330,
    D_004A7350, D_004A7370, D_004A7390, D_004A73B0,
    D_004A73D0, D_004A73F0, D_004A7410, D_004A7430,
    D_004A7450, D_004A7468, D_004A7478, D_004A7490,
    D_004A74B0, D_004A74C8, D_004A74D8, D_004A74E8,
    D_004A74F8, D_004A7508, D_004A7518, D_004A7530,
    D_004A7550, D_004A7570, D_004A7590, D_004A75A8,
    D_004A75B8, D_004A75D0, D_004A75F0, D_004A7610,
    D_004A7630, D_004A7650, D_004A7670, D_004A7690,
    D_004A76A8, D_004A76B8, D_004A76D0, D_004A76F0,
    D_004A7708, D_004A7718, D_004A7728, D_004A7738,
    D_004A7748, D_004A7758, D_004A7770, D_004A7790,
    D_004A77B0, D_004A77D0, D_004A77F0, D_004A7810,
    D_004A7828, D_004A7838, D_004A7850, D_004A7870,
    D_004A7850, D_004A7870, D_004A7890, D_004A78B0,
    D_004A78C8, D_004A78D8, D_004A78F0, D_004A7910,
    D_004A7928, D_004A7938, D_004A7948, D_004A7958,
    D_004A7968, D_004A7978, D_004A7988, D_004A7998,
    D_004A79A8, D_004A79B8, D_004A79C8, D_004A79D8,
    D_004A79E8, D_004A79F8, D_004A7A10, D_004A7A30,
    D_004A7A50, D_004A7A70, D_004A7A88, D_004A7A98,
    D_004A7AA8, D_004A7AB8, D_004A7AC8, D_004A7AD8,
    D_004A7AF0, D_004A7B08, D_004A7B20, D_004A7B40,
    D_004A7B60, D_004A7B80, D_004A7B98, D_004A7BB0,
    D_004A7BD0, D_004A7BF0, D_004A7C08, D_004A7C18,
    D_004A7C28, D_004A7C38, D_004A7C48, D_004A7C58,
    D_004A7C68, D_004A7C78, D_004A7C90, D_004A7CB0,
    D_004A7CD0, D_004A7CF0, D_004A7D10, D_004A7D30,
    D_004A7D50, D_004A7D68, D_004A7D80, D_004A7DA0,
    D_004A7DC0, D_004A7DA0, D_004A7DC0, D_004A7DD8,
    D_004A7DE8, D_004A7DF8, 0, 0,
};
char *gArmorWaistNames[240] = {
    D_004A00E8, D_004A7E10, D_004A7E30, D_004A7E50,
    D_004A7E68, D_004A7E80, D_004A7E98, D_004A7EB0,
    D_004A7EC8, D_004A7EE0, D_004A7EF8, D_004A7F10,
    D_004A7F28, D_004A7F38, D_004A7F50, D_004A7F68,
    D_004A7F80, D_004A7F98, D_004A7FB0, D_004A7FC8,
    D_004A7FE0, D_004A7FF8, D_004A8010, D_004A8030,
    D_004A8048, D_004A8060, D_004A8078, D_004A8090,
    D_004A80A8, D_004A80C0, D_004A80D8, D_004A80F0,
    D_004A8110, D_004A8128, D_004A8140, D_004A8158,
    D_004A8170, D_004A8188, D_004A81A0, D_004A81B8,
    D_004A81D0, D_004A81F0, D_004A8210, D_004A8230,
    D_004A8250, D_004A8268, D_004A8280, D_004A8298,
    D_004A82A8, D_004A82C0, D_004A82D8, D_004A82E8,
    D_004A8300, D_004A8320, D_004A8340, D_004A8358,
    D_004A8370, D_004A8388, D_004A8398, D_004A83A8,
    D_004A83B8, D_004A83C8, D_004A83D8, D_004A83E8,
    D_004A8400, D_004A8418, D_004A8430, D_004A8450,
    D_004A8470, D_004A8490, D_004A84B0, D_004A84C8,
    D_004A84E0, D_004A8500, D_004A8520, D_004A8538,
    D_004A8550, D_004A8568, D_004A8580, D_004A8598,
    D_004A85B0, D_004A85D0, D_004A85F0, D_004A8610,
    D_004A8630, D_004A8648, D_004A8660, D_004A8680,
    D_004A86A0, D_004A86C0, D_004A86E0, D_004A8700,
    D_004A8720, D_004A8740, D_004A8760, D_004A8780,
    D_004A87A0, D_004A87C0, D_004A87E0, D_004A8800,
    D_004A8818, D_004A8830, D_004A8850, D_004A8870,
    D_004A8890, D_004A88B0, D_004A88D0, D_004A88E8,
    D_004A88F8, D_004A8910, D_004A8930, D_004A8950,
    D_004A8970, D_004A8990, D_004A89B0, D_004A89D0,
    D_004A89F0, D_004A8A10, D_004A8A30, D_004A8A50,
    D_004A8A70, D_004A8A90, D_004A8AB0, D_004A8AD0,
    D_004A8AF0, D_004A8B10, D_004A8B30, D_004A8B50,
    D_004A8B68, D_004A8B80, D_004A8BA0, D_004A8BC0,
    D_004A8BD8, D_004A8BF0, D_004A8C08, D_004A8C20,
    D_004A8C38, D_004A8C50, D_004A8C70, D_004A8C90,
    D_004A8CB0, D_004A8CD0, D_004A8CE8, D_004A8D00,
    D_004A8D20, D_004A8D40, D_004A8D60, D_004A8D80,
    D_004A8DA0, D_004A8DC0, D_004A8DE0, D_004A8DF8,
    D_004A8E10, D_004A8E30, D_004A8E50, D_004A8E68,
    D_004A8E80, D_004A8E98, D_004A8EB0, D_004A8ED0,
    D_004A8EF0, D_004A8F10, D_004A8F30, D_004A8F50,
    D_004A8F68, D_004A8F78, D_004A8F90, D_004A8FB0,
    D_004A8F90, D_004A8FB0, D_004A8FD0, D_004A8FF0,
    D_004A9010, D_004A9030, D_004A9050, D_004A9070,
    D_004A9090, D_004A90A8, D_004A90C0, D_004A90D8,
    D_004A90F0, D_004A9108, D_004A9120, D_004A9138,
    D_004A9150, D_004A9168, D_004A9180, D_004A9198,
    D_004A91B0, D_004A91C8, D_004A91E0, D_004A9200,
    D_004A9220, D_004A9240, D_004A9260, D_004A9278,
    D_004A9290, D_004A92A8, D_004A92C0, D_004A92D8,
    D_004A92E8, D_004A92F8, D_004A9308, D_004A9320,
    D_004A9338, D_004A9348, D_004A9358, D_004A9370,
    D_004A9390, D_004A93B0, D_004A93C8, D_004A93D8,
    D_004A93F0, D_004A9408, D_004A9418, D_004A9428,
    D_004A9438, D_004A9448, D_004A9460, D_004A9480,
    D_004A94A0, D_004A94B8, D_004A94D0, D_004A94F0,
    D_004A9510, D_004A9530, D_004A9548, D_004A9560,
    D_004A9580, D_004A95A0, D_004A9580, D_004A95A0,
    D_004A95C0, D_004A95E0, D_004A9600, D_004A9618,
    D_004A9628, 0, 0, 0,
};
char *gArmorLegsNames[236] = {
    D_004A00E8, D_004A9640, D_004A9660, D_004A9680,
    D_004A96A0, D_004A96B8, D_004A96C8, D_004A96E0,
    D_004A9700, D_004A9718, D_004A9728, D_004A9740,
    D_004A9758, D_004A9768, D_004A9778, D_004A9788,
    D_004A97A0, D_004A97C0, D_004A97E0, D_004A9800,
    D_004A9820, D_004A9840, D_004A9860, D_004A9880,
    D_004A98A0, D_004A98C0, D_004A98E0, D_004A9900,
    D_004A9920, D_004A9938, D_004A9948, D_004A9960,
    D_004A9980, D_004A9998, D_004A99A8, D_004A99B8,
    D_004A99C8, D_004A99D8, D_004A99E8, D_004A9A00,
    D_004A9A20, D_004A9A40, D_004A9A60, D_004A9A80,
    D_004A9AA0, D_004A9AC0, D_004A9AD8, D_004A9AF0,
    D_004A9B10, D_004A9B30, D_004A9B48, D_004A9B58,
    D_004A9B70, D_004A9B90, D_004A9BB0, D_004A9BD0,
    D_004A9BF0, D_004A9C10, D_004A9C28, D_004A9C38,
    D_004A9C48, D_004A9C58, D_004A9C68, D_004A9C78,
    D_004A9C88, D_004A9C98, D_004A9CB0, D_004A9CD0,
    D_004A9CF0, D_004A9D10, D_004A9D30, D_004A9D50,
    D_004A9D70, D_004A9D90, D_004A9DB0, D_004A9DD0,
    D_004A9DF0, D_004A9E10, D_004A9E30, D_004A9E50,
    D_004A9E70, D_004A9E90, D_004A9EB0, D_004A9ED0,
    D_004A9EF0, D_004A9F10, D_004A9F30, D_004A9F50,
    D_004A9F70, D_004A9F90, D_004A9FB0, D_004A9FD0,
    D_004A9FF0, D_004AA010, D_004AA030, D_004AA050,
    D_004AA070, D_004AA090, D_004AA0B0, D_004AA0D0,
    D_004AA0F0, D_004AA110, D_004AA130, D_004AA150,
    D_004AA170, D_004AA190, D_004AA1A8, D_004AA1B8,
    D_004AA1D0, D_004AA1F0, D_004AA210, D_004AA230,
    D_004AA250, D_004AA270, D_004AA288, D_004AA298,
    D_004AA2B0, D_004AA2D0, D_004AA2F0, D_004AA310,
    D_004AA330, D_004AA350, D_004AA370, D_004AA390,
    D_004AA3B0, D_004AA3D0, D_004AA3F0, D_004AA410,
    D_004AA430, D_004AA450, D_004AA470, D_004AA490,
    D_004AA4B0, D_004AA4D0, D_004AA4F0, D_004AA510,
    D_004AA530, D_004AA550, D_004AA570, D_004AA590,
    D_004AA5B0, D_004AA5D0, D_004AA5F0, D_004AA610,
    D_004AA630, D_004AA650, D_004AA670, D_004AA690,
    D_004AA6B0, D_004AA6D0, D_004AA6F0, D_004AA710,
    D_004AA730, D_004AA750, D_004AA770, D_004AA790,
    D_004AA7B0, D_004AA7D0, D_004AA7F0, D_004AA810,
    D_004AA830, D_004AA850, D_004AA870, D_004AA890,
    D_004AA8A8, D_004AA8B8, D_004AA8D0, D_004AA8F0,
    D_004AA8D0, D_004AA8F0, D_004AA910, D_004AA930,
    D_004AA948, D_004AA958, D_004AA970, D_004AA990,
    D_004AA9A8, D_004AA9B8, D_004AA9D0, D_004AA9F0,
    D_004AAA10, D_004AAA30, D_004AAA48, D_004AAA58,
    D_004AAA70, D_004AAA90, D_004AAAB0, D_004AAAD0,
    D_004AAAF0, D_004AAB10, D_004AAB30, D_004AAB50,
    D_004AAB70, D_004AAB90, D_004AABA8, D_004AABB8,
    D_004AABD0, D_004AABF0, D_004AAC10, D_004AAC30,
    D_004AAC48, D_004AAC58, D_004AAC70, D_004AAC88,
    D_004AAC98, D_004AACA8, D_004AACB8, D_004AACD0,
    D_004AACF0, D_004AAD10, D_004AAD28, D_004AAD38,
    D_004AAD50, D_004AAD70, D_004AAD88, D_004AAD98,
    D_004AADA8, D_004AADB8, D_004AADD0, D_004AADF0,
    D_004AAE10, D_004AAE30, D_004AAE50, D_004AAE70,
    D_004AAE90, D_004AAEB0, D_004AAED0, D_004AAEF0,
    D_004AAF10, D_004AAEF0, D_004AAF10, D_004AAF30,
    D_004AAF50, D_004AAF70, D_004AAF90, D_004AAFB0,
};
u8 gWeaponKindClass[352] = {
#include "sub_main/0045DF80.inc"
};
u8 gAmmoLevels[36][4] = {
#include "sub_main/0045E0E0.inc"
};
u32 gBowgunClipSizes[264] = {
#include "sub_main/0045E170.inc"
};
u32 gBowgunClipShots[60] = {
#include "sub_main/0045E590.inc"
};
u32 gSharpnessTables_0[24] = {
#include "sub_main/0045E680.inc"
};
u32 gSharpnessTables_2[24] = {
#include "sub_main/0045E6E0.inc"
};
u32 gSharpnessTables_3[24] = {
#include "sub_main/0045E740.inc"
};
u32 gSharpnessTables_4[24] = {
#include "sub_main/0045E7A0.inc"
};
u32 gSharpnessTables_6[24] = {
#include "sub_main/0045E800.inc"
};
u32 gSharpnessTables[12] = {
    (u32)gSharpnessTables_0, (u32)gSharpnessTables_0, (u32)gSharpnessTables_2, (u32)gSharpnessTables_3,
    (u32)gSharpnessTables_4, (u32)gSharpnessTables_0, (u32)gSharpnessTables_6, (u32)gSharpnessTables_0,
    (u32)gSharpnessTables_2, (u32)gSharpnessTables_3, (u32)gSharpnessTables_0, 0,
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
u32 gHitSeIds[44] = {
#include "sub_main/0045EF10.inc"
};
u32 gHitKindMvFactors[24] = {
#include "sub_main/0045EFC0.inc"
};
u32 gSwordGaugeHitFactors[4] = {
#include "sub_main/0045F020.inc"
};
u32 gSharpnessRawMods[12] = {
#include "sub_main/0045F030.inc"
};
u32 gSharpnessLossByHit[4] = {
#include "sub_main/0045F060.inc"
};
u32 gAreaKindFlags_170[4] = {
#include "sub_main/0045F070.inc"
};
u32 D_0045F080[12] = {
#include "sub_main/0045F080.inc"
};
u32 gAreaLightColors_170[24] = {
#include "sub_main/0045F0B0.inc"
};
ItemData gItemData[170] = {
#include "sub_main/0045F110.inc"
};
u32 D_00460100[12] = {
#include "sub_main/00460100.inc"
};
u32 D_00460130[192] = {
#include "sub_main/00460130.inc"
};
u32 D_00460430[4304] = {
#include "sub_main/00460430.inc"
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
char D_004A00E8[0xC] = {
#include "sub_main/004A00E8.inc"
};
char gBladeNames_1[8] = {
#include "sub_main/004A00F8.inc"
};
u32 D_004A0100[4] = {
#include "sub_main/004A0100.inc"
};
char D_004A0110[0x18] = {
#include "sub_main/004A0110.inc"
};
char D_004A0128[15] = {
#include "sub_main/004A0128.inc"
};
char D_004A0140[0x18] = {
#include "sub_main/004A0140.inc"
};
char D_004A0158[0xC] = {
#include "sub_main/004A0158.inc"
};
char D_004A0170[0x18] = {
#include "sub_main/004A0170.inc"
};
char D_004A0188[15] = {
#include "sub_main/004A0188.inc"
};
char D_004A01A0[0x20] = {
#include "sub_main/004A01A0.inc"
};
char D_004A01C0[0x18] = {
#include "sub_main/004A01C0.inc"
};
char D_004A01D8[0x8] = {
#include "sub_main/004A01D8.inc"
};
char D_004A01E0[0x10] = {
#include "sub_main/004A01E0.inc"
};
char D_004A01F0[0x10] = {
#include "sub_main/004A01F0.inc"
};
char D_004A0200[0x10] = {
#include "sub_main/004A0200.inc"
};
char D_004A0210[0x10] = {
#include "sub_main/004A0210.inc"
};
char D_004A0220[0x10] = {
#include "sub_main/004A0220.inc"
};
char D_004A0230[0x10] = {
#include "sub_main/004A0230.inc"
};
char D_004A0240[0x10] = {
#include "sub_main/004A0240.inc"
};
char D_004A0250[0x10] = {
#include "sub_main/004A0250.inc"
};
char D_004A0260[0x10] = {
#include "sub_main/004A0260.inc"
};
char D_004A0270[0x10] = {
#include "sub_main/004A0270.inc"
};
char D_004A0280[0x10] = {
#include "sub_main/004A0280.inc"
};
char D_004A0290[0x10] = {
#include "sub_main/004A0290.inc"
};
char D_004A02A0[0x10] = {
#include "sub_main/004A02A0.inc"
};
char D_004A02B0[0x20] = {
#include "sub_main/004A02B0.inc"
};
char D_004A02D0[0x18] = {
#include "sub_main/004A02D0.inc"
};
char D_004A02E8[13] = {
#include "sub_main/004A02E8.inc"
};
char D_004A0300[0x18] = {
#include "sub_main/004A0300.inc"
};
char D_004A0318[15] = {
#include "sub_main/004A0318.inc"
};
char D_004A0330[0x18] = {
#include "sub_main/004A0330.inc"
};
char D_004A0348[0x8] = {
#include "sub_main/004A0348.inc"
};
char D_004A0350[0x10] = {
#include "sub_main/004A0350.inc"
};
char D_004A0360[0x10] = {
#include "sub_main/004A0360.inc"
};
char D_004A0370[0x20] = {
#include "sub_main/004A0370.inc"
};
char D_004A0390[0x20] = {
#include "sub_main/004A0390.inc"
};
char D_004A03B0[0x18] = {
#include "sub_main/004A03B0.inc"
};
char D_004A03C8[13] = {
#include "sub_main/004A03C8.inc"
};
char D_004A03D8[15] = {
#include "sub_main/004A03D8.inc"
};
char D_004A03E8[13] = {
#include "sub_main/004A03E8.inc"
};
char D_004A03F8[15] = {
#include "sub_main/004A03F8.inc"
};
char D_004A0408[15] = {
#include "sub_main/004A0408.inc"
};
char D_004A0418[15] = {
#include "sub_main/004A0418.inc"
};
char D_004A0428[15] = {
#include "sub_main/004A0428.inc"
};
char D_004A0438[15] = {
#include "sub_main/004A0438.inc"
};
char D_004A0448[15] = {
#include "sub_main/004A0448.inc"
};
char D_004A0458[0xC] = {
#include "sub_main/004A0458.inc"
};
char D_004A0470[0x20] = {
#include "sub_main/004A0470.inc"
};
char D_004A0490[0x18] = {
#include "sub_main/004A0490.inc"
};
char D_004A04A8[0x8] = {
#include "sub_main/004A04A8.inc"
};
char D_004A04B0[0x10] = {
#include "sub_main/004A04B0.inc"
};
char D_004A04C0[0x10] = {
#include "sub_main/004A04C0.inc"
};
char D_004A04D0[0x10] = {
#include "sub_main/004A04D0.inc"
};
char D_004A04E0[0x10] = {
#include "sub_main/004A04E0.inc"
};
char D_004A04F0[0x18] = {
#include "sub_main/004A04F0.inc"
};
char D_004A0508[13] = {
#include "sub_main/004A0508.inc"
};
char D_004A0520[0x20] = {
#include "sub_main/004A0520.inc"
};
char D_004A0540[0x18] = {
#include "sub_main/004A0540.inc"
};
char D_004A0558[15] = {
#include "sub_main/004A0558.inc"
};
char D_004A0568[0x8] = {
#include "sub_main/004A0568.inc"
};
char D_004A0570[0x10] = {
#include "sub_main/004A0570.inc"
};
char D_004A0580[0x20] = {
#include "sub_main/004A0580.inc"
};
char D_004A05A0[0x20] = {
#include "sub_main/004A05A0.inc"
};
char D_004A05C0[0x20] = {
#include "sub_main/004A05C0.inc"
};
char D_004A05E0[0x20] = {
#include "sub_main/004A05E0.inc"
};
char D_004A0600[0x20] = {
#include "sub_main/004A0600.inc"
};
char D_004A0620[0x20] = {
#include "sub_main/004A0620.inc"
};
char D_004A0640[0x20] = {
#include "sub_main/004A0640.inc"
};
char D_004A0660[0x20] = {
#include "sub_main/004A0660.inc"
};
char D_004A0680[0x20] = {
#include "sub_main/004A0680.inc"
};
char D_004A06A0[0x18] = {
#include "sub_main/004A06A0.inc"
};
char D_004A06B8[13] = {
#include "sub_main/004A06B8.inc"
};
char D_004A06C8[13] = {
#include "sub_main/004A06C8.inc"
};
char D_004A06E0[0x20] = {
#include "sub_main/004A06E0.inc"
};
char D_004A0700[0x18] = {
#include "sub_main/004A0700.inc"
};
char D_004A0718[15] = {
#include "sub_main/004A0718.inc"
};
char D_004A0730[0x18] = {
#include "sub_main/004A0730.inc"
};
char D_004A0748[0xC] = {
#include "sub_main/004A0748.inc"
};
char D_004A0760[0x18] = {
#include "sub_main/004A0760.inc"
};
char D_004A0778[15] = {
#include "sub_main/004A0778.inc"
};
char D_004A0790[0x20] = {
#include "sub_main/004A0790.inc"
};
char D_004A07B0[0x18] = {
#include "sub_main/004A07B0.inc"
};
char D_004A07C8[0xC] = {
#include "sub_main/004A07C8.inc"
};
char D_004A07E0[0x20] = {
#include "sub_main/004A07E0.inc"
};
char D_004A0800[0x20] = {
#include "sub_main/004A0800.inc"
};
char D_004A0820[0x18] = {
#include "sub_main/004A0820.inc"
};
char D_004A0838[13] = {
#include "sub_main/004A0838.inc"
};
char D_004A0848[15] = {
#include "sub_main/004A0848.inc"
};
char D_004A0860[0x18] = {
#include "sub_main/004A0860.inc"
};
char D_004A0878[13] = {
#include "sub_main/004A0878.inc"
};
char D_004A0888[13] = {
#include "sub_main/004A0888.inc"
};
char D_004A08A0[0x20] = {
#include "sub_main/004A08A0.inc"
};
char D_004A08C0[0x18] = {
#include "sub_main/004A08C0.inc"
};
char D_004A08D8[15] = {
#include "sub_main/004A08D8.inc"
};
char D_004A08F0[0x20] = {
#include "sub_main/004A08F0.inc"
};
char D_004A0910[0x18] = {
#include "sub_main/004A0910.inc"
};
char D_004A0928[13] = {
#include "sub_main/004A0928.inc"
};
char D_004A0938[0xC] = {
#include "sub_main/004A0938.inc"
};
char D_004A0948[0x8] = {
#include "sub_main/004A0948.inc"
};
char D_004A0950[0x10] = {
#include "sub_main/004A0950.inc"
};
char D_004A0960[0x8] = {
#include "sub_main/004A0960.inc"
};
char D_004A0968[0x8] = {
#include "sub_main/004A0968.inc"
};
char D_004A0970[0x20] = {
#include "sub_main/004A0970.inc"
};
char D_004A0990[0x18] = {
#include "sub_main/004A0990.inc"
};
char D_004A09A8[15] = {
#include "sub_main/004A09A8.inc"
};
char D_004A09C0[0x18] = {
#include "sub_main/004A09C0.inc"
};
char D_004A09D8[13] = {
#include "sub_main/004A09D8.inc"
};
char D_004A09E8[13] = {
#include "sub_main/004A09E8.inc"
};
char D_004A0A00[0x20] = {
#include "sub_main/004A0A00.inc"
};
char D_004A0A20[0x18] = {
#include "sub_main/004A0A20.inc"
};
char D_004A0A38[0xC] = {
#include "sub_main/004A0A38.inc"
};
char D_004A0A50[0x18] = {
#include "sub_main/004A0A50.inc"
};
char D_004A0A68[0x8] = {
#include "sub_main/004A0A68.inc"
};
char D_004A0A70[0x10] = {
#include "sub_main/004A0A70.inc"
};
char D_004A0A80[0x18] = {
#include "sub_main/004A0A80.inc"
};
char D_004A0A98[15] = {
#include "sub_main/004A0A98.inc"
};
char D_004A0AA8[13] = {
#include "sub_main/004A0AA8.inc"
};
char D_004A0AB8[15] = {
#include "sub_main/004A0AB8.inc"
};
char D_004A0AD0[0x18] = {
#include "sub_main/004A0AD0.inc"
};
char D_004A0AE8[13] = {
#include "sub_main/004A0AE8.inc"
};
char D_004A0AF8[13] = {
#include "sub_main/004A0AF8.inc"
};
char D_004A0B10[0x18] = {
#include "sub_main/004A0B10.inc"
};
char D_004A0B28[13] = {
#include "sub_main/004A0B28.inc"
};
char D_004A0B38[15] = {
#include "sub_main/004A0B38.inc"
};
char D_004A0B48[15] = {
#include "sub_main/004A0B48.inc"
};
char D_004A0B58[0xC] = {
#include "sub_main/004A0B58.inc"
};
char D_004A0B70[0x20] = {
#include "sub_main/004A0B70.inc"
};
char D_004A0B90[0x18] = {
#include "sub_main/004A0B90.inc"
};
char D_004A0BA8[13] = {
#include "sub_main/004A0BA8.inc"
};
char D_004A0BB8[15] = {
#include "sub_main/004A0BB8.inc"
};
char D_004A0BC8[13] = {
#include "sub_main/004A0BC8.inc"
};
char D_004A0BE0[0x18] = {
#include "sub_main/004A0BE0.inc"
};
char D_004A0BF8[13] = {
#include "sub_main/004A0BF8.inc"
};
char D_004A0C10[0x18] = {
#include "sub_main/004A0C10.inc"
};
char D_004A0C28[15] = {
#include "sub_main/004A0C28.inc"
};
char D_004A0C38[15] = {
#include "sub_main/004A0C38.inc"
};
char D_004A0C48[15] = {
#include "sub_main/004A0C48.inc"
};
char D_004A0C58[13] = {
#include "sub_main/004A0C58.inc"
};
char D_004A0C70[0x18] = {
#include "sub_main/004A0C70.inc"
};
char D_004A0C88[0xC] = {
#include "sub_main/004A0C88.inc"
};
char D_004A0C98[13] = {
#include "sub_main/004A0C98.inc"
};
char D_004A0CB0[0x20] = {
#include "sub_main/004A0CB0.inc"
};
char D_004A0CD0[0x20] = {
#include "sub_main/004A0CD0.inc"
};
char D_004A0CF0[0x20] = {
#include "sub_main/004A0CF0.inc"
};
char D_004A0D10[0x18] = {
#include "sub_main/004A0D10.inc"
};
char D_004A0D28[15] = {
#include "sub_main/004A0D28.inc"
};
char D_004A0D40[0x20] = {
#include "sub_main/004A0D40.inc"
};
char D_004A0D60[0x18] = {
#include "sub_main/004A0D60.inc"
};
char D_004A0D78[15] = {
#include "sub_main/004A0D78.inc"
};
char D_004A0D90[0x20] = {
#include "sub_main/004A0D90.inc"
};
char D_004A0DB0[0x18] = {
#include "sub_main/004A0DB0.inc"
};
char D_004A0DC8[13] = {
#include "sub_main/004A0DC8.inc"
};
char D_004A0DD8[0xC] = {
#include "sub_main/004A0DD8.inc"
};
char D_004A0DF0[0x18] = {
#include "sub_main/004A0DF0.inc"
};
char D_004A0E08[15] = {
#include "sub_main/004A0E08.inc"
};
char D_004A0E18[15] = {
#include "sub_main/004A0E18.inc"
};
char D_004A0E28[15] = {
#include "sub_main/004A0E28.inc"
};
char D_004A0E38[15] = {
#include "sub_main/004A0E38.inc"
};
char D_004A0E50[0x20] = {
#include "sub_main/004A0E50.inc"
};
char D_004A0E70[0x20] = {
#include "sub_main/004A0E70.inc"
};
char D_004A0E90[0x20] = {
#include "sub_main/004A0E90.inc"
};
char D_004A0EB0[0x18] = {
#include "sub_main/004A0EB0.inc"
};
char D_004A0EC8[0x8] = {
#include "sub_main/004A0EC8.inc"
};
char D_004A0ED0[0x8] = {
#include "sub_main/004A0ED0.inc"
};
char D_004A0ED8[0xC] = {
#include "sub_main/004A0ED8.inc"
};
char D_004A0EE8[13] = {
#include "sub_main/004A0EE8.inc"
};
char D_004A0F00[0x20] = {
#include "sub_main/004A0F00.inc"
};
char D_004A0F20[0x20] = {
#include "sub_main/004A0F20.inc"
};
char D_004A0F40[0x18] = {
#include "sub_main/004A0F40.inc"
};
char D_004A0F58[0xC] = {
#include "sub_main/004A0F58.inc"
};
char D_004A0F68[13] = {
#include "sub_main/004A0F68.inc"
};
char D_004A0F78[15] = {
#include "sub_main/004A0F78.inc"
};
char D_004A0F90[0x20] = {
#include "sub_main/004A0F90.inc"
};
char D_004A0FB0[0x18] = {
#include "sub_main/004A0FB0.inc"
};
char D_004A0FC8[15] = {
#include "sub_main/004A0FC8.inc"
};
char D_004A0FE0[0x20] = {
#include "sub_main/004A0FE0.inc"
};
char D_004A1000[0x20] = {
#include "sub_main/004A1000.inc"
};
char D_004A1020[0x20] = {
#include "sub_main/004A1020.inc"
};
char D_004A1040[0x18] = {
#include "sub_main/004A1040.inc"
};
char D_004A1058[13] = {
#include "sub_main/004A1058.inc"
};
char D_004A1068[15] = {
#include "sub_main/004A1068.inc"
};
char D_004A1078[13] = {
#include "sub_main/004A1078.inc"
};
char D_004A1088[15] = {
#include "sub_main/004A1088.inc"
};
char D_004A1098[13] = {
#include "sub_main/004A1098.inc"
};
char D_004A10A8[13] = {
#include "sub_main/004A10A8.inc"
};
char D_004A10C0[0x18] = {
#include "sub_main/004A10C0.inc"
};
char D_004A10D8[15] = {
#include "sub_main/004A10D8.inc"
};
char D_004A10F0[0x20] = {
#include "sub_main/004A10F0.inc"
};
char D_004A1110[0x18] = {
#include "sub_main/004A1110.inc"
};
char D_004A1128[0x8] = {
#include "sub_main/004A1128.inc"
};
char D_004A1130[0x20] = {
#include "sub_main/004A1130.inc"
};
char D_004A1150[0x18] = {
#include "sub_main/004A1150.inc"
};
char D_004A1168[15] = {
#include "sub_main/004A1168.inc"
};
char D_004A1178[15] = {
#include "sub_main/004A1178.inc"
};
char D_004A1190[0x18] = {
#include "sub_main/004A1190.inc"
};
char D_004A11A8[0xC] = {
#include "sub_main/004A11A8.inc"
};
char D_004A11C0[0x20] = {
#include "sub_main/004A11C0.inc"
};
char D_004A11E0[0x18] = {
#include "sub_main/004A11E0.inc"
};
char D_004A11F8[15] = {
#include "sub_main/004A11F8.inc"
};
char D_004A1208[0xC] = {
#include "sub_main/004A1208.inc"
};
char D_004A1218[15] = {
#include "sub_main/004A1218.inc"
};
char D_004A1230[0x18] = {
#include "sub_main/004A1230.inc"
};
char D_004A1248[0x8] = {
#include "sub_main/004A1248.inc"
};
char D_004A1250[0x10] = {
#include "sub_main/004A1250.inc"
};
char D_004A1260[0x18] = {
#include "sub_main/004A1260.inc"
};
char D_004A1278[13] = {
#include "sub_main/004A1278.inc"
};
char D_004A1288[13] = {
#include "sub_main/004A1288.inc"
};
char D_004A12A0[0x18] = {
#include "sub_main/004A12A0.inc"
};
char D_004A12B8[13] = {
#include "sub_main/004A12B8.inc"
};
char D_004A12C8[0x8] = {
#include "sub_main/004A12C8.inc"
};
char D_004A12D0[0x10] = {
#include "sub_main/004A12D0.inc"
};
char D_004A12E0[0x20] = {
#include "sub_main/004A12E0.inc"
};
char D_004A1300[0x18] = {
#include "sub_main/004A1300.inc"
};
char D_004A1318[15] = {
#include "sub_main/004A1318.inc"
};
char D_004A1330[0x20] = {
#include "sub_main/004A1330.inc"
};
char D_004A1350[0x18] = {
#include "sub_main/004A1350.inc"
};
char D_004A1368[0xC] = {
#include "sub_main/004A1368.inc"
};
char D_004A1380[0x20] = {
#include "sub_main/004A1380.inc"
};
char D_004A13A0[0x20] = {
#include "sub_main/004A13A0.inc"
};
char D_004A13C0[0x18] = {
#include "sub_main/004A13C0.inc"
};
char D_004A13D8[0x8] = {
#include "sub_main/004A13D8.inc"
};
char D_004A13E0[0x10] = {
#include "sub_main/004A13E0.inc"
};
char D_004A13F0[0x8] = {
#include "sub_main/004A13F0.inc"
};
char D_004A13F8[0x8] = {
#include "sub_main/004A13F8.inc"
};
char D_004A1400[0x10] = {
#include "sub_main/004A1400.inc"
};
char D_004A1410[0x10] = {
#include "sub_main/004A1410.inc"
};
char D_004A1420[0x10] = {
#include "sub_main/004A1420.inc"
};
char D_004A1430[0x10] = {
#include "sub_main/004A1430.inc"
};
char D_004A1440[0x8] = {
#include "sub_main/004A1440.inc"
};
char D_004A1448[13] = {
#include "sub_main/004A1448.inc"
};
char D_004A1458[15] = {
#include "sub_main/004A1458.inc"
};
char D_004A1470[0x18] = {
#include "sub_main/004A1470.inc"
};
char D_004A1488[0xC] = {
#include "sub_main/004A1488.inc"
};
char D_004A14A0[0x18] = {
#include "sub_main/004A14A0.inc"
};
char D_004A14B8[0xC] = {
#include "sub_main/004A14B8.inc"
};
char D_004A14C8[13] = {
#include "sub_main/004A14C8.inc"
};
char D_004A14E0[0x18] = {
#include "sub_main/004A14E0.inc"
};
char D_004A14F8[0xC] = {
#include "sub_main/004A14F8.inc"
};
char D_004A1508[15] = {
#include "sub_main/004A1508.inc"
};
char D_004A1518[0xC] = {
#include "sub_main/004A1518.inc"
};
char D_004A1530[0x20] = {
#include "sub_main/004A1530.inc"
};
char D_004A1550[0x20] = {
#include "sub_main/004A1550.inc"
};
char D_004A1570[0x18] = {
#include "sub_main/004A1570.inc"
};
char D_004A1588[15] = {
#include "sub_main/004A1588.inc"
};
char D_004A15A0[0x20] = {
#include "sub_main/004A15A0.inc"
};
char D_004A15C0[0x18] = {
#include "sub_main/004A15C0.inc"
};
char D_004A15D8[15] = {
#include "sub_main/004A15D8.inc"
};
char D_004A15E8[15] = {
#include "sub_main/004A15E8.inc"
};
char D_004A1600[0x18] = {
#include "sub_main/004A1600.inc"
};
char D_004A1618[15] = {
#include "sub_main/004A1618.inc"
};
char D_004A1630[0x18] = {
#include "sub_main/004A1630.inc"
};
char D_004A1648[13] = {
#include "sub_main/004A1648.inc"
};
char D_004A1658[15] = {
#include "sub_main/004A1658.inc"
};
char D_004A1670[0x20] = {
#include "sub_main/004A1670.inc"
};
char D_004A1690[0x18] = {
#include "sub_main/004A1690.inc"
};
char D_004A16A8[15] = {
#include "sub_main/004A16A8.inc"
};
char D_004A16C0[0x20] = {
#include "sub_main/004A16C0.inc"
};
char D_004A16E0[0x18] = {
#include "sub_main/004A16E0.inc"
};
char D_004A16F8[0x8] = {
#include "sub_main/004A16F8.inc"
};
char D_004A1700[0x10] = {
#include "sub_main/004A1700.inc"
};
char D_004A1710[0x20] = {
#include "sub_main/004A1710.inc"
};
char D_004A1730[0x18] = {
#include "sub_main/004A1730.inc"
};
char D_004A1748[15] = {
#include "sub_main/004A1748.inc"
};
char D_004A1758[15] = {
#include "sub_main/004A1758.inc"
};
char D_004A1770[0x20] = {
#include "sub_main/004A1770.inc"
};
char D_004A1790[0x20] = {
#include "sub_main/004A1790.inc"
};
char D_004A17B0[0x20] = {
#include "sub_main/004A17B0.inc"
};
char D_004A17D0[0x20] = {
#include "sub_main/004A17D0.inc"
};
char D_004A17F0[0x20] = {
#include "sub_main/004A17F0.inc"
};
char D_004A1810[0x18] = {
#include "sub_main/004A1810.inc"
};
char D_004A1828[15] = {
#include "sub_main/004A1828.inc"
};
char D_004A1838[0xC] = {
#include "sub_main/004A1838.inc"
};
char D_004A1848[0x8] = {
#include "sub_main/004A1848.inc"
};
char D_004A1850[0x10] = {
#include "sub_main/004A1850.inc"
};
char D_004A1860[0x18] = {
#include "sub_main/004A1860.inc"
};
char D_004A1878[0xC] = {
#include "sub_main/004A1878.inc"
};
char D_004A1888[13] = {
#include "sub_main/004A1888.inc"
};
char D_004A1898[15] = {
#include "sub_main/004A1898.inc"
};
char D_004A18A8[0xC] = {
#include "sub_main/004A18A8.inc"
};
char D_004A18B8[13] = {
#include "sub_main/004A18B8.inc"
};
char D_004A18D0[0x18] = {
#include "sub_main/004A18D0.inc"
};
char D_004A18E8[13] = {
#include "sub_main/004A18E8.inc"
};
char D_004A1900[0x18] = {
#include "sub_main/004A1900.inc"
};
char D_004A1918[15] = {
#include "sub_main/004A1918.inc"
};
char D_004A1928[0xC] = {
#include "sub_main/004A1928.inc"
};
char D_004A1938[13] = {
#include "sub_main/004A1938.inc"
};
char D_004A1948[0x8] = {
#include "sub_main/004A1948.inc"
};
char D_004A1950[0x20] = {
#include "sub_main/004A1950.inc"
};
char D_004A1970[0x20] = {
#include "sub_main/004A1970.inc"
};
char D_004A1990[0x20] = {
#include "sub_main/004A1990.inc"
};
char D_004A19B0[0x20] = {
#include "sub_main/004A19B0.inc"
};
char D_004A19D0[0x18] = {
#include "sub_main/004A19D0.inc"
};
char D_004A19E8[13] = {
#include "sub_main/004A19E8.inc"
};
char D_004A19F8[15] = {
#include "sub_main/004A19F8.inc"
};
char D_004A1A10[0x18] = {
#include "sub_main/004A1A10.inc"
};
char D_004A1A28[13] = {
#include "sub_main/004A1A28.inc"
};
char D_004A1A38[13] = {
#include "sub_main/004A1A38.inc"
};
char D_004A1A50[0x20] = {
#include "sub_main/004A1A50.inc"
};
char D_004A1A70[0x18] = {
#include "sub_main/004A1A70.inc"
};
char D_004A1A88[0x8] = {
#include "sub_main/004A1A88.inc"
};
char D_004A1A90[0x10] = {
#include "sub_main/004A1A90.inc"
};
char D_004A1AA0[0x10] = {
#include "sub_main/004A1AA0.inc"
};
char D_004A1AB0[0x10] = {
#include "sub_main/004A1AB0.inc"
};
char D_004A1AC0[0x18] = {
#include "sub_main/004A1AC0.inc"
};
char D_004A1AD8[0x8] = {
#include "sub_main/004A1AD8.inc"
};
char D_004A1AE0[0x10] = {
#include "sub_main/004A1AE0.inc"
};
char D_004A1AF0[0x10] = {
#include "sub_main/004A1AF0.inc"
};
char D_004A1B00[0x10] = {
#include "sub_main/004A1B00.inc"
};
char D_004A1B10[0x20] = {
#include "sub_main/004A1B10.inc"
};
char D_004A1B30[0x20] = {
#include "sub_main/004A1B30.inc"
};
char D_004A1B50[0x18] = {
#include "sub_main/004A1B50.inc"
};
char D_004A1B68[15] = {
#include "sub_main/004A1B68.inc"
};
char D_004A1B78[0xC] = {
#include "sub_main/004A1B78.inc"
};
char D_004A1B90[0x20] = {
#include "sub_main/004A1B90.inc"
};
char D_004A1BB0[0x18] = {
#include "sub_main/004A1BB0.inc"
};
char D_004A1BC8[13] = {
#include "sub_main/004A1BC8.inc"
};
char D_004A1BD8[15] = {
#include "sub_main/004A1BD8.inc"
};
char D_004A1BE8[13] = {
#include "sub_main/004A1BE8.inc"
};
char D_004A1BF8[0xC] = {
#include "sub_main/004A1BF8.inc"
};
char D_004A1C08[0x8] = {
#include "sub_main/004A1C08.inc"
};
char D_004A1C10[0x10] = {
#include "sub_main/004A1C10.inc"
};
char D_004A1C20[0x20] = {
#include "sub_main/004A1C20.inc"
};
char D_004A1C40[0x18] = {
#include "sub_main/004A1C40.inc"
};
char D_004A1C58[0xC] = {
#include "sub_main/004A1C58.inc"
};
char D_004A1C70[0x18] = {
#include "sub_main/004A1C70.inc"
};
char D_004A1C88[0x8] = {
#include "sub_main/004A1C88.inc"
};
char D_004A1C90[0x10] = {
#include "sub_main/004A1C90.inc"
};
char D_004A1CA0[0x10] = {
#include "sub_main/004A1CA0.inc"
};
char D_004A1CB0[0x20] = {
#include "sub_main/004A1CB0.inc"
};
char D_004A1CD0[0x18] = {
#include "sub_main/004A1CD0.inc"
};
char D_004A1CE8[15] = {
#include "sub_main/004A1CE8.inc"
};
char D_004A1CF8[15] = {
#include "sub_main/004A1CF8.inc"
};
char D_004A1D08[15] = {
#include "sub_main/004A1D08.inc"
};
char D_004A1D18[15] = {
#include "sub_main/004A1D18.inc"
};
char D_004A1D28[0xC] = {
#include "sub_main/004A1D28.inc"
};
char D_004A1D38[13] = {
#include "sub_main/004A1D38.inc"
};
char D_004A1D48[0xC] = {
#include "sub_main/004A1D48.inc"
};
char D_004A1D60[0x20] = {
#include "sub_main/004A1D60.inc"
};
char D_004A1D80[0x18] = {
#include "sub_main/004A1D80.inc"
};
char D_004A1D98[15] = {
#include "sub_main/004A1D98.inc"
};
char D_004A1DB0[0x20] = {
#include "sub_main/004A1DB0.inc"
};
char D_004A1DD0[0x20] = {
#include "sub_main/004A1DD0.inc"
};
char D_004A1DF0[0x18] = {
#include "sub_main/004A1DF0.inc"
};
char D_004A1E08[15] = {
#include "sub_main/004A1E08.inc"
};
char D_004A1E18[0x8] = {
#include "sub_main/004A1E18.inc"
};
char D_004A1E20[0x10] = {
#include "sub_main/004A1E20.inc"
};
char D_004A1E30[0x8] = {
#include "sub_main/004A1E30.inc"
};
char D_004A1E38[15] = {
#include "sub_main/004A1E38.inc"
};
char D_004A1E50[0x20] = {
#include "sub_main/004A1E50.inc"
};
char D_004A1E70[0x18] = {
#include "sub_main/004A1E70.inc"
};
char D_004A1E88[15] = {
#include "sub_main/004A1E88.inc"
};
char D_004A1EA0[0x20] = {
#include "sub_main/004A1EA0.inc"
};
char D_004A1EC0[0x18] = {
#include "sub_main/004A1EC0.inc"
};
char D_004A1ED8[15] = {
#include "sub_main/004A1ED8.inc"
};
char D_004A1EE8[15] = {
#include "sub_main/004A1EE8.inc"
};
char D_004A1EF8[15] = {
#include "sub_main/004A1EF8.inc"
};
char D_004A1F10[0x18] = {
#include "sub_main/004A1F10.inc"
};
char D_004A1F28[15] = {
#include "sub_main/004A1F28.inc"
};
char D_004A1F38[15] = {
#include "sub_main/004A1F38.inc"
};
char D_004A1F48[13] = {
#include "sub_main/004A1F48.inc"
};
char D_004A1F60[0x20] = {
#include "sub_main/004A1F60.inc"
};
char D_004A1F80[0x20] = {
#include "sub_main/004A1F80.inc"
};
char D_004A1FA0[0x20] = {
#include "sub_main/004A1FA0.inc"
};
char D_004A1FC0[0x18] = {
#include "sub_main/004A1FC0.inc"
};
char D_004A1FD8[13] = {
#include "sub_main/004A1FD8.inc"
};
char D_004A1FF0[0x20] = {
#include "sub_main/004A1FF0.inc"
};
char D_004A2010[0x18] = {
#include "sub_main/004A2010.inc"
};
char D_004A2028[0xC] = {
#include "sub_main/004A2028.inc"
};
char D_004A2038[13] = {
#include "sub_main/004A2038.inc"
};
char D_004A2050[0x20] = {
#include "sub_main/004A2050.inc"
};
char D_004A2070[0x18] = {
#include "sub_main/004A2070.inc"
};
char D_004A2088[0xC] = {
#include "sub_main/004A2088.inc"
};
char D_004A2098[0x8] = {
#include "sub_main/004A2098.inc"
};
char D_004A20A0[0x20] = {
#include "sub_main/004A20A0.inc"
};
char D_004A20C0[0x18] = {
#include "sub_main/004A20C0.inc"
};
char D_004A20D8[0xC] = {
#include "sub_main/004A20D8.inc"
};
char D_004A20F0[0x18] = {
#include "sub_main/004A20F0.inc"
};
char D_004A2108[0xC] = {
#include "sub_main/004A2108.inc"
};
char D_004A2118[15] = {
#include "sub_main/004A2118.inc"
};
char D_004A2128[13] = {
#include "sub_main/004A2128.inc"
};
char D_004A2138[13] = {
#include "sub_main/004A2138.inc"
};
char D_004A2150[0x20] = {
#include "sub_main/004A2150.inc"
};
char D_004A2170[0x20] = {
#include "sub_main/004A2170.inc"
};
char D_004A2190[0x18] = {
#include "sub_main/004A2190.inc"
};
char D_004A21A8[13] = {
#include "sub_main/004A21A8.inc"
};
char D_004A21B8[15] = {
#include "sub_main/004A21B8.inc"
};
char D_004A21D0[0x20] = {
#include "sub_main/004A21D0.inc"
};
char D_004A21F0[0x20] = {
#include "sub_main/004A21F0.inc"
};
char D_004A2210[0x20] = {
#include "sub_main/004A2210.inc"
};
char D_004A2230[0x18] = {
#include "sub_main/004A2230.inc"
};
char D_004A2248[13] = {
#include "sub_main/004A2248.inc"
};
char D_004A2258[0xC] = {
#include "sub_main/004A2258.inc"
};
char D_004A2268[13] = {
#include "sub_main/004A2268.inc"
};
char D_004A2278[15] = {
#include "sub_main/004A2278.inc"
};
char D_004A2288[15] = {
#include "sub_main/004A2288.inc"
};
char D_004A22A0[0x20] = {
#include "sub_main/004A22A0.inc"
};
char D_004A22C0[0x20] = {
#include "sub_main/004A22C0.inc"
};
char D_004A22E0[0x20] = {
#include "sub_main/004A22E0.inc"
};
char D_004A2300[0x18] = {
#include "sub_main/004A2300.inc"
};
char D_004A2318[13] = {
#include "sub_main/004A2318.inc"
};
char D_004A2330[0x18] = {
#include "sub_main/004A2330.inc"
};
char D_004A2348[15] = {
#include "sub_main/004A2348.inc"
};
char D_004A2360[0x20] = {
#include "sub_main/004A2360.inc"
};
char D_004A2380[0x20] = {
#include "sub_main/004A2380.inc"
};
char D_004A23A0[0x20] = {
#include "sub_main/004A23A0.inc"
};
char D_004A23C0[0x20] = {
#include "sub_main/004A23C0.inc"
};
char D_004A23E0[0x18] = {
#include "sub_main/004A23E0.inc"
};
char D_004A23F8[13] = {
#include "sub_main/004A23F8.inc"
};
char D_004A2408[15] = {
#include "sub_main/004A2408.inc"
};
char D_004A2420[0x20] = {
#include "sub_main/004A2420.inc"
};
char D_004A2440[0x20] = {
#include "sub_main/004A2440.inc"
};
char D_004A2460[0x18] = {
#include "sub_main/004A2460.inc"
};
char D_004A2478[15] = {
#include "sub_main/004A2478.inc"
};
char D_004A2488[15] = {
#include "sub_main/004A2488.inc"
};
char D_004A24A0[0x20] = {
#include "sub_main/004A24A0.inc"
};
char D_004A24C0[0x18] = {
#include "sub_main/004A24C0.inc"
};
char D_004A24D8[0xC] = {
#include "sub_main/004A24D8.inc"
};
char D_004A24F0[0x20] = {
#include "sub_main/004A24F0.inc"
};
char D_004A2510[0x20] = {
#include "sub_main/004A2510.inc"
};
char D_004A2530[0x20] = {
#include "sub_main/004A2530.inc"
};
char D_004A2550[0x20] = {
#include "sub_main/004A2550.inc"
};
char D_004A2570[0x20] = {
#include "sub_main/004A2570.inc"
};
char D_004A2590[0x20] = {
#include "sub_main/004A2590.inc"
};
char D_004A25B0[0x20] = {
#include "sub_main/004A25B0.inc"
};
char D_004A25D0[0x20] = {
#include "sub_main/004A25D0.inc"
};
char D_004A25F0[0x20] = {
#include "sub_main/004A25F0.inc"
};
char D_004A2610[0x18] = {
#include "sub_main/004A2610.inc"
};
char D_004A2628[0xC] = {
#include "sub_main/004A2628.inc"
};
char D_004A2640[0x18] = {
#include "sub_main/004A2640.inc"
};
char D_004A2658[0xC] = {
#include "sub_main/004A2658.inc"
};
char D_004A2670[0x20] = {
#include "sub_main/004A2670.inc"
};
char D_004A2690[0x20] = {
#include "sub_main/004A2690.inc"
};
char D_004A26B0[0x18] = {
#include "sub_main/004A26B0.inc"
};
char D_004A26C8[15] = {
#include "sub_main/004A26C8.inc"
};
char D_004A26E0[0x20] = {
#include "sub_main/004A26E0.inc"
};
char D_004A2700[0x20] = {
#include "sub_main/004A2700.inc"
};
char D_004A2720[0x20] = {
#include "sub_main/004A2720.inc"
};
char D_004A2740[0x20] = {
#include "sub_main/004A2740.inc"
};
char D_004A2760[0x20] = {
#include "sub_main/004A2760.inc"
};
char D_004A2780[0x20] = {
#include "sub_main/004A2780.inc"
};
char D_004A27A0[0x20] = {
#include "sub_main/004A27A0.inc"
};
char D_004A27C0[0x18] = {
#include "sub_main/004A27C0.inc"
};
char D_004A27D8[0xC] = {
#include "sub_main/004A27D8.inc"
};
char D_004A27F0[0x18] = {
#include "sub_main/004A27F0.inc"
};
char D_004A2808[13] = {
#include "sub_main/004A2808.inc"
};
char D_004A2820[0x18] = {
#include "sub_main/004A2820.inc"
};
char D_004A2838[0xC] = {
#include "sub_main/004A2838.inc"
};
char D_004A2848[15] = {
#include "sub_main/004A2848.inc"
};
char D_004A2860[0x18] = {
#include "sub_main/004A2860.inc"
};
char D_004A2878[13] = {
#include "sub_main/004A2878.inc"
};
char D_004A2888[0xC] = {
#include "sub_main/004A2888.inc"
};
char D_004A28A0[0x20] = {
#include "sub_main/004A28A0.inc"
};
char D_004A28C0[0x20] = {
#include "sub_main/004A28C0.inc"
};
char D_004A28E0[0x18] = {
#include "sub_main/004A28E0.inc"
};
char D_004A28F8[15] = {
#include "sub_main/004A28F8.inc"
};
char D_004A2908[13] = {
#include "sub_main/004A2908.inc"
};
char D_004A2920[0x20] = {
#include "sub_main/004A2920.inc"
};
char D_004A2940[0x18] = {
#include "sub_main/004A2940.inc"
};
char D_004A2958[0xC] = {
#include "sub_main/004A2958.inc"
};
char D_004A2968[15] = {
#include "sub_main/004A2968.inc"
};
char D_004A2978[0xC] = {
#include "sub_main/004A2978.inc"
};
char D_004A2988[15] = {
#include "sub_main/004A2988.inc"
};
char D_004A29A0[0x20] = {
#include "sub_main/004A29A0.inc"
};
char D_004A29C0[0x18] = {
#include "sub_main/004A29C0.inc"
};
char D_004A29D8[0x8] = {
#include "sub_main/004A29D8.inc"
};
char D_004A29E0[0x10] = {
#include "sub_main/004A29E0.inc"
};
char D_004A29F0[0x20] = {
#include "sub_main/004A29F0.inc"
};
char D_004A2A10[0x18] = {
#include "sub_main/004A2A10.inc"
};
char D_004A2A28[0xC] = {
#include "sub_main/004A2A28.inc"
};
char D_004A2A40[0x20] = {
#include "sub_main/004A2A40.inc"
};
char D_004A2A60[0x20] = {
#include "sub_main/004A2A60.inc"
};
char D_004A2A80[0x18] = {
#include "sub_main/004A2A80.inc"
};
char D_004A2A98[0x8] = {
#include "sub_main/004A2A98.inc"
};
char D_004A2AA0[0x18] = {
#include "sub_main/004A2AA0.inc"
};
char D_004A2AB8[13] = {
#include "sub_main/004A2AB8.inc"
};
char D_004A2AC8[13] = {
#include "sub_main/004A2AC8.inc"
};
char D_004A2AE0[0x20] = {
#include "sub_main/004A2AE0.inc"
};
char D_004A2B00[0x18] = {
#include "sub_main/004A2B00.inc"
};
char D_004A2B18[15] = {
#include "sub_main/004A2B18.inc"
};
char D_004A2B28[13] = {
#include "sub_main/004A2B28.inc"
};
char D_004A2B38[0x8] = {
#include "sub_main/004A2B38.inc"
};
char D_004A2B40[0x10] = {
#include "sub_main/004A2B40.inc"
};
char D_004A2B50[0x20] = {
#include "sub_main/004A2B50.inc"
};
char D_004A2B70[0x20] = {
#include "sub_main/004A2B70.inc"
};
char D_004A2B90[0x18] = {
#include "sub_main/004A2B90.inc"
};
char D_004A2BA8[13] = {
#include "sub_main/004A2BA8.inc"
};
char D_004A2BB8[13] = {
#include "sub_main/004A2BB8.inc"
};
char D_004A2BC8[15] = {
#include "sub_main/004A2BC8.inc"
};
char D_004A2BE0[0x18] = {
#include "sub_main/004A2BE0.inc"
};
char D_004A2BF8[15] = {
#include "sub_main/004A2BF8.inc"
};
char D_004A2C10[0x20] = {
#include "sub_main/004A2C10.inc"
};
char D_004A2C30[0x20] = {
#include "sub_main/004A2C30.inc"
};
char D_004A2C50[0x20] = {
#include "sub_main/004A2C50.inc"
};
char D_004A2C70[0x20] = {
#include "sub_main/004A2C70.inc"
};
char D_004A2C90[0x20] = {
#include "sub_main/004A2C90.inc"
};
char D_004A2CB0[0x20] = {
#include "sub_main/004A2CB0.inc"
};
char D_004A2CD0[0x20] = {
#include "sub_main/004A2CD0.inc"
};
char D_004A2CF0[0x20] = {
#include "sub_main/004A2CF0.inc"
};
char D_004A2D10[0x18] = {
#include "sub_main/004A2D10.inc"
};
char D_004A2D28[13] = {
#include "sub_main/004A2D28.inc"
};
char D_004A2D38[15] = {
#include "sub_main/004A2D38.inc"
};
char D_004A2D48[13] = {
#include "sub_main/004A2D48.inc"
};
char D_004A2D60[0x20] = {
#include "sub_main/004A2D60.inc"
};
char D_004A2D80[0x18] = {
#include "sub_main/004A2D80.inc"
};
char D_004A2D98[15] = {
#include "sub_main/004A2D98.inc"
};
char D_004A2DB0[0x18] = {
#include "sub_main/004A2DB0.inc"
};
char D_004A2DC8[13] = {
#include "sub_main/004A2DC8.inc"
};
char D_004A2DD8[0xC] = {
#include "sub_main/004A2DD8.inc"
};
char D_004A2DF0[0x18] = {
#include "sub_main/004A2DF0.inc"
};
char D_004A2E08[15] = {
#include "sub_main/004A2E08.inc"
};
char D_004A2E18[0x8] = {
#include "sub_main/004A2E18.inc"
};
char D_004A2E20[0x18] = {
#include "sub_main/004A2E20.inc"
};
char D_004A2E38[15] = {
#include "sub_main/004A2E38.inc"
};
char D_004A2E50[0x20] = {
#include "sub_main/004A2E50.inc"
};
char D_004A2E70[0x20] = {
#include "sub_main/004A2E70.inc"
};
char D_004A2E90[0x20] = {
#include "sub_main/004A2E90.inc"
};
char D_004A2EB0[0x20] = {
#include "sub_main/004A2EB0.inc"
};
char D_004A2ED0[0x20] = {
#include "sub_main/004A2ED0.inc"
};
char D_004A2EF0[0x20] = {
#include "sub_main/004A2EF0.inc"
};
char D_004A2F10[0x20] = {
#include "sub_main/004A2F10.inc"
};
char D_004A2F30[0x20] = {
#include "sub_main/004A2F30.inc"
};
char D_004A2F50[0x20] = {
#include "sub_main/004A2F50.inc"
};
char D_004A2F70[0x18] = {
#include "sub_main/004A2F70.inc"
};
char D_004A2F88[0xC] = {
#include "sub_main/004A2F88.inc"
};
char D_004A2F98[0xC] = {
#include "sub_main/004A2F98.inc"
};
char D_004A2FB0[0x18] = {
#include "sub_main/004A2FB0.inc"
};
char D_004A2FC8[15] = {
#include "sub_main/004A2FC8.inc"
};
char D_004A2FD8[0xC] = {
#include "sub_main/004A2FD8.inc"
};
char D_004A2FF0[0x20] = {
#include "sub_main/004A2FF0.inc"
};
char D_004A3010[0x20] = {
#include "sub_main/004A3010.inc"
};
char D_004A3030[0x20] = {
#include "sub_main/004A3030.inc"
};
char D_004A3050[0x20] = {
#include "sub_main/004A3050.inc"
};
char D_004A3070[0x18] = {
#include "sub_main/004A3070.inc"
};
char D_004A3088[15] = {
#include "sub_main/004A3088.inc"
};
char D_004A30A0[0x20] = {
#include "sub_main/004A30A0.inc"
};
char D_004A30C0[0x20] = {
#include "sub_main/004A30C0.inc"
};
char D_004A30E0[0x20] = {
#include "sub_main/004A30E0.inc"
};
char D_004A3100[0x20] = {
#include "sub_main/004A3100.inc"
};
char D_004A3120[0x18] = {
#include "sub_main/004A3120.inc"
};
char D_004A3138[15] = {
#include "sub_main/004A3138.inc"
};
char D_004A3148[13] = {
#include "sub_main/004A3148.inc"
};
char D_004A3160[0x20] = {
#include "sub_main/004A3160.inc"
};
char D_004A3180[0x18] = {
#include "sub_main/004A3180.inc"
};
char D_004A3198[0x8] = {
#include "sub_main/004A3198.inc"
};
char D_004A31A0[0x20] = {
#include "sub_main/004A31A0.inc"
};
char D_004A31C0[0x20] = {
#include "sub_main/004A31C0.inc"
};
char D_004A31E0[0x20] = {
#include "sub_main/004A31E0.inc"
};
char D_004A3200[0x18] = {
#include "sub_main/004A3200.inc"
};
char D_004A3218[15] = {
#include "sub_main/004A3218.inc"
};
char D_004A3228[15] = {
#include "sub_main/004A3228.inc"
};
char D_004A3240[0x20] = {
#include "sub_main/004A3240.inc"
};
char D_004A3260[0x20] = {
#include "sub_main/004A3260.inc"
};
char D_004A3280[0x20] = {
#include "sub_main/004A3280.inc"
};
char D_004A32A0[0x20] = {
#include "sub_main/004A32A0.inc"
};
char D_004A32C0[0x18] = {
#include "sub_main/004A32C0.inc"
};
char D_004A32D8[15] = {
#include "sub_main/004A32D8.inc"
};
char D_004A32E8[15] = {
#include "sub_main/004A32E8.inc"
};
char D_004A3300[0x18] = {
#include "sub_main/004A3300.inc"
};
char D_004A3318[15] = {
#include "sub_main/004A3318.inc"
};
char D_004A3328[13] = {
#include "sub_main/004A3328.inc"
};
char D_004A3338[13] = {
#include "sub_main/004A3338.inc"
};
char D_004A3348[15] = {
#include "sub_main/004A3348.inc"
};
char D_004A3360[0x50] = {
#include "sub_main/004A3360.inc"
};
char D_004A33B0[0x20] = {
#include "sub_main/004A33B0.inc"
};
char D_004A33D0[0x20] = {
#include "sub_main/004A33D0.inc"
};
char D_004A33F0[0x20] = {
#include "sub_main/004A33F0.inc"
};
char D_004A3410[0x30] = {
#include "sub_main/004A3410.inc"
};
char D_004A3440[0x18] = {
#include "sub_main/004A3440.inc"
};
char D_004A3458[13] = {
#include "sub_main/004A3458.inc"
};
char D_004A3470[0x18] = {
#include "sub_main/004A3470.inc"
};
char D_004A3488[13] = {
#include "sub_main/004A3488.inc"
};
char D_004A34A0[0x20] = {
#include "sub_main/004A34A0.inc"
};
char D_004A34C0[0x18] = {
#include "sub_main/004A34C0.inc"
};
char D_004A34D8[15] = {
#include "sub_main/004A34D8.inc"
};
char D_004A34E8[0xC] = {
#include "sub_main/004A34E8.inc"
};
char D_004A3500[0x18] = {
#include "sub_main/004A3500.inc"
};
char D_004A3518[15] = {
#include "sub_main/004A3518.inc"
};
char D_004A3528[0x8] = {
#include "sub_main/004A3528.inc"
};
char D_004A3530[0x10] = {
#include "sub_main/004A3530.inc"
};
char D_004A3540[0x18] = {
#include "sub_main/004A3540.inc"
};
char D_004A3558[15] = {
#include "sub_main/004A3558.inc"
};
char D_004A3568[13] = {
#include "sub_main/004A3568.inc"
};
char D_004A3578[15] = {
#include "sub_main/004A3578.inc"
};
char D_004A3590[0x18] = {
#include "sub_main/004A3590.inc"
};
char D_004A35A8[15] = {
#include "sub_main/004A35A8.inc"
};
char D_004A35C0[0x20] = {
#include "sub_main/004A35C0.inc"
};
char D_004A35E0[0x18] = {
#include "sub_main/004A35E0.inc"
};
char D_004A35F8[0xC] = {
#include "sub_main/004A35F8.inc"
};
char D_004A3608[13] = {
#include "sub_main/004A3608.inc"
};
char D_004A3620[0x60] = {
#include "sub_main/004A3620.inc"
};
char D_004A3680[0x20] = {
#include "sub_main/004A3680.inc"
};
char D_004A36A0[0x20] = {
#include "sub_main/004A36A0.inc"
};
char D_004A36C0[0x20] = {
#include "sub_main/004A36C0.inc"
};
char D_004A36E0[0x40] = {
#include "sub_main/004A36E0.inc"
};
char D_004A3720[0x18] = {
#include "sub_main/004A3720.inc"
};
char D_004A3738[13] = {
#include "sub_main/004A3738.inc"
};
char D_004A3750[0x20] = {
#include "sub_main/004A3750.inc"
};
char D_004A3770[0x18] = {
#include "sub_main/004A3770.inc"
};
char D_004A3788[0x8] = {
#include "sub_main/004A3788.inc"
};
char D_004A3790[0x10] = {
#include "sub_main/004A3790.inc"
};
char D_004A37A0[0x10] = {
#include "sub_main/004A37A0.inc"
};
char D_004A37B0[0x10] = {
#include "sub_main/004A37B0.inc"
};
char D_004A37C0[0x18] = {
#include "sub_main/004A37C0.inc"
};
char D_004A37D8[15] = {
#include "sub_main/004A37D8.inc"
};
char D_004A37F0[0x18] = {
#include "sub_main/004A37F0.inc"
};
char D_004A3808[15] = {
#include "sub_main/004A3808.inc"
};
char D_004A3818[15] = {
#include "sub_main/004A3818.inc"
};
char D_004A3830[0x18] = {
#include "sub_main/004A3830.inc"
};
char D_004A3848[13] = {
#include "sub_main/004A3848.inc"
};
char D_004A3858[15] = {
#include "sub_main/004A3858.inc"
};
char D_004A3868[15] = {
#include "sub_main/004A3868.inc"
};
char D_004A3880[0x18] = {
#include "sub_main/004A3880.inc"
};
char D_004A3898[13] = {
#include "sub_main/004A3898.inc"
};
char D_004A38A8[15] = {
#include "sub_main/004A38A8.inc"
};
char D_004A38B8[13] = {
#include "sub_main/004A38B8.inc"
};
char D_004A38C8[15] = {
#include "sub_main/004A38C8.inc"
};
char D_004A38D8[13] = {
#include "sub_main/004A38D8.inc"
};
char D_004A38E8[15] = {
#include "sub_main/004A38E8.inc"
};
char D_004A38F8[15] = {
#include "sub_main/004A38F8.inc"
};
char D_004A3910[0x18] = {
#include "sub_main/004A3910.inc"
};
char D_004A3928[15] = {
#include "sub_main/004A3928.inc"
};
char D_004A3940[0x18] = {
#include "sub_main/004A3940.inc"
};
char D_004A3958[15] = {
#include "sub_main/004A3958.inc"
};
char D_004A3970[0x18] = {
#include "sub_main/004A3970.inc"
};
char D_004A3988[15] = {
#include "sub_main/004A3988.inc"
};
char D_004A39A0[0x18] = {
#include "sub_main/004A39A0.inc"
};
char D_004A39B8[15] = {
#include "sub_main/004A39B8.inc"
};
char D_004A39D0[0x18] = {
#include "sub_main/004A39D0.inc"
};
char D_004A39E8[13] = {
#include "sub_main/004A39E8.inc"
};
char D_004A39F8[15] = {
#include "sub_main/004A39F8.inc"
};
char D_004A3A08[15] = {
#include "sub_main/004A3A08.inc"
};
char D_004A3A20[0x18] = {
#include "sub_main/004A3A20.inc"
};
char D_004A3A38[13] = {
#include "sub_main/004A3A38.inc"
};
char D_004A3A48[15] = {
#include "sub_main/004A3A48.inc"
};
char D_004A3A58[13] = {
#include "sub_main/004A3A58.inc"
};
char D_004A3A68[15] = {
#include "sub_main/004A3A68.inc"
};
char D_004A3A80[0x18] = {
#include "sub_main/004A3A80.inc"
};
char D_004A3A98[13] = {
#include "sub_main/004A3A98.inc"
};
char D_004A3AA8[15] = {
#include "sub_main/004A3AA8.inc"
};
char D_004A3AC0[0x20] = {
#include "sub_main/004A3AC0.inc"
};
char D_004A3AE0[0x20] = {
#include "sub_main/004A3AE0.inc"
};
char D_004A3B00[0x20] = {
#include "sub_main/004A3B00.inc"
};
char D_004A3B20[0x18] = {
#include "sub_main/004A3B20.inc"
};
char D_004A3B38[15] = {
#include "sub_main/004A3B38.inc"
};
char D_004A3B50[0x18] = {
#include "sub_main/004A3B50.inc"
};
char D_004A3B68[15] = {
#include "sub_main/004A3B68.inc"
};
char D_004A3B78[13] = {
#include "sub_main/004A3B78.inc"
};
char D_004A3B90[0x18] = {
#include "sub_main/004A3B90.inc"
};
char D_004A3BA8[0xC] = {
#include "sub_main/004A3BA8.inc"
};
char D_004A3BC0[0x18] = {
#include "sub_main/004A3BC0.inc"
};
char D_004A3BD8[15] = {
#include "sub_main/004A3BD8.inc"
};
char D_004A3BF0[0x20] = {
#include "sub_main/004A3BF0.inc"
};
char D_004A3C10[0x20] = {
#include "sub_main/004A3C10.inc"
};
char D_004A3C30[0x20] = {
#include "sub_main/004A3C30.inc"
};
char D_004A3C50[0x20] = {
#include "sub_main/004A3C50.inc"
};
char D_004A3C70[0x18] = {
#include "sub_main/004A3C70.inc"
};
char D_004A3C88[13] = {
#include "sub_main/004A3C88.inc"
};
char D_004A3C98[13] = {
#include "sub_main/004A3C98.inc"
};
char D_004A3CA8[15] = {
#include "sub_main/004A3CA8.inc"
};
char D_004A3CB8[15] = {
#include "sub_main/004A3CB8.inc"
};
char D_004A3CC8[0xC] = {
#include "sub_main/004A3CC8.inc"
};
char D_004A3CD8[0xC] = {
#include "sub_main/004A3CD8.inc"
};
char D_004A3CF0[0x20] = {
#include "sub_main/004A3CF0.inc"
};
char D_004A3D10[0x18] = {
#include "sub_main/004A3D10.inc"
};
char D_004A3D28[15] = {
#include "sub_main/004A3D28.inc"
};
char D_004A3D38[15] = {
#include "sub_main/004A3D38.inc"
};
char D_004A3D48[15] = {
#include "sub_main/004A3D48.inc"
};
char D_004A3D58[15] = {
#include "sub_main/004A3D58.inc"
};
char D_004A3D70[0x20] = {
#include "sub_main/004A3D70.inc"
};
char D_004A3D90[0x18] = {
#include "sub_main/004A3D90.inc"
};
char D_004A3DA8[0xC] = {
#include "sub_main/004A3DA8.inc"
};
char D_004A3DB8[15] = {
#include "sub_main/004A3DB8.inc"
};
char D_004A3DC8[0xC] = {
#include "sub_main/004A3DC8.inc"
};
char D_004A3DD8[0xC] = {
#include "sub_main/004A3DD8.inc"
};
char D_004A3DE8[15] = {
#include "sub_main/004A3DE8.inc"
};
char D_004A3E00[0x20] = {
#include "sub_main/004A3E00.inc"
};
char D_004A3E20[0x20] = {
#include "sub_main/004A3E20.inc"
};
char D_004A3E40[0x20] = {
#include "sub_main/004A3E40.inc"
};
char D_004A3E60[0x20] = {
#include "sub_main/004A3E60.inc"
};
char D_004A3E80[0x18] = {
#include "sub_main/004A3E80.inc"
};
char D_004A3E98[15] = {
#include "sub_main/004A3E98.inc"
};
char D_004A3EB0[0x20] = {
#include "sub_main/004A3EB0.inc"
};
char D_004A3ED0[0x20] = {
#include "sub_main/004A3ED0.inc"
};
char D_004A3EF0[0x18] = {
#include "sub_main/004A3EF0.inc"
};
char D_004A3F08[15] = {
#include "sub_main/004A3F08.inc"
};
char D_004A3F20[0x18] = {
#include "sub_main/004A3F20.inc"
};
char D_004A3F38[15] = {
#include "sub_main/004A3F38.inc"
};
char D_004A3F50[0x18] = {
#include "sub_main/004A3F50.inc"
};
char D_004A3F68[15] = {
#include "sub_main/004A3F68.inc"
};
char D_004A3F80[0x20] = {
#include "sub_main/004A3F80.inc"
};
char D_004A3FA0[0x20] = {
#include "sub_main/004A3FA0.inc"
};
char D_004A3FC0[0x20] = {
#include "sub_main/004A3FC0.inc"
};
char D_004A3FE0[0x20] = {
#include "sub_main/004A3FE0.inc"
};
char D_004A4000[0x20] = {
#include "sub_main/004A4000.inc"
};
char D_004A4020[0x20] = {
#include "sub_main/004A4020.inc"
};
char D_004A4040[0x20] = {
#include "sub_main/004A4040.inc"
};
char D_004A4060[0x20] = {
#include "sub_main/004A4060.inc"
};
char D_004A4080[0x20] = {
#include "sub_main/004A4080.inc"
};
char D_004A40A0[0x20] = {
#include "sub_main/004A40A0.inc"
};
char D_004A40C0[0x20] = {
#include "sub_main/004A40C0.inc"
};
char D_004A40E0[0x20] = {
#include "sub_main/004A40E0.inc"
};
char D_004A4100[0x20] = {
#include "sub_main/004A4100.inc"
};
char D_004A4120[0x20] = {
#include "sub_main/004A4120.inc"
};
char D_004A4140[0x20] = {
#include "sub_main/004A4140.inc"
};
char D_004A4160[0x20] = {
#include "sub_main/004A4160.inc"
};
char D_004A4180[0x18] = {
#include "sub_main/004A4180.inc"
};
char D_004A4198[15] = {
#include "sub_main/004A4198.inc"
};
char D_004A41B0[0x18] = {
#include "sub_main/004A41B0.inc"
};
char D_004A41C8[15] = {
#include "sub_main/004A41C8.inc"
};
char D_004A41E0[0x20] = {
#include "sub_main/004A41E0.inc"
};
char D_004A4200[0x20] = {
#include "sub_main/004A4200.inc"
};
char D_004A4220[0x20] = {
#include "sub_main/004A4220.inc"
};
char D_004A4240[0x20] = {
#include "sub_main/004A4240.inc"
};
char D_004A4260[0x20] = {
#include "sub_main/004A4260.inc"
};
char D_004A4280[0x20] = {
#include "sub_main/004A4280.inc"
};
char D_004A42A0[0x18] = {
#include "sub_main/004A42A0.inc"
};
char D_004A42B8[0xC] = {
#include "sub_main/004A42B8.inc"
};
char D_004A42C8[0xC] = {
#include "sub_main/004A42C8.inc"
};
char D_004A42E0[0x20] = {
#include "sub_main/004A42E0.inc"
};
char D_004A4300[0x20] = {
#include "sub_main/004A4300.inc"
};
char D_004A4320[0x20] = {
#include "sub_main/004A4320.inc"
};
char D_004A4340[0x18] = {
#include "sub_main/004A4340.inc"
};
char D_004A4358[15] = {
#include "sub_main/004A4358.inc"
};
char D_004A4370[0x18] = {
#include "sub_main/004A4370.inc"
};
char D_004A4388[15] = {
#include "sub_main/004A4388.inc"
};
char D_004A4398[15] = {
#include "sub_main/004A4398.inc"
};
char D_004A43B0[0x20] = {
#include "sub_main/004A43B0.inc"
};
char D_004A43D0[0x20] = {
#include "sub_main/004A43D0.inc"
};
char D_004A43F0[0x20] = {
#include "sub_main/004A43F0.inc"
};
char D_004A4410[0x20] = {
#include "sub_main/004A4410.inc"
};
char D_004A4430[0x20] = {
#include "sub_main/004A4430.inc"
};
char D_004A4450[0x20] = {
#include "sub_main/004A4450.inc"
};
char D_004A4470[0x20] = {
#include "sub_main/004A4470.inc"
};
char D_004A4490[0x20] = {
#include "sub_main/004A4490.inc"
};
char D_004A44B0[0x20] = {
#include "sub_main/004A44B0.inc"
};
char D_004A44D0[0x18] = {
#include "sub_main/004A44D0.inc"
};
char D_004A44E8[15] = {
#include "sub_main/004A44E8.inc"
};
char D_004A4500[0x20] = {
#include "sub_main/004A4500.inc"
};
char D_004A4520[0x20] = {
#include "sub_main/004A4520.inc"
};
char D_004A4540[0x18] = {
#include "sub_main/004A4540.inc"
};
char D_004A4558[15] = {
#include "sub_main/004A4558.inc"
};
char D_004A4570[0x18] = {
#include "sub_main/004A4570.inc"
};
char D_004A4588[15] = {
#include "sub_main/004A4588.inc"
};
char D_004A45A0[0x18] = {
#include "sub_main/004A45A0.inc"
};
char D_004A45B8[15] = {
#include "sub_main/004A45B8.inc"
};
char D_004A45D0[0x20] = {
#include "sub_main/004A45D0.inc"
};
char D_004A45F0[0x20] = {
#include "sub_main/004A45F0.inc"
};
char D_004A4610[0x20] = {
#include "sub_main/004A4610.inc"
};
char D_004A4630[0x20] = {
#include "sub_main/004A4630.inc"
};
char D_004A4650[0x18] = {
#include "sub_main/004A4650.inc"
};
char D_004A4668[15] = {
#include "sub_main/004A4668.inc"
};
char D_004A4680[0x20] = {
#include "sub_main/004A4680.inc"
};
char D_004A46A0[0x20] = {
#include "sub_main/004A46A0.inc"
};
char D_004A46C0[0x20] = {
#include "sub_main/004A46C0.inc"
};
char D_004A46E0[0x20] = {
#include "sub_main/004A46E0.inc"
};
char D_004A4700[0x20] = {
#include "sub_main/004A4700.inc"
};
char D_004A4720[0x20] = {
#include "sub_main/004A4720.inc"
};
char D_004A4740[0x18] = {
#include "sub_main/004A4740.inc"
};
char D_004A4758[15] = {
#include "sub_main/004A4758.inc"
};
char D_004A4770[0x20] = {
#include "sub_main/004A4770.inc"
};
char D_004A4790[0x20] = {
#include "sub_main/004A4790.inc"
};
char D_004A47B0[0x18] = {
#include "sub_main/004A47B0.inc"
};
char D_004A47C8[15] = {
#include "sub_main/004A47C8.inc"
};
char D_004A47E0[0x18] = {
#include "sub_main/004A47E0.inc"
};
char D_004A47F8[15] = {
#include "sub_main/004A47F8.inc"
};
char D_004A4810[0x18] = {
#include "sub_main/004A4810.inc"
};
char D_004A4828[15] = {
#include "sub_main/004A4828.inc"
};
char D_004A4840[0x20] = {
#include "sub_main/004A4840.inc"
};
char D_004A4860[0x20] = {
#include "sub_main/004A4860.inc"
};
char D_004A4880[0x20] = {
#include "sub_main/004A4880.inc"
};
char D_004A48A0[0x20] = {
#include "sub_main/004A48A0.inc"
};
char D_004A48C0[0x20] = {
#include "sub_main/004A48C0.inc"
};
char D_004A48E0[0x20] = {
#include "sub_main/004A48E0.inc"
};
char D_004A4900[0x18] = {
#include "sub_main/004A4900.inc"
};
char D_004A4918[0xC] = {
#include "sub_main/004A4918.inc"
};
char D_004A4928[0xC] = {
#include "sub_main/004A4928.inc"
};
char D_004A4940[0x20] = {
#include "sub_main/004A4940.inc"
};
char D_004A4960[0x20] = {
#include "sub_main/004A4960.inc"
};
char D_004A4980[0x20] = {
#include "sub_main/004A4980.inc"
};
char D_004A49A0[0x18] = {
#include "sub_main/004A49A0.inc"
};
char D_004A49B8[15] = {
#include "sub_main/004A49B8.inc"
};
char D_004A49D0[0x18] = {
#include "sub_main/004A49D0.inc"
};
char D_004A49E8[15] = {
#include "sub_main/004A49E8.inc"
};
char D_004A49F8[15] = {
#include "sub_main/004A49F8.inc"
};
char D_004A4A10[0x20] = {
#include "sub_main/004A4A10.inc"
};
char D_004A4A30[0x18] = {
#include "sub_main/004A4A30.inc"
};
char D_004A4A48[13] = {
#include "sub_main/004A4A48.inc"
};
char D_004A4A58[15] = {
#include "sub_main/004A4A58.inc"
};
char D_004A4A68[15] = {
#include "sub_main/004A4A68.inc"
};
char D_004A4A80[0x18] = {
#include "sub_main/004A4A80.inc"
};
char D_004A4A98[15] = {
#include "sub_main/004A4A98.inc"
};
char D_004A4AB0[0x18] = {
#include "sub_main/004A4AB0.inc"
};
char D_004A4AC8[13] = {
#include "sub_main/004A4AC8.inc"
};
char D_004A4AD8[15] = {
#include "sub_main/004A4AD8.inc"
};
char D_004A4AE8[15] = {
#include "sub_main/004A4AE8.inc"
};
char D_004A4B00[0x18] = {
#include "sub_main/004A4B00.inc"
};
char D_004A4B18[15] = {
#include "sub_main/004A4B18.inc"
};
char D_004A4B30[0x18] = {
#include "sub_main/004A4B30.inc"
};
char D_004A4B48[15] = {
#include "sub_main/004A4B48.inc"
};
char D_004A4B60[0x20] = {
#include "sub_main/004A4B60.inc"
};
char D_004A4B80[0x20] = {
#include "sub_main/004A4B80.inc"
};
char D_004A4BA0[0x20] = {
#include "sub_main/004A4BA0.inc"
};
char D_004A4BC0[0x20] = {
#include "sub_main/004A4BC0.inc"
};
char D_004A4BE0[0x18] = {
#include "sub_main/004A4BE0.inc"
};
char D_004A4BF8[13] = {
#include "sub_main/004A4BF8.inc"
};
char D_004A4C08[15] = {
#include "sub_main/004A4C08.inc"
};
char D_004A4C18[15] = {
#include "sub_main/004A4C18.inc"
};
char D_004A4C30[0x18] = {
#include "sub_main/004A4C30.inc"
};
char D_004A4C48[15] = {
#include "sub_main/004A4C48.inc"
};
char D_004A4C60[0x18] = {
#include "sub_main/004A4C60.inc"
};
char D_004A4C78[13] = {
#include "sub_main/004A4C78.inc"
};
char D_004A4C88[13] = {
#include "sub_main/004A4C88.inc"
};
char D_004A4C98[15] = {
#include "sub_main/004A4C98.inc"
};
char D_004A4CA8[15] = {
#include "sub_main/004A4CA8.inc"
};
char D_004A4CB8[15] = {
#include "sub_main/004A4CB8.inc"
};
char D_004A4CC8[15] = {
#include "sub_main/004A4CC8.inc"
};
char D_004A4CE0[0x20] = {
#include "sub_main/004A4CE0.inc"
};
char D_004A4D00[0x20] = {
#include "sub_main/004A4D00.inc"
};
char D_004A4D20[0x20] = {
#include "sub_main/004A4D20.inc"
};
char D_004A4D40[0x18] = {
#include "sub_main/004A4D40.inc"
};
char D_004A4D58[0xC] = {
#include "sub_main/004A4D58.inc"
};
char D_004A4D68[0xC] = {
#include "sub_main/004A4D68.inc"
};
char D_004A4D80[0x18] = {
#include "sub_main/004A4D80.inc"
};
char D_004A4D98[15] = {
#include "sub_main/004A4D98.inc"
};
char D_004A4DA8[13] = {
#include "sub_main/004A4DA8.inc"
};
char D_004A4DB8[13] = {
#include "sub_main/004A4DB8.inc"
};
char D_004A4DC8[0xC] = {
#include "sub_main/004A4DC8.inc"
};
char D_004A4DD8[0xC] = {
#include "sub_main/004A4DD8.inc"
};
char D_004A4DF0[0x20] = {
#include "sub_main/004A4DF0.inc"
};
char D_004A4E10[0x20] = {
#include "sub_main/004A4E10.inc"
};
char D_004A4E30[0x20] = {
#include "sub_main/004A4E30.inc"
};
char D_004A4E50[0x20] = {
#include "sub_main/004A4E50.inc"
};
char D_004A4E70[0x20] = {
#include "sub_main/004A4E70.inc"
};
char D_004A4E90[0x20] = {
#include "sub_main/004A4E90.inc"
};
char D_004A4EB0[0x20] = {
#include "sub_main/004A4EB0.inc"
};
char D_004A4ED0[0x20] = {
#include "sub_main/004A4ED0.inc"
};
char D_004A4EF0[0x18] = {
#include "sub_main/004A4EF0.inc"
};
char D_004A4F08[15] = {
#include "sub_main/004A4F08.inc"
};
char D_004A4F20[0x20] = {
#include "sub_main/004A4F20.inc"
};
char D_004A4F40[0x20] = {
#include "sub_main/004A4F40.inc"
};
char D_004A4F60[0x20] = {
#include "sub_main/004A4F60.inc"
};
char D_004A4F80[0x20] = {
#include "sub_main/004A4F80.inc"
};
char D_004A4FA0[0x18] = {
#include "sub_main/004A4FA0.inc"
};
char D_004A4FB8[0xC] = {
#include "sub_main/004A4FB8.inc"
};
char D_004A4FC8[0xC] = {
#include "sub_main/004A4FC8.inc"
};
char D_004A4FD8[0xC] = {
#include "sub_main/004A4FD8.inc"
};
char D_004A4FE8[13] = {
#include "sub_main/004A4FE8.inc"
};
char D_004A4FF8[13] = {
#include "sub_main/004A4FF8.inc"
};
char D_004A5008[13] = {
#include "sub_main/004A5008.inc"
};
char D_004A5018[13] = {
#include "sub_main/004A5018.inc"
};
char D_004A5028[13] = {
#include "sub_main/004A5028.inc"
};
char D_004A5038[13] = {
#include "sub_main/004A5038.inc"
};
char D_004A5048[13] = {
#include "sub_main/004A5048.inc"
};
char D_004A5058[15] = {
#include "sub_main/004A5058.inc"
};
char D_004A5070[0x18] = {
#include "sub_main/004A5070.inc"
};
char D_004A5088[15] = {
#include "sub_main/004A5088.inc"
};
char D_004A50A0[0x18] = {
#include "sub_main/004A50A0.inc"
};
char D_004A50B8[15] = {
#include "sub_main/004A50B8.inc"
};
char D_004A50C8[15] = {
#include "sub_main/004A50C8.inc"
};
char D_004A50E0[0x18] = {
#include "sub_main/004A50E0.inc"
};
char D_004A50F8[13] = {
#include "sub_main/004A50F8.inc"
};
char D_004A5108[15] = {
#include "sub_main/004A5108.inc"
};
char D_004A5118[15] = {
#include "sub_main/004A5118.inc"
};
char D_004A5130[0x18] = {
#include "sub_main/004A5130.inc"
};
char D_004A5148[13] = {
#include "sub_main/004A5148.inc"
};
char D_004A5158[15] = {
#include "sub_main/004A5158.inc"
};
char D_004A5168[13] = {
#include "sub_main/004A5168.inc"
};
char D_004A5178[15] = {
#include "sub_main/004A5178.inc"
};
char D_004A5188[13] = {
#include "sub_main/004A5188.inc"
};
char D_004A5198[15] = {
#include "sub_main/004A5198.inc"
};
char D_004A51A8[15] = {
#include "sub_main/004A51A8.inc"
};
char D_004A51C0[0x18] = {
#include "sub_main/004A51C0.inc"
};
char D_004A51D8[15] = {
#include "sub_main/004A51D8.inc"
};
char D_004A51F0[0x18] = {
#include "sub_main/004A51F0.inc"
};
char D_004A5208[13] = {
#include "sub_main/004A5208.inc"
};
char D_004A5218[15] = {
#include "sub_main/004A5218.inc"
};
char D_004A5228[15] = {
#include "sub_main/004A5228.inc"
};
char D_004A5240[0x18] = {
#include "sub_main/004A5240.inc"
};
char D_004A5258[15] = {
#include "sub_main/004A5258.inc"
};
char D_004A5270[0x18] = {
#include "sub_main/004A5270.inc"
};
char D_004A5288[15] = {
#include "sub_main/004A5288.inc"
};
char D_004A52A0[0x20] = {
#include "sub_main/004A52A0.inc"
};
char D_004A52C0[0x18] = {
#include "sub_main/004A52C0.inc"
};
char D_004A52D8[13] = {
#include "sub_main/004A52D8.inc"
};
char D_004A52E8[15] = {
#include "sub_main/004A52E8.inc"
};
char D_004A52F8[15] = {
#include "sub_main/004A52F8.inc"
};
char D_004A5310[0x18] = {
#include "sub_main/004A5310.inc"
};
char D_004A5328[13] = {
#include "sub_main/004A5328.inc"
};
char D_004A5338[15] = {
#include "sub_main/004A5338.inc"
};
char D_004A5348[13] = {
#include "sub_main/004A5348.inc"
};
char D_004A5358[15] = {
#include "sub_main/004A5358.inc"
};
char D_004A5370[0x20] = {
#include "sub_main/004A5370.inc"
};
char D_004A5390[0x18] = {
#include "sub_main/004A5390.inc"
};
char D_004A53A8[13] = {
#include "sub_main/004A53A8.inc"
};
char D_004A53B8[15] = {
#include "sub_main/004A53B8.inc"
};
char D_004A53D0[0x20] = {
#include "sub_main/004A53D0.inc"
};
char D_004A53F0[0x20] = {
#include "sub_main/004A53F0.inc"
};
char D_004A5410[0x20] = {
#include "sub_main/004A5410.inc"
};
char D_004A5430[0x18] = {
#include "sub_main/004A5430.inc"
};
char D_004A5448[15] = {
#include "sub_main/004A5448.inc"
};
char D_004A5460[0x18] = {
#include "sub_main/004A5460.inc"
};
char D_004A5478[15] = {
#include "sub_main/004A5478.inc"
};
char D_004A5488[13] = {
#include "sub_main/004A5488.inc"
};
char D_004A54A0[0x20] = {
#include "sub_main/004A54A0.inc"
};
char D_004A54C0[0x18] = {
#include "sub_main/004A54C0.inc"
};
char D_004A54D8[15] = {
#include "sub_main/004A54D8.inc"
};
char D_004A54F0[0x20] = {
#include "sub_main/004A54F0.inc"
};
char D_004A5510[0x20] = {
#include "sub_main/004A5510.inc"
};
char D_004A5530[0x18] = {
#include "sub_main/004A5530.inc"
};
char D_004A5548[13] = {
#include "sub_main/004A5548.inc"
};
char D_004A5558[13] = {
#include "sub_main/004A5558.inc"
};
char D_004A5570[0x20] = {
#include "sub_main/004A5570.inc"
};
char D_004A5590[0x18] = {
#include "sub_main/004A5590.inc"
};
char D_004A55A8[15] = {
#include "sub_main/004A55A8.inc"
};
char D_004A55B8[15] = {
#include "sub_main/004A55B8.inc"
};
char D_004A55C8[15] = {
#include "sub_main/004A55C8.inc"
};
char D_004A55E0[0x18] = {
#include "sub_main/004A55E0.inc"
};
char D_004A55F8[15] = {
#include "sub_main/004A55F8.inc"
};
char D_004A5608[15] = {
#include "sub_main/004A5608.inc"
};
char D_004A5618[13] = {
#include "sub_main/004A5618.inc"
};
char D_004A5628[13] = {
#include "sub_main/004A5628.inc"
};
char D_004A5638[15] = {
#include "sub_main/004A5638.inc"
};
char D_004A5648[15] = {
#include "sub_main/004A5648.inc"
};
char D_004A5660[0x20] = {
#include "sub_main/004A5660.inc"
};
char D_004A5680[0x20] = {
#include "sub_main/004A5680.inc"
};
char D_004A56A0[0x20] = {
#include "sub_main/004A56A0.inc"
};
char D_004A56C0[0x18] = {
#include "sub_main/004A56C0.inc"
};
char D_004A56D8[15] = {
#include "sub_main/004A56D8.inc"
};
char D_004A56F0[0x20] = {
#include "sub_main/004A56F0.inc"
};
char D_004A5710[0x20] = {
#include "sub_main/004A5710.inc"
};
char D_004A5730[0x18] = {
#include "sub_main/004A5730.inc"
};
char D_004A5748[15] = {
#include "sub_main/004A5748.inc"
};
char D_004A5760[0x18] = {
#include "sub_main/004A5760.inc"
};
char D_004A5778[15] = {
#include "sub_main/004A5778.inc"
};
char D_004A5790[0x18] = {
#include "sub_main/004A5790.inc"
};
char D_004A57A8[15] = {
#include "sub_main/004A57A8.inc"
};
char D_004A57C0[0x20] = {
#include "sub_main/004A57C0.inc"
};
char D_004A57E0[0x20] = {
#include "sub_main/004A57E0.inc"
};
char D_004A5800[0x20] = {
#include "sub_main/004A5800.inc"
};
char D_004A5820[0x20] = {
#include "sub_main/004A5820.inc"
};
char D_004A5840[0x18] = {
#include "sub_main/004A5840.inc"
};
char D_004A5858[15] = {
#include "sub_main/004A5858.inc"
};
char D_004A5870[0x20] = {
#include "sub_main/004A5870.inc"
};
char D_004A5890[0x20] = {
#include "sub_main/004A5890.inc"
};
char D_004A58B0[0x20] = {
#include "sub_main/004A58B0.inc"
};
char D_004A58D0[0x20] = {
#include "sub_main/004A58D0.inc"
};
char D_004A58F0[0x20] = {
#include "sub_main/004A58F0.inc"
};
char D_004A5910[0x20] = {
#include "sub_main/004A5910.inc"
};
char D_004A5930[0x20] = {
#include "sub_main/004A5930.inc"
};
char D_004A5950[0x20] = {
#include "sub_main/004A5950.inc"
};
char D_004A5970[0x20] = {
#include "sub_main/004A5970.inc"
};
char D_004A5990[0x20] = {
#include "sub_main/004A5990.inc"
};
char D_004A59B0[0x20] = {
#include "sub_main/004A59B0.inc"
};
char D_004A59D0[0x20] = {
#include "sub_main/004A59D0.inc"
};
char D_004A59F0[0x20] = {
#include "sub_main/004A59F0.inc"
};
char D_004A5A10[0x18] = {
#include "sub_main/004A5A10.inc"
};
char D_004A5A28[15] = {
#include "sub_main/004A5A28.inc"
};
char D_004A5A40[0x18] = {
#include "sub_main/004A5A40.inc"
};
char D_004A5A58[15] = {
#include "sub_main/004A5A58.inc"
};
char D_004A5A70[0x20] = {
#include "sub_main/004A5A70.inc"
};
char D_004A5A90[0x20] = {
#include "sub_main/004A5A90.inc"
};
char D_004A5AB0[0x20] = {
#include "sub_main/004A5AB0.inc"
};
char D_004A5AD0[0x20] = {
#include "sub_main/004A5AD0.inc"
};
char D_004A5AF0[0x20] = {
#include "sub_main/004A5AF0.inc"
};
char D_004A5B10[0x20] = {
#include "sub_main/004A5B10.inc"
};
char D_004A5B30[0x18] = {
#include "sub_main/004A5B30.inc"
};
char D_004A5B48[13] = {
#include "sub_main/004A5B48.inc"
};
char D_004A5B58[13] = {
#include "sub_main/004A5B58.inc"
};
char D_004A5B70[0x20] = {
#include "sub_main/004A5B70.inc"
};
char D_004A5B90[0x20] = {
#include "sub_main/004A5B90.inc"
};
char D_004A5BB0[0x20] = {
#include "sub_main/004A5BB0.inc"
};
char D_004A5BD0[0x20] = {
#include "sub_main/004A5BD0.inc"
};
char D_004A5BF0[0x20] = {
#include "sub_main/004A5BF0.inc"
};
char D_004A5C10[0x20] = {
#include "sub_main/004A5C10.inc"
};
char D_004A5C30[0x20] = {
#include "sub_main/004A5C30.inc"
};
char D_004A5C50[0x20] = {
#include "sub_main/004A5C50.inc"
};
char D_004A5C70[0x20] = {
#include "sub_main/004A5C70.inc"
};
char D_004A5C90[0x20] = {
#include "sub_main/004A5C90.inc"
};
char D_004A5CB0[0x20] = {
#include "sub_main/004A5CB0.inc"
};
char D_004A5CD0[0x20] = {
#include "sub_main/004A5CD0.inc"
};
char D_004A5CF0[0x20] = {
#include "sub_main/004A5CF0.inc"
};
char D_004A5D10[0x20] = {
#include "sub_main/004A5D10.inc"
};
char D_004A5D30[0x20] = {
#include "sub_main/004A5D30.inc"
};
char D_004A5D50[0x20] = {
#include "sub_main/004A5D50.inc"
};
char D_004A5D70[0x20] = {
#include "sub_main/004A5D70.inc"
};
char D_004A5D90[0x18] = {
#include "sub_main/004A5D90.inc"
};
char D_004A5DA8[15] = {
#include "sub_main/004A5DA8.inc"
};
char D_004A5DC0[0x20] = {
#include "sub_main/004A5DC0.inc"
};
char D_004A5DE0[0x20] = {
#include "sub_main/004A5DE0.inc"
};
char D_004A5E00[0x18] = {
#include "sub_main/004A5E00.inc"
};
char D_004A5E18[15] = {
#include "sub_main/004A5E18.inc"
};
char D_004A5E30[0x18] = {
#include "sub_main/004A5E30.inc"
};
char D_004A5E48[15] = {
#include "sub_main/004A5E48.inc"
};
char D_004A5E60[0x18] = {
#include "sub_main/004A5E60.inc"
};
char D_004A5E78[15] = {
#include "sub_main/004A5E78.inc"
};
char D_004A5E90[0x20] = {
#include "sub_main/004A5E90.inc"
};
char D_004A5EB0[0x20] = {
#include "sub_main/004A5EB0.inc"
};
char D_004A5ED0[0x20] = {
#include "sub_main/004A5ED0.inc"
};
char D_004A5EF0[0x20] = {
#include "sub_main/004A5EF0.inc"
};
char D_004A5F10[0x18] = {
#include "sub_main/004A5F10.inc"
};
char D_004A5F28[15] = {
#include "sub_main/004A5F28.inc"
};
char D_004A5F40[0x20] = {
#include "sub_main/004A5F40.inc"
};
char D_004A5F60[0x20] = {
#include "sub_main/004A5F60.inc"
};
char D_004A5F80[0x20] = {
#include "sub_main/004A5F80.inc"
};
char D_004A5FA0[0x20] = {
#include "sub_main/004A5FA0.inc"
};
char D_004A5FC0[0x20] = {
#include "sub_main/004A5FC0.inc"
};
char D_004A5FE0[0x20] = {
#include "sub_main/004A5FE0.inc"
};
char D_004A6000[0x20] = {
#include "sub_main/004A6000.inc"
};
char D_004A6020[0x18] = {
#include "sub_main/004A6020.inc"
};
char D_004A6038[15] = {
#include "sub_main/004A6038.inc"
};
char D_004A6050[0x20] = {
#include "sub_main/004A6050.inc"
};
char D_004A6070[0x20] = {
#include "sub_main/004A6070.inc"
};
char D_004A6090[0x18] = {
#include "sub_main/004A6090.inc"
};
char D_004A60A8[15] = {
#include "sub_main/004A60A8.inc"
};
char D_004A60C0[0x18] = {
#include "sub_main/004A60C0.inc"
};
char D_004A60D8[15] = {
#include "sub_main/004A60D8.inc"
};
char D_004A60F0[0x18] = {
#include "sub_main/004A60F0.inc"
};
char D_004A6108[15] = {
#include "sub_main/004A6108.inc"
};
char D_004A6120[0x20] = {
#include "sub_main/004A6120.inc"
};
char D_004A6140[0x20] = {
#include "sub_main/004A6140.inc"
};
char D_004A6160[0x20] = {
#include "sub_main/004A6160.inc"
};
char D_004A6180[0x20] = {
#include "sub_main/004A6180.inc"
};
char D_004A61A0[0x20] = {
#include "sub_main/004A61A0.inc"
};
char D_004A61C0[0x20] = {
#include "sub_main/004A61C0.inc"
};
char D_004A61E0[0x18] = {
#include "sub_main/004A61E0.inc"
};
char D_004A61F8[13] = {
#include "sub_main/004A61F8.inc"
};
char D_004A6208[13] = {
#include "sub_main/004A6208.inc"
};
char D_004A6220[0x20] = {
#include "sub_main/004A6220.inc"
};
char D_004A6240[0x20] = {
#include "sub_main/004A6240.inc"
};
char D_004A6260[0x20] = {
#include "sub_main/004A6260.inc"
};
char D_004A6280[0x20] = {
#include "sub_main/004A6280.inc"
};
char D_004A62A0[0x20] = {
#include "sub_main/004A62A0.inc"
};
char D_004A62C0[0x20] = {
#include "sub_main/004A62C0.inc"
};
char D_004A62E0[0x20] = {
#include "sub_main/004A62E0.inc"
};
char D_004A6300[0x18] = {
#include "sub_main/004A6300.inc"
};
char D_004A6318[13] = {
#include "sub_main/004A6318.inc"
};
char D_004A6328[15] = {
#include "sub_main/004A6328.inc"
};
char D_004A6338[15] = {
#include "sub_main/004A6338.inc"
};
char D_004A6350[0x18] = {
#include "sub_main/004A6350.inc"
};
char D_004A6368[15] = {
#include "sub_main/004A6368.inc"
};
char D_004A6380[0x18] = {
#include "sub_main/004A6380.inc"
};
char D_004A6398[13] = {
#include "sub_main/004A6398.inc"
};
char D_004A63A8[15] = {
#include "sub_main/004A63A8.inc"
};
char D_004A63B8[15] = {
#include "sub_main/004A63B8.inc"
};
char D_004A63D0[0x18] = {
#include "sub_main/004A63D0.inc"
};
char D_004A63E8[15] = {
#include "sub_main/004A63E8.inc"
};
char D_004A6400[0x18] = {
#include "sub_main/004A6400.inc"
};
char D_004A6418[15] = {
#include "sub_main/004A6418.inc"
};
char D_004A6430[0x20] = {
#include "sub_main/004A6430.inc"
};
char D_004A6450[0x20] = {
#include "sub_main/004A6450.inc"
};
char D_004A6470[0x20] = {
#include "sub_main/004A6470.inc"
};
char D_004A6490[0x20] = {
#include "sub_main/004A6490.inc"
};
char D_004A64B0[0x18] = {
#include "sub_main/004A64B0.inc"
};
char D_004A64C8[13] = {
#include "sub_main/004A64C8.inc"
};
char D_004A64D8[15] = {
#include "sub_main/004A64D8.inc"
};
char D_004A64E8[15] = {
#include "sub_main/004A64E8.inc"
};
char D_004A6500[0x18] = {
#include "sub_main/004A6500.inc"
};
char D_004A6518[15] = {
#include "sub_main/004A6518.inc"
};
char D_004A6530[0x18] = {
#include "sub_main/004A6530.inc"
};
char D_004A6548[13] = {
#include "sub_main/004A6548.inc"
};
char D_004A6558[13] = {
#include "sub_main/004A6558.inc"
};
char D_004A6568[15] = {
#include "sub_main/004A6568.inc"
};
char D_004A6578[15] = {
#include "sub_main/004A6578.inc"
};
char D_004A6590[0x18] = {
#include "sub_main/004A6590.inc"
};
char D_004A65A8[15] = {
#include "sub_main/004A65A8.inc"
};
char D_004A65C0[0x20] = {
#include "sub_main/004A65C0.inc"
};
char D_004A65E0[0x20] = {
#include "sub_main/004A65E0.inc"
};
char D_004A6600[0x20] = {
#include "sub_main/004A6600.inc"
};
char D_004A6620[0x18] = {
#include "sub_main/004A6620.inc"
};
char D_004A6638[15] = {
#include "sub_main/004A6638.inc"
};
char D_004A6648[15] = {
#include "sub_main/004A6648.inc"
};
char D_004A6658[15] = {
#include "sub_main/004A6658.inc"
};
char D_004A6670[0x18] = {
#include "sub_main/004A6670.inc"
};
char D_004A6688[13] = {
#include "sub_main/004A6688.inc"
};
char D_004A6698[13] = {
#include "sub_main/004A6698.inc"
};
char D_004A66A8[0xC] = {
#include "sub_main/004A66A8.inc"
};
char D_004A66B8[0xC] = {
#include "sub_main/004A66B8.inc"
};
char D_004A66D0[0x20] = {
#include "sub_main/004A66D0.inc"
};
char D_004A66F0[0x20] = {
#include "sub_main/004A66F0.inc"
};
char D_004A6710[0x20] = {
#include "sub_main/004A6710.inc"
};
char D_004A6730[0x20] = {
#include "sub_main/004A6730.inc"
};
char D_004A6750[0x20] = {
#include "sub_main/004A6750.inc"
};
char D_004A6770[0x20] = {
#include "sub_main/004A6770.inc"
};
char D_004A6790[0x20] = {
#include "sub_main/004A6790.inc"
};
char D_004A67B0[0x18] = {
#include "sub_main/004A67B0.inc"
};
char D_004A67C8[13] = {
#include "sub_main/004A67C8.inc"
};
char D_004A67E0[0x18] = {
#include "sub_main/004A67E0.inc"
};
char D_004A67F8[13] = {
#include "sub_main/004A67F8.inc"
};
char D_004A6810[0x20] = {
#include "sub_main/004A6810.inc"
};
char D_004A6830[0x20] = {
#include "sub_main/004A6830.inc"
};
char D_004A6850[0x18] = {
#include "sub_main/004A6850.inc"
};
char D_004A6868[15] = {
#include "sub_main/004A6868.inc"
};
char D_004A6878[15] = {
#include "sub_main/004A6878.inc"
};
char D_004A6890[0x20] = {
#include "sub_main/004A6890.inc"
};
char D_004A68B0[0x18] = {
#include "sub_main/004A68B0.inc"
};
char D_004A68C8[15] = {
#include "sub_main/004A68C8.inc"
};
char D_004A68D8[15] = {
#include "sub_main/004A68D8.inc"
};
char D_004A68E8[15] = {
#include "sub_main/004A68E8.inc"
};
char D_004A68F8[13] = {
#include "sub_main/004A68F8.inc"
};
char D_004A6908[13] = {
#include "sub_main/004A6908.inc"
};
char D_004A6918[15] = {
#include "sub_main/004A6918.inc"
};
char D_004A6928[15] = {
#include "sub_main/004A6928.inc"
};
char D_004A6938[13] = {
#include "sub_main/004A6938.inc"
};
char D_004A6948[13] = {
#include "sub_main/004A6948.inc"
};
char D_004A6958[13] = {
#include "sub_main/004A6958.inc"
};
char D_004A6968[13] = {
#include "sub_main/004A6968.inc"
};
char D_004A6978[13] = {
#include "sub_main/004A6978.inc"
};
char D_004A6988[13] = {
#include "sub_main/004A6988.inc"
};
char D_004A6998[15] = {
#include "sub_main/004A6998.inc"
};
char D_004A69A8[15] = {
#include "sub_main/004A69A8.inc"
};
char D_004A69B8[15] = {
#include "sub_main/004A69B8.inc"
};
char D_004A69C8[15] = {
#include "sub_main/004A69C8.inc"
};
char D_004A69D8[13] = {
#include "sub_main/004A69D8.inc"
};
char D_004A69E8[13] = {
#include "sub_main/004A69E8.inc"
};
char D_004A69F8[15] = {
#include "sub_main/004A69F8.inc"
};
char D_004A6A08[15] = {
#include "sub_main/004A6A08.inc"
};
char D_004A6A18[15] = {
#include "sub_main/004A6A18.inc"
};
char D_004A6A28[15] = {
#include "sub_main/004A6A28.inc"
};
char D_004A6A38[15] = {
#include "sub_main/004A6A38.inc"
};
char D_004A6A48[15] = {
#include "sub_main/004A6A48.inc"
};
char D_004A6A60[0x18] = {
#include "sub_main/004A6A60.inc"
};
char D_004A6A78[13] = {
#include "sub_main/004A6A78.inc"
};
char D_004A6A88[13] = {
#include "sub_main/004A6A88.inc"
};
char D_004A6A98[15] = {
#include "sub_main/004A6A98.inc"
};
char D_004A6AA8[15] = {
#include "sub_main/004A6AA8.inc"
};
char D_004A6AB8[13] = {
#include "sub_main/004A6AB8.inc"
};
char D_004A6AC8[13] = {
#include "sub_main/004A6AC8.inc"
};
char D_004A6AD8[13] = {
#include "sub_main/004A6AD8.inc"
};
char D_004A6AE8[13] = {
#include "sub_main/004A6AE8.inc"
};
char D_004A6B00[0x20] = {
#include "sub_main/004A6B00.inc"
};
char D_004A6B20[0x18] = {
#include "sub_main/004A6B20.inc"
};
char D_004A6B38[13] = {
#include "sub_main/004A6B38.inc"
};
char D_004A6B48[13] = {
#include "sub_main/004A6B48.inc"
};
char D_004A6B60[0x20] = {
#include "sub_main/004A6B60.inc"
};
char D_004A6B80[0x20] = {
#include "sub_main/004A6B80.inc"
};
char D_004A6BA0[0x20] = {
#include "sub_main/004A6BA0.inc"
};
char D_004A6BC0[0x18] = {
#include "sub_main/004A6BC0.inc"
};
char D_004A6BD8[15] = {
#include "sub_main/004A6BD8.inc"
};
char D_004A6BE8[15] = {
#include "sub_main/004A6BE8.inc"
};
char D_004A6BF8[15] = {
#include "sub_main/004A6BF8.inc"
};
char D_004A6C08[0xC] = {
#include "sub_main/004A6C08.inc"
};
char D_004A6C20[0x20] = {
#include "sub_main/004A6C20.inc"
};
char D_004A6C40[0x20] = {
#include "sub_main/004A6C40.inc"
};
char D_004A6C60[0x20] = {
#include "sub_main/004A6C60.inc"
};
char D_004A6C80[0x18] = {
#include "sub_main/004A6C80.inc"
};
char D_004A6C98[13] = {
#include "sub_main/004A6C98.inc"
};
char D_004A6CA8[13] = {
#include "sub_main/004A6CA8.inc"
};
char D_004A6CC0[0x20] = {
#include "sub_main/004A6CC0.inc"
};
char D_004A6CE0[0x18] = {
#include "sub_main/004A6CE0.inc"
};
char D_004A6CF8[15] = {
#include "sub_main/004A6CF8.inc"
};
char D_004A6D08[15] = {
#include "sub_main/004A6D08.inc"
};
char D_004A6D18[15] = {
#include "sub_main/004A6D18.inc"
};
char D_004A6D28[15] = {
#include "sub_main/004A6D28.inc"
};
char D_004A6D38[13] = {
#include "sub_main/004A6D38.inc"
};
char D_004A6D48[13] = {
#include "sub_main/004A6D48.inc"
};
char D_004A6D58[0xC] = {
#include "sub_main/004A6D58.inc"
};
char D_004A6D68[0xC] = {
#include "sub_main/004A6D68.inc"
};
char D_004A6D78[15] = {
#include "sub_main/004A6D78.inc"
};
char D_004A6D90[0x20] = {
#include "sub_main/004A6D90.inc"
};
char D_004A6DB0[0x20] = {
#include "sub_main/004A6DB0.inc"
};
char D_004A6DD0[0x20] = {
#include "sub_main/004A6DD0.inc"
};
char D_004A6DF0[0x20] = {
#include "sub_main/004A6DF0.inc"
};
char D_004A6E10[0x18] = {
#include "sub_main/004A6E10.inc"
};
char D_004A6E28[15] = {
#include "sub_main/004A6E28.inc"
};
char D_004A6E38[15] = {
#include "sub_main/004A6E38.inc"
};
char D_004A6E50[0x20] = {
#include "sub_main/004A6E50.inc"
};
char D_004A6E70[0x18] = {
#include "sub_main/004A6E70.inc"
};
char D_004A6E88[15] = {
#include "sub_main/004A6E88.inc"
};
char D_004A6E98[15] = {
#include "sub_main/004A6E98.inc"
};
char D_004A6EA8[15] = {
#include "sub_main/004A6EA8.inc"
};
char D_004A6EB8[15] = {
#include "sub_main/004A6EB8.inc"
};
char D_004A6EC8[15] = {
#include "sub_main/004A6EC8.inc"
};
char D_004A6ED8[15] = {
#include "sub_main/004A6ED8.inc"
};
char D_004A6EF0[0x20] = {
#include "sub_main/004A6EF0.inc"
};
char D_004A6F10[0x20] = {
#include "sub_main/004A6F10.inc"
};
char D_004A6F30[0x20] = {
#include "sub_main/004A6F30.inc"
};
char D_004A6F50[0x18] = {
#include "sub_main/004A6F50.inc"
};
char D_004A6F68[15] = {
#include "sub_main/004A6F68.inc"
};
char D_004A6F78[15] = {
#include "sub_main/004A6F78.inc"
};
char D_004A6F90[0x20] = {
#include "sub_main/004A6F90.inc"
};
char D_004A6FB0[0x20] = {
#include "sub_main/004A6FB0.inc"
};
char D_004A6FD0[0x20] = {
#include "sub_main/004A6FD0.inc"
};
char D_004A6FF0[0x20] = {
#include "sub_main/004A6FF0.inc"
};
char D_004A7010[0x20] = {
#include "sub_main/004A7010.inc"
};
char D_004A7030[0x20] = {
#include "sub_main/004A7030.inc"
};
char D_004A7050[0x20] = {
#include "sub_main/004A7050.inc"
};
char D_004A7070[0x20] = {
#include "sub_main/004A7070.inc"
};
char D_004A7090[0x20] = {
#include "sub_main/004A7090.inc"
};
char D_004A70B0[0x20] = {
#include "sub_main/004A70B0.inc"
};
char D_004A70D0[0x20] = {
#include "sub_main/004A70D0.inc"
};
char D_004A70F0[0x20] = {
#include "sub_main/004A70F0.inc"
};
char D_004A7110[0x18] = {
#include "sub_main/004A7110.inc"
};
char D_004A7128[15] = {
#include "sub_main/004A7128.inc"
};
char D_004A7138[15] = {
#include "sub_main/004A7138.inc"
};
char D_004A7148[15] = {
#include "sub_main/004A7148.inc"
};
char D_004A7158[15] = {
#include "sub_main/004A7158.inc"
};
char D_004A7170[0x20] = {
#include "sub_main/004A7170.inc"
};
char D_004A7190[0x20] = {
#include "sub_main/004A7190.inc"
};
char D_004A71B0[0x20] = {
#include "sub_main/004A71B0.inc"
};
char D_004A71D0[0x20] = {
#include "sub_main/004A71D0.inc"
};
char D_004A71F0[0x20] = {
#include "sub_main/004A71F0.inc"
};
char D_004A7210[0x18] = {
#include "sub_main/004A7210.inc"
};
char D_004A7228[13] = {
#include "sub_main/004A7228.inc"
};
char D_004A7238[13] = {
#include "sub_main/004A7238.inc"
};
char D_004A7250[0x20] = {
#include "sub_main/004A7250.inc"
};
char D_004A7270[0x20] = {
#include "sub_main/004A7270.inc"
};
char D_004A7290[0x20] = {
#include "sub_main/004A7290.inc"
};
char D_004A72B0[0x20] = {
#include "sub_main/004A72B0.inc"
};
char D_004A72D0[0x20] = {
#include "sub_main/004A72D0.inc"
};
char D_004A72F0[0x18] = {
#include "sub_main/004A72F0.inc"
};
char D_004A7308[15] = {
#include "sub_main/004A7308.inc"
};
char D_004A7318[15] = {
#include "sub_main/004A7318.inc"
};
char D_004A7330[0x20] = {
#include "sub_main/004A7330.inc"
};
char D_004A7350[0x20] = {
#include "sub_main/004A7350.inc"
};
char D_004A7370[0x20] = {
#include "sub_main/004A7370.inc"
};
char D_004A7390[0x20] = {
#include "sub_main/004A7390.inc"
};
char D_004A73B0[0x20] = {
#include "sub_main/004A73B0.inc"
};
char D_004A73D0[0x20] = {
#include "sub_main/004A73D0.inc"
};
char D_004A73F0[0x20] = {
#include "sub_main/004A73F0.inc"
};
char D_004A7410[0x20] = {
#include "sub_main/004A7410.inc"
};
char D_004A7430[0x20] = {
#include "sub_main/004A7430.inc"
};
char D_004A7450[0x18] = {
#include "sub_main/004A7450.inc"
};
char D_004A7468[15] = {
#include "sub_main/004A7468.inc"
};
char D_004A7478[15] = {
#include "sub_main/004A7478.inc"
};
char D_004A7490[0x20] = {
#include "sub_main/004A7490.inc"
};
char D_004A74B0[0x18] = {
#include "sub_main/004A74B0.inc"
};
char D_004A74C8[15] = {
#include "sub_main/004A74C8.inc"
};
char D_004A74D8[15] = {
#include "sub_main/004A74D8.inc"
};
char D_004A74E8[15] = {
#include "sub_main/004A74E8.inc"
};
char D_004A74F8[15] = {
#include "sub_main/004A74F8.inc"
};
char D_004A7508[15] = {
#include "sub_main/004A7508.inc"
};
char D_004A7518[15] = {
#include "sub_main/004A7518.inc"
};
char D_004A7530[0x20] = {
#include "sub_main/004A7530.inc"
};
char D_004A7550[0x20] = {
#include "sub_main/004A7550.inc"
};
char D_004A7570[0x20] = {
#include "sub_main/004A7570.inc"
};
char D_004A7590[0x18] = {
#include "sub_main/004A7590.inc"
};
char D_004A75A8[15] = {
#include "sub_main/004A75A8.inc"
};
char D_004A75B8[15] = {
#include "sub_main/004A75B8.inc"
};
char D_004A75D0[0x20] = {
#include "sub_main/004A75D0.inc"
};
char D_004A75F0[0x20] = {
#include "sub_main/004A75F0.inc"
};
char D_004A7610[0x20] = {
#include "sub_main/004A7610.inc"
};
char D_004A7630[0x20] = {
#include "sub_main/004A7630.inc"
};
char D_004A7650[0x20] = {
#include "sub_main/004A7650.inc"
};
char D_004A7670[0x20] = {
#include "sub_main/004A7670.inc"
};
char D_004A7690[0x18] = {
#include "sub_main/004A7690.inc"
};
char D_004A76A8[15] = {
#include "sub_main/004A76A8.inc"
};
char D_004A76B8[15] = {
#include "sub_main/004A76B8.inc"
};
char D_004A76D0[0x20] = {
#include "sub_main/004A76D0.inc"
};
char D_004A76F0[0x18] = {
#include "sub_main/004A76F0.inc"
};
char D_004A7708[15] = {
#include "sub_main/004A7708.inc"
};
char D_004A7718[15] = {
#include "sub_main/004A7718.inc"
};
char D_004A7728[15] = {
#include "sub_main/004A7728.inc"
};
char D_004A7738[15] = {
#include "sub_main/004A7738.inc"
};
char D_004A7748[15] = {
#include "sub_main/004A7748.inc"
};
char D_004A7758[15] = {
#include "sub_main/004A7758.inc"
};
char D_004A7770[0x20] = {
#include "sub_main/004A7770.inc"
};
char D_004A7790[0x20] = {
#include "sub_main/004A7790.inc"
};
char D_004A77B0[0x20] = {
#include "sub_main/004A77B0.inc"
};
char D_004A77D0[0x20] = {
#include "sub_main/004A77D0.inc"
};
char D_004A77F0[0x20] = {
#include "sub_main/004A77F0.inc"
};
char D_004A7810[0x18] = {
#include "sub_main/004A7810.inc"
};
char D_004A7828[13] = {
#include "sub_main/004A7828.inc"
};
char D_004A7838[13] = {
#include "sub_main/004A7838.inc"
};
char D_004A7850[0x20] = {
#include "sub_main/004A7850.inc"
};
char D_004A7870[0x20] = {
#include "sub_main/004A7870.inc"
};
char D_004A7890[0x20] = {
#include "sub_main/004A7890.inc"
};
char D_004A78B0[0x18] = {
#include "sub_main/004A78B0.inc"
};
char D_004A78C8[15] = {
#include "sub_main/004A78C8.inc"
};
char D_004A78D8[15] = {
#include "sub_main/004A78D8.inc"
};
char D_004A78F0[0x20] = {
#include "sub_main/004A78F0.inc"
};
char D_004A7910[0x18] = {
#include "sub_main/004A7910.inc"
};
char D_004A7928[13] = {
#include "sub_main/004A7928.inc"
};
char D_004A7938[13] = {
#include "sub_main/004A7938.inc"
};
char D_004A7948[15] = {
#include "sub_main/004A7948.inc"
};
char D_004A7958[15] = {
#include "sub_main/004A7958.inc"
};
char D_004A7968[15] = {
#include "sub_main/004A7968.inc"
};
char D_004A7978[15] = {
#include "sub_main/004A7978.inc"
};
char D_004A7988[13] = {
#include "sub_main/004A7988.inc"
};
char D_004A7998[13] = {
#include "sub_main/004A7998.inc"
};
char D_004A79A8[15] = {
#include "sub_main/004A79A8.inc"
};
char D_004A79B8[15] = {
#include "sub_main/004A79B8.inc"
};
char D_004A79C8[15] = {
#include "sub_main/004A79C8.inc"
};
char D_004A79D8[15] = {
#include "sub_main/004A79D8.inc"
};
char D_004A79E8[15] = {
#include "sub_main/004A79E8.inc"
};
char D_004A79F8[15] = {
#include "sub_main/004A79F8.inc"
};
char D_004A7A10[0x20] = {
#include "sub_main/004A7A10.inc"
};
char D_004A7A30[0x20] = {
#include "sub_main/004A7A30.inc"
};
char D_004A7A50[0x20] = {
#include "sub_main/004A7A50.inc"
};
char D_004A7A70[0x18] = {
#include "sub_main/004A7A70.inc"
};
char D_004A7A88[13] = {
#include "sub_main/004A7A88.inc"
};
char D_004A7A98[13] = {
#include "sub_main/004A7A98.inc"
};
char D_004A7AA8[15] = {
#include "sub_main/004A7AA8.inc"
};
char D_004A7AB8[15] = {
#include "sub_main/004A7AB8.inc"
};
char D_004A7AC8[15] = {
#include "sub_main/004A7AC8.inc"
};
char D_004A7AD8[15] = {
#include "sub_main/004A7AD8.inc"
};
char D_004A7AF0[0x18] = {
#include "sub_main/004A7AF0.inc"
};
char D_004A7B08[15] = {
#include "sub_main/004A7B08.inc"
};
char D_004A7B20[0x20] = {
#include "sub_main/004A7B20.inc"
};
char D_004A7B40[0x20] = {
#include "sub_main/004A7B40.inc"
};
char D_004A7B60[0x20] = {
#include "sub_main/004A7B60.inc"
};
char D_004A7B80[0x18] = {
#include "sub_main/004A7B80.inc"
};
char D_004A7B98[15] = {
#include "sub_main/004A7B98.inc"
};
char D_004A7BB0[0x20] = {
#include "sub_main/004A7BB0.inc"
};
char D_004A7BD0[0x20] = {
#include "sub_main/004A7BD0.inc"
};
char D_004A7BF0[0x18] = {
#include "sub_main/004A7BF0.inc"
};
char D_004A7C08[13] = {
#include "sub_main/004A7C08.inc"
};
char D_004A7C18[13] = {
#include "sub_main/004A7C18.inc"
};
char D_004A7C28[15] = {
#include "sub_main/004A7C28.inc"
};
char D_004A7C38[15] = {
#include "sub_main/004A7C38.inc"
};
char D_004A7C48[13] = {
#include "sub_main/004A7C48.inc"
};
char D_004A7C58[13] = {
#include "sub_main/004A7C58.inc"
};
char D_004A7C68[0xC] = {
#include "sub_main/004A7C68.inc"
};
char D_004A7C78[0xC] = {
#include "sub_main/004A7C78.inc"
};
char D_004A7C90[0x20] = {
#include "sub_main/004A7C90.inc"
};
char D_004A7CB0[0x20] = {
#include "sub_main/004A7CB0.inc"
};
char D_004A7CD0[0x20] = {
#include "sub_main/004A7CD0.inc"
};
char D_004A7CF0[0x20] = {
#include "sub_main/004A7CF0.inc"
};
char D_004A7D10[0x20] = {
#include "sub_main/004A7D10.inc"
};
char D_004A7D30[0x20] = {
#include "sub_main/004A7D30.inc"
};
char D_004A7D50[0x18] = {
#include "sub_main/004A7D50.inc"
};
char D_004A7D68[13] = {
#include "sub_main/004A7D68.inc"
};
char D_004A7D80[0x20] = {
#include "sub_main/004A7D80.inc"
};
char D_004A7DA0[0x20] = {
#include "sub_main/004A7DA0.inc"
};
char D_004A7DC0[0x18] = {
#include "sub_main/004A7DC0.inc"
};
char D_004A7DD8[15] = {
#include "sub_main/004A7DD8.inc"
};
char D_004A7DE8[15] = {
#include "sub_main/004A7DE8.inc"
};
char D_004A7DF8[0xC] = {
#include "sub_main/004A7DF8.inc"
};
char D_004A7E10[0x20] = {
#include "sub_main/004A7E10.inc"
};
char D_004A7E30[0x20] = {
#include "sub_main/004A7E30.inc"
};
char D_004A7E50[0x18] = {
#include "sub_main/004A7E50.inc"
};
char D_004A7E68[15] = {
#include "sub_main/004A7E68.inc"
};
char D_004A7E80[0x18] = {
#include "sub_main/004A7E80.inc"
};
char D_004A7E98[15] = {
#include "sub_main/004A7E98.inc"
};
char D_004A7EB0[0x18] = {
#include "sub_main/004A7EB0.inc"
};
char D_004A7EC8[13] = {
#include "sub_main/004A7EC8.inc"
};
char D_004A7EE0[0x18] = {
#include "sub_main/004A7EE0.inc"
};
char D_004A7EF8[15] = {
#include "sub_main/004A7EF8.inc"
};
char D_004A7F10[0x18] = {
#include "sub_main/004A7F10.inc"
};
char D_004A7F28[13] = {
#include "sub_main/004A7F28.inc"
};
char D_004A7F38[13] = {
#include "sub_main/004A7F38.inc"
};
char D_004A7F50[0x18] = {
#include "sub_main/004A7F50.inc"
};
char D_004A7F68[13] = {
#include "sub_main/004A7F68.inc"
};
char D_004A7F80[0x18] = {
#include "sub_main/004A7F80.inc"
};
char D_004A7F98[13] = {
#include "sub_main/004A7F98.inc"
};
char D_004A7FB0[0x18] = {
#include "sub_main/004A7FB0.inc"
};
char D_004A7FC8[15] = {
#include "sub_main/004A7FC8.inc"
};
char D_004A7FE0[0x18] = {
#include "sub_main/004A7FE0.inc"
};
char D_004A7FF8[15] = {
#include "sub_main/004A7FF8.inc"
};
char D_004A8010[0x20] = {
#include "sub_main/004A8010.inc"
};
char D_004A8030[0x18] = {
#include "sub_main/004A8030.inc"
};
char D_004A8048[13] = {
#include "sub_main/004A8048.inc"
};
char D_004A8060[0x18] = {
#include "sub_main/004A8060.inc"
};
char D_004A8078[15] = {
#include "sub_main/004A8078.inc"
};
char D_004A8090[0x18] = {
#include "sub_main/004A8090.inc"
};
char D_004A80A8[15] = {
#include "sub_main/004A80A8.inc"
};
char D_004A80C0[0x18] = {
#include "sub_main/004A80C0.inc"
};
char D_004A80D8[15] = {
#include "sub_main/004A80D8.inc"
};
char D_004A80F0[0x20] = {
#include "sub_main/004A80F0.inc"
};
char D_004A8110[0x18] = {
#include "sub_main/004A8110.inc"
};
char D_004A8128[13] = {
#include "sub_main/004A8128.inc"
};
char D_004A8140[0x18] = {
#include "sub_main/004A8140.inc"
};
char D_004A8158[15] = {
#include "sub_main/004A8158.inc"
};
char D_004A8170[0x18] = {
#include "sub_main/004A8170.inc"
};
char D_004A8188[13] = {
#include "sub_main/004A8188.inc"
};
char D_004A81A0[0x18] = {
#include "sub_main/004A81A0.inc"
};
char D_004A81B8[13] = {
#include "sub_main/004A81B8.inc"
};
char D_004A81D0[0x20] = {
#include "sub_main/004A81D0.inc"
};
char D_004A81F0[0x20] = {
#include "sub_main/004A81F0.inc"
};
char D_004A8210[0x20] = {
#include "sub_main/004A8210.inc"
};
char D_004A8230[0x20] = {
#include "sub_main/004A8230.inc"
};
char D_004A8250[0x18] = {
#include "sub_main/004A8250.inc"
};
char D_004A8268[15] = {
#include "sub_main/004A8268.inc"
};
char D_004A8280[0x18] = {
#include "sub_main/004A8280.inc"
};
char D_004A8298[0xC] = {
#include "sub_main/004A8298.inc"
};
char D_004A82A8[15] = {
#include "sub_main/004A82A8.inc"
};
char D_004A82C0[0x18] = {
#include "sub_main/004A82C0.inc"
};
char D_004A82D8[0xC] = {
#include "sub_main/004A82D8.inc"
};
char D_004A82E8[0xC] = {
#include "sub_main/004A82E8.inc"
};
char D_004A8300[0x20] = {
#include "sub_main/004A8300.inc"
};
char D_004A8320[0x20] = {
#include "sub_main/004A8320.inc"
};
char D_004A8340[0x18] = {
#include "sub_main/004A8340.inc"
};
char D_004A8358[15] = {
#include "sub_main/004A8358.inc"
};
char D_004A8370[0x18] = {
#include "sub_main/004A8370.inc"
};
char D_004A8388[15] = {
#include "sub_main/004A8388.inc"
};
char D_004A8398[15] = {
#include "sub_main/004A8398.inc"
};
char D_004A83A8[15] = {
#include "sub_main/004A83A8.inc"
};
char D_004A83B8[13] = {
#include "sub_main/004A83B8.inc"
};
char D_004A83C8[13] = {
#include "sub_main/004A83C8.inc"
};
char D_004A83D8[15] = {
#include "sub_main/004A83D8.inc"
};
char D_004A83E8[15] = {
#include "sub_main/004A83E8.inc"
};
char D_004A8400[0x18] = {
#include "sub_main/004A8400.inc"
};
char D_004A8418[15] = {
#include "sub_main/004A8418.inc"
};
char D_004A8430[0x20] = {
#include "sub_main/004A8430.inc"
};
char D_004A8450[0x20] = {
#include "sub_main/004A8450.inc"
};
char D_004A8470[0x20] = {
#include "sub_main/004A8470.inc"
};
char D_004A8490[0x20] = {
#include "sub_main/004A8490.inc"
};
char D_004A84B0[0x18] = {
#include "sub_main/004A84B0.inc"
};
char D_004A84C8[15] = {
#include "sub_main/004A84C8.inc"
};
char D_004A84E0[0x20] = {
#include "sub_main/004A84E0.inc"
};
char D_004A8500[0x20] = {
#include "sub_main/004A8500.inc"
};
char D_004A8520[0x18] = {
#include "sub_main/004A8520.inc"
};
char D_004A8538[15] = {
#include "sub_main/004A8538.inc"
};
char D_004A8550[0x18] = {
#include "sub_main/004A8550.inc"
};
char D_004A8568[15] = {
#include "sub_main/004A8568.inc"
};
char D_004A8580[0x18] = {
#include "sub_main/004A8580.inc"
};
char D_004A8598[15] = {
#include "sub_main/004A8598.inc"
};
char D_004A85B0[0x20] = {
#include "sub_main/004A85B0.inc"
};
char D_004A85D0[0x20] = {
#include "sub_main/004A85D0.inc"
};
char D_004A85F0[0x20] = {
#include "sub_main/004A85F0.inc"
};
char D_004A8610[0x20] = {
#include "sub_main/004A8610.inc"
};
char D_004A8630[0x18] = {
#include "sub_main/004A8630.inc"
};
char D_004A8648[15] = {
#include "sub_main/004A8648.inc"
};
char D_004A8660[0x20] = {
#include "sub_main/004A8660.inc"
};
char D_004A8680[0x20] = {
#include "sub_main/004A8680.inc"
};
char D_004A86A0[0x20] = {
#include "sub_main/004A86A0.inc"
};
char D_004A86C0[0x20] = {
#include "sub_main/004A86C0.inc"
};
char D_004A86E0[0x20] = {
#include "sub_main/004A86E0.inc"
};
char D_004A8700[0x20] = {
#include "sub_main/004A8700.inc"
};
char D_004A8720[0x20] = {
#include "sub_main/004A8720.inc"
};
char D_004A8740[0x20] = {
#include "sub_main/004A8740.inc"
};
char D_004A8760[0x20] = {
#include "sub_main/004A8760.inc"
};
char D_004A8780[0x20] = {
#include "sub_main/004A8780.inc"
};
char D_004A87A0[0x20] = {
#include "sub_main/004A87A0.inc"
};
char D_004A87C0[0x20] = {
#include "sub_main/004A87C0.inc"
};
char D_004A87E0[0x20] = {
#include "sub_main/004A87E0.inc"
};
char D_004A8800[0x18] = {
#include "sub_main/004A8800.inc"
};
char D_004A8818[15] = {
#include "sub_main/004A8818.inc"
};
char D_004A8830[0x20] = {
#include "sub_main/004A8830.inc"
};
char D_004A8850[0x20] = {
#include "sub_main/004A8850.inc"
};
char D_004A8870[0x20] = {
#include "sub_main/004A8870.inc"
};
char D_004A8890[0x20] = {
#include "sub_main/004A8890.inc"
};
char D_004A88B0[0x20] = {
#include "sub_main/004A88B0.inc"
};
char D_004A88D0[0x18] = {
#include "sub_main/004A88D0.inc"
};
char D_004A88E8[0xC] = {
#include "sub_main/004A88E8.inc"
};
char D_004A88F8[0xC] = {
#include "sub_main/004A88F8.inc"
};
char D_004A8910[0x20] = {
#include "sub_main/004A8910.inc"
};
char D_004A8930[0x20] = {
#include "sub_main/004A8930.inc"
};
char D_004A8950[0x20] = {
#include "sub_main/004A8950.inc"
};
char D_004A8970[0x20] = {
#include "sub_main/004A8970.inc"
};
char D_004A8990[0x20] = {
#include "sub_main/004A8990.inc"
};
char D_004A89B0[0x20] = {
#include "sub_main/004A89B0.inc"
};
char D_004A89D0[0x20] = {
#include "sub_main/004A89D0.inc"
};
char D_004A89F0[0x20] = {
#include "sub_main/004A89F0.inc"
};
char D_004A8A10[0x20] = {
#include "sub_main/004A8A10.inc"
};
char D_004A8A30[0x20] = {
#include "sub_main/004A8A30.inc"
};
char D_004A8A50[0x20] = {
#include "sub_main/004A8A50.inc"
};
char D_004A8A70[0x20] = {
#include "sub_main/004A8A70.inc"
};
char D_004A8A90[0x20] = {
#include "sub_main/004A8A90.inc"
};
char D_004A8AB0[0x20] = {
#include "sub_main/004A8AB0.inc"
};
char D_004A8AD0[0x20] = {
#include "sub_main/004A8AD0.inc"
};
char D_004A8AF0[0x20] = {
#include "sub_main/004A8AF0.inc"
};
char D_004A8B10[0x20] = {
#include "sub_main/004A8B10.inc"
};
char D_004A8B30[0x20] = {
#include "sub_main/004A8B30.inc"
};
char D_004A8B50[0x18] = {
#include "sub_main/004A8B50.inc"
};
char D_004A8B68[15] = {
#include "sub_main/004A8B68.inc"
};
char D_004A8B80[0x20] = {
#include "sub_main/004A8B80.inc"
};
char D_004A8BA0[0x20] = {
#include "sub_main/004A8BA0.inc"
};
char D_004A8BC0[0x18] = {
#include "sub_main/004A8BC0.inc"
};
char D_004A8BD8[15] = {
#include "sub_main/004A8BD8.inc"
};
char D_004A8BF0[0x18] = {
#include "sub_main/004A8BF0.inc"
};
char D_004A8C08[15] = {
#include "sub_main/004A8C08.inc"
};
char D_004A8C20[0x18] = {
#include "sub_main/004A8C20.inc"
};
char D_004A8C38[15] = {
#include "sub_main/004A8C38.inc"
};
char D_004A8C50[0x20] = {
#include "sub_main/004A8C50.inc"
};
char D_004A8C70[0x20] = {
#include "sub_main/004A8C70.inc"
};
char D_004A8C90[0x20] = {
#include "sub_main/004A8C90.inc"
};
char D_004A8CB0[0x20] = {
#include "sub_main/004A8CB0.inc"
};
char D_004A8CD0[0x18] = {
#include "sub_main/004A8CD0.inc"
};
char D_004A8CE8[15] = {
#include "sub_main/004A8CE8.inc"
};
char D_004A8D00[0x20] = {
#include "sub_main/004A8D00.inc"
};
char D_004A8D20[0x20] = {
#include "sub_main/004A8D20.inc"
};
char D_004A8D40[0x20] = {
#include "sub_main/004A8D40.inc"
};
char D_004A8D60[0x20] = {
#include "sub_main/004A8D60.inc"
};
char D_004A8D80[0x20] = {
#include "sub_main/004A8D80.inc"
};
char D_004A8DA0[0x20] = {
#include "sub_main/004A8DA0.inc"
};
char D_004A8DC0[0x20] = {
#include "sub_main/004A8DC0.inc"
};
char D_004A8DE0[0x18] = {
#include "sub_main/004A8DE0.inc"
};
char D_004A8DF8[15] = {
#include "sub_main/004A8DF8.inc"
};
char D_004A8E10[0x20] = {
#include "sub_main/004A8E10.inc"
};
char D_004A8E30[0x20] = {
#include "sub_main/004A8E30.inc"
};
char D_004A8E50[0x18] = {
#include "sub_main/004A8E50.inc"
};
char D_004A8E68[15] = {
#include "sub_main/004A8E68.inc"
};
char D_004A8E80[0x18] = {
#include "sub_main/004A8E80.inc"
};
char D_004A8E98[15] = {
#include "sub_main/004A8E98.inc"
};
char D_004A8EB0[0x20] = {
#include "sub_main/004A8EB0.inc"
};
char D_004A8ED0[0x20] = {
#include "sub_main/004A8ED0.inc"
};
char D_004A8EF0[0x20] = {
#include "sub_main/004A8EF0.inc"
};
char D_004A8F10[0x20] = {
#include "sub_main/004A8F10.inc"
};
char D_004A8F30[0x20] = {
#include "sub_main/004A8F30.inc"
};
char D_004A8F50[0x18] = {
#include "sub_main/004A8F50.inc"
};
char D_004A8F68[0xC] = {
#include "sub_main/004A8F68.inc"
};
char D_004A8F78[0xC] = {
#include "sub_main/004A8F78.inc"
};
char D_004A8F90[0x20] = {
#include "sub_main/004A8F90.inc"
};
char D_004A8FB0[0x20] = {
#include "sub_main/004A8FB0.inc"
};
char D_004A8FD0[0x20] = {
#include "sub_main/004A8FD0.inc"
};
char D_004A8FF0[0x20] = {
#include "sub_main/004A8FF0.inc"
};
char D_004A9010[0x20] = {
#include "sub_main/004A9010.inc"
};
char D_004A9030[0x20] = {
#include "sub_main/004A9030.inc"
};
char D_004A9050[0x20] = {
#include "sub_main/004A9050.inc"
};
char D_004A9070[0x20] = {
#include "sub_main/004A9070.inc"
};
char D_004A9090[0x18] = {
#include "sub_main/004A9090.inc"
};
char D_004A90A8[13] = {
#include "sub_main/004A90A8.inc"
};
char D_004A90C0[0x18] = {
#include "sub_main/004A90C0.inc"
};
char D_004A90D8[15] = {
#include "sub_main/004A90D8.inc"
};
char D_004A90F0[0x18] = {
#include "sub_main/004A90F0.inc"
};
char D_004A9108[15] = {
#include "sub_main/004A9108.inc"
};
char D_004A9120[0x18] = {
#include "sub_main/004A9120.inc"
};
char D_004A9138[13] = {
#include "sub_main/004A9138.inc"
};
char D_004A9150[0x18] = {
#include "sub_main/004A9150.inc"
};
char D_004A9168[15] = {
#include "sub_main/004A9168.inc"
};
char D_004A9180[0x18] = {
#include "sub_main/004A9180.inc"
};
char D_004A9198[15] = {
#include "sub_main/004A9198.inc"
};
char D_004A91B0[0x18] = {
#include "sub_main/004A91B0.inc"
};
char D_004A91C8[15] = {
#include "sub_main/004A91C8.inc"
};
char D_004A91E0[0x20] = {
#include "sub_main/004A91E0.inc"
};
char D_004A9200[0x20] = {
#include "sub_main/004A9200.inc"
};
char D_004A9220[0x20] = {
#include "sub_main/004A9220.inc"
};
char D_004A9240[0x20] = {
#include "sub_main/004A9240.inc"
};
char D_004A9260[0x18] = {
#include "sub_main/004A9260.inc"
};
char D_004A9278[13] = {
#include "sub_main/004A9278.inc"
};
char D_004A9290[0x18] = {
#include "sub_main/004A9290.inc"
};
char D_004A92A8[15] = {
#include "sub_main/004A92A8.inc"
};
char D_004A92C0[0x18] = {
#include "sub_main/004A92C0.inc"
};
char D_004A92D8[15] = {
#include "sub_main/004A92D8.inc"
};
char D_004A92E8[13] = {
#include "sub_main/004A92E8.inc"
};
char D_004A92F8[15] = {
#include "sub_main/004A92F8.inc"
};
char D_004A9308[15] = {
#include "sub_main/004A9308.inc"
};
char D_004A9320[0x18] = {
#include "sub_main/004A9320.inc"
};
char D_004A9338[15] = {
#include "sub_main/004A9338.inc"
};
char D_004A9348[15] = {
#include "sub_main/004A9348.inc"
};
char D_004A9358[15] = {
#include "sub_main/004A9358.inc"
};
char D_004A9370[0x20] = {
#include "sub_main/004A9370.inc"
};
char D_004A9390[0x20] = {
#include "sub_main/004A9390.inc"
};
char D_004A93B0[0x18] = {
#include "sub_main/004A93B0.inc"
};
char D_004A93C8[15] = {
#include "sub_main/004A93C8.inc"
};
char D_004A93D8[15] = {
#include "sub_main/004A93D8.inc"
};
char D_004A93F0[0x18] = {
#include "sub_main/004A93F0.inc"
};
char D_004A9408[15] = {
#include "sub_main/004A9408.inc"
};
char D_004A9418[13] = {
#include "sub_main/004A9418.inc"
};
char D_004A9428[13] = {
#include "sub_main/004A9428.inc"
};
char D_004A9438[0xC] = {
#include "sub_main/004A9438.inc"
};
char D_004A9448[0xC] = {
#include "sub_main/004A9448.inc"
};
char D_004A9460[0x20] = {
#include "sub_main/004A9460.inc"
};
char D_004A9480[0x20] = {
#include "sub_main/004A9480.inc"
};
char D_004A94A0[0x18] = {
#include "sub_main/004A94A0.inc"
};
char D_004A94B8[15] = {
#include "sub_main/004A94B8.inc"
};
char D_004A94D0[0x20] = {
#include "sub_main/004A94D0.inc"
};
char D_004A94F0[0x20] = {
#include "sub_main/004A94F0.inc"
};
char D_004A9510[0x20] = {
#include "sub_main/004A9510.inc"
};
char D_004A9530[0x18] = {
#include "sub_main/004A9530.inc"
};
char D_004A9548[15] = {
#include "sub_main/004A9548.inc"
};
char D_004A9560[0x20] = {
#include "sub_main/004A9560.inc"
};
char D_004A9580[0x20] = {
#include "sub_main/004A9580.inc"
};
char D_004A95A0[0x20] = {
#include "sub_main/004A95A0.inc"
};
char D_004A95C0[0x20] = {
#include "sub_main/004A95C0.inc"
};
char D_004A95E0[0x20] = {
#include "sub_main/004A95E0.inc"
};
char D_004A9600[0x18] = {
#include "sub_main/004A9600.inc"
};
char D_004A9618[15] = {
#include "sub_main/004A9618.inc"
};
char D_004A9628[15] = {
#include "sub_main/004A9628.inc"
};
char D_004A9640[0x20] = {
#include "sub_main/004A9640.inc"
};
char D_004A9660[0x20] = {
#include "sub_main/004A9660.inc"
};
char D_004A9680[0x20] = {
#include "sub_main/004A9680.inc"
};
char D_004A96A0[0x18] = {
#include "sub_main/004A96A0.inc"
};
char D_004A96B8[15] = {
#include "sub_main/004A96B8.inc"
};
char D_004A96C8[15] = {
#include "sub_main/004A96C8.inc"
};
char D_004A96E0[0x20] = {
#include "sub_main/004A96E0.inc"
};
char D_004A9700[0x18] = {
#include "sub_main/004A9700.inc"
};
char D_004A9718[15] = {
#include "sub_main/004A9718.inc"
};
char D_004A9728[15] = {
#include "sub_main/004A9728.inc"
};
char D_004A9740[0x18] = {
#include "sub_main/004A9740.inc"
};
char D_004A9758[15] = {
#include "sub_main/004A9758.inc"
};
char D_004A9768[15] = {
#include "sub_main/004A9768.inc"
};
char D_004A9778[15] = {
#include "sub_main/004A9778.inc"
};
char D_004A9788[15] = {
#include "sub_main/004A9788.inc"
};
char D_004A97A0[0x20] = {
#include "sub_main/004A97A0.inc"
};
char D_004A97C0[0x20] = {
#include "sub_main/004A97C0.inc"
};
char D_004A97E0[0x20] = {
#include "sub_main/004A97E0.inc"
};
char D_004A9800[0x20] = {
#include "sub_main/004A9800.inc"
};
char D_004A9820[0x20] = {
#include "sub_main/004A9820.inc"
};
char D_004A9840[0x20] = {
#include "sub_main/004A9840.inc"
};
char D_004A9860[0x20] = {
#include "sub_main/004A9860.inc"
};
char D_004A9880[0x20] = {
#include "sub_main/004A9880.inc"
};
char D_004A98A0[0x20] = {
#include "sub_main/004A98A0.inc"
};
char D_004A98C0[0x20] = {
#include "sub_main/004A98C0.inc"
};
char D_004A98E0[0x20] = {
#include "sub_main/004A98E0.inc"
};
char D_004A9900[0x20] = {
#include "sub_main/004A9900.inc"
};
char D_004A9920[0x18] = {
#include "sub_main/004A9920.inc"
};
char D_004A9938[15] = {
#include "sub_main/004A9938.inc"
};
char D_004A9948[15] = {
#include "sub_main/004A9948.inc"
};
char D_004A9960[0x20] = {
#include "sub_main/004A9960.inc"
};
char D_004A9980[0x18] = {
#include "sub_main/004A9980.inc"
};
char D_004A9998[15] = {
#include "sub_main/004A9998.inc"
};
char D_004A99A8[15] = {
#include "sub_main/004A99A8.inc"
};
char D_004A99B8[15] = {
#include "sub_main/004A99B8.inc"
};
char D_004A99C8[15] = {
#include "sub_main/004A99C8.inc"
};
char D_004A99D8[15] = {
#include "sub_main/004A99D8.inc"
};
char D_004A99E8[15] = {
#include "sub_main/004A99E8.inc"
};
char D_004A9A00[0x20] = {
#include "sub_main/004A9A00.inc"
};
char D_004A9A20[0x20] = {
#include "sub_main/004A9A20.inc"
};
char D_004A9A40[0x20] = {
#include "sub_main/004A9A40.inc"
};
char D_004A9A60[0x20] = {
#include "sub_main/004A9A60.inc"
};
char D_004A9A80[0x20] = {
#include "sub_main/004A9A80.inc"
};
char D_004A9AA0[0x20] = {
#include "sub_main/004A9AA0.inc"
};
char D_004A9AC0[0x18] = {
#include "sub_main/004A9AC0.inc"
};
char D_004A9AD8[0xC] = {
#include "sub_main/004A9AD8.inc"
};
char D_004A9AF0[0x20] = {
#include "sub_main/004A9AF0.inc"
};
char D_004A9B10[0x20] = {
#include "sub_main/004A9B10.inc"
};
char D_004A9B30[0x18] = {
#include "sub_main/004A9B30.inc"
};
char D_004A9B48[13] = {
#include "sub_main/004A9B48.inc"
};
char D_004A9B58[13] = {
#include "sub_main/004A9B58.inc"
};
char D_004A9B70[0x20] = {
#include "sub_main/004A9B70.inc"
};
char D_004A9B90[0x20] = {
#include "sub_main/004A9B90.inc"
};
char D_004A9BB0[0x20] = {
#include "sub_main/004A9BB0.inc"
};
char D_004A9BD0[0x20] = {
#include "sub_main/004A9BD0.inc"
};
char D_004A9BF0[0x20] = {
#include "sub_main/004A9BF0.inc"
};
char D_004A9C10[0x18] = {
#include "sub_main/004A9C10.inc"
};
char D_004A9C28[13] = {
#include "sub_main/004A9C28.inc"
};
char D_004A9C38[13] = {
#include "sub_main/004A9C38.inc"
};
char D_004A9C48[0xC] = {
#include "sub_main/004A9C48.inc"
};
char D_004A9C58[0xC] = {
#include "sub_main/004A9C58.inc"
};
char D_004A9C68[0xC] = {
#include "sub_main/004A9C68.inc"
};
char D_004A9C78[0xC] = {
#include "sub_main/004A9C78.inc"
};
char D_004A9C88[15] = {
#include "sub_main/004A9C88.inc"
};
char D_004A9C98[15] = {
#include "sub_main/004A9C98.inc"
};
char D_004A9CB0[0x20] = {
#include "sub_main/004A9CB0.inc"
};
char D_004A9CD0[0x20] = {
#include "sub_main/004A9CD0.inc"
};
char D_004A9CF0[0x20] = {
#include "sub_main/004A9CF0.inc"
};
char D_004A9D10[0x20] = {
#include "sub_main/004A9D10.inc"
};
char D_004A9D30[0x20] = {
#include "sub_main/004A9D30.inc"
};
char D_004A9D50[0x20] = {
#include "sub_main/004A9D50.inc"
};
char D_004A9D70[0x20] = {
#include "sub_main/004A9D70.inc"
};
char D_004A9D90[0x20] = {
#include "sub_main/004A9D90.inc"
};
char D_004A9DB0[0x20] = {
#include "sub_main/004A9DB0.inc"
};
char D_004A9DD0[0x20] = {
#include "sub_main/004A9DD0.inc"
};
char D_004A9DF0[0x20] = {
#include "sub_main/004A9DF0.inc"
};
char D_004A9E10[0x20] = {
#include "sub_main/004A9E10.inc"
};
char D_004A9E30[0x20] = {
#include "sub_main/004A9E30.inc"
};
char D_004A9E50[0x20] = {
#include "sub_main/004A9E50.inc"
};
char D_004A9E70[0x20] = {
#include "sub_main/004A9E70.inc"
};
char D_004A9E90[0x20] = {
#include "sub_main/004A9E90.inc"
};
char D_004A9EB0[0x20] = {
#include "sub_main/004A9EB0.inc"
};
char D_004A9ED0[0x20] = {
#include "sub_main/004A9ED0.inc"
};
char D_004A9EF0[0x20] = {
#include "sub_main/004A9EF0.inc"
};
char D_004A9F10[0x20] = {
#include "sub_main/004A9F10.inc"
};
char D_004A9F30[0x20] = {
#include "sub_main/004A9F30.inc"
};
char D_004A9F50[0x20] = {
#include "sub_main/004A9F50.inc"
};
char D_004A9F70[0x20] = {
#include "sub_main/004A9F70.inc"
};
char D_004A9F90[0x20] = {
#include "sub_main/004A9F90.inc"
};
char D_004A9FB0[0x20] = {
#include "sub_main/004A9FB0.inc"
};
char D_004A9FD0[0x20] = {
#include "sub_main/004A9FD0.inc"
};
char D_004A9FF0[0x20] = {
#include "sub_main/004A9FF0.inc"
};
char D_004AA010[0x20] = {
#include "sub_main/004AA010.inc"
};
char D_004AA030[0x20] = {
#include "sub_main/004AA030.inc"
};
char D_004AA050[0x20] = {
#include "sub_main/004AA050.inc"
};
char D_004AA070[0x20] = {
#include "sub_main/004AA070.inc"
};
char D_004AA090[0x20] = {
#include "sub_main/004AA090.inc"
};
char D_004AA0B0[0x20] = {
#include "sub_main/004AA0B0.inc"
};
char D_004AA0D0[0x20] = {
#include "sub_main/004AA0D0.inc"
};
char D_004AA0F0[0x20] = {
#include "sub_main/004AA0F0.inc"
};
char D_004AA110[0x20] = {
#include "sub_main/004AA110.inc"
};
char D_004AA130[0x20] = {
#include "sub_main/004AA130.inc"
};
char D_004AA150[0x20] = {
#include "sub_main/004AA150.inc"
};
char D_004AA170[0x20] = {
#include "sub_main/004AA170.inc"
};
char D_004AA190[0x18] = {
#include "sub_main/004AA190.inc"
};
char D_004AA1A8[13] = {
#include "sub_main/004AA1A8.inc"
};
char D_004AA1B8[13] = {
#include "sub_main/004AA1B8.inc"
};
char D_004AA1D0[0x20] = {
#include "sub_main/004AA1D0.inc"
};
char D_004AA1F0[0x20] = {
#include "sub_main/004AA1F0.inc"
};
char D_004AA210[0x20] = {
#include "sub_main/004AA210.inc"
};
char D_004AA230[0x20] = {
#include "sub_main/004AA230.inc"
};
char D_004AA250[0x20] = {
#include "sub_main/004AA250.inc"
};
char D_004AA270[0x18] = {
#include "sub_main/004AA270.inc"
};
char D_004AA288[13] = {
#include "sub_main/004AA288.inc"
};
char D_004AA298[13] = {
#include "sub_main/004AA298.inc"
};
char D_004AA2B0[0x20] = {
#include "sub_main/004AA2B0.inc"
};
char D_004AA2D0[0x20] = {
#include "sub_main/004AA2D0.inc"
};
char D_004AA2F0[0x20] = {
#include "sub_main/004AA2F0.inc"
};
char D_004AA310[0x20] = {
#include "sub_main/004AA310.inc"
};
char D_004AA330[0x20] = {
#include "sub_main/004AA330.inc"
};
char D_004AA350[0x20] = {
#include "sub_main/004AA350.inc"
};
char D_004AA370[0x20] = {
#include "sub_main/004AA370.inc"
};
char D_004AA390[0x20] = {
#include "sub_main/004AA390.inc"
};
char D_004AA3B0[0x20] = {
#include "sub_main/004AA3B0.inc"
};
char D_004AA3D0[0x20] = {
#include "sub_main/004AA3D0.inc"
};
char D_004AA3F0[0x20] = {
#include "sub_main/004AA3F0.inc"
};
char D_004AA410[0x20] = {
#include "sub_main/004AA410.inc"
};
char D_004AA430[0x20] = {
#include "sub_main/004AA430.inc"
};
char D_004AA450[0x20] = {
#include "sub_main/004AA450.inc"
};
char D_004AA470[0x20] = {
#include "sub_main/004AA470.inc"
};
char D_004AA490[0x20] = {
#include "sub_main/004AA490.inc"
};
char D_004AA4B0[0x20] = {
#include "sub_main/004AA4B0.inc"
};
char D_004AA4D0[0x20] = {
#include "sub_main/004AA4D0.inc"
};
char D_004AA4F0[0x20] = {
#include "sub_main/004AA4F0.inc"
};
char D_004AA510[0x20] = {
#include "sub_main/004AA510.inc"
};
char D_004AA530[0x20] = {
#include "sub_main/004AA530.inc"
};
char D_004AA550[0x20] = {
#include "sub_main/004AA550.inc"
};
char D_004AA570[0x20] = {
#include "sub_main/004AA570.inc"
};
char D_004AA590[0x20] = {
#include "sub_main/004AA590.inc"
};
char D_004AA5B0[0x20] = {
#include "sub_main/004AA5B0.inc"
};
char D_004AA5D0[0x20] = {
#include "sub_main/004AA5D0.inc"
};
char D_004AA5F0[0x20] = {
#include "sub_main/004AA5F0.inc"
};
char D_004AA610[0x20] = {
#include "sub_main/004AA610.inc"
};
char D_004AA630[0x20] = {
#include "sub_main/004AA630.inc"
};
char D_004AA650[0x20] = {
#include "sub_main/004AA650.inc"
};
char D_004AA670[0x20] = {
#include "sub_main/004AA670.inc"
};
char D_004AA690[0x20] = {
#include "sub_main/004AA690.inc"
};
char D_004AA6B0[0x20] = {
#include "sub_main/004AA6B0.inc"
};
char D_004AA6D0[0x20] = {
#include "sub_main/004AA6D0.inc"
};
char D_004AA6F0[0x20] = {
#include "sub_main/004AA6F0.inc"
};
char D_004AA710[0x20] = {
#include "sub_main/004AA710.inc"
};
char D_004AA730[0x20] = {
#include "sub_main/004AA730.inc"
};
char D_004AA750[0x20] = {
#include "sub_main/004AA750.inc"
};
char D_004AA770[0x20] = {
#include "sub_main/004AA770.inc"
};
char D_004AA790[0x20] = {
#include "sub_main/004AA790.inc"
};
char D_004AA7B0[0x20] = {
#include "sub_main/004AA7B0.inc"
};
char D_004AA7D0[0x20] = {
#include "sub_main/004AA7D0.inc"
};
char D_004AA7F0[0x20] = {
#include "sub_main/004AA7F0.inc"
};
char D_004AA810[0x20] = {
#include "sub_main/004AA810.inc"
};
char D_004AA830[0x20] = {
#include "sub_main/004AA830.inc"
};
char D_004AA850[0x20] = {
#include "sub_main/004AA850.inc"
};
char D_004AA870[0x20] = {
#include "sub_main/004AA870.inc"
};
char D_004AA890[0x18] = {
#include "sub_main/004AA890.inc"
};
char D_004AA8A8[13] = {
#include "sub_main/004AA8A8.inc"
};
char D_004AA8B8[13] = {
#include "sub_main/004AA8B8.inc"
};
char D_004AA8D0[0x20] = {
#include "sub_main/004AA8D0.inc"
};
char D_004AA8F0[0x20] = {
#include "sub_main/004AA8F0.inc"
};
char D_004AA910[0x20] = {
#include "sub_main/004AA910.inc"
};
char D_004AA930[0x18] = {
#include "sub_main/004AA930.inc"
};
char D_004AA948[13] = {
#include "sub_main/004AA948.inc"
};
char D_004AA958[13] = {
#include "sub_main/004AA958.inc"
};
char D_004AA970[0x20] = {
#include "sub_main/004AA970.inc"
};
char D_004AA990[0x18] = {
#include "sub_main/004AA990.inc"
};
char D_004AA9A8[15] = {
#include "sub_main/004AA9A8.inc"
};
char D_004AA9B8[15] = {
#include "sub_main/004AA9B8.inc"
};
char D_004AA9D0[0x20] = {
#include "sub_main/004AA9D0.inc"
};
char D_004AA9F0[0x20] = {
#include "sub_main/004AA9F0.inc"
};
char D_004AAA10[0x20] = {
#include "sub_main/004AAA10.inc"
};
char D_004AAA30[0x18] = {
#include "sub_main/004AAA30.inc"
};
char D_004AAA48[15] = {
#include "sub_main/004AAA48.inc"
};
char D_004AAA58[15] = {
#include "sub_main/004AAA58.inc"
};
char D_004AAA70[0x20] = {
#include "sub_main/004AAA70.inc"
};
char D_004AAA90[0x20] = {
#include "sub_main/004AAA90.inc"
};
char D_004AAAB0[0x20] = {
#include "sub_main/004AAAB0.inc"
};
char D_004AAAD0[0x20] = {
#include "sub_main/004AAAD0.inc"
};
char D_004AAAF0[0x20] = {
#include "sub_main/004AAAF0.inc"
};
char D_004AAB10[0x20] = {
#include "sub_main/004AAB10.inc"
};
char D_004AAB30[0x20] = {
#include "sub_main/004AAB30.inc"
};
char D_004AAB50[0x20] = {
#include "sub_main/004AAB50.inc"
};
char D_004AAB70[0x20] = {
#include "sub_main/004AAB70.inc"
};
char D_004AAB90[0x18] = {
#include "sub_main/004AAB90.inc"
};
char D_004AABA8[15] = {
#include "sub_main/004AABA8.inc"
};
char D_004AABB8[15] = {
#include "sub_main/004AABB8.inc"
};
char D_004AABD0[0x20] = {
#include "sub_main/004AABD0.inc"
};
char D_004AABF0[0x20] = {
#include "sub_main/004AABF0.inc"
};
char D_004AAC10[0x20] = {
#include "sub_main/004AAC10.inc"
};
char D_004AAC30[0x18] = {
#include "sub_main/004AAC30.inc"
};
char D_004AAC48[15] = {
#include "sub_main/004AAC48.inc"
};
char D_004AAC58[13] = {
#include "sub_main/004AAC58.inc"
};
char D_004AAC70[0x18] = {
#include "sub_main/004AAC70.inc"
};
char D_004AAC88[15] = {
#include "sub_main/004AAC88.inc"
};
char D_004AAC98[15] = {
#include "sub_main/004AAC98.inc"
};
char D_004AACA8[15] = {
#include "sub_main/004AACA8.inc"
};
char D_004AACB8[15] = {
#include "sub_main/004AACB8.inc"
};
char D_004AACD0[0x20] = {
#include "sub_main/004AACD0.inc"
};
char D_004AACF0[0x20] = {
#include "sub_main/004AACF0.inc"
};
char D_004AAD10[0x18] = {
#include "sub_main/004AAD10.inc"
};
char D_004AAD28[13] = {
#include "sub_main/004AAD28.inc"
};
char D_004AAD38[13] = {
#include "sub_main/004AAD38.inc"
};
char D_004AAD50[0x20] = {
#include "sub_main/004AAD50.inc"
};
char D_004AAD70[0x18] = {
#include "sub_main/004AAD70.inc"
};
char D_004AAD88[13] = {
#include "sub_main/004AAD88.inc"
};
char D_004AAD98[13] = {
#include "sub_main/004AAD98.inc"
};
char D_004AADA8[0xC] = {
#include "sub_main/004AADA8.inc"
};
char D_004AADB8[0xC] = {
#include "sub_main/004AADB8.inc"
};
char D_004AADD0[0x20] = {
#include "sub_main/004AADD0.inc"
};
char D_004AADF0[0x20] = {
#include "sub_main/004AADF0.inc"
};
char D_004AAE10[0x20] = {
#include "sub_main/004AAE10.inc"
};
char D_004AAE30[0x20] = {
#include "sub_main/004AAE30.inc"
};
char D_004AAE50[0x20] = {
#include "sub_main/004AAE50.inc"
};
char D_004AAE70[0x20] = {
#include "sub_main/004AAE70.inc"
};
char D_004AAE90[0x20] = {
#include "sub_main/004AAE90.inc"
};
char D_004AAEB0[0x20] = {
#include "sub_main/004AAEB0.inc"
};
char D_004AAED0[0x20] = {
#include "sub_main/004AAED0.inc"
};
char D_004AAEF0[0x20] = {
#include "sub_main/004AAEF0.inc"
};
char D_004AAF10[0x20] = {
#include "sub_main/004AAF10.inc"
};
char D_004AAF30[0x20] = {
#include "sub_main/004AAF30.inc"
};
char D_004AAF50[0x20] = {
#include "sub_main/004AAF50.inc"
};
char D_004AAF70[0x20] = {
#include "sub_main/004AAF70.inc"
};
char D_004AAF90[0x20] = {
#include "sub_main/004AAF90.inc"
};
char D_004AAFB0[0x20] = {
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
void Save_ClearEventFlag(s32 a);
s32 Bits32_Test(u32 *bits, s32 n);

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
    Save_ClearEventFlag(0x7E);
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


u8 Equip_GetSlotCount(EquipItem *item);
s32 Armor_LacksSexModel(s32 id, u8 female);
u16 Record_GroupHunted(SaveData *save, u8 group);
u16 Record_GroupCaptured(SaveData *save, u8 group);
u16 Record_GetHunted(SaveData *save, u8 monster);
u16 Record_GetCaptured(SaveData *save, u8 monster);
s16 Box_GetCapacity(SaveData *save);
DecoData *Item_GetDecoration(u16 item);
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
s32 Equip_TestFlag(EquipItem *item, s16 bit);
s32 Award_Grant(s32 arg0);
s32 RateFlag_SetAndNotify(s32 a);

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
