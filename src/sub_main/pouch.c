/*
 * The player's status: the item pouch (20 slots of {item, count} at
 * Player + 0x77C), the bowgun's ammo selection, health, stamina and
 * sharpness, and two menus driven from here.
 */
#include "common.h"
#include "quest.h"
#include "hud.h"
#include "player.h"
#include "scene.h"
#include "item.h"
#include "save.h"
#include "pad.h"
#include "em.h"
#include "objhead.h"

typedef s16 SharpRow[6];

/* Menu state shared with the drawing code (quantity at 0x55, its maximum
 * at 0x56). */

extern u8 gGestureStates[];
extern u8 gBowgunClipSizes[][32];
extern s16 D_005B396C;
extern Em gEms[];
extern SharpRow *gSharpnessTables[];
extern char *gSharpnessDownText[];
extern s32 gSharpnessUpText;
extern char *D_00446B10[];

s32 Quest_PhaseIsOver(s32 a);
s32 Scene_CanPlaceKind2(void);
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
s32 Player_IsLocal(void *obj);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Item_IsSpecial(u16 item);
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
void Pouch_SelectFirst(Player *p)
{
    s16 i;

    p->pouchCursor = 0;
    for (i = 0; i < 20; i++) {
        if (p->pouch[i].item != 0 && p->pouch[i].count > 0 && (gItemData[p->pouch[i].item].flags & 1)) {
            p->pouchCursor = (u8)i;
            return;
        }
    }
}

/*
 * Adds n of an item to the pouch (n < 0 takes them away). Returns 0 for an
 * invalid item, 5 when there is no free slot, 3 when the pouch already holds
 * the maximum, 4 when the stack is used up, 2 when it was capped at the
 * maximum, else 1. Unless `silent`, the item is recorded with
 * Save_MarkItemObtained.
 */
#pragma optimization_level 4
u8 Pouch_AddEx(Player *p, u16 item, s16 n, u8 silent)
{
    s16 max;
    s16 i;
    u8 ret;

    if (item >= 0x399) {
        return 0;
    }
    if (item == 0x87) {
        return 0;
    }
    if (gItemData[item].max == 0xFF) {
        n = 0xFF;
    }
    if (Pouch_Count(p, item) == 0) {
        ret = 5;
        for (i = 0; i < 20; i++) {
            if (p->pouch[i].item == 0 && n > 0) {
                max = gItemData[item].max;
                p->pouch[i].item = item;
                if (max < n) {
                    n = max;
                }
                p->pouch[i].count = n;
                if (silent == 0) {
                    Save_MarkItemObtained(&gSave, item);
                }
                if (gItemData[item].type == 2) {
                    if (i == p->ammoSlot) {
                        p->ammoSlot = Bowgun_NextAmmoSlot(p, i, 2);
                        Bowgun_LoadAmmo(p, 1);
                        p->savedAmmoCount = p->ammoCount;
                        p->savedClipSize = p->clipSize;
                        p->savedRoundsLoaded = 0;
                    } else if (p->weaponType == 1 || p->weaponType == 5) {
                        if (p->ammoSlot == 0xFF) {
                            p->ammoSlot = Bowgun_NextAmmoSlot(p, 0, 2);
                            Bowgun_LoadAmmo(p, 0);
                        } else if (gItemData[p->pouch[p->ammoSlot].item].type != 2) {
                            p->ammoSlot = Bowgun_NextAmmoSlot(p, p->ammoSlot, 0);
                            Bowgun_LoadAmmo(p, 0);
                        }
                    }
                }
                ret = 0;
                if (!(gItemData[p->pouch[p->pouchCursor].item].flags & 1)) {
                    p->pouchCursor = Pouch_NextSelectable(p, p->pouchCursor, 0);
                }
                break;
            }
        }
    } else if (n > 0 && Item_IsSpecial(item) == 1) {
        ret = 3;
    } else {
        for (i = 0; i < 20; i++) {
            if (p->pouch[i].item == item) {
                max = gItemData[item].max;
                if (n > 0 && p->pouch[i].count >= max) {
                    p->pouch[i].count = max;
                    if (silent == 0) {
                        Save_MarkItemObtained(&gSave, item);
                    }
                    ret = 3;
                } else {
                    if (n < 0 && gItemData[item].max == 0xFF) {
                        ret = 1;
                        break;
                    }
                    p->pouch[i].count += n;
                    if (p->pouch[i].count <= 0) {
                        Pouch_ClearSlot(p, i);
                        ret = 4;
                        p->pouch[i].item = 0;
                        p->pouch[i].count = 0;
                    } else {
                        if (max < p->pouch[i].count) {
                            p->pouch[i].count = max;
                            ret = 2;
                        } else {
                            ret = 1;
                        }
                        if (silent == 0) {
                            Save_MarkItemObtained(&gSave, item);
                        }
                    }
                }
                if (gItemData[item].type == 2 && i == p->ammoSlot) {
                    p->ammoCount = p->pouch[p->ammoSlot].count;
                    p->savedAmmoCount = p->ammoCount;
                    if (p->ammoCount < p->roundsLoaded) {
                        p->roundsLoaded = p->ammoCount;
                        p->savedRoundsLoaded = p->roundsLoaded;
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

u8 Pouch_Add(Player *p, u16 item, s16 n)
{
    return Pouch_AddEx(p, item, n, 0);
}

s32 Item_IsSpecial(u16 item)
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

/* Nonzero if the pouch holds one of the items 0xA1-0xA9 (Item_IsSpecial). */
s32 Pouch_HasSpecial(Player *p)
{
    s16 i;

    for (i = 0; i < 20; i++) {
        if (Item_IsSpecial(p->pouch[i].item) == 1) {
            return 1;
        }
    }
    return 0;
}

/* How many of an item the pouch holds. */
s16 Pouch_Count(Player *p, u16 item)
{
    s16 i;

    if (Item_IsSpecial(item) == 1) {
        return Pouch_HasSpecial(p) == 1;
    }
    for (i = 0; i < 20; i++) {
        if (p->pouch[i].item == item) {
            return p->pouch[i].count;
        }
    }
    return 0;
}

/* How many more of an item the pouch can take in its stack (0xFF: any). */
s16 Pouch_Room(Player *p, u16 item)
{
    s16 i;

    if (Item_IsSpecial(item) == 1) {
        return Pouch_HasSpecial(p) != 1;
    }
    for (i = 0; i < 20; i++) {
        if (p->pouch[i].item == item) {
            if (gItemData[item].max == 0xFF) {
                return 0xFF;
            }
            /* A return in both branches, as the game has it (each jumps to the
             * epilogue): the first probably held a print compiled out. */
            if (p->pouch[i].count > gItemData[item].max) {
                return gItemData[item].max - p->pouch[i].count;
            }
            return gItemData[item].max - p->pouch[i].count;
        }
    }
    return gItemData[item].max;
}

/* As Pouch_Room, and -1 when the item needs a free slot and there is none. */
s16 Pouch_Space(Player *p, u16 item)
{
    s16 i;
    s16 empty = 0;

    if (Item_IsSpecial(item) == 1 && Pouch_HasSpecial(p) == 1) {
        return 0;
    }
    for (i = 0; i < 20; i++) {
        if (p->pouch[i].item == item) {
            if (gItemData[item].max == 0xFF) {
                return 0xFF;
            }
            /* A return in both branches, as the game has it (each jumps to the
             * epilogue): the first probably held a print compiled out. */
            if (p->pouch[i].count > gItemData[item].max) {
                return gItemData[item].max - p->pouch[i].count;
            }
            return gItemData[item].max - p->pouch[i].count;
        }
        if (p->pouch[i].item == 0) {
            empty++;
        }
    }
    if (empty == 0) {
        return -1;
    }
    return gItemData[item].max;
}

/* The pouch slot of an item, 0xFF if none. */
s16 Pouch_FindSlot(Player *p, u16 item)
{
    s16 i;

    for (i = 0; i < 20; i++) {
        if (p->pouch[i].item == item) {
            return i;
        }
    }
    return 0xFF;
}

/* The count in the selected pouch slot. */
s16 Pouch_CursorCount(Player *p)
{
    return p->pouch[p->pouchCursor].count;
}

/* The number of pouch slots with a selectable item. */
s16 Pouch_CountSelectable(Player *p)
{
    s16 n;
    s16 i;

    i = 0;
    n = 0;
    for (; i < 20; i++) {
        if (p->pouch[i].count > 0) {
            s16 item = p->pouch[i].item;

            if (item != 0 && (gItemData[item].flags & 1)) {
                n++;
            }
        }
    }
    return n;
}

/* Nonzero if the pouch has a free slot. */
s16 Pouch_HasFreeSlot(Player *p)
{
    s16 i;

    for (i = 0; i < 20; i++) {
        if (p->pouch[i].item == 0) {
            return 1;
        }
    }
    return 0;
}

/* Empties a pouch slot, and reselects the bowgun's ammo if it was loaded. */
s16 Pouch_ClearSlot(Player *p, s16 slot)
{
    u16 item;

    if (slot >= 20) {
        return 2;
    }
    item = p->pouch[slot].item;
    p->pouch[slot].count = 0;
    if (p->pouch[slot].item == 0) {
        return 0;
    }
    p->pouch[slot].item = 0;
    if (gItemData[item].type == 2) {
        if ((u8)Bowgun_NextAmmoSlot(p, p->ammoSlot, 1) == 0xFF) {
            if (p->weaponType != 9 && p->weaponType != 10) {
                p->ammoSlot = 0xFF;
                p->ammoCount = 0;
                p->clipSize = 0;
                p->roundsLoaded = 0;
            }
        } else if (slot == p->ammoSlot) {
            p->ammoCount = 0;
            p->clipSize = 0;
            p->roundsLoaded = 0;
        }
    } else if (gItemData[item].type == 3 && slot == p->ammoSlot) {
        Bow_ClearCoatingSlot(p);
        p->ammoSlot = Bowgun_NextAmmoSlot(p, slot, 0);
        Bowgun_LoadAmmo(p, 0);
    }
    return 1;
}

/* Sets the count of an item to at least n (mode 1: exactly n). */
s32 Pouch_SetAtLeast(Player *p, u16 mode, u16 item, s16 n)
{
    s16 have = Pouch_Count(p, item);

    if (have < n) {
        Pouch_AddEx(p, item, n - have, 1);
        return 1;
    }
    if (mode == 1) {
        Pouch_AddEx(p, item, n - have, 1);
        return 2;
    }
    return 3;
}

void Player_TwistBone_Unk(Player *p, s16 kind)
{
    f32 *h;
    f32 f;

    switch (kind) {
    case 9:
        f = 0.3f * ((6.2831855f * p->lookYaw) / 65536.0f);
        h = Em_BoneMatrix(p, 9);
        Mtx_RotateY(h, f);
        Mtx_RotateZ(h, (6.2831855f * p->lean) / 65536.0f);
        break;
    case 10:
        f = 0.3f * ((6.2831855f * p->lookYaw) / 65536.0f);
        Mtx_RotateY(Em_BoneMatrix(p, 10), f);
        break;
    case 19:
        f = 0.3f * ((6.2831855f * p->lookYaw) / 65536.0f);
        Mtx_RotateY(Em_BoneMatrix(p, 19), f);
        break;
    case 20:
        f = 0.1f * ((6.2831855f * p->lookYaw) / 65536.0f);
        Mtx_RotateY(Em_BoneMatrix(p, 20), f);
        break;
    }
}

/* Moves the player to unk754 over `frames` frames, or at once when it is
 * 800 or more away. */
void Player_StartMoveTo(Player *p, s16 frames)
{
    if (!(Vec_Distance(p->pos, p->moveTo) < 800.0f)) {
        p->moveToFrames = 0;
        p->pos[0] = p->moveTo[0];
        p->pos[1] = p->moveTo[1];
        p->pos[2] = p->moveTo[2];
        return;
    }
    p->moveToFrames = frames;
    p->moveToStep[0] = (p->moveTo[0] - p->pos[0]) / frames;
    p->moveToStep[1] = (p->moveTo[1] - p->pos[1]) / frames;
    p->moveToStep[2] = (p->moveTo[2] - p->pos[2]) / frames;
}

void Player_UpdateMoveTo(Player *p)
{
    s16 n = p->moveToFrames;

    if (n != 0) {
        p->moveToFrames = n - 1;
        if (n > 0) {
            p->pos[0] += p->moveToStep[0];
            p->pos[1] += p->moveToStep[1];
            p->pos[2] += p->moveToStep[2];
        }
    }
}

s16 Player_GetAreaAmbModel(Player *p)
{
    if (Scene_CanPlaceKind2() == 0) {
        return -1;
    }
    return gAreaAmbModels[p->area][0];
}

s32 Player_IgnoresFlinch(Player *p)
{
    if (p->noFlinchTimer != 0 || p->noFlinchItemTimer != 0 || p->demonMode != 0) {
        return 1;
    }
    return 0;
}

void Stats_CountItemUse(Player *p)
{
    gScene.itemUses[p->index]++;
}

/* Adds n to the health, within [0, hpMax]; the recoverable part follows it up. */
void Player_AddHp(Player *p, s16 n)
{
    if (Player_IsLocal(p) != 0) {
        if (Quest_PhaseIsOver(0) == 1 && n < 0) {
            return;
        }
        if (p->stateGroup != 3) {
            p->hp += n;
            if (p->hp <= 0) {
                p->hp = 0;
            }
            if (p->hp >= p->hpMax) {
                p->hp = p->hpMax;
            }
            if (p->hp >= p->hpRecoverable) {
                p->hpRecoverable = p->hp;
            }
        }
    }
}

/* Takes damage; `lose` also drops the recoverable health. Returns 1 when the
 * player died (Player_Die). */
s32 Player_Damage(Player *p, s16 damage, s16 lose)
{
    if (Player_IsLocal(p) == 0) {
        return 0;
    }
    Player_AddHp(p, -damage);
    if (lose != 0) {
        p->hpRecoverable = p->hp;
    }
    if (p->hp <= 0) {
        p->hpRecoverable = 0;
        if (p->action != 2) {
            Player_Die(p);
            return 1;
        }
        if (Player_IsState(p, 0, 9) == 0) {
            Player_SetState(p, 0, 9, 0);
        }
    }
    return 0;
}

/* Heals n, a quarter more with skill 0xA3 and a quarter less with 0xA4;
 * QuestStats_Add(2, 1) unless `quiet`. */
void Player_Heal(Player *p, s16 n, s16 quiet)
{
    s16 m;

    if (Player_HasSkill(p, 0xA3) == 1 && n > 0) {
        m = n + n / 4;
    } else if (Player_HasSkill(p, 0xA4) == 1 && n > 0) {
        m = n - n / 4;
    } else {
        m = n;
    }
    Player_AddHp(p, m);
    if (quiet == 0 && Player_IsLocal(p) == 1) {
        QuestStats_Add(2, 1);
    }
}

/* Changes the maximum health (1..150); unless `temporary`, the saved one
 * (unk8FC) too. */
void Player_AddMaxHp(Player *p, s16 n, u8 temporary)
{
    if (Player_IsLocal(p) != 0) {
        p->hpMax += n;
        if (p->hpMax < 2) {
            p->hpMax = 1;
        }
        if (p->hpMax >= 150) {
            p->hpMax = 150;
        }
        if (p->hp >= p->hpMax) {
            p->hp = p->hpMax;
            D_005B396C = p->hp;
        }
        if (p->hpRecoverable >= p->hpMax) {
            p->hpRecoverable = p->hpMax;
        }
        if (temporary == 0) {
            p->hpMaxBase += n;
            if (p->hpMaxBase < 2) {
                p->hpMaxBase = 1;
            }
            if (p->hpMaxBase >= 150) {
                p->hpMaxBase = 150;
            }
        }
    }
}

/* The sharpness colour (0-5) of the player's weapon: the thresholds come
 * from a per-weapon-class table, row by the weapon's sharpness type. */
/* Round 21: the class's table read as s16 * and stepped by the row (as EquipMenu_DrawSharpness reads it). */
u8 Player_GetSharpnessLevel(Player *p)
{
    s16 *t;
    u8 level;

    if (p->isGunner == 1) {
        level = 0;
    } else {
        t = (s16 *)gSharpnessTables[p->weaponType];
        t += gBladeData[p->equip[6].id].sharpType * 6;
        if (p->sharpness <= t[0]) {
            level = 0;
        } else if (p->sharpness <= t[1]) {
            level = 1;
        } else if (p->sharpness <= t[2]) {
            level = 2;
        } else if (p->sharpness <= t[3]) {
            level = 3;
        } else if (p->sharpness <= t[4]) {
            level = 4;
        } else {
            level = 5;
        }
    }
    return level;
}

/* Changes the sharpness (losses halved with skill 0x1F, doubled with
 * 0x20), and plays the sounds of a colour change and of full sharpness. */
void Player_AddSharpness(Player *p, s16 n)
{
    u8 full = 0;
    u8 level;

    if (Player_IsLocal(p) != 0 && p->isGunner != 1) {
        if (n < 0) {
            if (Player_HasSkill(p, 0x1F) == 1) {
                n = n / 2;
                if (n == 0) {
                    n = -1;
                }
            } else if (Player_HasSkill(p, 0x20) == 1) {
                n = n * 2;
            }
        }
        p->sharpness += n;
        if (p->sharpness <= 0) {
            p->sharpness = 0;
        }
        if (p->sharpnessMax < p->sharpness) {
            p->sharpness = p->sharpnessMax;
            full = 1;
        }
        level = Player_GetSharpnessLevel(p);
        if (level != p->sharpnessLevel) {
            if (level < p->sharpnessLevel) {
                Notice_PushItem(gSharpnessDownText[0]);
            } else {
                Notice_PushItem((char *) gSharpnessUpText);
            }
            p->sharpnessLevel = level;
        }
        if (full) {
            Notice_PushItem(D_00446B10[0]);
        }
    }
}

/*
 * The next pouch slot from `slot` holding ammo the bowgun can load, 0xFF if
 * none (not a gunner, or no bowgun). dir 0 and 1 step forward and back
 * first; 2 and 3 start at `slot` itself. Bowgun kind 10 uses flag 4 and 21
 * slots, and also takes item 0x87.
 */
u16 Bowgun_NextAmmoSlot(Player *p, u16 slot, u8 dir)
{
    s16 n;
    u32 mask;
    s16 i;
    s32 flag;

    if (p->isGunner != 1) {
        return 0xFF;
    }
    if (p->equip[6].type != 7) {
        return 0xFF;
    }
    if (p->weaponType == 10) {
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
    mask = Bowgun_GetAmmoMask(p);
    if (p->weaponType == 10) {
        flag = 4;
    } else {
        flag = 2;
    }
    for (i = 0; i < n; i++) {
        if (p->pouch[slot].item != 0 && p->pouch[slot].count > 0 &&
            (flag & gItemData[p->pouch[slot].item].flags)) {
            if (flag == 4 && p->pouch[slot].item == 0x87) {
                return slot;
            }
            if (mask & (1 << gItemData[p->pouch[slot].item].ammoKind)) {
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
    return 0xFF;
}

void Bow_SetCoatingSlot(Player *p)
{
    p->coatingSlot = p->ammoSlot;
}

void Bow_ClearCoatingSlot(Player *p)
{
    p->coatingSlot = 0xFF;
}

/* Loads the ammo of the selected slot: its kind, the rounds left, and the
 * clip size for this bowgun (+1 with skill 0xB8). */
void Bowgun_LoadAmmo(Player *p, s32 unused)
{
    if (p->equip[6].type == 7 && p->ammoSlot != 0xFF) {
        p->ammoType = gItemData[p->pouch[p->ammoSlot].item].ammoKind;
        if (gItemData[p->pouch[p->ammoSlot].item].max == 0xFF) {
            p->ammoCount = 0xFF;
        } else {
            p->ammoCount = p->pouch[p->ammoSlot].count;
        }
        p->clipSize = gBowgunClipSizes[gGunData[p->equip[6].id].clipRow][p->ammoType];
        if (Player_HasSkill(p, 0xB8) == 1) {
            p->clipSize++;
        }
        p->roundsLoaded = 0;
        if (p->weaponType == 10) {
            Bow_ClearCoatingSlot(p);
        }
    }
}

/* The ammo kinds the player can load: the bowgun's own (weapon data), plus
 * those the skills 0x39-0x46 add (not for bowgun kind 10). */
u32 Bowgun_GetAmmoMask(Player *p)
{
    u32 mask;

    if (p->isGunner != 1) {
        return 0;
    }
    mask = gGunData[p->equip[6].id].ammoMask;
    if (p->weaponType == 10) {
        return mask;
    }
    if (Player_HasSkill(p, 0x39) == 1) {
        mask |= 1;
    } else if (Player_HasSkill(p, 0x3A) == 1) {
        mask |= 7;
    }
    if (Player_HasSkill(p, 0x3B) == 1) {
        mask |= 8;
    } else if (Player_HasSkill(p, 0x3C) == 1) {
        mask |= 0x18;
    } else if (Player_HasSkill(p, 0x3D) == 1) {
        mask |= 0x38;
    }
    if (Player_HasSkill(p, 0x3E) == 1) {
        mask |= 0x40;
    } else if (Player_HasSkill(p, 0x3F) == 1) {
        mask |= 0xC0;
    } else if (Player_HasSkill(p, 0x40) == 1) {
        mask |= 0x1C0;
    }
    if (Player_HasSkill(p, 0x41) == 1) {
        mask |= 0x200;
    } else if (Player_HasSkill(p, 0x42) == 1) {
        mask |= 0x600;
    } else if (Player_HasSkill(p, 0x43) == 1) {
        mask |= 0xE00;
    }
    if (Player_HasSkill(p, 0x44) == 1) {
        mask |= 0x1000;
    } else if (Player_HasSkill(p, 0x45) == 1) {
        mask |= 0x3000;
    } else if (Player_HasSkill(p, 0x46) == 1) {
        mask |= 0x7000;
    }
    if (Quest_IsVersus() == 1) {
        mask &= 0xEFFFFFFF;
    }
    return mask;
}

/* The ammo kinds the skills 0x39-0x46 let the player load (a mask). */
s32 Player_GetSkillAmmoMask(Player *p)
{
    s32 mask = 0;

    if (Player_HasSkill(p, 0x39) == 1) {
        mask |= 1;
    } else if (Player_HasSkill(p, 0x3A) == 1) {
        mask |= 7;
    }
    if (Player_HasSkill(p, 0x3B) == 1) {
        mask |= 8;
    } else if (Player_HasSkill(p, 0x3C) == 1) {
        mask |= 0x18;
    } else if (Player_HasSkill(p, 0x3D) == 1) {
        mask |= 0x38;
    }
    if (Player_HasSkill(p, 0x3E) == 1) {
        mask |= 0x40;
    } else if (Player_HasSkill(p, 0x3F) == 1) {
        mask |= 0xC0;
    } else if (Player_HasSkill(p, 0x40) == 1) {
        mask |= 0x1C0;
    }
    if (Player_HasSkill(p, 0x41) == 1) {
        mask |= 0x200;
    } else if (Player_HasSkill(p, 0x42) == 1) {
        mask |= 0x600;
    } else if (Player_HasSkill(p, 0x43) == 1) {
        mask |= 0xE00;
    }
    if (Player_HasSkill(p, 0x44) == 1) {
        mask |= 0x1000;
    } else if (Player_HasSkill(p, 0x45) == 1) {
        mask |= 0x3000;
    } else if (Player_HasSkill(p, 0x46) == 1) {
        mask |= 0x7000;
    }
    return mask;
}

u8 Bowgun_ClipSize(GunData *gun, u8 kind)
{
    return gBowgunClipSizes[gun->clipRow][kind];
}

void Player_TickWeaponTimer(Player *p)
{
    if (p->weaponTimer < 1000) {
        p->weaponTimer++;
        if (p->weaponTimer == 60) {
            Player_PlaySe(p, 9, 0, p->pos, 1, 0);
        }
    } else {
        p->weaponTimer = 1000;
    }
}

/* Adds n to the stamina, within [0, staminaMax]; losses are ignored when
 * Player_StaminaLocked says so. */
void Player_AddStamina(Player *p, s16 n)
{
    if (Player_IsLocal(p) != 0) {
        if (n < 0 && Player_StaminaLocked(p) == 1) {
            return;
        }
        p->stamina += n;
        if (p->stamina <= 0) {
            p->stamina = 0;
        }
        if (p->stamina >= p->staminaMax) {
            p->stamina = p->staminaMax;
        }
    }
}

/* Uses stamina: skill 0xB6 halves the cost (a cost of 1 every other frame),
 * skill 0xB7 doubles it every fifth frame. */
void Player_UseStamina(Player *p, s32 n)
{
    s16 cost;

    if (Player_IsLocal(p) != 0) {
        cost = n;
        if (cost < 0) {
            if (cost == -1) {
                if (Player_HasSkill(p, 0xB6) == 1 && (gScene.frame & 1)) {
                    return;
                }
            } else if (Player_HasSkill(p, 0xB6) == 1) {
                cost = cost / 2;
            }
            if (Player_HasSkill(p, 0xB7) == 1 && gScene.frame % 5 == 0) {
                cost *= 2;
            }
        }
        Player_AddStamina(p, cost);
    }
}

/* Changes the maximum stamina (75..450; reaching 450 restarts the timer). */
void Player_AddMaxStamina(Player *p, s16 n)
{
    if (Player_IsLocal(p) != 0) {
        if (n < 0 && Player_StaminaLocked(p) == 1) {
            return;
        }
        p->staminaMax += n;
        if (p->staminaMax < 76) {
            p->staminaMax = 75;
        } else if (p->staminaMax > 450) {
            p->staminaMax = 450;
            p->staminaTimer = 10800;
        }
        if (p->staminaMax < p->stamina) {
            p->stamina = p->staminaMax;
        }
    }
}

/*
 * Counts down to the next drop of the maximum stamina (75 every 10800
 * steps): 1 step a frame, more in hot (2) and cold (4) areas unless a drink
 * (unk872) or a skill (0x89-0x8D) protects, and the hunger skills 0x49-0x4C
 * change the pace.
 */
void Player_UpdateHunger(Player *p)
{
    s16 rate;
    s16 zone;

    if (Player_IsLocal(p) != 0) {
        zone = Area_Temperature(p->area);
        rate = 0;
        if (zone == 2 || zone == 4) {
            if (p->hotDrinkTimer == 0) {
                if ((zone != 2 || (Player_HasSkill(p, 0x8A) != 1 && Player_HasSkill(p, 0x8B) != 1)) &&
                    (zone != 4 || Player_HasSkill(p, 0x8B) != 1)) {
                    if (zone == 2) {
                        if (Player_HasSkill(p, 0x89) == 1) {
                            rate = 1;
                        } else if (Player_HasSkill(p, 0x8C) == 1) {
                            rate = 3;
                        } else if (Player_HasSkill(p, 0x8D) == 1) {
                            rate = 4;
                        } else {
                            rate = 2;
                        }
                    } else if (Player_HasSkill(p, 0x89) == 1) {
                        if (gScene.frame & 1) {
                            rate = 1;
                        } else {
                            rate = 2;
                        }
                    } else if (Player_HasSkill(p, 0x8A) == 1) {
                        rate = 1;
                    } else if (Player_HasSkill(p, 0x8C) == 1) {
                        rate = 4;
                    } else if (Player_HasSkill(p, 0x8D) == 1) {
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
        if (!Player_HasSkill(p, 0x4A)) {
            if (!Player_HasSkill(p, 0x49) || (gScene.frame & 1)) {
                rate++;
            }
            if (Player_HasSkill(p, 0x4C) == 1) {
                rate++;
            } else if (Player_HasSkill(p, 0x4B) == 1 && !(gScene.frame & 1)) {
                rate++;
            }
        }
        if (rate > 0) {
            p->staminaTimer -= rate;
            if (p->staminaTimer <= 0) {
                p->staminaTimer = 10800;
                Player_AddMaxStamina(p, -75);
            }
        }
    }
}

void Player_RequestAct(u8 a)
{
    Player *p = &gPlayers[gScene.localPlayer];

    p->actRequest = a;
}

void Player_StartAct(Player *p)
{
    if (p->actRequest != 0) {
        Player_SetState(p, 6, gGestureStates[p->actRequest], 0);
    }
}

/* Takes the item under the supply box cursor (or asks the host, online). */
void SupplyBox_Take(Player *p)
{
    SceneState *scene = &gScene;
    u16 item;

    p->menuDelay = 30;
    Hud_StartSupplyMsg((u8 *)p);
    if (Game_IsOnline() == 1) {
        p->supplyBoxTimer = 900;
        p->supplyTaken = 1;
        QSync_SendSupply(1, gScene.localPlayer);
        return;
    }
    p->supplyBoxTimer = 0;
    p->supplyTaken = 0;
    Pouch_Add(p, scene->supply[p->menuCursor].item, scene->supply[p->menuCursor].count);
    scene->supplyTaken[p->menuCursor / 32] |= 1 << (p->menuCursor % 32);
    SupplyBox_StartMove(scene->supply[p->menuCursor].item, p->menuCursor);
    item = scene->supply[p->menuCursor].item;
    switch (item) {
    case 0x2A4:
        Notice_Push(1, 0x13, item);
        break;
    case 0x2A6:
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
void SupplyBox_Update(Player *p)
{
    s16 pressed;
    SceneState *scene = &gScene;
    s16 buttons;

    if (Player_IsLocal(p) != 0) {
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        if (p->menuDelay != 0) {
            p->menuDelay--;
        }
        if (p->supplyBoxTimer != 0) {
            p->supplyBoxTimer--;
            if (p->supplyBoxTimer == 0) {
                p->supplyTaken = 0;
            }
        }
        if (p->supplyTaken != 0 || p->menuDelay != 0) {
            return;
        }
        if (buttons & 0x40) {
            Sound_PlaySeFlat(0, 0x14, 0);
            p->supplyBoxOpen = 0;
            MsgEvent_Check(2);
            return;
        }
        if (buttons & 0x800) {
            Sound_PlaySeFlat(0, 0x16, 0);
            if (p->menuCursor & 7) {
                p->menuCursor--;
            } else {
                p->menuCursor += 7;
            }
        }
        if (buttons & 0x400) {
            Sound_PlaySeFlat(0, 0x16, 0);
            if (p->menuCursor % 8 != 7) {
                p->menuCursor++;
            } else {
                p->menuCursor -= 7;
            }
        }
        if (buttons & 0x2000) {
            Sound_PlaySeFlat(0, 0x16, 0);
            if (p->menuCursor < 8) {
                p->menuCursor += 32;
            } else {
                p->menuCursor -= 8;
            }
        }
        if (buttons & 0x1000) {
            Sound_PlaySeFlat(0, 0x16, 0);
            if (p->menuCursor >= 32) {
                p->menuCursor -= 32;
            } else {
                p->menuCursor += 8;
            }
        }
        if (!(scene->supplyTaken[p->menuCursor / 32] & (1 << (p->menuCursor % 32))) && SupplyBox_IsOpen(p->menuCursor) == 1 &&
            (pressed & 0x20) && scene->supply[p->menuCursor].item != 0 && scene->supply[p->menuCursor].count != 0 &&
            p->menuDelay == 0) {
            if (Pouch_Count(p, scene->supply[p->menuCursor].item) == 0) {
                if (Pouch_HasFreeSlot(p) != 0) {
                    Sound_PlaySeFlat(0, 0x19, 0);
                    SupplyBox_Take(p);
                }
            } else if (Pouch_Room(p, scene->supply[p->menuCursor].item) >= scene->supply[p->menuCursor].count) {
                Sound_PlaySeFlat(0, 0x19, 0);
                SupplyBox_Take(p);
            }
        }
    }
}

/* A three-item menu (page 0: choose; page 1: confirm). */
void Player_RunDeliveryMenu(Player *p)
{
    s16 pressed;
    s16 buttons;

    if (Player_IsLocal(p) != 0) {
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        switch (p->menuPage) {
        case 0:
            if (p->menuDelay != 0) {
                p->menuDelay--;
            }
            if (p->menuDelay != 0) {
                break;
            }
            if (pressed & 0x40) {
                Sound_PlaySeFlat(0, 0x14, 0);
                p->deliveryMenu = 0;
            } else if (pressed & 0x20) {
                switch (p->menuCursor) {
                case 0:
                    Sound_PlaySeFlat(0, 0x13, 0);
                    p->deliveryMenu = 1;
                    break;
                case 1:
                    if (Quest_CheckSubClearMode() == 1 && Quest_PhaseIsOver(0) != 1) {
                        p->menuPage++;
                        p->yesNoCursor = 1;
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                    break;
                case 2:
                    if (Quest_PhaseIsOver(0) != 1) {
                        p->menuPage++;
                        p->yesNoCursor = 1;
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                    break;
                }
            } else {
                if (buttons & 0x2000) {
                    Sound_PlaySeFlat(0, 0x17, 0);
                    if (p->menuCursor <= 0) {
                        p->menuCursor = 2;
                    } else {
                        p->menuCursor--;
                    }
                }
                if (buttons & 0x1000) {
                    Sound_PlaySeFlat(0, 0x17, 0);
                    if (p->menuCursor >= 2) {
                        p->menuCursor = 0;
                    } else {
                        p->menuCursor++;
                    }
                }
            }
            break;
        case 1:
            if (p->yesNoCursor == 0) {
                if (buttons & 0x40) {
                    p->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else if (buttons & 0x20) {
                    switch (p->menuCursor) {
                    case 1:
                        Sound_PlaySeFlat(0, 0x13, 0);
                        p->deliveryMenu = 1;
                        break;
                    case 2:
                        Sound_PlaySeFlat(0, 0x13, 0);
                        p->deliveryMenu = 1;
                        break;
                    }
                } else if (buttons & 0x400) {
                    p->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            } else if (buttons & 0x60) {
                p->menuPage = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else if (buttons & 0x800) {
                p->yesNoCursor = 0;
                Sound_PlaySeFlat(0, 0x17, 0);
            }
            break;
        }
    }
}

/* A menu to take a quantity of an item (page 0: the item, 1: how many,
 * 2: confirm); Quest_GetDeliveryTarget gives the item and count of a choice. */
void Quest_DeliveryMenu(Player *p)
{
    HudState *menu = &gHudStateBuf;
    s16 pressed;
    s16 buttons;
    ItemStack sel;
    s16 have;
    s16 max;

    if (Player_IsLocal(p) != 0) {
        if (Quest_PhaseIsOver(0) == 1) {
            Sound_PlaySeFlat(0, 0x14, 0);
            p->deliveryOpen = 0;
            return;
        }
        pressed = gPadState[0].pressed;
        buttons = gPadState[0].pressed | gPadState[0].repeat;
        switch (p->menuPage) {
        case 0:
            if (p->menuDelay != 0) {
                p->menuDelay--;
            }
            if (p->menuDelay != 0) {
                return;
            }
            if (pressed & 0x40) {
                Sound_PlaySeFlat(0, 0x14, 0);
                p->deliveryOpen = 0;
                return;
            }
            if (pressed & 0x20) {
                if (Quest_GetDeliveryTarget(p, p->menuCursor, &sel) == 2) {
                    have = Pouch_Count(p, sel.item);
                    if (have > 0) {
                        p->menuPage++;
                        max = sel.count;
                        if (have >= max) {
                            menu->atMax = max;
                        } else {
                            menu->atMax = have;
                        }
                        menu->count = menu->atMax;
                        Sound_PlaySeFlat(0, 0x13, 0);
                    } else {
                        Sound_PlaySeFlat(0, 0x15, 0);
                    }
                } else {
                    Sound_PlaySeFlat(0, 0x15, 0);
                }
            } else {
                if (buttons & 0x2000) {
                    Sound_PlaySeFlat(0, 0x17, 0);
                    if (p->menuCursor <= 0) {
                        p->menuCursor = 2;
                    } else {
                        p->menuCursor--;
                    }
                }
                if (buttons & 0x1000) {
                    Sound_PlaySeFlat(0, 0x17, 0);
                    if (p->menuCursor >= 2) {
                        p->menuCursor = 0;
                    } else {
                        p->menuCursor++;
                    }
                }
            }
            break;
        case 1:
            if (pressed & 0x40) {
                p->menuPage = 0;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else if (pressed & 0x20) {
                p->menuPage++;
                p->yesNoCursor = 1;
                Sound_PlaySeFlat(0, 0x13, 0);
            } else if (pressed & 0x400) {
                if (menu->count >= menu->atMax) {
                    menu->count = menu->atMax;
                } else {
                    menu->count++;
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            } else if (pressed & 0x800) {
                if (menu->count < 2) {
                    menu->count = 1;
                } else {
                    menu->count--;
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            }
            break;
        case 2:
            if (p->yesNoCursor == 0) {
                if (buttons & 0x40) {
                    p->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, 0x14, 0);
                } else if (buttons & 0x20) {
                    Quest_GetDeliveryTarget(p, p->menuCursor, &sel);
                    Quest_DeliverItem(p, sel.item, menu->count);
                    p->deliveryOpen = 0;
                } else if (buttons & 0x1000) {
                    p->yesNoCursor = 1;
                    Sound_PlaySeFlat(0, 0x17, 0);
                }
            } else if (buttons & 0x60) {
                p->menuPage = 1;
                Sound_PlaySeFlat(0, 0x14, 0);
            } else if (buttons & 0x2000) {
                p->yesNoCursor = 0;
                Sound_PlaySeFlat(0, 0x17, 0);
            }
            break;
        }
        menu->yesNo = p->yesNoCursor;
    }
}

/* Nonzero when a monster of kind 10 is within 300 of the player. */
s32 Player_NearEmKind10(void *obj)
{
    ObjHead *o = obj;
    Em *em;
    s16 i;

    for (i = 0, em = gEms; i < 20; i++, em++) {
        if (em->type == 10 && Vec_Distance(o->pos, &em->posX) <= 300.0f) {
            return 1;
        }
    }
    return 0;
}

s32 Pouch_HasItem28(Player *p)
{
    return Pouch_Count(p, 0x28) != 0;
}

/* The sound of using an item, by its ItemData.unk0A. */
void Item_PlayUseSound(Player *p, u16 item)
{
    if (Player_IsLocal(p) != 0) {
        switch (gItemData[item].useSound) {
        case 0:
            Jingle_Play(p, 7);
            break;
        case 2:
            Jingle_Play(p, 8);
            break;
        case 3:
            Jingle_Play(p, 0xB7);
            break;
        }
    }
}
