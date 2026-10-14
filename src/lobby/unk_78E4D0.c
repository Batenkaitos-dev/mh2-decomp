/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
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
extern LobbyMenu *gLobbyMenu;

void Tutorial_Start(s8 arg0, s32 mask, s32 level) {
    memset(&gTutorial, 0, 0x1C);
    gTutorial.kind = arg0;
    gTutorial.active = 1;
    gTutorial.mask = mask;
    gTutorial.unkC = level;
}


s32 Tutorial_IsDone(void)
{
    s32 r;

    if (gTutorial.active == 0) {
        return 1;
    }
    r = gTutorial.done;
    r = r != 0;
    return r;
}

void Actor_SetState(void *, s32, s32);
s32 Motion_ReachedFrame(void *, s32, f32);
s16 Player_IsState(void *obj, u8 state, u8 sub);
void Npc_SetRouteTable(Em *npc, s32 arg1);
void Npc_PlaceOnRoute(Em *arg0, u8 set, u8 i);
u8 Area_GetSetIndex(u16 id);

/* The village tutorial with the NPC arg0 (kind 0 and 1: two lessons, kind 2:
   the NPCs of t->mask take turns). The `default:`-first switches on the NPC
   state leave the conditional delay slots of the whole function empty, as
   in the game. */
void Tutorial_RunNpc(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Tutorial *t = &gTutorial;
    s32 mask;
    s16 i;
    Em *e;
    s16 j;
    u8 k;

    if (gTutorial.active == 0) {
        return;
    }
    if (t->ticked == 0) {
        t->ticked = 1;
        if (++t->frames >= 5400) {
            t->active = 0;
            t->done = 1;
            return;
        }
        if (t->timer > 0) {
            t->timer--;
        }
        if (t->kind == 2) {
            t->turn = 0;
            t->idx = 0;
        }
    }
    switch (t->kind) {
    case 0:
        switch (t->step) {
        case 0:
            if (w->id == 0x55) {
                t->step++;
                Actor_SetState(npc, 0, 0xA4);
                t->timer = 10;
            }
            break;
        case 1:
            if (t->timer <= 0 && w->id == 0x55 && Motion_ReachedFrame(npc, 0, 172.0f) != 0) {
                t->step++;
                t->count = 3;
            }
            break;
        case 2:
            switch (w->id) {
            case 0x59:
            case 0x5A:
            case 0x5B:
                if (Player_IsState((Player *) npc, 0, 0xA5) == 0) {
                    Actor_SetState(npc, 0, 0xA5);
                }
                t->count--;
                if (t->count <= 0) {
                    t->step++;
                    t->timer = 30;
                }
                break;
            }
            break;
        case 3:
            if (t->timer <= 0 && w->id == 0x5C) {
                t->step++;
                t->timer = 120;
                t->count = 3;
                Npc_SetRouteTable(npc, 1);
            }
            break;
        case 4:
            if (t->timer <= 0) {
                switch (w->id) {
                case 0x59:
                case 0x5A:
                case 0x5B:
                    if (Player_IsState((Player *) npc, 0, 0xA5) != 0) {
                        Actor_SetState(npc, 0, 0x9D);
                        npc->step = 3;
                    }
                    t->count--;
                    if (t->count <= 0) {
                        t->step++;
                        t->timer = 4;
                    }
                    break;
                }
            }
            break;
        case 5:
            if (t->timer <= 0 && w->id == 0x5C) {
                t->step++;
                Npc_SetRouteTable(npc, 2);
            }
            break;
        case 6:
            if (w->id == 0x5C && npc->stateGroup == 0) {
                switch (npc->state) {
                default:
                    t->step++;
                    t->active = 0;
                    t->done = 1;
                    Npc_SetRouteTable(npc, 0);
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
    case 1:
        switch (t->step) {
        case 0:
            if (w->id == 0x58) {
                t->step++;
                Npc_SetRouteTable(npc, 4);
            }
            break;
        case 1:
            if (w->id == 0x58 && npc->motionId[0] == 0x40B && Motion_ReachedFrame(npc, 0, 74.0f) != 0) {
                t->step++;
                t->count = 3;
            }
            break;
        case 2:
            switch (w->id) {
            case 0x59:
            case 0x5A:
            case 0x5B:
                if (Player_IsState((Player *) npc, 0, 0xA5) == 0) {
                    Actor_SetState(npc, 0, 0xA5);
                }
                t->count--;
                if (t->count <= 0) {
                    t->step++;
                    t->timer = 30;
                }
                break;
            }
            break;
        case 3:
            if (w->id == 0x58 && Player_IsState((Player *) npc, 0, 0xAE) != 0 && npc->motionId[0] == 0x406) {
                t->step++;
                t->count = 3;
                Npc_SetRouteTable(npc, 5);
            }
            break;
        case 4:
            switch (w->id) {
            case 0x59:
            case 0x5A:
            case 0x5B:
                if (Player_IsState((Player *) npc, 0, 0xA5) != 0) {
                    Actor_SetState(npc, 0, 0x9D);
                    npc->step = 3;
                }
                t->count--;
                if (t->count <= 0) {
                    t->step++;
                    t->timer = 4;
                }
                break;
            }
            break;
        case 5:
            if (w->id == 0x58 && npc->stateGroup == 0) {
                switch (npc->state) {
                default:
                    t->step++;
                    t->active = 0;
                    t->done = 1;
                    Npc_SetRouteTable(npc, 3);
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
    case 2:
        mask = t->mask;
        t->count = 0;
        for (i = 0; i < 16; i++) {
            if (mask & (1 << i)) {
                t->count++;
            }
        }
        t->npcs = 0;
        for (e = gEms, i = 0; i < 20; i++, e++) {
            if (e->active != 0 && e->visible != 0 && e->work == 0x72) {
                t->npcs++;
            }
        }
        switch (Area_GetSetIndex(gScene.area)) {
        case 10:
        case 14:
            t->set = 3;
            break;
        case 11:
        default:
            t->set = 0;
            break;
        case 12:
            t->set = 1;
            break;
        case 13:
            t->set = 2;
            break;
        }
        if (w->id != 0x72) {
            return;
        }
        switch (t->step) {
        case 0:
            if (t->turn < t->count) {
                k = 0;
                for (j = 0; j < 16; j++) {
                    if (mask & (1 << j)) {
                        if (k == t->idx) {
                            npc->tutorialSlot = j;
                            break;
                        }
                        k++;
                    }
                }
                k = (u8)npc->tutorialSlot;
                Npc_PlaceOnRoute(npc, t->set, k);
                w->routeIdx++;
                Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
                t->idx++;
            } else {
                Npc_PlaceOnRoute(npc, t->set, 0);
                npc->tutorialSlot = -1;
            }
            t->turn++;
            if (t->turn >= t->npcs) {
                t->step++;
            }
            break;
        case 1:
            if (t->turn < t->count && Player_IsState((Player *) npc, 0, 0xB1) != 0) {
                t->idx++;
                if (t->idx >= t->count || t->idx >= t->npcs) {
                    t->step++;
                }
            }
            t->turn++;
            break;
        case 2:
            if (t->turn < t->count) {
                w->routeIdx++;
                Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
                t->idx++;
                if (t->idx >= t->count || t->idx >= t->npcs) {
                    t->step++;
                    t->timer = 30;
                }
            }
            t->turn++;
            break;
        case 3:
            if (t->timer <= 0) {
                t->step++;
                t->active = 0;
                t->done = 1;
            }
            break;
        }
        break;
    default:
        t->active = 0;
        t->done = 1;
        break;
    }
}

void LobbyMenu_AddHudLayers(void) {
    OT_AddAt(gDrawListText6, gLobbyMenu->layer[5], 0, 1);
    OT_AddAt(gDrawListText0b, gLobbyMenu->layer[6], 0, 1);
    OT_AddAt(gDrawListText0a, gLobbyMenu->layer[7], 0, 1);
}
