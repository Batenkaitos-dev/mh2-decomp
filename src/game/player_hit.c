/*
 * The player's reaction to the hits it takes.
 *
 * Player_ReactToHit runs for a player with a hit pending (Player.hitReceived,
 * set by ObjAttach_DamagePlayer in sub_main): a hit the guard takes
 * (Player_CanGuardHit) goes to Player_GuardHit, any other picks a state of
 * PL_GROUP_REACT from the attack's kind (enum AttackKind, objattach.h), its
 * force and the side it came from, after the skills that block or soften
 * it; the statuses the hit carries (poison, the buildups) are applied here.
 * The small Player_* predicates below are shared with the player's states
 * and sub_main.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "item.h"
#include "bgm.h"
#include "card.h"
#include "quest.h"
#include "sound.h"
#include "bone_effects.h"
#include "dust_flash_effects.h"
#include "hit_effects.h"
#include "rand.h"
#include "vibration.h"
#include "save.h"

/* prototypes */
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
void Player_ClearBuffs(Player *pl);
/* unit's own declaration: declared with s32 group and state; with the definition's u8 parameters and s16 result Player_InStatus4State compiles differently */
s32 Player_IsState(void *obj, s32 group, s32 state);
/* end of prototypes */

/* 1 when the status 5 buildup is full (50). */
s32 Player_Status5Full(Player *pl)
{
    return (pl->status5Buildup >= 50) & 0xFF;
}

/* 1 while the player has a buildup toward the mash status. */
s32 Player_HasMashBuildup(Player *pl)
{
    return (pl->mashBuildup >= 1) & 0xFF;
}

/* 1 while the player builds up to, suffers or shakes off the mash status:
 * a buildup or presses left, or one of the damage states of the mash. */
s32 Player_InMashStatus(Player *pl)
{
    if (pl->mashBuildup != 0 || pl->mashLeft != 0) {
        return 1;
    }
    if (pl->stateGroup == PL_GROUP_REACT) {
        switch (pl->state) {
        case 27:
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
            return 1;
        }
    }
    return 0;
}

/* Ends the mash status. If presses were still needed, it first makes stage
 * object STAGEOBJ_KIND_MASH_DEBRIS (mode 0) for the player, as the mash states do. */
void Player_EndMash(Player *pl)
{
    if (pl->mashLeft > 0) {
        MashDebris_Create(0, (s16 *) pl);
    }
    pl->mashLeft = 0;
}

/*
 * Adds `amount` to the player's poison, as the skills allow:
 * SKILL_POISON_IMMUNE clears it instead, SKILL_POISON_HALF halves the
 * amount, SKILL_POISON_DOUBLE doubles it, and SKILL_STATUS6_HALF or
 * SKILL_STATUS6_IMMUNE halve it again. The poison then ticks every 60
 * frames with SKILL_POISON_HALF, every 15 with SKILL_POISON_DOUBLE, else
 * every 30.
 */
void Player_AddPoison(Player *pl, s16 amount)
{
    if (Player_HasSkill(pl, SKILL_POISON_IMMUNE) == 1) {
        pl->poison = 0;
        return;
    }
    if (Player_HasSkill(pl, SKILL_POISON_HALF) == 1) {
        amount /= 2;
    } else if (Player_HasSkill(pl, SKILL_POISON_DOUBLE) == 1) {
        amount *= 2;
    }
    if (Player_HasSkill(pl, SKILL_STATUS6_HALF) == 1 || Player_HasSkill(pl, SKILL_STATUS6_IMMUNE) == 1) {
        amount /= 2;
    }
    pl->poison += amount;
    if (Player_HasSkill(pl, SKILL_POISON_HALF) == 1) {
        pl->poisonTimer = 60;
    } else if (Player_HasSkill(pl, SKILL_POISON_DOUBLE) == 1) {
        pl->poisonTimer = 15;
    } else {
        pl->poisonTimer = 30;
    }
}

/* 1 while the player is down in status 4, or hit while down. */
s32 Player_InStatus4State(Player *pl)
{
    if ((s16) Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS4) || (s16) Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS4_HIT)) {
        return 1;
    }
    return 0;
}

/* 1 while the player is blown away or tossed by a hit. */
s32 Player_InBlownAway(Player *pl)
{
    if (pl->stateGroup != PL_GROUP_REACT) {
        return 0;
    }
    switch (pl->state) {
    case 2:
    case 5:
    case 7:
    case 14:
        return 1;
    }
    return 0;
}

/* Makes the player faint (PL_FAINT, sent to the others), unless it already
 * has: health, buffs, aim and the area move are cleared. */
void Player_Die(Player *pl)
{
    if (pl->stateGroup != PL_GROUP_FAINT) {
        Bgm_StopEvent(pl);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, PLF_GUARD);
        pl->hp = 0;
        pl->hpRecoverable = 0;
        Player_ResetAim(pl);
        Player_ClearBuffs(pl);
        pl->areaMoveState = PL_AREA_MOVE_NONE;
        Player_RequestStateSynced(pl, PL_GROUP_FAINT, PL_FAINT, 0);
    }
}

/* Ends the hit slowdown and the hit stop. */
void Player_ClearHitSlow(Player *pl)
{
    pl->hitSlowTimer = 0;
    pl->hitStop = 0;
}

/* 1 when a hit travelling in direction hitAngle meets a player facing
 * `angle` from the front: angle - hitAngle within 110 degrees of a half turn
 * (16-bit turns: 70 to 290 degrees). */
s32 Player_InGuardArc(u16 angle, u16 hitAngle)
{
    s32 diff;

    diff = ((((angle & 0xFFFF) - (hitAngle & 0xFFFF)) & 0xFFFF) + 0x4000) & 0xFFFF;
    if ((diff <= 3641) || (diff >= 29127)) {
        return 1;
    }
    return 0;
}

/*
 * 1 when the guard takes the pending hit: the player has 75 stamina, the hit
 * can be guarded (attribute 0x80 needs SKILL_GUARD_PLUS20, and never with the heavy
 * bowgun), the state allows the guard (Player_CanGuard), the weapon guards
 * (great sword, sword and shield, lance, gunlance, heavy bowgun) and is
 * drawn, the player stands or crouches, guards (flag 0x8000, or the auto-guard
 * SKILL_AUTO_GUARD or the heavy bowgun's shield when it was free last frame) and
 * faces the hit (Player_InGuardArc).
 */
u8 Player_CanGuardHit(Player *pl)
{
    u8 ret = 0;

    if (pl->stamina < 75) {
        return 0;
    }
    if ((pl->hitAttrs & 0x80) && (Player_HasSkill(pl, SKILL_GUARD_PLUS20) == 0 || pl->weaponType == WEAPON_HEAVY_BOWGUN)) {
        return 0;
    }
    if (Player_CanGuard(pl) == 0) {
        return 0;
    }
    switch (pl->weaponType) {
    case WEAPON_GREAT_SWORD:
    case WEAPON_SWORD_SHIELD:
    case WEAPON_LANCE:
    case WEAPON_GUNLANCE:
    case WEAPON_HEAVY_BOWGUN:
        if (pl->weaponOut != 0) {
            switch (pl->action) {
            case PL_POSTURE_STANDING:
            case PL_POSTURE_CROUCHED:
                if (((Player_HasSkill(pl, SKILL_AUTO_GUARD) == 1 || HeavyBowgun_HasShield(pl) == 1) && pl->prevFreeState != 0)
                    || Player_TestFlag(pl, PLF_GUARD) != 0) {
                    if (Player_InGuardArc(*(u16 *) &pl->rot[1], pl->hitAngle) == 1) {
                        ret = 1;
                    }
                }
                break;
            }
        }
        break;
    }
    return ret;
}

/*
 * Reacts to a hit the guard took: its force (Player.hitForce), raised or
 * lowered by the guard skills SKILL_GUARD_PLUS10 to SKILL_GUARD_MINUS10, picks the reaction (a push, a
 * stagger or a knockback, by weapon) and the stamina and sharpness the
 * guard costs. Returns the reaction: 0 a push, 1 a stagger, 2 a knockback.
 * The great sword's test of lastAttackKind is always true (!= one kind or
 * != another), as the original has it.
 */
u8 Player_GuardHit(Player *pl)
{
    s16 dull;
    u8 force;
    u8 react;

    dull = 0;
    force = pl->hitForce;
    react = 0;
    if (Player_HasSkill(pl, SKILL_GUARD_PLUS20) == 1 && pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        if (force > 21) {
            force -= 20;
        } else {
            force = 1;
        }
    } else if (Player_HasSkill(pl, SKILL_GUARD_PLUS10) == 1 && pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        if (force > 11) {
            force -= 10;
        } else {
            force = 1;
        }
    } else if (Player_HasSkill(pl, SKILL_GUARD_MINUS5) == 1 && pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        if (force < 250) {
            force += 5;
        } else {
            force = 255;
        }
    } else if (Player_HasSkill(pl, SKILL_GUARD_MINUS10) == 1 && pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        if (force < 245) {
            force += 10;
        } else {
            force = 255;
        }
    }
    if (force >= 40) {
        react = 2;
        Player_SetState(pl, PL_GROUP_REACT, PL_REACT_GUARD_KNOCKBACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        dull = -10;
        Player_AddStamina(pl, -180);
    } else if (force >= 15) {
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
        case WEAPON_LONG_SWORD:
            react = 1;
            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_GUARD_STAGGER, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            dull = -2;
            break;
        case WEAPON_SWORD_SHIELD:
        case WEAPON_HEAVY_BOWGUN:
            react = 2;
            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_GUARD_KNOCKBACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            dull = -4;
            break;
        case WEAPON_LANCE:
        case WEAPON_GUNLANCE:
            react = 0;
            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_GUARD_PUSH, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            dull = -1;
            break;
        }
        Player_AddStamina(pl, -100);
    } else {
        react = 0;
        Player_SetState(pl, PL_GROUP_REACT, PL_REACT_GUARD_PUSH, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        dull = -1;
        Player_AddStamina(pl, -75);
    }
    if (pl->weaponType == WEAPON_GREAT_SWORD && (pl->lastAttackKind != ATTACK_ROAR || pl->lastAttackKind != ATTACK_ROAR_WEAK || pl->lastAttackKind != ATTACK_ROAR_STRONG)) {
        Player_AddSharpness(pl, dull);
    }
    return react;
}

s32 Player_HasSongEffect(Player *pl, s16 kind);

/*
 * Reacts to the hit the player took this frame (see the file's comment).
 * hitDir is the hit's direction relative to the player's heading plus a
 * quarter turn: below 0x8000 the hit came from behind. knockedDown picks
 * the heavier hit sound; noHitSound (set in the roar, wind and tremor
 * cases) skips the end of the event music and sound 30. The cases share
 * their tails through the gotos (react, turn, done).
 */
void Player_ReactToHit(Player *pl)
{
    f32 pos[3];
    u16 hitDir;
    u8 meal;
    u16 attackKind;
    u8 knockedDown;
    s16 hpBefore;
    u8 noHitSound;

    knockedDown = 0;
    noHitSound = 0;
    if (pl->hitReceived != 0 && (pl->stateGroup != PL_GROUP_FAINT || pl->lastAttackKind == ATTACK_REVIVE)) {
        Player_ClearHitSlow(pl);
        if (Player_CanGuardHit(pl) & 0xFF) {
            Player_GuardHit(pl);
            if (Player_IsLocal(pl) == 1) {
                hpBefore = pl->hp;
                if ((s16) Player_IsState(pl, PL_GROUP_REACT, PL_REACT_GUARD_KNOCKBACK) != 0 || (pl->hitAttrs & 0x80)) {
                    if (pl->hitAttrs & 0x80) {
                        Player_AddHp(pl, -(pl->hitDamage[0] / 2));
                    } else if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                        Player_AddHp(pl, -(pl->hitDamage[0] / 2));
                    } else {
                        Player_AddHp(pl, -(pl->hitDamage[0] / 4));
                    }
                    pl->hpRecoverable = pl->hp + (hpBefore - pl->hp) / 2;
                    if (pl->hp <= 0) {
                        Player_Die(pl);
                        return;
                    }
                }
                if (pl->status6Hit > 0 && Player_HasSkill(pl, SKILL_STATUS6_IMMUNE) == 0) {
                    if (Player_HasSkill(pl, SKILL_STATUS6_HALF) == 1) {
                        pl->status6Timer = 900;
                    } else if (Player_HasSkill(pl, SKILL_STATUS6_DOUBLE) == 1) {
                        pl->status6Timer = 3600;
                    } else {
                        pl->status6Timer = 1800;
                    }
                }
            }
        } else {
            hitDir = (u16) (pl->rot[1] - pl->hitAngle) + ANGLE_TURN / 4;
            if (Player_IsLocal(pl) == 1) {
                if (pl->hitDamage[0] != 0) {
                    hpBefore = pl->hp;
                    if (hpBefore > 0 && (Rand_Next(1) & 3) == 0) {
                        if (Player_HasSkill(pl, SKILL_DAMAGE_CHANCE_GUARD) == 1) {
                            pl->hitDamage[0] = 0.7f * pl->hitDamage[0];
                            Effect06_SpawnAtBone(pl, 2, 25, 10, 4.0f);
                            Em_GetBonePos(pl, 10, pos);
                            Sound_PlayPlayerSe(pl, 114, 0, pos, 1, 0);
                            if (pl->hitDamage[0] == 0) {
                                pl->hitDamage[0] = 1;
                            }
                        } else if (Player_HasSkill(pl, SKILL_DAMAGE_CHANCE_WEAK) == 1) {
                            pl->hitDamage[0] = 1.3f * pl->hitDamage[0];
                            Em_GetBonePos(pl, 10, pos);
                            HitSplash_CreateByHitFx((Em *) pl, pos, 5, 3, 1, 1.0f);
                            Sound_PlayPlayerSe(pl, 115, 0, pos, 1, 0);
                        }
                    }
                    Player_AddHp(pl, -pl->hitDamage[0]);
                    if (pl->hp < hpBefore) {
                        pl->hpRecoverable = pl->hp + (hpBefore - pl->hp) / 2;
                    } else {
                        pl->hpRecoverable = pl->hp;
                    }
                }
                if (pl->status5Hit > 0) {
                    if (Player_HasSkill(pl, SKILL_STATUS5_IMMUNE) == 1) {
                        pl->status5Hit = 0;
                    } else {
                        if (Player_HasSkill(pl, SKILL_STATUS5_HALF) == 1) {
                            pl->status5Hit /= 2;
                        }
                        pl->status5Buildup += pl->status5Hit;
                        pl->status5Delay = 150;
                    }
                }
                if (pl->status2Hit > 0) {
                    if (Player_HasSkill(pl, SKILL_STATUS2_IMMUNE) == 1) {
                        pl->status2Hit = 0;
                    } else {
                        pl->status2Buildup += pl->status2Hit;
                    }
                }
                if (pl->status4Hit > 0) {
                    if (Player_HasSkill(pl, SKILL_STATUS4_IMMUNE) == 1) {
                        pl->status4Hit = 0;
                    } else {
                        pl->status4Buildup += pl->status4Hit;
                    }
                }
                if (pl->poisonHit > 0) {
                    Player_AddPoison(pl, pl->poisonHit);
                }
                if (pl->status6Hit > 0 && Player_HasSkill(pl, SKILL_STATUS6_IMMUNE) == 0) {
                    if (Player_HasSkill(pl, SKILL_STATUS6_HALF) == 1) {
                        pl->status6Timer = 900;
                    } else if (Player_HasSkill(pl, SKILL_STATUS6_DOUBLE) == 1) {
                        pl->status6Timer = 3600;
                    } else {
                        pl->status6Timer = 1800;
                    }
                }
                if ((s16) pl->status7Hit > 0 && Player_HasSkill(pl, SKILL_STATUS7_IMMUNE) == 0) {
                    if (Player_HasSkill(pl, SKILL_STATUS7_HALF) == 1) {
                        pl->status7Timer = 900;
                    } else {
                        pl->status7Timer = 1800;
                    }
                }
                if (pl->status8Hit > 0 && Player_HasSkill(pl, SKILL_STATUS7_IMMUNE) == 0) {
                    if (Player_HasSkill(pl, SKILL_STATUS7_HALF) == 1) {
                        if (pl->staminaMax > 75) {
                            Player_AddMaxStamina(pl, pl->staminaMax / 75 / 2 * -75);
                        }
                    } else {
                        Player_AddMaxStamina(pl, -375);
                    }
                }
                if (pl->mashBuildupHit > 0) {
                    if (Player_HasSkill(pl, SKILL_MASH_IMMUNE) != 0) {
                        pl->mashBuildupHit = 0;
                    }
                    pl->mashBuildup += pl->mashBuildupHit;
                }
                Player_SetFlag(pl, 2);
                Player_ClearFlag(pl, PLF_GUARD);
                if (pl->hp > 0 || pl->lastAttackKind == ATTACK_REVIVE || pl->action == PL_POSTURE_AIRBORNE) {
                    attackKind = pl->lastAttackKind;
                    if (pl->hp <= 0 && pl->lastAttackKind != ATTACK_REVIVE) {
                        attackKind = ATTACK_BLOW;
                    }
                    if (pl->action == PL_POSTURE_AIRBORNE) {
                        switch (attackKind) {
                        case ATTACK_BLOW:
                        case 8:
                        case 11:
                        case 12:
                        case 13:
                        case 24:
                        case ATTACK_INHALE:
                        case ATTACK_TOSS_HIGH:
                            break;
                        case ATTACK_TOSS:
                            attackKind = ATTACK_BLOW;
                            break;
                        default:
                            attackKind = ATTACK_BLOW;
                            break;
                        }
                    }
                    switch (attackKind) {
                    case ATTACK_BLOW:
                    case ATTACK_TOSS:
                    case ATTACK_TOSS_HIGH:
                        if (hitDir < 0x8000) {
                            pl->rot[1] = pl->hitAngle;
                            pl->targetYaw = pl->rot[1];
                            if (attackKind == ATTACK_BLOW) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_BLOWN_FORWARD, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_TOSSED_FORWARD, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                        } else {
                            pl->rot[1] = (u16) (pl->hitAngle + ANGLE_TURN / 2);
                            pl->targetYaw = pl->rot[1];
                            if (attackKind == ATTACK_BLOW) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_BLOWN_BACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_TOSSED_BACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                        }
                        Player_EndMash(pl);
                        knockedDown = 1;
                        pl->action = PL_POSTURE_AIRBORNE;
                        pl->rideState = 0;
                        goto react;
                    case ATTACK_INHALE:
                        switch (pl->stateGroup) {
                        default:
                        case 2:
                        case 0:
                        case 1:
                            break;
                        case 4:
                        case 3:
                            noHitSound = 1;
                            goto done;
                        }
                        Player_EndMash(pl);
                        pl->action = PL_POSTURE_AIRBORNE;
                        pl->rideState = 0;
                        Player_SetState(pl, PL_GROUP_REACT, PL_REACT_INHALED, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                        knockedDown = 1;
                        goto react;
                        break;
                    case ATTACK_ROAR:
                    case ATTACK_ROAR_WEAK:
                    case ATTACK_ROAR_STRONG:
                        if (Player_InStatus4State(pl) == 1) {
                            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STATUS4_HIT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            goto react;
                        }
                        if (Player_IgnoresFlinch(pl) == 1 && attackKind != ATTACK_ROAR_STRONG) {
                            if ((Player_Status5Full(pl) & 0xFF) && Player_IsLocal(pl) == 1) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STUN, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else if ((Player_HasMashBuildup(pl) & 0xFF) && Player_IsLocal(pl) == 1) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_MASH_START, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                noHitSound = 1;
                            }
                            break;
                        }
                        if (Player_HasSongEffect(pl, PL_SONG_ROAR_GUARD) == 1) {
                            meal = pl->songEffect[PL_SONG_ROAR_GUARD];
                        } else {
                            meal = 0;
                        }
                        if (attackKind != ATTACK_ROAR_STRONG) {
                            if (Player_HasSkill(pl, SKILL_ROAR_GUARD_MOST) == 1 || Player_HasSkill(pl, SKILL_ROAR_GUARD_ALL) == 1 || meal >= 2) {
                                noHitSound = 1;
                                break;
                            }
                            if ((Player_HasSkill(pl, SKILL_ROAR_GUARD_WEAK) == 1 || meal > 0) && attackKind == ATTACK_ROAR_WEAK) {
                                noHitSound = 1;
                                break;
                            }
                        } else if (Player_HasSkill(pl, SKILL_ROAR_GUARD_ALL) == 1 || meal >= 3) {
                            noHitSound = 1;
                            break;
                        }
                        switch (pl->stateGroup) {
                        case 2:
                            switch (pl->state) {
                            case 0:
                            case 1:
                            case 2:
                            case 4:
                            case 5:
                            case 6:
                            case 7:
                            case 14:
                            case 15:
                            case 17:
                            case 18:
                            case 26:
                                noHitSound = 1;
                                break;
                            default:
                                goto turn;
                            }
                            break;
                        case 0:
                        case 1:
                            if (attackKind != ATTACK_ROAR_STRONG) {
                                if (pl->action == PL_POSTURE_CROUCHED) {
                                    noHitSound = 1;
                                    break;
                                }
                                if ((pl->weaponType == WEAPON_HAMMER || pl->weaponType == WEAPON_HUNTING_HORN) && pl->weaponTimer != 0) {
                                    noHitSound = 1;
                                    break;
                                }
                                if (Player_TestFlag(pl, 0x02000600) != 0 || pl->weaponOut != 0) {
                                    Player_EndMash(pl);
                                    Player_SetState(pl, PL_GROUP_REACT, PL_REACT_KNOCKDOWN_FRONT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                                }
                            }
                            goto turn;
                        case 4:
                        case 3:
                            noHitSound = 1;
                            break;
                        default:
                        turn:
                            pl->rot[1] = (u16) (pl->hitAngle + ANGLE_TURN / 2);
                            pl->targetYaw = pl->rot[1];
                            Player_EndMash(pl);
                            switch (attackKind) {
                            default:
                            case 4:
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_ROAR, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                                goto react;
                            case 15:
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_ROAR_SHORT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                                goto react;
                            case 21:
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_ROAR_STRONG, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                                goto react;
                            }
                        }
                        break;
                    case ATTACK_WIND:
                    case ATTACK_WIND_WEAK:
                        if (Player_InStatus4State(pl) == 1) {
                            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STATUS4_HIT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            goto react;
                        }
                        if (Player_HasSkill(pl, SKILL_WIND_GUARD_ALL) == 1) {
                            noHitSound = 1;
                        } else if (Player_HasSkill(pl, SKILL_WIND_GUARD_WEAK) == 1 && attackKind == ATTACK_WIND_WEAK) {
                            noHitSound = 1;
                        } else {
                            pl->rot[1] = (u16) (pl->hitAngle + ANGLE_TURN / 2);
                            pl->targetYaw = pl->rot[1];
                            Player_EndMash(pl);
                            if (attackKind == ATTACK_WIND) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_WIND, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_WIND_SHORT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                            goto react;
                        }
                        break;
                    case ATTACK_QUAKE_LONG:
                    case ATTACK_QUAKE:
                        if (Player_InStatus4State(pl) == 1) {
                            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STATUS4_HIT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            goto react;
                        }
                        if (Player_HasSkill(pl, SKILL_QUAKE_GUARD) == 1) {
                            noHitSound = 1;
                        } else {
                            Player_EndMash(pl);
                            if (attackKind == ATTACK_QUAKE_LONG) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_QUAKE_LONG, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_QUAKE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                            goto react;
                        }
                        break;
                    case 8:
                    case 11:
                    case 12:
                    case 13:
                    case 24:
                        noHitSound = 1;
                        break;
                    case ATTACK_REVIVE:
                        noHitSound = 1;
                        Player_SetState(pl, PL_GROUP_EVENT, PL_EVENT_REVIVE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                        break;
                    default:
                        if (Player_InStatus4State(pl) == 1) {
                            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STATUS4_HIT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            goto react;
                        }
                        if (Player_IgnoresFlinch(pl) == 1) {
                            if ((Player_Status5Full(pl) & 0xFF) && Player_IsLocal(pl) == 1) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STUN, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else if ((Player_HasMashBuildup(pl) & 0xFF) && Player_IsLocal(pl) == 1) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_MASH_START, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                noHitSound = 1;
                            }
                            break;
                        }
                        Player_EndMash(pl);
                        if (hitDir < 0x8000) {
                            pl->rot[1] = pl->hitAngle;
                            pl->targetYaw = pl->rot[1];
                            if (*(u16 *) ((u8 *) pl + 1010) < 11) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_FLINCH_BACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_KNOCKDOWN_BACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                        } else {
                            pl->rot[1] = (u16) (pl->hitAngle + ANGLE_TURN / 2);
                            pl->targetYaw = pl->rot[1];
                            if (*(u16 *) ((u8 *) pl + 1010) < 11) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_FLINCH_FRONT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_KNOCKDOWN_FRONT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                        }
                        pl->action = PL_POSTURE_STANDING;
                    react:
                        Vib_StartLocal(pl, 2);
                        if (!(knockedDown & 0xFF)) {
                            Player_PlaySe(pl, Sound_RandomSe(32, 2, 33, 2), 0, &pl->pos[0], 1, 0);
                        } else {
                            Player_PlaySe(pl, Sound_RandomSe(34, 4, 34, 0), 0, &pl->pos[0], 1, 0);
                        }
                        break;
                    }
                } else {
                    Player_Die(pl);
                }
            }
        }
    done:
        if (!(noHitSound & 0xFF)) {
            Bgm_StopEvent(pl);
            Player_PlaySe(pl, 30, 0, &pl->pos[0], 1, 0);
        }
    }
}
