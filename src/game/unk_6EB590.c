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
f32 TeostraBlastRing_ScatterFromBits(u16 a, u16 b);
void TeostraBlastRing_Draw(DrawSlot *d);
void Vec_RotateY(f32 *v, f32 angle);
extern void *TeostraBlastRing_RowsPtr;
extern HitShape *TeostraBlastRing_HitShapes;
extern f32 TeostraBlastRing_AnglesInOrder[];
extern f32 TeostraBlastRing_AnglesCrosswise[];
EmBone *Em_GetBone(void *obj, s16 bone);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
f32 Math_Absf(f32 x);
extern void *GunlanceBlast_RowsPtr;
extern u8 GunlanceBlast_ShellPower[];
extern u8 GunlanceBlast_WyvernFirePower[];
extern s8 TeostraBlastRing_PhaseFrames[];
f32 Rand_Unit(void);
f32 Rand_Signed(void);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
ObjAttach *ObjAttach_Alloc(s32 n);
u8 Player_InCurrentArea(void *obj);
void TeostraBlastRing_Update(ObjAttach *o);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_Identity(f32 *m);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
void Mtx_GetTrans(f32 *out, f32 *m);
extern HitShape *GunlanceBlast_HitShapes[];
void GunlanceBlast_PlaceOnWeapon(ObjAttach *o, Player *pl);
void GunlanceBlast_End(ObjAttach *o);
void GunlanceBlast_Free(ObjAttach *o);
void TeostraBlastRing_End(ObjAttach *o);
void TeostraBlastRing_Free(ObjAttach *o);
void ObjAttach_Free(ObjAttach *p);
void DrawSlot_Free(s32);
void GunlanceBlast_Run(ObjAttach *o);
void TeostraBlastRing_Run(ObjAttach *o);
void GunlanceBlast_Setup(ObjAttach *o);
void TeostraBlastRing_Setup(ObjAttach *o);
/* end of prototypes */

/*
 * The GunlanceBlast's per-frame update, by ObjAttach.state. A hit
 * (ATTACH_HIT) returns it to ATTACH_RUN with its window 2 frames longer, so
 * it can hit again. States 4, 5, 7 and 9 end it.
 */
void GunlanceBlast_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        GunlanceBlast_Setup(o);
        break;
    case ATTACH_RUN:
        GunlanceBlast_Run(o);
        break;
    case ATTACH_END:
        GunlanceBlast_End(o);
        break;
    case ATTACH_HIT:
        o->state = ATTACH_RUN;
        o->hitFrames += 2;
        GunlanceBlast_Run(o);
        break;
    case 4:
        GunlanceBlast_End(o);
        break;
    case ATTACH_DEFLECTED:
        GunlanceBlast_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        GunlanceBlast_End(o);
        break;
    case 7:
        GunlanceBlast_End(o);
        break;
    case ATTACH_FREE:
        GunlanceBlast_Free(o);
        
    default:
        break;
    }
}

/*
 * Puts the blast's capsule on the gunlance as it was at set up, as
 * PlayerAttack_PlaceOnWeapon does. The hit shape's points a and b, turned by
 * a fixed rotation and the bone 0xE matrix saved in the buffer, give pos2 and
 * pos. pl is unused.
 */
void GunlanceBlast_PlaceOnWeapon(ObjAttach *o, Player *pl)
{
    f32 shapePt[3];
    f32 turned[3];
    f32 weaponMtx[16];
    f32 mtx[16];

    Mtx_Copy((f32 *) weaponMtx, (f32 *) o->buf);
    Mtx_Identity(mtx);
    Mtx_SetRotXYZ(mtx, 0.06283185f, -0.5410521f, -3.047345f);
    Mtx_Concat3x3(mtx, weaponMtx);
    shapePt[0] = o->hitList->a[0];
    shapePt[1] = o->hitList->a[1];
    shapePt[2] = o->hitList->a[2];
    Vec_Rotate(turned, shapePt, mtx);
    Mtx_GetTrans(o->pos2, weaponMtx);
    o->pos2[0] += turned[0];
    o->pos2[1] += turned[1];
    o->pos2[2] += turned[2];
    shapePt[0] = o->hitList->b[0];
    shapePt[1] = o->hitList->b[1];
    shapePt[2] = o->hitList->b[2];
    Vec_Rotate(turned, shapePt, mtx);
    Mtx_GetTrans(o->pos, weaponMtx);
    o->pos[0] += turned[0];
    o->pos[1] += turned[1];
    o->pos[2] += turned[2];
}

/*
 * Splits the blast's power: rawShare of it stays power and the rest becomes
 * fire (elementPower, ELEMF_FIRE).
 */
static void GunlanceBlast_SplitFire(ObjAttach *o, f32 rawShare)
{
    f32 basePower = o->power;

    o->power = basePower * rawShare;
    o->elementPower = (u8) basePower - o->power;
    o->elementFlags = ELEMF_FIRE;
}

/*
 * Sets the blast up: saves the player's weapon bone (0xE) matrix in the
 * buffer and takes attack row type with its hit shapes, put on the weapon
 * when the shape's bone is 0x7E. Power: types 1-3
 * GunlanceBlast_ShellPower[bladeSpecial] (three quarters at sharpnessLevel 1
 * or less), half of it fire; type 4
 * GunlanceBlast_WyvernFirePower[bladeSpecial / 3], 60% fire. hitDelay is
 * counted from the player's motion (frame halved).
 */
void GunlanceBlast_Setup(ObjAttach *o)
{
    u8 *blastBuf = o->buf;
    Player *pl = &gPlayers[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->aux = 0;
    Mtx_Copy((f32 *) blastBuf, (f32 *) (Em_GetBone(pl, 14)));
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) pl, o->type, &GunlanceBlast_RowsPtr);
    o->hitList = GunlanceBlast_HitShapes[o->hitListNo];
    o->hurtList = 0;
    o->area = pl->area;
    if (o->hitList->bone == 0x7E) {
        GunlanceBlast_PlaceOnWeapon(o, pl);
    }
    switch (o->type) {
    case 1:
    case 2:
    case 3:
        o->power = GunlanceBlast_ShellPower[pl->bladeSpecial];
        if (pl->sharpnessLevel <= 1) {
            o->power = 0.75f * o->power;
        }
        GunlanceBlast_SplitFire(o, 0.5f);
        break;
    default:
        o->power = GunlanceBlast_WyvernFirePower[pl->bladeSpecial / 3];
        GunlanceBlast_SplitFire(o, 0.4f);
        break;
    }
    if (o->hitDelay != 0) {
        blend = Math_Absf(pl->motionBlend[0] % 100);
        o->hitDelay += blend - (s32) (Math_Absf(*(u16 *) &pl->motionFrame[0]) / 2.0f) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
}

/*
 * Keeps the blast's window open one more frame for each frame its player
 * spends in hit stop (hitStop or hitSlowTimer). o is the blast, declared as
 * Em *.
 */
static void GunlanceBlast_PauseInHitStop(Em *o)
{
    Player *pl;

    pl = &gPlayers[*(u8 *)((u8 *)o + 0xA)];
    if ((pl->hitStop != 0) || (pl->hitSlowTimer != 0)) {
        ((ObjAttach *) o)->hitFrames++;
    }
}

/*
 * Types 1-3 switch to the hit shapes four lists on after 4 frames, put on the
 * weapon again. The window is held through the player's hit stop, and the
 * blast ends when the player's motion changes.
 */
void GunlanceBlast_Run(ObjAttach *o)
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
            o->hitList = GunlanceBlast_HitShapes[o->hitListNo];
            GunlanceBlast_PlaceOnWeapon(o, pl);
        }
        break;
    case 4:
        break;
    }
    GunlanceBlast_PauseInHitStop((Em *) o);
    if (pl->motionId[0] != o->ownerMotion) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
}

/*
 * Ends the blast: hides it; it is freed on the next update (it has no draw
 * slot).
 */
void GunlanceBlast_End(ObjAttach *o)
{
    o->state = ATTACH_FREE;
    o->visible = 0;
}

/* Frees the blast (ObjAttach_Free). */
void GunlanceBlast_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes the first burst of a TeostraBlastRing of type (0-4) for em, if a
 * player is in em's area. It takes em's yaw and stateRand and waits 7 frames
 * (ownerMotion -7).
 */
void TeostraBlastRing_Create(Em *em, u8 attackType)
{
    ObjAttach *o;

    if ((Player_InCurrentArea(em) & 0xFF) && attackType <= 4) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->idx = em->index;
            o->isEm = em->isEm;
            o->area = em->area;
            o->kind = ATTACH_TEOSTRA_BLAST_RING;
            o->type = attackType;
            o->stateVar0 = 0;
            o->state = ATTACH_SETUP;
            o->update = TeostraBlastRing_Update;
            o->owner = em;
            o->randBits = em->stateRand;
            o->yaw = ((Em *) o->owner)->rotY;
            o->ownerMotion = -7;
            o->side = *(u8 *) &em->side;
        }
    }
}

/*
 * Makes burst n (stateVar0) of src's ring at pos (the ring's origin), with
 * src's type, owner, randBits, yaw and area. Only types 0-4 in the owner's
 * area.
 */
void TeostraBlastRing_SpawnNext(ObjAttach *src, f32 *pos, u8 index)
{
    ObjAttach *o;

    if ((Player_InCurrentArea(src->owner) & 0xFF) && src->type <= 4) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->idx = src->idx;
            o->isEm = src->isEm;
            o->area = src->area;
            o->kind = ATTACH_TEOSTRA_BLAST_RING;
            o->type = src->type;
            o->stateVar0 = index;
            o->state = ATTACH_SETUP;
            o->update = TeostraBlastRing_Update;
            o->owner = src->owner;
            Vec_Copy(o->pos, pos);
            o->randBits = src->randBits;
            *(u16 *) &o->yaw = *(u16 *) &src->yaw;
            o->ownerMotion = 0;
            o->side = src->side;
        }
    }
}

/*
 * The burst's per-frame update, by ObjAttach.state: a hit or a deflection
 * (states 3, 4, 5, 7, 9) keeps it running.
 */
void TeostraBlastRing_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        TeostraBlastRing_Setup(o);
        break;
    case ATTACH_RUN:
        TeostraBlastRing_Run(o);
        break;
    case ATTACH_END:
        TeostraBlastRing_End(o);
        break;
    case ATTACH_HIT:
        TeostraBlastRing_Run(o);
        break;
    case 4:
        TeostraBlastRing_Run(o);
        break;
    case ATTACH_DEFLECTED:
        TeostraBlastRing_Run(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        TeostraBlastRing_Run(o);
        break;
    case 7:
        TeostraBlastRing_Run(o);
        break;
    case ATTACH_FREE:
        TeostraBlastRing_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets a burst up: attack row 1 with ATKF_PIERCES and
 * TeostraBlastRing_HitShapes. The burst is placed at a distance from the
 * ring's origin, in the direction TeostraBlastRing_AnglesInOrder or
 * _AnglesCrosswise[stateVar0] plus its yaw, then scattered from randBits
 * (type 0: 800, in order, scatter 400; 1: 800, crosswise, 300; 2: 1800,
 * crosswise, 200; 3: 400, in order, 200). The first burst takes bone 1 of the
 * owner as the origin and opens its hit 7 frames later. A random turn and
 * size, and a draw slot (without one it frees itself); then its first frame
 * runs.
 */
void TeostraBlastRing_Setup(ObjAttach *o)
{
    f32 ofs[3];
    f32 boneMtx[16];
    TeostraBlastRingBuf *ringBuf = (TeostraBlastRingBuf *) o->buf;
    f32 dist;
    f32 scatter;
    f32 angle;
    f32 y = 0.0f;

    o->state = ATTACH_RUN;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_PIERCES);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) o->owner, 1, &TeostraBlastRing_RowsPtr);
    o->hitList = TeostraBlastRing_HitShapes;
    o->hurtList = 0;
    switch (o->type) {
    default:
        dist = 800.0f;
        angle = TeostraBlastRing_AnglesInOrder[o->stateVar0];
        scatter = 400.0f;
        break;
    case 1:
        dist = 800.0f;
        angle = TeostraBlastRing_AnglesCrosswise[o->stateVar0];
        scatter = 300.0f;
        break;
    case 2:
        dist = 1800.0f;
        angle = TeostraBlastRing_AnglesCrosswise[o->stateVar0];
        scatter = 200.0f;
        break;
    case 3:
        dist = 400.0f;
        angle = TeostraBlastRing_AnglesInOrder[o->stateVar0];
        scatter = 200.0f;
        break;
    }
    ofs[0] = 0.0f;
    ofs[1] = 0.0f;
    ofs[2] = dist;
    angle += (360.0f * (u16) o->yaw) / 65536.0f;
    Vec_RotateY(ofs, (6.2831855f * angle) / 360.0f);
    ofs[0] += scatter * TeostraBlastRing_ScatterFromBits(o->randBits * o->stateVar0, 8);
    ofs[1] += y;
    ofs[2] += scatter * TeostraBlastRing_ScatterFromBits(o->randBits * o->stateVar0, 14);
    ringBuf->phase = 0;
    ringBuf->rotZ = 0.5f + (65536.0f * (360.0f * Rand_Unit())) / 360.0f;
    ringBuf->scale = 0.01f * (100.0f + 20.0f * Rand_Unit());
    ringBuf->seDone = 0;
    if (o->stateVar0 <= 0) {
        o->hitDelay += 7;
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(o->owner, 1)));
        Mtx_GetTrans(o->pos, boneMtx);
        Vec_Copy(ringBuf->origin, o->pos);
    } else {
        Vec_Copy(ringBuf->origin, o->pos);
    }
    o->pos[0] += ofs[0];
    o->pos[1] += ofs[1];
    o->pos[2] += ofs[2];
    o->drawSlot = DrawSlot_Alloc();
    if (o->drawSlot != -1) {
        o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
        o->drawSlotBuf->owner = o;
        o->drawSlotBuf->draw = TeostraBlastRing_Draw;
    } else {
        ObjAttach_Free(o);
        return;
    }
    TeostraBlastRing_Run(o);
}

/*
 * Counts the burst through its phases (TeostraBlastRing_PhaseFrames), from
 * the first positive frame. When phase 0 ends it plays its sound (0x24) once
 * and spawns the next burst at the origin, up to the eighth. Phase 2 gets a
 * random spin and rise and rises; after it the burst ends. The draw slot is
 * queued every frame; without one the burst ends.
 */
void TeostraBlastRing_Run(ObjAttach *o)
{
    TeostraBlastRingBuf *ringBuf = (TeostraBlastRingBuf *) o->buf;
    f32 next;

    if (o->drawSlotBuf == NULL) {
        o->state = ATTACH_END;
        return;
    }
    if (++o->ownerMotion <= 0) {
        return;
    }
    if (o->ownerMotion > TeostraBlastRing_PhaseFrames[ringBuf->phase]) {
        switch (ringBuf->phase) {
        case 0:
            o->ownerMotion = 1;
            ringBuf->phase = 1;
            if (ringBuf->seDone == 0) {
                if (o->area == gScene.area) {
                    Em_PlaySeChecked(o->owner, 0x24, 0, o->pos, 15, 0);
                }
                ringBuf->seDone++;
            }
            if (o->stateVar0 < 7) {
                next = o->stateVar0 + 1;
                TeostraBlastRing_SpawnNext(o, ringBuf->origin, next);
            }
            break;
        case 1:
            o->ownerMotion = 1;
            ringBuf->phase = 2;
            ringBuf->rotZ = 0.5f + (65536.0f * (360.0f * Rand_Unit())) / 360.0f;
            ringBuf->spin = 0.5f + (65536.0f * ((10.0f * Rand_Signed()) / TeostraBlastRing_PhaseFrames[2])) / 360.0f;
            ringBuf->rise = (100.0f + 100.0f * Rand_Unit()) / TeostraBlastRing_PhaseFrames[2];
            break;
        case 2:
        default:
            o->state = ATTACH_END;
            return;
        }
    } else {
        switch (ringBuf->phase) {
        case 0:
        case 1:
            break;
        case 2:
            ringBuf->rotZ += ringBuf->spin;
            o->pos[1] += ringBuf->rise;
            break;
        }
    }
    Vec_Copy(o->drawSlotBuf->pos, o->pos);
    OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 0x40U, 0);
}

/*
 * Ends the burst: closes its hit window, hides it and frees its draw slot. It
 * is freed on the next update.
 */
void TeostraBlastRing_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* unk_6EB590's .data (0x8AE870-0x8AEC50) */
HitShape GunlanceBlast_HitShapes_0[2] = {
#include "game/008AE870.inc"
};
HitShape GunlanceBlast_HitShapes_1[2] = {
#include "game/008AE8C0.inc"
};
HitShape GunlanceBlast_HitShapes_2[2] = {
#include "game/008AE910.inc"
};
HitShape GunlanceBlast_HitShapes_3[2] = {
#include "game/008AE960.inc"
};
HitShape GunlanceBlast_HitShapes_4[2] = {
#include "game/008AE9B0.inc"
};
HitShape GunlanceBlast_HitShapes_5[2] = {
#include "game/008AEA00.inc"
};
HitShape GunlanceBlast_HitShapes_6[2] = {
#include "game/008AEA50.inc"
};
HitShape *GunlanceBlast_HitShapes[8] = {
    GunlanceBlast_HitShapes_0, GunlanceBlast_HitShapes_1, GunlanceBlast_HitShapes_2, GunlanceBlast_HitShapes_3,
    GunlanceBlast_HitShapes_4, GunlanceBlast_HitShapes_5, GunlanceBlast_HitShapes_6, 0,
};
u32 GunlanceBlast_Rows[30] = {
#include "game/008AEAC0.inc"
};
void *GunlanceBlast_RowsPtr = (void *)GunlanceBlast_Rows;
u8 GunlanceBlast_ShellPower[16] = {
#include "game/008AEB40.inc"
};
u8 GunlanceBlast_WyvernFirePower[16] = {
#include "game/008AEB50.inc"
};
HitShape D_008AEB60[2] = {
#include "game/008AEB60.inc"
};
HitShape *TeostraBlastRing_HitShapes = D_008AEB60;
u32 TeostraBlastRing_Rows[12] = {
#include "game/008AEBC0.inc"
};
void *TeostraBlastRing_RowsPtr = (void *)TeostraBlastRing_Rows;
f32 TeostraBlastRing_AnglesInOrder[8] = {
#include "game/008AEC00.inc"
};
f32 TeostraBlastRing_AnglesCrosswise[8] = {
#include "game/008AEC20.inc"
};
s8 TeostraBlastRing_PhaseFrames[16] = {
#include "game/008AEC40.inc"
};
