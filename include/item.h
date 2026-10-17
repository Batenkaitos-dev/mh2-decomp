#ifndef ITEM_H
#define ITEM_H

#include "common.h"

/*
 * Item data: sub_main's table at 0x45F110, 0x18 bytes per item id (0 is no
 * item; ids go up to 0x398). Item names are in the table at 0x464770.
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
    s32 price;          /* 0x0C: the buy price (Shop_GetBuyPrice: halved online in a kind-4 village); the lobby's Item_SellPrice gives half of it */
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
    u8 pad2[6];
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
    u8 elementType;     /* 0x0E: element 1-5, 0 none (Weapon_ElementValue) */
    u8 elementValue;    /* 0x0F */
    u8 statusType;      /* 0x10: status 1-3, 0 none (Weapon_StatusValue) */
    u8 statusValue;     /* 0x11 */
    u8 slots;           /* 0x12: decoration slots (Equip_GetSlotCount) */
    u8 special;         /* 0x13: a gunlance's shelling type, a horn's notes (Equip_GetBladeSpecial,
                         * Player.bladeSpecial) */
    u8 forgeCond;       /* 0x14: the condition that lets it be forged (FORGE_COND_*) */
    u8 unk15[3];        /* 0x15: [0] goes in BoxEquip_SortKey's bits 16-23 */
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
    u8 unk0E;           /* 0x0E: goes in BoxEquip_SortKey's bits 16-23 */
    u8 clipRow;         /* 0x0F: row of the clip size table (Bowgun_LoadAmmo) */
    u8 elementType;     /* 0x10: element 1-5, 0 none (Player_GetWeaponStat) */
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
    u16 unk12;          /* 0x12: goes in BoxEquip_SortKey's bits 16-31 */
    u8 maxLevel;        /* 0x14 */
    u8 slotRow;         /* 0x15: row of gArmorLevelSlots */
    u8 upgrade;         /* 0x16: row of gArmorUpgradeRows: the upgrade materials */
    u8 unk17;
    u16 unk18[3];
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
#define ITEM_ID_MEAT                0x028   /* the spit roast cooks one (Pouch_HasMeat); set down, a plain bait: a monster may go for it and eat it (EmAction kind 0 heals it) */
#define ITEM_ID_BAIT_POISON         0x02C   /* a bait of EmAction kind 1: poisons the monster */
#define ITEM_ID_BAIT_STATUS4        0x02D   /* kind 3: status 4 */
#define ITEM_ID_BAIT_STATUS2        0x02E   /* kind 2: status 2 */
#define ITEM_ID_PITFALL_TRAP        0x054   /* PitfallTrap_Create */
#define ITEM_ID_TRAP                0x055   /* PlacedItem kind 4 */
#define ITEM_ID_NO_BOMB_AREA_ITEM   0x056   /* ItemAttack type 0x16, set down only where barrel bombs are barred (Scene_InNoBarrelBombArea) */
#define ITEM_ID_SMALL_BARREL_BOMB   0x05B   /* ItemAttack type 4 */
#define ITEM_ID_SMALL_BARREL_BOMB_B 0x05C   /* ItemAttack type 0x10 */
#define ITEM_ID_LARGE_BARREL_BOMB   0x05D   /* PlacedItem kind 0 */
#define ITEM_ID_LARGE_BARREL_BOMB_B 0x05E   /* PlacedItem kind 1 */
#define ITEM_ID_LAUNCHED_BARREL_BOMB 0x05F  /* ItemAttack type 0x14 */
#define ITEM_ID_LAUNCHED_BARREL_BOMB_B 0x060 /* ItemAttack type 0x18 */
#define ITEM_ID_MONSTER_BOMB        0x061   /* PlacedItem kind 5, a bomb set on a monster */
#define ITEM_ID_PLACED_BOMB         0x062   /* PlacedItem kind 6; counts with the large barrel bombs (gScene.placed1) */
#define ITEM_ID_SUPPLY_TRAP         0x296
#define ITEM_ID_SUPPLY_PITFALL_TRAP 0x299
#define ITEM_ID_SUPPLY_LARGE_BARREL_BOMB 0x29A /* PlacedItem kind 1 */
#define ITEM_ID_DELIVERY            0x2A6   /* set down at a delivery spot (Quest_DeliverFromPouch); Item_UseMisc does nothing */
/* The team items (PL_ACT_TEAM_ITEM_USE, Item_UseTeamPowder): on the user and
 * on the local player when on the same side (and, for a remote user, in the
 * same area). */
#define ITEM_ID_TEAM_HEAL           0x0B0   /* heals 20 */
#define ITEM_ID_TEAM_CURE           0x0B1   /* cures poison */
#define ITEM_ID_TEAM_ATTACK         0x0B2   /* attackBuff 10 for 5400 */
#define ITEM_ID_TEAM_DEFENSE        0x0B3   /* defenseBuff 40 for 5400 */

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
