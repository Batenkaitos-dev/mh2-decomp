/*
 * The player's states, actions and per-frame update (Player_Update). The
 * last function, Item_IsStealable, lists the pouch items Chameleos prefers
 * when it steals, which Item_IsUsable also refuses while status 6 lasts.
 *
 * From Player_PickStatusState to Player_Update the code is one of Capcom's
 * units: Player_PickStatusState is static, and its one caller,
 * Player_Update, keeps its first argument in the same register across the
 * call, which the compiler does only when the callee is static. The code
 * from Player_RunGestureState on is built at optimization level 4
 * (Player_Update and Player_CheckTeamCombo need it), the code before it at
 * the default level.
 */
#include "common.h"
#include "stageobj.h"
#include "msg_event.h"
#include "sprite.h"
#include "syssettings.h"
#include "scene.h"
#include "quest.h"
#include "scevent.h"
#include "em.h"
#include "em_script.h"
#include "em_anteka.h"
#include "em_apceros.h"
#include "em_mosswine.h"
#include "em_felyne_cart.h"
#include "pad.h"
#include "player.h"
#include "item.h"
#include "area.h"
#include "objattach.h"
#include "qsync.h"
#include "chat_log.h"
#include "area_draw.h"
#include "bgm.h"
#include "place_rules.h"
#include "camera.h"
#include "capsule.h"
#include "card.h"
#include "fishing.h"
#include "hud.h"
#include "qnet.h"
#include "save.h"
#include "sound.h"
#include "stream.h"
#include "bone_effects.h"
#include "draw_slots.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "game_util.h"
#include "gm_em.h"
#include "hit_effects.h"
#include "part_effects.h"
#include "rand.h"
#include "vibration.h"
#include "supply_box.h"
#include "hit_spark.h"
#include "vec_math.h"
#include "attack_objects.h"
#include "cam_areas.h"
#include "cam_helpers.h"
#include "cam_shake.h"

/* The walls each player touches (WallContact): Player_TurnAlongWall and
 * Player_TurnAlongWallSide turn the player a quarter turn off the first one. */
extern WallContact gEmWallContacts[][WALL_CONTACTS];

/* Prototypes shared by the parts of this unit. */
void Effect06_SpawnAt(void *obj, s32 type, s32 bone, f32 *pos, f32 scale); /* unit's own declaration: type and bone declared s32 as the callers pass them; with the definition's s16 type and u8 bone the calls compile differently (Em_UpdateStatus most of all) */
u8 Pouch_Add(Player *p, u16 item, s16 n);
extern s16 gDrawnWalkStopIdle[];
s32 Player_HasSongEffect(Player *pl, s16 kind);
/* unit's own declaration: the flag declared u8 as Player_UpdateEnvironment passes it; with the definition's s32 its call compiles differently */
void PlayerFx_TickFloorHazard(Player *p, u8 flag);
extern f32 gStartPos[][3];
extern u16 gStartAngle[];
extern f32 gSpotLineupOffsets[];
typedef struct WeaponMotion { s16 motion; s16 blend; s16 flag; s16 next; } WeaponMotion;
extern WeaponMotion gDrawMotions[];
extern WeaponMotion gSheatheMotions[];
extern WeaponMotion gSheatheMovingMotions[];
extern WeaponMotion gDrawMovingMotions[];
u16 Pouch_NextSelectable(Player *pl, u16 cur, u8 dir);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Item_UseMisc(Player *pl);
void Item_UseTool(Player *pl);
void Player_SetMotionPair(Player *, s32, s32, s32); /* unit's own declaration: the last parameter declared s32 (the definition's is s16); with the s16 the calls compile differently (Player_StateSpitRoast most of all) */
void Player_SetPosture(Player *, u16 mode, s16 flag1, s16 flag2);
void Player_SetState(Player *p, u16 state, u8 sub, u16 flags);
static void Player_SetAimLayers(Player *, u16, u16);
/* end of prototypes */

s32 Player_InSceneEvent(void)
{
    return gSceneEvents.ended != 0;
}

/*
 * Sets the part motion overrides 0 and 1 (partAnimOn, partAnimMotion,
 * partAnimWeight) from pl->aimWeight: both off at weight 0. A positive weight
 * uses motion motA on part 0 and motA + 100 on part 1, a negative weight motB
 * the same way, with the weight's size (at most 100) as the override weight;
 * a motA of 1409 or a motB of 1410 becomes 1408 with part 0 left off.
 */
static void Player_SetAimLayers(Player *pl, u16 motA, u16 motB)
{

    if (pl->aimWeight == 0) {
        pl->partAnimOn[0] = 0;
        pl->partAnimOn[1] = 0;
        return;
    }
    pl->partAnimOn[0] = 1;
    pl->partAnimOn[1] = 1;
    if (pl->aimWeight > 0) {
        if (motA == 1409) {
            pl->partAnimMotion[0] = 1408;
            pl->partAnimOn[0] = 0;
        } else {
            pl->partAnimMotion[0] = motA;
        }
        pl->partAnimMotion[1] = motA + 100;
        pl->partAnimWeight[0] = pl->aimWeight;
        pl->partAnimWeight[1] = pl->aimWeight;
    } else {
        if (motB == 1410) {
            pl->partAnimMotion[0] = 1408;
            pl->partAnimOn[0] = 0;
        } else {
            pl->partAnimMotion[0] = motB;
        }
        pl->partAnimMotion[1] = motB + 100;
        pl->partAnimWeight[0] = -pl->aimWeight;
        pl->partAnimWeight[1] = -pl->aimWeight;
    }
    if (pl->partAnimWeight[0] > 100) {
        pl->partAnimWeight[0] = 100;
    }
    if (pl->partAnimWeight[1] > 100) {
        pl->partAnimWeight[1] = 100;
    }
}

/* Adds delta to pl->aimWeight, keeping it within -100 to 100. */
static void Bowgun_AddAimWeight(Player *pl, s8 delta)
{

    pl->aimWeight += delta;
    if (pl->aimWeight >= 100) {
        pl->aimWeight = 100;
    }
    if (pl->aimWeight < -99) {
        pl->aimWeight = -100;
    }
}

/*
 * The per-frame damage the player takes from poison and the surroundings;
 * does nothing for a fainted player, and stops early when Player_Damage
 * returns 1. It clears pl->inPoisonArea and ticks the poison (every
 * pl->poisonTimer frames: poison down by 1, 1 damage, the poison effect;
 * another player's copy only shows the effect while PL_STATUS_ICON_POISON is
 * set). For the local player it takes 1 damage per special item carried
 * (Player_CountSpecialItems) every 30 frames, and in a hot area
 * (Area_Temperature 1 or 3) 1 damage every pl->envTimer frames (set from the
 * area kind and SKILL_HEAT_RES, SKILL_HEAT_RES2 and the SKILL_HEAT_DAMAGE_* skills) unless coolDrinkTimer runs or skill
 * SKILL_HEAT_RES or SKILL_HEAT_RES2 protects it. Then, outside the reactions other than the
 * guard's, the faint, the quest events and noHitTimer, it applies the ground
 * hazard pl->floorHazard, or 2 when a Lunastra or Teostra with wind is close
 * (Player_InEmAura): hazard 1 raises poison to at least 10 (and sets
 * inPoisonArea) unless SKILL_POISON_IMMUNE; hazard 2 deals 1 damage every
 * damageInterval frames (8, changed by the SKILL_POISON_AREA_* skills).
 */
void Player_UpdateEnvironment(Player *pl)
{
    f32 effectPos[3];
    s16 n;
    s16 k;
    u8 hazard;
    u16 damageInterval;

    if (pl->stateGroup == PL_GROUP_FAINT) {
        return;
    }
    pl->inPoisonArea = 0;
    if (pl->poison > 0) {
        if (--pl->poisonTimer <= 0) {
            pl->poison--;
            if (Player_HasSkill(pl, SKILL_POISON_HALF) == 1) {
                k = 60;
            } else if (Player_HasSkill(pl, SKILL_POISON_DOUBLE) == 1) {
                k = 15;
            } else {
                k = 30;
            }
            pl->poisonTimer = k;
            if (Player_IsLocal(pl) == 1) {
                effectPos[0] = 0.0f;
                effectPos[1] = 20.0f;
                effectPos[2] = 0.0f;
                Effect06_SpawnAt(pl, 3, 20, effectPos, 1.0f);
            }
            if (Player_Damage(pl, 1, 0) == 1) {
                return;
            }
        }
    }
    if (Player_IsLocal(pl) == 0 && (pl->statusIcons & 1)) {
        k = Player_HasSkill(pl, SKILL_POISON_HALF) ? 60 : 30;
        if (gScene.frame % k == 0) {
            effectPos[0] = 0.0f;
            effectPos[1] = 20.0f;
            effectPos[2] = 0.0f;
            Effect06_SpawnAt(pl, 3, 20, effectPos, 1.0f);
        }
    }
    if (Player_IsLocal(pl) == 1) {
        n = Player_CountSpecialItems(pl);
        if (n > 0 && gScene.frame % 30 == 0) {
            if (Player_Damage(pl, n, 0) == 1) {
                return;
            }
        }
    }
    if (Player_IsLocal(pl) != 1) {
        return;
    }
    k = Area_Temperature(pl->area);
    switch (k) {
    case 1:
    case 3:
        if (pl->coolDrinkTimer != 0) {
            break;
        }
        if (k == 1 && (Player_HasSkill(pl, SKILL_HEAT_RES) == 1 || Player_HasSkill(pl, SKILL_HEAT_RES2) == 1)) {
            break;
        }
        if (k == 3 && Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
            break;
        }
        if (--pl->envTimer > 0) {
            break;
        }
        if (k == 1) {
            pl->envTimer = 90;
            if (Player_HasSkill(pl, SKILL_HEAT_DAMAGE_FAST) == 1) {
                pl->envTimer = 60;
            } else if (Player_HasSkill(pl, SKILL_HEAT_DAMAGE_FASTER) == 1) {
                pl->envTimer = 45;
            } else if (Player_HasSkill(pl, SKILL_HEAT_DAMAGE_SLOW) == 1) {
                pl->envTimer = 180;
            }
        } else {
            pl->envTimer = 60;
            if (Player_HasSkill(pl, SKILL_HEAT_DAMAGE_FAST) == 1) {
                pl->envTimer = 40;
            } else if (Player_HasSkill(pl, SKILL_HEAT_DAMAGE_FASTER) == 1) {
                pl->envTimer = 30;
            } else if (Player_HasSkill(pl, SKILL_HEAT_DAMAGE_SLOW) == 1) {
                pl->envTimer = 120;
            } else if (Player_HasSkill(pl, SKILL_HEAT_RES) == 1) {
                pl->envTimer = 180;
            }
        }
        if (Player_Damage(pl, 1, 0) == 1) {
            return;
        }
        break;
    case 2:
    case 4:
        break;
    }
    if (pl->stateGroup == PL_GROUP_REACT) {
        switch (pl->state) {
        case 1:
        case 3:
        case 8:
        case 10:
        case 11:
        case 12:
        case 38:
            break;
        default:
            return;
        }
    }
    if (pl->stateGroup == PL_GROUP_FAINT) {
        return;
    }
    if (pl->stateGroup == PL_GROUP_EVENT) {
        return;
    }
    if (pl->action == PL_POSTURE_AIRBORNE) {
        return;
    }
    if (pl->noHitTimer > 0) {
        return;
    }
    hazard = pl->floorHazard;
    if (hazard == 0 && Player_InEmAura(pl) == 1) {
        hazard = 2;
    }
    PlayerFx_TickFloorHazard(pl, hazard);
    switch (hazard) {
    case 0:
        break;
    case 1:
        if (Player_HasSkill(pl, SKILL_POISON_IMMUNE) == 0) {
            pl->inPoisonArea = 1;
            if (pl->poison < 10) {
                pl->poison = 10;
            }
        }
        break;
    case 2:
        pl->inPoisonArea = 1;
        damageInterval = 8;
        if (Player_HasSkill(pl, SKILL_POISON_AREA_SLOW) == 1) {
            damageInterval = 12;
        } else if (Player_HasSkill(pl, SKILL_POISON_AREA_SLOWER) == 1) {
            damageInterval = 24;
        } else if (Player_HasSkill(pl, SKILL_POISON_AREA_FAST) == 1) {
            damageInterval = 6;
        } else if (Player_HasSkill(pl, SKILL_POISON_AREA_FASTER) == 1) {
            damageInterval = 4;
        }
        if (gScene.frame % damageInterval == 0) {
            if (Player_Damage(pl, 1, 0) == 1) {
                return;
            }
        }
        break;
    }
}

/*
 * Returns 1 when one of the 20 pouch slots holds an item with a count above 0
 * whose item data flags have bit 0 set (usable from the item selector), else
 * 0.
 */
s32 Player_PouchHasSelectable(Player *pl)
{
    s16 i;

    for (i = 0; i < 20; i++) {
        if (pl->pouch[i].item != 0 && pl->pouch[i].count > 0 && (gItemData[pl->pouch[i].item].flags & 1)) {
            return 1;
        }
    }
    return 0;
}

/*
 * Runs the item selector of the local player. Unless the player is stunned,
 * fainted, in a quest event or the quest is over, holding PAD_L2 opens it
 * (pl->itemSelect = 1, with a sound) and, when the pouch has a selectable
 * item (Player_PouchHasSelectable), PAD_CIRCLE moves pl->pouchCursor to the
 * next selectable item (pouchHudFlags |= 2) and PAD_SQUARE to the one before
 * (|= 4); releasing PAD_L2 closes it with a sound. In the excluded states it
 * just clears itemSelect.
 */
void Player_UpdateItemSelect(Player *pl)
{
    u16 pressed;

    if (Player_IsLocal(pl) != 0) {
        if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STUN) == 0 && (u32) (pl->stateGroup - 3) > 1 && Quest_PhaseIsOver(1) != 1) {
            if (pl->input.held2 & PAD_L2) {
                if (pl->itemSelect == 0) {
                    Sound_PlaySeFlat(0, 17, 0);
                }
                pl->itemSelect = 1;
                if (Player_PouchHasSelectable(pl) != 0) {
                    pressed = pl->input.pressed2;
                    if (pressed & 0x20) {
                        Sound_PlaySeFlat(0, 18, 0);
                        pl->pouchCursor = Pouch_NextSelectable(pl, pl->pouchCursor, 0);
                        pl->pouchHudFlags |= 2;
                    } else if (pressed & 0x200) {
                        Sound_PlaySeFlat(0, 18, 0);
                        pl->pouchCursor = Pouch_NextSelectable(pl, pl->pouchCursor, 1);
                        pl->pouchHudFlags |= 4;
                    }
                }
            } else if (pl->itemSelect != 0) {
                Sound_PlaySeFlat(0, 20, 0);
                pl->itemSelect = 0;
            }
            return;
        }
        pl->itemSelect = 0;
    }
}

/*
 * Returns 1 when the loaded ammo type differs from savedAmmoType (saved when
 * the ammo menu opened) while the ammo menu is closed, PAD_L2 is not held and
 * the weapon is drawn; else 0 (also for a player who is not a gunner).
 * Player_IdleActions skips the weapon's attack input while it returns 1.
 */
static s32 Player_AmmoTypeChanged(Player *pl)
{
    if ((pl->isGunner != 1) && ((u16) pl->ammoSlot == 255)) {
        return 0;
    }
    if (pl->input.held2 & PAD_L2) {
        return 0;
    }
    if ((pl->ammoMenuOpen == 0) && (pl->savedAmmoType != pl->ammoType) && (pl->weaponOut != 0)) {
        return 1;
    }
    return 0;
}

/*
 * The ammo menu of a gunner (the bow's coatings). Does nothing for a non-
 * local player, a non-gunner, a stunned or fainted player, a quest event,
 * after the quest's end (Quest_PhaseIsOver(1)), with pl->ammoMenuLock set, and in
 * the shooting and stabbing steps of the weapon states (BOW_FIRE and
 * BOW_FIRE_QUICK before step 2; BOW_STAB, BOW_STAB_2, BOW_DRAW_STAB;
 * BOWGUN_FIRE_RECOIL_0 to _2 before step 2 and BOWGUN_RAPID_FIRE before step
 * 3). Holding PAD_L2 opens the menu (ammoMenuOpen; the ammo state goes to the
 * saved* fields) and PAD_CROSS or PAD_TRIANGLE steps the ammo slot either way
 * (Bowgun_NextAmmoSlot, then Bowgun_LoadAmmo, with a sound and a
 * pouchHudFlags bit). Releasing it closes the menu: a changed ammo type is
 * loaded, an unchanged one gets its saved count, clip size and rounds back.
 * Always returns 0.
 */
s32 Player_BowgunAmmoSwitch(Player *pl)
{

    if (Player_IsLocal(pl) == 0) {
        return 0;
    }
    if (pl->isGunner != 1) {
        return 0;
    }
    if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STUN) != 0) {
        return 0;
    }
    if (pl->stateGroup == PL_GROUP_FAINT || pl->stateGroup == PL_GROUP_EVENT) {
        return 0;
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (pl->ammoMenuLock != 0) {
        return 0;
    }
    if (pl->stateGroup == PL_GROUP_WEAPON) {
        if (pl->weaponType == WEAPON_BOW) {
            switch (pl->state) {
            case 0:
            case 14:
                if (pl->step < 2) {
                    return 0;
                }
                break;
            case 3:
            case 4:
            case 13:
                return 0;
            }
        } else {
            switch (pl->state) {
            case 0:
            case 4:
            case 5:
                if (pl->step < 2) {
                    return 0;
                }
                break;
            case 8:
                if (pl->step < 3) {
                    return 0;
                }
                break;
            }
        }
    }
    if (pl->ammoMenuOpen == 0) {
        pl->ammoMenuOpen = 1;
        pl->savedAmmoType = pl->ammoType;
        pl->savedAmmoCount = pl->ammoCount;
        pl->savedClipSize = pl->clipSize;
        pl->savedRoundsLoaded = pl->roundsLoaded;
        pl->savedCoatingSlot = pl->coatingSlot;
    }
    if (pl->input.held2 & PAD_L2) {
        if (pl->input.pressed2 & PAD_CROSS) {
            if (pl->ammoSlot == 255) {
                pl->ammoSlot = Bowgun_NextAmmoSlot(pl, 0, 3);
            } else {
                pl->ammoSlot = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 1);
            }
            Bowgun_LoadAmmo(pl, 0);
            if (pl->ammoSlot != 255) {
                pl->pouchHudFlags |= 8;
                Sound_PlaySeFlat(0, 18, 0);
            }
        } else if (pl->input.pressed2 & PAD_TRIANGLE) {
            if (pl->ammoSlot == 255) {
                pl->ammoSlot = Bowgun_NextAmmoSlot(pl, 0, 2);
            } else {
                pl->ammoSlot = Bowgun_NextAmmoSlot(pl, pl->ammoSlot, 0);
            }
            Bowgun_LoadAmmo(pl, 0);
            if (pl->ammoSlot != 255) {
                pl->pouchHudFlags |= 0x10;
                Sound_PlaySeFlat(0, 18, 0);
            }
        }
    } else if (pl->ammoMenuOpen != 0) {
        pl->ammoMenuOpen = 0;
        if (pl->savedAmmoType != pl->ammoType) {
            Bowgun_LoadAmmo(pl, 0);
        } else {
            pl->ammoCount = pl->savedAmmoCount;
            pl->clipSize = pl->savedClipSize;
            pl->roundsLoaded = pl->savedRoundsLoaded;
        }
    }
    if (pl->savedAmmoType == pl->ammoType) {
        pl->ammoCount = pl->savedAmmoCount;
        pl->clipSize = pl->savedClipSize;
        pl->roundsLoaded = pl->savedRoundsLoaded;
        pl->coatingSlot = pl->savedCoatingSlot;
    }
    return 0;
}

/*
 * Shows the banner that item `item` didn't fit in the pouch; the item states
 * call it when Pouch_GiveItem returns 3 (no room). Returns Notice_Push's result.
 */
u8 Player_NoticePouchFull(s32 item)
{
    return Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_POUCH_FULL, item);
}

/*
 * Returns 1 when the player's status 2 build-up is above 0, 2 when status 4's
 * is, else 0; Player_Update turns the result into PL_REACT_STATUS2 or
 * PL_REACT_STATUS4.
 */
static s32 Player_PickStatusState(Player *pl)
{
    if (pl->status2Buildup > 0) {
        return 1;
    }
    if (pl->status4Buildup > 0) {
        return 2;
    }
    return 0;
}

/*
 * Returns the heading a stick points to in the world: the stick's angle plus
 * the camera's yaw (Camera_GetYaw), as 16 bits. i 0 is the left stick
 * (input.stickAngle), 1 the right stick (input.rightStickAngle).
 */
s32 Player_GetStickYaw(Player *pl, s32 i)
{
    s32 yaw = Camera_GetYaw() & 0xFFFF;

    return ((&pl->input.stickAngle)[i] + yaw) & 0xFFFF;
}

/*
 * Sets pl->targetYaw to the left stick's heading (input.stickAngle plus the
 * camera's yaw), unless the camera is turning behind the player
 * (CamCtl_IsTurningBehind).
 */
void Player_FaceStick(Player *pl)
{
    if (CamCtl_IsTurningBehind() != 1) {
        *(s16 *)&pl->targetYaw = (s16) (pl->input.stickAngle + (Camera_GetYaw() & 0xFFFF));
    }
}

/*
 * Returns how far the left stick is pushed: 0 for none (also for a gunner
 * with the weapon drawn and pl->aimMode 1), 1 for a tilt (input.stickTilt) of
 * 40 or more, 3 for 120 or more (a run), and 5 when the sheathed player in
 * PL_GROUP_ACTION holds PAD_R2 with the stick pushed (the dash; not while
 * PL_ACT_RUN_AFTER_DASH waits out dashDelay). The second parameter is not
 * used.
 */
u8 Player_StickMoveLevel(Player *pl, s32 unused)
{
    u8 lvl = 0;
    u16 tilt;

    switch (pl->weaponType) {
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
    case WEAPON_BOW:
        if (pl->weaponOut != 0 && pl->aimMode == 1) {
            return 0;
        }
        break;
    case WEAPON_GREAT_SWORD:
    case WEAPON_HAMMER:
    case WEAPON_LANCE:
    case WEAPON_SWORD_SHIELD:
    case WEAPON_DUAL_SWORDS:
    case WEAPON_LONG_SWORD:
    case WEAPON_HUNTING_HORN:
    case WEAPON_GUNLANCE:
        break;
    }
    tilt = pl->input.stickTilt;
    if (tilt >= 120) {
        lvl = 3;
    } else if (tilt >= 85) {
        lvl = 1;
    } else if (tilt >= 40) {
        lvl = 1;
    }
    if (pl->action == PL_POSTURE_STANDING && pl->weaponOut == 0 && (pl->input.held & PAD_R2) && lvl != 0
        && (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH) == 0 || pl->dashDelay == 0)) {
        return 5;
    }
    return lvl;
}

/*
 * Works out whether a monster fights the player. First copies inBattle to
 * prevInBattle and clears it. For each monster in the player's area that is
 * active, visible, flagged startsBattle, not in EM_GROUP_DEFEAT, has noticed
 * this player and is in combat, it sets the player's bit in the monster's
 * combatPlayers, pl->inBattle = 1, pl->target to the monster and
 * pl->targetAngle to the angle from the monster to the player plus a quarter turn;
 * for Velocidrome, Gendrome and Iodrome it also sets pl->unk73A_combat.
 */
void Player_UpdateInBattle(Player *pl)
{
    s16 i;
    u8 bit;
    Em *monster;
    s32 monsterType;

    bit = 1 << pl->index;
    pl->prevInBattle = pl->inBattle;
    pl->inBattle = 0;
    for (i = 0, monster = gEms; i < 20; i++, monster++) {
        if (monster->area == pl->area && monster->active != 0 && monster->visible != 0 && monster->startsBattle != 0 && monster->stateGroup != 5
            && (monster->noticed & bit) && monster->combat == 1) {
            monster->combatPlayers |= bit;
            pl->inBattle = 1;
            pl->target = monster;
            pl->targetAngle = (u16) (Vec_Heading(&monster->posX, pl->pos) + ANGLE_TURN / 4);
            monsterType = monster->type;
            if (monsterType == 27 || monsterType == 28 || monsterType == 31) {
                pl->unk73A_combat = 1;
            }
        }
    }
}

/*
 * Called with a player (typed Em): returns 1 on the frame a battle begins
 * (inBattle set and not prevInBattle, and the byte Player.unk73A_combat still
 * 0, which it sets to 1), 255 while in a battle otherwise, 0 outside a
 * battle.
 */
static s32 Player_EnteredBattle(Em *em)
{
    u8 battleFlag;

    battleFlag = em->inBattle;
    if (battleFlag != 0) {
        if ((battleFlag != em->prevInBattle) && (em->combatPlayers == 0)) {
            em->combatPlayers = 1;
            return 1;
        }
        return 255;
    }
    return 0;
}

//---
s16 Player_CheckRideSlip(Player *pl)
{
    Em *monster;
    s16 i;
    f32 len;
    f32 lim;

    for (monster = gEms, i = 0; i < 20; i++, monster++) {
        if (pl->rideEm == monster->slotNo && monster->type != 55) {
            return 0;
        }
    }
    len = Vec_Length(pl->rideDelta);
    if (pl->stateGroup == PL_GROUP_ACTION && pl->state == PL_ACT_BED_MENU) {
        lim = 10.0f;
    } else if (pl->stateGroup == PL_GROUP_ACTION && pl->state == PL_ACT_RIDE_STAGGER) {
        lim = 4.0f;
    } else {
        lim = 7.0f;
    }
    if (!(len < lim)) {
        if (pl->stateGroup == PL_GROUP_ACTION && pl->state == PL_ACT_RIDE_STAGGER) {
            return 1;
        }
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RIDE_STAGGER, 0);
        return 1;
    }
    return 0;
}

/*
 * Takes the stamina cost of a dash (Player_AddStamina): 15, or 8 with
 * SKILL_STAMINA_COST_DOWN, or 18 with SKILL_STAMINA_COST_UP.
 */
void Player_UseDashStamina(Player *pl)
{
    if (Player_HasSkill(pl, SKILL_STAMINA_COST_DOWN) == 1) {
        Player_AddStamina(pl, -8);
        return;
    }
    if (Player_HasSkill(pl, SKILL_STAMINA_COST_UP) == 1) {
        Player_AddStamina(pl, -18);
        return;
    }
    Player_AddStamina(pl, -15);
}

/*
 * The input on a climbable wall; returns 1 when it started a state, else 0.
 * The stick up (input.stickDir & PAD_UP) requests PL_ACT_WALL_CLIMB_UP_FAST
 * when PAD_R2 is held (taking the dash stamina, Player_UseDashStamina, unless
 * noTurn is nonzero), else PL_ACT_WALL_CLIMB_UP; the stick down does the same
 * with the DOWN states; PAD_CROSS requests PL_ACT_WALL_LET_GO.
 */
s32 Player_WallClimbInput(Player *pl, s16 noTurn)
{
    u16 stick = pl->input.stickDir;

    if (stick & 0x2000) {
        if (pl->input.held & PAD_R2) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_UP_FAST) == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_UP_FAST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                if (noTurn == 0) {
                    Player_UseDashStamina(pl);
                }
            }
        } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_UP) == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_UP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        return 1;
    }
    if (stick & 0x1000) {
        if (pl->input.held & PAD_R2) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_DOWN_FAST) == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_DOWN_FAST, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                if (noTurn == 0) {
                    Player_UseDashStamina(pl);
                }
            }
        } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_DOWN) == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_CLIMB_DOWN, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        return 1;
    }
    if (pl->input.pressed & PAD_CROSS) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_LET_GO, 0);
        return 1;
    }
    return 0;
}

/*
 * Starts the action of the action spot the player stands at
 * (Player_FindActionSpot gives its position, angle and kind), when mode
 * allows. Kind 1 moves to the spot and starts PL_ACT_SPIT_ROAST_START; 3
 * PL_ACT_SUPPLY_BOX; 4 PL_ACT_TAKE_REWARDS; 7 PL_ACT_DELIVERY_BOX (not once
 * the quest is over); 9 PL_ACT_START_COUNTDOWN (while gScene.countdownOn is
 * 0); 5 (the bed) opens PL_ACT_BED_MENU with mode 0, else moves to a place
 * beside the spot by the player's index (gSpotLineupOffsets) and starts
 * PL_ACT_BED_LIE_DOWN. Other kinds, or none, start PL_ACT_TRADE when a trader
 * is near (Player_NearTrader).
 */
void Player_StartSpotAction(Player *pl, s32 mode)
{
    s8 kind;
    u16 angle;
    f32 pos[3];
    f32 worldOffset[3];
    f32 localOffset[3];
    s32 ang[3];
    f32 rotMtx[16];

    switch (Player_FindActionSpot((Player *)pl, (f32 *)pos, &angle, (u8 *)&kind)) {
    case 0:
    case 2:
    case 6:
    case 8:
    default:
        if ((u8) Player_NearTrader(pl)) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TRADE, 0);
        }
        break;
    case 1:
        Vec_Copy(pl->moveTo, pos);
        Player_StartMoveTo(pl, 20);
        pl->targetYaw = angle;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_START, 0);
        break;
    case 3:
        if (pl->itemSelect == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SUPPLY_BOX, 0);
        }
        break;
    case 4:
        pl->targetYaw = angle;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_REWARDS, 0);
        break;
    case 5:
        if (mode == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_MENU, 0);
            break;
        }
        Vec_Copy(pl->moveTo, pos);
        ang[0] = 0;
        ang[1] = angle;
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, rotMtx);
        Vec_Set(localOffset, 120.0f * gSpotLineupOffsets[pl->index], 0.0f, 0.0f);
        Vec_Rotate(worldOffset, localOffset, rotMtx);
        pl->moveTo[0] += worldOffset[0];
        pl->moveTo[1] += worldOffset[1];
        pl->moveTo[2] += worldOffset[2];
        pl->targetYaw = angle;
        Player_StartMoveTo(pl, 20);
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_LIE_DOWN, 0);
        break;
    case 9:
        if (gScene.countdownOn == 0) {
            pl->targetYaw = Vec_Heading(&pl->pos[0], pos) - ANGLE_TURN / 4;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_START_COUNTDOWN, 0);
        }
        break;
    case 7:
        if (Quest_PhaseIsOver(0) != 1 && pl->itemSelect == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DELIVERY_BOX, 0);
        }
        break;
    }
}

/*
 * PAD_R1 held: each weapon's own action. mode 0 and 2 are for a sheathed
 * weapon (it is drawn into the action), 1 for a drawn one; mode 2 first drops
 * a carried item. Great sword: PL_REACT_GUARD_START (the lance, sword and
 * shield and gunlance draw into PL_REACT_GUARD_DRAW when sheathed); hammer:
 * Hammer_StartCharge; hunting horn: HuntingHorn_StartPlaying; dual swords:
 * the demon mode (DualSwords_DemonToggle; mode 1 can also end it); long
 * sword: LongSword_SpiritDraw; bow: Bow_DrawToAim (sheathed only).
 */
void Player_HoldSpecialInput(Player *pl, u8 mode)
{

    if (pl->input.held & PAD_R1) {
        if (mode == 0 || mode == 2) {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_START, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_SWORD_SHIELD:
            case WEAPON_GUNLANCE:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_DRAW, 0);
                break;
            case WEAPON_DUAL_SWORDS:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                if (pl->demonMode == 0) {
                    DualSwords_DemonToggle(pl, 0);
                }
                break;
            case WEAPON_HAMMER:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                Hammer_StartCharge(pl);
                break;
            case WEAPON_HUNTING_HORN:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                HuntingHorn_StartPlaying(pl, 1);
                break;
            case WEAPON_LONG_SWORD:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                LongSword_SpiritDraw(pl);
                break;
            case WEAPON_BOW:
                if (mode == 2 && (pl->carry & PL_CARRY_HOLD_MASK)) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                }
                Bow_DrawToAim(pl);
                break;
            }
        } else {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            case WEAPON_LANCE:
            case WEAPON_SWORD_SHIELD:
            case WEAPON_GUNLANCE:
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_START, 0);
                break;
            case WEAPON_HAMMER:
                Hammer_StartCharge(pl);
                break;
            case WEAPON_HUNTING_HORN:
                HuntingHorn_StartPlaying(pl, 0);
                break;
            case WEAPON_DUAL_SWORDS:
                if (pl->demonMode == 0) {
                    DualSwords_DemonToggle(pl, 0);
                } else {
                    DualSwords_DemonToggle(pl, 1);
                }
                break;
            }
        }
    }
}

/*
 * Starts gathering at a gather point or a mine point. mode 0 is PAD_CIRCLE
 * standing, 1 crouched, 2 the item button with a tool selected; nothing
 * happens near the trader (Player_NearTrader) unless mode is 2. At a gather
 * point (Player_GetGatherPoint) of kind 0, modes 0 and 1 enter PL_ACT_GATHER
 * or PL_ACT_GATHER_CROUCHED; mode 2 at kind 3 with a pickaxe selected faces
 * the point and enters PL_ACT_MINE, at kind 4 with a net PL_ACT_BUG_NET, and
 * the wrong tool plays sound 21. In modes 0 and 1 a mine point under the
 * player (MinePoint_FindNearPlayer) then enters PL_ACT_GATHER_DROP,
 * PL_ACT_GATHER_SPECIAL (type 5) or PL_ACT_CARVE (type 2), or their crouched
 * forms.
 */
void Player_TryGather(Player *pl, s32 mode)
{
    f32 pos[3];
    u16 kind;
    u16 id;
    u16 mineType;

    if ((u8) Player_NearTrader(pl) && (u8) mode != 2) {
        return;
    }
    if (Player_GetGatherPoint(pl, &kind, pos) == 0xFFFF) {
        goto other;
    }
    switch (kind) {
    case 0:
        if ((u8) mode == 2) {
            goto other;
        }
        if ((u8) mode == 0) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER, 0);
        } else {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_CROUCHED, 0);
        }
        break;
    case 3:
        if ((u8) mode == 2) {
            switch (pl->pouch[pl->pouchCursor].item) {
            case 150:
            case 151:
            case 152:
                pl->targetYaw = Vec_Heading(&pl->pos[0], pos) - ANGLE_TURN / 4;
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_MINE, 0);
                break;
            case 153:
            case 154:
            case 155:
                Sound_PlaySeFlat(0, 21, 0);
                break;
            }
        }
        break;
    case 4:
        if ((u8) mode == 2) {
            id = pl->pouch[pl->pouchCursor].item;
            switch (id) {
            case 153:
            case 154:
            case 155:
                pl->useItem = id;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_BUG_NET, 0);
                break;
            case 150:
            case 151:
            case 152:
                Sound_PlaySeFlat(0, 21, 0);
                break;
            }
        }
        break;
    default:
    other:
        switch (pl->pouch[pl->pouchCursor].item) {
        case 150:
        case 151:
        case 152:
        case 153:
        case 154:
        case 155:
            if ((u8) mode == 2) {
                Sound_PlaySeFlat(0, 21, 0);
            }
            break;
        }
        break;
    }
    if ((u8) mode != 2) {
        mineType = MinePoint_FindNearPlayer(pl);
        if (mineType != 0xFFFF) {
            if ((u8) mode == 0) {
                switch (mineType) {
                default:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_DROP, 0);
                    break;
                case 5:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_SPECIAL, 0);
                    break;
                case 2:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CARVE, 0);
                    break;
                }
            } else {
                switch (mineType) {
                default:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_DROP_CROUCHED, 0);
                    break;
                case 5:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GATHER_SPECIAL_CROUCHED, 0);
                    break;
                case 2:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CARVE_CROUCHED, 0);
                    break;
                }
            }
        }
    }
}

/*
 * The input of the crouched states; always returns 0 (another player's copy
 * online does nothing). By gScene.questPhase it requests the quest's start
 * (phase 4, while questStartDone is 0), end (5, 6), abort (7) or leave (8) event
 * state. Otherwise it sets freeState and, by Player_StickMoveLevel, walks
 * crouched (PL_ACT_CROUCH_WALK, or PL_ACT_CROUCH_WALK_FAST at level 3) or
 * stops (PL_ACT_CROUCH_STOP); PAD_CROSS stands up (PL_ACT_CROUCH_STAND_UP)
 * unless mustCrouch is set. Unless the quest is over, it then starts requested
 * gestures, draws the weapon on a right-stick flick (PL_ACT_DRAW), runs
 * Player_HoldSpecialInput, uses an item on PAD_SQUARE (Player_TryGather mode
 * 2, Player_UseItem) and handles PAD_CIRCLE (an offered item, a spot,
 * gathering) or a pending offer (PL_ACT_OFFER_ITEM).
 */
s32 Player_CrouchInput(Player *pl)
{

    if (Player_IsLocal(pl) == 0 && Game_IsOnline() == 1) {
        return 0;
    }
    switch (gScene.questPhase) {
    case 4:
        if (pl->questStartDone == 0) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_START, 0);
            return 0;
        }
        break;
    case 6:
    case 5:
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_END, 0);
        return 0;
    case 7:
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_ABORT, 0);
        return 0;
    case 8:
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, 0);
        return 0;
    }
    pl->freeState = 1;
    switch ((u8) Player_StickMoveLevel(pl, 0)) {
    case 1:
    case 2:
        Player_FaceStick(pl);
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK) == 0) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK_FAST) != 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_STOP, 0);
            } else {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
        }
        break;
    case 3:
        Player_FaceStick(pl);
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK_FAST) == 0) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK_FAST, 0);
        }
        break;
    case 0:
    default:
        if ((Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK_FAST) != 0) && pl->timer == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_STOP, 0);
        }
        break;
    }
    if ((pl->input.pressed & PAD_CROSS) && pl->mustCrouch == 0) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_STAND_UP, 0);
    }
    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    if (pl->actRequest != 0) {
        Player_StartGesture(pl);
    }
    if (pl->mustCrouch == 0) {
        if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
        }
        Player_HoldSpecialInput(pl, 0);
    }
    if ((pl->input.pressed & PAD_SQUARE) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
        Player_TryGather(pl, 2);
        Player_UseItem(pl, 0);
        pl->pouchHudFlags |= 1;
    }
    if (pl->input.pressed & PAD_CIRCLE) {
        if (Player_TakeOfferedItem(pl) == 0) {
            Player_StartSpotAction(pl, 0);
            Player_TryGather(pl, 1);
        }
    } else if (pl->offerRequest != 0) {
        pl->freeState = 0;
        pl->offerStarted = 1;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM, PL_SET_SYNC_OFFER);
    }
    return 0;
}

/* The weapons input checks (the states cancel into another action), for the local player or a human one
 * when input is allowed. */
void Player_WeaponAttackInput(Player *pl, s32 arg)
{

    if ((Player_IsLocal(pl) != 0 || Game_IsOnline() != 1) && pl->itemSelect == 0 && Quest_PhaseIsOver(1) != 1) {
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
            GreatSword_AttackInput(pl);
            break;
        case WEAPON_LONG_SWORD:
            LongSword_AttackInput(pl);
            break;
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_LIGHT_BOWGUN:
            Bowgun_AttackInput(pl);
            break;
        case WEAPON_BOW:
            Bow_AttackInput(pl, arg);
            break;
        case WEAPON_HAMMER:
            Hammer_AttackInput(pl);
            break;
        case WEAPON_HUNTING_HORN:
            HuntingHorn_AttackInput(pl);
            break;
        case WEAPON_LANCE:
            Lance_AttackInput(pl);
            break;
        case WEAPON_GUNLANCE:
            Gunlance_AttackInput(pl);
            break;
        case WEAPON_SWORD_SHIELD:
            SwordShield_AttackInput(pl);
            break;
        case WEAPON_DUAL_SWORDS:
            DualSwords_AttackInput(pl);
            return;
        }
    }
}

/*
 * The input while the guard is held, for the local player (any player
 * offline) before the quest's end. Lance: the stick walks (PL_ACT_GUARD_WALK,
 * unless motion PL_MOT_WPN_GUARD_WALK plays), PAD_R3 and a right-stick flick start
 * Lance_GuardAttack (0 and 1). Gunlance: Gunlance_GuardInput, else the same
 * guard walk. Sword and shield: PAD_CROSS with stamina 75 or more rolls
 * (PL_ACT_ROLL), a flick up gives SwordShield_GuardSlash. Great sword and
 * long sword: PAD_CROSS with stamina 75 or more rolls.
 */
void Player_GuardInput(Player *pl)
{
    u8 weapon;

    if (((Player_IsLocal(pl) != 0) || (Game_IsOnline() != 1)) && (Quest_PhaseIsOver(1) != 1)) {
        weapon = pl->weaponType;
        switch (weapon) {
        case 3:
            if ((Player_StickMoveLevel(pl, 0) & 0xFF) && (pl->motionId[0] != 1011)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GUARD_WALK, PL_SET_SNAP_POS);
            }
            if (pl->input.pressed & PAD_R3) {
                Lance_GuardAttack(pl, 0);
            }
            if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                Lance_GuardAttack(pl, 1);
            }
            break;
        case 9:
            if ((Gunlance_GuardInput(pl, 0) == 0) && (Player_StickMoveLevel(pl, 0) & 0xFF) && (pl->motionId[0] != 1011)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_GUARD_WALK, PL_SET_SNAP_POS);
            }
            break;
        case 4:
            if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
            if (pl->input.stickDirPressed & PAD_RSTICK_UP) {
                SwordShield_GuardSlash(pl);
            }
            break;
        case 0:
        case 7:
            if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            }
            break;
        }
    }
}

/*
 * Turns the player to run along the wall it touches (rot[1] and targetYaw set
 * to the heading of the wall's normal plus or minus a quarter turn); returns 0 for none (not
 * in the current area, or no wall hit), 1 or 2. It reads the first of the
 * player's wall contacts (gEmWallContacts, ended by flags 0): a contact with
 * WALL_CONTACT_SPECIAL and a wall ahead (WALL_DIRS_AHEAD) turns by
 * -ANGLE_TURN / 4 for side 0 or +ANGLE_TURN / 4 for side 1 and returns 1; any other contact
 * turns by +ANGLE_TURN / 4 only for side 1 and returns 2. Player_CheckClimb starts
 * PL_ACT_WALL_GRAB on 1.
 */
s8 Player_TurnAlongWall(Player *pl, s16 side)
{
    s16 i;
    u16 flags;
    u16 ang;

    if ((u8)Obj_InCurrentArea(pl) == 0) {
        return 0;
    }
    if (pl->wallHitDirs != 0) {
        for (i = 0; i < 10; i++) {
            flags = gEmWallContacts[pl->index][i].flags;
            if (flags == 0) {
                break;
            }
            ang = Math_Heading(gEmWallContacts[pl->index][i].normal[0], gEmWallContacts[pl->index][i].normal[2], 0.0f, 0.0f);
            if ((flags & WALL_CONTACT_SPECIAL) && (pl->wallHitDirs & WALL_DIRS_AHEAD)) {
                if (side == 0) {
                    pl->rot[1] = (u16) (ang - ANGLE_TURN / 4);
                } else {
                    pl->rot[1] = (u16) (ang + ANGLE_TURN / 4);
                }
                pl->targetYaw = pl->rot[1];
                return 1;
            }
            if (flags != 0) {
                if (side == 1) {
                    pl->rot[1] = (u16) (ang + ANGLE_TURN / 4);
                    pl->targetYaw = pl->rot[1];
                }
                return 2;
            }
        }
    }
    return 0;
}

/*
 * As Player_TurnAlongWall for the wall at one side: side 0 needs a wall ahead
 * (WALL_DIRS_AHEAD), side 1 one behind (WALL_DIRS_BEHIND). Side 0
 * with a WALL_CONTACT_SPECIAL contact turns by -ANGLE_TURN / 4 and returns 1, side 1 turns by
 * +ANGLE_TURN / 4 and returns 2. Returns 0 otherwise.
 */
s32 Player_TurnAlongWallSide(Player *pl, s16 side)
{
    s16 i;
    u16 flags;
    u16 ang;

    if ((u8)Obj_InCurrentArea(pl) == 0) {
        return 0;
    }
    if (pl->wallHitDirs & (side == 0 ? WALL_DIRS_AHEAD : WALL_DIRS_BEHIND)) {
        for (i = 0; i < 10; i++) {
            flags = gEmWallContacts[pl->index][i].flags;
            if (flags == 0) {
                break;
            }
            ang = Math_Heading(gEmWallContacts[pl->index][i].normal[0], gEmWallContacts[pl->index][i].normal[2], 0.0f, 0.0f);
            if ((flags & WALL_CONTACT_SPECIAL) && side == 0) {
                pl->rot[1] = (u16) (ang - ANGLE_TURN / 4);
                pl->targetYaw = pl->rot[1];
                return 1;
            }
            if (side == 1) {
                pl->rot[1] = (u16) (ang + ANGLE_TURN / 4);
                pl->targetYaw = pl->rot[1];
                return 2;
            }
        }
    }
    return 0;
}

/*
 * Starts a climb or a hop when the player runs at a wall or a ledge; returns
 * 1 when it started a state, else 0. Player_TurnAlongWall returning 1 enters
 * PL_ACT_WALL_GRAB. The floor 55 units ahead (Player_FloorAheadAbove) decides
 * the rest: 60 to 180 above the player requests PL_ACT_STEP_UP, 180 to 210
 * PL_ACT_LEDGE_GRAB, 210 to 300 PL_ACT_JUMP_TO_LEDGE.
 */
u8 Player_CheckClimb(Player *pl)
{
    f32 dist;
    u8 ret = 0;

    switch (Player_TurnAlongWall(pl, 0)) {
    case 0:
        break;
    case 1:
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_GRAB, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        ret = 1;
        break;
    }
    if (Player_FloorAheadAbove(pl, &dist, 55.0f, 60.0f, 120.0f) != 0) {
        pl->targetYaw = pl->rot[1];
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_STEP_UP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        ret = 1;
    } else if (Player_FloorAheadAbove(pl, &dist, 55.0f, 180.0f, 30.0f) != 0) {
        pl->targetYaw = pl->rot[1];
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        ret = 1;
    } else if (Player_FloorAheadAbove(pl, &dist, 55.0f, 210.0f, 90.0f) != 0) {
        pl->targetYaw = pl->rot[1];
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_JUMP_TO_LEDGE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        ret = 1;
    }
    return ret;
}

/* Uses the selected pouch item: in mode 2 only ITEM_ID_MONSTER_BOMB (when Item_RideItemUsable
 * allows it); otherwise each item checks where it may be used (out of camp, a free mine point,
 * an action spot, no status 6) and starts its action state, or plays the refusal sound. */
void Player_UseItem(Player *pl, u8 mode)
{
    f32 spotPos[3];
    u16 ang;
    u8 k;

    if (Pouch_CursorCount(pl) <= 0) {
        Sound_PlaySeFlat(0, 21, 0);
        if (mode == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        return;
    }
    if (mode == 2) {
        switch (pl->pouch[pl->pouchCursor].item) {
        case ITEM_ID_MONSTER_BOMB:
            if (Item_RideItemUsable() == 1) {
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            } else {
                Sound_PlaySeFlat(0, 21, 0);
            }
            break;
        default:
            Sound_PlaySeFlat(0, 21, 0);
            break;
        }
        return;
    }
    switch (pl->pouch[pl->pouchCursor].item) {
    case 76:
    case 75:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        if (!Player_TestFlag(pl, 0x2000600)) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM_ALT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        break;
    case 77:
    case 698:
    case 79:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM_ALT, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case 78:
        if (Item_UsableOutsideCamp() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RETURN_TO_CAMP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 72:
    case 663:
    case 71:
    case 215:
    case 73:
    case 65:
    case 66:
    case 67:
    case 68:
    case 699:
    case 700:
    case 64:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case 80:
    case 69:
        if (Quest_IsVersus() == 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_THROW_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        }
        break;
    case ITEM_ID_SMALL_BARREL_BOMB:
    case ITEM_ID_SMALL_BARREL_BOMB_B:
    case ITEM_ID_LAUNCHED_BARREL_BOMB:
    case ITEM_ID_LAUNCHED_BARREL_BOMB_B:
        if (Item_CanUseSmallBarrelBomb() == 1) {
            goto drink;
        }
        Sound_PlaySeFlat(0, 21, 0);
        if (mode == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    case ITEM_ID_NO_BOMB_AREA_ITEM:
        if (Scene_InNoBarrelBombArea() == 1) {
            goto drink;
        }
        Sound_PlaySeFlat(0, 21, 0);
        if (mode == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    case ITEM_ID_MEAT:
    case ITEM_ID_BAIT_POISON:
    case ITEM_ID_BAIT_STATUS2:
    case ITEM_ID_BAIT_STATUS4:
        if (Scene_CanPlaceKind3() == 1) {
        drink:
            if (pl->action == PL_POSTURE_STANDING) {
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_KNEEL_TO_PLACE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            } else if (pl->action == PL_POSTURE_CROUCHED) {
                pl->useItem = pl->pouch[pl->pouchCursor].item;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_KNEEL_TO_PLACE_CROUCHED, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_LARGE_BARREL_BOMB:
    case ITEM_ID_LARGE_BARREL_BOMB_B:
    case ITEM_ID_SUPPLY_LARGE_BARREL_BOMB:
        if (Item_CanPlaceLargeBarrelBomb() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_PLACED_BOMB:
        if (Item_CanPlaceKind1() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_TRAP:
    case ITEM_ID_SUPPLY_TRAP:
        if (Item_CanPlaceKind2() == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_PITFALL_TRAP:
    case ITEM_ID_SUPPLY_PITFALL_TRAP:
        if (Player_GetAreaPitModel(pl) >= 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
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
        if (SpecialMine_IsFree(pl) == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 142:
    case 143:
    case 144:
    case 145:
    case 146:
    case 147:
    case 148:
    case 149:
        if (Player_FindActionSpot((Player *)pl, (f32 *)spotPos, &ang, (u8 *)&k) == 2) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            pl->targetYaw = ang;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_CAST, 0);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_DELIVERY:
        if (Player_FindActionSpot((Player *)pl, (f32 *)spotPos, &ang, (u8 *)&k) == 11) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 667:
        if (Player_FindActionSpot((Player *)pl, (f32 *)spotPos, &ang, (u8 *)&k) == 6) {
            pl->moveTo[0] = spotPos[0];
            pl->moveTo[1] = spotPos[1];
            pl->moveTo[2] = spotPos[2];
            pl->targetYaw = ang;
            Player_StartMoveTo(pl, 20);
            pl->stateRand = k;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM, PL_SET_KEEP_RAND);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 695:
        if (Player_FindActionSpot((Player *)pl, (f32 *)spotPos, &ang, (u8 *)&k) == 10) {
            pl->targetYaw = Vec_Heading(pl->pos, spotPos) - ANGLE_TURN / 4;
            pl->stateRand = k;
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPOT_ITEM_STRIKE, PL_SET_KEEP_RAND);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case ITEM_ID_HEAL_20:
    case ITEM_ID_CURE_POISON:
    case ITEM_ID_HEAL_10_CURE:
    case ITEM_ID_HEAL_30:
    case ITEM_ID_HEAL_50:
    case ITEM_ID_SUPPLY_HEAL_30:
    case ITEM_ID_MAX_HP_10:
    case ITEM_ID_MAX_HP_20:
    case ITEM_ID_MAX_HP_FULL:
    case ITEM_ID_SUPPLY_MAX_HP_FULL:
    case ITEM_ID_MAX_ALL:
    case ITEM_ID_STAMINA_LOCK:
    case ITEM_ID_STAMINA_LOCK_LONG:
    case ITEM_ID_ATTACK_DRUG:
    case ITEM_ID_ATTACK_DRUG_STRONG:
    case ITEM_ID_DEFENSE_DRUG:
    case ITEM_ID_DEFENSE_DRUG_STRONG:
    case ITEM_ID_SUPPLY_MAX_STAMINA_75:
    case ITEM_ID_HURT_CURE_CHANCE:
    case ITEM_ID_CURE_CHANCE:
    case ITEM_ID_POISON_GAMBLE:
    case ITEM_ID_ATTACK_SHARED:
    case ITEM_ID_DEFENSE_SHARED:
    case ITEM_ID_COOL_DRINK:
    case ITEM_ID_HOT_DRINK:
    case ITEM_ID_HEAL_50_TEAM:
    case ITEM_ID_DETECT:
    case ITEM_ID_REGEN:
    case ITEM_ID_MAX_STAMINA_75_B:
    case ITEM_ID_ATTACK_BURST:
    case ITEM_ID_DEFENSE_BURST:
    case ITEM_ID_MAX_STAMINA_75:
    case ITEM_ID_RANDOM:
        if (Player_Status6Active(pl) == 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DRINK, 0);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 660:
    case 156:
    case 157:
    case 159:
    case 158:
    case 160:
        if (Pouch_HasMeat(pl) == 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_START, 0);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 164:
    case 165:
    case 166:
    case 167:
    case 163:
    case 168:
    case 169:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_START, 0);
        break;
    case 172:
    case 661:
    case 234:
    case 242:
        if (pl->isGunner != 1) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SHARPEN, 0);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 175:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_HORN_PLAY, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case ITEM_ID_TEAM_HEAL:
    case ITEM_ID_TEAM_CURE:
    case ITEM_ID_TEAM_ATTACK:
    case ITEM_ID_TEAM_DEFENSE:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_TEAM_ITEM_USE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        break;
    case ITEM_ID_FOOD_STAMINA_75:
    case ITEM_ID_FOOD_STAMINA_150:
    case ITEM_ID_FOOD_STAMINA_FULL:
    case ITEM_ID_FOOD_STAMINA_GAMBLE:
    case ITEM_ID_FOOD_HEAL_20:
    case ITEM_ID_FOOD_PRIZE_A:
    case ITEM_ID_FOOD_PRIZE_B:
    case ITEM_ID_FOOD_HOT:
    case ITEM_ID_FOOD_HOT_STAMINA:
    case ITEM_ID_FOOD_COOL:
    case ITEM_ID_FOOD_COOL_STAMINA:
    case ITEM_ID_FOOD_CURE:
    case ITEM_ID_FOOD_CURE_STAMINA:
    case ITEM_ID_FOOD_RESTORE:
    case ITEM_ID_FOOD_REGEN_SHORT:
    case ITEM_ID_FOOD_REGEN:
    case ITEM_ID_FOOD_REGEN_B:
    case ITEM_ID_FOOD_REGEN_LONG:
    case ITEM_ID_FOOD_RESTORE_B:
        if (Player_Status6Active(pl) == 0) {
            pl->useItem = pl->pouch[pl->pouchCursor].item;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_EAT, 0);
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 173:
        if (Player_LocalCanUseBinoculars() == 1) {
            if (pl->action == PL_POSTURE_CROUCHED) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS_CROUCHED, 0);
            } else {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS, 0);
            }
        } else {
            Sound_PlaySeFlat(0, 21, 0);
            if (mode == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        }
        break;
    case 150:
    case 151:
    case 152:
    case 153:
    case 154:
    case 155:
        break;
    case 688:
    case 689:
    case 690:
    case 691:
    case 692:
    case 693:
    case 694:
        pl->useItem = pl->pouch[pl->pouchCursor].item;
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_HORN_ITEM_PLAY, 0);
        break;
    default:
        Sound_PlaySeFlat(0, 21, 0);
        if (mode == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

extern s32 gPouchFullTakeText;
/*
 * Returns 1 when the player starts taking an item another player holds out to
 * it, else 0 (always 0 offline, and while pl->offerNoticeTimer runs). The
 * giver is another active player in the same area, in PL_ACT_OFFER_ITEM,
 * offering to this player (offerTo), within 300 units, with an item of rarity
 * below 3. If the pouch has room it copies offerItem and offerCount, tells
 * the giver when the player is local (QSYNC_PL_ITEM_ASK) and requests
 * PL_ACT_TAKE_ITEM_WAIT; otherwise it pushes a notice and sets
 * offerNoticeTimer to 90.
 */
s32 Player_TakeOfferedItem(Player *pl)
{
    s16 i;
    Player *giver;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (pl->offerNoticeTimer != 0) {
        return 0;
    }
    giver = gPlayers;
    for (i = 0; i < gScene.playerCount; i++, giver++) {
        if (i != pl->index && giver->active != 0 && giver->area == pl->area && Player_IsState(giver, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM) != 0
            && giver->offerTo == pl->index && Vec_Distance(pl->pos, giver->pos) <= 300.0f && gItemData[giver->offerItem].rarity < 3) {
            if (Pouch_Count(pl, giver->offerItem) == 0) {
                if (Pouch_HasFreeSlot(pl) != 0) {
                    pl->offerItem = giver->offerItem;
                    pl->offerCount = giver->offerCount;
                    if (Player_IsLocal(pl) == 1) {
                        QSync_SendPlayer(pl, QSYNC_PL_ITEM_ASK, giver->index);
                    }
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM_WAIT, 0);
                    return 1;
                }
                Notice_PushItem((char *)gPouchFullTakeText);
                pl->offerNoticeTimer = 90;
            } else if (Pouch_Room(pl, giver->offerItem) >= giver->offerCount) {
                pl->offerItem = giver->offerItem;
                pl->offerCount = giver->offerCount;
                if (Player_IsLocal(pl) == 1) {
                    QSync_SendPlayer(pl, QSYNC_PL_ITEM_ASK, giver->index);
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM_WAIT, 0);
                return 1;
            } else {
                Notice_Push(NOTICE_KIND_ITEM, 15, giver->offerItem);
                pl->offerNoticeTimer = 90;
            }
            return 0;
        }
    }
    return 0;
}

/*
 * The idle input of the local player (others return at once);
 * Player_StateIdle calls it with arg 0, Player_StateMove with 1 (variant 9),
 * and arg goes to Bow_AttackInput as needPress. By gScene.questPhase it
 * requests the quest's start, end, abort or leave state. Riding a monster
 * (rideState) it throws the player off (PL_REACT_TOSSED_BACK) while the giant
 * it stands on has quakeTimer set, else walks (PL_ACT_RIDE_WALK) and handles
 * the offer, spot, gather and item buttons. On foot with the weapon drawn it
 * walks (PL_ACT_WALK_DRAWN), sheathes on PAD_CIRCLE, PAD_SQUARE or PAD_R2
 * with the stick, and runs Player_HoldSpecialInput and
 * Player_WeaponAttackInput. Sheathed, Player_StickMoveLevel picks the walks,
 * the run (or PL_ACT_TURN_AROUND) or the dash (PL_ACT_FLEE_DASH when fighting
 * a monster not ahead, else PL_ACT_DASH); PAD_CROSS rolls or crouches,
 * PAD_SQUARE gathers or uses an item, PAD_CIRCLE climbs or takes the offer,
 * spot or gather point, a right-stick flick draws or attacks as it draws, and
 * PAD_R3 starts PL_WPN_SHARED_ATTACK. Both also start an item offer, the
 * battle alert (PL_ACT_BATTLE_ALERT, when Player_EnteredBattle returns 1),
 * the stun (PL_REACT_STUN) and the mash status (PL_REACT_MASH_START).
 */
void Player_IdleActions(Player *pl, s32 arg)
{
    Em *monster = gEms;
    s16 i;
    u8 k;
    u8 stickLevel;
    u8 drew;
    u16 yawDelta;

    if (Game_IsOnline() == 0) {
        if (Player_IsLocal(pl) == 0) {
            return;
        }
    } else if (Player_IsLocal(pl) == 0) {
        return;
    }
    drew = 0;
    if (gScene.questPhase == 4 && pl->questStartDone == 0) {
        Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_START, 0);
        return;
    }
    pl->freeState = 1;
    if (pl->rideState != 0) {
        k = pl->rideEm;
        for (i = 0; i < 20; i++, monster++) {
            if (k == monster->slotNo && (monster->type == 7 || monster->type == 50 || monster->type == 58 || monster->type == 72 ||
                                   monster->type == 55)) {
                if (monster->quakeTimer != 0) {
                    Player_SetState(pl, PL_GROUP_REACT, PL_REACT_TOSSED_BACK, 0);
                    return;
                }
                break;
            }
        }
        if ((u8) Player_StickMoveLevel(pl, 1) > 0 && pl->state != 18) {
            Player_FaceStick(pl);
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RIDE_WALK, 0);
        }
        Player_CheckRideSlip(pl);
        if (Quest_PhaseIsOver(1) == 1) {
            return;
        }
        if (pl->input.pressed & PAD_CIRCLE) {
            if (Player_TakeOfferedItem(pl) == 0) {
                Player_StartSpotAction(pl, 0);
                Player_TryGather(pl, 0);
            }
        }
        if ((pl->input.pressed & PAD_SQUARE) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
            Player_UseItem(pl, 2);
            pl->pouchHudFlags |= 1;
        }
    } else {
        if (gScene.questPhase == 6 || gScene.questPhase == 5) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_END, 0);
            return;
        }
        if (gScene.questPhase == 7) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_ABORT, 0);
            return;
        }
        if (gScene.questPhase == 8) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, 0);
            return;
        }
        if (pl->weaponOut != 0) {
            stickLevel = Player_StickMoveLevel(pl, 1);
            switch (stickLevel) {
            case 0:
                break;
            case 1:
            case 2:
            case 3:
                if (pl->state != 36 && pl->state != 3 && pl->state != 116 && pl->mustCrouch == 0) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK_DRAWN, 0);
                }
                break;
            case 4:
                break;
            }
            if (stickLevel != 0 && pl->mustCrouch != 0 && pl->state != 59) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
            if (Quest_PhaseIsOver(1) == 1) {
                return;
            }
            if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
                if ((pl->input.pressed & PAD_CIRCLE) || ((pl->input.pressed & PAD_R2) && pl->input.stickTilt >= PL_STICK_HELD)) {
                    if (Player_TestFlag(pl, 4608) == 0) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE_MOVING, 0);
                    }
                } else if ((pl->input.pressed & PAD_SQUARE) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
                    Player_TryGather(pl, 2);
                    Player_UseItem(pl, 0);
                    pl->pouchHudFlags |= 1;
                }
                break;
            default:
                if ((pl->input.pressed & (PAD_SQUARE | PAD_CIRCLE)) || ((pl->input.pressed & PAD_R2) && pl->input.stickTilt >= PL_STICK_HELD)) {
                    if (Player_TestFlag(pl, 4608) == 0) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_SHEATHE_MOVING, 0);
                    }
                    pl->pouchHudFlags |= 1;
                }
                break;
            }
            Player_HoldSpecialInput(pl, 1);
            if ((Player_AmmoTypeChanged(pl) & 0xFF) == 0) {
                Player_WeaponAttackInput(pl, arg);
            }
            if (pl->offerRequest != 0) {
                pl->freeState = 0;
                pl->offerStarted = 1;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM, PL_SET_SYNC_OFFER);
            }
            if (Player_Status5Full(pl) & 0xFF) {
                pl->freeState = 0;
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_STUN, 0);
                return;
            }
            if (Player_HasMashBuildup(pl) & 0xFF) {
                pl->freeState = 0;
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_START, 0);
            }
        } else {
            stickLevel = Player_StickMoveLevel(pl, 0);
            switch (stickLevel) {
            case 0:
                break;
            case 1:
                if (pl->state != 73 && pl->state != 36 && pl->mustCrouch == 0) {
                    Player_FaceStick(pl);
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK_SLOW, 0);
                }
                break;
            case 2:
                if (pl->state != 2 && pl->state != 36 && pl->mustCrouch == 0) {
                    Player_FaceStick(pl);
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALK, 0);
                }
                break;
            case 3:
                if (pl->state != 1 && pl->state != 63 && pl->state != 19 && pl->state != 36 &&
                    pl->state != 31 && pl->mustCrouch == 0) {
                    Player_FaceStick(pl);
                    yawDelta = pl->targetYaw + ANGLE_TURN - pl->rot[1];
                    if (yawDelta >= 0x6000U && yawDelta < 0xA001U) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_TURN_AROUND, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN, 0);
                    }
                }
                break;
            case 4:
                break;
            case 5:
                if (Player_TestFlag(pl, 0x200) == 0) {
                    if (pl->inBattle != 0 && (u16) (pl->targetAngle - pl->rot[1] + ANGLE_TURN / 8) > 0x4000) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DASH, 0);
                    }
                    drew = 1;
                }
                break;
            }
            if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
            if (stickLevel != 0 && pl->mustCrouch != 0 && pl->state != 59) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
            if (Quest_PhaseIsOver(1) == 1) {
                return;
            }
            if (pl->input.pressed & PAD_CROSS) {
                if (pl->input.stickTilt >= 85) {
                    if (pl->stamina >= 75) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                    }
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH, 0);
                }
            }
            if ((pl->input.pressed & PAD_SQUARE) && pl->itemUseDelay == 0 && pl->itemSelect == 0) {
                Player_TryGather(pl, 2);
                Player_UseItem(pl, 0);
                pl->pouchHudFlags |= 1;
            }
            Player_HoldSpecialInput(pl, 0);
            if ((drew == 0 ? pl->input.pressed : pl->input.held) & 0x20) {
                Player_CheckClimb(pl);
            }
            if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                if (Player_TestFlag(pl, 0x2000600) == 0) {
                    if (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) {
                        Bowgun_DrawAttack(pl, 0);
                    } else {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
                    }
                } else {
                    switch (pl->weaponType) {
                    case WEAPON_GREAT_SWORD:
                    default:
                        GreatSword_DrawAttack(pl);
                        break;
                    case WEAPON_HEAVY_BOWGUN:
                    case WEAPON_LIGHT_BOWGUN:
                        Bowgun_DrawAttack(pl, 1);
                        break;
                    case WEAPON_BOW:
                        Bow_DrawAttack(pl);
                        break;
                    case WEAPON_SWORD_SHIELD:
                        SwordShield_DrawAttack(pl);
                        break;
                    case WEAPON_DUAL_SWORDS:
                        DualSwords_DrawAttack(pl);
                        break;
                    case WEAPON_LANCE:
                        Lance_DrawAttack(pl);
                        break;
                    case WEAPON_GUNLANCE:
                        Gunlance_DrawAttack(pl);
                        break;
                    case WEAPON_HAMMER:
                        Hammer_DrawAttack(pl);
                        break;
                    case WEAPON_HUNTING_HORN:
                        HuntingHorn_DrawAttack(pl);
                        break;
                    case WEAPON_LONG_SWORD:
                        LongSword_DrawAttack(pl);
                        break;
                    }
                }
            }
            if (pl->input.pressed & PAD_R3) {
                Player_SetState(pl, PL_GROUP_WEAPON, PL_WPN_SHARED_ATTACK, PL_SET_SNAP_POS);
            }
            if (pl->input.pressed & PAD_CIRCLE) {
                if (Player_TakeOfferedItem(pl) == 0) {
                    Player_StartSpotAction(pl, 0);
                    Player_TryGather(pl, 0);
                }
            } else if (pl->offerRequest != 0) {
                pl->freeState = 0;
                pl->offerStarted = 1;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM, PL_SET_SYNC_OFFER);
            }
            if ((Player_EnteredBattle((Em *) pl) & 0xFF) == 1) {
                pl->freeState = 0;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_BATTLE_ALERT, 0);
                CamCtl_SetTargetObj((u8 *) pl);
            }
            if (Player_Status5Full(pl) & 0xFF) {
                pl->freeState = 0;
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_STUN, 0);
                return;
            }
            if (Player_HasMashBuildup(pl) & 0xFF) {
                pl->freeState = 0;
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_START, 0);
            }
        }
    }
}

/*
 * The aim of a bowgun in its drawn idle (motion PL_MOT_WPN_IDLE) for the local gunner.
 * It sets aimLockTimer (10, or 5 after a vertical change) while the aim mode
 * or a recent aim holds, and blends the aim layers (Player_SetAimLayers,
 * motions 1005 and 1006); without them it needs PAD_L1 held and PAD_L2 not.
 * The D-pad up and down change pl->aimWeight by 5 (Bowgun_AddAimWeight,
 * reversed by gScene.aimReverse in the aim mode), left and right turn
 * pl->targetYaw by 0x240. A left-stick tilt of 40 or more does the same in
 * steps that grow with the tilt.
 */
void Player_BowgunAim(Player *pl)
{
    s8 stickStep;
    s8 dpadStep;
    u16 yawStep;

    if (Player_IsLocal(pl) == 0) {
        return;
    }
    if (pl->isGunner != 1) {
        return;
    }
    if (pl->motionId[0] != 1001) {
        return;
    }
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        pl->aimLockTimer = 10;
    }
    Player_SetAimLayers(pl, 1005, 1006);
    if (pl->aimLockTimer == 0 && pl->aimMode == 0) {
        if (pl->input.held & PAD_L2) {
            return;
        }
        if (!(pl->input.held & PAD_L1)) {
            return;
        }
    }
    if (pl->input.held & (PAD_UP | PAD_DOWN)) {
        dpadStep = 5;
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            dpadStep *= -1;
        }
        if (pl->input.held & PAD_UP) {
            Bowgun_AddAimWeight(pl, dpadStep);
        } else {
            Bowgun_AddAimWeight(pl, (s8) -dpadStep);
        }
        pl->aimLockTimer = 5;
    }
    if (pl->input.held & PAD_LEFT) {
        pl->aimLockTimer = 10;
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            pl->targetYaw -= 0x240;
        } else {
            pl->targetYaw += 0x240;
        }
    }
    if (pl->input.held & PAD_RIGHT) {
        pl->aimLockTimer = 10;
        if (pl->aimMode != 0 && gScene.aimReverse == 2) {
            pl->targetYaw += 0x240;
        } else {
            pl->targetYaw -= 0x240;
        }
    }
    Player_SetAimLayers(pl, 1005, 1006);
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        if (!(pl->input.stickDir & PAD_DPAD) || pl->input.stickTilt < PL_STICK_HELD) {
            return;
        }
        stickStep = 1;
        yawStep = 76;
        if (pl->input.stickTilt >= 120) {
            stickStep = 8;
            yawStep = 0x200;
        } else if (pl->input.stickTilt >= 90) {
            stickStep = 4;
            yawStep = 192;
        } else if (pl->input.stickTilt >= 60) {
            stickStep = 2;
            yawStep = 128;
        }
        if (pl->input.stickDir & (PAD_UP | PAD_DOWN)) {
            if (gScene.aimReverse != 0) {
                stickStep = -stickStep;
            }
            stickStep = (s8) stickStep;
            if (pl->input.stickDir & PAD_UP) {
                Bowgun_AddAimWeight(pl, stickStep);
            } else {
                Bowgun_AddAimWeight(pl, (s8) -stickStep);
            }
            pl->aimLockTimer = 5;
            Player_SetAimLayers(pl, 1005, 1006);
        }
        if (pl->input.stickDir & PAD_LEFT) {
            pl->aimLockTimer = 10;
            if (gScene.aimReverse == 2) {
                pl->targetYaw -= yawStep;
            } else {
                pl->targetYaw += yawStep;
            }
        }
        if (pl->input.stickDir & PAD_RIGHT) {
            pl->aimLockTimer = 10;
            if (gScene.aimReverse == 2) {
                pl->targetYaw += yawStep;
            } else {
                pl->targetYaw -= yawStep;
            }
        }
    }
    Player_SetAimLayers(pl, 1005, 1006);
}

/*
 * Adds delta to pl->aimPitch, keeping it within ANGLE_TURN / 8 (45 degrees) either
 * way.
 */
static void Player_AddAimPitch(Player *pl, s16 delta)
{
    pl->aimPitch += delta;
    if (delta >= 0) {
        if (pl->aimPitch >= ANGLE_TURN / 8) {
            pl->aimPitch = ANGLE_TURN / 8;
        }
    } else {
        if (pl->aimPitch <= -0x2000) {
            pl->aimPitch = -(ANGLE_TURN / 8);
        }
    }
}

/*
 * Adds delta to pl->aimPitchPct (kept within -100 to 100) and sets
 * pl->aimPitch from it: the percentage of 45 degrees upward when it is
 * positive, of 35 degrees downward when negative, as a 16-bit turn.
 */
static void Player_AddAimPitchPct(Player *pl, s8 delta)
{
    f32 fraction;
    s8 pct;

    pl->aimPitchPct += delta;
    if (pl->aimPitchPct > 100) {
        pl->aimPitchPct = 100;
    }
    if (pl->aimPitchPct < -100) {
        pl->aimPitchPct = -100;
    }
    pct = pl->aimPitchPct;
    fraction = pct / 100.0f;
    if (pct >= 0) {
        pl->aimPitch = 0.5f + (65536.0f * (45.0f * fraction)) / 360.0f;
        return;
    }
    pl->aimPitch = 0.5f + (65536.0f * (35.0f * fraction)) / 360.0f;
}

/*
 * The aim input of the local player whose current motion is mot: the
 * binoculars (motions 7 and 8) and the mounted gun (motion PL_MOT_MOUNTED_GUN). It needs
 * aimLockTimer or aimMode, else PAD_L1 held and PAD_L2 not. The D-pad up and
 * down change the pitch by ANGLE_TURN / 64 a frame (Player_AddAimPitch; the mounted gun
 * 4 percent, Player_AddAimPitchPct), left and right turn pl->targetYaw by
 * 0x180; a left-stick tilt of 40 or more does the same in larger steps by
 * the tilt. For the mounted gun targetYaw stays within a quarter turn of
 * pl->yawCenter. gScene.aimReverse flips the directions.
 */
void Player_UpdateLean(Player *pl, s32 mot)
{
    s8 stickStep;
    s8 dpadStep;
    u16 yawStep;
    u16 yawOffset;

    if (Player_IsLocal(pl) == 0) {
        return;
    }
    if (pl->motionId[0] != (u16) mot) {
        return;
    }
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        pl->aimLockTimer = 10;
    }
    if (pl->aimLockTimer == 0 && pl->aimMode == 0) {
        if (pl->input.held & PAD_L2) {
            return;
        }
        if (!(pl->input.held & PAD_L1)) {
            return;
        }
    }
    if (pl->input.held & (PAD_UP | PAD_DOWN)) {
        dpadStep = 4;
        if (pl->aimMode != 0 && gScene.aimReverse != 0) {
            dpadStep *= -1;
        }
        if (pl->input.held & PAD_UP) {
            if ((u16) mot != 414) {
                Player_AddAimPitch(pl, dpadStep << 8);
            } else {
                Player_AddAimPitchPct(pl, dpadStep);
            }
        } else {
            if ((u16) mot != 414) {
                Player_AddAimPitch(pl, -dpadStep << 8);
            } else {
                Player_AddAimPitchPct(pl, (s8) -dpadStep);
            }
        }
        pl->aimLockTimer = 5;
    }
    if (pl->input.held & PAD_LEFT) {
        pl->aimLockTimer = 10;
        if (gScene.aimReverse == 2) {
            pl->targetYaw -= 0x180;
        } else {
            pl->targetYaw += 0x180;
        }
    }
    if (pl->input.held & PAD_RIGHT) {
        pl->aimLockTimer = 10;
        if (gScene.aimReverse == 2) {
            pl->targetYaw += 0x180;
        } else {
            pl->targetYaw -= 0x180;
        }
    }
    if (pl->aimLockTimer != 0 || pl->aimMode != 0) {
        if ((pl->input.stickDir & PAD_DPAD) && pl->input.stickTilt >= PL_STICK_HELD) {
            stickStep = 1;
            yawStep = 76;
            if (pl->input.stickTilt >= 120) {
                stickStep = 8;
                yawStep = 0x200;
            } else if (pl->input.stickTilt >= 90) {
                stickStep = 4;
                yawStep = 192;
            } else if (pl->input.stickTilt >= 60) {
                stickStep = 2;
                yawStep = 128;
            }
            if (pl->input.stickDir & (PAD_UP | PAD_DOWN)) {
                if (gScene.aimReverse != 0) {
                    stickStep = -stickStep;
                }
                stickStep = (s8) stickStep;
                if (pl->input.stickDir & PAD_UP) {
                    if ((u16) mot != 414) {
                        Player_AddAimPitch(pl, stickStep << 8);
                    } else {
                        Player_AddAimPitchPct(pl, stickStep);
                    }
                } else {
                    if ((u16) mot != 414) {
                        Player_AddAimPitch(pl, -stickStep << 8);
                    } else {
                        Player_AddAimPitchPct(pl, (s8) -stickStep);
                    }
                }
                pl->aimLockTimer = 5;
            }
            if (pl->input.stickDir & PAD_LEFT) {
                pl->aimLockTimer = 10;
                if (gScene.aimReverse == 2) {
                    pl->targetYaw -= yawStep;
                } else {
                    pl->targetYaw += yawStep;
                }
            }
            if (pl->input.stickDir & PAD_RIGHT) {
                pl->aimLockTimer = 10;
                if (gScene.aimReverse == 2) {
                    pl->targetYaw += yawStep;
                } else {
                    pl->targetYaw -= yawStep;
                }
            }
        }
    }
    if ((u16) mot == 414) {
        yawOffset = pl->targetYaw - pl->yawCenter;
        if (yawOffset <= 0x8000U && yawOffset > 0x4000U) {
            pl->targetYaw = pl->yawCenter + ANGLE_TURN / 4;
        } else if (yawOffset >= 0x8000U && yawOffset < 0xC000U) {
            pl->targetYaw = pl->yawCenter - ANGLE_TURN / 4;
        }
    }
}

/*
 * PL_ACT_IDLE. By the current motion: PL_MOT_IDLE and PL_MOT_IDLE_ALT switch to
 * PL_MOT_IDLE_TIRED when staminaMax is below 76 (and back once it is above
 * 75), PL_MOT_IDLE switches to PL_MOT_IDLE_ALT after 240 frames for gender 1; PL_MOT_EXHAUSTED_START
 * ends in PL_MOT_EXHAUSTED_PANT, which makes an effect every 32 frames; a gunner's drawn
 * idle PL_MOT_WPN_IDLE blends the aim layers. A bowgun user then runs Player_BowgunAim;
 * every player runs Player_IdleActions.
 */
void Player_StateIdle(Player *pl, s32 unused)
{
    u16 motion = pl->motionId[0];

    switch (motion) {
    case 1:
    case 63:
        if (pl->staminaMax < 76) {
            Player_SetMotionPair(pl, PL_MOT_IDLE_TIRED, 6, 0);
        } else if (pl->motionTime >= 240 && pl->gender == 1 && motion != 63) {
            Player_SetMotionPair(pl, PL_MOT_IDLE_ALT, 2, 0);
        }
        break;
    case 21:
        if (pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_PANT, 4, 144);
        }
        break;
    case 5:
        if (!(gScene.frame & 0x1F)) {
            HitSpark_SpawnOnOwner(pl, 8, 20, 0.1f);
        }
        break;
    case 403:
        if (pl->staminaMax > 75) {
            Player_SetMotionPair(pl, PL_MOT_IDLE, 4, 0);
        }
        break;
    case 1001:
        if (pl->weaponOut != 0 && pl->isGunner == 1 && pl->weaponType != WEAPON_BOW) {
            Player_SetAimLayers(pl, 1005, 1006);
        }
        break;
    }
    switch (pl->weaponType) {
    case WEAPON_HEAVY_BOWGUN:
    case WEAPON_LIGHT_BOWGUN:
        Player_BowgunAim(pl);
        break;
    }
    Player_IdleActions(pl, 0);
}

/*
 * The walks and runs. variant: 0 PL_ACT_RUN, 1 PL_ACT_WALK, 2
 * PL_ACT_WALK_DRAWN, 3 PL_ACT_RIDE_WALK, 4 PL_ACT_MOVE_KEEP_MOTION, 5
 * PL_ACT_CROUCH_WALK_ALT, 6 PL_ACT_RUN_AFTER_DASH, 7 PL_ACT_WALK_SLOW, 8
 * PL_ACT_GUARD_WALK, 9 PL_ACT_BOW_WALK. Step 0 starts the variant's motion
 * (another player's copy online only does that). Once 10 frames have passed
 * with the stick released (Player_StickMoveLevel 0) the state ends: a run in
 * PL_ACT_RUN_STOP, the drawn walk in the idle of gDrawnWalkStopIdle (variant
 * 9: BOW_AIM_RESUME), the guard walk in PL_REACT_GUARD_HOLD (also when PAD_R1
 * is let go), the rest in idle. The stick turns the player
 * (Player_FaceStick); the drawn walk takes extra steps for the hunting horn
 * and the dual swords' demon mode; Player_IdleActions (Player_GuardInput for
 * variant 8) reads the input last.
 */
void Player_StateMove(Player *pl, s32 variant)
{
    f32 move[3];
    f32 delta[3];
    u8 stickLevel;
    u8 level;

    if (pl->step == 0) {
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 10;
        pl->timer = 10;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_RUN, 6, 0);
            Player_SetFlag(pl, 0x2000000);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_WALK, 4, 0);
            break;
        case 2:
        case 9:
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
            default:
                Player_SetMotionPair(pl, PL_MOT_WPN_WALK, 4, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_SetMotionPair(pl, PL_MOT_WPN_WALK, 4, 0);
                break;
            case WEAPON_HAMMER:
            case WEAPON_HUNTING_HORN:
                Player_SetMotionPair(pl, PL_MOT_WPN_WALK, 8, 0);
                break;
            case WEAPON_HEAVY_BOWGUN:
            case WEAPON_SWORD_SHIELD:
            case WEAPON_LIGHT_BOWGUN:
            case WEAPON_DUAL_SWORDS:
            case WEAPON_BOW:
                Player_SetMotionPair(pl, PL_MOT_WPN_WALK, 6, 0);
                break;
            }
            break;
        case 3:
            Player_SetMotionPair(pl, PL_MOT_CROUCH_WALK, 6, 0);
            break;
        case 4:
            break;
        case 5:
            Player_SetMotionPair(pl, PL_MOT_CROUCH_WALK, 2, 0);
            pl->action = PL_POSTURE_CROUCHED;
            break;
        case 6:
            pl->dashDelay = 30;
            if (pl->motionId[0] != 3) {
                Player_SetMotionPair(pl, PL_MOT_RUN, 0, 38);
            }
            Player_SetFlag(pl, 0x2000000);
            break;
        case 7:
            Player_SetMotionPair(pl, PL_MOT_WALK_SLOW, 6, 0);
            break;
        case 8:
            Player_SetFlag(pl, PLF_GUARD);
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_WALK, 4, 0);
            break;
        }
        pl->motionTime = 0;
    }
    if (Player_IsLocal(pl) == 0 && Game_IsOnline() == 1) {
        return;
    }
    stickLevel = Player_StickMoveLevel(pl, pl->weaponOut) & 0xFF;
    if (pl->stateVar1 != 0) {
        pl->stateVar1--;
    }
    if (pl->timer > 0) {
        pl->timer--;
    }
    level = stickLevel;
    if (level == 0 && pl->timer == 0) {
        if (pl->motionId[0] == 1004) {
            if (variant == 9) {
                Player_SetState(pl, PL_GROUP_WEAPON, BOW_AIM_RESUME, PL_SET_SNAP_POS);
            } else {
                Player_ReturnToIdle(pl, 0, gDrawnWalkStopIdle[pl->weaponType], 0);
            }
        } else if (pl->motionId[0] == 3 || pl->motionId[0] == 4) {
            if (pl->stateVar1 == 0) {
                pl->freeState = 1;
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
            } else {
                Player_ReturnToIdle(pl, 0, 4, 0);
            }
        } else if (pl->motionId[0] == 1011) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        } else {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        return;
    }
    switch (pl->motionId[0]) {
    case 2:
        if (Motion_PassedFrame(pl, 0, 108.0f)) {
            pl->stateVar0++;
            if (pl->stateVar0 >= 5) {
                pl->stateVar0 = 0;
                Player_SetMotionPair(pl, PL_MOT_WALK_VARIATION, 2, 0);
            }
        }
        break;
    case 3:
        if (level < 3) {
            pl->freeState = 1;
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
            return;
        }
        break;
    case 29:
        if (pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_WALK, 6, 36);
        }
        break;
    case 1011:
        if (!(pl->input.held & PAD_R1)) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        }
        break;
    }
    if (pl->mustCrouch != 0 && (variant != 5 || variant != 8)) {
        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
    }
    if (level != 0) {
        Player_FaceStick(pl);
    }
    if (Player_IsLocal(pl) == 0) {
        return;
    }
    if (variant == 2 && pl->weaponType == WEAPON_HUNTING_HORN && Player_IsLocal(pl) == 1 && pl->motionId[0] == 1004 &&
        Player_HasSongEffect(pl, PL_SONG_SPEED) == 1 && pl->songEffect[PL_SONG_SPEED] > 0) {
        move[0] = 0.0f;
        move[1] = 0.0f;
        move[2] = 8.0f;
        Vec_Rotate(delta, move, pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[2] += delta[2];
    }
    if (variant == 2 && pl->weaponType == WEAPON_DUAL_SWORDS && Player_IsLocal(pl) == 1 && pl->motionId[0] == 1004 &&
        pl->demonMode != 0) {
        move[0] = 0.0f;
        move[1] = 0.0f;
        move[2] = 2.0f;
        Vec_Rotate(delta, move, pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[2] += delta[2];
    }
    if (variant != 8) {
        if (variant == 9) {
            Player_IdleActions(pl, 1);
        } else {
            Player_IdleActions(pl, 0);
        }
    } else {
        Player_GuardInput(pl);
    }
}

/*
 * Draws or sheathes the weapon. variant: 0 PL_ACT_DRAW, 1 PL_ACT_SHEATHE, 2
 * PL_ACT_SHEATHE_MOVING, 3 PL_ACT_SHEATHE_TO_CROUCH, 4
 * PL_ACT_SHEATHE_TO_USE_ITEM, 5 PL_ACT_DRAW_MOVING. Step 0 takes the motion,
 * blend and new weaponOut from the weapon type's row of gDrawMotions,
 * gSheatheMotions, gSheatheMovingMotions or gDrawMovingMotions, and drops a
 * carried item. When the motion ends variant 3 requests PL_ACT_CROUCH,
 * variant 4 has the local player use the selected item (Player_UseItem, mode
 * 1), the others return to idle.
 */
void Player_StateDrawSheathe(Player *pl, u32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        switch (variant) {
        case 0:
            pl->weaponOut = gDrawMotions[pl->weaponType].flag;
            Player_SetMotionPair(pl, gDrawMotions[pl->weaponType].motion, gDrawMotions[pl->weaponType].blend, 0);
            break;
        case 1:
        case 3:
        case 4:
            pl->weaponOut = gSheatheMotions[pl->weaponType].flag;
            Player_SetMotionPair(pl, gSheatheMotions[pl->weaponType].motion, gSheatheMotions[pl->weaponType].blend, 0);
            break;
        case 2:
            Player_SetFlag(pl, 0x1000);
            pl->weaponOut = gSheatheMovingMotions[pl->weaponType].flag;
            if (pl->weaponType == WEAPON_HUNTING_HORN) {
                Player_SetMotionPair(pl, gSheatheMovingMotions[pl->weaponType].motion, gSheatheMovingMotions[pl->weaponType].blend, 10);
            } else {
                Player_SetMotionPair(pl, gSheatheMovingMotions[pl->weaponType].motion, gSheatheMovingMotions[pl->weaponType].blend, 0);
            }
            break;
        case 5:
            pl->weaponOut = gDrawMovingMotions[pl->weaponType].flag;
            Player_SetMotionPair(pl, gDrawMovingMotions[pl->weaponType].motion, gDrawMovingMotions[pl->weaponType].blend, 0);
            break;
        }
        if (pl->carry & PL_CARRY_HOLD_MASK) {
            pl->carry &= PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            switch (variant) {
            case 0:
                Player_ReturnToIdle(pl, 0, gDrawMotions[pl->weaponType].next, 0);
                break;
            case 1:
                Player_ReturnToIdle(pl, 0, gSheatheMotions[pl->weaponType].next, 0);
                break;
            case 2:
                Player_ReturnToIdle(pl, 0, gSheatheMovingMotions[pl->weaponType].next, 0);
                break;
            case 3:
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH, PL_SET_NO_SYNC);
                break;
            case 4:
                if (Player_IsLocal(pl) == 1) {
                    Player_UseItem(pl, 1);
                    pl->pouchHudFlags |= 1;
                    break;
                }
                Player_ReturnToIdle(pl, 0, gDrawMotions[pl->weaponType].next, 0);
                break;
            case 5:
                Player_ReturnToIdle(pl, 0, gDrawMovingMotions[pl->weaponType].next, 0);
                break;
            }
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_LEAP_OFF_LEDGE: a jump forward off a ledge (forward 20, up
 * 18; a ridden monster is left); 1 PL_ACT_FALL (forward 8, down 10).
 * pl->timer counts the time aloft. Falling, the player grabs a ledge ahead
 * (PL_ACT_LEDGE_GRAB, with hp above 0 and fallGrabCount under 21), else lands on
 * pl->floorY: the local player at 0 health dies (Player_Die), 30 or more
 * frames aloft request PL_ACT_LAND_HARD, else a landing motion plays and the
 * state returns to idle, or resumes the dash (PL_ACT_DASH_RESUME, or
 * PL_ACT_FLEE_DASH in battle) when the dash was held on landing.
 */
void Player_StateLeapOrFall(Player *pl, s32 variant)
{
    f32 move[3];
    f32 delta[3];
    f32 ledgeY;

    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = PL_POSTURE_AIRBORNE;
        pl->aimLockTimer = 0;
        Player_ResetAim(pl);
        switch (variant) {
        default:
            Player_SetMotionPair(pl, PL_MOT_LEAP_AIRBORNE, 0, 0);
            pl->vel[0] = 0.0f;
            pl->vel[2] = 20.0f;
            pl->vel[1] = 18.0f;
            pl->accel[0] = 0.0f;
            pl->accel[2] = -(pl->vel[2] / 80.0f);
            Player_StartJump(pl, 19);
            if (pl->rideState) {
                pl->rideState = 2;
            }
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_LEAP_AIRBORNE, 8, 32);
            pl->rideState = 0;
            pl->vel[0] = 0.0f;
            pl->vel[2] = 8.0f;
            pl->vel[1] = -10.0f;
            pl->accel[2] = 0.0f;
            pl->accel[0] = 0.0f;
            pl->accel[1] = -0.72727275f;
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->timer = 0;
        pl->stateVar1 = 0;
        break;
    case 1:
        pl->vel[0] += pl->accel[0];
        pl->vel[1] += pl->accel[1];
        pl->vel[2] += pl->accel[2];
        if (pl->vel[0] < 0.0f) {
            pl->vel[0] = 0.0f;
        }
        if (pl->vel[2] < 0.0f) {
            pl->vel[2] = 0.0f;
        }
        move[0] = pl->vel[0];
        move[1] = pl->vel[1];
        move[2] = pl->vel[2];
        Vec_Rotate(delta, move, pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[1] += delta[1];
        pl->pos[2] += delta[2];
        if (pl->vel[1] < 0.0f) {
            if (Player_FloorAheadAbove(pl, &ledgeY, 55.0f, 180.0f, 30.0f) && pl->hp > 0 && pl->fallGrabCount < 21) {
                pl->targetYaw = pl->rot[1];
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, PL_SET_SNAP_POS);
                break;
            }
            if (pl->floorY < pl->pos[1]) {
                break;
            }
            pl->pos[1] = pl->floorY;
            pl->action = PL_POSTURE_STANDING;
            pl->rideState = 0;
            if (pl->hp <= 0 && Player_IsLocal(pl) == 1) {
                Player_Die(pl);
                break;
            }
            if (pl->timer >= 30) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                break;
            }
            pl->step++;
            if (pl->weaponOut == 0 && (pl->input.held & PAD_R2) && (Player_StickMoveLevel(pl, 0) & 0xFF)) {
                pl->stateVar1 = 1;
                Player_SetMotionPair(pl, PL_MOT_LAND_INTO_DASH, 0, 0);
            } else {
                pl->stateVar1 = 0;
                Player_SetMotionPair(pl, PL_MOT_LAND, 0, 0);
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->stateVar1 == 0) {
                Player_ReturnToIdle(pl, 0, 8, 0);
            } else if (pl->inBattle == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DASH_RESUME, 0);
            } else {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH, 0);
            }
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_JUMP_TO_LEDGE (a crouch, then a jump up toward a high
 * ledge), 1 PL_ACT_WALL_LET_GO, 2 PL_ACT_LEDGE_DROP (first moves the player
 * 195 down and 30 back). Then it moves with Player_MoveAccel, grabs a ledge
 * ahead while falling (not after PL_ACT_LEDGE_DROP) and lands as
 * Player_StateLeapOrFall does.
 */
void Player_StateClimbAirborne(Player *pl, s32 variant)
{
    f32 y;
    f32 move[3];
    f32 delta[3];

    pl->timer++;
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
        default:
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_JUMP_UP_CROUCH, 2, 0);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
            break;
        case 2:
            pl->pos[1] -= 195.0f;
            move[0] = 0.0f;
            move[1] = 0.0f;
            move[2] = -30.0f;
            Vec_Rotate(delta, move, (f32 *) pl->worldMtx);
            pl->pos[0] += delta[0];
            pl->pos[2] += delta[2];
        case 1:
            pl->step = 2;
            Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
            Player_StopMove(pl);
            pl->vel[1] = -9.0f;
            pl->accel[1] = -8.0f / 11.0f;
            if (variant == 1) {
                Player_SetMotionPair(pl, PL_MOT_CLIMB_AIRBORNE, 4, 22);
            } else {
                Player_SetMotionPair(pl, PL_MOT_CLIMB_AIRBORNE, 0, 22);
            }
            break;
        }
        Player_ResetAim(pl);
        Player_UpdateMotionTime(pl, 0);
        pl->timer = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_StopMove(pl);
            pl->vel[1] = 2.0f * (15.0f * (pl->weaponOut ? 0.8f : 1.0f));
            Player_StartJump(pl, 20);
            Player_SetMotionPair(pl, PL_MOT_CLIMB_AIRBORNE, 2, 0);
            pl->action = PL_POSTURE_AIRBORNE;
            if (pl->rideState != 0) {
                pl->rideState = 2;
            }
            Player_UpdateMotionTime(pl, 0);
            pl->timer = 0;
        }
        break;
    case 2:
        Player_MoveAccel(pl);
        if (pl->vel[1] < 0.0f) {
            if (variant != 2 && Player_FloorAheadAbove(pl, &y, 55.0f, 180.0f, 30.0f) && pl->hp > 0) {
                pl->targetYaw = pl->rot[1];
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            if (!(pl->floorY < pl->pos[1])) {
                pl->pos[1] = pl->floorY;
                pl->action = PL_POSTURE_STANDING;
                pl->rideState = 0;
                if (pl->hp <= 0 && Player_IsLocal(pl) == 1) {
                    Player_Die(pl);
                    break;
                }
                if (pl->timer >= 30) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                    break;
                }
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_CLIMB_LAND, 0, 0);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 3, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_CROUCH: crouches down (motion 7, then the held crouch,
 * motion 8), sheathes and drops a carried item; 1 PL_ACT_CROUCH_HOLD:
 * straight to the held crouch. From step 2 Player_CrouchInput reads the
 * crouched controls every frame.
 */
void Player_StateCrouch(Player *pl, s32 variant)
{
    u8 carry;

    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        switch (variant) {
        case 0:
            pl->step++;
            pl->motionTime = 0;
            Player_SetMotionPair(pl, PL_MOT_CROUCH_DOWN, 2, 0);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
            pl->weaponOut = 0;
            carry = pl->carry;
            if (carry & PL_CARRY_HOLD_MASK) {
                pl->carry = carry & PL_CARRY_KIND_MASK;
                ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
            }
            break;
        case 1:
            pl->step = 2;
            if (pl->motionId[0] == 412) {
                Player_SetMotionPair(pl, PL_MOT_CROUCH, 8, 0);
            } else {
                Player_SetMotionPair(pl, PL_MOT_CROUCH, 2, 0);
            }
            Player_UpdateMotionTime(pl, 0);
            Player_SetPosture(pl, PL_POSTURE_CROUCHED | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
            pl->weaponOut = 0;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
            Player_SetPosture(pl, PL_POSTURE_CROUCHED | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
            Player_UpdateMotionTime(pl, 0);
        }
        break;
    case 2:
        Player_CrouchInput(pl);
        break;
    }
}

//---
void Player_StateBedMenu(Player *pl, s32 unused)
{

    pl->timer--;
    if (pl->timer <= 0) {
        pl->timer = 0;
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 30;
        Player_SetMotionPair(pl, PL_MOT_IDLE, 4, 0);
        pl->deliveryMenu = 255;
        pl->menuDelay = 30;
        pl->menuCursor = 0;
        pl->menuPage = 0;
        break;
    case 1:
        if ((s32) pl->motionTime >= 240 && pl->gender == 1 && pl->motionId[0] != 63) {
            Player_SetMotionPair(pl, PL_MOT_IDLE_ALT, 2, 0);
        }
        if (Player_IsLocal(pl) == 1 && pl->timer <= 0) {
            if (Quest_PhaseIsOver(1) == 1 || pl->deliveryMenu == 0) {
                Player_ReturnToIdle(pl, 0, 2, 0);
            } else if (pl->deliveryMenu == 1) {
                pl->deliveryMenu = 0;
                switch (pl->menuCursor) {
                case 0:
                    Player_StartSpotAction(pl, 1);
                    break;
                case 1:
                    Quest_ClearBySubTarget();
                    Player_ReturnToIdle(pl, 0, 2, 0);
                    break;
                case 2:
                    Quest_Abort();
                    Player_ReturnToIdle(pl, 0, 2, 0);
                    break;
                }
            }
        }
        break;
    }
}

void Player_StateToIdle(Player *pl, s32 unused)
{
    Player_ReturnToIdle(pl, 0, 2, 0);
}

/*
 * Throws pl->useItem. variant: 0 PL_ACT_THROW_ITEM, 1 PL_ACT_THROW_ITEM_ALT,
 * 2 PL_ACT_RETURN_TO_CAMP. At the release frame it makes the thrown object
 * (ItemAttack_Create, the type by the item); one item makes the effect
 * Boomerang_Create instead and is used up only on a 1 in 8 roll. While the quest
 * runs one of the item is used up. When the motion ends variant 2 sends the
 * local player to the camp area (gScene.campArea: PL_AREA_MOVE_ENTER, nextArea,
 * that area's start position, QSync_SendPlayer); the others return to idle.
 */
void Player_StateThrowItem(Player *pl, s32 variant)
{
    s8 kind;
    u8 nouse;
    u8 k;

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_THROW_ITEM, 2, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_THROW_ITEM_ALT, 2, 0);
        }
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, variant ? 20.0f : 24.0f)) {
            kind = 0;
            if (pl->prevStateGroup == 0) {
                if (pl->prevState == 1) {
                    kind = 2;
                }
                if (pl->prevState == 2) {
                    kind = 1;
                }
            }
            nouse = 0;
            switch (pl->useItem) {
            case 76:
                ItemAttack_Create(pl, 0, kind);
                break;
            case 78:
                ItemAttack_Create(pl, 7, kind);
                break;
            case 72:
                ItemAttack_Create(pl, 1, kind);
                break;
            case 663:
                ItemAttack_Create(pl, 1, kind);
                break;
            case 75:
                ItemAttack_Create(pl, 5, kind);
                break;
            case 77:
                ItemAttack_Create(pl, 11, kind);
                break;
            case 71:
                ItemAttack_Create(pl, 6, kind);
                break;
            case 215:
                ItemAttack_Create(pl, 8, kind);
                break;
            case 73:
                ItemAttack_Create(pl, 9, kind);
                break;
            case 80:
                ItemAttack_Create(pl, 10, kind);
                break;
            case 65:
                ItemAttack_Create(pl, ITEMATK_LOBBED_A, kind);
                break;
            case 66:
                ItemAttack_Create(pl, ITEMATK_LOBBED_B, kind);
                break;
            case 67:
                ItemAttack_Create(pl, ITEMATK_LOBBED_C, kind);
                break;
            case 68:
                ItemAttack_Create(pl, ITEMATK_LOBBED_D, kind);
                break;
            case 69:
                ItemAttack_Create(pl, ITEMATK_LOBBED_NO_VERSUS, kind);
                break;
            case 698:
                ItemAttack_Create(pl, 17, kind);
                break;
            case 699:
                ItemAttack_Create(pl, 18, kind);
                break;
            case 700:
                ItemAttack_Create(pl, 19, kind);
                break;
            case 79:
                ItemAttack_Create(pl, 23, kind);
                break;
            case 64:
                if (pl->prevStateGroup == 0 && pl->prevState == 73) {
                    kind = 1;
                }
                if (pl->stateRand % 8) {
                    nouse = 1;
                }
                if (Player_HasSkill(pl, SKILL_THROW_UP) == 1 && !(Rand_Next(1) & 7)) {
                    nouse = 1;
                }
                if (nouse == 0 && Player_IsLocal(pl) == 1) {
                    Notice_Push(NOTICE_KIND_ITEM, 5, 64);
                }
                Boomerang_Create((Em *) pl, kind, nouse);
                break;
            }
            if (Quest_PhaseIsOver(1) == 0 && nouse == 0) {
                Pouch_Add(pl, pl->useItem, -1);
            }
        }
        if (pl->motion[0].playing == 0) {
            switch (variant) {
            case 0:
                Player_ReturnToIdle(pl, 0, 10, 0);
                break;
            case 1:
                Player_ReturnToIdle(pl, 0, 2, 0);
                break;
            case 2:
                if (Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
                    k = gScene.campArea;
                    pl->areaMoveState = PL_AREA_MOVE_ENTER;
                    pl->startPos[0] = gStartPos[k][0];
                    pl->startPos[1] = gStartPos[k][1];
                    pl->startPos[2] = gStartPos[k][2];
                    pl->startAngle = gStartAngle[k];
                    pl->nextArea = k;
                    Player_AddSpawnOffset(pl, pl->startPos, pl->startAngle);
                    pl->noHitTimer = 5;
                    QSync_SendPlayer(pl, QSYNC_PL_RELOCATED, 0);
                } else {
                    Player_ReturnToIdle(pl, 0, 2, 0);
                }
                break;
            }
        }
        break;
    case 2:
        break;
    }
}

/*
 * variant 0 PL_ACT_LEDGE_HANG, 1 PL_ACT_LEDGE_GRAB (first steps 55 forward
 * onto the ledge at the floor's height, Em_GetFloorYAbove). Hangs from a
 * ledge (PL_MOT_LEDGE_HANG); a grab after PL_ACT_FALL counts pl->fallGrabCount up. Pulls
 * up (PL_ACT_LEDGE_PULL_UP) when the motion ends, on PAD_CIRCLE or with the
 * stick up; the local player lets go (PL_ACT_LEDGE_DROP) on a new stick-down
 * press once pl->timer (30) is 0.
 */
void Player_StateLedgeHang(Player *pl, s32 variant)
{
    f32 y;
    s32 ang[3];
    f32 offset[3];
    f32 pos[3];
    f32 delta[3];
    f32 yawMtx[16];

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->targetYaw = pl->rot[1];
        Player_SetMotionPair(pl, PL_MOT_LEDGE_HANG, 0, 0);
        pl->timer = 30;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 1) {
            ang[0] = 0;
            ang[1] = pl->rot[1];
            ang[2] = 0;
            Mtx_FromAnglesXYZ(ang, yawMtx);
            offset[0] = 0.0f;
            offset[1] = 0.0f;
            offset[2] = 55.0f;
            Vec_Rotate(delta, offset, yawMtx);
            pos[0] = pl->pos[0] + delta[0];
            pos[1] = pl->pos[1] + delta[1];
            pos[2] = pl->pos[2] + delta[2];
            if (Em_GetFloorYAbove((Em *) pl, pos, &y) == 1) {
                pl->pos[1] = y;
            }
            offset[2] = 55.0f;
            Vec_Rotate(delta, offset, (f32 *) pl->worldMtx);
            pl->pos[0] += delta[0];
            pl->pos[1] += delta[1];
            pl->pos[2] += delta[2];
        }
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB) == 1 && pl->prevStateGroup == 0 && pl->prevState == 9) {
            pl->fallGrabCount++;
        }
        break;
    case 1:
        if (pl->timer != 0) {
            pl->timer--;
            if (pl->timer <= 0) {
                pl->timer = 0;
            }
        }
        if (pl->motion[0].playing == 0 || (pl->input.pressed & PAD_CIRCLE) || (pl->input.stickDir & PAD_UP)) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_PULL_UP, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
        } else if (Player_IsLocal(pl) == 1 && (pl->input.stickDirPressed & PAD_DOWN) && pl->timer == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_DROP, PL_SET_SNAP_POS);
        }
        break;
    }
}

/*
 * PL_ACT_RIDE_STAGGER: the stagger of a player standing on a monster that
 * carries it fast (Player_CheckRideSlip). Plays PL_MOT_STAGGER until the
 * distance the monster moves the player (rideDelta) is under the limit again,
 * then returns to idle.
 */
void Player_StateRideStagger(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_STAGGER, 6, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Player_CheckRideSlip(pl) == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_PLACE_ITEM, 1 PL_ACT_PLACE_ITEM_CROUCHED: sets a tool item
 * down (a barrel bomb). At entry the local player runs Item_UseTool, which
 * makes the placed object for pl->useItem and uses one up. Variant 0 returns
 * to idle, variant 1 to PL_ACT_CROUCH_HOLD.
 */
void Player_StatePlaceItem(Player *pl, s32 variant)
{
    s32 timer;

    switch (pl->step) {
    case 0:
        pl->motionTime = 0;
        pl->timer = 0;
        switch (variant) {
        case 0:
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
            Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
            break;
        case 1:
            pl->step = 3;
            Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
            Player_SetMotionPair(pl, PL_MOT_PLACE_ITEM_CROUCHED, 0, 20);
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        if (Player_IsLocal(pl) == 1) {
            Item_UseTool(pl);
        }
        break;
    case 1:
        timer = pl->timer + 1;
        pl->timer = timer;
        if (timer >= 14) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH_STAND_UP, 0, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, PL_SET_NO_SYNC);
        }
        break;
    }
}

/*
 * PL_ACT_KNEEL_TO_PLACE: kneels (motion 7) before placing an item; then the
 * local player requests PL_ACT_PLACE_ITEM, and another player's copy holds
 * the crouch pose.
 */
void Player_StateKneelToPlace(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, PL_MOT_CROUCH_DOWN, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_ITEM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
        } else {
        case 2:
            break;
        }
        break;
    }
}

/*
 * The roll. variant 0 PL_ACT_ROLL, 1 PL_ACT_ROLL_FROM_CROUCH (stands up
 * first). Costs 75 stamina, sets the hit-free frames (Player_SetEvadeFrames)
 * and drops a carried item. With the weapon drawn the local player's sword
 * and shield and dual swords take a flick up (PAD_RSTICK_UP) in frames 10-44
 * and then turn an eighth of a turn toward the held side (PAD_LEFT, PAD_RIGHT) into
 * SwordShield_EvadeAttack or DualSwords_EvadeAttack; the hammer charges from
 * the roll (Hammer_ChargeFromRoll) at frame 40 with PAD_R1 held. Returns to
 * idle when the motion ends.
 */
void Player_StateRoll(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->motionTime = 0;
        Player_SetEvadeFrames(pl);
        pl->timer = 0;
        Player_ResetAim(pl);
        pl->noFlinchTimer = 0;
        pl->stateVar0 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 30, 0, pl->pos, 1, 0);
        if (pl->carry & PL_CARRY_HOLD_MASK) {
            pl->carry &= PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        if (variant == 0) {
            pl->step = 2;
            if ((pl->weaponType == WEAPON_HAMMER || pl->weaponType == WEAPON_HUNTING_HORN) && pl->weaponOut != 0) {
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_WALK, 2, 0);
            } else {
                Player_SetMotionPair(pl, PL_MOT_ROLL, 2, 0);
            }
        } else {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_ROLL_FROM_CROUCH, 0, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_ROLL, 0, 6);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        } else if (Player_IsLocal(pl) == 1 && variant == 0 && pl->weaponOut != 0) {
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
            case WEAPON_DUAL_SWORDS:
                if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && Motion_InFrameRange(pl, 0, 10.0f, 44.0f) && pl->stateVar0 == 0) {
                    pl->stateVar0 = 1;
                    if (pl->input.stickDir & PAD_LEFT) {
                        pl->stateVar0 = 2;
                    }
                    if (pl->input.stickDir & PAD_RIGHT) {
                        pl->stateVar0 = 3;
                    }
                }
                if (Motion_ReachedFrame(pl, 0, 44.0f) && pl->stateVar0 != 0) {
                    if ((pl->stateVar0 & 0xFF) == 2) {
                        pl->rot[1] += ANGLE_TURN / 8;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->stateVar0 == 3) {
                        pl->rot[1] -= ANGLE_TURN / 8;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->weaponType == WEAPON_SWORD_SHIELD) {
                        SwordShield_EvadeAttack(pl);
                    } else {
                        DualSwords_EvadeAttack(pl);
                    }
                }
                break;
            case WEAPON_HAMMER:
                if (Motion_ReachedFrame(pl, 0, 40.0f) && (pl->input.held & PAD_R1)) {
                    Hammer_ChargeFromRoll(pl);
                }
                break;
            }
        }
        break;
    }
}

//---
void Player_StateStepUp(Player *pl, s32 unused)
{
    f32 offset[3];
    f32 pos[4];
    f32 delta[4];
    f32 y;
    f32 *offsetZ;
    f32 *dy;
    f32 *dz;

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_LEDGE_CLIMB_UP, 0, 88);
        pl->targetYaw = pl->rot[1];
        offset[0] = 0.0f;
        offset[1] = 0.0f;
        offsetZ = &offset[2];
        *offsetZ = 55.0f;
        Vec_Rotate(delta, offset, (f32 *) pl->worldMtx);
        dy = &delta[1];
        dz = &delta[2];
        pos[0] = pl->pos[0] + delta[0];
        pos[1] = pl->pos[1] + *dy;
        pos[2] = pl->pos[2] + *dz;
        if (Em_GetFloorYAbove((Em *) pl, pos, &y) == 1) {
            pl->pos[1] = y;
        }
        *offsetZ = 55.0f;
        Vec_Rotate(delta, offset, (f32 *) pl->worldMtx);
        pl->pos[0] += delta[0];
        pl->pos[1] += *dy;
        pl->pos[2] += *dz;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The sprint (PAD_R2, sheathed). variant 0 PL_ACT_DASH, 1 PL_ACT_DASH_RESUME
 * (picked up mid-stride). Uses stamina each frame: under 76 requests
 * PL_ACT_DASH_TIRED; PAD_CROSS with 75 or more rolls, a drop ahead leaps off
 * (PL_ACT_LEAP_OFF_LEDGE), and in battle with the heading more than an eighth of a turn
 * off pl->targetAngle it becomes PL_ACT_FLEE_DASH. With PAD_R2 released the
 * local player goes to PL_ACT_RUN_AFTER_DASH (full stick, once dashDelay is
 * 0) or PL_ACT_RUN_STOP.
 */
void Player_StateDash(Player *pl, s32 variant)
{
    u8 stickLevel;
    f32 move[3];
    f32 delta[3];

    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 1536);
        pl->motionTime = 0;
        pl->dashDelay = 30;
        switch (variant) {
        case 0:
            if (pl->motionId[0] != 3) {
                Player_SetMotionPair(pl, PL_MOT_RUN, 0, 0);
            }
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_RUN, 4, 30);
            break;
        }
        pl->motion[0].step = 2.5f;
        pl->motion[1].step = 2.5f;
        pl->targetYaw = pl->rot[1];
        break;
    case 1:
        pl->motion[0].step = 2.5f;
        pl->motion[1].step = 2.5f;
        Player_UseStamina(pl, -1);
        if (pl->stamina < 76) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED, 0);
            break;
        }
        if ((pl->input.pressed & PAD_CROSS) && pl->stamina >= 75 && Quest_PhaseIsOver(1) == 0) {
            pl->targetYaw = pl->rot[1];
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
            break;
        }
        if (pl->input.stickTilt >= PL_STICK_HELD) {
            Player_FaceStick(pl);
        }
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f)) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEAP_OFF_LEDGE, 0);
            break;
        }
        if (pl->inBattle != 0 && (u16) (pl->targetAngle - pl->rot[1] + ANGLE_TURN / 8) > 0x4000) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH, 0);
            break;
        }
        move[0] = 0.0f;
        move[1] = 0.0f;
        move[2] = 5.0f;
        Vec_Rotate(delta, move, (f32 *) pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[2] += delta[2];
        if (Player_IsLocal(pl) != 0) {
            stickLevel = Player_StickMoveLevel(pl, 0);
            if (stickLevel != 5 && (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0)) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED) == 0 || pl->stamina > 0) {
                    if (stickLevel == 3) {
                        if (pl->dashDelay == 0) {
                            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH, 0);
                        } else {
                            Player_IdleActions(pl, 0);
                        }
                    } else {
                        pl->freeState = 1;
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
                    }
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_OUT_OF_BREATH, 0);
                }
            } else {
                Player_IdleActions(pl, 0);
            }
            if (pl->mustCrouch != 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
        }
        break;
    }
}

/*
 * The sprint away from the monster being fought. variant: 0 PL_ACT_FLEE_DASH,
 * 1 PL_ACT_DASH_TIRED, 2 PL_ACT_FLEE_DASH_RESUME (after the dive). Costs
 * stamina each frame. Except when tired: under 76 stamina requests
 * PL_ACT_DASH_TIRED, leaving the battle or facing pl->targetAngle again goes
 * to PL_ACT_DASH_RESUME, PAD_CROSS dives (PL_ACT_DIVE), a drop ahead leaps
 * off. With PAD_R2 released it goes to PL_ACT_RUN_AFTER_DASH or
 * PL_ACT_RUN_STOP; a tired player with no stamina to PL_ACT_OUT_OF_BREATH.
 */
void Player_StateFleeDash(Player *pl, s32 variant)
{
    u8 stickLevel;
    f32 move[3];
    f32 delta[3];

    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 1536);
        pl->motionTime = 0;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_FLEE_DASH, 0, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_DASH_TIRED, 0, 0);
            break;
        case 2:
            Player_SetMotionPair(pl, PL_MOT_FLEE_DASH, 4, 0);
            break;
        }
        pl->targetYaw = pl->rot[1];
        break;
    case 1:
        if (variant != 1) {
            Player_UseStamina(pl, -2);
            if (pl->stamina < 76) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED, 0);
                break;
            }
            if (pl->inBattle == 0 || (u16) (pl->targetAngle - pl->rot[1] + ANGLE_TURN / 8) <= 0x4000) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DASH_RESUME, 0);
                break;
            }
        } else {
            Player_UseStamina(pl, -1);
        }
        if ((pl->input.pressed & PAD_CROSS) && variant != 1 && pl->stamina >= 75) {
            pl->targetYaw = pl->rot[1];
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_DIVE, PL_SET_SNAP_POS);
            break;
        }
        if (pl->input.stickTilt >= PL_STICK_HELD) {
            Player_FaceStick(pl);
        }
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f) && variant != 1) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LEAP_OFF_LEDGE, 0);
            break;
        }
        move[0] = 0.0f;
        move[1] = 0.0f;
        move[2] = 1.5f;
        Vec_Rotate(delta, move, (f32 *) pl->rotMtx);
        pl->pos[0] += delta[0];
        pl->pos[2] += delta[2];
        if (Player_IsLocal(pl) != 0) {
            stickLevel = Player_StickMoveLevel(pl, 0);
            if (stickLevel != 5 && (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0)) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_DASH_TIRED) == 0 || pl->stamina > 0) {
                    if (stickLevel == 3 && variant != 1) {
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH, 0);
                    } else {
                        pl->freeState = 1;
                        Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
                    }
                } else {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_OUT_OF_BREATH, 0);
                }
            } else {
                Player_IdleActions(pl, 0);
            }
            if (pl->mustCrouch != 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_WALK, 0);
            }
        }
        break;
    }
}

/*
 * PL_ACT_OUT_OF_BREATH: the stamina ran out. Sheathes and pants for 150
 * frames, then refills the stamina to staminaMax and returns to idle.
 */
void Player_StateOutOfBreath(Player *pl, s32 variant)
{
    Player_AddStamina(pl, 4);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->weaponOut = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_PANT, 4, 0);
            pl->step = 2;
            pl->timer = 150;
            break;
        }
        Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_START, 8, 0);
        pl->timer = 10;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_PANT, 4, 144);
        }
        break;
    case 2:
        if (--pl->timer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_RECOVER, 4, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_AddStamina(pl, (s32) pl->staminaMax);
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_ACT_TIRED_STOP: plays the tired sprint's motion once, then returns to
 * idle (no entry seen).
 */
void Player_StateTiredStop(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_DASH_TIRED, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 5, 0);
        }
        break;
    }
}

/*
 * PL_ACT_LAND_HARD: the landing from a long fall. Clears rideState, shakes
 * the pad and the camera, and returns to idle when the motion ends.
 */
void Player_StateHardLanding(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
        pl->rideState = 0;
        Player_SetMotionPair(pl, PL_MOT_LAND_HARD, 2, 0);
        Vib_StartLocal(pl, 1);
        CamShake_ReqA(pl, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * PL_ACT_WALL_HOLD: holds still on a climbable wall in the pose of the climb
 * it came from (PLF_NO_ROOT_MOVE). Every frame it presses the player into the
 * wall (Player_StepBack) and Player_WallClimbInput starts the climbs or lets
 * go.
 */
void Player_StateWallHold(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    if (step == 0) {
        pl->step = step + 1;
        Player_SetFlag(pl, 0x20000U);
        pl->stateVar0 = 0;
        if (pl->motionId[0] == 34) {
            if (Motion_PassedFrame(pl, 0, 46.0f) != 0) {
                Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_UP, 0, 46);
            } else {
                Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_UP, 0, 90);
            }
        } else if (Motion_PassedFrame(pl, 0, 0.0f) != 0) {
            Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 0, 44);
        }
    } else {
        pl->stateVar0 = 1;
    }
    pl->noPushTimer = 2;
    Player_SetPosture(pl, PL_POSTURE_AIRBORNE | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
    Player_StepBack(pl, 1);
    Player_WallClimbInput(pl, 0);
}

/*
 * variant 0 PL_ACT_WALL_CLIMB_UP, 1 PL_ACT_WALL_CLIMB_UP_FAST (faster, each
 * cycle costs the dash stamina). At frames 46 and 90 the local player reads
 * the stick (Player_WallClimbInput; no input holds, PL_ACT_WALL_HOLD). No
 * stamina left requests PL_ACT_WALL_LET_GO; a ledge top ahead
 * PL_ACT_LEDGE_GRAB.
 */
void Player_StateWallClimbUp(Player *player, s32 variant)
{
    f32 ledgeY;
    u16 motion;
    u8 step;

    *(s16 *)&player->noPushTimer = 2;
    if (variant == 0) {
        player->motion[0].step = 1.6f;
        player->motion[1].step = 1.6f;
    } else {
        player->motion[0].step = 2.5f;
        player->motion[1].step = 2.5f;
    }
    step = player->step;
    if (step == 0) {
        player->step = step + 1;
        Player_SetPosture((void *) player, PL_POSTURE_AIRBORNE, 0, 0);
        *(s8 *)&player->rideState = 0;
        motion = player->motionId[0];
        if (motion != 34) {
            if (motion == 35) {
                if (Motion_ReachedFrame((void *) player, 0, 40.0f) != 0) {
                    Player_SetMotionPair((void *) player, 34, 0, 90);
                } else {
                    Player_SetMotionPair((void *) player, 34, 0, 46);
                }
            } else {
                Player_SetMotionPair((void *) player, 34, 4, 0);
            }
        }
    } else if (((Motion_PassedFrame((void *) player, 0, 46.0f) != 0) || (Motion_PassedFrame((void *) player, 0, 90.0f) != 0)) && (Player_IsLocal(player) == 1)) {
        if (!(Player_WallClimbInput(player, 1) & 0xFF)) {
            Player_RequestState((void *) player, PL_GROUP_ACTION, PL_ACT_WALL_HOLD, 0);
        } else if (variant == 1) {
            Player_UseDashStamina(player);
        }
    }
    if (player->stamina <= 0) {
        Player_RequestState((void *) player, PL_GROUP_ACTION, PL_ACT_WALL_LET_GO, 0);
        return;
    }
    if (Player_FloorAheadAbove((void *) player, &ledgeY, 55.0f, 180.0f, 30.0f) != 0) {
        player->targetYaw = (s16) player->rot[1];
        Player_RequestState((void *) player, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB, 0);
    }
    Player_StepBack(player, 1);
}

/*
 * variant 0 PL_ACT_WALL_CLIMB_DOWN, 1 PL_ACT_WALL_CLIMB_DOWN_FAST; as
 * Player_StateWallClimbUp, downward. No stamina left requests
 * PL_ACT_WALL_LET_GO; reaching pl->floorY PL_ACT_WALL_DISMOUNT.
 */
void Player_StateWallClimbDown(Player *pl, s32 variant)
{
    u16 motion;

    pl->noPushTimer = 2;
    if (variant == 0) {
        pl->motion[0].step = 1.6f;
        pl->motion[1].step = 1.6f;
    } else {
        pl->motion[0].step = 2.6f;
        pl->motion[1].step = 2.6f;
    }
    if (pl->step == 0) {
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->rideState = 0;
        motion = pl->motionId[0];
        if (motion != 35) {
            if (motion == 34) {
                if (Motion_ReachedFrame(pl, 0, 86.0f) != 0) {
                    Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 0, 44);
                } else {
                    Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 0, 0);
                }
            } else {
                Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_DOWN, 4, 0);
            }
        }
    } else if (Motion_PassedFrame(pl, 0, 0.0f) != 0 || Motion_PassedFrame(pl, 0, 44.0f) != 0) {
        if (Player_IsLocal(pl) == 1) {
            if (!(Player_WallClimbInput(pl, 1) & 0xFF)) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD, 0);
            } else if (variant == 1) {
                Player_UseDashStamina(pl);
            }
        }
    }
    if (pl->stamina <= 0) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_LET_GO, 0);
        return;
    }
    if (pl->pos[1] <= pl->floorY) {
        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_DISMOUNT, 0);
    }
    Player_StepBack(pl, 1);
}

/*
 * PL_ACT_WALL_DISMOUNT: steps off the foot of the wall, then returns to idle.
 */
void Player_StateWallDismount(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_WALL_DISMOUNT, 6, 0);
        Player_StepBack(pl, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            break;
        }
        Player_StepBack(pl, 1);
        break;
    }
}

/*
 * PL_ACT_WALL_GRAB: starts a wall climb (from Player_CheckClimb): turns to
 * face the wall (Player_TurnAlongWallSide, side 0) and at frame 90 of the
 * climb motion requests PL_ACT_WALL_HOLD.
 */
void Player_StateWallGrab(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->rideState = 0;
        Player_TurnAlongWallSide(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_WALL_CLIMB_UP, 4, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 90.0f) != 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD, 0);
        }
        break;
    }
    Player_StepBack(pl, 1);
}

/* PL_ACT_RUN_STOP: the skid at the end of a run or a sprint, then idle. */
void Player_StateRunStop(Player *pl, s32 unused)
{
    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_RUN_STOP, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 10, 0);
        }
        break;
    }
}

/*
 * PL_ACT_SET_TRAP: kneels to set a trap or a device for pl->useItem
 * (Player_UseItem sends the placed devices, the special mine point items and
 * the delivery item of an action spot of kind 11 here). At frame 60 of the
 * second motion the local player requests PL_ACT_SET_TRAP_DONE; another
 * player's copy finishes the motion and returns to idle.
 */
void Player_StateSetTrap(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -14, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 130.0f) != 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_SET_TRAP, 4, 0);
        }
        break;
    case 2:
        if (Motion_ReachedFrame(pl, 0, 60.0f) != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SET_TRAP_DONE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 10, 0);
        }
        break;
    }
}

/*
 * PL_ACT_SET_TRAP_DONE: at entry the local player applies the item: the
 * quest's delivery item is delivered (Quest_DeliverFromPouch, mode 1), any
 * other goes through Item_UseMisc (its effect or its special mine point, one
 * used up). Then stands up and returns to idle.
 */
void Player_StateSetTrapDone(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_SET_TRAP, 0, 60);
        if (Player_IsLocal(pl) == 1) {
            if (pl->useItem == ITEM_ID_DELIVERY) {
                Quest_DeliverFromPouch(pl, 1);
                break;
            }
            Item_UseMisc(pl);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -4, 206);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 10, 0);
        }
        break;
    }
}

/*
 * The sidestep with the weapon drawn. variant 0 PL_ACT_EVADE_RIGHT, 1
 * PL_ACT_EVADE_LEFT. Costs 75 stamina and sets the hit-free frames; the long
 * sword has its own motions. The sword and shield and the dual swords chain
 * their evade attack as in Player_StateRoll (frames 10-42). Returns to idle
 * when the motion ends.
 */
void Player_StateSideEvade(Player *pl, s32 variant)
{
    u8 dir;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->noFlinchTimer = 0;
        Player_SetEvadeFrames(pl);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 30, 0, &pl->pos[0], 1, 0);
        pl->motionTime = 0;
        if (variant == 0) {
            if (pl->weaponType == WEAPON_LONG_SWORD) {
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_END, 2, 0);
                break;
            }
            Player_SetMotionPair(pl, PL_MOT_WPN_EVADE_RIGHT, 2, 0);
            break;
        }
        if (pl->weaponType == WEAPON_LONG_SWORD) {
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_HOLD, 2, 0);
            break;
        }
        Player_SetMotionPair(pl, PL_MOT_WPN_EVADE_LEFT, 2, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
            break;
        }
        if (Player_IsLocal(pl) == 1 && pl->weaponOut != 0) {
            switch (pl->weaponType) {
            case WEAPON_SWORD_SHIELD:
            case WEAPON_DUAL_SWORDS:
                if ((pl->input.stickDirPressed & PAD_RSTICK_UP) && Motion_InFrameRange(pl, 0, 10.0f, 42.0f) != 0 && pl->stateVar0 == 0) {
                    pl->stateVar0 = 1;
                    if (pl->input.stickDir & PAD_LEFT) {
                        pl->stateVar0 = 2;
                    }
                    if (pl->input.stickDir & PAD_RIGHT) {
                        pl->stateVar0 = 3;
                    }
                }
                if (Motion_ReachedFrame(pl, 0, 42.0f) != 0 && (dir = pl->stateVar0)) {
                    if (dir == 2) {
                        pl->rot[1] += ANGLE_TURN / 8;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->stateVar0 == 3) {
                        pl->rot[1] -= ANGLE_TURN / 8;
                        pl->targetYaw = pl->rot[1];
                    }
                    if (pl->weaponType == WEAPON_SWORD_SHIELD) {
                        SwordShield_EvadeAttack(pl);
                        break;
                    }
                    DualSwords_EvadeAttack(pl);
                }
                break;
            }
        }
        break;
    }
}

f32 gSpotLineupOffsets[4] = {
#include "game/008C3AE0.inc"
};
s16 gDrawnWalkStopIdle[16] = {
#include "game/008C3AF0.inc"
};
WeaponMotion gDrawMotions[12] = {
#include "game/008C3B10.inc"
};
WeaponMotion gSheatheMotions[12] = {
#include "game/008C3B70.inc"
};
WeaponMotion gSheatheMovingMotions[12] = {
#include "game/008C3BD0.inc"
};
WeaponMotion gDrawMovingMotions[12] = {
#include "game/008C3C30.inc"
};

/* ---- The second part: from Player_StateBedLieDown on ---- */
/* Declarations of the functions called below. */

/* prototypes */
typedef struct SpitMeat {
    s16 raw;                    /* 0x00: 0: raw meat (ITEM_ID_MEAT) is used up, else the item and a bone (ITEM_ID_ROAST_LEFTOVER) back */
    s16 jingle;                 /* 0x02: played at the start; picks the result jingles */
    s16 rare;                   /* 0x04: the frame it is rare */
    s16 done;                   /* 0x06: the frame it is done */
    s16 doneLen;                /* 0x08: frames it stays done (burnt after) */
    s16 gourmet;                /* 0x0A: the gourmet window, from done + gourmet */
    s16 gourmetLen;             /* 0x0C */
    s16 unk0E;
    u16 itemRare;               /* 0x10: the items given */
    u16 itemDone;               /* 0x12 */
    u16 itemGourmet;            /* 0x14 */
    u16 itemBurnt;              /* 0x16 */
} SpitMeat;
extern SpitMeat gSpitMeats[];
void Player_ClearBuffs(Player *pl);
extern s8 gStunMashNeed[];
s16 Pouch_GiveItem(Player *p, u16 item, s16 key, u8 flags);
void Item_UseTeamPowder(Player *pl);
static void Player_ClearMash(Player *em);
void Item_Use(Player *pl);
void Item_UsePlaced(Player *pl);
extern GestureOneShot gGestureOneShots[];
/* end of prototypes */

void Player_StateBedLieDown(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    pl->fieldCollide = 0;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_BED_LIE_DOWN, -6, 0);
        pl->poison = 0;
        pl->mashBuildup = 0;
        pl->mashLeft = 0;
        pl->status7Timer = 0;
        pl->status6Timer = 0;
        pl->status5Buildup = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_SLEEP, 0);
        }
        break;
    }
}

/*
 * PL_ACT_BED_SLEEP: sleeps on the bed, regaining 1 health a frame
 * (Player_AddHp) with a puff effect three times a second; the local player
 * gets up (PL_ACT_BED_GET_UP) after 120 frames. Walls are ignored
 * (fieldCollide 0).
 */
void Player_StateBedSleep(Player *pl, s32 variant)
{
    f32 fxOffset[3];

    pl->noPushTimer = 2;
    pl->fieldCollide = 0;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->motionTime = 0;
        pl->timer = 120;
        Player_SetMotionPair(pl, PL_MOT_BED_SLEEP, 0, 0);
        Effect06_SpawnAtBone(pl, 0, ITEM_GLOW_HEAL, 10, 4.0f);
        break;
    case 1:
        if (--pl->timer <= 0 && Player_IsLocal(pl) == 1) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_BED_GET_UP, 0);
            break;
        }
        if (pl->motionId[0] == 427 && !(Rand_Next(1) & 0x7F)) {
            Player_SetMotionPair(pl, PL_MOT_BED_SLEEP_STIR, 4, 0);
        } else if (pl->motionId[0] == 428 && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_BED_SLEEP, 4, 0);
        }
        fxOffset[0] = 0.0f;
        fxOffset[1] = 20.0f;
        fxOffset[2] = 0.0f;
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 20, fxOffset, 0.8f);
        }
        Player_AddHp(pl, 1);
        break;
    }
}

/*
 * PL_ACT_BED_GET_UP: turns half a turn and gets up from the bed, then idle.
 */
void Player_StateBedGetUp(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    pl->fieldCollide = 0;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->motionTime = 0;
        pl->rot[1] = (pl->rot[1] + 0x7FFF + 1) & 0xFFFF;
        pl->targetYaw = (u16) pl->rot[1];
        Player_SetMotionPair(pl, PL_MOT_BED_GET_UP, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Aims the mounted gun of an action spot of kind 6. variant 0
 * PL_ACT_MOUNTED_GUN_AIM (resets the pitch), 1 PL_ACT_MOUNTED_GUN_REAIM
 * (after a shot, keeps it). yawCenter is the spot's angle; the local player
 * aims with Player_UpdateLean. After 30 frames PAD_CIRCLE or PAD_R3 with the
 * gun's ammo item in the pouch requests PL_ACT_MOUNTED_GUN_FIRE; after 60
 * frames PAD_CROSS (selector closed) or the quest's end returns to idle.
 */
void Player_StateMountedGunAim(Player *pl, s32 variant)
{
    f32 pos[3];
    u16 angle;
    s8 kind;

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_MOUNTED_GUN, 4, 0);
            pl->aimPitch = 0;
            pl->aimPitchPct = 0;
        } else {
            Player_SetMotionPair(pl, PL_MOT_MOUNTED_GUN, 0, 66);
        }
        Player_UpdateMotionTime(pl, 0);
        pl->timer = 60;
        pl->stateVar0 = 30;
        if (Player_FindActionSpot((Player *)pl, (f32 *)pos, &angle, (u8 *)&kind) == 6) {
            pl->yawCenter = angle;
            break;
        }
        pl->yawCenter = pl->targetYaw;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (pl->stateVar0 > 0) {
            pl->stateVar0--;
        }
        if (Player_IsLocal(pl) == 1) {
            if (Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            Player_UpdateLean(pl, 414);
            if (pl->stateVar0 == 0 && (pl->input.pressed & (PAD_CIRCLE | PAD_R3)) && Pouch_Count(pl, 667) != 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_FIRE, PL_SET_SNAP_POS | PL_SET_SNAP_YAW | PL_SET_SYNC_ALT);
                break;
            }
            if (pl->timer == 0 && (pl->input.pressed2 & PAD_CROSS) && pl->itemSelect == 0) {
                Player_ReturnToIdle(pl, 0, 6, 0);
            }
        }
        break;
    }
}

/*
 * PL_ACT_MOUNTED_GUN_FIRE: uses up one of the gun's ammo item, shakes the pad
 * and makes the shot (StageShot_CreateForPlayer type 0). After 60 frames it aims
 * again (PL_ACT_MOUNTED_GUN_REAIM) while ammo is left, else returns to idle.
 */
void Player_StateMountedGunFire(Player *pl, s32 variant)
{

    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_MOUNTED_GUN, 0, 66);
        Player_UpdateMotionTime(pl, 0);
        pl->timer = 60;
        if (Player_IsLocal(pl) != 1 || Pouch_Count(pl, 667) != 0) {
            Pouch_Add(pl, 667, -1);
            Vib_StartLocal(pl, 2);
            StageShot_CreateForPlayer(pl, STAGESHOT_MOUNTED_GUN);
        }
        break;
    case 1:
        pl->timer -= 1;
        if (pl->timer == 0) {
            if (Pouch_Count(pl, 667) > 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_ACT_CANNON_FIRE: fires the cannon of an action spot of kind 8 (from the
 * carry state): pad and camera shakes and the shot StageShot_CreateForPlayer(pl,
 * 1), then idle.
 */
void Player_StateCannonFire(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_BRACE, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Vib_StartLocal(pl, 3);
        CamShake_ReqA(pl, 5);
        StageShot_CreateForPlayer(pl, STAGESHOT_CANNON);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_BRACE_END, 2, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/* PL_ACT_CROUCH_STAND_UP: stands up from the crouch, then idle. */
void Player_StateCrouchStandUp(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_CROUCH_STAND_UP, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_ACT_LEDGE_PULL_UP: climbs up from the ledge hang (the step-up motion),
 * then idle.
 */
void Player_StateLedgePullUp(Player *pl, s32 unused)
{
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_LEDGE_CLIMB_UP, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_ACT_CROUCH_WALK: walks crouched. The local player rolls out
 * (PL_ACT_ROLL_FROM_CROUCH) on PAD_CROSS with 75 stamina and mustCrouch clear;
 * otherwise Player_CrouchInput reads the controls, and stops the walk only
 * once pl->timer (15) is 0.
 */
void Player_StateCrouchWalk(Player *pl, s32 unused)
{
    s32 timer;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_CROUCH_WALK, 8, 0);
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        pl->weaponOut = 0;
        Player_SetFlag(pl, 0x1000U);
        pl->freeState = 1;
        pl->timer = 15;
        break;
    case 1:
        timer = pl->timer;
        if (timer != 0) {
            pl->timer = timer - 1;
        }
        if (Player_IsLocal(pl) == 1) {
            if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75) && (pl->mustCrouch == 0)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL_FROM_CROUCH, PL_SET_SNAP_POS);
                break;
            }
            Player_CrouchInput(pl);
        } else {
        }
        break;
    }
}

/*
 * PL_ACT_CROUCH_STOP: stops the crouched walk; after 12 frames it switches to
 * PL_ACT_CROUCH_HOLD.
 */
void Player_StateCrouchStop(Player *pl, s32 variant)
{
    s16 blend;

    pl->timer++;
    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->motionId[0] == 37) {
            blend = 12;
        } else {
            blend = 8;
        }
        Player_SetPosture(pl, PL_POSTURE_CROUCHED | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_CROUCH, blend, 0);
        pl->timer = 0;
        break;
    case 1:
        if (pl->timer >= 12) {
            Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, 0);
        }
        break;
    }
}

/*
 * PL_ACT_CROUCH_WALK_FAST: the faster crouched walk (the stick fully pushed);
 * controls as Player_StateCrouchWalk.
 */
void Player_StateCrouchWalkFast(Player *pl, s32 unused)
{
    s32 timer;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_CROUCH_WALK_FAST, 6, 0);
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        Player_SetFlag(pl, 0x02000000U);
        pl->freeState = 1;
        pl->timer = 15;
        break;
    case 1:
        timer = pl->timer;
        if (timer != 0) {
            pl->timer = timer - 1;
        }
        if (Player_IsLocal(pl) == 1) {
            if ((pl->input.pressed & PAD_CROSS) && (pl->stamina >= 75) && (pl->mustCrouch == 0)) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_ROLL_FROM_CROUCH, PL_SET_SNAP_POS);
                break;
            }
            Player_CrouchInput(pl);
        } else {
        }
        break;
    }
}

/*
 * PL_ACT_BATTLE_ALERT: the start of a battle with the weapon sheathed: one
 * motion, then idle.
 */
void Player_StateBattleAlert(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_BATTLE_ALERT, 2, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * PL_ACT_TURN_AROUND: the quick half turn when the stick is pushed back
 * against a run (Player_IdleActions). Takes targetYaw at once, then requests
 * PL_ACT_RUN_AFTER_DASH.
 */
void Player_StateTurnAround(Player *pl, s32 unused)
{
    pl->freeState = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->rot[1] = (s32) pl->targetYaw;
        Player_SetMotionPair(pl, PL_MOT_TURN_AROUND, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH, 0);
        }
        break;
    }
}

/* moves the player 3 units back along its heading (forward when fwd is set) */
void Player_StepBack(Player *pl, s32 fwd)
{
    f32 stepVec[3];
    f32 stepWorld[3];
    s32 ang[3];
    f32 yawMtx[16];

    ang[0] = 0;
    ang[2] = 0;
    ang[1] = pl->rot[1];
    Mtx_FromAnglesXYZ(ang, yawMtx);
    stepVec[0] = 0.0f;
    stepVec[1] = 0.0f;
    stepVec[2] = -3.0f;
    if ((s16) fwd) {
        stepVec[2] *= -1.0f;
    }
    Vec_Rotate(stepWorld, stepVec, yawMtx);
    pl->pos[0] += stepWorld[0];
    pl->pos[2] += stepWorld[2];
}

/*
 * The local player's input with its back to a wall: the left stick's world
 * heading (Player_GetStickYaw) against the player's picks
 * PL_ACT_WALL_SIDLE_POS (ANGLE_TURN / 8 to ANGLE_TURN * 3 / 8 off), PL_ACT_WALL_SIDLE_NEG (ANGLE_TURN * 5 / 8
 * to ANGLE_TURN * 7 / 8) or PL_ACT_WALL_PRESS_HOLD. PAD_CROSS, or no wall behind
 * (Player_TurnAlongWallSide, side 1), requests PL_ACT_RUN_STOP.
 */
void Player_UpdateWallSidle(Player *pl)
{
    s32 angleDelta;

    if (Player_IsLocal(pl) != 0) {
        switch (Player_StickMoveLevel(pl, 0)) {
        case 1:
        case 2:
        case 3:
            angleDelta = (u16) ((u16) Player_GetStickYaw(pl, 0) - *(u16 *) &pl->rot[1]);
            if (angleDelta >= (ANGLE_TURN / 8) && angleDelta <= ANGLE_TURN * 3 / 8) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_POS) == 0) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_POS, 0);
                }
            } else if (angleDelta >= (ANGLE_TURN * 5 / 8) && angleDelta <= ANGLE_TURN * 7 / 8) {
                if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_NEG) == 0) {
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_SIDLE_NEG, 0);
                }
            } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS) == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS_HOLD) == 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS_HOLD, 0);
            }
            break;
        default:
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS) == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS_HOLD) == 0) {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_WALL_PRESS_HOLD, 0);
            }
            break;
        }
        if ((pl->input.pressed & PAD_CROSS) || !(Player_TurnAlongWallSide(pl, 1) & 0xFF)) {
            pl->freeState = 1;
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
        }
    }
}

/*
 * variant 0 PL_ACT_WALL_PRESS (puts its back to a wall), 1
 * PL_ACT_WALL_PRESS_HOLD (from frame 30 of the same motion). Pressed back
 * into the wall every frame (Player_StepBack); from frame 30
 * Player_UpdateWallSidle reads the stick.
 */
void Player_StateWallPress(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_WALL_PRESS, 2, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WALL_PRESS, 2, 30);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING | PL_POSTURE_NO_CAM_FOLLOW, 0, 0);
        Player_StepBack(pl, 0);
        break;
    case 1:
        Player_StepBack(pl, 0);
        if (Motion_ReachedFrame(pl, 0, 30.0f) != 0) {
            Player_UpdateWallSidle(pl);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_WALL_SIDLE_NEG, 1 PL_ACT_WALL_SIDLE_POS: sidles along a
 * wall with its back to it, pressed into it every frame;
 * Player_UpdateWallSidle reads the stick.
 */
void Player_StateWallSidle(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_WALL_SIDLE_NEG, 2, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WALL_SIDLE_POS, 2, 0);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        Player_StepBack(pl, 0);
        break;
    case 1:
        Player_StepBack(pl, 0);
        Player_UpdateWallSidle(pl);
        break;
    }
}

/*
 * PL_ACT_DIVE: the forward dive from the flee sprint. Costs 75 stamina and is
 * hit-free until the landing; the player leaps 16 forward and lands on
 * pl->floorY with a pad and camera shake. Then PL_ACT_FLEE_DASH_RESUME with
 * PAD_R2 held, else PL_ACT_RUN_STOP.
 */
void Player_StateDive(Player *pl, s32 unused)
{
    f32 move[3];
    f32 delta[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetMotionPair(pl, PL_MOT_DIVE_START, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 30, 0, pl->pos, 1, 0);
        pl->noHitTimer = 3;
        break;
    case 1:
        pl->noHitTimer = 3;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_DIVE_AIR, 0, 0);
            pl->pos[1] += 100.0f;
            move[0] = 0.0f;
            move[1] = 0.0f;
            move[2] = 16.0f;
            Vec_Rotate(delta, move, (f32 *) pl->worldMtx);
            pl->vel[0] = delta[0];
            pl->vel[1] = 3.0f;
            pl->vel[2] = delta[2];
            Player_ClearAccel(pl);
            Player_StartJump(pl, 8);
        }
        break;
    case 2:
        pl->noHitTimer = 3;
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= pl->floorY) {
            pl->step++;
            pl->action = PL_POSTURE_STANDING;
            pl->pos[1] = pl->floorY;
            Player_SetMotionPair(pl, PL_MOT_DIVE_LAND, 0, 0);
            Vib_StartLocal(pl, 1);
            CamShake_ReqA(pl, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            pl->noHitTimer = 0;
            if (pl->input.held & PAD_R2) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FLEE_DASH_RESUME, 0);
            } else {
                pl->freeState = 1;
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_RUN_STOP, 0);
            }
        } else if (Motion_ReachedFrame(pl, 0, 30.0f) == 0) {
            pl->noHitTimer = 2;
        }
        break;
    }
}

/* gathering at the player's spot (kind 0: a gather point, 1-3: a mining point); stat: 1, 2 count quest stats 14, 15. 1: an item was taken */
s32 Player_GatherAtSpot(Player *pl, u16 kind, u16 stat)
{
    u16 item;
    u16 sel;
    u16 id;
    s16 giveResult;

    if (Quest_PhaseIsOver(1) == 1) {
        return 0;
    }
    switch (kind) {
    case 0:
        item = Gather_AtPoint(pl);
        break;
    case 1:
    case 2:
    case 3:
        item = Gather_AtMinePoint(pl);
        break;
    }
    if (item == 0 || item == 0xFFFE || item == 0xFFFF) {
        if (item != 0xFFFE) {
            switch (kind) {
            case 0:
                Notice_Push(NOTICE_KIND_MESSAGE, 0, 0);
                break;
            case 1:
                Notice_Push(NOTICE_KIND_MESSAGE, 14, 0);
                break;
            case 2:
                Notice_Push(NOTICE_KIND_MESSAGE, 27, 0);
                break;
            case 3:
                Notice_Push(NOTICE_KIND_MESSAGE, 0, 0);
                break;
            }
        } else {
            Jingle_Play(pl, 10);
            Notice_PushItem((char *)gNothingMoreText);
        }
    } else {
        switch (item) {
        case 676:
            Notice_Push(NOTICE_KIND_ITEM, 19, item);
            break;
        case ITEM_ID_DELIVERY:
            Notice_Push(NOTICE_KIND_ITEM, 20, item);
            break;
        }
        switch (stat) {
        case 0:
            break;
        case 1:
            QuestStats_Add(14, 1);
            break;
        case 2:
            QuestStats_Add(15, 1);
            break;
        }
        if (kind == 1) {
            MsgEvent_Check(MSGEV_MINING);
        }
        giveResult = Pouch_GiveItem(pl, item, pl->gotItem, 3);
        switch (giveResult) {
        case 0:
        case 1:
        case 2:
            if (Item_CarryKind(item) & 0xFF) {
                pl->stateVar1 = 1;
                MsgEvent_Check(MSGEV_CARRY_ITEM);
                if (item == 675) {
                    Notice_Push(NOTICE_KIND_ITEM, 20, item);
                }
            }
            sel = pl->pouchCursor;
            id = pl->pouch[sel].item;
            if (id != 0 && !(gItemData[id].flags & 1)) {
                pl->pouchCursor = Pouch_NextSelectable(pl, sel, 0);
            }
            return 1;
        }
    }
    return 0;
}

/*
 * The standing gathers. variant: 0 PL_ACT_GATHER, 2 PL_ACT_TAKE_REWARDS
 * (Quest_GiveRewards), 3 PL_ACT_CARVE, 4 PL_ACT_MINE and 5 PL_ACT_BUG_NET
 * (with the tool pl->useItem), 6 PL_ACT_GATHER_SPECIAL, 7 PL_ACT_GATHER_DROP.
 * When pl->timer runs out (60 frames, 40 with a tool) the local player
 * gathers once (Player_GatherAtSpot); the pickaxe and the net break on a 1 in
 * n roll by the tool (skills change the odds). SKILL_GATHER_FAST speeds the motion.
 * When the motion ends it returns to idle, or picks up a gathered carried
 * item (PL_GROUP_CARRY, PL_CARRY_PICKED_UP).
 */
void Player_StateGather(Player *pl, u32 variant)
{
    u16 n;

    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->timer = 0;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        switch (variant) {
        case 0:
        case 1:
        case 6:
        case 7:
            Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -14, 0);
            if (Player_HasSkill(pl, SKILL_GATHER_FAST) == 1) {
                pl->motion[0].step = 2.6f;
                pl->motion[1].step = 2.6f;
            }
            break;
        case 2:
            if (Player_IsLocal(pl) == 1) {
                Notice_Push(NOTICE_KIND_MESSAGE, 10, 0);
            }
            Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -14, 0);
            if (Player_HasSkill(pl, SKILL_GATHER_FAST) == 1) {
                pl->motion[0].step = 2.6f;
                pl->motion[1].step = 2.6f;
            }
            break;
        case 3:
            Player_SetMotionPair(pl, PL_MOT_CARVE, -6, 0);
            break;
        case 4:
            Player_SetMotionPair(pl, PL_MOT_PICKAXE_SWING, 0, 0);
            if (Player_HasSkill(pl, SKILL_GATHER_FAST) == 1) {
                pl->timer = 40;
                pl->motion[0].step = 2.8f;
                pl->motion[1].step = 2.8f;
            }
            break;
        case 5:
            Player_SetMotionPair(pl, PL_MOT_NET_SWING, 0, 0);
            if (Player_HasSkill(pl, SKILL_GATHER_FAST) == 1) {
                pl->timer = 40;
                pl->motion[0].step = 2.8f;
                pl->motion[1].step = 2.8f;
            }
            break;
        }
        break;
    case 1:
        if (Player_IsLocal(pl) == 1) {
            if (pl->timer > 0) {
                pl->timer--;
            }
            if (pl->timer == 0 && pl->stateVar0 == 0 && Quest_PhaseIsOver(1) == 0) {
                n = 1;
                pl->stateVar0 = 1;
                switch (variant) {
                case 4:
                case 5:
                    switch (pl->useItem) {
                    case 150:
                        n = 3;
                        break;
                    case 151:
                        n = 10;
                        break;
                    case 152:
                        n = 15;
                        break;
                    case 153:
                        n = 3;
                        break;
                    case 154:
                        n = 10;
                        break;
                    case 155:
                        n = 15;
                        break;
                    }
                    if (Player_HasSkill(pl, SKILL_TOOL_LOSS_UP) == 1 && (Rand_Next(1) & 3) == 0) {
                        n = n / 2U + 1;
                    }
                    if (Player_HasSkill(pl, SKILL_TOOL_LOSS_UP2) == 1 && (Rand_Next(1) & 1) == 0) {
                        n = n / 2U + 1;
                    }
                    if (Rand_Next(1) % n == 0
                        && (Player_HasSkill(pl, SKILL_TOOL_LOSS_DOWN) != 1 || (Rand_Next(1) & 3))
                        && (Player_HasSkill(pl, SKILL_TOOL_LOSS_DOWN2) != 1 || (Rand_Next(1) & 1))) {
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_TOOL_LOST, pl->useItem);
                        Pouch_Add(pl, pl->useItem, -1);
                    }
                default:
                    switch (variant) {
                    default:
                        Player_GatherAtSpot(pl, 0, 0);
                        break;
                    case 4:
                        Player_GatherAtSpot(pl, 0, 1);
                        break;
                    case 5:
                        Player_GatherAtSpot(pl, 0, 2);
                        break;
                    }
                    break;
                case 6:
                    Player_GatherAtSpot(pl, 2, 0);
                    break;
                case 7:
                    Player_GatherAtSpot(pl, 3, 0);
                    break;
                case 2:
                    Quest_GiveRewards(pl);
                    break;
                case 3:
                    Player_GatherAtSpot(pl, 1, 0);
                    break;
                }
                break;
            }
        }
        if (pl->motion[0].playing == 0) {
            if (pl->stateVar1 == 0) {
                switch (variant) {
                case 0:
                case 1:
                case 2:
                case 6:
                case 7:
                    Player_ReturnToIdle(pl, 0, 10, 0);
                    break;
                case 3:
                case 5:
                    Player_ReturnToIdle(pl, 0, 6, 0);
                    break;
                default:
                    Player_ReturnToIdle(pl, 0, 4, 0);
                    break;
                }
            } else {
                Player_UpdateCarryKind(pl);
                Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_PICKED_UP, 0);
            }
        } else if (Motion_PassedFrame(pl, 0, 100.0f) != 0 && variant == 3 && Player_HasSkill(pl, SKILL_GATHER_FAST) == 1) {
            Player_SetMotionPair(pl, PL_MOT_CARVE, -8, 224);
        }
        break;
    }
}

/*
 * The crouched forms of Player_StateGather's gather, carve, special and drop.
 * variant: 0 PL_ACT_GATHER_CROUCHED, 1 PL_ACT_CARVE_CROUCHED, 2
 * PL_ACT_GATHER_SPECIAL_CROUCHED, 3 PL_ACT_GATHER_DROP_CROUCHED; the variant
 * is also the kind passed to Player_GatherAtSpot, which the local player
 * calls once at pl->timer 60. Ends in PL_ACT_CROUCH_HOLD, or the pickup of a
 * carried item.
 */
void Player_StateGatherCrouched(Player *pl, s32 variant)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->timer = 0;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        switch (variant) {
        case 0:
        case 2:
        case 3:
            Player_SetMotionPair(pl, PL_MOT_GATHER_FROM_CROUCH, -6, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_CARVE_FROM_CROUCH, -6, 0);
            break;
        }
        if (Player_HasSkill(pl, SKILL_GATHER_FAST) == 1) {
            pl->motion[0].step = 2.6f;
            pl->motion[1].step = 2.6f;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            switch (variant) {
            case 0:
            case 2:
            case 3:
                Player_SetMotionPair(pl, PL_MOT_KNEEL_GATHER, -4, 36);
                break;
            case 1:
                Player_SetMotionPair(pl, PL_MOT_CARVE, 0, 68);
                break;
            }
        }
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && (pl->timer >= 60) && (pl->stateVar0 == 0) && (Quest_PhaseIsOver(1) == 0)) {
            pl->stateVar0 = 1;
            Player_GatherAtSpot(pl, variant & 0xFFFF, 0);
        }
        switch (variant) {
        case 0:
        case 2:
        case 3:
            if (Motion_ReachedFrame(pl, 0, 200.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_GATHER_TO_CROUCH, -4, 0);
            }
            break;
        case 1:
            if (Motion_ReachedFrame(pl, 0, 294.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_CARVE_TO_CROUCH, -2, 0);
            }
            break;
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            if (pl->stateVar1 == 0) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, PL_SET_NO_SYNC);
                break;
            }
            Player_UpdateCarryKind(pl);
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_PICKED_UP, 0);
        }
        break;
    }
}

/*
 * PL_ACT_SPIT_ROAST_START: sets up the spit roast (Player_UseItem with a
 * meat, or an action spot of kind 1): clears cookTimer and cookResult and
 * turns the camera to the spit (CamCtl_TurnToPlayerFront); then
 * PL_ACT_SPIT_ROAST.
 */
void Player_StateSpitRoastStart(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        pl->cookTimer = 0;
        pl->cookResult = 0;
        Player_SetMotionPair(pl, PL_MOT_SPIT_ROAST_START, -4, 0);
        CamCtl_TurnToPlayerFront((u8 *) pl);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST, PL_SET_NO_SYNC);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_SPIT_ROAST_SUCCESS (done or gourmet), 1
 * PL_ACT_SPIT_ROAST_FAIL (raw, rare or burnt): one motion, then idle.
 */
void Player_StateSpitRoastEnd(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_SPIT_ROAST_SUCCESS, 0, 0);
            break;
        }
        Player_SetMotionPair(pl, PL_MOT_SPIT_ROAST_FAIL, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The fishing input (Player_StateFishWait), local player: returns to idle
 * when the state group changed or the quest is over. In PL_ACT_FISH_WAIT,
 * PAD_CIRCLE reels in: PL_ACT_FISH_REEL with a bite on the line (fishTimer
 * above 0), else PL_ACT_FISH_REEL_BAITED or PL_ACT_FISH_REEL_EMPTY by baited.
 */
void Player_UpdateFishingInput(Player *pl)
{

    if (Player_IsLocal(pl) != 0) {
        if (pl->stateGroup != PL_GROUP_ACTION) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            return;
        }
        if (Quest_PhaseIsOver(1) == 1) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            return;
        }
        switch (pl->state) {
        case 78:
        case 79:
        case 83:
            break;
        case 80:
            if (pl->input.pressed & PAD_CIRCLE) {
                if (pl->fishTimer > 0) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL, 0);
                } else if (pl->baited == 1) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL_BAITED, 0);
                } else {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL_EMPTY, 0);
                }
            }
            break;
        case 81:
        case 82:
            if (pl->input.pressed & PAD_CIRCLE) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_REEL, 0);
            }
            break;
        }
    }
}

/*
 * variant 0 PL_ACT_FISH_READY, 1 PL_ACT_FISH_WAIT (the line out): notes a
 * bite in stateVar1 and runs Player_UpdateFishingInput.
 */
void Player_StateFishWait(Player *pl, s32 variant)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_FISHING);
        pl->weaponOut = 0;
        pl->timer = 0;
        pl->stateVar1 = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_FISH_READY, 0, 0);
            break;
        }
        Player_SetMotionPair(pl, PL_MOT_FISH_WAIT, 2, 0);
        break;
    case 1:
        if (((s8) pl->fishTimer != 0) && (pl->stateVar1 == 0)) {
            pl->stateVar1 = 1;
        }
        Player_UpdateFishingInput(pl);
        break;
    }
}

/*
 * PL_ACT_FISH_CAST: casts the line at an action spot of kind 2 (sets baited);
 * at frame 104 makes the lure (Lure_Spawn), then PL_ACT_FISH_WAIT.
 */
void Player_StateFishCast(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_FISHING);
        pl->weaponOut = 0;
        pl->fishTimer = 0;
        pl->gotItemId = 0;
        pl->baited = 1;
        Player_SetMotionPair(pl, PL_MOT_FISH_CAST, -2, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 104.0f) != 0) {
            if (pl->useItem == 145) {
                Lure_Spawn(pl, 1);
            } else {
                Lure_Spawn(pl, 0);
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_FISH_WAIT, 0);
        }
        break;
    }
}

/*
 * PL_ACT_FISH_REEL: reels in. A monster that took this player's bait (hooked,
 * hookedBy) is kept in stateVar0. With none, the catch: the local player uses
 * up the bait, gets gotItemId (Pouch_GiveItem) with a jingle and counts the
 * quest stat; with a monster on the line the reel repeats twice and the bait
 * is lost. Then idle.
 */
void Player_StateFishReel(Player *pl, s32 variant)
{
    Em *em = gEms;
    s16 i;

    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        pl->msgTimer = 0;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_FISHING);
        pl->weaponOut = 0;
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->area == pl->area && em->hooked != 0 && em->hookedBy == pl->index) {
                pl->stateVar0 = 1;
            }
        }
        Player_SetMotionPair(pl, PL_MOT_FISH_REEL, 2, 0);
        break;
    case 1:
        if (pl->stateVar0 == 0) {
            if (Motion_PassedFrame(pl, 0, 118.0f) != 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_FISH_CATCH, 2, 0);
                break;
            }
            for (i = 0; i < EM_COUNT; i++, em++) {
                if (em->area == pl->area && em->hooked != 0 && em->hookedBy == pl->index) {
                    pl->stateVar0 = 1;
                }
            }
            break;
        }
        if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->stateVar1++;
            if (pl->stateVar1 >= 2) {
                pl->step = 3;
                Player_SetMotionPair(pl, PL_MOT_FISH_PULLED_DOWN, 4, 0);
                if (Quest_PhaseIsOver(1) == 0) {
                    Pouch_Add(pl, pl->useItem, -1);
                }
            }
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 120.0f) != 0 && Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_FISHED, pl->gotItemId);
            Jingle_Play(pl, 7);
            Pouch_Add(pl, pl->useItem, -1);
            i = pl->gotItem;
            if ((s16) Pouch_GiveItem(pl, pl->gotItemId, i, 0) == 3) {
                Player_NoticePouchFull(pl->gotItemId);
            }
            QuestStats_Add(13, 1);
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 2, 36);
        }
        break;
    case 4:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_FISH_REEL_EMPTY (the bait lost), 1 PL_ACT_FISH_REEL_BAITED
 * (the bait kept): reels in nothing; the local player gets a notice, variant
 * 0 uses up the bait. Then idle.
 */
void Player_StateFishReelEmpty(Player *pl, s32 variant)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_FISHING);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_FISH_REEL_EMPTY, 2, 0);
        break;
    case 1:
        if ((Motion_PassedFrame(pl, 0, 120.0f) != 0) && (Player_IsLocal(pl) == 1) && (Quest_PhaseIsOver(1) == 0)) {
            if (variant == 0) {
                Jingle_Play(pl, 10);
                Pouch_Add(pl, (s32) pl->useItem, -1);
                Notice_Push(NOTICE_KIND_MESSAGE, 3, 0);
            } else {
                Notice_Push(NOTICE_KIND_MESSAGE, 13, 0);
            }
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * Returns the item's row of gSpitMeats (the spit roast's timings and
 * results), 0 to 12; 0 for the first meat item and for any item not in the
 * list.
 */
u8 SpitMeat_Kind(u16 item)
{
    u8 kind = 0;

    switch (item) {
    case 660:
        kind = 0;
        break;
    case 156:
        kind = 1;
        break;
    case 157:
        kind = 2;
        break;
    case 158:
        kind = 3;
        break;
    case 159:
        kind = 4;
        break;
    case 160:
        kind = 5;
        break;
    case 164:
        kind = 6;
        break;
    case 165:
        kind = 7;
        break;
    case 166:
        kind = 8;
        break;
    case 167:
        kind = 9;
        break;
    case 163:
        kind = 10;
        break;
    case 168:
        kind = 11;
        break;
    case 169:
        kind = 12;
        break;
    }
    return kind;
}

/*
 * PL_ACT_SPIT_ROAST: turns the meat. The row m of gSpitMeats (SpitMeat_Kind
 * of pl->useItem) times cookResult as cookTimer counts: 0 raw, 1 rare (from
 * m->rare), 2 done (m->done), 4 gourmet (m->gourmetLen frames from m->gourmet
 * after done), 3 burnt (m->doneLen after done); the SKILL_ROAST_WINDOW_* skills shift the
 * windows. The local player takes the meat off with PAD_CIRCLE or at the time
 * limit: the meat is used up and, from rare on, the result's item given with
 * a jingle and a notice. Raw, rare and burnt request PL_ACT_SPIT_ROAST_FAIL,
 * done and gourmet PL_ACT_SPIT_ROAST_SUCCESS.
 */
void Player_StateSpitRoast(Player *pl, s32 unused)
{
    SpitMeat *meat;
    s16 done;
    s16 doneLen;
    s16 gourmet;
    s16 gourmetLen;
    s16 limit;
    s32 j;
    s16 n;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    pl->noPushTimer = 2;
    meat = &gSpitMeats[SpitMeat_Kind(pl->useItem)];
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_SPIT_ROAST_TURN, 4, Rand_Next(1) & 0x3F);
        pl->cookTimer = 0;
        pl->cookResult = 0;
        Jingle_Play(pl, meat->jingle);
        break;
    case 1:
        pl->cookTimer++;
        done = meat->done;
        doneLen = meat->doneLen;
        gourmet = meat->gourmet;
        gourmetLen = meat->gourmetLen;
        if (Player_HasSkill(pl, SKILL_ROAST_WINDOW_WIDE) == 1) {
            done -= 2;
            doneLen += 4;
            if (gourmet > 0) {
                gourmet -= 1;
                gourmetLen += 2;
            }
        } else if (Player_HasSkill(pl, SKILL_ROAST_WINDOW_WIDER) == 1) {
            done -= 4;
            doneLen += 8;
            if (gourmet > 0) {
                gourmet -= 2;
                gourmetLen += 4;
            }
        } else if (Player_HasSkill(pl, SKILL_ROAST_WINDOW_HALF) == 1) {
            doneLen /= 2;
            if (gourmet > 0) {
                gourmet /= 2;
                gourmetLen /= 2;
            }
        }
        if (pl->cookTimer >= done + doneLen) {
            pl->cookResult = 3;
        } else if (pl->cookTimer >= done) {
            if (gourmet > 0 && pl->cookTimer >= done + gourmet && pl->cookTimer < gourmetLen + (done + gourmet)) {
                pl->cookResult = 4;
            } else {
                pl->cookResult = 2;
            }
        } else if (pl->cookTimer >= meat->rare) {
            pl->cookResult = 1;
        } else {
            pl->cookResult = 0;
        }
        if (SpitMeat_Kind(pl->useItem) == 2) {
            limit = 180;
        } else {
            limit = 330;
        }
        if (Player_IsLocal(pl) == 1 && ((pl->input.pressed & PAD_CIRCLE) || pl->cookTimer >= limit)) {
            Bgm_StopEvent(pl);
            switch (pl->cookResult) {
            case 0:
                if (Quest_PhaseIsOver(1) == 0) {
                    Notice_Push(NOTICE_KIND_MESSAGE, 11, 0);
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            case 1:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (meat->raw == 0) {
                        Pouch_Add(pl, 40, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 162, 1);
                    }
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_COOKED, meat->itemRare);
                    Jingle_Play(pl, 14);
                    n = pl->gotItem;
                    if ((s16) Pouch_GiveItem(pl, meat->itemRare, n, 0) == 3) {
                        Player_NoticePouchFull(meat->itemRare);
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            case 2:
            case 4:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (meat->raw == 0) {
                        Pouch_Add(pl, 40, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 162, 1);
                    }
                    if (pl->cookResult == 4) {
                        switch (meat->jingle) {
                        case 30:
                            j = 34;
                            break;
                        case 25:
                            j = 27;
                            break;
                        case 22:
                            j = 24;
                            break;
                        case 19:
                            j = 21;
                            break;
                        case 31:
                        case 32:
                        default:
                            j = 18;
                            break;
                        }
                    } else {
                        switch (meat->jingle) {
                        case 30:
                            j = 33;
                            break;
                        case 25:
                            j = 26;
                            break;
                        case 22:
                            j = 23;
                            break;
                        case 19:
                            j = 20;
                            break;
                        case 31:
                        case 32:
                        default:
                            j = 15;
                            break;
                        }
                    }
                    Jingle_Play(pl, j);
                    if (pl->cookResult == 4) {
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_COOKED, meat->itemGourmet);
                        n = pl->gotItem;
                        if ((s16) Pouch_GiveItem(pl, meat->itemGourmet, n, 0) == 3) {
                            Player_NoticePouchFull(meat->itemGourmet);
                        }
                    } else {
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_COOKED, meat->itemDone);
                        n = pl->gotItem;
                        if ((s16) Pouch_GiveItem(pl, meat->itemDone, n, 0) == 3) {
                            Player_NoticePouchFull(meat->itemDone);
                        }
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_SUCCESS, 0);
                break;
            case 3:
                if (Quest_PhaseIsOver(1) == 0) {
                    if (meat->raw == 0) {
                        Pouch_Add(pl, 40, -1);
                    } else {
                        Pouch_Add(pl, pl->useItem, -1);
                        Pouch_Add(pl, 162, 1);
                    }
                    Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_BURNT, meat->itemBurnt);
                    Jingle_Play(pl, 16);
                    n = pl->gotItem;
                    if ((s16) Pouch_GiveItem(pl, meat->itemBurnt, n, 0) == 3) {
                        Player_NoticePouchFull(meat->itemBurnt);
                    }
                }
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_SPIT_ROAST_FAIL, 0);
                break;
            }
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_START_COUNTDOWN (strikes the countdown spot, an action
 * spot of kind 9: Quest_StartCountdown), 1 PL_ACT_SPOT_ITEM_STRIKE (the spot
 * item at an action spot of kind 0xA: StageShot_CreateForPlayer type 7, one item
 * used up). Then idle.
 */
void Player_StateSpotStrike(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_PICKAXE_SWING, 0, 0);
        if (variant == 0) {
            Quest_StartCountdown((Player *) pl);
            break;
        }
        StageShot_CreateForPlayer(pl, STAGESHOT_ARENA_STRIKE);
        Pouch_Add(pl, (s32) pl->useItem, -1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_SUPPLY_BOX, 1 PL_ACT_DELIVERY_BOX: stands hit-free while
 * the box's menu is open (supplyBoxOpen, or deliveryOpen with the cursor
 * reset); returns to idle once the menu clears its flag or the quest is over.
 */
void Player_StateBoxMenu(Player *pl, s32 variant)
{
    u8 isOpen;

    pl->noPushTimer = 2;
    pl->noHitTimer = 5;
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, PL_MOT_IDLE, 6, 0);
        }
        if (variant == 0) {
            pl->supplyBoxOpen = 1;
        } else {
            pl->deliveryOpen = 1;
            pl->menuCursor = 0;
            pl->menuPage = 0;
        }
        pl->menuDelay = 0;
        if (Player_IsLocal(pl) == 1) {
            Sound_PlaySeFlat(1, 142, 0);
        }
        break;
    case 1:
        if (variant == 0) {
            isOpen = pl->supplyBoxOpen;
        } else {
            isOpen = pl->deliveryOpen;
        }
        if (isOpen & 0xFF) {
            if (Quest_PhaseIsOver(1) == 1) {
                goto block_17;
            }
        } else {
block_17:
            pl->noHitTimer = 0;
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_DRINK, 1 PL_ACT_EAT. When the motion ends the local player
 * requests PL_ACT_APPLY_ITEM, another player's copy returns to idle. SKILL_EAT_FAST
 * speeds the eating; SKILL_EAT_SLOW replays part of it up to 4 times.
 */
void Player_StateConsume(Player *pl, s32 variant)
{
    u8 stateVar0;

    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        pl->stateVar1 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_DRINK, 4, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_EATING, 4, 0);
            break;
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_APPLY_ITEM, 0);
                break;
            }
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        if (variant == 1) {
            if ((Player_HasSkill(pl, SKILL_EAT_SLOW) == 1) && (Motion_PassedFrame(pl, 0, 62.0f) != 0)) {
                stateVar0 = pl->stateVar0;
                if ((s32) stateVar0 < 4) {
                    pl->stateVar0 = stateVar0 + 1;
                    pl->stateVar1 = 1;
                    Player_SetMotionPair(pl, PL_MOT_EATING, 4, 52);
                }
            }
            if ((Motion_ReachedFrame(pl, 0, 38.0f) != 0) && (Motion_ReachedFrame(pl, 0, 180.0f) == 0) && (Player_HasSkill(pl, SKILL_EAT_FAST) == 1)) {
                pl->motion[0].step = 3.6f;
                pl->motion[1].step = 3.6f;
                break;
            }
            pl->motion[0].step = 2.0f;
            pl->motion[1].step = 2.0f;
        } else {
        }
        break;
    }
}

/*
 * PL_ACT_APPLY_ITEM: at entry the local player applies the item just consumed
 * (Item_Use of pl->useItem); then idle.
 */
void Player_StateApplyItem(Player *pl, s32 unused)
{
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (Player_IsLocal(pl) == 1) {
            Item_Use(pl);
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_ACT_KNEEL_TO_PLACE_CROUCHED: as Player_StateKneelToPlace from the
 * crouch; at frame 20 the local player requests PL_ACT_PLACE_ITEM_CROUCHED.
 */
void Player_StateKneelToPlaceCrouched(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_PLACE_ITEM_CROUCHED, 2, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 20.0f) != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_ITEM_CROUCHED, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
        } else {
        case 2:
            break;
        }
        break;
    }
}

/*
 * Starts the trade cooldown: pl->tradeCooldown = 9000 frames unless it still
 * runs.
 */
static void Player_StartTradeTimer(Player *pl)
{
    if (pl->tradeCooldown <= 0) {
        pl->tradeCooldown = 9000;
    }
}

/*
 * PL_ACT_TRADE: trades with the trader (EM_KIND_TRADER; from
 * Player_StartSpotAction). Stands hit-free while its message shows
 * (tradeMsgTimer 60), then idle. tradeCount counts the trades up to 7; from 4
 * on half of them jump to 7, and reaching 7 starts the cooldown
 * (Player_StartTradeTimer).
 */
void Player_StateTrade(Player *pl, s32 variant)
{

    pl->noHitTimer = 3;
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, PL_MOT_IDLE, -6, 0);
        }
        pl->tradeMsgTimer = 60;
        if (pl->tradeCount < 7) {
            pl->tradeCount++;
            if (pl->tradeCount >= 4 && (Rand_Next(1) & 1)) {
                pl->tradeCount = 7;
                Player_StartTradeTimer(pl);
            }
        } else {
            pl->tradeCount = 7;
            Player_StartTradeTimer(pl);
        }
        pl->timer = 60;
        break;
    case 1:
        if (pl->tradeMsgTimer == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_HORN_PLAY (the horn item), 1 PL_ACT_TEAM_ITEM_USE (a team
 * item). The motion repeats: the horn up to 3 times (PAD_CROSS from frame 44
 * ends it), then PL_ACT_HORN_END; the team item twice, then
 * PL_ACT_TEAM_ITEM_END.
 */
void Player_StateInstrumentPlay(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_HORN_PLAY, 4, 0);
            Player_SetFlag(pl, 0x10000);
        } else {
            Player_SetMotionPair(pl, PL_MOT_TEAM_ITEM_USE, 4, 0);
        }
        pl->motionTime = 0;
        break;
    case 1:
        switch (variant) {
        case 0:
            if (pl->stateVar0 >= 3 || (Motion_ReachedFrame(pl, 0, 44.0f) != 0 && (pl->input.pressed & PAD_CROSS))) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_HORN_END, 0);
            } else if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
                pl->stateVar0++;
            }
            break;
        case 1:
            if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
                pl->stateVar0++;
                if (pl->stateVar0 >= 2) {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TEAM_ITEM_END, 0);
                }
            }
            break;
        }
        break;
    }
}

/*
 * variant 0 PL_ACT_HORN_END, 1 PL_ACT_TEAM_ITEM_END (the local player applies
 * the team item, Item_UseTeamPowder). At entry the local player may use up
 * pl->useItem on a 1 in n roll by the item; the SKILL_TOOL_LOSS_* skills and the
 * hunting horn change the odds. Then idle.
 */
void Player_StateInstrumentEnd(Player *pl, s32 variant)
{
    u16 chance = 1;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        if (variant == 0) {
            chance = 5;
            Player_SetMotionPair(pl, PL_MOT_HORN_END, 2, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_TEAM_ITEM_END, 4, 0);
            if (Player_IsLocal(pl) == 1) {
                Item_UseTeamPowder(pl);
            }
            switch (pl->useItem) {
            case ITEM_ID_TEAM_HEAL:
                chance = 12;
                break;
            case ITEM_ID_TEAM_CURE:
                chance = 12;
                break;
            case ITEM_ID_TEAM_ATTACK:
                chance = 8;
                break;
            case ITEM_ID_TEAM_DEFENSE:
                chance = 8;
                break;
            }
        }
        if (Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0) {
            if (Player_HasSkill(pl, SKILL_TOOL_LOSS_UP) == 1 && !(Rand_Next(1) & 3)) {
                chance = chance / 2 + 1;
            }
            if (Player_HasSkill(pl, SKILL_TOOL_LOSS_UP2) == 1 && !(Rand_Next(1) & 1)) {
                chance = chance / 2 + 1;
            }
            if (Rand_Next(1) % chance == 0) {
                if (Player_HasSkill(pl, SKILL_TOOL_LOSS_DOWN) != 1 || (Rand_Next(1) & 3)) {
                    if ((Player_HasSkill(pl, SKILL_TOOL_LOSS_DOWN2) != 1 || (Rand_Next(1) & 1)) && (pl->weaponType != WEAPON_HUNTING_HORN || (Rand_Next(1) & 3))) {
                        Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_TOOL_LOST, pl->useItem);
                        Pouch_Add(pl, pl->useItem, -1);
                    }
                }
            }
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_ACT_SHARPEN: sharpens with the whetstone pl->useItem. At frame 260 one
 * item is used up and Player_AddSharpness adds 100 to 200 by the item (half
 * with SKILL_SHARPEN_HALF); SKILL_SHARPEN_FAST skips part of the motion. Then idle.
 */
void Player_StateSharpen(Player *pl, s32 unused)
{
    s16 n;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_SHARPEN, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (Player_HasSkill(pl, SKILL_SHARPEN_FAST) == 1 && Motion_PassedFrame(pl, 0, 98.0f) != 0) {
            Player_SetMotionPair(pl, PL_MOT_SHARPEN, 0, 238);
        }
        if (Motion_PassedFrame(pl, 0, 260.0f) != 0 && Quest_PhaseIsOver(1) == 0) {
            Pouch_Add(pl, pl->useItem, -1);
            switch (pl->useItem) {
            case 242:
                n = 200;
                break;
            case 172:
            case 234:
                n = 150;
                break;
            case 661:
                n = 100;
                break;
            }
            if (Player_HasSkill(pl, SKILL_SHARPEN_HALF) == 1) {
                n = n / 2;
            }
            Player_AddSharpness(pl, n);
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Looks through the binoculars in the aim view (aimMode 1). variant 0
 * PL_ACT_BINOCULARS (crouches first), 1 PL_ACT_BINOCULARS_CROUCHED.
 * Player_UpdateLean aims; PAD_CROSS or the quest's end ends it
 * (Player_ResetAim, PL_ACT_CROUCH_HOLD).
 */
void Player_StateBinoculars(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_CROUCHED, 0, 0);
        pl->aimMode = 1;
        pl->weaponOut = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_CROUCH_DOWN, 2, 0);
            break;
        }
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_CROUCH, 2, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CROUCH, 6, 0);
            Player_UpdateLean(pl, 8);
            break;
        }
        Player_UpdateLean(pl, 7);
        break;
    case 2:
        Player_UpdateLean(pl, 8);
        if (Player_IsLocal(pl) != 0) {
            if (!(pl->input.pressed & PAD_CROSS)) {
                if (Quest_PhaseIsOver(1) == 1) {
                    goto block_15;
                }
            } else {
block_15:
                Player_ResetAim(pl);
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH_HOLD, 0);
            }
        }
        break;
    }
}

/*
 * PL_ACT_OFFER_ITEM: holds an item out to another player (offerItem,
 * offerCount, offerTo). The local player ends it (PL_ACT_OFFER_ITEM_END) with
 * PAD_CROSS or once offerState leaves PL_OFFER_WAITING (a taker asked:
 * QSYNC_PL_ITEM_ASK).
 */
void Player_StateOfferItem(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_OFFER_ITEM, 4, 0);
        pl->offerState = PL_OFFER_WAITING;
        break;
    case 1:
        if ((Player_IsLocal(pl) == 1) && ((pl->input.pressed & PAD_CROSS) || (pl->offerState != PL_OFFER_WAITING))) {
            Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_OFFER_ITEM_END, 0);
        }
        break;
    }
}

/*
 * PL_ACT_TAKE_ITEM_WAIT: waits for the giver's answer
 * (Player_TakeOfferedItem). PL_OFFER_RECEIVED (the item arrived:
 * QSYNC_PL_ITEM_ANSWER) requests PL_ACT_TAKE_ITEM; PL_OFFER_REFUSED or 601
 * frames without an answer return to idle.
 */
void Player_StateTakeItemWait(Player *pl, s32 unused)
{
    s32 timer;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, PL_MOT_IDLE, 6, 0);
        }
        pl->offerState = PL_OFFER_WAITING;
        pl->timer = 0;
        break;
    case 1:
        timer = pl->timer + 1;
        pl->timer = timer;
        if ((timer >= 601) || (pl->offerState != PL_OFFER_WAITING)) {
            if (pl->offerState == PL_OFFER_RECEIVED) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_TAKE_ITEM, 0);
                break;
            }
            Player_ReturnToIdle(pl, 0, 2, 0);
        } else {
        }
        break;
    }
}

/* PL_ACT_OFFER_ITEM_END: the giver's closing motion, then idle. */
void Player_StateOfferItemEnd(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_OFFER_ITEM_END, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_ACT_TAKE_ITEM: the taker's motion once the item has arrived, then idle.
 */
void Player_StateTakeItem(Player *pl, s32 unused)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32) pl, (u16 *) &pl->gotItem);
    Player_ResetAim(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_TAKE_ITEM, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The lance's and the gunlance's hop. variant: 0 PL_ACT_LANCE_HOP_SIDE_A, 1
 * PL_ACT_LANCE_HOP_SIDE_B, 2 PL_ACT_LANCE_HOP_BACK, 3
 * PL_ACT_LANCE_HOP_BACK_LONG. Costs 75 stamina and sets the hit-free frames.
 * From 30 frames before the motion's end a flick of the attack stick picks
 * the next weapon state into stateVar1 (255 none): down LANCE_UPPER_THRUST,
 * or for the gunlance GLANCE_HOP_RELOAD (clip not full) or
 * GLANCE_UPPER_THRUST; any other flick LANCE_THRUST or GLANCE_THRUST. From 10
 * frames before the end the left stick steers and the picked state starts in
 * PL_GROUP_WEAPON; with none it returns to idle.
 */
void Player_StateLanceHop(Player *pl, s32 variant)
{
    f32 move[3];
    f32 delta[3];
    u16 flickAngle;
    u16 steerAngle;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetEvadeFrames(pl);
        Player_AddStamina(pl, -75);
        Player_PlaySe(pl, 30, 0, pl->pos, 1, 0);
        pl->stateVar1 = 255;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_LANCE_HOP_SIDE_A, 2, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_LANCE_HOP_SIDE_B, 2, 0);
            break;
        case 2:
        case 3:
            Player_SetMotionPair(pl, PL_MOT_LANCE_HOP_BACK, 2, 0);
            break;
        }
        break;
    case 1:
        switch (variant) {
        case 0:
        case 1:
            if (pl->weaponType == WEAPON_GUNLANCE && Motion_ReachedFrame(pl, 0, 20.0f) == 0) {
                if (variant == 0) {
                    move[0] = 9.0f;
                } else {
                    move[0] = -9.0f;
                }
                move[1] = 0.0f;
                move[2] = 0.0f;
                Vec_Rotate(delta, move, pl->rotMtx);
                pl->pos[0] += delta[0];
                pl->pos[2] += delta[2];
            }
            break;
        case 3:
            if (Motion_ReachedFrame(pl, 0, 28.0f) == 0) {
                move[0] = 0.0f;
                move[1] = 0.0f;
                move[2] = -8.0f;
                Vec_Rotate(delta, move, pl->rotMtx);
                pl->pos[0] += delta[0];
                pl->pos[2] += delta[2];
            }
            break;
        }
        if (Motion_ReachedFrame(pl, 0, pl->motion[0].frameCount - 30.0f) != 0 && Player_IsLocal(pl) == 1) {
            if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                flickAngle = pl->input.rightStickAngle + 0x1555;
                switch (pl->weaponType) {
                case WEAPON_LANCE:
                    if (flickAngle < 0xAAAC) {
                        pl->stateVar1 = 0;
                    } else {
                        pl->stateVar1 = 1;
                    }
                    break;
                case WEAPON_GUNLANCE:
                    if (flickAngle < 0xAAAC) {
                        pl->stateVar1 = 0;
                    } else if (pl->roundsLoaded < pl->clipSize) {
                        pl->stateVar1 = 27;
                    } else {
                        pl->stateVar1 = 4;
                    }
                    break;
                }
            }
            if (Motion_ReachedFrame(pl, 0, pl->motion[0].frameCount - 10.0f) != 0 && pl->stateVar1 != 255) {
                if (pl->input.stickDir & PAD_DPAD) {
                    steerAngle = pl->input.stickAngle - 0x1555;
                    if (steerAngle >= 0xAAAB) {
                        if (pl->weaponType == WEAPON_GUNLANCE) {
                            pl->targetYaw -= 0x1555;
                        } else {
                            pl->targetYaw -= ANGLE_TURN / 16;
                        }
                    } else if (steerAngle >= 0x5555) {
                        if (pl->weaponType == WEAPON_GUNLANCE) {
                            pl->targetYaw += 0x1555;
                        } else {
                            pl->targetYaw += ANGLE_TURN / 16;
                        }
                    }
                }
                Player_SetState(pl, PL_GROUP_WEAPON, pl->stateVar1, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
            }
        }
        if (pl->motion[0].playing == 0) {
            if (variant == 2) {
                Player_ReturnToIdle(pl, 0, 6, 0);
            } else {
                Player_ReturnToIdle(pl, 0, 8, 0);
            }
        }
        break;
    }
}

/*
 * PL_ACT_PLACE_LARGE_ITEM: crouches to place a large item (pl->useItem); at
 * frame 78 the local player requests PL_ACT_PLACE_LARGE_ITEM_SET.
 */
void Player_StatePlaceLargeItem(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, PL_MOT_PLACE_LARGE_ITEM, 6, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 78.0f) != 0) {
            if (Player_IsLocal(pl) == 1) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_PLACE_LARGE_ITEM_SET, PL_SET_SNAP_POS | PL_SET_SNAP_YAW);
                break;
            }
            pl->step++;
        }
        break;
    case 2:
        Player_SetMotionPair(pl, PL_MOT_PLACE_LARGE_ITEM, 0, 78);
        break;
    }
}

/*
 * PL_ACT_PLACE_LARGE_ITEM_SET: the local player puts it down (Item_UsePlaced:
 * the placed object, one item used up); then idle.
 */
void Player_StatePlaceLargeItemSet(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        pl->timer = 0;
        Player_SetMotionPair(pl, PL_MOT_PLACE_LARGE_ITEM, 0, 78);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        if (Player_IsLocal(pl) == 1) {
            Item_UsePlaced(pl);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * PL_ACT_HORN_ITEM_PLAY: plays one of the horn items, as PL_ACT_HORN_PLAY:
 * the item sets the rounds (stateVar0, 1 to 3) and the use-up odds
 * (stateVar1: 1 in 8, 12 or 16). At the end the local player may use the item
 * up (skills and the hunting horn save it sometimes); then idle.
 */
void Player_StateHornItemPlay(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 1;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_HORN_PLAY, 4, 0);
        Player_SetFlag(pl, 0x10000);
        pl->motionTime = 0;
        switch (pl->useItem) {
        case 688:
        case 689:
        case 692:
        case 693:
            pl->stateVar0 = 2;
            pl->stateVar1 = 16;
            break;
        case 690:
            pl->stateVar0 = 1;
            pl->stateVar1 = 12;
            break;
        case 691:
        case 694:
            pl->stateVar0 = 3;
            pl->stateVar1 = 8;
            break;
        }
        break;
    case 1:
        if (pl->stateVar0 <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_HORN_END, 2, 0);
            if (Player_IsLocal(pl) == 1 && Quest_PhaseIsOver(1) == 0 && Rand_Next(1) % pl->stateVar1 == 0) {
                if (Player_HasSkill(pl, SKILL_TOOL_LOSS_DOWN) == 1) {
                    if (!(Rand_Next(1) & 3)) {
                        break;
                    }
                }
                if (Player_HasSkill(pl, SKILL_TOOL_LOSS_DOWN2) == 1) {
                    if (!(Rand_Next(1) & 1)) {
                        break;
                    }
                }
                if (pl->weaponType == WEAPON_HUNTING_HORN) {
                    if (!(Rand_Next(1) & 3)) {
                        break;
                    }
                }
                Notice_Push(NOTICE_KIND_ITEM, NOTICE_ITEM_TOOL_LOST, pl->useItem);
                Pouch_Add(pl, pl->useItem, -1);
            }
        } else if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->stateVar0--;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_ACTION (idle, moving,
 * climbing, items, the bed, fishing, gathering, the lance hops). States that
 * share a handler pass a variant (each handler's comment lists them); a state
 * with no case returns to idle.
 */
void Player_RunActionState(Player *pl)
{
    switch (pl->state) {
    case PL_ACT_IDLE:
        Player_StateIdle(pl, 0);
        break;
    case PL_ACT_RUN:
        Player_StateMove(pl, 0U);
        break;
    case PL_ACT_WALK:
        Player_StateMove(pl, 1U);
        break;
    case PL_ACT_WALK_DRAWN:
        Player_StateMove(pl, 2U);
        break;
    case PL_ACT_DRAW:
        Player_StateDrawSheathe(pl, 0U);
        break;
    case PL_ACT_SHEATHE:
        Player_StateDrawSheathe(pl, 1U);
        break;
    case PL_ACT_LEAP_OFF_LEDGE:
        Player_StateLeapOrFall(pl, 0);
        break;
    case PL_ACT_GUARD_WALK:
        Player_StateMove(pl, 8U);
        break;
    case PL_ACT_CROUCH:
        Player_StateCrouch(pl, 0);
        break;
    case PL_ACT_FALL:
        Player_StateLeapOrFall(pl, 1);
        break;
    case PL_ACT_SHEATHE_MOVING:
        Player_StateDrawSheathe(pl, 2U);
        break;
    case PL_ACT_THROW_ITEM_ALT:
        Player_StateThrowItem(pl, 1);
        break;
    case PL_ACT_BED_MENU:
        Player_StateBedMenu(pl, 0);
        break;
    case PL_ACT_TO_IDLE:
        Player_StateToIdle(pl, 0);
        break;
    case PL_ACT_THROW_ITEM:
        Player_StateThrowItem(pl, 0);
        break;
    case PL_ACT_SHEATHE_TO_CROUCH:
        Player_StateDrawSheathe(pl, 3U);
        break;
    case PL_ACT_SHEATHE_TO_USE_ITEM:
        Player_StateDrawSheathe(pl, 4U);
        break;
    case PL_ACT_LEDGE_HANG:
        Player_StateLedgeHang(pl, 0);
        break;
    case PL_ACT_RIDE_WALK:
        Player_StateMove(pl, 3U);
        break;
    case PL_ACT_MOVE_KEEP_MOTION:
        Player_StateMove(pl, 4U);
        break;
    case PL_ACT_PLACE_ITEM_CROUCHED:
        Player_StatePlaceItem(pl, 1);
        break;
    case PL_ACT_JUMP_TO_LEDGE:
        Player_StateClimbAirborne(pl, 0);
        break;
    case PL_ACT_RIDE_STAGGER:
        Player_StateRideStagger(pl, 0);
        break;
    case PL_ACT_DRAW_MOVING:
        Player_StateDrawSheathe(pl, 5U);
        break;
    case PL_ACT_PLACE_ITEM:
        Player_StatePlaceItem(pl, 0);
        break;
    case PL_ACT_KNEEL_TO_PLACE:
        Player_StateKneelToPlace(pl, 0);
        break;
    case PL_ACT_CROUCH_WALK_ALT:
        Player_StateMove(pl, 5U);
        break;
    case PL_ACT_LEDGE_GRAB:
        Player_StateLedgeHang(pl, 1);
        break;
    case PL_ACT_ROLL:
        Player_StateRoll(pl, 0);
        break;
    case PL_ACT_CROUCH_HOLD:
        Player_StateCrouch(pl, 1);
        break;
    case PL_ACT_STEP_UP:
        Player_StateStepUp(pl, 0);
        break;
    case PL_ACT_DASH:
        Player_StateDash(pl, 0);
        break;
    case PL_ACT_FLEE_DASH:
        Player_StateFleeDash(pl, 0);
        break;
    case PL_ACT_OUT_OF_BREATH:
        Player_StateOutOfBreath(pl, 0);
        break;
    case PL_ACT_DASH_TIRED:
        Player_StateFleeDash(pl, 1);
        break;
    case PL_ACT_RUN_AFTER_DASH:
        Player_StateMove(pl, 6U);
        break;
    case PL_ACT_TIRED_STOP:
        Player_StateTiredStop(pl, 0);
        break;
    case PL_ACT_LAND_HARD:
        Player_StateHardLanding(pl, 0);
        break;
    case PL_ACT_WALL_HOLD:
        Player_StateWallHold(pl, 0);
        break;
    case PL_ACT_WALL_CLIMB_UP:
        Player_StateWallClimbUp(pl, 0);
        break;
    case PL_ACT_WALL_CLIMB_DOWN:
        Player_StateWallClimbDown(pl, 0);
        break;
    case PL_ACT_WALL_DISMOUNT:
        Player_StateWallDismount(pl, 0);
        break;
    case PL_ACT_WALL_GRAB:
        Player_StateWallGrab(pl, 0);
        break;
    case PL_ACT_RUN_STOP:
        Player_StateRunStop(pl, 0);
        break;
    case PL_ACT_ROLL_FROM_CROUCH:
        Player_StateRoll(pl, 1);
        break;
    case PL_ACT_SET_TRAP:
        Player_StateSetTrap(pl, 0);
        break;
    case PL_ACT_SET_TRAP_DONE:
        Player_StateSetTrapDone(pl, 0);
        break;
    case PL_ACT_WALL_LET_GO:
        Player_StateClimbAirborne(pl, 1);
        break;
    case PL_ACT_EVADE_RIGHT:
        Player_StateSideEvade(pl, 0);
        break;
    case PL_ACT_EVADE_LEFT:
        Player_StateSideEvade(pl, 1);
        break;
    case PL_ACT_BED_LIE_DOWN:
        Player_StateBedLieDown(pl, 0);
        break;
    case PL_ACT_BED_SLEEP:
        Player_StateBedSleep(pl, 0);
        break;
    case PL_ACT_BED_GET_UP:
        Player_StateBedGetUp(pl, 0);
        break;
    case PL_ACT_MOUNTED_GUN_AIM:
        Player_StateMountedGunAim(pl, 0);
        break;
    case PL_ACT_CANNON_FIRE:
        Player_StateCannonFire(pl, 0);
        break;
    case PL_ACT_LEDGE_DROP:
        Player_StateClimbAirborne(pl, 2);
        break;
    case PL_ACT_CROUCH_STAND_UP:
        Player_StateCrouchStandUp(pl, 0);
        break;
    case PL_ACT_LEDGE_PULL_UP:
        Player_StateLedgePullUp(pl, 0);
        break;
    case PL_ACT_CROUCH_WALK:
        Player_StateCrouchWalk(pl, 0);
        break;
    case PL_ACT_CROUCH_STOP:
        Player_StateCrouchStop(pl, 0);
        break;
    case PL_ACT_CROUCH_WALK_FAST:
        Player_StateCrouchWalkFast(pl, 0);
        break;
    case PL_ACT_BATTLE_ALERT:
        Player_StateBattleAlert(pl, 0);
        break;
    case PL_ACT_TURN_AROUND:
        Player_StateTurnAround(pl, 0);
        break;
    case PL_ACT_WALL_PRESS:
        Player_StateWallPress(pl, 0);
        break;
    case PL_ACT_WALL_SIDLE_NEG:
        Player_StateWallSidle(pl, 0);
        break;
    case PL_ACT_WALL_SIDLE_POS:
        Player_StateWallSidle(pl, 1);
        break;
    case PL_ACT_WALL_PRESS_HOLD:
        Player_StateWallPress(pl, 1);
        break;
    case PL_ACT_DIVE:
        Player_StateDive(pl, 0);
        break;
    case PL_ACT_FLEE_DASH_RESUME:
        Player_StateFleeDash(pl, 2);
        break;
    case PL_ACT_DASH_RESUME:
        Player_StateDash(pl, 1);
        break;
    case PL_ACT_GATHER:
        Player_StateGather(pl, 0U);
        break;
    case PL_ACT_MOUNTED_GUN_FIRE:
        Player_StateMountedGunFire(pl, 0);
        break;
    case PL_ACT_WALK_SLOW:
        Player_StateMove(pl, 7U);
        break;
    case PL_ACT_CARVE:
        Player_StateGather(pl, 3U);
        break;
    case PL_ACT_SPIT_ROAST_FAIL:
        Player_StateSpitRoastEnd(pl, 1);
        break;
    case PL_ACT_SPIT_ROAST_START:
        Player_StateSpitRoastStart(pl, 0);
        break;
    case PL_ACT_SPIT_ROAST_SUCCESS:
        Player_StateSpitRoastEnd(pl, 0);
        break;
    case PL_ACT_FISH_READY:
        Player_StateFishWait(pl, 0);
        break;
    case PL_ACT_FISH_CAST:
        Player_StateFishCast(pl, 0);
        break;
    case PL_ACT_FISH_WAIT:
        Player_StateFishWait(pl, 1);
        break;
    case PL_ACT_MINE:
        Player_StateGather(pl, 4U);
        break;
    case PL_ACT_BUG_NET:
        Player_StateGather(pl, 5U);
        break;
    case PL_ACT_FISH_REEL:
        Player_StateFishReel(pl, 0);
        break;
    case PL_ACT_FISH_REEL_EMPTY:
        Player_StateFishReelEmpty(pl, 0);
        break;
    case PL_ACT_SPIT_ROAST:
        Player_StateSpitRoast(pl, 0);
        break;
    case PL_ACT_START_COUNTDOWN:
        Player_StateSpotStrike(pl, 0);
        break;
    case PL_ACT_SUPPLY_BOX:
        Player_StateBoxMenu(pl, 0);
        break;
    case PL_ACT_DRINK:
        Player_StateConsume(pl, 0);
        break;
    case PL_ACT_KNEEL_TO_PLACE_CROUCHED:
        Player_StateKneelToPlaceCrouched(pl, 0);
        break;
    case PL_ACT_EAT:
        Player_StateConsume(pl, 1);
        break;
    case PL_ACT_TRADE:
        Player_StateTrade(pl, 0);
        break;
    case PL_ACT_TAKE_REWARDS:
        Player_StateGather(pl, 2U);
        break;
    case PL_ACT_GATHER_CROUCHED:
        Player_StateGatherCrouched(pl, 0);
        break;
    case PL_ACT_HORN_PLAY:
        Player_StateInstrumentPlay(pl, 0);
        break;
    case PL_ACT_HORN_END:
        Player_StateInstrumentEnd(pl, 0);
        break;
    case PL_ACT_CARVE_CROUCHED:
        Player_StateGatherCrouched(pl, 1);
        break;
    case PL_ACT_SHARPEN:
        Player_StateSharpen(pl, 0);
        break;
    case PL_ACT_TEAM_ITEM_USE:
        Player_StateInstrumentPlay(pl, 1);
        break;
    case PL_ACT_TEAM_ITEM_END:
        Player_StateInstrumentEnd(pl, 1);
        break;
    case PL_ACT_FISH_REEL_BAITED:
        Player_StateFishReelEmpty(pl, 1);
        break;
    case PL_ACT_BINOCULARS:
        Player_StateBinoculars(pl, 0);
        break;
    case PL_ACT_BINOCULARS_CROUCHED:
        Player_StateBinoculars(pl, 1);
        break;
    case PL_ACT_OFFER_ITEM:
        Player_StateOfferItem(pl, 0);
        break;
    case PL_ACT_TAKE_ITEM_WAIT:
        Player_StateTakeItemWait(pl, 0);
        break;
    case PL_ACT_OFFER_ITEM_END:
        Player_StateOfferItemEnd(pl, 0);
        break;
    case PL_ACT_TAKE_ITEM:
        Player_StateTakeItem(pl, 0);
        break;
    case PL_ACT_LANCE_HOP_SIDE_A:
        Player_StateLanceHop(pl, 0);
        break;
    case PL_ACT_LANCE_HOP_SIDE_B:
        Player_StateLanceHop(pl, 1);
        break;
    case PL_ACT_LANCE_HOP_BACK:
        Player_StateLanceHop(pl, 2);
        break;
    case PL_ACT_LANCE_HOP_BACK_LONG:
        Player_StateLanceHop(pl, 3);
        break;
    case PL_ACT_PLACE_LARGE_ITEM:
        Player_StatePlaceLargeItem(pl, 0);
        break;
    case PL_ACT_PLACE_LARGE_ITEM_SET:
        Player_StatePlaceLargeItemSet(pl, 0);
        break;
    case PL_ACT_APPLY_ITEM:
        Player_StateApplyItem(pl, 0);
        break;
    case PL_ACT_RETURN_TO_CAMP:
        Player_StateThrowItem(pl, 2);
        break;
    case PL_ACT_HORN_ITEM_PLAY:
        Player_StateHornItemPlay(pl, 0);
        break;
    case PL_ACT_BOW_WALK:
        Player_StateMove(pl, 9U);
        break;
    case PL_ACT_GATHER_SPECIAL:
        Player_StateGather(pl, 6U);
        break;
    case PL_ACT_GATHER_SPECIAL_CROUCHED:
        Player_StateGatherCrouched(pl, 2);
        break;
    case PL_ACT_WALL_CLIMB_UP_FAST:
        Player_StateWallClimbUp(pl, 1);
        break;
    case PL_ACT_WALL_CLIMB_DOWN_FAST:
        Player_StateWallClimbDown(pl, 1);
        break;
    case PL_ACT_SPOT_ITEM_STRIKE:
        Player_StateSpotStrike(pl, 1);
        break;
    case PL_ACT_MOUNTED_GUN_REAIM:
        Player_StateMountedGunAim(pl, 1);
        break;
    case PL_ACT_DELIVERY_BOX:
        Player_StateBoxMenu(pl, 1);
        break;
    case PL_ACT_GATHER_DROP:
        Player_StateGather(pl, 7U);
        break;
    case PL_ACT_GATHER_DROP_CROUCHED:
        Player_StateGatherCrouched(pl, 3);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * PL_WPN_SHARED_ATTACK, the attack every weapon shares: one motion with
 * PlayerAttack 5, then idle. The second parameter is unused.
 */
void Player_StateSharedAttack(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_SHARED_ATTACK, 4, 0);
        PlayerAttack_Create(pl, 5U);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 1, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_WEAPON:
 * PL_WPN_SHARED_ATTACK goes to Player_StateSharedAttack, any other state to
 * the drawn weapon's dispatcher by pl->weaponType (the two bowguns share
 * Bowgun_RunState). Clears pl->unk310 first.
 */
void Player_RunWeaponState(Player *pl)
{
    pl->unk310 = 0;
    if (pl->state == 99) {
        Player_StateSharedAttack(pl, 0);
    } else {
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
            GreatSword_RunState(pl);
            return;
        case WEAPON_LONG_SWORD:
            LongSword_RunState(pl);
            return;
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_LIGHT_BOWGUN:
            Bowgun_RunState(pl);
            return;
        case WEAPON_BOW:
            Bow_RunState(pl);
            return;
        case WEAPON_HAMMER:
            Hammer_RunState(pl);
            return;
        case WEAPON_HUNTING_HORN:
            HuntingHorn_RunState(pl);
            return;
        case WEAPON_LANCE:
            Lance_RunState(pl);
            return;
        case WEAPON_GUNLANCE:
            Gunlance_RunState(pl);
            return;
        case WEAPON_SWORD_SHIELD:
            SwordShield_RunState(pl);
            return;
        case WEAPON_DUAL_SWORDS:
            DualSwords_RunState(pl);
            return;
        }
    }
}

/*
 * PL_REACT_KNOCKDOWN_FRONT: falls, gets up and returns to idle, unhittable
 * throughout (noHitTimer).
 */
void Player_StateKnockdownFront(Player *pl)
{
    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_KNOCKDOWN_FACE_DOWN, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 6, 34);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * A guarded hit slides the player back with the guard sound and a pad shake.
 * variant 0 PL_REACT_GUARD_STAGGER (a short slide), 1
 * PL_REACT_GUARD_KNOCKBACK (a long one, the heavy bowgun with its own
 * motion). The lance, gunlance and heavy bowgun slide less. Ends in
 * PL_REACT_GUARD_HOLD for the lance and gunlance, else in idle.
 */
void Player_StateGuardRecoil(Player *pl, s32 variant)
{
    f32 slideLocal[3];
    f32 slideWorld[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_STAGGER, 2, 0);
        } else if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
            Player_SetMotionPair(pl, PL_MOT_HBG_GUARD_KNOCKBACK, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_KNOCKBACK, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Sound_PlayPlayerSe(pl, 103, 0, pl->pos, 1, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        Vib_StartLocal(pl, 1);
        Player_SetFlag(pl, 2);
        Player_SetFlag(pl, PLF_GUARD);
        pl->weaponOut = 1;
        break;
    case 1:
        slideLocal[2] = 0.0f;
        if (variant == 0) {
            if (Motion_ReachedFrame(pl, 0, 30.0f) == 0) {
                slideLocal[0] = 0.0f;
                slideLocal[1] = 0.0f;
                switch (pl->weaponType) {
                case WEAPON_LANCE:
                case WEAPON_GUNLANCE:
                case WEAPON_HEAVY_BOWGUN:
                    slideLocal[2] = -15.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 32.0f)) / 360.0f);
                    break;
                case WEAPON_GREAT_SWORD:
                case WEAPON_LONG_SWORD:
                case WEAPON_SWORD_SHIELD:
                    slideLocal[2] = -20.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 32.0f)) / 360.0f);
                    break;
                }
                Vec_Rotate(slideWorld, slideLocal, pl->rotMtx);
                pl->pos[0] += slideWorld[0];
                pl->pos[2] += slideWorld[2];
            }
        } else {
            slideLocal[0] = 0.0f;
            slideLocal[1] = 0.0f;
            switch (pl->weaponType) {
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
            case WEAPON_HEAVY_BOWGUN:
                if (Motion_ReachedFrame(pl, 0, 90.0f) == 0) {
                    slideLocal[2] = -16.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 104.0f)) / 360.0f);
                }
                break;
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
                if (Motion_ReachedFrame(pl, 0, 104.0f) == 0) {
                    slideLocal[2] = -22.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 104.0f)) / 360.0f);
                }
                break;
            case WEAPON_SWORD_SHIELD:
                if (Motion_ReachedFrame(pl, 0, 94.0f) == 0) {
                    slideLocal[2] = -18.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 94.0f)) / 360.0f);
                }
                break;
            }
            Vec_Rotate(slideWorld, slideLocal, pl->rotMtx);
            pl->pos[0] += slideWorld[0];
            pl->pos[2] += slideWorld[2];
        }
        if (pl->motion[0].playing == 0) {
            switch (pl->weaponType) {
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
                break;
            case WEAPON_SWORD_SHIELD:
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            case WEAPON_GREAT_SWORD:
            case WEAPON_LONG_SWORD:
            case WEAPON_HEAVY_BOWGUN:
            default:
                Player_ReturnToIdle(pl, 0, 10, 0);
                break;
            }
        }
        break;
    }
}

/*
 * PL_REACT_BLOWN_BACK: flung backward onto the back. Ends the demon mode and
 * launches the player up and back; it lands near pl->floorY (impact sound,
 * armour rattle), lies, gets up and returns to idle. With no health left the
 * local player faints (PL_GROUP_FAINT, PL_FAINT_FACE_UP); another player's
 * copy waits on the ground. Unhittable throughout.
 */
void Player_StateBlownBack(Player *pl)
{
    f32 kickLocal[3];
    f32 kickWorld[3];
    s32 ang[3];
    f32 yawMtx[16];
    f32 y;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, PL_MOT_BLOWN_BACK_LAUNCH, 0, 16);
        Em_GetBonePos(pl, 2, kickLocal);
        if (pl->pos[1] < kickLocal[1]) {
            pl->pos[1] = kickLocal[1];
        }
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, yawMtx);
        kickLocal[0] = 0.0f;
        kickLocal[1] = 0.0f;
        kickLocal[2] = -11.17f;
        Vec_Rotate(kickWorld, kickLocal, yawMtx);
        pl->vel[0] = 2.0f * kickWorld[0];
        pl->vel[2] = 2.0f * kickWorld[2];
        pl->vel[1] = 6.0f;
        Player_StartJump(pl, 12);
        pl->vel[1] = 0.0f;
        pl->accel[1] = -1.0f / 3.0f;
        pl->timer = 16;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && pl->motionId[0] == 205) {
            Player_SetMotionPair(pl, PL_MOT_FLUNG_BACK_AIR, 0, 0);
        }
        Player_MoveAccel(pl);
        if (pl->timer > 0) {
            pl->timer--;
            y = 63.0f + pl->floorY;
            if (pl->pos[1] <= y) {
                pl->pos[1] = y;
            }
        } else if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->step++;
            pl->pos[1] = pl->floorY;
            Sound_PlayPlayerSe(pl, 66, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, EQUIP_BODY);
            Player_SetMotionPair(pl, PL_MOT_BLOWN_BACK_LAND, 0, 0);
            pl->action = PL_POSTURE_DOWN;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->hp > 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 2, 0);
            } else if (Player_IsLocal(pl) == 1) {
                Player_RequestStateSynced(pl, PL_GROUP_FAINT, PL_FAINT_FACE_UP, 0);
            } else {
                pl->step = 4;
                Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
            pl->noHitTimer = 6;
        }
        break;
    case 4:
        break;
    }
}

/*
 * The guard. variant 0 PL_REACT_GUARD_START, 1 PL_REACT_GUARD_HOLD, 2
 * PL_REACT_GUARD_HOLD_ALT (differs for the gunlance only). The motions are
 * the weapon's. For the local player (any player offline), from frame 18
 * letting go of PAD_R1 requests PL_REACT_GUARD_END, and Player_GuardInput
 * starts the guard's actions (not for the heavy bowgun).
 */
void Player_StateGuard(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            default:
            case WEAPON_LONG_SWORD:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD, 3, 0);
                break;
            case WEAPON_SWORD_SHIELD:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_START, 2, 0);
                break;
            case WEAPON_LANCE:
            case WEAPON_GUNLANCE:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_START, 2, 0);
                break;
            }
        } else {
            switch (pl->weaponType) {
            case WEAPON_GREAT_SWORD:
            default:
            case WEAPON_LONG_SWORD:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD, 6, 40);
                break;
            case WEAPON_SWORD_SHIELD:
            case WEAPON_LANCE:
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_HOLD, 2, 0);
                break;
            case WEAPON_GUNLANCE:
                if (variant == 1) {
                    Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_HOLD, 2, 0);
                } else {
                    Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_START, 4, 4);
                }
                break;
            case WEAPON_HEAVY_BOWGUN:
                Player_SetMotionPair(pl, PL_MOT_HBG_GUARD, 2, 0);
                break;
            }
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_GUARD);
        pl->weaponOut = 1;
        break;
    case 1:
        if (Player_IsLocal(pl) == 1 || Game_IsOnline() == 0) {
            if (pl->motionTime > 18 && !(pl->input.held & PAD_R1)) {
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_END, 0);
                break;
            }
        }
        switch (pl->weaponType) {
        case WEAPON_LANCE:
        case WEAPON_GUNLANCE:
        case WEAPON_SWORD_SHIELD:
            if (pl->motionId[0] == 1005 && pl->motion[0].playing == 0) {
                Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_HOLD, 2, 0);
            }
            if (pl->motionTime > 18) {
                Player_GuardInput(pl);
            }
            break;
        case WEAPON_GREAT_SWORD:
        case WEAPON_LONG_SWORD:
            if (pl->motionTime > 18) {
                Player_GuardInput(pl);
            }
            break;
        }
        break;
    }
}

/*
 * PL_REACT_ROAR_STRONG: braces against the strongest roar, unhittable until
 * frame 64, with effects until frame 40; then idle.
 */
void Player_StateRoarStrong(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, PL_MOT_ROAR_STRONG_BRACE, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            pl->noHitTimer = 6;
            break;
        }
        if (Motion_ReachedFrame(pl, 0, 40.0f) == 0 && pl->motionTime % 7 == 0) {
            PartFx_SpawnForPlayer(pl, 5, 9);
            PartFx_SpawnForPlayer(pl, 8, 9);
        }
        if (Motion_ReachedFrame(pl, 0, 64.0f) == 0) {
            pl->noHitTimer = 2;
        }
        break;
    }
}

/*
 * PL_REACT_BLOWN_FORWARD: flung forward onto the face; as
 * Player_StateBlownBack without the jump, and the local player faints into
 * PL_FAINT_FACE_DOWN.
 */
void Player_StateBlownForward(Player *pl)
{
    f32 kickLocal[3];
    f32 kickWorld[3];
    s32 ang[3];
    f32 yawMtx[16];
    f32 y;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, PL_MOT_BLOWN_FORWARD_LAUNCH, 0, 0);
        Em_GetBonePos(pl, 2, kickLocal);
        if (pl->pos[1] < kickLocal[1]) {
            pl->pos[1] = kickLocal[1];
        }
        pl->targetYaw = pl->rot[1];
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = (u16) pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, yawMtx);
        kickLocal[0] = 0.0f;
        kickLocal[1] = 0.0f;
        kickLocal[2] = 11.5f;
        Vec_Rotate(kickWorld, kickLocal, yawMtx);
        pl->vel[0] = 2.0f * kickWorld[0];
        pl->vel[2] = 2.0f * kickWorld[2];
        pl->vel[1] = 0.0f;
        pl->accel[1] = -0.25f;
        pl->timer = 16;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && pl->motionId[0] == 208) {
            Player_SetMotionPair(pl, PL_MOT_FLUNG_FORWARD_AIR, 0, 0);
        }
        Player_MoveAccel(pl);
        if (pl->timer > 0) {
            pl->timer--;
            y = 63.0f + pl->floorY;
            if (pl->pos[1] <= y) {
                pl->pos[1] = y;
            }
        } else if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->step++;
            pl->pos[1] = pl->floorY;
            Sound_PlayPlayerSe(pl, 66, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, EQUIP_BODY);
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_DOWN, 0, 0);
            pl->action = PL_POSTURE_DOWN;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            if (pl->hp > 0) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
            } else if (Player_IsLocal(pl) == 1) {
                Player_RequestStateSynced(pl, PL_GROUP_FAINT, PL_FAINT_FACE_DOWN, 0);
            } else {
                pl->step = 4;
                Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    case 4:
        break;
    }
}

/*
 * PL_REACT_KNOCKDOWN_BACK: falls, gets up and returns to idle, unhittable
 * throughout.
 */
void Player_StateKnockdownBack(Player *pl)
{
    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, PL_MOT_KNOCKDOWN_FACE_UP, 0, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 6, 58);
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * Thrown into the air, landing lying down. variant 0 PL_REACT_TOSSED_BACK, 1
 * PL_REACT_TOSSED_FORWARD: the direction and the motions. Ends the demon mode
 * and launches the player; on landing it turns half a turn, gets up and
 * returns to idle. Unhittable throughout.
 */
void Player_StateTossed(Player *pl, s32 variant)
{
    f32 kickLocal[3];
    f32 kickWorld[3];
    s32 ang[3];
    f32 yawMtx[16];
    f32 z;

    pl->noHitTimer = 2;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_TOSSED_BACK_LAUNCH, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_TOSSED_FORWARD_LAUNCH, 0, 0);
        }
        Em_GetBonePos(pl, 2, kickLocal);
        if (pl->pos[1] < kickLocal[1]) {
            pl->pos[1] = kickLocal[1];
        }
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, yawMtx);
        kickLocal[0] = 0.0f;
        kickLocal[1] = 0.0f;
        if (variant != 0) {
            z = 7.0f;
        } else {
            z = -7.0f;
        }
        kickLocal[2] = z;
        Vec_Rotate(kickWorld, kickLocal, yawMtx);
        pl->vel[0] = 2.0f * kickWorld[0];
        pl->vel[2] = 2.0f * kickWorld[2];
        pl->vel[1] = 20.0f;
        pl->accel[1] = -8.0f / 11.0f;
        break;
    case 1:
        if (pl->motion[0].playing == 0 && (pl->motionId[0] == 213 || pl->motionId[0] == 219)) {
            if (variant == 0) {
                Player_SetMotionPair(pl, PL_MOT_FLUNG_BACK_AIR, 0, 0);
            } else {
                Player_SetMotionPair(pl, PL_MOT_FLUNG_FORWARD_AIR, 0, 0);
            }
        }
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->step++;
            Sound_PlayPlayerSe(pl, 66, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, EQUIP_BODY);
            Player_SetMotionPair(pl, PL_MOT_TOSSED_LAND, 0, 0);
            pl->pos[1] = pl->floorY;
            pl->action = PL_POSTURE_DOWN;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->rot[1] = (pl->rot[1] + ANGLE_TURN / 2) & 0xFFFF;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * PL_REACT_GUARD_END: lowers the guard with the weapon's motion, then idle.
 */
void Player_StateGuardEnd(Player *pl)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        switch (pl->weaponType) {
        case WEAPON_GREAT_SWORD:
        default:
        case WEAPON_LONG_SWORD:
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_STAGGER, 2, 0);
            break;
        case WEAPON_SWORD_SHIELD:
        case WEAPON_LANCE:
        case WEAPON_GUNLANCE:
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_END, 2, 0);
            break;
        case WEAPON_HEAVY_BOWGUN:
            Player_SetMotionPair(pl, PL_MOT_HBG_GUARD_END, 2, 0);
            break;
        }
        Player_SetFlag(pl, PLF_GUARD);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * The light hit. variant 0 PL_REACT_FLINCH_FRONT (pushed back), 1
 * PL_REACT_FLINCH_BACK (pushed forward), relative to pl->targetYaw until
 * frame 16; the motion depends on weaponOut. Then idle.
 */
void Player_StateFlinch(Player *pl, s32 variant)
{
    f32 pushLocal[3];
    s32 ang[3];
    f32 yawMtx[16];
    f32 z;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        if (pl->weaponOut != 0) {
            Player_SetMotionPair(pl, PL_MOT_FLINCH_DRAWN, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_FLINCH, 0, 0);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        ang[0] = 0;
        ang[2] = 0;
        ang[1] = pl->targetYaw;
        Mtx_FromAnglesXYZ(ang, yawMtx);
        pushLocal[0] = 0.0f;
        pushLocal[1] = 0.0f;
        if (variant != 0) {
            z = 4.0f;
        } else {
            z = -4.0f;
        }
        pushLocal[2] = z;
        Vec_Rotate(&pl->vel[0], pushLocal, yawMtx);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 16.0f) == 0) {
            Player_Move(pl);
        }
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_REACT_GUARD_PUSH: a light guarded hit: a short guard motion, the guard
 * sound and a pad shake, then PL_REACT_GUARD_HOLD (the heavy bowgun returns
 * to idle).
 */
void Player_StateGuardPush(Player *pl)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
            Player_SetMotionPair(pl, PL_MOT_HBG_GUARD, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_PUSH, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Sound_PlayPlayerSe(pl, 103, 0, &pl->pos[0], 1, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        Player_SetFlag(pl, 0x8000U);
        Vib_StartLocal(pl, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (pl->weaponType == WEAPON_HEAVY_BOWGUN) {
                Player_ReturnToIdle(pl, 0, 6, 0);
                break;
            }
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        } else {
        }
        break;
    }
}

/*
 * The roar that holds the player in place. variant 0 PL_REACT_ROAR, 1
 * PL_REACT_ROAR_SHORT. Pushed back until frame 40 with effects; the short
 * form ends at its first pass of frame 44, the long one at its second. Then
 * idle.
 */
void Player_StateRoar(Player *pl, s32 variant)
{
    f32 slideLocal[3];
    f32 slideWorld[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        Player_SetMotionPair(pl, PL_MOT_ROAR_COVER, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 28.0f) != 0 && variant == 1) {
            pl->step = 3;
            Player_SetMotionPair(pl, PL_MOT_ROAR_RECOVER, 6, 20);
        } else if (Motion_ReachedFrame(pl, 0, 40.0f) == 0) {
            slideLocal[0] = 0.0f;
            slideLocal[1] = 0.0f;
            slideLocal[2] = -18.0f * Math_Cos(6.2831855f * (90.0f * (pl->motion[0].frame / 42.0f)) / 360.0f);
            Vec_Rotate(slideWorld, slideLocal, pl->rotMtx);
            pl->pos[0] += slideWorld[0];
            pl->pos[2] += slideWorld[2];
            if (pl->motionTime % 7 == 0) {
                PartFx_SpawnForPlayer(pl, 5, 9);
                PartFx_SpawnForPlayer(pl, 8, 9);
            }
        } else {
            pl->step++;
        }
        break;
    case 2:
        if (Motion_ReachedFrame(pl, 0, 44.0f) != 0) {
            pl->stateVar0++;
            if (variant == 1 || (variant == 0 && pl->stateVar0 >= 2)) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_ROAR_RECOVER, 2, 0);
            }
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 12, 0);
        }
        break;
    }
}

/*
 * The wind pressure: the player braces in a looping motion. variant 0
 * PL_REACT_WIND (4 loops), 1 PL_REACT_WIND_SHORT (one); stateVar0 counts
 * them. Then an ending motion and idle.
 */
void Player_StateWind(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar0 = 0;
        if (variant == 0) {
            pl->stateVar1 = 4;
        } else {
            pl->stateVar1 = 1;
        }
        Player_SetMotionPair(pl, PL_MOT_BRACE, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->timer = 0;
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, pl->motion[0].frameCount) != 0) {
            pl->stateVar0++;
            if ((s32) pl->stateVar0 >= (s32) pl->stateVar1) {
                pl->step++;
                Player_SetMotionPair(pl, PL_MOT_BRACE_END, 2, 0);
                break;
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 12, 0);
        }
        break;
    }
}

/*
 * PL_REACT_FLINCH_CARRYING and PL_REACT_FLINCH_CARRYING2: a flinch in place
 * that keeps the carried item (Player_RunReactState drops it in every other
 * react state). Then idle.
 */
void Player_StateFlinchCarrying(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        if (pl->weaponOut != 0) {
            Player_SetMotionPair(pl, PL_MOT_FLINCH_DRAWN, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_FLINCH, 0, 0);
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * Ends the stun build-up (pl->status5Buildup and its delay) and blocks the
 * item button for 30 frames (itemUseDelay).
 */
static void Player_ClearStun(Player *pl)
{
    pl->status5Delay = 0;
    pl->status5Buildup = 0;
    pl->itemUseDelay = 30;
}

/*
 * PL_REACT_STUN: dazed, with the stun effect, in 5 stages (stateVar1 counts
 * down). A stage ends after 300 frames, or sooner once the presses counted in
 * stateVar0 reach gStunMashNeed[stage] (doubled with skill 9). After the last
 * stage it clears the stun and returns to idle.
 */
void Player_StateStun(Player *pl, s32 unused)
{
    s8 need;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateMotionTime(pl, 0);
        pl->weaponOut = 0;
        pl->noFlinchTimer = 0;
        Player_SetMotionPair(pl, PL_MOT_STAGGER, 4, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->timer = 300;
        pl->stateVar0 = 0;
        pl->stateVar1 = 5;
        pl->status5Delay = 0;
        pl->status5Buildup = 0;
        Player_ClearMash(pl);
        pl->targetYaw = pl->rot[1];
        StunStars_CreatePlayer(pl, 0);
        break;
    case 1:
        if (--pl->timer <= 0) {
            pl->stateVar0 = 0;
            pl->stateVar1--;
            pl->timer = 300;
        } else if ((pl->input.pressed & PL_ANY_BUTTON) || (pl->input.stickDirPressed & (PAD_DPAD | PL_RSTICK_ANY))) {
            pl->stateVar0++;
            need = gStunMashNeed[pl->stateVar1];
            if (Player_HasSkill(pl, SKILL_STATUS5_DOUBLE) == 1) {
                need *= 2;
            }
            if (pl->stateVar0 >= need) {
                pl->stateVar0 = 0;
                pl->stateVar1--;
                pl->timer = 300;
            }
        }
        if (pl->stateVar1 == 0) {
            Player_ClearStun(pl);
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_REACT_14: one motion with the weapon sheathed, then idle (no entry
 * seen).
 */
void Player_StateReact14(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_REACT_STATUS2: falls, then lies for 240 frames (halved with skill 4,
 * doubled with skill 6) regaining stamina; each press takes a frame off (once
 * per 3 frames). Then gets up and returns to idle. Entry clears the status 2
 * build-up, the stun and the mash status.
 */
void Player_StateStatus2(Player *pl, s32 variant)
{
    f32 effectOffset[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->weaponOut = 0;
        pl->status2Buildup = 0;
        Player_ClearStun(pl);
        Player_ClearMash(pl);
        Player_SetMotionPair(pl, PL_MOT_FALL_ASLEEP, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->timer = 240;
            if (Player_HasSkill(pl, SKILL_STATUS2_HALF) == 1) {
                pl->timer /= 2;
            }
            if (Player_HasSkill(pl, SKILL_STATUS2_DOUBLE) == 1) {
                pl->timer *= 2;
            }
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 0, 154);
        }
        break;
    case 2:
        if (pl->stateVar1 > 0) {
            pl->stateVar1--;
        }
        if (((pl->input.pressed & PL_ANY_BUTTON) || (pl->input.stickDirPressed & (PAD_DPAD | PL_RSTICK_ANY))) && pl->stateVar1 == 0) {
            pl->stateVar1 = 3;
            pl->timer--;
        }
        if (--pl->timer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
            break;
        }
        effectOffset[0] = 0.0f;
        effectOffset[1] = 20.0f;
        effectOffset[2] = 0.0f;
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 20, effectOffset, 0.8f);
        }
        Player_AddStamina(pl, 2);
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * The status 4 fall. variant 0 PL_REACT_STATUS4 (falls, unhittable, then
 * lies), 1 PL_REACT_STATUS4_HIT (hit while down: lies at once and keeps
 * recoverTimer). Lies for recoverTimer frames (150, halved with skill 1,
 * doubled with skill 3); each press takes a frame off. Then gets up,
 * unhittable, and returns to idle.
 */
void Player_StateStatus4(Player *pl, s32 variant)
{

    pl->stateVar0++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->stateVar1 = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->weaponOut = 0;
        pl->status4Buildup = 0;
        pl->status2Buildup = 0;
        Player_ClearStun(pl);
        Player_ClearMash(pl);
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_STATUS4_COLLAPSE, 4, 0);
            pl->motionTime = 0;
            pl->stateVar0 = 0;
            Effect06_SpawnAtBone(pl, 5, 0, 2, 0.75f);
            pl->recoverTimer = 150;
            if (Player_HasSkill(pl, SKILL_STATUS4_HALF) == 1) {
                pl->recoverTimer /= 2;
            }
            if (Player_HasSkill(pl, SKILL_STATUS4_DOUBLE) == 1) {
                pl->recoverTimer *= 2;
            }
            pl->noHitTimer = 2;
            break;
        }
        pl->step = 2;
        if (pl->recoverTimer <= 0) {
            pl->recoverTimer = 0;
        }
        Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 2, 150);
        break;
    case 1:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 2, 150);
            pl->noHitTimer = 0;
        }
        if (pl->step < 3 && !(pl->stateVar0 & 0x1F)) {
            Effect06_SpawnAtBone(pl, 5, 0, 2, 0.75f);
        }
        break;
    case 2:
        if (pl->stateVar1 > 0) {
            pl->stateVar1--;
        }
        if (((pl->input.pressed & PL_ANY_BUTTON) || (pl->input.stickDirPressed & (PAD_DPAD | PL_RSTICK_ANY))) && pl->stateVar1 == 0) {
            pl->stateVar1 = 3;
            pl->recoverTimer--;
        }
        if (--pl->recoverTimer <= 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
            pl->noHitTimer = 2;
            break;
        }
        if (pl->motionId[0] == 210 && Motion_PassedFrame(pl, 0, 170.0f) != 0 && !(Rand_Next(1) & 0x3F)) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP_STIR, 0, 0);
        } else if (pl->motionId[0] == 223 && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 4, 170);
        }
        if (pl->step < 3 && !(pl->stateVar0 & 0x1F)) {
            Effect06_SpawnAtBone(pl, 5, 0, 2, 0.75f);
        }
        break;
    case 3:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * PL_REACT_GUARD_DRAW (lance, sword and shield, gunlance): draws the weapon
 * straight into the guard, then PL_REACT_GUARD_HOLD.
 */
void Player_StateGuardDraw(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_WPN_GUARD_DRAW, 2, 1);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, PLF_GUARD);
        pl->weaponOut = 1;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_GUARD_HOLD, 0);
        }
        break;
    }
}

/*
 * The mash status's input for the local player. caller: 0 from
 * Player_StateMashIdle, 1 from the walk and the run. A press (once per 2
 * frames, mashTimer) takes 10 off pl->mashLeft (doubled with SKILL_MASH_FAST,
 * halved with SKILL_MASH_SLOW); at 0 the mash ends (PL_STATUS_ICON_MASH cleared)
 * with PL_REACT_MASH_FREE. Otherwise the left stick turns the player and
 * requests PL_REACT_MASH_RUN (PAD_R2) or PL_REACT_MASH_WALK; released, it
 * goes back to PL_REACT_MASH_IDLE (for caller 1 once pl->timer is 0).
 */
void Player_UpdateMashEscape(Player *pl, s32 caller)
{
    s16 n;

    if (Player_IsLocal(pl) != 0) {
        if (pl->mashTimer == 0) {
            if ((pl->input.pressed & PL_ANY_BUTTON) || (pl->input.stickDirPressed & (PAD_DPAD | PL_RSTICK_ANY))) {
                if (pl->mashLeft > 0) {
                    n = 10;
                    if (Player_HasSkill(pl, SKILL_MASH_FAST) == 1) {
                        n *= 2;
                    }
                    if (Player_HasSkill(pl, SKILL_MASH_SLOW) == 1) {
                        n /= 2;
                    }
                    pl->mashLeft -= n;
                    if (pl->mashLeft < 0) {
                        pl->mashLeft = 0;
                    }
                    pl->mashTimer = 1;
                }
            }
        } else {
            pl->mashTimer--;
        }
        if (pl->mashLeft <= 0) {
            Player_ClearMash(pl);
            pl->statusIcons &= 0xFFBF;
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_FREE, 0);
            return;
        }
        if (pl->input.stickTilt >= 40) {
            Player_FaceStick(pl);
            if ((pl->input.held & PAD_R2) && pl->dashDelay == 0) {
                if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_MASH_RUN) == 0) {
                    Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_RUN, 0);
                }
            } else if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_MASH_WALK) == 0 && pl->dashDelay == 0) {
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_WALK, 0);
            }
        } else if (Player_TestFlag(pl, 0x2001200) != 0) {
            if ((s16) caller != 1 || pl->timer == 0) {
                Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_IDLE, 0);
            }
        }
    }
}

/* Ends the mash status: clears pl->mashLeft, mashBuildup and mashTimer. */
static void Player_ClearMash(Player *pl)
{
    pl->mashLeft = 0;
    pl->mashBuildup = 0;
    pl->mashTimer = 0;
}

/*
 * Standing in the mash status. variant 0 PL_REACT_MASH_START (the status
 * begins: clears the status 2 and 4 build-ups, the stun and the flinch
 * timers, mashLeft 900; presses count after 15 frames), 1 PL_REACT_MASH_IDLE
 * (back from walking, running or landing). The local player runs
 * Player_UpdateMashEscape each frame.
 */
void Player_StateMashIdle(Player *pl, s32 variant)
{
    if (pl->motionId[0] != 51) {
        Player_SetMotionPair(pl, PL_MOT_CARRY_IDLE, 4, 0);
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 0;
        if (variant == 0) {
            pl->status4Buildup = 0;
            pl->status2Buildup = 0;
            Player_ClearStun(pl);
            Player_ClearMash(pl);
            pl->mashLeft = 900;
            pl->noFlinchTimer = 0;
            pl->noFlinchItemTimer = 0;
            pl->timer = 15;
        } else {
            pl->timer = 30;
        }
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->weaponOut = 0;
    case 1:
        if (--pl->timer <= 0) {
            pl->step++;
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            Player_UpdateMashEscape(pl, 0);
        }
        break;
    }
}

/*
 * PL_REACT_MASH_FREE: breaks free of the mash status, with the status effect
 * object (MashDebris_Create type 0); then idle.
 */
void Player_StateMashFree(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 0;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 6, 0);
        MashDebris_Create(0, (s16 *) pl);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * Shaken by a tremor, unable to act. variant 0 PL_REACT_QUAKE (40 frames), 1
 * PL_REACT_QUAKE_LONG (80); then idle.
 */
void Player_StateQuake(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_STAGGER, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 1);
        if (variant == 0) {
            pl->timer = 40;
            break;
        }
        pl->timer = 80;
        break;
    case 1:
        if (--pl->timer <= 0) {
            Player_ReturnToIdle(pl, 0, 12, 0);
        }
        break;
    }
}

/*
 * PL_REACT_MASH_WALK: walks in the mash status, running
 * Player_UpdateMashEscape(pl, 1) each frame, with the status effect object
 * every tenth frame.
 */
void Player_StateMashWalk(Player *pl, s32 unused)
{
    f32 strideLocal[3];
    f32 strideWorld[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 20;
        pl->dashDelay = 30;
        Player_SetMotionPair(pl, PL_MOT_CARRY_WALK, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        break;
    case 1:
        strideLocal[0] = 0.0f;
        strideLocal[1] = 0.0f;
        strideLocal[2] = 2.0f;
        Vec_Rotate(strideWorld, strideLocal, pl->rotMtx);
        pl->pos[0] += strideWorld[0];
        pl->pos[2] += strideWorld[2];
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateMashEscape(pl, 1);
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        MashDebris_Create(1, (s16 *) pl);
    }
}

/*
 * PL_REACT_MASH_RUN: runs in the mash status; a drop ahead requests
 * PL_REACT_MASH_JUMP_OFF. Otherwise as Player_StateMashWalk.
 */
void Player_StateMashRun(Player *pl, s32 unused)
{
    f32 strideLocal[3];
    f32 strideWorld[3];

    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->gender == 0) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_RUN, 4, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_RUN_ALT, 4, 0);
        }
        pl->timer = 20;
        pl->dashDelay = 30;
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x2000200);
        break;
    case 1:
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f) != 0) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_JUMP_OFF, 0);
            return;
        }
        strideLocal[0] = 0.0f;
        strideLocal[1] = 0.0f;
        strideLocal[2] = 5.0f;
        Vec_Rotate(strideWorld, strideLocal, pl->rotMtx);
        pl->pos[0] += strideWorld[0];
        pl->pos[2] += strideWorld[2];
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateMashEscape(pl, 1);
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        MashDebris_Create(1, (s16 *) pl);
    }
}

/*
 * The mash status in the air. variant 0 PL_REACT_MASH_JUMP_OFF (a hop off a
 * ledge), 1 PL_REACT_MASH_FALL. pl->timer counts the time aloft: landing
 * after 50 breaks the mash (Player_ClearMash) with PL_ACT_LAND_HARD, after 30
 * PL_REACT_MASH_LAND_HARD, else PL_REACT_MASH_LAND.
 */
void Player_StateMashAirborne(Player *pl, s32 variant)
{
    f32 velLocal[3];
    f32 moveWorld[3];

    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = PL_POSTURE_AIRBORNE;
        switch (variant) {
        case 0:
        default:
            Player_SetMotionPair(pl, PL_MOT_CARRY_AIRBORNE, 0, 0);
            pl->vel[0] = 0.0f;
            pl->vel[2] = 20.0f;
            pl->vel[1] = 18.0f;
            pl->accel[0] = 0.0f;
            pl->accel[2] = -(pl->vel[2] / 80.0f);
            Player_StartJump(pl, 19);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_CARRY_AIRBORNE, 8, 32);
            pl->rideState = 0;
            pl->vel[0] = 0.0f;
            pl->vel[2] = 8.0f;
            pl->vel[1] = -10.0f;
            pl->accel[2] = 0.0f;
            pl->accel[0] = 0.0f;
            pl->accel[1] = -8.0f / 11.0f;
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->timer = 0;
        pl->stateVar1 = 0;
        break;
    case 1:
        pl->vel[0] += pl->accel[0];
        pl->vel[1] += pl->accel[1];
        pl->vel[2] += pl->accel[2];
        if (pl->vel[0] < 0.0f) {
            pl->vel[0] = 0.0f;
        }
        if (pl->vel[2] < 0.0f) {
            pl->vel[2] = 0.0f;
        }
        velLocal[0] = pl->vel[0];
        velLocal[1] = pl->vel[1];
        velLocal[2] = pl->vel[2];
        Vec_Rotate(moveWorld, velLocal, pl->rotMtx);
        pl->pos[0] += moveWorld[0];
        pl->pos[1] += moveWorld[1];
        pl->pos[2] += moveWorld[2];
        if (pl->vel[1] < 0.0f) {
            pl->timer++;
            if (!(pl->floorY < pl->pos[1])) {
                pl->pos[1] = pl->floorY;
                pl->action = PL_POSTURE_STANDING;
                pl->rideState = 0;
                if (pl->timer >= 50) {
                    MashDebris_Create(0, (s16 *) pl);
                    Player_ClearMash(pl);
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                } else if (pl->timer >= 30) {
                    Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_LAND_HARD, PL_SET_NO_SYNC);
                } else {
                    Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_LAND, PL_SET_NO_SYNC);
                }
            }
        }
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        MashDebris_Create(1, (s16 *) pl);
    }
}

/*
 * The landings in the mash status: variant 0 PL_REACT_MASH_LAND_HARD, 1
 * PL_REACT_MASH_LAND (the motion). Then PL_REACT_MASH_IDLE.
 */
void Player_StateMashLand(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_LAND_HARD, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_LAND, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_REACT, PL_REACT_MASH_IDLE, PL_SET_NO_SYNC);
        }
        break;
    }
    if (((s32) gScene.frame % 10) == 0) {
        MashDebris_Create(1, (s16 *) pl);
    }
}

/*
 * PL_REACT_INHALED: pulled toward a Yama Tsukami. Looks for a visible
 * EM_KIND_YAMA_TSUKAMI in the player's area and launches the player toward
 * its bone 36 (with none, thrown back as PL_REACT_TOSSED_BACK). Lands,
 * turns half a turn, gets up and returns to idle.
 */
void Player_StateInhaled(Player *pl, s32 variant)
{
    f32 kickLocal[3];
    f32 kickWorld[3];
    f32 pullPos[3];
    s32 ang[3];
    f32 mtx[16];
    Em *inhaler;
    s16 i;
    s16 found;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->demonMode = 0;
        Player_SetMotionPair(pl, PL_MOT_TOSSED_BACK_LAUNCH, 0, 0);
        pl->noHitTimer = 10;
        Em_GetBonePos(pl, 2, kickLocal);
        if (pl->pos[1] < kickLocal[1]) {
            pl->pos[1] = kickLocal[1];
        }
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 1);
        pl->rideState = 0;
        Player_ClearAccel(pl);
        found = 0;
        inhaler = gEms;
        for (i = 0; i < 20; i++, inhaler++) {
            if (inhaler->active && inhaler->visible && (Player_InCurrentArea(inhaler) & 0xFF) && inhaler->type == 58) {
                found = 1;
                Em_BonePos(inhaler, 36, pullPos);
                pl->rot[1] = Vec_Heading(pl->pos, pullPos) + ANGLE_TURN / 4;
                pl->targetYaw = pl->rot[1];
            }
        }
        ang[0] = 0;
        ang[1] = pl->rot[1];
        ang[2] = 0;
        Mtx_FromAnglesXYZ(ang, mtx);
        if (found == 0) {
            kickLocal[0] = 0.0f;
            kickLocal[1] = 0.0f;
            kickLocal[2] = -7.0f;
            Vec_Rotate(kickWorld, kickLocal, mtx);
            pl->vel[0] = 2.0f * kickWorld[0];
            pl->vel[2] = 2.0f * kickWorld[2];
            pl->vel[1] = 20.0f;
            pl->accel[1] = -8.0f / 11.0f;
        } else {
            kickWorld[0] = (pullPos[0] - pl->pos[0]) / 50.0f;
            kickWorld[1] = (pullPos[1] - pl->pos[1]) / 50.0f;
            kickWorld[2] = (pullPos[2] - pl->pos[2]) / 50.0f;
            pl->vel[0] = kickWorld[0];
            pl->vel[2] = kickWorld[2];
            if (kickWorld[1] <= 15.0f) {
                kickWorld[1] = 15.0f;
            }
            pl->vel[1] = 2.0f * kickWorld[1];
            pl->accel[1] = -8.0f / 11.0f;
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0 && (pl->motionId[0] == 213 || pl->motionId[0] == 219)) {
            if (variant == 0) {
                Player_SetMotionPair(pl, PL_MOT_FLUNG_BACK_AIR, 0, 0);
            } else {
                Player_SetMotionPair(pl, PL_MOT_FLUNG_FORWARD_AIR, 0, 0);
            }
        }
        Player_MoveAccel(pl);
        if (pl->vel[1] <= 0.0f && pl->pos[1] <= 63.0f + pl->floorY) {
            pl->noHitTimer = 2;
            pl->step++;
            Sound_PlayPlayerSe(pl, 66, 0, pl->pos, 1, 0);
            Sound_PlayArmorRattle(pl, EQUIP_BODY);
            Player_SetMotionPair(pl, PL_MOT_TOSSED_LAND, 0, 0);
            pl->pos[1] = pl->floorY;
            pl->action = PL_POSTURE_DOWN;
        }
        break;
    case 2:
        pl->noHitTimer = 2;
        if (pl->motion[0].playing == 0) {
            pl->step++;
            pl->rot[1] = (pl->rot[1] + ANGLE_TURN / 2) & 0xFFFF;
            pl->targetYaw = pl->rot[1];
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
            pl->noHitTimer = 6;
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_REACT. First it leaves
 * the aim mode, closes the box and trade menus, clears noFlinchTimer and
 * PLF_FISHING, and drops a carried item (ItemProp_CreateAtPlayer) in every
 * state but the carrying flinches. States sharing a handler pass a variant; a
 * state with no case returns to idle.
 */
void Player_RunReactState(Player *pl)
{
    u8 state;
    u8 carry;

    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    pl->noFlinchTimer = 0;
    Player_ClearFlag(pl, PLF_FISHING);
    state = pl->state;
    if ((state != 17) && (state != 18)) {
        carry = pl->carry;
        if (carry & PL_CARRY_HOLD_MASK) {
            pl->carry = carry & PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
    }
    switch (pl->state) {
    case PL_REACT_KNOCKDOWN_FRONT:
        Player_StateKnockdownFront(pl);
        break;
    case PL_REACT_GUARD_STAGGER:
        Player_StateGuardRecoil(pl, 0);
        break;
    case PL_REACT_BLOWN_BACK:
        Player_StateBlownBack(pl);
        break;
    case PL_REACT_GUARD_START:
        Player_StateGuard(pl, 0);
        break;
    case PL_REACT_ROAR_STRONG:
        Player_StateRoarStrong(pl, 0);
        break;
    case PL_REACT_BLOWN_FORWARD:
        Player_StateBlownForward(pl);
        break;
    case PL_REACT_KNOCKDOWN_BACK:
        Player_StateKnockdownBack(pl);
        break;
    case PL_REACT_TOSSED_BACK:
        Player_StateTossed(pl, 0);
        break;
    case PL_REACT_GUARD_END:
        Player_StateGuardEnd(pl);
        break;
    case PL_REACT_FLINCH_FRONT:
        Player_StateFlinch(pl, 0);
        break;
    case PL_REACT_GUARD_KNOCKBACK:
        Player_StateGuardRecoil(pl, 1);
        break;
    case PL_REACT_GUARD_PUSH:
        Player_StateGuardPush(pl);
        break;
    case PL_REACT_GUARD_HOLD:
        Player_StateGuard(pl, 1);
        break;
    case PL_REACT_FLINCH_BACK:
        Player_StateFlinch(pl, 1);
        break;
    case PL_REACT_TOSSED_FORWARD:
        Player_StateTossed(pl, 1);
        break;
    case PL_REACT_ROAR:
        Player_StateRoar(pl, 0);
        break;
    case PL_REACT_WIND:
        Player_StateWind(pl, 0);
        break;
    case PL_REACT_FLINCH_CARRYING:
        Player_StateFlinchCarrying(pl, 0);
        break;
    case PL_REACT_FLINCH_CARRYING2:
        Player_StateFlinchCarrying(pl, 1);
        break;
    case PL_REACT_STUN:
        Player_StateStun(pl, 0);
        break;
    case PL_REACT_14:
        Player_StateReact14(pl, 0);
        break;
    case PL_REACT_STATUS2:
        Player_StateStatus2(pl, 0);
        break;
    case PL_REACT_STATUS4:
        Player_StateStatus4(pl, 0);
        break;
    case PL_REACT_GUARD_DRAW:
        Player_StateGuardDraw(pl, 0);
        break;
    case PL_REACT_STATUS4_HIT:
        Player_StateStatus4(pl, 1);
        break;
    case PL_REACT_WIND_SHORT:
        Player_StateWind(pl, 1);
        break;
    case PL_REACT_ROAR_SHORT:
        Player_StateRoar(pl, 1);
        break;
    case PL_REACT_MASH_START:
        Player_StateMashIdle(pl, 0);
        break;
    case PL_REACT_MASH_FREE:
        Player_StateMashFree(pl, 0);
        break;
    case PL_REACT_QUAKE:
        Player_StateQuake(pl, 0);
        break;
    case PL_REACT_MASH_IDLE:
        Player_StateMashIdle(pl, 1);
        break;
    case PL_REACT_MASH_WALK:
        Player_StateMashWalk(pl, 0);
        break;
    case PL_REACT_MASH_RUN:
        Player_StateMashRun(pl, 0);
        break;
    case PL_REACT_MASH_JUMP_OFF:
        Player_StateMashAirborne(pl, 0);
        break;
    case PL_REACT_MASH_LAND_HARD:
        Player_StateMashLand(pl, 0);
        break;
    case PL_REACT_MASH_LAND:
        Player_StateMashLand(pl, 1);
        break;
    case PL_REACT_MASH_FALL:
        Player_StateMashAirborne(pl, 1);
        break;
    case PL_REACT_QUAKE_LONG:
        Player_StateQuake(pl, 1);
        break;
    case PL_REACT_GUARD_HOLD_ALT:
        Player_StateGuard(pl, 2);
        break;
    case PL_REACT_INHALED:
        Player_StateInhaled(pl, 0);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * The faint, PL_GROUP_FAINT. variant 0 PL_FAINT, 1 PL_FAINT_FACE_UP, 2
 * PL_FAINT_FACE_DOWN (the lying motion). Step 0 counts the faint:
 * Quest_PlayerFainted for a hunter (FAINT_FAILED and FAINT_QUEST_OVER end in
 * a wait), Versus_MonsterDown in versus; pl->faints counts up and the notices
 * are posted. Then the player lies: at 600 frames the local player is sent to
 * the camp area (QSync_SendPlayer). In versus it revives after 450 frames
 * instead (PL_EVENT_REVIVE).
 */
void Player_StateFaint(Player *pl, s32 variant)
{
    s8 noPenalty;
    s8 result;
    u8 k;

    pl->noPushTimer = 10;
    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    Player_ClearFlag(pl, PLF_FISHING);
    if (pl->carry & PL_CARRY_HOLD_MASK) {
        pl->carry &= PL_CARRY_KIND_MASK;
        ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
    }
    switch (pl->step) {
    case 0:
        pl->step++;
        switch (variant) {
        case 0:
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 0, 0);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 0, 140);
            break;
        case 2:
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_DOWN, 0, 140);
            Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            break;
        }
        Vib_StartLocal(pl, 3);
        Player_UpdateMotionTime(pl, 0);
        Player_PlaySeAtSelf((u8 *) pl, 34);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, PLF_GUARD);
        pl->weaponOut = 0;
        switch (*(u8 *) &pl->stateRand) {
        default:
            noPenalty = 0;
            break;
        case 1:
            noPenalty = 1;
            break;
        }
        if (pl->side == 0) {
            result = Quest_PlayerFainted(pl, noPenalty);
            if (result != 0 && result != 3) {
                pl->step++;
            }
        } else {
            result = 2;
            Versus_MonsterDown((Em *) pl);
        }
        if (pl->faints <= 98) {
            pl->faints++;
        } else {
            pl->faints = 99;
        }
        if (Player_IsLocal(pl) == 0) {
            Notice_Push(NOTICE_KIND_PLAYER, 0, pl->index);
            if (result <= 1) {
                Quest_ShowValue();
            }
            if (gQuest.reward <= 0 && result == 1) {
                Notice_PushItem((char *)gRewardZeroText);
            }
        } else {
            CamReq_PlayerDown();
            Notice_PushItem((char *)gFaintedText);
            if (result <= 1) {
                Quest_ShowValue();
            }
            if (pl->step <= 1) {
                Quest_ResetMusicIfIdle();
                Bgm_PlayFaintMusic();
            } else {
                if (result == 1) {
                    Notice_PushItem((char *)gRewardZeroText2);
                }
                Notice_PushItem((char *)gNoMoreReviveText);
            }
        }
        break;
    case 1:
        if (pl->side != 0) {
            pl->step = 3;
            pl->timer = 0;
        }
        if (pl->motionId[0] == 210 && Motion_PassedFrame(pl, 0, 164.0f) != 0 && (Rand_Next(1) & 0x3F) == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP_STIR, 0, 0);
        } else if (pl->motionId[0] == 223 && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 4, 164);
        }
        if (Quest_PhaseIsOver(2) == 1) {
            pl->step++;
            break;
        }
        if (pl->motionTime == 300 && Player_IsLocal(pl) == 1) {
            switch (gScene.area) {
            case AREA_TOWER_0x7C:
            case AREA_TOWER_4:
            case AREA_TOWER_0x8A:
                Notice_Push(NOTICE_KIND_MESSAGE, 31, 0);
                break;
            default:
                Notice_Push(NOTICE_KIND_MESSAGE, 7, 0);
                break;
            }
        }
        if (pl->motionTime == 600 && Player_IsLocal(pl) == 1) {
            switch (gScene.area) {
            case AREA_TOWER_0x7C:
                k = 84;
                pl->areaMoveState = PL_AREA_MOVE_RESPAWN;
                break;
            case AREA_TOWER_4:
                k = 125;
                pl->areaMoveState = PL_AREA_MOVE_RESPAWN;
                break;
            case AREA_TOWER_0x8A:
                k = 129;
                pl->areaMoveState = PL_AREA_MOVE_RESPAWN;
                break;
            default:
                k = gScene.campArea;
                pl->areaMoveState = PL_AREA_MOVE_CART;
                break;
            }
            pl->startPos[0] = gStartPos[k][0];
            pl->startPos[1] = gStartPos[k][1];
            pl->startPos[2] = gStartPos[k][2];
            pl->startAngle = gStartAngle[k];
            pl->nextArea = k;
            QSync_SendPlayer(pl, QSYNC_PL_FAINT_TO_CAMP, 0);
        }
        break;
    case 2:
        break;
    case 3:
        if (pl->motionId[0] == 210 && Motion_PassedFrame(pl, 0, 164.0f) != 0 && (Rand_Next(1) & 0x3F) == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP_STIR, 0, 0);
        } else if (pl->motionId[0] == 223 && pl->motion[0].playing == 0) {
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 4, 164);
        }
        if (Player_IsLocal(pl) == 1) {
            if (++pl->timer >= 450) {
                CamReq_Cancel();
                pl->hpMax = (s16)Player_GetHpSkillBonus(pl) + 100;
                if (pl->hpMax > 150) {
                    pl->hpMax = 150;
                }
                pl->hp = pl->hpMax;
                pl->hpRecoverable = pl->hpMax;
                pl->hpMaxBase = pl->hpMax;
                pl->staminaMax = 300;
                pl->staminaTimer = 10800;
                pl->stamina = pl->staminaMax;
                Player_ClearBuffs(pl);
                pl->weaponSeMute = 0;
                Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_REVIVE, PL_SET_NO_SYNC);
            }
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_FAINT: the three faints
 * go to Player_StateFaint (variants 0, 1, 2) after closing the box and trade
 * menus.
 */
void Player_RunFaintState(Player *pl)
{
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    switch (pl->state) {
    case PL_FAINT:
        Player_StateFaint(pl, 0);
        break;
    case PL_FAINT_FACE_UP:
        Player_StateFaint(pl, 1);
        break;
    case PL_FAINT_FACE_DOWN:
        Player_StateFaint(pl, 2);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * PL_EVENT_CART: the cart carries the fainted player away. Step 0 makes the
 * cart, an EM_KIND_FELYNE_CART monster at the player (pl->cart; with none the
 * state returns to idle); then the player rides at CART_BONE_RIDER and
 * requests PL_EVENT_CART_DROP when it drops it. The local camera takes mode
 * 1.
 */
void Player_StateCart(Player *pl, s32 variant)
{
    Em *cartEm;
    s16 i;
    s16 kind;

    pl->noPushTimer = 10;
    pl->noHitTimer = 10;
    pl->visible = 1;
    switch (pl->step) {
    case 0:
        pl->step++;
        kind = -1;
        pl->stateVar0 = 0;
        for (i = 0; i < 4; i++) {
            if (gScene.emKind[i] == EM_KIND_FELYNE_CART && (cartEm = EmSlot_Alloc()) != NULL) {
                cartEm->modelSlot = i;
                cartEm->type = gScene.emKind[i];
                cartEm->area = gScene.area;
                cartEm->variant = 0;
                cartEm->variant = 255;
                cartEm->cartPlayer = pl->index;
                GmEm_Task(cartEm);
                cartEm->posX = pl->pos[0];
                cartEm->posY = pl->pos[1];
                cartEm->posZ = pl->pos[2];
                cartEm->rotY = pl->rot[1];
                pl->cart = cartEm;
                kind = gScene.emKind[i];
                break;
            }
        }
        if (kind != EM_KIND_FELYNE_CART) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else {
            pl->stateVar1 = 0;
            Player_SetMotionPair(pl, PL_MOT_CART_RIDE, 0, 0);
        }
        if (Player_IsLocal(pl) == 1) {
            CamReq_Cart();
        }
        break;
    case 1:
        cartEm = pl->cart;
        if (cartEm == NULL) {
            Player_ReturnToIdle(pl, 0, 4, 0);
            break;
        }
        Em_BonePos(cartEm, CART_BONE_RIDER, &pl->pos[0]);
        if (cartEm->motionId[0] == 1003) {
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_CART_DROP, PL_SET_NO_SYNC);
        }
        pl->rot[1] = cartEm->rotY;
        pl->targetYaw = pl->rot[1];
        break;
    }
}

/*
 * PL_EVENT_CART_DROP: the cart drops the player at the camp; the player sinks
 * to the floor, lies, gets up and returns to idle.
 */
void Player_StateCartDrop(Player *pl, s32 unused)
{
    f32 floorY;

    pl->noPushTimer = 10;
    pl->noHitTimer = 10;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_CART_DROPPED, 0, 0);
        pl->action = PL_POSTURE_AIRBORNE;
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 28.0f) != 0) {
            pl->pos[1] -= 4.0f;
        }
        floorY = pl->floorY;
        if (pl->pos[1] <= floorY) {
            pl->pos[1] = floorY;
        }
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_LIE_FACE_DOWN, 0, 70);
            pl->action = PL_POSTURE_STANDING;
        }
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
        }
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * PL_EVENT_QUEST_START: the pose at the quest's start (sets questStartDone), then
 * idle.
 */
void Player_StateQuestStart(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->questStartDone = 1;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_ITEM_POSE, 4, 0);
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_EVENT_QUEST_END: the quest is cleared or failed: sheathes, plays one
 * motion, drops a carried item and sets the local camera's mode 10; then
 * waits until the scene changes.
 */
void Player_StateQuestEnd(Player *pl, s32 unused)
{
    u8 carry;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, PL_MOT_GESTURE_SIT, 6, 0);
        pl->motionTime = 0;
        carry = pl->carry;
        if (carry & PL_CARRY_HOLD_MASK) {
            pl->carry = carry & PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        if (Player_IsLocal(pl) == 1) {
            CamReq_PlayerDown();
        }
        
    case 1:
        break;
    }
}

/*
 * PL_EVENT_REVIVE: the versus revive: gets up, then 150 hit-free frames and
 * idle.
 */
void Player_StateRevive(Player *pl, s32 unused)
{
    pl->noHitTimer = 4;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->weaponOut = 0;
        if (pl->motionId[0] == 209) {
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_DOWN, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
        }
        pl->motionTime = 0;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->noHitTimer = 150;
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_EVENT_LEAVE: collapses and vanishes: after 300 frames pl->active and
 * pl->visible are cleared.
 */
void Player_StateLeave(Player *pl, s32 unused)
{
    u8 carry;

    pl->noHitTimer = 10;
    pl->noPushTimer = 10;
    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    Player_ClearFlag(pl, PLF_FISHING);
    pl->pos[1] = pl->floorY;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_PlaySeAtSelf((u8 *) pl, 34);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, PLF_GUARD);
        pl->weaponOut = 0;
        pl->timer = 300;
        carry = pl->carry;
        if (carry & PL_CARRY_HOLD_MASK) {
            pl->carry = carry & PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        if (Game_IsOnline() != 1) {
            return;
        }
        return;
    case 1:
        if (--pl->timer <= 0) {
            pl->step++;
            return;
        }
        break;
    case 2:
        pl->active = 0;
        pl->visible = 0;
        break;
    }
}

/*
 * PL_EVENT_ABORT: the quest is abandoned: two motions, then after 300 frames
 * pl->active and pl->visible are cleared.
 */
void Player_StateQuestAbort(Player *pl, s32 unused)
{
    u8 carry;

    pl->noHitTimer = 10;
    pl->noPushTimer = 10;
    Player_ResetAim(pl);
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->deliveryOpen = 0;
    pl->tradeMsgTimer = 0;
    Player_ClearFlag(pl, PLF_FISHING);
    pl->pos[1] = pl->floorY;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_START, 0, 0);
        Player_ClearFlag(pl, 2);
        Player_ClearFlag(pl, PLF_GUARD);
        pl->weaponOut = 0;
        pl->timer = 300;
        carry = pl->carry;
        if (carry & PL_CARRY_HOLD_MASK) {
            pl->carry = carry & PL_CARRY_KIND_MASK;
            ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_EXHAUSTED_PANT, 4, 144);
        }
        break;
    case 2:
        if (--pl->timer <= 0) {
            pl->step++;
        }
        break;
    case 3:
        pl->active = 0;
        pl->visible = 0;
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_EVENT (the cart, the
 * start, the end, the revive, leaving and abandoning), after closing the box
 * and trade menus.
 */
void Player_RunEventState(Player *pl)
{
    pl->supplyBoxOpen = 0;
    pl->deliveryMenu = 0;
    pl->tradeMsgTimer = 0;
    pl->deliveryOpen = 0;
    switch (pl->state) {
    case PL_EVENT_CART:
        Player_StateCart(pl, 0);
        break;
    case PL_EVENT_CART_DROP:
        Player_StateCartDrop(pl, 0);
        break;
    case PL_EVENT_QUEST_START:
        Player_StateQuestStart(pl, 0);
        break;
    case PL_EVENT_QUEST_END:
        Player_StateQuestEnd(pl, 0);
        break;
    case PL_EVENT_REVIVE:
        Player_StateRevive(pl, 0);
        break;
    case PL_EVENT_LEAVE:
        Player_StateLeave(pl, 0);
        break;
    case PL_EVENT_ABORT:
        Player_StateQuestAbort(pl, 0);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * Keeps pl->carry current while carrying: for the local player (not while
 * delivering or handing over) its high nibble is the pouch's carried item
 * (Item_CarryKind of Pouch_FindCarryItem); the low nibble is set to
 * PL_CARRY_HOLD, the mark the react and event states test when they drop the
 * item.
 */
void Player_UpdateCarryKind(Player *pl)
{
    u16 slot;

    if (Player_IsLocal(pl) == 1 && Player_IsState(pl, PL_GROUP_CARRY, PL_CARRY_DELIVER) == 0 && Player_IsState(pl, PL_GROUP_CARRY, PL_CARRY_HAND_OVER) == 0) {
        slot = Pouch_FindCarryItem(pl);
        if (slot != 0xFFFF) {
            pl->carry = ((Item_CarryKind(slot) & 0xFF) - 1) * 16;
        } else {
            pl->carry = 0;
        }
    }
    pl->carry = (pl->carry & PL_CARRY_KIND_MASK) | PL_CARRY_HOLD;
}

/*
 * The carry states' input for the local player. caller: 0 from
 * Player_StateCarryIdle, 1 from the walk and the run (they stop only once
 * pl->timer is 0). The quest phases 5 to 8 request the end, abort and leave
 * events; no carried item left requests PL_CARRY_DROP. The stick walks or
 * runs (PAD_R2); PAD_CROSS rolls or crouches, a flick draws (PL_ACT_DRAW),
 * PAD_SQUARE drops the item, and PAD_CIRCLE delivers it at the delivery box
 * (an action spot of kind 7, PL_CARRY_DELIVER), fires the cannon with it at
 * kind 8 (PL_ACT_CANNON_FIRE), or hands it to a trader (EM_KIND_TRADER,
 * PL_CARRY_HAND_OVER).
 */
void Player_UpdateCarryInput(Player *pl, s32 caller)
{
    f32 buf[3];
    ItemStack wanted;
    u16 ang;
    u8 k;
    s16 i;

    if (Player_IsLocal(pl) != 0) {
        switch (gScene.questPhase) {
        case 6:
        case 5:
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_QUEST_END, 0);
            break;
        case 7:
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_ABORT, 0);
            break;
        case 8:
            Player_RequestState(pl, PL_GROUP_EVENT, PL_EVENT_LEAVE, 0);
            break;
        default:
            if ((u16) Pouch_FindCarryItem(pl) == 0xFFFF) {
                Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DROP, 0);
                break;
            }
            if (pl->input.stickTilt >= 40) {
                Player_FaceStick(pl);
                if ((pl->input.held & PAD_R2) && pl->dashDelay == 0) {
                    if (Player_IsState(pl, PL_GROUP_CARRY, PL_CARRY_RUN) == 0) {
                        Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_RUN, 0);
                    }
                } else if (Player_IsState(pl, PL_GROUP_CARRY, PL_CARRY_WALK) == 0 && pl->dashDelay == 0) {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_WALK, 0);
                }
            } else if (Player_TestFlag(pl, 0x2001200) != 0) {
                if ((s16) caller != 1 || pl->timer == 0) {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_IDLE, 0);
                }
            }
            if (Quest_PhaseIsOver(1) == 1) {
                break;
            }
            if (pl->input.pressed & PAD_CROSS) {
                if (pl->input.stickTilt >= 85) {
                    if (pl->stamina >= 75) {
                        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_ROLL, PL_SET_SNAP_POS);
                    }
                } else {
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CROUCH, 0);
                }
                return;
            }
            if (pl->input.stickDirPressed & PL_RSTICK_ANY) {
                Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_DRAW, 0);
                break;
            }
            if ((pl->input.pressed & PAD_SQUARE) && pl->itemSelect == 0) {
                Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DROP, 0);
            }
            Player_HoldSpecialInput(pl, 2);
            if (pl->input.pressed & PAD_CIRCLE) {
                switch (Player_FindActionSpot((Player *)pl, (f32 *)buf, &ang, (u8 *)&k)) {
                case 7:
                    if (Quest_PhaseIsOver(0) == 1) {
                        break;
                    }
                    for (i = 0; i < 3; i++) {
                        if (Quest_GetDeliveryTarget(pl, i, &wanted) == 2 && wanted.item == (u16) Pouch_FindCarryItem(pl)) {
                            Quest_DeliverItem(pl, wanted.item, 1);
                        }
                    }
                    if ((u16) Pouch_FindCarryItem(pl) == 0xFFFF) {
                        Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DELIVER, 0);
                    }
                    break;
                case 8:
                    if ((u16) Pouch_FindCarryItem(pl) == 668) {
                        pl->carry &= PL_CARRY_KIND_MASK;
                        Pouch_Add(pl, 668, -10);
                        pl->targetYaw = ang;
                        pl->stateRand = k;
                        Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_CANNON_FIRE, PL_SET_KEEP_RAND);
                    }
                    break;
                default:
                    if (Player_NearTrader(pl) & 0xFF) {
                        Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_HAND_OVER, 0);
                        return;
                    }
                    break;
                }
            }
            break;
        }
    }
}

/*
 * Standing with the item. variant 0 PL_CARRY_IDLE, 1 PL_CARRY_PICKED_UP
 * (input held off 30 frames). Runs Player_UpdateCarryKind and
 * Player_UpdateCarryInput(pl, 0).
 */
void Player_StateCarryIdle(Player *pl, s32 variant)
{
    s32 timer;

    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 30;
        pl->dashDelay = 0;
        
    case 1:
        if (pl->motionId[0] != 51) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_IDLE, 4, 0);
        }
        if (variant == 1) {
            timer = pl->timer;
            if (timer <= 0) {
                Player_UpdateCarryInput(pl, 0);
                break;
            }
            pl->timer = timer - 1;
            break;
        }
        Player_UpdateCarryInput(pl, 0);
        break;
    }
}

/*
 * PL_CARRY_WALK: walks with the item; Player_HasCarryBonus (SKILL_CARRY_BONUS or carry kind
 * 6) adds a stride. Runs Player_UpdateCarryInput(pl, 1).
 */
void Player_StateCarryWalk(Player *pl, s32 unused)
{
    f32 strideLocal[3];
    f32 strideWorld[3];

    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->timer = 20;
        pl->dashDelay = 30;
        Player_SetMotionPair(pl, PL_MOT_CARRY_WALK, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x1000);
        break;
    case 1:
        if (Player_HasCarryBonus(pl) != 0) {
            strideLocal[0] = 0.0f;
            strideLocal[1] = 0.0f;
            strideLocal[2] = 2.0f;
            Vec_Rotate(strideWorld, strideLocal, pl->rotMtx);
            pl->pos[0] += strideWorld[0];
            pl->pos[2] += strideWorld[2];
        }
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateCarryInput(pl, 1);
        break;
    }
}

/*
 * PL_CARRY_RUN: runs with the item, using stamina (none left:
 * PL_CARRY_DROP_TIRED); a drop ahead requests PL_CARRY_JUMP_OFF. Runs
 * Player_UpdateCarryInput(pl, 1).
 */
void Player_StateCarryRun(Player *pl, s32 unused)
{
    f32 strideLocal[3];
    f32 strideWorld[3];

    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        if (pl->gender == 0) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_RUN, 4, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_RUN_ALT, 4, 0);
        }
        pl->timer = 20;
        pl->dashDelay = 30;
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        Player_SetFlag(pl, 0x2000200);
        break;
    case 1:
        if (Player_FloorAheadBelow(pl, 0, 85.0f, -60.0f) != 0) {
            pl->targetYaw = pl->rot[1];
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_JUMP_OFF, 0);
            break;
        }
        Player_UseStamina(pl, -1);
        if (pl->stamina <= 0) {
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_DROP_TIRED, 0);
            break;
        }
        if (Player_HasCarryBonus(pl) != 0) {
            strideLocal[0] = 0.0f;
            strideLocal[1] = 0.0f;
            strideLocal[2] = 5.0f;
            Vec_Rotate(strideWorld, strideLocal, pl->rotMtx);
            pl->pos[0] += strideWorld[0];
            pl->pos[2] += strideWorld[2];
        }
        if (pl->timer > 0) {
            pl->timer--;
        }
        Player_UpdateCarryInput(pl, 1);
        break;
    }
}

/*
 * Puts the item down. variant 0 PL_CARRY_DROP (the dropped item is made and
 * the local player's carried item leaves the pouch, with a notice), 1
 * PL_CARRY_DROP_TIRED (dropped but kept in the pouch), 2 PL_CARRY_DELIVER
 * (handed in at the delivery box: nothing is dropped). Then idle.
 */
void Player_StateCarryPutDown(Player *pl, s32 variant)
{
    u16 item;

    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 1) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_DROP_TIRED, 4, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_PUT_DOWN, 4, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, variant == 1 ? 4 : 114) != 0) {
            pl->step++;
            pl->carry &= PL_CARRY_KIND_MASK;
            if (variant != 2) {
                ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
            }
            if (variant == 0 && Player_IsLocal(pl) == 1) {
                item = Pouch_FindCarryItem(pl);
                if (item != 0xFFFF) {
                    Notice_Push(NOTICE_KIND_ITEM, 12, item);
                    Pouch_Add(pl, item, -100);
                }
            }
            break;
        }
        Player_UpdateCarryKind(pl);
        break;
    case 2:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 8, 0);
        }
        break;
    }
}

/*
 * The carrier in the air. variant 0 PL_CARRY_JUMP_OFF, 1 PL_CARRY_FALL; as
 * Player_StateMashAirborne. Landing after 60 frames (50 without
 * Player_HasCarryBonus) drops the item with PL_ACT_LAND_HARD, after 40 (30)
 * PL_CARRY_LAND_HARD, else PL_CARRY_LAND.
 */
void Player_StateCarryAirborne(Player *pl, s32 variant)
{
    f32 velLocal[3];
    f32 moveWorld[3];
    s16 dropTime;
    s16 hardTime;

    Player_UpdateCarryKind(pl);
    pl->timer++;
    switch (pl->step) {
    case 0:
        pl->step++;
        pl->action = PL_POSTURE_AIRBORNE;
        switch (variant) {
        case 0:
        default:
            Player_SetMotionPair(pl, PL_MOT_CARRY_AIRBORNE, 0, 0);
            pl->vel[0] = 0.0f;
            pl->vel[2] = 20.0f;
            pl->vel[1] = 18.0f;
            pl->accel[0] = 0.0f;
            pl->accel[2] = -(pl->vel[2] / 80.0f);
            Player_StartJump(pl, 19);
            break;
        case 1:
            Player_SetMotionPair(pl, PL_MOT_CARRY_AIRBORNE, 8, 32);
            pl->rideState = 0;
            pl->vel[0] = 0.0f;
            pl->vel[2] = 8.0f;
            pl->vel[1] = -10.0f;
            pl->accel[2] = 0.0f;
            pl->accel[0] = 0.0f;
            pl->accel[1] = -8.0f / 11.0f;
            break;
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_AIRBORNE, 0, 0);
        pl->timer = 0;
        pl->stateVar1 = 0;
        break;
    case 1:
        pl->vel[0] += pl->accel[0];
        pl->vel[1] += pl->accel[1];
        pl->vel[2] += pl->accel[2];
        if (pl->vel[0] < 0.0f) {
            pl->vel[0] = 0.0f;
        }
        if (pl->vel[2] < 0.0f) {
            pl->vel[2] = 0.0f;
        }
        velLocal[0] = pl->vel[0];
        velLocal[1] = pl->vel[1];
        velLocal[2] = pl->vel[2];
        Vec_Rotate(moveWorld, velLocal, pl->rotMtx);
        pl->pos[0] += moveWorld[0];
        pl->pos[1] += moveWorld[1];
        pl->pos[2] += moveWorld[2];
        if (pl->vel[1] < 0.0f) {
            pl->timer++;
            if (!(pl->floorY < pl->pos[1])) {
                pl->pos[1] = pl->floorY;
                pl->action = PL_POSTURE_STANDING;
                pl->rideState = 0;
                if (Player_HasCarryBonus(pl) == 1) {
                    dropTime = 60;
                    hardTime = 40;
                } else {
                    dropTime = 50;
                    hardTime = 30;
                }
                if (pl->timer >= dropTime) {
                    pl->carry &= PL_CARRY_KIND_MASK;
                    ItemProp_CreateAtPlayer(pl, ITEMPROP_DROPPED);
                    Player_RequestState(pl, PL_GROUP_ACTION, PL_ACT_LAND_HARD, 0);
                } else if (pl->timer >= hardTime) {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_LAND_HARD, PL_SET_NO_SYNC);
                } else {
                    Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_LAND, PL_SET_NO_SYNC);
                }
            }
        }
        break;
    }
}

/*
 * The landings with the item: variant 0 PL_CARRY_LAND_HARD, 1 PL_CARRY_LAND
 * (the motion). Then PL_CARRY_IDLE.
 */
void Player_StateCarryLand(Player *pl, s32 variant)
{
    Player_UpdateCarryKind(pl);
    switch (pl->step) {
    case 0:
        pl->step++;
        if (variant == 0) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_LAND_HARD, 0, 0);
        } else {
            Player_SetMotionPair(pl, PL_MOT_CARRY_LAND, 0, 0);
        }
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_RequestState(pl, PL_GROUP_CARRY, PL_CARRY_IDLE, PL_SET_NO_SYNC);
        }
        break;
    }
}

/*
 * PL_CARRY_HAND_OVER: gives the carried item to the trader (EM_KIND_TRADER):
 * waits for it to leave the pouch, puts it down, waits for tradeMsgTimer and
 * returns to idle.
 */
void Player_StateCarryHandOver(Player *pl, s32 unused)
{
    pl->noHitTimer = 3;
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_UpdateCarryKind(pl);
        if (pl->motionId[0] != 51) {
            Player_SetMotionPair(pl, PL_MOT_CARRY_IDLE, 4, 0);
        }
        pl->tradeMsgTimer = 60;
        pl->timer = 60;
        break;
    case 1:
        Player_UpdateCarryKind(pl);
        if ((Pouch_FindCarryItem(pl) & 0xFFFF) == 0xFFFF) {
            pl->step++;
            Player_SetMotionPair(pl, PL_MOT_CARRY_PUT_DOWN, 4, 0);
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 114.0f) != 0) {
            pl->step++;
            pl->carry &= PL_CARRY_KIND_MASK;
            break;
        }
        Player_UpdateCarryKind(pl);
        break;
    case 3:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_StartIdleMotion(pl, 8, 0);
        }
        break;
    case 4:
        if (pl->tradeMsgTimer == 0) {
            Player_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * Runs the handler of the player's state in PL_GROUP_CARRY, the weapon
 * sheathed. Shared handlers get a variant: Player_StateCarryPutDown (drop,
 * tired drop, deliver), Player_StateCarryAirborne, Player_StateCarryLand,
 * Player_StateCarryIdle (also PL_CARRY_PICKED_UP). Other states return to
 * idle.
 */
void Player_RunCarryState(Player *pl)
{
    pl->weaponOut = 0;
    switch (pl->state) {
    case PL_CARRY_IDLE:
        Player_StateCarryIdle(pl, 0);
        break;
    case PL_CARRY_WALK:
        Player_StateCarryWalk(pl, 0);
        break;
    case PL_CARRY_RUN:
        Player_StateCarryRun(pl, 0);
        break;
    case PL_CARRY_DROP:
        Player_StateCarryPutDown(pl, 0);
        break;
    case PL_CARRY_DROP_TIRED:
        Player_StateCarryPutDown(pl, 1);
        break;
    case PL_CARRY_JUMP_OFF:
        Player_StateCarryAirborne(pl, 0);
        break;
    case PL_CARRY_FALL:
        Player_StateCarryAirborne(pl, 1);
        break;
    case PL_CARRY_LAND_HARD:
        Player_StateCarryLand(pl, 0);
        break;
    case PL_CARRY_LAND:
        Player_StateCarryLand(pl, 1);
        break;
    case PL_CARRY_PICKED_UP:
        Player_StateCarryIdle(pl, 1);
        break;
    case PL_CARRY_DELIVER:
        Player_StateCarryPutDown(pl, 2);
        break;
    case PL_CARRY_HAND_OVER:
        Player_StateCarryHandOver(pl, 0);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * PL_GESTURE_LIE_DOWN: lies down; the local player gets up
 * (PL_GESTURE_GET_UP) with the left stick or at the quest's end, or starts a
 * requested gesture (Player_StartGesture).
 */
void Player_StateGestureLieDown(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_LIE_FACE_UP, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1) {
            if (((s32) pl->input.stickTilt >= PL_STICK_HELD) || (Quest_PhaseIsOver(1) == 1)) {
                Player_RequestState(pl, PL_GROUP_GESTURE, PL_GESTURE_GET_UP, 0);
                break;
            }
            if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        } else {
        }
        break;
    }
}

/* PL_GESTURE_GET_UP: gets up, then idle. */
void Player_StateGestureGetUp(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_SCOUT: the scouting gesture, held at least 60 frames, then left
 * as the other gestures are. When the area's flyer (Area_HasFlyer,
 * Area_GetFlyerPos) is within about 33 degrees of the facing, it sets pl->scoutSpotted
 * and actDetectTimer (300) once.
 */
void Player_StateGestureScout(Player *pl, s32 variant)
{
    f32 pos[3];
    s32 angleOff;

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_GESTURE_SCOUT, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            } else if (Area_HasFlyer() == 1 && pl->scoutSpotted == 0) {
                Area_GetFlyerPos(pos, pl->area);
                angleOff = (u16) (pl->rot[1] - (u16) (Vec_Heading(pos, pl->pos) + ANGLE_TURN / 4));
                if (angleOff <= (ANGLE_TURN * 3 / 32) || angleOff >= ANGLE_TURN * 29 / 32) {
                    pl->scoutSpotted = 1;
                    pl->actDetectTimer = 300;
                }
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_03: a looped gesture; from frame 90 the local player moves to
 * PL_GESTURE_03_END with the left stick or at the quest's end, or starts a
 * requested gesture.
 */
void Player_StateGesture03(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_GESTURE_SIT, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (Player_IsLocal(pl) == 1 && Motion_ReachedFrame(pl, 0, 90.0f) != 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_RequestState(pl, PL_GROUP_GESTURE, PL_GESTURE_03_END, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_03_END: the end of PL_GESTURE_03, then idle. */
void Player_StateGesture03End(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, PL_MOT_GESTURE_SIT_END, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->step++;
            Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
            Player_SetMotionPair(pl, PL_MOT_GET_UP_FACE_UP, -4, 60);
        }
        break;
    case 2:
        Player_SetFlag(pl, PLF_NO_ROOT_MOVE);
        if (pl->motion[0].playing == 0) {
            Player_ClearFlag(pl, PLF_NO_ROOT_MOVE);
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * PL_GESTURE_05: a gesture that makes PlayerAttack 96 and 97 at set
 * frames; held at least 60 frames, then left as Player_StateGesture06.
 */
void Player_StateGesture05(Player *pl, s32 variant)
{

    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 628, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Motion_PassedFrame(pl, 0, 48.0f) != 0) {
            PlayerAttack_Create(pl, 96);
        }
        if (Motion_PassedFrame(pl, 0, 68.0f) != 0) {
            PlayerAttack_Create(pl, 97);
        }
        if (Motion_PassedFrame(pl, 0, 168.0f) != 0) {
            PlayerAttack_Create(pl, 97);
        }
        if (Motion_PassedFrame(pl, 0, 190.0f) != 0) {
            PlayerAttack_Create(pl, 96);
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt > 39 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 10, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * PL_GESTURE_06 and PL_GESTURE_06_ALT: a gesture held at least 60 frames;
 * then the local player returns to idle with the left stick or at the quest's
 * end, or starts a requested gesture (Player_StartGesture).
 */
void Player_StateGesture06(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 632, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 10, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_07: a gesture; as Player_StateGesture06. */
void Player_StateGesture07(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 622, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_08: a gesture; as Player_StateGesture06. */
void Player_StateGesture08(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 627, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/*
 * The one-shot gestures: one motion, then idle. variant: the row of
 * gGestureOneShots (0 PL_GESTURE_ONESHOT0, 1 PL_GESTURE_ONESHOT1, 3
 * PL_GESTURE_ONESHOT3).
 */
void Player_StateGestureOneShot(Player *pl, s32 variant)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, gGestureOneShots[variant].motion, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Player_ReturnToIdle(pl, 0, gGestureOneShots[variant].blend, 0);
        }
        break;
    }
}

/* PL_GESTURE_0A: a gesture; as Player_StateGesture06. */
void Player_StateGesture0A(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 625, 4, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_0B: a gesture; as Player_StateGesture06. */
void Player_StateGesture0B(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 630, 16, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 10, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_0C: a gesture; as Player_StateGesture06. */
void Player_StateGesture0C(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 626, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* PL_GESTURE_10: a gesture; as Player_StateGesture06. */
void Player_StateGesture10(Player *pl, s32 unused)
{
    switch (pl->step) {
    case 0:
        pl->step++;
        Player_SetMotionPair(pl, 633, 6, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetPosture(pl, PL_POSTURE_STANDING, 0, 0);
        pl->timer = 60;
        break;
    case 1:
        if (pl->timer > 0) {
            pl->timer--;
        }
        if (Player_IsLocal(pl) == 1 && pl->timer == 0) {
            if (pl->input.stickTilt >= 40 || Quest_PhaseIsOver(1) == 1) {
                Player_ReturnToIdle(pl, 0, 4, 0);
            } else if (pl->actRequest != 0) {
                Player_StartGesture(pl);
            }
        }
        break;
    }
}

/* The second part's data. */
SpitMeat gSpitMeats[13] = {
#include "game/008C3C90.inc"
};
s8 gStunMashNeed[8] = {
#include "game/008C3DC8.inc"
};
GestureOneShot gGestureOneShots[4] = {
#include "game/008C3DD0.inc"
};

/* ---- The third part: from Player_RunGestureState on, at optimization level 4 ---- */
/* Declarations of the functions called below. */
struct TriHdr;
struct ObjHead;
/* Level 4 from here to the end of the unit (Player_Update, Player_CheckTeamCombo
 * need it; the parts before need level 3). */
#pragma optimization_level 4

/* prototypes */
typedef struct ArmorIds {
    u8 id[4];
} ArmorIds;
typedef struct ArmorCombo {
    u8 a;
    u8 b;
    ArmorIds ids;
} ArmorCombo;
extern ArmorCombo gArmorCombos[14];
/* end of prototypes */

void Player_RunGestureState(Player *pl)
{
    u8 state;

    Player_ResetAim(pl);
    state = pl->state;
    switch (state) {
    case PL_GESTURE_LIE_DOWN:
        Player_StateGestureLieDown(pl, 0);
        break;
    case PL_GESTURE_GET_UP:
        Player_StateGestureGetUp(pl, 0);
        break;
    case PL_GESTURE_SCOUT:
        Player_StateGestureScout(pl, 0);
        break;
    case PL_GESTURE_03:
        Player_StateGesture03(pl, 0);
        break;
    case PL_GESTURE_03_END:
        Player_StateGesture03End(pl, 0);
        break;
    case PL_GESTURE_05:
        Player_StateGesture05(pl, 0);
        break;
    case PL_GESTURE_06:
        Player_StateGesture06(pl, 0);
        break;
    case PL_GESTURE_07:
        Player_StateGesture07(pl, 0);
        break;
    case PL_GESTURE_08:
        Player_StateGesture08(pl, 0);
        break;
    case PL_GESTURE_ONESHOT0:
        Player_StateGestureOneShot(pl, 0);
        break;
    case PL_GESTURE_0A:
        Player_StateGesture0A(pl, 0);
        break;
    case PL_GESTURE_0B:
        Player_StateGesture0B(pl, 0);
        break;
    case PL_GESTURE_0C:
        Player_StateGesture0C(pl, 0);
        break;
    case PL_GESTURE_ONESHOT1:
        Player_StateGestureOneShot(pl, 1);
        break;
    case PL_GESTURE_06_ALT:
        Player_StateGesture06(pl, 0);
        break;
    case PL_GESTURE_ONESHOT3:
        Player_StateGestureOneShot(pl, 3);
        break;
    case PL_GESTURE_10:
        Player_StateGesture10(pl, 0);
        break;
    default:
        Player_ReturnToIdle(pl, 0, 4, 0);
        break;
    }
}

/*
 * The players' frame: reads the pad (Player_UpdateInput), updates the
 * PLAYER_SLOTS players (Player_Update), then the attack objects' timers
 * (ObjAttach_UpdateAllTimers).
 */
void Game_UpdatePlayers(void)
{
    s32 i;

    Player_UpdateInput();
    for (i = 0; i < 10; i++) {
        Player_Update(&gPlayers[i]);
    }
    ObjAttach_UpdateAllTimers();
}

/*
 * Runs the handler of the player's state group (Player_RunActionState ...
 * Player_RunGestureState for PL_GROUP_ACTION to PL_GROUP_GESTURE), after
 * setting freeState 0 and fieldCollide 1.
 */
void Player_RunState(Player *pl)
{
    pl->freeState = 0;
    pl->fieldCollide = 1;
    switch (pl->stateGroup) {
    case 0:
        Player_RunActionState(pl);
        break;
    case 1:
        Player_RunWeaponState(pl);
        break;
    case 2:
        Player_RunReactState(pl);
        break;
    case 3:
        Player_RunFaintState(pl);
        break;
    case 4:
        Player_RunEventState(pl);
        break;
    case 5:
        Player_RunCarryState(pl);
        break;
    case 6:
        Player_RunGestureState(pl);
        
    default:
        break;
    }
}

/*
 * Offline, for the local player: when a visible EM_KIND_KUSHALA_DAORA is in
 * the player's area, sets EVENT_FLAG_KUSHALA_SEEN (the Kushala Daora has been
 * seen).
 */
void Player_CheckKushalaSeen(Player *pl)
{
    Em *em;
    s16 i;

    if (Player_IsLocal(pl) == 0 || Game_IsOnline() == 1) {
        return;
    }
    em = gEms;
    for (i = 0; i < EM_COUNT; i++) {
        if (em->active != 0 && em->visible != 0 && em->type == EM_KIND_KUSHALA_DAORA && em->area == pl->area && Save_TestEventFlag(EVENT_FLAG_KUSHALA_SEEN) == 0) {
            Save_SetEventFlag(EVENT_FLAG_KUSHALA_SEEN);
        }
        em++;
    }
}

struct DrawSlot;
extern u16 gFrameTick; /* sub_main: the frame counter */

/* A player's per-frame update (Game_UpdatePlayers). */
void Player_Update(Player *pl)
{
    f32 floorY;
    f32 pos[3];
    f32 fade;
    f32 buf[3];
    f32 y;
    f32 groundY;
    s32 clamp;
    u8 group;
    u8 w;
    s16 n;

    if (pl->active == 0) {
        return;
    }
    Player_CheckKushalaSeen(pl);
    Player_UpdateInBattle(pl);
    Player_TickTimers(pl);
    pl->pouchHudFlags = 0;
    pl->offerStarted = 0;
    if (pl->supplyBoxOpen == 0 || pl->deliveryMenu != 0) {
        Player_UpdateItemSelect(pl);
        Player_BowgunAmmoSwitch(pl);
    }
    ((Em *) pl)->prevPosX = pl->pos[0];
    ((Em *) pl)->prevPosY = pl->pos[1];
    ((Em *) pl)->prevPosZ = pl->pos[2];
    pl->unk58C = -1;
    pl->unk58E = -1;
    pl->unk3F8 = 0;
    pl->bowgunReady = 0;
    pl->freeState = 0;
    if (Player_IsLocal(pl) == 1) {
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_FALL) == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB) == 0) {
            pl->fallGrabCount = 0;
        }
        Player_CheckTeamComboLost(pl);
    } else {
        pl->fallGrabCount = 0;
    }
    Player_ReactToHit((struct Player *)pl);
    pl->prevFreeState = 0;
    switch ((u8) Player_PickStatusState(pl)) {
    case 1:
        Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STATUS2, 0);
        break;
    case 2:
        if (Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS4) == 0 && Player_IsState(pl, PL_GROUP_REACT, PL_REACT_STATUS4_HIT) == 0) {
            Player_SetState(pl, PL_GROUP_REACT, PL_REACT_STATUS4, 0);
        }
        break;
    }
    Player_CalcAttack(pl);
    Player_CalcDefense(pl);
    Player_CalcResists(pl);
    if (pl->hitStop == 0) {
        Player_UpdateEnvironment(pl);
        Player_RunState(pl);
        if (pl->stateRequest != 0) {
            Player_RunState(pl);
            pl->stateRequest = 0;
        }
        Player_UpdateAutoTurn(pl);
        Player_FindLookTarget(pl);
        if (pl->stateGroup != PL_GROUP_EVENT) {
            Player_UpdateMoveTo(pl);
        }
    }
    Player_CheckPointMessages((u8 *) pl);
    pl->prevFreeState = pl->freeState;
    if (pl->weaponOut == 0) {
        pl->demonMode = 0;
    }
    Player_UpdateStatusIcons(pl);
    if (pl->statusIcons & 0x400) {
        PlayerFx_TickStatus6(pl);
    }
    if (pl->statusIcons & 0x800) {
        PlayerFx_TickStatus7(pl);
    }
    pl->actRequest = 0;
    pl->offerRequest = 0;
    Player_UpdateHitStop(pl);
    if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) == 0 || (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_WALL_HOLD) != 0 && pl->stateVar0 == 0)) {
        Player_UpdateMotionStep(pl);
    }
    Player_UpdateHitTimers(pl);
    pl->handlers->motionFxStep(pl);
    if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_GRAB) == 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_STEP_UP) == 0) {
        if ((u8) Obj_InCurrentArea(pl)) {
            Em_CollideField((struct Em *)pl, 0);
        }
    }
    if (pl->stateGroup == PL_GROUP_ACTION && (pl->state == 1 || pl->state == 2 || pl->state == 3 || pl->state == 19) && (u8) Obj_InCurrentArea(pl)) {
        Em_FindFloorTri((Em *)pl, buf, 1);
    }
    Player_RecordActionSpot(pl);
    if (Em_GetFloorInfo((struct Em *)pl, pl->pos, (struct TriHdr *)(&(pl)->floorKind), &floorY, (s32 *)(&(pl)->surfaceY)) == 1 && Player_IsState(pl, PL_GROUP_EVENT, PL_EVENT_CART) == 0) {
        pl->floorY = floorY;
    }
    if ((u8) Obj_InCurrentArea(pl)) {
        Obj_UpdateFloorEffects((struct ObjHead *) pl);
    }
    if (pl->action != PL_POSTURE_AIRBORNE && pl->rideState == 0 && Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_LEDGE_PULL_UP) == 0 && pl->stateGroup != PL_GROUP_EVENT && (u8) Obj_InCurrentArea(pl)) {
        groundY = pl->floorY;
        y = pl->pos[1];
        if (y < groundY) {
            pl->pos[1] = groundY;
            pl->rideState = 0;
        } else if (y >= groundY) {
            if (y - groundY < 30.0f) {
                pl->pos[1] = groundY;
                pl->rideState = 0;
            } else if (pl->hp > 0) {
            group = pl->stateGroup;
            if (group != 5) {
                if (group == 2) {
                switch (pl->state) {
                case 27:
                case 30:
                case 31:
                case 32:
                case 33:
                case 34:
                case 35:
                    Player_SetState(pl, PL_GROUP_REACT, PL_REACT_MASH_FALL, 0);
                    break;
                default:
                    Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FALL, 0);
                    break;
                }
            } else {
                Player_SetState(pl, PL_GROUP_ACTION, PL_ACT_FALL, 0);
            }
            } else {
                Player_SetState(pl, PL_GROUP_CARRY, PL_CARRY_FALL, 0);
            }
            }
        }
    }
    pl->unk417 = 1;
    Player_UpdateAreaPos(pl);
    group = pl->stateGroup;
    if (group != 2 && group != 3) {
        if (pl->weaponOut && (pl->weaponType == WEAPON_HEAVY_BOWGUN || pl->weaponType == WEAPON_LIGHT_BOWGUN) && (pl->input.pressed & PAD_R1)) {
            if (group == 0) {
                switch (pl->state) {
                case 28:
                case 103:
                case 104:
                case 105:
                case 106:
                    break;
                case 27:
                case 58:
                    pl->aimMode = 0;
                    break;
                default:
                    pl->aimMode ^= 1;
                    break;
                }
            } else if (group != 6 && Player_IsState(pl, PL_GROUP_WEAPON, BOWGUN_MELEE) == 0) {
                pl->aimMode ^= 1;
            }
            if (pl->action == PL_POSTURE_AIRBORNE) {
                pl->aimMode = 0;
            }
            if (Player_LocalCanUseBinoculars() == 0) {
                if ((u8) Obj_InCurrentArea(pl) == 1) {
                    Sound_PlaySeFlat(0, 21, 0);
                }
                Player_ResetAim(pl);
            }
        } else if (pl->weaponOut == 0) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_AIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_REAIM) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_MOUNTED_GUN_FIRE) != 0) {
                if (pl->input.pressed & PAD_R1) {
                    pl->aimMode ^= 1;
                }
            } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS_CROUCHED) != 0) {
                pl->aimMode = 1;
            } else {
                Player_ResetAim(pl);
            }
        } else if (pl->isGunner != 1) {
            if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_BINOCULARS_CROUCHED) != 0) {
                pl->aimMode = 1;
            } else {
                Player_ResetAim(pl);
            }
        }
    }
    if (pl->aimMode == 0) {
        pl->aimLockTimer = 0;
    }
    if (pl->weaponType == WEAPON_HUNTING_HORN && HuntingHorn_PlayKind(pl) == 0) {
        HuntingHorn_ClearNotes((struct Player *)pl);
    }
    if (pl->area == gScene.area) {
        Player_UpdateNearMonster(pl);
        pl->drawSlot->pos[0] = pl->pos[0];
        pl->drawSlot->pos[1] = pl->pos[1];
        pl->drawSlot->pos[2] = pl->pos[2];
        if (Player_IsLocal(pl) == 1) {
            pl->nearCamera = 0;
        } else {
            pos[0] = pl->pos[0];
            pos[1] = pl->pos[1] + 80.0f;
            pos[2] = pl->pos[2];
            if (Camera_Distance(pos) <= 200.0f) {
                pl->nearCamera = 1;
            } else {
                pl->nearCamera = 0;
            }
        }
        if (Player_IsLocal(pl) == 0 && Player_TestFlag(pl, 0x02001600) != 0) {
            pl->rot[1] = (u16) (Vec_Heading(pl->pos, pl->prevPos) + ANGLE_TURN / 4);
        }
        CamCtl_GetPlayerFade(&fade);
        clamp = Player_InSceneEvent() == 1;
        if (pl->nearCamera == 0 && !(fade < 1.0f)) {
            OT_AddByDepth(gDrawOt32, pl->drawSlot, 32, clamp);
        } else {
            OT_AddByDepth(gDrawOtSorted, pl->drawSlot, 64, clamp);
        }
        if (pl->syncTimer != 0) {
            (pl->syncTimer)--;
        }
        n = *(s16 *)&gQSyncSendPeriod;
        if (pl->syncTimer == 0 && gFrameTick % n == 0) {
            QSync_SendPlayer(pl, QSYNC_PL_POS, 0);
        }
    }
}

/*
 * Turns the player (rot[1]) toward targetYaw by a step that depends on the
 * state (the run and the drawn weapon's first actions turn fastest, the dash
 * slower, the turn-around not at all), and swings the body's lean with the
 * turn.
 */
void Player_UpdateAutoTurn(Player *pl)
{
    s32 step;
    u32 angleDelta;
    u16 turn;
    s32 target;
    s32 dir;

    if (pl->areaMoveState != PL_AREA_MOVE_NONE) {
        return;
    }
    if (pl->weaponOut != 0 && pl->stateGroup == PL_GROUP_ACTION && (
        (u32) (pl->state - 1) <= 1
        || pl->state == 12)) {
        step = 1820;
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_RUN) != 0 || Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_RUN_AFTER_DASH) != 0) {
        step = 1820;
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_DASH) != 0) {
        step = 1456;
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_TURN_AROUND) != 0) {
        step = 0;
    } else {
        step = 4004;
    }
turn:
    dir = pl->rot[1];
    target = pl->targetYaw;
    angleDelta = (u16) (target - (u16) dir);
    switch (pl->motionId[0]) {
    case 3:
        turn = 128;
        break;
    case 4:
        turn = 160;
        break;
    default:
        turn = 0;
        break;
    }
    if ((u16) (angleDelta + step) < (u32) (step * 2)) {
        pl->rot[1] = target;
        pl->unk2B8 = 0;
        if (pl->lean + 768 <= 1536) {
            pl->lean = 0;
        } else if (pl->lean < 0x8000) {
            pl->lean -= 768;
        } else {
            pl->lean += 768;
        }
    } else if (angleDelta < 0x8000U) {
        pl->rot[1] = (u16) (dir + step);
        pl->lean -= turn;
    } else {
        pl->rot[1] = (u16) (dir - step);
        pl->lean += turn;
    }
    if (pl->lean <= 0xF600 && pl->lean >= 2560) {
        if (pl->lean >= 0x8000) {
            pl->lean = 62976;
        } else {
            pl->lean = 2560;
        }
    }
}

/*
 * The head turn toward the nearest monster (pl->lookYaw): in PL_MOT_FLEE_DASH
 * only, picks the nearest active, visible monster within 1500 units
 * (lookFound) and turns the head toward it by ANGLE_TURN / 32 a frame, to about 60
 * degrees either side; without one the turn unwinds. The turn is read and
 * written as a u16 here, as the game's code does (its tests are unsigned).
 */
void Player_FindLookTarget(Player *pl)
{
    u32 i;
    u32 best;
    Em *em;
    f32 min;
    f32 dist;
    u16 relAngle;

    best = 255;
    pl->lookFound = 0;
    min = 1500.0f;
    em = gEms;
    if (pl->motionId[0] == 38) {
        for (i = 0; i < EM_COUNT; i++, em++) {
            if (em->active != 0 && em->visible != 0) {
                dist = Vec_Distance(&pl->pos[0], &em->posX);
                if (!(min < dist)) {
                    best = i;
                    min = dist;
                    pl->lookFound = 1;
                }
            }
        }
    }
    if (pl->lookFound == 0) {
        if (*(u16 *) &pl->lookYaw != 0) {
            if (*(u16 *) &pl->lookYaw + 0x400 <= 0x800 && *(u16 *) &pl->lookYaw + 0x400 >= 0) {
                *(u16 *) &pl->lookYaw = 0;
            } else if ((s16) *(u16 *) &pl->lookYaw >= 0) {
                *(u16 *) &pl->lookYaw -= 0x400;
            } else {
                *(u16 *) &pl->lookYaw += 0x400;
            }
        }
    } else {
        relAngle = (u16) (Vec_Heading(&gEms[best].posX, &pl->pos[0]) + ANGLE_TURN / 4) + ANGLE_TURN - pl->rot[1];
        if (relAngle >= ANGLE_TURN / 2) {
            if (relAngle < 0xD556) {
                relAngle = -0x2AAA;
            }
            if (*(u16 *) &pl->lookYaw >= ANGLE_TURN / 2) {
                if (*(u16 *) &pl->lookYaw - (ANGLE_TURN / 32) <= relAngle) {
                    *(u16 *) &pl->lookYaw = relAngle;
                } else {
                    *(u16 *) &pl->lookYaw = *(u16 *) &pl->lookYaw - ANGLE_TURN / 32;
                }
            } else {
                *(u16 *) &pl->lookYaw -= 0x800;
            }
        } else {
            if (relAngle > 0x2AAB) {
                relAngle = 0x2AAB;
            }
            if (*(u16 *) &pl->lookYaw < (ANGLE_TURN / 2)) {
                if (*(u16 *) &pl->lookYaw + (ANGLE_TURN / 32) >= relAngle) {
                    *(u16 *) &pl->lookYaw = relAngle;
                } else {
                    *(u16 *) &pl->lookYaw = *(u16 *) &pl->lookYaw + ANGLE_TURN / 32;
                }
            } else {
                *(u16 *) &pl->lookYaw += 0x800;
            }
        }
    }
}

/*
 * For the local player: the advice message of the action spot it stands at
 * (MsgEvent_Check: 1 at a spot of kind 3, 3 at the delivery box, 4 at the
 * bed).
 */
void Player_CheckPointMessages(u8 *script)
{
    f32 pos[3];
    u16 spotAngle;
    s8 spotType;
    s32 spot;

    if (Player_IsLocal(script) != 0) {
        spot = Player_FindActionSpot((Player *)script, (f32 *)pos, &spotAngle, (u8 *)&spotType) & 0xFFFF;
        switch (spot) {
        case 3:
            MsgEvent_Check(MSGEV_SUPPLY_BOX);
            break;
        case 7:
            MsgEvent_Check(MSGEV_DELIVERY_BOX);
            break;
        case 5:
            MsgEvent_Check(MSGEV_BED);
            break;
        }
    }
}

/*
 * Returns how many of three special items the player holds (0 to 3);
 * Player_UpdateEnvironment takes that much health every 30 frames, and the
 * HUD flashes the health bar.
 */
s16 Player_CountSpecialItems(Player *pl)
{
    s16 n = 0;

    if (Pouch_Count(pl, 676) > 0 || ItemGet_Result() == 676) {
        n++;
    }
    if (Pouch_Count(pl, 678) > 0 || ItemGet_Result() == 678) {
        n++;
    }
    if (Pouch_Count(pl, 675) > 0) {
        n++;
    }
    return n;
}

/* With four hunters each wearing armour, looks up their four armour styles (Player.style, in any
 * order) in the 14 combinations of gArmorCombos; a match sets the team combo skill (teamSkillGroup,
 * teamSkill) and shows its message. */
void Player_CheckTeamCombo(Player *self)
{
    u8 have[4];
    u8 left[4];
    u8 want[4];
    s16 i;
    s16 n;
    Player *pl;
    s16 slot;
    u8 w;
    u8 *src;
    s16 j;
    u8 skill;
    u8 skillGroup;
    ArmorCombo *combo;
    s16 k;

    self->teamSkill = 0;
    self->teamSkillGroup = 0;
    if (Quest_IsVersus() != 1 && gScene.playerCount > 3) {
    for (pl = gPlayers, i = 0; i < 4; i++, pl++) {
        if (pl->active == 0) {
            return;
        }
        have[i] = 255;
        n = 0;
        if (Armor_HasPartFlag(&pl->equip[2], pl->gender) == 1) {
            n++;
        }
        if (Armor_HasPartFlag(&pl->equip[3], pl->gender) == 1) {
            n++;
        }
        if (Armor_HasPartFlag(&pl->equip[4], pl->gender) == 1) {
            n++;
        }
        if (Armor_HasPartFlag(&pl->equip[5], pl->gender) == 1) {
            n++;
        }
        if (Armor_HasPartFlag(&pl->equip[0], pl->gender) == 1) {
            n++;
        }
        if (n == 0) {
            return;
        }
        have[i] = pl->style;
    }
    for (i = 0, combo = gArmorCombos; i < 14; i++, combo++) {
        skillGroup = combo->a;
        skill = combo->b;
        src = combo->ids.id;
        for (j = 0; j < 4; j++) {
            left[j] = have[j];
            want[j] = *src++;
        }
        for (j = 0; j < 4; j++) {
            w = want[j];
            for (slot = 0; slot < 4; slot++) {
                if (w != 255 && w == left[slot]) {
                    want[j] = 255;
                    left[slot] = 255;
                    break;
                }
            }
        }
        for (j = 0; j < 4; j++) {
            if (want[j] != 255) {
                goto next;
            }
        }
        self->teamSkillGroup = skillGroup;
        self->teamSkill = skill;
        Notice_Push(NOTICE_KIND_MESSAGE, 29, 0);
        return;
    next:;
    }
    }
}

/*
 * Ends the team combo skill (teamSkill, teamSkillGroup) with a message when
 * any of the quest's players has left the quest.
 */
void Player_CheckTeamComboLost(Player *pl)
{
    s16 i;

    if (pl->teamSkill != 0) {
        for (i = 0; i < gScene.playerCount; i++) {
            if (gScene.inQuest[i] != 1) {
                pl->teamSkill = 0;
                pl->teamSkillGroup = 0;
                Notice_Push(NOTICE_KIND_MESSAGE, 30, 0);
                return;
            }
        }
    }
}

/*
 * The hit-free frames of an evasion: noHitTimer 6, plus 2 with SKILL_ROLL_NO_HIT_PLUS2 or
 * else plus 4 with SKILL_ROLL_NO_HIT_PLUS4.
 */
void Player_SetEvadeFrames(Player *pl)
{
    s16 frames = 6;

    if (Player_HasSkill(pl, SKILL_ROLL_NO_HIT_PLUS2) == 1) {
        frames += 2;
    } else if (Player_HasSkill(pl, SKILL_ROLL_NO_HIT_PLUS4) == 1) {
        frames += 4;
    }
    pl->noHitTimer = frames;
}

/* 1 while status 6 lasts (Player.status6Timer); the drink and eat items
 * refuse to start then. */
s32 Player_Status6Active(Player *pl)
{
    return pl->status6Timer > 0;
}

/* 1 while status 7 lasts (Player.status7Timer; the HUD's status icon). */
s32 Player_Status7Active(Player *pl)
{
    return pl->status7Timer > 0;
}

/*
 * Returns 1 when the item id is in a fixed list, else 0: the Chameleos
 * prefers these when it steals from the pouch, and Item_IsUsable refuses them
 * while status 6 lasts (Player_Status6Active).
 */
s32 Item_IsStealable(s32 item)
{
    switch (item & 0xFFFF) {
    case ITEM_ID_HEAL_20:
    case ITEM_ID_CURE_POISON:
    case ITEM_ID_HEAL_10_CURE:
    case ITEM_ID_HEAL_30:
    case ITEM_ID_HEAL_50:
    case ITEM_ID_SUPPLY_HEAL_30:
    case ITEM_ID_MAX_HP_10:
    case ITEM_ID_MAX_HP_20:
    case ITEM_ID_MAX_HP_FULL:
    case ITEM_ID_SUPPLY_MAX_HP_FULL:
    case ITEM_ID_MAX_ALL:
    case ITEM_ID_STAMINA_LOCK:
    case ITEM_ID_STAMINA_LOCK_LONG:
    case ITEM_ID_ATTACK_DRUG:
    case ITEM_ID_ATTACK_DRUG_STRONG:
    case ITEM_ID_DEFENSE_DRUG:
    case ITEM_ID_DEFENSE_DRUG_STRONG:
    case ITEM_ID_SUPPLY_MAX_STAMINA_75:
    case ITEM_ID_HURT_CURE_CHANCE:
    case ITEM_ID_CURE_CHANCE:
    case ITEM_ID_POISON_GAMBLE:
    case ITEM_ID_ATTACK_SHARED:
    case ITEM_ID_DEFENSE_SHARED:
    case ITEM_ID_COOL_DRINK:
    case ITEM_ID_HOT_DRINK:
    case ITEM_ID_HEAL_50_TEAM:
    case ITEM_ID_DETECT:
    case ITEM_ID_REGEN:
    case ITEM_ID_MAX_STAMINA_75_B:
    case ITEM_ID_ATTACK_BURST:
    case ITEM_ID_DEFENSE_BURST:
    case ITEM_ID_MAX_STAMINA_75:
    case ITEM_ID_RANDOM:
    case ITEM_ID_FOOD_STAMINA_75:
    case ITEM_ID_FOOD_STAMINA_150:
    case ITEM_ID_FOOD_STAMINA_FULL:
    case ITEM_ID_FOOD_STAMINA_GAMBLE:
    case ITEM_ID_FOOD_HEAL_20:
    case ITEM_ID_FOOD_PRIZE_A:
    case ITEM_ID_FOOD_PRIZE_B:
    case ITEM_ID_FOOD_HOT:
    case ITEM_ID_FOOD_HOT_STAMINA:
    case ITEM_ID_FOOD_COOL:
    case ITEM_ID_FOOD_COOL_STAMINA:
    case ITEM_ID_FOOD_CURE:
    case ITEM_ID_FOOD_CURE_STAMINA:
    case ITEM_ID_FOOD_RESTORE:
    case ITEM_ID_FOOD_REGEN_SHORT:
    case ITEM_ID_FOOD_REGEN:
    case ITEM_ID_FOOD_REGEN_B:
    case ITEM_ID_FOOD_REGEN_LONG:
    case ITEM_ID_FOOD_RESTORE_B:
        return 1;
    }
    return 0;
}

/* The third part's data. */
ArmorCombo gArmorCombos[14] = {
#include "game/008C3DE0.inc"
};
