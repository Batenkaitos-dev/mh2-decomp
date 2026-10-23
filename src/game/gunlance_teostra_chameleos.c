/*
 * Four attack-object families: GunlanceBlast (the gunlance's shells and
 * blast, made by the gunlance's states), TeostraBlastRing (Lunastra's and
 * Teostra's blast ring), ChameleosAttack (Chameleos's hits, made on given
 * frames of its motions) and RemobraAttack (Remobra's).
 * Each family is a kind of ObjAttach (enum
 * ObjAttachKind) on sub_main's object list: its Create fills the object and
 * its Update, which ObjAttach_UpdateAll calls every frame, switches on
 * ObjAttach.state (enum ObjAttachState) to its Setup, Run, Hit, End and Free;
 * its type picks a row of its tables (the hit's power, kind, element and
 * shapes).
 */
#include "common.h"
#include "em.h"
#include "em_chameleos.h"
#include "objattach.h"
#include "player.h"
#include "sprite.h"
#include "capsule.h"
#include "render_state.h"
#include "quest.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "hit_setup.h"
#include "area.h"
#include "sound.h"
/* Declarations of the functions called below. */

/* prototypes */
extern Vec3Key TeostraBlastRing_ScaleKeysStart[];
extern ColorKey TeostraBlastRing_ColorKeysStart[];
extern Vec3Key TeostraBlastRing_ScaleKeysMiddle[];
extern ColorKey TeostraBlastRing_ColorKeysMiddle[];
extern Vec3Key TeostraBlastRing_ScaleKeysEnd[];
extern ColorKey TeostraBlastRing_ColorKeysEnd[];
extern u8 TeostraBlastRing_ModelKinds[];
extern void *TeostraBlastRing_RowsPtr;
extern HitShape *TeostraBlastRing_HitShapes;
extern f32 TeostraBlastRing_AnglesInOrder[];
extern f32 TeostraBlastRing_AnglesCrosswise[];
extern void *GunlanceBlast_RowsPtr;
extern u8 GunlanceBlast_ShellPower[];
extern u8 GunlanceBlast_WyvernFirePower[];
extern s8 TeostraBlastRing_PhaseFrames[];
extern HitShape *GunlanceBlast_HitShapes[];
extern f32 gCamMatrix[];
extern HitShape *ChameleosAttack_HitShapes[];
extern void *ChameleosAttack_RowsPtr;
extern HitShape *RemobraAttack_HitShapes[];
extern void *RemobraAttack_RowsPtr;
/* end of prototypes */

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

/*
 * The GunlanceBlast's per-frame update, by ObjAttach.state. A hit
 * (ATTACH_HIT) returns it to ATTACH_RUN with its window 2 frames longer, so
 * it can hit again. States 4, 5, 7 and 9 end it.
 */
void GunlanceBlast_Update(ObjAttach *blast)
{
    switch (blast->state) {
    case ATTACH_SETUP:
        GunlanceBlast_Setup(blast);
        break;
    case ATTACH_RUN:
        GunlanceBlast_Run(blast);
        break;
    case ATTACH_END:
        GunlanceBlast_End(blast);
        break;
    case ATTACH_HIT:
        blast->state = ATTACH_RUN;
        blast->hitFrames += 2;
        GunlanceBlast_Run(blast);
        break;
    case 4:
        GunlanceBlast_End(blast);
        break;
    case ATTACH_DEFLECTED:
        GunlanceBlast_End(blast);
        break;
    case ATTACH_DEFLECTED_AURA:
        GunlanceBlast_End(blast);
        break;
    case 7:
        GunlanceBlast_End(blast);
        break;
    case ATTACH_FREE:
        GunlanceBlast_Free(blast);
        
    default:
        break;
    }
}

/*
 * Puts the blast's capsule on the gunlance as it was at set up, as
 * PlayerAttack_PlaceOnWeapon does. The hit shape's points a and b, turned by
 * a fixed rotation and the bone PL_BONE_HAND matrix saved in the buffer, give pos2 and
 * pos. pl is unused.
 */
void GunlanceBlast_PlaceOnWeapon(ObjAttach *blast, Player *pl)
{
    f32 shapePt[3];
    f32 turned[3];
    f32 weaponMtx[16];
    f32 mtx[16];

    Mtx_Copy((f32 *) weaponMtx, (f32 *) blast->buf);
    Mtx_Identity(mtx);
    Mtx_SetRotXYZ(mtx, 0.06283185f, -0.5410521f, -3.047345f);
    Mtx_Concat3x3(mtx, weaponMtx);
    shapePt[0] = blast->hitList->a[0];
    shapePt[1] = blast->hitList->a[1];
    shapePt[2] = blast->hitList->a[2];
    Vec_Rotate(turned, shapePt, mtx);
    Mtx_GetTrans(blast->pos2, weaponMtx);
    blast->pos2[0] += turned[0];
    blast->pos2[1] += turned[1];
    blast->pos2[2] += turned[2];
    shapePt[0] = blast->hitList->b[0];
    shapePt[1] = blast->hitList->b[1];
    shapePt[2] = blast->hitList->b[2];
    Vec_Rotate(turned, shapePt, mtx);
    Mtx_GetTrans(blast->pos, weaponMtx);
    blast->pos[0] += turned[0];
    blast->pos[1] += turned[1];
    blast->pos[2] += turned[2];
}

/*
 * Splits the blast's power: rawShare of it stays power and the rest becomes
 * fire (elementPower, ELEMF_FIRE).
 */
static void GunlanceBlast_SplitFire(ObjAttach *blast, f32 rawShare)
{
    f32 basePower = blast->power;

    blast->power = basePower * rawShare;
    blast->elementPower = (u8) basePower - blast->power;
    blast->elementFlags = ELEMF_FIRE;
}

/*
 * Sets the blast up: saves the player's weapon bone (PL_BONE_HAND) matrix in the
 * buffer and takes attack row type with its hit shapes, put on the weapon
 * when the shape's bone is 126. Power: types 1-3
 * GunlanceBlast_ShellPower[bladeSpecial] (three quarters at sharpnessLevel 1
 * or less), half of it fire; type 4
 * GunlanceBlast_WyvernFirePower[bladeSpecial / 3], 60% fire. hitDelay is
 * counted from the player's motion (frame halved).
 */
void GunlanceBlast_Setup(ObjAttach *blast)
{
    u8 *blastBuf = blast->buf;
    Player *pl = &gPlayers[blast->idx];
    s32 blend;

    blast->state++;
    blast->visible = 1;
    blast->draw = NULL;
    blast->aux = 0;
    Mtx_Copy((f32 *) blastBuf, (f32 *) (Em_GetBone(pl, 14)));
    ObjAttach_StartOwnedAttack(blast, (s32) pl, blast->type, &GunlanceBlast_RowsPtr);
    blast->hitList = GunlanceBlast_HitShapes[blast->hitListNo];
    blast->hurtList = 0;
    blast->area = pl->area;
    if (blast->hitList->bone == HIT_BONE_SEGMENT) {
        GunlanceBlast_PlaceOnWeapon(blast, pl);
    }
    switch (blast->type) {
    case 1:
    case 2:
    case 3:
        blast->power = GunlanceBlast_ShellPower[pl->bladeSpecial];
        if (pl->sharpnessLevel <= 1) {
            blast->power = 0.75f * blast->power;
        }
        GunlanceBlast_SplitFire(blast, 0.5f);
        break;
    default:
        blast->power = GunlanceBlast_WyvernFirePower[pl->bladeSpecial / 3];
        GunlanceBlast_SplitFire(blast, 0.4f);
        break;
    }
    if (blast->hitDelay != 0) {
        blend = Math_Absf(pl->motionBlend[0] % 100);
        blast->hitDelay += blend - (s32) (Math_Absf(*(u16 *) &pl->motionFrame[0]) / 2.0f) - 1;
        if ((s8) blast->hitDelay < 0) {
            blast->hitDelay = 0;
        }
    }
}

/*
 * Keeps the blast's window open one more frame for each frame its player
 * spends in hit stop (hitStop or hitSlowTimer). The blast is declared as
 * Em *.
 */
static void GunlanceBlast_PauseInHitStop(Em *blast)
{
    Player *pl;

    pl = &gPlayers[((ObjAttach *) blast)->idx];
    if ((pl->hitStop != 0) || (pl->hitSlowTimer != 0)) {
        ((ObjAttach *) blast)->hitFrames++;
    }
}

/*
 * Types 1-3 switch to the hit shapes four lists on after 4 frames, put on the
 * weapon again. The window is held through the player's hit stop, and the
 * blast ends when the player's motion changes.
 */
void GunlanceBlast_Run(ObjAttach *blast)
{
    Player *pl = &gPlayers[blast->idx];

    switch (blast->type) {
    case 1:
    case 2:
    case 3:
        blast->aux++;
        if (blast->aux >= 4 && blast->stateVar0 == 0) {
            blast->stateVar0++;
            blast->hitListNo += 4;
            blast->hitList = GunlanceBlast_HitShapes[blast->hitListNo];
            GunlanceBlast_PlaceOnWeapon(blast, pl);
        }
        break;
    case 4:
        break;
    }
    GunlanceBlast_PauseInHitStop((Em *) blast);
    if (pl->motionId[0] != blast->ownerMotion) {
        blast->hitPhase = 0;
        blast->state = ATTACH_END;
    }
}

/*
 * Ends the blast: hides it; it is freed on the next update (it has no draw
 * slot).
 */
void GunlanceBlast_End(ObjAttach *blast)
{
    blast->state = ATTACH_FREE;
    blast->visible = 0;
}

/* Frees the blast (ObjAttach_Free). */
void GunlanceBlast_Free(ObjAttach *blast)
{
    ObjAttach_Free(blast);
}

/*
 * Makes the first burst of a TeostraBlastRing of type (0-4) for em, if a
 * player is in em's area. It takes em's yaw and stateRand and waits 7 frames
 * (ownerMotion -7).
 */
void TeostraBlastRing_Create(Em *em, u8 attackType)
{
    ObjAttach *burst;

    if ((Player_InCurrentArea(em) & 0xFF) && attackType <= 4) {
        if ((burst = ObjAttach_Alloc(1)) != NULL) {
            burst->idx = em->index;
            burst->isEm = em->isEm;
            burst->area = em->area;
            burst->kind = ATTACH_TEOSTRA_BLAST_RING;
            burst->type = attackType;
            burst->stateVar0 = 0;
            burst->state = ATTACH_SETUP;
            burst->update = TeostraBlastRing_Update;
            burst->owner = em;
            burst->randBits = em->stateRand;
            burst->yaw = ((Em *) burst->owner)->rotY;
            burst->ownerMotion = -7;
            burst->side = *(u8 *) &em->side;
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
    ObjAttach *burst;

    if ((Player_InCurrentArea(src->owner) & 0xFF) && src->type <= 4) {
        if ((burst = ObjAttach_Alloc(1)) != NULL) {
            burst->idx = src->idx;
            burst->isEm = src->isEm;
            burst->area = src->area;
            burst->kind = ATTACH_TEOSTRA_BLAST_RING;
            burst->type = src->type;
            burst->stateVar0 = index;
            burst->state = ATTACH_SETUP;
            burst->update = TeostraBlastRing_Update;
            burst->owner = src->owner;
            Vec_Copy(burst->pos, pos);
            burst->randBits = src->randBits;
            *(u16 *) &burst->yaw = *(u16 *) &src->yaw;
            burst->ownerMotion = 0;
            burst->side = src->side;
        }
    }
}

/*
 * The burst's per-frame update, by ObjAttach.state: a hit or a deflection
 * (states 3, 4, 5, 7, 9) keeps it running.
 */
void TeostraBlastRing_Update(ObjAttach *burst)
{
    switch (burst->state) {
    case ATTACH_SETUP:
        TeostraBlastRing_Setup(burst);
        break;
    case ATTACH_RUN:
        TeostraBlastRing_Run(burst);
        break;
    case ATTACH_END:
        TeostraBlastRing_End(burst);
        break;
    case ATTACH_HIT:
        TeostraBlastRing_Run(burst);
        break;
    case 4:
        TeostraBlastRing_Run(burst);
        break;
    case ATTACH_DEFLECTED:
        TeostraBlastRing_Run(burst);
        break;
    case ATTACH_DEFLECTED_AURA:
        TeostraBlastRing_Run(burst);
        break;
    case 7:
        TeostraBlastRing_Run(burst);
        break;
    case ATTACH_FREE:
        TeostraBlastRing_Free(burst);
        
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
void TeostraBlastRing_Setup(ObjAttach *burst)
{
    f32 ofs[3];
    f32 boneMtx[16];
    TeostraBlastRingBuf *ringBuf = (TeostraBlastRingBuf *) burst->buf;
    f32 dist;
    f32 scatter;
    f32 angle;
    f32 y = 0.0f;

    burst->state = ATTACH_RUN;
    burst->visible = 1;
    burst->draw = NULL;
    Player_TestSetFlags(burst, ATKF_PIERCES);
    ObjAttach_StartOwnedAttack(burst, (s32) burst->owner, 1, &TeostraBlastRing_RowsPtr);
    burst->hitList = TeostraBlastRing_HitShapes;
    burst->hurtList = 0;
    switch (burst->type) {
    default:
        dist = 800.0f;
        angle = TeostraBlastRing_AnglesInOrder[burst->stateVar0];
        scatter = 400.0f;
        break;
    case 1:
        dist = 800.0f;
        angle = TeostraBlastRing_AnglesCrosswise[burst->stateVar0];
        scatter = 300.0f;
        break;
    case 2:
        dist = 1800.0f;
        angle = TeostraBlastRing_AnglesCrosswise[burst->stateVar0];
        scatter = 200.0f;
        break;
    case 3:
        dist = 400.0f;
        angle = TeostraBlastRing_AnglesInOrder[burst->stateVar0];
        scatter = 200.0f;
        break;
    }
    ofs[0] = 0.0f;
    ofs[1] = 0.0f;
    ofs[2] = dist;
    angle += (360.0f * (u16) burst->yaw) / 65536.0f;
    Vec_RotateY(ofs, (6.2831855f * angle) / 360.0f);
    ofs[0] += scatter * TeostraBlastRing_ScatterFromBits(burst->randBits * burst->stateVar0, 8);
    ofs[1] += y;
    ofs[2] += scatter * TeostraBlastRing_ScatterFromBits(burst->randBits * burst->stateVar0, 14);
    ringBuf->phase = 0;
    ringBuf->rotZ = 0.5f + (65536.0f * (360.0f * Rand_Unit())) / 360.0f;
    ringBuf->scale = 0.01f * (100.0f + 20.0f * Rand_Unit());
    ringBuf->seDone = 0;
    if (burst->stateVar0 <= 0) {
        burst->hitDelay += 7;
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(burst->owner, 1)));
        Mtx_GetTrans(burst->pos, boneMtx);
        Vec_Copy(ringBuf->origin, burst->pos);
    } else {
        Vec_Copy(ringBuf->origin, burst->pos);
    }
    burst->pos[0] += ofs[0];
    burst->pos[1] += ofs[1];
    burst->pos[2] += ofs[2];
    burst->drawSlot = DrawSlot_Alloc();
    if (burst->drawSlot != -1) {
        burst->drawSlotBuf = DrawSlot_Get(burst->drawSlot);
        burst->drawSlotBuf->owner = burst;
        burst->drawSlotBuf->draw = TeostraBlastRing_Draw;
    } else {
        ObjAttach_Free(burst);
        return;
    }
    TeostraBlastRing_Run(burst);
}

/*
 * Counts the burst through its phases (TeostraBlastRing_PhaseFrames), from
 * the first positive frame. When phase 0 ends it plays its sound (36) once
 * and spawns the next burst at the origin, up to the eighth. Phase 2 gets a
 * random spin and rise and rises; after it the burst ends. The draw slot is
 * queued every frame; without one the burst ends.
 */
void TeostraBlastRing_Run(ObjAttach *burst)
{
    TeostraBlastRingBuf *ringBuf = (TeostraBlastRingBuf *) burst->buf;
    f32 next;

    if (burst->drawSlotBuf == NULL) {
        burst->state = ATTACH_END;
        return;
    }
    if (++burst->ownerMotion <= 0) {
        return;
    }
    if (burst->ownerMotion > TeostraBlastRing_PhaseFrames[ringBuf->phase]) {
        switch (ringBuf->phase) {
        case 0:
            burst->ownerMotion = 1;
            ringBuf->phase = 1;
            if (ringBuf->seDone == 0) {
                if (burst->area == gScene.area) {
                    Em_PlaySeChecked(burst->owner, 36, 0, burst->pos, 15, 0);
                }
                ringBuf->seDone++;
            }
            if (burst->stateVar0 < 7) {
                next = burst->stateVar0 + 1;
                TeostraBlastRing_SpawnNext(burst, ringBuf->origin, next);
            }
            break;
        case 1:
            burst->ownerMotion = 1;
            ringBuf->phase = 2;
            ringBuf->rotZ = 0.5f + (65536.0f * (360.0f * Rand_Unit())) / 360.0f;
            ringBuf->spin = 0.5f + (65536.0f * ((10.0f * Rand_Signed()) / TeostraBlastRing_PhaseFrames[2])) / 360.0f;
            ringBuf->rise = (100.0f + 100.0f * Rand_Unit()) / TeostraBlastRing_PhaseFrames[2];
            break;
        case 2:
        default:
            burst->state = ATTACH_END;
            return;
        }
    } else {
        switch (ringBuf->phase) {
        case 0:
        case 1:
            break;
        case 2:
            ringBuf->rotZ += ringBuf->spin;
            burst->pos[1] += ringBuf->rise;
            break;
        }
    }
    Vec_Copy(burst->drawSlotBuf->pos, burst->pos);
    OT_AddByDepth(gDrawOtSorted, burst->drawSlotBuf, 64U, 0);
}

/*
 * Ends the burst: closes its hit window, hides it and frees its draw slot. It
 * is freed on the next update.
 */
void TeostraBlastRing_End(ObjAttach *burst)
{
    burst->hitPhase = 0;
    burst->state = ATTACH_FREE;
    burst->visible = 0;
    if (burst->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) burst->drawSlot);
    }
}

/* Frees the burst (ObjAttach_Free). */
void TeostraBlastRing_Free(ObjAttach *burst)
{
    ObjAttach_Free(burst);
}

/*
 * Returns the monster model slot (EmSlot_FindKind) of the first kind of
 * TeostraBlastRing_ModelKinds that is loaded (Lunastra, then Teostra), or -1
 * when neither is.
 */
s16 TeostraBlastRing_FindModelSlot(void)
{
    u8 *kinds = TeostraBlastRing_ModelKinds;
    u8 kind;
    s16 slot;

    for (kind = *kinds; kind != 255; kind = *++kinds) {
        slot = EmSlot_FindKind(kind);
        if (slot != -1) {
            return slot;
        }
    }
    return -1;
}

/*
 * Draws a burst at its draw slot, facing the camera, while it is in the
 * current area. Phases 0 and 1 use models of the common stage set (phase 1
 * steps a 4 by 4 texture flipbook by its frame count); phase 2 uses the
 * loaded Lunastra's or Teostra's own model (TeostraBlastRing_FindModelSlot)
 * and draws nothing when neither is loaded. The phase's scale and colour keys
 * are read at its frame count (ObjAttach.ownerMotion, which this family uses
 * to count a phase's frames); the x and y scale are multiplied by
 * TeostraBlastRingBuf.scale, the model is turned by rotZ, and depth writes
 * are off while it draws.
 */
void TeostraBlastRing_Draw(DrawSlot *drawSlot)
{
    u32 col;
    f32 scale[3];
    f32 rot[3];
    f32 mtx[16];
    f32 uvMtx[16];
    ObjAttach *burst;
    TeostraBlastRingBuf *ringBuf;
    Model *bank;
    u8 *matSet;
    Vec3Key *scaleKeys;
    ColorKey *colorKeys;
    u16 flags;
    s16 i;
    s8 frame;

    bank = gStageModelSet;
    burst = drawSlot->owner;
    ringBuf = (TeostraBlastRingBuf *) burst->buf;
    flags = 0;
    if (burst->area != gScene.area) {
        return;
    }
    switch (ringBuf->phase) {
    case 0:
        matSet = (u8 *) &bank->matSets[81];
        flags |= 2;
        scaleKeys = TeostraBlastRing_ScaleKeysStart;
        colorKeys = TeostraBlastRing_ColorKeysStart;
        break;
    case 1:
        matSet = (u8 *) &bank->matSets[78];
        frame = burst->ownerMotion;
        flags |= 2;
        scaleKeys = TeostraBlastRing_ScaleKeysMiddle;
        colorKeys = TeostraBlastRing_ColorKeysMiddle;
        Mtx_Translation(uvMtx, 0.25f * (frame & 3), 0.25f * (frame / 4), 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        break;
    case 2:
        i = TeostraBlastRing_FindModelSlot();
        if (i == -1) {
            return;
        }
        bank = gScene.partModels[i];
        flags |= 4;
        scaleKeys = TeostraBlastRing_ScaleKeysEnd;
        colorKeys = TeostraBlastRing_ColorKeysEnd;
        matSet = (u8 *) &bank->matSets[1];
        break;
    }
    if (bank != NULL && bank->active != 0) {
        bank = (Model *) bank->mats;
        rot[0] = 0.0f;
        rot[1] = 0.0f;
        rot[2] = 6.2831855f * ringBuf->rotZ / 65536.0f;
        Key_LerpVec3((f32 *) scaleKeys, (f32 *) scale, burst->ownerMotion);
        scale[0] *= ringBuf->scale;
        scale[1] *= ringBuf->scale;
        Mtx_FromScaleRotTrans(scale, rot, drawSlot->pos, 3, mtx);
        Mtx_Concat3x3(mtx, gCamMatrix);
        Color_EvalKeys((s32 *) colorKeys, burst->ownerMotion, &col);
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        Gs_SetState(RS_COLOR2, col);
        Gs_SetState(RS_ZWRITE, 0);
        Effect_DrawModel(matSet, mtx, col, flags, bank);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/*
 * Returns a scatter from bits: its low 16 - |16 - bitPos| bits shifted up by
 * |16 - bitPos| and divided by 64000 (0 to about 1.02), negative when that
 * shift is even. TeostraBlastRing_Setup passes randBits times the burst's
 * number, with bitPos 8 for x and 14 for z.
 */
f32 TeostraBlastRing_ScatterFromBits(u16 bits, u16 bitPos)
{
    s32 sign = -1;
    u16 shift = Math_Absf(16 - bitPos);
    u16 mask = 0xFFFF >> shift;

    if (shift & 1) {
        sign = 1;
    }
    return 0.000015625f * sign * ((bits & mask) << shift);
}

/*
 * Makes a ChameleosAttack of type attackType for em when a player is in em's
 * area: it takes em's index, motion, facing and side, and clears em's
 * hitPending and contactHit. Types 4 and 7 face backward (yaw turned half a turn).
 */
void ChameleosAttack_Create(Em *em, s32 attackType)
{
    ObjAttach *attack;

    if ((u8) Player_InCurrentArea(em) != 0 && (attack = ObjAttach_Alloc(0)) != NULL) {
        attack->kind = ATTACH_CHAMELEOS_ATTACK;
        attack->type = attackType;
        attack->update = ChameleosAttack_Update;
        attack->idx = em->index;
        attack->isEm = em->isEm;
        attack->ownerMotion = em->motionId[0];
        attack->owner = em;
        attack->yaw = em->rotY;
        switch (attackType) {
        case 4:
        case 7:
            *(u16 *) &attack->yaw += 0x8000;
            break;
        }
        attack->rotX = em->rotX;
        attack->rotY = em->rotY;
        attack->rotZ = em->rotZ;
        attack->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->contactHit = 0;
    }
}

/*
 * The attack's update, run every frame: switches on ObjAttach.state;
 * ATTACH_HIT keeps it running, states 4, ATTACH_DEFLECTED, 7 and
 * ATTACH_DEFLECTED_AURA end it.
 */
void ChameleosAttack_Update(ObjAttach *attack)
{
    switch (attack->state) {
    case ATTACH_SETUP:
        ChameleosAttack_Setup(attack);
        break;
    case ATTACH_RUN:
        ChameleosAttack_Run(attack);
        break;
    case ATTACH_END:
        ChameleosAttack_End(attack);
        break;
    case ATTACH_HIT:
        ChameleosAttack_Run(attack);
        break;
    case 4:
        ChameleosAttack_End(attack);
        break;
    case ATTACH_DEFLECTED:
        ChameleosAttack_End(attack);
        break;
    case ATTACH_DEFLECTED_AURA:
        ChameleosAttack_End(attack);
        break;
    case 7:
        ChameleosAttack_End(attack);
        break;
    case ATTACH_FREE:
        ChameleosAttack_Free(attack);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from row type of its attack table with
 * ATKF_OWNER_CONTACT: its hit shapes (ChameleosAttack_HitShapes) and em's
 * area. A nonzero hitDelay is counted from the monster's motion (its blend
 * count less its frame, on layer 1 for type 3, else layer 0) and kept at 0 or
 * more; it gets no draw slot.
 */
void ChameleosAttack_Setup(ObjAttach *attack)
{
    Em *em = &gEms[attack->idx];
    s32 blend;
    s32 frame;

    attack->state++;
    attack->visible = 1;
    attack->draw = NULL;
    Player_TestSetFlags(attack, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(attack, (s32) em, attack->type, &ChameleosAttack_RowsPtr);
    attack->hitList = ChameleosAttack_HitShapes[attack->hitListNo];
    attack->hurtList = 0;
    attack->area = em->area;
    if (attack->hitDelay != 0) {
        if (attack->type == 3) {
            blend = Math_Absf((s16) em->motionBlend[1] % 100);
            frame = Math_Absf(em->motionFrame[1]);
        } else {
            blend = Math_Absf((s16) em->motionBlend[0] % 100);
            frame = Math_Absf(em->motionFrame[0]);
        }
        attack->hitDelay += blend - frame - 1;
        if ((s8) attack->hitDelay < 0) {
            attack->hitDelay = 0;
        }
    }
    attack->drawSlotBuf = NULL;
}

/*
 * Ends the attack once the monster's motion is no longer the one that made it
 * or its hit window closed; type 6 holds its window open (hitFrames 99) and
 * type 9 also lives through CHAM_MOT_SPIT_A and CHAM_MOT_SPIT_B_END. A draw slot would follow
 * it, but Setup gives it none.
 */
void ChameleosAttack_Run(ObjAttach *attack)
{
    Em *em = &gEms[attack->idx];

    switch (attack->type) {
    case 6:
        attack->hitFrames = 99;
        break;
    }
    switch (attack->type) {
    case 9:
        if ((em->motionId[0] != EM_MOTION_ID(0, CHAM_MOT_SPIT_A) && em->motionId[0] != EM_MOTION_ID(0, CHAM_MOT_SPIT_B_END) && em->motionId[0] != attack->ownerMotion) || attack->hitPhase == 0) {
            attack->hitPhase = 0;
            attack->state = ATTACH_END;
        }
        break;
    default:
        if (em->motionId[0] != attack->ownerMotion || attack->hitPhase == 0) {
            attack->hitPhase = 0;
            attack->state = ATTACH_END;
        }
        break;
    }
    if (attack->drawSlotBuf != NULL) {
        Vec_Copy(attack->drawSlotBuf->pos, attack->pos);
        OT_AddByDepth(gDrawOt32, attack->drawSlotBuf, 32, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot if
 * it has one; it is freed on the next update.
 */
void ChameleosAttack_End(ObjAttach *attack)
{
    attack->hitPhase = 0;
    attack->state = ATTACH_FREE;
    attack->visible = 0;
    if (attack->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) attack->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void ChameleosAttack_Free(ObjAttach *attack)
{
    ObjAttach_Free(attack);
}

/*
 * Makes a RemobraAttack of type attackType for em when a player is in em's
 * area (type 1 from EmRemobra_Strike, 2 and 3 from EmRemobra_SwoopStrike):
 * it takes em's index, motion, facing and side, and clears em's hitPending
 * and contactHit.
 */
void RemobraAttack_Create(Em *em, u8 attackType)
{
    ObjAttach *attack;

    if ((u8) Player_InCurrentArea(em) != 0 && (attack = ObjAttach_Alloc(0)) != NULL) {
        attack->kind = ATTACH_HERMITAUR_ATTACK;
        attack->type = attackType;
        attack->update = RemobraAttack_Update;
        attack->idx = em->index;
        attack->isEm = em->isEm;
        attack->ownerMotion = em->motionId[0];
        attack->owner = em;
        attack->yaw = em->rotY;
        attack->rotX = em->rotX;
        attack->rotY = em->rotY;
        attack->rotZ = em->rotZ;
        attack->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->contactHit = 0;
    }
}

/*
 * The attack's update, run every frame: switches on ObjAttach.state;
 * ATTACH_HIT keeps it running, states 4, ATTACH_DEFLECTED, 7 and
 * ATTACH_DEFLECTED_AURA end it.
 */
void RemobraAttack_Update(ObjAttach *attack)
{
    switch (attack->state) {
    case ATTACH_SETUP:
        RemobraAttack_Setup(attack);
        break;
    case ATTACH_RUN:
        RemobraAttack_Run(attack);
        break;
    case ATTACH_END:
        RemobraAttack_End(attack);
        break;
    case ATTACH_HIT:
        RemobraAttack_Run(attack);
        break;
    case 4:
        RemobraAttack_End(attack);
        break;
    case ATTACH_DEFLECTED:
        RemobraAttack_End(attack);
        break;
    case ATTACH_DEFLECTED_AURA:
        RemobraAttack_End(attack);
        break;
    case 7:
        RemobraAttack_End(attack);
        break;
    case ATTACH_FREE:
        RemobraAttack_Free(attack);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from row type of its attack table with
 * ATKF_OWNER_CONTACT, at the monster's position with hitScale 1.0: its hit
 * shapes (RemobraAttack_HitShapes) and em's area. A nonzero hitDelay is
 * counted from the monster's motion on layer 0 and kept at 0 or more; it gets
 * no draw slot.
 */
void RemobraAttack_Setup(ObjAttach *attack)
{
    Em *em = &gEms[attack->idx];

    attack->state++;
    attack->visible = 1;
    attack->draw = NULL;
    Player_TestSetFlags(attack, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(attack, (s32) em, attack->type, &RemobraAttack_RowsPtr);
    attack->hitList = RemobraAttack_HitShapes[attack->hitListNo];
    attack->hurtList = 0;
    attack->area = em->area;
    Vec_Copy(attack->pos, (f32 *)&em->posX);
    attack->hitScale = 1.0f;
    if (attack->hitDelay != 0) {
        attack->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) attack->hitDelay < 0) {
            attack->hitDelay = 0;
        }
    }
    attack->drawSlotBuf = NULL;
}

/*
 * Ends the attack once the monster's motion is no longer the one that made it
 * or its hit window closed. A draw slot would follow it, but Setup gives it
 * none.
 */
void RemobraAttack_Run(ObjAttach *attack)
{
    Em *em = &gEms[attack->idx];

    if (em->motionId[0] != attack->ownerMotion || attack->hitPhase == 0) {
        attack->hitPhase = 0;
        attack->state = ATTACH_END;
    }
    if (attack->drawSlotBuf != NULL) {
        Vec_Copy(attack->drawSlotBuf->pos, attack->pos);
        OT_AddByDepth(gDrawOt32, attack->drawSlotBuf, 32U, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot if
 * it has one; it is freed on the next update.
 */
void RemobraAttack_End(ObjAttach *attack)
{
    attack->hitPhase = 0;
    attack->state = ATTACH_FREE;
    attack->visible = 0;
    if (attack->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) attack->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void RemobraAttack_Free(ObjAttach *attack)
{
    ObjAttach_Free(attack);
}

/* The file's data. */
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
AttackRow GunlanceBlast_Rows[5] = {
#include "game/008AEAC0.inc"
};
void *GunlanceBlast_RowsPtr = (void *)GunlanceBlast_Rows;
u8 GunlanceBlast_ShellPower[16] = {
#include "game/008AEB40.inc"
};
u8 GunlanceBlast_WyvernFirePower[16] = {
#include "game/008AEB50.inc"
};
HitShape TeostraBlastRing_HitShapeList[2] = {
#include "game/008AEB60.inc"
};
HitShape *TeostraBlastRing_HitShapes = TeostraBlastRing_HitShapeList;
AttackRow TeostraBlastRing_Rows[2] = {
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
Vec3Key TeostraBlastRing_ScaleKeysStart[4] = {
#include "game/008AEC50.inc"
};
ColorKey TeostraBlastRing_ColorKeysStart[4] = {
#include "game/008AEC90.inc"
};
Vec3Key TeostraBlastRing_ScaleKeysMiddle[3] = {
#include "game/008AECB0.inc"
};
ColorKey TeostraBlastRing_ColorKeysMiddle[4] = {
#include "game/008AECE0.inc"
};
Vec3Key TeostraBlastRing_ScaleKeysEnd[3] = {
#include "game/008AED00.inc"
};
ColorKey TeostraBlastRing_ColorKeysEnd[5] = {
#include "game/008AED30.inc"
};
u8 TeostraBlastRing_ModelKinds[8] = {
#include "game/008AED58.inc"
};

HitShape ChameleosAttack_HitShapes_0[2] = {
#include "game/008AED60.inc"
};

HitShape ChameleosAttack_HitShapes_1[3] = {
#include "game/008AEDB0.inc"
};

HitShape ChameleosAttack_HitShapes_2[3] = {
#include "game/008AEE30.inc"
};

HitShape ChameleosAttack_HitShapes_3[7] = {
#include "game/008AEEB0.inc"
};

HitShape ChameleosAttack_HitShapes_4[3] = {
#include "game/008AEFD0.inc"
};

HitShape ChameleosAttack_HitShapes_5[2] = {
#include "game/008AF050.inc"
};

HitShape ChameleosAttack_HitShapes_6[4] = {
#include "game/008AF0A0.inc"
};

HitShape *ChameleosAttack_HitShapes[8] = {
    ChameleosAttack_HitShapes_0, ChameleosAttack_HitShapes_1, ChameleosAttack_HitShapes_2, ChameleosAttack_HitShapes_3,
    ChameleosAttack_HitShapes_4, ChameleosAttack_HitShapes_5, ChameleosAttack_HitShapes_6, 0,
};

AttackRow ChameleosAttack_Rows[10] = {
#include "game/008AF160.inc"
};

void *ChameleosAttack_RowsPtr = (void *)ChameleosAttack_Rows;

HitShape RemobraAttack_HitShapes_0[4] = {
#include "game/008AF260.inc"
};

HitShape RemobraAttack_HitShapes_1[3] = {
#include "game/008AF300.inc"
};

HitShape *RemobraAttack_HitShapes[2] = {
    RemobraAttack_HitShapes_0, RemobraAttack_HitShapes_1,
};

AttackRow RemobraAttack_Rows[4] = {
#include "game/008AF380.inc"
};

void *RemobraAttack_RowsPtr = (void *)RemobraAttack_Rows;
