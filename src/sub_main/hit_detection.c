/*
 * The attack objects' hits and what follows a hit. Each frame
 * ObjAttach_HitAll tests every attack object (ObjAttach) whose hit window is
 * open against the monsters, the players and the other objects of its area,
 * by their hit shapes (HitShape lists); the first hit runs ObjAttach_DamageEm
 * or ObjAttach_DamagePlayer (damage, statuses, hit stop, sounds) and
 * ObjAttach_HitEffects (sparks and splashes). The hit marks and statuses
 * are cleared here, and the hit windows' timers (hitPhase, hitDelay,
 * hitFrames) and the hit ids are kept here too.
 */
#include "common.h"
#include "em.h"
#include "scene.h"
#include "player.h"
#include "objhead.h"
#include "objattach.h"
#include "stageobj.h"
#include "sprite.h"
#include "capsule.h"
#include "area.h"
#include "render_state.h"
#include "pad.h"
#include "lobby_state.h"
#include "shadow.h"
#include "item.h"
#include "sub_main/hit_detection.h"   /* the texts this file uses (tools/content_gen.py) */
#include "bgm.h"
#include "place_rules.h"
#include "card.h"
#include "light.h"
#include "quest.h"
#include "save.h"
#include "sound.h"
#include "text.h"
#include "bone_effects.h"
#include "dust_flash_effects.h"
#include "gm_em.h"
#include "hit_setup.h"
#include "hit_spark.h"
#include "vibration.h"
#include "libc.h"
#include "libgcc.h"
#include "vec_math.h"

#pragma optimization_level 4

extern ObjAttach *gObjAttachList;
/* end of prototypes */

/* Clears the hit marks of a player or monster (its ObjHead): hitPhase, the 16 hit ids taken
 * (hitIds) and the next slot (hitIdNext). */
static void Obj_ClearHitMarks(void *obj)
{
    ObjHead *head = obj;
    s16 i;

    head->hitPhase = 0;
    for (i = 0; i < 16; i++) {
        head->hitIds[i] = 0;
    }
    head->hitIdNext = 0;
}

extern Em gEms[];
/* Restarts the hit ids and clears the hit marks of the four players and the 20 monsters. */
void Hit_ResetAll(void)
{
    s32 i;

    HitId_Reset();
    for (i = 0; i < SCENE_PLAYERS; i++) {
        Obj_ClearHitMarks(&gPlayers[i]);
    }
    for (i = 0; i < EM_COUNT; i++) {
        Obj_ClearHitMarks(&gEms[i]);
    }
}

/* Clears the hit and status buildup state of the players and the monsters. */
void Hit_ClearStatusAll(void)
{
    Player *player;
    s32 i;
    s32 j;
    Em *em;
    s32 k;

    for (i = 0, player = gPlayers; i < SCENE_PLAYERS; i++, player++) {
        player->hitReceived = 0;
        player->status5Hit = 0;
        for (j = 0; j < 8; j++) {
            player->hitDamage[j] = 0;
            player->partFlags[j] = 0;
        }
        player->status2Hit = 0;
        player->status1Hit = 0;
        player->status4Hit = 0;
        player->poisonHit = 0;
        player->mashBuildupHit = 0;
        player->status6Hit = 0;
        player->status7Hit = 0;
        player->status8Hit = 0;
        player->hitAttrs = 0;
    }
    for (k = 0, em = gEms; k < EM_COUNT; k++, em++) {
        em->hitReceived = 0;
        em->status5Hit = 0;
        for (j = 0; j < 8; j++) {
            em->hitPartDamage[j] = 0;
            em->partFlags[j] = 0;
        }
        em->status2Hit = 0;
        em->status1Hit = 0;
        em->status4Hit = 0;
        em->poisonHit = 0;
        em->mashBuildupHit = 0;
        em->hitAttrs = 0;
        em->lastAttackKind = 0;
    }
    ObjAttach_HitAll();
}

/* Steps *cur past a hit shape, or past a group header and the zone records it holds when the monster's fxFlags has the bits of its kind. */
void HitList_Next(Em *em, HitShape **cur)
{
    HitShape *shape = *cur;

    if (em->fxFlags & shape->kind) {
        *cur = shape + (shape->zone + 1);
    } else {
        *cur = shape + 1;
    }
}

/* HitList_Next for an attack object's list: a group header (HIT_BONE_GROUP) tests the fxFlags
 * of the monster that owns the object; with no owner, or a player owner, it steps one record. */
void HitList_NextForObj(ObjAttach *attach, HitShape **cur)
{
    HitShape *shape;

    if (attach->ownerIndex == ATTACH_OWNER_NONE || attach->isEm == 0) {
        *cur += 1;
        return;
    }
    shape = *cur;
    if (((Em *)attach->owner)->fxFlags & shape->kind) {
        *cur = shape + (shape->zone + 1);
    } else {
        *cur = shape + 1;
    }
}

/* Applies the status effects of hit o (elementFlags, elementPower) to its target, a player or a monster
 * read as a Player (the status 6 to 8 fields are the player's): the hit amounts and timers. */
void Em_ApplyHitStatus(Player *target, ObjAttach *attach)
{
    Quest_IsVersus();
    if (attach->elementFlags & ELEMF_EM_STATUS2) {
        target->status2Hit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_POISON) {
        target->poisonHit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_EM_STATUS4) {
        target->status4Hit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_EM_STATUS1) {
        target->status1Hit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_MASH) {
        target->mashBuildupHit += attach->elementPower;
    }
    if (attach->elementFlags & ELEMF_STATUS6) {
        target->status6Hit = 1800; /* frames: 30 s */
    }
    if (attach->elementFlags & ELEMF_STATUS7) {
        target->status7Hit = 1800; /* frames: 30 s */
    }
    if (attach->elementFlags & ELEMF_STATUS8) {
        target->status8Hit = 10;
    }
}

void HitSplash_CreateOnEm(Em *em, ObjAttach *o, u8 a, s16 b, f32 *pos, f32 scale);
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
void ElementBurst_Create(f32 *pos, s32 k, u16 v, void *owner, f32 scale);
/* unit's own declaration: the definition is (Em *em, f32 *pos, u8 k, s16 mode, s16 part, f32 scale); the calls here pass the scale before the hit kind, the order in which the game's code loads them */
void HitSplash_CreateByHitFx(Em *em, f32 *pos, s32 a, s32 b, f32 scale, s16 kind);
/* The effects and sounds of a hit of attach (owned by attacker) on target (a player or a monster,
 * read as a Player) at out, by the hit effect kind (hitFx & HITFX_KIND_MASK): a guard spark when
 * a player guards it or the target is monster kind 29, else the sparks, splashes and element
 * bursts of the kind, by the hit's hitKind. */
void ObjAttach_HitEffects(ObjAttach *attach, Player *attacker, ObjHead *targetHead, f32 *out)
{
    Player *target = (Player *)targetHead;

    if ((attach->hitFx & HITFX_KIND_MASK) != 0 && attach->area == gScene.area) {
        if (attacker != NULL && attacker->isEm == 0) {
            if (attach->hitFx & HITFX_CRITICAL) {
                PlayerFx_CreateAt(out, 16, 0, 1.0f);
            } else if (attach->hitFx & HITFX_WEAK) {
                PlayerFx_CreateAt(out, 16, 1, 1.0f);
            }
        }
        if (target->isEm == 1 && target->weaponType == 29) {
            HitSpark_SpawnForPlayer(target, 0, out);
            HitSplash_CreateOnEm((Em *) target, attach, 6, 1, out, 1.0f);
            return;
        }
        if (target->isEm == 0 && Player_CanGuard(target) == 1 && (!(target->hitAttrs & 0x80) || Player_HasSkill(target, SKILL_GUARD_PLUS20) == 1)) {
            if (Player_TestFlag(target, PLF_GUARD) != 0
                || ((Player_HasSkill(target, SKILL_AUTO_GUARD) == 1 || HeavyBowgun_HasShield(target) == 1) && target->weaponOut != 0 && target->prevFreeState != 0)) {
                if (Player_InGuardArc(target->rot[1], target->hitAngle) == 1 && target->stamina >= 75) {
                    HitSpark_SpawnForPlayer(target, 0, out);
                    HitSplash_CreateOnEm((Em *) target, attach, 6, 1, out, 1.0f);
                    return;
                }
            }
        }
        switch (attach->hitFx & HITFX_KIND_MASK) {
        case 28:
            FireBlast_CreateScaled(out, 0, 1.0f);
            return;
        case 18:
            ShotFlash_CreateAt(out, 8, 0);
            return;
        case 1:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
        case 17:
        case 20:
        case 27:
            switch (attach->hitFx & HITFX_KIND_MASK) {
            case 6:
                ShotFlash_CreateAt(out, 7, 1);
                break;
            case 7:
                ShotFlash_CreateAt(out, 7, 2);
                break;
            case 8:
                ShotFlash_CreateAt(out, 7, 3);
                break;
            case 17:
                ShotFlash_CreateAt(out, 7, 5);
                break;
            case 20:
                ShotFlash_CreateAt(out, 7, 6);
                break;
            case 4:
            case 5:
                switch (attach->hitKind) {
                case 0:
                case 2:
                case 3:
                    if ((attach->hitFx & HITFX_KIND_MASK) == 4) {
                        HitSpark_SpawnForPlayer(target, 7, out);
                    } else {
                        HitSpark_SpawnForPlayer(target, 5, out);
                    }
                    HitSpark_SpawnForPlayer(target, 6, out);
                    break;
                }
            case 1:
            case 27:
                if (attacker != NULL && attacker->isEm == 0) {
                    if (attach->elementFlags & ELEMF_FIRE) {
                        ElementBurst_Create(out, 0, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_WATER) {
                        ElementBurst_Create(out, 2, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_THUNDER) {
                        ElementBurst_Create(out, 3, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_ICE) {
                        ElementBurst_Create(out, 8, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_DRAGON) {
                        PlayerFx_CreateAtDefault(out, 12, 1.0f);
                    } else if (attach->elementFlags & ELEMF_POISON) {
                        ElementBurst_Create(out, 1, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_EM_STATUS4) {
                        ElementBurst_Create(out, 7, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_EM_STATUS2) {
                        ElementBurst_Create(out, 6, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_STATUS7) {
                        ElementBurst_Create(out, 6, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_STATUS6) {
                        ElementBurst_Create(out, 1, target->hitAngle, attacker, 1.0f);
                    } else if (attach->elementFlags & ELEMF_STATUS8) {
                        ElementBurst_Create(out, 7, target->hitAngle, attacker, 1.0f);
                    }
                }
                break;
            }
            switch (attach->hitKind) {
            case 1:
                if (attach->ownerIndex != ATTACH_OWNER_NONE) {
                    HitSpark_SpawnForPlayer(target, 0, out);
                    HitSplash_CreateOnEm((Em *) target, attach, 6, 1, out, 1.0f);
                }
                break;
            default:
            case 0:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 0, out, 0.8f);
                HitSplash_CreateByHitFx((Em *) target, out, 2, 0, 0.8f, attach->hitFx & HITFX_KIND_MASK);
                break;
            case 2:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 2, out, 0.8f);
                HitSplash_CreateByHitFx((Em *) target, out, 2, 2, 0.8f, attach->hitFx & HITFX_KIND_MASK);
                break;
            case 3:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 3, out, 1.0f);
                HitSplash_CreateByHitFx((Em *) target, out, 5, 3, 1.0f, attach->hitFx & HITFX_KIND_MASK);
                HitSpark_SpawnForOwner(target, 0, 4, 3, out, 1.0f);
                break;
            }
            if ((attach->hitFx & HITFX_KIND_MASK) == 27 && attach->hitKind != 1) {
                HitSpark_SpawnForPlayer(target, 12, out);
                return;
            }
            break;
        case 9:
        case 11:
        case 13:
        case 15:
            ShotFlash_CreateAt(out, 7, 0);
        case 10:
        case 12:
        case 14:
        case 16:
            switch (attach->hitFx & HITFX_KIND_MASK) {
            case 9:
            case 10:
                if (attach->ownerIndex != ATTACH_OWNER_NONE) {
                    HitSpark_SpawnForPlayer(target, 0, out);
                    HitSplash_CreateOnEm((Em *) target, attach, 6, 1, out, 1.0f);
                    return;
                }
                break;
            case 15:
            case 16:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 0, out, 0.8f);
                HitSplash_CreateByHitFx((Em *) target, out, 2, 0, 0.8f, attach->hitFx & HITFX_KIND_MASK);
                return;
            case 11:
            case 12:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 2, out, 0.8f);
                HitSplash_CreateByHitFx((Em *) target, out, 2, 2, 0.8f, attach->hitFx & HITFX_KIND_MASK);
                return;
            case 13:
            case 14:
                HitSplash_CreateOnEm((Em *) target, attach, 0, 3, out, 1.0f);
                HitSplash_CreateByHitFx((Em *) target, out, 5, 3, 1.0f, attach->hitFx & HITFX_KIND_MASK);
                HitSpark_Spawn((u8 *)target, attach->hitFx & HITFX_KIND_MASK, 4, 3, out, 1.0f);
                return;
            }
            break;
        case 2:
            HitSplash_CreateOnEm((Em *) target, attach, 8, 0, out, 1.0f);
            HitSplash_CreateOnEm((Em *) target, attach, 9, 0, out, 1.0f);
            return;
        case 3:
            Effect06_SpawnWeapon(target, 0, 0);
            return;
        case 23:
            if (target->isEm == 0) {
                Effect06_SpawnWeapon(target, 0, 1);
                return;
            }
            break;
        case 21:
            if (target->isEm == 0) {
                Effect06_SpawnWeapon(target, 0, 2);
                return;
            }
            break;
        case 22:
            if (target->isEm == 0) {
                Effect06_SpawnWeapon(target, 0, 3);
                return;
            }
            break;
        case 29:
            if (target->isEm == 0) {
                Effect06_SpawnAtBone(target, 2, 25, 10, 4.0f);
                /* (f32 *)&out, as the game: the address of the pointer out itself, so the bone's
                 * position is written over it and the stack after it */
                Em_GetBonePos(target, 10, (f32 *)&out);
                Sound_PlayPlayerSe(target, 114, 0, (f32 *)&out, 1, 0);
                return;
            }
            break;
        case 19:
            Effect06_SpawnWeapon(target, 9, 0);
            return;
        case 24:
            if (attacker != NULL && attacker->isEm == 0) {
                if (attach->elementFlags & ELEMF_FIRE) {
                    ElementBurst_Create(out, 0, target->hitAngle, attacker, 1.5f);
                } else if (attach->elementFlags & ELEMF_WATER) {
                    ElementBurst_Create(out, 2, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_THUNDER) {
                    ElementBurst_Create(out, 3, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_DRAGON) {
                    PlayerFx_CreateAtDefault(out, 12, 1.0f);
                } else if (attach->elementFlags & ELEMF_ICE) {
                    ElementBurst_Create(out, 8, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_POISON) {
                    ElementBurst_Create(out, 1, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_EM_STATUS4) {
                    ElementBurst_Create(out, 7, target->hitAngle, attacker, 1.2f);
                } else if (attach->elementFlags & ELEMF_EM_STATUS2) {
                    ElementBurst_Create(out, 6, target->hitAngle, attacker, 1.2f);
                }
            }
            HitSplash_CreateOnEm((Em *) target, attach, 0, 2, out, 0.8f);
            HitSplash_CreateByHitFx((Em *) target, out, 2, 2, 0.8f, 12);
            return;
        case 25:
            if (target->isEm == 0) {
                PlayerFx_CreateOnPlayerDefault(target, 1, 1.0f);
                return;
            }
            break;
        case 26:
            if (target->isEm == 0) {
                PlayerHitBurst_Create(target);
                return;
            }
            break;
        case 30:
            ShotFlash_CreateAt(out, 7, 8);
            return;
        case 31:
            ShotFlash_CreateAt(out, 9, 1);
            return;
        case 32:
            ShotFlash_CreateAt(out, 9, 2);
            return;
        case 33:
            ShotFlash_CreateAt(out, 9, 3);
            break;
        }
    }
}

/* Counts down the player's own hit window (hitDelay, then hitFrames) by its hitPhase (0 off,
 * 1 waiting, 2 open), as ObjAttach_UpdateTimers does for the attack objects. When the window
 * closes with a next row set (nextRow), Player_StartNextHit is called with it (it does nothing)
 * and the window reopens. */
void Player_UpdateHitTimers(Player *player)
{
    if (Player_TestFlag(player, 0x800) != 0) {
        player->hitPhase = 1;
        Player_ClearFlag(player, 0x40000);
    }
    if (player->hitStop != 0) {
        return;
    }
    if (Player_TestFlag(player, PLF_HIT_WINDOW) == 0) {
        player->hitPhase = 0;
        Player_ClearFlag(player, 0x40000);
    }
    switch (player->hitPhase) {
    case 0:
        break;
    case 1:
        if (player->hitDelay-- != 0) {
            break;
        }
        player->hitDelay = 0;
        player->hitPhase++;
    case 2:
        if (player->hitFrames == 0) {
            if (player->nextRow != 0) {
                Player_StartNextHit(player, player->nextRow);
                Player_ClearFlag(player, 0x40000);
                if (player->hitDelay == 0) {
                    player->hitPhase = 2;
                    player->hitFrames--;
                } else {
                    player->hitPhase = 1;
                }
            } else {
                player->hitPhase = 0;
            }
        } else {
            player->hitFrames--;
        }
        break;
    }
}

extern Player gPlayers[];
/* Tests attach's attack shapes (hitList) against the hurt shapes (hurtList) of the other
 * active objects of its area: not those of its own side's owners, not one already hit
 * (wasHit), not an ATKF_SHIELDED one unless attach is a monster's with ATKF_PIERCES. The first
 * object hit gets wasHit and hitBy (the player or monster at the HIT object's own idx, as the
 * game indexes it; NULL when attach has no owner). The trap, hold, roar and wind attack kinds
 * don't hit objects. */
void ObjAttach_HitObjects(ObjAttach *attach)
{
    HitShape *list;
    HitShape *elist;
    f32 out[3];
    HitCapsule segA;
    HitCapsule segB;
    HitSphere sphA;
    HitSphere sphB;
    Capsule capA;
    Capsule capB;
    ObjAttach *other;
    void *owner;
    void *eowner;
    s16 shapeA;
    s16 shapeB;
    s16 hit;

    other = gObjAttachList;
    switch (attach->attackKind) {
    case ATTACK_TRAP_A:
    case ATTACK_TRAP_B:
    case ATTACK_HOLD:
    case ATTACK_ROAR:
    case ATTACK_ROAR_WEAK:
    case ATTACK_ROAR_STRONG:
    case ATTACK_WIND:
    case ATTACK_WIND_WEAK:
        return;
    }
    if (attach->isEm == 0) {
        owner = &gPlayers[attach->idx];
    } else {
        owner = &gEms[attach->idx];
    }
    for (; other != NULL; other = other->next) {
        if (attach->area != other->area || other->active == 0) {
            continue;
        }
        if (other->isEm == attach->isEm && attach->ownerIndex != ATTACH_OWNER_NONE && other->ownerIndex != ATTACH_OWNER_NONE) {
            continue;
        }
        if (other->hurtList == 0 || other->wasHit != 0) {
            continue;
        }
        if (Player_TestFlags(other, ATKF_SHIELDED) != 0 && attach->isEm != 0 && Player_TestFlags(attach, ATKF_PIERCES) == 0) {
            continue;
        }
        list = attach->hitList;
        if (other->isEm == 0) {
            eowner = &gPlayers[other->idx];
        } else {
            eowner = &gEms[other->idx];
        }
        while (list->bone != HIT_BONE_END) {
            switch (list->bone) {
            case HIT_BONE_AT_POS:
                shapeA = HitShape_Place(attach->pos, list, &segA, &sphA, attach->hitScale);
                break;
            case HIT_BONE_SEGMENT:
                shapeA = Capsule_FromPoints(attach->pos, attach->pos2, list, &segA, attach->hitScale);
                break;
            case HIT_BONE_GROUP:
                HitList_NextForObj(attach, &list);
                continue;
            default:
                shapeA = BoneHit_Place(owner, list, &segA, &sphA);
                break;
            }
            if (shapeA != 0) {
                Capsule_Copy(&segA, &capA);
            }
            elist = other->hurtList;
            while (elist->bone != HIT_BONE_END) {
                switch (elist->bone) {
                case HIT_BONE_AT_POS:
                    shapeB = HitShape_Place(other->pos, elist, &segB, &sphB, 1.0f);
                    break;
                case HIT_BONE_SEGMENT:
                    shapeB = Capsule_FromPoints(other->pos, other->pos2, elist, &segB, 1.0f);
                    break;
                case HIT_BONE_GROUP:
                    HitList_NextForObj(other, &elist);
                    continue;
                default:
                    shapeB = BoneHit_Place(eowner, elist, &segB, &sphB);
                    break;
                }
                if (shapeB != 0) {
                    Capsule_Copy(&segB, &capB);
                }
                if (shapeA == 1) {
                    if (shapeB == 1) {
                        hit = Capsule_ContactCapsule(&capA, &capB, out);
                    } else {
                        hit = (u8)Capsule_SphereContactOnSphere(&capA, sphB.pos, out, sphB.radius);
                    }
                } else if (shapeB == 1) {
                    hit = (u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
                } else {
                    hit = (u8)Spheres_Touch(sphA.radius, sphB.radius, sphA.pos, sphB.pos, out);
                }
                if (hit != 0) {
                    other->wasHit = 1;
                    if (attach->ownerIndex == ATTACH_OWNER_NONE) {
                        other->hitBy = NULL;
                    } else if (attach->isEm == 0) {
                        other->hitBy = &gPlayers[other->idx];
                    } else {
                        other->hitBy = &gEms[other->idx];
                    }
                    goto next; /* leaves both shape loops at once: the next object */
                }
                elist++;
            }
            list++;
        }
    next:;
    }
}

void ObjAttach_TryHitTarget(ObjAttach *o, Player *p, ObjHead *e);
/* For every active attack object: clears didHit, tests the other objects while its window is
 * open (ObjAttach_HitObjects), then tries the monsters and the players of its area it may hit:
 * not its owner unless ATKF_HITS_OWNER, not one that already took its hit id, not one that took
 * a hit this frame. The trap kinds skip a monster in the air or dead, the hold kind also one
 * under the sand or in a move it doesn't break off; neither hits players. */
void ObjAttach_HitAll(void)
{
    ObjAttach *attach;
    void *owner;
    Em *em;
    s32 i;
    s32 k;
    s32 j;
    Player *player;

    for (attach = gObjAttachList; attach != NULL; attach = attach->next) {
        if (attach->active == 0) {
            continue;
        }
        attach->didHit = 0;
        if (attach->hitPhase == 2) {
            ObjAttach_HitObjects(attach);
        }
        if (attach->ownerIndex == ATTACH_OWNER_NONE) {
            owner = NULL;
        } else if (attach->isEm == 0) {
            owner = &gPlayers[attach->idx];
        } else {
            owner = &gEms[attach->idx];
        }
        for (i = 0, em = gEms; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->area == attach->area && Player_TestFlags(attach, ATKF_NO_EM) == 0) {
                if (attach->ownerIndex == ATTACH_OWNER_NONE || owner != em || Player_TestFlags(attach, ATKF_HITS_OWNER) != 0) {
                    for (k = 0; k < 16; k++) {
                        if (attach->hitId == em->hitIds[k]) {
                            goto next_em; /* hit by this attack already; a break and a test after the loop compile differently */
                        }
                    }
                    if (em->hitReceived == 0) {
                        switch (attach->attackKind) {
                        case ATTACK_HOLD:
                            if (em->stance == EM_STANCE_UNDER || em->stateArmor > 0) {
                                break;
                            }
                        case ATTACK_TRAP_A:
                        case ATTACK_TRAP_B:
                            if (em->stance == EM_STANCE_AIR || em->hp <= 0) {
                                break;
                            }
                        default:
                            ObjAttach_TryHitTarget(attach, owner, (ObjHead *)em);
                            break;
                        }
                    }
                }
            }
        next_em:;
        }
        for (player = gPlayers, j = 0; j < gScene.playerCount; j++, player++) {
            if (player->active != 0 && player->area == attach->area && Player_TestFlags(attach, ATKF_NO_PLAYER) == 0) {
                if (!(attach->attrs & 1) || (Players_AnySide() != 0 && Side_Match(attach->side, player->side, 1) != 1)) {
                    if (attach->ownerIndex == ATTACH_OWNER_NONE || owner != player || Player_TestFlags(attach, ATKF_HITS_OWNER) != 0) {
                        for (k = 0; k < 16; k++) {
                            if (attach->hitId == player->hitIds[k]) {
                                goto next_pl; /* as next_em */
                            }
                        }
                        if (player->hitReceived == 0) {
                            switch (attach->attackKind) {
                            default:
                                ObjAttach_TryHitTarget(attach, owner, (ObjHead *)player);
                                break;
                            case ATTACK_TRAP_A:
                            case ATTACK_TRAP_B:
                            case ATTACK_HOLD:
                                break;
                            }
                        }
                    }
                }
            }
        next_pl:;
        }
    }
}

void ObjAttach_HitTarget(ObjAttach *o, Player *p, ObjHead *e);
/* Runs ObjAttach_HitTarget for attach on target (a player or a monster) when the target is
 * active and in the current area, the attacker is not in hit stop (or there is none), and
 * attach's delay is over and its window open (hitPhase 2). */
void ObjAttach_TryHitTarget(ObjAttach *attach, Player *attacker, ObjHead *target)
{
    if (target->active != 0 && (attach->ownerIndex == ATTACH_OWNER_NONE || attacker->hitStop == 0)) {
        if (attach->delay == 0 && (u8)Obj_InCurrentArea(target) != 0) {
            switch (attach->hitPhase) {
            case 2:
                ObjAttach_HitTarget(attach, attacker, target);
                break;
            }
        }
    }
}

/* 1 when target is a monster another console runs (Em.remote): the damage functions then mark
 * the hit but leave the damage to that console. Debug switch 42 makes it 0 always. */
s32 ObjAttach_TargetIsRemote(Em *target)
{
    if (Debug_IsOn(42) != 0) {
        return 0;
    }
    if (target->isEm != 0 && target->remote != 0) {
        return 1;
    }
    return 0;
}

/* Which of shape's open states hold now: bit 1 when it has a wind bit (HITSHAPE_WIND_*) and
 * em's windTimer runs, bit 2 when it has a hermit bit (HITSHAPE_HERMIT_*) and its hermitTimer
 * runs. ObjAttach_DamageEm deflects an attack on them. */
static u8 ObjAttach_HitFlags(Em *em, HitShape *shape)
{
    u8 flags = shape->openFlags;
    u8 open = 0;

    if ((flags & (HITSHAPE_WIND_OPEN | HITSHAPE_WIND_ONLY)) && em->windTimer > 0) {
        open |= HIT_OPEN_WIND;
    }
    if ((flags & (HITSHAPE_HERMIT_OPEN | HITSHAPE_HERMIT_ONLY)) && (s8)em->hermitTimer > 0) {
        open |= HIT_OPEN_HERMIT;
    }
    return open;
}

static u8 ObjAttach_HitFlags(Em *p, HitShape *e);
s32 Player_HasSongEffect(Player *pl, s16 kind);
extern s8 gSeVolumeByType[];
extern f32 gHitKindMvFactors[];
extern f32 gSharpnessRawMods[];
extern f32 gSharpnessElemMods[];
extern f32 gSwordGaugeHitFactors[];
extern s16 gSharpnessLossByHit[];
extern s16 gHitSeIds[];

/* A hit by object o (of player p) on monster t at out, hit shape elist of list: the reaction of p
 * (by the motion value against the hit zone), the sounds, the damage and the element. */
void ObjAttach_DamageEm(ObjAttach *attach, Player *attacker, Em *em, HitShape *list, HitShape *elist, f32 *out)
{
    f32 motionValue;
    f32 atk;
    f32 elemFactor;
    f32 fire;
    f32 thunder;
    f32 ice;
    f32 water;
    f32 dragon;
    f32 cutRate;
    f32 impactRate;
    f32 pct;
    f32 powerF;
    s16 damage;
    s16 extraSe;
    u8 mvKind;
    u8 shellOpen;
    u8 shellHit;
    u8 deflected;
    EmHitzone *zoneRow;
    u8 byPlayer;
    u8 attackId;
    u8 crit;
    u8 oldAttrs;
    u8 noBounce;

    byPlayer = 0;
    deflected = 0;
    shellOpen = 0;
    attach->hitKind = 0;
    motionValue = 1.0f;
    elemFactor = 1.0f;
    atk = 1.0f;
    fire = 0.0f;
    crit = 0;
    zoneRow = gEmHitzoneTables[em->type][(s8)em->hitzoneSet];
    zoneRow += elist->zone;
    thunder = 0.0f;
    water = 0.0f;
    ice = 0.0f;
    dragon = 0.0f;
    attackId = attach->hitId;
    if (!(attach->multiHit & 0xF)) {
        if (Player_TestFlags(attach, ATKF_WIND_DEFLECTS) != 0 && (ObjAttach_HitFlags(em, elist) & HIT_OPEN_WIND)) {
            switch (em->type) {
            case EM_KIND_LUNASTRA:
            case EM_KIND_TEOSTRA:
                attach->state = ATTACH_DEFLECTED_AURA;
                break;
            default:
                attach->state = ATTACH_DEFLECTED;
                break;
            }
            deflected = 1;
        } else if (Player_TestFlags(attach, ATKF_SHELL_DEFLECTS) != 0 && (ObjAttach_HitFlags(em, elist) & HIT_OPEN_HERMIT)) {
            attach->state = ATTACH_DEFLECTED;
            deflected = 1;
        } else {
            attach->state = ATTACH_HIT;
        }
        attach->step = 0;
        attach->didHit = 1;
    } else {
        ObjAttach_NextMultiHit(attach);
    }
    attach->targetEm = em;
    attach->targetPart = elist;
    if (attach->ownerIndex != ATTACH_OWNER_NONE && attacker != NULL) {
        ObjAttach_LinkTarget(attach, attacker);
        attacker->hitPending = elist->zone + 1;
        if (Player_TestFlags(attach, ATKF_OWNER_CONTACT) != 0) {
            attacker->hitStopAdd = attach->hitStop;
            attacker->contact = em;
            attacker->contactAttach = attach;
        }
        if (Player_TestFlags(attach, ATKF_REPORT_CONTACT) != 0) {
            attacker->hitStopAdd = attach->hitStop;
            attacker->contact = em;
            attacker->contactAttach = attach;
            attacker->contactHit = 1;
        }
        if (ObjAttach_HitFlags(em, elist) & HIT_OPEN_HERMIT) {
            attacker->hitPending |= 0x80;
            shellOpen = 1;
            /* (f32 *)&out, as the game: the address of the pointer out, not the hit point */
            Em_PlaySeChecked(em, 45, 0, (f32 *)&out, gSeVolumeByType[em->type], 0);
        }
        if (attacker->isEm != 0) {
            Sound_PlayPlayerSe(em, 96, 0, out, 1, 0);
        } else {
            byPlayer = 1;
            if (attach->elementFlags & ELEMF_FIRE) {
                fire = attach->elementPower;
            }
            if (attach->elementFlags & ELEMF_WATER) {
                water = attach->elementPower;
            }
            if (attach->elementFlags & ELEMF_THUNDER) {
                thunder = attach->elementPower;
            }
            if (attach->elementFlags & ELEMF_ICE) {
                ice = attach->elementPower;
            }
            if (attach->elementFlags & ELEMF_DRAGON) {
                dragon = attach->elementPower;
            }
            if (Player_TestFlags(attach, ATKF_WEAPON) != 0) {
                if (attach->hitMarks & 1) {
                    crit = 1;
                }
                if (attach->mvFramesA == 255) {
                    mvKind = 0;
                } else if (attach->mvFramesA != 0) {
                    mvKind = 1;
                } else if (attach->mvFramesB != 0) {
                    if (list->zone != 1) {
                        mvKind = 2;
                    } else {
                        mvKind = 3;
                    }
                } else {
                    mvKind = 0;
                }
                motionValue = gHitKindMvFactors[mvKind + attacker->sharpnessLevel * 4];
                atk = attacker->attackFactor;
                if (attacker->weaponType == WEAPON_BOW) {
                    elemFactor = 1.0f;
                } else {
                    motionValue *= gSharpnessRawMods[attacker->sharpnessLevel];
                    elemFactor = gSharpnessElemMods[attacker->sharpnessLevel];
                }
                cutRate = attacker->attackPercent / 100.0f;
                impactRate = attacker->attackPercentBase / 100.0f;
                cutRate *= zoneRow->cut / 100.0f;
                impactRate *= zoneRow->impact / 100.0f;
                if (!(cutRate < impactRate)) {
                    motionValue *= cutRate;
                } else {
                    motionValue *= impactRate;
                }
                shellHit = shellOpen;
                if (shellHit != 0 && !(motionValue <= 0.1f)) {
                    motionValue = 0.1f;
                }
                if (!(motionValue < 1.0f)) {
                    attacker->hitKind = 3;
                    attacker->hitStopAdd = attach->hitStop * 3;
                } else if (!(motionValue < 0.45f)) {
                    attacker->hitKind = 2;
                    attacker->hitStopAdd = 0;
                    attacker->hitSlowTimer = attach->hitStop;
                } else {
                    if (!(motionValue < 0.25f)) {
                        attacker->hitKind = 0;
                    } else {
                        attacker->hitKind = 1;
                    }
                    attacker->hitStopAdd = 0;
                }
                noBounce = 0;
                attach->hitKind = attacker->hitKind;
                if (Player_HasSongEffect(attacker, 0) == 1 && attacker->songEffect[0] >= 2) {
                    noBounce = 1;
                }
                if (Player_HasSkill(attacker, SKILL_AFFINITY_PLUS10_NO_BOUNCE) != 0 || Player_HasSkill(attacker, SKILL_AFFINITY_PLUS15_NO_BOUNCE) != 0) {
                    noBounce = 1;
                }
                if (noBounce != 0 && attacker->hitKind == 1) {
                    attacker->hitKind = 0;
                }
                if (attacker != NULL) {
                    if (attach->swordGaugeAdd > 0) {
                        LongSword_AddGauge(attacker, attach->swordGaugeAdd * gSwordGaugeHitFactors[attacker->hitKind]);
                    }
                }
                if (attacker->weaponType != WEAPON_BOW) {
                    if (shellHit != 0) {
                        Player_AddSharpness(attacker, gSharpnessLossByHit[attacker->hitKind] * 5);
                    } else {
                        Player_AddSharpness(attacker, gSharpnessLossByHit[attacker->hitKind]);
                    }
                }
                switch (attach->hitSe) {
                case 255:
                    break;
                case 0:
                default:
                    Sound_PlayPlayerSe(em, gHitSeIds[attacker->weaponType * 8 + attacker->hitKind * 2], 0, out, 1, 0);
                    extraSe = gHitSeIds[attacker->weaponType * 8 + attacker->hitKind * 2 + 1];
                    if (extraSe != 0) {
                        Sound_PlayPlayerSe(em, extraSe, 0, out, 1, 0);
                    }
                    break;
                case 10:
                    Sound_PlayPlayerSe(em, 130, 0, out, 1, 0);
                    break;
                case 1:
                    Sound_PlayPlayerSe(em, 32, 0, out, 1, 0);
                    break;
                }
            } else {
                motionValue = 1.0f;
                if (Player_TestFlags(attach, ATKF_SHOT) != 0) {
                    motionValue *= zoneRow->shot / 100.0f;
                    if (Debug_IsOn(174) != 0) {
                        if (attach->hitFx & HITFX_CRITICAL) {
                            printf(TXT(0x004AB080) /* debug message: a ranged attack hit (critical) */);
                        } else if (attach->hitFx & HITFX_WEAK) {
                            printf(TXT(0x004AB0B0) /* debug message: a ranged attack hit (critical failed) */);
                        } else {
                            printf(TXT(0x004AB0F0) /* debug message: a ranged attack hit */);
                        }
                        printf(TXT(0x004AB110) /* debug message: the player's attack modifier */, fptodp(1.0f));
                        printf(TXT(0x004AB130) /* debug message: the hit zone's result against a shot */, fptodp(motionValue));
                    }
                } else if (Debug_IsOn(174) != 0) {
                    printf(TXT(0x004AB150) /* debug message: another player attack hit */);
                }
                switch (attach->hitSe) {
                case 255:
                    break;
                case 2:
                case 3:
                    Sound_PlayPlayerSe(em, 93, 0, out, 1, 0);
                    break;
                case 4:
                    Sound_PlayPlayerSe(em, 92, 0, out, 1, 0);
                    break;
                case 5:
                    break;
                case 6:
                    Sound_PlayPlayerSe(em, 77, 0, out, 1, 0);
                    break;
                case 1:
                    Sound_PlayPlayerSe(em, 32, 0, out, 1, 0);
                    break;
                case 7:
                    /* the sword and shield's sound for hit kind 3 (gHitSeIds' row: weaponType * 8 + hitKind * 2) */
                    Sound_PlayPlayerSe(em, gHitSeIds[38], 0, out, 1, 0);
                    break;
                case 8:
                    Sound_PlayPlayerSe(em, 128, 0, out, 1, 0);
                    break;
                case 9:
                    Sound_PlayPlayerSe(em, 129, 0, out, 1, 0);
                    break;
                case 10:
                    Sound_PlayPlayerSe(em, 130, 0, out, 1, 0);
                    break;
                case 11:
                    Sound_PlayPlayerSe(em, 131, 0, out, 1, 0);
                    break;
                case 12:
                    Sound_PlayPlayerSe(em, 132, 0, out, 1, 0);
                    break;
                }
            }
            fire *= zoneRow->fire / 100.0f;
            water *= zoneRow->water / 100.0f;
            thunder *= zoneRow->thunder / 100.0f;
            ice *= zoneRow->ice / 100.0f;
            dragon *= zoneRow->dragon / 100.0f;
            fire = elemFactor * (fire + water + thunder + ice + dragon);
            Vib_StartLocal(attacker, 0);
        }
    }
    if (attach->attackKind == ATTACK_PAINT && em->painted != 255 && (u8)ObjAttach_TargetIsRemote(em) == 0 && deflected == 0) {
        em->painted = 1;
        em->paintTimer = EM_PAINT_FRAMES;
    }
    em->hitIds[em->hitIdNext] = attackId;
    em->hitIdNext = (em->hitIdNext + 1) & 0xF;
    if ((u8)ObjAttach_TargetIsRemote(em) == 0 && Side_Match(attach->side, ((ObjHead *)em)->side, 1) != 1) { /* the unit read the side as the shared u8 (Em.side is s8) */
        powerF = attach->power;
        if (attach->powerPct > 0) {
            pct = attach->powerPct;
            pct /= 100.0f;
            powerF *= pct;
        }
        switch (em->hitReactMode) {
        case EM_HIT_REACT_WAKE:
            if ((s8)em->guardTimer <= 0 && byPlayer == 0) {
                powerF *= 3.0f;
            }
            break;
        case EM_HIT_REACT_NO_FLINCH:
            powerF *= 1.1f;
            break;
        case EM_HIT_REACT_STATUS:
            powerF *= 1.1f;
            break;
        case EM_HIT_REACT_FLINCH:
        case 1:
            break;
        }
        if (attach->attackKind == ATTACK_HEAL) {
            damage = -1.0f * attach->power;
        } else {
            damage = atk * (powerF * motionValue);
        }
        if (attach->isEm != 0 && em->type != 29) {
            damage /= 4;
        }
        if (damage == 0 && attach->power != 0) {
            damage = 1;
        }
        damage += (s16)fire;
        if (deflected == 0) {
            if (crit != 0) {
                em->partFlags[elist->part & 7] |= 1;
            }
            if (attach->hitMarks & 2) {
                em->partFlags[elist->part & 7] |= 2;
            }
            if (attach->hitMarks & 8) {
                em->partFlags[elist->part & 7] |= 8;
            }
            if (attach->elementFlags & ELEMF_FIRE) {
                em->partFlags[elist->part & 7] |= 4;
            }
            if (attach->elementFlags & ELEMF_WATER) {
                em->partFlags[elist->part & 7] |= 0x10;
            }
            if (attach->elementFlags & ELEMF_ICE) {
                em->partFlags[elist->part & 7] |= 0x20;
            }
            if (attach->elementFlags & ELEMF_THUNDER) {
                em->partFlags[elist->part & 7] |= 0x40;
            }
            if (attach->elementFlags & ELEMF_DRAGON) {
                em->partFlags[elist->part & 7] |= 0x80;
            }
            em->hitPartDamage[elist->part & 7] += damage;
            Em_ApplyHitStatus((Player *)em, attach);
            if (attach->isEm == 0) {
                em->status5Hit += (s16)(attach->stun * (zoneRow->stun / 100.0f));
            }
            if (attach->ownerIndex != ATTACH_OWNER_NONE && attacker->isEm == 0) {
                em->reactValue[attacker->index & 3] += damage;
            }
            em->hitStopAdd = attach->hitStop;
            em->hitReceived = 2;
            em->flinchPart = elist->part;
            if (Player_TestFlags(attach, ATKF_OWNER_CONTACT) == 0) {
                Player_SetFlag(em, 0x80000001);
            } else {
                Player_ClearFlag(em, 0x80000001);
            }
            em->hitAttach = attach;
            em->lastAttackKind = attach->attackKind;
            em->hitForce = attach->hitForce;
            oldAttrs = em->hitAttrs;
            em->hitAttrs = oldAttrs | attach->attrs;
            if (attach->attackKind == ATTACK_CALM) {
                Em_EndRage(em);
            }
        }
    }
}

/* The rest of this unit's .rodata: the item names, which no code loads; equip_save.c's
 * gItemNames points to them. */
#pragma rodata on
char gItemNames_0[13] = {
#include "sub_main/004AB180.inc"
};
char gItemNames_1[15] = {
#include "sub_main/004AB190.inc"
};
char gItemNames_2[15] = {
#include "sub_main/004AB1A0.inc"
};
char gItemNames_3[15] = {
#include "sub_main/004AB1B0.inc"
};
char gItemNames_4[15] = {
#include "sub_main/004AB1C0.inc"
};
char gItemNames_5[15] = {
#include "sub_main/004AB1D0.inc"
};
char gItemNames_6[17] = {
#include "sub_main/004AB1E0.inc"
};
char gItemNames_7[7] = {
#include "sub_main/004AB1F8.inc"
};
char gItemNames_8[15] = {
#include "sub_main/004AB200.inc"
};
char gItemNames_9[7] = {
#include "sub_main/004AB210.inc"
};
char gItemNames_10[15] = {
#include "sub_main/004AB218.inc"
};
char gItemNames_11[7] = {
#include "sub_main/004AB228.inc"
};
char gItemNames_12[7] = {
#include "sub_main/004AB230.inc"
};
char gItemNames_13[7] = {
#include "sub_main/004AB238.inc"
};
char gItemNames_14[15] = {
#include "sub_main/004AB240.inc"
};
char gItemNames_15[7] = {
#include "sub_main/004AB250.inc"
};
char gItemNames_16[15] = {
#include "sub_main/004AB258.inc"
};
char gItemNames_17[11] = {
#include "sub_main/004AB268.inc"
};
char gItemNames_18[7] = {
#include "sub_main/004AB278.inc"
};
char gItemNames_19[15] = {
#include "sub_main/004AB280.inc"
};
char gItemNames_20[11] = {
#include "sub_main/004AB290.inc"
};
char gItemNames_21[17] = {
#include "sub_main/004AB2A0.inc"
};
char gItemNames_22[15] = {
#include "sub_main/004AB2B8.inc"
};
char gItemNames_23[11] = {
#include "sub_main/004AB2C8.inc"
};
char gItemNames_24[17] = {
#include "sub_main/004AB2E0.inc"
};
char gItemNames_25[17] = {
#include "sub_main/004AB300.inc"
};
char gItemNames_26[7] = {
#include "sub_main/004AB318.inc"
};
char gItemNames_27[5] = {
#include "sub_main/004AB320.inc"
};
char gItemNames_28[15] = {
#include "sub_main/004AB328.inc"
};
char gItemNames_29[7] = {
#include "sub_main/004AB338.inc"
};
char gItemNames_30[13] = {
#include "sub_main/004AB340.inc"
};
char D_004AB350[11] = {
#include "sub_main/004AB350.inc"
};
char gItemNames_32[5] = {
#include "sub_main/004AB360.inc"
};
char gItemNames_33[9] = {
#include "sub_main/004AB368.inc"
};
char gItemNames_34[11] = {
#include "sub_main/004AB378.inc"
};
char gItemNames_35[9] = {
#include "sub_main/004AB388.inc"
};
char gItemNames_36[7] = {
#include "sub_main/004AB398.inc"
};
char gItemNames_37[11] = {
#include "sub_main/004AB3A0.inc"
};
char gItemNames_38[9] = {
#include "sub_main/004AB3B0.inc"
};
char gItemNames_39[13] = {
#include "sub_main/004AB3C0.inc"
};
char gItemNames_40[5] = {
#include "sub_main/004AB3D0.inc"
};
char gItemNames_41[7] = {
#include "sub_main/004AB3D8.inc"
};
char gItemNames_42[15] = {
#include "sub_main/004AB3E0.inc"
};
char gItemNames_43[11] = {
#include "sub_main/004AB3F0.inc"
};
char gItemNames_44[7] = {
#include "sub_main/004AB400.inc"
};
char gItemNames_45[11] = {
#include "sub_main/004AB408.inc"
};
char gItemNames_46[9] = {
#include "sub_main/004AB418.inc"
};
char gItemNames_47[9] = {
#include "sub_main/004AB428.inc"
};
char gItemNames_48[11] = {
#include "sub_main/004AB438.inc"
};
char gItemNames_49[13] = {
#include "sub_main/004AB448.inc"
};
char gItemNames_50[7] = {
#include "sub_main/004AB458.inc"
};
char gItemNames_51[13] = {
#include "sub_main/004AB460.inc"
};
char gItemNames_52[15] = {
#include "sub_main/004AB470.inc"
};
char gItemNames_53[15] = {
#include "sub_main/004AB480.inc"
};
char gItemNames_54[17] = {
#include "sub_main/004AB490.inc"
};
char gItemNames_55[11] = {
#include "sub_main/004AB4A8.inc"
};
char gItemNames_56[13] = {
#include "sub_main/004AB4B8.inc"
};
char gItemNames_57[9] = {
#include "sub_main/004AB4C8.inc"
};
char gItemNames_58[11] = {
#include "sub_main/004AB4D8.inc"
};
char gItemNames_59[15] = {
#include "sub_main/004AB4E8.inc"
};
char gItemNames_60[13] = {
#include "sub_main/004AB4F8.inc"
};
char gItemNames_61[17] = {
#include "sub_main/004AB510.inc"
};
char gItemNames_62[7] = {
#include "sub_main/004AB528.inc"
};
char gItemNames_64[11] = {
#include "sub_main/004AB530.inc"
};
char gItemNames_65[11] = {
#include "sub_main/004AB540.inc"
};
char gItemNames_66[13] = {
#include "sub_main/004AB550.inc"
};
char gItemNames_67[15] = {
#include "sub_main/004AB560.inc"
};
char gItemNames_68[15] = {
#include "sub_main/004AB570.inc"
};
char gItemNames_69[17] = {
#include "sub_main/004AB580.inc"
};
char gItemNames_70[7] = {
#include "sub_main/004AB598.inc"
};
char gItemNames_71[15] = {
#include "sub_main/004AB5A0.inc"
};
char gItemNames_72[7] = {
#include "sub_main/004AB5B0.inc"
};
char gItemNames_73[7] = {
#include "sub_main/004AB5B8.inc"
};
char gItemNames_74[17] = {
#include "sub_main/004AB5C0.inc"
};
char gItemNames_75[9] = {
#include "sub_main/004AB5D8.inc"
};
char gItemNames_76[9] = {
#include "sub_main/004AB5E8.inc"
};
char gItemNames_77[11] = {
#include "sub_main/004AB5F8.inc"
};
char gItemNames_78[9] = {
#include "sub_main/004AB608.inc"
};
char gItemNames_79[7] = {
#include "sub_main/004AB618.inc"
};
char gItemNames_80[13] = {
#include "sub_main/004AB620.inc"
};
char gItemNames_81[9] = {
#include "sub_main/004AB630.inc"
};
char gItemNames_82[7] = {
#include "sub_main/004AB640.inc"
};
char gItemNames_83[15] = {
#include "sub_main/004AB648.inc"
};
char gItemNames_84[9] = {
#include "sub_main/004AB658.inc"
};
char gItemNames_85[9] = {
#include "sub_main/004AB668.inc"
};
char gItemNames_86[7] = {
#include "sub_main/004AB678.inc"
};
char gItemNames_88[9] = {
#include "sub_main/004AB680.inc"
};
char gItemNames_89[7] = {
#include "sub_main/004AB690.inc"
};
char gItemNames_90[7] = {
#include "sub_main/004AB698.inc"
};
char gItemNames_91[11] = {
#include "sub_main/004AB6A0.inc"
};
char gItemNames_92[13] = {
#include "sub_main/004AB6B0.inc"
};
char gItemNames_93[11] = {
#include "sub_main/004AB6C0.inc"
};
char gItemNames_94[13] = {
#include "sub_main/004AB6D0.inc"
};
char gItemNames_95[15] = {
#include "sub_main/004AB6E0.inc"
};
char gItemNames_96[17] = {
#include "sub_main/004AB6F0.inc"
};
char gItemNames_97[11] = {
#include "sub_main/004AB708.inc"
};
char gItemNames_98[13] = {
#include "sub_main/004AB718.inc"
};
char gItemNames_99[9] = {
#include "sub_main/004AB728.inc"
};
char gItemNames_100[13] = {
#include "sub_main/004AB738.inc"
};
char gItemNames_101[13] = {
#include "sub_main/004AB748.inc"
};
char gItemNames_102[11] = {
#include "sub_main/004AB758.inc"
};
char gItemNames_103[11] = {
#include "sub_main/004AB768.inc"
};
char gItemNames_104[11] = {
#include "sub_main/004AB778.inc"
};
char gItemNames_105[11] = {
#include "sub_main/004AB788.inc"
};
char gItemNames_106[11] = {
#include "sub_main/004AB798.inc"
};
char gItemNames_107[11] = {
#include "sub_main/004AB7A8.inc"
};
char gItemNames_108[9] = {
#include "sub_main/004AB7B8.inc"
};
char gItemNames_109[9] = {
#include "sub_main/004AB7C8.inc"
};
char gItemNames_110[9] = {
#include "sub_main/004AB7D8.inc"
};
char gItemNames_111[13] = {
#include "sub_main/004AB7E8.inc"
};
char gItemNames_112[13] = {
#include "sub_main/004AB7F8.inc"
};
char gItemNames_113[13] = {
#include "sub_main/004AB808.inc"
};
char gItemNames_114[11] = {
#include "sub_main/004AB818.inc"
};
char gItemNames_115[11] = {
#include "sub_main/004AB828.inc"
};
char gItemNames_116[11] = {
#include "sub_main/004AB838.inc"
};
char gItemNames_117[7] = {
#include "sub_main/004AB848.inc"
};
char gItemNames_118[7] = {
#include "sub_main/004AB850.inc"
};
char gItemNames_119[7] = {
#include "sub_main/004AB858.inc"
};
char gItemNames_120[7] = {
#include "sub_main/004AB860.inc"
};
char gItemNames_121[7] = {
#include "sub_main/004AB868.inc"
};
char gItemNames_122[11] = {
#include "sub_main/004AB870.inc"
};
char gItemNames_123[11] = {
#include "sub_main/004AB880.inc"
};
char gItemNames_124[9] = {
#include "sub_main/004AB890.inc"
};
char gItemNames_125[9] = {
#include "sub_main/004AB8A0.inc"
};
char gItemNames_126[11] = {
#include "sub_main/004AB8B0.inc"
};
char gItemNames_127[11] = {
#include "sub_main/004AB8C0.inc"
};
char gItemNames_128[11] = {
#include "sub_main/004AB8D0.inc"
};
char gItemNames_129[11] = {
#include "sub_main/004AB8E0.inc"
};
char gItemNames_130[13] = {
#include "sub_main/004AB8F0.inc"
};
char gItemNames_131[11] = {
#include "sub_main/004AB900.inc"
};
char gItemNames_132[7] = {
#include "sub_main/004AB910.inc"
};
char gItemNames_133[7] = {
#include "sub_main/004AB918.inc"
};
char gItemNames_135[9] = {
#include "sub_main/004AB920.inc"
};
char gItemNames_136[9] = {
#include "sub_main/004AB930.inc"
};
char gItemNames_137[9] = {
#include "sub_main/004AB940.inc"
};
char gItemNames_138[7] = {
#include "sub_main/004AB950.inc"
};
char gItemNames_139[9] = {
#include "sub_main/004AB958.inc"
};
char gItemNames_140[9] = {
#include "sub_main/004AB968.inc"
};
char gItemNames_142[11] = {
#include "sub_main/004AB978.inc"
};
char gItemNames_143[11] = {
#include "sub_main/004AB988.inc"
};
char gItemNames_144[11] = {
#include "sub_main/004AB998.inc"
};
char gItemNames_145[11] = {
#include "sub_main/004AB9A8.inc"
};
char gItemNames_146[11] = {
#include "sub_main/004AB9B8.inc"
};
char gItemNames_147[11] = {
#include "sub_main/004AB9C8.inc"
};
char gItemNames_148[11] = {
#include "sub_main/004AB9D8.inc"
};
char gItemNames_149[15] = {
#include "sub_main/004AB9E8.inc"
};
char gItemNames_150[13] = {
#include "sub_main/004AB9F8.inc"
};
char gItemNames_151[9] = {
#include "sub_main/004ABA08.inc"
};
char gItemNames_152[17] = {
#include "sub_main/004ABA20.inc"
};
char gItemNames_153[11] = {
#include "sub_main/004ABA38.inc"
};
char gItemNames_154[7] = {
#include "sub_main/004ABA48.inc"
};
char gItemNames_155[15] = {
#include "sub_main/004ABA50.inc"
};
char gItemNames_156[13] = {
#include "sub_main/004ABA60.inc"
};
char gItemNames_157[17] = {
#include "sub_main/004ABA70.inc"
};
char gItemNames_158[17] = {
#include "sub_main/004ABA90.inc"
};
char gItemNames_159[17] = {
#include "sub_main/004ABAB0.inc"
};
char gItemNames_160[17] = {
#include "sub_main/004ABAD0.inc"
};
char gItemNames_161[17] = {
#include "sub_main/004ABAF0.inc"
};
char gItemNames_162[17] = {
#include "sub_main/004ABB10.inc"
};
char D_004ABB28[15] = {
#include "sub_main/004ABB28.inc"
};
char D_004ABB38[15] = {
#include "sub_main/004ABB38.inc"
};
char gItemNames_172[5] = {
#include "sub_main/004ABB48.inc"
};
char gItemNames_173[7] = {
#include "sub_main/004ABB50.inc"
};
char gItemNames_174[13] = {
#include "sub_main/004ABB58.inc"
};
char gItemNames_175[5] = {
#include "sub_main/004ABB68.inc"
};
char gItemNames_176[7] = {
#include "sub_main/004ABB70.inc"
};
char gItemNames_177[7] = {
#include "sub_main/004ABB78.inc"
};
char gItemNames_178[7] = {
#include "sub_main/004ABB80.inc"
};
char gItemNames_179[7] = {
#include "sub_main/004ABB88.inc"
};
char gItemNames_180[9] = {
#include "sub_main/004ABB90.inc"
};
char gItemNames_181[7] = {
#include "sub_main/004ABBA0.inc"
};
char gItemNames_182[7] = {
#include "sub_main/004ABBA8.inc"
};
char gItemNames_183[5] = {
#include "sub_main/004ABBB0.inc"
};
char gItemNames_184[9] = {
#include "sub_main/004ABBB8.inc"
};
char gItemNames_185[7] = {
#include "sub_main/004ABBC8.inc"
};
char gItemNames_186[9] = {
#include "sub_main/004ABBD0.inc"
};
char gItemNames_187[9] = {
#include "sub_main/004ABBE0.inc"
};
char gItemNames_188[13] = {
#include "sub_main/004ABBF0.inc"
};
char gItemNames_189[9] = {
#include "sub_main/004ABC00.inc"
};
char gItemNames_190[13] = {
#include "sub_main/004ABC10.inc"
};
char gItemNames_191[13] = {
#include "sub_main/004ABC20.inc"
};
char gItemNames_192[11] = {
#include "sub_main/004ABC30.inc"
};
char gItemNames_193[11] = {
#include "sub_main/004ABC40.inc"
};
char gItemNames_194[11] = {
#include "sub_main/004ABC50.inc"
};
char gItemNames_195[11] = {
#include "sub_main/004ABC60.inc"
};
char gItemNames_196[11] = {
#include "sub_main/004ABC70.inc"
};
char gItemNames_197[11] = {
#include "sub_main/004ABC80.inc"
};
char gItemNames_198[9] = {
#include "sub_main/004ABC90.inc"
};
char gItemNames_199[13] = {
#include "sub_main/004ABCA0.inc"
};
char gItemNames_200[13] = {
#include "sub_main/004ABCB0.inc"
};
char gItemNames_201[13] = {
#include "sub_main/004ABCC0.inc"
};
char gItemNames_202[17] = {
#include "sub_main/004ABCD0.inc"
};
char gItemNames_203[17] = {
#include "sub_main/004ABCF0.inc"
};
char gItemNames_204[13] = {
#include "sub_main/004ABD08.inc"
};
char gItemNames_205[13] = {
#include "sub_main/004ABD18.inc"
};
char gItemNames_206[9] = {
#include "sub_main/004ABD28.inc"
};
char gItemNames_207[9] = {
#include "sub_main/004ABD38.inc"
};
char gItemNames_208[11] = {
#include "sub_main/004ABD48.inc"
};
char gItemNames_209[13] = {
#include "sub_main/004ABD58.inc"
};
char gItemNames_210[9] = {
#include "sub_main/004ABD68.inc"
};
char gItemNames_211[13] = {
#include "sub_main/004ABD78.inc"
};
char gItemNames_212[11] = {
#include "sub_main/004ABD88.inc"
};
char gItemNames_213[7] = {
#include "sub_main/004ABD98.inc"
};
char gItemNames_214[7] = {
#include "sub_main/004ABDA0.inc"
};
char gItemNames_215[7] = {
#include "sub_main/004ABDA8.inc"
};
char gItemNames_216[7] = {
#include "sub_main/004ABDB0.inc"
};
char gItemNames_217[11] = {
#include "sub_main/004ABDB8.inc"
};
char gItemNames_218[15] = {
#include "sub_main/004ABDC8.inc"
};
char gItemNames_219[17] = {
#include "sub_main/004ABDE0.inc"
};
char gItemNames_220[17] = {
#include "sub_main/004ABE00.inc"
};
char gItemNames_221[13] = {
#include "sub_main/004ABE18.inc"
};
char gItemNames_222[7] = {
#include "sub_main/004ABE28.inc"
};
char gItemNames_224[7] = {
#include "sub_main/004ABE30.inc"
};
char gItemNames_225[17] = {
#include "sub_main/004ABE40.inc"
};
char gItemNames_226[17] = {
#include "sub_main/004ABE60.inc"
};
char gItemNames_227[7] = {
#include "sub_main/004ABE78.inc"
};
char gItemNames_228[7] = {
#include "sub_main/004ABE80.inc"
};
char gItemNames_230[13] = {
#include "sub_main/004ABE88.inc"
};
char gItemNames_231[13] = {
#include "sub_main/004ABE98.inc"
};
char gItemNames_232[13] = {
#include "sub_main/004ABEA8.inc"
};
char gItemNames_233[13] = {
#include "sub_main/004ABEB8.inc"
};
char gItemNames_234[9] = {
#include "sub_main/004ABEC8.inc"
};
char gItemNames_235[11] = {
#include "sub_main/004ABED8.inc"
};
char gItemNames_236[5] = {
#include "sub_main/004ABEE8.inc"
};
char gItemNames_237[11] = {
#include "sub_main/004ABEF0.inc"
};
char gItemNames_238[13] = {
#include "sub_main/004ABF00.inc"
};
char gItemNames_239[17] = {
#include "sub_main/004ABF10.inc"
};
char gItemNames_240[15] = {
#include "sub_main/004ABF28.inc"
};
char gItemNames_241[17] = {
#include "sub_main/004ABF40.inc"
};
char gItemNames_242[13] = {
#include "sub_main/004ABF58.inc"
};
char gItemNames_243[9] = {
#include "sub_main/004ABF68.inc"
};
char gItemNames_244[15] = {
#include "sub_main/004ABF78.inc"
};
char gItemNames_245[17] = {
#include "sub_main/004ABF90.inc"
};
char gItemNames_246[13] = {
#include "sub_main/004ABFA8.inc"
};
char gItemNames_247[13] = {
#include "sub_main/004ABFB8.inc"
};
char gItemNames_248[17] = {
#include "sub_main/004ABFD0.inc"
};
char gItemNames_249[7] = {
#include "sub_main/004ABFE8.inc"
};
char gItemNames_250[13] = {
#include "sub_main/004ABFF0.inc"
};
char gItemNames_251[7] = {
#include "sub_main/004AC000.inc"
};
char gItemNames_252[7] = {
#include "sub_main/004AC008.inc"
};
char gItemNames_253[7] = {
#include "sub_main/004AC010.inc"
};
char gItemNames_254[9] = {
#include "sub_main/004AC018.inc"
};
char gItemNames_255[9] = {
#include "sub_main/004AC028.inc"
};
char gItemNames_256[13] = {
#include "sub_main/004AC038.inc"
};
char gItemNames_257[11] = {
#include "sub_main/004AC048.inc"
};
char gItemNames_258[13] = {
#include "sub_main/004AC058.inc"
};
char gItemNames_259[7] = {
#include "sub_main/004AC068.inc"
};
char gItemNames_260[7] = {
#include "sub_main/004AC070.inc"
};
char gItemNames_261[5] = {
#include "sub_main/004AC078.inc"
};
char gItemNames_262[7] = {
#include "sub_main/004AC080.inc"
};
char gItemNames_263[13] = {
#include "sub_main/004AC088.inc"
};
char gItemNames_264[15] = {
#include "sub_main/004AC098.inc"
};
char gItemNames_266[13] = {
#include "sub_main/004AC0A8.inc"
};
char gItemNames_267[11] = {
#include "sub_main/004AC0B8.inc"
};
char gItemNames_268[15] = {
#include "sub_main/004AC0C8.inc"
};
char gItemNames_269[15] = {
#include "sub_main/004AC0D8.inc"
};
char gItemNames_270[17] = {
#include "sub_main/004AC0F0.inc"
};
char gItemNames_271[13] = {
#include "sub_main/004AC108.inc"
};
char gItemNames_272[15] = {
#include "sub_main/004AC118.inc"
};
char gItemNames_273[13] = {
#include "sub_main/004AC128.inc"
};
char gItemNames_274[13] = {
#include "sub_main/004AC138.inc"
};
char gItemNames_275[17] = {
#include "sub_main/004AC150.inc"
};
char gItemNames_276[9] = {
#include "sub_main/004AC168.inc"
};
char gItemNames_277[11] = {
#include "sub_main/004AC178.inc"
};
char gItemNames_278[7] = {
#include "sub_main/004AC188.inc"
};
char gItemNames_279[15] = {
#include "sub_main/004AC190.inc"
};
char gItemNames_280[11] = {
#include "sub_main/004AC1A0.inc"
};
char gItemNames_281[13] = {
#include "sub_main/004AC1B0.inc"
};
char gItemNames_282[15] = {
#include "sub_main/004AC1C0.inc"
};
char gItemNames_283[15] = {
#include "sub_main/004AC1D0.inc"
};
char gItemNames_284[11] = {
#include "sub_main/004AC1E0.inc"
};
char gItemNames_285[17] = {
#include "sub_main/004AC1F0.inc"
};
char gItemNames_286[5] = {
#include "sub_main/004AC208.inc"
};
char gItemNames_287[15] = {
#include "sub_main/004AC210.inc"
};
char gItemNames_288[9] = {
#include "sub_main/004AC220.inc"
};
char gItemNames_289[15] = {
#include "sub_main/004AC230.inc"
};
char gItemNames_290[13] = {
#include "sub_main/004AC240.inc"
};
char gItemNames_291[13] = {
#include "sub_main/004AC250.inc"
};
char gItemNames_292[11] = {
#include "sub_main/004AC260.inc"
};
char gItemNames_293[17] = {
#include "sub_main/004AC270.inc"
};
char gItemNames_294[11] = {
#include "sub_main/004AC288.inc"
};
char gItemNames_295[15] = {
#include "sub_main/004AC298.inc"
};
char gItemNames_296[7] = {
#include "sub_main/004AC2A8.inc"
};
char gItemNames_297[13] = {
#include "sub_main/004AC2B0.inc"
};
char gItemNames_298[9] = {
#include "sub_main/004AC2C0.inc"
};
char gItemNames_299[17] = {
#include "sub_main/004AC2D0.inc"
};
char gItemNames_300[13] = {
#include "sub_main/004AC2E8.inc"
};
char gItemNames_302[7] = {
#include "sub_main/004AC2F8.inc"
};
char gItemNames_303[5] = {
#include "sub_main/004AC300.inc"
};
char gItemNames_304[7] = {
#include "sub_main/004AC308.inc"
};
char gItemNames_306[7] = {
#include "sub_main/004AC310.inc"
};
char gItemNames_308[7] = {
#include "sub_main/004AC318.inc"
};
char gItemNames_309[7] = {
#include "sub_main/004AC320.inc"
};
char gItemNames_310[11] = {
#include "sub_main/004AC328.inc"
};
char gItemNames_311[15] = {
#include "sub_main/004AC338.inc"
};
char gItemNames_312[11] = {
#include "sub_main/004AC348.inc"
};
char gItemNames_313[11] = {
#include "sub_main/004AC358.inc"
};
char gItemNames_314[11] = {
#include "sub_main/004AC368.inc"
};
char gItemNames_315[7] = {
#include "sub_main/004AC378.inc"
};
char gItemNames_316[7] = {
#include "sub_main/004AC380.inc"
};
char gItemNames_317[9] = {
#include "sub_main/004AC388.inc"
};
char gItemNames_318[11] = {
#include "sub_main/004AC398.inc"
};
char gItemNames_319[13] = {
#include "sub_main/004AC3A8.inc"
};
char gItemNames_320[9] = {
#include "sub_main/004AC3B8.inc"
};
char gItemNames_321[9] = {
#include "sub_main/004AC3C8.inc"
};
char gItemNames_322[7] = {
#include "sub_main/004AC3D8.inc"
};
char gItemNames_323[7] = {
#include "sub_main/004AC3E0.inc"
};
char gItemNames_324[5] = {
#include "sub_main/004AC3E8.inc"
};
char gItemNames_325[11] = {
#include "sub_main/004AC3F0.inc"
};
char gItemNames_326[15] = {
#include "sub_main/004AC400.inc"
};
char gItemNames_327[9] = {
#include "sub_main/004AC410.inc"
};
char gItemNames_328[7] = {
#include "sub_main/004AC420.inc"
};
char gItemNames_329[11] = {
#include "sub_main/004AC428.inc"
};
char gItemNames_330[7] = {
#include "sub_main/004AC438.inc"
};
char gItemNames_331[11] = {
#include "sub_main/004AC440.inc"
};
char gItemNames_332[9] = {
#include "sub_main/004AC450.inc"
};
char gItemNames_333[15] = {
#include "sub_main/004AC460.inc"
};
char gItemNames_334[13] = {
#include "sub_main/004AC470.inc"
};
char gItemNames_335[9] = {
#include "sub_main/004AC480.inc"
};
char gItemNames_336[13] = {
#include "sub_main/004AC490.inc"
};
char gItemNames_337[11] = {
#include "sub_main/004AC4A0.inc"
};
char gItemNames_338[11] = {
#include "sub_main/004AC4B0.inc"
};
char gItemNames_339[15] = {
#include "sub_main/004AC4C0.inc"
};
char gItemNames_340[13] = {
#include "sub_main/004AC4D0.inc"
};
char gItemNames_341[13] = {
#include "sub_main/004AC4E0.inc"
};
char gItemNames_342[13] = {
#include "sub_main/004AC4F0.inc"
};
char gItemNames_343[15] = {
#include "sub_main/004AC500.inc"
};
char gItemNames_344[13] = {
#include "sub_main/004AC510.inc"
};
char gItemNames_345[17] = {
#include "sub_main/004AC520.inc"
};
char gItemNames_346[17] = {
#include "sub_main/004AC540.inc"
};
char gItemNames_347[15] = {
#include "sub_main/004AC558.inc"
};
char gItemNames_348[17] = {
#include "sub_main/004AC570.inc"
};
char gItemNames_349[15] = {
#include "sub_main/004AC588.inc"
};
char gItemNames_350[15] = {
#include "sub_main/004AC598.inc"
};
char gItemNames_351[17] = {
#include "sub_main/004AC5B0.inc"
};
char gItemNames_352[13] = {
#include "sub_main/004AC5C8.inc"
};
char gItemNames_353[15] = {
#include "sub_main/004AC5D8.inc"
};
char gItemNames_354[13] = {
#include "sub_main/004AC5E8.inc"
};
char gItemNames_355[17] = {
#include "sub_main/004AC600.inc"
};
char gItemNames_356[17] = {
#include "sub_main/004AC620.inc"
};
char gItemNames_357[15] = {
#include "sub_main/004AC638.inc"
};
char gItemNames_358[13] = {
#include "sub_main/004AC648.inc"
};
char gItemNames_359[15] = {
#include "sub_main/004AC658.inc"
};
char gItemNames_360[13] = {
#include "sub_main/004AC668.inc"
};
char gItemNames_361[17] = {
#include "sub_main/004AC680.inc"
};
char gItemNames_362[17] = {
#include "sub_main/004AC6A0.inc"
};
char gItemNames_363[9] = {
#include "sub_main/004AC6B8.inc"
};
char gItemNames_364[11] = {
#include "sub_main/004AC6C8.inc"
};
char gItemNames_365[11] = {
#include "sub_main/004AC6D8.inc"
};
char gItemNames_366[9] = {
#include "sub_main/004AC6E8.inc"
};
char gItemNames_367[13] = {
#include "sub_main/004AC6F8.inc"
};
char gItemNames_368[15] = {
#include "sub_main/004AC708.inc"
};
char gItemNames_369[15] = {
#include "sub_main/004AC718.inc"
};
char gItemNames_370[11] = {
#include "sub_main/004AC728.inc"
};
char gItemNames_371[11] = {
#include "sub_main/004AC738.inc"
};
char gItemNames_372[13] = {
#include "sub_main/004AC748.inc"
};
char gItemNames_373[13] = {
#include "sub_main/004AC758.inc"
};
char gItemNames_374[11] = {
#include "sub_main/004AC768.inc"
};
char gItemNames_375[13] = {
#include "sub_main/004AC778.inc"
};
char gItemNames_376[11] = {
#include "sub_main/004AC788.inc"
};
char gItemNames_377[13] = {
#include "sub_main/004AC798.inc"
};
char gItemNames_378[13] = {
#include "sub_main/004AC7A8.inc"
};
char gItemNames_379[11] = {
#include "sub_main/004AC7B8.inc"
};
char gItemNames_380[9] = {
#include "sub_main/004AC7C8.inc"
};
char gItemNames_381[9] = {
#include "sub_main/004AC7D8.inc"
};
char gItemNames_382[11] = {
#include "sub_main/004AC7E8.inc"
};
char gItemNames_383[11] = {
#include "sub_main/004AC7F8.inc"
};
char gItemNames_384[11] = {
#include "sub_main/004AC808.inc"
};
char gItemNames_385[13] = {
#include "sub_main/004AC818.inc"
};
char gItemNames_386[9] = {
#include "sub_main/004AC828.inc"
};
char gItemNames_387[11] = {
#include "sub_main/004AC838.inc"
};
char gItemNames_388[11] = {
#include "sub_main/004AC848.inc"
};
char gItemNames_389[13] = {
#include "sub_main/004AC858.inc"
};
char gItemNames_390[11] = {
#include "sub_main/004AC868.inc"
};
char gItemNames_391[13] = {
#include "sub_main/004AC878.inc"
};
char gItemNames_392[11] = {
#include "sub_main/004AC888.inc"
};
char gItemNames_393[13] = {
#include "sub_main/004AC898.inc"
};
char gItemNames_394[13] = {
#include "sub_main/004AC8A8.inc"
};
char gItemNames_395[13] = {
#include "sub_main/004AC8B8.inc"
};
char gItemNames_396[13] = {
#include "sub_main/004AC8C8.inc"
};
char gItemNames_397[13] = {
#include "sub_main/004AC8D8.inc"
};
char gItemNames_398[11] = {
#include "sub_main/004AC8E8.inc"
};
char gItemNames_399[13] = {
#include "sub_main/004AC8F8.inc"
};
char gItemNames_400[11] = {
#include "sub_main/004AC908.inc"
};
char gItemNames_401[13] = {
#include "sub_main/004AC918.inc"
};
char gItemNames_402[13] = {
#include "sub_main/004AC928.inc"
};
char gItemNames_403[13] = {
#include "sub_main/004AC938.inc"
};
char gItemNames_404[13] = {
#include "sub_main/004AC948.inc"
};
char gItemNames_405[13] = {
#include "sub_main/004AC958.inc"
};
char gItemNames_406[11] = {
#include "sub_main/004AC968.inc"
};
char gItemNames_407[11] = {
#include "sub_main/004AC978.inc"
};
char gItemNames_408[9] = {
#include "sub_main/004AC988.inc"
};
char gItemNames_409[11] = {
#include "sub_main/004AC998.inc"
};
char gItemNames_410[11] = {
#include "sub_main/004AC9A8.inc"
};
char gItemNames_411[11] = {
#include "sub_main/004AC9B8.inc"
};
char gItemNames_412[11] = {
#include "sub_main/004AC9C8.inc"
};
char gItemNames_413[11] = {
#include "sub_main/004AC9D8.inc"
};
char gItemNames_414[11] = {
#include "sub_main/004AC9E8.inc"
};
char gItemNames_415[11] = {
#include "sub_main/004AC9F8.inc"
};
char gItemNames_416[9] = {
#include "sub_main/004ACA08.inc"
};
char gItemNames_417[11] = {
#include "sub_main/004ACA18.inc"
};
char gItemNames_418[13] = {
#include "sub_main/004ACA28.inc"
};
char gItemNames_419[13] = {
#include "sub_main/004ACA38.inc"
};
char gItemNames_420[13] = {
#include "sub_main/004ACA48.inc"
};
char gItemNames_421[13] = {
#include "sub_main/004ACA58.inc"
};
char gItemNames_422[11] = {
#include "sub_main/004ACA68.inc"
};
char gItemNames_423[13] = {
#include "sub_main/004ACA78.inc"
};
char gItemNames_424[13] = {
#include "sub_main/004ACA88.inc"
};
char gItemNames_425[13] = {
#include "sub_main/004ACA98.inc"
};
char gItemNames_426[13] = {
#include "sub_main/004ACAA8.inc"
};
char gItemNames_427[11] = {
#include "sub_main/004ACAB8.inc"
};
char gItemNames_428[15] = {
#include "sub_main/004ACAC8.inc"
};
char gItemNames_429[13] = {
#include "sub_main/004ACAD8.inc"
};
char gItemNames_430[17] = {
#include "sub_main/004ACAF0.inc"
};
char gItemNames_431[17] = {
#include "sub_main/004ACB10.inc"
};
char gItemNames_432[13] = {
#include "sub_main/004ACB28.inc"
};
char gItemNames_433[13] = {
#include "sub_main/004ACB38.inc"
};
char gItemNames_434[11] = {
#include "sub_main/004ACB48.inc"
};
char gItemNames_435[11] = {
#include "sub_main/004ACB58.inc"
};
char gItemNames_436[9] = {
#include "sub_main/004ACB68.inc"
};
char gItemNames_437[11] = {
#include "sub_main/004ACB78.inc"
};
char gItemNames_438[11] = {
#include "sub_main/004ACB88.inc"
};
char gItemNames_439[11] = {
#include "sub_main/004ACB98.inc"
};
char gItemNames_440[11] = {
#include "sub_main/004ACBA8.inc"
};
char gItemNames_441[13] = {
#include "sub_main/004ACBB8.inc"
};
char gItemNames_442[13] = {
#include "sub_main/004ACBC8.inc"
};
char gItemNames_443[13] = {
#include "sub_main/004ACBD8.inc"
};
char gItemNames_444[11] = {
#include "sub_main/004ACBE8.inc"
};
char gItemNames_445[17] = {
#include "sub_main/004ACC00.inc"
};
char gItemNames_446[11] = {
#include "sub_main/004ACC18.inc"
};
char gItemNames_447[11] = {
#include "sub_main/004ACC28.inc"
};
char gItemNames_448[11] = {
#include "sub_main/004ACC38.inc"
};
char gItemNames_449[11] = {
#include "sub_main/004ACC48.inc"
};
char gItemNames_450[11] = {
#include "sub_main/004ACC58.inc"
};
char gItemNames_451[9] = {
#include "sub_main/004ACC68.inc"
};
char gItemNames_452[9] = {
#include "sub_main/004ACC78.inc"
};
char gItemNames_453[13] = {
#include "sub_main/004ACC88.inc"
};
char gItemNames_454[13] = {
#include "sub_main/004ACC98.inc"
};
char gItemNames_455[13] = {
#include "sub_main/004ACCA8.inc"
};
char gItemNames_456[13] = {
#include "sub_main/004ACCB8.inc"
};
char gItemNames_457[13] = {
#include "sub_main/004ACCC8.inc"
};
char gItemNames_458[17] = {
#include "sub_main/004ACCE0.inc"
};
char gItemNames_459[17] = {
#include "sub_main/004ACD00.inc"
};
char gItemNames_460[15] = {
#include "sub_main/004ACD18.inc"
};
char gItemNames_461[17] = {
#include "sub_main/004ACD30.inc"
};
char gItemNames_462[17] = {
#include "sub_main/004ACD50.inc"
};
char gItemNames_463[15] = {
#include "sub_main/004ACD68.inc"
};
char gItemNames_464[15] = {
#include "sub_main/004ACD78.inc"
};
char gItemNames_465[17] = {
#include "sub_main/004ACD90.inc"
};
char gItemNames_466[11] = {
#include "sub_main/004ACDA8.inc"
};
char gItemNames_467[11] = {
#include "sub_main/004ACDB8.inc"
};
char gItemNames_468[13] = {
#include "sub_main/004ACDC8.inc"
};
char gItemNames_469[15] = {
#include "sub_main/004ACDD8.inc"
};
char gItemNames_470[9] = {
#include "sub_main/004ACDE8.inc"
};
char gItemNames_471[13] = {
#include "sub_main/004ACDF8.inc"
};
char gItemNames_472[13] = {
#include "sub_main/004ACE08.inc"
};
char gItemNames_473[17] = {
#include "sub_main/004ACE20.inc"
};
char gItemNames_474[9] = {
#include "sub_main/004ACE38.inc"
};
char gItemNames_475[15] = {
#include "sub_main/004ACE48.inc"
};
char gItemNames_476[15] = {
#include "sub_main/004ACE58.inc"
};
char gItemNames_477[13] = {
#include "sub_main/004ACE68.inc"
};
char gItemNames_478[11] = {
#include "sub_main/004ACE78.inc"
};
char gItemNames_479[13] = {
#include "sub_main/004ACE88.inc"
};
char gItemNames_480[17] = {
#include "sub_main/004ACEA0.inc"
};
char gItemNames_481[13] = {
#include "sub_main/004ACEB8.inc"
};
char gItemNames_482[13] = {
#include "sub_main/004ACEC8.inc"
};
char gItemNames_483[13] = {
#include "sub_main/004ACED8.inc"
};
char gItemNames_484[11] = {
#include "sub_main/004ACEE8.inc"
};
char gItemNames_485[11] = {
#include "sub_main/004ACEF8.inc"
};
char gItemNames_486[13] = {
#include "sub_main/004ACF08.inc"
};
char gItemNames_487[13] = {
#include "sub_main/004ACF18.inc"
};
char gItemNames_488[13] = {
#include "sub_main/004ACF28.inc"
};
char gItemNames_489[13] = {
#include "sub_main/004ACF38.inc"
};
char gItemNames_490[15] = {
#include "sub_main/004ACF48.inc"
};
char gItemNames_491[9] = {
#include "sub_main/004ACF58.inc"
};
char gItemNames_492[9] = {
#include "sub_main/004ACF68.inc"
};
char gItemNames_493[11] = {
#include "sub_main/004ACF78.inc"
};
char gItemNames_494[9] = {
#include "sub_main/004ACF88.inc"
};
char gItemNames_495[11] = {
#include "sub_main/004ACF98.inc"
};
char gItemNames_496[11] = {
#include "sub_main/004ACFA8.inc"
};
char gItemNames_497[11] = {
#include "sub_main/004ACFB8.inc"
};
char gItemNames_498[11] = {
#include "sub_main/004ACFC8.inc"
};
char gItemNames_499[11] = {
#include "sub_main/004ACFD8.inc"
};
char gItemNames_500[11] = {
#include "sub_main/004ACFE8.inc"
};
char gItemNames_501[11] = {
#include "sub_main/004ACFF8.inc"
};
char gItemNames_502[11] = {
#include "sub_main/004AD008.inc"
};
char gItemNames_503[11] = {
#include "sub_main/004AD018.inc"
};
char gItemNames_504[9] = {
#include "sub_main/004AD028.inc"
};
char gItemNames_507[11] = {
#include "sub_main/004AD038.inc"
};
char gItemNames_508[11] = {
#include "sub_main/004AD048.inc"
};
char gItemNames_509[13] = {
#include "sub_main/004AD058.inc"
};
char gItemNames_510[11] = {
#include "sub_main/004AD068.inc"
};
char gItemNames_511[13] = {
#include "sub_main/004AD078.inc"
};
char gItemNames_512[11] = {
#include "sub_main/004AD088.inc"
};
char gItemNames_513[13] = {
#include "sub_main/004AD098.inc"
};
char gItemNames_514[11] = {
#include "sub_main/004AD0A8.inc"
};
char gItemNames_515[11] = {
#include "sub_main/004AD0B8.inc"
};
char gItemNames_516[11] = {
#include "sub_main/004AD0C8.inc"
};
char gItemNames_517[11] = {
#include "sub_main/004AD0D8.inc"
};
char gItemNames_518[13] = {
#include "sub_main/004AD0E8.inc"
};
char gItemNames_519[13] = {
#include "sub_main/004AD0F8.inc"
};
char gItemNames_521[13] = {
#include "sub_main/004AD108.inc"
};
char gItemNames_522[13] = {
#include "sub_main/004AD118.inc"
};
char gItemNames_523[13] = {
#include "sub_main/004AD128.inc"
};
char gItemNames_524[13] = {
#include "sub_main/004AD138.inc"
};
char gItemNames_525[13] = {
#include "sub_main/004AD148.inc"
};
char gItemNames_526[9] = {
#include "sub_main/004AD158.inc"
};
char gItemNames_527[9] = {
#include "sub_main/004AD168.inc"
};
char gItemNames_528[7] = {
#include "sub_main/004AD178.inc"
};
char gItemNames_529[11] = {
#include "sub_main/004AD180.inc"
};
char gItemNames_530[11] = {
#include "sub_main/004AD190.inc"
};
char gItemNames_531[9] = {
#include "sub_main/004AD1A0.inc"
};
char gItemNames_532[11] = {
#include "sub_main/004AD1B0.inc"
};
char gItemNames_533[11] = {
#include "sub_main/004AD1C0.inc"
};
char gItemNames_535[11] = {
#include "sub_main/004AD1D0.inc"
};
char gItemNames_536[11] = {
#include "sub_main/004AD1E0.inc"
};
char gItemNames_537[11] = {
#include "sub_main/004AD1F0.inc"
};
char gItemNames_538[9] = {
#include "sub_main/004AD200.inc"
};
char gItemNames_539[9] = {
#include "sub_main/004AD210.inc"
};
char gItemNames_540[9] = {
#include "sub_main/004AD220.inc"
};
char gItemNames_541[11] = {
#include "sub_main/004AD230.inc"
};
char gItemNames_543[11] = {
#include "sub_main/004AD240.inc"
};
char gItemNames_544[11] = {
#include "sub_main/004AD250.inc"
};
char gItemNames_545[9] = {
#include "sub_main/004AD260.inc"
};
char gItemNames_546[11] = {
#include "sub_main/004AD270.inc"
};
char gItemNames_548[11] = {
#include "sub_main/004AD280.inc"
};
char gItemNames_549[11] = {
#include "sub_main/004AD290.inc"
};
char gItemNames_550[9] = {
#include "sub_main/004AD2A0.inc"
};
char gItemNames_551[11] = {
#include "sub_main/004AD2B0.inc"
};
char gItemNames_552[9] = {
#include "sub_main/004AD2C0.inc"
};
char gItemNames_553[11] = {
#include "sub_main/004AD2D0.inc"
};
char gItemNames_554[9] = {
#include "sub_main/004AD2E0.inc"
};
char gItemNames_555[11] = {
#include "sub_main/004AD2F0.inc"
};
char gItemNames_556[11] = {
#include "sub_main/004AD300.inc"
};
char gItemNames_557[11] = {
#include "sub_main/004AD310.inc"
};
char gItemNames_558[11] = {
#include "sub_main/004AD320.inc"
};
char gItemNames_559[11] = {
#include "sub_main/004AD330.inc"
};
char gItemNames_560[9] = {
#include "sub_main/004AD340.inc"
};
char gItemNames_561[11] = {
#include "sub_main/004AD350.inc"
};
char gItemNames_562[9] = {
#include "sub_main/004AD360.inc"
};
char gItemNames_563[11] = {
#include "sub_main/004AD370.inc"
};
char gItemNames_564[9] = {
#include "sub_main/004AD380.inc"
};
char gItemNames_565[11] = {
#include "sub_main/004AD390.inc"
};
char gItemNames_566[11] = {
#include "sub_main/004AD3A0.inc"
};
char gItemNames_567[11] = {
#include "sub_main/004AD3B0.inc"
};
char gItemNames_569[9] = {
#include "sub_main/004AD3C0.inc"
};
char gItemNames_570[9] = {
#include "sub_main/004AD3D0.inc"
};
char gItemNames_571[11] = {
#include "sub_main/004AD3E0.inc"
};
char gItemNames_572[11] = {
#include "sub_main/004AD3F0.inc"
};
char gItemNames_573[9] = {
#include "sub_main/004AD400.inc"
};
char gItemNames_574[11] = {
#include "sub_main/004AD410.inc"
};
char gItemNames_575[13] = {
#include "sub_main/004AD420.inc"
};
char gItemNames_576[17] = {
#include "sub_main/004AD430.inc"
};
char gItemNames_577[13] = {
#include "sub_main/004AD448.inc"
};
char gItemNames_578[13] = {
#include "sub_main/004AD458.inc"
};
char gItemNames_579[11] = {
#include "sub_main/004AD468.inc"
};
char gItemNames_581[13] = {
#include "sub_main/004AD478.inc"
};
char gItemNames_582[13] = {
#include "sub_main/004AD488.inc"
};
char gItemNames_583[17] = {
#include "sub_main/004AD4A0.inc"
};
char gItemNames_584[13] = {
#include "sub_main/004AD4B8.inc"
};
char gItemNames_585[13] = {
#include "sub_main/004AD4C8.inc"
};
char gItemNames_586[11] = {
#include "sub_main/004AD4D8.inc"
};
char gItemNames_587[13] = {
#include "sub_main/004AD4E8.inc"
};
char gItemNames_588[13] = {
#include "sub_main/004AD4F8.inc"
};
char gItemNames_589[11] = {
#include "sub_main/004AD508.inc"
};
char gItemNames_590[13] = {
#include "sub_main/004AD518.inc"
};
char gItemNames_591[5] = {
#include "sub_main/004AD528.inc"
};
char gItemNames_592[5] = {
#include "sub_main/004AD530.inc"
};
char gItemNames_594[9] = {
#include "sub_main/004AD538.inc"
};
char gItemNames_595[11] = {
#include "sub_main/004AD548.inc"
};
char gItemNames_596[11] = {
#include "sub_main/004AD558.inc"
};
char gItemNames_597[9] = {
#include "sub_main/004AD568.inc"
};
char gItemNames_598[11] = {
#include "sub_main/004AD578.inc"
};
char gItemNames_599[13] = {
#include "sub_main/004AD588.inc"
};
char gItemNames_600[11] = {
#include "sub_main/004AD598.inc"
};
char gItemNames_601[15] = {
#include "sub_main/004AD5A8.inc"
};
char gItemNames_602[11] = {
#include "sub_main/004AD5B8.inc"
};
char gItemNames_603[13] = {
#include "sub_main/004AD5C8.inc"
};
char gItemNames_604[17] = {
#include "sub_main/004AD5E0.inc"
};
char gItemNames_605[11] = {
#include "sub_main/004AD5F8.inc"
};
char gItemNames_606[13] = {
#include "sub_main/004AD608.inc"
};
char gItemNames_607[9] = {
#include "sub_main/004AD618.inc"
};
char gItemNames_608[13] = {
#include "sub_main/004AD628.inc"
};
char gItemNames_609[9] = {
#include "sub_main/004AD638.inc"
};
char gItemNames_610[11] = {
#include "sub_main/004AD648.inc"
};
char gItemNames_611[9] = {
#include "sub_main/004AD658.inc"
};
char gItemNames_612[15] = {
#include "sub_main/004AD668.inc"
};
char gItemNames_613[15] = {
#include "sub_main/004AD678.inc"
};
char gItemNames_614[15] = {
#include "sub_main/004AD688.inc"
};
char gItemNames_615[15] = {
#include "sub_main/004AD698.inc"
};
char gItemNames_616[9] = {
#include "sub_main/004AD6A8.inc"
};
char gItemNames_617[15] = {
#include "sub_main/004AD6B8.inc"
};
char gItemNames_618[15] = {
#include "sub_main/004AD6C8.inc"
};
char gItemNames_619[15] = {
#include "sub_main/004AD6D8.inc"
};
char gItemNames_620[15] = {
#include "sub_main/004AD6E8.inc"
};
char gItemNames_621[17] = {
#include "sub_main/004AD700.inc"
};
char gItemNames_622[17] = {
#include "sub_main/004AD720.inc"
};
char gItemNames_623[17] = {
#include "sub_main/004AD740.inc"
};
char gItemNames_624[17] = {
#include "sub_main/004AD760.inc"
};
char gItemNames_625[11] = {
#include "sub_main/004AD778.inc"
};
char gItemNames_626[17] = {
#include "sub_main/004AD790.inc"
};
char gItemNames_627[11] = {
#include "sub_main/004AD7A8.inc"
};
char gItemNames_628[13] = {
#include "sub_main/004AD7B8.inc"
};
char gItemNames_629[11] = {
#include "sub_main/004AD7C8.inc"
};
char gItemNames_630[11] = {
#include "sub_main/004AD7D8.inc"
};
char gItemNames_631[19] = {
#include "sub_main/004AD7F0.inc"
};
char gItemNames_632[19] = {
#include "sub_main/004AD810.inc"
};
char gItemNames_633[19] = {
#include "sub_main/004AD830.inc"
};
char gItemNames_634[15] = {
#include "sub_main/004AD848.inc"
};
char gItemNames_635[15] = {
#include "sub_main/004AD858.inc"
};
char gItemNames_636[11] = {
#include "sub_main/004AD868.inc"
};
char gItemNames_637[19] = {
#include "sub_main/004AD880.inc"
};
char gItemNames_638[19] = {
#include "sub_main/004AD8A0.inc"
};
char gItemNames_639[19] = {
#include "sub_main/004AD8C0.inc"
};
char gItemNames_640[11] = {
#include "sub_main/004AD8D8.inc"
};
char gItemNames_641[17] = {
#include "sub_main/004AD8F0.inc"
};
char gItemNames_642[15] = {
#include "sub_main/004AD908.inc"
};
char gItemNames_643[11] = {
#include "sub_main/004AD918.inc"
};
char gItemNames_644[19] = {
#include "sub_main/004AD930.inc"
};
char gItemNames_645[15] = {
#include "sub_main/004AD948.inc"
};
char gItemNames_646[21] = {
#include "sub_main/004AD960.inc"
};
char gItemNames_647[21] = {
#include "sub_main/004AD980.inc"
};
char gItemNames_648[19] = {
#include "sub_main/004AD9A0.inc"
};
char gItemNames_649[15] = {
#include "sub_main/004AD9B8.inc"
};
char gItemNames_650[17] = {
#include "sub_main/004AD9D0.inc"
};
char gItemNames_655[11] = {
#include "sub_main/004AD9E8.inc"
};
char gItemNames_656[11] = {
#include "sub_main/004AD9F8.inc"
};
char gItemNames_657[5] = {
#include "sub_main/004ADA08.inc"
};
char gItemNames_658[7] = {
#include "sub_main/004ADA10.inc"
};
char gItemNames_659[9] = {
#include "sub_main/004ADA18.inc"
};
char gItemNames_660[17] = {
#include "sub_main/004ADA30.inc"
};
char gItemNames_661[9] = {
#include "sub_main/004ADA48.inc"
};
char gItemNames_662[13] = {
#include "sub_main/004ADA58.inc"
};
char gItemNames_663[15] = {
#include "sub_main/004ADA68.inc"
};
char gItemNames_664[13] = {
#include "sub_main/004ADA78.inc"
};
char gItemNames_665[17] = {
#include "sub_main/004ADA90.inc"
};
char gItemNames_666[17] = {
#include "sub_main/004ADAB0.inc"
};
char gItemNames_667[13] = {
#include "sub_main/004ADAC8.inc"
};
char gItemNames_668[9] = {
#include "sub_main/004ADAD8.inc"
};
char gItemNames_669[11] = {
#include "sub_main/004ADAE8.inc"
};
char gItemNames_670[9] = {
#include "sub_main/004ADAF8.inc"
};
char gItemNames_671[11] = {
#include "sub_main/004ADB08.inc"
};
char gItemNames_672[11] = {
#include "sub_main/004ADB18.inc"
};
char gItemNames_673[13] = {
#include "sub_main/004ADB28.inc"
};
char gItemNames_674[13] = {
#include "sub_main/004ADB38.inc"
};
char gItemNames_675[7] = {
#include "sub_main/004ADB48.inc"
};
char gItemNames_676[15] = {
#include "sub_main/004ADB50.inc"
};
char gItemNames_677[15] = {
#include "sub_main/004ADB60.inc"
};
char gItemNames_678[15] = {
#include "sub_main/004ADB70.inc"
};
char gItemNames_679[9] = {
#include "sub_main/004ADB80.inc"
};
char gItemNames_680[13] = {
#include "sub_main/004ADB90.inc"
};
char gItemNames_681[11] = {
#include "sub_main/004ADBA0.inc"
};
char gItemNames_682[11] = {
#include "sub_main/004ADBB0.inc"
};
char gItemNames_683[9] = {
#include "sub_main/004ADBC0.inc"
};
char gItemNames_684[15] = {
#include "sub_main/004ADBD0.inc"
};
char gItemNames_685[13] = {
#include "sub_main/004ADBE0.inc"
};
char gItemNames_686[13] = {
#include "sub_main/004ADBF0.inc"
};
char gItemNames_687[13] = {
#include "sub_main/004ADC00.inc"
};
char gItemNames_688[7] = {
#include "sub_main/004ADC10.inc"
};
char gItemNames_689[7] = {
#include "sub_main/004ADC18.inc"
};
char gItemNames_690[7] = {
#include "sub_main/004ADC20.inc"
};
char gItemNames_691[7] = {
#include "sub_main/004ADC28.inc"
};
char gItemNames_692[7] = {
#include "sub_main/004ADC30.inc"
};
char gItemNames_693[7] = {
#include "sub_main/004ADC38.inc"
};
char gItemNames_694[7] = {
#include "sub_main/004ADC40.inc"
};
char gItemNames_695[11] = {
#include "sub_main/004ADC48.inc"
};
char gItemNames_696[15] = {
#include "sub_main/004ADC58.inc"
};
char gItemNames_698[13] = {
#include "sub_main/004ADC68.inc"
};
char gItemNames_699[7] = {
#include "sub_main/004ADC78.inc"
};
char gItemNames_700[7] = {
#include "sub_main/004ADC80.inc"
};
char gItemNames_701[11] = {
#include "sub_main/004ADC88.inc"
};
char gItemNames_702[13] = {
#include "sub_main/004ADC98.inc"
};
char gItemNames_703[13] = {
#include "sub_main/004ADCA8.inc"
};
char gItemNames_704[11] = {
#include "sub_main/004ADCB8.inc"
};
char gItemNames_705[13] = {
#include "sub_main/004ADCC8.inc"
};
char gItemNames_706[17] = {
#include "sub_main/004ADCE0.inc"
};
char gItemNames_707[15] = {
#include "sub_main/004ADCF8.inc"
};
char gItemNames_708[15] = {
#include "sub_main/004ADD08.inc"
};
char gItemNames_709[15] = {
#include "sub_main/004ADD18.inc"
};
char gItemNames_710[13] = {
#include "sub_main/004ADD28.inc"
};
char gItemNames_711[13] = {
#include "sub_main/004ADD38.inc"
};
char gItemNames_712[13] = {
#include "sub_main/004ADD48.inc"
};
char gItemNames_713[13] = {
#include "sub_main/004ADD58.inc"
};
char gItemNames_714[13] = {
#include "sub_main/004ADD68.inc"
};
char gItemNames_715[13] = {
#include "sub_main/004ADD78.inc"
};
char gItemNames_716[13] = {
#include "sub_main/004ADD88.inc"
};
char gItemNames_717[13] = {
#include "sub_main/004ADD98.inc"
};
char gItemNames_718[13] = {
#include "sub_main/004ADDA8.inc"
};
char gItemNames_719[13] = {
#include "sub_main/004ADDB8.inc"
};
char gItemNames_720[17] = {
#include "sub_main/004ADDD0.inc"
};
char gItemNames_721[13] = {
#include "sub_main/004ADDE8.inc"
};
char gItemNames_722[17] = {
#include "sub_main/004ADE00.inc"
};
char gItemNames_723[11] = {
#include "sub_main/004ADE18.inc"
};
char gItemNames_724[9] = {
#include "sub_main/004ADE28.inc"
};
char gItemNames_725[9] = {
#include "sub_main/004ADE38.inc"
};
char gItemNames_726[9] = {
#include "sub_main/004ADE48.inc"
};
char gItemNames_727[9] = {
#include "sub_main/004ADE58.inc"
};
char gItemNames_728[17] = {
#include "sub_main/004ADE70.inc"
};
char gItemNames_729[17] = {
#include "sub_main/004ADE90.inc"
};
char gItemNames_730[17] = {
#include "sub_main/004ADEB0.inc"
};
char gItemNames_731[9] = {
#include "sub_main/004ADEC8.inc"
};
char gItemNames_732[15] = {
#include "sub_main/004ADED8.inc"
};
char gItemNames_733[15] = {
#include "sub_main/004ADEE8.inc"
};
char gItemNames_734[15] = {
#include "sub_main/004ADEF8.inc"
};
char gItemNames_735[11] = {
#include "sub_main/004ADF08.inc"
};
char gItemNames_736[5] = {
#include "sub_main/004ADF18.inc"
};
char gItemNames_737[13] = {
#include "sub_main/004ADF20.inc"
};
char gItemNames_738[13] = {
#include "sub_main/004ADF30.inc"
};
char gItemNames_739[9] = {
#include "sub_main/004ADF40.inc"
};
char gItemNames_740[9] = {
#include "sub_main/004ADF50.inc"
};
char gItemNames_741[13] = {
#include "sub_main/004ADF60.inc"
};
char gItemNames_742[9] = {
#include "sub_main/004ADF70.inc"
};
char gItemNames_743[7] = {
#include "sub_main/004ADF80.inc"
};
char gItemNames_744[9] = {
#include "sub_main/004ADF88.inc"
};
char gItemNames_745[13] = {
#include "sub_main/004ADF98.inc"
};
char gItemNames_746[15] = {
#include "sub_main/004ADFA8.inc"
};
char gItemNames_747[9] = {
#include "sub_main/004ADFB8.inc"
};
char gItemNames_748[17] = {
#include "sub_main/004ADFD0.inc"
};
char gItemNames_749[13] = {
#include "sub_main/004ADFE8.inc"
};
char gItemNames_750[17] = {
#include "sub_main/004AE000.inc"
};
char gItemNames_751[15] = {
#include "sub_main/004AE018.inc"
};
char gItemNames_752[15] = {
#include "sub_main/004AE028.inc"
};
char gItemNames_753[17] = {
#include "sub_main/004AE040.inc"
};
char gItemNames_754[15] = {
#include "sub_main/004AE058.inc"
};
char gItemNames_755[17] = {
#include "sub_main/004AE070.inc"
};
char gItemNames_756[17] = {
#include "sub_main/004AE090.inc"
};
char gItemNames_757[17] = {
#include "sub_main/004AE0B0.inc"
};
char gItemNames_758[15] = {
#include "sub_main/004AE0C8.inc"
};
char gItemNames_759[13] = {
#include "sub_main/004AE0D8.inc"
};
char D_004AE0F0[17] = {
#include "sub_main/004AE0F0.inc"
};
char D_004AE110[17] = {
#include "sub_main/004AE110.inc"
};
char D_004AE130[17] = {
#include "sub_main/004AE130.inc"
};
char gItemNames_782[17] = {
#include "sub_main/004AE150.inc"
};
char gItemNames_794[9] = {
#include "sub_main/004AE168.inc"
};
char gItemNames_795[9] = {
#include "sub_main/004AE178.inc"
};
char gItemNames_796[9] = {
#include "sub_main/004AE188.inc"
};
char gItemNames_797[9] = {
#include "sub_main/004AE198.inc"
};
char gItemNames_798[7] = {
#include "sub_main/004AE1A8.inc"
};
char gItemNames_799[9] = {
#include "sub_main/004AE1B0.inc"
};
char gItemNames_800[7] = {
#include "sub_main/004AE1C0.inc"
};
char gItemNames_801[7] = {
#include "sub_main/004AE1C8.inc"
};
char gItemNames_802[7] = {
#include "sub_main/004AE1D0.inc"
};
char gItemNames_803[7] = {
#include "sub_main/004AE1D8.inc"
};
char gItemNames_804[7] = {
#include "sub_main/004AE1E0.inc"
};
char gItemNames_805[7] = {
#include "sub_main/004AE1E8.inc"
};
char gItemNames_806[7] = {
#include "sub_main/004AE1F0.inc"
};
char gItemNames_807[7] = {
#include "sub_main/004AE1F8.inc"
};
char gItemNames_808[7] = {
#include "sub_main/004AE200.inc"
};
char gItemNames_809[7] = {
#include "sub_main/004AE208.inc"
};
char gItemNames_810[7] = {
#include "sub_main/004AE210.inc"
};
char gItemNames_811[7] = {
#include "sub_main/004AE218.inc"
};
char gItemNames_812[7] = {
#include "sub_main/004AE220.inc"
};
char gItemNames_813[7] = {
#include "sub_main/004AE228.inc"
};
char gItemNames_814[7] = {
#include "sub_main/004AE230.inc"
};
char gItemNames_815[7] = {
#include "sub_main/004AE238.inc"
};
char gItemNames_816[7] = {
#include "sub_main/004AE240.inc"
};
char gItemNames_817[7] = {
#include "sub_main/004AE248.inc"
};
char gItemNames_818[7] = {
#include "sub_main/004AE250.inc"
};
char gItemNames_819[7] = {
#include "sub_main/004AE258.inc"
};
char gItemNames_820[7] = {
#include "sub_main/004AE260.inc"
};
char gItemNames_821[7] = {
#include "sub_main/004AE268.inc"
};
char gItemNames_822[7] = {
#include "sub_main/004AE270.inc"
};
char gItemNames_823[7] = {
#include "sub_main/004AE278.inc"
};
char gItemNames_824[7] = {
#include "sub_main/004AE280.inc"
};
char gItemNames_825[7] = {
#include "sub_main/004AE288.inc"
};
char gItemNames_826[7] = {
#include "sub_main/004AE290.inc"
};
char gItemNames_827[5] = {
#include "sub_main/004AE298.inc"
};
char gItemNames_828[7] = {
#include "sub_main/004AE2A0.inc"
};
char gItemNames_829[7] = {
#include "sub_main/004AE2A8.inc"
};
char gItemNames_830[7] = {
#include "sub_main/004AE2B0.inc"
};
char gItemNames_831[7] = {
#include "sub_main/004AE2B8.inc"
};
char gItemNames_832[7] = {
#include "sub_main/004AE2C0.inc"
};
char gItemNames_833[7] = {
#include "sub_main/004AE2C8.inc"
};
char gItemNames_834[7] = {
#include "sub_main/004AE2D0.inc"
};
char gItemNames_835[7] = {
#include "sub_main/004AE2D8.inc"
};
char gItemNames_836[7] = {
#include "sub_main/004AE2E0.inc"
};
char gItemNames_837[7] = {
#include "sub_main/004AE2E8.inc"
};
char gItemNames_838[7] = {
#include "sub_main/004AE2F0.inc"
};
char gItemNames_839[7] = {
#include "sub_main/004AE2F8.inc"
};
char gItemNames_840[7] = {
#include "sub_main/004AE300.inc"
};
char gItemNames_841[7] = {
#include "sub_main/004AE308.inc"
};
char gItemNames_842[7] = {
#include "sub_main/004AE310.inc"
};
char gItemNames_843[7] = {
#include "sub_main/004AE318.inc"
};
char gItemNames_844[7] = {
#include "sub_main/004AE320.inc"
};
char gItemNames_845[7] = {
#include "sub_main/004AE328.inc"
};
char gItemNames_846[7] = {
#include "sub_main/004AE330.inc"
};
char gItemNames_847[7] = {
#include "sub_main/004AE338.inc"
};
char gItemNames_848[7] = {
#include "sub_main/004AE340.inc"
};
char gItemNames_849[7] = {
#include "sub_main/004AE348.inc"
};
char gItemNames_850[7] = {
#include "sub_main/004AE350.inc"
};
char gItemNames_851[7] = {
#include "sub_main/004AE358.inc"
};
char gItemNames_852[7] = {
#include "sub_main/004AE360.inc"
};
char gItemNames_853[7] = {
#include "sub_main/004AE368.inc"
};
char gItemNames_854[7] = {
#include "sub_main/004AE370.inc"
};
char gItemNames_855[7] = {
#include "sub_main/004AE378.inc"
};
char gItemNames_856[5] = {
#include "sub_main/004AE380.inc"
};
char gItemNames_857[7] = {
#include "sub_main/004AE388.inc"
};
char gItemNames_858[7] = {
#include "sub_main/004AE390.inc"
};
char gItemNames_859[7] = {
#include "sub_main/004AE398.inc"
};
char gItemNames_860[7] = {
#include "sub_main/004AE3A0.inc"
};
char gItemNames_861[7] = {
#include "sub_main/004AE3A8.inc"
};
char gItemNames_862[7] = {
#include "sub_main/004AE3B0.inc"
};
char gItemNames_863[5] = {
#include "sub_main/004AE3B8.inc"
};
char gItemNames_864[7] = {
#include "sub_main/004AE3C0.inc"
};
char gItemNames_865[7] = {
#include "sub_main/004AE3C8.inc"
};
char gItemNames_866[5] = {
#include "sub_main/004AE3D0.inc"
};
char gItemNames_867[5] = {
#include "sub_main/004AE3D8.inc"
};
char gItemNames_868[7] = {
#include "sub_main/004AE3E0.inc"
};
char gItemNames_869[7] = {
#include "sub_main/004AE3E8.inc"
};
char gItemNames_870[7] = {
#include "sub_main/004AE3F0.inc"
};
char gItemNames_871[7] = {
#include "sub_main/004AE3F8.inc"
};
char gItemNames_872[7] = {
#include "sub_main/004AE400.inc"
};
char gItemNames_873[7] = {
#include "sub_main/004AE408.inc"
};
char gItemNames_874[9] = {
#include "sub_main/004AE410.inc"
};
char gItemNames_875[7] = {
#include "sub_main/004AE420.inc"
};
char gItemNames_876[7] = {
#include "sub_main/004AE428.inc"
};
char gItemNames_877[7] = {
#include "sub_main/004AE430.inc"
};
char gItemNames_878[7] = {
#include "sub_main/004AE438.inc"
};
char gItemNames_879[7] = {
#include "sub_main/004AE440.inc"
};
char gItemNames_880[7] = {
#include "sub_main/004AE448.inc"
};
char gItemNames_881[7] = {
#include "sub_main/004AE450.inc"
};
char gItemNames_882[7] = {
#include "sub_main/004AE458.inc"
};
char gItemNames_883[7] = {
#include "sub_main/004AE460.inc"
};
char gItemNames_884[7] = {
#include "sub_main/004AE468.inc"
};
char gItemNames_885[7] = {
#include "sub_main/004AE470.inc"
};
char gItemNames_886[7] = {
#include "sub_main/004AE478.inc"
};
char gItemNames_887[7] = {
#include "sub_main/004AE480.inc"
};
char gItemNames_888[7] = {
#include "sub_main/004AE488.inc"
};
char gItemNames_889[7] = {
#include "sub_main/004AE490.inc"
};
char gItemNames_890[7] = {
#include "sub_main/004AE498.inc"
};
char gItemNames_891[7] = {
#include "sub_main/004AE4A0.inc"
};
char gItemNames_892[7] = {
#include "sub_main/004AE4A8.inc"
};
char gItemNames_893[7] = {
#include "sub_main/004AE4B0.inc"
};
char gItemNames_894[7] = {
#include "sub_main/004AE4B8.inc"
};
char gItemNames_895[7] = {
#include "sub_main/004AE4C0.inc"
};
char gItemNames_896[7] = {
#include "sub_main/004AE4C8.inc"
};
char gItemNames_897[7] = {
#include "sub_main/004AE4D0.inc"
};
char gItemNames_898[7] = {
#include "sub_main/004AE4D8.inc"
};
char gItemNames_899[7] = {
#include "sub_main/004AE4E0.inc"
};
char gItemNames_900[7] = {
#include "sub_main/004AE4E8.inc"
};
char gItemNames_901[7] = {
#include "sub_main/004AE4F0.inc"
};
char gItemNames_902[7] = {
#include "sub_main/004AE4F8.inc"
};
char gItemNames_903[7] = {
#include "sub_main/004AE500.inc"
};
char gItemNames_904[7] = {
#include "sub_main/004AE508.inc"
};
char gItemNames_905[7] = {
#include "sub_main/004AE510.inc"
};
char gItemNames_906[7] = {
#include "sub_main/004AE518.inc"
};
char gItemNames_907[7] = {
#include "sub_main/004AE520.inc"
};
char gItemNames_908[5] = {
#include "sub_main/004AE528.inc"
};
char gItemNames_909[7] = {
#include "sub_main/004AE530.inc"
};
char gItemNames_910[7] = {
#include "sub_main/004AE538.inc"
};
char gItemNames_911[7] = {
#include "sub_main/004AE540.inc"
};
char gItemNames_912[7] = {
#include "sub_main/004AE548.inc"
};
char gItemNames_913[7] = {
#include "sub_main/004AE550.inc"
};
char gItemNames_914[7] = {
#include "sub_main/004AE558.inc"
};
char gItemNames_915[7] = {
#include "sub_main/004AE560.inc"
};
char gItemNames_916[7] = {
#include "sub_main/004AE568.inc"
};
char gItemNames_917[7] = {
#include "sub_main/004AE570.inc"
};
char gItemNames_918[7] = {
#include "sub_main/004AE578.inc"
};
char gItemNames_919[7] = {
#include "sub_main/004AE580.inc"
};
char gItemNames_920[7] = {
#include "sub_main/004AE588.inc"
};
#pragma rodata off

/* A hit by object o (owned by player p) on player t at pos: the reaction,
 * the damage (attack factor, defence, element resistance; halved in versus
 * quests) and the hit sound. */
void ObjAttach_DamagePlayer(ObjAttach *attach, Player *attacker, Player *victim, f32 *pos)
{
    f32 atk;
    f32 powerF;
    f32 defense;
    f32 factor;
    u8 stun;
    s16 damage;
    u8 guard;
    u8 attackId;
    u8 elemMarks;

    attackId = attach->hitId;
    if (!(attach->multiHit & 0xF)) {
        attach->state = ATTACH_HIT;
        attach->step = 0;
        attach->didHit = 1;
    } else {
        ObjAttach_NextMultiHit(attach);
    }
    attach->targetEm = (Em *) victim;
    attach->targetPart = 0;
    attach->hitKind = 0;
    atk = 1.0f;
    if (attach->ownerIndex != ATTACH_OWNER_NONE) {
        ObjAttach_LinkTarget(attach, attacker);
        attacker->hitPending = 1;
        if (Player_TestFlags(attach, ATKF_OWNER_CONTACT) != 0) {
            attacker->hitStopAdd = attach->hitStop;
        }
        if (Player_TestFlags(attach, ATKF_REPORT_CONTACT) != 0) {
            attacker->contactHit = 1;
        }
        attacker->hitKind = 0;
        attacker->contact = victim;
        attacker->contactAttach = attach;
        atk = attacker->attackFactor;
    }
    victim->hitIds[victim->hitIdNext] = attackId;
    victim->hitIdNext = (victim->hitIdNext + 1) & 0xF;
    victim->partFlags[0] |= attach->hitMarks;
    if ((u8)ObjAttach_TargetIsRemote((Em *)victim) != 0) {
        return;
    }
    if (Player_IsLocal(victim) == 1) {
        switch (attach->attackKind) {
        case ATTACK_GIVE_ATTACK_UP:
            victim->attackDrug = 3;
            break;
        case ATTACK_GIVE_DEFENSE_UP:
            victim->defenseDrug = 6;
            break;
        case ATTACK_CURE_POISON:
            victim->poison = 0;
            break;
        case ATTACK_CURE_STATUS6:
            victim->status6Timer = 0;
            break;
        }
    }
    if (Side_Match(victim->side, attach->side, 1) != 1 || attach->attackKind != ATTACK_HEAL) {
        if ((victim->side == 0 && attach->isEm != 0) || attach->ownerIndex == ATTACH_OWNER_NONE || attach->attackKind == ATTACK_HEAL
            || Side_Match(victim->side, attach->side, 0) == 0 || (attach->side == 0 && attach->isEm != 0)) {
            Em_ApplyHitStatus(victim, attach);
            if (attach->attackKind == ATTACK_HEAL) {
                victim->hitDamage[0] -= attach->power;
            } else {
                powerF = (f32)attach->power * atk;
                if (attach->powerPct > 0) {
                    factor = attach->powerPct;
                    factor /= 100.0f;
                    powerF *= factor;
                }
                defense = victim->defenseFactor;
                factor = 80.0f;
                powerF = powerF - powerF * defense / (defense + factor);
                factor = 1.0f;
                elemMarks = attach->hitMarks;
                if (elemMarks & 4) {
                    factor = (100.0f - victim->resist[0]) / 100.0f;
                } else if (elemMarks & 0x10) {
                    factor = (100.0f - victim->resist[1]) / 100.0f;
                } else if (elemMarks & 0x40) {
                    factor = (100.0f - victim->resist[2]) / 100.0f;
                } else if (elemMarks & 0x20) {
                    factor = (100.0f - victim->resist[3]) / 100.0f;
                } else if (elemMarks & 0x80) {
                    factor = (100.0f - victim->resist[4]) / 100.0f;
                }
                stun = attach->stun;
                damage = powerF * factor;
                if (Quest_IsVersus() == 1 && victim->isEm == 0 && attach->isEm == 0) {
                    if (damage > 0) {
                        damage /= 2;
                        if (damage <= 0) {
                            damage = 1;
                        }
                    }
                    if (stun > 0) {
                        stun = stun / 2;
                        if (stun <= 0) {
                            stun = 1;
                        }
                    }
                }
                victim->hitDamage[0] += damage;
                victim->status5Hit += stun;
            }
        }
    }
    victim->target = attacker;
    victim->hitPos[0] = pos[0];
    victim->hitPos[1] = pos[1];
    victim->hitPos[2] = pos[2];
    victim->hitReceived = 2;
    if (Player_CanGuard(victim) == 0) {
        guard = 1;
    } else if ((Player_HasSkill(victim, SKILL_AUTO_GUARD) == 1 || HeavyBowgun_HasShield(victim) == 1) && victim->weaponOut != 0 && victim->prevFreeState != 0) {
        guard = 0;
    } else if (Player_TestFlag(victim, PLF_GUARD) != 0) {
        guard = 0;
    } else {
        guard = 1;
    }
    if (Player_TestFlag(victim, PLF_GUARD) == 0 || Player_InGuardArc((u16)victim->rot[1], victim->hitAngle) == 0
        || victim->stamina < 75 || guard == 1) {
        switch (attach->hitSe) {
        case 255:
            break;
        case 0:
        default:
            Sound_PlayPlayerSe(victim, 33, 0, pos, 1, 0);
            break;
        case 1:
            Sound_PlayPlayerSe(victim, 32, 0, pos, 1, 0);
            break;
        }
    }
    victim->flinchPart = 0;
    victim->hitAttach = attach;
    victim->lastAttackKind = attach->attackKind;
    victim->hitForce = attach->hitForce;
    victim->hitAttrs |= attach->attrs;
}

/* Sets the hit target's hitAngle: the heading from `from` to `to` plus a quarter turn when
 * attach's attrs has 0x10, else attach's yaw plus its hitYawOfs (degrees). */
void ObjAttach_SetHitHeading(ObjAttach *attach, Player *target, f32 *from, f32 *to)
{
    if (attach->attrs & 0x10) {
        target->hitAngle = Vec_Heading(from, to) + ANGLE_TURN / 4;
    } else {
        target->hitAngle = attach->yaw + (u16)(s32)(0.5f + 65536.0f * attach->hitYawOfs / 360.0f);
    }
}

void Player_BodyCapsule(Player *p, HitCapsule *cap, f32 r);
void ObjAttach_HitEffects(ObjAttach *o, Player *p, ObjHead *e, f32 *out);
extern HitShape *gEmBoneHits[];
/* Tests attach's attack shapes against target: a player's body capsule (a fainted player is hit
 * only by ATTACK_REVIVE, and only it hits one), or each hurt shape of a monster's kind
 * (gEmBoneHits[type]), skipping the shapes this attack can't hit (the HITSHAPE_* flags: closed
 * wind or hermit shapes, never-hit shapes, the trap and hold shapes). The first hit sets the
 * hit's heading and runs ObjAttach_DamagePlayer or ObjAttach_DamageEm, then ObjAttach_HitEffects. */
void ObjAttach_HitTarget(ObjAttach *attach, Player *attacker, ObjHead *target)
{
    HitShape *list;
    HitShape *elist;
    f32 out[3];
    HitCapsule segA;
    HitCapsule segB;
    HitSphere sphA;
    HitSphere sphB;
    Capsule capA;
    Capsule capB;
    s16 shapeA;
    s16 shapeB;
    s16 hit;
    u8 shapeAtk;
    u8 shapeAtk2;
    u8 kind;
    Em *em;

    if (target->noHitTimer != 0) {
        return;
    }
    if (attach->hitList == 0) {
        return;
    }
    list = attach->hitList;
    if (target->isEm == 0) {
        if (target->stateGroup == PL_GROUP_FAINT) {
            if (attach->attackKind != ATTACK_REVIVE) {
                return;
            }
        } else if (attach->attackKind == ATTACK_REVIVE) {
            return;
        }
        Player_BodyCapsule((Player *)target, &segB, 40.0f);
        Capsule_Copy(&segB, &capB);
    }
    while (list->bone != HIT_BONE_END) {
        switch (list->bone) {
        case HIT_BONE_AT_POS:
            shapeA = HitShape_Place(attach->pos, list, &segA, &sphA, attach->hitScale);
            break;
        case HIT_BONE_SEGMENT:
            shapeA = Capsule_FromPoints(attach->pos, attach->pos2, list, &segA, attach->hitScale);
            break;
        case HIT_BONE_GROUP:
            HitList_NextForObj(attach, &list);
            continue;
        default:
            shapeA = BoneHit_Place((ObjHead *)attacker, list, &segA, &sphA);
            break;
        }
        if (shapeA != 0) {
            Capsule_Copy(&segA, &capA);
        }
        if (target->isEm == 0) {
            hit = (shapeA == 1) ? Capsule_ContactCapsule(&capA, &capB, out) : (s32)(u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
            if (hit != 0) {
                if (shapeA == 1) {
                    ObjAttach_SetHitHeading(attach, (Player *)target, capB.mid, capA.mid);
                } else {
                    ObjAttach_SetHitHeading(attach, (Player *)target, capB.mid, sphA.pos);
                }
                ObjAttach_DamagePlayer(attach, attacker, (Player *)target, out);
                ObjAttach_HitEffects(attach, attacker, target, out);
                return;
            }
        } else {
            em = (Em *)target;
            elist = gEmBoneHits[em->type];
            while (elist->bone != HIT_BONE_END) {
                switch (elist->bone) {
                case HIT_BONE_AT_POS:
                case HIT_BONE_SEGMENT:
                    shapeB = HitShape_Place(target->pos, elist, &segB, &sphB, 1.0f);
                    break;
                case HIT_BONE_GROUP:
                    HitList_Next((Em *)target, &elist);
                    continue;
                default:
                    shapeB = BoneHit_Place(target, elist, &segB, &sphB);
                    break;
                }
                if ((elist->openFlags & HITSHAPE_WIND_ONLY) && (em->windTimer == 0 || Player_TestFlags(attach, ATKF_WIND_DEFLECTS) == 0)) {
                    goto next;
                }
                if ((elist->openFlags & HITSHAPE_HERMIT_ONLY) && (s8)em->hermitTimer == 0) {
                    goto next;
                }
                if (elist->offFlags & HITSHAPE_NO_HIT) {
                    goto next;
                }
                shapeAtk = elist->atkFlags;
                if (shapeAtk & HITSHAPE_TRAP_ONLY) {
                    if (em->remote != 0 || (attach->attackKind != ATTACK_TRAP_A && attach->attackKind != ATTACK_TRAP_B)) {
                        goto next;
                    }
                }
                shapeAtk2 = elist->atkFlags2;
                if (shapeAtk2 & HITSHAPE2_HOLD_ONLY) {
                    if (em->remote != 0 || attach->attackKind != ATTACK_HOLD) {
                        goto next;
                    }
                }
                kind = attach->attackKind;
                if (kind == 6 || kind == 9) {
                    if (!(shapeAtk & HITSHAPE_TRAP_HIT) || em->remote != 0) {
                        goto next;
                    }
                }
                if (kind == 19) {
                    if (!(shapeAtk2 & HITSHAPE2_HOLD_HIT) || em->remote != 0) {
                        goto next;
                    }
                }
                if (kind == ATTACK_WIND || kind == ATTACK_WIND_WEAK) {
                    if (!(shapeAtk & HITSHAPE_WIND_HIT)) {
                        goto next;
                    }
                }
                if (shapeB != 0) {
                    Capsule_Copy(&segB, &capB);
                }
                if (shapeA == 1) {
                    if (shapeB == 1) {
                        hit = Capsule_ContactCapsule(&capA, &capB, out);
                    } else {
                        hit = (u8)Capsule_SphereContactOnSphere(&capA, sphB.pos, out, sphB.radius);
                    }
                } else if (shapeB == 1) {
                    hit = (u8)Capsule_SphereContact(sphA.pos, &capB, out, sphA.radius);
                } else {
                    hit = (u8)Spheres_Touch(sphA.radius, sphB.radius, sphA.pos, sphB.pos, out);
                }
                if (hit != 0) {
                    if (shapeA == 1) {
                        if (shapeB == 1) {
                            ObjAttach_SetHitHeading(attach, (Player *)target, capB.mid, capA.mid);
                        } else {
                            ObjAttach_SetHitHeading(attach, (Player *)target, sphB.pos, capA.mid);
                        }
                    } else if (shapeB == 1) {
                        ObjAttach_SetHitHeading(attach, (Player *)target, capB.mid, sphA.pos);
                    } else {
                        ObjAttach_SetHitHeading(attach, (Player *)target, sphB.pos, sphA.pos);
                    }
                    ObjAttach_DamageEm(attach, attacker, (Em *)target, list, elist, out);
                    ObjAttach_HitEffects(attach, attacker, target, out);
                    return;
                }
            next: /* the shape is skipped; written as a for loop with continue, the loop compiles differently */
                elist++;
            }
        }
        list++;
    }
}

void ObjAttach_LinkTarget(ObjAttach *attach, Player *attacker)
{
    attacker->hitTimers = &attach->hitDelay;
}

/* A multi-hit attack's next hit: one fewer hit left (multiHit's low 4 bits); when the delay
 * (the high 4 bits) is set, the window reopens after it with a new hit id. */
void ObjAttach_NextMultiHit(ObjAttach *attach)
{
    u8 bits = attach->multiHit;

    attach->multiHit = (bits & 0xF0) | (s8)((s8)(bits & 0xF) - 1);
    if (attach->multiHit >> 4) {
        attach->hitDelay = attach->multiHit >> 4;
        attach->hitPhase = 1;
        attach->hitId = HitId_Next();
    }
}

/* Counts down the object's hit window by hitPhase (0 off, 1 waiting hitDelay frames, 2 open for
 * hitFrames). When it closes and nextRow is set, the owner player's row nextRow is loaded
 * (ObjAttach_LoadOwnedRow) and the window reopens; while open, mvFramesA counts down (255:
 * never), then mvFramesB: ObjAttach_DamageEm picks the motion value row by them. */
void ObjAttach_UpdateTimers(ObjAttach *attach)
{
    switch (attach->hitPhase) {
    case 0:
        break;
    case 1:
        if (attach->hitDelay-- != 0) {
            break;
        }
        attach->hitDelay = 0;
        attach->hitPhase++;
    case 2:
        if (attach->hitFrames == 0) {
            if (attach->nextRow != 0) {
                ObjAttach_LoadOwnedRow(attach, &gPlayers[attach->idx], attach->nextRow);
                attach->hitPhase = 1;
                if (attach->hitDelay == 0) {
                    attach->hitPhase = 2;
                    attach->hitFrames--;
                } else {
                    attach->hitPhase = 1;
                    break;
                }
            } else {
                attach->hitPhase = 0;
                attach->hitDelay = 0;
                attach->hitFrames = 0;
                break;
            }
        } else {
            attach->hitFrames--;
        }
        if (attach->mvFramesA != 0 && attach->mvFramesA != 255) {
            attach->mvFramesA--;
            break;
        }
        if (attach->mvFramesB != 0) {
            attach->mvFramesB--;
        }
        break;
    }
}

/* ObjAttach_UpdateTimers for every active attack object whose delay is over. */
void ObjAttach_UpdateAllTimers(void)
{
    ObjAttach *attach;

    attach = gObjAttachList;
    if (attach != NULL) {
        do {
            if (attach->active != 0 && attach->delay == 0) {
                ObjAttach_UpdateTimers(attach);
            }
            attach = attach->next;
        } while (attach != NULL);
    }
}

void HitId_Reset(void)
{
    gScene.hitId = 0;
}

/* Steps the id counter SceneState.hitId (gScene.hitId is the same byte), skipping 0; returns it. */
u8 HitId_Next(void)
{
    SceneState *scene = (SceneState *)&gScene;

    gScene.hitId++;
    if (gScene.hitId == 0) {
        scene->hitId = 1;
    }
    return scene->hitId;
}
