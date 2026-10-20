/*
 * The lobby's town players: the local hunter and the other members shown in
 * the village, run each frame by TownPc_UpdateAll.
 *
 * - The actor core that town players and townspeople (Em slots) share
 *   through ObjHead: the timers, state changes (Actor_SetState*), the
 *   return to idle, the posture and flag helpers and the move-to steps.
 *   They are the town's copies of the field's Player_SetState,
 *   Player_ReturnToIdle and Player_SetPosture. A local player's state
 *   change is sent to the other members (TownMsg_SendStateChange).
 * - The pad input, the move kind (walk, run) and the turning.
 * - The area's action spots (ActionSpot lists, gAreaActionSpots) and
 *   whether one is usable; the lock-on that cycles through the townspeople
 *   and the other players (LockOn_*); the house-owner tests (MyHouse_*).
 * - The state handlers of group 0 (enum TownPcState, TownPc_State*): idle,
 *   walk and run, the item hand-over between two players, the bed, the
 *   drunk idle, the table seats with their d-pad poses and drinks, the
 *   kitchen's order and the meal's three ways of rising. Their dispatcher,
 *   TownPc_RunActionState, and the action checks (TownPc_HandleInput) follow in the
 *   next files.
 * - The walks to an action spot: the player's own, a contest seat, a table
 *   seat taken with TOWN_MODE_SEAT_WAIT.
 *
 * The data: the action-spot lists of each area set, the gesture states by
 * request (gTownGestureStates) and the sleep effect's offset.
 */
#include "common.h"
#include "lobby_menu.h"
#include "quest.h"
#include "player.h"
#include "m2c_macros.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "lobby_state.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "objhead.h"
#include "lobby_npc.h"
#include "area.h"

/* Built at optimization level 4: TownPc_UpdateAll matches only at 4, the others at 4 too
 * (TownPc_StateGrabbed too). One unit: the functions share the helpers
 * Actor_ReturnToIdle, Actor_SetPosture and Actor_SetState throughout. */
#pragma optimization_level 4
s32 Game_IsOnline(void);
s32 TownPc_IsFree(Player *);
u16 Camera_GetYaw(void);
int abs(int);

/* No prototype: Actor_NeedsStateSend calls it with its loop counter still in a1. */
/* unit's own declaration: Area_HasTownPcs ignores a1, which holds var_a1 at Actor_NeedsStateSend (the player loop count, 10) */
s32 Area_HasTownPcs();
void TownPc_ReadPad(s32 pl);
extern LobbyMenu *gLobbyMenu;
void Actor_SetStateFlags(Player *obj, u8 state, u8 sub, u16 flags);

/*
 * Reads the local player's pad into its input block, unless the lobby menu
 * is redrawing this frame.
 */
void TownPc_UpdateInput(void)
{
    if (gLobbyMenu->redraw == 0) {
        TownPc_ReadPad(gScene.localPlayer);
    }
}

u32 Player_TestFlag(void *obj, u32 flag);

s32 Hud_InputBlocked(void);
s32 Hud_TownMenuOpen(void);
void Player_RunInputHook(Player *p);

/*
 * Copies the pad (buttons, left and right stick) into player playerNo's
 * input block, as sub_main's Player_CopyPadInput does. The stick tilt
 * reads 0 while the town menu is open; while input is blocked or the
 * supply box is open the buttons are cleared and only the D-pad directions
 * of the stick are kept (none with the supply box open); with the item
 * selector open the buttons are cleared but the second copy (held2 and the
 * rest) keeps them.
 */
/*
 * Copies the pad (buttons, left and right stick) into player playerNo's
 * input block, as sub_main's Player_CopyPadInput does. The stick tilt
 * reads 0 while the town menu is open; while input is blocked or the
 * supply box is open the buttons are cleared and only the D-pad directions
 * of the stick are kept (none with the supply box open); with the item
 * selector open the buttons are cleared but the second copy (held2 and the
 * rest) keeps them.
 */
void TownPc_ReadPad(s32 playerNo)
{
    Player *player = &gPlayers[playerNo];
    PlayerInput *padInput = &player->input;

    padInput->stickDir = gPadInput.stickDir[0].cur;
    padInput->prevStickDir = gPadInput.stickDir[0].prev;
    padInput->prevStickDirPressed = padInput->stickDirPressed;
    padInput->stickDirPressed = padInput->stickDir & ~padInput->prevStickDir;
    padInput->stickAngle = gPadInput.stickAngle[0].left;
    padInput->rightStickAngle = gPadInput.stickAngle[0].right;
    if (Hud_TownMenuOpen() == 0) {
        padInput->stickTilt = gPadInput.stickTilt[0].left;
        padInput->rightStickTilt = gPadInput.stickTilt[0].right;
    } else {
        padInput->stickTilt = 0;
        padInput->rightStickTilt = 0;
    }
    if (Hud_InputBlocked() == 0 && player->supplyBoxOpen == 0) {
        padInput->held = gPadInput.buttons[0].cur;
        padInput->prevHeld = gPadInput.buttons[0].prev;
        padInput->prevPressed = padInput->pressed;
        padInput->pressed = padInput->held & ~padInput->prevHeld;
        padInput->changed = (padInput->held ^ padInput->prevHeld) | (padInput->held ^ gPadInput.buttons[0].cur);
        padInput->held2 = padInput->held;
        padInput->prevHeld2 = padInput->prevHeld;
        padInput->pressed2 = padInput->pressed;
        padInput->prevPressed2 = padInput->prevPressed;
        if (player->itemSelect != 0) {
            padInput->held = 0;
            padInput->prevHeld = 0;
            padInput->pressed = 0;
            padInput->prevPressed = 0;
        }
        Player_RunInputHook(player);
    } else {
        if (player->supplyBoxOpen == 0) {
            padInput->stickDir &= 0x3C00;
            padInput->prevStickDir &= 0x3C00;
            padInput->prevStickDirPressed &= 0x3C00;
            padInput->stickDirPressed &= 0x3C00;
        } else {
            padInput->stickDir = 0;
            padInput->prevStickDir = 0;
            padInput->prevStickDirPressed = 0;
            padInput->stickDirPressed = 0;
        }
        padInput->rightStickAngle = 0;
        padInput->rightStickTilt = 0;
        padInput->held = 0;
        padInput->prevHeld = 0;
        padInput->pressed = 0;
        padInput->prevPressed = 0;
        padInput->held2 = 0;
        padInput->prevHeld2 = 0;
        padInput->pressed2 = 0;
        padInput->prevPressed2 = 0;
        padInput->changed = 0;
    }
}

f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
s32 Player_UpdateMotion(Player *p);

/*
 * Builds the rotation matrix from the angles, starts the two motion layers
 * that have not been started yet, and advances the motion.
 */
void Actor_StartMotions(Player *pl)
{
    Mtx_FromAnglesXYZ(&pl->rot[0], &pl->rotMtx[0]);
    if (pl->motionStarted[0] == 0) {
        pl->motionStarted[0]++;
        Player_BeginLayerMotion(pl, (u16)pl->motionFrame[0], pl->motionBlend[0], 0);
    }
    if (pl->motionStarted[1] == 0) {
        pl->motionStarted[1]++;
        Player_BeginLayerMotion(pl, (u16)pl->motionFrame[1], pl->motionBlend[1], 1);
    }
    Player_UpdateMotion(pl);
}

/*
 * Each frame, unless in hit stop: counts the hit-window time up (by 2
 * while PLF_HIT_WINDOW or flag bit 0x400000 is set), advances motionTime (wrapping 100000
 * back to 10000) and counts down the short timers (dash delay, offer
 * notice, weapon sound mute, the shadow hide and, for players, the fishing
 * timer). The townspeople's update calls it too, with their Em.
 */
void Actor_TickTimers(Player *obj)
{
    s16 dashLeft;
    s8 fishTimer;
    u8 countdown;
    u8 shadowLeft;
    u8 offerNoticeTimer;
    u8 weaponSeMute;

    if (obj->hitStop == 0) {
        if (Player_TestFlag((Player *) obj, 0x400001U) != 0) {
            obj->unk3A0 = (u16) (obj->unk3A0 + 2);
        } else {
            obj->unk3A0 = 0U;
        }
        obj->motionTime = (s32) (obj->motionTime + 2);
        if ((s32)obj->motionTime >= 0x186A0) {
            obj->motionTime = 0x2710;
        }
        countdown = obj->unk434;
        if (countdown != 0) {
            obj->unk434 = (u8) (countdown - 1);
        }
        shadowLeft = obj->hideShadow;
        if (shadowLeft != 0) {
            obj->hideShadow = (u8) (shadowLeft - 1);
        }
        dashLeft = obj->dashDelay;
        if (dashLeft != 0) {
            obj->dashDelay = (s16) (dashLeft - 1);
        }
        offerNoticeTimer = obj->offerNoticeTimer;
        if (offerNoticeTimer != 0) {
            obj->offerNoticeTimer = (u8) (offerNoticeTimer - 1);
        }
        weaponSeMute = obj->weaponSeMute;
        if (weaponSeMute != 0) {
            obj->weaponSeMute = (u8) (weaponSeMute - 1);
        }
        if (obj->isEm == 0) {
            fishTimer = obj->fishTimer;
            if (fishTimer != 0) {
                obj->fishTimer = (s8) (fishTimer - 1);
            }
        }
    }
}

/*
 * Actor_SetStateFlags, then marks the state request so that TownPc_Update
 * runs the new state in the same frame.
 */
void Actor_SetStateNow(Player *pl, u8 group, u8 newState, u16 flags)
{
    Actor_SetStateFlags(pl, group, newState, flags);
    pl->stateRequest = 1;
}

u16 Rand_Next(s32 stream);
s32 Actor_NeedsStateSend(void *arg0);
void Actor_ReturnToIdle(void *obj, s32 a1, s32 n, s32 m);
static void Actor_RestartMotionTime(void *obj, s16 flag);
s16 Actor_IsState(void *arg0, s32 arg1, s32 arg2);
void TownMsg_SendStateChange(void *a0);

/*
 * Puts a player in state (group, state): keeps the previous state, clears
 * the step and state values, draws a new stateRand, clears the part-motion
 * overrides unless flags has PL_SET_KEEP_PARTS, restarts the motion time
 * and clears the per-state flags. Another member's player that falls back
 * to the idle state gets the idle motion; the local player's change is
 * sent to the others when Actor_NeedsStateSend says so.
 */
/*
 * Puts a player in state (group, state): keeps the previous state, clears
 * the step and state values, draws a new stateRand, clears the part-motion
 * overrides unless flags has PL_SET_KEEP_PARTS, restarts the motion time
 * and clears the per-state flags. Another member's player that falls back
 * to the idle state gets the idle motion; the local player's change is
 * sent to the others when Actor_NeedsStateSend says so.
 */
void Actor_SetStateFlags(Player *pl, u8 group, u8 newState, u16 flags)
{
    s32 i;

    pl->prevStateGroup = pl->stateGroup;
    pl->prevState = pl->state;
    pl->stateGroup = group;
    pl->state = newState;
    pl->step = 0;
    pl->stateVar0 = 0;
    pl->stateVar1 = 0;
    pl->stateRand = Rand_Next(1);
    pl->stateRequest = 0;
    if (!(flags & 1)) {
        for (i = 0; i < 4; i++) {
            pl->partAnimOn[i] = 0;
            pl->partAnimMotion[i] = 0;
            pl->partAnimWeight[i] = 0;
        }
    }
    Actor_RestartMotionTime(pl, 0);
    pl->flags &= 0x80100;
    pl->stateFlags = 0;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
    if (pl->index != gScene.localPlayer) {
        if (Actor_IsState(pl, TOWN_PC_GROUP_ACTION, 0) == 0) {
            return;
        }
        if (pl->motionId[0] != 1) {
            Actor_ReturnToIdle(pl, 0, 4, 0);
        }
    }
    if (Actor_NeedsStateSend(pl) == 1) {
        TownMsg_SendStateChange(pl);
    }
}

/*
 * Returns 1 when a state change of obj must be sent to the other members:
 * online, obj is the local player run by the town code, and
 * Area_HasTownPcs allows the current area; else 0. It also renumbers
 * gPlayers[i].index on the way.
 */
s32 Actor_NeedsStateSend(void *obj)
{
    s8 i;
    ObjHead *head = obj;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    for (i = 0; i < 10; i++) {
        gPlayers[i].index = i;
    }
    if ((head->index == gScene.localPlayer) && (head->ownTask == 0)) {
        return Area_HasTownPcs(gScene.area, i);
    }
    return 0;
}

void Actor_ClearStateWork(void *arg0);
void Actor_StopMoving(void *arg0);
void Actor_StartIdle(void *obj, s32 n, s32 m);

/*
 * Returns an actor to the idle state: clears its state work and velocity,
 * stands it up and starts the idle motion with the given blend and start
 * frame (Actor_StartIdle). The posture argument is ignored. The town's
 * copy of Player_ReturnToIdle.
 */
void Actor_ReturnToIdle(void *obj, s32 posture, s32 blend, s32 startFrame)
{
    ObjHead *head = obj;
    Actor_ClearStateWork(obj);
    Actor_StopMoving(obj);
    head->action = PL_POSTURE_STANDING;
    switch (head->action) {
    default:
        Actor_StartIdle(obj, blend, startFrame);
        break;
    case 0xF:
        break;
    }
    Actor_RestartMotionTime(obj, 0);
}

/*
 * Clears the per-state values a state leaves behind: step and state
 * values, motion start, hit window and timers, flags but 0x100, the part-
 * motion overrides. The town's copy of Player_ClearActionState; it clears
 * the weapon timer whatever the weapon.
 */
void Actor_ClearStateWork(void *obj)
{
    ObjHead *head = obj;
    *(s32 *)&head->unk004 = 0;
    head->unk2B4 = 0;
    *(s16 *)head->motionStarted = 0;
    head->unk3A0 = 0;
    head->motionTime = 0;
    head->hitDelay = 0;
    head->hitFrames = 0;
    head->unk4BA = 0;
    head->flags = (s32) (head->flags & 0x100);
    head->stateFlags = 0;
    head->motion[0].unk04 = (s32) head->index;
    head->motion[1].unk04 = (s32) head->index;
    head->unk430 = 0;
    head->unk2B8 = 0;
    head->noHitTimer = 0;
    head->unk434 = 0;
    head->timer = 0;
    head->unk392 = 0;
    head->unk4C4 = 0;
    head->unk4C1 = 0;
    head->unk3F8 = 0;
    head->unk417 = 0;
    ((Player *)obj)->weaponTimer = 0;
    head->carry = 0;
    head->motion[0].step = 2.0f;
    head->motion[1].step = 2.0f;
    head->partAnimOn[0] = 0;
    head->partAnimMotion[0] = 0;
    head->partAnimWeight[0] = 0;
    head->partAnimOn[1] = 0;
    head->partAnimMotion[1] = 0;
    head->partAnimWeight[1] = 0;
    head->partAnimOn[2] = 0;
    head->partAnimMotion[2] = 0;
    head->partAnimWeight[2] = 0;
    head->partAnimOn[3] = 0;
    head->partAnimMotion[3] = 0;
    head->partAnimWeight[3] = 0;
}

/*
 * Stops the actor's movement (velocity and acceleration) and clears its
 * flags but 0x104.
 */
void Actor_StopMoving(void *obj)
{
    ObjHead *head = obj;
    head->vel[0] = 0;
    head->vel[1] = 0;
    head->vel[2] = 0;
    head->accel[0] = 0;
    head->accel[1] = 0;
    head->accel[2] = 0;
    head->flags = (s32) (head->flags & 0x104);
}


/*
 * Starts the idle motions (PL_MOT_IDLE, and motion 0x65 on the second
 * layer) with the given blend and start frame, enters the idle state
 * (group 0, state 1) and sends the change when Actor_NeedsStateSend says
 * so.
 */
void Actor_StartIdle(void *obj, s32 blend, s32 startFrame)
{
    ObjHead *head = obj;
    head->motionFrame[0] = startFrame;
    head->motionFrame[1] = startFrame;
    head->motionId[0] = 1;
    head->motionId[1] = 0x65;
    head->motionBlend[0] = blend / 2;
    head->motionBlend[1] = blend / 2;
    head->stateGroup = 0;
    head->state = 1;
    if (Actor_NeedsStateSend(obj) != 0) {
        TownMsg_SendStateChange(obj);
    }
}

/*
 * Sets motionTime to the motion's start frame less its blend; with
 * addHitDelay set, also moves hitDelay by the difference. Declared static:
 * so the compiler lets Actor_SetState keep the object in the same register
 * across the call, as the game's code does.
 */
static void Actor_RestartMotionTime(void *obj, s16 addHitDelay)
{
    ObjHead *head = obj;
    s16 blend = head->motionBlend[0];
    s16 startFrame = head->motionFrame[0];

    head->motionTime = 0;
    if (blend < 0) {
        blend = -blend;
    }
    head->motionTime += startFrame - blend;
    if (addHitDelay != 0) {
        if (blend != 0) {
            blend--;
        }
        head->hitDelay += (u8)(blend - startFrame);
    }
}

/* Returns 1 when the actor is in state (group, state), else 0. */
s16 Actor_IsState(void *obj, s32 group, s32 state)
{
    ObjHead *head = obj;
    if ((head->stateGroup == (group & 0xFF)) && (head->state == (state & 0xFF))) {
        return 1;
    }
    return 0;
}


s32 Player_IsLocal(void *obj);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Actor_SetPosture(void *a, u16 mode, s32 b, s32 c);
void Actor_SetState(void *, s32, s32);
void TownMsg_SendStateNow(Player *p);
void CamFocus_End(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
u8 TownPc_MoveKind(Player *a);
u16 TownPc_InputYaw(Player *, s32);
void Actor_SetFlag(void *, u32);
void TownPc_StartRequestedOffer(Player *pl);
void TownPc_CheckAction(Player *);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_UpdateMotionTime(Player *p, s16 step);
s32 ItemGet_Reserve(s32 pl, u16 *gotItem);
s32 Game_IsOnline(void);
void TownMsg_SendOfferEnd(void);
extern s32 gTownGestureStates[];
u8 Area_GetSetIndex(u16 area);
extern s32 gAreaActionSpots[];


/*
 * How the player moves this frame (TOWN_MOVE_*): not at all, a walk (the
 * stick tilted PL_STICK_HELD or more), a walk with the stick fully tilted
 * (0x78 or more), or a dash (PAD_R2 held, standing, the weapon sheathed,
 * while moving). The local player is read from its stick (TOWN_MOVE_NONE
 * while the town menu is open), the others from their state.
 */
u8 TownPc_MoveKind(Player *pl)
{
    u8 kind = TOWN_MOVE_NONE;
    u16 tilt;

    if (gTownMenuOpen == 1 && pl->index == gScene.localPlayer) {
        return TOWN_MOVE_NONE;
    }
    if (gScene.localPlayer == pl->index) {
        tilt = pl->input.stickTilt;
        if (tilt >= 0x78) {
            kind = TOWN_MOVE_WALK_FULL;
        } else if (tilt >= 0x55) {
            kind = TOWN_MOVE_WALK;
        } else if (tilt >= PL_STICK_HELD) {
            kind = TOWN_MOVE_WALK;
        }
    } else if (pl->state == TOWN_PC_IDLE || pl->state == TOWN_PC_DRUNK_IDLE) {
        kind = TOWN_MOVE_NONE;
    } else if (pl->state == TOWN_PC_WALK || pl->state == TOWN_PC_DRUNK_WALK) {
        kind = TOWN_MOVE_WALK;
    } else {
        kind = TOWN_MOVE_DASH;
    }
    if (pl->action == 0 && pl->weaponOut == 0 && (pl->input.held & PAD_R2) && kind != TOWN_MOVE_NONE
        && (Actor_IsState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SEAT_POSE_LEFT) == 0 || pl->dashDelay == 0)) {
        return TOWN_MOVE_DASH;
    }
    return kind;
}

/*
 * Returns the world heading the player's stick points to: the stick's
 * angle (stick 0 left, 1 right) plus the camera's yaw.
 */
u16 TownPc_InputYaw(Player *pl, s32 stick)
{
    u16 yaw = Camera_GetYaw();
    u16 stickYaw = (&pl->input.stickAngle)[stick];

    return stickYaw + yaw;
}

void Actor_ClearFlag(void *obj, u32 f);

/*
 * Sets the actor's posture (crouched and airborne kept, anything else
 * standing), sets PLF_CAM_FOLLOW unless the posture has
 * PL_POSTURE_NO_CAM_FOLLOW, and clears flags 1 and 2. The town's copy of
 * Player_SetPosture; its last two arguments are ignored.
 */
/*
 * Sets the actor's posture (crouched and airborne kept, anything else
 * standing), sets PLF_CAM_FOLLOW unless the posture has
 * PL_POSTURE_NO_CAM_FOLLOW, and clears flags 1 and 2. The town's copy of
 * Player_SetPosture; its last two arguments are ignored.
 */
void Actor_SetPosture(void *obj, u16 posture, s32 setFlag1, s32 setFlag2)
{
    s32 postureBits = posture;
    ObjHead *head = obj;

    switch ((u8)postureBits) {
    default:
        head->action = PL_POSTURE_STANDING;
        break;
    case 1:
        head->action = PL_POSTURE_CROUCHED;
        break;
    case 2:
        head->action = PL_POSTURE_AIRBORNE;
        break;
    }
    if (postureBits & 0x8000) {
        Actor_ClearFlag(obj, 8);
    } else {
        Actor_SetFlag(obj, 8);
    }
    Actor_ClearFlag(obj, 1);
    Actor_ClearFlag(obj, 2);
}

f32 Vec_Distance(f32 *a, f32 *b);

/*
 * Moves a player to its moveTo position over frames frames
 * (TownPc_StepMoveTo), or at once when it is 800 or more away; also sets
 * up the turn toward targetYaw over the same frames. Used for the other
 * members' position messages.
 */
void TownPc_StartMoveTo(Player *pl, s16 frames)
{
    if (!(Vec_Distance(&pl->pos[0], &pl->moveTo[0]) < 800.0f)) {
        pl->moveToFrames = 0;
        pl->pos[0] = pl->moveTo[0];
        pl->pos[1] = pl->moveTo[1];
        pl->pos[2] = pl->moveTo[2];
        return;
    }
    pl->moveToFrames = frames;
    pl->moveToStep[0] = (pl->moveTo[0] - pl->pos[0]) / (f32)frames;
    pl->moveToStep[1] = (pl->moveTo[1] - pl->pos[1]) / (f32)frames;
    pl->moveToStep[2] = (pl->moveTo[2] - pl->pos[2]) / (f32)frames;
    pl->weaponTimer = frames;
    pl->envTimer = (s16)(pl->targetYaw - pl->rot[1]) / frames;
}

void Player_SetFlag(void *obj, u32 flag);

/*
 * Moves a townsperson to its goal over frames frames (the step per frame
 * and the frame count in its NpcWork) and sets flag 0x4000000.
 */
void Npc_StartMoveTo(Em *npc, s32 frames)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    npcWork->moveCount = frames;
    npcWork->step[0] = (npc->goal[0] - npc->posX) / (f32)frames;
    npcWork->step[1] = (npc->goal[1] - npc->posY) / (f32)frames;
    npcWork->step[2] = (npc->goal[2] - npc->posZ) / (f32)frames;
    Player_SetFlag((Player *) npc, 0x4000000);
}

/*
 * One frame of a townsperson's move to its goal (the route runner's): adds
 * the step while frames are left.
 */
void Npc_StepRouteMove(Em *npc)
{
    NpcWork *npcWork;

    npcWork = (NpcWork *)&npc->work;
    if (npcWork->moveCount != 0) {
        npc->posX = npc->posX + npcWork->step[0];
        npc->posY = npc->posY + npcWork->step[1];
        npc->posZ = npc->posZ + npcWork->step[2];
        npcWork->moveCount = npcWork->moveCount - 1;
    }
}

/*
 * One frame of a player's move to moveTo (TownPc_StartMoveTo): adds the
 * step while frames are left. As sub_main's Player_UpdateMoveTo.
 */
void TownPc_StepMoveTo(Player *pl)
{
    s16 framesLeft = pl->moveToFrames;

    if (framesLeft != 0) {
        pl->moveToFrames = framesLeft - 1;
        if (framesLeft > 0) {
            pl->pos[0] += pl->moveToStep[0];
            pl->pos[1] += pl->moveToStep[1];
            pl->pos[2] += pl->moveToStep[2];
        }
    }
}

/*
 * For another member's player: turns it a quarter of the way to targetYaw
 * each frame and counts its turn frames down (not in state 2).
 */
void TownPc_SmoothRemoteYaw(Player *pl)
{
    if (pl->index != gScene.localPlayer) {
        if (pl->state != 2) {
            (pl->weaponTimer)--;
        }
        pl->rot[1] += (s16)(pl->targetYaw - pl->rot[1]) / 4;
    }
}

/*
 * One frame of a townsperson's move to its goal (Npc_Run's), as
 * Npc_StepRouteMove but counting down first and stepping only while the
 * count was positive.
 */
void Npc_StepMoveTo(Em *npc)
{
    u16 count;
    NpcWork *npcWork;

    npcWork = (NpcWork *)&npc->work;
    count = npcWork->moveCount;
    if (count != 0) {
        npcWork->moveCount = count - 1;
        if ((s32) count > 0) {
            npc->posX += npcWork->step[0];
            npc->posY += npcWork->step[1];
            npc->posZ += npcWork->step[2];
        }
    }
}

/*
 * Clears flag bits of an actor: in flags, or in stateFlags when the value
 * has bit 31 set.
 */
void Actor_ClearFlag(void *obj, u32 flag)
{
    ObjHead *head = obj;
    if ((flag & 0x80000000) == 0) {
        head->flags &= ~flag;
    } else {
        head->stateFlags &= ~(flag & 0x7FFFFFFF);
    }
}

/*
 * Sets flag bits of an actor: in flags, or in stateFlags when the value
 * has bit 31 set.
 */
void Actor_SetFlag(void *obj, u32 flag)
{
    ObjHead *head = obj;
    if ((flag & 0x80000000) == 0) {
        head->flags |= flag;
    } else {
        head->stateFlags |= flag & 0x7FFFFFFF;
    }
}

/*
 * Returns the action spot list of an area, by the area's index in
 * gMsgAreaIds (an unknown area gets list 0).
 */
u8 *ActSpot_GetList(u16 area)
{
    return *(u8 **)((u8 *)gAreaActionSpots + ((Area_GetSetIndex(area) & 0xFF) * 4));
}

f32 Math_Sqrtf(f32 x);
s32 ActSpot_IsUsable(Player *pl, ActionSpot *s);

/*
 * For the local player outside the town menu: finds the action spot it
 * stands in (within the spot's radius, from 50 below to 300 above it) and
 * keeps it in pl->spot when ActSpot_IsUsable allows it. A spot found while
 * the player had none and the town was free also makes the player its own
 * lock-on target (the marker on the spot); leaving the spots clears that.
 */
/*
 * For the local player outside the town menu: finds the action spot it
 * stands in (within the spot's radius, from 50 below to 300 above it) and
 * keeps it in pl->spot when ActSpot_IsUsable allows it. A spot found while
 * the player had none and the town was free also makes the player its own
 * lock-on target (the marker on the spot); leaving the spots clears that.
 */
void TownPc_FindSpot(Player *pl)
{
    s8 targetSelf = 0;
    ActionSpot *cand;
    f32 spotX;
    f32 dx;
    f32 dz;

    if (pl->index == gScene.localPlayer && gTownMenuOpen != 1) {
        if (pl->spot == NULL && gTownMode == TOWN_MODE_FREE) {
            targetSelf = 1;
        }
        pl->spot = NULL;
        cand = (ActionSpot *)ActSpot_GetList(pl->area);
        if (cand == NULL) {
            pl->spot = NULL;
            return;
        }
        for (; (spotX = cand->x) != -1.0f; cand++) {
            if (pl->pos[1] < cand->y - 50.0f || !(pl->pos[1] < cand->y + 300.0f)) {
                continue;
            }
            dx = pl->pos[0] - spotX;
            dz = pl->pos[2] - cand->z;
            if (Math_Sqrtf(dx * dx + dz * dz) <= cand->radius) {
                if (ActSpot_IsUsable(pl, cand) == 1) {
                    pl->spot = cand;
                    if (targetSelf != 0) {
                        pl->target = pl;
                    }
                }
                return;
            }
        }
        pl->spot = NULL;
        if (((Player *)pl->target) == pl) {
            pl->target = NULL;
        }
    }
}

s32 Save_TestEventFlag(s32 flag);

/*
 * Returns 1 when the current area's event has not been seen yet
 * (AREA_HUB_0xAC without EVENT_FLAG_MINE_STRONG_PICK, AREA_HUB_0x57
 * without event flag 0x8A), else 0.
 */
s32 ActSpot_AreaEventUnseen(void)
{
    switch (gScene.area) {
    case AREA_HUB_0xAC:
        if (Save_TestEventFlag(0x89) == 0) {
            return 1;
        }
        break;
    case AREA_HUB_0x57:
        if (Save_TestEventFlag(0x8A) == 0) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Hud_ItemGetBusy(void);
s32 Actor_IsFacingWithin(u8 *arg0, void *arg1, s32 arg2);
Em *Npc_Find(u8 kind);
s32 Cond_Check(u16 id);
s32 LobbyTime_IsAm(void);
s32 MyHouse_InOwnHouse(void);
extern u8 gTownSeats[];
extern s16 gHouseCloseTimer;
extern u8 *gPoogieDigSpots;
extern u8 gPoogieDigSpotIdx;
extern u8 gPoogieDigState;

/*
 * Returns 1 when the player can use action spot s now, by the spot's kind:
 * whether its townsperson is there and in the right state, the time of
 * day, a quest taken, a meal eaten, the house's owner, the Poogie's
 * current dig spot and so on. Returns 0 while an item-get message is busy.
 */
s32 ActSpot_IsUsable(Player *pl, ActionSpot *spot)
{
    Em *npc;

    if (Hud_ItemGetBusy() == 1) {
        return 0;
    }
    switch (spot->kind) {
    case 3:
    case 0xC:
        return Actor_IsFacingWithin((u8 *) pl, &spot->x, 0x5A);
    case 0x11:
        if (gLobby->questTaken == 0) {
            return 0;
        }
        break;
    case 1:
        if (Game_IsOnline() == 0) {
            npc = Npc_Find(0x10);
            if (npc == NULL) {
                return 0;
            }
            if (Cond_Check(0x15) == 0) {
                return 0;
            }
            return npc->state == 0x8A ? 1 : 0;
        }
        break;
    case 2:
        if (Game_IsOnline() == 1) {
            npc = Npc_Find(0x33);
        } else {
            npc = Npc_Find(0x19);
        }
        if (npc == NULL) {
            return 0;
        }
        break;
    case 0xE:
        if (Game_IsOnline() == 0) {
            npc = Npc_Find(5);
            if (Cond_Check(1) == 0) {
                return 0;
            }
            if (Cond_Check(0x41) == 0) {
                return 0;
            }
            if (npc == NULL) {
                return 0;
            }
            switch (npc->state) {
            default:
                return 0;
            case TOWN_PC_IDLE:
                break;
            }
            if (gTownMode == TOWN_MODE_CONTEST) {
                return 1;
            }
            switch (spot->type) {
            case 0:
                if (LobbyTime_IsAm() != 1) {
                    return 0;
                }
                break;
            case 1:
                if (LobbyTime_IsAm() != 0) {
                    return 0;
                }
                break;
            }
        } else {
            if (gLobby->questTaken != 0) {
                return 0;
            }
            if (gTownSeats[spot->type] != 0) {
                return 0;
            }
        }
        break;
    case 0xD:
        if (gScene.area == AREA_HUB_0x89) {
            npc = Npc_Find(0x14);
        } else if (gScene.area == AREA_HUB_0x83) {
            npc = Npc_Find(0x22);
        } else {
            return 0;
        }
        if (npc == NULL) {
            return 0;
        }
        if (gTownMode != TOWN_MODE_FREE) {
            return 0;
        }
        if (gPoogieDigState == 0) {
            return 0;
        }
        if (spot->type != gPoogieDigSpots[gPoogieDigSpotIdx]) {
            return 0;
        }
        if (pl->state == TOWN_PC_POOGIE_DIG) {
            return 0;
        }
        break;
    case 0x18:
        switch (spot->type) {
        case 0x88:
            if (Cond_Check(0x64) == 0) {
                return 0;
            }
            break;
        }
        break;
    case 8:
        if (ActSpot_AreaEventUnseen() == 0) {
            return 0;
        }
        break;
    case 5:
    case 6:
        if (MyHouse_InOwnHouse() == 0) {
            return 0;
        }
        break;
    case 0xA:
        if (gLobby->mealEaten != 0) {
            return 0;
        }
        if (Game_IsOnline() == 1) {
            if (gHouseCloseTimer > 0) {
                return 0;
            }
        } else if (gSave.awayFromVillage == 0 && Npc_Find(0xA) != NULL && spot->type == 1) {
            return 0;
        }
        break;
    }
    return 1;
}

extern NetUser gNetUsers[];
s32 memcmp(void *, void *, s32);
s32 Area_IsHouse(u16 area);

/*
 * Returns 1 when player pl is shown to the local player: in the same area;
 * online also the same user in its member slot, in a house only the
 * players of the same house (none while the area menu is open), and for
 * another player only where Area_HasTownPcs allows it.
 */
s32 TownPc_IsInMyArea(Player *pl)
{

    if (pl->area != gScene.area) {
        return 0;
    }
    if (Game_IsOnline() == 0) {
        return 1;
    }
    if (memcmp(gLobby->members[pl->index].id, gNetUsers[pl->index].id, 8) != 0) {
        return 0;
    }
    if (Area_IsHouse(gScene.area) == 1) {
        if (gTownMode == TOWN_MODE_AREA_MENU) {
            return 0;
        }
        if (gLobby->memberSlot[gScene.localPlayer] != gLobby->memberSlot[pl->index]) {
            return 0;
        }
    }
    if (pl->index != gScene.localPlayer) {
        return Area_HasTownPcs(pl->area) == 1;
    }
    return 1;
}

/*
 * Applies a townsperson's pending hit stop, or counts the running one
 * down.
 */
void Npc_UpdateHitStop(Em *npc)
{
    u8 add;
    u8 left;

    add = npc->hitStopAdd;
    if (add != 0) {
        npc->hitStop = (u8) (npc->hitStop + add);
        npc->hitStopAdd = 0U;
        return;
    }
    left = npc->hitStop;
    if (left != 0) {
        npc->hitStop = (u8) (left - 1);
    }
}


/*
 * Puts an actor (a player or a townsperson) in state (group, state) and
 * resets the per-state values; townspeople also clear their part-motion
 * overrides, players keep the previous state and draw a new stateRand.
 * Another member's player that falls back to idle gets the idle motion;
 * the local player's change is sent when Actor_NeedsStateSend says so.
 */
void Actor_SetState(void *obj, s32 group, s32 state)
{
    ObjHead *head = obj;

    if (head->isEm == 0) {
        head->prevStateGroup = head->stateGroup;
        head->prevState = head->state;
    }
    head->stateGroup = group;
    head->state = state;
    head->step = 0;
    head->stateVar0 = 0;
    head->stateVar1 = 0;
    if (head->isEm == 0) {
        head->stateRand = Rand_Next(1);
    }
    if (head->isEm != 0) {
        head->partAnimOn[0] = 0;
        head->partAnimMotion[0] = 0;
        head->partAnimWeight[0] = 0;
        head->partAnimOn[1] = 0;
        head->partAnimMotion[1] = 0;
        head->partAnimWeight[1] = 0;
        head->partAnimOn[2] = 0;
        head->partAnimMotion[2] = 0;
        head->partAnimWeight[2] = 0;
        head->partAnimOn[3] = 0;
        head->partAnimMotion[3] = 0;
        head->partAnimWeight[3] = 0;
    }
    if (head->isEm == 0 && head->stateGroup == 2) {
        head->unk417 = 0;
    }
    Actor_RestartMotionTime(head, 0);
    head->flags &= 0x100100;
    head->stateFlags = 0;
    head->motion[0].step = 2.0f;
    head->motion[1].step = 2.0f;
    if (head->ownTask == 0 && head->index != gScene.localPlayer) {
        if (Actor_IsState(head, TOWN_PC_GROUP_ACTION, 0) == 0) {
            return;
        }
        if (head->motionId[0] != 1) {
            Actor_ReturnToIdle(head, 0, 4, 0);
        }
    }
    if (Actor_NeedsStateSend(head) != 0) {
        TownMsg_SendStateChange(head);
    }
}

/* A lock-on candidate (a player or a townsperson) as the lobby's lock-on
 * reads it: it keeps its own values in fields the game uses otherwise
 * (ObjHead.hp, ObjHead.target). */
typedef struct LockOn {
    /* 0x000 */ u8 pad0[0xAC];
    /* 0x0AC */ f32 pos[3];         /* ObjHead.pos */
    /* 0x0B8 */ u8 padB8[0x2C2 - 0xB8];
    /* 0x2C2 */ s16 sector;         /* ObjHead.hp: by the angle from the player, 0 to 5 (LockOn_SetSector); -1 out of reach; orders the list (LockOn_Insert) */
    /* 0x2C4 */ u8 pad2C4[0x3B4 - 0x2C4];
    /* 0x3B4 */ struct LockOn *next; /* ObjHead.target: the next candidate (LockOn_Insert); a player's: its lock-on target */
    /* 0x3B8 */ u8 pad3B8[0x3D6 - 0x3B8];
    /* 0x3D6 */ u8 targeted;
    /* 0x3D7 */ u8 pad3D7[0x4A8 - 0x3D7];
    /* 0x4A8 */ f32 dist;           /* from the player (LockOn_TestCandidate) */
} LockOn;

/*
 * Inserts candidate obj into the lock-on list, ordered by sector and then
 * by distance (nearest first in sectors 0-2, farthest first in 3-5).
 */
void LockOn_Insert(LockOn **list, LockOn *cand)
{
    LockOn *cur;
    LockOn *prev;
    s32 prioSector;
    LockOn *head;

    if (list == NULL) {
        *list = cand;
        return;
    }
    head = *list;
    prev = NULL;
    cur = head;
    if (cur != NULL) {
        prioSector = cand->sector;
        do {
            if ((u16)cur->sector > prioSector) {
                break;
            }
            if ((u16)cur->sector == prioSector) {
                if (prioSector < 3) {
                    if (!(cur->dist <= cand->dist)) {
                        break;
                    }
                } else if (cur->dist < cand->dist) {
                    break;
                }
            }
            prev = cur;
            cur = cur->next;
        } while (cur != NULL);
    }
    if (prev == NULL) {
        cand->next = head;
        *list = cand;
        return;
    }
    cand->next = prev->next;
    prev->next = cand;
}

/*
 * Sets the candidate's sector (0 to 5) from its angle off the player's
 * heading within the half arc range; with a target already locked, the
 * cycle direction (1 or 2) decides from which side the sectors are
 * counted.
 */
void LockOn_SetSector(Player *pl, LockOn *cand, u32 angle, u32 halfArc, u8 cycleDir)
{
    if (pl->target != 0 && cycleDir != 0) {
        if (cycleDir == 1) {
            if ((u16)angle < (u16)halfArc) {
                angle = (u16)(s32)(angle * (1.0f / (halfArc / 3.0f)));
            } else {
                angle = (u16)(u32)(6.0f - (s32)((0xFFFF - (u16)angle) * (1.0f / (halfArc / 3.0f))) - 1.0f);
            }
        } else {
            if ((u16)angle < (u16)halfArc) {
                angle = (u16)(u32)(6.0f - (s32)(angle * (1.0f / (halfArc / 3.0f))) - 1.0f);
            } else {
                angle = (u16)(s32)((0xFFFF - (u16)angle) * (1.0f / (halfArc / 3.0f)));
            }
        }
    } else {
        if ((u16)angle < (u16)halfArc) {
            angle = (u16)(s32)(angle * (1.0f / (halfArc / 3.0f)));
        } else {
            angle = (u16)(s32)((0xFFFF - (u16)angle) * (1.0f / (halfArc / 3.0f)));
        }
    }
    cand->sector = angle;
}

/* unit's own declaration: LockOn_Insert reads a1 = arg1, the candidate (left by LockOn_SetSector(arg0, arg1, ...), which keeps a1) */
void LockOn_Insert();
void LockOn_SetSector(Player *arg0, LockOn *arg1, u32 angle, u32 range, u8 mode);
s32 Actor_HeadingDiff(void *obj, void *pos);

/*
 * Tests a lock-on candidate: within 200 in height, inside the half arc
 * range of the player's heading and nearer than maxDist. Then returns 1
 * and inserts it in the list with its sector and distance; else marks it
 * out of reach (sector -1) and returns 0.
 */
s32 LockOn_TestCandidate(Player *pl, LockOn *cand, LockOn **list, s32 halfArc, u8 cycleDir, f32 maxDist)
{
    f32 dist;
    s32 arc;
    s32 angle;
    s32 angle16;

    cand->next = NULL;
    cand->targeted = 0;
    if (!((f32) abs((s32) (cand->pos[1] - pl->pos[1])) <= 200.0f)) {
        cand->sector = -1;
        return 0;
    }
    angle = Actor_HeadingDiff(pl, cand->pos) & 0xFFFF;
    dist = Vec_Distance(&pl->pos[0], cand->pos);
    arc = halfArc & 0xFFFF;
    angle16 = angle & 0xFFFF;
    if (((angle16 < arc) || ((0xFFFF - arc) < angle16)) && (dist < maxDist)) {
        cand->dist = dist;
        LockOn_SetSector((Player *)((u8 *) pl), cand, angle, halfArc, cycleDir);
        LockOn_Insert(list);
        return 1;
    }
    cand->sector = -1;
    return 0;
}

s32 Kbd_IsOpen(void); /* unit's own declaration: the definition returns s8; with s8 here, LockOn_Update's test of it compiles differently */
s32 Npc_IsPet(Em *em);
void LobbySe_Play(s32 id);
s32 Area_IsOnlineHouse(s32 area);
extern u8 gLockOnCycleDir;
extern s8 gLockOnMarkSkip;
extern s8 gLockOnNoCycle;

/*
 * Each frame, picks the local player's lock-on target among the
 * townspeople and the other shown players, or the action spot it stands at
 * (the player itself as target). PAD_LEFT and PAD_RIGHT cycle the target,
 * PAD_CROSS goes back to the spot, and the lock-on sound plays when the
 * target changes. Suspended while a menu or the keyboard is open, in town
 * modes other than free and quest departure, and while crouched.
 */
void LockOn_Update(void)
{
    Em *em;
    Player *localPl;
    NpcWork *work;
    s32 i;
    u16 count;
    LockOn *old;
    f32 dist;
    LockOn *target;
    Player *player;
    Player *local;

    target = NULL;
    localPl = &gPlayers[gScene.localPlayer];
    old = ((LockOn *)localPl->target);
    em = gEms;
    if (Hud_ItemGetBusy() == 1) {
        localPl->target = NULL;
        return;
    }
    if (gTownMenuOpen == 1 || (gTownMode != TOWN_MODE_FREE && gTownMode != TOWN_MODE_SET_OUT)) {
        if (gScene.menuOpen != 0 || Kbd_IsOpen() != 0 || gTownMode != TOWN_MODE_FREE) {
            gLockOnMarkSkip = 1;
        }
        return;
    }
    if (gTownMode == TOWN_MODE_SET_OUT) {
        if (localPl->spot == NULL) {
            gLockOnMarkSkip = 1;
            return;
        }
        if (((ActionSpot *)localPl->spot)->kind != 0x11) {
            gLockOnMarkSkip = 1;
            return;
        }
    }
    if (localPl->action == 1) {
        localPl->target = (LockOn *)localPl;
        return;
    }
    if (localPl->input.pressed & PAD_LEFT) {
        gLockOnCycleDir = 2;
    } else if (localPl->input.pressed & PAD_RIGHT) {
        gLockOnCycleDir = 1;
    }
    count = 0;
    for (i = 0; i < 20; i++, em++) {
        if (em->active != 0) {
            if (Area_IsOnlineHouse(gScene.area) != 0 || Npc_IsPet(em) != 0) {
                dist = 200.0f;
            } else {
                dist = 400.0f;
            }
            work = (NpcWork *)&em->work;
            if (Game_IsOnline() == 1 && MyHouse_InOwnHouse() == 0 && work->id == 0x14) {
                continue;
            }
            if ((s8)work->noTalk == 0) {
                if (em->variant == 0x4D) {
                    count += (u8)LockOn_TestCandidate(localPl, (LockOn *)em, &target, 0x2AAB, gLockOnCycleDir, 1.5f * dist);
                } else {
                    count += (u8)LockOn_TestCandidate(localPl, (LockOn *)em, &target, 0x2AAB, gLockOnCycleDir, dist);
                }
            }
        }
    }
    player = gPlayers;
    local = &gPlayers[gScene.localPlayer];
    for (i = 0; i < 10; i++, player++) {
        if (player->active != 0 && player->index != gScene.localPlayer && (u8)TownPc_IsInMyArea(player) &&
            gLobby->modelLoaded[i] != 0) {
            count += (u8)LockOn_TestCandidate((Player *)local, (LockOn *)player, &target, 0x238E, gLockOnCycleDir, 300.0f);
        }
    }
    if (count >= 2 || (count > 0 && local->spot != NULL)) {
        gLockOnNoCycle = 0;
    } else {
        gLockOnNoCycle = 1;
    }
    if (gTownMode == TOWN_MODE_SET_OUT) {
        target = NULL;
    }
    if (target == NULL) {
        if (local->spot == NULL) {
            if (((LockOn *)local->target) != NULL) {
                ((LockOn *)local->target)->targeted = 0;
            }
            local->target = NULL;
            return;
        }
        local->target = (LockOn *)local;
        return;
    }
    if (((LockOn *)local->target) != NULL) {
        ((LockOn *)local->target)->targeted = 0;
        if ((local->input.pressed & PAD_CROSS) && local->spot != NULL) {
            local->target = (LockOn *)local;
        } else if (local->input.pressed & (PAD_LEFT | PAD_RIGHT)) {
            LockOn *cur = ((LockOn *)local->target);
            LockOn *next = cur->next;
            if (next == NULL && local->spot == NULL) {
                local->target = target;
            } else if (next == NULL && local->spot != NULL) {
                local->target = (LockOn *)local;
            } else if (cur == (LockOn *)local) {
                local->target = target;
            } else {
                local->target = next;
            }
        } else {
            LockOn *cur = ((LockOn *)local->target);
            if (cur->sector == -1) {
                local->target = target;
            } else if (local->spot == NULL && cur == (LockOn *)local) {
                local->target = target;
            }
        }
        if (old != ((LockOn *)local->target)) {
            LobbySe_Play(0xC);
        }
    } else {
        local->target = target;
        LobbySe_Play(0xC);
    }
    ((LockOn *)local->target)->targeted = 1;
}

/*
 * Starts the requested gesture: the gesture group's state from the town's
 * gesture table, when actRequest is set (as sub_main's
 * Player_StartGesture).
 */
void TownPc_StartGesture(Player *pl)
{
    u8 gesture;

    gesture = pl->actRequest;
    if (gesture != 0) {
        Actor_SetStateFlags(pl, 1, *(u8 *)((u8 *)gTownGestureStates + gesture), 0);
    }
}

/*
 * Counts a drink (the drink state's start); online, the tenth makes the
 * player drunk.
 */
void TownPc_CountDrink(Player *pl)
{
    if (pl->drunk == 0) {
        pl->drinkCount = (pl->drinkCount + 1);
        if (((s32) pl->drinkCount >= 0xA) && (Game_IsOnline() == 1)) {
            pl->drunk = 1U;
        }
    }
}

s8 MyHouse_LocalSlot(void);

/*
 * Returns 1 when the local player owns the house it is in (always
 * offline).
 */
s32 MyHouse_IsOwner(void)
{
    s8 slot;

    if (Game_IsOnline() == 0) {
        return 1;
    }
    slot = MyHouse_LocalSlot();
    return slot == gScene.localPlayer;
}

/*
 * Returns 1 when the local player is in its own house: the house area
 * offline, or one of the online house areas as its owner.
 */
s32 MyHouse_InOwnHouse(void)
{
    switch (gScene.area) {
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        break;
    case AREA_HOUSE:
        if (Game_IsOnline() == 0) {
            return 1;
        }
        break;
    default:
        return 0;
    }
    if (Game_IsOnline() == 0) {
        return 1;
    }
    return MyHouse_IsOwner() != 0;
}

/*
 * Returns the house slot the local player is in (0-9, from its member
 * slot, gLobby->memberSlot's '0'-'9'), or -1 outside the house areas
 * (AREA_HOUSE, AREA_ONLINE_HOUSE_0 to _2). memberSlot's address is written
 * as an integer sum and the test as `0 <= slot`: the game's code adds the
 * index first and compares that way.
 */
s8 MyHouse_LocalSlot(void)
{
    s32 i = gScene.localPlayer;
    u16 area = gPlayers[i].area;
    s8 slot;

    switch (area) {
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
    case AREA_HOUSE:
        break;
    default:
        return -1;
    }
    if (*(u8 *)(i + (s32)gLobby + 0x1538) == 0) {
        return -1;
    }
    slot = *(u8 *)(i + (s32)gLobby + 0x1538) - '0';
    if (0 <= slot && slot < 10) {
        return slot;
    }
    return -1;
}

/*
 * Returns 1 when area is one of the online house's areas (one per house
 * level), else 0.
 */
s32 Area_IsOnlineHouse(s32 area)
{
    switch (area & 0xFFFF) {
    case AREA_ONLINE_HOUSE_0:
    case AREA_ONLINE_HOUSE_1:
    case AREA_ONLINE_HOUSE_2:
        return 1;
    }
    return 0;
}

void Pad_UpdateInput(void);
void TownPc_UpdateInput(void);
void TownPc_Update(Player *pl);
void TownMsg_SendPosition(Player *unused0);

/*
 * Each frame, updates the players shown to the local player
 * (TownPc_IsInMyArea; skipped while their look is pending, hidden until
 * their model is loaded), the local one after reading its pad; then sends
 * the local player's position.
 */
void TownPc_UpdateAll(void)
{
    Player *pl;
    s32 i;

    i = 0;
    while (i < 10) {
        pl = &gPlayers[i];
        if ((s8)gLobby->lookPending[i] != 0) {
            goto next;
        }
        if (pl->active == 0 || (s8)gLobby->pcLoaded[i] == 0) {
            pl->visible = 0;
            i++;
            continue;
        }
        if ((u8)TownPc_IsInMyArea(pl) == 0) {
            pl->visible = 0;
        } else if ((s8)gLobby->pcLoaded[pl->index] == 1) {
            pl->visible = 1;
        }
        if (pl->visible != 0) {
            gScene.padRead = 1;
            if (pl->index == gScene.localPlayer) {
                Pad_UpdateInput();
                TownPc_UpdateInput();
            }
            TownPc_Update(pl);
        }
    next:
        i++;
    }
    TownMsg_SendPosition(&gPlayers[gScene.localPlayer]);
}

void TownPc_RunActionState(Player *pl);
void TownPc_RunGestureState(Player *pl);
void Actor_TurnToTarget(Player *p);
void TownPc_UnwindLookYaw(Player *pl);
void TownPc_StepMoveTo(Player *obj);
void TownPc_SmoothRemoteYaw(Player *obj);
void Em_CollideField(Em *em, u8 keep);
void Obj_UpdateFloorEffects(ObjHead *obj);
struct TriHdr;
s32 Em_GetFloorInfo(Em *em, f32 *pos, struct TriHdr *info, f32 *out, s32 *arg4);

/*
 * A player's frame in the lobby: timers, its state handler (run twice when
 * a state change asks for it), the turn and slides, motions, the floor
 * height, the action spot search for the local player, the motion effects,
 * and whether the weapon is shown.
 */
void TownPc_Update(Player *pl)
{
    f32 y;

    pl->offerStarted = 0;
    Actor_TickTimers(pl);
    *(Vec3f *)pl->prevPos = *(Vec3f *)pl->pos;
    pl->unk58E = pl->unk58C = -1;
    pl->unk3F8 = 0;
    pl->freeState = 0;
    switch (pl->stateGroup) {
    case 0:
        TownPc_RunActionState(pl);
        break;
    case 1:
        TownPc_RunGestureState(pl);
        break;
    }
    if (pl->stateRequest != 0) {
        pl->freeState = 0;
        switch (pl->stateGroup) {
        case 0:
            TownPc_RunActionState(pl);
            break;
        case 1:
            TownPc_RunGestureState(pl);
            break;
        }
        pl->stateRequest = 0;
    }
    pl->actRequest = 0;
    Actor_TurnToTarget(pl);
    TownPc_UnwindLookYaw(pl);
    TownPc_StepMoveTo(pl);
    if (pl->index != gScene.localPlayer) {
        TownPc_SmoothRemoteYaw(pl);
    }
    Actor_StartMotions(pl);
    if (pl->index == gScene.localPlayer) {
        Em_CollideField((Em *) pl, 0);
    }
    if ((pl->state != TOWN_PC_REST_LONG && pl->state != TOWN_PC_REST) || gScene.area == AREA_HUB_0x57) {
        if (Em_GetFloorInfo((Em *) pl, pl->pos, (struct TriHdr *) &pl->floorKind, &y, (s32 *) &pl->surfaceY) == 1) {
            pl->floorY = y;
        }
        pl->pos[1] = pl->floorY;
    }
    pl->offerRequest = 0;
    if ((u8)TownPc_IsInMyArea(pl)) {
        Obj_UpdateFloorEffects((ObjHead *) pl);
    }
    if (pl->index == gScene.localPlayer) {
        TownPc_FindSpot(pl);
    }
    if ((u8)TownPc_IsInMyArea(pl)) {
        pl->handlers->motionFxStep(pl);
    }
    if (TownPc_IsFree(pl) == 0) {
        pl->weaponShown = 0;
    } else {
        pl->weaponShown = 1;
    }
}

s32 Motion_ReachedFrame(void *, s32, f32);
s32 HairMenu_IsIdle(void);
s32 Contest_IsIdle(void);

/*
 * Returns 0 while the player is busy (in bed past frame 144 of lying down,
 * sleeping, getting up, and other seated or event states) or a contest,
 * the drink contest or the hair menu is running; else 1.
 */
s32 TownPc_IsFree(Player *pl)
{
    if (pl->stateGroup == TOWN_PC_GROUP_ACTION) {
        switch (pl->state) {
        case TOWN_PC_BED_LIE_DOWN:
            if (Motion_ReachedFrame(pl, 0, 144.0f) != 0) {
                return 0;
            }
            break;
        case TOWN_PC_CONTEST_SEAT_A:
        case TOWN_PC_CONTEST_SEAT_B:
        case TOWN_PC_CONTEST_GRIP:
        case TOWN_PC_CONTEST_WIN:
        case TOWN_PC_CONTEST_LOSE:
        case TOWN_PC_BED_SLEEP:
        case TOWN_PC_BED_GET_UP:
        case TOWN_PC_BED_NAP:
        case TOWN_PC_REST_LONG:
            return 0;
        }
    }
    switch (gTownMode) {
    case TOWN_MODE_CONTEST:
        if (pl->index == gScene.localPlayer) {
            return Contest_IsIdle();
        }
        break;
    case TOWN_MODE_HAIR_MENU:
        return HairMenu_IsIdle();
    case TOWN_MODE_DRINK_CONTEST:
        return 0;
    }
    return 1;
}

/*
 * Turns a player toward targetYaw at a rate set by its state and running
 * motion, leaning it while it turns; the lean eases back once it faces the
 * target and is kept out of 0xA00..0xF600. Another member's player in the
 * walk state is turned by TownPc_SmoothRemoteYaw instead.
 */
/*
 * Turns a player toward targetYaw at a rate set by its state and running
 * motion, leaning it while it turns; the lean eases back once it faces the
 * target and is kept out of 0xA00..0xF600. Another member's player in the
 * walk state is turned by TownPc_SmoothRemoteYaw instead.
 */
void Actor_TurnToTarget(Player *pl)
{
    u32 rate;
    u32 diff;
    u16 leanStep;
    s32 leanNow;
    u32 goalYaw;

    if (pl->index != gScene.localPlayer && pl->state == TOWN_PC_WALK) {
        return;
    }
    if (Actor_IsState(pl, TOWN_PC_GROUP_ACTION, 2) != 0 || Actor_IsState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN_LOOP) != 0) {
        rate = 0x71C;
    } else if (Actor_IsState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN) != 0) {
        rate = 0x5B0;
    } else if (Actor_IsState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_TURN_AROUND) != 0) {
        rate = 0;
    } else {
        rate = 0xFA4;
    }
    goalYaw = pl->targetYaw;
    diff = (u16)(goalYaw - (u16)pl->rot[1]);
    switch (pl->motionId[0]) {
    case 0x27:
        rate = 0x71C;
        leanStep = 0x80;
        break;
    case 3:
        leanStep = 0x80;
        break;
    case 4:
        leanStep = 0xA0;
        break;
    default:
        leanStep = 0;
        break;
    }
    if ((u16)(diff + rate) < rate * 2) {
        pl->rot[1] = goalYaw;
        pl->unk2B8 = 0;
        leanNow = pl->lean;
        if (leanNow + 0x300 <= 0x600) {
            pl->lean = 0;
        } else if (leanNow < 0x8000) {
            pl->lean = leanNow - 0x300;
        } else {
            pl->lean = leanNow + 0x300;
        }
    } else if (diff < 0x8000U) {
        pl->rot[1] = (u16)(pl->rot[1] + rate);
        pl->lean -= leanStep;
    } else {
        pl->rot[1] = (u16)(pl->rot[1] - rate);
        pl->lean += leanStep;
    }
    leanNow = pl->lean;
    if (leanNow <= 0xF600 && leanNow >= 0xA00) {
        if (leanNow >= 0x8000) {
            pl->lean = 0xF600;
        } else {
            pl->lean = 0xA00;
        }
    }
}

/* Clears lookFound and turns lookYaw back toward 0 by 0x400 a frame. */
void TownPc_UnwindLookYaw(Player *pl)
{
    u16 yaw;
    s32 sum;

    pl->lookFound = 0;
    yaw = *(u16 *)&pl->lookYaw;
    if (yaw != 0) {
        sum = yaw + 0x400;
        if (sum < 0x801 && sum >= 0) {
            pl->lookYaw = 0;
        } else if ((s16)yaw >= 0) {
            *(u16 *)&pl->lookYaw -= 0x400;
        } else {
            *(u16 *)&pl->lookYaw += 0x400;
        }
    }
}

/*
 * The idle state: a drunk player goes to the drunk idle; after 240 frames
 * a gender-1 player plays the second idle motion; otherwise it keeps the
 * idle motion running. Then the action checks.
 */
/*
 * The idle state: a drunk player goes to the drunk idle; after 240 frames
 * a gender-1 player plays the second idle motion; otherwise it keeps the
 * idle motion running. Then the action checks.
 */
void TownPc_StateIdle(Player *pl)
{
    u16 motion;

    if (pl->drunk != 0) {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_DRUNK_IDLE);
        return;
    }
    if (pl->motionTime >= 0xF0 && pl->gender == 1 && pl->motionId[0] != 0x3F) {
        Player_SetMotionPair(pl, 0x3F, 2, 0);
    } else {
        motion = pl->motionId[0];
        if (motion != 1 && motion != 0x3F) {
            if (motion == 0xCF || motion == 0x26E) {
                Actor_ReturnToIdle(pl, 0, 0xC, 0);
            } else {
                Actor_ReturnToIdle(pl, 0, 4, 0);
            }
        }
    }
    TownPc_CheckAction(pl);
}

/*
 * The walk states, by variant: 1 the walk (state 3), 6 the run loop (state
 * 5), 9 the drunk walk (state 0x23), 0 a run start (no dispatcher passes
 * it). Starts the variant's motion (the drunk ones when drunk), then each
 * frame stops (idle, or the run stop after a run) when the stick is
 * released, turns toward the stick and runs the action checks.
 */
void TownPc_StateWalk(Player *pl, s32 variant)
{
    s32 kind;
    s32 timer;
    s32 moveKind;
    u16 motionId;
    u8 step;
    u8 stateVar1;

    step = pl->step;
    if (step == 0) {
        pl->step = (u8) (step + 1);
        pl->stateVar0 = 0;
        pl->stateVar1 = 0xAU;
        pl->timer = 0xA;
        Actor_SetPosture(pl, 0, 0, 0);
        Actor_SetFlag(pl, 0x1000);
        switch (variant) {
        case 0:
            if (pl->drunk != 0) {
                if (pl->motionId[0] != 0x27) {
                    Actor_SetMotion(pl, PL_MOT_DASH_TIRED, 6, 0);
                }
            } else if (pl->motionId[0] != 3) {
                Actor_SetMotion(pl, PL_MOT_RUN, 6, 0);
            }
            Actor_SetFlag(pl, 0x02000000);
            break;
        case 1:
            if (pl->drunk != 0) {
                if (pl->motionId[0] != 0x258) {
                    Actor_SetMotion(pl, 0x258, 4, 0);
                }
            } else if (pl->gender == 1) {
                Actor_SetMotion(pl, 0x40, 4, 0);
            } else {
                Actor_SetMotion(pl, PL_MOT_WALK, 4, 0);
            }
            break;
        case 6:
            pl->dashDelay = 0x1E;
            if (pl->drunk != 0) {
                if (pl->motionId[0] != 0x27) {
                    Actor_SetMotion(pl, PL_MOT_DASH_TIRED, 4, 0);
                }
            } else if (pl->motionId[0] != 3) {
                Actor_SetMotion(pl, PL_MOT_RUN, 0, 0x3A);
            }
            Actor_SetFlag(pl, 0x02000000);
            break;
        case 9:
            Actor_SetMotion(pl, 0x258, 4, 0);
            break;
        }
        pl->motionTime = 0;
    }
    moveKind = TownPc_MoveKind((Player *)pl) & 0xFF;
    stateVar1 = pl->stateVar1;
    if (stateVar1 != 0) {
        pl->stateVar1 = (u8) (stateVar1 - 1);
    }
    timer = pl->timer;
    if (timer > 0) {
        pl->timer = (s32) (timer - 1);
    }
    kind = moveKind & 0xFF;
    if ((kind == 0) && (pl->timer == 0)) {
        motionId = pl->motionId[0];
        if ((motionId == 3) || (motionId == 4)) {
            if (pl->stateVar1 == 0) {
                Actor_SetStateFlags((void *) pl, 0U, 6U, 0);
            } else {
                Actor_ReturnToIdle((void *) pl, 0, 4, 0);
            }
        } else {
            Actor_ReturnToIdle((void *) pl, 0, 4, 0);
        }
        TownPc_StartRequestedOffer(pl);
        return;
    }
    if ((pl->motionId[0] == 3) && (kind < 5)) {
        Actor_ReturnToIdle((void *) pl, 0, 4, 0);
        return;
    }
    if ((pl->index == gScene.localPlayer) && (kind != 0)) {
        pl->targetYaw = TownPc_InputYaw(pl, 0);
    }
    if (Player_IsLocal((Player *) pl) != 0) {
        TownPc_CheckAction((void *) pl);
    }
}

void Vec_Rotate(f32 *out, f32 *v, f32 *m);

/*
 * The run (PAD_R2 held): runs forward 5 units a frame (1 when drunk)
 * toward the stick; when PAD_R2 is let go it goes to the walk, or to the
 * run stop when the stick is released too.
 */
void TownPc_StateRun(Player *pl)
{
    f32 move[4];
    f32 dir[3];
    u8 moveKind;

    switch (pl->step) {
    case 0:
        pl->step++;
        Actor_SetPosture(pl, 0, 0, 0);
        Actor_SetFlag(pl, 0x600);
        pl->motionTime = 0;
        pl->dashDelay = 0x1E;
        if (pl->drunk != 0) {
            if (pl->motionId[0] != 0x27) {
                Player_SetMotionPair(pl, 0x27, 4, 0);
            }
            pl->motion[1].step = pl->motion[0].step = 2.0f;
        } else if (pl->motionId[0] == 3) {
            pl->motion[1].step = pl->motion[0].step = 2.5f;
        } else {
            Player_SetMotionPair(pl, 3, 0, 0x3A);
            pl->motion[1].step = pl->motion[0].step = 2.5f;
        }
        pl->targetYaw = pl->rot[1];
        break;
    case 1:
        if (pl->index == gScene.localPlayer && pl->input.stickTilt >= PL_STICK_HELD) {
            pl->targetYaw = TownPc_InputYaw(pl, 0);
        }
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        if (pl->drunk != 0) {
            dir[2] = 1.0f;
        } else {
            dir[2] = 5.0f;
        }
        Vec_Rotate((f32 *) move, (f32 *) dir, &pl->rotMtx[0]);
        if (gScene.localPlayer == pl->index) {
            pl->pos[0] += move[0];
            pl->pos[2] += move[2];
        }
        if (Player_IsLocal(pl)) {
            moveKind = TownPc_MoveKind(pl);
            if (moveKind != 5 && (gScene.localPlayer == pl->index || Game_IsOnline() == 0)) {
                if (moveKind != 0) {
                    if (pl->dashDelay == 0) {
                        Actor_SetStateFlags(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_WALK, 0);
                    } else {
                        TownPc_CheckAction(pl);
                    }
                } else {
                    Actor_SetStateFlags(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN_STOP, 0);
                }
            } else {
                TownPc_CheckAction(pl);
            }
        }
        break;
    }
}

/*
 * The run stop: plays the skid at the end of a run, then goes idle. A
 * drunk player goes to the drunk idle at once.
 */
void TownPc_StateRunStop(Player *pl)
{
    u8 step;

    if (pl->drunk != 0) {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_DRUNK_IDLE);
        Actor_SetPosture(pl, 0, 0, 0);
    } else {
        step = pl->step;
        switch (step) {
        case 0:
            pl->step = (u8) (step + 1);
            Actor_SetPosture(pl, 0, 0, 0);
            Actor_SetMotion(pl, PL_MOT_RUN_STOP, 4, 0);
            break;
        case 1:
            if (pl->motion[0].playing == 0) {
                Actor_ReturnToIdle((void *) pl, 0, 0xA, 0);
            }
            break;
        }
    }
}

s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 SavePouch_Count(u16 item);
s16 SavePouch_HasFreeSlot(void);
s32 SavePouch_Room(u16 item);
u8 Notice_PushItem(char *item);
u8 Notice_Push(u8, s16, s16);
void TownMsg_SendOfferTake(void *x, Player *p);

/*
 * Called when the local player presses circle: looks for another player in
 * the same area (and, in an online house, the same member group) who holds
 * an item out to this player within 300 units. Items above rarity 2 are
 * skipped. If the pouch has room the item and its count are copied and the
 * player starts waiting for it (the local player also asks the giver over
 * the net); otherwise a pouch-full notice shows and no new take is tried
 * for 90 frames. Returns 1 when such a giver was found, else 0.
 */
s32 TownPc_TakeOfferedItem(Player *pl)
{
    s16 i;
    Player *giver;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (pl->offerNoticeTimer != 0) {
        return 0;
    }
    for (giver = gPlayers, i = 0; i < 10; i++, giver++) {
        if (giver->active == 0 || i == pl->index || giver->area != pl->area) {
            continue;
        }
        if (Player_IsState(giver, 0, 0xA) == 0 || giver->offerTo != pl->index) {
            continue;
        }
        if (!(Vec_Distance(&pl->pos[0], giver->pos) <= 300.0f)) {
            continue;
        }
        if (gItemData[giver->offerItem].rarity > 2) {
            continue;
        }
        if (Area_IsOnlineHouse(pl->area) &&
            (u8)gLobby->memberSlot[giver->index] != (u8)gLobby->memberSlot[pl->index]) {
            continue;
        }
        if (SavePouch_Count(giver->offerItem) == 0) {
            if (SavePouch_HasFreeSlot() != 0) {
                pl->offerItem = giver->offerItem;
                pl->offerCount = giver->offerCount;
                if (Player_IsLocal(pl) == 1) {
                    TownMsg_SendOfferTake(&gLobby->members[i], giver);
                }
                Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_TAKE_ITEM_WAIT, 0);
            } else {
                Notice_PushItem((char *) gPouchFullTakeText);
                pl->offerNoticeTimer = 0x5A;
            }
        } else if (SavePouch_Room(giver->offerItem) >= giver->offerCount) {
            pl->offerItem = giver->offerItem;
            pl->offerCount = giver->offerCount;
            if (Player_IsLocal(pl) == 1) {
                TownMsg_SendOfferTake(&gLobby->members[i], giver);
            }
            Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_TAKE_ITEM_WAIT, 0);
        } else {
            Notice_Push(1, 0xF, (s16)giver->offerItem);
            pl->offerNoticeTimer = 0x5A;
        }
        return 1;
    }
    return 0;
}


/*
 * The giver holds the item out to another player. The local giver stops on
 * cross, when an answer arrives, or when the asker's net slot is empty,
 * and goes to the end of the offer.
 */
void TownPc_StateOfferItem(Player *pl)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        Actor_SetPosture(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x3C, 4, 0);
        pl->offerState = PL_OFFER_WAITING;
        pl->freeState = 0;
        break;
    case 1:
        if (Player_IsLocal(pl) == 1 && ((pl->input.pressed & PAD_CROSS) || pl->offerState != PL_OFFER_WAITING || gNetUsers[pl->offerTo].id[0] == 0)) {
            Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_OFFER_ITEM_END, 0);
        }
        break;
    }
}

/*
 * The taker waits for the giver to hand the item over. When it arrives it
 * plays the take; after about 10 s (601 frames) or a refusal it goes idle,
 * and the local player tells the others the wait ended.
 */
void TownPc_StateTakeItemWait(Player *pl)
{
    s32 timer;
    u8 step;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, 1, 6, 0);
        }
        pl->offerState = PL_OFFER_WAITING;
        pl->timer = 0;
        break;
    case 1:
        timer = pl->timer + 1;
        pl->timer = timer;
        if ((timer >= 0x259) || (pl->offerState != PL_OFFER_WAITING)) {
            if (pl->offerState == PL_OFFER_RECEIVED) {
                Actor_SetStateNow((void *) pl, 0, 0xD, 0);
                break;
            }
            Actor_ReturnToIdle((void *) pl, 0, 2, 0);
            if (pl->index == gScene.localPlayer) {
                TownMsg_SendOfferEnd();
            }
        } else {
        }
        break;
    }
}

/*
 * The giver's closing motion after the hand-over or a cancel, then idle.
 */
void TownPc_StateOfferItemEnd(Player *pl)
{
    u8 step;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0x3D, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_ReturnToIdle((void *) pl, 0, 6, 0);
        }
        break;
    }
}

/* The taker's motion once the offered item has arrived, then idle. */
void TownPc_StateTakeItem(Player *pl)
{
    u8 step;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0x280, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_ReturnToIdle((void *) pl, 0, 6, 0);
        }
        break;
    }
}

/* Lies down on the bed, then goes to the sleep loop. */
void TownPc_StateBedLieDown(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 0, 0, 0);
        pl->motionTime = 0;
        Actor_SetMotion(pl, PL_MOT_BED_LIE_DOWN, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_SetStateNow((void *) pl, 0, 0xF, 0);
        }
        break;
    }
}

extern Vec3f gBedSleepFxOffset;
void Effect06_SpawnAt(void *obj, s16 type, u8 bone, f32 *pos, f32 scale);
s32 TownPc_IsInMyArea(Player *p);

/*
 * Runs the bed's sleep loop: a random stir one time in 64, back to the
 * loop when it ends, and, while the player is shown to the local player,
 * the sleep effect above the head three times a second at the start of
 * each second.
 */
void TownPc_LoopBedSleep(Player *pl)
{
    Vec3f fxOffset = gBedSleepFxOffset;

    if (pl->motionId[0] == 0x1AB && ((u16)Rand_Next(1) & 0x3F) == 0) {
        Actor_SetMotion(pl, PL_MOT_BED_SLEEP_STIR, 0, 0);
    } else if (pl->motionId[0] == 0x1AC && pl->motion[0].playing == 0) {
        Actor_SetMotion(pl, PL_MOT_BED_SLEEP, 0, 0);
    }
    if (TownPc_IsInMyArea(pl) != 0) {
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 0x14, (f32 *) &fxOffset, 0.6f);
            break;
        }
    }
}

u16 LobbyPad_Get(s32 a);

/*
 * Sleeps in bed while the bed's menu runs. While the local player's town
 * is fading out to leave (the three exit modes) only the sleep loop runs.
 * The parameter is unused.
 */
void TownPc_StateBedSleep(Player *pl, s32 unused)
{
    u8 step;

    LobbyPad_Get(0);
    if (pl->index == gScene.localPlayer) {
        switch (gTownMode) {
        case TOWN_MODE_QUIT:
        case TOWN_MODE_QUIT_FADE:
        case TOWN_MODE_QUIT_DONE:
            TownPc_LoopBedSleep(pl);
            return;
        }
    }
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = step + 1;
        Actor_SetPosture(pl, 0, 0, 0);
        pl->motionTime = 0;
        Actor_SetMotion(pl, PL_MOT_BED_SLEEP, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        TownPc_LoopBedSleep(pl);
        break;
    }
}

/*
 * Turns half a turn and gets out of bed; the local player's town mode goes
 * back to free. The parameter is unused.
 */
void TownPc_StateBedGetUp(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 0, 0, 0);
        pl->motionTime = 0;
        pl->rot[1] = (s32) ((pl->rot[1] + 0x7FFF + 1) & 0xFFFF);
        pl->targetYaw = (s16) pl->rot[1];
        Actor_SetMotion(pl, PL_MOT_BED_GET_UP, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (pl->index == gScene.localPlayer) {
                gTownMode = TOWN_MODE_FREE;
            }
            Actor_ReturnToIdle((void *) pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The quick half turn (the turn-around motion), then the run. A drunk
 * player runs at once.
 */
void TownPc_StateTurnAround(Player *pl)
{
    u8 step;

    if (pl->drunk != 0) {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN);
    } else {
        step = pl->step;
        switch (step) {
        case 0:
            pl->step = (u8) (step + 1);
            pl->motionTime = 0;
            pl->rot[1] = (s32) pl->targetYaw;
            Actor_SetMotion(pl, PL_MOT_TURN_AROUND, 0, 0);
            Actor_SetPosture(pl, 0, 0, 0);
            Actor_SetFlag((void *) pl, 0x1000);
            break;
        case 1:
            if (pl->motion[0].playing == 0) {
                Actor_SetStateNow((void *) pl, 0, 4, 0);
            }
            break;
        }
    }
}

/*
 * The drunk collapse: the player sinks to the ground and stays there
 * (checking for an offered item) until the local player tilts the stick,
 * then gets up.
 */
void TownPc_StateDrunkCollapse(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Actor_SetPosture(pl, 0, 0, 0);
        if (pl->motionId[0] != 0x101) {
            Actor_SetMotion(pl, 0x101, 0x10, 0x1E);
        }
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
        }
        TownPc_StartRequestedOffer(pl);
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && ((s32) pl->input.stickTilt >= PL_STICK_HELD)) {
            Actor_SetState(pl, 0, 0x13U);
        }
        break;
    }
}

/*
 * Gets up from the drunk collapse (rises, then the face-down get-up), then
 * back to the drunk idle.
 */
void TownPc_StateDrunkGetUp(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Actor_SetPosture(pl, 0, 0, 0);
        Actor_SetMotion(pl, 0x102, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_DOWN, 4, 0x36);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Actor_SetStateNow((void *) pl, 0, 0x1D, 0);
            break;
        }
        TownPc_StartRequestedOffer(pl);
        break;
    }
}

/*
 * Returns 1 when (group, state) is one of the states that hold a seat:
 * sitting down, seated, the seated poses and their ends, the drink, the
 * kitchen's order and the served meal; else 0. Only group 0 counts.
 */
s32 TownPc_IsSeatedState(u8 group, u8 state)
{
    if (group & 0xFF) {
        return 0;
    }
    switch (state & 0xFF) {
    case 0x14:
    case 0x1C:
    case 0x21:
    case 0x22:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x28:
    case 0x16:
    case 8:
    case 7:
    case 9:
    case 0x29:
    case 0x1E:
        return 1;
    }
    return 0;
}

/*
 * Sits down in the barber's chair while the hair menu runs (local player
 * only): the sit-down motion, then the seated loop.
 */
void TownPc_StateHairChair(Player *pl)
{
    u8 step;

    if (pl->index == gScene.localPlayer) {
        step = pl->step;
        switch (step) {
        case 0:
            Actor_SetPosture(pl, 1, 0, 0);
            Actor_SetMotion(pl, PL_MOT_TOWN_SIT, -4, 0);
            pl->step = (u8) (pl->step + 1);
            break;
        case 1:
            if (pl->motion[0].playing == 0) {
                pl->step = (u8) (step + 1);
                Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 4, 0);
            }
            break;
        }
    }
}

void LobbyBgm_SetUnk11F(s32 on);
s32 LobbyBgm_GetTrack(void);
s32 func_007D98B0(void);
void Kitchen_Open(void);
s32 Area_IsTownHall(s32 area);
extern u8 gTownCamEndOnClose;

/*
 * Sits down on a table seat; alreadySeated 1 (TOWN_PC_SEATED) skips the
 * sit-down. After sitting down the state becomes TOWN_PC_SEATED; the local
 * player in their own house (or offline outside AREA_HUB_0x89) opens the
 * kitchen when no meal was eaten yet. Seated, circle stands up, and in the
 * areas Area_IsTownHall accepts the other buttons pick a seated action
 * (cross, square, and the four d-pad poses); in AREA_TOWN_ARENA
 * LobbyBgm_SetUnk11F turns its stage camera on while seated and off on
 * standing.
 */
void TownPc_StateSitDown(Player *pl, s32 alreadySeated)
{
    u16 pad;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Actor_SetPosture(pl, 1, 0, 0);
        pl->weaponOut = 0;
        if (alreadySeated == 1) {
            pl->step = 2;
            pl->timer = 0xE10;
            pl->timer = 0x3C;
            if (pl->motionId[0] != 0x261) {
                Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            }
        } else {
            Actor_SetMotion(pl, PL_MOT_TOWN_SIT, -4, 0);
            if (pl->index == gScene.localPlayer) {
                gFieldControl = 0;
                gTownCamEndOnClose = 1;
                if (gScene.area == AREA_TOWN_ARENA) {
                    LobbyBgm_SetUnk11F(1);
                }
            }
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->timer = 0xE10;
            pl->timer = 0x3C;
            pl->step++;
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 4, 0);
            pl->state = 0x25;
            if (pl->index == gScene.localPlayer) {
                if (gTownMode != TOWN_MODE_DRINK_CONTEST) {
                    gFieldControl = 1;
                }
                if ((Game_IsOnline() == 1 && MyHouse_IsOwner() == 1) ||
                    (Game_IsOnline() == 0 && gScene.area != AREA_HUB_0x89)) {
                    if (gTownMode == TOWN_MODE_FREE && gLobby->mealEaten == 0) {
                        Kitchen_Open();
                    }
                }
            }
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            if ((pl->input.pressed & PAD_CIRCLE) && gTownMode == TOWN_MODE_FREE) {
                Actor_SetStateFlags(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_STAND_UP, 0);
                CamFocus_End();
                if (gScene.area == AREA_TOWN_ARENA) {
                    LobbyBgm_SetUnk11F(0);
                }
            } else if (Area_IsTownHall(gScene.area) == 1 && gTownMode == TOWN_MODE_FREE) {
                pad = pl->input.pressed;
                if (pad & 0x40) {
                    Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SEAT_CROSS);
                } else if (pad & 0x200) {
                    Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SEAT_DRINK);
                } else if (pad & 0x2000) {
                    Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SEAT_POSE_UP);
                } else if (pad & 0x1000) {
                    Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SEAT_POSE_DOWN);
                } else if (pad & 0x800) {
                    Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SEAT_POSE_LEFT);
                } else if (pad & 0x400) {
                    Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SEAT_POSE_RIGHT);
                } else if (gScene.area == AREA_TOWN_ARENA && LobbyBgm_GetTrack() != -1 && func_007D98B0() == 0) {
                    LobbyBgm_SetUnk11F(1);
                }
            }
        }
        break;
    }
}

/*
 * Returns 1 when (group, state) is a state that gives the seat up:
 * standing up and the three ways of rising after a meal; else 0.
 */
s32 TownPc_IsLeaveSeatState(s32 group, s32 state)
{
    if (group == 0) {
        switch (state) {
        case TOWN_PC_STAND_UP:
        case TOWN_PC_MEAL_RISE:
        case TOWN_PC_MEAL_COLLAPSE:
        case TOWN_PC_MEAL_RISE_GOOD:
            return 1;
        }
    }
    return 0;
}

/* Stands up from the seat, then idle. */
void TownPc_StateStandUp(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Actor_SetPosture(pl, 0, 0, 0);
        Actor_SetMotion(pl, 0x262, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_ReturnToIdle((void *) pl, 0, 4, 0);
        }
        break;
    }
}

/*
 * A nap in bed for the local player: the sleep loop for 6 s (360 frames),
 * then gets up. Other players go back to idle.
 */
void TownPc_StateBedNap(Player *pl)
{
    s32 timer;
    u8 step;

    if (pl->index != gScene.localPlayer) {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_IDLE);
    } else {
        step = pl->step;
        switch (step) {
        case 0:
            pl->step = (u8) (step + 1);
            Actor_SetPosture(pl, 0, 0, 0);
            pl->motionTime = 0;
            if (pl->motionId[0] != 0x1AB) {
                Actor_SetMotion(pl, PL_MOT_BED_SLEEP, 0, 0);
            }
            pl->timer = 0x168;
            break;
        case 1:
            TownPc_LoopBedSleep(pl);
            timer = pl->timer - 1;
            pl->timer = timer;
            if (timer <= 0) {
                Actor_SetStateNow((void *) pl, 0, 0x10, 0);
            }
            break;
        }
    }
}

void Jingle_Play(void *obj, s32 id);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
void Player_ClearFlag(void *obj, u32 flag);
extern u8 gOffersRefreshPending;

/* Sleeping (k 0 goes to bed, k 1 wakes up in it): the motions, the saved
 * bed position (+0x438 +0x14, angle +0x22), the jingle, then back to state
 * 0 at the bed (in AREA_HUB_0x57, turned around). Other players only idle. */
/* A lobby player's block at Player +0x438 (Player.work, whose PlayerWork it extends), as TownPc_StateRest (the bed) reads it. */
typedef struct TownPcWork {
    u8 unk00[0x14];
    Vec3f bedPos;               /* 0x14: the position kept while the player sleeps: saved at frames 166 and 260 of the sleep motion, put back at frame 198 and in step 3 */
    s16 jingleTimer;            /* 0x20: step 4 in AREA_HUB_0x57: frames until the jingle (0xA0) */
    u16 bedYaw;                 /* 0x22: the yaw kept with bedPos */
    u8 unk24[0x80 - 0x24];
} TownPcWork;
typedef char TownPcWork_size_check[(sizeof(TownPcWork) == 0x80) ? 1 : -1];

/*
 * The rest scene at the action spot of kind 8 (local player only; others
 * go idle); noEvent is 0 when an area event was pending (its gift went to
 * the box first), 1 otherwise. Outside AREA_HUB_0x57 the long motion keeps
 * the player at the position saved in the bed work block, plays jingle 9,
 * and at its end puts the player at one of two fixed places; in
 * AREA_HUB_0x57 the house's own motions play and the player is turned
 * round. At the end control returns, the offers are marked for a refresh
 * and the town mode goes back to free.
 */
void TownPc_StateRest(Player *pl, s8 shortForm)
{
    TownPcWork *restWork = (TownPcWork *)&pl->work;

    if (pl->index != gScene.localPlayer) {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_IDLE);
        return;
    }
    switch (pl->step) {
    case 0:
        pl->fieldCollide = 0;
        pl->step++;
        restWork->jingleTimer = 0;
        if (gScene.area == AREA_HUB_0x57) {
            pl->step = 4;
            if (shortForm == 0) {
                Actor_SetMotion(pl, 0x287, 0, 0);
            } else {
                Actor_SetMotion(pl, 0x288, 0, 0);
            }
        } else if (shortForm == 0) {
            Actor_SetMotion(pl, 0xFA, 0, 0);
            pl->timer = 2;
        } else {
            Actor_SetMotion(pl, 0xFB, 0, 0);
            pl->step = 4;
        }
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 198.0f)) {
            Actor_SetMotion(pl, 0xFA, 0, 0xA6);
            *(Vec3f *)pl->pos = restWork->bedPos;
            *(Vec3f *)pl->moveTo = *(Vec3f *)pl->pos;
            pl->targetYaw = pl->rot[1] = restWork->bedYaw;
            if (--pl->timer <= 0) {
                pl->step++;
            }
        } else if (Motion_PassedFrame(pl, 0, 166.0f) && pl->timer == 2) {
            restWork->bedPos = *(Vec3f *)pl->pos;
            restWork->bedYaw = pl->rot[1];
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 230.0f)) {
            Jingle_Play(pl, 9);
        }
        if (Motion_ReachedFrame(pl, 0, 260.0f)) {
            pl->step++;
            pl->timer = 0x3C;
            restWork->bedPos = *(Vec3f *)pl->pos;
            restWork->bedYaw = pl->rot[1];
        }
        break;
    case 3:
        if (--pl->timer <= 0) {
            pl->step++;
        } else {
            Actor_SetMotion(pl, 0xFA, 0, 0x104);
            *(Vec3f *)pl->pos = restWork->bedPos;
            *(Vec3f *)pl->moveTo = *(Vec3f *)pl->pos;
            pl->targetYaw = pl->rot[1] = restWork->bedYaw;
        }
        break;
    case 4:
        if (shortForm == 0 && gScene.area == AREA_HUB_0x57) {
            restWork->jingleTimer++;
            if (restWork->jingleTimer == 0xA0) {
                Jingle_Play(pl, 9);
            }
        }
        if (pl->motion[0].playing <= 0) {
            gFieldControl = 1;
            pl->fieldCollide = 1;
            gOffersRefreshPending = 1;
            Actor_ReturnToIdle(pl, 0, 0, 0);
            if (pl->index == gScene.localPlayer) {
                gTownMode = TOWN_MODE_FREE;
            }
            if (gScene.area == AREA_HUB_0x57) {
                Player_ClearFlag(pl, 0x20000);
                pl->rot[1] = pl->targetYaw = pl->rot[1] + 0x8000;
            } else if (shortForm == 0) {
                pl->pos[0] = 5045.0f;
                pl->pos[1] = 202.0f;
                pl->pos[2] = 6479.0f;
                pl->rot[1] = pl->targetYaw = 0x271C;
            } else {
                pl->pos[0] = 4847.5f;
                pl->pos[1] = -1.19f;
                pl->pos[2] = 7061.75f;
                *(Vec3f *)pl->moveTo = *(Vec3f *)pl->pos;
                pl->rot[1] = pl->targetYaw = 0x72A2;
            }
        }
        break;
    }
}

/*
 * Gets up from lying on the back (after the bad meal's collapse), then
 * idle.
 */
void TownPc_StateGetUpFaceUp(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_UP, 2, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_ReturnToIdle(pl, 0, 6, 0);
        }
        break;
    }
}

/*
 * The seated pose picked with right: its motion, then the local player
 * leaves it with circle, cross or the d-pad.
 */
void TownPc_StateSeatPoseRight(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x26A, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
        }
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && (pl->input.pressed & (PAD_CIRCLE | PAD_CROSS | PAD_DPAD))) {
            Actor_SetState(pl, 0, 9U);
        }
        break;
    }
}

/* The end of the right pose, back to seated. */
void TownPc_StateSeatPoseRightEnd(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x282, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

/*
 * The drunk idle: from the face-down get-up straight into the doubled-over
 * motion, else through the plain idle; then the panting loop. After 240
 * frames of motion time the player collapses; meanwhile the action checks
 * run.
 */
void TownPc_StateDrunkIdle(Player *pl)
{
    u16 motion;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        motion = pl->motionId[0];
        if (motion == 0xD8) {
            pl->step = 2U;
            Actor_SetMotion(pl, PL_MOT_EXHAUSTED_START, 4, 0);
            break;
        }
        if (motion != 1) {
            Actor_SetMotion(pl, PL_MOT_IDLE, 8, 0);
        }
        break;
    case 1:
        if (pl->motion[0].blend == 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_EXHAUSTED_START, 4, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_EXHAUSTED_PANT, 4, 0x90);
        }
        TownPc_CheckAction((void *) pl);
        break;
    case 3:
        if (pl->motionTime >= 0xF0) {
            Actor_SetState(pl, 0, 0x12U);
            break;
        }
        TownPc_CheckAction((void *) pl);
        break;
    }
}

/*
 * The kitchen's order at the table (Kitchen_Open sets it): its motion,
 * then seated.
 */
void TownPc_StateKitchenOrder(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x25A, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

/*
 * Thrown by the NPC that grabbed the player: the throw, the fall and the
 * face-down get-up, then idle. The local player's camera focus ends, the
 * town menu closes, the town mode goes back to free and the state is sent
 * at once.
 */
void TownPc_StateThrown(Player *pl)
{
    u8 step;
    u8 localPlayer;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x25C, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_FISH_PULLED_DOWN, 4, 0x2C);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_DOWN, 6, 0x24);
        }
        break;
    case 3:
        if (pl->motion[0].playing <= 0) {
            Actor_ReturnToIdle(pl, 0, 8, 0);
            if (gScene.localPlayer == pl->index) {
                CamFocus_End();
                gTownMenuOpen = 0;
                localPlayer = gScene.localPlayer;
                if (pl->index == localPlayer) {
                    gTownMode = TOWN_MODE_FREE;
                }
                TownMsg_SendStateNow((void *)((s32) &gPlayers[localPlayer & 0xFF]));
            }
        }
        break;
    }
}

/*
 * Held by an NPC (an NPC's state starts it before the throw): one motion,
 * held.
 */
void TownPc_StateGrabbed(Player *pl)
{
    switch (pl->step) {
    case 0:
        Actor_SetMotion(pl, 0x25B, 6, 0);
        pl->step++;
    case 1:
        break;
    }
}

/*
 * The seated drink (square): counts the drink toward drunkenness, plays
 * its motion, then seated.
 */
void TownPc_StateSeatDrink(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        TownPc_CountDrink((void *) pl);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x263, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

/* The seated action on cross: one motion, then seated. */
void TownPc_StateSeatCross(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x264, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

/* The end of the left pose, back to seated. */
void TownPc_StateSeatPoseLeftEnd(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x269, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

/*
 * The seated pose picked with left: after its motion and one more second
 * the local player leaves it with circle, cross or the d-pad.
 */
void TownPc_StateSeatPoseLeft(Player *pl)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x268, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->timer = 0x3C;
            pl->step = (u8) (pl->step + 1);
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            timer = pl->timer;
            if (timer <= 0) {
                if (pl->input.pressed & (PAD_CIRCLE | PAD_CROSS | PAD_DPAD)) {
                    Actor_SetState(pl, 0, 0x16U);
                    break;
                }
            } else {
                pl->timer = (s32) (timer - 1);
            }
        }
        break;
    }
}

/*
 * The seated pose picked with down: its motion, then the local player
 * leaves it with circle, cross or the d-pad.
 */
void TownPc_StateSeatPoseDown(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x265, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
        }
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && (pl->input.pressed & (PAD_CIRCLE | PAD_CROSS | PAD_DPAD))) {
            Actor_SetState(pl, 0, 7U);
        }
        break;
    }
}

/* The end of the down pose, back to seated. */
void TownPc_StateSeatPoseDownEnd(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x266, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownMsg_SendHouseSeatFree(void *pl);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);

/*
 * Rises from the seat after the meal: goodMeal 0 (TOWN_PC_MEAL_RISE) plays
 * the plain motion, 1 (TOWN_PC_MEAL_RISE_GOOD) the pose with the good-
 * effect sparkle. Then idle; the local player sends the seat's release.
 */
void TownPc_StateMealRise(Player *pl, s8 goodMeal)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        pl->motionTime = 0;
        Actor_SetPosture(pl, 0, 0, 0);
        Actor_SetMotion(pl, 0x262, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            if (goodMeal == 0) {
                Actor_SetMotion(pl, 0x27D, 6, 0);
                break;
            }
            Actor_SetMotion(pl, PL_MOT_ITEM_POSE, 6, 0x14);
            Effect06_SpawnAtBone(pl, 0, 8, 0xAU, 4.0f);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Actor_ReturnToIdle(pl, 0, 6, 0);
            if (pl->index == gScene.localPlayer) {
                TownMsg_SendHouseSeatFree(pl);
            }
        }
        break;
    }
}

/*
 * The seated pose picked with up: its motion, then the local player leaves
 * it with circle, cross or the d-pad.
 */
void TownPc_StateSeatPoseUp(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x267, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
        }
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && (pl->input.pressed & (PAD_CIRCLE | PAD_CROSS | PAD_DPAD))) {
            Actor_SetState(pl, 0, 8U);
        }
        break;
    }
}

/* The end of the up pose, back to seated. */
void TownPc_StateSeatPoseUpEnd(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x283, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

/*
 * The meal is served: the eating motion, then the seated loop; the meal
 * scene moves the player on.
 */
void TownPc_StateMealServed(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x27B, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
        }
        
    case 2:
        break;
    }
}

/*
 * The bad meal: rises and collapses onto the back, a quarter turn round,
 * lies 60 to 91 frames, then the local player gets up and sends the seat's
 * release.
 */
void TownPc_StateMealCollapse(Player *pl)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        Actor_SetMotion(pl, 0x27C, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            Actor_SetMotion(pl, PL_MOT_LIE_FACE_UP, 0, 0x92);
            pl->targetYaw += 0x4000;
            pl->rot[1] += 0x4000;
            pl->timer = ((u16)Rand_Next(1) & 0x1F) + 0x3C;
        }
        break;
    case 2:
        if (--pl->timer <= 0 && gScene.localPlayer == pl->index) {
            Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_GET_UP_FACE_UP);
            TownMsg_SendHouseSeatFree(pl);
        }
        break;
    }
}

/*
 * Two motions in a row, then held; step 3 (idle once the motion ends) is
 * never reached from here.
 */
void TownPc_State2B(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x25D, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 0x25E, 0, 0);
            break;
        }
    case 2:
        break;
    case 3:
        if (pl->motion[0].playing <= 0) {
            Actor_ReturnToIdle(pl, 0, 4, 0);
        }
        break;
    }
}

void Vec_Copy(f32 *dst, f32 *src);
void TownPc_StartMoveTo(Player *obj, s16 n);

/*
 * Walks the player to its action spot (Player.spot), turning to the spot's
 * angle, and marks the town menu open.
 */
void TownPc_WalkToSpot(Player *pl)
{
    f32 target[3];
    u16 yaw;

    target[0] = ((ActionSpot *)(pl)->spot)->x;
    target[1] = ((ActionSpot *)(pl)->spot)->y;
    target[2] = ((ActionSpot *)(pl)->spot)->z;
    yaw = ((ActionSpot *)(pl)->spot)->angle;
    Vec_Copy(&pl->moveTo[0], (f32 *) target);
    TownPc_StartMoveTo(pl, 0x14);
    pl->targetYaw = yaw;
    gTownMenuOpen = 1;
}

void TownMsg_SendContestSeatFree(Player *arg0);
void TownMsg_SendPlace(Player *obj);
extern u8 gContestSeatId;

/*
 * The local player's contest seat in AREA_TAVERN, once the host has
 * answered. If the mode is no longer the seat wait the seat is released
 * (to the others too) and control returns. Otherwise finds the contest
 * seat spot with id gContestSeatId (at most 20 spots), walks there, sends the
 * seat's position as the player's position, and starts contest state 0x70
 * for seat 1, else 0x71.
 */
void Contest_TakeSeat(void)
{
    ActionSpot *spot = (ActionSpot *)ActSpot_GetList(gScene.area);
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 i;
    f32 point[3];
    u16 ang;

    if (gScene.area != AREA_TAVERN) {
        return;
    }
    if (gTownMode != TOWN_MODE_CONTEST_SEAT_WAIT) {
        gTownMenuOpen = 0;
        TownMsg_SendContestSeatFree(pl);
        gTownMode = TOWN_MODE_FREE;
        gFieldControl = 1;
        return;
    }
    for (i = 0; 0 < spot->kind && spot != NULL;) {
        if (spot->kind == 0xE && spot->type == gContestSeatId) {
            break;
        }
        i++;
        spot++;
        if (i >= 0x14 || spot == NULL || spot->kind == 0) {
            gTownMenuOpen = 0;
            gTownMode = TOWN_MODE_FREE;
            Actor_ReturnToIdle(pl, 0, 0x14, 0);
            return;
        }
    }
    gFieldControl = 0;
    gTownMode = TOWN_MODE_CONTEST_SEATED;
    point[0] = spot->x;
    point[1] = spot->y;
    point[2] = spot->z;
    ang = spot->angle;
    Vec_Copy(pl->moveTo, point);
    TownPc_StartMoveTo(pl, 0x14);
    *(Vec3f *)point = *(Vec3f *)pl->pos;
    *(Vec3f *)pl->pos = *(Vec3f *)pl->moveTo;
    TownMsg_SendPlace(pl);
    *(Vec3f *)pl->pos = *(Vec3f *)point;
    pl->targetYaw = ang;
    if (gContestSeatId == 1) {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, 0x70);
    } else {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, 0x71);
    }
}

void TownMsg_SendSeatFree(Player *pl);
s32 Area_IsTownHall(s32 area);
extern u8 gTakeSpotId;

/*
 * The local player takes the table seat gTakeSpotId. In the areas
 * Area_IsTownHall accepts the mode must still be TOWN_MODE_SEAT_WAIT (else
 * the seat is released to the others and the mode freed), and the seat
 * spot (kind 1) is looked up and announced; offline elsewhere a seat of
 * kind 1 or 0xA is looked up. Online outside those areas the player's own
 * spot is used. Walks there, turns to the spot's angle and sits down; the
 * town mode is freed unless the drink contest runs.
 */
void TownPc_TakeSeat(void)
{
    ActionSpot *seat = (ActionSpot *)ActSpot_GetList(gScene.area);
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 i;
    f32 target[3];
    u16 ang;

    if (Area_IsTownHall(gScene.area) == 1) {
        if (gTownMode != TOWN_MODE_SEAT_WAIT) {
            gTownMenuOpen = 0;
            if (Area_IsOnlineHouse(gScene.area) == 1) {
                TownMsg_SendHouseSeatFree(pl);
            } else {
                TownMsg_SendSeatFree(pl);
            }
            gTownMode = TOWN_MODE_FREE;
            return;
        }
        for (i = 0; 0 < seat->kind && seat != NULL;) {
            if (seat->kind == 1 && seat->type == gTakeSpotId) {
                break;
            }
            i++;
            seat++;
            if (i >= 0x14 || seat == NULL || seat->kind == 0) {
                return;
            }
        }
        TownMsg_SendPlace(pl);
    } else if (Game_IsOnline() == 0) {
        for (i = 0; 0 < seat->kind && seat != NULL;) {
            if ((seat->kind == 1 || seat->kind == 0xA) && seat->type == gTakeSpotId) {
                break;
            }
            i++;
            seat++;
            if (i >= 0x1E || seat == NULL) {
                return;
            }
        }
    }
    gTownMenuOpen = 0;
    if (gTownMode != TOWN_MODE_DRINK_CONTEST) {
        gTownMode = TOWN_MODE_FREE;
    }
    if (Area_IsTownHall(gScene.area) == 0 && Game_IsOnline() == 1) {
        target[0] = ((ActionSpot *)pl->spot)->x;
        target[1] = ((ActionSpot *)pl->spot)->y;
        target[2] = ((ActionSpot *)pl->spot)->z;
        ang = ((ActionSpot *)pl->spot)->angle;
    } else {
        target[0] = seat->x;
        target[1] = seat->y;
        target[2] = seat->z;
        ang = seat->angle;
    }
    Vec_Copy(pl->moveTo, target);
    TownPc_StartMoveTo(pl, 0x14);
    pl->targetYaw = ang;
    Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SIT_DOWN, 0);
}

/* The file's data: the action-spot lists, then the gesture states and the sleep effect's offset. */
u32 gAreaActionSpots_0[56] = {
#include "lobby/00801DE0.inc"
};
u32 gAreaActionSpots_1[60] = {
#include "lobby/00801EC0.inc"
};
u32 gAreaActionSpots_2[24] = {
#include "lobby/00801FB0.inc"
};
u32 gAreaActionSpots_3[128] = {
#include "lobby/00802010.inc"
};
u32 gAreaActionSpots_4[92] = {
#include "lobby/00802210.inc"
};
u32 gAreaActionSpots_5[104] = {
#include "lobby/00802380.inc"
};
u32 gAreaActionSpots_14[36] = {
#include "lobby/00802520.inc"
};
u32 gAreaActionSpots_10[36] = {
#include "lobby/008025B0.inc"
};
u32 gAreaActionSpots_6[108] = {
#include "lobby/00802640.inc"
};
u32 gAreaActionSpots_7[20] = {
#include "lobby/008027F0.inc"
};
u32 gAreaActionSpots_11[44] = {
#include "lobby/00802840.inc"
};
u32 gAreaActionSpots_12[68] = {
#include "lobby/008028F0.inc"
};
u32 gAreaActionSpots_13[128] = {
#include "lobby/00802A00.inc"
};
s32 gAreaActionSpots[16] = {
    (s32)gAreaActionSpots_0, (s32)gAreaActionSpots_1, (s32)gAreaActionSpots_2, (s32)gAreaActionSpots_3,
    (s32)gAreaActionSpots_4, (s32)gAreaActionSpots_5, (s32)gAreaActionSpots_6, (s32)gAreaActionSpots_7,
    (s32)gAreaActionSpots_7, (s32)gAreaActionSpots_7, (s32)gAreaActionSpots_10, (s32)gAreaActionSpots_11,
    (s32)gAreaActionSpots_12, (s32)gAreaActionSpots_13, (s32)gAreaActionSpots_14, 0,
};
s32 gTownGestureStates[4] = {
#include "lobby/00802C40.inc"
};
Vec3f gBedSleepFxOffset = {
#include "lobby/00802C50.inc"
};
