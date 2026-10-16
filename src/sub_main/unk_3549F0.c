/*
 * The in-game menus' common helpers (the menus themselves are in
 * unk_33E620.c and unk_356380.c): the HUD's textures, pausing, moving a
 * cursor through a list, a grid, the pouch's two columns, the phrase menu's
 * four columns and a yes/no choice; the left stick read as a D-pad; whether
 * an item can be used now; sorting the pouch; the menus' cursor bar,
 * icons, and equipment and rarity colours; the map and detect skill
 * queries the map draws with, and the map halves.
 *
 * The menus' button word (GameMenu_GetPad, Pad_StickToDpad): 0x2000 up,
 * 0x1000 down, 0x0800 left, 0x0400 right; 0x0008 and 0x0080 also step left
 * and right where a list has columns.
 */
#include "common.h"
#include "grid.h"
#include "item.h"
#include "pad.h"
#include "player.h"
#include "save.h"
#include "scene.h"
#include "sprite.h"


#include "prim.h"
#include "hud.h"

/* An action spot (Player.spot): the start of gather.c's ActionSpot, which
 * no header shares yet. */
typedef struct Spot {
    u16 type;       /* 0x0 */
    u16 kind;       /* 0x2 */
} Spot;

s32 Tex_LoadPack(s32 id, s32 slot, s32 kind);
s32 func_723BC0(Player *p);
s32 Item_IsStealable(u16 id); /* unit's own declaration: the definition (game) is s32 Item_IsStealable(s32 arg0); the calls here match only with this one */
s32 Item_RideItemUsable(void);
s32 Item_CanPlaceLargeBarrelBomb(void);
s32 Item_CanUseSmallBarrelBomb(void);
s32 Item_CanPlaceKind2(void);
s32 Scene_InNoBarrelBombArea(void);
s32 Item_CanPlaceKind1(void);
s32 Scene_CanPlaceKind3(void);
s32 Player_LocalAreaOk_Unk(void);
s32 Item_UsableOutsideCamp(void);
s32 Quest_IsVersus(void);
s16 Player_GetAreaAmbModel(Player *p);
s32 Pouch_HasItem28(Player *p);
u16 Player_GetGatherPoint(Player *p, u16 *id, f32 *pos);
s32 SpecialMine_IsFree(Player *unused0);
u32 Player_TestFlag(void *obj, u32 flag);
f32 Math_Sin(f32 x);
struct PrimLine;
s32 Prim_FillRect(struct PrimLine *r);
void Gfx_SetState63(s32 a);
void Gfx_SetBlend(s32 a, s32 b);
void Tex_Prepare(s32 a, s32 b);
void Tex_Bind(s32 a);
s32 Sprite_Draw(struct Sprite *spr);
void Tex_LoadFile(s32 a, s32 b, s32 c);
void Tex_LoadStageFile(s32 file, s32 unused, s32 slot, s32 kind);
s32 Scene_RequestPause(s32 mode);
void Stream_Pause(s32 a, s32 b);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
s32 Save_TestEventFlag(s32 n);
u8 Player_GetSkill(Player *p, u8 group);
s16 Player_IsState(void *obj, u8 state, u8 sub);
s16 Pouch_Count(Player *p, u16 item);
void Text_Draw(char *s);
s32 Menu_CursorUpDown(u8 *cur, u16 buttons, u8 count);
s32 Menu_CursorLeftRight(u8 *cur, u16 buttons, u8 count);
void Pouch_MoveCursor(u8 *cur, u16 buttons);
void PouchMenu_DecoCursor(u16 item, u16 buttons);
s32 Save_HasRate(void);
s16 Pouch_FixCursor(void);
s32 Scene_InField(void);
struct SortPair;
void Sort_PairsDesc(struct SortPair *a, u32 n);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
void Bowgun_LoadAmmo(Player *p, s32 a);
void Save_RestorePouch(Player *p);
s32 Map_GetHalf(u8 stage, u8 area);

extern s32 gSystemTexFiles[];
extern u16 gFrameTick;
extern IconUV gIconUVs[];
extern u32 gEquipColors[];
extern u8 gRarityColorSlots[];
extern u8 gRarityColors[];
extern s16 gHelpLine;
extern u8 gHelpColumn;
extern u8 gChatInputMode;
extern u16 gChatWinTimer;
extern u8 gChatWinOpen;

/* Loads the HUD's textures: the system pack, then offline the stage's map
 * and the field HUD's sheet, online the town's five sheets, and the sheet
 * every mode uses. */
void Hud_LoadTextures(void)
{
    Tex_LoadPack(gSystemTexFiles[1], 0x144, 0);
    if (gScene.online == 0) {
        Tex_LoadStageFile(gScene.map, gScene.questId, 0x145, 0);
        Tex_LoadFile(2, 0x146, 0);
    } else {
        Tex_LoadFile(9, 0x168, 0);
        Tex_LoadFile(10, 0x169, 0);
        Tex_LoadFile(11, 0x16A, 0);
        Tex_LoadFile(12, 0x16B, 0);
        Tex_LoadFile(13, 0x16C, 0);
    }
    Tex_LoadFile(7, 0x147, 0);
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
    if (buttons & 0x2000) {
        if (old == 0) {
            n = count - 1;
        } else {
            n = old - 1;
        }
    } else if (buttons & 0x1000) {
        n = old + 1;
        if (n >= count) {
            n = 0;
        }
    }
    if (n != *cur) {
        *cur = n;
        Sound_PlaySeFlat(0, 0x17, 0);
        return 1;
    }
    return 0;
}

/* Menu_CursorUpDown for left and right (0x0808 back, 0x0480 forward). */
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
    if (buttons & 0x808) {
        if (old == 0) {
            n = count - 1;
        } else {
            n = old - 1;
        }
    } else if (buttons & 0x480) {
        n = old + 1;
        if (n >= count) {
            n = 0;
        }
    }
    if (n != *cur) {
        *cur = n;
        Sound_PlaySeFlat(0, 0x17, 0);
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

    if ((buttons & 0xC00) && grid->cols > 1) {
        if (buttons & 0x800) {
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
        Sound_PlaySeFlat(0, 0x11, 0);
    } else if (buttons & 0x3000) {
        if (col >= grid->cols - 1) {
            rowCount = grid->lastRows;
        } else {
            rowCount = grid->rows;
        }
        if (rowCount > 1) {
            if (buttons & 0x2000) {
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
            Sound_PlaySeFlat(0, 0x17, 0);
        }
        grid->row = row;
    }
    grid->index = row + grid->rows * col;
}

/* Moves the pouch menu's cursor (Pouch_MoveCursor), sets the help line to
 * the item's description and lets 0x80 page a decoration's help; returns
 * the item under the cursor. */
u16 PouchMenu_MoveCursor(u8 *cur, u16 buttons)
{
    u16 item;

    Pouch_MoveCursor(cur, buttons);
    item = gPlayers[gScene.localPlayer].pouch[*cur].item;
    gHelpLine = item + 0x18;
    PouchMenu_DecoCursor(item, buttons & 0x80);
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
    if (buttons & 0xC00) {
        if (buttons & 0x800) {
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
        Sound_PlaySeFlat(0, 0x11, 0);
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
    if (buttons & 0xC00) {
        if (buttons & 0x800) {
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
        Sound_PlaySeFlat(0, 0x11, 0);
    }
    return cur;
}

/* For a decoration, moves the help's column (gHelpColumn, 3 of them) with
 * the buttons given. */
void PouchMenu_DecoCursor(u16 item, u16 buttons)
{
    if (gItemData[item].deco != 0 && item != 0) {
        Menu_CursorLeftRight(&gHelpColumn, buttons & 0x88, 3);
    }
}

/* Moves a yes/no choice (0 yes, 1 no) by the buttons in mask: down or
 * right to no, up or left to yes, with the cursor sound. Returns the
 * choice. */
u8 YesNo_Toggle(u16 buttons, u8 choice, u16 mask)
{
    buttons = buttons & mask;
    if (choice == 0) {
        if (buttons & 0x1400) {
            choice = 1;
            Sound_PlaySeFlat(0, 0x17, 0);
        }
    } else {
        if (buttons & 0x2800) {
            choice = 0;
            Sound_PlaySeFlat(0, 0x17, 0);
        }
    }
    return choice;
}

/* The stick of pad as D-pad buttons: none inside the dead zone (0x40),
 * else one of eight directions by its angle (a 16-bit turn, turned by
 * 0x1200 so that each sector starts at 0): right, up-right, up, up-left,
 * left, down-left, down, down-right. The four straight sectors are wider
 * (0x2400) than the diagonal ones (0x1C00). */
u16 Pad_StickToDpad(u8 pad)
{
    u16 angle;

    if (gPadState[pad].magR <= 0x40) {
        return 0;
    }
    angle = gPadState[pad].degR + 0x1200;
    if (angle < 0x2400) {
        return 0x400;
    }
    if (angle < 0x4000) {
        return 0x2400;
    }
    if (angle < 0x6400) {
        return 0x2000;
    }
    if (angle < 0x8000) {
        return 0x2800;
    }
    if (angle < 0xA400) {
        return 0x800;
    }
    if (angle < 0xC000) {
        return 0x1800;
    }
    if (angle < 0xE400) {
        return 0x1000;
    }
    return 0x1400;
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
 * player rides a monster only item 0x61 can be used. Items with icon 4 need
 * an action spot of kind 2 unless player flag 0x80000 is set. Returns 1 if
 * usable.
 */
s32 Item_IsUsable(u16 id)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 pointPos[12];
    s16 pointKind;
    u8 ride;

    if (func_723BC0(pl) == 1 && Item_IsStealable(id) == 1) {
        return 0;
    }
    ride = pl->rideState;
    if (ride == 1) {
        if (id == 0x61) {
            return Item_RideItemUsable();
        }
        return 0;
    }
    switch (id) {
    case 0x54:
    case 0x299:
        if (Player_GetAreaAmbModel(pl) < 0) {
            return 0;
        }
        break;
    case 0x2A6:
        if (pl->spot == NULL) {
            return 0;
        }
        if (((Spot *)pl->spot)->kind != 0xB) {
            return 0;
        }
        break;
    case 0x55:
    case 0x296:
        return Item_CanPlaceKind2();
    case 0x56:
        return Scene_InNoBarrelBombArea();
    case 0x9C:
    case 0x294:
    case 0x9D:
    case 0x9E:
    case 0x9F:
    case 0xA0:
        return Pouch_HasItem28(pl);
    case 0xA2:
    case 0x2F7:
        return 0;
    case 0x2F8:
    case 0x2F9:
    case 0x30A:
    case 0x30B:
    case 0x30D:
    case 0x2FA:
    case 0x2FB:
    case 0x2FC:
    case 0x2FF:
    case 0x300:
    case 0x301:
    case 0x306:
    case 0x307:
    case 0x308:
    case 0x309:
    case 0x314:
    case 0x318:
    case 0x30F:
    case 0x310:
    case 0x311:
    case 0x312:
    case 0x315:
    case 0x316:
    case 0x317:
    case 0x30C:
    case 0x319:
    case 0x313:
        return SpecialMine_IsFree(pl);
    case 0xAC:
    case 0x295:
    case 0xEA:
    case 0xF2:
        if (pl->isGunner == 1) {
            return 0;
        }
        break;
    case 0x5B:
    case 0x5C:
    case 0x5F:
    case 0x60:
        return Item_CanUseSmallBarrelBomb();
    case 0x5D:
    case 0x5E:
    case 0x29A:
        return Item_CanPlaceLargeBarrelBomb();
    case 0x62:
        return Item_CanPlaceKind1();
    case 0x61:
        if (ride != 1) {
            return 0;
        }
        if (Item_RideItemUsable() != 1) {
            return 0;
        }
        break;
    case 0x28:
    case 0x2C:
    case 0x2E:
    case 0x2D:
        return Scene_CanPlaceKind3();
    case 0x96:
    case 0x97:
    case 0x98:
        if (Player_GetGatherPoint(pl, (u16 *) &pointKind, (f32 *) pointPos) == 0xFFFF) {
            return 0;
        }
        if (pointKind != 3) {
            return 0;
        }
        break;
    case 0x99:
    case 0x9A:
    case 0x9B:
        if (Player_GetGatherPoint(pl, (u16 *) &pointKind, (f32 *) pointPos) == 0xFFFF) {
            return 0;
        }
        if (pointKind != 4) {
            return 0;
        }
        break;
    case 0x29B:
        if (pl->spot == NULL) {
            return 0;
        }
        if (((Spot *)pl->spot)->kind != 6) {
            return 0;
        }
        break;
    case 0x2B7:
        if (pl->spot == NULL) {
            return 0;
        }
        if (((Spot *)pl->spot)->kind != 0xA) {
            return 0;
        }
        break;
    case 0x4E:
        return Item_UsableOutsideCamp();
    case 0xAD:
        return Player_LocalAreaOk_Unk();
    case 0x45:
    case 0x50:
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

    if (Save_TestEventFlag(0xB6) != 1) {
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
        if (pl->ammoSlot != 0xFF) {
            ammo = pl->pouch[pl->ammoSlot];
        }
        if (pl->coatingSlot != 0xFF) {
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
            if (pl->ammoSlot != 0xFF) {
                for (slot = pl->pouch, i = 0; i < 20; i++, slot++) {
                    if (slot->item == ammo.item) {
                        pl->ammoSlot = i;
                        break;
                    }
                }
            }
            if (pl->coatingSlot != 0xFF) {
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
 * attack input runs with rounds loaded, or the player is in state 0x36 or
 * 0x7A with item 0x29B in the pouch. */
s32 Player_IsAiming(Player *pl)
{
    if (pl->bowgunReady != 0) {
        return 1;
    }
    if ((Player_IsState(pl, 0, 0x36) != 0 || Player_IsState(pl, 0, 0x7A) != 0) && Pouch_Count(pl, 0x29B) != 0) {
        return 1;
    }
    return 0;
}

/* Draws a list's cursor bar over entry index (rows step pixels apart from
 * top), from x left to x right, in colour rgb with its alpha pulsing
 * (0xBF +- 48, a 64-frame period). */
void ListPanel_DrawCursorBar(s32 left, s32 right, s16 top, s16 step, s32 index, s32 rgb)
{
    PrimLine bar;
    u16 phase;

    bar.x1 = right;
    bar.x0 = left;
    bar.y0 = top - 2 + step * index;
    bar.y1 = bar.y0 + step + 2;
    phase = (gFrameTick & 0x3F) << 10;
    bar.color = (rgb & 0xFFFFFF) | (((s8)(48.0f * Math_Sin((6.2831855f / 65536.0f) * phase)) + 0xBF) << 24);
    Prim_FillRect(&bar);
}

/* Draws count icons from the HUD's icon sheet: each a square of its size
 * at its place, with its alpha. */
void Draw_Icons(Icon *icons, u8 count)
{
    TexRect spr;

    Gfx_SetState63(1);
    Gfx_SetBlend(4, 5);
    Tex_Prepare(1, 0x155);
    Tex_Bind(0x155);
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

/* Whether pl's map shows: by the map skill (skill group 0x23) when the
 * player has one of its values, which shows it only at 0x91; else when
 * item 0x291 is in the pouch. */
s32 Player_HasMap(Player *pl)
{
    u8 skill = Player_GetSkill(pl, 0x23);

    if (skill != 0) {
        if (skill == 0x91) {
            return 1;
        }
        return 0;
    }
    return Pouch_Count(pl, 0x291) != 0;
}

/* Which monsters pl's map shows (DETECT_* bits): the detect skill (skill
 * group 0x1B; 0xA2 is its higher level), and DETECT_TIMER while a detect
 * effect runs. */
u8 Player_DetectLevel(Player *pl)
{
    u8 level = 0;
    u8 skill = Player_GetSkill(pl, 0x1B);

    if (skill != 0) {
        if (skill == 0xA2) {
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
 * 0x79 to 0x7B are on the left half and the others on the right; every
 * other stage has a whole map. */
s32 Map_GetHalf(u8 stage, u8 area)
{
    switch (stage) {
    case 13:
    case 14:
    case 15:
        switch (area) {
        case 0x79:
        case 0x7A:
        case 0x7B:
            return MAP_HALF_LEFT;
        default:
            return MAP_HALF_RIGHT;
        }
    default:
        return MAP_HALF_WHOLE;
    }
}

/* Draws digit (0..9) as its full-width character (Shift-JIS 0x824F + digit). */
void Text_DrawDigit(u8 digit)
{
    char buf[3];

    buf[0] = 0x82;
    buf[1] = digit + 0x4F;
    buf[2] = 0;
    Text_Draw(buf);
}
