/*
 * The attack objects the weapon states and the monsters make: the first six
 * families of ObjAttach (enum ObjAttachKind, ATTACH_PLAYER_ATTACK to
 * ATTACH_LAOSHAN_ATTACK), each with its Create, Update (switching on
 * ObjAttach.state, enum ObjAttachState), Setup, Run, End and Free. An
 * attack's type is a row of its family's Rows table (the hit's delay, length,
 * power, kind and element, copied over the object), and its hit shapes a
 * list of its HitShapes table:
 * - PlayerAttack: a player's weapon hit, made by the weapon states;
 * - RathalosAttack, FatalisAttack, LaoShanAttack: those monsters' body
 *   attacks (made by those monsters' states and by stage objects);
 * - EmContactAttack: Kirin's, Melynx's, Anteka's and Mosswine's attacks,
 *   which report what they touch to their owner;
 * - ItemAttack: an item a player throws or puts down (enum ItemAttackMode).
 */

#include "common.h"
#include "em.h"
#include "objhead.h"
#include "objattach.h"
#include "player.h"
#include "sprite.h"
#include "capsule.h"
#include "render_state.h"
#include "area.h"
#include "quest.h"
#include "sound.h"
#include "attack_objects.h"
#include "draw_slots.h"
#include "engine_math.h"
#include "hit_effects.h"
#include "hit_setup.h"
#include "part_effects.h"
#include "place_rules.h"
#include "rand.h"
#include "hit_spark.h"
#include "vec_math.h"
#include "save.h"

extern void *PlayerAttack_RowsPtr;
extern HitShape *PlayerAttack_HitShapes[];
extern s16 ItemAttack_Models[];
extern s8 ItemAttack_HitsInFlight[];
extern f32 ItemAttack_ThrowSpeedsAlt[][2];
extern f32 ItemAttack_ThrowSpeeds[][2];
extern f32 ItemAttack_LobSpeeds[][2];
extern f32 ItemAttack_ScaleKeysA[];
extern f32 ItemAttack_ScaleKeysB[];
extern ColorKey ItemAttack_ColorKeysA[];
extern ColorKey ItemAttack_ColorKeysB[];
extern s16 ItemAttack_PartModels[];
extern f32 gCamMatrix[];
extern f32 gCamYawMtx[];
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
extern s8 ItemAttack_HasSide[];
/* ItemAttack's two parts (ITEMATK_NO_BOMB_AREA_ITEM): its buffer's 16-byte blocks, each with
 * its own draw slot (ItemAttack_InitParts, ItemAttack_AnimParts,
 * ItemAttack_DrawPart). */
typedef struct AttachPart {
    s16 slot;       /* the draw slot (DrawSlotB_Alloc), -1: none */
    s16 idx;        /* 0 the low part, 1 the high one (+1060) */
    DrawSlot *node; /* DrawSlotB_Get(slot), NULL: none */
    s16 frame;      /* the animation frame, 1-4 (scale keys, the texture's quarter) */
    s16 time;       /* frames since the start (colour keys) */
    s16 unk0C;
    s16 unk0E;
} AttachPart;

/* the calls cast to u8: this unit's code takes the value as u8 */

extern s8 ItemAttack_Modes[];
extern HitShape *FatalisAttack_HitShapes[];
extern void *FatalisAttack_RowsPtr;
extern HitShape *EmContactAttack_HitShapes[];
extern void *EmContactAttack_RowsPtr;
extern HitShape *LaoShanAttack_HitShapes[];
extern void *LaoShanAttack_RowsPtr;
extern HitShape *RathalosAttack_HitShapes[];
extern void *RathalosAttack_RowsPtr;
void ObjAttach_Move(ObjAttach *);
void ObjAttach_MoveAccel(ObjAttach *);
extern HitShape *ItemAttack_HitShapes[];
extern void *ItemAttack_RowsPtr;
extern s8 ItemAttack_AttackRow[];
/*
 * Makes a PlayerAttack for pl: an ObjAttach that follows pl and hits with the
 * row attackId of PlayerAttack_Rows from pl's current motion. Clears pl's
 * hitPending and contactHit for the new attack.
 */
void PlayerAttack_Create(Player *pl, u8 attackId)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_PLAYER_ATTACK;
        obj->type = attackId;
        obj->update = PlayerAttack_Update;
        obj->idx = pl->index;
        obj->isEm = pl->isEm;
        obj->ownerMotion = pl->motionId[0];
        obj->owner = pl;
        obj->yaw = pl->rot[1];
        obj->randBits = pl->stateRand;
        obj->stateVar0 = 0;
        obj->stateVar1 = 0;
        obj->rotX = pl->rot[0];
        obj->rotY = pl->rot[1];
        obj->rotZ = pl->rot[2];
        Vec_Copy(obj->pos, &pl->pos[0]);
        obj->side = *(u8 *) &pl->side;
        obj->unk080 = 0;
        pl->hitPending = 0;
        pl->contactHit = 0;
    }
}

/*
 * PlayerAttack_Create for the dual swords: mirrored (bit 0) swaps the hit
 * shape lists 9 and 10, the two blades (PlayerAttack_Setup). Leaves
 * contactHit alone.
 */
void PlayerAttack_CreateDual(Player *pl, s32 attackId, s32 mirrored)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_PLAYER_ATTACK;
        obj->type = attackId;
        obj->update = PlayerAttack_Update;
        obj->idx = pl->index;
        obj->isEm = pl->isEm;
        obj->ownerMotion = pl->motionId[0];
        obj->owner = pl;
        obj->yaw = pl->rot[1];
        obj->randBits = pl->stateRand;
        obj->stateVar0 = 0;
        obj->stateVar1 = mirrored;
        obj->rotX = pl->rot[0];
        obj->rotY = pl->rot[1];
        obj->rotZ = pl->rot[2];
        Vec_Copy(obj->pos, &pl->pos[0]);
        obj->side = *(u8 *) &pl->side;
        obj->unk080 = 0;
        pl->hitPending = 0;
    }
}

/*
 * PlayerAttack_Create for the bow's close stab (attack ids 60, 61): it
 * carries the loaded coating, pl->ammoType, which PlayerAttack_Setup applies
 * (BowArrow_ApplyCoating).
 */
void PlayerAttack_CreateBowStab(Player *pl, s8 attackId)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_PLAYER_ATTACK;
        obj->type = attackId;
        obj->update = PlayerAttack_Update;
        obj->idx = pl->index;
        obj->isEm = pl->isEm;
        obj->ownerMotion = pl->motionId[0];
        obj->owner = pl;
        obj->yaw = pl->rot[1];
        obj->randBits = pl->stateRand;
        obj->stateVar1 = 0;
        obj->rotX = pl->rot[0];
        obj->rotY = pl->rot[1];
        obj->rotZ = pl->rot[2];
        Vec_Copy(obj->pos, &pl->pos[0]);
        obj->side = *(u8 *) &pl->side;
        obj->stateVar0 = pl->ammoType;
        pl->hitPending = 0;
    }
}

/*
 * The PlayerAttack's per-frame update, by ObjAttach.state. A hit (ATTACH_HIT)
 * keeps the window open two more frames and runs on.
 */
void PlayerAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        PlayerAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        PlayerAttack_Run(obj);
        break;
    case ATTACH_END:
        PlayerAttack_End(obj);
        break;
    case ATTACH_HIT:
        obj->state = ATTACH_RUN;
        (obj)->hitFrames += 2;
        PlayerAttack_Run(obj);
        break;
    case 4:
        PlayerAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        PlayerAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        PlayerAttack_End(obj);
        break;
    case 7:
        PlayerAttack_End(obj);
        break;
    case ATTACH_FREE:
        PlayerAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Puts the attack's capsule on the weapon: pos and pos2 are the hit shape's
 * two z offsets (a[2], b[2]) along bone PL_BONE_HAND, turned by a fixed rotation. Used
 * for hit shapes whose bone is 126.
 */
void PlayerAttack_PlaceOnWeapon(ObjAttach *obj, Player *pl)
{
    f32 *iy;
    f32 *iz;
    f32 *oy;
    f32 *oz;
    f32 m0[16];
    f32 m1[16];
    f32 in[3];
    f32 out[3];

    Mtx_Copy(m0, (f32 *)Em_GetBone(pl, PL_BONE_HAND));
    Mtx_Identity(m1);
    Mtx_SetRotXYZ(m1, 0.0628318489f, -0.541052103f, -3.04734492f);
    Mtx_Concat3x3(m1, m0);
    in[0] = 0.0f;
    iy = &in[1];
    *iy = 0.0f;
    iz = &in[2];
    *iz = obj->hitList->a[2];
    Vec_Rotate(out, in, m1);
    Mtx_GetTrans(obj->pos2, m0);
    oy = &out[1];
    oz = &out[2];
    obj->pos2[0] += out[0];
    obj->pos2[1] += *oy;
    obj->pos2[2] += *oz;
    in[0] = 0.0f;
    *iy = 0.0f;
    *iz = obj->hitList->b[2];
    Vec_Rotate(out, in, m1);
    Mtx_GetTrans(obj->pos, m0);
    obj->pos[0] += out[0];
    obj->pos[1] += *oy;
    obj->pos[2] += *oz;
}

/*
 * Sets the attack up from its row (ObjAttach_StartOwnedAttack copies it over
 * hitDelay..mvFramesB): the hit shapes, the attack flags, the weapon's element
 * (and, one hit in three, its status) and the affinity's roll (a critical hit
 * 1.25 times the power, a weak one half; only from sharpness level 3). The
 * dual swords' demon mode adds 35 % power. hitDelay is counted from the
 * motion's blend and current frame.
 */
void PlayerAttack_Setup(ObjAttach *obj)
{
    Player *pl = &gPlayers[obj->idx];
    s16 stat;
    s32 blendFrames;
    f32 in[3];
    f32 out[3];
    u8 takesElement;

    takesElement = 0;
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    ObjAttach_StartOwnedAttack(obj, (s32) pl, obj->type, &PlayerAttack_RowsPtr);
    if (obj->stateVar1 & 1) {
        switch (obj->hitListNo) {
        case 9:
            obj->hitListNo = 10;
            break;
        case 10:
            obj->hitListNo = 9;
            break;
        }
    }
    obj->hitList = PlayerAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->area = pl->area;
    if (obj->hitList->bone == HIT_BONE_SEGMENT) {
        PlayerAttack_PlaceOnWeapon(obj, pl);
    }
    switch (obj->type) {
    case 11:
    case 86:
        in[0] = 0.0f;
        in[1] = 0.0f;
        in[2] = 200.0f;
        Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) pl->worldMtx);
        obj->pos[0] += out[0];
        obj->pos[1] += out[1];
        obj->pos[2] += out[2];
        obj->aux = 4;
        Player_TestSetFlags(obj, ATKF_OWNER_CONTACT | ATKF_WEAPON);
        takesElement = 1;
        break;
    case 62:
        in[0] = 117.0f;
        in[1] = 21.0f;
        in[2] = 187.0f;
        Vec_Rotate(out, in, (f32 *)Em_GetBone(pl, PL_BONE_HAND));
        Mtx_GetTrans(obj->pos, Em_GetBone(pl, PL_BONE_HAND)->world);
        takesElement = 1;
        obj->pos[0] += out[0];
        obj->pos[1] += out[1];
        obj->pos[2] += out[2];
        break;
    case 4:
    case 5:
    case 29:
    case 96:
    case 97:
        Player_TestSetFlags(obj, ATKF_OWNER_CONTACT);
        break;
    case 60:
    case 61:
        Player_TestSetFlags(obj, ATKF_OWNER_CONTACT | ATKF_WEAPON);
        BowArrow_ApplyCoating(obj, obj->stateVar0, 1);
        break;
    default:
        Player_TestSetFlags(obj, ATKF_OWNER_CONTACT | ATKF_WEAPON);
        takesElement = 1;
        break;
    }
    if (takesElement) {
        stat = Player_GetWeaponStat(pl, WSTAT_FIRE);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_FIRE;
            obj->elementPower = stat;
        }
        stat = Player_GetWeaponStat(pl, WSTAT_WATER);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_WATER;
            obj->elementPower = stat;
        }
        stat = Player_GetWeaponStat(pl, WSTAT_THUNDER);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_THUNDER;
            obj->elementPower = stat;
        }
        stat = Player_GetWeaponStat(pl, WSTAT_ICE);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_ICE;
            obj->elementPower = stat;
        }
        stat = Player_GetWeaponStat(pl, WSTAT_DRAGON);
        if (stat > 0) {
            obj->elementFlags |= ELEMF_DRAGON;
            obj->elementPower = stat;
        }
        if (pl->stateRand % 3 == 0) {
            stat = Player_GetWeaponStat(pl, WSTAT_POISON);
            if (stat > 0) {
                obj->elementFlags |= ELEMF_POISON;
                obj->elementPower = stat;
            }
            stat = Player_GetWeaponStat(pl, WSTAT_STATUS_TYPE2);
            if (stat > 0) {
                obj->elementFlags |= ELEMF_EM_STATUS4;
                obj->elementPower = stat;
            }
            stat = Player_GetWeaponStat(pl, WSTAT_STATUS_TYPE3);
            if (stat > 0) {
                obj->elementFlags |= ELEMF_EM_STATUS2;
                obj->elementPower = stat;
            }
        }
    }
    obj->hitFx &= HITFX_KIND_MASK;
    stat = Player_GetWeaponStat(pl, WSTAT_AFFINITY);
    if (stat != 0 && pl->sharpnessLevel >= 3) {
        if (stat < 0) {
            if (pl->stateRand % 100 < -stat) {

                obj->power >>= 1;
                obj->hitFx |= HITFX_WEAK;
            }
        } else if (stat > pl->stateRand % 100) {
            obj->power = (s32) (1.25f * obj->power);
            obj->hitFx |= HITFX_CRITICAL;
        }
    }
    if (pl->weaponType == WEAPON_DUAL_SWORDS && pl->demonMode != 0) {
        obj->mvFramesA = 0;
        obj->mvFramesB = 18;
        obj->power = 1.35f * obj->power;
    }
    if (obj->hitDelay != 0) {
        blendFrames = Math_Absf(pl->motionBlend[0] % 100);
        obj->hitDelay += blendFrames - (s32) (Math_Absf((u16) pl->motionFrame[0]) / 2.0f) - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
}

/*
 * Keeps the hit window open while the player is in hit stop or slowed after a
 * hit.
 */
static void PlayerAttack_PauseInHitStop(ObjAttach *obj)
{
    Player *pl;

    pl = &gPlayers[obj->idx];
    if ((pl->hitStop != 0) || (((Player *) pl)->hitSlowTimer != 0)) {
        (obj)->hitFrames++;
    }
}

/*
 * Follows the weapon and ends the attack when the player's motion changes
 * (attack 8 also lasts through motion 1407, 100 through 1406, 19 holds
 * its window open for the whole motion); attacks 11 and 86 last four
 * frames, 96 and 97 until their window closes.
 */
void PlayerAttack_Run(ObjAttach *obj)
{
    Player *pl = &gPlayers[obj->idx];

    if (obj->hitList->bone == HIT_BONE_SEGMENT && obj->type != 68) {
        PlayerAttack_PlaceOnWeapon(obj, pl);
    }
    PlayerAttack_PauseInHitStop(obj);
    switch (obj->type) {
    case 8:
        if (pl->motionId[0] == obj->ownerMotion || pl->motionId[0] == 1407) {
            return;
        }
        goto end;
    case 100:
        if (pl->motionId[0] == obj->ownerMotion || pl->motionId[0] == 1406) {
            return;
        }
        goto end;
    case 11:
    case 86:
        if (--obj->aux > 0) {
            return;
        }
        goto end;
    case 19:
        if (pl->motionId[0] == obj->ownerMotion) {
            obj->hitFrames = 99;
            return;
        }
        goto end;
    case 96:
    case 97:
        if (obj->hitFrames <= 0) {
            goto end;
        }
        break;
    default:
        if (pl->motionId[0] != obj->ownerMotion) {
        end:
            obj->hitPhase = 0;
            obj->state = ATTACH_END;
        }
        break;
    }
}

/* Hides the attack and frees it on the next update. */
void PlayerAttack_End(ObjAttach *obj)
{
    obj->state = ATTACH_FREE;
    obj->visible = 0;
}

/* Returns the object to the pool. */
void PlayerAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

/*
 * Makes a RathalosAttack for em with the row attackId of RathalosAttack_Rows,
 * if a player is in em's area. Clears em's hitPending and contactHit.
 */
void RathalosAttack_Create(Em *em, s8 attackId)
{
    ObjAttach *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = ObjAttach_Alloc(0)) != NULL) {
            obj->kind = ATTACH_RATHALOS_ATTACK;
            obj->type = attackId;
            obj->update = RathalosAttack_Update;
            obj->idx = em->index;
            obj->isEm = em->isEm;
            obj->ownerMotion = em->motionId[0];
            obj->owner = em;
            obj->yaw = em->rotY;
            obj->rotX = em->rotX;
            obj->rotY = em->rotY;
            obj->rotZ = em->rotZ;
            obj->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/*
 * RathalosAttack_Create at the stage object src's position, for src's owner
 * (and without the area check): the attack starts at step 1.
 */
void RathalosAttack_CreateOnObj(StageObj *src, s8 attackId)
{
    ObjAttach *obj;
    Em *em;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        em = src->owner;
        obj->kind = ATTACH_RATHALOS_ATTACK;
        obj->type = attackId;
        obj->update = RathalosAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->side = *(u8 *) &em->side;
        obj->pos[0] = src->pos[0];
        obj->pos[1] = src->pos[1];
        obj->pos[2] = src->pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
    }
}

/*
 * RathalosAttack_Create at pos (without the area check): the attack starts at
 * step 1.
 */
void RathalosAttack_CreateAt(Em *em, f32 *pos, s8 attackId)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_RATHALOS_ATTACK;
        obj->type = attackId;
        obj->update = RathalosAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->side = *(u8 *) &em->side;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
    }
}

/* The RathalosAttack's per-frame update, by ObjAttach.state. */
void RathalosAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        RathalosAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        RathalosAttack_Run(obj);
        break;
    case ATTACH_END:
        RathalosAttack_End(obj);
        break;
    case ATTACH_HIT:
        RathalosAttack_Run(obj);
        break;
    case 4:
        RathalosAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        RathalosAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        RathalosAttack_End(obj);
        break;
    case 7:
        RathalosAttack_End(obj);
        break;
    case ATTACH_FREE:
        RathalosAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row: the hit shapes and ATKF_OWNER_CONTACT;
 * hitDelay is counted from the monster's motion.
 */
void RathalosAttack_Setup(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    Player_TestSetFlags(obj, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(obj, (s32) em, obj->type, &RathalosAttack_RowsPtr);
    obj->hitList = RathalosAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->area = em->area;
    if (obj->hitDelay != 0) {
        obj->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
    obj->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes;
 * attacks 24 and 34 keep their window open.
 */
void RathalosAttack_Run(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    switch (obj->type) {
    case 24:
    case 34:
        obj->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != obj->ownerMotion || obj->hitPhase == 0) {
        obj->hitPhase = 0;
        obj->state = ATTACH_END;
    }
    if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 32U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void RathalosAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) obj->drawSlot);
    }
}

void RathalosAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

/*
 * Makes an EmContactAttack for em with the row attackId of
 * EmContactAttack_Rows, if a player is in em's area; returns it, or NULL.
 */
ObjAttach *EmContactAttack_Create(Em *em, s8 attackId)
{
    ObjAttach *obj;

    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_EM_CONTACT_ATTACK;
        obj->type = attackId;
        obj->update = EmContactAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->side = *(u8 *) &em->side;
        em->hitPending = 0;
        em->contactHit = 0;
    }
    return obj;
}

/* The EmContactAttack's per-frame update, by ObjAttach.state. */
void EmContactAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        EmContactAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        EmContactAttack_Run(obj);
        break;
    case ATTACH_END:
        EmContactAttack_End(obj);
        break;
    case ATTACH_HIT:
        EmContactAttack_Run(obj);
        break;
    case 4:
        EmContactAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        EmContactAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        EmContactAttack_End(obj);
        break;
    case 7:
        EmContactAttack_End(obj);
        break;
    case ATTACH_FREE:
        EmContactAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from row attackId + 1: the hit shapes,
 * ATKF_OWNER_CONTACT and ATKF_REPORT_CONTACT (the owner learns what it
 * touched); hitDelay is counted from the monster's motion.
 */
void EmContactAttack_Setup(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    obj->state++;
    obj->visible = 1;
    obj->draw = EmContactAttack_DrawNop;
    Player_TestSetFlags(obj, ATKF_OWNER_CONTACT | ATKF_REPORT_CONTACT);
    obj->wasHit = 0;
    obj->area = em->area;
    ObjAttach_StartOwnedAttack(obj, (s32) em, obj->type + 1, &EmContactAttack_RowsPtr);
    obj->hitList = EmContactAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    if (obj->hitDelay != 0) {
        obj->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
    obj->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes, the hit window closes or
 * the monster's task leaves its run phase; some attacks keep their window
 * open.
 */
void EmContactAttack_Run(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    switch (obj->type) {
    case 2:
    case 3:
    case 4:
    case 7:
    case 8:
    case 9:
    case 10:
    case 14:
    case 15:
    case 16:
    case 22:
        obj->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != obj->ownerMotion || obj->hitPhase == 0 || em->taskPhase >= 2) {
        obj->hitPhase = 0;
        obj->state = ATTACH_END;
    } else if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 32U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void EmContactAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) obj->drawSlot);
    }
}

void EmContactAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

void EmContactAttack_DrawNop(ObjAttach *obj)
{
}

/*
 * Makes an ItemAttack: the item type (a row of ItemAttack_Modes and the other
 * ItemAttack tables) thrown or put down by owner, a player. throwRow picks
 * the throw's speeds (ItemAttack_SetupThrown). A thrown item starts at the
 * hand (part record 18), a placed one at the feet; nothing is made for a
 * thrown item outside the current area. ITEMATK_NO_BOMB_AREA_ITEM takes the second pool and
 * plays its sound.
 */
void ItemAttack_Create(void *owner, s32 type, s8 throwRow)
{
    Player *pl = owner;
    ObjHead *head = owner;
    s16 pool;
    ObjAttach *item;
    s8 *mode = &ItemAttack_Modes[type];

    if (*mode == ITEM_PLACED || *mode == ITEM_HOPPING || (u8)Obj_InCurrentArea(pl) != 0) {
        switch (type) {
        case ITEMATK_NO_BOMB_AREA_ITEM:
            pool = 1;
            break;
        default:
            pool = 0;
            break;
        }
        if ((item = ObjAttach_Alloc(pool)) != NULL) {
            item->kind = ATTACH_ITEM_ATTACK;
            item->type = type;
            item->stateVar1 = throwRow;
            item->update = ItemAttack_Update;
            item->idx = head->index;
            item->isEm = head->isEm;
            item->ownerMotion = 0;
            item->owner = owner;
            item->yaw = head->rot[1];
            item->rotX = 0;
            item->rotY = head->rot[1];
            item->rotZ = 0;
            item->area = head->area;
            if (ItemAttack_HasSide[type] != 0) {
                item->side = head->side;
            }
            switch (*mode) {
            case ITEM_PLACED:
            case ITEM_HOPPING:
                item->pos[0] = head->pos[0];
                item->pos[1] = 21.0f + head->pos[1];
                item->pos[2] = head->pos[2];
                item->keep = 1;
                break;
            default:
                item->pos[0] = ((f32 *) pl->partRecs[18])[28];
                item->pos[1] = ((f32 *) pl->partRecs[18])[29];
                item->pos[2] = ((f32 *) pl->partRecs[18])[30];
                item->keep = 0;
                break;
            }
            if (item->type == ITEMATK_NO_BOMB_AREA_ITEM && gScene.area == item->area) {
                Sound_PlaySeAtPosChecked(1, 145, 0, item->pos, 1, 0);
            }
            item->velX = 0;
            item->velY = 0;
            item->velZ = 0;
            item->homingTurn = 0;
            item->gravity = 0;
            item->homingSpeed = 0;
            item->randBits = head->stateRand;
        }
    }
}

/*
 * The ItemAttack's per-frame update, by ObjAttach.state: a hit bursts items
 * 20 and 24 (ItemAttack_OnHit); a deflected item bounces off or flies back
 * (ItemAttack_Deflect).
 */
void ItemAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        ItemAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        ItemAttack_Run(obj);
        break;
    case ATTACH_END:
        ItemAttack_End(obj);
        break;
    case ATTACH_HIT:
        ItemAttack_OnHit(obj);
        break;
    case 4:
        ItemAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        ItemAttack_Deflect(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        ItemAttack_Deflect(obj);
        break;
    case 7:
        ItemAttack_End(obj);
        break;
    case ATTACH_FREE:
        ItemAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the item up by its mode, then takes a draw slot (the second pool for a
 * placed item) and runs the first frame; frees the object when no slot is
 * left.
 */
void ItemAttack_Setup(ObjAttach *obj)
{
    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    switch (ItemAttack_Modes[obj->type]) {
    case ITEM_PLACED:
        ItemAttack_SetupPlaced(obj);
        break;
    case ITEM_HOPPING:
        ItemAttack_SetupHopping(obj);
        break;
    default:
        ItemAttack_SetupThrown(obj);
        break;
    }
    Vec_Copy(obj->pos2, obj->pos);
    if (obj->keep == 1) {
        obj->drawSlot = DrawSlotB_Alloc();
        if (obj->drawSlot != -1) {
            obj->drawSlotBuf = DrawSlotB_Get(obj->drawSlot);
        }
    } else {
        obj->drawSlot = DrawSlot_Alloc();
        if (obj->drawSlot != -1) {
            obj->drawSlotBuf = DrawSlot_Get(obj->drawSlot);
        }
    }
    if (obj->drawSlot != -1) {
        obj->drawSlotBuf->owner = obj;
        obj->drawSlotBuf->draw = ItemAttack_Draw;
        ItemAttack_Run(obj);
    } else {
        ObjAttach_Free(obj);
    }
}

/*
 * A thrown item: its hit row, its flags (0x12 hits no player, 8 is deflected
 * by wind), and its velocity from the throw's speeds (ItemAttack_LobSpeeds
 * for the ITEMATK_LOBBED_* types, the other table when the thrower's motion is PL_MOT_THROW_ITEM_ALT).
 * SKILL_THROW_UP throws 1.5 times as far, SKILL_STATUS_PLUS adds an eighth to the
 * element power; stateVar0 keeps both as bits 2 and 4.
 */
void ItemAttack_SetupThrown(ObjAttach *obj)
{
    Player *owner = obj->owner;
    f32 (*speeds)[2];
    f32 *forward;
    f32 in[3];
    f32 out[3];
    f32 power;

    switch (obj->type) {
    case 18:
        Player_TestSetFlags(obj, ATKF_NO_PLAYER);
        break;
    case 8:
        Player_TestSetFlags(obj, ATKF_WIND_DEFLECTS);
        break;
    default:
        Player_TestSetFlags(obj, 0);
        break;
    }
    ObjAttach_StartOwnedAttack(obj, (s32) owner, ItemAttack_AttackRow[obj->type], &ItemAttack_RowsPtr);
    obj->hitList = ItemAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    if (ItemAttack_HitsInFlight[obj->type] == 0) {
        obj->hitPhase = 0;
    }
    switch (obj->type) {
    default:
        if (owner->motionId[0] == 417) {
            speeds = ItemAttack_ThrowSpeedsAlt;
        } else {
            speeds = ItemAttack_ThrowSpeeds;
        }
        obj->gravity = -0.8f;
        break;
    case ITEMATK_LOBBED_A:
    case ITEMATK_LOBBED_B:
    case ITEMATK_LOBBED_C:
    case ITEMATK_LOBBED_D:
    case ITEMATK_LOBBED_NO_VERSUS:
        speeds = ItemAttack_LobSpeeds;
        obj->gravity = -0.04f;
        break;
    }
    in[0] = 0.0f;
    in[1] = 0.0f;
    forward = &in[2];
    *forward = speeds[obj->stateVar1][0];
    obj->stateVar0 = 0;
    if (Player_HasSkill(owner, SKILL_THROW_UP) == 1) {
        *forward *= 1.5f;
        obj->stateVar0 |= 2;
    }
    if (Player_HasSkill(owner, SKILL_STATUS_PLUS) == 1) {
        obj->stateVar0 |= 4;
        if (obj->elementPower > 0) {
            power = 1.125f * obj->elementPower;
            if (power > 255.0f) {
                power = 255.0f;
            }
            obj->elementPower = power;
        }
    }
    Vec_Rotate((f32 *) out, (f32 *) in, (f32 *) owner->worldMtx);
    obj->velX = 2.0f * out[0];
    obj->velY = 2.0f * speeds[obj->stateVar1][1];
    obj->velZ = 2.0f * out[2];
    obj->homingTurn = 0;
    obj->homingSpeed = 0;
    obj->stateVar1 = 0;
}

/*
 * ITEMATK_NO_BOMB_AREA_ITEM's two parts: a draw slot each from the second pool, at 5 and 1060
 * above the item.
 */
void ItemAttack_InitParts(ObjAttach *obj)
{
    AttachPart *part = (AttachPart *) obj->buf;
    s16 i;

    for (i = 0; i < 2; i++, part++) {
        part->idx = i;
        part->frame = 0;
        part->time = 0;
        part->slot = DrawSlotB_Alloc();
        part->unk0C = 0;
        if (part->slot != -1) {
            part->node = DrawSlotB_Get(part->slot);
            part->node->owner = obj;
            part->node->part = i;
            part->node->draw = ItemAttack_DrawPart;
            Vec_Copy(part->node->pos, obj->pos);
            switch (part->idx) {
            case 0:
                part->node->pos[1] += 5.0f;
                break;
            case 1:
                part->node->pos[1] += 1060.0f;
                break;
            }
        } else {
            part->node = NULL;
        }
    }
}

/*
 * A placed item: no hit of its own; aux starts at 75 (the fuse's blink).
 * SKILL_COMBINE_SURE sets stateVar0 1 (passed to the burst). ITEMATK_NO_BOMB_AREA_ITEM makes its
 * parts, the others a ground effect.
 */
void ItemAttack_SetupPlaced(ObjAttach *obj)
{
    void *owner = obj->owner;

    Player_TestSetFlags(obj, 0);
    ObjAttach_StartOwnedAttack(obj, (s32) owner, ItemAttack_AttackRow[obj->type], &ItemAttack_RowsPtr);
    obj->hitList = 0;
    obj->hurtList = 0;
    obj->hitPhase = 0;
    if (Player_HasSkill(owner, SKILL_COMBINE_SURE) == 1) {
        obj->stateVar0 = 1;
    } else {
        obj->stateVar0 = 0;
    }
    obj->stateVar1 = 0;
    obj->aux = 75;
    switch (obj->type) {
    case ITEMATK_NO_BOMB_AREA_ITEM:
        ItemAttack_InitParts(obj);
        break;
    default:
        if (obj->area == gScene.area) {
            HitSplash_CreateAt(obj->pos, 13, 75, 10, 1.0f);
        }
        break;
    }
}

/*
 * A hopping item: its hit row, no hit until it jumps; SKILL_COMBINE_SURE sets
 * stateVar0 1 (passed to the burst).
 */
void ItemAttack_SetupHopping(ObjAttach *obj)
{
    s32 owner;

    owner = (s32) obj->owner;
    Player_TestSetFlags(obj, 0);
    ObjAttach_StartOwnedAttack(obj, owner, ItemAttack_AttackRow[obj->type], &ItemAttack_RowsPtr);
    obj->hitList = ItemAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->hitPhase = 0;
    if (Player_HasSkill((Player *) owner, SKILL_COMBINE_SURE) == 1) {
        obj->stateVar0 = 1;
    } else {
        obj->stateVar0 = 0;
    }
    obj->stateVar1 = 0;
    obj->aux = 0;
}

/*
 * Moves the item: flying back (stateVar1 255), bouncing off (254, ended
 * after 30 frames) or by its mode; then draws it at its position.
 */
void ItemAttack_Run(ObjAttach *obj)
{

    Vec_Copy(obj->pos2, obj->pos);
    if (obj->stateVar1 == 255) {
        ItemAttack_FlyBack(obj);
    } else if (obj->stateVar1 == 254) {
        if (ItemAttack_BounceOff(obj) & 0xFF) {
            obj->state = ATTACH_END;
            return;
        }
    } else {
        switch (ItemAttack_Modes[obj->type]) {
        default:
            ItemAttack_RunThrown(obj);
            break;
        case ITEM_PLACED:
            ItemAttack_RunPlaced(obj);
            break;
        case ITEM_HOPPING:
            ItemAttack_RunHopping(obj);
            break;
        }
    }
    if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 32U, 0);
    }
}

/*
 * A thrown item flies; ownerMotion counts its frames. Item 1 bursts after 13
 * frames (8 with SKILL_THROW_UP), item 9 after 16; the ITEMATK_LOBBED_* types fall
 * faster after 15 frames. On the floor it ends, leaving its mark (items 0, 5,
 * 7, 11, 17 and 23 make a stage object, and 11, 17 and 23 their burst).
 */
void ItemAttack_RunThrown(ObjAttach *obj)
{
    Player *owner = obj->owner;
    f32 floorY;
    s16 fuseFrames;

    ObjAttach_MoveAccel(obj);
    if (obj->type == 1) {
        obj->hitFrames = 0;
    } else {
        obj->hitFrames = 99;
    }
    floorY = AreaGrid2_GetFloorYFiltered(obj->pos);
    obj->ownerMotion++;
    switch (obj->type) {
    case 1:
        fuseFrames = (obj->stateVar0 & 2) ? 8 : 13;
        if (fuseFrames <= obj->ownerMotion) {
            if (obj->side != 0) {
                Explosion_Create(obj->owner, obj->pos, EXPLOSION_THROWN_PLAYERS_ONLY);
            }
            if ((u8)Obj_InCurrentArea(owner) != 0) {
                FireBlast_CreateOwned(obj->pos, 4, 1.0f, owner);
                Sound_PlaySeAtPosChecked(1, 71, 0, obj->pos, 15, 0);
            }
            obj->state = ATTACH_END;
            return;
        }
        break;
    case 9:
        if (obj->ownerMotion >= 16) {
            Explosion_Create(obj->owner, obj->pos, EXPLOSION_THROWN_BURST);
            if (obj->area == gScene.area) {
                ElementBurst_CreateAt(obj->pos, 4, 0, 0.5f);
            }
            obj->state = ATTACH_END;
            return;
        }
        break;
    case ITEMATK_LOBBED_A:
    case ITEMATK_LOBBED_B:
    case ITEMATK_LOBBED_C:
    case ITEMATK_LOBBED_D:
    case ITEMATK_LOBBED_NO_VERSUS:
        switch (obj->stateVar1) {
        case 0:
            if (obj->ownerMotion > 15) {
                obj->stateVar1++;
                obj->gravity -= 1.0f;
            }
            break;
        case 1:
            break;
        }
        break;
    }
    if (obj->pos[1] <= floorY) {
        if (obj->pos[1] > floorY - 100.0f) {
            obj->pos[1] = floorY;
        }
        switch (obj->type) {
        case 0:
            ItemProp_CreateFromAttach(obj, ITEMPROP_SMOKE, 0);
            break;
        case 5:
            ItemProp_CreateFromAttach(obj, ITEMPROP_SMOKE, 1);
            break;
        case 7:
            if (obj->area == gScene.area) {
                ItemProp_Create(obj->pos, 0, ITEMPROP_CLOUD, 2);
            }
            break;
        case 11:
            if (obj->area == gScene.area) {
                ItemProp_Create(obj->pos, 0, ITEMPROP_CLOUD, 3);
                Explosion_Create(obj->owner, obj->pos, EXPLOSION_THROWN_CLOUD_A);
            }
            break;
        case 17:
            if (obj->area == gScene.area) {
                ItemProp_Create(obj->pos, 0, ITEMPROP_CLOUD, 4);
                Explosion_Create(obj->owner, obj->pos, EXPLOSION_THROWN_CLOUD_B);
            }
            break;
        case 23:
            if (obj->area == gScene.area) {
                ItemProp_Create(obj->pos, 0, ITEMPROP_CLOUD, 5);
                Explosion_Create(obj->owner, obj->pos, EXPLOSION_THROWN_CLOUD_C);
            }
            break;
        }
        obj->hitPhase = 0;
        obj->visible = 0;
        obj->state = ATTACH_END;
    }
}

/*
 * Steps ITEMATK_NO_BOMB_AREA_ITEM's two parts: frames 1-4 in a loop, the low part restarting
 * after 16 frames, the high one holding at 65; draws them.
 */
void ItemAttack_AnimParts(ObjAttach *obj)
{
    AttachPart *part = (AttachPart *) obj->buf;
    s16 i;

    for (i = 0; i < 2; i++, part++) {
        part->frame++;
        part->time++;
        switch (part->idx) {
        case 0:
            if (part->frame > 4) {
                part->frame = 1;
            }
            if (part->time > 16) {
                part->frame = 1;
            }
            break;
        case 1:
            if (part->frame > 4) {
                part->frame = 1;
            }
            if (part->time > 65) {
                part->time = 65;
            }
            break;
        }
        if (part->frame > 0 && part->time > 0 && part->node != NULL) {
            OT_AddByDepth(gDrawOtSorted, part->node, 64, 0);
        }
    }
}

/*
 * A placed item's fuse: it bursts after 75 frames (ITEMATK_SMALL_BARREL_BOMB_B a larger burst,
 * ITEMATK_NO_BOMB_AREA_ITEM its own effects). Until then it ticks every 15 frames, smokes, and
 * from frame 55 blinks faster (aux, scaled by 0.8 each blink, is the gap).
 */
void ItemAttack_RunPlaced(ObjAttach *obj)
{
    s16 frames;

    if (++obj->ownerMotion >= 75) {
        if (obj->area == gScene.area) {
            switch (obj->type) {
            case ITEMATK_SMALL_BARREL_BOMB_B:
                obj->visible ^= 1;
                FireBlast_CreateScaled(obj->pos, 0, 1.5f);
                Explosion_CreateAt(obj->pos, EXPLOSION_SMALL_BARREL_BOMB_B, obj->area, obj->stateVar0);
                break;
            case ITEMATK_NO_BOMB_AREA_ITEM:
                PlayerFx_CreateAtDefault(obj->pos, 13, 1.5f);
                PlayerFx_CreateAtDefault(obj->pos, 14, 1.5f);
                Explosion_CreateAt(obj->pos, EXPLOSION_PLACED_BOMB, obj->area, 0);
                Sound_PlaySeAtPosChecked(1, 147, 0, obj->pos, 1, 0);
                break;
            default:
                obj->visible ^= 1;
                FireBlast_CreateScaled(obj->pos, 0, 1.0f);
                Explosion_CreateAt(obj->pos, EXPLOSION_BOMB, obj->area, obj->stateVar0);
                break;
            }
        }
        obj->state = ATTACH_END;
        return;
    }
    switch (obj->type) {
    default:
    case 4:
    case ITEMATK_SMALL_BARREL_BOMB_B:
        frames = obj->ownerMotion;
        if (frames >= 55) {
            obj->visible ^= 1;
            break;
        }
        if (obj->stateVar1 == 0) {
            if (0.8f * obj->aux > (f32) (55 - frames)) {
                obj->visible ^= 1;
                if (55 - obj->ownerMotion > 2) {
                    obj->stateVar1 = 2;
                }
            }
        } else if (--obj->stateVar1 == 0) {
            obj->visible ^= 1;
            obj->aux = 55 - obj->ownerMotion;
        }
        if (obj->area == gScene.area) {
            if (obj->ownerMotion == 1) {
                Sound_PlaySeAtPosChecked(1, 36, 0, obj->pos, 1, 0);
            }
            frames = obj->ownerMotion;
            if (frames != 0 && frames % 15 == 0) {
                Sound_PlaySeAtPosChecked(1, 37, 0, obj->pos, 1, 0);
            }
            if (obj->ownerMotion & 1) {
                HitSpark_SpawnRandomAngle(ANGLE_TURN * 3 / 4, 0, 2, obj->pos);
            }
        }
        break;
    case ITEMATK_NO_BOMB_AREA_ITEM:
        if (obj->ownerMotion == 62 && gScene.area == obj->area) {
            Sound_PlaySeAtPosChecked(1, 146, 0, obj->pos, 1, 0);
        }
        ItemAttack_AnimParts(obj);
        break;
    }
}

/*
 * A hopping item: after 26 frames it jumps (item 24 also forward along its
 * yaw), opens its hit 7 frames later, spins, and bursts 18 frames after the
 * jump.
 */
void ItemAttack_RunHopping(ObjAttach *obj)
{
    f32 effectPos[3];
    u16 yaw;
    u16 pitch;
    s16 frames;

    obj->ownerMotion++;
    ObjAttach_MoveAccel(obj);
    switch (obj->stateVar1) {
    case 0:
        if (obj->ownerMotion >= 26) {
            obj->stateVar1++;
            obj->ownerMotion = 0;
            obj->velY = 50.0f;
            obj->gravity = -0.2f;
            if (obj->type == 24) {
                obj->velZ = 30.0f;
                Vec_RotateY(&obj->velX, (6.2831855f * obj->rotY) / 65536.0f);
            }
            if (obj->area == gScene.area) {
                effectPos[0] = obj->pos[0];
                effectPos[1] = obj->pos[1] - 20.0f;
                effectPos[2] = obj->pos[2];
                PartFx_SpawnAt(effectPos, 7, 0.5f);
            }
        }
        break;
    case 1:
        if (obj->ownerMotion >= 7) {
            obj->stateVar1++;
            obj->hitPhase = 1;
        }
    case 2:
        obj->rotX += ANGLE_TURN / 8;
        obj->rotY += ANGLE_TURN / 8;
        if (obj->area == gScene.area) {
            frames = obj->ownerMotion;
            if (frames != 0 && frames % 15 == 0) {
                Sound_PlaySeAtPosChecked(1, 37, 0, obj->pos, 1, 0);
            }
            if (obj->ownerMotion & 1) {
                Vec_DirToAngles(&obj->velX, (s16 *) &yaw, (s16 *) &pitch);
                HitSpark_SpawnRandomAngle(yaw + ANGLE_TURN / 2, pitch, 2, obj->pos);
            }
        }
        if (obj->ownerMotion >= 18) {
            if (obj->area == gScene.area) {
                FireBlast_CreateScaled(obj->pos, 0, 1.0f);
                Explosion_CreateAt(obj->pos, EXPLOSION_BOMB, obj->area, obj->stateVar0);
            }
            obj->state = ATTACH_END;
        }
        break;
    }
    if (obj->ownerMotion == 1 && obj->area == gScene.area) {
        Sound_PlaySeAtPosChecked(1, 36, 0, obj->pos, 1, 0);
    }
}

/* A hit: items 20 and 24 burst where they are; every item then ends. */
void ItemAttack_OnHit(ObjAttach *obj)
{

    switch (obj->type) {
    case 20:
        if (obj->area == gScene.area) {
            FireBlast_CreateScaled(obj->pos, 0, 1.0f);
            Explosion_CreateAt(obj->pos, EXPLOSION_BOMB, obj->area, obj->stateVar0);
        }
        break;
    case 24:
        if (obj->area == gScene.area) {
            FireBlast_CreateScaled(obj->pos, 0, 1.0f);
            Explosion_CreateAt(obj->pos, EXPLOSION_BOMB, obj->area, obj->stateVar0);
        }
        break;
    }
    ItemAttack_End(obj);
}

/*
 * A deflected item (ATTACH_DEFLECTED, ATTACH_DEFLECTED_AURA). Either it
 * bounces off at a random angle (stateVar1 254), or, when ObjAttach_RollBatBack says
 * so, the monster in targetEm bats it back: it becomes that monster's attack
 * (row 12) flying at its thrower's chest (stateVar1 255).
 */
void ItemAttack_Deflect(ObjAttach *obj)
{
    Em *owner = obj->owner;
    Em *batter = obj->targetEm;
    f32 aim[3];
    f32 dir[3];
    u16 yaw;
    u16 pitch;

    obj->step = 0;
    obj->stateVar0 = 0;
    obj->ownerMotion = 0;
    obj->side = 0;
    if (ObjAttach_RollBatBack(obj, obj->randBits) == 0) {
        obj->stateVar1 = 254;
        obj->hitPhase = 0;
        Vec_Diff(dir, obj->pos2, obj->pos);
        Vec_DirToAngles(dir, (s16 *) &yaw, (s16 *) &pitch);
        yaw += -(Rand_Next(1) & 0x7FF) - 0x200;
        pitch += (Rand_Next(1) & 0x1FFF) - ANGLE_TURN / 16;
        obj->velX = 0.0f;
        obj->velY = Rand_Next(1) & 7;
        obj->velZ = 7.0f + (Rand_Next(1) & 3);
        obj->homingTurn = 0.0f;
        obj->gravity = -1.5f - (Rand_Next(1) & 0x1F) / 32.0f;
        obj->homingSpeed = 0.0f;
        Vec_RotateX(&obj->velX, (6.2831855f * yaw) / 65536.0f);
        Vec_RotateY(&obj->velX, (6.2831855f * pitch) / 65536.0f);
    } else {
        obj->stateVar1 = 255;
        obj->idx = batter->index;
        obj->isEm = batter->isEm;
        obj->owner = batter;
        Player_TestSetFlags(obj, 0);
        ObjAttach_StartOwnedAttack(obj, (s32) batter, 12, &ItemAttack_RowsPtr);
        obj->hitList = ItemAttack_HitShapes[obj->hitListNo];
        obj->hurtList = 0;
        Vec_Copy(aim, &owner->posX);
        aim[1] += 150.0f;
        Vec_Diff(dir, aim, obj->pos);
        Vec_DirToAngles(dir, (s16 *) &yaw, (s16 *) &pitch);
        obj->rotX = yaw;
        obj->rotY = pitch;
        obj->yaw = obj->rotY;
        obj->velX = 0.0f;
        obj->velY = 0.0f;
        obj->velZ = 70.0f;
        Vec_RotateX(&obj->velX, (6.2831855f * obj->rotX) / 65536.0f);
        Vec_RotateY(&obj->velX, (6.2831855f * obj->rotY) / 65536.0f);
        obj->homingTurn = 0.0f;
        obj->gravity = -0.4f;
        obj->homingSpeed = 0.0f;
        Vec_Copy(obj->pos2, obj->pos);
        if (gScene.area == obj->area) {
            Sound_PlaySeAtPosChecked(1, 150, 0, obj->pos, 6, 0);
        }
    }
    obj->state = ATTACH_RUN;
    ItemAttack_Run(obj);
}

/*
 * A batted-back item flies with its window open until it reaches the floor.
 */
void ItemAttack_FlyBack(ObjAttach *obj)
{
    obj->hitFrames = 99;
    if (++obj->ownerMotion < 0) {
        ObjAttach_Move(obj);
    } else {
        ObjAttach_MoveAccel(obj);
    }
    if (obj->pos[1] <= AreaGrid2_GetFloorYFiltered(obj->pos)) {
        obj->hitFrames = 0;
        obj->state = ATTACH_END;
    }
}

/*
 * A bouncing item tumbles for 30 frames without a hit; returns 1 when it is
 * over.
 */
s32 ItemAttack_BounceOff(ObjAttach *obj)
{
    s16 frames;

    obj->hitPhase = 0;
    frames = obj->ownerMotion + 1;
    obj->ownerMotion = frames;
    if (frames > 30) {
        return 1;
    }
    ObjAttach_MoveAccel(obj);
    obj->rotX = (s32) (obj->rotX - 0x400);
    obj->rotY = (s32) (obj->rotY - 0x400);
    return 0;
}

/* Frees ITEMATK_NO_BOMB_AREA_ITEM's two parts' draw slots. */
void ItemAttack_FreeDrawSlots(ObjAttach *obj)
{
    AttachPart *part = (AttachPart *) obj->buf;
    s16 i;

    for (i = 0; i < 2; i++) {
        if (part->node != NULL) {
            DrawSlotB_Free(part->slot);
        }
        part++;
    }
}

/*
 * Hides the item, frees its draw slot (and ITEMATK_NO_BOMB_AREA_ITEM's parts) and frees it on
 * the next update.
 */
void ItemAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        if (obj->keep == 1) {
            DrawSlotB_Free((s32) obj->drawSlot);
        } else {
            DrawSlot_Free((s32) obj->drawSlot);
        }
    }
    switch (obj->type) {
    case ITEMATK_NO_BOMB_AREA_ITEM:
        ItemAttack_FreeDrawSlots(obj);
        break;
    }
}

void ItemAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

/*
 * Draws one of ITEMATK_NO_BOMB_AREA_ITEM's parts while it is shown in the current area: part
 * 0 faces the camera, part 1 turns with the camera's yaw and steps through
 * the four quarters of its texture; both are scaled and coloured by their
 * keys.
 */
void ItemAttack_DrawPart(DrawSlot *slot)
{
    f32 mtx[16];
    f32 uvMtx[16];
    f32 scale[3];
    f32 rot[3];
    u32 col;
    ObjAttach *obj = slot->owner;
    AttachPart *part = (AttachPart *) obj->buf + slot->part;
    ModelMat *tex;
    Model *stage = gCommonModels[0];
    ModelMatSet *mdl;
    u8 facing;

    if (gScene.area == obj->area && obj->visible != 0 && part->frame > 0) {
        tex = stage->mats;
        switch (part->idx) {
        case 0:
            Key_LerpVec2(ItemAttack_ScaleKeysA, scale, part->frame);
            Color_EvalKeys((s32 *) ItemAttack_ColorKeysA, part->time, &col);
            facing = 1;
            break;
        case 1:
            Key_LerpVec2(ItemAttack_ScaleKeysB, scale, part->frame);
            Color_EvalKeys((s32 *) ItemAttack_ColorKeysB, part->time, &col);
            Mtx_Translation(uvMtx, 0.25f * (part->frame & 1), 0.0625f * ((part->frame >> 1) & 1), 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            facing = 2;
            break;
        default:
            return;
        }
        Mtx_FromScaleRotTrans(scale, rot, slot->pos, 0, mtx);
        mdl = &stage->matSets[ItemAttack_PartModels[part->idx]];
        switch (facing) {
        case 0:
            break;
        case 1:
            Vec_RotateSelf(mtx, gCamMatrix);
            break;
        case 2:
            Vec_RotateSelf(mtx, gCamYawMtx);
            break;
        }
        Gs_SetState(RS_ZWRITE, 0);
        Effect_DrawModel(mdl, mtx, col, 2, tex);
        Gs_SetState(RS_ZWRITE, 1);
    }
}

/*
 * Draws the item's model (ItemAttack_Models[type]) at its position: a thrown
 * item tumbles (the ITEMATK_LOBBED_* types keep their yaw), a placed one turns by its
 * yaw, a hopping one by its spin.
 */
void ItemAttack_Draw(DrawSlot *slot)
{
    f32 mtx[16];
    Model *stage = gCommonModels[0];
    ObjAttach *obj = slot->owner;
    ModelMat *mats;
    ModelMatSet *mdl;
    ModelMat *mat;
    s32 i;

    if (obj->area == gScene.area && stage != NULL && stage->active != 0) {
        mats = stage->mats;
        Mtx_Translation(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        switch (ItemAttack_Modes[obj->type]) {
        case ITEM_THROWN:
            switch (obj->type) {
            case ITEMATK_LOBBED_A:
            case ITEMATK_LOBBED_B:
            case ITEMATK_LOBBED_C:
            case ITEMATK_LOBBED_D:
            case ITEMATK_LOBBED_NO_VERSUS:
                Mtx_RotateXYZ(mtx, 0.0f, (6.2831855f * (u16) obj->yaw) / 65536.0f, 0.0f);
                break;
            default:
                Mtx_RotateXYZ(mtx, (6.2831855f * ((obj->ownerMotion << 11) & 0xFFFF)) / 65536.0f, 0.0f, 0.0f);
                break;
            }
            break;
        case ITEM_PLACED:
            Mtx_RotateY(mtx, (6.2831855f * obj->rotY) / 65536.0f);
            break;
        case ITEM_HOPPING:
            Mtx_RotateX(mtx, (6.2831855f * obj->rotX) / 65536.0f);
            Mtx_RotateY(mtx, (6.2831855f * obj->rotY) / 65536.0f);
            break;
        }
        mdl = stage->matSets + ItemAttack_Models[obj->type];
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        if (mdl != NULL && mdl->id != -1) {
            for (i = 0; i < mdl->count; i++) {
                mat = &mats[mdl->mat[i]];
                if (i == 1 && ItemAttack_Models[obj->type] == 55) {
                    mat->alpha = 0.0f;
                }
                mat->rgb[0] = 1.0f;
                mat->rgb[1] = 1.0f;
                mat->rgb[2] = 1.0f;
                Gs_SetState((u8) RS_MATERIAL(i), (s32) mat);
            }
            Draw_ApplyStates(mdl->flags);
            Model_Draw(mdl->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/*
 * Makes a FatalisAttack for em with the row attackId of FatalisAttack_Rows,
 * if a player is in em's area.
 */
void FatalisAttack_Create(Em *em, s8 attackId)
{
    ObjAttach *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = ObjAttach_Alloc(0)) != NULL) {
            obj->kind = ATTACH_FATALIS_ATTACK;
            obj->type = attackId;
            obj->update = FatalisAttack_Update;
            obj->idx = em->index;
            obj->isEm = em->isEm;
            obj->ownerMotion = em->motionId[0];
            obj->owner = em;
            obj->yaw = em->rotY;
            obj->rotX = em->rotX;
            obj->rotY = em->rotY;
            obj->rotZ = em->rotZ;
            obj->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
        }
    }
}

/*
 * FatalisAttack_Create at the stage object src's position, for src's owner
 * (without the area check).
 */
void FatalisAttack_CreateOnObj(StageObj *src, s8 attackId)
{
    ObjAttach *obj;
    Em *em;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        em = src->owner;
        obj->kind = ATTACH_FATALIS_ATTACK;
        obj->type = attackId;
        obj->update = FatalisAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->pos[0] = src->pos[0];
        obj->pos[1] = src->pos[1];
        obj->pos[2] = src->pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
    }
}

/* The FatalisAttack's per-frame update, by ObjAttach.state. */
void FatalisAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        FatalisAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        FatalisAttack_Run(obj);
        break;
    case ATTACH_END:
        FatalisAttack_End(obj);
        break;
    case ATTACH_HIT:
        FatalisAttack_Run(obj);
        break;
    case 4:
        FatalisAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        FatalisAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        FatalisAttack_End(obj);
        break;
    case 7:
        FatalisAttack_End(obj);
        break;
    case ATTACH_FREE:
        FatalisAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row; attack 3 counts hitDelay from the
 * monster's second motion layer.
 */
void FatalisAttack_Setup(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];
    s32 blendFrames;
    s32 motionFrame;

    obj->state++;
    obj->visible = 1;
    obj->draw = NULL;
    Player_TestSetFlags(obj, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(obj, (s32) em, obj->type, &FatalisAttack_RowsPtr);
    obj->hitList = FatalisAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->area = em->area;
    if (obj->hitDelay != 0) {
        if (obj->type == 3) {
            blendFrames = Math_Absf((s16) em->motionBlend[1] % 100);
            motionFrame = Math_Absf(em->motionFrame[1]);
        } else {
            blendFrames = Math_Absf((s16) em->motionBlend[0] % 100);
            motionFrame = Math_Absf(em->motionFrame[0]);
        }
        obj->hitDelay += blendFrames - motionFrame - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
    obj->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the window closes.
 * Attack 30 lasts while the monster's state is EM_GROUP_ATTACK's 12, 13 or 20:
 * its window opens for 60 frames once the motion is 1036.
 */
void FatalisAttack_Run(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    switch (obj->type) {
    case 2:
        obj->hitFrames = 99;
        break;
    }
    if (obj->type == 30) {
        switch (obj->stateVar0) {
        case 0:
            if (em->motionId[0] == 1036) {
                obj->stateVar0++;
                obj->hitFrames = 60;
            } else {
                obj->hitFrames = 99;
            }
            break;
        case 1:
            if (em->motionId[0] != 1036 || obj->hitPhase == 0) {
                obj->hitPhase = 0;
                obj->state = ATTACH_END;
            }
            break;
        }
        if (em->stateGroup != 3 || ((u32) (em->state - 12) > 1 && em->state != 20) || em->stateGroup == 4 || em->stateGroup == 5) {
            obj->hitPhase = 0;
            obj->state = ATTACH_END;
        }
    } else if (em->motionId[0] != obj->ownerMotion || obj->hitPhase == 0) {
        obj->hitPhase = 0;
        obj->state = ATTACH_END;
    }
    if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 32, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void FatalisAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) obj->drawSlot);
    }
}

void FatalisAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

/*
 * Makes a LaoShanAttack for em with the row attackId of LaoShanAttack_Rows,
 * if a player is in em's area.
 */
void LaoShanAttack_Create(Em *em, s8 attackId)
{
    ObjAttach *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = ObjAttach_Alloc(0)) != NULL) {
            obj->kind = ATTACH_LAOSHAN_ATTACK;
            obj->type = attackId;
            obj->update = LaoShanAttack_Update;
            obj->idx = em->index;
            obj->isEm = em->isEm;
            obj->ownerMotion = em->motionId[0];
            obj->owner = em;
            obj->yaw = em->rotY;
            obj->rotX = em->rotX;
            obj->rotY = em->rotY;
            obj->rotZ = em->rotZ;
            obj->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
            obj->step = 0;
            obj->hitSpan = 0;
        }
    }
}

/*
 * LaoShanAttack_Create with a hit window of span frames (halved for attack
 * 71, the only user).
 */
void LaoShanAttack_CreateSpan(Em *em, s32 attackId, s32 span)
{
    ObjAttach *obj;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((obj = ObjAttach_Alloc(0)) != NULL) {
            obj->kind = ATTACH_LAOSHAN_ATTACK;
            obj->type = attackId;
            obj->update = LaoShanAttack_Update;
            obj->idx = em->index;
            obj->isEm = em->isEm;
            obj->ownerMotion = em->motionId[0];
            obj->owner = em;
            obj->yaw = em->rotY;
            obj->rotX = em->rotX;
            obj->rotY = em->rotY;
            obj->rotZ = em->rotZ;
            obj->side = *(u8 *) &em->side;
            em->hitPending = 0;
            em->contactHit = 0;
            obj->step = 0;
            obj->hitSpan = span;
        }
    }
}

/* LaoShanAttack_Create at pos (without the area check). */
void LaoShanAttack_CreateAt(Em *em, f32 *pos, s8 attackId)
{
    ObjAttach *obj;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        obj->kind = ATTACH_LAOSHAN_ATTACK;
        obj->type = attackId;
        obj->update = LaoShanAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = 0;
        obj->rotY = 0;
        obj->rotZ = 0;
        obj->side = *(u8 *) &em->side;
        obj->pos[0] = pos[0];
        obj->pos[1] = pos[1];
        obj->pos[2] = pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
        obj->hitSpan = 0;
    }
}

/*
 * LaoShanAttack_Create at the stage object src's position, for src's owner
 * (without the area check).
 */
void LaoShanAttack_CreateOnObj(StageObj *src, s8 attackId)
{
    ObjAttach *obj;
    Em *em;

    if ((obj = ObjAttach_Alloc(0)) != NULL) {
        em = src->owner;
        obj->kind = ATTACH_LAOSHAN_ATTACK;
        obj->type = attackId;
        obj->update = LaoShanAttack_Update;
        obj->idx = em->index;
        obj->isEm = em->isEm;
        obj->ownerMotion = em->motionId[0];
        obj->owner = em;
        obj->yaw = em->rotY;
        obj->rotX = em->rotX;
        obj->rotY = em->rotY;
        obj->rotZ = em->rotZ;
        obj->pos[0] = src->pos[0];
        obj->pos[1] = src->pos[1];
        obj->pos[2] = src->pos[2];
        em->hitPending = 0;
        em->contactHit = 0;
        obj->step = 1;
        obj->hitSpan = 0;
    }
}

/* The LaoShanAttack's per-frame update, by ObjAttach.state. */
void LaoShanAttack_Update(ObjAttach *obj)
{
    switch (obj->state) {
    case ATTACH_SETUP:
        LaoShanAttack_Setup(obj);
        break;
    case ATTACH_RUN:
        LaoShanAttack_Run(obj);
        break;
    case ATTACH_END:
        LaoShanAttack_End(obj);
        break;
    case ATTACH_HIT:
        LaoShanAttack_End(obj);
        break;
    case 4:
        LaoShanAttack_End(obj);
        break;
    case ATTACH_DEFLECTED:
        LaoShanAttack_End(obj);
        break;
    case ATTACH_DEFLECTED_AURA:
        LaoShanAttack_End(obj);
        break;
    case 7:
        LaoShanAttack_End(obj);
        break;
    case ATTACH_FREE:
        LaoShanAttack_Free(obj);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from its row; hitDelay is counted from the monster's
 * motion.
 */
void LaoShanAttack_Setup(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    obj->state++;
    obj->visible = 1;
    obj->draw = LaoShanAttack_DrawNop;
    Player_TestSetFlags(obj, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(obj, (s32) em, obj->type, &LaoShanAttack_RowsPtr);
    obj->hitList = LaoShanAttack_HitShapes[obj->hitListNo];
    obj->hurtList = 0;
    obj->area = em->area;
    if (obj->type == 71) {
        obj->hitFrames = obj->hitSpan / 2;
    }
    if (obj->hitDelay != 0) {
        obj->hitDelay += (s32) Math_Absf((s16) em->motionBlend[0] % 100) - (s32) Math_Absf(em->motionFrame[0]) - 1;
        if ((s8) obj->hitDelay < 0) {
            obj->hitDelay = 0;
        }
    }
    obj->drawSlotBuf = NULL;
}

/*
 * Ends the attack when the monster's motion changes or the hit window closes.
 */
void LaoShanAttack_Run(ObjAttach *obj)
{
    Em *em = &gEms[obj->idx];

    if (em->motionId[0] != obj->ownerMotion || obj->hitPhase == 0) {
        obj->hitPhase = 0;
        obj->state = ATTACH_END;
    }
    if (obj->drawSlotBuf != NULL) {
        Vec_Copy(obj->drawSlotBuf->pos, obj->pos);
        OT_AddByDepth(gDrawOt32, obj->drawSlotBuf, 32U, 0);
    }
}

/* Hides the attack, frees its draw slot and frees it on the next update. */
void LaoShanAttack_End(ObjAttach *obj)
{
    obj->hitPhase = 0;
    obj->state = ATTACH_FREE;
    obj->visible = 0;
    if (obj->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) obj->drawSlot);
    }
}

void LaoShanAttack_Free(ObjAttach *obj)
{
    ObjAttach_Free(obj);
}

void LaoShanAttack_DrawNop(ObjAttach *obj)
{
}

/* This file's initialised data. */
HitShape PlayerAttack_HitShapes_0[4] = {
#include "game/00897040.inc"
};
HitShape PlayerAttack_HitShapes_1[3] = {
#include "game/008970E0.inc"
};
HitShape PlayerAttack_HitShapes_2[3] = {
#include "game/00897160.inc"
};
HitShape PlayerAttack_HitShapes_3[2] = {
#include "game/008971E0.inc"
};
HitShape PlayerAttack_HitShapes_4[2] = {
#include "game/00897230.inc"
};
HitShape PlayerAttack_HitShapes_5[2] = {
#include "game/00897280.inc"
};
HitShape PlayerAttack_HitShapes_6[2] = {
#include "game/008972D0.inc"
};
HitShape PlayerAttack_HitShapes_7[2] = {
#include "game/00897320.inc"
};
HitShape PlayerAttack_HitShapes_8[2] = {
#include "game/00897370.inc"
};
HitShape PlayerAttack_HitShapes_9[2] = {
#include "game/008973C0.inc"
};
HitShape PlayerAttack_HitShapes_10[2] = {
#include "game/00897410.inc"
};
HitShape PlayerAttack_HitShapes_11[3] = {
#include "game/00897460.inc"
};
HitShape PlayerAttack_HitShapes_12[2] = {
#include "game/008974E0.inc"
};
HitShape PlayerAttack_HitShapes_13[2] = {
#include "game/00897530.inc"
};
HitShape PlayerAttack_HitShapes_14[2] = {
#include "game/00897580.inc"
};
HitShape PlayerAttack_HitShapes_15[2] = {
#include "game/008975D0.inc"
};
HitShape PlayerAttack_HitShapes_16[2] = {
#include "game/00897620.inc"
};
HitShape PlayerAttack_HitShapes_17[2] = {
#include "game/00897670.inc"
};
HitShape PlayerAttack_HitShapes_18[2] = {
#include "game/008976C0.inc"
};
HitShape PlayerAttack_HitShapes_19[2] = {
#include "game/00897710.inc"
};
HitShape *PlayerAttack_HitShapes[20] = {
    PlayerAttack_HitShapes_0, PlayerAttack_HitShapes_1, PlayerAttack_HitShapes_2, PlayerAttack_HitShapes_3,
    PlayerAttack_HitShapes_4, PlayerAttack_HitShapes_5, PlayerAttack_HitShapes_6, PlayerAttack_HitShapes_7,
    PlayerAttack_HitShapes_8, PlayerAttack_HitShapes_9, PlayerAttack_HitShapes_10, PlayerAttack_HitShapes_11,
    PlayerAttack_HitShapes_12, PlayerAttack_HitShapes_13, PlayerAttack_HitShapes_14, PlayerAttack_HitShapes_15,
    PlayerAttack_HitShapes_16, PlayerAttack_HitShapes_17, PlayerAttack_HitShapes_18, PlayerAttack_HitShapes_19,
};
AttackRow PlayerAttack_Rows[101] = {
#include "game/008977B0.inc"
};
void *PlayerAttack_RowsPtr = (void *)PlayerAttack_Rows;
HitShape RathalosAttack_HitShapes_0[2] = {
#include "game/00898130.inc"
};
HitShape RathalosAttack_HitShapes_1[3] = {
#include "game/00898180.inc"
};
HitShape RathalosAttack_HitShapes_2[3] = {
#include "game/00898200.inc"
};
HitShape RathalosAttack_HitShapes_3[3] = {
#include "game/00898280.inc"
};
HitShape RathalosAttack_HitShapes_4[4] = {
#include "game/00898300.inc"
};
HitShape RathalosAttack_HitShapes_5[2] = {
#include "game/008983A0.inc"
};
HitShape RathalosAttack_HitShapes_6[2] = {
#include "game/008983F0.inc"
};
HitShape RathalosAttack_HitShapes_7[2] = {
#include "game/00898440.inc"
};
HitShape RathalosAttack_HitShapes_8[3] = {
#include "game/00898490.inc"
};
HitShape RathalosAttack_HitShapes_9[3] = {
#include "game/00898510.inc"
};
HitShape RathalosAttack_HitShapes_10[2] = {
#include "game/00898590.inc"
};
HitShape RathalosAttack_HitShapes_11[2] = {
#include "game/008985E0.inc"
};
HitShape RathalosAttack_HitShapes_12[2] = {
#include "game/00898630.inc"
};
HitShape RathalosAttack_HitShapes_13[2] = {
#include "game/00898680.inc"
};
HitShape RathalosAttack_HitShapes_14[4] = {
#include "game/008986D0.inc"
};
HitShape RathalosAttack_HitShapes_15[2] = {
#include "game/00898770.inc"
};
HitShape RathalosAttack_HitShapes_16[6] = {
#include "game/008987C0.inc"
};
HitShape *RathalosAttack_HitShapes[20] = {
    RathalosAttack_HitShapes_0, RathalosAttack_HitShapes_1, RathalosAttack_HitShapes_2, RathalosAttack_HitShapes_3,
    RathalosAttack_HitShapes_4, RathalosAttack_HitShapes_5, RathalosAttack_HitShapes_6, RathalosAttack_HitShapes_7,
    RathalosAttack_HitShapes_8, RathalosAttack_HitShapes_9, RathalosAttack_HitShapes_10, RathalosAttack_HitShapes_11,
    RathalosAttack_HitShapes_12, RathalosAttack_HitShapes_13, RathalosAttack_HitShapes_14, RathalosAttack_HitShapes_15,
    RathalosAttack_HitShapes_16, 0, 0, 0,
};
AttackRow RathalosAttack_Rows[49] = {
#include "game/00898900.inc"
};
void *RathalosAttack_RowsPtr = (void *)RathalosAttack_Rows;
HitShape EmContactAttack_HitShapes_0[2] = {
#include "game/00898DA0.inc"
};
HitShape EmContactAttack_HitShapes_1[2] = {
#include "game/00898DF0.inc"
};
HitShape EmContactAttack_HitShapes_2[2] = {
#include "game/00898E40.inc"
};
HitShape EmContactAttack_HitShapes_3[2] = {
#include "game/00898E90.inc"
};
HitShape EmContactAttack_HitShapes_4[2] = {
#include "game/00898EE0.inc"
};
HitShape EmContactAttack_HitShapes_5[2] = {
#include "game/00898F30.inc"
};
HitShape EmContactAttack_HitShapes_6[2] = {
#include "game/00898F80.inc"
};
HitShape EmContactAttack_HitShapes_7[2] = {
#include "game/00898FD0.inc"
};
HitShape *EmContactAttack_HitShapes[8] = {
    EmContactAttack_HitShapes_0, EmContactAttack_HitShapes_1, EmContactAttack_HitShapes_2, EmContactAttack_HitShapes_3,
    EmContactAttack_HitShapes_4, EmContactAttack_HitShapes_5, EmContactAttack_HitShapes_6, EmContactAttack_HitShapes_7,
};
AttackRow EmContactAttack_Rows[24] = {
#include "game/00899040.inc"
};
void *EmContactAttack_RowsPtr = (void *)EmContactAttack_Rows;
HitShape ItemAttack_HitShapes_0[2] = {
#include "game/00899290.inc"
};
HitShape ItemAttack_HitShapes_1[2] = {
#include "game/008992E0.inc"
};
HitShape ItemAttack_HitShapes_2[2] = {
#include "game/00899330.inc"
};
HitShape *ItemAttack_HitShapes[4] = {
    ItemAttack_HitShapes_0, ItemAttack_HitShapes_1, ItemAttack_HitShapes_2, 0,
};
AttackRow ItemAttack_Rows[14] = {
#include "game/00899390.inc"
};
void *ItemAttack_RowsPtr = (void *)ItemAttack_Rows;
s8 ItemAttack_AttackRow[32] = {
#include "game/008994F0.inc"
};
s8 ItemAttack_HitsInFlight[32] = {
#include "game/00899510.inc"
};
s8 ItemAttack_HasSide[32] = {
#include "game/00899530.inc"
};
s8 ItemAttack_Modes[32] = {
#include "game/00899550.inc"
};
s16 ItemAttack_Models[32] = {
#include "game/00899570.inc"
};
f32 ItemAttack_ScaleKeysA[12] = {
#include "game/008995B0.inc"
};
f32 ItemAttack_ScaleKeysB[8] = {
#include "game/008995E0.inc"
};
ColorKey ItemAttack_ColorKeysA[4] = {
#include "game/00899600.inc"
};
ColorKey ItemAttack_ColorKeysB[6] = {
#include "game/00899620.inc"
};
s16 ItemAttack_PartModels[8] = {
#include "game/00899650.inc"
};
f32 ItemAttack_ThrowSpeedsAlt[4][2] = {
#include "game/00899660.inc"
};
f32 ItemAttack_ThrowSpeeds[4][2] = {
#include "game/00899680.inc"
};
f32 ItemAttack_LobSpeeds[4][2] = {
#include "game/008996A0.inc"
};
HitShape FatalisAttack_HitShapes_0[2] = {
#include "game/008996C0.inc"
};
HitShape FatalisAttack_HitShapes_1[2] = {
#include "game/00899710.inc"
};
HitShape FatalisAttack_HitShapes_2[5] = {
#include "game/00899760.inc"
};
HitShape FatalisAttack_HitShapes_3[2] = {
#include "game/00899830.inc"
};
HitShape FatalisAttack_HitShapes_4[2] = {
#include "game/00899880.inc"
};
HitShape FatalisAttack_HitShapes_5[2] = {
#include "game/008998D0.inc"
};
HitShape FatalisAttack_HitShapes_6[2] = {
#include "game/00899920.inc"
};
HitShape FatalisAttack_HitShapes_7[2] = {
#include "game/00899970.inc"
};
HitShape FatalisAttack_HitShapes_8[2] = {
#include "game/008999C0.inc"
};
HitShape FatalisAttack_HitShapes_9[6] = {
#include "game/00899A10.inc"
};
HitShape FatalisAttack_HitShapes_10[2] = {
#include "game/00899B00.inc"
};
HitShape FatalisAttack_HitShapes_11[3] = {
#include "game/00899B50.inc"
};
HitShape FatalisAttack_HitShapes_12[2] = {
#include "game/00899BD0.inc"
};
HitShape FatalisAttack_HitShapes_13[8] = {
#include "game/00899C20.inc"
};
HitShape FatalisAttack_HitShapes_14[3] = {
#include "game/00899D60.inc"
};
HitShape FatalisAttack_HitShapes_15[3] = {
#include "game/00899DE0.inc"
};
HitShape FatalisAttack_HitShapes_16[2] = {
#include "game/00899E60.inc"
};
HitShape FatalisAttack_HitShapes_17[2] = {
#include "game/00899EB0.inc"
};
HitShape FatalisAttack_HitShapes_18[3] = {
#include "game/00899F00.inc"
};
HitShape *FatalisAttack_HitShapes[20] = {
    FatalisAttack_HitShapes_0, FatalisAttack_HitShapes_1, FatalisAttack_HitShapes_2, FatalisAttack_HitShapes_3,
    FatalisAttack_HitShapes_4, FatalisAttack_HitShapes_5, FatalisAttack_HitShapes_6, FatalisAttack_HitShapes_7,
    FatalisAttack_HitShapes_8, FatalisAttack_HitShapes_9, FatalisAttack_HitShapes_10, FatalisAttack_HitShapes_11,
    FatalisAttack_HitShapes_12, FatalisAttack_HitShapes_13, FatalisAttack_HitShapes_14, FatalisAttack_HitShapes_15,
    FatalisAttack_HitShapes_16, FatalisAttack_HitShapes_17, FatalisAttack_HitShapes_18, 0,
};
AttackRow FatalisAttack_Rows[36] = {
#include "game/00899FD0.inc"
};
void *FatalisAttack_RowsPtr = (void *)FatalisAttack_Rows;
HitShape LaoShanAttack_HitShapes_0[2] = {
#include "game/0089A340.inc"
};
HitShape LaoShanAttack_HitShapes_1[2] = {
#include "game/0089A390.inc"
};
HitShape LaoShanAttack_HitShapes_2[3] = {
#include "game/0089A3E0.inc"
};
HitShape LaoShanAttack_HitShapes_3[3] = {
#include "game/0089A460.inc"
};
HitShape LaoShanAttack_HitShapes_4[2] = {
#include "game/0089A4E0.inc"
};
HitShape LaoShanAttack_HitShapes_5[4] = {
#include "game/0089A530.inc"
};
HitShape LaoShanAttack_HitShapes_6[3] = {
#include "game/0089A5D0.inc"
};
HitShape LaoShanAttack_HitShapes_7[5] = {
#include "game/0089A650.inc"
};
HitShape LaoShanAttack_HitShapes_8[3] = {
#include "game/0089A720.inc"
};
HitShape LaoShanAttack_HitShapes_9[4] = {
#include "game/0089A7A0.inc"
};
HitShape LaoShanAttack_HitShapes_10[5] = {
#include "game/0089A840.inc"
};
HitShape LaoShanAttack_HitShapes_11[3] = {
#include "game/0089A910.inc"
};
HitShape LaoShanAttack_HitShapes_12[2] = {
#include "game/0089A990.inc"
};
HitShape LaoShanAttack_HitShapes_13[2] = {
#include "game/0089A9E0.inc"
};
HitShape LaoShanAttack_HitShapes_14[2] = {
#include "game/0089AA30.inc"
};
HitShape LaoShanAttack_HitShapes_15[2] = {
#include "game/0089AA80.inc"
};
HitShape LaoShanAttack_HitShapes_16[2] = {
#include "game/0089AAD0.inc"
};
HitShape LaoShanAttack_HitShapes_17[2] = {
#include "game/0089AB20.inc"
};
HitShape LaoShanAttack_HitShapes_18[2] = {
#include "game/0089AB70.inc"
};
HitShape LaoShanAttack_HitShapes_19[2] = {
#include "game/0089ABC0.inc"
};
HitShape *LaoShanAttack_HitShapes[20] = {
    LaoShanAttack_HitShapes_0, LaoShanAttack_HitShapes_1, LaoShanAttack_HitShapes_2, LaoShanAttack_HitShapes_3,
    LaoShanAttack_HitShapes_4, LaoShanAttack_HitShapes_5, LaoShanAttack_HitShapes_6, LaoShanAttack_HitShapes_7,
    LaoShanAttack_HitShapes_8, LaoShanAttack_HitShapes_9, LaoShanAttack_HitShapes_10, LaoShanAttack_HitShapes_11,
    LaoShanAttack_HitShapes_12, LaoShanAttack_HitShapes_13, LaoShanAttack_HitShapes_14, LaoShanAttack_HitShapes_15,
    LaoShanAttack_HitShapes_16, LaoShanAttack_HitShapes_17, LaoShanAttack_HitShapes_18, LaoShanAttack_HitShapes_19,
};
AttackRow LaoShanAttack_Rows[88] = {
#include "game/0089AC60.inc"
};
void *LaoShanAttack_RowsPtr = (void *)LaoShanAttack_Rows;
