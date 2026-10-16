/*
 * The player's reaction to the hits it takes, and the monster script's
 * trigger handling.
 *
 * Player_ReactToHit runs for a player with a hit pending (Player.hitReceived,
 * set by ObjAttach_DamagePlayer in sub_main): a hit the guard takes
 * (Player_CanGuardHit) goes to Player_GuardHit, any other picks a state of
 * PL_GROUP_REACT from the attack's kind (enum AttackKind, objattach.h), its
 * force and the side it came from, after the skills that block or soften
 * it; the statuses the hit carries (poison, the buildups) are applied here.
 * The small Player_* predicates below are shared with the state code
 * (unk_70A290.c) and sub_main.
 *
 * The monster side: EmScript_PickTrigger starts the script of a pending
 * trigger (Em.triggers), EmScript_Restart restarts the script, Em_SetCombat
 * enters and leaves combat, Em_CanEnterArea checks a move. EmDrome_MotionSounds
 * (the sounds of em_drome.c's motions) closes the file.
 */
#include "common.h"
#include "em.h"
#include "player.h"
#include "objattach.h"

/* prototypes */
void Player_AddStamina(Player *p, s16 n);
void Player_AddSharpness(Player *p, s16 n);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
extern u16 **D_008BF840[];
u16 Area_BaseId(u16);
u16 Area_DayId(u16 area);
void *EmScript_Trigger10Script(Em *em, u8 i);
void *EmScript_Trigger20Script(Em *em, u8 i);
void *EmScript_Trigger02Script(Em *em, u8 i);
void *EmScript_Trigger04Script(Em *em, u8 i);
void *EmScript_Trigger08Script(Em *em, u8 i);
void *EmScript_Trigger80Script(Em *em);
void *EmScript_Trigger40Script(Em *em);
void Em_PlaySeAtBoneOnFrame(Em *em, s32 frame, s32 se, s32 bone);
s32 Sound_RandomSe(s32 id, s16 lo, s32 alt, s16 n);
void Bgm_StopEvent(void *);
void Player_ClearBuffs(Player *pl);
void Player_RequestStateSynced(Player *p, u16 state, u8 sub, u16 flags);
void Player_ClearFlag(void *, u32 flag);
void Player_ResetAim(Player *p);
void StageObj_K1B_Create(s8 k, s16 *arg);
/* unit's own declaration: with the definition's s16 (void *obj, u8 state, u8 sub) (sub_main/unk_3A1070.c), Player_InStatus4State: 18 of 26 words differ */
s32 Player_IsState(void *obj, s32 group, s32 state);
s32 Em_StartScript(Em *em);
s32 Player_HasSkill(Player *p, u8 skill);
s32 Player_CanGuard(Player *p);
s32 HeavyBowgun_HasShield(Player *p);
u32 Player_TestFlag(void *obj, u32 flag);
/* end of prototypes */

/*
 * Starts the script of the most urgent pending trigger (Em.triggers): 0x40
 * and 0x80 replace the running script thread outright, the others (0x20,
 * 0x10, 8, 4, 2, in that order) are added to the running threads in
 * Em.scriptFlags. A trigger whose thread already runs (Em_ScriptFlagsFrom)
 * waits. Returns the script started first, or NULL; it becomes Em.scriptPos.
 */
u8 *EmScript_PickTrigger(Em *em)
{
    void *script;
    s32 pending;

    pending = em->triggers;
    if (pending == 0) {
        return 0;
    }
    script = NULL;
    if (((u8) pending & 0x40) && Em_ScriptFlagsFrom(em, 0x40) == 0) {
        script = EmScript_Trigger40Script(em);
        em->triggers = 0;
        em->scriptFlags = 0x40;
    }
    if ((em->triggers & 0x80) && Em_ScriptFlagsFrom(em, 0x80) == 0 && script == NULL) {
        script = EmScript_Trigger80Script(em);
        em->triggers = 0;
        em->scriptFlags = 0x80;
    }
    if ((em->triggers & 0x20) && Em_ScriptFlagsFrom(em, 0x20) == 0) {
        em->trigger20Pos = EmScript_Trigger20Script(em, 0);
        if (script == NULL) {
            script = em->trigger20Pos;
        }
        em->triggers &= 0xCE;
        em->scriptFlags |= 0x20;
    }
    if ((em->triggers & 0x10) && Em_ScriptFlagsFrom(em, 0x10) == 0) {
        em->trigger10Pos = EmScript_Trigger10Script(em, 0);
        if (script == NULL) {
            script = em->trigger10Pos;
        }
        em->triggers &= 0xEF;
        em->scriptFlags |= 0x10;
    }
    if ((em->triggers & 8) && Em_ScriptFlagsFrom(em, 8) == 0) {
        em->trigger08Pos = EmScript_Trigger08Script(em, 0);
        if (script == NULL) {
            script = em->trigger08Pos;
        }
        em->triggers &= 0xF7;
        em->scriptFlags |= 8;
    }
    if ((em->triggers & 4) && Em_ScriptFlagsFrom(em, 4) == 0) {
        em->trigger04Pos = EmScript_Trigger04Script(em, 0);
        if (script == NULL) {
            script = em->trigger04Pos;
        }
        em->triggers &= 0xC8;
        em->scriptFlags |= 4;
    }
    if ((em->triggers & 2) && Em_ScriptFlagsFrom(em, 2) == 0) {
        em->trigger02Pos = EmScript_Trigger02Script(em, 0);
        if (script == NULL) {
            script = em->trigger02Pos;
        }
        em->triggers &= 0xCC;
        em->scriptFlags |= 2;
    }
    if (script != NULL) {
        em->scriptPos = script;
    }
    return script;
}

/* Restarts the monster's script from main script 0 when Em.scriptRestart is
 * set: the threads, the loop, the area sequence and the destination area are
 * cleared. */
void EmScript_Restart(Em *em)
{
    if (em->scriptRestart != 0) {
        em->scriptPending = 0;
        em->scriptRestart = 0;
        em->scriptFlags = 0;
        em->scriptDelay = 0;
        em->scriptNo = 0;
        em->mainScriptPos = (u8 *) Em_StartScript(em);
        em->loopPoint = -1;
        em->loopCount = -1;
        em->areaSeqPos = -1;
        em->areaSeqLen = -1;
        *(u16 *) &em->destArea = 0xFFFF;
        em->prevArea = 0xFFFF;
        em->unk7A8 = 0;
    }
}

/*
 * Sets the monster's combat mode (Em.combat) to `mode`, for `frames` frames
 * when frames > 0 (Em.combatTimer). The trigger threads below 0x40 stop.
 * Leaving combat (mode 0) clears what it had noticed of the players, and
 * the players it fought leave the battle. Returns 1 when the mode changed,
 * 0 when it was already set.
 */
s32 Em_SetCombat(Em *em, u8 mode, s16 frames)
{
    s8 i;

    if (em->combat == mode) {
        return 0;
    }
    if (frames > 0) {
        em->combatTimer = frames;
    }
    em->combat = mode;
    em->scriptFlags &= 0xC0;
    if (em->combat == 0) {
        em->seen = 0;
        em->noticed = 0;
        for (i = 0; i < 4; i++) {
            em->alertBuildup[i] = 0;
            em->noticeCount[i] = 0;
            em->noticeTimer[i] = 0;
            if ((em->combatPlayers & (1 << i)) && gPlayers[i].active != 0 && gPlayers[i].unk73A_combat != 0) {
                gPlayers[i].unk73A_combat = 0;
            }
        }
        em->combatPlayers = 0;
    }
    return 1;
}

/* 1 if the monster may go to area `area`: its own area, or one of the list of its kind for the
 * current quest row (ids from 10000 on name an area, the others a day id), ending with 0xFFFF. */
s32 Em_CanEnterArea(Em *em, u16 area)
{
    u16 **kindAreas = D_008BF840[em->type];
    u16 *entry;

    if (em->area == area) {
        return 1;
    }
    if (kindAreas == NULL) {
        return 0;
    }
    kindAreas += Area_BaseId(gScene.map);
    entry = *kindAreas;
    if (entry == NULL) {
        return 0;
    }
    while (*entry != 0xFFFF) {
        if (*entry >= 10000) {
            if (*entry - 10000 == area) {
                return 1;
            }
        } else if (Area_DayId(*entry) == Area_DayId(area)) {
            return 1;
        }
        entry++;
    }
    return 0;
}

/* 1 when the status 5 buildup is full (50). */
s32 Player_Status5Full(Player *pl)
{
    return (pl->status5Buildup >= 0x32) & 0xFF;
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
        case 0x1B:
        case 0x1E:
        case 0x1F:
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
            return 1;
        }
    }
    return 0;
}

/* Ends the mash status. If presses were still needed, it first makes stage
 * object 0x1B (mode 0) for the player, as the mash states do. */
void Player_EndMash(Player *pl)
{
    if (pl->mashLeft > 0) {
        StageObj_K1B_Create(0, (s16 *) pl);
    }
    pl->mashLeft = 0;
}

/*
 * Adds `amount` to the player's poison, as the skills allow: skill
 * 0xB makes the player immune (the poison is cleared), 0xA halves the
 * amount, 0xC doubles it, 0xD or 0xE halve it again. The poison then ticks
 * every 60 frames with skill 0xA, every 15 with 0xC, else every 30.
 */
void Player_AddPoison(Player *pl, s16 amount)
{
    if (Player_HasSkill(pl, 0xB) == 1) {
        pl->poison = 0;
        return;
    }
    if (Player_HasSkill(pl, 0xA) == 1) {
        amount /= 2;
    } else if (Player_HasSkill(pl, 0xC) == 1) {
        amount *= 2;
    }
    if (Player_HasSkill(pl, 0xD) == 1 || Player_HasSkill(pl, 0xE) == 1) {
        amount /= 2;
    }
    pl->poison += amount;
    if (Player_HasSkill(pl, 0xA) == 1) {
        pl->poisonTimer = 60;
    } else if (Player_HasSkill(pl, 0xC) == 1) {
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
    case 0xE:
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
        Player_ClearFlag(pl, 0x8000);
        pl->hp = 0;
        pl->hpRecoverable = 0;
        Player_ResetAim(pl);
        Player_ClearBuffs(pl);
        pl->areaMoveState = 0;
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
    if ((diff <= 3641) || (diff >= 0x71C7)) {
        return 1;
    }
    return 0;
}

/*
 * 1 when the guard takes the pending hit: the player has 75 stamina, the hit
 * can be guarded (attribute 0x80 needs skill 0x29, and never with the heavy
 * bowgun), the state allows the guard (Player_CanGuard), the weapon guards
 * (great sword, sword and shield, lance, gunlance, heavy bowgun) and is
 * drawn, the player stands or crouches, guards (flag 0x8000, or the auto-guard
 * skill 0x2C or the heavy bowgun's shield when it was free last frame) and
 * faces the hit (Player_InGuardArc).
 */
u8 Player_CanGuardHit(Player *pl)
{
    u8 ret = 0;

    if (pl->stamina < 75) {
        return 0;
    }
    if ((pl->hitAttrs & 0x80) && (Player_HasSkill(pl, 0x29) == 0 || pl->weaponType == WEAPON_HEAVY_BOWGUN)) {
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
            case 0:
            case 1:
                if (((Player_HasSkill(pl, 0x2C) == 1 || HeavyBowgun_HasShield(pl) == 1) && pl->prevFreeState != 0)
                    || Player_TestFlag(pl, 0x8000) != 0) {
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
 * lowered by the guard skills 0x28-0x2B, picks the reaction (a push, a
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
    if (Player_HasSkill(pl, 0x29) == 1 && pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        if (force > 0x15) {
            force -= 0x14;
        } else {
            force = 1;
        }
    } else if (Player_HasSkill(pl, 0x28) == 1 && pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        if (force > 0xB) {
            force -= 0xA;
        } else {
            force = 1;
        }
    } else if (Player_HasSkill(pl, 0x2A) == 1 && pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        if (force < 0xFA) {
            force += 5;
        } else {
            force = 0xFF;
        }
    } else if (Player_HasSkill(pl, 0x2B) == 1 && pl->weaponType != WEAPON_HEAVY_BOWGUN) {
        if (force < 0xF5) {
            force += 0xA;
        } else {
            force = 0xFF;
        }
    }
    if (force >= 0x28) {
        react = 2;
        Player_SetState(pl, PL_GROUP_REACT, PL_REACT_GUARD_KNOCKBACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        dull = -0xA;
        Player_AddStamina(pl, -0xB4);
    } else if (force >= 0xF) {
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
        Player_AddStamina(pl, -0x64);
    } else {
        react = 0;
        Player_SetState(pl, PL_GROUP_REACT, PL_REACT_GUARD_PUSH, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        dull = -1;
        Player_AddStamina(pl, -0x4B);
    }
    if (pl->weaponType == WEAPON_GREAT_SWORD && (pl->lastAttackKind != ATTACK_ROAR || pl->lastAttackKind != ATTACK_ROAR_WEAK || pl->lastAttackKind != ATTACK_ROAR_STRONG)) {
        Player_AddSharpness(pl, dull);
    }
    return react;
}

void Effect06_SpawnAtBone(Player *pl, s16 type, u8 sub, s16 bone, f32 scale);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void Player_AddHp(Player *pl, s16 n);
void Player_AddMaxStamina(Player *pl, s16 n);
s32 Player_HasMealEffect(Player *pl, s16 kind);
s32 Player_IsLocal(void *obj);
void Player_SetFlag(void *obj, u32 flag);
u16 Rand_Next(s32 stream);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
void StageObj_K10_CreateOnEmAt(Em *em, f32 *pos, u8 k, s16 mode, s16 part, f32 scale);
void Vib_StartLocal(void *port, s32 i);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Player_IgnoresFlinch(Player *pl);
void Player_AddPoison(Player *pl, s16 n);

/*
 * Reacts to the hit the player took this frame (see the file's comment).
 * hitDir is the hit's direction relative to the player's heading plus a
 * quarter turn: below 0x8000 the hit came from behind. knockedDown picks
 * the heavier hit sound; noHitSound (set in the roar, wind and tremor
 * cases) skips the end of the event music and sound 0x1E. The cases share
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
                if (pl->status6Hit > 0 && Player_HasSkill(pl, 0xE) == 0) {
                    if (Player_HasSkill(pl, 0xD) == 1) {
                        pl->status6Timer = 900;
                    } else if (Player_HasSkill(pl, 0xF) == 1) {
                        pl->status6Timer = 3600;
                    } else {
                        pl->status6Timer = 1800;
                    }
                }
            }
        } else {
            hitDir = (u16) (pl->rot[1] - pl->hitAngle) + 0x4000;
            if (Player_IsLocal(pl) == 1) {
                if (pl->hitDamage[0] != 0) {
                    hpBefore = pl->hp;
                    if (hpBefore > 0 && (Rand_Next(1) & 3) == 0) {
                        if (Player_HasSkill(pl, 0x58) == 1) {
                            pl->hitDamage[0] = 0.7f * pl->hitDamage[0];
                            Effect06_SpawnAtBone(pl, 2, 0x19, 0xA, 4.0f);
                            Em_GetBonePos(pl, 0xA, pos);
                            Sound_PlayPlayerSe(pl, 0x72, 0, pos, 1, 0);
                            if (pl->hitDamage[0] == 0) {
                                pl->hitDamage[0] = 1;
                            }
                        } else if (Player_HasSkill(pl, 0x59) == 1) {
                            pl->hitDamage[0] = 1.3f * pl->hitDamage[0];
                            Em_GetBonePos(pl, 0xA, pos);
                            StageObj_K10_CreateOnEmAt((Em *) pl, pos, 5, 3, 1, 1.0f);
                            Sound_PlayPlayerSe(pl, 0x73, 0, pos, 1, 0);
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
                    if (Player_HasSkill(pl, 8) == 1) {
                        pl->status5Hit = 0;
                    } else {
                        if (Player_HasSkill(pl, 7) == 1) {
                            pl->status5Hit /= 2;
                        }
                        pl->status5Buildup += pl->status5Hit;
                        pl->status5Delay = 150;
                    }
                }
                if (pl->status2Hit > 0) {
                    if (Player_HasSkill(pl, 5) == 1) {
                        pl->status2Hit = 0;
                    } else {
                        pl->status2Buildup += pl->status2Hit;
                    }
                }
                if (pl->status4Hit > 0) {
                    if (Player_HasSkill(pl, 2) == 1) {
                        pl->status4Hit = 0;
                    } else {
                        pl->status4Buildup += pl->status4Hit;
                    }
                }
                if (pl->poisonHit > 0) {
                    Player_AddPoison(pl, pl->poisonHit);
                }
                if (pl->status6Hit > 0 && Player_HasSkill(pl, 0xE) == 0) {
                    if (Player_HasSkill(pl, 0xD) == 1) {
                        pl->status6Timer = 900;
                    } else if (Player_HasSkill(pl, 0xF) == 1) {
                        pl->status6Timer = 3600;
                    } else {
                        pl->status6Timer = 1800;
                    }
                }
                if ((s16) pl->status7Hit > 0 && Player_HasSkill(pl, 0xC5) == 0) {
                    if (Player_HasSkill(pl, 0xC4) == 1) {
                        pl->status7Timer = 900;
                    } else {
                        pl->status7Timer = 1800;
                    }
                }
                if (pl->status8Hit > 0 && Player_HasSkill(pl, 0xC5) == 0) {
                    if (Player_HasSkill(pl, 0xC4) == 1) {
                        if (pl->staminaMax > 75) {
                            Player_AddMaxStamina(pl, pl->staminaMax / 75 / 2 * -75);
                        }
                    } else {
                        Player_AddMaxStamina(pl, -375);
                    }
                }
                if (pl->mashBuildupHit > 0) {
                    if (Player_HasSkill(pl, 0x11) != 0) {
                        pl->mashBuildupHit = 0;
                    }
                    pl->mashBuildup += pl->mashBuildupHit;
                }
                Player_SetFlag(pl, 2);
                Player_ClearFlag(pl, 0x8000);
                if (pl->hp > 0 || pl->lastAttackKind == ATTACK_REVIVE || pl->action == 2) {
                    attackKind = pl->lastAttackKind;
                    if (pl->hp <= 0 && pl->lastAttackKind != ATTACK_REVIVE) {
                        attackKind = ATTACK_BLOW;
                    }
                    if (pl->action == 2) {
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
                            pl->rot[1] = (u16) (pl->hitAngle + 0x8000);
                            pl->targetYaw = pl->rot[1];
                            if (attackKind == ATTACK_BLOW) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_BLOWN_BACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_TOSSED_BACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                        }
                        Player_EndMash(pl);
                        knockedDown = 1;
                        pl->action = 2;
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
                        pl->action = 2;
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
                        if (Player_HasMealEffect(pl, 4) == 1) {
                            meal = pl->mealEffect[4];
                        } else {
                            meal = 0;
                        }
                        if (attackKind != ATTACK_ROAR_STRONG) {
                            if (Player_HasSkill(pl, 0x8F) == 1 || Player_HasSkill(pl, 0x90) == 1 || meal >= 2) {
                                noHitSound = 1;
                                break;
                            }
                            if ((Player_HasSkill(pl, 0x8E) == 1 || meal > 0) && attackKind == ATTACK_ROAR_WEAK) {
                                noHitSound = 1;
                                break;
                            }
                        } else if (Player_HasSkill(pl, 0x90) == 1 || meal >= 3) {
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
                            case 0xE:
                            case 0xF:
                            case 0x11:
                            case 0x12:
                            case 0x1A:
                                noHitSound = 1;
                                break;
                            default:
                                goto turn;
                            }
                            break;
                        case 0:
                        case 1:
                            if (attackKind != ATTACK_ROAR_STRONG) {
                                if (pl->action == 1) {
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
                            pl->rot[1] = (u16) (pl->hitAngle + 0x8000);
                            pl->targetYaw = pl->rot[1];
                            Player_EndMash(pl);
                            switch (attackKind) {
                            default:
                            case 4:
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_ROAR, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                                goto react;
                            case 0xF:
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_ROAR_SHORT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                                goto react;
                            case 0x15:
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
                        if (Player_HasSkill(pl, 0x5B) == 1) {
                            noHitSound = 1;
                        } else if (Player_HasSkill(pl, 0x5A) == 1 && attackKind == ATTACK_WIND_WEAK) {
                            noHitSound = 1;
                        } else {
                            pl->rot[1] = (u16) (pl->hitAngle + 0x8000);
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
                        if (Player_HasSkill(pl, 0xC3) == 1) {
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
                            if (*(u16 *) ((u8 *) pl + 0x3F2) < 0xB) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_FLINCH_BACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_KNOCKDOWN_BACK, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                        } else {
                            pl->rot[1] = (u16) (pl->hitAngle + 0x8000);
                            pl->targetYaw = pl->rot[1];
                            if (*(u16 *) ((u8 *) pl + 0x3F2) < 0xB) {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_FLINCH_FRONT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            } else {
                                Player_SetState(pl, PL_GROUP_REACT, PL_REACT_KNOCKDOWN_FRONT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                            }
                        }
                        pl->action = 0;
                    react:
                        Vib_StartLocal(pl, 2);
                        if (!(knockedDown & 0xFF)) {
                            Player_PlaySe(pl, Sound_RandomSe(0x20, 2, 0x21, 2), 0, &pl->pos[0], 1, 0);
                        } else {
                            Player_PlaySe(pl, Sound_RandomSe(0x22, 4, 0x22, 0), 0, &pl->pos[0], 1, 0);
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
            Player_PlaySe(pl, 0x1E, 0, &pl->pos[0], 1, 0);
        }
    }
}

/*
 * Plays the sounds of the motion em_drome.c's monster started last
 * (work->prevMotion): per motion id, Em_PlaySeAtBoneOnFrame(em, frame,
 * sound, bone) for each sound and the frame it plays at. The numbers are
 * motion ids, frames, sound ids and bones: a table written as code.
 */
void EmDrome_MotionSounds(Em *em, EmWork *work)
{
    s16 prevMotion;

    prevMotion = work->prevMotion;
    switch (prevMotion) {
    case 0x3E9:
        Em_PlaySeAtBoneOnFrame(em, 0xC8, Sound_RandomSe(0xE, 2, 0xE, 2), 0);
        break;
    case 0x3EA:
        Em_PlaySeAtBoneOnFrame(em, 8, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 2, 0);
        break;
    case 0x3EB:
        Em_PlaySeAtBoneOnFrame(em, 0x14, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x10, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 3, 0);
        break;
    case 0x3EC:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 8, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0);
        break;
    case 0x3ED:
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA, 1, 0);
        break;
    case 0x3EE:
    case 0x3EF:
        Em_PlaySeAtBoneOnFrame(em, 6, 5, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 4, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2A, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 1, 0);
        break;
    case 0x3F0:
        Em_PlaySeAtBoneOnFrame(em, 8, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 8, 4, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x20, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 7, 0);
        break;
    case 0x3F1:
    case 0x3F2:
        Em_PlaySeAtBoneOnFrame(em, 0xC, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xC, 4, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 7, 0);
        break;
    case 0x3F3:
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA4, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x22, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x104, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x120, 0x17, 0);
        break;
    case 0x3F4:
        Em_PlaySeAtBoneOnFrame(em, 0x76, 0xE, 0);
        break;
    case 0x3FC:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 8, 0);
        break;
    case 0x3FD:
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x24, 0, 0);
        break;
    case 0x3FE:
        Em_PlaySeAtBoneOnFrame(em, 0x28, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x76, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1A, 0x10, 0);
        break;
    case 0x3FF:
        Em_PlaySeAtBoneOnFrame(em, 0x16, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 3, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 8, 0);
        break;
    case 0x400:
        Em_PlaySeAtBoneOnFrame(em, 4, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 1, 0);
        break;
    case 0x401:
        Em_PlaySeAtBoneOnFrame(em, 2, 0x10, 0);
        break;
    case 0x402:
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x80, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x48, 0x1C, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3C, 8, 0);
        break;
    case 0x406:
        Em_PlaySeAtBoneOnFrame(em, 4, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1E, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x1C, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 8, 0);
        break;
    case 0x407:
    case 0x409:
    case 0x40B:
    case 0x40D:
        Em_PlaySeAtBoneOnFrame(em, 4, 0xB, 0);
        break;
    case 0x408:
    case 0x40A:
    case 0x40C:
    case 0x40E:
        Em_PlaySeAtBoneOnFrame(em, 4, 0xF, 0);
        break;
    case 0x40F:
        Em_PlaySeAtBoneOnFrame(em, 0x18, 3, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2C, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x36, 2, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x58, 0, 0);
        break;
    case 0x412:
        Em_PlaySeAtBoneOnFrame(em, 0xB4, 9, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 0xB, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x34, 0xA, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x56, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x84, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x8C, 0, 0);
        break;
    case 0x413:
        Em_PlaySeAtBoneOnFrame(em, 4, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x42, 0x11, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x82, 0x12, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x30, 0x17, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA2, 0xF, 0);
        break;
    case 0x414:
        Em_PlaySeAtBoneOnFrame(em, 4, 0xE, 0);
        break;
    case 0x416:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0xD, 0);
        break;
    case 0x417:
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0x19, 0);
        break;
    case 0x41A:
        Em_PlaySeAtBoneOnFrame(em, 0x20, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x44, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x68, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x14, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xA0, 0x17, 0);
        break;
    case 0x41B:
        Em_PlaySeAtBoneOnFrame(em, 0xA, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x40, 0x14, 0);
        Em_PlaySeAtBoneOnFrame(em, 6, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x7A, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x86, 0, 0);
        break;
    case 0x41C:
        Em_PlaySeAtBoneOnFrame(em, 8, 0x15, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x62, 0x16, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x16, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x3A, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x6A, 1, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x9E, 0, 0);
        break;
    case 0x41D:
        Em_PlaySeAtBoneOnFrame(em, 0x54, 0x18, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x7E, 0x18, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xD8, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x102, 0x1A, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x124, 0x1B, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x9C, 0xB, 0);
        Em_PlaySeAtBoneOnFrame(em, 0xE, 0, 0);
        Em_PlaySeAtBoneOnFrame(em, 0x2E, 2, 0);
        
    default:
        break;
    }
}

/* unk_733EA0's .data (0x8BF450-0x8BF9C0) */
u32 D_008BF450[4] = {
#include "game/008BF450.inc"
};
u32 D_008BF460[4] = {
#include "game/008BF460.inc"
};
u32 D_008BF470[4] = {
#include "game/008BF470.inc"
};
u32 D_008BF480[12] = {
#include "game/008BF480.inc"
};
u16 *D_008BF4B0[16] = {
    NULL, NULL, (u16 *)D_008BF450, (u16 *)D_008BF460,
    (u16 *)D_008BF470, NULL, (u16 *)D_008BF480, NULL,
    NULL, NULL, NULL, (u16 *)&D_008BF480[6],
    NULL, NULL, NULL, NULL,
};
u32 D_008BF4F0[4] = {
#include "game/008BF4F0.inc"
};
u32 D_008BF500[2] = {
#include "game/008BF500.inc"
};
u32 D_008BF508[3] = {
#include "game/008BF508.inc"
};
u16 *D_008BF520[16] = {
    NULL, NULL, NULL, NULL,
    (u16 *)D_008BF4F0, NULL, NULL, NULL,
    NULL, (u16 *)D_008BF500, NULL, (u16 *)D_008BF508,
    NULL, NULL, NULL, NULL,
};
u32 D_008BF560[4] = {
#include "game/008BF560.inc"
};
u32 D_008BF570[4] = {
#include "game/008BF570.inc"
};
u32 D_008BF580[4] = {
#include "game/008BF580.inc"
};
u16 *D_008BF590[16] = {
    NULL, NULL, (u16 *)D_008BF560, NULL,
    (u16 *)D_008BF570, NULL, (u16 *)D_008BF580, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u32 D_008BF5D0[8] = {
#include "game/008BF5D0.inc"
};
u16 *D_008BF5F0[16] = {
    NULL, NULL, NULL, NULL,
    NULL, NULL, (u16 *)D_008BF5D0, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u32 D_008BF630[4] = {
#include "game/008BF630.inc"
};
u32 D_008BF640[4] = {
#include "game/008BF640.inc"
};
u32 D_008BF650[4] = {
#include "game/008BF650.inc"
};
u32 D_008BF660[8] = {
#include "game/008BF660.inc"
};
u16 *D_008BF680[16] = {
    NULL, NULL, NULL, NULL,
    (u16 *)D_008BF630, (u16 *)D_008BF640, (u16 *)D_008BF650, NULL,
    NULL, (u16 *)D_008BF660, NULL, (u16 *)&D_008BF660[2],
    NULL, NULL, NULL, NULL,
};
u32 D_008BF6C0[8] = {
#include "game/008BF6C0.inc"
};
u32 D_008BF6E0[12] = {
#include "game/008BF6E0.inc"
};
u16 *D_008BF710[16] = {
    NULL, NULL, NULL, (u16 *)D_008BF6C0,
    NULL, NULL, (u16 *)D_008BF6E0, NULL,
    NULL, NULL, NULL, (u16 *)&D_008BF6E0[6],
    NULL, NULL, NULL, NULL,
};
u32 D_008BF750[4] = {
#include "game/008BF750.inc"
};
u32 D_008BF760[4] = {
#include "game/008BF760.inc"
};
u32 D_008BF770[4] = {
#include "game/008BF770.inc"
};
u16 *D_008BF780[16] = {
    NULL, NULL, (u16 *)D_008BF750, NULL,
    (u16 *)D_008BF760, NULL, (u16 *)D_008BF770, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u32 D_008BF7C0[6] = {
#include "game/008BF7C0.inc"
};
u32 D_008BF7D8[3] = {
#include "game/008BF7D8.inc"
};
u32 D_008BF7E8[3] = {
#include "game/008BF7E8.inc"
};
u16 *D_008BF800[16] = {
    NULL, NULL, NULL, (u16 *)D_008BF7C0,
    (u16 *)D_008BF7D8, (u16 *)D_008BF7E8, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u16* *D_008BF840[76] = {
    NULL, D_008BF4B0, NULL, NULL,
    NULL, NULL, D_008BF590, NULL,
    NULL, NULL, NULL, D_008BF4B0,
    NULL, NULL, NULL, D_008BF520,
    NULL, NULL, NULL, NULL,
    D_008BF590, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, D_008BF4B0, D_008BF590, D_008BF590,
    D_008BF5F0, D_008BF4B0, D_008BF4B0, NULL,
    NULL, D_008BF520, NULL, NULL,
    NULL, D_008BF4B0, NULL, D_008BF680,
    D_008BF680, D_008BF680, D_008BF710, NULL,
    NULL, NULL, NULL, D_008BF780,
    D_008BF710, NULL, NULL, NULL,
    D_008BF800, D_008BF800, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u32 gEmStartsBattle[20] = {
#include "game/008BF970.inc"
};
