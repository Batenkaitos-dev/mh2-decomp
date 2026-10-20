/*
 * EmRangedAttack, the monsters' ranged attacks: an attack-object family
 * (ObjAttach kind ATTACH_EM_RANGED_ATTACK). The monsters' attack states create
 * one at a bone (EmRangedAttack_CreateAtBone, EmRangedAttack_CreateAtBoneTimed),
 * or an attack creates one at a point (EmRangedAttack_CreateFrom,
 * EmRangedAttack_CreateFromAimed). Its type (enum EmRangedAttackType) is the
 * monster's move: fireballs, shots and spit that fly and burst on the floor,
 * streams and beams held at the owner's bone, Kirin's lightning strikes,
 * Khezu's ground lightning, thrown lumps. EmRangedAttack_Update, which
 * ObjAttach_UpdateAll calls every frame, switches on ObjAttach.state to its
 * Setup, Run, Hit, End and Free. The object's buffer is an EmRangedAttackBuf
 * followed by one EmRangedAttackPart per drawn part; the per-type tables give
 * the parts' counts, lives, key curves and models, the speeds and the attack
 * rows. EmRangedAttack_Impact leaves a burst's effects, sounds and Explosion;
 * EmRangedAttack_DrawParts draws the parts from up to two draw slots.
 * Capcom's build compiled this file at optimization level 4:
 * EmRangedAttack_Run and EmRangedAttack_DrawParts come out as the game's code
 * only at that level.
 */
#include "common.h"
#include "render_state.h"
#include "em.h"
#include "player.h"
#include "objattach.h"
#include "objhead.h"
#include "stageobj.h"
#include "scene.h"
#include "sprite.h"
#include "capsule.h"

#pragma optimization_level 4

void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
void CamShake_SetA(s32 kind, f32 *pos);
/* unit's own declaration: the angle declared u16 and the kind s32 as the callers pass them; with the definition's s32 angle and s16 kind the calls compile differently */
void AttackBurst_CreateAtScaled(f32 *pos, u16 r, s32 kind, f32 scale);
void AttackBurst_CreateAt(f32 *pos, s16 k, u8 c, f32 scale);
/* unit's own declaration: the kind declared s32 and the angle u16 as the callers pass them; with the definition's s16 kind and s32 angle the calls compile differently */
void AttackBurst_CreateAtYaw(f32 *pos, s32 kind, u8 type, u16 ang, f32 scale);
void ShotFlash_CreateAtScaled(f32 *pos, s32 k, u8 c, f32 scale);
void FireBlast_CreateScaledExact(f32 *pos, s16 k, f32 scale);
void MonsterFx_CreateAt(f32 *src, u8 k, f32 f);
StageObj *MonsterFx_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
extern f32 EmRangedAttack_RemobraSpitScales[];
void FireBlast_CreateScaled(f32 *pos, s16 k, f32 scale);
typedef struct { s32 frame; u32 color; } ColorKey;
void Mtx_RotateZ(f32 *m, f32 a);
void EmRangedAttack_KeepStrikePos(u8 *a, u8 *b);
void ThunderSpark_CreateAt(void *src, u8 k, u8 c, f32 f);
void Vec_Add(f32 *, f32 *, f32 *);
extern f32 (*EmRangedAttack_KirinStrikeSpots[])[2];
void Vec_RotateSelf(f32 *v, f32 *m);
extern f32 EmRangedAttack_BoneOfs[];
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateX(f32 *v, f32 angle);
void Vec_RotateY(f32 *v, f32 angle);
f32 Math_Atan2(f32 y, f32 x);
f32 Math_Sqrtf(f32 x);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
void Gfx_SetBlend(s32 a, s32 b);
extern f32 gCamMatrix[];
void ShotFlash_CreateAt(f32 *pos, s32 k, u8 c);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
f32 Math_Absf(f32 x);
s32 Model_Draw(u32 h, s32 flags);
void ObjAttach_Free(ObjAttach *p);
u16 Rand_Next(s32 stream);
void EmRangedAttack_Setup(ObjAttach *o);
void EmRangedAttack_Run(ObjAttach *o);
void EmRangedAttack_Hit(ObjAttach *);
void EmRangedAttack_End(ObjAttach *o);
void EmRangedAttack_Free(ObjAttach *o);
s32 Gs_SetState(u8 reg, u32 v);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
void Vec_Copy(f32 *out, f32 *in);
/* unit's own declaration: the flags declared s32; with the definition's u16 EmRangedAttack_DrawModel's call compiles differently */
void Draw_SetBlendFlags(s32);
f32 AreaGrid2_GetFloorYFiltered(f32 *);
void Draw_ApplyStates(u32);
void Draw_SetDefaultState(void);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void DrawSlot_Free(s32);
void ObjAttach_MoveAccel(ObjAttach *);
void EmRangedAttack_Update(ObjAttach *o);
void Vec_Normalize(f32 *);
f32 Vec_Dot(f32 *a, f32 *b);
void Vec_Cross(f32 *, f32 *, f32 *);
void Vec_Sub(f32 *, f32 *, f32 *);
extern f32 gCamPos;
ObjAttach *ObjAttach_Alloc(s32 n);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Em_BonePos(void *obj, s16 bone, f32 *out);
u8 Player_InCurrentArea(void *obj);
s32 Player_TestSetFlags(void *, s32 mask);
void ObjAttach_StartOwnedAttack(ObjAttach *, s32 a, s32 b, void *c);
extern s16 EmRangedAttack_RowBase[];
extern void *EmRangedAttack_RowsPtr;
void EmRangedAttack_StartAttackRow(ObjAttach *, s32);
void EmRangedAttack_Impact(ObjAttach *o, s16 n, f32 *pos);

/*
 * Fills a new EmRangedAttack object for its owner: the kind
 * ATTACH_EM_RANGED_ATTACK, the type, the variant (stateVar1),
 * EmRangedAttack_Update as its update, and the owner's monster flag, index,
 * side and heading (yaw).
 */
void EmRangedAttack_Init(ObjAttach *atk, ObjHead *owner, u8 type, u8 variant)
{
    atk->kind = ATTACH_EM_RANGED_ATTACK;
    atk->type = type;
    atk->stateVar1 = variant;
    atk->update = EmRangedAttack_Update;
    atk->owner = owner;
    atk->isEm = owner->isEm;
    atk->idx = owner->index;
    atk->side = owner->side;
    atk->yaw = owner->rot[1];
}

/*
 * Creates an EmRangedAttack of the given type and variant at bone of monster
 * em, when em is in the current area and the pool has room. pitch becomes
 * rotX and yawOfs is added to the monster's heading for rotY. The bone waits
 * in ownerMotion and the monster kind in aux until EmRangedAttack_Setup.
 */
void EmRangedAttack_CreateAtBone(Em *em, s32 bone, s32 atkType, s32 variant, s32 pitch, s32 yawOfs)
{
    ObjAttach *atk;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((atk = ObjAttach_Alloc(1)) != NULL) {
            EmRangedAttack_Init(atk, (ObjHead *) em, atkType, variant);
            Em_BonePos(em, bone, atk->pos);
            atk->ownerMotion = bone;
            atk->aux = em->type;
            atk->rotX = (u16) pitch;
            atk->rotY = (u16) yawOfs + em->rotY;
            atk->step = 0;
            atk->stateVar0 = 0;
        }
    }
}

/*
 * As EmRangedAttack_CreateAtBone, with frames kept in stateVar0: the life of
 * Plesioth's and Kushala Daora's streams, the hold of the Gravios and Rajang
 * beams (EmRangedAttackBuf.holdFrames).
 */
void EmRangedAttack_CreateAtBoneTimed(Em *em, s32 bone, s32 atkType, s32 variant, s32 pitch, s32 yawOfs, s32 frames)
{
    ObjAttach *atk;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((atk = ObjAttach_Alloc(1)) != NULL) {
            EmRangedAttack_Init(atk, (ObjHead *) em, atkType, variant);
            Em_BonePos(em, bone, atk->pos);
            atk->ownerMotion = bone;
            atk->aux = em->type;
            atk->rotX = (u16) pitch;
            atk->rotY = (u16) yawOfs + em->rotY;
            atk->step = 0;
            atk->stateVar0 = frames;
        }
    }
}

/*
 * Creates an EmRangedAttack of the given type and variant at src for the
 * owner of the attack parent (Plesioth's stream makes its floor wave this
 * way). It takes parent's yaw, heading, side and monster kind, flies level
 * (rotX 0), and skips the bone offset at set up (step 1). Only when parent is
 * in the current area.
 */
void EmRangedAttack_CreateFrom(void *owner, s32 type, s32 variant, ObjAttach *parent, f32 *src)
{
    ObjAttach *atk;

    if (parent->area == gScene.area) {
        if ((atk = ObjAttach_Alloc(1)) != NULL) {
            EmRangedAttack_Init(atk, owner, type & 0xFF, variant & 0xFF);
            atk->yaw = parent->yaw;
            Vec_Copy(atk->pos, src);
            atk->aux = parent->aux;
            atk->side = parent->side;
            atk->rotX = 0;
            atk->rotY = parent->rotY;
            atk->step = 1;
            atk->stateVar0 = 0;
        }
    }
}

/*
 * As EmRangedAttack_CreateFrom, with its own pitch and heading (rotY, also
 * its yaw). Blangonga's thrown lump makes its three fragments with it.
 */
void EmRangedAttack_CreateFromAimed(void *owner, s32 type, s32 variant, ObjAttach *parent, f32 *src, s32 pitch, u16 rotY)
{
    ObjAttach *atk;

    if (parent->area == gScene.area) {
        if ((atk = ObjAttach_Alloc(1)) != NULL) {
            EmRangedAttack_Init(atk, owner, type & 0xFF, variant & 0xFF);
            atk->yaw = rotY;
            Vec_Copy(atk->pos, src);
            atk->aux = parent->aux;
            atk->side = parent->side;
            atk->rotX = pitch & 0xFFFF;
            atk->rotY = rotY & 0xFFFF;
            atk->step = 1;
            atk->stateVar0 = 0;
        }
    }
}

/*
 * EmRangedAttack's per-frame update: switches on ObjAttach.state to
 * EmRangedAttack_Setup, _Run, _Hit, _End or _Free. The deflected states and
 * the unnamed states 4 and 7 end the object.
 */
void EmRangedAttack_Update(ObjAttach *atk)
{
    switch (atk->state) {
    case ATTACH_SETUP:
        EmRangedAttack_Setup(atk);
        break;
    case ATTACH_RUN:
        EmRangedAttack_Run(atk);
        break;
    case ATTACH_END:
        EmRangedAttack_End(atk);
        break;
    case ATTACH_HIT:
        EmRangedAttack_Hit(atk);
        break;
    case 4:
        EmRangedAttack_End(atk);
        break;
    case ATTACH_DEFLECTED:
        EmRangedAttack_End(atk);
        break;
    case ATTACH_DEFLECTED_AURA:
        EmRangedAttack_End(atk);
        break;
    case 7:
        EmRangedAttack_End(atk);
        break;
    case ATTACH_FREE:
        EmRangedAttack_Free(atk);
        
    default:
        break;
    }
}

/*
 * Starts the object's attack: ObjAttach_StartOwnedAttack with owner (the
 * monster, passed as an integer) and the row EmRangedAttack_RowBase[type]
 * plus the variant (0 for the Fatalis sweep and Remobra's spit) of the
 * family's attack rows, which gives the object its power, attack kind and hit
 * window.
 */
void EmRangedAttack_StartAttackRow(ObjAttach *atk, s32 owner)
{
    s16 row;
    u8 kind = atk->type;

    switch (kind) {
    case 12:
    case 24:
        row = 0;
        break;
    default:
        row = atk->stateVar1;
        break;
    }
    ObjAttach_StartOwnedAttack(atk, owner, row + EmRangedAttack_RowBase[kind], &EmRangedAttack_RowsPtr);
}

/* EmRangedAttack's object buffer (o->buf): an EmRangedAttackBuf, then one EmRangedAttackPart per
 * drawn part (EmRangedAttack_PartCounts[type]). */
typedef struct EmRangedAttackPart {
    s8 idx;                     /* 0x00 */
    u8 alpha;                   /* 0x01: types 4 and 21: the part's alpha (0xFF in phases 1-3) */
    s16 frame;                  /* 0x02: the part's frame; a negative one is a delay */
    f32 scale[3];               /* 0x04 */
    u16 rot[3];                 /* 0x10 */
    s16 spin[3];                /* 0x16 */
    u8 unk1C[3];                /* 0x1C */
    u8 shown;                   /* 0x1F: types 4 and 21: 0 hides the part */
} EmRangedAttackPart;

typedef struct EmRangedAttackBuf {
    u8 stage;                   /* 0x00: type 1: 0 until the stream reaches the floor, then counted; type 8: strikes left + 1 */
    s8 pitchOfs;                /* 0x01: streams and beams: the pitch against the bone, >> 8; type 8: the strike number */
    u8 phase;                   /* 0x02: types 1, 4, 18 and 21: the phase 0-3; type 8: the texture column */
    s8 boltModel;               /* 0x03: type 8: the bolt's model, 0-2 */
    s16 slot;                   /* 0x04: the second draw slot */
    u8 pad06[2];
    DrawSlot *ds;               /* 0x08 */
    f32 pos[3];                 /* 0x0C */
    f32 strikePos[3];            /* 0x18: type 8: where the strike lands */
    u8 pad24[0x30 - 0x24];
    s16 bone;                   /* 0x30 */
    u16 phaseTimer;             /* 0x32: the frames in the phase */
    f32 floorY;                 /* 0x34 */
    s16 ownerMotionId;          /* 0x38: the owner's motion at set up: a stream or beam ends when it changes */
    u8 holdFrames;               /* 0x3A: types 4 and 21: phase 2's length */
    s8 yawOfs;                  /* 0x3B: the heading against the owner's, >> 8 */
    EmRangedAttackPart part[1];           /* 0x3C */
} EmRangedAttackBuf;

extern f32 EmRangedAttack_Speeds[];
extern f32 EmRangedAttack_Gravities[];
extern s16 EmRangedAttack_PartCounts[];
extern f32 EmRangedAttack_FatalisSweepSpots[];
extern s16 EmRangedAttack_SpinX[];
extern s16 EmRangedAttack_SpinY[];
extern s16 EmRangedAttack_KirinStrikeCounts[];
extern HitShape *EmRangedAttack_HitShapes[];
EmBone *Em_Bone(void *obj, s16 bone);
f32 AreaGrid2_GetFloorY(f32 *pos);
void EmRangedAttack_DrawParts(DrawSlot *ds);
void EmRangedAttack_AttachToBone(ObjAttach *o, u8 *b);
void EmRangedAttack_KirinNextStrike(ObjAttach *o, u8 *b);

/*
 * Sets up an EmRangedAttack on its first update (ATTACH_SETUP), then runs its
 * first frame (EmRangedAttack_Run). It moves the creator's bone from
 * ownerMotion to the buffer, adds the type's bone offset
 * (EmRangedAttack_BoneOfs) unless an attack made it (step 1), aims the speed
 * EmRangedAttack_Speeds[type] along rotX and rotY, sets the gravity, takes
 * the main draw slot and clears the parts. Then by type: the second draw
 * slot, the parts' spins, scales and random angles, the streams' and beams'
 * offsets from the owner's bone, the doubled speed of the fast spit variants
 * and the start sounds; Kirin's first strike (EmRangedAttack_KirinNextStrike)
 * or, for the others, the attack row (EmRangedAttack_StartAttackRow); and the
 * hit shapes of hitListNo.
 */
void EmRangedAttack_Setup(ObjAttach *atk)
{
    f32 boneMtx[16];
    f32 ofs[3];
    EmRangedAttackBuf *head;
    EmRangedAttackPart *parts;
    Em *em;
    s32 n;
    s16 i;

    em = &gEms[atk->idx];
    head = (EmRangedAttackBuf *) atk->buf;
    atk->state = ATTACH_RUN;
    parts = head->part;
    atk->visible = 1;
    atk->draw = NULL;
    Player_TestSetFlags(atk, ATKF_PIERCES);
    atk->wasHit = 0;
    atk->area = em->area;
    head->stage = 0;
    head->pitchOfs = 0;
    head->phase = 0;
    head->boltModel = 0;
    head->bone = atk->ownerMotion;
    head->phaseTimer = 0;
    atk->ownerMotion = 0;
    atk->velX = 0.0f;
    atk->velY = 0.0f;
    atk->velZ = EmRangedAttack_Speeds[atk->type];
    if (atk->step != 1) {
        Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_Bone(em, head->bone)));
        ofs[0] = EmRangedAttack_BoneOfs[atk->type * 3];
        ofs[1] = EmRangedAttack_BoneOfs[atk->type * 3 + 1];
        ofs[2] = EmRangedAttack_BoneOfs[atk->type * 3 + 2];
        Vec_RotateSelf(ofs, boneMtx);
        Vec_Add(atk->pos, atk->pos, ofs);
        Vec_Copy(atk->pos2, atk->pos);
    }
    Vec_RotateX(&atk->velX, 6.2831855f * atk->rotX / 65536.0f);
    Vec_RotateY(&atk->velX, 6.2831855f * atk->rotY / 65536.0f);
    atk->homingTurn = 0;
    atk->gravity = EmRangedAttack_Gravities[atk->type];
    atk->homingSpeed = 0;
    atk->step = 0;
    atk->drawSlot = DrawSlot_Alloc();
    if (atk->drawSlot != -1) {
        atk->drawSlotBuf = DrawSlot_Get(atk->drawSlot);
        atk->drawSlotBuf->owner = atk;
        atk->drawSlotBuf->part = 0;
        atk->drawSlotBuf->draw = EmRangedAttack_DrawParts;
    } else {
        atk->drawSlotBuf = NULL;
    }
    head->ds = NULL;
    n = EmRangedAttack_PartCounts[atk->type];
    for (i = 0; i < n; i++) {
        parts->idx = i;
        parts->frame = 0;
        parts->rot[0] = 0;
        parts->rot[1] = 0;
        parts->rot[2] = 0;
        parts->spin[0] = 0;
        parts->spin[1] = 0;
        parts->spin[2] = 0;
        parts->unk1C[0] = 0;
        parts->unk1C[1] = 0;
        parts->unk1C[2] = 0;
        parts->shown = 0;
        parts->scale[2] = 1.0f;
        parts->scale[1] = 1.0f;
        parts->scale[0] = 1.0f;
        parts++;
    }
    parts = head->part;
    switch (atk->type) {
    case EMRANGED_FATALIS_SWEEP:
        Vec_Copy(ofs, &EmRangedAttack_FatalisSweepSpots[atk->stateVar1 * 3]);
        Vec_RotateY(ofs, 6.2831855f * atk->rotY / 65536.0f);
        atk->pos[0] += ofs[0];
        atk->pos[1] += ofs[1];
        atk->pos[2] += ofs[2];
        Vec_Copy(atk->pos2, atk->pos);
    case EMRANGED_FIREBALL:
    case EMRANGED_FATALIS_FIREBALL:
        head->slot = DrawSlot_Alloc();
        if (head->slot != -1) {
            head->ds = DrawSlot_Get(head->slot);
            head->ds->owner = atk;
            head->ds->part = 1;
            head->ds->draw = EmRangedAttack_DrawParts;
        } else {
            head->ds = NULL;
        }
        for (i = 0; i < n; i++, parts++) {
            if (i < 3) {
                if (i == 0) {
                    parts->rot[2] = Rand_Next(1);
                } else {
                    parts->spin[0] = EmRangedAttack_SpinX[i - 1] + (u16) (s32) (0.5f + 65536.0f * (0.1f * ((Rand_Next(1) & 0x1F) - 0x10)) / 360.0f);
                    parts->spin[1] = EmRangedAttack_SpinY[i - 1] + (u16) (s32) (0.5f + 65536.0f * (0.1f * ((Rand_Next(1) & 0x1F) - 0x10)) / 360.0f);
                }
            } else {
                parts->rot[0] = 0xC000;
            }
        }
        break;
    case EMRANGED_PLESIOTH_STREAM:
    case EMRANGED_KUSHALA_BREATH:
        head->pitchOfs = atk->rotX >> 8;
        head->yawOfs = (atk->rotY - em->rotY) >> 8;
        head->ownerMotionId = ((Em *) atk->owner)->motionId[0];
        EmRangedAttack_AttachToBone(atk, (u8 *) head);
        parts->spin[2] = 0x71C;
        break;
    case EMRANGED_HERMITAUR_SHOT:
        Em_PlaySeChecked(atk->owner, 0x1C, 0, atk->pos, 1, 0);
        goto grow;
    case EMRANGED_REMOBRA_SPIT:
        atk->hitScale = EmRangedAttack_RemobraSpitScales[atk->stateVar1];
        switch (atk->stateVar1) {
        default:
        case 0:
            Em_PlaySeChecked(atk->owner, 0x18, 0, atk->pos, 1, 0);
            break;
        case 1:
            Em_PlaySeChecked(atk->owner, 0x27, 0, atk->pos, 1, 0);
            break;
        }
    case EMRANGED_KUTKU_FIREBALL_SIDE:
    case EMRANGED_KUTKU_FIREBALL:
    case EMRANGED_PREY_SPIT:
    case EMRANGED_GRAVIOS_SHOT:
    case EMRANGED_PREY_SPIT_B:
    grow:
        if ((atk->type == EMRANGED_PREY_SPIT && atk->stateVar1 >= 2) || (atk->type == EMRANGED_PREY_SPIT_B && atk->stateVar1 > 0)) {
            atk->velX *= 2.0f;
            atk->velY *= 2.0f;
            atk->velZ *= 2.0f;
        }
        head->slot = DrawSlot_Alloc();
        if (head->slot != -1) {
            head->ds = DrawSlot_Get(head->slot);
            head->ds->owner = atk;
            head->ds->part = 1;
            head->ds->draw = EmRangedAttack_DrawParts;
        } else {
            head->ds = NULL;
        }
        for (i = 0; i < n; i++) {
            parts->idx = 0;
            parts->frame = -i * 4;
            parts->rot[2] = Rand_Next(1);
            parts->spin[2] = 0x199A;
            parts++;
        }
        break;
    case EMRANGED_GRAVIOS_BEAM:
    case EMRANGED_RAJANG_BEAM:
        head->pitchOfs = atk->rotX >> 8;
        head->yawOfs = (atk->rotY - em->rotY) >> 8;
        head->ownerMotionId = ((Em *) atk->owner)->motionId[0];
        EmRangedAttack_AttachToBone(atk, (u8 *) head);
        head->holdFrames = atk->stateVar0;
        atk->stateVar0 = 0;
        for (i = 0; i < n; i++) {
            switch (parts->idx) {
            case 0:
                parts->spin[2] = 0x1000;
            }
            parts++;
        }
        break;
    case EMRANGED_KHEZU_GROUND_BOLT:
        atk->pos[1] = em->floorY;
        Em_PlaySeChecked(atk->owner, 0x37, 0, atk->pos, 1, 0);
        head->slot = DrawSlot_Alloc();
        if (head->slot != -1) {
            head->ds = DrawSlot_Get(head->slot);
            head->ds->owner = atk;
            head->ds->part = 1;
            head->ds->draw = EmRangedAttack_DrawParts;
        } else {
            head->ds = NULL;
        }
        for (i = 0; i < n; i++) {
            parts->idx = i;
            if (i > 0) {
                if (i == 5) {
                    parts->idx = 4;
                    parts->frame = -5;
                }
                parts->rot[2] = Rand_Next(1);
            }
            parts++;
        }
        break;
    case EMRANGED_KIRIN_BOLTS:
        head->stage = EmRangedAttack_KirinStrikeCounts[atk->stateVar1 % 6] + 1;
        Vec_Copy(head->pos, &((Em *) atk->owner)->posX);
        parts->frame = -7;
        break;
    case EMRANGED_PLESIOTH_FLOOR_WAVE:
        atk->pos[1] = AreaGrid2_GetFloorY(atk->pos);
        break;
    case EMRANGED_FLOOR_TRAIL:
        for (i = 0; i < n; i++) {
            parts->scale[2] = 1.0f;
            parts->scale[1] = 1.0f;
            parts->scale[0] = 1.0f;
            parts++;
        }
        break;
    case EMRANGED_BLANGONGA_LUMP:
        head->slot = DrawSlot_Alloc();
        if (head->slot != -1) {
            head->ds = DrawSlot_Get(head->slot);
            head->ds->owner = atk;
            head->ds->part = 1;
            head->ds->draw = EmRangedAttack_DrawParts;
        } else {
            head->ds = NULL;
        }
        parts->scale[2] = 3.0f;
        parts->scale[1] = 3.0f;
        parts->scale[0] = 3.0f;
        parts->rot[0] = Rand_Next(1);
        parts->rot[1] = Rand_Next(1);
        parts->rot[2] = Rand_Next(1);
        head->floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
        break;
    case EMRANGED_THROW_FRAGMENT:
        parts->scale[2] = 2.0f;
        parts->scale[1] = 2.0f;
        parts->scale[0] = 2.0f;
        parts->rot[0] = Rand_Next(1);
        parts->rot[1] = Rand_Next(1);
        parts->rot[2] = Rand_Next(1);
        parts->spin[0] = (Rand_Next(1) & 0xFF) - 0x80;
        parts->spin[1] = (Rand_Next(1) & 0xFF) - 0x80;
        parts->spin[2] = (Rand_Next(1) & 0xFF) - 0x80;
        break;
    case EMRANGED_CONGALALA_THROW:
        parts->scale[2] = 1.5f;
        parts->scale[1] = 1.5f;
        parts->scale[0] = 1.5f;
        parts->rot[0] = Rand_Next(1);
        parts->rot[1] = Rand_Next(1);
        parts->rot[2] = Rand_Next(1);
        parts->spin[0] = (Rand_Next(1) & 0xFF) - 0x80;
        parts->spin[1] = (Rand_Next(1) & 0xFF) - 0x80;
        parts->spin[2] = (Rand_Next(1) & 0xFF) - 0x80;
        break;
    case EMRANGED_BIG_FRAGMENT:
        parts->scale[2] = 5.0f;
        parts->scale[1] = 5.0f;
        parts->scale[0] = 5.0f;
        parts->rot[0] = Rand_Next(1);
        parts->rot[1] = Rand_Next(1);
        parts->rot[2] = Rand_Next(1);
        parts->spin[0] = (Rand_Next(1) & 0xFF) - 0x80;
        parts->spin[1] = (Rand_Next(1) & 0xFF) - 0x80;
        parts->spin[2] = (Rand_Next(1) & 0xFF) - 0x80;
        break;
    }
    if (atk->type == EMRANGED_KIRIN_BOLTS) {
        EmRangedAttack_KirinNextStrike(atk, (u8 *) head);
    } else {
        EmRangedAttack_StartAttackRow(atk, (s32) em);
    }
    atk->hitList = EmRangedAttack_HitShapes[atk->hitListNo];
    atk->hurtList = 0;
    EmRangedAttack_Run(atk);
}

extern s16 EmRangedAttack_PartFrames[];
extern s16 EmRangedAttack_Lives[];
extern s16 EmRangedAttack_GroundBoltPartFrames[];
void EmRangedAttack_FloorCrossing(ObjAttach *o, ObjAttach *ref, f32 *out);
void EmRangedAttack_RestartBoltPart(ObjAttach *o, EmRangedAttackPart *p);
extern s16 EmRangedAttack_FirstCurve[];
extern s16 EmRangedAttack_CurveSteps[];
extern u32 EmRangedAttack_Curves[];
extern f32 gCamDir[3];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void CamShake_SetAEverywhere(s32);
void AttackBurst_CreateAtBone(Em *em, s16 bone, s16 k, u16 c);
void AttackBurst_CreateTrail(f32 *pos, u16 v, u16 w, u8 a, u8 b);
void MonsterFx_CreateAtAngled(f32 *pos, u16 w, u16 v, u8 k, f32 scale);
f32 AreaGrid2_GetFloorY(f32 *pos);
/*
 * Runs an EmRangedAttack every frame (ATTACH_RUN). It counts frames in
 * ownerMotion and ends the object past its life (EmRangedAttack_Lives[type],
 * the creator's frames for the streams, holdFrames + 31 for the beams). By
 * type it moves the shots (ObjAttach_MoveAccel) with their trail effects and
 * sounds, keeps the streams and beams at the owner's bone through their
 * phases (ended when the owner leaves the motion), and ends Kirin's strikes
 * when the monster reacts or is defeated. It steps each part's frame and key
 * animation (EmRangedAttack_Curves through Key_LerpVec3, spins, scales), then
 * tests the floor: a shot that reaches it calls EmRangedAttack_Impact and
 * ends, a stream's or beam's tip is placed along its aim, and the fireballs
 * shake the camera near it. Last it queues its draw slots. The test before
 * hitFrames is set is always true, so hitFrames is 99 every frame.
 */
void EmRangedAttack_Run(ObjAttach *atk)
{
    f32 vec[3];
    f32 pos[3];
    f32 lowPoint[3];
    f32 delta[3];
    Em *em;
    u8 spark;
    EmRangedAttackBuf *head;
    EmRangedAttackPart *parts;
    EmRangedAttackPart *top;
    EmRangedAttackPart *beamPart;
    Em *owner;
    f32 floorY;
    f32 dist;
    f32 factor;
    s16 n;
    s16 life;
    s16 lim;
    s16 step;
    s16 key;
    s16 i;
    s16 frameLimit;
    u8 type;
    s32 beamPhase;
    u32 curve;

    head = (EmRangedAttackBuf *) atk->buf;
    em = &gEms[atk->idx];
    type = atk->type;
    parts = head->part;
    spark = 0;
    n = EmRangedAttack_PartCounts[type];
    life = EmRangedAttack_Lives[type];
    key = EmRangedAttack_FirstCurve[type];
    step = EmRangedAttack_CurveSteps[type];
    lim = EmRangedAttack_PartFrames[type];
    top = parts;
    if (!((type == 8 || (type == 15 && atk->hitPhase == 1)) && type == 16 && atk->hitPhase == 1)) {
        atk->hitFrames = 0x63;
    }
    atk->yaw = atk->rotY;
    switch (atk->type) {
    case EMRANGED_FIREBALL:
    case EMRANGED_KUTKU_FIREBALL_SIDE:
    case EMRANGED_KUTKU_FIREBALL:
    case EMRANGED_KHEZU_CEILING_SHOT:
    case EMRANGED_KHEZU_GROUND_BOLT:
    case EMRANGED_CEPHADROME_SHOT:
    case EMRANGED_PREY_SPIT:
    case EMRANGED_PLESIOTH_FLOOR_WAVE:
    case EMRANGED_FATALIS_FIREBALL:
    case EMRANGED_FATALIS_SWEEP:
    case EMRANGED_GRAVIOS_SHOT:
    case EMRANGED_FLOOR_TRAIL:
    case EMRANGED_BLANGONGA_LUMP:
    case EMRANGED_THROW_FRAGMENT:
    case EMRANGED_BLANGO_THROW:
    case EMRANGED_CONGALALA_THROW:
    case EMRANGED_BIG_FRAGMENT:
    case EMRANGED_HERMITAUR_SHOT:
    case EMRANGED_PREY_SPIT_B:
    case EMRANGED_REMOBRA_SPIT:
        atk->pos2[0] = atk->pos[0];
        atk->pos2[1] = atk->pos[1];
        atk->pos2[2] = atk->pos[2];
        break;
    case EMRANGED_PLESIOTH_STREAM:
    case EMRANGED_KUSHALA_BREATH:
        life = atk->stateVar0;
        break;
    case EMRANGED_GRAVIOS_BEAM:
        life = head->holdFrames + 31;
        break;
    case EMRANGED_KIRIN_BOLTS:
        break;
    case EMRANGED_RAJANG_BEAM:
        life = head->holdFrames + 31;
        break;
    }
    if (life < ++atk->ownerMotion) {
        atk->hitFrames = 0;
        atk->state = ATTACH_END;
        return;
    }
    switch (atk->type) {
    case EMRANGED_FIREBALL:
    case EMRANGED_FATALIS_FIREBALL:
    case EMRANGED_FATALIS_SWEEP:
        switch (atk->type) {
        case 0:
            frameLimit = 30;
            break;
        case 11:
            frameLimit = 30;
            break;
        default:
            frameLimit = life;
            break;
        }
        if (frameLimit < atk->ownerMotion) {
            factor = 1.0f / (atk->ownerMotion - frameLimit);
            atk->velX *= factor;
            atk->velY *= factor;
            atk->velZ *= factor;
        }
        ObjAttach_MoveAccel(atk);
        if (atk->area == gScene.area) {
            if (atk->ownerMotion < life - 5) {
                if (atk->ownerMotion == 1 || atk->ownerMotion % 3 != 1) {
                    AttackBurst_CreateTrail(atk->pos, atk->rotX, atk->rotY, 0, ((Em *) atk->owner)->type);
                } else {
                    AttackBurst_CreateTrail(atk->pos, atk->rotX, atk->rotY, 2, ((Em *) atk->owner)->type);
                }
            }
            if (atk->type == EMRANGED_FATALIS_SWEEP) {
                if (atk->pos2[1] >= 3000.0f && atk->pos[1] < 3000.0f) {
                    Em_PlaySeChecked((Em *) atk->owner, 0x42, 0, atk->pos, 7, 0);
                }
                if (atk->ownerMotion % 5 == 0) {
                    Em_PlaySeChecked((Em *) atk->owner, 0x3B, 0, atk->pos, 0x18, 0);
                }
            } else if (atk->ownerMotion % 5 == 0) {
                Em_PlaySeChecked((Em *) atk->owner, 0x3A, 0, atk->pos, 0x18, 0);
            }
        }
        break;
    case EMRANGED_PLESIOTH_STREAM:
        owner = (Em *) atk->owner;
        if (!owner->active || owner->taskPhase == 3 || head->ownerMotionId != owner->motionId[0]) {
            atk->hitPhase = 0;
            atk->hitFrames = 0;
            atk->state = ATTACH_END;
            return;
        }
        EmRangedAttack_AttachToBone(atk, (u8 *) head);
        switch (head->phase) {
        case 0:
            lim = 10;
            if (++head->phaseTimer > lim) {
                head->phase++;
                lim = life - 30;
                head->phaseTimer = 0;
                for (i = 0; i < n; i++) {
                    parts->frame = 0;
                    parts++;
                }
                parts = top;
            }
            break;
        case 1:
            lim = life - 30;
            if (++head->phaseTimer > lim) {
                lim = 20;
                head->phase++;
                head->phaseTimer = 0;
                for (i = 0; i < n; i++) {
                    parts->frame = 0;
                    parts++;
                }
                parts = top;
            }
            break;
        default:
        case 2:
            lim = 20;
            if (++head->phaseTimer > lim) {
                atk->hitPhase = 0;
                atk->hitFrames = 0;
                atk->state = ATTACH_END;
                return;
            }
            break;
        }
        atk->yaw = atk->rotY;
        break;
    case EMRANGED_KUTKU_FIREBALL_SIDE:
    case EMRANGED_KUTKU_FIREBALL:
    case EMRANGED_GRAVIOS_SHOT:
        ObjAttach_MoveAccel(atk);
        if (atk->area == gScene.area && atk->ownerMotion >= 3 && (atk->ownerMotion & 1)) {
            if (atk->type == EMRANGED_KUTKU_FIREBALL_SIDE || atk->type == EMRANGED_KUTKU_FIREBALL) {
                AttackBurst_CreateAtYaw(atk->pos, 0x12, ((Em *) atk->owner)->type, atk->rotY, 1.0f);
            } else if (atk->type == EMRANGED_GRAVIOS_SHOT) {
                AttackBurst_CreateAtYaw(atk->pos, 0x12, ((Em *) atk->owner)->type, atk->rotY, 2.0f);
            }
        }
        break;
    case EMRANGED_GRAVIOS_BEAM:
        if (atk->stateVar0 != 0xFF) {
            owner = (Em *) atk->owner;
            if (!owner->active || owner->taskPhase == 3
                || (owner->motionId[0] != head->ownerMotionId && owner->motionId[0] != 0x45A)) {
                atk->stateVar0 = 0xFF;
            }
        }
        if (atk->stateVar0 != 0xFF) {
            EmRangedAttack_AttachToBone(atk, (u8 *) head);
        }
        switch (head->phase) {
        case 0:
            lim = 11;
            if (++head->phaseTimer > lim) {
                head->phase++;
                if (atk->stateVar0 != 0xFF && atk->area == gScene.area) {
                    AttackBurst_CreateAtBone(em, head->bone, 7, 0);
                }
                head->phaseTimer = 0;
                lim = 7;
                for (i = 0; i < n; i++) {
                    beamPart = &parts[i];
                    beamPart->frame = 0;
                    switch (beamPart->idx) {
                    case 0:
                        beamPart->rot[2] = 0;
                        beamPart->spin[2] = 0;
                        break;
                    case 1:
                        beamPart->rot[2] = 0;
                        beamPart->spin[2] = 0x1000;
                        break;
                    case 2:
                        beamPart->rot[2] = 0;
                        beamPart->spin[2] = 0x1000;
                        break;
                    }
                }
            }
            break;
        case 1:
            lim = 7;
            if (++head->phaseTimer > lim) {
                head->phase++;
                head->phaseTimer = 0;
                lim = head->holdFrames;
                for (i = 0; i < n; i++) {
                    parts[i].frame = 0;
                }
            }
            break;
        case 2:
            lim = head->holdFrames;
            if (++head->phaseTimer > lim) {
                lim = 13;
                head->phase++;
                head->phaseTimer = 0;
                for (i = 0; i < n; i++) {
                    parts[i].frame = 0;
                }
                atk->stateVar0 = 0xFF;
                atk->hitPhase = 0;
                if (atk->area == gScene.area) {
                    AttackBurst_CreateAtYaw(atk->pos2, 6, ((Em *) atk->owner)->type, atk->rotY, 1.0f);
                }
            }
            break;
        case 3:
            lim = 13;
            if (++head->phaseTimer > lim) {
                atk->hitFrames = 0;
                atk->state = ATTACH_END;
                return;
            }
            break;
        default:
            atk->hitFrames = 0;
            atk->hitPhase = 0;
            atk->state = ATTACH_END;
            return;
        }
        switch (head->phase) {
        case 0:
            step = 2;
            key = 5;
            break;
        case 1:
            step = 1;
            key = 6;
            break;
        case 2:
            step = 0;
            key = -1;
            break;
        case 3:
            step = 1;
            key = 8;
            break;
        }
        if (head->phase == 1 || head->phase == 2) {
            if (atk->area == gScene.area && !(atk->ownerMotion & 3)) {
                AttackBurst_CreateAtYaw(atk->pos2, 0, ((Em *) atk->owner)->type, atk->rotY, 1.0f);
            }
        }
        atk->yaw = atk->rotY;
        break;
    case EMRANGED_KHEZU_CEILING_SHOT:
    case EMRANGED_CEPHADROME_SHOT:
    case EMRANGED_PREY_SPIT:
    case EMRANGED_PLESIOTH_FLOOR_WAVE:
    case EMRANGED_FLOOR_TRAIL:
    case EMRANGED_THROW_FRAGMENT:
    case EMRANGED_BLANGO_THROW:
    case EMRANGED_BIG_FRAGMENT:
    case EMRANGED_HERMITAUR_SHOT:
    case EMRANGED_PREY_SPIT_B:
    case EMRANGED_REMOBRA_SPIT:
        ObjAttach_MoveAccel(atk);
        break;
    case EMRANGED_BLANGONGA_LUMP:
        ObjAttach_MoveAccel(atk);
        if (atk->velY <= -300.0f) {
            atk->velY = -300.0f;
        }
        break;
    case EMRANGED_KHEZU_GROUND_BOLT:
        if (atk->ownerMotion >= life - 2) {
            if (atk->ownerMotion == life - 2) {
                atk->hitPhase = 0;
                spark = 1;
            } else {
                return;
            }
        }
        ObjAttach_MoveAccel(atk);
        break;
    case EMRANGED_KIRIN_BOLTS:
        owner = (Em *) atk->owner;
        if (!owner->active || owner->taskPhase == 3) {
            atk->hitPhase = 0;
            atk->hitFrames = 0;
            atk->state = ATTACH_END;
            return;
        }
        if (owner->stateGroup == EM_GROUP_REACT || owner->stateGroup == EM_GROUP_DEFEAT) {
            atk->hitPhase = 0;
            atk->hitFrames = 0;
            atk->state = ATTACH_END;
            return;
        }
        atk->pos2[0] = atk->pos[0];
        atk->pos2[1] = atk->pos[1];
        atk->pos2[2] = atk->pos[2];
        break;
    case EMRANGED_KUSHALA_BREATH:
        owner = (Em *) atk->owner;
        if (!owner->active || owner->taskPhase == 3 || head->ownerMotionId != owner->motionId[0]) {
            atk->hitPhase = 0;
            atk->hitFrames = 0;
            atk->state = ATTACH_END;
            return;
        }
        EmRangedAttack_AttachToBone(atk, (u8 *) head);
        switch (head->phase) {
        case 0:
            lim = 10;
            if (++head->phaseTimer > lim) {
                lim = 2;
                head->phase++;
                head->phaseTimer = 0;
                for (i = 0; i < n; i++) {
                    parts->frame = 0;
                    parts++;
                }
                parts = top;
            }
            break;
        case 1:
            lim = 2;
            if (++head->phaseTimer > life - 30) {
                lim = 20;
                head->phase++;
                head->phaseTimer = 0;
                for (i = 0; i < n; i++) {
                    parts->frame = 0;
                    parts++;
                }
                parts = top;
            }
            break;
        default:
        case 2:
            lim = 20;
            if (++head->phaseTimer > lim) {
                atk->hitPhase = 0;
                atk->hitFrames = 0;
                atk->state = ATTACH_END;
                return;
            }
            break;
        }
        atk->yaw = atk->rotY;
        break;
    case EMRANGED_CONGALALA_THROW:
        ObjAttach_MoveAccel(atk);
        if (atk->area == gScene.area && !(atk->ownerMotion & 1)) {
            MonsterFx_CreateAt(atk->pos, 0x16, 2.0f);
        }
        break;
    case EMRANGED_RAJANG_BEAM:
        if (atk->stateVar0 != 0xFF) {
            owner = (Em *) atk->owner;
            if (!owner->active || owner->taskPhase == 3
                || (owner->motionId[0] != head->ownerMotionId && owner->motionId[0] != 0x439)) {
                atk->stateVar0 = 0xFF;
            }
        }
        if (atk->stateVar0 != 0xFF) {
            EmRangedAttack_AttachToBone(atk, (u8 *) head);
        }
        switch (head->phase) {
        case 0:
            lim = 11;
            if (++head->phaseTimer > lim) {
                head->phase++;
                head->phaseTimer = 0;
                lim = 7;
                for (i = 0; i < n; i++) {
                    beamPart = &parts[i];
                    beamPart->frame = 0;
                    switch (beamPart->idx) {
                    case 0:
                        beamPart->rot[2] = 0;
                        beamPart->spin[2] = 0;
                        break;
                    case 1:
                        beamPart->rot[2] = 0;
                        beamPart->spin[2] = 0x1000;
                        break;
                    case 2:
                        beamPart->rot[2] = 0;
                        beamPart->spin[2] = 0x1000;
                        break;
                    }
                }
            }
            break;
        case 1:
            lim = 7;
            if (++head->phaseTimer > lim) {
                head->phase++;
                head->phaseTimer = 0;
                lim = head->holdFrames;
                for (i = 0; i < n; i++) {
                    parts[i].frame = 0;
                }
            }
            break;
        case 2:
            lim = head->holdFrames;
            if (++head->phaseTimer > lim) {
                lim = 13;
                head->phase++;
                head->phaseTimer = 0;
                for (i = 0; i < n; i++) {
                    parts[i].frame = 0;
                }
                atk->stateVar0 = 0xFF;
                atk->hitPhase = 0;
            }
            break;
        case 3:
            lim = 13;
            if (++head->phaseTimer > lim) {
                atk->hitFrames = 0;
                atk->state = ATTACH_END;
                return;
            }
            break;
        default:
            atk->hitFrames = 0;
            atk->hitPhase = 0;
            atk->state = ATTACH_END;
            return;
        }
        beamPhase = head->phase;
        switch (beamPhase) {
        case 0:
            step = 2;
            key = 5;
            break;
        case 1:
            step = 1;
            key = 6;
            break;
        case 2:
            step = 0;
            key = -1;
            break;
        case 3:
            step = 1;
            key = 8;
            break;
        }
        if ((beamPhase == 1 || beamPhase == 2 || beamPhase == 3) && atk->area == gScene.area && !(atk->ownerMotion & 1)) {
            if (atk->stateVar1 == 1) {
                factor = 0.7f;
            } else {
                factor = 1.0f;
            }
            if (beamPhase == 3) {
                factor *= 1.0f - head->phaseTimer / (f32) lim;
            }
            if (factor > 0.0f) {
                MonsterFx_CreateAtAngled(atk->pos2, atk->rotX, atk->rotY, 0x24, factor);
            }
        }
        atk->yaw = atk->rotY;
        break;
    }
    for (i = 0; i < n; i++, parts++) {
        switch (atk->type) {
        case EMRANGED_FIREBALL:
            if (i < 3) {
                lim = 36;
                if (parts->idx != 0) {
                    key = 1;
                }
            } else {
                lim = 5;
            }
            break;
        case EMRANGED_KHEZU_GROUND_BOLT:
            lim = EmRangedAttack_GroundBoltPartFrames[parts->idx];
            break;
        case EMRANGED_KIRIN_BOLTS:
            if (parts->frame == 0) {
                if (head->stage == 0) {
                    parts->frame = -80;
                    atk->hitPhase = 0;
                } else {
                    EmRangedAttack_KirinNextStrike(atk, (u8 *) head);
                }
            }
            break;
        case EMRANGED_FATALIS_FIREBALL:
            if (i < 3) {
                lim = 36;
                if (parts->idx != 0) {
                    key = 1;
                }
            } else {
                lim = 5;
            }
            break;
        case EMRANGED_FATALIS_SWEEP:
            if (i < 3) {
                lim = 1000;
                if (parts->idx != 0) {
                    key = 1;
                }
            } else {
                lim = 5;
            }
            break;
        }
        if (++parts->frame <= 0) {
            key += step;
            continue;
        }
        if (parts->frame > lim) {
            switch (atk->type) {
            case EMRANGED_KUTKU_FIREBALL_SIDE:
            case EMRANGED_KUTKU_FIREBALL:
            case EMRANGED_GRAVIOS_SHOT:
            case EMRANGED_KUSHALA_BREATH:
                parts->frame = 1;
                break;
            case EMRANGED_KHEZU_GROUND_BOLT:
                EmRangedAttack_RestartBoltPart(atk, parts);
                if (parts->frame <= 0) {
                    key += step;
                    continue;
                }
                break;
            case EMRANGED_CEPHADROME_SHOT:
                break;
            case EMRANGED_KIRIN_BOLTS:
                parts->frame = 0;
                key += step;
                continue;
            case EMRANGED_FATALIS_SWEEP:
                if (i < 3) {
                    parts->frame = 0;
                    break;
                }
                key += step;
                continue;
            default:
                key += step;
                continue;
            }
        }
        switch (atk->type) {
        case EMRANGED_FIREBALL:
        case EMRANGED_FATALIS_FIREBALL:
        case EMRANGED_FATALIS_SWEEP:
            curve = EmRangedAttack_Curves[key++];
            Key_LerpVec3((void *) curve, parts->scale, parts->frame);
            if (parts->idx == 0) {
                if (!(atk->ownerMotion & 1)) {
                    atk->stateVar0++;
                    atk->stateVar0 &= 7;
                }
            } else {
                parts->rot[0] += parts->spin[0];
                parts->rot[1] += parts->spin[1];
                if (parts->idx == 1) {
                    parts->scale[0] *= 0.9f;
                    parts->scale[1] *= 0.9f;
                    parts->scale[2] *= 0.9f;
                }
            }
            switch (atk->type) {
            case 11:
                parts->scale[0] *= 1.5f;
                parts->scale[1] *= 1.5f;
                parts->scale[2] *= 1.5f;
                break;
            }
            break;
        case EMRANGED_PLESIOTH_STREAM:
            switch (head->phase) {
            case 0:
                curve = EmRangedAttack_Curves[key++];
            Key_LerpVec3((void *) curve, parts->scale, parts->frame);
                break;
            case 1:
                break;
            case 2:
                parts->scale[0] = 1.0f - parts->frame / (f32) lim;
                break;
            }
            parts->rot[2] += parts->spin[2];
            break;
        case EMRANGED_KUTKU_FIREBALL_SIDE:
        case EMRANGED_KUTKU_FIREBALL:
        case EMRANGED_PREY_SPIT:
        case EMRANGED_GRAVIOS_SHOT:
        case EMRANGED_HERMITAUR_SHOT:
        case EMRANGED_PREY_SPIT_B:
        case EMRANGED_REMOBRA_SPIT:
            parts->rot[2] += parts->spin[2];
            Key_LerpVec3((void *) EmRangedAttack_Curves[key], parts->scale, parts->frame);
            if (atk->type == EMRANGED_GRAVIOS_SHOT) {
                parts->scale[0] *= 2.0f;
                parts->scale[1] *= 2.0f;
                parts->scale[2] *= 2.0f;
            }
            break;
        case EMRANGED_GRAVIOS_BEAM:
        case EMRANGED_RAJANG_BEAM:
            switch (head->phase) {
            case 0:
                switch (parts->idx) {
                case 0:
                    parts->shown = 1;
                    curve = EmRangedAttack_Curves[key++];
                    Key_LerpVec3((void *) curve, parts->scale, parts->frame);
                    break;
                default:
                    parts->shown = 0;
                    continue;
                }
                break;
            case 1:
                parts->shown = 1;
                parts->alpha = 0xFF;
                switch (parts->idx) {
                case 0:
                case 1:
                    curve = EmRangedAttack_Curves[key++];
            Key_LerpVec3((void *) curve, parts->scale, parts->frame);
                    break;
                case 2:
                    parts->shown = 0;
                    parts->scale[0] = 1.0f;
                    parts->scale[1] = 1.0f;
                    parts->scale[2] = 1.0f;
                    if (atk->type == EMRANGED_RAJANG_BEAM && atk->stateVar1 == 1) {
                        parts->shown = 0;
                    }
                    break;
                }
                break;
            case 2:
                parts->alpha = 0xFF;
                switch (parts->idx) {
                case 0:
                    parts->shown = 0;
                    break;
                case 1:
                    parts->shown = 1;
                    parts->scale[0] = 0.8f + 0.4f / 1000.0f * (Rand_Next(1) & 0x3FF);
                    parts->scale[1] = 1.0f;
                    parts->scale[2] = 1.0f;
                    break;
                case 2:
                    parts->shown = 1;
                    parts->scale[0] = 1.0f;
                    parts->scale[1] = 1.0f;
                    parts->scale[2] = 1.0f;
                    if (atk->type == EMRANGED_RAJANG_BEAM && atk->stateVar1 == 1) {
                        parts->shown = 0;
                    }
                    break;
                }
                break;
            case 3:
                parts->alpha = 0xFF;
                switch (parts->idx) {
                case 0:
                    parts->shown = 0;
                    break;
                case 1:
                    parts->shown = 1;
                    curve = EmRangedAttack_Curves[key++];
            Key_LerpVec3((void *) curve, parts->scale, parts->frame);
                    break;
                case 2:
                    parts->shown = 1;
                    curve = EmRangedAttack_Curves[key++];
            Key_LerpVec3((void *) curve, parts->scale, parts->frame);
                    if (atk->type == EMRANGED_RAJANG_BEAM && atk->stateVar1 == 1) {
                        parts->shown = 0;
                    }
                    break;
                }
                break;
            }
            parts->rot[2] += parts->spin[2];
            break;
        case EMRANGED_KHEZU_GROUND_BOLT:
            curve = EmRangedAttack_Curves[key++];
            Key_LerpVec3((void *) curve, parts->scale, parts->frame);
            parts->scale[0] *= 1.5f;
            parts->scale[1] *= 1.5f;
            parts->scale[2] *= 1.5f;
            break;
        case EMRANGED_CEPHADROME_SHOT:
            switch (parts->idx) {
            case 0:
                parts->scale[0] = 0.8f + 0.2f / 1000.0f * (Rand_Next(1) & 0x3FF);
                break;
            default:
            case 1:
                factor = 0.8f + 0.2f / 1000.0f * (Rand_Next(1) & 0x3FF);
                parts->scale[0] = factor;
                parts->scale[1] = factor;
                break;
            }
            break;
        case EMRANGED_KIRIN_BOLTS:
            Key_LerpVec3((void *) EmRangedAttack_Curves[key], parts->scale, parts->frame);
            break;
        case EMRANGED_BLANGONGA_LUMP:
        case EMRANGED_THROW_FRAGMENT:
        case EMRANGED_CONGALALA_THROW:
        case EMRANGED_BIG_FRAGMENT:
            parts->rot[0] += parts->spin[0];
            parts->rot[1] += parts->spin[1];
            parts->rot[2] += parts->spin[2];
            break;
        case EMRANGED_KUSHALA_BREATH:
            switch (head->phase) {
            case 0:
                parts->scale[2] = 0.7f * (parts->frame / (f32) lim);
                break;
            case 1:
                curve = EmRangedAttack_Curves[key++];
            Key_LerpVec3((void *) curve, parts->scale, parts->frame);
                parts->scale[2] *= 0.7f;
                break;
            case 2:
                parts->scale[0] = 1.0f - parts->frame / (f32) lim;
                break;
            }
            parts->rot[2] += parts->spin[2];
            break;
        }
    }
    switch (atk->type) {
    case EMRANGED_FIREBALL:
    case EMRANGED_FATALIS_FIREBALL:
    case EMRANGED_FATALIS_SWEEP:
        switch (atk->type) {
        case 0:
            frameLimit = 33;
            break;
        case 11:
            frameLimit = 33;
            break;
        default:
            frameLimit = 36;
            break;
        }
        if (atk->ownerMotion <= frameLimit) {
            floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
            if (atk->pos[1] <= floorY) {
                if (atk->type != EMRANGED_FATALIS_SWEEP && floorY - atk->pos[1] > 100.0f) {
                    EmRangedAttack_Impact(atk, 2, atk->pos);
                } else {
                    pos[0] = atk->pos[0];
                    pos[1] = floorY;
                    pos[2] = atk->pos[2];
                    EmRangedAttack_Impact(atk, 0, pos);
                }
                atk->hitFrames = 0;
                atk->hitPhase = 0;
                atk->state = ATTACH_END;
            }
            if (atk->area == gScene.area) {
                Mtx_GetTrans(vec, gCamMatrix);
                vec[0] -= atk->pos[0];
                vec[1] -= atk->pos[1];
                vec[2] -= atk->pos[2];
                if (0.0f < Vec_Dot(vec, gCamDir)) {
                    dist = Vec_Dot(vec, vec);
                    if (10000.0f > dist) {
                        CamShake_SetAEverywhere(2);
                    }
                    if (90000.0f > dist) {
                        CamShake_SetAEverywhere(1);
                    } else if (360000.0f > dist) {
                        CamShake_SetAEverywhere(0);
                    }
                }
            }
        }
        break;
    case EMRANGED_PLESIOTH_STREAM:
        vec[0] = 0.0f;
        vec[1] = 0.0f;
        vec[2] = 20.0f * top->scale[2];
        Vec_RotateX(vec, 6.2831855f * atk->rotX / 65536.0f);
        Vec_RotateY(vec, 6.2831855f * atk->rotY / 65536.0f);
        atk->pos[0] = atk->pos2[0] + vec[0];
        atk->pos[1] = atk->pos2[1] + vec[1];
        atk->pos[2] = atk->pos2[2] + vec[2];
        switch (head->stage) {
        case 0:
            floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
            if (atk->pos[1] <= floorY) {
                head->stage++;
                pos[0] = atk->pos[0];
                if (floorY >= atk->pos2[1]) {
                    pos[1] = atk->pos[1];
                } else {
                    pos[1] = floorY;
                }
                pos[2] = atk->pos[2];
                head->floorY = pos[1];
                if (atk->stateVar1 == 1) {
                    EmRangedAttack_FloorCrossing(atk, (ObjAttach *) head, pos);
                    EmRangedAttack_CreateFrom(em, 10, 0, atk, pos);
                }
            }
            break;
        case 1:
            if (atk->stateVar1 != 1 && !(atk->ownerMotion & 1)) {
                EmRangedAttack_FloorCrossing(atk, (ObjAttach *) head, pos);
                EmRangedAttack_Impact(atk, 0, pos);
            }
            if (head->phase > 2) {
                head->stage++;
            }
            break;
        case 2:
            break;
        }
        break;
    case EMRANGED_KUTKU_FIREBALL_SIDE:
    case EMRANGED_KUTKU_FIREBALL:
    case EMRANGED_PREY_SPIT:
    case EMRANGED_GRAVIOS_SHOT:
    case EMRANGED_THROW_FRAGMENT:
    case EMRANGED_BLANGO_THROW:
    case EMRANGED_CONGALALA_THROW:
    case EMRANGED_BIG_FRAGMENT:
    case EMRANGED_HERMITAUR_SHOT:
    case EMRANGED_PREY_SPIT_B:
    case EMRANGED_REMOBRA_SPIT:
        floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
        if (atk->pos[1] <= floorY) {
            if (floorY - atk->pos[1] > 100.0f) {
                EmRangedAttack_Impact(atk, 2, atk->pos);
            } else {
                pos[0] = atk->pos[0];
                pos[1] = floorY;
                pos[2] = atk->pos[2];
                EmRangedAttack_Impact(atk, 0, pos);
            }
            atk->hitFrames = 0;
            atk->hitPhase = 0;
            atk->state = ATTACH_END;
        }
        break;
    case EMRANGED_BLANGONGA_LUMP:
        floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
        if (floorY <= head->floorY - 500.0f) {
            lowPoint[0] = atk->pos[0];
            lowPoint[1] = head->floorY;
            lowPoint[2] = atk->pos[2];
            floorY = AreaGrid2_GetFloorYFiltered(lowPoint);
        }
        if (atk->pos[1] <= floorY) {
            pos[0] = atk->pos[0];
            pos[1] = floorY;
            pos[2] = atk->pos[2];
            EmRangedAttack_Impact(atk, 0, pos);
            atk->hitFrames = 0;
            atk->hitPhase = 0;
            atk->state = ATTACH_END;
        }
        head->floorY = floorY;
        break;
    case EMRANGED_GRAVIOS_BEAM:
        vec[0] = 0.0f;
        vec[1] = 0.0f;
        vec[2] = 6400.0f;
        Vec_RotateX(vec, 6.2831855f * atk->rotX / 65536.0f);
        Vec_RotateY(vec, 6.2831855f * atk->rotY / 65536.0f);
        atk->pos[0] = atk->pos2[0] + vec[0];
        atk->pos[1] = atk->pos2[1] + vec[1];
        atk->pos[2] = atk->pos2[2] + vec[2];
        break;
    case EMRANGED_KHEZU_CEILING_SHOT:
        floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
        if (atk->pos[1] <= floorY) {
            if (floorY - atk->pos[1] > 200.0f) {
                EmRangedAttack_Impact(atk, 2, atk->pos);
            } else {
                pos[0] = atk->pos[0];
                pos[1] = floorY;
                pos[2] = atk->pos[2];
                EmRangedAttack_Impact(atk, 0, pos);
            }
            atk->hitFrames = 0;
            atk->hitPhase = 0;
            atk->state = ATTACH_END;
        }
        break;
    case EMRANGED_KHEZU_GROUND_BOLT:
        floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
        if (Math_Absf(floorY - atk->pos[1]) > 100.0f) {
            atk->hitFrames = 0;
            atk->hitPhase = 0;
            atk->state = ATTACH_END;
        } else {
            atk->pos[1] = floorY;
        }
        Vec_Sub(delta, atk->pos, atk->pos2);
        atk->rotX = (s32) (0.5f + 65536.0f * Math_Atan2(delta[1], Math_Sqrtf(delta[0] * delta[0] + delta[2] * delta[2])) / 6.2831855f) & 0xFFFF;
        if (spark && atk->area == gScene.area) {
            ThunderSpark_CreateAt(atk->pos, SPARK_KHEZU_GROUND_BOLT, ((Em *) atk->owner)->type, 1.0f);
        }
        break;
    case EMRANGED_CEPHADROME_SHOT:
        floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
        if (atk->pos[1] <= floorY) {
            if (floorY - atk->pos[1] > 100.0f) {
                EmRangedAttack_Impact(atk, 2, atk->pos);
            } else {
                pos[0] = atk->pos[0];
                pos[1] = floorY;
                pos[2] = atk->pos[2];
                EmRangedAttack_Impact(atk, 0, pos);
            }
            atk->hitFrames = 0;
            atk->hitPhase = 0;
            atk->state = ATTACH_END;
        }
        break;
    case EMRANGED_PLESIOTH_FLOOR_WAVE:
        floorY = AreaGrid2_GetFloorY(atk->pos);
        if (Math_Absf(floorY - atk->pos[1]) > 100.0f) {
            atk->hitFrames = 0;
            atk->hitPhase = 0;
            atk->state = ATTACH_END;
        } else {
            atk->pos[1] = floorY;
        }
        if (atk->area == gScene.area && !(atk->ownerMotion & 1)) {
            if (!(atk->ownerMotion & 2)) {
                Em_PlaySeChecked((Em *) atk->owner, 0x25, 0, atk->pos, 1, 0);
            }
            AttackBurst_CreateAt(atk->pos, 13, ((Em *) atk->owner)->type, 1.0f);
        }
        break;
    case EMRANGED_FLOOR_TRAIL:
        if (atk->ownerMotion % 3 == 0) {
            floorY = AreaGrid2_GetFloorY(atk->pos);
            pos[0] = atk->pos[0];
            pos[1] = floorY;
            pos[2] = atk->pos[2];
            MonsterFx_CreateAtYaw(pos, atk->rotY, 5, 1.0f);
            if (atk->ownerMotion >= 4) {
                MonsterFx_CreateAtYaw(pos, atk->rotY, (Rand_Next(1) & 1) + 7, 1.0f);
            }
            if (atk->ownerMotion >= 8) {
                MonsterFx_CreateAt(pos, 6, 1.0f);
            }
        }
        break;
    case EMRANGED_KUSHALA_BREATH:
        vec[0] = 0.0f;
        vec[1] = 0.0f;
        vec[2] = 1300.0f * top->scale[2];
        Vec_RotateX(vec, 6.2831855f * atk->rotX / 65536.0f);
        Vec_RotateY(vec, 6.2831855f * atk->rotY / 65536.0f);
        atk->pos[0] = atk->pos2[0] + vec[0];
        atk->pos[1] = atk->pos2[1] + vec[1];
        atk->pos[2] = atk->pos2[2] + vec[2];
        if (atk->ownerMotion % 5 == 0) {
            floorY = AreaGrid2_GetFloorYFiltered(atk->pos);
            if (atk->pos[1] > floorY - 50.0f && atk->pos[1] < 50.0f + floorY) {
                pos[0] = atk->pos[0];
                pos[1] = floorY;
                pos[2] = atk->pos[2];
                head->floorY = pos[1];
                EmRangedAttack_Impact(atk, 0, pos);
            } else if (atk->pos[1] < floorY && atk->pos2[1] > floorY) {
                factor = (floorY - atk->pos2[1]) / (atk->pos[1] - atk->pos2[1]);
                vec[0] = factor * (atk->pos[0] - atk->pos2[0]);
                vec[2] = factor * (atk->pos[2] - atk->pos2[2]);
                pos[0] = atk->pos2[0] + vec[0];
                pos[1] = floorY;
                pos[2] = atk->pos2[2] + vec[2];
                floorY = AreaGrid2_GetFloorYFiltered(pos);
                if (pos[1] > floorY - 50.0f || pos[1] < 50.0f + floorY) {
                    pos[1] = floorY;
                }
                EmRangedAttack_Impact(atk, 0, pos);
            }
        }
        break;
    case EMRANGED_RAJANG_BEAM:
        vec[0] = 0.0f;
        vec[1] = 0.0f;
        vec[2] = 6400.0f;
        Vec_RotateX(vec, 6.2831855f * atk->rotX / 65536.0f);
        Vec_RotateY(vec, 6.2831855f * atk->rotY / 65536.0f);
        atk->pos[0] = atk->pos2[0] + vec[0];
        atk->pos[1] = atk->pos2[1] + vec[1];
        atk->pos[2] = atk->pos2[2] + vec[2];
        break;
    }
    if (atk->drawSlotBuf != NULL) {
        switch (atk->type) {
        case EMRANGED_PLESIOTH_STREAM:
        case EMRANGED_GRAVIOS_BEAM:
        case EMRANGED_KUSHALA_BREATH:
        case EMRANGED_RAJANG_BEAM:
            Vec_Copy(atk->drawSlotBuf->pos, atk->pos2);
            OT_AddByDepth(gDrawOtSorted, atk->drawSlotBuf, 0x40, 1);
            break;
        case EMRANGED_KIRIN_BOLTS:
            Vec_Copy(atk->drawSlotBuf->pos, head->strikePos);
            OT_AddByDepth(gDrawOtSorted, atk->drawSlotBuf, 0x40, 1);
            break;
        case EMRANGED_PLESIOTH_FLOOR_WAVE:
            return;
        default:
            Vec_Copy(atk->drawSlotBuf->pos, atk->pos);
            OT_AddByDepth(gDrawOtSorted, atk->drawSlotBuf, 0x40, 0);
            break;
        }
    }
    if (head->ds != NULL) {
        switch (atk->type) {
        case EMRANGED_FIREBALL:
        case EMRANGED_FATALIS_FIREBALL:
        case EMRANGED_FATALIS_SWEEP:
        case EMRANGED_BLANGONGA_LUMP:
            head->ds->pos[0] = atk->pos[0];
            head->ds->pos[1] = 10.0f + floorY;
            head->ds->pos[2] = atk->pos[2];
            OT_AddByDepth(gDrawOtSorted, head->ds, 0x40, 0);
            break;
        case EMRANGED_KUTKU_FIREBALL_SIDE:
        case EMRANGED_KUTKU_FIREBALL:
        case EMRANGED_PREY_SPIT:
        case EMRANGED_GRAVIOS_SHOT:
        case EMRANGED_HERMITAUR_SHOT:
        case EMRANGED_PREY_SPIT_B:
        case EMRANGED_REMOBRA_SPIT:
            vec[0] = 0.0f;
            vec[1] = 0.0f;
            vec[2] = -20.0f;
            Vec_RotateSelf(vec, gCamMatrix);
            head->ds->pos[0] = atk->pos[0] + vec[0];
            head->ds->pos[1] = atk->pos[1] + vec[1];
            head->ds->pos[2] = atk->pos[2] + vec[2];
            OT_AddByDepth(gDrawOtSorted, head->ds, 0x40, 0);
            break;
        case EMRANGED_KHEZU_GROUND_BOLT:
            Vec_Copy(head->ds->pos, atk->pos);
            OT_AddByDepth(gDrawOtSorted, head->ds, 0x40, 0);
            break;
        }
    }
}

/*
 * Runs when the attack hit a monster or a player (ATTACH_HIT). The fireballs,
 * Khezu's ceiling shot, Cephadrome's shot and Congalala's throw burst there
 * (EmRangedAttack_Impact, how 1) and end; the other shots and thrown objects
 * end without a burst; the streams, beams, Kirin's strikes, Khezu's ground
 * lightning, the floor wave and Blangonga's lump keep running
 * (EmRangedAttack_Run).
 */
void EmRangedAttack_Hit(ObjAttach *atk)
{
    f32 pos[3];

    switch (atk->type) {
    case EMRANGED_FIREBALL:
    case EMRANGED_KHEZU_CEILING_SHOT:
    case EMRANGED_CEPHADROME_SHOT:
    case EMRANGED_FATALIS_FIREBALL:
    case EMRANGED_FATALIS_SWEEP:
    case EMRANGED_CONGALALA_THROW:
        pos[0] = atk->pos[0];
        pos[1] = atk->pos[1];
        pos[2] = atk->pos[2];
        atk->hitFrames = 0;
        atk->state = ATTACH_END;
        EmRangedAttack_Impact(atk, 1, pos);
        break;
    case EMRANGED_KUTKU_FIREBALL_SIDE:
    case EMRANGED_KUTKU_FIREBALL:
    case EMRANGED_PREY_SPIT:
    case EMRANGED_GRAVIOS_SHOT:
    case EMRANGED_FLOOR_TRAIL:
    case EMRANGED_THROW_FRAGMENT:
    case EMRANGED_BLANGO_THROW:
    case EMRANGED_BIG_FRAGMENT:
    case EMRANGED_HERMITAUR_SHOT:
    case EMRANGED_PREY_SPIT_B:
    case EMRANGED_REMOBRA_SPIT:
        atk->hitFrames = 0;
        atk->state = ATTACH_END;
        break;
    default:
        EmRangedAttack_Run(atk);
        break;
    }
}

/*
 * Ends an EmRangedAttack: frees its main draw slot and, when ds is set, the
 * buffer's second slot (EmRangedAttackBuf.slot and .ds, read here as raw
 * bytes of the buffer), closes the hit window, hides it and moves to
 * ATTACH_FREE.
 */
void EmRangedAttack_End(ObjAttach *atk)
{
    u8 *w;

    w = atk->buf;
    if (atk->drawSlotBuf != NULL) {
        DrawSlot_Free(atk->drawSlot);
    }
    if (*(s32 *)(w + 8) != 0) {
        DrawSlot_Free(*(s16 *)(w + 4));
    }
    atk->hitPhase = 0;
    atk->state = ATTACH_FREE;
    atk->visible = 0;
}

/* Returns the object to the pool (ObjAttach_Free). */
void EmRangedAttack_Free(ObjAttach *atk)
{
    ObjAttach_Free(atk);
}

/*
 * Draws the mesh set set with the matrix mtx, the colour color (0xAARRGGBB),
 * the blend flags flags and the material list mats, then restores the default
 * draw state. Draws nothing when set is NULL or has no model (id -1).
 */
void EmRangedAttack_DrawModel(ModelMatSet *set, f32 *mtx, u32 color, s32 flags, ModelMat *mats)
{
    if ((set != NULL) && (set->id != -1)) {
        Gs_SetState(RS_MTX_SLOT(0), (s32) mtx);
        Gs_SetState(RS_COLOR2, (s32) color);
        Gs_SetLightList(mats, set);
        Draw_ApplyStates(set->flags);
        Draw_SetBlendFlags(flags);
        Model_Draw(set->id, 0);
        Draw_SetDefaultState();
    }
}

s16 EmSlot_FindKind(s16 id);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateZXY(f32 *m, f32 x, f32 y, f32 z);
void Eff_ColorKeyLerp(ColorKey *k, s32 t, u32 *out);
void EmRangedAttack_Billboard(f32 *arg0, void *arg1);
extern f32 gCamYawMtx[];
extern s16 EmRangedAttack_Models[];
extern s16 EmRangedAttack_Lives[];
extern ColorKey *EmRangedAttack_FireballPartKeys[];
extern ColorKey *EmRangedAttack_FatalisFireballPartKeys[];
extern ColorKey EmRangedAttack_KutkuSideShotKeys[];
extern ColorKey EmRangedAttack_FireShotKeys[];
extern ColorKey EmRangedAttack_PreySpitBKeys[];
extern ColorKey EmRangedAttack_SpitKeys[];
extern ColorKey EmRangedAttack_GraviosBeamStartKeys[];
extern ColorKey EmRangedAttack_BeamEndKeys[];
extern ColorKey EmRangedAttack_KirinBoltKeys[];
extern ColorKey EmRangedAttack_RajangBeamStartKeys[];
extern s16 EmRangedAttack_GroundBoltPartFrames[];
extern ColorKey *EmRangedAttack_GroundBoltPartKeys[];
extern s16 EmRangedAttack_GroundBoltPartModels[];
extern s16 EmRangedAttack_KirinBoltModels[];

/*
 * Draws one draw slot of an EmRangedAttack (slot->part 0: the main slot, 1:
 * the buffer's second slot), only while the object is in the current area.
 * Most types draw the owner monster's part models (gScene.partModels of the
 * kind in aux); the spits, the fragments and the second slots' floor glow and
 * shadow use gStageModelSet. By type it builds each part's matrix from its
 * scale and rotation (facing the camera through gCamMatrix or
 * EmRangedAttack_Billboard for some), takes its colour from the colour keys
 * (Eff_ColorKeyLerp) and scrolls its texture (RS_UV_MTX); a falling thrown
 * lump's shadow fades with its height. Draws nothing when a model set it
 * needs is missing.
 */
void EmRangedAttack_DrawParts(DrawSlot *slot)
{
    u8 rajangFacesCamera;
    u8 graviosFacesCamera;
    ObjAttach *atk;
    EmRangedAttackBuf *head;
    EmRangedAttackPart *parts;
    Model *emModel;
    Model *stageModel;
    ModelMat *emMats;
    ModelMat *mats;
    ModelMatSet *mdl;
    ModelMat *mat;
    ColorKey **keysp;
    ColorKey *keys;
    s16 model;
    s32 mode;
    s16 matSetIdx;
    s16 boltParts;
    u8 alpha;
    u8 red;
    u8 green;
    u8 blue;
    u8 flag;
    s16 glowCount;
    s16 i;
    s16 n;
    s16 k;
    s16 last;
    s32 div;
    s16 thr;
    s32 j;
    f32 fade;
    f32 unusedScale;
    f32 dy;
    f32 yawAngle;
    f32 angleX;
    f32 angleY;
    f32 angleZ;
    u32 color;
    f32 mtx[16];
    f32 uvMtx[16];
    f32 scaleVec[3];
    s16 lim;

    mode = 0;
    atk = slot->owner;
    stageModel = gStageModelSet;
    head = (EmRangedAttackBuf *) atk->buf;
    parts = head->part;
    if (atk->area != gScene.area) {
        goto end;
    }
    switch (atk->type) {
    default:
        k = EmSlot_FindKind(atk->aux);
        if (k == -1) {
            goto end;
        }
        emModel = gScene.partModels[k];
        if (emModel == NULL || !emModel->active) {
            goto end;
        }
        emMats = emModel->mats;
        break;
    case EMRANGED_PREY_SPIT:
    case EMRANGED_THROW_FRAGMENT:
    case EMRANGED_BIG_FRAGMENT:
    case EMRANGED_HERMITAUR_SHOT:
    case EMRANGED_PREY_SPIT_B:
    case EMRANGED_REMOBRA_SPIT:
        break;
    }
    if (stageModel == NULL || !stageModel->active) {
        goto end;
    }
    mats = stageModel->mats;
    Gs_SetState(RS_ALPHA_REF, 0);
    model = EmRangedAttack_Models[atk->type];
    switch (atk->type) {
    case EMRANGED_FIREBALL:
    case EMRANGED_FATALIS_FIREBALL:
    case EMRANGED_FATALIS_SWEEP:
        last = EmRangedAttack_Lives[atk->type];
        if (atk->type == EMRANGED_FIREBALL) {
            div = 6;
            thr = 30;
            glowCount = 1;
            red = 0xFF;
            n = 3;
            green = 0xCB;
            keysp = EmRangedAttack_FireballPartKeys;
            blue = 0x79;
        } else if (atk->type == EMRANGED_FATALIS_FIREBALL) {
            div = 6;
            thr = 30;
            glowCount = 1;
            red = 0xFF;
            n = 3;
            green = 0xF3;
            keysp = EmRangedAttack_FatalisFireballPartKeys;
            blue = 0xDF;
        } else {
            div = 1000;
            thr = 1000;
            glowCount = 1;
            red = 0xFF;
            n = 3;
            green = 0xF3;
            keysp = EmRangedAttack_FatalisFireballPartKeys;
            blue = 0xDF;
        }
        if (atk->ownerMotion < thr) {
            alpha = 0xFF;
        } else {
            fade = (1.0f / div) * (last - atk->ownerMotion);
            alpha = 255.0f * fade;
        }
        switch (slot->part) {
        case 0:
            Gs_SetState(RS_ZWRITE, 0);
            for (i = 0; i < n; i++) {
                angleX = (6.2831855f * parts->rot[0]) / 65536.0f;
                angleY = (6.2831855f * parts->rot[1]) / 65536.0f;
                angleZ = (6.2831855f * parts->rot[2]) / 65536.0f;
                if (atk->ownerMotion < thr) {
                    scaleVec[0] = parts->scale[0];
                    scaleVec[1] = parts->scale[1];
                    scaleVec[2] = parts->scale[2];
                } else {
                    scaleVec[0] = parts->scale[0] * (3.5f - 2.5f * fade);
                    scaleVec[1] = parts->scale[1] * (3.5f - 2.5f * fade);
                    scaleVec[2] = parts->scale[2] * (3.5f - 2.5f * fade);
                }
                keys = *keysp;
                Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
                Mtx_RotateXYZ(mtx, angleX, angleY, angleZ);
                Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
                if (parts->idx == 0) {
                    matSetIdx = 0;
                    Mtx_Translation(uvMtx, 0.125f * atk->stateVar0, 0.0f, 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    Mtx_Concat3x3(mtx, gCamMatrix);
                    mode = (u16) (mode | 2);
                    Eff_ColorKeyLerp(keys, atk->stateVar0, &color);
                } else {
                    matSetIdx = 1;
                    if (parts->idx == 2) {
                        mode = (u16) (mode | 2);
                    }
                    Eff_ColorKeyLerp(keys, atk->ownerMotion % 9, &color);
                }
                mdl = &emModel->matSets[matSetIdx];
                color = (alpha << 24) | (((color >> 16) & 0xFF) << 16) | (((color >> 8) & 0xFF) << 8) | (color & 0xFF);
                EmRangedAttack_DrawModel(mdl, mtx, color, mode, emMats);
                mode = 0;
                parts++;
                keysp++;
            }
            break;
        case 1:
            parts += n;
            Gs_SetState(RS_ZWRITE, 0);
            mdl = &stageModel->matSets[59];
            for (i = 0; i < glowCount; i++) {
                scaleVec[0] = 4.0f * parts->scale[0];
                scaleVec[1] = 4.0f * parts->scale[1];
                scaleVec[2] = 4.0f * parts->scale[2];
                Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
                Mtx_RotateX(mtx, (6.2831855f * parts->rot[0]) / 65536.0f);
                Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
                EmRangedAttack_DrawModel(mdl, mtx, ((u8) (0.3f * alpha) << 24) | (red << 16) | (green << 8) | blue, mode, mats);
                mode = 0;
                parts++;
            }
            Gs_SetState(RS_ZWRITE, 1);
            break;
        }
        break;
    case EMRANGED_PLESIOTH_STREAM:
        Mtx_Identity(mtx);
        Mtx_RotateXYZ(mtx, (6.2831855f * atk->rotX) / 65536.0f, (6.2831855f * atk->rotY) / 65536.0f, 0.0f);
        EmRangedAttack_Billboard(mtx, atk->pos2);
        mtx[0] *= parts->scale[0];
        mtx[1] *= parts->scale[0];
        mtx[2] *= parts->scale[0];
        mtx[4] *= parts->scale[1];
        mtx[5] *= parts->scale[1];
        mtx[6] *= parts->scale[1];
        mtx[8] *= parts->scale[2];
        mtx[9] *= parts->scale[2];
        mtx[10] *= parts->scale[2];
        mdl = &emModel->matSets[model];
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        Gs_SetState(RS_ZWRITE, 0);
        Mtx_Translation(uvMtx, 0.0f, 0.1f * (atk->ownerMotion % 10), 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        EmRangedAttack_DrawModel(mdl, mtx, -1, 2, emMats);
        break;
    case EMRANGED_KUTKU_FIREBALL_SIDE:
    case EMRANGED_KUTKU_FIREBALL:
    case EMRANGED_PREY_SPIT:
    case EMRANGED_GRAVIOS_SHOT:
    case EMRANGED_HERMITAUR_SHOT:
    case EMRANGED_PREY_SPIT_B:
    case EMRANGED_REMOBRA_SPIT:
        switch (slot->part) {
        case 0:
            break;
        case 1:
            parts++;
            break;
        }
        if (parts->frame > 0 && parts->frame < 11) {
            Vec_Copy(scaleVec, parts->scale);
            switch (atk->type) {
            case 24:
                scaleVec[0] *= EmRangedAttack_RemobraSpitScales[atk->stateVar1];
                scaleVec[1] *= EmRangedAttack_RemobraSpitScales[atk->stateVar1];
                scaleVec[2] *= EmRangedAttack_RemobraSpitScales[atk->stateVar1];
            case 9:
            case 22:
            case 23:
                mdl = &stageModel->matSets[model];
                scaleVec[0] *= 0.5f;
                scaleVec[1] *= 0.5f;
                scaleVec[2] *= 0.5f;
                break;
            default:
                mats = emMats;
                mdl = &emModel->matSets[model];
                break;
            }
            switch (atk->type) {
            case 2:
                keys = EmRangedAttack_KutkuSideShotKeys;
                break;
            case 3:
            case 13:
                keys = EmRangedAttack_FireShotKeys;
                break;
            case 23:
                keys = EmRangedAttack_PreySpitBKeys;
                break;
            default:
                keys = EmRangedAttack_SpitKeys;
                break;
            }
            Eff_ColorKeyLerp(keys, parts->frame, &color);
            Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
            Mtx_RotateZ(mtx, (6.2831855f * parts->rot[2]) / 65536.0f);
            Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
            Mtx_Concat3x3(mtx, gCamMatrix);
            EmRangedAttack_DrawModel(mdl, mtx, color, 0, mats);
        }
        break;
    case EMRANGED_GRAVIOS_BEAM:
        Gs_SetState(RS_ZWRITE, 0);
        for (i = 0; i < 3; i++, parts++) {
            if (parts->shown == 0) {
                continue;
            }
            graviosFacesCamera = 0;
            switch ((u8) head->phase) {
            default:
                continue;
            case 0:
                switch (parts->idx) {
                case 0:
                    k = 0;
                    Mtx_Translation(uvMtx, 0.0f, 1.0f - 0.125f * (atk->ownerMotion & 7), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    Eff_ColorKeyLerp(EmRangedAttack_GraviosBeamStartKeys, parts->frame, &color);
                    break;
                default:
                    continue;
                }
                break;
            case 1:
                switch (parts->idx) {
                default:
                    continue;
                case 0:
                    graviosFacesCamera = 1;
                    k = 7;
                    break;
                case 1:
                    k = 0;
                    Mtx_Translation(uvMtx, 0.0f, 1.0f - 0.125f * (atk->ownerMotion & 7), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    break;
                case 2:
                    k = 1;
                    Mtx_Translation(uvMtx, 0.0f, 0.1f * (atk->ownerMotion % 10), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    break;
                }
                color = (parts->alpha << 24) | 0xFFFFFF;
                break;
            case 2:
            case 3:
                flag = 0;
                switch (parts->idx) {
                default:
                    continue;
                case 1:
                    k = 0;
                    Mtx_Translation(uvMtx, 0.0f, 1.0f - 0.125f * (atk->ownerMotion & 7), 0.0f);
                    if ((u8) head->phase == 3) {
                        flag = 1;
                    }
                    break;
                case 2:
                    k = 1;
                    Mtx_Translation(uvMtx, 0.0f, 0.1f * (atk->ownerMotion % 5), 0.0f);
                    break;
                }
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                if (flag) {
                    Eff_ColorKeyLerp(EmRangedAttack_BeamEndKeys, parts->frame, &color);
                } else {
                    color = (parts->alpha << 24) | 0xFFFFFF;
                }
                break;
            }
            if (graviosFacesCamera) {
                Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
                Mtx_Concat3x3(mtx, gCamMatrix);
            } else {
                Mtx_Scale(mtx, parts->scale[0], parts->scale[1], 4.0f);
                Mtx_RotateZXY(mtx, (6.2831855f * atk->rotX) / 65536.0f, (6.2831855f * atk->rotY) / 65536.0f,
                              (6.2831855f * parts->rot[2]) / 65536.0f);
            }
            Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
            EmRangedAttack_DrawModel(&emModel->matSets[k], mtx, color, 2, emMats);
        }
        break;
    case EMRANGED_KHEZU_CEILING_SHOT:
        mdl = &emModel->matSets[model];
        Mtx_Identity(mtx);
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        Mtx_Concat3x3(mtx, gCamYawMtx);
        EmRangedAttack_DrawModel(mdl, mtx, -1, 0, emMats);
        break;
    case EMRANGED_KHEZU_GROUND_BOLT:
        switch (slot->part) {
        case 0:
            boltParts = 1;
            break;
        case 1:
            boltParts = 5;
            parts++;
            break;
        }
        Gs_SetState(RS_ZWRITE, 0);
        for (i = 0; i < boltParts; i++) {
            lim = EmRangedAttack_GroundBoltPartFrames[parts->idx];
            if (parts->frame > 0 && parts->frame <= lim) {
                Eff_ColorKeyLerp(EmRangedAttack_GroundBoltPartKeys[parts->idx], parts->frame, &color);
                k = EmRangedAttack_GroundBoltPartModels[parts->idx];
                switch (slot->part) {
                case 0:
                    Mtx_Identity(mtx);
                    Mtx_RotateXYZ(mtx, (6.2831855f * atk->rotX) / 65536.0f, (6.2831855f * atk->rotY) / 65536.0f, 0.0f);
                    EmRangedAttack_Billboard(mtx, atk->pos);
                    mtx[0] *= parts->scale[0];
                    mtx[1] *= parts->scale[0];
                    mtx[2] *= parts->scale[0];
                    mtx[4] *= parts->scale[1];
                    mtx[5] *= parts->scale[1];
                    mtx[6] *= parts->scale[1];
                    mtx[8] *= parts->scale[2];
                    mtx[9] *= parts->scale[2];
                    mtx[10] *= parts->scale[2];
                    Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
                    mode = (u16) (mode | 2);
                    break;
                case 1:
                    scaleVec[0] = parts->scale[0];
                    scaleVec[1] = parts->scale[1];
                    scaleVec[2] = parts->scale[2];
                    Mtx_Scale(mtx, scaleVec[0], scaleVec[1], scaleVec[2]);
                    switch (parts->idx) {
                    case 3:
                        Gs_SetState(RS_ALPHA_REF, 0x80);
                        yawAngle = (6.2831855f * atk->rotY) / 65536.0f;
                        Mtx_RotateZ(mtx, (6.2831855f * parts->rot[2]) / 65536.0f);
                        Mtx_RotateY(mtx, yawAngle);
                        Mtx_Translation(uvMtx, 0.0f, 0.0234375f * parts->frame, 0.0f);
                        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                        break;
                    default:
                        Gs_SetState(RS_ALPHA_REF, 0);
                        Mtx_RotateZ(mtx, (6.2831855f * parts->rot[2]) / 65536.0f);
                        Mtx_Concat3x3(mtx, gCamMatrix);
                        mode = (u16) (mode | 2);
                        break;
                    }
                    Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
                    break;
                }
                EmRangedAttack_DrawModel(&emModel->matSets[k], mtx, color, mode, emMats);
            }
            parts++;
        }
        break;
    case EMRANGED_CEPHADROME_SHOT:
        Gs_SetState(RS_ZWRITE, 0);
        for (i = 0; i < 2; i++) {
            switch (parts->idx) {
            case 0:
                Mtx_Identity(mtx);
                Mtx_RotateXYZ(mtx, (6.2831855f * atk->rotX) / 65536.0f, (6.2831855f * atk->rotY) / 65536.0f, 0.0f);
                EmRangedAttack_Billboard(mtx, atk->pos);
                mtx[0] *= parts->scale[0];
                mtx[1] *= parts->scale[0];
                mtx[2] *= parts->scale[0];
                mtx[8] *= 2.0f;
                mtx[9] *= 2.0f;
                mtx[10] *= 2.0f;
                Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
                model = 5;
            default:
                Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
                mdl = &emModel->matSets[model];
                color = 0xFFD8B68A;
                EmRangedAttack_DrawModel(mdl, mtx, color, 0, emMats);
                parts++;
                break;
            case 1:
                break;
            }
        }
        break;
    case EMRANGED_KIRIN_BOLTS:
        if (parts->frame > 0 && parts->frame < 9) {
            Gs_SetState(RS_ZWRITE, 0);
            mdl = &emModel->matSets[EmRangedAttack_KirinBoltModels[(u8) head->boltModel]];
            Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
            if (atk->stateVar0 & 1) {
                Mtx_RotateY(mtx, 3.1415927f);
            }
            Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
            Eff_ColorKeyLerp(EmRangedAttack_KirinBoltKeys, parts->frame, &color);
            Mtx_Translation(uvMtx, 0.25f * (u8) head->phase, 0.125f - 0.015625f * parts->frame, 0.0f);
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            Mtx_Concat3x3(mtx, gCamMatrix);
            EmRangedAttack_DrawModel(mdl, mtx, color, 2, emMats);
        }
        break;
    case EMRANGED_FLOOR_TRAIL:
        Gs_SetState(RS_ZWRITE, 0);
        Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
        Mtx_RotateXYZ(mtx, (6.2831855f * atk->rotX) / 65536.0f, (6.2831855f * atk->rotY) / 65536.0f, 0.0f);
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        mdl = &emModel->matSets[model];
        color = -1;
        Mtx_Translation(uvMtx, (atk->ownerMotion & 0xF) / 16.0f, 1.0f, 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        EmRangedAttack_DrawModel(mdl, mtx, color, 2, emMats);
        break;
    case EMRANGED_THROW_FRAGMENT:
    case EMRANGED_BIG_FRAGMENT:
        Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
        Mtx_RotateXYZ(mtx, (6.2831855f * parts->rot[0]) / 65536.0f, (6.2831855f * parts->rot[1]) / 65536.0f,
                      (6.2831855f * parts->rot[2]) / 65536.0f);
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        if (atk->type == EMRANGED_BIG_FRAGMENT && parts->idx != 0) {
            mdl = &stageModel->matSets[73];
        } else {
            mdl = &stageModel->matSets[model];
        }
        color = -1;
        EmRangedAttack_DrawModel(mdl, mtx, color, 0, mats);
        break;
    case EMRANGED_BLANGONGA_LUMP:
    case EMRANGED_BLANGO_THROW:
        color = -1;
        switch (slot->part) {
        case 0:
            Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
            Mtx_RotateXYZ(mtx, (6.2831855f * parts->rot[0]) / 65536.0f, (6.2831855f * parts->rot[1]) / 65536.0f,
                          (6.2831855f * parts->rot[2]) / 65536.0f);
            Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
            EmRangedAttack_DrawModel(&emModel->matSets[model], mtx, color, 0, emMats);
            break;
        case 1:
            Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
            Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
            mdl = &stageModel->matSets[50];
            dy = atk->pos[1] - slot->pos[1];
            for (j = 0; j < mdl->count; j++) {
                mat = &mats[mdl->mat[j]];
                if (!(atk->velY < 0.0f)) {
                    mat->alpha = 0.0f;
                } else if (!(dy < 3500.0f)) {
                    mat->alpha = 0.0f;
                } else if (dy <= 2000.0f) {
                    mat->alpha = 1.0f;
                } else {
                    mat->alpha = 1.0f - (dy - 2000.0f) / 1500.0f;
                }
                Gs_SetState((u8) RS_MATERIAL(j), (s32) mat);
            }
            EmRangedAttack_DrawModel(mdl, mtx, color, 0, mats);
            break;
        }
        break;
    case EMRANGED_KUSHALA_BREATH:
        Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
        Mtx_RotateZXY(mtx, (6.2831855f * atk->rotX) / 65536.0f, (6.2831855f * atk->rotY) / 65536.0f,
                      (6.2831855f * parts->rot[2]) / 65536.0f);
        mdl = &emModel->matSets[model];
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        Gs_SetState(RS_ZWRITE, 0);
        Mtx_Translation(uvMtx, 0.0f, 0.1f * (atk->ownerMotion % 10), 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        EmRangedAttack_DrawModel(mdl, mtx, -1, 2, emMats);
        break;
    case EMRANGED_CONGALALA_THROW:
        Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
        Mtx_RotateXYZ(mtx, (6.2831855f * parts->rot[0]) / 65536.0f, (6.2831855f * parts->rot[1]) / 65536.0f,
                      (6.2831855f * parts->rot[2]) / 65536.0f);
        Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
        mdl = &emModel->matSets[model];
        color = 0xFF808080;
        EmRangedAttack_DrawModel(mdl, mtx, color, 0, emMats);
        break;
    case EMRANGED_RAJANG_BEAM:
        Gs_SetState(RS_ZWRITE, 0);
        for (i = 0; i < 3; i++, parts++) {
            if (parts->shown == 0) {
                continue;
            }
            rajangFacesCamera = 0;
            switch ((u8) head->phase) {
            default:
                continue;
            case 0:
                switch (parts->idx) {
                case 0:
                    k = 0;
                    Mtx_Translation(uvMtx, 0.0f, 1.0f - 0.125f * (atk->ownerMotion & 7), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    Eff_ColorKeyLerp(EmRangedAttack_RajangBeamStartKeys, parts->frame, &color);
                    break;
                default:
                    continue;
                }
                break;
            case 1:
                switch (parts->idx) {
                default:
                    continue;
                case 0:
                    rajangFacesCamera = 1;
                    k = 4;
                    break;
                case 1:
                    k = 0;
                    Mtx_Translation(uvMtx, 0.0f, 1.0f - 0.125f * (atk->ownerMotion & 7), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    break;
                case 2:
                    k = 1;
                    Mtx_Translation(uvMtx, 0.0f, 0.1f * (atk->ownerMotion % 10), 0.0f);
                    Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                    break;
                }
                color = (parts->alpha << 24) | 0xFFFFFF;
                break;
            case 2:
            case 3:
                flag = 0;
                switch (parts->idx) {
                default:
                    continue;
                case 1:
                    k = 0;
                    Mtx_Translation(uvMtx, 0.0f, 1.0f - 0.125f * (atk->ownerMotion & 7), 0.0f);
                    if ((u8) head->phase == 3) {
                        flag = 1;
                    }
                    break;
                case 2:
                    k = 1;
                    Mtx_Translation(uvMtx, 0.0f, 0.1f * (atk->ownerMotion % 5), 0.0f);
                    break;
                }
                Gs_SetState(RS_UV_MTX, (s32) uvMtx);
                if (flag) {
                    Eff_ColorKeyLerp(EmRangedAttack_BeamEndKeys, parts->frame, &color);
                } else {
                    color = (parts->alpha << 24) | 0xFFFFFF;
                }
                break;
            }
            if (rajangFacesCamera) {
                if (atk->stateVar1 == 1) {
                    Mtx_Scale(mtx, 0.5f * parts->scale[0], 0.5f * parts->scale[1], parts->scale[2]);
                } else {
                    Mtx_Scale(mtx, parts->scale[0], parts->scale[1], parts->scale[2]);
                }
                Mtx_Concat3x3(mtx, gCamMatrix);
            } else {
                if (atk->stateVar1 == 1) {
                    Mtx_Scale(mtx, 0.5f * parts->scale[0], 0.5f * parts->scale[1], 4.0f);
                } else {
                    Mtx_Scale(mtx, parts->scale[0], parts->scale[1], 4.0f);
                }
                Mtx_RotateZXY(mtx, (6.2831855f * atk->rotX) / 65536.0f, (6.2831855f * atk->rotY) / 65536.0f,
                              (6.2831855f * parts->rot[2]) / 65536.0f);
            }
            Mtx_SetTrans(mtx, slot->pos[0], slot->pos[1], slot->pos[2]);
            EmRangedAttack_DrawModel(&emModel->matSets[k], mtx, color, 2, emMats);
        }
        break;
    }
    Draw_SetDefaultState();
    Gfx_SetBlend(4, 5);
    Gs_SetState(RS_ZWRITE, 1);
end:;
}

/*
 * Leaves the burst of an EmRangedAttack at pos: by type, the sound, the fire,
 * spark and splash effects, the camera shake and the Explosion
 * (EXPLOSION_FIREBALL or EXPLOSION_FIREBALL_BIG by variant for the fireball,
 * EXPLOSION_FATALIS_FIREBALL, the Kut-Ku, Cephadrome and Gravios shots' own).
 * Blangonga's thrown lump breaks into three EMRANGED_THROW_FRAGMENT. how is 0
 * when it reached the floor (pos on it), 1 when it hit a target, 2 when the
 * floor is far under it; the fireballs' Explosion comes only at 0 and 2.
 * Effects and sounds come only in the current area, the Explosion always.
 */
void EmRangedAttack_Impact(ObjAttach *atk, s16 how, f32 *pos)
{
    f32 scale;
    f32 fxScale;
    s32 explosionType;

    switch (atk->type) {
    case EMRANGED_FIREBALL:
    case EMRANGED_FATALIS_FIREBALL:
    case EMRANGED_FATALIS_SWEEP:
        if (atk->type == EMRANGED_FATALIS_FIREBALL || atk->type == EMRANGED_FATALIS_SWEEP) {
            scale = 2.25f;
            explosionType = EXPLOSION_FATALIS_FIREBALL;
        } else if (atk->stateVar1 == 0) {
            scale = 1.5f;
            explosionType = EXPLOSION_FIREBALL;
        } else {
            scale = 2.15f;
            explosionType = EXPLOSION_FIREBALL_BIG;
        }
        if (atk->area == gScene.area) {
            Em_PlaySeChecked(atk->owner, 0x25, 0, pos, 15, 0);
            fxScale = 0.15f * scale;
            FireBlast_CreateScaled((f32 *) pos, 1, fxScale);
            AttackBurst_CreateAtScaled(pos, atk->rotY, 4, fxScale);
            AttackBurst_CreateAtScaled(pos, atk->rotY, 9, scale);
        }
        switch (how) {
        case 0:
            if (atk->type != EMRANGED_REMOBRA_SPIT && atk->area == gScene.area) {
                AttackBurst_CreateAtScaled(pos, atk->rotY, 8, scale);
                AttackBurst_CreateAtScaled(pos, atk->rotY, 0xB, scale);
                CamShake_SetA(2, pos);
            }
            Explosion_CreateOwned(atk->owner, pos, explosionType);
            break;
        case 2:
            Explosion_CreateOwned(atk->owner, pos, explosionType);
            break;
        }
        break;
    case EMRANGED_PLESIOTH_STREAM:
        if (atk->area == gScene.area) {
            if (atk->ownerMotion & 2) {
                Em_PlaySeChecked(atk->owner, 0x25, 0, pos, 1, 0);
            }
            AttackBurst_CreateAt(pos, 0xD, ((Em *) atk->owner)->type, 1.0f);
        }
        break;
    case EMRANGED_KUTKU_FIREBALL_SIDE:
        if (atk->area == gScene.area) {
            Em_PlaySeChecked(atk->owner, 0x25, 0, pos, 1, 0);
        }
        if (atk->area == gScene.area) {
            AttackBurst_CreateAtScaled(pos, atk->rotY, 5, 1.0f);
            AttackBurst_CreateAtYaw(pos, 0xF, ((Em *) atk->owner)->type, atk->rotY, 1.4f);
        }
        if (atk->stateVar1 == 1) {
            Explosion_CreateOwned(atk->owner, pos, EXPLOSION_KUTKU_FIREBALL_SIDE_B);
        } else {
            Explosion_CreateOwned(atk->owner, pos, EXPLOSION_KUTKU_FIREBALL_SIDE);
        }
        break;
    case EMRANGED_KUTKU_FIREBALL:
        if (atk->area == gScene.area) {
            Em_PlaySeChecked(atk->owner, 0x25, 0, pos, 1, 0);
        }
        if (atk->area == gScene.area) {
            AttackBurst_CreateAtScaled(pos, atk->rotY, 0x10, 1.0f);
            AttackBurst_CreateAtYaw(pos, 0xF, ((Em *) atk->owner)->type, atk->rotY, 1.2f);
        }
        Explosion_CreateOwned(atk->owner, pos, EXPLOSION_KUTKU_FIREBALL);
        break;
    case EMRANGED_KHEZU_CEILING_SHOT:
        if (atk->area == gScene.area) {
            Em_PlaySeChecked(atk->owner, 0x1D, 0, pos, 1, 0);
            ThunderSpark_CreateAt(pos, SPARK_KHEZU_SHOT_FLASH, ((Em *) atk->owner)->type, 1.0f);
            ThunderSpark_CreateAt(pos, SPARK_KHEZU_SHOT_RISE, ((Em *) atk->owner)->type, 1.0f);
        }
        break;
    case EMRANGED_CEPHADROME_SHOT:
        switch (how) {
        case 0:
            if (atk->area == gScene.area) {
                AttackBurst_CreateAt(pos, 0x14, ((Em *) atk->owner)->type, 1.0f);
            }
            Explosion_CreateOwned(atk->owner, pos, EXPLOSION_CEPHADROME_SHOT);
            break;
        case 1:
        case 2:
            if (atk->area == gScene.area) {
                AttackBurst_CreateAt(pos, 0x13, ((Em *) atk->owner)->type, 1.0f);
            }
            break;
        }
        break;
    case EMRANGED_PREY_SPIT:
    case EMRANGED_HERMITAUR_SHOT:
        if (atk->area == gScene.area) {
            ShotFlash_CreateAt(pos, 9, 1);
        }
        break;
    case EMRANGED_PREY_SPIT_B:
        if (atk->area == gScene.area) {
            ShotFlash_CreateAt(pos, 9, 7);
        }
        break;
    case EMRANGED_REMOBRA_SPIT:
        if (atk->area == gScene.area) {
            ShotFlash_CreateAtScaled(pos, 9, 1, EmRangedAttack_RemobraSpitScales[atk->stateVar1]);
        }
        break;
    case EMRANGED_GRAVIOS_SHOT:
        if (atk->area == gScene.area) {
            Em_PlaySeChecked(atk->owner, 0x25, 0, pos, 1, 0);
        }
        if (atk->area == gScene.area) {
            CamShake_SetA(2, pos);
            FireBlast_CreateScaledExact(atk->pos, 0, 1.5f);
            AttackBurst_CreateAtYaw(pos, 0xF, ((Em *) atk->owner)->type, atk->rotY, 1.8000001f);
        }
        Explosion_CreateOwned(atk->owner, pos, EXPLOSION_GRAVIOS_SHOT);
        break;
    case EMRANGED_BLANGONGA_LUMP:
        if (atk->area == gScene.area) {
            CamShake_SetA(2, pos);
            MonsterFx_CreateAt(pos, 5, 1.0f);
            Em_PlaySeChecked(atk->owner, 0x17, 0, pos, 15, 0);
        }
        EmRangedAttack_CreateFromAimed(atk->owner, 0x10, 0, atk, pos, 0xE000, atk->rotY);
        EmRangedAttack_CreateFromAimed(atk->owner, 0x10, 0, atk, pos, 0xE000, atk->rotY + 0x6000);
        EmRangedAttack_CreateFromAimed(atk->owner, 0x10, 0, atk, pos, 0xE000, atk->rotY + 0xA000);
        break;
    case EMRANGED_KUSHALA_BREATH:
        if (atk->area == gScene.area) {
            MonsterFx_CreateAtYaw(pos, atk->rotY, 0xE, 1.0f);
            if (how == 0) {
                Em_PlaySeChecked(atk->owner, 0x3D, 0, pos, 1, 0);
            }
        }
        break;
    case EMRANGED_CONGALALA_THROW:
        if (atk->area == gScene.area) {
            Em_PlaySeChecked(atk->owner, 0x1C, 0, pos, 1, 0);
            MonsterFx_CreateAtYaw(pos, atk->rotY, 0x1E, 2.0f);
        }
        break;
    }
}

/*
 * Places a stream or beam at its owner's bone (EmRangedAttackBuf.bone) plus
 * the type's bone offset, sets pos2 to the same point, and turns it with the
 * bone: the heading of the bone's z axis plus yawOfs; Plesioth's variant 1
 * stream takes its pitch from the bone instead, plus pitchOfs.
 */
void EmRangedAttack_AttachToBone(ObjAttach *atk, u8 *buf)
{
    f32 ofs[3];
    f32 boneMtx[16];
    f32 z;
    f32 x;

    Mtx_Copy((f32 *) boneMtx, (f32 *) (Em_GetBone(atk->owner, *(s16 *) (buf + 0x30))));
    ofs[0] = EmRangedAttack_BoneOfs[atk->type * 3];
    ofs[1] = EmRangedAttack_BoneOfs[atk->type * 3 + 1];
    ofs[2] = EmRangedAttack_BoneOfs[atk->type * 3 + 2];
    Vec_RotateSelf(ofs, boneMtx);
    atk->pos[0] = boneMtx[12] + ofs[0];
    atk->pos[1] = boneMtx[13] + ofs[1];
    atk->pos[2] = boneMtx[14] + ofs[2];
    Vec_Copy(atk->pos2, atk->pos);
    if (atk->type == EMRANGED_PLESIOTH_STREAM && atk->stateVar1 == 1) {
        x = boneMtx[8];
        z = boneMtx[10];
        atk->rotX = (u16) (s32) (0.5f + (65536.0f * Math_Atan2(-boneMtx[9], Math_Sqrtf(x * x + z * z))) / 6.2831855f);
        atk->rotX += buf[1] << 8;
    } else {
        atk->rotY = (buf[0x3B] << 8) + (u16) (s32) (0.5f + (65536.0f * Math_Atan2(boneMtx[8], boneMtx[10])) / 6.2831855f);
    }
}

/*
 * Gives in out the point of the segment from pos2 (a stream's start) to pos
 * (its tip) at the height of EmRangedAttackBuf.floorY: the buffer is passed
 * as an ObjAttach, so its pos[1] is the floorY field. When the segment is
 * level, out is pos; when that height is above the start, out is the start.
 */
void EmRangedAttack_FloorCrossing(ObjAttach *atk, ObjAttach *bufView, f32 *out)
{
    f32 frac;
    f32 startX;
    f32 endX;
    f32 endZ;
    f32 dz;

    if (Math_Absf(atk->pos[1] - atk->pos2[1]) < 0.0001f) {
        out[0] = atk->pos[0];
        out[1] = atk->pos[1];
        out[2] = atk->pos[2];
    } else if (!(bufView->pos[1] <= atk->pos2[1])) {
        out[0] = atk->pos2[0];
        out[1] = atk->pos2[1];
        out[2] = atk->pos2[2];
    } else {
        f32 *start = atk->pos2;
        f32 startZ;

        frac = (bufView->pos[1] - start[1]) / (atk->pos[1] - start[1]);
        startX = start[0];
        endX = atk->pos[0];
        endZ = atk->pos[2];
        startZ = start[2];
        dz = frac * (endZ - startZ);
        out[0] = startX + frac * (endX - startX);
        out[1] = bufView->pos[1];
        out[2] = start[2] + dz;
    }
}

/*
 * Turns the matrix dst about its own z axis (row 2 is kept) so that its x-y
 * plane faces the camera seen from pos: a stream drawn along z shows its
 * broad side. Rows 0 and 1 are replaced unless the camera lies along the z
 * axis.
 */
void EmRangedAttack_Billboard(f32 *dst, void *pos)
{
    f32 toCam[4];
    f32 axisZ[4];
    f32 axisX[4];
    f32 axisY[4];

    Vec_Sub(toCam, &gCamPos, (f32 *) pos);
    Vec_Copy(axisZ, dst + 8);
    Vec_Cross(axisX, toCam, axisZ);
    Vec_Normalize(axisX);
    Vec_Cross(axisY, axisZ, axisX);
    if (!(Vec_Dot(axisY, axisY) <= 0.0001f)) {
        Vec_Cross(axisX, axisY, axisZ);
        Vec_Copy(dst, axisX);
        Vec_Copy(dst + 4, axisY);
    }
}

/*
 * Gives in out the colour at frame of a colour-key list (ColorKey rows ended
 * by frame -1): a key's colour at its frame, or each byte of 0xAARRGGBB
 * interpolated between the two keys around frame. out is left unchanged when
 * frame is outside the keys.
 */
void Eff_ColorKeyLerp(ColorKey *key, s32 curFrame, u32 *out)
{
    f32 frac;
    s32 keyFrame;
    s32 nextFrame;
    u32 keyColor;
    u32 nextColor;
    u8 alpha;
    u8 red;
    u8 green;
    u8 blue;

    for (;; key++) {
        keyFrame = key->frame;
        if (keyFrame == -1) {
            return;
        }
        if (curFrame == keyFrame) {
            *out = key->color;
            return;
        }
        nextFrame = key[1].frame;
        if (keyFrame < curFrame && curFrame < nextFrame) {
            frac = (f32) (curFrame - keyFrame) / (f32) (nextFrame - keyFrame);
            keyColor = key->color;
            red = keyColor >> 16;
            green = keyColor >> 8;
            blue = keyColor;
            alpha = keyColor >> 24;
            *out = (u8) (alpha + (u8) (frac * ((f32) (u8) (key[1].color >> 24) - (f32) alpha))) << 24
                 | (u8) (red + (u8) (frac * ((f32) (u8) (key[1].color >> 16) - (f32) red))) << 16
                 | (u8) (green + (u8) (frac * ((f32) (u8) (key[1].color >> 8) - (f32) green))) << 8
                 | (u8) (blue + (u8) (frac * ((f32) (u8) key[1].color - (f32) blue)));
            return;
        }
    }
}

/*
 * Restarts a part of Khezu's ground lightning (EMRANGED_KHEZU_GROUND_BOLT)
 * when its frames are over, with its sound: its frame goes back to 1 (part 4:
 * -1, a frame's wait), and parts 1, 3 and 4 take a new random turn.
 */
void EmRangedAttack_RestartBoltPart(ObjAttach *atk, EmRangedAttackPart *part)
{
    s8 partIdx;

    Em_PlaySeChecked(atk->owner, 0x37, 0, atk->pos, 5, 1);
    partIdx = part->idx;
    switch (partIdx) {
    case 0:
        part->frame = 1;
        break;
    case 1:
        part->frame = 1;
        part->rot[2] = Rand_Next(1);
        break;
    case 2:
        part->frame = 1;
        break;
    case 3:
        part->frame = 1;
        part->rot[2] = Rand_Next(1);
        break;
    case 4:
        part->frame = -1;
        part->rot[2] = Rand_Next(1);
        break;
    }
}

/*
 * Moves Kirin's lightning (EMRANGED_KIRIN_BOLTS) to its next strike. It
 * counts a strike off (EmRangedAttackBuf.stage) and picks at random the
 * mirror (stateVar0), the texture column (phase) and the bolt model
 * (boltModel). After the first strike it keeps the finished spot as the
 * bolt's draw position (EmRangedAttack_KeepStrikePos) with the strike's
 * sound, and stops there when none is left. Otherwise it restarts the attack
 * row and puts the object on the floor at the next spot of the variant's
 * EmRangedAttack_KirinStrikeSpots row, turned by rotY around the monster's
 * position at set up, with sparks above and on the ground (their kind
 * argument is the owner's monster kind, read through Player.weaponType, at
 * the same offset as Em.type).
 */
void EmRangedAttack_KirinNextStrike(ObjAttach *atk, u8 *buf)
{
    f32 ofs[3];
    f32 sparkPos[3];
    u8 randBits;
    f32 (*tbl)[2];

    buf[0]--;
    randBits = Rand_Next(1);
    atk->stateVar0 = randBits & 1;
    buf[2] = (randBits >> 1) % 3;
    buf[3] = (randBits >> 3) % 3;
    if (buf[1] != 0) {
        EmRangedAttack_KeepStrikePos((u8 *) atk, buf);
        if (gScene.area == atk->area) {
            Em_PlaySeChecked(atk->owner, 0x18, 0, atk->pos, 1, 0);
        }
        if (buf[0] == 0) {
            return;
        }
    }
    EmRangedAttack_StartAttackRow(atk, (s32) atk->owner);
    tbl = EmRangedAttack_KirinStrikeSpots[atk->stateVar1 % 6];
    ofs[0] = tbl[buf[1]][0];
    ofs[1] = 0.0f;
    ofs[2] = tbl[buf[1]][1];
    Vec_RotateY(ofs, (6.2831855f * atk->rotY) / 65536.0f);
    Vec_Add(atk->pos, (f32 *) (buf + 0xC), ofs);
    atk->pos[1] = AreaGrid2_GetFloorYFiltered(atk->pos);
    sparkPos[0] = atk->pos[0];
    sparkPos[1] = 1000.0f + atk->pos[1];
    sparkPos[2] = atk->pos[2];
    if (gScene.area == atk->area) {
        ThunderSpark_CreateAt(sparkPos, SPARK_KIRIN_BOLT_SKY, ((Player *) atk->owner)->weaponType, 1.0f);
        ThunderSpark_CreateAt(atk->pos, SPARK_KIRIN_BOLT_GROUND, ((Player *) atk->owner)->weaponType, 1.0f);
    }
    buf[1]++;
}

/*
 * Copies the object's position (atk, the ObjAttach read as bytes) to the
 * buffer's strikePos (buf): where Kirin's bolt is drawn.
 */
void EmRangedAttack_KeepStrikePos(u8 *atk, u8 *buf)
{
    Vec_Copy((f32 *) (buf + 0x18), (f32 *) (atk + 0x30));
}

/* The file's data. */
HitShape EmRangedAttack_HitShapes_0[2] = {
#include "game/0089C6F0.inc"
};
HitShape EmRangedAttack_HitShapes_1[2] = {
#include "game/0089C740.inc"
};
HitShape EmRangedAttack_HitShapes_2[2] = {
#include "game/0089C790.inc"
};
HitShape EmRangedAttack_HitShapes_3[3] = {
#include "game/0089C7E0.inc"
};
HitShape EmRangedAttack_HitShapes_4[2] = {
#include "game/0089C860.inc"
};
HitShape EmRangedAttack_HitShapes_5[2] = {
#include "game/0089C8B0.inc"
};
HitShape EmRangedAttack_HitShapes_6[2] = {
#include "game/0089C900.inc"
};
HitShape EmRangedAttack_HitShapes_7[2] = {
#include "game/0089C950.inc"
};
HitShape EmRangedAttack_HitShapes_8[2] = {
#include "game/0089C9A0.inc"
};
HitShape EmRangedAttack_HitShapes_9[2] = {
#include "game/0089C9F0.inc"
};
HitShape EmRangedAttack_HitShapes_10[2] = {
#include "game/0089CA40.inc"
};
HitShape EmRangedAttack_HitShapes_11[2] = {
#include "game/0089CA90.inc"
};
HitShape *EmRangedAttack_HitShapes[12] = {
    EmRangedAttack_HitShapes_0, EmRangedAttack_HitShapes_1, EmRangedAttack_HitShapes_2, EmRangedAttack_HitShapes_3,
    EmRangedAttack_HitShapes_4, EmRangedAttack_HitShapes_5, EmRangedAttack_HitShapes_6, EmRangedAttack_HitShapes_7,
    EmRangedAttack_HitShapes_8, EmRangedAttack_HitShapes_9, EmRangedAttack_HitShapes_10, EmRangedAttack_HitShapes_11,
};
u32 EmRangedAttack_Rows[300] = {
#include "game/0089CB10.inc"
};
void *EmRangedAttack_RowsPtr = (void *)EmRangedAttack_Rows;
u32 EmRangedAttack_Curves_0[20] = {
#include "game/0089CFD0.inc"
};
u32 EmRangedAttack_Curves_1[20] = {
#include "game/0089D020.inc"
};
u32 EmRangedAttack_Curves_2[14] = {
#include "game/0089D070.inc"
};
s16 EmRangedAttack_SpinX[4] = {
#include "game/0089D0A8.inc"
};
s16 EmRangedAttack_SpinY[8] = {
#include "game/0089D0B0.inc"
};
ColorKey EmRangedAttack_KeysP12_0[4] = {
#include "game/0089D0C0.inc"
};
ColorKey EmRangedAttack_KeysP12_1[4] = {
#include "game/0089D0E0.inc"
};
ColorKey EmRangedAttack_KeysP12_2[4] = {
#include "game/0089D100.inc"
};
ColorKey *EmRangedAttack_FireballPartKeys[4] = {
    EmRangedAttack_KeysP12_0, EmRangedAttack_KeysP12_1, EmRangedAttack_KeysP12_2, NULL,
};
u32 EmRangedAttack_Curves_3[16] = {
#include "game/0089D130.inc"
};
u32 EmRangedAttack_Curves_4[20] = {
#include "game/0089D170.inc"
};
ColorKey EmRangedAttack_KutkuSideShotKeys[4] = {
#include "game/0089D1C0.inc"
};
ColorKey EmRangedAttack_FireShotKeys[4] = {
#include "game/0089D1E0.inc"
};
u32 EmRangedAttack_Curves_5[16] = {
#include "game/0089D200.inc"
};
ColorKey EmRangedAttack_GraviosBeamStartKeys[4] = {
#include "game/0089D240.inc"
};
u32 EmRangedAttack_Curves_6[20] = {
#include "game/0089D260.inc"
};
u32 EmRangedAttack_Curves_7[20] = {
#include "game/0089D2B0.inc"
};
u32 EmRangedAttack_Curves_8[20] = {
#include "game/0089D300.inc"
};
ColorKey EmRangedAttack_BeamEndKeys[4] = {
#include "game/0089D350.inc"
};
u32 EmRangedAttack_Curves_9[20] = {
#include "game/0089D370.inc"
};
u32 EmRangedAttack_Curves_10[24] = {
#include "game/0089D3C0.inc"
};
u32 EmRangedAttack_Curves_11[20] = {
#include "game/0089D420.inc"
};
u32 EmRangedAttack_Curves_12[12] = {
#include "game/0089D470.inc"
};
u32 EmRangedAttack_Curves_13[12] = {
#include "game/0089D4A0.inc"
};
u32 EmRangedAttack_Curves_14[12] = {
#include "game/0089D4D0.inc"
};
ColorKey EmRangedAttack_ColorKeysByIdx_0[4] = {
#include "game/0089D500.inc"
};
ColorKey EmRangedAttack_ColorKeysByIdx_1[4] = {
#include "game/0089D520.inc"
};
ColorKey EmRangedAttack_ColorKeysByIdx_2[4] = {
#include "game/0089D540.inc"
};
ColorKey EmRangedAttack_ColorKeysByIdx_3[4] = {
#include "game/0089D560.inc"
};
ColorKey EmRangedAttack_ColorKeysByIdx_4[4] = {
#include "game/0089D580.inc"
};
ColorKey *EmRangedAttack_GroundBoltPartKeys[6] = {
    EmRangedAttack_ColorKeysByIdx_0, EmRangedAttack_ColorKeysByIdx_1, EmRangedAttack_ColorKeysByIdx_2, EmRangedAttack_ColorKeysByIdx_3,
    EmRangedAttack_ColorKeysByIdx_4, NULL,
};
s16 EmRangedAttack_GroundBoltPartFrames[6] = {
#include "game/0089D5B8.inc"
};
s16 EmRangedAttack_GroundBoltPartModels[6] = {
#include "game/0089D5C8.inc"
};
u32 EmRangedAttack_Curves_16[20] = {
#include "game/0089D5E0.inc"
};
ColorKey EmRangedAttack_KirinBoltKeys[6] = {
#include "game/0089D630.inc"
};
ColorKey EmRangedAttack_SpitKeys[4] = {
#include "game/0089D660.inc"
};
ColorKey EmRangedAttack_PreySpitBKeys[4] = {
#include "game/0089D680.inc"
};
ColorKey EmRangedAttack_KeysP12B_0[4] = {
#include "game/0089D6A0.inc"
};
ColorKey EmRangedAttack_KeysP12B_1[4] = {
#include "game/0089D6C0.inc"
};
ColorKey EmRangedAttack_KeysP12B_2[4] = {
#include "game/0089D6E0.inc"
};
ColorKey *EmRangedAttack_FatalisFireballPartKeys[4] = {
    EmRangedAttack_KeysP12B_0, EmRangedAttack_KeysP12B_1, EmRangedAttack_KeysP12B_2, NULL,
};
u32 EmRangedAttack_Curves_17[12] = {
#include "game/0089D710.inc"
};
ColorKey EmRangedAttack_RajangBeamStartKeys[4] = {
#include "game/0089D740.inc"
};
u32 EmRangedAttack_Curves[20] = {
    (u32)EmRangedAttack_Curves_0, (u32)EmRangedAttack_Curves_1, (u32)EmRangedAttack_Curves_2, (u32)EmRangedAttack_Curves_3,
    (u32)EmRangedAttack_Curves_4, (u32)EmRangedAttack_Curves_5, (u32)EmRangedAttack_Curves_6, (u32)EmRangedAttack_Curves_7,
    (u32)EmRangedAttack_Curves_8, (u32)EmRangedAttack_Curves_9, (u32)EmRangedAttack_Curves_10, (u32)EmRangedAttack_Curves_11,
    (u32)EmRangedAttack_Curves_12, (u32)EmRangedAttack_Curves_13, (u32)EmRangedAttack_Curves_14, (u32)EmRangedAttack_Curves_14,
    (u32)EmRangedAttack_Curves_16, (u32)EmRangedAttack_Curves_17, 0, 0,
};
s16 EmRangedAttack_PartCounts[32] = {
#include "game/0089D7B0.inc"
};
s16 EmRangedAttack_PartFrames[32] = {
#include "game/0089D7F0.inc"
};
s16 EmRangedAttack_Lives[32] = {
#include "game/0089D830.inc"
};
s16 EmRangedAttack_FirstCurve[32] = {
#include "game/0089D870.inc"
};
s16 EmRangedAttack_CurveSteps[32] = {
#include "game/0089D8B0.inc"
};
s16 EmRangedAttack_Models[32] = {
#include "game/0089D8F0.inc"
};
s16 EmRangedAttack_RowBase[32] = {
#include "game/0089D930.inc"
};
f32 EmRangedAttack_Speeds[28] = {
#include "game/0089D970.inc"
};
f32 EmRangedAttack_Gravities[28] = {
#include "game/0089D9E0.inc"
};
f32 EmRangedAttack_BoneOfs[76] = {
#include "game/0089DA50.inc"
};
s16 EmRangedAttack_KirinStrikeCounts[8] = {
#include "game/0089DB80.inc"
};
u32 EmRangedAttack_KirinStrikeSpots_0[8] = {
#include "game/0089DB90.inc"
};
u32 EmRangedAttack_KirinStrikeSpots_1[12] = {
#include "game/0089DBB0.inc"
};
u32 EmRangedAttack_KirinStrikeSpots_2[16] = {
#include "game/0089DBE0.inc"
};
u32 EmRangedAttack_KirinStrikeSpots_3[12] = {
#include "game/0089DC20.inc"
};
u32 EmRangedAttack_KirinStrikeSpots_4[16] = {
#include "game/0089DC50.inc"
};
u32 EmRangedAttack_KirinStrikeSpots_5[12] = {
#include "game/0089DC90.inc"
};
f32 (*EmRangedAttack_KirinStrikeSpots[6])[2] = {
    (void *)EmRangedAttack_KirinStrikeSpots_0, (void *)EmRangedAttack_KirinStrikeSpots_1, (void *)EmRangedAttack_KirinStrikeSpots_2, (void *)EmRangedAttack_KirinStrikeSpots_3,
    (void *)EmRangedAttack_KirinStrikeSpots_4, (void *)EmRangedAttack_KirinStrikeSpots_5,
};
s16 EmRangedAttack_KirinBoltModels[4] = {
#include "game/0089DCD8.inc"
};
f32 EmRangedAttack_FatalisSweepSpots[72] = {
#include "game/0089DCE0.inc"
};
f32 EmRangedAttack_RemobraSpitScales[4] = {
#include "game/0089DE00.inc"
};
