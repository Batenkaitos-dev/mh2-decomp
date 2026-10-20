#ifndef ITEM_H
#define ITEM_H

#include "common.h"

/*
 * Item data: gItemData (0x45F110), 0x18 bytes per item id (0 is no
 * item; ids go up to 0x398). Item names are in gItemNames (0x464770).
 */
typedef struct ItemData {
    u8 type;            /* 0x00: 2 ammo, 3 coating (bowgun ammo slots) */
    u8 flags;           /* 0x01: 1 selectable in the pouch, 2 ammo, 4 (for bowgun kind 10) */
    u8 rarity;          /* 0x02: Item_DrawRarity */
    u8 max;             /* 0x03: most the pouch holds; ITEM_MAX_UNLIMITED: no limit */
    u8 flags2;          /* 0x04: bit 0 the item icon (Item_DrawIcon, HelpWindow_Draw), bit 2 (Plsel_ResultsUpdate) */
    u8 icon;            /* 0x05: the item icon, 0xFF none (drawn as icon + 1) */
    u8 color;           /* 0x06: index into gItemColors */
    u8 unk07;
    s16 ammoKind;       /* 0x08: bit number in the loadable-ammo mask */
    u16 useSound;       /* 0x0A: 0, 2 or 3 selects the sound of use (Item_PlayUseSound) */
    s32 price;          /* 0x0C: the buy price (Shop_GetBuyPrice: halved online in a kind-4 village); the lobby's Item_HalfPrice gives half of it (the decorations counter's make price) */
    s32 value;          /* 0x10: the item box's sell price (Item_GetSellPrice, the box's sell menu) and the reward value: plsel sums value * count (Plsel_ItemStackValue, Plsel_ComputeRewards) */
    u8 category;        /* 0x14: the item category (the lobby's item box counts per category) */
    u8 unk15;
    u16 deco;          /* 0x16: decoration record (gDecoTable, 16 bytes each); 0: not one */
} ItemData;

extern ItemData gItemData[];

/* ItemData.max: no limit on the stack (Pouch_AddEx; the save's pouch and item box, SavePouch_*
 * and ItemBox_*). */
#define ITEM_MAX_UNLIMITED      0xFF

/* A decoration record (16 bytes): gDecoTable's entry for ItemData.deco
 * (Item_GetDecoration). */
typedef struct DecoData {
    u8 size;                    /* 0x0: the slots it takes (1-3) */
    u8 flags;                   /* 0x1: what it fits (Deco_FitsEquip): 1 a blade, 2 a gun, 4 and 8 armour with the same ArmorData.flags bit (blademaster, gunner) */
    u8 pad2[2];
    s32 price;                  /* 0x4: zenny, the fee for attaching it (Deco_GetAttachFee) */
    u8 skills[4][2];            /* 0x8: {skill, points (s8)} */
} DecoData;

extern DecoData gDecoTable[];

/* An item and a count: a pouch slot (Player.pouch) or a supply box slot
 * (SceneState.supply). */
typedef struct ItemStack {
    u16 item;           /* 0x0: 0 = none */
    s16 count;          /* 0x2 */
} ItemStack;

#define POUCH_SLOTS     20      /* Player.pouch's slots */
#define POUCH_SLOT_NONE 0xFF    /* no pouch slot (Player.ammoSlot, Player.coatingSlot, Pouch_FindSlot) */

/* ItemData.flags bits. */
#define ITEM_FLAG_SELECTABLE    0x01    /* the pouch cursor can select it (Pouch_SelectFirst, Pouch_NextSelectable) */
#define ITEM_FLAG_AMMO          0x02

/* Item ids run from 1 to ITEM_ID_COUNT - 1 (0 is no item; Pouch_AddEx). */
#define ITEM_ID_COUNT           0x399

/*
 * An equipment item (12 bytes): an entry of the equipment box and of the
 * equipped items in the save data (SaveData.box, .equip), and a piece the
 * player wears (Player.equip).
 */
typedef struct EquipItem {
    u8 used;            /* 0x0 */
    u8 type;            /* 0x1: enum EquipType */
    u16 id;             /* 0x2: 0 = none */
    u16 level;          /* 0x4: upgrade level; guns: level in bits 0-3, options 0x10-0x80 */
    u16 deco[3];        /* 0x6: decorations in the slots (item ids), 0 = empty */
} EquipItem;

/* EquipItem.type: what a piece of equipment is. An armour piece's type is also its slot in
 * Player.equip, and a weapon of either type sits in Player.equip[EQUIP_SLOT_WEAPON]
 * (Equip_GetData picks the data table by it; type 1 isn't used, and Player.equip[1] holds the
 * face). SaveData.equip keeps another order (save.h's enum SaveEquipSlot). */
enum EquipType {
    EQUIP_LEGS = 0,
    EQUIP_HEAD = 2,
    EQUIP_BODY = 3,
    EQUIP_ARMS = 4,
    EQUIP_WAIST = 5,
    EQUIP_BLADE = 6,            /* a blademaster's weapon (gBladeData) */
    EQUIP_GUN = 7               /* a bowgun (gGunData) */
};
/* Player.equip's slots besides the armour pieces' (which are their types). */
#define EQUIP_SLOT_FACE     1   /* the face (its id is SaveData.face + 1) */
#define EQUIP_SLOT_WEAPON   6   /* the weapon, of either type */

/* Sound_PlayArmorRattle's kind (PlayerSe_EquipAtFrame): an armour type plays that piece's rattle
 * (EQUIP_BODY, EQUIP_ARMS, EQUIP_WAIST: the body's and the waist's only when they sound unlike the
 * pieces before them), and EQUIP_RATTLE_ALL the legs', body's and waist's, each distinct sound
 * once. The sound is the piece's gArmorRattleKinds entry. */
#define EQUIP_RATTLE_ALL    0

/* BladeData/GunData.elementType: the weapon's element. Player_GetWeaponStat
 * reads it for WSTAT_FIRE ... WSTAT_ICE and WSTAT_DRAGON. */
#define WEAPON_ELEMENT_NONE     0
#define WEAPON_ELEMENT_FIRE     1
#define WEAPON_ELEMENT_WATER    2
#define WEAPON_ELEMENT_THUNDER  3
#define WEAPON_ELEMENT_ICE      4
#define WEAPON_ELEMENT_DRAGON   5

/* BladeData.statusType: the status a blade inflicts. Player_GetWeaponStat
 * reads it for WSTAT_POISON, WSTAT_STATUS_TYPE2 and WSTAT_STATUS_TYPE3. */
#define WEAPON_STATUS_NONE      0
#define WEAPON_STATUS_POISON    1
#define WEAPON_STATUS_TYPE2     2   /* builds up the monster's status 4 (ELEMF_EM_STATUS4) */
#define WEAPON_STATUS_TYPE3     3   /* builds up the monster's status 2 (ELEMF_EM_STATUS2) */

/* A blade weapon (0x18 bytes, gBladeData at 0x44CCA0, by weapon id). */
typedef struct BladeData {
    u16 kind;           /* 0x00: row of gWeaponKindClass, the weapon class */
    u8 rarity;          /* 0x02: low 4 bits: Equip_GetRarity */
    u8 unk03;
    u32 price;          /* 0x04: Equip_GetPrice */
    u8 sharpType;       /* 0x08: row of the sharpness table (Player_GetSharpnessLevel) */
    u8 sharpMax;        /* 0x09: Player.sharpnessMax = sharpMax * 50 + 150; Player_ApplySharpnessSkill's base */
    s16 attack;         /* 0x0A: attack (EquipView_GetStat) */
    u8 defense;         /* 0x0C: defense it adds (EquipView_GetStat kind 1) */
    u8 affinity;        /* 0x0D: read as an s8 (Player_GetWeaponStat kind 8) */
    u8 elementType;     /* 0x0E: WEAPON_ELEMENT_* (Weapon_ElementValue) */
    u8 elementValue;    /* 0x0F */
    u8 statusType;      /* 0x10: WEAPON_STATUS_* (Weapon_StatusValue) */
    u8 statusValue;     /* 0x11 */
    u8 slots;           /* 0x12: decoration slots (Equip_GetSlotCount) */
    u8 special;         /* 0x13: a gunlance's shelling type, a horn's notes (Equip_GetBladeSpecial,
                         * Player.bladeSpecial) */
    u8 forgeCond;       /* 0x14: the condition that lets it be forged (FORGE_COND_*) */
    u8 sortOrder;       /* 0x15: the order within its class in the sorted box (BoxEquip_SortKey's bits 16-23) */
    u8 unk16[2];
} BladeData;
typedef char BladeData_size_check[(sizeof(BladeData) == 0x18) ? 1 : -1];

/* A gun (0x18 bytes, gGunData at 0x44FA30, by weapon id). */
typedef struct GunData {
    u16 kind;           /* 0x00: row of gWeaponKindClass, the weapon class */
    u8 rarity;          /* 0x02: low 4 bits: Equip_GetRarity */
    u8 levelRow;        /* 0x03: row of gGunLevelRows (Gun_GetAttack) */
    u32 price;          /* 0x04: Equip_GetPrice */
    s16 attack;         /* 0x08: base attack (Gun_GetAttack) */
    u8 defense;         /* 0x0A: defense it adds (EquipView_GetStat kind 1) */
    u8 recoil;          /* 0x0B: row of gGunRecoilTexts (Bowgun_RecoilLevel subtracts it) */
    u8 slots;           /* 0x0C: decoration slots (Equip_GetSlotCount) */
    u8 affinity;        /* 0x0D: read as an s8 (Player_GetWeaponStat kind 8) */
    u8 sortOrder;       /* 0x0E: the order within its class in the sorted box (BoxEquip_SortKey's bits 16-23) */
    u8 clipRow;         /* 0x0F: row of the clip size table (Bowgun_LoadAmmo) */
    u8 elementType;     /* 0x10: WEAPON_ELEMENT_* (Player_GetWeaponStat) */
    u8 elementValue;    /* 0x11 */
    u8 reload;          /* 0x12: row of gGunReloadTexts (Bowgun_ReloadLevel subtracts it) */
    u8 forgeCond;       /* 0x13: the condition that lets it be forged (FORGE_COND_*) */
    u32 ammoMask;       /* 0x14: the ammo kinds it loads (Bowgun_GetAmmoMask) */
} GunData;
typedef char GunData_size_check[(sizeof(GunData) == 0x18) ? 1 : -1];

/* What lets a weapon be forged (BladeData.forgeCond, GunData.forgeCond), and what unlocks a
 * recipe through one of its materials (the recipes' cond[]): Forge_CheckCondition,
 * Forge_CheckRecipe. A recipe's material with FORGE_COND_OBTAINED, _HR, _GROUP or _HR_GROUP also
 * needs the material obtained (Save_IsItemObtained); a weapon's are met without it. */
#define FORGE_COND_NONE         0   /* none: always met */
#define FORGE_COND_OBTAINED     1   /* the material obtained */
#define FORGE_COND_HR           2   /* a hunter rank above 0 (gSave.hr) */
#define FORGE_COND_PSP_LINK_7   3   /* EVENT_FLAG_PSP_LINK_7 set */
#define FORGE_COND_PSP_LINK_8   4   /* EVENT_FLAG_PSP_LINK_8 set */
#define FORGE_COND_PSP_LINK_9   5   /* EVENT_FLAG_PSP_LINK_9 set */
#define FORGE_COND_GROUP        6   /* a monster of gMonsterGroups[0xB] hunted or captured */
#define FORGE_COND_HR_GROUP     7   /* both FORGE_COND_HR and FORGE_COND_GROUP */

/* Forge_CheckRecipe's list, its `where` and its result. */
#define FORGE_RECIPES_ARMOR     0   /* gArmorRecipes */
#define FORGE_RECIPES_WEAPON    1   /* gWeaponRecipes */
#define FORGE_COUNT_ALL         0   /* the materials counted in the pouch and the box */
#define FORGE_RECIPE_LOCKED     0
#define FORGE_RECIPE_SHOWN      1   /* unlocked (or always shown), materials missing */
#define FORGE_RECIPE_READY      2   /* unlocked, every material at hand */
#define FORGE_RECIPE_END        0xFF    /* ForgeRecipe.type of a recipe table's end */

/* A weapon recipe's id from here on is a random-weapon recipe's: its weapon
 * is rolled from gRandomEquips when it is made (WeaponForge_RollRandom). */
#define WEAPON_ID_RANDOM_FIRST  900

/* A skill and the points an item gives in it (ArmorData.skills). */
typedef struct SkillPoints {
    u8 skill;
    s8 points;
} SkillPoints;

/* An armour piece (0x28 bytes, by armour id): gArmorHeadData, gArmorBodyData, gArmorArmsData,
 * gArmorWaistData and gArmorLegsData (gArmorTables by armour type). */
typedef struct ArmorData {
    u8 modelMale;       /* 0x00: the part's model for a male hunter (Equip_GetModelId) */
    u8 modelFemale;     /* 0x01: for a female hunter */
    u8 flags;           /* 0x02: ARMOR_FLAG_*: who can wear it */
    u8 rarity;          /* 0x03: low 4 bits: Equip_GetRarity, the colour index (Equip_Color) */
    u32 price;          /* 0x04 */
    u8 defense;         /* 0x08 */
    s8 resist[5];       /* 0x09: resistances (EquipView_GetStat kinds 2-5, 13) */
    u8 defenseRow;      /* 0x0E: row of gArmorLevelDefense */
    u8 costRow;         /* 0x0F: row of gArmorLevelCost */
    u8 slots;           /* 0x10: decoration slots at level 1 (Equip_GetSlotCount) */
    u8 slotsMax;        /* 0x11: at most */
    u16 sortOrder;      /* 0x12: the order within its type in the sorted box (BoxEquip_SortKey's bits 16-31) */
    u8 maxLevel;        /* 0x14 */
    u8 slotRow;         /* 0x15: row of gArmorLevelSlots */
    u8 upgrade;         /* 0x16: row of gArmorUpgradeRows: the upgrade materials */
    u8 deriveLevel;     /* 0x17: from this level on, the piece can be made into derives' pieces */
    u16 derives[3];     /* 0x18: the armour ids (same type) it can be made into; 0 none */
    SkillPoints skills[5]; /* 0x1E */
} ArmorData;
typedef char ArmorData_size_check[(sizeof(ArmorData) == 0x28) ? 1 : -1];

/* ArmorData.flags: who can wear the piece (Equip_CanWear, Equip_GetWearMask,
 * Save_UnequipUnwearable). A decoration's DecoData.flags use the same two hunter-type bits. */
#define ARMOR_FLAG_MALE         0x01
#define ARMOR_FLAG_FEMALE       0x02
#define ARMOR_FLAG_BLADEMASTER  0x04
#define ARMOR_FLAG_GUNNER       0x08

/* What Equip_GetData returns: the data record of an item of equipment, by its type (EquipItem.type
 * EQUIP_BLADE, EQUIP_GUN, else an armour piece). */
typedef union EquipData {
    BladeData blade;
    GunData gun;
    ArmorData armor;
} EquipData;

/* A row of gGunLevelRows (GunData.levelRow), 24 bytes a level: the attack the level adds as the
 * level's first s16 (Gun_GetAttack); a bowgun's level has f32 values at +4 to +0x14, which its
 * shots read (BowgunShotBuf.levelRow). */
typedef union GunLevelRow {
    s16 *levels;        /* 12 s16 a level */
    u8 *bytes;          /* the level's bytes (a bowgun's f32 values) */
} GunLevelRow;

extern BladeData gBladeData[];
extern GunData gGunData[];
extern GunLevelRow gGunLevelRows[];
extern ArmorData gArmorHeadData[];
extern ArmorData gArmorBodyData[];
extern ArmorData gArmorArmsData[];
extern ArmorData gArmorWaistData[];
extern ArmorData gArmorLegsData[];

/*
 * Skills (Player_HasSkill, Player_GetSkill). SKILL_GROUP_GATHER is the group
 * Player_GetSkill tests; the four skills change how soon a gathering point
 * runs out (Gather_AtPoint: the chance in 32 a gather empties it, 4 without
 * one), and are named here by that effect.
 */
#define SKILL_GROUP_GATHER      0x16
#define SKILL_GATHER_PLUS1      0x93    /* chance 2 */
#define SKILL_GATHER_PLUS2      0x94    /* chance 0 */
#define SKILL_GATHER_MINUS1     0x95    /* chance 8 */
#define SKILL_GATHER_MINUS2     0x96    /* chance 12 */
/* The temperature skills (Player_StartIdleMotion): the first covers the hot or cold areas, the second the very hot or very cold ones too. */
#define SKILL_HEAT_RES          0x85
#define SKILL_HEAT_RES2         0x86
#define SKILL_COLD_RES          0x8A
#define SKILL_COLD_RES2         0x8B
/* The quest rewards' luck (Quest_RollRewards: the row of the rate tables; the first held wins, in this order: UP, DOWN, UP2, DOWN2). */
#define SKILL_LUCK_UP           0x9C    /* better chances for the main rewards' draws */
#define SKILL_LUCK_UP2          0x9D    /* better still */
#define SKILL_LUCK_DOWN         0x9E    /* worse chances */
#define SKILL_LUCK_DOWN2        0x9F    /* worse still */
#define SKILL_EXTRA_CARVE       0xBE    /* one more gather at each point placed at run time: a carcass's carves (MinePoint_Add) */

#define SKILL_COMBINE_SURE      0x48    /* recipes flagged RECIPE_SURE_WITH_SKILL always succeed */
#define SKILL_GROUP_COMBINE     0x25    /* the combination rate skills: */
#define SKILL_COMBINE_PLUS5     0xA5
#define SKILL_COMBINE_PLUS10    0xA6
#define SKILL_COMBINE_PLUS20    0xA7
#define SKILL_COMBINE_MINUS5    0xA8
#define SKILL_COMBINE_MINUS15   0xA9
#define SKILL_COMBINE_AMMO_FIXED 0xAA   /* ammo and coatings are made in the base quantity (Combine_HasFixedYield) */
#define SKILL_RECIPE_FLAG1      0xAB    /* recipes flagged 1 are usable with it (Recipe_CanUse) */

/* The health skills: the maximum health they add at the quest's start (Player_GetHpSkillBonus;
 * the first held wins, in this order). */
#define SKILL_HEALTH_PLUS10     0x15
#define SKILL_HEALTH_PLUS20     0x16
#define SKILL_HEALTH_PLUS30     0x17
#define SKILL_HEALTH_MINUS10    0x18
#define SKILL_HEALTH_MINUS20    0x19
#define SKILL_HEALTH_MINUS30    0x1A
/* The sharpness skills (Player_CalcSharpnessMax): the maximum sharpness +50 (to at most 400) or
 * -50 (to at least 150). */
#define SKILL_SHARPNESS_PLUS50  0x21
#define SKILL_SHARPNESS_MINUS50 0x22

/* The item skills (Item_Use, Player_Heal, Player_ScaleItemDuration). */
#define SKILL_ITEM_STAMINA      0x4E    /* any item used: 1 in 3 for maximum stamina +75 (Item_StaminaBonus) */
#define SKILL_FOOD_STAMINA      0x4D    /* a food eaten: always maximum stamina +75, instead of SKILL_ITEM_STAMINA's chance */
#define SKILL_GROUP_ITEM_SHARE  0x4E    /* the group of the two sharing skills: a teammate's item also reaches the local player */
#define SKILL_ITEM_SHARE_HALF   0x5D    /* the shared effect at half strength */
#define SKILL_ITEM_SHARE_FULL   0x5E    /* at full strength */
#define SKILL_HEAL_UP           0xA3    /* heals a quarter more */
#define SKILL_HEAL_DOWN         0xA4    /* heals a quarter less */
#define SKILL_ITEM_TIME_UP      0xB4    /* the items' timed effects last 1.5 times as long */
#define SKILL_ITEM_TIME_DOWN    0xB5    /* 0.66 times as long */
/* The status skills (Player_AddPoison, Item_Use's ITEM_ID_RANDOM, the
 * player's status updates in the game overlay). */
#define SKILL_POISON_HALF       0x0A    /* poison halved */
#define SKILL_POISON_IMMUNE     0x0B    /* no poison */
#define SKILL_POISON_DOUBLE     0x0C    /* poison doubled */
#define SKILL_STATUS6_HALF      0x0D    /* status 6 for 900 frames instead of 1800; poison halved too */
#define SKILL_STATUS6_IMMUNE    0x0E    /* no status 6; poison halved too */
#define SKILL_STATUS6_DOUBLE    0x0F    /* status 6 for 3600 frames */
#define SKILL_STATUS7_HALF      0xC4    /* status 7 for 900 frames instead of 1800, and status 8's stamina loss halved */
#define SKILL_STATUS7_IMMUNE    0xC5    /* no status 7 or 8 */

/* The attack and defense skills (Player_CalcAttack, Player_CalcDefense): the
 * first held of each list counts. */
#define SKILL_ATTACK_PLUS3      0x4F
#define SKILL_ATTACK_PLUS5      0x50
#define SKILL_ATTACK_PLUS10     0x51
#define SKILL_DEFENSE_PLUS20    0x52
#define SKILL_DEFENSE_PLUS30    0x53
#define SKILL_DEFENSE_PLUS40    0x54
#define SKILL_DEFENSE_MINUS20   0x55
#define SKILL_DEFENSE_MINUS30   0x56
#define SKILL_DEFENSE_MINUS40   0x57
/* At 40% health or less every player's defense gains 60; these change that. */
#define SKILL_LOW_HP_DEFENSE    0xB1    /* the defense gains 90 instead */
#define SKILL_LOW_HP_FURY       0xB2    /* the defense gains 90, and the attack is 1.5 times as high */
#define SKILL_LOW_HP_WEAKNESS   0xB3    /* the defense gains only 42, and the attack falls to 0.7 times */
/* The affinity skills (Player_GetWeaponStat's WSTAT_AFFINITY): +5, +10, +15.
 * The two higher ones also keep a hit from bouncing off a hard part: the hit
 * code turns a bouncing hit (hitKind 1) into a light one with either held,
 * and the lance's charge skips its sharpness recoil. */
#define SKILL_AFFINITY_PLUS5    0x23
#define SKILL_AFFINITY_PLUS10_NO_BOUNCE 0x24
#define SKILL_AFFINITY_PLUS15_NO_BOUNCE 0x25
/* Stops a monster stealing an item: the Chameleos's tongue, the Melynx's and
 * the Yian Kut-Ku's thefts test it. */
#define SKILL_THEFT_GUARD       0x5C
#define SKILL_STATUS_PLUS       0x47    /* the weapon's statuses and dragon element an eighth stronger */
/* The reload skills (Bowgun_ReloadLevel): the reload level less 1, 2 or 3, or plus 1 or 3. */
#define SKILL_RELOAD_PLUS1      0x2E
#define SKILL_RELOAD_PLUS2      0x2F
#define SKILL_RELOAD_PLUS3      0x30
#define SKILL_RELOAD_MINUS1     0x31
#define SKILL_RELOAD_MINUS3     0x32
/* A bow's or bowgun's shot uses no loaded round, and a bow charges faster
 * (Bow_ChargeLevel); the gun's own recoil and the recoil skills no longer
 * lower the recoil level (Bowgun_RecoilLevel). */
#define SKILL_FREE_SHOTS        0x33
/* The recoil skills (Bowgun_RecoilLevel): the recoil level less 2 or 4. */
#define SKILL_RECOIL_DOWN1      0x34
#define SKILL_RECOIL_DOWN2      0x35
#define SKILL_CARRY_BONUS       0x5F    /* Player_HasCarryBonus */
/* A bowgun's clip holds one more round (Bowgun_LoadAmmo), and a bow charges to
 * level 3 instead of 2 (Bow_ChargeLevel). */
#define SKILL_LOAD_UP           0xB8
/* Thrown items fly 1.5 times as fast (ItemAttack_SetupThrown), a thrown
 * item of type 1 bursts after 8 frames instead of 13 (ItemAttack_RunThrown),
 * and a boomerang is not used up one time in 8 (Player_StateThrowItem). */
#define SKILL_THROW_UP          0x2D

/*
 * The player's statuses (Player_ReactToHit, Player.status*Hit): in each
 * group of three, the first halves the status's length, the second keeps
 * the status off (a hit's buildup is dropped), the third doubles it.
 * Status 4: the recovery timer (Player_StateStatus4); status 2: the time
 * lying (Player_StateStatus2); status 5's DOUBLE doubles the presses the
 * stun's escape needs (Player_StateStun), and its HALF halves the buildup.
 */
#define SKILL_STATUS4_HALF      0x01
#define SKILL_STATUS4_IMMUNE    0x02
#define SKILL_STATUS4_DOUBLE    0x03
#define SKILL_STATUS2_HALF      0x04
#define SKILL_STATUS2_IMMUNE    0x05
#define SKILL_STATUS2_DOUBLE    0x06
#define SKILL_STATUS5_HALF      0x07
#define SKILL_STATUS5_IMMUNE    0x08
#define SKILL_STATUS5_DOUBLE    0x09
/* The mash status (Player.mashBuildup, escaped by pressing buttons): each
 * press counts twice (FAST) or half (SLOW) in Player_UpdateMashEscape, and
 * IMMUNE drops a hit's buildup. */
#define SKILL_MASH_FAST         0x10
#define SKILL_MASH_IMMUNE       0x11
#define SKILL_MASH_SLOW         0x12

/* The guard skills (Player_GuardHit): a guarded hit's force less 10 or 20,
 * or more 5 or 10, never with the heavy bowgun. GUARD_PLUS20 also lets the
 * guard take hits whose attribute 0x80 otherwise can't be guarded
 * (Player_CanGuardHit). These ids are skills; SKILL_GROUP_TORSO_* below
 * are skill groups, a different list. */
#define SKILL_GUARD_PLUS10      0x28
#define SKILL_GUARD_PLUS20      0x29
#define SKILL_GUARD_MINUS5      0x2A
#define SKILL_GUARD_MINUS10     0x2B
/* Guards on its own whenever the weapon is drawn and the player was free last
 * frame, as the heavy bowgun's shield does (Player_CanGuardHit). */
#define SKILL_AUTO_GUARD        0x2C
/* A hit's damage, one time in 4: 0.7 times (GUARD) or 1.3 times (WEAK). */
#define SKILL_DAMAGE_CHANCE_GUARD 0x58
#define SKILL_DAMAGE_CHANCE_WEAK  0x59
/* The wind's push: ignored for the weak wind (WEAK) or for both (ALL). */
#define SKILL_WIND_GUARD_WEAK   0x5A
#define SKILL_WIND_GUARD_ALL    0x5B
/* The roars: ignored for the weak roar (WEAK), for all but the strong roar
 * (MOST) or for every roar (ALL), as the roar-guard song's levels 1-3 do. */
#define SKILL_ROAR_GUARD_WEAK   0x8E
#define SKILL_ROAR_GUARD_MOST   0x8F
#define SKILL_ROAR_GUARD_ALL    0x90
#define SKILL_QUAKE_GUARD       0xC3    /* the quakes' stagger is ignored */

/*
 * The resistance skills (Player_CalcResists): +3, +5 or +10, or -3, -5 or
 * -10, to all five element resistances or to one. Of the three of a sign,
 * only the strongest held counts.
 */
#define SKILL_RES_ALL_PLUS3         0x60
#define SKILL_RES_ALL_PLUS5         0x61
#define SKILL_RES_ALL_PLUS10        0x62
#define SKILL_RES_ALL_MINUS3        0x63
#define SKILL_RES_ALL_MINUS5        0x64
#define SKILL_RES_ALL_MINUS10       0x65
#define SKILL_RES_FIRE_PLUS3        0x66
#define SKILL_RES_FIRE_PLUS5        0x67
#define SKILL_RES_FIRE_PLUS10       0x68
#define SKILL_RES_FIRE_MINUS3       0x69
#define SKILL_RES_FIRE_MINUS5       0x6A
#define SKILL_RES_FIRE_MINUS10      0x6B
#define SKILL_RES_WATER_PLUS3       0x6C
#define SKILL_RES_WATER_PLUS5       0x6D
#define SKILL_RES_WATER_PLUS10      0x6E
#define SKILL_RES_WATER_MINUS3      0x6F
#define SKILL_RES_WATER_MINUS5      0x70
#define SKILL_RES_WATER_MINUS10     0x71
#define SKILL_RES_DRAGON_PLUS3      0x72
#define SKILL_RES_DRAGON_PLUS5      0x73
#define SKILL_RES_DRAGON_PLUS10     0x74
#define SKILL_RES_DRAGON_MINUS3     0x75
#define SKILL_RES_DRAGON_MINUS5     0x76
#define SKILL_RES_DRAGON_MINUS10    0x77
#define SKILL_RES_THUNDER_PLUS3     0x78
#define SKILL_RES_THUNDER_PLUS5     0x79
#define SKILL_RES_THUNDER_PLUS10    0x7A
#define SKILL_RES_THUNDER_MINUS3    0x7B
#define SKILL_RES_THUNDER_MINUS5    0x7C
#define SKILL_RES_THUNDER_MINUS10   0x7D
#define SKILL_RES_ICE_PLUS3         0x7E
#define SKILL_RES_ICE_PLUS5         0x7F
#define SKILL_RES_ICE_PLUS10        0x80
#define SKILL_RES_ICE_MINUS3        0x81
#define SKILL_RES_ICE_MINUS5        0x82
#define SKILL_RES_ICE_MINUS10       0x83
#define SKILL_FISHING_BITE_X2       0xA0    /* a fish's bite lasts twice as long (Fish_StepBite) */

/*
 * Skill groups that change how the body armour's points count
 * (Player_GetTorsoSkillPoints): for each piece worn with points of
 * SKILL_GROUP_TORSO_UP, the body's points of every skill count once more;
 * then each piece with points of SKILL_GROUP_TORSO_PLUS1 adds 1, and of
 * SKILL_GROUP_TORSO_PLUS2 adds 2 (Equip_TorsoUpBonus).
 */
#define SKILL_GROUP_TORSO_PLUS1     0x29
#define SKILL_GROUP_TORSO_PLUS2     0x2A
#define SKILL_GROUP_TORSO_UP        0x2B

/* The end of the skill table, gSkillTable (SkillDef.group). */
#define SKILL_GROUP_END             0xFFFF

/* A skill group's thresholds (gSkillTable, ended by SKILL_GROUP_END): the
 * points a player's equipment gives in the group pick at most one skill. */
typedef struct SkillDef {
    u16 group;                  /* 0x0 */
    s8 neg[3][2];               /* 0x2: {points, skill}: at most that many points gives the skill (the lowest first) */
    s8 pos[3][2];               /* 0x8: {points, skill}: at least that many points gives the skill (the highest last) */
    u8 pad[2];
} SkillDef;

/* ItemData.type values seen (the bowgun's ammo slots, Combine_HasFixedYield). */
#define ITEM_TYPE_AMMO          2
#define ITEM_TYPE_COATING       3

/*
 * Item ids (gItemData's index) the code tests, named by what the code does
 * with them: Item_Use and its helpers, and Player_UseItem's choice of the
 * action (game overlay). Times are in frames (30 a second). "Shared": with
 * a SKILL_GROUP_ITEM_SHARE skill, the user's item also works on the local
 * player when the user is a teammate in the same area. The SUPPLY_ ids have
 * the effect of the id they are named after; the quest's results take them
 * back from the pouch (seen in gItemData: flags2 bit 2, Plsel_ResultsUpdate).
 * A _B id has the same effect as the one before it.
 */
/* Used with the short motion (PL_ACT_DRINK). */
#define ITEM_ID_HEAL_30             0x007   /* heals 30; shared (15 or 30) */
#define ITEM_ID_HEAL_50             0x008
#define ITEM_ID_MAX_HP_10           0x009   /* maximum health +10 */
#define ITEM_ID_MAX_HP_20           0x00A
#define ITEM_ID_CURE_POISON         0x00B   /* cures poison; shared */
#define ITEM_ID_REGEN               0x00C   /* regenTimer 9000 */
#define ITEM_ID_STAMINA_LOCK        0x00D   /* maximum stamina +75; staminaLockTimer 5400 */
#define ITEM_ID_STAMINA_LOCK_LONG   0x00E   /* +150; staminaLockTimer 10800 */
#define ITEM_ID_ATTACK_DRUG         0x00F   /* attackDrug 3 */
#define ITEM_ID_ATTACK_DRUG_STRONG  0x010   /* attackDrug 5 */
#define ITEM_ID_ATTACK_BURST        0x011   /* attackBuff 25 for 630 */
#define ITEM_ID_DEFENSE_DRUG        0x012   /* defenseDrug 6 */
#define ITEM_ID_DEFENSE_DRUG_STRONG 0x013   /* defenseDrug 10 */
#define ITEM_ID_DEFENSE_BURST       0x014   /* defenseBuff 50 for 630, and no flinching */
#define ITEM_ID_COOL_DRINK          0x015   /* coolDrinkTimer 18000 */
#define ITEM_ID_HOT_DRINK           0x016   /* hotDrinkTimer 18000 */
#define ITEM_ID_DETECT              0x017   /* detectTimer 300 */
#define ITEM_ID_HEAL_10_CURE        0x01A   /* heals 10 and cures poison */
#define ITEM_ID_MAX_HP_FULL         0x01B   /* maximum health +150, heals 150 */
#define ITEM_ID_MAX_ALL             0x01C   /* the same, and maximum stamina +450 */
#define ITEM_ID_MAX_STAMINA_75      0x01E   /* maximum stamina +75 */
#define ITEM_ID_HEAL_50_TEAM        0x022   /* heals 50, and the local player 50 when on the same side in the same area */
#define ITEM_ID_HEAL_20             0x0B7   /* heals 20; shared (10 or 20) */
#define ITEM_ID_CURE_CHANCE         0x0B8   /* cures poison one time in two */
#define ITEM_ID_POISON_GAMBLE       0x0C7   /* poisons, and maximum health +10 or -10 */
#define ITEM_ID_RANDOM              0x0C8   /* a random effect: poison and statuses 6 and 7, half the health, stamina -75, or another item's */
#define ITEM_ID_MAX_STAMINA_75_B    0x0CA
#define ITEM_ID_ATTACK_SHARED       0x0CE   /* attackBuff 10 for 5400; shared (5 or 10) */
#define ITEM_ID_DEFENSE_SHARED      0x0CF   /* defenseBuff 40 for 5400; shared (20 or 40) */
#define ITEM_ID_HURT_CURE_CHANCE    0x104   /* health -10, then cures poison one time in two */
#define ITEM_ID_SUPPLY_HEAL_30      0x292   /* not shared */
#define ITEM_ID_SUPPLY_MAX_STAMINA_75 0x293
#define ITEM_ID_SUPPLY_MAX_HP_FULL  0x298
/* Eaten, with the long motion (PL_ACT_EAT). */
#define ITEM_ID_FOOD_STAMINA_75     0x02F   /* maximum stamina +75 */
#define ITEM_ID_FOOD_STAMINA_150    0x030
#define ITEM_ID_FOOD_STAMINA_FULL   0x031   /* +450 */
#define ITEM_ID_FOOD_STAMINA_GAMBLE 0x032   /* +75 or -75 */
#define ITEM_ID_FOOD_HOT            0x033   /* hotDrinkTimer 9000 */
#define ITEM_ID_FOOD_HOT_STAMINA    0x034   /* the same, and maximum stamina +150 */
#define ITEM_ID_FOOD_COOL           0x035   /* coolDrinkTimer 9000 */
#define ITEM_ID_FOOD_COOL_STAMINA   0x036
#define ITEM_ID_FOOD_CURE           0x037   /* cures poison */
#define ITEM_ID_FOOD_CURE_STAMINA   0x038
#define ITEM_ID_FOOD_RESTORE        0x039   /* health back up to hpRecoverable */
#define ITEM_ID_FOOD_REGEN_SHORT    0x03A   /* the same, and regenTimer 3600 */
#define ITEM_ID_FOOD_REGEN          0x03B   /* regenTimer 7200 */
#define ITEM_ID_FOOD_REGEN_B        0x03C
#define ITEM_ID_FOOD_REGEN_LONG     0x03D   /* regenTimer 14400 */
#define ITEM_ID_FOOD_RESTORE_B      0x03E
#define ITEM_ID_FOOD_HEAL_20        0x0EB   /* heals 20 */
#define ITEM_ID_FOOD_PRIZE_A        0x0F7   /* a prize draw (gFoodPrizesA), or heals 20 */
#define ITEM_ID_FOOD_PRIZE_B        0x0F8   /* a prize draw (gFoodPrizesB), or heals 20 */
/* Set down, thrown or placed (Item_UseTool, Item_UsePlaced, Item_UseMisc). */
#define ITEM_ID_MEAT                0x028   /* the spit roast cooks one (Pouch_HasMeat); set down, a plain bait: a monster may go for it and eat it (EmObj kind 0 heals it) */
#define ITEM_ID_BAIT_POISON         0x02C   /* a bait of EmObj kind 1: poisons the monster */
#define ITEM_ID_BAIT_STATUS4        0x02D   /* kind 3: status 4 */
#define ITEM_ID_BAIT_STATUS2        0x02E   /* kind 2: status 2 */
#define ITEM_ID_PITFALL_TRAP        0x054   /* PitfallTrap_Create */
#define ITEM_ID_TRAP                0x055   /* PLACED_TRAP */
#define ITEM_ID_NO_BOMB_AREA_ITEM   0x056   /* ItemAttack type 0x16, set down only where barrel bombs are barred (Scene_InNoBarrelBombArea) */
#define ITEM_ID_SMALL_BARREL_BOMB   0x05B   /* ItemAttack type 4 */
#define ITEM_ID_SMALL_BARREL_BOMB_B 0x05C   /* ItemAttack type 0x10 */
#define ITEM_ID_LARGE_BARREL_BOMB   0x05D   /* PLACED_LARGE_BARREL_BOMB */
#define ITEM_ID_LARGE_BARREL_BOMB_B 0x05E   /* PLACED_LARGE_BARREL_BOMB_B */
#define ITEM_ID_LAUNCHED_BARREL_BOMB 0x05F  /* ItemAttack type 0x14 */
#define ITEM_ID_LAUNCHED_BARREL_BOMB_B 0x060 /* ItemAttack type 0x18 */
#define ITEM_ID_MONSTER_BOMB        0x061   /* PLACED_MONSTER_BOMB, a bomb set on a monster */
#define ITEM_ID_PLACED_BOMB         0x062   /* PLACED_BOMB; counts with the large barrel bombs (gScene.placed1) */
#define ITEM_ID_SUPPLY_TRAP         0x296
#define ITEM_ID_SUPPLY_PITFALL_TRAP 0x299
#define ITEM_ID_SUPPLY_LARGE_BARREL_BOMB 0x29A /* PLACED_LARGE_BARREL_BOMB_B */
#define ITEM_ID_DELIVERY            0x2A6   /* set down at a delivery spot (Quest_DeliverFromPouch); Item_UseMisc does nothing */
#define ITEM_ID_TRADE_GOODS         0x2A4   /* from the supply box, with its own notice; the trader
                                             * (EM_KIND_TRADER) takes it for item 0x2A5; held, it costs
                                             * health as ITEM_ID_DELIVERY does (Player_CountSpecialItems) */
/* The team items (PL_ACT_TEAM_ITEM_USE, Item_UseTeamPowder): on the user and
 * on the local player when on the same side (and, for a remote user, in the
 * same area). */
#define ITEM_ID_TEAM_HEAL           0x0B0   /* heals 20 */
#define ITEM_ID_TEAM_CURE           0x0B1   /* cures poison */
#define ITEM_ID_TEAM_ATTACK         0x0B2   /* attackBuff 10 for 5400 */
#define ITEM_ID_TEAM_DEFENSE        0x0B3   /* defenseBuff 40 for 5400 */
/* Not used: they work while in the pouch (Player_CalcAttack, Player_CalcDefense). */
#define ITEM_ID_POUCH_ATTACK_6      0x023   /* attack +6 */
#define ITEM_ID_POUCH_ATTACK_9      0x024   /* attack +9 */
#define ITEM_ID_POUCH_DEFENSE_16    0x025   /* defense +16 */
#define ITEM_ID_POUCH_DEFENSE_24    0x026   /* defense +24 */
/* Works while in the pouch: the recipes flagged RECIPE_NEEDS_SKILL are usable (Recipe_CanUse). */
#define ITEM_ID_RECIPE_UNLOCK       0x006
/* Used by the hunter's actions (Player_UseItem and the action states, game overlay):
 * the action each one starts. A letter ends the name of ids treated alike here, whose
 * difference lies in their data (an ItemAttack row, a gathering table ...). */
#define ITEM_ID_RETURNING_THROW          0x040   /* PL_ACT_THROW_ITEM; Player_StateThrowItem: Boomerang_Create, used up only on a 1 in 8 roll (skill 0x2D saves more); its own notice */
#define ITEM_ID_LOBBED_A                 0x041   /* PL_ACT_THROW_ITEM; ItemAttack type 0xC: lobbed (ItemAttack_LobSpeeds, falls faster after 15 frames) */
#define ITEM_ID_LOBBED_B                 0x042   /* as ITEM_ID_LOBBED_A, ItemAttack type 0xD */
#define ITEM_ID_LOBBED_C                 0x043   /* as ITEM_ID_LOBBED_A, ItemAttack type 0xE */
#define ITEM_ID_LOBBED_D                 0x044   /* as ITEM_ID_LOBBED_A, ItemAttack type 0xF */
#define ITEM_ID_LOBBED_NO_VERSUS         0x045   /* thrown only outside versus (Quest_IsVersus 0); ItemAttack type 0x15, lobbed */
#define ITEM_ID_THROWN_HIT_A             0x047   /* PL_ACT_THROW_ITEM; ItemAttack type 6 (a plain thrown hit; its effect is in its ItemAttack row) */
#define ITEM_ID_THROWN_AIR_BURST         0x048   /* PL_ACT_THROW_ITEM; ItemAttack type 1: bursts in flight after 13 frames (8 with skill 0x2D) */
#define ITEM_ID_THROWN_AIR_BURST_LATE    0x049   /* PL_ACT_THROW_ITEM; ItemAttack type 9: bursts in flight after 16 frames */
#define ITEM_ID_THROWN_GROUND_MARK_B     0x04B   /* PL_ACT_THROW_ITEM_ALT standing, PL_ACT_THROW_ITEM on the move; ItemAttack type 5: leaves a stage object where it lands */
#define ITEM_ID_THROWN_GROUND_MARK       0x04C   /* as ITEM_ID_THROWN_GROUND_MARK_B; ItemAttack type 0: leaves a stage object where it lands */
#define ITEM_ID_THROWN_GROUND_BURST      0x04D   /* PL_ACT_THROW_ITEM_ALT; ItemAttack type 0xB: on the floor a stage object and its burst */
#define ITEM_ID_RETURN_TO_CAMP           0x04E   /* only outside camp (Item_UsableOutsideCamp) -> PL_ACT_RETURN_TO_CAMP */
#define ITEM_ID_THROWN_GROUND_BURST_C    0x04F   /* PL_ACT_THROW_ITEM_ALT; ItemAttack type 0x17: on the floor a stage object and its burst */
#define ITEM_ID_THROWN_NO_VERSUS         0x050   /* thrown only outside versus; ItemAttack type 0xA */
#define ITEM_ID_FISH_BAIT_A              0x08E   /* at an action spot of kind 2 -> PL_ACT_FISH_CAST (Lure_Spawn type 0); used up on a catch or a monster bite */
#define ITEM_ID_FISH_BAIT_B              0x08F   /* as ITEM_ID_FISH_BAIT_A */
#define ITEM_ID_FISH_BAIT_C              0x090   /* as ITEM_ID_FISH_BAIT_A */
#define ITEM_ID_FISH_BAIT_LURE_ALT       0x091   /* as ITEM_ID_FISH_BAIT_A, but Player_StateFishCast spawns Lure_Spawn type 1 */
#define ITEM_ID_FISH_BAIT_D              0x092   /* as ITEM_ID_FISH_BAIT_A */
#define ITEM_ID_FISH_BAIT_E              0x093   /* as ITEM_ID_FISH_BAIT_A */
#define ITEM_ID_FISH_BAIT_F              0x094   /* as ITEM_ID_FISH_BAIT_A */
#define ITEM_ID_FISH_BAIT_G              0x095   /* as ITEM_ID_FISH_BAIT_A */
#define ITEM_ID_MINE_TOOL_WEAK           0x096   /* spot use (kind 3, mode 2) -> PL_ACT_MINE; refused at a bug spot; Player_StateGather: breaks 1 in 3; Player_UseItem does nothing (no refusal sound) */
#define ITEM_ID_MINE_TOOL_MID            0x097   /* as ITEM_ID_MINE_TOOL_WEAK, breaks 1 in 10 */
#define ITEM_ID_MINE_TOOL_STRONG         0x098   /* as ITEM_ID_MINE_TOOL_WEAK, breaks 1 in 15 */
#define ITEM_ID_NET_TOOL_WEAK            0x099   /* spot use (kind 4, mode 2) -> PL_ACT_BUG_NET; refused at a mine spot; Player_StateGather: breaks 1 in 3; Player_UseItem does nothing */
#define ITEM_ID_NET_TOOL_MID             0x09A   /* as ITEM_ID_NET_TOOL_WEAK, breaks 1 in 10 */
#define ITEM_ID_NET_TOOL_STRONG          0x09B   /* as ITEM_ID_NET_TOOL_WEAK, breaks 1 in 15 */
#define ITEM_ID_ROAST_TOOL_A             0x09C   /* needs a meat in the pouch (Pouch_HasMeat) -> PL_ACT_SPIT_ROAST_START */
#define ITEM_ID_ROAST_TOOL_QUICK         0x09D   /* as ITEM_ID_ROAST_TOOL_A, row 2; Player_StateSpitRoast's time limit is 180 frames instead of 330 */
#define ITEM_ID_ROAST_TOOL_C             0x09E   /* as ITEM_ID_ROAST_TOOL_A, row 3 */
#define ITEM_ID_ROAST_TOOL_D             0x09F   /* as ITEM_ID_ROAST_TOOL_A, row 4 */
#define ITEM_ID_ROAST_TOOL_E             0x0A0   /* as ITEM_ID_ROAST_TOOL_A, row 5 */
#define ITEM_ID_ROAST_LEFTOVER           0x0A2   /* Player_StateSpitRoast: Pouch_Add(+1) when the row's raw flag is set (the roasted item itself is used up and this given back) */
#define ITEM_ID_ROAST_SELF_A             0x0A3   /* PL_ACT_SPIT_ROAST_START with no meat check; SpitMeat_Kind row 10 (likely raw-flag rows: the item itself is roasted) */
#define ITEM_ID_ROAST_SELF_B             0x0A4   /* as ITEM_ID_ROAST_SELF_A, row 6 */
#define ITEM_ID_ROAST_SELF_C             0x0A5   /* as ITEM_ID_ROAST_SELF_A, row 7 */
#define ITEM_ID_ROAST_SELF_D             0x0A6   /* as ITEM_ID_ROAST_SELF_A, row 8 */
#define ITEM_ID_ROAST_SELF_E             0x0A7   /* as ITEM_ID_ROAST_SELF_A, row 9 */
#define ITEM_ID_ROAST_SELF_F             0x0A8   /* as ITEM_ID_ROAST_SELF_A, row 11 */
#define ITEM_ID_ROAST_SELF_G             0x0A9   /* as ITEM_ID_ROAST_SELF_A, row 12 */
#define ITEM_ID_SHARPEN_150              0x0AC   /* not for gunners -> PL_ACT_SHARPEN; Player_StateSharpen: sharpness +150 (half with skill 0x27), one used up */
#define ITEM_ID_SCOPE_VIEW               0x0AD   /* Player_UseItem (Player_LocalCanUseBinoculars) -> PL_ACT_BINOCULARS or _CROUCHED: the aim view; not used up here */
#define ITEM_ID_HORN_PLAY                0x0AF   /* PL_ACT_HORN_PLAY: up to 3 rounds; Player_StateInstrumentEnd uses it up 1 in 5 (skills and the hunting horn save it) */
#define ITEM_ID_THROWN_WIND_DEFLECTED    0x0D7   /* PL_ACT_THROW_ITEM; ItemAttack type 8 (ATKF_WIND_DEFLECTS: wind turns it away) */
#define ITEM_ID_SHARPEN_150_B            0x0EA   /* as ITEM_ID_SHARPEN_150 (+150) */
#define ITEM_ID_SHARPEN_200              0x0F2   /* as ITEM_ID_SHARPEN_150, sharpness +200 */
#define ITEM_ID_SUPPLY_ROAST_TOOL        0x294   /* as ITEM_ID_ROAST_TOOL_A, SpitMeat_Kind row 0; likely a SUPPLY_ id (between item.h's SUPPLY_ ids 0x292-0x29A) */
#define ITEM_ID_SUPPLY_SHARPEN_100       0x295   /* as ITEM_ID_SHARPEN_150, sharpness +100; likely a SUPPLY_ id (same range) */
#define ITEM_ID_SUPPLY_THROWN_AIR_BURST  0x297   /* same as ITEM_ID_THROWN_AIR_BURST (ItemAttack type 1); likely a SUPPLY_ id (same range) */
#define ITEM_ID_MOUNTED_GUN_AMMO         0x29B   /* at an action spot of kind 6 walks there -> PL_ACT_MOUNTED_GUN_AIM */
#define ITEM_ID_CANNON_AMMO              0x29C   /* Player_UpdateCarryInput: carried; PAD_CIRCLE at an action spot of kind 8 takes 10 (Pouch_Add -10) and starts PL_ACT_CANNON_FIRE */
#define ITEM_ID_DELIVERY_CARRIED         0x2A3   /* Player_GatherAtSpot: a gathered carried item (Item_CarryKind) with the delivery notice (0x14); Player_CountSpecialItems: held, it costs health */
#define ITEM_ID_MINE_CLOSER              0x2DA   /* held, it keeps the mine closed further: Mine_IsOpen is 0 (the pickaxes keep their first tables) */
#define ITEM_ID_BOX_UPGRADE_1            0x26D   /* sold in the shops and kept as obtained, not in the pouch: its purchase shows LOBBY_NOTICE_BOX_UPGRADE_1 (Shop_Buy) */
#define ITEM_ID_BOX_UPGRADE_2            0x26E   /* likewise, with LOBBY_NOTICE_BOX_UPGRADE_2 */
#define ITEM_ID_HOUSE_UPGRADE_1          0x26F   /* kept as obtained; its purchase raises VILLAGE_FACILITY_HOUSE by one (Shop_Buy) */
#define ITEM_ID_HOUSE_UPGRADE_2          0x270   /* as ITEM_ID_HOUSE_UPGRADE_1 */
#define ITEM_ID_HORN_ITEM_A              0x2B0   /* PL_ACT_HORN_ITEM_PLAY; Player_StateHornItemPlay: 2 rounds, used up 1 in 16 */
#define ITEM_ID_HORN_ITEM_B              0x2B1   /* as ITEM_ID_HORN_ITEM_A: 2 rounds, 1 in 16 (Side_HornItemPlaying 2) */
#define ITEM_ID_HORN_ITEM_SHORT          0x2B2   /* as ITEM_ID_HORN_ITEM_A: 1 round, 1 in 12 (Side_HornItemPlaying 3) */
#define ITEM_ID_HORN_ITEM_LONG           0x2B3   /* as ITEM_ID_HORN_ITEM_A: 3 rounds, 1 in 8 (Side_HornItemPlaying 4) */
#define ITEM_ID_HORN_ITEM_C              0x2B4   /* as ITEM_ID_HORN_ITEM_A: 2 rounds, 1 in 16 (Side_HornItemPlaying 5) */
#define ITEM_ID_HORN_ITEM_D              0x2B5   /* as ITEM_ID_HORN_ITEM_A: 2 rounds, 1 in 16 (Side_HornItemPlaying 6) */
#define ITEM_ID_HORN_ITEM_LONG_B         0x2B6   /* as ITEM_ID_HORN_ITEM_A: 3 rounds, 1 in 8 (Side_HornItemPlaying 7) */
#define ITEM_ID_SPOT_STRIKE_ITEM         0x2B7   /* at an action spot of kind 0xA -> PL_ACT_SPOT_ITEM_STRIKE (StageShot_CreateForPlayer type 7, one used up) */
#define ITEM_ID_THROWN_GROUND_BURST_B    0x2BA   /* PL_ACT_THROW_ITEM_ALT; ItemAttack type 0x11: on the floor a stage object and its burst */
#define ITEM_ID_THROWN_NO_PLAYER_HIT     0x2BB   /* PL_ACT_THROW_ITEM; ItemAttack type 0x12 (ATKF_NO_PLAYER: hits no player) */
#define ITEM_ID_THROWN_HIT_B             0x2BC   /* PL_ACT_THROW_ITEM; ItemAttack type 0x13 (a plain thrown hit) */
#define ITEM_ID_SPECIAL_MINE_A           0x2F8   /* when SpecialMine_IsFree -> PL_ACT_SET_TRAP; Item_UseMisc: SpecialMine_Create, one used up (all 28 special mine items below are treated alike) */
#define ITEM_ID_SPECIAL_MINE_B           0x2F9   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_C           0x2FA   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_D           0x2FB   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_E           0x2FC   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_F           0x2FF   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_G           0x300   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_H           0x301   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_I           0x306   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_J           0x307   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_K           0x308   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_L           0x309   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_M           0x30A   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_N           0x30B   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_O           0x30C   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_P           0x30D   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_Q           0x30E   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_R           0x30F   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_S           0x310   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_T           0x311   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_U           0x312   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_V           0x313   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_W           0x314   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_X           0x315   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_Y           0x316   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_Z           0x317   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_AA          0x318   /* as ITEM_ID_SPECIAL_MINE_A */
#define ITEM_ID_SPECIAL_MINE_AB          0x319   /* as ITEM_ID_SPECIAL_MINE_A */

/* The colour of the glow an item's use shows on the player (Effect06_SpawnAtBone's
 * third argument, with type 0, as Item_Use and Item_UseTeamPowder pick it). */
enum ItemGlow {
    ITEM_GLOW_HEAL = 0,
    ITEM_GLOW_CURE = 1,
    ITEM_GLOW_ATTACK = 2,
    ITEM_GLOW_DEFENSE = 3,
    ITEM_GLOW_STAMINA = 4,
    ITEM_GLOW_COOL = 5,
    ITEM_GLOW_HOT = 6,
    ITEM_GLOW_REGEN = 7,
    ITEM_GLOW_DETECT = 9
};

/* A range of weapon kinds, lo to hi (gDualSwordRanges: the dual swords; lo -1 ends the list). */
typedef struct WeaponKindRange {
    s16 lo;
    s16 hi;
} WeaponKindRange;

EquipData *Equip_GetData(EquipItem *item);

#endif /* ITEM_H */
