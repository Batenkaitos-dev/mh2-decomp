/*
 * The townspeople's scripted scenes (gNpcScene, struct NpcScene): a
 * starter sets one up (NpcScene_Start) and waits for its end
 * (NpcScene_IsDone); every townsperson's state update runs its part each
 * frame (NpcScene_RunNpc). Online, the workshop's forging
 * (NPC_SCENE_WORKSHOP, NPC_SCENE_WORKSHOP_HELPER): the townsperson
 * talked to forges while three others hammer; in the house, the meal
 * (NPC_SCENE_COOKING): the kitchen helpers go to the seats of the
 * seat mask. The scenes move the townspeople by actor states and route
 * tables (Npc_SetLessonRoute, Npc_PlaceOnRoute). The lobby's frame clears
 * NpcScene.ticked, so the first townsperson to run counts the frame.
 * The .rodata holds the compiler's constants for NpcScene_RunNpc.
 */
#include "common.h"
#include "sprite.h"
#include "lobby_npc.h"
#include "m2c_macros.h"
#include "em.h"
#include "lobby_globals.h"
#include "scene.h"
#include "player.h"
#include "lobby_state.h"

void *memset(void *dst, s32 c, u32 n);
s32 OT_AddAt(u32 *table, void *node, s32 idx, s32 n);

/* Starts a scene of the given kind (TUTORIAL_KIND_*): clears gNpcScene and
 * keeps the seat mask (the cooking scene's seats, one bit each; 0 for the
 * workshop) and the meal's level (the kitchen helpers hand it on when they
 * serve). */
void NpcScene_Start(s8 sceneKind, s32 mask, s32 level)
{
    memset(&gNpcScene, 0, sizeof(NpcScene));
    gNpcScene.kind = sceneKind;
    gNpcScene.active = 1;
    gNpcScene.seatMask = mask;
    gNpcScene.mealLevel = level;
}


/* Whether the scene is over: 1 when none runs, else its done flag (0 or 1).
 * The starters wait for 1. */
s32 NpcScene_IsDone(void)
{
    s32 isDone;

    if (gNpcScene.active == 0) {
        return 1;
    }
    isDone = gNpcScene.done;
    isDone = isDone != 0;
    return isDone;
}

void Actor_SetState(void *, s32, s32);
s32 Motion_ReachedFrame(void *, s32, f32);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Npc_SetLessonRoute(Em *npc, s32 arg1);
void Npc_PlaceOnRoute(Em *arg0, u8 set, u8 i);
u8 Area_GetSetIndex(u16 id);

/*
 * Runs townsperson npc's part in the scene; each townsperson's state update
 * calls it every frame. The first call of a frame counts the frame (after
 * 5400 frames, 3 minutes, the scene ends as done) and the timer down.
 * - NPC_SCENE_WORKSHOP: townsperson 0x55 forges (state 0xA4); at its
 *   motion's frame 172 the three townspeople 0x59-0x5B hammer (state 0xA5,
 *   count: one call each); 30 frames on, townsperson 0x5C walks route
 *   table 1; 120 frames on, the three stop (state 0x9D); 0x5C walks route
 *   table 2, and once it is back in a plain state (not 2, 4, 0xAA or 0x81)
 *   the scene is done and 0x5C goes back to route table 0.
 * - NPC_SCENE_WORKSHOP_HELPER: NPC_WORKSHOP_HELPER_ONLINE leads instead:
 *   route table 4, its forging motion 0x40B to frame 74, the three hammer,
 *   it reaches state 0xAE with motion 0x406 and walks route table 5, the
 *   three stop, and once it is in a plain state (not 4, 0xAD or 0x81) the
 *   scene is done and it goes back to route table 3.
 * - NPC_SCENE_COOKING: only the kitchen helpers (townsperson 0x72) take
 *   part. Each frame the seats of the mask and the helpers present are
 *   counted and the route set is picked from the house's area; the helpers
 *   then take turns: in step 0 each of the first ones gets the next seat of
 *   the mask (Em.sceneSeat), is placed on that seat's route and enters
 *   its first point's state, the others go to route 0 with no seat (-1);
 *   step 1 waits until the seated helpers are all in state 0xB1, step 2
 *   moves them on to their next route point, and 30 frames later the scene
 *   is done.
 * Any other kind ends at once. The switches on the townsperson's state
 * list `default:` first: in that order the compiler leaves the branch delay
 * slots of the whole function empty, as the game's code has them.
 */
void NpcScene_RunNpc(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcScene *scene = &gNpcScene;
    s32 mask;
    s16 i;
    Em *em;
    s16 j;
    u8 k;

    if (gNpcScene.active == 0) {
        return;
    }
    if (scene->ticked == 0) {
        scene->ticked = 1;
        if (++scene->frames >= 5400) { /* frames: 3 min */
            scene->active = 0;
            scene->done = 1;
            return;
        }
        if (scene->timer > 0) {
            scene->timer--;
        }
        if (scene->kind == NPC_SCENE_COOKING) {
            scene->turn = 0;
            scene->seatIdx = 0;
        }
    }
    switch (scene->kind) {
    case NPC_SCENE_WORKSHOP:
        switch (scene->step) {
        case 0:
            if (w->id == 0x55) {
                scene->step++;
                Actor_SetState(npc, 0, NPC_STATE_FORGE);
                scene->timer = 10;
            }
            break;
        case 1:
            if (scene->timer <= 0 && w->id == 0x55 && Motion_ReachedFrame(npc, 0, 172.0f) != 0) {
                scene->step++;
                scene->count = 3;
            }
            break;
        case 2:
            switch (w->id) {
            case NPC_WORKSHOP_HAMMER_1:
            case NPC_WORKSHOP_HAMMER_2:
            case NPC_WORKSHOP_HAMMER_3:
                if (Player_IsState((Player *) npc, 0, NPC_STATE_HAMMER) == 0) {
                    Actor_SetState(npc, 0, NPC_STATE_HAMMER);
                }
                scene->count--;
                if (scene->count <= 0) {
                    scene->step++;
                    scene->timer = 30;
                }
                break;
            }
            break;
        case 3:
            if (scene->timer <= 0 && w->id == 0x5C) {
                scene->step++;
                scene->timer = 120;
                scene->count = 3;
                Npc_SetLessonRoute(npc, 1);
            }
            break;
        case 4:
            if (scene->timer <= 0) {
                switch (w->id) {
                case NPC_WORKSHOP_HAMMER_1:
                case NPC_WORKSHOP_HAMMER_2:
                case NPC_WORKSHOP_HAMMER_3:
                    if (Player_IsState((Player *) npc, 0, NPC_STATE_HAMMER) != 0) {
                        Actor_SetState(npc, 0, 0x9D);
                        npc->step = 3;
                    }
                    scene->count--;
                    if (scene->count <= 0) {
                        scene->step++;
                        scene->timer = 4;
                    }
                    break;
                }
            }
            break;
        case 5:
            if (scene->timer <= 0 && w->id == 0x5C) {
                scene->step++;
                Npc_SetLessonRoute(npc, 2);
            }
            break;
        case 6:
            if (w->id == 0x5C && npc->stateGroup == 0) {
                switch (npc->state) {
                default:
                    scene->step++;
                    scene->active = 0;
                    scene->done = 1;
                    Npc_SetLessonRoute(npc, 0);
                    break;
                case 2:
                case 4:
                case 0xAA:
                case 0x81:
                    break;
                }
            }
            break;
        }
        break;
    case NPC_SCENE_WORKSHOP_HELPER:
        switch (scene->step) {
        case 0:
            if (w->id == NPC_WORKSHOP_HELPER_ONLINE) {
                scene->step++;
                Npc_SetLessonRoute(npc, 4);
            }
            break;
        case 1:
            if (w->id == NPC_WORKSHOP_HELPER_ONLINE && npc->motionId[0] == 0x40B && Motion_ReachedFrame(npc, 0, 74.0f) != 0) {
                scene->step++;
                scene->count = 3;
            }
            break;
        case 2:
            switch (w->id) {
            case NPC_WORKSHOP_HAMMER_1:
            case NPC_WORKSHOP_HAMMER_2:
            case NPC_WORKSHOP_HAMMER_3:
                if (Player_IsState((Player *) npc, 0, NPC_STATE_HAMMER) == 0) {
                    Actor_SetState(npc, 0, NPC_STATE_HAMMER);
                }
                scene->count--;
                if (scene->count <= 0) {
                    scene->step++;
                    scene->timer = 30;
                }
                break;
            }
            break;
        case 3:
            if (w->id == NPC_WORKSHOP_HELPER_ONLINE && Player_IsState((Player *) npc, 0, 0xAE) != 0 && npc->motionId[0] == 0x406) {
                scene->step++;
                scene->count = 3;
                Npc_SetLessonRoute(npc, 5);
            }
            break;
        case 4:
            switch (w->id) {
            case NPC_WORKSHOP_HAMMER_1:
            case NPC_WORKSHOP_HAMMER_2:
            case NPC_WORKSHOP_HAMMER_3:
                if (Player_IsState((Player *) npc, 0, NPC_STATE_HAMMER) != 0) {
                    Actor_SetState(npc, 0, 0x9D);
                    npc->step = 3;
                }
                scene->count--;
                if (scene->count <= 0) {
                    scene->step++;
                    scene->timer = 4;
                }
                break;
            }
            break;
        case 5:
            if (w->id == NPC_WORKSHOP_HELPER_ONLINE && npc->stateGroup == 0) {
                switch (npc->state) {
                default:
                    scene->step++;
                    scene->active = 0;
                    scene->done = 1;
                    Npc_SetLessonRoute(npc, 3);
                    break;
                case 4:
                case 0xAD:
                case 0x81:
                    break;
                }
            }
            break;
        }
        break;
    case NPC_SCENE_COOKING:
        mask = scene->seatMask;
        scene->count = 0;
        for (i = 0; i < 16; i++) {
            if (mask & (1 << i)) {
                scene->count++;
            }
        }
        scene->helpers = 0;
        /* the kitchen helpers present (an Em's work block starts with
           NpcWork.id) */
        for (em = gEms, i = 0; i < 20; i++, em++) {
            if (em->active != 0 && em->visible != 0 && ((NpcWork *)&em->work)->id == NPC_KITCHEN_HELPER) {
                scene->helpers++;
            }
        }
        /* The route set of the house the player is in, by the area's index
           in the town's area list (Area_GetSetIndex): 11-13
           (AREA_ONLINE_HOUSE_0-2) sets 0-2, 10 (AREA_HUB_0x57) and 14
           (AREA_HOUSE) set 3. */
        switch (Area_GetSetIndex(gScene.area)) {
        case 10:
        case 14:
            scene->routeSet = NPC_ROUTE_SET_HOUSE;
            break;
        case 11:
        default:
            scene->routeSet = NPC_ROUTE_SET_ONLINE_HOUSE_0;
            break;
        case 12:
            scene->routeSet = NPC_ROUTE_SET_ONLINE_HOUSE_1;
            break;
        case 13:
            scene->routeSet = NPC_ROUTE_SET_ONLINE_HOUSE_2;
            break;
        }
        if (w->id != NPC_KITCHEN_HELPER) {
            return;
        }
        switch (scene->step) {
        case 0:
            if (scene->turn < scene->count) {
                k = 0;
                for (j = 0; j < 16; j++) {
                    if (mask & (1 << j)) {
                        if (k == scene->seatIdx) {
                            npc->sceneSeat = j;
                            break;
                        }
                        k++;
                    }
                }
                k = (u8)npc->sceneSeat;
                Npc_PlaceOnRoute(npc, scene->routeSet, k);
                w->routeIdx++;
                Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
                scene->seatIdx++;
            } else {
                Npc_PlaceOnRoute(npc, scene->routeSet, 0);
                npc->sceneSeat = -1;
            }
            scene->turn++;
            if (scene->turn >= scene->helpers) {
                scene->step++;
            }
            break;
        case 1:
            if (scene->turn < scene->count && Player_IsState((Player *) npc, 0, 0xB1) != 0) {
                scene->seatIdx++;
                if (scene->seatIdx >= scene->count || scene->seatIdx >= scene->helpers) {
                    scene->step++;
                }
            }
            scene->turn++;
            break;
        case 2:
            if (scene->turn < scene->count) {
                w->routeIdx++;
                Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
                scene->seatIdx++;
                if (scene->seatIdx >= scene->count || scene->seatIdx >= scene->helpers) {
                    scene->step++;
                    scene->timer = 30;
                }
            }
            scene->turn++;
            break;
        case 3:
            if (scene->timer <= 0) {
                scene->step++;
                scene->active = 0;
                scene->done = 1;
            }
            break;
        }
        break;
    default:
        scene->active = 0;
        scene->done = 1;
        break;
    }
}
