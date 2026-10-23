/* The workshop's armour forge, the first of its counters: making armour
 * from gArmorRecipes, or upgrading a piece of the equipment box.
 *
 * Its work is an ArmorForgeWork at gMenuWork + FORGE_WORK_OFFSET, after the
 * workshop's own header; the workshop's update runs ArmorForge_Input, then
 * ArmorForge_Confirm, and draws with ArmorForge_Draw. Making: the armour
 * type (gEquipTabTypes), then a page of the recipes the local hunter can
 * wear and has unlocked; the price is taken with the materials and the
 * piece goes to the box. Upgrading: a piece of the box (ten by ten a page)
 * goes one level up or becomes another piece. Both end in a price question
 * and a question whether to wear the result at once. Square cycles a detail
 * view (the piece, its materials, a comparison with the worn piece, the
 * skills); select hides or shows the model preview. With the workshop's
 * short list (WorkshopMenu.shortList) the forge opens on making and lists
 * only gForgeRecipeList's recipes. The pages, their states and views and the
 * closing question's steps are lobby_forge.h's, shared with the weapon
 * forge. Equip_CopyEquipped, among the forge's functions, copies the worn
 * piece of a type for the comparison; the decorations counter calls it
 * too. */
#include "common.h"
#include "lobby_state.h"
#include "lobby_text.h"
#include "lobby_globals.h"
#include "save.h"
#include "scene.h"
#include "player.h"
#include "lobby_menu.h"
#include "lobby_forge.h"
#include "pad.h"
#include "lobby_labels.h"
#include "area_draw.h"
#include "bgm.h"
#include "chat_box.h"
#include "box_menu.h"
#include "camera.h"
#include "model.h"
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
#include "town_frame.h"
#include "libc.h"
#include "window.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4
extern s8 gMenuInputLock;
/* end of prototypes */

struct Player;
/*
 * Sets up the armour forge when the workshop opens it: the model preview
 * starts hidden and the workshop's short-list flag
 * (WorkshopMenu.shortList) is copied to filter. With it set the forge
 * skips the choice and opens production on its first armour type.
 */
void ArmorForge_Open(WorkshopMenu *menu)
{
    ArmorForgeWork *forge = (ArmorForgeWork *)((u8 *)menu + FORGE_WORK_OFFSET);

    forge->previewOff = 1;
    forge->filter = menu->shortList;
    if (forge->filter != 0) {
        forge->previewOff = 1;
        forge->row = 0;
        forge->choice = FORGE_CHOICE_MAKE;
        forge->step = FORGE_STEP_MAKE;
        ArmorForge_OpenMake(forge);
        Chat_SetColor(0);
    }
}

/*
 * The forge's input each frame, by step: the choice between production and
 * upgrades (circle enters one, cross leaves the forge), then the upgrade
 * screen or the production screen. Returns MENU_DONE when a piece is
 * confirmed (step goes back to 0 for the confirmation), MENU_CANCEL when
 * the forge is left (also when production is cancelled with the filter
 * set), MENU_RUNNING otherwise.
 */
s32 ArmorForge_Input(u8 *task, u16 pad)
{
    ArmorForgeWork *forge = (ArmorForgeWork *)(task + 12);

    switch (forge->step) {
    case FORGE_STEP_CHOOSE:
        if (pad & PAD_CIRCLE) {
            forge->previewOff = 1;
            forge->row = 0;
            if (forge->choice == FORGE_CHOICE_MAKE) {
                forge->step = FORGE_STEP_MAKE;
                ArmorForge_OpenMake(forge);
            } else {
                forge->step = FORGE_STEP_UPGRADE;
                ArmorForge_LoadBox(forge);
                ArmorForge_AddUpgrade(forge, 0);
                CamFocus_SetAside(1);
                EquipPreview_Reset();
            }
            Chat_SetColor(0);
            LobbySe_Play(LOBBY_SE_SELECT);
        } else if (pad & PAD_CROSS) {
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            forge->choice = Menu_WrapUpDownSe(forge->choice, 2, LOBBY_SE_CURSOR);
        }
        break;
    case FORGE_STEP_UPGRADE:
        switch (ArmorForge_UpgradeInput(forge, pad)) {
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
        switch (ArmorForge_MakeInput(forge, pad)) {
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
 * Opens the upgrade screen on the equipment box's first page and first
 * slot: counts the box pages, runs ArmorForge_AddUpgrade over every used
 * armour slot of the box (weapons skipped) and starts both windows sliding
 * in.
 */
void ArmorForge_LoadBox(ArmorForgeWork *forge)
{
    s32 i;

    forge->page = 0;
    forge->pages = Box_GetPageCount(&gSave);
    forge->col = 0;
    forge->row = 0;
    forge->view = FORGE_BOX_VIEW_GRID;
    for (i = 0; i < BOX_SLOT_COUNT; i++) {
        if (gSave.box[i].used != 0) {
            switch (gSave.box[i].type) {
            case EQUIP_BLADE:
            case EQUIP_GUN:
                break;
            default:
                ArmorForge_AddUpgrade(forge, i);
                break;
            }
        }
    }
    WinSlide_Step(&forge->listWin, 640, 300);
    WinSlide_Step(&forge->upgradeWin, -296, 10);
}

/*
 * The equipment box's input on the upgrade screen: circle takes the armour
 * piece under the cursor when it has upgrades (MENU_DONE), refusing a
 * weapon or a piece without any; cross closes the details or leaves
 * (MENU_CANCEL). Square toggles the piece's details, R1/L1 turn their
 * pages, R2/L2 the box pages, the d-pad moves the cursor; when the cursor
 * reaches another slot its upgrades are rebuilt. Returns MENU_RUNNING
 * otherwise.
 */
s32 ArmorForge_BoxInput(ArmorForgeWork *forge, u16 pad)
{
    s32 oldSlot = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    s32 newSlot;

    if (pad & PAD_CIRCLE) {
        switch (gSave.box[oldSlot].type) {
        case EQUIP_BLADE:
        case EQUIP_GUN:
            LobbySe_Play(LOBBY_SE_REFUSE);
            break;
        default:
            if (forge->count != 0) {
                LobbySe_Play(LOBBY_SE_SELECT);
                return MENU_DONE;
            }
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
        if (gSave.box[oldSlot].id != 0) {
            forge->view ^= FORGE_BOX_VIEW_DETAIL;
            LobbySe_Play(LOBBY_SE_DESCRIPTION);
            forge->detailPage = 0;
            forge->detailPages = Equip_LevelMark(&gSave.box[oldSlot], 0, NULL);
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
    newSlot = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    if (oldSlot != newSlot) {
        switch (gSave.box[newSlot].type) {
        case EQUIP_BLADE:
        case EQUIP_GUN:
            forge->pieceType = gSave.box[newSlot].type;
            break;
        default:
            ArmorForge_AddUpgrade(forge, newSlot);
            break;
        }
    }
    return MENU_RUNNING;
}

s32 Forge_CheckCondition(s16 cond);
/*
 * Builds the list of what the box slot's armour piece can become: first
 * the same piece one level up (unless it is at its last level), then each
 * piece it can change into at its level. An entry is kept when its
 * materials' conditions are met (with FORGE_COND_OBTAINED or FORGE_COND_HR
 * also the material obtained) and, for a new piece, when the local
 * player's gender can wear it; a new piece seen for the first time gets
 * the made and new marks. Fills upgrades, cost, mats and count, and when
 * there is one, the box piece, the worn piece of its type and the first
 * entry as the choice.
 */
void ArmorForge_AddUpgrade(ArmorForgeWork *forge, s32 slot)
{
    s8 cond[4];
    EquipItem candidate;
    u16 unlocks[4];
    ItemStack need[6];
    s32 blocked;
    s32 i;
    s32 j;
    s32 unlockCount;
    s32 k;

    memset(&candidate, 0, sizeof(candidate));
    memset(forge->upgrades, 0, sizeof(forge->upgrades));
    memset(forge->cost, 0, sizeof(forge->cost));
    memset(forge->unkC0, 0, sizeof(forge->unkC0));
    forge->count = 0;
    if (ArmorUpgrade_IsLocked(&gSave.box[slot]) == 0) {
        ArmorUpgrade_GetMaterials(&gSave.box[slot], need, cond);
        blocked = 0;
        for (i = 0; i < ARMOR_UP_MATS; i++) {
            if (need[i].item != 0) {
                if (cond[i] == 1 || cond[i] == 2) {
                    if (Save_IsItemObtained(&gSave, need[i].item) == 0) {
                        blocked = 1;
                    }
                }
                if (Forge_CheckCondition(cond[i]) == 0) {
                    blocked = 1;
                }
            }
        }
        if (blocked == 0) {
            forge->upgrades[forge->count] = gSave.box[slot];
            forge->upgrades[forge->count].used = 1;
            forge->upgrades[forge->count].level++;
            forge->cost[forge->count] = Armor_GetUpgradeCost(&forge->upgrades[forge->count]);
            memcpy(&forge->mats[forge->count], need, sizeof(forge->mats[0].need));
            forge->count++;
        }
    }
    unlockCount = ArmorUpgrade_GetDerived(&gSave.box[slot], unlocks);
    for (k = 0; k < unlockCount; k++) {
        memset(&candidate, 0, sizeof(candidate));
        candidate.type = gSave.box[slot].type;
        candidate.id = unlocks[k];
        ArmorUpgrade_GetMakeMaterials(&candidate, need, cond);
        blocked = 0;
        for (j = 0; j < ARMOR_UP_MATS; j++) {
            if (need[j].item != 0) {
                if (cond[j] == 1 || cond[j] == 2) {
                    if (Save_IsItemObtained(&gSave, need[j].item) == 0) {
                        blocked = 1;
                    }
                }
                if (Forge_CheckCondition(cond[j]) == 0) {
                    blocked = 1;
                }
                /* i, the first loop's counter, reused for the class bit */
                i = 1 << gPlayers[gScene.localPlayer].gender;
                if ((i & Equip_GetWearMask(&gSave, &candidate)) == 0) {
                    blocked = 1;
                }
            }
        }
        if (blocked == 0) {
            forge->upgrades[forge->count].used = 1;
            forge->upgrades[forge->count].type = gSave.box[slot].type;
            forge->upgrades[forge->count].id = unlocks[k];
            forge->cost[forge->count] = Armor_GetUpgradeCost(&forge->upgrades[forge->count]);
            memcpy(&forge->mats[forge->count], need, sizeof(forge->mats[0].need));
            if (Equip_TestFlag(&forge->upgrades[forge->count], 1) == 0) {
                Equip_SetFlag(&forge->upgrades[forge->count], 2);
                Equip_SetFlag(&forge->upgrades[forge->count], 1);
            }
            forge->count++;
        }
    }
    if (forge->count != 0) {
        forge->cursor = 0;
        forge->boxPiece = gSave.box[slot];
        forge->pieceType = gSave.box[slot].type;
        Equip_CopyEquipped(&forge->worn, gSave.box[slot].type);
        forge->result = forge->upgrades[forge->cursor];
    }
}

/*
 * The upgrade screen, by state: the windows slide in, the box
 * (ArmorForge_BoxInput), the list of upgrades of the chosen piece
 * (ArmorForge_UpgradeListInput), then the yes/no with the comparison of
 * the piece and the result (R1/L1 turn its pages). Returns MENU_DONE on
 * yes, MENU_CANCEL when the box is left, MENU_RUNNING otherwise.
 */
s32 ArmorForge_UpgradeInput(ArmorForgeWork *forge, u16 pad)
{
    switch (forge->state) {
    case FORGE_UPGRADE_SLIDE_IN:
        WinSlide_Step(&forge->upgradeWin, -296, 10);
        switch (WinSlide_Step(&forge->listWin, 640, 300)) {
        case MENU_DONE:
            forge->state++;
            break;
        }
        break;
    case FORGE_UPGRADE_BOX:
        switch (ArmorForge_BoxInput(forge, pad)) {
        case MENU_DONE:
            forge->state++;
            forge->detailPage = 0;
            forge->view = FORGE_UPGRADE_VIEW_STATS;
            if (forge->previewOff == 0) {
                EquipPreview_Hide(0);
                forge->detailPages = Equip_LevelMark(&forge->boxPiece, 16, &forge->result);
            }
            ArmorForge_UpgradeDetail(forge);
            break;
        case MENU_CANCEL:
            forge->state = FORGE_UPGRADE_SLIDE_IN;
            return MENU_CANCEL;
        }
        break;
    case FORGE_UPGRADE_LIST:
        switch (ArmorForge_UpgradeListInput(forge, pad)) {
        case MENU_DONE:
            forge->state++;
            forge->detailPage = 0;
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            forge->view = FORGE_UPGRADE_VIEW_COMPARE;
            ArmorForge_UpgradeDetail(forge);
            EquipPreview_Hide(1);
            break;
        case MENU_CANCEL:
            forge->state = FORGE_UPGRADE_BOX;
            forge->view = FORGE_BOX_VIEW_GRID;
            forge->detailPage = 0;
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
            switch (Menu_YesNoInput()) {
            case MENU_DONE:
                forge->state = FORGE_UPGRADE_SLIDE_IN;
                forge->detailPage = 0;
                WinSlide_Step(&forge->upgradeWin, -296, 10);
                WinSlide_Step(&forge->listWin, 640, 300);
                return MENU_DONE;
            case MENU_CANCEL:
                forge->state--;
                forge->previewOff = 1;
                EquipPreview_Hide(1);
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/*
 * Whether upgrade `index` of the list can be done now: the player can wear
 * the result, has the money and has every material. Returns 1 or 0.
 */
s32 ArmorForge_CanUpgrade(ArmorForgeWork *forge, s16 index)
{
    EquipItem unused;
    EquipItem *upgrade;
    s32 cost;
    s32 j;

    memset(&unused, 0, sizeof(unused));
    upgrade = &forge->upgrades[index];
    if (Equip_CanWear(&gSave, upgrade) == 0) {
        return 0;
    }
    cost = Armor_GetUpgradeCost(upgrade);
    if (gSave.money < cost) {
        return 0;
    }
    for (j = 0; j < ARMOR_UP_MATS; j++) {
        if (forge->mats[index].need[j].item != 0 && Items_HaveEnough(forge->mats[index].need[j].item, forge->mats[index].need[j].count) == 0) {
            return 0;
        }
    }
    return 1;
}

/* unit's own declaration: declared with a byte type and gender as the calls here pass them; with the definition's u16 kind, id and female the calls compile differently */
s32 Equip_GetModelId(u8 type, u16 id, u8 x);
void EquipPreview_Load(u8 type, s32 n, u8 x); /* unit's own declaration: declared with the types the calls here pass; with the definition's (u16 kind, u16 id, u8 female) the calls compile differently */
/*
 * The list of upgrades' input: circle chooses the entry when it can be
 * done (MENU_DONE; otherwise the refuse sound, and the can't-wear notice
 * when that is why), cross goes back (MENU_CANCEL). Square cycles the
 * detail panel through its four views, R1/L1 turn the detail pages, select
 * shows or hides the model preview (loading the chosen piece's model), up
 * and down move the cursor. Returns MENU_RUNNING otherwise.
 */
s32 ArmorForge_UpgradeListInput(ArmorForgeWork *forge, u16 pad)
{
    u16 oldCursor = forge->cursor;
    u8 female;

    if (pad & PAD_CIRCLE) {
        if (ArmorForge_CanUpgrade(forge, forge->cursor) == 1) {
            forge->detailPage = 0;
            LobbySe_Play(LOBBY_SE_CONFIRM);
            return MENU_DONE;
        }
        if (Equip_CanWear(&gSave, &forge->upgrades[forge->cursor]) == 0) {
            Menu_PushNoticeOnce(LOBBY_NOTICE_CANNOT_WEAR);
        }
        LobbySe_Play(LOBBY_SE_REFUSE);
    } else if (pad & PAD_CROSS) {
        LobbySe_Play(LOBBY_SE_CANCEL);
        return MENU_CANCEL;
    } else if (pad & PAD_SQUARE) {
        if (++forge->view >= FORGE_UPGRADE_VIEW_COUNT) {
            forge->view = FORGE_UPGRADE_VIEW_STATS;
        }
        forge->detailPage = 0;
        ArmorForge_UpgradeDetail(forge);
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
        if (forge->view == FORGE_UPGRADE_VIEW_STATS || forge->view == FORGE_UPGRADE_VIEW_MATERIALS) {
            if (forge->previewOff == 0) {
                forge->previewOff = 1;
                EquipPreview_Hide(1);
            } else {
                forge->previewOff = 0;
                if (forge->result.type != forge->shown.type || forge->result.id != forge->shown.id) {
                    forge->shown = forge->result;
                    if (Equip_IsNoModelArmor(&forge->shown) == 0) {
                        EquipPreview_Free();
                        female = gSave.gender;
                        EquipPreview_Load(forge->result.type, Equip_GetModelId(forge->result.type, forge->result.id, female), female);
                    }
                }
                EquipPreview_Hide(0);
            }
        }
    } else {
        forge->cursor = Menu_WrapUpDownSe(forge->cursor, forge->count, LOBBY_SE_CURSOR);
    }
    if (oldCursor != forge->cursor) {
        forge->result = forge->upgrades[forge->cursor];
        ArmorForge_UpgradeDetail(forge);
    }
    return MENU_RUNNING;
}

/*
 * Sets up the upgrade screen's detail view for the chosen upgrade: its
 * page count and, for the preview views, the chosen piece's model (loaded
 * when it changed, unless the piece has no model) and the preview shown
 * unless the player hid it.
 */
void ArmorForge_UpgradeDetail(ArmorForgeWork *forge)
{
    u8 female;

    switch (forge->view) {
    case FORGE_UPGRADE_VIEW_HUNTER:
        EquipPreview_Hide(1);
        forge->detailPage = 0;
        forge->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &forge->result);
        break;
    default:
        EquipPreview_Hide(1);
        forge->detailPages = Equip_LevelMark(&forge->result, 16, NULL);
        break;
    case FORGE_UPGRADE_VIEW_COMPARE:
        forge->detailPages = Equip_LevelMark(&forge->boxPiece, 16, &forge->result);
        EquipPreview_Hide(1);
        break;
    case FORGE_UPGRADE_VIEW_STATS:
    case FORGE_UPGRADE_VIEW_MATERIALS:
        if (forge->previewOff == 0) {
            if (forge->result.type != forge->shown.type || forge->result.id != forge->shown.id) {
                forge->shown = forge->result;
                if (Equip_IsNoModelArmor(&forge->shown) == 0) {
                    EquipPreview_Free();
                    female = gSave.gender;
                    EquipPreview_Load(forge->result.type, Equip_GetModelId(forge->result.type, forge->result.id, female), female);
                }
            }
            EquipPreview_Hide(0);
        }
        forge->detailPages = Equip_LevelMark(&forge->result, 0, NULL);
        break;
    }
}

/*
 * The same for the production screen's detail view of the recipe under the
 * cursor: the model preview, the piece's window, the materials (one page),
 * the comparison with the piece of that type being worn, or the skills.
 */
void ArmorForge_MakeDetail(ArmorForgeWork *forge)
{
    ForgeRecipe *recipe = &gArmorRecipes[forge->makeList[forge->cursor]];
    EquipItem piece;
    u16 *id;
    u8 *type;
    u8 female;

    memset(&piece, 0, sizeof(piece));
    type = &piece.type;
    id = &piece.id;
    *type = recipe->type;
    *id = recipe->id;
    switch (forge->view) {
    case FORGE_MAKE_VIEW_HUNTER:
        EquipPreview_Hide(1);
        forge->detailPage = 0;
        forge->detailPages = Equip_PreviewSkills(&gPlayers[gScene.localPlayer], &piece);
        break;
    default:
        EquipPreview_Hide(1);
        forge->detailPages = Equip_LevelMark(&forge->result, 16, NULL);
        break;
    case FORGE_MAKE_VIEW_COMPARE:
        forge->detailPages = Equip_LevelMark(&forge->worn, 80, &piece);
        EquipPreview_Hide(1);
        break;
    case FORGE_MAKE_VIEW_STATS:
    case FORGE_MAKE_VIEW_MATERIALS:
        if (forge->previewOff == 0) {
            if (*type != forge->shown.type || *id != forge->shown.id) {
                forge->shown = piece;
                if (Equip_IsNoModelArmor(&forge->shown) == 0) {
                    EquipPreview_Free();
                    female = gSave.gender;
                    EquipPreview_Load(*type, Equip_GetModelId(*type, *id, female), female);
                }
            }
            EquipPreview_Hide(0);
        }
        if (forge->view == FORGE_MAKE_VIEW_MATERIALS) {
            forge->detailPages = 1;
        } else {
            forge->detailPages = Equip_LevelMark(&piece, 0, NULL);
        }
        break;
    }
}

/*
 * Whether recipe line `line` of the production page can be made now: every
 * material at hand (Forge_CheckRecipe gives FORGE_RECIPE_READY), the money
 * for its price and a free box slot. Returns 1 or 0.
 */
s32 ArmorForge_CanMake(ArmorForgeWork *forge, s16 line)
{
    s32 recipeIdx = forge->makeList[line];
    ForgeRecipe *recipe = &gArmorRecipes[recipeIdx];
    s32 cost;

    if (Forge_CheckRecipe(FORGE_RECIPES_ARMOR, recipeIdx, FORGE_COUNT_ALL) != FORGE_RECIPE_READY) {
        return 0;
    }
    cost = Equip_HalfPrice(recipe->type, recipe->id);
    if (gSave.money < cost) {
        return 0;
    }
    if ((u16)Box_FindFreeSlot(&gSave) == BOX_NONE) {
        return 0;
    }
    return 1;
}

/*
 * Copies into out the worn piece of equipment of the given type (an
 * EquipType; both weapon types give the weapon) from gSave.equip, marked
 * used.
 */
void Equip_CopyEquipped(EquipItem *out, s16 type)
{
    out->type = type;
    switch (type) {
    case EQUIP_HEAD:
        *out = gSave.equip[SAVE_EQUIP_HEAD];
        break;
    case EQUIP_BODY:
        *out = gSave.equip[SAVE_EQUIP_BODY];
        break;
    case EQUIP_ARMS:
        *out = gSave.equip[SAVE_EQUIP_ARMS];
        break;
    case EQUIP_WAIST:
        *out = gSave.equip[SAVE_EQUIP_WAIST];
        break;
    case EQUIP_LEGS:
        *out = gSave.equip[SAVE_EQUIP_LEGS];
        break;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        *out = gSave.equip[SAVE_EQUIP_WEAPON];
        break;
    }
    out->used = 1;
}

extern s16 gEquipTabTypes[];
/*
 * The production screen, by state: the list of the five armour types
 * (circle opens its recipes, warning when the box is full; cross leaves),
 * the window sliding in, the recipe list (circle chooses a recipe that can
 * be made, cross closes the details or goes back to the types, square
 * cycles the five detail views, left/right turn the list's page, up/down
 * move the cursor, R1/L1 turn the detail pages, select shows or hides the
 * preview), then the yes/no. Returns MENU_DONE on yes, MENU_CANCEL when
 * the type list is left, MENU_RUNNING otherwise.
 */
s32 ArmorForge_MakeInput(ArmorForgeWork *forge, u16 pad)
{
    EquipItem piece;
    u16 *id;
    u8 *type;
    s32 line;
    u8 female;

    switch (forge->state) {
    case FORGE_MAKE_TAB:
        if (pad & PAD_CIRCLE) {
            forge->state++;
            forge->page = 0;
            forge->view = FORGE_MAKE_VIEW_LIST;
            forge->detailPage = 0;
            forge->cursor = 0;
            WinSlide_Step(&forge->listWin, 640, 280);
            ArmorForge_LoadMakePage(forge, 0);
            ArmorForge_MakeDetail(forge);
            Equip_CopyEquipped(&forge->worn, gEquipTabTypes[forge->row]);
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
            CamFocus_SetAside(1);
            EquipPreview_Reset();
            if ((u16)Box_FindFreeSlot(&gSave) == BOX_NONE) {
                Menu_PushNotice(LOBBY_NOTICE_BOX_FULL);
            }
        } else if (pad & PAD_CROSS) {
            forge->state = FORGE_MAKE_TAB;
            LobbySe_Play(LOBBY_SE_CANCEL);
            return MENU_CANCEL;
        } else {
            forge->row = Menu_WrapUpDownSe(forge->row, ARMOR_TAB_COUNT, LOBBY_SE_CURSOR);
        }
        break;
    case FORGE_MAKE_SLIDE_IN:
        switch (WinSlide_Step(&forge->listWin, 640, 280)) {
        case MENU_DONE:
            forge->state++;
            break;
        }
        break;
    case FORGE_MAKE_LIST:
        if (pad & PAD_CIRCLE) {
            if (forge->count != 0 && ArmorForge_CanMake(forge, forge->cursor) == 1) {
                forge->state++;
                forge->detailPage = 0;
                forge->view = FORGE_MAKE_VIEW_COMPARE;
                ArmorForge_MakeDetail(forge);
                Menu_SetYesNo(0);
                gMenuInputLock = 9;
                EquipPreview_Hide(1);
                LobbySe_Play(LOBBY_SE_CONFIRM);
            } else {
                LobbySe_Play(LOBBY_SE_REFUSE);
            }
        } else if (pad & PAD_CROSS) {
            if (forge->view != FORGE_MAKE_VIEW_LIST) {
                forge->view = FORGE_MAKE_VIEW_LIST;
                forge->detailPage = 0;
                ArmorForge_MakeDetail(forge);
            } else {
                forge->state = FORGE_MAKE_TAB;
                CamFocus_SetAside(0);
                EquipPreview_Hide(1);
            }
            LobbySe_Play(LOBBY_SE_WINDOW_SLIDE);
        } else if (pad & PAD_SQUARE) {
            if (forge->count != 0) {
                if (++forge->view == FORGE_MAKE_VIEW_COUNT) {
                    forge->view = FORGE_MAKE_VIEW_STATS;
                }
                forge->detailPage = 0;
                ArmorForge_MakeDetail(forge);
                LobbySe_Play(LOBBY_SE_DESCRIPTION);
            }
        } else if (pad & (PAD_LEFT | PAD_RIGHT)) {
            if (forge->pages > 1) {
                forge->page = Menu_WrapLeftRight(forge->page, forge->pages);
                ArmorForge_LoadMakePage(forge, forge->page);
                ArmorForge_MakeDetail(forge);
            }
        } else if (pad & (PAD_UP | PAD_DOWN)) {
            forge->cursor = Menu_WrapUpDownSe(forge->cursor, forge->count, LOBBY_SE_CURSOR);
            ArmorForge_MakeDetail(forge);
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
            if (forge->view == FORGE_MAKE_VIEW_STATS || forge->view == FORGE_MAKE_VIEW_MATERIALS) {
                line = forge->cursor;
                memset(&piece, 0, sizeof(piece));
                if (forge->previewOff == 0) {
                    forge->previewOff = 1;
                    EquipPreview_Hide(1);
                } else {
                    forge->previewOff = 0;
                    type = &piece.type;
                    id = &piece.id;
                    *type = gArmorRecipes[forge->makeList[line]].type;
                    *id = gArmorRecipes[forge->makeList[line]].id;
                    if (*type != forge->shown.type || *id != forge->shown.id) {
                        forge->shown = piece;
                        if (Equip_IsNoModelArmor(&forge->shown) == 0) {
                            EquipPreview_Free();
                            female = gSave.gender;
                            EquipPreview_Load(*type, Equip_GetModelId(*type, *id, female), female);
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
                forge->state = FORGE_MAKE_TAB;
                forge->detailPage = 0;
                WinSlide_Step(&forge->listWin, 640, 280);
                return MENU_DONE;
            case MENU_CANCEL:
                forge->state = FORGE_MAKE_LIST;
                forge->previewOff = 1;
                EquipPreview_Hide(1);
                break;
            }
        }
        break;
    }
    return MENU_RUNNING;
}

/* Called as the making pages open (ArmorForge_Open, ArmorForge_Input): the
 * game's function is empty. The weapon forge's twin sets its upgrade kind. */
void ArmorForge_OpenMake(ArmorForgeWork *forge)
{
}

/*
 * Whether recipe `recipeIdx` belongs on the production list of armour type
 * `tab`: its type is the tab's, the local player's gender can wear it and
 * it is unlocked (Forge_CheckRecipe). A listed piece seen for the first
 * time gets the made and new marks. Returns 1 or 0.
 */
s32 ArmorForge_IsListed(s16 tab, ForgeRecipe *recipe, s32 recipeIdx)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    EquipItem piece;

    memset(&piece, 0, sizeof(piece));
    if (recipe->type != gEquipTabTypes[tab]) {
        return 0;
    }
    piece.type = recipe->type;
    piece.id = recipe->id;
    if (!(Equip_GetWearMask(&gSave, &piece) & (1 << pl->gender))) {
        return 0;
    }
    if (Forge_CheckRecipe(FORGE_RECIPES_ARMOR, recipeIdx, FORGE_COUNT_ALL) == FORGE_RECIPE_LOCKED) {
        return 0;
    }
    if (Equip_TestFlag(&piece, EQUIP_FLAG_MADE) == 0) {
        Equip_SetFlag(&piece, EQUIP_FLAG_NEW);
        Equip_SetFlag(&piece, EQUIP_FLAG_MADE);
    }
    return 1;
}

/*
 * Fills the production list with page `page` (7 entries) of the recipes
 * listed for the current armour type and filter (only gForgeRecipeList's
 * recipes with the filter set, else the others), sets count and the page
 * count and keeps the cursor on the page.
 */
void ArmorForge_LoadMakePage(ArmorForgeWork *forge, s32 page)
{
    s32 last;
    ForgeRecipe *recipe;
    s32 filled;
    s32 listed;
    s32 first;
    s32 i;

    recipe = gArmorRecipes;
    first = page * FORGE_LIST_ROWS;
    filled = 0;
    last = first + FORGE_LIST_ROWS - 1;
    listed = 0;
    i = 0;
    if (recipe->type != FORGE_RECIPE_END) {
        do {
            if (ArmorForge_IsListed(forge->row, recipe, i) == 1) {
                if ((forge->filter == 0 && Recipe_IsInList(recipe->type, recipe->id) == 0)
                    || (forge->filter == 1 && Recipe_IsInList(recipe->type, recipe->id) == 1)) {
                    if (listed >= first && listed <= last) {
                        forge->makeList[filled] = i;
                        filled++;
                    }
                    listed++;
                }
            }
            recipe++;
            i++;
        } while (recipe->type != FORGE_RECIPE_END);
    }
    forge->count = filled;
    if (forge->cursor >= forge->count) {
        forge->cursor = forge->count - 1;
    }
    forge->pages = Menu_CountPages(listed, FORGE_LIST_ROWS);
}

extern u8 gLobbyTextDirty;
/*
 * Does the chosen upgrade: takes the materials and the cost, writes the
 * result over the box slot (a piece that becomes another starts at level 0
 * without decorations) and plays the money sound. When it is to be worn,
 * puts it on and rebuilds the local player (the equipped notice);
 * otherwise shows the forge-done notice. Then rebuilds the slot's list of
 * upgrades. --
 */
void ArmorForge_Upgrade(ArmorForgeWork *forge)
{
    s32 slot = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;
    EquipItem upgraded;
    s32 cost;
    s32 i;

    upgraded = gSave.box[slot];
    upgraded = forge->result;
    cost = forge->cost[forge->cursor];
    for (i = 0; i < ARMOR_UP_MATS; i++) {
        Items_Take(forge->mats[forge->cursor].need[i].item, forge->mats[forge->cursor].need[i].count);
    }
    if (gSave.box[slot].id != upgraded.id) {
        upgraded.level = 0;
        memset(upgraded.deco, 0, sizeof(upgraded.deco));
    }
    Box_SetSlot(&gSave, slot, &upgraded);
    Save_AddMoney(-cost);
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
    forge->cursor = 0;
    ArmorForge_AddUpgrade(forge, slot);
}

/*
 * The upgrade's confirmation, by step: a piece already worn is put on
 * again without asking, a piece the player can't wear is not asked about,
 * otherwise the equip-now yes/no (R1/L1 turn the comparison's pages); then
 * the upgrade is done and the upgrade screen comes back on its box.
 * Returns MENU_DONE once done, MENU_RUNNING before.
 */
s32 ArmorForge_UpgradeConfirm(ArmorForgeWork *forge, u16 pad)
{
    s32 slot = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;

    switch (forge->step) {
    case ARMOR_CONFIRM_SETUP:
        if (Equip_IsSlotEquipped(&gSave, slot) == 1) {
            forge->wear = 1;
            forge->step = ARMOR_CONFIRM_APPLY;
        } else if (Equip_CanWear(&gSave, &forge->result) == 0) {
            forge->wear = 0;
            forge->step = ARMOR_CONFIRM_APPLY;
        } else {
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            forge->step = ARMOR_CONFIRM_ASK;
        }
        forge->detailPages = Equip_LevelMark(&forge->worn, 16, &forge->result);
        break;
    case ARMOR_CONFIRM_ASK:
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
                forge->step = ARMOR_CONFIRM_APPLY;
                forge->wear = 1;
                break;
            case MENU_CANCEL:
                forge->step = ARMOR_CONFIRM_APPLY;
                forge->wear = 0;
                break;
            }
        }
        break;
    case ARMOR_CONFIRM_APPLY:
        forge->step = FORGE_STEP_UPGRADE;
        forge->state = FORGE_UPGRADE_SLIDE_IN;
        forge->view = FORGE_BOX_VIEW_GRID;
        ArmorForge_Upgrade(forge);
        forge->cursor = 0;
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/*
 * The forge's confirmation (the workshop's confirm state): runs the
 * production's or the upgrade's, by choice, and returns what it returns.
 */
s32 ArmorForge_Confirm(u8 *task, u16 pad)
{
    ArmorForgeWork *forge = (ArmorForgeWork *)(task + 12);

    if (forge->choice == FORGE_CHOICE_MAKE) {
        return ArmorForge_MakeConfirm(forge, pad);
    }
    return ArmorForge_UpgradeConfirm(forge, pad);
}

u16 Box_AddItem(SaveData *save, u8 type, u16 id, u16 level);
/*
 * Makes the recipe under the cursor: takes the materials and the price,
 * adds the piece to the equipment box and plays the money sound. When it
 * is to be worn, puts it on and rebuilds the local player (the equipped
 * notice); otherwise shows the forge-done notice.
 */
void ArmorForge_Make(ArmorForgeWork *forge)
{
    ForgeRecipe *recipe = &gArmorRecipes[forge->makeList[forge->cursor]];
    EquipItem piece;
    s32 price;
    u16 *id;
    u8 *type;
    s32 i;
    s32 slot;

    memset(&piece, 0, sizeof(piece));
    type = &piece.type;
    id = &piece.id;
    *type = recipe->type;
    *id = recipe->id;
    price = Equip_HalfPrice(*type, *id);
    for (i = 0; i < FORGE_RECIPE_MATS; i++) {
        Items_Take(recipe->mat[i].item, recipe->mat[i].count);
    }
    slot = Box_AddItem(&gSave, *type, *id, 0);
    Save_AddMoney(-price);
    LobbySe_Play(LOBBY_SE_MONEY);
    if (forge->wear != 0) {
        LobbySe_Play(LOBBY_SE_EQUIP);
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
 * The production's confirmation, by step: a piece the player can't wear is
 * not asked about, otherwise the equip-now yes/no (R1/L1 turn the
 * comparison's pages); then the piece is made, the worn piece of the type
 * is read again and the recipe list's window slides back in. Returns
 * MENU_DONE once made, MENU_RUNNING before.
 */
s32 ArmorForge_MakeConfirm(ArmorForgeWork *forge, u16 pad)
{
    ForgeRecipe *recipe = &gArmorRecipes[forge->makeList[forge->cursor]];
    EquipItem piece;

    memset(&piece, 0, sizeof(piece));
    piece.used = 1;
    piece.type = recipe->type;
    piece.id = recipe->id;
    switch (forge->step) {
    case ARMOR_CONFIRM_SETUP:
        if (Equip_CanWear(&gSave, &piece) == 0) {
            forge->wear = 0;
            forge->step = ARMOR_CONFIRM_APPLY;
        } else {
            Menu_SetYesNo(0);
            gMenuInputLock = 9;
            forge->step = ARMOR_CONFIRM_ASK;
        }
        forge->detailPages = Equip_LevelMark(&forge->worn, 16, &piece);
        break;
    case ARMOR_CONFIRM_ASK:
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
                forge->step = ARMOR_CONFIRM_APPLY;
                forge->wear = 1;
                break;
            case MENU_CANCEL:
                forge->step = ARMOR_CONFIRM_APPLY;
                forge->wear = 0;
                break;
            }
        }
        break;
    case ARMOR_CONFIRM_APPLY:
        forge->step = FORGE_STEP_MAKE;
        forge->state = FORGE_MAKE_SLIDE_IN;
        forge->view = FORGE_MAKE_VIEW_LIST;
        ArmorForge_Make(forge);
        Equip_CopyEquipped(&forge->worn, gEquipTabTypes[forge->row]);
        WinSlide_Step(&forge->listWin, 640, 280);
        return MENU_DONE;
    }
    return MENU_RUNNING;
}

/* The plain string format, as an array of its own: this unit's .rodata holds
 * a second copy of it after the town guide's literal, and a literal here would
 * reuse that first one. */
static const char sStringFormat[3] = {
#include "lobby/0082E950.inc"
};
void Lobby_DrawEquipIcon(s16 x, s16 y, s32 w, s32 h, EquipItem *e); /* unit's own declaration: declared with s16 coordinates as the forge's draws pass them; with the definition's s32 ones the calls compile differently */
char *Equip_GetName(u8 type, u16 id);
/*
 * Draws the production list in its wide form (no detail view): the page
 * number, each recipe's new mark (blinking) or icon, name and price,
 * greyed when it can't be made, the cursor bar, and under a dashed line
 * the chosen piece's summary line. With no recipe, the no-armour-to-make
 * text instead.
 */
void ArmorForge_DrawMakeListFull(ArmorForgeWork *forge, s16 x, s16 y)
{
    ForgeRecipe *recipe;
    s16 rowY = y;
    EquipItem piece;
    s16 i;
    s32 textX;
    EquipItem selPiece;
    s16 cursorShift;
    s32 winY;
    s32 winX;
    char *text;
    s32 msgY;
    s16 msgX;

    memset(&piece, 0, sizeof(piece));
    Icon_Draw(x, y, 340, 290, 0xFFFFDDDD, 0);
    winY = rowY;
    winX = x;
    msgY = rowY;
    Icon_Draw(winX + 110, winY + 45, 120, 200, 0x60FFFFFF, 36);
    Font_SetColor(0x80101010);
    if (forge->count == 0) {
        text = gNoArmorToMakeTexts[0];
        msgX = Text_CenterX(text, winX + 170, 18);
        Font_SetColor(0x8000052C);
        Font_SetSpacing(18, 18);
        Font_SetPos(msgX, msgY + 90);
        Text_PrintfColored(sStringFormat, text);
        Menu_DrawDashLine(winX + 20, winY + 186, 300, 0xFF010101);
        return;
    }
    Menu_DrawPageNum(winX + 250, winY + 14, forge->page, forge->pages, 1);
    rowY += 34;
    Font_SetSpacing(20, 20);
    i = 0;
    if (i < forge->count) {
        textX = winX + 40;
        do {
            recipe = &gArmorRecipes[forge->makeList[i]];
            piece.type = recipe->type;
            piece.id = recipe->id;
            if (Equip_TestFlag(&piece, EQUIP_FLAG_NEW) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw((s16)textX - 22, rowY, 16, 20, 0xFFC03000, 99);
            } else {
                Lobby_DrawEquipIcon((s16)textX - 28, rowY, 20, 20, &piece);
            }
            if (i == forge->cursor) {
                selPiece = piece;
                if (gFrameTick & 0x10) {
                    cursorShift = 4;
                } else {
                    cursorShift = 10;
                }
                Icon_Draw(textX, rowY + 14, 274, 8, 0x99106060, 111);
                Menu_DrawCursorIcon((s16)textX - 50 - cursorShift, rowY - 2, 24, 0xFF20C0C0);
            }
            if (ArmorForge_CanMake(forge, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(textX, rowY);
            Text_PrintfColored("%-20s %6d%s", Equip_GetName(recipe->type, recipe->id), Equip_HalfPrice(recipe->type, recipe->id), gLobbyWords[LOBBY_WORD_ZENNY]);
            rowY += 22;
            i++;
        } while (i < forge->count);
    }
    Menu_DrawDashLine(winX + 20, winY + 186, 300, 0xFF010101);
    EquipShop_DrawLine(winX + 16, winY + 201, &selPiece);
}

/*
 * Draws the production list in its narrow form beside a detail view: the
 * page number, each recipe's new mark or icon, name and price, greyed when
 * it can't be made, and the cursor bar.
 */
void ArmorForge_DrawMakeList(ArmorForgeWork *forge, s16 x, s16 y)
{
    ForgeRecipe *recipe;
    s16 rowY = y;
    EquipItem piece;
    s16 i;
    s32 textX;
    EquipItem selPiece;
    s16 cursorShift;
    s32 winY;
    s32 winX;

    memset(&piece, 0, sizeof(piece));
    memset(&selPiece, 0, sizeof(selPiece));
    Icon_Draw(x, y, 290, 180, 0xFFFFDDDD, 0);
    winY = rowY;
    winX = x;
    Icon_Draw(winX + 100, winY + 15, 90, 150, 0x60FFFFFF, 36);
    Font_SetColor(0x80101010);
    Menu_DrawPageNum(winX + 200, winY + 12, forge->page, forge->pages, 1);
    rowY += 34;
    Font_SetSpacing(17, 17);
    i = 0;
    if (i < forge->count) {
        textX = winX + 40;
        do {
            recipe = &gArmorRecipes[forge->makeList[i]];
            piece.type = recipe->type;
            piece.id = recipe->id;
            if (Equip_TestFlag(&piece, EQUIP_FLAG_NEW) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw((s16)textX - 20, rowY, 16, 20, 0xFFC03000, 99);
            } else {
                Lobby_DrawEquipIcon((s16)textX - 26, rowY, 20, 20, &piece);
            }
            if (i == forge->cursor) {
                selPiece = piece;
                if (gFrameTick & 0x10) {
                    cursorShift = 4;
                } else {
                    cursorShift = 10;
                }
                Icon_Draw((s16)textX - 8, rowY + 12, 248, 8, 0x99106060, 111);
                Menu_DrawCursorIcon((s16)textX - 42 - cursorShift, rowY - 2, 22, 0xFF20C0C0);
            }
            if (ArmorForge_CanMake(forge, i) == 0) {
                Font_SetColor(0x8042352E);
            } else {
                Font_SetColor(0x80101010);
            }
            Font_SetPos(textX, rowY);
            Text_PrintfColored("%-20s", Equip_GetName(recipe->type, recipe->id));
            Font_SetPos((s16)textX + 178, rowY);
            Text_PrintfColored("%6d%s", Equip_HalfPrice(recipe->type, recipe->id), gLobbyWords[LOBBY_WORD_ZENNY]);
            rowY += 20;
            i++;
        } while (i < forge->count);
    }
}

void Win_Draw(s16, s16, s16, s16, s32, s32, s32); /* unit's own declaration: declared with s16 coordinates as the forge's draws pass them; with the definition's s32 coordinates and s16 kind the calls compile differently */
s32 Items_DrawNeedLine(s16 x, s16 y, u16 item, s32 n, u32 color);
/*
 * Draws the window of the materials of the recipe under the cursor: the
 * materials heading and up to four material lines (Items_DrawNeedLine: an
 * item and the count needed).
 */
void ArmorForge_DrawMaterials(ArmorForgeWork *forge, s16 x, s16 y)
{
    ForgeRecipe *recipe = &gArmorRecipes[forge->makeList[forge->cursor]];
    s16 i;

    Win_Draw(x, y, 306, 114, 0xBDB59F6C, 0xA0202020, 6);
    y += 10;
    Font_SetSpacing(18, 18);
    Font_SetColor(0x807F7F28);
    Font_SetPos(x + 14, y);
    Text_PrintfColored(gForgeTexts[FORGE_TEXT_MATERIALS_HEAD]);
    for (i = 0; i < FORGE_RECIPE_MATS; i++) {
        y += 20;
        Items_DrawNeedLine(x + 34, y, recipe->mat[i].item, recipe->mat[i].count, 0x807F7F7F);
    }
}

extern LobbyMenu *gLobbyMenu;
/*
 * Draws the production screen: the armour type's title bar and the money,
 * then by state the list of armour types, the recipe list with the current
 * detail view (the model preview with its help or the no-model text, the
 * piece's window, the materials, the comparison with the worn piece, or
 * the skills) and its help text, or the comparison with the price question
 * and the yes/no.
 */
void ArmorForge_DrawMake(ArmorForgeWork *forge)
{
    s32 cursorIdx = forge->cursor;
    EquipItem piece;
    s16 x;
    s16 y;
    char *text;
    ForgeRecipe *recipe;

    memset(&piece, 0, sizeof(piece));
    if (forge->state != FORGE_MAKE_TAB) {
        Font_SetSpacing(22, 22);
        Menu_DrawLabelBar(10, 18, 160, 0xFFB0B000, gEquipGroupTexts[EQUIP_GROUP_TEXT_ARMOR + forge->row], 2, 1);
        Menu_DrawMoney(480, 16, 0);
        recipe = &gArmorRecipes[forge->makeList[cursorIdx]];
        piece.used = 1;
        y = 52;
        piece.type = recipe->type;
        piece.id = recipe->id;
        x = forge->listWin.pos;
    }
    switch (forge->state) {
    case FORGE_MAKE_TAB:
        Font_SetSpacing(20, 20);
        Menu_DrawLabelList(460, &gEquipGroupTexts[EQUIP_GROUP_TEXT_ARMOR], ARMOR_TAB_COUNT, forge->row, 0xFFB0B000);
        break;
    case FORGE_MAKE_SLIDE_IN:
    case FORGE_MAKE_LIST:
        switch (forge->view) {
        case FORGE_MAKE_VIEW_LIST:
            ArmorForge_DrawMakeListFull(forge, x + 12, y);
            break;
        case FORGE_MAKE_VIEW_STATS:
            if (forge->previewOff == 0) {
                EquipPreview_Hide(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, forge->previewOff);
            if (forge->previewOff == 0 && Equip_IsNoModelArmor(&forge->shown) == 1) {
                text = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
                EquipPreview_Hide(1);
                Font_SetSpacing(18, 18);
                Font_SetPos(Text_CenterX(text, 462, 18), 110);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(text);
            }
            ArmorForge_DrawMakeList(forge, 10, 50);
            EquipWindow_DrawLocal(&piece, 310, 216, forge->detailPage | 0x40, NULL);
            break;
        case FORGE_MAKE_VIEW_MATERIALS:
            if (forge->previewOff == 0) {
                EquipPreview_Hide(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, forge->previewOff);
            if (forge->previewOff == 0 && Equip_IsNoModelArmor(&forge->shown) == 1) {
                text = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
                EquipPreview_Hide(1);
                Font_SetSpacing(18, 18);
                Font_SetPos(Text_CenterX(text, 462, 18), 110);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(text);
            }
            ArmorForge_DrawMakeList(forge, 10, 50);
            ArmorForge_DrawMaterials(forge, 310, 216);
            break;
        case FORGE_MAKE_VIEW_COMPARE:
            ArmorForge_DrawMakeList(forge, 10, 50);
            EquipCompare_Draw(&forge->worn, &piece, 310, 54, forge->detailPage | 0x50);
            break;
        case FORGE_MAKE_VIEW_HUNTER:
            ArmorForge_DrawMakeList(forge, 10, 50);
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, 58, forge->detailPage);
            break;
        }
        if (forge->view != FORGE_MAKE_VIEW_HUNTER) {
            Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_PICK_ARMOR_TO_MAKE]);
        }
        break;
    case FORGE_MAKE_ASK:
        EquipCompare_Draw(&forge->worn, &piece, 310, 54, forge->detailPage | 0x50);
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020,
                      Text_FillDollar(gForgeTexts[FORGE_TEXT_MATERIALS_ASK], Equip_HalfPrice(piece.type, piece.id)));
        Font_SetPos(444, 400);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    }
}

void Forge_DrawMaterials(s16 x, s16 y, ItemStack *need, s8 n);
/*
 * Draws the upgrade screen's right side, by state: on the list of
 * upgrades, the current detail view of the chosen upgrade (preview and
 * window, preview and materials, comparison, skills) and the help text; on
 * the yes/no, the comparison, the cost question (worded for a level up or
 * for a change of piece) and the yes/no; otherwise the equipment box list
 * (with the piece's details when shown) and its help text.
 */
void ArmorForge_DrawUpgradeDetail(ArmorForgeWork *forge, s16 x, s16 y)
{
    s32 slot = forge->col + (forge->row * 10 + forge->page * 100);
    char *text;

    switch (forge->state) {
    case FORGE_UPGRADE_LIST:
        switch (forge->view) {
        case FORGE_UPGRADE_VIEW_STATS:
            if (forge->previewOff == 0) {
                EquipPreview_Hide(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, forge->previewOff);
            if (forge->previewOff == 0 && Equip_IsNoModelArmor(&forge->shown) == 1) {
                text = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
                EquipPreview_Hide(1);
                Font_SetSpacing(18, 18);
                Font_SetPos(Text_CenterX(text, 462, 18), 110);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(text);
            }
            EquipWindow_DrawLocal(&forge->result, 310, 224, forge->detailPage | 0x40, NULL);
            break;
        case FORGE_UPGRADE_VIEW_COMPARE:
            EquipCompare_Draw(&forge->boxPiece, &forge->result, 310, y + 4, forge->detailPage | 0x50);
            break;
        case FORGE_UPGRADE_VIEW_MATERIALS:
            if (forge->previewOff == 0) {
                EquipPreview_Hide(0);
            }
            Equip_DrawPreviewHelp(0xBDB59F6C, forge->previewOff);
            if (forge->previewOff == 0 && Equip_IsNoModelArmor(&forge->shown) == 1) {
                text = gForgeTexts[FORGE_TEXT_NO_SAMPLE];
                EquipPreview_Hide(1);
                Font_SetSpacing(18, 18);
                Font_SetPos(Text_CenterX(text, 462, 18), 110);
                Font_SetColor(0x807F212E);
                Text_PrintfColored(text);
            }
            Forge_DrawMaterials(310, 224, forge->mats[forge->cursor].need, ARMOR_UP_MATS);
            break;
        case FORGE_UPGRADE_VIEW_HUNTER:
            EquipDetail_Draw(&gPlayers[gScene.localPlayer], 310, 58, forge->detailPage);
            return;
        }
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_PICK_UPGRADE_KIND]);
        break;
    case FORGE_UPGRADE_ASK:
        EquipCompare_Draw(&forge->boxPiece, &forge->result, 310, y + 4, forge->detailPage | 0x50);
        if (forge->boxPiece.id != forge->result.id) {
            text = Text_FillDollar(gForgeTexts[FORGE_TEXT_MATERIALS_FEE_ASK], forge->cost[forge->cursor]);
        } else {
            text = Text_FillDollar(gForgeTexts[FORGE_TEXT_MATERIALS_ASK], forge->cost[forge->cursor]);
        }
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, text);
        Font_SetPos(444, 400);
        Text_Printf(gYesNoTexts[gLobbyMenu->yesNo]);
        break;
    default:
        if (forge->view == FORGE_BOX_VIEW_GRID) {
            BoxMenu_DrawItemList(slot, 0x2800010, x);
        } else {
            BoxMenu_DrawItemList(slot, (forge->detailPage & 7) | 0x2800078, x);
        }
        if (forge->state == FORGE_UPGRADE_BOX || forge->state == FORGE_UPGRADE_SLIDE_IN) {
            Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_PICK_ARMOR_UPGRADE]);
        } else {
            Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, gForgeTexts[FORGE_TEXT_PICK_UPGRADE_KIND]);
        }
        break;
    }
}

/*
 * Draws the list of upgrades of box piece `piece`: the heading, then for
 * an armour piece each upgrade (its name, or the level-up label when it
 * stays the same piece), its cost, greyed when it can't be done, the
 * blinking new mark and the cursor bar while the list is active. A weapon
 * or an empty slot draws only the heading; a piece without upgrades the
 * cannot-upgrade text.
 */
void ArmorForge_DrawUpgradeList(ArmorForgeWork *forge, EquipItem *piece, s16 x, s16 y)
{
    s16 winX = x;
    EquipItem upgrade;
    u32 cost;
    s32 i;
    s16 rowY = y;
    u16 *id;
    u8 *type;

    memset(&upgrade, 0, sizeof(upgrade));
    rowY += 10;
    Font_SetPos(winX + 10, rowY);
    Font_SetColor(0x807F7F28);
    Text_PrintfColored(gForgeTexts[FORGE_TEXT_ARMOR_UPGRADE_HEAD]);
    switch (piece->type) {
    case EQUIP_BLADE:
    case EQUIP_GUN:
        return;
    }
    if (piece->id == 0) {
        return;
    }
    if (forge->count == 0) {
        Font_SetColor(0x807F7F7F);
        Font_SetPos(Text_CenterX(gForgeTexts[FORGE_TEXT_CANNOT_UPGRADE], winX + 138, 17), y + 51);
        Text_PrintfColored(gForgeTexts[FORGE_TEXT_CANNOT_UPGRADE]);
        return;
    }
    i = 0;
    if (i < forge->count) {
        do {
            if (ArmorForge_CanUpgrade(forge, i) == 1) {
                Font_SetColor(0x807F7F7F);
            } else {
                Font_SetColor(0x8042352E);
            }
            upgrade = forge->upgrades[i];
            cost = forge->cost[i];
            Font_SetPos(winX + 14, rowY += 20);
            id = &upgrade.id;
            if (*id != forge->boxPiece.id) {
                type = &upgrade.type;
                Text_PrintfColored(Equip_GetName(*type, *id));
            } else {
                Text_PrintfColored(gForgeTexts[FORGE_TEXT_GUN_MODS]);
            }
            Font_SetPos(winX + 200, rowY);
            Text_PrintfColored("%7dz", cost);
            if (Equip_TestFlag(&upgrade, EQUIP_FLAG_NEW) == 1 && (gFrameTick & 0x10)) {
                Icon_Draw(x, rowY - 4, 16, 20, 0xFFC03000, 99);
            }
            if (forge->cursor == i && forge->state == FORGE_UPGRADE_LIST) {
                Menu_DrawGlowBar(winX + 138, rowY - 1, 276, 18, 0xB05050F0);
            }
            i++;
        } while (i < forge->count);
    }
}

/*
 * Draws the upgrade screen's left window: the title bar, the money, the
 * window (brighter while its list is active) and the list of upgrades of
 * the box piece under the cursor.
 */
void ArmorForge_DrawUpgradePanel(ArmorForgeWork *forge, s16 x, s16 y)
{
    u32 frameColor;
    u32 fillColor;
    s32 slot = forge->row * BOX_PAGE_COLS + forge->page * BOX_PAGE_SLOTS + forge->col;

    Font_SetSpacing(22, 22);
    Menu_DrawLabelBar(10, 18, 160, 0xFFB0B000, gArmorForgeTexts[ARMOR_FORGE_TITLE_UPGRADE], 2, 1);
    Menu_DrawMoney(480, 16, 0);
    switch (forge->state) {
    case FORGE_UPGRADE_LIST:
        frameColor = 0xBDB59F6C;
        fillColor = 0xA0202020;
        break;
    default:
        frameColor = 0xFF202020;
        fillColor = 0xA0000000;
        break;
    }
    Font_SetSpacing(17, 17);
    Win_Draw(x, y, 276, 174, frameColor, fillColor, 6);
    ArmorForge_DrawUpgradeList(forge, &gSave.box[slot], x, y);
}

/*
 * The forge's draw while it runs: by step, the production/upgrade choice,
 * the production screen, or the upgrade screen (its right side and its
 * left window at their slide positions).
 */
void ArmorForge_Draw(u8 *task)
{
    ArmorForgeWork *forge = (ArmorForgeWork *)(task + 12);

    switch (forge->step) {
    case FORGE_STEP_CHOOSE:
        Font_SetSpacing(20, 20);
        Menu_DrawLabelList(460, gArmorForgeTexts, 2, forge->choice, 0xFFB0B000);
        break;
    case FORGE_STEP_MAKE:
        ArmorForge_DrawMake(forge);
        break;
    case FORGE_STEP_UPGRADE:
        ArmorForge_DrawUpgradeDetail(forge, forge->listWin.pos + 12, 50);
        ArmorForge_DrawUpgradePanel(forge, forge->upgradeWin.pos + 6, 54);
        break;
    }
}

/*
 * The forge's draw during the confirmation: when the made or upgraded
 * piece can be worn, the comparison with the worn piece and the equip-now
 * question with its yes/no. Nothing on the confirmation's first step.
 */
void ArmorForge_DrawEquipPrompt(u8 *task)
{
    ArmorForgeWork *forge = (ArmorForgeWork *)(task + 12);
    EquipItem piece;
    char *text;
    ForgeRecipe *recipe;

    if (forge->step == ARMOR_CONFIRM_SETUP) {
        return;
    }
    memset(&piece, 0, sizeof(piece));
    if (forge->choice == FORGE_CHOICE_MAKE) {
        recipe = &gArmorRecipes[forge->makeList[forge->cursor]];
        piece.used = 1;
        piece.type = recipe->type;
        piece.id = recipe->id;
    } else {
        piece = forge->result;
    }
    if (Equip_CanWear(&gSave, &piece)) {
        switch (forge->step) {
        case ARMOR_CONFIRM_ASK:
            text = gForgeTexts[FORGE_TEXT_EQUIP_ASK];
            break;
        }
        EquipCompare_Draw(&forge->worn, &piece, 306, 56, forge->detailPage | 0x50);
        Win_DrawText(304, 352, 316, 80, 0xBDB59F6C, 0xA0202020, text);
        Font_SetPos(324, 400);
        Text_Printf(gEquipNowTexts[gLobbyMenu->yesNo]);
    }
}

/* The file's initialised data, in the game's order */
s16 gEquipTabTypes[8] = {
#include "lobby/007E8AA0.inc"
};
