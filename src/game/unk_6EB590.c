/*
 * Game overlay code at 0x6EB590-0x6EC680, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "em.h"
#include "objattach.h"
#include "player.h"
#include "sprite.h"
#include "capsule.h"
/* prototypes of callees (round 13) */
void Vec_Copy(f32 *out, f32 *in);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

/* prototypes */
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
s32 Player_TestSetFlags(void *, s32 mask);
f32 EffKushala39_UnpackFloat(u16 a, u16 b);
void EffKushala39_Draw(DrawSlot *d);
void Vec_RotateY(f32 *v, f32 angle);
extern void *EffKushala39_MotionDesc;
extern HitShape *EffKushala39_HitList;
extern f32 EffKushala39_Angles[];
extern f32 EffKushala39_Angles1[];
EmBone *Em_GetBone(void *obj, s16 bone);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
f32 Math_Absf(f32 x);
extern void *EffGunlance38_MotionDesc;
extern u8 EffGunlance38_Power3[];
extern u8 EffGunlance38_Power[];
extern s8 EffKushala39_Life[];
f32 Lure_RandUnit(void);
f32 Lure_RandSigned(void);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
ObjAttach *ObjAttach_Alloc(s32 n);
u8 Player_InCurrentArea(void *obj);
void EffKushala39_Update(ObjAttach *o);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_Identity(f32 *m);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Mtx_GetTrans(f32 *out, f32 *m);
extern HitShape *gEffGunlance38HitLists[];
void EffGunlance38_UpdatePos(ObjAttach *o, Player *pl);
void EffGunlance38_End(ObjAttach *o);
void EffGunlance38_Free(ObjAttach *o);
void EffKushala39_End(ObjAttach *o);
void EffKushala39_Free(ObjAttach *o);
void ObjAttach_Free(ObjAttach *p);
void DrawSlot_Free(s32);
void EffGunlance38_Run(ObjAttach *o);
void EffKushala39_Run(ObjAttach *o);
void EffGunlance38_Setup(ObjAttach *o);
void EffKushala39_Setup(ObjAttach *o);
/* end of prototypes */

void EffGunlance38_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffGunlance38_Setup(o);
        break;
    case 1:
        EffGunlance38_Run(o);
        break;
    case 2:
        EffGunlance38_End(o);
        break;
    case 3:
        o->state = 1;
        o->hitFrames += 2;
        EffGunlance38_Run(o);
        break;
    case 4:
        EffGunlance38_End(o);
        break;
    case 5:
        EffGunlance38_End(o);
        break;
    case 9:
        EffGunlance38_End(o);
        break;
    case 7:
        EffGunlance38_End(o);
        break;
    case 6:
        EffGunlance38_Free(o);
        
    default:
        break;
    }
}

void EffGunlance38_UpdatePos(ObjAttach *o, Player *pl)
{
    f32 in[3];
    f32 out[3];
    f32 mb[16];
    f32 m[16];

    Mtx_Copy((f32 *) mb, (f32 *) o->buf);
    Mtx_Identity(m);
    Mtx_SetRotXYZ(m, 0.06283185f, -0.5410521f, -3.047345f);
    Mtx_Concat3x3(m, mb);
    in[0] = o->hitList->a[0];
    in[1] = o->hitList->a[1];
    in[2] = o->hitList->a[2];
    Vec_Rotate(out, in, m);
    Mtx_GetTrans(o->pos2, mb);
    o->pos2[0] += out[0];
    o->pos2[1] += out[1];
    o->pos2[2] += out[2];
    in[0] = o->hitList->b[0];
    in[1] = o->hitList->b[1];
    in[2] = o->hitList->b[2];
    Vec_Rotate(out, in, m);
    Mtx_GetTrans(o->pos, mb);
    o->pos[0] += out[0];
    o->pos[1] += out[1];
    o->pos[2] += out[2];
}

static void EffGunlance38_ScaleLife(ObjAttach *o, f32 scale)
{
    f32 v = o->power;

    o->power = v * scale;
    o->elementPower = (u8) v - o->power;
    o->elementFlags = 0x10;
}

void EffGunlance38_Setup(ObjAttach *o)
{
    u8 *b = o->buf;
    Player *pl = &gPlayers[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->aux = 0;
    Mtx_Copy((f32 *) b, (f32 *) (Em_GetBone(pl, 14)));
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) pl, o->type, &EffGunlance38_MotionDesc);
    o->hitList = gEffGunlance38HitLists[o->hitListNo];
    o->hurtList = 0;
    o->area = pl->area;
    if (o->hitList->bone == 0x7E) {
        EffGunlance38_UpdatePos(o, pl);
    }
    switch (o->type) {
    case 1:
    case 2:
    case 3:
        o->power = EffGunlance38_Power3[pl->bladeSpecial];
        if (pl->sharpnessLevel <= 1) {
            o->power = 0.75f * o->power;
        }
        EffGunlance38_ScaleLife(o, 0.5f);
        break;
    default:
        o->power = EffGunlance38_Power[pl->bladeSpecial / 3];
        EffGunlance38_ScaleLife(o, 0.4f);
        break;
    }
    if (o->hitDelay != 0) {
        a = Math_Absf(pl->motionBlend[0] % 100);
        o->hitDelay += a - (s32) (Math_Absf(*(u16 *) &pl->motionFrame[0]) / 2.0f) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
}

static void EffGunlance38_PauseInHitStop(Em *em)
{
    Player *p;

    p = &gPlayers[*(u8 *)((u8 *)em + 0xA)];
    if ((p->hitStop != 0) || (p->hitSlowTimer != 0)) {
        ((ObjAttach *) em)->hitFrames++;
    }
}

void EffGunlance38_Run(ObjAttach *o)
{
    Player *pl = &gPlayers[o->idx];

    switch (o->type) {
    case 1:
    case 2:
    case 3:
        o->aux++;
        if (o->aux >= 4 && o->stateVar0 == 0) {
            o->stateVar0++;
            o->hitListNo += 4;
            o->hitList = gEffGunlance38HitLists[o->hitListNo];
            EffGunlance38_UpdatePos(o, pl);
        }
        break;
    case 4:
        break;
    }
    EffGunlance38_PauseInHitStop((Em *) o);
    if (pl->motionId[0] != o->ownerMotion) {
        o->hitPhase = 0;
        o->state = 2;
    }
}

void EffGunlance38_End(ObjAttach *o)
{
    o->state = 6;
    o->visible = 0;
}

void EffGunlance38_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffKushala39_Create(Em *em, u8 kind)
{
    ObjAttach *o;

    if ((Player_InCurrentArea(em) & 0xFF) && kind <= 4) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->idx = em->index;
            o->isEm = em->isEm;
            o->area = em->area;
            o->kind = 39;
            o->type = kind;
            o->stateVar0 = 0;
            o->state = 0;
            o->update = EffKushala39_Update;
            o->owner = em;
            o->randBits = em->stateRand;
            o->yaw = ((Em *) o->owner)->rotY;
            o->ownerMotion = -7;
            o->side = *(u8 *) &em->side;
        }
    }
}

void EffKushala39_Spawn(ObjAttach *src, f32 *pos, u8 n)
{
    ObjAttach *o;

    if ((Player_InCurrentArea(src->owner) & 0xFF) && src->type <= 4) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->idx = src->idx;
            o->isEm = src->isEm;
            o->area = src->area;
            o->kind = 39;
            o->type = src->type;
            o->stateVar0 = n;
            o->state = 0;
            o->update = EffKushala39_Update;
            o->owner = src->owner;
            Vec_Copy(o->pos, pos);
            o->randBits = src->randBits;
            *(u16 *) &o->yaw = *(u16 *) &src->yaw;
            o->ownerMotion = 0;
            o->side = src->side;
        }
    }
}

void EffKushala39_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffKushala39_Setup(o);
        break;
    case 1:
        EffKushala39_Run(o);
        break;
    case 2:
        EffKushala39_End(o);
        break;
    case 3:
        EffKushala39_Run(o);
        break;
    case 4:
        EffKushala39_Run(o);
        break;
    case 5:
        EffKushala39_Run(o);
        break;
    case 9:
        EffKushala39_Run(o);
        break;
    case 7:
        EffKushala39_Run(o);
        break;
    case 6:
        EffKushala39_Free(o);
        
    default:
        break;
    }
}

void EffKushala39_Setup(ObjAttach *o)
{
    f32 v[3];
    f32 m[16];
    EffKushala39Buf *b = (EffKushala39Buf *) o->buf;
    f32 dist;
    f32 r;
    f32 a;
    f32 y = 0.0f;

    o->state = 1;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x200);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, 1, &EffKushala39_MotionDesc);
    o->hitList = EffKushala39_HitList;
    o->hurtList = 0;
    switch (o->type) {
    default:
        dist = 800.0f;
        a = EffKushala39_Angles[o->stateVar0];
        r = 400.0f;
        break;
    case 1:
        dist = 800.0f;
        a = EffKushala39_Angles1[o->stateVar0];
        r = 300.0f;
        break;
    case 2:
        dist = 1800.0f;
        a = EffKushala39_Angles1[o->stateVar0];
        r = 200.0f;
        break;
    case 3:
        dist = 400.0f;
        a = EffKushala39_Angles[o->stateVar0];
        r = 200.0f;
        break;
    }
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = dist;
    a += (360.0f * (u16) o->yaw) / 65536.0f;
    Vec_RotateY(v, (6.2831855f * a) / 360.0f);
    v[0] += r * EffKushala39_UnpackFloat(o->randBits * o->stateVar0, 8);
    v[1] += y;
    v[2] += r * EffKushala39_UnpackFloat(o->randBits * o->stateVar0, 14);
    b->phase = 0;
    b->rotZ = 0.5f + (65536.0f * (360.0f * Lure_RandUnit())) / 360.0f;
    b->scale = 0.01f * (100.0f + 20.0f * Lure_RandUnit());
    b->seDone = 0;
    if (o->stateVar0 <= 0) {
        o->hitDelay += 7;
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 1)));
        Mtx_GetTrans(o->pos, m);
        Vec_Copy(b->origin, o->pos);
    } else {
        Vec_Copy(b->origin, o->pos);
    }
    o->pos[0] += v[0];
    o->pos[1] += v[1];
    o->pos[2] += v[2];
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = EffKushala39_Draw;
    } else {
        ObjAttach_Free(o);
        return;
    }
    EffKushala39_Run(o);
}

void EffKushala39_Run(ObjAttach *o)
{
    EffKushala39Buf *b = (EffKushala39Buf *) o->buf;
    f32 n;

    if (o->drawSlotBuf == NULL) {
        o->state = 2;
        return;
    }
    if (++o->ownerMotion <= 0) {
        return;
    }
    if (o->ownerMotion > EffKushala39_Life[b->phase]) {
        switch (b->phase) {
        case 0:
            o->ownerMotion = 1;
            b->phase = 1;
            if (b->seDone == 0) {
                if (o->area == gScene.area) {
                    Em_PlaySeChecked(o->owner, 0x24, 0, o->pos, 15, 0);
                }
                b->seDone++;
            }
            if (o->stateVar0 < 7) {
                n = o->stateVar0 + 1;
                EffKushala39_Spawn(o, b->origin, n);
            }
            break;
        case 1:
            o->ownerMotion = 1;
            b->phase = 2;
            b->rotZ = 0.5f + (65536.0f * (360.0f * Lure_RandUnit())) / 360.0f;
            b->spin = 0.5f + (65536.0f * ((10.0f * Lure_RandSigned()) / EffKushala39_Life[2])) / 360.0f;
            b->rise = (100.0f + 100.0f * Lure_RandUnit()) / EffKushala39_Life[2];
            break;
        case 2:
        default:
            o->state = 2;
            return;
        }
    } else {
        switch (b->phase) {
        case 0:
        case 1:
            break;
        case 2:
            b->rotZ += b->spin;
            o->pos[1] += b->rise;
            break;
        }
    }
    Vec_Copy(o->drawSlotBuf->pos, o->pos);
    OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 0x40U, 0);
}

void EffKushala39_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* unk_6EB590's .data (0x8AE870-0x8AEC50) */
HitShape gEffGunlance38HitLists_0[2] = {
#include "game/008AE870.inc"
};
HitShape gEffGunlance38HitLists_1[2] = {
#include "game/008AE8C0.inc"
};
HitShape gEffGunlance38HitLists_2[2] = {
#include "game/008AE910.inc"
};
HitShape gEffGunlance38HitLists_3[2] = {
#include "game/008AE960.inc"
};
HitShape gEffGunlance38HitLists_4[2] = {
#include "game/008AE9B0.inc"
};
HitShape gEffGunlance38HitLists_5[2] = {
#include "game/008AEA00.inc"
};
HitShape gEffGunlance38HitLists_6[2] = {
#include "game/008AEA50.inc"
};
HitShape *gEffGunlance38HitLists[8] = {
    gEffGunlance38HitLists_0, gEffGunlance38HitLists_1, gEffGunlance38HitLists_2, gEffGunlance38HitLists_3,
    gEffGunlance38HitLists_4, gEffGunlance38HitLists_5, gEffGunlance38HitLists_6, 0,
};
u32 D_008AEAC0[30] = {
#include "game/008AEAC0.inc"
};
void *EffGunlance38_MotionDesc = (void *)D_008AEAC0;
u8 EffGunlance38_Power3[16] = {
#include "game/008AEB40.inc"
};
u8 EffGunlance38_Power[16] = {
#include "game/008AEB50.inc"
};
HitShape D_008AEB60[2] = {
#include "game/008AEB60.inc"
};
HitShape *EffKushala39_HitList = D_008AEB60;
u32 D_008AEBC0[12] = {
#include "game/008AEBC0.inc"
};
void *EffKushala39_MotionDesc = (void *)D_008AEBC0;
f32 EffKushala39_Angles[8] = {
#include "game/008AEC00.inc"
};
f32 EffKushala39_Angles1[8] = {
#include "game/008AEC20.inc"
};
s8 EffKushala39_Life[16] = {
#include "game/008AEC40.inc"
};
