/*
 * The pets' state handlers: what the Poogie (NPC_POOGIE), the care pet
 * (NPC_POOGIE_CARE) and the house pet (NPC_HOUSE_PET), the townspeople of
 * variants 79 and 80, do in each actor state (Em.state). Npc_UpdateState
 * runs Pet_RunState, the dispatcher at the file's end, for them.
 *
 * The file holds, in order: standing and sitting, the petting and its
 * answers (the face, anger, joy, great joy), running off along a route of
 * AREA_HUB_0x89's zones (gNpc4FArea*, gNpc4FRoute*), wandering, following
 * the local player or the naming talker, waiting, digging, and a few
 * lookups (Pet_FindNpc, ActSpot_FindFirstOfKind).
 *
 * The last data, gDecoFrameColors (the menus' decorated frame colours, read
 * by Menu_DrawDecoFrame and the quest desk's drawing, by nothing here), sits
 * at the end of this file's data in the game, so it is defined here.
 */

#include "common.h"
#include "lobby_npc.h"
#include "npc_motion.h"
#include "em.h"
#include "player.h"
#include "save.h"
#include "area.h"
#include "lobby_menu.h"
#include "quest.h"
#include "capsule.h"
#include "pet_states.h"
#include "engine_math.h"
#include "game_util.h"
#include "mine_model_objects.h"
#include "my_house.h"
#include "rand.h"
#include "town_area_change.h"

/* unit's own declaration: a u16 state (the definition takes s32): the callers here pass the state as a halfword, and Pet_StateRunOff, Pet_StateRunRoute and Pet_StateWander compile as the game's code only with this type */
void Actor_SetState(void *obj, s32 group, u16 state);

void Npc_StartMoveTo(Em *obj, s32 n);
extern s32 gTownMode;
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);

/*
 * The pets' zones and run-off routes in AREA_HUB_0x89 (Pet_StateRunOff), the
 * zone where the Poogie waits below a climbing player (gNpc4FArea), and the
 * quest desk's frame colours (gDecoFrameColors, read by the quest desk's
 * drawing and the menu sheets).
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

/*
 * PET_STATE_IDLE (flag 0) and state 89 (flag 1): stands (motion PET_MOT_IDLE, blended
 * over 2 or 8 frames).
 */
void Pet_StateIdle(Em *pet, s8 slowBlend)
{
    switch (pet->step) {
    case 0:
        pet->step++;
        if (slowBlend == 0) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_IDLE), 2, 0, 0);
        } else {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_IDLE), 8, 0, 0);
        }
        break;
    }
}

/*
 * NPC_STATE_HOUSE_OPEN: while the house is open to visitors the pet sits
 * (motion PET_MOT_SIT; already seated, from its frame 88), its root movement
 * not applied.
 */
void Pet_StateHouseOpen(Em *pet)
{
    switch (pet->step) {
    case 0:
        pet->step++;
        Player_SetFlag((Player *) pet, EM_FLAG_NO_ROOT_MOTION);
        if (pet->motionId[0] != EM_MOTION_ID(0, PET_MOT_SIT)) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_SIT), 14, 0, 0);
        } else {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_SIT), 0, 88, 0);
        }
        break;
    case 1:
        break;
    }
}

/*
 * States 82 (flag 0: placed already seated, from frame 88 of motion
 * PET_MOT_SIT) and 83 (flag 1: sits down): sits by the player, its root
 * movement not applied. It stays while the player is in the save menu in bed
 * (TOWN_MODE_SAVE with TOWN_PC_BED_LIE_DOWN or TOWN_PC_BED_SLEEP), or else
 * while the player is within 130 of the spot it walked to. Then it gets up
 * (PET_MOT_STAND_UP), stands 30 frames (counted in motionPhase) and follows
 * again (PET_STATE_FOLLOW).
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
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_SIT), 0, 88, 0);
        } else {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_SIT), 14, 0, 0);
        }
        break;
    case 1:
        if (gTownMode == TOWN_MODE_SAVE) {
            if (pl->state != TOWN_PC_BED_LIE_DOWN && pl->state != TOWN_PC_BED_SLEEP) {
                Player_ClearFlag((Player *) pet, EM_FLAG_NO_ROOT_MOTION);
                Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STAND_UP), 4, 0, 0);
                pet->step++;
            }
        } else if (!(Vec_Distance((f32 *) pl->pos, w->followSpot.v) <= 130.0f)) {
            Player_ClearFlag((Player *) pet, EM_FLAG_NO_ROOT_MOTION);
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STAND_UP), 4, 0, 0);
            pet->step++;
        }
        break;
    case 2:
        if (pet->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_IDLE), 10, 0, 0);
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

/*
 * PET_STATE_PETTED: puts the local player in the petting pose
 * (TOWN_PC_GRABBED). Over 10 frames the pet turns toward the player it touches
 * (Em.contact). It then steps back to 78 from the player over 20 frames if
 * nearer, and keeps turning to face the player.
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
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_TURN_TO_PLAYER), 2, 0, 0);
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
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STEP_BACK), 4, 0, 0);
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
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_TURN_TO_PLAYER), 2, 0, 0);
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

extern s8 gPetFollows;

/*
 * PET_STATE_ANGRY: the petting angered the pet (PET_ACT_RUN_OFF). The Poogie
 * stops following (gPetFollows). Anger marks pop up every 3-7 frames
 * (ModelObj_SpawnOn, counted in motionPhase). It rushes at three times the
 * speed for 20 frames, then knocks the local player down (TOWN_PC_THROWN). Once
 * the player stands again (TOWN_PC_IDLE), it runs off (PET_STATE_RUN_OFF).
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
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_CHARGE), 4, 38, 0);
        pet->timer = 20;
        break;
    case 1:
        if (--pet->timer <= 0) {
            pet->step++;
            pet->timer = 5;
            pet->motionSpeed = 1.0f;
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_TACKLE), 4, 0, 0);
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, TOWN_PC_THROWN);
        }
        break;
    case 2:
        if (pet->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_IDLE), 4, 0, 0);
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


/*
 * PET_STATE_JOY: plays its joy (motion PET_MOT_JOY, with a mark over it) as
 * PET_ACT_REACT, then PET_MOT_STOP for 100 frames, then stands and follows
 * (PET_STATE_FOLLOW).
 */
void Pet_StateJoy(Em *pet)
{
    NpcWork *w = (NpcWork *)&pet->work;

    switch (pet->step) {
    case 0:
        w->petAct = PET_ACT_REACT;
        pet->step++;
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_JOY), 4, 0, 0);
        ModelObj_SpawnOn(pet, 0);
        break;
    case 1:
        if (pet->motion[0].playing <= 0) {
            w->petAct = PET_ACT_NONE;
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STOP), 6, 0, 0);
            pet->step++;
            pet->timer = 100;
        }
        break;
    case 2:
        if (--pet->timer <= 0) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_IDLE), 24, 0, 0);
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

/* PET_STATE_GREAT_JOY: plays its great joy (motion PET_MOT_GREAT_JOY) as
 * PET_ACT_REACT, then follows (PET_STATE_FOLLOW). */
void Pet_StateGreatJoy(Em *pet)
{
    NpcWork *w = (NpcWork *)&pet->work;

    switch (pet->step) {
    case 0:
        w->petAct = PET_ACT_REACT;
        pet->step++;
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_GREAT_JOY), 4, 0, 0);
        break;
    case 1:
        if (pet->motion[0].playing <= 0) {
            w->petAct = PET_ACT_NONE;
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_IDLE), 2, 0, 0);
            pet->step++;
        }
        break;
    case 2:
        Actor_SetState(pet, 0, PET_STATE_FOLLOW);
        break;
    }
}

u8 *ActSpot_GetList(u16 area);
/* Pos_InZone gives 0 or 1. Pet_StateRunOff masks the result to a byte before it
 * compares, while Pet_StateFollow and Pet_StateWait compare the whole register.
 * So the declaration says s32, and Pet_StateRunOff has (u8) casts: both forms
 * then compile as the game's code. */
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
 * Elsewhere it turns over 10 frames toward the area's first action spot and
 * runs that way, at double speed (normal speed in the online houses). After 270
 * frames it stops (PET_ACT_NONE) and stays there (NPC_STATE_GONE).
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
        Actor_SetMotionOnLayer(obj, EM_MOTION_ID(0, PET_MOT_RUN), 2, 0, 0);
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
            Actor_SetState(obj, 0, 96);
        }
        break;
    case 3:
        break;
    }
}

/* An identity helper: through it the entry address (routeIdx * 20, one
   NpcRouteEnt each) is added index first, as
   the game does after stepping the route (Pet_StateRunRoute). */
static inline s32 Npc_RouteAddr(NpcRouteEnt *route)
{
    return (s32)route;
}

/*
 * State 175: runs along its route (motion PET_MOT_RUN), turning a fifth of the
 * way each frame. Within 30 of a point it steps on: an end entry (state 0)
 * starts the route over, an entry of another state is entered at once, and an
 * entry of this state keeps it running.
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
        if (pet->motionId[0] != EM_MOTION_ID(0, PET_MOT_RUN)) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_RUN), 2, 0, 0);
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
            next = (NpcRouteEnt *)(Npc_RouteAddr(w->route) + w->routeIdx * 20);
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
 * State 87: walks along its route (motion PET_MOT_WALK), turning a tenth of
 * the way each frame. At each point it reached it stops half the time for
 * motion PET_MOT_STOP. Otherwise it goes on as state 175 does. While the house
 * is open to visitors it sits instead (NPC_STATE_HOUSE_OPEN).
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
        if (pet->motionId[0] == EM_MOTION_ID(0, PET_MOT_STOP)) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WALK), 8, 0, 0);
        } else {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WALK), 4, 0, 0);
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
            next = (NpcRouteEnt *)(Npc_RouteAddr(w->route) + w->routeIdx * 20);
            if (next->state == 0) {
                w->routeIdx = 0;
            } else if ((u16)Rand_Next(1) & 1) {
                pet->step++;
                Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STOP), 10, 0, 0);
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

extern f32 gNpc4FArea[];

/*
 * PET_STATE_FOLLOW: follows the local player. Before the steps it checks four
 * cases:
 * - while the house is open to visitors, it sits (NPC_STATE_HOUSE_OPEN);
 * - offline, before the Poogie has a name, every pet but NPC_HOUSE_PET follows
 *   the naming talker instead (state 92);
 * - the Poogie inside gNpc4FArea waits (state 93) while the player climbs or
 *   stands above it;
 * - NPC_HOUSE_PET in AREA_HOUSE waits.
 * Em.petWaitBelow is 1 while it waits that way.
 * Step 0 picks a spot within 99 of the player and walks (motion PET_MOT_WALK).
 * Step 1 walks there, at 2.5 times the speed offline in AREA_HUB_0x89 and
 * AREA_HUB_0x57. More than 5000 away, it is put at a fixed entry point (by the
 * player's side of the town) and walks in from there for 200 frames without
 * wall collision. While the player lies in bed it stands within 65, plays
 * motion PET_MOT_STOP (steps 2-3), then sits down (state 83). Near the spot, with
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
        Actor_SetState(pet, 0, 92);
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
            Actor_SetState(pet, 0, 93);
            return;
        }
    }
    if (w->id == NPC_HOUSE_PET && gScene.area == AREA_HOUSE) {
        pet->petWaitBelow = 1;
        Actor_SetState(pet, 0, 93);
        return;
    }
    switch (pet->step) {
    case 0:
        w->petCount = 0;
        pet->fieldCollide = 1;
        pet->step++;
        if (pet->motionId[0] != EM_MOTION_ID(0, PET_MOT_WALK)) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WALK), 4, 0, 0);
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
                Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_IDLE), 4, 0, 0);
            } else if (pet->motionId[0] != EM_MOTION_ID(0, PET_MOT_WALK)) {
                Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WALK), 2, 0, 0);
            }
        } else if (dist < 130.0f) {
            if (!(Vec_Distance(w->followSpot.v, (f32 *) pl->pos) <= 130.0f)) {
                pet->step--;
            } else {
                if (pet->motionId[0] != EM_MOTION_ID(0, PET_MOT_IDLE)) {
                    Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_IDLE), 4, 0, 0);
                }
                if (w->motionPhase != 0 && w->warpTimer == 0) {
                    if (--w->motionPhase <= 0) {
                        pet->motionSpeed = 1.0f;
                        Actor_SetState(pet, 0, 83);
                    }
                } else {
                    w->motionPhase = ((u16)Rand_Next(1) % 5 + 3) * 15;
                }
            }
        } else if (pet->motionId[0] != EM_MOTION_ID(0, PET_MOT_WALK)) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WALK), 2, 0, 0);
        }
        break;
    case 2:
        if (pet->motion[0].playing <= 0) {
            pet->step++;
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STOP), 10, 0, 0);
        }
        break;
    case 3:
        if (pet->motion[0].playing <= 0) {
            Actor_SetState(pet, 0, 83);
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
    if (w->petCount > 120) {
        w->petCount = 0;
        pet->motionSpeed = 1.0f;
        Actor_SetState(pet, 0, 83);
    }
}

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
 * State 92: offline, before the Poogie has a name, it follows the naming
 * talker (other, NPC_NAMING_TALKER) and sets gPoogieFollowing. It picks a spot
 * within 99 of the talker and walks there without wall collision. Its speed
 * eases between 0.5 and 1.2: faster, and not talkable, while more than 150
 * away. More than 5000 away it is put 150 beside the talker. Within 65 of a
 * spot the talker has left, it picks a new one. With no talker it stands
 * (PET_STATE_IDLE). While the talker talks (NPC_STATE_TALK) it waits (state
 * 93).
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
    if (talker->state == 44) {
        pet->motionSpeed = 1.0f;
        Actor_SetState(pet, 0, 93);
        return;
    }
    switch (pet->step) {
    case 0:
        pet->motionSpeed = 0.5f;
        pet->fieldCollide = 0;
        pet->step++;
        if (pet->motionId[0] != EM_MOTION_ID(0, PET_MOT_WALK)) {
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WALK), 4, 0, 0);
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
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_SIT), 14, 0, 0);
        break;
    case 1:
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WAIT_B), 10, 0, 0);
        pet->stateVar0 = 1;
        pet->stateVar1 = 0;
        break;
    case 2:
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WAIT_A), 10, 0, 0);
        pet->stateVar0 = 1;
        pet->stateVar1 = 0;
        break;
    case 3:
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_WAIT_C), 10, 0, 0);
        pet->stateVar0 = 1;
        pet->stateVar1 = 0;
        break;
    default:
        pet->stateVar0 = 2;
        pet->step = 1;
        break;
    }
}


/*
 * State 93: waits.
 * - Following the naming talker (other): with no talker it stands
 *   (PET_STATE_IDLE). Once the talker stops talking it follows the player if the
 *   Poogie has a name (PET_ACT_NONE in AREA_ONLINE_HOUSE_2), else the talker
 *   again (state 92).
 * - Waiting below the player (Em.petWaitBelow): once its motion ends, NPC_HOUSE_PET in
 *   AREA_HUB_0x57 follows again. The Poogie follows again once out of gNpc4FArea,
 *   or once the player is no longer climbing nor above it.
 * Meanwhile it waits 30 frames, then turns to the local player playing motion
 * PET_MOT_STOP, then plays idle motions picked by Pet_PickWaitMotion. A sit lasts
 * 100-355 frames, then it gets up (PET_MOT_STAND_UP).
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
        } else if (talker->state != 44) {
            if (pet->step == 1) {
                if ((s8)((SaveData *)save)->poogieName[0] != 0) {
                    Actor_SetState(pet, 0, PET_STATE_FOLLOW);
                    if (gScene.area == AREA_ONLINE_HOUSE_2) {
                        w->petAct = PET_ACT_NONE;
                    }
                } else {
                    Actor_SetState(pet, 0, 92);
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
                Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STOP), 10, 0, 0);
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
                Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_SIT), 14, 0, 0);
                break;
            case 1:
                if (--w->petCount <= 0) {
                    pet->stateVar1++;
                }
                break;
            case 2:
                Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STAND_UP), 4, 0, 0);
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

/* State 171: plays motion PET_MOT_STOP over and over. */
void Npc79_State171(Em *pet)
{
    switch (pet->step) {
    case 0:
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STOP), 10, 0, 0);
        pet->step++;
        break;
    case 1:
        if (pet->motion[0].playing <= 0) {
            pet->step = 0;
        }
        break;
    }
}

extern u8 *gPoogieDigSpots;
extern u8 gPoogieDigSpotCount;
extern s16 gPoogieDigTimer;
extern u8 gPoogieDigSpotIdx;
extern u8 gPoogieDigState;
extern u8 gPoogieDigTaken;

/*
 * PET_STATE_DIG: the pet digs (PET_ACT_DIG). After 1800 frames
 * (gPoogieDigTimer), or in an area with no TOWN_SPOT_POOGIE_DIG spot, it gives
 * up: PET_ACT_NONE, and Pet_EndDig once its motion ends. Otherwise, at the
 * start (unless the dig is POOGIE_DIG_DONE), it picks one of the
 * gPoogieDigSpotCount spots of gPoogieDigSpots at random. It writes that spot's
 * position into its route's first point and runs there at double speed
 * (POOGIE_DIG_GOING). Within 100 it plays motion PET_MOT_STOP. It stops
 * (POOGIE_DIG_DONE, Pet_EndDig) when the find is taken (gPoogieDigTaken), or
 * once it has arrived and the local player comes within 100.
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
        spot += gPoogieDigSpots[gPoogieDigSpotIdx] * 24;
        target[0] = ((ActionSpot *)spot)->x;
        target[1] = ((ActionSpot *)spot)->y;
        target[2] = ((ActionSpot *)spot)->z;
        gPoogieDigState = POOGIE_DIG_GOING;
        pet->motionSpeed *= 2.0f;
        Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_RUN), 2, 0, 0);
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
            Actor_SetMotionOnLayer(pet, EM_MOTION_ID(0, PET_MOT_STOP), 10, 0, 0);
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

/* Whether the local player is within 100 of the pet on the ground plane (x and
 * z only). */
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


/*
 * The first spot of the given kind (ActionSpot.kind, enum TownSpotKind) in the
 * area's action spot list: the one whose own id (ActionSpot.type) is 0. The 40
 * entries are searched, 24 bytes each (an ActionSpot). Returns NULL when
 * there is none. The Poogie's dig spots are counted on from it
 * (gPoogieDigSpots).
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
            return spots + i * 24;
        }
        i++;
        entry += 12;
    } while (i < 40);
    return NULL;
}

/*
 * The pets' handler (looks 79 and 80) by Em.state; NPC_STATE_GONE runs
 * nothing. In AREA_HOUSE and AREA_HUB_0x57 the pet's area follows the scene's.
 * The pet can't be talked to while it acts (petAct), nor while the Poogie has
 * no name and has not followed the naming talker yet (gPoogieFollowing).
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
    case 89:
        Pet_StateIdle(pet, 1);
        break;
    case 82:
        Pet_StateSit(pet, 0);
        break;
    case 83:
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
    case 87:
        Pet_StateWander(pet);
        break;
    case PET_STATE_FOLLOW:
        Pet_StateFollow(pet);
        break;
    case 92:
        Pet_StateFollowTalker(pet, talker);
        break;
    case 93:
        Pet_StateWait(pet, talker);
        break;
    case 171:
        Npc79_State171(pet);
        break;
    case PET_STATE_DIG:
        Pet_StateDig(pet);
        break;
    case 175:
        Pet_StateRunRoute(pet);
        break;
    case 96:
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
