/*
 * The lobby's town players: the local hunter and the other members shown in
 * the village, and the actor core they share with the townspeople.
 *
 * - The motion starts the town's actors share (Actor_SetMotion*), for
 *   players and townspeople (Em) alike: the motion of each layer, its
 *   blend frames and its start frame.
 * - The actor core that town players and townspeople (Em slots) share
 *   through ObjHead: the timers, state changes (Actor_SetState*), the
 *   return to idle, the posture and flag helpers and the move-to steps.
 *   They are the town's copies of the field's Player_SetState,
 *   Player_ReturnToIdle and Player_SetPosture. A local player's state
 *   change is sent to the other members (TownMsg_SendStateChange).
 * - The pad input, the move kind (walk, run) and the turning.
 * - The area's action spots (ActionSpot lists, gAreaActionSpots) and
 *   whether one is usable.
 *
 * The lock-on, the gesture request and drink count, the house queries
 * (MyHouse_*) and the players' per-frame update (TownPc_UpdateAll) follow,
 * each in a file of its own; then the state handlers of group 0 (enum
 * TownPcState).
 *
 * The data: the action-spot lists of each area set.
 */
#include "common.h"
#include "lobby_menu.h"
#include "quest.h"
#include "player.h"
#include "em.h"
#include "save.h"
#include "pad.h"
#include "objhead.h"
#include "lobby_npc.h"
#include "area.h"
#include "village_cond.h"
#include "town_players.h"
#include "camera.h"
#include "engine_math.h"
#include "game_util.h"
#include "hud.h"
#include "lobby_queries.h"
#include "my_house.h"
#include "rand.h"
#include "town_area_change.h"
#include "town_input.h"
#include "vec_math.h"
#include "libc.h"

/* Starts motion on layer 0 of obj (a Player or an Em) and, unless it has
 * exactly one motion layer, motion + 100 on layer 1, each with blend
 * frames blend and start frame startFrame; Player.unk771 is cleared first, as
 * every motion start clears it. */
void Actor_SetMotion(void *obj, s32 motion, s32 blend, s32 startFrame)
{
    u16 layers = ((ObjHead *)obj)->motionLayerCount;

    if (layers <= 1 && layers != 0) {
        ((Player *)obj)->unk771 = 0;
        Actor_SetMotionLayer(obj, motion, blend, startFrame, 0);
    } else {
        ((Player *)obj)->unk771 = 0;
        Actor_SetMotionLayer(obj, motion, blend, startFrame, 0);
        Actor_SetMotionLayer(obj, motion + 100, blend, startFrame, 1);
    }
}

/* Starts motion on layer layer of obj (a Player or an Em), with blend
 * frames blend and start frame startFrame, clearing Player.unk771 first. */
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 blend, s32 startFrame, s32 layer)
{
    ((Player *)obj)->unk771 = 0;
    Actor_SetMotionLayer(obj, motion, blend, startFrame, layer);
}

/* Sets layer layer's motion of obj: the motion id, the start frame
 * startFrame and the blend frames (halved); motionStarted cleared, so the actor's update
 * begins it. */
void Actor_SetMotionLayer(ObjHead *obj, s32 motion, s32 blend, s32 startFrame, s32 layer)
{
    obj->motionId[layer] = motion;
    obj->motionBlend[layer] = blend / 2;
    obj->motionFrame[layer] = startFrame;
    obj->motionStarted[layer] = 0;
}

/* Built at optimization level 4, as the next files: their TownPc_UpdateAll
 * matches only at 4, and the state handlers, built at 4 as well, use the
 * helpers here (Actor_ReturnToIdle, Actor_SetPosture, Actor_SetState). */
#pragma optimization_level 4

/* No prototype: Actor_NeedsStateSend calls it with its loop counter still in a1. */
/* unit's own declaration: Area_HasTownPcs ignores a1, which holds var_a1 at Actor_NeedsStateSend (the player loop count, 10) */
s32 Area_HasTownPcs();
extern LobbyMenu *gLobbyMenu;

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
        if ((s32)obj->motionTime >= 100000) {
            obj->motionTime = 10000;
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

static void Actor_RestartMotionTime(void *obj, s16 flag);
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
        if (pl->motionId[0] != PL_MOT_IDLE) {
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
    case 15:
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
 * Starts the idle motions (PL_MOT_IDLE, and PL_MOT_IDLE + 100 on the second
 * layer) with the given blend and start frame, enters the idle state
 * (group 0, state 1) and sends the change when Actor_NeedsStateSend says
 * so.
 */
void Actor_StartIdle(void *obj, s32 blend, s32 startFrame)
{
    ObjHead *head = obj;
    head->motionFrame[0] = startFrame;
    head->motionFrame[1] = startFrame;
    head->motionId[0] = PL_MOT_IDLE;
    head->motionId[1] = PL_MOT_IDLE + 100;
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

void TownMsg_SendStateNow(Player *p);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void TownMsg_SendOfferEnd(void);
u8 Area_GetSetIndex(u16 area);
extern ActionSpot *gAreaActionSpots[];

/*
 * How the player moves this frame (TOWN_MOVE_*): not at all, a walk (the
 * stick tilted PL_STICK_HELD or more), a walk with the stick fully tilted
 * (120 or more), or a dash (PAD_R2 held, standing, the weapon sheathed,
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
        if (tilt >= 120) {
            kind = TOWN_MOVE_WALK_FULL;
        } else if (tilt >= 85) {
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

/*
 * Returns 1 when the current area's event has not been seen yet
 * (AREA_HUB_0xAC without EVENT_FLAG_MINE_STRONG_PICK, AREA_HUB_0x57
 * without event flag 0x8A), else 0.
 */
s32 ActSpot_AreaEventUnseen(void)
{
    switch (gScene.area) {
    case AREA_HUB_0xAC:
        if (Save_TestEventFlag(137) == 0) {
            return 1;
        }
        break;
    case AREA_HUB_0x57:
        if (Save_TestEventFlag(138) == 0) {
            return 1;
        }
        break;
    }
    return 0;
}

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
    case 12:
        return Actor_IsFacingWithin(pl, &spot->x, 90);
    case 17:
        if (gLobby->questTaken == 0) {
            return 0;
        }
        break;
    case 1:
        if (Game_IsOnline() == 0) {
            npc = Npc_Find(16);
            if (npc == NULL) {
                return 0;
            }
            if (Cond_Check(21) == 0) {
                return 0;
            }
            return npc->state == 138 ? 1 : 0;
        }
        break;
    case 2:
        if (Game_IsOnline() == 1) {
            npc = Npc_Find(51);
        } else {
            npc = Npc_Find(25);
        }
        if (npc == NULL) {
            return 0;
        }
        break;
    case 14:
        if (Game_IsOnline() == 0) {
            npc = Npc_Find(5);
            if (Cond_Check(1) == 0) {
                return 0;
            }
            if (Cond_Check(65) == 0) {
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
    case 13:
        if (gScene.area == AREA_HUB_0x89) {
            npc = Npc_Find(20);
        } else if (gScene.area == AREA_ONLINE_VILLAGE) {
            npc = Npc_Find(34);
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
    case 24:
        switch (spot->type) {
        case 136:
            if (Cond_Check(COND_HR_GATE_2_HR_31) == 0) {
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
    case 10:
        if (gLobby->mealEaten != 0) {
            return 0;
        }
        if (Game_IsOnline() == 1) {
            if (gHouseCloseTimer > 0) {
                return 0;
            }
        } else if (gSave.awayFromVillage == 0 && Npc_Find(10) != NULL && spot->type == 1) {
            return 0;
        }
        break;
    }
    return 1;
}

extern NetUser gNetUsers[];

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
        if (head->motionId[0] != PL_MOT_IDLE) {
            Actor_ReturnToIdle(head, 0, 4, 0);
        }
    }
    if (Actor_NeedsStateSend(head) != 0) {
        TownMsg_SendStateChange(head);
    }
}

/* The file's data: the action-spot lists of each area set. */
ActionSpot gAreaActionSpots_0[9] = {
#include "lobby/00801DE0.inc"
};
ActionSpot gAreaActionSpots_1[10] = {
#include "lobby/00801EC0.inc"
};
ActionSpot gAreaActionSpots_2[4] = {
#include "lobby/00801FB0.inc"
};
ActionSpot gAreaActionSpots_3[21] = {
#include "lobby/00802010.inc"
};
ActionSpot gAreaActionSpots_4[15] = {
#include "lobby/00802210.inc"
};
ActionSpot gAreaActionSpots_5[17] = {
#include "lobby/00802380.inc"
};
ActionSpot gAreaActionSpots_14[6] = {
#include "lobby/00802520.inc"
};
ActionSpot gAreaActionSpots_10[6] = {
#include "lobby/008025B0.inc"
};
ActionSpot gAreaActionSpots_6[18] = {
#include "lobby/00802640.inc"
};
ActionSpot gAreaActionSpots_7[3] = {
#include "lobby/008027F0.inc"
};
ActionSpot gAreaActionSpots_11[7] = {
#include "lobby/00802840.inc"
};
ActionSpot gAreaActionSpots_12[11] = {
#include "lobby/008028F0.inc"
};
ActionSpot gAreaActionSpots_13[21] = {
#include "lobby/00802A00.inc"
};
ActionSpot *gAreaActionSpots[16] = {
    gAreaActionSpots_0, gAreaActionSpots_1, gAreaActionSpots_2, gAreaActionSpots_3,
    gAreaActionSpots_4, gAreaActionSpots_5, gAreaActionSpots_6, gAreaActionSpots_7,
    gAreaActionSpots_7, gAreaActionSpots_7, gAreaActionSpots_10, gAreaActionSpots_11,
    gAreaActionSpots_12, gAreaActionSpots_13, gAreaActionSpots_14, NULL,
};
