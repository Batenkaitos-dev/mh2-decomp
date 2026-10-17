/*
 * Player stats and physics: attack (+0x630) and defense (+0x632) from the
 * equipment, items, skills and buffs; the element resistances (+0x878,
 * five floats, within +-100) from the equipment and the skills; the skill
 * slots (+0x868 ids, +0x894 groups, 5 slots); the HUD status flags
 * (+0x4B8); motion layers (+0x29C...); velocity and acceleration
 * (+0x3B8/+0x3C4); pushing apart from other players and monsters.
 */
#include "common.h"
#include "quest.h"
#include "player.h"
#include "scene.h"
#include "em.h"
#include "model.h"
#include "objhead.h"
#include "capsule.h"

/* The game built this file at optimization level 4. */
#pragma optimization_level 4

extern s16 gResistMealEffects[5];       /* the element numbers of the resistances */

/* A stat modifier record of the equipment data (Item_GetDecoration): up to four
 * {kind, value} pairs from +8. */
typedef struct StatMods {
    u8 unk0[8];
    u8 kind0;
    s8 value0;
    u8 kind1;
    s8 value1;
    u8 kind2;
    s8 value2;
    u8 kind3;
    s8 value3;
} StatMods;

s32 Quest_IsDrill(void);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Player_SetFlag(void *obj, u32 flag);
void Player_ClearFlag(void *obj, u32 flag);
s16 Pouch_Count(Player *p, u16 item);
s32 Player_InMashStatus(Player *p);
s32 Player_HasMealEffect(Player *pl, s16 kind);
s32 Quest_IsVersus(void);
s32 Quest_DrillHasCharm(void);
s16 Player_GetEquipStat(Player *p, s32 kind);
s16 EquipView_GetStat(s32 kind);
void EquipView_FromPlayer(Player *p);
s32 Town_InMode46(void);
s32 HeavyBowgun_HasShield(Player *p);
struct DecoData *Item_GetDecoration(u16 item);
f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
/* unit's own declaration: with the definition's void (Player *p, s32 frame, s32 n, s32 part) (sub_main/unk_2E3940.c), Player_StartMotionLayer: 1 of 42 words differ */
void Player_BeginLayerMotion(Player *p, u16 motion, s16 a, s32 layer);
f32 AreaGrid2_GetFloorY(f32 *pos);
AreaRecord *Stage_GetAreaRecord(u16 x);
s8 Equip_GetSkillPoints(EquipItem *e, u8 kind);
static void Player_AddResists(f32 bonus, Player *p, s16 elem, f32 v);
static void Player_ApplyResistSkills(Player *p);
static void Player_ClampResists(Player *p);
s8 Equip_TorsoUpBonus(EquipItem *e);
s8 Player_GetTorsoSkillPoints(Player *p, u8 kind);

/* A gathering point (gather.c): 24-byte records, x == -1.0f ends the list. */
typedef struct GatherPt {
    f32 x;                      /* 0x00 */
    f32 y;                      /* 0x04 */
    f32 z;                      /* 0x08 */
    f32 radius;                 /* 0x0C */
    u16 table;                  /* 0x10 */
    u16 count;                  /* 0x12 */
    u16 unk14;                  /* 0x14 */
    u16 bonus;                  /* 0x16 */
} GatherPt;

void *Gather_GetPoints(s32 area);
f32 Math_Sqrtf(f32 x);
extern Em gEms[20];
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Mtx_FromDirection(f32 *dir, f32 *m, u8 axis);
void Capsule_Copy(HitCapsule *src, Capsule *dst);
s32 Capsule_PushCapsule(Capsule *a, Capsule *b, f32 *push);
void Player_BodyCapsule(Player *p, HitCapsule *cap, f32 r);
s32 Em_GetFloorYAbove(struct Em *obj, f32 *pos, f32 *out);
s32 Player_IsLocal(void *obj);
s32 Player_HasSkill(Player *p, u8 skill);

/* +0x597: 1 when the player is near a large monster (its +0x596 at least 3: within
 * 300, or 500 above 3, horizontally and less than 300 below), unless +0x588 is set or
 * the player is more than 100 above its start height. */
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
        pl->unk597[0] = 0;
        return;
    }
    for (em = gEms, i = 0; i < 20; i++, em++) {
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
            pl->unk597[0] = 1;
            return;
        }
    }
    pl->unk597[0] = 0;
}

/* A capsule of radius r along the player's body: from the middle of records 5 and 8
 * of +0xD0 to record 20, both ends moved by r across the axis. */
void Player_BodyCapsule(Player *pl, HitCapsule *cap, f32 capRadius)
{
    f32 mtx[16];
    f32 part5[3];
    f32 part8[3];
    f32 axis[3];
    f32 offset[3];
    f32 *axisY;
    f32 *axisZ;

    Mtx_GetTrans(part5, pl->partRecs[5]->m1);
    Mtx_GetTrans(part8, pl->partRecs[8]->m1);
    cap->a[0] = (part5[0] + part8[0]) / 2.0f;
    cap->a[1] = (part5[1] + part8[1]) / 2.0f;
    cap->a[2] = (part5[2] + part8[2]) / 2.0f;
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

/* Sets the movement mode (+0x390: 0, 1 or 2) and the flags 8, 1 and 2. */
void Player_SetMoveMode(Player *pl, u16 mode, s16 setFlag1, s16 setFlag2)
{
    s32 modeBits = mode;

    switch ((u8)modeBits) {
    default:
        pl->action = 0;
        break;
    case 1:
        pl->action = 1;
        break;
    case 2:
        pl->action = 2;
        break;
    }
    if (modeBits & 0x8000) {
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

/* Nonzero for the player of this console: its number (+0x0C) is the local
 * one (gScene.localPlayer), or everyone counts as local (gScene.allLocal, offline). */
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
        if (Player_HasMealEffect(pl, 1) == 1) {
            flags |= PL_STATUS_ICON_ATTACK_UP;
        }
        if ((s16)(pl->defenseDrug + pl->defenseBuff) > 0) {
            flags |= PL_STATUS_ICON_DEFENSE_UP;
        }
        if (Player_HasMealEffect(pl, 2) == 1) {
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
            if (pl->mealEffectTimer[i] > 0) {
                flags |= PL_STATUS_ICON_MEAL;
            }
        }
        if (Quest_IsVersus() == 1 && pl->stateGroup == PL_GROUP_FAINT) {
            flags |= PL_STATUS_ICON_VERSUS_FAINT;
        }
        pl->statusIcons = flags;
    }
}

/*
 * The attack power (+0x630, 1..800) and its factor (+0x724, /100) of the
 * local player: the equipment's (Player_GetEquipStat(p, 0)), +6 and +9 for items
 * 0x23 and 0x24 in the pouch, the buffs +0x628/+0x629, the skills 0x4F-0x51
 * (+3, +5, +10), a meal effect (+0x8D7) offline, and the skills 0xB3 (x0.7)
 * and 0xB2 (x1.5) at 40% health or less.
 */
void Player_CalcAttack(Player *pl)
{
    s32 atk = 0;

    if (Player_IsLocal(pl) == 1) {
        atk += Player_GetEquipStat(pl, 0);
        if (Pouch_Count(pl, 0x23) != 0) {
            atk += 6;
        }
        if (Pouch_Count(pl, 0x24) != 0) {
            atk += 9;
        }
        atk += pl->attackDrug;
        atk += pl->attackBuff;
        if (Player_HasSkill(pl, 0x4F) == 1) {
            atk += 3;
        } else if (Player_HasSkill(pl, 0x50) == 1) {
            atk += 5;
        } else if (Player_HasSkill(pl, 0x51) == 1) {
            atk += 10;
        }
        if (gScene.online == 0 && Player_HasMealEffect(pl, 1) == 1) {
            switch (pl->mealEffect[1]) {
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
        if (Player_HasSkill(pl, 0xB3) == 1) {
            if ((f32)pl->hp / (f32)pl->hpMax <= 0.4f) {
                atk = 0.7f * atk;
            }
        } else if (Player_HasSkill(pl, 0xB2) == 1) {
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

/* Player_CalcAttack for another player, from the equipment data received. */
void Player_CalcAttackRemote(Player *pl)
{
    s32 atk = 0;

    EquipView_FromPlayer(pl);
    atk += EquipView_GetStat(0);
    if (Pouch_Count(pl, 0x23) != 0) {
        atk += 6;
    }
    if (Pouch_Count(pl, 0x24) != 0) {
        atk += 9;
    }
    atk += pl->attackDrug;
    atk += pl->attackBuff;
    if (Player_HasSkill(pl, 0x4F) == 1) {
        atk += 3;
    } else if (Player_HasSkill(pl, 0x50) == 1) {
        atk += 5;
    } else if (Player_HasSkill(pl, 0x51) == 1) {
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
 * The defense (+0x632) and its factor (+0x728, /2, at least 1) of the local
 * player: 1 + the equipment's, +16 and +24 for items 0x25 and 0x26, the
 * buffs, the skills 0x52-0x57 (+20, +30, +40, -20, -30, -40), a bonus at
 * 40% health or less (skill 0xB3: +42, 0xB1 or 0xB2: +90, else +60), and a
 * meal effect (+0x8D8) offline.
 */
void Player_CalcDefense(Player *pl)
{
    s32 def = 1;

    def += Player_GetEquipStat(pl, 1);
    if (Pouch_Count(pl, 0x25) != 0) {
        def += 16;
    }
    if (Pouch_Count(pl, 0x26) != 0) {
        def += 24;
    }
    def += pl->defenseDrug;
    def += pl->defenseBuff;
    if (Player_HasSkill(pl, 0x52) == 1) {
        def += 20;
    } else if (Player_HasSkill(pl, 0x53) == 1) {
        def += 30;
    } else if (Player_HasSkill(pl, 0x54) == 1) {
        def += 40;
    } else if (Player_HasSkill(pl, 0x55) == 1) {
        def -= 20;
    } else if (Player_HasSkill(pl, 0x56) == 1) {
        def -= 30;
    } else if (Player_HasSkill(pl, 0x57) == 1) {
        def -= 40;
    }
    if (def <= 0) {
        def = 1;
    }
    if ((f32)pl->hp / (f32)pl->hpMax <= 0.4f) {
        if (Player_HasSkill(pl, 0xB3) == 1) {
            def += 42;
        } else if (Player_HasSkill(pl, 0xB1) == 1 || Player_HasSkill(pl, 0xB2) == 1) {
            def += 90;
        } else {
            def += 60;
        }
    }
    if (gScene.online == 0 && Player_HasMealEffect(pl, 2) == 1) {
        switch (pl->mealEffect[2]) {
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

/* Player_CalcDefense for another player, from the equipment data received. */
void Player_CalcDefenseRemote(Player *pl)
{
    s32 def = 1;

    EquipView_FromPlayer(pl);
    def += EquipView_GetStat(1);
    if (Pouch_Count(pl, 0x25) != 0) {
        def += 16;
    }
    if (Pouch_Count(pl, 0x26) != 0) {
        def += 24;
    }
    def += pl->defenseDrug;
    def += pl->defenseBuff;
    if (Player_HasSkill(pl, 0x52) == 1) {
        def += 20;
    } else if (Player_HasSkill(pl, 0x53) == 1) {
        def += 30;
    } else if (Player_HasSkill(pl, 0x54) == 1) {
        def += 40;
    } else if (Player_HasSkill(pl, 0x55) == 1) {
        def -= 20;
    } else if (Player_HasSkill(pl, 0x56) == 1) {
        def -= 30;
    } else if (Player_HasSkill(pl, 0x57) == 1) {
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

/* Adds v to the five resistances, `bonus` instead for the element `elem`
 * (6 fire, 7 water, 8 thunder, 9 dragon, 10 ice). */
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

/* The resistance skills: +10/+5/+3 or -10/-5/-3 to one element, or to all
 * (0x60-0x65). */
static void Player_ApplyResistSkills(Player *pl)
{
    if (Player_HasSkill(pl, 0x68) == 1) {
        pl->resist[0] += 10.0f;
    } else if (Player_HasSkill(pl, 0x67) == 1) {
        pl->resist[0] += 5.0f;
    } else if (Player_HasSkill(pl, 0x66) == 1) {
        pl->resist[0] += 3.0f;
    }
    if (Player_HasSkill(pl, 0x6B) == 1) {
        pl->resist[0] -= 10.0f;
    } else if (Player_HasSkill(pl, 0x6A) == 1) {
        pl->resist[0] -= 5.0f;
    } else if (Player_HasSkill(pl, 0x69) == 1) {
        pl->resist[0] -= 3.0f;
    }
    if (Player_HasSkill(pl, 0x6E) == 1) {
        pl->resist[1] += 10.0f;
    } else if (Player_HasSkill(pl, 0x6D) == 1) {
        pl->resist[1] += 5.0f;
    } else if (Player_HasSkill(pl, 0x6C) == 1) {
        pl->resist[1] += 3.0f;
    }
    if (Player_HasSkill(pl, 0x71) == 1) {
        pl->resist[1] -= 10.0f;
    } else if (Player_HasSkill(pl, 0x70) == 1) {
        pl->resist[1] -= 5.0f;
    } else if (Player_HasSkill(pl, 0x6F) == 1) {
        pl->resist[1] -= 3.0f;
    }
    if (Player_HasSkill(pl, 0x7A) == 1) {
        pl->resist[2] += 10.0f;
    } else if (Player_HasSkill(pl, 0x79) == 1) {
        pl->resist[2] += 5.0f;
    } else if (Player_HasSkill(pl, 0x78) == 1) {
        pl->resist[2] += 3.0f;
    }
    if (Player_HasSkill(pl, 0x7D) == 1) {
        pl->resist[2] -= 10.0f;
    } else if (Player_HasSkill(pl, 0x7C) == 1) {
        pl->resist[2] -= 5.0f;
    } else if (Player_HasSkill(pl, 0x7B) == 1) {
        pl->resist[2] -= 3.0f;
    }
    if (Player_HasSkill(pl, 0x80) == 1) {
        pl->resist[3] += 10.0f;
    } else if (Player_HasSkill(pl, 0x7F) == 1) {
        pl->resist[3] += 5.0f;
    } else if (Player_HasSkill(pl, 0x7E) == 1) {
        pl->resist[3] += 3.0f;
    }
    if (Player_HasSkill(pl, 0x83) == 1) {
        pl->resist[3] -= 10.0f;
    } else if (Player_HasSkill(pl, 0x82) == 1) {
        pl->resist[3] -= 5.0f;
    } else if (Player_HasSkill(pl, 0x81) == 1) {
        pl->resist[3] -= 3.0f;
    }
    if (Player_HasSkill(pl, 0x74) == 1) {
        pl->resist[4] += 10.0f;
    } else if (Player_HasSkill(pl, 0x73) == 1) {
        pl->resist[4] += 5.0f;
    } else if (Player_HasSkill(pl, 0x72) == 1) {
        pl->resist[4] += 3.0f;
    }
    if (Player_HasSkill(pl, 0x77) == 1) {
        pl->resist[4] -= 10.0f;
    } else if (Player_HasSkill(pl, 0x76) == 1) {
        pl->resist[4] -= 5.0f;
    } else if (Player_HasSkill(pl, 0x75) == 1) {
        pl->resist[4] -= 3.0f;
    }
    if (Player_HasSkill(pl, 0x60) == 1) {
        pl->resist[0] += 3.0f;
        pl->resist[1] += 3.0f;
        pl->resist[2] += 3.0f;
        pl->resist[3] += 3.0f;
        pl->resist[4] += 3.0f;
    } else if (Player_HasSkill(pl, 0x61) == 1) {
        pl->resist[0] += 5.0f;
        pl->resist[1] += 5.0f;
        pl->resist[2] += 5.0f;
        pl->resist[3] += 5.0f;
        pl->resist[4] += 5.0f;
    } else if (Player_HasSkill(pl, 0x62) == 1) {
        pl->resist[0] += 10.0f;
        pl->resist[1] += 10.0f;
        pl->resist[2] += 10.0f;
        pl->resist[3] += 10.0f;
        pl->resist[4] += 10.0f;
    }
    if (Player_HasSkill(pl, 0x63) == 1) {
        pl->resist[0] -= 3.0f;
        pl->resist[1] -= 3.0f;
        pl->resist[2] -= 3.0f;
        pl->resist[3] -= 3.0f;
        pl->resist[4] -= 3.0f;
    } else if (Player_HasSkill(pl, 0x64) == 1) {
        pl->resist[0] -= 5.0f;
        pl->resist[1] -= 5.0f;
        pl->resist[2] -= 5.0f;
        pl->resist[3] -= 5.0f;
        pl->resist[4] -= 5.0f;
    } else if (Player_HasSkill(pl, 0x65) == 1) {
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

/* The resistances of the local player: the equipment's, the skills, and
 * the meal effects (+10 to one element and -5 to the others, or 15/-8,
 * 20/-10). */
/* Matches with its three callees static (Player_ClampResists alone: 15 words off). */
void Player_CalcResists(Player *pl)
{
    s16 i;
    s16 kind;
    s16 *kindPtr;

    pl->resist[0] = Player_GetEquipStat(pl, 2);
    pl->resist[1] = Player_GetEquipStat(pl, 3);
    pl->resist[2] = Player_GetEquipStat(pl, 4);
    pl->resist[3] = Player_GetEquipStat(pl, 5);
    pl->resist[4] = Player_GetEquipStat(pl, 13);
    Player_ApplyResistSkills(pl);
    for (i = 0, kindPtr = gResistMealEffects; i < 5; i++, kindPtr++) {
        kind = *kindPtr;
        if (Player_HasMealEffect(pl, kind) == 1) {
            switch (pl->mealEffect[kind]) {
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

/* Player_CalcResists for another player, from the equipment data received. */
void Player_CalcResistsRemote(Player *pl)
{
    EquipView_FromPlayer(pl);
    pl->resist[0] = EquipView_GetStat(2);
    pl->resist[1] = EquipView_GetStat(3);
    pl->resist[2] = EquipView_GetStat(4);
    pl->resist[3] = EquipView_GetStat(5);
    pl->resist[4] = EquipView_GetStat(13);
    Player_ApplyResistSkills(pl);
    Player_ClampResists(pl);
}

/* The sum of the modifiers of `kind` in the current equipment record. */
s8 Equip_SumModifiers(u16 item, u8 group)
{
    s8 sum = 0;
    StatMods *mods = (StatMods *)Item_GetDecoration(item);

    if (mods != NULL) {
        if (mods->kind0 == group) {
            sum += mods->value0;
        }
        if (mods->kind1 == group) {
            sum += mods->value1;
        }
        if (mods->kind2 == group) {
            sum += mods->value2;
        }
        if (mods->kind3 == group) {
            sum += mods->value3;
        }
    }
    return sum;
}

extern ArmorData *gArmorTables[];

/* The points of skill `kind` an equipment item gives: an armour piece's own
 * five {skill, points}, then its decorations (weapons: the decorations only). */
s8 Equip_GetSkillPoints(EquipItem *piece, u8 group)
{
    s8 sum = 0;
    ArmorData *armor;
    s16 i;

    if (piece->id == 0) {
        return 0;
    }
    switch (piece->type) {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
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
    case 6:
    case 7:
        for (i = 0; i < 3; i++) {
            if (piece->deco[i] != 0) {
                sum += Equip_SumModifiers(piece->deco[i], group);
            }
        }
        break;
    }
    return sum;
}

/* 1 for a piece with points of kind 0x29, +2 with kind 0x2A (0 for no
 * piece). */
s8 Equip_TorsoUpBonus(EquipItem *piece)
{
    s8 bonus = 0;

    if (piece->id == 0) {
        return 0;
    }
    if (Equip_GetSkillPoints(piece, 0x29) > 0) {
        bonus++;
    }
    if (Equip_GetSkillPoints(piece, 0x2A) > 0) {
        bonus += 2;
    }
    return bonus;
}

/* The torso's points of a skill kind, multiplied by 1 + the pieces with
 * kind 0x2B (torso up), plus Equip_TorsoUpBonus of each piece. */
s8 Player_GetTorsoSkillPoints(Player *pl, u8 group)
{
    s8 points = Equip_GetSkillPoints(&pl->equip[3], group);
    s8 factor;

    if (points != 0) {
        factor = 1;
        if (Equip_GetSkillPoints(&pl->equip[2], 0x2B)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[3], 0x2B)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[4], 0x2B)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[5], 0x2B)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[0], 0x2B)) {
            factor++;
        }
        if (Equip_GetSkillPoints(&pl->equip[EQUIP_SLOT_WEAPON], 0x2B)) {
            factor++;
        }
        points *= factor;
        points += Equip_TorsoUpBonus(&pl->equip[2]);
        points += Equip_TorsoUpBonus(&pl->equip[3]);
        points += Equip_TorsoUpBonus(&pl->equip[4]);
        points += Equip_TorsoUpBonus(&pl->equip[5]);
        points += Equip_TorsoUpBonus(&pl->equip[0]);
        points += Equip_TorsoUpBonus(&pl->equip[EQUIP_SLOT_WEAPON]);
    }
    return points;
}

/* The player's total points of a skill kind over the equipment. */
s8 Player_GetSkillPoints(Player *pl, u8 group)
{
    s8 points = 0;

    points += Equip_GetSkillPoints(&pl->equip[2], group);
    points += Equip_GetSkillPoints(&pl->equip[4], group);
    points += Equip_GetSkillPoints(&pl->equip[5], group);
    points += Equip_GetSkillPoints(&pl->equip[0], group);
    points += Equip_GetSkillPoints(&pl->equip[EQUIP_SLOT_WEAPON], group);
    points += Player_GetTorsoSkillPoints(pl, group);
    return points;
}

/* Adds a skill to the next free slot (at most 5). */
void Player_AddSkill(Player *pl, u8 skill, u8 *count, u16 group, s8 points)
{
    if (*count < 5) {
        pl->skills[*count] = skill;
        pl->skillGroups[*count] = group;
        pl->skillPoints[*count] = points;
        (*count)++;
    }
}

/* A skill kind's thresholds (D_0044A9A0, ended by kind 0xFFFF). */
typedef struct SkillDef {
    u16 kind;                   /* 0x0 */
    s8 neg[3][2];               /* 0x2: at most [i][0] points gives the skill [i][1] */
    s8 pos[3][2];               /* 0x8: at least [i][0] points gives the skill [i][1] */
    u8 pad[2];
} SkillDef;

extern SkillDef D_0044A9A0[];

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

    def = D_0044A9A0;
    count = 0;
    for (i = 0; i < 5; i++) {
        pl->skills[i] = 0;
        pl->skillGroups[i] = 0;
        pl->skillPoints[i] = 0;
    }
    while (def->kind != 0xFFFF) {
        points = Player_GetSkillPoints(pl, def->kind);
        if (points <= def->neg[0][0]) {
            Player_AddSkill(pl, def->neg[0][1], &count, def->kind, points);
        } else if (points <= def->neg[1][0]) {
            Player_AddSkill(pl, def->neg[1][1], &count, def->kind, points);
        } else if (points <= def->neg[2][0]) {
            Player_AddSkill(pl, def->neg[2][1], &count, def->kind, points);
        } else if (points >= def->pos[2][0]) {
            Player_AddSkill(pl, def->pos[2][1], &count, def->kind, points);
        } else if (points >= def->pos[1][0]) {
            Player_AddSkill(pl, def->pos[1][1], &count, def->kind, points);
        } else if (points >= def->pos[0][0]) {
            Player_AddSkill(pl, def->pos[0][1], &count, def->kind, points);
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

/* Nonzero when the player has the skill: in one of the 5 slots, or, offline,
 * the local player's meal skill (+0x8C9). None while Town_InMode46 says so
 * online. */
s32 Player_HasSkill(Player *pl, u8 skill)
{
    Player *local = &gPlayers[gScene.localPlayer];
    s16 i;

    if (gScene.online != 0 && Town_InMode46() == 1) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        if (pl->skills[i] == skill) {
            return 1;
        }
    }
    if (gScene.online == 0 && local->mealSkill == skill) {
        return 1;
    }
    return 0;
}

/* The player's skill of a skill group (0 if none). */
u8 Player_GetSkill(Player *pl, u8 group)
{
    Player *local = &gPlayers[gScene.localPlayer];
    s16 i;

    if (gScene.online != 0 && Town_InMode46() == 1) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        if (pl->skillGroups[i] == group) {
            return pl->skills[i];
        }
    }
    if (gScene.online == 0 && local->mealSkillGroup == group) {
        return local->mealSkill;
    }
    return 0;
}

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
extern HitShape *D_7F1950[];

/* Pushes player a (its capsule, Player_BodyCapsule with radius 40) and monster b (the hit shapes
 * of its bones, gEmBoneHits, online D_7F1950, 0x28 bytes each, bone -1 ends) apart: up to two
 * pushes summed at the longest length (no height unless a->action is 2, nor upwards while a
 * rises in states 2/3); the lower +0x596 moves (both halfway when equal), +0x738 set on it.
 * Then a stays between its start height +0x574 and 480 above it (when it started below). */
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
    if (gScene.online == 0) {
        shape = gEmBoneHits[em->weaponType];
    } else {
        shape = D_7F1950[em->weaponType];
    }
    while ((boneNo = shape->bone) != -1) {
        if ((shape->atkFlags & 4) || (shape->atkFlags2 & 2) || (shape->openFlags & 8) || (shape->openFlags & 2)
            || (shape->offFlags & 4) || (shape->offFlags & 1)) {
            shape++;
            continue;
        }
        switch (boneNo) {
        case 0x7F:
        case 0x7E:
            shapeKind = HitShape_Place(em->pos, shape, &shapeHit, &shapeSphere, 1.0f);
            break;
        case 0x7D:
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
            if (pl->action != 2) {
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
            ((u8 *)em)[0x738] = 1;
        } else if (em->pushPriority == pl->pushPriority) {
            pl->pos[0] += 0.5f * push[0][0];
            pl->pos[1] += 0.5f * push[0][1];
            pl->pos[2] += 0.5f * push[0][2];
            /* a height check compiled out (an empty compound if: the schedule needs it) */
            if (pl->pos[1] < pl->floorY || pl->pos[1] > pl->floorY + 480.0f) { /* compiled-out check */
            }
            em->pos[0] += -0.5f * push[0][0];
            em->pos[1] += -0.5f * push[0][1];
            em->pos[2] += -0.5f * push[0][2];
            ((u8 *)pl)[0x738] = 1;
            ((u8 *)em)[0x738] = 1;
        } else {
            pl->pos[0] += push[0][0];
            pl->pos[1] += push[0][1];
            pl->pos[2] += push[0][2];
            ((u8 *)pl)[0x738] = 1;
        }
        if (pl->pos[1] < pl->floorY) {
            pl->pos[1] = pl->floorY;
        } else if (startY < pl->floorY + 480.0f && pl->pos[1] > pl->floorY + 480.0f) {
            pl->pos[1] = pl->floorY + 480.0f;
        }
    }
}

s32 Sphere_PushOut(f32 ra, f32 rb, f32 *a, f32 *b, f32 *out);
extern f32 *D_8BA130[];
extern f32 *D_7F1980[];

/* Pushes monsters a and b apart when their hit spheres (the tables D_8BA130, online D_7F1980,
 * per kind: x, y, z, radius, ending with radius -1, scaled by a->scaleX) touch: sums up to two
 * pushes, keeps the longest length, and moves both halfway when their +0x596 are equal, else
 * the one with the lower +0x596; the height only when +0x390 is 2. Sets +0x738 on the moved ones. */
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
    if (gScene.online == 0) {
        spheresA = D_8BA130[emA->weaponType];
        spheresB = D_8BA130[emB->weaponType];
    } else {
        spheresA = D_7F1980[emA->weaponType];
        spheresB = D_7F1980[emB->weaponType];
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
                goto done;
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
        if (emA->action != 2) {
            push[0][1] = 0.0f;
        }
        Vec_Normalize(push[0]);
        push[0][0] *= pushLen[0];
        push[0][1] *= pushLen[0];
        push[0][2] *= pushLen[0];
        if (emA->pushPriority == emB->pushPriority) {
            emA->pos[0] += 0.5f * push[0][0];
            if (emA->action == 2) {
                emA->pos[1] += 0.5f * push[0][1];
            }
            emA->pos[2] += 0.5f * push[0][2];
            emB->pos[0] += -0.5f * push[0][0];
            if (emB->action == 2) {
                emB->pos[1] += -0.5f * push[0][1];
            }
            emB->pos[2] += -0.5f * push[0][2];
            ((u8 *)emA)[0x738] = 1;
            ((u8 *)emB)[0x738] = 1;
        } else if (emA->pushPriority < emB->pushPriority) {
            emA->pos[0] += push[0][0];
            if (emA->action == 2) {
                emA->pos[1] += push[0][1];
            }
            emA->pos[2] += push[0][2];
            ((u8 *)emA)[0x738] = 1;
        } else {
            emB->pos[0] -= push[0][0];
            if (emB->action == 2) {
                emB->pos[1] -= push[0][1];
            }
            emB->pos[2] -= push[0][2];
            ((u8 *)emB)[0x738] = 1;
        }
    }
}

/* Pushes two players apart when their body capsules (Player_BodyCapsule, radius 40) meet:
 * half the push each, the height too for those in the air (action 2), and marks both. */
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
        if (plA->action == 2 && plB->action == 2) {
            plA->pos[1] += 0.5f * push[1];
            plB->pos[1] += -0.5f * push[1];
        } else if (plA->action == 2) {
            plA->pos[1] += push[1];
        } else if (plB->action == 2) {
            plB->pos[1] -= push[1];
        }
        plA->unk72C[0x738 - 0x72C] = 1;
        plB->unk72C[0x738 - 0x72C] = 1;
    }
}

/* Clears the +0x738 flags of the players and monsters, then pairs every local player in the
 * current area with the other players (when Debug_IsOn(0xA9)) and the monsters in its area
 * (keeping the distance at +0x3B0), and the monsters in the current area with each other. */
s32 Obj_InCurrentArea(void *obj);
s32 Obj_SameArea(void *obj, Player *b);
s32 Debug_IsOn(s32 unused0);
f32 Vec_Length(f32 *v);
void PlayerEm_PushApart(Player *a, Player *b);
void EmEm_PushApart(Player *a, Player *b);
void Players_PushApart(Player *a, Player *b);

/* Pushes overlapping bodies apart: each local player against the other players and the
 * monsters (keeping its distance to the last monster in unk3B0), then each pair of monsters.
 * The pointers to v[1] and v[2] stay in saved registers (set with the first loop). */
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

    for (i = 0, other = gPlayers; i < 4; i++, other++) {
        ((u8 *)other)[0x738] = 0;
    }
    for (em = gEms, j = 0; j < 20; j++, em++) {
        em->pushed = 0;
    }
    for (i = 0, pl = gPlayers, diffY = &diff[1], diffZ = &diff[2]; i < 4; i++, pl++) {
        if (pl->active != 0 && pl->visible != 0 && Player_IsLocal(pl) != 0 && pl->noPushTimer == 0
            && (u8)Obj_InCurrentArea(pl)) {
            if (Debug_IsOn(0xA9) != 0) {
                for (j = 0, other = gPlayers; j < 4; j++, other++) {
                    if (other->active != 0 && other->visible != 0 && other->noPushTimer == 0 && (u8)Obj_SameArea(pl, other)) {
                        Players_PushApart(pl, other);
                    }
                }
            }
            for (other = (Player *)gEms, j = 0; j < 20; j++, other = (Player *)((u8 *)other + sizeof(Em))) {
                if (other->active != 0 && other->visible != 0 && other->noPushTimer == 0 && (u8)Obj_SameArea(pl, other)) {
                    PlayerEm_PushApart(pl, other);
                    diff[0] = pl->pos[0] - other->pos[0];
                    *diffY = pl->pos[1] - other->pos[1];
                    *diffZ = pl->pos[2] - other->pos[2];
                    dist = Vec_Length(diff);
                    *(f32 *)pl->unk3B0 = dist;
                }
            }
        }
    }
    for (i = 0, emA = (Player *)gEms; i < 19; i++, emA = (Player *)((u8 *)emA + sizeof(Em))) {
        if (emA->active != 0 && emA->visible != 0 && emA->noPushTimer == 0 && (u8)Obj_InCurrentArea(emA)) {
            wallFlag = *(u16 *)((u8 *)emA + 0x736) != 0;
            for (j = i + 1, other = (Player *)&gEms[i + 1]; j < 20; j++, other = (Player *)((u8 *)other + sizeof(Em))) {
                if (other->active != 0 && other->visible != 0) {
                    if (emA->weaponType == 0x1D && other->weaponType == 0x1D) {
                        continue;
                    }
                    if ((emA->weaponType == 0x1D || other->weaponType == 0x1D || (emA->startItemList == 0 && other->startItemList == 0))
                        && other->noPushTimer == 0 && (u8)Obj_SameArea(emA, other)) {
                        if ((wallFlag == 0 && *(u16 *)((u8 *)other + 0x736) == 0) || emA->weaponType == 0x1D || other->weaponType == 0x1D) {
                            EmEm_PushApart(emA, other);
                        }
                    }
                }
            }
        }
    }
}

/* Sets motion layer `layer`: the motion, half of `a` and b. */
static void Player_SetLayerMotion(Player *pl, s32 motion, s32 blend, s16 frame, s32 layer)
{
    pl->motionId[layer] = motion;
    pl->motionBlend[layer] = blend / 2;
    pl->motionFrame[layer] = frame;
    pl->motionStarted[layer] = 0;
}

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

void func_003B0510(Player *pl, u16 buttons)
{
}

void func_003B0520(Player *pl)
{
    func_003B0510(pl, ~pl->input.held);
}

/* Starts a jump of `frames` frames: the vertical acceleration that stops
 * the rise halfway. */
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

/* An object that moves like the player, with pos +0x30, vel +0x48 and
 * accel +0x54 (MoveObj_AddVelocity, MoveObj_Integrate); +0x6C/+0x6E/+0x90 are set by
 * MoveObj_StartMotionOwned/MoveObj_StartMotionEntry. */
/* A motion table entry (0x18 bytes, copied whole into MoveObj.motion). */
typedef struct MotionDef {
    u8 unk0;
    u8 unk1;
    u8 pad2[2];
    s32 unk4;
    s32 unk8;
    u16 unkC;
    u8 unkE;
    u8 padF;
    s32 unk10;
    u8 pad14;
    u8 unk15;
    u8 unk16;
    u8 pad17;
} MotionDef;

typedef struct MoveObj {
    u8 pad0[0x8];
    u8 unk8;
    u8 pad9[2];
    u8 unkB;
    u8 padC[0x1E - 0xC];
    u8 unk1E;
    u8 pad1F[0x30 - 0x1F];
    f32 pos[3];                 /* 0x30 */
    u8 pad3C[0x48 - 0x3C];
    f32 vel[3];                 /* 0x48 */
    f32 accel[3];               /* 0x54 */
    MotionDef motion;           /* 0x60: the current motion entry */
    u8 pad78[0x90 - 0x78];
    MotionDef **unk90;          /* 0x90: the motion table */
    u8 pad94[0x9C - 0x94];
    s32 unk9C;
    s32 unkA0;
    u8 padA4[0xB5 - 0xA4];
    u8 unkB5;
} MoveObj;

u8 HitId_Next(void);

typedef struct Words2 {
    s32 a;
    s32 b;
} Words2;

void Obj_StartMotionOwned(MoveObj *o, void *owner, s32 i);
void Obj_StartMotionEntry(MoveObj *o, s32 i);

void Player_Move(void *obj)
{
    ObjHead *head = obj;
    head->pos[0] += head->vel[0];
    head->pos[1] += head->vel[1];
    head->pos[2] += head->vel[2];
}

void Player_MoveAccel(Player *pl)
{
    pl->vel[0] += pl->accel[0];
    pl->vel[1] += pl->accel[1];
    pl->vel[2] += pl->accel[2];
    pl->pos[0] += pl->vel[0];
    pl->pos[1] += pl->vel[1];
    pl->pos[2] += pl->vel[2];
}

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

/* Moves, and stops on the ground (AreaGrid2_GetFloorY: its height here). */
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

void Player_ClearAccel(Player *pl)
{
    pl->accel[2] = 0.0f;
    pl->accel[1] = 0.0f;
    pl->accel[0] = 0.0f;
}

void Player_ClearAccel2(void *obj)
{
    ObjHead *head = obj;
    head->accel[2] = 0.0f;
    head->accel[1] = 0.0f;
    head->accel[0] = 0.0f;
}

void MoveObj_AddVelocity(MoveObj *obj)
{
    obj->pos[0] += obj->vel[0];
    obj->pos[1] += obj->vel[1];
    obj->pos[2] += obj->vel[2];
}

void MoveObj_Integrate(MoveObj *obj)
{
    obj->vel[0] += obj->accel[0];
    obj->vel[1] += obj->accel[1];
    obj->vel[2] += obj->accel[2];
    obj->pos[0] += obj->vel[0];
    obj->pos[1] += obj->vel[1];
    obj->pos[2] += obj->vel[2];
}

/* The motion's remaining frames (+0x3A4) from layer 0; with `step`, the
 * frame counter +0x3D4 advances too. */
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

void func_003B09B0(Player *unused0, u8 unused1)
{
}

/* As Obj_StartMotionEntry for motion entry i with the owner's +0xC as +8, keeping the bits
 * of +0x6C other than 8 and the byte +0x6E. */
void Obj_StartMotionOwned(MoveObj *obj, void *owner, s32 row)
{
    ObjHead *ownerHead = owner;
    s32 *src;
    s32 *dst;
    s32 k;
    u8 keepFlags;
    u8 keepPower;

    keepFlags = obj->motion.unkC & 0xF7;
    keepPower = keepFlags == 0 ? 0 : obj->motion.unkE;
    src = (s32 *)*obj->unk90;
    src += row * 6;
    dst = (s32 *)&obj->motion;
    for (k = 0; k < 6; k++) {
        *dst++ = *src++;
    }
    obj->unk8 = ownerHead->index;
    obj->unkB = 1;
    obj->unk1E = 0;
    obj->unk9C = 0;
    obj->unkA0 = 0;
    if (keepPower != 0) {
        obj->motion.unkC |= keepFlags;
        obj->motion.unkE = keepPower;
    }
    obj->motion.unk0 >>= 1;
    obj->motion.unk1 >>= 1;
    if (obj->motion.unk15 != 0xFF) {
        obj->motion.unk15 >>= 1;
    }
    obj->motion.unk16 >>= 1;
    obj->unkB5 = HitId_Next();
}

void MoveObj_StartMotionOwned(MoveObj *obj, s32 owner, s32 row, void *rows)
{
    obj->unk90 = (MotionDef **)rows;
    obj->motion.unkE = 0;
    obj->motion.unkC = 0;
    Obj_StartMotionOwned(obj, (void *) owner, row);
}

/* Starts motion entry i of the object table: the entry copied word by word, the first
 * values halved, a new hit id. */
void Obj_StartMotionEntry(MoveObj *obj, s32 row)
{
    s32 *src;
    s32 *dst;
    s32 k;

    src = (s32 *)*obj->unk90;
    src += row * 6;
    dst = (s32 *)&obj->motion;

    for (k = 0; k < 6; k++) {
        *dst++ = *src++;
    }
    obj->unk8 = 0xFF;
    obj->unkB = 1;
    obj->unk1E = 0;
    obj->unk9C = 0;
    obj->unkA0 = 0;
    obj->motion.unk0 >>= 1;
    obj->motion.unk1 >>= 1;
    if (obj->motion.unk15 != 0xFF) {
        obj->motion.unk15 >>= 1;
    }
    obj->motion.unk16 >>= 1;
    obj->unkB5 = HitId_Next();
}

void MoveObj_StartMotionEntry(MoveObj *obj, s32 row, void *rows)
{
    obj->unk90 = (MotionDef **)rows;
    Obj_StartMotionEntry(obj, row);
}


/* Places the hit shape def in the world by its owner's bone: a sphere into s (0), a
 * capsule into c (1), -1 for none. */
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

/* A sphere (type 0: x, y, z, r) or a capsule (type 1: two points and r)
 * from a hit record placed at pos, the radius scaled by s. */
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

/* The floor fwd ahead of the player (along its heading): 1 with its height in *out when
 * it lies from up to up + range above the player's height, 0 when not, 1 with none found. */
s32 Player_FloorAheadAbove(Player *pl, f32 *out, f32 fwd, f32 up, f32 range)
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
    minY = *pointY + up;
    if (Em_GetFloorYAbove((struct Em *)pl, point, &floorY) != 1) {
        return 1;
    }
    if (!(floorY < minY) && floorY <= minY + range) {
        *out = floorY;
        return 1;
    }
    return 0;
}

/* The floor fwd ahead of the player (along its heading) against the player's height plus
 * up: 1 when it is below that (above it with `above`), 0 when not or with no floor. */
s32 Player_FloorAheadBelow(Player *pl, u16 above, f32 fwd, f32 up)
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
    refY = pl->pos[1] + up;
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

/* Nonzero when the player is in the current area (SceneState.area). */
s32 Obj_InCurrentArea(void *obj)
{
    ObjHead *head = obj;
    return head->area == gScene.area;
}

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

void Player_UpdateAreaPos(void *obj)
{
    ObjHead *head = obj;
    AreaRecord *rec = Stage_GetAreaRecord(head->area);

    head->mapPos[0] = head->pos[0] + rec->originX;
    head->mapPos[1] = head->pos[1] + (rec->bottom + rec->top) / 2.0f;
    head->mapPos[2] = head->pos[2] + rec->originZ;
}

void Area_ToWorld(s32 area, f32 *pos, f32 *out)
{
    AreaRecord *rec = Stage_GetAreaRecord(area);

    out[0] = pos[0] + rec->originX;
    out[1] = pos[1] + (rec->bottom + rec->top) / 2.0f;
    out[2] = pos[2] + rec->originZ;
}

/* The gathering point the player stands at (as Gather_AtPoint): its item table, with
 * its +0x14 in *id and its position in pos; 0xFFFF for none. */
u16 Player_GetGatherPoint(Player *pl, u16 *kind, f32 *pos)
{
    GatherPt *point;
    f32 dx;
    f32 dz;
    u16 table;

    point = Gather_GetPoints(pl->area);
    if (point == NULL) {
        return 0xFFFF;
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
        *kind = point->unk14;
        pos[0] = point->x;
        pos[1] = point->y;
        pos[2] = point->z;
        return table;
    }
    return 0xFFFF;
}
