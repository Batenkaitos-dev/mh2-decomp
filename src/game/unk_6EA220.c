/*
 * Game overlay code at 0x6EA220-0x6EB060: ShakalakaAttack_*, TeostraAttack_*
 * and BulldromeAttack_*.
 */
#include "common.h"
#include "em.h"
#include "objattach.h"
#include "player.h"
#include "sprite.h"
#include "capsule.h"

s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
void BulldromeAttack_Update(ObjAttach *o);
void ShakalakaAttack_Draw(ObjAttach *o);
void BulldromeAttack_Draw(ObjAttach *o);
extern HitShape *ShakalakaAttack_HitShapes[];
extern void *ShakalakaAttack_RowsPtr;
extern HitShape *BulldromeAttack_HitShapes[];
extern void *BulldromeAttack_RowsPtr;
ObjAttach *ObjAttach_Alloc(s32 n);
void ObjAttach_Free(ObjAttach *p);
void ShakalakaAttack_End(ObjAttach *o);
void ShakalakaAttack_Free(ObjAttach *o);
void TeostraAttack_Setup(ObjAttach *o);
void TeostraAttack_End(ObjAttach *o);
void TeostraAttack_Free(ObjAttach *o);
void BulldromeAttack_Run(struct ObjAttach *o);
void BulldromeAttack_End(struct ObjAttach *o);
void BulldromeAttack_Free(struct ObjAttach *o);
void DrawSlot_Free(s32);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Vec_Copy(f32 *out, f32 *in);
void ShakalakaAttack_Update(ObjAttach *o);
void TeostraAttack_Update(ObjAttach *o);
u8 Player_InCurrentArea(void *obj);
s32 Player_TestSetFlags(void *, s32 mask);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
extern HitShape *TeostraAttack_HitShapes[];
extern void *TeostraAttack_RowsPtr;
f32 Math_Absf(f32 x);
void ShakalakaAttack_Setup(ObjAttach *o);
void BulldromeAttack_Setup(ObjAttach *o);
void ShakalakaAttack_Run(ObjAttach *o);
void TeostraAttack_Run(ObjAttach *o);

/*
 * Makes a ShakalakaAttack of type for em, if a player is in em's area. Takes
 * em's angles and motion and clears its hitPending and contactHit.
 */
void ShakalakaAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_SHAKALAKA_ATTACK;
            o->type = kind;
            o->update = ShakalakaAttack_Update;
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
            em->contactHit = 0;
        }
    }
}

/*
 * The ShakalakaAttack's per-frame update, by ObjAttach.state: a hit keeps it
 * running; states 4, 5, 7 and 9 end it.
 */
void ShakalakaAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        ShakalakaAttack_Setup(o);
        break;
    case ATTACH_RUN:
        ShakalakaAttack_Run(o);
        break;
    case ATTACH_END:
        ShakalakaAttack_End(o);
        break;
    case ATTACH_HIT:
        ShakalakaAttack_Run(o);
        break;
    case 4:
        ShakalakaAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        ShakalakaAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        ShakalakaAttack_End(o);
        break;
    case 7:
        ShakalakaAttack_End(o);
        break;
    case ATTACH_FREE:
        ShakalakaAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from attack row type: its hit shapes,
 * ATKF_OWNER_CONTACT, the empty ShakalakaAttack_Draw, and hitDelay counted
 * from the monster's motion.
 */
void ShakalakaAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = ShakalakaAttack_Draw;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    o->wasHit = 0;
    o->area = em->area;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) em, o->type, &ShakalakaAttack_RowsPtr);
    o->hitList = ShakalakaAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    if (o->hitDelay != 0) {
        blend = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * Holds the window through the attack's motions. Type 2 holds through motion
 * 0x3FF and through 0x400 up to frame 40; type 3 while 0x3F5 plays; type 4
 * through 0x403 and 0x404; type 5 while 0x3F6 plays. The attack ends when the
 * motion differs from ownerMotion, the window closes or the monster's task
 * leaves its run phase.
 */
void ShakalakaAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 2:
        switch (em->motionId[0]) {
        case 0x400:
            o->ownerMotion = 0x400;
            if (Motion_ReachedFrame(em, 0, 40.0f) == 0) {
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
        switch (em->motionId[0]) {
        case 0x3F5:
            o->hitFrames = 99;
            break;
        default:
            o->hitPhase = 0;
            break;
        }
        break;
    case 4:
        switch (em->motionId[0]) {
        case 0x405:
        default:
            o->hitPhase = 0;
            break;
        case 0x403:
        case 0x404:
            o->ownerMotion = em->motionId[0];
            o->hitFrames = 99;
            break;
        }
        break;
    case 5:
        switch (em->motionId[0]) {
        case 0x3F6:
            o->hitFrames = 99;
            break;
        default:
            o->hitPhase = 0;
            break;
        }
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0 || em->taskPhase >= 2) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
        return;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void ShakalakaAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void ShakalakaAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/* Draws nothing. */
void ShakalakaAttack_Draw(ObjAttach *o)
{
}

/*
 * Makes a TeostraAttack (one of Lunastra's or Teostra's body hit zones) of
 * type for em, if a player is in em's area. EmKushala_CreateAttack calls it
 * for kinds other than 0x36 and 0x3C.
 */
void TeostraAttack_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_KUSHALA_ATTACK;
            o->type = kind;
            o->update = TeostraAttack_Update;
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
            em->contactHit = 0;
        }
    }
}

/*
 * The TeostraAttack's per-frame update, by ObjAttach.state: a hit keeps it
 * running; states 4, 5, 7 and 9 end it.
 */
void TeostraAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        TeostraAttack_Setup(o);
        break;
    case ATTACH_RUN:
        TeostraAttack_Run(o);
        break;
    case ATTACH_END:
        TeostraAttack_End(o);
        break;
    case ATTACH_HIT:
        TeostraAttack_Run(o);
        break;
    case 4:
        TeostraAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        TeostraAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        TeostraAttack_End(o);
        break;
    case 7:
        TeostraAttack_End(o);
        break;
    case ATTACH_FREE:
        TeostraAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from attack row type of its own tables: its hit shapes,
 * ATKF_OWNER_CONTACT, and hitDelay counted from the monster's motion.
 */
void TeostraAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) em, o->type, &TeostraAttack_RowsPtr);
    o->hitList = TeostraAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = em->area;
    if (o->hitDelay != 0) {
        blend = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/*
 * As KushalaAttack_Run: types 8 and 25 hold the window open; 9, 26 and 29
 * stay 31, 1 or 32 frames after motion 0x3F6 or 0x409 starts. Each type ends
 * when the window closes or the motion leaves its own.
 */
void TeostraAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 8:
        o->hitFrames = 99;
        break;
    case 9:
        switch (o->step) {
        case 0:
            if (em->motionId[0] == 0x3F6) {
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
            if (em->motionId[0] == 0x3F6) {
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
            if (em->motionId[0] == 0x409) {
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
        if (!(em->motionId[0] == 0x3FD || em->motionId[0] == 0x3FE || em->motionId[0] == 0x3FF) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case 9:
    case 26:
        if (!(em->motionId[0] == 0x3F4 || em->motionId[0] == 0x3F5 || em->motionId[0] == 0x3F6) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case 29:
        if (!(em->motionId[0] == 0x411 || em->motionId[0] == 0x409) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    default:
        if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void TeostraAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void TeostraAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a BulldromeAttack of type for em, if a player is in em's area.
 * Returns it, or NULL. Types: 1 Bulldrome's spin, 3-5 its charge, 0 and 2 the
 * dash of monster kind 0x38.
 */
ObjAttach *BulldromeAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    if ((o = ObjAttach_Alloc(0)) != NULL) {
        o->kind = ATTACH_EM_CONTACT_ATTACK;
        o->type = kind;
        o->update = BulldromeAttack_Update;
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
        em->contactHit = 0;
    }
    return o;
}

/*
 * The BulldromeAttack's per-frame update, by ObjAttach.state: a hit keeps it
 * running; states 4, 5, 7 and 9 end it.
 */
void BulldromeAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        BulldromeAttack_Setup(o);
        break;
    case ATTACH_RUN:
        BulldromeAttack_Run(o);
        break;
    case ATTACH_END:
        BulldromeAttack_End(o);
        break;
    case ATTACH_HIT:
        BulldromeAttack_Run(o);
        break;
    case 4:
        BulldromeAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        BulldromeAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        BulldromeAttack_End(o);
        break;
    case 7:
        BulldromeAttack_End(o);
        break;
    case ATTACH_FREE:
        BulldromeAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from attack row type + 1 (as EmContactAttack, but with
 * ATKF_OWNER_CONTACT only): its hit shapes, and hitDelay counted from the
 * monster's motion.
 */
void BulldromeAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = BulldromeAttack_Draw;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    o->wasHit = 0;
    o->area = em->area;
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) em, o->type + 1, &BulldromeAttack_RowsPtr);
    o->hitList = BulldromeAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    if (o->hitDelay != 0) {
        blend = Math_Absf(em->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

/* unk_6EA220's .data (0x8AD3A0-0x8AE7D0) */
HitShape ShakalakaAttack_HitShapes_0[2] = {
#include "game/008AD3A0.inc"
};
HitShape *ShakalakaAttack_HitShapes[4] = {
    ShakalakaAttack_HitShapes_0, 0, 0, 0,
};
u32 ShakalakaAttack_Rows[36] = {
#include "game/008AD400.inc"
};
void *ShakalakaAttack_RowsPtr = (void *)ShakalakaAttack_Rows;
HitShape TeostraAttack_HitShapes_0[8] = {
#include "game/008AD4A0.inc"
};
HitShape TeostraAttack_HitShapes_1[6] = {
#include "game/008AD5E0.inc"
};
HitShape TeostraAttack_HitShapes_2[3] = {
#include "game/008AD6D0.inc"
};
HitShape TeostraAttack_HitShapes_3[3] = {
#include "game/008AD750.inc"
};
HitShape TeostraAttack_HitShapes_4[6] = {
#include "game/008AD7D0.inc"
};
HitShape TeostraAttack_HitShapes_5[2] = {
#include "game/008AD8C0.inc"
};
HitShape TeostraAttack_HitShapes_6[8] = {
#include "game/008AD910.inc"
};
HitShape TeostraAttack_HitShapes_7[6] = {
#include "game/008ADA50.inc"
};
HitShape TeostraAttack_HitShapes_8[9] = {
#include "game/008ADB40.inc"
};
HitShape TeostraAttack_HitShapes_9[9] = {
#include "game/008ADCB0.inc"
};
HitShape TeostraAttack_HitShapes_10[3] = {
#include "game/008ADE20.inc"
};
HitShape TeostraAttack_HitShapes_11[3] = {
#include "game/008ADEA0.inc"
};
HitShape TeostraAttack_HitShapes_12[6] = {
#include "game/008ADF20.inc"
};
HitShape TeostraAttack_HitShapes_13[2] = {
#include "game/008AE010.inc"
};
HitShape TeostraAttack_HitShapes_14[2] = {
#include "game/008AE060.inc"
};
HitShape TeostraAttack_HitShapes_15[2] = {
#include "game/008AE0B0.inc"
};
HitShape TeostraAttack_HitShapes_16[2] = {
#include "game/008AE100.inc"
};
HitShape TeostraAttack_HitShapes_17[8] = {
#include "game/008AE150.inc"
};
HitShape TeostraAttack_HitShapes_18[2] = {
#include "game/008AE290.inc"
};
HitShape *TeostraAttack_HitShapes[20] = {
    TeostraAttack_HitShapes_0, TeostraAttack_HitShapes_1, TeostraAttack_HitShapes_2, TeostraAttack_HitShapes_3,
    TeostraAttack_HitShapes_4, TeostraAttack_HitShapes_5, TeostraAttack_HitShapes_6, TeostraAttack_HitShapes_7,
    TeostraAttack_HitShapes_8, TeostraAttack_HitShapes_9, TeostraAttack_HitShapes_10, TeostraAttack_HitShapes_11,
    TeostraAttack_HitShapes_12, TeostraAttack_HitShapes_13, TeostraAttack_HitShapes_14, TeostraAttack_HitShapes_15,
    TeostraAttack_HitShapes_16, TeostraAttack_HitShapes_17, TeostraAttack_HitShapes_18, 0,
};
u32 TeostraAttack_Rows[186] = {
#include "game/008AE330.inc"
};
void *TeostraAttack_RowsPtr = (void *)TeostraAttack_Rows;
HitShape BulldromeAttack_HitShapes_0[2] = {
#include "game/008AE620.inc"
};
HitShape BulldromeAttack_HitShapes_1[2] = {
#include "game/008AE670.inc"
};
HitShape BulldromeAttack_HitShapes_2[2] = {
#include "game/008AE6C0.inc"
};
HitShape *BulldromeAttack_HitShapes[4] = {
    BulldromeAttack_HitShapes_0, BulldromeAttack_HitShapes_1, BulldromeAttack_HitShapes_2, 0,
};
u32 BulldromeAttack_Rows[42] = {
#include "game/008AE720.inc"
};
void *BulldromeAttack_RowsPtr = (void *)BulldromeAttack_Rows;
