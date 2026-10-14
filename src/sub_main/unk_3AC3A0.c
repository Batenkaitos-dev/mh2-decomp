/*
 * Player helpers: resetting a player's action state, and copying the pad
 * input into each player's input block every frame.
 *
 * func_003AC3A0 and Player_ClearMotion are static: they are only called from this
 * file, and MWCC uses the register usage of static functions defined earlier
 * in the file. After calling the empty func_003AC3A0 it knows a0 still holds
 * the player, so Player_BeginStateKeepParts passes the player to the next calls without
 * setting a0 again.
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
void Player_SetAction(Player *p, u8 action);          /* p->action = action */
void func_003ABE90(Player *p, s16 a, s32 b);
void Player_SetIdle(Player *p, s32 a, s32 b, s32 c);
void Player_UpdateMotionTime(Player *p, s16 step);
void Player_CopyPadInput(s32 playerNo);

static void func_003AC3A0(Player *p)
{
}

void Player_ClearActionState(Player *p)
{
    *(u32 *)&p->unk004 = 0;
    p->unk2B4 = 0;
    *(u16 *)p->motionStarted = 0;
    p->unk3A0 = 0;
    p->motionTime = 0;
    p->hitDelay = 0;
    p->hitFrames = 0;
    p->unk4BA = 0;
    p->flags &= 0x100;
    p->stateFlags = 0;
    p->motion[0].unk04 = p->index;
    p->motion[1].unk04 = p->index;
    p->unk430 = 0;
    p->unk2B8 = 0;
    p->noHitTimer = 0;
    p->unk434 = 0;
    p->timer = 0;
    p->unk392 = 0;
    p->unk4C4 = 0;
    p->unk4C1 = 0;
    p->unk3F8 = 0;
    p->unk417 = 0;
    if (p->weaponType != 9) {
        p->weaponTimer = 0;
    }
    p->carry = 0;
    p->motion[0].step = 2.0f;
    p->motion[1].step = 2.0f;
}

static void Player_ClearMotion(Player *p)
{
    p->vel[0] = 0.0f;
    p->vel[1] = 0.0f;
    p->vel[2] = 0.0f;
    p->accel[0] = 0.0f;
    p->accel[1] = 0.0f;
    p->accel[2] = 0.0f;
    p->flags &= 0x104;
}

void Player_BeginState(Player *p, s32 action, s32 a2, s32 a3)
{
    s32 flag80 = action & 0x80;

    Player_ClearActionState(p);
    func_003AC3A0(p);
    Player_ClearPartOverrides(p);
    p->freeState = 1;
    if ((action & ~0x80) != 2) {
        Player_ClearMotion(p);
        p->action = 0;
    } else {
        Player_SetAction(p, 2);
    }
    switch (p->action) {
    default:
        Player_SetIdle(p, a2, a3, flag80);
        break;
    case 2:
        func_003ABE90(p, a2, a3);
        break;
    case 0xF:
        break;
    }
    Player_UpdateMotionTime(p, 0);
}

void Player_BeginStateKeepParts(Player *p, s32 action, s32 a2, s32 a3)
{
    s32 flag80 = action & 0x80;

    Player_ClearActionState(p);
    func_003AC3A0(p);
    if ((action & ~0x80) != 2) {
        Player_ClearMotion(p);
        p->action = 0;
    } else {
        Player_SetAction(p, 2);
    }
    switch (p->action) {
    default:
        Player_SetIdle(p, a2, a3, flag80);
        break;
    case 2:
        func_003ABE90(p, a2, a3);
        break;
    case 0xF:
        break;
    }
    Player_UpdateMotionTime(p, 0);
}

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
    PlayerInput *in = &player->input;
    u16 port;

    if (Game_IsOnline() == 0) {
        port = playerNo;
    } else {
        port = 0;
    }
    port &= 1;

    in->stickDir = gPadInput.stickDir[port].cur;
    in->prevStickDir = gPadInput.stickDir[port].prev;
    in->prevStickDirPressed = in->stickDirPressed;
    in->stickDirPressed = in->stickDir & ~in->prevStickDir;
    in->stickAngle = gPadInput.stickAngle[port].cur;
    in->prevStickAngle = gPadInput.stickAngle[port].prev;
    in->stickTilt = gPadInput.stickTilt[port].cur;
    in->prevStickTilt = gPadInput.stickTilt[port].prev;

    in->held = gPadInput.buttons[port].cur;
    in->prevHeld = gPadInput.buttons[port].prev;
    in->prevPressed = in->pressed;
    in->pressed = in->held & ~in->prevHeld;
    in->changed = (in->held ^ in->prevHeld) | (in->held ^ gPadInput.buttons[port].cur);
    in->held2 = in->held;
    in->prevHeld2 = in->prevHeld;
    in->pressed2 = in->pressed;
    in->prevPressed2 = in->prevPressed;

    if (gScene.eventRunning != 0 && Quest_IsVersus() == 1) {
        Mem_Fill(in, 0, sizeof(PlayerInput));
    }

    if (Hud_InputBlocked() == 0 && player->supplyBoxOpen == 0 && player->deliveryMenu == 0 && player->deliveryOpen == 0 &&
        gQuest.inputLock == 0) {
        if (player->itemSelect != 0) {
            in->held &= 0x90;
            in->prevHeld &= 0x90;
            in->pressed &= 0x90;
            in->prevPressed &= 0x90;
        }
    } else {
        if (player->supplyBoxOpen == 0 && player->deliveryOpen == 0 && player->deliveryMenu == 0 && gQuest.inputLock == 0) {
            in->stickDir &= 0x3C00;
            in->prevStickDir &= 0x3C00;
            in->prevStickDirPressed &= 0x3C00;
            in->stickDirPressed &= 0x3C00;
        } else {
            in->stickDir = 0;
            in->prevStickDir = 0;
            in->prevStickDirPressed = 0;
            in->stickDirPressed = 0;
        }
        in->prevStickAngle = 0;
        in->prevStickTilt = 0;
        in->held = 0;
        in->prevHeld = 0;
        in->pressed = 0;
        in->prevPressed = 0;
        in->held2 = 0;
        in->prevHeld2 = 0;
        in->pressed2 = 0;
        in->prevPressed2 = 0;
        in->changed = 0;
    }
}

void Player_PlaySeAtSelf(u8 *a0, s32 a1)
{
    Player_PlaySe((Player *) a0, a1, 0, a0 + 0xAC, 1, 0);
}
