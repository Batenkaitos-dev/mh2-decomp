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

/* The NPCs' state machines, 0x780580-0x78E4D0: one of the game's units.
 * Npc_StartTurnBack is static (Npc_StateA1 matches only so) and is called
 * from 0x780580 to 0x7873F0's Npc_State7E, so round 19 merged round 18's
 * unk_780580.c, unk_7855B0.c, unk_7855D0.c, unk_7873A0.c and unk_7873F0.c
 * here; the "----" lines mark where each began. */

/* Prototypes shared by the parts of this unit. */
void Actor_SetAction(void *a, u16 mode, s32 b, s32 c);
void Actor_SetState(void *obj, s32 group, u16 state); /* real: s32 state; these callers pass a u16 */ /* unit's own declaration: state u16 (the definition has s32); with the definition's type Npc_NextRoutePoint by 34 of 63, Npc51_State9B by 1 of 126, Npc51_State2C by 1 of 241 words differ */
void Actor_StartMoveTo(Em *obj, s32 n);
s32 Cond_Check(u16 id);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
void Npc_EnterRoutePoint(Em *npc);
Em *Npc_Find(u8 kind);
s32 Npc_NextRoutePoint(Em *a);
void Npc_State01(Em *a);
void Npc_State03(Em *a);
void Npc_State04(Em *a);
void Npc_State14(Em *a, u8 flag);
void Npc_State2C(Em *npc);
void Npc_State30(Em *a);
void Npc_State31(Em *a);
void Npc_State32(Em *a);
void Npc_State34(Em *a);
void Npc_State3C(Em *a);
void Npc_State3D(Em *a);
void Npc_State3F(Em *a);
void Npc_State41(Em *a);
void Npc_State43(Em *a, s8 arg1);
void Npc_State5A(Em *arg0);
void Npc_State66(Em *a);
void Npc_State67(Em *a);
void Npc_State68(Em *a, u8 flag);
void Npc_State6A(Em *a);
void Npc_State6B(Em *a);
void Npc_State76(Em *a);
void Npc_State77Pair(Em *a);
void Npc_State77Solo(Em *a);
void Npc_State9D(Em *a);
void Npc_StateA1(Em *a);
void Npc_StateA6(Em *a, s32 first);
u16 Rand_Next(s32 stream);
u8 Village_FacilityLevel(s32 k);
int abs(int);

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Npc_TurnToPlayer(Em *, s32);
extern s32 gTownMode;
s32 Npc2_NearNpc1(Em *);
void Npc_PickIdleState(Em *a);

s32 Game_IsOnline(void);
s32 Npc_IsBusyNotAct3C4B(Em *arg0);
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
s32 Actor_HeadingDiff(void *obj, void *pos);
s32 abs(s32);
s32 Npc_IsFree(Em *arg0);
f32 Vec_Distance(f32 *a, f32 *b);

/* A route entry (0x14 bytes, as in the 0x7873F0 part below): the point, the state to
 * enter there (Actor_SetState) and an argument. */

/* Advance to the next point of the route (back to the first after the end,
 * returning 0) and enter its state. */
s32 Npc_NextRoutePoint(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;
    NpcRouteEnt *p;
    s32 ret;

    m->routeIdx++;
    p = &m->route[m->routeIdx];
    ret = 1;
    if (p->state == 0) {
        m->routeIdx = 0;
        ret = 0;
        p = &m->route[m->routeIdx];
    }
    if (p->state != a->state) {
        Actor_SetState(a, 0, p->state);
        if (p->arg == -1 || p->state == 1) {
            *(NpcPos *)a->goal = p->pos;
            Actor_StartMoveTo(a, 0x14);
        }
    }
    m->wait = 0;
    return ret;
}

void Npc_StepTurn(Em *npc)
{
    u16 count;
    NpcWork *m;

    m = (NpcWork *)&npc->work;
    count = m->turnCount;
    if (count != 0) {
        m->turnCount = (s16) (count - 1);
        npc->rotY = (s32) (npc->rotY + m->turnStep);
    }
}

void Npc_TurnToPlayer(Em *pl, s32 frames)
{
    NpcWork *m = (NpcWork *)&pl->work;

    m->turnDelta = Actor_HeadingDiff(pl, gPlayers[gScene.localPlayer].pos);
    if (abs(m->turnDelta) > 10) {
        m->homeYaw = pl->rotY;
    }
    m->turnStep = m->turnDelta / (s16)frames;
    m->turnCount = frames;
}

void Npc_StartTurnTo(Em *npc, s32 frames, s32 angle)
{
    NpcWork *m = (NpcWork *)&npc->work;

    m->turnDelta = (s16)angle - (s16)npc->rotY;
    m->turnStep = m->turnDelta / (s16)frames;
    m->turnCount = frames;
}

static void Npc_StartTurnBack(Em *npc, s32 frames)
{
    NpcWork *m = (NpcWork *)&npc->work;

    m->turnDelta = (u16)m->homeYaw - npc->rotY;
    m->turnStep = m->turnDelta / (s16)frames;
    m->turnCount = frames;
}

s32 Npc2_NearNpc1(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;
    Em *em;

    if (gScene.area != 0x89) {
        return 0;
    }
    switch (m->id) {
    case 2:
        break;
    default:
        return 0;
    }
    em = Npc_Find(1);
    if (em == NULL) {
        return 0;
    }
    if (200.0f > Vec_Distance(&npc->posX, (f32 *) &em->posX)) {
        if ((em->state == 0x44 || em->state == 0x43) && em->stateGroup == 0) {
            return 1;
        }
        return 2;
    }
    return 0;
}

typedef struct V007808D0_0 {
    f32 x1, z1, x2, z2;
} V007808D0_0;
extern V007808D0_0 gNpcAboveLines[];

/* static: Npc_RouteKindBySide keeps a3 across the call. */
static s32 Npc_PlayerAboveLine(u16 i)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    V007808D0_0 *l;
    f32 a;

    if (i > 3) {
        i = 0;
    }
    l = &gNpcAboveLines[i];
    a = (l->z2 - l->z1) / (l->x2 - l->x1);
    if (pl->pos[2] - a * pl->pos[0] >= l->z1 - l->x1 * a) {
        return 1;
    }
    return 0;
}

/* Pairs of route kinds a and b split by a line (Npc_PlayerAboveLine), the
 * list ending with line 0xFFFF. */
typedef struct V00780970_0 {
    u8 a;
    u8 b;
    u16 line;
} V00780970_0;
extern V00780970_0 gNpcSideRouteLines[];

/* The route kind to use instead of kind: a pair's other kind, by the side
 * of its line the player is on (kinds 8 and 0xE need facility 2). */
u8 Npc_RouteKindBySide(u8 kind)
{
    s32 k = kind;
    V00780970_0 *e;
    u16 line;

    switch (k) {
    case 0xE:
    case 8:
        if ((u8)Village_FacilityLevel(2) == 0) {
            return kind;
        }
    }
    e = gNpcSideRouteLines;
    for (line = gNpcSideRouteLines[0].line; line != 0xFFFF; line = e->line) {
        if (k == e->a || k == e->b) {
            break;
        }
        e++;
    }
    if (line == 0xFFFF) {
        return kind;
    }
    if (Npc_PlayerAboveLine(line) == 1) {
        if (k == e->b) {
            return e->a;
        }
    } else if (k == e->a) {
        return e->b;
    }
    return kind;
}

s32 Area_IsHouse(u16 area);

void Npc_CheckSideRoute(Em *npc, NpcWork *t)
{
    u8 k;

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
    if (t->unk49 == 0 && npc != gPlayers[gScene.localPlayer].target) {
        switch (t->unk48) {
        case 4:
            k = Npc_RouteKindBySide(t->id);
            if (t->id != k) {
                npc->state = 0x60;
                t->unk49 = 1;
                t->unk5C = k;
            }
            return;
        }
    }
}

s32 LobbyTime_IsAm(void);
void Npc_SetRouteByKind(Em *npc);

void Npc_CheckTimeRoute(Em *npc, NpcWork *t)
{
    if (Game_IsOnline() == 0 && t->id != 0x67 && Area_IsHouse(gScene.area) == 1) {
        return;
    }
    switch (npc->state) {
    case 0x2C:
    case 0x42:
    case 0x5F:
    case 0x60:
        return;
    }
    if (t->unk49 != 0) {
        return;
    }
    if (gTownMode == 4 && npc == gPlayers[gScene.localPlayer].target) {
        return;
    }
    Npc_SetRouteByKind(npc);
    switch (t->unk48) {
    case 0:
        return;
    case 1:
        if (LobbyTime_IsAm() == 0) {
            return;
        }
        return;
    case 2:
        if (LobbyTime_IsAm() == 1) {
            if (t->id == 9 && (u8)Village_FacilityLevel(3) == 0) {
                return;
            }
            switch (t->id) {
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

void Npc_Idle0F(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;
    Em *em = Npc_Find(0xE);
    u8 step;

    if (em == NULL && a->step != 0) {
        return;
    }
    step = a->step;
    switch (step) {
    case 0:
        a->step = step + 1;
        m->motionPhase = 0;
        if (a->motionId[0] == 0x406) {
            Actor_SetMotionOnLayer(a, 0x406, 0x14, 0, 0);
        }
        break;
    case 1:
        if (em->motionId[0] == 0x40B && Motion_ReachedFrame(em, 0, 44.0f) != 0) {
            Actor_SetMotionOnLayer(a, 0x40B, 6, 0, 0);
            a->step++;
        }
        break;
    case 2:
        if (a->motion[0].playing <= 0) {
            a->step = step + 1;
            a->timer = 2;
            Actor_SetMotionOnLayer(a, 0x40C, 4, 0, 0);
        }
        break;
    case 3:
        if (m->motionPhase == 0 && Motion_ReachedFrame(a, 0, 30.0f) != 0) {
            m->motionPhase = 1;
        }
        if (a->motion[0].playing <= 0 && --a->timer < 2) {
            a->step++;
        }
        break;
    case 4:
        if (Motion_ReachedFrame(a, 0, 136.0f) != 0) {
            Actor_SetMotionOnLayer(a, 0x41A, 0, 0, 0);
            a->step++;
        }
        break;
    case 5:
        if (a->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(a, 0x40D, 0, 0x14, 0);
            a->step++;
        }
        break;
    case 6:
        if (m->motionPhase == 1 && Motion_ReachedFrame(a, 0, 118.0f) != 0) {
            m->motionPhase = 0;
        }
        if (a->motion[0].playing <= 0) {
            a->step = 1;
            m->motionPhase = 0;
            Actor_SetMotionOnLayer(a, 0x406, 0x10, 0, 0);
        }
        break;
    }
}

s32 Npc6_IsIdle(void)
{
    Em *em = Npc_Find(6);

    if (em == NULL) {
        return 0;
    }
    if ((u8)Village_FacilityLevel(3) == 2 || em->state == 4 || em->motionId[0] == 0x292 || em->motionId[0] == 1) {
        return 0;
    }
    return 1;
}

void Npc_Idle05(Em *a)
{
    u8 step = a->step;

    switch (step) {
    case 0:
        a->step = step + 1;
        if (Npc6_IsIdle() == 1 && ((u16)Rand_Next(1) & 1) && a->motionId[0] != 0x372) {
            Actor_SetMotion(a, 0x372, 0, 0);
            break;
        }
        if (a->motionId[0] == 0x371) {
            Actor_SetMotion(a, 0x370, 0, 0);
            break;
        }
        Actor_SetMotion(a, 0x371, 0, 0);
        break;
    case 1:
        if (a->motion[0].playing <= 0 && (u16)Rand_Next(1) % 100 <= 29) {
            a->step = 0;
        }
        break;
    }
}

/* The pair 0x69/0x6B: each stays near the other (within 400) and plays
 * 0x3EF/0x3F0 in turn; farther away it plays 0x3E9. */
void Npc_Idle69(Em *a)
{
    u8 step = a->step;
    NpcWork *m = (NpcWork *)&a->work;
    Em *o;

    switch (step) {
    case 0:
        a->step = step + 1;
        break;
    case 1:
        switch (m->id) {
        case 0x69:
            o = Npc_Find(0x6B);
            goto near;
        case 0x6B:
            o = Npc_Find(0x69);
        near:
            if (o != NULL) {
                if (Vec_Distance(&a->posX, (f32 *) &o->posX) <= 400.0f) {
                    if (o->state == 0x42 || o->state == 0x2C) {
                        Npc_PickIdleState(a);
                        return;
                    }
                    switch (a->motionId[0]) {
                    case 0x3EF:
                        if (a->motion[0].playing == 0) {
                            a->timer = (u16)Rand_Next(1) % 60 + 90;
                            Actor_SetMotionOnLayer(a, 0x3F0, 4, 0, 0);
                        }
                        break;
                    case 0x3F0:
                        if (--a->timer <= 0) {
                            Actor_SetMotionOnLayer(a, 0x3EF, 4, 0, 0);
                        }
                        break;
                    default:
                        a->timer = (u16)Rand_Next(1) % 60 + 90;
                        Actor_SetMotionOnLayer(a, 0x3F0, 4, 0, 0);
                        break;
                    }
                } else if (a->motionId[0] != 0x3E9) {
                    Actor_SetMotionOnLayer(a, 0x3E9, 4, 0, 0);
                }
            } else if (a->motionId[0] != 0x3E9) {
                Actor_SetMotionOnLayer(a, 0x3E9, 4, 0, 0);
            }
            break;
        }
        break;
    }
}

void Npc_Idle4D(Em *a)
{
    ((NpcWork *)&a->work)->unk45 = 1;
    switch (a->step) {
    case 0:
        if (a->motionId[0] != 0x3E9) {
            Actor_SetMotionOnLayer(a, 0x3E9, 4, 0, 0);
            a->timer = (u16)Rand_Next(1) % 2 + 4;
        }
        a->step++;
        break;
    case 1:
        if (a->motion[0].playing <= 0) {
            if (--a->timer <= 0) {
                Actor_SetMotionOnLayer(a, 0x3EA, 4, 0, 0);
                a->step++;
            }
        }
        break;
    case 2:
        if (a->motion[0].playing <= 0) {
            a->step = 0;
        }
        break;
    }
}

void Npc_Idle3C(Em *obj)
{
    u8 step = obj->step;

    switch (step) {
    case 0:
        obj->step = step + 1;
        Actor_SetMotion(obj, 0x2B2, 4, 0);
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            if ((u16)Rand_Next(1) & 1) {
                Actor_SetMotion(obj, 0x2B2, 4, 0);
            } else {
                Actor_SetMotion(obj, 0x2B6, 8, 0);
            }
        }
        break;
    }
}

void Npc_Idle4B(Em *obj)
{
    switch (obj->step) {
    case 0:
        if (obj->motionId[0] == 0x292) {
            Actor_SetMotion(obj, 0xFD, 0x14, 0);
        } else if ((u16)Rand_Next(1) & 1) {
            if (obj->motionId[0] != 0xFD) {
                Actor_SetMotion(obj, 0xFD, 4, 0);
            }
        } else if (obj->motionId[0] != 0xFE) {
            Actor_SetMotion(obj, 0xFE, 4, 0);
        }
        obj->step++;
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step = 0;
        }
        break;
    }
}

void Npc_Idle4E(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        
    case 1:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3EA, 0, 0);
            obj->step = (u8) (obj->step + 1);
            break;
        }
    default:
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3EB, 0, 0);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 3:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3EC, 0, 0);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 4:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3ED, 0, 0);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 5:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3EE, 0, 0);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 6:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3EF, 0, 0);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 7:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3F1, 0, 0);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 8:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3F2, 0, 0);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 9:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x3F4, 0, 0);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 10:
        if (obj->motion[0].playing <= 0) {
            obj->step = 0U;
        }
        break;
    }
}

s32 Npc_IsType4B(Em *arg0);
s32 Npc_IsType4D(Em *arg0);
s32 Npc_IsType3Cto3E(Em *arg0);
s32 Npc_IsType4E(Em *arg0);
s32 Npc_IsType31to36(Em *arg0);
s32 Npc_IsType40(Em *arg0);
s32 Npc_IsFreeAct29or2A(Em *arg0);
s32 Npc_IsFreeId0Aor2E(Em *arg0);
s32 Npc_IsFreeAct15to19(Em *arg0);
s32 Npc_IsFreeAct18Set(Em *arg0);
void Npc_Idle05(Em *a);
void Npc_Idle0F(Em *a);
void Npc_Idle4B(Em *arg0);
void Npc_Idle4D(Em *a);
void Npc_Idle3C(Em *arg0);
void Npc_Idle4E(Em *arg0);
void Npc_Idle69(Em *a);

void Npc_State01(Em *a)
{
    NpcWork *m;
    s32 dir;
    Player *pl;

    pl = &gPlayers[gScene.localPlayer];
    m = (NpcWork *)&a->work;
    dir = (m->moveCount != 0) ? -1 : 1;

    if (m->id == 5) {
        Npc_Idle05(a);
    } else if (m->id == 0xF) {
        Npc_Idle0F(a);
    } else if (Npc_IsType4B(a) == 1) {
        Npc_Idle4B(a);
    } else if (Npc_IsType4D(a) == 1) {
        Npc_Idle4D(a);
    } else if (Npc_IsType3Cto3E(a) == 1) {
        Npc_Idle3C(a);
    } else if (Npc_IsType4E(a) == 1) {
        Npc_Idle4E(a);
    } else if (m->id == 0x69) {
        Npc_Idle69(a);
    } else if (m->id == 0x6B) {
        Npc_Idle69(a);
    } else {
        if (m->id == 0x58) {
            m->motionPhase = 1;
        }
        switch (a->step) {
        case 0:
            a->step++;
            Actor_SetAction(a, 0, 0, 0);
            if (Npc_IsType31to36(a) == 1) {
                Actor_SetMotion(a, 0x35A, 6, 0);
            } else if (Npc_IsType40(a) == 1) {
                Actor_SetMotion(a, 0x364, 4, 0);
                if (m->id == 0x2F || m->id == 0x20) {
                    m->unk45 = 1;
                }
            } else if (Npc_IsFreeAct29or2A(a) == 1) {
                if (Npc_IsFreeId0Aor2E(a) == 1) {
                    Actor_SetMotion(a, 0x2AB, dir * 14, 0);
                } else {
                    Actor_SetMotion(a, 0x2A8, dir * 4, 0);
                }
            } else if (Npc_IsFreeAct15to19(a) == 1) {
                Actor_SetMotion(a, 0x2A3, dir * 16, 0);
            } else if (Npc_IsFreeAct18Set(a) == 1) {
                Actor_SetMotion(a, 0x3F, dir * 16, 0);
            } else if (Npc_IsBusyNotAct3C4B(a)) {
                Actor_SetMotionOnLayer(a, 0x3E9, dir * 16, 0, 0);
            } else {
                Actor_SetMotion(a, 1, dir * 16, 0);
            }
            break;
        case 1:
        case 2:
            if (Npc_IsFreeAct18Set(a) == 1 && Npc_IsFreeAct15to19(a) == 0 && Npc_IsType31to36(a) == 0) {
                if (a->motionId[0] == 1 && a->motion[0].playing <= 0) {
                    Actor_SetMotion(a, 0x3F, dir * 2, 0);
                }
            } else if (Npc_IsFreeId0Aor2E(a) == 1) {
                if (Vec_Distance(&a->posX, (f32 *) pl->pos) < 400.0f) {
                    Actor_SetState(a, 0, 0x3C);
                }
            } else {
                Npc_IsType40(a);
            }
            break;
        }
    }
}

s32 Npc_IsFreeAct2Bto30(Em *arg0);
s32 Npc_IsBusyId0E(Em *arg0);
s32 Npc_IsFreeAct29or2A(Em *arg0);
s32 Npc_IsType40(Em *arg0);
s32 Npc_IsType3Cto3E(Em *arg0);

void Npc_PickIdleState(Em *a)
{
    if (Npc_IsBusyId0E(a) == 1) {
        Actor_SetState(a, 0, 0x68);
        return;
    }
    if (Npc_IsFreeAct29or2A(a) == 1) {
        Actor_SetState(a, 0, 0x3E);
        return;
    }
    if (Npc_IsType40(a) == 1 && a->state == 4) {
        Actor_SetState(a, 0, 0x8E);
        return;
    }
    if (Npc_IsFreeAct2Bto30(a) == 1) {
        Actor_SetState(a, 0, 0x2F);
        return;
    }
    if (Npc_IsType3Cto3E(a) == 1) {
        Actor_SetState(a, 0, 0x40);
        return;
    }
    switch (a->state) {
    case 0x8F:
        if (a->motionId[0] != 0x365) {
            Actor_SetState(a, 0, 0x2C);
        }
        break;
    case 0x92:
        Actor_SetState(a, 0, 0x93);
        break;
    case 0x30:
    case 0x31:
    case 0xA2:
        Actor_SetState(a, 0, 0x31);
        break;
    case 0xE:
    case 0x32:
        Actor_SetState(a, 0, 0x32);
        break;
    case 0x33:
    case 0x34:
        Actor_SetState(a, 0, 0x34);
        break;
    default:
        Actor_SetState(a, 0, 0x2C);
        break;
    }
}

s32 Npc_IsType49(Em *arg0);

void Npc_State03(Em *a)
{
    Em *o;
    NpcWork *m = (NpcWork *)&a->work;
    NpcPos p;
    f32 d;

    switch (a->step) {
    case 0:
        m->unk45 = 0;
        Actor_SetAction(a, 0, 0, 0);
        a->step++;
        if (Npc_IsType49(a)) {
            if (a->motionId[0] != 0x41F) {
                Actor_SetMotionOnLayer(a, 0x41F, 4, 0, 0);
            }
        } else if (a->variant == 0x3B) {
            if ((u8)Village_FacilityLevel(3) == 2) {
                if (a->motionId[0] != 2) {
                    Actor_SetMotion(a, 2, 4, 0);
                }
            } else if (a->motionId[0] == 0x383) {
                Actor_SetMotion(a, 0x382, 0x14, 0);
            } else if (a->motionId[0] != 0x382) {
                Actor_SetMotion(a, 0x382, 4, 0);
            }
        } else if (Npc_IsType31to36(a)) {
            if (a->motionId[0] == 0x35B) {
                Actor_SetMotion(a, 0x361, 6, 0);
            } else if (a->motionId[0] != 0x361) {
                Actor_SetMotion(a, 0x361, 4, 0);
            }
        } else if (Npc_IsType40(a) == 1) {
            if (a->motionId[0] != 0x368) {
                Actor_SetMotion(a, 0x368, 0, 0);
            }
        } else if (Npc_IsFreeAct15to19(a) == 1) {
            if (a->motionId[0] != 0x2A4) {
                Actor_SetMotion(a, 0x2A4, 4, 0);
            }
        } else if (Npc_IsFreeAct18Set(a) == 1) {
            if (a->motionId[0] != 0x40) {
                Actor_SetMotion(a, 0x40, 4, 0);
            }
        } else if (Npc_IsBusyNotAct3C4B(a) == 1) {
            if (a->motionId[0] != 0x3F5) {
                Actor_SetMotionOnLayer(a, 0x3F5, 4, 0, 0);
            }
        } else if (a->motionId[0] != 2) {
            Actor_SetMotion(a, 2, 4, 0);
        }
        break;
    case 1:
        if (m->id == 6) {
            o = Npc_Find(5);
            if (o != NULL) {
                d = Vec_Distance(&a->posX, &o->posX);
                if (o->motionId[0] == 0x372 && d < 800.0f && Motion_ReachedFrame(o, 0, 26.0f) &&
                    !Motion_ReachedFrame(o, 0, 60.0f)) {
                    Actor_SetState(a, 0, 0xA3);
                }
            }
        }
        p = m->route[m->routeIdx].pos;
        p.v[1] = a->posY;
        a->targetYaw = (s16)(u16)Actor_HeadingDiff(a, &p) / 10;
        a->rotY += (s16)a->targetYaw;
        d = Vec_Distance(&a->posX, (f32 *) &p);
        if (d < 0.0f) {
            d *= -1.0f;
        }
        if (d <= 30.0f) {
            Npc_NextRoutePoint(a);
        }
        break;
    }
}

void Npc_State04(Em *a)
{
    u8 step = a->step;
    NpcWork *m = (NpcWork *)&a->work;
    NpcPos p;
    f32 d;

    switch (step) {
    case 0:
        Actor_SetAction(a, 0, 0, 0);
        a->step++;
        if (Npc_IsBusyNotAct3C4B(a) == 1) {
            if (a->motionId[0] != 0x3FD) {
                Actor_SetMotionOnLayer(a, 0x3FD, 4, 0, 0);
            }
        } else if (Npc_IsType40(a) == 1) {
            if (a->motionId[0] != 0x369) {
                Actor_SetMotion(a, 0x369, 0x14, 0);
            }
        } else if (a->motionId[0] != 3) {
            Actor_SetMotion(a, 3, 0x14, 0);
        }
        if (m->id == 0x58) {
            m->motionPhase = 1;
        }
        break;
    case 1:
        p = m->route[m->routeIdx].pos;
        p.v[1] = a->posY;
        a->targetYaw = (s16)(u16)Actor_HeadingDiff(a, &p) / 10;
        a->rotY += (s16)a->targetYaw;
        d = Vec_Distance(&a->posX, (f32 *) &p);
        if (d < 0.0f) {
            d *= -1.0f;
        }
        if (d <= 30.0f) {
            Npc_NextRoutePoint(a);
        }
        break;
    }
}

void Npc_State35(Em *obj, s8 variant)
{
    NpcWork *m = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        m->unk45 = 1;
        obj->step++;
        Actor_SetAction(obj, 1, 0, 0);
        if (variant == 1) {
            if (obj->motionId[0] != 0x2A0) {
                Actor_SetMotion(obj, 0x2A0, 0x3C, 0);
            }
        } else {
            if (obj->motionId[0] != 0x29E) {
                Actor_SetMotion(obj, 0x29E, 0x3C, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

s32 Npc_IsFreeAct15to19(Em *arg0);
extern u8 gNpcPartnerPairs[];

void Npc_State43(Em *a, s8 variant)
{
    u8 *p = gNpcPartnerPairs;
    NpcWork *m = (NpcWork *)&a->work;
    Em *o;

    switch (a->step) {
    case 0:
        a->step++;
        Actor_SetAction(a, 0, 0, 0);
        if (Npc_IsFreeAct2Bto30(a) == 1) {
            if (a->motionId[0] != 0x28B) {
                Actor_SetMotion(a, 0x28B, 8, 0);
            }
        } else if (Npc_IsFreeAct15to19(a) == 1) {
            Actor_SetMotion(a, 0x2B8, 0xA, 0);
        } else {
            switch (variant) {
            case 0:
                Actor_SetMotion(a, 0x291, 0xA, 0);
                break;
            case 1:
                Actor_SetMotion(a, 0x290, 0xA, 0);
                break;
            case 2:
                Actor_SetMotion(a, 0x292, 0xC, 0);
                break;
            case 3:
                Actor_SetMotion(a, 0x35B, 8, 0);
                break;
            }
        }
        break;
    case 1:
        switch (m->id) {
        case 0x60:
            o = Npc_Find(0x61);
            if (200.0f <= Vec_Distance(&a->posX, &o->posX)) {
                if (a->motionId[0] != 1) {
                    Actor_SetMotion(a, 1, 4, 0);
                }
            } else if (a->motionId[0] != 0x290) {
                Actor_SetMotion(a, 0x290, 4, 0);
            }
            break;
        case 0x61:
            o = Npc_Find(0x60);
            if (200.0f <= Vec_Distance(&a->posX, &o->posX)) {
                if (a->motionId[0] != 0x3F) {
                    Actor_SetMotion(a, 0x3F, 4, 0);
                }
            } else if (a->motionId[0] != 0x291) {
                Actor_SetMotion(a, 0x291, 4, 0);
            }
            break;
        }
        if (gScene.area == 0x89 || gScene.area == 0x57) {
            if (Npc_IsFreeAct2Bto30(a) == 1 && a->motion[0].playing == 0 && a->motionId[0] == 0x28B) {
                Actor_SetMotion(a, 0x28C, 2, 0);
            }
            for (; p[0] != 0xFF; p += 2) {
                if (p[0] == m->id) {
                    o = Npc_Find(p[1]);
                } else if (p[1] == m->id) {
                    o = Npc_Find(p[0]);
                }
                if (o != NULL && (o->state == 0x42 || o->state == 0x2C)) {
                    if (!(200.0f <= Vec_Distance(&a->posX, &o->posX))) {
                        Npc_PickIdleState(a);
                    }
                }
            }
        }
        break;
    }
}

typedef struct V007828E0_0 {
    f32 x, y, z;
} V007828E0_0;

void Npc_State5A(Em *obj)
{
    NpcWork *m = (NpcWork *)&obj->work;

    if (m->route == NULL) {
        Actor_SetState(obj, 0, 1);
    }
    *(V007828E0_0 *)(&obj->posX) = *(V007828E0_0 *)&m->route[m->routeIdx].pos;
    Actor_SetState(obj, 0, 1);
}

void Npc_State37(Em *obj, s8 variant)
{
    NpcWork *m = (NpcWork *)&obj->work;
    u8 step = obj->step;

    switch (step) {
    case 0:
        obj->step = step + 1;
        m->unk45 = 1;
        Actor_SetAction(obj, 1, 0, 0);
        if (variant == 1) {
            if (obj->motionId[0] != 0x29C) {
                Actor_SetMotion(obj, 0x29C, 0x3C, 0);
            }
        } else if (variant == 0) {
            if (obj->motionId[0] != 0x29A) {
                Actor_SetMotion(obj, 0x29A, 0x3C, 0);
            }
        } else {
            if (obj->motionId[0] != 0x268) {
                Actor_SetMotion(obj, 0x268, 0, 0x7C);
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
 * The game's empty delay slots come from the default-first switch on Npc_IsBusyNotAct3C4B's result. */
void Npc_State2C(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;
    NpcRouteEnt *tbl;

    switch (npc->motionId[0]) {
    case 0x2A5:
    case 0x2A6:
    case 0x285:
    case 0x268:
        Actor_SetState(npc, 0, *(u16 *)&m->route[m->routeIdx].state);
        break;
    default:
        switch (npc->step) {
        case 0:
            m->unk2 = 1;
            if ((s8)m->unk45 != 0 || npc->stance == 1) {
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
            switch (Npc_IsBusyNotAct3C4B(npc)) {
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
            if (m->id == 0x19) {
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
            if (m->id == 0x5) {
                Actor_SetMotion(npc, 0x379, 0, 0);
                return;
            }
            if (m->id == 0x10) {
                Actor_SetMotion(npc, 0x35B, 0, 0);
                return;
            }
            if (m->id == 0x4E && ElderRaid_InState1() == 1) {
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
                    if (m->id == 0x4B) {
                        return;
                    }
                    Actor_SetMotionOnLayer(npc, 0x3EF, 0x10, 0, 0);
                    return;
                }
            break;
        case 1:
            m->unk2 = 1;
            npc->rotY += (s16)npc->targetYaw / 10;
            if (--npc->timer <= 0) {
                npc->step++;
            }
            break;
        case 2:
            m->unk2 = 1;
            if (Npc_IsType49(npc) != 0 && npc->motion[0].playing <= 0) {
                Actor_SetMotionOnLayer(npc, 0x429, 8, 0, 0);
            } else if (Npc_IsBusyNotAct3C4B(npc) != 0 && npc->motion[0].playing <= 0) {
                Actor_SetMotionOnLayer(npc, 0x3F0, 8, 0, 0);
            } else if (npc->motionId[0] == 0x261 && Motion_ReachedFrame(npc, 0, 5.0f) != 0) {
                Actor_SetMotion(npc, 0x289, 0, 0);
            }
            if (gTownMode == 0) {
                npc->timer = 0x10;
                npc->step++;
                if (m->id == 0x19 || m->id == 0x29) {
                    npc->timer = 10;
                }
            }
            break;
        case 3:
            m->unk2 = 1;
            switch (m->id) {
            case 0x29:
            case 0x19:
            case 0x1E:
            case 0x2A:
            case 0x30:
            case 0x33:
                m->unk2 = 1;
                break;
            }
            if (npc->motionId[0] == 0x286) {
                if (Motion_ReachedFrame(npc, 0, 260.0f) != 0) {
                    Actor_SetMotion(npc, 0x284, 0x12, 0);
                }
                break;
            }
            if (--npc->timer <= 0) {
                if ((s8)m->unk45 != 0) {
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
                    if (m->id != 1 && Npc_IsType4B(npc) != 1) {
                        if (m->id == 0xF) {
                            npc->timer = 10;
                            Actor_SetMotionOnLayer(npc, 0x406, 0x14, 0, 0);
                        } else if (Npc_IsType49(npc) != 0 || Npc_IsBusyNotAct3C4B(npc) != 0) {
                            if (m->id != 0x4E) {
                                Actor_SetMotionOnLayer(npc, 0x3E9, 0xA, 0, 0);
                            }
                        } else {
                            switch (npc->motionId[0]) {
                            case 0x295:
                            case 0x424:
                                m->unk2 = 1;
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
                                if (npc->state != 0x41 && npc->state != 0x25 && npc->state != 0x14) {
                                    Actor_SetMotion(npc, 1, 0xA, 0);
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
            m->unk2 = 1;
            if (npc->motion[0].playing != 0) {
                switch (npc->motionId[0]) {
                case 0x35C:
                case 0x3EC:
                    return;
                }
            }
            switch (m->id) {
            case 0x29:
            case 0x19:
            case 0x1E:
            case 0x2A:
            case 0x30:
            case 0x33:
            case 0x60:
            case 0x69:
            case 0x61:
            case 5:
                m->unk2 = 1;
                break;
            }
            npc->rotY -= (s16)npc->targetYaw / 10;
            if (--npc->timer <= 0) {
                tbl = m->route;
                if (tbl != NULL) {
                    Actor_SetState(npc, 0, *(u16 *)&tbl[m->routeIdx].state);
                    return;
                }
                Actor_SetState(npc, 0, 1);
            }
            break;
        }
        break;
    }
}

void Npc_State46(Em *obj, s8 variant)
{
    u8 step = obj->step;

    switch (step) {
    case 0:
        obj->step = step + 1;
        switch (variant) {
        case 0:
            if (obj->motionId[0] != 0x293) {
                Actor_SetMotion(obj, 0x293, 0x14, 0);
            }
            break;
        case 1:
            if (obj->motionId[0] != 0x294) {
                Actor_SetMotion(obj, 0x294, 0x14, 0);
            }
            break;
        }
        break;
    }
}

void Npc_State3A(Em *obj, s8 variant)
{
    NpcWork *m = (NpcWork *)&obj->work;
    u8 step;

    m->unk45 = 1;
    step = obj->step;
    switch (step) {
    case 0:
        obj->step = step + 1;
        switch (variant) {
        case 0:
            if (obj->motionId[0] != 0x296) {
                obj->timer = 0x14;
                m->unk2 = 1;
                Actor_SetMotion(obj, 0x296, 0x14, 0);
            }
            break;
        case 1:
            if (obj->motionId[0] != 0x298) {
                Actor_SetMotion(obj, 0x298, 0xA, 0);
            }
            break;
        }
        break;
    case 1:
        if (obj->timer != 0) {
            m->unk2 = 1;
            if (--obj->timer <= 0) {
                m->unk2 = 0;
            }
        }
        break;
    }
}

s32 Area_Is86to88(s32 arg0);

void Npc_State41(Em *a)
{
    u8 step = a->step;
    NpcWork *m = (NpcWork *)&a->work;

    switch (step) {
    case 0:
        a->step = step + 1;
        a->timer = (u8)((u16)Rand_Next(1) + 0xB4);
        m->unk45 = 1;
        Actor_SetAction(a, 1, 0, 0);
        if (a->motionId[0] == 1) {
            Actor_SetMotion(a, 0x260, 6, 0);
        } else if (a->motionId[0] == 0x289) {
            Actor_SetMotion(a, 0x261, 0xA, 0);
        } else if (a->motionId[0] != 0x261) {
            Actor_SetMotion(a, 0x261, 0, 0);
        }
        break;
    case 1:
        if (Area_Is86to88(gScene.area) == 1) {
            if (--a->timer <= 0) {
                a->step++;
                Actor_SetMotion(a, 0x263, 6, 0);
            }
        } else if (--a->timer <= 0) {
            a->step++;
            if (a->motionId[0] != 0x289) {
                Actor_SetMotion(a, 0x289, 0, 0);
            }
        }
        break;
    case 2:
        if (Area_Is86to88(gScene.area) == 1) {
            if (a->motion[0].playing <= 0) {
                a->step++;
                Actor_SetMotion(a, 0x261, 4, 0);
            }
        } else if (a->motion[0].playing <= 0) {
            if (((u16)Rand_Next(1) & 1) == 0) {
                a->timer = 0;
                a->step--;
            } else {
                a->step++;
                Actor_SetMotion(a, 0x261, 4, 0);
            }
        }
        break;
    case 3:
        if (a->motion[0].playing <= 0) {
            a->step = 0;
        }
        break;
    }
}

void Npc_State14(Em *a, u8 flag)
{
    u8 step = a->step;
    NpcWork *m = (NpcWork *)&a->work;

    switch (step) {
    case 0:
        a->step = step + 1;
        a->motionTime = 0;
        m->unk2 = 1;
        Actor_SetAction(a, 1, 0, 0);
        if (flag == 1) {
            a->step = 2;
            if (a->motionId[0] != 0x261) {
                Actor_SetMotion(a, 0x261, 0, 0);
            }
        } else {
            Actor_SetMotion(a, 0x260, -4, 0);
        }
        break;
    case 1:
        m->unk2 = 1;
        if (a->motion[0].playing == 0) {
            a->step++;
            Actor_SetMotion(a, 0x261, 4, 0);
            a->state = 0x25;
        }
        break;
    case 2:
        break;
    }
}

void Npc_State48(Em *obj, s32 variant)
{
    u16 motion;
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        if (!(variant & 0xFF)) {
            motion = obj->motionId[0];
            if ((motion != 0x2A5) && (motion != 0x2A6)) {
                Actor_SetMotion(obj, 0x2A5, 0, 0);
                break;
            }
        }
        break;
    case 1:
        if (obj->motion[0].playing > 0) {
            if (variant & 0xFF) {
                goto block_10;
            }
        } else {
block_10:
            if (obj->motionId[0] != 0x2A6) {
                Actor_SetMotion(obj, 0x2A6, 0, 0);
            }
        }
        break;
    }
}

void Npc_State4A(Em *obj)
{
    ((NpcWork *)&obj->work)->unk2 = 1;
    if (obj->motionId[0] != 0x2A7) {
        Actor_SetMotion(obj, 0x2A7, 6, 0);
    }
}

void Npc_State42(Em *obj)
{
    s32 timer;
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->timer = 8;
        obj->step = (u8) (obj->step + 1);
        break;
    case 1:
        timer = obj->timer - 1;
        obj->timer = timer;
        if (timer <= 0) {
            Actor_SetState(obj, 0, 0x2CU);
        }
        break;
    }
}

void Npc_State5B(Em *obj)
{
    NpcWork *m = (NpcWork *)&obj->work;
    u8 step = obj->step;
    NpcRouteEnt *t;

    switch (step) {
    case 0:
        obj->step = step + 1;
        t = m->route;
        obj->timer = (u8)(t->arg + (u16)Rand_Next(1));
        if (Npc_IsFree(obj) == 1) {
            Actor_SetMotion(obj, 1, 0x14, 0);
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

void Npc_State84(Em *obj)
{
    NpcWork *m = (NpcWork *)&obj->work;
    u8 step = obj->step;
    NpcRouteEnt *t;

    switch (step) {
    case 0:
        obj->step = step + 1;
        t = m->route;
        t->arg = ((u16)Rand_Next(1) % 30) * 5;
        if (obj->motionId[0] == 0) {
            if (Npc_IsFree(obj) == 1) {
                Actor_SetMotion(obj, 1, 0, 0);
            } else {
                Actor_SetMotionOnLayer(obj, 0x3E9, 0, 0, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

void Npc_State85(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        if (obj->motionId[0] != 0x35D) {
            Actor_SetMotion(obj, 0x35D, 6, 0);
            obj->step = (u8) (obj->step + 1);
        }
        
    case 1:
        break;
    }
}

void Npc_State86(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        Actor_SetMotion(obj, 0x35E, 0xA, 0);
        
    case 1:
        break;
    }
}

void Npc_State87(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        Actor_SetMotion(obj, 0x265, 6, 0);
        
    case 1:
        break;
    }
}

void Npc_State88(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        Actor_SetMotion(obj, 0x266, 6, 0);
        
    case 1:
        break;
    }
}

void Npc_State89(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        Actor_SetMotion(obj, 0x262, 6, 0);
        
    case 1:
        break;
    }
}

void Npc_State4B(Em *obj)
{
    NpcWork *m = (NpcWork *)&obj->work;
    u16 mot;

    switch (obj->step) {
    case 0:
        m->unk45 = 1;
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

void Npc_State4C(Em *obj, s8 variant)
{
    NpcWork *m = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        m->unk45 = 1;
        obj->step++;
        switch (variant) {
        case 0:
            if (obj->motionId[0] != 0x285) {
                Actor_SetMotion(obj, 0x285, 0, 0);
            }
            break;
        case 1:
            if (obj->motionId[0] != 0x284) {
                Actor_SetMotion(obj, 0x284, 0, 0);
            }
            break;
        }
        break;
    }
}

void Npc_State30(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;
    u8 step;

    if (Npc_IsBusyNotAct3C4B(a) == 0) {
        Actor_SetState(a, 0, 1);
    }
    step = a->step;
    switch (step) {
    case 0:
        a->step = step + 1;
        m->unk45 = 1;
        a->timer = (u8)((u16)Rand_Next(1) + 0x12C);
        if (a->motionId[0] != 0x3FF) {
            Actor_SetMotionOnLayer(a, 0x3FF, 2, 0, 0);
        }
        break;
    case 1:
        if (--a->timer <= 0) {
            a->step++;
            Actor_SetMotionOnLayer(a, 0x400, 2, 0, 0);
        }
        break;
    case 2:
        if (a->motion[0].playing <= 0) {
            a->step = 0;
        }
        break;
    }
}

void Npc_State31(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;

    switch (a->step) {
    case 0:
        m->unk2 = 1;
        a->step++;
        Actor_SetMotionOnLayer(a, 0x401, 8, 0, 0);
        break;
    case 1:
        m->unk2 = 1;
        if (a->motion[0].playing <= 0) {
            a->step++;
            Actor_SetMotionOnLayer(a, 0x402, 2, 0, 0);
        } else if (gTownMode == 0) {
            a->step += 3;
            a->timer = ((u16)Rand_Next(1) & 0x1F) + 0x60;
        }
        break;
    case 2:
        m->unk2 = 1;
        if (a->motion[0].playing <= 0) {
            a->step++;
            Actor_SetMotionOnLayer(a, 0x403, 2, 0, 0);
        } else if (gTownMode == 0) {
            a->step += 2;
            a->timer = ((u16)Rand_Next(1) & 0x1F) + 0x60;
            Actor_SetMotionOnLayer(a, 0x402, 0xA, 0, 0);
        }
        break;
    case 3:
        m->unk2 = 1;
        if (gTownMode == 0) {
            a->step++;
            a->timer = ((u16)Rand_Next(1) & 0x1F) + 0x60;
            Actor_SetMotionOnLayer(a, 0x402, 0xA, 0, 0);
        }
        break;
    case 4:
        if (--a->timer <= 0) {
            a->step++;
            a->timer = (u8)Rand_Next(1) + 600;
            Actor_SetMotionOnLayer(a, 0x3FF, 0x3C, 0, 0);
        }
        break;
    case 5:
        if (--a->timer <= 0) {
            Actor_SetState(a, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_StateA2(Em *obj)
{
    NpcWork *m = (NpcWork *)&obj->work;
    u8 step = obj->step;

    switch (step) {
    case 0:
        obj->step = step + 1;
        if (obj->motionId[0] != 0x401) {
            Actor_SetMotionOnLayer(obj, 0x401, 8, 0, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step = step + 1;
            Actor_SetMotionOnLayer(obj, 0x402, 2, 0, 0);
        }
        break;
    case 2:
        m->unk2 = 1;
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            Actor_SetMotionOnLayer(obj, 0x403, 2, 0, 0);
        }
        break;
    }
}

void Npc_State0E(Em *obj)
{
    u8 step;

    if (Npc_IsBusyNotAct3C4B(obj) == 0) {
        Actor_SetState(obj, 0, 1);
    }
    step = obj->step;
    switch (step) {
    case 0:
        obj->step = step + 1;
        Actor_SetMotionOnLayer(obj, 0x3F7, 2, 0, 0);
        break;
    }
}

void Npc_State32(Em *a)
{
    u8 step = a->step;
    NpcWork *m = (NpcWork *)&a->work;

    switch (step) {
    case 0:
        a->step = step + 1;
        m->unk2 = 1;
        if (a->motionId[0] != 1) {
            Actor_SetMotionOnLayer(a, 0x3F8, 4, 0, 0);
        }
        break;
    case 1:
        a->step = step + 1;
        m->unk2 = 1;
        Actor_SetMotionOnLayer(a, 0x3F9, 2, 0, 0);
        break;
    case 2:
        m->unk2 = 1;
        if (a->motion[0].playing <= 0 && a->motionId[0] != 0x3FA) {
            Actor_SetMotionOnLayer(a, 0x3FA, 2, 0, 0);
        }
        if (gTownMode == 0) {
            a->timer = (u8)Rand_Next(1) + 600;
            a->step++;
            Actor_SetMotionOnLayer(a, 0x3F9, 0x1A, 0, 0);
        }
        break;
    case 3:
        if (--a->timer == 0) {
            a->step++;
            Actor_SetMotionOnLayer(a, 0x3F7, 0x1E, 0, 0);
        } else if (a->motion[0].playing == 0) {
            Actor_SetMotionOnLayer(a, 0x3F9, 2, 0, 0);
        }
        break;
    case 4:
        if (a->motion[0].playing <= 0) {
            Actor_SetState(a, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_State33(Em *obj)
{
    NpcWork *m = (NpcWork *)&obj->work;

    if (Npc_IsBusyNotAct3C4B(obj) == 0) {
        Actor_SetState(obj, 0, 1);
    }
    switch (obj->step) {
    case 0:
        m->unk45 = 1;
        obj->step++;
        if (obj->motionId[0] != 0x3FB) {
            Actor_SetMotionOnLayer(obj, 0x3FB, 2, 0, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing == 0) {
            Actor_SetMotionOnLayer(obj, 0x3FB, 2, 0x58, 0);
        }
        break;
    }
}

void Npc_State34(Em *a)
{
    u8 step;
    NpcWork *m;

    m = (NpcWork *)&a->work;
    m->unk2 = 1;
    step = a->step;
    switch (step) {
    case 0:
        if (a->motionId[0] == 0x3FB) {
            if (Motion_ReachedFrame(a, 0, 89.0f) != 0) {
                a->step++;
            }
        } else if (a->motionId[0] == 0x3FC || a->motionId[0] == 0x3F0) {
            a->step = 2;
        } else {
            a->step = step + 1;
        }
        break;
    case 1:
        a->step = step + 1;
        Actor_SetMotionOnLayer(a, 0x3FC, 4, 0, 0);
        break;
    case 2:
        if (a->motion[0].playing <= 0) {
            a->step = step + 1;
            Actor_SetMotionOnLayer(a, 0x3F0, 4, 0, 0);
        }
        break;
    case 3:
        if (gTownMode == 0) {
            Actor_SetMotionOnLayer(a, 0x3FB, 0xA, 0, 0);
            a->step++;
        }
        break;
    case 4:
        if (a->motion[0].playing <= 0) {
            Actor_SetState(a, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_State3F(Em *a)
{
    u8 step = a->step;

    switch (step) {
    case 0:
        a->step = step + 1;
        a->timer = (u8)((u16)Rand_Next(1) + 100);
        Actor_SetMotion(a, 0x2B2, 0, 0);
        break;
    case 1:
        if (--a->timer <= 0) {
            if (((u16)Rand_Next(1) & 1) == 0) {
                a->step++;
                a->timer = ((u16)Rand_Next(1) & 3) + 3;
                Actor_SetMotion(a, 0x2B3, 6, 0);
            } else {
                a->step = 4;
                Actor_SetMotion(a, 0x2B6, 8, 0);
            }
        }
        break;
    case 2:
        if (a->motion[0].playing <= 0 && --a->timer <= 0) {
            a->step++;
            Actor_SetMotion(a, 0x2B4, 0xA, 0);
        }
        break;
    case 3:
        if (a->motion[0].playing <= 0) {
            a->step = 1;
            a->timer = 0x78;
            Actor_SetMotion(a, 0x2B2, 4, 0);
        }
        break;
    case 4:
        if (a->motion[0].playing <= 0) {
            a->step = 1;
            a->timer = (u8)((u16)Rand_Next(1) + 100);
            Actor_SetMotion(a, 0x2B2, 0xA, 0);
        }
        break;
    }
}

void Npc_State40(Em *obj)
{
    u8 step;
    NpcWork *m;

    step = obj->step;
    m = (NpcWork *)&obj->work;
    switch (step) {
    case 0:
        m->unk2 = 1;
        obj->step = (u8) (obj->step + 1);
        Actor_SetMotion(obj, 0x2B5, 0xA, 0);
        Npc_TurnToPlayer(obj, 0xA);
        break;
    case 1:
        m->unk2 = 1;
        if (gTownMode == 0) {
            obj->step = (u8) (obj->step + 1);
            Actor_SetMotion(obj, 0x2B2, 0xA, 0);
            Npc_StartTurnBack(obj, 0xA);
        }
        break;
    case 2:
        if (obj->motion[1].playing <= 0) {
            Actor_SetState(obj, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_State2E(Em *obj)
{
    s32 timer;
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        obj->timer = (s32) (((Rand_Next(1) & 0xFFFF) + 0xA) & 0xFF);
        if (obj->motionId[0] != 0x28A) {
            Actor_SetMotion(obj, 0x28A, 0, 0);
        }
        break;
    case 1:
        timer = obj->timer - 1;
        obj->timer = timer;
        if (timer <= 0) {
            obj->step = (u8) (obj->step + 1);
            Actor_SetMotion(obj, 0x28E, 0xA, 0);
        }
        break;
    case 2:
        if (obj->motion[0].playing == 0) {
            obj->step = 0U;
        }
        break;
    }
}

void Npc_State2F(Em *obj)
{
    s32 timer;
    u8 step;
    NpcWork *m;

    step = obj->step;
    m = (NpcWork *)&obj->work;
    switch (step) {
    case 0:
        m->unk2 = 1;
        obj->step = (u8) (obj->step + 1);
        if (obj->motionId[0] != 0x28B) {
            Actor_SetMotion(obj, 0x28B, 8, 0);
        }
        break;
    case 1:
        if ((obj->motion[0].playing == 0) && (obj->motionId[0] == 0x28B)) {
            Actor_SetMotion(obj, 0x28C, 2, 0);
        }
        if (gTownMode == 0) {
            if (Game_IsOnline() == 0) {
                if (Npc2_NearNpc1(obj) != 1) {
                    goto block_15;
                }
            } else {
block_15:
                obj->step = (u8) (obj->step + 1);
                Actor_SetMotion(obj, 0x28D, 2, 0);
                break;
            }
        }
        break;
    case 2:
        if (obj->motion[0].playing == 0) {
            Actor_SetMotion(obj, 0x28A, 0xE, 0);
            obj->timer = (s32) ((Rand_Next(1) & 0xFF) + 0xFF);
            obj->step = (u8) (obj->step + 1);
        }
        break;
    case 3:
        timer = obj->timer - 1;
        obj->timer = timer;
        if (timer <= 0) {
            Actor_SetState(obj, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_State3C(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = a->step;
    f32 d;

    switch (step) {
    case 0:
        a->step = step + 1;
        if (pl->state == 3 || pl->state == 1) {
            Actor_SetMotion(a, 0x2AC, 0xA, 0);
        } else {
            Actor_SetMotion(a, 0x2AD, 8, 0);
        }
        break;
    case 1:
        d = Vec_Distance(&a->posX, (f32 *) pl->pos);
        if (d < 300.0f) {
            a->step++;
            if (a->motionId[0] != 0x2AD) {
                Actor_SetMotion(a, 0x2AD, 8, 0);
            }
        } else if (!(d <= 400.0f)) {
            a->step = 3;
            a->timer = 0x78;
        }
        break;
    case 2:
        if (a->motion[0].playing <= 0) {
            Actor_SetState(a, 0, 0x3D);
        }
        break;
    case 3:
        if (--a->timer <= 0) {
            Actor_SetState(a, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_State3D(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = a->step;

    switch (step) {
    case 0:
        a->step = step + 1;
        if (((u16)Rand_Next(1) & 1) == 0) {
            Actor_SetMotion(a, 0x2A9, 4, 0);
        } else {
            Actor_SetMotion(a, 0x2A8, 8, 0);
        }
        break;
    case 1:
        if (!(Vec_Distance(&a->posX, (f32 *) pl->pos) <= 300.0f)) {
            a->step++;
            a->timer = 0x78;
        }
        break;
    case 2:
        if (--a->timer <= 0) {
            if (Vec_Distance(&a->posX, (f32 *) pl->pos) < 300.0f) {
                a->step--;
            } else {
                Actor_SetState(a, 0, *(u16 *)&m->route[m->routeIdx].state);
            }
        }
        break;
    }
}

void Npc_State3E(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;
    Player *pl = &gPlayers[gScene.localPlayer];

    switch (npc->step) {
    case 0:
        m->unk2 = 1;
        npc->step++;
        Actor_SetMotion(npc, 0x2AA, 10, 0);
        break;
    case 1:
        m->unk2 = 1;
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
            Actor_SetState(npc, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_State7A(Em *obj)
{
    s32 near;
    NpcWork *m;

    m = (NpcWork *)&obj->work;
    Npc_State43(obj, 0);
    near = Npc2_NearNpc1(obj);
    switch (near) {
    case 0:
        Actor_SetState(obj, 0, *(u16 *)&m->route[m->routeIdx].state);
        break;
    case 2:
        Npc_PickIdleState(obj);
        break;
    }
}

/* ---- 0x7855B0 (was unk_7855B0.c) ---- */
void Npc_State5F(Em *npc) {
    npc->state = 0x60;
    ((NpcWork *)&npc->work)->unk49 = 1;
}

/* ---- 0x7855D0 (was unk_7855D0.c) ---- */
typedef struct NpcVec3 {
    f32 x, y, z;
} NpcVec3;

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);

f32 Vec_Distance(f32 *a, f32 *b);

/* A townsperson that steps along its route by the player: it moves on
 * when the player (within 800) is nearer the next point than this one. */
void Npc_State66(Em *a)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&a->work;
    u8 *cur;
    f32 dnext;
    f32 dcur;

    dnext = Vec_Distance((f32 *) pl->pos, (f32 *) ((u8 *)&w->route[w->routeIdx + 1]));
    cur = (u8 *)&w->route[w->routeIdx];
    dcur = Vec_Distance((f32 *) pl->pos, (f32 *) cur);
    switch (a->step) {
    case 0:
        a->step++;
        *(NpcVec3 *)a->goal = *(NpcVec3 *)cur;
        *(NpcVec3 *)(&a->posX) = *(NpcVec3 *)a->goal;
        w->unk2 = 1;
        if (a->motionId[0] == 0x354) {
            a->rotY = 0x2000;
            Actor_SetMotion(a, 0x355, 0, 0);
        } else {
            a->step = 2;
            a->rotY = 0x2000;
            Actor_SetMotion(a, 1, 0x10, 0);
        }
        break;
    case 1:
        if (a->motion[0].playing <= 0) {
            a->step++;
            Actor_SetMotion(a, 1, 0x10, 0);
        } else {
            w->unk2 = 1;
        }
        break;
    case 2:
        if ((dcur < 800.0f || dnext < 800.0f) && pl->pos[1] < 200.0f && dcur > dnext) {
            a->step++;
            Actor_SetMotion(a, 0x352, 0, 0);
            w->unk2 = 1;
        }
        break;
    case 3:
        if (a->motion[0].playing <= 0) {
            if ((dcur < 800.0f || dnext < 800.0f) && dcur < dnext) {
                a->step = 1;
                *(NpcVec3 *)a->goal = *(NpcVec3 *)cur;
                *(NpcVec3 *)(&a->posX) = *(NpcVec3 *)a->goal;
                Actor_SetMotion(a, 0x355, 0, 0);
                w->unk2 = 1;
            } else {
                (w->routeIdx)++;
                Actor_SetState(a, 0, *(u16 *)&w->route[w->routeIdx].state);
                a->timer = 1;
            }
        } else {
            w->unk2 = 1;
        }
        break;
    }
}


/* The same as Npc_State66, back along the route (the previous point). */
void Npc_State67(Em *a)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&a->work;
    u8 *cur;
    f32 dnext;
    f32 dcur;

    dnext = Vec_Distance((f32 *) pl->pos, (f32 *) ((u8 *)&w->route[w->routeIdx - 1]));
    cur = (u8 *)&w->route[w->routeIdx];
    dcur = Vec_Distance((f32 *) pl->pos, (f32 *) cur);
    switch (a->step) {
    case 0:
        a->step++;
        *(NpcVec3 *)a->goal = *(NpcVec3 *)cur;
        *(NpcVec3 *)(&a->posX) = *(NpcVec3 *)a->goal;
        w->unk2 = 1;
        if (a->motionId[0] == 0x352) {
            a->rotY = 0x2AAB;
            Actor_SetMotion(a, 0x353, 0, 0);
        } else {
            a->step = 2;
            a->rotY = 0x2AAB;
            Actor_SetMotion(a, 1, 0x10, 0);
        }
        break;
    case 1:
        if (a->motion[0].playing <= 0) {
            a->step++;
            Actor_SetMotion(a, 1, 0x10, 0);
        } else {
            w->unk2 = 1;
        }
        break;
    case 2:
        if ((dcur < 800.0f || dnext < 800.0f) && pl->pos[1] < 200.0f && dcur > dnext) {
            a->step++;
            Actor_SetMotion(a, 0x354, 0, 0);
            w->unk2 = 1;
        }
        break;
    case 3:
        if (a->motion[0].playing <= 0) {
            if ((dcur < 800.0f || dnext < 800.0f) && dcur < dnext) {
                a->step = 1;
                *(NpcVec3 *)a->goal = *(NpcVec3 *)cur;
                *(NpcVec3 *)(&a->posX) = *(NpcVec3 *)a->goal;
                Actor_SetMotion(a, 0x353, 0, 0);
                w->unk2 = 1;
            } else {
                (w->routeIdx)--;
                Actor_SetState(a, 0, *(u16 *)&w->route[w->routeIdx].state);
                a->timer = 1;
            }
        } else {
            w->unk2 = 1;
        }
        break;
    }
}

struct V0077C490_0 *NpcTable_Find(u8 kind, u16 id); /* unit's own declaration: with the definition's V0077C490_0 *(u8 kind, s32 id) (lobby/unk_77A960.c), Npc_State68 2 of 96 words differ */
void Npc_TurnToPlayer(Em *arg0, s32 arg1);
void Npc_StartTurnTo(Em *arg0, s32 arg1, s32 arg2);
void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
extern s32 gTownMode;

void Npc_State68(Em *a, u8 flag)
{
    NpcWork *m = (NpcWork *)&a->work;
    u8 step;

    NpcTable_Find(a->work, gScene.area);
    m->unk2 = 1;
    step = a->step;
    switch (step) {
    case 0:
        a->step = step + 1;
        Npc_TurnToPlayer(a, 6);
        if (a->motionId[0] != 0x40A) {
            if (flag == 0) {
                Actor_SetMotionOnLayer(a, 0x40A, 4, 0, 0);
            } else {
                Actor_SetMotionOnLayer(a, 0x40A, 4, 0x9A, 0);
            }
        }
        break;
    case 1:
        if (gTownMode == 0) {
            if (!(a->posY <= 200.0f)) {
                Npc_StartTurnTo(a, 0xC, 0xED84);
            }
            a->step++;
            Actor_SetMotionOnLayer(a, 0x406, 0xA, 0, 0);
        }
        break;
    case 2:
        if ((u16)m->turnCount == 0) {
            Actor_SetState(a, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

s32 Motion_ReachedFrame(void *, s32, f32);

void Npc_State69Solo(Em *a)
{
    NpcWork *w = (NpcWork *)&a->work;

    switch (a->step) {
    case 0:
        a->step++;
        if (a == gPlayers[gScene.localPlayer].target) {
            w->turnStep = -w->turnStep;
            w->turnCount = 6;
        }
        w->motionPhase = 0;
        if (a->motionId[0] != 0x40B) {
            Actor_SetMotionOnLayer(a, 0x40B, 0, 0, 0);
        }
        break;
    case 1:
        if (a->motion[0].playing <= 0) {
            a->step++;
            a->timer = 2;
            Actor_SetMotionOnLayer(a, 0x40C, 4, 0, 0);
        }
        break;
    case 2:
        if (w->motionPhase == 0 && Motion_ReachedFrame(a, 0, 30.0f)) {
            w->motionPhase = 1;
        }
        if (a->motion[0].playing <= 0 && --a->timer <= 0) {
            a->step++;
            Actor_SetMotionOnLayer(a, 0x40D, 0, 0, 0);
        }
        break;
    case 3:
        if (w->motionPhase == 1 && Motion_ReachedFrame(a, 0, 118.0f)) {
            w->motionPhase = 0;
        }
        if (a->motion[0].playing <= 0) {
            a->step++;
            if (a == gPlayers[gScene.localPlayer].target) {
                Npc_TurnToPlayer(a, 6);
            }
            Actor_SetMotionOnLayer(a, 0x40E, 0x10, 0, 0);
        }
        break;
    case 4:
        if (a->motion[0].playing <= 0) {
            if (a == gPlayers[gScene.localPlayer].target) {
                Actor_SetState(a, 0, 0x7F);
            } else {
                Actor_SetState(a, 0, *(u16 *)&w->route[w->routeIdx].state);
            }
        }
        break;
    }
}


void Npc_State69Pair(Em *a)
{
    NpcWork *m = (NpcWork *)&a->work;
    Em *o = Npc_Find(0x1A);
    u8 step;

    if (o == NULL) {
        return;
    }
    step = a->step;
    switch (step) {
    case 0:
        a->step = step + 1;
        if (a == gPlayers[gScene.localPlayer].target) {
            m->turnStep = -m->turnStep;
            m->turnCount = 6;
        }
        if (a->motionId[0] != 0x40B) {
            Actor_SetMotionOnLayer(a, 0x40B, 0, 0, 0);
        }
        break;
    case 1:
        if (a->motion[0].playing <= 0) {
            a->step = step + 1;
            Actor_SetMotionOnLayer(a, 0x406, 4, 0, 0);
        }
        break;
    case 2:
        if (a->motion[0].playing <= 0 && o->motionId[0] == 0x358) {
            a->step = step + 1;
        }
        break;
    case 3:
        if (o->motion[0].playing <= 0) {
            a->step = step + 1;
            if (a == gPlayers[gScene.localPlayer].target) {
                Npc_TurnToPlayer(a, 6);
            }
            Actor_SetMotionOnLayer(a, 0x40E, 4, 0, 0);
        }
        break;
    case 4:
        if (a->motion[0].playing <= 0) {
            Actor_SetState(a, 0, 0x7F);
        }
        break;
    }
}

void Npc_State69Solo(Em *);
void Npc_State69Pair(Em *a);

void Npc_State69(Em *obj)
{
    ((NpcWork *)&obj->work)->unk2 = 1;
    switch ((u8)Village_FacilityLevel(2)) {
    case 0:
        Npc_State69Solo(obj);
        break;
    default:
        if (Npc_Find(0xF) != 0) {
            Npc_State69Solo(obj);
        } else {
            Npc_State69Pair(obj);
        }
        break;
    }
}


/* A townsperson that waits on townsperson 0xE: stands, turns, waits for
 * it to reach motion 0x406, and runs off when it reaches 0x40B. */
void Npc_State76(Em *a)
{
    Em *other;
    NpcWork *w = (NpcWork *)&a->work;

    other = Npc_Find(0xE);

    if (other == NULL) {
        return;
    }
    switch (a->step) {
    case 0:
        if (a->motionId[0] != 0x357) {
            Actor_SetMotion(a, 0x357, 6, 0);
            a->timer = 3;
            w->motionPhase = (u16)Rand_Next(1) & 1;
        }
        a->step++;
        break;
    case 1:
        if (a->motion[0].playing <= 0) {
            if (--a->timer <= 0) {
                Actor_SetMotion(a, 0x358, 0, 0);
                a->step++;
            }
        }
        break;
    case 2:
        if (a->motion[0].playing <= 0) {
            Actor_SetMotion(a, 0x356, 6, 0);
            a->step++;
        }
        break;
    case 3:
        if (a->motion[0].playing <= 0 && other->motionId[0] == 0x406) {
            a->step = 0;
        }
        break;
    case 4:
        Actor_SetMotion(a, 0x359, 6, 0);
        a->step++;
        break;
    case 5:
        if (a->motion[0].playing <= 0 && other->motionId[0] == 0x406) {
            a->step = 0;
        }
        break;
    }
    if (other->motionId[0] == 0x408 && a->step <= 3) {
        a->step = 4;
    }
    if (other->motionId[0] == 0x40B && Npc_Find(0xF) == 0) {
        Actor_SetState(a, 0, 0x78);
        return;
    }
    if ((other->state == 0x68 || other->state == 0x7F) && a->motionId[0] != 0x356) {
        a->step = 3;
        Actor_SetMotion(a, 0x356, 0x14, 0);
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);

void Npc_State9D(Em *a)
{
    switch (a->step) {
    case 0:
        if (a->type == 1) {
            Actor_SetMotionOnLayer(a, 0x40C, 4, 0, 0);
            a->step = 0x63;
        } else if (a->motionId[0] != 0x358) {
            Actor_SetMotion(a, 0x358, 0, 0);
            a->step++;
        }
        break;
    case 1:
        if (a->motion[0].playing <= 0) {
            a->timer = (u16)Rand_Next(1) & 0x3F;
            a->step++;
        }
        break;
    case 2:
        if (--a->timer <= 0) {
            a->step++;
            a->timer = ((u16)Rand_Next(1) + 3) & 3;
            if (a->motionId[0] != 0x357) {
                Actor_SetMotion(a, 0x357, 6, 0);
            }
        }
        break;
    case 3:
        if (Motion_PassedFrame((Player *)a, 0, 226.0f)) {
            a->motionSpeed = 1.0f;
        }
        if (a->motion[0].playing <= 0) {
            if (--a->timer <= 0) {
                a->step = 0;
            }
        }
        break;
    }
}

void Npc_State78(Em *obj)
{
    s32 other;
    s32 timer;
    u16 motion;
    u8 step;
    NpcWork *m;

    m = (NpcWork *)&obj->work;
    other = (s32)Npc_Find(0xE);
    if (other != 0) {
        step = obj->step;
        switch (step) {
        case 0:
            obj->step = (u8) (step + 1);
            if (obj->motionId[0] != 0x357) {
                Actor_SetMotion(obj, 0x357, 6, 0);
                obj->timer = 3;
            }
            break;
        case 1:
            if (obj->motion[0].playing <= 0) {
                timer = obj->timer - 1;
                obj->timer = timer;
                if (timer <= 0) {
                    Actor_SetMotion(obj, 0x358, 0, 0);
                    obj->step = (u8) (obj->step + 1);
                    break;
                }
            }
            break;
        case 2:
            if (obj->motion[0].playing <= 0) {
                Actor_SetMotion(obj, 0x356, 0, 0);
                obj->step = (u8) (obj->step + 1);
            }
            break;
        case 3:
            motion = *(u16 *)((u8 *)other + 0x29C);
            if (motion == 0x40B) {
                Actor_SetState(obj, 0, 0x78U);
                break;
            }
            if ((obj->motion[0].playing <= 0) && (motion == 0x406)) {
                Actor_SetState(obj, 0, *(u16 *)&m->route[m->routeIdx].state);
            }
            break;
        }
    }
}

s32 Motion_ReachedFrame(void *, s32, f32);

void Npc_State77Pair(Em *a)
{
    Em *o = Npc_Find(0x1A);
    NpcWork *om;

    if (o == NULL) {
        return;
    }
    om = (NpcWork *)&o->work;
    switch (a->step) {
    case 0:
        if (a->motionId[0] != 0x406) {
            a->targetYaw = a->rotY = 0xED84;
            Actor_SetMotionOnLayer(a, 0x406, 0, 0, 0);
        }
        a->step++;
        a->timer = 0x3C;
        break;
    case 1:
        if (--a->timer <= 0) {
            if (o->motionId[0] == 0x357) {
                if (om->motionPhase == 1) {
                    a->step++;
                }
            } else if (o->motionId[0] == 0x356 && o->state != 0x2C && o->step > 2) {
                Actor_SetMotionOnLayer(a, 0x407, 0xA, 0, 0);
                a->step = 3;
            }
        }
        break;
    case 2:
        if (o->timer < 3 && Motion_ReachedFrame(o, 0, 234.0f) != 0) {
            Actor_SetMotionOnLayer(a, 0x408, 4, 0, 0);
            a->step++;
        }
        break;
    case 3:
        if (a->motion[0].playing <= 0 || o->state == 0x2C) {
            a->step = 1;
            a->timer = 0x96;
            Actor_SetMotionOnLayer(a, 0x406, 4, 0, 0);
        }
        break;
    }
}

void Npc_State7B(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        if (obj->motionId[0] != 0x19F) {
            Actor_SetMotion(obj, 0x19F, 0, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x19F, 0, 0);
        }
        break;
    }
}

void Npc_State7C(Em *obj)
{
    if (obj->motionId[0] != 0x273) {
        Actor_SetMotion(obj, 0x273, 6, 0);
    }
}

extern s32 gTownMode;

/* Matches with Npc_StartTurnBack static (round 19: the five Npc files are one unit). */
void Npc_StateA1(Em *a)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = a->step;

    switch (step) {
    case 0:
        a->step = step + 1;
        a->stateVar0 = 0;
        if (a->motionId[0] != 0x273) {
            Actor_SetMotion(a, 0x273, 6, 0);
        }
    case 1:
        switch (gTownMode) {
        case 0x30:
        case 0x32:
            if (pl->stateGroup == 0) {
                switch (pl->state) {
                case 0x75:
                case 0x91:
                    return;
                }
            }
            break;
        }
        break;
    }
    if (a->stateVar0 == 0) {
        a->stateVar0++;
        Npc_StartTurnBack(a, 0xA);
        Npc_EnterRoutePoint(a);
    }
}

void Npc_StateA6(Em *a, s32 first)
{
    u8 step = a->step;

    switch (step) {
    case 0:
        if (first == 0) {
            a->step = 2;
            a->stateVar0 = 0;
            if (a->motionId[0] != 0x277) {
                Actor_SetMotion(a, 0x277, 0xA, 0);
            }
        } else {
            a->step = step + 1;
            a->timer = 0xA0;
        }
        break;
    case 1:
        if (--a->timer <= 0) {
            a->step++;
            a->stateVar0 = 0;
            if (a->motionId[0] != 0x277) {
                Actor_SetMotion(a, 0x277, 6, 0);
            }
        }
        break;
    case 2:
        if (a->motion[0].playing == 0) {
            a->step = step + 1;
            Actor_SetMotion(a, 0x36C, 0xA, 0);
        }
        break;
    case 3:
        switch (gTownMode) {
        case 0x30:
        case 0x32:
            return;
        default:
            if (a->stateVar0 == 0) {
                a->stateVar0++;
                Npc_StartTurnBack(a, 0xA);
                Npc_EnterRoutePoint(a);
            }
            break;
        }
        break;
    }
}


extern NpcVec3 gNpcState6ASpot;
f32 Vec_Distance(f32 *a, f32 *b);

/* A townsperson that idles until the player comes near gNpcState6ASpot, then
 * jumps to another place and moves on. */
void Npc_State6A(Em *a)
{
    NpcWork *w = (NpcWork *)&a->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcVec3 spot = gNpcState6ASpot;

    w->motionPhase = 0;
    switch (a->step) {
    case 0:
        a->step++;
        if (a->motionId[0] != 0x406) {
            Actor_SetMotionOnLayer(a, 0x406, 4, 0, 0);
            a->timer = 1;
        }
        break;
    case 1:
        if (a->motion[0].playing <= 0 && --a->timer <= 0) {
            a->timer = (u16)Rand_Next(1) & 3;
            if ((u16)Rand_Next(1) & 1) {
                Actor_SetMotionOnLayer(a, 0x406, 6, 0, 0);
            } else {
                Actor_SetMotionOnLayer(a, 0x407, 0xA, 0, 0);
            }
        }
        if (pl->pos[1] < 200.0f && Vec_Distance((f32 *) pl->pos, (f32 *) &spot) <= 450.0f) {
            a->step++;
            Actor_SetMotionOnLayer(a, 0x40F, 2, 0, 0);
        }
        break;
    case 2:
        w->unk2 = 1;
        if (a->motion[0].playing <= 0) {
            a->step++;
            a->timer = 10;
        }
        break;
    case 3:
        w->unk2 = 1;
        if (--a->timer <= 0) {
            a->step++;
            w->motionPhase = 2;
            a->posX = 10375.0f;
            a->posY = 0.0f;
            a->posZ = 9055.0f;
            *(NpcVec3 *)a->goal = *(NpcVec3 *)(&a->posX);
            a->targetYaw = a->rotY = 0x2AAB;
            Actor_SetMotionOnLayer(a, 0x411, 0, 0, 0);
        }
        break;
    case 4:
        w->motionPhase = 2;
        w->unk2 = 1;
        if (a->motion[0].playing <= 0 || !(pl->pos[1] <= 240.0f)) {
            (w->routeIdx)--;
            Actor_SetState(a, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

extern NpcVec3 gNpcState6AJumpPos;
f32 Vec_Distance(f32 *a, f32 *b);

/* A townsperson that waits at a spot: when the player (condition
 * 0x2F) comes near gNpcState6AJumpPos, it jumps to its place and moves on. */
void Npc_State6B(Em *a)
{
    NpcWork *w = (NpcWork *)&a->work;
    Player *pl = &gPlayers[gScene.localPlayer];
    NpcVec3 spot = gNpcState6AJumpPos;

    w->motionPhase = 2;
    switch (a->step) {
    case 0:
        a->step++;
        if (a->motionId[0] != 0x409) {
            a->targetYaw = a->rotY = 0x2AAB;
            Actor_SetMotionOnLayer(a, 0x409, 0x1E, 0, 0);
            a->timer = (u16)Rand_Next(1) % 120 + 120;
        }
        break;
    case 1:
        if (a->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(a, 0x409, 0x1E, 0, 0);
            a->timer = (u16)Rand_Next(1) % 120 + 120;
        }
        if (Cond_Check(0x2F) == 1 && !(pl->pos[1] <= 240.0f) && Vec_Distance((f32 *) pl->pos, (f32 *) &spot) <= 310.0f) {
            a->step++;
            a->posX = 9979.0f;
            a->posY = 248.0f;
            a->posZ = 8661.0f;
            *(NpcVec3 *)a->goal = *(NpcVec3 *)(&a->posX);
            a->targetYaw = a->rotY = 0xED84;
            a->timer = 1;
            Actor_SetMotionOnLayer(a, 0x410, 0, 0, 0);
            w->motionPhase = 0;
        }
        break;
    case 2:
        w->unk2 = 1;
        w->motionPhase = 0;
        if (a->motion[0].playing <= 0) {
            (w->routeIdx)++;
            Actor_SetState(a, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}


void Npc_State77Solo(Em *a)
{
    Player *pl = &gPlayers[gScene.localPlayer];
    u8 step = a->step;

    switch (step) {
    case 0:
        a->step = step + 1;
        Actor_SetAction(a, 0, 0, 0);
        a->timer = (u16)Rand_Next(1) & 3;
        Actor_SetMotionOnLayer(a, 0x406, 0, 0, 0);
        break;
    case 1:
    case 2:
        if (a->posY < 90.0f) {
            if (!(pl->pos[1] < 100.0f) && Cond_Check(0x2F) == 1) {
                Actor_SetState(a, 0, 0x6A);
                break;
            }
        } else if (pl->pos[1] < 100.0f) {
            Actor_SetState(a, 0, 0x6B);
            break;
        }
        if (a->motion[0].playing <= 0 && --a->timer <= 0) {
            a->timer = (u16)Rand_Next(1) & 3;
            if ((u16)Rand_Next(1) & 1) {
                Actor_SetMotionOnLayer(a, 0x406, 4, 0, 0);
            } else {
                Actor_SetMotionOnLayer(a, 0x407, 0xA, 0, 0);
            }
        }
        break;
    }
}

/* The 0x7855D0 part's .data (0x7FE490-0x7FE4B0) */
NpcVec3 gNpcState6ASpot = {
#include "lobby/007FE490.inc"
};
NpcVec3 gNpcState6AJumpPos = {
#include "lobby/007FE4A0.inc"
};

/* ---- 0x7873A0 (was unk_7873A0.c) ---- */

void Npc_State77(Em *npc) {
    if (!(Village_FacilityLevel(2) & 0xFF)) {
        Npc_State77Solo(npc);
        return;
    }
    Npc_State77Pair(npc);
}

/* ---- 0x7873F0 (was unk_7873F0.c) ---- */

void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
s32 Motion_ReachedFrame(void *, s32, f32);
void VPart_Spawn(void *arg0);

void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);

f32 Vec_Distance(f32 *a, f32 *b);

void Npc_State6D(Em *obj)
{
    NpcWork *w;
    Em *other;

    w = (NpcWork *)&obj->work;
    Npc_Find(0x32);
    if (w->id == 0x31) {
        other = Npc_Find(0x32);
    } else {
        other = Npc_Find(0x31);
    }
    switch (obj->step) {
    case 0:
        obj->step++;
        if (w->id == 0x31) {
            Actor_SetMotion(obj, 0x2A3, 0x10, 0);
        } else {
            Actor_SetMotion(obj, 1, 0x10, 0);
        }
        break;
    case 1:
        if (other != NULL && Vec_Distance(&obj->posX, &other->posX) < 200.0f) {
            obj->step++;
            obj->timer = (u16)Rand_Next(1) & 0xF;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            obj->step++;
            if (w->id == 0x31) {
                Actor_SetMotion(obj, 0x2B8, 0xA, 0);
            } else {
                Actor_SetMotion(obj, 0x290, 0xA, 0);
            }
        }
        break;
    case 3:
        break;
    }
}

void Npc_State6E(Em *obj)
{
    u8 sub;

    sub = obj->stateVar0;
    switch (sub) {
    case 0:
        obj->stateVar0 = (u8) (sub + 1);
        Actor_SetMotion(obj, 0x373, 6, 0);
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotion(obj, 0x373, 0, 0xD4);
        }
        break;
    }
}

void Npc_State70(Em *obj)
{
    if (obj->motionId[0] != 0x375) {
        Actor_SetMotion(obj, 0x375, 6, 0);
    }
}

void Npc_State6F(Em *obj)
{
    if (obj->motionId[0] != 0x376) {
        Actor_SetMotion(obj, 0x376, 4, 0);
        VPart_Spawn(obj);
    }
}

void Npc_State72(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        if (obj->motionId[0] != 0x374) {
            Actor_SetMotion(obj, 0x374, 6, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step = (u8) (step + 1);
            Actor_SetState(obj, 0, 1U);
        }
        break;
    }
}

void Npc_State73(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    if (obj->work == 5) {
        w->unk2 = 1;
    }
    switch (obj->step) {
    case 0:
        obj->step++;
        if (w->id == 5 && obj->motionId[0] != 0x378) {
            Actor_SetMotion(obj, 0x378, 6, 0x31C);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            if (w->id == 5 || w->id == 0x10) {
                Actor_SetState(obj, 0, *(u16 *)&w->route[w->routeIdx].state);
            }
        }
        break;
    case 2:
        break;
    }
}

void Npc_State74(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    if (obj->work == 5) {
        if (obj->motionId[0] != 0x378) {
            Actor_SetMotion(obj, 0x378, 0, 0);
        }
        w->unk2 = 1;
    } else if (obj->work == 0x10) {
        if (obj->motionId[0] != 0x360) {
            Actor_SetMotion(obj, 0x360, 0x14, 0);
        }
    }
}

void Npc_State75(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        obj->step++;
        if (w->id == 5) {
            if (obj->motionId[0] != 0x377) {
                Actor_SetMotion(obj, 0x377, 4, 0);
            }
        } else if (w->id == 0x10) {
            if (obj->motionId[0] != 0x35F) {
                Actor_SetMotion(obj, 0x35F, 0x14, 0);
            }
        }
        break;
    case 1:
        break;
    }
}

void Npc_State7D(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        if (obj->motionId[0] != 0x36B) {
            Actor_SetMotion(obj, 0x36B, 0x14, 0);
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

void Npc_State7E(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;
    Player *me;
    f32 pos[3];

    switch (obj->step) {
    case 0:
        obj->step++;
        obj->stateVar0 = 0;
        if (obj->motionId[0] != 0x36C) {
            Actor_SetMotion(obj, 0x36C, 0x14, 0);
        }
        break;
    case 1:
        if (obj->variant != 0x37 || (u16)w->turnCount != 0) {
            break;
        }
        me = &gPlayers[gScene.localPlayer];
        if (Vec_Distance((f32 *) me->pos, &obj->posX) <= 400.0f && Game_IsOnline() == 0) {
            switch (gTownMode) {
            case 0x30:
            case 0x32:
                if (obj->stateVar0 == 0) {
                    obj->stateVar0++;
                    if (gTownMode == 0x30) {
                        Npc_TurnToPlayer(obj, 10);
                    } else {
                        pos[0] = 9216.0f;
                        pos[1] = 126.0f;
                        pos[2] = 10984.0f;
                        w->turnDelta = Actor_HeadingDiff(obj, pos);
                        if (!(Math_Absf(w->turnDelta) <= 10.0f)) {
                            w->homeYaw = obj->rotY;
                        }
                        w->turnStep = w->turnDelta / 10;
                        w->turnCount = 10;
                    }
                    Npc_HoldRoute(obj);
                }
                if (me->stateGroup == 0) {
                    switch (me->state) {
                    case 0x75:
                    case 0x91:
                        if (gTownMode == 0x32) {
                            w->turnDelta = Actor_HeadingDiff(obj, me->pos);
                            w->turnStep = w->turnDelta / 10;
                            w->turnCount = 10;
                        }
                        Actor_SetState(obj, 0, 0xA1);
                        break;
                    case 0x74:
                        Actor_SetState(obj, 0, 0xA6);
                        break;
                    case 0x90:
                        Actor_SetState(obj, 0, 0xA7);
                        break;
                    }
                }
                break;
            default:
                if (obj->stateVar0 != 0) {
                    obj->stateVar0 = 0;
                    Npc_StartTurnBack(obj, 10);
                    Npc_EnterRoutePoint(obj);
                }
                break;
            }
        } else if (obj->stateVar0 != 0) {
            obj->stateVar0 = 0;
            Npc_StartTurnBack(obj, 10);
            Npc_EnterRoutePoint(obj);
        }
        break;
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
void Npc_StartTurnTo(Em *arg0, s32 arg1, s32 arg2);

void Npc_State81(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *p = &w->route[w->routeIdx];

    w->unk2 = 1;
    Npc_StartTurnTo(npc, 10, p->arg);
    Npc_NextRoutePoint(npc);
}

s32 Actor_HeadingDiff(void *obj, void *pos);

void Npc_State82(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;
    NpcPos p;
    f32 d;

    switch (obj->step) {
    case 0:
        Actor_SetAction(obj, 0, 0, 0);
        obj->step++;
        if ((u16)Rand_Next(1) & 1) {
            if (obj->motionId[0] != 3) {
                if (obj->motionId[0] == 0x36B) {
                    Actor_SetMotion(obj, 3, 0xC, 0);
                } else {
                    Actor_SetMotion(obj, 3, 4, 0);
                }
            }
        } else {
            if (obj->motionId[0] != 2) {
                if (obj->motionId[0] == 0x36B) {
                    Actor_SetMotion(obj, 2, 0xC, 0);
                } else {
                    Actor_SetMotion(obj, 2, 4, 0);
                }
            }
        }
        break;
    case 1:
        p = w->route[w->routeIdx].pos;
        p.v[1] = obj->posY;
        obj->targetYaw = (s16)(u16)Actor_HeadingDiff(obj, &p) / 10;
        obj->rotY += (s16)obj->targetYaw;
        d = Vec_Distance(&obj->posX, (f32 *) &p);
        if (d < 0.0f) {
            d *= -1.0f;
        }
        if (d <= 40.0f) {
            Npc_NextRoutePoint(obj);
        }
        break;
    }
}

void Npc_State8A(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        if (obj->motionId[0] != 0x35A) {
            Actor_SetMotion(obj, 0x35A, 6, 0);
            obj->timer = ((u16)Rand_Next(1) & 3) + 2;
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            if (--obj->timer <= 0) {
                if (obj->motionId[0] != 0x362) {
                    Actor_SetMotion(obj, 0x362, 0, 0);
                    obj->timer = 1;
                } else {
                    Actor_SetMotion(obj, 0x35A, 6, 0);
                    obj->timer = ((u16)Rand_Next(1) & 3) + 2;
                }
            }
        }
        break;
    }
}

void Npc_State8C(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        if (obj->motionId[0] != 0x35C) {
            Actor_SetMotion(obj, 0x35C, 0x14, 0);
        }
        obj->step = (u8) (obj->step + 1);
        
    case 1:
        break;
    }
}

s32 Actor_HeadingDiff(void *obj, void *pos);

void Npc_State8E(Em *npc)
{
    NpcWork *m = (NpcWork *)&npc->work;

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
        m->unk2 = 1;
        npc->rotY += (s16)npc->targetYaw / 10;
        if (--npc->timer <= 0) {
            npc->step++;
        }
        break;
    case 3:
        if (npc->motion[0].playing <= 0) {
            Actor_SetState(npc, 0, 0x2C);
        }
        break;
    }
}


void Npc_State8F(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;
    f32 d;

    w->unk45 = 1;
    d = Vec_Distance(&obj->posX, (f32 *) gPlayers[gScene.localPlayer].pos);
    switch (obj->step) {
    case 0:
        if (obj->motionId[0] != 0x364) {
            Actor_SetMotion(obj, 0x364, 4, 0);
        }
        obj->step++;
        break;
    case 1:
        if (Cond_Check(100) == 1 && w->id == 0x2F) {
            break;
        }
        if (d < 300.0f) {
            Actor_SetMotion(obj, 0x365, 0, 0);
            obj->step++;
        }
        break;
    case 2:
        if (!(d < 300.0f)) {
            Actor_SetMotion(obj, 0x366, 2, 0);
            obj->step++;
        }
        break;
    case 3:
        if (obj->motion[0].playing <= 0) {
            obj->step = 0;
        }
        break;
    }
}

void Npc_State92(Em *obj)
{
    switch (obj->step) {
    case 0:
        if (obj->motionId[0] != 0x421) {
            if (obj->motionId[0] == 0x3F0 || obj->motionId[0] == 0x423 || obj->motionId[0] == 0x3E9) {
                Actor_SetMotionOnLayer(obj, 0x421, 0x1E, 0, 0);
            } else {
                Actor_SetMotionOnLayer(obj, 0x421, 4, 0, 0);
            }
        }
        obj->timer = ((u16)Rand_Next(1) & 3) + 3;
        obj->step++;
        break;
    case 1:
        if (obj->motion[0].playing <= 0 && --obj->timer <= 0) {
            Actor_SetMotionOnLayer(obj, 0x422, 6, 0, 0);
            obj->step++;
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0 && --obj->timer <= 0) {
            obj->step = 1;
            Actor_SetMotionOnLayer(obj, 0x421, 0x1E, 0, 0);
            obj->timer = ((u16)Rand_Next(1) & 3) + 3;
        }
        break;
    }
}

extern s32 gTownMode;

void Npc_State93(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        if (obj->motionId[0] != 0x423) {
            Actor_SetMotionOnLayer(obj, 0x423, 6, 0, 0);
            obj->step++;
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(obj, 0x3F0, 4, 0, 0);
            obj->step++;
        }
        break;
    case 2:
        if (gTownMode == 0) {
            Actor_SetState(obj, 0, *(u16 *)&w->route[w->routeIdx].state);
        }
        break;
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);

void Npc_State94(Em *obj, u8 flag)
{
    switch (obj->step) {
    case 0:
        if (flag == 0) {
            Actor_SetMotionOnLayer(obj, 0x427, 0, 0, 0);
        } else {
            Actor_SetMotionOnLayer(obj, 0x428, 0, 0, 0);
        }
        obj->step++;
        break;
    }
}

s32 ElderRaid_InState1(void);
s32 LobbyTime_IsPm(void);

void Npc_State96(Em *obj)
{
    s32 r = LobbyTime_IsPm();

    switch (obj->step) {
    case 0:
        obj->timer = 1;
        obj->step++;
        if (ElderRaid_InState1() == 1) {
            if (obj->motionId[0] != 0x424) {
                Actor_SetMotionOnLayer(obj, 0x424, 0x1E, 0, 0);
            }
        } else if (r == 1) {
            if (obj->motionId[0] == 0x3F0 || obj->motionId[0] == 0x3EF) {
                Actor_SetMotionOnLayer(obj, 0x3E9, 0x1E, 0, 0);
                obj->timer = 3;
            } else if (obj->motionId[0] != 0x425) {
                Actor_SetMotionOnLayer(obj, 0x425, 0x1E, 0, 0);
                obj->timer = 3;
            }
        } else {
            if (obj->motionId[0] == 0x3F0 || obj->motionId[0] == 0x3EF) {
                Actor_SetMotionOnLayer(obj, 0x3E9, 0x1E, 0, 0);
            } else if (obj->motionId[0] != 0x3E9) {
                Actor_SetMotionOnLayer(obj, 0x3E9, 4, 0, 0);
            }
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0 && --obj->timer <= 0) {
            if (obj->motionId[0] == 0x425) {
                Actor_SetMotionOnLayer(obj, 0x426, 6, 0, 0);
                obj->step++;
            } else {
                obj->step = 0;
            }
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(obj, 0x3E9, 0x1E, 0, 0);
            obj->timer = 2;
            obj->step = 1;
        }
        break;
    }
}

void Actor_SetMotionOnLayer(void *obj, s32 motion, s32 frames, s32 c, s32 i);
void Npc_StartTurnTo(Em *arg0, s32 arg1, s32 arg2);

void Npc_State9F(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        if (obj->motionId[0] != 0x424) {
            Actor_SetMotionOnLayer(obj, 0x424, 0x1E, 0, 0);
        }
        break;
    }
}

void Npc_State15(Em *obj)
{
    u8 step;

    ((NpcWork *)&obj->work)->unk2 = 1;
    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        Actor_SetMotion(obj, 0x262, 4, 0);
        break;
    }
}

void Npc_StateA3(Em *obj)
{
    u8 step;
    NpcWork *m;

    step = obj->step;
    m = (NpcWork *)&obj->work;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        if (obj->motionId[0] != 0x383) {
            Actor_SetMotion(obj, 0x383, 0xA, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            Actor_SetState(obj, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_StateA4(Em *obj)
{
    u8 step;
    NpcWork *m;

    step = obj->step;
    m = (NpcWork *)&obj->work;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        if (obj->motionId[0] != 0xFF) {
            Actor_SetMotion(obj, 0xFF, 4, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            Actor_SetState(obj, 0, *(u16 *)&m->route[m->routeIdx].state);
        }
        break;
    }
}

void Npc_StateA5(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        obj->stateVar0 = 0;
        if (obj->motionId[0] != 0x100) {
            Actor_SetMotion(obj, 0x100, 0xC, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            Actor_SetMotion(obj, 0x357, 0, 0xB8);
            obj->motionSpeed = 1.8f;
        }
        break;
    case 2:
        break;
    }
}

void Player_SetFlag(void *obj, u32 flag);

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

void Npc_StateAE(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        obj->step++;
        obj->stateVar0 = 0;
        w->motionPhase = 2;
        if (obj->motionId[0] != 0x40B) {
            Actor_SetMotionOnLayer(obj, 0x40B, 4, 0, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            Actor_SetMotionOnLayer(obj, 0x40C, 4, 0, 0);
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            obj->stateVar0++;
            if (obj->stateVar0 >= 2) {
                obj->step++;
                Actor_SetMotionOnLayer(obj, 0x40D, 4, 0, 0);
            }
        } else if (w->motionPhase == 0 && Motion_ReachedFrame(obj, 0, 30.0f) != 0) {
            w->motionPhase = 1;
        }
        break;
    case 3:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            Actor_SetMotionOnLayer(obj, 0x406, 4, 0, 0);
        } else if (w->motionPhase == 1 && Motion_ReachedFrame(obj, 0, 118.0f) != 0) {
            w->motionPhase = 0;
        }
        break;
    case 4:
        break;
    }
}

void Npc_State29(Em *obj)
{
    u8 step;

    step = obj->step;
    switch (step) {
    case 0:
        obj->step = (u8) (step + 1);
        Actor_SetMotion(obj, 0x27B, 6, 0);
        obj->step = (u8) (obj->step + 1);
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step = (u8) (step + 1);
            Actor_SetMotion(obj, 0x261, 6, 0);
            break;
        }
    case 2:
        break;
    case 3:
        if (Motion_ReachedFrame(obj, 0, 267.0f) != 0) {
            Actor_SetMotion(obj, 0x27B, 6, 0xC8);
        }
        break;
    }
}

void Npc_State18(Em *npc)
{
    u8 step;

    step = npc->step;
    switch (step) {
    case 0:
        npc->step = (u8) (step + 1);
        npc->motionTime = 0;
        Actor_SetAction(npc, 0, 0, 0);
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

void Npc_State83(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    w->route = *(NpcRouteEnt **)&w->route[w->routeIdx].arg;
    w->routeIdx = 0;
    Actor_SetState(npc, 0, *(u16 *)&w->route[w->routeIdx].state);
}

void Npc_StateB3(Em *obj)
{
    NpcWork *m;

    m = (NpcWork *)&obj->work;
    if (((s32) (Rand_Next(1) & 0xFFFF) % 10) == 0) {
        m->routeIdx = m->routeIdx + 1;
    }
    m->route = *(NpcRouteEnt **)&m->route[m->routeIdx].arg;
    m->routeIdx = 0;
    Actor_SetState(obj, 0, *(u16 *)&m->route[m->routeIdx].state);
}

void Npc_State35(Em *arg0, s8 arg1);
void Npc_State37(Em *arg0, s8 arg1);
void Npc_State46(Em *arg0, s8 arg1);
void Npc_State3A(Em *arg0, s8 arg1);
void Npc_State48(Em *arg0, s32 arg1);
void Npc_State4A(Em *arg0);
void Npc_State42(Em *arg0);
void Npc_State5B(Em *arg0);
void Npc_State84(Em *arg0);
void Npc_State85(Em *arg0);
void Npc_State86(Em *arg0);
void Npc_State87(Em *arg0);
void Npc_State88(Em *arg0);
void Npc_State89(Em *arg0);
void Npc_State4B(Em *arg0);
void Npc_State4C(Em *arg0, s8 arg1);
void Npc_StateA2(Em *arg0);
void Npc_State0E(Em *arg0);
void Npc_State33(Em *arg0);
void Npc_State40(Em *arg0);
void Npc_State2E(Em *arg0);
void Npc_State2F(Em *arg0);
void Npc_State3E(Em *npc);
void Npc_State7A(Em *arg0);
void Npc_State5F(Em *arg0);
void Npc_State69(Em *obj);
void Npc_State78(Em *arg0);
void Npc_State7B(Em *arg0);
void Npc_State7C(Em *arg0);
void Npc_State77(Em *npc);

/* the NPCs' state handlers, by state (+0x15); 0 for state 0x5F */
s32 Npc_RunState(Em *npc)
{
    switch (npc->state) {
    case 0x1:
        Npc_State01(npc);
        break;
    case 0x3:
        Npc_State03(npc);
        break;
    case 0x4:
        Npc_State04(npc);
        break;
    case 0x35:
        Npc_State35(npc, 0);
        break;
    case 0x36:
        Npc_State35(npc, 1);
        break;
    case 0x43:
        Npc_State43(npc, 0);
        break;
    case 0x44:
        Npc_State43(npc, 1);
        break;
    case 0x45:
        Npc_State43(npc, 2);
        break;
    case 0x37:
        Npc_State37(npc, 0);
        break;
    case 0x38:
        Npc_State37(npc, 1);
        break;
    case 0x39:
        Npc_State37(npc, 3);
        break;
    case 0x2C:
        Npc_State2C(npc);
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
    case 0x41:
        Npc_State41(npc);
        break;
    case 0x48:
        Npc_State48(npc, 0);
        break;
    case 0x49:
        Npc_State48(npc, 1);
        break;
    case 0x14:
        Npc_State14(npc, 0);
        break;
    case 0x25:
        Npc_State14(npc, 1);
        break;
    case 0x29:
        Npc_State29(npc);
        break;
    case 0x18:
        Npc_State18(npc);
        break;
    case 0x4A:
        Npc_State4A(npc);
        break;
    case 0x42:
        Npc_State42(npc);
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
    case 0x5A:
        Npc_State5A(npc);
        break;
    case 0x5B:
        Npc_State5B(npc);
        break;
    case 0x30:
        Npc_State30(npc);
        break;
    case 0x31:
        Npc_State31(npc);
        break;
    case 0xE:
        Npc_State0E(npc);
        break;
    case 0x32:
        Npc_State32(npc);
        break;
    case 0x33:
        Npc_State33(npc);
        break;
    case 0x34:
        Npc_State34(npc);
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
    case 0x3C:
        Npc_State3C(npc);
        break;
    case 0x3D:
        Npc_State3D(npc);
        break;
    case 0x3E:
        Npc_State3E(npc);
        break;
    case 0x7A:
        Npc_State7A(npc);
        break;
    case 0x66:
        Npc_State66(npc);
        break;
    case 0x67:
        Npc_State67(npc);
        break;
    case 0x68:
        Npc_State68(npc, 0);
        break;
    case 0x7F:
        Npc_State68(npc, 1);
        break;
    case 0x69:
        Npc_State69(npc);
        break;
    case 0x6A:
        Npc_State6A(npc);
        break;
    case 0x6B:
        Npc_State6B(npc);
        break;
    case 0x6D:
        Npc_State6D(npc);
        break;
    case 0x6E:
        Npc_State6E(npc);
        break;
    case 0x70:
        Npc_State70(npc);
        break;
    case 0x6F:
        Npc_State6F(npc);
        break;
    case 0x72:
        Npc_State72(npc);
        break;
    case 0x73:
        Npc_State73(npc);
        break;
    case 0x74:
        Npc_State74(npc);
        break;
    case 0x75:
        Npc_State75(npc);
        break;
    case 0x76:
        Npc_State76(npc);
        break;
    case 0x77:
        Npc_State77(npc);
        break;
    case 0x78:
        Npc_State78(npc);
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
    case 0x7E:
        Npc_State7E(npc);
        break;
    case 0x81:
        Npc_State81(npc);
        break;
    case 0x82:
        Npc_State82(npc);
        break;
    case 0x83:
        Npc_State83(npc);
        break;
    case 0x84:
        Npc_State84(npc);
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
    case 0x8B:
        Npc_State43(npc, 3);
        break;
    case 0x8C:
        Npc_State8C(npc);
        break;
    case 0x8E:
        Npc_State8E(npc);
        break;
    case 0x8F:
        Npc_State8F(npc);
        break;
    case 0x92:
        Npc_State92(npc);
        break;
    case 0x93:
        Npc_State93(npc);
        break;
    case 0x94:
        Npc_State94(npc, 0U);
        break;
    case 0x95:
        Npc_State94(npc, 1U);
        break;
    case 0x96:
        Npc_State96(npc);
        break;
    case 0x9F:
        Npc_State9F(npc);
        break;
    case 0x9D:
        Npc_State9D(npc);
        break;
    case 0x15:
        Npc_State15(npc);
        break;
    case 0x5F:
        Npc_State5F(npc);
        return 0;
    case 0xA1:
        Npc_StateA1(npc);
        break;
    case 0xA2:
        Npc_StateA2(npc);
        break;
    case 0xA3:
        Npc_StateA3(npc);
        break;
    case 0xA4:
        Npc_StateA4(npc);
        break;
    case 0xA5:
        Npc_StateA5(npc);
        break;
    case 0xA6:
        Npc_StateA6(npc, 0);
        break;
    case 0xA7:
        Npc_StateA6(npc, 1);
        break;
    case 0xAD:
        Npc_StateAD(npc);
        break;
    case 0xAE:
        Npc_StateAE(npc);
        break;
    case 0xB3:
        Npc_StateB3(npc);
        break;
    case 0x60:
        break;
    }
    return 1;
}

void Npc51_State4E(Em *obj, u8 flag)
{
    NpcWork *w = (NpcWork *)&obj->work;

    w->unk42 = 2;
    switch (obj->step) {
    case 0:
        obj->step++;
        w->unk45 = 1;
        if (obj->motionId[0] != 0x431) {
            Actor_SetMotionOnLayer(obj, 0x431, 4, 0, 0);
        }
        break;
    case 1:
        break;
    }
}

s32 Npc_IsType52(Em *arg0);
extern s8 gTownModeStep;
extern s8 D_008BEB3F;

void Npc51_State01(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        obj->step++;
        w->unk42 = 0;
        w->unk45 = 0;
        obj->timer = (u8)((u16)Rand_Next(1) + 0xFF);
        if (Npc_IsType52(obj) == 1) {
            Actor_SetMotionOnLayer(obj, 0x436, 4, 0, 0);
        } else if (w->id == 0x71 || w->id == 0x49) {
            Actor_SetMotionOnLayer(obj, 0x3E9, -20, 0, 0);
        } else {
            Actor_SetMotionOnLayer(obj, 0x3E9, 20, 0, 0);
        }
        break;
    case 1:
        if (w->id == 0x71) {
            if (gTownMode == 0xF && D_008BEB3F == 0 && gTownModeStep == 2 && w->motionPhase == 0) {
                Actor_SetState(obj, 0, 0x51);
            }
        } else if (Npc_IsType52(obj) == 0) {
            if (Vec_Distance(&obj->posX, (f32 *) gPlayers[gScene.localPlayer].pos) < 130.0f) {
                if (w->motionPhase == 0) {
                    Actor_SetState(obj, 0, 0x50);
                    w->motionPhase = 1;
                    obj->timer = (u8)Rand_Next(1) + 0xFF;
                }
            } else if (--obj->timer <= 0) {
                Actor_SetState(obj, 0, 0x4F);
            } else {
                w->motionPhase = 0;
            }
        }
        break;
    }
}

void ThrownObj_Spawn(void *arg, s32 kind, s32 n);

void Npc51_State02(Em *npc, u8 kind)
{
    NpcWork *w = (NpcWork *)&npc->work;
    NpcRouteEnt *pt;
    f32 d;

    switch (npc->step) {
    case 0:
        npc->step++;
        switch (kind) {
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
            ThrownObj_Spawn(npc, npc->tutorialSlot, (u8)gTutorial.unkC);
            if (npc->motionId[0] != 0x3F8) {
                Actor_SetMotionOnLayer(npc, 0x3F8, 4, 0, 0);
            }
            break;
        }
        break;
    case 1:
        pt = &w->route[w->routeIdx];
        npc->targetYaw = (s16)(u16)Actor_HeadingDiff(npc, pt) / 10;
        npc->rotY += (s16)npc->targetYaw;
        d = Vec_Distance(&npc->posX, pt->pos.v);
        if (d < 0.0f) {
            d *= -1.0f;
        }
        if (d <= 80.0f) {
            Npc_NextRoutePoint(npc);
        }
        break;
    }
}

void Npc51_State4F(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        Actor_SetMotionOnLayer(obj, 0x3EC, 4, 0, 0);
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            Actor_SetMotionOnLayer(obj, 0x3E9, 8, 0, 0);
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            Actor_SetState(obj, 0, 1);
        }
        break;
    }
}

void Npc51_State50(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        obj->targetYaw = (s16)(u16)Actor_HeadingDiff(obj, gPlayers[gScene.localPlayer].pos) / 10;
        obj->timer = 10;
        Actor_SetMotionOnLayer(obj, 0x42F, 8, 0, 0);
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->rotY += (s16)obj->targetYaw;
            obj->step++;
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(obj, 0x3E9, 10, 0, 0);
            obj->timer = 10;
            obj->step++;
        }
        break;
    case 3:
        if (--obj->timer <= 0) {
            obj->rotY -= (s16)obj->targetYaw;
            Actor_SetState(obj, 0, 1);
        }
        break;
    }
}

f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Effect0D_SpawnAt(f32 *pos, s32 id, f32 scale);
void Sound_PlaySeFlat(u32 slot, u32 id, s32 prog);

void Npc51_StateAA(Em *npc)
{
    f32 pos[3];
    f32 out[3];
    s32 ang[3];
    f32 m[16];

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
            ang[0] = 0;
            ang[1] = npc->rotY;
            ang[2] = 0;
            Mtx_FromAnglesXYZ(ang, m);
            pos[0] = 0.0f;
            pos[1] = 0.0f;
            pos[2] = 30.0f;
            Vec_Rotate(out, pos, m);
            pos[0] = npc->posX + out[0];
            pos[1] = 100.0f + npc->posY;
            pos[2] = npc->posZ + out[2];
            Effect0D_SpawnAt(pos, 0x1D, 1.0f);
            Sound_PlaySeFlat(1, 0x9A, 0);
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

void Npc51_State97(Em *npc)
{
    u16 r;

    switch (npc->step) {
    case 0:
        if (npc->variant == 0x55) {
            if (npc->motionId[0] != 0x439) {
                Actor_SetMotionOnLayer(npc, 0x439, 4, 0, 0);
            }
        } else {
            r = (u16)Rand_Next(1) % 3;
            switch (r) {
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
        Player_SetFlag((Player *) npc, 0x20000);
    } else {
        Player_ClearFlag((Player *) npc, 0x20000);
    }
}

void Npc51_State98(Em *obj)
{
    Player *me = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&obj->work;

    w->unk2 = 1;
    switch (obj->step) {
    case 0:
        if (obj->motionId[0] == 0x439) {
            obj->posX = 2698.0f;
            obj->posY = 0.0f;
            obj->posZ = 2930.0f;
            *(NpcPos *)obj->goal = *(NpcPos *)(&obj->posX);
            Npc_NextRoutePoint(obj);
            Actor_SetMotionOnLayer(obj, 0x3F3, 0, 0, 0);
        } else if (obj->motionId[0] != 0x43A) {
            Actor_SetMotionOnLayer(obj, 0x43A, 4, 0, 0);
        }
        obj->step++;
        break;
    case 1:
        if (Vec_Distance((f32 *) me->pos, &obj->posX) < 200.0f) {
            Actor_SetMotionOnLayer(obj, 0x43B, 6, 0, 0);
            obj->step++;
        }
        break;
    case 2:
        if (Motion_ReachedFrame(obj, 0, 240.0f) != 0) {
            w->unk2 = 0;
        }
        if (obj->motion[0].playing <= 0) {
            obj->posX = 2698.0f;
            obj->posY = 0.0f;
            obj->posZ = 2930.0f;
            *(NpcPos *)obj->goal = *(NpcPos *)(&obj->posX);
            obj->rotY = 0;
            obj->targetYaw = obj->rotY;
            Npc_NextRoutePoint(obj);
            Actor_SetMotionOnLayer(obj, 0x3F3, 0, 0, 0);
        }
        break;
    }
}

void Npc51_StateB1(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        Actor_SetMotionOnLayer(obj, 0x40C, 4, 0x8C, 0);
        break;
    case 1:
        break;
    }
}

void Npc51_StateB2(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        Actor_SetMotionOnLayer(obj, 0x432, 4, 0, 0);
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            obj->timer = 100;
            Actor_SetMotionOnLayer(obj, 0x430, 6, 0xC, 0);
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0 || --obj->timer <= 0) {
            obj->step++;
            Npc_NextRoutePoint(obj);
        }
        break;
    }
}

void ThrownObj_Spawn2(void *arg, s32 kind);

void Npc51_State99(Em *npc)
{
    switch (npc->step) {
    case 0:
        npc->step++;
        Actor_SetMotionOnLayer(npc, 0x40C, 0, 0, 0);
        break;
    case 1:
        if (Motion_ReachedFrame(npc, 0, 48.0f) != 0) {
            npc->step++;
            ThrownObj_Spawn2(npc, 0);
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

void VPart_SpawnKind(void *a, void *b, u8 k);
void Npc_SetRouteFromHairChair(Em *arg0);

void Npc51_State9B(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;
    Player *me = &gPlayers[gScene.localPlayer];

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
            if (w->id == 0x70) {
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
            VPart_SpawnKind(me, npc, 1);
        } else if (Motion_PassedFrame((Player *) npc, 0, 120.0f) != 0) {
            VPart_SpawnKind(me, npc, 2);
        }
    }
}

s32 Npc_IsAct51to55(Em *arg0);

void Npc51_State2C(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;
    NpcRouteEnt *e;

    switch (obj->motionId[0]) {
    case 0x431:
        Actor_SetState(obj, 0, *(u16 *)&w->route[w->routeIdx].state);
        break;
    default:
        switch (obj->step) {
        case 0:
            w->unk2 = 1;
            if (*(s8 *)&w->unk45 != 0) {
                obj->timer = 0;
            } else {
                obj->timer = 10;
                obj->targetYaw = Actor_HeadingDiff(obj, gPlayers[gScene.localPlayer].pos);
            }
            obj->step++;
            if (w->id == 0x70) {
                Actor_SetMotionOnLayer(obj, 0x439, 4, 0, 0);
            } else if (Npc_IsType52(obj) == 1) {
                Actor_SetMotionOnLayer(obj, 0x434, 4, 0, 0);
            } else if (obj->motionId[0] == 0x43F) {
                Actor_SetMotionOnLayer(obj, 0x440, 2, 0, 0);
            } else {
                Actor_SetMotionOnLayer(obj, 0x42F, 4, 0, 0);
            }
            break;
        case 1:
            w->unk2 = 1;
            obj->rotY += (s16)obj->targetYaw / 10;
            if (--obj->timer <= 0) {
                obj->step++;
            }
            break;
        case 2:
            w->unk2 = 1;
            if ((obj->motionId[0] == 0x42F || obj->motionId[0] == 0x440) && obj->motion[0].playing <= 0) {
                Actor_SetMotionOnLayer(obj, 0x42E, 4, 0, 0);
            }
            if (gTownMode == 0) {
                obj->timer = 16;
                obj->step++;
                if (Npc_IsAct51to55(obj) == 1 && obj->motionId[0] == 0x42E) {
                    obj->timer = 32;
                    Actor_SetMotionOnLayer(obj, 0x3E9, 16, 0, 0);
                }
            }
            break;
        case 3:
            w->unk2 = 1;
            if (--obj->timer <= 0) {
                if (*(s8 *)&w->unk45 != 0) {
                    obj->timer = 0;
                } else {
                    obj->timer = 10;
                }
                obj->step++;
            }
            break;
        case 4:
            w->unk2 = 1;
            obj->rotY -= (s16)obj->targetYaw / 10;
            if (--obj->timer <= 0) {
                if (w->route != NULL) {
                    e = w->route + w->routeIdx;
                    Actor_SetState(obj, 0, e->state);
                    if (e->arg == -1) {
                        *(NpcPos *)obj->goal = e->pos;
                        Actor_StartMoveTo(obj, 20);
                    }
                } else {
                    Actor_SetState(obj, 0, 1);
                }
            }
            break;
        }
        break;
    }
}

void Npc51_State9E(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        if (obj->motionId[0] != 0x43F) {
            Actor_SetMotionOnLayer(obj, 0x43F, 4, 0, 0);
        }
        break;
    case 1:
        break;
    }
}

void Npc51_State55(Em *obj)
{
    Player *me = &gPlayers[gScene.localPlayer];
    NpcWork *w = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        obj->step++;
        if (obj->motionId[0] == 0x43F) {
            Actor_SetMotionOnLayer(obj, 0x440, 2, 0, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            if (obj->motionId[0] != 0x40E) {
                Actor_SetMotionOnLayer(obj, 0x40E, 0x10, 0, 0);
            }
        }
        break;
    case 2:
        if (w->id == 0x71 && gTownMode == 0x49 && me->state == 1) {
            w->routeIdx = 0;
            Actor_SetState(obj, 0, w->route[w->routeIdx].state);
        }
        break;
    }
}

extern u8 gTownJingleOn;
void Bgm_StopEventFade(Player *p);

void Npc51_StateA8(Em *npc)
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

void Npc_SetKitchenRoute(Em *obj, u8 kind)
{
    NpcWork *w = (NpcWork *)&obj->work;

    if (gSave.unk36E8 != 0 && Game_IsOnline() == 0) {
        w->route = gNpcRouteKitchen;
    } else {
        switch (kind) {
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
    Actor_SetState(obj, 0, w->route[w->routeIdx].state);
}

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

void Npc_PlaceOnRoute(Em *obj, u8 set, u8 i)
{
    NpcWork *w = (NpcWork *)&obj->work;

    switch (set) {
    default:
        w->route = gNpcPlaceRoutes0[i];
        break;
    case 1:
        w->route = gNpcPlaceRoutes1[i];
        break;
    case 2:
        w->route = gNpcPlaceRoutes2[i];
        break;
    case 3:
        w->route = gNpcPlaceRoutes3[i];
        break;
    }
    w->routeIdx = 0;
    if ((w->route)[w->routeIdx].state == 0x81) {
        w->unk2 = 1;
        obj->rotY = (w->route)[w->routeIdx].arg;
        obj->targetYaw = (w->route)[w->routeIdx].arg;
        Npc_NextRoutePoint(obj);
    }
    Actor_SetState(obj, 0, (w->route)[w->routeIdx].state);
    *(NpcPos *)(&obj->posX) = (w->route)->pos;
    *(NpcPos *)obj->goal = (w->route)->pos;
    *(s16 *)&w->moveCount = 0;
}

void Npc51_StateB4(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        if (obj->motionId[0] != 0x3EC) {
            Actor_SetMotionOnLayer(obj, 0x3EC, 4, 0, 0);
        }
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            Actor_SetMotionOnLayer(obj, 0x3E9, 8, 0, 0);
            obj->timer = 60;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            Npc_NextRoutePoint(obj);
        }
        break;
    }
}

void Player_ClearFlag(void *obj, u32 flag);

void Npc51_StateB5(Em *npc)
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
        Player_SetFlag((Player *) npc, 0x20000);
    } else {
        Player_ClearFlag((Player *) npc, 0x20000);
    }
}

void Npc_State5F(Em *arg0);
void Npc51_State2C(Em *arg0);

s32 Npc51_RunState(Em *npc)
{
    NpcWork *w = (NpcWork *)&npc->work;

    switch (npc->state) {
    case 0x59:
    case 0x1:
        Npc51_State01(npc);
        break;
    case 0x97:
        Npc51_State97(npc);
        break;
    case 0x98:
        Npc51_State98(npc);
        break;
    case 0x3:
        Npc51_State02(npc, 0);
        break;
    case 0x2:
        Npc51_State02(npc, 1);
        break;
    case 0x4E:
        Npc51_State4E(npc, 0);
        break;
    case 0x6C:
        Npc51_State4E(npc, 1);
        break;
    case 0x2C:
        Npc51_State2C(npc);
        break;
    case 0x4F:
        Npc51_State4F(npc);
        break;
    case 0x50:
        Npc51_State50(npc);
        break;
    case 0x5A:
        Npc_State5A(npc);
        break;
    case 0x99:
        Npc51_State99(npc);
        break;
    case 0x81:
        Npc_State81(npc);
        break;
    case 0x9B:
        Npc51_State9B(npc);
        break;
    case 0x9E:
        Npc51_State9E(npc);
        break;
    case 0x55:
        Npc51_State55(npc);
        break;
    case 0x4:
        Npc51_State02(npc, 2);
        break;
    case 0xA8:
        Npc51_StateA8(npc);
        break;
    case 0x83:
        Npc_State83(npc);
        break;
    case 0xAA:
        Npc51_StateAA(npc);
        break;
    case 0x5F:
        Npc_State5F(npc);
        return 0;
    case 0xB0:
        Npc51_State02(npc, 3);
        break;
    case 0xB1:
        Npc51_StateB1(npc);
        break;
    case 0xB2:
        Npc51_StateB2(npc);
        break;
    case 0xB4:
        Npc51_StateB4(npc);
        break;
    case 0xB5:
        Npc51_StateB5(npc);
        break;
    case 0xB3:
        Npc_StateB3(npc);
        break;
    }
    if ((w->id == 0x71 || w->id == 0x49) && npc->state != 1 && npc->state != 0x4F && npc->state != 0x50) {
        w->unk2 = 1;
    }
    return 1;
}

void Npc4F_State01(Em *obj, s8 flag)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        if (flag == 0) {
            Actor_SetMotionOnLayer(obj, 0x3E9, 2, 0, 0);
        } else {
            Actor_SetMotionOnLayer(obj, 0x3E9, 8, 0, 0);
        }
        break;
    }
}

void Player_SetFlag(void *obj, u32 flag);

void Npc4F_StateB6(Em *obj)
{
    switch (obj->step) {
    case 0:
        obj->step++;
        Player_SetFlag((Player *) obj, 0x20000);
        if (obj->motionId[0] != 0x432) {
            Actor_SetMotionOnLayer(obj, 0x432, 0xE, 0, 0);
        } else {
            Actor_SetMotionOnLayer(obj, 0x432, 0, 0x58, 0);
        }
        break;
    case 1:
        break;
    }
}

void Npc4F_State52(Em *obj, s8 flag)
{
    NpcWork *w = (NpcWork *)&obj->work;
    Player *me = &gPlayers[gScene.localPlayer];

    switch (obj->step) {
    case 0:
        obj->step++;
        Player_SetFlag((Player *) obj, 0x20000);
        if (flag == 0) {
            Actor_SetMotionOnLayer(obj, 0x432, 0, 0x58, 0);
        } else {
            Actor_SetMotionOnLayer(obj, 0x432, 0xE, 0, 0);
        }
        break;
    case 1:
        if (gTownMode == 0x14) {
            if (me->state != 0xE && me->state != 0xF) {
                Player_ClearFlag((Player *) obj, 0x20000);
                Actor_SetMotionOnLayer(obj, 0x431, 4, 0, 0);
                obj->step++;
            }
        } else if (!(Vec_Distance((f32 *) me->pos, w->unk20.v) <= 130.0f)) {
            Player_ClearFlag((Player *) obj, 0x20000);
            Actor_SetMotionOnLayer(obj, 0x431, 4, 0, 0);
            obj->step++;
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(obj, 0x3E9, 10, 0, 0);
            w->motionPhase = 30;
            obj->step++;
        }
        break;
    case 3:
        if (--w->motionPhase <= 0) {
            Actor_SetState(obj, 0, 0x58);
        }
        break;
    }
}

void Vec_Normalize(f32 *v);

void Npc4F_State2C(Em *obj)
{
    Player *me = &gPlayers[gScene.localPlayer];
    f32 v[3];

    switch (obj->step) {
    case 0:
        obj->motionSpeed = 1.0f;
        obj->step++;
        Actor_SetState(me, 0, 0x20);
        obj->timer = 10;
        obj->targetYaw = Actor_HeadingDiff(obj, (u8 *)(obj)->contact + 0xAC);
        Actor_SetMotionOnLayer(obj, 0x3EA, 2, 0, 0);
        break;
    case 1:
        if (--obj->timer >= 0) {
            obj->rotY += (s16)obj->targetYaw / 10;
        } else {
            obj->step++;
        }
        break;
    case 2:
        if (Vec_Distance(&obj->posX, (f32 *) me->pos) < 78.0f) {
            v[0] = obj->posX - me->pos[0];
            v[1] = obj->posY - me->pos[1];
            v[2] = obj->posZ - me->pos[2];
            Vec_Normalize(v);
            v[0] *= 78.0f;
            v[1] *= 78.0f;
            v[2] *= 78.0f;
            obj->goal[0] = me->pos[0] + v[0];
            obj->goal[1] = me->pos[1] + v[1];
            obj->goal[2] = me->pos[2] + v[2];
            Actor_StartMoveTo(obj, 20);
            Actor_SetMotionOnLayer(obj, 0x3F2, 4, 0, 0);
            obj->step++;
        }
        break;
    case 3:
        obj->rotY += (s16)(u16)Actor_HeadingDiff(obj, me->pos) / 5;
        break;
    }
}

void Npc4F_StateA9(Em *obj)
{
    Player *me = &gPlayers[gScene.localPlayer];

    switch (obj->step) {
    case 0:
        obj->motionSpeed = 1.0f;
        obj->step++;
        obj->timer = 10;
        obj->targetYaw = Actor_HeadingDiff(obj, (u8 *)(obj)->contact + 0xAC);
        Actor_SetMotionOnLayer(obj, 0x3EA, 2, 0, 0);
        break;
    case 1:
        if (--obj->timer >= 0) {
            obj->rotY += (s16)obj->targetYaw / 10;
        } else {
            obj->step++;
        }
        break;
    case 2:
        if (Vec_Distance(&obj->posX, (f32 *) me->pos) < 100.0f) {
            obj->step++;
        }
        break;
    case 3:
        Actor_SetState(obj, 0, 1);
        break;
    }
}

void ModelObj_SpawnOn(void *arg0, s32 arg1);
extern s8 gNpc14Follows;

void Npc4F_State54(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    if (obj->step <= 2) {
        if ((s16)--w->motionPhase <= 0) {
            ModelObj_SpawnOn(obj, 1);
            w->motionPhase = (u16)Rand_Next(1) % 5 + 3;
        }
    }
    switch (obj->step) {
    case 0:
        if (w->id == 0x14) {
            gNpc14Follows = 0;
        }
        w->unk44 = 1;
        obj->step++;
        obj->motionSpeed = 3.0f;
        Actor_SetMotionOnLayer(obj, 0x3F6, 4, 0x26, 0);
        obj->timer = 20;
        break;
    case 1:
        if (--obj->timer <= 0) {
            obj->step++;
            obj->timer = 5;
            obj->motionSpeed = 1.0f;
            Actor_SetMotionOnLayer(obj, 0x3FA, 4, 0, 0);
            Actor_SetState(&gPlayers[gScene.localPlayer], 0, 0x1F);
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            Actor_SetMotionOnLayer(obj, 0x3E9, 4, 0, 0);
            obj->step++;
        }
        break;
    case 3:
        if (gPlayers[gScene.localPlayer].state == 1) {
            Actor_SetState(obj, 0, 0x56);
        }
        break;
    }
}

void ModelObj_SpawnOn(void *arg0, s32 arg1);

void Npc4F_State55(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        w->unk44 = 2;
        obj->step++;
        Actor_SetMotionOnLayer(obj, 0x3F3, 4, 0, 0);
        ModelObj_SpawnOn(obj, 0);
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            w->unk44 = 0;
            Actor_SetMotionOnLayer(obj, 0x3EF, 6, 0, 0);
            obj->step++;
            obj->timer = 100;
        }
        break;
    case 2:
        if (--obj->timer <= 0) {
            Actor_SetMotionOnLayer(obj, 0x3E9, 0x18, 0, 0);
            obj->step++;
        }
        break;
    case 3:
        if (obj->motion[0].playing <= 0) {
            Actor_SetState(obj, 0, 0x58);
        }
        break;
    }
}

void Npc4F_State5E(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    switch (obj->step) {
    case 0:
        w->unk44 = 2;
        obj->step++;
        Actor_SetMotionOnLayer(obj, 0x433, 4, 0, 0);
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            w->unk44 = 0;
            Actor_SetMotionOnLayer(obj, 0x3E9, 2, 0, 0);
            obj->step++;
        }
        break;
    case 2:
        Actor_SetState(obj, 0, 0x58);
        break;
    }
}

u8 *ActSpot_GetList(u16 area);
s32 Area_IsOnlineHouse(s32 arg0);
/* Pos_InArea returns 0 or 1 as a u8: Npc4F_State56 (and so its unit, it
   seems) sees that type, Npc4F_State58 and Npc4F_State5D compare the result
   unmasked, as an int; hence s32 here and the (u8) casts. */
s32 Pos_InArea(void *obj, f32 *p);
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

void Npc4F_State56(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;
    u8 *spot = ActSpot_GetList(gScene.area);
    NpcRouteEnt *r;
    f32 p[3];

    if (gScene.area == 0x89) {
        obj->motionSpeed *= 2.0f;
        if ((u8)Pos_InArea(&obj->posX, gNpc4FArea0) == 1) {
            r = gNpc4FRoute0;
        } else if ((u8)Pos_InArea(&obj->posX, gNpc4FArea2) == 1) {
            r = gNpc4FRoute2;
        } else if ((u8)Pos_InArea(&obj->posX, gNpc4FArea1) == 1) {
            r = gNpc4FRoute1;
        } else if ((u8)Pos_InArea(&obj->posX, gNpc4FArea3) == 1) {
            r = gNpc4FRoute3;
        } else if ((u8)Pos_InArea(&obj->posX, gNpc4FArea4) == 1) {
            r = gNpc4FRoute4;
        } else {
            r = gNpc4FRouteElse;
        }
        w->route = r;
        w->routeIdx = 0;
        Actor_SetState(obj, 0, r->state);
        return;
    }
    p[0] = *(f32 *)(spot + 4);
    p[1] = *(f32 *)(spot + 8);
    p[2] = *(f32 *)(spot + 0xC);
    switch (obj->step) {
    case 0:
        w->unk44 = 1;
        obj->step++;
        obj->timer = 10;
        obj->targetYaw = Actor_HeadingDiff(obj, p);
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
            w->unk44 = 0;
            obj->step++;
            Actor_SetState(obj, 0, 0x60);
        }
        break;
    case 3:
        break;
    }
}

/* An identity helper: through it the entry address is added index first, as
   the game does after stepping the route (Npc4F_StateAF). */
static inline s32 Npc_RouteAddr(NpcRouteEnt *route)
{
    return (s32)route;
}

void Npc4F_StateAF(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;
    NpcRouteEnt *pt;
    NpcRouteEnt *e;
    f32 d;

    switch (obj->step) {
    case 0:
        obj->step++;
        if (obj->motionId[0] != 0x3F7) {
            Actor_SetMotionOnLayer(obj, 0x3F7, 2, 0, 0);
        }
        break;
    case 1:
        pt = w->route + w->routeIdx;
        obj->targetYaw = (s16)(u16)Actor_HeadingDiff(obj, pt) / 5;
        obj->rotY += (s16)obj->targetYaw;
        d = Vec_Distance(&obj->posX, pt->pos.v);
        if (d < 0.0f) {
            d *= -1.0f;
        }
        if (d <= 30.0f) {
            (w->routeIdx)++;
            e = (NpcRouteEnt *)(Npc_RouteAddr(w->route) + w->routeIdx * 0x14);
            if (e->state == 0) {
                w->routeIdx = 0;
            } else if (obj->state != e->state) {
                obj->step++;
                Actor_SetState(obj, 0, e->state);
                w->wait = 0;
            } else {
                obj->step = 0;
            }
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            e = w->route + w->routeIdx;
            if (obj->state != e->state) {
                obj->step++;
                Actor_SetState(obj, 0, e->state);
                w->wait = 0;
            } else {
                obj->step = 0;
            }
        }
        break;
    }
}

extern u8 gHouseAccess;

void Npc4F_State57(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;
    NpcRouteEnt *pt;
    NpcRouteEnt *e;
    f32 d;

    if (gHouseAccess != 0) {
        Actor_SetState(obj, 0, 0xB6);
        return;
    }
    switch (obj->step) {
    case 0:
        obj->step++;
        if (obj->motionId[0] == 0x3EF) {
            Actor_SetMotionOnLayer(obj, 0x3EB, 8, 0, 0);
        } else {
            Actor_SetMotionOnLayer(obj, 0x3EB, 4, 0, 0);
        }
        break;
    case 1:
        pt = w->route + w->routeIdx;
        obj->targetYaw = (s16)(u16)Actor_HeadingDiff(obj, pt) / 10;
        obj->rotY += (s16)obj->targetYaw;
        d = Vec_Distance(&obj->posX, pt->pos.v);
        if (d < 0.0f) {
            d *= -1.0f;
        }
        if (d <= 30.0f) {
            (w->routeIdx)++;
            e = (NpcRouteEnt *)(Npc_RouteAddr(w->route) + w->routeIdx * 0x14);
            if (e->state == 0) {
                w->routeIdx = 0;
            } else if ((u16)Rand_Next(1) & 1) {
                obj->step++;
                Actor_SetMotionOnLayer(obj, 0x3EF, 10, 0, 0);
            } else {
                e = w->route + w->routeIdx;
                if (obj->state != e->state) {
                    obj->step++;
                    Actor_SetState(obj, 0, e->state);
                    w->wait = 0;
                } else {
                    obj->step = 0;
                }
            }
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            e = w->route + w->routeIdx;
            if (obj->state != e->state) {
                obj->step++;
                Actor_SetState(obj, 0, e->state);
                w->wait = 0;
            } else {
                obj->step = 0;
            }
        }
        break;
    }
}

s32 Npc_IsRising(Em *obj);
void Em_CollideField(Em *em, u8 keep);
extern f32 gNpc4FArea[];

/* an NPC walking with the local player (w: the NPC's work at +0x438):
   steps 0 pick a spot near the player, 1 walk there (or across the
   village), 2/3 end; Pos_InArea/Npc_IsRising test where it is */
void Npc4F_State58(Em *obj)
{
    Player *me;
    NpcWork *w;
    u8 *save;
    f32 pos[3];
    f32 prev[3];
    u32 r;
    f32 d;

    w = (NpcWork *)&obj->work;
    me = &gPlayers[gScene.localPlayer];
    save = (u8 *)&gSave;
    if (gHouseAccess != 0) {
        Actor_SetState(obj, 0, 0xB6);
        return;
    }
    obj->unk3EB = 0;
    if (Game_IsOnline() == 0 && (s8)save[0x2C98] == 0 && w->id != 0x6D) {
        Actor_SetState(obj, 0, 0x5C);
        return;
    }
    if (w->id == 0x14) {
        if (Pos_InArea(&obj->posX, gNpc4FArea) == 1) {
            if (Npc_IsRising((Em *)me) == 1) {
                obj->unk3EB = 1;
            } else if (!(me->pos[1] <= obj->posY)) {
                obj->unk3EB = 1;
            }
        }
        if (obj->unk3EB == 1) {
            Actor_SetState(obj, 0, 0x5D);
            return;
        }
    }
    if (w->id == 0x6D && gScene.area == 0x56) {
        obj->unk3EB = 1;
        Actor_SetState(obj, 0, 0x5D);
        return;
    }
    switch (obj->step) {
    case 0:
        w->unk42 = 0;
        obj->fieldCollide = 1;
        obj->step++;
        if (obj->motionId[0] != 0x3EB) {
            Actor_SetMotionOnLayer(obj, 0x3EB, 4, 0, 0);
        }
        *(NpcPos *)pos = *(NpcPos *)me->pos;
        r = (u16)((u16)Rand_Next(1) % 100);
        if (r & 1) {
            pos[0] += r;
        } else {
            pos[0] -= r;
        }
        r = (u16)((u16)Rand_Next(1) % 100);
        if (r & 1) {
            pos[2] += r;
        } else {
            pos[2] -= r;
        }
        w->unk20 = *(NpcPos *)pos;
        break;
    case 1:
        if (Game_IsOnline() == 0) {
            if (gScene.area == 0x89 || gScene.area == 0x57) {
                obj->motionSpeed = 2.5f;
            } else {
                obj->motionSpeed = 1.0f;
            }
        }
        obj->targetYaw = Actor_HeadingDiff(obj, w->unk20.v);
        obj->rotY += (s16)obj->targetYaw / 10;
        d = Vec_Distance(&obj->posX, w->unk20.v);
        if (d < 15.0f) {
            w->unk5C = 0;
        }
        if (d < 5000.0f && w->unk5C != 0) {
            break;
        }
        if (!(d <= 5000.0f)) {
            if (!(me->pos[0] <= 2500.0f)) {
                w->unk20.v[0] = 10959.0f;
                w->unk20.v[1] = me->pos[1];
                w->unk20.v[2] = 14487.0f;
                w->unk5C = 200;
                obj->posX = 11338.0f;
                obj->posY = me->pos[1];
                obj->posZ = 14352.0f;
                obj->rotY = 0;
                obj->targetYaw = Actor_HeadingDiff(obj, w->unk20.v);
                obj->rotY = obj->targetYaw;
            } else {
                w->unk20.v[0] = 2070.0f;
                w->unk20.v[1] = me->pos[1];
                w->unk20.v[2] = 1100.0f;
                w->unk5C = 200;
                obj->posX = 2540.0f;
                obj->posY = me->pos[1];
                obj->posZ = 961.0f;
                obj->rotY = 0;
                obj->targetYaw = Actor_HeadingDiff(obj, w->unk20.v);
                obj->rotY = obj->targetYaw;
            }
        } else if (me->state == 0xE || me->state == 0xF) {
            if (d < 65.0f) {
                obj->step++;
                Actor_SetMotionOnLayer(obj, 0x3E9, 4, 0, 0);
            } else if (obj->motionId[0] != 0x3EB) {
                Actor_SetMotionOnLayer(obj, 0x3EB, 2, 0, 0);
            }
        } else if (d < 130.0f) {
            if (!(Vec_Distance(w->unk20.v, (f32 *) me->pos) <= 130.0f)) {
                obj->step--;
            } else {
                if (obj->motionId[0] != 0x3E9) {
                    Actor_SetMotionOnLayer(obj, 0x3E9, 4, 0, 0);
                }
                if (w->motionPhase != 0 && w->unk5C == 0) {
                    if (--w->motionPhase <= 0) {
                        obj->motionSpeed = 1.0f;
                        Actor_SetState(obj, 0, 0x53);
                    }
                } else {
                    w->motionPhase = ((u16)Rand_Next(1) % 5 + 3) * 15;
                }
            }
        } else if (obj->motionId[0] != 0x3EB) {
            Actor_SetMotionOnLayer(obj, 0x3EB, 2, 0, 0);
        }
        break;
    case 2:
        if (obj->motion[0].playing <= 0) {
            obj->step++;
            Actor_SetMotionOnLayer(obj, 0x3EF, 10, 0, 0);
        }
        break;
    case 3:
        if (obj->motion[0].playing <= 0) {
            Actor_SetState(obj, 0, 0x53);
        }
        break;
    }
    prev[0] = obj->posX;
    prev[2] = obj->posZ;
    prev[1] = obj->posY;
    if (w->unk5C != 0) {
        (w->unk5C)--;
    }
    if (w->unk5C == 0 || Game_IsOnline() == 1) {
        Em_CollideField(obj, 0);
    }
    d = Vec_Distance(&obj->posX, (f32 *) prev);
    if (obj->wallHitDirs != 0 && !(d <= 1.0f)) {
        (w->unk42)++;
    } else {
        w->unk42 = 0;
    }
    if (w->unk42 > 0x78) {
        w->unk42 = 0;
        obj->motionSpeed = 1.0f;
        Actor_SetState(obj, 0, 0x53);
    }
}

u8 Slab_Contains(f32 *pos, f32 *a, f32 *b, f32 height, f32 radius);

s32 Pos_InArea(void *obj, f32 *p)
{
    f32 a[3];
    f32 b[3];

    a[0] = p[0];
    a[1] = -600.0f;
    a[2] = p[1];
    b[0] = p[2];
    b[1] = -600.0f;
    b[2] = p[3];
    return Slab_Contains(obj, a, b, 1500.0f, p[4]);
}

s32 Npc_IsRising(Em *obj)
{
    return obj->posY - obj->prevPosY > 0.0f;
}

extern u8 gPoogieFollowing;

void Npc4F_State5C(Em *obj, Em *other)
{
    NpcWork *w = (NpcWork *)&obj->work;
    NpcPos p;
    u16 r;
    f32 d;

    if (other == NULL) {
        Actor_SetState(obj, 0, 1);
        return;
    }
    if (other->state == 0x2C) {
        obj->motionSpeed = 1.0f;
        Actor_SetState(obj, 0, 0x5D);
        return;
    }
    switch (obj->step) {
    case 0:
        obj->motionSpeed = 0.5f;
        obj->fieldCollide = 0;
        obj->step++;
        if (obj->motionId[0] != 0x3EB) {
            Actor_SetMotionOnLayer(obj, 0x3EB, 4, 0, 0);
        }
        p = *(NpcPos *)&other->posX;
        r = (u16)Rand_Next(1) % 100;
        if (r & 1) {
            p.v[0] += r;
        } else {
            p.v[0] -= r;
        }
        r = (u16)Rand_Next(1) % 100;
        if (r & 1) {
            p.v[2] += r;
        } else {
            p.v[2] -= r;
        }
        w->unk20 = p;
        break;
    case 1:
        gPoogieFollowing = 1;
        obj->targetYaw = Actor_HeadingDiff(obj, w->unk20.v);
        obj->rotY += (s16)obj->targetYaw / 10;
        d = Vec_Distance(&obj->posX, w->unk20.v);
        if (!(d <= 150.0f)) {
            w->unk2 = 1;
            obj->motionSpeed += 0.7f;
            if (!(obj->motionSpeed <= 1.2f)) {
                obj->motionSpeed = 1.2f;
            }
        } else {
            w->unk2 = 0;
            obj->motionSpeed -= 0.7f;
            if (obj->motionSpeed < 0.5f) {
                obj->motionSpeed = 0.5f;
            }
        }
        if (!(d <= 5000.0f)) {
            obj->posX = 150.0f + other->posX;
            obj->posY = other->posY;
            obj->posZ = other->posZ;
            obj->rotY = obj->targetYaw;
        } else if (d < 65.0f && !(Vec_Distance(w->unk20.v, (f32 *) &other->posX) <= 65.0f)) {
            obj->step--;
        }
        break;
    }
}

void Npc_PickIdleMotion(Em *obj)
{
    u16 r = (u16)Rand_Next(1) & 0x63;

    obj->step++;
    if (r == 0 && obj->stateVar1 == 2) {
        obj->stateVar0 = 2;
        obj->step = 1;
        return;
    }
    switch (r) {
    case 0:
        obj->stateVar0 = 0;
        obj->stateVar1 = 0;
        Actor_SetMotionOnLayer(obj, 0x432, 0xE, 0, 0);
        break;
    case 1:
        Actor_SetMotionOnLayer(obj, 0x3F1, 10, 0, 0);
        obj->stateVar0 = 1;
        obj->stateVar1 = 0;
        break;
    case 2:
        Actor_SetMotionOnLayer(obj, 0x3EE, 10, 0, 0);
        obj->stateVar0 = 1;
        obj->stateVar1 = 0;
        break;
    case 3:
        Actor_SetMotionOnLayer(obj, 0x3FC, 10, 0, 0);
        obj->stateVar0 = 1;
        obj->stateVar1 = 0;
        break;
    default:
        obj->stateVar0 = 2;
        obj->step = 1;
        break;
    }
}

extern f32 gNpc4FArea[];

void Npc4F_State5D(Em *obj, Em *other)
{
    NpcWork *w = (NpcWork *)&obj->work;
    Player *me = &gPlayers[gScene.localPlayer];
    u8 *save = (u8 *)&gSave;

    if (obj->unk3EB != 1) {
        if (other == NULL) {
            Actor_SetState(obj, 0, 1);
            return;
        } else if (other->state != 0x2C) {
            if (obj->step == 1) {
                if (*(s8 *)(save + 0x2C98) != 0) {
                    Actor_SetState(obj, 0, 0x58);
                    if (gScene.area == 0xAF) {
                        w->unk44 = 0;
                    }
                } else {
                    Actor_SetState(obj, 0, 0x5C);
                }
                return;
            }
            if (obj->stateVar0 == 0 && obj->stateVar1 == 1) {
                obj->stateVar1++;
            }
        }
    } else {
        if (obj->motion[0].playing <= 0) {
            if (w->id == 0x6D) {
                if (gScene.area == 0x57) {
                    Actor_SetState(obj, 0, 0x58);
                    return;
                }
            } else if (Pos_InArea(&obj->posX, gNpc4FArea) == 1) {
                if (Npc_IsRising((Em *)me) == 0 && me->pos[1] <= obj->posY) {
                    Actor_SetState(obj, 0, 0x58);
                    return;
                }
            } else {
                Actor_SetState(obj, 0, 0x58);
                return;
            }
        }
    }
    switch (obj->step) {
    case 0:
        w->unk42 = 0;
        obj->step++;
        obj->stateVar0 = 0;
        obj->stateVar1 = 0;
        break;
    case 1:
        switch (obj->stateVar0) {
        case 0:
            w->unk42 += 1;
            if (w->unk42 == 30) {
                obj->motionSpeed = 1.0f;
                Actor_SetMotionOnLayer(obj, 0x3EF, 10, 0, 0);
                obj->stateVar0++;
            }
            break;
        case 1:
            obj->rotY += (s16)(u16)Actor_HeadingDiff(obj, me->pos) / 10;
            if (obj->motion[0].playing <= 0) {
                obj->step++;
            }
            break;
        case 2:
            Npc_PickIdleMotion(obj);
            break;
        }
        break;
    case 2:
        switch (obj->stateVar0) {
        case 0:
            switch (obj->stateVar1) {
            case 0:
                obj->stateVar1++;
                w->unk42 = (u8)Rand_Next(1) + 100;
                Actor_SetMotionOnLayer(obj, 0x432, 14, 0, 0);
                break;
            case 1:
                if (--w->unk42 <= 0) {
                    obj->stateVar1++;
                }
                break;
            case 2:
                Actor_SetMotionOnLayer(obj, 0x431, 4, 0, 0);
                obj->stateVar0++;
                break;
            }
            break;
        case 1:
            if (obj->motion[0].playing <= 0) {
                obj->step = 1;
                obj->stateVar0 = 2;
            }
            break;
        }
        break;
    }
}

void Npc4F_StateAB(Em *obj)
{
    switch (obj->step) {
    case 0:
        Actor_SetMotionOnLayer(obj, 0x3EF, 10, 0, 0);
        obj->step++;
        break;
    case 1:
        if (obj->motion[0].playing <= 0) {
            obj->step = 0;
        }
        break;
    }
}

void Npc_ResumeRoute(Em *arg0);
s32 Npc_IsNearPlayer(Em *obj);
u8 *AreaSpot_FindFree(u16 area, u16 id);
extern u8 *gPoogieDigSpots;
extern u8 gPoogieDigSpotCount;
extern s16 gPoogieDigTimer;
extern u8 gPoogieDigSpotIdx;
extern u8 gPoogieDigState;
extern u8 gPoogieDigTaken;

void Npc4F_StateAC(Em *obj)
{
    Player *me = &gPlayers[gScene.localPlayer];
    u8 *list;
    NpcWork *w = (NpcWork *)&obj->work;
    f32 *dst;

    if (gPoogieDigTimer++ >= 1800) {
        if (obj->motion[0].playing <= 0) {
            Npc_ResumeRoute(obj);
        }
        w->unk44 = 0;
        return;
    }
    w->unk44 = 3;
    list = AreaSpot_FindFree(me->area, 0xD);
    if (list == NULL) {
        if (obj->motion[0].playing <= 0) {
            Npc_ResumeRoute(obj);
        }
        w->unk44 = 0;
        return;
    }
    dst = w->route->pos.v; /* the first route point */;
    switch (obj->stateVar0) {
    case 0:
        if (gPoogieDigState != 2) {
            if (gPoogieDigSpotCount < 2) {
                gPoogieDigSpotIdx = 0;
            } else {
                gPoogieDigSpotIdx = (u16)Rand_Next(1) % gPoogieDigSpotCount;
            }
        }
        list += gPoogieDigSpots[gPoogieDigSpotIdx] * 0x18;
        dst[0] = *(f32 *)(list + 4);
        dst[1] = *(f32 *)(list + 8);
        dst[2] = *(f32 *)(list + 0xC);
        gPoogieDigState = 1;
        obj->motionSpeed *= 2.0f;
        Actor_SetMotionOnLayer(obj, 0x3F7, 2, 0, 0);
        obj->stateVar0++;
        break;
    case 1:
        if (gPoogieDigTaken == 1) {
            Npc_ResumeRoute(obj);
            obj->motionSpeed = 1.0f;
            w->unk44 = 0;
            gPoogieDigState = 2;
            obj->stateVar0 = 3;
        }
        obj->targetYaw = (s16)(u16)Actor_HeadingDiff(obj, dst) / 10;
        obj->rotY += (s16)obj->targetYaw;
        if (Vec_Distance(&obj->posX, (f32 *) dst) < 100.0f) {
            obj->motionSpeed = 1.0f;
            Actor_SetMotionOnLayer(obj, 0x3EF, 10, 0, 0);
            obj->stateVar0++;
        }
        break;
    case 2:
        if (gPoogieDigTaken == 1 || Npc_IsNearPlayer(obj) == 1) {
            Npc_ResumeRoute(obj);
            w->unk44 = 0;
            gPoogieDigState = 2;
            obj->stateVar0++;
        }
        break;
    case 3:
        break;
    }
}

f32 Vec_Length(f32 *v);

s32 Npc_IsNearPlayer(Em *obj)
{
    f32 v[3];
    Player *me = &gPlayers[gScene.localPlayer];

    v[0] = obj->posX - me->pos[0];
    v[1] = 0.0f;
    v[2] = obj->posZ - me->pos[2];
    return Vec_Length(v) <= 100.0f;
}

void Npc_ResumeRoute(Em *obj)
{
    NpcWork *w = (NpcWork *)&obj->work;

    if (obj->work == 0x14) {
        Actor_SetState(obj, 0, 0x58);
    } else if (obj->work == 0x22) {
        Actor_SetState(obj, 0, *(u16 *)&w->route[w->routeIdx].state);
    }
}

Em *Npc_FindByKind(u8 id)
{
    Em *p = gEms;
    u32 i;

    for (i = 0; i < 20; i++, p++) {
        if (p->active != 0 && p->work == id) {
            return p;
        }
    }
    return NULL;
}

u8 *ActSpot_GetList(u16 area);

u8 *AreaSpot_FindFree(u16 area, u16 id)
{
    u8 *list = ActSpot_GetList(area);
    u8 i;
    u16 *p;

    i = 0;
    p = (u16 *)list;
    do {
        if (p[0] == 0 && id == p[1]) {
            return list + i * 0x18;
        }
        i++;
        p += 12;
    } while (i < 40);
    return NULL;
}

void Npc4F_State56(Em *arg0);
void Npc4F_StateAF(Em *arg0);
void Npc4F_State57(Em *arg0);
void Npc4F_State58(Em *arg0);
void Npc4F_State5C(Em *arg0, Em *npc);
void Npc4F_State5D(Em *arg0, Em *npc);
void Npc4F_StateAC(Em *arg0);
extern u8 gPoogieFollowing;

s32 Npc4F_RunState(Em *obj)
{
    Em *npc = Npc_FindByKind(0x12);
    NpcWork *w = (NpcWork *)&obj->work;
    u8 *save = (u8 *)&gSave;

    switch (obj->state) {
    case 0x1:
        Npc4F_State01(obj, 0);
        break;
    case 0x59:
        Npc4F_State01(obj, 1);
        break;
    case 0x52:
        Npc4F_State52(obj, 0);
        break;
    case 0x53:
        Npc4F_State52(obj, 1);
        break;
    case 0xB6:
        Npc4F_StateB6(obj);
        break;
    case 0x2C:
        Npc4F_State2C(obj);
        break;
    case 0xA9:
        Npc4F_StateA9(obj);
        break;
    case 0x54:
        Npc4F_State54(obj);
        break;
    case 0x55:
        Npc4F_State55(obj);
        break;
    case 0x5E:
        Npc4F_State5E(obj);
        break;
    case 0x56:
        Npc4F_State56(obj);
        break;
    case 0x57:
        Npc4F_State57(obj);
        break;
    case 0x58:
        Npc4F_State58(obj);
        break;
    case 0x5C:
        Npc4F_State5C(obj, npc);
        break;
    case 0x5D:
        Npc4F_State5D(obj, npc);
        break;
    case 0xAB:
        Npc4F_StateAB(obj);
        break;
    case 0xAC:
        Npc4F_StateAC(obj);
        break;
    case 0xAF:
        Npc4F_StateAF(obj);
        break;
    case 0x60:
        break;
    }
    if (gScene.area == 0x56 || gScene.area == 0x57) {
        obj->area = gScene.area;
    }
    if (w->unk44 != 0 || (*(s8 *)(save + 0x2C98) == 0 && gPoogieFollowing != 1)) {
        w->unk2 = 1;
    }
    return 1;
}

/* The 0x7873F0 part's .data (0x7FE4B0-0x7FE850) */
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
