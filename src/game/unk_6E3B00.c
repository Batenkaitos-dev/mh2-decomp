/*
 * Game overlay code at 0x6E3B00-0x6EA220: the EffEm33_* effect objects
 * (EffEm33_Init to EffEm33_ChainAlpha), one unit: its .data is 0x8AB600-0x8AD3A0
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

/* Built at optimization level 4: EffEm33_DrawPart matches only there, every
   other matched function at both levels (EffKushala30_Run before the unit
   and EffKushala30b_Run after it need level 3). */
#pragma optimization_level 4

/* prototypes of callees (round 13) */
void DrawSlotB_Free(s32);
/* prototypes */
extern s16 EffEm33_CurveIdx[];
extern s16 EffEm33_ColorIdx[];
extern void *EffEm33_Curves[];
extern void *EffEm33_ColorKeys[];
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
/* A link of the chain (kind 0x21 type 0xF, 0x64 bytes: o->buf holds eight of them,
 * EffEm33Chain): each follows the one before it (EffEm33_UpdateLink), the first the
 * owner's bone; then they drift up and fade (EffEm33_RunChain). */
typedef struct EffEm33Link {
    s16 slot;                   /* 0x00: its draw slot (-1 none) */
    s16 index;                  /* 0x02: its place in the chain (0 at the bone) */
    f32 pos[3];                 /* 0x04 */
    s16 age;                    /* 0x10: frames (from 8 - index); the texture scroll (EffEm33_DrawChain) */
    s16 delay;                  /* 0x12: counts up from -1 - index; 0 starts the link's fade, drawn to 29 */
    u8 unk14[0x18 - 0x14];
    u16 spin;                   /* 0x18: its roll (EffEm33_RandLinkSpin) */
    s16 spinSpeed;              /* 0x1A */
    f32 scale;                  /* 0x1C: 1 + 0.2 * index */
    DrawSlot *draw;             /* 0x20 */
    f32 size[3];                /* 0x24: the scale per axis (EffEm33_Curves) */
    u32 color;                  /* 0x30: (EffEm33_ColorKeys) */
    f32 from[2][3];             /* 0x34: where the link starts (the link before's pos) */
    f32 dir[2][3];              /* 0x4C: and its direction */
} EffEm33Link;
typedef char EffEm33Link_size_check[(sizeof(EffEm33Link) == 0x64) ? 1 : -1];

typedef struct EffEm33Chain {
    EffEm33Link link[8];        /* 0x000 */
    s16 timer;                  /* 0x320: frames since the owner let go (38: the chain ends) */
    u16 headAlive;              /* 0x322: the first link still follows the owner's bone */
} EffEm33Chain;
typedef char EffEm33Chain_size_check[(sizeof(EffEm33Chain) == 0x324) ? 1 : -1];

void EffEm33_RandLinkSpin(EffEm33Link *e);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Mtx_Copy(f32 *dst, f32 *src);
f32 Math_Atan2(f32 y, f32 x);
s32 Em_PlaySeAtPos(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void CamShake_SetA(s32 kind, f32 *pos);
/* unit's own declaration: with the definition's void (f32 *pos, s32 v, s16 k, f32 scale) (game/unk_874B90.c), EffEm33_Impact: 3 of 193 words differ */
void StageObj_K11_CreateAtScaled(f32 *pos, u16 ang, s32 kind, f32 scale);
void StageObj_K14_CreateAt(f32 *pos, u8 k, u8 c, f32 scale);
void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale);
void EffEm07_CreateAt(Em *em, f32 *pos, s8 kind);
extern s16 EffEm33_PartVal2CA[];
extern s16 D_008ABFE8[];
extern s16 EffEm33_PartVal2CB[];
extern u16 EffEm33_PartVal32[];
extern s16 EffEm33_PartVal2CC[];
u16 Vec_Heading(f32 *a, f32 *b);
u16 Vec_Elevation(f32 *a, f32 *b);
extern u16 *EffEm33_DrawTbls[];
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
s32 Effect_DrawModel(void *mdl, f32 *a1, s32 color, u16 flags, void *a4);
extern f32 gCamMatrix[];
s16 EmSlot_FindKind(s16 k);
u16 Area_DayId(u16 id);
void EffEm33_InitParts(ObjAttach *o);
extern HitShape *EffEm33_Vals088[];
extern f32 EffEm33_Val050ByType[];
extern f32 EffEm33_Val058ByType[];
extern f32 EffEm33_Offsets[][3];
extern f32 EffEm33_StartPos[][3];
extern f32 EffEm33_TblA[][3];
extern f32 EffEm33_TblB[][3];
extern f32 EffEm33_TblC[][3];
extern f32 EffEm33_TblD[][3];
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
f32 EffEm33_ChainAlpha(ObjAttach *o);
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
void EffEm33_DrawPart(DrawSlot *d);
EmBone *Em_GetBone(void *obj, s16 bone);
extern u8 EffEm33_SeqTypes[];
extern u16 EffEm33_SeqFrames[];
f32 Lure_RandUnit(void);
void Vec_RotateX(f32 *v, f32 angle);
extern s16 EffEm33_PartPeriods[];
void Vec_RotateY(f32 *v, f32 angle);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *, f32 *, f32 *);
extern u8 EffEm33_Bones[];
extern f32 EffEm33_BoneOfs[];
extern f32 D_008AD2E4[];
extern f32 D_008AD2E8[];
StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
void Eff09_CreateOwned(Player *owner, f32 *pos, s32 kind);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
extern s16 EffEm33_MotionBase[];
extern void *EffEm33_MotionDesc;
typedef struct EffPart { s16 slot; u8 type; u8 unk03; f32 vec[3]; u32 color; f32 size[3]; f32 scale; DrawSlot *draw; f32 unk28; s16 unk2C; u16 unk2E; u16 unk30; u16 unk32; s16 unk34; s16 unk36; s16 unk38; u8 unk3A; u8 unk3B; } EffPart;
void EffEm33_StartPart(ObjAttach *o, EffPart *p);
f32 Lure_RandSigned(void);
u16 Area_IdForCurrentTime(u16);
ObjAttach *ObjAttach_Alloc(s32 n);
void ObjAttach_Free(ObjAttach *p);
u16 Rand_Next(s32 stream);
void EffEm33_Run(ObjAttach *o);
void EffEm33_Hit(ObjAttach *o);
void EffEm33_End(ObjAttach *o);
void EffEm33_Free(ObjAttach *o);
void DrawSlot_Free(s32);
extern s16 EffEm33_PartCounts[];
void EffEm33_Update(ObjAttach *o);
struct DrawSlot;
s32 OT_AddByDepth(u32 *table, struct DrawSlot *node, u32 n, s32 clamp);
void Vec_Copy(f32 *out, f32 *in);
u8 Player_InCurrentArea(void *obj);
s32 Player_TestSetFlags(void *, s32 mask);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
f32 Math_Absf(f32 x);
void *EffEm33_Create(Em *em, s32 kind, s32 c, s32 a, s32 b);
void EffEm33_UpdateLink(ObjAttach *o, EffEm33Link *e, EffEm33Link *prev, void *bone, f32 *dir, s16 f);
void EffEm33_Setup(ObjAttach *o);

static void EffEm33_Init(ObjAttach *o, Player *owner, s32 kind, s32 c)
{
    o->kind = 0x21;
    o->type = kind;
    o->stateVar1 = c;
    o->update = EffEm33_Update;
    o->owner = owner;
    o->isEm = owner->isEm;
    o->idx = owner->index;
    o->side = owner->side;
    o->yaw = owner->rot[1];
}

static void EffEm33_CallInit(ObjAttach *o, Em *owner, s32 kind, s32 c)
{
    u8 *p = o->buf;

    EffEm33_Init(o, (Player *) owner, kind, c);
    *(s16 *) (p + 6) = -1;
}

void *EffEm33_Create(Em *em, s32 kind, s32 c, s32 a, s32 b)
{
    ObjAttach *o;

    if (!(Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    if ((o = ObjAttach_Alloc(1)) != NULL) {
        EffEm33_CallInit(o, em, kind & 0xFF, c & 0xFF);
        o->aux = em->type;
        o->rotX = (u16) a;
        o->rotY = (u16) b + em->rotY;
        o->randBits = em->stateRand;
        o->area = em->area;
        return o;
    }
    return NULL;
}

void EffEm33_CreateShengaoren(Em *em, s32 arg1, u16 area, s32 arg3, s32 arg4)
{
    ObjAttach *o;

    if ((o = ObjAttach_Alloc(1)) != NULL) {
        EffEm33_CallInit(o, em, arg1 & 0xFF, 0);
        o->aux = em->type;
        o->rotX = (s32) (arg3 & 0xFFFF);
        o->rotY = (s32) (arg4 & 0xFFFF);
        o->randBits = em->stateRand;
        o->keep = 1;
        o->area = Area_IdForCurrentTime(area);
    }
}

void EffEm33_CreateV(Em *em, s32 a1, s32 a2, s32 a3, s32 a4, s16 v)
{
    ObjAttach *o = EffEm33_Create(em, a1, a2, a3, a4);

    if (o != NULL) {
        *(s16 *) (o->buf + 6) = v;
    }
}

void *EffEm33_CreatePlus18(Em *em, s16 kind, s16 arg2, s32 arg3, s32 d)
{
    return EffEm33_Create(em, kind, (s16) (arg2 + 0x18), arg3, d);
}

void EffEm33_CreateB(Em *em, s32 kind, s32 c, s32 a, s32 b)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(2)) != NULL) {
            EffEm33_Init(o, (Player *) em, kind & 0xFF, c & 0xFF);
            o->aux = em->type;
            o->rotX = (u16) a;
            o->rotY = (u16) b + em->rotY;
            o->randBits = em->stateRand;
            o->area = em->area;
        }
    }
}

void EffEm33_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffEm33_Setup(o);
        break;
    case 1:
        EffEm33_Run(o);
        break;
    case 2:
        EffEm33_End(o);
        break;
    case 3:
        EffEm33_Hit(o);
        break;
    case 4:
        EffEm33_End(o);
        break;
    case 5:
        EffEm33_End(o);
        break;
    case 9:
        EffEm33_End(o);
        break;
    case 7:
        EffEm33_End(o);
        break;
    case 6:
        EffEm33_Free(o);
        
    default:
        break;
    }
}

void EffEm33_StartMotion(ObjAttach *o, s32 owner)
{
    s16 n;
    u8 kind = o->type;

    switch (kind) {
    case 12:
    case 14:
        n = 0;
        break;
    default:
        n = o->stateVar1;
        break;
    }
    MoveObj_StartMotionOwned((struct MoveObj *)o, owner, n + EffEm33_MotionBase[kind], &EffEm33_MotionDesc);
}

void EffEm33_InitParts(ObjAttach *o)
{
    u8 *b = o->buf;
    EffPart *p = (EffPart *) (b + 0xC);
    Em *m = o->owner;
    s16 n;
    s16 i;
    f32 x;
    f32 y;
    f32 z;
    f32 (*tbl)[3];
    u16 r;

    switch (o->type) {
    case 0:
    case 1:
    case 4:
    case 0xB:
        *(s16 *) (b + 4) = o->rotY - m->rotY;
        *(u16 *) b = m->motionId[0];
        break;
    case 6:
        o->yaw = o->rotY + 0x8000;
        if (o->aux == 0x3E) {
            o->stateVar0 = 1;
        }
        break;
    case 7:
        Em_PlaySeChecked(m, 0x25, 0, o->pos, 6, 0);
        Em_PlaySeChecked(m, 0x18, 0, o->pos, 6, 0);
        break;
    case 0xA:
        *(s16 *) (b + 2) = o->rotX;
        *(s16 *) (b + 4) = o->rotY - m->rotY;
        *(u16 *) b = m->motionId[0];
        *(f32 *) (b + 8) = m->floorY;
        Em_PlaySeChecked(m, 0x3A, 0, o->pos2, 0x18, 0);
        break;
    case 0xC:
        if (o->area == gScene.area) {
            Em_PlaySeChecked(m, 0x42, 0, o->pos, 15, 0);
        }
        break;
    case 0xE:
        o->hitDelay += 30;
        if (o->area == gScene.area) {
            Em_PlaySeChecked(m, 0x42, 0, o->pos, 15, 0);
        }
        break;
    }
    n = EffEm33_PartCounts[o->type];
    for (i = 0; i < n; i++, p++) {
        p->unk2C = 0;
        p->type = i;
        p->unk3B = 0;
        p->scale = m->scaleX;
        p->unk03 = 1;
        p->unk3A = 1;
        p->unk2E = 0;
        p->unk30 = 0;
        p->unk32 = 0;
        p->unk34 = 0;
        p->unk36 = 0;
        p->unk38 = 0;
        switch (o->type) {
        case 0:
            p->unk2C = -EffEm33_PartVal2CA[p->type];
            switch (i) {
            case 0:
            default:
                p->type = 0;
                p->unk03 = 0;
                break;
            case 1:
            case 2:
                p->type = 1;
                p->unk03 = 0;
                break;
            case 3:
            case 4:
            case 5:
                p->type = 2;
                break;
            case 6:
            case 7:
                p->type = 3;
                p->unk03 = 0;
                p->unk3A = 0;
                break;
            }
            break;
        case 1:
            p->unk2C = -D_008ABFE8[i];
            if (i <= 0) {
                p->type = 0;
                p->unk03 = 0;
                p->unk32 = Rand_Next(1);
                p->unk38 = 0x444;
            } else {
                p->type = 1;
            }
            break;
        case 2:
            Vec_Copy(p->vec, o->pos);
            p->unk2E = o->rotX;
            p->unk30 = o->rotY;
            switch (p->type) {
            case 0:
                p->unk03 = 0;
                p->unk32 = Rand_Next(1);
                p->unk38 = 0x1000;
                break;
            case 1:
                p->unk03 = 0;
                p->unk32 = Rand_Next(1);
                break;
            case 2:
            case 3:
                break;
            }
            break;
        case 3:
        case 5:
        case 8:
            p->scale *= 5.0f;
            switch (i) {
            case 0:
                p->type = 1;
                p->unk3B = 1;
                p->unk34 = 0x1222;
                p->unk36 = 0x234;
                p->unk38 = 0;
                break;
            case 1:
                p->scale *= 0.9f;
                p->unk3B = 0;
                p->unk34 = -0xACE;
                p->unk36 = -0x40D;
                p->unk38 = 0;
                break;
            case 2:
                p->type = 0;
                break;
            case 3:
                p->type = 2;
                p->unk2E = 0x4000;
                switch (Area_DayId(o->area)) {
                case 0xC:
                default:
                    tbl = EffEm33_TblA;
                    break;
                case 0xB1:
                    tbl = EffEm33_TblB;
                    break;
                case 0xB2:
                    tbl = EffEm33_TblC;
                    break;
                case 0xB3:
                    tbl = EffEm33_TblD;
                    break;
                }
                p->vec[0] = tbl[o->stateVar0][0];
                p->vec[1] = 20.0f + tbl[o->stateVar0][1];
                p->vec[2] = tbl[o->stateVar0][2];
                break;
            }
            switch (p->type) {
            case 0:
                p->unk32 = Rand_Next(1);
                break;
            case 1:
            default:
                r = Rand_Next(1);
                p->unk2E = r & 0xF000;
                p->unk30 = (r << 4) & 0xF000;
                p->unk32 = (r << 8) & 0xF000;
                p->unk03 = 0;
                break;
            case 2:
                break;
            }
            break;
        case 4:
            p->unk2C = -EffEm33_PartVal2CB[i];
            if (i <= 0) {
                Vec_Copy(p->vec, o->pos);
                p->unk2E = o->rotX;
                p->unk30 = o->rotY;
                p->unk32 = Rand_Next(1);
                p->unk38 = 0xE39;
                p->unk03 = 0;
                p->unk3B = 0;
                p->type = 0;
            } else if (i <= 4) {
                p->type = 1;
            } else {
                p->type = Rand_Next(1) % 3 + 1;
            }
            break;
        case 6:
            EffEm33_StartPart(o, p);
            p->vec[1] = m->floorY;
            break;
        case 7:
            p->type = 0;
            Vec_Copy(p->vec, o->pos);
            p->unk2C = -4;
            x = EffEm33_BoneOfs[o->type * 3];
            y = EffEm33_BoneOfs[o->type * 3 + 1];
            z = EffEm33_BoneOfs[o->type * 3 + 2];
            x += (Rand_Next(1) & 0x3F) - 0x20;
            y += (Rand_Next(1) & 0x1F) - 0x10;
            x += (Rand_Next(1) & 0x3F) - 0x20;
            p->vec[0] += x;
            p->vec[1] += y;
            p->vec[2] += z;
            p->unk32 = Rand_Next(1);
            p->unk38 = 0x5B;
            r = Rand_Next(1);
            if (r & 1) {
                p->unk38 = -p->unk38;
            }
            p->unk3B = (r >> 1) & 1;
            break;
        case 9:
            p->type = 0;
            p->unk2C = -i * 2;
            break;
        case 10:
            p->unk3B = 0;
            p->unk03 = 0;
            break;
        case 11:
            p->unk3B = 0;
            p->unk32 = Rand_Next(1);
            p->unk38 = 0xB61;
            p->unk03 = 0;
            break;
        case 12:
        case 14:
            Vec_Copy(p->vec, o->pos);
            if (o->type == 0xC) {
                p->scale *= 1.5f;
                if (o->stateVar1 <= 23) {
                    p->unk2C -= 30;
                }
            } else {
                p->scale *= 2.0f;
                p->unk2C -= 30;
            }
            switch (p->type) {
            case 0:
                p->unk32 = EffEm33_PartVal32[o->stateVar0];
                break;
            case 1:
                p->unk32 = Rand_Next(1);
                break;
            case 2:
                p->unk03 = 0;
                p->unk30 = Rand_Next(1);
                p->vec[1] += 5.0f * p->scale;
                break;
            case 3:
                break;
            case 4:
                p->unk32 = Rand_Next(1);
                break;
            case 5:
                if (o->type == 0xC) {
                    if (o->stateVar1 <= 23) {
                        p->unk2C = 0;
                    } else {
                        p->unk2C = EffEm33_PartVal2CC[p->type];
                    }
                } else {
                    p->unk2C = 0;
                }
                p->unk2E = 0x4000;
                p->vec[1] += 5.0f;
                break;
            }
            break;
        case 13:
            Vec_Copy(p->vec, o->pos);
            p->scale = 1.8f;
            p->unk2E = o->rotX;
            p->unk30 = o->rotY;
            switch (p->type) {
            case 0:
                p->unk03 = 0;
                p->unk32 = Rand_Next(1);
                p->unk38 = 0x1000;
                break;
            case 1:
                p->unk03 = 0;
                p->unk32 = Rand_Next(1);
                p->unk38 = 0x71C;
                break;
            case 2:
                p->unk2C = -15;
                p->unk03 = 0;
                p->unk32 = Rand_Next(1);
                p->unk38 = 0x71C;
                break;
            case 3:
            case 4:
                break;
            case 5:
                p->unk03 = 0;
                break;
            }
            break;
        }
        if (o->keep) {
            p->slot = DrawSlotB_Alloc();
        } else {
            p->slot = DrawSlot_Alloc();
        }
        if (p->slot != -1) {
            if (o->keep) {
                p->draw = DrawSlotB_Get(p->slot);
            } else {
                p->draw = DrawSlot_Get(p->slot);
            }
            p->draw->owner = o;
            p->draw->part = i;
            p->draw->draw = EffEm33_DrawPart;
        } else {
            p->draw = NULL;
        }
    }
}

void EffEm33_InitChain(ObjAttach *o)
{
    EffEm33Link *e;
    EffEm33Link *prev;
    EffEm33Link *b;
    void *bone;
    s16 n;
    s16 i;

    e = (EffEm33Link *)o->buf;
    n = EffEm33_PartCounts[o->type];
    b = e;
    bone = Em_Bone(o->owner, EffEm33_Bones[o->type]);

    /* b is the first link: a copy of e of e's type (an EffEm33Chain *b = (EffEm33Chain *)e
     * lets MWCC store headAlive through e) */
    ((EffEm33Chain *)b)->headAlive = 1;
    for (i = 0; i < n; i++, e++) {
        e->age = 0;
        e->index = i;
        e->scale = 1.0f;
        EffEm33_UpdateLink(o, e, prev, bone, b->dir[0], 0);
        e->age = 8 - i;
        e->delay = -1 - i;
        e->spin = Rand_Next(1);
        if (e->index == 0) {
            e->spinSpeed = 0xB6;
        } else {
            e->spinSpeed = 0;
        }
        e->scale += 0.2f * i;
        e->size[0] = 1.0f;
        e->size[1] = 1.0f;
        e->size[2] = 1.0f;
        e->color = -1;
        prev = e;
        if (o->keep) {
            e->slot = DrawSlotB_Alloc();
        } else {
            e->slot = DrawSlot_Alloc();
        }
        if (e->slot != -1) {
            if (o->keep) {
                e->draw = DrawSlotB_Get(e->slot);
            } else {
                e->draw = DrawSlot_Get(e->slot);
            }
            e->draw->owner = o;
            e->draw->part = i;
            e->draw->draw = EffEm33_DrawPart;
        } else {
            e->draw = NULL;
        }
    }
}

void EffEm33_Setup(ObjAttach *o)
{
    f32 off[3];
    Em *m = o->owner;
    void *bone;
    f32 (*tbl)[3];
    s16 n;

    o->state = 1;
    o->visible = 1;
    o->draw = NULL;
    o->step = 0;
    o->stateVar0 = 0;
    o->ownerMotion = 0;
    switch (o->type) {
    case 0:
    case 1:
    case 6:
    case 7:
        Player_TestSetFlags(o, 0);
        break;
    default:
        Player_TestSetFlags(o, 0x200);
        break;
    }
    o->wasHit = 0;
    o->velX = 0.0f;
    o->velY = 0.0f;
    o->velZ = EffEm33_Val050ByType[o->type];
    Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
    Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
    o->homingTurn = 0.0f;
    o->gravity = EffEm33_Val058ByType[o->type];
    o->homingSpeed = 0.0f;
    if (EffEm33_Bones[o->type] != 0xFF) {
        bone = Em_Bone(m, EffEm33_Bones[o->type]);
        off[0] = EffEm33_BoneOfs[o->type * 3];
        off[1] = EffEm33_BoneOfs[o->type * 3 + 1];
        off[2] = EffEm33_BoneOfs[o->type * 3 + 2];
        Vec_RotateSelf(off, bone);
        Mtx_GetTrans(o->pos, bone);
        Vec_Add(o->pos, o->pos, off);
    } else {
        switch (o->type) {
        case 8:
            switch (Area_DayId(o->area)) {
            case 0xC:
            default:
                n = 3;
                tbl = EffEm33_TblA;
                break;
            case 0xB1:
                n = 4;
                tbl = EffEm33_TblB;
                break;
            case 0xB2:
                n = 4;
                tbl = EffEm33_TblC;
                break;
            case 0xB3:
                n = 4;
                tbl = EffEm33_TblD;
                break;
            }
            o->stateVar0 = o->randBits % n;
            o->pos[0] = tbl[o->stateVar0][0] - 60.0f * o->velX;
            o->pos[1] = tbl[o->stateVar0][1] - (60.0f * o->velY + 60.0f * (60.0f * (0.5f * o->gravity)));
            o->pos[2] = tbl[o->stateVar0][2] - 60.0f * o->velZ;
            break;
        case 12:
            o->stateVar0 = Rand_Next(1) % 3;
            Vec_Copy(off, EffEm33_Offsets[o->stateVar1]);
            Vec_RotateY(off, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = m->posX + off[0];
            o->pos[1] = m->posY + off[1];
            o->pos[2] = m->posZ + off[2];
            break;
        case 14:
            o->stateVar0 = Rand_Next(1) % 3;
            Vec_Copy(o->pos, EffEm33_StartPos[o->stateVar1]);
            break;
        }
    }
    Vec_Copy(o->pos2, o->pos);
    o->drawSlotBuf = NULL;
    switch (o->type) {
    case 0xF:
        EffEm33_InitChain(o);
        break;
    default:
        EffEm33_InitParts(o);
        break;
    }
    EffEm33_StartMotion(o, (s32) m);
    o->hitList = EffEm33_Vals088[o->hitListNo];
    o->hurtList = 0;
    switch (o->type) {
    case 12:
        if (o->stateVar1 <= 23) {
            o->hitDelay += 30;
        }
        break;
    case 14:
        o->hitDelay += 30;
        break;
    }
    EffEm33_Run(o);
}

/* The owner's bone 0x25, through an inlined helper: its return value is a
 * temporary with a 16-byte stack slot, where bone is spilled (sp+0xB0; a
 * plain `bone = Em_Bone(m, 0x25)` gets a 4-byte slot at sp+0xBC). */
static inline void *EffEm33_Bone(Em *m)
{
    return Em_Bone(m, 0x25);
}

s32 EffEm33_RunChain(ObjAttach *o)
{
    f32 off[3];
    EffEm33Chain *b = (EffEm33Chain *)o->buf;
    EffEm33Link *e = b->link;
    Em *m = o->owner;
    s16 n = EffEm33_PartCounts[o->type];
    s16 ci = EffEm33_CurveIdx[o->type];
    s16 ai = EffEm33_ColorIdx[o->type];
    u8 moved = 0;
    EffEm33Link *prev;
    void *bone;
    s16 i;

    switch (o->stateVar0) {
    case 0:
        if (m->active == 0 || m->taskPhase == 3 || (m->motionId[0] != 0x417 && m->motionId[0] != 0x41A) || o->ownerMotion >= 300) {
            b->timer = 0;
            Vec_Copy(b->link[0].pos, b->link[0].from[0]);
            o->stateVar0++;
            o->hitPhase = 0;
        } else {
            bone = EffEm33_Bone(m);
            off[0] = EffEm33_BoneOfs[o->type * 3];
            off[1] = EffEm33_BoneOfs[o->type * 3 + 1];
            off[2] = EffEm33_BoneOfs[o->type * 3 + 2];
            Vec_RotateSelf(off, bone);
            Mtx_GetTrans(o->pos2, bone);
            Vec_Add(o->pos2, o->pos2, off);
        }
        break;
    case 1:
        if (++b->timer >= 38) {
            return 1;
        }
        break;
    }
    for (i = 0; i < n; i++, e++) {
        (e->age)++;
        switch (o->stateVar0) {
        case 0:
            EffEm33_UpdateLink(o, e, prev, bone, b->link[0].dir[0], 1);
            e->spin += (s32) e->spinSpeed;
            prev = e;
            Vec_Copy(o->pos, e->pos);
            moved = 1;
            break;
        case 1:
        default:
            if (++e->delay == 0) {
                EffEm33_RandLinkSpin(e);
            }
            if (e->delay > 29) {
                continue;
            }
            if (e->delay >= 0) {
                Key_LerpVec2(EffEm33_Curves[ci], e->size, (f32) e->delay);
                Color_EvalKeys(EffEm33_ColorKeys[ai], (f32) e->delay, &e->color);
                e->pos[1] += 2.0f;
            }
            break;
        case 2:
            if (++e->delay == 0) {
                if (b->headAlive != 0) {
                    if (m->active != 0 || m->taskPhase != 3) {
                        Em_BonePos(m, 0x25, e->pos);
                    } else {
                        b->headAlive = 0;
                        e->delay = 30;
                    }
                } else {
                    e->delay = 30;
                }
            }
            if (e->delay > 29) {
                continue;
            }
            if (e->delay <= 0) {
                continue;
            }
            Key_LerpVec2(EffEm33_Curves[ci], e->size, (f32) e->delay);
            Color_EvalKeys(EffEm33_ColorKeys[ai], (f32) e->delay, &e->color);
            e->pos[1] += 2.0f;
            break;
        }
        if (e->draw != NULL) {
            Vec_Copy(e->draw->pos, e->pos);
            OT_AddByDepth(gDrawOtSorted, e->draw, 0x40U, 0);
        }
    }
    if (moved != 0 && o->ownerMotion % 10 == 0) {
        Em_PlaySeChecked(m, 0x42, 0, o->pos, 6, 0);
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
void EffEm33_Impact(ObjAttach *o, s16 n, f32 *pos);
void EffEm33_PlaceAtBone(ObjAttach *o, u16 *ang);
void EffEm33_StartPartType0(ObjAttach *o, EffPart *p);
void EffEm33_StartPartType1(ObjAttach *o, EffPart *p);
void EffEm33_StartPartType4(ObjAttach *o, EffPart *p);
void EffEm33_PartAtBone(ObjAttach *o, EffPart *p);
void EffEm33_StartPartType9(ObjAttach *o, EffPart *p);
s32 EffEm33_RunChain(ObjAttach *o);
extern s16 EffEm33_Lives[];
extern u32 EffEm33_RunTbls[];

void EffEm33_Run(ObjAttach *o)
{
    f32 v[3];
    f32 q[3];
    u8 *b = o->buf;
    Em *m = o->owner;
    EffPart *p = (EffPart *) (b + 0xC);
    s16 *tbl;
    s16 n;
    s32 k;
    s16 ai;
    u8 flag;
    s16 lim;
    s16 done;
    s16 i;
    s16 ci;
    f32 f;
    f32 floor;
    f32 t;
    f32 (*tab)[3];
    s32 cnt;
    s32 life;

    n = EffEm33_PartCounts[o->type];
    life = EffEm33_Lives[o->type];
    tbl = (s16 *) EffEm33_RunTbls[o->type];
    o->ownerMotion++;
    if (life != 1000 && life < o->ownerMotion) {
        o->hitFrames = 0;
        o->hitPhase = 0;
        o->state = 2;
        return;
    }
    o->hitFrames = 99;
    switch (o->type) {
    default:
        o->pos2[0] = o->pos[0];
        o->pos2[1] = o->pos[1];
        o->pos2[2] = o->pos[2];
        break;
    case 0:
    case 1:
    case 4:
    case 10:
    case 11:
        break;
    case 8:
        if (o->ownerMotion <= 0x8C) {
            return;
        }
        o->pos2[0] = o->pos[0];
        o->pos2[1] = o->pos[1];
        o->pos2[2] = o->pos[2];
        break;
    case 15:
        if ((u8) EffEm33_RunChain(o) == 1) {
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        }
        return;
    }
    switch (o->type) {
    case 0:
        if (m->active == 0 || m->taskPhase == 3 || *(s16 *) b != m->motionId[0]) {
            o->hitPhase = 0;
            o->hitFrames = 0;
            o->state = 2;
            return;
        }
        EffEm33_PlaceAtBone(o, (u16 *) b);
        o->yaw = o->rotY;
        break;
    case 1:
        switch (o->stateVar0) {
        case 0:
            if (m->active == 0 || m->taskPhase == 3 || *(s16 *) b != m->motionId[0] || EmMotion_ReachedFrame(m, 0, 235.0f) != 0) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                o->stateVar0 = 0xFF;
                done = 0;
            } else {
                EffEm33_PlaceAtBone(o, (u16 *) b);
                o->yaw = o->rotY;
            }
            break;
        case 0xFF:
            done = 0;
            break;
        }
        break;
    case 2:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
            Em_PlaySeChecked(o->owner, 0xE, 0, o->pos, 0x18, 0);
        }
        break;
    case 3:
    case 5:
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
                Em_PlaySeAtPos(m, 0x38, 0, o->pos, 0x18, 0);
            }
        }
        break;
    case 4:
        done = 0;
        if (o->stateVar0 != 0xFF) {
            if (o->ownerMotion >= 0x26 || m->active == 0 || m->taskPhase == 3) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                o->stateVar0 = 0xFF;
            } else {
                EffEm33_PlaceAtBone(o, (u16 *) b);
            }
        }
        break;
    case 6:
        if (m->active == 0 || m->taskPhase == 3) {
            o->hitPhase = 0;
            o->hitFrames = 0;
            o->state = 2;
            return;
        }
        if (o->ownerMotion >= 0x24) {
            o->hitPhase = 0;
        }
        break;
    case 7:
        if (o->ownerMotion >= 0x2A) {
            o->hitPhase = 0;
        }
        break;
    case 8:
        MoveObj_Integrate((struct MoveObj *)o);
        Vec_DirToAngles(&o->velX, (s16 *) &o->rotX, (s16 *) &o->rotY);
        if (o->area == gScene.area && o->ownerMotion < life - 5 && o->ownerMotion >= 4 && o->ownerMotion % 3 == 2) {
            StageObj_K1C_CreateAtAngled(o->pos, o->rotX, o->rotY, 0x1B, 1.0f);
        }
        break;
    case 9:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area && o->ownerMotion > 0 && (o->ownerMotion & 1) == 0) {
            StageObj_K1C_CreateAt(o->pos, 0x1F, 1.0f);
        }
        break;
    case 10:
        switch (o->stateVar0) {
        case 0:
            flag = 0;
            if (m->active == 0 || m->taskPhase == 3 || *(s16 *) b != m->motionId[0]) {
                flag = 1;
            } else if (*(s16 *) (b + 6) < 0) {
                if (EmMotion_ReachedFrame(m, 0, 168.0f) != 0) {
                    flag = 1;
                }
            } else if (--*(s16 *) (b + 6) <= 0) {
                flag = 1;
            }
            if (flag & 0xFF) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                done = 0;
                o->stateVar0 = 0xFF;
            } else {
                EffEm33_PlaceAtBone(o, (u16 *) b);
                o->yaw = o->rotY;
            }
            break;
        case 0xFF:
            done = 0;
            break;
        }
        break;
    case 11:
        switch (o->stateVar0) {
        case 0:
            if (m->active == 0 || m->taskPhase == 3 || *(s16 *) b != m->motionId[0] || --*(s16 *) (b + 6) <= 0) {
                o->hitPhase = 0;
                o->hitFrames = 0;
                o->stateVar0 = 0xFF;
                done = 0;
            } else {
                EffEm33_PlaceAtBone(o, (u16 *) b);
            }
            break;
        case 0xFF:
            done = 0;
            break;
        }
        break;
    case 12:
    case 14:
        break;
    case 13:
        MoveObj_Integrate((struct MoveObj *)o);
        if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
            Em_PlaySeChecked(o->owner, 0x38, 0, o->pos, 0x18, 0);
        }
        break;
    default:
        MoveObj_Integrate((struct MoveObj *)o);
        break;
    }
    cnt = n;
    for (i = 0; i < cnt; i++, p++) {
        if (tbl != NULL) {
            lim = tbl[p->type];
        }
        switch (o->type) {
        case 1:
            if (o->stateVar0 == 0xFF) {
                switch (p->type) {
                case 0:
                    done++;
                    continue;
                }
            }
            ci = p->type + EffEm33_CurveIdx[o->type];
            ai = p->type + (EffEm33_ColorIdx[o->type] + o->stateVar1 * 2);
            break;
        case 4:
            ci = p->type + EffEm33_CurveIdx[o->type];
            ai = p->type + EffEm33_ColorIdx[o->type];
            break;
        case 6:
            if (p->type == 0xFF) {
                continue;
            }
            ci = p->type + EffEm33_CurveIdx[o->type];
            ai = p->type + EffEm33_ColorIdx[o->type];
            break;
        case 9:
            ci = p->type + EffEm33_CurveIdx[o->type];
            ai = o->stateVar1 + EffEm33_ColorIdx[o->type];
            break;
        default:
            ci = p->type + EffEm33_CurveIdx[o->type];
            ai = p->type + EffEm33_ColorIdx[o->type];
            break;
        }
        if (++p->unk2C > lim) {
            switch (o->type) {
            case 0:
                switch (p->type) {
                case 2:
                    p->unk2C = -30;
                    break;
                default:
                    p->unk2C = 1;
                    break;
                }
                break;
            case 1:
                if (o->stateVar0 == 0xFF) {
                    p->unk2C = lim;
                    done++;
                    continue;
                }
                switch (p->type) {
                default:
                    p->unk2C = lim;
                    break;
                case 1:
                    p->unk2C = -5;
                    break;
                }
                break;
            case 2:
                p->unk2C = 1;
                switch (p->type) {
                case 1:
                    p->unk32 = Rand_Next(1);
                    break;
                }
                break;
            case 4:
                if (o->stateVar0 == 0xFF) {
                    p->unk2C = lim;
                    done++;
                    continue;
                }
                switch (p->type) {
                case 0:
                    break;
                case 1:
                    p->unk2C = -5;
                    break;
                case 2:
                case 3:
                case 4:
                    p->unk2C = -3;
                    break;
                }
                break;
            case 6:
                EffEm33_StartPart(o, p);
                if (p->type == 0xFF) {
                    continue;
                }
                if (tbl != NULL) {
                    lim = tbl[p->type];
                }
                break;
            case 9:
                p->unk2C = 1;
                break;
            case 10:
                p->unk2C = lim;
                break;
            case 11:
                p->unk2C = lim;
                break;
            case 13:
                p->unk2C = 1;
                break;
            }
        }
        if (p->unk2C <= 0 || p->unk2C > lim) {
            switch (o->type) {
            case 1:
            case 4:
            case 6:
                if (o->stateVar0 == 0xFF) {
                    p->unk2C = lim;
                    done++;
                }
                break;
            }
            continue;
        }
        if (p->unk2C == 1) {
            switch (o->type) {
            case 0:
                EffEm33_StartPartType0(o, p);
                break;
            case 1:
                if (o->stateVar0 == 0xFF) {
                    p->unk2C = lim;
                    done++;
                    continue;
                }
                EffEm33_StartPartType1(o, p);
                break;
            case 4:
                EffEm33_StartPartType4(o, p);
                break;
            case 6:
                EffEm33_PartAtBone(o, p);
                break;
            case 9:
                EffEm33_StartPartType9(o, p);
                break;
            case 13:
                switch (p->type) {
                case 3:
                    p->vec[0] = 40.0f * Lure_RandSigned();
                    p->vec[1] = 40.0f * Lure_RandSigned();
                    p->vec[2] = 40.0f * Lure_RandSigned();
                    p->unk32 = Rand_Next(1);
                    break;
                }
                break;
            }
        }
        switch (o->type) {
        default:
        case 0:
        case 2:
        case 6:
        case 7:
        case 9:
        case 12:
        case 13:
            k = p->unk2C;
            f = k;
            break;
        case 1:
            switch (p->type) {
            case 0:
                f = (o->ownerMotion & 1) + 1;
                break;
            default:
                f = p->unk2C;
                break;
            }
            k = p->unk2C;
            break;
        case 3:
        case 5:
        case 8:
            switch (p->type) {
            case 2:
                k = p->unk2C;
                f = k;
                break;
            default:
                if (p->unk2C >= 8) {
                    f = 8.0f;
                } else {
                    f = p->unk2C;
                }
                k = (p->unk2C + p->type) % 7;
                break;
            }
            break;
        case 4:
            switch (p->type) {
            case 0:
                if (o->stateVar0 == 0xFF) {
                    if (++p->unk3B >= 10) {
                        p->unk3B = 10;
                        done++;
                        continue;
                    }
                    f = 4.0f + p->unk3B;
                } else if (p->unk2C > 4) {
                    f = 4.0f;
                } else {
                    f = p->unk2C;
                }
                k = p->unk3B;
                break;
            case 1:
            default:
                k = p->unk2C;
                f = k;
                break;
            }
            break;
        case 10:
            if (o->stateVar0 == 0xFF) {
                if (++p->unk3B >= 10) {
                    p->unk3B = 10;
                    done++;
                    continue;
                }
                f = p->unk3B + 10;
            } else {
                f = p->unk2C;
            }
            k = p->unk3B;
            break;
        case 11:
            if (o->stateVar0 == 0xFF) {
                if (++p->unk3B >= 20) {
                    p->unk3B = 20;
                    done++;
                    continue;
                }
                k = p->unk3B + 10;
                f = k;
            } else {
                k = p->unk2C;
                f = k;
            }
            break;
        }
        if (p->unk03 != 0) {
            Key_LerpVec2(EffEm33_Curves[ci], p->size, f);
        } else {
            Key_LerpVec3(EffEm33_Curves[ci], p->size, f);
        }
        Color_EvalKeys(EffEm33_ColorKeys[ai], k, (u32 *) &p->color);
        switch (o->type) {
        case 0:
            switch (p->type) {
            case 0:
            case 1:
                Vec_Copy(p->vec, o->pos);
                p->unk2E = o->rotX;
                p->unk30 = o->rotY;
                break;
            case 2:
            case 3:
            case 4:
                Vec_Diff(v, o->pos, p->vec);
                Vec_Scale(v, v, 0.8f / (f32) (lim + 1 - p->unk2C));
                Vec_Add(p->vec, p->vec, v);
                break;
            }
            break;
        case 1:
            if (p->type != 1) {
                Vec_Copy(p->vec, o->pos);
                p->unk2E = o->rotX;
                p->unk30 = o->rotY;
            } else {
                p->vec[1] += 2.0f;
            }
            break;
        case 2:
            Vec_Copy(p->vec, o->pos);
            break;
        case 3:
        case 5:
        case 8:
            if (p->type != 2) {
                Vec_Copy(p->vec, o->pos);
                p->unk2E = o->rotX;
                p->unk30 = o->rotY;
            }
            break;
        case 4:
            switch (p->type) {
            case 0:
                Vec_Copy(p->vec, o->pos2);
                break;
            case 1:
                p->unk28 += -0.5f;
                p->vec[1] += p->unk28;
                break;
            case 2:
            case 3:
            case 4:
                p->vec[1] += p->unk28;
                break;
            }
            break;
        case 6:
            switch (p->type) {
            default:
                p->unk28 *= 0.95f;
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = p->unk28;
                Vec_RotateY(v, (6.2831855f * p->unk30) / 65536.0f);
                p->vec[0] += v[0];
                p->vec[1] += 3.0f * p->scale;
                p->vec[2] += v[2];
                break;
            case 0:
                break;
            }
            break;
        case 7:
            p->vec[1] += 5.0f * p->scale;
            break;
        case 10:
        case 11:
            Vec_Copy(p->vec, o->pos2);
            p->unk2E = o->rotX;
            p->unk30 = o->rotY;
            break;
        case 14:
            break;
        }
        p->unk2E += p->unk34;
        p->unk30 += p->unk36;
        p->unk32 += p->unk38;
        if (p->draw != NULL) {
            switch (o->type) {
            default:
                Vec_Copy(p->draw->pos, p->vec);
                break;
            case 9:
                p->draw->pos[0] = o->pos[0] + p->vec[0];
                p->draw->pos[1] = o->pos[1] + p->vec[1];
                p->draw->pos[2] = o->pos[2] + p->vec[2];
                break;
            case 13:
                switch (p->type) {
                case 3:
                    p->draw->pos[0] = o->pos[0] + p->vec[0];
                    p->draw->pos[1] = o->pos[1] + p->vec[1];
                    p->draw->pos[2] = o->pos[2] + p->vec[2];
                    break;
                default:
                    Vec_Copy(p->draw->pos, o->pos);
                    break;
                }
                break;
            }
            if (p->unk3A == 0) {
                OT_AddByDepth(gDrawOt32, p->draw, 0x20, 0);
            } else {
                OT_AddByDepth(gDrawOtSorted, p->draw, 0x40, 0);
            }
        }
    }
    switch (o->type) {
    case 0:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 1900.0f * m->scaleZ;
        Vec_RotateX(v, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
        o->pos[0] = o->pos2[0] + v[0];
        o->pos[1] = o->pos2[1] + v[1];
        o->pos[2] = o->pos2[2] + v[2];
        break;
    case 1:
        if (o->hitPhase != 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 200.0f * m->scaleZ;
            Vec_RotateX(v, (6.2831855f * o->rotX) / 65536.0f);
            Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + v[0];
            o->pos[1] = o->pos2[1] + v[1];
            o->pos[2] = o->pos2[2] + v[2];
        } else if (done >= cnt) {
            o->state = 2;
        }
        break;
    case 2:
    case 9:
    case 13:
        floor = AreaGrid2_GetFloorYFiltered(o->pos);
        if (o->pos[1] <= floor) {
            if (floor - o->pos[1] > 100.0f) {
                EffEm33_Impact(o, 2, o->pos);
            } else {
                q[0] = o->pos[0];
                q[1] = floor;
                q[2] = o->pos[2];
                EffEm33_Impact(o, 0, q);
            }
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 3:
        floor = AreaGrid2_GetFloorYFiltered(o->pos);
        if (o->pos[1] <= floor) {
            if (floor - o->pos[1] > 200.0f) {
                EffEm33_Impact(o, 1, o->pos);
            } else {
                q[0] = o->pos[0];
                q[1] = floor;
                q[2] = o->pos[2];
                EffEm33_Impact(o, 0, q);
            }
            o->hitFrames = 0;
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 4:
        p = (EffPart *) (b + 0xC);
        if (o->hitPhase != 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 300.0f * p->size[2];
            Vec_RotateX(v, (6.2831855f * o->rotX) / 65536.0f);
            Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + v[0];
            o->pos[1] = o->pos2[1] + v[1];
            o->pos[2] = o->pos2[2] + v[2];
            if (o->area == gScene.area && o->ownerMotion % 10 == 0) {
                Em_PlaySeChecked(m, 0x44, 0, o->pos, 6, 0);
            }
        } else if (done >= cnt) {
            o->state = 2;
        }
        break;
    case 6:
        if (o->hitPhase != 0) {
            v[0] = 0.0f;
            v[1] = 2.0f;
            v[2] = -5.0f;
            Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + v[0];
            o->pos[1] = o->pos2[1] + v[1];
            o->pos[2] = o->pos2[2] + v[2];
        }
        break;
    case 7:
        if (o->hitPhase != 0) {
            o->pos[1] = 5.0f + o->pos2[1];
        }
        break;
    case 8:
        if (o->ownerMotion == life) {
            switch (Area_DayId(o->area)) {
            case 12:
            default:
                tab = EffEm33_TblA;
                break;
            case 0xB1:
                tab = EffEm33_TblB;
                break;
            case 0xB2:
                tab = EffEm33_TblC;
                break;
            case 0xB3:
                tab = EffEm33_TblD;
                break;
            }
            EffEm33_Impact(o, 0, tab[o->stateVar0]);
        }
        break;
    case 10:
        p = (EffPart *) (b + 0xC);
        if (o->hitPhase != 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 20.0f * p->size[2];
            Vec_RotateX(v, (6.2831855f * o->rotX) / 65536.0f);
            Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + v[0];
            o->pos[1] = o->pos2[1] + v[1];
            o->pos[2] = o->pos2[2] + v[2];
            if (o->area == gScene.area && Math_Absf(v[1]) > 500.0f) {
                t = (*(f32 *) (b + 8) - o->pos2[1]) / v[1];
                if (Math_Absf(t) > 0.0f && Math_Absf(t) <= 1.0f && (o->ownerMotion & 3) == 0) {
                    q[0] = o->pos2[0] + v[0] * t;
                    q[1] = *(f32 *) (b + 8);
                    q[2] = o->pos2[2] + v[2] * t;
                    Em_PlaySeChecked(m, 0x3B, 0, q, 0x18, 0);
                    StageObj_K1C_CreateAt(q, 0x23, 1.0f);
                }
            }
        } else if (done >= cnt) {
            o->state = 2;
        }
        break;
    case 11:
        if (o->hitPhase != 0) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 1.0f;
            Vec_RotateX(v, (6.2831855f * o->rotX) / 65536.0f);
            Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
            o->pos[0] = o->pos2[0] + 900.0f * v[0];
            o->pos[1] = o->pos2[1] + 900.0f * v[1];
            o->pos[2] = o->pos2[2] + 900.0f * v[2];
            if (o->area == gScene.area) {
                if (o->ownerMotion % 10 == 0) {
                    Em_PlaySeChecked(m, 0x3C, 0, o->pos, 6, 0);
                }
                if (o->ownerMotion % 6 == 0) {
                    StageObj_K1C_CreateTwoPoint(o->pos, v, 4, 1.0f);
                }
            }
        } else if (done >= cnt) {
            o->state = 2;
        }
        break;
    }
}

void EffEm33_Hit(ObjAttach *o)
{
    EffEm33_Run(o);
}

void EffEm33_FreeSlots(ObjAttach *o)
{
    EffPart *p = (EffPart *) (o->buf + 0xC);
    s16 n = EffEm33_PartCounts[o->type];
    s16 i;

    for (i = 0; i < n; i++) {
        if (p->draw != NULL) {
            if (o->keep != 0) {
                DrawSlotB_Free(p->slot);
            } else {
                DrawSlot_Free(p->slot);
            }
        }
        p++;
    }
}

void EffEm33_FreeSlotsType15(ObjAttach *o)
{
    EffEm33Link *p = (EffEm33Link *) o->buf;
    s16 n = EffEm33_PartCounts[o->type];
    s16 i;

    for (i = 0; i < n; i++) {
        if (p->draw != NULL) {
            if (o->keep != 0) {
                DrawSlotB_Free(p->slot);
            } else {
                DrawSlot_Free(p->slot);
            }
        }
        p++;
    }
}

void EffEm33_End(ObjAttach *o)
{
    if (o->drawSlotBuf != NULL) {
        if (o->keep != 0) {
            DrawSlotB_Free(o->drawSlot);
        } else {
            DrawSlot_Free(o->drawSlot);
        }
    }
    switch (o->type) {
    case 0xF:
        EffEm33_FreeSlotsType15(o);
        break;
    default:
        EffEm33_FreeSlots(o);
        break;
    }
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
}

void EffEm33_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffEm33_DrawChain(DrawSlot *d)
{
    f32 sc[3];
    f32 rot[3];
    f32 vz[3];
    f32 m[16];
    f32 t[16];
    EffEm33Link *p;
    ObjAttach *o;
    u16 *tbl;
    ModelMat *tex;
    u32 c;
    ModelMatSet *model;
    u16 flags;
    u32 col;
    Model *mdl;
    s16 id;
    s16 k;
    f32 s;
    s32 q;
    u8 a8;

    o = d->owner;
    p = &((EffEm33Chain *)o->buf)->link[d->part];
    mdl = gCommonModels[0];
    tbl = EffEm33_DrawTbls[o->type];
    flags = 0;
    if (tbl[p->index] & 0x8000) {
        id = EmSlot_FindKind(o->aux);
        if (id == -1) {
            return;
        }
        mdl = gScene.partModels[id];
    }
    if (mdl == NULL) {
        return;
    }
    if (mdl->active == 0) {
        return;
    }
    if (p->delay < 0) {
        k = tbl[p->index] & ~0x8000;
    } else {
        k = 1;
    }
    model = &mdl->matSets[k];
    tex = mdl->mats;
    s = EffEm33_ChainAlpha(o);
    sc[0] = s * (p->size[0] * p->scale);
    sc[1] = s * (p->size[1] * p->scale);
    sc[2] = s * (p->size[2] * p->scale);
    rot[2] = (6.2831855f * p->spin) / 65536.0f;
    Mtx_FromScaleRotTrans(sc, rot, d->pos, 3, m);
    c = p->color;
    a8 = (u8) (c >> 24) * (1.0f - 0.5f * (p->index / 8.0f));
    col = a8 << 24 | (u8) (c >> 16) << 16 | (u8) (c >> 8) << 8 | (u8) c;
    if (p->delay < 0) {
        flags |= 2;
        switch (p->index) {
        case 0:
            vz[0] = vz[1] = vz[2] = 0.0f;
            rot[0] = (6.2831855f * Vec_Elevation(p->dir[0], vz)) / 65536.0f;
            rot[1] = (6.2831855f * ((u16) Vec_Heading(p->dir[0], vz) + 0x4000)) / 65536.0f;
            Mtx_RotateX(m, rot[0]);
            Mtx_RotateY(m, rot[1]);
            Mtx_Translation(t, 0.0f, (p->age & 0xF) / 16.0f, 0.0f);
            break;
        default:
            Mtx_Concat3x3(m, gCamMatrix);
            q = p->age % 15;
            Mtx_Translation(t, 0.25f * (q & 3), 0.25f * (q >> 2), 0.0f);
            break;
        }
        Gs_SetState(0x19, (s32) t);
    } else {
        Mtx_Concat3x3(m, gCamMatrix);
    }
    Gs_SetState(0x6C, 0);
    Effect_DrawModel(model, m, col, flags, tex);
    Gs_SetState(0x6C, 1);
}

s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
void Mtx_Billboard(f32 *m, f32 *pos);
extern u16 EffEm33_DrawKeys[];

void EffEm33_DrawPart(DrawSlot *d)
{
    f32 m[16];
    f32 tm[16];
    f32 rot[3];
    f32 sc[3];
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    ObjAttach *o;
    EffPart *p;
    u16 *tbl;
    Model *mdl;
    ModelMat *tex;
    ModelMatSet *model;
    u32 col;
    s16 id;
    s16 k;
    s16 q;
    u16 flags;
    u16 order;
    u8 mode;

    mode = 1;
    flags = 0;
    order = 0;
    o = d->owner;
    p = (EffPart *) (o->buf + d->part * 0x3C + 0xC);
    if (o->area != gScene.area) {
        return;
    }
    if (o->type == 0xF) {
        EffEm33_DrawChain(d);
        return;
    }
    tbl = EffEm33_DrawTbls[o->type];
    mdl = gCommonModels[0];
    if (tbl[p->type] & 0x8000) {
        id = EmSlot_FindKind(o->aux);
        if (id == -1) {
            return;
        }
        mdl = gScene.partModels[id];
    }
    if (mdl == NULL) {
        return;
    }
    if (mdl->active == 0) {
        return;
    }
    if ((o->type == 0xC && p->type == 3) || (o->type == 0xE && p->type == 3)) {
        k = EffEm33_DrawKeys[o->stateVar0] & ~0x8000;
    } else {
        k = tbl[p->type] & ~0x8000;
    }
    model = &mdl->matSets[k];
    col = p->color;
    tex = mdl->mats;
    switch (o->type) {
    case 0:
        switch (p->type) {
        case 0:
            Mtx_Translation(tm, 0.0f, (o->ownerMotion & 0x1F) / 32.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
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
    case 1:
        switch (p->type) {
        case 0:
            Mtx_Translation(tm, 0.0f, (o->ownerMotion & 0x1FF) / 512.0f, 0.0f);
            order = 7;
            mode = 0;
            break;
        case 1:
            if (p->unk3B == 0) {
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            } else {
                Mtx_Translation(tm, 0.5f, 0.75f, 0.0f);
            }
            order = 3;
            break;
        default:
            return;
        }
        Gs_SetState(0x19, (s32) tm);
        break;
    case 2:
        switch (p->type) {
        case 0:
            order = 7;
            mode = 0;
            break;
        case 1:
            Mtx_Translation(tm, 0.0f, 0.1f * (o->ownerMotion % 10), 0.0f);
            Gs_SetState(0x19, (s32) tm);
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
    case 3:
    case 5:
    case 8:
        switch (p->type) {
        case 0:
            q = p->unk2C;
            Mtx_Translation(tm, 0.125f * (q % 4), 0.125f * ((q & 7) >> 2), 0.0f);
            Gs_SetState(0x19, (s32) tm);
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
    case 4:
        switch (p->type) {
        case 0:
            Mtx_Translation(tm, 0.0f, (o->ownerMotion & 0xF) / 16.0f, 0.0f);
            order = 7;
            mode = 0;
            break;
        case 1:
            Mtx_Identity(tm);
            order = 3;
            mode = 1;
            break;
        case 2:
        case 3:
        case 4:
            if ((u8) (col >> 24) != 0xFF) {
                Mtx_Translation(tm, 0.75f, 0.0f, 0.0f);
            } else {
                Mtx_Identity(tm);
            }
            break;
        default:
            return;
        }
        Gs_SetState(0x19, (s32) tm);
        break;
    case 6:
        switch (p->type) {
        case 0:
            order = 2;
            mode = 0;
            Effect_GetColor(gScene.area, &r, &g, &b, &a);
            col = b | ((g << 8) | ((r << 16) | ((u8) ((a / 255.0f) * (u8) (p->color >> 24)) << 24)));
            break;
        case 1:
        case 2:
            if (p->unk3B == 0) {
                Mtx_Identity(tm);
            } else {
                Mtx_Translation(tm, 0.5f, 0.75f, 0.0f);
            }
            mode = 1;
            order = 3;
            break;
        default:
            return;
        }
        Gs_SetState(0x19, (s32) tm);
        break;
    case 7:
        if (p->unk3B == 0) {
            Mtx_Identity(tm);
        } else {
            Mtx_Translation(tm, 0.5f, 0.75f, 0.0f);
        }
        mode = 1;
        order = 3;
        Gs_SetState(0x19, (s32) tm);
        break;
    case 9:
        mode = 1;
        order = 3;
        break;
    case 10:
        Mtx_Translation(tm, 0.0f, (o->ownerMotion & 7) / 8.0f, 0.0f);
        order = 4;
        mode = 3;
        Gs_SetState(0x19, (s32) tm);
        break;
    case 11:
        order = 7;
        mode = 0;
        Mtx_Translation(tm, 0.0f, (o->ownerMotion & 0x1F) / 32.0f, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        break;
    case 12:
    case 14:
        switch (p->type) {
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
        Mtx_Identity(tm);
        Gs_SetState(0x19, (s32) tm);
        break;
    case 13:
        switch (p->type) {
        case 0:
            order = 7;
            mode = 0;
            break;
        case 1:
        case 2:
            order = 7;
            mode = 0;
            Mtx_Translation(tm, 0.0f, 0.1875f * (p->unk2C / 30.0f), 0.0f);
            Gs_SetState(0x19, (s32) tm);
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
        sc[0] = 1.0f;
        sc[1] = 1.0f;
        sc[2] = 1.0f;
    } else {
        sc[0] = p->size[0] * p->scale;
        sc[1] = p->size[1] * p->scale;
        sc[2] = p->size[2] * p->scale;
    }
    rot[0] = (6.2831855f * p->unk2E) / 65536.0f;
    rot[1] = (6.2831855f * p->unk30) / 65536.0f;
    rot[2] = (6.2831855f * p->unk32) / 65536.0f;
    Mtx_FromScaleRotTrans(sc, rot, d->pos, order, m);
    switch ((u8) mode) {
    case 0:
        break;
    case 1:
        Mtx_Concat3x3(m, gCamMatrix);
        break;
    case 3:
        Mtx_Billboard(m, d->pos);
        sc[0] = p->size[0] * p->scale;
        sc[1] = p->size[1] * p->scale;
        sc[2] = p->size[2] * p->scale;
        m[0] *= sc[0];
        m[1] *= sc[0];
        m[2] *= sc[0];
        m[4] *= sc[1];
        m[5] *= sc[1];
        m[6] *= sc[1];
        m[8] *= sc[2];
        m[9] *= sc[2];
        m[10] *= sc[2];
        break;
    }
    if (p->unk3A == 0) {
        Gs_SetState(0x67, col);
        Effect_DrawModelAdd(model, m, tex);
        return;
    }
    Gs_SetState(0x6C, 0);
    Effect_DrawModel(model, m, col, flags, tex);
    Gs_SetState(0x6C, 1);
}

void EffEm33_Impact(ObjAttach *o, s16 n, f32 *pos)
{
    Em *em = o->owner;

    if (o->area == gScene.area) {
        switch (o->type) {
        default:
            return;
        case 2:
            Em_PlaySeChecked(em, 0xF, 0, pos, 6, 0);
            StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x11, 1.0f);
            CamShake_SetA(4, pos);
            Eff09_CreateOwned((Player *)em, pos, 0x27);
            break;
        case 3:
        case 8:
            Em_PlaySeAtPos(em, 0x39, 0, o->pos, 0x18, 0);
            Sound_PlaySeAtPosChecked(1, 0x4F, 0, o->pos, 0x18, 0);
            StageObj_K0E_CreateScaled(pos, 1, 0.45000002f);
            StageObj_K11_CreateAtScaled(pos, o->rotY, 4, 0.45000002f);
            StageObj_K11_CreateAtScaled(pos, o->rotY, 9, 3.0f);
            switch (n) {
            case 0:
                StageObj_K11_CreateAtScaled(pos, o->rotY, 0xB, 3.0f);
                CamShake_SetA(5, pos);
                break;
            case 1:
                break;
            }
            break;
        case 9:
            if (o->stateVar1 != 1) {
                Em_PlaySeChecked(em, 0x3B, 0, o->pos, 6, 0);
                StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x20, 1.0f);
                EffEm07_CreateAt(em, pos, 7);
                switch (n) {
                case 0:
                    StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x21, 1.0f);
                    break;
                case 1:
                    break;
                }
            } else {
                Em_PlaySeChecked(em, 0x3E, 0, o->pos, 6, 0);
                StageObj_K14_CreateAt(pos, 0x1D, 0, 1.2f);
                EffEm07_CreateAt(em, pos, 14);
            }
            break;
        case 13:
            Em_PlaySeChecked(em, 0x25, 0, pos, 15, 0);
            StageObj_K1C_CreateAtYaw(pos, o->rotY, 0x11, 1.0f);
            CamShake_SetA(4, pos);
            Eff09_CreateOwned((Player *)em, pos, 0x28);
            break;
        }
    }
}

void EffEm33_StartPartType0(ObjAttach *o, EffPart *p)
{
    Em *m = o->owner;
    f32 v[3];

    Vec_Copy(p->vec, o->pos);
    switch (p->type) {
    case 0:
        break;
    case 1:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = m->scaleZ * ((Rand_Next(1) & 0x1FF) - 0x100);
        Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += v[0];
        p->vec[2] += v[2];
        p->unk32 = Rand_Next(1);
        p->unk38 = -(Rand_Next(1) & 0x3FF);
        break;
    case 2:
        v[0] = m->scaleX * ((Rand_Next(1) & 0x1FF) - 0x100);
        v[1] = m->scaleY * ((Rand_Next(1) & 0x1FF) - 0x100);
        v[2] = 2000.0f + m->scaleZ * ((Rand_Next(1) & 0x1FF) - 0x100);
        Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += v[0];
        p->vec[1] += v[1];
        p->vec[2] += v[2];
        p->unk32 = Rand_Next(1);
        p->unk38 = 0.5f + (65536.0f * ((20.0f + 20.0f * Lure_RandUnit()) / 30.0f)) / 360.0f;
        if (p->unk32 & 1) {
            p->unk38 = -p->unk38;
        }
        break;
    case 3:
    case 4:
        v[0] = m->scaleX * ((Rand_Next(1) & 0x3FF) - 0x200);
        v[1] = m->scaleY * ((Rand_Next(1) & 0x3FF) - 0x200);
        v[2] = 2000.0f + m->scaleZ * ((Rand_Next(1) & 0x3FF) - 0x200);
        Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += v[0];
        p->vec[1] += v[1];
        p->vec[2] += v[2];
        p->scale = m->scaleX * (1.0f + Lure_RandUnit());
        p->unk2E = Rand_Next(1);
        p->unk30 = Rand_Next(1);
        p->unk32 = Rand_Next(1);
        break;
    }
}

void EffEm33_StartPartType1(ObjAttach *o, EffPart *p)
{
    Em *m = o->owner;
    f32 v[3];
    u16 r;

    Vec_Copy(p->vec, o->pos);
    switch (p->type) {
    case 0:
        break;
    case 1:
        v[0] = m->scaleX * ((Rand_Next(1) & 0x7F) - 0x40);
        v[1] = m->scaleY * ((Rand_Next(1) & 0x3F) - 0x20);
        v[2] = 230.0f + m->scaleZ * ((Rand_Next(1) & 0x7F) - 0x40);
        Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += v[0];
        p->vec[1] += v[1];
        p->vec[2] += v[2];
        r = Rand_Next(1);
        p->unk3B = r & 1;
        p->unk32 = Rand_Next(1);
        p->unk38 = 0x5B;
        if ((r >> 1) & 1) {
            p->unk38 = -p->unk38;
        }
        Em_PlaySeChecked(m, 0x1A, 0, p->vec, 1, 0);
        break;
    }
}

void EffEm33_StartPartType4(ObjAttach *o, EffPart *p)
{
    Em *m = o->owner;
    f32 v[3];

    Vec_Copy(p->vec, o->pos2);
    switch (p->type) {
    case 0:
        break;
    case 1:
    case 2:
    case 3:
    case 4:
        p->scale = 1.5f * m->scaleX;
        v[0] = m->scaleX * ((Rand_Next(1) & 0xFF) - 0x80);
        v[1] = m->scaleY * ((Rand_Next(1) & 0xFF) - 0x80);
        v[2] = 900.0f + m->scaleZ * ((Rand_Next(1) & 0xFF) - 0x80);
        Vec_RotateX(v, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
        p->vec[0] += v[0];
        p->vec[1] += v[1];
        p->vec[2] += v[2];
        p->unk32 = Rand_Next(1);
        p->unk38 = 0.5f + (65536.0f * (15.0f / EffEm33_PartPeriods[p->type])) / 360.0f;
        if (Rand_Next(1) & 1) {
            p->unk38 = -p->unk38;
        }
        p->unk28 = 2.0f;
        break;
    }
}

void EffEm33_StartPart(ObjAttach *o, EffPart *p)
{
    u16 *frames = EffEm33_SeqFrames;
    u8 *types = EffEm33_SeqTypes;

    if (types[o->stateVar0] == 0xFF) {
        p->type = 0xFF;
        return;
    }
    p->type = types[o->stateVar0];
    p->unk2C = o->ownerMotion - frames[o->stateVar0];
    switch (p->type) {
    case 1:
        p->unk30 = o->rotY + 0x8000 + (o->stateVar0 % 3 - 1) * 0x18E4 + ((Rand_Next(1) & 0x1FFF) - 0x1000);
        break;
    case 2:
        p->unk30 = o->rotY + 0x8000 + (((o->stateVar0 & 1) - 1) << 13) + (Rand_Next(1) & 0x1FFF);
        break;
    default:
        p->unk30 = o->rotY;
        break;
    }
    o->stateVar0++;
}

void EffEm33_PartAtBone(ObjAttach *o, EffPart *p)
{
    f32 y = p->vec[1];
    void *bone = Em_Bone(o->owner, EffEm33_Bones[o->type]);
    f32 v[3];

    v[0] = EffEm33_BoneOfs[o->type * 3];
    v[1] = D_008AD2E4[o->type * 3];
    v[2] = D_008AD2E8[o->type * 3];
    Vec_RotateSelf(v, bone);
    Mtx_GetTrans(p->vec, bone);
    Vec_Add(p->vec, p->vec, v);
    switch (p->type) {
    case 0:
        p->unk03 = 0;
        p->vec[1] = y;
        p->scale = 1.0f;
        break;
    case 1:
        p->unk03 = 1;
        p->unk32 = Rand_Next(1);
        p->unk38 = 0x5B;
        p->unk3B = Rand_Next(1) & 1;
        p->unk28 = 20.0f * p->scale;
        p->scale = 1.0f;
        break;
    case 2:
        p->unk03 = 1;
        p->unk32 = Rand_Next(1);
        p->unk38 = 0x5B;
        p->unk3B = Rand_Next(1) & 1;
        p->scale = 0.5f;
        p->unk28 = 15.0f * p->scale;
        break;
    }
}

void EffEm33_StartPartType9(ObjAttach *o, EffPart *p)
{
    p->unk32 = Rand_Next(1);
    p->unk38 = 0x71C;
    if (Rand_Next(1) & 1) {
        p->unk38 = -p->unk38;
    }
    p->vec[0] = p->scale * (40.0f * Lure_RandSigned());
    p->vec[1] = p->scale * (40.0f * Lure_RandSigned());
    p->vec[2] = p->scale * (40.0f * Lure_RandSigned());
}

void EffEm33_PlaceAtBone(ObjAttach *o, u16 *ang)
{
    f32 *vy;
    f32 *vz;
    f32 *mz;
    f32 *mx;
    f32 v[3];
    f32 m[16];

    Mtx_Copy(m, (f32 *)Em_GetBone(o->owner, EffEm33_Bones[o->type]));
    vy = &v[1];
    vz = &v[2];
    v[0] = EffEm33_BoneOfs[o->type * 3];
    *vy = D_008AD2E4[o->type * 3];
    *vz = D_008AD2E8[o->type * 3];
    Vec_RotateSelf(v, m);
    o->pos[0] = m[12] + v[0];
    o->pos[1] = m[13] + *vy;
    o->pos[2] = m[14] + *vz;
    Vec_Copy(o->pos2, o->pos);
    mz = &m[10];
    mx = &m[8];
    o->rotY = ang[2] + (u16) (s32) (0.5f + (65536.0f * Math_Atan2(*mx, *mz)) / 6.2831855f);
    switch (o->type) {
    case 0:
        v[0] = 0.0f;
        *vy = 0.0f;
        *vz = 1000.0f * ((Em *) o->owner)->scaleZ;
        Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
        o->pos2[0] += v[0];
        o->pos2[1] += *vy;
        o->pos2[2] += *vz;
        break;
    case 10:
        o->rotX = ang[1] + (u16) (s32) (0.5f + (65536.0f * Math_Atan2(m[9], Math_Sqrtf(*mx * *mx + *mz * *mz))) / 6.2831855f);
        break;
    }
}

void EffEm33_UpdateLink(ObjAttach *o, EffEm33Link *e, EffEm33Link *prev, void *bone, f32 *dir, s16 f)
{
    f32 v[3];
    u16 ax;
    u16 ay;
    f32 len;
    s16 i;
    EffEm33Link *p;

    switch (e->index) {
    case 0:
        Mtx_GetTrans(e->pos, bone);
        Vec_Copy(v, (f32 *) ((u8 *) bone + 0x20));
        Vec_DirToAngles(v, (s16 *) &ax, (s16 *) &ay);
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 1.0f;
        Vec_RotateX(dir, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(dir, (6.2831855f * ay) / 65536.0f);
        e->from[0][0] = e->pos[0] + 150.0f * dir[0];
        e->from[0][1] = e->pos[1] + 150.0f * dir[1];
        e->from[0][2] = e->pos[2] + 150.0f * dir[2];
        return;
    case 1:
        len = 300.0f * EffEm33_ChainAlpha(o);
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
    if (f == 0) {
        for (i = 0, p = e; i < 2; i++, p = (EffEm33Link *)((u8 *)p + 0xC)) {
            Vec_Copy(p->dir[0], dir);
            Vec_Copy(p->from[0], prev->pos);
        }
    } else {
        if (e->index == 1) {
            Vec_Copy(e->dir[0], dir);
            Vec_Copy(e->from[0], prev->pos);
        } else {
            p = (EffEm33Link *)((u8 *)e + 0xC);
            Vec_Copy(e->dir[0], p->dir[0]);
            Vec_Copy(e->from[0], p->from[0]);
        }
        Vec_Copy(e->dir[1], dir);
        Vec_Copy(e->from[1], prev->pos);
    }
    e->pos[0] = e->from[0][0] + e->dir[0][0] * len;
    e->pos[1] = e->from[0][1] + e->dir[0][1] * len;
    e->pos[2] = e->from[0][2] + e->dir[0][2] * len;
}

void EffEm33_RandLinkSpin(EffEm33Link *e)
{
    e->spin = Rand_Next(1);
    e->spinSpeed = (s16) ((Rand_Next(1) & 0xFFFF & 0x1FF) - 0x100);
}

f32 EffEm33_ChainAlpha(ObjAttach *o)
{
    f32 ret;
    s16 t;

    t = o->ownerMotion;
    if (t < 0) {
        return 0.2f;
    }
    if (t < 0xF) {
        ret = 0.2f + ((0.8f * (f32) t) / 15.0f);
    } else {
        ret = 1.0f;
    }
    return ret;
}

/* unk_6E3B00's .data (0x8AB600-0x8AD3A0) */
HitShape EffEm33_Vals088_0[2] = {
#include "game/008AB600.inc"
};
HitShape EffEm33_Vals088_1[2] = {
#include "game/008AB650.inc"
};
HitShape EffEm33_Vals088_2[2] = {
#include "game/008AB6A0.inc"
};
HitShape EffEm33_Vals088_3[2] = {
#include "game/008AB6F0.inc"
};
HitShape EffEm33_Vals088_4[2] = {
#include "game/008AB740.inc"
};
HitShape EffEm33_Vals088_5[2] = {
#include "game/008AB790.inc"
};
HitShape EffEm33_Vals088_6[2] = {
#include "game/008AB7E0.inc"
};
HitShape EffEm33_Vals088_7[3] = {
#include "game/008AB830.inc"
};
HitShape EffEm33_Vals088_8[3] = {
#include "game/008AB8B0.inc"
};
HitShape EffEm33_Vals088_9[2] = {
#include "game/008AB930.inc"
};
HitShape EffEm33_Vals088_10[3] = {
#include "game/008AB980.inc"
};
HitShape EffEm33_Vals088_11[2] = {
#include "game/008ABA00.inc"
};
HitShape *EffEm33_Vals088[12] = {
    EffEm33_Vals088_0, EffEm33_Vals088_1, EffEm33_Vals088_2, EffEm33_Vals088_3,
    EffEm33_Vals088_4, EffEm33_Vals088_5, EffEm33_Vals088_6, EffEm33_Vals088_7,
    EffEm33_Vals088_8, EffEm33_Vals088_9, EffEm33_Vals088_10, EffEm33_Vals088_11,
};
u32 D_008ABA80[132] = {
#include "game/008ABA80.inc"
};
void *EffEm33_MotionDesc = (void *)D_008ABA80;
u32 EffEm33_Curves_0[16] = {
#include "game/008ABCA0.inc"
};
u32 EffEm33_Curves_1[12] = {
#include "game/008ABCE0.inc"
};
u32 EffEm33_Curves_2[12] = {
#include "game/008ABD10.inc"
};
u32 EffEm33_Curves_3[12] = {
#include "game/008ABD40.inc"
};
u32 EffEm33_ColorKeys_0[8] = {
#include "game/008ABD70.inc"
};
u32 EffEm33_ColorKeys_1[8] = {
#include "game/008ABD90.inc"
};
u32 EffEm33_ColorKeys_2[12] = {
#include "game/008ABDB0.inc"
};
u32 EffEm33_ColorKeys_3[8] = {
#include "game/008ABDE0.inc"
};
s16 EffEm33_PartVal2CA[8] = {
#include "game/008ABE00.inc"
};
u16 EffEm33_DrawTbls_0[8] = {
#include "game/008ABE10.inc"
};
u32 EffEm33_RunTbls_0[4] = {
#include "game/008ABE20.inc"
};
u32 EffEm33_Curves_5[16] = {
#include "game/008ABE30.inc"
};
u32 EffEm33_Curves_6[12] = {
#include "game/008ABE70.inc"
};
u32 EffEm33_ColorKeys_5[12] = {
#include "game/008ABEA0.inc"
};
u32 EffEm33_ColorKeys_6[8] = {
#include "game/008ABED0.inc"
};
u32 EffEm33_ColorKeys_7[12] = {
#include "game/008ABEF0.inc"
};
u32 EffEm33_ColorKeys_8[8] = {
#include "game/008ABF20.inc"
};
u32 EffEm33_ColorKeys_9[12] = {
#include "game/008ABF40.inc"
};
u32 EffEm33_ColorKeys_10[8] = {
#include "game/008ABF70.inc"
};
u32 EffEm33_ColorKeys_11[12] = {
#include "game/008ABF90.inc"
};
u32 EffEm33_ColorKeys_12[14] = {
#include "game/008ABFC0.inc"
};
u16 EffEm33_DrawTbls_1[4] = {
#include "game/008ABFF8.inc"
};
u32 EffEm33_RunTbls_1[4] = {
#include "game/008AC000.inc"
};
u32 EffEm33_Curves_7[16] = {
#include "game/008AC010.inc"
};
u32 EffEm33_Curves_8[12] = {
#include "game/008AC050.inc"
};
u32 EffEm33_Curves_9[8] = {
#include "game/008AC080.inc"
};
u32 EffEm33_Curves_10[8] = {
#include "game/008AC0A0.inc"
};
u32 EffEm33_ColorKeys_13[8] = {
#include "game/008AC0C0.inc"
};
u32 EffEm33_ColorKeys_14[8] = {
#include "game/008AC0E0.inc"
};
u32 EffEm33_ColorKeys_15[8] = {
#include "game/008AC100.inc"
};
u32 EffEm33_ColorKeys_16[8] = {
#include "game/008AC120.inc"
};
u16 EffEm33_DrawTbls_2[4] = {
#include "game/008AC140.inc"
};
u32 EffEm33_RunTbls_2[2] = {
#include "game/008AC148.inc"
};
u32 EffEm33_Curves_11[12] = {
#include "game/008AC150.inc"
};
u32 EffEm33_Curves_12[16] = {
#include "game/008AC180.inc"
};
u32 EffEm33_ColorKeys_17[8] = {
#include "game/008AC1C0.inc"
};
u16 EffEm33_DrawTbls_3[4] = {
#include "game/008AC1E0.inc"
};
u32 EffEm33_RunTbls_3[2] = {
#include "game/008AC1E8.inc"
};
u32 EffEm33_Curves_14[16] = {
#include "game/008AC1F0.inc"
};
u32 EffEm33_Curves_15[12] = {
#include "game/008AC230.inc"
};
u32 EffEm33_ColorKeys_20[8] = {
#include "game/008AC260.inc"
};
u32 EffEm33_ColorKeys_21[12] = {
#include "game/008AC280.inc"
};
u32 EffEm33_ColorKeys_22[8] = {
#include "game/008AC2B0.inc"
};
u16 EffEm33_DrawTbls_4[8] = {
#include "game/008AC2D0.inc"
};
s16 EffEm33_PartPeriods[8] = {
#include "game/008AC2E0.inc"
};
s16 EffEm33_PartVal2CB[8] = {
#include "game/008AC2F0.inc"
};
u32 EffEm33_Curves_19[16] = {
#include "game/008AC300.inc"
};
u32 EffEm33_Curves_20[12] = {
#include "game/008AC340.inc"
};
u32 EffEm33_ColorKeys_25[8] = {
#include "game/008AC370.inc"
};
u32 EffEm33_ColorKeys_26[10] = {
#include "game/008AC390.inc"
};
u16 EffEm33_DrawTbls_6[4] = {
#include "game/008AC3B8.inc"
};
u32 EffEm33_RunTbls_6[2] = {
#include "game/008AC3C0.inc"
};
u8 EffEm33_SeqTypes[12] = {
#include "game/008AC3C8.inc"
};
u16 EffEm33_SeqFrames[16] = {
#include "game/008AC3E0.inc"
};
u32 EffEm33_Curves_22[12] = {
#include "game/008AC400.inc"
};
u32 EffEm33_ColorKeys_28[10] = {
#include "game/008AC430.inc"
};
u16 EffEm33_DrawTbls_7[4] = {
#include "game/008AC458.inc"
};
u32 EffEm33_RunTbls_7[4] = {
#include "game/008AC460.inc"
};
f32 EffEm33_TblA[4][3] = {
#include "game/008AC470.inc"
};
f32 EffEm33_TblB[4][3] = {
#include "game/008AC4A0.inc"
};
f32 EffEm33_TblC[4][3] = {
#include "game/008AC4D0.inc"
};
f32 EffEm33_TblD[4][3] = {
#include "game/008AC500.inc"
};
u32 EffEm33_Curves_13[12] = {
#include "game/008AC530.inc"
};
u32 EffEm33_ColorKeys_19[8] = {
#include "game/008AC560.inc"
};
u32 EffEm33_Curves_23[12] = {
#include "game/008AC580.inc"
};
u32 EffEm33_ColorKeys_29[8] = {
#include "game/008AC5B0.inc"
};
u32 EffEm33_ColorKeys_30[8] = {
#include "game/008AC5D0.inc"
};
u16 EffEm33_DrawTbls_9[4] = {
#include "game/008AC5F0.inc"
};
u32 EffEm33_RunTbls_9[2] = {
#include "game/008AC5F8.inc"
};
u32 EffEm33_Curves_24[16] = {
#include "game/008AC600.inc"
};
u32 EffEm33_ColorKeys_31[8] = {
#include "game/008AC640.inc"
};
u16 EffEm33_DrawTbls_10[4] = {
#include "game/008AC660.inc"
};
u32 EffEm33_RunTbls_10[2] = {
#include "game/008AC668.inc"
};
u32 EffEm33_Curves_25[20] = {
#include "game/008AC670.inc"
};
u32 EffEm33_ColorKeys_32[8] = {
#include "game/008AC6C0.inc"
};
u16 EffEm33_DrawTbls_11[4] = {
#include "game/008AC6E0.inc"
};
u32 EffEm33_RunTbls_11[2] = {
#include "game/008AC6E8.inc"
};
u32 EffEm33_Curves_26[12] = {
#include "game/008AC6F0.inc"
};
u32 EffEm33_Curves_27[16] = {
#include "game/008AC720.inc"
};
u32 EffEm33_Curves_28[24] = {
#include "game/008AC760.inc"
};
u32 EffEm33_Curves_29[16] = {
#include "game/008AC7C0.inc"
};
u32 EffEm33_Curves_30[16] = {
#include "game/008AC800.inc"
};
u32 EffEm33_Curves_31[8] = {
#include "game/008AC840.inc"
};
u32 EffEm33_ColorKeys_33[8] = {
#include "game/008AC860.inc"
};
u32 EffEm33_ColorKeys_34[8] = {
#include "game/008AC880.inc"
};
u32 EffEm33_ColorKeys_35[12] = {
#include "game/008AC8A0.inc"
};
u32 EffEm33_ColorKeys_36[12] = {
#include "game/008AC8D0.inc"
};
u32 EffEm33_ColorKeys_37[8] = {
#include "game/008AC900.inc"
};
u32 EffEm33_ColorKeys_38[10] = {
#include "game/008AC920.inc"
};
u16 EffEm33_DrawTbls_12[6] = {
#include "game/008AC948.inc"
};
u16 EffEm33_PartVal32[4] = {
#include "game/008AC958.inc"
};
u16 EffEm33_DrawKeys[4] = {
#include "game/008AC960.inc"
};
s16 EffEm33_PartVal2CC[6] = {
#include "game/008AC968.inc"
};
f32 EffEm33_Offsets[40][3] = {
#include "game/008AC980.inc"
};
u32 EffEm33_Curves_32[16] = {
#include "game/008ACB60.inc"
};
u32 EffEm33_Curves_33[12] = {
#include "game/008ACBA0.inc"
};
u32 EffEm33_Curves_34[12] = {
#include "game/008ACBD0.inc"
};
u32 EffEm33_Curves_35[8] = {
#include "game/008ACC00.inc"
};
u32 EffEm33_Curves_36[8] = {
#include "game/008ACC20.inc"
};
u32 EffEm33_Curves_37[12] = {
#include "game/008ACC40.inc"
};
u32 EffEm33_ColorKeys_39[8] = {
#include "game/008ACC70.inc"
};
u32 EffEm33_ColorKeys_40[12] = {
#include "game/008ACC90.inc"
};
u32 EffEm33_ColorKeys_41[12] = {
#include "game/008ACCC0.inc"
};
u32 EffEm33_ColorKeys_42[8] = {
#include "game/008ACCF0.inc"
};
u32 EffEm33_ColorKeys_43[8] = {
#include "game/008ACD10.inc"
};
u32 EffEm33_ColorKeys_44[8] = {
#include "game/008ACD30.inc"
};
u16 EffEm33_DrawTbls_13[8] = {
#include "game/008ACD50.inc"
};
u32 EffEm33_RunTbls_13[4] = {
#include "game/008ACD60.inc"
};
f32 EffEm33_StartPos[40][3] = {
#include "game/008ACD70.inc"
};
u32 EffEm33_Curves_38[12] = {
#include "game/008ACF50.inc"
};
u32 EffEm33_ColorKeys_45[12] = {
#include "game/008ACF80.inc"
};
u16 EffEm33_DrawTbls_15[8] = {
#include "game/008ACFB0.inc"
};
u32 EffEm33_RunTbls_15[4] = {
#include "game/008ACFC0.inc"
};
void *EffEm33_Curves[40] = {
    (void *)EffEm33_Curves_0, (void *)EffEm33_Curves_1, (void *)EffEm33_Curves_2, (void *)EffEm33_Curves_3,
    (void *)EffEm33_Curves_3, (void *)EffEm33_Curves_5, (void *)EffEm33_Curves_6, (void *)EffEm33_Curves_7,
    (void *)EffEm33_Curves_8, (void *)EffEm33_Curves_9, (void *)EffEm33_Curves_10, (void *)EffEm33_Curves_11,
    (void *)EffEm33_Curves_12, (void *)EffEm33_Curves_13, (void *)EffEm33_Curves_14, (void *)EffEm33_Curves_15,
    (void *)EffEm33_Curves_15, (void *)EffEm33_Curves_15, (void *)EffEm33_Curves_15, (void *)EffEm33_Curves_19,
    (void *)EffEm33_Curves_20, (void *)EffEm33_Curves_20, (void *)EffEm33_Curves_22, (void *)EffEm33_Curves_23,
    (void *)EffEm33_Curves_24, (void *)EffEm33_Curves_25, (void *)EffEm33_Curves_26, (void *)EffEm33_Curves_27,
    (void *)EffEm33_Curves_28, (void *)EffEm33_Curves_29, (void *)EffEm33_Curves_30, (void *)EffEm33_Curves_31,
    (void *)EffEm33_Curves_32, (void *)EffEm33_Curves_33, (void *)EffEm33_Curves_34, (void *)EffEm33_Curves_35,
    (void *)EffEm33_Curves_36, (void *)EffEm33_Curves_37, (void *)EffEm33_Curves_38, NULL,
};
void *EffEm33_ColorKeys[48] = {
    (void *)EffEm33_ColorKeys_0, (void *)EffEm33_ColorKeys_1, (void *)EffEm33_ColorKeys_2, (void *)EffEm33_ColorKeys_3,
    (void *)EffEm33_ColorKeys_3, (void *)EffEm33_ColorKeys_5, (void *)EffEm33_ColorKeys_6, (void *)EffEm33_ColorKeys_7,
    (void *)EffEm33_ColorKeys_8, (void *)EffEm33_ColorKeys_9, (void *)EffEm33_ColorKeys_10, (void *)EffEm33_ColorKeys_11,
    (void *)EffEm33_ColorKeys_12, (void *)EffEm33_ColorKeys_13, (void *)EffEm33_ColorKeys_14, (void *)EffEm33_ColorKeys_15,
    (void *)EffEm33_ColorKeys_16, (void *)EffEm33_ColorKeys_17, (void *)EffEm33_ColorKeys_17, (void *)EffEm33_ColorKeys_19,
    (void *)EffEm33_ColorKeys_20, (void *)EffEm33_ColorKeys_21, (void *)EffEm33_ColorKeys_22, (void *)EffEm33_ColorKeys_22,
    (void *)EffEm33_ColorKeys_22, (void *)EffEm33_ColorKeys_25, (void *)EffEm33_ColorKeys_26, (void *)EffEm33_ColorKeys_26,
    (void *)EffEm33_ColorKeys_28, (void *)EffEm33_ColorKeys_29, (void *)EffEm33_ColorKeys_30, (void *)EffEm33_ColorKeys_31,
    (void *)EffEm33_ColorKeys_32, (void *)EffEm33_ColorKeys_33, (void *)EffEm33_ColorKeys_34, (void *)EffEm33_ColorKeys_35,
    (void *)EffEm33_ColorKeys_36, (void *)EffEm33_ColorKeys_37, (void *)EffEm33_ColorKeys_38, (void *)EffEm33_ColorKeys_39,
    (void *)EffEm33_ColorKeys_40, (void *)EffEm33_ColorKeys_41, (void *)EffEm33_ColorKeys_42, (void *)EffEm33_ColorKeys_43,
    (void *)EffEm33_ColorKeys_44, (void *)EffEm33_ColorKeys_45, NULL, NULL,
};
s16 EffEm33_PartCounts[16] = {
#include "game/008AD130.inc"
};
s16 EffEm33_Lives[16] = {
#include "game/008AD150.inc"
};
s16 EffEm33_CurveIdx[16] = {
#include "game/008AD170.inc"
};
s16 EffEm33_ColorIdx[16] = {
#include "game/008AD190.inc"
};
u16 *EffEm33_DrawTbls[16] = {
    EffEm33_DrawTbls_0, EffEm33_DrawTbls_1, EffEm33_DrawTbls_2, EffEm33_DrawTbls_3,
    EffEm33_DrawTbls_4, EffEm33_DrawTbls_3, EffEm33_DrawTbls_6, EffEm33_DrawTbls_7,
    EffEm33_DrawTbls_3, EffEm33_DrawTbls_9, EffEm33_DrawTbls_10, EffEm33_DrawTbls_11,
    EffEm33_DrawTbls_12, EffEm33_DrawTbls_13, EffEm33_DrawTbls_12, EffEm33_DrawTbls_15,
};
u32 EffEm33_RunTbls[16] = {
    (u32)EffEm33_RunTbls_0, (u32)EffEm33_RunTbls_1, (u32)EffEm33_RunTbls_2, (u32)EffEm33_RunTbls_3,
    (u32)EffEm33_PartPeriods, (u32)EffEm33_RunTbls_3, (u32)EffEm33_RunTbls_6, (u32)EffEm33_RunTbls_7,
    (u32)EffEm33_RunTbls_3, (u32)EffEm33_RunTbls_9, (u32)EffEm33_RunTbls_10, (u32)EffEm33_RunTbls_11,
    (u32)EffEm33_PartVal2CC, (u32)EffEm33_RunTbls_13, (u32)EffEm33_PartVal2CC, (u32)EffEm33_RunTbls_15,
};
s16 EffEm33_MotionBase[16] = {
#include "game/008AD230.inc"
};
f32 EffEm33_Val050ByType[16] = {
#include "game/008AD250.inc"
};
f32 EffEm33_Val058ByType[16] = {
#include "game/008AD290.inc"
};
u8 EffEm33_Bones[16] = {
#include "game/008AD2D0.inc"
};
f32 EffEm33_BoneOfs[48] = {
#include "game/008AD2E0.inc"
};
