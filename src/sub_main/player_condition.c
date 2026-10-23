/*
 * The player's condition: the health (with its recoverable part and its
 * maximum), the weapon's sharpness, the bowgun's ammo and the bow's coating
 * slots, the stamina and its maximum, hunger, and the gestures. Before
 * them, a few small player helpers: the look's twist of the head and body
 * bones, a timed move to a point, the area's pitfall model, whether the
 * player ignores a flinch, and counting an item use. Item_Use, the
 * player's states (in the game overlay) and the HUD use these.
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
#include "place_rules.h"
#include "card.h"
#include "qsync.h"
#include "engine_math.h"
#include "game_util.h"
extern u8 gGestureStates[];
extern u8 gBowgunClipSizes[][32];
extern s16 D_005B396C;
extern s32 gSharpnessUpText;
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_Die(Player *p); /* unit's own declaration: the game's takes Em * */
void QuestStats_Add(s32 a, s32 b);
/* end of prototypes */

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

/* Heals n, a quarter more with SKILL_HEAL_UP and a quarter less with SKILL_HEAL_DOWN;
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

/* Changes the sharpness (losses halved with SKILL_SHARPNESS_LOSS_HALF, doubled with
 * SKILL_SHARPNESS_LOSS_DOUBLE), and plays the sounds of a colour change and of full sharpness. */
void Player_AddSharpness(Player *pl, s16 n)
{
    u8 full = 0;
    u8 level;

    if (Player_IsLocal(pl) != 0 && pl->isGunner != 1) {
        if (n < 0) {
            if (Player_HasSkill(pl, SKILL_SHARPNESS_LOSS_HALF) == 1) {
                n = n / 2;
                if (n == 0) {
                    n = -1;
                }
            } else if (Player_HasSkill(pl, SKILL_SHARPNESS_LOSS_DOUBLE) == 1) {
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
 * slots, and also takes item 135.
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
            if (flag == 4 && pl->pouch[slot].item == 135) {
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
 * those the skills 57-70 add (not for bowgun kind 10). */
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
    if (Player_HasSkill(pl, 57) == 1) {
        mask |= 1;
    } else if (Player_HasSkill(pl, 58) == 1) {
        mask |= 7;
    }
    if (Player_HasSkill(pl, 59) == 1) {
        mask |= 8;
    } else if (Player_HasSkill(pl, 60) == 1) {
        mask |= 0x18;
    } else if (Player_HasSkill(pl, 61) == 1) {
        mask |= 0x38;
    }
    if (Player_HasSkill(pl, 62) == 1) {
        mask |= 0x40;
    } else if (Player_HasSkill(pl, 63) == 1) {
        mask |= 0xC0;
    } else if (Player_HasSkill(pl, 64) == 1) {
        mask |= 0x1C0;
    }
    if (Player_HasSkill(pl, 65) == 1) {
        mask |= 0x200;
    } else if (Player_HasSkill(pl, 66) == 1) {
        mask |= 0x600;
    } else if (Player_HasSkill(pl, 67) == 1) {
        mask |= 0xE00;
    }
    if (Player_HasSkill(pl, 68) == 1) {
        mask |= 0x1000;
    } else if (Player_HasSkill(pl, 69) == 1) {
        mask |= 0x3000;
    } else if (Player_HasSkill(pl, 70) == 1) {
        mask |= 0x7000;
    }
    if (Quest_IsVersus() == 1) {
        mask &= 0xEFFFFFFF;
    }
    return mask;
}

/* The ammo kinds the skills 57-70 let the player load (a mask). */
s32 Player_GetSkillAmmoMask(Player *pl)
{
    s32 mask = 0;

    if (Player_HasSkill(pl, 57) == 1) {
        mask |= 1;
    } else if (Player_HasSkill(pl, 58) == 1) {
        mask |= 7;
    }
    if (Player_HasSkill(pl, 59) == 1) {
        mask |= 8;
    } else if (Player_HasSkill(pl, 60) == 1) {
        mask |= 0x18;
    } else if (Player_HasSkill(pl, 61) == 1) {
        mask |= 0x38;
    }
    if (Player_HasSkill(pl, 62) == 1) {
        mask |= 0x40;
    } else if (Player_HasSkill(pl, 63) == 1) {
        mask |= 0xC0;
    } else if (Player_HasSkill(pl, 64) == 1) {
        mask |= 0x1C0;
    }
    if (Player_HasSkill(pl, 65) == 1) {
        mask |= 0x200;
    } else if (Player_HasSkill(pl, 66) == 1) {
        mask |= 0x600;
    } else if (Player_HasSkill(pl, 67) == 1) {
        mask |= 0xE00;
    }
    if (Player_HasSkill(pl, 68) == 1) {
        mask |= 0x1000;
    } else if (Player_HasSkill(pl, 69) == 1) {
        mask |= 0x3000;
    } else if (Player_HasSkill(pl, 70) == 1) {
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

/* Uses stamina: SKILL_STAMINA_COST_DOWN halves the cost (a cost of 1 every other frame),
 * SKILL_STAMINA_COST_UP doubles it every fifth frame. */
void Player_UseStamina(Player *pl, s32 n)
{
    s16 cost;

    if (Player_IsLocal(pl) != 0) {
        cost = n;
        if (cost < 0) {
            if (cost == -1) {
                if (Player_HasSkill(pl, SKILL_STAMINA_COST_DOWN) == 1 && (gScene.frame & 1)) {
                    return;
                }
            } else if (Player_HasSkill(pl, SKILL_STAMINA_COST_DOWN) == 1) {
                cost = cost / 2;
            }
            if (Player_HasSkill(pl, SKILL_STAMINA_COST_UP) == 1 && gScene.frame % 5 == 0) {
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
 * (unk872) or a skill (137-141) protects, and the hunger skills 73-76
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
                        if (Player_HasSkill(pl, 137) == 1) {
                            rate = 1;
                        } else if (Player_HasSkill(pl, 140) == 1) {
                            rate = 3;
                        } else if (Player_HasSkill(pl, 141) == 1) {
                            rate = 4;
                        } else {
                            rate = 2;
                        }
                    } else if (Player_HasSkill(pl, 137) == 1) {
                        if (gScene.frame & 1) {
                            rate = 1;
                        } else {
                            rate = 2;
                        }
                    } else if (Player_HasSkill(pl, SKILL_COLD_RES) == 1) {
                        rate = 1;
                    } else if (Player_HasSkill(pl, 140) == 1) {
                        rate = 4;
                    } else if (Player_HasSkill(pl, 141) == 1) {
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
        if (!Player_HasSkill(pl, 74)) {
            if (!Player_HasSkill(pl, 73) || (gScene.frame & 1)) {
                rate++;
            }
            if (Player_HasSkill(pl, 76) == 1) {
                rate++;
            } else if (Player_HasSkill(pl, 75) == 1 && !(gScene.frame & 1)) {
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

