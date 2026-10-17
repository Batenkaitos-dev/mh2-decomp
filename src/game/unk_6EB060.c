/*
 * 0x6EB060-0x6EB590: two kinds of objects of sub_main's object list
 * (ObjAttach_Alloc allocates them; +0x20 is the update function), each tied to
 * a monster (+0xA: index into gEms). VolcanoHazard_Update is kind 0x25's
 * update: a switch on the object's state (+0x4), one handler per state.
 * The file owns its .rodata: the jump tables of BulldromeAttack_Run and
 * VolcanoHazard_Update (0x8DE1F0-0x8DE240).
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
void GunlanceBlast_Update(ObjAttach *o);
void Vec_Copy(f32 *out, f32 *in);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);

void DrawSlot_Free(s32 i);
void ObjAttach_Free(ObjAttach *p);

void VolcanoHazard_Setup(ObjAttach *o);
void VolcanoHazard_Run(ObjAttach *o);
void VolcanoHazard_End(ObjAttach *o);
void VolcanoHazard_Free(ObjAttach *o);

s32 Player_TestSetFlags(void *, s32 mask);
struct MoveObj;
void MoveObj_StartMotionEntry(struct MoveObj *, s32 a, void *b);
extern void *VolcanoHazard_RowsPtr[];
extern HitShape *VolcanoHazard_HitShapes[];
/*
 * Types 3-5 keep the window open. Types 0 and 2 also keep it open, closing it
 * once the monster leaves EM_GROUP_ATTACK or Em.unk473 is 0
 * (EmKind38_ChangeState clears it). Type 1 follows the monster's yaw, its
 * power 35 while more than 99 or 43-50 window frames are left, else 25. The
 * attack ends when the motion changes, the window closes or the monster's
 * task leaves its run phase.
 */
void BulldromeAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 0:
    case 2:
        o->hitFrames = 99;
        if (em->stateGroup != EM_GROUP_ATTACK) {
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
        o->state = ATTACH_END;
        return;
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void BulldromeAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != 0) {
        DrawSlot_Free(o->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void BulldromeAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/* Draws nothing. */
void BulldromeAttack_Draw(ObjAttach *o)
{
}

void VolcanoHazard_Update(ObjAttach *o);

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
 * Sets the hazard up from attack row type (MoveObj_StartMotionEntry; it has
 * no owner): its hit shapes. It hits players only (ATKF_NO_EM).
 */
void VolcanoHazard_Setup(ObjAttach *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_NO_EM);
    MoveObj_StartMotionEntry((struct MoveObj *)o, o->type, VolcanoHazard_RowsPtr);
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
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
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

/*
 * Makes a GunlanceBlast of type for player pl: 1-3 the shell blast by
 * shelling type (Gunlance_ShellEffect), 4 the wyvern fire
 * (Gunlance_StateWyvernFire). It starts at pl's position with pl's angles,
 * motion and stateRand, and clears pl's hitPending and contactHit.
 */
void GunlanceBlast_Create(Player *pl, s8 attackType)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(1)) != NULL) {
        o->kind = ATTACH_GUNLANCE_BLAST;
        o->type = attackType;
        o->update = GunlanceBlast_Update;
        o->idx = pl->index;
        o->isEm = pl->isEm;
        o->ownerMotion = pl->motionId[0];
        o->owner = pl;
        o->yaw = pl->rot[1];
        o->randBits = pl->stateRand;
        o->stateVar0 = 0;
        o->stateVar1 = 0;
        o->rotX = pl->rot[0];
        o->rotY = pl->rot[1];
        o->rotZ = pl->rot[2];
        Vec_Copy(o->pos, pl->pos);
        o->side = pl->side;
        o->unk080 = 0;
        pl->hitPending = 0;
        pl->contactHit = 0;
    }
}

/* unk_6EB060's .data (0x8AE7D0-0x8AE870) */
HitShape VolcanoHazard_HitShapes_0[2] = {
#include "game/008AE7D0.inc"
};
HitShape *VolcanoHazard_HitShapes[4] = {
    VolcanoHazard_HitShapes_0, 0, 0, 0,
};
u32 VolcanoHazard_Rows[12] = {
#include "game/008AE830.inc"
};
void *VolcanoHazard_RowsPtr[1] = {
    (void *)VolcanoHazard_Rows,
};
