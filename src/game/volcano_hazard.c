/*
 * VolcanoHazard, a kind of ObjAttach (ATTACH_VOLCANO_HAZARD) with no owner:
 * an attack that an area object (AreaObj_K19_Run) starts at one of the
 * area's points, lasting a number of frames and hitting players only.
 * VolcanoHazard_Update is its update: a switch on the object's state
 * (ObjAttach.state), one handler per state. The file holds
 * VolcanoHazard_Update's jump table.
 */
#include "common.h"
#include "scene.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "sprite.h"
#include "capsule.h"
#include "quest.h"
#include "volcano_hazard.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "hit_setup.h"
#include "obj.h"
/* Declarations of the functions called below. */

extern void *VolcanoHazard_RowsPtr[];
extern HitShape *VolcanoHazard_HitShapes[];

/*
 * Makes a VolcanoHazard of type at pos in the current area, with no owner,
 * lasting life frames (stateVar1). AreaObj_K19_Run makes type 1 for 130
 * frames at a point of volcano areas 1, 2, 5 and 6.
 */
void VolcanoHazard_Create(f32 *pos, u8 attackType, u8 life)
{
    ObjAttach *obj = ObjAttach_Alloc(0);

    if (obj != NULL) {
        obj->kind = ATTACH_VOLCANO_HAZARD;
        obj->type = attackType;
        obj->update = VolcanoHazard_Update;
        obj->idx = 0;
        obj->isEm = 0;
        obj->ownerMotion = 0;
        obj->owner = NULL;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        obj->stateVar1 = life;
        obj->area = gScene.area;
    }
}

/*
 * The VolcanoHazard's per-frame update, by ObjAttach.state: a hit keeps it
 * running; states 4, 5, 7 and 9 end it.
 */
void VolcanoHazard_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        VolcanoHazard_Setup(o);
        break;
    case ATTACH_RUN:
        VolcanoHazard_Run(o);
        break;
    case ATTACH_END:
        VolcanoHazard_End(o);
        break;
    case ATTACH_HIT:
        VolcanoHazard_Run(o);
        break;
    case 4:
        VolcanoHazard_End(o);
        break;
    case ATTACH_DEFLECTED:
        VolcanoHazard_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        VolcanoHazard_End(o);
        break;
    case 7:
        VolcanoHazard_End(o);
        break;
    case ATTACH_FREE:
        VolcanoHazard_Free(o);
        break;
    }
}

/*
 * Sets the hazard up from attack row type (ObjAttach_StartAttack; it has
 * no owner): its hit shapes. It hits players only (ATKF_NO_EM).
 */
void VolcanoHazard_Setup(ObjAttach *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_NO_EM);
    ObjAttach_StartAttack(o, o->type, VolcanoHazard_RowsPtr);
    o->hitList = VolcanoHazard_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->drawSlotBuf = NULL;
}

/*
 * Keeps the hit window open and counts the hazard's frames in ownerMotion.
 * Past stateVar1 frames it ends.
 */
void VolcanoHazard_Run(ObjAttach *o)
{
    o->hitFrames = 99;
    if (++o->ownerMotion > o->stateVar1) {
        o->hitFrames = 0;
        o->state = ATTACH_END;
        return;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 32U, 0);
    }
}

/*
 * Ends the hazard: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void VolcanoHazard_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != 0) {
        DrawSlot_Free(o->drawSlot);
    }
}

/* Frees the hazard (ObjAttach_Free). */
void VolcanoHazard_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/* The file's data. */
HitShape VolcanoHazard_HitShapes_0[2] = {
#include "game/008AE7D0.inc"
};
HitShape *VolcanoHazard_HitShapes[4] = {
    VolcanoHazard_HitShapes_0, 0, 0, 0,
};
AttackRow VolcanoHazard_Rows[2] = {
#include "game/008AE830.inc"
};
void *VolcanoHazard_RowsPtr[1] = {
    (void *)VolcanoHazard_Rows,
};
