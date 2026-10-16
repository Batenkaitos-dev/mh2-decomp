/*
 * Game overlay code at 0x6E0650-0x6E3B00: the effect objects Eff28_*,
 * EffKushala29_*, EffKushala30_*, EffShengaoren31_* and EffYamatsukami31_*.
 */
#include "common.h"
#include "em.h"
#include "objattach.h"
#include "player.h"
#include "scene.h"
#include "stageobj.h"
#include "sprite.h"
#include "capsule.h"
/* prototypes of callees (round 13) */
void DrawSlotB_Free(s32);

/* prototypes */
extern s16 EffEm33_CurveIdx[];
extern s16 EffEm33_ColorIdx[];
extern void *EffEm33_Curves[];
extern void *EffEm33_ColorKeys[];
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void EffEm33_RandLinkSpin(struct EffEm33Link *e);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Mtx_Copy(f32 *dst, f32 *src);
f32 Math_Atan2(f32 y, f32 x);
s32 Em_PlaySeAtPos(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
void CamShake_SetA(s32 kind, f32 *pos);
void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale);
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
f32 AreaGrid2_GetFloorY(f32 *pos);
void EffKushala29_Draw(DrawSlot *d);
extern u8 EffKushala29_Params0[];
extern f32 D_008A8C2C[];
extern s8 EffKushala29_MotionIds[];
extern void *EffKushala29_MotionDesc;
extern HitShape *gEffKushala29HitLists[];
u16 Area_BaseId(u16 area);
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
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
void Mtx_RotateZ(f32 *m, f32 a);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *a);
f32 Math_Atan(f32 x);
f32 Math_Sqrtf(f32 x);
u16 Math_Heading(f32 x1, f32 z1, f32 x2, f32 z2);
s32 Gs_SetState(u8 reg, u32 v);
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Key_LerpVec2(f32 *k, volatile f32 *out, f32 t);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
extern void *EffKushala29_AlphaKeys[];
extern void *EffKushala29_VecKeys[];
extern f32 EffKushala29_HeightScale[];
s16 DrawSlot_Alloc(void);
s16 DrawSlotB_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
DrawSlot *DrawSlotB_Get(s32 i);
void EffEm33_DrawPart(DrawSlot *d);
s32 Motion_ReachedFrame(void *obj, s32 layer, f32 frame);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
EmBone *Em_GetBone(void *obj, s16 bone);
void Vec_Rotate(f32 *out, f32 *v, f32 *m);
extern f32 EffKushala29_BoneOfs2[];
void EffKushala29_Update(ObjAttach *o);
void EffShengaoren31_Update(ObjAttach *o);
void EffYamatsukami31_Update(ObjAttach *o);
void EffEm02_Update(ObjAttach *o);
extern u8 EffEm33_SeqTypes[];
extern u16 EffEm33_SeqFrames[];
f32 Rand_Unit(void);
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
void EffShakalaka34_Draw(ObjAttach *o);
void EffEm02_Draw(ObjAttach *o);
extern HitShape *gEffShakalaka34HitLists[];
extern void *EffShakalaka34_MotionDesc;
extern HitShape *gEffEm02HitLists[];
extern void *EffEm02_MotionDesc;
StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
/* unit's own declaration: with the definition's void (Player *owner, f32 *pos, s32 kind) (game/unk_6D29F0.c), EffKushala29_Impact: 1 of 106 words differ */
void Eff09_CreateOwned(Player *owner, f32 *pos, u8 kind);
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
extern s16 EffEm33_MotionBase[];
extern void *EffEm33_MotionDesc;
typedef struct EffPart { s16 slot; u8 type; u8 unk03; f32 vec[3]; u32 color; f32 size[3]; f32 scale; u8 *draw; f32 unk28; s16 unk2C; u16 unk2E; u16 unk30; u16 unk32; s16 unk34; s16 unk36; s16 unk38; u8 unk3A; u8 unk3B; } EffPart;
void EffEm33_StartPart(ObjAttach *o, EffPart *p);
f32 Rand_Signed(void);
u16 Area_IdForCurrentTime(u16);
ObjAttach *ObjAttach_Alloc(s32 n);
void ObjAttach_Free(ObjAttach *p);
u16 Rand_Next(s32 stream);
void EffKushala29_Run(ObjAttach *o);
void EffKushala29_End(ObjAttach *o);
void EffKushala29_Free(ObjAttach *o);
void EffKushala30_Setup(ObjAttach *o);
void EffKushala30_End(ObjAttach *o);
void EffKushala30_Free(ObjAttach *o);
void EffShengaoren31_Setup(ObjAttach *o);
void EffShengaoren31_Run(ObjAttach *o);
void EffShengaoren31_End(ObjAttach *o);
void EffShengaoren31_Free(ObjAttach *o);
void EffYamatsukami31_Setup(ObjAttach *o);
void EffYamatsukami31_Run(ObjAttach *o);
void EffYamatsukami31_End(ObjAttach *o);
void EffYamatsukami31_Free(ObjAttach *o);
void EffEm33_Run(ObjAttach *o);
void EffEm33_Hit(ObjAttach *o);
void EffEm33_End(ObjAttach *o);
void EffEm33_Free(ObjAttach *o);
void EffShakalaka34_End(ObjAttach *o);
void EffShakalaka34_Free(ObjAttach *o);
void EffKushala30b_Setup(ObjAttach *o);
void EffKushala30b_End(ObjAttach *o);
void EffKushala30b_Free(ObjAttach *o);
void EffEm02_Run(ObjAttach *o);
void EffEm02_End(ObjAttach *o);
void EffEm02_Free(ObjAttach *o);
void DrawSlot_Free(s32);
extern s16 EffEm33_PartCounts[];
extern s16 EffEm33_PartCounts[];
extern s16 gEffKushala29Counts[];
void EffEm33_Update(ObjAttach *o);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void Vec_Copy(f32 *out, f32 *in);
void EffKushala30_Update(ObjAttach *o);
void EffShakalaka34_Update(ObjAttach *o);
void EffKushala30b_Update(ObjAttach *o);
u8 Player_InCurrentArea(void *obj);
s32 Player_TestSetFlags(void *, s32 mask);
struct MoveObj;
void MoveObj_StartMotionOwned(struct MoveObj *, s32 a, s32 b, void *c);
extern HitShape *gEffKushala30HitLists[];
extern void *EffKushala30_MotionDesc;
extern HitShape *gEffShengaoren31HitLists[];
extern void *EffShengaoren31_MotionDesc;
extern HitShape *gEffYamatsukami31HitLists[];
extern void *EffYamatsukami31_MotionDesc;
extern HitShape *gEffKushala30bHitLists[];
extern void *EffKushala30b_MotionDesc;
f32 Math_Absf(f32 x);
void EffShakalaka34_Setup(ObjAttach *o);
void EffEm02_Setup(ObjAttach *o);
void *EffEm33_Create(Em *em, s32 kind, s32 c, s32 a, s32 b);
void EffShakalaka34_Run(ObjAttach *o);
void EffKushala30_Run(ObjAttach *o);
void EffKushala30b_Run(ObjAttach *o);
void EffEm33_UpdateLink(ObjAttach *o, struct EffEm33Link *e, struct EffEm33Link *prev, void *bone, f32 *dir, s16 f);
void EffEm33_Setup(ObjAttach *o);
void EffKushala29_Setup(ObjAttach *o);
/* end of prototypes */

void Eff28_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void Eff28_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void Eff28_Draw(DrawSlot *d)
{
    f32 m1[16];
    f32 m2[16];
    ObjAttach *o;
    u8 *b;
    Model *mdl;
    ModelMat *tex;
    ModelMatSet *model;
    f32 ax;
    u16 ay;

    mdl = gCommonModels[0];
    o = d->owner;
    b = o->buf;
    if (o->area == gScene.area && mdl != NULL && mdl->active != 0) {
        if (b[0x2C] == 1) {
            o->rotZ = 0;
        }
        model = &mdl->matSets[152];
        tex = mdl->mats;
        Mtx_Identity(m1);
        ax = -1.0f * Math_Atan(o->velY / Math_Sqrtf(o->velZ * o->velZ + o->velX * o->velX));
        ay = Math_Heading(o->pos[0], o->pos[2], o->pos2[0], o->pos2[2]) + 0x4000;
        Mtx_RotateX(m1, ax);
        Mtx_RotateY(m1, (6.2831855f * ay) / 65536.0f);
        Mtx_Identity(m2);
        Mtx_Scale(m2, 1.0f, 1.0f, 1.0f);
        Mtx_SetTrans(m2, d->pos[0], d->pos[1], d->pos[2]);
        Mtx_RotateY(m2, (6.2831855f * o->rotY) / 65536.0f);
        Mtx_RotateZ(m2, (6.2831855f * o->rotZ) / 65536.0f);
        Mtx_Concat3x3(m2, m1);
        Gs_SetState(0x1A, (s32) m2);
        Gs_SetState(0x67, -1);
        Gs_SetLightList(tex, model);
        if (model != NULL && model->id != -1) {
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

/* EffKushala29's object buffer (o->buf) and its particles (gEffKushala29Counts[type] of them). */
typedef struct Ku29Part {
    f32 pos[3];                 /* 0x00 */
    f32 scale[3];               /* 0x0C: Key_LerpVec2/3 of EffKushala29_VecKeys, scaled by the buffer's size */
    f32 vel[3];                 /* 0x18 */
    DrawSlot *ds;               /* 0x24: its draw slot (part = its index) */
    f32 unk28;                  /* 0x28 */
    u16 rot[3];                 /* 0x2C: X, Y, Z (kind 2); Z spins (EffKushala29_SpinPart) */
    s16 timer;                  /* 0x32: starts at or below 0; reset at 0, moves while positive */
    s16 slot;                   /* 0x34 */
    s8 kind;                    /* 0x36: 0, 1 (on the floor) or 2 */
    u8 active;                  /* 0x37 */
    u8 alpha;                   /* 0x38: 255 * Key_LerpFloat of EffKushala29_AlphaKeys */
    u8 flags;                   /* 0x39: 4: spins forward (EffKushala29_SpinPart), 2: the second model (kind 1) */
    u8 pad3A[2];
} Ku29Part;

typedef struct Ku29Buf {
    f32 vec[3];                 /* 0x00: Key_LerpVec3's value, scaled by EffKushala29_HeightScale[size] */
    f32 height;                 /* 0x0C */
    u16 ang;                    /* 0x10 */
    s16 spin;                   /* 0x12: the bone at creation; then the Z spin (+0x71C a frame) */
    u8 size;                    /* 0x14: index into EffKushala29_HeightScale */
    u8 pad15[3];
    Ku29Part part[1];           /* 0x18 */
} Ku29Buf;

void EffKushala29_CreateAtBone(Em *em, s32 bone, u8 kind, u8 c, u16 a, u16 b)
{
    ObjAttach *o;
    f32 v[3];
    Ku29Buf *p;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(1)) != NULL) {
            o->kind = 29;
            o->type = kind;
            o->state = 0;
            o->step = 0;
            o->stateVar0 = 0;
            o->stateVar1 = 0;
            o->idx = em->index;
            o->update = EffKushala29_Update;
            o->isEm = em->isEm;
            Em_GetBonePos(em, bone, o->pos);
            o->aux = em->type;
            o->owner = em;
            o->yaw = em->rotY;
            o->rotX = a;
            o->rotY = b + em->rotY;
            o->rotZ = 0;
            ((s32 *) &o->velX)[0] = 0;
            ((s32 *) &o->velX)[1] = 0;
            ((s32 *) &o->velX)[2] = 0;
            switch (o->type) {
            case 1:
            case 2:
                Vec_Copy(v, EffKushala29_BoneOfs2);
                Vec_Rotate((f32 *) v, (f32 *) v, (f32 *) (Em_GetBone(em, bone)));
                o->pos[0] += v[0];
                o->pos[1] += v[1];
                o->pos[2] += v[2];
                break;
            }
            o->area = em->area;
            o->side = *(u8 *) &em->side;
            p = (Ku29Buf *) o->buf;
            p->size = c;
            p->spin = bone;
        }
    }
}

void EffKushala29_Spawn(ObjAttach *src, u8 kind, u8 c)
{
    ObjAttach *o;

    if (src->area == gScene.area) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 29;
            o->type = kind;
            o->state = 0;
            o->step = 0;
            o->stateVar0 = 0;
            o->stateVar1 = c;
            o->idx = src->idx;
            o->update = EffKushala29_Update;
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

void EffKushala29_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffKushala29_Setup(o);
        break;
    case 1:
        EffKushala29_Run(o);
        break;
    case 2:
        EffKushala29_End(o);
        break;
    case 3:
        EffKushala29_Run(o);
        break;
    case 4:
        EffKushala29_End(o);
        break;
    case 5:
        EffKushala29_End(o);
        break;
    case 9:
        EffKushala29_End(o);
        break;
    case 7:
        EffKushala29_End(o);
        break;
    case 6:
        EffKushala29_Free(o);
        
    default:
        break;
    }
}

void EffKushala29_Setup(ObjAttach *o)
{
    f32 v[3];
    Ku29Part *e;
    Em *owner;
    s16 i;
    Ku29Buf *b;
    u8 save;
    s16 sub;
    s16 n;

    owner = o->owner;
    sub = 0;
    o->state = 1;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    case 0:
    case 1:
    case 2:
        b = (Ku29Buf *) o->buf;
        b->spin = 0;
        e = b->part;
        sub += b->size;
        o->drawSlot = DrawSlot_Alloc();
        if (o->drawSlot != -1) {
            o->drawSlotBuf = DrawSlot_Get(o->drawSlot);
            o->drawSlotBuf->owner = o;
            o->drawSlotBuf->part = -1;
            o->drawSlotBuf->draw = EffKushala29_Draw;
        } else {
            o->drawSlotBuf = NULL;
        }
        break;
    case 3:
        o->drawSlotBuf = NULL;
        sub += o->stateVar1;
        break;
    }
    switch (o->type) {
    case 0:
        o->pos[1] = AreaGrid2_GetFloorY(o->pos);
        o->pos[1] += *(f32 *) (EffKushala29_Params0 + 4) * EffKushala29_HeightScale[b->size];
        o->velZ = 100.0f;
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        break;
    case 1:
        o->velZ = 80.0f;
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        break;
    case 2:
        b->height = o->pos[1] - AreaGrid2_GetFloorY(o->pos);
        b->ang = o->rotX;
        o->rotX = (u16) (s32) (0.5f + (65536.0f * ((360.0f * b->ang) / 65536.0f)) / 360.0f);
        o->velZ = 100.0f;
        Vec_RotateX(&o->velX, (6.2831855f * o->rotX) / 65536.0f);
        Vec_RotateY(&o->velX, (6.2831855f * o->rotY) / 65536.0f);
        break;
    case 3:
        Vec_Copy(v, D_008A8C2C);
        Vec_RotateY(v, (6.2831855f * o->rotY) / 65536.0f);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        save = o->hitId;
        break;
    }
    Vec_Copy(o->pos2, o->pos);
    o->ownerMotion = 0;
    n = gEffKushala29Counts[o->type];
    for (i = 0; i < n; i++, e++) {
        e->slot = DrawSlot_Alloc();
        if (e->slot != -1) {
            e->ds = DrawSlot_Get(e->slot);
            e->ds->owner = o;
            e->ds->part = i;
            e->ds->draw = EffKushala29_Draw;
        } else {
            e->ds = NULL;
        }
        e->rot[0] = 0;
        e->rot[1] = 0;
        e->rot[2] = 0;
        e->vel[0] = 0.0f;
        e->vel[1] = 0.0f;
        switch (o->type) {
        case 0:
            e->active = 1;
            break;
        case 2:
        default:
            e->active = 0;
            break;
        }
        if (i <= 2) {
            e->kind = 1;
            e->timer = -i * 5 - 15;
            e->vel[2] = 100.0f + (-500.0f + *(f32 *) (EffKushala29_Params0 + (i + 1) * 12 + 8)) / 10.0f;
            Vec_RotateY(e->vel, (6.2831855f * o->rotY) / 65536.0f);
        } else if (i <= 4) {
            e->kind = 0;
            e->timer = -(i - 3) * 4 - 5;
            e->vel[2] = 0.0f;
        } else {
            e->kind = 2;
            e->timer = -(i - 5) * 3;
            e->vel[2] = 100.0f + (-300.0f + -200.0f * Rand_Unit()) / 6.0f;
            Vec_RotateY(e->vel, (6.2831855f * o->rotY) / 65536.0f);
        }
    }
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) owner, EffKushala29_MotionIds[o->type] + sub, &EffKushala29_MotionDesc);
    o->hitList = gEffKushala29HitLists[o->hitListNo];
    o->hurtList = 0;
    switch (o->type) {
    case 3:
        o->hitId = save;
        break;
    }
    EffKushala29_Run(o);
}

extern s16 gEffKushala29Limits[];
extern f32 D_008A8BC4[];
extern f32 EffKushala29_Dir0[];
extern f32 D_008A8BFC[];
extern f32 D_008A8C08[];
extern f32 D_008A8C14[];
void MoveObj_AddVelocity(struct MoveObj *);
void EffKushala29_SpinPart(ObjAttach *o, Ku29Part *p, u16 n);
void EffKushala29_DrawPart(ObjAttach *o, Ku29Part *p);
void EffKushala29_Impact(ObjAttach *o, s32 arg1, f32 *pos);

/* EffKushala29's frame (state 1): moves it; past its lifetime (gEffKushala29Limits[type] frames) ends it
 * (EffKushala29_Impact at the floor for types 0-2); else types 0 and 2 spawn type 3 children and play
 * sound 0x3E, types 0-2 follow the floor (ending when they leave it), and each live particle is
 * reset at its timer's 0 (position, spin, velocity by kind), moved and drawn. */
void EffKushala29_Run(ObjAttach *o)
{
    f32 d[3];
    f32 r[3];
    f32 v[3];
    Ku29Buf *b;
    Ku29Part *e;
    s16 n;
    f32 floor;
    f32 h;
    f32 y;
    s16 lim;
    s16 i;
    s32 a;

    b = (Ku29Buf *) o->buf;
    lim = gEffKushala29Limits[o->type];
    e = b->part;
    n = gEffKushala29Counts[o->type];
    o->hitFrames = 99;
    Vec_Copy(o->pos2, o->pos);
    MoveObj_AddVelocity((struct MoveObj *)o);
    if (lim < ++o->ownerMotion) {
        switch (o->type) {
        case 0:
        case 1:
        case 2:
            if (gScene.area != o->area) {
                break;
            }
            floor = AreaGrid2_GetFloorY(o->pos);
            Vec_Copy(v, o->pos);
            switch (o->type) {
            case 0:
            case 2:
                h = D_008A8BC4[0] * EffKushala29_HeightScale[b->size];
                if (Math_Absf(o->pos[1] - (floor + h)) <= 50.0f) {
                    v[1] = floor;
                } else {
                    v[1] -= h;
                }
                break;
            case 1:
                if (o->pos[1] <= floor && floor - o->pos[1] <= 100.0f) {
                    v[1] = floor;
                }
                break;
            }
            EffKushala29_Impact(o, 0, v);
            break;
        }
        o->hitFrames = 0;
        o->hitPhase = 0;
        o->state = 2;
        return;
    }
    switch (o->type) {
    case 0:
        if (o->ownerMotion > 5 && (o->ownerMotion - 5) % 4 == 0) {
            EffKushala29_Spawn(o, 3, 0);
        }
        if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
            Em_PlaySeChecked(o->owner, 0x3E, 0, o->pos, 0x18, 0);
        }
        goto run;
    case 2:
        switch (o->stateVar0) {
        case 1:
            if (o->ownerMotion % 4 == 0) {
                switch (b->size) {
                case 2:
                case 5:
                    EffKushala29_Spawn(o, 3, 1);
                    break;
                default:
                    EffKushala29_Spawn(o, 3, 0);
                    break;
                }
            }
            if (o->area == gScene.area && o->ownerMotion % 5 == 0) {
                Em_PlaySeChecked(o->owner, 0x3E, 0, o->pos, 0x18, 0);
            }
        }
    default:
    run:
        b->spin += 0x71C;
        floor = AreaGrid2_GetFloorY(o->pos);
        Key_LerpVec3(EffKushala29_VecKeys[0], b->vec, o->ownerMotion % 4);
        b->vec[0] *= EffKushala29_HeightScale[b->size];
        b->vec[1] *= EffKushala29_HeightScale[b->size];
        b->vec[2] *= EffKushala29_HeightScale[b->size];
        switch (o->type) {
        case 0:
            h = D_008A8BC4[0] * EffKushala29_HeightScale[b->size];
            y = floor + h;
            if (!(Math_Absf(o->pos[1] - y) <= 50.0f)) {
                v[0] = o->pos[0];
                v[1] = o->pos[1] - h;
                v[2] = o->pos[2];
                EffKushala29_Impact(o, 0, v);
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = 2;
                return;
            }
            o->pos[1] = y;
            break;
        case 1:
            if (o->pos[1] <= floor) {
                if (!(floor - o->pos[1] <= 100.0f)) {
                    EffKushala29_Impact(o, 0, o->pos);
                } else {
                    v[0] = o->pos[0];
                    v[1] = floor;
                    v[2] = o->pos[2];
                    EffKushala29_Impact(o, 0, v);
                }
                o->hitFrames = 0;
                o->hitPhase = 0;
                o->state = 2;
                return;
            }
            break;
        case 2:
            h = D_008A8BC4[0] * EffKushala29_HeightScale[b->size];
            switch (o->step) {
            case 0:
                y = floor + h;
                if (o->pos[1] - y < 0.0f) {
                    v[0] = o->pos[0];
                    v[1] = o->pos[1] - h;
                    v[2] = o->pos[2];
                    EffKushala29_Impact(o, 0, v);
                    o->hitFrames = 0;
                    o->hitPhase = 0;
                    o->state = 2;
                    return;
                }
                a = (u16) (u32) ((f32) (u32) b->ang * (o->pos[1] - y - h) / (b->height - h));
                if (a <= 0x200) {
                    o->rotX = 0;
                    o->pos[1] = y;
                    o->step++;
                    for (i = 0; i < n; i++) {
                        e->active = 1;
                        e++;
                    }
                    o->stateVar0++;
                } else if (a <= o->rotX) {
                    o->rotX = a;
                }
                o->velX = 0.0f;
                o->velY = 0.0f;
                o->velZ = 100.0f;
                Vec_RotateX(&o->velX, 6.2831855f * o->rotX / 65536.0f);
                Vec_RotateY(&o->velX, 6.2831855f * o->rotY / 65536.0f);
                break;
            case 1:
                y = floor + h;
                if (!(Math_Absf(o->pos[1] - y) <= 50.0f)) {
                    v[0] = o->pos[0];
                    v[1] = o->pos[1] - h;
                    v[2] = o->pos[2];
                    EffKushala29_Impact(o, 0, v);
                    o->hitFrames = 0;
                    o->hitPhase = 0;
                    o->state = 2;
                    return;
                }
                o->pos[1] = y;
                break;
            }
            break;
        }
        if (o->drawSlotBuf != NULL) {
            Vec_Copy(o->drawSlotBuf->pos, o->pos);
            OT_AddByDepth(gDrawOtSorted, o->drawSlotBuf, 0x40, 0);
        }
        e = b->part;
        for (i = 0; i < n; i++, e++) {
            if (e->active == 0) {
                continue;
            }
            if (e->timer == 0) {
                Vec_Copy(e->pos, o->pos);
                e->rot[2] = Rand_Next(1);
                switch (e->kind) {
                case 0:
                    e->rot[2] = Rand_Next(1);
                    e->flags = Rand_Next(1) & 4;
                    Vec_Copy(r, D_008A8BFC);
                    r[0] *= Rand_Signed();
                    r[1] *= Rand_Signed();
                    r[2] *= Rand_Signed();
                    Vec_Copy(d, EffKushala29_Dir0);
                    Vec_Add(d, d, r);
                    break;
                case 1:
                    e->rot[2] = Rand_Next(1);
                    e->flags = Rand_Next(1) & 6;
                    Vec_Copy(d, (f32 *) (EffKushala29_Params0 + (i + 1) * 12));
                    break;
                case 2:
                    e->rot[0] = (s16) (Rand_Next(1) % 910) - 0x38E;
                    e->rot[1] = (s16) (Rand_Next(1) % 1820) - 0x71C;
                    e->rot[2] = (s16) (Rand_Next(1) % 1820) - 0x71C;
                    Vec_Copy(r, D_008A8C14);
                    r[0] *= Rand_Signed();
                    r[1] *= Rand_Signed();
                    r[2] *= Rand_Signed();
                    Vec_Copy(d, D_008A8C08);
                    Vec_Add(d, d, r);
                    break;
                }
                d[0] *= EffKushala29_HeightScale[b->size];
                d[1] *= EffKushala29_HeightScale[b->size];
                d[2] *= EffKushala29_HeightScale[b->size];
                Vec_RotateY(d, 6.2831855f * o->rotY / 65536.0f);
                Vec_Add(e->pos, e->pos, d);
                switch (e->kind) {
                case 1:
                    e->pos[1] = AreaGrid2_GetFloorY(e->pos);
                }
            } else if (e->timer > 0) {
                switch (e->kind) {
                case 1:
                    if (e->timer < 11) {
                        e->pos[0] += e->vel[0];
                        e->pos[1] += e->vel[1];
                        e->pos[2] += e->vel[2];
                        EffKushala29_SpinPart(o, e, 0x444);
                    }
                    break;
                case 0:
                    if (e->timer < 9) {
                        EffKushala29_SpinPart(o, e, 0x444);
                    }
                    break;
                case 2:
                    if (e->timer < 7) {
                        e->pos[0] += e->vel[0];
                        e->pos[1] += e->vel[1];
                        e->pos[2] += e->vel[2];
                    }
                    break;
                }
            }
            if (e->timer >= 0) {
                EffKushala29_DrawPart(o, e);
            }
            e->timer++;
            switch (e->kind) {
            case 1:
                if (e->timer >= 10) {
                    if (lim < o->ownerMotion + 10) {
                        e->active = 0;
                    } else {
                        e->timer = 0;
                    }
                }
                break;
            case 0:
                if (e->timer >= 8) {
                    if (lim < o->ownerMotion + 8) {
                        e->active = 0;
                    } else {
                        e->timer = 0;
                    }
                }
                break;
            case 2:
                if (e->timer >= 6) {
                    if (lim < o->ownerMotion + 6) {
                        e->active = 0;
                    } else {
                        e->timer = 0;
                    }
                }
                break;
            }
        }
    case 3:
        break;
    }
}

void EffKushala29_SpinPart(ObjAttach *o, Ku29Part *p, u16 n)
{
    if ((p->flags & 4) == 4) {
        p->rot[2] += (s32) n;
    } else {
        p->rot[2] = p->rot[2] - n;
    }
}

void EffKushala29_DrawPart(ObjAttach *o, Ku29Part *p)
{
    Ku29Buf *b = (Ku29Buf *) o->buf;
    u16 k;
    s16 t = *(s16 *) &p->ds->part;
    f32 a;
    void *c;

    switch (p->kind) {
    case 1:
        k = 0;
        break;
    case 0:
        k = 1;
        break;
    case 2:
        k = 2;
        break;
    }
    Key_LerpFloat(EffKushala29_AlphaKeys[k], &a, p->timer);
    p->alpha = 255.0f * a;
    switch (p->kind) {
    case 1:
        k = t + 1;
        break;
    case 0:
        k = 4;
        break;
    case 2:
        k = 5;
        break;
    }
    c = EffKushala29_VecKeys[k];
    switch (p->kind) {
    case 1:
        Key_LerpVec2(c, p->scale, p->timer);
        break;
    case 0:
    case 2:
        Key_LerpVec3(c, p->scale, p->timer);
        break;
    }
    (p->scale)[0] *= EffKushala29_HeightScale[b->size];
    (p->scale)[1] *= EffKushala29_HeightScale[b->size];
    (p->scale)[2] *= EffKushala29_HeightScale[b->size];
    switch (o->type) {
    case 0:
    case 2:
        if (p->ds != NULL) {
            Vec_Copy(p->ds->pos, p->pos);
            OT_AddByDepth(gDrawOtSorted, p->ds, 0x40U, 0);
        }
        break;
    case 1:
        break;
    }
}

void EffKushala29_End(ObjAttach *o)
{
    Ku29Part *p = ((Ku29Buf *) o->buf)->part;
    s16 n;
    s16 i;

    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free(o->drawSlot);
    }
    n = gEffKushala29Counts[o->type];
    for (i = 0; i < n; i++) {
        if (p->ds != NULL) {
            DrawSlot_Free(p->slot);
        }
        p++;
    }
}

void EffKushala29_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffKushala29_Draw(DrawSlot *d)
{
    f32 m[16];
    f32 t[16];
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    ObjAttach *o = d->owner;
    Ku29Buf *p;
    Ku29Part *e;
    ModelMat *tex;
    Model *bank;
    u16 flags;
    u32 col;
    s16 k;
    ModelMatSet *mdl;
    s16 id;

    p = (Ku29Buf *) o->buf;
    flags = 0;

    if (o->area != gScene.area) {
        return;
    }
    id = EmSlot_FindKind(o->aux);
    if (id == -1) {
        return;
    }
    bank = gScene.partModels[id];
    if (bank == NULL) {
        return;
    }
    if (bank->active == 0) {
        return;
    }
    tex = bank->mats;
    Gs_SetState(0x6C, 0);
    if (d->part == -1) {
        k = 0;
        Mtx_Scale(m, p->vec[0], p->vec[1], p->vec[2]);
        Mtx_RotateZ(m, (6.2831855f * (u16) p->spin) / 65536.0f);
        switch (o->type) {
        case 1:
        case 2:
            Mtx_RotateX(m, (6.2831855f * o->rotX) / 65536.0f);
            break;
        }
        Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
        Mtx_Translation(t, 0.0f, 0.0625f * (o->ownerMotion % 16), 0.0f);
        Gs_SetState(0x19, (s32) t);
        flags |= 2;
        col = -1;
    } else {
        Effect_GetColor(gScene.area, &r, &g, &b, &a);
        e = &p->part[d->part];
        switch (e->kind) {
        case 1:
            if ((e->flags & 2) == 2) {
                k = 1;
            } else {
                k = 2;
            }
            break;
        case 0:
            k = 5;
            break;
        case 2:
            k = 3;
            break;
        }
        Mtx_Scale(m, e->scale[0], e->scale[1], e->scale[2]);
        switch (e->kind) {
        case 1:
        case 0:
            Mtx_RotateZ(m, (6.2831855f * e->rot[2]) / 65536.0f);
            break;
        case 2:
            Mtx_RotateXYZ(m, (6.2831855f * e->rot[0]) / 65536.0f, (6.2831855f * e->rot[1]) / 65536.0f,
                          (6.2831855f * e->rot[2]) / 65536.0f);
            break;
        }
        switch (e->kind) {
        case 0:
        case 2:
            Mtx_RotateY(m, (6.2831855f * o->rotY) / 65536.0f);
            switch (Area_BaseId(gScene.map)) {
            case 3:
            case 0xB:
                Mtx_Translation(t, 0.0f, 0.25f, 0.0f);
                break;
            case 4:
            case 6:
                Mtx_Translation(t, 0.0f, 0.5f, 0.0f);
                break;
            default:
                Mtx_Translation(t, 0.0f, 0.0f, 0.0f);
                break;
            }
            Gs_SetState(0x19, (s32) t);
            break;
        case 1:
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        }
        col = r << 16 | (a & e->alpha) << 24 | g << 8 | b;
    }
    mdl = &bank->matSets[k];
    Mtx_SetTrans(m, d->pos[0], d->pos[1], d->pos[2]);
    Effect_DrawModel(mdl, m, col, flags, tex);
}

void EffKushala29_Impact(ObjAttach *o, s32 unused, f32 *pos)
{
    u16 kind = 0x1D;
    Ku29Buf *buf = (Ku29Buf *) o->buf;
    f32 scale = 1.0f;
    u8 n = 0xF;

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
            kind = 0x1C;
            break;
        case 2:
            scale = 1.0f;
            kind = 0x1D;
            break;
        case 3:
        case 4:
            scale = 0.7f;
            kind = 0x1F;
            break;
        case 5:
            scale = 1.0f;
            kind = 0x20;
            break;
        }
        break;
    case 1:
        scale = 0.7f;
        n = 0x11;
        if (buf->size != 1) {
            kind = 0x1C;
        } else {
            kind = 0x1F;
        }
        break;
    }
    if (o->area == gScene.area) {
        Em_PlaySeChecked(o->owner, 0x3C, 0, pos, 6, 0);
        StageObj_K1C_CreateAtYaw(pos, o->rotY, n, scale);
        Eff09_CreateOwned(o->owner, pos, kind);
    }
}

void EffKushala30_Create(Em *em, s32 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 30;
            o->type = kind;
            o->update = EffKushala30_Update;
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
            em->attackHit800 = 0;
        }
    }
}

void EffKushala30_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffKushala30_Setup(o);
        break;
    case 1:
        EffKushala30_Run(o);
        break;
    case 2:
        EffKushala30_End(o);
        break;
    case 3:
        EffKushala30_Run(o);
        break;
    case 4:
        EffKushala30_End(o);
        break;
    case 5:
        EffKushala30_End(o);
        break;
    case 9:
        EffKushala30_End(o);
        break;
    case 7:
        EffKushala30_End(o);
        break;
    case 6:
        EffKushala30_Free(o);
        
    default:
        break;
    }
}

void EffKushala30_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffKushala30_MotionDesc);
    o->hitList = gEffKushala30HitLists[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffKushala30_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 8:
        o->hitFrames = 99;
        break;
    case 9:
        switch (o->step) {
        case 0:
            if (m->motionId[0] == 0x3F6) {
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
            if (m->motionId[0] == 0x3F6) {
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
            if (m->motionId[0] == 0x409) {
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
        if (!(m->motionId[0] == 0x3FD || m->motionId[0] == 0x3FE || m->motionId[0] == 0x3FF) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 9:
    case 26:
        if (!(m->motionId[0] == 0x3F4 || m->motionId[0] == 0x3F5 || m->motionId[0] == 0x3F6) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    case 29:
        if (!(m->motionId[0] == 0x411 || m->motionId[0] == 0x409) || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    default:
        if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
            o->hitPhase = 0;
            o->state = 2;
        }
        break;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffKushala30_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffKushala30_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffShengaoren31_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 31;
            o->type = kind;
            o->update = EffShengaoren31_Update;
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
            em->attackHit800 = 0;
        }
    }
}

void EffShengaoren31_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffShengaoren31_Setup(o);
        break;
    case 1:
        EffShengaoren31_Run(o);
        break;
    case 2:
        EffShengaoren31_End(o);
        break;
    case 3:
        EffShengaoren31_Run(o);
        break;
    case 4:
        EffShengaoren31_End(o);
        break;
    case 5:
        EffShengaoren31_End(o);
        break;
    case 9:
        EffShengaoren31_End(o);
        break;
    case 7:
        EffShengaoren31_End(o);
        break;
    case 6:
        EffShengaoren31_Free(o);
        
    default:
        break;
    }
}

void EffShengaoren31_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffShengaoren31_MotionDesc);
    o->hitList = gEffShengaoren31HitLists[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffShengaoren31_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffShengaoren31_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffShengaoren31_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

void EffYamatsukami31_Create(Em *em, s8 kind)
{
    ObjAttach *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = ObjAttach_Alloc(0)) != NULL) {
            o->kind = 31;
            o->type = kind;
            o->update = EffYamatsukami31_Update;
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
            em->attackHit800 = 0;
        }
    }
}

void EffYamatsukami31_Update(ObjAttach *o)
{
    switch (o->state) {
    case 0:
        EffYamatsukami31_Setup(o);
        break;
    case 1:
        EffYamatsukami31_Run(o);
        break;
    case 2:
        EffYamatsukami31_End(o);
        break;
    case 3:
        EffYamatsukami31_Run(o);
        break;
    case 4:
        EffYamatsukami31_End(o);
        break;
    case 5:
        EffYamatsukami31_End(o);
        break;
    case 9:
        EffYamatsukami31_End(o);
        break;
    case 7:
        EffYamatsukami31_End(o);
        break;
    case 6:
        EffYamatsukami31_Free(o);
        
    default:
        break;
    }
}

void EffYamatsukami31_Setup(ObjAttach *o)
{
    Em *m = &gEms[o->idx];
    s32 a;

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    Player_TestSetFlags(o, 0x20);
    MoveObj_StartMotionOwned((struct MoveObj *)o, (s32) m, o->type, &EffYamatsukami31_MotionDesc);
    o->hitList = gEffYamatsukami31HitLists[o->hitListNo];
    o->hurtList = 0;
    o->area = m->area;
    if (o->hitDelay != 0) {
        a = Math_Absf(m->motionBlend[0] % 100);
        o->hitDelay += a - (s32) Math_Absf(m->motionFrame[0]) - 1;
        if ((s8) o->hitDelay < 0) {
            o->hitDelay = 0;
        }
    }
    o->drawSlotBuf = NULL;
}

void EffYamatsukami31_Run(ObjAttach *o)
{
    Em *m = &gEms[o->idx];

    switch (o->type) {
    case 0:
    case 16:
    case 17:
    case 21:
        o->hitFrames = 99;
        break;
    }
    if (m->motionId[0] != o->ownerMotion || o->hitPhase == 0) {
        o->hitPhase = 0;
        o->state = 2;
    }
    if (o->drawSlotBuf != NULL) {
        Vec_Copy(o->drawSlotBuf->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->drawSlotBuf, 0x20U, 0);
    }
}

void EffYamatsukami31_End(ObjAttach *o)
{
    o->hitPhase = 0;
    o->state = 6;
    o->visible = 0;
    if (o->drawSlotBuf != NULL) {
        DrawSlot_Free((s32) o->drawSlot);
    }
}

void EffYamatsukami31_Free(ObjAttach *o)
{
    ObjAttach_Free(o);
}

/* unk_6E0650's .data (0x8A8700-0x8AB600) */
HitShape gEffKushala29HitLists_0[2] = {
#include "game/008A8700.inc"
};
HitShape gEffKushala29HitLists_1[2] = {
#include "game/008A8750.inc"
};
HitShape gEffKushala29HitLists_2[2] = {
#include "game/008A87A0.inc"
};
HitShape gEffKushala29HitLists_3[2] = {
#include "game/008A87F0.inc"
};
HitShape gEffKushala29HitLists_4[2] = {
#include "game/008A8840.inc"
};
HitShape gEffKushala29HitLists_5[2] = {
#include "game/008A8890.inc"
};
HitShape *gEffKushala29HitLists[8] = {
    gEffKushala29HitLists_0, gEffKushala29HitLists_1, gEffKushala29HitLists_2, gEffKushala29HitLists_3,
    gEffKushala29HitLists_4, gEffKushala29HitLists_5, 0, 0,
};
u32 D_008A8900[66] = {
#include "game/008A8900.inc"
};
void *EffKushala29_MotionDesc = (void *)D_008A8900;
s8 EffKushala29_MotionIds[16] = {
#include "game/008A8A10.inc"
};
u32 EffKushala29_VecKeys_0[16] = {
#include "game/008A8A20.inc"
};
u32 EffKushala29_VecKeys_1[8] = {
#include "game/008A8A60.inc"
};
u32 EffKushala29_VecKeys_2[8] = {
#include "game/008A8A80.inc"
};
u32 EffKushala29_VecKeys_3[8] = {
#include "game/008A8AA0.inc"
};
u32 EffKushala29_VecKeys_4[16] = {
#include "game/008A8AC0.inc"
};
u32 EffKushala29_VecKeys_5[12] = {
#include "game/008A8B00.inc"
};
void *EffKushala29_VecKeys[8] = {
    (void *)EffKushala29_VecKeys_0, (void *)EffKushala29_VecKeys_1, (void *)EffKushala29_VecKeys_2, (void *)EffKushala29_VecKeys_3,
    (void *)EffKushala29_VecKeys_4, (void *)EffKushala29_VecKeys_5, NULL, NULL,
};
u32 EffKushala29_AlphaKeys_0[8] = {
#include "game/008A8B50.inc"
};
u32 EffKushala29_AlphaKeys_1[8] = {
#include "game/008A8B70.inc"
};
u32 EffKushala29_AlphaKeys_2[6] = {
#include "game/008A8B90.inc"
};
void *EffKushala29_AlphaKeys[3] = {
    (void *)EffKushala29_AlphaKeys_0, (void *)EffKushala29_AlphaKeys_1, (void *)EffKushala29_AlphaKeys_2,
};
u8 EffKushala29_Params0[48] = {
#include "game/008A8BC0.inc"
};
f32 EffKushala29_Dir0[12] = {
#include "game/008A8BF0.inc"
};
f32 EffKushala29_BoneOfs2[6] = {
#include "game/008A8C20.inc"
};
s16 gEffKushala29Counts[4] = {
#include "game/008A8C38.inc"
};
s16 gEffKushala29Limits[8] = {
#include "game/008A8C40.inc"
};
f32 EffKushala29_HeightScale[8] = {
#include "game/008A8C50.inc"
};
HitShape gEffKushala30HitLists_0[8] = {
#include "game/008A8C70.inc"
};
HitShape gEffKushala30HitLists_1[6] = {
#include "game/008A8DB0.inc"
};
HitShape gEffKushala30HitLists_2[3] = {
#include "game/008A8EA0.inc"
};
HitShape gEffKushala30HitLists_3[3] = {
#include "game/008A8F20.inc"
};
HitShape gEffKushala30HitLists_4[6] = {
#include "game/008A8FA0.inc"
};
HitShape gEffKushala30HitLists_5[2] = {
#include "game/008A9090.inc"
};
HitShape gEffKushala30HitLists_6[8] = {
#include "game/008A90E0.inc"
};
HitShape gEffKushala30HitLists_7[6] = {
#include "game/008A9220.inc"
};
HitShape gEffKushala30HitLists_8[9] = {
#include "game/008A9310.inc"
};
HitShape gEffKushala30HitLists_9[9] = {
#include "game/008A9480.inc"
};
HitShape gEffKushala30HitLists_10[3] = {
#include "game/008A95F0.inc"
};
HitShape gEffKushala30HitLists_11[3] = {
#include "game/008A9670.inc"
};
HitShape gEffKushala30HitLists_12[6] = {
#include "game/008A96F0.inc"
};
HitShape gEffKushala30HitLists_13[2] = {
#include "game/008A97E0.inc"
};
HitShape gEffKushala30HitLists_14[2] = {
#include "game/008A9830.inc"
};
HitShape gEffKushala30HitLists_15[2] = {
#include "game/008A9880.inc"
};
HitShape gEffKushala30HitLists_16[2] = {
#include "game/008A98D0.inc"
};
HitShape *gEffKushala30HitLists[20] = {
    gEffKushala30HitLists_0, gEffKushala30HitLists_1, gEffKushala30HitLists_2, gEffKushala30HitLists_3,
    gEffKushala30HitLists_4, gEffKushala30HitLists_5, gEffKushala30HitLists_6, gEffKushala30HitLists_7,
    gEffKushala30HitLists_8, gEffKushala30HitLists_9, gEffKushala30HitLists_10, gEffKushala30HitLists_11,
    gEffKushala30HitLists_12, gEffKushala30HitLists_13, gEffKushala30HitLists_14, gEffKushala30HitLists_15,
    gEffKushala30HitLists_16, 0, 0, 0,
};
u32 D_008A9970[186] = {
#include "game/008A9970.inc"
};
void *EffKushala30_MotionDesc = (void *)D_008A9970;
HitShape gEffShengaoren31HitLists_0[5] = {
#include "game/008A9C60.inc"
};
HitShape gEffShengaoren31HitLists_1[3] = {
#include "game/008A9D30.inc"
};
HitShape gEffShengaoren31HitLists_2[4] = {
#include "game/008A9DB0.inc"
};
HitShape gEffShengaoren31HitLists_3[4] = {
#include "game/008A9E50.inc"
};
HitShape gEffShengaoren31HitLists_4[3] = {
#include "game/008A9EF0.inc"
};
HitShape gEffShengaoren31HitLists_5[4] = {
#include "game/008A9F70.inc"
};
HitShape gEffShengaoren31HitLists_6[4] = {
#include "game/008AA010.inc"
};
HitShape gEffShengaoren31HitLists_7[3] = {
#include "game/008AA0B0.inc"
};
HitShape gEffShengaoren31HitLists_8[3] = {
#include "game/008AA130.inc"
};
HitShape gEffShengaoren31HitLists_9[3] = {
#include "game/008AA1B0.inc"
};
HitShape gEffShengaoren31HitLists_10[3] = {
#include "game/008AA230.inc"
};
HitShape gEffShengaoren31HitLists_11[3] = {
#include "game/008AA2B0.inc"
};
HitShape gEffShengaoren31HitLists_12[3] = {
#include "game/008AA330.inc"
};
HitShape gEffShengaoren31HitLists_13[3] = {
#include "game/008AA3B0.inc"
};
HitShape *gEffShengaoren31HitLists[16] = {
    gEffShengaoren31HitLists_0, gEffShengaoren31HitLists_1, gEffShengaoren31HitLists_2, gEffShengaoren31HitLists_3,
    gEffShengaoren31HitLists_4, gEffShengaoren31HitLists_5, gEffShengaoren31HitLists_6, gEffShengaoren31HitLists_7,
    gEffShengaoren31HitLists_8, gEffShengaoren31HitLists_9, gEffShengaoren31HitLists_10, gEffShengaoren31HitLists_11,
    gEffShengaoren31HitLists_12, gEffShengaoren31HitLists_13, 0, 0,
};
u32 D_008AA470[156] = {
#include "game/008AA470.inc"
};
void *EffShengaoren31_MotionDesc = (void *)D_008AA470;
HitShape gEffYamatsukami31HitLists_0[14] = {
#include "game/008AA6F0.inc"
};
HitShape gEffYamatsukami31HitLists_1[7] = {
#include "game/008AA920.inc"
};
HitShape gEffYamatsukami31HitLists_2[7] = {
#include "game/008AAA40.inc"
};
HitShape gEffYamatsukami31HitLists_3[4] = {
#include "game/008AAB60.inc"
};
HitShape gEffYamatsukami31HitLists_4[4] = {
#include "game/008AAC00.inc"
};
HitShape gEffYamatsukami31HitLists_5[13] = {
#include "game/008AACA0.inc"
};
HitShape gEffYamatsukami31HitLists_6[7] = {
#include "game/008AAEB0.inc"
};
HitShape gEffYamatsukami31HitLists_7[2] = {
#include "game/008AAFD0.inc"
};
HitShape gEffYamatsukami31HitLists_8[2] = {
#include "game/008AB020.inc"
};
HitShape gEffYamatsukami31HitLists_9[14] = {
#include "game/008AB070.inc"
};
HitShape gEffYamatsukami31HitLists_10[2] = {
#include "game/008AB2A0.inc"
};
HitShape gEffYamatsukami31HitLists_11[2] = {
#include "game/008AB2F0.inc"
};
HitShape gEffYamatsukami31HitLists_12[2] = {
#include "game/008AB340.inc"
};
HitShape *gEffYamatsukami31HitLists[16] = {
    gEffYamatsukami31HitLists_0, gEffYamatsukami31HitLists_1, gEffYamatsukami31HitLists_2, gEffYamatsukami31HitLists_3,
    gEffYamatsukami31HitLists_4, gEffYamatsukami31HitLists_5, gEffYamatsukami31HitLists_6, gEffYamatsukami31HitLists_7,
    gEffYamatsukami31HitLists_8, gEffYamatsukami31HitLists_9, gEffYamatsukami31HitLists_10, gEffYamatsukami31HitLists_11,
    gEffYamatsukami31HitLists_12, 0, 0, 0,
};
u32 D_008AB3D0[138] = {
#include "game/008AB3D0.inc"
};
void *EffYamatsukami31_MotionDesc = (void *)D_008AB3D0;
