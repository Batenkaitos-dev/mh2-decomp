/*
 * The town players' state handlers of group 0 (enum TownPcState,
 * TownPc_State*), the first half: idle, walk and run, the item hand-over
 * between two players, the bed, the drunk idle, the table seats with their
 * d-pad poses and drinks, the kitchen's order and the meal's three ways of
 * rising; then the walks to an action spot (the player's own, a contest
 * seat, a table seat taken with TOWN_MODE_SEAT_WAIT). The second half and
 * their dispatcher, TownPc_RunActionState, follow in the next file. The
 * file's data is the sleep effect's offset (gBedSleepFxOffset).
 *
 * Built at optimization level 4, as the town players' core before it:
 * TownPc_StateGrabbed matches only at level 4.
 */

#include "common.h"
#include "lobby_menu.h"
#include "quest.h"
#include "player.h"
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
#include "village_cond.h"
#include "town_player_states.h"
#include "area_draw.h"
#include "bgm.h"
#include "bone_effects.h"
#include "camera.h"
#include "engine_math.h"
#include "game_util.h"
#include "hud.h"
#include "item_shop.h"
#include "kitchen.h"
#include "lobby_net.h"
#include "lobby_queries.h"
#include "lobby_world.h"
#include "my_house.h"
#include "rand.h"
#include "town_gesture_drink.h"
#include "town_input.h"
#include "town_players.h"

#pragma optimization_level 4
extern s32 gTownMode;
extern s32 gTownMenuOpen;
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
extern NetUser gNetUsers[];

/* Clears lookFound and turns lookYaw back toward 0 by ANGLE_TURN / 64 a frame. */
void TownPc_UnwindLookYaw(Player *pl)
{
    u16 yaw;
    s32 sum;

    pl->lookFound = 0;
    yaw = *(u16 *)&pl->lookYaw;
    if (yaw != 0) {
        sum = yaw + 0x400;
        if (sum < 2049 && sum >= 0) {
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
    if (pl->motionTime >= 240 && pl->gender == 1 && pl->motionId[0] != PL_MOT_IDLE_ALT) {
        Player_SetMotionPair(pl, PL_MOT_IDLE_ALT, 2, 0);
    } else {
        motion = pl->motionId[0];
        if (motion != 1 && motion != 63) {
            if (motion == 207 || motion == 622) {
                Actor_ReturnToIdle(pl, 0, 12, 0);
            } else {
                Actor_ReturnToIdle(pl, 0, 4, 0);
            }
        }
    }
    TownPc_CheckAction(pl);
}

/*
 * The walk states, by variant: 1 the walk (state 3), 6 the run loop (state
 * 5), 9 the drunk walk (TOWN_PC_DRUNK_WALK), 0 a run start (no dispatcher passes
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
        pl->stateVar1 = 10U;
        pl->timer = 10;
        Actor_SetPosture(pl, 0, 0, 0);
        Actor_SetFlag(pl, 0x1000);
        switch (variant) {
        case 0:
            if (pl->drunk != 0) {
                if (pl->motionId[0] != PL_MOT_DASH_TIRED) {
                    Actor_SetMotion(pl, PL_MOT_DASH_TIRED, 6, 0);
                }
            } else if (pl->motionId[0] != PL_MOT_RUN) {
                Actor_SetMotion(pl, PL_MOT_RUN, 6, 0);
            }
            Actor_SetFlag(pl, 0x02000000);
            break;
        case 1:
            if (pl->drunk != 0) {
                if (pl->motionId[0] != 600) {
                    Actor_SetMotion(pl, 600, 4, 0);
                }
            } else if (pl->gender == 1) {
                Actor_SetMotion(pl, 64, 4, 0);
            } else {
                Actor_SetMotion(pl, PL_MOT_WALK, 4, 0);
            }
            break;
        case 6:
            pl->dashDelay = 30;
            if (pl->drunk != 0) {
                if (pl->motionId[0] != PL_MOT_DASH_TIRED) {
                    Actor_SetMotion(pl, PL_MOT_DASH_TIRED, 4, 0);
                }
            } else if (pl->motionId[0] != PL_MOT_RUN) {
                Actor_SetMotion(pl, PL_MOT_RUN, 0, 58);
            }
            Actor_SetFlag(pl, 0x02000000);
            break;
        case 9:
            Actor_SetMotion(pl, 600, 4, 0);
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
    if ((pl->motionId[0] == PL_MOT_RUN) && (kind < 5)) {
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
        Actor_SetFlag(pl, 1536);
        pl->motionTime = 0;
        pl->dashDelay = 30;
        if (pl->drunk != 0) {
            if (pl->motionId[0] != PL_MOT_DASH_TIRED) {
                Player_SetMotionPair(pl, PL_MOT_DASH_TIRED, 4, 0);
            }
            pl->motion[1].step = pl->motion[0].step = 2.0f;
        } else if (pl->motionId[0] == PL_MOT_RUN) {
            pl->motion[1].step = pl->motion[0].step = 2.5f;
        } else {
            Player_SetMotionPair(pl, PL_MOT_RUN, 0, 58);
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
                Actor_ReturnToIdle((void *) pl, 0, 10, 0);
            }
            break;
        }
    }
}

s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 SavePouch_Count(u16 item);

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
        if (Player_IsState(giver, 0, 10) == 0 || giver->offerTo != pl->index) {
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
                pl->offerNoticeTimer = 90;
            }
        } else if (SavePouch_Room(giver->offerItem) >= giver->offerCount) {
            pl->offerItem = giver->offerItem;
            pl->offerCount = giver->offerCount;
            if (Player_IsLocal(pl) == 1) {
                TownMsg_SendOfferTake(&gLobby->members[i], giver);
            }
            Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_TAKE_ITEM_WAIT, 0);
        } else {
            Notice_Push(1, 15, (s16)giver->offerItem);
            pl->offerNoticeTimer = 90;
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
        Player_SetMotionPair(pl, PL_MOT_OFFER_ITEM, 4, 0);
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
        if (pl->motionId[0] != PL_MOT_IDLE) {
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
                Actor_SetStateNow((void *) pl, 0, 13, 0);
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
        Player_SetMotionPair(pl, PL_MOT_OFFER_ITEM_END, 4, 0);
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
        Player_SetMotionPair(pl, PL_MOT_TAKE_ITEM, 4, 0);
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
            Actor_SetStateNow((void *) pl, 0, 15, 0);
        }
        break;
    }
}

extern Vec3f gBedSleepFxOffset;
void Effect06_SpawnAt(void *obj, s16 type, u8 bone, f32 *pos, f32 scale);

/*
 * Runs the bed's sleep loop: a random stir one time in 64, back to the
 * loop when it ends, and, while the player is shown to the local player,
 * the sleep effect above the head three times a second at the start of
 * each second.
 */
void TownPc_LoopBedSleep(Player *pl)
{
    Vec3f fxOffset = gBedSleepFxOffset;

    if (pl->motionId[0] == 427 && ((u16)Rand_Next(1) & 0x3F) == 0) {
        Actor_SetMotion(pl, PL_MOT_BED_SLEEP_STIR, 0, 0);
    } else if (pl->motionId[0] == 428 && pl->motion[0].playing == 0) {
        Actor_SetMotion(pl, PL_MOT_BED_SLEEP, 0, 0);
    }
    if (TownPc_IsInMyArea(pl) != 0) {
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 20, (f32 *) &fxOffset, 0.6f);
            break;
        }
    }
}

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
        pl->timer = 60;
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
        if (pl->motionId[0] != 257) {
            Actor_SetMotion(pl, 257, 16, 30);
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
            Actor_SetState(pl, 0, 19U);
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
        Actor_SetMotion(pl, 258, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_DOWN, 4, 54);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Actor_SetStateNow((void *) pl, 0, 29, 0);
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
    case 20:
    case 28:
    case 33:
    case 34:
    case 36:
    case 37:
    case 38:
    case 40:
    case 22:
    case 8:
    case 7:
    case 9:
    case 41:
    case 30:
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

extern u8 gTownCamEndOnClose;

/*
 * Sits down on a table seat; alreadySeated 1 (TOWN_PC_SEATED) skips the
 * sit-down. After sitting down the state becomes TOWN_PC_SEATED; the local
 * player in their own house (or offline outside AREA_HUB_0x89) opens the
 * kitchen when no meal was eaten yet. Seated, circle stands up, and in the
 * areas Area_IsTownHall accepts the other buttons pick a seated action
 * (cross, square, and the four d-pad poses); in AREA_TOWN_ARENA
 * LobbyBgm_SetStageCam turns its stage camera on while seated and off on
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
            pl->timer = 3600;
            pl->timer = 60;
            if (pl->motionId[0] != 609) {
                Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            }
        } else {
            Actor_SetMotion(pl, PL_MOT_TOWN_SIT, -4, 0);
            if (pl->index == gScene.localPlayer) {
                gFieldControl = 0;
                gTownCamEndOnClose = 1;
                if (gScene.area == AREA_TOWN_ARENA) {
                    LobbyBgm_SetStageCam(1);
                }
            }
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->timer = 3600;
            pl->timer = 60;
            pl->step++;
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 4, 0);
            pl->state = 37;
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
                    LobbyBgm_SetStageCam(0);
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
                } else if (gScene.area == AREA_TOWN_ARENA && LobbyBgm_GetTrack() != -1 && LobbyBgm_IsStageCamOn() == 0) {
                    LobbyBgm_SetStageCam(1);
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
        Actor_SetMotion(pl, 610, 4, 0);
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
            if (pl->motionId[0] != 427) {
                Actor_SetMotion(pl, PL_MOT_BED_SLEEP, 0, 0);
            }
            pl->timer = 360;
            break;
        case 1:
            TownPc_LoopBedSleep(pl);
            timer = pl->timer - 1;
            pl->timer = timer;
            if (timer <= 0) {
                Actor_SetStateNow((void *) pl, 0, 16, 0);
            }
            break;
        }
    }
}

extern u8 gOffersRefreshPending;

/* Sleeping (k 0 goes to bed, k 1 wakes up in it): the motions, the saved
 * bed position (+0x438 +0x14, angle +0x22), the jingle, then back to state
 * 0 at the bed (in AREA_HUB_0x57, turned around). Other players only idle. */
/* A lobby player's block at Player +0x438 (Player.work, whose PlayerWork it extends), as TownPc_StateRest (the bed) reads it. */
typedef struct TownPcWork {
    u8 unk00[20];
    Vec3f bedPos;               /* 0x14: the position kept while the player sleeps: saved at frames 166 and 260 of the sleep motion, put back at frame 198 and in step 3 */
    s16 jingleTimer;            /* 0x20: step 4 in AREA_HUB_0x57: frames until the jingle (160) */
    u16 bedYaw;                 /* 0x22: the yaw kept with bedPos */
    u8 unk24[128 - 36];
} TownPcWork;
typedef char TownPcWork_size_check[(sizeof(TownPcWork) == 128) ? 1 : -1];

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
                Actor_SetMotion(pl, 647, 0, 0);
            } else {
                Actor_SetMotion(pl, 648, 0, 0);
            }
        } else if (shortForm == 0) {
            Actor_SetMotion(pl, 250, 0, 0);
            pl->timer = 2;
        } else {
            Actor_SetMotion(pl, 251, 0, 0);
            pl->step = 4;
        }
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 198.0f)) {
            Actor_SetMotion(pl, 250, 0, 166);
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
            pl->timer = 60;
            restWork->bedPos = *(Vec3f *)pl->pos;
            restWork->bedYaw = pl->rot[1];
        }
        break;
    case 3:
        if (--pl->timer <= 0) {
            pl->step++;
        } else {
            Actor_SetMotion(pl, 250, 0, 260);
            *(Vec3f *)pl->pos = restWork->bedPos;
            *(Vec3f *)pl->moveTo = *(Vec3f *)pl->pos;
            pl->targetYaw = pl->rot[1] = restWork->bedYaw;
        }
        break;
    case 4:
        if (shortForm == 0 && gScene.area == AREA_HUB_0x57) {
            restWork->jingleTimer++;
            if (restWork->jingleTimer == 160) {
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
                Player_ClearFlag(pl, PLF_NO_ROOT_MOVE);
                pl->rot[1] = pl->targetYaw = pl->rot[1] + ANGLE_TURN / 2;
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
        Actor_SetMotion(pl, 618, 6, 0);
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
        Actor_SetMotion(pl, 642, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetState(pl, 0, 37U);
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
        if (motion == 216) {
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
            Actor_SetMotion(pl, PL_MOT_EXHAUSTED_PANT, 4, 144);
        }
        TownPc_CheckAction((void *) pl);
        break;
    case 3:
        if (pl->motionTime >= 240) {
            Actor_SetState(pl, 0, 18U);
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
        Actor_SetMotion(pl, 602, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 37U);
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
        Actor_SetMotion(pl, 604, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_FISH_PULLED_DOWN, 4, 44);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_DOWN, 6, 36);
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
        Actor_SetMotion(pl, 603, 6, 0);
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
        Actor_SetMotion(pl, 611, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 37U);
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
        Actor_SetMotion(pl, 612, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 37U);
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
        Actor_SetMotion(pl, 617, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 37U);
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
        Actor_SetMotion(pl, 616, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->timer = 60;
            pl->step = (u8) (pl->step + 1);
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            timer = pl->timer;
            if (timer <= 0) {
                if (pl->input.pressed & (PAD_CIRCLE | PAD_CROSS | PAD_DPAD)) {
                    Actor_SetState(pl, 0, 22U);
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
        Actor_SetMotion(pl, 613, 6, 0);
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
        Actor_SetMotion(pl, 614, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 37U);
        }
        break;
    }
}

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
        Actor_SetMotion(pl, 610, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            if (goodMeal == 0) {
                Actor_SetMotion(pl, 637, 6, 0);
                break;
            }
            Actor_SetMotion(pl, PL_MOT_ITEM_POSE, 6, 20);
            Effect06_SpawnAtBone(pl, 0, 8, 10U, 4.0f);
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
        Actor_SetMotion(pl, 615, 6, 0);
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
        Actor_SetMotion(pl, 643, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 37U);
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
        Actor_SetMotion(pl, 635, 6, 0);
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
        Actor_SetMotion(pl, 636, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            Actor_SetMotion(pl, PL_MOT_LIE_FACE_UP, 0, 146);
            pl->targetYaw += ANGLE_TURN / 4;
            pl->rot[1] += ANGLE_TURN / 4;
            pl->timer = ((u16)Rand_Next(1) & 0x1F) + 60;
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
void TownPc_State43(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 605, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 606, 0, 0);
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
    TownPc_StartMoveTo(pl, 20);
    pl->targetYaw = yaw;
    gTownMenuOpen = 1;
}

extern u8 gContestSeatId;

/*
 * The local player's contest seat in AREA_TAVERN, once the host has
 * answered. If the mode is no longer the seat wait the seat is released
 * (to the others too) and control returns. Otherwise finds the contest
 * seat spot with id gContestSeatId (at most 20 spots), walks there, sends the
 * seat's position as the player's position, and starts TOWN_PC_CONTEST_SEAT_A
 * for seat 1, else TOWN_PC_CONTEST_SEAT_B.
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
        if (spot->kind == 14 && spot->type == gContestSeatId) {
            break;
        }
        i++;
        spot++;
        if (i >= 20 || spot == NULL || spot->kind == 0) {
            gTownMenuOpen = 0;
            gTownMode = TOWN_MODE_FREE;
            Actor_ReturnToIdle(pl, 0, 20, 0);
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
    TownPc_StartMoveTo(pl, 20);
    *(Vec3f *)point = *(Vec3f *)pl->pos;
    *(Vec3f *)pl->pos = *(Vec3f *)pl->moveTo;
    TownMsg_SendPlace(pl);
    *(Vec3f *)pl->pos = *(Vec3f *)point;
    pl->targetYaw = ang;
    if (gContestSeatId == 1) {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, 112);
    } else {
        Actor_SetState(pl, TOWN_PC_GROUP_ACTION, 113);
    }
}

extern u8 gTakeSpotId;

/*
 * The local player takes the table seat gTakeSpotId. In the areas
 * Area_IsTownHall accepts the mode must still be TOWN_MODE_SEAT_WAIT (else
 * the seat is released to the others and the mode freed), and the seat
 * spot (kind 1) is looked up and announced; offline elsewhere a seat of
 * kind 1 or 10 is looked up. Online outside those areas the player's own
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
            if (i >= 20 || seat == NULL || seat->kind == 0) {
                return;
            }
        }
        TownMsg_SendPlace(pl);
    } else if (Game_IsOnline() == 0) {
        for (i = 0; 0 < seat->kind && seat != NULL;) {
            if ((seat->kind == 1 || seat->kind == 10) && seat->type == gTakeSpotId) {
                break;
            }
            i++;
            seat++;
            if (i >= 30 || seat == NULL) {
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
    TownPc_StartMoveTo(pl, 20);
    pl->targetYaw = ang;
    Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_SIT_DOWN, 0);
}

/* This file's initialised data. */
Vec3f gBedSleepFxOffset = {
#include "lobby/00802C50.inc"
};
