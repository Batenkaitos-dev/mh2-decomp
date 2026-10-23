/*
 * The two route setters that walk the hairdresser to the hair menu's chair and
 * back, the tests of what a townsperson is (by its body, NpcWork.bodyKind, its
 * look, NpcWork.look and Em.variant, or its id), the online workshop lessons' routes
 * (Npc_SetLessonRoute) and the Poogie's placing by the player
 * (Poogie_PlaceByPlayer). The townsperson's state update (Npc_UpdateState),
 * the route stepping, the turns and the state handlers follow in the next
 * file.
 */
#include "common.h"
#include "em.h"
#include "lobby_npc.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "lobby_menu.h"
#include "engine_math.h"
#include "my_house.h"
#include "town_players.h"

extern NpcRouteEnt gLessonRouteBHome[];
extern NpcRouteEnt gLessonRouteB1[];
extern NpcJumpRoute4End gLessonRouteB2;
extern NpcRouteEnt gLessonRouteAHome[];
extern NpcRouteEnt gLessonRouteA1[];
extern NpcJumpRoute3End gLessonRouteA2;

extern NpcRouteEnt gNpcRouteToHairChair[];
extern NpcRouteEnt gNpcRouteFromHairChair[];

/* Puts the townsperson (NPC_HAIRDRESSER, from HairMenu_MovePlayer) on the route
 * to the hair menu's chair (gNpcRouteToHairChair) from its start and enters the
 * first point's state. */
void Npc_SetRouteToHairChair(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    npcWork->route = gNpcRouteToHairChair;
    npcWork->routeIdx = 0;
    Actor_SetState(npc, 0, (u16)npcWork->route[npcWork->routeIdx].state);
}

/* Puts the townsperson on the route back from the hair menu's chair
 * (gNpcRouteFromHairChair) from its start and enters the first point's state. */
void Npc_SetRouteFromHairChair(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    npcWork->route = gNpcRouteFromHairChair;
    npcWork->routeIdx = 0;
    Actor_SetState(npc, 0, (u16)npcWork->route[npcWork->routeIdx].state);
}

/* 1 when Npc_IsFree and the look (NpcWork.look) is 0 or 43-48;
 * Npc_StartTalkState gives these state 47. */
s32 Npc_IsFreeAct2Bto30(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->look) {
        case 0:
        case 43:
        case 46:
        case 48:
        case 44:
        case 45:
        case 47:
            return 1;
        }
    }
    return 0;
}

/* 1 when Npc_IsFree and the look (NpcWork.look) is 4, 21-23 or 25
 * (not 24); their idles use motions 675 and 676. */
s32 Npc_IsFreeAct15to19(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->look) {
        case 4:
        case 21:
        case 23:
        case 25:
        case 22:
            return 1;
        }
    }
    return 0;
}

/* 1 when Npc_IsFree and the look (NpcWork.look) is 0-2, 21, 24, 26,
 * 34 or 37; their idles use motions 63 and 64. */
s32 Npc_IsFreeAct18Set(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->look) {
        case 21:
        case 24:
        case 26:
        case 34:
        case 37:
        case 0:
        case 1:
        case 2:
            return 1;
        }
    }
    return 0;
}

/* 1 for body 1 (NpcWork.bodyKind) except the looks 60, 61 and 75-80: its
 * head is bone 12 (Npc_UpdateMatrix turns it; Npc_Init takes a position from it)
 * and the lock-on mark has its own height for it. */
s32 Npc_HasSecondHead(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npcWork->bodyKind) {
    case 1:
        switch (npcWork->look) {
        case 75:
        case 79:
        case 80:
        case 60:
        case 61:
        case 76:
        case 78:
        case 77:
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 1 when Npc_IsFree and it is townsperson 10 or 46 (their idles use motion
 * 683 instead of 680). */
s32 Npc_IsFreeId0Aor2E(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->id) {
        case 46:
        case 10:
            return 1;
        }
    }
    return 0;
}

/* 1 for NPC_WORKSHOP_SMITH on body 1: Npc_StartTalkState gives it NPC_STATE_SMITH_TALK, and
 * Npc_HidePart treats it with the other workshop townspeople. */
s32 Npc_IsWorkshopSmith(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (npcWork->bodyKind == 1 && npcWork->id == NPC_WORKSHOP_SMITH) {
        return 1;
    }
    return 0;
}

/* 1 when Npc_IsFree and the look (NpcWork.look) is 41 or 42;
 * Npc_StartTalkState gives these NPC_STATE_GREET_TALK. */
s32 Npc_IsFreeAct29or2A(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->look) {
        case 41:
        case 42:
            return 1;
        }
    }
    return 0;
}

/* 0 for the special looks 77-85 (Em.npcLook, the same byte as
 * NpcWork.look: the music actor, the pets and the looks ServiceNpc_RunState runs)
 * and for body 1 (NpcWork.bodyKind) unless its look (Em.variant) is 61; else 1.
 * "Free" is not "idle": the lock-on mark's height, the idle motions and the
 * bone Npc_Init takes a position from go by it. */
s32 Npc_IsFree(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npc->npcLook) {
    case 77:
    case 78:
    case 79:
    case 80:
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
        return 0;
    }
    if (npcWork->bodyKind == 1 && npc->variant != 61) {
        return 0;
    }
    return 1;
}

/* 1 for the looks 81-85 (Em.npcLook), whose states ServiceNpc_RunState runs;
 * Npc_Init takes a position from their bone 10 and the lock-on mark has its own
 * height for them. */
s32 Npc_IsAct51to55(Em *npc)
{
    switch (npc->npcLook) {
    case 81:
    case 82:
    case 83:
    case 84:
    case 85:
        return 1;
    }
    return 0;
}

/* 1 for the pets' looks 79 and 80 (Em.npcLook, the same byte as
 * NpcWork.look): Npc_UpdateState runs their states with Pet_RunState, the
 * PET_STATE_* handlers. */
s32 Npc_IsPet(Em *em)
{
    u8 look;

    look = em->npcLook;
    if ((look == 79) || (look == 80)) {
        return 1;
    }
    return 0;
}

/* 1 for look 77: Npc_SetupDraw gives it body 1's handler table, the talk
 * keeps the camera off it, and the lock-on mark has its own height for it. */
s32 Npc_IsType77(Em *npc)
{
    return npc->variant == 77;
}

/* 1 for the look 78, the townsperson the lobby music's motion scripts move
 * (Npc_Init hands it to LobbyBgm_SetActor): while that music plays, Npc_Run and
 * Npc_UpdateState leave it alone. */
s32 Npc_IsMusicActor(Em *npc)
{
    return npc->variant == 78;
}

/* 1 for the looks 49-54: they keep their route point while the drinking
 * contest runs (Npc_UpdateState), and their idles use motions of their own
 * (858, 865). */
s32 Npc_IsType31to36(Em *npc)
{
    u8 look;

    look = npc->variant;
    switch (look) {
    case 49:
    case 50:
    case 51:
    case 52:
    case 53:
    case 54:
        return 1;
    default:
        return 0;
    }
}

/* 1 for NPC_DRINK_CONTEST_HOST and townsperson 30, the two who hold a drink
 * mug (Npc_SetupFromArea spawns it: DrinkMug_Spawn). */
s32 Npc_HasDrinkMug(Em *npc)
{
    switch (((NpcWork *)&npc->work)->id) {
    case NPC_DRINK_CONTEST_HOST:
    case 30:
        return 1;
    }
    return 0;
}

s32 Npc_IsType40(Em *npc)
{
    return npc->variant == 64;
}

s32 Npc_IsType52(Em *npc)
{
    return npc->variant == 82;
}

/* 1 for the looks 60-62: Npc_SetupDraw gives them gNpcCommonHandlers (the motion
 * sounds) whatever their body, and their head stays still (Npc_UpdateMatrix). */
s32 Npc_IsType3Cto3E(Em *npc)
{
    switch (npc->variant) {
    case 60:
    case 61:
    case 62:
        return 1;
    }
    return 0;
}

s32 Npc_IsType4B(Em *npc)
{
    return npc->variant == 75;
}

s32 Npc_IsType49(Em *npc)
{
    return npc->variant == 73;
}

/* Puts the townsperson on route arg1 (0-5) of the online workshop lessons
 * (NpcScene_RunNpc) from its start and enters the first point's state. Lesson
 * NPC_SCENE_WORKSHOP walks townsperson 92 on routes 1 and 2 and back on 0;
 * NPC_SCENE_WORKSHOP_HELPER walks NPC_WORKSHOP_HELPER_ONLINE on 4 and 5 and
 * back on 3. */
void Npc_SetLessonRoute(Em *npc, s32 routeNo)
{
    u32 routeSel;
    NpcWork *npcWork;

    routeSel = routeNo & 0xFF;
    npcWork = (NpcWork *)&npc->work;
    switch (routeSel) {
    case 0:
        npcWork->route = gLessonRouteAHome;
        break;
    case 1:
        npcWork->route = gLessonRouteA1;
        break;
    case 2:
        npcWork->route = gLessonRouteA2.pt;
        break;
    case 3:
        npcWork->route = gLessonRouteBHome;
        break;
    case 4:
        npcWork->route = gLessonRouteB1;
        break;
    case 5:
        npcWork->route = gLessonRouteB2.pt;
        break;
    }
    npcWork->routeIdx = 0;
    Npc_EnterRoutePoint(npc);
}

extern u8 gPoogieByPlayer;

/* Puts the Poogie (NPC_POOGIE) 130 units from the local player (the offset
 * (0, 0, -1) turned by the player's yaw), facing the player's way, when its route
 * starts with PET_STATE_FOLLOW, or in an online house while gPoogieByPlayer is 1.
 * Returns 1 when it was placed, 0 when not or when it isn't in the area. */
s32 Poogie_PlaceByPlayer(void)
{
    f32 offset[3];
    Player *pl;
    Em *npc;
    f32 *y;
    f32 *z;

    pl = &gPlayers[gScene.localPlayer];
    npc = Npc_Find(NPC_POOGIE);
    if (npc == NULL) {
        return 0;
    }
    if (((NpcWork *)&npc->work)->route->state != PET_STATE_FOLLOW) {
        if (Area_IsOnlineHouse(gScene.area) != 1 || gPoogieByPlayer != 1) {
            return 0;
        }
    }
    offset[0] = 0.0f;
    y = &offset[1];
    z = &offset[2];
    *y = 0.0f;
    *z = -1.0f;
    Vec_RotateY(offset, (6.2831855f * (f32)pl->rot[1]) / 65536.0f);
    offset[0] *= 130.0f;
    *y *= 130.0f;
    *z *= 130.0f;
    npc->posX = pl->pos[0] + offset[0];
    npc->posY = pl->pos[1] + *y;
    npc->posZ = pl->pos[2] + *z;
    npc->rotY = pl->rot[1];
    return 1;
}
