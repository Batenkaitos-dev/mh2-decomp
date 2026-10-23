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

/* The rest of this unit's .rodata: objects no code loads (data may point to them), kept as
 * the game's bytes. */
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB180);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB1F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB218);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB228);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB238);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB258);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB268);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB278);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB2B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB2C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB408);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB418);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB550);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB560);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB580);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB5F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB608);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB618);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB620);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB630);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB640);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB678);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB680);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB690);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB698);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB6F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB708);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB718);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB728);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB738);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB748);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB758);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB768);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB788);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB798);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB7F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB808);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB818);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB828);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB838);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB850);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB860);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB870);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB880);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB890);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB8F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB900);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB910);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB920);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB930);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB940);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB950);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB958);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB968);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB978);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB988);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AB9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABAD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABAF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABB90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABBF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABC90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABCF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABD98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABDE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABE98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABEF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABF90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ABFF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC000);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC008);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC010);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC038);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC048);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC068);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC070);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC078);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC080);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC088);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC098);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC0F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC108);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC118);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC128);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC138);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC150);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC188);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC208);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC288);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC298);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC2F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC308);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC348);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC358);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC440);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC450);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC4F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC500);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC520);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC588);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC5B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC5C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC5D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC5E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC600);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC620);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC638);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC680);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC6F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC708);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC718);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC728);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC738);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC748);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC758);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC768);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC788);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC798);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC7F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC808);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC818);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC828);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC838);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC878);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC888);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC898);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC8F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC908);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC928);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC938);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC948);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC958);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC968);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC978);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC988);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC998);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AC9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACA98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACAF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACB98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACBE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACC98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACCA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACCB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACCC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACCE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACD90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACDF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACE88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACEF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACF98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ACFF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD008);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD038);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD048);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD068);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD078);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD088);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD098);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD0F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD108);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD118);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD128);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD138);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD148);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD158);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD180);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD190);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD2F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD330);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD370);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD390);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD430);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD468);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD478);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD488);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD508);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD518);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD538);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD548);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD568);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD578);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD588);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD598);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD5F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD608);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD618);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD628);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD638);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD648);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD658);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD668);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD678);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD688);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD698);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD6E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD700);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD720);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD740);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD760);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD778);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD790);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD7F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD810);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD830);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD848);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD858);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD868);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD880);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD8A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD8C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD8D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD8F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD908);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD918);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD930);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD948);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD960);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD980);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AD9F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADA90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAD8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADAF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADB90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBA0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBC0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADBF0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC10);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADC98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCE0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADCF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD68);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD78);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADD98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADDA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADDB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADDD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADDE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE00);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE28);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE38);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE48);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE58);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADE90);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADEB0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADEC8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADED8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADEE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADEF8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF08);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF18);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF20);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF30);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF40);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF50);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF60);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF70);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF80);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF88);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADF98);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADFA8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADFB8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADFD0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004ADFE8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE000);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE018);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE028);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE040);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE058);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE070);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE090);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE0B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE0C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE0D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE0F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE110);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE130);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE150);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE168);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE178);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE188);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE198);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE1F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE200);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE208);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE210);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE218);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE220);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE228);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE230);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE238);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE240);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE248);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE250);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE258);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE260);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE268);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE270);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE278);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE280);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE288);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE290);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE298);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE2F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE300);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE308);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE310);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE318);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE320);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE328);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE330);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE338);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE340);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE348);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE350);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE358);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE360);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE368);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE370);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE378);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE380);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE388);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE390);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE398);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE3F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE400);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE408);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE410);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE420);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE428);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE430);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE438);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE440);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE448);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE450);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE458);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE460);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE468);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE470);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE478);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE480);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE488);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE490);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE498);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4A0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4A8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4B0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4B8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4C0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4C8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4D0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4D8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4E0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4E8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4F0);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE4F8);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE500);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE508);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE510);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE518);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE520);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE528);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE530);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE538);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE540);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE548);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE550);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE558);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE560);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE568);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE570);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE578);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE580);
INCLUDE_RODATA("asm/sub_main/nonmatchings/hit_detection", D_004AE588);

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
