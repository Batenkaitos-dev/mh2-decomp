/*
 * The weapon forge: one of the workshop's counters, where the hunter makes
 * a weapon from a recipe or upgrades one from the equipment box.
 *
 * - The top level: opening the forge, its input by step (the forge or
 *   upgrade choice, the upgrade pages, the forging pages) and its confirm
 *   phase, which asks whether to wear the result and then makes the weapon
 *   (taking the materials and the money, putting it in the box) or applies
 *   the upgrade or the gun mod.
 * - Forging: the recipes each of the five weapon classes lists
 *   (gWeaponRecipes), the list's pages, the forging page's input and detail,
 *   and the roll of a hidden-name recipe's weapon (gRandomEquips).
 * - Upgrading: the box's grid, a blade's or bow's upgrades read from its
 *   upgrade entry (what it becomes, the materials, half the new weapon's
 *   price), a bowgun's mods (priced with gGunModCostRates), the upgrade
 *   pages' input and detail.
 * - The draws: the upgrade pages, the forging page, the forge's main draw
 *   and the question whether to wear the new weapon.
 *
 * Everything works on the forge's WeaponForgeWork (lobby_forge.h), which
 * sits in the workshop's menu task after the task's header (gMenuWork +
 * 0xC). The workshop's update calls WeaponForge_Open, WeaponForge_Input and
 * WeaponForge_Confirm.
 *
 * Three helpers sit among the forge's functions: Forge_IsRandomWeapon
 * (whether a recipe is a random weapon's, which the equipment shop and the
 * name tags also ask), and Equip_ItemHalfPrice and Equip_HalfPrice (the
 * price of forging or upgrading into a piece, half its shop price; the
 * armour forge prices its pieces with Equip_HalfPrice too).
 */
#include "common.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "em.h"
#include "lobby_globals.h"
#include "scene.h"
#include "save.h"
#include "player.h"
#include "lobby_menu.h"
#include "lobby_forge.h"
#include "pad.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "sprite_queue.h"
#include "box_menu.h"
#include "camera.h"
#include "text.h"
#include "draw_helpers.h"
#include "equip_parts.h"
#include "equip_shop.h"
#include "font.h"
#include "item_shop.h"
#include "lobby_dialog_box.h"
#include "lobby_queries.h"
#include "lobby_util.h"
#include "menu_steps.h"
#include "menu_widgets.h"
#include "offer_menu.h"
#include "rand.h"
#include "town_frame.h"
#include "libc.h"
#include "window.h"

/* prototypes */
void Forge_DrawMaterials(s32, s32, ItemStack *need, s32); /* unit's own declaration: declared with s32 parameters as the forge passes them; with the definition's (s16 x, s16 y, ItemStack *need, s8 n) the call compiles differently */
s32 WeaponForge_IsBow(EquipItem *e);
/* end of prototypes */

extern u8 gLobbyTextDirty;

extern u8 gMenuInputLock;
u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
/* The forge's opening, its top input and its make and upgrade steps are
 * built at optimization level 4, the rest of the file at the default. */
#pragma optimization_level 4
/*
 * Sets up the weapon forge when the workshop opens it: the model preview
 * starts hidden, and the workshop header's list-only flag is copied into
 * the forge's filter (WorkshopMenu.shortList). With that flag set the forge skips its choice and
 * opens straight on the forging page, on the first weapon tab, listing
 * only the recipes of gForgeRecipeList.
 */
void WeaponForge_Open(WorkshopMenu *menu)
{
    WeaponForgeWork *forge = (WeaponForgeWork *)((u8 *)menu + FORGE_WORK_OFFSET);

    forge->previewOff = 1;
    forge->filter = menu->shortList;
    if (forge->filter != 0) {
        forge->previewOff = 1;
        forge->row = WEAPON_TAB_GREAT_LONG;
        forge->choice = FORGE_CHOICE_MAKE;
        forge->step = FORGE_STEP_MAKE;
        WeaponForge_SetBladeKind(forge);
        Chat_SetColor(0);
    }
}

/* Whether a weapon recipe's id is a random-weapon recipe's (WEAPON_ID_RANDOM_FIRST and up). */
s32 Forge_IsRandomWeapon(s16 id)
{
    return id >= WEAPON_ID_RANDOM_FIRST;
}

/*
 * The weapon forge's input, by step: the forge/upgrade choice, then the
 * upgrade pages or the forging pages. Returns MENU_DONE when a weapon is
 * to be made or upgraded (the workshop moves on to WeaponForge_Confirm),
 * MENU_CANCEL when the forge is left, else MENU_RUNNING.
 */
s32 WeaponForge_Input(u8 *task, u16 pad)
{
    WeaponForgeWork *forge = (WeaponForgeWork *)(task + FORGE_WORK_OFFSET);

    switch (forge->step) {
    case FORGE_STEP_CHOOSE:
        if (pad & PAD_CIRCLE) {
            forge->previewOff = 1;
            forge->row = 0;
            if (forge->choice == FORGE_CHOICE_MAKE) {
                forge->step = FORGE_STEP_MAKE;
                WeaponForge_SetBladeKind(forge);
            } else {
                EquipPreview_Reset();
                forge->step = FORGE_STEP_UPGRADE;
                WeaponForge_OpenUpgrades(forge);
                CamFocus_SetAside(1);
            }
            LobbySe_Play(LOBBY_SE_SELECT);
            Chat_SetColor(0);
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            forge->choice = Menu_WrapUpDownSe(forge->choice, 2, LOBBY_SE_CURSOR);
        }
        break;
    case FORGE_STEP_UPGRADE:
        switch (WeaponForge_UpgradeInput(forge, pad)) {
        case MENU_DONE:
            forge->step = FORGE_STEP_CHOOSE;
            return MENU_DONE;
        case MENU_CANCEL:
            forge->step = FORGE_STEP_CHOOSE;
            CamFocus_SetAside(0);
            Chat_SetColor(-1);
            break;
        }
        break;
    case FORGE_STEP_MAKE:
        switch (WeaponForge_MakeInput(forge, pad)) {
        case MENU_DONE:
            forge->step = FORGE_STEP_CHOOSE;
            return MENU_DONE;
        case MENU_CANCEL:
            if (forge->filter != 0) {
                Chat_SetColor(-1);
                return MENU_CANCEL;
            }
            forge->step = FORGE_STEP_CHOOSE;
            Chat_SetColor(-1);
            break;
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Makes the weapon of the recipe at the cursor: a random-weapon recipe
 * gives the weapon already rolled, for WEAPON_RANDOM_PRICE, another gives the
 * recipe's weapon for half its price. Takes the four materials and the
 * money and adds the weapon to the box; when the player chose to wear it,
 * equips it, rebuilds the hunter and shows the equipped notice, else the
 * made notice.
 */
void WeaponForge_Make(WeaponForgeWork *forge)
{
    ForgeRecipe *recipe = &gWeaponRecipes[forge->makeList[forge->cursor]];
    EquipItem made;
    s32 price;
    u16 *id;
    u8 *type;
    s32 i;
    s32 slot;

    memset(&made, 0, sizeof(made));
    if (Forge_IsRandomWeapon(recipe->id)) {
        made = forge->result;
        price = WEAPON_RANDOM_PRICE;
    } else {
        type = &made.type;
        id = &made.id;
        *type = recipe->type;
        *id = recipe->id;
        price = Equip_HalfPrice(*type, *id);
    }
    for (i = 0; i < FORGE_RECIPE_MATS; i++) {
        Items_Take(recipe->mat[i].item, recipe->mat[i].count);
    }
    slot = Box_AddItem(&gSave, made.type, made.id, 0);
    Save_AddMoney(-price);
    LobbySe_Play(LOBBY_SE_MONEY);
    if (forge->wear != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, slot);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
        Menu_PushNotice(LOBBY_NOTICE_EQUIPPED);
        LobbySe_Play(LOBBY_SE_EQUIP);
    } else {
        Player_SetColorWord(&gPlayers[gScene.localPlayer], gSave.symbolColor);
        Menu_PushNotice(LOBBY_NOTICE_SENT_TO_BOX);
    }
}

/*
 * The forging's confirm phase, using the step field as its own step: asks
 * whether to wear the new weapon (with a warning when its type differs
 * from the worn weapon's), with L1/R1 turning the comparison's pages, then
 * makes it. Returns MENU_DONE once made (the forging page comes back, its
 * window sliding in), else MENU_RUNNING.
 */
s32 WeaponForge_MakeConfirm(WeaponForgeWork *forge, u16 pad)
{
    ForgeRecipe *recipe = &gWeaponRecipes[forge->makeList[forge->cursor]];
    EquipItem made;
    u8 *type;

    memset(&made, 0, sizeof(made));
    type = &made.type;
    *type = recipe->type;
    made.id = recipe->id;
    switch (forge->step) {
    case WEAPON_CONFIRM_SETUP:
        forge->detailPages = Equip_LevelMark(&gSave.equip[0], 16, &made);
        Menu_SetYesNo(0);
        gMenuInputLock = 9;
        if (*type != gSave.equip[0].type) {
            forge->step = WEAPON_CONFIRM_ASK_CHANGE;
        } else {
            forge->step = WEAPON_CONFIRM_ASK;
        }
        break;
    case WEAPON_CONFIRM_ASK:
    case WEAPON_CONFIRM_ASK_CHANGE:
        if (pad & PAD_R1) {
            if (forge->detailPages > 1) {
                if (++forge->detailPage >= forge->detailPages) {
                    forge->detailPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (forge->detailPages > 1) {
                if (--forge->detailPage < 0) {
                    forge->detailPage = forge->detailPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                forge->step = WEAPON_CONFIRM_APPLY;
                forge->wear = 1;
                break;
            case MENU_CANCEL:
                forge->step = WEAPON_CONFIRM_APPLY;
                forge->wear = 0;
                break;
            }
        }
        break;
    case WEAPON_CONFIRM_APPLY:
        forge->step = FORGE_STEP_MAKE;
        forge->subStep = FORGE_MAKE_SLIDE_IN;
        forge->view = FORGE_MAKE_VIEW_LIST;
        WeaponForge_Make(forge);
        WinSlide_Step(&forge->detailWin, 640, 280);
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/*
 * The upgrade's confirm phase, as WeaponForge_MakeConfirm: when the box
 * piece is worn it skips the question and wears the result, else asks
 * whether to wear it. Then applies the upgrade, reloads the piece's
 * upgrades and returns MENU_DONE; MENU_RUNNING before.
 */
s32 WeaponForge_UpgradeConfirm(WeaponForgeWork *forge, u16 pad)
{
    s32 idx = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    EquipItem upgraded;

    memset(&upgraded, 0, sizeof(upgraded));
    switch (forge->step) {
    case WEAPON_CONFIRM_SETUP:
        if (forge->upgradeKind == EQUIP_BLADE) {
            upgraded = forge->upgrades[forge->cursor];
            if (upgraded.id != 0) {
                upgraded.used = 1;
            } else {
                upgraded.used = 0;
            }
        } else {
            upgraded = forge->result;
        }
        forge->detailPages = Equip_LevelMark(&gSave.equip[0], 16, &upgraded);
        if (Equip_IsSlotEquipped(&gSave, idx) == 1) {
            forge->wear = 1;
            forge->step = WEAPON_CONFIRM_APPLY;
        } else {
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            if (gSave.box[idx].type != gSave.equip[0].type) {
                forge->step = WEAPON_CONFIRM_ASK_CHANGE;
            } else {
                forge->step = WEAPON_CONFIRM_ASK;
            }
        }
        break;
    case WEAPON_CONFIRM_ASK:
    case WEAPON_CONFIRM_ASK_CHANGE:
        if (pad & PAD_R1) {
            if (forge->detailPages > 1) {
                if (++forge->detailPage >= forge->detailPages) {
                    forge->detailPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (forge->detailPages > 1) {
                if (--forge->detailPage < 0) {
                    forge->detailPage = forge->detailPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                forge->step = WEAPON_CONFIRM_APPLY;
                forge->wear = 1;
                break;
            case MENU_CANCEL:
                forge->step = WEAPON_CONFIRM_APPLY;
                forge->wear = 0;
                break;
            }
        }
        break;
    case WEAPON_CONFIRM_APPLY:
        forge->step = FORGE_STEP_UPGRADE;
        forge->subStep = FORGE_UPGRADE_SLIDE_IN;
        forge->view = FORGE_BOX_VIEW_GRID;
        WeaponForge_Upgrade(forge);
        forge->cursor = 0;
        WeaponForge_LoadUpgrades(forge);
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/*
 * The weapon forge's confirm phase: forging's (FORGE_CHOICE_MAKE) or the upgrade's.
 * Returns what that one returns.
 */
s32 WeaponForge_Confirm(u8 *task, u16 pad)
{
    WeaponForgeWork *forge = (WeaponForgeWork *)(task + FORGE_WORK_OFFSET);

    if (forge->choice == FORGE_CHOICE_MAKE) {
        return WeaponForge_MakeConfirm(forge, pad);
    }
    return WeaponForge_UpgradeConfirm(forge, pad);
}

/*
 * Applies the upgrade at the cursor to the box piece under the box cursor:
 * a blade or a bow becomes the chosen weapon at level 0, for its three
 * materials and its price; a bowgun gets the chosen mod (a level, or a
 * silencer, barrel, scope or shield fitted or removed) for the mod's
 * price. Then wears it or shows the notice as WeaponForge_Make, and
 * reloads a bowgun's mods.
 */
void WeaponForge_Upgrade(WeaponForgeWork *forge)
{
    s32 idx = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    EquipItem piece;
    s32 cost;
    s32 i;

    piece = gSave.box[idx];
    switch (forge->upgradeKind) {
    case EQUIP_BLADE:
        piece = forge->upgrades[forge->cursor];
        cost = forge->upgradeCost[forge->cursor];
        for (i = 0; i < WEAPON_UP_MATS; i++) {
            Items_Take(forge->mats[forge->cursor].need[i].item, forge->mats[forge->cursor].need[i].count);
        }
        piece.level = 0;
        Box_SetSlot(&gSave, idx, &piece);
        Save_AddMoney(-cost);
        LobbySe_Play(LOBBY_SE_MONEY);
        break;
    case EQUIP_GUN:
        switch (forge->mod[forge->cursor]) {
        case GUN_MOD_LEVEL_UP:
            Box_GunLevelUp(&gSave, idx);
            break;
        case GUN_MOD_FIT_SILENCER:
            Box_SetGunSilencer(&gSave, idx, BOX_GUN_FIT);
            break;
        case GUN_MOD_REMOVE_SILENCER:
            Box_SetGunSilencer(&gSave, idx, BOX_GUN_REMOVE);
            break;
        case GUN_MOD_FIT_BARREL:
        case GUN_MOD_FIT_HEAVY_BARREL:
            Box_SetGunBarrel(&gSave, idx, BOX_GUN_FIT);
            break;
        case GUN_MOD_REMOVE_BARREL:
        case GUN_MOD_REMOVE_HEAVY_BARREL:
            Box_SetGunBarrel(&gSave, idx, BOX_GUN_REMOVE);
            break;
        case GUN_MOD_FIT_SCOPE:
            Box_SetGunScope(&gSave, idx, BOX_GUN_FIT);
            break;
        case GUN_MOD_REMOVE_SCOPE:
            Box_SetGunScope(&gSave, idx, BOX_GUN_REMOVE);
            break;
        case GUN_MOD_FIT_SHIELD:
            Box_SetGunShield(&gSave, idx, BOX_GUN_FIT);
            break;
        case GUN_MOD_REMOVE_SHIELD:
            Box_SetGunShield(&gSave, idx, BOX_GUN_REMOVE);
            break;
        }
        Save_AddMoney(-forge->modCost[forge->cursor]);
        LobbySe_Play(LOBBY_SE_MONEY);
        break;
    }
    if (forge->wear != 0) {
        Equip_Refresh(&gSave);
        Player_LoadFromSave(&gPlayers[gScene.localPlayer]);
        Equip_EquipSlot(&gSave, idx);
        Player_SetupFromSave(&gPlayers[gScene.localPlayer], (struct SaveData *)&gSave);
        Player_CalcSkills(&gPlayers[gScene.localPlayer]);
        Town_ReloadLocalPlayer();
        gLobbyTextDirty = 1;
        Menu_PushNotice(LOBBY_NOTICE_EQUIPPED);
        LobbySe_Play(LOBBY_SE_EQUIP);
    } else {
        Player_SetColorWord(&gPlayers[gScene.localPlayer], gSave.symbolColor);
        Menu_PushNotice(LOBBY_NOTICE_SENT_TO_BOX);
    }
    if (forge->upgradeKind == EQUIP_GUN) {
        WeaponForge_LoadGunMods(forge, idx);
    }
}

/*
 * Half the price of a piece of equipment (its type and id): what forging
 * or upgrading into it costs.
 */
s32 Equip_ItemHalfPrice(EquipItem *item)
{
    return (s32)Equip_GetPrice(item->type, item->id) / 2;
}

s32 WeaponForge_IsBow(EquipItem *weapon)
{
    return (Weapon_GetClass(weapon->type, weapon->id) & 0xFF) == WEAPON_BOW;
}

void WeaponForge_SetBladeKind(WeaponForgeWork *forge)
{
    forge->upgradeKind = EQUIP_BLADE;
}
#pragma optimization_level reset

/*
 * Whether recipe idx (r) is listed on forge tab `tab`: its weapon's class
 * must be the tab's (great and long swords, sword-and-shields and dual
 * swords, hammers and horns, lances and gunlances, then the bowguns and
 * bows on the last tab) and the recipe unlocked (Forge_CheckRecipe).
 * Returns 1 or 0; the new-mark flags it sets go to a local copy and are
 * lost.
 */
s32 WeaponForge_IsListed(s16 tab, ForgeRecipe *recipe, s32 recipeIdx)
{
    EquipItem made;

    memset(&made, 0, sizeof(made));
    if (recipe->type == EQUIP_BLADE) {
        if (tab == WEAPON_TAB_GUNS) {
            return 0;
        }
    } else {
        if (tab != WEAPON_TAB_GUNS) {
            return 0;
        }
    }
    switch (Weapon_GetClass(recipe->type, recipe->id)) {
    case WEAPON_GREAT_SWORD:
    case WEAPON_LONG_SWORD:
        if (tab != WEAPON_TAB_GREAT_LONG) {
            return 0;
        }
        break;
    case WEAPON_SWORD_SHIELD:
    case WEAPON_DUAL_SWORDS:
        if (tab != WEAPON_TAB_SWORDS) {
            return 0;
        }
        break;
    case WEAPON_HAMMER:
    case WEAPON_HUNTING_HORN:
        if (tab != WEAPON_TAB_HAMMER_HORN) {
            return 0;
        }
        break;
    case WEAPON_LANCE:
    case WEAPON_GUNLANCE:
        if (tab != WEAPON_TAB_LANCES) {
            return 0;
        }
        break;
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
    default:
        if (tab != WEAPON_TAB_GUNS) {
            return 0;
        }
        break;
    }
    made.type = recipe->type;
    made.id = recipe->id;
    if (Forge_CheckRecipe(FORGE_RECIPES_WEAPON, recipeIdx, FORGE_COUNT_ALL) == FORGE_RECIPE_LOCKED) {
        return 0;
    }
    if (Equip_TestFlag(&made, EQUIP_FLAG_MADE) == 0) {
        Equip_SetFlag(&made, EQUIP_FLAG_NEW);
        Equip_SetFlag(&made, EQUIP_FLAG_MADE);
    }
    return 1;
}

/*
 * Whether the recipe on line `line` of the forge list can be made now: every
 * material at hand, the money for its price (WEAPON_RANDOM_PRICE for a random-weapon
 * recipe, else half the weapon's price) and a free box slot. Returns 1 or
 * 0.
 */
s32 WeaponForge_CanMake(WeaponForgeWork *forge, s16 line)
{
    s32 idx = forge->makeList[line];
    ForgeRecipe *recipe = &gWeaponRecipes[idx];
    s32 price;

    if (Forge_CheckRecipe(FORGE_RECIPES_WEAPON, idx, FORGE_COUNT_ALL) != FORGE_RECIPE_READY) {
        return 0;
    }
    if (Forge_IsRandomWeapon(recipe->id) != 0) {
        price = WEAPON_RANDOM_PRICE;
    } else {
        price = Equip_HalfPrice(recipe->type, recipe->id);
    }
    if (gSave.money < price) {
        return 0;
    }
    return (u16)Box_FindFreeSlot(&gSave) != BOX_NONE;
}

/*
 * Fills the forge list with page `page` of the recipes listed on the
 * current tab that pass the filter (seven a page), sets the line count and
 * the page count, and keeps the cursor on a line.
 */
void WeaponForge_LoadMakePage(WeaponForgeWork *forge, s32 page)
{
    s32 lastShown;
    ForgeRecipe *recipe;
    s32 shown;
    s32 listed;
    s32 firstShown;
    s32 recipeIdx;
    u8 type;

    recipe = gWeaponRecipes;
    firstShown = page * FORGE_LIST_ROWS;
    shown = 0;
    type = gWeaponRecipes[0].type;
    lastShown = firstShown + FORGE_LIST_ROWS - 1;
    listed = 0;
    recipeIdx = 0;
    while (type != FORGE_RECIPE_END) {
        if (WeaponForge_IsListed(forge->row, recipe, recipeIdx) == 1) {
            if ((forge->filter == 0 && Recipe_IsInList(recipe->type, recipe->id) == 0)
                || (forge->filter == 1 && Recipe_IsInList(recipe->type, recipe->id) == 1)) {
                if (listed >= firstShown && listed <= lastShown) {
                    forge->makeList[shown] = recipeIdx;
                    shown++;
                }
                listed++;
            }
        }
        recipe++;
        recipeIdx++;
        type = recipe->type;
    }
    forge->count = shown;
    if (forge->cursor >= forge->count) {
        forge->cursor = forge->count - 1;
    }
    forge->pages = Menu_CountPages(listed, FORGE_LIST_ROWS);
}

/* unit's own declaration: declared with the u8 types the calls here pass; with the definition's u16 parameters the calls compile differently */
s32 Equip_GetModelId(u8 type, u16 id, u8 x);
void EquipPreview_Load(u8 type, s32 n, u8 x); /* unit's own declaration: declared with the types the calls here pass; with the definition's (u16 kind, u16 id, u8 female) the calls compile differently */
/*
 * The forging pages' input, by sub-step: the weapon tab (circle opens its
 * list, sliding the window in, with a notice when the box is full), the
 * slide, the recipe list (circle on a makeable recipe asks the price
 * question, square cycles the detail views, L1/R1 turn the detail pages,
 * left/right the list's pages, up/down the cursor, Select shows or hides
 * the model), then the price question. Returns MENU_DONE on yes (a random-
 * weapon recipe's weapon is rolled then), MENU_CANCEL when the tab choice
 * is left, else MENU_RUNNING.
 */
s32 WeaponForge_MakeInput(WeaponForgeWork *forge, u16 pad)
{
    s32 cur = forge->cursor;
    EquipItem preview;
    u16 *id;
    u8 *type;
    u8 x;

    memset(&preview, 0, sizeof(preview));
    switch (forge->subStep) {
    case FORGE_MAKE_TAB:
        if (pad & PAD_CIRCLE) {
            forge->subStep++;
            forge->page = 0;
            forge->detailPage = 0;
            forge->cursor = 0;
            forge->view = FORGE_MAKE_VIEW_LIST;
            WinSlide_Step(&forge->detailWin, 640, 280);
            WeaponForge_LoadMakePage(forge, 0);
            WeaponForge_MakeDetail(forge);
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            CamFocus_SetAside(1);
            EquipPreview_Reset();
            if ((u16)Box_FindFreeSlot(&gSave) == BOX_NONE) {
                Menu_PushNotice(LOBBY_NOTICE_BOX_FULL);
            }
        } else if (pad & PAD_CROSS) {
            forge->subStep = FORGE_MAKE_TAB;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            forge->row = Menu_WrapUpDownSe(forge->row, WEAPON_TAB_COUNT, LOBBY_SE_CURSOR);
        }
        break;
    case FORGE_MAKE_SLIDE_IN:
        switch (WinSlide_Step(&forge->detailWin, 640, 280)) {
        case MENU_DONE:
            forge->subStep++;
            break;
        }
        break;
    case FORGE_MAKE_LIST:
        if (pad & PAD_CIRCLE) {
            if (forge->count != 0 && WeaponForge_CanMake(forge, forge->cursor) == 1) {
                forge->subStep++;
                forge->detailPage = 0;
                Menu_SetYesNo(0);
                gMenuInputLock = 9;
                EquipPreview_Hide(1);
                LobbySe_Play(LOBBY_SE_SELECT);
                forge->view = FORGE_MAKE_VIEW_COMPARE;
                WeaponForge_MakeDetail(forge);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            if (forge->view != FORGE_MAKE_VIEW_LIST) {
                forge->view = FORGE_MAKE_VIEW_LIST;
                WeaponForge_MakeDetail(forge);
            } else {
                forge->subStep = FORGE_MAKE_TAB;
                EquipPreview_Hide(1);
                CamFocus_SetAside(0);
            }
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        } else if (pad & PAD_SQUARE) {
            if (forge->count != 0) {
                if (++forge->view == FORGE_MAKE_VIEW_COUNT) {
                    forge->view = FORGE_MAKE_VIEW_STATS;
                }
                forge->detailPage = 0;
                WeaponForge_MakeDetail(forge);
                LobbySe_Play(LOBBY_SE_DESCRIPTION);
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (forge->pages > 1) {
                forge->page = Menu_WrapLeftRight(forge->page, forge->pages);
                WeaponForge_LoadMakePage(forge, forge->page);
                WeaponForge_MakeDetail(forge);
            }
        } else if (pad & (PAD_UP | PAD_DOWN)) {
            forge->cursor = Menu_WrapUpDownSe(forge->cursor, forge->count, LOBBY_SE_CURSOR);
            WeaponForge_MakeDetail(forge);
        } else if (pad & PAD_R1) {
            if (forge->view != FORGE_MAKE_VIEW_LIST && forge->detailPages > 1) {
                if (++forge->detailPage >= forge->detailPages) {
                    forge->detailPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (forge->view != FORGE_MAKE_VIEW_LIST && forge->detailPages > 1) {
                if (--forge->detailPage < 0) {
                    forge->detailPage = forge->detailPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_SELECT) {
            if (forge->upgradeKind != EQUIP_GUN && (forge->view == FORGE_MAKE_VIEW_STATS || forge->view == FORGE_MAKE_VIEW_MATERIALS)) {
                if (forge->previewOff == 0) {
                    forge->previewOff = 1;
                    EquipPreview_Hide(1);
                } else {
                    forge->previewOff = 0;
                    memset(&preview, 0, sizeof(preview));
                    preview.used = 1;
                    type = &preview.type;
                    id = &preview.id;
                    /* (u32): the index is taken as unsigned here, which makes the compiler build the address as the game's code does */
                    *type = gWeaponRecipes[forge->makeList[(u32)cur]].type;
                    *id = gWeaponRecipes[forge->makeList[cur]].id;
                    if (*type != forge->shown.type || *id != forge->shown.id) {
                        forge->shown = preview;
                        if (Forge_IsRandomWeapon(*id) == 0) {
                            EquipPreview_Free();
                            x = gSave.gender;
                            EquipPreview_Load(*type, Equip_GetModelId(*type, *id, x), x);
                        }
                    }
                    EquipPreview_Hide(0);
                }
            }
        }
        break;
    case FORGE_MAKE_ASK:
        if (pad & PAD_R1) {
            if (forge->detailPages > 1) {
                if (++forge->detailPage >= forge->detailPages) {
                    forge->detailPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (forge->detailPages > 1) {
                if (--forge->detailPage < 0) {
                    forge->detailPage = forge->detailPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (LobbyMenu_YesNoInput(LOBBY_SE_SELECT)) {
            case MENU_DONE:
                forge->subStep = FORGE_MAKE_TAB;
                forge->detailPage = 0;
                WinSlide_Step(&forge->detailWin, 640, 280);
                if (Forge_IsRandomWeapon(gWeaponRecipes[forge->makeList[cur]].id)) {
                    WeaponForge_RollRandom(&forge->result, gWeaponRecipes[forge->makeList[cur]].mat[0].item);
                }
                return MENU_DONE;
            case MENU_CANCEL:
                forge->subStep = FORGE_MAKE_LIST;
                forge->previewOff = 1;
                EquipPreview_Hide(1);
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

extern RandEquip gRandomEquips[];
/*
 * Rolls the weapon a random-weapon recipe gives: finds key's entry in
 * gRandomEquips (the key is the recipe's first material) and writes into
 * out one of its three weapons, with chances of 5, 20 and 75 percent from
 * the saved random number; then steps the saved random stream.
 */
void WeaponForge_RollRandom(EquipItem *out, u16 key)
{
    RandEquip *entry = gRandomEquips;
    u16 roll = gSave.randSaved[5] % 100;
    u16 entryKey = gRandomEquips[0].key;

    while (entryKey != 0) {
        if (entryKey == key) {
            memset(out, 0, sizeof(EquipItem));
            out->used = 1;
            out->type = entry->type;
            if (roll < 5) {
                out->id = entry->id[0];
            } else if (roll < 25) {
                out->id = entry->id[1];
            } else {
                out->id = entry->id[2];
            }
        }
        entry++;
        entryKey = entry->key;
    }
    Rand_StepSaved();
}

extern WeaponUp gBladeUpgrades[];
extern WeaponUp gGunUpgrades[];
/*
 * Opens the upgrade pages: the box cursor on the first slot, both windows
 * sliding in. It walks the whole box testing each blade's and bow's
 * upgrades (the results are not kept), then loads the upgrades, or a
 * bowgun's mods, of the piece under the cursor.
 */
void WeaponForge_OpenUpgrades(WeaponForgeWork *forge)
{
    s32 i;
    s32 j;

    forge->cursor = 0;
    forge->page = 0;
    forge->pages = Box_GetPageCount(&gSave);
    forge->col = 0;
    forge->row = 0;
    forge->view = FORGE_BOX_VIEW_GRID;
    WinSlide_Step(&forge->detailWin, 640, 300);
    WinSlide_Step(&forge->panelWin, -296, 10);
    for (i = 0; i < BOX_SLOT_COUNT; i++) {
        if ((&gSave.box[i])->used != 0) {
            switch ((&gSave.box[i])->type) {
            case EQUIP_BLADE:
                forge->upgradeKind = EQUIP_BLADE;
                forge->upgradeEntry = &gBladeUpgrades[(&gSave.box[i])->id];
                break;
            case EQUIP_GUN:
                if (WeaponForge_IsBow(&gSave.box[i]) != 1) {
                    continue;
                }
                forge->upgradeKind = EQUIP_BLADE;
                forge->upgradeEntry = &gGunUpgrades[(&gSave.box[i])->id];
                break;
            default:
                continue;
            }
            for (j = 0; j < FORGE_UPGRADE_LINES; j++) {
                if (forge->upgradeEntry->next[j] != 0) {
                    WeaponForge_CanUpgrade(forge, j);
                }
            }
        }
    }
    /* the loop counter reused for the box index (as the registers show) */
    i = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    switch (gSave.box[i].type) {
    case EQUIP_BLADE:
        forge->upgradeKind = EQUIP_BLADE;
        break;
    case EQUIP_GUN:
        if (WeaponForge_IsBow(&gSave.box[i]) == 1) {
            forge->upgradeKind = EQUIP_BLADE;
        } else {
            forge->upgradeKind = EQUIP_GUN;
            WeaponForge_LoadGunMods(forge, i);
        }
        break;
    }
    if (forge->upgradeKind == EQUIP_BLADE) {
        if (WeaponForge_IsBow(&gSave.box[i]) == 1) {
            forge->upgradeEntry = &gGunUpgrades[gSave.box[i].id];
        } else {
            forge->upgradeEntry = &gBladeUpgrades[gSave.box[i].id];
        }
    }
    WeaponForge_LoadUpgrades(forge);
}

/*
 * The upgrade pages' input, by sub-step: the windows slide in, the box
 * grid, the upgrade or mod list, then the price question (L1/R1 turn the
 * comparison's pages). Returns MENU_DONE on yes, MENU_CANCEL when the box
 * grid is left, else MENU_RUNNING.
 */
s32 WeaponForge_UpgradeInput(WeaponForgeWork *forge, u16 pad)
{
    EquipItem unused;

    memset(&unused, 0, sizeof(unused));
    switch (forge->subStep) {
    case FORGE_UPGRADE_SLIDE_IN:
        WinSlide_Step(&forge->panelWin, -296, 10);
        switch (WinSlide_Step(&forge->detailWin, 640, 300)) {
        case MENU_DONE:
            forge->subStep++;
            break;
        }
        break;
    case FORGE_UPGRADE_BOX:
        switch (WeaponForge_BoxInput(forge, pad)) {
        case MENU_DONE:
            forge->subStep++;
            forge->detailPage = 0;
            forge->cursor = 0;
            forge->view = FORGE_UPGRADE_VIEW_STATS;
            WeaponForge_UpgradeDetail(forge);
            break;
        case MENU_CANCEL:
            forge->subStep = FORGE_UPGRADE_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case FORGE_UPGRADE_LIST:
        switch (WeaponForge_UpgradeListInput(forge, pad)) {
        case MENU_DONE:
            forge->subStep++;
            forge->detailPage = 0;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            EquipPreview_Hide(1);
            forge->view = FORGE_UPGRADE_VIEW_COMPARE;
            WeaponForge_UpgradeDetail(forge);
            break;
        case MENU_CANCEL:
            forge->subStep = FORGE_UPGRADE_BOX;
            forge->view = FORGE_BOX_VIEW_GRID;
            forge->detailPage = 0;
            forge->cursor = 0;
            EquipPreview_Hide(1);
            break;
        }
        break;
    case FORGE_UPGRADE_ASK:
        if (pad & PAD_R1) {
            if (forge->detailPages > 1) {
                if (++forge->detailPage >= forge->detailPages) {
                    forge->detailPage = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else if (pad & PAD_L1) {
            if (forge->detailPages > 1) {
                if (--forge->detailPage < 0) {
                    forge->detailPage = forge->detailPages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        } else {
            switch (LobbyMenu_YesNoInput(LOBBY_SE_SELECT)) {
            case MENU_DONE:
                forge->subStep = FORGE_UPGRADE_SLIDE_IN;
                forge->detailPage = 0;
                WinSlide_Step(&forge->panelWin, -296, 10);
                WinSlide_Step(&forge->detailWin, 640, 300);
                return MENU_DONE;
            case MENU_CANCEL:
                forge->subStep--;
                forge->previewOff = 1;
                EquipPreview_Hide(1);
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

extern s32 gGunModCostRates[];





/*
 * Builds the mod list of the bowgun in box slot idx: a level up while
 * below level 4 (priced with gGunModCostRates), then a silencer (light
 * bowguns) or a shield (heavy), a barrel and a scope, each to fit (priced
 * from the gun's price) or to remove (10 zenny) by what is fitted. It also
 * keeps the gun's level byte and sets the result to the gun with the mod
 * at the cursor applied (the mod read before the list is rebuilt).
 */
void WeaponForge_LoadGunMods(WeaponForgeWork *forge, s32 idx)
{
    s32 price;
    s16 n;
    u8 cls;
    s32 modLevel;
    s32 i;

    forge->result = gSave.box[idx];
    forge->result.used = 1;
    forge->gunLevel = forge->result.level;
    forge->result.level = WeaponForge_ApplyGunMod(forge->gunLevel, forge->mod[forge->cursor]);
    if (Box_GetGunLevel(&gSave, idx) >= GUN_LEVEL_MAX) {
        forge->count = GUN_MOD_LINES - 1;
    } else {
        forge->count = GUN_MOD_LINES;
    }
    cls = Weapon_GetClass(forge->result.type, forge->result.id);
    n = 0;
    price = Equip_GetPrice(EQUIP_GUN, gSave.box[idx].id);
    forge->mod[0] = GUN_MOD_NONE;
    forge->mod[1] = GUN_MOD_NONE;
    forge->mod[2] = GUN_MOD_NONE;
    forge->mod[3] = GUN_MOD_NONE;
    modLevel = Box_GetGunLevel(&gSave, idx);
    if (modLevel < GUN_LEVEL_MAX) {
        forge->modCost[n] = price / 10 * gGunModCostRates[modLevel];
        forge->mod[n] = GUN_MOD_LEVEL_UP;
        n++;
    }
    if (cls == WEAPON_HEAVY_BOWGUN) {
        if (Box_TestGunOption(&gSave, idx, GUN_OPT_SHIELD) == 1) {
            i = n;
            forge->modCost[i] = GUN_MOD_REMOVE_FEE;
            forge->mod[i] = GUN_MOD_REMOVE_SHIELD;
        } else {
            i = n;
            forge->modCost[i] = price / 10 * 3;
            forge->mod[i] = GUN_MOD_FIT_SHIELD;
        }
    } else {
        if (Box_TestGunOption(&gSave, idx, GUN_OPT_SILENCER) == 1) {
            i = n;
            forge->modCost[i] = GUN_MOD_REMOVE_FEE;
            forge->mod[i] = GUN_MOD_REMOVE_SILENCER;
        } else {
            i = n;
            forge->modCost[i] = price / 10 * 2;
            forge->mod[i] = GUN_MOD_FIT_SILENCER;
        }
    }
    if (Box_TestGunOption(&gSave, idx, GUN_OPT_BARREL) == 1) {
        forge->modCost[(s16)(i + 1)] = GUN_MOD_REMOVE_FEE;
        if (cls == WEAPON_HEAVY_BOWGUN) {
            forge->mod[(s16)(i + 1)] = GUN_MOD_REMOVE_HEAVY_BARREL;
        } else {
            forge->mod[(s16)(i + 1)] = GUN_MOD_REMOVE_BARREL;
        }
    } else {
        forge->modCost[(s16)(i + 1)] = price / 10 * 3;
        if (cls == WEAPON_HEAVY_BOWGUN) {
            forge->mod[(s16)(i + 1)] = GUN_MOD_FIT_HEAVY_BARREL;
        } else {
            forge->mod[(s16)(i + 1)] = GUN_MOD_FIT_BARREL;
        }
    }
    if (Box_TestGunOption(&gSave, idx, GUN_OPT_SCOPE) == 1) {
        forge->modCost[(s16)(i + 2)] = GUN_MOD_REMOVE_FEE;
        forge->mod[(s16)(i + 2)] = GUN_MOD_REMOVE_SCOPE;
    } else {
        forge->modCost[(s16)(i + 2)] = price / 10 * 3;
        forge->mod[(s16)(i + 2)] = GUN_MOD_FIT_SCOPE;
    }
}

/*
 * Returns a bowgun's level byte (flags) after mod `ev`: a level up adds
 * one level, at most 4; fitting the barrel removes the silencer and the
 * shield, fitting either of them removes the barrel; a removal clears the
 * option's bit.
 */
u16 WeaponForge_ApplyGunMod(u16 levelByte, u16 mod)
{
    u16 gunLevel;

    switch (mod) {
    case GUN_MOD_LEVEL_UP:
        gunLevel = levelByte & GUN_LEVEL_MASK;
        if (gunLevel >= GUN_LEVEL_MAX) {
            gunLevel = GUN_LEVEL_MAX;
        } else {
            gunLevel++;
        }
        levelByte = (levelByte & GUN_OPT_MASK) | gunLevel;
        break;
    case GUN_MOD_FIT_SILENCER:
        levelByte |= GUN_OPT_SILENCER;
        levelByte &= ~GUN_OPT_BARREL;
        break;
    case GUN_MOD_REMOVE_SILENCER:
        levelByte &= ~GUN_OPT_SILENCER;
        break;
    case GUN_MOD_FIT_BARREL:
    case GUN_MOD_FIT_HEAVY_BARREL:
        levelByte |= GUN_OPT_BARREL;
        levelByte &= ~(GUN_OPT_SILENCER | GUN_OPT_SHIELD);
        break;
    case GUN_MOD_REMOVE_BARREL:
    case GUN_MOD_REMOVE_HEAVY_BARREL:
        levelByte &= ~GUN_OPT_BARREL;
        break;
    case GUN_MOD_FIT_SCOPE:
        levelByte |= GUN_OPT_SCOPE;
        break;
    case GUN_MOD_REMOVE_SCOPE:
        levelByte &= ~GUN_OPT_SCOPE;
        break;
    case GUN_MOD_FIT_SHIELD:
        levelByte |= GUN_OPT_SHIELD;
        levelByte &= ~GUN_OPT_BARREL;
        break;
    case GUN_MOD_REMOVE_SHIELD:
        levelByte &= ~GUN_OPT_SHIELD;
        break;
    }
    return levelByte;
}

const char gForgePriceFmt[5] = {
#include "lobby/0082EA20.inc"
};
const char gForgeTextFmt[3] = {
#include "lobby/0082EA28.inc"
};

/*
 * Loads what the box piece under the cursor can become: for a blade or a
 * bow, each next weapon of its upgrade entry that may be forged, with its
 * price and materials (for a blade the result is set to the upgrade at the
 * cursor); for a bowgun, its mods. Another piece has no list and its type
 * is kept as the kind.
 */
void WeaponForge_LoadUpgrades(WeaponForgeWork *forge)
{
    s32 idx;
    s32 i;
    u8 type;

    idx = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    type = gSave.box[idx].type;

    switch (type) {
    case EQUIP_BLADE:
        forge->upgradeKind = EQUIP_BLADE;
        forge->upgradeEntry = &gBladeUpgrades[gSave.box[idx].id];
        forge->count = 0;
        for (i = 0; i < FORGE_UPGRADE_LINES; i++) {
            if (forge->upgradeEntry->next[i] != 0 && WeaponForge_IsUnlocked(type, forge->upgradeEntry->next[i]) == 1) {
                memset(&forge->upgrades[forge->count], 0, sizeof(EquipItem));
                forge->upgrades[forge->count].type = type;
                forge->upgrades[forge->count].id = forge->upgradeEntry->next[i];
                forge->upgradeCost[forge->count] = Equip_ItemHalfPrice(&forge->upgrades[forge->count]);
                memcpy(&forge->mats[forge->count], &gBladeUpgrades[forge->upgradeEntry->next[i]].mats, sizeof(WeaponMats));
                forge->count++;
            }
        }
        forge->result = forge->upgrades[forge->cursor];
        break;
    case EQUIP_GUN:
        if (WeaponForge_IsBow(&gSave.box[idx]) == 1) {
            forge->upgradeKind = EQUIP_BLADE;
            forge->upgradeEntry = &gGunUpgrades[gSave.box[idx].id];
            forge->count = 0;
            for (i = 0; i < FORGE_UPGRADE_LINES; i++) {
                if (forge->upgradeEntry->next[i] != 0 && WeaponForge_IsUnlocked(type, forge->upgradeEntry->next[i]) == 1) {
                    memset(&forge->upgrades[forge->count], 0, sizeof(EquipItem));
                    forge->upgrades[forge->count].type = type;
                    forge->upgrades[forge->count].id = forge->upgradeEntry->next[i];
                    forge->upgradeCost[forge->count] = Equip_ItemHalfPrice(&forge->upgrades[forge->count]);
                    memcpy(&forge->mats[forge->count], &gGunUpgrades[forge->upgradeEntry->next[i]].mats, sizeof(WeaponMats));
                    forge->count++;
                }
            }
        } else {
            forge->upgradeKind = EQUIP_GUN;
            WeaponForge_LoadGunMods(forge, idx);
        }
        break;
    default:
        forge->upgradeKind = type;
        break;
    }
}

/*
 * The upgrade pages' box grid: circle on a blade or bow with upgrades
 * takes it (the cursor on its first upgrade, its model loaded unless
 * hidden), on a bowgun loads its mods, else refuses; cross closes the
 * piece's detail or leaves; square shows or hides the detail; L1/R1 turn
 * the detail's pages, L2/R2 the box pages; the d-pad moves over the 10 by
 * 10 page. Reloads the upgrades when the slot changes. Returns MENU_DONE
 * when a piece is taken, MENU_CANCEL when left, else MENU_RUNNING.
 */
s32 WeaponForge_BoxInput(WeaponForgeWork *forge, u16 pad)
{
    s32 idx = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    EquipItem upgrade;
    u16 *id;
    u8 *type;
    u8 female;

    memset(&upgrade, 0, sizeof(upgrade));
    if (pad & PAD_CIRCLE) {
        switch (forge->upgradeKind) {
        case EQUIP_BLADE:
            if (forge->count == 0) {
                LobbySe_Play(LOBBY_SE_REFUSE);
                return MENU_RUNNING;
            }
            forge->cursor = 0;
            upgrade = forge->upgrades[forge->cursor];
            forge->result = upgrade;
            if (forge->previewOff == 0) {
                type = &upgrade.type;
                if (*type != forge->shown.type || upgrade.id != forge->shown.id) {
                    forge->shown = upgrade;
                    id = &upgrade.id;
                    if (Forge_IsRandomWeapon(*id) == 0) {
                        EquipPreview_Free();
                        female = gSave.gender;
                        EquipPreview_Load(*type, Equip_GetModelId(*type, *id, female), female);
                    }
                }
                EquipPreview_Hide(0);
            }
            forge->detailPages = Equip_LevelMark(&gSave.box[idx], 0, &upgrade);
            LobbySe_Play(LOBBY_SE_SELECT);
            return MENU_DONE;
        case EQUIP_GUN:
            forge->cursor = 0;
            WeaponForge_LoadGunMods(forge, idx);
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        default:
            LobbySe_Play(LOBBY_SE_REFUSE);
            break;
        }
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        if (forge->view == FORGE_BOX_VIEW_GRID) {
            return MENU_CANCEL;
        }
        forge->view = FORGE_BOX_VIEW_GRID;
    } else if (pad & PAD_SQUARE) {
        if (gSave.box[idx].id != 0) {
            forge->view ^= FORGE_BOX_VIEW_DETAIL;
            forge->detailPages = Equip_LevelMark(&gSave.box[idx], 0, NULL);
            forge->detailPage = 0;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
        }
    } else if (pad & PAD_R1) {
        if (forge->view != FORGE_BOX_VIEW_GRID && forge->detailPages > 1) {
            if (++forge->detailPage >= forge->detailPages) {
                forge->detailPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (forge->view != FORGE_BOX_VIEW_GRID && forge->detailPages > 1) {
            if (--forge->detailPage < 0) {
                forge->detailPage = forge->detailPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_R2) {
        if (forge->view == FORGE_BOX_VIEW_GRID && forge->pages > 1) {
            if (forge->pages > 1) {
                if (++forge->page >= forge->pages) {
                    forge->page = 0;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        }
    } else if (pad & PAD_L2) {
        if (forge->view == FORGE_BOX_VIEW_GRID && forge->pages > 1) {
            if (forge->pages > 1) {
                if (--forge->page < 0) {
                    forge->page = forge->pages - 1;
                }
                LobbySe_Play(LOBBY_SE_SCROLL);
            }
        }
    } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
        if (forge->view == FORGE_BOX_VIEW_GRID) {
            forge->col = Menu_WrapLeftRight(forge->col, BOX_PAGE_COLS);
        }
    } else {
        if (forge->view == FORGE_BOX_VIEW_GRID) {
            forge->row = Menu_WrapUpDown(forge->row, BOX_PAGE_ROWS);
        }
    }
    if (idx != forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col) {
        WeaponForge_LoadUpgrades(forge);
    }
    return MENU_RUNNING;
}

/* FAKEMATCH: the u32 loadedType is only copied into modelType later
 * (`modelType = loadedType;`): loading the type into it before the compare
 * puts the type's load where the game's code has it. */
/*
 * The upgrade or mod list: circle takes the line when it can be paid for
 * (MENU_DONE), cross goes back (MENU_CANCEL); square cycles the detail
 * views (four for a blade or bow, stats and comparison for a bowgun);
 * L1/R1 turn the detail pages; Select shows or hides the model; up/down
 * move the cursor, and the result becomes the line's upgrade, or the gun
 * with the line's mod. Else MENU_RUNNING.
 */
s32 WeaponForge_UpgradeListInput(WeaponForgeWork *forge, u16 pad)
{
    u16 prevCursor = forge->cursor;
    EquipItem upgrade;
    u8 *type;
    u8 female;
    u16 id;
    u8 modelType;
    u32 loadedType;

    if (pad & PAD_CIRCLE) {
        if (WeaponForge_CanUpgrade(forge, forge->cursor) == 1) {
            LobbySe_Play(LOBBY_SE_SELECT);
            return MENU_DONE;
        }
        LobbySe_Play(LOBBY_SE_REFUSE);
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_SQUARE) {
        if (forge->upgradeKind == EQUIP_BLADE) {
            if (++forge->view >= FORGE_UPGRADE_VIEW_COUNT) {
                forge->view = FORGE_UPGRADE_VIEW_STATS;
            }
        } else if (forge->view == FORGE_UPGRADE_VIEW_STATS) {
            forge->view = FORGE_UPGRADE_VIEW_COMPARE;
        } else {
            forge->view = FORGE_UPGRADE_VIEW_STATS;
        }
        forge->detailPage = 0;
        WeaponForge_UpgradeDetail(forge);
        LobbySe_Play(LOBBY_SE_DESCRIPTION);
    } else if (pad & PAD_R1) {
        if (forge->view != FORGE_UPGRADE_VIEW_MATERIALS && forge->detailPages > 1) {
            if (++forge->detailPage >= forge->detailPages) {
                forge->detailPage = 0;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_L1) {
        if (forge->view != FORGE_UPGRADE_VIEW_MATERIALS && forge->detailPages > 1) {
            if (--forge->detailPage < 0) {
                forge->detailPage = forge->detailPages - 1;
            }
            LobbySe_Play(LOBBY_SE_SCROLL);
        }
    } else if (pad & PAD_SELECT) {
        if (forge->upgradeKind != EQUIP_GUN && (forge->view == FORGE_UPGRADE_VIEW_STATS || forge->view == FORGE_UPGRADE_VIEW_MATERIALS)) {
            if (forge->previewOff == 0) {
                forge->previewOff = 1;
                EquipPreview_Hide(1);
            } else {
                forge->previewOff = 0;
                upgrade = forge->upgrades[forge->cursor];
                type = &upgrade.type;
                loadedType = *type;
                if (*type != forge->shown.type || upgrade.id != forge->shown.id) {
                    forge->shown = upgrade;
                    id = upgrade.id;
                    if (Forge_IsRandomWeapon(id) == 0) {
                        EquipPreview_Free();
                        female = gSave.gender;
                        modelType = loadedType;
                        EquipPreview_Load(modelType, Equip_GetModelId(modelType, id, female), female);
                    }
                }
                EquipPreview_Hide(0);
            }
        }
    } else {
        forge->cursor = Menu_WrapUpDownSe(forge->cursor, forge->count, LOBBY_SE_CURSOR);
    }
    if (prevCursor != forge->cursor) {
        if (forge->upgradeKind == EQUIP_GUN) {
            forge->result.level = WeaponForge_ApplyGunMod(forge->gunLevel, forge->mod[forge->cursor]);
        } else {
            forge->result = forge->upgrades[forge->cursor];
        }
        WeaponForge_UpgradeDetail(forge);
    }
    return MENU_RUNNING;
}

/*
 * Whether line i of the upgrade list can be paid for: a mod's price, or an
 * upgrade's price and its three materials at hand. Returns 1 or 0.
 */
s32 WeaponForge_CanUpgrade(WeaponForgeWork *forge, s16 line)
{
    EquipItem upgrade;
    s32 j;

    memset(&upgrade, 0, sizeof(upgrade));
    if (forge->upgradeKind == EQUIP_GUN) {
        if (forge->modCost[line] > gSave.money) {
            return 0;
        }
    } else {
        upgrade = forge->upgrades[line];
        if (Equip_TestFlag(&upgrade, EQUIP_FLAG_MADE) == 0) {
            Equip_SetFlag(&upgrade, EQUIP_FLAG_NEW);
            Equip_SetFlag(&upgrade, EQUIP_FLAG_MADE);
        }
        if (forge->upgradeCost[line] > (u32)gSave.money) {
            return 0;
        }
        for (j = 0; j < WEAPON_UP_MATS; j++) {
            if (Items_HaveEnough(forge->mats[line].need[j].item, forge->mats[line].need[j].count) == 0) {
                return 0;
            }
        }
    }
    return 1;
}

/*
 * Sets up the upgrade pages' detail for the current view: the result's own
 * stats (with its model for a blade or bow, unless hidden), the comparison
 * with the box piece, or the hunter's stats with the result worn; sets the
 * detail's page count.
 */
void WeaponForge_UpgradeDetail(WeaponForgeWork *forge)
{
    EquipItem upgraded;
    u8 *type;
    s32 idx = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    u8 x;

    upgraded = forge->result;
    switch (forge->view) {
    case FORGE_UPGRADE_VIEW_HUNTER:
        EquipPreview_Hide(1);
        forge->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &upgraded);
        forge->detailPage = 0;
        break;
    default:
        forge->detailPages = Equip_LevelMark(&gSave.box[idx], 16, &upgraded);
        EquipPreview_Hide(1);
        break;
    case FORGE_UPGRADE_VIEW_COMPARE:
        forge->detailPages = Equip_LevelMark(&gSave.box[idx], 16, &upgraded);
        EquipPreview_Hide(1);
        break;
    case FORGE_UPGRADE_VIEW_STATS:
    case FORGE_UPGRADE_VIEW_MATERIALS:
        forge->detailPages = Equip_LevelMark(&upgraded, 0, NULL);
        if (forge->upgradeKind == EQUIP_BLADE) {
            if (forge->previewOff == 0) {
                type = &upgraded.type;
                if (upgraded.type != forge->shown.type || upgraded.id != forge->shown.id) {
                    forge->shown = upgraded;
                    if (Forge_IsRandomWeapon(upgraded.id) == 0) {
                        EquipPreview_Free();
                        x = gSave.gender;
                        EquipPreview_Load(*type, Equip_GetModelId(*type, upgraded.id, x), x);
                    }
                }
                EquipPreview_Hide(0);
            }
        } else {
            EquipPreview_Hide(1);
        }
        break;
    }
}

/*
 * Sets up the forging page's detail for the current view: the recipe's
 * weapon's stats, or its materials (each with the model unless hidden),
 * the comparison with the worn weapon, or the hunter's stats with it worn;
 * sets the page count and keeps the detail page within it.
 */
void WeaponForge_MakeDetail(WeaponForgeWork *forge)
{
    s32 line = forge->cursor;
    EquipItem made;
    ForgeRecipe *recipe;
    u16 *id;
    u8 *type;
    u8 female;

    memset(&made, 0, sizeof(made));
    type = &made.type;
    id = &made.id;
    recipe = &gWeaponRecipes[forge->makeList[line]];
    *type = recipe->type;
    *id = recipe->id;
    switch (forge->view) {
    case FORGE_MAKE_VIEW_HUNTER:
        EquipPreview_Hide(1);
        forge->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &made);
        forge->detailPage = 0;
        break;
    default:
        forge->detailPages = Equip_LevelMark(gSave.equip, 16, &made);
        EquipPreview_Hide(1);
        break;
    case FORGE_MAKE_VIEW_COMPARE:
        forge->detailPages = Equip_LevelMark(gSave.equip, 80, &made);
        EquipPreview_Hide(1);
        break;
    case FORGE_MAKE_VIEW_STATS:
    case FORGE_MAKE_VIEW_MATERIALS:
        if (forge->view == FORGE_MAKE_VIEW_MATERIALS) {
            forge->detailPages = 0;
        } else {
            forge->detailPages = Equip_LevelMark(&made, 64, NULL);
        }
        if (forge->previewOff == 0) {
            if (*type != forge->shown.type || *id != forge->shown.id) {
                forge->shown = made;
                if (Forge_IsRandomWeapon(*id) == 0) {
                    EquipPreview_Free();
                    female = gSave.gender;
                    EquipPreview_Load(*type, Equip_GetModelId(*type, *id, female), female);
                }
            }
            EquipPreview_Hide(0);
        }
        break;
    }
    if (forge->detailPages > 0) {
        if (forge->detailPage >= forge->detailPages) {
            forge->detailPage = forge->detailPages - 1;
        }
    } else {
        forge->detailPage = 0;
    }
}

/*
 * Draws the three materials of the upgrade at the cursor at (x, y), with
 * the armour upgrade's drawer; the bow test on e has no use.
 */
void WeaponForge_DrawMaterials(WeaponForgeWork *forge, EquipItem *result, s32 x, s32 y)
{
    WeaponForge_IsBow(result);
    Forge_DrawMaterials(x, y, forge->mats[forge->cursor].need, WEAPON_UP_MATS);
}

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: declared with s16 coordinates as the forge's draws pass them; with the definition's s32 coordinates and s16 kind the calls compile differently */
void Font_SetSpacing(u8 a, u8 b);
/*
 * Draws the help window of the gun mod at the cursor, its description
 * text, at (x, y).
 */
void WeaponForge_DrawGunModHelp(WeaponForgeWork *forge, s16 x, s16 y)
{
    s32 helpText;

    Win_Draw(x, y, 306, 114, 0xBDB59F6C, 0xA0202020, 6);
    Font_SetSpacing(18, 18);
    Font_SetPos(x + 8, y + 4);
    helpText = forge->mod[forge->cursor] + GUN_MOD_HELP_OFFSET;
    Text_Printf(gForgeTexts[helpText]);
}

extern LobbyMenu *gLobbyMenu;
/*
 * Draws the upgrade pages' right side by sub-step: in the list, the
 * current view (the result's stats with the model's help, or a bowgun's
 * mod help; the materials; the comparison; the hunter's stats) and the
 * list's hint; at the price question, the comparison and the question (a
 * mod removal has its own) with yes/no; otherwise the box grid at x, or
 * the box piece's detail, with the box's hint.
 */
void WeaponForge_DrawUpgradeDetail(WeaponForgeWork *forge, s32 x, s16 y)
{
    s32 idx = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    EquipItem shownItem;
    char *text;

    shownItem = gSave.box[idx];
    if (forge->upgradeKind == EQUIP_BLADE) {
        shownItem = forge->upgrades[forge->cursor];
        if (shownItem.id != 0) {
            shownItem.used = 1;
        } else {
            shownItem.used = 0;
        }
    } else {
        shownItem = forge->result;
    }
    switch (forge->subStep) {
    case FORGE_UPGRADE_LIST:
        switch (forge->view) {
        case FORGE_UPGRADE_VIEW_STATS:
            if (forge->upgradeKind != EQUIP_BLADE) {
                EquipWindow_DrawLocal(&shownItem, 310, y + 4, forge->detailPage | 0x40, NULL);
                WeaponForge_DrawGunModHelp(forge, 310, 224);
            } else {
                if (forge->previewOff == 0) {
                    EquipPreview_Hide(0);
                }
                Equip_DrawPreviewHelp(0xBDB59F6C, forge->previewOff);
                if (forge->previewOff == 0 && Forge_IsRandomWeapon(forge->shown.id)) {
                    text = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
                    EquipPreview_Hide(1);
                    Font_SetSpacing(18, 18);
                    Font_SetPos(Text_CenterX(text, 462, 18), 110);
                    Font_SetColor(0x807F212E);
                    Text_PrintfColored(text);
                }
                EquipWindow_DrawLocal(&shownItem, 310, 224, forge->detailPage | 0x40, NULL);
            }
            break;
        case FORGE_UPGRADE_VIEW_COMPARE:
            EquipCompare_Draw(&gSave.box[idx], &shownItem, 310, y + 4, forge->detailPage | 0x50);
            break;
        case FORGE_UPGRADE_VIEW_MATERIALS:
            if (forge->previewOff == 0) {
                EquipPreview_Hide(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, forge->previewOff);
            if (forge->previewOff == 0 && Forge_IsRandomWeapon(forge->shown.id)) {
                text = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
                EquipPreview_Hide(1);
                Font_SetSpacing(18, 18);
                Font_SetPos(Text_CenterX(text, 462, 18), 110);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(text);
            }
            WeaponForge_DrawMaterials(forge, &shownItem, 310, 224);
            break;
        case FORGE_UPGRADE_VIEW_HUNTER:
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, 58, forge->detailPage);
            return;
        }
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_PICK_UPGRADE_KIND]);
        break;
    case FORGE_UPGRADE_ASK:
        EquipCompare_Draw(&gSave.box[idx], &shownItem, 310, y + 4, forge->detailPage | 0x50);
        if (forge->upgradeKind == EQUIP_BLADE) {
            text = Text_FillDollar(gForgeTexts[FORGE_TEXT_MATERIALS_FEE_ASK], forge->upgradeCost[forge->cursor]);
        } else {
            switch (forge->mod[forge->cursor]) {
            case GUN_MOD_REMOVE_SILENCER:
                text = Text_FillDollar(gForgeTexts[FORGE_TEXT_MOD_FEE_DROPS_SILENCER], forge->modCost[forge->cursor]);
                break;
            case GUN_MOD_REMOVE_BARREL:
                text = Text_FillDollar(gForgeTexts[FORGE_TEXT_MOD_FEE_DROPS_BARREL], forge->modCost[forge->cursor]);
                break;
            case GUN_MOD_REMOVE_SHIELD:
                text = Text_FillDollar(gForgeTexts[FORGE_TEXT_MOD_FEE_DROPS_SHIELD], forge->modCost[forge->cursor]);
                break;
            case GUN_MOD_REMOVE_HEAVY_BARREL:
                text = Text_FillDollar(gForgeTexts[FORGE_TEXT_MOD_FEE_DROPS_HEAVY_BARREL], forge->modCost[forge->cursor]);
                break;
            default:
                text = Text_FillDollar(gForgeTexts[FORGE_TEXT_MOD_FEE_ASK], forge->modCost[forge->cursor]);
                break;
            }
        }
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, text);
        Font_SetPos(444, 400);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    default:
        if (forge->view == FORGE_BOX_VIEW_GRID) {
            BoxMenu_DrawItemList(idx, 1040, x);
        } else {
            BoxMenu_DrawItemList(idx, (forge->detailPage & 7) | 0x478, x);
        }
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_PICK_WEAPON_UPGRADE]);
        break;
    }
}

/*
 * Draws the list of what the box piece can become at (x, y): the header,
 * each upgrade's name and price (greyed when it can't be paid for, the new
 * mark blinking), empty lines up to four, the cursor bar in the list's
 * sub-step; or the no-upgrade text when there is none.
 */
void WeaponForge_DrawUpgrades(WeaponForgeWork *forge, s16 x, s16 y)
{
    EquipItem *upgrade;
    u32 cost;
    s16 i;
    s16 lineY = y;

    if (forge->upgradeEntry->next[0] == 0 || forge->count == 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(Text_CenterX(gForgeTexts[FORGE_TEXT_NO_UPGRADE], x + 138, 17), y + 51);
        Text_PrintfColored(gForgeTexts[FORGE_TEXT_NO_UPGRADE]);
        return;
    }
    lineY += 10;
    Font_SetPos(x + 10, lineY);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(gForgeTexts[FORGE_TEXT_UPGRADES_HEAD]);
    for (i = 0; i < FORGE_UPGRADE_LINES; i++) {
        if (i < forge->count) {
            upgrade = &forge->upgrades[i];
            cost = forge->upgradeCost[i];
            if (WeaponForge_CanUpgrade(forge, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x807F7F7F);
            }
            lineY += 20;
            Font_SetPos(x + 14, lineY);
            Text_PrintfColored(Equip_GetName(upgrade->type, upgrade->id));
            Font_SetPos(x + 200, lineY);
            Text_PrintfColored(gForgePriceFmt, cost);
            if (Equip_TestFlag(upgrade, EQUIP_FLAG_NEW) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw(x, lineY - 4, 16, 20, 0xFFC03000, 99);
            }
        } else {
            Font_SetColor(0x8042352E);
            lineY += 20;
            Font_SetPos(x + 14, lineY);
            Text_PrintfColored(gForgeTexts[FORGE_TEXT_EMPTY_LINE]);
        }
        if (forge->cursor == i && forge->subStep == FORGE_UPGRADE_LIST) {
            Menu_DrawGlowBar(x + 138, lineY - 1, 276, 18, 0xB05050F0);
        }
    }
}

/*
 * Draws the mod list of the bowgun in box slot idx at (x, y): the header,
 * each mod's name and price (greyed when it costs more than the money
 * held) and the cursor bar in the list's sub-step.
 */
void WeaponForge_DrawGunMods(WeaponForgeWork *forge, s32 idx, s16 x, s16 y)
{
    s16 lineY = y;
    s16 i;

    lineY += 10;
    Font_SetPos(x + 10, lineY);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(gForgeTexts[FORGE_TEXT_MODS_HEAD]);
    Equip_GetPrice(EQUIP_GUN, gSave.box[idx].id);
    for (i = 0; i < GUN_MOD_LINES; i++) {
        if (forge->mod[i] != GUN_MOD_NONE) {
            if (forge->modCost[i] > gSave.money) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x807F7F7F);
            }
            lineY += 20;
            Font_SetPos(x + 14, lineY);
            Text_PrintfColored(gForgeTexts[forge->mod[i]]);
            Font_SetPos(x + 200, lineY);
            Text_PrintfColored(gForgePriceFmt, forge->modCost[i]);
            if (forge->cursor == i && forge->subStep == FORGE_UPGRADE_LIST) {
                Menu_DrawGlowBar(x + 138, lineY - 1, 276, 18, 0xB05050F0);
            }
        }
    }
}

void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: declared with s16 coordinates as the forge's draws pass them; with the definition's s32 ones the calls compile differently */
s32 Box_CountEquip(u8 type, u16 id);
/*
 * Draws the upgrade pages' left side: the title bar and the money, and for
 * a weapon under the box cursor a window (lit in the list's sub-step) with
 * its upgrade or mod list and, under it, the result's icon, name and how
 * many the box holds.
 */
void WeaponForge_DrawUpgradePanel(WeaponForgeWork *forge, s16 x, s16 y)
{
    s32 idx = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    EquipItem shownItem;
    u32 frameColor;
    u32 fillColor;
    u8 *type;
    s32 panelY;
    s32 textX;
    u16 *id;

    memset(&shownItem, 0, sizeof(shownItem));
    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 160, 0xFFB0B000, gWeaponForgeTexts[forge->choice], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    switch (forge->subStep) {
    case FORGE_UPGRADE_LIST:
        frameColor = 0xBDB59F6C;
        fillColor = 0xA0202020;
        break;
    default:
        frameColor = 0xFF202020;
        fillColor = 0xA0000000;
        break;
    }
    switch (gSave.box[idx].type) {
    default:
        return;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        Font_SetSpacing(17, 17);
        Win_Draw(x, y, 276, 174, frameColor, fillColor, 6);
        switch (forge->upgradeKind) {
        case EQUIP_BLADE:
            if (WeaponForge_IsBow(&gSave.box[idx]) == 1) {
                forge->upgradeEntry = &gGunUpgrades[gSave.box[idx].id];
            } else {
                forge->upgradeEntry = &gBladeUpgrades[gSave.box[idx].id];
            }
            WeaponForge_DrawUpgrades(forge, x, y);
            shownItem = forge->upgrades[forge->cursor];
            if (forge->upgradeEntry->next[0] == 0 || forge->count == 0) {
                return;
            }
            break;
        case EQUIP_GUN:
            shownItem = forge->result;
            WeaponForge_DrawGunMods(forge, idx, x, y);
            break;
        default:
            return;
        }
        panelY = y;
        textX = x;
        Gfx_DrawLine(textX + 8, panelY + 116, 260, 2, frameColor);
        id = &shownItem.id;
        if (*id != 0) {
            Lobby_DrawEquipIcon(textX + 10, panelY + 118, 48, 48, &shownItem);
            Font_SetSpacing(18, 18);
            Font_SetColor(0x807F7F7F);
            Font_SetPos(textX += 70, panelY + 124);
            type = &shownItem.type;
            Text_PrintfColored(gForgeTextFmt, Equip_GetName(*type, *id));
            Font_SetPos(textX, panelY + 144);
            Text_PrintfColored("%s%s%3d", gCountLabels[COUNT_LABEL_HELD], gLobbyWords[LOBBY_WORD_TIMES], Box_CountEquip(*type, *id));
        }
        break;
    }
}

/* Returns the price of making a weapon at the forge: half of the piece's
 * shop price (Equip_GetPrice). */
u32 Equip_HalfPrice(s32 type, s32 id)
{
    return Equip_GetPrice(type & 0xFF, id & 0xFFFF) >> 1;
}

/* gWorkshopTexts[WORKSHOP_TEXT_NO_WEAPON_TO_MAKE] under a name of its own (a linker
 * alias): read through the table, the load is scheduled differently from the
 * game's code. */
extern char *gNoWeaponToMakeText;
struct WeaponForgeWork;
/*
 * Draws the forge list on its own (no detail panel): a page of the chosen
 * weapon class's recipes with their names and prices, the page number, the
 * blinking new mark, and the cursor. Recipes the hunter can't make yet are
 * greyed; hidden-name recipes show an unknown name and the fixed price.
 * Under the list it draws the selected weapon's shop line, or, when the
 * class has no recipe, a centred note instead of the list.
 */
void WeaponForge_DrawMakeListFull(WeaponForgeWork *forge, s16 x, s16 y)
{
    ForgeRecipe *recipe;
    s16 rowY = y;
    EquipItem item;
    s16 i;
    s32 textX;
    EquipItem selected;
    s16 blink;
    s32 top;
    s32 left;
    char *note;
    s32 boxTop;
    s16 noteX;

    memset(&item, 0, sizeof(item));
    memset(&selected, 0, sizeof(selected));
    Icon_Draw(x, y, 340, 290, 0xFFFFDDDD, 0);
    top = rowY;
    left = x;
    boxTop = rowY;
    Icon_Draw(left + 110, top + 45, 120, 200, 0x60FFFFFF, 36);
    Font_SetColor(0x80101010);
    if (forge->count == 0) {
        note = gNoWeaponToMakeText;
        noteX = Text_CenterX(note, left + 170, 18);
        Font_SetColor(0x8000052C);
        Font_SetSpacing(18, 18);
        Font_SetPos(noteX, boxTop + 90);
        Text_PrintfColored(gForgeTextFmt, note);
        Menu_DrawDashLine(left + 20, top + 186, 300, 0xFF010101);
        return;
    }
    Menu_DrawPageNum(left + 250, top + 14, forge->page, forge->pages, 1);
    rowY += 34;
    Font_SetSpacing(20, 20);
    i = 0;
    if (i < forge->count) {
        textX = left + 40;
        do {
            recipe = &gWeaponRecipes[forge->makeList[i]];
            item.type = recipe->type;
            item.id = recipe->id;
            if (Equip_TestFlag(&item, EQUIP_FLAG_NEW) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw((s16)textX - 20, rowY, 16, 20, 0xFFC03000, 99);
            } else {
                Lobby_DrawEquipIcon((s16)textX - 26, rowY, 20, 20, &item);
            }
            if (i == forge->cursor) {
                selected = item;
                if (gFrameTick & 0x10) {
                    blink = 4;
                } else {
                    blink = 10;
                }
                Icon_Draw(textX, rowY + 14, 274, 8, 0x99106060, 111);
                Menu_DrawCursorIcon((s16)textX - 50 - blink, rowY - 2, 24, 0xFF20C0C0);
            }
            if (WeaponForge_CanMake((struct WeaponForgeWork *)forge, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(textX, rowY);
            if (Forge_IsRandomWeapon(item.id) != 0) {
                Text_PrintfColored("%-20s %6d%s", gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME], WEAPON_RANDOM_PRICE, gLobbyWords[LOBBY_WORD_ZENNY]);
            } else {
                Text_PrintfColored("%-20s %6d%s", Equip_GetName(recipe->type, recipe->id), Equip_HalfPrice(recipe->type, recipe->id), gLobbyWords[LOBBY_WORD_ZENNY]);
            }
            rowY += 22;
            i++;
        } while (i < forge->count);
    }
    Menu_DrawDashLine(left + 20, top + 186, 300, 0xFF000000);
    EquipShop_DrawLine(left + 16, top + 200, &selected);
}

/*
 * Draws the narrow forge list shown beside a detail panel: the same rows
 * as WeaponForge_DrawMakeListFull (icon or new mark, cursor, greyed rows,
 * name and price in two columns), in a smaller window and without the
 * bottom line.
 */
void WeaponForge_DrawMakeList(WeaponForgeWork *forge, s16 x, s16 y)
{
    ForgeRecipe *recipe;
    s16 rowY = y;
    EquipItem item;
    s16 i;
    s32 textX;
    EquipItem selected;
    s16 blink;
    s32 top;
    s32 left;

    memset(&item, 0, sizeof(item));
    memset(&selected, 0, sizeof(selected));
    Icon_Draw(x, y, 290, 180, 0xFFFFDDDD, 0);
    top = rowY;
    left = x;
    Icon_Draw(left + 100, top + 15, 90, 150, 0x60FFFFFF, 36);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(left + 200, top + 12, forge->page, forge->pages, 1);
    rowY += 34;
    Font_SetSpacing(17, 17);
    i = 0;
    if (i < forge->count) {
        textX = left + 40;
        do {
            recipe = &gWeaponRecipes[forge->makeList[i]];
            item.type = recipe->type;
            item.id = recipe->id;
            if (Equip_TestFlag(&item, EQUIP_FLAG_NEW) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw((s16)textX - 20, rowY, 16, 20, 0xFFC03000, 99);
            } else {
                Lobby_DrawEquipIcon((s16)textX - 26, rowY, 20, 20, &item);
            }
            if (i == forge->cursor) {
                selected = item;
                if (gFrameTick & 0x10) {
                    blink = 4;
                } else {
                    blink = 10;
                }
                Icon_Draw((s16)textX - 8, rowY + 12, 248, 8, 0x99106060, 111);
                Menu_DrawCursorIcon((s16)textX - 42 - blink, rowY - 2, 22, 0xFF20C0C0);
            }
            if (WeaponForge_CanMake((struct WeaponForgeWork *)forge, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(textX, rowY);
            if (Forge_IsRandomWeapon(item.id) != 0) {
                Text_PrintfColored("%-20s", gGameMenuLabels[MENU_LABEL_UNKNOWN_NAME]);
                Font_SetPos((s16)textX + 178, rowY);
                Text_PrintfColored("%6d%s", WEAPON_RANDOM_PRICE, gLobbyWords[LOBBY_WORD_ZENNY]);
            } else {
                Text_PrintfColored("%-20s", Equip_GetName(recipe->type, recipe->id));
                Font_SetPos((s16)textX + 178, rowY);
                Text_PrintfColored("%6d%s", Equip_HalfPrice(recipe->type, recipe->id), gLobbyWords[LOBBY_WORD_ZENNY]);
            }
            rowY += 20;
            i++;
        } while (i < forge->count);
    }
}

/*
 * Draws the forging page for the make step: the class choice, then the
 * class's label bar, the money and the recipe list with the detail panel
 * chosen by the detail view (stats window, materials, comparison or the
 * hunter's stats), and the help box. At the confirm step it draws the
 * comparison with the worn equipment, the price question with the price
 * filled in, and the yes/no choice.
 */
void WeaponForge_DrawMake(WeaponForgeWork *forge)
{
    s32 cursorIdx = forge->cursor;
    EquipItem item;
    s16 x;
    s16 y;
    char *prompt;
    ForgeRecipe *recipe;

    memset(&item, 0, sizeof(item));
    if (forge->subStep != FORGE_MAKE_TAB) {
        Font_SetSpacing(22, 22);
        Menu_DrawLabelBar(10, 18, 190, 0xFFB0B000, gWeaponClassTexts[forge->row], 2, 1);
        Menu_DrawMoney(480, 16, 0);
        recipe = &gWeaponRecipes[forge->makeList[cursorIdx]];
        item.used = 1;
        y = 52;
        item.type = recipe->type;
        item.id = recipe->id;
        x = forge->detailWin.pos;
    }
    switch (forge->subStep) {
    case FORGE_MAKE_TAB:
        Font_SetSpacing(20, 20);
        Menu_DrawLabelList(420, gWeaponClassTexts, WEAPON_TAB_COUNT, forge->row, 0xFFB0B000);
        break;
    case FORGE_MAKE_SLIDE_IN:
    case FORGE_MAKE_LIST:
        switch (forge->view) {
        case FORGE_MAKE_VIEW_LIST:
            WeaponForge_DrawMakeListFull(forge, x + 12, y);
            break;
        case FORGE_MAKE_VIEW_STATS:
            if (forge->previewOff == 0) {
                EquipPreview_Hide(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, forge->previewOff);
            if (forge->previewOff == 0 && Forge_IsRandomWeapon(forge->shown.id)) {
                prompt = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
                EquipPreview_Hide(1);
                Font_SetSpacing(18, 18);
                Font_SetPos(Text_CenterX(prompt, 462, 18), 110);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(prompt);
            }
            WeaponForge_DrawMakeList(forge, 10, 50);
            EquipWindow_DrawLocal(&item, 310, 216, forge->detailPage | 0x40, NULL);
            break;
        case FORGE_MAKE_VIEW_MATERIALS:
            if (forge->previewOff == 0) {
                EquipPreview_Hide(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, forge->previewOff);
            if (forge->previewOff == 0 && Forge_IsRandomWeapon(forge->shown.id)) {
                prompt = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
                EquipPreview_Hide(1);
                Font_SetSpacing(18, 18);
                Font_SetPos(Text_CenterX(prompt, 462, 18), 110);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(prompt);
            }
            WeaponForge_DrawMakeList(forge, 10, 50);
            WeaponForge_DrawMakeMaterials(forge, 310, 216);
            break;
        case FORGE_MAKE_VIEW_COMPARE:
            WeaponForge_DrawMakeList(forge, 10, 50);
            EquipCompare_Draw(gSave.equip, &item, 310, 54, forge->detailPage | 0x50);
            break;
        case FORGE_MAKE_VIEW_HUNTER:
            WeaponForge_DrawMakeList(forge, 10, 50);
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, 58, forge->detailPage);
            break;
        }
        if (forge->view != FORGE_MAKE_VIEW_HUNTER) {
            Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_PICK_WEAPON_TO_MAKE]);
        }
        break;
    case FORGE_MAKE_ASK:
        EquipCompare_Draw(gSave.equip, &item, 310, 54, forge->detailPage | 0x50);
        if (Forge_IsRandomWeapon(item.id)) {
            prompt = Text_FillDollar((char *) gForgeTexts[FORGE_TEXT_MATERIALS_ASK], WEAPON_RANDOM_PRICE);
        } else {
            prompt = Text_FillDollar((char *) gForgeTexts[FORGE_TEXT_MATERIALS_ASK], Equip_HalfPrice(item.type, item.id));
        }
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, prompt);
        Font_SetPos(444, 400);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

/*
 * Draws the materials panel for the recipe under the cursor: a window with
 * its header and the four needed items, each with the count held against
 * the count needed.
 */
void WeaponForge_DrawMakeMaterials(WeaponForgeWork *forge, s16 x, s16 y)
{
    ForgeRecipe *recipe = &gWeaponRecipes[forge->makeList[forge->cursor]];
    s16 i;

    Win_Draw(x, y, 306, 114, 0xBDB59F6C, 0xA0202020, 6);
    y += 6;
    Font_SetSpacing(18, 18);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 10, y);
    Text_PrintfColored(gForgeTexts[FORGE_TEXT_MATERIALS_HEAD]);
    for (i = 0; i < FORGE_RECIPE_MATS; i++) {
        y += 20;
        Items_DrawNeedLine(x + 30, y, recipe->mat[i].item, recipe->mat[i].count, 0x807F7F7F);
    }
}

/*
 * Draws the weapon workshop by its step: the forge/upgrade choice, the
 * upgrade pages (the chosen weapon's detail and the upgrade box), or the
 * forging page. work is the workshop's work area (gMenuWork); its
 * WeaponForgeWork starts FORGE_WORK_OFFSET bytes in.
 */
void WeaponForge_Draw(u8 *work)
{
    WeaponForgeWork *forge = (WeaponForgeWork *)(work + FORGE_WORK_OFFSET);

    switch (forge->step) {
    case FORGE_STEP_CHOOSE:
        Font_SetSpacing(20, 20);
        Menu_DrawLabelList(460, gWeaponForgeTexts, 2, forge->choice, 0xFFB0B000);
        break;
    case FORGE_STEP_UPGRADE:
        WeaponForge_DrawUpgradeDetail(forge, (s16)(forge->detailWin.pos + 10), 50);
        WeaponForge_DrawUpgradePanel(forge, forge->panelWin.pos + 6, 54);
        break;
    case FORGE_STEP_MAKE:
        WeaponForge_DrawMake(forge);
        break;
    }
}


/*
 * Draws the equip-now question after a weapon is made or upgraded: the new
 * weapon (the recipe's, the rolled piece for a hidden-name recipe, or the
 * chosen upgrade) compared with the worn equipment, and the question with
 * its yes/no choice. It draws nothing at WEAPON_CONFIRM_SETUP or WEAPON_CONFIRM_APPLY.
 */
void WeaponForge_DrawPreview(u8 *work)
{
    WeaponForgeWork *forge = (WeaponForgeWork *)(work + FORGE_WORK_OFFSET);
    ForgeRecipe *recipe;
    char *question;
    s32 recipeIndex;
    EquipItem item;

    if (forge->step == WEAPON_CONFIRM_SETUP) {
        return;
    }
    memset(&item, 0, sizeof(EquipItem));
    if (forge->choice == FORGE_CHOICE_MAKE) {
        recipeIndex = forge->makeList[forge->cursor];
        recipe = &gWeaponRecipes[recipeIndex];
        if (Forge_IsRandomWeapon(gWeaponRecipes[recipeIndex].id)) {
            item = forge->result;
        } else {
            item.used = 1;
            item.type = recipe->type;
            item.id = recipe->id;
        }
    } else if (forge->upgradeKind == EQUIP_BLADE) {
        item = forge->upgrades[forge->cursor];
        if (item.id != 0) {
            item.used = 1;
        } else {
            item.used = 0;
        }
    } else {
        item = forge->result;
    }
    switch (forge->step) {
    case WEAPON_CONFIRM_ASK:
        question = gForgeTexts[FORGE_TEXT_EQUIP_ASK];
        break;
    case WEAPON_CONFIRM_ASK_CHANGE:
        question = gForgeTexts[FORGE_TEXT_WEAPON_CHANGE_WARN];
        break;
    case WEAPON_CONFIRM_APPLY:
        return;
    }
    EquipCompare_Draw(gSave.equip, &item, 308, 56, forge->detailPage | 0x50);
    Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, question);
    Font_SetPos(324, 400);
    Text_Printf(gEquipNowTexts[gLobbyMenu->yesNo]);
}

/* The weapon forge's tables: the gun mods' cost rates and the pieces a hidden-name recipe can roll. */
s32 gGunModCostRates[4] = {
#include "lobby/007E9360.inc"
};
RandEquip gRandomEquips[11] = {
#include "lobby/007E9370.inc"
};
