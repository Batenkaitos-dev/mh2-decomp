/*
 * The town players' update, run each frame: TownPc_UpdateAll steps every
 * shown player, and TownPc_Update runs one player's state group; the
 * state handlers of group 0 (enum TownPcState) are the next files. With
 * them, whether a player is free to act in the current town mode
 * (TownPc_IsFree) and the turn toward a target yaw (Actor_TurnToTarget).
 *
 * Built at optimization level 4, which TownPc_UpdateAll needs.
 */

#include "common.h"
#include "lobby_menu.h"
#include "player.h"
#include "em.h"
#include "objhead.h"
#include "area.h"
#include "town_player_update.h"
#include "arm_wrestling.h"
#include "frame.h"
#include "hair_menu.h"
#include "town_gestures.h"
#include "town_player_activities.h"
#include "town_player_states.h"
#include "town_players.h"
#include "lobby_net.h"

#pragma optimization_level 4

extern s32 gTownMode;

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

struct TriHdr;

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
 * target and is kept within 2560 of upright either way. Another member's
 * player in the walk state is turned by TownPc_SmoothRemoteYaw instead.
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
        rate = 1820;
    } else if (Actor_IsState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_RUN) != 0) {
        rate = 1456;
    } else if (Actor_IsState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_TURN_AROUND) != 0) {
        rate = 0;
    } else {
        rate = 4004;
    }
    goalYaw = pl->targetYaw;
    diff = (u16)(goalYaw - (u16)pl->rot[1]);
    switch (pl->motionId[0]) {
    case 39:
        rate = 1820;
        leanStep = 128;
        break;
    case 3:
        leanStep = 128;
        break;
    case 4:
        leanStep = 160;
        break;
    default:
        leanStep = 0;
        break;
    }
    if ((u16)(diff + rate) < rate * 2) {
        pl->rot[1] = goalYaw;
        pl->unk2B8 = 0;
        leanNow = pl->lean;
        if (leanNow + 768 <= 1536) {
            pl->lean = 0;
        } else if (leanNow < ANGLE_TURN / 2) {
            pl->lean = leanNow - 768;
        } else {
            pl->lean = leanNow + 768;
        }
    } else if (diff < ANGLE_TURN / 2) {
        pl->rot[1] = (u16)(pl->rot[1] + rate);
        pl->lean -= leanStep;
    } else {
        pl->rot[1] = (u16)(pl->rot[1] - rate);
        pl->lean += leanStep;
    }
    leanNow = pl->lean;
    if (leanNow <= ANGLE_TURN - 2560 && leanNow >= 2560) {
        if (leanNow >= ANGLE_TURN / 2) {
            pl->lean = ANGLE_TURN - 2560;
        } else {
            pl->lean = 2560;
        }
    }
}
