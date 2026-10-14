/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
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

void Npc_EnterRoutePoint(Em *npc);
extern NpcRouteEnt gNpcRouteSel3[];
extern NpcRouteEnt gNpcRouteSel4[];
extern NpcRouteEnt gNpcRouteSel5[];
extern NpcRouteEnt gNpcRouteSel0[];
extern NpcRouteEnt gNpcRouteSel1[];
extern NpcRouteEnt gNpcRouteSel2[];
s32 Npc_IsAct4For50(Em *arg0);
s32 Npc_IsType4D(Em *arg0);
s32 Npc_IsType4E(Em *arg0);
s32 Npc_IsType40(Em *arg0);
s32 Npc_IsType52(Em *arg0);
s32 Npc_IsType4B(Em *arg0);
s32 Npc_IsType49(Em *arg0);
void Npc_HoldRoute(Em *arg0);
void Npc_HideAndAdvance(Em *arg0);

s32 Npc_IsAct4For50(Em *em) {
    u8 act;

    act = em->npcAct;
    if ((act == 0x4F) || (act == 0x50)) {
        return 1;
    }
    return 0;
}

s32 Npc_IsType4D(Em *npc) {
    return npc->variant == 0x4D;
}

s32 Npc_IsType4E(Em *npc) {
    return npc->variant == 0x4E;
}

s32 Npc_IsType31to36(Em *npc)
{
    u8 type;

    type = npc->variant;
    switch (type) {
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

s32 Npc_IsId10or1E(Em *npc)
{
    switch (npc->work) {
    case 0x10:
    case 0x1E:
        return 1;
    }
    return 0;
}

s32 Npc_IsType40(Em *npc) {
    return npc->variant == 0x40;
}

s32 Npc_IsType52(Em *npc) {
    return npc->variant == 0x52;
}

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

s32 Npc_IsType4B(Em *npc) {
    return npc->variant == 0x4B;
}

s32 Npc_IsType49(Em *npc) {
    return npc->variant == 0x49;
}

void Npc_SetRouteTable(Em *npc, s32 arg1)
{
    u32 temp_v0;
    NpcWork *w;

    temp_v0 = arg1 & 0xFF;
    w = (NpcWork *)&npc->work;
    switch (temp_v0) {
    case 0:
        w->route = gNpcRouteSel0;
        break;
    case 1:
        w->route = gNpcRouteSel1;
        break;
    case 2:
        w->route = gNpcRouteSel2;
        break;
    case 3:
        w->route = gNpcRouteSel3;
        break;
    case 4:
        w->route = gNpcRouteSel4;
        break;
    case 5:
        w->route = gNpcRouteSel5;
        break;
    }
    w->routeIdx = 0;
    Npc_EnterRoutePoint(npc);
}

Em *Npc_Find(u8);
void Vec_RotateY(f32 *v, f32 angle);
s32 Area_IsOnlineHouse(s32);
extern u8 gNpc14ByPlayer;

s32 Npc14_PlaceByPlayer(void)
{
    f32 v[3];
    Player *pl;
    Em *npc;
    f32 *y;
    f32 *z;

    pl = &gPlayers[gScene.localPlayer];
    npc = Npc_Find(0x14);
    if (npc == NULL) {
        return 0;
    }
    if (((NpcWork *)&npc->work)->route->state != 0x58) {
        if (Area_IsOnlineHouse(gScene.area) != 1 || gNpc14ByPlayer != 1) {
            return 0;
        }
    }
    v[0] = 0.0f;
    y = &v[1];
    z = &v[2];
    *y = 0.0f;
    *z = -1.0f;
    Vec_RotateY(v, (6.2831855f * (f32)pl->rot[1]) / 65536.0f);
    v[0] *= 130.0f;
    *y *= 130.0f;
    *z *= 130.0f;
    npc->posX = pl->pos[0] + v[0];
    npc->posY = pl->pos[1] + *y;
    npc->posZ = pl->pos[2] + *z;
    npc->rotY = pl->rot[1];
    return 1;
}

s32 LobbyBgm_IsActive(void);
void Tutorial_RunNpc(Em *npc);
s32 Npc51_RunState(Em *a);
s32 Npc4F_RunState(Em *a);
s32 Npc_RunState(Em *a);
void Actor_StepMoveNpc(Em *obj);
void Npc_StepTurn(Em *arg0);
extern s32 gTownMode;
s32 Npc2_NearNpc1(Em *npc);
void Actor_SetState(void *obj, s32 group, s32 state);
s32 Npc_NextRoutePoint(Em *a);
void Npc_CheckTimeRoute(Em *npc, NpcWork *t);
void Npc_CheckSideRoute(Em *npc, NpcWork *t);

/* The original's own bug: when Npc_IsType4E and LobbyBgm_IsActive send it straight to end,
 * the game reads flag and ret unset. flag is $s0 and ret is $s1, which nothing writes before
 * end on that path, so the game tests and returns the caller's own $s0 and $s1 (this function
 * saves them on entry). A native port reads those two as the caller's s0/s1 values. */
s32 Npc_UpdateState(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;
    s32 ret;
    s32 flag;
    s32 *argp;
    s32 n;

    if (Npc_IsType4E(a) == 1 && LobbyBgm_IsActive() != 0) {
        goto end;
    }
    m->unk2 = 0;
    Tutorial_RunNpc(a);
    switch (a->variant) {
    case 0x51:
    case 0x52:
    case 0x53:
    case 0x54:
    case 0x55:
        ret = Npc51_RunState(a);
        break;
    case 0x4F:
    case 0x50:
        ret = Npc4F_RunState(a);
        break;
    default:
        ret = Npc_RunState(a);
        break;
    }
    Actor_StepMoveNpc(a);
    Npc_StepTurn(a);
    flag = 1;
    if (m->route == NULL) {
        goto end;
    }
    switch (a->state) {
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
        if (m->holdRoute == 0) {
            if (Npc_IsType31to36(a) == 1 && gTownMode == 0x32) {
                flag = 1;
                break;
            }
            if (m->id == 5 && gTownMode == 0x30) {
                flag = 1;
                break;
            }
            if (m->id == 0x14 && gTownMode == 0x11) {
                flag = 1;
                break;
            }
            if (m->id == 0x22 && (gTownMode == 0x12 || a->state == 0xAC)) {
                flag = 1;
                break;
            }
            if (m->id == 0x6D && gTownMode == 0x13) {
                flag = 1;
                break;
            }
            argp = &m->route[m->routeIdx].arg;
            n = *argp;
            if (n == -1) {
                if (Npc2_NearNpc1(a) == 1) {
                    Actor_SetState(a, 0, 0x7A);
                }
                flag = 0;
                break;
            }
            if (n == -2) {
                if (++m->wait > 2 && a->motion[0].playing <= 0) {
                    flag = (s8)Npc_NextRoutePoint(a);
                }
                break;
            }
            if (++m->wait >= n) {
                flag = (s8)Npc_NextRoutePoint(a);
            }
        } else {
            /* a held route: flag = 1, written out as in every other branch (already 1:
             * MWCC drops the store late, which leaves the original's b to end above) */
            flag = 1;
        }
        break;
    }
end:
    if (flag == 0) {
        Npc_CheckTimeRoute(a, m);
    }
    Npc_CheckSideRoute(a, m);
    if (m->id == 0x72) {
        m->unk2 = 1;
    }
    return ret;
}

void Npc_HoldRoute(Em *npc) {
    ((NpcWork *)&npc->work)->holdRoute = 1;
}

void Npc_EnterRoutePoint(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    w->holdRoute = 0;
    Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
}

void Npc_HideAndAdvance(Em *npc) {
    npc->taskPhase = (u8) (npc->taskPhase + 1);
    npc->visible = 0;
}
