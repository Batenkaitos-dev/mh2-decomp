/*
 * The player's item pouch (Player.pouch, POUCH_SLOTS ItemStacks of
 * ITEM_ID_* items): adding, taking and counting, the room left, and the
 * cursor. Item_Use and its helpers and the player's states (in the game
 * overlay) call these; the HUD draws what they keep.
 */
#include "common.h"
#include "quest.h"
#include "hud.h"
#include "player.h"
#include "scene.h"
#include "item.h"
#include "save.h"
#include "pad.h"
#include "sound.h"
#include "em.h"
#include "objhead.h"
#include "area.h"
#include "area_draw.h"
#include "bgm.h"
#include "card.h"
#include "qsync.h"

u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);

/* Selects the first pouch slot with a selectable item (flag 1). */
void Pouch_SelectFirst(Player *pl)
{
    s16 i;

    pl->pouchCursor = 0;
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].item != 0 && pl->pouch[i].count > 0 && (gItemData[pl->pouch[i].item].flags & ITEM_FLAG_SELECTABLE)) {
            pl->pouchCursor = (u8)i;
            return;
        }
    }
}

#pragma optimization_level 4
/*
 * Adds n of an item to the pouch (n < 0 takes them away). Returns 0 for an
 * invalid item (and for a new stack), 5 when there is no free slot, 3 when
 * the pouch already holds the maximum (or another one-per-pouch item), 4
 * when the stack is used up, 2 when it was capped at the maximum, else 1.
 * Unless `silent`, the item is recorded with Save_MarkItemObtained. Ammo
 * added or used up reloads the bowgun's selected slot. Item 135 is never
 * added. Compiled at optimization level 4: the match needs it.
 */
u8 Pouch_AddEx(Player *pl, u16 item, s16 n, u8 silent)
{
    s16 max;
    s16 i;
    u8 ret;

    if (item >= ITEM_ID_COUNT) {
        return 0;
    }
    if (item == 135) {
        return 0;
    }
    if (gItemData[item].max == ITEM_MAX_UNLIMITED) {
        n = ITEM_MAX_UNLIMITED;
    }
    if (Pouch_Count(pl, item) == 0) {
        ret = 5;
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (pl->pouch[i].item == 0 && n > 0) {
                max = gItemData[item].max;
                pl->pouch[i].item = item;
                if (max < n) {
                    n = max;
                }
                pl->pouch[i].count = n;
                if (silent == 0) {
                    Save_MarkItemObtained(&gSave, item);
                }
                if (gItemData[item].type == ITEM_TYPE_AMMO) {
                    if (i == pl->ammoSlot) {
                        pl->ammoSlot = Bowgun_NextAmmoSlot(pl, i, 2);
                        Bowgun_LoadAmmo(pl, 1);
                        pl->savedAmmoCount = pl->ammoCount;
                        pl->savedClipSize = pl->clipSize;
                        pl->savedRoundsLoaded = 0;
                    } else if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                        if (pl->ammoSlot == POUCH_SLOT_NONE) {
                            pl->ammoSlot = Bowgun_NextAmmoSlot(pl, 0, 2);
                            Bowgun_LoadAmmo(pl, 0);
                        } else if (gItemData[pl->pouch[pl->ammoSlot].item].type != ITEM_TYPE_AMMO) {
                            pl->ammoSlot = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 0);
                            Bowgun_LoadAmmo(pl, 0);
                        }
                    }
                }
                ret = 0;
                if (!(gItemData[pl->pouch[pl->pouchCursor].item].flags & ITEM_FLAG_SELECTABLE)) {
                    pl->pouchCursor = Pouch_NextSelectable(pl, pl->pouchCursor, 0);
                }
                break;
            }
        }
    } else if (n > 0 && Item_IsOnePerPouch(item) == 1) {
        ret = 3;
    } else {
        for (i = 0; i < POUCH_SLOTS; i++) {
            if (pl->pouch[i].item == item) {
                max = gItemData[item].max;
                if (n > 0 && pl->pouch[i].count >= max) {
                    pl->pouch[i].count = max;
                    if (silent == 0) {
                        Save_MarkItemObtained(&gSave, item);
                    }
                    ret = 3;
                } else {
                    if (n < 0 && gItemData[item].max == ITEM_MAX_UNLIMITED) {
                        ret = 1;
                        break;
                    }
                    pl->pouch[i].count += n;
                    if (pl->pouch[i].count <= 0) {
                        Pouch_ClearSlot(pl, i);
                        ret = 4;
                        pl->pouch[i].item = 0;
                        pl->pouch[i].count = 0;
                    } else {
                        if (max < pl->pouch[i].count) {
                            pl->pouch[i].count = max;
                            ret = 2;
                        } else {
                            ret = 1;
                        }
                        if (silent == 0) {
                            Save_MarkItemObtained(&gSave, item);
                        }
                    }
                }
                if (gItemData[item].type == ITEM_TYPE_AMMO && i == pl->ammoSlot) {
                    pl->ammoCount = pl->pouch[pl->ammoSlot].count;
                    pl->savedAmmoCount = pl->ammoCount;
                    if (pl->ammoCount < pl->roundsLoaded) {
                        pl->roundsLoaded = pl->ammoCount;
                        pl->savedRoundsLoaded = pl->roundsLoaded;
                    }
                }
                break;
            }
        }
    }
    /* the game reads a register it never set: when the second loop finds no slot holding item (Pouch_Count said there is one, so it does not happen), ret is s2, still the caller's */
    return ret;
}
#pragma optimization_level reset

/* Pouch_AddEx, recording the item as obtained. */
u8 Pouch_Add(Player *pl, u16 item, s16 n)
{
    return Pouch_AddEx(pl, item, n, 0);
}

/*
 * 1 for the items 161 to 169: the pouch holds one of them at most, all
 * together (Pouch_Room, Pouch_Count; the spit roast gives back ITEM_ID_ROAST_LEFTOVER).
 */
s32 Item_IsOnePerPouch(u16 item)
{
    switch (item) {
    case 161:
    case 162:
    case 163:
    case 164:
    case 165:
    case 166:
    case 167:
    case 168:
    case 169:
        return 1;
    }
    return 0;
}

/* 1 if the pouch holds one of the one-per-pouch items (Item_IsOnePerPouch). */
s32 Pouch_HasOnePerPouch(Player *pl)
{
    s16 i;

    for (i = 0; i < POUCH_SLOTS; i++) {
        if (Item_IsOnePerPouch(pl->pouch[i].item) == 1) {
            return 1;
        }
    }
    return 0;
}

/* How many of an item the pouch holds (a one-per-pouch item: 1 when any is held). */
s16 Pouch_Count(Player *pl, u16 item)
{
    s16 i;

    if (Item_IsOnePerPouch(item) == 1) {
        return Pouch_HasOnePerPouch(pl) == 1;
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].item == item) {
            return pl->pouch[i].count;
        }
    }
    return 0;
}

/* How many more of an item the pouch can take in its stack (ITEM_MAX_UNLIMITED: any). */
s16 Pouch_Room(Player *pl, u16 item)
{
    s16 i;

    if (Item_IsOnePerPouch(item) == 1) {
        return Pouch_HasOnePerPouch(pl) != 1;
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].item == item) {
            if (gItemData[item].max == ITEM_MAX_UNLIMITED) {
                return ITEM_MAX_UNLIMITED;
            }
            /* A return in both branches, as the game has it (each jumps to the
             * epilogue): the first probably held a print compiled out. */
            if (pl->pouch[i].count > gItemData[item].max) {
                return gItemData[item].max - pl->pouch[i].count;
            }
            return gItemData[item].max - pl->pouch[i].count;
        }
    }
    return gItemData[item].max;
}

/* As Pouch_Room, and -1 when the item needs a free slot and there is none. */
s16 Pouch_Space(Player *pl, u16 item)
{
    s16 i;
    s16 empty = 0;

    if (Item_IsOnePerPouch(item) == 1 && Pouch_HasOnePerPouch(pl) == 1) {
        return 0;
    }
    for (i = 0; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].item == item) {
            if (gItemData[item].max == ITEM_MAX_UNLIMITED) {
                return ITEM_MAX_UNLIMITED;
            }
            /* A return in both branches, as the game has it (each jumps to the
             * epilogue): the first probably held a print compiled out. */
            if (pl->pouch[i].count > gItemData[item].max) {
                return gItemData[item].max - pl->pouch[i].count;
            }
            return gItemData[item].max - pl->pouch[i].count;
        }
        if (pl->pouch[i].item == 0) {
            empty++;
        }
    }
    if (empty == 0) {
        return -1;
    }
    return gItemData[item].max;
}

/* The pouch slot of an item, POUCH_SLOT_NONE if none. */
s16 Pouch_FindSlot(Player *pl, u16 item)
{
    s16 i;

    for (i = 0; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].item == item) {
            return i;
        }
    }
    return POUCH_SLOT_NONE;
}

/* The count in the selected pouch slot. */
s16 Pouch_CursorCount(Player *pl)
{
    return pl->pouch[pl->pouchCursor].count;
}

/* The number of pouch slots with a selectable item. */
s16 Pouch_CountSelectable(Player *pl)
{
    s16 n;
    s16 i;

    i = 0;
    n = 0;
    for (; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].count > 0) {
            s16 item = pl->pouch[i].item;

            if (item != 0 && (gItemData[item].flags & ITEM_FLAG_SELECTABLE)) {
                n++;
            }
        }
    }
    return n;
}

/* Nonzero if the pouch has a free slot. */
s16 Pouch_HasFreeSlot(Player *pl)
{
    s16 i;

    for (i = 0; i < POUCH_SLOTS; i++) {
        if (pl->pouch[i].item == 0) {
            return 1;
        }
    }
    return 0;
}

/* Empties a pouch slot, and reselects the bowgun's ammo if it was loaded. */
s16 Pouch_ClearSlot(Player *pl, s16 slot)
{
    u16 item;

    if (slot >= POUCH_SLOTS) {
        return 2;
    }
    item = pl->pouch[slot].item;
    pl->pouch[slot].count = 0;
    if (pl->pouch[slot].item == 0) {
        return 0;
    }
    pl->pouch[slot].item = 0;
    if (gItemData[item].type == ITEM_TYPE_AMMO) {
        if ((u8)Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 1) == POUCH_SLOT_NONE) {
            if (pl->weaponType != WEAPON_GUNLANCE && pl->weaponType != WEAPON_BOW) {
                pl->ammoSlot = POUCH_SLOT_NONE;
                pl->ammoCount = 0;
                pl->clipSize = 0;
                pl->roundsLoaded = 0;
            }
        } else if (slot == pl->ammoSlot) {
            pl->ammoCount = 0;
            pl->clipSize = 0;
            pl->roundsLoaded = 0;
        }
    } else if (gItemData[item].type == ITEM_TYPE_COATING && slot == pl->ammoSlot) {
        Bow_ClearCoatingSlot(pl);
        pl->ammoSlot = Bowgun_NextAmmoSlot(pl, slot, 0);
        Bowgun_LoadAmmo(pl, 0);
    }
    return 1;
}

/* Sets the count of an item to at least n (mode 1: exactly n). */
s32 Pouch_SetAtLeast(Player *pl, u16 mode, u16 item, s16 n)
{
    s16 have = Pouch_Count(pl, item);

    if (have < n) {
        Pouch_AddEx(pl, item, n - have, 1);
        return 1;
    }
    if (mode == 1) {
        Pouch_AddEx(pl, item, n - have, 1);
        return 2;
    }
    return 3;
}

