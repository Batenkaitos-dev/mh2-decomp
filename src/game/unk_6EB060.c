/*
 * 0x6EB060-0x6EB590: two kinds of objects of sub_main's object list
 * (ObjAttach_Alloc allocates them; +0x20 is the update function), each tied to
 * a monster (+0xA: index into gEms). Eff37_Update is kind 0x25's
 * update: a switch on the object's state (+0x4), one handler per state.
 * The file owns its .rodata: the jump tables of EffEm02_Run and
 * Eff37_Update (0x8DE1F0-0x8DE240).
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "sprite.h"
#include "capsule.h"
/* prototypes of callees (round 13) */
ObjAttach *ObjAttach_Alloc(s32 n);
void EffGunlance38_Update(ObjAttach *o);
void Vec_Copy(f32 *out, f32 *in);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

void DrawSlot_Free(s32 i);
void ObjAttach_Free(ObjAttach *p);

void Eff37_Setup(ObjAttach *o);
void Eff37_Run(ObjAttach *o);
void Eff37_End(ObjAttach *o);
void Eff37_Free(ObjAttach *o);

s32 Player_TestSetFlags(void *, s32 mask);
struct MoveObj;
void MoveObj_StartMotionEntry(struct MoveObj *, s32 a, void *b);
extern void *Eff37_MotionDesc[];
extern HitShape *gEff37HitLists[];
void EffEm02_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 0:
    case 2:
        o->hitFrames = 99;
        if (em->stateGroup != 3) {
            o->hitPhase = 0;
        }
        if (em->unk473 == 0) {
            o->hitPhase = 0;
        }
        break;
    case 3:
    case 4:
    case 5:
        o->hitFrames = 99;
        break;
    case 1:
        o->yaw = em->rotY;
        if (o->hitFrames > 99) {
            o->power = 35;
        } else if (o->hitFrames > 50) {
            o->power = 25;
        } else if (o->hitFrames > 42) {
            o->power = 35;
        } else {
            o->power = 25;
        }
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0 || em->taskPhase >= 2) {
        o->hitPhase = 0;
        o->state = 2;
        return;
    }
}

void EffEm02_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != 0) {
        DrawSlot_Free(o->drawSlot);
    }
}

void EffEm02_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffEm02_Draw(ObjAttach *o)
{
}

void Eff37_Update(ObjAttach *o);

void Eff37_Create(f32 *pos, u8 k, u8 c)
{
    ObjAttach *obj = ObjAttach_Alloc(0);

    if (obj != NULL) {
        obj->kind = 0x25;
        obj->type = k;
        obj->update = Eff37_Update;
        obj->idx = 0;
        obj->isEm = 0;
        obj->ownerMotion = 0;
        obj->owner = NULL;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        obj->stateVar1 = c;
        obj->area = gScene.area;
    }
}

void Eff37_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        Eff37_Setup(o);
        break;
    case 1:
        Eff37_Run(o);
        break;
    case 2:
        Eff37_End(o);
        break;
    case 3:
        Eff37_Run(o);
        break;
    case 4:
        Eff37_End(o);
        break;
    case 5:
        Eff37_End(o);
        break;
    case 9:
        Eff37_End(o);
        break;
    case 7:
        Eff37_End(o);
        break;
    case 6:
        Eff37_Free(o);
        break;
    }
}

void Eff37_Setup(ObjAttach *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x80);
    MoveObj_StartMotionEntry((struct MoveObj *)o, o->type, Eff37_MotionDesc);
    o->hitList = gEff37HitLists[o->hitListNo];
    o->hurtList = 0;
    o->drawSlotBuf = NULL;
}

void Eff37_Run(ObjAttach *o)
{
    o->hitFrames = 99;
    if (++o->ownerMotion > o->stateVar1) {
        o->hitFrames = 0;
        o->state = 2;
        return;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void Eff37_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != 0) {
        DrawSlot_Free(o->drawSlot);
    }
}

void Eff37_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffGunlance38_Create(Player *p, s8 kind)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(1)) != NULL) {
        o->kind = 38;
        o->type = kind;
        o->update = EffGunlance38_Update;
        o->idx = p->index;
        o->isEm = p->isEm;
        o->ownerMotion = p->motionId[0];
        o->owner = p;
        o->yaw = p->rot[1];
        o->randBits = p->stateRand;
        o->stateVar0 = 0;
        o->stateVar1 = 0;
        o->rotX = p->rot[0];
        o->rotY = p->rot[1];
        o->rotZ = p->rot[2];
        Vec_Copy(o->pos, p->pos);
        o->side = p->side;
        o->unk080 = 0;
        p->hitPending = 0;
        p->attackHit800 = 0;
    }
}

/* unk_6EB060's .data (0x8AE7D0-0x8AE870) */
HitShape gEff37HitLists_0[2] = {
#include "game/008AE7D0.inc"
};
HitShape *gEff37HitLists[4] = {
    gEff37HitLists_0, 0, 0, 0,
};
u32 Eff37_MotionDesc_0[12] = {
#include "game/008AE830.inc"
};
void *Eff37_MotionDesc[1] = {
    (void *)Eff37_MotionDesc_0,
};
