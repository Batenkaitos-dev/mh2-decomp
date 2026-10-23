/*
 * EmImpactAttack, the attack-object family of kind ATTACH_EM_IMPACT_ATTACK
 * (enum ObjAttachKind): a monster's hit zone made at a point for one attack
 * row (a slam, a landing, a stomp), made by the monster states through
 * EmImpactAttack_CreateAt. Its Update, which ObjAttach_UpdateAll calls every
 * frame, switches on ObjAttach.state (enum ObjAttachState) to its Setup,
 * Run, End and Free; its type picks a row of EmImpactAttack_Rows and a list
 * of EmImpactAttack_HitShapes.
 */

#include "common.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "capsule.h"
#include "sprite.h"
#include "quest.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "hit_setup.h"
#include "area.h"

extern f32 (*EmRangedAttack_KirinStrikeSpots[])[2];
extern HitShape *EmImpactAttack_HitShapes[];
extern void *EmImpactAttack_RowsPtr;
/*
 * Makes an EmImpactAttack of type kind (its row of the attack and hit tables)
 * for the monster em at pos, when em is in the current area: it copies the
 * monster's motion, rotation and side, and clears the monster's hitPending
 * and contactHit. Types 5 and 6 (EmHermitaur_MotionEffects) take the
 * monster's scale for the hit capsule.
 */
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind)
{
    ObjAttach *impact;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((impact = ObjAttach_Alloc(0)) != NULL) {
            impact->kind = ATTACH_EM_IMPACT_ATTACK;
            impact->type = kind;
            impact->update = EmImpactAttack_Update;
            impact->idx = em->index;
            impact->isEm = em->isEm;
            impact->ownerMotion = em->motionId[0];
            impact->owner = em;
            impact->yaw = em->rotY;
            impact->pos[0] = pos[0];
            impact->pos[1] = pos[1];
            impact->pos[2] = pos[2];
            impact->rotX = em->rotX;
            impact->rotY = em->rotY;
            impact->rotZ = em->rotZ;
            impact->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
            switch (impact->type) {
            case 5:
            case 6:
                impact->hitScale = em->scaleX;
                break;
            }
        }
    }
}

/*
 * The family's update (ObjAttach.update): by ObjAttach.state it runs
 * EmImpactAttack_Setup, EmImpactAttack_Run (also in ATTACH_HIT),
 * EmImpactAttack_End in every other live state, and EmImpactAttack_Free.
 */
void EmImpactAttack_Update(ObjAttach *impact)
{
    switch (impact->state) {
    case ATTACH_SETUP:
        EmImpactAttack_Setup(impact);
        break;
    case ATTACH_RUN:
        EmImpactAttack_Run(impact);
        break;
    case ATTACH_END:
        EmImpactAttack_End(impact);
        break;
    case ATTACH_HIT:
        EmImpactAttack_Run(impact);
        break;
    case 4:
        EmImpactAttack_End(impact);
        break;
    case ATTACH_DEFLECTED:
        EmImpactAttack_End(impact);
        break;
    case ATTACH_DEFLECTED_AURA:
        EmImpactAttack_End(impact);
        break;
    case 7:
        EmImpactAttack_End(impact);
        break;
    case ATTACH_FREE:
        EmImpactAttack_Free(impact);
        
    default:
        break;
    }
}

/*
 * Sets an impact up: its attack flags by type (6 hits no player, 7 hits
 * shielded objects, the others give the owner its contact), its attack row
 * and hit shapes, the monster's area, and its hitDelay moved by the monster
 * motion's blend frames less its start frame, never below 0. It has no model
 * and no draw slot.
 */
void EmImpactAttack_Setup(ObjAttach *impact)
{
    Em *ownerEm = &gEms[impact->idx];
    s32 blendFrames;

    impact->state++;
    impact->visible = 1;
    impact->draw = NULL;
    switch (impact->type) {
    case 6:
        Player_TestSetFlags(impact, ATKF_NO_PLAYER);
        break;
    case 7:
        Player_TestSetFlags(impact, ATKF_PIERCES);
        break;
    default:
        Player_TestSetFlags(impact, ATKF_OWNER_CONTACT);
        break;
    }
    ObjAttach_StartOwnedAttack(impact, (s32) ownerEm, impact->type, &EmImpactAttack_RowsPtr);
    impact->hitList = EmImpactAttack_HitShapes[impact->hitListNo];
    impact->hurtList = 0;
    impact->area = ownerEm->area;
    if (impact->hitDelay != 0) {
        blendFrames = Math_Absf(ownerEm->motionBlend[0] % 100);
        impact->hitDelay += blendFrames - (s32) Math_Absf(ownerEm->motionFrame[0]) - 1;
        if ((s8) impact->hitDelay < 0) {
            impact->hitDelay = 0;
        }
    }
    impact->drawSlotBuf = NULL;
}

/*
 * Ends the impact once its hit window has closed (hitPhase 0), and queues its
 * draw slot when it has one (Setup gives it none).
 */
void EmImpactAttack_Run(ObjAttach *impact)
{

    if (impact->hitPhase == 0) {
        impact->state = ATTACH_END;
    }
    if (impact->drawSlotBuf != NULL) {
        Vec_Copy(impact->drawSlotBuf->pos, impact->pos);
        OT_AddByDepth(gDrawOt32, impact->drawSlotBuf, 32U, 0);
    }
}

/*
 * Ends an impact: closes its hit, hides it, frees its draw slot if it has
 * one, and sets ATTACH_FREE.
 */
void EmImpactAttack_End(ObjAttach *impact)
{
    impact->hitPhase = 0;
    impact->state = ATTACH_FREE;
    impact->visible = 0;
    if (impact->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) impact->drawSlot);
    }
}

/* Frees the object (ObjAttach_Free). */
void EmImpactAttack_Free(ObjAttach *impact)
{
    ObjAttach_Free(impact);
}

/* This file's initialised data. */
HitShape EmImpactAttack_HitShapes_0[2] = {
#include "game/0089C1B0.inc"
};
HitShape EmImpactAttack_HitShapes_1[2] = {
#include "game/0089C200.inc"
};
HitShape EmImpactAttack_HitShapes_2[2] = {
#include "game/0089C250.inc"
};
HitShape EmImpactAttack_HitShapes_3[2] = {
#include "game/0089C2A0.inc"
};
HitShape EmImpactAttack_HitShapes_4[2] = {
#include "game/0089C2F0.inc"
};
HitShape EmImpactAttack_HitShapes_5[2] = {
#include "game/0089C340.inc"
};
HitShape EmImpactAttack_HitShapes_6[2] = {
#include "game/0089C390.inc"
};
HitShape EmImpactAttack_HitShapes_7[2] = {
#include "game/0089C3E0.inc"
};
HitShape EmImpactAttack_HitShapes_8[2] = {
#include "game/0089C430.inc"
};
HitShape EmImpactAttack_HitShapes_9[2] = {
#include "game/0089C480.inc"
};
HitShape EmImpactAttack_HitShapes_10[2] = {
#include "game/0089C4D0.inc"
};
HitShape *EmImpactAttack_HitShapes[12] = {
    EmImpactAttack_HitShapes_0, EmImpactAttack_HitShapes_1, EmImpactAttack_HitShapes_2, EmImpactAttack_HitShapes_3,
    EmImpactAttack_HitShapes_4, EmImpactAttack_HitShapes_5, EmImpactAttack_HitShapes_6, EmImpactAttack_HitShapes_7,
    EmImpactAttack_HitShapes_8, EmImpactAttack_HitShapes_9, EmImpactAttack_HitShapes_10, 0,
};
AttackRow EmImpactAttack_Rows[17] = {
#include "game/0089C550.inc"
};
void *EmImpactAttack_RowsPtr = (void *)EmImpactAttack_Rows;
