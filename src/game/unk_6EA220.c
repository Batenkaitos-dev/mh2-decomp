/*
 * Game overlay code at 0x6EA220-0x6EB060: EffShakalaka34_*, EffKushala30b_*
 * and EffEm02_*.
 */
#include "common.h"
#include "em.h"
#include "objattach.h"
#include "player.h"
#include "sprite.h"
#include "capsule.h"

s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
void EffEm02_Update(ObjAttach *o);
void EffShakalaka34_Draw(ObjAttach *o);
void EffEm02_Draw(ObjAttach *o);
extern HitShape *gEffShakalaka34HitLists[];
extern void *EffShakalaka34_MotionDesc;
extern HitShape *gEffEm02HitLists[];
extern void *EffEm02_MotionDesc;
ObjAttach *ObjAttach_Alloc(s32 n);
void ObjAttach_Free(ObjAttach *p);
void EffShakalaka34_End(ObjAttach *o);
void EffShakalaka34_Free(ObjAttach *o);
void EffKushala30b_Setup(ObjAttach *o);
void EffKushala30b_End(ObjAttach *o);
void EffKushala30b_Free(ObjAttach *o);
void EffEm02_Run(struct ObjAttach *o);
void EffEm02_End(struct ObjAttach *o);
void EffEm02_Free(struct ObjAttach *o);
void DrawSlot_Free(s32);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Vec_Copy(f32 *out, f32 *in);
void EffShakalaka34_Update(ObjAttach *o);
void EffKushala30b_Update(ObjAttach *o);
u8 Player_InCurrentArea(void *obj);
s32 Player_TestSetFlags(void *, s32 mask);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
extern HitShape *gEffKushala30bHitLists[];
extern void *EffKushala30b_MotionDesc;
f32 Math_Absf(f32 x);
void EffShakalaka34_Setup(ObjAttach *o);
void EffEm02_Setup(ObjAttach *o);
void EffShakalaka34_Run(ObjAttach *o);
void EffKushala30b_Run(ObjAttach *o);

void EffShakalaka34_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 34;
            o->type = kind;
            o->update = EffShakalaka34_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffShakalaka34_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffShakalaka34_Setup(o);
        break;
    case 1:
        EffShakalaka34_Run(o);
        break;
    case 2:
        EffShakalaka34_End(o);
        break;
    case 3:
        EffShakalaka34_Run(o);
        break;
    case 4:
        EffShakalaka34_End(o);
        break;
    case 5:
        EffShakalaka34_End(o);
        break;
    case 9:
        EffShakalaka34_End(o);
        break;
    case 7:
        EffShakalaka34_End(o);
        break;
    case 6:
        EffShakalaka34_Free(o);
        
    default:
        break;
    }
}

void EffShakalaka34_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = EffShakalaka34_Draw;
    Player_TestSetFlags(o, 0x20);
    o->wasHit = 0;
    o->area = m->area;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffShakalaka34_MotionDesc);
    o->hitList = gEffShakalaka34HitLists[o->hitListNo];
    o->hurtList = 0;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffShakalaka34_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 2:
        switch (m->motionId[0]) {
        case 0x400:
            o->ownerMotion = 0x400;
            if (Motion_ReachedFrame(m, 0, 40.0f) == 0) {
                o->hitFrames = 99;
            } else {
                o->hitPhase = 0;
            }
            break;
        case 0x3FF:
        default:
            o->hitFrames = 99;
            break;
        }
        break;
    case 3:
        switch (m->motionId[0]) {
        case 0x3F5:
            o->hitFrames = 99;
            break;
        default:
            o->hitPhase = 0;
            break;
        }
        break;
    case 4:
        switch (m->motionId[0]) {
        case 0x405:
        default:
            o->hitPhase = 0;
            break;
        case 0x403:
        case 0x404:
            o->ownerMotion = m->motionId[0];
            o->hitFrames = 99;
            break;
        }
        break;
    case 5:
        switch (m->motionId[0]) {
        case 0x3F6:
            o->hitFrames = 99;
            break;
        default:
            o->hitPhase = 0;
            break;
        }
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0 || m->taskPhase >= 2) {
        o->hitPhase = 0;
        o->state = 2;
        return;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffShakalaka34_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffShakalaka34_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffShakalaka34_Draw(ObjAttach *o)
{
}

void EffKushala30b_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 30;
            o->type = kind;
            o->update = EffKushala30b_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = em->rotX;
            o->rotY = em->rotY;
            o->rotZ = em->rotZ;
            o->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->attackHit800 = 0;
        }
    }
}

void EffKushala30b_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffKushala30b_Setup(o);
        break;
    case 1:
        EffKushala30b_Run(o);
        break;
    case 2:
        EffKushala30b_End(o);
        break;
    case 3:
        EffKushala30b_Run(o);
        break;
    case 4:
        EffKushala30b_End(o);
        break;
    case 5:
        EffKushala30b_End(o);
        break;
    case 9:
        EffKushala30b_End(o);
        break;
    case 7:
        EffKushala30b_End(o);
        break;
    case 6:
        EffKushala30b_Free(o);
        
    default:
        break;
    }
}

void EffKushala30b_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffKushala30b_MotionDesc);
    o->hitList = gEffKushala30bHitLists[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffKushala30b_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 8:
        o->hitFrames = 99;
        break;
    case 9:
        switch (o->step) {
        case 0:
            if (m->motionId[0] == 0x3F6) {
                o->step++;
                o->hitFrames = 31;
            } else {
                o->hitFrames = 99;
            }
            break;
        case 1:
            break;
        }
        break;
    case 25:
        o->hitFrames = 99;
        break;
    case 26:
        switch (o->step) {
        case 0:
            if (m->motionId[0] == 0x3F6) {
                o->step++;
                o->hitFrames = 1;
            } else {
                o->hitFrames = 99;
            }
            break;
        case 1:
            break;
        }
        break;
    case 29:
        switch (o->step) {
        case 0:
            if (m->motionId[0] == 0x409) {
                o->step++;
                o->hitFrames = 32;
            } else {
                o->hitFrames = 99;
            }
            break;
        case 1:
            break;
        }
        break;
    }
    switch (o->type) {
    case 8:
        if (!(m->motionId[0] == 0x3FD || m->motionId[0] == 0x3FE || m->motionId[0] == 0x3FF) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 9:
    case 26:
        if (!(m->motionId[0] == 0x3F4 || m->motionId[0] == 0x3F5 || m->motionId[0] == 0x3F6) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 29:
        if (!(m->motionId[0] == 0x411 || m->motionId[0] == 0x409) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    default:
        if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffKushala30b_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffKushala30b_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

ObjAttach *EffEm02_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = 2;
        o->type = kind;
        o->update = EffEm02_Update;
        o->idx = em->index;
        o->isEm = em->isEm;
        o->ownerMotion = em->motionId[0];
        o->owner = em;
        o->yaw = em->rotY;
        o->rotX = em->rotX;
        o->rotY = em->rotY;
        o->rotZ = em->rotZ;
        o->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->attackHit800 = 0;
    }
    return o;
}

void EffEm02_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffEm02_Setup(o);
        break;
    case 1:
        EffEm02_Run(o);
        break;
    case 2:
        EffEm02_End(o);
        break;
    case 3:
        EffEm02_Run(o);
        break;
    case 4:
        EffEm02_End(o);
        break;
    case 5:
        EffEm02_End(o);
        break;
    case 9:
        EffEm02_End(o);
        break;
    case 7:
        EffEm02_End(o);
        break;
    case 6:
        EffEm02_Free(o);
        
    default:
        break;
    }
}

void EffEm02_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = EffEm02_Draw;
    Player_TestSetFlags(o, 0x20);
    o->wasHit = 0;
    o->area = m->area;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type + 1, &EffEm02_MotionDesc);
    o->hitList = gEffEm02HitLists[o->hitListNo];
    o->hurtList = 0;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/* unk_6EA220's .data (0x8AD3A0-0x8AE7D0) */
HitShape gEffShakalaka34HitLists_0[2] = {
#include "game/008AD3A0.inc"
};
HitShape *gEffShakalaka34HitLists[4] = {
    gEffShakalaka34HitLists_0, 0, 0, 0,
};
u32 D_008AD400[36] = {
#include "game/008AD400.inc"
};
void *EffShakalaka34_MotionDesc = (void *)D_008AD400;
HitShape gEffKushala30bHitLists_0[8] = {
#include "game/008AD4A0.inc"
};
HitShape gEffKushala30bHitLists_1[6] = {
#include "game/008AD5E0.inc"
};
HitShape gEffKushala30bHitLists_2[3] = {
#include "game/008AD6D0.inc"
};
HitShape gEffKushala30bHitLists_3[3] = {
#include "game/008AD750.inc"
};
HitShape gEffKushala30bHitLists_4[6] = {
#include "game/008AD7D0.inc"
};
HitShape gEffKushala30bHitLists_5[2] = {
#include "game/008AD8C0.inc"
};
HitShape gEffKushala30bHitLists_6[8] = {
#include "game/008AD910.inc"
};
HitShape gEffKushala30bHitLists_7[6] = {
#include "game/008ADA50.inc"
};
HitShape gEffKushala30bHitLists_8[9] = {
#include "game/008ADB40.inc"
};
HitShape gEffKushala30bHitLists_9[9] = {
#include "game/008ADCB0.inc"
};
HitShape gEffKushala30bHitLists_10[3] = {
#include "game/008ADE20.inc"
};
HitShape gEffKushala30bHitLists_11[3] = {
#include "game/008ADEA0.inc"
};
HitShape gEffKushala30bHitLists_12[6] = {
#include "game/008ADF20.inc"
};
HitShape gEffKushala30bHitLists_13[2] = {
#include "game/008AE010.inc"
};
HitShape gEffKushala30bHitLists_14[2] = {
#include "game/008AE060.inc"
};
HitShape gEffKushala30bHitLists_15[2] = {
#include "game/008AE0B0.inc"
};
HitShape gEffKushala30bHitLists_16[2] = {
#include "game/008AE100.inc"
};
HitShape gEffKushala30bHitLists_17[8] = {
#include "game/008AE150.inc"
};
HitShape gEffKushala30bHitLists_18[2] = {
#include "game/008AE290.inc"
};
HitShape *gEffKushala30bHitLists[20] = {
    gEffKushala30bHitLists_0, gEffKushala30bHitLists_1, gEffKushala30bHitLists_2, gEffKushala30bHitLists_3,
    gEffKushala30bHitLists_4, gEffKushala30bHitLists_5, gEffKushala30bHitLists_6, gEffKushala30bHitLists_7,
    gEffKushala30bHitLists_8, gEffKushala30bHitLists_9, gEffKushala30bHitLists_10, gEffKushala30bHitLists_11,
    gEffKushala30bHitLists_12, gEffKushala30bHitLists_13, gEffKushala30bHitLists_14, gEffKushala30bHitLists_15,
    gEffKushala30bHitLists_16, gEffKushala30bHitLists_17, gEffKushala30bHitLists_18, 0,
};
u32 D_008AE330[186] = {
#include "game/008AE330.inc"
};
void *EffKushala30b_MotionDesc = (void *)D_008AE330;
HitShape gEffEm02HitLists_0[2] = {
#include "game/008AE620.inc"
};
HitShape gEffEm02HitLists_1[2] = {
#include "game/008AE670.inc"
};
HitShape gEffEm02HitLists_2[2] = {
#include "game/008AE6C0.inc"
};
HitShape *gEffEm02HitLists[4] = {
    gEffEm02HitLists_0, gEffEm02HitLists_1, gEffEm02HitLists_2, 0,
};
u32 D_008AE720[42] = {
#include "game/008AE720.inc"
};
void *EffEm02_MotionDesc = (void *)D_008AE720;
