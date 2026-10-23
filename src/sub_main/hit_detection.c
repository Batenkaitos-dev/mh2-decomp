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
const char D_004AB180[13] = {
#include "sub_main/004AB180.inc"
};
const char D_004AB190[15] = {
#include "sub_main/004AB190.inc"
};
const char D_004AB1A0[15] = {
#include "sub_main/004AB1A0.inc"
};
const char D_004AB1B0[15] = {
#include "sub_main/004AB1B0.inc"
};
const char D_004AB1C0[15] = {
#include "sub_main/004AB1C0.inc"
};
const char D_004AB1D0[15] = {
#include "sub_main/004AB1D0.inc"
};
const char D_004AB1E0[17] = {
#include "sub_main/004AB1E0.inc"
};
const char D_004AB1F8[7] = {
#include "sub_main/004AB1F8.inc"
};
const char D_004AB200[15] = {
#include "sub_main/004AB200.inc"
};
const char D_004AB210[7] = {
#include "sub_main/004AB210.inc"
};
const char D_004AB218[15] = {
#include "sub_main/004AB218.inc"
};
const char D_004AB228[7] = {
#include "sub_main/004AB228.inc"
};
const char D_004AB230[7] = {
#include "sub_main/004AB230.inc"
};
const char D_004AB238[7] = {
#include "sub_main/004AB238.inc"
};
const char D_004AB240[15] = {
#include "sub_main/004AB240.inc"
};
const char D_004AB250[7] = {
#include "sub_main/004AB250.inc"
};
const char D_004AB258[15] = {
#include "sub_main/004AB258.inc"
};
const char D_004AB268[11] = {
#include "sub_main/004AB268.inc"
};
const char D_004AB278[7] = {
#include "sub_main/004AB278.inc"
};
const char D_004AB280[15] = {
#include "sub_main/004AB280.inc"
};
const char D_004AB290[11] = {
#include "sub_main/004AB290.inc"
};
const char D_004AB2A0[17] = {
#include "sub_main/004AB2A0.inc"
};
const char D_004AB2B8[15] = {
#include "sub_main/004AB2B8.inc"
};
const char D_004AB2C8[11] = {
#include "sub_main/004AB2C8.inc"
};
const char D_004AB2E0[17] = {
#include "sub_main/004AB2E0.inc"
};
const char D_004AB300[17] = {
#include "sub_main/004AB300.inc"
};
const char D_004AB318[7] = {
#include "sub_main/004AB318.inc"
};
const char D_004AB320[5] = {
#include "sub_main/004AB320.inc"
};
const char D_004AB328[15] = {
#include "sub_main/004AB328.inc"
};
const char D_004AB338[7] = {
#include "sub_main/004AB338.inc"
};
const char D_004AB340[13] = {
#include "sub_main/004AB340.inc"
};
const char D_004AB350[11] = {
#include "sub_main/004AB350.inc"
};
const char D_004AB360[5] = {
#include "sub_main/004AB360.inc"
};
const char D_004AB368[9] = {
#include "sub_main/004AB368.inc"
};
const char D_004AB378[11] = {
#include "sub_main/004AB378.inc"
};
const char D_004AB388[9] = {
#include "sub_main/004AB388.inc"
};
const char D_004AB398[7] = {
#include "sub_main/004AB398.inc"
};
const char D_004AB3A0[11] = {
#include "sub_main/004AB3A0.inc"
};
const char D_004AB3B0[9] = {
#include "sub_main/004AB3B0.inc"
};
const char D_004AB3C0[13] = {
#include "sub_main/004AB3C0.inc"
};
const char D_004AB3D0[5] = {
#include "sub_main/004AB3D0.inc"
};
const char D_004AB3D8[7] = {
#include "sub_main/004AB3D8.inc"
};
const char D_004AB3E0[15] = {
#include "sub_main/004AB3E0.inc"
};
const char D_004AB3F0[11] = {
#include "sub_main/004AB3F0.inc"
};
const char D_004AB400[7] = {
#include "sub_main/004AB400.inc"
};
const char D_004AB408[11] = {
#include "sub_main/004AB408.inc"
};
const char D_004AB418[9] = {
#include "sub_main/004AB418.inc"
};
const char D_004AB428[9] = {
#include "sub_main/004AB428.inc"
};
const char D_004AB438[11] = {
#include "sub_main/004AB438.inc"
};
const char D_004AB448[13] = {
#include "sub_main/004AB448.inc"
};
const char D_004AB458[7] = {
#include "sub_main/004AB458.inc"
};
const char D_004AB460[13] = {
#include "sub_main/004AB460.inc"
};
const char D_004AB470[15] = {
#include "sub_main/004AB470.inc"
};
const char D_004AB480[15] = {
#include "sub_main/004AB480.inc"
};
const char D_004AB490[17] = {
#include "sub_main/004AB490.inc"
};
const char D_004AB4A8[11] = {
#include "sub_main/004AB4A8.inc"
};
const char D_004AB4B8[13] = {
#include "sub_main/004AB4B8.inc"
};
const char D_004AB4C8[9] = {
#include "sub_main/004AB4C8.inc"
};
const char D_004AB4D8[11] = {
#include "sub_main/004AB4D8.inc"
};
const char D_004AB4E8[15] = {
#include "sub_main/004AB4E8.inc"
};
const char D_004AB4F8[13] = {
#include "sub_main/004AB4F8.inc"
};
const char D_004AB510[17] = {
#include "sub_main/004AB510.inc"
};
const char D_004AB528[7] = {
#include "sub_main/004AB528.inc"
};
const char D_004AB530[11] = {
#include "sub_main/004AB530.inc"
};
const char D_004AB540[11] = {
#include "sub_main/004AB540.inc"
};
const char D_004AB550[13] = {
#include "sub_main/004AB550.inc"
};
const char D_004AB560[15] = {
#include "sub_main/004AB560.inc"
};
const char D_004AB570[15] = {
#include "sub_main/004AB570.inc"
};
const char D_004AB580[17] = {
#include "sub_main/004AB580.inc"
};
const char D_004AB598[7] = {
#include "sub_main/004AB598.inc"
};
const char D_004AB5A0[15] = {
#include "sub_main/004AB5A0.inc"
};
const char D_004AB5B0[7] = {
#include "sub_main/004AB5B0.inc"
};
const char D_004AB5B8[7] = {
#include "sub_main/004AB5B8.inc"
};
const char D_004AB5C0[17] = {
#include "sub_main/004AB5C0.inc"
};
const char D_004AB5D8[9] = {
#include "sub_main/004AB5D8.inc"
};
const char D_004AB5E8[9] = {
#include "sub_main/004AB5E8.inc"
};
const char D_004AB5F8[11] = {
#include "sub_main/004AB5F8.inc"
};
const char D_004AB608[9] = {
#include "sub_main/004AB608.inc"
};
const char D_004AB618[7] = {
#include "sub_main/004AB618.inc"
};
const char D_004AB620[13] = {
#include "sub_main/004AB620.inc"
};
const char D_004AB630[9] = {
#include "sub_main/004AB630.inc"
};
const char D_004AB640[7] = {
#include "sub_main/004AB640.inc"
};
const char D_004AB648[15] = {
#include "sub_main/004AB648.inc"
};
const char D_004AB658[9] = {
#include "sub_main/004AB658.inc"
};
const char D_004AB668[9] = {
#include "sub_main/004AB668.inc"
};
const char D_004AB678[7] = {
#include "sub_main/004AB678.inc"
};
const char D_004AB680[9] = {
#include "sub_main/004AB680.inc"
};
const char D_004AB690[7] = {
#include "sub_main/004AB690.inc"
};
const char D_004AB698[7] = {
#include "sub_main/004AB698.inc"
};
const char D_004AB6A0[11] = {
#include "sub_main/004AB6A0.inc"
};
const char D_004AB6B0[13] = {
#include "sub_main/004AB6B0.inc"
};
const char D_004AB6C0[11] = {
#include "sub_main/004AB6C0.inc"
};
const char D_004AB6D0[13] = {
#include "sub_main/004AB6D0.inc"
};
const char D_004AB6E0[15] = {
#include "sub_main/004AB6E0.inc"
};
const char D_004AB6F0[17] = {
#include "sub_main/004AB6F0.inc"
};
const char D_004AB708[11] = {
#include "sub_main/004AB708.inc"
};
const char D_004AB718[13] = {
#include "sub_main/004AB718.inc"
};
const char D_004AB728[9] = {
#include "sub_main/004AB728.inc"
};
const char D_004AB738[13] = {
#include "sub_main/004AB738.inc"
};
const char D_004AB748[13] = {
#include "sub_main/004AB748.inc"
};
const char D_004AB758[11] = {
#include "sub_main/004AB758.inc"
};
const char D_004AB768[11] = {
#include "sub_main/004AB768.inc"
};
const char D_004AB778[11] = {
#include "sub_main/004AB778.inc"
};
const char D_004AB788[11] = {
#include "sub_main/004AB788.inc"
};
const char D_004AB798[11] = {
#include "sub_main/004AB798.inc"
};
const char D_004AB7A8[11] = {
#include "sub_main/004AB7A8.inc"
};
const char D_004AB7B8[9] = {
#include "sub_main/004AB7B8.inc"
};
const char D_004AB7C8[9] = {
#include "sub_main/004AB7C8.inc"
};
const char D_004AB7D8[9] = {
#include "sub_main/004AB7D8.inc"
};
const char D_004AB7E8[13] = {
#include "sub_main/004AB7E8.inc"
};
const char D_004AB7F8[13] = {
#include "sub_main/004AB7F8.inc"
};
const char D_004AB808[13] = {
#include "sub_main/004AB808.inc"
};
const char D_004AB818[11] = {
#include "sub_main/004AB818.inc"
};
const char D_004AB828[11] = {
#include "sub_main/004AB828.inc"
};
const char D_004AB838[11] = {
#include "sub_main/004AB838.inc"
};
const char D_004AB848[7] = {
#include "sub_main/004AB848.inc"
};
const char D_004AB850[7] = {
#include "sub_main/004AB850.inc"
};
const char D_004AB858[7] = {
#include "sub_main/004AB858.inc"
};
const char D_004AB860[7] = {
#include "sub_main/004AB860.inc"
};
const char D_004AB868[7] = {
#include "sub_main/004AB868.inc"
};
const char D_004AB870[11] = {
#include "sub_main/004AB870.inc"
};
const char D_004AB880[11] = {
#include "sub_main/004AB880.inc"
};
const char D_004AB890[9] = {
#include "sub_main/004AB890.inc"
};
const char D_004AB8A0[9] = {
#include "sub_main/004AB8A0.inc"
};
const char D_004AB8B0[11] = {
#include "sub_main/004AB8B0.inc"
};
const char D_004AB8C0[11] = {
#include "sub_main/004AB8C0.inc"
};
const char D_004AB8D0[11] = {
#include "sub_main/004AB8D0.inc"
};
const char D_004AB8E0[11] = {
#include "sub_main/004AB8E0.inc"
};
const char D_004AB8F0[13] = {
#include "sub_main/004AB8F0.inc"
};
const char D_004AB900[11] = {
#include "sub_main/004AB900.inc"
};
const char D_004AB910[7] = {
#include "sub_main/004AB910.inc"
};
const char D_004AB918[7] = {
#include "sub_main/004AB918.inc"
};
const char D_004AB920[9] = {
#include "sub_main/004AB920.inc"
};
const char D_004AB930[9] = {
#include "sub_main/004AB930.inc"
};
const char D_004AB940[9] = {
#include "sub_main/004AB940.inc"
};
const char D_004AB950[7] = {
#include "sub_main/004AB950.inc"
};
const char D_004AB958[9] = {
#include "sub_main/004AB958.inc"
};
const char D_004AB968[9] = {
#include "sub_main/004AB968.inc"
};
const char D_004AB978[11] = {
#include "sub_main/004AB978.inc"
};
const char D_004AB988[11] = {
#include "sub_main/004AB988.inc"
};
const char D_004AB998[11] = {
#include "sub_main/004AB998.inc"
};
const char D_004AB9A8[11] = {
#include "sub_main/004AB9A8.inc"
};
const char D_004AB9B8[11] = {
#include "sub_main/004AB9B8.inc"
};
const char D_004AB9C8[11] = {
#include "sub_main/004AB9C8.inc"
};
const char D_004AB9D8[11] = {
#include "sub_main/004AB9D8.inc"
};
const char D_004AB9E8[15] = {
#include "sub_main/004AB9E8.inc"
};
const char D_004AB9F8[13] = {
#include "sub_main/004AB9F8.inc"
};
const char D_004ABA08[9] = {
#include "sub_main/004ABA08.inc"
};
const char D_004ABA20[17] = {
#include "sub_main/004ABA20.inc"
};
const char D_004ABA38[11] = {
#include "sub_main/004ABA38.inc"
};
const char D_004ABA48[7] = {
#include "sub_main/004ABA48.inc"
};
const char D_004ABA50[15] = {
#include "sub_main/004ABA50.inc"
};
const char D_004ABA60[13] = {
#include "sub_main/004ABA60.inc"
};
const char D_004ABA70[17] = {
#include "sub_main/004ABA70.inc"
};
const char D_004ABA90[17] = {
#include "sub_main/004ABA90.inc"
};
const char D_004ABAB0[17] = {
#include "sub_main/004ABAB0.inc"
};
const char D_004ABAD0[17] = {
#include "sub_main/004ABAD0.inc"
};
const char D_004ABAF0[17] = {
#include "sub_main/004ABAF0.inc"
};
const char D_004ABB10[17] = {
#include "sub_main/004ABB10.inc"
};
const char D_004ABB28[15] = {
#include "sub_main/004ABB28.inc"
};
const char D_004ABB38[15] = {
#include "sub_main/004ABB38.inc"
};
const char D_004ABB48[5] = {
#include "sub_main/004ABB48.inc"
};
const char D_004ABB50[7] = {
#include "sub_main/004ABB50.inc"
};
const char D_004ABB58[13] = {
#include "sub_main/004ABB58.inc"
};
const char D_004ABB68[5] = {
#include "sub_main/004ABB68.inc"
};
const char D_004ABB70[7] = {
#include "sub_main/004ABB70.inc"
};
const char D_004ABB78[7] = {
#include "sub_main/004ABB78.inc"
};
const char D_004ABB80[7] = {
#include "sub_main/004ABB80.inc"
};
const char D_004ABB88[7] = {
#include "sub_main/004ABB88.inc"
};
const char D_004ABB90[9] = {
#include "sub_main/004ABB90.inc"
};
const char D_004ABBA0[7] = {
#include "sub_main/004ABBA0.inc"
};
const char D_004ABBA8[7] = {
#include "sub_main/004ABBA8.inc"
};
const char D_004ABBB0[5] = {
#include "sub_main/004ABBB0.inc"
};
const char D_004ABBB8[9] = {
#include "sub_main/004ABBB8.inc"
};
const char D_004ABBC8[7] = {
#include "sub_main/004ABBC8.inc"
};
const char D_004ABBD0[9] = {
#include "sub_main/004ABBD0.inc"
};
const char D_004ABBE0[9] = {
#include "sub_main/004ABBE0.inc"
};
const char D_004ABBF0[13] = {
#include "sub_main/004ABBF0.inc"
};
const char D_004ABC00[9] = {
#include "sub_main/004ABC00.inc"
};
const char D_004ABC10[13] = {
#include "sub_main/004ABC10.inc"
};
const char D_004ABC20[13] = {
#include "sub_main/004ABC20.inc"
};
const char D_004ABC30[11] = {
#include "sub_main/004ABC30.inc"
};
const char D_004ABC40[11] = {
#include "sub_main/004ABC40.inc"
};
const char D_004ABC50[11] = {
#include "sub_main/004ABC50.inc"
};
const char D_004ABC60[11] = {
#include "sub_main/004ABC60.inc"
};
const char D_004ABC70[11] = {
#include "sub_main/004ABC70.inc"
};
const char D_004ABC80[11] = {
#include "sub_main/004ABC80.inc"
};
const char D_004ABC90[9] = {
#include "sub_main/004ABC90.inc"
};
const char D_004ABCA0[13] = {
#include "sub_main/004ABCA0.inc"
};
const char D_004ABCB0[13] = {
#include "sub_main/004ABCB0.inc"
};
const char D_004ABCC0[13] = {
#include "sub_main/004ABCC0.inc"
};
const char D_004ABCD0[17] = {
#include "sub_main/004ABCD0.inc"
};
const char D_004ABCF0[17] = {
#include "sub_main/004ABCF0.inc"
};
const char D_004ABD08[13] = {
#include "sub_main/004ABD08.inc"
};
const char D_004ABD18[13] = {
#include "sub_main/004ABD18.inc"
};
const char D_004ABD28[9] = {
#include "sub_main/004ABD28.inc"
};
const char D_004ABD38[9] = {
#include "sub_main/004ABD38.inc"
};
const char D_004ABD48[11] = {
#include "sub_main/004ABD48.inc"
};
const char D_004ABD58[13] = {
#include "sub_main/004ABD58.inc"
};
const char D_004ABD68[9] = {
#include "sub_main/004ABD68.inc"
};
const char D_004ABD78[13] = {
#include "sub_main/004ABD78.inc"
};
const char D_004ABD88[11] = {
#include "sub_main/004ABD88.inc"
};
const char D_004ABD98[7] = {
#include "sub_main/004ABD98.inc"
};
const char D_004ABDA0[7] = {
#include "sub_main/004ABDA0.inc"
};
const char D_004ABDA8[7] = {
#include "sub_main/004ABDA8.inc"
};
const char D_004ABDB0[7] = {
#include "sub_main/004ABDB0.inc"
};
const char D_004ABDB8[11] = {
#include "sub_main/004ABDB8.inc"
};
const char D_004ABDC8[15] = {
#include "sub_main/004ABDC8.inc"
};
const char D_004ABDE0[17] = {
#include "sub_main/004ABDE0.inc"
};
const char D_004ABE00[17] = {
#include "sub_main/004ABE00.inc"
};
const char D_004ABE18[13] = {
#include "sub_main/004ABE18.inc"
};
const char D_004ABE28[7] = {
#include "sub_main/004ABE28.inc"
};
const char D_004ABE30[7] = {
#include "sub_main/004ABE30.inc"
};
const char D_004ABE40[17] = {
#include "sub_main/004ABE40.inc"
};
const char D_004ABE60[17] = {
#include "sub_main/004ABE60.inc"
};
const char D_004ABE78[7] = {
#include "sub_main/004ABE78.inc"
};
const char D_004ABE80[7] = {
#include "sub_main/004ABE80.inc"
};
const char D_004ABE88[13] = {
#include "sub_main/004ABE88.inc"
};
const char D_004ABE98[13] = {
#include "sub_main/004ABE98.inc"
};
const char D_004ABEA8[13] = {
#include "sub_main/004ABEA8.inc"
};
const char D_004ABEB8[13] = {
#include "sub_main/004ABEB8.inc"
};
const char D_004ABEC8[9] = {
#include "sub_main/004ABEC8.inc"
};
const char D_004ABED8[11] = {
#include "sub_main/004ABED8.inc"
};
const char D_004ABEE8[5] = {
#include "sub_main/004ABEE8.inc"
};
const char D_004ABEF0[11] = {
#include "sub_main/004ABEF0.inc"
};
const char D_004ABF00[13] = {
#include "sub_main/004ABF00.inc"
};
const char D_004ABF10[17] = {
#include "sub_main/004ABF10.inc"
};
const char D_004ABF28[15] = {
#include "sub_main/004ABF28.inc"
};
const char D_004ABF40[17] = {
#include "sub_main/004ABF40.inc"
};
const char D_004ABF58[13] = {
#include "sub_main/004ABF58.inc"
};
const char D_004ABF68[9] = {
#include "sub_main/004ABF68.inc"
};
const char D_004ABF78[15] = {
#include "sub_main/004ABF78.inc"
};
const char D_004ABF90[17] = {
#include "sub_main/004ABF90.inc"
};
const char D_004ABFA8[13] = {
#include "sub_main/004ABFA8.inc"
};
const char D_004ABFB8[13] = {
#include "sub_main/004ABFB8.inc"
};
const char D_004ABFD0[17] = {
#include "sub_main/004ABFD0.inc"
};
const char D_004ABFE8[7] = {
#include "sub_main/004ABFE8.inc"
};
const char D_004ABFF0[13] = {
#include "sub_main/004ABFF0.inc"
};
const char D_004AC000[7] = {
#include "sub_main/004AC000.inc"
};
const char D_004AC008[7] = {
#include "sub_main/004AC008.inc"
};
const char D_004AC010[7] = {
#include "sub_main/004AC010.inc"
};
const char D_004AC018[9] = {
#include "sub_main/004AC018.inc"
};
const char D_004AC028[9] = {
#include "sub_main/004AC028.inc"
};
const char D_004AC038[13] = {
#include "sub_main/004AC038.inc"
};
const char D_004AC048[11] = {
#include "sub_main/004AC048.inc"
};
const char D_004AC058[13] = {
#include "sub_main/004AC058.inc"
};
const char D_004AC068[7] = {
#include "sub_main/004AC068.inc"
};
const char D_004AC070[7] = {
#include "sub_main/004AC070.inc"
};
const char D_004AC078[5] = {
#include "sub_main/004AC078.inc"
};
const char D_004AC080[7] = {
#include "sub_main/004AC080.inc"
};
const char D_004AC088[13] = {
#include "sub_main/004AC088.inc"
};
const char D_004AC098[15] = {
#include "sub_main/004AC098.inc"
};
const char D_004AC0A8[13] = {
#include "sub_main/004AC0A8.inc"
};
const char D_004AC0B8[11] = {
#include "sub_main/004AC0B8.inc"
};
const char D_004AC0C8[15] = {
#include "sub_main/004AC0C8.inc"
};
const char D_004AC0D8[15] = {
#include "sub_main/004AC0D8.inc"
};
const char D_004AC0F0[17] = {
#include "sub_main/004AC0F0.inc"
};
const char D_004AC108[13] = {
#include "sub_main/004AC108.inc"
};
const char D_004AC118[15] = {
#include "sub_main/004AC118.inc"
};
const char D_004AC128[13] = {
#include "sub_main/004AC128.inc"
};
const char D_004AC138[13] = {
#include "sub_main/004AC138.inc"
};
const char D_004AC150[17] = {
#include "sub_main/004AC150.inc"
};
const char D_004AC168[9] = {
#include "sub_main/004AC168.inc"
};
const char D_004AC178[11] = {
#include "sub_main/004AC178.inc"
};
const char D_004AC188[7] = {
#include "sub_main/004AC188.inc"
};
const char D_004AC190[15] = {
#include "sub_main/004AC190.inc"
};
const char D_004AC1A0[11] = {
#include "sub_main/004AC1A0.inc"
};
const char D_004AC1B0[13] = {
#include "sub_main/004AC1B0.inc"
};
const char D_004AC1C0[15] = {
#include "sub_main/004AC1C0.inc"
};
const char D_004AC1D0[15] = {
#include "sub_main/004AC1D0.inc"
};
const char D_004AC1E0[11] = {
#include "sub_main/004AC1E0.inc"
};
const char D_004AC1F0[17] = {
#include "sub_main/004AC1F0.inc"
};
const char D_004AC208[5] = {
#include "sub_main/004AC208.inc"
};
const char D_004AC210[15] = {
#include "sub_main/004AC210.inc"
};
const char D_004AC220[9] = {
#include "sub_main/004AC220.inc"
};
const char D_004AC230[15] = {
#include "sub_main/004AC230.inc"
};
const char D_004AC240[13] = {
#include "sub_main/004AC240.inc"
};
const char D_004AC250[13] = {
#include "sub_main/004AC250.inc"
};
const char D_004AC260[11] = {
#include "sub_main/004AC260.inc"
};
const char D_004AC270[17] = {
#include "sub_main/004AC270.inc"
};
const char D_004AC288[11] = {
#include "sub_main/004AC288.inc"
};
const char D_004AC298[15] = {
#include "sub_main/004AC298.inc"
};
const char D_004AC2A8[7] = {
#include "sub_main/004AC2A8.inc"
};
const char D_004AC2B0[13] = {
#include "sub_main/004AC2B0.inc"
};
const char D_004AC2C0[9] = {
#include "sub_main/004AC2C0.inc"
};
const char D_004AC2D0[17] = {
#include "sub_main/004AC2D0.inc"
};
const char D_004AC2E8[13] = {
#include "sub_main/004AC2E8.inc"
};
const char D_004AC2F8[7] = {
#include "sub_main/004AC2F8.inc"
};
const char D_004AC300[5] = {
#include "sub_main/004AC300.inc"
};
const char D_004AC308[7] = {
#include "sub_main/004AC308.inc"
};
const char D_004AC310[7] = {
#include "sub_main/004AC310.inc"
};
const char D_004AC318[7] = {
#include "sub_main/004AC318.inc"
};
const char D_004AC320[7] = {
#include "sub_main/004AC320.inc"
};
const char D_004AC328[11] = {
#include "sub_main/004AC328.inc"
};
const char D_004AC338[15] = {
#include "sub_main/004AC338.inc"
};
const char D_004AC348[11] = {
#include "sub_main/004AC348.inc"
};
const char D_004AC358[11] = {
#include "sub_main/004AC358.inc"
};
const char D_004AC368[11] = {
#include "sub_main/004AC368.inc"
};
const char D_004AC378[7] = {
#include "sub_main/004AC378.inc"
};
const char D_004AC380[7] = {
#include "sub_main/004AC380.inc"
};
const char D_004AC388[9] = {
#include "sub_main/004AC388.inc"
};
const char D_004AC398[11] = {
#include "sub_main/004AC398.inc"
};
const char D_004AC3A8[13] = {
#include "sub_main/004AC3A8.inc"
};
const char D_004AC3B8[9] = {
#include "sub_main/004AC3B8.inc"
};
const char D_004AC3C8[9] = {
#include "sub_main/004AC3C8.inc"
};
const char D_004AC3D8[7] = {
#include "sub_main/004AC3D8.inc"
};
const char D_004AC3E0[7] = {
#include "sub_main/004AC3E0.inc"
};
const char D_004AC3E8[5] = {
#include "sub_main/004AC3E8.inc"
};
const char D_004AC3F0[11] = {
#include "sub_main/004AC3F0.inc"
};
const char D_004AC400[15] = {
#include "sub_main/004AC400.inc"
};
const char D_004AC410[9] = {
#include "sub_main/004AC410.inc"
};
const char D_004AC420[7] = {
#include "sub_main/004AC420.inc"
};
const char D_004AC428[11] = {
#include "sub_main/004AC428.inc"
};
const char D_004AC438[7] = {
#include "sub_main/004AC438.inc"
};
const char D_004AC440[11] = {
#include "sub_main/004AC440.inc"
};
const char D_004AC450[9] = {
#include "sub_main/004AC450.inc"
};
const char D_004AC460[15] = {
#include "sub_main/004AC460.inc"
};
const char D_004AC470[13] = {
#include "sub_main/004AC470.inc"
};
const char D_004AC480[9] = {
#include "sub_main/004AC480.inc"
};
const char D_004AC490[13] = {
#include "sub_main/004AC490.inc"
};
const char D_004AC4A0[11] = {
#include "sub_main/004AC4A0.inc"
};
const char D_004AC4B0[11] = {
#include "sub_main/004AC4B0.inc"
};
const char D_004AC4C0[15] = {
#include "sub_main/004AC4C0.inc"
};
const char D_004AC4D0[13] = {
#include "sub_main/004AC4D0.inc"
};
const char D_004AC4E0[13] = {
#include "sub_main/004AC4E0.inc"
};
const char D_004AC4F0[13] = {
#include "sub_main/004AC4F0.inc"
};
const char D_004AC500[15] = {
#include "sub_main/004AC500.inc"
};
const char D_004AC510[13] = {
#include "sub_main/004AC510.inc"
};
const char D_004AC520[17] = {
#include "sub_main/004AC520.inc"
};
const char D_004AC540[17] = {
#include "sub_main/004AC540.inc"
};
const char D_004AC558[15] = {
#include "sub_main/004AC558.inc"
};
const char D_004AC570[17] = {
#include "sub_main/004AC570.inc"
};
const char D_004AC588[15] = {
#include "sub_main/004AC588.inc"
};
const char D_004AC598[15] = {
#include "sub_main/004AC598.inc"
};
const char D_004AC5B0[17] = {
#include "sub_main/004AC5B0.inc"
};
const char D_004AC5C8[13] = {
#include "sub_main/004AC5C8.inc"
};
const char D_004AC5D8[15] = {
#include "sub_main/004AC5D8.inc"
};
const char D_004AC5E8[13] = {
#include "sub_main/004AC5E8.inc"
};
const char D_004AC600[17] = {
#include "sub_main/004AC600.inc"
};
const char D_004AC620[17] = {
#include "sub_main/004AC620.inc"
};
const char D_004AC638[15] = {
#include "sub_main/004AC638.inc"
};
const char D_004AC648[13] = {
#include "sub_main/004AC648.inc"
};
const char D_004AC658[15] = {
#include "sub_main/004AC658.inc"
};
const char D_004AC668[13] = {
#include "sub_main/004AC668.inc"
};
const char D_004AC680[17] = {
#include "sub_main/004AC680.inc"
};
const char D_004AC6A0[17] = {
#include "sub_main/004AC6A0.inc"
};
const char D_004AC6B8[9] = {
#include "sub_main/004AC6B8.inc"
};
const char D_004AC6C8[11] = {
#include "sub_main/004AC6C8.inc"
};
const char D_004AC6D8[11] = {
#include "sub_main/004AC6D8.inc"
};
const char D_004AC6E8[9] = {
#include "sub_main/004AC6E8.inc"
};
const char D_004AC6F8[13] = {
#include "sub_main/004AC6F8.inc"
};
const char D_004AC708[15] = {
#include "sub_main/004AC708.inc"
};
const char D_004AC718[15] = {
#include "sub_main/004AC718.inc"
};
const char D_004AC728[11] = {
#include "sub_main/004AC728.inc"
};
const char D_004AC738[11] = {
#include "sub_main/004AC738.inc"
};
const char D_004AC748[13] = {
#include "sub_main/004AC748.inc"
};
const char D_004AC758[13] = {
#include "sub_main/004AC758.inc"
};
const char D_004AC768[11] = {
#include "sub_main/004AC768.inc"
};
const char D_004AC778[13] = {
#include "sub_main/004AC778.inc"
};
const char D_004AC788[11] = {
#include "sub_main/004AC788.inc"
};
const char D_004AC798[13] = {
#include "sub_main/004AC798.inc"
};
const char D_004AC7A8[13] = {
#include "sub_main/004AC7A8.inc"
};
const char D_004AC7B8[11] = {
#include "sub_main/004AC7B8.inc"
};
const char D_004AC7C8[9] = {
#include "sub_main/004AC7C8.inc"
};
const char D_004AC7D8[9] = {
#include "sub_main/004AC7D8.inc"
};
const char D_004AC7E8[11] = {
#include "sub_main/004AC7E8.inc"
};
const char D_004AC7F8[11] = {
#include "sub_main/004AC7F8.inc"
};
const char D_004AC808[11] = {
#include "sub_main/004AC808.inc"
};
const char D_004AC818[13] = {
#include "sub_main/004AC818.inc"
};
const char D_004AC828[9] = {
#include "sub_main/004AC828.inc"
};
const char D_004AC838[11] = {
#include "sub_main/004AC838.inc"
};
const char D_004AC848[11] = {
#include "sub_main/004AC848.inc"
};
const char D_004AC858[13] = {
#include "sub_main/004AC858.inc"
};
const char D_004AC868[11] = {
#include "sub_main/004AC868.inc"
};
const char D_004AC878[13] = {
#include "sub_main/004AC878.inc"
};
const char D_004AC888[11] = {
#include "sub_main/004AC888.inc"
};
const char D_004AC898[13] = {
#include "sub_main/004AC898.inc"
};
const char D_004AC8A8[13] = {
#include "sub_main/004AC8A8.inc"
};
const char D_004AC8B8[13] = {
#include "sub_main/004AC8B8.inc"
};
const char D_004AC8C8[13] = {
#include "sub_main/004AC8C8.inc"
};
const char D_004AC8D8[13] = {
#include "sub_main/004AC8D8.inc"
};
const char D_004AC8E8[11] = {
#include "sub_main/004AC8E8.inc"
};
const char D_004AC8F8[13] = {
#include "sub_main/004AC8F8.inc"
};
const char D_004AC908[11] = {
#include "sub_main/004AC908.inc"
};
const char D_004AC918[13] = {
#include "sub_main/004AC918.inc"
};
const char D_004AC928[13] = {
#include "sub_main/004AC928.inc"
};
const char D_004AC938[13] = {
#include "sub_main/004AC938.inc"
};
const char D_004AC948[13] = {
#include "sub_main/004AC948.inc"
};
const char D_004AC958[13] = {
#include "sub_main/004AC958.inc"
};
const char D_004AC968[11] = {
#include "sub_main/004AC968.inc"
};
const char D_004AC978[11] = {
#include "sub_main/004AC978.inc"
};
const char D_004AC988[9] = {
#include "sub_main/004AC988.inc"
};
const char D_004AC998[11] = {
#include "sub_main/004AC998.inc"
};
const char D_004AC9A8[11] = {
#include "sub_main/004AC9A8.inc"
};
const char D_004AC9B8[11] = {
#include "sub_main/004AC9B8.inc"
};
const char D_004AC9C8[11] = {
#include "sub_main/004AC9C8.inc"
};
const char D_004AC9D8[11] = {
#include "sub_main/004AC9D8.inc"
};
const char D_004AC9E8[11] = {
#include "sub_main/004AC9E8.inc"
};
const char D_004AC9F8[11] = {
#include "sub_main/004AC9F8.inc"
};
const char D_004ACA08[9] = {
#include "sub_main/004ACA08.inc"
};
const char D_004ACA18[11] = {
#include "sub_main/004ACA18.inc"
};
const char D_004ACA28[13] = {
#include "sub_main/004ACA28.inc"
};
const char D_004ACA38[13] = {
#include "sub_main/004ACA38.inc"
};
const char D_004ACA48[13] = {
#include "sub_main/004ACA48.inc"
};
const char D_004ACA58[13] = {
#include "sub_main/004ACA58.inc"
};
const char D_004ACA68[11] = {
#include "sub_main/004ACA68.inc"
};
const char D_004ACA78[13] = {
#include "sub_main/004ACA78.inc"
};
const char D_004ACA88[13] = {
#include "sub_main/004ACA88.inc"
};
const char D_004ACA98[13] = {
#include "sub_main/004ACA98.inc"
};
const char D_004ACAA8[13] = {
#include "sub_main/004ACAA8.inc"
};
const char D_004ACAB8[11] = {
#include "sub_main/004ACAB8.inc"
};
const char D_004ACAC8[15] = {
#include "sub_main/004ACAC8.inc"
};
const char D_004ACAD8[13] = {
#include "sub_main/004ACAD8.inc"
};
const char D_004ACAF0[17] = {
#include "sub_main/004ACAF0.inc"
};
const char D_004ACB10[17] = {
#include "sub_main/004ACB10.inc"
};
const char D_004ACB28[13] = {
#include "sub_main/004ACB28.inc"
};
const char D_004ACB38[13] = {
#include "sub_main/004ACB38.inc"
};
const char D_004ACB48[11] = {
#include "sub_main/004ACB48.inc"
};
const char D_004ACB58[11] = {
#include "sub_main/004ACB58.inc"
};
const char D_004ACB68[9] = {
#include "sub_main/004ACB68.inc"
};
const char D_004ACB78[11] = {
#include "sub_main/004ACB78.inc"
};
const char D_004ACB88[11] = {
#include "sub_main/004ACB88.inc"
};
const char D_004ACB98[11] = {
#include "sub_main/004ACB98.inc"
};
const char D_004ACBA8[11] = {
#include "sub_main/004ACBA8.inc"
};
const char D_004ACBB8[13] = {
#include "sub_main/004ACBB8.inc"
};
const char D_004ACBC8[13] = {
#include "sub_main/004ACBC8.inc"
};
const char D_004ACBD8[13] = {
#include "sub_main/004ACBD8.inc"
};
const char D_004ACBE8[11] = {
#include "sub_main/004ACBE8.inc"
};
const char D_004ACC00[17] = {
#include "sub_main/004ACC00.inc"
};
const char D_004ACC18[11] = {
#include "sub_main/004ACC18.inc"
};
const char D_004ACC28[11] = {
#include "sub_main/004ACC28.inc"
};
const char D_004ACC38[11] = {
#include "sub_main/004ACC38.inc"
};
const char D_004ACC48[11] = {
#include "sub_main/004ACC48.inc"
};
const char D_004ACC58[11] = {
#include "sub_main/004ACC58.inc"
};
const char D_004ACC68[9] = {
#include "sub_main/004ACC68.inc"
};
const char D_004ACC78[9] = {
#include "sub_main/004ACC78.inc"
};
const char D_004ACC88[13] = {
#include "sub_main/004ACC88.inc"
};
const char D_004ACC98[13] = {
#include "sub_main/004ACC98.inc"
};
const char D_004ACCA8[13] = {
#include "sub_main/004ACCA8.inc"
};
const char D_004ACCB8[13] = {
#include "sub_main/004ACCB8.inc"
};
const char D_004ACCC8[13] = {
#include "sub_main/004ACCC8.inc"
};
const char D_004ACCE0[17] = {
#include "sub_main/004ACCE0.inc"
};
const char D_004ACD00[17] = {
#include "sub_main/004ACD00.inc"
};
const char D_004ACD18[15] = {
#include "sub_main/004ACD18.inc"
};
const char D_004ACD30[17] = {
#include "sub_main/004ACD30.inc"
};
const char D_004ACD50[17] = {
#include "sub_main/004ACD50.inc"
};
const char D_004ACD68[15] = {
#include "sub_main/004ACD68.inc"
};
const char D_004ACD78[15] = {
#include "sub_main/004ACD78.inc"
};
const char D_004ACD90[17] = {
#include "sub_main/004ACD90.inc"
};
const char D_004ACDA8[11] = {
#include "sub_main/004ACDA8.inc"
};
const char D_004ACDB8[11] = {
#include "sub_main/004ACDB8.inc"
};
const char D_004ACDC8[13] = {
#include "sub_main/004ACDC8.inc"
};
const char D_004ACDD8[15] = {
#include "sub_main/004ACDD8.inc"
};
const char D_004ACDE8[9] = {
#include "sub_main/004ACDE8.inc"
};
const char D_004ACDF8[13] = {
#include "sub_main/004ACDF8.inc"
};
const char D_004ACE08[13] = {
#include "sub_main/004ACE08.inc"
};
const char D_004ACE20[17] = {
#include "sub_main/004ACE20.inc"
};
const char D_004ACE38[9] = {
#include "sub_main/004ACE38.inc"
};
const char D_004ACE48[15] = {
#include "sub_main/004ACE48.inc"
};
const char D_004ACE58[15] = {
#include "sub_main/004ACE58.inc"
};
const char D_004ACE68[13] = {
#include "sub_main/004ACE68.inc"
};
const char D_004ACE78[11] = {
#include "sub_main/004ACE78.inc"
};
const char D_004ACE88[13] = {
#include "sub_main/004ACE88.inc"
};
const char D_004ACEA0[17] = {
#include "sub_main/004ACEA0.inc"
};
const char D_004ACEB8[13] = {
#include "sub_main/004ACEB8.inc"
};
const char D_004ACEC8[13] = {
#include "sub_main/004ACEC8.inc"
};
const char D_004ACED8[13] = {
#include "sub_main/004ACED8.inc"
};
const char D_004ACEE8[11] = {
#include "sub_main/004ACEE8.inc"
};
const char D_004ACEF8[11] = {
#include "sub_main/004ACEF8.inc"
};
const char D_004ACF08[13] = {
#include "sub_main/004ACF08.inc"
};
const char D_004ACF18[13] = {
#include "sub_main/004ACF18.inc"
};
const char D_004ACF28[13] = {
#include "sub_main/004ACF28.inc"
};
const char D_004ACF38[13] = {
#include "sub_main/004ACF38.inc"
};
const char D_004ACF48[15] = {
#include "sub_main/004ACF48.inc"
};
const char D_004ACF58[9] = {
#include "sub_main/004ACF58.inc"
};
const char D_004ACF68[9] = {
#include "sub_main/004ACF68.inc"
};
const char D_004ACF78[11] = {
#include "sub_main/004ACF78.inc"
};
const char D_004ACF88[9] = {
#include "sub_main/004ACF88.inc"
};
const char D_004ACF98[11] = {
#include "sub_main/004ACF98.inc"
};
const char D_004ACFA8[11] = {
#include "sub_main/004ACFA8.inc"
};
const char D_004ACFB8[11] = {
#include "sub_main/004ACFB8.inc"
};
const char D_004ACFC8[11] = {
#include "sub_main/004ACFC8.inc"
};
const char D_004ACFD8[11] = {
#include "sub_main/004ACFD8.inc"
};
const char D_004ACFE8[11] = {
#include "sub_main/004ACFE8.inc"
};
const char D_004ACFF8[11] = {
#include "sub_main/004ACFF8.inc"
};
const char D_004AD008[11] = {
#include "sub_main/004AD008.inc"
};
const char D_004AD018[11] = {
#include "sub_main/004AD018.inc"
};
const char D_004AD028[9] = {
#include "sub_main/004AD028.inc"
};
const char D_004AD038[11] = {
#include "sub_main/004AD038.inc"
};
const char D_004AD048[11] = {
#include "sub_main/004AD048.inc"
};
const char D_004AD058[13] = {
#include "sub_main/004AD058.inc"
};
const char D_004AD068[11] = {
#include "sub_main/004AD068.inc"
};
const char D_004AD078[13] = {
#include "sub_main/004AD078.inc"
};
const char D_004AD088[11] = {
#include "sub_main/004AD088.inc"
};
const char D_004AD098[13] = {
#include "sub_main/004AD098.inc"
};
const char D_004AD0A8[11] = {
#include "sub_main/004AD0A8.inc"
};
const char D_004AD0B8[11] = {
#include "sub_main/004AD0B8.inc"
};
const char D_004AD0C8[11] = {
#include "sub_main/004AD0C8.inc"
};
const char D_004AD0D8[11] = {
#include "sub_main/004AD0D8.inc"
};
const char D_004AD0E8[13] = {
#include "sub_main/004AD0E8.inc"
};
const char D_004AD0F8[13] = {
#include "sub_main/004AD0F8.inc"
};
const char D_004AD108[13] = {
#include "sub_main/004AD108.inc"
};
const char D_004AD118[13] = {
#include "sub_main/004AD118.inc"
};
const char D_004AD128[13] = {
#include "sub_main/004AD128.inc"
};
const char D_004AD138[13] = {
#include "sub_main/004AD138.inc"
};
const char D_004AD148[13] = {
#include "sub_main/004AD148.inc"
};
const char D_004AD158[9] = {
#include "sub_main/004AD158.inc"
};
const char D_004AD168[9] = {
#include "sub_main/004AD168.inc"
};
const char D_004AD178[7] = {
#include "sub_main/004AD178.inc"
};
const char D_004AD180[11] = {
#include "sub_main/004AD180.inc"
};
const char D_004AD190[11] = {
#include "sub_main/004AD190.inc"
};
const char D_004AD1A0[9] = {
#include "sub_main/004AD1A0.inc"
};
const char D_004AD1B0[11] = {
#include "sub_main/004AD1B0.inc"
};
const char D_004AD1C0[11] = {
#include "sub_main/004AD1C0.inc"
};
const char D_004AD1D0[11] = {
#include "sub_main/004AD1D0.inc"
};
const char D_004AD1E0[11] = {
#include "sub_main/004AD1E0.inc"
};
const char D_004AD1F0[11] = {
#include "sub_main/004AD1F0.inc"
};
const char D_004AD200[9] = {
#include "sub_main/004AD200.inc"
};
const char D_004AD210[9] = {
#include "sub_main/004AD210.inc"
};
const char D_004AD220[9] = {
#include "sub_main/004AD220.inc"
};
const char D_004AD230[11] = {
#include "sub_main/004AD230.inc"
};
const char D_004AD240[11] = {
#include "sub_main/004AD240.inc"
};
const char D_004AD250[11] = {
#include "sub_main/004AD250.inc"
};
const char D_004AD260[9] = {
#include "sub_main/004AD260.inc"
};
const char D_004AD270[11] = {
#include "sub_main/004AD270.inc"
};
const char D_004AD280[11] = {
#include "sub_main/004AD280.inc"
};
const char D_004AD290[11] = {
#include "sub_main/004AD290.inc"
};
const char D_004AD2A0[9] = {
#include "sub_main/004AD2A0.inc"
};
const char D_004AD2B0[11] = {
#include "sub_main/004AD2B0.inc"
};
const char D_004AD2C0[9] = {
#include "sub_main/004AD2C0.inc"
};
const char D_004AD2D0[11] = {
#include "sub_main/004AD2D0.inc"
};
const char D_004AD2E0[9] = {
#include "sub_main/004AD2E0.inc"
};
const char D_004AD2F0[11] = {
#include "sub_main/004AD2F0.inc"
};
const char D_004AD300[11] = {
#include "sub_main/004AD300.inc"
};
const char D_004AD310[11] = {
#include "sub_main/004AD310.inc"
};
const char D_004AD320[11] = {
#include "sub_main/004AD320.inc"
};
const char D_004AD330[11] = {
#include "sub_main/004AD330.inc"
};
const char D_004AD340[9] = {
#include "sub_main/004AD340.inc"
};
const char D_004AD350[11] = {
#include "sub_main/004AD350.inc"
};
const char D_004AD360[9] = {
#include "sub_main/004AD360.inc"
};
const char D_004AD370[11] = {
#include "sub_main/004AD370.inc"
};
const char D_004AD380[9] = {
#include "sub_main/004AD380.inc"
};
const char D_004AD390[11] = {
#include "sub_main/004AD390.inc"
};
const char D_004AD3A0[11] = {
#include "sub_main/004AD3A0.inc"
};
const char D_004AD3B0[11] = {
#include "sub_main/004AD3B0.inc"
};
const char D_004AD3C0[9] = {
#include "sub_main/004AD3C0.inc"
};
const char D_004AD3D0[9] = {
#include "sub_main/004AD3D0.inc"
};
const char D_004AD3E0[11] = {
#include "sub_main/004AD3E0.inc"
};
const char D_004AD3F0[11] = {
#include "sub_main/004AD3F0.inc"
};
const char D_004AD400[9] = {
#include "sub_main/004AD400.inc"
};
const char D_004AD410[11] = {
#include "sub_main/004AD410.inc"
};
const char D_004AD420[13] = {
#include "sub_main/004AD420.inc"
};
const char D_004AD430[17] = {
#include "sub_main/004AD430.inc"
};
const char D_004AD448[13] = {
#include "sub_main/004AD448.inc"
};
const char D_004AD458[13] = {
#include "sub_main/004AD458.inc"
};
const char D_004AD468[11] = {
#include "sub_main/004AD468.inc"
};
const char D_004AD478[13] = {
#include "sub_main/004AD478.inc"
};
const char D_004AD488[13] = {
#include "sub_main/004AD488.inc"
};
const char D_004AD4A0[17] = {
#include "sub_main/004AD4A0.inc"
};
const char D_004AD4B8[13] = {
#include "sub_main/004AD4B8.inc"
};
const char D_004AD4C8[13] = {
#include "sub_main/004AD4C8.inc"
};
const char D_004AD4D8[11] = {
#include "sub_main/004AD4D8.inc"
};
const char D_004AD4E8[13] = {
#include "sub_main/004AD4E8.inc"
};
const char D_004AD4F8[13] = {
#include "sub_main/004AD4F8.inc"
};
const char D_004AD508[11] = {
#include "sub_main/004AD508.inc"
};
const char D_004AD518[13] = {
#include "sub_main/004AD518.inc"
};
const char D_004AD528[5] = {
#include "sub_main/004AD528.inc"
};
const char D_004AD530[5] = {
#include "sub_main/004AD530.inc"
};
const char D_004AD538[9] = {
#include "sub_main/004AD538.inc"
};
const char D_004AD548[11] = {
#include "sub_main/004AD548.inc"
};
const char D_004AD558[11] = {
#include "sub_main/004AD558.inc"
};
const char D_004AD568[9] = {
#include "sub_main/004AD568.inc"
};
const char D_004AD578[11] = {
#include "sub_main/004AD578.inc"
};
const char D_004AD588[13] = {
#include "sub_main/004AD588.inc"
};
const char D_004AD598[11] = {
#include "sub_main/004AD598.inc"
};
const char D_004AD5A8[15] = {
#include "sub_main/004AD5A8.inc"
};
const char D_004AD5B8[11] = {
#include "sub_main/004AD5B8.inc"
};
const char D_004AD5C8[13] = {
#include "sub_main/004AD5C8.inc"
};
const char D_004AD5E0[17] = {
#include "sub_main/004AD5E0.inc"
};
const char D_004AD5F8[11] = {
#include "sub_main/004AD5F8.inc"
};
const char D_004AD608[13] = {
#include "sub_main/004AD608.inc"
};
const char D_004AD618[9] = {
#include "sub_main/004AD618.inc"
};
const char D_004AD628[13] = {
#include "sub_main/004AD628.inc"
};
const char D_004AD638[9] = {
#include "sub_main/004AD638.inc"
};
const char D_004AD648[11] = {
#include "sub_main/004AD648.inc"
};
const char D_004AD658[9] = {
#include "sub_main/004AD658.inc"
};
const char D_004AD668[15] = {
#include "sub_main/004AD668.inc"
};
const char D_004AD678[15] = {
#include "sub_main/004AD678.inc"
};
const char D_004AD688[15] = {
#include "sub_main/004AD688.inc"
};
const char D_004AD698[15] = {
#include "sub_main/004AD698.inc"
};
const char D_004AD6A8[9] = {
#include "sub_main/004AD6A8.inc"
};
const char D_004AD6B8[15] = {
#include "sub_main/004AD6B8.inc"
};
const char D_004AD6C8[15] = {
#include "sub_main/004AD6C8.inc"
};
const char D_004AD6D8[15] = {
#include "sub_main/004AD6D8.inc"
};
const char D_004AD6E8[15] = {
#include "sub_main/004AD6E8.inc"
};
const char D_004AD700[17] = {
#include "sub_main/004AD700.inc"
};
const char D_004AD720[17] = {
#include "sub_main/004AD720.inc"
};
const char D_004AD740[17] = {
#include "sub_main/004AD740.inc"
};
const char D_004AD760[17] = {
#include "sub_main/004AD760.inc"
};
const char D_004AD778[11] = {
#include "sub_main/004AD778.inc"
};
const char D_004AD790[17] = {
#include "sub_main/004AD790.inc"
};
const char D_004AD7A8[11] = {
#include "sub_main/004AD7A8.inc"
};
const char D_004AD7B8[13] = {
#include "sub_main/004AD7B8.inc"
};
const char D_004AD7C8[11] = {
#include "sub_main/004AD7C8.inc"
};
const char D_004AD7D8[11] = {
#include "sub_main/004AD7D8.inc"
};
const char D_004AD7F0[19] = {
#include "sub_main/004AD7F0.inc"
};
const char D_004AD810[19] = {
#include "sub_main/004AD810.inc"
};
const char D_004AD830[19] = {
#include "sub_main/004AD830.inc"
};
const char D_004AD848[15] = {
#include "sub_main/004AD848.inc"
};
const char D_004AD858[15] = {
#include "sub_main/004AD858.inc"
};
const char D_004AD868[11] = {
#include "sub_main/004AD868.inc"
};
const char D_004AD880[19] = {
#include "sub_main/004AD880.inc"
};
const char D_004AD8A0[19] = {
#include "sub_main/004AD8A0.inc"
};
const char D_004AD8C0[19] = {
#include "sub_main/004AD8C0.inc"
};
const char D_004AD8D8[11] = {
#include "sub_main/004AD8D8.inc"
};
const char D_004AD8F0[17] = {
#include "sub_main/004AD8F0.inc"
};
const char D_004AD908[15] = {
#include "sub_main/004AD908.inc"
};
const char D_004AD918[11] = {
#include "sub_main/004AD918.inc"
};
const char D_004AD930[19] = {
#include "sub_main/004AD930.inc"
};
const char D_004AD948[15] = {
#include "sub_main/004AD948.inc"
};
const char D_004AD960[21] = {
#include "sub_main/004AD960.inc"
};
const char D_004AD980[21] = {
#include "sub_main/004AD980.inc"
};
const char D_004AD9A0[19] = {
#include "sub_main/004AD9A0.inc"
};
const char D_004AD9B8[15] = {
#include "sub_main/004AD9B8.inc"
};
const char D_004AD9D0[17] = {
#include "sub_main/004AD9D0.inc"
};
const char D_004AD9E8[11] = {
#include "sub_main/004AD9E8.inc"
};
const char D_004AD9F8[11] = {
#include "sub_main/004AD9F8.inc"
};
const char D_004ADA08[5] = {
#include "sub_main/004ADA08.inc"
};
const char D_004ADA10[7] = {
#include "sub_main/004ADA10.inc"
};
const char D_004ADA18[9] = {
#include "sub_main/004ADA18.inc"
};
const char D_004ADA30[17] = {
#include "sub_main/004ADA30.inc"
};
const char D_004ADA48[9] = {
#include "sub_main/004ADA48.inc"
};
const char D_004ADA58[13] = {
#include "sub_main/004ADA58.inc"
};
const char D_004ADA68[15] = {
#include "sub_main/004ADA68.inc"
};
const char D_004ADA78[13] = {
#include "sub_main/004ADA78.inc"
};
const char D_004ADA90[17] = {
#include "sub_main/004ADA90.inc"
};
const char D_004ADAB0[17] = {
#include "sub_main/004ADAB0.inc"
};
const char D_004ADAC8[13] = {
#include "sub_main/004ADAC8.inc"
};
const char D_004ADAD8[9] = {
#include "sub_main/004ADAD8.inc"
};
const char D_004ADAE8[11] = {
#include "sub_main/004ADAE8.inc"
};
const char D_004ADAF8[9] = {
#include "sub_main/004ADAF8.inc"
};
const char D_004ADB08[11] = {
#include "sub_main/004ADB08.inc"
};
const char D_004ADB18[11] = {
#include "sub_main/004ADB18.inc"
};
const char D_004ADB28[13] = {
#include "sub_main/004ADB28.inc"
};
const char D_004ADB38[13] = {
#include "sub_main/004ADB38.inc"
};
const char D_004ADB48[7] = {
#include "sub_main/004ADB48.inc"
};
const char D_004ADB50[15] = {
#include "sub_main/004ADB50.inc"
};
const char D_004ADB60[15] = {
#include "sub_main/004ADB60.inc"
};
const char D_004ADB70[15] = {
#include "sub_main/004ADB70.inc"
};
const char D_004ADB80[9] = {
#include "sub_main/004ADB80.inc"
};
const char D_004ADB90[13] = {
#include "sub_main/004ADB90.inc"
};
const char D_004ADBA0[11] = {
#include "sub_main/004ADBA0.inc"
};
const char D_004ADBB0[11] = {
#include "sub_main/004ADBB0.inc"
};
const char D_004ADBC0[9] = {
#include "sub_main/004ADBC0.inc"
};
const char D_004ADBD0[15] = {
#include "sub_main/004ADBD0.inc"
};
const char D_004ADBE0[13] = {
#include "sub_main/004ADBE0.inc"
};
const char D_004ADBF0[13] = {
#include "sub_main/004ADBF0.inc"
};
const char D_004ADC00[13] = {
#include "sub_main/004ADC00.inc"
};
const char D_004ADC10[7] = {
#include "sub_main/004ADC10.inc"
};
const char D_004ADC18[7] = {
#include "sub_main/004ADC18.inc"
};
const char D_004ADC20[7] = {
#include "sub_main/004ADC20.inc"
};
const char D_004ADC28[7] = {
#include "sub_main/004ADC28.inc"
};
const char D_004ADC30[7] = {
#include "sub_main/004ADC30.inc"
};
const char D_004ADC38[7] = {
#include "sub_main/004ADC38.inc"
};
const char D_004ADC40[7] = {
#include "sub_main/004ADC40.inc"
};
const char D_004ADC48[11] = {
#include "sub_main/004ADC48.inc"
};
const char D_004ADC58[15] = {
#include "sub_main/004ADC58.inc"
};
const char D_004ADC68[13] = {
#include "sub_main/004ADC68.inc"
};
const char D_004ADC78[7] = {
#include "sub_main/004ADC78.inc"
};
const char D_004ADC80[7] = {
#include "sub_main/004ADC80.inc"
};
const char D_004ADC88[11] = {
#include "sub_main/004ADC88.inc"
};
const char D_004ADC98[13] = {
#include "sub_main/004ADC98.inc"
};
const char D_004ADCA8[13] = {
#include "sub_main/004ADCA8.inc"
};
const char D_004ADCB8[11] = {
#include "sub_main/004ADCB8.inc"
};
const char D_004ADCC8[13] = {
#include "sub_main/004ADCC8.inc"
};
const char D_004ADCE0[17] = {
#include "sub_main/004ADCE0.inc"
};
const char D_004ADCF8[15] = {
#include "sub_main/004ADCF8.inc"
};
const char D_004ADD08[15] = {
#include "sub_main/004ADD08.inc"
};
const char D_004ADD18[15] = {
#include "sub_main/004ADD18.inc"
};
const char D_004ADD28[13] = {
#include "sub_main/004ADD28.inc"
};
const char D_004ADD38[13] = {
#include "sub_main/004ADD38.inc"
};
const char D_004ADD48[13] = {
#include "sub_main/004ADD48.inc"
};
const char D_004ADD58[13] = {
#include "sub_main/004ADD58.inc"
};
const char D_004ADD68[13] = {
#include "sub_main/004ADD68.inc"
};
const char D_004ADD78[13] = {
#include "sub_main/004ADD78.inc"
};
const char D_004ADD88[13] = {
#include "sub_main/004ADD88.inc"
};
const char D_004ADD98[13] = {
#include "sub_main/004ADD98.inc"
};
const char D_004ADDA8[13] = {
#include "sub_main/004ADDA8.inc"
};
const char D_004ADDB8[13] = {
#include "sub_main/004ADDB8.inc"
};
const char D_004ADDD0[17] = {
#include "sub_main/004ADDD0.inc"
};
const char D_004ADDE8[13] = {
#include "sub_main/004ADDE8.inc"
};
const char D_004ADE00[17] = {
#include "sub_main/004ADE00.inc"
};
const char D_004ADE18[11] = {
#include "sub_main/004ADE18.inc"
};
const char D_004ADE28[9] = {
#include "sub_main/004ADE28.inc"
};
const char D_004ADE38[9] = {
#include "sub_main/004ADE38.inc"
};
const char D_004ADE48[9] = {
#include "sub_main/004ADE48.inc"
};
const char D_004ADE58[9] = {
#include "sub_main/004ADE58.inc"
};
const char D_004ADE70[17] = {
#include "sub_main/004ADE70.inc"
};
const char D_004ADE90[17] = {
#include "sub_main/004ADE90.inc"
};
const char D_004ADEB0[17] = {
#include "sub_main/004ADEB0.inc"
};
const char D_004ADEC8[9] = {
#include "sub_main/004ADEC8.inc"
};
const char D_004ADED8[15] = {
#include "sub_main/004ADED8.inc"
};
const char D_004ADEE8[15] = {
#include "sub_main/004ADEE8.inc"
};
const char D_004ADEF8[15] = {
#include "sub_main/004ADEF8.inc"
};
const char D_004ADF08[11] = {
#include "sub_main/004ADF08.inc"
};
const char D_004ADF18[5] = {
#include "sub_main/004ADF18.inc"
};
const char D_004ADF20[13] = {
#include "sub_main/004ADF20.inc"
};
const char D_004ADF30[13] = {
#include "sub_main/004ADF30.inc"
};
const char D_004ADF40[9] = {
#include "sub_main/004ADF40.inc"
};
const char D_004ADF50[9] = {
#include "sub_main/004ADF50.inc"
};
const char D_004ADF60[13] = {
#include "sub_main/004ADF60.inc"
};
const char D_004ADF70[9] = {
#include "sub_main/004ADF70.inc"
};
const char D_004ADF80[7] = {
#include "sub_main/004ADF80.inc"
};
const char D_004ADF88[9] = {
#include "sub_main/004ADF88.inc"
};
const char D_004ADF98[13] = {
#include "sub_main/004ADF98.inc"
};
const char D_004ADFA8[15] = {
#include "sub_main/004ADFA8.inc"
};
const char D_004ADFB8[9] = {
#include "sub_main/004ADFB8.inc"
};
const char D_004ADFD0[17] = {
#include "sub_main/004ADFD0.inc"
};
const char D_004ADFE8[13] = {
#include "sub_main/004ADFE8.inc"
};
const char D_004AE000[17] = {
#include "sub_main/004AE000.inc"
};
const char D_004AE018[15] = {
#include "sub_main/004AE018.inc"
};
const char D_004AE028[15] = {
#include "sub_main/004AE028.inc"
};
const char D_004AE040[17] = {
#include "sub_main/004AE040.inc"
};
const char D_004AE058[15] = {
#include "sub_main/004AE058.inc"
};
const char D_004AE070[17] = {
#include "sub_main/004AE070.inc"
};
const char D_004AE090[17] = {
#include "sub_main/004AE090.inc"
};
const char D_004AE0B0[17] = {
#include "sub_main/004AE0B0.inc"
};
const char D_004AE0C8[15] = {
#include "sub_main/004AE0C8.inc"
};
const char D_004AE0D8[13] = {
#include "sub_main/004AE0D8.inc"
};
const char D_004AE0F0[17] = {
#include "sub_main/004AE0F0.inc"
};
const char D_004AE110[17] = {
#include "sub_main/004AE110.inc"
};
const char D_004AE130[17] = {
#include "sub_main/004AE130.inc"
};
const char D_004AE150[17] = {
#include "sub_main/004AE150.inc"
};
const char D_004AE168[9] = {
#include "sub_main/004AE168.inc"
};
const char D_004AE178[9] = {
#include "sub_main/004AE178.inc"
};
const char D_004AE188[9] = {
#include "sub_main/004AE188.inc"
};
const char D_004AE198[9] = {
#include "sub_main/004AE198.inc"
};
const char D_004AE1A8[7] = {
#include "sub_main/004AE1A8.inc"
};
const char D_004AE1B0[9] = {
#include "sub_main/004AE1B0.inc"
};
const char D_004AE1C0[7] = {
#include "sub_main/004AE1C0.inc"
};
const char D_004AE1C8[7] = {
#include "sub_main/004AE1C8.inc"
};
const char D_004AE1D0[7] = {
#include "sub_main/004AE1D0.inc"
};
const char D_004AE1D8[7] = {
#include "sub_main/004AE1D8.inc"
};
const char D_004AE1E0[7] = {
#include "sub_main/004AE1E0.inc"
};
const char D_004AE1E8[7] = {
#include "sub_main/004AE1E8.inc"
};
const char D_004AE1F0[7] = {
#include "sub_main/004AE1F0.inc"
};
const char D_004AE1F8[7] = {
#include "sub_main/004AE1F8.inc"
};
const char D_004AE200[7] = {
#include "sub_main/004AE200.inc"
};
const char D_004AE208[7] = {
#include "sub_main/004AE208.inc"
};
const char D_004AE210[7] = {
#include "sub_main/004AE210.inc"
};
const char D_004AE218[7] = {
#include "sub_main/004AE218.inc"
};
const char D_004AE220[7] = {
#include "sub_main/004AE220.inc"
};
const char D_004AE228[7] = {
#include "sub_main/004AE228.inc"
};
const char D_004AE230[7] = {
#include "sub_main/004AE230.inc"
};
const char D_004AE238[7] = {
#include "sub_main/004AE238.inc"
};
const char D_004AE240[7] = {
#include "sub_main/004AE240.inc"
};
const char D_004AE248[7] = {
#include "sub_main/004AE248.inc"
};
const char D_004AE250[7] = {
#include "sub_main/004AE250.inc"
};
const char D_004AE258[7] = {
#include "sub_main/004AE258.inc"
};
const char D_004AE260[7] = {
#include "sub_main/004AE260.inc"
};
const char D_004AE268[7] = {
#include "sub_main/004AE268.inc"
};
const char D_004AE270[7] = {
#include "sub_main/004AE270.inc"
};
const char D_004AE278[7] = {
#include "sub_main/004AE278.inc"
};
const char D_004AE280[7] = {
#include "sub_main/004AE280.inc"
};
const char D_004AE288[7] = {
#include "sub_main/004AE288.inc"
};
const char D_004AE290[7] = {
#include "sub_main/004AE290.inc"
};
const char D_004AE298[5] = {
#include "sub_main/004AE298.inc"
};
const char D_004AE2A0[7] = {
#include "sub_main/004AE2A0.inc"
};
const char D_004AE2A8[7] = {
#include "sub_main/004AE2A8.inc"
};
const char D_004AE2B0[7] = {
#include "sub_main/004AE2B0.inc"
};
const char D_004AE2B8[7] = {
#include "sub_main/004AE2B8.inc"
};
const char D_004AE2C0[7] = {
#include "sub_main/004AE2C0.inc"
};
const char D_004AE2C8[7] = {
#include "sub_main/004AE2C8.inc"
};
const char D_004AE2D0[7] = {
#include "sub_main/004AE2D0.inc"
};
const char D_004AE2D8[7] = {
#include "sub_main/004AE2D8.inc"
};
const char D_004AE2E0[7] = {
#include "sub_main/004AE2E0.inc"
};
const char D_004AE2E8[7] = {
#include "sub_main/004AE2E8.inc"
};
const char D_004AE2F0[7] = {
#include "sub_main/004AE2F0.inc"
};
const char D_004AE2F8[7] = {
#include "sub_main/004AE2F8.inc"
};
const char D_004AE300[7] = {
#include "sub_main/004AE300.inc"
};
const char D_004AE308[7] = {
#include "sub_main/004AE308.inc"
};
const char D_004AE310[7] = {
#include "sub_main/004AE310.inc"
};
const char D_004AE318[7] = {
#include "sub_main/004AE318.inc"
};
const char D_004AE320[7] = {
#include "sub_main/004AE320.inc"
};
const char D_004AE328[7] = {
#include "sub_main/004AE328.inc"
};
const char D_004AE330[7] = {
#include "sub_main/004AE330.inc"
};
const char D_004AE338[7] = {
#include "sub_main/004AE338.inc"
};
const char D_004AE340[7] = {
#include "sub_main/004AE340.inc"
};
const char D_004AE348[7] = {
#include "sub_main/004AE348.inc"
};
const char D_004AE350[7] = {
#include "sub_main/004AE350.inc"
};
const char D_004AE358[7] = {
#include "sub_main/004AE358.inc"
};
const char D_004AE360[7] = {
#include "sub_main/004AE360.inc"
};
const char D_004AE368[7] = {
#include "sub_main/004AE368.inc"
};
const char D_004AE370[7] = {
#include "sub_main/004AE370.inc"
};
const char D_004AE378[7] = {
#include "sub_main/004AE378.inc"
};
const char D_004AE380[5] = {
#include "sub_main/004AE380.inc"
};
const char D_004AE388[7] = {
#include "sub_main/004AE388.inc"
};
const char D_004AE390[7] = {
#include "sub_main/004AE390.inc"
};
const char D_004AE398[7] = {
#include "sub_main/004AE398.inc"
};
const char D_004AE3A0[7] = {
#include "sub_main/004AE3A0.inc"
};
const char D_004AE3A8[7] = {
#include "sub_main/004AE3A8.inc"
};
const char D_004AE3B0[7] = {
#include "sub_main/004AE3B0.inc"
};
const char D_004AE3B8[5] = {
#include "sub_main/004AE3B8.inc"
};
const char D_004AE3C0[7] = {
#include "sub_main/004AE3C0.inc"
};
const char D_004AE3C8[7] = {
#include "sub_main/004AE3C8.inc"
};
const char D_004AE3D0[5] = {
#include "sub_main/004AE3D0.inc"
};
const char D_004AE3D8[5] = {
#include "sub_main/004AE3D8.inc"
};
const char D_004AE3E0[7] = {
#include "sub_main/004AE3E0.inc"
};
const char D_004AE3E8[7] = {
#include "sub_main/004AE3E8.inc"
};
const char D_004AE3F0[7] = {
#include "sub_main/004AE3F0.inc"
};
const char D_004AE3F8[7] = {
#include "sub_main/004AE3F8.inc"
};
const char D_004AE400[7] = {
#include "sub_main/004AE400.inc"
};
const char D_004AE408[7] = {
#include "sub_main/004AE408.inc"
};
const char D_004AE410[9] = {
#include "sub_main/004AE410.inc"
};
const char D_004AE420[7] = {
#include "sub_main/004AE420.inc"
};
const char D_004AE428[7] = {
#include "sub_main/004AE428.inc"
};
const char D_004AE430[7] = {
#include "sub_main/004AE430.inc"
};
const char D_004AE438[7] = {
#include "sub_main/004AE438.inc"
};
const char D_004AE440[7] = {
#include "sub_main/004AE440.inc"
};
const char D_004AE448[7] = {
#include "sub_main/004AE448.inc"
};
const char D_004AE450[7] = {
#include "sub_main/004AE450.inc"
};
const char D_004AE458[7] = {
#include "sub_main/004AE458.inc"
};
const char D_004AE460[7] = {
#include "sub_main/004AE460.inc"
};
const char D_004AE468[7] = {
#include "sub_main/004AE468.inc"
};
const char D_004AE470[7] = {
#include "sub_main/004AE470.inc"
};
const char D_004AE478[7] = {
#include "sub_main/004AE478.inc"
};
const char D_004AE480[7] = {
#include "sub_main/004AE480.inc"
};
const char D_004AE488[7] = {
#include "sub_main/004AE488.inc"
};
const char D_004AE490[7] = {
#include "sub_main/004AE490.inc"
};
const char D_004AE498[7] = {
#include "sub_main/004AE498.inc"
};
const char D_004AE4A0[7] = {
#include "sub_main/004AE4A0.inc"
};
const char D_004AE4A8[7] = {
#include "sub_main/004AE4A8.inc"
};
const char D_004AE4B0[7] = {
#include "sub_main/004AE4B0.inc"
};
const char D_004AE4B8[7] = {
#include "sub_main/004AE4B8.inc"
};
const char D_004AE4C0[7] = {
#include "sub_main/004AE4C0.inc"
};
const char D_004AE4C8[7] = {
#include "sub_main/004AE4C8.inc"
};
const char D_004AE4D0[7] = {
#include "sub_main/004AE4D0.inc"
};
const char D_004AE4D8[7] = {
#include "sub_main/004AE4D8.inc"
};
const char D_004AE4E0[7] = {
#include "sub_main/004AE4E0.inc"
};
const char D_004AE4E8[7] = {
#include "sub_main/004AE4E8.inc"
};
const char D_004AE4F0[7] = {
#include "sub_main/004AE4F0.inc"
};
const char D_004AE4F8[7] = {
#include "sub_main/004AE4F8.inc"
};
const char D_004AE500[7] = {
#include "sub_main/004AE500.inc"
};
const char D_004AE508[7] = {
#include "sub_main/004AE508.inc"
};
const char D_004AE510[7] = {
#include "sub_main/004AE510.inc"
};
const char D_004AE518[7] = {
#include "sub_main/004AE518.inc"
};
const char D_004AE520[7] = {
#include "sub_main/004AE520.inc"
};
const char D_004AE528[5] = {
#include "sub_main/004AE528.inc"
};
const char D_004AE530[7] = {
#include "sub_main/004AE530.inc"
};
const char D_004AE538[7] = {
#include "sub_main/004AE538.inc"
};
const char D_004AE540[7] = {
#include "sub_main/004AE540.inc"
};
const char D_004AE548[7] = {
#include "sub_main/004AE548.inc"
};
const char D_004AE550[7] = {
#include "sub_main/004AE550.inc"
};
const char D_004AE558[7] = {
#include "sub_main/004AE558.inc"
};
const char D_004AE560[7] = {
#include "sub_main/004AE560.inc"
};
const char D_004AE568[7] = {
#include "sub_main/004AE568.inc"
};
const char D_004AE570[7] = {
#include "sub_main/004AE570.inc"
};
const char D_004AE578[7] = {
#include "sub_main/004AE578.inc"
};
const char D_004AE580[7] = {
#include "sub_main/004AE580.inc"
};
const char D_004AE588[7] = {
#include "sub_main/004AE588.inc"
};

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
