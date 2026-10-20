/*
 * The lobby's town players, second half of the group-0 states (enum
 * TownPcState): fishing (the wait, the cast, the reels), the shooting
 * gallery's aim, the arm-wrestling contest (seat, grip, win, lose, draw),
 * digging at a Poogie dig spot, the drinking contest's reactions at the table
 * (miss, lose, win) and the walk to the barber's chair. Then
 * TownPc_RunActionState, the switch that runs every state of the group,
 * the first half's included (the walking, seats, bed and meals of the
 * previous file).
 */
#include "common.h"
#include "lobby_menu.h"
#include "lobby_state.h"
#include "player.h"
#include "scene.h"
#include "lobby_globals.h"
#include "save.h"
#include "pad.h"
#include "lobby/town_player_activities.h"   /* the texts this file uses (tools/content_gen.py) */
#include "area.h"

/* prototypes */
void TownPc_StateIdle(Player *a);
void TownPc_StateWalk(Player *arg0, s32 arg1);
void TownPc_StateRun(Player *a);
void TownPc_StateRunStop(Player *arg0);
void TownPc_StateOfferItem(Player *pl);
void TownPc_StateTakeItemWait(Player *arg0);
void TownPc_StateOfferItemEnd(Player *arg0);
void TownPc_StateTakeItem(Player *arg0);
void TownPc_StateBedLieDown(Player *arg0);
void TownPc_StateBedSleep(Player *a, s32 k);
void TownPc_StateBedGetUp(Player *arg0, s32 arg1);
void TownPc_StateTurnAround(Player *arg0);
void TownPc_StateDrunkCollapse(Player *arg0);
void TownPc_StateDrunkGetUp(Player *arg0);
void TownPc_StateHairChair(Player *arg0);
void TownPc_StateSitDown(Player *a, s32 k);
void TownPc_StateStandUp(Player *arg0);
void TownPc_StateBedNap(Player *arg0);
void TownPc_StateRest(Player *a, s8 k);
void TownPc_StateGetUpFaceUp(Player *arg0);
void TownPc_StateSeatPoseRight(Player *arg0);
void TownPc_StateSeatPoseRightEnd(Player *arg0);
void TownPc_StateDrunkIdle(Player *arg0);
void TownPc_StateKitchenOrder(Player *arg0);
void TownPc_StateThrown(Player *arg0);
void TownPc_StateGrabbed(Player *arg0);
void TownPc_StateSeatDrink(Player *arg0);
void TownPc_StateSeatCross(Player *arg0);
void TownPc_StateSeatPoseLeftEnd(Player *arg0);
void TownPc_StateSeatPoseLeft(Player *arg0);
void TownPc_StateSeatPoseDown(Player *arg0);
void TownPc_StateSeatPoseDownEnd(Player *arg0);
void TownPc_StateMealRise(Player *a, s8 flag);
void TownPc_StateSeatPoseUp(Player *arg0);
void TownPc_StateSeatPoseUpEnd(Player *arg0);
void TownPc_StateMealServed(Player *arg0);
void TownPc_StateMealCollapse(Player *a);
void TownPc_State2B(Player *arg0);
;
;
void TownPc_StateFishReel(Player *pl, s32 k);
;
;
;
;
void TownPc_StateContestWin(Player *a);
void TownPc_StateContestLose(Player *a);
void TownPc_StateDig(Player *a);
;
;
;
void TownPc_StateWalkToHairChair(Player *a);
;
void TownPc_SteerToDest(Player *pl);
/* end of prototypes */

void Items_Take(s32 item, s32 n);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
s32 Player_IsLocal(void *obj);
void Player_SetFlag(void *obj, u32 flag);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_SetPosture(Player *p, u16 mode, s16 flag1, s16 flag2);
void Actor_ReturnToIdle(void *, s32, s32, s32);
s32 ItemGet_Reserve(s32, u16 *);
void Jingle_Play(void *obj, s32 id);
u8 Notice_Push(u8, s16, s16);

void Actor_SetStateNow(Player *, u8, u8, u16);
void Lure_Spawn(Player *pl, u8 k);

void TownPc_UpdateFishingInput(Player *pl);

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);

void Actor_SetPosture(void *a, u16 mode, s32 b, s32 c);
void TownMsg_SendStateNow(Player *p);
void TownMsg_SendContestSeatFree(Player *arg0);
extern s32 gTownMode;
extern s32 gTownMenuOpen;

void ActorParticles_SpawnSpray(void *arg0);

void Actor_SetState(void *, s32, s32);

s32 Motion_ReachedFrame(void *, s32, f32);

/*
 * Runs the local player's input while it waits with the line out: if its
 * state group is no longer the action group it returns to idle; on
 * PAD_CIRCLE it starts reeling in, with the catch when a fish is biting
 * (fishTimer above 0), else the empty reel that keeps or loses the bait by
 * baited. Does nothing for the other players.
 */
void TownPc_UpdateFishingInput(Player *pl)
{
    if (Player_IsLocal(pl) != 0) {
        if (pl->stateGroup != TOWN_PC_GROUP_ACTION) {
            Actor_ReturnToIdle(pl, 0, 4, 0);
            return;
        }
        if (pl->input.pressed & PAD_CIRCLE) {
            if (pl->fishTimer > 0) {
                Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, PL_TOWN_FISH_REEL, 0);
                return;
            }
            if (pl->baited == 1) {
                Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, PL_TOWN_FISH_REEL_BAITED, 0);
                return;
            }
            Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, PL_TOWN_FISH_REEL_EMPTY, 0);
        }
    }
}

/*
 * The fishing wait with the line out (motion PL_MOT_FISH_WAIT): marks
 * stateVar1 once a fish bites (fishTimer starts) and runs
 * TownPc_UpdateFishingInput each frame. The item-get ticket is renewed
 * every frame so the catch can be given later.
 */
void TownPc_StateFishWait(Player *pl, s32 unused)
{
    u8 step;

    ItemGet_Reserve((s32) pl, (u16 *)&pl->gotItem);
    pl->noPushTimer = 2;
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Player_SetPosture(pl, 0U, 0, 0);
        Player_SetFlag(pl, 0x80000U);
        pl->weaponOut = 0;
        pl->timer = 0;
        pl->stateVar1 = 0U;
        Player_SetMotionPair(pl, 0x326, 2, 0);
        break;
    case 1:
        if ((pl->fishTimer != 0) && (pl->stateVar1 == 0)) {
            pl->stateVar1 = 1U;
        }
        TownPc_UpdateFishingInput(pl);
        break;
    }
}

/*
 * The cast: plays PL_MOT_FISH_CAST, clears the bite timer and the caught
 * item, sets baited, spawns the lure at frame 104 and goes on to the
 * fishing wait when the motion ends.
 */
void TownPc_StateFishCast(Player *pl, s32 unused)
{
    u8 step;

    ItemGet_Reserve((s32) pl, (u16 *)&pl->gotItem);
    pl->noPushTimer = 2;
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Player_SetPosture(pl, 0U, 0, 0);
        Player_SetFlag(pl, 0x80000U);
        pl->weaponOut = 0;
        pl->fishTimer = 0;
        pl->gotItemId = 0;
        pl->baited = 1;
        Player_SetMotionPair(pl, 0x325, -2, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 104.0f) != 0) {
            Lure_Spawn(pl, 0);
        }
        if (pl->motion[0].playing == 0) {
            Actor_SetStateNow(pl, TOWN_PC_GROUP_ACTION, 0x61, 0);
        }
        break;
    }
}

extern s32 gPoogieDigSpots;
extern u8 gPoogieDigSpotIdx;
extern u8 gDigOverLimit;
void Save_RestorePouch(Player *p);
/* unit's own declaration: the definition takes an s16 key; declared s32 here, as the callers pass it, since with s16 TownPc_StateFishReel's call compiles differently */
s16 Pouch_GiveItem(Player *p, u16 item, s32 key, u8 flags);

/*
 * The reel with a fish on the line (PL_TOWN_FISH_REEL): reels in, lands
 * the catch, and at frame 120 the local player gets the catch notice and
 * jingle, loses one bait, and gets the fish (the pouch-full notice when it
 * has no room). Then idle.
 */
void TownPc_StateFishReel(Player *pl, s32 unused)
{
    u8 step;

    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    pl->noHitTimer = pl->noPushTimer = 2;
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = step + 1;
        pl->motionTime = 0;
        Player_SetPosture(pl, 0, 0, 0);
        Player_SetFlag(pl, 0x80000);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x327, 2, 0);
        break;
    case 1:
        if (Motion_PassedFrame(pl, 0, 118.0f) != 0) {
            pl->step++;
            Player_SetMotionPair(pl, 0x328, 2, 0);
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 120.0f) != 0 && Player_IsLocal(pl) == 1) {
            Notice_Push(1, 0xB, pl->gotItemId);
            Jingle_Play(pl, 7);
            Items_Take(pl->useItem, 1);
            Save_RestorePouch(pl);
            if (Pouch_GiveItem(pl, pl->gotItemId, pl->gotItem, 0) == 3) {
                Notice_Push(1, 3, pl->gotItemId);
            }
        }
        if (pl->motion[0].playing == 0) {
            Actor_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * The reel with nothing on the line (PL_TOWN_FISH_REEL_EMPTY / _BAITED);
 * variant 1 means the bait was kept. At frame 120 the local player gets a
 * notice; with the bait lost it also gets a jingle and loses one bait.
 * Then idle.
 */
void TownPc_StateFishReelEmpty(Player *pl, s32 baitKept)
{
    u8 step;

    ItemGet_Reserve((s32) pl, (u16 *)&pl->gotItem);
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Player_SetPosture(pl, 0U, 0, 0);
        Player_SetFlag(pl, 0x80000U);
        pl->weaponOut = 0;
        Player_SetMotionPair(pl, 0x329, 2, 0);
        break;
    case 1:
        if ((Motion_PassedFrame(pl, 0, 120.0f) != 0) && (Player_IsLocal(pl) == 1)) {
            if (baitKept == 0) {
                Jingle_Play(pl, 0xA);
                Items_Take((s32) pl->useItem, 1);
                Notice_Push(0, 3, 0);
            } else {
                Notice_Push(0, 0xD, 0);
            }
        }
        if (pl->motion[0].playing == 0) {
            Actor_ReturnToIdle(pl, 0, 2, 0);
        }
        break;
    }
}

/*
 * The shooting gallery's aim (TOWN_PC_GALLERY_AIM): stands and holds the
 * gallery gun's motion until the gallery changes the state.
 */
void TownPc_StateGalleryAim(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Player_SetPosture(pl, 0U, 0, 0);
        Actor_SetMotion(pl, 0x349, 4, 0);
        
    case 1:
        break;
    }
}

/*
 * Seated at the arm-wrestling table, waiting for the match (states 0x70
 * and 0x71; variant 1 is the second seat, with its own motion). After 60
 * frames the local player can leave with PAD_CROSS until the contest has
 * started (TOWN_MODE_CONTEST): the town mode goes back to free, field
 * control returns, and the state is sent.
 */
void TownPc_StateContestSeat(Player *pl, s32 seat)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->timer = 0x3C;
        Actor_SetPosture(pl, 0, 0, 0);
        if (!(seat & 0xFF)) {
            if (pl->motionId[0] != 0x37A) {
                Actor_SetMotion(pl, 0x37A, -0xA, 0);
                break;
            }
        } else {
            if (pl->motionId[0] != 0x37E) {
                Actor_SetMotion(pl, 0x37E, -6, 0);
            }
            break;
        }
        break;
    case 1:
        timer = pl->timer - 1;
        pl->timer = timer;
        if (timer <= 0) {
            pl->step = (u8) (pl->step + 1);
        }
        break;
    case 2:
        if ((gScene.localPlayer == pl->index) && (gTownMode != TOWN_MODE_CONTEST) && (gPadState[0].pressed & PAD_CROSS)) {
            gTownMode = TOWN_MODE_FREE;
            gFieldControl = 1;
            gTownMenuOpen = 0;
            Actor_ReturnToIdle(pl, 0, 0x14, 0);
            TownMsg_SendContestSeatFree(pl);
            TownMsg_SendStateNow((void *)((s32) pl));
        }
        break;
    }
}

/*
 * Returns 1 while the player is seated or gripping at the arm-wrestling
 * table (states 0x6F to 0x71 of the action group), else 0.
 */
s32 TownPc_IsInContest(Player *pl)
{
    if (pl->stateGroup == TOWN_PC_GROUP_ACTION) {
        switch (pl->state) {
        case TOWN_PC_CONTEST_GRIP:
        case TOWN_PC_CONTEST_SEAT_A:
        case TOWN_PC_CONTEST_SEAT_B:
            return 1;
        }
    }
    return 0;
}

/*
 * The grip: when the seat motion ends, plays the side's grip motion and
 * spawns a spray of particles; the contest's menu then decides the result.
 */
void TownPc_StateContestGrip(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            if (pl->motionId[0] == 0x37A) {
                Actor_SetMotion(pl, 0x37B, 4, 0);
            } else {
                Actor_SetMotion(pl, 0x37F, 4, 0);
            }
            ActorParticles_SpawnSpray(pl);
        }
        
    case 1:
        break;
    }
}

s32 Award_Grant(s32 n);

/*
 * The winner's motion (the arm-wrestling winner; also the drinking
 * contest's opponent when it wins). The local player's contest-win record
 * goes up (capped at 65000) and grants the award at 50 wins. The cheer
 * motion plays four times, then idle and, for the local player, the state
 * is sent. yawCenter serves as the cheer counter here.
 */
void TownPc_StateContestWin(Player *pl)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        Actor_SetPosture(pl, 0, 0, 0);
        if (pl->motionId[0] == 0x37B) {
            pl->yawCenter = 0;
            Actor_SetMotion(pl, 0x37C, 0, 0);
        } else {
            pl->yawCenter = 0;
            Actor_SetMotion(pl, 0x380, 0, 0);
        }
        if (pl->index == gScene.localPlayer && gSave.recordCounts[34] < 65000) {
            gSave.recordCounts[34] = gSave.recordCounts[34] + 1;
            if (gSave.recordCounts[34] >= 0x32) {
                Award_Grant(0x24);
            }
        }
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            if ((pl->yawCenter)++ > 3) {
                pl->step++;
            } else {
                Actor_SetMotion(pl, 0x26E, 0, 0x18);
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Actor_ReturnToIdle(pl, 0, 0xC, 0);
            if (gScene.localPlayer == pl->index) {
                TownMsg_SendStateNow((void *)((s32)pl));
            }
        }
        break;
    }
}

/*
 * The loser's motion: on the first seat it falls back, sits up and gets up
 * face-up; on the second seat only its lose motion plays. Then idle, the
 * local player sends the state, and the state idles in step 4 until the
 * idle takes over.
 */
void TownPc_StateContestLose(Player *pl)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        Actor_SetPosture(pl, 0, 0, 0);
        if (pl->motionId[0] == 0x37B) {
            Actor_SetMotion(pl, 0x37D, 4, 0);
        } else {
            pl->step = 3;
            Actor_SetMotion(pl, 0x381, 4, 0);
        }
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            Actor_SetMotion(pl, PL_MOT_GESTURE_SIT_END, 0, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            Actor_SetMotion(pl, PL_MOT_GET_UP_FACE_UP, 0, 0x3C);
        }
        break;
    case 3:
        if (pl->motion[0].playing <= 0) {
            Actor_ReturnToIdle(pl, 0, 0xC, 0);
            if (gScene.localPlayer == pl->index) {
                TownMsg_SendStateNow((void *)((s32)pl));
            }
            pl->step++;
        }
        break;
    case 4:
        break;
    }
}

u8 Notice_PushItem(char *);
u8 *ActSpot_GetList(u16 area);
u16 PoogieDig_RollItem(u16 id, u8 k);

/*
 * Digging at a Poogie dig spot: crouches (TOWN_MODE_DIG), digs for 90
 * frames; when the area's gathering limit is reached it shows that notice
 * and stands up. Otherwise it finds the spot's entry in the area's action-
 * spot list from the current zone's dig spots, rolls the find for the
 * area's gather kind, and gives it or shows the nothing-found notice. Back
 * to idle and town mode free.
 */
void TownPc_StateDig(Player *pl)
{
    u8 step = pl->step;
    u8 *digSpot;
    u8 gatherKind;
    u16 item;

    switch (step) {
    case 0:
        pl->step = step + 1;
        pl->timer = 0;
        pl->action = PL_POSTURE_CROUCHED;
        gTownMode = TOWN_MODE_DIG;
        Actor_SetMotion(pl, 0x296, 0x28, 0);
        break;
    case 1:
        if ((pl->timer)++ > 0x5A) {
            pl->step++;
            if (gDigOverLimit == 1) {
                Notice_PushItem(TXT(0x0083B880) /* the notice that nothing more is left to dig */);
                Actor_SetMotion(pl, PL_MOT_IDLE, 0x1E, 0);
                pl->step = 3;
            }
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Save_RestorePouch(pl);
            if (gScene.area == AREA_HUB_0x89) {
                digSpot = ActSpot_GetList(pl->area) + 0x108;
                gatherKind = 0;
            } else if (gScene.area == AREA_HUB_0x83) {
                digSpot = ActSpot_GetList(pl->area) + 0x48;
                gatherKind = 1;
            } else {
                Actor_SetMotion(pl, PL_MOT_IDLE, 0x1E, 0);
                pl->step++;
                break;
            }
            if (gPoogieDigSpots == 0) {
                Actor_SetMotion(pl, PL_MOT_IDLE, 0x1E, 0);
                pl->step++;
                break;
            }
            digSpot += ((u8 *)gPoogieDigSpots)[gPoogieDigSpotIdx] * 0x18;
            item = PoogieDig_RollItem(*(u16 *)digSpot, gatherKind);
            if (item <= 0) {
                Notice_PushItem(TXT(0x0083B8A0) /* the notice that nothing is there to dig */);
            } else {
                Pouch_GiveItem(pl, item, pl->gotItem, 1);
            }
            Actor_SetMotion(pl, PL_MOT_IDLE, 0x1E, 0);
            pl->step++;
        }
        break;
    case 3:
        if (pl->motion[0].blend == 0) {
            Actor_ReturnToIdle(pl, 0, 0, 0);
            if (gTownMode != TOWN_MODE_FREE) {
                gTownMode = TOWN_MODE_FREE;
            }
        }
        break;
    }
}

/* the padding at the end of the file's .rodata */
const u8 D_0083B8B8[8] = { 0 };

/*
 * The drinking contest's stumble after a missed key: plays the miss
 * motion, then the seated motion and back to the seated state at the
 * table.
 */
void TownPc_StateDrinkMiss(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x363, 4, 0);
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
 * The drinking contest's loss: the first entry plays the lose reaction;
 * the contest sets the state again after its camera, which plays the
 * second motion and returns to the seated state.
 */
void TownPc_StateDrinkLose(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetPosture(pl, 1, 0, 0);
        if (pl->motionId[0] != 0x265) {
            Actor_SetMotion(pl, 0x265, 0, 0);
            break;
        }
        Actor_SetMotion(pl, 0x266, 6, 0);
        break;
    case 1:
        if ((pl->motionId[0] == 0x266) && (pl->motion[0].playing <= 0)) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

/* The drinking contest's win: as the loss, with the win motions. */
void TownPc_StateDrinkWin(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        if (pl->motionId[0] != 0x26A) {
            Actor_SetMotion(pl, 0x26A, 0, 0);
            break;
        }
        Actor_SetMotion(pl, 0x282, 4, 0);
        break;
    case 1:
        if ((pl->motionId[0] == 0x282) && (pl->motion[0].playing <= 0)) {
            Actor_SetMotion(pl, PL_MOT_TOWN_SEATED, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

f32 Vec_Distance(f32 *a, f32 *b);
void TownPc_StartMoveTo(Player *obj, s16 n);
s32 Actor_HeadingDiff(void *obj, void *pos);

/*
 * The walk to the barber's chair (set by the hair menu with moveTo at the
 * chair): turns toward moveTo and walks with the gender's walk motion;
 * within 50 units it turns to the chair's heading and stops, and 10 frames
 * later is placed on the chair's spot and goes idle.
 */
void TownPc_StateWalkToHairChair(Player *pl)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        pl->fieldCollide = 0;
        pl->rot[1] += (s16)Actor_HeadingDiff(pl, pl->moveTo);
        pl->targetYaw = pl->rot[1];
        if (pl->gender == 1) {
            Actor_SetMotion(pl, 0x40, 4, 0);
        } else {
            Actor_SetMotion(pl, PL_MOT_WALK, 4, 0);
        }
        break;
    case 1:
        if (Vec_Distance(&pl->pos[0], &pl->moveTo[0]) < 50.0f) {
            pl->fieldCollide = 1;
            pl->timer = 10;
            pl->targetYaw = 0x8E39;
            TownPc_StartMoveTo(pl, pl->timer);
            Actor_SetMotion(pl, PL_MOT_IDLE, 4, -4);
            pl->step++;
        }
        break;
    case 2:
        if (--pl->timer <= 0) {
            pl->pos[0] = 2480.0f;
            pl->pos[1] = 0.0f;
            pl->pos[2] = 2876.0f;
            Actor_SetState(pl, TOWN_PC_GROUP_ACTION, TOWN_PC_IDLE);
        }
        break;
    }
}

/*
 * The arm-wrestling draw: plays the second seat's motion from frame 160 to
 * frame 216, then the gesture motion; 5 frames after it ends the local
 * player goes idle and sends the state.
 */
void TownPc_StateContestDraw(Player *pl)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x37E, 4, 0xA0);
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 216.0f) != 0) {
            pl->step = (u8) (pl->step + 1);
            pl->timer = 5;
            Actor_SetMotion(pl, 0x273, 0x14, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            timer = pl->timer - 1;
            pl->timer = timer;
            if ((timer <= 0) && (pl->index == gScene.localPlayer)) {
                Actor_ReturnToIdle(pl, 0, 0xC, 0);
                TownMsg_SendStateNow((void *)((s32) pl));
            }
        }
        break;
    }
}

/*
 * Runs the player's state in the action group: one TownPc_State function
 * per state, some with a variant argument. Then, for a player other than
 * the local one, steers it toward the position the network gave it.
 */
void TownPc_RunActionState(Player *pl)
{
    u8 state;

    state = pl->state;
    switch (state) {
    case TOWN_PC_IDLE:
        TownPc_StateIdle(pl);
        break;
    case TOWN_PC_WALK:
        TownPc_StateWalk(pl, 1);
        break;
    case TOWN_PC_RUN:
        TownPc_StateRun(pl);
        break;
    case TOWN_PC_RUN_LOOP:
        TownPc_StateWalk(pl, 6);
        break;
    case TOWN_PC_SEAT_POSE_DOWN_END:
        TownPc_StateSeatPoseDownEnd(pl);
        break;
    case TOWN_PC_SEAT_POSE_UP_END:
        TownPc_StateSeatPoseUpEnd(pl);
        break;
    case TOWN_PC_SEAT_POSE_RIGHT_END:
        TownPc_StateSeatPoseRightEnd(pl);
        break;
    case TOWN_PC_RUN_STOP:
        TownPc_StateRunStop(pl);
        break;
    case TOWN_PC_OFFER_ITEM:
        TownPc_StateOfferItem(pl);
        break;
    case TOWN_PC_TAKE_ITEM_WAIT:
        TownPc_StateTakeItemWait(pl);
        break;
    case TOWN_PC_OFFER_ITEM_END:
        TownPc_StateOfferItemEnd(pl);
        break;
    case TOWN_PC_TAKE_ITEM:
        TownPc_StateTakeItem(pl);
        break;
    case TOWN_PC_BED_LIE_DOWN:
        TownPc_StateBedLieDown(pl);
        break;
    case TOWN_PC_BED_SLEEP:
        TownPc_StateBedSleep(pl, 0);
        break;
    case TOWN_PC_BED_GET_UP:
        TownPc_StateBedGetUp(pl, 0);
        break;
    case TOWN_PC_TURN_AROUND:
        TownPc_StateTurnAround(pl);
        break;
    case TOWN_PC_DRUNK_COLLAPSE:
        TownPc_StateDrunkCollapse(pl);
        break;
    case TOWN_PC_DRUNK_GET_UP:
        TownPc_StateDrunkGetUp(pl);
        break;
    case TOWN_PC_SIT_DOWN:
        TownPc_StateSitDown(pl, 0);
        break;
    case TOWN_PC_STAND_UP:
        TownPc_StateStandUp(pl);
        break;
    case TOWN_PC_SEAT_POSE_LEFT_END:
        TownPc_StateSeatPoseLeftEnd(pl);
        break;
    case TOWN_PC_BED_NAP:
        TownPc_StateBedNap(pl);
        break;
    case TOWN_PC_REST:
        TownPc_StateRest(pl, 1);
        break;
    case TOWN_PC_REST_LONG:
        TownPc_StateRest(pl, 0);
        break;
    case TOWN_PC_GET_UP_FACE_UP:
        TownPc_StateGetUpFaceUp(pl);
        break;
    case TOWN_PC_SEAT_POSE_RIGHT:
        TownPc_StateSeatPoseRight(pl);
        break;
    case TOWN_PC_DRUNK_IDLE:
        TownPc_StateDrunkIdle(pl);
        break;
    case TOWN_PC_KITCHEN_ORDER:
        TownPc_StateKitchenOrder(pl);
        break;
    case TOWN_PC_THROWN:
        TownPc_StateThrown(pl);
        break;
    case TOWN_PC_GRABBED:
        TownPc_StateGrabbed(pl);
        break;
    case TOWN_PC_SEAT_DRINK:
        TownPc_StateSeatDrink(pl);
        break;
    case TOWN_PC_SEAT_CROSS:
        TownPc_StateSeatCross(pl);
        break;
    case TOWN_PC_DRUNK_WALK:
        TownPc_StateWalk(pl, 9);
        break;
    case TOWN_PC_SEAT_POSE_LEFT:
        TownPc_StateSeatPoseLeft(pl);
        break;
    case TOWN_PC_SIT_DOWN_SEATED:
        TownPc_StateSitDown(pl, 1);
        break;
    case TOWN_PC_SEAT_POSE_DOWN:
        TownPc_StateSeatPoseDown(pl);
        break;
    case TOWN_PC_MEAL_RISE:
        TownPc_StateMealRise(pl, 0);
        break;
    case TOWN_PC_MEAL_RISE_GOOD:
        TownPc_StateMealRise(pl, 1);
        break;
    case TOWN_PC_SEAT_POSE_UP:
        TownPc_StateSeatPoseUp(pl);
        break;
    case TOWN_PC_MEAL_SERVED:
        TownPc_StateMealServed(pl);
        break;
    case TOWN_PC_MEAL_COLLAPSE:
        TownPc_StateMealCollapse(pl);
        break;
    case 0x2B:
        TownPc_State2B(pl);
        break;
    case 0x61:
        TownPc_StateFishWait(pl, 0);
        break;
    case 0x62:
        TownPc_StateFishCast(pl, 0);
        break;
    case PL_TOWN_FISH_REEL:
        TownPc_StateFishReel(pl, 0);
        break;
    case PL_TOWN_FISH_REEL_EMPTY:
        TownPc_StateFishReelEmpty(pl, 0);
        break;
    case PL_TOWN_FISH_REEL_BAITED:
        TownPc_StateFishReelEmpty(pl, 1);
        break;
    case TOWN_PC_CONTEST_GRIP:
        TownPc_StateContestGrip(pl);
        break;
    case TOWN_PC_CONTEST_SEAT_A:
        TownPc_StateContestSeat(pl, 0);
        break;
    case TOWN_PC_CONTEST_SEAT_B:
        TownPc_StateContestSeat(pl, 1);
        break;
    case TOWN_PC_CONTEST_LOSE:
        TownPc_StateContestLose(pl);
        break;
    case TOWN_PC_CONTEST_WIN:
        TownPc_StateContestWin(pl);
        break;
    case TOWN_PC_GALLERY_AIM:
        TownPc_StateGalleryAim(pl);
        break;
    case TOWN_PC_POOGIE_DIG:
        TownPc_StateDig(pl);
        break;
    case TOWN_PC_DRINK_MISS:
        TownPc_StateDrinkMiss(pl);
        break;
    case TOWN_PC_DRINK_LOSE:
        TownPc_StateDrinkLose(pl);
        break;
    case TOWN_PC_DRINK_WIN:
        TownPc_StateDrinkWin(pl);
        break;
    case TOWN_PC_HAIR_CHAIR:
        TownPc_StateHairChair(pl);
        break;
    case TOWN_PC_WALK_TO_HAIR_CHAIR:
        TownPc_StateWalkToHairChair(pl);
        break;
    case TOWN_PC_CONTEST_DRAW:
        TownPc_StateContestDraw(pl);
        break;
    }
    if (pl->index != gScene.localPlayer) {
        TownPc_SteerToDest(pl);
    }
}
