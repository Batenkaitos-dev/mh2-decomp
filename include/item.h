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
    u8 max;             /* 0x03: most the pouch holds; 0xFF: unlimited */
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

/* An item and a count: a pouch slot (Player.pouch) or a supply box slot
 * (SceneState.supply). */
typedef struct ItemStack {
    u16 item;           /* 0x0: 0 = none */
    s16 count;          /* 0x2 */
} ItemStack;

/*
 * An equipment item (12 bytes): an entry of the equipment box and of the
 * equipped items in the save data (SaveData.box, .equip), and a piece the
 * player wears (Player.equip).
 */
typedef struct EquipItem {
    u8 used;            /* 0x0 */
    u8 type;            /* 0x1: 0-5 armour (0, 2-5 used), 6 blade weapon, 7 gun */
    u16 id;             /* 0x2: 0 = none */
    u16 level;          /* 0x4: upgrade level; guns: level in bits 0-3, options 0x10-0x80 */
    u16 deco[3];        /* 0x6: decorations in the slots (item ids), 0 = empty */
} EquipItem;

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
    u8 statusType;      /* 0x10: status 1-3, 0 none (Weapon_Element2Value) */
    u8 statusValue;     /* 0x11 */
    u8 slots;           /* 0x12: decoration slots (Equip_GetSlotCount) */
    u8 unk13;           /* 0x13: Equip_GetBladeSpecial */
    u8 unk14;           /* 0x14: forging condition (Forge_CheckCondition) */
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
    u8 unk13;           /* 0x13: forging condition (Forge_CheckCondition) */
    u32 ammoMask;       /* 0x14: the ammo kinds it loads (Bowgun_GetAmmoMask) */
} GunData;
typedef char GunData_size_check[(sizeof(GunData) == 0x18) ? 1 : -1];

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
    u8 flags;           /* 0x02: 1 male, 2 female, 4 blademaster, 8 gunner */
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

/* What Equip_GetData returns: the data record of an item of equipment, by its type (EquipItem.type
 * 6 a blade, 7 a gun, else an armour piece). */
typedef union EquipData {
    BladeData blade;
    GunData gun;
    ArmorData armor;
} EquipData;

/* A row of gGunLevelRows (GunData.levelRow), 24 bytes a level: the attack the level adds as the
 * level's first s16 (Gun_GetAttack); a bow's level has f32 values at +4 to +0x14 (the arrow's,
 * Eff06Buf.levelRow). */
typedef union GunLevelRow {
    s16 *levels;        /* 12 s16 a level */
    u8 *bytes;          /* the level's bytes (a bow's f32 values) */
} GunLevelRow;

extern BladeData gBladeData[];
extern GunData gGunData[];
extern GunLevelRow gGunLevelRows[];
extern ArmorData gArmorHeadData[];
extern ArmorData gArmorBodyData[];
extern ArmorData gArmorArmsData[];
extern ArmorData gArmorWaistData[];
extern ArmorData gArmorLegsData[];

EquipData *Equip_GetData(EquipItem *item);

#endif /* ITEM_H */
