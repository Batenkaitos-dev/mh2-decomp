/* Functions matched automatically from m2c's output;
 * the others are still assembly. First-pass C, to be named and typed. */
#include "common.h"
#include "player.h"
#include "m2c_macros.h"
#include "em.h"
#include "item.h"
#include "scene.h"
#include "lobby_state.h"
#include "save.h"
#include "pad.h"
#include "lobby_globals.h"
#include "objhead.h"
#include "lobby_npc.h"

/* Built at optimization level 4: TownPc_UpdateAll matches only at 4, the others at 4 too
 * (TownPc_State20 too). One unit: the functions share the helpers
 * Actor_SetIdle, Actor_SetAction and Actor_SetState throughout. */
#pragma optimization_level 4
s32 Game_IsOnline(void);
s32 TownPc_IsFree(Player *);
u16 Camera_GetYaw(void);
int abs(int);

typedef struct Vec3f {
    f32 x, y, z;
} Vec3f;

/* No prototype: Actor_NeedsStateSend calls it with its loop counter still in a1. */
/* unit's own declaration: Area_HasTownPcs ignores a1, which holds var_a1 at Actor_NeedsStateSend (the player loop count, 10) */
s32 Area_HasTownPcs();
void TownPc_ReadPad(s32 pl);
extern LobbyMenu *gLobbyMenu;
void Actor_SetStateEx(Player *obj, u8 state, u8 sub, u16 flags);

void TownPc_UpdateInput(void) {
    if (gLobbyMenu->redraw == 0) {
        TownPc_ReadPad(gScene.localPlayer);
    }
}

u32 Player_TestFlag(void *obj, u32 flag);

s32 Hud_InputBlocked(void);
s32 Hud_TownMenuOpen(void);
void func_3B0520(Player *p);

void TownPc_ReadPad(s32 pl)
{
    Player *player = &gPlayers[pl];
    PlayerInput *in = &player->input;

    in->stickDir = gPadInput.stickDir[0].cur;
    in->prevStickDir = gPadInput.stickDir[0].prev;
    in->prevStickDirPressed = in->stickDirPressed;
    in->stickDirPressed = in->stickDir & ~in->prevStickDir;
    in->stickAngle = gPadInput.stickAngle[0].cur;
    in->prevStickAngle = gPadInput.stickAngle[0].prev;
    if (Hud_TownMenuOpen() == 0) {
        in->stickTilt = gPadInput.stickTilt[0].cur;
        in->prevStickTilt = gPadInput.stickTilt[0].prev;
    } else {
        in->stickTilt = 0;
        in->prevStickTilt = 0;
    }
    if (Hud_InputBlocked() == 0 && player->supplyBoxOpen == 0) {
        in->held = gPadInput.buttons[0].cur;
        in->prevHeld = gPadInput.buttons[0].prev;
        in->prevPressed = in->pressed;
        in->pressed = in->held & ~in->prevHeld;
        in->changed = (in->held ^ in->prevHeld) | (in->held ^ gPadInput.buttons[0].cur);
        in->held2 = in->held;
        in->prevHeld2 = in->prevHeld;
        in->pressed2 = in->pressed;
        in->prevPressed2 = in->prevPressed;
        if (player->itemSelect != 0) {
            in->held = 0;
            in->prevHeld = 0;
            in->pressed = 0;
            in->prevPressed = 0;
        }
        func_3B0520(player);
    } else {
        if (player->supplyBoxOpen == 0) {
            in->stickDir &= 0x3C00;
            in->prevStickDir &= 0x3C00;
            in->prevStickDirPressed &= 0x3C00;
            in->stickDirPressed &= 0x3C00;
        } else {
            in->stickDir = 0;
            in->prevStickDir = 0;
            in->prevStickDirPressed = 0;
            in->stickDirPressed = 0;
        }
        in->prevStickAngle = 0;
        in->prevStickTilt = 0;
        in->held = 0;
        in->prevHeld = 0;
        in->pressed = 0;
        in->prevPressed = 0;
        in->held2 = 0;
        in->prevHeld2 = 0;
        in->pressed2 = 0;
        in->prevPressed2 = 0;
        in->changed = 0;
    }
}

f32 *Mtx_FromAnglesXYZ(s32 *angles, f32 *m);
void Player_BeginLayerMotion(Player *p, s32 frame, s32 n, s32 part);
s32 Player_UpdateMotion(Player *p);

void Actor_StartMotions(Player *pl)
{
    Mtx_FromAnglesXYZ(&pl->rot[0], &pl->rotMtx[0]);
    if (pl->motionStarted[0] == 0) {
        pl->motionStarted[0]++;
        Player_BeginLayerMotion(pl, (u16)pl->motionFrame[0], pl->motionBlend[0], 0);
    }
    if (pl->motionStarted[1] == 0) {
        pl->motionStarted[1]++;
        Player_BeginLayerMotion(pl, (u16)pl->motionFrame[1], pl->motionBlend[1], 1);
    }
    Player_UpdateMotion(pl);
}

void TownPc_TickTimers(Player *p)
{
    s16 temp_v1_3;
    s8 fishTimer;
    u8 temp_v1;
    u8 temp_v1_2;
    u8 offerNoticeTimer;
    u8 weaponSeMute;

    if (p->hitStop == 0) {
        if (Player_TestFlag((Player *) p, 0x400001U) != 0) {
            p->unk3A0 = (u16) (p->unk3A0 + 2);
        } else {
            p->unk3A0 = 0U;
        }
        p->motionTime = (s32) (p->motionTime + 2);
        if ((s32)p->motionTime >= 0x186A0) {
            p->motionTime = 0x2710;
        }
        temp_v1 = p->unk434;
        if (temp_v1 != 0) {
            p->unk434 = (u8) (temp_v1 - 1);
        }
        temp_v1_2 = p->hideShadow;
        if (temp_v1_2 != 0) {
            p->hideShadow = (u8) (temp_v1_2 - 1);
        }
        temp_v1_3 = p->dashDelay;
        if (temp_v1_3 != 0) {
            p->dashDelay = (s16) (temp_v1_3 - 1);
        }
        offerNoticeTimer = p->offerNoticeTimer;
        if (offerNoticeTimer != 0) {
            p->offerNoticeTimer = (u8) (offerNoticeTimer - 1);
        }
        weaponSeMute = p->weaponSeMute;
        if (weaponSeMute != 0) {
            p->weaponSeMute = (u8) (weaponSeMute - 1);
        }
        if (p->isEm == 0) {
            fishTimer = p->fishTimer;
            if (fishTimer != 0) {
                p->fishTimer = (s8) (fishTimer - 1);
            }
        }
    }
}

void Actor_SetStateNow(Player *pl, u8 state, u8 sub, u16 flags)
{
    Actor_SetStateEx(pl, state, sub, flags);
    pl->stateRequest = 1;
}

u16 Rand_Next(s32 stream);
s32 Actor_NeedsStateSend(void *arg0);
void Actor_SetIdle(void *obj, s32 a1, s32 n, s32 m);
static void Actor_RestartMotionTime(void *obj, s16 flag);
s16 Actor_IsState(void *arg0, s32 arg1, s32 arg2);
void TownMsg_SendStateChange(void *a0);

void Actor_SetStateEx(Player *pl, u8 state, u8 sub, u16 flags)
{
    s32 i;

    pl->prevStateGroup = pl->stateGroup;
    pl->prevState = pl->state;
    pl->stateGroup = state;
    pl->state = sub;
    pl->step = 0;
    pl->stateVar0 = 0;
    pl->stateVar1 = 0;
    pl->stateRand = Rand_Next(1);
    pl->stateRequest = 0;
    if (!(flags & 1)) {
        for (i = 0; i < 4; i++) {
            pl->partAnimOn[i] = 0;
            pl->partAnimMotion[i] = 0;
            pl->partAnimWeight[i] = 0;
        }
    }
    Actor_RestartMotionTime(pl, 0);
    pl->flags &= 0x80100;
    pl->stateFlags = 0;
    pl->motion[0].step = 2.0f;
    pl->motion[1].step = 2.0f;
    if (pl->index != gScene.localPlayer) {
        if (Actor_IsState(pl, 0, 0) == 0) {
            return;
        }
        if (pl->motionId[0] != 1) {
            Actor_SetIdle(pl, 0, 4, 0);
        }
    }
    if (Actor_NeedsStateSend(pl) == 1) {
        TownMsg_SendStateChange(pl);
    }
}

s32 Actor_NeedsStateSend(void *obj)
{
    s8 i;
    ObjHead *o = obj;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    for (i = 0; i < 10; i++) {
        gPlayers[i].index = i;
    }
    if ((o->index == gScene.localPlayer) && (o->unk01E == 0)) {
        return Area_HasTownPcs(gScene.area, i);
    }
    return 0;
}

void Actor_ClearStateWork(void *arg0);
void Actor_ClearEvents(void *arg0);
void Actor_StartIdle(void *obj, s32 n, s32 m);

void Actor_SetIdle(void *obj, s32 a1, s32 n, s32 m)
{
    ObjHead *o = obj;
    Actor_ClearStateWork(obj);
    Actor_ClearEvents(obj);
    o->action = 0;
    switch (o->action) {
    default:
        Actor_StartIdle(obj, n, m);
        break;
    case 0xF:
        break;
    }
    Actor_RestartMotionTime(obj, 0);
}

void Actor_ClearStateWork(void *obj) {
    ObjHead *o = obj;
    *(s32 *)&o->unk004 = 0;
    o->unk2B4 = 0;
    *(s16 *)o->motionStarted = 0;
    o->unk3A0 = 0;
    o->motionTime = 0;
    o->hitDelay = 0;
    o->hitFrames = 0;
    o->unk4BA = 0;
    o->flags = (s32) (o->flags & 0x100);
    o->stateFlags = 0;
    o->motion[0].unk04 = (s32) o->index;
    o->motion[1].unk04 = (s32) o->index;
    o->unk430 = 0;
    o->unk2B8 = 0;
    o->noHitTimer = 0;
    o->unk434 = 0;
    o->timer = 0;
    o->unk392 = 0;
    o->unk4C4 = 0;
    o->unk4C1 = 0;
    o->unk3F8 = 0;
    o->unk417 = 0;
    *(s16 *)((u8 *)obj + 0x7D4) = 0;
    o->carry = 0;
    o->motion[0].step = 2.0f;
    o->motion[1].step = 2.0f;
    o->partAnimOn[0] = 0;
    o->partAnimMotion[0] = 0;
    o->partAnimWeight[0] = 0;
    o->partAnimOn[1] = 0;
    o->partAnimMotion[1] = 0;
    o->partAnimWeight[1] = 0;
    o->partAnimOn[2] = 0;
    o->partAnimMotion[2] = 0;
    o->partAnimWeight[2] = 0;
    o->partAnimOn[3] = 0;
    o->partAnimMotion[3] = 0;
    o->partAnimWeight[3] = 0;
}

void Actor_ClearEvents(void *obj) {
    ObjHead *o = obj;
    o->vel[0] = 0;
    o->vel[1] = 0;
    o->vel[2] = 0;
    o->accel[0] = 0;
    o->accel[1] = 0;
    o->accel[2] = 0;
    o->flags = (s32) (o->flags & 0x104);
}


void Actor_StartIdle(void *obj, s32 n, s32 m)
{
    ObjHead *o = obj;
    o->motionFrame[0] = m;
    o->motionFrame[1] = m;
    o->motionId[0] = 1;
    o->motionId[1] = 0x65;
    o->motionBlend[0] = n / 2;
    o->motionBlend[1] = n / 2;
    o->stateGroup = 0;
    o->state = 1;
    if (Actor_NeedsStateSend(obj) != 0) {
        TownMsg_SendStateChange(obj);
    }
}

/* Static: Actor_SetState keeps the object in a0 across the call. */
static void Actor_RestartMotionTime(void *obj, s16 flag)
{
    ObjHead *o = obj;
    s16 a = o->motionBlend[0];
    s16 b = o->motionFrame[0];

    o->motionTime = 0;
    if (a < 0) {
        a = -a;
    }
    o->motionTime += b - a;
    if (flag != 0) {
        if (a != 0) {
            a--;
        }
        o->hitDelay += (u8)(a - b);
    }
}

s16 Actor_IsState(void *obj, s32 group, s32 state) {
    ObjHead *o = obj;
    if ((o->stateGroup == (group & 0xFF)) && (o->state == (state & 0xFF))) {
        return 1;
    }
    return 0;
}


s32 Player_IsLocal(void *obj);
void Actor_SetMotion(void *obj, s32 motion, s32 frames, s32 c);
void Actor_SetAction(void *a, u16 mode, s32 b, s32 c);
void Actor_SetState(void *, s32, s32);
void TownMsg_SendStateNow(Player *p);
void CamFocus_End(void);
extern s32 gTownMode;
extern s32 gTownMenuOpen;
u8 TownPc_MoveKind(Player *a);
u16 TownPc_InputYaw(Player *, s32);
void Actor_SetFlag(void *, u32);
void TownPc_IdleCheckOffer(Player *pl);
void TownPc_CheckAction(Player *);
void Player_SetMotionPair(Player *p, s32 motion, s32 a, s16 b);
void Player_UpdateMotionTime(Player *p, s16 step);
s32 ItemGet_Reserve(s32 pl, u16 *gotItem);
s32 Game_IsOnline(void);
void TownMsg_Send12(void);
extern s32 D_00802C40[];
u8 Area_GetSetIndex(u16 area);
extern s32 gAreaSpawnLists[];


u8 TownPc_MoveKind(Player *pl)
{
    u8 r = 0;
    u16 v;

    if (gTownMenuOpen == 1 && pl->index == gScene.localPlayer) {
        return 0;
    }
    if (gScene.localPlayer == pl->index) {
        v = pl->input.stickTilt;
        if (v >= 0x78) {
            r = 3;
        } else if (v >= 0x55) {
            r = 1;
        } else if (v >= 0x28) {
            r = 1;
        }
    } else if (pl->state == 1 || pl->state == 0x1D) {
        r = 0;
    } else if (pl->state == 3 || pl->state == 0x23) {
        r = 1;
    } else {
        r = 5;
    }
    if (pl->action == 0 && pl->weaponOut == 0 && (pl->input.held & 0x10) && r != 0
        && (Actor_IsState(pl, 0, 0x24) == 0 || pl->dashDelay == 0)) {
        return 5;
    }
    return r;
}

u16 TownPc_InputYaw(Player *pl, s32 i)
{
    u16 yaw = Camera_GetYaw();
    u16 r = (&pl->input.stickAngle)[i];

    return r + yaw;
}

void Actor_ClearFlag(void *obj, u32 f);

void Actor_SetAction(void *a, u16 mode, s32 b, s32 c)
{
    s32 f = mode;
    ObjHead *o = a;

    switch ((u8)f) {
    default:
        o->action = 0;
        break;
    case 1:
        o->action = 1;
        break;
    case 2:
        o->action = 2;
        break;
    }
    if (f & 0x8000) {
        Actor_ClearFlag(a, 8);
    } else {
        Actor_SetFlag(a, 8);
    }
    Actor_ClearFlag(a, 1);
    Actor_ClearFlag(a, 2);
}

f32 Vec_Distance(f32 *a, f32 *b);

void Actor_StartWalk(Player *pl, s16 n)
{
    if (!(Vec_Distance(&pl->pos[0], &pl->moveTo[0]) < 800.0f)) {
        pl->moveToFrames = 0;
        pl->pos[0] = pl->moveTo[0];
        pl->pos[1] = pl->moveTo[1];
        pl->pos[2] = pl->moveTo[2];
        return;
    }
    pl->moveToFrames = n;
    pl->moveToStep[0] = (pl->moveTo[0] - pl->pos[0]) / (f32)n;
    pl->moveToStep[1] = (pl->moveTo[1] - pl->pos[1]) / (f32)n;
    pl->moveToStep[2] = (pl->moveTo[2] - pl->pos[2]) / (f32)n;
    pl->weaponTimer = n;
    pl->envTimer = (s16)(pl->targetYaw - pl->rot[1]) / n;
}

void Player_SetFlag(void *obj, u32 flag);

void Actor_StartMoveTo(Em *obj, s32 n)
{
    NpcWork *m = (NpcWork *)&obj->work;

    m->moveCount = n;
    m->step[0] = (obj->goal[0] - obj->posX) / (f32)n;
    m->step[1] = (obj->goal[1] - obj->posY) / (f32)n;
    m->step[2] = (obj->goal[2] - obj->posZ) / (f32)n;
    Player_SetFlag((Player *) obj, 0x4000000);
}

void Actor_StepMoveNpc(Em *obj)
{
    NpcWork *m;

    m = (NpcWork *)&obj->work;
    if (m->moveCount != 0) {
        obj->posX = obj->posX + m->step[0];
        obj->posY = obj->posY + m->step[1];
        obj->posZ = obj->posZ + m->step[2];
        m->moveCount = m->moveCount - 1;
    }
}

void TownPc_StepSlide(Player *pl)
{
    s16 n = pl->moveToFrames;

    if (n != 0) {
        pl->moveToFrames = n - 1;
        if (n > 0) {
            pl->pos[0] += pl->moveToStep[0];
            pl->pos[1] += pl->moveToStep[1];
            pl->pos[2] += pl->moveToStep[2];
        }
    }
}

void TownPc_SmoothRemoteYaw(Player *pl)
{
    if (pl->index != gScene.localPlayer) {
        if (pl->state != 2) {
            (pl->weaponTimer)--;
        }
        pl->rot[1] += (s16)(pl->targetYaw - pl->rot[1]) / 4;
    }
}

void Actor_StepMove(Em *obj)
{
    u16 count;
    NpcWork *m;

    m = (NpcWork *)&obj->work;
    count = m->moveCount;
    if (count != 0) {
        m->moveCount = count - 1;
        if ((s32) count > 0) {
            obj->posX += m->step[0];
            obj->posY += m->step[1];
            obj->posZ += m->step[2];
        }
    }
}

void Actor_ClearFlag(void *obj, u32 f)
{
    ObjHead *o = obj;
    if ((f & 0x80000000) == 0) {
        o->flags &= ~f;
    } else {
        o->stateFlags &= ~(f & 0x7FFFFFFF);
    }
}

void Actor_SetFlag(void *obj, u32 f)
{
    ObjHead *o = obj;
    if ((f & 0x80000000) == 0) {
        o->flags |= f;
    } else {
        o->stateFlags |= f & 0x7FFFFFFF;
    }
}

/* The action spots of the current area (gAreaSpawnLists[0] by Area_GetSetIndex); the
 * callers pass the area, which it does not use. */
u8 *ActSpot_GetList(u16 area)
{
    return *(u8 **)((u8 *)gAreaSpawnLists + ((Area_GetSetIndex(area) & 0xFF) * 4));
}

/* An action spot of an area (ActSpot_GetList; 0x18 bytes). */
typedef struct ActSpot {
    /* 0x00 */ u16 id;
    /* 0x02 */ u16 kind;            /* 0 ends some lists */
    /* 0x04 */ f32 x;
    /* 0x08 */ f32 y;
    /* 0x0C */ f32 z;
    /* 0x10 */ f32 r;
    /* 0x14 */ u16 angle;
    /* 0x16 */ u16 unk16;
} ActSpot;
f32 Math_Sqrtf(f32 x);
s32 ActSpot_IsUsable(Player *pl, ActSpot *s);

void TownPc_FindSpot(Player *pl)
{
    s8 f = 0;
    ActSpot *s;
    f32 x;
    f32 dx;
    f32 dz;

    if (pl->index == gScene.localPlayer && gTownMenuOpen != 1) {
        if (pl->spot == NULL && gTownMode == 0) {
            f = 1;
        }
        pl->spot = NULL;
        s = (ActSpot *)ActSpot_GetList(pl->area);
        if (s == NULL) {
            pl->spot = NULL;
            return;
        }
        for (; (x = s->x) != -1.0f; s++) {
            if (pl->pos[1] < s->y - 50.0f || !(pl->pos[1] < s->y + 300.0f)) {
                continue;
            }
            dx = pl->pos[0] - x;
            dz = pl->pos[2] - s->z;
            if (Math_Sqrtf(dx * dx + dz * dz) <= s->r) {
                if (ActSpot_IsUsable(pl, s) == 1) {
                    pl->spot = s;
                    if (f != 0) {
                        pl->target = pl;
                    }
                }
                return;
            }
        }
        pl->spot = NULL;
        if (((Player *)pl->target) == pl) {
            pl->target = NULL;
        }
    }
}

s32 Save_TestEventFlag(s32 flag);

s32 ActSpot_AreaEventUnseen(void)
{
    switch (gScene.area) {
    case 0xAC:
        if (Save_TestEventFlag(0x89) == 0) {
            return 1;
        }
        break;
    case 0x57:
        if (Save_TestEventFlag(0x8A) == 0) {
            return 1;
        }
        break;
    }
    return 0;
}

s32 Hud_ItemGetBusy(void);
s32 Actor_IsFacingWithin(u8 *arg0, void *arg1, s32 arg2);
Em *Npc_Find(u8 kind);
s32 Cond_Check(u16 id);
s32 LobbyTime_IsAm(void);
s32 MyHouse_InOwnHouse(void);
extern u8 gTownSeats[];
extern s16 gHouseCloseTimer;
extern u8 *gPoogieDigSpots;
extern u8 gPoogieDigSpotIdx;
extern u8 gPoogieDigState;

/* The game's empty delay slots come from the default-first switch on npc[0x15] (case 1); the
 * one-case switch on s->id gives case 0x18's beq and b. */
s32 ActSpot_IsUsable(Player *pl, ActSpot *s)
{
    u8 *npc;

    if (Hud_ItemGetBusy() == 1) {
        return 0;
    }
    switch (s->kind) {
    case 3:
    case 0xC:
        return Actor_IsFacingWithin((u8 *) pl, &s->x, 0x5A);
    case 0x11:
        if (((u8 *)gLobby)[0x396A] == 0) {
            return 0;
        }
        break;
    case 1:
        if (Game_IsOnline() == 0) {
            npc = (u8 *)Npc_Find(0x10);
            if (npc == NULL) {
                return 0;
            }
            if (Cond_Check(0x15) == 0) {
                return 0;
            }
            return npc[0x15] == 0x8A ? 1 : 0;
        }
        break;
    case 2:
        if (Game_IsOnline() == 1) {
            npc = (u8 *)Npc_Find(0x33);
        } else {
            npc = (u8 *)Npc_Find(0x19);
        }
        if (npc == NULL) {
            return 0;
        }
        break;
    case 0xE:
        if (Game_IsOnline() == 0) {
            npc = (u8 *)Npc_Find(5);
            if (Cond_Check(1) == 0) {
                return 0;
            }
            if (Cond_Check(0x41) == 0) {
                return 0;
            }
            if (npc == NULL) {
                return 0;
            }
            switch (npc[0x15]) {
            default:
                return 0;
            case 1:
                break;
            }
            if (gTownMode == 0x30) {
                return 1;
            }
            switch (s->id) {
            case 0:
                if (LobbyTime_IsAm() != 1) {
                    return 0;
                }
                break;
            case 1:
                if (LobbyTime_IsAm() != 0) {
                    return 0;
                }
                break;
            }
        } else {
            if (((u8 *)gLobby)[0x396A] != 0) {
                return 0;
            }
            if (gTownSeats[s->id] != 0) {
                return 0;
            }
        }
        break;
    case 0xD:
        if (gScene.area == 0x89) {
            npc = (u8 *)Npc_Find(0x14);
        } else if (gScene.area == 0x83) {
            npc = (u8 *)Npc_Find(0x22);
        } else {
            return 0;
        }
        if (npc == NULL) {
            return 0;
        }
        if (gTownMode != 0) {
            return 0;
        }
        if (gPoogieDigState == 0) {
            return 0;
        }
        if (s->id != gPoogieDigSpots[gPoogieDigSpotIdx]) {
            return 0;
        }
        if (pl->state == 0x80) {
            return 0;
        }
        break;
    case 0x18:
        switch (s->id) {
        case 0x88:
            if (Cond_Check(0x64) == 0) {
                return 0;
            }
            break;
        }
        break;
    case 8:
        if (ActSpot_AreaEventUnseen() == 0) {
            return 0;
        }
        break;
    case 5:
    case 6:
        if (MyHouse_InOwnHouse() == 0) {
            return 0;
        }
        break;
    case 0xA:
        if (((u8 *)gLobby)[0x396C] != 0) {
            return 0;
        }
        if (Game_IsOnline() == 1) {
            if (gHouseCloseTimer > 0) {
                return 0;
            }
        } else if (gSave.unk36E8 == 0 && Npc_Find(0xA) != NULL && s->id == 1) {
            return 0;
        }
        break;
    }
    return 1;
}

extern u8 gNetUsers[];
s32 memcmp(void *, void *, s32);
s32 Area_IsHouse(u16 area);

/* Whether the player pl is shown to the local player: in the same area,
 * and online the same member in its slot (LobbyState +0xA16 vs gNetUsers),
 * in the same party slot group where Area_IsHouse says so, and
 * Area_HasTownPcs for the area. */
s32 TownPc_IsInMyArea(Player *pl)
{

    if (pl->area != gScene.area) {
        return 0;
    }
    if (Game_IsOnline() == 0) {
        return 1;
    }
    if (memcmp((u8 *)(gLobby->members[0].id + pl->index * 0x11C), gNetUsers + pl->index * 0x11C, 8) != 0) {
        return 0;
    }
    if (Area_IsHouse(gScene.area) == 1) {
        if (gTownMode == 0x3A) {
            return 0;
        }
        if (gLobby->memberSlot[gScene.localPlayer] != gLobby->memberSlot[pl->index]) {
            return 0;
        }
    }
    if (pl->index != gScene.localPlayer) {
        return Area_HasTownPcs(pl->area) == 1;
    }
    return 1;
}

void Npc_UpdateHitStop(Em *npc)
{
    u8 temp_a1;
    u8 temp_v1;

    temp_a1 = npc->hitStopAdd;
    if (temp_a1 != 0) {
        npc->hitStop = (u8) (npc->hitStop + temp_a1);
        npc->hitStopAdd = 0U;
        return;
    }
    temp_v1 = npc->hitStop;
    if (temp_v1 != 0) {
        npc->hitStop = (u8) (temp_v1 - 1);
    }
}


/* Enter state (group, state) and reset the per-state values. The game
 * passes the object to Actor_IsState in a0 without setting it again after
 * Actor_RestartMotionTime: that callee is static and defined earlier in this unit. */
void Actor_SetState(void *obj, s32 group, s32 state)
{
    ObjHead *o = obj;

    if (o->isEm == 0) {
        o->prevStateGroup = o->stateGroup;
        o->prevState = o->state;
    }
    o->stateGroup = group;
    o->state = state;
    o->step = 0;
    o->stateVar0 = 0;
    o->stateVar1 = 0;
    if (o->isEm == 0) {
        o->stateRand = Rand_Next(1);
    }
    if (o->isEm != 0) {
        o->partAnimOn[0] = 0;
        o->partAnimMotion[0] = 0;
        o->partAnimWeight[0] = 0;
        o->partAnimOn[1] = 0;
        o->partAnimMotion[1] = 0;
        o->partAnimWeight[1] = 0;
        o->partAnimOn[2] = 0;
        o->partAnimMotion[2] = 0;
        o->partAnimWeight[2] = 0;
        o->partAnimOn[3] = 0;
        o->partAnimMotion[3] = 0;
        o->partAnimWeight[3] = 0;
    }
    if (o->isEm == 0 && o->stateGroup == 2) {
        o->unk417 = 0;
    }
    Actor_RestartMotionTime(o, 0);
    o->flags &= 0x100100;
    o->stateFlags = 0;
    o->motion[0].step = 2.0f;
    o->motion[1].step = 2.0f;
    if (o->unk01E == 0 && o->index != gScene.localPlayer) {
        if (Actor_IsState(o, 0, 0) == 0) {
            return;
        }
        if (o->motionId[0] != 1) {
            Actor_SetIdle(o, 0, 4, 0);
        }
    }
    if (Actor_NeedsStateSend(o) != 0) {
        TownMsg_SendStateChange(o);
    }
}

/* A lock-on candidate (a player or a townsperson) as the lobby's lock-on
 * reads it: it keeps its own values in fields the game uses otherwise
 * (ObjHead.hp, ObjHead.target). */
typedef struct LockOn {
    /* 0x000 */ u8 pad0[0xAC];
    /* 0x0AC */ f32 pos[3];         /* ObjHead.pos */
    /* 0x0B8 */ u8 padB8[0x2C2 - 0xB8];
    /* 0x2C2 */ s16 sector;         /* ObjHead.hp: by the angle from the player, 0 to 5 (LockOn_SetSector); -1 out of reach; orders the list (LockOn_Insert) */
    /* 0x2C4 */ u8 pad2C4[0x3B4 - 0x2C4];
    /* 0x3B4 */ struct LockOn *next; /* ObjHead.target: the next candidate (LockOn_Insert); a player's: its lock-on target */
    /* 0x3B8 */ u8 pad3B8[0x3D6 - 0x3B8];
    /* 0x3D6 */ u8 targeted;
    /* 0x3D7 */ u8 pad3D7[0x4A8 - 0x3D7];
    /* 0x4A8 */ f32 dist;           /* from the player (LockOn_TestCandidate) */
} LockOn;

void LockOn_Insert(LockOn **list, LockOn *obj)
{
    LockOn *p;
    LockOn *prev;
    s32 prio;
    LockOn *head;

    if (list == NULL) {
        *list = obj;
        return;
    }
    head = *list;
    prev = NULL;
    p = head;
    if (p != NULL) {
        prio = obj->sector;
        do {
            if ((u16)p->sector > prio) {
                break;
            }
            if ((u16)p->sector == prio) {
                if (prio < 3) {
                    if (!(p->dist <= obj->dist)) {
                        break;
                    }
                } else if (p->dist < obj->dist) {
                    break;
                }
            }
            prev = p;
            p = p->next;
        } while (p != NULL);
    }
    if (prev == NULL) {
        obj->next = head;
        *list = obj;
        return;
    }
    obj->next = prev->next;
    prev->next = obj;
}

void LockOn_SetSector(Player *pl, LockOn *target, u32 angle, u32 range, u8 mode)
{
    if (pl->target != 0 && mode != 0) {
        if (mode == 1) {
            if ((u16)angle < (u16)range) {
                angle = (u16)(s32)(angle * (1.0f / (range / 3.0f)));
            } else {
                angle = (u16)(u32)(6.0f - (s32)((0xFFFF - (u16)angle) * (1.0f / (range / 3.0f))) - 1.0f);
            }
        } else {
            if ((u16)angle < (u16)range) {
                angle = (u16)(u32)(6.0f - (s32)(angle * (1.0f / (range / 3.0f))) - 1.0f);
            } else {
                angle = (u16)(s32)((0xFFFF - (u16)angle) * (1.0f / (range / 3.0f)));
            }
        }
    } else {
        if ((u16)angle < (u16)range) {
            angle = (u16)(s32)(angle * (1.0f / (range / 3.0f)));
        } else {
            angle = (u16)(s32)((0xFFFF - (u16)angle) * (1.0f / (range / 3.0f)));
        }
    }
    target->sector = angle;
}

/* unit's own declaration: LockOn_Insert reads a1 = arg1, the candidate (left by LockOn_SetSector(arg0, arg1, ...), which keeps a1) */
void LockOn_Insert();
void LockOn_SetSector(Player *arg0, LockOn *arg1, u32 angle, u32 range, u8 mode);
s32 Actor_HeadingDiff(void *obj, void *pos);

s32 LockOn_TestCandidate(Player *pl, LockOn *target, LockOn **list, s32 range, u8 mode, f32 fparg0)
{
    f32 dist;
    s32 temp_a0;
    s32 angle;
    s32 temp_v1;

    target->next = NULL;
    target->targeted = 0;
    if (!((f32) abs((s32) (target->pos[1] - pl->pos[1])) <= 200.0f)) {
        target->sector = -1;
        return 0;
    }
    angle = Actor_HeadingDiff(pl, target->pos) & 0xFFFF;
    dist = Vec_Distance(&pl->pos[0], target->pos);
    temp_a0 = range & 0xFFFF;
    temp_v1 = angle & 0xFFFF;
    if (((temp_v1 < temp_a0) || ((0xFFFF - temp_a0) < temp_v1)) && (dist < fparg0)) {
        target->dist = dist;
        LockOn_SetSector((Player *)((u8 *) pl), target, angle, range, mode);
        LockOn_Insert(list);
        return 1;
    }
    target->sector = -1;
    return 0;
}

s32 Kbd_IsOpen(void); /* unit's own declaration: the definition (sub_main) returns s8;
 * LockOn_Update is 281 words off with s8 */
s32 Npc_IsAct4For50(Em *em);
void LobbySe_Play(s32 id);
s32 Area_IsOnlineHouse(s32 area);
extern u8 gLockOnCycleDir;
extern s8 gLockOnMarkSkip;
extern s8 gLockOnNoCycle;

void LockOn_Update(void)
{
    u8 *em;
    Player *pl;
    u8 *n;
    s32 i;
    u16 count;
    LockOn *old;
    f32 dist;
    LockOn *target;
    Player *p;
    Player *me;

    target = NULL;
    pl = &gPlayers[gScene.localPlayer];
    old = ((LockOn *)pl->target);
    em = (u8 *)gEms;
    if (Hud_ItemGetBusy() == 1) {
        pl->target = NULL;
        return;
    }
    if (gTownMenuOpen == 1 || (gTownMode != 0 && gTownMode != 7)) {
        if (gScene.menuOpen != 0 || Kbd_IsOpen() != 0 || gTownMode != 0) {
            gLockOnMarkSkip = 1;
        }
        return;
    }
    if (gTownMode == 7) {
        if (pl->spot == NULL) {
            gLockOnMarkSkip = 1;
            return;
        }
        if (((u16 *)pl->spot)[1] != 0x11) {
            gLockOnMarkSkip = 1;
            return;
        }
    }
    if (pl->action == 1) {
        pl->target = (LockOn *)pl;
        return;
    }
    if (pl->input.pressed & 0x800) {
        gLockOnCycleDir = 2;
    } else if (pl->input.pressed & 0x400) {
        gLockOnCycleDir = 1;
    }
    count = 0;
    for (i = 0; i < 20; i++, em += 0x9D0) {
        if (em[0] != 0) {
            if (Area_IsOnlineHouse(gScene.area) != 0 || Npc_IsAct4For50((Em *)em) != 0) {
                dist = 200.0f;
            } else {
                dist = 400.0f;
            }
            n = em + 0x438;
            if (Game_IsOnline() == 1 && MyHouse_InOwnHouse() == 0 && n[0] == 0x14) {
                continue;
            }
            if (((s8 *)n)[2] == 0) {
                if (em[0x1B] == 0x4D) {
                    count += (u8)LockOn_TestCandidate(pl, (LockOn *)em, &target, 0x2AAB, gLockOnCycleDir, 1.5f * dist);
                } else {
                    count += (u8)LockOn_TestCandidate(pl, (LockOn *)em, &target, 0x2AAB, gLockOnCycleDir, dist);
                }
            }
        }
    }
    p = gPlayers;
    me = &gPlayers[gScene.localPlayer];
    for (i = 0; i < 10; i++, p++) {
        if (p->active != 0 && p->index != gScene.localPlayer && (u8)TownPc_IsInMyArea(p) &&
            ((u8 *)((s32)gLobby + i))[0x394A] != 0) {
            count += (u8)LockOn_TestCandidate((Player *)me, (LockOn *)p, &target, 0x238E, gLockOnCycleDir, 300.0f);
        }
    }
    if (count >= 2 || (count > 0 && me->spot != NULL)) {
        gLockOnNoCycle = 0;
    } else {
        gLockOnNoCycle = 1;
    }
    if (gTownMode == 7) {
        target = NULL;
    }
    if (target == NULL) {
        if (me->spot == NULL) {
            if (((LockOn *)me->target) != NULL) {
                ((LockOn *)me->target)->targeted = 0;
            }
            me->target = NULL;
            return;
        }
        me->target = (LockOn *)me;
        return;
    }
    if (((LockOn *)me->target) != NULL) {
        ((LockOn *)me->target)->targeted = 0;
        if ((me->input.pressed & 0x40) && me->spot != NULL) {
            me->target = (LockOn *)me;
        } else if (me->input.pressed & 0xC00) {
            LockOn *cur = ((LockOn *)me->target);
            LockOn *next = cur->next;
            if (next == NULL && me->spot == NULL) {
                me->target = target;
            } else if (next == NULL && me->spot != NULL) {
                me->target = (LockOn *)me;
            } else if (cur == (LockOn *)me) {
                me->target = target;
            } else {
                me->target = next;
            }
        } else {
            LockOn *cur = ((LockOn *)me->target);
            if (cur->sector == -1) {
                me->target = target;
            } else if (me->spot == NULL && cur == (LockOn *)me) {
                me->target = target;
            }
        }
        if (old != ((LockOn *)me->target)) {
            LobbySe_Play(0xC);
        }
    } else {
        me->target = target;
        LobbySe_Play(0xC);
    }
    ((LockOn *)me->target)->targeted = 1;
}

void TownPc_SetG1FromTable(Player *pl)
{
    u8 temp_v1;

    temp_v1 = pl->actRequest;
    if (temp_v1 != 0) {
        Actor_SetStateEx(pl, 1, *(u8 *)((u8 *)D_00802C40 + temp_v1), 0);
    }
}

void func_00796DE0(Player *pl)
{
    if (pl->unk844 == 0) {
        pl->unk867 = (pl->unk867 + 1);
        if (((s32) pl->unk867 >= 0xA) && (Game_IsOnline() == 1)) {
            pl->unk844 = 1U;
        }
    }
}

s8 MyHouse_LocalSlot(void);

s32 MyHouse_IsOwner(void)
{
    s8 r;

    if (Game_IsOnline() == 0) {
        return 1;
    }
    r = MyHouse_LocalSlot();
    return r == gScene.localPlayer;
}

s32 MyHouse_InOwnHouse(void)
{
    switch (gScene.area) {
    case 0xAD:
    case 0xAE:
    case 0xAF:
        break;
    case 0x56:
        if (Game_IsOnline() == 0) {
            return 1;
        }
        break;
    default:
        return 0;
    }
    if (Game_IsOnline() == 0) {
        return 1;
    }
    return MyHouse_IsOwner() != 0;
}

/* The local player's slot (0-9, from LobbyState +0x1538's '0'-'9') while
 * in the areas 0x56 and 0xAD-0xAF, else -1. The address is an integer sum
 * (the original adds the index first) and `0 <= slot` gives its slt. */
s8 MyHouse_LocalSlot(void)
{
    s32 i = gScene.localPlayer;
    u16 area = gPlayers[i].area;
    s8 slot;

    switch (area) {
    case 0xAD:
    case 0xAE:
    case 0xAF:
    case 0x56:
        break;
    default:
        return -1;
    }
    if (*(u8 *)(i + (s32)gLobby + 0x1538) == 0) {
        return -1;
    }
    slot = *(u8 *)(i + (s32)gLobby + 0x1538) - '0';
    if (0 <= slot && slot < 10) {
        return slot;
    }
    return -1;
}

s32 Area_IsOnlineHouse(s32 area)
{
    switch (area & 0xFFFF) {
    case 0xAD:
    case 0xAE:
    case 0xAF:
        return 1;
    }
    return 0;
}

void Pad_UpdateInput(void);
void TownPc_UpdateInput(void);
void TownPc_Update(Player *pl);
void TownMsg_SendPosition(Player *unused0);

/* Each frame, the players shown in the village (TownPc_IsInMyArea; LobbyState
 * +0x3989 skips one, +0x152E its state) get TownPc_Update, the local one
 * after its pad input; then the local position message (TownMsg_SendPosition
 * takes the player, unused). */
void TownPc_UpdateAll(void)
{
    Player *pl;
    s32 i;

    i = 0;
    while (i < 10) {
        pl = &gPlayers[i];
        if ((s8)gLobby->lookPending[i] != 0) {
            goto next;
        }
        if (pl->active == 0 || (s8)gLobby->pcLoaded[i] == 0) {
            pl->visible = 0;
            i++;
            continue;
        }
        if ((u8)TownPc_IsInMyArea(pl) == 0) {
            pl->visible = 0;
        } else if ((s8)gLobby->pcLoaded[pl->index] == 1) {
            pl->visible = 1;
        }
        if (pl->visible != 0) {
            gScene.padRead = 1;
            if (pl->index == gScene.localPlayer) {
                Pad_UpdateInput();
                TownPc_UpdateInput();
            }
            TownPc_Update(pl);
        }
    next:
        i++;
    }
    TownMsg_SendPosition(&gPlayers[gScene.localPlayer]);
}

void TownPc_RunGroup0(Player *pl);
void TownPc_RunGroup1(Player *pl);
void Actor_TurnToTarget(Player *p);
void TownPc_UnwindAngle76E(Player *pl);
void TownPc_StepSlide(Player *obj);
void TownPc_SmoothRemoteYaw(Player *obj);
void Em_CollideField(Em *em, u8 keep);
void func_330DE0(void *p);
struct TriHdr;
s32 Em_GetFloorInfo(Em *em, f32 *pos, struct TriHdr *info, f32 *out, s32 *arg4);

/* A player's frame in the lobby (the local one and the others): its state
 * function, motions, floor height and the action checks. */
void TownPc_Update(Player *pl)
{
    f32 y;

    pl->offerStarted = 0;
    TownPc_TickTimers(pl);
    *(Vec3f *)pl->prevPos = *(Vec3f *)pl->pos;
    pl->unk58E = pl->unk58C = -1;
    pl->unk3F8 = 0;
    pl->freeState = 0;
    switch (pl->stateGroup) {
    case 0:
        TownPc_RunGroup0(pl);
        break;
    case 1:
        TownPc_RunGroup1(pl);
        break;
    }
    if (pl->stateRequest != 0) {
        pl->freeState = 0;
        switch (pl->stateGroup) {
        case 0:
            TownPc_RunGroup0(pl);
            break;
        case 1:
            TownPc_RunGroup1(pl);
            break;
        }
        pl->stateRequest = 0;
    }
    pl->actRequest = 0;
    Actor_TurnToTarget(pl);
    TownPc_UnwindAngle76E(pl);
    TownPc_StepSlide(pl);
    if (pl->index != gScene.localPlayer) {
        TownPc_SmoothRemoteYaw(pl);
    }
    Actor_StartMotions(pl);
    if (pl->index == gScene.localPlayer) {
        Em_CollideField((Em *) pl, 0);
    }
    if ((pl->state != 0x1A && pl->state != 0x19) || gScene.area == 0x57) {
        if (Em_GetFloorInfo((Em *) pl, pl->pos, (struct TriHdr *) &pl->floorKind, &y, (s32 *) &pl->surfaceY) == 1) {
            pl->floorY = y;
        }
        pl->pos[1] = pl->floorY;
    }
    pl->offerRequest = 0;
    if ((u8)TownPc_IsInMyArea(pl)) {
        func_330DE0(pl);
    }
    if (pl->index == gScene.localPlayer) {
        TownPc_FindSpot(pl);
    }
    if ((u8)TownPc_IsInMyArea(pl)) {
        pl->handlers->unkC(pl);
    }
    if (TownPc_IsFree(pl) == 0) {
        pl->weaponShown = 0;
    } else {
        pl->weaponShown = 1;
    }
}

s32 Motion_ReachedFrame(void *, s32, f32);
s32 HairMenu_IsIdle(void);
s32 Contest_IsIdle(void);

s32 TownPc_IsFree(Player *pl)
{
    if (pl->stateGroup == 0) {
        switch (pl->state) {
        case 0xE:
            if (Motion_ReachedFrame(pl, 0, 144.0f) != 0) {
                return 0;
            }
            break;
        case 0x70:
        case 0x71:
        case 0x6F:
        case 0x75:
        case 0x74:
        case 0xF:
        case 0x10:
        case 0x17:
        case 0x1A:
            return 0;
        }
    }
    switch (gTownMode) {
    case 0x30:
        if (pl->index == gScene.localPlayer) {
            return Contest_IsIdle();
        }
        break;
    case 0x3C:
        return HairMenu_IsIdle();
    case 0x32:
        return 0;
    }
    return 1;
}

/* Turns an actor from its angle (+0xA4) toward its target angle (+0xE) at
 * a rate by its state, leaning (+0x69C, kept outside 0xA00..0xF600) while it
 * turns; the local player's turn is always done here, the others' unless
 * they are in state 3. */
void Actor_TurnToTarget(Player *pl)
{
    u32 rate;
    u32 diff;
    u16 lean;
    s32 t;
    u32 cur;

    if (pl->index != gScene.localPlayer && pl->state == 3) {
        return;
    }
    if (Actor_IsState(pl, 0, 2) != 0 || Actor_IsState(pl, 0, 5) != 0) {
        rate = 0x71C;
    } else if (Actor_IsState(pl, 0, 4) != 0) {
        rate = 0x5B0;
    } else if (Actor_IsState(pl, 0, 0x11) != 0) {
        rate = 0;
    } else {
        rate = 0xFA4;
    }
    cur = pl->targetYaw;
    diff = (u16)(cur - (u16)pl->rot[1]);
    switch (pl->motionId[0]) {
    case 0x27:
        rate = 0x71C;
        lean = 0x80;
        break;
    case 3:
        lean = 0x80;
        break;
    case 4:
        lean = 0xA0;
        break;
    default:
        lean = 0;
        break;
    }
    if ((u16)(diff + rate) < rate * 2) {
        pl->rot[1] = cur;
        pl->unk2B8 = 0;
        t = pl->lean;
        if (t + 0x300 <= 0x600) {
            pl->lean = 0;
        } else if (t < 0x8000) {
            pl->lean = t - 0x300;
        } else {
            pl->lean = t + 0x300;
        }
    } else if (diff < 0x8000U) {
        pl->rot[1] = (u16)(pl->rot[1] + rate);
        pl->lean -= lean;
    } else {
        pl->rot[1] = (u16)(pl->rot[1] - rate);
        pl->lean += lean;
    }
    t = pl->lean;
    if (t <= 0xF600 && t >= 0xA00) {
        if (t >= 0x8000) {
            pl->lean = 0xF600;
        } else {
            pl->lean = 0xA00;
        }
    }
}

void TownPc_UnwindAngle76E(Player *pl)
{
    u16 x;
    s32 t;

    pl->lookFound = 0;
    x = *(u16 *)&pl->lookYaw;
    if (x != 0) {
        t = x + 0x400;
        if (t < 0x801 && t >= 0) {
            pl->lookYaw = 0;
        } else if ((s16)x >= 0) {
            *(u16 *)&pl->lookYaw -= 0x400;
        } else {
            *(u16 *)&pl->lookYaw += 0x400;
        }
    }
}

void TownPc_State01(Player *pl)
{
    u16 m;

    if (pl->unk844 != 0) {
        Actor_SetState(pl, 0, 0x1D);
        return;
    }
    if (pl->motionTime >= 0xF0 && pl->gender == 1 && pl->motionId[0] != 0x3F) {
        Player_SetMotionPair(pl, 0x3F, 2, 0);
    } else {
        m = pl->motionId[0];
        if (m != 1 && m != 0x3F) {
            if (m == 0xCF || m == 0x26E) {
                Actor_SetIdle(pl, 0, 0xC, 0);
            } else {
                Actor_SetIdle(pl, 0, 4, 0);
            }
        }
    }
    TownPc_CheckAction(pl);
}

void TownPc_State03(Player *p, s32 variant)
{
    s32 kind;
    s32 timer;
    s32 temp_v1;
    u16 motionId;
    u8 step;
    u8 stateVar1;

    step = p->step;
    if (step == 0) {
        p->step = (u8) (step + 1);
        p->stateVar0 = 0;
        p->stateVar1 = 0xAU;
        p->timer = 0xA;
        Actor_SetAction(p, 0, 0, 0);
        Actor_SetFlag(p, 0x1000);
        switch (variant) {
        case 0:
            if (p->unk844 != 0) {
                if (p->motionId[0] != 0x27) {
                    Actor_SetMotion(p, 0x27, 6, 0);
                }
            } else if (p->motionId[0] != 3) {
                Actor_SetMotion(p, 3, 6, 0);
            }
            Actor_SetFlag(p, 0x02000000);
            break;
        case 1:
            if (p->unk844 != 0) {
                if (p->motionId[0] != 0x258) {
                    Actor_SetMotion(p, 0x258, 4, 0);
                }
            } else if (p->gender == 1) {
                Actor_SetMotion(p, 0x40, 4, 0);
            } else {
                Actor_SetMotion(p, 2, 4, 0);
            }
            break;
        case 6:
            p->dashDelay = 0x1E;
            if (p->unk844 != 0) {
                if (p->motionId[0] != 0x27) {
                    Actor_SetMotion(p, 0x27, 4, 0);
                }
            } else if (p->motionId[0] != 3) {
                Actor_SetMotion(p, 3, 0, 0x3A);
            }
            Actor_SetFlag(p, 0x02000000);
            break;
        case 9:
            Actor_SetMotion(p, 0x258, 4, 0);
            break;
        }
        p->motionTime = 0;
    }
    temp_v1 = TownPc_MoveKind((Player *)p) & 0xFF;
    stateVar1 = p->stateVar1;
    if (stateVar1 != 0) {
        p->stateVar1 = (u8) (stateVar1 - 1);
    }
    timer = p->timer;
    if (timer > 0) {
        p->timer = (s32) (timer - 1);
    }
    kind = temp_v1 & 0xFF;
    if ((kind == 0) && (p->timer == 0)) {
        motionId = p->motionId[0];
        if ((motionId == 3) || (motionId == 4)) {
            if (p->stateVar1 == 0) {
                Actor_SetStateEx((void *) p, 0U, 6U, 0);
            } else {
                Actor_SetIdle((void *) p, 0, 4, 0);
            }
        } else {
            Actor_SetIdle((void *) p, 0, 4, 0);
        }
        TownPc_IdleCheckOffer(p);
        return;
    }
    if ((p->motionId[0] == 3) && (kind < 5)) {
        Actor_SetIdle((void *) p, 0, 4, 0);
        return;
    }
    if ((p->index == gScene.localPlayer) && (kind != 0)) {
        p->targetYaw = TownPc_InputYaw(p, 0);
    }
    if (Player_IsLocal((Player *) p) != 0) {
        TownPc_CheckAction((void *) p);
    }
}

void Vec_Rotate(f32 *out, f32 *v, f32 *m);

/* A player stepping forward (a walk start, or a dash with an item
 * out), then back to the idle states by what TownPc_MoveKind says. */
void TownPc_State04(Player *pl)
{
    f32 v[4];
    f32 d[3];
    u8 r;

    switch (pl->step) {
    case 0:
        pl->step++;
        Actor_SetAction(pl, 0, 0, 0);
        Actor_SetFlag(pl, 0x600);
        pl->motionTime = 0;
        pl->dashDelay = 0x1E;
        if (pl->unk844 != 0) {
            if (pl->motionId[0] != 0x27) {
                Player_SetMotionPair(pl, 0x27, 4, 0);
            }
            pl->motion[1].step = pl->motion[0].step = 2.0f;
        } else if (pl->motionId[0] == 3) {
            pl->motion[1].step = pl->motion[0].step = 2.5f;
        } else {
            Player_SetMotionPair(pl, 3, 0, 0x3A);
            pl->motion[1].step = pl->motion[0].step = 2.5f;
        }
        pl->targetYaw = pl->rot[1];
        break;
    case 1:
        if (pl->index == gScene.localPlayer && pl->input.stickTilt >= 0x28) {
            pl->targetYaw = TownPc_InputYaw(pl, 0);
        }
        d[0] = 0.0f;
        d[1] = 0.0f;
        if (pl->unk844 != 0) {
            d[2] = 1.0f;
        } else {
            d[2] = 5.0f;
        }
        Vec_Rotate((f32 *) v, (f32 *) d, &pl->rotMtx[0]);
        if (gScene.localPlayer == pl->index) {
            pl->pos[0] += v[0];
            pl->pos[2] += v[2];
        }
        if (Player_IsLocal(pl)) {
            r = TownPc_MoveKind(pl);
            if (r != 5 && (gScene.localPlayer == pl->index || Game_IsOnline() == 0)) {
                if (r != 0) {
                    if (pl->dashDelay == 0) {
                        Actor_SetStateEx(pl, 0, 3, 0);
                    } else {
                        TownPc_CheckAction(pl);
                    }
                } else {
                    Actor_SetStateEx(pl, 0, 6, 0);
                }
            } else {
                TownPc_CheckAction(pl);
            }
        }
        break;
    }
}

void TownPc_State06(Player *pl)
{
    u8 step;

    if (pl->unk844 != 0) {
        Actor_SetState(pl, 0, 0x1D);
        Actor_SetAction(pl, 0, 0, 0);
    } else {
        step = pl->step;
        switch (step) {
        case 0:
            pl->step = (u8) (step + 1);
            Actor_SetAction(pl, 0, 0, 0);
            Actor_SetMotion(pl, 0x1E, 4, 0);
            break;
        case 1:
            if (pl->motion[0].playing == 0) {
                Actor_SetIdle((void *) pl, 0, 0xA, 0);
            }
            break;
        }
    }
}

s16 Player_IsState(void *obj, u8 state, u8 sub);
s32 SavePouch_Count(u16 item);
s16 SavePouch_HasFreeSlot(void);
s32 SavePouch_Room(u16 item);
u8 Notice_PushItem(char *item);
u8 Notice_Push(u8, s16, s16);
void TownMsg_Send0B(void *x, Player *p);

/* A player taking the item another player in the same area holds out to
 * it (state 0, 0xA, within 300): into the pouch when it has room, else
 * a notice. Returns 1 when a player was found. */
s32 TownPc_TakeOfferedItem(Player *pl)
{
    s16 i;
    Player *o;

    if (Game_IsOnline() == 0) {
        return 0;
    }
    if (pl->offerNoticeTimer != 0) {
        return 0;
    }
    for (o = gPlayers, i = 0; i < 10; i++, o++) {
        if (o->active == 0 || i == pl->index || o->area != pl->area) {
            continue;
        }
        if (Player_IsState(o, 0, 0xA) == 0 || o->offerTo != pl->index) {
            continue;
        }
        if (!(Vec_Distance(&pl->pos[0], o->pos) <= 300.0f)) {
            continue;
        }
        if (gItemData[o->offerItem].rarity > 2) {
            continue;
        }
        if (Area_IsOnlineHouse(pl->area) &&
            (u8)gLobby->memberSlot[o->index] != (u8)gLobby->memberSlot[pl->index]) {
            continue;
        }
        if (SavePouch_Count(o->offerItem) == 0) {
            if (SavePouch_HasFreeSlot() != 0) {
                pl->offerItem = o->offerItem;
                pl->offerCount = o->offerCount;
                if (Player_IsLocal(pl) == 1) {
                    TownMsg_Send0B(&gLobby->members[i], o);
                }
                Actor_SetStateNow(pl, 0, 0xB, 0);
            } else {
                Notice_PushItem((char *) gPouchFullTakeText);
                pl->offerNoticeTimer = 0x5A;
            }
        } else if (SavePouch_Room(o->offerItem) >= o->offerCount) {
            pl->offerItem = o->offerItem;
            pl->offerCount = o->offerCount;
            if (Player_IsLocal(pl) == 1) {
                TownMsg_Send0B(&gLobby->members[i], o);
            }
            Actor_SetStateNow(pl, 0, 0xB, 0);
        } else {
            Notice_Push(1, 0xF, (s16)o->offerItem);
            pl->offerNoticeTimer = 0x5A;
        }
        return 1;
    }
    return 0;
}


void TownPc_State0A(Player *pl)
{
    pl->offerStarted = 1;
    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    switch (pl->step) {
    case 0:
        pl->step++;
        Actor_SetAction(pl, 0, 0, 0);
        Player_SetMotionPair(pl, 0x3C, 4, 0);
        pl->offerState = 0;
        pl->freeState = 0;
        break;
    case 1:
        if (Player_IsLocal(pl) == 1 && ((pl->input.pressed & 0x40) || pl->offerState != 0 || *(s8 *)&gNetUsers[pl->offerTo * 0x11C] == 0)) {
            Actor_SetStateNow(pl, 0, 0xC, 0);
        }
        break;
    }
}

void TownPc_State0B(Player *pl)
{
    s32 timer;
    u8 step;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        if (pl->motionId[0] != 1) {
            Player_SetMotionPair(pl, 1, 6, 0);
        }
        pl->offerState = 0U;
        pl->timer = 0;
        break;
    case 1:
        timer = pl->timer + 1;
        pl->timer = timer;
        if ((timer >= 0x259) || (pl->offerState != 0)) {
            if (pl->offerState == 1) {
                Actor_SetStateNow((void *) pl, 0, 0xD, 0);
                break;
            }
            Actor_SetIdle((void *) pl, 0, 2, 0);
            if (pl->index == gScene.localPlayer) {
                TownMsg_Send12();
            }
        } else {
        }
        break;
    }
}

void TownPc_State0C(Player *pl)
{
    u8 step;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0x3D, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_SetIdle((void *) pl, 0, 6, 0);
        }
        break;
    }
}

void TownPc_State0D(Player *pl)
{
    u8 step;

    pl->offerStarted = 1;
    ItemGet_Reserve((s32)pl, (u16 *)&pl->gotItem);
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 0, 0, 0);
        Player_UpdateMotionTime(pl, 0);
        Player_SetMotionPair(pl, 0x280, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_SetIdle((void *) pl, 0, 6, 0);
        }
        break;
    }
}

void TownPc_State0E(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 0, 0, 0);
        pl->motionTime = 0;
        Actor_SetMotion(pl, 0x1AA, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            Actor_SetStateNow((void *) pl, 0, 0xF, 0);
        }
        break;
    }
}

extern Vec3f D_00802C50;
void Effect06_SpawnAt(void *obj, s16 type, u8 bone, f32 *pos, f32 scale);
s32 TownPc_IsInMyArea(Player *p);

void TownPc_LoopMotion1AB(Player *pl)
{
    Vec3f v = D_00802C50;

    if (pl->motionId[0] == 0x1AB && ((u16)Rand_Next(1) & 0x3F) == 0) {
        Actor_SetMotion(pl, 0x1AC, 0, 0);
    } else if (pl->motionId[0] == 0x1AC && pl->motion[0].playing == 0) {
        Actor_SetMotion(pl, 0x1AB, 0, 0);
    }
    if (TownPc_IsInMyArea(pl) != 0) {
        switch (gScene.frame % 60) {
        case 0:
        case 10:
        case 20:
            Effect06_SpawnAt(pl, 4, 0x14, (f32 *) &v, 0.6f);
            break;
        }
    }
}

u16 LobbyPad_Get(s32 a);

void TownPc_State0F(Player *pl, s32 k)
{
    u8 step;

    LobbyPad_Get(0);
    if (pl->index == gScene.localPlayer) {
        switch (gTownMode) {
        case 0x23:
        case 0x24:
        case 0x25:
            TownPc_LoopMotion1AB(pl);
            return;
        }
    }
    step = pl->step;
    switch (step) {
    case 0:
        pl->step = step + 1;
        Actor_SetAction(pl, 0, 0, 0);
        pl->motionTime = 0;
        Actor_SetMotion(pl, 0x1AB, 0, 0);
        pl->timer = 0x3C;
        break;
    case 1:
        TownPc_LoopMotion1AB(pl);
        break;
    }
}

void TownPc_State10(Player *pl, s32 unused)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 0, 0, 0);
        pl->motionTime = 0;
        pl->rot[1] = (s32) ((pl->rot[1] + 0x7FFF + 1) & 0xFFFF);
        pl->targetYaw = (s16) pl->rot[1];
        Actor_SetMotion(pl, 0x1AD, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            if (pl->index == gScene.localPlayer) {
                gTownMode = 0;
            }
            Actor_SetIdle((void *) pl, 0, 6, 0);
        }
        break;
    }
}

void TownPc_State11(Player *pl)
{
    u8 step;

    if (pl->unk844 != 0) {
        Actor_SetState(pl, 0, 4);
    } else {
        step = pl->step;
        switch (step) {
        case 0:
            pl->step = (u8) (step + 1);
            pl->motionTime = 0;
            pl->rot[1] = (s32) pl->targetYaw;
            Actor_SetMotion(pl, 0x2A, 0, 0);
            Actor_SetAction(pl, 0, 0, 0);
            Actor_SetFlag((void *) pl, 0x1000);
            break;
        case 1:
            if (pl->motion[0].playing == 0) {
                Actor_SetStateNow((void *) pl, 0, 4, 0);
            }
            break;
        }
    }
}

void TownPc_State12(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        if (pl->motionId[0] != 0x101) {
            Actor_SetMotion(pl, 0x101, 0x10, 0x1E);
        }
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
        }
        TownPc_IdleCheckOffer(pl);
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && ((s32) pl->input.stickTilt >= 0x28)) {
            Actor_SetState(pl, 0, 0x13U);
        }
        break;
    }
}

void TownPc_State13(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        Actor_SetMotion(pl, 0x102, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 0xD8, 4, 0x36);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Actor_SetStateNow((void *) pl, 0, 0x1D, 0);
            break;
        }
        TownPc_IdleCheckOffer(pl);
        break;
    }
}

s32 TownPc_IsSeatedState(u8 group, u8 state)
{
    if (group & 0xFF) {
        return 0;
    }
    switch (state & 0xFF) {
    case 0x14:
    case 0x1C:
    case 0x21:
    case 0x22:
    case 0x24:
    case 0x25:
    case 0x26:
    case 0x28:
    case 0x16:
    case 8:
    case 7:
    case 9:
    case 0x29:
    case 0x1E:
        return 1;
    }
    return 0;
}

void TownPc_State9A(Player *pl)
{
    u8 step;

    if (pl->index == gScene.localPlayer) {
        step = pl->step;
        switch (step) {
        case 0:
            Actor_SetAction(pl, 1, 0, 0);
            Actor_SetMotion(pl, 0x260, -4, 0);
            pl->step = (u8) (pl->step + 1);
            break;
        case 1:
            if (pl->motion[0].playing == 0) {
                pl->step = (u8) (step + 1);
                Actor_SetMotion(pl, 0x261, 4, 0);
            }
            break;
        }
    }
}

void LobbyBgm_SetUnk11F(s32 on);
s32 LobbyBgm_GetTrack(void);
s32 func_007D98B0(void);
void Kitchen_Open(void);
s32 Area_Is86to88(s32 area);
extern u8 D_008BEB63;

/* Sitting down at a table (kind 1 sits at once): the local player then
 * opens the kitchen, and the pad (state 2) stands up or picks an action. */
void TownPc_State14(Player *pl, s32 k)
{
    u16 pad;

    switch (pl->step) {
    case 0:
        pl->step++;
        pl->motionTime = 0;
        Actor_SetAction(pl, 1, 0, 0);
        pl->weaponOut = 0;
        if (k == 1) {
            pl->step = 2;
            pl->timer = 0xE10;
            pl->timer = 0x3C;
            if (pl->motionId[0] != 0x261) {
                Actor_SetMotion(pl, 0x261, 6, 0);
            }
        } else {
            Actor_SetMotion(pl, 0x260, -4, 0);
            if (pl->index == gScene.localPlayer) {
                gFieldControl = 0;
                D_008BEB63 = 1;
                if (gScene.area == 0x87) {
                    LobbyBgm_SetUnk11F(1);
                }
            }
        }
        break;
    case 1:
        if (pl->motion[0].playing == 0) {
            pl->timer = 0xE10;
            pl->timer = 0x3C;
            pl->step++;
            Actor_SetMotion(pl, 0x261, 4, 0);
            pl->state = 0x25;
            if (pl->index == gScene.localPlayer) {
                if (gTownMode != 0x32) {
                    gFieldControl = 1;
                }
                if ((Game_IsOnline() == 1 && MyHouse_IsOwner() == 1) ||
                    (Game_IsOnline() == 0 && gScene.area != 0x89)) {
                    if (gTownMode == 0 && gLobby->mealEaten == 0) {
                        Kitchen_Open();
                    }
                }
            }
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            if ((pl->input.pressed & 0x20) && gTownMode == 0) {
                Actor_SetStateEx(pl, 0, 0x15, 0);
                CamFocus_End();
                if (gScene.area == 0x87) {
                    LobbyBgm_SetUnk11F(0);
                }
            } else if (Area_Is86to88(gScene.area) == 1 && gTownMode == 0) {
                pad = pl->input.pressed;
                if (pad & 0x40) {
                    Actor_SetState(pl, 0, 0x22);
                } else if (pad & 0x200) {
                    Actor_SetState(pl, 0, 0x21);
                } else if (pad & 0x2000) {
                    Actor_SetState(pl, 0, 0x28);
                } else if (pad & 0x1000) {
                    Actor_SetState(pl, 0, 0x26);
                } else if (pad & 0x800) {
                    Actor_SetState(pl, 0, 0x24);
                } else if (pad & 0x400) {
                    Actor_SetState(pl, 0, 0x1C);
                } else if (gScene.area == 0x87 && LobbyBgm_GetTrack() != -1 && func_007D98B0() == 0) {
                    LobbyBgm_SetUnk11F(1);
                }
            }
        }
        break;
    }
}

s32 TownPc_IsLeaveSeatState(s32 a, s32 b)
{
    if (a == 0) {
        switch (b) {
        case 0x15:
        case 0x27:
        case 0x2A:
        case 0x18:
            return 1;
        }
    }
    return 0;
}

void TownPc_State15(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        Actor_SetMotion(pl, 0x262, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetIdle((void *) pl, 0, 4, 0);
        }
        break;
    }
}

void TownPc_State17(Player *pl)
{
    s32 timer;
    u8 step;

    if (pl->index != gScene.localPlayer) {
        Actor_SetState(pl, 0, 1);
    } else {
        step = pl->step;
        switch (step) {
        case 0:
            pl->step = (u8) (step + 1);
            Actor_SetAction(pl, 0, 0, 0);
            pl->motionTime = 0;
            if (pl->motionId[0] != 0x1AB) {
                Actor_SetMotion(pl, 0x1AB, 0, 0);
            }
            pl->timer = 0x168;
            break;
        case 1:
            TownPc_LoopMotion1AB(pl);
            timer = pl->timer - 1;
            pl->timer = timer;
            if (timer <= 0) {
                Actor_SetStateNow((void *) pl, 0, 0x10, 0);
            }
            break;
        }
    }
}

void Jingle_Play(void *obj, s32 id);
s32 Motion_PassedFrame(void *obj, s32 layer, f32 frame);
void Player_ClearFlag(void *obj, u32 flag);
extern u8 gOffersRefreshPending;

/* Sleeping (k 0 goes to bed, k 1 wakes up in it): the motions, the saved
 * bed position (+0x438 +0x14, angle +0x22), the jingle, then back to state
 * 0 at the bed (in area 0x57, turned around). Other players only idle. */
/* A lobby player's block at Player +0x438 (the gap unk438), as TownPc_State19 (the bed) reads it. */
typedef struct TownPcWork {
    u8 unk00[0x14];
    Vec3f bedPos;               /* 0x14: the position kept while the player sleeps: saved at frames 166 and 260 of the sleep motion, put back at frame 198 and in step 3 */
    s16 jingleTimer;            /* 0x20: step 4 in area 0x57: frames until the jingle (0xA0) */
    u16 bedYaw;                 /* 0x22: the yaw kept with bedPos */
    u8 unk24[0x80 - 0x24];
} TownPcWork;
typedef char TownPcWork_size_check[(sizeof(TownPcWork) == 0x80) ? 1 : -1];

void TownPc_State19(Player *pl, s8 k)
{
    TownPcWork *w = (TownPcWork *)pl->unk438;

    if (pl->index != gScene.localPlayer) {
        Actor_SetState(pl, 0, 1);
        return;
    }
    switch (pl->step) {
    case 0:
        pl->fieldCollide = 0;
        pl->step++;
        w->jingleTimer = 0;
        if (gScene.area == 0x57) {
            pl->step = 4;
            if (k == 0) {
                Actor_SetMotion(pl, 0x287, 0, 0);
            } else {
                Actor_SetMotion(pl, 0x288, 0, 0);
            }
        } else if (k == 0) {
            Actor_SetMotion(pl, 0xFA, 0, 0);
            pl->timer = 2;
        } else {
            Actor_SetMotion(pl, 0xFB, 0, 0);
            pl->step = 4;
        }
        break;
    case 1:
        if (Motion_ReachedFrame(pl, 0, 198.0f)) {
            Actor_SetMotion(pl, 0xFA, 0, 0xA6);
            *(Vec3f *)pl->pos = w->bedPos;
            *(Vec3f *)pl->moveTo = *(Vec3f *)pl->pos;
            pl->targetYaw = pl->rot[1] = w->bedYaw;
            if (--pl->timer <= 0) {
                pl->step++;
            }
        } else if (Motion_PassedFrame(pl, 0, 166.0f) && pl->timer == 2) {
            w->bedPos = *(Vec3f *)pl->pos;
            w->bedYaw = pl->rot[1];
        }
        break;
    case 2:
        if (Motion_PassedFrame(pl, 0, 230.0f)) {
            Jingle_Play(pl, 9);
        }
        if (Motion_ReachedFrame(pl, 0, 260.0f)) {
            pl->step++;
            pl->timer = 0x3C;
            w->bedPos = *(Vec3f *)pl->pos;
            w->bedYaw = pl->rot[1];
        }
        break;
    case 3:
        if (--pl->timer <= 0) {
            pl->step++;
        } else {
            Actor_SetMotion(pl, 0xFA, 0, 0x104);
            *(Vec3f *)pl->pos = w->bedPos;
            *(Vec3f *)pl->moveTo = *(Vec3f *)pl->pos;
            pl->targetYaw = pl->rot[1] = w->bedYaw;
        }
        break;
    case 4:
        if (k == 0 && gScene.area == 0x57) {
            w->jingleTimer++;
            if (w->jingleTimer == 0xA0) {
                Jingle_Play(pl, 9);
            }
        }
        if (pl->motion[0].playing <= 0) {
            gFieldControl = 1;
            pl->fieldCollide = 1;
            gOffersRefreshPending = 1;
            Actor_SetIdle(pl, 0, 0, 0);
            if (pl->index == gScene.localPlayer) {
                gTownMode = 0;
            }
            if (gScene.area == 0x57) {
                Player_ClearFlag(pl, 0x20000);
                pl->rot[1] = pl->targetYaw = pl->rot[1] + 0x8000;
            } else if (k == 0) {
                pl->pos[0] = 5045.0f;
                pl->pos[1] = 202.0f;
                pl->pos[2] = 6479.0f;
                pl->rot[1] = pl->targetYaw = 0x271C;
            } else {
                pl->pos[0] = 4847.5f;
                pl->pos[1] = -1.19f;
                pl->pos[2] = 7061.75f;
                *(Vec3f *)pl->moveTo = *(Vec3f *)pl->pos;
                pl->rot[1] = pl->targetYaw = 0x72A2;
            }
        }
        break;
    }
}

void TownPc_State1B(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0xCF, 2, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetIdle(pl, 0, 6, 0);
        }
        break;
    }
}

void TownPc_State1C(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x26A, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
        }
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && (pl->input.pressed & 0x3C60)) {
            Actor_SetState(pl, 0, 9U);
        }
        break;
    }
}

void TownPc_State09(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x282, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownPc_State1D(Player *pl)
{
    u16 motion;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        pl->motionTime = 0;
        motion = pl->motionId[0];
        if (motion == 0xD8) {
            pl->step = 2U;
            Actor_SetMotion(pl, 0x15, 4, 0);
            break;
        }
        if (motion != 1) {
            Actor_SetMotion(pl, 1, 8, 0);
        }
        break;
    case 1:
        if (pl->motion[0].blend == 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 0x15, 4, 0);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 5, 4, 0x90);
        }
        TownPc_CheckAction((void *) pl);
        break;
    case 3:
        if (pl->motionTime >= 0xF0) {
            Actor_SetState(pl, 0, 0x12U);
            break;
        }
        TownPc_CheckAction((void *) pl);
        break;
    }
}

void TownPc_State1E(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x25A, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownPc_State1F(Player *pl)
{
    u8 step;
    u8 localPlayer;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x25C, 0, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 0x32A, 4, 0x2C);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 0xD8, 6, 0x24);
        }
        break;
    case 3:
        if (pl->motion[0].playing <= 0) {
            Actor_SetIdle(pl, 0, 8, 0);
            if (gScene.localPlayer == pl->index) {
                CamFocus_End();
                gTownMenuOpen = 0;
                localPlayer = gScene.localPlayer;
                if (pl->index == localPlayer) {
                    gTownMode = 0;
                }
                TownMsg_SendStateNow((void *)((s32) &gPlayers[localPlayer & 0xFF]));
            }
        }
        break;
    }
}

void TownPc_State20(Player *pl)
{
    switch (pl->step) {
    case 0:
        Actor_SetMotion(pl, 0x25B, 6, 0);
        pl->step++;
    case 1:
        break;
    }
}

void TownPc_State21(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        func_00796DE0((void *) pl);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x263, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownPc_State22(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x264, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownPc_State16(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x269, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownPc_State24(Player *pl)
{
    s32 timer;
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x268, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->timer = 0x3C;
            pl->step = (u8) (pl->step + 1);
        }
        break;
    case 2:
        if (Player_IsLocal(pl) == 1) {
            timer = pl->timer;
            if (timer <= 0) {
                if (pl->input.pressed & 0x3C60) {
                    Actor_SetState(pl, 0, 0x16U);
                    break;
                }
            } else {
                pl->timer = (s32) (timer - 1);
            }
        }
        break;
    }
}

void TownPc_State26(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x265, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
        }
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && (pl->input.pressed & 0x3C60)) {
            Actor_SetState(pl, 0, 7U);
        }
        break;
    }
}

void TownPc_State07(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x266, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownMsg_Send17(void *pl);
void Effect06_SpawnAtBone(Player *p, s16 type, u8 sub, s16 bone, f32 scale);

void TownPc_State27(Player *pl, s8 flag)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        pl->motionTime = 0;
        Actor_SetAction(pl, 0, 0, 0);
        Actor_SetMotion(pl, 0x262, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            if (flag == 0) {
                Actor_SetMotion(pl, 0x27D, 6, 0);
                break;
            }
            Actor_SetMotion(pl, 0x197, 6, 0x14);
            Effect06_SpawnAtBone(pl, 0, 8, 0xAU, 4.0f);
        }
        break;
    case 2:
        if (pl->motion[0].playing <= 0) {
            Actor_SetIdle(pl, 0, 6, 0);
            if (pl->index == gScene.localPlayer) {
                TownMsg_Send17(pl);
            }
        }
        break;
    }
}

void TownPc_State28(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x267, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
        }
        break;
    case 2:
        if ((Player_IsLocal(pl) == 1) && (pl->input.pressed & 0x3C60)) {
            Actor_SetState(pl, 0, 8U);
        }
        break;
    }
}

void TownPc_State08(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetAction(pl, 1, 0, 0);
        Actor_SetMotion(pl, 0x283, 8, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            Actor_SetMotion(pl, 0x261, 6, 0);
            Actor_SetState(pl, 0, 0x25U);
        }
        break;
    }
}

void TownPc_State29(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x27B, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 0x261, 6, 0);
        }
        
    case 2:
        break;
    }
}

void TownPc_State2A(Player *pl)
{
    u8 step = pl->step;

    switch (step) {
    case 0:
        pl->step = step + 1;
        Actor_SetMotion(pl, 0x27C, 4, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = step + 1;
            Actor_SetMotion(pl, 0xD2, 0, 0x92);
            pl->targetYaw += 0x4000;
            pl->rot[1] += 0x4000;
            pl->timer = ((u16)Rand_Next(1) & 0x1F) + 0x3C;
        }
        break;
    case 2:
        if (--pl->timer <= 0 && gScene.localPlayer == pl->index) {
            Actor_SetState(pl, 0, 0x1B);
            TownMsg_Send17(pl);
        }
        break;
    }
}

void TownPc_State2B(Player *pl)
{
    u8 step;

    step = pl->step;
    switch (step) {
    case 0:
        pl->step = (u8) (step + 1);
        Actor_SetMotion(pl, 0x25D, 6, 0);
        break;
    case 1:
        if (pl->motion[0].playing <= 0) {
            pl->step = (u8) (step + 1);
            Actor_SetMotion(pl, 0x25E, 0, 0);
            break;
        }
    case 2:
        break;
    case 3:
        if (pl->motion[0].playing <= 0) {
            Actor_SetIdle(pl, 0, 4, 0);
        }
        break;
    }
}

void Vec_Copy(f32 *dst, f32 *src);
void Actor_StartWalk(Player *obj, s16 n);

/* The player walks to its action spot (Player +0x7D0), facing the spot's angle. */
void TownPc_WalkToSpot(Player *pl)
{
    f32 v[3];
    u16 m;

    v[0] = ((ActSpot *)(pl)->spot)->x;
    v[1] = ((ActSpot *)(pl)->spot)->y;
    v[2] = ((ActSpot *)(pl)->spot)->z;
    m = ((ActSpot *)(pl)->spot)->angle;
    Vec_Copy(&pl->moveTo[0], (f32 *) v);
    Actor_StartWalk(pl, 0x14);
    pl->targetYaw = m;
    gTownMenuOpen = 1;
}

void TownMsg_Send1D(Player *arg0);
void TownMsg_Send02(Player *obj);
extern u8 gTownSpotId;

/* In area 0x86 (the action 0x36): the local player walks to the spot of
 * kind 0xE for gTownSpotId and takes state 0x70 or 0x71 there; without the
 * action it is cancelled. */
void Contest_TakeSeat(void)
{
    ActSpot *s = (ActSpot *)ActSpot_GetList(gScene.area);
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 i;
    f32 v[3];
    u16 ang;

    if (gScene.area != 0x86) {
        return;
    }
    if (gTownMode != 0x36) {
        gTownMenuOpen = 0;
        TownMsg_Send1D(pl);
        gTownMode = 0;
        gFieldControl = 1;
        return;
    }
    for (i = 0; 0 < s->kind && s != NULL;) {
        if (s->kind == 0xE && s->id == gTownSpotId) {
            break;
        }
        i++;
        s++;
        if (i >= 0x14 || s == NULL || s->kind == 0) {
            gTownMenuOpen = 0;
            gTownMode = 0;
            Actor_SetIdle(pl, 0, 0x14, 0);
            return;
        }
    }
    gFieldControl = 0;
    gTownMode = 0x37;
    v[0] = s->x;
    v[1] = s->y;
    v[2] = s->z;
    ang = s->angle;
    Vec_Copy(pl->moveTo, v);
    Actor_StartWalk(pl, 0x14);
    *(Vec3f *)v = *(Vec3f *)pl->pos;
    *(Vec3f *)pl->pos = *(Vec3f *)pl->moveTo;
    TownMsg_Send02(pl);
    *(Vec3f *)pl->pos = *(Vec3f *)v;
    pl->targetYaw = ang;
    if (gTownSpotId == 1) {
        Actor_SetState(pl, 0, 0x70);
    } else {
        Actor_SetState(pl, 0, 0x71);
    }
}

void TownMsg_Send07(Player *pl);
s32 Area_Is86to88(s32 area);
extern u8 gTakeSpotId;

/* The action 0x19 (the local player walking to the spot gTakeSpotId of
 * kind 1, or 1/0xA in other areas); without the action it is cancelled. */
void TownPc_TakeSpot(void)
{
    ActSpot *s = (ActSpot *)ActSpot_GetList(gScene.area);
    Player *pl = &gPlayers[gScene.localPlayer];
    u16 i;
    f32 v[3];
    u16 ang;

    if (Area_Is86to88(gScene.area) == 1) {
        if (gTownMode != 0x19) {
            gTownMenuOpen = 0;
            if (Area_IsOnlineHouse(gScene.area) == 1) {
                TownMsg_Send17(pl);
            } else {
                TownMsg_Send07(pl);
            }
            gTownMode = 0;
            return;
        }
        for (i = 0; 0 < s->kind && s != NULL;) {
            if (s->kind == 1 && s->id == gTakeSpotId) {
                break;
            }
            i++;
            s++;
            if (i >= 0x14 || s == NULL || s->kind == 0) {
                return;
            }
        }
        TownMsg_Send02(pl);
    } else if (Game_IsOnline() == 0) {
        for (i = 0; 0 < s->kind && s != NULL;) {
            if ((s->kind == 1 || s->kind == 0xA) && s->id == gTakeSpotId) {
                break;
            }
            i++;
            s++;
            if (i >= 0x1E || s == NULL) {
                return;
            }
        }
    }
    gTownMenuOpen = 0;
    if (gTownMode != 0x32) {
        gTownMode = 0;
    }
    if (Area_Is86to88(gScene.area) == 0 && Game_IsOnline() == 1) {
        v[0] = ((ActSpot *)pl->spot)->x;
        v[1] = ((ActSpot *)pl->spot)->y;
        v[2] = ((ActSpot *)pl->spot)->z;
        ang = ((ActSpot *)pl->spot)->angle;
    } else {
        v[0] = s->x;
        v[1] = s->y;
        v[2] = s->z;
        ang = s->angle;
    }
    Vec_Copy(pl->moveTo, v);
    Actor_StartWalk(pl, 0x14);
    pl->targetYaw = ang;
    Actor_SetStateNow(pl, 0, 0x14, 0);
}

/* unk_7949D0's .data (0x801DE0-0x802C60) */
u32 gAreaSpawnLists_0[56] = {
#include "lobby/00801DE0.inc"
};
u32 gAreaSpawnLists_1[60] = {
#include "lobby/00801EC0.inc"
};
u32 gAreaSpawnLists_2[24] = {
#include "lobby/00801FB0.inc"
};
u32 gAreaSpawnLists_3[128] = {
#include "lobby/00802010.inc"
};
u32 gAreaSpawnLists_4[92] = {
#include "lobby/00802210.inc"
};
u32 gAreaSpawnLists_5[104] = {
#include "lobby/00802380.inc"
};
u32 gAreaSpawnLists_14[36] = {
#include "lobby/00802520.inc"
};
u32 gAreaSpawnLists_10[36] = {
#include "lobby/008025B0.inc"
};
u32 gAreaSpawnLists_6[108] = {
#include "lobby/00802640.inc"
};
u32 gAreaSpawnLists_7[20] = {
#include "lobby/008027F0.inc"
};
u32 gAreaSpawnLists_11[44] = {
#include "lobby/00802840.inc"
};
u32 gAreaSpawnLists_12[68] = {
#include "lobby/008028F0.inc"
};
u32 gAreaSpawnLists_13[128] = {
#include "lobby/00802A00.inc"
};
s32 gAreaSpawnLists[16] = {
    (s32)gAreaSpawnLists_0, (s32)gAreaSpawnLists_1, (s32)gAreaSpawnLists_2, (s32)gAreaSpawnLists_3,
    (s32)gAreaSpawnLists_4, (s32)gAreaSpawnLists_5, (s32)gAreaSpawnLists_6, (s32)gAreaSpawnLists_7,
    (s32)gAreaSpawnLists_7, (s32)gAreaSpawnLists_7, (s32)gAreaSpawnLists_10, (s32)gAreaSpawnLists_11,
    (s32)gAreaSpawnLists_12, (s32)gAreaSpawnLists_13, (s32)gAreaSpawnLists_14, 0,
};
s32 D_00802C40[4] = {
#include "lobby/00802C40.inc"
};
Vec3f D_00802C50 = {
#include "lobby/00802C50.inc"
};
