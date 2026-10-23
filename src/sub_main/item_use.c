/*
 * Using the pouch's items (sub_main): Item_Use and its helpers for the
 * drugs, foods, tools, placed items and team powders; the pouch cursor's
 * next selectable item; and the player's timed buffs (the item time
 * skill's scaling, the song effects, clearing the buffs). Among them, one
 * small check fires an area's message event when the local player enters
 * that area.
 * The players are gPlayers (player.h); the items' records are item.h's.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "item.h"
#include "quest.h"
#include "syssettings.h"
#include "area.h"
#include "objattach.h"
#include "area_draw.h"
#include "camera.h"
#include "qsync.h"
#include "game_util.h"
#include "hit_effects.h"
#include "rand.h"
#include "attack_objects.h"
#include "msg_event.h"

extern u16 gFoodPrizesA[];
extern u16 gFoodPrizesB[];

u8 Pouch_Add(Player *p, u16 item, s16 n);
/* unit's own declaration: s32 parameters as Item_Use's calls pass them; with the definition's s16, u8 and s16 the calls compile differently */
void Effect06_SpawnAtBone(Player *pl, s32 type, s32 sub, s32 bone, f32 scale);
u8 Player_GetSkill(Player *p, u8 group);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);
s16 Player_ScaleItemDuration(Player *pl, s16 n);
void Item_StaminaBonus(Player *pl);
void Player_ClearSongEffects(Player *pl);
/* end of prototypes */

/*
 * With SKILL_ITEM_STAMINA, the local player's item use has a 1 in 3 chance
 * of raising the maximum stamina by 75.
 */
void Item_StaminaBonus(Player *pl)
{
    if (Player_IsLocal(pl) == 1 && Player_HasSkill(pl, SKILL_ITEM_STAMINA) == 1 && Rand_Next(1) % 3 == 0) {
        Player_AddMaxStamina(pl, 75);
    }
}

/*
 * Uses the player's item (Player.useItem, an ITEM_ID_*) once its motion
 * ends: takes it from the pouch (the prize draws ITEM_ID_FOOD_PRIZE_A and _B
 * only on the user's console; nothing after the quest), then gives its
 * effect: healing, maximum health or stamina, the drugs and timed buffs, the
 * hot and cool drinks, the cures and the gambles. A remote teammate's item
 * may also reach the local player (SKILL_GROUP_ITEM_SHARE). Each path ends
 * in a glow on the player (Effect06_SpawnAtBone, an ITEM_GLOW_* colour) and
 * a motion: the pose (PL_MOT_ITEM_POSE), the eating motion (PL_MOT_EAT) or
 * the no-gain motion (PL_MOT_ITEM_NO_GAIN); then the use is counted.
 */
void Item_Use(Player *pl)
{
    s32 glow;
    Player *local;
    u16 item;
    u16 roll;
    u16 prize;
    u16 *prizeTable;

    local = &gPlayers[gScene.localPlayer];
    glow = ITEM_GLOW_STAMINA;
    pl->teamItemMask = 15;
    item = pl->useItem;
    if (item != ITEM_ID_FOOD_PRIZE_A && item != ITEM_ID_FOOD_PRIZE_B && Quest_PhaseIsOver(1) == 0) {
        Pouch_Add(pl, pl->useItem, -1);
    }
    switch (item) {
    case ITEM_ID_ATTACK_BURST:
        if (Player_IsLocal(pl) == 1) {
            pl->attackBuff = 25;
            pl->attackBuffTimer = Player_ScaleItemDuration(pl, 630);
        }
        Item_StaminaBonus(pl);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
        goto pose_end;
    case ITEM_ID_DEFENSE_BURST:
        if (Player_IsLocal(pl) == 1) {
            pl->defenseBuff = 50;
            pl->defenseBuffTimer = Player_ScaleItemDuration(pl, 630);
            pl->noFlinchItemTimer = pl->defenseBuffTimer;
        }
        Item_StaminaBonus(pl);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_DEFENSE, 10, 4.0f);
        goto pose_end;
    case ITEM_ID_ATTACK_DRUG:
    case ITEM_ID_ATTACK_DRUG_STRONG:
        if (Player_IsLocal(pl) == 1) {
            if (pl->useItem == ITEM_ID_ATTACK_DRUG) {
                if (pl->attackDrug < 3) {
                    pl->attackDrug = 3;
                }
            } else {
                if (pl->attackDrug < 5) {
                    pl->attackDrug = 5;
                }
            }
        }
        Item_StaminaBonus(pl);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
        goto pose_end;
    case ITEM_ID_DEFENSE_DRUG:
    case ITEM_ID_DEFENSE_DRUG_STRONG:
        if (Player_IsLocal(pl) == 1) {
            if (pl->useItem == ITEM_ID_DEFENSE_DRUG) {
                if (pl->defenseDrug < 6) {
                    pl->defenseDrug = 6;
                }
            } else {
                if (pl->defenseDrug < 10) {
                    pl->defenseDrug = 10;
                }
            }
            Item_StaminaBonus(pl);
        }
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_DEFENSE, 10, 4.0f);
        goto pose_end;
    case ITEM_ID_ATTACK_SHARED:
        if (!Player_IsLocal(pl) && pl->area == local->area && Player_OnSameSide(pl, local) == 1 && Player_GetSkill(pl, SKILL_GROUP_ITEM_SHARE) != 0) {
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
            local->attackBuffTimer = Player_ScaleItemDuration(pl, 5400);
            if (Player_HasSkill(pl, SKILL_ITEM_SHARE_HALF) == 1) {
                local->attackBuff = 5;
            }
            if (Player_HasSkill(pl, SKILL_ITEM_SHARE_FULL) == 1) {
                local->attackBuff = 10;
            }
        }
    attack:
        Item_StaminaBonus(pl);
        if (Player_IsLocal(pl) == 1) {
            pl->attackBuff = 10;
            pl->attackBuffTimer = Player_ScaleItemDuration(pl, 5400);
        }
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
        goto pose_end;
    case ITEM_ID_DEFENSE_SHARED:
        if (!Player_IsLocal(pl) && pl->area == local->area && Player_OnSameSide(pl, local) == 1 && Player_GetSkill(pl, SKILL_GROUP_ITEM_SHARE) != 0) {
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_DEFENSE, 10, 4.0f);
            local->defenseBuffTimer = Player_ScaleItemDuration(pl, 5400);
            if (Player_HasSkill(pl, SKILL_ITEM_SHARE_HALF) == 1) {
                local->defenseBuff = 20;
            }
            if (Player_HasSkill(pl, SKILL_ITEM_SHARE_FULL) == 1) {
                local->defenseBuff = 40;
            }
        }
    defense:
        Item_StaminaBonus(pl);
        if (Player_IsLocal(pl) == 1) {
            pl->defenseBuff = 40;
            pl->defenseBuffTimer = Player_ScaleItemDuration(pl, 5400);
        }
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_DEFENSE, 10, 4.0f);
        goto pose_end;
    case ITEM_ID_HOT_DRINK:
        pl->hotDrinkTimer = Player_ScaleItemDuration(pl, 18000);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HOT, 10, 4.0f);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 17, 0);
            Item_StaminaBonus(pl);
        }
        goto pose_end;
    case ITEM_ID_COOL_DRINK:
        pl->coolDrinkTimer = Player_ScaleItemDuration(pl, 18000);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_COOL, 10, 4.0f);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 16, 0);
            Item_StaminaBonus(pl);
        }
        goto pose_end;
    case ITEM_ID_DETECT:
        pl->detectTimer = Player_ScaleItemDuration(pl, 300);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_DETECT, 10, 4.0f);
        Item_StaminaBonus(pl);
        goto pose_end;
    case ITEM_ID_REGEN:
        pl->regenTimer = Player_ScaleItemDuration(pl, 9000);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_REGEN, 10, 4.0f);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 23, 0);
            Item_StaminaBonus(pl);
        }
        goto pose_end;
    case ITEM_ID_STAMINA_LOCK:
    stamina:
        Item_StaminaBonus(pl);
        Player_AddMaxStamina(pl, 75);
        pl->staminaLockTimer = Player_ScaleItemDuration(pl, 5400);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 24, 0);
        }
        goto effect0;
    case ITEM_ID_STAMINA_LOCK_LONG:
        Player_AddMaxStamina(pl, 150);
        pl->staminaLockTimer = Player_ScaleItemDuration(pl, 10800);
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 24, 0);
            Item_StaminaBonus(pl);
        }
        goto effect0;
    case ITEM_ID_MAX_ALL:
    maxhp:
        Player_AddMaxStamina(pl, 450);
    case ITEM_ID_MAX_HP_FULL:
    case ITEM_ID_SUPPLY_MAX_HP_FULL:
        Item_StaminaBonus(pl);
        Player_AddMaxHp(pl, 150, 0);
        Player_Heal(pl, 150, 0);
        goto effect0;
    case ITEM_ID_HEAL_20:
        if (!Player_IsLocal(pl) && pl->area == local->area && Player_OnSameSide(pl, local) == 1 && Player_GetSkill(pl, SKILL_GROUP_ITEM_SHARE) != 0) {
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_HEAL, 10, 4.0f);
            if (Player_HasSkill(pl, SKILL_ITEM_SHARE_HALF) == 1) {
                Player_Heal(local, 10, 1);
            }
            if (Player_HasSkill(pl, SKILL_ITEM_SHARE_FULL) == 1) {
                Player_Heal(local, 20, 1);
            }
        }
    heal:
        Item_StaminaBonus(pl);
        Player_Heal(pl, 20, 0);
        goto effect0;
    case ITEM_ID_HEAL_50_TEAM:
        Player_Heal(pl, 50, 0);
        Item_StaminaBonus(pl);
        if (!Player_IsLocal(pl) && pl->area == local->area && Player_OnSameSide(pl, local) == 1 && Player_OnSameSide(local, pl) == 1) {
            Player_Heal(local, 50, 1);
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_HEAL, 10, 4.0f);
        }
        goto effect0;
    case ITEM_ID_HEAL_30:
        Player_Heal(pl, 30, 0);
        Item_StaminaBonus(pl);
        if (!Player_IsLocal(pl) && pl->area == local->area && Player_OnSameSide(pl, local) == 1 && Player_GetSkill(pl, SKILL_GROUP_ITEM_SHARE) != 0) {
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_HEAL, 10, 4.0f);
            if (Player_HasSkill(pl, SKILL_ITEM_SHARE_HALF) == 1) {
                Player_Heal(local, 15, 1);
            }
            if (Player_HasSkill(pl, SKILL_ITEM_SHARE_FULL) == 1) {
                Player_Heal(local, 30, 1);
            }
        }
        goto effect0;
    case ITEM_ID_SUPPLY_HEAL_30:
        Player_Heal(pl, 30, 0);
        Item_StaminaBonus(pl);
        goto effect0;
    case ITEM_ID_POISON_GAMBLE:
        Item_StaminaBonus(pl);
        Player_AddPoison(pl, 10);
        if (pl->stateRand & 1) {
            Player_AddMaxHp(pl, 10, 0);
        } else {
            Player_AddMaxHp(pl, -10, 0);
        }
        Player_SetMotionPair(pl, PL_MOT_ITEM_NO_GAIN, 4, 0);
        break;
    case ITEM_ID_HURT_CURE_CHANCE:
        Player_AddHp(pl, -10);
        if (pl->hp <= 0) {
            pl->hp = 1;
        }
        if (pl->stateRand & 1) {
            pl->poison = 0;
            Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_CURE, 10, 4.0f);
            Item_StaminaBonus(pl);
        } else {
            Player_SetMotionPair(pl, PL_MOT_ITEM_NO_GAIN, 4, 0);
        }
        break;
    case ITEM_ID_CURE_CHANCE:
        if (pl->stateRand & 1) {
            pl->poison = 0;
            Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_CURE, 10, 4.0f);
            Item_StaminaBonus(pl);
        } else {
            Player_SetMotionPair(pl, PL_MOT_ITEM_NO_GAIN, 4, 0);
        }
        break;
    case ITEM_ID_HEAL_50:
        Player_Heal(pl, 50, 0);
        Item_StaminaBonus(pl);
    effect0:
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
    pose_end:
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
        break;
    case ITEM_ID_MAX_HP_10:
        Player_AddMaxHp(pl, 10, 0);
        Item_StaminaBonus(pl);
        goto effect0b;
    case ITEM_ID_MAX_HP_20:
        Player_AddMaxHp(pl, 20, 0);
        Item_StaminaBonus(pl);
    effect0b:
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
        break;
    case ITEM_ID_SUPPLY_MAX_STAMINA_75:
    case ITEM_ID_MAX_STAMINA_75_B:
    case ITEM_ID_MAX_STAMINA_75:
        Player_AddMaxStamina(pl, 75);
        Item_StaminaBonus(pl);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_STAMINA, 10, 4.0f);
        goto pose_end;
    case ITEM_ID_FOOD_RESTORE:
    case ITEM_ID_FOOD_RESTORE_B:
        pl->hp = pl->hpRecoverable;
        glow = ITEM_GLOW_HEAL;
        goto drink;
    case ITEM_ID_FOOD_REGEN_SHORT:
        pl->hp = pl->hpRecoverable;
        pl->regenTimer = Player_ScaleItemDuration(pl, 3600);
        glow = ITEM_GLOW_REGEN;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 23, 0);
        }
        goto drink;
    case ITEM_ID_FOOD_REGEN:
        pl->hp = pl->hpRecoverable;
        pl->regenTimer = Player_ScaleItemDuration(pl, 7200);
        glow = ITEM_GLOW_REGEN;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 23, 0);
        }
        goto drink;
    case ITEM_ID_FOOD_REGEN_B:
        pl->hp = pl->hpRecoverable;
        pl->regenTimer = Player_ScaleItemDuration(pl, 7200);
        glow = ITEM_GLOW_REGEN;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 23, 0);
        }
        goto drink;
    case ITEM_ID_FOOD_REGEN_LONG:
        pl->hp = pl->hpRecoverable;
        pl->regenTimer = Player_ScaleItemDuration(pl, 14400);
        glow = ITEM_GLOW_REGEN;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 23, 0);
        }
        goto drink;
    case ITEM_ID_FOOD_CURE:
        pl->poison = 0;
        glow = ITEM_GLOW_CURE;
        goto drink;
    case ITEM_ID_FOOD_CURE_STAMINA:
        pl->poison = 0;
        glow = ITEM_GLOW_CURE;
        Player_AddMaxStamina(pl, 150);
        goto drink;
    case ITEM_ID_FOOD_HOT:
        pl->hotDrinkTimer = Player_ScaleItemDuration(pl, 9000);
        glow = ITEM_GLOW_HOT;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 17, 0);
        }
        goto drink;
    case ITEM_ID_FOOD_HOT_STAMINA:
        pl->hotDrinkTimer = Player_ScaleItemDuration(pl, 9000);
        Player_AddMaxStamina(pl, 150);
        glow = ITEM_GLOW_HOT;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 17, 0);
        }
        goto drink;
    case ITEM_ID_FOOD_COOL:
        pl->coolDrinkTimer = Player_ScaleItemDuration(pl, 9000);
        glow = ITEM_GLOW_COOL;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 16, 0);
        }
        goto drink;
    case ITEM_ID_FOOD_COOL_STAMINA:
        pl->coolDrinkTimer = Player_ScaleItemDuration(pl, 9000);
        Player_AddMaxStamina(pl, 150);
        glow = ITEM_GLOW_COOL;
        if (Player_IsLocal(pl) == 1) {
            Notice_Push(0, 16, 0);
        }
        goto drink;
    case ITEM_ID_FOOD_STAMINA_75:
        Player_AddMaxStamina(pl, 75);
        goto drink;
    case ITEM_ID_FOOD_STAMINA_FULL:
        Player_AddMaxStamina(pl, 450);
        goto drink;
    case ITEM_ID_FOOD_STAMINA_150:
        Player_AddMaxStamina(pl, 150);
    drink:
        if (Player_HasSkill(pl, SKILL_FOOD_STAMINA) == 1) {
            Player_AddMaxStamina(pl, 75);
        } else {
            Item_StaminaBonus(pl);
        }
        Effect06_SpawnAtBone(pl, 0, glow, 10, 4.0f);
        Player_SetMotionPair(pl, PL_MOT_EAT, 4, 0);
        break;
    case ITEM_ID_FOOD_HEAL_20:
    eat:
        Player_Heal(pl, 20, 0);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
        Player_SetMotionPair(pl, PL_MOT_EAT, 4, 0);
        Item_StaminaBonus(pl);
        break;
    case ITEM_ID_FOOD_PRIZE_A:
    case ITEM_ID_FOOD_PRIZE_B:
        if (Player_IsLocal(pl) == 1) {
            Pouch_Add(pl, pl->useItem, -1);
        }
        if (pl->stateRand & 0x8000) {
            goto eat;
        }
        if (Player_IsLocal(pl) == 1) {
            if (item == ITEM_ID_FOOD_PRIZE_A) {
                prize = 255;
                prizeTable = gFoodPrizesA;
            } else {
                prize = 264;
                prizeTable = gFoodPrizesB;
            }
            roll = pl->stateRand % 100;
            while (prizeTable[0] != 0xFFFF) {
                if (roll < prizeTable[0]) {
                    prize = prizeTable[1];
                    break;
                }
                roll -= prizeTable[0];
                prizeTable += 2;
            }
            Pouch_GiveItem(pl, prize, pl->gotItem, 3);
        }
        Player_SetMotionPair(pl, PL_MOT_ITEM_NO_GAIN, 4, 0);
        break;
    case ITEM_ID_FOOD_STAMINA_GAMBLE:
        if (pl->stateRand & 1) {
            Player_AddMaxStamina(pl, 75);
            goto drink;
        }
        Player_AddMaxStamina(pl, -75);
        Player_SetMotionPair(pl, PL_MOT_ITEM_NO_GAIN, 4, 0);
        break;
    case ITEM_ID_HEAL_10_CURE:
        Player_Heal(pl, 10, 0);
    case ITEM_ID_CURE_POISON:
        Item_StaminaBonus(pl);
        pl->poison = 0;
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_CURE, 10, 4.0f);
        if (!Player_IsLocal(pl) && pl->area == local->area && (Player_HasSkill(pl, SKILL_ITEM_SHARE_HALF) == 1 || Player_HasSkill(pl, SKILL_ITEM_SHARE_FULL) == 1) && pl->useItem == ITEM_ID_CURE_POISON && Player_OnSameSide(local, pl) == 1) {
            local->poison = 0;
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_CURE, 10, 4.0f);
        }
        break;
    case ITEM_ID_RANDOM:
        roll = pl->stateRand % 100;
        if (roll <= 0) {
            Player_AddPoison(pl, 10);
            if (Player_HasSkill(pl, SKILL_STATUS7_IMMUNE) == 0) {
                if (Player_HasSkill(pl, SKILL_STATUS7_HALF) == 1) {
                    pl->status7Timer = 900;
                } else {
                    pl->status7Timer = 1800;
                }
            }
            if (Player_HasSkill(pl, SKILL_STATUS6_IMMUNE) == 0) {
                if (Player_HasSkill(pl, SKILL_STATUS6_HALF) == 1) {
                    pl->status6Timer = 900;
                } else if (Player_HasSkill(pl, SKILL_STATUS6_DOUBLE) == 1) {
                    pl->status6Timer = 3600;
                } else {
                    pl->status6Timer = 1800;
                }
            }
            Player_SetMotionPair(pl, PL_MOT_ITEM_NO_GAIN, 4, 0);
            break;
        }
        if (roll < 6) {
            Player_AddHp(pl, -(pl->hp / 2));
            pl->hpRecoverable = pl->hp;
            Player_SetMotionPair(pl, PL_MOT_ITEM_NO_GAIN, 4, 0);
            break;
        }
        if (roll < 20) {
            Player_AddMaxStamina(pl, -75);
            Player_SetMotionPair(pl, PL_MOT_ITEM_NO_GAIN, 4, 0);
            break;
        }
        if (roll < 47) {
            goto heal;
        }
        if (roll < 67) {
            goto attack;
        }
        if (roll < 87) {
            goto defense;
        }
        if (roll < 97) {
            goto stamina;
        }
        goto maxhp;
    }
    Stats_CountItemUse(pl);
}

/*
 * Places the player's tool (Player_StatePlaceItem): the small and launched
 * barrel bombs (ItemAttack_Create with the ItemAttack type), a large barrel
 * bomb (PlacedItem_Create) and the baits, which a monster's goal search can
 * target (ItemProp_CreateBait: an ITEMPROP_BAIT, with the bait's
 * kind).
 */
void Item_UseTool(Player *pl)
{
    switch (pl->useItem) {
    case ITEM_ID_NO_BOMB_AREA_ITEM:
        ItemAttack_Create(pl, ITEMATK_NO_BOMB_AREA_ITEM, 0);
        break;
    case ITEM_ID_SMALL_BARREL_BOMB:
        ItemAttack_Create(pl, 4, 0);
        break;
    case ITEM_ID_SMALL_BARREL_BOMB_B:
        ItemAttack_Create(pl, ITEMATK_SMALL_BARREL_BOMB_B, 0);
        break;
    case ITEM_ID_LAUNCHED_BARREL_BOMB:
        ItemAttack_Create(pl, 20, 0);
        break;
    case ITEM_ID_LAUNCHED_BARREL_BOMB_B:
        ItemAttack_Create(pl, 24, 0);
        break;
    case ITEM_ID_LARGE_BARREL_BOMB:
        PlacedItem_Create(pl->pos, PLACED_LARGE_BARREL_BOMB, pl->area, pl);
        break;
    case ITEM_ID_MEAT:
        ItemProp_CreateBait(pl, ITEMPROP_BAIT, 0);
        break;
    case ITEM_ID_BAIT_POISON:
        ItemProp_CreateBait(pl, ITEMPROP_BAIT, 1);
        break;
    case ITEM_ID_BAIT_STATUS2:
        ItemProp_CreateBait(pl, ITEMPROP_BAIT, 2);
        break;
    case ITEM_ID_BAIT_STATUS4:
        ItemProp_CreateBait(pl, ITEMPROP_BAIT, 3);
        break;
    }
    Stats_CountItemUse(pl);
    if (Quest_PhaseIsOver(1) == 0) {
        Pouch_Add(pl, pl->useItem, -1);
    }
}

/*
 * Sets down the player's large bomb (Player_StatePlaceLargeItemSet):
 * PlacedItem_Create with the bomb's kind (0 and 1 the large barrel bombs, 5
 * the bomb set on a monster, 6 ITEM_ID_PLACED_BOMB).
 */
void Item_UsePlaced(Player *pl)
{
    switch (pl->useItem) {
    case ITEM_ID_LARGE_BARREL_BOMB:
        PlacedItem_Create(pl->pos, PLACED_LARGE_BARREL_BOMB, pl->area, pl);
        break;
    case ITEM_ID_LARGE_BARREL_BOMB_B:
    case ITEM_ID_SUPPLY_LARGE_BARREL_BOMB:
        PlacedItem_Create(pl->pos, PLACED_LARGE_BARREL_BOMB_B, pl->area, pl);
        break;
    case ITEM_ID_MONSTER_BOMB:
        PlacedItem_Create(pl->pos, PLACED_MONSTER_BOMB, pl->area, pl);
        break;
    case ITEM_ID_PLACED_BOMB:
        PlacedItem_Create(pl->pos, PLACED_BOMB, pl->area, pl);
        break;
    }
    Stats_CountItemUse(pl);
    if (Quest_PhaseIsOver(1) == 0) {
        Pouch_Add(pl, pl->useItem, -1);
    }
}

/*
 * Sets the player's trap or special item (Player_StateSetTrapDone): the
 * trap (PlacedItem kind 4), the pitfall trap (PitfallTrap_Create) and the
 * special mining items (ids 760 to 793, not all used: SpecialMine_Create).
 * ITEM_ID_DELIVERY is neither counted nor taken here.
 */
void Item_UseMisc(Player *pl)
{
    if (pl->useItem == ITEM_ID_DELIVERY) {
        return;
    }
    switch (pl->useItem) {
    case ITEM_ID_TRAP:
    case ITEM_ID_SUPPLY_TRAP:
        PlacedItem_Create(pl->pos, PLACED_TRAP, pl->area, pl);
        break;
    case ITEM_ID_PITFALL_TRAP:
    case ITEM_ID_SUPPLY_PITFALL_TRAP:
        PitfallTrap_Create(pl, 0);
        break;
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
    case 782:
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
        SpecialMine_Create(pl);
        break;
    }
    Stats_CountItemUse(pl);
    if (Quest_PhaseIsOver(1) == 0) {
        Pouch_Add(pl, pl->useItem, -1);
    }
}

/*
 * A team item (ITEM_ID_TEAM_*): heals 20, cures poison, or raises attack
 * or defence for 5400 frames (3 minutes), on the local player when it is on
 * the user's side and, for a remote user, in the same area.
 */
void Item_UseTeamPowder(Player *pl)
{
    Player *local = &gPlayers[gScene.localPlayer];

    pl->teamItemMask = 15;
    if (!Player_IsLocal(pl) && pl->area != local->area) {
        return;
    }
    switch (pl->useItem) {
    case ITEM_ID_TEAM_HEAL:
        if (Player_OnSameSide(local, pl) == 1) {
            Player_AddHp(local, 20);
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_HEAL, 10, 4.0f);
        }
        if (!Player_IsLocal(pl)) {
            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
        } else {
            QuestStats_Add(2, 1);
        }
        break;
    case ITEM_ID_TEAM_CURE:
        if (Player_OnSameSide(local, pl) == 1) {
            local->poison = 0;
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_CURE, 10, 4.0f);
        }
        if (!Player_IsLocal(pl)) {
            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_CURE, 10, 4.0f);
        }
        break;
    case ITEM_ID_TEAM_ATTACK:
        if (Player_OnSameSide(local, pl) == 1) {
            local->attackBuff = 10;
            local->attackBuffTimer = 5400;
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
        }
        if (!Player_IsLocal(pl)) {
            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_ATTACK, 10, 4.0f);
        }
        break;
    case ITEM_ID_TEAM_DEFENSE:
        if (Player_OnSameSide(local, pl) == 1) {
            local->defenseBuff = 40;
            local->defenseBuffTimer = 5400;
            Effect06_SpawnAtBone(local, 0, ITEM_GLOW_DEFENSE, 10, 4.0f);
        }
        if (!Player_IsLocal(pl)) {
            Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_DEFENSE, 10, 4.0f);
        }
        break;
    }
}

/*
 * The next pouch slot after `cur` whose item can be selected (ItemData
 * flag 1), walking forward (`dir` 0) or back and wrapping, `cur` itself
 * last; 0 when there is none.
 */
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir)
{
    s16 found = 0;
    s16 i;
    s16 j;

    if (dir == 0) {
        j = cur;
        for (i = 0; i < POUCH_SLOTS; i++) {
            j++;
            if (j >= POUCH_SLOTS) {
                j = 0;
            }
            if (pl->pouch[j].item != 0 && (gItemData[pl->pouch[j].item].flags & 1)) {
                cur = j;
                found = 1;
                break;
            }
        }
    } else {
        for (j = 0, i = cur; j < POUCH_SLOTS; j++) {
            if (i == 0) {
                i = POUCH_SLOTS - 1;
            } else {
                i--;
            }
            if (pl->pouch[i].item != 0 && (gItemData[pl->pouch[i].item].flags & 1)) {
                cur = i;
                found = 1;
                break;
            }
        }
    }
    if (found == 0) {
        cur = 0;
    }
    return cur;
}

/*
 * For the local player: entering one of the areas below fires that area's
 * message event (the first jungle areas by day and night, the first
 * fortress area).
 */
void Player_CheckAreaMsgEvent(Player *pl)
{
    if (Player_IsLocal(pl)) {
        switch (pl->area) {
        case AREA_JUNGLE_1:
        case AREA_JUNGLE_NIGHT_1:
            MsgEvent_Check(5);
            break;
        case AREA_JUNGLE_2:
        case AREA_JUNGLE_NIGHT_2:
            MsgEvent_Check(6);
            break;
        case AREA_JUNGLE_3:
        case AREA_JUNGLE_NIGHT_3:
            MsgEvent_Check(7);
            break;
        case AREA_JUNGLE_4:
        case AREA_JUNGLE_NIGHT_4:
            MsgEvent_Check(8);
            break;
        case AREA_JUNGLE_8:
        case AREA_JUNGLE_NIGHT_8:
            MsgEvent_Check(9);
            break;
        case AREA_JUNGLE_6:
        case AREA_JUNGLE_NIGHT_6:
            MsgEvent_Check(10);
            break;
        case AREA_FORTRESS_0:
        case AREA_FORTRESS_NIGHT_0:
            MsgEvent_Check(11);
            break;
        }
    }
}

/*
 * An item's duration in frames, times 1.5 with the item time skill (SKILL_ITEM_TIME_UP)
 * or times 0.66 with its negative (SKILL_ITEM_TIME_DOWN).
 */
s16 Player_ScaleItemDuration(Player *pl, s16 frames)
{
    s16 scaled = frames;

    if (Player_HasSkill(pl, SKILL_ITEM_TIME_UP) == 1) {
        scaled = 1.5f * frames;
    } else if (Player_HasSkill(pl, SKILL_ITEM_TIME_DOWN) == 1) {
        scaled = 0.66f * frames;
    }
    return scaled;
}

/* Clears the song effects (the hunting horn's) and their timers. */
void Player_ClearSongEffects(Player *pl)
{
    s16 i;

    for (i = 0; i < PL_SONG_KINDS; i++) {
        pl->songEffect[i] = 0;
        pl->songEffectTimer[i] = 0;
    }
}

/* 1 while the song effect `kind`'s (PL_SONG_*) timer runs. */
s32 Player_HasSongEffect(Player *pl, s16 kind)
{
    return pl->songEffectTimer[kind] > 0;
}

/*
 * Clears every timed state: the drinks' and items' buffs, the drugs, the
 * status build-ups and timers (poison too), the sword gauge, the horn's
 * notes and the song effects.
 */
void Player_ClearBuffs(Player *pl)
{
    pl->staminaLockTimer = 0;
    pl->coolDrinkTimer = 0;
    pl->hotDrinkTimer = 0;
    pl->regenTimer = 0;
    pl->detectTimer = 0;
    pl->actDetectTimer = 0;
    pl->status5Delay = 0;
    pl->status5Buildup = 0;
    pl->poison = 0;
    pl->mashLeft = 0;
    pl->status7Timer = 0;
    pl->status6Timer = 0;
    pl->mashBuildup = 0;
    pl->status2Buildup = 0;
    pl->status4Buildup = 0;
    pl->recoverTimer = 0;
    pl->attackDrug = 0;
    pl->defenseDrug = 0;
    pl->attackBuff = 0;
    pl->defenseBuff = 0;
    pl->attackBuffTimer = 0;
    pl->defenseBuffTimer = 0;
    pl->swordGauge = 0;
    HuntingHorn_ClearNotes(pl);
    Player_ClearSongEffects(pl);
}

/* The file's initialised data, in the game's order */
u16 gFoodPrizesA[24] = {
#include "sub_main/0044AF80.inc"
};
u16 gFoodPrizesB[16] = {
#include "sub_main/0044AFB0.inc"
};
