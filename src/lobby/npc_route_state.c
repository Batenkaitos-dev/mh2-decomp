/*
 * The two route setters that walk the hairdresser to the hair menu's chair and
 * back, the tests of what a townsperson is (by its body, NpcWork.bodyKind, its
 * look, NpcWork.look and Em.variant, or its id), the online workshop lessons' routes
 * (Npc_SetLessonRoute), the Poogie's placing by the player (Poogie_PlaceByPlayer)
 * and the townsperson's state update (Npc_UpdateState): its state handler by
 * look, its move and turn, the wait at its route point and the step to the next,
 * and the route checks by time and side. Npc_HoldRoute, Npc_EnterRoutePoint and
 * Npc_HideAndAdvance are the route's small steps. The route stepping, the turns
 * and the state handlers follow in the next file.
 */
#include "common.h"
#include "em.h"
#include "lobby_npc.h"
#include "m2c_macros.h"
#include "item.h"
#include "scene.h"
#include "player.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "lobby_menu.h"

void Npc_EnterRoutePoint(Em *npc);
extern NpcRouteEnt gLessonRouteBHome[];
extern NpcRouteEnt gLessonRouteB1[];
extern NpcRouteEnt gLessonRouteB2[];
extern NpcRouteEnt gLessonRouteAHome[];
extern NpcRouteEnt gLessonRouteA1[];
extern NpcRouteEnt gLessonRouteA2[];
s32 Npc_IsPet(Em *arg0);
s32 Npc_IsType4D(Em *arg0);
s32 Npc_IsMusicActor(Em *arg0);
s32 Npc_IsType40(Em *arg0);
s32 Npc_IsType52(Em *arg0);
s32 Npc_IsType4B(Em *arg0);
s32 Npc_IsType49(Em *arg0);
void Npc_HoldRoute(Em *arg0);
void Npc_HideAndAdvance(Em *arg0);

void Actor_SetState(void *obj, s32 group, s32 state);
s32 Npc_IsFree(Em *arg0);
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

/* 1 when Npc_IsFree and the look (NpcWork.look) is 0 or 0x2B-0x30;
 * Npc_StartTalkState gives these state 0x2F. */
s32 Npc_IsFreeAct2Bto30(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->look) {
        case 0:
        case 0x2B:
        case 0x2E:
        case 0x30:
        case 0x2C:
        case 0x2D:
        case 0x2F:
            return 1;
        }
    }
    return 0;
}

/* 1 when Npc_IsFree and the look (NpcWork.look) is 4, 0x15-0x17 or 0x19
 * (not 0x18); their idles use motions 0x2A3 and 0x2A4. */
s32 Npc_IsFreeAct15to19(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->look) {
        case 4:
        case 0x15:
        case 0x17:
        case 0x19:
        case 0x16:
            return 1;
        }
    }
    return 0;
}

/* 1 when Npc_IsFree and the look (NpcWork.look) is 0-2, 0x15, 0x18, 0x1A,
 * 0x22 or 0x25; their idles use motions 0x3F and 0x40. */
s32 Npc_IsFreeAct18Set(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->look) {
        case 0x15:
        case 0x18:
        case 0x1A:
        case 0x22:
        case 0x25:
        case 0:
        case 1:
        case 2:
            return 1;
        }
    }
    return 0;
}

/* 1 for body 1 (NpcWork.bodyKind) except the looks 0x3C, 0x3D and 0x4B-0x50: its
 * head is bone 0xC (Npc_UpdateMatrix turns it; Npc_Init takes a position from it)
 * and the lock-on mark has its own height for it. */
s32 Npc_HasSecondHead(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npcWork->bodyKind) {
    case 1:
        switch (npcWork->look) {
        case 0x4B:
        case 0x4F:
        case 0x50:
        case 0x3C:
        case 0x3D:
        case 0x4C:
        case 0x4E:
        case 0x4D:
            return 0;
        }
        return 1;
    }
    return 0;
}

/* 1 when Npc_IsFree and it is townsperson 0x0A or 0x2E (their idles use motion
 * 0x2AB instead of 0x2A8). */
s32 Npc_IsFreeId0Aor2E(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->id) {
        case 0x2E:
        case 0xA:
            return 1;
        }
    }
    return 0;
}

/* 1 for NPC_WORKSHOP_SMITH on body 1: Npc_StartTalkState gives it state 0x68, and
 * Npc_HidePart treats it with the other workshop townspeople. */
s32 Npc_IsWorkshopSmith(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (npcWork->bodyKind == 1 && npcWork->id == NPC_WORKSHOP_SMITH) {
        return 1;
    }
    return 0;
}

/* 1 when Npc_IsFree and the look (NpcWork.look) is 0x29 or 0x2A;
 * Npc_StartTalkState gives these state 0x3E. */
s32 Npc_IsFreeAct29or2A(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    if (Npc_IsFree(npc) == 1) {
        switch (npcWork->look) {
        case 0x29:
        case 0x2A:
            return 1;
        }
    }
    return 0;
}

/* 0 for the special looks 0x4D-0x55 (Em.npcLook, the same byte as
 * NpcWork.look: the music actor, the pets and the looks ServiceNpc_RunState runs)
 * and for body 1 (NpcWork.bodyKind) unless its look (Em.variant) is 0x3D; else 1.
 * "Free" is not "idle": the lock-on mark's height, the idle motions and the
 * bone Npc_Init takes a position from go by it. */
s32 Npc_IsFree(Em *npc)
{
    NpcWork *npcWork = (NpcWork *)&npc->work;

    switch (npc->npcLook) {
    case 0x4D:
    case 0x4E:
    case 0x4F:
    case 0x50:
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
        return 0;
    }
    if (npcWork->bodyKind == 1 && npc->variant != 0x3D) {
        return 0;
    }
    return 1;
}

/* 1 for the looks 0x51-0x55 (Em.npcLook), whose states ServiceNpc_RunState runs;
 * Npc_Init takes a position from their bone 0xA and the lock-on mark has its own
 * height for them. */
s32 Npc_IsAct51to55(Em *npc)
{
    switch (npc->npcLook) {
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
        return 1;
    }
    return 0;
}

/* 1 for the pets' looks 0x4F and 0x50 (Em.npcLook, the same byte as
 * NpcWork.look): Npc_UpdateState runs their states with Pet_RunState, the
 * PET_STATE_* handlers. */
s32 Npc_IsPet(Em *em)
{
    u8 look;

    look = em->npcLook;
    if ((look == 0x4F) || (look == 0x50)) {
        return 1;
    }
    return 0;
}

/* 1 for the look 0x4D: Npc_SetupDraw gives it body 1's handler table, the talk
 * keeps the camera off it, and the lock-on mark has its own height for it. */
s32 Npc_IsType4D(Em *npc)
{
    return npc->variant == 0x4D;
}

/* 1 for the look 0x4E, the townsperson the lobby music's motion scripts move
 * (Npc_Init hands it to LobbyBgm_SetActor): while that music plays, Npc_Run and
 * Npc_UpdateState leave it alone. */
s32 Npc_IsMusicActor(Em *npc)
{
    return npc->variant == 0x4E;
}

/* 1 for the looks 0x31-0x36: they keep their route point while the drinking
 * contest runs (Npc_UpdateState), and their idles use motions of their own
 * (0x35A, 0x361). */
s32 Npc_IsType31to36(Em *npc)
{
    u8 look;

    look = npc->variant;
    switch (look) {
    case 0x31:
    case 0x32:
    case 0x33:
    case 0x34:
    case 0x35:
    case 0x36:
        return 1;
    default:
        return 0;
    }
}

/* 1 for NPC_DRINK_CONTEST_HOST and townsperson 0x1E, the two who hold a drink
 * mug (Npc_SetupFromArea spawns it: DrinkMug_Spawn). */
s32 Npc_HasDrinkMug(Em *npc)
{
    switch (((NpcWork *)&npc->work)->id) {
    case NPC_DRINK_CONTEST_HOST:
    case 0x1E:
        return 1;
    }
    return 0;
}

s32 Npc_IsType40(Em *npc)
{
    return npc->variant == 0x40;
}

s32 Npc_IsType52(Em *npc)
{
    return npc->variant == 0x52;
}

/* 1 for the looks 0x3C-0x3E: Npc_SetupDraw gives them gNpcCommonHandlers (the motion
 * sounds) whatever their body, and their head stays still (Npc_UpdateMatrix). */
s32 Npc_IsType3Cto3E(Em *npc)
{
    switch (npc->variant) {
    case 0x3C:
    case 0x3D:
    case 0x3E:
        return 1;
    }
    return 0;
}

s32 Npc_IsType4B(Em *npc)
{
    return npc->variant == 0x4B;
}

s32 Npc_IsType49(Em *npc)
{
    return npc->variant == 0x49;
}

/* Puts the townsperson on route arg1 (0-5) of the online workshop lessons
 * (NpcScene_RunNpc) from its start and enters the first point's state. Lesson
 * NPC_SCENE_WORKSHOP walks townsperson 0x5C on routes 1 and 2 and back on 0;
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
        npcWork->route = gLessonRouteA2;
        break;
    case 3:
        npcWork->route = gLessonRouteBHome;
        break;
    case 4:
        npcWork->route = gLessonRouteB1;
        break;
    case 5:
        npcWork->route = gLessonRouteB2;
        break;
    }
    npcWork->routeIdx = 0;
    Npc_EnterRoutePoint(npc);
}

Em *Npc_Find(u8);
void Vec_RotateY(f32 *v, f32 angle);
s32 Area_IsOnlineHouse(s32);
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

s32 LobbyBgm_IsActive(void);
void NpcScene_RunNpc(Em *npc);
s32 ServiceNpc_RunState(Em *a);
s32 Pet_RunState(Em *a);
s32 Npc_RunState(Em *a);
void Npc_StepRouteMove(Em *obj);
void Npc_StepTurn(Em *arg0);
extern s32 gTownMode;
s32 Npc_CheckHubPartner(Em *npc);
s32 Npc_NextRoutePoint(Em *a);
void Npc_CheckTimeRoute(Em *npc, NpcWork *t);
void Npc_CheckSideSwap(Em *npc, NpcWork *t);

/* A townsperson's state update for this frame (Npc_Run calls it). Unless it is the
 * music actor while the lobby music plays (Npc_IsMusicActor, LobbyBgm_IsActive), it
 * clears NpcWork.noTalk, runs the online workshop lesson's step for it
 * (NpcScene_RunNpc) and its state handler by look (ServiceNpc_RunState for 0x51-0x55,
 * Pet_RunState for the pets, else Npc_RunState), then its move and turn. On a
 * route, outside the states that keep it where it is and while no menu or contest
 * holds it (the drinking contest's looks 0x31-0x36, NPC_CONTEST_HOST, the three
 * pets) and its route isn't held, it counts NpcWork.wait at the route point: the
 * point's argument is the frames to wait, -1 to stay (with state 0x7A when
 * Npc_CheckHubPartner answers 1), -2 to wait for the motion to end; then
 * Npc_NextRoutePoint. Last come the route checks by time (only when flag ends 0:
 * it stays, or Npc_NextRoutePoint answered 0) and by side; townsperson 0x72 can
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
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
        ret = ServiceNpc_RunState(npc);
        break;
    case 0x4F:
    case 0x50:
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
    case 0x3:
    case 0x2:
    case 0x4:
    case 0x8E:
    case 0x2C:
    case 0x42:
    case 0x2F:
    case 0x5A:
    case 0x7A:
    case 0x81:
    case 0x82:
    case 0x93:
    case 0x54:
    case 0x55:
    case 0x5E:
    case 0x56:
    case 0x52:
    case 0x53:
    case 0x57:
    case 0x58:
    case 0x5C:
    case 0x5D:
    case 0x5B:
    case 0xA3:
    case 0x6C:
    case 0x5F:
    case 0x60:
    case 0x99:
    case 0x9B:
    case 0x97:
    case 0xA4:
    case 0xA5:
    case 0xB0:
    case 0xB6:
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
    if (npcWork->id == 0x72) {
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
