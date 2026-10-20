/*
 * Player helpers: clearing a player's action state and returning it to the
 * idle motion (Player_ReturnToIdle, called by most of the player's states
 * when they end), and copying the pad input into the local player's input
 * block every frame (Player_CopyPadInput), with the menus' input locks.
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


void Mem_Fill(void *dst, u8 c, s32 n);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Hud_InputBlocked(void);        /* input lock */
s32 Game_IsOnline(void);
s32 Quest_IsVersus(void);
void Player_ClearPartOverrides(Player *p);
void Player_SetPostureOnly(Player *p, u8 posture);
void Player_SetAirIdle(Player *p, s16 a, s32 b);
void Player_SetIdle(Player *p, s32 a, s32 b, s32 c);
void Player_UpdateMotionTime(Player *p, s16 step);
void Player_CopyPadInput(s32 playerNo);

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
 * cleared too. Every caller passes PL_POSTURE_STANDING. A posture of 0xF
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
    case 0xF:
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
    case 0xF:
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
