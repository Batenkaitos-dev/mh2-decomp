/*
 * The townspeople's state handlers: what each townsperson of the town and
 * the village does in each actor state (Em.state, set by Actor_SetState).
 * The townspeople are Em slots whose work block is NpcWork (Em.work), and
 * Npc_UpdateState picks the handler set by the townsperson's variant
 * (Em.variant): Npc_RunState for most of them (its states are
 * enum NpcState's NPC_STATE_*), ServiceNpc_RunState for the
 * house kitchen's staff (variants 0x51-0x55), Pet_RunState for the pets
 * (variants 0x4F and 0x50: the Poogie, the care pet, the house pet).
 *
 * A townsperson walks a route (NpcWork.route, NpcRouteEnt): at each point
 * it enters the point's state, whose handler plays its motions and moves
 * on to the next point when it is done (Npc_NextRoutePoint). The file holds,
 * in order:
 * - the shared steps: the next route point, the turns toward the player and
 *   back, and the swaps of a pair of townspeople by the side of a line the
 *   player is on (Npc_PairIdBySide, Npc_CheckSideSwap);
 * - the idles by kind (Npc_Idle*) and the common states: standing, walking,
 *   running, the talk (Npc_StateTalk) and the state each kind takes when the
 *   player talks to it (Npc_StartTalkState), the seats and poses, the pairs
 *   that stand together, the townspeople who greet a passing player;
 * - the smithy: the smith's work, forging and talk, with its helper and its
 *   partner, and its moves between the smithy's two levels;
 * - the other townspeople's states and Npc_RunState, their dispatcher;
 * - the service townspeople's states (ServiceNpc_*: the cook, the
 *   hairdresser, the bait seller) and their routes;
 * - the pets' states (Pet_*): following the player, the petting, running
 *   off and digging, with their areas and routes (gNpc4FArea*,
 *   gNpc4FRoute*), and a few lookups (Pet_FindNpc, ActSpot_FindFirstOfKind).
 *
 * The townspeople's set-up, update and drawing (Npc_Init, Npc_UpdateState)
 * and the kind tests (Npc_IsMusicActor, Npc_IsFree ...) come before this file;
 * the tutorial's lessons (NpcScene_RunNpc) follow it. The last data,
 * gDecoFrameColors (the menus' decorated frame colours, read by
 * Menu_DrawDecoFrame and the quest desk's drawing, by nothing here), sits
 * at the end of this file's data in the game, so it is defined here.
 */
#include "common.h"
#include "lobby_npc.h"
#include "em.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "lobby_state.h"
#include "area.h"
#include "lobby_menu.h"
#include "sound.h"
#include "quest.h"


/* Prototypes shared by the parts of this unit. */
void Actor_SetPosture(void *a, u16 mode, s32 b, s32 c);
/* unit's own declaration: a u16 state (the definition takes s32): the callers here pass the state as a halfword, and Npc_NextRoutePoint, ServiceNpc_StateHairDo and ServiceNpc_StateTalk compile as the game's code only with this type */
void Actor_SetState(void *obj, s32 group, u16 state);
void Npc_StartMoveTo(Em *obj, s32 n);
s32 Cond_Check(u16 id);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
void Npc_EnterRoutePoint(Em *npc);
Em *Npc_Find(u8 kind);
s32 Npc_NextRoutePoint(Em *a);
void Npc_StateIdle(Em *a);
void Npc_StateWalk(Em *a);
void Npc_StateRun(Em *a);
void Npc_StateSitDown(Em *a, u8 flag);
void Npc_StateTalk(Em *npc);
void Npc_StateBusyIdle(Em *a);
void Npc_StateBusyTalk(Em *a);
void Npc_StateBusyPoseTalk(Em *a);
void Npc_StateBusyLoopTalk(Em *a);
void Npc_StateGreet(Em *a);
void Npc_StateGreetHold(Em *a);
void Npc_State3F(Em *a);
void Npc_StateSeatedAtTable(Em *a);
void Npc_StatePairStand(Em *a, s8 arg1);
void Npc_StateWarpToRoute(Em *arg0);
void Npc_StateStepAhead(Em *a);
void Npc_StateStepBack(Em *a);
void Npc_StateSmithTalk(Em *a, u8 flag);
void Npc_StateSmithMeetBelow(Em *a);
void Npc_StateSmithMeetAbove(Em *a);
void Npc_StatePartnerWork(Em *a);
void Npc_SmithWorkWithPartner(Em *a);
void Npc_SmithWorkAlone(Em *a);
void Npc_State9D(Em *a);
void Npc_StateContestWait(Em *a);
void Npc_StateContestReact(Em *a, s32 first);
u16 Rand_Next(s32 stream);
u8 Village_FacilityLevel(s32 k);
int abs(int);

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Npc_TurnToPlayer(Em *, s32);
extern s32 gTownMode;
s32 Npc_CheckHubPartner(Em *);
void Npc_StartTalkState(Em *a);

s32 Game_IsOnline(void);
s32 Npc_HasSecondHead(Em *arg0);
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
s32 Actor_HeadingDiff(void *obj, void *pos);
s32 abs(s32);
s32 Npc_IsFree(Em *arg0);
f32 Vec_Distance(f32 *a, f32 *b);


/* Advance to the next point of the route (back to the first after the end,
 * returning 0) and enter its state. */
s32 Npc_NextRoutePoint(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    NpcRouteEnt *point;
    s32 ret;

    npcWork->routeIdx++;
    point = &npcWork->route[npcWork->routeIdx];
    ret = 1;
    if (point->state == 0) {
        npcWork->routeIdx = 0;
        ret = 0;
        point = &npcWork->route[npcWork->routeIdx];
    }
    if (point->state != npc->state) {
        Actor_SetState(npc, 0, point->state);
        if (point->arg == -1 || point->state == 1) {
            *(NpcPos *)npc->goal = point->pos;
            Npc_StartMoveTo(npc, 0x14);
        }
    }
    npcWork->wait = 0;
    return ret;
}

/* Steps a turn started by Npc_TurnToPlayer, Npc_StartTurnTo or
 * Npc_StartTurnBack: turnStep a frame while turnCount lasts.
 * Npc_UpdateState calls it every frame. */
void Npc_StepTurn(Em *npc)
{
    u16 count;
    NpcWork *npcWork;

    npcWork = (NpcWork *)&npc->work;
    count = npcWork->turnCount;
    if (count != 0) {
        npcWork->turnCount = (s16) (count - 1);
        npc->rotY = (s32) (npc->rotY + npcWork->turnStep);
    }
}

/* Starts turning the townsperson toward the local player over the given
 * frames. A turn of more than 10 (0x10000 a full turn) first keeps the
 * current heading in homeYaw, for Npc_StartTurnBack. */
void Npc_TurnToPlayer(Em *npc, s32 frames)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    npcWork->turnDelta = Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos);
    if (abs(npcWork->turnDelta) > 10) {
        npcWork->homeYaw = npc->rotY;
    }
    npcWork->turnStep = npcWork->turnDelta / (s16)frames;
    npcWork->turnCount = frames;
}

/* Starts turning the townsperson to the heading angle (0x10000 a full
 * turn) over the given frames. */
void Npc_StartTurnTo(Em *npc, s32 frames, s32 angle)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    npcWork->turnDelta = (s16)angle - (s16)npc->rotY;
    npcWork->turnStep = npcWork->turnDelta / (s16)frames;
    npcWork->turnCount = frames;
}

/* Starts turning the townsperson back to the heading kept in homeYaw over
 * the given frames. It is static: Npc_StateContestWait, one of its callers,
 * compiles as the game's code only with a static callee, and that ties
 * this file's functions up to Npc_StateWatchContest into one unit. */
static void Npc_StartTurnBack(Em *npc, s32 frames)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    npcWork->turnDelta = (u16)npcWork->homeYaw - npc->rotY;
    npcWork->turnStep = npcWork->turnDelta / (s16)frames;
    npcWork->turnCount = frames;
}

/* For townsperson 2 in AREA_HUB_0x89: 1 when townsperson 1 is within 200
 * and standing with it (state 0x43 or 0x44, Npc_StatePairStand), 2 when it is
 * within 200 doing anything else (a talk), else 0; 0 for any other
 * townsperson or area. */
s32 Npc_CheckHubPartner(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Em *partner;

    if (gScene.area != AREA_HUB_0x89) {
        return 0;
    }
    switch (npcWork->id) {
    case 2:
        break;
    default:
        return 0;
    }
    partner = Npc_Find(1);
    if (partner == NULL) {
        return 0;
    }
    if (200.0f > Vec_Distance(&npc->posX, (f32 *) &partner->posX)) {
        if ((partner->state == NPC_STATE_PAIR_STAND_1 || partner->state == NPC_STATE_PAIR_STAND) && partner->stateGroup == 0) {
            return 1;
        }
        return 2;
    }
    return 0;
}

extern NpcSideLine gNpcSideLines[];

/* Whether the local player is on the upper side of line i of
 * gNpcSideLines (its z above the line's z at the player's x): 1 or 0.
 * Lines past 3 count as line 0. It is static: so the caller keeps its
 * pair pointer in the same register across the call, as the game's code
 * does. */
static s32 Npc_PlayerAboveLine(u16 lineIdx)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcSideLine *line;
    f32 slope;

    if (lineIdx > 3) {
        lineIdx = 0;
    }
    line = &gNpcSideLines[lineIdx];
    slope = (line->z2 - line->z1) / (line->x2 - line->x1);
    if (pl->pos[2] - slope * pl->pos[0] >= line->z1 - line->x1 * slope) {
        return 1;
    }
    return 0;
}

extern NpcSidePair gNpcSidePairs[];

/* The route kind to use instead of kind: a pair's other kind, by the side
 * of its line the player is on (kinds 8 and 0xE need facility 2). */
u8 Npc_PairIdBySide(u8 kind)
{
    s32 kindValue = kind;
    NpcSidePair *pair;
    u16 line;

    switch (kindValue) {
    case NPC_WORKSHOP_SMITH:
    case 8:
        if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) == 0) {
            return kind;
        }
    }
    pair = gNpcSidePairs;
    for (line = gNpcSidePairs[0].line; line != 0xFFFF; line = pair->line) {
        if (kindValue == pair->idA || kindValue == pair->idB) {
            break;
        }
        pair++;
    }
    if (line == 0xFFFF) {
        return kind;
    }
    if (Npc_PlayerAboveLine(line) == 1) {
        if (kindValue == pair->idB) {
            return pair->idA;
        }
    } else if (kindValue == pair->idA) {
        return pair->idB;
    }
    return kind;
}

s32 Area_IsHouse(u16 area);

/* Swaps a townsperson of a side pair (gNpcSidePairs) for its partner:
 * when its presence rule (unk48) is 4 and the side of the line the local
 * player is on calls for the partner (Npc_PairIdBySide), it leaves
 * (state 0x60, unk49 set) and partnerId keeps the partner's id. Skipped
 * offline in the house, during a talk or a leave, and for the player's
 * lock-on target. */
void Npc_CheckSideSwap(Em *npc, NpcWork *work)
{
    u8 partnerId;

    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1) {
        return;
    }
    switch (npc->state) {
    case 0x2C:
    case 0x42:
    case 0x5F:
    case 0x60:
        return;
    }
    if (work->leaving == 0 && npc != gPlayers[gScene.localPlayer].target) {
        switch (work->presence) {
        case NPC_PRESENCE_SIDE:
            partnerId = Npc_PairIdBySide(work->id);
            if (work->id != partnerId) {
                npc->state = 0x60;
                work->leaving = 1;
                work->partnerId = partnerId;
            }
            return;
        }
    }
}

s32 LobbyTime_IsAm(void);
void Npc_SetRouteByKind(Em *npc);

/* Re-picks the townsperson's route for the time of day
 * (Npc_SetRouteByKind), except offline in the house (where only
 * townsperson 0x67 goes on), during a talk or a leave, once it is leaving
 * (unk49), and while the player talks to it. The checks of the presence
 * rule (unk48) after the call change nothing. */
void Npc_CheckTimeRoute(Em *npc, NpcWork *work)
{
    if (Game_IsOnline() == 0 && work->id != 0x67 && Area_IsHouse(gScene.area) == 1) {
        return;
    }
    switch (npc->state) {
    case 0x2C:
    case 0x42:
    case 0x5F:
    case 0x60:
        return;
    }
    if (work->leaving != 0) {
        return;
    }
    if (gTownMode == TOWN_MODE_TALK && npc == gPlayers[gScene.localPlayer].target) {
        return;
    }
    Npc_SetRouteByKind(npc);
    switch (work->presence) {
    case NPC_PRESENCE_ALWAYS:
        return;
    case NPC_PRESENCE_AM:
        if (LobbyTime_IsAm() == 0) {
            return;
        }
        return;
    case NPC_PRESENCE_PM:
        if (LobbyTime_IsAm() == 1) {
            if (work->id == 9 && (u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 0) {
                return;
            }
            switch (work->id) {
            case 6:
            case 9:
                return;
            }
            return;
        }
        return;
    }
}

s32 Motion_ReachedFrame(void *, s32, f32);

/* The workshop helper's idle (NPC_WORKSHOP_HELPER): it strikes along with
 * the smith (NPC_WORKSHOP_SMITH). When the smith's forging motion (0x40B)
 * passes frame 44 it plays its own strike (0x40B, 0x40C twice, 0x41A,
 * 0x40D; motionPhase marks frames 30 to 118 of it), then stands (0x406)
 * and waits for the next one. Without the smith it stops after its
 * first step. */
void Npc_IdleWorkshopHelper(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Em *smith = Npc_Find(NPC_WORKSHOP_SMITH);
    u8 step;

    if (smith == NULL && npc->step != 0) {
        return;
    }
    step = npc->step;
    switch (step) {
    case 0:
        npc->step = step + 1;
        npcWork->motionPhase = 0;
        if (npc->motionId[0] == 0x406) {
            Actor_SetMotionOnLayer(npc, 0x406, 0x14, 0, 0);
        }
        break;
    case 1:
        if (smith->motionId[0] == 0x40B && Motion_ReachedFrame(smith, 0, 44.0f) != 0) {
            Actor_SetMotionOnLayer(npc, 0x40B, 6, 0, 0);
            npc->step++;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step = step + 1;
            npc->timer = 2;
            Actor_SetMotionOnLayer(npc, 0x40C, 4, 0, 0);
        }
        break;
    case 3:
        if (npcWork->motionPhase == 0 && Motion_ReachedFrame(npc, 0, 30.0f) != 0) {
            npcWork->motionPhase = 1;
        }
        if (npc->motion[0].playing <= 0 && --npc->timer < 2) {
            npc->step++;
        }
        break;
    case 4:
        if (Motion_ReachedFrame(npc, 0, 136.0f) != 0) {
            Actor_SetMotionOnLayer(npc, 0x41A, 0, 0, 0);
            npc->step++;
        }
        break;
    case 5:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, 0x40D, 0, 0x14, 0);
            npc->step++;
        }
        break;
    case 6:
        if (npcWork->motionPhase == 1 && Motion_ReachedFrame(npc, 0, 118.0f) != 0) {
            npcWork->motionPhase = 0;
        }
        if (npc->motion[0].playing <= 0) {
            npc->step = 1;
            npcWork->motionPhase = 0;
            Actor_SetMotionOnLayer(npc, 0x406, 0x10, 0, 0);
        }
        break;
    }
}

/* Whether townsperson 6 is walking by: present, the port
 * (VILLAGE_FACILITY_PORT) not at level 2, and neither running (state 4)
 * nor standing (motion PL_MOT_IDLE or 0x292). The contest host's idle
 * calls to it then (Npc_IdleContestHost). Returns 1 or 0. */
s32 Npc_IsHostCalleeWalking(void)
{
    Em *em = Npc_Find(6);

    if (em == NULL) {
        return 0;
    }
    if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2 || em->state == 4 || em->motionId[0] == 0x292 || em->motionId[0] == PL_MOT_IDLE) {
        return 0;
    }
    return 1;
}

/* The contest host's idle (NPC_CONTEST_HOST): it alternates its standing
 * motions 0x371 and 0x370, and while townsperson 6 walks by
 * (Npc_IsHostCalleeWalking) calls to it (0x372) half the time. After a motion ends it
 * picks the next one with a 30% chance a frame. */
void Npc_IdleContestHost(Em *npc)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        if (Npc_IsHostCalleeWalking() == 1 && ((u16)Rand_Next(1) & 1) && npc->motionId[0] != 0x372) {
            Actor_SetMotion(npc, 0x372, 0, 0);
            break;
        }
        if (npc->motionId[0] == 0x371) {
            Actor_SetMotion(npc, 0x370, 0, 0);
            break;
        }
        Actor_SetMotion(npc, 0x371, 0, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && (u16)Rand_Next(1) % 100 <= 29) {
            npc->step = 0;
        }
        break;
    }
}

/* The pair 0x69/0x6B: each stays near the other (within 400) and plays
 * 0x3EF/0x3F0 in turn; farther away it plays 0x3E9. */
void Npc_IdlePaired(Em *npc)
{
    u8 step = npc->step;
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Em *partner;

    switch (step) {
    case 0:
        npc->step = step + 1;
        break;
    case 1:
        switch (npcWork->id) {
        case 0x69:
            partner = Npc_Find(0x6B);
            goto near;
        case 0x6B:
            partner = Npc_Find(0x69);
        near:
            if (partner != NULL) {
                if (Vec_Distance(&npc->posX, (f32 *) &partner->posX) <= 400.0f) {
                    if (partner->state == NPC_STATE_TALK_DELAY || partner->state == NPC_STATE_TALK) {
                        Npc_StartTalkState(npc);
                        return;
                    }
                    switch (npc->motionId[0]) {
                    case 0x3EF:
                        if (npc->motion[0].playing == 0) {
                            npc->timer = (u16)Rand_Next(1) % 60 + 90;
                            Actor_SetMotionOnLayer(npc, 0x3F0, 4, 0, 0);
                        }
                        break;
                    case 0x3F0:
                        if (--npc->timer <= 0) {
                            Actor_SetMotionOnLayer(npc, 0x3EF, 4, 0, 0);
                        }
                        break;
                    default:
                        npc->timer = (u16)Rand_Next(1) % 60 + 90;
                        Actor_SetMotionOnLayer(npc, 0x3F0, 4, 0, 0);
                        break;
                    }
                } else if (npc->motionId[0] != 0x3E9) {
                    Actor_SetMotionOnLayer(npc, 0x3E9, 4, 0, 0);
                }
            } else if (npc->motionId[0] != 0x3E9) {
                Actor_SetMotionOnLayer(npc, 0x3E9, 4, 0, 0);
            }
            break;
        }
        break;
    }
}

/* The idle of kind 0x4D (Npc_IsType4D): stands (0x3E9) for 4 or 5 loops,
 * then plays 0x3EA once. It keeps facing during a talk (keepFacing). */
void Npc_Idle4D(Em *npc)
{
    ((NpcWork *)&npc->work)->keepFacing = 1;
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 0x3E9) {
            Actor_SetMotionOnLayer(npc, 0x3E9, 4, 0, 0);
            npc->timer = (u16)Rand_Next(1) % 2 + 4;
        }
        npc->step++;
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            if (--npc->timer <= 0) {
                Actor_SetMotionOnLayer(npc, 0x3EA, 4, 0, 0);
                npc->step++;
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step = 0;
        }
        break;
    }
}

/* The idle of kinds 0x3C-0x3E (Npc_IsType3Cto3E): 0x2B2 or 0x2B6, picked
 * by chance as each loop ends. */
void Npc_Idle3C(Em *npc)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        Actor_SetMotion(npc, 0x2B2, 4, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            if ((u16)Rand_Next(1) & 1) {
                Actor_SetMotion(npc, 0x2B2, 4, 0);
            } else {
                Actor_SetMotion(npc, 0x2B6, 8, 0);
            }
        }
        break;
    }
}

/* The idle of kind 0x4B (Npc_IsType4B): 0xFD or 0xFE, picked by chance as
 * each loop ends (0xFD, blended over 20 frames, after 0x292). */
void Npc_Idle4B(Em *npc)
{
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] == 0x292) {
            Actor_SetMotion(npc, 0xFD, 0x14, 0);
        } else if ((u16)Rand_Next(1) & 1) {
            if (npc->motionId[0] != 0xFD) {
                Actor_SetMotion(npc, 0xFD, 4, 0);
            }
        } else if (npc->motionId[0] != 0xFE) {
            Actor_SetMotion(npc, 0xFE, 4, 0);
        }
        npc->step++;
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = 0;
        }
        break;
    }
}

/* The idle of kind 0x4E, the townsperson the lobby music's scripts move
 * (LobbyBgm_SetActor; Npc_UpdateState skips it while that music plays):
 * between tunes it plays its motions 0x3EA to 0x3F4 in turn, then starts
 * again. */
void Npc_IdleMusicActor(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3EA, 0, 0);
            npc->step = (u8) (npc->step + 1);
            break;
        }
    default:
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3EB, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3EC, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3ED, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 5:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3EE, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 6:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3EF, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 7:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3F1, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 8:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3F2, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 9:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x3F4, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 10:
        if (npc->motion[0].playing <= 0) {
            npc->step = 0U;
        }
        break;
    }
}

s32 Npc_IsType4B(Em *arg0);
s32 Npc_IsType4D(Em *arg0);
s32 Npc_IsType3Cto3E(Em *arg0);
s32 Npc_IsMusicActor(Em *arg0);
s32 Npc_IsType31to36(Em *arg0);
s32 Npc_IsType40(Em *arg0);
s32 Npc_IsFreeAct29or2A(Em *arg0);
s32 Npc_IsFreeId0Aor2E(Em *arg0);
s32 Npc_IsFreeAct15to19(Em *arg0);
s32 Npc_IsFreeAct18Set(Em *arg0);
void Npc_IdleContestHost(Em *a);
void Npc_IdleWorkshopHelper(Em *a);
void Npc_Idle4B(Em *arg0);
void Npc_Idle4D(Em *a);
void Npc_Idle3C(Em *arg0);
void Npc_IdleMusicActor(Em *arg0);
void Npc_IdlePaired(Em *a);

/* State 0x01, standing: the kind's own idle (Npc_Idle*), or the standing
 * motion by kind (PL_MOT_IDLE for most, PL_MOT_IDLE_ALT, the kinds' own),
 * with negative blend frames while a move is still under way (moveCount).
 * The greeters (Npc_IsFreeId0Aor2E) start their greeting (state 0x3C)
 * when the local player comes within 400. */
void Npc_StateIdle(Em *npc)
{
    NpcWork *npcWork;
    s32 blendSign;
    Player *pl;

    pl = &gPlayers[gScene.localPlayer];
    npcWork = (NpcWork *)&npc->work;
    blendSign = (npcWork->moveCount != 0) ? -1 : 1;

    if (npcWork->id == NPC_CONTEST_HOST) {
        Npc_IdleContestHost(npc);
    } else if (npcWork->id == NPC_WORKSHOP_HELPER) {
        Npc_IdleWorkshopHelper(npc);
    } else if (Npc_IsType4B(npc) == 1) {
        Npc_Idle4B(npc);
    } else if (Npc_IsType4D(npc) == 1) {
        Npc_Idle4D(npc);
    } else if (Npc_IsType3Cto3E(npc) == 1) {
        Npc_Idle3C(npc);
    } else if (Npc_IsMusicActor(npc) == 1) {
        Npc_IdleMusicActor(npc);
    } else if (npcWork->id == 0x69) {
        Npc_IdlePaired(npc);
    } else if (npcWork->id == 0x6B) {
        Npc_IdlePaired(npc);
    } else {
        if (npcWork->id == NPC_WORKSHOP_HELPER_ONLINE) {
            npcWork->motionPhase = 1;
        }
        switch (npc->step) {
        case 0:
            npc->step++;
            Actor_SetPosture(npc, PL_POSTURE_STANDING, 0, 0);
            if (Npc_IsType31to36(npc) == 1) {
                Actor_SetMotion(npc, 0x35A, 6, 0);
            } else if (Npc_IsType40(npc) == 1) {
                Actor_SetMotion(npc, 0x364, 4, 0);
                if (npcWork->id == 0x2F || npcWork->id == 0x20) {
                    npcWork->keepFacing = 1;
                }
            } else if (Npc_IsFreeAct29or2A(npc) == 1) {
                if (Npc_IsFreeId0Aor2E(npc) == 1) {
                    Actor_SetMotion(npc, 0x2AB, blendSign * 14, 0);
                } else {
                    Actor_SetMotion(npc, 0x2A8, blendSign * 4, 0);
                }
            } else if (Npc_IsFreeAct15to19(npc) == 1) {
                Actor_SetMotion(npc, 0x2A3, blendSign * 16, 0);
            } else if (Npc_IsFreeAct18Set(npc) == 1) {
                Actor_SetMotion(npc, PL_MOT_IDLE_ALT, blendSign * 16, 0);
            } else if (Npc_HasSecondHead(npc)) {
                Actor_SetMotionOnLayer(npc, 0x3E9, blendSign * 16, 0, 0);
            } else {
                Actor_SetMotion(npc, PL_MOT_IDLE, blendSign * 16, 0);
            }
            break;
        case 1:
        case 2:
            if (Npc_IsFreeAct18Set(npc) == 1 && Npc_IsFreeAct15to19(npc) == 0 && Npc_IsType31to36(npc) == 0) {
                if (npc->motionId[0] == PL_MOT_IDLE && npc->motion[0].playing <= 0) {
                    Actor_SetMotion(npc, PL_MOT_IDLE_ALT, blendSign * 2, 0);
                }
            } else if (Npc_IsFreeId0Aor2E(npc) == 1) {
                if (Vec_Distance(&npc->posX, (f32 *) pl->pos) < 400.0f) {
                    Actor_SetState(npc, 0, NPC_STATE_GREET);
                }
            } else {
                Npc_IsType40(npc);
            }
            break;
        }
    }
}

s32 Npc_IsFreeAct2Bto30(Em *arg0);
s32 Npc_IsWorkshopSmith(Em *arg0);
s32 Npc_IsFreeAct29or2A(Em *arg0);
s32 Npc_IsType40(Em *arg0);
s32 Npc_IsType3Cto3E(Em *arg0);

/* Puts a townsperson the player talks to into its talk state: the
 * smith's (0x68), the greeters' (0x3E), kinds 0x40 when running (0x8E),
 * kinds 0x2B-0x30 (0x2F), kinds 0x3C-0x3E (0x40); else by its current
 * state: the work states' own talks (0x30, 0x31, 0xA2 to 0x31; 0x0E, 0x32
 * to 0x32; 0x33, 0x34 to 0x34; 0x92 to 0x93), or the common talk, state
 * 0x2C (from 0x8F only once its motion 0x365 is over). TownPc_HandleInput,
 * the bait menu and the paired townspeople's states call it. */
void Npc_StartTalkState(Em *npc)
{
    if (Npc_IsWorkshopSmith(npc) == 1) {
        Actor_SetState(npc, 0, NPC_STATE_SMITH_TALK);
        return;
    }
    if (Npc_IsFreeAct29or2A(npc) == 1) {
        Actor_SetState(npc, 0, NPC_STATE_GREET_TALK);
        return;
    }
    if (Npc_IsType40(npc) == 1 && npc->state == 4) {
        Actor_SetState(npc, 0, NPC_STATE_CALL_OVER);
        return;
    }
    if (Npc_IsFreeAct2Bto30(npc) == 1) {
        Actor_SetState(npc, 0, 0x2F);
        return;
    }
    if (Npc_IsType3Cto3E(npc) == 1) {
        Actor_SetState(npc, 0, 0x40);
        return;
    }
    switch (npc->state) {
    case 0x8F:
        if (npc->motionId[0] != 0x365) {
            Actor_SetState(npc, 0, NPC_STATE_TALK);
        }
        break;
    case 0x92:
        Actor_SetState(npc, 0, NPC_STATE_WAIT_TOWN_FREE);
        break;
    case 0x30:
    case 0x31:
    case 0xA2:
        Actor_SetState(npc, 0, NPC_STATE_BUSY_TALK);
        break;
    case 0xE:
    case 0x32:
        Actor_SetState(npc, 0, NPC_STATE_BUSY_POSE_TALK);
        break;
    case 0x33:
    case 0x34:
        Actor_SetState(npc, 0, NPC_STATE_BUSY_LOOP_TALK);
        break;
    default:
        Actor_SetState(npc, 0, NPC_STATE_TALK);
        break;
    }
}

s32 Npc_IsType49(Em *arg0);

/* State 0x03, walking to the route point: the walking motion by kind
 * (PL_MOT_WALK for most), a tenth of the heading difference turned a
 * frame, the next route point once within 30. Townsperson 6 stops to
 * answer the contest host's call (its motion 0x372 between frames 26 and
 * 60, within 800: state 0xA3). */
void Npc_StateWalk(Em *npc)
{
    Em *host;
    NpcWork *npcWork = (NpcWork *)&npc->work;
    NpcPos point;
    f32 dist;

    switch (npc->step) {
    case 0:
        npcWork->keepFacing = 0;
        Actor_SetPosture(npc, PL_POSTURE_STANDING, 0, 0);
        npc->step++;
        if (Npc_IsType49(npc)) {
            if (npc->motionId[0] != 0x41F) {
                Actor_SetMotionOnLayer(npc, 0x41F, 4, 0, 0);
            }
        } else if (npc->variant == 0x3B) {
            if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2) {
                if (npc->motionId[0] != PL_MOT_WALK) {
                    Actor_SetMotion(npc, PL_MOT_WALK, 4, 0);
                }
            } else if (npc->motionId[0] == 0x383) {
                Actor_SetMotion(npc, 0x382, 0x14, 0);
            } else if (npc->motionId[0] != 0x382) {
                Actor_SetMotion(npc, 0x382, 4, 0);
            }
        } else if (Npc_IsType31to36(npc)) {
            if (npc->motionId[0] == 0x35B) {
                Actor_SetMotion(npc, 0x361, 6, 0);
            } else if (npc->motionId[0] != 0x361) {
                Actor_SetMotion(npc, 0x361, 4, 0);
            }
        } else if (Npc_IsType40(npc) == 1) {
            if (npc->motionId[0] != 0x368) {
                Actor_SetMotion(npc, 0x368, 0, 0);
            }
        } else if (Npc_IsFreeAct15to19(npc) == 1) {
            if (npc->motionId[0] != 0x2A4) {
                Actor_SetMotion(npc, 0x2A4, 4, 0);
            }
        } else if (Npc_IsFreeAct18Set(npc) == 1) {
            if (npc->motionId[0] != 0x40) {
                Actor_SetMotion(npc, 0x40, 4, 0);
            }
        } else if (Npc_HasSecondHead(npc) == 1) {
            if (npc->motionId[0] != 0x3F5) {
                Actor_SetMotionOnLayer(npc, 0x3F5, 4, 0, 0);
            }
        } else if (npc->motionId[0] != PL_MOT_WALK) {
            Actor_SetMotion(npc, PL_MOT_WALK, 4, 0);
        }
        break;
    case 1:
        if (npcWork->id == 6) {
            host = Npc_Find(NPC_CONTEST_HOST);
            if (host != NULL) {
                dist = Vec_Distance(&npc->posX, &host->posX);
                if (host->motionId[0] == 0x372 && dist < 800.0f && Motion_ReachedFrame(host, 0, 26.0f) &&
                    !Motion_ReachedFrame(host, 0, 60.0f)) {
                    Actor_SetState(npc, 0, 0xA3);
                }
            }
        }
        point = npcWork->route[npcWork->routeIdx].pos;
        point.v[1] = npc->posY;
        npc->targetYaw = (s16)(u16)Actor_HeadingDiff(npc, &point) / 10;
        npc->rotY += (s16)npc->targetYaw;
        dist = Vec_Distance(&npc->posX, (f32 *) &point);
        if (dist < 0.0f) {
            dist *= -1.0f;
        }
        if (dist <= 30.0f) {
            Npc_NextRoutePoint(npc);
        }
        break;
    }
}

/* State 0x04, running to the route point (PL_MOT_RUN for most), as
 * Npc_StateWalk walks. */
void Npc_StateRun(Em *npc)
{
    u8 step = npc->step;
    NpcWork *npcWork = (NpcWork *)&npc->work;
    NpcPos point;
    f32 dist;

    switch (step) {
    case 0:
        Actor_SetPosture(npc, 0, 0, 0);
        npc->step++;
        if (Npc_HasSecondHead(npc) == 1) {
            if (npc->motionId[0] != 0x3FD) {
                Actor_SetMotionOnLayer(npc, 0x3FD, 4, 0, 0);
            }
        } else if (Npc_IsType40(npc) == 1) {
            if (npc->motionId[0] != 0x369) {
                Actor_SetMotion(npc, 0x369, 0x14, 0);
            }
        } else if (npc->motionId[0] != PL_MOT_RUN) {
            Actor_SetMotion(npc, PL_MOT_RUN, 0x14, 0);
        }
        if (npcWork->id == NPC_WORKSHOP_HELPER_ONLINE) {
            npcWork->motionPhase = 1;
        }
        break;
    case 1:
        point = npcWork->route[npcWork->routeIdx].pos;
        point.v[1] = npc->posY;
        npc->targetYaw = (s16)(u16)Actor_HeadingDiff(npc, &point) / 10;
        npc->rotY += (s16)npc->targetYaw;
        dist = Vec_Distance(&npc->posX, (f32 *) &point);
        if (dist < 0.0f) {
            dist *= -1.0f;
        }
        if (dist <= 30.0f) {
            Npc_NextRoutePoint(npc);
        }
        break;
    }
}

/* States 0x35 and 0x36 (variant 0, 1): sits (PL_POSTURE_CROUCHED) in one
 * of two seated poses, 0x29E or 0x2A0, and keeps facing during a talk. */
void Npc_StateSeatPoseA(Em *npc, s8 variant)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npcWork->keepFacing = 1;
        npc->step++;
        Actor_SetPosture(npc, PL_POSTURE_CROUCHED, 0, 0);
        if (variant == 1) {
            if (npc->motionId[0] != 0x2A0) {
                Actor_SetMotion(npc, 0x2A0, 0x3C, 0);
            }
        } else {
            if (npc->motionId[0] != 0x29E) {
                Actor_SetMotion(npc, 0x29E, 0x3C, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

s32 Npc_IsFreeAct15to19(Em *arg0);
extern NpcPair gNpcPartnerPairs[];

/* States 0x43-0x45 and 0x8B (variant 0-3): stands with a partner, in one
 * of four poses (0x28B or 0x2B8 for some kinds). Townspeople 0x60 and 0x61
 * face each other within 200 and idle apart. In AREA_HUB_0x89 and
 * AREA_HUB_0x57 a member of a pair of gNpcPartnerPairs (ids by twos, 0xFF
 * ends) joins its partner's talk (Npc_StartTalkState) when the partner,
 * within 200, is talked to (state 0x2C or 0x42). */
void Npc_StatePairStand(Em *npc, s8 variant)
{
    NpcPair *pair = gNpcPartnerPairs;
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Em *partner;

    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetPosture(npc, PL_POSTURE_STANDING, 0, 0);
        if (Npc_IsFreeAct2Bto30(npc) == 1) {
            if (npc->motionId[0] != 0x28B) {
                Actor_SetMotion(npc, 0x28B, 8, 0);
            }
        } else if (Npc_IsFreeAct15to19(npc) == 1) {
            Actor_SetMotion(npc, 0x2B8, 0xA, 0);
        } else {
            switch (variant) {
            case 0:
                Actor_SetMotion(npc, 0x291, 0xA, 0);
                break;
            case 1:
                Actor_SetMotion(npc, 0x290, 0xA, 0);
                break;
            case 2:
                Actor_SetMotion(npc, 0x292, 0xC, 0);
                break;
            case 3:
                Actor_SetMotion(npc, 0x35B, 8, 0);
                break;
            }
        }
        break;
    case 1:
        switch (npcWork->id) {
        case 0x60:
            partner = Npc_Find(0x61);
            if (200.0f <= Vec_Distance(&npc->posX, &partner->posX)) {
                if (npc->motionId[0] != PL_MOT_IDLE) {
                    Actor_SetMotion(npc, PL_MOT_IDLE, 4, 0);
                }
            } else if (npc->motionId[0] != 0x290) {
                Actor_SetMotion(npc, 0x290, 4, 0);
            }
            break;
        case 0x61:
            partner = Npc_Find(0x60);
            if (200.0f <= Vec_Distance(&npc->posX, &partner->posX)) {
                if (npc->motionId[0] != PL_MOT_IDLE_ALT) {
                    Actor_SetMotion(npc, PL_MOT_IDLE_ALT, 4, 0);
                }
            } else if (npc->motionId[0] != 0x291) {
                Actor_SetMotion(npc, 0x291, 4, 0);
            }
            break;
        }
        if (gScene.area == AREA_HUB_0x89 || gScene.area == AREA_HUB_0x57) {
            if (Npc_IsFreeAct2Bto30(npc) == 1 && npc->motion[0].playing == 0 && npc->motionId[0] == 0x28B) {
                Actor_SetMotion(npc, 0x28C, 2, 0);
            }
            for (; pair->first != 0xFF; pair++) {
                if (pair->first == npcWork->id) {
                    partner = Npc_Find(pair->second);
                } else if (pair->second == npcWork->id) {
                    partner = Npc_Find(pair->first);
                }
                if (partner != NULL && (partner->state == NPC_STATE_TALK_DELAY || partner->state == NPC_STATE_TALK)) {
                    if (!(200.0f <= Vec_Distance(&npc->posX, &partner->posX))) {
                        Npc_StartTalkState(npc);
                    }
                }
            }
        }
        break;
    }
}

/* State 0x5A: puts the townsperson on its route point at once, then
 * standing (state 0x01). */
void Npc_StateWarpToRoute(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (npcWork->route == NULL) {
        Actor_SetState(npc, 0, 1);
    }
    *(NpcPos *)(&npc->posX) = *(NpcPos *)&npcWork->route[npcWork->routeIdx].pos;
    Actor_SetState(npc, 0, 1);
}

/* States 0x37-0x39 (variant 0, 1, 3): sits (PL_POSTURE_CROUCHED) in one
 * of three seated poses, 0x29A, 0x29C or 0x268, and keeps facing during a
 * talk. */
void Npc_StateSeatPoseB(Em *npc, s8 variant)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npcWork->keepFacing = 1;
        Actor_SetPosture(npc, PL_POSTURE_CROUCHED, 0, 0);
        if (variant == 1) {
            if (npc->motionId[0] != 0x29C) {
                Actor_SetMotion(npc, 0x29C, 0x3C, 0);
            }
        } else if (variant == 0) {
            if (npc->motionId[0] != 0x29A) {
                Actor_SetMotion(npc, 0x29A, 0x3C, 0);
            }
        } else {
            if (npc->motionId[0] != 0x268) {
                Actor_SetMotion(npc, 0x268, 0, 0x7C);
            }
        }
        break;
    case 1:
        break;
    }
}

s32 ElderRaid_InState1(void);

/* A townsperson's step machine (npc[5]): turn toward the player and pick the motion by kind
 * and current motion (Actor_SetMotion/Actor_SetMotionOnLayer), walk, then the next route point.
 * The game's empty delay slots come from the default-first switch on Npc_HasSecondHead's result. */
void Npc_StateTalk(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    NpcRouteEnt *tbl;

    switch (npc->motionId[0]) {
    case 0x2A5:
    case 0x2A6:
    case 0x285:
    case 0x268:
        Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        break;
    default:
        switch (npc->step) {
        case 0:
            npcWork->noTalk = 1;
            if ((s8)npcWork->keepFacing != 0 || npc->stance == 1) {
                npc->timer = 0;
            } else if (npc->motionId[0] != 0x36A) {
                npc->timer = 10;
                npc->targetYaw = Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos);
            }
            npc->step++;
            if (Npc_IsType49(npc) != 0) {
                if (npc->motionId[0] == 0x420) {
                    return;
                }
                Actor_SetMotionOnLayer(npc, 0x420, 8, 0, 0);
                return;
            }
            switch (Npc_HasSecondHead(npc)) {
            default:
                if (npc->motionId[0] == 0x425) {
                    Actor_SetMotionOnLayer(npc, 0x426, 6, 0, 0);
                    return;
                }
                if (npc->motionId[0] == 0x3EF) {
                    return;
                }
                Actor_SetMotionOnLayer(npc, 0x3EF, 0x10, 0, 0);
                return;
            case 0:
                break;
            }
            if (npcWork->id == NPC_FISHING_SPOT) {
                Actor_SetMotion(npc, 0x293, 0x14, 0);
                return;
            }
            if (Npc_IsType31to36(npc) == 1) {
                Actor_SetMotion(npc, 0x35B, 8, 0);
                return;
            }
            if (Npc_IsType4D(npc) == 1) {
                Actor_SetMotionOnLayer(npc, 0x3EB, 6, 0, 0);
                return;
            }
            if (Npc_IsType40(npc) == 1) {
                Actor_SetMotion(npc, 0x367, 4, 0);
                return;
            }
            if (npc->variant == 0x39) {
                Actor_SetMotion(npc, 0x356, 0x28, 0);
                return;
            }
            if (npcWork->id == NPC_CONTEST_HOST) {
                Actor_SetMotion(npc, 0x379, 0, 0);
                return;
            }
            if (npcWork->id == NPC_DRINK_CONTEST_HOST) {
                Actor_SetMotion(npc, 0x35B, 0, 0);
                return;
            }
            if (npcWork->id == NPC_OFFICIAL_DESK_HIGH_HR && ElderRaid_InState1() == 1) {
                Actor_SetMotionOnLayer(npc, 0x424, 0x1E, 0, 0);
                return;
            }
                switch (npc->motionId[0]) {
                case 0x29F:
                case 0x2B8:
                case 0x2A1:
                case 0x29B:
                case 0x29D:
                case 0x297:
                case 0x299:
                case 0x295:
                case 0x292:
                case 0x281:
                case 0x286:
                case 0x289:
                case 0x424:
                    break;
                case 0x29E:
                    Actor_SetMotion(npc, 0x29F, 0xA, 0);
                    return;
                case 0x383:
                case 0x382:
                    Actor_SetMotion(npc, 0xFC, 4, 0);
                    return;
                case 0x2A3:
                case 0x2A4:
                    Actor_SetMotion(npc, 0x2B8, 0x10, 0);
                    return;
                case 0x2A0:
                    Actor_SetMotion(npc, 0x2A1, 0xA, 0);
                    return;
                case 0x29A:
                    Actor_SetMotion(npc, 0x29B, 0xA, 0);
                    return;
                case 0x29C:
                    Actor_SetMotion(npc, 0x29D, 0xA, 0);
                    return;
                case 0x296:
                    Actor_SetMotion(npc, 0x297, 0xA, 0);
                    return;
                case 0x298:
                    Actor_SetMotion(npc, 0x299, 0xA, 0);
                    return;
                case 0x294:
                case 0x293:
                    Actor_SetMotion(npc, 0x295, 0xA, 0);
                    return;
                case 0x27E:
                    Actor_SetMotion(npc, 0x27F, 4, 0);
                    return;
                case 0x284:
                    Actor_SetMotion(npc, 0x286, 0x10, 0);
                    return;
                case 0x263:
                    Actor_SetMotion(npc, 0x261, 0x10, 0);
                    return;
                case 0x261:
                    Actor_SetMotion(npc, 0x289, 0, 0);
                    return;
                default:
                    if (npc->stance == 0 && npc->motionId[0] < 0x3E8) {
                        Actor_SetMotion(npc, 0x292, 0xC, 0);
                        return;
                    }
                    if (npcWork->id == 0x4B) {
                        return;
                    }
                    Actor_SetMotionOnLayer(npc, 0x3EF, 0x10, 0, 0);
                    return;
                }
            break;
        case 1:
            npcWork->noTalk = 1;
            npc->rotY += (s16)npc->targetYaw / 10;
            if (--npc->timer <= 0) {
                npc->step++;
            }
            break;
        case 2:
            npcWork->noTalk = 1;
            if (Npc_IsType49(npc) != 0 && npc->motion[0].playing <= 0) {
                Actor_SetMotionOnLayer(npc, 0x429, 8, 0, 0);
            } else if (Npc_HasSecondHead(npc) != 0 && npc->motion[0].playing <= 0) {
                Actor_SetMotionOnLayer(npc, 0x3F0, 8, 0, 0);
            } else if (npc->motionId[0] == 0x261 && Motion_ReachedFrame(npc, 0, 5.0f) != 0) {
                Actor_SetMotion(npc, 0x289, 0, 0);
            }
            if (gTownMode == TOWN_MODE_FREE) {
                npc->timer = 0x10;
                npc->step++;
                if (npcWork->id == NPC_FISHING_SPOT || npcWork->id == 0x29) {
                    npc->timer = 10;
                }
            }
            break;
        case 3:
            npcWork->noTalk = 1;
            switch (npcWork->id) {
            case 0x29:
            case NPC_FISHING_SPOT:
            case 0x1E:
            case 0x2A:
            case 0x30:
            case NPC_FISHING_SPOT_ONLINE:
                npcWork->noTalk = 1;
                break;
            }
            if (npc->motionId[0] == 0x286) {
                if (Motion_ReachedFrame(npc, 0, 260.0f) != 0) {
                    Actor_SetMotion(npc, 0x284, 0x12, 0);
                }
                break;
            }
            if (--npc->timer <= 0) {
                if ((s8)npcWork->keepFacing != 0) {
                    npc->timer = 0;
                    if (Npc_IsType4D(npc) == 1) {
                        Actor_SetMotionOnLayer(npc, 0x3EC, 0xE, 0, 0);
                    } else {
                        switch (npc->motionId[0]) {
                        case 0x27F:
                            Actor_SetMotion(npc, 0x281, 0xA, 0);
                            break;
                        }
                    }
                } else {
                    npc->timer = 10;
                    if (npcWork->id != 1 && Npc_IsType4B(npc) != 1) {
                        if (npcWork->id == NPC_WORKSHOP_HELPER) {
                            npc->timer = 10;
                            Actor_SetMotionOnLayer(npc, 0x406, 0x14, 0, 0);
                        } else if (Npc_IsType49(npc) != 0 || Npc_HasSecondHead(npc) != 0) {
                            if (npcWork->id != NPC_OFFICIAL_DESK_HIGH_HR) {
                                Actor_SetMotionOnLayer(npc, 0x3E9, 0xA, 0, 0);
                            }
                        } else {
                            switch (npc->motionId[0]) {
                            case 0x295:
                            case 0x424:
                                npcWork->noTalk = 1;
                                break;
                            case 0x2B8:
                                Actor_SetMotion(npc, 0x2A3, 0xA, 0);
                                break;
                            case 0x356:
                            case 0xFC:
                            case 0x379:
                                break;
                            case 0x35B:
                                Actor_SetMotion(npc, 0x35C, 0x14, 0);
                                break;
                            case 0x367:
                                Actor_SetMotion(npc, 0x364, 6, 0);
                                break;
                            default:
                                if (npc->state != NPC_STATE_SEATED_AT_TABLE && npc->state != NPC_STATE_SEATED && npc->state != NPC_STATE_SIT_DOWN) {
                                    Actor_SetMotion(npc, PL_MOT_IDLE, 0xA, 0);
                                }
                                break;
                            }
                        }
                    }
                }
                npc->step++;
            }
            break;
        case 4:
            npcWork->noTalk = 1;
            if (npc->motion[0].playing != 0) {
                switch (npc->motionId[0]) {
                case 0x35C:
                case 0x3EC:
                    return;
                }
            }
            switch (npcWork->id) {
            case 0x29:
            case NPC_FISHING_SPOT:
            case 0x1E:
            case 0x2A:
            case 0x30:
            case NPC_FISHING_SPOT_ONLINE:
            case 0x60:
            case 0x69:
            case 0x61:
            case NPC_CONTEST_HOST:
                npcWork->noTalk = 1;
                break;
            }
            npc->rotY -= (s16)npc->targetYaw / 10;
            if (--npc->timer <= 0) {
                tbl = npcWork->route;
                if (tbl != NULL) {
                    Actor_SetState(npc, 0, *(u16 *)&tbl[npcWork->routeIdx].state);
                    return;
                }
                Actor_SetState(npc, 0, 1);
            }
            break;
        }
        break;
    }
}

/* States 0x46 and 0x47 (variant 0, 1): holds pose 0x293 or 0x294; the
 * talk answers from it with 0x295. */
void Npc_State46(Em *npc, s8 variant)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        switch (variant) {
        case 0:
            if (npc->motionId[0] != 0x293) {
                Actor_SetMotion(npc, 0x293, 0x14, 0);
            }
            break;
        case 1:
            if (npc->motionId[0] != 0x294) {
                Actor_SetMotion(npc, 0x294, 0x14, 0);
            }
            break;
        }
        break;
    }
}

/* States 0x3A and 0x3B (variant 0, 1): holds pose 0x296 or 0x298 and
 * keeps facing during a talk; in state 0x3A it can't be talked to for its
 * first 20 frames. */
void Npc_State3A(Em *npc, s8 variant)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    u8 step;

    npcWork->keepFacing = 1;
    step = npc->step;
    switch (step) {
    case 0:
        npc->step = step + 1;
        switch (variant) {
        case 0:
            if (npc->motionId[0] != 0x296) {
                npc->timer = 0x14;
                npcWork->noTalk = 1;
                Actor_SetMotion(npc, 0x296, 0x14, 0);
            }
            break;
        case 1:
            if (npc->motionId[0] != 0x298) {
                Actor_SetMotion(npc, 0x298, 0xA, 0);
            }
            break;
        }
        break;
    case 1:
        if (npc->timer != 0) {
            npcWork->noTalk = 1;
            if (--npc->timer <= 0) {
                npcWork->noTalk = 0;
            }
        }
        break;
    }
}

s32 Area_IsTownHall(s32 arg0);

/* State 0x41: seated at a table (PL_POSTURE_CROUCHED; PL_MOT_TOWN_SIT,
 * then PL_MOT_TOWN_SEATED), keeping facing during a talk. After a random
 * 0-255 frames it gestures: 0x263 in AREA_TAVERN, AREA_TOWN_ARENA and
 * AREA_ELDER_HALL (Area_IsTownHall), 0x289 elsewhere (at random once or
 * twice), then settles again. */
void Npc_StateSeatedAtTable(Em *npc)
{
    u8 step = npc->step;
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npc->timer = (u8)((u16)Rand_Next(1) + 0xB4);
        npcWork->keepFacing = 1;
        Actor_SetPosture(npc, PL_POSTURE_CROUCHED, 0, 0);
        if (npc->motionId[0] == PL_MOT_IDLE) {
            Actor_SetMotion(npc, PL_MOT_TOWN_SIT, 6, 0);
        } else if (npc->motionId[0] == 0x289) {
            Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 0xA, 0);
        } else if (npc->motionId[0] != PL_MOT_TOWN_SEATED) {
            Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 0, 0);
        }
        break;
    case 1:
        if (Area_IsTownHall(gScene.area) == 1) {
            if (--npc->timer <= 0) {
                npc->step++;
                Actor_SetMotion(npc, 0x263, 6, 0);
            }
        } else if (--npc->timer <= 0) {
            npc->step++;
            if (npc->motionId[0] != 0x289) {
                Actor_SetMotion(npc, 0x289, 0, 0);
            }
        }
        break;
    case 2:
        if (Area_IsTownHall(gScene.area) == 1) {
            if (npc->motion[0].playing <= 0) {
                npc->step++;
                Actor_SetMotion(npc, 0x261, 4, 0);
            }
        } else if (npc->motion[0].playing <= 0) {
            if (((u16)Rand_Next(1) & 1) == 0) {
                npc->timer = 0;
                npc->step--;
            } else {
                npc->step++;
                Actor_SetMotion(npc, 0x261, 4, 0);
            }
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            npc->step = 0;
        }
        break;
    }
}

/* States 0x14 and 0x25 (flag 0, 1): sits down at a table (PL_MOT_TOWN_SIT,
 * then PL_MOT_TOWN_SEATED, and the state becomes 0x25), or with flag 1 is
 * seated already. It can't be talked to while it sits down. */
void Npc_StateSitDown(Em *npc, u8 seated)
{
    u8 step = npc->step;
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npc->motionTime = 0;
        npcWork->noTalk = 1;
        Actor_SetPosture(npc, PL_POSTURE_CROUCHED, 0, 0);
        if (seated == 1) {
            npc->step = 2;
            if (npc->motionId[0] != PL_MOT_TOWN_SEATED) {
                Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 0, 0);
            }
        } else {
            Actor_SetMotion(npc, PL_MOT_TOWN_SIT, -4, 0);
        }
        break;
    case 1:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing == 0) {
            npc->step++;
            Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 4, 0);
            npc->state = 0x25;
        }
        break;
    case 2:
        break;
    }
}

/* States 0x48 and 0x49 (variant 0, 1): plays 0x2A5, then holds 0x2A6;
 * state 0x49 holds 0x2A6 at once. A talk leaves these poses alone
 * (Npc_StateTalk). The goto shares the hold between the two cases, as the
 * game's code does. */
void Npc_State48(Em *npc, s32 variant)
{
    u16 motion;
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (!(variant & 0xFF)) {
            motion = npc->motionId[0];
            if ((motion != 0x2A5) && (motion != 0x2A6)) {
                Actor_SetMotion(npc, 0x2A5, 0, 0);
                break;
            }
        }
        break;
    case 1:
        if (npc->motion[0].playing > 0) {
            if (variant & 0xFF) {
                goto block_10;
            }
        } else {
block_10:
            if (npc->motionId[0] != 0x2A6) {
                Actor_SetMotion(npc, 0x2A6, 0, 0);
            }
        }
        break;
    }
}

/* State 0x4A: holds pose 0x2A7; it can't be talked to. */
void Npc_State4A(Em *npc)
{
    ((NpcWork *)&npc->work)->noTalk = 1;
    if (npc->motionId[0] != 0x2A7) {
        Actor_SetMotion(npc, 0x2A7, 6, 0);
    }
}

/* State 0x42: waits 8 frames, then the talk (state 0x2C). */
void Npc_StateTalkDelay(Em *npc)
{
    s32 timer;
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->timer = 8;
        npc->step = (u8) (npc->step + 1);
        break;
    case 1:
        timer = npc->timer - 1;
        npc->timer = timer;
        if (timer <= 0) {
            Actor_SetState(npc, 0, 0x2CU);
        }
        break;
    }
}

/* State 0x5B: stands for a random time (the first route point's argument
 * plus a random number, kept as a byte), then state 0x01. */
void Npc_StateRouteWait(Em *obj)
{
    NpcWork *npcWork = (NpcWork *)&obj->work;
    u8 step = obj->step;
    NpcRouteEnt *point;

    switch (step) {
    case 0:
        obj->step = step + 1;
        point = npcWork->route;
        obj->timer = (u8)(point->arg + (u16)Rand_Next(1));
        if (Npc_IsFree(obj) == 1) {
            Actor_SetMotion(obj, PL_MOT_IDLE, 0x14, 0);
        } else {
            Actor_SetMotionOnLayer(obj, 0x3E9, 0x14, 0, 0);
        }
        break;
    case 1:
        if (--obj->timer <= 0) {
            Actor_SetState(obj, 0, 1);
        }
        break;
    }
}

/* State 0x84: stands, and sets the wait at its route's first point
 * (route[0].arg) to a random 0-145 frames in steps of 5. */
void Npc_StateRandomizeWait(Em *obj)
{
    NpcWork *npcWork = (NpcWork *)&obj->work;
    u8 step = obj->step;
    NpcRouteEnt *point;

    switch (step) {
    case 0:
        obj->step = step + 1;
        point = npcWork->route;
        point->arg = ((u16)Rand_Next(1) % 30) * 5;
        if (obj->motionId[0] == 0) {
            if (Npc_IsFree(obj) == 1) {
                Actor_SetMotion(obj, PL_MOT_IDLE, 0, 0);
            } else {
                Actor_SetMotionOnLayer(obj, 0x3E9, 0, 0, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

/* State 0x85: holds pose 0x35D. The empty case falls through, as in the
 * game's code. */
void Npc_State85(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        if (npc->motionId[0] != 0x35D) {
            Actor_SetMotion(npc, 0x35D, 6, 0);
            npc->step = (u8) (npc->step + 1);
        }
        
    case 1:
        break;
    }
}

/* State 0x86: holds pose 0x35E. */
void Npc_State86(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 0x35E, 0xA, 0);
        
    case 1:
        break;
    }
}

/* State 0x87: holds pose 0x265. */
void Npc_State87(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 0x265, 6, 0);
        
    case 1:
        break;
    }
}

/* State 0x88: holds pose 0x266. */
void Npc_State88(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 0x266, 6, 0);
        
    case 1:
        break;
    }
}

/* State 0x89: holds pose 0x262. */
void Npc_State89(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 0x262, 6, 0);
        
    case 1:
        break;
    }
}

/* State 0x4B: holds pose 0x27E (once 0x281 has ended) and keeps facing
 * during a talk. */
void Npc_State4B(Em *obj)
{
    NpcWork *npcWork = (NpcWork *)&obj->work;
    u16 mot;

    switch (obj->step) {
    case 0:
        npcWork->keepFacing = 1;
        mot = obj->motionId[0];
        if (mot != 0x27E) {
            if (mot == 0x281) {
                if (obj->motion[0].playing <= 0) {
                    Actor_SetMotion(obj, 0x27E, 2, 0);
                    obj->step++;
                }
            } else {
                Actor_SetMotion(obj, 0x27E, 2, 0);
                obj->step++;
            }
        } else {
            obj->step++;
        }
        break;
    }
}

/* States 0x4C and 0x4D (variant 0, 1): holds pose 0x285 or 0x284 and
 * keeps facing during a talk. */
void Npc_State4C(Em *npc, s8 variant)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npcWork->keepFacing = 1;
        npc->step++;
        switch (variant) {
        case 0:
            if (npc->motionId[0] != 0x285) {
                Actor_SetMotion(npc, 0x285, 0, 0);
            }
            break;
        case 1:
            if (npc->motionId[0] != 0x284) {
                Actor_SetMotion(npc, 0x284, 0, 0);
            }
            break;
        }
        break;
    }
}

/* State 0x30, a working townsperson's idle (Npc_HasSecondHead; any
 * other goes back to state 0x01): holds 0x3FF, keeping facing during a
 * talk, and after a random 0-255 frames plays 0x400 once. Its talk is
 * state 0x31. */
void Npc_StateBusyIdle(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    u8 step;

    if (Npc_HasSecondHead(npc) == 0) {
        Actor_SetState(npc, 0, 1);
    }
    step = npc->step;
    switch (step) {
    case 0:
        npc->step = step + 1;
        npcWork->keepFacing = 1;
        npc->timer = (u8)((u16)Rand_Next(1) + 0x12C);
        if (npc->motionId[0] != 0x3FF) {
            Actor_SetMotionOnLayer(npc, 0x3FF, 2, 0, 0);
        }
        break;
    case 1:
        if (--npc->timer <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x400, 2, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step = 0;
        }
        break;
    }
}

/* State 0x31, the talk of a townsperson in state 0x30 or 0xA2: 0x401,
 * 0x402, then 0x403 while the talk lasts (no talk again meanwhile); when
 * it ends, 0x402 for 96-127 frames and 0x3FF for 600-855 frames, then its
 * route point's state. */
void Npc_StateBusyTalk(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npcWork->noTalk = 1;
        npc->step++;
        Actor_SetMotionOnLayer(npc, 0x401, 8, 0, 0);
        break;
    case 1:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x402, 2, 0, 0);
        } else if (gTownMode == TOWN_MODE_FREE) {
            npc->step += 3;
            npc->timer = ((u16)Rand_Next(1) & 0x1F) + 0x60;
        }
        break;
    case 2:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x403, 2, 0, 0);
        } else if (gTownMode == TOWN_MODE_FREE) {
            npc->step += 2;
            npc->timer = ((u16)Rand_Next(1) & 0x1F) + 0x60;
            Actor_SetMotionOnLayer(npc, 0x402, 0xA, 0, 0);
        }
        break;
    case 3:
        npcWork->noTalk = 1;
        if (gTownMode == TOWN_MODE_FREE) {
            npc->step++;
            npc->timer = ((u16)Rand_Next(1) & 0x1F) + 0x60;
            Actor_SetMotionOnLayer(npc, 0x402, 0xA, 0, 0);
        }
        break;
    case 4:
        if (--npc->timer <= 0) {
            npc->step++;
            npc->timer = (u8)Rand_Next(1) + 600;
            Actor_SetMotionOnLayer(npc, 0x3FF, 0x3C, 0, 0);
        }
        break;
    case 5:
        if (--npc->timer <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* State 0xA2: plays 0x401, 0x402 and 0x403 once and holds the last; a
 * talk takes it to state 0x31. */
void Npc_StateBusyTalkHold(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        if (npc->motionId[0] != 0x401) {
            Actor_SetMotionOnLayer(npc, 0x401, 8, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = step + 1;
            Actor_SetMotionOnLayer(npc, 0x402, 2, 0, 0);
        }
        break;
    case 2:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x403, 2, 0, 0);
        }
        break;
    }
}

/* State 0x0E, a working townsperson's pose 0x3F7 (any other goes back to
 * state 0x01). Its talk is state 0x32. */
void Npc_StateBusyPose(Em *npc)
{
    u8 step;

    if (Npc_HasSecondHead(npc) == 0) {
        Actor_SetState(npc, 0, 1);
    }
    step = npc->step;
    switch (step) {
    case 0:
        npc->step = step + 1;
        Actor_SetMotionOnLayer(npc, 0x3F7, 2, 0, 0);
        break;
    }
}

/* State 0x32, the talk of a townsperson in state 0x0E: 0x3F8 (unless it
 * was standing), 0x3F9, then 0x3FA held while the talk lasts (no talk
 * again meanwhile); afterwards 0x3F9 for 600-855 frames and 0x3F7, then
 * its route point's state. */
void Npc_StateBusyPoseTalk(Em *npc)
{
    u8 step = npc->step;
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npcWork->noTalk = 1;
        if (npc->motionId[0] != PL_MOT_IDLE) {
            Actor_SetMotionOnLayer(npc, 0x3F8, 4, 0, 0);
        }
        break;
    case 1:
        npc->step = step + 1;
        npcWork->noTalk = 1;
        Actor_SetMotionOnLayer(npc, 0x3F9, 2, 0, 0);
        break;
    case 2:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing <= 0 && npc->motionId[0] != 0x3FA) {
            Actor_SetMotionOnLayer(npc, 0x3FA, 2, 0, 0);
        }
        if (gTownMode == TOWN_MODE_FREE) {
            npc->timer = (u8)Rand_Next(1) + 600;
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x3F9, 0x1A, 0, 0);
        }
        break;
    case 3:
        if (--npc->timer == 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x3F7, 0x1E, 0, 0);
        } else if (npc->motion[0].playing == 0) {
            Actor_SetMotionOnLayer(npc, 0x3F9, 2, 0, 0);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* State 0x33, a working townsperson's loop of 0x3FB (any other goes back
 * to state 0x01), keeping facing during a talk. Its talk is state 0x34. */
void Npc_StateBusyLoop(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_HasSecondHead(npc) == 0) {
        Actor_SetState(npc, 0, 1);
    }
    switch (npc->step) {
    case 0:
        npcWork->keepFacing = 1;
        npc->step++;
        if (npc->motionId[0] != 0x3FB) {
            Actor_SetMotionOnLayer(npc, 0x3FB, 2, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing == 0) {
            Actor_SetMotionOnLayer(npc, 0x3FB, 2, 0x58, 0);
        }
        break;
    }
}

/* State 0x34, the talk of a townsperson in state 0x33: finishes 0x3FB (to
 * frame 89), plays 0x3FC and holds 0x3F0 while the talk lasts, then 0x3FB
 * and its route point's state. It can't be talked to again meanwhile. */
void Npc_StateBusyLoopTalk(Em *npc)
{
    u8 step;
    NpcWork *npcWork;

    npcWork = (NpcWork *)&npc->work;
    npcWork->noTalk = 1;
    step = npc->step;
    switch (step) {
    case 0:
        if (npc->motionId[0] == 0x3FB) {
            if (Motion_ReachedFrame(npc, 0, 89.0f) != 0) {
                npc->step++;
            }
        } else if (npc->motionId[0] == 0x3FC || npc->motionId[0] == 0x3F0) {
            npc->step = 2;
        } else {
            npc->step = step + 1;
        }
        break;
    case 1:
        npc->step = step + 1;
        Actor_SetMotionOnLayer(npc, 0x3FC, 4, 0, 0);
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step = step + 1;
            Actor_SetMotionOnLayer(npc, 0x3F0, 4, 0, 0);
        }
        break;
    case 3:
        if (gTownMode == TOWN_MODE_FREE) {
            Actor_SetMotionOnLayer(npc, 0x3FB, 0xA, 0, 0);
            npc->step++;
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* State 0x3F, the idle of kinds 0x3C-0x3E: holds 0x2B2 for a random 0-255
 * frames, then either 0x2B3 3 to 6 times and 0x2B4, or 0x2B6 once. Its
 * talk is state 0x40. */
void Npc_State3F(Em *npc)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npc->timer = (u8)((u16)Rand_Next(1) + 100);
        Actor_SetMotion(npc, 0x2B2, 0, 0);
        break;
    case 1:
        if (--npc->timer <= 0) {
            if (((u16)Rand_Next(1) & 1) == 0) {
                npc->step++;
                npc->timer = ((u16)Rand_Next(1) & 3) + 3;
                Actor_SetMotion(npc, 0x2B3, 6, 0);
            } else {
                npc->step = 4;
                Actor_SetMotion(npc, 0x2B6, 8, 0);
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->step++;
            Actor_SetMotion(npc, 0x2B4, 0xA, 0);
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            npc->step = 1;
            npc->timer = 0x78;
            Actor_SetMotion(npc, 0x2B2, 4, 0);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            npc->step = 1;
            npc->timer = (u8)((u16)Rand_Next(1) + 100);
            Actor_SetMotion(npc, 0x2B2, 0xA, 0);
        }
        break;
    }
}

/* State 0x40, the talk of kinds 0x3C-0x3E: turns to the local player over
 * 10 frames with 0x2B5; when the talk ends it turns back (Npc_StartTurnBack)
 * with 0x2B2 and, once the turn's layer is done, returns to its route
 * point's state. No talk again meanwhile. */
void Npc_State40(Em *npc)
{
    u8 step;
    NpcWork *npcWork;

    step = npc->step;
    npcWork = (NpcWork *)&npc->work;
    switch (step) {
    case 0:
        npcWork->noTalk = 1;
        npc->step = (u8) (npc->step + 1);
        Actor_SetMotion(npc, 0x2B5, 0xA, 0);
        Npc_TurnToPlayer(npc, 0xA);
        break;
    case 1:
        npcWork->noTalk = 1;
        if (gTownMode == TOWN_MODE_FREE) {
            npc->step = (u8) (npc->step + 1);
            Actor_SetMotion(npc, 0x2B2, 0xA, 0);
            Npc_StartTurnBack(npc, 0xA);
        }
        break;
    case 2:
        if (npc->motion[1].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* State 0x2E, the idle of kinds 0x2B-0x30: holds 0x28A for a random 0-255
 * frames, then plays 0x28E once. Its talk is state 0x2F. */
void Npc_State2E(Em *npc)
{
    s32 timer;
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        npc->timer = (s32) (((Rand_Next(1) & 0xFFFF) + 0xA) & 0xFF);
        if (npc->motionId[0] != 0x28A) {
            Actor_SetMotion(npc, 0x28A, 0, 0);
        }
        break;
    case 1:
        timer = npc->timer - 1;
        npc->timer = timer;
        if (timer <= 0) {
            npc->step = (u8) (npc->step + 1);
            Actor_SetMotion(npc, 0x28E, 0xA, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing == 0) {
            npc->step = 0U;
        }
        break;
    }
}

/* State 0x2F, the talk of kinds 0x2B-0x30: 0x28B, then 0x28C while the
 * talk lasts (no talk again meanwhile); afterwards 0x28D and 0x28A for
 * 255-510 frames, then its route point's state. Offline, townsperson 2
 * standing with townsperson 1 (Npc_CheckHubPartner) stays in its talk pose. The
 * goto shares the ending between the offline and online cases, as the
 * game's code does. */
void Npc_State2F(Em *npc)
{
    s32 timer;
    u8 step;
    NpcWork *npcWork;

    step = npc->step;
    npcWork = (NpcWork *)&npc->work;
    switch (step) {
    case 0:
        npcWork->noTalk = 1;
        npc->step = (u8) (npc->step + 1);
        if (npc->motionId[0] != 0x28B) {
            Actor_SetMotion(npc, 0x28B, 8, 0);
        }
        break;
    case 1:
        if ((npc->motion[0].playing == 0) && (npc->motionId[0] == 0x28B)) {
            Actor_SetMotion(npc, 0x28C, 2, 0);
        }
        if (gTownMode == TOWN_MODE_FREE) {
            if (Game_IsOnline() == 0) {
                if (Npc_CheckHubPartner(npc) != 1) {
                    goto block_15;
                }
            } else {
block_15:
                npc->step = (u8) (npc->step + 1);
                Actor_SetMotion(npc, 0x28D, 2, 0);
                break;
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing == 0) {
            Actor_SetMotion(npc, 0x28A, 0xE, 0);
            npc->timer = (s32) ((Rand_Next(1) & 0xFF) + 0xFF);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 3:
        timer = npc->timer - 1;
        npc->timer = timer;
        if (timer <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* State 0x3C, a greeter's greeting (from state 0x01 when the local player
 * comes within 400): 0x2AC if the player stands or walks (TOWN_PC_IDLE,
 * TOWN_PC_WALK), else 0x2AD; within 300 it goes on to state 0x3D, beyond
 * 400 it waits 120 frames and returns to its route point's state. */
void Npc_StateGreet(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = npc->step;
    f32 dist;

    switch (step) {
    case 0:
        npc->step = step + 1;
        if (pl->state == TOWN_PC_WALK || pl->state == TOWN_PC_IDLE) {
            Actor_SetMotion(npc, 0x2AC, 0xA, 0);
        } else {
            Actor_SetMotion(npc, 0x2AD, 8, 0);
        }
        break;
    case 1:
        dist = Vec_Distance(&npc->posX, (f32 *) pl->pos);
        if (dist < 300.0f) {
            npc->step++;
            if (npc->motionId[0] != 0x2AD) {
                Actor_SetMotion(npc, 0x2AD, 8, 0);
            }
        } else if (!(dist <= 400.0f)) {
            npc->step = 3;
            npc->timer = 0x78;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, NPC_STATE_GREET_HOLD);
        }
        break;
    case 3:
        if (--npc->timer <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* State 0x3D, a greeter beside the player: 0x2A9 or 0x2A8 while the local
 * player stays within 300; 120 frames after the player has left it
 * returns to its route point's state. */
void Npc_StateGreetHold(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        if (((u16)Rand_Next(1) & 1) == 0) {
            Actor_SetMotion(npc, 0x2A9, 4, 0);
        } else {
            Actor_SetMotion(npc, 0x2A8, 8, 0);
        }
        break;
    case 1:
        if (!(Vec_Distance(&npc->posX, (f32 *) pl->pos) <= 300.0f)) {
            npc->step++;
            npc->timer = 0x78;
        }
        break;
    case 2:
        if (--npc->timer <= 0) {
            if (Vec_Distance(&npc->posX, (f32 *) pl->pos) < 300.0f) {
                npc->step--;
            } else {
                Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
            }
        }
        break;
    }
}

/* State 0x3E, a greeter's talk: 0x2AA, then 0x2A8 while the local player
 * is within 300; 120 frames after the player has left it returns to its
 * route point's state. */
void Npc_StateGreetTalk(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];

    switch (npc->step) {
    case 0:
        npcWork->noTalk = 1;
        npc->step++;
        Actor_SetMotion(npc, 0x2AA, 10, 0);
        break;
    case 1:
        npcWork->noTalk = 1;
        Actor_SetMotion(npc, 0x2A8, 0x10, 0);
        npc->step++;
        break;
    case 2:
        if (!(Vec_Distance(&npc->posX, (f32 *) pl->pos) <= 300.0f) && npc->motion[0].playing <= 0) {
            npc->step++;
            npc->timer = 120;
        }
        break;
    case 3:
        if (--npc->timer <= 0) {
            if (Vec_Distance(&npc->posX, (f32 *) pl->pos) < 300.0f) {
                npc->step--;
                break;
            }
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* State 0x7A, townsperson 2 standing with townsperson 1 (as state 0x43):
 * back to its route point's state once 1 is no longer near, and into its
 * talk state (Npc_StartTalkState) when 1, near, does something else. */
void Npc_StatePairWait(Em *npc)
{
    s32 near;
    NpcWork *npcWork;

    npcWork = (NpcWork *)&npc->work;
    Npc_StatePairStand(npc, 0);
    near = Npc_CheckHubPartner(npc);
    switch (near) {
    case 0:
        Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        break;
    case 2:
        Npc_StartTalkState(npc);
        break;
    }
}

/* State 0x5F: the townsperson leaves: its slot ends (state 0x60: the
 * update then ends its task) and unk49 marks it as leaving. */
void Npc_StateLeave(Em *npc) {
    npc->state = 0x60;
    ((NpcWork *)&npc->work)->leaving = 1;
}

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);

f32 Vec_Distance(f32 *a, f32 *b);

/* A townsperson that steps along its route by the player: it moves on
 * when the player (within 800) is nearer the next point than this one. */
void Npc_StateStepAhead(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&npc->work;
    NpcPos *point;
    f32 dnext;
    f32 dcur;

    dnext = Vec_Distance((f32 *) pl->pos, w->route[w->routeIdx + 1].pos.v);
    point = &w->route[w->routeIdx].pos;
    dcur = Vec_Distance((f32 *) pl->pos, point->v);
    switch (npc->step) {
    case 0:
        npc->step++;
        *(NpcPos *)npc->goal = *point;
        *(NpcPos *)(&npc->posX) = *(NpcPos *)npc->goal;
        w->noTalk = 1;
        if (npc->motionId[0] == 0x354) {
            npc->rotY = 0x2000;
            Actor_SetMotion(npc, 0x355, 0, 0);
        } else {
            npc->step = 2;
            npc->rotY = 0x2000;
            Actor_SetMotion(npc, 1, 0x10, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotion(npc, 1, 0x10, 0);
        } else {
            w->noTalk = 1;
        }
        break;
    case 2:
        if ((dcur < 800.0f || dnext < 800.0f) && pl->pos[1] < 200.0f && dcur > dnext) {
            npc->step++;
            Actor_SetMotion(npc, 0x352, 0, 0);
            w->noTalk = 1;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            if ((dcur < 800.0f || dnext < 800.0f) && dcur < dnext) {
                npc->step = 1;
                *(NpcPos *)npc->goal = *point;
                *(NpcPos *)(&npc->posX) = *(NpcPos *)npc->goal;
                Actor_SetMotion(npc, 0x355, 0, 0);
                w->noTalk = 1;
            } else {
                (w->routeIdx)++;
                Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
                npc->timer = 1;
            }
        } else {
            w->noTalk = 1;
        }
        break;
    }
}


/* The same as Npc_StateStepAhead, back along the route (the previous point). */
void Npc_StateStepBack(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&npc->work;
    NpcPos *point;
    f32 dnext;
    f32 dcur;

    dnext = Vec_Distance((f32 *) pl->pos, w->route[w->routeIdx - 1].pos.v);
    point = &w->route[w->routeIdx].pos;
    dcur = Vec_Distance((f32 *) pl->pos, point->v);
    switch (npc->step) {
    case 0:
        npc->step++;
        *(NpcPos *)npc->goal = *point;
        *(NpcPos *)(&npc->posX) = *(NpcPos *)npc->goal;
        w->noTalk = 1;
        if (npc->motionId[0] == 0x352) {
            npc->rotY = 0x2AAB;
            Actor_SetMotion(npc, 0x353, 0, 0);
        } else {
            npc->step = 2;
            npc->rotY = 0x2AAB;
            Actor_SetMotion(npc, 1, 0x10, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotion(npc, 1, 0x10, 0);
        } else {
            w->noTalk = 1;
        }
        break;
    case 2:
        if ((dcur < 800.0f || dnext < 800.0f) && pl->pos[1] < 200.0f && dcur > dnext) {
            npc->step++;
            Actor_SetMotion(npc, 0x354, 0, 0);
            w->noTalk = 1;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            if ((dcur < 800.0f || dnext < 800.0f) && dcur < dnext) {
                npc->step = 1;
                *(NpcPos *)npc->goal = *point;
                *(NpcPos *)(&npc->posX) = *(NpcPos *)npc->goal;
                Actor_SetMotion(npc, 0x353, 0, 0);
                w->noTalk = 1;
            } else {
                (w->routeIdx)--;
                Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
                npc->timer = 1;
            }
        } else {
            w->noTalk = 1;
        }
        break;
    }
}

/* unit's own declaration: a u16 area (the definition takes s32): Npc_StateSmithTalk's call compiles as the game's code only with this type */
struct NpcAreaEntry *NpcTable_Find(u8 kind, u16 id);
void Npc_TurnToPlayer(Em *arg0, s32 arg1);
void Npc_StartTurnTo(Em *arg0, s32 arg1, s32 arg2);
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
extern s32 gTownMode;

/* States 0x68 and 0x7F (flag 0, 1), the smith's talk (NPC_WORKSHOP_SMITH;
 * 0x7F follows a forging): turns to the local player over 6 frames with
 * 0x40A; when the talk ends it stands (0x406), turns to its forge (heading
 * 0xED84) if it is up high (above 200), and once turned returns to its
 * route point's state. No talk again meanwhile. The NpcTable_Find call's
 * result is unused (Em.work is the townsperson's id, NpcWork.id); the
 * call stays, as in the game's code. */
void Npc_StateSmithTalk(Em *npc, u8 afterForge)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    u8 step;

    NpcTable_Find(npc->work, gScene.area);
    npcWork->noTalk = 1;
    step = npc->step;
    switch (step) {
    case 0:
        npc->step = step + 1;
        Npc_TurnToPlayer(npc, 6);
        if (npc->motionId[0] != 0x40A) {
            if (afterForge == 0) {
                Actor_SetMotionOnLayer(npc, 0x40A, 4, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, 0x40A, 4, 0x9A, 0);
            }
        }
        break;
    case 1:
        if (gTownMode == TOWN_MODE_FREE) {
            if (!(npc->posY <= 200.0f)) {
                Npc_StartTurnTo(npc, 0xC, 0xED84);
            }
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x406, 0xA, 0, 0);
        }
        break;
    case 2:
        if ((u16)npcWork->turnCount == 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

s32 Motion_ReachedFrame(void *, s32, f32);

/* The smith forging alone (state 0x69): strikes with 0x40B, 0x40C twice
 * and 0x40D (motionPhase marks frames 30 to 118), then 0x40E. If the
 * player has it locked on, it turns away first and back to the player at
 * the end, then talks (state 0x7F); otherwise it returns to its route
 * point's state. */
void Npc_SmithForgeAlone(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc == gPlayers[gScene.localPlayer].target) {
            w->turnStep = -w->turnStep;
            w->turnCount = 6;
        }
        w->motionPhase = 0;
        if (npc->motionId[0] != 0x40B) {
            Actor_SetMotionOnLayer(npc, 0x40B, 0, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            npc->timer = 2;
            Actor_SetMotionOnLayer(npc, 0x40C, 4, 0, 0);
        }
        break;
    case 2:
        if (w->motionPhase == 0 && Motion_ReachedFrame(npc, 0, 30.0f)) {
            w->motionPhase = 1;
        }
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x40D, 0, 0, 0);
        }
        break;
    case 3:
        if (w->motionPhase == 1 && Motion_ReachedFrame(npc, 0, 118.0f)) {
            w->motionPhase = 0;
        }
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            if (npc == gPlayers[gScene.localPlayer].target) {
                Npc_TurnToPlayer(npc, 6);
            }
            Actor_SetMotionOnLayer(npc, 0x40E, 0x10, 0, 0);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            if (npc == gPlayers[gScene.localPlayer].target) {
                Actor_SetState(npc, 0, NPC_STATE_SMITH_TALK_AFTER_FORGE);
            } else {
                Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
            }
        }
        break;
    }
}


/* The smith forging with its partner (townsperson 0x1A): strikes (0x40B),
 * stands (0x406) until the partner strikes (0x358) and finishes, then
 * 0x40E and the talk state 0x7F. The player's lock-on turns it as in
 * Npc_SmithForgeAlone. Nothing happens without the partner. */
void Npc_SmithForgeWithPartner(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Em *partner = Npc_Find(0x1A);
    u8 step;

    if (partner == NULL) {
        return;
    }
    step = npc->step;
    switch (step) {
    case 0:
        npc->step = step + 1;
        if (npc == gPlayers[gScene.localPlayer].target) {
            npcWork->turnStep = -npcWork->turnStep;
            npcWork->turnCount = 6;
        }
        if (npc->motionId[0] != 0x40B) {
            Actor_SetMotionOnLayer(npc, 0x40B, 0, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = step + 1;
            Actor_SetMotionOnLayer(npc, 0x406, 4, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0 && partner->motionId[0] == 0x358) {
            npc->step = step + 1;
        }
        break;
    case 3:
        if (partner->motion[0].playing <= 0) {
            npc->step = step + 1;
            if (npc == gPlayers[gScene.localPlayer].target) {
                Npc_TurnToPlayer(npc, 6);
            }
            Actor_SetMotionOnLayer(npc, 0x40E, 4, 0, 0);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, NPC_STATE_SMITH_TALK_AFTER_FORGE);
        }
        break;
    }
}

void Npc_SmithForgeAlone(Em *);
void Npc_SmithForgeWithPartner(Em *a);

/* State 0x69, the smith's forging: alone while the smithy
 * (VILLAGE_FACILITY_SMITHY) is at level 0 or the workshop helper
 * (NPC_WORKSHOP_HELPER) is present, else with its partner (townsperson
 * 0x1A). It can't be talked to meanwhile. */
void Npc_StateSmithForge(Em *npc)
{
    ((NpcWork *)&npc->work)->noTalk = 1;
    switch ((u8)Village_FacilityLevel(VILLAGE_FACILITY_SMITHY)) {
    case 0:
        Npc_SmithForgeAlone(npc);
        break;
    default:
        if (Npc_Find(NPC_WORKSHOP_HELPER) != 0) {
            Npc_SmithForgeAlone(npc);
        } else {
            Npc_SmithForgeWithPartner(npc);
        }
        break;
    }
}


/* State 0x76, the smith's partner (townsperson 0x1A) at work beside the
 * smith (NPC_WORKSHOP_SMITH): 0x357 three times (motionPhase 0 or 1 at
 * random), 0x358 and 0x356, again each time the smith stands (0x406);
 * 0x359 when the smith plays 0x408. When the smith forges (0x40B) without
 * the workshop helper it strikes along (state 0x78); during the smith's
 * talk (state 0x68 or 0x7F) it stands (0x356). */
void Npc_StatePartnerWork(Em *npc)
{
    Em *smith;
    NpcWork *w = (NpcWork *)&npc->work;

    smith = Npc_Find(NPC_WORKSHOP_SMITH);

    if (smith == NULL) {
        return;
    }
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 0x357) {
            Actor_SetMotion(npc, 0x357, 6, 0);
            npc->timer = 3;
            w->motionPhase = (u16)Rand_Next(1) & 1;
        }
        npc->step++;
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            if (--npc->timer <= 0) {
                Actor_SetMotion(npc, 0x358, 0, 0);
                npc->step++;
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x356, 6, 0);
            npc->step++;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0 && smith->motionId[0] == 0x406) {
            npc->step = 0;
        }
        break;
    case 4:
        Actor_SetMotion(npc, 0x359, 6, 0);
        npc->step++;
        break;
    case 5:
        if (npc->motion[0].playing <= 0 && smith->motionId[0] == 0x406) {
            npc->step = 0;
        }
        break;
    }
    if (smith->motionId[0] == 0x408 && npc->step <= 3) {
        npc->step = 4;
    }
    if (smith->motionId[0] == 0x40B && Npc_Find(NPC_WORKSHOP_HELPER) == 0) {
        Actor_SetState(npc, 0, NPC_STATE_PARTNER_STRIKE);
        return;
    }
    if ((smith->state == NPC_STATE_SMITH_TALK || smith->state == NPC_STATE_SMITH_TALK_AFTER_FORGE) && npc->motionId[0] != 0x356) {
        npc->step = 3;
        Actor_SetMotion(npc, 0x356, 0x14, 0);
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);

/* State 0x9D: a townsperson of model type 1 holds 0x40C; the others loop
 * 0x358 and 0x357 with random pauses (0-63 frames, 0-3 repeats), the
 * motion speed set back to 1 after frame 226 of 0x357. */
void Npc_State9D(Em *npc)
{
    switch (npc->step) {
    case 0:
        if (npc->type == 1) {
            Actor_SetMotionOnLayer(npc, 0x40C, 4, 0, 0);
            npc->step = 0x63;
        } else if (npc->motionId[0] != 0x358) {
            Actor_SetMotion(npc, 0x358, 0, 0);
            npc->step++;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->timer = (u16)Rand_Next(1) & 0x3F;
            npc->step++;
        }
        break;
    case 2:
        if (--npc->timer <= 0) {
            npc->step++;
            npc->timer = ((u16)Rand_Next(1) + 3) & 3;
            if (npc->motionId[0] != 0x357) {
                Actor_SetMotion(npc, 0x357, 6, 0);
            }
        }
        break;
    case 3:
        if (Motion_PassedFrame((Player *)npc, 0, 226.0f)) {
            npc->motionSpeed = 1.0f;
        }
        if (npc->motion[0].playing <= 0) {
            if (--npc->timer <= 0) {
                npc->step = 0;
            }
        }
        break;
    }
}

/* State 0x78, the smith's partner striking along with the smith's
 * forging: 0x357 three times, 0x358 and 0x356, then again while the smith
 * forges (0x40B), or back to its route point's state once the smith
 * stands (0x406). Nothing happens without the smith. */
void Npc_StatePartnerStrike(Em *npc)
{
    s32 smith;
    s32 timer;
    u16 motion;
    u8 step;
    NpcWork *npcWork;

    npcWork = (NpcWork *)&npc->work;
    smith = (s32)Npc_Find(NPC_WORKSHOP_SMITH);
    if (smith != 0) {
        step = npc->step;
        switch (step) {
        case 0:
            npc->step = (u8) (step + 1);
            if (npc->motionId[0] != 0x357) {
                Actor_SetMotion(npc, 0x357, 6, 0);
                npc->timer = 3;
            }
            break;
        case 1:
            if (npc->motion[0].playing <= 0) {
                timer = npc->timer - 1;
                npc->timer = timer;
                if (timer <= 0) {
                    Actor_SetMotion(npc, 0x358, 0, 0);
                    npc->step = (u8) (npc->step + 1);
                    break;
                }
            }
            break;
        case 2:
            if (npc->motion[0].playing <= 0) {
                Actor_SetMotion(npc, 0x356, 0, 0);
                npc->step = (u8) (npc->step + 1);
            }
            break;
        case 3:
            motion = ((Em *)smith)->motionId[0];
            if (motion == 0x40B) {
                Actor_SetState(npc, 0, 0x78U);
                break;
            }
            if ((npc->motion[0].playing <= 0) && (motion == 0x406)) {
                Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
            }
            break;
        }
    }
}

s32 Motion_ReachedFrame(void *, s32, f32);

/* The smith at its forge with its partner (townsperson 0x1A): faces the
 * forge (heading 0xED84) standing (0x406) and, after 60 frames and then
 * every 150, answers the partner: 0x408 as the partner's 0x357 nears its
 * end (motionPhase 1, frame 234), 0x407 while the partner stands (0x356,
 * not talking). Nothing happens without the partner. */
void Npc_SmithWorkWithPartner(Em *npc)
{
    Em *partner = Npc_Find(0x1A);
    NpcWork *partnerWork;

    if (partner == NULL) {
        return;
    }
    partnerWork = (NpcWork *)&partner->work;
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 0x406) {
            npc->targetYaw = npc->rotY = 0xED84;
            Actor_SetMotionOnLayer(npc, 0x406, 0, 0, 0);
        }
        npc->step++;
        npc->timer = 0x3C;
        break;
    case 1:
        if (--npc->timer <= 0) {
            if (partner->motionId[0] == 0x357) {
                if (partnerWork->motionPhase == 1) {
                    npc->step++;
                }
            } else if (partner->motionId[0] == 0x356 && partner->state != NPC_STATE_TALK && partner->step > 2) {
                Actor_SetMotionOnLayer(npc, 0x407, 0xA, 0, 0);
                npc->step = 3;
            }
        }
        break;
    case 2:
        if (partner->timer < 3 && Motion_ReachedFrame(partner, 0, 234.0f) != 0) {
            Actor_SetMotionOnLayer(npc, 0x408, 4, 0, 0);
            npc->step++;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0 || partner->state == NPC_STATE_TALK) {
            npc->step = 1;
            npc->timer = 0x96;
            Actor_SetMotionOnLayer(npc, 0x406, 4, 0, 0);
        }
        break;
    }
}

/* State 0x7B: loops motion 0x19F. */
void Npc_State7B(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 0x19F) {
            Actor_SetMotion(npc, 0x19F, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x19F, 0, 0);
        }
        break;
    }
}

/* State 0x7C: holds motion 0x273. */
void Npc_State7C(Em *npc)
{
    if (npc->motionId[0] != 0x273) {
        Actor_SetMotion(npc, 0x273, 6, 0);
    }
}

extern s32 gTownMode;

/* State 0xA1: holds 0x273 while a contest (TOWN_MODE_CONTEST,
 * TOWN_MODE_DRINK_CONTEST) ends with the player's win
 * (TOWN_PC_CONTEST_WIN, TOWN_PC_DRINK_WIN); then, once (stateVar0), it
 * turns back (Npc_StartTurnBack) and re-enters its route point
 * (Npc_EnterRoutePoint). It compiles as the game's code only with
 * Npc_StartTurnBack static. */
void Npc_StateContestWait(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npc->stateVar0 = 0;
        if (npc->motionId[0] != 0x273) {
            Actor_SetMotion(npc, 0x273, 6, 0);
        }
    case 1:
        switch (gTownMode) {
        case TOWN_MODE_CONTEST:
        case TOWN_MODE_DRINK_CONTEST:
            if (pl->stateGroup == 0) {
                switch (pl->state) {
                case TOWN_PC_CONTEST_WIN:
                case TOWN_PC_DRINK_WIN:
                    return;
                }
            }
            break;
        }
        break;
    }
    if (npc->stateVar0 == 0) {
        npc->stateVar0++;
        Npc_StartTurnBack(npc, 0xA);
        Npc_EnterRoutePoint(npc);
    }
}

/* States 0xA6 and 0xA7 (first 0, 1; 0xA7 first waits 160 frames): plays
 * 0x277, then 0x36C; once no contest runs (TOWN_MODE_CONTEST,
 * TOWN_MODE_DRINK_CONTEST) it turns back, once, and re-enters its route
 * point. */
void Npc_StateContestReact(Em *npc, s32 delayed)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        if (delayed == 0) {
            npc->step = 2;
            npc->stateVar0 = 0;
            if (npc->motionId[0] != 0x277) {
                Actor_SetMotion(npc, 0x277, 0xA, 0);
            }
        } else {
            npc->step = step + 1;
            npc->timer = 0xA0;
        }
        break;
    case 1:
        if (--npc->timer <= 0) {
            npc->step++;
            npc->stateVar0 = 0;
            if (npc->motionId[0] != 0x277) {
                Actor_SetMotion(npc, 0x277, 6, 0);
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing == 0) {
            npc->step = step + 1;
            Actor_SetMotion(npc, 0x36C, 0xA, 0);
        }
        break;
    case 3:
        switch (gTownMode) {
        case TOWN_MODE_CONTEST:
        case TOWN_MODE_DRINK_CONTEST:
            return;
        default:
            if (npc->stateVar0 == 0) {
                npc->stateVar0++;
                Npc_StartTurnBack(npc, 0xA);
                Npc_EnterRoutePoint(npc);
            }
            break;
        }
        break;
    }
}


extern NpcPos gSmithMeetSpotBelow;
f32 Vec_Distance(f32 *a, f32 *b);

/* State 0x6A, the smith (NPC_WORKSHOP_SMITH; Npc_SmithWorkAlone enters it):
 * stands (0x406, 0x407 at random) until the local player, below height
 * 200, comes within 450 of gSmithMeetSpotBelow; then it leaves (0x40F),
 * reappears on the ground at (10375, 0, 9055) facing heading 0x2AAB
 * (0x411) and goes back one route point. It can't be talked to while it
 * moves. */
void Npc_StateSmithMeetBelow(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcPos spot = gSmithMeetSpotBelow;

    w->motionPhase = 0;
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x406) {
            Actor_SetMotionOnLayer(npc, 0x406, 4, 0, 0);
            npc->timer = 1;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->timer = (u16)Rand_Next(1) & 3;
            if ((u16)Rand_Next(1) & 1) {
                Actor_SetMotionOnLayer(npc, 0x406, 6, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, 0x407, 0xA, 0, 0);
            }
        }
        if (pl->pos[1] < 200.0f && Vec_Distance((f32 *) pl->pos, spot.v) <= 450.0f) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x40F, 2, 0, 0);
        }
        break;
    case 2:
        w->noTalk = 1;
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            npc->timer = 10;
        }
        break;
    case 3:
        w->noTalk = 1;
        if (--npc->timer <= 0) {
            npc->step++;
            w->motionPhase = 2;
            npc->posX = 10375.0f;
            npc->posY = 0.0f;
            npc->posZ = 9055.0f;
            *(NpcPos *)npc->goal = *(NpcPos *)(&npc->posX);
            npc->targetYaw = npc->rotY = 0x2AAB;
            Actor_SetMotionOnLayer(npc, 0x411, 0, 0, 0);
        }
        break;
    case 4:
        w->motionPhase = 2;
        w->noTalk = 1;
        if (npc->motion[0].playing <= 0 || !(pl->pos[1] <= 240.0f)) {
            (w->routeIdx)--;
            Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

extern NpcPos gSmithMeetSpotAbove;
f32 Vec_Distance(f32 *a, f32 *b);

/* State 0x6B, the smith (Npc_SmithWorkAlone enters it): waits (0x409); when
 * condition 0x2F holds (the condition the smith's workshop talk needs) and
 * the local player, above height 240, comes within 310 of
 * gSmithMeetSpotAbove, it reappears up at (9979, 248, 8661) facing heading
 * 0xED84 (0x410) and goes on one route point. */
void Npc_StateSmithMeetAbove(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcPos spot = gSmithMeetSpotAbove;

    w->motionPhase = 2;
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x409) {
            npc->targetYaw = npc->rotY = 0x2AAB;
            Actor_SetMotionOnLayer(npc, 0x409, 0x1E, 0, 0);
            npc->timer = (u16)Rand_Next(1) % 120 + 120;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, 0x409, 0x1E, 0, 0);
            npc->timer = (u16)Rand_Next(1) % 120 + 120;
        }
        if (Cond_Check(0x2F) == 1 && !(pl->pos[1] <= 240.0f) && Vec_Distance((f32 *) pl->pos, spot.v) <= 310.0f) {
            npc->step++;
            npc->posX = 9979.0f;
            npc->posY = 248.0f;
            npc->posZ = 8661.0f;
            *(NpcPos *)npc->goal = *(NpcPos *)(&npc->posX);
            npc->targetYaw = npc->rotY = 0xED84;
            npc->timer = 1;
            Actor_SetMotionOnLayer(npc, 0x410, 0, 0, 0);
            w->motionPhase = 0;
        }
        break;
    case 2:
        w->noTalk = 1;
        w->motionPhase = 0;
        if (npc->motion[0].playing <= 0) {
            (w->routeIdx)++;
            Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}


/* The smith at its forge alone: stands (0x406, 0x407 at random); on the
 * ground (below 90) it goes to state 0x6A when condition 0x2F holds and
 * the local player is up (100 or higher), and up high it goes to state
 * 0x6B when the player is below 100. */
void Npc_SmithWorkAlone(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        Actor_SetPosture(npc, PL_POSTURE_STANDING, 0, 0);
        npc->timer = (u16)Rand_Next(1) & 3;
        Actor_SetMotionOnLayer(npc, 0x406, 0, 0, 0);
        break;
    case 1:
    case 2:
        if (npc->posY < 90.0f) {
            if (!(pl->pos[1] < 100.0f) && Cond_Check(0x2F) == 1) {
                Actor_SetState(npc, 0, NPC_STATE_SMITH_MEET_BELOW);
                break;
            }
        } else if (pl->pos[1] < 100.0f) {
            Actor_SetState(npc, 0, NPC_STATE_SMITH_MEET_ABOVE);
            break;
        }
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->timer = (u16)Rand_Next(1) & 3;
            if ((u16)Rand_Next(1) & 1) {
                Actor_SetMotionOnLayer(npc, 0x406, 4, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, 0x407, 0xA, 0, 0);
            }
        }
        break;
    }
}

/* The two spots the smith's moves watch (Npc_StateSmithMeetBelow, Npc_StateSmithMeetAbove). */
NpcPos gSmithMeetSpotBelow = {
#include "lobby/007FE490.inc"
};
NpcPos gSmithMeetSpotAbove = {
#include "lobby/007FE4A0.inc"
};

/* State 0x77, the smith at its forge: alone while the smithy
 * (VILLAGE_FACILITY_SMITHY) is at level 0, else with its partner
 * (townsperson 0x1A). */

void Npc_StateSmithWork(Em *npc) {
    if (!(Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) & 0xFF)) {
        Npc_SmithWorkAlone(npc);
        return;
    }
    Npc_SmithWorkWithPartner(npc);
}


void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
s32 Motion_ReachedFrame(void *, s32, f32);
void ActorParticles_SpawnSpray(void *arg0);

void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);

f32 Vec_Distance(f32 *a, f32 *b);

/*
 * State 0x6D: townspeople 0x31 and 0x32, who meet each other. Each finds the
 * other; once the other is within 200, both wait a random 0-15 frames and play
 * their meeting motion (0x2B8 for 0x31, 0x290 for 0x32), then stay.
 */
void Npc_StatePairMeet(Em *npc)
{
    NpcWork *w;
    Em *partner;

    w = (NpcWork *)&npc->work;
    Npc_Find(0x32);
    if (w->id == 0x31) {
        partner = Npc_Find(0x32);
    } else {
        partner = Npc_Find(0x31);
    }
    switch (npc->step) {
    case 0:
        npc->step++;
        if (w->id == 0x31) {
            Actor_SetMotion(npc, 0x2A3, 0x10, 0);
        } else {
            Actor_SetMotion(npc, 1, 0x10, 0);
        }
        break;
    case 1:
        if (partner != NULL && Vec_Distance(&npc->posX, &partner->posX) < 200.0f) {
            npc->step++;
            npc->timer = (u16)Rand_Next(1) & 0xF;
        }
        break;
    case 2:
        if (--npc->timer <= 0) {
            npc->step++;
            if (w->id == 0x31) {
                Actor_SetMotion(npc, 0x2B8, 0xA, 0);
            } else {
                Actor_SetMotion(npc, 0x290, 0xA, 0);
            }
        }
        break;
    case 3:
        break;
    }
}

/*
 * State 0x6E: the contest spot sets it on NPC_CONTEST_HOST as an offline
 * arm-wrestling contest opens. It sits down at the table (motion 0x373), then
 * keeps replaying that motion's end from frame 212.
 */
void Npc_StateContestSitDown(Em *npc)
{
    u8 phase;

    phase = npc->stateVar0;
    switch (phase) {
    case 0:
        npc->stateVar0 = (u8) (phase + 1);
        Actor_SetMotion(npc, 0x373, 6, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 0x373, 0, 0xD4);
        }
        break;
    }
}

/* State 0x70 (as the player's TOWN_PC_CONTEST_SEAT_A): the contest seat's pose (motion 0x375). */
void Npc_StateContestSeat(Em *npc)
{
    if (npc->motionId[0] != 0x375) {
        Actor_SetMotion(npc, 0x375, 6, 0);
    }
}

/*
 * State 0x6F (as the player's TOWN_PC_CONTEST_GRIP): the arm-wrestling grip
 * (motion 0x376); the contest sets it on both sides as the match starts. Sweat
 * spray when the motion starts.
 */
void Npc_StateContestGrip(Em *npc)
{
    if (npc->motionId[0] != 0x376) {
        Actor_SetMotion(npc, 0x376, 4, 0);
        ActorParticles_SpawnSpray(npc);
    }
}

/* State 0x72: the contest has closed; gets up from the table (motion 0x374), then stands (state 1). */
void Npc_StateContestLeave(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 0x374) {
            Actor_SetMotion(npc, 0x374, 6, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = (u8) (step + 1);
            Actor_SetState(npc, 0, 1U);
        }
        break;
    }
}

/*
 * State 0x73: the offline contest's end. NPC_CONTEST_HOST can't be talked to
 * and plays motion 0x378. When the motion ends, NPC_CONTEST_HOST and
 * NPC_DRINK_CONTEST_HOST go back to their route point's state.
 */
void Npc_StateContestEnd(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (npc->work == NPC_CONTEST_HOST) {
        w->noTalk = 1;
    }
    switch (npc->step) {
    case 0:
        npc->step++;
        if (w->id == NPC_CONTEST_HOST && npc->motionId[0] != 0x378) {
            Actor_SetMotion(npc, 0x378, 6, 0x31C);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            if (w->id == NPC_CONTEST_HOST || w->id == NPC_DRINK_CONTEST_HOST) {
                Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
            }
        }
        break;
    case 2:
        break;
    }
}

/*
 * State 0x74 (as the player's TOWN_PC_CONTEST_LOSE): the contest's host has
 * lost. NPC_CONTEST_HOST holds motion 0x378 and can't be talked to;
 * NPC_DRINK_CONTEST_HOST plays 0x360.
 */
void Npc_StateContestLose(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (npc->work == NPC_CONTEST_HOST) {
        if (npc->motionId[0] != 0x378) {
            Actor_SetMotion(npc, 0x378, 0, 0);
        }
        w->noTalk = 1;
    } else if (npc->work == NPC_DRINK_CONTEST_HOST) {
        if (npc->motionId[0] != 0x360) {
            Actor_SetMotion(npc, 0x360, 0x14, 0);
        }
    }
}

/*
 * State 0x75 (as the player's TOWN_PC_CONTEST_WIN): the contest's host has
 * won. NPC_CONTEST_HOST plays motion 0x377; NPC_DRINK_CONTEST_HOST plays 0x35F.
 */
void Npc_StateContestWin(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        if (w->id == NPC_CONTEST_HOST) {
            if (npc->motionId[0] != 0x377) {
                Actor_SetMotion(npc, 0x377, 4, 0);
            }
        } else if (w->id == NPC_DRINK_CONTEST_HOST) {
            if (npc->motionId[0] != 0x35F) {
                Actor_SetMotion(npc, 0x35F, 0x14, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

/* State 0x7D: plays motion 0x36B once (20-frame blend) and keeps it. Case 0 runs on into case 1, which does nothing. */
void Npc_State7D(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 0x36B) {
            Actor_SetMotion(npc, 0x36B, 0x14, 0);
        }
        
    case 1:
        break;
    }
}

f32 Math_Absf(f32 x);
void Npc_TurnToPlayer(Em *arg0, s32 arg1);
void Npc_HoldRoute(Em *arg0);
s32 Actor_HeadingDiff(void *obj, void *pos);
s32 Game_IsOnline(void);
extern s32 gTownMode;

/*
 * State 0x7E: the contests' onlooker (townspeople of type 0x37). It plays
 * motion 0x36C. Offline, with the local player within 400 and while it is not
 * turning:
 * - during TOWN_MODE_CONTEST it turns to the player once, and during
 *   TOWN_MODE_DRINK_CONTEST toward the tavern table's point (9216, 126, 10984);
 *   it then stays at its route point;
 * - it reacts to the player's result: a win (TOWN_PC_CONTEST_WIN,
 *   TOWN_PC_DRINK_WIN; in the drinking contest it turns to the player too)
 *   gives state 0xA1, an arm-wrestling loss 0xA6, a drinking loss 0xA7.
 * In other modes, or once the player is away, it turns back and goes on along
 * its route.
 */
void Npc_StateWatchContest(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Player *pl;
    f32 tablePos[3];

    switch (npc->step) {
    case 0:
        npc->step++;
        npc->stateVar0 = 0;
        if (npc->motionId[0] != 0x36C) {
            Actor_SetMotion(npc, 0x36C, 0x14, 0);
        }
        break;
    case 1:
        if (npc->variant != 0x37 || (u16)w->turnCount != 0) {
            break;
        }
        pl = &gPlayers[gScene.localPlayer];
        if (Vec_Distance((f32 *) pl->pos, &npc->posX) <= 400.0f && Game_IsOnline() == 0) {
            switch (gTownMode) {
            case TOWN_MODE_CONTEST:
            case TOWN_MODE_DRINK_CONTEST:
                if (npc->stateVar0 == 0) {
                    npc->stateVar0++;
                    if (gTownMode == TOWN_MODE_CONTEST) {
                        Npc_TurnToPlayer(npc, 10);
                    } else {
                        tablePos[0] = 9216.0f;
                        tablePos[1] = 126.0f;
                        tablePos[2] = 10984.0f;
                        w->turnDelta = Actor_HeadingDiff(npc, tablePos);
                        if (!(Math_Absf(w->turnDelta) <= 10.0f)) {
                            w->homeYaw = npc->rotY;
                        }
                        w->turnStep = w->turnDelta / 10;
                        w->turnCount = 10;
                    }
                    Npc_HoldRoute(npc);
                }
                if (pl->stateGroup == TOWN_PC_GROUP_ACTION) {
                    switch (pl->state) {
                    case TOWN_PC_CONTEST_WIN:
                    case TOWN_PC_DRINK_WIN:
                        if (gTownMode == TOWN_MODE_DRINK_CONTEST) {
                            w->turnDelta = Actor_HeadingDiff(npc, pl->pos);
                            w->turnStep = w->turnDelta / 10;
                            w->turnCount = 10;
                        }
                        Actor_SetState(npc, 0, NPC_STATE_CONTEST_WAIT);
                        break;
                    case TOWN_PC_CONTEST_LOSE:
                        Actor_SetState(npc, 0, NPC_STATE_CONTEST_REACT);
                        break;
                    case TOWN_PC_DRINK_LOSE:
                        Actor_SetState(npc, 0, NPC_STATE_CONTEST_REACT_LATE);
                        break;
                    }
                }
                break;
            default:
                if (npc->stateVar0 != 0) {
                    npc->stateVar0 = 0;
                    Npc_StartTurnBack(npc, 10);
                    Npc_EnterRoutePoint(npc);
                }
                break;
            }
        } else if (npc->stateVar0 != 0) {
            npc->stateVar0 = 0;
            Npc_StartTurnBack(npc, 10);
            Npc_EnterRoutePoint(npc);
        }
        break;
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
void Npc_StartTurnTo(Em *arg0, s32 arg1, s32 arg2);

/*
 * State 0x81, a route point: turns over 10 frames to the angle in the entry's
 * argument, then goes on to the next point. Can't be talked to meanwhile.
 */
void Npc_StateRouteTurn(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *entry = &w->route[w->routeIdx];

    w->noTalk = 1;
    Npc_StartTurnTo(npc, 10, entry->arg);
    Npc_NextRoutePoint(npc);
}

s32 Actor_HeadingDiff(void *obj, void *pos);

/*
 * State 0x82: walks to the current route point. On entry it stands and picks
 * walk motion 2 or 3 at random, with a longer blend from motion 0x36B. Each
 * frame it turns a tenth of the way toward the point (at its own height), and
 * within 40 of the point it goes on to the next one.
 */
void Npc_StateRouteWalk(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcPos target;
    f32 dist;

    switch (npc->step) {
    case 0:
        Actor_SetPosture(npc, 0, 0, 0);
        npc->step++;
        if ((u16)Rand_Next(1) & 1) {
            if (npc->motionId[0] != 3) {
                if (npc->motionId[0] == 0x36B) {
                    Actor_SetMotion(npc, 3, 0xC, 0);
                } else {
                    Actor_SetMotion(npc, 3, 4, 0);
                }
            }
        } else {
            if (npc->motionId[0] != 2) {
                if (npc->motionId[0] == 0x36B) {
                    Actor_SetMotion(npc, 2, 0xC, 0);
                } else {
                    Actor_SetMotion(npc, 2, 4, 0);
                }
            }
        }
        break;
    case 1:
        target = w->route[w->routeIdx].pos;
        target.v[1] = npc->posY;
        npc->targetYaw = (s16)(u16)Actor_HeadingDiff(npc, &target) / 10;
        npc->rotY += (s16)npc->targetYaw;
        dist = Vec_Distance(&npc->posX, (f32 *) &target);
        if (dist < 0.0f) {
            dist *= -1.0f;
        }
        if (dist <= 40.0f) {
            Npc_NextRoutePoint(npc);
        }
        break;
    }
}

/* State 0x8A: plays motion 0x35A two to five times, then 0x362 once, and over again. */
void Npc_State8A(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x35A) {
            Actor_SetMotion(npc, 0x35A, 6, 0);
            npc->timer = ((u16)Rand_Next(1) & 3) + 2;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            if (--npc->timer <= 0) {
                if (npc->motionId[0] != 0x362) {
                    Actor_SetMotion(npc, 0x362, 0, 0);
                    npc->timer = 1;
                } else {
                    Actor_SetMotion(npc, 0x35A, 6, 0);
                    npc->timer = ((u16)Rand_Next(1) & 3) + 2;
                }
            }
        }
        break;
    }
}

/* State 0x8C: plays motion 0x35C once (20-frame blend) and keeps it. */
void Npc_State8C(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        if (npc->motionId[0] != 0x35C) {
            Actor_SetMotion(npc, 0x35C, 0x14, 0);
        }
        npc->step = (u8) (npc->step + 1);
        
    case 1:
        break;
    }
}

s32 Actor_HeadingDiff(void *obj, void *pos);

/*
 * State 0x8E: plays motion 0x36A. At its frame 30 the townsperson turns toward
 * the local player over 10 frames and can't be talked to meanwhile. When the
 * motion ends it starts its talk (state 0x2C).
 */
void Npc_StateCallOver(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 0x36A) {
            Actor_SetMotion(npc, 0x36A, 4, 0);
        }
        npc->step++;
        break;
    case 1:
        if (Motion_ReachedFrame(npc, 0, 30.0f) != 0) {
            npc->timer = 10;
            npc->targetYaw = Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos);
            npc->step++;
        }
        break;
    case 2:
        w->noTalk = 1;
        npc->rotY += (s16)npc->targetYaw / 10;
        if (--npc->timer <= 0) {
            npc->step++;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, NPC_STATE_TALK);
        }
        break;
    }
}


/*
 * State 0x8F: reacts to the local player coming near. It does not turn when
 * talked to, and plays motion 0x364. Within 300 it plays 0x365; when
 * the player leaves it plays 0x366, then starts over. Townsperson 0x2F does not
 * react while Cond_Check(100) holds.
 */
void Npc_StateReactNear(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    f32 dist;

    w->keepFacing = 1;
    dist = Vec_Distance(&npc->posX, (f32 *) gPlayers[gScene.localPlayer].pos);
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 0x364) {
            Actor_SetMotion(npc, 0x364, 4, 0);
        }
        npc->step++;
        break;
    case 1:
        if (Cond_Check(100) == 1 && w->id == 0x2F) {
            break;
        }
        if (dist < 300.0f) {
            Actor_SetMotion(npc, 0x365, 0, 0);
            npc->step++;
        }
        break;
    case 2:
        if (!(dist < 300.0f)) {
            Actor_SetMotion(npc, 0x366, 2, 0);
            npc->step++;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            npc->step = 0;
        }
        break;
    }
}

/*
 * State 0x92: plays motion 0x421 three to six times, then 0x422, and over
 * again. The blend is 30 frames when it comes from 0x3F0, 0x423 or 0x3E9.
 */
void Npc_State92(Em *npc)
{
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 0x421) {
            if (npc->motionId[0] == 0x3F0 || npc->motionId[0] == 0x423 || npc->motionId[0] == 0x3E9) {
                Actor_SetMotionOnLayer(npc, 0x421, 0x1E, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, 0x421, 4, 0, 0);
            }
        }
        npc->timer = ((u16)Rand_Next(1) & 3) + 3;
        npc->step++;
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            Actor_SetMotionOnLayer(npc, 0x422, 6, 0, 0);
            npc->step++;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->step = 1;
            Actor_SetMotionOnLayer(npc, 0x421, 0x1E, 0, 0);
            npc->timer = ((u16)Rand_Next(1) & 3) + 3;
        }
        break;
    }
}

extern s32 gTownMode;

/* State 0x93: plays motion 0x423, then holds 0x3F0 until the town is back to TOWN_MODE_FREE, then its route point's state. */
void Npc_StateWaitTownFree(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 0x423) {
            Actor_SetMotionOnLayer(npc, 0x423, 6, 0, 0);
            npc->step++;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, 0x3F0, 4, 0, 0);
            npc->step++;
        }
        break;
    case 2:
        if (gTownMode == TOWN_MODE_FREE) {
            Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);

/* States 0x94 (flag 0) and 0x95 (flag 1): holds motion 0x427 or 0x428. */
void Npc_State94(Em *npc, u8 second)
{
    switch (npc->step) {
    case 0:
        if (second == 0) {
            Actor_SetMotionOnLayer(npc, 0x427, 0, 0, 0);
        } else {
            Actor_SetMotionOnLayer(npc, 0x428, 0, 0, 0);
        }
        npc->step++;
        break;
    }
}

s32 ElderRaid_InState1(void);
s32 LobbyTime_IsPm(void);

/*
 * State 0x96: an idle by the time of day. While ElderRaid_InState1 holds it
 * plays motion 0x424. In the afternoon (LobbyTime_IsPm) it plays 0x425 three
 * times, then 0x426, and over again. Otherwise it loops 0x3E9. Coming from
 * 0x3F0 or 0x3EF, it first plays 0x3E9 with a 30-frame blend.
 */
void Npc_StateIdleByTime(Em *npc)
{
    s32 isPm = LobbyTime_IsPm();

    switch (npc->step) {
    case 0:
        npc->timer = 1;
        npc->step++;
        if (ElderRaid_InState1() == 1) {
            if (npc->motionId[0] != 0x424) {
                Actor_SetMotionOnLayer(npc, 0x424, 0x1E, 0, 0);
            }
        } else if (isPm == 1) {
            if (npc->motionId[0] == 0x3F0 || npc->motionId[0] == 0x3EF) {
                Actor_SetMotionOnLayer(npc, 0x3E9, 0x1E, 0, 0);
                npc->timer = 3;
            } else if (npc->motionId[0] != 0x425) {
                Actor_SetMotionOnLayer(npc, 0x425, 0x1E, 0, 0);
                npc->timer = 3;
            }
        } else {
            if (npc->motionId[0] == 0x3F0 || npc->motionId[0] == 0x3EF) {
                Actor_SetMotionOnLayer(npc, 0x3E9, 0x1E, 0, 0);
            } else if (npc->motionId[0] != 0x3E9) {
                Actor_SetMotionOnLayer(npc, 0x3E9, 4, 0, 0);
            }
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            if (npc->motionId[0] == 0x425) {
                Actor_SetMotionOnLayer(npc, 0x426, 6, 0, 0);
                npc->step++;
            } else {
                npc->step = 0;
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, 0x3E9, 0x1E, 0, 0);
            npc->timer = 2;
            npc->step = 1;
        }
        break;
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
void Npc_StartTurnTo(Em *arg0, s32 arg1, s32 arg2);

/* State 0x9F: plays motion 0x424 (30-frame blend) and keeps it. */
void Npc_State9F(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x424) {
            Actor_SetMotionOnLayer(npc, 0x424, 0x1E, 0, 0);
        }
        break;
    }
}

/* State 0x15 (as the player's TOWN_PC_STAND_UP): gets up from the seat (motion 0x262); can't be talked to. */
void Npc_StateStandUp(Em *npc)
{
    u8 step;

    ((NpcWork *)&npc->work)->noTalk = 1;
    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 0x262, 4, 0);
        break;
    }
}

/* State 0xA3: plays motion 0x383 once, then its route point's state. */
void Npc_StateA3(Em *npc)
{
    u8 step;
    NpcWork *w;

    step = npc->step;
    w = (NpcWork *)&npc->work;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 0x383) {
            Actor_SetMotion(npc, 0x383, 0xA, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

/* State 0xA4: plays motion 0xFF once, then its route point's state. */
void Npc_StateForge(Em *npc)
{
    u8 step;
    NpcWork *w;

    step = npc->step;
    w = (NpcWork *)&npc->work;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 0xFF) {
            Actor_SetMotion(npc, 0xFF, 4, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

/* State 0xA5: plays motion 0x100, then loops 0x357 at 1.8 times the speed. */
void Npc_StateHammer(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        npc->stateVar0 = 0;
        if (npc->motionId[0] != 0x100) {
            Actor_SetMotion(npc, 0x100, 0xC, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotion(npc, 0x357, 0, 0xB8);
            npc->motionSpeed = 1.8f;
        }
        break;
    case 2:
        break;
    }
}

void Player_SetFlag(void *obj, u32 flag);

/*
 * State 0xAD: plays motion 0x3FE (from frame 0xB0) with motionPhase 1, then its
 * route point's state. Each frame it sets flag 0x4000000, so the motion's root
 * movement is skipped for that frame (Player_MoveLocal clears the flag).
 */
void Npc_StateAD(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    Player_SetFlag((Player *) npc, 0x04000000U);
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x3FE) {
            Actor_SetMotionOnLayer(npc, 0x3FE, -2, 0xB0, 0);
        }
        w->motionPhase = 1;
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

/*
 * State 0xAE: plays motion 0x40B, 0x40C twice, 0x40D, then keeps 0x406.
 * motionPhase marks the part where the motion's effect runs: 2 on entry, 1 from
 * frame 30 of 0x40C, back to 0 at frame 118 of 0x40D.
 */
void Npc_StateAE(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        npc->stateVar0 = 0;
        w->motionPhase = 2;
        if (npc->motionId[0] != 0x40B) {
            Actor_SetMotionOnLayer(npc, 0x40B, 4, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x40C, 4, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->stateVar0++;
            if (npc->stateVar0 >= 2) {
                npc->step++;
                Actor_SetMotionOnLayer(npc, 0x40D, 4, 0, 0);
            }
        } else if (w->motionPhase == 0 && Motion_ReachedFrame(npc, 0, 30.0f) != 0) {
            w->motionPhase = 1;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x406, 4, 0, 0);
        } else if (w->motionPhase == 1 && Motion_ReachedFrame(npc, 0, 118.0f) != 0) {
            w->motionPhase = 0;
        }
        break;
    case 4:
        break;
    }
}

/*
 * State 0x29 (as the player's TOWN_PC_MEAL_SERVED; the gallery's meal scene sets
 * it on both): plays motion 0x27B. Case 0 advances the step twice, so it stops
 * in case 2. Steps 1 and 3 (seated at the table, PL_MOT_TOWN_SEATED, and 0x27B
 * again from frame 267) are not reached from here.
 */
void Npc_StateMealServed(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 0x27B, 6, 0);
        npc->step = (u8) (npc->step + 1);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = (u8) (step + 1);
            Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 6, 0);
            break;
        }
    case 2:
        break;
    case 3:
        if (Motion_ReachedFrame(npc, 0, 267.0f) != 0) {
            Actor_SetMotion(npc, 0x27B, 6, 0xC8);
        }
        break;
    }
}

/*
 * State 0x18 (as the player's TOWN_PC_MEAL_RISE_GOOD; the gallery's meal scene
 * sets it on both): stands, gets up (motion 0x262), then plays the pleased pose
 * 0x197 with an effect at bone 10, then stands (state 1).
 */
void Npc_StateMealRiseGood(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        npc->motionTime = 0;
        Actor_SetPosture(npc, 0, 0, 0);
        Actor_SetMotion(npc, 0x262, 4, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = (u8) (step + 1);
            Actor_SetMotion(npc, 0x197, 6, 0x14);
            Effect06_SpawnAtBone((void *) npc, 0, 8, 0xAU, 4.0f);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, 1U);
        }
        break;
    }
}

/* State 0x83, a route point: switches to the route in the entry's argument and enters its first point's state. */
void Npc_StateRouteJump(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    w->route = *(NpcRouteEnt **)&w->route[w->routeIdx].arg;
    w->routeIdx = 0;
    Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
}

/*
 * State 0xB3, a route point: as state 0x83, but one time in ten it takes the
 * next entry's route instead.
 */
void Npc_StateRouteJumpRandom(Em *npc)
{
    NpcWork *w;

    w = (NpcWork *)&npc->work;
    if (((s32) (Rand_Next(1) & 0xFFFF) % 10) == 0) {
        w->routeIdx = w->routeIdx + 1;
    }
    w->route = *(NpcRouteEnt **)&w->route[w->routeIdx].arg;
    w->routeIdx = 0;
    Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
}

void Npc_StateSeatPoseA(Em *arg0, s8 arg1);
void Npc_StateSeatPoseB(Em *arg0, s8 arg1);
void Npc_State46(Em *arg0, s8 arg1);
void Npc_State3A(Em *arg0, s8 arg1);
void Npc_State48(Em *arg0, s32 arg1);
void Npc_State4A(Em *arg0);
void Npc_StateTalkDelay(Em *arg0);
void Npc_StateRouteWait(Em *arg0);
void Npc_StateRandomizeWait(Em *arg0);
void Npc_State85(Em *arg0);
void Npc_State86(Em *arg0);
void Npc_State87(Em *arg0);
void Npc_State88(Em *arg0);
void Npc_State89(Em *arg0);
void Npc_State4B(Em *arg0);
void Npc_State4C(Em *arg0, s8 arg1);
void Npc_StateBusyTalkHold(Em *arg0);
void Npc_StateBusyPose(Em *arg0);
void Npc_StateBusyLoop(Em *arg0);
void Npc_State40(Em *arg0);
void Npc_State2E(Em *arg0);
void Npc_State2F(Em *arg0);
void Npc_StateGreetTalk(Em *npc);
void Npc_StatePairWait(Em *arg0);
void Npc_StateLeave(Em *arg0);
void Npc_StateSmithForge(Em *obj);
void Npc_StatePartnerStrike(Em *arg0);
void Npc_State7B(Em *arg0);
void Npc_State7C(Em *arg0);
void Npc_StateSmithWork(Em *npc);

/* the NPCs' state handlers, by state (+0x15); 0 for state 0x5F */
s32 Npc_RunState(Em *npc)
{
    switch (npc->state) {
    case NPC_STATE_IDLE:
        Npc_StateIdle(npc);
        break;
    case NPC_STATE_WALK:
        Npc_StateWalk(npc);
        break;
    case NPC_STATE_RUN:
        Npc_StateRun(npc);
        break;
    case NPC_STATE_SEAT_POSE_A:
        Npc_StateSeatPoseA(npc, 0);
        break;
    case NPC_STATE_SEAT_POSE_A_1:
        Npc_StateSeatPoseA(npc, 1);
        break;
    case NPC_STATE_PAIR_STAND:
        Npc_StatePairStand(npc, 0);
        break;
    case NPC_STATE_PAIR_STAND_1:
        Npc_StatePairStand(npc, 1);
        break;
    case NPC_STATE_PAIR_STAND_2:
        Npc_StatePairStand(npc, 2);
        break;
    case NPC_STATE_SEAT_POSE_B:
        Npc_StateSeatPoseB(npc, 0);
        break;
    case NPC_STATE_SEAT_POSE_B_1:
        Npc_StateSeatPoseB(npc, 1);
        break;
    case NPC_STATE_SEAT_POSE_B_3:
        Npc_StateSeatPoseB(npc, 3);
        break;
    case NPC_STATE_TALK:
        Npc_StateTalk(npc);
        break;
    case 0x46:
        Npc_State46(npc, 0);
        break;
    case 0x47:
        Npc_State46(npc, 1);
        break;
    case 0x3A:
        Npc_State3A(npc, 0);
        break;
    case 0x3B:
        Npc_State3A(npc, 1);
        break;
    case NPC_STATE_SEATED_AT_TABLE:
        Npc_StateSeatedAtTable(npc);
        break;
    case 0x48:
        Npc_State48(npc, 0);
        break;
    case 0x49:
        Npc_State48(npc, 1);
        break;
    case NPC_STATE_SIT_DOWN:
        Npc_StateSitDown(npc, 0);
        break;
    case NPC_STATE_SEATED:
        Npc_StateSitDown(npc, 1);
        break;
    case NPC_STATE_MEAL_SERVED:
        Npc_StateMealServed(npc);
        break;
    case NPC_STATE_MEAL_RISE_GOOD:
        Npc_StateMealRiseGood(npc);
        break;
    case 0x4A:
        Npc_State4A(npc);
        break;
    case NPC_STATE_TALK_DELAY:
        Npc_StateTalkDelay(npc);
        break;
    case 0x4B:
        Npc_State4B(npc);
        break;
    case 0x4C:
        Npc_State4C(npc, 0);
        break;
    case 0x4D:
        Npc_State4C(npc, 1);
        break;
    case NPC_STATE_WARP_TO_ROUTE:
        Npc_StateWarpToRoute(npc);
        break;
    case NPC_STATE_ROUTE_WAIT:
        Npc_StateRouteWait(npc);
        break;
    case NPC_STATE_BUSY_IDLE:
        Npc_StateBusyIdle(npc);
        break;
    case NPC_STATE_BUSY_TALK:
        Npc_StateBusyTalk(npc);
        break;
    case NPC_STATE_BUSY_POSE:
        Npc_StateBusyPose(npc);
        break;
    case NPC_STATE_BUSY_POSE_TALK:
        Npc_StateBusyPoseTalk(npc);
        break;
    case NPC_STATE_BUSY_LOOP:
        Npc_StateBusyLoop(npc);
        break;
    case NPC_STATE_BUSY_LOOP_TALK:
        Npc_StateBusyLoopTalk(npc);
        break;
    case 0x3F:
        Npc_State3F(npc);
        break;
    case 0x40:
        Npc_State40(npc);
        break;
    case 0x2E:
        Npc_State2E(npc);
        break;
    case 0x2F:
        Npc_State2F(npc);
        break;
    case NPC_STATE_GREET:
        Npc_StateGreet(npc);
        break;
    case NPC_STATE_GREET_HOLD:
        Npc_StateGreetHold(npc);
        break;
    case NPC_STATE_GREET_TALK:
        Npc_StateGreetTalk(npc);
        break;
    case NPC_STATE_PAIR_WAIT:
        Npc_StatePairWait(npc);
        break;
    case NPC_STATE_STEP_AHEAD:
        Npc_StateStepAhead(npc);
        break;
    case NPC_STATE_STEP_BACK:
        Npc_StateStepBack(npc);
        break;
    case NPC_STATE_SMITH_TALK:
        Npc_StateSmithTalk(npc, 0);
        break;
    case NPC_STATE_SMITH_TALK_AFTER_FORGE:
        Npc_StateSmithTalk(npc, 1);
        break;
    case NPC_STATE_SMITH_FORGE:
        Npc_StateSmithForge(npc);
        break;
    case NPC_STATE_SMITH_MEET_BELOW:
        Npc_StateSmithMeetBelow(npc);
        break;
    case NPC_STATE_SMITH_MEET_ABOVE:
        Npc_StateSmithMeetAbove(npc);
        break;
    case NPC_STATE_PAIR_MEET:
        Npc_StatePairMeet(npc);
        break;
    case NPC_STATE_CONTEST_SIT_DOWN:
        Npc_StateContestSitDown(npc);
        break;
    case NPC_STATE_CONTEST_SEAT:
        Npc_StateContestSeat(npc);
        break;
    case NPC_STATE_CONTEST_GRIP:
        Npc_StateContestGrip(npc);
        break;
    case NPC_STATE_CONTEST_LEAVE:
        Npc_StateContestLeave(npc);
        break;
    case NPC_STATE_CONTEST_END:
        Npc_StateContestEnd(npc);
        break;
    case NPC_STATE_CONTEST_LOSE:
        Npc_StateContestLose(npc);
        break;
    case NPC_STATE_CONTEST_WIN:
        Npc_StateContestWin(npc);
        break;
    case NPC_STATE_PARTNER_WORK:
        Npc_StatePartnerWork(npc);
        break;
    case NPC_STATE_SMITH_WORK:
        Npc_StateSmithWork(npc);
        break;
    case NPC_STATE_PARTNER_STRIKE:
        Npc_StatePartnerStrike(npc);
        break;
    case 0x7B:
        Npc_State7B(npc);
        break;
    case 0x7C:
        Npc_State7C(npc);
        break;
    case 0x7D:
        Npc_State7D(npc);
        break;
    case NPC_STATE_WATCH_CONTEST:
        Npc_StateWatchContest(npc);
        break;
    case NPC_STATE_ROUTE_TURN:
        Npc_StateRouteTurn(npc);
        break;
    case NPC_STATE_ROUTE_WALK:
        Npc_StateRouteWalk(npc);
        break;
    case NPC_STATE_ROUTE_JUMP:
        Npc_StateRouteJump(npc);
        break;
    case NPC_STATE_RANDOMIZE_WAIT:
        Npc_StateRandomizeWait(npc);
        break;
    case 0x85:
        Npc_State85(npc);
        break;
    case 0x86:
        Npc_State86(npc);
        break;
    case 0x87:
        Npc_State87(npc);
        break;
    case 0x88:
        Npc_State88(npc);
        break;
    case 0x89:
        Npc_State89(npc);
        break;
    case 0x8A:
        Npc_State8A(npc);
        break;
    case NPC_STATE_PAIR_STAND_3:
        Npc_StatePairStand(npc, 3);
        break;
    case 0x8C:
        Npc_State8C(npc);
        break;
    case NPC_STATE_CALL_OVER:
        Npc_StateCallOver(npc);
        break;
    case NPC_STATE_REACT_NEAR:
        Npc_StateReactNear(npc);
        break;
    case 0x92:
        Npc_State92(npc);
        break;
    case NPC_STATE_WAIT_TOWN_FREE:
        Npc_StateWaitTownFree(npc);
        break;
    case 0x94:
        Npc_State94(npc, 0U);
        break;
    case 0x95:
        Npc_State94(npc, 1U);
        break;
    case NPC_STATE_IDLE_BY_TIME:
        Npc_StateIdleByTime(npc);
        break;
    case 0x9F:
        Npc_State9F(npc);
        break;
    case 0x9D:
        Npc_State9D(npc);
        break;
    case NPC_STATE_STAND_UP:
        Npc_StateStandUp(npc);
        break;
    case NPC_STATE_LEAVE:
        Npc_StateLeave(npc);
        return 0;
    case NPC_STATE_CONTEST_WAIT:
        Npc_StateContestWait(npc);
        break;
    case NPC_STATE_BUSY_TALK_HOLD:
        Npc_StateBusyTalkHold(npc);
        break;
    case 0xA3:
        Npc_StateA3(npc);
        break;
    case NPC_STATE_FORGE:
        Npc_StateForge(npc);
        break;
    case NPC_STATE_HAMMER:
        Npc_StateHammer(npc);
        break;
    case NPC_STATE_CONTEST_REACT:
        Npc_StateContestReact(npc, 0);
        break;
    case NPC_STATE_CONTEST_REACT_LATE:
        Npc_StateContestReact(npc, 1);
        break;
    case 0xAD:
        Npc_StateAD(npc);
        break;
    case 0xAE:
        Npc_StateAE(npc);
        break;
    case NPC_STATE_ROUTE_JUMP_RANDOM:
        Npc_StateRouteJumpRandom(npc);
        break;
    case NPC_STATE_GONE:
        break;
    }
    return 1;
}

/*
 * The service townspeople's handlers (looks 0x51-0x55, ServiceNpc_RunState):
 * the house cook, the hairdresser, the bait seller and townsperson 0x49.
 *
 * States 0x4E and 0x6C (flag 0 or 1, unused): busy. Model part set 2 shows,
 * and it does not turn when talked to; it holds motion 0x431. Its talk
 * (ServiceNpc_StateTalk) sends it straight back to its route point's state.
 */
void ServiceNpc_StateBusy(Em *npc, u8 flag)
{
    NpcWork *w = (NpcWork *)&npc->work;

    w->partSet = 2;
    switch (npc->step) {
    case 0:
        npc->step++;
        w->keepFacing = 1;
        if (npc->motionId[0] != 0x431) {
            Actor_SetMotionOnLayer(npc, 0x431, 4, 0, 0);
        }
        break;
    case 1:
        break;
    }
}

s32 Npc_IsType52(Em *arg0);
extern s8 gTownModeStep;
extern s8 D_008BEB3F;

/*
 * States 0x01 and 0x59: stands (motion 0x3E9; variant 0x52 its own 0x436) with
 * model part set 0. NPC_HOUSE_COOK goes to state 0x51 at step 2 of town mode
 * 0xF while a town byte, not yet named, is 0. The others (not variant 0x52) greet the local
 * player once when within 130 (state 0x50); otherwise, after a wait of up to 255
 * frames (255-510 after a greeting), they
 * fidget (state 0x4F). motionPhase is 1 once greeted, until the player leaves.
 */
void ServiceNpc_StateIdle(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        w->partSet = 0;
        w->keepFacing = 0;
        npc->timer = (u8)((u16)Rand_Next(1) + 0xFF);
        if (Npc_IsType52(npc) == 1) {
            Actor_SetMotionOnLayer(npc, 0x436, 4, 0, 0);
        } else if (w->id == NPC_HOUSE_COOK || w->id == 0x49) {
            Actor_SetMotionOnLayer(npc, 0x3E9, -20, 0, 0);
        } else {
            Actor_SetMotionOnLayer(npc, 0x3E9, 20, 0, 0);
        }
        break;
    case 1:
        if (w->id == NPC_HOUSE_COOK) {
            if (gTownMode == 0xF && D_008BEB3F == 0 && gTownModeStep == 2 && w->motionPhase == 0) {
                Actor_SetState(npc, 0, 0x51);
            }
        } else if (Npc_IsType52(npc) == 0) {
            if (Vec_Distance(&npc->posX, (f32 *) gPlayers[gScene.localPlayer].pos) < 130.0f) {
                if (w->motionPhase == 0) {
                    Actor_SetState(npc, 0, 0x50);
                    w->motionPhase = 1;
                    npc->timer = (u8)Rand_Next(1) + 0xFF;
                }
            } else if (--npc->timer <= 0) {
                Actor_SetState(npc, 0, 0x4F);
            } else {
                w->motionPhase = 0;
            }
        }
        break;
    }
}

void ThrownObj_Spawn(void *arg, s32 kind, s32 n);

/*
 * Walks to the current route point; kind picks the motion: 0 (state 3) a walk
 * (variant 0x52 its own), 1 (state 2) motion 0x3F4, 2 (state 4) a run (0x3F7), 3
 * (state 0xB0) carrying: it first spawns its cooking lesson object
 * (ThrownObj_Spawn with its sceneSeat). It turns a tenth of the way each
 * frame and takes the next point within 80.
 */
void ServiceNpc_StateWalk(Em *npc, u8 gait)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *point;
    f32 dist;

    switch (npc->step) {
    case 0:
        npc->step++;
        switch (gait) {
        case 0:
            if (Npc_IsType52(npc) == 1) {
                if (npc->motionId[0] != 0x435) {
                    Actor_SetMotionOnLayer(npc, 0x435, 4, 0, 0);
                }
            } else if (npc->motionId[0] != 0x3F3) {
                Actor_SetMotionOnLayer(npc, 0x3F3, 4, 0, 0);
            }
            break;
        case 1:
            if (npc->motionId[0] != 0x3F4) {
                Actor_SetMotionOnLayer(npc, 0x3F4, 4, 0, 0);
            }
            break;
        case 2:
            if (npc->motionId[0] != 0x3F7) {
                Actor_SetMotionOnLayer(npc, 0x3F7, 4, 0, 0);
            }
            break;
        case 3:
            ThrownObj_Spawn(npc, npc->sceneSeat, (u8)gNpcScene.mealLevel);
            if (npc->motionId[0] != 0x3F8) {
                Actor_SetMotionOnLayer(npc, 0x3F8, 4, 0, 0);
            }
            break;
        }
        break;
    case 1:
        point = &w->route[w->routeIdx];
        npc->targetYaw = (s16)(u16)Actor_HeadingDiff(npc, point) / 10;
        npc->rotY += (s16)npc->targetYaw;
        dist = Vec_Distance(&npc->posX, point->pos.v);
        if (dist < 0.0f) {
            dist *= -1.0f;
        }
        if (dist <= 80.0f) {
            Npc_NextRoutePoint(npc);
        }
        break;
    }
}

/* State 0x4F: a fidget (motion 0x3EC), then stands again (state 1). */
void ServiceNpc_StateFidget(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, 0x3EC, 4, 0, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x3E9, 8, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, 1);
        }
        break;
    }
}

/*
 * State 0x50: greets the local player. It turns toward the player by a tenth
 * of the angle after 10 frames, nods (motion 0x42F), turns back and stands
 * again (state 1).
 */
void ServiceNpc_StateGreet(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        npc->targetYaw = (s16)(u16)Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos) / 10;
        npc->timer = 10;
        Actor_SetMotionOnLayer(npc, 0x42F, 8, 0, 0);
        break;
    case 1:
        if (--npc->timer <= 0) {
            npc->rotY += (s16)npc->targetYaw;
            npc->step++;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, 0x3E9, 10, 0, 0);
            npc->timer = 10;
            npc->step++;
        }
        break;
    case 3:
        if (--npc->timer <= 0) {
            npc->rotY -= (s16)npc->targetYaw;
            Actor_SetState(npc, 0, 1);
        }
        break;
    }
}

f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void PartFx_SpawnAt(f32 *pos, s32 id, f32 scale);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

/*
 * State 0xAA: as the greeting (state 0x50) with motion 0x437. Once, at frame 62,
 * it puts a PartFx 0x1D 30 ahead of it and 100 up, and plays common sound 0x9A.
 */
void ServiceNpc_StateGreetFx(Em *npc)
{
    f32 pos[3];
    f32 offset[3];
    s32 angles[3];
    f32 mtx[16];

    switch (npc->step) {
    case 0:
        npc->step++;
        npc->stateVar0 = 0;
        npc->targetYaw = (s16)(u16)Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos) / 10;
        npc->timer = 10;
        Actor_SetMotionOnLayer(npc, 0x437, 4, 0, 0);
        break;
    case 1:
        if (--npc->timer <= 0) {
            npc->rotY += (s16)npc->targetYaw;
            npc->step++;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, 0x3E9, 10, 0, 0);
            npc->timer = 10;
            npc->step++;
        } else if (npc->stateVar0 == 0 && Motion_PassedFrame((Player *) npc, 0, 62.0f) != 0) {
            angles[0] = 0;
            angles[1] = npc->rotY;
            angles[2] = 0;
            Mtx_FromAnglesXYZ(angles, mtx);
            pos[0] = 0.0f;
            pos[1] = 0.0f;
            pos[2] = 30.0f;
            Vec_Rotate(offset, pos, mtx);
            pos[0] = npc->posX + offset[0];
            pos[1] = 100.0f + npc->posY;
            pos[2] = npc->posZ + offset[2];
            PartFx_SpawnAt(pos, 0x1D, 1.0f);
            Sound_PlaySeFlat(SE_SLOT_COMMON, 0x9A, 0);
        }
        break;
    case 3:
        if (--npc->timer <= 0) {
            npc->rotY -= (s16)npc->targetYaw;
            Actor_SetState(npc, 0, 1);
        }
        break;
    }
}

void Player_ClearFlag(void *obj, u32 flag);

/*
 * State 0x97: the hair menu sets it on the hairdresser while the player gets
 * ready. Variant 0x55 holds motion 0x439; the others play one of three idle
 * motions at random, over and over. The motion's root movement is not applied
 * while 0x3EB plays.
 */
void ServiceNpc_StateHairWait(Em *npc)
{
    u16 pick;

    switch (npc->step) {
    case 0:
        if (npc->variant == 0x55) {
            if (npc->motionId[0] != 0x439) {
                Actor_SetMotionOnLayer(npc, 0x439, 4, 0, 0);
            }
        } else {
            pick = (u16)Rand_Next(1) % 3;
            switch (pick) {
            case 0:
                Actor_SetMotionOnLayer(npc, 0x3E9, 4, 0, 0);
                break;
            case 1:
                Actor_SetMotionOnLayer(npc, 0x3EA, 4, 0, 0);
                break;
            case 2:
                Actor_SetMotionOnLayer(npc, 0x3EB, 4, 0, 0);
                break;
            default:
                Actor_SetMotionOnLayer(npc, 0x3EC, 4, 0, 0);
                break;
            }
        }
        npc->step++;
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = 0;
        }
        break;
    }
    if (npc->motionId[0] == 0x3EB) {
        Player_SetFlag((Player *) npc, EM_FLAG_NO_ROOT_MOTION);
    } else {
        Player_ClearFlag((Player *) npc, EM_FLAG_NO_ROOT_MOTION);
    }
}

/*
 * State 0x98: can't be talked to. Coming from motion 0x439 it is put at the
 * point (2698, 0, 2930) and walks on to its next route point. Otherwise it plays
 * 0x43A, and once the local player is within 200 it plays 0x43B (talkable again
 * from frame 240). It is then put back at that point, facing angle 0, and goes on
 * along its route.
 */
void Npc51_State98(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&npc->work;

    w->noTalk = 1;
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] == 0x439) {
            npc->posX = 2698.0f;
            npc->posY = 0.0f;
            npc->posZ = 2930.0f;
            *(NpcPos *)npc->goal = *(NpcPos *)(&npc->posX);
            Npc_NextRoutePoint(npc);
            Actor_SetMotionOnLayer(npc, 0x3F3, 0, 0, 0);
        } else if (npc->motionId[0] != 0x43A) {
            Actor_SetMotionOnLayer(npc, 0x43A, 4, 0, 0);
        }
        npc->step++;
        break;
    case 1:
        if (Vec_Distance((f32 *) pl->pos, &npc->posX) < 200.0f) {
            Actor_SetMotionOnLayer(npc, 0x43B, 6, 0, 0);
            npc->step++;
        }
        break;
    case 2:
        if (Motion_ReachedFrame(npc, 0, 240.0f) != 0) {
            w->noTalk = 0;
        }
        if (npc->motion[0].playing <= 0) {
            npc->posX = 2698.0f;
            npc->posY = 0.0f;
            npc->posZ = 2930.0f;
            *(NpcPos *)npc->goal = *(NpcPos *)(&npc->posX);
            npc->rotY = 0;
            npc->targetYaw = npc->rotY;
            Npc_NextRoutePoint(npc);
            Actor_SetMotionOnLayer(npc, 0x3F3, 0, 0, 0);
        }
        break;
    }
}

/* State 0xB1: holds motion 0x40C from frame 0x8C. */
void Npc51_StateB1(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, 0x40C, 4, 0x8C, 0);
        break;
    case 1:
        break;
    }
}

/* State 0xB2: plays motion 0x432, then 0x430 for at most 100 frames, then goes on to the next route point. */
void Npc51_StateB2(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, 0x432, 4, 0, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            npc->timer = 100;
            Actor_SetMotionOnLayer(npc, 0x430, 6, 0xC, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0 || --npc->timer <= 0) {
            npc->step++;
            Npc_NextRoutePoint(npc);
        }
        break;
    }
}

void ThrownObj_SpawnForHairMenu(void *arg, s32 kind);

/*
 * State 0x99: the hair menu sets it on the hairdresser once the player stands
 * ready. It plays motion 0x40C, spawns the hair menu's object at frame 48
 * (ThrownObj_SpawnForHairMenu), plays 0x432 from frame 100, then stands (0x3E9;
 * the hair menu waits for that motion).
 */
void ServiceNpc_StateHairPrepare(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, 0x40C, 0, 0, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(npc, 0, 48.0f) != 0) {
            npc->step++;
            ThrownObj_SpawnForHairMenu(npc, 0);
        }
        break;
    case 2:
        if (Motion_ReachedFrame(npc, 0, 100.0f) != 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x432, 4, 0x1E, 0);
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, 0x3E9, 0x1E, 0, 0);
            npc->step++;
        }
        break;
    }
}

void ActorParticles_Spawn(void *a, void *b, u8 k);
void Npc_SetRouteFromHairChair(Em *arg0);

/*
 * State 0x9B: the hair menu sets it once the player sits in the chair. It does
 * the hair: motion 0x43C (particles from the player at its frames 120 and 200),
 * then 0x43D, then 0x43E (NPC_HAIRDRESSER) or 0x40E. It then takes its route from
 * the hair chair (Npc_SetRouteFromHairChair) and enters that route point's state.
 */
void ServiceNpc_StateHairDo(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];

    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x43C) {
            Actor_SetMotionOnLayer(npc, 0x43C, 2, 0, 0);
            npc->timer = 4;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x43D, 4, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            if (w->id == NPC_HAIRDRESSER) {
                Actor_SetMotionOnLayer(npc, 0x43E, 4, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, 0x40E, 4, 0, 0);
            }
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            Npc_SetRouteFromHairChair(npc);
            Actor_SetState(npc, 0, w->route[w->routeIdx].state);
        }
        break;
    }
    if (npc->motionId[0] == 0x43C) {
        if (Motion_PassedFrame((Player *) npc, 0, 200.0f) != 0) {
            ActorParticles_Spawn(pl, npc, 1);
        } else if (Motion_PassedFrame((Player *) npc, 0, 120.0f) != 0) {
            ActorParticles_Spawn(pl, npc, 2);
        }
    }
}

s32 Npc_IsAct51to55(Em *arg0);

/*
 * State 0x2C: the talk with the player. While busy (motion 0x431, states 0x4E
 * and 0x6C) it goes straight back to its route point's state. Otherwise it can't
 * be talked to again meanwhile. Unless its state keeps it facing its way, it turns toward the
 * local player over 10 frames. It plays its talk motion (NPC_HAIRDRESSER 0x439,
 * variant 0x52 0x434, 0x440 after the bait pose 0x43F, else 0x42F then 0x42E).
 * Once the town is back to TOWN_MODE_FREE it waits 16 frames (32, standing, for
 * looks 0x51-0x55 in 0x42E), turns back and enters its route point's state.
 * When that entry's argument is -1, it also starts a 20-frame move to the point.
 * Without a route it stands (state 1).
 */
void ServiceNpc_StateTalk(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *entry;

    switch (npc->motionId[0]) {
    case 0x431:
        Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        break;
    default:
        switch (npc->step) {
        case 0:
            w->noTalk = 1;
            if (*(s8 *)&w->keepFacing != 0) {
                npc->timer = 0;
            } else {
                npc->timer = 10;
                npc->targetYaw = Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos);
            }
            npc->step++;
            if (w->id == NPC_HAIRDRESSER) {
                Actor_SetMotionOnLayer(npc, 0x439, 4, 0, 0);
            } else if (Npc_IsType52(npc) == 1) {
                Actor_SetMotionOnLayer(npc, 0x434, 4, 0, 0);
            } else if (npc->motionId[0] == 0x43F) {
                Actor_SetMotionOnLayer(npc, 0x440, 2, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, 0x42F, 4, 0, 0);
            }
            break;
        case 1:
            w->noTalk = 1;
            npc->rotY += (s16)npc->targetYaw / 10;
            if (--npc->timer <= 0) {
                npc->step++;
            }
            break;
        case 2:
            w->noTalk = 1;
            if ((npc->motionId[0] == 0x42F || npc->motionId[0] == 0x440) && npc->motion[0].playing <= 0) {
                Actor_SetMotionOnLayer(npc, 0x42E, 4, 0, 0);
            }
            if (gTownMode == TOWN_MODE_FREE) {
                npc->timer = 16;
                npc->step++;
                if (Npc_IsAct51to55(npc) == 1 && npc->motionId[0] == 0x42E) {
                    npc->timer = 32;
                    Actor_SetMotionOnLayer(npc, 0x3E9, 16, 0, 0);
                }
            }
            break;
        case 3:
            w->noTalk = 1;
            if (--npc->timer <= 0) {
                if (*(s8 *)&w->keepFacing != 0) {
                    npc->timer = 0;
                } else {
                    npc->timer = 10;
                }
                npc->step++;
            }
            break;
        case 4:
            w->noTalk = 1;
            npc->rotY -= (s16)npc->targetYaw / 10;
            if (--npc->timer <= 0) {
                if (w->route != NULL) {
                    entry = w->route + w->routeIdx;
                    Actor_SetState(npc, 0, entry->state);
                    if (entry->arg == -1) {
                        *(NpcPos *)npc->goal = entry->pos;
                        Npc_StartMoveTo(npc, 20);
                    }
                } else {
                    Actor_SetState(npc, 0, 1);
                }
            }
            break;
        }
        break;
    }
}

/* State 0x9E: the bait menu sets it on the bait seller: holds the bait pose (motion 0x43F). */
void ServiceNpc_StateBaitOffer(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x43F) {
            Actor_SetMotionOnLayer(npc, 0x43F, 4, 0, 0);
        }
        break;
    case 1:
        break;
    }
}

/*
 * State 0x55: set after a service, by the kitchen and the kitchen guest on the
 * cook after the meal and by the bait menu on the bait seller. From the bait pose
 * it plays 0x440, then 0x40E. NPC_HOUSE_COOK then waits until, in
 * TOWN_MODE_KITCHEN_GUEST, the local player stands (TOWN_PC_IDLE). It then goes
 * back to its route's first point.
 */
void ServiceNpc_StateServed(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] == 0x43F) {
            Actor_SetMotionOnLayer(npc, 0x440, 2, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            if (npc->motionId[0] != 0x40E) {
                Actor_SetMotionOnLayer(npc, 0x40E, 0x10, 0, 0);
            }
        }
        break;
    case 2:
        if (w->id == NPC_HOUSE_COOK && gTownMode == TOWN_MODE_KITCHEN_GUEST && pl->state == TOWN_PC_IDLE) {
            w->routeIdx = 0;
            Actor_SetState(npc, 0, w->route[w->routeIdx].state);
        }
        break;
    }
}

extern u8 gTownJingleOn;
void Bgm_StopEventFade(Player *p);

/* State 0xA8, a route point: stops the town's event jingle (Bgm_StopEventFade) if one plays. */
void ServiceNpc_StateStopJingle(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (gTownJingleOn != 0) {
            Bgm_StopEventFade(&gPlayers[gScene.localPlayer]);
            gTownJingleOn = 0;
        }
        break;
    }
}

s32 Game_IsOnline(void);
extern NpcRouteEnt gNpcRouteKitchen[];
extern NpcRouteEnt gKitchenRoute0[];
extern NpcRouteEnt gKitchenRoute1[];
extern NpcRouteEnt gKitchenRoute2[];

/*
 * Gives the cook its kitchen route and enters the first point's state. Offline,
 * away from the village (gSave.awayFromVillage), it takes gNpcRouteKitchen.
 * Otherwise it takes route 0, 1 or 2 by kind (the host's spot; any other value
 * takes route 0).
 */
void Npc_SetKitchenRoute(Em *npc, u8 spot)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (gSave.awayFromVillage != 0 && Game_IsOnline() == 0) {
        w->route = gNpcRouteKitchen;
    } else {
        switch (spot) {
        default:
            w->route = gKitchenRoute0;
            break;
        case 1:
            w->route = gKitchenRoute1;
            break;
        case 2:
            w->route = gKitchenRoute2;
            break;
        }
    }
    w->routeIdx = 0;
    Actor_SetState(npc, 0, w->route[w->routeIdx].state);
}

/* Steps to the next route point and enters its state (the kitchen moves the cook on with it). */
void Npc_AdvanceRoute(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    w->routeIdx++;
    Actor_SetState(npc, 0, (u16)w->route[w->routeIdx].state);
}

extern NpcRouteEnt *gNpcPlaceRoutes0[];
extern NpcRouteEnt *gNpcPlaceRoutes1[];
extern NpcRouteEnt *gNpcPlaceRoutes2[];
extern NpcRouteEnt *gNpcPlaceRoutes3[];

/*
 * Puts a townsperson on route i of route set 0-3 (gNpcPlaceRoutes0-3; any other
 * set is 0): at the route's first point, with its goal there and no move left.
 * It enters the first point's state; a first point of state 0x81 instead sets
 * its facing from the entry's argument (not talkable) and starts at the next point.
 */
void Npc_PlaceOnRoute(Em *npc, u8 set, u8 routeNo)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (set) {
    default:
        w->route = gNpcPlaceRoutes0[routeNo];
        break;
    case 1:
        w->route = gNpcPlaceRoutes1[routeNo];
        break;
    case 2:
        w->route = gNpcPlaceRoutes2[routeNo];
        break;
    case 3:
        w->route = gNpcPlaceRoutes3[routeNo];
        break;
    }
    w->routeIdx = 0;
    if ((w->route)[w->routeIdx].state == 0x81) {
        w->noTalk = 1;
        npc->rotY = (w->route)[w->routeIdx].arg;
        npc->targetYaw = (w->route)[w->routeIdx].arg;
        Npc_NextRoutePoint(npc);
    }
    Actor_SetState(npc, 0, (w->route)[w->routeIdx].state);
    *(NpcPos *)(&npc->posX) = (w->route)->pos;
    *(NpcPos *)npc->goal = (w->route)->pos;
    *(s16 *)&w->moveCount = 0;
}

/* State 0xB4: a fidget (motion 0x3EC), then stands 60 frames, then goes on to the next route point. */
void ServiceNpc_StateFidgetPause(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x3EC) {
            Actor_SetMotionOnLayer(npc, 0x3EC, 4, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x3E9, 8, 0, 0);
            npc->timer = 60;
        }
        break;
    case 2:
        if (--npc->timer <= 0) {
            Npc_NextRoutePoint(npc);
        }
        break;
    }
}

void Player_ClearFlag(void *obj, u32 flag);

/*
 * State 0xB5: walks in place twice (motion 0x3EB, its root movement not
 * applied), plays 0x40E, stands 8 frames, then goes on to the next route point.
 */
void ServiceNpc_StateStepInPlace(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 0x3EB) {
            Actor_SetMotionOnLayer(npc, 0x3EB, -4, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x3EB, -4, 0, 0);
            npc->timer = 60;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x40E, 4, 0, 0);
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, 0x3E9, 4, 0, 0);
            npc->timer = 8;
        }
        break;
    case 4:
        if (--npc->timer <= 0) {
            Npc_NextRoutePoint(npc);
        }
        break;
    }
    if (npc->motionId[0] == 0x3EB) {
        Player_SetFlag((Player *) npc, EM_FLAG_NO_ROOT_MOTION);
    } else {
        Player_ClearFlag((Player *) npc, EM_FLAG_NO_ROOT_MOTION);
    }
}

void Npc_StateLeave(Em *arg0);
void ServiceNpc_StateTalk(Em *arg0);

/*
 * The service townspeople's handler (looks 0x51-0x55) by Em.state; the
 * route states 0x81, 0x83, 0xB3 and state 0x5A are the general ones. The cook
 * and townsperson 0x49 can't be talked to outside states 1, 0x4F and 0x50
 * (standing, fidgeting, greeting). Returns 0 for state 0x5F, else 1.
 */
s32 ServiceNpc_RunState(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->state) {
    case 0x59:
    case 0x1:
        ServiceNpc_StateIdle(npc);
        break;
    case 0x97:
        ServiceNpc_StateHairWait(npc);
        break;
    case 0x98:
        Npc51_State98(npc);
        break;
    case 0x3:
        ServiceNpc_StateWalk(npc, 0);
        break;
    case 0x2:
        ServiceNpc_StateWalk(npc, 1);
        break;
    case 0x4E:
        ServiceNpc_StateBusy(npc, 0);
        break;
    case 0x6C:
        ServiceNpc_StateBusy(npc, 1);
        break;
    case 0x2C:
        ServiceNpc_StateTalk(npc);
        break;
    case 0x4F:
        ServiceNpc_StateFidget(npc);
        break;
    case 0x50:
        ServiceNpc_StateGreet(npc);
        break;
    case 0x5A:
        Npc_StateWarpToRoute(npc);
        break;
    case 0x99:
        ServiceNpc_StateHairPrepare(npc);
        break;
    case 0x81:
        Npc_StateRouteTurn(npc);
        break;
    case 0x9B:
        ServiceNpc_StateHairDo(npc);
        break;
    case 0x9E:
        ServiceNpc_StateBaitOffer(npc);
        break;
    case 0x55:
        ServiceNpc_StateServed(npc);
        break;
    case 0x4:
        ServiceNpc_StateWalk(npc, 2);
        break;
    case 0xA8:
        ServiceNpc_StateStopJingle(npc);
        break;
    case 0x83:
        Npc_StateRouteJump(npc);
        break;
    case 0xAA:
        ServiceNpc_StateGreetFx(npc);
        break;
    case 0x5F:
        Npc_StateLeave(npc);
        return 0;
    case 0xB0:
        ServiceNpc_StateWalk(npc, 3);
        break;
    case 0xB1:
        Npc51_StateB1(npc);
        break;
    case 0xB2:
        Npc51_StateB2(npc);
        break;
    case 0xB4:
        ServiceNpc_StateFidgetPause(npc);
        break;
    case 0xB5:
        ServiceNpc_StateStepInPlace(npc);
        break;
    case 0xB3:
        Npc_StateRouteJumpRandom(npc);
        break;
    }
    if ((w->id == NPC_HOUSE_COOK || w->id == 0x49) && npc->state != 1 && npc->state != 0x4F && npc->state != 0x50) {
        w->noTalk = 1;
    }
    return 1;
}

/*
 * The pets' handlers (looks 0x4F and 0x50, Pet_RunState): the Poogie
 * (NPC_POOGIE), the care pet (NPC_POOGIE_CARE) and the house pet (NPC_HOUSE_PET).
 *
 * PET_STATE_IDLE (flag 0) and state 0x59 (flag 1): stands (motion 0x3E9, blended
 * over 2 or 8 frames).
 */
void Pet_StateIdle(Em *pet, s8 slowBlend)
{
    switch (pet->step) {
    case 0:
        pet->step++;
        if (slowBlend == 0) {
            Actor_SetMotionOnLayer(pet, 0x3E9, 2, 0, 0);
        } else {
            Actor_SetMotionOnLayer(pet, 0x3E9, 8, 0, 0);
        }
        break;
    }
}

void Player_SetFlag(void *obj, u32 flag);

/*
 * NPC_STATE_HOUSE_OPEN: while the house is open to visitors the pet sits (motion
 * 0x432; already seated, from its frame 0x58), its root movement not applied.
 */
void Pet_StateHouseOpen(Em *pet)
{
    switch (pet->step) {
    case 0:
        pet->step++;
        Player_SetFlag((Player *) pet, EM_FLAG_NO_ROOT_MOTION);
        if (pet->motionId[0] != 0x432) {
            Actor_SetMotionOnLayer(pet, 0x432, 0xE, 0, 0);
        } else {
            Actor_SetMotionOnLayer(pet, 0x432, 0, 0x58, 0);
        }
        break;
    case 1:
        break;
    }
}

/*
 * States 0x52 (flag 0: placed already seated, from frame 0x58 of motion 0x432)
 * and 0x53 (flag 1: sits down): sits by the player, its root movement not
 * applied. It stays while the player is in the save menu in bed (TOWN_MODE_SAVE
 * with TOWN_PC_BED_LIE_DOWN or TOWN_PC_BED_SLEEP), or else while the player is
 * within 130 of the spot it walked to. Then it gets up (0x431), stands 30 frames
 * (counted in motionPhase) and follows again (PET_STATE_FOLLOW).
 */
void Pet_StateSit(Em *pet, s8 sitDown)
{
    NpcWork *w = (NpcWork *)&pet->work;
    Player *pl = &gPlayers[gScene.localPlayer];

    switch (pet->step) {
    case 0:
        pet->step++;
        Player_SetFlag((Player *) pet, EM_FLAG_NO_ROOT_MOTION);
        if (sitDown == 0) {
            Actor_SetMotionOnLayer(pet, 0x432, 0, 0x58, 0);
        } else {
            Actor_SetMotionOnLayer(pet, 0x432, 0xE, 0, 0);
        }
        break;
    case 1:
        if (gTownMode == TOWN_MODE_SAVE) {
            if (pl->state != TOWN_PC_BED_LIE_DOWN && pl->state != TOWN_PC_BED_SLEEP) {
                Player_ClearFlag((Player *) pet, EM_FLAG_NO_ROOT_MOTION);
                Actor_SetMotionOnLayer(pet, 0x431, 4, 0, 0);
                pet->step++;
            }
        } else if (!(Vec_Distance((f32 *) pl->pos, w->followSpot.v) <= 130.0f)) {
            Player_ClearFlag((Player *) pet, EM_FLAG_NO_ROOT_MOTION);
            Actor_SetMotionOnLayer(pet, 0x431, 4, 0, 0);
            pet->step++;
        }
        break;
    case 2:
        if (pet->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(pet, 0x3E9, 10, 0, 0);
            w->motionPhase = 30;
            pet->step++;
        }
        break;
    case 3:
        if (--w->motionPhase <= 0) {
            Actor_SetState(pet, 0, PET_STATE_FOLLOW);
        }
        break;
    }
}

void Vec_Normalize(f32 *v);

/*
 * PET_STATE_PETTED: puts the local player in the petting pose (TOWN_PC_GRABBED).
 * Over 10 frames the pet turns toward the player it touches (Em.contact). It then
 * steps back to 78 from the player over 20 frames if nearer, and keeps turning to
 * face the player.
 */
void Pet_StatePetted(Em *pet)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    f32 away[3];

    switch (pet->step) {
    case 0:
        pet->motionSpeed = 1.0f;
        pet->step++;
        Actor_SetState(pl, 0, TOWN_PC_GRABBED);
        pet->timer = 10;
        pet->targetYaw = Actor_HeadingDiff(pet, (pet)->contact->pos);
        Actor_SetMotionOnLayer(pet, 0x3EA, 2, 0, 0);
        break;
    case 1:
        if (--pet->timer >= 0) {
            pet->rotY += (s16)pet->targetYaw / 10;
        } else {
            pet->step++;
        }
        break;
    case 2:
        if (Vec_Distance(&pet->posX, (f32 *) pl->pos) < 78.0f) {
            away[0] = pet->posX - pl->pos[0];
            away[1] = pet->posY - pl->pos[1];
            away[2] = pet->posZ - pl->pos[2];
            Vec_Normalize(away);
            away[0] *= 78.0f;
            away[1] *= 78.0f;
            away[2] *= 78.0f;
            pet->goal[0] = pl->pos[0] + away[0];
            pet->goal[1] = pl->pos[1] + away[1];
            pet->goal[2] = pl->pos[2] + away[2];
            Npc_StartMoveTo(pet, 20);
            Actor_SetMotionOnLayer(pet, 0x3F2, 4, 0, 0);
            pet->step++;
        }
        break;
    case 3:
        pet->rotY += (s16)(u16)Actor_HeadingDiff(pet, pl->pos) / 5;
        break;
    }
}

/*
 * PET_STATE_FACE: a pet menu opens. The pet turns over 10 frames toward the
 * player it touches, waits until the local player is within 100, then stands
 * (PET_STATE_IDLE).
 */
void Pet_StateFace(Em *pet)
{
    Player *pl = &gPlayers[gScene.localPlayer];

    switch (pet->step) {
    case 0:
        pet->motionSpeed = 1.0f;
        pet->step++;
        pet->timer = 10;
        pet->targetYaw = Actor_HeadingDiff(pet, (pet)->contact->pos);
        Actor_SetMotionOnLayer(pet, 0x3EA, 2, 0, 0);
        break;
    case 1:
        if (--pet->timer >= 0) {
            pet->rotY += (s16)pet->targetYaw / 10;
        } else {
            pet->step++;
        }
        break;
    case 2:
        if (Vec_Distance(&pet->posX, (f32 *) pl->pos) < 100.0f) {
            pet->step++;
        }
        break;
    case 3:
        Actor_SetState(pet, 0, PET_STATE_IDLE);
        break;
    }
}

void ModelObj_SpawnOn(void *arg0, s32 arg1);
extern s8 gPetFollows;

/*
 * PET_STATE_ANGRY: the petting angered the pet (PET_ACT_RUN_OFF). The Poogie
 * stops following (gPetFollows). Anger marks pop up every 3-7 frames
 * (ModelObj_SpawnOn, counted in motionPhase). It rushes at three times the speed
 * for 20 frames, then knocks the local player down (TOWN_PC_THROWN). Once the
 * player stands again (TOWN_PC_IDLE), it runs off (PET_STATE_RUN_OFF).
 */
void Pet_StateAngry(Em *pet)
{
    NpcWork *w = (NpcWork *)&pet->work;

    if (pet->step <= 2) {
        if ((s16)--w->motionPhase <= 0) {
            ModelObj_SpawnOn(pet, 1);
            w->motionPhase = (u16)Rand_Next(1) % 5 + 3;
        }
    }
    switch (pet->step) {
    case 0:
        if (w->id == NPC_POOGIE) {
            gPetFollows = 0;
        }
        w->petAct = PET_ACT_RUN_OFF;
        pet->step++;
        pet->motionSpeed = 3.0f;
        Actor_SetMotionOnLayer(pet, 0x3F6, 4, 0x26, 0);
        pet->timer = 20;
        break;
    case 1:
        if (--pet->timer <= 0) {
            pet->step++;
            pet->timer = 5;
            pet->motionSpeed = 1.0f;
            Actor_SetMotionOnLayer(pet, 0x3FA, 4, 0, 0);
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_THROWN);
        }
        break;
    case 2:
        if (pet->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(pet, 0x3E9, 4, 0, 0);
            pet->step++;
        }
        break;
    case 3:
        if (gPlayers[gScene.localPlayer].state == TOWN_PC_IDLE) {
            Actor_SetState(pet, 0, PET_STATE_RUN_OFF);
        }
        break;
    }
}

void ModelObj_SpawnOn(void *arg0, s32 arg1);

/*
 * PET_STATE_JOY: plays its joy (motion 0x3F3, with a mark over it) as
 * PET_ACT_REACT, then 0x3EF for 100 frames, then stands and follows
 * (PET_STATE_FOLLOW).
 */
void Pet_StateJoy(Em *pet)
{
    NpcWork *w = (NpcWork *)&pet->work;

    switch (pet->step) {
    case 0:
        w->petAct = PET_ACT_REACT;
        pet->step++;
        Actor_SetMotionOnLayer(pet, 0x3F3, 4, 0, 0);
        ModelObj_SpawnOn(pet, 0);
        break;
    case 1:
        if (pet->motion[0].playing <= 0) {
            w->petAct = PET_ACT_NONE;
            Actor_SetMotionOnLayer(pet, 0x3EF, 6, 0, 0);
            pet->step++;
            pet->timer = 100;
        }
        break;
    case 2:
        if (--pet->timer <= 0) {
            Actor_SetMotionOnLayer(pet, 0x3E9, 0x18, 0, 0);
            pet->step++;
        }
        break;
    case 3:
        if (pet->motion[0].playing <= 0) {
            Actor_SetState(pet, 0, PET_STATE_FOLLOW);
        }
        break;
    }
}

/* PET_STATE_GREAT_JOY: plays its great joy (motion 0x433) as PET_ACT_REACT, then follows (PET_STATE_FOLLOW). */
void Pet_StateGreatJoy(Em *pet)
{
    NpcWork *w = (NpcWork *)&pet->work;

    switch (pet->step) {
    case 0:
        w->petAct = PET_ACT_REACT;
        pet->step++;
        Actor_SetMotionOnLayer(pet, 0x433, 4, 0, 0);
        break;
    case 1:
        if (pet->motion[0].playing <= 0) {
            w->petAct = PET_ACT_NONE;
            Actor_SetMotionOnLayer(pet, 0x3E9, 2, 0, 0);
            pet->step++;
        }
        break;
    case 2:
        Actor_SetState(pet, 0, PET_STATE_FOLLOW);
        break;
    }
}

u8 *ActSpot_GetList(u16 area);
s32 Area_IsOnlineHouse(s32 arg0);
/* Pos_InZone gives 0 or 1. Pet_StateRunOff masks the result to a byte before it
   compares, while Pet_StateFollow and Pet_StateWait compare the whole register.
   So the declaration says s32, and Pet_StateRunOff has (u8) casts: both forms then
   compile as the game's code. */
s32 Pos_InZone(void *obj, f32 *p);
extern f32 gNpc4FArea0[];
extern f32 gNpc4FArea1[];
extern f32 gNpc4FArea2[];
extern f32 gNpc4FArea3[];
extern f32 gNpc4FArea4[];
extern NpcRouteEnt gNpc4FRoute0[];
extern NpcRouteEnt gNpc4FRoute1[];
extern NpcRouteEnt gNpc4FRouteElse[];
extern NpcRouteEnt gNpc4FRoute3[];
extern NpcRouteEnt gNpc4FRoute2[];
extern NpcRouteEnt gNpc4FRoute4[];

/*
 * PET_STATE_RUN_OFF: the pet runs off (PET_ACT_RUN_OFF). In AREA_HUB_0x89 it
 * takes the run-off route of the zone it is in (gNpc4FArea0-4; any other place
 * gNpc4FRouteElse) at double speed and enters its first point's state.
 * Elsewhere it turns over 10 frames toward the area's first action spot and runs
 * that way, at double speed (normal speed in the online houses). After 270 frames
 * it stops (PET_ACT_NONE) and stays there (state 0x60).
 */
void Pet_StateRunOff(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;
    u8 *spot = ActSpot_GetList(gScene.area);
    NpcRouteEnt *nextRoute;
    f32 spotPos[3];

    if (gScene.area == AREA_HUB_0x89) {
        obj->motionSpeed *= 2.0f;
        if ((u8)Pos_InZone(&obj->posX, gNpc4FArea0) == 1) {
            nextRoute = gNpc4FRoute0;
        } else if ((u8)Pos_InZone(&obj->posX, gNpc4FArea2) == 1) {
            nextRoute = gNpc4FRoute2;
        } else if ((u8)Pos_InZone(&obj->posX, gNpc4FArea1) == 1) {
            nextRoute = gNpc4FRoute1;
        } else if ((u8)Pos_InZone(&obj->posX, gNpc4FArea3) == 1) {
            nextRoute = gNpc4FRoute3;
        } else if ((u8)Pos_InZone(&obj->posX, gNpc4FArea4) == 1) {
            nextRoute = gNpc4FRoute4;
        } else {
            nextRoute = gNpc4FRouteElse;
        }
        w->route = nextRoute;
        w->routeIdx = 0;
        Actor_SetState(obj, 0, nextRoute->state);
        return;
    }
    spotPos[0] = ((ActionSpot *)spot)->x;
    spotPos[1] = ((ActionSpot *)spot)->y;
    spotPos[2] = ((ActionSpot *)spot)->z;
    switch (obj->step) {
    case 0:
        w->petAct = PET_ACT_RUN_OFF;
        obj->step++;
        obj->timer = 10;
        obj->targetYaw = Actor_HeadingDiff(obj, spotPos);
        obj->motionSpeed *= 2.0f;
        if (Area_IsOnlineHouse(gScene.area) != 0) {
            obj->motionSpeed = 1.0f;
        }
        Actor_SetMotionOnLayer(obj, 0x3F7, 2, 0, 0);
        break;
    case 1:
        if (--obj->timer >= 0) {
            obj->rotY += (s16)obj->targetYaw / 10;
        } else {
            obj->step++;
        }
        break;
    case 2:
        if (++obj->timer >= 270) {
            obj->motionSpeed = 1.0f;
            w->petAct = PET_ACT_NONE;
            obj->step++;
            Actor_SetState(obj, 0, 0x60);
        }
        break;
    case 3:
        break;
    }
}

/* An identity helper: through it the entry address (routeIdx * 0x14, one
   NpcRouteEnt each) is added index first, as
   the game does after stepping the route (Pet_StateRunRoute). */
static inline s32 Npc_RouteAddr(NpcRouteEnt *route)
{
    return (s32)route;
}

/*
 * State 0xAF: runs along its route (motion 0x3F7), turning a fifth of the way
 * each frame. Within 30 of a point it steps on: an end entry (state 0) starts the
 * route over, an entry of another state is entered at once, and an entry of
 * this state keeps it running.
 */
void Pet_StateRunRoute(Em *pet)
{
    NpcWork *w = (NpcWork *)&pet->work;
    NpcRouteEnt *point;
    NpcRouteEnt *next;
    f32 dist;

    switch (pet->step) {
    case 0:
        pet->step++;
        if (pet->motionId[0] != 0x3F7) {
            Actor_SetMotionOnLayer(pet, 0x3F7, 2, 0, 0);
        }
        break;
    case 1:
        point = w->route + w->routeIdx;
        pet->targetYaw = (s16)(u16)Actor_HeadingDiff(pet, point) / 5;
        pet->rotY += (s16)pet->targetYaw;
        dist = Vec_Distance(&pet->posX, point->pos.v);
        if (dist < 0.0f) {
            dist *= -1.0f;
        }
        if (dist <= 30.0f) {
            (w->routeIdx)++;
            next = (NpcRouteEnt *)(Npc_RouteAddr(w->route) + w->routeIdx * 0x14);
            if (next->state == 0) {
                w->routeIdx = 0;
            } else if (pet->state != next->state) {
                pet->step++;
                Actor_SetState(pet, 0, next->state);
                w->wait = 0;
            } else {
                pet->step = 0;
            }
        }
        break;
    case 2:
        if (pet->motion[0].playing <= 0) {
            next = w->route + w->routeIdx;
            if (pet->state != next->state) {
                pet->step++;
                Actor_SetState(pet, 0, next->state);
                w->wait = 0;
            } else {
                pet->step = 0;
            }
        }
        break;
    }
}

extern u8 gHouseAccess;

/*
 * State 0x57: walks along its route (motion 0x3EB), turning a tenth of the way
 * each frame. At each point it reached it stops half the time for motion 0x3EF.
 * Otherwise it goes on as state 0xAF does. While the house is open to visitors
 * it sits instead (NPC_STATE_HOUSE_OPEN).
 */
void Pet_StateWander(Em *pet)
{
    NpcWork *w = (NpcWork *)&pet->work;
    NpcRouteEnt *point;
    NpcRouteEnt *next;
    f32 dist;

    if (gHouseAccess != HOUSE_ACCESS_CLOSED) {
        Actor_SetState(pet, 0, NPC_STATE_HOUSE_OPEN);
        return;
    }
    switch (pet->step) {
    case 0:
        pet->step++;
        if (pet->motionId[0] == 0x3EF) {
            Actor_SetMotionOnLayer(pet, 0x3EB, 8, 0, 0);
        } else {
            Actor_SetMotionOnLayer(pet, 0x3EB, 4, 0, 0);
        }
        break;
    case 1:
        point = w->route + w->routeIdx;
        pet->targetYaw = (s16)(u16)Actor_HeadingDiff(pet, point) / 10;
        pet->rotY += (s16)pet->targetYaw;
        dist = Vec_Distance(&pet->posX, point->pos.v);
        if (dist < 0.0f) {
            dist *= -1.0f;
        }
        if (dist <= 30.0f) {
            (w->routeIdx)++;
            next = (NpcRouteEnt *)(Npc_RouteAddr(w->route) + w->routeIdx * 0x14);
            if (next->state == 0) {
                w->routeIdx = 0;
            } else if ((u16)Rand_Next(1) & 1) {
                pet->step++;
                Actor_SetMotionOnLayer(pet, 0x3EF, 10, 0, 0);
            } else {
                next = w->route + w->routeIdx;
                if (pet->state != next->state) {
                    pet->step++;
                    Actor_SetState(pet, 0, next->state);
                    w->wait = 0;
                } else {
                    pet->step = 0;
                }
            }
        }
        break;
    case 2:
        if (pet->motion[0].playing <= 0) {
            next = w->route + w->routeIdx;
            if (pet->state != next->state) {
                pet->step++;
                Actor_SetState(pet, 0, next->state);
                w->wait = 0;
            } else {
                pet->step = 0;
            }
        }
        break;
    }
}

s32 Npc_IsRising(Em *obj);
void Em_CollideField(Em *em, u8 keep);
extern f32 gNpc4FArea[];

/*
 * PET_STATE_FOLLOW: follows the local player. Before the steps it checks four
 * cases:
 * - while the house is open to visitors, it sits (NPC_STATE_HOUSE_OPEN);
 * - offline, before the Poogie has a name, every pet but NPC_HOUSE_PET follows
 *   the naming talker instead (state 0x5C);
 * - the Poogie inside gNpc4FArea waits (state 0x5D) while the player climbs or
 *   stands above it;
 * - NPC_HOUSE_PET in AREA_HOUSE waits.
 * Em.petWaitBelow is 1 while it waits that way.
 * Step 0 picks a spot within 99 of the player and walks (motion 0x3EB).
 * Step 1 walks there, at 2.5 times the speed offline in AREA_HUB_0x89 and
 * AREA_HUB_0x57. More than 5000 away, it is put at a fixed entry point (by the
 * player's side of the town) and walks in from there for 200 frames without
 * wall collision. While the player lies in bed it stands within 65, plays
 * motion 0x3EF (steps 2-3), then sits down (state 0x53). Near the spot, with
 * the player within 130 of it, it stands and sits down after 45-105 frames;
 * the player moving away sends it back to step 0. After 120 frames pushed against
 * a wall without moving, it sits down too.
 */
void Pet_StateFollow(Em *pet)
{
    Player *pl;
    NpcWork *w;
    u8 *save;
    f32 spot[3];
    f32 prevPos[3];
    u32 offset;
    f32 dist;

    w = (NpcWork *)&pet->work;
    pl = &gPlayers[gScene.localPlayer];
    save = (u8 *)&gSave;
    if (gHouseAccess != HOUSE_ACCESS_CLOSED) {
        Actor_SetState(pet, 0, NPC_STATE_HOUSE_OPEN);
        return;
    }
    pet->petWaitBelow = 0;
    if (Game_IsOnline() == 0 && (s8)((SaveData *)save)->poogieName[0] == 0 && w->id != NPC_HOUSE_PET) {
        Actor_SetState(pet, 0, 0x5C);
        return;
    }
    if (w->id == NPC_POOGIE) {
        if (Pos_InZone(&pet->posX, gNpc4FArea) == 1) {
            if (Npc_IsRising((Em *)pl) == 1) {
                pet->petWaitBelow = 1;
            } else if (!(pl->pos[1] <= pet->posY)) {
                pet->petWaitBelow = 1;
            }
        }
        if (pet->petWaitBelow == 1) {
            Actor_SetState(pet, 0, 0x5D);
            return;
        }
    }
    if (w->id == NPC_HOUSE_PET && gScene.area == AREA_HOUSE) {
        pet->petWaitBelow = 1;
        Actor_SetState(pet, 0, 0x5D);
        return;
    }
    switch (pet->step) {
    case 0:
        w->petCount = 0;
        pet->fieldCollide = 1;
        pet->step++;
        if (pet->motionId[0] != 0x3EB) {
            Actor_SetMotionOnLayer(pet, 0x3EB, 4, 0, 0);
        }
        *(NpcPos *)spot = *(NpcPos *)pl->pos;
        offset = (u16)((u16)Rand_Next(1) % 100);
        if (offset & 1) {
            spot[0] += offset;
        } else {
            spot[0] -= offset;
        }
        offset = (u16)((u16)Rand_Next(1) % 100);
        if (offset & 1) {
            spot[2] += offset;
        } else {
            spot[2] -= offset;
        }
        w->followSpot = *(NpcPos *)spot;
        break;
    case 1:
        if (Game_IsOnline() == 0) {
            if (gScene.area == AREA_HUB_0x89 || gScene.area == AREA_HUB_0x57) {
                pet->motionSpeed = 2.5f;
            } else {
                pet->motionSpeed = 1.0f;
            }
        }
        pet->targetYaw = Actor_HeadingDiff(pet, w->followSpot.v);
        pet->rotY += (s16)pet->targetYaw / 10;
        dist = Vec_Distance(&pet->posX, w->followSpot.v);
        if (dist < 15.0f) {
            w->warpTimer = 0;
        }
        if (dist < 5000.0f && w->warpTimer != 0) {
            break;
        }
        if (!(dist <= 5000.0f)) {
            if (!(pl->pos[0] <= 2500.0f)) {
                w->followSpot.v[0] = 10959.0f;
                w->followSpot.v[1] = pl->pos[1];
                w->followSpot.v[2] = 14487.0f;
                w->warpTimer = 200;
                pet->posX = 11338.0f;
                pet->posY = pl->pos[1];
                pet->posZ = 14352.0f;
                pet->rotY = 0;
                pet->targetYaw = Actor_HeadingDiff(pet, w->followSpot.v);
                pet->rotY = pet->targetYaw;
            } else {
                w->followSpot.v[0] = 2070.0f;
                w->followSpot.v[1] = pl->pos[1];
                w->followSpot.v[2] = 1100.0f;
                w->warpTimer = 200;
                pet->posX = 2540.0f;
                pet->posY = pl->pos[1];
                pet->posZ = 961.0f;
                pet->rotY = 0;
                pet->targetYaw = Actor_HeadingDiff(pet, w->followSpot.v);
                pet->rotY = pet->targetYaw;
            }
        } else if (pl->state == TOWN_PC_BED_LIE_DOWN || pl->state == TOWN_PC_BED_SLEEP) {
            if (dist < 65.0f) {
                pet->step++;
                Actor_SetMotionOnLayer(pet, 0x3E9, 4, 0, 0);
            } else if (pet->motionId[0] != 0x3EB) {
                Actor_SetMotionOnLayer(pet, 0x3EB, 2, 0, 0);
            }
        } else if (dist < 130.0f) {
            if (!(Vec_Distance(w->followSpot.v, (f32 *) pl->pos) <= 130.0f)) {
                pet->step--;
            } else {
                if (pet->motionId[0] != 0x3E9) {
                    Actor_SetMotionOnLayer(pet, 0x3E9, 4, 0, 0);
                }
                if (w->motionPhase != 0 && w->warpTimer == 0) {
                    if (--w->motionPhase <= 0) {
                        pet->motionSpeed = 1.0f;
                        Actor_SetState(pet, 0, 0x53);
                    }
                } else {
                    w->motionPhase = ((u16)Rand_Next(1) % 5 + 3) * 15;
                }
            }
        } else if (pet->motionId[0] != 0x3EB) {
            Actor_SetMotionOnLayer(pet, 0x3EB, 2, 0, 0);
        }
        break;
    case 2:
        if (pet->motion[0].playing <= 0) {
            pet->step++;
            Actor_SetMotionOnLayer(pet, 0x3EF, 10, 0, 0);
        }
        break;
    case 3:
        if (pet->motion[0].playing <= 0) {
            Actor_SetState(pet, 0, 0x53);
        }
        break;
    }
    prevPos[0] = pet->posX;
    prevPos[2] = pet->posZ;
    prevPos[1] = pet->posY;
    if (w->warpTimer != 0) {
        (w->warpTimer)--;
    }
    if (w->warpTimer == 0 || Game_IsOnline() == 1) {
        Em_CollideField(pet, 0);
    }
    dist = Vec_Distance(&pet->posX, (f32 *) prevPos);
    if (pet->wallHitDirs != 0 && !(dist <= 1.0f)) {
        (w->petCount)++;
    } else {
        w->petCount = 0;
    }
    if (w->petCount > 0x78) {
        w->petCount = 0;
        pet->motionSpeed = 1.0f;
        Actor_SetState(pet, 0, 0x53);
    }
}

u8 Slab_Contains(f32 *pos, f32 *a, f32 *b, f32 height, f32 radius);

/*
 * Whether the position lies in a pets' zone: p holds two points (x, z), (x, z)
 * and a radius. The zone is the slab around the segment between them, from
 * height -600 up 1500 (Slab_Contains). Returns 1 or 0.
 */
s32 Pos_InZone(void *pos, f32 *zone)
{
    f32 start[3];
    f32 end[3];

    start[0] = zone[0];
    start[1] = -600.0f;
    start[2] = zone[1];
    end[0] = zone[2];
    end[1] = -600.0f;
    end[2] = zone[3];
    return Slab_Contains(pos, start, end, 1500.0f, zone[4]);
}

s32 Npc_IsRising(Em *actor)
{
    return actor->posY - actor->prevPosY > 0.0f;
}

extern u8 gPoogieFollowing;

/*
 * State 0x5C: offline, before the Poogie has a name, it follows the naming
 * talker (other, NPC_NAMING_TALKER) and sets gPoogieFollowing. It picks a spot
 * within 99 of the talker and walks there without wall collision. Its speed
 * eases between 0.5 and 1.2: faster, and not talkable, while more than 150 away.
 * More than 5000 away it is put 150 beside the talker. Within 65 of a spot the
 * talker has left, it picks a new one. With no talker it stands (PET_STATE_IDLE).
 * While the talker talks (state 0x2C) it waits (state 0x5D).
 */
void Pet_StateFollowTalker(Em *pet, Em *talker)
{
    NpcWork *w = (NpcWork *)&pet->work;
    NpcPos spot;
    u16 offset;
    f32 dist;

    if (talker == NULL) {
        Actor_SetState(pet, 0, PET_STATE_IDLE);
        return;
    }
    if (talker->state == 0x2C) {
        pet->motionSpeed = 1.0f;
        Actor_SetState(pet, 0, 0x5D);
        return;
    }
    switch (pet->step) {
    case 0:
        pet->motionSpeed = 0.5f;
        pet->fieldCollide = 0;
        pet->step++;
        if (pet->motionId[0] != 0x3EB) {
            Actor_SetMotionOnLayer(pet, 0x3EB, 4, 0, 0);
        }
        spot = *(NpcPos *)&talker->posX;
        offset = (u16)Rand_Next(1) % 100;
        if (offset & 1) {
            spot.v[0] += offset;
        } else {
            spot.v[0] -= offset;
        }
        offset = (u16)Rand_Next(1) % 100;
        if (offset & 1) {
            spot.v[2] += offset;
        } else {
            spot.v[2] -= offset;
        }
        w->followSpot = spot;
        break;
    case 1:
        gPoogieFollowing = 1;
        pet->targetYaw = Actor_HeadingDiff(pet, w->followSpot.v);
        pet->rotY += (s16)pet->targetYaw / 10;
        dist = Vec_Distance(&pet->posX, w->followSpot.v);
        if (!(dist <= 150.0f)) {
            w->noTalk = 1;
            pet->motionSpeed += 0.7f;
            if (!(pet->motionSpeed <= 1.2f)) {
                pet->motionSpeed = 1.2f;
            }
        } else {
            w->noTalk = 0;
            pet->motionSpeed -= 0.7f;
            if (pet->motionSpeed < 0.5f) {
                pet->motionSpeed = 0.5f;
            }
        }
        if (!(dist <= 5000.0f)) {
            pet->posX = 150.0f + talker->posX;
            pet->posY = talker->posY;
            pet->posZ = talker->posZ;
            pet->rotY = pet->targetYaw;
        } else if (dist < 65.0f && !(Vec_Distance(w->followSpot.v, (f32 *) &talker->posX) <= 65.0f)) {
            pet->step--;
        }
        break;
    }
}

/*
 * The waiting pet's next motion (Pet_StateWait), on a random roll: one time in
 * 16 it sits down for a while (never twice in a row: stateVar1 2 marks a sit
 * just done), three times in 16 it plays one of three idle motions, and
 * otherwise it picks again next frame. stateVar0 tells Pet_StateWait which of
 * these runs.
 */
void Pet_PickWaitMotion(Em *pet)
{
    u16 roll = (u16)Rand_Next(1) & 0x63;

    pet->step++;
    if (roll == 0 && pet->stateVar1 == 2) {
        pet->stateVar0 = 2;
        pet->step = 1;
        return;
    }
    switch (roll) {
    case 0:
        pet->stateVar0 = 0;
        pet->stateVar1 = 0;
        Actor_SetMotionOnLayer(pet, 0x432, 0xE, 0, 0);
        break;
    case 1:
        Actor_SetMotionOnLayer(pet, 0x3F1, 10, 0, 0);
        pet->stateVar0 = 1;
        pet->stateVar1 = 0;
        break;
    case 2:
        Actor_SetMotionOnLayer(pet, 0x3EE, 10, 0, 0);
        pet->stateVar0 = 1;
        pet->stateVar1 = 0;
        break;
    case 3:
        Actor_SetMotionOnLayer(pet, 0x3FC, 10, 0, 0);
        pet->stateVar0 = 1;
        pet->stateVar1 = 0;
        break;
    default:
        pet->stateVar0 = 2;
        pet->step = 1;
        break;
    }
}

extern f32 gNpc4FArea[];

/*
 * State 0x5D: waits.
 * - Following the naming talker (other): with no talker it stands
 *   (PET_STATE_IDLE). Once the talker stops talking it follows the player if the
 *   Poogie has a name (PET_ACT_NONE in AREA_ONLINE_HOUSE_2), else the talker
 *   again (state 0x5C).
 * - Waiting below the player (Em.petWaitBelow): once its motion ends, NPC_HOUSE_PET in
 *   AREA_HUB_0x57 follows again. The Poogie follows again once out of gNpc4FArea,
 *   or once the player is no longer climbing nor above it.
 * Meanwhile it waits 30 frames, then turns to the local player playing motion
 * 0x3EF, then plays idle motions picked by Pet_PickWaitMotion. A sit lasts
 * 100-355 frames, then it gets up (0x431).
 */
void Pet_StateWait(Em *pet, Em *talker)
{
    NpcWork *w = (NpcWork *)&pet->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 *save = (u8 *)&gSave;

    if (pet->petWaitBelow != 1) {
        if (talker == NULL) {
            Actor_SetState(pet, 0, PET_STATE_IDLE);
            return;
        } else if (talker->state != 0x2C) {
            if (pet->step == 1) {
                if ((s8)((SaveData *)save)->poogieName[0] != 0) {
                    Actor_SetState(pet, 0, PET_STATE_FOLLOW);
                    if (gScene.area == AREA_ONLINE_HOUSE_2) {
                        w->petAct = PET_ACT_NONE;
                    }
                } else {
                    Actor_SetState(pet, 0, 0x5C);
                }
                return;
            }
            if (pet->stateVar0 == 0 && pet->stateVar1 == 1) {
                pet->stateVar1++;
            }
        }
    } else {
        if (pet->motion[0].playing <= 0) {
            if (w->id == NPC_HOUSE_PET) {
                if (gScene.area == AREA_HUB_0x57) {
                    Actor_SetState(pet, 0, PET_STATE_FOLLOW);
                    return;
                }
            } else if (Pos_InZone(&pet->posX, gNpc4FArea) == 1) {
                if (Npc_IsRising((Em *)pl) == 0 && pl->pos[1] <= pet->posY) {
                    Actor_SetState(pet, 0, PET_STATE_FOLLOW);
                    return;
                }
            } else {
                Actor_SetState(pet, 0, PET_STATE_FOLLOW);
                return;
            }
        }
    }
    switch (pet->step) {
    case 0:
        w->petCount = 0;
        pet->step++;
        pet->stateVar0 = 0;
        pet->stateVar1 = 0;
        break;
    case 1:
        switch (pet->stateVar0) {
        case 0:
            w->petCount += 1;
            if (w->petCount == 30) {
                pet->motionSpeed = 1.0f;
                Actor_SetMotionOnLayer(pet, 0x3EF, 10, 0, 0);
                pet->stateVar0++;
            }
            break;
        case 1:
            pet->rotY += (s16)(u16)Actor_HeadingDiff(pet, pl->pos) / 10;
            if (pet->motion[0].playing <= 0) {
                pet->step++;
            }
            break;
        case 2:
            Pet_PickWaitMotion(pet);
            break;
        }
        break;
    case 2:
        switch (pet->stateVar0) {
        case 0:
            switch (pet->stateVar1) {
            case 0:
                pet->stateVar1++;
                w->petCount = (u8)Rand_Next(1) + 100;
                Actor_SetMotionOnLayer(pet, 0x432, 14, 0, 0);
                break;
            case 1:
                if (--w->petCount <= 0) {
                    pet->stateVar1++;
                }
                break;
            case 2:
                Actor_SetMotionOnLayer(pet, 0x431, 4, 0, 0);
                pet->stateVar0++;
                break;
            }
            break;
        case 1:
            if (pet->motion[0].playing <= 0) {
                pet->step = 1;
                pet->stateVar0 = 2;
            }
            break;
        }
        break;
    }
}

/* State 0xAB: plays motion 0x3EF over and over. */
void Npc4F_StateAB(Em *pet)
{
    switch (pet->step) {
    case 0:
        Actor_SetMotionOnLayer(pet, 0x3EF, 10, 0, 0);
        pet->step++;
        break;
    case 1:
        if (pet->motion[0].playing <= 0) {
            pet->step = 0;
        }
        break;
    }
}

void Pet_EndDig(Em *arg0);
s32 Pet_IsNearPlayer(Em *obj);
u8 *ActSpot_FindFirstOfKind(u16 area, u16 id);
extern u8 *gPoogieDigSpots;
extern u8 gPoogieDigSpotCount;
extern s16 gPoogieDigTimer;
extern u8 gPoogieDigSpotIdx;
extern u8 gPoogieDigState;
extern u8 gPoogieDigTaken;

/*
 * PET_STATE_DIG: the pet digs (PET_ACT_DIG). After 1800 frames (gPoogieDigTimer),
 * or in an area with no TOWN_SPOT_POOGIE_DIG spot, it gives up: PET_ACT_NONE, and
 * Pet_EndDig once its motion ends. Otherwise, at the start (unless the dig
 * is POOGIE_DIG_DONE), it picks one of the gPoogieDigSpotCount spots of
 * gPoogieDigSpots at random. It writes that spot's position into its route's
 * first point and runs there at double speed (POOGIE_DIG_GOING). Within 100 it
 * plays motion 0x3EF. It stops (POOGIE_DIG_DONE, Pet_EndDig) when the find
 * is taken (gPoogieDigTaken), or once it has arrived and the local player comes
 * within 100.
 */
void Pet_StateDig(Em *pet)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 *spot;
    NpcWork *w = (NpcWork *)&pet->work;
    f32 *target;

    if (gPoogieDigTimer++ >= 1800) {
        if (pet->motion[0].playing <= 0) {
            Pet_EndDig(pet);
        }
        w->petAct = PET_ACT_NONE;
        return;
    }
    w->petAct = PET_ACT_DIG;
    spot = ActSpot_FindFirstOfKind(pl->area, TOWN_SPOT_POOGIE_DIG);
    if (spot == NULL) {
        if (pet->motion[0].playing <= 0) {
            Pet_EndDig(pet);
        }
        w->petAct = PET_ACT_NONE;
        return;
    }
    target = w->route->pos.v; /* the first route point */;
    switch (pet->stateVar0) {
    case 0:
        if (gPoogieDigState != POOGIE_DIG_DONE) {
            if (gPoogieDigSpotCount < 2) {
                gPoogieDigSpotIdx = 0;
            } else {
                gPoogieDigSpotIdx = (u16)Rand_Next(1) % gPoogieDigSpotCount;
            }
        }
        spot += gPoogieDigSpots[gPoogieDigSpotIdx] * 0x18;
        target[0] = ((ActionSpot *)spot)->x;
        target[1] = ((ActionSpot *)spot)->y;
        target[2] = ((ActionSpot *)spot)->z;
        gPoogieDigState = POOGIE_DIG_GOING;
        pet->motionSpeed *= 2.0f;
        Actor_SetMotionOnLayer(pet, 0x3F7, 2, 0, 0);
        pet->stateVar0++;
        break;
    case 1:
        if (gPoogieDigTaken == 1) {
            Pet_EndDig(pet);
            pet->motionSpeed = 1.0f;
            w->petAct = PET_ACT_NONE;
            gPoogieDigState = POOGIE_DIG_DONE;
            pet->stateVar0 = 3;
        }
        pet->targetYaw = (s16)(u16)Actor_HeadingDiff(pet, target) / 10;
        pet->rotY += (s16)pet->targetYaw;
        if (Vec_Distance(&pet->posX, (f32 *) target) < 100.0f) {
            pet->motionSpeed = 1.0f;
            Actor_SetMotionOnLayer(pet, 0x3EF, 10, 0, 0);
            pet->stateVar0++;
        }
        break;
    case 2:
        if (gPoogieDigTaken == 1 || Pet_IsNearPlayer(pet) == 1) {
            Pet_EndDig(pet);
            w->petAct = PET_ACT_NONE;
            gPoogieDigState = POOGIE_DIG_DONE;
            pet->stateVar0++;
        }
        break;
    case 3:
        break;
    }
}

f32 Vec_Length(f32 *v);

/* Whether the local player is within 100 of the pet on the ground plane (x and z only). */
s32 Pet_IsNearPlayer(Em *pet)
{
    f32 delta[3];
    Player *pl = &gPlayers[gScene.localPlayer];

    delta[0] = pet->posX - pl->pos[0];
    delta[1] = 0.0f;
    delta[2] = pet->posZ - pl->pos[2];
    return Vec_Length(delta) <= 100.0f;
}

/*
 * The end of a dig: the Poogie follows the player again (PET_STATE_FOLLOW), the
 * care pet enters its route point's state; other pets are left as they are.
 */
void Pet_EndDig(Em *pet)
{
    NpcWork *w = (NpcWork *)&pet->work;

    if (pet->work == NPC_POOGIE) {
        Actor_SetState(pet, 0, PET_STATE_FOLLOW);
    } else if (pet->work == NPC_POOGIE_CARE) {
        Actor_SetState(pet, 0, *(u16 *)&w->route[w->routeIdx].state);
    }
}

/*
 * The active townsperson with the given id (NpcWork.id), or NULL. The same
 * search as Npc_Find; this unit has its own copy.
 */
Em *Pet_FindNpc(u8 id)
{
    Em *em = gEms;
    u32 i;

    for (i = 0; i < EM_COUNT; i++, em++) {
        if (em->active != 0 && em->work == id) {
            return em;
        }
    }
    return NULL;
}

u8 *ActSpot_GetList(u16 area);

/*
 * The first spot of the given kind (ActionSpot.kind, enum TownSpotKind) in the
 * area's action spot list: the one whose own id (ActionSpot.type) is 0. The 40
 * entries are searched, 0x18 bytes each (an ActionSpot). Returns NULL when there
 * is none. The Poogie's dig spots are counted on from it (gPoogieDigSpots).
 */
u8 *ActSpot_FindFirstOfKind(u16 area, u16 id)
{
    u8 *spots = ActSpot_GetList(area);
    u8 i;
    u16 *entry;

    i = 0;
    entry = (u16 *)spots;
    do {
        if (entry[0] == 0 && id == entry[1]) {
            return spots + i * 0x18;
        }
        i++;
        entry += 12;
    } while (i < 40);
    return NULL;
}

void Pet_StateRunOff(Em *arg0);
void Pet_StateRunRoute(Em *arg0);
void Pet_StateWander(Em *arg0);
void Pet_StateFollow(Em *arg0);
void Pet_StateFollowTalker(Em *arg0, Em *npc);
void Pet_StateWait(Em *arg0, Em *npc);
void Pet_StateDig(Em *arg0);
extern u8 gPoogieFollowing;

/*
 * The pets' handler (looks 0x4F and 0x50) by Em.state; state 0x60 runs
 * nothing. In AREA_HOUSE and AREA_HUB_0x57 the pet's area follows the scene's.
 * The pet can't be talked to while it acts (petAct), nor while the Poogie has no
 * name and has not followed the naming talker yet (gPoogieFollowing).
 * Returns 1.
 */
s32 Pet_RunState(Em *pet)
{
    Em *talker = Pet_FindNpc(NPC_NAMING_TALKER);
    NpcWork *w = (NpcWork *)&pet->work;
    u8 *save = (u8 *)&gSave;

    switch (pet->state) {
    case PET_STATE_IDLE:
        Pet_StateIdle(pet, 0);
        break;
    case 0x59:
        Pet_StateIdle(pet, 1);
        break;
    case 0x52:
        Pet_StateSit(pet, 0);
        break;
    case 0x53:
        Pet_StateSit(pet, 1);
        break;
    case NPC_STATE_HOUSE_OPEN:
        Pet_StateHouseOpen(pet);
        break;
    case PET_STATE_PETTED:
        Pet_StatePetted(pet);
        break;
    case PET_STATE_FACE:
        Pet_StateFace(pet);
        break;
    case PET_STATE_ANGRY:
        Pet_StateAngry(pet);
        break;
    case PET_STATE_JOY:
        Pet_StateJoy(pet);
        break;
    case PET_STATE_GREAT_JOY:
        Pet_StateGreatJoy(pet);
        break;
    case PET_STATE_RUN_OFF:
        Pet_StateRunOff(pet);
        break;
    case 0x57:
        Pet_StateWander(pet);
        break;
    case PET_STATE_FOLLOW:
        Pet_StateFollow(pet);
        break;
    case 0x5C:
        Pet_StateFollowTalker(pet, talker);
        break;
    case 0x5D:
        Pet_StateWait(pet, talker);
        break;
    case 0xAB:
        Npc4F_StateAB(pet);
        break;
    case PET_STATE_DIG:
        Pet_StateDig(pet);
        break;
    case 0xAF:
        Pet_StateRunRoute(pet);
        break;
    case 0x60:
        break;
    }
    if (gScene.area == AREA_HOUSE || gScene.area == AREA_HUB_0x57) {
        pet->area = gScene.area;
    }
    if (w->petAct != PET_ACT_NONE || ((s8)((SaveData *)save)->poogieName[0] == 0 && gPoogieFollowing != 1)) {
        w->noTalk = 1;
    }
    return 1;
}

/*
 * The pets' zones and run-off routes in AREA_HUB_0x89 (Pet_StateRunOff), the zone
 * where the Poogie waits below a climbing player (gNpc4FArea), and the quest
 * desk's frame colours (gDecoFrameColors, read by the quest desk's drawing and
 * the menu sheets).
 */
f32 gNpc4FArea0[8] = {
#include "lobby/007FE4B0.inc"
};
f32 gNpc4FArea1[8] = {
#include "lobby/007FE4D0.inc"
};
f32 gNpc4FArea2[8] = {
#include "lobby/007FE4F0.inc"
};
f32 gNpc4FArea3[8] = {
#include "lobby/007FE510.inc"
};
f32 gNpc4FArea4[8] = {
#include "lobby/007FE530.inc"
};
NpcRouteEnt gNpc4FRoute0[5] = {
#include "lobby/007FE550.inc"
};
NpcRouteEnt gNpc4FRoute1[6] = {
#include "lobby/007FE5C0.inc"
};
NpcRouteEnt gNpc4FRouteElse[4] = {
#include "lobby/007FE640.inc"
};
NpcRouteEnt gNpc4FRoute3[2] = {
#include "lobby/007FE690.inc"
};
NpcRouteEnt gNpc4FRoute2[7] = {
#include "lobby/007FE6C0.inc"
};
NpcRouteEnt gNpc4FRoute4[5] = {
#include "lobby/007FE750.inc"
};
f32 gNpc4FArea[8] = {
#include "lobby/007FE7C0.inc"
};
u32 gDecoFrameColors_0[8] = {
#include "lobby/007FE7E0.inc"
};
u32 gDecoFrameColors_1[8] = {
#include "lobby/007FE800.inc"
};
u32 gDecoFrameColors_2[6] = {
#include "lobby/007FE820.inc"
};
u32 gDecoFrameColors[3] = {
    (u32)gDecoFrameColors_0, (u32)gDecoFrameColors_1, (u32)gDecoFrameColors_2,
};
