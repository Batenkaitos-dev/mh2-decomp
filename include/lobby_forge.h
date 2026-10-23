#ifndef LOBBY_FORGE_H
#define LOBBY_FORGE_H

/* The town workshop's three counters: the armour forge (ArmorForge_*), the
 * decorations counter (Deco_*) and the weapon forge (WeaponForge_*).
 *
 * The workshop's menu (Workshop_Open, Workshop_Update) keeps its own header
 * at the start of gMenuWork; the counter it opens keeps its work right after
 * it, at gMenuWork + 0xC (FORGE_WORK_OFFSET). Each counter has a top choice
 * (make, or change what the box holds), a list of recipes, the box's grid of
 * pieces, a detail view of the piece under the cursor, a model preview of
 * it and a closing question about its price and whether to wear the result.
 * The recipes are the game's tables gArmorRecipes and gWeaponRecipes
 * (ForgeRecipe) and gDecoRecipes (DecoRecipe). */

#include "common.h"
#include "item.h"
#include "lobby_menu.h"

#define FORGE_WORK_OFFSET 0xC   /* a counter's work in gMenuWork, after the workshop's header */
#define FORGE_LIST_ROWS 7       /* the recipe list's rows a page */
#define FORGE_RECIPE_MATS 4     /* ForgeRecipe.mat: a recipe's materials */
#define DECO_RECIPE_MATS 4      /* DecoRecipe.need: a decoration recipe's materials */
#define WEAPON_UP_MATS 3        /* WeaponMats.need: a weapon upgrade's materials */
#define FORGE_UPGRADE_LINES 4   /* a piece's upgrades (WeaponUp.next, the upgrade list's lines) */
#define ARMOR_UP_MATS 4         /* ArmorMats.need: an armour upgrade's materials */
#define ARMOR_TAB_COUNT 5       /* the armour types the making pages list (gEquipTabTypes) */

/* WeaponForgeWork.choice and ArmorForgeWork.choice: the forge's first list. */
#define FORGE_CHOICE_MAKE       0
#define FORGE_CHOICE_UPGRADE    1

/* WeaponForgeWork.step and ArmorForgeWork.step: the forge's pages. */
enum ForgeStep {
    FORGE_STEP_CHOOSE = 0,      /* the make-or-upgrade list */
    FORGE_STEP_UPGRADE = 1,     /* the upgrade pages: the box, then the piece's upgrades */
    FORGE_STEP_MAKE = 2         /* the making pages: the tab, then its recipes */
};

/* WeaponForgeWork.subStep and ArmorForgeWork.state on the making pages. */
enum ForgeMakeState {
    FORGE_MAKE_TAB = 0,         /* the tab list: a weapon class or an armour type */
    FORGE_MAKE_SLIDE_IN = 1,    /* the recipe list's window slides in */
    FORGE_MAKE_LIST = 2,        /* the recipe list */
    FORGE_MAKE_ASK = 3          /* the price question */
};

/* WeaponForgeWork.subStep and ArmorForgeWork.state on the upgrade pages. */
enum ForgeUpgradeState {
    FORGE_UPGRADE_SLIDE_IN = 0, /* both windows slide in */
    FORGE_UPGRADE_BOX = 1,      /* the box's grid */
    FORGE_UPGRADE_LIST = 2,     /* the chosen piece's upgrades (a bowgun's mods) */
    FORGE_UPGRADE_ASK = 3       /* the price question */
};

/* The making pages' detail view (WeaponForgeWork.view, ArmorForgeWork.view):
 * square steps through STATS to HUNTER, cross goes back to LIST. */
enum ForgeMakeView {
    FORGE_MAKE_VIEW_LIST = 0,       /* the recipe list alone, at its full width */
    FORGE_MAKE_VIEW_STATS = 1,      /* the recipe's piece: its stats and model */
    FORGE_MAKE_VIEW_MATERIALS = 2,  /* its materials and model */
    FORGE_MAKE_VIEW_COMPARE = 3,    /* compared with the piece worn (the price question's too) */
    FORGE_MAKE_VIEW_HUNTER = 4,     /* the hunter's stats with it worn */
    FORGE_MAKE_VIEW_COUNT = 5
};

/* The upgrade list's detail view: square steps through them (a bowgun's mod
 * list has only STATS and COMPARE). In the box's grid the view is 0 for the
 * grid alone and 1 with the piece's detail (FORGE_BOX_VIEW_*). */
enum ForgeUpgradeView {
    FORGE_UPGRADE_VIEW_STATS = 0,
    FORGE_UPGRADE_VIEW_MATERIALS = 1,
    FORGE_UPGRADE_VIEW_COMPARE = 2,     /* compared with the box piece (the price question's too) */
    FORGE_UPGRADE_VIEW_HUNTER = 3,
    FORGE_UPGRADE_VIEW_COUNT = 4
};
#define FORGE_BOX_VIEW_GRID     0
#define FORGE_BOX_VIEW_DETAIL   1

/* WeaponForgeWork.step while the weapon forge's closing question runs
 * (WeaponForge_MakeConfirm, WeaponForge_UpgradeConfirm, WeaponForge_DrawPreview). */
enum WeaponForgeConfirm {
    WEAPON_CONFIRM_SETUP = 0,
    WEAPON_CONFIRM_ASK = 1,         /* whether to wear the new weapon */
    WEAPON_CONFIRM_ASK_CHANGE = 2,  /* the same, warning that the worn weapon's type changes */
    WEAPON_CONFIRM_APPLY = 3        /* the weapon is made or upgraded */
};

/* ArmorForgeWork.step while the armour forge's closing question runs
 * (ArmorForge_MakeConfirm, ArmorForge_UpgradeConfirm, ArmorForge_DrawEquipPrompt). */
enum ArmorForgeConfirm {
    ARMOR_CONFIRM_SETUP = 0,
    ARMOR_CONFIRM_ASK = 1,          /* whether to wear the new piece */
    ARMOR_CONFIRM_APPLY = 2         /* the piece is made or upgraded */
};

/* WeaponForgeWork.row on the making pages: the weapon tabs, each two weapon
 * classes (WeaponForge_IsListed), the bowguns and bows last. */
enum WeaponForgeTab {
    WEAPON_TAB_GREAT_LONG = 0,      /* WEAPON_GREAT_SWORD, WEAPON_LONG_SWORD */
    WEAPON_TAB_SWORDS = 1,          /* WEAPON_SWORD_SHIELD, WEAPON_DUAL_SWORDS */
    WEAPON_TAB_HAMMER_HORN = 2,     /* WEAPON_HAMMER, WEAPON_HUNTING_HORN */
    WEAPON_TAB_LANCES = 3,          /* WEAPON_LANCE, WEAPON_GUNLANCE */
    WEAPON_TAB_GUNS = 4,            /* the bowguns and the bow: every EQUIP_GUN recipe */
    WEAPON_TAB_COUNT = 5
};

#define WEAPON_RANDOM_PRICE     1000    /* zenny: a random-weapon recipe's price (Forge_IsRandomWeapon) */

/* A bowgun's mods on the upgrade pages (WeaponForgeWork.mod), each applied by
 * WeaponForge_ApplyGunMod and Box_SetGun*. Each is also the index of its label
 * in gForgeTexts (FORGE_TEXT_GUN_MODS), and its help text
 * follows GUN_MOD_HELP_OFFSET entries later. The barrel is a light bowgun's
 * (BARREL) or a heavy one's (HEAVY_BARREL). */
enum GunMod {
    GUN_MOD_NONE = -1,
    GUN_MOD_LEVEL_UP = 0x13,
    GUN_MOD_FIT_SILENCER = 0x14,
    GUN_MOD_REMOVE_SILENCER = 0x15,
    GUN_MOD_FIT_BARREL = 0x16,
    GUN_MOD_REMOVE_BARREL = 0x17,
    GUN_MOD_FIT_SCOPE = 0x18,
    GUN_MOD_REMOVE_SCOPE = 0x19,
    GUN_MOD_FIT_SHIELD = 0x1A,
    GUN_MOD_REMOVE_SHIELD = 0x1B,
    GUN_MOD_FIT_HEAVY_BARREL = 0x1C,
    GUN_MOD_REMOVE_HEAVY_BARREL = 0x1D
};
#define GUN_MOD_HELP_OFFSET     11
#define GUN_MOD_LINES           4       /* WeaponForgeWork.mod's lines */
#define GUN_MOD_REMOVE_FEE      10      /* zenny: what a removal costs */

/* Box_SetGun*'s last argument. */
#define BOX_GUN_FIT             0
#define BOX_GUN_REMOVE          1

/* A forging recipe (0x18 bytes), as sub_main's own copy reads it. */
typedef struct ForgeRecipe {
    /* 0x00 */ u8 type;             /* gWeaponRecipes: EQUIP_BLADE or EQUIP_GUN */
    /* 0x01 */ u8 alwaysShown;      /* nonzero: Forge_CheckRecipe gives 1 even while locked */
    /* 0x02 */ u16 id;
    /* 0x04 */ ItemStack mat[FORGE_RECIPE_MATS];    /* the materials; a random recipe's first one is its key (RandEquip) */
    /* 0x14 */ u8 cond[4];          /* per material, the condition that unlocks the recipe */
} ForgeRecipe;
extern ForgeRecipe gArmorRecipes[];
extern ForgeRecipe gWeaponRecipes[];

/* The materials of a weapon upgrade. */
typedef struct WeaponMats {
    /* 0x0 */ ItemStack need[WEAPON_UP_MATS];
} WeaponMats;

/* A weapon's upgrade entry (gBladeUpgrades for blades, gGunUpgrades for
 * guns, by id). */
typedef struct WeaponUp {
    /* 0x00 */ WeaponMats mats;     /* what an upgrade into this weapon takes */
    /* 0x0C */ u16 next[FORGE_UPGRADE_LINES];       /* what it can become */
    /* 0x14 */ u8 unk14[4];
} WeaponUp;

/* The materials of an armour upgrade. */
typedef struct ArmorMats {
    /* 0x00 */ ItemStack need[ARMOR_UP_MATS];
    /* 0x10 */ u8 unk10[4];         /* never read: an upgrade copies only need */
} ArmorMats;

/* The weapon forge's work (gMenuWork + FORGE_WORK_OFFSET): forging from
 * gWeaponRecipes (makeList) or upgrading a weapon of the box (upgrades,
 * their materials and prices, or a gun's mods). */
typedef struct WeaponForgeWork {
    /* 0x000 */ EquipItem shown;        /* the piece whose model the preview has loaded */
    /* 0x00C */ EquipItem result;       /* forging: the random weapon rolled; upgrades: the upgrade at the cursor, or the gun with the cursor's mod */
    /* 0x018 */ WeaponUp *upgradeEntry; /* the box weapon's upgrade entry */
    /* 0x01C */ WinSlide detailWin;     /* the right window (the list, the box) */
    /* 0x024 */ WinSlide panelWin;      /* the left window (the upgrades) */
    /* 0x02C */ s32 modCost[GUN_MOD_LINES];     /* a gun mod line's price */
    /* 0x03C */ s32 mod[GUN_MOD_LINES];     /* each gun mod line's GunMod, GUN_MOD_NONE for none */
    /* 0x04C */ EquipItem upgrades[FORGE_UPGRADE_LINES]; /* what the box weapon can become */
    /* 0x07C */ u8 unk7C[0xC];
    /* 0x088 */ u32 upgradeCost[FORGE_UPGRADE_LINES]; /* half the upgrade's price */
    /* 0x098 */ u8 unk98[4];
    /* 0x09C */ WeaponMats mats[FORGE_UPGRADE_LINES]; /* each upgrade's materials */
    /* 0x0CC */ u8 unkCC[0xC];
    /* 0x0D8 */ s32 makeList[FORGE_LIST_ROWS]; /* forging: a page of gWeaponRecipes indexes */
    /* 0x0F4 */ s16 choice;             /* FORGE_CHOICE_MAKE or FORGE_CHOICE_UPGRADE */
    /* 0x0F6 */ s16 unkF6;
    /* 0x0F8 */ s16 page;               /* the list's page; the box's: slot = page * BOX_PAGE_SLOTS + row * 10 + col */
    /* 0x0FA */ s16 pages;
    /* 0x0FC */ s16 detailPage;
    /* 0x0FE */ s16 detailPages;
    /* 0x100 */ s16 col;
    /* 0x102 */ s16 row;                /* forging: the weapon class tab; upgrades: the box's row */
    /* 0x104 */ s16 cursor;
    /* 0x106 */ s16 count;
    /* 0x108 */ s16 view;               /* the detail view shown */
    /* 0x10A */ u16 gunLevel;           /* the box gun's level and mods before a mod is added */
    /* 0x10C */ s8 step;                /* the forge's step; the closing question's own while it runs */
    /* 0x10D */ s8 subStep;             /* a step within the screen */
    /* 0x10E */ s8 wear;                /* 1: the made or upgraded weapon is worn after */
    /* 0x10F */ s8 upgradeKind;         /* EQUIP_BLADE: through an upgrade entry; EQUIP_GUN: mods */
    /* 0x110 */ s8 previewOff;          /* 1: the model preview hidden (select toggles it) */
    /* 0x111 */ s8 filter;              /* 1: only gForgeRecipeList's recipes, and cancel leaves the whole forge */
} WeaponForgeWork;

/* The armour forge's work (gMenuWork + FORGE_WORK_OFFSET): making armour from
 * gArmorRecipes (makeList, row the armour type) or upgrading a piece of the
 * box (boxPiece, upgrades, mats, cost). */
typedef struct ArmorForgeWork {
    /* 0x000 */ EquipItem shown;        /* the piece whose model the preview has loaded */
    /* 0x00C */ EquipItem worn;         /* the piece of that type being worn */
    /* 0x018 */ EquipItem boxPiece;     /* upgrades: the chosen box piece */
    /* 0x024 */ EquipItem result;       /* upgrades: the upgrade at the cursor */
    /* 0x030 */ EquipItem upgrades[FORGE_UPGRADE_LINES];  /* what the box piece can become */
    /* 0x060 */ ArmorMats mats[FORGE_UPGRADE_LINES];
    /* 0x0B0 */ s32 cost[FORGE_UPGRADE_LINES];
    /* 0x0C0 */ u8 unkC0[4];            /* cleared with the upgrades, never read */
    /* 0x0C4 */ WinSlide listWin;       /* the right window: the box, the recipe list */
    /* 0x0CC */ WinSlide upgradeWin;    /* the left window: the upgrades */
    /* 0x0D4 */ u8 unkD4[0x20];
    /* 0x0F4 */ s32 makeList[FORGE_LIST_ROWS]; /* making: a page of gArmorRecipes indexes */
    /* 0x110 */ s32 pieceType;          /* upgrades: the box piece's EquipType (written, not read) */
    /* 0x114 */ s16 col;                /* upgrades: the box's column */
    /* 0x116 */ s16 row;                /* making: the armour type (gEquipTabTypes); upgrades: the box's row */
    /* 0x118 */ s16 cursor;
    /* 0x11A */ s16 count;
    /* 0x11C */ s16 view;               /* the detail view shown */
    /* 0x11E */ s16 page;               /* the list's page; the box's: slot = page * BOX_PAGE_SLOTS + row * 10 + col */
    /* 0x120 */ s16 pages;
    /* 0x122 */ s16 detailPage;
    /* 0x124 */ s16 detailPages;
    /* 0x126 */ s16 choice;             /* FORGE_CHOICE_MAKE or FORGE_CHOICE_UPGRADE */
    /* 0x128 */ u8 unk128[7];
    /* 0x12F */ s8 step;                /* the forge's step; the closing question's own while it runs */
    /* 0x130 */ s8 state;               /* the screen's state */
    /* 0x131 */ s8 wear;                /* 1: the new piece is worn after */
    /* 0x132 */ s8 previewOff;          /* 1: the model preview hidden (select toggles it) */
    /* 0x133 */ s8 filter;              /* 1: only gForgeRecipeList's recipes, and cancel leaves the whole forge */
} ArmorForgeWork;

/* A decoration recipe's material (gDecoRecipes). */
typedef struct DecoMat {
    u16 item;
    u8 count;
    u8 cond;    /* a condition (Forge_CheckCondition): the recipe stays hidden until it holds or the material has been obtained */
} DecoMat;
/* A decoration recipe (gDecoRecipes, 0x12 bytes). */
typedef struct DecoRecipe {
    u16 deco;           /* the decoration made (an item id) */
    DecoMat need[DECO_RECIPE_MATS];
} DecoRecipe;
s32 Deco_CanMake(s32 recipeIdx, s32 quantity);
s32 Item_HalfPrice(u16 item);
s32 Deco_GetAttachFee(u16 item);
s32 Deco_GetRemoveFee(u16 item);
extern DecoRecipe gDecoRecipes[];

/* gRandomEquips' entry: a random-weapon recipe's three weapons
 * (WeaponForge_RollRandom). */
typedef struct RandEquip {
    /* 0x0 */ u16 key;      /* the recipe's first material */
    /* 0x2 */ s8 type;      /* EQUIP_BLADE or EQUIP_GUN */
    /* 0x3 */ u8 pad3;
    /* 0x4 */ s16 id[3];    /* rolled with chances of 5, 20 and 75 percent */
} RandEquip;

/* DecoWork.choice: the decorations counter's top menu, in its rows' order. */
#define DECO_CHOICE_MAKE        0
#define DECO_CHOICE_ATTACH      1
#define DECO_CHOICE_REMOVE      2
#define DECO_CHOICE_COUNT       3

/* DecoWork.mode: the screen shown (Deco_Input). */
enum DecoMode {
    DECO_MODE_MENU = 0,         /* the top menu */
    DECO_MODE_MAKE = 1,
    DECO_MODE_ATTACH = 2,
    DECO_MODE_REMOVE = 3
};

/* DecoWork.mode while the wear question runs, after an attach or a removal
 * (Deco_WearAskInput). The question starts at DECO_WEAR_SETUP, the menu's
 * value, and sets the mode back to DECO_MODE_ATTACH or DECO_MODE_REMOVE
 * once the change is made. */
enum DecoWearStep {
    DECO_WEAR_SETUP = 0,
    DECO_WEAR_ASK = 1,          /* whether to wear the changed piece */
    DECO_WEAR_ASK_CHANGE = 2,   /* the same, warning that the worn weapon's type changes */
    DECO_WEAR_APPLY = 3         /* the change is made */
};

/* DecoWork.step on the make screen (Deco_MakeInput). */
enum DecoMakeStep {
    DECO_MAKE_SLIDE_IN = 0,     /* the window slides in */
    DECO_MAKE_LIST = 1,         /* the recipe list */
    DECO_MAKE_QUANTITY = 2,     /* how many to make */
    DECO_MAKE_ASK = 3           /* the price question */
};

/* DecoWork.step on the attach screen (Deco_AttachInput). */
enum DecoAttachStep {
    DECO_ATTACH_SLIDE_IN = 0,       /* the box's window slides in */
    DECO_ATTACH_BOX = 1,            /* the box: a piece with a free slot */
    DECO_ATTACH_LIST_SLIDE_IN = 2,  /* the owned decorations' list slides in */
    DECO_ATTACH_LIST = 3,           /* the list: a decoration */
    DECO_ATTACH_ASK = 4             /* the fee question */
};

/* DecoWork.step on the remove screen (Deco_RemoveInput). */
enum DecoRemoveStep {
    DECO_REMOVE_SLIDE_IN = 0,   /* the box's window slides in */
    DECO_REMOVE_BOX = 1,        /* the box: a piece with a decoration */
    DECO_REMOVE_PANEL = 2,      /* the piece's slots: a decoration */
    DECO_REMOVE_ASK = 3         /* the fee question */
};

/* DecoWork.panelStep: the removal panel (Deco_ChooseRemove). */
#define DECO_PANEL_SLIDE_IN     0
#define DECO_PANEL_CHOOSE       1

/* DecoWork.view on the make screen. */
enum DecoMakeView {
    DECO_MAKE_VIEW_LIST = 0,    /* the make list alone */
    DECO_MAKE_VIEW_INFO = 1,    /* the recipe's panel, with the decoration's info */
    DECO_MAKE_VIEW_DETAILS = 2  /* the same with its details, in DecoWork.viewPages pages */
};

/* DecoWork.view on the attach and remove screens' box (Deco_BoxInput). */
enum DecoBoxView {
    DECO_BOX_VIEW_GRID = 0,     /* the box's grid alone */
    DECO_BOX_VIEW_DETAIL = 1,   /* the piece's level and decorations (Equip_LevelMark) */
    DECO_BOX_VIEW_SKILLS = 2,   /* the skills with the piece worn (Equip_PreviewSkills) */
    DECO_BOX_VIEW_COUNT = 3
};

/* DecoWork.view on the attach list and the removal panel
 * (Deco_CountPanelViewPages). */
enum DecoPanelView {
    DECO_VIEW_NONE = 0,         /* the list alone */
    DECO_VIEW_DETAILS = 1,      /* the decoration's details, three pages */
    DECO_VIEW_COMPARE = 2,      /* the piece before and after the change */
    DECO_VIEW_SKILLS = 3,       /* the skills with the changed piece worn */
    DECO_VIEW_COUNT = 4
};
#define DECO_DETAIL_PAGES       3   /* DECO_VIEW_DETAILS' pages, and DECO_MAKE_VIEW_DETAILS' */

/* Deco_CheckAttach's result: why a decoration can't be attached, as bits. */
#define DECO_ATTACH_NO_FIT      0x1 /* the piece's kind doesn't take it (Deco_FitsEquip) */
#define DECO_ATTACH_NO_SLOTS    0x2 /* the piece has too few free slots */
#define DECO_ATTACH_NO_MONEY    0x4 /* the fee is more than the money held */

#define DECO_MAKE_LIMIT         100 /* a make's quantity stays under this (at most 99) */
#define DECO_NO_DATA_FEE        100 /* zenny: the attach fee of an item without decoration data */

/* The decorations counter's work (gMenuWork + FORGE_WORK_OFFSET): make a
 * decoration (choice DECO_CHOICE_MAKE), attach one to a box piece (ATTACH)
 * or remove one (REMOVE). */
typedef struct DecoWork {
    /* 0x00 */ EquipItem equip;         /* the box piece at the cursor, then the chosen one */
    /* 0x0C */ EquipItem changed;       /* the chosen piece as it will be after the change */
    /* 0x18 */ WinSlide win;
    /* 0x20 */ WinSlide win2;
    /* 0x28 */ s32 recipes[FORGE_LIST_ROWS]; /* make: a page of gDecoRecipes indexes */
    /* 0x44 */ u16 list[FORGE_LIST_ROWS];    /* attach: a page of owned decorations; remove: the piece's slots */
    /* 0x52 */ s16 choice;           /* the top menu's row (DECO_CHOICE_*) */
    /* 0x54 */ s16 row;                 /* the box's row */
    /* 0x56 */ s16 col;                 /* the box's column */
    /* 0x58 */ s16 page;                /* make: the list's page; attach and remove: the box's */
    /* 0x5A */ s16 pages;
    /* 0x5C */ s16 viewPage;            /* the detail view's page */
    /* 0x5E */ s16 viewPages;
    /* 0x60 */ s16 cursor;
    /* 0x62 */ s16 count;
    /* 0x64 */ s16 decoPage;            /* attach: the owned decorations' page */
    /* 0x66 */ s16 decoPages;
    /* 0x68 */ u8 unk68[4];
    /* 0x6C */ s32 quantity;            /* make: how many */
    /* 0x70 */ s8 mode;                 /* the screen (enum DecoMode); the wear question's steps while it runs (enum DecoWearStep) */
    /* 0x71 */ s8 step;                 /* the screen's step: enum DecoMakeStep, DecoAttachStep or DecoRemoveStep */
    /* 0x72 */ s8 panelStep;            /* remove: the slot panel's step (DECO_PANEL_*) */
    /* 0x73 */ s8 view;                 /* the detail view shown: enum DecoMakeView, DecoBoxView or DecoPanelView */
    /* 0x74 */ s8 wear;                 /* 1: the changed piece is worn after */
} DecoWork;
void Deco_Open(void);
s32 Deco_Input(u8 *task, u16 pad);
void Deco_LoadRecipes(DecoWork *counter, s16 page);
void Deco_OpenMake(DecoWork *counter);
s32 Deco_MakeInput(DecoWork *counter, u16 pad);
void Deco_Make(DecoWork *counter);
s32 Deco_QuantityInput(DecoWork *counter, u16 pad);
s32 Deco_RecipeListInput(DecoWork *counter, u16 pad);
void Deco_OpenRemove(DecoWork *counter);
s32 Deco_RemoveInput(DecoWork *counter, u16 pad);
void Deco_OpenRemovePanel(DecoWork *counter);
s32 Deco_ChooseRemove(DecoWork *counter, u16 pad);
void Deco_LoadOwned(DecoWork *counter, s16 page);
void Deco_OpenAttach(DecoWork *counter);
s32 Deco_AttachInput(DecoWork *counter, u16 pad);
s32 Deco_ChooseAttach(DecoWork *counter, u16 pad);
s32 Deco_BoxInput(DecoWork *counter, u16 pad);
void Deco_CountBoxViewPages(DecoWork *counter);
void Deco_CountPanelViewPages(DecoWork *counter);
s32 Deco_MenuInput(DecoWork *counter, u16 pad);
void Deco_Remove(DecoWork *counter);
s32 Deco_WearAskInput(DecoWork *counter, u16 pad);
s32 Deco_Confirm(u8 *task, u16 pad);
void Deco_DrawAttachList(DecoWork *counter, s16 x, s16 y);
void Deco_DrawRemovePanel(DecoWork *counter, s16 x, s16 y);
void Deco_DrawRemove(DecoWork *counter, s16 x, s16 y);
void Deco_DrawMakeList(DecoWork *counter, s16 x, s16 y);
void Deco_DrawMake(DecoWork *counter, s16 x, s16 y);
void Deco_Draw(u8 *task);
void Deco_Draw2(u8 *unused0);
void Deco_DrawEquipAsk(DecoWork *counter, s16 choice);
void Deco_DrawEquipPrompt(u8 *task);

s32 Recipe_IsInList(s32 type, s32 id);
s32 WeaponForge_IsUnlocked(s32 type, s32 id);
s32 ArmorUpgrade_IsLocked(EquipItem *piece);
s32 ArmorUpgrade_GetMaterials(EquipItem *piece, ItemStack *out, s8 *cond);
s32 ArmorUpgrade_GetMakeMaterials(EquipItem *piece, ItemStack *out, s8 *cond);
s32 ArmorUpgrade_GetDerived(EquipItem *piece, u16 *out);
void ArmorForge_Open(WorkshopMenu *menu);
s32 ArmorForge_Input(u8 *task, u16 pad);
void ArmorForge_LoadBox(ArmorForgeWork *forge);
void ArmorForge_AddUpgrade(ArmorForgeWork *forge, s32 slot);
s32 ArmorForge_UpgradeInput(ArmorForgeWork *forge, u16 pad);
s32 ArmorForge_UpgradeListInput(ArmorForgeWork *forge, u16 pad);
void ArmorForge_UpgradeDetail(ArmorForgeWork *forge);
s32 ArmorForge_CanMake(ArmorForgeWork *forge, s16 line);
void Equip_CopyEquipped(EquipItem *out, s16 type);
s32 ArmorForge_MakeInput(ArmorForgeWork *forge, u16 pad);
void ArmorForge_OpenMake(ArmorForgeWork *forge);
void ArmorForge_LoadMakePage(ArmorForgeWork *forge, s32 page);
s32 ArmorForge_UpgradeConfirm(ArmorForgeWork *forge, u16 pad);
s32 ArmorForge_Confirm(u8 *task, u16 pad);
void ArmorForge_Make(ArmorForgeWork *forge);
s32 ArmorForge_MakeConfirm(ArmorForgeWork *forge, u16 pad);
void ArmorForge_DrawMakeListFull(ArmorForgeWork *forge, s16 x, s16 y);
void ArmorForge_DrawMaterials(ArmorForgeWork *forge, s16 x, s16 y);
void ArmorForge_DrawMake(ArmorForgeWork *forge);
void ArmorForge_DrawUpgradeDetail(ArmorForgeWork *forge, s16 x, s16 y);
void ArmorForge_DrawUpgradeList(ArmorForgeWork *forge, EquipItem *piece, s16 x, s16 y);
void ArmorForge_DrawUpgradePanel(ArmorForgeWork *forge, s16 x, s16 y);
void ArmorForge_Draw(u8 *task);
void ArmorForge_DrawEquipPrompt(u8 *task);
void WeaponForge_Open(WorkshopMenu *menu);
s32 Forge_IsRandomWeapon(s16 id);
s32 WeaponForge_Input(u8 *task, u16 pad);
void WeaponForge_Make(WeaponForgeWork *forge);
s32 WeaponForge_MakeConfirm(WeaponForgeWork *forge, u16 pad);
s32 WeaponForge_UpgradeConfirm(WeaponForgeWork *forge, u16 pad);
s32 WeaponForge_Confirm(u8 *task, u16 pad);
void WeaponForge_Upgrade(WeaponForgeWork *forge);
s32 Equip_ItemHalfPrice(EquipItem *item);
void WeaponForge_SetBladeKind(WeaponForgeWork *forge);
s32 WeaponForge_CanMake(WeaponForgeWork *forge, s16 line);
s32 WeaponForge_MakeInput(WeaponForgeWork *forge, u16 pad);
void WeaponForge_RollRandom(EquipItem *out, u16 key);
void WeaponForge_OpenUpgrades(WeaponForgeWork *forge);
s32 WeaponForge_UpgradeInput(WeaponForgeWork *forge, u16 pad);
void WeaponForge_LoadGunMods(WeaponForgeWork *forge, s32 idx);
u16 WeaponForge_ApplyGunMod(u16 levelByte, u16 mod);
void WeaponForge_LoadUpgrades(WeaponForgeWork *forge);
s32 WeaponForge_BoxInput(WeaponForgeWork *forge, u16 pad);
s32 WeaponForge_UpgradeListInput(WeaponForgeWork *forge, u16 pad);
s32 WeaponForge_CanUpgrade(WeaponForgeWork *forge, s16 line);
void WeaponForge_UpgradeDetail(WeaponForgeWork *forge);
void WeaponForge_MakeDetail(WeaponForgeWork *forge);
void WeaponForge_DrawUpgradePanel(WeaponForgeWork *forge, s16 x, s16 y);
u32 Equip_HalfPrice(s32 type, s32 id);
void WeaponForge_DrawMake(WeaponForgeWork *forge);
void WeaponForge_DrawMakeMaterials(WeaponForgeWork *forge, s16 x, s16 y);
void WeaponForge_Draw(u8 *work);
void WeaponForge_DrawPreview(u8 *work);

#endif /* LOBBY_FORGE_H */
