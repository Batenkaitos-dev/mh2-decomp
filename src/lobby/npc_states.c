/*
 * The townspeople's state handlers: what each townsperson of the town and
 * the village does in each actor state (Em.state, set by Actor_SetState).
 * The townspeople are Em slots whose work block is NpcWork (Em.work), and
 * Npc_UpdateState picks the handler set by the townsperson's variant
 * (Em.variant): Npc_RunState for most of them (its states are
 * enum NpcState's NPC_STATE_*), ServiceNpc_RunState for the
 * house kitchen's staff (variants 81-85), Pet_RunState for the pets
 * (variants 79 and 80: the Poogie, the care pet, the house pet).
 *
 * A townsperson walks a route (NpcWork.route, NpcRouteEnt): at each point
 * it enters the point's state, whose handler plays its motions and moves
 * on to the next point when it is done (Npc_NextRoutePoint). The file holds,
 * in order:
 * - the state update each frame (Npc_UpdateState): the handler set by
 *   variant, the move and turn, the wait at the route point and the step to
 *   the next, and the route checks by time and side; then the route's small
 *   steps (Npc_HoldRoute, Npc_EnterRoutePoint, Npc_HideAndAdvance);
 * - the shared steps: the next route point, the turns toward the player and
 *   back, and the swaps of a pair of townspeople by the side of a line the
 *   player is on (Npc_PairIdBySide, Npc_CheckSideSwap);
 * - the idles by kind (Npc_Idle*) and the common states: standing, walking,
 *   running, the talk (Npc_StateTalk) and the state each kind takes when the
 *   player talks to it (Npc_StartTalkState), the seats and poses, the pairs
 *   that stand together, the townspeople who greet a passing player;
 * - the smithy: the smith's work, forging and talk, with its helper and its
 *   partner, and its moves between the smithy's two levels;
 * - the other townspeople's states and Npc_RunState, their dispatcher.
 *
 * The townspeople's set-up, task and drawing (Npc_Init, Npc_Update)
 * and the kind tests (Npc_IsMusicActor, Npc_IsFree ...) come before this file;
 * the service townspeople's states (ServiceNpc_RunState) and the pets'
 * (Pet_RunState) follow it.
 */
#include "common.h"
#include "lobby_npc.h"
#include "npc_motion.h"
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
#include "capsule.h"
#include "village_cond.h"
#include "bone_effects.h"
#include "engine_math.h"
#include "gallery_objects.h"
#include "game_util.h"
#include "lobby_queries.h"
#include "lobby_world.h"
#include "pet_states.h"
#include "rand.h"
#include "service_npc_states.h"
#include "town_area_change.h"
#include "libc.h"

/* Prototypes shared by the parts of this unit. */
void Actor_SetPosture(void *a, u16 mode, s32 b, s32 c);
/* unit's own declaration: a u16 state (the definition takes s32): the callers here pass the state as a halfword, and Npc_NextRoutePoint compiles as the game's code only with this type */
void Actor_SetState(void *obj, s32 group, u16 state);
void Npc_StartMoveTo(Em *obj, s32 n);
u8 Village_FacilityLevel(s32 k);

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
extern s32 gTownMode;

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
void Npc_StepRouteMove(Em *obj);

/* A townsperson's state update for this frame (Npc_Run calls it). Unless it is the
 * music actor while the lobby music plays (Npc_IsMusicActor, LobbyBgm_IsActive), it
 * clears NpcWork.noTalk, runs the online workshop lesson's step for it
 * (NpcScene_RunNpc) and its state handler by look (ServiceNpc_RunState for 81-85,
 * Pet_RunState for the pets, else Npc_RunState), then its move and turn. On a
 * route, outside the states that keep it where it is and while no menu or contest
 * holds it (the drinking contest's looks 49-54, NPC_CONTEST_HOST, the three
 * pets) and its route isn't held, it counts NpcWork.wait at the route point: the
 * point's argument is the frames to wait, -1 to stay (with NPC_STATE_PAIR_WAIT when
 * Npc_CheckHubPartner answers 1), -2 to wait for the motion to end; then
 * Npc_NextRoutePoint. Last come the route checks by time (only when flag ends 0:
 * it stays, or Npc_NextRoutePoint answered 0) and by side; NPC_KITCHEN_HELPER can
 * never be talked to. Returns the state handler's result.
 *
 * The game's own bug: on the music actor's path the function jumps to the end with
 * flag and ret never set. The game keeps them in the registers s0 and s1, which
 * this function saves on entry, so it tests and returns the caller's values of s0
 * and s1; a native port reads the same two values. */
s32 Npc_UpdateState(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    s32 ret;
    s32 flag;
    s32 *argPtr;
    s32 waitArg;

    if (Npc_IsMusicActor(npc) == 1 && LobbyBgm_IsActive() != 0) {
        goto end;
    }
    npcWork->noTalk = 0;
    NpcScene_RunNpc(npc);
    switch (npc->variant) {
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
        ret = ServiceNpc_RunState(npc);
        break;
    case 79:
    case 80:
        ret = Pet_RunState(npc);
        break;
    default:
        ret = Npc_RunState(npc);
        break;
    }
    Npc_StepRouteMove(npc);
    Npc_StepTurn(npc);
    flag = 1;
    if (npcWork->route == NULL) {
        goto end;
    }
    /* the states that keep the townsperson where it is (among them the pets'
     * PET_STATE_* and NPC_STATE_HOUSE_OPEN): no wait at the route point */
    switch (npc->state) {
    case 3:
    case 2:
    case 4:
    case 142:
    case 44:
    case 66:
    case 47:
    case 90:
    case 122:
    case 129:
    case 130:
    case 147:
    case 84:
    case 85:
    case 94:
    case 86:
    case 82:
    case 83:
    case 87:
    case 88:
    case 92:
    case 93:
    case 91:
    case 163:
    case 108:
    case 95:
    case 96:
    case 153:
    case 155:
    case 151:
    case 164:
    case 165:
    case 176:
    case 182:
        flag = 1;
        break;
    default:
        if (npcWork->holdRoute == 0) {
            if (Npc_IsType31to36(npc) == 1 && gTownMode == TOWN_MODE_DRINK_CONTEST) {
                flag = 1;
                break;
            }
            if (npcWork->id == NPC_CONTEST_HOST && gTownMode == TOWN_MODE_CONTEST) {
                flag = 1;
                break;
            }
            if (npcWork->id == NPC_POOGIE && gTownMode == TOWN_MODE_POOGIE_MENU) {
                flag = 1;
                break;
            }
            if (npcWork->id == NPC_POOGIE_CARE && (gTownMode == TOWN_MODE_POOGIE_CARE || npc->state == PET_STATE_DIG)) {
                flag = 1;
                break;
            }
            if (npcWork->id == NPC_HOUSE_PET && gTownMode == TOWN_MODE_HOUSE_PET) {
                flag = 1;
                break;
            }
            argPtr = &npcWork->route[npcWork->routeIdx].arg;
            waitArg = *argPtr;
            if (waitArg == -1) {
                if (Npc_CheckHubPartner(npc) == 1) {
                    Actor_SetState(npc, 0, NPC_STATE_PAIR_WAIT);
                }
                flag = 0;
                break;
            }
            if (waitArg == -2) {
                if (++npcWork->wait > 2 && npc->motion[0].playing <= 0) {
                    flag = (s8)Npc_NextRoutePoint(npc);
                }
                break;
            }
            if (++npcWork->wait >= waitArg) {
                flag = (s8)Npc_NextRoutePoint(npc);
            }
        } else {
            /* a held route (NpcWork.holdRoute): flag is already 1. The store is
             * written out as in the other branches: the compiler removes it only
             * after laying out the branches, which leaves the jump to end that the
             * game's code has here. */
            flag = 1;
        }
        break;
    }
end:
    if (flag == 0) {
        Npc_CheckTimeRoute(npc, npcWork);
    }
    Npc_CheckSideSwap(npc, npcWork);
    if (npcWork->id == NPC_KITCHEN_HELPER) {
        npcWork->noTalk = 1;
    }
    return ret;
}

/* Keeps the townsperson at its route point: Npc_UpdateState stops counting the
 * wait until Npc_EnterRoutePoint releases it. */
void Npc_HoldRoute(Em *npc)
{
    ((NpcWork *)&npc->work)->holdRoute = 1;
}

/* Releases a held route and enters the state of the townsperson's current route
 * point. The state is read as its low halfword, as the game's code reads it. */
void Npc_EnterRoutePoint(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    npcWork->holdRoute = 0;
    Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
}

/* Hides the townsperson and moves its task to the next phase (from running to
 * freeing: Npc_Update frees its slot next). */
void Npc_HideAndAdvance(Em *npc)
{
    npc->taskPhase = (u8) (npc->taskPhase + 1);
    npc->visible = 0;
}

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
            Npc_StartMoveTo(npc, 20);
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
 * frames. A turn of more than 10 (ANGLE_TURN is a full turn) first keeps the
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

/* Starts turning the townsperson to the heading angle (ANGLE_TURN is a full
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

/* For townsperson 2 in AREA_HUB_0x89: 1 when townsperson 1 is within 200 and
 * standing with it (NPC_STATE_PAIR_STAND or NPC_STATE_PAIR_STAND_1,
 * Npc_StatePairStand), 2 when it is within 200 doing anything else (a talk),
 * else 0; 0 for any other townsperson or area. */
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
 * of its line the player is on (kinds 8 and NPC_WORKSHOP_SMITH need facility 2). */
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

/* Swaps a townsperson of a side pair (gNpcSidePairs) for its partner:
 * when its presence rule (unk48) is 4 and the side of the line the local
 * player is on calls for the partner (Npc_PairIdBySide), it leaves
 * (NPC_STATE_GONE, unk49 set) and partnerId keeps the partner's id. Skipped
 * offline in the house, during a talk or a leave, and for the player's
 * lock-on target. */
void Npc_CheckSideSwap(Em *npc, NpcWork *work)
{
    u8 partnerId;

    if (Game_IsOnline() == 0 && Area_IsHouse(gScene.area) == 1) {
        return;
    }
    switch (npc->state) {
    case 44:
    case 66:
    case 95:
    case 96:
        return;
    }
    if (work->leaving == 0 && npc != gPlayers[gScene.localPlayer].target) {
        switch (work->presence) {
        case NPC_PRESENCE_SIDE:
            partnerId = Npc_PairIdBySide(work->id);
            if (work->id != partnerId) {
                npc->state = 96;
                work->leaving = 1;
                work->partnerId = partnerId;
            }
            return;
        }
    }
}

/* Re-picks the townsperson's route for the time of day
 * (Npc_SetRouteByKind), except offline in the house (where only
 * townsperson 103 goes on), during a talk or a leave, once it is leaving
 * (unk49), and while the player talks to it. The checks of the presence
 * rule (unk48) after the call change nothing. */
void Npc_CheckTimeRoute(Em *npc, NpcWork *work)
{
    if (Game_IsOnline() == 0 && work->id != 103 && Area_IsHouse(gScene.area) == 1) {
        return;
    }
    switch (npc->state) {
    case 44:
    case 66:
    case 95:
    case 96:
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

/* The workshop helper's idle (NPC_WORKSHOP_HELPER): it strikes along with the
 * smith (NPC_WORKSHOP_SMITH). When the smith's forging motion
 * (STATIONED_MOT_FORGE_START) passes frame 44 it plays its own strike
 * (STATIONED_MOT_FORGE_START, STATIONED_MOT_FORGE_HAMMER twice,
 * STATIONED_MOT_FORGE_HELPER_TURN, STATIONED_MOT_FORGE_LAST; motionPhase marks
 * frames 30 to 118 of it), then stands (STATIONED_MOT_SMITH_IDLE) and waits for
 * the next one. Without the smith it stops after its first step. */
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
        if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 20, 0, 0);
        }
        break;
    case 1:
        if (smith->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_FORGE_START) && Motion_ReachedFrame(smith, 0, 44.0f) != 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_START), 6, 0, 0);
            npc->step++;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step = step + 1;
            npc->timer = 2;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_HAMMER), 4, 0, 0);
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
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_HELPER_TURN), 0, 0, 0);
            npc->step++;
        }
        break;
    case 5:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_LAST), 0, 20, 0);
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
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 16, 0, 0);
        }
        break;
    }
}

/* Whether townsperson 6 is walking by: present, the port
 * (VILLAGE_FACILITY_PORT) not at level 2, and neither running (state 4)
 * nor standing (motion PL_MOT_IDLE or 658). The contest host's idle
 * calls to it then (Npc_IdleContestHost). Returns 1 or 0. */
s32 Npc_IsHostCalleeWalking(void)
{
    Em *em = Npc_Find(6);

    if (em == NULL) {
        return 0;
    }
    if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2 || em->state == 4 || em->motionId[0] == 658 || em->motionId[0] == PL_MOT_IDLE) {
        return 0;
    }
    return 1;
}

/* The contest host's idle (NPC_CONTEST_HOST): it alternates its standing
 * motions 881 and 880, and while townsperson 6 walks by
 * (Npc_IsHostCalleeWalking) calls to it (882) half the time. After a motion
 * ends it picks the next one with a 30% chance a frame. */
void Npc_IdleContestHost(Em *npc)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        if (Npc_IsHostCalleeWalking() == 1 && ((u16)Rand_Next(1) & 1) && npc->motionId[0] != 882) {
            Actor_SetMotion(npc, 882, 0, 0);
            break;
        }
        if (npc->motionId[0] == 881) {
            Actor_SetMotion(npc, 880, 0, 0);
            break;
        }
        Actor_SetMotion(npc, 881, 0, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && (u16)Rand_Next(1) % 100 <= 29) {
            npc->step = 0;
        }
        break;
    }
}

/* The pair 105/107: each stays near the other (within 400) and plays
 * STATIONED_MOT_TALK_START/STATIONED_MOT_TALK in turn; farther away it plays
 * STATIONED_MOT_IDLE. */
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
        case 105:
            partner = Npc_Find(107);
            goto near;
        case 107:
            partner = Npc_Find(105);
        near:
            if (partner != NULL) {
                if (Vec_Distance(&npc->posX, (f32 *) &partner->posX) <= 400.0f) {
                    if (partner->state == NPC_STATE_TALK_DELAY || partner->state == NPC_STATE_TALK) {
                        Npc_StartTalkState(npc);
                        return;
                    }
                    switch (npc->motionId[0]) {
                    case EM_MOTION_ID(0, STATIONED_MOT_TALK_START):
                        if (npc->motion[0].playing == 0) {
                            npc->timer = (u16)Rand_Next(1) % 60 + 90;
                            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_TALK), 4, 0, 0);
                        }
                        break;
                    case EM_MOTION_ID(0, STATIONED_MOT_TALK):
                        if (--npc->timer <= 0) {
                            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_TALK_START), 4, 0, 0);
                        }
                        break;
                    default:
                        npc->timer = (u16)Rand_Next(1) % 60 + 90;
                        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_TALK), 4, 0, 0);
                        break;
                    }
                } else if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_IDLE)) {
                    Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 4, 0, 0);
                }
            } else if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_IDLE)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 4, 0, 0);
            }
            break;
        }
        break;
    }
}

/* The idle of kind 77 (Npc_IsType77): stands (1001) for 4 or 5 loops,
 * then plays 1002 once. It keeps facing during a talk (keepFacing). */
void Npc_Idle4D(Em *npc)
{
    ((NpcWork *)&npc->work)->keepFacing = 1;
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 1001) {
            Actor_SetMotionOnLayer(npc, 1001, 4, 0, 0);
            npc->timer = (u16)Rand_Next(1) % 2 + 4;
        }
        npc->step++;
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            if (--npc->timer <= 0) {
                Actor_SetMotionOnLayer(npc, 1002, 4, 0, 0);
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

/* The idle of kinds 60-62 (Npc_IsType3Cto3E): 690 or 694, picked
 * by chance as each loop ends. */
void Npc_Idle3C(Em *npc)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        Actor_SetMotion(npc, 690, 4, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            if ((u16)Rand_Next(1) & 1) {
                Actor_SetMotion(npc, 690, 4, 0);
            } else {
                Actor_SetMotion(npc, 694, 8, 0);
            }
        }
        break;
    }
}

/* The idle of kind 75 (Npc_IsType4B): 253 or 254, picked by chance as
 * each loop ends (253, blended over 20 frames, after 658). */
void Npc_Idle4B(Em *npc)
{
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] == 658) {
            Actor_SetMotion(npc, 253, 20, 0);
        } else if ((u16)Rand_Next(1) & 1) {
            if (npc->motionId[0] != 253) {
                Actor_SetMotion(npc, 253, 4, 0);
            }
        } else if (npc->motionId[0] != 254) {
            Actor_SetMotion(npc, 254, 4, 0);
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

/* The idle of kind 78, the townsperson the lobby music's scripts move
 * (LobbyBgm_SetActor; Npc_UpdateState skips it while that music plays):
 * between tunes it plays its motions 1002 to 1012 in turn, then starts
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
            Actor_SetMotion(npc, 1002, 0, 0);
            npc->step = (u8) (npc->step + 1);
            break;
        }
    default:
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 1003, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 1004, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 1005, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 5:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 1006, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 6:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 1007, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 7:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 1009, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 8:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 1010, 0, 0);
            npc->step = (u8) (npc->step + 1);
        }
        break;
    case 9:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 1012, 0, 0);
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

/* NPC_STATE_IDLE, standing: the kind's own idle (Npc_Idle*), or the standing
 * motion by kind (PL_MOT_IDLE for most, PL_MOT_IDLE_ALT, the kinds' own),
 * with negative blend frames while a move is still under way (moveCount).
 * The greeters (Npc_IsFreeId0Aor2E) start their greeting (NPC_STATE_GREET)
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
    } else if (Npc_IsType77(npc) == 1) {
        Npc_Idle4D(npc);
    } else if (Npc_IsType3Cto3E(npc) == 1) {
        Npc_Idle3C(npc);
    } else if (Npc_IsMusicActor(npc) == 1) {
        Npc_IdleMusicActor(npc);
    } else if (npcWork->id == 105) {
        Npc_IdlePaired(npc);
    } else if (npcWork->id == 107) {
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
                Actor_SetMotion(npc, 858, 6, 0);
            } else if (Npc_IsType40(npc) == 1) {
                Actor_SetMotion(npc, 868, 4, 0);
                if (npcWork->id == 47 || npcWork->id == 32) {
                    npcWork->keepFacing = 1;
                }
            } else if (Npc_IsFreeAct29or2A(npc) == 1) {
                if (Npc_IsFreeId0Aor2E(npc) == 1) {
                    Actor_SetMotion(npc, 683, blendSign * 14, 0);
                } else {
                    Actor_SetMotion(npc, 680, blendSign * 4, 0);
                }
            } else if (Npc_IsFreeAct15to19(npc) == 1) {
                Actor_SetMotion(npc, 675, blendSign * 16, 0);
            } else if (Npc_IsFreeAct18Set(npc) == 1) {
                Actor_SetMotion(npc, PL_MOT_IDLE_ALT, blendSign * 16, 0);
            } else if (Npc_HasSecondHead(npc)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE), blendSign * 16, 0, 0);
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

/* Puts a townsperson the player talks to into its talk state: the
 * smith's (NPC_STATE_SMITH_TALK), the greeters' (NPC_STATE_GREET_TALK),
 * kind 64 when running (NPC_STATE_CALL_OVER), kinds 43-48 (state 47),
 * kinds 60-62 (state 64); else by its current state: the work states' own
 * talks (NPC_STATE_BUSY_IDLE, NPC_STATE_BUSY_TALK and
 * NPC_STATE_BUSY_TALK_HOLD to NPC_STATE_BUSY_TALK; NPC_STATE_BUSY_POSE and
 * NPC_STATE_BUSY_POSE_TALK to NPC_STATE_BUSY_POSE_TALK; NPC_STATE_BUSY_LOOP
 * and NPC_STATE_BUSY_LOOP_TALK to NPC_STATE_BUSY_LOOP_TALK; state 146 to
 * NPC_STATE_WAIT_TOWN_FREE), or the common talk, NPC_STATE_TALK (from
 * NPC_STATE_REACT_NEAR only once its motion 869 is over). TownPc_HandleInput,
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
        Actor_SetState(npc, 0, 47);
        return;
    }
    if (Npc_IsType3Cto3E(npc) == 1) {
        Actor_SetState(npc, 0, 64);
        return;
    }
    switch (npc->state) {
    case 143:
        if (npc->motionId[0] != 869) {
            Actor_SetState(npc, 0, NPC_STATE_TALK);
        }
        break;
    case 146:
        Actor_SetState(npc, 0, NPC_STATE_WAIT_TOWN_FREE);
        break;
    case 48:
    case 49:
    case 162:
        Actor_SetState(npc, 0, NPC_STATE_BUSY_TALK);
        break;
    case 14:
    case 50:
        Actor_SetState(npc, 0, NPC_STATE_BUSY_POSE_TALK);
        break;
    case 51:
    case 52:
        Actor_SetState(npc, 0, NPC_STATE_BUSY_LOOP_TALK);
        break;
    default:
        Actor_SetState(npc, 0, NPC_STATE_TALK);
        break;
    }
}

/* NPC_STATE_WALK, walking to the route point: the walking motion by kind
 * (PL_MOT_WALK for most), a tenth of the heading difference turned a
 * frame, the next route point once within 30. Townsperson 6 stops to
 * answer the contest host's call (its motion 882 between frames 26 and
 * 60, within 800: state 163). */
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
            if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_NAMING_TALKER_WALK)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_NAMING_TALKER_WALK), 4, 0, 0);
            }
        } else if (npc->variant == 59) {
            if ((u8)Village_FacilityLevel(VILLAGE_FACILITY_PORT) == 2) {
                if (npc->motionId[0] != PL_MOT_WALK) {
                    Actor_SetMotion(npc, PL_MOT_WALK, 4, 0);
                }
            } else if (npc->motionId[0] == 899) {
                Actor_SetMotion(npc, 898, 20, 0);
            } else if (npc->motionId[0] != 898) {
                Actor_SetMotion(npc, 898, 4, 0);
            }
        } else if (Npc_IsType31to36(npc)) {
            if (npc->motionId[0] == 859) {
                Actor_SetMotion(npc, 865, 6, 0);
            } else if (npc->motionId[0] != 865) {
                Actor_SetMotion(npc, 865, 4, 0);
            }
        } else if (Npc_IsType40(npc) == 1) {
            if (npc->motionId[0] != 872) {
                Actor_SetMotion(npc, 872, 0, 0);
            }
        } else if (Npc_IsFreeAct15to19(npc) == 1) {
            if (npc->motionId[0] != 676) {
                Actor_SetMotion(npc, 676, 4, 0);
            }
        } else if (Npc_IsFreeAct18Set(npc) == 1) {
            if (npc->motionId[0] != 64) {
                Actor_SetMotion(npc, 64, 4, 0);
            }
        } else if (Npc_HasSecondHead(npc) == 1) {
            if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_WALK)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_WALK), 4, 0, 0);
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
                if (host->motionId[0] == 882 && dist < 800.0f && Motion_ReachedFrame(host, 0, 26.0f) &&
                    !Motion_ReachedFrame(host, 0, 60.0f)) {
                    Actor_SetState(npc, 0, 163);
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

/* NPC_STATE_RUN, running to the route point (PL_MOT_RUN for most), as
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
            if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_RUN)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_RUN), 4, 0, 0);
            }
        } else if (Npc_IsType40(npc) == 1) {
            if (npc->motionId[0] != 873) {
                Actor_SetMotion(npc, 873, 20, 0);
            }
        } else if (npc->motionId[0] != PL_MOT_RUN) {
            Actor_SetMotion(npc, PL_MOT_RUN, 20, 0);
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

/* NPC_STATE_SEAT_POSE_A and NPC_STATE_SEAT_POSE_A_1 (variant 0, 1): sits
 * (PL_POSTURE_CROUCHED) in one of two seated poses, 670 or 672, and keeps
 * facing during a talk. */
void Npc_StateSeatPoseA(Em *npc, s8 variant)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npcWork->keepFacing = 1;
        npc->step++;
        Actor_SetPosture(npc, PL_POSTURE_CROUCHED, 0, 0);
        if (variant == 1) {
            if (npc->motionId[0] != 672) {
                Actor_SetMotion(npc, 672, 60, 0);
            }
        } else {
            if (npc->motionId[0] != 670) {
                Actor_SetMotion(npc, 670, 60, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

extern NpcPair gNpcPartnerPairs[];

/* NPC_STATE_PAIR_STAND to NPC_STATE_PAIR_STAND_2 and NPC_STATE_PAIR_STAND_3
 * (variant 0-3): stands with a partner, in one of four poses (651 or 696 for
 * some kinds). Townspeople 96 and 97 face each other within 200 and idle
 * apart. In AREA_HUB_0x89 and AREA_HUB_0x57 a member of a pair of
 * gNpcPartnerPairs (ids by twos, 255 ends) joins its partner's talk
 * (Npc_StartTalkState) when the partner, within 200, is talked to
 * (NPC_STATE_TALK or NPC_STATE_TALK_DELAY). */
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
            if (npc->motionId[0] != 651) {
                Actor_SetMotion(npc, 651, 8, 0);
            }
        } else if (Npc_IsFreeAct15to19(npc) == 1) {
            Actor_SetMotion(npc, 696, 10, 0);
        } else {
            switch (variant) {
            case 0:
                Actor_SetMotion(npc, 657, 10, 0);
                break;
            case 1:
                Actor_SetMotion(npc, 656, 10, 0);
                break;
            case 2:
                Actor_SetMotion(npc, 658, 12, 0);
                break;
            case 3:
                Actor_SetMotion(npc, 859, 8, 0);
                break;
            }
        }
        break;
    case 1:
        switch (npcWork->id) {
        case 96:
            partner = Npc_Find(97);
            if (200.0f <= Vec_Distance(&npc->posX, &partner->posX)) {
                if (npc->motionId[0] != PL_MOT_IDLE) {
                    Actor_SetMotion(npc, PL_MOT_IDLE, 4, 0);
                }
            } else if (npc->motionId[0] != 656) {
                Actor_SetMotion(npc, 656, 4, 0);
            }
            break;
        case 97:
            partner = Npc_Find(96);
            if (200.0f <= Vec_Distance(&npc->posX, &partner->posX)) {
                if (npc->motionId[0] != PL_MOT_IDLE_ALT) {
                    Actor_SetMotion(npc, PL_MOT_IDLE_ALT, 4, 0);
                }
            } else if (npc->motionId[0] != 657) {
                Actor_SetMotion(npc, 657, 4, 0);
            }
            break;
        }
        if (gScene.area == AREA_HUB_0x89 || gScene.area == AREA_HUB_0x57) {
            if (Npc_IsFreeAct2Bto30(npc) == 1 && npc->motion[0].playing == 0 && npc->motionId[0] == 651) {
                Actor_SetMotion(npc, 652, 2, 0);
            }
            for (; pair->first != 255; pair++) {
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

/* NPC_STATE_WARP_TO_ROUTE: puts the townsperson on its route point at once,
 * then standing (NPC_STATE_IDLE). */
void Npc_StateWarpToRoute(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (npcWork->route == NULL) {
        Actor_SetState(npc, 0, 1);
    }
    *(NpcPos *)(&npc->posX) = *(NpcPos *)&npcWork->route[npcWork->routeIdx].pos;
    Actor_SetState(npc, 0, 1);
}

/* NPC_STATE_SEAT_POSE_B to NPC_STATE_SEAT_POSE_B_3 (variant 0, 1, 3): sits
 * (PL_POSTURE_CROUCHED) in one of three seated poses, 666, 668 or 616, and
 * keeps facing during a talk. */
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
            if (npc->motionId[0] != 668) {
                Actor_SetMotion(npc, 668, 60, 0);
            }
        } else if (variant == 0) {
            if (npc->motionId[0] != 666) {
                Actor_SetMotion(npc, 666, 60, 0);
            }
        } else {
            if (npc->motionId[0] != 616) {
                Actor_SetMotion(npc, 616, 0, 124);
            }
        }
        break;
    case 1:
        break;
    }
}

/* A townsperson's step machine (npc[5]): turn toward the player and pick the
 * motion by kind and current motion (Actor_SetMotion/Actor_SetMotionOnLayer),
 * walk, then the next route point. The game's empty delay slots come from the
 * default-first switch on Npc_HasSecondHead's result. */
void Npc_StateTalk(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    NpcRouteEnt *tbl;

    switch (npc->motionId[0]) {
    case 677:
    case 678:
    case 645:
    case 616:
        Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        break;
    default:
        switch (npc->step) {
        case 0:
            npcWork->noTalk = 1;
            if ((s8)npcWork->keepFacing != 0 || npc->stance == 1) {
                npc->timer = 0;
            } else if (npc->motionId[0] != 874) {
                npc->timer = 10;
                npc->targetYaw = Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos);
            }
            npc->step++;
            if (Npc_IsType49(npc) != 0) {
                if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_NAMING_TALKER_TALK_START)) {
                    return;
                }
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_NAMING_TALKER_TALK_START), 8, 0, 0);
                return;
            }
            switch (Npc_HasSecondHead(npc)) {
            default:
                if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_IDLE_PM)) {
                    Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE_PM_BREAK), 6, 0, 0);
                    return;
                }
                if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_TALK_START)) {
                    return;
                }
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_TALK_START), 16, 0, 0);
                return;
            case 0:
                break;
            }
            if (npcWork->id == NPC_FISHING_SPOT) {
                Actor_SetMotion(npc, 659, 20, 0);
                return;
            }
            if (Npc_IsType31to36(npc) == 1) {
                Actor_SetMotion(npc, 859, 8, 0);
                return;
            }
            if (Npc_IsType77(npc) == 1) {
                Actor_SetMotionOnLayer(npc, 1003, 6, 0, 0);
                return;
            }
            if (Npc_IsType40(npc) == 1) {
                Actor_SetMotion(npc, 871, 4, 0);
                return;
            }
            if (npc->variant == 57) {
                Actor_SetMotion(npc, 854, 40, 0);
                return;
            }
            if (npcWork->id == NPC_CONTEST_HOST) {
                Actor_SetMotion(npc, 889, 0, 0);
                return;
            }
            if (npcWork->id == NPC_DRINK_CONTEST_HOST) {
                Actor_SetMotion(npc, 859, 0, 0);
                return;
            }
            if (npcWork->id == NPC_OFFICIAL_DESK_HIGH_HR && ElderRaid_IsOn() == 1) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_RAID_WAIT), 30, 0, 0);
                return;
            }
                switch (npc->motionId[0]) {
                case 671:
                case 696:
                case 673:
                case 667:
                case 669:
                case 663:
                case 665:
                case 661:
                case 658:
                case 641:
                case 646:
                case 649:
                case EM_MOTION_ID(0, STATIONED_MOT_RAID_WAIT):
                    break;
                case 670:
                    Actor_SetMotion(npc, 671, 10, 0);
                    return;
                case 899:
                case 898:
                    Actor_SetMotion(npc, 252, 4, 0);
                    return;
                case 675:
                case 676:
                    Actor_SetMotion(npc, 696, 16, 0);
                    return;
                case 672:
                    Actor_SetMotion(npc, 673, 10, 0);
                    return;
                case 666:
                    Actor_SetMotion(npc, 667, 10, 0);
                    return;
                case 668:
                    Actor_SetMotion(npc, 669, 10, 0);
                    return;
                case 662:
                    Actor_SetMotion(npc, 663, 10, 0);
                    return;
                case 664:
                    Actor_SetMotion(npc, 665, 10, 0);
                    return;
                case 660:
                case 659:
                    Actor_SetMotion(npc, 661, 10, 0);
                    return;
                case 638:
                    Actor_SetMotion(npc, 639, 4, 0);
                    return;
                case 644:
                    Actor_SetMotion(npc, 646, 16, 0);
                    return;
                case 611:
                    Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 16, 0);
                    return;
                case 609:
                    Actor_SetMotion(npc, 649, 0, 0);
                    return;
                default:
                    if (npc->stance == 0 && npc->motionId[0] < 1000) {
                        Actor_SetMotion(npc, 658, 12, 0);
                        return;
                    }
                    if (npcWork->id == 75) {
                        return;
                    }
                    Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_TALK_START), 16, 0, 0);
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
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_NAMING_TALKER_TALK), 8, 0, 0);
            } else if (Npc_HasSecondHead(npc) != 0 && npc->motion[0].playing <= 0) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_TALK), 8, 0, 0);
            } else if (npc->motionId[0] == PL_MOT_TOWN_SEATED && Motion_ReachedFrame(npc, 0, 5.0f) != 0) {
                Actor_SetMotion(npc, 649, 0, 0);
            }
            if (gTownMode == TOWN_MODE_FREE) {
                npc->timer = 16;
                npc->step++;
                if (npcWork->id == NPC_FISHING_SPOT || npcWork->id == 41) {
                    npc->timer = 10;
                }
            }
            break;
        case 3:
            npcWork->noTalk = 1;
            switch (npcWork->id) {
            case 41:
            case NPC_FISHING_SPOT:
            case 30:
            case 42:
            case 48:
            case NPC_FISHING_SPOT_ONLINE:
                npcWork->noTalk = 1;
                break;
            }
            if (npc->motionId[0] == 646) {
                if (Motion_ReachedFrame(npc, 0, 260.0f) != 0) {
                    Actor_SetMotion(npc, 644, 18, 0);
                }
                break;
            }
            if (--npc->timer <= 0) {
                if ((s8)npcWork->keepFacing != 0) {
                    npc->timer = 0;
                    if (Npc_IsType77(npc) == 1) {
                        Actor_SetMotionOnLayer(npc, 1004, 14, 0, 0);
                    } else {
                        switch (npc->motionId[0]) {
                        case 639:
                            Actor_SetMotion(npc, 641, 10, 0);
                            break;
                        }
                    }
                } else {
                    npc->timer = 10;
                    if (npcWork->id != 1 && Npc_IsType4B(npc) != 1) {
                        if (npcWork->id == NPC_WORKSHOP_HELPER) {
                            npc->timer = 10;
                            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 20, 0, 0);
                        } else if (Npc_IsType49(npc) != 0 || Npc_HasSecondHead(npc) != 0) {
                            if (npcWork->id != NPC_OFFICIAL_DESK_HIGH_HR) {
                                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 10, 0, 0);
                            }
                        } else {
                            switch (npc->motionId[0]) {
                            case 661:
                            case EM_MOTION_ID(0, STATIONED_MOT_RAID_WAIT):
                                npcWork->noTalk = 1;
                                break;
                            case 696:
                                Actor_SetMotion(npc, 675, 10, 0);
                                break;
                            case 854:
                            case 252:
                            case 889:
                                break;
                            case 859:
                                Actor_SetMotion(npc, 860, 20, 0);
                                break;
                            case 871:
                                Actor_SetMotion(npc, 868, 6, 0);
                                break;
                            default:
                                if (npc->state != NPC_STATE_SEATED_AT_TABLE && npc->state != NPC_STATE_SEATED && npc->state != NPC_STATE_SIT_DOWN) {
                                    Actor_SetMotion(npc, PL_MOT_IDLE, 10, 0);
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
                case 860:
                case 1004:
                    return;
                }
            }
            switch (npcWork->id) {
            case 41:
            case NPC_FISHING_SPOT:
            case 30:
            case 42:
            case 48:
            case NPC_FISHING_SPOT_ONLINE:
            case 96:
            case 105:
            case 97:
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

/* States 70 and 71 (variant 0, 1): holds pose 659 or 660; the
 * talk answers from it with 661. */
void Npc_State70(Em *npc, s8 variant)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        switch (variant) {
        case 0:
            if (npc->motionId[0] != 659) {
                Actor_SetMotion(npc, 659, 20, 0);
            }
            break;
        case 1:
            if (npc->motionId[0] != 660) {
                Actor_SetMotion(npc, 660, 20, 0);
            }
            break;
        }
        break;
    }
}

/* States 58 and 59 (variant 0, 1): holds pose 662 or 664 and
 * keeps facing during a talk; in state 58 it can't be talked to for its
 * first 20 frames. */
void Npc_State58(Em *npc, s8 variant)
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
            if (npc->motionId[0] != 662) {
                npc->timer = 20;
                npcWork->noTalk = 1;
                Actor_SetMotion(npc, 662, 20, 0);
            }
            break;
        case 1:
            if (npc->motionId[0] != 664) {
                Actor_SetMotion(npc, 664, 10, 0);
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

/* NPC_STATE_SEATED_AT_TABLE: seated at a table (PL_POSTURE_CROUCHED;
 * PL_MOT_TOWN_SIT, then PL_MOT_TOWN_SEATED), keeping facing during a talk.
 * After a random 0-255 frames it gestures: 611 in AREA_TAVERN, AREA_TOWN_ARENA
 * and AREA_ELDER_HALL (Area_IsTownHall), 649 elsewhere (at random once or
 * twice), then settles again. */
void Npc_StateSeatedAtTable(Em *npc)
{
    u8 step = npc->step;
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npc->timer = (u8)((u16)Rand_Next(1) + 180);
        npcWork->keepFacing = 1;
        Actor_SetPosture(npc, PL_POSTURE_CROUCHED, 0, 0);
        if (npc->motionId[0] == PL_MOT_IDLE) {
            Actor_SetMotion(npc, PL_MOT_TOWN_SIT, 6, 0);
        } else if (npc->motionId[0] == 649) {
            Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 10, 0);
        } else if (npc->motionId[0] != PL_MOT_TOWN_SEATED) {
            Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 0, 0);
        }
        break;
    case 1:
        if (Area_IsTownHall(gScene.area) == 1) {
            if (--npc->timer <= 0) {
                npc->step++;
                Actor_SetMotion(npc, 611, 6, 0);
            }
        } else if (--npc->timer <= 0) {
            npc->step++;
            if (npc->motionId[0] != 649) {
                Actor_SetMotion(npc, 649, 0, 0);
            }
        }
        break;
    case 2:
        if (Area_IsTownHall(gScene.area) == 1) {
            if (npc->motion[0].playing <= 0) {
                npc->step++;
                Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 4, 0);
            }
        } else if (npc->motion[0].playing <= 0) {
            if (((u16)Rand_Next(1) & 1) == 0) {
                npc->timer = 0;
                npc->step--;
            } else {
                npc->step++;
                Actor_SetMotion(npc, PL_MOT_TOWN_SEATED, 4, 0);
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

/* NPC_STATE_SIT_DOWN and NPC_STATE_SEATED (flag 0, 1): sits down at a table
 * (PL_MOT_TOWN_SIT, then PL_MOT_TOWN_SEATED, and the state becomes NPC_STATE_SEATED), or
 * with flag 1 is seated already. It can't be talked to while it sits down. */
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
            npc->state = 37;
        }
        break;
    case 2:
        break;
    }
}

/* States 72 and 73 (variant 0, 1): plays 677, then holds 678;
 * state 73 holds 678 at once. A talk leaves these poses alone
 * (Npc_StateTalk). The goto shares the hold between the two cases, as the
 * game's code does. */
void Npc_State72(Em *npc, s32 variant)
{
    u16 motion;
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (!(variant & 0xFF)) {
            motion = npc->motionId[0];
            if ((motion != 677) && (motion != 678)) {
                Actor_SetMotion(npc, 677, 0, 0);
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
            if (npc->motionId[0] != 678) {
                Actor_SetMotion(npc, 678, 0, 0);
            }
        }
        break;
    }
}

/* State 74: holds pose 679; it can't be talked to. */
void Npc_State74(Em *npc)
{
    ((NpcWork *)&npc->work)->noTalk = 1;
    if (npc->motionId[0] != 679) {
        Actor_SetMotion(npc, 679, 6, 0);
    }
}

/* NPC_STATE_TALK_DELAY: waits 8 frames, then the talk (NPC_STATE_TALK). */
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
            Actor_SetState(npc, 0, 44U);
        }
        break;
    }
}

/* NPC_STATE_ROUTE_WAIT: stands for a random time (the first route point's
 * argument plus a random number, kept as a byte), then NPC_STATE_IDLE. */
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
            Actor_SetMotion(obj, PL_MOT_IDLE, 20, 0);
        } else {
            Actor_SetMotionOnLayer(obj, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 20, 0, 0);
        }
        break;
    case 1:
        if (--obj->timer <= 0) {
            Actor_SetState(obj, 0, 1);
        }
        break;
    }
}

/* NPC_STATE_RANDOMIZE_WAIT: stands, and sets the wait at its route's first
 * point (route[0].arg) to a random 0-145 frames in steps of 5. */
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
                Actor_SetMotionOnLayer(obj, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 0, 0, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

/* NPC_STATE_DRINK_WAIT: holds pose 861. The empty case falls through, as in the
 * game's code. */
void Npc_State133(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        if (npc->motionId[0] != 861) {
            Actor_SetMotion(npc, 861, 6, 0);
            npc->step = (u8) (npc->step + 1);
        }
        
    case 1:
        break;
    }
}

/* NPC_STATE_DRINK: holds pose 862. */
void Npc_State134(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 862, 10, 0);
        
    case 1:
        break;
    }
}

/* State 135: holds pose 613. */
void Npc_State135(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 613, 6, 0);
        
    case 1:
        break;
    }
}

/* State 136: holds pose 614. */
void Npc_State136(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 614, 6, 0);
        
    case 1:
        break;
    }
}

/* State 137: holds pose 610. */
void Npc_State137(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 610, 6, 0);
        
    case 1:
        break;
    }
}

/* State 75: holds pose 638 (once 641 has ended) and keeps facing
 * during a talk. */
void Npc_State75(Em *obj)
{
    NpcWork *npcWork = (NpcWork *)&obj->work;
    u16 mot;

    switch (obj->step) {
    case 0:
        npcWork->keepFacing = 1;
        mot = obj->motionId[0];
        if (mot != 638) {
            if (mot == 641) {
                if (obj->motion[0].playing <= 0) {
                    Actor_SetMotion(obj, 638, 2, 0);
                    obj->step++;
                }
            } else {
                Actor_SetMotion(obj, 638, 2, 0);
                obj->step++;
            }
        } else {
            obj->step++;
        }
        break;
    }
}

/* States 76 and 77 (variant 0, 1): holds pose 645 or 644 and
 * keeps facing during a talk. */
void Npc_State76(Em *npc, s8 variant)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npcWork->keepFacing = 1;
        npc->step++;
        switch (variant) {
        case 0:
            if (npc->motionId[0] != 645) {
                Actor_SetMotion(npc, 645, 0, 0);
            }
            break;
        case 1:
            if (npc->motionId[0] != 644) {
                Actor_SetMotion(npc, 644, 0, 0);
            }
            break;
        }
        break;
    }
}

/* NPC_STATE_BUSY_IDLE, a working townsperson's idle (Npc_HasSecondHead; any
 * other goes back to NPC_STATE_IDLE): holds STATIONED_MOT_BUSY_IDLE, keeping
 * facing during a talk, and after a random 0-255 frames plays
 * STATIONED_MOT_BUSY_IDLE_FIDGET once. Its talk is NPC_STATE_BUSY_TALK. */
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
        npc->timer = (u8)((u16)Rand_Next(1) + 300);
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_BUSY_IDLE)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_IDLE), 2, 0, 0);
        }
        break;
    case 1:
        if (--npc->timer <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_IDLE_FIDGET), 2, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step = 0;
        }
        break;
    }
}

/* NPC_STATE_BUSY_TALK, the talk of a townsperson in NPC_STATE_BUSY_IDLE or
 * NPC_STATE_BUSY_TALK_HOLD: STATIONED_MOT_BUSY_TALK_START,
 * STATIONED_MOT_BUSY_TALK_TURN, then STATIONED_MOT_BUSY_TALK while the talk
 * lasts (no talk again meanwhile); when it ends, STATIONED_MOT_BUSY_TALK_TURN
 * for 96-127 frames and STATIONED_MOT_BUSY_IDLE for 600-855 frames, then its
 * route point's state. */
void Npc_StateBusyTalk(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npcWork->noTalk = 1;
        npc->step++;
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK_START), 8, 0, 0);
        break;
    case 1:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK_TURN), 2, 0, 0);
        } else if (gTownMode == TOWN_MODE_FREE) {
            npc->step += 3;
            npc->timer = ((u16)Rand_Next(1) & 0x1F) + 96;
        }
        break;
    case 2:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK), 2, 0, 0);
        } else if (gTownMode == TOWN_MODE_FREE) {
            npc->step += 2;
            npc->timer = ((u16)Rand_Next(1) & 0x1F) + 96;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK_TURN), 10, 0, 0);
        }
        break;
    case 3:
        npcWork->noTalk = 1;
        if (gTownMode == TOWN_MODE_FREE) {
            npc->step++;
            npc->timer = ((u16)Rand_Next(1) & 0x1F) + 96;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK_TURN), 10, 0, 0);
        }
        break;
    case 4:
        if (--npc->timer <= 0) {
            npc->step++;
            npc->timer = (u8)Rand_Next(1) + 600;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_IDLE), 60, 0, 0);
        }
        break;
    case 5:
        if (--npc->timer <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* NPC_STATE_BUSY_TALK_HOLD: plays STATIONED_MOT_BUSY_TALK_START,
 * STATIONED_MOT_BUSY_TALK_TURN and STATIONED_MOT_BUSY_TALK once and holds the
 * last; a talk takes it to NPC_STATE_BUSY_TALK. */
void Npc_StateBusyTalkHold(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK_START)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK_START), 8, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = step + 1;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK_TURN), 2, 0, 0);
        }
        break;
    case 2:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_TALK), 2, 0, 0);
        }
        break;
    }
}

/* NPC_STATE_BUSY_POSE, a working townsperson's pose STATIONED_MOT_BUSY_POSE
 * (any other goes back to NPC_STATE_IDLE). Its talk is
 * NPC_STATE_BUSY_POSE_TALK. */
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
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE), 2, 0, 0);
        break;
    }
}

/* NPC_STATE_BUSY_POSE_TALK, the talk of a townsperson in NPC_STATE_BUSY_POSE:
 * STATIONED_MOT_BUSY_POSE_TALK_START (unless it was standing),
 * STATIONED_MOT_BUSY_POSE_TALK_TURN, then STATIONED_MOT_BUSY_POSE_TALK held
 * while the talk lasts (no talk again meanwhile); afterwards
 * STATIONED_MOT_BUSY_POSE_TALK_TURN for 600-855 frames and
 * STATIONED_MOT_BUSY_POSE, then its route point's state. */
void Npc_StateBusyPoseTalk(Em *npc)
{
    u8 step = npc->step;
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npcWork->noTalk = 1;
        if (npc->motionId[0] != PL_MOT_IDLE) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE_TALK_START), 4, 0, 0);
        }
        break;
    case 1:
        npc->step = step + 1;
        npcWork->noTalk = 1;
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE_TALK_TURN), 2, 0, 0);
        break;
    case 2:
        npcWork->noTalk = 1;
        if (npc->motion[0].playing <= 0 && npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE_TALK)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE_TALK), 2, 0, 0);
        }
        if (gTownMode == TOWN_MODE_FREE) {
            npc->timer = (u8)Rand_Next(1) + 600;
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE_TALK_TURN), 26, 0, 0);
        }
        break;
    case 3:
        if (--npc->timer == 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE), 30, 0, 0);
        } else if (npc->motion[0].playing == 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_POSE_TALK_TURN), 2, 0, 0);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* NPC_STATE_BUSY_LOOP, a working townsperson's loop of STATIONED_MOT_BUSY_LOOP
 * (any other goes back to NPC_STATE_IDLE), keeping facing during a talk. Its
 * talk is NPC_STATE_BUSY_LOOP_TALK. */
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
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP), 2, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing == 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP), 2, 88, 0);
        }
        break;
    }
}

/* NPC_STATE_BUSY_LOOP_TALK, the talk of a townsperson in NPC_STATE_BUSY_LOOP:
 * finishes STATIONED_MOT_BUSY_LOOP (to frame 89), plays
 * STATIONED_MOT_BUSY_LOOP_TALK_START and holds STATIONED_MOT_TALK while the
 * talk lasts, then STATIONED_MOT_BUSY_LOOP and its route point's state. It
 * can't be talked to again meanwhile. */
void Npc_StateBusyLoopTalk(Em *npc)
{
    u8 step;
    NpcWork *npcWork;

    npcWork = (NpcWork *)&npc->work;
    npcWork->noTalk = 1;
    step = npc->step;
    switch (step) {
    case 0:
        if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP)) {
            if (Motion_ReachedFrame(npc, 0, 89.0f) != 0) {
                npc->step++;
            }
        } else if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP_TALK_START) || npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_TALK)) {
            npc->step = 2;
        } else {
            npc->step = step + 1;
        }
        break;
    case 1:
        npc->step = step + 1;
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP_TALK_START), 4, 0, 0);
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step = step + 1;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_TALK), 4, 0, 0);
        }
        break;
    case 3:
        if (gTownMode == TOWN_MODE_FREE) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_BUSY_LOOP), 10, 0, 0);
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

/* State 63, the idle of kinds 60-62: holds 690 for a random 0-255
 * frames, then either 691 3 to 6 times and 692, or 694 once. Its
 * talk is state 64. */
void Npc_State63(Em *npc)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        npc->timer = (u8)((u16)Rand_Next(1) + 100);
        Actor_SetMotion(npc, 690, 0, 0);
        break;
    case 1:
        if (--npc->timer <= 0) {
            if (((u16)Rand_Next(1) & 1) == 0) {
                npc->step++;
                npc->timer = ((u16)Rand_Next(1) & 3) + 3;
                Actor_SetMotion(npc, 691, 6, 0);
            } else {
                npc->step = 4;
                Actor_SetMotion(npc, 694, 8, 0);
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->step++;
            Actor_SetMotion(npc, 692, 10, 0);
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            npc->step = 1;
            npc->timer = 120;
            Actor_SetMotion(npc, 690, 4, 0);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            npc->step = 1;
            npc->timer = (u8)((u16)Rand_Next(1) + 100);
            Actor_SetMotion(npc, 690, 10, 0);
        }
        break;
    }
}

/* State 64, the talk of kinds 60-62: turns to the local player over
 * 10 frames with 693; when the talk ends it turns back (Npc_StartTurnBack)
 * with 690 and, once the turn's layer is done, returns to its route
 * point's state. No talk again meanwhile. */
void Npc_State64(Em *npc)
{
    u8 step;
    NpcWork *npcWork;

    step = npc->step;
    npcWork = (NpcWork *)&npc->work;
    switch (step) {
    case 0:
        npcWork->noTalk = 1;
        npc->step = (u8) (npc->step + 1);
        Actor_SetMotion(npc, 693, 10, 0);
        Npc_TurnToPlayer(npc, 10);
        break;
    case 1:
        npcWork->noTalk = 1;
        if (gTownMode == TOWN_MODE_FREE) {
            npc->step = (u8) (npc->step + 1);
            Actor_SetMotion(npc, 690, 10, 0);
            Npc_StartTurnBack(npc, 10);
        }
        break;
    case 2:
        if (npc->motion[1].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}

/* State 46, the idle of kinds 43-48: holds 650 for a random 0-255
 * frames, then plays 654 once. Its talk is state 47. */
void Npc_State46(Em *npc)
{
    s32 timer;
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        npc->timer = (s32) (((Rand_Next(1) & 0xFFFF) + 10) & 0xFF);
        if (npc->motionId[0] != 650) {
            Actor_SetMotion(npc, 650, 0, 0);
        }
        break;
    case 1:
        timer = npc->timer - 1;
        npc->timer = timer;
        if (timer <= 0) {
            npc->step = (u8) (npc->step + 1);
            Actor_SetMotion(npc, 654, 10, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing == 0) {
            npc->step = 0U;
        }
        break;
    }
}

/* State 47, the talk of kinds 43-48: 651, then 652 while the
 * talk lasts (no talk again meanwhile); afterwards 653 and 650 for
 * 255-510 frames, then its route point's state. Offline, townsperson 2
 * standing with townsperson 1 (Npc_CheckHubPartner) stays in its talk pose. The
 * goto shares the ending between the offline and online cases, as the
 * game's code does. */
void Npc_State47(Em *npc)
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
        if (npc->motionId[0] != 651) {
            Actor_SetMotion(npc, 651, 8, 0);
        }
        break;
    case 1:
        if ((npc->motion[0].playing == 0) && (npc->motionId[0] == 651)) {
            Actor_SetMotion(npc, 652, 2, 0);
        }
        if (gTownMode == TOWN_MODE_FREE) {
            if (Game_IsOnline() == 0) {
                if (Npc_CheckHubPartner(npc) != 1) {
                    goto block_15;
                }
            } else {
block_15:
                npc->step = (u8) (npc->step + 1);
                Actor_SetMotion(npc, 653, 2, 0);
                break;
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing == 0) {
            Actor_SetMotion(npc, 650, 14, 0);
            npc->timer = (s32) ((Rand_Next(1) & 0xFF) + 255);
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

/* NPC_STATE_GREET, a greeter's greeting (from NPC_STATE_IDLE when the local
 * player comes within 400): 684 if the player stands or walks (TOWN_PC_IDLE,
 * TOWN_PC_WALK), else 685; within 300 it goes on to NPC_STATE_GREET_HOLD,
 * beyond 400 it waits 120 frames and returns to its route point's state. */
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
            Actor_SetMotion(npc, 684, 10, 0);
        } else {
            Actor_SetMotion(npc, 685, 8, 0);
        }
        break;
    case 1:
        dist = Vec_Distance(&npc->posX, (f32 *) pl->pos);
        if (dist < 300.0f) {
            npc->step++;
            if (npc->motionId[0] != 685) {
                Actor_SetMotion(npc, 685, 8, 0);
            }
        } else if (!(dist <= 400.0f)) {
            npc->step = 3;
            npc->timer = 120;
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

/* NPC_STATE_GREET_HOLD, a greeter beside the player: 681 or 680 while the local
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
            Actor_SetMotion(npc, 681, 4, 0);
        } else {
            Actor_SetMotion(npc, 680, 8, 0);
        }
        break;
    case 1:
        if (!(Vec_Distance(&npc->posX, (f32 *) pl->pos) <= 300.0f)) {
            npc->step++;
            npc->timer = 120;
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

/* NPC_STATE_GREET_TALK, a greeter's talk: 682, then 680 while the local player
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
        Actor_SetMotion(npc, 682, 10, 0);
        break;
    case 1:
        npcWork->noTalk = 1;
        Actor_SetMotion(npc, 680, 16, 0);
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

/* NPC_STATE_PAIR_WAIT, townsperson 2 standing with townsperson 1 (as
 * NPC_STATE_PAIR_STAND): back to its route point's state once 1 is no longer
 * near, and into its talk state (Npc_StartTalkState) when 1, near, does
 * something else. */
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

/* NPC_STATE_LEAVE: the townsperson leaves: its slot ends (NPC_STATE_GONE: the
 * update then ends its task) and unk49 marks it as leaving. */
void Npc_StateLeave(Em *npc) {
    npc->state = 96;
    ((NpcWork *)&npc->work)->leaving = 1;
}



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
        if (npc->motionId[0] == 852) {
            npc->rotY = ANGLE_TURN / 8;
            Actor_SetMotion(npc, 853, 0, 0);
        } else {
            npc->step = 2;
            npc->rotY = ANGLE_TURN / 8;
            Actor_SetMotion(npc, PL_MOT_IDLE, 16, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotion(npc, PL_MOT_IDLE, 16, 0);
        } else {
            w->noTalk = 1;
        }
        break;
    case 2:
        if ((dcur < 800.0f || dnext < 800.0f) && pl->pos[1] < 200.0f && dcur > dnext) {
            npc->step++;
            Actor_SetMotion(npc, 850, 0, 0);
            w->noTalk = 1;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            if ((dcur < 800.0f || dnext < 800.0f) && dcur < dnext) {
                npc->step = 1;
                *(NpcPos *)npc->goal = *point;
                *(NpcPos *)(&npc->posX) = *(NpcPos *)npc->goal;
                Actor_SetMotion(npc, 853, 0, 0);
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
        if (npc->motionId[0] == 850) {
            npc->rotY = 0x2AAB;
            Actor_SetMotion(npc, 851, 0, 0);
        } else {
            npc->step = 2;
            npc->rotY = 0x2AAB;
            Actor_SetMotion(npc, PL_MOT_IDLE, 16, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotion(npc, PL_MOT_IDLE, 16, 0);
        } else {
            w->noTalk = 1;
        }
        break;
    case 2:
        if ((dcur < 800.0f || dnext < 800.0f) && pl->pos[1] < 200.0f && dcur > dnext) {
            npc->step++;
            Actor_SetMotion(npc, 852, 0, 0);
            w->noTalk = 1;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            if ((dcur < 800.0f || dnext < 800.0f) && dcur < dnext) {
                npc->step = 1;
                *(NpcPos *)npc->goal = *point;
                *(NpcPos *)(&npc->posX) = *(NpcPos *)npc->goal;
                Actor_SetMotion(npc, 851, 0, 0);
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

/* NPC_STATE_SMITH_TALK and NPC_STATE_SMITH_TALK_AFTER_FORGE (flag 0, 1), the
 * smith's talk (NPC_WORKSHOP_SMITH; NPC_STATE_SMITH_TALK_AFTER_FORGE follows a forging): turns to the local
 * player over 6 frames with STATIONED_MOT_SMITH_TALK; when the talk ends it
 * stands (STATIONED_MOT_SMITH_IDLE), turns to its forge (heading 0xED84) if it
 * is up high (above 200), and once turned returns to its route point's state.
 * No talk again meanwhile. The NpcTable_Find call's result is unused (Em.work
 * is the townsperson's id, NpcWork.id); the call stays, as in the game's code.
 */
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
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_SMITH_TALK)) {
            if (afterForge == 0) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_TALK), 4, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_TALK), 4, 154, 0);
            }
        }
        break;
    case 1:
        if (gTownMode == TOWN_MODE_FREE) {
            if (!(npc->posY <= 200.0f)) {
                Npc_StartTurnTo(npc, 12, 60804);
            }
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 10, 0, 0);
        }
        break;
    case 2:
        if ((u16)npcWork->turnCount == 0) {
            Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
        }
        break;
    }
}


/* The smith forging alone (NPC_STATE_SMITH_FORGE): strikes with
 * STATIONED_MOT_FORGE_START, STATIONED_MOT_FORGE_HAMMER twice and
 * STATIONED_MOT_FORGE_LAST (motionPhase marks frames 30 to 118), then
 * STATIONED_MOT_FORGE_END. If the player has it locked on, it turns away first
 * and back to the player at the end, then talks
 * (NPC_STATE_SMITH_TALK_AFTER_FORGE); otherwise it returns to its route point's
 * state. */
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
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_FORGE_START)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_START), 0, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            npc->timer = 2;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_HAMMER), 4, 0, 0);
        }
        break;
    case 2:
        if (w->motionPhase == 0 && Motion_ReachedFrame(npc, 0, 30.0f)) {
            w->motionPhase = 1;
        }
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_LAST), 0, 0, 0);
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
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_END), 16, 0, 0);
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

/* The smith forging with its partner (townsperson 26): strikes
 * (STATIONED_MOT_FORGE_START), stands (STATIONED_MOT_SMITH_IDLE) until the
 * partner strikes (856) and finishes, then STATIONED_MOT_FORGE_END and the talk
 * NPC_STATE_SMITH_TALK_AFTER_FORGE. The player's lock-on turns it as in
 * Npc_SmithForgeAlone. Nothing happens without the partner. */
void Npc_SmithForgeWithPartner(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;
    Em *partner = Npc_Find(26);
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
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_FORGE_START)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_START), 0, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = step + 1;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 4, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0 && partner->motionId[0] == 856) {
            npc->step = step + 1;
        }
        break;
    case 3:
        if (partner->motion[0].playing <= 0) {
            npc->step = step + 1;
            if (npc == gPlayers[gScene.localPlayer].target) {
                Npc_TurnToPlayer(npc, 6);
            }
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_END), 4, 0, 0);
        }
        break;
    case 4:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, NPC_STATE_SMITH_TALK_AFTER_FORGE);
        }
        break;
    }
}

/* NPC_STATE_SMITH_FORGE, the smith's forging: alone while the smithy
 * (VILLAGE_FACILITY_SMITHY) is at level 0 or the workshop helper
 * (NPC_WORKSHOP_HELPER) is present, else with its partner (townsperson
 * 26). It can't be talked to meanwhile. */
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

/* NPC_STATE_PARTNER_WORK, the smith's partner (townsperson 26) at work beside
 * the smith (NPC_WORKSHOP_SMITH): 855 three times (motionPhase 0 or 1 at
 * random), 856 and 854, again each time the smith stands
 * (STATIONED_MOT_SMITH_IDLE); 857 when the smith plays
 * STATIONED_MOT_SMITH_PARTNER_CUE. When the smith forges
 * (STATIONED_MOT_FORGE_START) without the workshop helper it strikes along
 * (NPC_STATE_PARTNER_STRIKE); during the smith's talk (NPC_STATE_SMITH_TALK or
 * NPC_STATE_SMITH_TALK_AFTER_FORGE) it stands (854). */
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
        if (npc->motionId[0] != 855) {
            Actor_SetMotion(npc, 855, 6, 0);
            npc->timer = 3;
            w->motionPhase = (u16)Rand_Next(1) & 1;
        }
        npc->step++;
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            if (--npc->timer <= 0) {
                Actor_SetMotion(npc, 856, 0, 0);
                npc->step++;
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 854, 6, 0);
            npc->step++;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0 && smith->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE)) {
            npc->step = 0;
        }
        break;
    case 4:
        Actor_SetMotion(npc, 857, 6, 0);
        npc->step++;
        break;
    case 5:
        if (npc->motion[0].playing <= 0 && smith->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE)) {
            npc->step = 0;
        }
        break;
    }
    if (smith->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_SMITH_PARTNER_CUE) && npc->step <= 3) {
        npc->step = 4;
    }
    if (smith->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_FORGE_START) && Npc_Find(NPC_WORKSHOP_HELPER) == 0) {
        Actor_SetState(npc, 0, NPC_STATE_PARTNER_STRIKE);
        return;
    }
    if ((smith->state == NPC_STATE_SMITH_TALK || smith->state == NPC_STATE_SMITH_TALK_AFTER_FORGE) && npc->motionId[0] != 854) {
        npc->step = 3;
        Actor_SetMotion(npc, 854, 20, 0);
    }
}


/* State 157: a townsperson of model type 1 holds STATIONED_MOT_FORGE_HAMMER;
 * the others loop 856 and 855 with random pauses (0-63 frames, 0-3 repeats),
 * the motion speed set back to 1 after frame 226 of 855. */
void Npc_State157(Em *npc)
{
    switch (npc->step) {
    case 0:
        if (npc->type == 1) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_HAMMER), 4, 0, 0);
            npc->step = 99;
        } else if (npc->motionId[0] != 856) {
            Actor_SetMotion(npc, 856, 0, 0);
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
            if (npc->motionId[0] != 855) {
                Actor_SetMotion(npc, 855, 6, 0);
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

/* NPC_STATE_PARTNER_STRIKE, the smith's partner striking along with the smith's
 * forging: 855 three times, 856 and 854, then again while the smith forges
 * (STATIONED_MOT_FORGE_START), or back to its route point's state once the
 * smith stands (STATIONED_MOT_SMITH_IDLE). Nothing happens without the smith.
 */
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
            if (npc->motionId[0] != 855) {
                Actor_SetMotion(npc, 855, 6, 0);
                npc->timer = 3;
            }
            break;
        case 1:
            if (npc->motion[0].playing <= 0) {
                timer = npc->timer - 1;
                npc->timer = timer;
                if (timer <= 0) {
                    Actor_SetMotion(npc, 856, 0, 0);
                    npc->step = (u8) (npc->step + 1);
                    break;
                }
            }
            break;
        case 2:
            if (npc->motion[0].playing <= 0) {
                Actor_SetMotion(npc, 854, 0, 0);
                npc->step = (u8) (npc->step + 1);
            }
            break;
        case 3:
            motion = ((Em *)smith)->motionId[0];
            if (motion == EM_MOTION_ID(0, STATIONED_MOT_FORGE_START)) {
                Actor_SetState(npc, 0, 120U);
                break;
            }
            if ((npc->motion[0].playing <= 0) && (motion == EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE))) {
                Actor_SetState(npc, 0, *(u16 *)&npcWork->route[npcWork->routeIdx].state);
            }
            break;
        }
    }
}


/* The smith at its forge with its partner (townsperson 26): faces the forge
 * (heading 0xED84) standing (STATIONED_MOT_SMITH_IDLE) and, after 60 frames and
 * then every 150, answers the partner: STATIONED_MOT_SMITH_PARTNER_CUE as the
 * partner's 855 nears its end (motionPhase 1, frame 234),
 * STATIONED_MOT_SMITH_IDLE_ALT while the partner stands (854, not talking).
 * Nothing happens without the partner. */
void Npc_SmithWorkWithPartner(Em *npc)
{
    Em *partner = Npc_Find(26);
    NpcWork *partnerWork;

    if (partner == NULL) {
        return;
    }
    partnerWork = (NpcWork *)&partner->work;
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE)) {
            npc->targetYaw = npc->rotY = 0xED84;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 0, 0, 0);
        }
        npc->step++;
        npc->timer = 60;
        break;
    case 1:
        if (--npc->timer <= 0) {
            if (partner->motionId[0] == 855) {
                if (partnerWork->motionPhase == 1) {
                    npc->step++;
                }
            } else if (partner->motionId[0] == 854 && partner->state != NPC_STATE_TALK && partner->step > 2) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE_ALT), 10, 0, 0);
                npc->step = 3;
            }
        }
        break;
    case 2:
        if (partner->timer < 3 && Motion_ReachedFrame(partner, 0, 234.0f) != 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_PARTNER_CUE), 4, 0, 0);
            npc->step++;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0 || partner->state == NPC_STATE_TALK) {
            npc->step = 1;
            npc->timer = 150;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 4, 0, 0);
        }
        break;
    }
}

/* State 123: loops motion 415. */
void Npc_State123(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != PL_MOT_PICKAXE_SWING) {
            Actor_SetMotion(npc, PL_MOT_PICKAXE_SWING, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, PL_MOT_PICKAXE_SWING, 0, 0);
        }
        break;
    }
}

/* State 124: holds motion 627. */
void Npc_State124(Em *npc)
{
    if (npc->motionId[0] != 627) {
        Actor_SetMotion(npc, 627, 6, 0);
    }
}


/* NPC_STATE_CONTEST_WAIT: holds 627 while a contest (TOWN_MODE_CONTEST,
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
        if (npc->motionId[0] != 627) {
            Actor_SetMotion(npc, 627, 6, 0);
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
        Npc_StartTurnBack(npc, 10);
        Npc_EnterRoutePoint(npc);
    }
}

/* NPC_STATE_CONTEST_REACT and NPC_STATE_CONTEST_REACT_LATE (first 0, 1; NPC_STATE_CONTEST_REACT_LATE
 * first waits 160 frames): plays 631, then 876; once no contest runs
 * (TOWN_MODE_CONTEST, TOWN_MODE_DRINK_CONTEST) it turns back, once, and
 * re-enters its route point. */
void Npc_StateContestReact(Em *npc, s32 delayed)
{
    u8 step = npc->step;

    switch (step) {
    case 0:
        if (delayed == 0) {
            npc->step = 2;
            npc->stateVar0 = 0;
            if (npc->motionId[0] != 631) {
                Actor_SetMotion(npc, 631, 10, 0);
            }
        } else {
            npc->step = step + 1;
            npc->timer = 160;
        }
        break;
    case 1:
        if (--npc->timer <= 0) {
            npc->step++;
            npc->stateVar0 = 0;
            if (npc->motionId[0] != 631) {
                Actor_SetMotion(npc, 631, 6, 0);
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing == 0) {
            npc->step = step + 1;
            Actor_SetMotion(npc, 876, 10, 0);
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
                Npc_StartTurnBack(npc, 10);
                Npc_EnterRoutePoint(npc);
            }
            break;
        }
        break;
    }
}

extern NpcPos gSmithMeetSpotBelow;

/* NPC_STATE_SMITH_MEET_BELOW, the smith (NPC_WORKSHOP_SMITH; Npc_SmithWorkAlone
 * enters it): stands (STATIONED_MOT_SMITH_IDLE, STATIONED_MOT_SMITH_IDLE_ALT at
 * random) until the local player, below height 200, comes within 450 of
 * gSmithMeetSpotBelow; then it leaves (STATIONED_MOT_SMITH_LEAVE), reappears on
 * the ground at (10375, 0, 9055) facing heading 0x2AAB
 * (STATIONED_MOT_SMITH_ARRIVE_BELOW) and goes back one route point. It can't be
 * talked to while it moves. */
void Npc_StateSmithMeetBelow(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcPos spot = gSmithMeetSpotBelow;

    w->motionPhase = 0;
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 4, 0, 0);
            npc->timer = 1;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->timer = (u16)Rand_Next(1) & 3;
            if ((u16)Rand_Next(1) & 1) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 6, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE_ALT), 10, 0, 0);
            }
        }
        if (pl->pos[1] < 200.0f && Vec_Distance((f32 *) pl->pos, spot.v) <= 450.0f) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_LEAVE), 2, 0, 0);
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
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_ARRIVE_BELOW), 0, 0, 0);
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

/* NPC_STATE_SMITH_MEET_ABOVE, the smith (Npc_SmithWorkAlone enters it): waits
 * (STATIONED_MOT_SMITH_WAIT_ABOVE); when condition 47 holds (the condition
 * the smith's workshop talk needs) and the local player, above height 240,
 * comes within 310 of gSmithMeetSpotAbove, it reappears up at (9979, 248, 8661)
 * facing heading 0xED84 (STATIONED_MOT_SMITH_ARRIVE_ABOVE) and goes on one
 * route point. */
void Npc_StateSmithMeetAbove(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcPos spot = gSmithMeetSpotAbove;

    w->motionPhase = 2;
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_SMITH_WAIT_ABOVE)) {
            npc->targetYaw = npc->rotY = 0x2AAB;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_WAIT_ABOVE), 30, 0, 0);
            npc->timer = (u16)Rand_Next(1) % 120 + 120;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_WAIT_ABOVE), 30, 0, 0);
            npc->timer = (u16)Rand_Next(1) % 120 + 120;
        }
        if (Cond_Check(47) == 1 && !(pl->pos[1] <= 240.0f) && Vec_Distance((f32 *) pl->pos, spot.v) <= 310.0f) {
            npc->step++;
            npc->posX = 9979.0f;
            npc->posY = 248.0f;
            npc->posZ = 8661.0f;
            *(NpcPos *)npc->goal = *(NpcPos *)(&npc->posX);
            npc->targetYaw = npc->rotY = 0xED84;
            npc->timer = 1;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_ARRIVE_ABOVE), 0, 0, 0);
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

/* The smith at its forge alone: stands (STATIONED_MOT_SMITH_IDLE,
 * STATIONED_MOT_SMITH_IDLE_ALT at random); on the ground (below 90) it goes to
 * NPC_STATE_SMITH_MEET_BELOW when condition 47 holds and the local player is
 * up (100 or higher), and up high it goes to NPC_STATE_SMITH_MEET_ABOVE when
 * the player is below 100. */
void Npc_SmithWorkAlone(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = npc->step;

    switch (step) {
    case 0:
        npc->step = step + 1;
        Actor_SetPosture(npc, PL_POSTURE_STANDING, 0, 0);
        npc->timer = (u16)Rand_Next(1) & 3;
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 0, 0, 0);
        break;
    case 1:
    case 2:
        if (npc->posY < 90.0f) {
            if (!(pl->pos[1] < 100.0f) && Cond_Check(47) == 1) {
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
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 4, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE_ALT), 10, 0, 0);
            }
        }
        break;
    }
}

/* The two spots the smith's moves watch (Npc_StateSmithMeetBelow,
 * Npc_StateSmithMeetAbove). */
NpcPos gSmithMeetSpotBelow = {
#include "lobby/007FE490.inc"
};
NpcPos gSmithMeetSpotAbove = {
#include "lobby/007FE4A0.inc"
};

/* NPC_STATE_SMITH_WORK, the smith at its forge: alone while the smithy
 * (VILLAGE_FACILITY_SMITHY) is at level 0, else with its partner
 * (townsperson 26). */

void Npc_StateSmithWork(Em *npc) {
    if (!(Village_FacilityLevel(VILLAGE_FACILITY_SMITHY) & 0xFF)) {
        Npc_SmithWorkAlone(npc);
        return;
    }
    Npc_SmithWorkWithPartner(npc);
}


/*
 * NPC_STATE_PAIR_MEET: townspeople 49 and 50, who meet each other. Each
 * finds the other; once the other is within 200, both wait a random 0-15 frames
 * and play their meeting motion (696 for 49, 656 for 50), then stay.
 */
void Npc_StatePairMeet(Em *npc)
{
    NpcWork *w;
    Em *partner;

    w = (NpcWork *)&npc->work;
    Npc_Find(50);
    if (w->id == 49) {
        partner = Npc_Find(50);
    } else {
        partner = Npc_Find(49);
    }
    switch (npc->step) {
    case 0:
        npc->step++;
        if (w->id == 49) {
            Actor_SetMotion(npc, 675, 16, 0);
        } else {
            Actor_SetMotion(npc, PL_MOT_IDLE, 16, 0);
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
            if (w->id == 49) {
                Actor_SetMotion(npc, 696, 10, 0);
            } else {
                Actor_SetMotion(npc, 656, 10, 0);
            }
        }
        break;
    case 3:
        break;
    }
}

/*
 * NPC_STATE_CONTEST_SIT_DOWN: the contest spot sets it on NPC_CONTEST_HOST as
 * an offline arm-wrestling contest opens. It sits down at the table (motion
 * 883), then keeps replaying that motion's end from frame 212.
 */
void Npc_StateContestSitDown(Em *npc)
{
    u8 phase;

    phase = npc->stateVar0;
    switch (phase) {
    case 0:
        npc->stateVar0 = (u8) (phase + 1);
        Actor_SetMotion(npc, 883, 6, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotion(npc, 883, 0, 212);
        }
        break;
    }
}

/* NPC_STATE_CONTEST_SEAT (as the player's TOWN_PC_CONTEST_SEAT_A): the contest
 * seat's pose (motion 885). */
void Npc_StateContestSeat(Em *npc)
{
    if (npc->motionId[0] != 885) {
        Actor_SetMotion(npc, 885, 6, 0);
    }
}

/*
 * NPC_STATE_CONTEST_GRIP (as the player's TOWN_PC_CONTEST_GRIP): the
 * arm-wrestling grip (motion 886); the contest sets it on both sides as the
 * match starts. Sweat spray when the motion starts.
 */
void Npc_StateContestGrip(Em *npc)
{
    if (npc->motionId[0] != 886) {
        Actor_SetMotion(npc, 886, 4, 0);
        ActorParticles_SpawnSpray(npc);
    }
}

/* NPC_STATE_CONTEST_LEAVE: the contest has closed; gets up from the table
 * (motion 884), then stands (state 1). */
void Npc_StateContestLeave(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 884) {
            Actor_SetMotion(npc, 884, 6, 0);
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
 * NPC_STATE_CONTEST_END: the offline contest's end. NPC_CONTEST_HOST can't be
 * talked to and plays motion 888. When the motion ends, NPC_CONTEST_HOST and
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
        if (w->id == NPC_CONTEST_HOST && npc->motionId[0] != 888) {
            Actor_SetMotion(npc, 888, 6, 796);
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
 * NPC_STATE_CONTEST_LOSE (as the player's TOWN_PC_CONTEST_LOSE): the contest's
 * host has lost. NPC_CONTEST_HOST holds motion 888 and can't be talked to;
 * NPC_DRINK_CONTEST_HOST plays 864.
 */
void Npc_StateContestLose(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    if (npc->work == NPC_CONTEST_HOST) {
        if (npc->motionId[0] != 888) {
            Actor_SetMotion(npc, 888, 0, 0);
        }
        w->noTalk = 1;
    } else if (npc->work == NPC_DRINK_CONTEST_HOST) {
        if (npc->motionId[0] != 864) {
            Actor_SetMotion(npc, 864, 20, 0);
        }
    }
}

/*
 * NPC_STATE_CONTEST_WIN (as the player's TOWN_PC_CONTEST_WIN): the contest's
 * host has won. NPC_CONTEST_HOST plays motion 887; NPC_DRINK_CONTEST_HOST plays
 * 863.
 */
void Npc_StateContestWin(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        if (w->id == NPC_CONTEST_HOST) {
            if (npc->motionId[0] != 887) {
                Actor_SetMotion(npc, 887, 4, 0);
            }
        } else if (w->id == NPC_DRINK_CONTEST_HOST) {
            if (npc->motionId[0] != 863) {
                Actor_SetMotion(npc, 863, 20, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

/* State 125: plays motion 875 once (20-frame blend) and keeps it. Case 0 runs
 * on into case 1, which does nothing. */
void Npc_State125(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 875) {
            Actor_SetMotion(npc, 875, 20, 0);
        }
        
    case 1:
        break;
    }
}

/*
 * NPC_STATE_WATCH_CONTEST: the contests' onlooker (townspeople of type 55). It plays
 * motion 876. Offline, with the local player within 400 and while it is not
 * turning:
 * - during TOWN_MODE_CONTEST it turns to the player once, and during
 *   TOWN_MODE_DRINK_CONTEST toward the tavern table's point (9216, 126, 10984);
 *   it then stays at its route point;
 * - it reacts to the player's result: a win (TOWN_PC_CONTEST_WIN,
 *   TOWN_PC_DRINK_WIN; in the drinking contest it turns to the player too)
 *   gives NPC_STATE_CONTEST_WAIT, an arm-wrestling loss NPC_STATE_CONTEST_REACT, a drinking loss NPC_STATE_CONTEST_REACT_LATE.
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
        if (npc->motionId[0] != 876) {
            Actor_SetMotion(npc, 876, 20, 0);
        }
        break;
    case 1:
        if (npc->variant != 55 || (u16)w->turnCount != 0) {
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


/*
 * NPC_STATE_ROUTE_TURN, a route point: turns over 10 frames to the angle in the
 * entry's argument, then goes on to the next point. Can't be talked to
 * meanwhile.
 */
void Npc_StateRouteTurn(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *entry = &w->route[w->routeIdx];

    w->noTalk = 1;
    Npc_StartTurnTo(npc, 10, entry->arg);
    Npc_NextRoutePoint(npc);
}


/*
 * NPC_STATE_ROUTE_WALK: walks to the current route point. On entry it stands
 * and picks PL_MOT_WALK or PL_MOT_RUN at random, with a longer blend from
 * motion 875. Each frame it turns a tenth of the way toward the point (at its
 * own height), and within 40 of the point it goes on to the next one.
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
            if (npc->motionId[0] != PL_MOT_RUN) {
                if (npc->motionId[0] == 875) {
                    Actor_SetMotion(npc, PL_MOT_RUN, 12, 0);
                } else {
                    Actor_SetMotion(npc, PL_MOT_RUN, 4, 0);
                }
            }
        } else {
            if (npc->motionId[0] != PL_MOT_WALK) {
                if (npc->motionId[0] == 875) {
                    Actor_SetMotion(npc, PL_MOT_WALK, 12, 0);
                } else {
                    Actor_SetMotion(npc, PL_MOT_WALK, 4, 0);
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

/* State 138: plays motion 858 two to five times, then 866 once, and over
 * again. */
void Npc_State138(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 858) {
            Actor_SetMotion(npc, 858, 6, 0);
            npc->timer = ((u16)Rand_Next(1) & 3) + 2;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            if (--npc->timer <= 0) {
                if (npc->motionId[0] != 866) {
                    Actor_SetMotion(npc, 866, 0, 0);
                    npc->timer = 1;
                } else {
                    Actor_SetMotion(npc, 858, 6, 0);
                    npc->timer = ((u16)Rand_Next(1) & 3) + 2;
                }
            }
        }
        break;
    }
}

/* State 140: plays motion 860 once (20-frame blend) and keeps it. */
void Npc_State140(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        if (npc->motionId[0] != 860) {
            Actor_SetMotion(npc, 860, 20, 0);
        }
        npc->step = (u8) (npc->step + 1);
        
    case 1:
        break;
    }
}


/*
 * NPC_STATE_CALL_OVER: plays motion 874. At its frame 30 the townsperson turns
 * toward the local player over 10 frames and can't be talked to meanwhile. When
 * the motion ends it starts its talk (NPC_STATE_TALK).
 */
void Npc_StateCallOver(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 874) {
            Actor_SetMotion(npc, 874, 4, 0);
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
 * NPC_STATE_REACT_NEAR: reacts to the local player coming near. It does not
 * turn when talked to, and plays motion 868. Within 300 it plays 869; when the
 * player leaves it plays 870, then starts over. Townsperson 47 does not react
 * while Cond_Check(100) holds.
 */
void Npc_StateReactNear(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    f32 dist;

    w->keepFacing = 1;
    dist = Vec_Distance(&npc->posX, (f32 *) gPlayers[gScene.localPlayer].pos);
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != 868) {
            Actor_SetMotion(npc, 868, 4, 0);
        }
        npc->step++;
        break;
    case 1:
        if (Cond_Check(100) == 1 && w->id == 47) {
            break;
        }
        if (dist < 300.0f) {
            Actor_SetMotion(npc, 869, 0, 0);
            npc->step++;
        }
        break;
    case 2:
        if (!(dist < 300.0f)) {
            Actor_SetMotion(npc, 870, 2, 0);
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
 * State 146: plays motion STATIONED_MOT_IDLE_B three to six times, then
 * STATIONED_MOT_IDLE_B_BREAK, and over again. The blend is 30 frames when it
 * comes from STATIONED_MOT_TALK, STATIONED_MOT_IDLE_B_TALK_START or
 * STATIONED_MOT_IDLE.
 */
void Npc_State146(Em *npc)
{
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_IDLE_B)) {
            if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_TALK) || npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_IDLE_B_TALK_START) || npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_IDLE)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE_B), 30, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE_B), 4, 0, 0);
            }
        }
        npc->timer = ((u16)Rand_Next(1) & 3) + 3;
        npc->step++;
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE_B_BREAK), 6, 0, 0);
            npc->step++;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->step = 1;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE_B), 30, 0, 0);
            npc->timer = ((u16)Rand_Next(1) & 3) + 3;
        }
        break;
    }
}


/* NPC_STATE_WAIT_TOWN_FREE: plays motion STATIONED_MOT_IDLE_B_TALK_START, then
 * holds STATIONED_MOT_TALK until the town is back to TOWN_MODE_FREE, then its
 * route point's state. */
void Npc_StateWaitTownFree(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_IDLE_B_TALK_START)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE_B_TALK_START), 6, 0, 0);
            npc->step++;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_TALK), 4, 0, 0);
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


/* States 148 (flag 0) and 149 (flag 1): holds motion 1063 or 1064. */
void Npc_State148(Em *npc, u8 second)
{
    switch (npc->step) {
    case 0:
        if (second == 0) {
            Actor_SetMotionOnLayer(npc, 1063, 0, 0, 0);
        } else {
            Actor_SetMotionOnLayer(npc, 1064, 0, 0, 0);
        }
        npc->step++;
        break;
    }
}

/*
 * NPC_STATE_IDLE_BY_TIME: an idle by the time of day. While ElderRaid_IsOn
 * holds it plays motion STATIONED_MOT_RAID_WAIT. In the afternoon
 * (LobbyTime_IsPm) it plays STATIONED_MOT_IDLE_PM three times, then
 * STATIONED_MOT_IDLE_PM_BREAK, and over again. Otherwise it loops
 * STATIONED_MOT_IDLE. Coming from STATIONED_MOT_TALK or
 * STATIONED_MOT_TALK_START, it first plays STATIONED_MOT_IDLE with a 30-frame
 * blend.
 */
void Npc_StateIdleByTime(Em *npc)
{
    s32 isPm = LobbyTime_IsPm();

    switch (npc->step) {
    case 0:
        npc->timer = 1;
        npc->step++;
        if (ElderRaid_IsOn() == 1) {
            if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_RAID_WAIT)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_RAID_WAIT), 30, 0, 0);
            }
        } else if (isPm == 1) {
            if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_TALK) || npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_TALK_START)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 30, 0, 0);
                npc->timer = 3;
            } else if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_IDLE_PM)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE_PM), 30, 0, 0);
                npc->timer = 3;
            }
        } else {
            if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_TALK) || npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_TALK_START)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 30, 0, 0);
            } else if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_IDLE)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 4, 0, 0);
            }
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            if (npc->motionId[0] == EM_MOTION_ID(0, STATIONED_MOT_IDLE_PM)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE_PM_BREAK), 6, 0, 0);
                npc->step++;
            } else {
                npc->step = 0;
            }
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_IDLE), 30, 0, 0);
            npc->timer = 2;
            npc->step = 1;
        }
        break;
    }
}


/* State 159: plays motion STATIONED_MOT_RAID_WAIT (30-frame blend) and keeps
 * it. */
void Npc_State159(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_RAID_WAIT)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_RAID_WAIT), 30, 0, 0);
        }
        break;
    }
}

/* NPC_STATE_STAND_UP (as the player's TOWN_PC_STAND_UP): gets up from the seat
 * (motion 610); can't be talked to. */
void Npc_StateStandUp(Em *npc)
{
    u8 step;

    ((NpcWork *)&npc->work)->noTalk = 1;
    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 610, 4, 0);
        break;
    }
}

/* State 163: plays motion 899 once, then its route point's state. */
void Npc_State163(Em *npc)
{
    u8 step;
    NpcWork *w;

    step = npc->step;
    w = (NpcWork *)&npc->work;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 899) {
            Actor_SetMotion(npc, 899, 10, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

/* NPC_STATE_FORGE: plays motion 255 once, then its route point's state. */
void Npc_StateForge(Em *npc)
{
    u8 step;
    NpcWork *w;

    step = npc->step;
    w = (NpcWork *)&npc->work;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        if (npc->motionId[0] != 255) {
            Actor_SetMotion(npc, 255, 4, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

/* NPC_STATE_HAMMER: plays motion 256, then loops 855 at 1.8 times the speed. */
void Npc_StateHammer(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        npc->stateVar0 = 0;
        if (npc->motionId[0] != 256) {
            Actor_SetMotion(npc, 256, 12, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotion(npc, 855, 0, 184);
            npc->motionSpeed = 1.8f;
        }
        break;
    case 2:
        break;
    }
}

/*
 * State 173: plays motion 1022 (from frame 176) with motionPhase 1, then its
 * route point's state. Each frame it sets flag 0x4000000, so the motion's root
 * movement is skipped for that frame (Player_MoveLocal clears the flag).
 */
void Npc_State173(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    Player_SetFlag((Player *) npc, 0x04000000U);
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != 1022) {
            Actor_SetMotionOnLayer(npc, 1022, -2, 176, 0);
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
 * State 174: plays motion STATIONED_MOT_FORGE_START,
 * STATIONED_MOT_FORGE_HAMMER twice, STATIONED_MOT_FORGE_LAST, then keeps
 * STATIONED_MOT_SMITH_IDLE. motionPhase marks the part where the motion's
 * effect runs: 2 on entry, 1 from frame 30 of STATIONED_MOT_FORGE_HAMMER, back
 * to 0 at frame 118 of STATIONED_MOT_FORGE_LAST.
 */
void Npc_State174(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        npc->stateVar0 = 0;
        w->motionPhase = 2;
        if (npc->motionId[0] != EM_MOTION_ID(0, STATIONED_MOT_FORGE_START)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_START), 4, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_HAMMER), 4, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->stateVar0++;
            if (npc->stateVar0 >= 2) {
                npc->step++;
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_FORGE_LAST), 4, 0, 0);
            }
        } else if (w->motionPhase == 0 && Motion_ReachedFrame(npc, 0, 30.0f) != 0) {
            w->motionPhase = 1;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, STATIONED_MOT_SMITH_IDLE), 4, 0, 0);
        } else if (w->motionPhase == 1 && Motion_ReachedFrame(npc, 0, 118.0f) != 0) {
            w->motionPhase = 0;
        }
        break;
    case 4:
        break;
    }
}

/*
 * NPC_STATE_MEAL_SERVED (as the player's TOWN_PC_MEAL_SERVED; the gallery's
 * meal scene sets it on both): plays motion 635. Case 0 advances the step
 * twice, so it stops in case 2. Steps 1 and 3 (seated at the table,
 * PL_MOT_TOWN_SEATED, and 635 again from frame 267) are not reached from here.
 */
void Npc_StateMealServed(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        Actor_SetMotion(npc, 635, 6, 0);
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
            Actor_SetMotion(npc, 635, 6, 200);
        }
        break;
    }
}

/*
 * NPC_STATE_MEAL_RISE_GOOD (as the player's TOWN_PC_MEAL_RISE_GOOD; the
 * gallery's meal scene sets it on both): stands, gets up (motion 610), then
 * plays the pleased pose 407 with an effect at bone 10, then stands (state
 * 1).
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
        Actor_SetMotion(npc, 610, 4, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step = (u8) (step + 1);
            Actor_SetMotion(npc, PL_MOT_ITEM_POSE, 6, 20);
            Effect06_SpawnAtBone((void *) npc, 0, 8, 10U, 4.0f);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, 1U);
        }
        break;
    }
}

/* NPC_STATE_ROUTE_JUMP, a route point: switches to the route in the entry's
 * argument and enters its first point's state. */
void Npc_StateRouteJump(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    w->route = *(NpcRouteEnt **)&w->route[w->routeIdx].arg;
    w->routeIdx = 0;
    Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
}

/*
 * NPC_STATE_ROUTE_JUMP_RANDOM, a route point: as NPC_STATE_ROUTE_JUMP, but one
 * time in ten it takes the next entry's route instead.
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

/* the NPCs' state handlers, by state (+0x15); 0 for NPC_STATE_LEAVE */
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
    case 70:
        Npc_State70(npc, 0);
        break;
    case 71:
        Npc_State70(npc, 1);
        break;
    case 58:
        Npc_State58(npc, 0);
        break;
    case 59:
        Npc_State58(npc, 1);
        break;
    case NPC_STATE_SEATED_AT_TABLE:
        Npc_StateSeatedAtTable(npc);
        break;
    case 72:
        Npc_State72(npc, 0);
        break;
    case 73:
        Npc_State72(npc, 1);
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
    case 74:
        Npc_State74(npc);
        break;
    case NPC_STATE_TALK_DELAY:
        Npc_StateTalkDelay(npc);
        break;
    case 75:
        Npc_State75(npc);
        break;
    case 76:
        Npc_State76(npc, 0);
        break;
    case 77:
        Npc_State76(npc, 1);
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
    case 63:
        Npc_State63(npc);
        break;
    case 64:
        Npc_State64(npc);
        break;
    case 46:
        Npc_State46(npc);
        break;
    case 47:
        Npc_State47(npc);
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
    case 123:
        Npc_State123(npc);
        break;
    case 124:
        Npc_State124(npc);
        break;
    case 125:
        Npc_State125(npc);
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
    case 133:
        Npc_State133(npc);
        break;
    case 134:
        Npc_State134(npc);
        break;
    case 135:
        Npc_State135(npc);
        break;
    case 136:
        Npc_State136(npc);
        break;
    case 137:
        Npc_State137(npc);
        break;
    case 138:
        Npc_State138(npc);
        break;
    case NPC_STATE_PAIR_STAND_3:
        Npc_StatePairStand(npc, 3);
        break;
    case 140:
        Npc_State140(npc);
        break;
    case NPC_STATE_CALL_OVER:
        Npc_StateCallOver(npc);
        break;
    case NPC_STATE_REACT_NEAR:
        Npc_StateReactNear(npc);
        break;
    case 146:
        Npc_State146(npc);
        break;
    case NPC_STATE_WAIT_TOWN_FREE:
        Npc_StateWaitTownFree(npc);
        break;
    case 148:
        Npc_State148(npc, 0U);
        break;
    case 149:
        Npc_State148(npc, 1U);
        break;
    case NPC_STATE_IDLE_BY_TIME:
        Npc_StateIdleByTime(npc);
        break;
    case 159:
        Npc_State159(npc);
        break;
    case 157:
        Npc_State157(npc);
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
    case 163:
        Npc_State163(npc);
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
    case 173:
        Npc_State173(npc);
        break;
    case 174:
        Npc_State174(npc);
        break;
    case NPC_STATE_ROUTE_JUMP_RANDOM:
        Npc_StateRouteJumpRandom(npc);
        break;
    case NPC_STATE_GONE:
        break;
    }
    return 1;
}
