/*
 * The in-game menus' common helpers: the HUD's textures, pausing, moving a
 * cursor through a list, a grid, the pouch's two columns, the phrase menu's
 * four columns and a yes/no choice; the left stick read as a D-pad; whether
 * an item can be used now; sorting the pouch; the menus' cursor bar,
 * icons, and equipment and rarity colours; the map and detect skill
 * queries the map draws with, and the map halves.
 *
 * The menus take the game's button word (pad.h's PAD_*: GameMenu_GetPad,
 * Pad_StickToDpad); PAD_L1 and PAD_R1 also step left and right where a
 * list has columns.
 *
 * The file has no data. Its helpers go by kind (the cursors, then the tests and the pouch's
 * sorting, then drawing helpers and colours, then the map's queries) rather than by menu: the
 * pouch's appear in four places, so a cut by subject would split them.
 */
#include "common.h"
#include "grid.h"
#include "item.h"
#include "pad.h"
#include "player.h"
#include "save.h"
#include "scene.h"
#include "sprite.h"
#include "supply_box.h"


#include "prim.h"
#include "hud.h"
#include "sound.h"
#include "area.h"
#include "texture.h"
#include "render_state.h"
#include "bgm.h"
#include "place_rules.h"
#include "camera.h"
#include "model.h"
#include "quest.h"
#include "stream.h"
#include "text.h"
#include "window.h"
#include "menu_util.h"
#include "engine_math.h"
#include "file_load.h"
#include "game_util.h"
#include "tex_files.h"
#include "cam_areas.h"
#include "stageobj.h"

/* An action spot (Player.spot): the start of gather.c's ActionSpot, which
 * no header shares yet. */
typedef struct Spot {
    u16 type;       /* 0x0 */
    u16 kind;       /* 0x2 */
} Spot;

s32 Item_IsStealable(u16 id); /* unit's own declaration: the definition (game) is s32 Item_IsStealable(s32 arg0); the calls here match only with this one */
s32 Item_RideItemUsable(void);
struct PrimLine;
u8 Player_GetSkill(Player *p, u8 group);
s16 Player_IsState(void *obj, u8 state, u8 sub);
struct SortPair;
void Sort_PairsDesc(struct SortPair *a, u32 n);

extern u16 gFrameTick;
extern IconUV gIconUVs[];
extern s16 gHelpLine;
extern u8 gHelpColumn;
extern u8 gChatInputMode;
extern u16 gChatWinTimer;
extern u8 gChatWinOpen;

/* Loads the HUD's textures: the system pack, then in a quest the stage's map
 * and the field HUD's sheet, in the town the town's five sheets, and the sheet
 * every mode uses. */
void Hud_LoadTextures(void)
{
    Tex_LoadPack(gSystemTexFiles[1], 324, 0);
    if (gScene.inLobby == 0) {
        Tex_LoadStageFile(gScene.map, gScene.questId, 325, 0);
        Tex_LoadFile(2, 326, 0);
    } else {
        Tex_LoadFile(9, 360, 0);
        Tex_LoadFile(10, 361, 0);
        Tex_LoadFile(11, 362, 0);
        Tex_LoadFile(12, 363, 0);
        Tex_LoadFile(13, 364, 0);
    }
    Tex_LoadFile(7, TEX_SLOT_PICTURES, 0);
}

/* Pauses the game and closes the chat window; with withStreams 1, the two
 * sound streams pause too. */
void Game_Pause(u8 withStreams)
{
    Scene_RequestPause(1);
    gChatWinOpen = gChatWinTimer = 0;
    if (withStreams == 1) {
        Stream_Pause(1, 1);
        Stream_Pause(0, 1);
    }
}

/* Ends Game_Pause; withStreams 1 resumes the sound streams too. */
void Game_Resume(u8 withStreams)
{
    Scene_RequestPause(0);
    if (withStreams == 1) {
        Stream_Pause(1, 0);
        Stream_Pause(0, 0);
    }
}

/* Moves the list cursor *cur by up and down, wrapping round count entries,
 * with the cursor sound. Returns 1 if it moved; a list of fewer than two
 * entries keeps it at 0. */
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count)
{
    u8 n;
    u8 old;

    if (count < 2) {
        *cur = 0;
        return 0;
    }
    old = *cur;
    n = old;
    if (buttons & PAD_UP) {
        if (old == 0) {
            n = count - 1;
        } else {
            n = old - 1;
        }
    } else if (buttons & PAD_DOWN) {
        n = old + 1;
        if (n >= count) {
            n = 0;
        }
    }
    if (n != *cur) {
        *cur = n;
        Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
        return 1;
    }
    return 0;
}

/* Menu_CursorUpDown for left and right (PAD_LEFT or PAD_L1 back, PAD_RIGHT or PAD_R1 forward). */
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count)
{
    u8 n;
    u8 old;

    if (count < 2) {
        *cur = 0;
        return 0;
    }
    old = *cur;
    n = old;
    if (buttons & (PAD_LEFT | PAD_L1)) {
        if (old == 0) {
            n = count - 1;
        } else {
            n = old - 1;
        }
    } else if (buttons & (PAD_RIGHT | PAD_R1)) {
        n = old + 1;
        if (n >= count) {
            n = 0;
        }
    }
    if (n != *cur) {
        *cur = n;
        Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
        return 1;
    }
    return 0;
}

/* Moves a grid's cursor: left and right through the columns, up and down
 * through the rows of the column (the last column has lastRows), wrapping
 * round, with the cursor sounds; then sets the entry index (rows per column
 * times the column, plus the row). */
void Menu_GridCursor(Grid *grid, u16 buttons)
{
    s16 col;
    s16 row;
    s16 rowCount;

    row = grid->row;
    col = grid->col;

    if ((buttons & (PAD_LEFT | PAD_RIGHT)) && grid->cols > 1) {
        if (buttons & PAD_LEFT) {
            col--;
            if (col < 0) {
                col = grid->cols - 1;
            }
        } else {
            col++;
            if (col >= grid->cols) {
                col = 0;
            }
        }
        if (col == grid->cols - 1 && row >= grid->lastRows) {
            row = grid->lastRows - 1;
            grid->row = row;
        }
        grid->col = col;
        Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    } else if (buttons & (PAD_UP | PAD_DOWN)) {
        if (col >= grid->cols - 1) {
            rowCount = grid->lastRows;
        } else {
            rowCount = grid->rows;
        }
        if (rowCount > 1) {
            if (buttons & PAD_UP) {
                row--;
                if (row < 0) {
                    row = rowCount - 1;
                }
            } else {
                row++;
                if (row >= rowCount) {
                    row = 0;
                }
            }
            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
        }
        grid->row = row;
    }
    grid->index = row + grid->rows * col;
}

/* Moves the pouch menu's cursor (Pouch_MoveCursor), sets the help line to
 * the item's description and lets PAD_R1 page a decoration's help; returns
 * the item under the cursor. */
u16 PouchMenu_MoveCursor(u8 *cur, u16 buttons)
{
    u16 item;

    Pouch_MoveCursor(cur, buttons);
    item = gPlayers[gScene.localPlayer].pouch[*cur].item;
    gHelpLine = item + 24;
    PouchMenu_DecoCursor(item, buttons & PAD_R1);
    return item;
}

/* Moves a cursor through the pouch's 20 slots, two columns of 10: up and
 * down within the column, left and right to the other column. */
void Pouch_MoveCursor(u8 *cur, u16 buttons)
{
    if (*cur < POUCH_MENU_ROWS) {
        Menu_CursorUpDown(cur, buttons, POUCH_MENU_ROWS);
    } else {
        *cur -= POUCH_MENU_ROWS;
        Menu_CursorUpDown(cur, buttons, POUCH_MENU_ROWS);
        *cur += POUCH_MENU_ROWS;
    }
    if (buttons & (PAD_LEFT | PAD_RIGHT)) {
        if (buttons & PAD_LEFT) {
            if (*cur < POUCH_MENU_ROWS) {
                *cur += POUCH_MENU_ROWS * 2;
            }
            *cur -= POUCH_MENU_ROWS;
        } else {
            *cur += POUCH_MENU_ROWS;
            if (*cur >= POUCH_MENU_ROWS * 2) {
                *cur -= POUCH_MENU_ROWS * 2;
            }
        }
        Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    }
}

/* Moves the phrase menu's cursor through its 24 entries, four columns of
 * 6, as Pouch_MoveCursor does; returns the new entry. */
u8 PhraseMenu_MoveCursor(u16 buttons, u8 cur)
{
    if (cur < PHRASE_MENU_ROWS) {
        Menu_CursorUpDown(&cur, buttons, PHRASE_MENU_ROWS);
    } else if (cur < PHRASE_MENU_ROWS * 2) {
        cur -= PHRASE_MENU_ROWS;
        Menu_CursorUpDown(&cur, buttons, PHRASE_MENU_ROWS);
        cur += PHRASE_MENU_ROWS;
    } else if (cur < PHRASE_MENU_ROWS * 3) {
        cur -= PHRASE_MENU_ROWS * 2;
        Menu_CursorUpDown(&cur, buttons, PHRASE_MENU_ROWS);
        cur += PHRASE_MENU_ROWS * 2;
    } else {
        cur -= PHRASE_MENU_ROWS * 3;
        Menu_CursorUpDown(&cur, buttons, PHRASE_MENU_ROWS);
        cur += PHRASE_MENU_ROWS * 3;
    }
    if (buttons & (PAD_LEFT | PAD_RIGHT)) {
        if (buttons & PAD_LEFT) {
            if (cur < PHRASE_MENU_ROWS) {
                cur += PHRASE_MENU_ROWS * PHRASE_MENU_COLUMNS;
            }
            cur -= PHRASE_MENU_ROWS;
        } else {
            cur += PHRASE_MENU_ROWS;
            if (cur >= PHRASE_MENU_ROWS * PHRASE_MENU_COLUMNS) {
                cur -= PHRASE_MENU_ROWS * PHRASE_MENU_COLUMNS;
            }
        }
        Sound_PlaySeFlat(0, SE_SYS_OPEN, 0);
    }
    return cur;
}

/* For a decoration, moves the help's column (gHelpColumn, 3 of them) with
 * the buttons given. */
void PouchMenu_DecoCursor(u16 item, u16 buttons)
{
    if (gItemData[item].deco != 0 && item != 0) {
        Menu_CursorLeftRight(&gHelpColumn, buttons & (PAD_R1 | PAD_L1), 3);
    }
}

/* Moves a yes/no choice (0 yes, 1 no) by the buttons in mask: down or
 * right to no, up or left to yes, with the cursor sound. Returns the
 * choice. */
u8 YesNo_Toggle(u16 buttons, u8 choice, u16 mask)
{
    buttons = buttons & mask;
    if (choice == 0) {
        if (buttons & (PAD_DOWN | PAD_RIGHT)) {
            choice = 1;
            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
        }
    } else {
        if (buttons & (PAD_UP | PAD_LEFT)) {
            choice = 0;
            Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
        }
    }
    return choice;
}

/* The stick of pad as D-pad buttons: none inside the dead zone (64),
 * else one of eight directions by its angle (a 16-bit turn, turned by
 * 0x1200 so that each sector starts at 0): right, up-right, up, up-left,
 * left, down-left, down, down-right. The four straight sectors are wider
 * (0x2400) than the diagonal ones (0x1C00). */
u16 Pad_StickToDpad(u8 pad)
{
    u16 angle;

    if (gPadState[pad].magR <= 64) {
        return 0;
    }
    angle = gPadState[pad].degR + 0x1200;
    if (angle < 0x2400) {
        return PAD_RIGHT;
    }
    if (angle < (ANGLE_TURN / 4)) {
        return PAD_UP | PAD_RIGHT;
    }
    if (angle < 0x6400) {
        return PAD_UP;
    }
    if (angle < (ANGLE_TURN / 2)) {
        return PAD_UP | PAD_LEFT;
    }
    if (angle < 0xA400) {
        return PAD_LEFT;
    }
    if (angle < (ANGLE_TURN * 3 / 4)) {
        return PAD_DOWN | PAD_LEFT;
    }
    if (angle < 0xE400) {
        return PAD_DOWN;
    }
    return PAD_DOWN | PAD_RIGHT;
}

s32 Chat_InInputMode(void)
{
    return gChatInputMode != 0;
}

/* Whether pouch slot slot of pl holds an item that can be used (an item
 * with flag 1 in gItemData). */
s32 Pouch_IsSlotUsable(Player *pl, u16 slot)
{
    if (pl->pouch[slot].count > 0) {
        s16 item = pl->pouch[slot].item;
        if (item != 0 && (gItemData[item].flags & 1)) {
            return 1;
        }
    }
    return 0;
}

/*
 * Whether the local player can use item id now (the pouch menu). Each case
 * is a group of item ids that share a rule: some need a place (an area's
 * ambient model, an action spot of a kind, a gathering point of a kind,
 * outside the camp), some a free placement (Item_CanPlace*), some are not
 * for gunners, some only offline-style quests (not versus). While the
 * player rides a monster only ITEM_ID_MONSTER_BOMB can be used. Items with icon 4 need
 * an action spot of kind 2 unless player flag 0x80000 is set. Returns 1 if
 * usable.
 */
s32 Item_IsUsable(u16 id)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 pointPos[12];
    s16 pointKind;
    u8 ride;

    if (Player_Status6Active(pl) == 1 && Item_IsStealable(id) == 1) {
        return 0;
    }
    ride = pl->rideState;
    if (ride == 1) {
        if (id == ITEM_ID_MONSTER_BOMB) {
            return Item_RideItemUsable();
        }
        return 0;
    }
    switch (id) {
    case 84:
    case 665:
        if (Player_GetAreaPitModel(pl) < 0) {
            return 0;
        }
        break;
    case 678:
        if (pl->spot == NULL) {
            return 0;
        }
        if (((Spot *)pl->spot)->kind != 11) {
            return 0;
        }
        break;
    case 85:
    case 662:
        return Item_CanPlaceKind2();
    case 86:
        return Scene_InNoBarrelBombArea();
    case 156:
    case 660:
    case 157:
    case 158:
    case 159:
    case 160:
        return Pouch_HasMeat(pl);
    case 162:
    case 759:
        return 0;
    case 760:
    case 761:
    case 778:
    case 779:
    case 781:
    case 762:
    case 763:
    case 764:
    case 767:
    case 768:
    case 769:
    case 774:
    case 775:
    case 776:
    case 777:
    case 788:
    case 792:
    case 783:
    case 784:
    case 785:
    case 786:
    case 789:
    case 790:
    case 791:
    case 780:
    case 793:
    case 787:
        return SpecialMine_IsFree(pl);
    case 172:
    case 661:
    case 234:
    case 242:
        if (pl->isGunner == 1) {
            return 0;
        }
        break;
    case 91:
    case 92:
    case 95:
    case 96:
        return Item_CanUseSmallBarrelBomb();
    case 93:
    case 94:
    case 666:
        return Item_CanPlaceLargeBarrelBomb();
    case 98:
        return Item_CanPlaceKind1();
    case 97:
        if (ride != 1) {
            return 0;
        }
        if (Item_RideItemUsable() != 1) {
            return 0;
        }
        break;
    case 40:
    case 44:
    case 46:
    case 45:
        return Scene_CanPlaceKind3();
    case 150:
    case 151:
    case 152:
        if (Player_GetGatherPoint(pl, (u16 *) &pointKind, (f32 *) pointPos) == 0xFFFF) {
            return 0;
        }
        if (pointKind != 3) {
            return 0;
        }
        break;
    case 153:
    case 154:
    case 155:
        if (Player_GetGatherPoint(pl, (u16 *) &pointKind, (f32 *) pointPos) == 0xFFFF) {
            return 0;
        }
        if (pointKind != 4) {
            return 0;
        }
        break;
    case 667:
        if (pl->spot == NULL) {
            return 0;
        }
        if (((Spot *)pl->spot)->kind != 6) {
            return 0;
        }
        break;
    case 695:
        if (pl->spot == NULL) {
            return 0;
        }
        if (((Spot *)pl->spot)->kind != 10) {
            return 0;
        }
        break;
    case 78:
        return Item_UsableOutsideCamp();
    case 173:
        return Player_LocalCanUseBinoculars();
    case 69:
    case 80:
        return Quest_IsVersus() == 0;
    default:
        if (gItemData[id].icon == 4 && Player_TestFlag(pl, 0x80000) == 0) {
            if (pl->spot == NULL) {
                return 0;
            }
            if (((Spot *)pl->spot)->kind != 2) {
                return 0;
            }
        }
        break;
    }
    return 1;
}

s32 Player_IsFree(Player *pl)
{
    return pl->freeState != 0;
}

/* Whether a monster rate has been earned: event flag 0xB6 is set and one
 * of the twelve rate flags (SaveData.rateFlags). */
s32 Save_HasRate(void)
{
    SaveData *save = &gSave;

    if (Save_TestEventFlag(182) != 1) {
        return 0;
    }
    if ((save->rateFlags & 0xFFF) != 0) {
        return 1;
    }
    return 0;
}

/* Whether the status page shows the symbol's line: once a rate or a style
 * has been earned. */
s32 StatusPage_HasSymbolColor(void)
{
    SaveData *save = &gSave;

    if (Save_HasRate() != 1 && save->styleBits == 0) {
        return 0;
    }
    return 1;
}

/* Puts the local player's pouch cursor on an item: the slot it is on if
 * that holds one, else the next slot that does (wrapping round). Returns
 * the slot, or -1 for an empty pouch. */
s16 Pouch_FixCursor(void)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    s32 n;
    u32 i = pl->pouchCursor;

    if (pl->pouch[i].item != 0) {
        return i;
    }
    n = 20;
    do {
        if (pl->pouch[i].item != 0) {
            pl->pouchCursor = i;
            return i;
        }
        i++;
        if (i >= 20) {
            i = 0;
        }
    } while (--n != 0);
    return -1;
}

/*
 * Sorts a pouch: the local player's (box 0) or the saved one (gSave.pouch,
 * box 1). The items move up over the empty slots, then Sort_PairsDesc
 * orders them. In the field, the pouch cursor, the loaded ammo and the
 * coating follow their items to their new slots (the loaded rounds are
 * kept); outside it, the saved pouch is copied back to the player
 * (Save_RestorePouch).
 *
 * The inner search loop counts with i (i = left - 1; n = i), not with
 * n = left - 1: that is the form whose registers match.
 */
void Pouch_Compact(u8 box)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    ItemStack *items;
    ItemStack *slot;
    ItemStack *next;
    ItemStack cur;
    ItemStack ammo;
    ItemStack coat;
    s32 left;
    s32 n;
    s32 count;
    u32 i;
    u8 loaded;
    u8 coatSlot;

    if (box == 0) {
        items = pl->pouch;
    } else {
        items = gSave.pouch;
    }
    if (Scene_InField() == 1) {
        if (Pouch_FixCursor() < 0) {
            return;
        }
        pl->ammoSlot = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 2);
        loaded = pl->roundsLoaded;
        coatSlot = pl->coatingSlot;
        Bowgun_LoadAmmo(pl, 0);
        pl->roundsLoaded = loaded;
        pl->coatingSlot = coatSlot;
        cur = pl->pouch[pl->pouchCursor];
        if (pl->ammoSlot != 255) {
            ammo = pl->pouch[pl->ammoSlot];
        }
        if (pl->coatingSlot != 255) {
            coat = pl->pouch[pl->coatingSlot];
        }
    }
    n = 20;
    slot = items;
    count = 0;
    for (left = n; left != 0 && n != 0; left--, slot++) {
        if (slot->item != 0) {
            count++;
        } else {
            i = left - 1;
            for (n = i, next = slot + 1; n != 0; n--, next++) {
                if (next->item != 0) {
                    count++;
                    *slot = *next;
                    next->count = 0;
                    next->item = 0;
                    break;
                }
            }
        }
    }
    Sort_PairsDesc((struct SortPair *)items, count);
    if (Scene_InField() == 1) {
        if (box == 0) {
            for (slot = pl->pouch, i = 0; i < 20; i++, slot++) {
                if (slot->item == cur.item) {
                    pl->pouchCursor = i;
                    break;
                }
            }
            if (pl->ammoSlot != 255) {
                for (slot = pl->pouch, i = 0; i < 20; i++, slot++) {
                    if (slot->item == ammo.item) {
                        pl->ammoSlot = i;
                        break;
                    }
                }
            }
            if (pl->coatingSlot != 255) {
                for (slot = pl->pouch, i = 0; i < 20; i++, slot++) {
                    if (slot->item == coat.item) {
                        pl->coatingSlot = i;
                        break;
                    }
                }
            }
        }
    } else if (box != 0) {
        Save_RestorePouch(pl);
    }
}

/* Whether pl looks through a scope (the HUD's aim view): the bowgun's
 * attack input runs with rounds loaded, or the player is in state PL_ACT_MOUNTED_GUN_AIM or
 * PL_ACT_MOUNTED_GUN_REAIM with ITEM_ID_MOUNTED_GUN_AMMO in the pouch. */
s32 Player_IsAiming(Player *pl)
{
    if (pl->bowgunReady != 0) {
        return 1;
    }
    if ((Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM) != 0) && Pouch_Count(pl, ITEM_ID_MOUNTED_GUN_AMMO) != 0) {
        return 1;
    }
    return 0;
}

/* Draws a list's cursor bar over entry index (rows step pixels apart from
 * top), from x left to x right, in colour rgb with its alpha pulsing
 * (191 +- 48, a 64-frame period). */
void ListPanel_DrawCursorBar(s32 left, s32 right, s16 top, s16 step, s32 index, s32 rgb)
{
    PrimLine bar;
    u16 phase;

    bar.x1 = right;
    bar.x0 = left;
    bar.y0 = top - 2 + step * index;
    bar.y1 = bar.y0 + step + 2;
    phase = (gFrameTick & 0x3F) << 10;
    bar.color = (rgb & 0xFFFFFF) | (((s8)(48.0f * Math_Sin((6.2831855f / 65536.0f) * phase)) + 191) << 24);
    Prim_FillRect(&bar);
}

/* Draws count icons from the HUD's icon sheet: each a square of its size
 * at its place, with its alpha. */
void Draw_Icons(Icon *icons, u8 count)
{
    TexRect spr;

    Gfx_SetTexFilter(GFX_TEX_BILINEAR);
    Gfx_SetBlend(GFX_BLEND_SRC_ALPHA, GFX_BLEND_INV_SRC_ALPHA);
    Tex_Prepare(1, TEX_SLOT_ICONS);
    Tex_Bind(TEX_SLOT_ICONS);
    for (; count > 0; count--) {
        spr.x = icons->x;
        spr.y = icons->y;
        spr.w = icons->size;
        spr.h = icons->size;
        spr.uv0 = gIconUVs[icons->kind].uv0;
        spr.uv1 = gIconUVs[icons->kind].uv1;
        spr.color = (icons->alpha << 24) | 0xFFFFFF;
        Sprite_Draw((struct Sprite *)&spr);
        icons++;
    }
}

/* The colour of an equipment of rarity rarity (0..6), with alpha: the
 * first set of gRarityColorSlots, or with alt the second. */
u32 Equip_Color(u8 rarity, u8 alpha, s32 alt)
{
    if ((u8)alt) {
        rarity += 7;
    }
    return (alpha << 24) | gEquipColors[gRarityColorSlots[rarity]];
}

/* The text colour of rarity rarity (gRarityColors). */
s16 Rarity_Color(u8 rarity)
{
    return gRarityColors[rarity];
}

/* Whether pl's map shows: by the map skill (skill group 35) when the
 * player has one of its values, which shows it only at 145; else when
 * item 657 is in the pouch. */
s32 Player_HasMap(Player *pl)
{
    u8 skill = Player_GetSkill(pl, 35);

    if (skill != 0) {
        if (skill == 145) {
            return 1;
        }
        return 0;
    }
    return Pouch_Count(pl, 657) != 0;
}

/* Which monsters pl's map shows (DETECT_* bits): the detect skill (skill
 * group 27; 162 is its higher level), and DETECT_TIMER while a detect
 * effect runs. */
u8 Player_DetectLevel(Player *pl)
{
    u8 level = 0;
    u8 skill = Player_GetSkill(pl, 27);

    if (skill != 0) {
        if (skill == 162) {
            level = DETECT_SKILL_HIGH;
        } else {
            level = DETECT_SKILL;
        }
    }
    if (((u16)pl->detectTimer | (u16)pl->actDetectTimer) != 0) {
        level |= DETECT_TIMER;
    }
    return level;
}

/* The map half (MAP_HALF_*) of the area pl is in. */
s32 Player_GetMapHalf(Player *pl)
{
    return Map_GetHalf(gScene.map, pl->area);
}

/* The map half (MAP_HALF_*) area is drawn on: on stages 13 to 15, areas
 * AREA_TOWER_0 to AREA_TOWER_2 are on the left half and the others on the right; every
 * other stage has a whole map. */
s32 Map_GetHalf(u8 stage, u8 area)
{
    switch (stage) {
    case 13:
    case 14:
    case 15:
        switch (area) {
        case AREA_TOWER_0:
        case AREA_TOWER_1:
        case AREA_TOWER_2:
            return MAP_HALF_LEFT;
        default:
            return MAP_HALF_RIGHT;
        }
    default:
        return MAP_HALF_WHOLE;
    }
}

/* Draws digit (0..9) as its full-width character (the Shift-JIS character 0x824F + digit). */
void Text_DrawDigit(u8 digit)
{
    char buf[3];

    buf[0] = 130;
    buf[1] = digit + 79;
    buf[2] = 0;
    Text_Draw(buf);
}
