/*
 * Game overlay code at 0x6E3B00-0x6EA220: the EmEffectAttack_* effect objects
 * (EmEffectAttack_Init to EmEffectAttack_ChainGrowth), one unit: its .data is 0x8AB600-0x8AD3A0
 * and its jump tables 0x8DDF10-0x8DE160, and no function outside calls in.
 */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "objattach.h"
#include "player.h"
#include "scene.h"
#include "stageobj.h"
#include "capsule.h"
#include "area.h"

/* Built at optimization level 4: EmEffectAttack_DrawPart matches only there, every
   other matched function at both levels (KushalaAttack_Run before the unit
   and TeostraAttack_Run after it need level 3). */
#pragma optimization_level 4

/* prototypes of callees (round 13) */
void DrawSlotB_Free(s32);
/* prototypes */
extern s16 EmEffectAttack_ScaleKeyBase[];
extern s16 EmEffectAttack_ColorKeyBase[];
extern void *EmEffectAttack_ScaleKeys[];
extern void *EmEffectAttack_ColorKeys[];
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
/* A link of the chain (kind 0x21 type 0xF, 0x64 bytes: o->buf holds eight of them,
 * EmEffectAttackChain): each follows the one before it (EmEffectAttack_UpdateLink), the first the
 * owner's bone; then they drift up and fade (EmEffectAttack_RunChain). */
typedef struct EmEffectAttackLink {
    s16 slot;                   /* 0x00: its draw slot (-1 none) */
    s16 index;                  /* 0x02: its place in the chain (0 at the bone) */
    f32 pos[3];                 /* 0x04 */
    s16 age;                    /* 0x10: frames (from 8 - index); the texture scroll (EmEffectAttack_DrawChain) */
    s16 delay;                  /* 0x12: counts up from -1 - index; 0 starts the link's fade, drawn to 29 */
    u8 unk14[0x18 - 0x14];
    u16 spin;                   /* 0x18: its roll (EmEffectAttack_RandLinkSpin) */
    s16 spinSpeed;              /* 0x1A */
    f32 scale;                  /* 0x1C: 1 + 0.2 * index */
    DrawSlot *draw;             /* 0x20 */
    f32 size[3];                /* 0x24: the scale per axis (EmEffectAttack_ScaleKeys) */
    u32 color;                  /* 0x30: (EmEffectAttack_ColorKeys) */
    f32 from[2][3];             /* 0x34: where the link starts (the link before's pos) */
    f32 dir[2][3];              /* 0x4C: and its direction */
} EmEffectAttackLink;
typedef char EmEffectAttackLink_size_check[(sizeof(EmEffectAttackLink) == 0x64) ? 1 : -1];

typedef struct EmEffectAttackChain {
    EmEffectAttackLink link[8];        /* 0x000 */
    s16 timer;                  /* 0x320: frames since the owner let go (38: the chain ends) */
    u16 headAlive;              /* 0x322: the first link still follows the owner's bone */
} EmEffectAttackChain;
typedef char EmEffectAttackChain_size_check[(sizeof(EmEffectAttackChain) == 0x324) ? 1 : -1];

void EmEffectAttack_RandLinkSpin(EmEffectAttackLink *e);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Mtx_Copy(f32 *dst, f32 *src);
f32 Math_Atan2(f32 y, f32 x);
s32 Em_PlaySeAtPos(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void CamShake_SetA(s32 kind, f32 *pos);
/* unit's own declaration: with the definition's void (f32 *pos, s32 v, s16 k, f32 scale) (game/unk_874B90.c), EmEffectAttack_Impact: 3 of 193 words differ */
void StageObj_K11_CreateAtScaled(f32 *pos, u16 ang, s32 kind, f32 scale);
void StageObj_K14_CreateAt(f32 *pos, u8 k, u8 c, f32 scale);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void EmImpactAttack_CreateAt(Em *em, f32 *pos, s8 kind);
extern s16 EmEffectAttack_YamaInhaleDelays[];
extern s16 EmEffectAttack_CongalalaBreathDelays[];
extern s16 EmEffectAttack_DaimyoJetDelays[];
extern u16 EmEffectAttack_FatalisBlastVariantRolls[];
extern s16 EmEffectAttack_FatalisBlastFrames[];
u16 Vec_Heading(f32 *a, f32 *b);
u16 Vec_Elevation(f32 *a, f32 *b);
extern u16 *EmEffectAttack_PartModels[];
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
extern f32 gCamMatrix[];
s16 EmSlot_FindKind(s16 k);
u16 Area_DayId(u16 id);
void EmEffectAttack_InitParts(ObjAttach *o);
extern HitShape *EmEffectAttack_HitShapes[];
extern f32 EmEffectAttack_Speeds[];
extern f32 EmEffectAttack_Gravities[];
extern f32 EmEffectAttack_FatalisSweepSpots[][3];
extern f32 EmEffectAttack_FatalisBlastSpots[][3];
extern f32 EmEffectAttack_ShenDropSpotsFortress5[][3];
extern f32 EmEffectAttack_ShenDropSpotsTown2[][3];
extern f32 EmEffectAttack_ShenDropSpotsTown3[][3];
extern f32 EmEffectAttack_ShenDropSpotsTown0[][3];
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
f32 EmEffectAttack_ChainGrowth(ObjAttach *o);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_Concat3x3(f32 *m, f32 *a);
f32 Math_Sqrtf(f32 x);
s32 Gs_SetState(u8 reg, u32 v);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
s16 DrawSlot_Alloc(void);
s16 DrawSlotB_Alloc(void);
struct DrawSlot *DrawSlot_Get(s32 i);
struct DrawSlot *DrawSlotB_Get(s32 i);
void EmEffectAttack_DrawPart(DrawSlot *d);
EmBone *Em_GetBone(void *obj, s16 bone);
extern u8 EmEffectAttack_CongalalaGasKinds[];
extern u16 EmEffectAttack_CongalalaGasStarts[];
f32 Rand_Unit(void);
void Vec_RotateX(f32 *v, f32 angle);
extern s16 EmEffectAttack_DaimyoJetFrames[];
void Vec_RotateY(f32 *v, f32 angle);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *, f32 *, f32 *);
extern u8 EmEffectAttack_Bones[];
extern f32 EmEffectAttack_BoneOfs[];
extern f32 EmEffectAttack_BoneOfsY[];
extern f32 EmEffectAttack_BoneOfsZ[];
StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
void Explosion_CreateOwned(Player *owner, f32 *pos, s32 kind);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
extern s16 EmEffectAttack_MotionBase[];
extern void *EmEffectAttack_RowsPtr;
/*
 * One part of an EmEffectAttack (0x3C bytes): the parts follow the
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
    DrawSlot *draw; /* 0x24 */
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
f32 Rand_Signed(void);
u16 Area_IdForCurrentTime(u16);
ObjAttach *ObjAttach_Alloc(s32 n);
void ObjAttach_Free(ObjAttach *p);
u16 Rand_Next(s32 stream);
void EmEffectAttack_Run(ObjAttach *o);
void EmEffectAttack_Hit(ObjAttach *o);
void EmEffectAttack_End(ObjAttach *o);
void EmEffectAttack_Free(ObjAttach *o);
void DrawSlot_Free(s32);
extern s16 EmEffectAttack_PartCounts[];
void EmEffectAttack_Update(ObjAttach *o);
struct DrawSlot;
s32 OT_AddByDepth(u32 *table, struct DrawSlot *node, u32 n, s32 clamp);
void Vec_Copy(f32 *out, f32 *in);
u8 Player_InCurrentArea(void *obj);
s32 Player_TestSetFlags(void *, s32 mask);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
f32 Math_Absf(f32 x);
void *EmEffectAttack_Create(Em *em, s32 kind, s32 c, s32 a, s32 b);
void EmEffectAttack_UpdateLink(ObjAttach *o, EmEffectAttackLink *e, EmEffectAttackLink *prev, void *bone, f32 *dir, s16 f);
void EmEffectAttack_Setup(ObjAttach *o);

/*
 * Sets o up as an EmEffectAttack of the given type and variant (stateVar1)
 * owned by owner: kind ATTACH_EM_EFFECT_ATTACK, update EmEffectAttack_Update,
 * and the owner's side, index and yaw.
 */
static void EmEffectAttack_Init(ObjAttach *o, Player *owner, s32 attackType, s32 variant)
{
    o->kind = ATTACH_EM_EFFECT_ATTACK;
    o->type = attackType;
    o->stateVar1 = variant;
    o->update = EmEffectAttack_Update;
    o->owner = owner;
    o->isEm = owner->isEm;
    o->idx = owner->index;
    o->side = owner->side;
    o->yaw = owner->rot[1];
}

/*
 * Runs EmEffectAttack_Init, then sets the frame limit in the buffer's header
 * to -1 (none). The header is the owner's motion id, pitch, yaw offset, frame
 * limit and floor height. EmEffectAttack_Run counts the frame limit down for
 * types 10 and 11.
 */
static void EmEffectAttack_CallInit(ObjAttach *o, Em *owner, s32 type, s32 variant)
{
    u8 *attackBuf = o->buf;

    EmEffectAttack_Init(o, (Player *) owner, type, variant);
    *(s16 *) (attackBuf + 6) = -1;
}

/*
 * Makes an EmEffectAttack of the given type for em, if em is in the current
 * area. The variant picks the attack row and colours (for type 12, a spot).
 * pitch is absolute and yaw is added to em's heading. It copies em's kind
 * (aux), stateRand and area. Returns the object, or NULL when em is elsewhere
 * or the pool is full.
 */
void *EmEffectAttack_Create(Em *em, s32 attackType, s32 variant, s32 pitch, s32 yaw)
{
    ObjAttach *o;

    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    if ((o = ObjAttach_Alloc(1)) != NULL) {
        EmEffectAttack_CallInit(o, em, attackType & 0xFF, variant & 0xFF);
        o->aux = em->type;
        o->rotX = (u16) pitch;
        o->rotY = (u16) yaw + em->rotY;
        o->randBits = em->stateRand;
        o->area = em->area;
        return o;
    }
    return NULL;
}

/*
 * Makes an EmEffectAttack of the given type for em in another area
 * (Area_IdForCurrentTime(area)), wherever em is. pitch and yaw are absolute.
 * The object is kept across area changes (keep), so its parts take
 * gDrawSlotsB slots. Used for Shen Gaoren's shot landing in another area
 * (type 8).
 */
void EmEffectAttack_CreateShengaoren(Em *em, s32 attackType, u16 area, s32 pitch, s32 yaw)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(1)) != NULL) {
        EmEffectAttack_CallInit(o, em, attackType & 0xFF, 0);
        o->aux = em->type;
        o->rotX = (s32) (pitch & 0xFFFF);
        o->rotY = (s32) (yaw & 0xFFFF);
        o->randBits = em->stateRand;
        o->keep = 1;
        o->area = Area_IdForCurrentTime(area);
    }
}

/*
 * EmEffectAttack_Create with a frame limit: the stream (type 10 or 11) stops
 * after that many frames instead of at the end of its owner's motion.
 */
void EmEffectAttack_CreateTimed(Em *em, s32 type, s32 variant, s32 pitch, s32 yaw, s16 frames)
{
    ObjAttach *o = EmEffectAttack_Create(em, type, variant, pitch, yaw);

    if (o != NULL) {
        *(s16 *) (o->buf + 6) = frames;
    }
}

/*
 * EmEffectAttack_Create with the spot number moved past the 24 spots of the
 * third Fatalis kind's sweep. EmFatalis_EventDescend's blasts (type 12)
 * therefore use EmEffectAttack_FatalisSweepSpots from row 24 on, with no
 * 30-frame mark. Returns the object or NULL.
 */
void *EmEffectAttack_CreateDescentBlast(Em *em, s16 type, s16 spot, s32 pitch, s32 yaw)
{
    return EmEffectAttack_Create(em, type, (s16) (spot + 0x18), pitch, yaw);
}

/*
 * As EmEffectAttack_Create, but with two buffer blocks for type 15's
 * EmEffectAttackChain and no frame limit. Returns nothing.
 */
void EmEffectAttack_CreateChain(Em *em, s32 attackType, s32 variant, s32 pitch, s32 yaw)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(2)) != NULL) {
            EmEffectAttack_Init(o, (Player *) em, attackType & 0xFF, variant & 0xFF);
            o->aux = em->type;
            o->rotX = (u16) pitch;
            o->rotY = (u16) yaw + em->rotY;
            o->randBits = em->stateRand;
            o->area = em->area;
        }
    }
}

/*
 * The per-frame update: by o->state, runs set up, run, hit, end or free. The
 * other states (4, deflected 5 and 9, and 7) end the object.
 */
void EmEffectAttack_Update(ObjAttach *o)
{
    switch (o->state) {
    case ATTACH_SETUP:
        EmEffectAttack_Setup(o);
        break;
    case ATTACH_RUN:
        EmEffectAttack_Run(o);
        break;
    case ATTACH_END:
        EmEffectAttack_End(o);
        break;
    case ATTACH_HIT:
        EmEffectAttack_Hit(o);
        break;
    case 4:
        EmEffectAttack_End(o);
        break;
    case ATTACH_DEFLECTED:
        EmEffectAttack_End(o);
        break;
    case ATTACH_DEFLECTED_AURA:
        EmEffectAttack_End(o);
        break;
    case 7:
        EmEffectAttack_End(o);
        break;
    case ATTACH_FREE:
        EmEffectAttack_Free(o);
        
    default:
        break;
    }
}

/*
 * Copies o's attack row from EmEffectAttack_Rows: the type's first row
 * (EmEffectAttack_MotionBase) plus its variant (stateVar1). Types 12 and 14
 * always take the first row, since their variant is a spot number. owner is
 * the monster.
 */
void EmEffectAttack_StartMotion(ObjAttach *o, s32 owner)
{
    s16 row;
    u8 attackType = o->type;

    switch (attackType) {
    case 12:
    case 14:
        row = 0;
        break;
    default:
        row = o->stateVar1;
        break;
    }
    MoveObj_StartMotionOwned((struct MoveObj *)o, owner, row + EmEffectAttack_MotionBase[attackType], &EmEffectAttack_RowsPtr);
}

/*
 * Sets up the parts (EmEffectAttackPart, after the buffer's 12-byte header)
 * of every type except 15. By type, it plays the start sounds and fills the
 * header that the streams use. Then for each part it sets the kind, start
 * delay, scale, spins, start place and draw list, and takes a draw slot
 * (drawn by EmEffectAttack_DrawPart; from gDrawSlotsB when o->keep).
 */
void EmEffectAttack_InitParts(ObjAttach *o)
{
    u8 *attackBuf = o->buf;
    EmEffectAttackPart *effPart = (EmEffectAttackPart *) (attackBuf + 0xC);
    Em *em = o->owner;
    s16 count;
    s16 i;
    f32 x;
    f32 y;
    f32 z;
    f32 (*spots)[3];
    u16 bits;

    switch (o->type) {
    case EMFX_YAMA_INHALE:
    case EMFX_CONGALALA_BREATH:
    case EMFX_DAIMYO_JET:
    case EMFX_CHAMELEOS_BREATH:
        *(s16 *) (attackBuf + 4) = o->rotY - em->rotY;
        *(u16 *) attackBuf = em->motionId[0];
        break;
    case EMFX_CONGALALA_GAS:
        o->yaw = o->rotY + 0x8000;
        if (o->aux == 0x3E) {
            o->stateVar0 = 1;
        }
        break;
    case EMFX_CONGALALA_RAGE_GAS:
        Em_PlaySeChecked(em, 0x25, 0, o->pos, 6, 0);
        Em_PlaySeChecked(em, 0x18, 0, o->pos, 6, 0);
        break;
    case EMFX_SHOGUN_JET:
        *(s16 *) (attackBuf + 2) = o->rotX;
        *(s16 *) (attackBuf + 4) = o->rotY - em->rotY;
        *(u16 *) attackBuf = em->motionId[0];
        *(f32 *) (attackBuf + 8) = em->floorY;
        Em_PlaySeChecked(em, 0x3A, 0, o->pos2, 0x18, 0);
        break;
    case EMFX_FATALIS_SWEEP_BLAST:
        if (o->area == gScene.area) {
            Em_PlaySeChecked(em, 0x42, 0, o->pos, 15, 0);
        }
        break;
    case EMFX_FATALIS_BLAST:
        o->hitDelay += 30;
        if (o->area == gScene.area) {
            Em_PlaySeChecked(em, 0x42, 0, o->pos, 15, 0);
        }
        break;
    }
    count = EmEffectAttack_PartCounts[o->type];
    for (i = 0; i < count; i++, effPart++) {
        effPart->frame = 0;
        effPart->type = i;
        effPart->aux = 0;
        effPart->scale = em->scaleX;
        effPart->curveXY = 1;
        effPart->sorted = 1;
        effPart->rotX = 0;
        effPart->rotY = 0;
        effPart->rotZ = 0;
        effPart->spinX = 0;
        effPart->spinY = 0;
        effPart->spinZ = 0;
        switch (o->type) {
        case EMFX_YAMA_INHALE:
            effPart->frame = -EmEffectAttack_YamaInhaleDelays[effPart->type];
            switch (i) {
            case 0:
            default:
                effPart->type = 0;
                effPart->curveXY = 0;
                break;
            case 1:
            case 2:
                effPart->type = 1;
                effPart->curveXY = 0;
                break;
            case 3:
            case 4:
            case 5:
                effPart->type = 2;
                break;
            case 6:
            case 7:
                effPart->type = 3;
                effPart->curveXY = 0;
                effPart->sorted = 0;
                break;
            }
            break;
        case EMFX_CONGALALA_BREATH:
            effPart->frame = -EmEffectAttack_CongalalaBreathDelays[i];
            if (i <= 0) {
                effPart->type = 0;
                effPart->curveXY = 0;
                effPart->rotZ = Rand_Next(1);
                effPart->spinZ = 0x444;
            } else {
                effPart->type = 1;
            }
            break;
        case EMFX_RAJANG_SHOT:
            Vec_Copy(effPart->vec, o->pos);
            effPart->rotX = o->rotX;
            effPart->rotY = o->rotY;
            switch (effPart->type) {
            case 0:
                effPart->curveXY = 0;
                effPart->rotZ = Rand_Next(1);
                effPart->spinZ = 0x1000;
                break;
            case 1:
                effPart->curveXY = 0;
                effPart->rotZ = Rand_Next(1);
                break;
            case 2:
            case 3:
                break;
            }
            break;
        case EMFX_SHEN_SHOT:
        case EMFX_SHEN_SHOT_AWAY:
        case EMFX_SHEN_SHOT_DROP:
            effPart->scale *= 5.0f;
            switch (i) {
            case 0:
                effPart->type = 1;
                effPart->aux = 1;
                effPart->spinX = 0x1222;
                effPart->spinY = 0x234;
                effPart->spinZ = 0;
                break;
            case 1:
                effPart->scale *= 0.9f;
                effPart->aux = 0;
                effPart->spinX = -0xACE;
                effPart->spinY = -0x40D;
                effPart->spinZ = 0;
                break;
            case 2:
                effPart->type = 0;
                break;
            case 3:
                effPart->type = 2;
                effPart->rotX = 0x4000;
                switch (Area_DayId(o->area)) {
                case AREA_FORTRESS_5:
                default:
                    spots = EmEffectAttack_ShenDropSpotsFortress5;
                    break;
                case AREA_BATTLE_TOWN_2:
                    spots = EmEffectAttack_ShenDropSpotsTown2;
                    break;
                case AREA_BATTLE_TOWN_3:
                    spots = EmEffectAttack_ShenDropSpotsTown3;
                    break;
                case AREA_BATTLE_TOWN_0:
                    spots = EmEffectAttack_ShenDropSpotsTown0;
                    break;
                }
                effPart->vec[0] = spots[o->stateVar0][0];
                effPart->vec[1] = 20.0f + spots[o->stateVar0][1];
                effPart->vec[2] = spots[o->stateVar0][2];
                break;
            }
            switch (effPart->type) {
            case 0:
                effPart->rotZ = Rand_Next(1);
                break;
            case 1:
            default:
                bits = Rand_Next(1);
                effPart->rotX = bits & 0xF000;
                effPart->rotY = (bits << 4) & 0xF000;
                effPart->rotZ = (bits << 8) & 0xF000;
                effPart->curveXY = 0;
                break;
            case 2:
                break;
            }
            break;
        case EMFX_DAIMYO_JET:
            effPart->frame = -EmEffectAttack_DaimyoJetDelays[i];
            if (i <= 0) {
                Vec_Copy(effPart->vec, o->pos);
                effPart->rotX = o->rotX;
                effPart->rotY = o->rotY;
                effPart->rotZ = Rand_Next(1);
                effPart->spinZ = 0xE39;
                effPart->curveXY = 0;
                effPart->aux = 0;
                effPart->type = 0;
            } else if (i <= 4) {
                effPart->type = 1;
            } else {
                effPart->type = Rand_Next(1) % 3 + 1;
            }
            break;
        case EMFX_CONGALALA_GAS:
            EmEffectAttack_StartCongalalaGasPuff(o, effPart);
            effPart->vec[1] = em->floorY;
            break;
        case EMFX_CONGALALA_RAGE_GAS:
            effPart->type = 0;
            Vec_Copy(effPart->vec, o->pos);
            effPart->frame = -4;
            x = EmEffectAttack_BoneOfs[o->type * 3];
            y = EmEffectAttack_BoneOfs[o->type * 3 + 1];
            z = EmEffectAttack_BoneOfs[o->type * 3 + 2];
            x += (Rand_Next(1) & 0x3F) - 0x20;
            y += (Rand_Next(1) & 0x1F) - 0x10;
            x += (Rand_Next(1) & 0x3F) - 0x20;
            effPart->vec[0] += x;
            effPart->vec[1] += y;
            effPart->vec[2] += z;
            effPart->rotZ = Rand_Next(1);
            effPart->spinZ = 0x5B;
            bits = Rand_Next(1);
            if (bits & 1) {
                effPart->spinZ = -effPart->spinZ;
            }
            effPart->aux = (bits >> 1) & 1;
            break;
        case EMFX_CHAMELEOS_BALL:
            effPart->type = 0;
            effPart->frame = -i * 2;
            break;
        case EMFX_SHOGUN_JET:
            effPart->aux = 0;
            effPart->curveXY = 0;
            break;
        case EMFX_CHAMELEOS_BREATH:
            effPart->aux = 0;
            effPart->rotZ = Rand_Next(1);
            effPart->spinZ = 0xB61;
            effPart->curveXY = 0;
            break;
        case EMFX_FATALIS_SWEEP_BLAST:
        case EMFX_FATALIS_BLAST:
            Vec_Copy(effPart->vec, o->pos);
            if (o->type == EMFX_FATALIS_SWEEP_BLAST) {
                effPart->scale *= 1.5f;
                if (o->stateVar1 <= 23) {
                    effPart->frame -= 30;
                }
            } else {
                effPart->scale *= 2.0f;
                effPart->frame -= 30;
            }
            switch (effPart->type) {
            case 0:
                effPart->rotZ = EmEffectAttack_FatalisBlastVariantRolls[o->stateVar0];
                break;
            case 1:
                effPart->rotZ = Rand_Next(1);
                break;
            case 2:
                effPart->curveXY = 0;
                effPart->rotY = Rand_Next(1);
                effPart->vec[1] += 5.0f * effPart->scale;
                break;
            case 3:
                break;
            case 4:
                effPart->rotZ = Rand_Next(1);
                break;
            case 5:
                if (o->type == EMFX_FATALIS_SWEEP_BLAST) {
                    if (o->stateVar1 <= 23) {
                        effPart->frame = 0;
                    } else {
                        effPart->frame = EmEffectAttack_FatalisBlastFrames[effPart->type];
                    }
                } else {
                    effPart->frame = 0;
                }
                effPart->rotX = 0x4000;
                effPart->vec[1] += 5.0f;
                break;
            }
            break;
        case EMFX_FATALIS_FIREBALL:
            Vec_Copy(effPart->vec, o->pos);
            effPart->scale = 1.8f;
            effPart->rotX = o->rotX;
            effPart->rotY = o->rotY;
            switch (effPart->type) {
            case 0:
                effPart->curveXY = 0;
                effPart->rotZ = Rand_Next(1);
                effPart->spinZ = 0x1000;
                break;
            case 1:
                effPart->curveXY = 0;
                effPart->rotZ = Rand_Next(1);
                effPart->spinZ = 0x71C;
                break;
            case 2:
                effPart->frame = -15;
                effPart->curveXY = 0;
                effPart->rotZ = Rand_Next(1);
                effPart->spinZ = 0x71C;
                break;
            case 3:
            case 4:
                break;
            case 5:
                effPart->curveXY = 0;
                break;
            }
            break;
        }
        if (o->keep) {
            effPart->slot = DrawSlotB_Alloc();
        } else {
            effPart->slot = DrawSlot_Alloc();
        }
        if (effPart->slot != -1) {
            if (o->keep) {
                effPart->draw = DrawSlotB_Get(effPart->slot);
            } else {
                effPart->draw = DrawSlot_Get(effPart->slot);
            }
            effPart->draw->owner = o;
            effPart->draw->part = i;
            effPart->draw->draw = EmEffectAttack_DrawPart;
        } else {
            effPart->draw = NULL;
        }
    }
}

/*
 * Sets up type 15's chain: places the eight links behind the head bone
 * (EmEffectAttack_UpdateLink), staggers their ages and fade delays, makes
 * them larger down the chain, and takes a draw slot for each.
 */
void EmEffectAttack_InitChain(ObjAttach *o)
{
    EmEffectAttackLink *link;
    EmEffectAttackLink *prev;
    EmEffectAttackLink *head;
    void *bone;
    s16 count;
    s16 i;

    link = (EmEffectAttackLink *)o->buf;
    count = EmEffectAttack_PartCounts[o->type];
    head = link;
    bone = Em_Bone(o->owner, EmEffectAttack_Bones[o->type]);

    /* b is the first link: a copy of e of e's type (an EmEffectAttackChain *b = (EmEffectAttackChain *)e
     * lets MWCC store headAlive through e) */
    ((EmEffectAttackChain *)head)->headAlive = 1;
    for (i = 0; i < count; i++, link++) {
        link->age = 0;
        link->index = i;
        link->scale = 1.0f;
        EmEffectAttack_UpdateLink(o, link, prev, bone, head->dir[0], 0);
        link->age = 8 - i;
        link->delay = -1 - i;
        link->spin = Rand_Next(1);
        if (link->index == 0) {
            link->spinSpeed = 0xB6;
        } else {
            link->spinSpeed = 0;
        }
        link->scale += 0.2f * i;
        link->size[0] = 1.0f;
        link->size[1] = 1.0f;
        link->size[2] = 1.0f;
        link->color = -1;
        prev = link;
        if (o->keep) {
            link->slot = DrawSlotB_Alloc();
        } else {
            link->slot = DrawSlot_Alloc();
        }
        if (link->slot != -1) {
            if (o->keep) {
                link->draw = DrawSlotB_Get(link->slot);
            } else {
                link->draw = DrawSlot_Get(link->slot);
            }
            link->draw->owner = o;
            link->draw->part = i;
            link->draw->draw = EmEffectAttack_DrawPart;
        } else {
            link->draw = NULL;
        }
    }
}

/*
 * The first frame. Starts the object running, gives types 0, 1, 6 and 7 no
 * attack flags and the others ATKF_PIERCES, and takes the type's speed
 * (turned by rotX and rotY) and gravity. Places it at the type's bone plus
 * offset; the types with no bone go to a drop spot (8: set back by 60 frames
 * of flight), a spot around the monster (12) or a fixed spot (14), with a
 * random variant in stateVar0. Then sets up the chain (15) or the parts, the
 * attack row and hit shapes, and 30 more frames of hit delay for the marked
 * blasts. Ends by running a first frame.
 */
void EmEffectAttack_Setup(ObjAttach *o)
{
    f32 off[3];
    Em *em = o->owner;
    void *bone;
    f32 (*spots)[3];
    s16 count;

    o->state = ATTACH_RUN;
    o->visible = 1;
    o->draw = NULL;
    o->step = 0;
    o->stateVar0 = 0;
    o->ownerMotion = 0;
    switch (o->type) {
    case EMFX_YAMA_INHALE:
    case EMFX_CONGALALA_BREATH:
    case EMFX_CONGALALA_GAS:
    case EMFX_CONGALALA_RAGE_GAS:
        Player_TestSetFlags(o, 0);
        break;
    default:
        Player_TestSetFlags(o, ATKF_PIERCES);
        break;
    }
    o->wasHit = 0;
    o->velX = 0.0f;
    o->velY = 0.0f;
    o->velZ = EmEffectAttack_Speeds[o->type];
    Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
    Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
    o->homingTurn = 0.0f;
    o->gravity = EmEffectAttack_Gravities[o->type];
    o->homingSpeed = 0.0f;
    if (EmEffectAttack_Bones[o->type] != 0xFF) {
        bone = Em_Bone(em, EmEffectAttack_Bones[o->type]);
        off[0] = EmEffectAttack_BoneOfs[o->type * 3];
        off[1] = EmEffectAttack_BoneOfs[o->type * 3 + 1];
        off[2] = EmEffectAttack_BoneOfs[o->type * 3 + 2];
        Vec_RotateSelf(off, bone);
        Mtx_GetTrans(o->pos, bone);
        Vec_Add(o->pos, o->pos, off);
    } else {
        switch (o->type) {
        case EMFX_SHEN_SHOT_DROP:
            switch (Area_DayId(o->area)) {
            case AREA_FORTRESS_5:
            default:
                count = 3;
                spots = EmEffectAttack_ShenDropSpotsFortress5;
                break;
            case AREA_BATTLE_TOWN_2:
                count = 4;
                spots = EmEffectAttack_ShenDropSpotsTown2;
                break;
            case AREA_BATTLE_TOWN_3:
                count = 4;
                spots = EmEffectAttack_ShenDropSpotsTown3;
                break;
            case AREA_BATTLE_TOWN_0:
                count = 4;
                spots = EmEffectAttack_ShenDropSpotsTown0;
                break;
            }
            o->stateVar0 = o->randBits % count;
            o->pos[0] = spots[o->stateVar0][0] - 60.0f * o->velX;
            o->pos[1] = spots[o->stateVar0][1] - (60.0f * o->velY + 60.0f * (60.0f * (0.5f * o->gravity)));
            o->pos[2] = spots[o->stateVar0][2] - 60.0f * o->velZ;
            break;
        case EMFX_FATALIS_SWEEP_BLAST:
            o->stateVar0 = Rand_Next(1) % 3;
            Vec_Copy(off, EmEffectAttack_FatalisSweepSpots[o->stateVar1]);
            Vec_RotateY(off, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = em->posX + off[0];
            o->pos[1] = em->posY + off[1];
            o->pos[2] = em->posZ + off[2];
            break;
        case EMFX_FATALIS_BLAST:
            o->stateVar0 = Rand_Next(1) % 3;
            Vec_Copy(o->pos, EmEffectAttack_FatalisBlastSpots[o->stateVar1]);
            break;
        }
    }
    Vec_Copy(o->pos2, o->pos);
    o->drawSlotBuf = NULL;
    switch (o->type) {
    case EMFX_BREATH_CHAIN:
        EmEffectAttack_InitChain(o);
        break;
    default:
        EmEffectAttack_InitParts(o);
        break;
    }
    EmEffectAttack_StartMotion(o, (s32) em);
    o->hitList = EmEffectAttack_HitShapes[o->hitListNo];
    o->hurtList = 0;
    switch (o->type) {
    case EMFX_FATALIS_SWEEP_BLAST:
        if (o->stateVar1 <= 23) {
            o->hitDelay += 30;
        }
        break;
    case EMFX_FATALIS_BLAST:
        o->hitDelay += 30;
        break;
    }
    EmEffectAttack_Run(o);
}

/* The owner's bone 0x25, through an inlined helper: its return value is a
 * temporary with a 16-byte stack slot, where bone is spilled (sp+0xB0; a
 * plain `bone = Em_Bone(m, 0x25)` gets a 4-byte slot at sp+0xBC). */
static inline void *EmEffectAttack_ChainBone(Em *em)
{
    return Em_Bone(em, 0x25);
}

/*
 * Runs a frame of the breath chain (type 15). While the owner is alive and
 * stays in one of its two breath motions for under 300 frames, the head
 * follows the head bone (pos2) and the links follow the head
 * (EmEffectAttack_UpdateLink), with a sound every 10 frames. Once the owner
 * lets go, each link in turn fades (delay), drifting up on its scale and
 * colour keys. Returns 1 when the chain is over, 38 frames later; otherwise
 * 0.
 */
s32 EmEffectAttack_RunChain(ObjAttach *o)
{
    f32 off[3];
    EmEffectAttackChain *chain = (EmEffectAttackChain *)o->buf;
    EmEffectAttackLink *chainLink = chain->link;
    Em *em = o->owner;
    s16 count = EmEffectAttack_PartCounts[o->type];
    s16 curveIdx = EmEffectAttack_ScaleKeyBase[o->type];
    s16 colorIdx = EmEffectAttack_ColorKeyBase[o->type];
    u8 moved = 0;
    EmEffectAttackLink *prev;
    void *bone;
    s16 i;

    switch (o->stateVar0) {
    case 0:
        if (em->active == 0 || em->taskPhase == 3 || (em->motionId[0] != 0x417 && em->motionId[0] != 0x41A) || o->ownerMotion >= 300) {
            chain->timer = 0;
            Vec_Copy(chain->link[0].pos, chain->link[0].from[0]);
            o->stateVar0++;
            o->hitPhase = 0;
        } else {
            bone = EmEffectAttack_ChainBone(em);
            off[0] = EmEffectAttack_BoneOfs[o->type * 3];
            off[1] = EmEffectAttack_BoneOfs[o->type * 3 + 1];
            off[2] = EmEffectAttack_BoneOfs[o->type * 3 + 2];
            Vec_RotateSelf(off, bone);
            Mtx_GetTrans(o->pos2, bone);
            Vec_Add(o->pos2, o->pos2, off);
        }
        break;
    case 1:
        if (++chain->timer >= 38) {
            return 1;
        }
        break;
    }
    for (i = 0; i < count; i++, chainLink++) {
        (chainLink->age)++;
        switch (o->stateVar0) {
        case 0:
            EmEffectAttack_UpdateLink(o, chainLink, prev, bone, chain->link[0].dir[0], 1);
            chainLink->spin += (s32) chainLink->spinSpeed;
            prev = chainLink;
            Vec_Copy(o->pos, chainLink->pos);
            moved = 1;
            break;
        case 1:
        default:
            if (++chainLink->delay == 0) {
                EmEffectAttack_RandLinkSpin(chainLink);
            }
            if (chainLink->delay > 29) {
                continue;
            }
            if (chainLink->delay >= 0) {
                Key_LerpVec2(EmEffectAttack_ScaleKeys[curveIdx], chainLink->size, (f32) chainLink->delay);
                Color_EvalKeys(EmEffectAttack_ColorKeys[colorIdx], (f32) chainLink->delay, &chainLink->color);
                chainLink->pos[1] += 2.0f;
            }
            break;
        case 2:
            if (++chainLink->delay == 0) {
                if (chain->headAlive != 0) {
                    if (em->active != 0 || em->taskPhase != 3) {
                        Em_BonePos(em, 0x25, chainLink->pos);
                    } else {
                        chain->headAlive = 0;
                        chainLink->delay = 30;
                    }
                } else {
                    chainLink->delay = 30;
                }
            }
            if (chainLink->delay > 29) {
                continue;
            }
            if (chainLink->delay <= 0) {
                continue;
            }
            Key_LerpVec2(EmEffectAttack_ScaleKeys[curveIdx], chainLink->size, (f32) chainLink->delay);
            Color_EvalKeys(EmEffectAttack_ColorKeys[colorIdx], (f32) chainLink->delay, &chainLink->color);
            chainLink->pos[1] += 2.0f;
            break;
        }
        if (chainLink->draw != NULL) {
            Vec_Copy(chainLink->draw->pos, chainLink->pos);
            OT_AddByDepth(gDrawOtSorted, chainLink->draw, 0x40U, 0);
        }
    }
    if (moved != 0 && o->ownerMotion % 10 == 0) {
        Em_PlaySeChecked(em, 0x42, 0, o->pos, 6, 0);
    }
    return 0;
}

void MoveObj_Integrate(struct MoveObj *);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
f32 AreaGrid2_GetFloorYFiltered(f32 *pos);
void StageObj_K1C_CreateAt(f32 *src, u8 k, f32 f);
void StageObj_K1C_CreateAtAngled(f32 *pos, u16 w, u16 v, u8 k, f32 scale);
void StageObj_K1C_CreateTwoPoint(f32 *pos, f32 *pos2, u8 k, f32 scale);
void Vec_Diff(f32 *out, f32 *a, f32 *b);
void Vec_Scale(f32 *out, f32 *in, f32 s);
void EmEffectAttack_Impact(ObjAttach *o, s16 n, f32 *pos);
void EmEffectAttack_PlaceAtBone(ObjAttach *o, u16 *ang);
void EmEffectAttack_StartYamaInhalePart(ObjAttach *o, EmEffectAttackPart *p);
void EmEffectAttack_StartCongalalaBreathPart(ObjAttach *o, EmEffectAttackPart *p);
void EmEffectAttack_StartDaimyoJetDrop(ObjAttach *o, EmEffectAttackPart *p);
void EmEffectAttack_PlaceCongalalaGasPuff(ObjAttach *o, EmEffectAttackPart *p);
void EmEffectAttack_StartChameleosBallPart(ObjAttach *o, EmEffectAttackPart *p);
s32 EmEffectAttack_RunChain(ObjAttach *o);
extern s16 EmEffectAttack_Lives[];
extern u32 EmEffectAttack_PartFrames[];

/*
 * Runs a frame of an EmEffectAttack. Counts its age in ownerMotion and ends
 * it once past the type's life (EmEffectAttack_Lives; 1000 means no limit).
 * By type, it follows the bone (streams 0, 1, 4, 10 and 11, until the owner's
 * motion ends or their time runs out), flies (the shots) or runs the chain
 * (15). Then it steps each part's frame, scale and colour keys, place and
 * spin, and queues the part's draw slot. Last, it stretches a stream's hit
 * capsule from pos2 to pos, makes a shot's impact where it meets the floor
 * (the drop's on its last frame), and ends a stopped stream once all its
 * parts have faded.
 */
void EmEffectAttack_Run(ObjAttach *o)
{
    f32 ofs[3];
    f32 ground[3];
    u8 *attackBuf = o->buf;
    Em *em = o->owner;
    EmEffectAttackPart *part = (EmEffectAttackPart *) (attackBuf + 0xC);
    s16 *frameTbl;
    s16 count;
    s32 colorTime;
    s16 colorIdx;
    u8 stop;
    s16 limit;
    s16 faded;
    s16 i;
    s16 curveIdx;
    f32 scaleTime;
    f32 floor;
    f32 floorFrac;
    f32 (*spots)[3];
    s32 total;
    s32 life;

    count = EmEffectAttack_PartCounts[o->type];
    life = EmEffectAttack_Lives[o->type];
    frameTbl = (s16 *) EmEffectAttack_PartFrames[o->type];
    o->ownerMotion++;
    if (life != 1000 && life < o->ownerMotion) {
        o->hitFrames = 0;
        o->hitPhase = 0;
        o->state = ATTACH_END;
        return;
    }
    o->hitFrames = 99;
    switch (o->type) {
    default:
        o->pos2[0] = o->pos[0];
        o->pos2[1] = o->pos[1];
        o->pos2[2] = o->pos[2];
        break;
    case EMFX_YAMA_INHALE:
    case EMFX_CONGALALA_BREATH:
    case EMFX_DAIMYO_JET:
    case EMFX_SHOGUN_JET:
    case EMFX_CHAMELEOS_BREATH:
        break;
    case EMFX_SHEN_SHOT_DROP:
        if (o->ownerMotion <= 0x8C) {
            return;
        }
        o->pos2[0] = o->pos[0];
        o->pos2[1] = o->pos[1];
        o->pos2[2] = o->pos[2];
        break;
    case EMFX_BREATH_CHAIN:
        if ((u8) EmEffectAttack_RunChain(o) == 1) {
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        return;
    }
    switch (o->type) {
    case EMFX_YAMA_INHALE:
        if (em->active == 0 || em->taskPhase == 3 || *(s16 *) attackBuf != em->motionId[0]) {
            o->hitPhase = 0;
            o->hitFrames = 0;
            o->state = ATTACH_END;
            return;
        }
        EmEffectAttack_PlaceAtBone(o, (u16 *) attackBuf);
        o->yaw = o->rotY;
        break;
    case EMFX_CONGALALA_BREATH:
        switch (o->stateVar0) {
        case 0:
            if (em->active == 0 || em->taskPhase == 3 || *(s16 *) attackBuf != em->motionId[0] || EmMotion_ReachedFrame(em, 0, 235.0f) != 0) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                o->stateVar0 = 0xFF;
                faded = 0;
            } else {
                EmEffectAttack_PlaceAtBone(o, (u16 *) attackBuf);
                o->yaw = o->rotY;
            }
            break;
        case 0xFF:
            faded = 0;
            break;
        }
        break;
    case EMFX_RAJANG_SHOT:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
            Em_PlaySeChecked(o->owner, 0xE, 0, o->pos, 0x18, 0);
        }
        break;
    case EMFX_SHEN_SHOT:
    case EMFX_SHEN_SHOT_AWAY:
        if (o->velY < 0.0f) {
            o->velX *= 0.985f;
            o->velZ *= 0.985f;
        }
        MoveObj_Integrate((struct MoveObj *)o);
        Vec_DirToAngles(&o->velX, (s16 *) &o->rotX, (s16 *) &o->rotY);
        if (o->area == gScene.area) {
            if (o->ownerMotion < life - 5 && o->ownerMotion >= 4 && o->ownerMotion % 3 == 2) {
                StageObj_K1C_CreateAtAngled(o->pos, o->rotX, o->rotY, 0x1B, 1.0f);
            }
            if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
                Em_PlaySeAtPos(em, 0x38, 0, o->pos, 0x18, 0);
            }
        }
        break;
    case EMFX_DAIMYO_JET:
        faded = 0;
        if (o->stateVar0 != 0xFF) {
            if (o->ownerMotion >= 0x26 || em->active == 0 || em->taskPhase == 3) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                o->stateVar0 = 0xFF;
            } else {
                EmEffectAttack_PlaceAtBone(o, (u16 *) attackBuf);
            }
        }
        break;
    case EMFX_CONGALALA_GAS:
        if (em->active == 0 || em->taskPhase == 3) {
            o->hitPhase = 0;
            o->hitFrames = 0;
            o->state = ATTACH_END;
            return;
        }
        if (o->ownerMotion >= 0x24) {
            o->hitPhase = 0;
        }
        break;
    case EMFX_CONGALALA_RAGE_GAS:
        if (o->ownerMotion >= 0x2A) {
            o->hitPhase = 0;
        }
        break;
    case EMFX_SHEN_SHOT_DROP:
        MoveObj_Integrate((struct MoveObj *)o);
        Vec_DirToAngles(&o->velX, (s16 *) &o->rotX, (s16 *) &o->rotY);
        if (o->area == gScene.area && o->ownerMotion < life - 5 && o->ownerMotion >= 4 && o->ownerMotion % 3 == 2) {
            StageObj_K1C_CreateAtAngled(o->pos, o->rotX, o->rotY, 0x1B, 1.0f);
        }
        break;
    case EMFX_CHAMELEOS_BALL:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area && o->ownerMotion > 0 && (o->ownerMotion & 1) == 0) {
            StageObj_K1C_CreateAt(o->pos, 0x1F, 1.0f);
        }
        break;
    case EMFX_SHOGUN_JET:
        switch (o->stateVar0) {
        case 0:
            stop = 0;
            if (em->active == 0 || em->taskPhase == 3 || *(s16 *) attackBuf != em->motionId[0]) {
                stop = 1;
            } else if (*(s16 *) (attackBuf + 6) < 0) {
                if (EmMotion_ReachedFrame(em, 0, 168.0f) != 0) {
                    stop = 1;
                }
            } else if (--*(s16 *) (attackBuf + 6) <= 0) {
                stop = 1;
            }
            if (stop & 0xFF) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                faded = 0;
                o->stateVar0 = 0xFF;
            } else {
                EmEffectAttack_PlaceAtBone(o, (u16 *) attackBuf);
                o->yaw = o->rotY;
            }
            break;
        case 0xFF:
            faded = 0;
            break;
        }
        break;
    case EMFX_CHAMELEOS_BREATH:
        switch (o->stateVar0) {
        case 0:
            if (em->active == 0 || em->taskPhase == 3 || *(s16 *) attackBuf != em->motionId[0] || --*(s16 *) (attackBuf + 6) <= 0) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                o->stateVar0 = 0xFF;
                faded = 0;
            } else {
                EmEffectAttack_PlaceAtBone(o, (u16 *) attackBuf);
            }
            break;
        case 0xFF:
            faded = 0;
            break;
        }
        break;
    case EMFX_FATALIS_SWEEP_BLAST:
    case EMFX_FATALIS_BLAST:
        break;
    case EMFX_FATALIS_FIREBALL:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
            Em_PlaySeChecked(o->owner, 0x38, 0, o->pos, 0x18, 0);
        }
        break;
    default:
        MoveObj_Integrate((struct MoveObj *)o);
        break;
    }
    total = count;
    for (i = 0; i < total; i++, part++) {
        if (frameTbl != NULL) {
            limit = frameTbl[part->type];
        }
        switch (o->type) {
        case EMFX_CONGALALA_BREATH:
            if (o->stateVar0 == 0xFF) {
                switch (part->type) {
                case 0:
                    faded++;
                    continue;
                }
            }
            curveIdx = part->type + EmEffectAttack_ScaleKeyBase[o->type];
            colorIdx = part->type + (EmEffectAttack_ColorKeyBase[o->type] + o->stateVar1 * 2);
            break;
        case EMFX_DAIMYO_JET:
            curveIdx = part->type + EmEffectAttack_ScaleKeyBase[o->type];
            colorIdx = part->type + EmEffectAttack_ColorKeyBase[o->type];
            break;
        case EMFX_CONGALALA_GAS:
            if (part->type == 0xFF) {
                continue;
            }
            curveIdx = part->type + EmEffectAttack_ScaleKeyBase[o->type];
            colorIdx = part->type + EmEffectAttack_ColorKeyBase[o->type];
            break;
        case EMFX_CHAMELEOS_BALL:
            curveIdx = part->type + EmEffectAttack_ScaleKeyBase[o->type];
            colorIdx = o->stateVar1 + EmEffectAttack_ColorKeyBase[o->type];
            break;
        default:
            curveIdx = part->type + EmEffectAttack_ScaleKeyBase[o->type];
            colorIdx = part->type + EmEffectAttack_ColorKeyBase[o->type];
            break;
        }
        if (++part->frame > limit) {
            switch (o->type) {
            case EMFX_YAMA_INHALE:
                switch (part->type) {
                case 2:
                    part->frame = -30;
                    break;
                default:
                    part->frame = 1;
                    break;
                }
                break;
            case EMFX_CONGALALA_BREATH:
                if (o->stateVar0 == 0xFF) {
                    part->frame = limit;
                    faded++;
                    continue;
                }
                switch (part->type) {
                default:
                    part->frame = limit;
                    break;
                case 1:
                    part->frame = -5;
                    break;
                }
                break;
            case EMFX_RAJANG_SHOT:
                part->frame = 1;
                switch (part->type) {
                case 1:
                    part->rotZ = Rand_Next(1);
                    break;
                }
                break;
            case EMFX_DAIMYO_JET:
                if (o->stateVar0 == 0xFF) {
                    part->frame = limit;
                    faded++;
                    continue;
                }
                switch (part->type) {
                case 0:
                    break;
                case 1:
                    part->frame = -5;
                    break;
                case 2:
                case 3:
                case 4:
                    part->frame = -3;
                    break;
                }
                break;
            case EMFX_CONGALALA_GAS:
                EmEffectAttack_StartCongalalaGasPuff(o, part);
                if (part->type == 0xFF) {
                    continue;
                }
                if (frameTbl != NULL) {
                    limit = frameTbl[part->type];
                }
                break;
            case EMFX_CHAMELEOS_BALL:
                part->frame = 1;
                break;
            case EMFX_SHOGUN_JET:
                part->frame = limit;
                break;
            case EMFX_CHAMELEOS_BREATH:
                part->frame = limit;
                break;
            case EMFX_FATALIS_FIREBALL:
                part->frame = 1;
                break;
            }
        }
        if (part->frame <= 0 || part->frame > limit) {
            switch (o->type) {
            case EMFX_CONGALALA_BREATH:
            case EMFX_DAIMYO_JET:
            case EMFX_CONGALALA_GAS:
                if (o->stateVar0 == 0xFF) {
                    part->frame = limit;
                    faded++;
                }
                break;
            }
            continue;
        }
        if (part->frame == 1) {
            switch (o->type) {
            case EMFX_YAMA_INHALE:
                EmEffectAttack_StartYamaInhalePart(o, part);
                break;
            case EMFX_CONGALALA_BREATH:
                if (o->stateVar0 == 0xFF) {
                    part->frame = limit;
                    faded++;
                    continue;
                }
                EmEffectAttack_StartCongalalaBreathPart(o, part);
                break;
            case EMFX_DAIMYO_JET:
                EmEffectAttack_StartDaimyoJetDrop(o, part);
                break;
            case EMFX_CONGALALA_GAS:
                EmEffectAttack_PlaceCongalalaGasPuff(o, part);
                break;
            case EMFX_CHAMELEOS_BALL:
                EmEffectAttack_StartChameleosBallPart(o, part);
                break;
            case EMFX_FATALIS_FIREBALL:
                switch (part->type) {
                case 3:
                    part->vec[0] = 40.0f * Rand_Signed();
                    part->vec[1] = 40.0f * Rand_Signed();
                    part->vec[2] = 40.0f * Rand_Signed();
                    part->rotZ = Rand_Next(1);
                    break;
                }
                break;
            }
        }
        switch (o->type) {
        default:
        case EMFX_YAMA_INHALE:
        case EMFX_RAJANG_SHOT:
        case EMFX_CONGALALA_GAS:
        case EMFX_CONGALALA_RAGE_GAS:
        case EMFX_CHAMELEOS_BALL:
        case EMFX_FATALIS_SWEEP_BLAST:
        case EMFX_FATALIS_FIREBALL:
            colorTime = part->frame;
            scaleTime = colorTime;
            break;
        case EMFX_CONGALALA_BREATH:
            switch (part->type) {
            case 0:
                scaleTime = (o->ownerMotion & 1) + 1;
                break;
            default:
                scaleTime = part->frame;
                break;
            }
            colorTime = part->frame;
            break;
        case EMFX_SHEN_SHOT:
        case EMFX_SHEN_SHOT_AWAY:
        case EMFX_SHEN_SHOT_DROP:
            switch (part->type) {
            case 2:
                colorTime = part->frame;
                scaleTime = colorTime;
                break;
            default:
                if (part->frame >= 8) {
                    scaleTime = 8.0f;
                } else {
                    scaleTime = part->frame;
                }
                colorTime = (part->frame + part->type) % 7;
                break;
            }
            break;
        case EMFX_DAIMYO_JET:
            switch (part->type) {
            case 0:
                if (o->stateVar0 == 0xFF) {
                    if (++part->aux >= 10) {
                        part->aux = 10;
                        faded++;
                        continue;
                    }
                    scaleTime = 4.0f + part->aux;
                } else if (part->frame > 4) {
                    scaleTime = 4.0f;
                } else {
                    scaleTime = part->frame;
                }
                colorTime = part->aux;
                break;
            case 1:
            default:
                colorTime = part->frame;
                scaleTime = colorTime;
                break;
            }
            break;
        case EMFX_SHOGUN_JET:
            if (o->stateVar0 == 0xFF) {
                if (++part->aux >= 10) {
                    part->aux = 10;
                    faded++;
                    continue;
                }
                scaleTime = part->aux + 10;
            } else {
                scaleTime = part->frame;
            }
            colorTime = part->aux;
            break;
        case EMFX_CHAMELEOS_BREATH:
            if (o->stateVar0 == 0xFF) {
                if (++part->aux >= 20) {
                    part->aux = 20;
                    faded++;
                    continue;
                }
                colorTime = part->aux + 10;
                scaleTime = colorTime;
            } else {
                colorTime = part->frame;
                scaleTime = colorTime;
            }
            break;
        }
        if (part->curveXY != 0) {
            Key_LerpVec2(EmEffectAttack_ScaleKeys[curveIdx], part->size, scaleTime);
        } else {
            Key_LerpVec3(EmEffectAttack_ScaleKeys[curveIdx], part->size, scaleTime);
        }
        Color_EvalKeys(EmEffectAttack_ColorKeys[colorIdx], colorTime, (u32 *) &part->color);
        switch (o->type) {
        case EMFX_YAMA_INHALE:
            switch (part->type) {
            case 0:
            case 1:
                Vec_Copy(part->vec, o->pos);
                part->rotX = o->rotX;
                part->rotY = o->rotY;
                break;
            case 2:
            case 3:
            case 4:
                Vec_Diff(ofs, o->pos, part->vec);
                Vec_Scale(ofs, ofs, 0.8f / (f32) (limit + 1 - part->frame));
                Vec_Add(part->vec, part->vec, ofs);
                break;
            }
            break;
        case EMFX_CONGALALA_BREATH:
            if (part->type != 1) {
                Vec_Copy(part->vec, o->pos);
                part->rotX = o->rotX;
                part->rotY = o->rotY;
            } else {
                part->vec[1] += 2.0f;
            }
            break;
        case EMFX_RAJANG_SHOT:
            Vec_Copy(part->vec, o->pos);
            break;
        case EMFX_SHEN_SHOT:
        case EMFX_SHEN_SHOT_AWAY:
        case EMFX_SHEN_SHOT_DROP:
            if (part->type != 2) {
                Vec_Copy(part->vec, o->pos);
                part->rotX = o->rotX;
                part->rotY = o->rotY;
            }
            break;
        case EMFX_DAIMYO_JET:
            switch (part->type) {
            case 0:
                Vec_Copy(part->vec, o->pos2);
                break;
            case 1:
                part->speed += -0.5f;
                part->vec[1] += part->speed;
                break;
            case 2:
            case 3:
            case 4:
                part->vec[1] += part->speed;
                break;
            }
            break;
        case EMFX_CONGALALA_GAS:
            switch (part->type) {
            default:
                part->speed *= 0.95f;
                ofs[0] = 0.0f;
                ofs[1] = 0.0f;
                ofs[2] = part->speed;
                Vec_RotateY(ofs, (6.2831855f * part->rotY) / 65536.0f);
                part->vec[0] += ofs[0];
                part->vec[1] += 3.0f * part->scale;
                part->vec[2] += ofs[2];
                break;
            case 0:
                break;
            }
            break;
        case EMFX_CONGALALA_RAGE_GAS:
            part->vec[1] += 5.0f * part->scale;
            break;
        case EMFX_SHOGUN_JET:
        case EMFX_CHAMELEOS_BREATH:
            Vec_Copy(part->vec, o->pos2);
            part->rotX = o->rotX;
            part->rotY = o->rotY;
            break;
        case EMFX_FATALIS_BLAST:
            break;
        }
        part->rotX += part->spinX;
        part->rotY += part->spinY;
        part->rotZ += part->spinZ;
        if (part->draw != NULL) {
            switch (o->type) {
            default:
                Vec_Copy(part->draw->pos, part->vec);
                break;
            case EMFX_CHAMELEOS_BALL:
                part->draw->pos[0] = o->pos[0] + part->vec[0];
                part->draw->pos[1] = o->pos[1] + part->vec[1];
                part->draw->pos[2] = o->pos[2] + part->vec[2];
                break;
            case EMFX_FATALIS_FIREBALL:
                switch (part->type) {
                case 3:
                    part->draw->pos[0] = o->pos[0] + part->vec[0];
                    part->draw->pos[1] = o->pos[1] + part->vec[1];
                    part->draw->pos[2] = o->pos[2] + part->vec[2];
                    break;
                default:
                    Vec_Copy(part->draw->pos, o->pos);
                    break;
                }
                break;
            }
            if (part->sorted == 0) {
                OT_AddByDepth(gDrawOt32, part->draw, 0x20, 0);
            } else {
                OT_AddByDepth(gDrawOtSorted, part->draw, 0x40, 0);
            }
        }
    }
    switch (o->type) {
    case EMFX_YAMA_INHALE:
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = 1900.0f * em->scaleZ;
        Vec_RotateX(ofs, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
        o->pos[0] = o->pos2[0] + ofs[0];
        o->pos[1] = o->pos2[1] + ofs[1];
        o->pos[2] = o->pos2[2] + ofs[2];
        break;
    case EMFX_CONGALALA_BREATH:
        if (o->hitPhase != 0) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 200.0f * em->scaleZ;
            Vec_RotateX(ofs, (6.2831855f * o->rotX) / 65536.0f);
            Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + ofs[0];
            o->pos[1] = o->pos2[1] + ofs[1];
            o->pos[2] = o->pos2[2] + ofs[2];
        } else if (faded >= total) {
            o->state = ATTACH_END;
        }
        break;
    case EMFX_RAJANG_SHOT:
    case EMFX_CHAMELEOS_BALL:
    case EMFX_FATALIS_FIREBALL:
        floor = AreaGrid2_GetFloorYFiltered(o->pos);
        if (o->pos[1] <= floor) {
            if (floor - o->pos[1] > 100.0f) {
                EmEffectAttack_Impact(o, 2, o->pos);
            } else {
                ground[0] = o->pos[0];
                ground[1] = floor;
                ground[2] = o->pos[2];
                EmEffectAttack_Impact(o, 0, ground);
            }
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case EMFX_SHEN_SHOT:
        floor = AreaGrid2_GetFloorYFiltered(o->pos);
        if (o->pos[1] <= floor) {
            if (floor - o->pos[1] > 200.0f) {
                EmEffectAttack_Impact(o, 1, o->pos);
            } else {
                ground[0] = o->pos[0];
                ground[1] = floor;
                ground[2] = o->pos[2];
                EmEffectAttack_Impact(o, 0, ground);
            }
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = ATTACH_END;
        }
        break;
    case EMFX_DAIMYO_JET:
        part = (EmEffectAttackPart *) (attackBuf + 0xC);
        if (o->hitPhase != 0) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 300.0f * part->size[2];
            Vec_RotateX(ofs, (6.2831855f * o->rotX) / 65536.0f);
            Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + ofs[0];
            o->pos[1] = o->pos2[1] + ofs[1];
            o->pos[2] = o->pos2[2] + ofs[2];
            if (o->area == gScene.area && o->ownerMotion % 10 == 0) {
                Em_PlaySeChecked(em, 0x44, 0, o->pos, 6, 0);
            }
        } else if (faded >= total) {
            o->state = ATTACH_END;
        }
        break;
    case EMFX_CONGALALA_GAS:
        if (o->hitPhase != 0) {
            ofs[0] = 0.0f;
            ofs[1] = 2.0f;
            ofs[2] = -5.0f;
            Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + ofs[0];
            o->pos[1] = o->pos2[1] + ofs[1];
            o->pos[2] = o->pos2[2] + ofs[2];
        }
        break;
    case EMFX_CONGALALA_RAGE_GAS:
        if (o->hitPhase != 0) {
            o->pos[1] = 5.0f + o->pos2[1];
        }
        break;
    case EMFX_SHEN_SHOT_DROP:
        if (o->ownerMotion == life) {
            switch (Area_DayId(o->area)) {
            case AREA_FORTRESS_5:
            default:
                spots = EmEffectAttack_ShenDropSpotsFortress5;
                break;
            case AREA_BATTLE_TOWN_2:
                spots = EmEffectAttack_ShenDropSpotsTown2;
                break;
            case AREA_BATTLE_TOWN_3:
                spots = EmEffectAttack_ShenDropSpotsTown3;
                break;
            case AREA_BATTLE_TOWN_0:
                spots = EmEffectAttack_ShenDropSpotsTown0;
                break;
            }
            EmEffectAttack_Impact(o, 0, spots[o->stateVar0]);
        }
        break;
    case EMFX_SHOGUN_JET:
        part = (EmEffectAttackPart *) (attackBuf + 0xC);
        if (o->hitPhase != 0) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 20.0f * part->size[2];
            Vec_RotateX(ofs, (6.2831855f * o->rotX) / 65536.0f);
            Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + ofs[0];
            o->pos[1] = o->pos2[1] + ofs[1];
            o->pos[2] = o->pos2[2] + ofs[2];
            if (o->area == gScene.area && Math_Absf(ofs[1]) > 500.0f) {
                floorFrac = (*(f32 *) (attackBuf + 8) - o->pos2[1]) / ofs[1];
                if (Math_Absf(floorFrac) > 0.0f && Math_Absf(floorFrac) <= 1.0f && (o->ownerMotion & 3) == 0) {
                    ground[0] = o->pos2[0] + ofs[0] * floorFrac;
                    ground[1] = *(f32 *) (attackBuf + 8);
                    ground[2] = o->pos2[2] + ofs[2] * floorFrac;
                    Em_PlaySeChecked(em, 0x3B, 0, ground, 0x18, 0);
                    StageObj_K1C_CreateAt(ground, 0x23, 1.0f);
                }
            }
        } else if (faded >= total) {
            o->state = ATTACH_END;
        }
        break;
    case EMFX_CHAMELEOS_BREATH:
        if (o->hitPhase != 0) {
            ofs[0] = 0.0f;
            ofs[1] = 0.0f;
            ofs[2] = 1.0f;
            Vec_RotateX(ofs, (6.2831855f * o->rotX) / 65536.0f);
            Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + 900.0f * ofs[0];
            o->pos[1] = o->pos2[1] + 900.0f * ofs[1];
            o->pos[2] = o->pos2[2] + 900.0f * ofs[2];
            if (o->area == gScene.area) {
                if (o->ownerMotion % 10 == 0) {
                    Em_PlaySeChecked(em, 0x3C, 0, o->pos, 6, 0);
                }
                if (o->ownerMotion % 6 == 0) {
                    StageObj_K1C_CreateTwoPoint(o->pos, ofs, 4, 1.0f);
                }
            }
        } else if (faded >= total) {
            o->state = ATTACH_END;
        }
        break;
    }
}

/* A hit does not stop it: runs EmEffectAttack_Run. */
void EmEffectAttack_Hit(ObjAttach *o)
{
    EmEffectAttack_Run(o);
}

/* Frees the draw slots of o's parts (gDrawSlotsB ones when o->keep). */
void EmEffectAttack_FreePartSlots(ObjAttach *o)
{
    EmEffectAttackPart *part = (EmEffectAttackPart *) (o->buf + 0xC);
    s16 count = EmEffectAttack_PartCounts[o->type];
    s16 i;

    for (i = 0; i < count; i++) {
        if (part->draw != NULL) {
            if (o->keep != 0) {
                DrawSlotB_Free(part->slot);
            } else {
                DrawSlot_Free(part->slot);
            }
        }
        part++;
    }
}

/*
 * Frees the draw slots of the breath chain's links (gDrawSlotsB ones when
 * o->keep).
 */
void EmEffectAttack_FreeChainSlots(ObjAttach *o)
{
    EmEffectAttackLink *link = (EmEffectAttackLink *) o->buf;
    s16 count = EmEffectAttack_PartCounts[o->type];
    s16 i;

    for (i = 0; i < count; i++) {
        if (link->draw != NULL) {
            if (o->keep != 0) {
                DrawSlotB_Free(link->slot);
            } else {
                DrawSlot_Free(link->slot);
            }
        }
        link++;
    }
}

/*
 * Ends o: frees its own draw slot if it has one, and its parts' or links'
 * slots, then closes the hit window and sets the state to ATTACH_FREE.
 */
void EmEffectAttack_End(ObjAttach *o)
{
    if (o->drawSlotBuf != NULL) {
        if (o->keep != 0) {
            DrawSlotB_Free(o->drawSlot);
        } else {
            DrawSlot_Free(o->drawSlot);
        }
    }
    switch (o->type) {
    case EMFX_BREATH_CHAIN:
        EmEffectAttack_FreeChainSlots(o);
        break;
    default:
        EmEffectAttack_FreePartSlots(o);
        break;
    }
    o->hitPhase = 0;
    o->state = ATTACH_FREE;
    o->visible = 0;
}

/* Returns o to the pool (ObjAttach_Free). */
void EmEffectAttack_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/*
 * Draws link d->part of the breath chain. The material set comes from
 * EmEffectAttack_BreathChainModels by link index (bit 0x8000: from the
 * owner's part model); a fading link uses material set 1. The link is scaled
 * by EmEffectAttack_ChainGrowth and its size, and fainter further down the
 * chain. While following, the head turns along its direction with a scrolling
 * texture and the others face the camera with an animated texture cell; once
 * fading, they only face the camera.
 */
void EmEffectAttack_DrawChain(DrawSlot *d)
{
    f32 scaleVec[3];
    f32 rot[3];
    f32 origin[3];
    f32 mtx[16];
    f32 texMtx[16];
    EmEffectAttackLink *chainLink;
    ObjAttach *o;
    u16 *models;
    ModelMat *tex;
    u32 keyColor;
    ModelMatSet *model;
    u16 flags;
    u32 col;
    Model *mdl;
    s16 slot;
    s16 matSet;
    f32 growth;
    s32 cell;
    u8 alpha;

    o = d->owner;
    chainLink = &((EmEffectAttackChain *)o->buf)->link[d->part];
    mdl = gCommonModels[0];
    models = EmEffectAttack_PartModels[o->type];
    flags = 0;
    if (models[chainLink->index] & 0x8000) {
        slot = EmSlot_FindKind(o->aux);
        if (slot == -1) {
            return;
        }
        mdl = gScene.partModels[slot];
    }
    if (mdl == NULL) {
        return;
    }
    if (mdl->active == 0) {
        return;
    }
    if (chainLink->delay < 0) {
        matSet = models[chainLink->index] & ~0x8000;
    } else {
        matSet = 1;
    }
    model = &mdl->matSets[matSet];
    tex = mdl->mats;
    growth = EmEffectAttack_ChainGrowth(o);
    scaleVec[0] = growth * (chainLink->size[0] * chainLink->scale);
    scaleVec[1] = growth * (chainLink->size[1] * chainLink->scale);
    scaleVec[2] = growth * (chainLink->size[2] * chainLink->scale);
    rot[2] = (6.2831855f * chainLink->spin) / 65536.0f;
    Mtx_FromScaleRotTrans(scaleVec, rot, d->pos, 3, mtx);
    keyColor = chainLink->color;
    alpha = (u8) (keyColor >> 24) * (1.0f - 0.5f * (chainLink->index / 8.0f));
    col = alpha << 24 | (u8) (keyColor >> 16) << 16 | (u8) (keyColor >> 8) << 8 | (u8) keyColor;
    if (chainLink->delay < 0) {
        flags |= 2;
        switch (chainLink->index) {
        case 0:
            origin[0] = origin[1] = origin[2] = 0.0f;
            rot[0] = (6.2831855f * Vec_Elevation(chainLink->dir[0], origin)) / 65536.0f;
            rot[1] = (6.2831855f * ((u16) Vec_Heading(chainLink->dir[0], origin) + 0x4000)) / 65536.0f;
            Mtx_RotateX(mtx, rot[0]);
            Mtx_RotateY(mtx, rot[1]);
            Mtx_Translation(texMtx, 0.0f, (chainLink->age & 0xF) / 16.0f, 0.0f);
            break;
        default:
            Mtx_Concat3x3(mtx, gCamMatrix);
            cell = chainLink->age % 15;
            Mtx_Translation(texMtx, 0.25f * (cell & 3), 0.25f * (cell >> 2), 0.0f);
            break;
        }
        Gs_SetState(0x19, (s32) texMtx);
    } else {
        Mtx_Concat3x3(mtx, gCamMatrix);
    }
    Gs_SetState(0x6C, 0);
    Effect_DrawModel(model, mtx, col, flags, tex);
    Gs_SetState(0x6C, 1);
}

s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
void Mtx_Billboard(f32 *m, f32 *pos);
extern u16 EmEffectAttack_FatalisBlastVariantModels[];

/*
 * Draws part d->part of o when o is in the current area; the chain's links go
 * to EmEffectAttack_DrawChain. The material set comes from the type's
 * EmEffectAttack_PartModels (bit 0x8000: from the owner's part model), or
 * from EmEffectAttack_FatalisBlastVariantModels for the blasts' part kind 3.
 * By type and part kind, it sets the texture scroll, the rotation order and
 * the facing (mode). It draws the part at its scale, rotation and colour:
 * with Effect_DrawModel when sorted, otherwise with Effect_DrawModelAdd.
 */
void EmEffectAttack_DrawPart(DrawSlot *d)
{
    f32 mtx[16];
    f32 texMtx[16];
    f32 rot[3];
    f32 scaleVec[3];
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
    ObjAttach *o;
    EmEffectAttackPart *effPart;
    u16 *models;
    Model *mdl;
    ModelMat *tex;
    ModelMatSet *model;
    u32 col;
    s16 slot;
    s16 matSet;
    s16 frame;
    u16 flags;
    u16 order;
    u8 mode;

    mode = 1;
    flags = 0;
    order = 0;
    o = d->owner;
    effPart = (EmEffectAttackPart *) (o->buf + d->part * 0x3C + 0xC);
    if (o->area != gScene.area) {
        return;
    }
    if (o->type == EMFX_BREATH_CHAIN) {
        EmEffectAttack_DrawChain(d);
        return;
    }
    models = EmEffectAttack_PartModels[o->type];
    mdl = gCommonModels[0];
    if (models[effPart->type] & 0x8000) {
        slot = EmSlot_FindKind(o->aux);
        if (slot == -1) {
            return;
        }
        mdl = gScene.partModels[slot];
    }
    if (mdl == NULL) {
        return;
    }
    if (mdl->active == 0) {
        return;
    }
    if ((o->type == EMFX_FATALIS_SWEEP_BLAST && effPart->type == 3) || (o->type == EMFX_FATALIS_BLAST && effPart->type == 3)) {
        matSet = EmEffectAttack_FatalisBlastVariantModels[o->stateVar0] & ~0x8000;
    } else {
        matSet = models[effPart->type] & ~0x8000;
    }
    model = &mdl->matSets[matSet];
    col = effPart->color;
    tex = mdl->mats;
    switch (o->type) {
    case EMFX_YAMA_INHALE:
        switch (effPart->type) {
        case 0:
            Mtx_Translation(texMtx, 0.0f, (o->ownerMotion & 0x1F) / 32.0f, 0.0f);
            Gs_SetState(0x19, (s32) texMtx);
            order = 4;
            flags |= 2;
            mode = 0;
            break;
        case 1:
            flags |= 2;
            order = 7;
            mode = 0;
            break;
        case 2:
            order = 3;
            flags |= 2;
            break;
        case 3:
            order = 6;
            mode = 0;
            break;
        }
        break;
    case EMFX_CONGALALA_BREATH:
        switch (effPart->type) {
        case 0:
            Mtx_Translation(texMtx, 0.0f, (o->ownerMotion & 0x1FF) / 512.0f, 0.0f);
            order = 7;
            mode = 0;
            break;
        case 1:
            if (effPart->aux == 0) {
                Mtx_Translation(texMtx, 0.0f, 0.0f, 0.0f);
            } else {
                Mtx_Translation(texMtx, 0.5f, 0.75f, 0.0f);
            }
            order = 3;
            break;
        default:
            return;
        }
        Gs_SetState(0x19, (s32) texMtx);
        break;
    case EMFX_RAJANG_SHOT:
        switch (effPart->type) {
        case 0:
            order = 7;
            mode = 0;
            break;
        case 1:
            Mtx_Translation(texMtx, 0.0f, 0.1f * (o->ownerMotion % 10), 0.0f);
            Gs_SetState(0x19, (s32) texMtx);
            order = 7;
            mode = 0;
            break;
        case 2:
            order = 3;
            mode = 1;
            break;
        case 3:
            mode = 3;
            order = 4;
            break;
        default:
            return;
        }
        flags |= 2;
        break;
    case EMFX_SHEN_SHOT:
    case EMFX_SHEN_SHOT_AWAY:
    case EMFX_SHEN_SHOT_DROP:
        switch (effPart->type) {
        case 0:
            frame = effPart->frame;
            Mtx_Translation(texMtx, 0.125f * (frame % 4), 0.125f * ((frame & 7) >> 2), 0.0f);
            Gs_SetState(0x19, (s32) texMtx);
            order = 3;
            mode = 1;
            flags |= 2;
            break;
        case 1:
            order = 6;
            mode = 0;
            break;
        case 2:
            order = 1;
            mode = 0;
            break;
        default:
            return;
        }
        break;
    case EMFX_DAIMYO_JET:
        switch (effPart->type) {
        case 0:
            Mtx_Translation(texMtx, 0.0f, (o->ownerMotion & 0xF) / 16.0f, 0.0f);
            order = 7;
            mode = 0;
            break;
        case 1:
            Mtx_Identity(texMtx);
            order = 3;
            mode = 1;
            break;
        case 2:
        case 3:
        case 4:
            if ((u8) (col >> 24) != 0xFF) {
                Mtx_Translation(texMtx, 0.75f, 0.0f, 0.0f);
            } else {
                Mtx_Identity(texMtx);
            }
            break;
        default:
            return;
        }
        Gs_SetState(0x19, (s32) texMtx);
        break;
    case EMFX_CONGALALA_GAS:
        switch (effPart->type) {
        case 0:
            order = 2;
            mode = 0;
            Effect_GetColor(gScene.area, &red, &green, &blue, &alpha);
            col = blue | ((green << 8) | ((red << 16) | ((u8) ((alpha / 255.0f) * (u8) (effPart->color >> 24)) << 24)));
            break;
        case 1:
        case 2:
            if (effPart->aux == 0) {
                Mtx_Identity(texMtx);
            } else {
                Mtx_Translation(texMtx, 0.5f, 0.75f, 0.0f);
            }
            mode = 1;
            order = 3;
            break;
        default:
            return;
        }
        Gs_SetState(0x19, (s32) texMtx);
        break;
    case EMFX_CONGALALA_RAGE_GAS:
        if (effPart->aux == 0) {
            Mtx_Identity(texMtx);
        } else {
            Mtx_Translation(texMtx, 0.5f, 0.75f, 0.0f);
        }
        mode = 1;
        order = 3;
        Gs_SetState(0x19, (s32) texMtx);
        break;
    case EMFX_CHAMELEOS_BALL:
        mode = 1;
        order = 3;
        break;
    case EMFX_SHOGUN_JET:
        Mtx_Translation(texMtx, 0.0f, (o->ownerMotion & 7) / 8.0f, 0.0f);
        order = 4;
        mode = 3;
        Gs_SetState(0x19, (s32) texMtx);
        break;
    case EMFX_CHAMELEOS_BREATH:
        order = 7;
        mode = 0;
        Mtx_Translation(texMtx, 0.0f, (o->ownerMotion & 0x1F) / 32.0f, 0.0f);
        Gs_SetState(0x19, (s32) texMtx);
        break;
    case EMFX_FATALIS_SWEEP_BLAST:
    case EMFX_FATALIS_BLAST:
        switch (effPart->type) {
        case 0:
            flags |= 2;
            order = 3;
            mode = 2;
            break;
        case 1:
            flags |= 2;
            order = 3;
            mode = 1;
            break;
        case 2:
            flags |= 2;
            order = 2;
            mode = 0;
            break;
        case 3:
            flags |= 2;
            mode = 2;
            break;
        case 4:
            mode = 1;
            break;
        case 5:
            flags |= 2;
            order = 1;
            mode = 0;
            break;
        default:
            return;
        }
        Mtx_Identity(texMtx);
        Gs_SetState(0x19, (s32) texMtx);
        break;
    case EMFX_FATALIS_FIREBALL:
        switch (effPart->type) {
        case 0:
            order = 7;
            mode = 0;
            break;
        case 1:
        case 2:
            order = 7;
            mode = 0;
            Mtx_Translation(texMtx, 0.0f, 0.1875f * (effPart->frame / 30.0f), 0.0f);
            Gs_SetState(0x19, (s32) texMtx);
            break;
        case 3:
        case 4:
            order = 3;
            mode = 1;
            break;
        case 5:
            order = 4;
            mode = 3;
            break;
        default:
            return;
        }
        flags |= 2;
        break;
    }
    if ((u8) mode == 3) {
        scaleVec[0] = 1.0f;
        scaleVec[1] = 1.0f;
        scaleVec[2] = 1.0f;
    } else {
        scaleVec[0] = effPart->size[0] * effPart->scale;
        scaleVec[1] = effPart->size[1] * effPart->scale;
        scaleVec[2] = effPart->size[2] * effPart->scale;
    }
    rot[0] = (6.2831855f * effPart->rotX) / 65536.0f;
    rot[1] = (6.2831855f * effPart->rotY) / 65536.0f;
    rot[2] = (6.2831855f * effPart->rotZ) / 65536.0f;
    Mtx_FromScaleRotTrans(scaleVec, rot, d->pos, order, mtx);
    switch ((u8) mode) {
    case 0:
        break;
    case 1:
        Mtx_Concat3x3(mtx, gCamMatrix);
        break;
    case 3:
        Mtx_Billboard(mtx, d->pos);
        scaleVec[0] = effPart->size[0] * effPart->scale;
        scaleVec[1] = effPart->size[1] * effPart->scale;
        scaleVec[2] = effPart->size[2] * effPart->scale;
        mtx[0] *= scaleVec[0];
        mtx[1] *= scaleVec[0];
        mtx[2] *= scaleVec[0];
        mtx[4] *= scaleVec[1];
        mtx[5] *= scaleVec[1];
        mtx[6] *= scaleVec[1];
        mtx[8] *= scaleVec[2];
        mtx[9] *= scaleVec[2];
        mtx[10] *= scaleVec[2];
        break;
    }
    if (effPart->sorted == 0) {
        Gs_SetState(0x67, col);
        Effect_DrawModelAdd(model, mtx, tex);
        return;
    }
    Gs_SetState(0x6C, 0);
    Effect_DrawModel(model, mtx, col, flags, tex);
    Gs_SetState(0x6C, 1);
}

/*
 * A shot's impact at pos, if o is in the current area. By type: sounds, stage
 * effects, a camera shake, and an Explosion (Rajang's shot, the fireball) or
 * an EmImpactAttack (Chameleos's ball). sunk is 0 when the shot met the floor
 * and nonzero when it was found deep under it; then Shen Gaoren's and
 * Chameleos's shots skip part of the effect. Other types make no impact.
 */
void EmEffectAttack_Impact(ObjAttach *o, s16 sunk, f32 *pos)
{
    Em *em = o->owner;

    if (o->area == gScene.area) {
        switch (o->type) {
        default:
            return;
        case EMFX_RAJANG_SHOT:
            Em_PlaySeChecked(em, 0xF, 0, pos, 6, 0);
            StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x11, 1.0f);
            CamShake_SetA(4, pos);
            Explosion_CreateOwned((Player *)em, pos, 0x27);
            break;
        case EMFX_SHEN_SHOT:
        case EMFX_SHEN_SHOT_DROP:
            Em_PlaySeAtPos(em, 0x39, 0, o->pos, 0x18, 0);
            Sound_PlaySeAtPosChecked(1, 0x4F, 0, o->pos, 0x18, 0);
            StageObj_K0E_CreateScaled(pos, 1, 0.45000002f);
            StageObj_K11_CreateAtScaled(pos, o->rotY, 4, 0.45000002f);
            StageObj_K11_CreateAtScaled(pos, o->rotY, 9, 3.0f);
            switch (sunk) {
            case 0:
                StageObj_K11_CreateAtScaled(pos, o->rotY, 0xB, 3.0f);
                CamShake_SetA(5, pos);
                break;
            case 1:
                break;
            }
            break;
        case EMFX_CHAMELEOS_BALL:
            if (o->stateVar1 != 1) {
                Em_PlaySeChecked(em, 0x3B, 0, o->pos, 6, 0);
                StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x20, 1.0f);
                EmImpactAttack_CreateAt(em, pos, 7);
                switch (sunk) {
                case 0:
                    StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x21, 1.0f);
                    break;
                case 1:
                    break;
                }
            } else {
                Em_PlaySeChecked(em, 0x3E, 0, o->pos, 6, 0);
                StageObj_K14_CreateAt(pos, 0x1D, 0, 1.2f);
                EmImpactAttack_CreateAt(em, pos, 14);
            }
            break;
        case EMFX_FATALIS_FIREBALL:
            Em_PlaySeChecked(em, 0x25, 0, pos, 15, 0);
            StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x11, 1.0f);
            CamShake_SetA(4, pos);
            Explosion_CreateOwned((Player *)em, pos, 0x28);
            break;
        }
    }
}

/*
 * Restarts a part of Yama Tsukami's inhale at o->pos, which is the head bone
 * at this point (Run calls it before moving pos). Part kind 1 starts at a
 * random distance along the inhale's line. Kinds 2 to 4 start at random
 * places far ahead and Run draws them in toward the head.
 */
void EmEffectAttack_StartYamaInhalePart(ObjAttach *o, EmEffectAttackPart *p)
{
    Em *em = o->owner;
    f32 ofs[3];

    Vec_Copy(p->vec, o->pos);
    switch (p->type) {
    case 0:
        break;
    case 1:
        ofs[0] = 0.0f;
        ofs[1] = 0.0f;
        ofs[2] = em->scaleZ * ((Rand_Next(1) & 0x1FF) - 0x100);
        Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += ofs[0];
        p->vec[2] += ofs[2];
        p->rotZ = Rand_Next(1);
        p->spinZ = -(Rand_Next(1) & 0x3FF);
        break;
    case 2:
        ofs[0] = em->scaleX * ((Rand_Next(1) & 0x1FF) - 0x100);
        ofs[1] = em->scaleY * ((Rand_Next(1) & 0x1FF) - 0x100);
        ofs[2] = 2000.0f + em->scaleZ * ((Rand_Next(1) & 0x1FF) - 0x100);
        Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += ofs[0];
        p->vec[1] += ofs[1];
        p->vec[2] += ofs[2];
        p->rotZ = Rand_Next(1);
        p->spinZ = 0.5f + (65536.0f * ((20.0f + 20.0f * Rand_Unit()) / 30.0f)) / 360.0f;
        if (p->rotZ & 1) {
            p->spinZ = -p->spinZ;
        }
        break;
    case 3:
    case 4:
        ofs[0] = em->scaleX * ((Rand_Next(1) & 0x3FF) - 0x200);
        ofs[1] = em->scaleY * ((Rand_Next(1) & 0x3FF) - 0x200);
        ofs[2] = 2000.0f + em->scaleZ * ((Rand_Next(1) & 0x3FF) - 0x200);
        Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += ofs[0];
        p->vec[1] += ofs[1];
        p->vec[2] += ofs[2];
        p->scale = em->scaleX * (1.0f + Rand_Unit());
        p->rotX = Rand_Next(1);
        p->rotY = Rand_Next(1);
        p->rotZ = Rand_Next(1);
        break;
    }
}

/*
 * Restarts a part of Congalala's breath at o->pos. A kind 1 puff appears at a
 * random place about 230 units ahead with a random texture cell and spin, and
 * plays a sound.
 */
void EmEffectAttack_StartCongalalaBreathPart(ObjAttach *o, EmEffectAttackPart *p)
{
    Em *em = o->owner;
    f32 ofs[3];
    u16 bits;

    Vec_Copy(p->vec, o->pos);
    switch (p->type) {
    case 0:
        break;
    case 1:
        ofs[0] = em->scaleX * ((Rand_Next(1) & 0x7F) - 0x40);
        ofs[1] = em->scaleY * ((Rand_Next(1) & 0x3F) - 0x20);
        ofs[2] = 230.0f + em->scaleZ * ((Rand_Next(1) & 0x7F) - 0x40);
        Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += ofs[0];
        p->vec[1] += ofs[1];
        p->vec[2] += ofs[2];
        bits = Rand_Next(1);
        p->aux = bits & 1;
        p->rotZ = Rand_Next(1);
        p->spinZ = 0x5B;
        if ((bits >> 1) & 1) {
            p->spinZ = -p->spinZ;
        }
        Em_PlaySeChecked(em, 0x1A, 0, p->vec, 1, 0);
        break;
    }
}

/*
 * Restarts a part of Daimyo Hermitaur's water jet at o->pos2. Kinds 1 to 4
 * are drops at a random place about 900 units along the jet, at 1.5 times the
 * monster's scale, thrown up (speed) to fall in Run. Each turns 15 degrees
 * over its kind's frames (EmEffectAttack_DaimyoJetFrames).
 */
void EmEffectAttack_StartDaimyoJetDrop(ObjAttach *o, EmEffectAttackPart *p)
{
    Em *em = o->owner;
    f32 ofs[3];

    Vec_Copy(p->vec, o->pos2);
    switch (p->type) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        p->scale = 1.5f * em->scaleX;
        ofs[0] = em->scaleX * ((Rand_Next(1) & 0xFF) - 0x80);
        ofs[1] = em->scaleY * ((Rand_Next(1) & 0xFF) - 0x80);
        ofs[2] = 900.0f + em->scaleZ * ((Rand_Next(1) & 0xFF) - 0x80);
        Vec_RotateX(ofs, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += ofs[0];
        p->vec[1] += ofs[1];
        p->vec[2] += ofs[2];
        p->rotZ = Rand_Next(1);
        p->spinZ = 0.5f + (65536.0f * (15.0f / EmEffectAttack_DaimyoJetFrames[p->type])) / 360.0f;
        if (Rand_Next(1) & 1) {
            p->spinZ = -p->spinZ;
        }
        p->speed = 2.0f;
        break;
    }
}

/*
 * Gives a part of Congalala's gas the next entry (stateVar0) of the puff
 * schedule. The kind comes from EmEffectAttack_CongalalaGasKinds (0xFF: none
 * left, so the part stays off), and the frame from the start time in
 * EmEffectAttack_CongalalaGasStarts. Kind 1 and 2 puffs head out behind the
 * monster with some spread.
 */
void EmEffectAttack_StartCongalalaGasPuff(ObjAttach *o, EmEffectAttackPart *p)
{
    u16 *frames = EmEffectAttack_CongalalaGasStarts;
    u8 *types = EmEffectAttack_CongalalaGasKinds;

    if (types[o->stateVar0] == 0xFF) {
        p->type = 0xFF;
        return;
    }
    p->type = types[o->stateVar0];
    p->frame = o->ownerMotion - frames[o->stateVar0];
    switch (p->type) {
    case 1:
        p->rotY = o->rotY + 0x8000 + (o->stateVar0 % 3 - 1) * 0x18E4 + ((Rand_Next(1) & 0x1FFF) - 0x1000);
        break;
    case 2:
        p->rotY = o->rotY + 0x8000 + (((o->stateVar0 & 1) - 1) << 13) + (Rand_Next(1) & 0x1FFF);
        break;
    default:
        p->rotY = o->rotY;
        break;
    }
    o->stateVar0++;
}

/*
 * Places a gas part (type 6) at the type's bone plus offset. Kind 0 keeps its
 * height (the floor) at scale 1. Kinds 1 and 2 get a random spin and texture
 * cell and an outward speed, at scale 1 or 0.5.
 */
void EmEffectAttack_PlaceCongalalaGasPuff(ObjAttach *o, EmEffectAttackPart *p)
{
    f32 y = p->vec[1];
    void *bone = Em_Bone(o->owner, EmEffectAttack_Bones[o->type]);
    f32 ofs[3];

    ofs[0] = EmEffectAttack_BoneOfs[o->type * 3];
    ofs[1] = EmEffectAttack_BoneOfsY[o->type * 3];
    ofs[2] = EmEffectAttack_BoneOfsZ[o->type * 3];
    Vec_RotateSelf(ofs, bone);
    Mtx_GetTrans(p->vec, bone);
    Vec_Add(p->vec, p->vec, ofs);
    switch (p->type) {
    case 0:
        p->curveXY = 0;
        p->vec[1] = y;
        p->scale = 1.0f;
        break;
    case 1:
        p->curveXY = 1;
        p->rotZ = Rand_Next(1);
        p->spinZ = 0x5B;
        p->aux = Rand_Next(1) & 1;
        p->speed = 20.0f * p->scale;
        p->scale = 1.0f;
        break;
    case 2:
        p->curveXY = 1;
        p->rotZ = Rand_Next(1);
        p->spinZ = 0x5B;
        p->aux = Rand_Next(1) & 1;
        p->scale = 0.5f;
        p->speed = 15.0f * p->scale;
        break;
    }
}

/*
 * Restarts a part of Chameleos's ball at a random place within 40 units of
 * the ball (times its scale), with a random spin.
 */
void EmEffectAttack_StartChameleosBallPart(ObjAttach *o, EmEffectAttackPart *p)
{
    p->rotZ = Rand_Next(1);
    p->spinZ = 0x71C;
    if (Rand_Next(1) & 1) {
        p->spinZ = -p->spinZ;
    }
    p->vec[0] = p->scale * (40.0f * Rand_Signed());
    p->vec[1] = p->scale * (40.0f * Rand_Signed());
    p->vec[2] = p->scale * (40.0f * Rand_Signed());
}

/*
 * Puts a stream (types 0, 1, 4, 10, 11) at its bone: pos and pos2 go to the
 * type's bone plus offset, and rotY becomes the bone's heading plus the yaw
 * offset ang[2]. Type 10 also sets rotX to the bone's pitch plus ang[1]; type
 * 0 moves pos2 1000 units ahead (times the monster's z scale). ang is the
 * buffer's header.
 */
void EmEffectAttack_PlaceAtBone(ObjAttach *o, u16 *ang)
{
    f32 *ofsY;
    f32 *ofsZ;
    f32 *fwdZ;
    f32 *fwdX;
    f32 ofs[3];
    f32 mtx[16];

    Mtx_Copy(mtx, (f32 *)Em_GetBone(o->owner, EmEffectAttack_Bones[o->type]));
    ofsY = &ofs[1];
    ofsZ = &ofs[2];
    ofs[0] = EmEffectAttack_BoneOfs[o->type * 3];
    *ofsY = EmEffectAttack_BoneOfsY[o->type * 3];
    *ofsZ = EmEffectAttack_BoneOfsZ[o->type * 3];
    Vec_RotateSelf(ofs, mtx);
    o->pos[0] = mtx[12] + ofs[0];
    o->pos[1] = mtx[13] + *ofsY;
    o->pos[2] = mtx[14] + *ofsZ;
    Vec_Copy(o->pos2, o->pos);
    fwdZ = &mtx[10];
    fwdX = &mtx[8];
    o->rotY = ang[2] + (u16) (s32) (0.5f + (65536.0f * Math_Atan2(*fwdX, *fwdZ)) / 6.2831855f);
    switch (o->type) {
    case EMFX_YAMA_INHALE:
        ofs[0] = 0.0f;
        *ofsY = 0.0f;
        *ofsZ = 1000.0f * ((Em *) o->owner)->scaleZ;
        Vec_RotateY(ofs, (6.2831855f * o->rotY) / 65536.0f);
        o->pos2[0] += ofs[0];
        o->pos2[1] += *ofsY;
        o->pos2[2] += *ofsZ;
        break;
    case EMFX_SHOGUN_JET:
        o->rotX = ang[1] + (u16) (s32) (0.5f + (65536.0f * Math_Atan2(mtx[9], Math_Sqrtf(*fwdX * *fwdX + *fwdZ * *fwdZ))) / 6.2831855f);
        break;
    }
}

/*
 * Places link e of the breath chain. The head (index 0) sits at the bone and
 * sets dir, the breath's direction (the bone's heading pitched by rotX), and
 * puts its from[0] 150 units out along it. The other links lie len along dir
 * from the link before: 300 times EmEffectAttack_ChainGrowth for the second,
 * growing to 100 over 15 frames for the rest. following is 0 at set up (both
 * from/dir entries are filled) and 1 afterwards, when links after the second
 * use last frame's entry (a one-frame lag).
 */
void EmEffectAttack_UpdateLink(ObjAttach *o, EmEffectAttackLink *link, EmEffectAttackLink *prev, void *bone, f32 *dir, s16 following)
{
    f32 axis[3];
    u16 pitch;
    u16 yaw;
    f32 len;
    s16 i;
    EmEffectAttackLink *hist;

    switch (link->index) {
    case 0:
        Mtx_GetTrans(link->pos, bone);
        Vec_Copy(axis, (f32 *) ((u8 *) bone + 0x20));
        Vec_DirToAngles(axis, (s16 *) &pitch, (s16 *) &yaw);
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 1.0f;
        Vec_RotateX(dir, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(dir, (6.2831855f * yaw) / 65536.0f);
        link->from[0][0] = link->pos[0] + 150.0f * dir[0];
        link->from[0][1] = link->pos[1] + 150.0f * dir[1];
        link->from[0][2] = link->pos[2] + 150.0f * dir[2];
        return;
    case 1:
        len = 300.0f * EmEffectAttack_ChainGrowth(o);
        break;
    default:
        if (o->ownerMotion < 0) {
            len = 0.0f;
        } else if (o->ownerMotion < 15) {
            len = 100.0f * (o->ownerMotion / 15.0f);
        } else {
            len = 100.0f;
        }
        break;
    }
    if (following == 0) {
        for (i = 0, hist = link; i < 2; i++, hist = (EmEffectAttackLink *)((u8 *)hist + 0xC)) {
            Vec_Copy(hist->dir[0], dir);
            Vec_Copy(hist->from[0], prev->pos);
        }
    } else {
        if (link->index == 1) {
            Vec_Copy(link->dir[0], dir);
            Vec_Copy(link->from[0], prev->pos);
        } else {
            hist = (EmEffectAttackLink *)((u8 *)link + 0xC);
            Vec_Copy(link->dir[0], hist->dir[0]);
            Vec_Copy(link->from[0], hist->from[0]);
        }
        Vec_Copy(link->dir[1], dir);
        Vec_Copy(link->from[1], prev->pos);
    }
    link->pos[0] = link->from[0][0] + link->dir[0][0] * len;
    link->pos[1] = link->from[0][1] + link->dir[0][1] * len;
    link->pos[2] = link->from[0][2] + link->dir[0][2] * len;
}

/* Gives a link a random roll and roll speed as it starts to fade. */
void EmEffectAttack_RandLinkSpin(EmEffectAttackLink *link)
{
    link->spin = Rand_Next(1);
    link->spinSpeed = (s16) ((Rand_Next(1) & 0xFFFF & 0x1FF) - 0x100);
}

/*
 * Returns the breath chain's growth by age (ownerMotion): 0.2 before it
 * starts, rising to 1.0 over the first 15 frames. It scales the links
 * (EmEffectAttack_DrawChain) and the second link's distance; it is not an
 * alpha.
 */
f32 EmEffectAttack_ChainGrowth(ObjAttach *o)
{
    f32 growth;
    s16 age;

    age = o->ownerMotion;
    if (age < 0) {
        return 0.2f;
    }
    if (age < 0xF) {
        growth = 0.2f + ((0.8f * (f32) age) / 15.0f);
    } else {
        growth = 1.0f;
    }
    return growth;
}

/* unk_6E3B00's .data (0x8AB600-0x8AD3A0) */
HitShape EmEffectAttack_HitShapes_0[2] = {
#include "game/008AB600.inc"
};
HitShape EmEffectAttack_HitShapes_1[2] = {
#include "game/008AB650.inc"
};
HitShape EmEffectAttack_HitShapes_2[2] = {
#include "game/008AB6A0.inc"
};
HitShape EmEffectAttack_HitShapes_3[2] = {
#include "game/008AB6F0.inc"
};
HitShape EmEffectAttack_HitShapes_4[2] = {
#include "game/008AB740.inc"
};
HitShape EmEffectAttack_HitShapes_5[2] = {
#include "game/008AB790.inc"
};
HitShape EmEffectAttack_HitShapes_6[2] = {
#include "game/008AB7E0.inc"
};
HitShape EmEffectAttack_HitShapes_7[3] = {
#include "game/008AB830.inc"
};
HitShape EmEffectAttack_HitShapes_8[3] = {
#include "game/008AB8B0.inc"
};
HitShape EmEffectAttack_HitShapes_9[2] = {
#include "game/008AB930.inc"
};
HitShape EmEffectAttack_HitShapes_10[3] = {
#include "game/008AB980.inc"
};
HitShape EmEffectAttack_HitShapes_11[2] = {
#include "game/008ABA00.inc"
};
HitShape *EmEffectAttack_HitShapes[12] = {
    EmEffectAttack_HitShapes_0, EmEffectAttack_HitShapes_1, EmEffectAttack_HitShapes_2, EmEffectAttack_HitShapes_3,
    EmEffectAttack_HitShapes_4, EmEffectAttack_HitShapes_5, EmEffectAttack_HitShapes_6, EmEffectAttack_HitShapes_7,
    EmEffectAttack_HitShapes_8, EmEffectAttack_HitShapes_9, EmEffectAttack_HitShapes_10, EmEffectAttack_HitShapes_11,
};
u32 EmEffectAttack_Rows[132] = {
#include "game/008ABA80.inc"
};
void *EmEffectAttack_RowsPtr = (void *)EmEffectAttack_Rows;
u32 EmEffectAttack_ScaleKeys_0[16] = {
#include "game/008ABCA0.inc"
};
u32 EmEffectAttack_ScaleKeys_1[12] = {
#include "game/008ABCE0.inc"
};
u32 EmEffectAttack_ScaleKeys_2[12] = {
#include "game/008ABD10.inc"
};
u32 EmEffectAttack_ScaleKeys_3[12] = {
#include "game/008ABD40.inc"
};
u32 EmEffectAttack_ColorKeys_0[8] = {
#include "game/008ABD70.inc"
};
u32 EmEffectAttack_ColorKeys_1[8] = {
#include "game/008ABD90.inc"
};
u32 EmEffectAttack_ColorKeys_2[12] = {
#include "game/008ABDB0.inc"
};
u32 EmEffectAttack_ColorKeys_3[8] = {
#include "game/008ABDE0.inc"
};
s16 EmEffectAttack_YamaInhaleDelays[8] = {
#include "game/008ABE00.inc"
};
u16 EmEffectAttack_PartModels_0[8] = {
#include "game/008ABE10.inc"
};
u32 EmEffectAttack_PartFrames_0[4] = {
#include "game/008ABE20.inc"
};
u32 EmEffectAttack_ScaleKeys_5[16] = {
#include "game/008ABE30.inc"
};
u32 EmEffectAttack_ScaleKeys_6[12] = {
#include "game/008ABE70.inc"
};
u32 EmEffectAttack_ColorKeys_5[12] = {
#include "game/008ABEA0.inc"
};
u32 EmEffectAttack_ColorKeys_6[8] = {
#include "game/008ABED0.inc"
};
u32 EmEffectAttack_ColorKeys_7[12] = {
#include "game/008ABEF0.inc"
};
u32 EmEffectAttack_ColorKeys_8[8] = {
#include "game/008ABF20.inc"
};
u32 EmEffectAttack_ColorKeys_9[12] = {
#include "game/008ABF40.inc"
};
u32 EmEffectAttack_ColorKeys_10[8] = {
#include "game/008ABF70.inc"
};
u32 EmEffectAttack_ColorKeys_11[12] = {
#include "game/008ABF90.inc"
};
u32 EmEffectAttack_ColorKeys_12[14] = {
#include "game/008ABFC0.inc"
};
u16 EmEffectAttack_PartModels_1[4] = {
#include "game/008ABFF8.inc"
};
u32 EmEffectAttack_PartFrames_1[4] = {
#include "game/008AC000.inc"
};
u32 EmEffectAttack_ScaleKeys_7[16] = {
#include "game/008AC010.inc"
};
u32 EmEffectAttack_ScaleKeys_8[12] = {
#include "game/008AC050.inc"
};
u32 EmEffectAttack_ScaleKeys_9[8] = {
#include "game/008AC080.inc"
};
u32 EmEffectAttack_ScaleKeys_10[8] = {
#include "game/008AC0A0.inc"
};
u32 EmEffectAttack_ColorKeys_13[8] = {
#include "game/008AC0C0.inc"
};
u32 EmEffectAttack_ColorKeys_14[8] = {
#include "game/008AC0E0.inc"
};
u32 EmEffectAttack_ColorKeys_15[8] = {
#include "game/008AC100.inc"
};
u32 EmEffectAttack_ColorKeys_16[8] = {
#include "game/008AC120.inc"
};
u16 EmEffectAttack_PartModels_2[4] = {
#include "game/008AC140.inc"
};
u32 EmEffectAttack_PartFrames_2[2] = {
#include "game/008AC148.inc"
};
u32 EmEffectAttack_ScaleKeys_11[12] = {
#include "game/008AC150.inc"
};
u32 EmEffectAttack_ScaleKeys_12[16] = {
#include "game/008AC180.inc"
};
u32 EmEffectAttack_ColorKeys_17[8] = {
#include "game/008AC1C0.inc"
};
u16 EmEffectAttack_PartModels_3[4] = {
#include "game/008AC1E0.inc"
};
u32 EmEffectAttack_PartFrames_3[2] = {
#include "game/008AC1E8.inc"
};
u32 EmEffectAttack_ScaleKeys_14[16] = {
#include "game/008AC1F0.inc"
};
u32 EmEffectAttack_ScaleKeys_15[12] = {
#include "game/008AC230.inc"
};
u32 EmEffectAttack_ColorKeys_20[8] = {
#include "game/008AC260.inc"
};
u32 EmEffectAttack_ColorKeys_21[12] = {
#include "game/008AC280.inc"
};
u32 EmEffectAttack_ColorKeys_22[8] = {
#include "game/008AC2B0.inc"
};
u16 EmEffectAttack_PartModels_4[8] = {
#include "game/008AC2D0.inc"
};
s16 EmEffectAttack_DaimyoJetFrames[8] = {
#include "game/008AC2E0.inc"
};
s16 EmEffectAttack_DaimyoJetDelays[8] = {
#include "game/008AC2F0.inc"
};
u32 EmEffectAttack_ScaleKeys_19[16] = {
#include "game/008AC300.inc"
};
u32 EmEffectAttack_ScaleKeys_20[12] = {
#include "game/008AC340.inc"
};
u32 EmEffectAttack_ColorKeys_25[8] = {
#include "game/008AC370.inc"
};
u32 EmEffectAttack_ColorKeys_26[10] = {
#include "game/008AC390.inc"
};
u16 EmEffectAttack_PartModels_6[4] = {
#include "game/008AC3B8.inc"
};
u32 EmEffectAttack_PartFrames_6[2] = {
#include "game/008AC3C0.inc"
};
u8 EmEffectAttack_CongalalaGasKinds[12] = {
#include "game/008AC3C8.inc"
};
u16 EmEffectAttack_CongalalaGasStarts[16] = {
#include "game/008AC3E0.inc"
};
u32 EmEffectAttack_ScaleKeys_22[12] = {
#include "game/008AC400.inc"
};
u32 EmEffectAttack_ColorKeys_28[10] = {
#include "game/008AC430.inc"
};
u16 EmEffectAttack_PartModels_7[4] = {
#include "game/008AC458.inc"
};
u32 EmEffectAttack_PartFrames_7[4] = {
#include "game/008AC460.inc"
};
f32 EmEffectAttack_ShenDropSpotsFortress5[4][3] = {
#include "game/008AC470.inc"
};
f32 EmEffectAttack_ShenDropSpotsTown2[4][3] = {
#include "game/008AC4A0.inc"
};
f32 EmEffectAttack_ShenDropSpotsTown3[4][3] = {
#include "game/008AC4D0.inc"
};
f32 EmEffectAttack_ShenDropSpotsTown0[4][3] = {
#include "game/008AC500.inc"
};
u32 EmEffectAttack_ScaleKeys_13[12] = {
#include "game/008AC530.inc"
};
u32 EmEffectAttack_ColorKeys_19[8] = {
#include "game/008AC560.inc"
};
u32 EmEffectAttack_ScaleKeys_23[12] = {
#include "game/008AC580.inc"
};
u32 EmEffectAttack_ColorKeys_29[8] = {
#include "game/008AC5B0.inc"
};
u32 EmEffectAttack_ColorKeys_30[8] = {
#include "game/008AC5D0.inc"
};
u16 EmEffectAttack_PartModels_9[4] = {
#include "game/008AC5F0.inc"
};
u32 EmEffectAttack_PartFrames_9[2] = {
#include "game/008AC5F8.inc"
};
u32 EmEffectAttack_ScaleKeys_24[16] = {
#include "game/008AC600.inc"
};
u32 EmEffectAttack_ColorKeys_31[8] = {
#include "game/008AC640.inc"
};
u16 EmEffectAttack_PartModels_10[4] = {
#include "game/008AC660.inc"
};
u32 EmEffectAttack_PartFrames_10[2] = {
#include "game/008AC668.inc"
};
u32 EmEffectAttack_ScaleKeys_25[20] = {
#include "game/008AC670.inc"
};
u32 EmEffectAttack_ColorKeys_32[8] = {
#include "game/008AC6C0.inc"
};
u16 EmEffectAttack_PartModels_11[4] = {
#include "game/008AC6E0.inc"
};
u32 EmEffectAttack_PartFrames_11[2] = {
#include "game/008AC6E8.inc"
};
u32 EmEffectAttack_ScaleKeys_26[12] = {
#include "game/008AC6F0.inc"
};
u32 EmEffectAttack_ScaleKeys_27[16] = {
#include "game/008AC720.inc"
};
u32 EmEffectAttack_ScaleKeys_28[24] = {
#include "game/008AC760.inc"
};
u32 EmEffectAttack_ScaleKeys_29[16] = {
#include "game/008AC7C0.inc"
};
u32 EmEffectAttack_ScaleKeys_30[16] = {
#include "game/008AC800.inc"
};
u32 EmEffectAttack_ScaleKeys_31[8] = {
#include "game/008AC840.inc"
};
u32 EmEffectAttack_ColorKeys_33[8] = {
#include "game/008AC860.inc"
};
u32 EmEffectAttack_ColorKeys_34[8] = {
#include "game/008AC880.inc"
};
u32 EmEffectAttack_ColorKeys_35[12] = {
#include "game/008AC8A0.inc"
};
u32 EmEffectAttack_ColorKeys_36[12] = {
#include "game/008AC8D0.inc"
};
u32 EmEffectAttack_ColorKeys_37[8] = {
#include "game/008AC900.inc"
};
u32 EmEffectAttack_ColorKeys_38[10] = {
#include "game/008AC920.inc"
};
u16 EmEffectAttack_PartModels_12[6] = {
#include "game/008AC948.inc"
};
u16 EmEffectAttack_FatalisBlastVariantRolls[4] = {
#include "game/008AC958.inc"
};
u16 EmEffectAttack_FatalisBlastVariantModels[4] = {
#include "game/008AC960.inc"
};
s16 EmEffectAttack_FatalisBlastFrames[6] = {
#include "game/008AC968.inc"
};
f32 EmEffectAttack_FatalisSweepSpots[40][3] = {
#include "game/008AC980.inc"
};
u32 EmEffectAttack_ScaleKeys_32[16] = {
#include "game/008ACB60.inc"
};
u32 EmEffectAttack_ScaleKeys_33[12] = {
#include "game/008ACBA0.inc"
};
u32 EmEffectAttack_ScaleKeys_34[12] = {
#include "game/008ACBD0.inc"
};
u32 EmEffectAttack_ScaleKeys_35[8] = {
#include "game/008ACC00.inc"
};
u32 EmEffectAttack_ScaleKeys_36[8] = {
#include "game/008ACC20.inc"
};
u32 EmEffectAttack_ScaleKeys_37[12] = {
#include "game/008ACC40.inc"
};
u32 EmEffectAttack_ColorKeys_39[8] = {
#include "game/008ACC70.inc"
};
u32 EmEffectAttack_ColorKeys_40[12] = {
#include "game/008ACC90.inc"
};
u32 EmEffectAttack_ColorKeys_41[12] = {
#include "game/008ACCC0.inc"
};
u32 EmEffectAttack_ColorKeys_42[8] = {
#include "game/008ACCF0.inc"
};
u32 EmEffectAttack_ColorKeys_43[8] = {
#include "game/008ACD10.inc"
};
u32 EmEffectAttack_ColorKeys_44[8] = {
#include "game/008ACD30.inc"
};
u16 EmEffectAttack_PartModels_13[8] = {
#include "game/008ACD50.inc"
};
u32 EmEffectAttack_PartFrames_13[4] = {
#include "game/008ACD60.inc"
};
f32 EmEffectAttack_FatalisBlastSpots[40][3] = {
#include "game/008ACD70.inc"
};
u32 EmEffectAttack_ScaleKeys_38[12] = {
#include "game/008ACF50.inc"
};
u32 EmEffectAttack_ColorKeys_45[12] = {
#include "game/008ACF80.inc"
};
u16 EmEffectAttack_PartModels_15[8] = {
#include "game/008ACFB0.inc"
};
u32 EmEffectAttack_PartFrames_15[4] = {
#include "game/008ACFC0.inc"
};
void *EmEffectAttack_ScaleKeys[40] = {
    (void *)EmEffectAttack_ScaleKeys_0, (void *)EmEffectAttack_ScaleKeys_1, (void *)EmEffectAttack_ScaleKeys_2, (void *)EmEffectAttack_ScaleKeys_3,
    (void *)EmEffectAttack_ScaleKeys_3, (void *)EmEffectAttack_ScaleKeys_5, (void *)EmEffectAttack_ScaleKeys_6, (void *)EmEffectAttack_ScaleKeys_7,
    (void *)EmEffectAttack_ScaleKeys_8, (void *)EmEffectAttack_ScaleKeys_9, (void *)EmEffectAttack_ScaleKeys_10, (void *)EmEffectAttack_ScaleKeys_11,
    (void *)EmEffectAttack_ScaleKeys_12, (void *)EmEffectAttack_ScaleKeys_13, (void *)EmEffectAttack_ScaleKeys_14, (void *)EmEffectAttack_ScaleKeys_15,
    (void *)EmEffectAttack_ScaleKeys_15, (void *)EmEffectAttack_ScaleKeys_15, (void *)EmEffectAttack_ScaleKeys_15, (void *)EmEffectAttack_ScaleKeys_19,
    (void *)EmEffectAttack_ScaleKeys_20, (void *)EmEffectAttack_ScaleKeys_20, (void *)EmEffectAttack_ScaleKeys_22, (void *)EmEffectAttack_ScaleKeys_23,
    (void *)EmEffectAttack_ScaleKeys_24, (void *)EmEffectAttack_ScaleKeys_25, (void *)EmEffectAttack_ScaleKeys_26, (void *)EmEffectAttack_ScaleKeys_27,
    (void *)EmEffectAttack_ScaleKeys_28, (void *)EmEffectAttack_ScaleKeys_29, (void *)EmEffectAttack_ScaleKeys_30, (void *)EmEffectAttack_ScaleKeys_31,
    (void *)EmEffectAttack_ScaleKeys_32, (void *)EmEffectAttack_ScaleKeys_33, (void *)EmEffectAttack_ScaleKeys_34, (void *)EmEffectAttack_ScaleKeys_35,
    (void *)EmEffectAttack_ScaleKeys_36, (void *)EmEffectAttack_ScaleKeys_37, (void *)EmEffectAttack_ScaleKeys_38, NULL,
};
void *EmEffectAttack_ColorKeys[48] = {
    (void *)EmEffectAttack_ColorKeys_0, (void *)EmEffectAttack_ColorKeys_1, (void *)EmEffectAttack_ColorKeys_2, (void *)EmEffectAttack_ColorKeys_3,
    (void *)EmEffectAttack_ColorKeys_3, (void *)EmEffectAttack_ColorKeys_5, (void *)EmEffectAttack_ColorKeys_6, (void *)EmEffectAttack_ColorKeys_7,
    (void *)EmEffectAttack_ColorKeys_8, (void *)EmEffectAttack_ColorKeys_9, (void *)EmEffectAttack_ColorKeys_10, (void *)EmEffectAttack_ColorKeys_11,
    (void *)EmEffectAttack_ColorKeys_12, (void *)EmEffectAttack_ColorKeys_13, (void *)EmEffectAttack_ColorKeys_14, (void *)EmEffectAttack_ColorKeys_15,
    (void *)EmEffectAttack_ColorKeys_16, (void *)EmEffectAttack_ColorKeys_17, (void *)EmEffectAttack_ColorKeys_17, (void *)EmEffectAttack_ColorKeys_19,
    (void *)EmEffectAttack_ColorKeys_20, (void *)EmEffectAttack_ColorKeys_21, (void *)EmEffectAttack_ColorKeys_22, (void *)EmEffectAttack_ColorKeys_22,
    (void *)EmEffectAttack_ColorKeys_22, (void *)EmEffectAttack_ColorKeys_25, (void *)EmEffectAttack_ColorKeys_26, (void *)EmEffectAttack_ColorKeys_26,
    (void *)EmEffectAttack_ColorKeys_28, (void *)EmEffectAttack_ColorKeys_29, (void *)EmEffectAttack_ColorKeys_30, (void *)EmEffectAttack_ColorKeys_31,
    (void *)EmEffectAttack_ColorKeys_32, (void *)EmEffectAttack_ColorKeys_33, (void *)EmEffectAttack_ColorKeys_34, (void *)EmEffectAttack_ColorKeys_35,
    (void *)EmEffectAttack_ColorKeys_36, (void *)EmEffectAttack_ColorKeys_37, (void *)EmEffectAttack_ColorKeys_38, (void *)EmEffectAttack_ColorKeys_39,
    (void *)EmEffectAttack_ColorKeys_40, (void *)EmEffectAttack_ColorKeys_41, (void *)EmEffectAttack_ColorKeys_42, (void *)EmEffectAttack_ColorKeys_43,
    (void *)EmEffectAttack_ColorKeys_44, (void *)EmEffectAttack_ColorKeys_45, NULL, NULL,
};
s16 EmEffectAttack_PartCounts[16] = {
#include "game/008AD130.inc"
};
s16 EmEffectAttack_Lives[16] = {
#include "game/008AD150.inc"
};
s16 EmEffectAttack_ScaleKeyBase[16] = {
#include "game/008AD170.inc"
};
s16 EmEffectAttack_ColorKeyBase[16] = {
#include "game/008AD190.inc"
};
u16 *EmEffectAttack_PartModels[16] = {
    EmEffectAttack_PartModels_0, EmEffectAttack_PartModels_1, EmEffectAttack_PartModels_2, EmEffectAttack_PartModels_3,
    EmEffectAttack_PartModels_4, EmEffectAttack_PartModels_3, EmEffectAttack_PartModels_6, EmEffectAttack_PartModels_7,
    EmEffectAttack_PartModels_3, EmEffectAttack_PartModels_9, EmEffectAttack_PartModels_10, EmEffectAttack_PartModels_11,
    EmEffectAttack_PartModels_12, EmEffectAttack_PartModels_13, EmEffectAttack_PartModels_12, EmEffectAttack_PartModels_15,
};
u32 EmEffectAttack_PartFrames[16] = {
    (u32)EmEffectAttack_PartFrames_0, (u32)EmEffectAttack_PartFrames_1, (u32)EmEffectAttack_PartFrames_2, (u32)EmEffectAttack_PartFrames_3,
    (u32)EmEffectAttack_DaimyoJetFrames, (u32)EmEffectAttack_PartFrames_3, (u32)EmEffectAttack_PartFrames_6, (u32)EmEffectAttack_PartFrames_7,
    (u32)EmEffectAttack_PartFrames_3, (u32)EmEffectAttack_PartFrames_9, (u32)EmEffectAttack_PartFrames_10, (u32)EmEffectAttack_PartFrames_11,
    (u32)EmEffectAttack_FatalisBlastFrames, (u32)EmEffectAttack_PartFrames_13, (u32)EmEffectAttack_FatalisBlastFrames, (u32)EmEffectAttack_PartFrames_15,
};
s16 EmEffectAttack_MotionBase[16] = {
#include "game/008AD230.inc"
};
f32 EmEffectAttack_Speeds[16] = {
#include "game/008AD250.inc"
};
f32 EmEffectAttack_Gravities[16] = {
#include "game/008AD290.inc"
};
u8 EmEffectAttack_Bones[16] = {
#include "game/008AD2D0.inc"
};
f32 EmEffectAttack_BoneOfs[48] = {
#include "game/008AD2E0.inc"
};
