/*
 * The player's status: the item pouch (Player.pouch, POUCH_SLOTS ItemStacks
 * of ITEM_ID_* items: adding, taking and counting, the cursor), the bowgun's
 * and the bow's ammo and coating slots, health, stamina, hunger and
 * sharpness, the gestures, and two menus run from the player's state: the
 * supply box (SceneState.supply) and the quest's delivery box. Item_Use and
 * its helpers and the player's states (in the game overlay) call these; the
 * HUD draws what they keep.
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

typedef s16 SharpRow[6];


extern u8 gGestureStates[];
extern u8 gBowgunClipSizes[][32];
extern s16 D_005B396C;
extern Em gEms[];
extern SharpRow *gSharpnessTables[];
extern char *gSharpnessDownText[];
extern s32 gSharpnessUpText;
extern char *gSharpnessFullText[];

s32 Quest_PhaseIsOver(s32 a);
s32 Scene_CanPlaceKind2(void);
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
s32 Player_IsLocal(void *obj);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Item_IsOnePerPouch(u16 item);
void Save_MarkItemObtained(SaveData *save, u16 item);
void Player_Die(Player *p); /* unit's own declaration: the game's takes Em * */
void QuestStats_Add(s32 a, s32 b);
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir);
void Bow_ClearCoatingSlot(Player *p);
void Bowgun_LoadAmmo(Player *p, s32 a);
f32 *Em_BoneMatrix(void *obj, s16 bone);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_RotateZ(f32 *m, f32 a);
f32 Vec_Distance(f32 *a, f32 *b);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);
u8 Notice_PushItem(char *item);
s32 Player_StaminaLocked(Player *p);
s16 Area_Temperature(u16 area);
void Hud_StartSupplyMsg(u8 *p);
s32 Game_IsOnline(void);
void QSync_SendSupply(u8 kind, u8 who);
void SupplyBox_StartMove(u16 item, u8 slot);
u8 Notice_Push(u8 kind, s16 a, s16 b);
s32 QuestGoal_IsDone(s32 a);
s32 MsgEvent_Check(u16 kind);
s32 Quest_CheckSubClearMode(void);
void Jingle_Play(void *obj, s32 se);
s16 Pouch_Room(Player *p, u16 item);
s16 Pouch_HasFreeSlot(Player *p);
u32 Bowgun_GetAmmoMask(Player *p);
s32 Quest_IsVersus(void);
s32 Quest_GetDeliveryTarget(Player *p, s32 slot, ItemStack *out);
u16 Quest_DeliverItem(Player *pl, u16 item, s16 count);

u8 Pouch_AddEx(Player *p, u16 item, s16 n, u8 silent);
s16 Pouch_Count(Player *p, u16 item);
s16 Pouch_Space(Player *p, u16 item);
u8 Player_GetSharpnessLevel(Player *p);
s16 Pouch_ClearSlot(Player *p, s16 slot);

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
 * added or used up reloads the bowgun's selected slot. Item 0x87 is never
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
    if (item == 0x87) {
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
 * 1 for the items 0xA1 to 0xA9: the pouch holds one of them at most, all
 * together (Pouch_Room, Pouch_Count; the spit roast gives back 0xA2).
 */
s32 Item_IsOnePerPouch(u16 item)
{
    switch (item) {
    case 0xA1:
    case 0xA2:
    case 0xA3:
    case 0xA4:
    case 0xA5:
    case 0xA6:
    case 0xA7:
    case 0xA8:
    case 0xA9:
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

/*
 * Turns one of the bones that follow the player's look (9, 10, 19 and 20)
 * by a share of Player.lookYaw: 0.3 for the first three, 0.1 for bone 20;
 * bone 9 also takes Player.lean.
 */
void Player_TwistBoneToLook(Player *pl, s16 bone)
{
    f32 *mtx;
    f32 angle;

    switch (bone) {
    case 9:
        angle = 0.3f * ((6.2831855f * pl->lookYaw) / 65536.0f);
        mtx = Em_BoneMatrix(pl, 9);
        Mtx_RotateY(mtx, angle);
        Mtx_RotateZ(mtx, (6.2831855f * pl->lean) / 65536.0f);
        break;
    case 10:
        angle = 0.3f * ((6.2831855f * pl->lookYaw) / 65536.0f);
        Mtx_RotateY(Em_BoneMatrix(pl, 10), angle);
        break;
    case 19:
        angle = 0.3f * ((6.2831855f * pl->lookYaw) / 65536.0f);
        Mtx_RotateY(Em_BoneMatrix(pl, 19), angle);
        break;
    case 20:
        angle = 0.1f * ((6.2831855f * pl->lookYaw) / 65536.0f);
        Mtx_RotateY(Em_BoneMatrix(pl, 20), angle);
        break;
    }
}

/* Moves the player to Player.moveTo over `frames` frames
 * (Player_UpdateMoveTo), or at once when it is 800 or more away. */
void Player_StartMoveTo(Player *pl, s16 frames)
{
    if (!(Vec_Distance(pl->pos, pl->moveTo) < 800.0f)) {
        pl->moveToFrames = 0;
        pl->pos[0] = pl->moveTo[0];
        pl->pos[1] = pl->moveTo[1];
        pl->pos[2] = pl->moveTo[2];
        return;
    }
    pl->moveToFrames = frames;
    pl->moveToStep[0] = (pl->moveTo[0] - pl->pos[0]) / frames;
    pl->moveToStep[1] = (pl->moveTo[1] - pl->pos[1]) / frames;
    pl->moveToStep[2] = (pl->moveTo[2] - pl->pos[2]) / frames;
}

/* One frame of Player_StartMoveTo's move: a step while frames are left. */
void Player_UpdateMoveTo(Player *pl)
{
    s16 framesLeft = pl->moveToFrames;

    if (framesLeft != 0) {
        pl->moveToFrames = framesLeft - 1;
        if (framesLeft > 0) {
            pl->pos[0] += pl->moveToStep[0];
            pl->pos[1] += pl->moveToStep[1];
            pl->pos[2] += pl->moveToStep[2];
        }
    }
}

/* The first entry of gAreaFxModels for the player's area, or -1 where a
 * kind 2 object can't be placed (Scene_CanPlaceKind2); the pitfall trap is
 * set only when it is 0 or more (Player_UseItem). */
s16 Player_GetAreaPitModel(Player *pl)
{
    if (Scene_CanPlaceKind2() == 0) {
        return -1;
    }
    return gAreaFxModels[pl->area][0];
}

/* 1 while the player doesn't flinch: its own timer, an item's
 * (ITEM_ID_DEFENSE_BURST) or the dual swords' demon mode. */
s32 Player_IgnoresFlinch(Player *pl)
{
    if (pl->noFlinchTimer != 0 || pl->noFlinchItemTimer != 0 || pl->demonMode != 0) {
        return 1;
    }
    return 0;
}

/* Counts an item use for the player (SceneState.itemUses). */
void Stats_CountItemUse(Player *pl)
{
    gScene.itemUses[pl->index]++;
}

/* Adds n to the health, within [0, hpMax]; the recoverable part follows it up. */
void Player_AddHp(Player *pl, s16 n)
{
    if (Player_IsLocal(pl) != 0) {
        if (Quest_PhaseIsOver(0) == 1 && n < 0) {
            return;
        }
        if (pl->stateGroup != PL_GROUP_FAINT) {
            pl->hp += n;
            if (pl->hp <= 0) {
                pl->hp = 0;
            }
            if (pl->hp >= pl->hpMax) {
                pl->hp = pl->hpMax;
            }
            if (pl->hp >= pl->hpRecoverable) {
                pl->hpRecoverable = pl->hp;
            }
        }
    }
}

/* Takes damage; `lose` also drops the recoverable health. Returns 1 when the
 * player died (Player_Die). */
s32 Player_Damage(Player *pl, s16 damage, s16 lose)
{
    if (Player_IsLocal(pl) == 0) {
        return 0;
    }
    Player_AddHp(pl, -damage);
    if (lose != 0) {
        pl->hpRecoverable = pl->hp;
    }
    if (pl->hp <= 0) {
        pl->hpRecoverable = 0;
        if (pl->action != PL_POSTURE_AIRBORNE) {
            Player_Die(pl);
            return 1;
        }
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_FALL) == 0) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FALL, 0);
        }
    }
    return 0;
}

/* Heals n, a quarter more with skill 0xA3 and a quarter less with 0xA4;
 * QuestStats_Add(2, 1) unless `quiet`. */
void Player_Heal(Player *pl, s16 n, s16 quiet)
{
    s16 healed;

    if (Player_HasSkill(pl, SKILL_HEAL_UP) == 1 && n > 0) {
        healed = n + n / 4;
    } else if (Player_HasSkill(pl, SKILL_HEAL_DOWN) == 1 && n > 0) {
        healed = n - n / 4;
    } else {
        healed = n;
    }
    Player_AddHp(pl, healed);
    if (quiet == 0 && Player_IsLocal(pl) == 1) {
        QuestStats_Add(2, 1);
    }
}

/* Changes the local player's maximum health (1..150), lowering the health
 * to it; unless `transient`, the base one (Player.hpMaxBase) too. */
void Player_AddMaxHp(Player *pl, s16 n, u8 transient)
{
    if (Player_IsLocal(pl) != 0) {
        pl->hpMax += n;
        if (pl->hpMax < 2) {
            pl->hpMax = 1;
        }
        if (pl->hpMax >= 150) {
            pl->hpMax = 150;
        }
        if (pl->hp >= pl->hpMax) {
            pl->hp = pl->hpMax;
            D_005B396C = pl->hp; /* a copy of the health nothing in the C reads */
        }
        if (pl->hpRecoverable >= pl->hpMax) {
            pl->hpRecoverable = pl->hpMax;
        }
        if (transient == 0) {
            pl->hpMaxBase += n;
            if (pl->hpMaxBase < 2) {
                pl->hpMaxBase = 1;
            }
            if (pl->hpMaxBase >= 150) {
                pl->hpMaxBase = 150;
            }
        }
    }
}

/* The sharpness colour (0-5) of the player's weapon: the thresholds come
 * from a per-weapon-class table, row by the weapon's sharpness type. */
/* The class's table is read as s16 * and stepped by the row, as EquipMenu_DrawSharpness reads it. */
u8 Player_GetSharpnessLevel(Player *pl)
{
    s16 *thresholds;
    u8 level;

    if (pl->isGunner == 1) {
        level = 0;
    } else {
        thresholds = (s16 *)gSharpnessTables[pl->weaponType];
        thresholds += gBladeData[pl->equip[EQUIP_SLOT_WEAPON].id].sharpType * 6;
        if (pl->sharpness <= thresholds[0]) {
            level = 0;
        } else if (pl->sharpness <= thresholds[1]) {
            level = 1;
        } else if (pl->sharpness <= thresholds[2]) {
            level = 2;
        } else if (pl->sharpness <= thresholds[3]) {
            level = 3;
        } else if (pl->sharpness <= thresholds[4]) {
            level = 4;
        } else {
            level = 5;
        }
    }
    return level;
}

/* Changes the sharpness (losses halved with skill 0x1F, doubled with
 * 0x20), and plays the sounds of a colour change and of full sharpness. */
void Player_AddSharpness(Player *pl, s16 n)
{
    u8 full = 0;
    u8 level;

    if (Player_IsLocal(pl) != 0 && pl->isGunner != 1) {
        if (n < 0) {
            if (Player_HasSkill(pl, 0x1F) == 1) {
                n = n / 2;
                if (n == 0) {
                    n = -1;
                }
            } else if (Player_HasSkill(pl, 0x20) == 1) {
                n = n * 2;
            }
        }
        pl->sharpness += n;
        if (pl->sharpness <= 0) {
            pl->sharpness = 0;
        }
        if (pl->sharpnessMax < pl->sharpness) {
            pl->sharpness = pl->sharpnessMax;
            full = 1;
        }
        level = Player_GetSharpnessLevel(pl);
        if (level != pl->sharpnessLevel) {
            if (level < pl->sharpnessLevel) {
                Notice_PushItem(gSharpnessDownText[0]);
            } else {
                Notice_PushItem((char *) gSharpnessUpText);
            }
            pl->sharpnessLevel = level;
        }
        if (full) {
            Notice_PushItem(gSharpnessFullText[0]);
        }
    }
}

/*
 * The next pouch slot from `slot` holding ammo the bowgun can load, POUCH_SLOT_NONE if
 * none (not a gunner, or no bowgun). dir 0 and 1 step forward and back
 * first; 2 and 3 start at `slot` itself. Bowgun kind 10 uses flag 4 and 21
 * slots, and also takes item 0x87.
 */
u16 Bowgun_NextAmmoSlot(Player *pl, u16 slot, u8 dir)
{
    s16 n;
    u32 mask;
    s16 i;
    s32 flag;

    if (pl->isGunner != 1) {
        return POUCH_SLOT_NONE;
    }
    if (pl->equip[EQUIP_SLOT_WEAPON].type != EQUIP_GUN) {
        return POUCH_SLOT_NONE;
    }
    if (pl->weaponType == WEAPON_BOW) {
        n = 21;
    } else {
        n = 20;
    }
    if (slot >= n) {
        slot = 0;
    }
    switch (dir) {
    case 0:
        slot = (slot + 1) % n;
        break;
    case 1:
        if (slot == 0) {
            slot = n - 1;
        } else {
            slot--;
        }
        break;
    case 2:
    case 3:
        break;
    }
    mask = Bowgun_GetAmmoMask(pl);
    if (pl->weaponType == WEAPON_BOW) {
        flag = 4;
    } else {
        flag = 2;
    }
    for (i = 0; i < n; i++) {
        if (pl->pouch[slot].item != 0 && pl->pouch[slot].count > 0 &&
            (flag & gItemData[pl->pouch[slot].item].flags)) {
            if (flag == 4 && pl->pouch[slot].item == 0x87) {
                return slot;
            }
            if (mask & (1 << gItemData[pl->pouch[slot].item].ammoKind)) {
                return slot;
            }
        }
        switch (dir) {
        case 0:
        case 2:
            slot = (slot + 1) % n;
            break;
        case 1:
        case 3:
            if (slot == 0) {
                slot = n - 1;
            } else {
                slot--;
            }
            break;
        }
    }
    return POUCH_SLOT_NONE;
}

void Bow_SetCoatingSlot(Player *pl)
{
    pl->coatingSlot = pl->ammoSlot;
}

void Bow_ClearCoatingSlot(Player *pl)
{
    pl->coatingSlot = POUCH_SLOT_NONE;
}

/* Loads the ammo of the selected slot: its kind, the rounds left, and the
 * clip size for this bowgun (+1 with SKILL_LOAD_UP). */
void Bowgun_LoadAmmo(Player *pl, s32 unused)
{
    if (pl->equip[EQUIP_SLOT_WEAPON].type == EQUIP_GUN && pl->ammoSlot != POUCH_SLOT_NONE) {
        pl->ammoType = gItemData[pl->pouch[pl->ammoSlot].item].ammoKind;
        if (gItemData[pl->pouch[pl->ammoSlot].item].max == ITEM_MAX_UNLIMITED) {
            pl->ammoCount = ITEM_MAX_UNLIMITED;
        } else {
            pl->ammoCount = pl->pouch[pl->ammoSlot].count;
        }
        pl->clipSize = gBowgunClipSizes[gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].clipRow][pl->ammoType];
        if (Player_HasSkill(pl, SKILL_LOAD_UP) == 1) {
            pl->clipSize++;
        }
        pl->roundsLoaded = 0;
        if (pl->weaponType == WEAPON_BOW) {
            Bow_ClearCoatingSlot(pl);
        }
    }
}

/* The ammo kinds the player can load: the bowgun's own (weapon data), plus
 * those the skills 0x39-0x46 add (not for bowgun kind 10). */
u32 Bowgun_GetAmmoMask(Player *pl)
{
    u32 mask;

    if (pl->isGunner != 1) {
        return 0;
    }
    mask = gGunData[pl->equip[EQUIP_SLOT_WEAPON].id].ammoMask;
    if (pl->weaponType == WEAPON_BOW) {
        return mask;
    }
    if (Player_HasSkill(pl, 0x39) == 1) {
        mask |= 1;
    } else if (Player_HasSkill(pl, 0x3A) == 1) {
        mask |= 7;
    }
    if (Player_HasSkill(pl, 0x3B) == 1) {
        mask |= 8;
    } else if (Player_HasSkill(pl, 0x3C) == 1) {
        mask |= 0x18;
    } else if (Player_HasSkill(pl, 0x3D) == 1) {
        mask |= 0x38;
    }
    if (Player_HasSkill(pl, 0x3E) == 1) {
        mask |= 0x40;
    } else if (Player_HasSkill(pl, 0x3F) == 1) {
        mask |= 0xC0;
    } else if (Player_HasSkill(pl, 0x40) == 1) {
        mask |= 0x1C0;
    }
    if (Player_HasSkill(pl, 0x41) == 1) {
        mask |= 0x200;
    } else if (Player_HasSkill(pl, 0x42) == 1) {
        mask |= 0x600;
    } else if (Player_HasSkill(pl, 0x43) == 1) {
        mask |= 0xE00;
    }
    if (Player_HasSkill(pl, 0x44) == 1) {
        mask |= 0x1000;
    } else if (Player_HasSkill(pl, 0x45) == 1) {
        mask |= 0x3000;
    } else if (Player_HasSkill(pl, 0x46) == 1) {
        mask |= 0x7000;
    }
    if (Quest_IsVersus() == 1) {
        mask &= 0xEFFFFFFF;
    }
    return mask;
}

/* The ammo kinds the skills 0x39-0x46 let the player load (a mask). */
s32 Player_GetSkillAmmoMask(Player *pl)
{
    s32 mask = 0;

    if (Player_HasSkill(pl, 0x39) == 1) {
        mask |= 1;
    } else if (Player_HasSkill(pl, 0x3A) == 1) {
        mask |= 7;
    }
    if (Player_HasSkill(pl, 0x3B) == 1) {
        mask |= 8;
    } else if (Player_HasSkill(pl, 0x3C) == 1) {
        mask |= 0x18;
    } else if (Player_HasSkill(pl, 0x3D) == 1) {
        mask |= 0x38;
    }
    if (Player_HasSkill(pl, 0x3E) == 1) {
        mask |= 0x40;
    } else if (Player_HasSkill(pl, 0x3F) == 1) {
        mask |= 0xC0;
    } else if (Player_HasSkill(pl, 0x40) == 1) {
        mask |= 0x1C0;
    }
    if (Player_HasSkill(pl, 0x41) == 1) {
        mask |= 0x200;
    } else if (Player_HasSkill(pl, 0x42) == 1) {
        mask |= 0x600;
    } else if (Player_HasSkill(pl, 0x43) == 1) {
        mask |= 0xE00;
    }
    if (Player_HasSkill(pl, 0x44) == 1) {
        mask |= 0x1000;
    } else if (Player_HasSkill(pl, 0x45) == 1) {
        mask |= 0x3000;
    } else if (Player_HasSkill(pl, 0x46) == 1) {
        mask |= 0x7000;
    }
    return mask;
}

/* The bowgun's clip size for an ammo kind (gBowgunClipSizes, by GunData.clipRow). */
u8 Bowgun_ClipSize(GunData *gun, u8 kind)
{
    return gBowgunClipSizes[gun->clipRow][kind];
}

/* Counts Player.weaponTimer up to 1000, with sound 9 at frame 60. */
void Player_TickWeaponTimer(Player *pl)
{
    if (pl->weaponTimer < 1000) {
        pl->weaponTimer++;
        if (pl->weaponTimer == 60) {
            Player_PlaySe(pl, 9, 0, pl->pos, 1, 0);
        }
    } else {
        pl->weaponTimer = 1000;
    }
}

/* Adds n to the stamina, within [0, staminaMax]; losses are ignored when
 * Player_StaminaLocked says so. */
void Player_AddStamina(Player *pl, s16 n)
{
    if (Player_IsLocal(pl) != 0) {
        if (n < 0 && Player_StaminaLocked(pl) == 1) {
            return;
        }
        pl->stamina += n;
        if (pl->stamina <= 0) {
            pl->stamina = 0;
        }
        if (pl->stamina >= pl->staminaMax) {
            pl->stamina = pl->staminaMax;
        }
    }
}

/* Uses stamina: skill 0xB6 halves the cost (a cost of 1 every other frame),
 * skill 0xB7 doubles it every fifth frame. */
void Player_UseStamina(Player *pl, s32 n)
{
    s16 cost;

    if (Player_IsLocal(pl) != 0) {
        cost = n;
        if (cost < 0) {
            if (cost == -1) {
                if (Player_HasSkill(pl, 0xB6) == 1 && (gScene.frame & 1)) {
                    return;
                }
            } else if (Player_HasSkill(pl, 0xB6) == 1) {
                cost = cost / 2;
            }
            if (Player_HasSkill(pl, 0xB7) == 1 && gScene.frame % 5 == 0) {
                cost *= 2;
            }
        }
        Player_AddStamina(pl, cost);
    }
}

/* Changes the maximum stamina (75..450; reaching 450 restarts the timer). */
void Player_AddMaxStamina(Player *pl, s16 n)
{
    if (Player_IsLocal(pl) != 0) {
        if (n < 0 && Player_StaminaLocked(pl) == 1) {
            return;
        }
        pl->staminaMax += n;
        if (pl->staminaMax < 76) {
            pl->staminaMax = 75;
        } else if (pl->staminaMax > 450) {
            pl->staminaMax = 450;
            pl->staminaTimer = 10800;
        }
        if (pl->staminaMax < pl->stamina) {
            pl->stamina = pl->staminaMax;
        }
    }
}

/*
 * Counts down to the next drop of the maximum stamina (75 every 10800
 * steps): 1 step a frame, more in hot (2) and cold (4) areas unless a drink
 * (unk872) or a skill (0x89-0x8D) protects, and the hunger skills 0x49-0x4C
 * change the pace.
 */
void Player_UpdateHunger(Player *pl)
{
    s16 rate;
    s16 zone;

    if (Player_IsLocal(pl) != 0) {
        zone = Area_Temperature(pl->area);
        rate = 0;
        if (zone == 2 || zone == 4) {
            if (pl->hotDrinkTimer == 0) {
                if ((zone != 2 || (Player_HasSkill(pl, SKILL_COLD_RES) != 1 && Player_HasSkill(pl, SKILL_COLD_RES2) != 1)) &&
                    (zone != 4 || Player_HasSkill(pl, SKILL_COLD_RES2) != 1)) {
                    if (zone == 2) {
                        if (Player_HasSkill(pl, 0x89) == 1) {
                            rate = 1;
                        } else if (Player_HasSkill(pl, 0x8C) == 1) {
                            rate = 3;
                        } else if (Player_HasSkill(pl, 0x8D) == 1) {
                            rate = 4;
                        } else {
                            rate = 2;
                        }
                    } else if (Player_HasSkill(pl, 0x89) == 1) {
                        if (gScene.frame & 1) {
                            rate = 1;
                        } else {
                            rate = 2;
                        }
                    } else if (Player_HasSkill(pl, SKILL_COLD_RES) == 1) {
                        rate = 1;
                    } else if (Player_HasSkill(pl, 0x8C) == 1) {
                        rate = 4;
                    } else if (Player_HasSkill(pl, 0x8D) == 1) {
                        rate = 6;
                    } else {
                        rate = 3;
                    }
                }
            }
        } else {
            /* FAKEMATCH: rate is already 0 here; this store, dropped late as redundant, leaves
             * the original's b to the join (with rate = 3 in its slot) at the end of the cold ladder. */
            rate = 0;
        }
        if (!Player_HasSkill(pl, 0x4A)) {
            if (!Player_HasSkill(pl, 0x49) || (gScene.frame & 1)) {
                rate++;
            }
            if (Player_HasSkill(pl, 0x4C) == 1) {
                rate++;
            } else if (Player_HasSkill(pl, 0x4B) == 1 && !(gScene.frame & 1)) {
                rate++;
            }
        }
        if (rate > 0) {
            pl->staminaTimer -= rate;
            if (pl->staminaTimer <= 0) {
                pl->staminaTimer = 10800;
                Player_AddMaxStamina(pl, -75);
            }
        }
    }
}

/* Asks the local player for a gesture (1 and up; Player_StartGesture starts it). */
void Player_RequestGesture(u8 gesture)
{
    Player *local = &gPlayers[gScene.localPlayer];

    local->actRequest = gesture;
}

/* Starts the requested gesture: its PL_GROUP_GESTURE state from gGestureStates. */
void Player_StartGesture(Player *pl)
{
    if (pl->actRequest != 0) {
        Player_SetState(pl, PL_GROUP_GESTURE, gGestureStates[pl->actRequest], 0);
    }
}

/* Takes the item under the supply box cursor (or asks the host, online). */
void SupplyBox_Take(Player *pl)
{
    SceneState *scene = &gScene;
    u16 item;

    pl->menuDelay = 30;
    Hud_StartSupplyMsg((u8 *)pl);
    if (Game_IsOnline() == 1) {
        pl->supplyBoxTimer = 900;
        pl->supplyTaken = 1;
        QSync_SendSupply(1, gScene.localPlayer);
        return;
    }
    pl->supplyBoxTimer = 0;
    pl->supplyTaken = 0;
    Pouch_Add(pl, scene->supply[pl->menuCursor].item, scene->supply[pl->menuCursor].count);
    scene->supplyTaken[pl->menuCursor / 32] |= 1 << (pl->menuCursor % 32);
    SupplyBox_StartMove(scene->supply[pl->menuCursor].item, pl->menuCursor);
    item = scene->supply[pl->menuCursor].item;
    switch (item) {
    case ITEM_ID_TRADE_GOODS:
        Notice_Push(1, 0x13, item);
        break;
    case ITEM_ID_DELIVERY:
        Notice_Push(1, 0x14, item);
        break;
    }
}

/* Whether a supply box slot is available yet: the first 24 at once (unless
 * gQuest.timedEvent), then 24-31 and 32-39 with the deliveries (QuestGoal_IsDone). */
s32 SupplyBox_IsOpen(u8 slot)
{
    if (slot < 24) {
        if (gQuest.timedEvent != 0) {
            return 0;
        }
    } else if (slot < 32) {
        if (QuestGoal_IsDone(1) == 0) {
            return 0;
        }
    } else if (QuestGoal_IsDone(2) == 0) {
        return 0;
    }
    return 1;
}

/* The supply box menu: 40 slots, 8 to a row. */
void SupplyBox_Update(Player *pl)
{
    s16 pressed;
    SceneState *scene = &gScene;
    s16 buttons;

    if (Player_IsLocal(pl) != 0) {
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        if (pl->menuDelay != 0) {
            pl->menuDelay--;
        }
        if (pl->supplyBoxTimer != 0) {
            pl->supplyBoxTimer--;
            if (pl->supplyBoxTimer == 0) {
                pl->supplyTaken = 0;
            }
        }
        if (pl->supplyTaken != 0 || pl->menuDelay != 0) {
            return;
        }
        if (buttons & PAD_CROSS) {
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            pl->supplyBoxOpen = 0;
            MsgEvent_Check(2);
            return;
        }
        if (buttons & PAD_LEFT) {
            Sound_PlaySeFlat(0, 0x16, 0);
            if (pl->menuCursor & 7) {
                pl->menuCursor--;
            } else {
                pl->menuCursor += 7;
            }
        }
        if (buttons & PAD_RIGHT) {
            Sound_PlaySeFlat(0, 0x16, 0);
            if (pl->menuCursor % 8 != 7) {
                pl->menuCursor++;
            } else {
                pl->menuCursor -= 7;
            }
        }
        if (buttons & PAD_UP) {
            Sound_PlaySeFlat(0, 0x16, 0);
            if (pl->menuCursor < 8) {
                pl->menuCursor += 32;
            } else {
                pl->menuCursor -= 8;
            }
        }
        if (buttons & PAD_DOWN) {
            Sound_PlaySeFlat(0, 0x16, 0);
            if (pl->menuCursor >= 32) {
                pl->menuCursor -= 32;
            } else {
                pl->menuCursor += 8;
            }
        }
        if (!(scene->supplyTaken[pl->menuCursor / 32] & (1 << (pl->menuCursor % 32))) && SupplyBox_IsOpen(pl->menuCursor) == 1 &&
            (pressed & PAD_CIRCLE) && scene->supply[pl->menuCursor].item != 0 && scene->supply[pl->menuCursor].count != 0 &&
            pl->menuDelay == 0) {
            if (Pouch_Count(pl, scene->supply[pl->menuCursor].item) == 0) {
                if (Pouch_HasFreeSlot(pl) != 0) {
                    Sound_PlaySeFlat(0, 0x19, 0);
                    SupplyBox_Take(pl);
                }
            } else if (Pouch_Room(pl, scene->supply[pl->menuCursor].item) >= scene->supply[pl->menuCursor].count) {
                Sound_PlaySeFlat(0, 0x19, 0);
                SupplyBox_Take(pl);
            }
        }
    }
}

/* A three-item menu (page 0: choose; page 1: confirm). */
void Player_RunDeliveryMenu(Player *pl)
{
    s16 pressed;
    s16 buttons;

    if (Player_IsLocal(pl) != 0) {
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        switch (pl->menuPage) {
        case 0:
            if (pl->menuDelay != 0) {
                pl->menuDelay--;
            }
            if (pl->menuDelay != 0) {
                break;
            }
            if (pressed & PAD_CROSS) {
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                pl->deliveryMenu = 0;
            } else if (pressed & PAD_CIRCLE) {
                switch (pl->menuCursor) {
                case 0:
                    Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    pl->deliveryMenu = 1;
                    break;
                case 1:
                    if (Quest_CheckSubClearMode() == 1 && Quest_PhaseIsOver(0) != 1) {
                        pl->menuPage++;
                        pl->yesNoCursor = 1;
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                    }
                    break;
                case 2:
                    if (Quest_PhaseIsOver(0) != 1) {
                        pl->menuPage++;
                        pl->yesNoCursor = 1;
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                    }
                    break;
                }
            } else {
                if (buttons & PAD_UP) {
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    if (pl->menuCursor <= 0) {
                        pl->menuCursor = 2;
                    } else {
                        pl->menuCursor--;
                    }
                }
                if (buttons & PAD_DOWN) {
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    if (pl->menuCursor >= 2) {
                        pl->menuCursor = 0;
                    } else {
                        pl->menuCursor++;
                    }
                }
            }
            break;
        case 1:
            if (pl->yesNoCursor == 0) {
                if (buttons & PAD_CROSS) {
                    pl->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else if (buttons & PAD_CIRCLE) {
                    switch (pl->menuCursor) {
                    case 1:
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        pl->deliveryMenu = 1;
                        break;
                    case 2:
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                        pl->deliveryMenu = 1;
                        break;
                    }
                } else if (buttons & PAD_RIGHT) {
                    pl->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                }
            } else if (buttons & (PAD_CROSS | PAD_CIRCLE)) {
                pl->menuPage = 0;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            } else if (buttons & PAD_LEFT) {
                pl->yesNoCursor = 0;
                Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            }
            break;
        }
    }
}

/* A menu to take a quantity of an item (page 0: the item, 1: how many,
 * 2: confirm); Quest_GetDeliveryTarget gives the item and count of a choice. */
void Quest_DeliveryMenu(Player *pl)
{
    HudState *menu = &gHudStateBuf;
    s16 pressed;
    s16 buttons;
    ItemStack sel;
    s16 have;
    s16 max;

    if (Player_IsLocal(pl) != 0) {
        if (Quest_PhaseIsOver(0) == 1) {
            Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            pl->deliveryOpen = 0;
            return;
        }
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        switch (pl->menuPage) {
        case 0:
            if (pl->menuDelay != 0) {
                pl->menuDelay--;
            }
            if (pl->menuDelay != 0) {
                return;
            }
            if (pressed & PAD_CROSS) {
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                pl->deliveryOpen = 0;
                return;
            }
            if (pressed & PAD_CIRCLE) {
                if (Quest_GetDeliveryTarget(pl, pl->menuCursor, &sel) == 2) {
                    have = Pouch_Count(pl, sel.item);
                    if (have > 0) {
                        pl->menuPage++;
                        max = sel.count;
                        if (have >= max) {
                            menu->atMax = max;
                        } else {
                            menu->atMax = have;
                        }
                        menu->count = menu->atMax;
                        Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
                    } else {
                        Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                    }
                } else {
                    Sound_PlaySeFlat(0, SE_SYS_REFUSE, 0);
                }
            } else {
                if (buttons & PAD_UP) {
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    if (pl->menuCursor <= 0) {
                        pl->menuCursor = 2;
                    } else {
                        pl->menuCursor--;
                    }
                }
                if (buttons & PAD_DOWN) {
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                    if (pl->menuCursor >= 2) {
                        pl->menuCursor = 0;
                    } else {
                        pl->menuCursor++;
                    }
                }
            }
            break;
        case 1:
            if (pressed & PAD_CROSS) {
                pl->menuPage = 0;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            } else if (pressed & PAD_CIRCLE) {
                pl->menuPage++;
                pl->yesNoCursor = 1;
                Sound_PlaySeFlat(0, SE_SYS_CONFIRM, 0);
            } else if (pressed & PAD_RIGHT) {
                if (menu->count >= menu->atMax) {
                    menu->count = menu->atMax;
                } else {
                    menu->count++;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                }
            } else if (pressed & PAD_LEFT) {
                if (menu->count < 2) {
                    menu->count = 1;
                } else {
                    menu->count--;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                }
            }
            break;
        case 2:
            if (pl->yesNoCursor == 0) {
                if (buttons & PAD_CROSS) {
                    pl->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
                } else if (buttons & PAD_CIRCLE) {
                    Quest_GetDeliveryTarget(pl, pl->menuCursor, &sel);
                    Quest_DeliverItem(pl, sel.item, menu->count);
                    pl->deliveryOpen = 0;
                } else if (buttons & PAD_DOWN) {
                    pl->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
                }
            } else if (buttons & (PAD_CROSS | PAD_CIRCLE)) {
                pl->menuPage = 1;
                Sound_PlaySeFlat(0, SE_SYS_CANCEL, 0);
            } else if (buttons & PAD_UP) {
                pl->yesNoCursor = 0;
                Sound_PlaySeFlat(0, SE_SYS_SCROLL, 0);
            }
            break;
        }
        menu->yesNo = pl->yesNoCursor;
    }
}

/* 1 when a trader (EM_KIND_TRADER) is within 300 of the object. */
s32 Player_NearTrader(void *obj)
{
    ObjHead *head = obj;
    Em *em;
    s16 i;

    for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
        if (em->type == EM_KIND_TRADER && Vec_Distance(head->pos, &em->posX) <= 300.0f) {
            return 1;
        }
    }
    return 0;
}

/* 1 when the pouch holds ITEM_ID_MEAT (the spit roast needs one). */
s32 Pouch_HasMeat(Player *pl)
{
    return Pouch_Count(pl, ITEM_ID_MEAT) != 0;
}

/* The local player's jingle for using an item, by its ItemData.useSound. */
void Item_PlayUseSound(Player *pl, u16 item)
{
    if (Player_IsLocal(pl) != 0) {
        switch (gItemData[item].useSound) {
        case 0:
            Jingle_Play(pl, 7);
            break;
        case 2:
            Jingle_Play(pl, 8);
            break;
        case 3:
            Jingle_Play(pl, 0xB7);
            break;
        }
    }
}
