/*
 * Attack-object families: KushalaBreath (Kushala Daora's breath, which
 * leaves an Explosion by its size),
 * KushalaAttack, ShenGaorenAttack and YamaTsukamiAttack (those monsters'
 * body attacks).
 * Each family is a kind of ObjAttach (enum
 * ObjAttachKind) on sub_main's object list: its Create fills the object and
 * its Update, which ObjAttach_UpdateAll calls every frame, switches on
 * ObjAttach.state (enum ObjAttachState) to its Setup, Run, Hit, End and Free;
 * its type picks a row of its tables (the hit's power, kind, element and
 * shapes).
 */
#include "common.h"
#include "em.h"
#include "em_kushala.h"
#include "objattach.h"
#include "player.h"
#include "scene.h"
#include "stageobj.h"
#include "sprite.h"
#include "capsule.h"
#include "area.h"
#include "render_state.h"
#include "camera.h"
#include "card.h"
#include "quest.h"
#include "sound.h"
#include "elder_attacks.h"
#include "draw_slots.h"
#include "dust_flash_effects.h"
#include "engine_math.h"
#include "hit_effects.h"
#include "hit_setup.h"
#include "monster_effects.h"
#include "rand.h"
#include "shakalaka_teostra_bulldrome.h"
#include "vec_math.h"
/* Declarations of the functions called below. */

/* prototypes */
void EmEffectAttack_RandLinkSpin(struct EmEffectAttackLink *e);
extern s16 EmEffectAttack_CongalalaBreathDelays[];
extern u8 KushalaBreath_Offsets[];
extern f32 KushalaBreath_TrailOfs[];
extern s8 KushalaBreath_FirstRow[];
extern void *KushalaBreath_RowsPtr;
extern HitShape *KushalaBreath_HitShapes[];
extern f32 gCamMatrix[];
extern HitShape *EmEffectAttack_HitShapes[];
extern f32 EmEffectAttack_FatalisSweepSpots[][3];
extern f32 EmEffectAttack_FatalisBlastSpots[][3];
extern f32 EmEffectAttack_ShenDropSpotsFortress5[][3];
extern f32 EmEffectAttack_ShenDropSpotsTown2[][3];
extern f32 EmEffectAttack_ShenDropSpotsTown3[][3];
extern f32 EmEffectAttack_ShenDropSpotsTown0[][3];
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
extern void *KushalaBreath_AlphaKeys[];
extern void *KushalaBreath_ScaleKeys[];
extern f32 KushalaBreath_SizeScale[];
extern f32 KushalaBreath_LaunchOfs[];
EmBone *Em_Bone(void *obj, s16 bone);
extern f32 EmEffectAttack_BoneOfsY[];
extern f32 EmEffectAttack_BoneOfsZ[];
/* unit's own declaration: the kind declared u8, as KushalaBreath_Impact passes it; with the definition's s32 the call compiles differently */
void Explosion_CreateOwned(Player *owner, f32 *pos, u8 kind);
/*
 * One part of an EmEffectAttack (60 bytes): the parts follow the
 * buffer's 12-byte header.
 */
typedef struct EmEffectAttackPart {
    s16 slot;       /* 0x00 */
    u8 type;        /* 0x02 */
    u8 curveXY;     /* 0x03: 1, the scale keys are read as x and y (z = 1) */
    f32 vec[3];     /* 0x04 */
    u32 color;      /* 0x10 */
    f32 size[3];    /* 0x14 */
    f32 scale;      /* 0x20 */
    u8 *draw; /* 0x24 */
    f32 speed;      /* 0x28: the drops' fall speed, the puffs' outward speed */
    s16 frame;      /* 0x2C: the key time; negative is a start delay */
    u16 rotX;       /* 0x2E */
    u16 rotY;       /* 0x30 */
    u16 rotZ;       /* 0x32 */
    s16 spinX;      /* 0x34: added to rotX each frame */
    s16 spinY;      /* 0x36 */
    s16 spinZ;      /* 0x38 */
    u8 sorted;      /* 0x3A: 1, drawn sorted (gDrawOtSorted); 0, added (gDrawOt32) */
    u8 aux;         /* 0x3B: by type: the texture's second cell, or the fade frames */
} EmEffectAttackPart;
void EmEffectAttack_StartCongalalaGasPuff(ObjAttach *o, EmEffectAttackPart *p);
u16 Area_IdForCurrentTime(u16);
extern s16 KushalaBreath_PartCounts[];
extern HitShape *KushalaAttack_HitShapes[];
extern void *KushalaAttack_RowsPtr;
extern HitShape *ShenGaorenAttack_HitShapes[];
extern void *ShenGaorenAttack_RowsPtr;
extern HitShape *YamaTsukamiAttack_HitShapes[];
extern void *YamaTsukamiAttack_RowsPtr;
void *EmEffectAttack_Create(Em *em, s32 kind, s32 c, s32 a, s32 b);
void EmEffectAttack_UpdateLink(ObjAttach *o, struct EmEffectAttackLink *e, struct EmEffectAttackLink *prev, void *bone, f32 *dir, s16 f);
/* end of prototypes */

/*
 * Makes a KushalaBreath of type at bone of em, if a player is in em's area.
 * Types 1 and 2 start KushalaBreath_LaunchOfs ahead of the bone; the pitch is
 * pitch and the yaw is em's plus yawOfs. size picks the scale
 * (KushalaBreath_SizeScale) and the burst (KushalaBreath_Impact):
 * EmKushala_CreateBreathAtBone adds 3 (types 0, 2) or 1 (type 1) on the
 * snowy mountains. The bone is kept in buf->spin until set up.
 */
void KushalaBreath_CreateAtBone(Em *em, s32 bone, u8 attackType, u8 sizeNo, u16 pitch, u16 yawOfs)
{
    ObjAttach *o;
    f32 ofs[3];
    KushalaBreathBuf *breathBuf;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->kind = ATTACH_KUSHALA_BREATH;
            o->type = attackType;
            o->state = ATTACH_SETUP;
            o->step = 0;
            o->stateVar0 = 0;
            o->stateVar1 = 0;
            o->idx = em->index;
            o->update = KushalaBreath_Update;
            o->isEm = em->isEm;
            Em_GetBonePos(em, bone, o->pos);
            o->aux = em->type;
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = pitch;
            o->rotY = yawOfs + em->rotY;
            o->rotZ = 0;
            ((s32 *) &o->velX)[0] = 0;
            ((s32 *) &o->velX)[1] = 0;
            ((s32 *) &o->velX)[2] = 0;
            switch (o->type) {
            case 1:
            case 2:
                Vec_Copy(ofs, KushalaBreath_LaunchOfs);
                Vec_Rotate((f32 *) ofs, (f32 *) ofs, (f32 *) (Em_GetBone(em, bone)));
                o->pos[0] += ofs[0];
                o->pos[1] += ofs[1];
                o->pos[2] += ofs[2];
                break;
            }
            o->area = em->area;
            o->side = *(u8 *) &em->side;
            breathBuf = (KushalaBreathBuf *) o->buf;
            breathBuf->size = sizeNo;
            breathBuf->spin = bone;
        }
    }
}

/*
 * Makes a child of the breath src with type and row offset rowOfs (stateVar1,
 * added to the type's attack row). The child is put at src's position with
 * src's velocity, angles, owner and hitId, so a target already hit is not hit
 * again. KushalaBreath_Run uses it for the trail (type 3; rowOfs 1 behind the
 * large sizes 2 and 5). Only in the current area.
 */
void KushalaBreath_SpawnTrail(ObjAttach *src, u8 attackType, u8 rowOfs)
{
    ObjAttach *o;

    if (src->area == gScene.area) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_KUSHALA_BREATH;
            o->type = attackType;
            o->state = ATTACH_SETUP;
            o->step = 0;
            o->stateVar0 = 0;
            o->stateVar1 = rowOfs;
            o->idx = src->idx;
            o->update = KushalaBreath_Update;
            o->isEm = src->isEm;
            o->aux = src->aux;
            o->owner = src->owner;
            o->yaw = src->rotY;
            o->rotX = src->rotX;
            o->rotY = src->rotY;
            o->rotZ = src->rotZ;
            o->area = src->area;
            o->side = src->side;
            Vec_Copy(o->pos, src->pos);
            Vec_Copy(&o->velX, &src->velX);
            o->hitId = src->hitId;
        }
    }
}

/*
 * The breath's per-frame update, by ObjAttach.state: a hit (ATTACH_HIT) keeps
 * it running; states 4, 5, 7 and 9 end it.
 */
void KushalaBreath_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        KushalaBreath_Setup(o);
        break;
    case ATTACH_RUN:
        KushalaBreath_Run(o);
        break;
    case ATTACH_END:
        KushalaBreath_End(o);
        break;
    case ATTACH_HIT:
        KushalaBreath_Run(o);
        break;
    case 4:
        KushalaBreath_End(o);
        break;
    case ATTACH_DEFLECTED:
        KushalaBreath_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        KushalaBreath_End(o);
        break;
    case 7:
        KushalaBreath_End(o);
        break;
    case ATTACH_FREE:
        KushalaBreath_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the breath up. Types 0-2 take a draw slot for the body, their
 * particles (KushalaBreath_PartCounts) and attack row
 * KushalaBreath_FirstRow[type] + size. Type 0 starts at the skim height above
 * the floor and flies at 100 along its yaw; type 1 flies at 80 along its
 * pitch and yaw; type 2 flies at 100 and remembers its height and pitch for
 * the dive. Type 3 (the trail) takes row + stateVar1, moves by
 * KushalaBreath_TrailOfs and keeps its parent's hitId. It then runs its first
 * frame.
 */
void KushalaBreath_Setup(ObjAttach *o)
{
    f32 trailOfs[3];
    KushalaBreathPart *particle;
    Em *owner;
    s16 i;
    KushalaBreathBuf *breathBuf;
    u8 savedHitId;
    s16 row;
    s16 count;

    owner = o->owner;
    row = 0;
    o->state = ATTACH_RUN;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    case 0:
    case 1:
    case 2:
        breathBuf = (KushalaBreathBuf *) o->buf;
        breathBuf->spin = 0;
        particle = breathBuf->part;
        row += breathBuf->size;
        o->drawSlot = DrawSlot_Alloc();
        if (o->drawSlot != -1) {
            o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
            o->drawSlotBuf->owner = o;
            o->drawSlotBuf->part = -1;
            o->drawSlotBuf->draw = KushalaBreath_Draw;
        } else {
            o->drawSlotBuf = NULL;
        }
        break;
    case 3:
        o->drawSlotBuf = NULL;
        row += o->stateVar1;
        break;
    }
    switch (o->type) {
    case 0:
        o->pos[1] = AreaGrid2_GetFloorY(o->pos);
        o->pos[1] += *(f32 *) (KushalaBreath_Offsets + 4) * KushalaBreath_SizeScale[breathBuf->size];
        o->velZ = 100.0f;
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        break;
    case 1:
        o->velZ = 80.0f;
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        break;
    case 2:
        breathBuf->height = o->pos[1] - AreaGrid2_GetFloorY(o->pos);
        breathBuf->ang = o->rotX;
        o->rotX = (u16) (s32) (0.5f + (65536.0f * ((360.0f * breathBuf->ang) / 65536.0f)) / 360.0f);
        o->velZ = 100.0f;
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        break;
    case 3:
        Vec_Copy(trailOfs, KushalaBreath_TrailOfs);
        Vec_RotateY(trailOfs, (6.2831855f * o->rotY) / 65536.0f);
        o->pos[0] += trailOfs[0];
        o->pos[1] += trailOfs[1];
        o->pos[2] += trailOfs[2];
        savedHitId = o->hitId;
        break;
    }
    Vec_Copy(o->pos2, o->pos);
    o->ownerMotion = 0;
    count = KushalaBreath_PartCounts[o->type];
    for (i = 0; i < count; i++, particle++) {
        particle->slot = DrawSlot_Alloc();
        if (particle->slot != -1) {
            particle->ds = DrawSlot_Get(particle->slot);
            particle->ds->owner = o;
            particle->ds->part = i;
            particle->ds->draw = KushalaBreath_Draw;
        } else {
            particle->ds = NULL;
        }
        particle->rot[0] = 0;
        particle->rot[1] = 0;
        particle->rot[2] = 0;
        particle->vel[0] = 0.0f;
        particle->vel[1] = 0.0f;
        switch (o->type) {
        case 0:
            particle->active = 1;
            break;
        case 2:
        default:
            particle->active = 0;
            break;
        }
        if (i <= 2) {
            particle->kind = 1;
            particle->timer = -i * 5 - 15;
            particle->vel[2] = 100.0f + (-500.0f + *(f32 *) (KushalaBreath_Offsets + (i + 1) * 12 + 8)) / 10.0f;
            Vec_RotateY(particle->vel, (6.2831855f * o->rotY) / 65536.0f);
        } else if (i <= 4) {
            particle->kind = 0;
            particle->timer = -(i - 3) * 4 - 5;
            particle->vel[2] = 0.0f;
        } else {
            particle->kind = 2;
            particle->timer = -(i - 5) * 3;
            particle->vel[2] = 100.0f + (-300.0f + -200.0f * Rand_Unit()) / 6.0f;
            Vec_RotateY(particle->vel, (6.2831855f * o->rotY) / 65536.0f);
        }
    }
    ObjAttach_StartOwnedAttack(o, (s32) owner, KushalaBreath_FirstRow[o->type] + row, &KushalaBreath_RowsPtr);
    o->hitList = KushalaBreath_HitShapes[o->hitListNo];
    o->hurtList = 0;
    switch (o->type) {
    case 3:
        o->hitId = savedHitId;
        break;
    }
    KushalaBreath_Run(o);
}

extern s16 KushalaBreath_Life[];
extern f32 KushalaBreath_SkimHeight[];
extern f32 KushalaBreath_SprayOfs[];
extern f32 KushalaBreath_SprayJitter[];
extern f32 KushalaBreath_DebrisOfs[];
extern f32 KushalaBreath_DebrisJitter[];
void ObjAttach_Move(ObjAttach *);

/* KushalaBreath's frame (state 1): moves it; past its lifetime (KushalaBreath_Life[type] frames) ends it
 * (KushalaBreath_Impact at the floor for types 0-2); else types 0 and 2 spawn type 3 children and play
 * sound 62, types 0-2 follow the floor (ending when they leave it), and each live particle is
 * reset at its timer's 0 (position, spin, velocity by kind), moved and drawn. */
void KushalaBreath_Run(ObjAttach *o)
{
    f32 partOfs[3];
    f32 jitter[3];
    f32 burstPos[3];
    KushalaBreathBuf *breathBuf;
    KushalaBreathPart *particle;
    s16 count;
    f32 floor;
    f32 skimHeight;
    f32 y;
    s16 life;
    s16 i;
    s32 pitch;

    breathBuf = (KushalaBreathBuf *) o->buf;
    life = KushalaBreath_Life[o->type];
    particle = breathBuf->part;
    count = KushalaBreath_PartCounts[o->type];
    o->hitFrames = 99;
    Vec_Copy(o->pos2, o->pos);
    ObjAttach_Move(o);
    if (life < ++o->ownerMotion) {
        switch (o->type) {
        case 0:
        case 1:
        case 2:
            if (gScene.area != o->area) {
                break;
            }
            floor = AreaGrid2_GetFloorY(o->pos);
            Vec_Copy(burstPos, o->pos);
            switch (o->type) {
            case 0:
            case 2:
                skimHeight = KushalaBreath_SkimHeight[0] * KushalaBreath_SizeScale[breathBuf->size];
                if (Math_Absf(o->pos[1] - (floor + skimHeight)) <= 50.0f) {
                    burstPos[1] = floor;
                } else {
                    burstPos[1] -= skimHeight;
                }
                break;
            case 1:
                if (o->pos[1] <= floor && floor - o->pos[1] <= 100.0f) {
                    burstPos[1] = floor;
                }
                break;
            }
            KushalaBreath_Impact(o, 0, burstPos);
            break;
        }
        o->hitFrames = 0;
        o->hitPhase = 0;
        o->state = ATTACH_END;
        return;
    }
    switch (o->type) {
    case 0:
        if (o->ownerMotion > 5 && (o->ownerMotion - 5) % 4 == 0) {
            KushalaBreath_SpawnTrail(o, 3, 0);
        }
        if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
            Em_PlaySeChecked(o->owner, 62, 0, o->pos, 24, 0);
        }
        goto run;
    case 2:
        switch (o->stateVar0) {
        case 1:
            if (o->ownerMotion % 4 == 0) {
                switch (breathBuf->size) {
                case 2:
                case 5:
                    KushalaBreath_SpawnTrail(o, 3, 1);
                    break;
                default:
                    KushalaBreath_SpawnTrail(o, 3, 0);
                    break;
                }
            }
            if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
                Em_PlaySeChecked(o->owner, 62, 0, o->pos, 24, 0);
            }
        }
    default:
    run:
        breathBuf->spin += 1820;
        floor = AreaGrid2_GetFloorY(o->pos);
        Key_LerpVec3(KushalaBreath_ScaleKeys[0], breathBuf->vec, o->ownerMotion % 4);
        breathBuf->vec[0] *= KushalaBreath_SizeScale[breathBuf->size];
        breathBuf->vec[1] *= KushalaBreath_SizeScale[breathBuf->size];
        breathBuf->vec[2] *= KushalaBreath_SizeScale[breathBuf->size];
        switch (o->type) {
        case 0:
            skimHeight = KushalaBreath_SkimHeight[0] * KushalaBreath_SizeScale[breathBuf->size];
            y = floor + skimHeight;
            if (!(Math_Absf(o->pos[1] - y) <= 50.0f)) {
                burstPos[0] = o->pos[0];
                burstPos[1] = o->pos[1] - skimHeight;
                burstPos[2] = o->pos[2];
                KushalaBreath_Impact(o, 0, burstPos);
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = ATTACH_END;
                return;
            }
            o->pos[1] = y;
            break;
        case 1:
            if (o->pos[1] <= floor) {
                if (!(floor - o->pos[1] <= 100.0f)) {
                    KushalaBreath_Impact(o, 0, o->pos);
                } else {
                    burstPos[0] = o->pos[0];
                    burstPos[1] = floor;
                    burstPos[2] = o->pos[2];
                    KushalaBreath_Impact(o, 0, burstPos);
                }
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = ATTACH_END;
                return;
            }
            break;
        case 2:
            skimHeight = KushalaBreath_SkimHeight[0] * KushalaBreath_SizeScale[breathBuf->size];
            switch (o->step) {
            case 0:
                y = floor + skimHeight;
                if (o->pos[1] - y < 0.0f) {
                    burstPos[0] = o->pos[0];
                    burstPos[1] = o->pos[1] - skimHeight;
                    burstPos[2] = o->pos[2];
                    KushalaBreath_Impact(o, 0, burstPos);
                    o->hitFrames = 0;
                    o->hitPhase = 0;
                    o->state = ATTACH_END;
                    return;
                }
                pitch = (u16) (u32) ((f32) (u32) breathBuf->ang * (o->pos[1] - y - skimHeight) / (breathBuf->height - skimHeight));
                if (pitch <= 0x200) {
                    o->rotX = 0;
                    o->pos[1] = y;
                    o->step++;
                    for (i = 0; i < count; i++) {
                        particle->active = 1;
                        particle++;
                    }
                    o->stateVar0++;
                } else if (pitch <= o->rotX) {
                    o->rotX = pitch;
                }
                o->velX = 0.0f;
                o->velY = 0.0f;
                o->velZ = 100.0f;
                Vec_RotateX(&o->velX, 6.2831855f * o->rotX / 65536.0f);
                Vec_RotateY(&o->velX, 6.2831855f * o->rotY / 65536.0f);
                break;
            case 1:
                y = floor + skimHeight;
                if (!(Math_Absf(o->pos[1] - y) <= 50.0f)) {
                    burstPos[0] = o->pos[0];
                    burstPos[1] = o->pos[1] - skimHeight;
                    burstPos[2] = o->pos[2];
                    KushalaBreath_Impact(o, 0, burstPos);
                    o->hitFrames = 0;
                    o->hitPhase = 0;
                    o->state = ATTACH_END;
                    return;
                }
                o->pos[1] = y;
                break;
            }
            break;
        }
        if (o->drawSlotBuf != NULL) {
            Vec_Copy(o->drawSlotBuf->pos, o->pos);
            OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 64, 0);
        }
        particle = breathBuf->part;
        for (i = 0; i < count; i++, particle++) {
            if (particle->active == 0) {
                continue;
            }
            if (particle->timer == 0) {
                Vec_Copy(particle->pos, o->pos);
                particle->rot[2] = Rand_Next(1);
                switch (particle->kind) {
                case 0:
                    particle->rot[2] = Rand_Next(1);
                    particle->flags = Rand_Next(1) & 4;
                    Vec_Copy(jitter, KushalaBreath_SprayJitter);
                    jitter[0] *= Rand_Signed();
                    jitter[1] *= Rand_Signed();
                    jitter[2] *= Rand_Signed();
                    Vec_Copy(partOfs, KushalaBreath_SprayOfs);
                    Vec_Add(partOfs, partOfs, jitter);
                    break;
                case 1:
                    particle->rot[2] = Rand_Next(1);
                    particle->flags = Rand_Next(1) & 6;
                    Vec_Copy(partOfs, (f32 *) (KushalaBreath_Offsets + (i + 1) * 12));
                    break;
                case 2:
                    particle->rot[0] = (s16) (Rand_Next(1) % 910) - 0x38E;
                    particle->rot[1] = (s16) (Rand_Next(1) % 1820) - 0x71C;
                    particle->rot[2] = (s16) (Rand_Next(1) % 1820) - 0x71C;
                    Vec_Copy(jitter, KushalaBreath_DebrisJitter);
                    jitter[0] *= Rand_Signed();
                    jitter[1] *= Rand_Signed();
                    jitter[2] *= Rand_Signed();
                    Vec_Copy(partOfs, KushalaBreath_DebrisOfs);
                    Vec_Add(partOfs, partOfs, jitter);
                    break;
                }
                partOfs[0] *= KushalaBreath_SizeScale[breathBuf->size];
                partOfs[1] *= KushalaBreath_SizeScale[breathBuf->size];
                partOfs[2] *= KushalaBreath_SizeScale[breathBuf->size];
                Vec_RotateY(partOfs, 6.2831855f * o->rotY / 65536.0f);
                Vec_Add(particle->pos, particle->pos, partOfs);
                switch (particle->kind) {
                case 1:
                    particle->pos[1] = AreaGrid2_GetFloorY(particle->pos);
                }
            } else if (particle->timer > 0) {
                switch (particle->kind) {
                case 1:
                    if (particle->timer < 11) {
                        particle->pos[0] += particle->vel[0];
                        particle->pos[1] += particle->vel[1];
                        particle->pos[2] += particle->vel[2];
                        KushalaBreath_SpinPart(o, particle, 1092);
                    }
                    break;
                case 0:
                    if (particle->timer < 9) {
                        KushalaBreath_SpinPart(o, particle, 1092);
                    }
                    break;
                case 2:
                    if (particle->timer < 7) {
                        particle->pos[0] += particle->vel[0];
                        particle->pos[1] += particle->vel[1];
                        particle->pos[2] += particle->vel[2];
                    }
                    break;
                }
            }
            if (particle->timer >= 0) {
                KushalaBreath_DrawPart(o, particle);
            }
            particle->timer++;
            switch (particle->kind) {
            case 1:
                if (particle->timer >= 10) {
                    if (life < o->ownerMotion + 10) {
                        particle->active = 0;
                    } else {
                        particle->timer = 0;
                    }
                }
                break;
            case 0:
                if (particle->timer >= 8) {
                    if (life < o->ownerMotion + 8) {
                        particle->active = 0;
                    } else {
                        particle->timer = 0;
                    }
                }
                break;
            case 2:
                if (particle->timer >= 6) {
                    if (life < o->ownerMotion + 6) {
                        particle->active = 0;
                    } else {
                        particle->timer = 0;
                    }
                }
                break;
            }
        }
    case 3:
        break;
    }
}

/*
 * Turns particle p's z angle by step: forward when its flags have 4, else
 * backward. o is unused.
 */
void KushalaBreath_SpinPart(ObjAttach *o, KushalaBreathPart *part, u16 step)
{
    if ((part->flags & 4) == 4) {
        part->rot[2] += (s32) step;
    } else {
        part->rot[2] = part->rot[2] - step;
    }
}

/*
 * Sets particle p's alpha and scale from its kind's keys at its timer (times
 * the size's scale) and queues its draw slot at its position (types 0 and 2).
 */
void KushalaBreath_DrawPart(ObjAttach *o, KushalaBreathPart *particle)
{
    KushalaBreathBuf *breathBuf = (KushalaBreathBuf *) o->buf;
    u16 keyNo;
    s16 partNo = *(s16 *) &particle->ds->part;
    f32 alphaVal;
    void *scaleKeys;

    switch (particle->kind) {
    case 1:
        keyNo = 0;
        break;
    case 0:
        keyNo = 1;
        break;
    case 2:
        keyNo = 2;
        break;
    }
    Key_LerpFloat(KushalaBreath_AlphaKeys[keyNo], &alphaVal, particle->timer);
    particle->alpha = 255.0f * alphaVal;
    switch (particle->kind) {
    case 1:
        keyNo = partNo + 1;
        break;
    case 0:
        keyNo = 4;
        break;
    case 2:
        keyNo = 5;
        break;
    }
    scaleKeys = KushalaBreath_ScaleKeys[keyNo];
    switch (particle->kind) {
    case 1:
        Key_LerpVec2(scaleKeys, particle->scale, particle->timer);
        break;
    case 0:
    case 2:
        Key_LerpVec3(scaleKeys, particle->scale, particle->timer);
        break;
    }
    (particle->scale)[0] *= KushalaBreath_SizeScale[breathBuf->size];
    (particle->scale)[1] *= KushalaBreath_SizeScale[breathBuf->size];
    (particle->scale)[2] *= KushalaBreath_SizeScale[breathBuf->size];
    switch (o->type) {
    case 0:
    case 2:
        if (particle->ds != NULL) {
            Vec_Copy(particle->ds->pos, particle->pos);
            OT_AddByDepth(gDrawOtSorted, particle->ds, 64U, 0);
        }
        break;
    case 1:
        break;
    }
}

/*
 * Ends the breath: closes its hit window, hides it and frees its body and
 * particle draw slots. It is freed on the next update.
 */
void KushalaBreath_End(ObjAttach *o)
{
    KushalaBreathPart *particle = ((KushalaBreathBuf *) o->buf)->part;
    s16 count;
    s16 i;

    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free(o->drawSlot);
    }
    count = KushalaBreath_PartCounts[o->type];
    for (i = 0; i < count; i++) {
        if (particle->ds != NULL) {
            DrawSlot_Free(particle->slot);
        }
        particle++;
    }
}

/* Frees the breath (ObjAttach_Free). */
void KushalaBreath_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Draws drawSlot's part of the breath with the owner kind's part models
 * (EmSlot_FindKind(o->aux)), in the current area only. The body (part -1) is
 * scaled by buf->vec, spun about z, pitched (types 1, 2) and turned by the
 * yaw, with its texture scrolling. A particle uses its kind's model: floor
 * puffs face the camera; spray and debris turn with the yaw, their texture
 * row set by the map (desert or snow, swamp or jungle). Particles take the
 * area's effect colour and their alpha.
 */
void KushalaBreath_Draw(DrawSlot *drawSlot)
{
    f32 mtx[16];
    f32 uvMtx[16];
    u8 red;
    u8 green;
    u8 blue;
    u8 alphaVal;
    ObjAttach *o = drawSlot->owner;
    KushalaBreathBuf *breathBuf;
    KushalaBreathPart *particle;
    ModelMat *tex;
    Model *bank;
    u16 flags;
    u32 col;
    s16 matSetNo;
    ModelMatSet *matSet;
    s16 slot;

    breathBuf = (KushalaBreathBuf *) o->buf;
    flags = 0;

    if (o->area != gScene.area) {
        return;
    }
    slot = EmSlot_FindKind(o->aux);
    if (slot == -1) {
        return;
    }
    bank = gScene.partModels[slot];
    if (bank == NULL) {
        return;
    }
    if (bank->active == 0) {
        return;
    }
    tex = bank->mats;
    Gs_SetState(RS_ZWRITE, 0);
    if (drawSlot->part == -1) {
        matSetNo = 0;
        Mtx_Scale(mtx, breathBuf->vec[0], breathBuf->vec[1], breathBuf->vec[2]);
        Mtx_RotateZ(mtx, (6.2831855f * (u16) breathBuf->spin) / 65536.0f);
        switch (o->type) {
        case 1:
        case 2:
            Mtx_RotateX(mtx, (6.2831855f * o->rotX) / 65536.0f);
            break;
        }
        Mtx_RotateY(mtx, (6.2831855f * o->rotY) / 65536.0f);
        Mtx_Translation(uvMtx, 0.0f, 0.0625f * (o->ownerMotion % 16), 0.0f);
        Gs_SetState(RS_UV_MTX, (s32) uvMtx);
        flags |= 2;
        col = -1;
    } else {
        Effect_GetColor(gScene.area, &red, &green, &blue, &alphaVal);
        particle = &breathBuf->part[drawSlot->part];
        switch (particle->kind) {
        case 1:
            if ((particle->flags & 2) == 2) {
                matSetNo = 1;
            } else {
                matSetNo = 2;
            }
            break;
        case 0:
            matSetNo = 5;
            break;
        case 2:
            matSetNo = 3;
            break;
        }
        Mtx_Scale(mtx, particle->scale[0], particle->scale[1], particle->scale[2]);
        switch (particle->kind) {
        case 1:
        case 0:
            Mtx_RotateZ(mtx, (6.2831855f * particle->rot[2]) / 65536.0f);
            break;
        case 2:
            Mtx_RotateXYZ(mtx, (6.2831855f * particle->rot[0]) / 65536.0f, (6.2831855f * particle->rot[1]) / 65536.0f,
                          (6.2831855f * particle->rot[2]) / 65536.0f);
            break;
        }
        switch (particle->kind) {
        case 0:
        case 2:
            Mtx_RotateY(mtx, (6.2831855f * o->rotY) / 65536.0f);
            switch (Map_DayId(gScene.map)) {
            case MAP_DESERT:
            case MAP_SNOWY_MOUNTAINS:
                Mtx_Translation(uvMtx, 0.0f, 0.25f, 0.0f);
                break;
            case MAP_SWAMP:
            case MAP_JUNGLE:
                Mtx_Translation(uvMtx, 0.0f, 0.5f, 0.0f);
                break;
            default:
                Mtx_Translation(uvMtx, 0.0f, 0.0f, 0.0f);
                break;
            }
            Gs_SetState(RS_UV_MTX, (s32) uvMtx);
            break;
        case 1:
            Mtx_Concat3x3(mtx, gCamMatrix);
            break;
        }
        col = red << 16 | (alphaVal & particle->alpha) << 24 | green << 8 | blue;
    }
    matSet = &bank->matSets[matSetNo];
    Mtx_SetTrans(mtx, drawSlot->pos[0], drawSlot->pos[1], drawSlot->pos[2]);
    Effect_DrawModel(matSet, mtx, col, flags, tex);
}

/*
 * Bursts the breath at pos (types 0-2 only): its burst sound (60), a
 * MonsterFx effect (MonsterFx_CreateAtYaw, kind 15; 17 for type 1) and an
 * Explosion owned by the monster. The Explosion's kind follows the size:
 * EXPLOSION_KUSHALA_BREATH small, EXPLOSION_KUSHALA_BREATH_BIG large,
 * EXPLOSION_KUSHALA_BREATH_B and EXPLOSION_KUSHALA_BREATH_B_BIG their
 * snowy-mountain versions (for type 1, EXPLOSION_KUSHALA_BREATH_B at size 1).
 */
void KushalaBreath_Impact(ObjAttach *o, s32 unused, f32 *pos)
{
    u16 explosionKind = EXPLOSION_KUSHALA_BREATH_BIG;
    KushalaBreathBuf *buf = (KushalaBreathBuf *) o->buf;
    f32 scale = 1.0f;
    u8 fxKind = 15;

    switch (o->type) {
    case 0:
    case 1:
    case 2:
        break;
    default:
        return;
    }
    switch (o->type) {
    case 0:
    case 2:
        switch (buf->size) {
        case 0:
        case 1:
        default:
            scale = 0.7f;
            explosionKind = EXPLOSION_KUSHALA_BREATH;
            break;
        case 2:
            scale = 1.0f;
            explosionKind = EXPLOSION_KUSHALA_BREATH_BIG;
            break;
        case 3:
        case 4:
            scale = 0.7f;
            explosionKind = EXPLOSION_KUSHALA_BREATH_B;
            break;
        case 5:
            scale = 1.0f;
            explosionKind = EXPLOSION_KUSHALA_BREATH_B_BIG;
            break;
        }
        break;
    case 1:
        scale = 0.7f;
        fxKind = 17;
        if (buf->size != 1) {
            explosionKind = EXPLOSION_KUSHALA_BREATH;
        } else {
            explosionKind = EXPLOSION_KUSHALA_BREATH_B;
        }
        break;
    }
    if (o->area == gScene.area) {
        Em_PlaySeChecked(o->owner, 60, 0, pos, 6, 0);
        MonsterFx_CreateAtYaw(pos, o->rotY, fxKind, scale);
        Explosion_CreateOwned(o->owner, pos, explosionKind);
    }
}

/*
 * Makes a KushalaAttack (one of Kushala Daora's body hit zones) of type for
 * em, if a player is in em's area. Takes em's angles and motion and clears
 * its hitPending and contactHit.
 */
void KushalaAttack_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_KUSHALA_ATTACK;
            o->type = kind;
            o->update = KushalaAttack_Update;
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
 * The KushalaAttack's per-frame update, by ObjAttach.state: a hit keeps it
 * running; states 4, 5, 7 and 9 end it.
 */
void KushalaAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        KushalaAttack_Setup(o);
        break;
    case ATTACH_RUN:
        KushalaAttack_Run(o);
        break;
    case ATTACH_END:
        KushalaAttack_End(o);
        break;
    case ATTACH_HIT:
        KushalaAttack_Run(o);
        break;
    case 4:
        KushalaAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        KushalaAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        KushalaAttack_End(o);
        break;
    case 7:
        KushalaAttack_End(o);
        break;
    case ATTACH_FREE:
        KushalaAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from attack row type: its hit shapes,
 * ATKF_OWNER_CONTACT, and hitDelay counted from the monster's motion.
 */
void KushalaAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &KushalaAttack_RowsPtr);
    o->hitList = KushalaAttack_HitShapes[o->hitListNo];
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
 * Keeps the window open for types 8 and 25. Types 9 and 26 hold it until
 * KUSH_MOT_DASH_END starts, then stay 31 or 1 more frames; type 29 waits for
 * KUSH_MOT_TESCA_ACTION_C_END, then stays 32 more. The attack ends when the
 * window closes or the motion leaves its own: KUSH_MOT_RUN_START to
 * KUSH_MOT_RUN_C (type 8), KUSH_MOT_FLY_FAST to KUSH_MOT_DASH_END (9, 26),
 * KUSH_MOT_TESCA_ACTION_C_END or KUSH_MOT_TESCA_ACTION_C (29), else
 * ownerMotion.
 */
void KushalaAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 8:
        o->hitFrames = 99;
        break;
    case 9:
        switch (o->step) {
        case 0:
            if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_DASH_END)) {
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
            if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_DASH_END)) {
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
            if (em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C_END)) {
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
        if (!(em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_START) || em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_B) || em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_RUN_C)) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case 9:
    case 26:
        if (!(em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_FLY_FAST) || em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_DASH_START) || em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_DASH_END)) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case 29:
        if (!(em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C) || em->motionId[0] == EM_MOTION_ID(0, KUSH_MOT_TESCA_ACTION_C_END)) || o->hitPhase == 0) {
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
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 32U, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void KushalaAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void KushalaAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a ShenGaorenAttack of type at em's position, if a player is in em's
 * area. Types 18-23 are the stomps under the legs
 * (EmShenGaoren_SpawnGroundEffect); the rest come from
 * EmShenGaoren_MotionEffects.
 */
void ShenGaorenAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_GIANT_ATTACK;
            o->type = kind;
            o->update = ShenGaorenAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->pos[0] = em->posX;
            o->pos[1] = em->posY;
            o->pos[2] = em->posZ;
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
 * The ShenGaorenAttack's per-frame update, by ObjAttach.state: a hit keeps it
 * running; states 4, 5, 7 and 9 end it.
 */
void ShenGaorenAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        ShenGaorenAttack_Setup(o);
        break;
    case ATTACH_RUN:
        ShenGaorenAttack_Run(o);
        break;
    case ATTACH_END:
        ShenGaorenAttack_End(o);
        break;
    case ATTACH_HIT:
        ShenGaorenAttack_Run(o);
        break;
    case 4:
        ShenGaorenAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        ShenGaorenAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        ShenGaorenAttack_End(o);
        break;
    case 7:
        ShenGaorenAttack_End(o);
        break;
    case ATTACH_FREE:
        ShenGaorenAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from attack row type: its hit shapes,
 * ATKF_OWNER_CONTACT, and hitDelay counted from the monster's motion.
 */
void ShenGaorenAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &ShenGaorenAttack_RowsPtr);
    o->hitList = ShenGaorenAttack_HitShapes[o->hitListNo];
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
 * Ends the attack when the monster's motion changes or its hit window closes.
 */
void ShenGaorenAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 32U, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void ShenGaorenAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void ShenGaorenAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Makes a YamaTsukamiAttack of type at em's position, if a player is in em's
 * area.
 */
void YamaTsukamiAttack_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = ATTACH_GIANT_ATTACK;
            o->type = kind;
            o->update = YamaTsukamiAttack_Update;
            o->idx = em->index;
            o->isEm = em->isEm;
            o->ownerMotion = em->motionId[0];
            o->owner = em;
            o->yaw = em->rotY;
            o->pos[0] = em->posX;
            o->pos[1] = em->posY;
            o->pos[2] = em->posZ;
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
 * The YamaTsukamiAttack's per-frame update, by ObjAttach.state: a hit keeps
 * it running; states 4, 5, 7 and 9 end it.
 */
void YamaTsukamiAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        YamaTsukamiAttack_Setup(o);
        break;
    case ATTACH_RUN:
        YamaTsukamiAttack_Run(o);
        break;
    case ATTACH_END:
        YamaTsukamiAttack_End(o);
        break;
    case ATTACH_HIT:
        YamaTsukamiAttack_Run(o);
        break;
    case 4:
        YamaTsukamiAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        YamaTsukamiAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        YamaTsukamiAttack_End(o);
        break;
    case 7:
        YamaTsukamiAttack_End(o);
        break;
    case ATTACH_FREE:
        YamaTsukamiAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Sets the attack up from attack row type: its hit shapes,
 * ATKF_OWNER_CONTACT, and hitDelay counted from the monster's motion.
 */
void YamaTsukamiAttack_Setup(ObjAttach *o)
{
    Em *em = &gEms[o->idx];
    s32 blend;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, ATKF_OWNER_CONTACT);
    ObjAttach_StartOwnedAttack(o, (s32) em, o->type, &YamaTsukamiAttack_RowsPtr);
    o->hitList = YamaTsukamiAttack_HitShapes[o->hitListNo];
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
 * Keeps the window open for types 0, 16, 17 and 21. Ends the attack when the
 * monster's motion changes or the window closes.
 */
void YamaTsukamiAttack_Run(ObjAttach *o)
{
    Em *em = &gEms[o->idx];

    switch (o->type) {
    case 0:
    case 16:
    case 17:
    case 21:
        o->hitFrames = 99;
        break;
    }
    if (em->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = ATTACH_END;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 32U, 0);
    }
}

/*
 * Ends the attack: closes its hit window, hides it and frees its draw slot.
 * It is freed on the next update.
 */
void YamaTsukamiAttack_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

/* Frees the attack (ObjAttach_Free). */
void YamaTsukamiAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/* The file's data. */
HitShape KushalaBreath_HitShapes_0[2] = {
#include "game/008A8700.inc"
};
HitShape KushalaBreath_HitShapes_1[2] = {
#include "game/008A8750.inc"
};
HitShape KushalaBreath_HitShapes_2[2] = {
#include "game/008A87A0.inc"
};
HitShape KushalaBreath_HitShapes_3[2] = {
#include "game/008A87F0.inc"
};
HitShape KushalaBreath_HitShapes_4[2] = {
#include "game/008A8840.inc"
};
HitShape KushalaBreath_HitShapes_5[2] = {
#include "game/008A8890.inc"
};
HitShape *KushalaBreath_HitShapes[8] = {
    KushalaBreath_HitShapes_0, KushalaBreath_HitShapes_1, KushalaBreath_HitShapes_2, KushalaBreath_HitShapes_3,
    KushalaBreath_HitShapes_4, KushalaBreath_HitShapes_5, 0, 0,
};
AttackRow KushalaBreath_Rows[11] = {
#include "game/008A8900.inc"
};
void *KushalaBreath_RowsPtr = (void *)KushalaBreath_Rows;
s8 KushalaBreath_FirstRow[16] = {
#include "game/008A8A10.inc"
};
Vec3Key KushalaBreath_ScaleKeys_0[4] = {
#include "game/008A8A20.inc"
};
f32 KushalaBreath_ScaleKeys_1[8] = {
#include "game/008A8A60.inc"
};
f32 KushalaBreath_ScaleKeys_2[8] = {
#include "game/008A8A80.inc"
};
f32 KushalaBreath_ScaleKeys_3[8] = {
#include "game/008A8AA0.inc"
};
f32 KushalaBreath_ScaleKeys_4[16] = {
#include "game/008A8AC0.inc"
};
Vec3Key KushalaBreath_ScaleKeys_5[3] = {
#include "game/008A8B00.inc"
};
void *KushalaBreath_ScaleKeys[8] = {
    (void *)KushalaBreath_ScaleKeys_0, (void *)KushalaBreath_ScaleKeys_1, (void *)KushalaBreath_ScaleKeys_2, (void *)KushalaBreath_ScaleKeys_3,
    (void *)KushalaBreath_ScaleKeys_4, (void *)KushalaBreath_ScaleKeys_5, NULL, NULL,
};
FloatKey KushalaBreath_AlphaKeys_0[3] = {
#include "game/008A8B50.inc"
};
FloatKey KushalaBreath_AlphaKeys_1[3] = {
#include "game/008A8B70.inc"
};
FloatKey KushalaBreath_AlphaKeys_2[3] = {
#include "game/008A8B90.inc"
};
void *KushalaBreath_AlphaKeys[3] = {
    (void *)KushalaBreath_AlphaKeys_0, (void *)KushalaBreath_AlphaKeys_1, (void *)KushalaBreath_AlphaKeys_2,
};
u8 KushalaBreath_Offsets[48] = {
#include "game/008A8BC0.inc"
};
f32 KushalaBreath_SprayOfs[12] = {
#include "game/008A8BF0.inc"
};
f32 KushalaBreath_LaunchOfs[6] = {
#include "game/008A8C20.inc"
};
s16 KushalaBreath_PartCounts[4] = {
#include "game/008A8C38.inc"
};
s16 KushalaBreath_Life[8] = {
#include "game/008A8C40.inc"
};
f32 KushalaBreath_SizeScale[8] = {
#include "game/008A8C50.inc"
};
HitShape KushalaAttack_HitShapes_0[8] = {
#include "game/008A8C70.inc"
};
HitShape KushalaAttack_HitShapes_1[6] = {
#include "game/008A8DB0.inc"
};
HitShape KushalaAttack_HitShapes_2[3] = {
#include "game/008A8EA0.inc"
};
HitShape KushalaAttack_HitShapes_3[3] = {
#include "game/008A8F20.inc"
};
HitShape KushalaAttack_HitShapes_4[6] = {
#include "game/008A8FA0.inc"
};
HitShape KushalaAttack_HitShapes_5[2] = {
#include "game/008A9090.inc"
};
HitShape KushalaAttack_HitShapes_6[8] = {
#include "game/008A90E0.inc"
};
HitShape KushalaAttack_HitShapes_7[6] = {
#include "game/008A9220.inc"
};
HitShape KushalaAttack_HitShapes_8[9] = {
#include "game/008A9310.inc"
};
HitShape KushalaAttack_HitShapes_9[9] = {
#include "game/008A9480.inc"
};
HitShape KushalaAttack_HitShapes_10[3] = {
#include "game/008A95F0.inc"
};
HitShape KushalaAttack_HitShapes_11[3] = {
#include "game/008A9670.inc"
};
HitShape KushalaAttack_HitShapes_12[6] = {
#include "game/008A96F0.inc"
};
HitShape KushalaAttack_HitShapes_13[2] = {
#include "game/008A97E0.inc"
};
HitShape KushalaAttack_HitShapes_14[2] = {
#include "game/008A9830.inc"
};
HitShape KushalaAttack_HitShapes_15[2] = {
#include "game/008A9880.inc"
};
HitShape KushalaAttack_HitShapes_16[2] = {
#include "game/008A98D0.inc"
};
HitShape *KushalaAttack_HitShapes[20] = {
    KushalaAttack_HitShapes_0, KushalaAttack_HitShapes_1, KushalaAttack_HitShapes_2, KushalaAttack_HitShapes_3,
    KushalaAttack_HitShapes_4, KushalaAttack_HitShapes_5, KushalaAttack_HitShapes_6, KushalaAttack_HitShapes_7,
    KushalaAttack_HitShapes_8, KushalaAttack_HitShapes_9, KushalaAttack_HitShapes_10, KushalaAttack_HitShapes_11,
    KushalaAttack_HitShapes_12, KushalaAttack_HitShapes_13, KushalaAttack_HitShapes_14, KushalaAttack_HitShapes_15,
    KushalaAttack_HitShapes_16, 0, 0, 0,
};
AttackRow KushalaAttack_Rows[31] = {
#include "game/008A9970.inc"
};
void *KushalaAttack_RowsPtr = (void *)KushalaAttack_Rows;
HitShape ShenGaorenAttack_HitShapes_0[5] = {
#include "game/008A9C60.inc"
};
HitShape ShenGaorenAttack_HitShapes_1[3] = {
#include "game/008A9D30.inc"
};
HitShape ShenGaorenAttack_HitShapes_2[4] = {
#include "game/008A9DB0.inc"
};
HitShape ShenGaorenAttack_HitShapes_3[4] = {
#include "game/008A9E50.inc"
};
HitShape ShenGaorenAttack_HitShapes_4[3] = {
#include "game/008A9EF0.inc"
};
HitShape ShenGaorenAttack_HitShapes_5[4] = {
#include "game/008A9F70.inc"
};
HitShape ShenGaorenAttack_HitShapes_6[4] = {
#include "game/008AA010.inc"
};
HitShape ShenGaorenAttack_HitShapes_7[3] = {
#include "game/008AA0B0.inc"
};
HitShape ShenGaorenAttack_HitShapes_8[3] = {
#include "game/008AA130.inc"
};
HitShape ShenGaorenAttack_HitShapes_9[3] = {
#include "game/008AA1B0.inc"
};
HitShape ShenGaorenAttack_HitShapes_10[3] = {
#include "game/008AA230.inc"
};
HitShape ShenGaorenAttack_HitShapes_11[3] = {
#include "game/008AA2B0.inc"
};
HitShape ShenGaorenAttack_HitShapes_12[3] = {
#include "game/008AA330.inc"
};
HitShape ShenGaorenAttack_HitShapes_13[3] = {
#include "game/008AA3B0.inc"
};
HitShape *ShenGaorenAttack_HitShapes[16] = {
    ShenGaorenAttack_HitShapes_0, ShenGaorenAttack_HitShapes_1, ShenGaorenAttack_HitShapes_2, ShenGaorenAttack_HitShapes_3,
    ShenGaorenAttack_HitShapes_4, ShenGaorenAttack_HitShapes_5, ShenGaorenAttack_HitShapes_6, ShenGaorenAttack_HitShapes_7,
    ShenGaorenAttack_HitShapes_8, ShenGaorenAttack_HitShapes_9, ShenGaorenAttack_HitShapes_10, ShenGaorenAttack_HitShapes_11,
    ShenGaorenAttack_HitShapes_12, ShenGaorenAttack_HitShapes_13, 0, 0,
};
AttackRow ShenGaorenAttack_Rows[26] = {
#include "game/008AA470.inc"
};
void *ShenGaorenAttack_RowsPtr = (void *)ShenGaorenAttack_Rows;
HitShape YamaTsukamiAttack_HitShapes_0[14] = {
#include "game/008AA6F0.inc"
};
HitShape YamaTsukamiAttack_HitShapes_1[7] = {
#include "game/008AA920.inc"
};
HitShape YamaTsukamiAttack_HitShapes_2[7] = {
#include "game/008AAA40.inc"
};
HitShape YamaTsukamiAttack_HitShapes_3[4] = {
#include "game/008AAB60.inc"
};
HitShape YamaTsukamiAttack_HitShapes_4[4] = {
#include "game/008AAC00.inc"
};
HitShape YamaTsukamiAttack_HitShapes_5[13] = {
#include "game/008AACA0.inc"
};
HitShape YamaTsukamiAttack_HitShapes_6[7] = {
#include "game/008AAEB0.inc"
};
HitShape YamaTsukamiAttack_HitShapes_7[2] = {
#include "game/008AAFD0.inc"
};
HitShape YamaTsukamiAttack_HitShapes_8[2] = {
#include "game/008AB020.inc"
};
HitShape YamaTsukamiAttack_HitShapes_9[14] = {
#include "game/008AB070.inc"
};
HitShape YamaTsukamiAttack_HitShapes_10[2] = {
#include "game/008AB2A0.inc"
};
HitShape YamaTsukamiAttack_HitShapes_11[2] = {
#include "game/008AB2F0.inc"
};
HitShape YamaTsukamiAttack_HitShapes_12[2] = {
#include "game/008AB340.inc"
};
HitShape *YamaTsukamiAttack_HitShapes[16] = {
    YamaTsukamiAttack_HitShapes_0, YamaTsukamiAttack_HitShapes_1, YamaTsukamiAttack_HitShapes_2, YamaTsukamiAttack_HitShapes_3,
    YamaTsukamiAttack_HitShapes_4, YamaTsukamiAttack_HitShapes_5, YamaTsukamiAttack_HitShapes_6, YamaTsukamiAttack_HitShapes_7,
    YamaTsukamiAttack_HitShapes_8, YamaTsukamiAttack_HitShapes_9, YamaTsukamiAttack_HitShapes_10, YamaTsukamiAttack_HitShapes_11,
    YamaTsukamiAttack_HitShapes_12, 0, 0, 0,
};
AttackRow YamaTsukamiAttack_Rows[23] = {
#include "game/008AB3D0.inc"
};
void *YamaTsukamiAttack_RowsPtr = (void *)YamaTsukamiAttack_Rows;
