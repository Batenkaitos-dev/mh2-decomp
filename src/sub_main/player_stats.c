/*
 * The player's stats: the attack and defense (Player.attack and
 * Player.defense, with their factors) from the equipment, the pouch, the
 * drugs and buffs, the skills and the song effects (Player.songEffect,
 * which only the hunting horn's songs set); the five element resistances
 * (Player.resist); the skills (Player.skills: the points the equipment
 * gives in each skill group, turned into skills by gSkillTable); whether
 * the weapon can guard; the HUD's status icons. Each stat has a version for
 * another player, from the equipment data that player's console sent
 * (EquipView_*).
 *
 * Before them, a few small player helpers: whether a large monster is
 * near, the body capsule, the posture, whether a player is this console's,
 * and whether it is using a mounted gun. Some of these also run on
 * monsters, read through the Player struct: the fields they use sit at the
 * same offsets in Player and Em (the shared ones are listed by ObjHead).
 */
#include "common.h"
#include "quest.h"
#include "item.h"
#include "save.h"
#include "mine_point.h"
#include "player.h"
#include "scene.h"
#include "em.h"
#include "model.h"
#include "objhead.h"
#include "capsule.h"
#include "objattach.h"
#include "area.h"
#include "bgm.h"
#include "engine_math.h"
#include "game_util.h"
#include "lobby_queries.h"
#include "stage_data.h"
#include "vec_math.h"

/* The game's compiler built this file at optimization level 4: its code
 * matches only at that level. */
#pragma optimization_level 4

extern s16 gResistSongEffects[5];       /* the song effect kind of each resistance, in Player.resist's order */

s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 Player_HasSongEffect(Player *pl, s16 kind);
static void Player_AddResists(f32 bonus, Player *p, s16 elem, f32 v);
static void Player_ApplyResistSkills(Player *p);
static void Player_ClampResists(Player *p);

extern Em gEms[EM_COUNT];
void Player_BodyCapsule(Player *p, HitCapsule *cap, f32 r);

/* Sets Player.nearLargeMonster: 1 while a large monster (pushPriority 3 or
 * more) is within 300 horizontally (500 for one above 3) and the player is
 * less than 300 above it; 0 while the player rides, or is more than 100
 * above its floor. */
void Player_UpdateNearMonster(Player *pl)
{
    s32 i;
    Em *em;
    f32 dx;
    f32 dz;
    f32 dist;
    f32 range;
    f32 maxDrop;

    if (pl->rideState != 0 || !(pl->pos[1] - pl->floorY <= 100.0f)) {
        pl->nearLargeMonster = 0;
        return;
    }
    for (em = gEms, i = 0; i < EM_COUNT; i++, em++) {
        if (em->active == 0 || em->visible == 0 || em->pushPriority < 3) {
            continue;
        }
        dx = pl->pos[0] - em->posX;
        dz = pl->pos[2] - em->posZ;
        dist = Math_Sqrtf(dx * dx + dz * dz);
        if (em->pushPriority == 3) {
            maxDrop = range = 300.0f;
        } else {
            range = 500.0f;
            maxDrop = 300.0f;
        }
        if (dist <= range && pl->pos[1] - em->posY < maxDrop) {
            pl->nearLargeMonster = 1;
            return;
        }
    }
    pl->nearLargeMonster = 0;
}

/* The player's body as a capsule of radius capRadius: from the midpoint of
 * parts 5 and 8 (Player.partRecs) to part 20, each end then moved toward the
 * other by the radius, so that the rounded ends stay within the two points. */
void Player_BodyCapsule(Player *pl, HitCapsule *cap, f32 capRadius)
{
    f32 mtx[16];
    f32 partA[3];
    f32 partB[3];
    f32 axis[3];
    f32 offset[3];
    f32 *axisY;
    f32 *axisZ;

    Mtx_GetTrans(partA, pl->partRecs[5]->m1);
    Mtx_GetTrans(partB, pl->partRecs[8]->m1);
    cap->a[0] = (partA[0] + partB[0]) / 2.0f;
    cap->a[1] = (partA[1] + partB[1]) / 2.0f;
    cap->a[2] = (partA[2] + partB[2]) / 2.0f;
    Mtx_GetTrans(cap->b, pl->partRecs[20]->m1);
    axisY = &axis[1];
    axisZ = &axis[2];
    axis[0] = cap->b[0] - cap->a[0];
    *axisY = cap->b[1] - cap->a[1];
    *axisZ = cap->b[2] - cap->a[2];
    Mtx_FromDirection(axis, mtx, 1);
    axis[0] = 0.0f;
    *axisY = capRadius;
    *axisZ = 0.0f;
    Vec_Rotate(offset, axis, mtx);
    cap->a[0] += offset[0];
    cap->a[1] += offset[1];
    cap->a[2] += offset[2];
    cap->b[0] -= offset[0];
    cap->b[1] -= offset[1];
    cap->b[2] -= offset[2];
    cap->radius = capRadius;
}

/*
 * Sets the player's posture (Player.action) from the low byte of posture:
 * PL_POSTURE_CROUCHED and PL_POSTURE_AIRBORNE are kept, anything else is
 * PL_POSTURE_STANDING. Also sets three of the player's flags: 8 unless
 * posture has bit 15 (0x8000), 1 by setFlag1 (the flag that keeps the hit
 * window open, Player_UpdateHitTimers) and 2 by setFlag2.
 */
void Player_SetPosture(Player *pl, u16 posture, s16 setFlag1, s16 setFlag2)
{
    s32 postureBits = posture;

    switch ((u8)postureBits) {
    default:
        pl->action = PL_POSTURE_STANDING;
        break;
    case PL_POSTURE_CROUCHED:
        pl->action = PL_POSTURE_CROUCHED;
        break;
    case PL_POSTURE_AIRBORNE:
        pl->action = PL_POSTURE_AIRBORNE;
        break;
    }
    if (postureBits & 0x8000) {
        Player_ClearFlag(pl, 8);
    } else {
        Player_SetFlag(pl, 8);
    }
    if (setFlag1 == 0) {
        Player_ClearFlag(pl, 1);
    } else {
        Player_SetFlag(pl, 1);
    }
    if (setFlag2 == 0) {
        Player_ClearFlag(pl, 2);
    } else {
        Player_SetFlag(pl, 2);
    }
}

/* Nonzero for the player of this console: its number (ObjHead.index) is the
 * local one (gScene.localPlayer), or, in a drill quest with gScene.allLocal
 * set, every player counts as local. */
s32 Player_IsLocal(void *obj)
{
    ObjHead *head = obj;
    if (Quest_IsDrill() == 1 && gScene.allLocal != 0) {
        return 1;
    }
    if (head->index == gScene.localPlayer) {
        return 1;
    }
    return 0;
}

/* Nonzero while the player aims, fires or re-aims a mounted gun. */
s32 Player_OnMountedGun(Player *pl)
{
    if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_FIRE) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM) != 0) {
        return 1;
    }
    return 0;
}

/* Sets the local player's HUD status icons (Player.statusIcons, PL_STATUS_ICON_*) from its state. */
void Player_UpdateStatusIcons(Player *pl)
{
    u16 flags = 0;
    s16 i;

    if (Player_IsLocal(pl) != 0) {
        if (pl->poison != 0) {
            flags |= PL_STATUS_ICON_POISON;
        }
        if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS2) != 0) {
            flags |= PL_STATUS_ICON_STATUS2;
        }
        if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS4) != 0 || Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS4_HIT) != 0) {
            flags |= PL_STATUS_ICON_STATUS4;
        }
        if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STUN) != 0) {
            flags |= PL_STATUS_ICON_STUN;
        }
        if (Player_InMashStatus(pl) == 1) {
            flags |= PL_STATUS_ICON_MASH;
        }
        if ((s16)(pl->attackDrug + pl->attackBuff) > 0) {
            flags |= PL_STATUS_ICON_ATTACK_UP;
        }
        if (Player_HasSongEffect(pl, PL_SONG_ATTACK) == 1) {
            flags |= PL_STATUS_ICON_ATTACK_UP;
        }
        if ((s16)(pl->defenseDrug + pl->defenseBuff) > 0) {
            flags |= PL_STATUS_ICON_DEFENSE_UP;
        }
        if (Player_HasSongEffect(pl, PL_SONG_DEFENSE) == 1) {
            flags |= PL_STATUS_ICON_DEFENSE_UP;
        }
        if (pl->coolDrinkTimer > 0) {
            flags |= PL_STATUS_ICON_COOL_DRINK;
        }
        if (pl->hotDrinkTimer > 0) {
            flags |= PL_STATUS_ICON_HOT_DRINK;
        }
        if (Quest_DrillHasCharm() == 1) {
            flags |= PL_STATUS_ICON_DRILL_CHARM;
        }
        if (pl->status6Timer > 0) {
            flags |= PL_STATUS_ICON_STATUS6;
        }
        if (pl->status7Timer > 0) {
            flags |= PL_STATUS_ICON_STATUS7;
        }
        if (pl->regenTimer > 0) {
            flags |= PL_STATUS_ICON_REGEN;
        }
        if (pl->weaponType == WEAPON_DUAL_SWORDS && pl->demonMode != 0) {
            flags |= PL_STATUS_ICON_DEMON;
        }
        for (i = 0; i < 12; i++) {
            if (pl->songEffectTimer[i] > 0) {
                flags |= PL_STATUS_ICON_SONG;
            }
        }
        if (Quest_IsVersus() == 1 && pl->stateGroup == PL_GROUP_FAINT) {
            flags |= PL_STATUS_ICON_VERSUS_FAINT;
        }
        pl->statusIcons = flags;
    }
}

/*
 * The local player's attack power (Player.attack, at most 800) and its
 * factor (Player.attackFactor, attack / 100): the equipment's, the pouch
 * items that add to it, the drug and the buff, the attack skills, in a quest
 * (outside the lobby) the attack song effect (PL_SONG_ATTACK: x1.1, x1.15 or x1.25 by its level), and
 * at 40% health or less SKILL_LOW_HP_WEAKNESS or SKILL_LOW_HP_FURY. Another
 * player's attack is left as it is; only the factor is recomputed.
 */
void Player_CalcAttack(Player *pl)
{
    s32 atk = 0;

    if (Player_IsLocal(pl) == 1) {
        atk += Player_GetEquipStat(pl, EQUIP_STAT_ATTACK);
        if (Pouch_Count(pl, ITEM_ID_POUCH_ATTACK_6) != 0) {
            atk += 6;
        }
        if (Pouch_Count(pl, ITEM_ID_POUCH_ATTACK_9) != 0) {
            atk += 9;
        }
        atk += pl->attackDrug;
        atk += pl->attackBuff;
        if (Player_HasSkill(pl, SKILL_ATTACK_PLUS3) == 1) {
            atk += 3;
        } else if (Player_HasSkill(pl, SKILL_ATTACK_PLUS5) == 1) {
            atk += 5;
        } else if (Player_HasSkill(pl, SKILL_ATTACK_PLUS10) == 1) {
            atk += 10;
        }
        if (gScene.inLobby == 0 && Player_HasSongEffect(pl, PL_SONG_ATTACK) == 1) {
            switch (pl->songEffect[PL_SONG_ATTACK]) {
            case 1:
                atk = 1.1f * atk;
                break;
            case 2:
            case 3:
                atk = 1.15f * atk;
                break;
            case 4:
                atk = 1.25f * atk;
                break;
            }
        }
        if (Player_HasSkill(pl, SKILL_LOW_HP_WEAKNESS) == 1) {
            if ((f32)pl->hp / (f32)pl->hpMax <= 0.4f) {
                atk = 0.7f * atk;
            }
        } else if (Player_HasSkill(pl, SKILL_LOW_HP_FURY) == 1) {
            if ((f32)pl->hp / (f32)pl->hpMax <= 0.4f) {
                atk = 1.5f * atk;
            }
        }
        if (atk < 0) {
            atk = 1;
        }
        pl->attack = atk;
        if (pl->attack >= 800) {
            pl->attack = 800;
        }
    }
    pl->attackFactor = pl->attack / 100.0f;
}

/* Player_CalcAttack for another player, from the equipment data its console
 * sent: without the song effect and the low-health skills. */
void Player_CalcAttackRemote(Player *pl)
{
    s32 atk = 0;

    EquipView_FromPlayer(pl);
    atk += EquipView_GetStat(EQUIP_STAT_ATTACK);
    if (Pouch_Count(pl, ITEM_ID_POUCH_ATTACK_6) != 0) {
        atk += 6;
    }
    if (Pouch_Count(pl, ITEM_ID_POUCH_ATTACK_9) != 0) {
        atk += 9;
    }
    atk += pl->attackDrug;
    atk += pl->attackBuff;
    if (Player_HasSkill(pl, SKILL_ATTACK_PLUS3) == 1) {
        atk += 3;
    } else if (Player_HasSkill(pl, SKILL_ATTACK_PLUS5) == 1) {
        atk += 5;
    } else if (Player_HasSkill(pl, SKILL_ATTACK_PLUS10) == 1) {
        atk += 10;
    }
    if (atk < 0) {
        atk = 1;
    }
    pl->attack = atk;
    if (pl->attack >= 800) {
        pl->attack = 800;
    }
    pl->attackFactor = pl->attack / 100.0f;
}

/*
 * The player's defense (Player.defense) and its factor
 * (Player.defenseFactor: defense / 2, at least 1): 1 plus the equipment's,
 * the pouch items that add to it, the drug and the buff, the defense skills
 * (at least 1 after them), at 40% health or less 60 more (42 with
 * SKILL_LOW_HP_WEAKNESS, 90 with SKILL_LOW_HP_DEFENSE or SKILL_LOW_HP_FURY),
 * and in a quest the defense song effect (PL_SONG_DEFENSE: x1.1, x1.15 or x1.25).
 */
void Player_CalcDefense(Player *pl)
{
    s32 def = 1;

    def += Player_GetEquipStat(pl, EQUIP_STAT_DEFENSE);
    if (Pouch_Count(pl, ITEM_ID_POUCH_DEFENSE_16) != 0) {
        def += 16;
    }
    if (Pouch_Count(pl, ITEM_ID_POUCH_DEFENSE_24) != 0) {
        def += 24;
    }
    def += pl->defenseDrug;
    def += pl->defenseBuff;
    if (Player_HasSkill(pl, SKILL_DEFENSE_PLUS20) == 1) {
        def += 20;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_PLUS30) == 1) {
        def += 30;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_PLUS40) == 1) {
        def += 40;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_MINUS20) == 1) {
        def -= 20;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_MINUS30) == 1) {
        def -= 30;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_MINUS40) == 1) {
        def -= 40;
    }
    if (def <= 0) {
        def = 1;
    }
    if ((f32)pl->hp / (f32)pl->hpMax <= 0.4f) {
        if (Player_HasSkill(pl, SKILL_LOW_HP_WEAKNESS) == 1) {
            def += 42;
        } else if (Player_HasSkill(pl, SKILL_LOW_HP_DEFENSE) == 1 || Player_HasSkill(pl, SKILL_LOW_HP_FURY) == 1) {
            def += 90;
        } else {
            def += 60;
        }
    }
    if (gScene.inLobby == 0 && Player_HasSongEffect(pl, PL_SONG_DEFENSE) == 1) {
        switch (pl->songEffect[PL_SONG_DEFENSE]) {
        case 1:
            def = 1.1f * def;
            break;
        case 2:
        case 3:
            def = 1.15f * def;
            break;
        case 4:
            def = 1.25f * def;
            break;
        }
    }
    pl->defense = def;
    pl->defenseFactor = pl->defense / 2.0f;
    if (pl->defenseFactor < 1.0f) {
        pl->defenseFactor = 1.0f;
    }
}

/* Player_CalcDefense for another player, from the equipment data its
 * console sent: without the low-health bonus and the song effect. */
void Player_CalcDefenseRemote(Player *pl)
{
    s32 def = 1;

    EquipView_FromPlayer(pl);
    def += EquipView_GetStat(EQUIP_STAT_DEFENSE);
    if (Pouch_Count(pl, ITEM_ID_POUCH_DEFENSE_16) != 0) {
        def += 16;
    }
    if (Pouch_Count(pl, ITEM_ID_POUCH_DEFENSE_24) != 0) {
        def += 24;
    }
    def += pl->defenseDrug;
    def += pl->defenseBuff;
    if (Player_HasSkill(pl, SKILL_DEFENSE_PLUS20) == 1) {
        def += 20;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_PLUS30) == 1) {
        def += 30;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_PLUS40) == 1) {
        def += 40;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_MINUS20) == 1) {
        def -= 20;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_MINUS30) == 1) {
        def -= 30;
    } else if (Player_HasSkill(pl, SKILL_DEFENSE_MINUS40) == 1) {
        def -= 40;
    }
    if (def <= 0) {
        def = 1;
    }
    pl->defense = def;
    pl->defenseFactor = pl->defense / 2.0f;
    if (pl->defenseFactor < 1.0f) {
        pl->defenseFactor = 1.0f;
    }
}

/* Adds `others` to the five resistances, and `bonus` instead to the one the
 * song effect kind `elem` raises (PL_SONG_RES_FIRE to PL_SONG_RES_ICE). */
static void Player_AddResists(f32 bonus, Player *pl, s16 elem, f32 others)
{
    if (elem == 6) {
        pl->resist[0] += bonus;
    } else {
        pl->resist[0] += others;
    }
    if (elem == 7) {
        pl->resist[1] += bonus;
    } else {
        pl->resist[1] += others;
    }
    if (elem == 8) {
        pl->resist[2] += bonus;
    } else {
        pl->resist[2] += others;
    }
    if (elem == 9) {
        pl->resist[4] += bonus;
    } else {
        pl->resist[4] += others;
    }
    if (elem == 10) {
        pl->resist[3] += bonus;
    } else {
        pl->resist[3] += others;
    }
}

/* Applies the resistance skills: for each element, and then for all five,
 * the strongest raising skill held and the strongest lowering one. */
static void Player_ApplyResistSkills(Player *pl)
{
    if (Player_HasSkill(pl, SKILL_RES_FIRE_PLUS10) == 1) {
        pl->resist[0] += 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_FIRE_PLUS5) == 1) {
        pl->resist[0] += 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_FIRE_PLUS3) == 1) {
        pl->resist[0] += 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_FIRE_MINUS10) == 1) {
        pl->resist[0] -= 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_FIRE_MINUS5) == 1) {
        pl->resist[0] -= 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_FIRE_MINUS3) == 1) {
        pl->resist[0] -= 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_WATER_PLUS10) == 1) {
        pl->resist[1] += 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_WATER_PLUS5) == 1) {
        pl->resist[1] += 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_WATER_PLUS3) == 1) {
        pl->resist[1] += 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_WATER_MINUS10) == 1) {
        pl->resist[1] -= 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_WATER_MINUS5) == 1) {
        pl->resist[1] -= 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_WATER_MINUS3) == 1) {
        pl->resist[1] -= 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_THUNDER_PLUS10) == 1) {
        pl->resist[2] += 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_THUNDER_PLUS5) == 1) {
        pl->resist[2] += 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_THUNDER_PLUS3) == 1) {
        pl->resist[2] += 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_THUNDER_MINUS10) == 1) {
        pl->resist[2] -= 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_THUNDER_MINUS5) == 1) {
        pl->resist[2] -= 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_THUNDER_MINUS3) == 1) {
        pl->resist[2] -= 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_ICE_PLUS10) == 1) {
        pl->resist[3] += 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_ICE_PLUS5) == 1) {
        pl->resist[3] += 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_ICE_PLUS3) == 1) {
        pl->resist[3] += 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_ICE_MINUS10) == 1) {
        pl->resist[3] -= 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_ICE_MINUS5) == 1) {
        pl->resist[3] -= 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_ICE_MINUS3) == 1) {
        pl->resist[3] -= 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_DRAGON_PLUS10) == 1) {
        pl->resist[4] += 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_DRAGON_PLUS5) == 1) {
        pl->resist[4] += 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_DRAGON_PLUS3) == 1) {
        pl->resist[4] += 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_DRAGON_MINUS10) == 1) {
        pl->resist[4] -= 10.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_DRAGON_MINUS5) == 1) {
        pl->resist[4] -= 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_DRAGON_MINUS3) == 1) {
        pl->resist[4] -= 3.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_ALL_PLUS3) == 1) {
        pl->resist[0] += 3.0f;
        pl->resist[1] += 3.0f;
        pl->resist[2] += 3.0f;
        pl->resist[3] += 3.0f;
        pl->resist[4] += 3.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_ALL_PLUS5) == 1) {
        pl->resist[0] += 5.0f;
        pl->resist[1] += 5.0f;
        pl->resist[2] += 5.0f;
        pl->resist[3] += 5.0f;
        pl->resist[4] += 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_ALL_PLUS10) == 1) {
        pl->resist[0] += 10.0f;
        pl->resist[1] += 10.0f;
        pl->resist[2] += 10.0f;
        pl->resist[3] += 10.0f;
        pl->resist[4] += 10.0f;
    }
    if (Player_HasSkill(pl, SKILL_RES_ALL_MINUS3) == 1) {
        pl->resist[0] -= 3.0f;
        pl->resist[1] -= 3.0f;
        pl->resist[2] -= 3.0f;
        pl->resist[3] -= 3.0f;
        pl->resist[4] -= 3.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_ALL_MINUS5) == 1) {
        pl->resist[0] -= 5.0f;
        pl->resist[1] -= 5.0f;
        pl->resist[2] -= 5.0f;
        pl->resist[3] -= 5.0f;
        pl->resist[4] -= 5.0f;
    } else if (Player_HasSkill(pl, SKILL_RES_ALL_MINUS10) == 1) {
        pl->resist[0] -= 10.0f;
        pl->resist[1] -= 10.0f;
        pl->resist[2] -= 10.0f;
        pl->resist[3] -= 10.0f;
        pl->resist[4] -= 10.0f;
    }
}

/* Clamps the resistances to [-100, 100]. */
static void Player_ClampResists(Player *pl)
{
    if (pl->resist[0] < -100.0f) {
        pl->resist[0] = -100.0f;
    }
    if (pl->resist[1] < -100.0f) {
        pl->resist[1] = -100.0f;
    }
    if (pl->resist[2] < -100.0f) {
        pl->resist[2] = -100.0f;
    }
    if (pl->resist[3] < -100.0f) {
        pl->resist[3] = -100.0f;
    }
    if (pl->resist[4] < -100.0f) {
        pl->resist[4] = -100.0f;
    }
    if (pl->resist[0] > 100.0f) {
        pl->resist[0] = 100.0f;
    }
    if (pl->resist[1] > 100.0f) {
        pl->resist[1] = 100.0f;
    }
    if (pl->resist[2] > 100.0f) {
        pl->resist[2] = 100.0f;
    }
    if (pl->resist[3] > 100.0f) {
        pl->resist[3] = 100.0f;
    }
    if (pl->resist[4] > 100.0f) {
        pl->resist[4] = 100.0f;
    }
}

/* The local player's resistances: the equipment's, the skills, and the song
 * effects that raise one (+10 to its element and -5 to the others, +15/-8
 * or +20/-10 by the effect's level), within -100..100.
 * Its three helpers are static functions: only then does this function
 * compile as the game's code does. */
void Player_CalcResists(Player *pl)
{
    s16 i;
    s16 kind;
    s16 *kindPtr;

    pl->resist[0] = Player_GetEquipStat(pl, EQUIP_STAT_RES_FIRE);
    pl->resist[1] = Player_GetEquipStat(pl, EQUIP_STAT_RES_WATER);
    pl->resist[2] = Player_GetEquipStat(pl, EQUIP_STAT_RES_THUNDER);
    pl->resist[3] = Player_GetEquipStat(pl, EQUIP_STAT_RES_ICE);
    pl->resist[4] = Player_GetEquipStat(pl, EQUIP_STAT_RES_DRAGON);
    Player_ApplyResistSkills(pl);
    for (i = 0, kindPtr = gResistSongEffects; i < 5; i++, kindPtr++) {
        kind = *kindPtr;
        if (Player_HasSongEffect(pl, kind) == 1) {
            switch (pl->songEffect[kind]) {
            case 1:
                Player_AddResists(10.0f, pl, kind, -5.0f);
                break;
            case 2:
            case 3:
                Player_AddResists(15.0f, pl, kind, -8.0f);
                break;
            case 4:
                Player_AddResists(20.0f, pl, kind, -10.0f);
                break;
            }
        }
    }
    Player_ClampResists(pl);
}

/* Player_CalcResists for another player, from the equipment data its
 * console sent: without the song effects. */
void Player_CalcResistsRemote(Player *pl)
{
    EquipView_FromPlayer(pl);
    pl->resist[0] = EquipView_GetStat(EQUIP_STAT_RES_FIRE);
    pl->resist[1] = EquipView_GetStat(EQUIP_STAT_RES_WATER);
    pl->resist[2] = EquipView_GetStat(EQUIP_STAT_RES_THUNDER);
    pl->resist[3] = EquipView_GetStat(EQUIP_STAT_RES_ICE);
    pl->resist[4] = EquipView_GetStat(EQUIP_STAT_RES_DRAGON);
    Player_ApplyResistSkills(pl);
    Player_ClampResists(pl);
}

/* The points a decoration (by its item id) gives in skill group `group`:
 * the sum of its {group, points} pairs of that group; 0 when the item is not
 * a decoration. */
s8 Equip_SumModifiers(u16 item, u8 group)
{
    s8 sum = 0;
    DecoData *deco = Item_GetDecoration(item);

    if (deco != NULL) {
        if (deco->skills[0][0] == group) {
            sum += (s8)deco->skills[0][1];
        }
        if (deco->skills[1][0] == group) {
            sum += (s8)deco->skills[1][1];
        }
        if (deco->skills[2][0] == group) {
            sum += (s8)deco->skills[2][1];
        }
        if (deco->skills[3][0] == group) {
            sum += (s8)deco->skills[3][1];
        }
    }
    return sum;
}

/* The points a worn piece gives in skill group `group`: an armour piece's
 * own five {group, points} pairs, then its decorations' (a weapon's: the
 * decorations' only); 0 for an empty slot. */
s8 Equip_GetSkillPoints(EquipItem *piece, u8 group)
{
    s8 sum = 0;
    ArmorData *armor;
    s16 i;

    if (piece->id == 0) {
        return 0;
    }
    switch (piece->type) {
    case EQUIP_LEGS:
    case EQUIP_HEAD:
    case EQUIP_BODY:
    case EQUIP_ARMS:
    case EQUIP_WAIST:
        armor = gArmorTables[piece->type];
        armor += piece->id;
        if (armor->skills[0].skill == group) {
            sum += armor->skills[0].points;
        }
        if (armor->skills[1].skill == group) {
            sum += armor->skills[1].points;
        }
        if (armor->skills[2].skill == group) {
            sum += armor->skills[2].points;
        }
        if (armor->skills[3].skill == group) {
            sum += armor->skills[3].points;
        }
        if (armor->skills[4].skill == group) {
            sum += armor->skills[4].points;
        }
        for (i = 0; i < 3; i++) {
            if (piece->deco[i] != 0) {
                sum += Equip_SumModifiers(piece->deco[i], group);
            }
        }
        break;
    case EQUIP_BLADE:
    case EQUIP_GUN:
        for (i = 0; i < 3; i++) {
            if (piece->deco[i] != 0) {
                sum += Equip_SumModifiers(piece->deco[i], group);
            }
        }
        break;
    }
    return sum;
}

/* What a worn piece adds to the body armour's points of every skill: 1 if
 * it has points of SKILL_GROUP_TORSO_PLUS1, 2 more if of
 * SKILL_GROUP_TORSO_PLUS2; 0 for an empty slot. */
s8 Equip_TorsoUpBonus(EquipItem *piece)
{
    s8 bonus = 0;

    if (piece->id == 0) {
        return 0;
    }
    if (Equip_GetSkillPoints(piece, SKILL_GROUP_TORSO_PLUS1) > 0) {
        bonus++;
    }
    if (Equip_GetSkillPoints(piece, SKILL_GROUP_TORSO_PLUS2) > 0) {
        bonus += 2;
    }
    return bonus;
}

/* The body armour's points in skill group `group`: counted once more for
 * each worn piece (the body's own included) with points of
 * SKILL_GROUP_TORSO_UP, then each piece's Equip_TorsoUpBonus added; 0 when
 * the body armour has none. */
s8 Player_GetTorsoSkillPoints(Player *pl, u8 group)
{
    s8 points = Equip_GetSkillPoints(&pl->equip[EQUIP_BODY], group);
    s8 factor;

    if (points != 0) {
        factor = 1;
        if (Equip_GetSkillPoints(&pl->equip[EQUIP_HEAD], SKILL_GROUP_TORSO_UP)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[EQUIP_BODY], SKILL_GROUP_TORSO_UP)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[EQUIP_ARMS], SKILL_GROUP_TORSO_UP)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[EQUIP_WAIST], SKILL_GROUP_TORSO_UP)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[EQUIP_LEGS], SKILL_GROUP_TORSO_UP)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[EQUIP_SLOT_WEAPON], SKILL_GROUP_TORSO_UP)) {
            factor++;
        }
        points *= factor;
        points += Equip_TorsoUpBonus(&pl->equip[EQUIP_HEAD]);
        points += Equip_TorsoUpBonus(&pl->equip[EQUIP_BODY]);
        points += Equip_TorsoUpBonus(&pl->equip[EQUIP_ARMS]);
        points += Equip_TorsoUpBonus(&pl->equip[EQUIP_WAIST]);
        points += Equip_TorsoUpBonus(&pl->equip[EQUIP_LEGS]);
        points += Equip_TorsoUpBonus(&pl->equip[EQUIP_SLOT_WEAPON]);
    }
    return points;
}

/* The player's points in skill group `group` over all its equipment. */
s8 Player_GetSkillPoints(Player *pl, u8 group)
{
    s8 points = 0;

    points += Equip_GetSkillPoints(&pl->equip[EQUIP_HEAD], group);
    points += Equip_GetSkillPoints(&pl->equip[EQUIP_ARMS], group);
    points += Equip_GetSkillPoints(&pl->equip[EQUIP_WAIST], group);
    points += Equip_GetSkillPoints(&pl->equip[EQUIP_LEGS], group);
    points += Equip_GetSkillPoints(&pl->equip[EQUIP_SLOT_WEAPON], group);
    points += Player_GetTorsoSkillPoints(pl, group);
    return points;
}

/* Puts a skill in the next free one of the player's five skill slots, with
 * its group and points; nothing once all five are used. */
void Player_AddSkill(Player *pl, u8 skill, u8 *count, u16 group, s8 points)
{
    if (*count < 5) {
        pl->skills[*count] = skill;
        pl->skillGroups[*count] = group;
        pl->skillPoints[*count] = points;
        (*count)++;
    }
}

extern SkillDef gSkillTable[];

/*
 * Fills the player's five skill slots from its equipment: for each group
 * of gSkillTable, the skill its thresholds give for the player's points in
 * it (the first five found). Then sorts the slots: the skills from positive
 * points first, by points, then those from negative points, by how far
 * below zero, ties by the higher group, empty slots last. Each slot's
 * skillPoints keeps the sort key (points + 200, or 100 - points).
 */
void Player_CalcSkills(Player *pl)
{
    s16 sortKey[5];
    u8 groups[5];
    u8 ids[5];
    u8 count;
    SkillDef *def;
    s8 points;
    s32 i;
    s32 j;
    s16 keyTmp;
    u8 swap;

    def = gSkillTable;
    count = 0;
    for (i = 0; i < 5; i++) {
        pl->skills[i] = 0;
        pl->skillGroups[i] = 0;
        pl->skillPoints[i] = 0;
    }
    while (def->group != SKILL_GROUP_END) {
        points = Player_GetSkillPoints(pl, def->group);
        if (points <= def->neg[0][0]) {
            Player_AddSkill(pl, def->neg[0][1], &count, def->group, points);
        } else if (points <= def->neg[1][0]) {
            Player_AddSkill(pl, def->neg[1][1], &count, def->group, points);
        } else if (points <= def->neg[2][0]) {
            Player_AddSkill(pl, def->neg[2][1], &count, def->group, points);
        } else if (points >= def->pos[2][0]) {
            Player_AddSkill(pl, def->pos[2][1], &count, def->group, points);
        } else if (points >= def->pos[1][0]) {
            Player_AddSkill(pl, def->pos[1][1], &count, def->group, points);
        } else if (points >= def->pos[0][0]) {
            Player_AddSkill(pl, def->pos[0][1], &count, def->group, points);
        }
        def++;
    }
    for (i = 0; i < 5; i++) {
        if (pl->skillPoints[i] != 0) {
            if (pl->skillPoints[i] > 0) {
                sortKey[i] = (s16)pl->skillPoints[i] + 200;
                groups[i] = pl->skillGroups[i];
                ids[i] = pl->skills[i];
            } else {
                sortKey[i] = (s16)-pl->skillPoints[i] + 100;
                groups[i] = pl->skillGroups[i];
                ids[i] = pl->skills[i];
            }
        } else {
            sortKey[i] = 0;
            groups[i] = 0;
            ids[i] = 0;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = i + 1; j < 5; j++) {
            if (sortKey[i] < sortKey[j] || (sortKey[i] == sortKey[j] && groups[i] < groups[j])) {
                keyTmp = sortKey[i];
                swap = groups[i];
                sortKey[i] = sortKey[j];
                groups[i] = groups[j];
                sortKey[j] = keyTmp;
                groups[j] = swap;
                swap = ids[i];
                ids[i] = ids[j];
                ids[j] = swap;
            }
        }
    }
    for (i = 0; i < 5; i++) {
        pl->skillPoints[i] = sortKey[i];
        pl->skillGroups[i] = groups[i];
        pl->skills[i] = ids[i];
    }
}

/* Nonzero when the player has the skill: in one of its five slots, or, in
 * a quest (outside the lobby), as the local player's team combo skill
 * (Player.teamSkill, which counts for every hunter). In the lobby, no one
 * has any skill while Town_InRecordsMode holds. */
s32 Player_HasSkill(Player *pl, u8 skill)
{
    Player *local = &gPlayers[gScene.localPlayer];
    s16 i;

    if (gScene.inLobby != 0 && Town_InRecordsMode() == 1) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        if (pl->skills[i] == skill) {
            return 1;
        }
    }
    if (gScene.inLobby == 0 && local->teamSkill == skill) {
        return 1;
    }
    return 0;
}

/* The player's skill of skill group `group` (its slots, then in a quest
 * the local player's team combo skill); 0 for none, and in the lobby
 * while Town_InRecordsMode holds. */
u8 Player_GetSkill(Player *pl, u8 group)
{
    Player *local = &gPlayers[gScene.localPlayer];
    s16 i;

    if (gScene.inLobby != 0 && Town_InRecordsMode() == 1) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        if (pl->skillGroups[i] == group) {
            return pl->skills[i];
        }
    }
    if (gScene.inLobby == 0 && local->teamSkillGroup == group) {
        return local->teamSkill;
    }
    return 0;
}

/* Nonzero for the weapons that can guard: the great sword, the sword and
 * shield, the lance, the gunlance, and a heavy bowgun with a shield. */
s32 Player_CanGuard(Player *pl)
{
    switch (pl->weaponType) {
    case WEAPON_GREAT_SWORD:
    case WEAPON_SWORD_SHIELD:
    case WEAPON_LANCE:
    case WEAPON_GUNLANCE:
        return 1;
    case WEAPON_HEAVY_BOWGUN:
        if (HeavyBowgun_HasShield(pl) == 1) {
            return 1;
        }
    default:
        return 0;
    }
}
