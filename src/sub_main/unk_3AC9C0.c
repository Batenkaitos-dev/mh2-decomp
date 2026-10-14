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
void Player_UpdateNearMonster(Player *p)
{
    s32 i;
    Em *e;
    f32 dx;
    f32 dz;
    f32 d;
    f32 r;
    f32 lim;

    if (p->rideState != 0 || !(p->pos[1] - p->floorY <= 100.0f)) {
        p->unk597[0] = 0;
        return;
    }
    for (e = gEms, i = 0; i < 20; i++, e++) {
        if (e->active == 0 || e->visible == 0 || e->pushPriority < 3) {
            continue;
        }
        dx = p->pos[0] - e->posX;
        dz = p->pos[2] - e->posZ;
        d = Math_Sqrtf(dx * dx + dz * dz);
        if (e->pushPriority == 3) {
            lim = r = 300.0f;
        } else {
            r = 500.0f;
            lim = 300.0f;
        }
        if (d <= r && p->pos[1] - e->posY < lim) {
            p->unk597[0] = 1;
            return;
        }
    }
    p->unk597[0] = 0;
}

/* A capsule of radius r along the player's body: from the middle of records 5 and 8
 * of +0xD0 to record 20, both ends moved by r across the axis. */
void Player_BodyCapsule(Player *p, HitCapsule *cap, f32 r)
{
    f32 m[16];
    f32 a[3];
    f32 b[3];
    f32 d[3];
    f32 off[3];
    f32 *dy;
    f32 *dz;

    Mtx_GetTrans(a, p->partRecs[5]->m1);
    Mtx_GetTrans(b, p->partRecs[8]->m1);
    cap->a[0] = (a[0] + b[0]) / 2.0f;
    cap->a[1] = (a[1] + b[1]) / 2.0f;
    cap->a[2] = (a[2] + b[2]) / 2.0f;
    Mtx_GetTrans(cap->b, p->partRecs[20]->m1);
    dy = &d[1];
    dz = &d[2];
    d[0] = cap->b[0] - cap->a[0];
    *dy = cap->b[1] - cap->a[1];
    *dz = cap->b[2] - cap->a[2];
    Mtx_FromDirection(d, m, 1);
    d[0] = 0.0f;
    *dy = r;
    *dz = 0.0f;
    Vec_Rotate(off, d, m);
    cap->a[0] += off[0];
    cap->a[1] += off[1];
    cap->a[2] += off[2];
    cap->b[0] -= off[0];
    cap->b[1] -= off[1];
    cap->b[2] -= off[2];
    cap->radius = r;
}

/* Sets the movement mode (+0x390: 0, 1 or 2) and the flags 8, 1 and 2. */
void Player_SetMoveMode(Player *p, u16 mode, s16 flag1, s16 flag2)
{
    s32 m = mode;

    switch ((u8)m) {
    default:
        p->action = 0;
        break;
    case 1:
        p->action = 1;
        break;
    case 2:
        p->action = 2;
        break;
    }
    if (m & 0x8000) {
        Player_ClearFlag(p, 8);
    } else {
        Player_SetFlag(p, 8);
    }
    if (flag1 == 0) {
        Player_ClearFlag(p, 1);
    } else {
        Player_SetFlag(p, 1);
    }
    if (flag2 == 0) {
        Player_ClearFlag(p, 2);
    } else {
        Player_SetFlag(p, 2);
    }
}

/* Nonzero for the player of this console: its number (+0x0C) is the local
 * one (gScene.localPlayer), or everyone counts as local (gScene.allLocal, offline). */
s32 Player_IsLocal(void *obj)
{
    ObjHead *o = obj;
    if (Quest_IsDrill() == 1 && gScene.allLocal != 0) {
        return 1;
    }
    if (o->index == gScene.localPlayer) {
        return 1;
    }
    return 0;
}

s32 func_003ACE10(Player *p)
{
    if (Player_IsState(p, 0, 0x36) != 0 || Player_IsState(p, 0, 0x48) != 0 || Player_IsState(p, 0, 0x7A) != 0) {
        return 1;
    }
    return 0;
}

/* The HUD's status icons (+0x4B8) of the local player. */
void Player_UpdateStatusIcons(Player *p)
{
    u16 flags = 0;
    s16 i;

    if (Player_IsLocal(p) != 0) {
        if (p->poison != 0) {
            flags |= 1;
        }
        if (Player_IsState(p, 2, 0x15) != 0) {
            flags |= 2;
        }
        if (Player_IsState(p, 2, 0x16) != 0 || Player_IsState(p, 2, 0x18) != 0) {
            flags |= 4;
        }
        if (Player_IsState(p, 2, 0x13) != 0) {
            flags |= 8;
        }
        if (Player_InMashStatus(p) == 1) {
            flags |= 0x40;
        }
        if ((s16)(p->attackDrug + p->attackBuff) > 0) {
            flags |= 0x10;
        }
        if (Player_HasMealEffect(p, 1) == 1) {
            flags |= 0x10;
        }
        if ((s16)(p->defenseDrug + p->defenseBuff) > 0) {
            flags |= 0x20;
        }
        if (Player_HasMealEffect(p, 2) == 1) {
            flags |= 0x20;
        }
        if (p->coolDrinkTimer > 0) {
            flags |= 0x80;
        }
        if (p->hotDrinkTimer > 0) {
            flags |= 0x100;
        }
        if (Quest_DrillHasCharm() == 1) {
            flags |= 0x200;
        }
        if (p->status6Timer > 0) {
            flags |= 0x400;
        }
        if (p->status7Timer > 0) {
            flags |= 0x800;
        }
        if (p->regenTimer > 0) {
            flags |= 0x1000;
        }
        if (p->weaponType == 6 && p->demonMode != 0) {
            flags |= 0x8000;
        }
        for (i = 0; i < 12; i++) {
            if (p->mealEffectTimer[i] > 0) {
                flags |= 0x2000;
            }
        }
        if (Quest_IsVersus() == 1 && p->stateGroup == 3) {
            flags |= 0x4000;
        }
        p->statusIcons = flags;
    }
}

/*
 * The attack power (+0x630, 1..800) and its factor (+0x724, /100) of the
 * local player: the equipment's (Player_GetEquipStat(p, 0)), +6 and +9 for items
 * 0x23 and 0x24 in the pouch, the buffs +0x628/+0x629, the skills 0x4F-0x51
 * (+3, +5, +10), a meal effect (+0x8D7) offline, and the skills 0xB3 (x0.7)
 * and 0xB2 (x1.5) at 40% health or less.
 */
void Player_CalcAttack(Player *p)
{
    s32 atk = 0;

    if (Player_IsLocal(p) == 1) {
        atk += Player_GetEquipStat(p, 0);
        if (Pouch_Count(p, 0x23) != 0) {
            atk += 6;
        }
        if (Pouch_Count(p, 0x24) != 0) {
            atk += 9;
        }
        atk += p->attackDrug;
        atk += p->attackBuff;
        if (Player_HasSkill(p, 0x4F) == 1) {
            atk += 3;
        } else if (Player_HasSkill(p, 0x50) == 1) {
            atk += 5;
        } else if (Player_HasSkill(p, 0x51) == 1) {
            atk += 10;
        }
        if (gScene.online == 0 && Player_HasMealEffect(p, 1) == 1) {
            switch (p->mealEffect[1]) {
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
        if (Player_HasSkill(p, 0xB3) == 1) {
            if ((f32)p->hp / (f32)p->hpMax <= 0.4f) {
                atk = 0.7f * atk;
            }
        } else if (Player_HasSkill(p, 0xB2) == 1) {
            if ((f32)p->hp / (f32)p->hpMax <= 0.4f) {
                atk = 1.5f * atk;
            }
        }
        if (atk < 0) {
            atk = 1;
        }
        p->attack = atk;
        if (p->attack >= 800) {
            p->attack = 800;
        }
    }
    p->attackFactor = p->attack / 100.0f;
}

/* Player_CalcAttack for another player, from the equipment data received. */
void Player_CalcAttackRemote(Player *p)
{
    s32 atk = 0;

    EquipView_FromPlayer(p);
    atk += EquipView_GetStat(0);
    if (Pouch_Count(p, 0x23) != 0) {
        atk += 6;
    }
    if (Pouch_Count(p, 0x24) != 0) {
        atk += 9;
    }
    atk += p->attackDrug;
    atk += p->attackBuff;
    if (Player_HasSkill(p, 0x4F) == 1) {
        atk += 3;
    } else if (Player_HasSkill(p, 0x50) == 1) {
        atk += 5;
    } else if (Player_HasSkill(p, 0x51) == 1) {
        atk += 10;
    }
    if (atk < 0) {
        atk = 1;
    }
    p->attack = atk;
    if (p->attack >= 800) {
        p->attack = 800;
    }
    p->attackFactor = p->attack / 100.0f;
}

/*
 * The defense (+0x632) and its factor (+0x728, /2, at least 1) of the local
 * player: 1 + the equipment's, +16 and +24 for items 0x25 and 0x26, the
 * buffs, the skills 0x52-0x57 (+20, +30, +40, -20, -30, -40), a bonus at
 * 40% health or less (skill 0xB3: +42, 0xB1 or 0xB2: +90, else +60), and a
 * meal effect (+0x8D8) offline.
 */
void Player_CalcDefense(Player *p)
{
    s32 def = 1;

    def += Player_GetEquipStat(p, 1);
    if (Pouch_Count(p, 0x25) != 0) {
        def += 16;
    }
    if (Pouch_Count(p, 0x26) != 0) {
        def += 24;
    }
    def += p->defenseDrug;
    def += p->defenseBuff;
    if (Player_HasSkill(p, 0x52) == 1) {
        def += 20;
    } else if (Player_HasSkill(p, 0x53) == 1) {
        def += 30;
    } else if (Player_HasSkill(p, 0x54) == 1) {
        def += 40;
    } else if (Player_HasSkill(p, 0x55) == 1) {
        def -= 20;
    } else if (Player_HasSkill(p, 0x56) == 1) {
        def -= 30;
    } else if (Player_HasSkill(p, 0x57) == 1) {
        def -= 40;
    }
    if (def <= 0) {
        def = 1;
    }
    if ((f32)p->hp / (f32)p->hpMax <= 0.4f) {
        if (Player_HasSkill(p, 0xB3) == 1) {
            def += 42;
        } else if (Player_HasSkill(p, 0xB1) == 1 || Player_HasSkill(p, 0xB2) == 1) {
            def += 90;
        } else {
            def += 60;
        }
    }
    if (gScene.online == 0 && Player_HasMealEffect(p, 2) == 1) {
        switch (p->mealEffect[2]) {
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
    p->defense = def;
    p->defenseFactor = p->defense / 2.0f;
    if (p->defenseFactor < 1.0f) {
        p->defenseFactor = 1.0f;
    }
}

/* Player_CalcDefense for another player, from the equipment data received. */
void Player_CalcDefenseRemote(Player *p)
{
    s32 def = 1;

    EquipView_FromPlayer(p);
    def += EquipView_GetStat(1);
    if (Pouch_Count(p, 0x25) != 0) {
        def += 16;
    }
    if (Pouch_Count(p, 0x26) != 0) {
        def += 24;
    }
    def += p->defenseDrug;
    def += p->defenseBuff;
    if (Player_HasSkill(p, 0x52) == 1) {
        def += 20;
    } else if (Player_HasSkill(p, 0x53) == 1) {
        def += 30;
    } else if (Player_HasSkill(p, 0x54) == 1) {
        def += 40;
    } else if (Player_HasSkill(p, 0x55) == 1) {
        def -= 20;
    } else if (Player_HasSkill(p, 0x56) == 1) {
        def -= 30;
    } else if (Player_HasSkill(p, 0x57) == 1) {
        def -= 40;
    }
    if (def <= 0) {
        def = 1;
    }
    p->defense = def;
    p->defenseFactor = p->defense / 2.0f;
    if (p->defenseFactor < 1.0f) {
        p->defenseFactor = 1.0f;
    }
}

/* Adds v to the five resistances, `bonus` instead for the element `elem`
 * (6 fire, 7 water, 8 thunder, 9 dragon, 10 ice). */
static void Player_AddResists(f32 bonus, Player *p, s16 elem, f32 v)
{
    if (elem == 6) {
        p->resist[0] += bonus;
    } else {
        p->resist[0] += v;
    }
    if (elem == 7) {
        p->resist[1] += bonus;
    } else {
        p->resist[1] += v;
    }
    if (elem == 8) {
        p->resist[2] += bonus;
    } else {
        p->resist[2] += v;
    }
    if (elem == 9) {
        p->resist[4] += bonus;
    } else {
        p->resist[4] += v;
    }
    if (elem == 10) {
        p->resist[3] += bonus;
    } else {
        p->resist[3] += v;
    }
}

/* The resistance skills: +10/+5/+3 or -10/-5/-3 to one element, or to all
 * (0x60-0x65). */
static void Player_ApplyResistSkills(Player *p)
{
    if (Player_HasSkill(p, 0x68) == 1) {
        p->resist[0] += 10.0f;
    } else if (Player_HasSkill(p, 0x67) == 1) {
        p->resist[0] += 5.0f;
    } else if (Player_HasSkill(p, 0x66) == 1) {
        p->resist[0] += 3.0f;
    }
    if (Player_HasSkill(p, 0x6B) == 1) {
        p->resist[0] -= 10.0f;
    } else if (Player_HasSkill(p, 0x6A) == 1) {
        p->resist[0] -= 5.0f;
    } else if (Player_HasSkill(p, 0x69) == 1) {
        p->resist[0] -= 3.0f;
    }
    if (Player_HasSkill(p, 0x6E) == 1) {
        p->resist[1] += 10.0f;
    } else if (Player_HasSkill(p, 0x6D) == 1) {
        p->resist[1] += 5.0f;
    } else if (Player_HasSkill(p, 0x6C) == 1) {
        p->resist[1] += 3.0f;
    }
    if (Player_HasSkill(p, 0x71) == 1) {
        p->resist[1] -= 10.0f;
    } else if (Player_HasSkill(p, 0x70) == 1) {
        p->resist[1] -= 5.0f;
    } else if (Player_HasSkill(p, 0x6F) == 1) {
        p->resist[1] -= 3.0f;
    }
    if (Player_HasSkill(p, 0x7A) == 1) {
        p->resist[2] += 10.0f;
    } else if (Player_HasSkill(p, 0x79) == 1) {
        p->resist[2] += 5.0f;
    } else if (Player_HasSkill(p, 0x78) == 1) {
        p->resist[2] += 3.0f;
    }
    if (Player_HasSkill(p, 0x7D) == 1) {
        p->resist[2] -= 10.0f;
    } else if (Player_HasSkill(p, 0x7C) == 1) {
        p->resist[2] -= 5.0f;
    } else if (Player_HasSkill(p, 0x7B) == 1) {
        p->resist[2] -= 3.0f;
    }
    if (Player_HasSkill(p, 0x80) == 1) {
        p->resist[3] += 10.0f;
    } else if (Player_HasSkill(p, 0x7F) == 1) {
        p->resist[3] += 5.0f;
    } else if (Player_HasSkill(p, 0x7E) == 1) {
        p->resist[3] += 3.0f;
    }
    if (Player_HasSkill(p, 0x83) == 1) {
        p->resist[3] -= 10.0f;
    } else if (Player_HasSkill(p, 0x82) == 1) {
        p->resist[3] -= 5.0f;
    } else if (Player_HasSkill(p, 0x81) == 1) {
        p->resist[3] -= 3.0f;
    }
    if (Player_HasSkill(p, 0x74) == 1) {
        p->resist[4] += 10.0f;
    } else if (Player_HasSkill(p, 0x73) == 1) {
        p->resist[4] += 5.0f;
    } else if (Player_HasSkill(p, 0x72) == 1) {
        p->resist[4] += 3.0f;
    }
    if (Player_HasSkill(p, 0x77) == 1) {
        p->resist[4] -= 10.0f;
    } else if (Player_HasSkill(p, 0x76) == 1) {
        p->resist[4] -= 5.0f;
    } else if (Player_HasSkill(p, 0x75) == 1) {
        p->resist[4] -= 3.0f;
    }
    if (Player_HasSkill(p, 0x60) == 1) {
        p->resist[0] += 3.0f;
        p->resist[1] += 3.0f;
        p->resist[2] += 3.0f;
        p->resist[3] += 3.0f;
        p->resist[4] += 3.0f;
    } else if (Player_HasSkill(p, 0x61) == 1) {
        p->resist[0] += 5.0f;
        p->resist[1] += 5.0f;
        p->resist[2] += 5.0f;
        p->resist[3] += 5.0f;
        p->resist[4] += 5.0f;
    } else if (Player_HasSkill(p, 0x62) == 1) {
        p->resist[0] += 10.0f;
        p->resist[1] += 10.0f;
        p->resist[2] += 10.0f;
        p->resist[3] += 10.0f;
        p->resist[4] += 10.0f;
    }
    if (Player_HasSkill(p, 0x63) == 1) {
        p->resist[0] -= 3.0f;
        p->resist[1] -= 3.0f;
        p->resist[2] -= 3.0f;
        p->resist[3] -= 3.0f;
        p->resist[4] -= 3.0f;
    } else if (Player_HasSkill(p, 0x64) == 1) {
        p->resist[0] -= 5.0f;
        p->resist[1] -= 5.0f;
        p->resist[2] -= 5.0f;
        p->resist[3] -= 5.0f;
        p->resist[4] -= 5.0f;
    } else if (Player_HasSkill(p, 0x65) == 1) {
        p->resist[0] -= 10.0f;
        p->resist[1] -= 10.0f;
        p->resist[2] -= 10.0f;
        p->resist[3] -= 10.0f;
        p->resist[4] -= 10.0f;
    }
}

/* Clamps the resistances to [-100, 100]. */
static void Player_ClampResists(Player *p)
{
    if (p->resist[0] < -100.0f) {
        p->resist[0] = -100.0f;
    }
    if (p->resist[1] < -100.0f) {
        p->resist[1] = -100.0f;
    }
    if (p->resist[2] < -100.0f) {
        p->resist[2] = -100.0f;
    }
    if (p->resist[3] < -100.0f) {
        p->resist[3] = -100.0f;
    }
    if (p->resist[4] < -100.0f) {
        p->resist[4] = -100.0f;
    }
    if (p->resist[0] > 100.0f) {
        p->resist[0] = 100.0f;
    }
    if (p->resist[1] > 100.0f) {
        p->resist[1] = 100.0f;
    }
    if (p->resist[2] > 100.0f) {
        p->resist[2] = 100.0f;
    }
    if (p->resist[3] > 100.0f) {
        p->resist[3] = 100.0f;
    }
    if (p->resist[4] > 100.0f) {
        p->resist[4] = 100.0f;
    }
}

/* The resistances of the local player: the equipment's, the skills, and
 * the meal effects (+10 to one element and -5 to the others, or 15/-8,
 * 20/-10). */
/* Matches with its three callees static (Player_ClampResists alone: 15 words off). */
void Player_CalcResists(Player *p)
{
    s16 i;
    s16 e;
    s16 *elem;

    p->resist[0] = Player_GetEquipStat(p, 2);
    p->resist[1] = Player_GetEquipStat(p, 3);
    p->resist[2] = Player_GetEquipStat(p, 4);
    p->resist[3] = Player_GetEquipStat(p, 5);
    p->resist[4] = Player_GetEquipStat(p, 13);
    Player_ApplyResistSkills(p);
    for (i = 0, elem = gResistMealEffects; i < 5; i++, elem++) {
        e = *elem;
        if (Player_HasMealEffect(p, e) == 1) {
            switch (p->mealEffect[e]) {
            case 1:
                Player_AddResists(10.0f, p, e, -5.0f);
                break;
            case 2:
            case 3:
                Player_AddResists(15.0f, p, e, -8.0f);
                break;
            case 4:
                Player_AddResists(20.0f, p, e, -10.0f);
                break;
            }
        }
    }
    Player_ClampResists(p);
}

/* Player_CalcResists for another player, from the equipment data received. */
void Player_CalcResistsRemote(Player *p)
{
    EquipView_FromPlayer(p);
    p->resist[0] = EquipView_GetStat(2);
    p->resist[1] = EquipView_GetStat(3);
    p->resist[2] = EquipView_GetStat(4);
    p->resist[3] = EquipView_GetStat(5);
    p->resist[4] = EquipView_GetStat(13);
    Player_ApplyResistSkills(p);
    Player_ClampResists(p);
}

/* The sum of the modifiers of `kind` in the current equipment record. */
s8 Equip_SumModifiers(u16 item, u8 kind)
{
    s8 sum = 0;
    StatMods *m = (StatMods *)Item_GetDecoration(item);

    if (m != NULL) {
        if (m->kind0 == kind) {
            sum += m->value0;
        }
        if (m->kind1 == kind) {
            sum += m->value1;
        }
        if (m->kind2 == kind) {
            sum += m->value2;
        }
        if (m->kind3 == kind) {
            sum += m->value3;
        }
    }
    return sum;
}

extern ArmorData *gArmorTables[];

/* The points of skill `kind` an equipment item gives: an armour piece's own
 * five {skill, points}, then its decorations (weapons: the decorations only). */
s8 Equip_GetSkillPoints(EquipItem *e, u8 kind)
{
    s8 sum = 0;
    ArmorData *a;
    s16 i;

    if (e->id == 0) {
        return 0;
    }
    switch (e->type) {
    case 0:
    case 2:
    case 3:
    case 4:
    case 5:
        a = gArmorTables[e->type];
        a += e->id;
        if (a->skills[0].skill == kind) {
            sum += a->skills[0].points;
        }
        if (a->skills[1].skill == kind) {
            sum += a->skills[1].points;
        }
        if (a->skills[2].skill == kind) {
            sum += a->skills[2].points;
        }
        if (a->skills[3].skill == kind) {
            sum += a->skills[3].points;
        }
        if (a->skills[4].skill == kind) {
            sum += a->skills[4].points;
        }
        for (i = 0; i < 3; i++) {
            if (e->deco[i] != 0) {
                sum += Equip_SumModifiers(e->deco[i], kind);
            }
        }
        break;
    case 6:
    case 7:
        for (i = 0; i < 3; i++) {
            if (e->deco[i] != 0) {
                sum += Equip_SumModifiers(e->deco[i], kind);
            }
        }
        break;
    }
    return sum;
}

/* 1 for a piece with points of kind 0x29, +2 with kind 0x2A (0 for no
 * piece). */
s8 Equip_TorsoUpBonus(EquipItem *e)
{
    s8 n = 0;

    if (e->id == 0) {
        return 0;
    }
    if (Equip_GetSkillPoints(e, 0x29) > 0) {
        n++;
    }
    if (Equip_GetSkillPoints(e, 0x2A) > 0) {
        n += 2;
    }
    return n;
}

/* The torso's points of a skill kind, multiplied by 1 + the pieces with
 * kind 0x2B (torso up), plus Equip_TorsoUpBonus of each piece. */
s8 Player_GetTorsoSkillPoints(Player *p, u8 kind)
{
    s8 pts = Equip_GetSkillPoints(&p->equip[3], kind);
    s8 mul;

    if (pts != 0) {
        mul = 1;
        if (Equip_GetSkillPoints(&p->equip[2], 0x2B)) {
            mul++;
        }
        if (Equip_GetSkillPoints(&p->equip[3], 0x2B)) {
            mul++;
        }
        if (Equip_GetSkillPoints(&p->equip[4], 0x2B)) {
            mul++;
        }
        if (Equip_GetSkillPoints(&p->equip[5], 0x2B)) {
            mul++;
        }
        if (Equip_GetSkillPoints(&p->equip[0], 0x2B)) {
            mul++;
        }
        if (Equip_GetSkillPoints(&p->equip[6], 0x2B)) {
            mul++;
        }
        pts *= mul;
        pts += Equip_TorsoUpBonus(&p->equip[2]);
        pts += Equip_TorsoUpBonus(&p->equip[3]);
        pts += Equip_TorsoUpBonus(&p->equip[4]);
        pts += Equip_TorsoUpBonus(&p->equip[5]);
        pts += Equip_TorsoUpBonus(&p->equip[0]);
        pts += Equip_TorsoUpBonus(&p->equip[6]);
    }
    return pts;
}

/* The player's total points of a skill kind over the equipment. */
s8 Player_GetSkillPoints(Player *p, u8 kind)
{
    s8 pts = 0;

    pts += Equip_GetSkillPoints(&p->equip[2], kind);
    pts += Equip_GetSkillPoints(&p->equip[4], kind);
    pts += Equip_GetSkillPoints(&p->equip[5], kind);
    pts += Equip_GetSkillPoints(&p->equip[0], kind);
    pts += Equip_GetSkillPoints(&p->equip[6], kind);
    pts += Player_GetTorsoSkillPoints(p, kind);
    return pts;
}

/* Adds a skill to the next free slot (at most 5). */
void Player_AddSkill(Player *p, u8 skill, u8 *n, u16 group, s8 points)
{
    if (*n < 5) {
        p->skills[*n] = skill;
        p->skillGroups[*n] = group;
        p->skillPoints[*n] = points;
        (*n)++;
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

void Player_CalcSkills(Player *p)
{
    s16 key[5];
    u8 grp[5];
    u8 id[5];
    u8 n;
    SkillDef *e;
    s8 pts;
    s32 i;
    s32 j;
    s16 ks;
    u8 t;

    e = D_0044A9A0;
    n = 0;
    for (i = 0; i < 5; i++) {
        p->skills[i] = 0;
        p->skillGroups[i] = 0;
        p->skillPoints[i] = 0;
    }
    while (e->kind != 0xFFFF) {
        pts = Player_GetSkillPoints(p, e->kind);
        if (pts <= e->neg[0][0]) {
            Player_AddSkill(p, e->neg[0][1], &n, e->kind, pts);
        } else if (pts <= e->neg[1][0]) {
            Player_AddSkill(p, e->neg[1][1], &n, e->kind, pts);
        } else if (pts <= e->neg[2][0]) {
            Player_AddSkill(p, e->neg[2][1], &n, e->kind, pts);
        } else if (pts >= e->pos[2][0]) {
            Player_AddSkill(p, e->pos[2][1], &n, e->kind, pts);
        } else if (pts >= e->pos[1][0]) {
            Player_AddSkill(p, e->pos[1][1], &n, e->kind, pts);
        } else if (pts >= e->pos[0][0]) {
            Player_AddSkill(p, e->pos[0][1], &n, e->kind, pts);
        }
        e++;
    }
    for (i = 0; i < 5; i++) {
        if (p->skillPoints[i] != 0) {
            if (p->skillPoints[i] > 0) {
                key[i] = (s16)p->skillPoints[i] + 200;
                grp[i] = p->skillGroups[i];
                id[i] = p->skills[i];
            } else {
                key[i] = (s16)-p->skillPoints[i] + 100;
                grp[i] = p->skillGroups[i];
                id[i] = p->skills[i];
            }
        } else {
            key[i] = 0;
            grp[i] = 0;
            id[i] = 0;
        }
    }
    for (i = 0; i < 4; i++) {
        for (j = i + 1; j < 5; j++) {
            if (key[i] < key[j] || (key[i] == key[j] && grp[i] < grp[j])) {
                ks = key[i];
                t = grp[i];
                key[i] = key[j];
                grp[i] = grp[j];
                key[j] = ks;
                grp[j] = t;
                t = id[i];
                id[i] = id[j];
                id[j] = t;
            }
        }
    }
    for (i = 0; i < 5; i++) {
        p->skillPoints[i] = key[i];
        p->skillGroups[i] = grp[i];
        p->skills[i] = id[i];
    }
}

/* Nonzero when the player has the skill: in one of the 5 slots, or, offline,
 * the local player's meal skill (+0x8C9). None while Town_InMode46 says so
 * online. */
s32 Player_HasSkill(Player *p, u8 skill)
{
    Player *local = &gPlayers[gScene.localPlayer];
    s16 i;

    if (gScene.online != 0 && Town_InMode46() == 1) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        if (p->skills[i] == skill) {
            return 1;
        }
    }
    if (gScene.online == 0 && local->mealSkill == skill) {
        return 1;
    }
    return 0;
}

/* The player's skill of a skill group (0 if none). */
u8 Player_GetSkill(Player *p, u8 group)
{
    Player *local = &gPlayers[gScene.localPlayer];
    s16 i;

    if (gScene.online != 0 && Town_InMode46() == 1) {
        return 0;
    }
    for (i = 0; i < 5; i++) {
        if (p->skillGroups[i] == group) {
            return p->skills[i];
        }
    }
    if (gScene.online == 0 && local->mealSkillGroup == group) {
        return local->mealSkill;
    }
    return 0;
}

s32 Player_CanGuard(Player *p)
{
    switch (p->weaponType) {
    case 0:
    case 4:
    case 3:
    case 9:
        return 1;
    case 1:
        if (HeavyBowgun_HasShield(p) == 1) {
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
void PlayerEm_PushApart(Player *a, Player *b)
{
    HitShape *c;
    f32 len[2];
    Capsule work;
    Capsule work2;
    HitCapsule own;
    HitCapsule cap;
    HitSphere sph;
    f32 push[2][3];
    f32 y;
    s16 n;
    s16 i;
    s32 k;
    s16 r;
    s32 hit;

    y = a->pos[1];
    n = 0;
    Player_BodyCapsule(a, &own, 40.0f);
    Capsule_Copy(&own, &work);
    if (gScene.online == 0) {
        c = gEmBoneHits[b->weaponType];
    } else {
        c = D_7F1950[b->weaponType];
    }
    while ((k = c->bone) != -1) {
        if ((c->atkFlags & 4) || (c->atkFlags2 & 2) || (c->openFlags & 8) || (c->openFlags & 2)
            || (c->offFlags & 4) || (c->offFlags & 1)) {
            c++;
            continue;
        }
        switch (k) {
        case 0x7F:
        case 0x7E:
            r = HitShape_Place(b->pos, c, &cap, &sph, 1.0f);
            break;
        case 0x7D:
            HitList_Next((Em *)b, &c);
            continue;
        default:
            r = BoneHit_Place((ObjHead *)b, c, &cap, &sph);
            break;
        }
        if (r == 0) {
            hit = (u8)Capsule_PushSphere(&work, sph.pos, push[n], sph.radius);
        } else {
            Capsule_Copy(&cap, &work2);
            hit = (u8)Capsule_PushCapsule(&work2, &work, push[n]);
        }
        if ((u8)hit) {
            len[n] = Vec_Length(push[n]);
            if (a->action != 2) {
                push[n][1] = 0.0f;
            }
            n++;
        }
        if (n >= 2) {
            break;
        }
        c++;
    }
    if (n != 0) {
        for (i = 1; i < n; i++) {
            push[0][0] += push[i][0];
            push[0][1] += push[i][1];
            push[0][2] += push[i][2];
            if (len[0] < len[i]) {
                len[0] = len[i];
            }
        }
        Vec_Normalize(push[0]);
        push[0][0] *= len[0];
        push[0][1] *= len[0];
        push[0][2] *= len[0];
        if ((a->stateGroup == 2 || a->stateGroup == 3) && a->vel[1] > 0.0f && push[0][1] > 0.0f) {
            push[0][1] = 0.0f;
        }
        if (b->pushPriority < a->pushPriority) {
            b->pos[0] -= push[0][0];
            b->pos[1] -= push[0][1];
            b->pos[2] -= push[0][2];
            ((u8 *)b)[0x738] = 1;
        } else if (b->pushPriority == a->pushPriority) {
            a->pos[0] += 0.5f * push[0][0];
            a->pos[1] += 0.5f * push[0][1];
            a->pos[2] += 0.5f * push[0][2];
            /* a height check compiled out (an empty compound if: the schedule needs it) */
            if (a->pos[1] < a->floorY || a->pos[1] > a->floorY + 480.0f) { /* compiled-out check */
            }
            b->pos[0] += -0.5f * push[0][0];
            b->pos[1] += -0.5f * push[0][1];
            b->pos[2] += -0.5f * push[0][2];
            ((u8 *)a)[0x738] = 1;
            ((u8 *)b)[0x738] = 1;
        } else {
            a->pos[0] += push[0][0];
            a->pos[1] += push[0][1];
            a->pos[2] += push[0][2];
            ((u8 *)a)[0x738] = 1;
        }
        if (a->pos[1] < a->floorY) {
            a->pos[1] = a->floorY;
        } else if (y < a->floorY + 480.0f && a->pos[1] > a->floorY + 480.0f) {
            a->pos[1] = a->floorY + 480.0f;
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
void EmEm_PushApart(Player *a, Player *b)
{
    f32 pa[3];
    f32 pb[3];
    f32 len[2];
    f32 push[2][3];
    f32 *ha;
    f32 *hb;
    f32 r;
    s16 n;
    s16 i;

    n = 0;
    if (gScene.online == 0) {
        ha = D_8BA130[a->weaponType];
        hb = D_8BA130[b->weaponType];
    } else {
        ha = D_7F1980[a->weaponType];
        hb = D_7F1980[b->weaponType];
    }
    for (; ha[3] != -1.0f; ha += 4) {
        pa[0] = a->pos[0] + ha[0] * a->scale[0];
        pa[1] = a->pos[1] + ha[1] * a->scale[1];
        pa[2] = a->pos[2] + ha[2] * a->scale[2];
        r = ha[3] * a->scale[0];
        for (; hb[3] != -1.0f; hb += 4) {
            pb[0] = b->pos[0] + hb[0] * a->scale[0];
            pb[1] = b->pos[1] + hb[1] * a->scale[1];
            pb[2] = b->pos[2] + hb[2] * a->scale[2];
            if (Sphere_PushOut(r, hb[3] * b->scale[0], pa, pb, push[n]) != 0) {
                len[n] = Vec_Length(push[n]);
                n++;
            }
            if (n >= 2) {
                goto done;
            }
        }
    }
done:
    if (n != 0) {
        for (i = 1; i < n; i++) {
            push[0][0] += push[i][0];
            push[0][1] += push[i][1];
            push[0][2] += push[i][2];
            if (len[0] < len[i]) {
                len[0] = len[i];
            }
        }
        if (a->action != 2) {
            push[0][1] = 0.0f;
        }
        Vec_Normalize(push[0]);
        push[0][0] *= len[0];
        push[0][1] *= len[0];
        push[0][2] *= len[0];
        if (a->pushPriority == b->pushPriority) {
            a->pos[0] += 0.5f * push[0][0];
            if (a->action == 2) {
                a->pos[1] += 0.5f * push[0][1];
            }
            a->pos[2] += 0.5f * push[0][2];
            b->pos[0] += -0.5f * push[0][0];
            if (b->action == 2) {
                b->pos[1] += -0.5f * push[0][1];
            }
            b->pos[2] += -0.5f * push[0][2];
            ((u8 *)a)[0x738] = 1;
            ((u8 *)b)[0x738] = 1;
        } else if (a->pushPriority < b->pushPriority) {
            a->pos[0] += push[0][0];
            if (a->action == 2) {
                a->pos[1] += push[0][1];
            }
            a->pos[2] += push[0][2];
            ((u8 *)a)[0x738] = 1;
        } else {
            b->pos[0] -= push[0][0];
            if (b->action == 2) {
                b->pos[1] -= push[0][1];
            }
            b->pos[2] -= push[0][2];
            ((u8 *)b)[0x738] = 1;
        }
    }
}

/* Pushes two players apart when their body capsules (Player_BodyCapsule, radius 40) meet:
 * half the push each, the height too for those in the air (action 2), and marks both. */
void Players_PushApart(Player *a, Player *b)
{
    Capsule ca;
    Capsule cb;
    HitCapsule sa;
    HitCapsule sb;
    f32 push[3];

    Player_BodyCapsule(a, &sa, 40.0f);
    Capsule_Copy(&sa, &ca);
    Player_BodyCapsule(b, &sb, 40.0f);
    Capsule_Copy(&sb, &cb);
    if (Capsule_PushCapsule(&cb, &ca, push) != 0) {
        a->pos[0] += 0.5f * push[0];
        a->pos[2] += 0.5f * push[2];
        b->pos[0] += -0.5f * push[0];
        b->pos[2] += -0.5f * push[2];
        if (a->action == 2 && b->action == 2) {
            a->pos[1] += 0.5f * push[1];
            b->pos[1] += -0.5f * push[1];
        } else if (a->action == 2) {
            a->pos[1] += push[1];
        } else if (b->action == 2) {
            b->pos[1] -= push[1];
        }
        a->unk72C[0x738 - 0x72C] = 1;
        b->unk72C[0x738 - 0x72C] = 1;
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
    Em *m;
    Player *q;
    Player *e;
    s32 i;
    s32 j;
    f32 *vy;
    f32 *vz;
    f32 v[3];
    s16 flag;
    f32 d;
    Player *p;

    for (i = 0, q = gPlayers; i < 4; i++, q++) {
        ((u8 *)q)[0x738] = 0;
    }
    for (m = gEms, j = 0; j < 20; j++, m++) {
        m->pushed = 0;
    }
    for (i = 0, p = gPlayers, vy = &v[1], vz = &v[2]; i < 4; i++, p++) {
        if (p->active != 0 && p->visible != 0 && Player_IsLocal(p) != 0 && p->noPushTimer == 0
            && (u8)Obj_InCurrentArea(p)) {
            if (Debug_IsOn(0xA9) != 0) {
                for (j = 0, q = gPlayers; j < 4; j++, q++) {
                    if (q->active != 0 && q->visible != 0 && q->noPushTimer == 0 && (u8)Obj_SameArea(p, q)) {
                        Players_PushApart(p, q);
                    }
                }
            }
            for (q = (Player *)gEms, j = 0; j < 20; j++, q = (Player *)((u8 *)q + sizeof(Em))) {
                if (q->active != 0 && q->visible != 0 && q->noPushTimer == 0 && (u8)Obj_SameArea(p, q)) {
                    PlayerEm_PushApart(p, q);
                    v[0] = p->pos[0] - q->pos[0];
                    *vy = p->pos[1] - q->pos[1];
                    *vz = p->pos[2] - q->pos[2];
                    d = Vec_Length(v);
                    *(f32 *)p->unk3B0 = d;
                }
            }
        }
    }
    for (i = 0, e = (Player *)gEms; i < 19; i++, e = (Player *)((u8 *)e + sizeof(Em))) {
        if (e->active != 0 && e->visible != 0 && e->noPushTimer == 0 && (u8)Obj_InCurrentArea(e)) {
            flag = *(u16 *)((u8 *)e + 0x736) != 0;
            for (j = i + 1, q = (Player *)&gEms[i + 1]; j < 20; j++, q = (Player *)((u8 *)q + sizeof(Em))) {
                if (q->active != 0 && q->visible != 0) {
                    if (e->weaponType == 0x1D && q->weaponType == 0x1D) {
                        continue;
                    }
                    if ((e->weaponType == 0x1D || q->weaponType == 0x1D || (e->startItemList == 0 && q->startItemList == 0))
                        && q->noPushTimer == 0 && (u8)Obj_SameArea(e, q)) {
                        if ((flag == 0 && *(u16 *)((u8 *)q + 0x736) == 0) || e->weaponType == 0x1D || q->weaponType == 0x1D) {
                            EmEm_PushApart(e, q);
                        }
                    }
                }
            }
        }
    }
}

/* Sets motion layer `layer`: the motion, half of `a` and b. */
static void Player_SetLayerMotion(Player *p, s32 motion, s32 a, s16 b, s32 layer)
{
    p->motionId[layer] = motion;
    p->motionBlend[layer] = a / 2;
    p->motionFrame[layer] = b;
    p->motionStarted[layer] = 0;
}

void Player_SetMotion(Player *p, s32 motion, s32 a, s16 b, s32 layer)
{
    p->unk771 = 0;
    Player_SetLayerMotion(p, motion, a, b, layer);
}

/* Starts a motion on layers 0 and 1 (motion + 100). */
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b)
{
    p->unk771 = 0;
    Player_SetLayerMotion(p, motion, a, b, 0);
    Player_SetLayerMotion(p, motion + 100, a, b, 1);
}

void Player_StartMotionLayer(Player *p, u16 motion, s32 a, s16 b, s32 layer)
{
    if (layer < p->motionLayerCount) {
        Mtx_FromAnglesXYZ(p->rot, p->rotMtx);
        p->motionId[layer] = motion;
        p->motionBlend[layer] = a / 2;
        p->motionFrame[layer] = b;
        Player_BeginLayerMotion(p, p->motionFrame[layer], p->motionBlend[layer], layer);
    }
}

void func_003B0510(Player *p, u16 buttons)
{
}

void func_003B0520(Player *p)
{
    func_003B0510(p, ~p->input.held);
}

/* Starts a jump of `frames` frames: the vertical acceleration that stops
 * the rise halfway. */
s32 Player_StartJump(void *obj, s16 frames)
{
    ObjHead *o = obj;
    f32 a;

    frames = frames / 2;
    a = -1.0f * o->vel[1];
    if (frames <= 1 || o->vel[1] < 0.0f) {
        o->accel[1] = a;
        return 1;
    }
    a /= frames;
    o->accel[1] = a;
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
    ObjHead *o = obj;
    o->pos[0] += o->vel[0];
    o->pos[1] += o->vel[1];
    o->pos[2] += o->vel[2];
}

void Player_MoveAccel(Player *p)
{
    p->vel[0] += p->accel[0];
    p->vel[1] += p->accel[1];
    p->vel[2] += p->accel[2];
    p->pos[0] += p->vel[0];
    p->pos[1] += p->vel[1];
    p->pos[2] += p->vel[2];
}

void Player_Integrate(void *obj)
{
    ObjHead *o = obj;
    o->vel[0] += o->accel[0];
    o->vel[1] += o->accel[1];
    o->vel[2] += o->accel[2];
    o->pos[0] += o->vel[0];
    o->pos[1] += o->vel[1];
    o->pos[2] += o->vel[2];
}

/* Moves, and stops on the ground (AreaGrid2_GetFloorY: its height here). */
s32 Player_MoveFall(Player *p)
{
    f32 ground;

    p->vel[0] += p->accel[0];
    p->vel[1] += p->accel[1];
    p->vel[2] += p->accel[2];
    p->pos[0] += p->vel[0];
    p->pos[1] += p->vel[1];
    p->pos[2] += p->vel[2];
    ground = AreaGrid2_GetFloorY(p->pos);
    if (p->pos[1] <= ground) {
        p->pos[1] = ground;
        return 1;
    }
    return 0;
}

void Player_StopMove(void *obj)
{
    ObjHead *o = obj;
    o->accel[2] = 0.0f;
    o->accel[1] = 0.0f;
    o->accel[0] = 0.0f;
    o->vel[2] = 0.0f;
    o->vel[1] = 0.0f;
    o->vel[0] = 0.0f;
}

void Player_ClearVelocity(void *obj)
{
    ObjHead *o = obj;
    o->accel[2] = 0.0f;
    o->accel[1] = 0.0f;
    o->accel[0] = 0.0f;
    o->vel[2] = 0.0f;
    o->vel[1] = 0.0f;
    o->vel[0] = 0.0f;
}

void Player_ClearAccel(Player *p)
{
    p->accel[2] = 0.0f;
    p->accel[1] = 0.0f;
    p->accel[0] = 0.0f;
}

void Player_ClearAccel2(void *obj)
{
    ObjHead *o = obj;
    o->accel[2] = 0.0f;
    o->accel[1] = 0.0f;
    o->accel[0] = 0.0f;
}

void MoveObj_AddVelocity(MoveObj *o)
{
    o->pos[0] += o->vel[0];
    o->pos[1] += o->vel[1];
    o->pos[2] += o->vel[2];
}

void MoveObj_Integrate(MoveObj *o)
{
    o->vel[0] += o->accel[0];
    o->vel[1] += o->accel[1];
    o->vel[2] += o->accel[2];
    o->pos[0] += o->vel[0];
    o->pos[1] += o->vel[1];
    o->pos[2] += o->vel[2];
}

/* The motion's remaining frames (+0x3A4) from layer 0; with `step`, the
 * frame counter +0x3D4 advances too. */
void Player_UpdateMotionTime(Player *p, s16 step)
{
    s16 blend = p->motionBlend[0];
    s16 len = p->motionFrame[0];

    p->motionTime = 0;
    if (blend < 0) {
        blend = -blend;
    }
    p->motionTime += len - blend;
    if (step != 0) {
        if (blend != 0) {
            blend--;
        }
        p->hitDelay += (u8)(blend - len);
    }
}

void Motion_InitTime(void *obj, s16 step)
{
    Player *p = obj;
    s16 blend = p->motionBlend[0];
    s16 len = p->motionFrame[0];

    p->motionTime = 0;
    if (blend < 0) {
        blend = -blend;
    }
    p->motionTime += len - blend;
    if (step != 0) {
        if (blend != 0) {
            blend--;
        }
        p->hitDelay += (u8)(blend - len);
    }
}

void func_003B09B0(Player *unused0, u8 unused1)
{
}

/* As Obj_StartMotionEntry for motion entry i with the owner's +0xC as +8, keeping the bits
 * of +0x6C other than 8 and the byte +0x6E. */
void Obj_StartMotionOwned(MoveObj *o, void *owner, s32 i)
{
    ObjHead *h = owner;
    s32 *src;
    s32 *d;
    s32 k;
    u8 bits;
    u8 keep;

    bits = o->motion.unkC & 0xF7;
    keep = bits == 0 ? 0 : o->motion.unkE;
    src = (s32 *)*o->unk90;
    src += i * 6;
    d = (s32 *)&o->motion;
    for (k = 0; k < 6; k++) {
        *d++ = *src++;
    }
    o->unk8 = h->index;
    o->unkB = 1;
    o->unk1E = 0;
    o->unk9C = 0;
    o->unkA0 = 0;
    if (keep != 0) {
        o->motion.unkC |= bits;
        o->motion.unkE = keep;
    }
    o->motion.unk0 >>= 1;
    o->motion.unk1 >>= 1;
    if (o->motion.unk15 != 0xFF) {
        o->motion.unk15 >>= 1;
    }
    o->motion.unk16 >>= 1;
    o->unkB5 = HitId_Next();
}

void MoveObj_StartMotionOwned(MoveObj *o, s32 a, s32 b, void *c)
{
    o->unk90 = (MotionDef **)c;
    o->motion.unkE = 0;
    o->motion.unkC = 0;
    Obj_StartMotionOwned(o, (void *) a, b);
}

/* Starts motion entry i of the object table: the entry copied word by word, the first
 * values halved, a new hit id. */
void Obj_StartMotionEntry(MoveObj *o, s32 i)
{
    s32 *src;
    s32 *d;
    s32 k;

    src = (s32 *)*o->unk90;
    src += i * 6;
    d = (s32 *)&o->motion;

    for (k = 0; k < 6; k++) {
        *d++ = *src++;
    }
    o->unk8 = 0xFF;
    o->unkB = 1;
    o->unk1E = 0;
    o->unk9C = 0;
    o->unkA0 = 0;
    o->motion.unk0 >>= 1;
    o->motion.unk1 >>= 1;
    if (o->motion.unk15 != 0xFF) {
        o->motion.unk15 >>= 1;
    }
    o->motion.unk16 >>= 1;
    o->unkB5 = HitId_Next();
}

void MoveObj_StartMotionEntry(MoveObj *o, s32 a, void *b)
{
    o->unk90 = (MotionDef **)b;
    Obj_StartMotionEntry(o, a);
}


/* Places the hit shape def in the world by its owner's bone: a sphere into s (0), a
 * capsule into c (1), -1 for none. */
s32 BoneHit_Place(ObjHead *o, HitShape *def, HitCapsule *c, HitSphere *s)
{
    f32 off[3];
    f32 pos[3];
    EmBone *bones;

    if (def->bone == 0x7F) {
        return -1;
    }
    bones = o->model->bones;
    Mtx_GetTrans(pos, bones[def->bone].world);
    switch (def->kind) {
    case 0:
        s->radius = def->radius * o->scale[0];
        Vec_Rotate(off, def->a, (f32 *)&bones[def->bone]);
        s->pos[0] = pos[0] + off[0];
        s->pos[1] = pos[1] + off[1];
        s->pos[2] = pos[2] + off[2];
        return 0;
    case 1:
        c->radius = def->radius * o->scale[0];
        Vec_Rotate(off, def->a, (f32 *)&bones[def->bone]);
        c->a[0] = pos[0] + off[0];
        c->a[1] = pos[1] + off[1];
        c->a[2] = pos[2] + off[2];
        Vec_Rotate(off, def->b, (f32 *)&bones[def->bone]);
        c->b[0] = pos[0] + off[0];
        c->b[1] = pos[1] + off[1];
        c->b[2] = pos[2] + off[2];
        return 1;
    }
    return -1;
}

/* A sphere (type 0: x, y, z, r) or a capsule (type 1: two points and r)
 * from a hit record placed at pos, the radius scaled by s. */
s32 HitShape_Place(f32 *pos, HitShape *e, HitCapsule *cap, HitSphere *sph, f32 s)
{
    switch (e->kind) {
    case 0:
        sph->radius = e->radius * s;
        sph->pos[0] = pos[0] + e->a[0];
        sph->pos[1] = pos[1] + e->a[1];
        sph->pos[2] = pos[2] + e->a[2];
        return 0;
    case 1:
        cap->radius = e->radius * s;
        cap->a[0] = pos[0] + e->a[0];
        cap->a[1] = pos[1] + e->a[1];
        cap->a[2] = pos[2] + e->a[2];
        cap->b[0] = pos[0] + e->b[0];
        cap->b[1] = pos[1] + e->b[1];
        cap->b[2] = pos[2] + e->b[2];
        return 1;
    }
    return -1;
}

s32 Capsule_FromPoints(f32 *a, f32 *b, HitShape *c, HitCapsule *out, f32 s)
{
    out->radius = c->radius * s;
    out->a[0] = a[0];
    out->a[1] = a[1];
    out->a[2] = a[2];
    out->b[0] = b[0];
    out->b[1] = b[1];
    out->b[2] = b[2];
    return 1;
}

/* The floor fwd ahead of the player (along its heading): 1 with its height in *out when
 * it lies from up to up + range above the player's height, 0 when not, 1 with none found. */
s32 Player_FloorAheadAbove(Player *p, f32 *out, f32 fwd, f32 up, f32 range)
{
    f32 m[16];
    f32 v[3];
    f32 off[3];
    s32 ang[3];
    f32 y;
    f32 *vy;
    f32 *vz;
    f32 h;

    vy = &v[1];
    vz = &v[2];
    v[0] = 0.0f;
    *vy = 0.0f;
    *vz = fwd;
    ang[0] = 0;
    ang[1] = p->rot[1];
    ang[2] = 0;
    Mtx_FromAnglesXYZ(ang, m);
    Vec_Rotate(off, v, m);
    v[0] = p->pos[0] + off[0];
    *vy = p->pos[1] + off[1];
    *vz = p->pos[2] + off[2];
    h = *vy + up;
    if (Em_GetFloorYAbove((struct Em *)p, v, &y) != 1) {
        return 1;
    }
    if (!(y < h) && y <= h + range) {
        *out = y;
        return 1;
    }
    return 0;
}

/* The floor fwd ahead of the player (along its heading) against the player's height plus
 * up: 1 when it is below that (above it with `above`), 0 when not or with no floor. */
s32 Player_FloorAheadBelow(Player *p, u16 above, f32 fwd, f32 up)
{
    f32 m[16];
    f32 v[3];
    f32 off[3];
    s32 ang[3];
    f32 y;
    f32 *vy;
    f32 *vz;
    f32 h;

    vy = &v[1];
    vz = &v[2];
    v[0] = 0.0f;
    *vy = 0.0f;
    *vz = fwd;
    ang[0] = 0;
    ang[1] = p->rot[1];
    ang[2] = 0;
    Mtx_FromAnglesXYZ(ang, m);
    Vec_Rotate(off, v, m);
    v[0] = p->pos[0] + off[0];
    *vy = p->pos[1] + off[1];
    *vz = p->pos[2] + off[2];
    h = p->pos[1] + up;
    if (Em_GetFloorYAbove((struct Em *)p, v, &y) != 1) {
        return 0;
    }
    if (above == 0) {
        if (y < h) {
            return 1;
        }
    } else if (!(y <= h)) {
        return 1;
    }
    return 0;
}

/* Nonzero when the player is in the current area (SceneState.area). */
s32 Obj_InCurrentArea(void *obj)
{
    ObjHead *o = obj;
    return o->area == gScene.area;
}

u8 Player_InCurrentArea(void *obj)
{
    ObjHead *o = obj;
    return o->area == gScene.area;
}

/* Nonzero when two players (or a player and a monster) are in the same area. */
s32 Obj_SameArea(void *obj, Player *b)
{
    ObjHead *o = obj;
    return o->area == b->area;
}

void Player_UpdateAreaPos(void *obj)
{
    ObjHead *o = obj;
    AreaRecord *a = Stage_GetAreaRecord(o->area);

    o->mapPos[0] = o->pos[0] + a->originX;
    o->mapPos[1] = o->pos[1] + (a->bottom + a->top) / 2.0f;
    o->mapPos[2] = o->pos[2] + a->originZ;
}

void Area_ToWorld(s32 area, f32 *pos, f32 *out)
{
    AreaRecord *a = Stage_GetAreaRecord(area);

    out[0] = pos[0] + a->originX;
    out[1] = pos[1] + (a->bottom + a->top) / 2.0f;
    out[2] = pos[2] + a->originZ;
}

/* The gathering point the player stands at (as Gather_AtPoint): its item table, with
 * its +0x14 in *id and its position in pos; 0xFFFF for none. */
u16 Player_GetGatherPoint(Player *p, u16 *id, f32 *pos)
{
    GatherPt *pt;
    f32 dx;
    f32 dz;
    u16 table;

    pt = Gather_GetPoints(p->area);
    if (pt == NULL) {
        return 0xFFFF;
    }
    for (; pt->x != -1.0f; pt++) {
        if (p->pos[1] < pt->y - 200.0f) {
            continue;
        }
        if (!(p->pos[1] < pt->y + 100.0f)) {
            continue;
        }
        dx = p->pos[0] - pt->x;
        dz = p->pos[2] - pt->z;
        if (!(Math_Sqrtf(dx * dx + dz * dz) <= pt->radius)) {
            continue;
        }
        table = pt->table;
        *id = pt->unk14;
        pos[0] = pt->x;
        pos[1] = pt->y;
        pos[2] = pt->z;
        return table;
    }
    return 0xFFFF;
}
