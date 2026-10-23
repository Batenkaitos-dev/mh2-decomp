/*
 * The service townspeople's state handlers: what the house cook, the
 * hairdresser, the bait seller and townsperson 73, the townspeople of
 * variants 81-85, do in each actor state (Em.state). Npc_UpdateState runs
 * ServiceNpc_RunState, the dispatcher at the file's end, for them.
 *
 * The file holds, in order: busy, standing, walking, the fidgets and the
 * greetings; the hairdresser's states around the hair menu; the talk, the
 * bait seller's offer and the end of a service; the cook's kitchen route
 * (Npc_SetKitchenRoute, Npc_AdvanceRoute) and the placing of a townsperson
 * on a route (Npc_PlaceOnRoute).
 */

#include "common.h"
#include "lobby_npc.h"
#include "npc_motion.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "lobby_menu.h"
#include "sound.h"
#include "quest.h"
#include "service_npc_states.h"
#include "engine_math.h"
#include "field_objects.h"
#include "gallery_objects.h"
#include "game_util.h"
#include "part_effects.h"
#include "rand.h"
#include "town_area_change.h"
#include "vec_math.h"
#include "bgm.h"

/* unit's own declaration: a u16 state (the definition takes s32): the callers here pass the state as a halfword, and ServiceNpc_StateHairDo, ServiceNpc_StateTalk, ServiceNpc_StateServed, Npc_SetKitchenRoute and Npc_PlaceOnRoute compile as the game's code only with this type */
void Actor_SetState(void *obj, s32 group, u16 state);

void Npc_StartMoveTo(Em *obj, s32 n);
extern s32 gTownMode;
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);

/*
 * States 78 and 108 (flag 0 or 1, unused): busy. Model part set 2 shows,
 * and it does not turn when talked to; it holds motion SERVICE_MOT_BUSY. Its talk
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
        if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_BUSY)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_BUSY), 4, 0, 0);
        }
        break;
    case 1:
        break;
    }
}

extern s8 gTownModeStep;
extern s8 D_008BEB3F;

/*
 * States NPC_STATE_IDLE and 89: stands (motion SERVICE_MOT_IDLE; variant 82
 * its own SERVICE_MOT_ALT_IDLE) with model part set 0. NPC_HOUSE_COOK goes to
 * state 81 at step 2 of town mode 15 while a town byte, not yet named, is 0.
 * The others (not variant 82) greet the local player once when within 130
 * (state 80); otherwise, after a wait of up to 255 frames (255-510 after a
 * greeting), they fidget (state 79). motionPhase is 1 once greeted, until the
 * player leaves.
 */
void ServiceNpc_StateIdle(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        w->partSet = 0;
        w->keepFacing = 0;
        npc->timer = (u8)((u16)Rand_Next(1) + 255);
        if (Npc_IsType52(npc) == 1) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_ALT_IDLE), 4, 0, 0);
        } else if (w->id == NPC_HOUSE_COOK || w->id == 73) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), -20, 0, 0);
        } else {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 20, 0, 0);
        }
        break;
    case 1:
        if (w->id == NPC_HOUSE_COOK) {
            if (gTownMode == 15 && D_008BEB3F == 0 && gTownModeStep == 2 && w->motionPhase == 0) {
                Actor_SetState(npc, 0, 81);
            }
        } else if (Npc_IsType52(npc) == 0) {
            if (Vec_Distance(&npc->posX, (f32 *) gPlayers[gScene.localPlayer].pos) < 130.0f) {
                if (w->motionPhase == 0) {
                    Actor_SetState(npc, 0, 80);
                    w->motionPhase = 1;
                    npc->timer = (u8)Rand_Next(1) + 255;
                }
            } else if (--npc->timer <= 0) {
                Actor_SetState(npc, 0, 79);
            } else {
                w->motionPhase = 0;
            }
        }
        break;
    }
}

/*
 * Walks to the current route point; kind picks the motion: 0 (state 3) a walk
 * (variant 82 its own), 1 (state 2) motion SERVICE_MOT_WALK_ALT, 2 (state 4)
 * a run (SERVICE_MOT_RUN), 3 (state 176) carrying: it first spawns its cooking
 * lesson object (ThrownObj_Spawn with its sceneSeat). It turns a tenth of the
 * way each frame and takes the next point within 80.
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
                if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_ALT_WALK)) {
                    Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_ALT_WALK), 4, 0, 0);
                }
            } else if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_WALK)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_WALK), 4, 0, 0);
            }
            break;
        case 1:
            if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_WALK_ALT)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_WALK_ALT), 4, 0, 0);
            }
            break;
        case 2:
            if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_RUN)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_RUN), 4, 0, 0);
            }
            break;
        case 3:
            ThrownObj_Spawn(npc, npc->sceneSeat, (u8)gNpcScene.mealLevel);
            if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_WALK_CARRY)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_WALK_CARRY), 4, 0, 0);
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

/* State 79: a fidget (motion SERVICE_MOT_FIDGET), then stands again (state
 * 1). */
void ServiceNpc_StateFidget(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_FIDGET), 4, 0, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 8, 0, 0);
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
 * State 80: greets the local player. It turns toward the player by a tenth of
 * the angle after 10 frames, nods (motion SERVICE_MOT_GREET), turns back and
 * stands again (state 1).
 */
void ServiceNpc_StateGreet(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        npc->targetYaw = (s16)(u16)Actor_HeadingDiff(npc, gPlayers[gScene.localPlayer].pos) / 10;
        npc->timer = 10;
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_GREET), 8, 0, 0);
        break;
    case 1:
        if (--npc->timer <= 0) {
            npc->rotY += (s16)npc->targetYaw;
            npc->step++;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 10, 0, 0);
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

/*
 * State 170: as the greeting (state 80) with motion
 * SERVICE_MOT_GREET_EFFECT. Once, at frame 62, it puts a PartFx 29 30 ahead
 * of it and 100 up, and plays common sound 154.
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
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_GREET_EFFECT), 4, 0, 0);
        break;
    case 1:
        if (--npc->timer <= 0) {
            npc->rotY += (s16)npc->targetYaw;
            npc->step++;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 10, 0, 0);
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
            PartFx_SpawnAt(pos, 29, 1.0f);
            Sound_PlaySeFlat(SE_SLOT_COMMON, 154, 0);
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

/*
 * NPC_STATE_HAIR_WAIT: the hair menu sets it on the hairdresser while the
 * player gets ready. Variant 85 holds motion SERVICE_MOT_HAIR_WAIT; the
 * others play one of three idle motions at random, over and over. The motion's
 * root movement is not applied while SERVICE_MOT_STEP_IN_PLACE plays.
 */
void ServiceNpc_StateHairWait(Em *npc)
{
    u16 pick;

    switch (npc->step) {
    case 0:
        if (npc->variant == 85) {
            if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_HAIR_WAIT)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_HAIR_WAIT), 4, 0, 0);
            }
        } else {
            pick = (u16)Rand_Next(1) % 3;
            switch (pick) {
            case 0:
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 4, 0, 0);
                break;
            case 1:
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_WAIT), 4, 0, 0);
                break;
            case 2:
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_STEP_IN_PLACE), 4, 0, 0);
                break;
            default:
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_FIDGET), 4, 0, 0);
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
    if (npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_STEP_IN_PLACE)) {
        Player_SetFlag((Player *) npc, EM_FLAG_NO_ROOT_MOTION);
    } else {
        Player_ClearFlag((Player *) npc, EM_FLAG_NO_ROOT_MOTION);
    }
}

/*
 * NPC_STATE_HAIR_DONE: can't be talked to. Coming from motion
 * SERVICE_MOT_HAIR_WAIT it is put at the point (2698, 0, 2930) and walks on to
 * its next route point. Otherwise it plays 1082, and once the local player is
 * within 200 it plays 1083 (talkable again from frame 240). It is then put back
 * at that point, facing angle 0, and goes on along its route.
 */
void Npc81_State152(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&npc->work;

    w->noTalk = 1;
    switch (npc->step) {
    case 0:
        if (npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_HAIR_WAIT)) {
            npc->posX = 2698.0f;
            npc->posY = 0.0f;
            npc->posZ = 2930.0f;
            *(NpcPos *)npc->goal = *(NpcPos *)(&npc->posX);
            Npc_NextRoutePoint(npc);
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_WALK), 0, 0, 0);
        } else if (npc->motionId[0] != 1082) {
            Actor_SetMotionOnLayer(npc, 1082, 4, 0, 0);
        }
        npc->step++;
        break;
    case 1:
        if (Vec_Distance((f32 *) pl->pos, &npc->posX) < 200.0f) {
            Actor_SetMotionOnLayer(npc, 1083, 6, 0, 0);
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
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_WALK), 0, 0, 0);
        }
        break;
    }
}

/* State 177: holds motion SERVICE_MOT_HAIR_PREPARE from frame 140. */
void Npc81_State177(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_HAIR_PREPARE), 4, 140, 0);
        break;
    case 1:
        break;
    }
}

/* State 178: plays motion 1074, then 1072 for at most 100 frames, then goes on
 * to the next route point. */
void Npc81_State178(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, 1074, 4, 0, 0);
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            npc->timer = 100;
            Actor_SetMotionOnLayer(npc, 1072, 6, 12, 0);
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

/*
 * NPC_STATE_HAIR_PREPARE: the hair menu sets it on the hairdresser once the
 * player stands ready. It plays motion SERVICE_MOT_HAIR_PREPARE, spawns the
 * hair menu's object at frame 48 (ThrownObj_SpawnForHairMenu), plays 1074 from
 * frame 100, then stands (SERVICE_MOT_IDLE; the hair menu waits for that
 * motion).
 */
void ServiceNpc_StateHairPrepare(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_HAIR_PREPARE), 0, 0, 0);
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
            Actor_SetMotionOnLayer(npc, 1074, 4, 30, 0);
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 30, 0, 0);
            npc->step++;
        }
        break;
    }
}

/*
 * NPC_STATE_HAIR_DO: the hair menu sets it once the player sits in the chair.
 * It does the hair: motion SERVICE_MOT_HAIR_CUT (particles from the player at
 * its frames 120 and 200), then SERVICE_MOT_HAIR_CUT_NEXT, then
 * SERVICE_MOT_HAIR_DONE (NPC_HAIRDRESSER) or SERVICE_MOT_FINISH. It then takes
 * its route from the hair chair (Npc_SetRouteFromHairChair) and enters that
 * route point's state.
 */
void ServiceNpc_StateHairDo(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];

    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_HAIR_CUT)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_HAIR_CUT), 2, 0, 0);
            npc->timer = 4;
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0 && --npc->timer <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_HAIR_CUT_NEXT), 4, 0, 0);
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            if (w->id == NPC_HAIRDRESSER) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_HAIR_DONE), 4, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_FINISH), 4, 0, 0);
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
    if (npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_HAIR_CUT)) {
        if (Motion_PassedFrame((Player *) npc, 0, 200.0f) != 0) {
            ActorParticles_Spawn(pl, npc, 1);
        } else if (Motion_PassedFrame((Player *) npc, 0, 120.0f) != 0) {
            ActorParticles_Spawn(pl, npc, 2);
        }
    }
}

/*
 * NPC_STATE_TALK: the talk with the player. While busy (motion
 * SERVICE_MOT_BUSY, states 78 and 108) it goes straight back to its route
 * point's state. Otherwise it can't be talked to again meanwhile. Unless its
 * state keeps it facing its way, it turns toward the local player over 10
 * frames. It plays its talk motion (NPC_HAIRDRESSER SERVICE_MOT_HAIR_WAIT,
 * variant 82 SERVICE_MOT_ALT_TALK, SERVICE_MOT_BAIT_OFFER_END after the bait
 * pose SERVICE_MOT_BAIT_OFFER, else SERVICE_MOT_GREET then SERVICE_MOT_TALK).
 * Once the town is back to TOWN_MODE_FREE it waits 16 frames (32, standing, for
 * looks 81-85 in SERVICE_MOT_TALK), turns back and enters its route point's
 * state. When that entry's argument is -1, it also starts a 20-frame move to
 * the point. Without a route it stands (state 1).
 */
void ServiceNpc_StateTalk(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *entry;

    switch (npc->motionId[0]) {
    case EM_MOTION_ID(0, SERVICE_MOT_BUSY):
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
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_HAIR_WAIT), 4, 0, 0);
            } else if (Npc_IsType52(npc) == 1) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_ALT_TALK), 4, 0, 0);
            } else if (npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_BAIT_OFFER)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_BAIT_OFFER_END), 2, 0, 0);
            } else {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_GREET), 4, 0, 0);
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
            if ((npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_GREET) || npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_BAIT_OFFER_END)) && npc->motion[0].playing <= 0) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_TALK), 4, 0, 0);
            }
            if (gTownMode == TOWN_MODE_FREE) {
                npc->timer = 16;
                npc->step++;
                if (Npc_IsAct51to55(npc) == 1 && npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_TALK)) {
                    npc->timer = 32;
                    Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 16, 0, 0);
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

/* State 158: the bait menu sets it on the bait seller: holds the bait pose
 * (motion SERVICE_MOT_BAIT_OFFER). */
void ServiceNpc_StateBaitOffer(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_BAIT_OFFER)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_BAIT_OFFER), 4, 0, 0);
        }
        break;
    case 1:
        break;
    }
}

/*
 * State 85: set after a service, by the kitchen and the kitchen guest on the
 * cook after the meal and by the bait menu on the bait seller. From the bait
 * pose it plays SERVICE_MOT_BAIT_OFFER_END, then SERVICE_MOT_FINISH.
 * NPC_HOUSE_COOK then waits until, in TOWN_MODE_KITCHEN_GUEST, the local player
 * stands (TOWN_PC_IDLE). It then goes back to its route's first point.
 */
void ServiceNpc_StateServed(Em *npc)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_BAIT_OFFER)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_BAIT_OFFER_END), 2, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_FINISH)) {
                Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_FINISH), 16, 0, 0);
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

/* State 168, a route point: stops the town's event jingle (Bgm_StopEventFade)
 * if one plays. */
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

extern NpcJumpRoute6End gNpcRouteKitchen;
extern NpcJumpRoute8End gKitchenRoute0;
extern NpcJumpRoute11End gKitchenRoute1;
extern NpcJumpRoute7End gKitchenRoute2;

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
        w->route = gNpcRouteKitchen.pt;
    } else {
        switch (spot) {
        default:
            w->route = gKitchenRoute0.pt;
            break;
        case 1:
            w->route = gKitchenRoute1.pt;
            break;
        case 2:
            w->route = gKitchenRoute2.pt;
            break;
        }
    }
    w->routeIdx = 0;
    Actor_SetState(npc, 0, w->route[w->routeIdx].state);
}

/* Steps to the next route point and enters its state (the kitchen moves the
 * cook on with it). */
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
 * It enters the first point's state; a first point of NPC_STATE_ROUTE_TURN
 * instead sets its facing from the entry's argument (not talkable) and starts
 * at the next point.
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
    if ((w->route)[w->routeIdx].state == 129) {
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

/* State 180: a fidget (motion SERVICE_MOT_FIDGET), then stands 60 frames, then
 * goes on to the next route point. */
void ServiceNpc_StateFidgetPause(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_FIDGET)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_FIDGET), 4, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 8, 0, 0);
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

/*
 * State 181: walks in place twice (motion SERVICE_MOT_STEP_IN_PLACE, its root
 * movement not applied), plays SERVICE_MOT_FINISH, stands 8 frames, then goes
 * on to the next route point.
 */
void ServiceNpc_StateStepInPlace(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        if (npc->motionId[0] != EM_MOTION_ID(0, SERVICE_MOT_STEP_IN_PLACE)) {
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_STEP_IN_PLACE), -4, 0, 0);
        }
        break;
    case 1:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_STEP_IN_PLACE), -4, 0, 0);
            npc->timer = 60;
        }
        break;
    case 2:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_FINISH), 4, 0, 0);
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            npc->step++;
            Actor_SetMotionOnLayer(npc, EM_MOTION_ID(0, SERVICE_MOT_IDLE), 4, 0, 0);
            npc->timer = 8;
        }
        break;
    case 4:
        if (--npc->timer <= 0) {
            Npc_NextRoutePoint(npc);
        }
        break;
    }
    if (npc->motionId[0] == EM_MOTION_ID(0, SERVICE_MOT_STEP_IN_PLACE)) {
        Player_SetFlag((Player *) npc, EM_FLAG_NO_ROOT_MOTION);
    } else {
        Player_ClearFlag((Player *) npc, EM_FLAG_NO_ROOT_MOTION);
    }
}

/*
 * The service townspeople's handler (looks 81-85) by Em.state; the route
 * NPC_STATE_ROUTE_TURN, NPC_STATE_ROUTE_JUMP, NPC_STATE_ROUTE_JUMP_RANDOM and
 * NPC_STATE_WARP_TO_ROUTE are the general ones. The cook and townsperson 73
 * can't be talked to outside states 1, 79 and 80 (standing, fidgeting,
 * greeting). Returns 0 for NPC_STATE_LEAVE, else 1.
 */
s32 ServiceNpc_RunState(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->state) {
    case 89:
    case 1:
        ServiceNpc_StateIdle(npc);
        break;
    case 151:
        ServiceNpc_StateHairWait(npc);
        break;
    case 152:
        Npc81_State152(npc);
        break;
    case 3:
        ServiceNpc_StateWalk(npc, 0);
        break;
    case 2:
        ServiceNpc_StateWalk(npc, 1);
        break;
    case 78:
        ServiceNpc_StateBusy(npc, 0);
        break;
    case 108:
        ServiceNpc_StateBusy(npc, 1);
        break;
    case 44:
        ServiceNpc_StateTalk(npc);
        break;
    case 79:
        ServiceNpc_StateFidget(npc);
        break;
    case 80:
        ServiceNpc_StateGreet(npc);
        break;
    case 90:
        Npc_StateWarpToRoute(npc);
        break;
    case 153:
        ServiceNpc_StateHairPrepare(npc);
        break;
    case 129:
        Npc_StateRouteTurn(npc);
        break;
    case 155:
        ServiceNpc_StateHairDo(npc);
        break;
    case 158:
        ServiceNpc_StateBaitOffer(npc);
        break;
    case 85:
        ServiceNpc_StateServed(npc);
        break;
    case 4:
        ServiceNpc_StateWalk(npc, 2);
        break;
    case 168:
        ServiceNpc_StateStopJingle(npc);
        break;
    case 131:
        Npc_StateRouteJump(npc);
        break;
    case 170:
        ServiceNpc_StateGreetFx(npc);
        break;
    case 95:
        Npc_StateLeave(npc);
        return 0;
    case 176:
        ServiceNpc_StateWalk(npc, 3);
        break;
    case 177:
        Npc81_State177(npc);
        break;
    case 178:
        Npc81_State178(npc);
        break;
    case 180:
        ServiceNpc_StateFidgetPause(npc);
        break;
    case 181:
        ServiceNpc_StateStepInPlace(npc);
        break;
    case 179:
        Npc_StateRouteJumpRandom(npc);
        break;
    }
    if ((w->id == NPC_HOUSE_COOK || w->id == 73) && npc->state != 1 && npc->state != 79 && npc->state != 80) {
        w->noTalk = 1;
    }
    return 1;
}
