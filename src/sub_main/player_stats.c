/*
 * The player's stats, motions and body physics.
 *
 * - Stats: the attack and defense (Player.attack and Player.defense, with
 *   their factors) from the equipment, the pouch, the drugs and buffs, the
 *   skills and the song effects (Player.songEffect, which only the hunting
 *   horn's songs set); the five element resistances (Player.resist); the
 *   skills (Player.skills: the points the equipment gives in each skill
 *   group, turned into skills by gSkillTable); the HUD's status icons.
 *   Each stat has a version for another player, from the equipment data
 *   that player's console sent (EquipView_*).
 * - Motions: the motion layers' ids, blends and frames (Player.motionId ...).
 * - Movement: velocity and acceleration for the player, for any object with
 *   the shared header (ObjHead), and for the attack objects (ObjAttach),
 *   whose attack rows are started here too.
 * - Bodies: hit shapes placed by bone or by position (HitShape), and the
 *   push that keeps players and monsters from overlapping (Field_PushApart).
 * - Areas and the floor: which area an object is in, the floor ahead of the
 *   player, the gathering point the player stands at.
 *
 * Several of these functions also run on monsters, read through the Player
 * struct: the fields they use sit at the same offsets in Player and Em
 * (the shared ones are listed by ObjHead).
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

/* The game's compiler built this file at optimization level 4: its code
 * matches only at that level. */
#pragma optimization_level 4

extern s16 gResistSongEffects[5];       /* the song effect kind of each resistance, in Player.resist's order */

s32 Quest_IsDrill(void);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_SetFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);
s16 Pouch_Count(Player *p, u16 item);
s32 Player_InMashStatus(Player *p);
s32 Player_HasSongEffect(Player *pl, s16 kind);
s32 Quest_IsVersus(void);
s32 Quest_DrillHasCharm(void);
s16 Player_GetEquipStat(Player *p, s32 kind);
s16 EquipView_GetStat(s32 kind);
void EquipView_FromPlayer(Player *p);
s32 Town_InMode46(void);
s32 HeavyBowgun_HasShield(Player *p);
struct DecoData *Item_GetDecoration(u16 item);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
/* unit's own declaration: u16 and s16 as Player_StartMotionLayer's call converts its arguments; with the definition's s32 the call compiles differently */
void Player_BeginLayerMotion(Player *pl, u16 frame, s16 blend, s32 layer);
f32 AreaGrid2_GetFloorY(f32 *pos);
AreaRecord *Stage_GetAreaRecord(u16 x);
s8 Equip_GetSkillPoints(EquipItem *e, u8 kind);
static void Player_AddResists(f32 bonus, Player *p, s16 elem, f32 v);
static void Player_ApplyResistSkills(Player *p);
static void Player_ClampResists(Player *p);
s8 Equip_TorsoUpBonus(EquipItem *e);
s8 Player_GetTorsoSkillPoints(Player *p, u8 kind);

void *Gather_GetPoints(s32 area);
f32 Math_Sqrtf(f32 x);
extern Em gEms[EM_COUNT];
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Mtx_FromDirection(f32 *dir, f32 *m, u8 axis);
void Capsule_Copy(HitCapsule *src, Capsule *dst);
s32 Capsule_PushCapsule(Capsule *a, Capsule *b, f32 *push);
void Player_BodyCapsule(Player *p, HitCapsule *cap, f32 r);
s32 Em_GetFloorYAbove(struct Em *obj, f32 *pos, f32 *out);
s32 Player_IsLocal(void *obj);
s32 Player_HasSkill(Player *p, u8 skill);

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

extern ArmorData *gArmorTables[];

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
 * has any skill while Town_InMode46 holds. */
s32 Player_HasSkill(Player *pl, u8 skill)
{
    Player *local = &gPlayers[gScene.localPlayer];
    s16 i;

    if (gScene.inLobby != 0 && Town_InMode46() == 1) {
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
 * while Town_InMode46 holds. */
u8 Player_GetSkill(Player *pl, u8 group)
{
    Player *local = &gPlayers[gScene.localPlayer];
    s16 i;

    if (gScene.inLobby != 0 && Town_InMode46() == 1) {
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

f32 Vec_Length(f32 *v);
void Vec_Normalize(f32 *v);
s32 BoneHit_Place(ObjHead *o, HitShape *def, HitCapsule *c, HitSphere *s);
s32 HitShape_Place(f32 *pos, HitShape *e, HitCapsule *cap, HitSphere *sph, f32 s);
s32 Capsule_PushSphere(Capsule *cap, f32 *p, f32 *out, f32 r);
void HitList_Next(Em *p, HitShape **cur);
extern HitShape *gEmBoneHits[];
extern HitShape *gEmBoneHitsLobby[];

/*
 * Pushes a player and a monster apart. The player is its body capsule
 * (Player_BodyCapsule, radius 40); the monster is its kind's hit shapes
 * (gEmBoneHits, or gEmBoneHitsLobby in the lobby), placed by its bones, less the
 * shapes flagged as not pushing. Up to two overlaps are summed, at the
 * longest one's length; the height counts only while the player is
 * airborne (PL_POSTURE_AIRBORNE), and never upward while it rises in state groups 2
 * and 3. The body with the lower pushPriority moves (both halfway when
 * they are equal) and is marked pushed. The player then stays between its
 * floor and 480 above it (unless it started higher).
 * The monster is read through the Player struct: the fields used (its kind
 * as weaponType, pos, pushPriority, pushed) sit at the same offsets in Em.
 */
void PlayerEm_PushApart(Player *pl, Player *em)
{
    HitShape *shape;
    f32 pushLen[2];
    Capsule bodyCap;
    Capsule shapeCap;
    HitCapsule bodyHit;
    HitCapsule shapeHit;
    HitSphere shapeSphere;
    f32 push[2][3];
    f32 startY;
    s16 count;
    s16 i;
    s32 boneNo;
    s16 shapeKind;
    s32 hit;

    startY = pl->pos[1];
    count = 0;
    Player_BodyCapsule(pl, &bodyHit, 40.0f);
    Capsule_Copy(&bodyHit, &bodyCap);
    if (gScene.inLobby == 0) {
        shape = gEmBoneHits[em->weaponType];
    } else {
        shape = gEmBoneHitsLobby[em->weaponType];
    }
    while ((boneNo = shape->bone) != -1) {
        if ((shape->atkFlags & HITSHAPE_TRAP_ONLY) || (shape->atkFlags2 & HITSHAPE2_HOLD_ONLY) || (shape->openFlags & HITSHAPE_HERMIT_ONLY) || (shape->openFlags & HITSHAPE_WIND_ONLY)
            || (shape->offFlags & HITSHAPE_NO_PUSH) || (shape->offFlags & HITSHAPE_NO_HIT)) {
            shape++;
            continue;
        }
        switch (boneNo) {
        case 0x7F: /* a shape at the monster's position */
        case 0x7E:
            shapeKind = HitShape_Place(em->pos, shape, &shapeHit, &shapeSphere, 1.0f);
            break;
        case 0x7D: /* a group's header: on to the group's shapes */
            HitList_Next((Em *)em, &shape);
            continue;
        default:
            shapeKind = BoneHit_Place((ObjHead *)em, shape, &shapeHit, &shapeSphere);
            break;
        }
        if (shapeKind == 0) {
            hit = (u8)Capsule_PushSphere(&bodyCap, shapeSphere.pos, push[count], shapeSphere.radius);
        } else {
            Capsule_Copy(&shapeHit, &shapeCap);
            hit = (u8)Capsule_PushCapsule(&shapeCap, &bodyCap, push[count]);
        }
        if ((u8)hit) {
            pushLen[count] = Vec_Length(push[count]);
            if (pl->action != PL_POSTURE_AIRBORNE) {
                push[count][1] = 0.0f;
            }
            count++;
        }
        if (count >= 2) {
            break;
        }
        shape++;
    }
    if (count != 0) {
        for (i = 1; i < count; i++) {
            push[0][0] += push[i][0];
            push[0][1] += push[i][1];
            push[0][2] += push[i][2];
            if (pushLen[0] < pushLen[i]) {
                pushLen[0] = pushLen[i];
            }
        }
        Vec_Normalize(push[0]);
        push[0][0] *= pushLen[0];
        push[0][1] *= pushLen[0];
        push[0][2] *= pushLen[0];
        if ((pl->stateGroup == 2 || pl->stateGroup == 3) && pl->vel[1] > 0.0f && push[0][1] > 0.0f) {
            push[0][1] = 0.0f;
        }
        if (em->pushPriority < pl->pushPriority) {
            em->pos[0] -= push[0][0];
            em->pos[1] -= push[0][1];
            em->pos[2] -= push[0][2];
            em->pushed = 1;
        } else if (em->pushPriority == pl->pushPriority) {
            pl->pos[0] += 0.5f * push[0][0];
            pl->pos[1] += 0.5f * push[0][1];
            pl->pos[2] += 0.5f * push[0][2];
            /* A height check with nothing in it: the game's code still
             * evaluates it, and the compiler orders the stores around it. */
            if (pl->pos[1] < pl->floorY || pl->pos[1] > pl->floorY + 480.0f) {
            }
            em->pos[0] += -0.5f * push[0][0];
            em->pos[1] += -0.5f * push[0][1];
            em->pos[2] += -0.5f * push[0][2];
            pl->pushed = 1;
            em->pushed = 1;
        } else {
            pl->pos[0] += push[0][0];
            pl->pos[1] += push[0][1];
            pl->pos[2] += push[0][2];
            pl->pushed = 1;
        }
        if (pl->pos[1] < pl->floorY) {
            pl->pos[1] = pl->floorY;
        } else if (startY < pl->floorY + 480.0f && pl->pos[1] > pl->floorY + 480.0f) {
            pl->pos[1] = pl->floorY + 480.0f;
        }
    }
}

s32 Sphere_PushOut(f32 ra, f32 rb, f32 *a, f32 *b, f32 *out);
extern f32 *gEmPushSpheres[];
extern f32 *gEmPushSpheresLobby[];

/*
 * Pushes two monsters apart when their push spheres touch: each kind's list
 * in gEmPushSpheres (or gEmPushSpheresLobby in the lobby) of {x, y, z, radius}
 * records, ended by radius -1, offset from the monster's position. Both
 * lists are scaled by the first monster's scale, the radii each by its
 * own. Up to two overlaps are summed, at the longest one's length; the
 * height counts only for a monster in the air (stance EM_STANCE_AIR). Both move halfway
 * when their pushPriority is equal, else the one with the lower; the moved
 * ones are marked pushed. Both are read through the Player struct, whose
 * fields used here sit at the same offsets in Em.
 */
void EmEm_PushApart(Player *emA, Player *emB)
{
    f32 centerA[3];
    f32 centerB[3];
    f32 pushLen[2];
    f32 push[2][3];
    f32 *spheresA;
    f32 *spheresB;
    f32 radiusA;
    s16 count;
    s16 i;

    count = 0;
    if (gScene.inLobby == 0) {
        spheresA = gEmPushSpheres[emA->weaponType];
        spheresB = gEmPushSpheres[emB->weaponType];
    } else {
        spheresA = gEmPushSpheresLobby[emA->weaponType];
        spheresB = gEmPushSpheresLobby[emB->weaponType];
    }
    for (; spheresA[3] != -1.0f; spheresA += 4) {
        centerA[0] = emA->pos[0] + spheresA[0] * emA->scale[0];
        centerA[1] = emA->pos[1] + spheresA[1] * emA->scale[1];
        centerA[2] = emA->pos[2] + spheresA[2] * emA->scale[2];
        radiusA = spheresA[3] * emA->scale[0];
        for (; spheresB[3] != -1.0f; spheresB += 4) {
            centerB[0] = emB->pos[0] + spheresB[0] * emA->scale[0];
            centerB[1] = emB->pos[1] + spheresB[1] * emA->scale[1];
            centerB[2] = emB->pos[2] + spheresB[2] * emA->scale[2];
            if (Sphere_PushOut(radiusA, spheresB[3] * emB->scale[0], centerA, centerB, push[count]) != 0) {
                pushLen[count] = Vec_Length(push[count]);
                count++;
            }
            if (count >= 2) {
                goto done; /* leaves both loops */
            }
        }
    }
done:
    if (count != 0) {
        for (i = 1; i < count; i++) {
            push[0][0] += push[i][0];
            push[0][1] += push[i][1];
            push[0][2] += push[i][2];
            if (pushLen[0] < pushLen[i]) {
                pushLen[0] = pushLen[i];
            }
        }
        if (emA->action != EM_STANCE_AIR) {
            push[0][1] = 0.0f;
        }
        Vec_Normalize(push[0]);
        push[0][0] *= pushLen[0];
        push[0][1] *= pushLen[0];
        push[0][2] *= pushLen[0];
        if (emA->pushPriority == emB->pushPriority) {
            emA->pos[0] += 0.5f * push[0][0];
            if (emA->action == EM_STANCE_AIR) {
                emA->pos[1] += 0.5f * push[0][1];
            }
            emA->pos[2] += 0.5f * push[0][2];
            emB->pos[0] += -0.5f * push[0][0];
            if (emB->action == EM_STANCE_AIR) {
                emB->pos[1] += -0.5f * push[0][1];
            }
            emB->pos[2] += -0.5f * push[0][2];
            emA->pushed = 1;
            emB->pushed = 1;
        } else if (emA->pushPriority < emB->pushPriority) {
            emA->pos[0] += push[0][0];
            if (emA->action == EM_STANCE_AIR) {
                emA->pos[1] += push[0][1];
            }
            emA->pos[2] += push[0][2];
            emA->pushed = 1;
        } else {
            emB->pos[0] -= push[0][0];
            if (emB->action == EM_STANCE_AIR) {
                emB->pos[1] -= push[0][1];
            }
            emB->pos[2] -= push[0][2];
            emB->pushed = 1;
        }
    }
}

/* Pushes two players apart when their body capsules (Player_BodyCapsule,
 * radius 40) meet: half the push each across the ground; the height half
 * each when both are airborne (PL_POSTURE_AIRBORNE), else all of it to the one that
 * is; both marked pushed. */
void Players_PushApart(Player *plA, Player *plB)
{
    Capsule capA;
    Capsule capB;
    HitCapsule hitA;
    HitCapsule hitB;
    f32 push[3];

    Player_BodyCapsule(plA, &hitA, 40.0f);
    Capsule_Copy(&hitA, &capA);
    Player_BodyCapsule(plB, &hitB, 40.0f);
    Capsule_Copy(&hitB, &capB);
    if (Capsule_PushCapsule(&capB, &capA, push) != 0) {
        plA->pos[0] += 0.5f * push[0];
        plA->pos[2] += 0.5f * push[2];
        plB->pos[0] += -0.5f * push[0];
        plB->pos[2] += -0.5f * push[2];
        if (plA->action == PL_POSTURE_AIRBORNE && plB->action == PL_POSTURE_AIRBORNE) {
            plA->pos[1] += 0.5f * push[1];
            plB->pos[1] += -0.5f * push[1];
        } else if (plA->action == PL_POSTURE_AIRBORNE) {
            plA->pos[1] += push[1];
        } else if (plB->action == PL_POSTURE_AIRBORNE) {
            plB->pos[1] -= push[1];
        }
        plA->pushed = 1;
        plB->pushed = 1;
    }
}

s32 Obj_InCurrentArea(void *obj);
s32 Obj_SameArea(void *obj, Player *b);
s32 Debug_IsOn(s32 unused0);
f32 Vec_Length(f32 *v);
void PlayerEm_PushApart(Player *a, Player *b);
void EmEm_PushApart(Player *a, Player *b);
void Players_PushApart(Player *a, Player *b);

/*
 * Keeps bodies from overlapping, once a frame. Clears every player's and
 * monster's pushed mark. Each local player in the current area is pushed
 * against the other players (only with debug switch 0xA9 on) and against
 * the monsters in its area, its emDist set to its distance to each in turn.
 * Then each pair of monsters in the current area is pushed apart: never two
 * of kind 0x1D; otherwise only when one is of kind 0x1D, or when this
 * console runs both (Em.remote clear) and neither has Em.wallTouch set.
 * The monsters are walked through Player pointers, as the push functions
 * read them, so Em.remote is read as Player.startItemList, at the same
 * offset: read through an Em cast, the compiler assigns its registers
 * otherwise. Em.wallTouch is read unsigned, as the game's code loads it.
 * diffY and diffZ point into diff, set up with the first loop: the game's
 * code keeps the two pointers in registers.
 */
void Field_PushApart(void)
{
    Em *em;
    Player *other;
    Player *emA;
    s32 i;
    s32 j;
    f32 *diffY;
    f32 *diffZ;
    f32 diff[3];
    s16 wallFlag;
    f32 dist;
    Player *pl;

    for (i = 0, other = gPlayers; i < SCENE_PLAYERS; i++, other++) {
        other->pushed = 0;
    }
    for (em = gEms, j = 0; j < EM_COUNT; j++, em++) {
        em->pushed = 0;
    }
    for (i = 0, pl = gPlayers, diffY = &diff[1], diffZ = &diff[2]; i < SCENE_PLAYERS; i++, pl++) {
        if (pl->active != 0 && pl->visible != 0 && Player_IsLocal(pl) != 0 && pl->noPushTimer == 0
            && (u8)Obj_InCurrentArea(pl)) {
            if (Debug_IsOn(0xA9) != 0) {
                for (j = 0, other = gPlayers; j < SCENE_PLAYERS; j++, other++) {
                    if (other->active != 0 && other->visible != 0 && other->noPushTimer == 0 && (u8)Obj_SameArea(pl, other)) {
                        Players_PushApart(pl, other);
                    }
                }
            }
            for (other = (Player *)gEms, j = 0; j < EM_COUNT; j++, other = (Player *)((u8 *)other + sizeof(Em))) {
                if (other->active != 0 && other->visible != 0 && other->noPushTimer == 0 && (u8)Obj_SameArea(pl, other)) {
                    PlayerEm_PushApart(pl, other);
                    diff[0] = pl->pos[0] - other->pos[0];
                    *diffY = pl->pos[1] - other->pos[1];
                    *diffZ = pl->pos[2] - other->pos[2];
                    dist = Vec_Length(diff);
                    pl->emDist = dist;
                }
            }
        }
    }
    for (i = 0, emA = (Player *)gEms; i < EM_COUNT - 1; i++, emA = (Player *)((u8 *)emA + sizeof(Em))) {
        if (emA->active != 0 && emA->visible != 0 && emA->noPushTimer == 0 && (u8)Obj_InCurrentArea(emA)) {
            wallFlag = (u16)((Em *)emA)->wallTouch != 0;
            for (j = i + 1, other = (Player *)&gEms[i + 1]; j < EM_COUNT; j++, other = (Player *)((u8 *)other + sizeof(Em))) {
                if (other->active != 0 && other->visible != 0) {
                    if (emA->weaponType == 0x1D && other->weaponType == 0x1D) {
                        continue;
                    }
                    if ((emA->weaponType == 0x1D || other->weaponType == 0x1D || (emA->startItemList == 0 && other->startItemList == 0))
                        && other->noPushTimer == 0 && (u8)Obj_SameArea(emA, other)) {
                        if ((wallFlag == 0 && (u16)((Em *)other)->wallTouch == 0) || emA->weaponType == 0x1D || other->weaponType == 0x1D) {
                            EmEm_PushApart(emA, other);
                        }
                    }
                }
            }
        }
    }
}

/* Sets motion layer `layer`'s motion id, blend (half of `blend`) and frame;
 * the layer starts the motion on its next update. */
static void Player_SetLayerMotion(Player *pl, s32 motion, s32 blend, s16 frame, s32 layer)
{
    pl->motionId[layer] = motion;
    pl->motionBlend[layer] = blend / 2;
    pl->motionFrame[layer] = frame;
    pl->motionStarted[layer] = 0;
}

/* Sets a motion on one layer (Player_SetLayerMotion), clearing
 * Player.unk771 as every motion start does. */
void Player_SetMotion(Player *pl, s32 motion, s32 blend, s16 frame, s32 layer)
{
    pl->unk771 = 0;
    Player_SetLayerMotion(pl, motion, blend, frame, layer);
}

/* Starts a motion on layers 0 and 1 (motion + 100). */
void Player_SetMotionPair(Player *pl, s32 motion, s32 blend, s16 frame)
{
    pl->unk771 = 0;
    Player_SetLayerMotion(pl, motion, blend, frame, 0);
    Player_SetLayerMotion(pl, motion + 100, blend, frame, 1);
}

/* Starts a motion on layer `layer` at once, when the player has that many
 * layers: the rotation matrix refreshed from Player.rot, the layer's motion
 * id, blend (half of `blend`) and frame set, and the layer begun
 * (Player_BeginLayerMotion). */
void Player_StartMotionLayer(Player *pl, u16 motion, s32 blend, s16 frame, s32 layer)
{
    if (layer < pl->motionLayerCount) {
        Mtx_FromAnglesXYZ(pl->rot, pl->rotMtx);
        pl->motionId[layer] = motion;
        pl->motionBlend[layer] = blend / 2;
        pl->motionFrame[layer] = frame;
        Player_BeginLayerMotion(pl, pl->motionFrame[layer], pl->motionBlend[layer], layer);
    }
}

/* Does nothing: the game's function is empty. Player_RunInputHook passes
 * it the buttons not held. */
void Player_InputHook(Player *pl, u16 buttons)
{
}

/* Runs Player_InputHook (which does nothing) with the buttons not held; the
 * town's input update calls it after reading the pad. */
void Player_RunInputHook(Player *pl)
{
    Player_InputHook(pl, ~pl->input.held);
}

/* Sets the vertical acceleration that brings the object's rise to a stop
 * in half of `frames` frames. Returns 1 when it stops within one frame
 * (frames 3 or fewer, or already falling), else 0. */
s32 Player_StartJump(void *obj, s16 frames)
{
    ObjHead *head = obj;
    f32 riseAccel;

    frames = frames / 2;
    riseAccel = -1.0f * head->vel[1];
    if (frames <= 1 || head->vel[1] < 0.0f) {
        head->accel[1] = riseAccel;
        return 1;
    }
    riseAccel /= frames;
    head->accel[1] = riseAccel;
    return 0;
}

/*
 * An attack row: the settings an attack object starts with, 0x18 bytes
 * copied over its fields from hitDelay to 0x77 (ObjAttach's names). Each
 * family of attack objects has a table of them.
 */
typedef struct AttackRow {
    u8 hitDelay;                /* 0x00 */
    u8 hitFrames;               /* 0x01 */
    u8 power;                   /* 0x02 */
    u8 attackKind;              /* 0x03 */
    u8 unk04;                   /* 0x04 */
    u8 attrs;                   /* 0x05 */
    s8 hitYawOfs;               /* 0x06 */
    u8 unk07;                   /* 0x07 */
    u8 hitForce;                /* 0x08 */
    u8 hitMarks;                /* 0x09 */
    u8 hitListNo;               /* 0x0A */
    u8 hitSe;                   /* 0x0B */
    u16 elementFlags;           /* 0x0C */
    u8 elementPower;            /* 0x0E */
    u8 hitFx;                   /* 0x0F */
    u8 hitStop;                 /* 0x10 */
    u8 swordGaugeAdd;           /* 0x11 */
    u8 multiHit;                /* 0x12 */
    u8 unk13;                   /* 0x13 */
    u8 stun;                    /* 0x14 */
    u8 unk15;                   /* 0x15: a frame count, halved like hitDelay; 0xFF kept */
    u8 unk16;                   /* 0x16: a frame count, halved like hitDelay */
    u8 unk17;                   /* 0x17 */
} AttackRow;

u8 HitId_Next(void);

void ObjAttach_LoadOwnedRow(ObjAttach *o, void *owner, s32 i);
void ObjAttach_LoadRow(ObjAttach *o, s32 i);

/* Moves the object by its velocity. */
void Player_Move(void *obj)
{
    ObjHead *head = obj;
    head->pos[0] += head->vel[0];
    head->pos[1] += head->vel[1];
    head->pos[2] += head->vel[2];
}

/* Adds the acceleration to the velocity, then moves the player by it. */
void Player_MoveAccel(Player *pl)
{
    pl->vel[0] += pl->accel[0];
    pl->vel[1] += pl->accel[1];
    pl->vel[2] += pl->accel[2];
    pl->pos[0] += pl->vel[0];
    pl->pos[1] += pl->vel[1];
    pl->pos[2] += pl->vel[2];
}

/* Player_MoveAccel for any object with the shared header. */
void Player_Integrate(void *obj)
{
    ObjHead *head = obj;
    head->vel[0] += head->accel[0];
    head->vel[1] += head->accel[1];
    head->vel[2] += head->accel[2];
    head->pos[0] += head->vel[0];
    head->pos[1] += head->vel[1];
    head->pos[2] += head->vel[2];
}

/* Moves as Player_MoveAccel, then stands the player on the floor
 * (AreaGrid2_GetFloorY) if it went below it: returns 1 then, else 0. */
s32 Player_MoveFall(Player *pl)
{
    f32 ground;

    pl->vel[0] += pl->accel[0];
    pl->vel[1] += pl->accel[1];
    pl->vel[2] += pl->accel[2];
    pl->pos[0] += pl->vel[0];
    pl->pos[1] += pl->vel[1];
    pl->pos[2] += pl->vel[2];
    ground = AreaGrid2_GetFloorY(pl->pos);
    if (pl->pos[1] <= ground) {
        pl->pos[1] = ground;
        return 1;
    }
    return 0;
}

/* Clears the object's acceleration and velocity. */
void Player_StopMove(void *obj)
{
    ObjHead *head = obj;
    head->accel[2] = 0.0f;
    head->accel[1] = 0.0f;
    head->accel[0] = 0.0f;
    head->vel[2] = 0.0f;
    head->vel[1] = 0.0f;
    head->vel[0] = 0.0f;
}

/* The same as Player_StopMove. */
void Player_ClearVelocity(void *obj)
{
    ObjHead *head = obj;
    head->accel[2] = 0.0f;
    head->accel[1] = 0.0f;
    head->accel[0] = 0.0f;
    head->vel[2] = 0.0f;
    head->vel[1] = 0.0f;
    head->vel[0] = 0.0f;
}

/* Clears the player's acceleration. */
void Player_ClearAccel(Player *pl)
{
    pl->accel[2] = 0.0f;
    pl->accel[1] = 0.0f;
    pl->accel[0] = 0.0f;
}

/* Player_ClearAccel for any object with the shared header. */
void Player_ClearAccel2(void *obj)
{
    ObjHead *head = obj;
    head->accel[2] = 0.0f;
    head->accel[1] = 0.0f;
    head->accel[0] = 0.0f;
}

/* Moves an attack object by its velocity. */
void ObjAttach_Move(ObjAttach *obj)
{
    obj->pos[0] += obj->velX;
    obj->pos[1] += obj->velY;
    obj->pos[2] += obj->velZ;
}

/* Adds an attack object's acceleration to its velocity, then moves it. The
 * acceleration is the three words from homingTurn: gravity is its y, and the
 * homing families use its x and z as their turn and speed instead. */
void ObjAttach_MoveAccel(ObjAttach *obj)
{
    obj->velX += obj->homingTurn;
    obj->velY += obj->gravity;
    obj->velZ += obj->homingSpeed;
    obj->pos[0] += obj->velX;
    obj->pos[1] += obj->velY;
    obj->pos[2] += obj->velZ;
}

/* Sets Player.motionTime from layer 0: its frame less the size of its
 * blend. With `step`, Player.hitDelay (a byte) is also lowered by that
 * count, and by one more while a blend is set. */
void Player_UpdateMotionTime(Player *pl, s16 step)
{
    s16 blend = pl->motionBlend[0];
    s16 len = pl->motionFrame[0];

    pl->motionTime = 0;
    if (blend < 0) {
        blend = -blend;
    }
    pl->motionTime += len - blend;
    if (step != 0) {
        if (blend != 0) {
            blend--;
        }
        pl->hitDelay += (u8)(blend - len);
    }
}

/* Player_UpdateMotionTime for an object passed untyped. */
void Motion_InitTime(void *obj, s16 step)
{
    Player *pl = obj;
    s16 blend = pl->motionBlend[0];
    s16 len = pl->motionFrame[0];

    pl->motionTime = 0;
    if (blend < 0) {
        blend = -blend;
    }
    pl->motionTime += len - blend;
    if (step != 0) {
        if (blend != 0) {
            blend--;
        }
        pl->hitDelay += (u8)(blend - len);
    }
}

/* Does nothing: the game's function is empty. Player_UpdateHitTimers calls
 * it with the hits left, where an attack object would start its next hit. */
void Player_StartNextHit(Player *unused0, u8 unused1)
{
}

/*
 * ObjAttach_LoadRow with an owner: its index (ObjHead.index) as the
 * object's ownerIndex. An element already on the object (the low byte of
 * elementFlags less ELEMF_EM_STATUS1, with its elementPower) stays, added
 * to the row's.
 */
void ObjAttach_LoadOwnedRow(ObjAttach *obj, void *owner, s32 row)
{
    ObjHead *ownerHead = owner;
    s32 *src;
    s32 *dst;
    s32 k;
    u8 keepFlags;
    u8 keepPower;

    keepFlags = obj->elementFlags & 0xF7;
    keepPower = keepFlags == 0 ? 0 : obj->elementPower;
    src = *(s32 **)obj->attackRows;
    src += row * 6;
    dst = (s32 *)&obj->hitDelay;
    for (k = 0; k < 6; k++) {
        *dst++ = *src++;
    }
    obj->ownerIndex = ownerHead->index;
    obj->hitPhase = 1;
    obj->didHit = 0;
    obj->targetEm = NULL;
    obj->targetPart = NULL;
    if (keepPower != 0) {
        obj->elementFlags |= keepFlags;
        obj->elementPower = keepPower;
    }
    obj->hitDelay >>= 1;
    obj->hitFrames >>= 1;
    if (obj->mvFramesA != 0xFF) {
        obj->mvFramesA >>= 1;
    }
    obj->mvFramesB >>= 1;
    obj->hitId = HitId_Next();
}

/* Starts an attack object on row `row` of the family's table `rows` (the
 * address of its table pointer), for the owner (a Player or an Em), with no
 * element carried over. */
void ObjAttach_StartOwnedAttack(ObjAttach *obj, s32 owner, s32 row, void *rows)
{
    obj->attackRows = rows;
    obj->elementPower = 0;
    obj->elementFlags = 0;
    ObjAttach_LoadOwnedRow(obj, (void *) owner, row);
}

/*
 * Copies attack row `row` of the object's table over its fields, word by
 * word, for an attack with no owner (ownerIndex 0xFF). The hit window waits
 * (hitPhase 1), nothing is hit or targeted yet, the row's frame counts are
 * halved, and the attack takes a new hit id.
 */
void ObjAttach_LoadRow(ObjAttach *obj, s32 row)
{
    s32 *src;
    s32 *dst;
    s32 k;

    src = *(s32 **)obj->attackRows;
    src += row * 6;
    dst = (s32 *)&obj->hitDelay;

    for (k = 0; k < 6; k++) {
        *dst++ = *src++;
    }
    obj->ownerIndex = 0xFF;
    obj->hitPhase = 1;
    obj->didHit = 0;
    obj->targetEm = NULL;
    obj->targetPart = NULL;
    obj->hitDelay >>= 1;
    obj->hitFrames >>= 1;
    if (obj->mvFramesA != 0xFF) {
        obj->mvFramesA >>= 1;
    }
    obj->mvFramesB >>= 1;
    obj->hitId = HitId_Next();
}

/* Starts an attack object with no owner on row `row` of the family's table
 * `rows` (the address of its table pointer). */
void ObjAttach_StartAttack(ObjAttach *obj, s32 row, void *rows)
{
    obj->attackRows = rows;
    ObjAttach_LoadRow(obj, row);
}


/* Places hit shape `def` in the world by its owner's bone: a sphere into
 * sph (returns 0) or a capsule into cap (returns 1); -1 for a shape not on
 * a bone (bone 0x7F) or of another kind. */
s32 BoneHit_Place(ObjHead *owner, HitShape *def, HitCapsule *cap, HitSphere *sph)
{
    f32 off[3];
    f32 pos[3];
    EmBone *bones;

    if (def->bone == 0x7F) {
        return -1;
    }
    bones = owner->model->bones;
    Mtx_GetTrans(pos, bones[def->bone].world);
    switch (def->kind) {
    case 0:
        sph->radius = def->radius * owner->scale[0];
        Vec_Rotate(off, def->a, (f32 *)&bones[def->bone]);
        sph->pos[0] = pos[0] + off[0];
        sph->pos[1] = pos[1] + off[1];
        sph->pos[2] = pos[2] + off[2];
        return 0;
    case 1:
        cap->radius = def->radius * owner->scale[0];
        Vec_Rotate(off, def->a, (f32 *)&bones[def->bone]);
        cap->a[0] = pos[0] + off[0];
        cap->a[1] = pos[1] + off[1];
        cap->a[2] = pos[2] + off[2];
        Vec_Rotate(off, def->b, (f32 *)&bones[def->bone]);
        cap->b[0] = pos[0] + off[0];
        cap->b[1] = pos[1] + off[1];
        cap->b[2] = pos[2] + off[2];
        return 1;
    }
    return -1;
}

/* Places a hit shape at pos, its radius scaled by `scale`: a sphere (kind
 * 0) into sph, returning 0, or a capsule (kind 1) into cap, returning 1;
 * -1 for another kind. */
s32 HitShape_Place(f32 *pos, HitShape *shape, HitCapsule *cap, HitSphere *sph, f32 scale)
{
    switch (shape->kind) {
    case 0:
        sph->radius = shape->radius * scale;
        sph->pos[0] = pos[0] + shape->a[0];
        sph->pos[1] = pos[1] + shape->a[1];
        sph->pos[2] = pos[2] + shape->a[2];
        return 0;
    case 1:
        cap->radius = shape->radius * scale;
        cap->a[0] = pos[0] + shape->a[0];
        cap->a[1] = pos[1] + shape->a[1];
        cap->a[2] = pos[2] + shape->a[2];
        cap->b[0] = pos[0] + shape->b[0];
        cap->b[1] = pos[1] + shape->b[1];
        cap->b[2] = pos[2] + shape->b[2];
        return 1;
    }
    return -1;
}

/* The capsule from start to end with the shape's radius scaled by `scale`;
 * returns 1. */
s32 Capsule_FromPoints(f32 *start, f32 *end, HitShape *shape, HitCapsule *out, f32 scale)
{
    out->radius = shape->radius * scale;
    out->a[0] = start[0];
    out->a[1] = start[1];
    out->a[2] = start[2];
    out->b[0] = end[0];
    out->b[1] = end[1];
    out->b[2] = end[2];
    return 1;
}

/* Looks for the floor `fwd` ahead of the player (along its heading). Returns
 * 1 with its height in *out when it lies between `rise` and `rise + range`
 * above the player's height; 0 when it lies elsewhere; 1, *out unchanged,
 * when there is no floor there. */
s32 Player_FloorAheadAbove(Player *pl, f32 *out, f32 fwd, f32 rise, f32 range)
{
    f32 mtx[16];
    f32 point[3];
    f32 offset[3];
    s32 angles[3];
    f32 floorY;
    f32 *pointY;
    f32 *pointZ;
    f32 minY;

    pointY = &point[1];
    pointZ = &point[2];
    point[0] = 0.0f;
    *pointY = 0.0f;
    *pointZ = fwd;
    angles[0] = 0;
    angles[1] = pl->rot[1];
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(offset, point, mtx);
    point[0] = pl->pos[0] + offset[0];
    *pointY = pl->pos[1] + offset[1];
    *pointZ = pl->pos[2] + offset[2];
    minY = *pointY + rise;
    if (Em_GetFloorYAbove((struct Em *)pl, point, &floorY) != 1) {
        return 1;
    }
    if (!(floorY < minY) && floorY <= minY + range) {
        *out = floorY;
        return 1;
    }
    return 0;
}

/* Compares the floor `fwd` ahead of the player (along its heading) with the
 * player's height plus `rise`: 1 when the floor is below it (above it, with
 * `above`), 0 when not or when there is no floor there. */
s32 Player_FloorAheadBelow(Player *pl, u16 above, f32 fwd, f32 rise)
{
    f32 mtx[16];
    f32 point[3];
    f32 offset[3];
    s32 angles[3];
    f32 floorY;
    f32 *pointY;
    f32 *pointZ;
    f32 refY;

    pointY = &point[1];
    pointZ = &point[2];
    point[0] = 0.0f;
    *pointY = 0.0f;
    *pointZ = fwd;
    angles[0] = 0;
    angles[1] = pl->rot[1];
    angles[2] = 0;
    Mtx_FromAnglesXYZ(angles, mtx);
    Vec_Rotate(offset, point, mtx);
    point[0] = pl->pos[0] + offset[0];
    *pointY = pl->pos[1] + offset[1];
    *pointZ = pl->pos[2] + offset[2];
    refY = pl->pos[1] + rise;
    if (Em_GetFloorYAbove((struct Em *)pl, point, &floorY) != 1) {
        return 0;
    }
    if (above == 0) {
        if (floorY < refY) {
            return 1;
        }
    } else if (!(floorY <= refY)) {
        return 1;
    }
    return 0;
}

/* Nonzero when the object is in the current area (gScene.area). */
s32 Obj_InCurrentArea(void *obj)
{
    ObjHead *head = obj;
    return head->area == gScene.area;
}

/* The same as Obj_InCurrentArea, as a byte. */
u8 Player_InCurrentArea(void *obj)
{
    ObjHead *head = obj;
    return head->area == gScene.area;
}

/* Nonzero when two players (or a player and a monster) are in the same area. */
s32 Obj_SameArea(void *obj, Player *other)
{
    ObjHead *head = obj;
    return head->area == other->area;
}

/* Sets the object's map position: its position in the area plus the area's
 * origin on the map (AreaRecord), the height from the middle of the area's
 * vertical range. */
void Player_UpdateAreaPos(void *obj)
{
    ObjHead *head = obj;
    AreaRecord *rec = Stage_GetAreaRecord(head->area);

    head->mapPos[0] = head->pos[0] + rec->originX;
    head->mapPos[1] = head->pos[1] + (rec->bottom + rec->top) / 2.0f;
    head->mapPos[2] = head->pos[2] + rec->originZ;
}

/* Player_UpdateAreaPos for a position in area `area`, into out. */
void Area_ToWorld(s32 area, f32 *pos, f32 *out)
{
    AreaRecord *rec = Stage_GetAreaRecord(area);

    out[0] = pos[0] + rec->originX;
    out[1] = pos[1] + (rec->bottom + rec->top) / 2.0f;
    out[2] = pos[2] + rec->originZ;
}

/* The gathering point the player stands at (within its radius, from 200
 * below it to 100 above): returns its item table, with its action
 * (MINE_ACTION_*) in *action and its position in pos; GATHER_NO_ITEM when
 * there is none. */
u16 Player_GetGatherPoint(Player *pl, u16 *action, f32 *pos)
{
    GatherPoint *point;
    f32 dx;
    f32 dz;
    u16 table;

    point = Gather_GetPoints(pl->area);
    if (point == NULL) {
        return GATHER_NO_ITEM;
    }
    for (; point->x != -1.0f; point++) {
        if (pl->pos[1] < point->y - 200.0f) {
            continue;
        }
        if (!(pl->pos[1] < point->y + 100.0f)) {
            continue;
        }
        dx = pl->pos[0] - point->x;
        dz = pl->pos[2] - point->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= point->radius)) {
            continue;
        }
        table = point->table;
        *action = point->action;
        pos[0] = point->x;
        pos[1] = point->y;
        pos[2] = point->z;
        return table;
    }
    return GATHER_NO_ITEM;
}
