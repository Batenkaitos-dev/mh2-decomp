/*
 * The player's state code (sub_main's half of it; the states themselves are
 * the game overlay's, include/player.h):
 * - the state changes (Player_SetState, Player_RequestState, Player_IsState)
 *   and the flags (Player_TestFlag, Player_SetFlag, Player_ClearFlag), which
 *   take a Player or an Em (both start with ObjHead);
 * - setting the idle motion (Player_SetIdle), clearing a player's action
 *   state and returning it to idle (Player_ReturnToIdle, called by most of
 *   the player's states when they end);
 * - copying the pad input into the local player's input block every frame
 *   (Player_CopyPadInput), with the menus' input locks.
 *
 * Player_ResetActionStub and Player_ClearMotion are static: MWCC uses the
 * register use of a static function defined earlier in the file, so after
 * the empty Player_ResetActionStub it knows a0 still holds the player, and
 * Player_ReturnToIdleKeepParts passes it on without setting a0 again (the
 * match needs this).
 */
#include "common.h"
#include "quest.h"
#include "scene.h"
#include "player.h"
#include "pad.h"
#include "hud.h"
#include "qsync.h"
#include "objhead.h"
#include "em.h"
#include "area.h"
#include "game_util.h"
#include "mem_util.h"
#include "rand.h"
#include "sound.h"

s16 Player_IsState(void *obj, u8 state, u8 sub);
/* unit's own declaration: an s32 frame as Player_StartIdleMotion's calls pass it; with the definition's s16 the calls compile differently */
void Player_SetMotion(Player *pl, s32 motion, s32 blend, s32 frame, s32 layer);

/* Turns off the part motion overrides of the four motion layers. */
void Player_ClearPartOverrides(Player *pl)
{
    s32 i;

    for (i = 0; i < 4; i++) {
        pl->partAnimOn[i] = 0;
        pl->partAnimMotion[i] = 0;
        pl->partAnimWeight[i] = 0;
    }
}

/*
 * Sets the state group and state of a player or a monster (obj), as
 * Player_SetState but without its checks or flags. A monster keeps no
 * previous state nor stateRand, and keeps its part overrides in weapon-group
 * states 0, 4 and 5 when the byte at Player.weaponType is 1, 5 or 10. For a
 * player with unk01E 0, the local player sends the change; another player
 * put in the idle state returns to the idle motion.
 */
void Player_ForceState(void *obj, s32 state, s32 sub)
{
    Player *pl = obj;
    if (pl->isEm == 0) {
        pl->prevStateGroup = pl->stateGroup;
        pl->prevState = pl->state;
    }
    pl->stateGroup = state;
    pl->state = sub;
    pl->step = 0;
    pl->stateVar0 = 0;
    pl->stateVar1 = 0;
    if (pl->isEm == 0) {
        pl->stateRand = Rand_Next(1);
    }
    if (pl->isEm == 0) {
        Player_ClearPartOverrides(pl);
    } else {
        switch (pl->weaponType) {
        case WEAPON_HEAVY_BOWGUN:
        case WEAPON_LIGHT_BOWGUN:
        case WEAPON_BOW:
            if (pl->stateGroup == PL_GROUP_WEAPON) {
                switch (pl->state) {
                case 0:
                case 4:
                case 5:
                    break;
                default:
                    Player_ClearPartOverrides(pl);
                    break;
                }
            } else {
                Player_ClearPartOverrides(pl);
            }
            break;
        default:
            Player_ClearPartOverrides(pl);
            break;
        }
    }
    if (pl->isEm == 0 && pl->stateGroup == PL_GROUP_REACT) {
        pl->unk417 = 0;
    }
    Player_UpdateMotionTime(pl, 0);
    pl->flags &= 0x100100;
    pl->stateFlags = 0;
    if (pl->isEm == 0 && pl->ownTask == 0) {
        if (Player_IsLocal(pl) == 1) {
            QSync_SendPlayer(pl, QSYNC_PL_STATE, 0);
            return;
        }
        if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_IDLE) != 0) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        }
    }
}

/*
 * Changes pl's state to group `state`, state `sub` (PL_GROUP_*, and the
 * group's enum in player.h): the old one goes to prevStateGroup/prevState,
 * the step, the scratch bytes, the motion speed and the flags not kept are
 * reset, and stateRand is drawn again. `flags` are PL_SET_*. The local
 * player sends the change (QSync_SendPlayer); another player put in the idle
 * state returns to the idle motion. While the local player moves to another
 * area, only an event state is taken.
 */
void Player_SetState(Player *pl, u16 state, u8 sub, u16 flags)
{
    u16 low;

    if (Player_IsLocal(pl) == 1 && pl->areaMoveState != PL_AREA_MOVE_NONE && state != PL_GROUP_EVENT) {
        return;
    }
    pl->prevStateGroup = pl->stateGroup;
    pl->prevState = pl->state;
    pl->stateGroup = state;
    pl->state = sub;
    pl->step = 0;
    pl->stateVar0 = 0;
    pl->stateVar1 = 0;
    if (flags & PL_SET_KEEP_RAND) {
        low = (u8)pl->stateRand;
        pl->stateRand = Rand_Next(1) & 0xFF00;
        pl->stateRand |= low;
    } else {
        pl->stateRand = Rand_Next(1);
    }
    pl->stateRequest = 0;
    pl->motionSpeed = 1.0f;
    if (!(flags & PL_SET_KEEP_PARTS)) {
        Player_ClearPartOverrides(pl);
        pl->aimWeight = 0;
    }
    Player_ResetAttackPercent(pl);
    if (pl->stateGroup == PL_GROUP_REACT) {
        pl->unk417 = 0;
    }
    Player_UpdateMotionTime(pl, 0);
    pl->flags &= 0x80100;
    pl->stateFlags = 0;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
    if (Player_IsLocal(pl) == 1) {
        if (!(flags & PL_SET_NO_SYNC)) {
            if (flags & PL_SET_SYNC_OFFER) {
                QSync_SendPlayer(pl, QSYNC_PL_HANDOVER, flags);
            } else if (flags & PL_SET_SYNC_ALT) {
                QSync_SendPlayer(pl, QSYNC_PL_STATE_AIM, flags);
            } else {
                QSync_SendPlayer(pl, QSYNC_PL_STATE, flags);
            }
        }
    } else if (Player_IsState(pl, PL_GROUP_ACTION, PL_ACT_IDLE) != 0) {
        if (!(flags & PL_SET_KEEP_PARTS)) {
            Player_ReturnToIdle(pl, 0, 4, 0);
        } else {
            Player_ReturnToIdleKeepParts(pl, 0, 4, 0);
        }
    }
}

/* Player_SetState, marked as a request (stateRequest); refused as there while the local player changes areas. */
void Player_RequestState(Player *pl, u16 state, u8 sub, u16 flags)
{
    if (Player_IsLocal(pl) != 1 || pl->areaMoveState == PL_AREA_MOVE_NONE || state == PL_GROUP_EVENT) {
        Player_SetState(pl, state, sub, flags);
        pl->stateRequest = 1;
    }
}

/*
 * Player_RequestState with PL_SET_KEEP_RAND: the low byte of stateRand
 * carries gQuest.drillCharm for the local player (0 for another).
 */
void Player_RequestStateSynced(Player *pl, u16 state, u8 sub, u16 flags)
{
    if (Player_IsLocal(pl) != 1 || pl->areaMoveState == PL_AREA_MOVE_NONE || state == PL_GROUP_EVENT) {
        if (Player_IsLocal(pl) == 1) {
            pl->stateRand = gQuest.drillCharm;
        } else {
            pl->stateRand = 0;
        }
        Player_SetState(pl, state, sub, flags | PL_SET_KEEP_RAND);
        pl->stateRequest = 1;
    }
}

/* Returns 1 when the player or monster obj is in group `state`, state `sub`. */
s16 Player_IsState(void *obj, u8 state, u8 sub)
{
    ObjHead *head = obj;
    if (head->stateGroup == state && head->state == sub) {
        return 1;
    }
    return 0;
}

/* Returns obj's flags masked by `flag`; with bit 31 set, its stateFlags (cleared at each state change) instead. */
u32 Player_TestFlag(void *obj, u32 flag)
{
    ObjHead *head = obj;
    if ((flag & 0x80000000) == 0) {
        return head->flags & flag;
    }
    return head->stateFlags & (flag & 0x7FFFFFFF);
}

/* Sets `flag` in obj's flags, or in its stateFlags with bit 31 set. */
void Player_SetFlag(void *obj, u32 flag)
{
    ObjHead *head = obj;
    if (!(flag & 0x80000000)) {
        head->flags |= flag;
    } else {
        head->stateFlags |= flag & 0x7FFFFFFF;
    }
}

/* Clears `flag` in obj's flags, or in its stateFlags with bit 31 set. */
void Player_ClearFlag(void *obj, u32 flag)
{
    ObjHead *head = obj;
    if (!(flag & 0x80000000)) {
        head->flags &= ~flag;
    } else {
        head->stateFlags &= ~(flag & 0x7FFFFFFF);
    }
}

/* Sets the player's posture (Player.action, PL_POSTURE_*) and nothing else. */
void Player_SetPostureOnly(Player *pl, u8 posture)
{
    pl->action = posture;
}

/* Returns obj's ObjHead.attackFlags bits (ATKF_*) masked by `mask`. */
s32 Player_TestFlags(void *obj, s32 mask)
{
    ObjHead *head = obj;
    return head->attackFlags & mask;
}

/* Returns obj's ObjHead.attackFlags bits (ATKF_*) masked by `mask`, then sets them. */
s32 Player_TestSetFlags(void *obj, s32 mask)
{
    ObjHead *head = obj;
    s32 ret = head->attackFlags & mask;

    head->attackFlags |= (u16)mask;
    return ret;
}

/*
 * The idle motion in the air (Player_ReturnToIdle with move mode 2): motion
 * 1017 from frame 8, blended over `blend` frames; `variant` picks unk2B4,
 * 2 or 6. Sets flag 1.
 */
void Player_SetAirIdle(Player *pl, s16 blend, s32 variant)
{
    pl->motionId[0] = 1017;
    if (variant == 0) {
        pl->unk2B4 = 2;
    } else {
        pl->unk2B4 = 6;
    }
    pl->motionBlend[0] = blend / 2;
    pl->motionBlend[1] = blend / 2;
    pl->motionFrame[0] = 8;
    pl->motionFrame[1] = 8;
    Player_SetFlag(pl, 1);
}

/*
 * Starts pl's idle motion pair (lower and upper layers), blended over
 * `blend` frames from `startFrame`: riding, the weapon drawn, tired (the
 * maximum stamina below 76), or by the area's temperature (AREA_TEMP_*)
 * unless a resistance skill covers it.
 */
void Player_StartIdleMotion(Player *pl, s32 blend, s32 startFrame)
{
    u16 lowerMotion;
    u16 upperMotion;

    if (pl->rideState != 0) {
        lowerMotion = 24;
        upperMotion = 124;
    } else if (pl->weaponOut != 0) {
        lowerMotion = 1001;
        upperMotion = 1101;
    } else if (pl->staminaMax < 76 /* tired */) {
        lowerMotion = 403;
        upperMotion = 503;
    } else {
        switch (Area_Temperature(pl->area)) {
        case AREA_TEMP_NORMAL:
        default:
            lowerMotion = 1;
            upperMotion = 101;
            break;
        case AREA_TEMP_HOT:
            if (Player_HasSkill(pl, SKILL_HEAT_RES) == 1 || Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
                lowerMotion = 1;
                upperMotion = 101;
            } else {
                lowerMotion = 21;
                upperMotion = 121;
            }
            break;
        case AREA_TEMP_VERY_HOT:
            if (Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
                lowerMotion = 1;
                upperMotion = 101;
            } else {
                lowerMotion = 21;
                upperMotion = 121;
            }
            break;
        case AREA_TEMP_COLD:
            if (Player_HasSkill(pl, SKILL_COLD_RES) == 1 || Player_HasSkill(pl, SKILL_COLD_RES2) == 1) {
                lowerMotion = 1;
                upperMotion = 101;
            } else {
                lowerMotion = 431;
                upperMotion = 531;
            }
            break;
        case AREA_TEMP_VERY_COLD:
            if (Player_HasSkill(pl, SKILL_COLD_RES2) == 1) {
                lowerMotion = 1;
                upperMotion = 101;
            } else {
                lowerMotion = 431;
                upperMotion = 531;
            }
            break;
        }
    }
    Player_SetMotion(pl, lowerMotion, blend, startFrame, 0);
    Player_SetMotion(pl, upperMotion, blend, startFrame, 1);
}

/*
 * Puts pl in the idle state (group 0, state 0) with the idle motions of
 * Player_StartIdleMotion, set to start at the next motion update; the local
 * player sends the change unless `quiet`.
 */
void Player_SetIdle(Player *pl, s32 blend, s32 startFrame, s32 quiet)
{
    if (pl->rideState != 0) {
        pl->motionId[0] = 24;
        pl->motionId[1] = 124;
    } else if (pl->weaponOut != 0) {
        pl->motionId[0] = 1001;
        pl->motionId[1] = 1101;
    } else if (pl->staminaMax < 76 /* tired */) {
        pl->motionId[0] = 403;
        pl->motionId[1] = 503;
    } else {
        switch (Area_Temperature(pl->area)) {
        case AREA_TEMP_NORMAL:
        default:
            pl->motionId[0] = 1;
            pl->motionId[1] = 101;
            break;
        case AREA_TEMP_HOT:
            if (Player_HasSkill(pl, SKILL_HEAT_RES) == 1 || Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
                pl->motionId[0] = 1;
                pl->motionId[1] = 101;
            } else {
                pl->motionId[0] = 21;
                pl->motionId[1] = 121;
            }
            break;
        case AREA_TEMP_VERY_HOT:
            if (Player_HasSkill(pl, SKILL_HEAT_RES2) == 1) {
                pl->motionId[0] = 1;
                pl->motionId[1] = 101;
            } else {
                pl->motionId[0] = 21;
                pl->motionId[1] = 121;
            }
            break;
        case AREA_TEMP_COLD:
            if (Player_HasSkill(pl, SKILL_COLD_RES) == 1 || Player_HasSkill(pl, SKILL_COLD_RES2) == 1) {
                pl->motionId[0] = 1;
                pl->motionId[1] = 101;
            } else {
                pl->motionId[0] = 431;
                pl->motionId[1] = 531;
            }
            break;
        case AREA_TEMP_VERY_COLD:
            if (Player_HasSkill(pl, SKILL_COLD_RES2) == 1) {
                pl->motionId[0] = 1;
                pl->motionId[1] = 101;
            } else {
                pl->motionId[0] = 431;
                pl->motionId[1] = 531;
            }
            break;
        }
    }
    pl->motionBlend[0] = blend / 2;
    pl->motionBlend[1] = blend / 2;
    pl->motionFrame[0] = startFrame;
    pl->motionFrame[1] = startFrame;
    pl->stateGroup = PL_GROUP_ACTION;
    pl->state = PL_ACT_IDLE;
    if (Player_IsLocal(pl) == 1 && quiet == 0) {
        QSync_SendPlayer(pl, QSYNC_PL_STATE, 0);
    }
}

static void Player_ResetActionStub(Player *pl)
{
}

/* Clears the per-state values a player's state leaves behind: the motion
 * start, the hit window, the timers and flags (all but 0x100). */
void Player_ClearActionState(Player *pl)
{
    *(u32 *)&pl->unk004 = 0;
    pl->unk2B4 = 0;
    *(u16 *)pl->motionStarted = 0;
    pl->unk3A0 = 0;
    pl->motionTime = 0;
    pl->hitDelay = 0;
    pl->hitFrames = 0;
    pl->unk4BA = 0;
    pl->flags &= 0x100;
    pl->stateFlags = 0;
    pl->motion[0].unk04 = pl->index;
    pl->motion[1].unk04 = pl->index;
    pl->unk430 = 0;
    pl->unk2B8 = 0;
    pl->noHitTimer = 0;
    pl->unk434 = 0;
    pl->timer = 0;
    pl->unk392 = 0;
    pl->unk4C4 = 0;
    pl->unk4C1 = 0;
    pl->unk3F8 = 0;
    pl->unk417 = 0;
    if (pl->weaponType != WEAPON_GUNLANCE) {
        pl->weaponTimer = 0;
    }
    pl->carry = 0;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
}

/* Stops the player's movement (velocity, acceleration) and clears its flags
 * but 0x104. */
static void Player_ClearMotion(Player *pl)
{
    pl->vel[0] = 0.0f;
    pl->vel[1] = 0.0f;
    pl->vel[2] = 0.0f;
    pl->accel[0] = 0.0f;
    pl->accel[1] = 0.0f;
    pl->accel[2] = 0.0f;
    pl->flags &= 0x104;
}

/*
 * Ends the player's state and returns it to idle: the ground idle
 * (Player_SetIdle with blend and startFrame; quiet, unsent, with bit 0x80
 * of posture), or the air idle when posture is PL_POSTURE_AIRBORNE
 * (Player_SetAirIdle, startFrame as its variant). The part overrides are
 * cleared too. Every caller passes PL_POSTURE_STANDING. A posture of 15
 * would leave the motion as it is; nothing here sets it.
 */
void Player_ReturnToIdle(Player *pl, s32 posture, s32 blend, s32 startFrame)
{
    s32 quiet = posture & 0x80;

    Player_ClearActionState(pl);
    Player_ResetActionStub(pl);
    Player_ClearPartOverrides(pl);
    pl->freeState = 1;
    if ((posture & ~0x80) != PL_POSTURE_AIRBORNE) {
        Player_ClearMotion(pl);
        pl->action = PL_POSTURE_STANDING;
    } else {
        Player_SetPostureOnly(pl, PL_POSTURE_AIRBORNE);
    }
    switch (pl->action) {
    default:
        Player_SetIdle(pl, blend, startFrame, quiet);
        break;
    case PL_POSTURE_AIRBORNE:
        Player_SetAirIdle(pl, blend, startFrame);
        break;
    case 15:
        break;
    }
    Player_UpdateMotionTime(pl, 0);
}

/* Player_ReturnToIdle without clearing the part overrides. */
void Player_ReturnToIdleKeepParts(Player *pl, s32 posture, s32 blend, s32 startFrame)
{
    s32 quiet = posture & 0x80;

    Player_ClearActionState(pl);
    Player_ResetActionStub(pl);
    if ((posture & ~0x80) != PL_POSTURE_AIRBORNE) {
        Player_ClearMotion(pl);
        pl->action = PL_POSTURE_STANDING;
    } else {
        Player_SetPostureOnly(pl, PL_POSTURE_AIRBORNE);
    }
    switch (pl->action) {
    default:
        Player_SetIdle(pl, blend, startFrame, quiet);
        break;
    case PL_POSTURE_AIRBORNE:
        Player_SetAirIdle(pl, blend, startFrame);
        break;
    case 15:
        break;
    }
    Player_UpdateMotionTime(pl, 0);
}

/* Copies the pad input into the local player's input block (Player_CopyPadInput). */
void Player_UpdateInput(void)
{
    if (Game_IsOnline() == 0) {
        Player_CopyPadInput(0);
    } else {
        Player_CopyPadInput(gScene.localPlayer);
    }
}

/*
 * Copies the pad input into the player's input block. The pad port is the
 * player number (port 0 when Game_IsOnline() is set). The input locks (menus,
 * cutscenes and the like) then clear the buttons, keeping only the directions of
 * stickDir when only Hud_InputBlocked() locks.
 */
void Player_CopyPadInput(s32 playerNo)
{
    Player *player = &gPlayers[playerNo];
    PlayerInput *padIn = &player->input;
    u16 port;

    if (Game_IsOnline() == 0) {
        port = playerNo;
    } else {
        port = 0;
    }
    port &= 1;

    padIn->stickDir = gPadInput.stickDir[port].cur;
    padIn->prevStickDir = gPadInput.stickDir[port].prev;
    padIn->prevStickDirPressed = padIn->stickDirPressed;
    padIn->stickDirPressed = padIn->stickDir & ~padIn->prevStickDir;
    padIn->stickAngle = gPadInput.stickAngle[port].left;
    padIn->rightStickAngle = gPadInput.stickAngle[port].right;
    padIn->stickTilt = gPadInput.stickTilt[port].left;
    padIn->rightStickTilt = gPadInput.stickTilt[port].right;

    padIn->held = gPadInput.buttons[port].cur;
    padIn->prevHeld = gPadInput.buttons[port].prev;
    padIn->prevPressed = padIn->pressed;
    padIn->pressed = padIn->held & ~padIn->prevHeld;
    padIn->changed = (padIn->held ^ padIn->prevHeld) | (padIn->held ^ gPadInput.buttons[port].cur);
    padIn->held2 = padIn->held;
    padIn->prevHeld2 = padIn->prevHeld;
    padIn->pressed2 = padIn->pressed;
    padIn->prevPressed2 = padIn->prevPressed;

    if (gScene.eventRunning != 0 && Quest_IsVersus() == 1) {
        Mem_Fill(padIn, 0, sizeof(PlayerInput));
    }

    if (Hud_InputBlocked() == 0 && player->supplyBoxOpen == 0 && player->deliveryMenu == 0 && player->deliveryOpen == 0 &&
        gQuest.inputLock == 0) {
        if (player->itemSelect != 0) {
            padIn->held &= PAD_R1 | PAD_R2;
            padIn->prevHeld &= PAD_R1 | PAD_R2;
            padIn->pressed &= PAD_R1 | PAD_R2;
            padIn->prevPressed &= PAD_R1 | PAD_R2;
        }
    } else {
        if (player->supplyBoxOpen == 0 && player->deliveryOpen == 0 && player->deliveryMenu == 0 && gQuest.inputLock == 0) {
            padIn->stickDir &= PAD_DPAD;
            padIn->prevStickDir &= PAD_DPAD;
            padIn->prevStickDirPressed &= PAD_DPAD;
            padIn->stickDirPressed &= PAD_DPAD;
        } else {
            padIn->stickDir = 0;
            padIn->prevStickDir = 0;
            padIn->prevStickDirPressed = 0;
            padIn->stickDirPressed = 0;
        }
        padIn->rightStickAngle = 0;
        padIn->rightStickTilt = 0;
        padIn->held = 0;
        padIn->prevHeld = 0;
        padIn->pressed = 0;
        padIn->prevPressed = 0;
        padIn->held2 = 0;
        padIn->prevHeld2 = 0;
        padIn->pressed2 = 0;
        padIn->prevPressed2 = 0;
        padIn->changed = 0;
    }
}

/* Plays sound seId at the player's position. */
void Player_PlaySeAtSelf(u8 *obj, s32 seId)
{
    Player_PlaySe((Player *) obj, seId, 0, ((Player *) obj)->pos, 1, 0);
}
