/*
 * Game overlay code at 0x880D50-0x88E760, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "sprite.h"
#include "scene.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "area.h"
/* prototypes of callees (round 13) */
void DrawSlot_Free(s32 i);
/* the calls that mask the value again (u8 or & 0xFF) cast it to s32 first: MWCC keeps the game's andi that way */
u8 Player_InCurrentArea(void *obj);
s32 StageObj_K1C_OwnerActive(Em *em);

/* Built at optimization level 4: StageObj_RandomDistinct matches only at 4 and
 * StageObj_K1C_RunChain is closer there; StageObj_K1C_NextStepD only at 3 (a pair below).
 * One unit for K1C: every helper from 0x889030 to 0x88A8E0 serves StageObj_K1C_SetupSeq
 * and StageObj_K1C_RunSeq. */
#pragma optimization_level 4

/* a part of a kind 0x1C object of types 0 and 0xB (0x64 bytes, in its blocks) */
typedef struct Obj1CPartA {
    s16 slot;                   /* 0x00: its draw slot */
    s16 kind;                   /* 0x02: 0 the first part, 1 the second, others follow */
    f32 pos[3];                 /* 0x04 */
    s16 unk10;                  /* 0x10: 8 - its index; a frame (texture scroll) */
    s16 unk12;                  /* 0x12: -1 - its index */
    u16 unk14;                  /* 0x14: extra angles of the other parts (type 0xB) */
    u16 unk16;                  /* 0x16 */
    u16 unk18;                  /* 0x18: a random angle (StageObj_K1C_RandomSpin) */
    s16 unk1A;                  /* 0x1A: its speed: 0xB6 for the first part; -0x100..0xFF */
    f32 scale;                  /* 0x1C: the object's, + 0.2 per part */
    DrawSlot *node;                /* 0x20 */
    f32 sc[3];                  /* 0x24: a scale from a curve (1.0 at first) */
    s32 color;                  /* 0x30: from a curve, -1 at first */
    f32 base[2][3];             /* 0x34: the last two base points */
    f32 dir[2][3];              /* 0x4C: the last two directions */
} Obj1CPartA;

/* a part of a kind 0x1C object of the other types (0x40 bytes): a step sequence (StageObj_K1C_NextStepA) */
typedef struct Obj1CPart {
    s16 slot;                   /* 0x00: its draw slot */
    u8 idx;                     /* 0x02: the entry of the step table, 0xFF: done */
    u8 sub;                     /* 0x03 */
    f32 pos[3];                 /* 0x04 */
    u32 color;                  /* 0x10 */
    f32 sc[3];                  /* 0x14: a scale (StageObj_K1C_DrawSeq) */
    f32 scale;                  /* 0x20 */
    DrawSlot *node;                /* 0x24 */
    s16 wait;                   /* 0x28: frames before the step */
    u16 unk2A;                  /* 0x2A: angles (x, y, z) */
    u16 unk2C;                  /* 0x2C */
    u16 unk2E;                  /* 0x2E */
    s16 unk30;                  /* 0x30: their speeds */
    s16 unk32;                  /* 0x32 */
    s16 unk34;                  /* 0x34 */
    u8 unk36;                   /* 0x36 */
    u8 unk37;                   /* 0x37 */
    f32 unk38;                  /* 0x38 */
    f32 unk3C;                  /* 0x3C */
} Obj1CPart;


/* prototypes */
extern u16 StageObj_K1C_StepBFrames[];
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
u16 Vec_Elevation(f32 *a, f32 *b);
u16 Vec_Heading(f32 *a, f32 *b);
void StageObj_K1C_RandomSpin(Obj1CPartA *w);
extern f32 *gStageObjK1C_ScaleCurves[];
extern u8 *gStageObjK1C_ColorKeys[];
extern s16 gStageObjK1C_ScaleCurveIdx[];
extern s16 gStageObjK1C_ColorCurveIdx[];
s32 Em_PlaySeChecked(struct Em *a0, s32 id, s32 a, void *a3, s32 b, s32 c);
extern void *StageObj_K1B_ScaleKeys[];
extern void *StageObj_K1B_AlphaKeys[];
extern f32 StageObj_K1B_SpinDeg[];
extern u8 StageObj_K1B_PartFalls[];
extern u8 StageObj_K1B_Type1ScaleKeys[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpFloat(f32 *k, f32 *out, f32 t);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
void StageObj_K1C_FollowBone(StageObj *o, Obj1CPartA *w, Obj1CPartA *prev, f32 *m, f32 *dir, s16 first);
void StageObj_K1C_Draw(DrawSlot *node);
s32 Effect_DrawModelAdd(void *arg0, f32 *m, void *mats);
s32 func_00761C70(Em *em);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_DirToAngles(f32 *dir, s16 *pitch, s16 *yaw);
void Vec_RotateX(f32 *v, f32 a);
void Vec_RotateY(f32 *v, f32 a);
f32 StageObj_K1C_GrowFactor(StageObj *o);
extern s16 StageObj_K1C_StepCFrames[];
extern u16 StageObj_K1C_StepAFrames[];
f32 Rand_Unit(void);
extern u8 D_008D6738[];
extern u16 StageObj_K1C_StepCWaits[];
extern u8 StageObj_K1C_StepCKinds17[];
extern u16 StageObj_K1C_StepCWaits17[];
extern u8 StageObj_K1C_StepDKinds[];
extern u16 StageObj_K1C_StepDWaits[];
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
f32 Math_Sin(f32 x);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern f32 gAreaFlyerScaleKeys[];
extern u8 gAreaFlyerColorKeys[];
extern f32 gCamMatrix[];
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *n);
void Mtx_Identity(f32 *m);
void Vec_Normalize(f32 *v);
s32 Gs_SetState(u8 reg, u32 v);
s32 Effect_DrawModel(void *mdl, f32 *m, s32 color, u16 flags, void *a4);
void Key_LerpVec2(f32 *a, f32 *b, f32 f);
void Color_EvalKeys(s32 *a, s32 t, u32 *c);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void StageObj_AreaFlyer_Draw(DrawSlot *node);
u16 Rand_Next(s32 stream);
void StageObj_K1C_Setup(StageObj *o);
void StageObj_K1C_Run(StageObj *o);
void StageObj_K1C_End(StageObj *o);
void StageObj_K1C_Free(StageObj *o);
void StageObj_AreaFlyer_Setup(StageObj *o);
void StageObj_AreaFlyer_Run(StageObj *o);
void StageObj_AreaFlyer_End(StageObj *o);
void StageObj_AreaFlyer_Free(StageObj *o);
void StageObj_AreaFlyer_GetCenter(f32 *out, u8 idx);
void StageObj_Gun_Setup(StageObj *o);
void StageObj_Gun_Run(StageObj *o);
void StageObj_Gun_End(StageObj *o);
void StageObj_Gun_Free(StageObj *o);
void Vec_Copy(f32 *out, f32 *in);
extern f32 gAreaFlyerCenters[][3];
extern u8 StageObj_K1C_StepBKinds[];
extern u16 StageObj_K1C_StepBStart[];
f32 Rand_Signed(void);
extern u8 D_008D6138[];
extern u16 StageObj_K1C_StepAStart[];
void StageObj_K1B_Update(StageObj *o);
StageObj *StageObj_K1C_Alloc(u8 k);
extern s16 gStageObjK1C_Counts[];
void StageObj_K1C_EndChain(StageObj *o, s16 n);
void StageObj_K1C_EndSeq(StageObj *o, s16 n);
void Em_BonePos(void *obj, s16 bone, f32 *out);
extern u8 *gStageObjK1C_ModelSets[];
s16 EmSlot_FindKind(s16 k);
extern u8 StageObj_K1B_PartCounts[];
void StageObj_K1C_SetupChain(StageObj *o);
void StageObj_K1C_SetupSeq(StageObj *o);
void StageObj_K1C_DrawChain(DrawSlot *node);
void StageObj_K1C_DrawSeq(DrawSlot *node);
StageObj *StageObj_K1C_CreateOnEm(Em *em, u8 k, f32 scale);
StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale);
void StageObj_K1C_RunChain(StageObj *o);
void StageObj_K1C_RunSeq(StageObj *o);
extern s16 gStageObjK1C_Limits[];
/* end of prototypes */

void StageObj_K1B_Run(StageObj *o)
{
    f32 a;
    f32 v[3];
    Obj1BPart *p = (Obj1BPart *) o->buf;
    u8 done = 0;
    u8 i;

    if (o->owner == NULL) {
        o->state++;
        return;
    }
    for (i = 0; i < StageObj_K1B_PartCounts[o->type]; i++) {
        if (++p->t >= p->life) {
            p++;
            if (++done == StageObj_K1B_PartCounts[o->type]) {
                o->state++;
                o->visible = 0;
                return;
            }
            continue;
        }
        switch (o->type) {
        case 0:
            if (i == 0 || i == 4) {
                if (p->pos[1] < p->ground - 20.0f) {
                    p->life = 0;
                }
            }
            if (StageObj_K1B_ScaleKeys[i] != NULL) {
                Key_LerpVec3(StageObj_K1B_ScaleKeys[i], p->scale, p->t);
            }
            Key_LerpFloat(StageObj_K1B_AlphaKeys[i], &a, p->t);
            p->alpha = 255.0f * a;
            if (o->var0 == 0) {
                p->rot[2] += (u16) (s32) (0.5f + (65536.0f * StageObj_K1B_SpinDeg[i]) / 360.0f);
            } else {
                p->rot[2] -= (u16) (s32) (0.5f + (65536.0f * StageObj_K1B_SpinDeg[i]) / 360.0f);
            }
            if (StageObj_K1B_PartFalls[i] != 0) {
                p->pos[1] -= p->fall;
                if (p->fall < 20.0f) {
                    p->fall += 2.0f;
                }
            }
            p->pos[1] -= p->fall;
            p->pos[0] += p->vel[0];
            p->pos[1] += p->vel[1];
            p->pos[2] += p->vel[2];
            break;
        case 1:
            if (p->t > 0) {
                if (p->t == 1) {
                    Em_GetBonePos(o->owner, 2, v);
                    p->pos[0] += v[0];
                    p->pos[1] += v[1] - 50.0f;
                    p->pos[2] += v[2];
                }
                p->pos[1] -= p->fall;
                if (p->fall < 8.0f) {
                    p->fall += 0.5f;
                }
                Key_LerpVec3((f32 *)StageObj_K1B_Type1ScaleKeys, p->scale, p->t);
            }
            break;
        }
        if (p->node != NULL) {
            p->node->pos[0] = p->pos[0];
            p->node->pos[1] = p->pos[1];
            p->node->pos[2] = p->pos[2];
            if (o->type == 0 && i == 0) {
                OT_AddByDepth(gDrawOt32, p->node, 0x20, 0);
            } else {
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
        }
        p++;
    }
}

void StageObj_K1B_End(StageObj *o)
{
    Obj1BPart *p = (Obj1BPart *) o->buf;
    u8 i;

    o->state++;
    for (i = 0; i < StageObj_K1B_PartCounts[o->type]; i++) {
        if (p->node != NULL) {
            DrawSlot_Free(p->slot);
        }
        p++;
    }
}

void StageObj_K1B_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_K1B_Draw(void *arg)
{
    ModelMat *mats;
    ModelMatSet *model;
    u8 bill = 1;
    s32 color;
    DrawSlot *node = arg;
    StageObj *o = node->owner;
    Model *set = gStageModelSet;
    Obj1BPart *p = (Obj1BPart *) o->buf + node->part;
    f32 scale[3];
    f32 rot[3];
    f32 m[16];

    if (set != NULL && set->active != 0 && o->visible != 0) {
        color = (p->alpha << 24) | 0xFFFFFF;
        mats = set->mats;
        model = &set->matSets[p->model];
        scale[0] = p->scale[0];
        scale[1] = p->scale[1];
        scale[2] = p->scale[2];
        rot[0] = (6.2831855f * p->rot[0]) / 65536.0f;
        rot[1] = (6.2831855f * p->rot[1]) / 65536.0f;
        rot[2] = (6.2831855f * p->rot[2]) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 7, m);
        switch (o->type) {
        case 0:
            if (node->part == 0 || node->part == 4) {
                bill = 0;
            }
            break;
        }
        if (bill) {
            Mtx_Concat3x3(m, gCamMatrix);
        }
        if (o->type == 0 && (node->part == 0 || node->part == 4)) {
            Gs_SetState(0x67, -1);
            Effect_DrawModelAdd(model, m, mats);
        } else {
            Effect_DrawModel(model, m, color, 0, mats);
        }
    }
}

s32 Obj_InCurrentArea(void *obj);
void StageObj_K1B_Create(s8 k, s16 *arg)
{
    StageObj *o;

    if (Obj_InCurrentArea(arg) & 0xFF) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x1B;
            o->update = StageObj_K1B_Update;
            o->type = k;
            o->owner = (void *) arg;
        }
    }
}

void StageObj_K1C_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K1C_Setup(o);
        break;
    case 1:
        StageObj_K1C_Run(o);
        break;
    case 2:
        StageObj_K1C_End(o);
        break;
    case 3:
        StageObj_K1C_Free(o);
        break;
    }
}

void StageObj_K1C_Setup(StageObj *o)
{
    o->state++;
    o->var0 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    switch (o->type) {
    case 0:
    case 0xB:
        StageObj_K1C_SetupChain(o);
        break;
    default:
        StageObj_K1C_SetupSeq(o);
        break;
    }
}

void StageObj_K1C_SetupChain(StageObj *o)
{
    Obj1CPartA *p = (Obj1CPartA *) o->buf;
    Em *em = o->owner;
    Obj1CPartA *prev;
    Obj1CPartA *first;
    f32 *m;
    s16 i;
    s16 n;

    n = gStageObjK1C_Counts[o->type];

    switch (o->type) {
    case 0:
        m = (f32 *)Em_Bone(em, 0x25);
        first = p;
        break;
    default:
        StageObj_K1C_Free(o);
        return;
    }
    for (i = 0; i < n; i++, p++) {
        p->unk10 = 0;
        p->kind = i;
        p->scale = o->scale;
        switch (o->type) {
        case 0:
            StageObj_K1C_FollowBone(o, p, prev, m, first->dir[0], 0);
            p->unk10 = 8 - i;
            p->unk12 = -1 - i;
            p->unk18 = Rand_Next(1);
            if (p->kind == 0) {
                p->unk1A = 0xB6;
            } else {
                p->unk1A = 0;
            }
            p->scale += 0.2f * i;
            p->sc[0] = 1.0f;
            p->sc[1] = 1.0f;
            p->sc[2] = 1.0f;
            p->color = -1;
            prev = p;
            break;
        }
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K1C_Draw;
        } else {
            p->node = NULL;
        }
    }
}

extern u32 StageObj_K1C_T1Delay[], StageObj_K1C_T1Rise[], StageObj_K1C_T1Scale[], StageObj_K1C_T2Delay[], StageObj_K1C_T6Index[], StageObj_K1C_T6Rise[];
extern u32 StageObj_K1C_T8Depth[], StageObj_K1C_T10Delay[], StageObj_K1C_T24Index[], StageObj_K1C_T26Delay[], StageObj_K1C_T30Delay[], StageObj_K1C_T6Frames[];
extern u32 StageObj_K1C_T32Index[], D_008D7318[], StageObj_K1C_T2Frames[], StageObj_K1C_T33Frames[], StageObj_K1C_T33Index[], StageObj_K1C_T33Delay[];
extern u32 StageObj_K1C_T38Delay[], StageObj_K1C_T0Frames[];
void StageObj_RandomDistinct(s16 n, s16 count, s16 *out);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void StageObj_K1C_PlaceBehindOwner(StageObj *o);
void StageObj_K1C_GetOwnerBone(StageObj *o, f32 *out);
void StageObj_K1C_Jitter(Obj1CPart *w, f32 r);
void StageObj_K1C_NextStepC(StageObj *o, Obj1CPart *w);
void StageObj_K1C_NextStepD(StageObj *o, Obj1CPart *w);

void StageObj_K1C_SetupSeq(StageObj *o)
{
    f32 t[3];
    f32 v[3];
    s16 dist[4];
    Obj1CPart *p = (Obj1CPart *) o->buf;
    Em *em = o->owner;
    s16 n = gStageObjK1C_Counts[o->type];
    f32 *m;
    s16 i;
    s16 cnt;
    f32 sc;
    f32 y;
    u16 r;

    switch (o->type) {
    case 3:
        Mtx_GetTrans(o->pos, Em_Bone(em, 0x25)->world);
        break;
    case 4:
        Vec_Copy(t, o->pos);
        Vec_Copy(o->pos, p->pos);
        t[0] += (Rand_Next(1) & 0x1FF) - 0x100;
        t[1] += (Rand_Next(1) & 0x1FF) - 0x100;
        t[2] += (Rand_Next(1) & 0x1FF) - 0x100;
        break;
    case 5:
    case 6:
    case 14:
        r = Rand_Next(1);
        break;
    case 7:
        m = (f32 *)Em_Bone(em, 0x12);
        v[0] = 0.0f;
        v[1] = -70.0f;
        v[2] = 42.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        t[0] += v[0];
        t[1] += v[1];
        t[2] += v[2];
        r = Rand_Next(1);
        switch (em->type) {
        case 0x33:
        default:
            o->var1 = 0;
            break;
        case 0x34:
            o->var1 = 1;
            break;
        }
        break;
    case 8:
        m = (f32 *)Em_Bone(em, 0x12);
        v[0] = 0.0f;
        v[1] = -60.0f;
        v[2] = 45.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        o->angle = em->rotY;
        break;
    case 9:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 330.0f * o->scale;
        Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        break;
    case 10:
    case 18:
        m = (f32 *)Em_Bone(em, 0x25);
        v[0] = 0.0f;
        v[1] = -60.0f;
        v[2] = 150.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        o->angle += em->rotY;
        break;
    case 12:
        o->var0 = 1;
        o->var1 = 0;
        o->var2 = 0;
        o->ownerIndex = 0;
        o->angle = em->rage;
        o->scale = em->scaleX;
        break;
    case 13:
        switch (em->type) {
        case 0x36:
        case 0x3C:
        default:
            o->var1 = 0;
            m = (f32 *)Em_Bone(em, 0x27);
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 100.0f;
            break;
        case 0x40:
        case 0x41:
            o->var1 = 1;
            m = (f32 *)Em_Bone(em, 0x27);
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 100.0f;
            break;
        case 0x3B:
            o->var1 = 2;
            m = (f32 *)Em_Bone(em, 0x29);
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 30.0f;
            break;
        case 0x44:
            o->var1 = 3;
            m = (f32 *)Em_Bone(em, 0xB);
            v[0] = 0.0f;
            v[1] = -10.0f;
            v[2] = 20.0f;
            break;
        }
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        t[0] += v[0];
        t[1] += v[1];
        t[2] += v[2];
        r = Rand_Next(1);
        break;
    case 15:
    case 17:
        o->var2 = 0;
        break;
    case 16:
        Mtx_GetTrans(o->pos, Em_Bone(em, 1)->world);
        o->pos[1] = em->floorY;
        o->angle = em->rotY;
        r = Rand_Next(1);
        break;
    case 19:
        m = (f32 *)Em_Bone(em, 0x1C);
        v[0] = 0.0f;
        v[1] = -80.0f;
        v[2] = 0.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        o->angle = em->rotY;
        y = em->floorY;
        break;
    case 20:
        m = (f32 *)Em_Bone(em, 0x1C);
        v[0] = 0.0f;
        v[1] = -50.0f;
        v[2] = 0.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        Em_PlaySeChecked((struct Em *)em, 0x25, 0, o->pos, 6, 0);
        break;
    case 21:
        m = (f32 *)Em_Bone(em, 1);
        v[0] = 0.0f;
        v[1] = 100.0f;
        v[2] = -30.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        Em_PlaySeChecked((struct Em *)em, 0x25, 0, o->pos, 6, 0);
        Em_PlaySeChecked((struct Em *)em, 0x18, 0, o->pos, 6, 0);
        break;
    case 23:
        m = (f32 *)Em_Bone(em, 0x11);
        v[0] = 0.0f;
        v[1] = -110.0f;
        v[2] = 80.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        o->angle = em->rotY;
        break;
    case 24:
        Em_BonePos(em, 4, o->pos);
        o->pos[1] -= 100.0f;
        o->angle = em->rotY;
        r = Rand_Next(1);
        Em_PlaySeChecked((struct Em *)em, 0x3C, 0, o->pos, 7, 0);
        break;
    case 25:
        StageObj_K1C_PlaceBehindOwner(o);
        break;
    case 26:
        StageObj_K1C_PlaceBehindOwner(o);
        break;
    case 28:
        m = (f32 *)Em_Bone(em, 0x1F);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 100.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        o->angle2 += (Rand_Next(1) & 0x3FF) - 0x200;
        o->angle += (Rand_Next(1) & 0x3FF) - 0x200;
        break;
    case 29:
        o->var0 = 0;
        o->var1 = 0;
        o->var2 = 0;
        o->ownerIndex = 0;
        o->scale = em->scaleX;
        o->angle = 0;
        break;
    case 32:
    case 33:
        StageObj_RandomDistinct(4, 4, dist);
        cnt = 0;
        break;
    case 34:
        Vec_Copy(t, o->pos);
        o->pos[1] = em->floorY;
        break;
    case 37:
        m = (f32 *)Em_Bone(em, 0x13);
        v[0] = 0.0f;
        v[1] = -50.0f;
        v[2] = 60.0f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        break;
    case 39:
        Em_BonePos(em, 0x10, o->pos);
        r = Rand_Next(1);
        break;
    case 40:
        StageObj_K1C_GetOwnerBone(o, t);
        if (o->var2 == 0) {
            Em_PlaySeChecked((struct Em *)em, 0x36, 0, t, 8, 0);
        } else {
            Em_PlaySeChecked((struct Em *)em, 0x37, 0, t, 8, 0);
        }
        r = Rand_Next(1);
        break;
    }
    for (i = 0; i < n; i++, p++) {
        p->wait = 0;
        p->idx = i;
        p->sub = 0;
        p->scale = o->scale;
        p->unk36 = 1;
        p->unk37 = 1;
        p->unk2A = 0;
        p->unk2C = 0;
        p->unk2E = 0;
        p->unk30 = 0;
        p->unk32 = 0;
        p->unk34 = 0;
        switch (o->type) {
        case 1:
            Vec_Copy(p->pos, o->pos);
            p->wait = -((s16 *) StageObj_K1C_T1Delay)[i];
            p->pos[1] += ((f32 *) StageObj_K1C_T1Rise)[i];
            p->scale *= ((f32 *) StageObj_K1C_T1Scale)[i];
            switch (i) {
            case 0:
                p->idx = 0;
                p->unk2E = Rand_Next(1);
                p->unk36 = 0;
                break;
            case 1:
                p->idx = 1;
                p->unk36 = 0;
                break;
            case 2:
            case 3:
                p->idx = 2;
                p->unk2A = Rand_Next(1);
                p->unk2C = Rand_Next(1);
                p->unk2E = Rand_Next(1);
                p->unk30 = (Rand_Next(1) & 0x1FF) - 0x100;
                p->unk32 = (Rand_Next(1) & 0x1FF) - 0x100;
                p->unk34 = (Rand_Next(1) & 0x1FF) - 0x100;
                p->unk37 = 0;
                break;
            case 4:
            case 5:
                p->idx = 3;
                p->unk2E = Rand_Next(1);
                p->unk34 = (Rand_Next(1) & 0x1FF) - 0x100;
                break;
            case 6:
            case 7:
            case 8:
                p->idx = 4;
                p->unk2E = Rand_Next(1);
                p->unk34 = (Rand_Next(1) & 0xFF) - 0x80;
                p->unk36 = 1;
                p->pos[0] += 75.0f * Rand_Signed();
                p->pos[1] += 35.0f * Rand_Signed();
                p->pos[2] += 25.0f * Rand_Signed();
                p->unk36 = 0;
                break;
            }
            break;
        case 2:
            Vec_Copy(p->pos, o->pos);
            p->wait = -((s16 *) StageObj_K1C_T2Delay)[i];
            switch (i) {
            case 0:
            case 1:
            case 2:
                p->idx = 0;
                p->unk2A = Rand_Next(1);
                p->unk2C = Rand_Next(1);
                p->unk2E = Rand_Next(1);
                p->unk30 = (Rand_Next(1) & 0x3F) - 0x20;
                p->unk32 = (Rand_Next(1) & 0x3F) - 0x20;
                p->unk34 = (Rand_Next(1) & 0x3F) - 0x20;
                p->unk37 = 0;
                break;
            case 3:
                p->idx = 1;
                break;
            case 4:
                p->idx = 2;
                break;
            }
            break;
        case 3:
            Vec_Copy(p->pos, o->pos);
            p->unk2E = Rand_Next(1);
            p->unk34 = 0xB61;
            p->unk37 = 0;
            break;
        case 4:
            Vec_Copy(p->pos, t);
            p->unk2E = Rand_Next(1);
            p->unk34 = (Rand_Next(1) & 0xFF) - 0x80;
            p->scale *= 0.8f + 0.4f * Rand_Unit();
            break;
        case 5:
        case 6:
            p->idx = ((s16 *) StageObj_K1C_T6Index)[i];
            Vec_Copy(p->pos, o->pos);
            p->pos[1] += o->scale * ((f32 *) StageObj_K1C_T6Rise)[i];
            switch (p->idx) {
            case 0:
                p->unk37 = 0;
                p->unk2C = Rand_Next(1);
                break;
            case 1:
            default:
                p->wait = (i - 1) * -2;
                p->unk2E = Rand_Next(1);
                p->unk34 = 0x32;
                if (r & (1 << i)) {
                    p->unk34 = -p->unk34;
                }
                p->unk38 = 10.0f;
                break;
            }
            break;
        case 7:
            if (i <= 0) {
                p->idx = 0;
                p->unk34 = r & 1;
            } else {
                p->idx = ((r >> i) & 1) + 1;
                p->scale *= 1.0f - 0.16f * (i - 1);
                p->unk38 = p->scale * ((70.0f + 20.0f * Rand_Unit()) / 8.0f);
                p->wait = (i - 1) * -5;
            }
            Vec_Copy(p->pos, t);
            p->unk2E = Rand_Next(1);
            break;
        case 8:
            Vec_Copy(p->pos, o->pos);
            if (i <= 0) {
                p->idx = 0;
                p->wait = -1;
                p->unk37 = 0;
                p->unk2C = o->angle;
                p->scale *= 0.7f;
            } else if (i < 2) {
                p->idx = 1;
            } else {
                p->idx = 2;
                p->unk34 = 0xB6;
                if (r & (1 << i)) {
                    p->unk34 = -p->unk34;
                }
                p->wait = -1 - (s16) (i - 2) * 3;
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = p->scale * ((f32 *) StageObj_K1C_T8Depth)[(s16) (i - 2)];
                Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
                Vec_Add(p->pos, p->pos, v);
                p->unk38 = 10.0f;
            }
            p->unk2E = Rand_Next(1);
            break;
        case 9:
            Vec_Copy(p->pos, o->pos);
            if (i <= 0) {
                p->idx = 0;
                p->unk34 = 0x36;
            } else if (i < 4) {
                p->idx = 1;
                p->wait = (s16) (i - 1) * -8;
                StageObj_K1C_Jitter(p, 150.0f);
                p->unk38 = 10.0f;
            } else {
                p->idx = 2;
                p->wait = -10 - (s16) (i - 4) * 3;
                StageObj_K1C_Jitter(p, 300.0f);
            }
            p->unk2E = Rand_Next(1);
            break;
        case 10:
            Vec_Copy(p->pos, o->pos);
            p->wait = -((s16 *) StageObj_K1C_T10Delay)[i];
            if (i <= 0) {
                p->idx = 0;
                p->unk2A = o->angle2;
                p->unk2C = o->angle;
                p->unk37 = 0;
                p->unk2E = Rand_Next(1);
            } else if (i < 2) {
                p->idx = 1;
            } else {
                p->idx = 2;
                p->unk2A = o->angle2 + (u16) (8192.0f * Rand_Unit());
                p->unk2C = o->angle;
                p->unk2E = Rand_Next(1);
                p->unk34 = (Rand_Next(1) & 0x7FF) - 0x400;
                p->unk38 = -50.0f * o->scale;
                p->unk37 = 0;
            }
            break;
        case 13:
            if (i <= 0) {
                p->idx = 0;
                p->unk34 = r & 1;
            } else {
                p->idx = ((r >> i) & 1) + 1;
                p->scale *= 1.0f - 0.16f * (i - 1);
                p->unk38 = p->scale * ((70.0f + 20.0f * Rand_Unit()) / 8.0f);
                p->wait = (i - 1) * -5;
            }
            Vec_Copy(p->pos, t);
            p->unk2E = Rand_Next(1);
            break;
        case 14:
            switch (i) {
            case 0:
                sc = 0.8f + 0.4f * Rand_Unit();
                p->scale *= sc;
                p->unk2E = 0x4000;
                break;
            case 1:
                p->scale *= sc;
                p->sub = r & 3;
                break;
            case 2:
                p->idx = ((r >> 2) & 1) + 2;
                p->unk2C = o->angle;
                p->unk2E = Rand_Next(1);
                p->unk34 = 0.5f + 65536.0f * ((5.0f + 15.0f * Rand_Unit()) / 15.0f) / 360.0f;
                if (r & 0x8000) {
                    p->unk34 = -p->unk34;
                }
                p->unk38 = (100.0f + 100.0f * Rand_Unit()) / 15.0f;
                break;
            }
            Vec_Copy(p->pos, o->pos);
            break;
        case 15:
        case 17:
            StageObj_K1C_NextStepC(o, p);
            break;
        case 16:
            Vec_Copy(p->pos, o->pos);
            switch (i) {
            case 0:
                p->idx = 0;
                p->unk37 = 0;
                p->unk2C = Rand_Next(1);
                p->unk32 = 0.5f + 65536.0f * (48.0f / ((s16 *) StageObj_K1C_T0Frames)[p->idx]) / 360.0f;
                break;
            default:
                p->idx = 1;
                p->sub = i & 3;
                Vec_Copy(p->pos, o->pos);
                p->unk34 = 0.5f + 65536.0f * (30.0f / ((s16 *) StageObj_K1C_T0Frames)[p->idx]) / 360.0f;
                p->unk2C = o->angle + (p->sub << 14);
                p->scale *= 1.5f;
                p->unk38 = 120.0f * p->scale;
                break;
            }
            break;
        case 18:
            Vec_Copy(p->pos, o->pos);
            if (i <= 0) {
                p->idx = 0;
                p->unk2A = o->angle2;
                p->unk2C = o->angle;
                p->unk37 = 0;
                p->unk2E = Rand_Next(1);
            } else {
                p->idx = 1;
            }
            break;
        case 19:
            StageObj_K1C_NextStepD(o, p);
            p->pos[1] = y;
            break;
        case 20:
            Vec_Copy(p->pos, o->pos);
            p->unk2E = Rand_Next(1);
            p->unk34 = 0x5B;
            r = Rand_Next(1);
            if (r & 1) {
                p->unk34 = -p->unk34;
            }
            p->sub = (r >> 1) & 1;
            p->scale *= 0.5f;
            break;
        case 21:
            p->idx = 0;
            Vec_Copy(p->pos, o->pos);
            p->wait = -4;
            v[0] += (Rand_Next(1) & 0x3F) - 0x20;
            v[1] += (Rand_Next(1) & 0x1F) - 0x10;
            v[0] += (Rand_Next(1) & 0x3F) - 0x20;
            p->pos[0] += v[0];
            p->pos[1] += v[1];
            p->pos[2] += v[2];
            p->unk2E = Rand_Next(1);
            p->unk34 = 0x5B;
            r = Rand_Next(1);
            if (r & 1) {
                p->unk34 = -p->unk34;
            }
            p->sub = (r >> 1) & 1;
            break;
        case 22:
            Vec_Copy(p->pos, o->pos);
            p->pos[0] += 100.0f * Rand_Signed();
            p->pos[1] += 100.0f * Rand_Signed();
            p->pos[2] += 100.0f * Rand_Signed();
            p->unk2E = Rand_Next(1);
            p->unk34 = (Rand_Next(1) & 0x1FF) - 0x100;
            p->scale = 2.5f + 0.5f * Rand_Unit();
            p->unk38 = 2.0f;
            break;
        case 23:
            Vec_Copy(p->pos, o->pos);
            p->idx = 0;
            p->unk2C = o->angle;
            p->unk37 = 0;
            p->sub = o->var2;
            break;
        case 24:
            Vec_Copy(p->pos, o->pos);
            p->idx = ((u8 *) StageObj_K1C_T24Index)[i];
            p->unk37 = 0;
            p->unk2C = o->angle;
            switch (p->idx) {
            case 1:
                p->unk2C += r >> 1;
                if (i & 1) {
                    p->unk2C += 0x7FFF + 1;
                }
                p->unk32 = -0x2D7;
                break;
            }
            break;
        case 25:
            Vec_Copy(p->pos, o->pos);
            p->unk2A = 0xF05C;
            p->unk2C = o->angle;
            p->scale = 0.2f;
            switch (p->idx) {
            case 0:
                p->unk37 = 0;
                p->unk2E = Rand_Next(1);
                p->unk34 = 0x12;
                break;
            case 1:
                break;
            }
            break;
        case 26:
            Vec_Copy(p->pos, o->pos);
            p->wait = -((s16 *) StageObj_K1C_T26Delay)[i];
            switch (p->idx) {
            case 0:
                break;
            case 1:
                p->unk37 = 0;
                p->unk2A = 0xE800;
                p->unk2C = o->angle;
                p->unk2E = Rand_Next(1);
                break;
            case 2:
                p->unk37 = 0;
                p->unk2A = 0xE800;
                p->unk2C = o->angle;
                p->unk2E = Rand_Next(1);
                p->unk34 = 0xAAB;
                break;
            }
            break;
        case 27:
            Vec_Copy(p->pos, o->pos);
            p->unk2E = Rand_Next(1);
            p->unk34 = (Rand_Next(1) & 0xFF) - 0x80;
            p->scale *= 5.0f + 1.5f * Rand_Signed();
            break;
        case 28:
            p->idx = Rand_Next(1) % 3;
            Vec_Copy(p->pos, o->pos);
            p->unk2E = Rand_Next(1);
            p->unk34 = 0x5B0;
            p->scale *= 0.8f + 0.2f * Rand_Unit();
            break;
        case 29:
            if (i < 2) {
                p->idx = StageObj_K1C_StepBKinds[o->var2];
                p->wait = o->var1 - StageObj_K1C_StepBStart[o->var2];
                o->var2++;
            } else {
                p->idx = 0xFF;
            }
            break;
        case 30:
            p->unk37 = 0;
            p->wait = ((s16 *) StageObj_K1C_T30Delay)[i];
            Vec_Copy(p->pos, o->pos);
            switch (p->idx) {
            case 0:
            case 1:
                p->unk38 = 40.0f + 20.0f * Rand_Signed();
                p->unk32 = 0.5f + 65536.0f * (20.0f * Rand_Signed()) / 360.0f;
                p->unk2E = (s16) (0.5f + 65536.0f * (360.0f * Rand_Unit()) / 360.0f);
                p->unk3C = 30.0f;
                break;
            case 2:
            case 3:
                p->unk38 = 15.0f + 10.0f * Rand_Signed();
                p->unk2E = (s16) (0.5f + 65536.0f * (360.0f * Rand_Unit()) / 360.0f);
                p->unk3C = 20.0f;
                break;
            case 4:
            case 5:
            case 6:
                p->unk2E = (s16) (0.5f + 65536.0f * (360.0f * Rand_Unit()) / 360.0f);
                p->unk34 = 0.5f + 65536.0f * ((20.0f * Rand_Signed()) / ((s16 *) StageObj_K1C_T6Frames)[p->idx]) / 360.0f;
                p->sub = (Rand_Next(1) % 2) * 14;
                p->pos[0] += 100.0f * Rand_Signed();
                p->pos[1] += 100.0f * Rand_Signed();
                p->pos[2] += 100.0f * Rand_Signed();
                break;
            }
            switch (p->idx) {
            case 0:
            case 1:
                p->scale = 0.5f + 0.5f * Rand_Unit();
                break;
            case 2:
            case 3:
            case 4:
                p->scale = 1.0f;
                break;
            }
            break;
        case 31:
            p->unk2E = Rand_Next(1);
            p->unk38 = -10.0f * Rand_Unit();
            p->pos[0] = o->pos[0] + 50.0f * Rand_Signed();
            p->pos[1] = o->pos[1] + 50.0f * Rand_Signed();
            p->pos[2] = o->pos[2] + 50.0f * Rand_Signed();
            break;
        case 32:
            Vec_Copy(p->pos, o->pos);
            p->idx = ((u8 *) StageObj_K1C_T32Index)[i];
            p->wait = -((s16 *) D_008D7318)[i];
            switch (p->idx) {
            case 0:
            case 1:
                p->unk37 = 0;
                p->unk2C = Rand_Next(1);
                break;
            case 2:
                p->unk2E = Rand_Next(1);
                p->sub = dist[cnt++];
                p->unk38 = p->scale * ((300.0f + 100.0f * Rand_Unit()) / ((u16 *) StageObj_K1C_T2Frames)[p->idx]);
                break;
            case 3:
                p->unk37 = 0;
                break;
            }
            break;
        case 33:
            Vec_Copy(p->pos, o->pos);
            p->idx = ((u8 *) StageObj_K1C_T33Index)[i];
            p->wait = -((s16 *) StageObj_K1C_T33Delay)[i];
            switch (p->idx) {
            case 0:
                p->unk2E = Rand_Next(1);
                p->unk34 = 0x1555 / ((u16 *) StageObj_K1C_T33Frames)[p->idx];
                if (r & 1) {
                    p->unk34 = -p->unk34;
                }
                p->sub = dist[cnt++];
                p->unk38 = p->scale * ((400.0f + 100.0f * Rand_Unit()) / ((u16 *) StageObj_K1C_T33Frames)[p->idx]);
                break;
            case 1:
                p->unk2A = 0x4000;
                p->unk2E = Rand_Next(1);
                p->unk34 = 0x1555 / ((u16 *) StageObj_K1C_T33Frames)[p->idx];
                if (r & 1) {
                    p->unk34 = -p->unk34;
                }
                p->pos[1] += 30.0f * p->scale;
                break;
            case 2:
                p->pos[0] += 400.0f * Rand_Signed();
                p->pos[2] += 400.0f * Rand_Signed();
                p->unk2E = Rand_Next(1);
                break;
            }
            break;
        case 34:
            p->idx = 0;
            if (o->var2 == 0) {
                if (i > 0) {
                    p->wait = 0x28;
                } else {
                    p->wait = i * -5;
                }
            } else {
                p->wait = i * -5;
            }
            p->unk37 = 0;
            p->unk38 = 4.0f;
            break;
        case 35:
            Vec_Copy(p->pos, o->pos);
            p->idx = Rand_Next(1) & 3;
            p->unk37 = 0;
            break;
        case 36:
            Vec_Copy(p->pos, o->pos);
            p->unk37 = 0;
            p->unk2A = o->angle2;
            p->unk2C = o->angle;
            p->unk2E = Rand_Next(1);
            break;
        case 37:
            Vec_Copy(p->pos, o->pos);
            switch (p->idx) {
            case 0:
                break;
            case 1:
            case 2:
                p->unk2A = o->angle2;
                p->unk2C = o->angle;
                break;
            }
            break;
        case 38:
            p->wait = -((s16 *) StageObj_K1C_T38Delay)[i];
            switch (p->idx) {
            case 0:
                break;
            case 1:
            case 2:
                p->unk2A = o->angle2;
                p->unk2C = o->angle;
                p->unk2E = Rand_Next(1);
                p->unk34 = 0xB6;
                p->unk37 = 0;
                break;
            }
            break;
        case 39:
            Vec_Copy(p->pos, o->pos);
            p->pos[0] += o->scale * (50.0f * Rand_Signed());
            p->pos[2] += o->scale * (50.0f * Rand_Signed());
            p->idx = r & 1;
            p->unk2E = r;
            p->unk34 = 0x5B;
            if ((r >> 1) & 1) {
                p->unk34 = -p->unk34;
            }
            break;
        case 40:
            p->wait = i * -3;
            p->idx = (r >> i) & 1;
            p->unk2A = Rand_Next(1);
            p->unk2C = Rand_Next(1);
            p->unk2E = Rand_Next(1);
            p->unk37 = 0;
            break;
        }
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K1C_Draw;
        } else {
            p->node = NULL;
        }
    }
}

void StageObj_K1C_Run(StageObj *o)
{
    s16 limit = gStageObjK1C_Limits[o->type];

    o->timer++;
    if (limit != 1000 && limit < o->timer) {
        o->state++;
        o->visible = 0;
        return;
    }
    switch (o->type) {
    case 0:
    case 0xB:
        StageObj_K1C_RunChain(o);
        break;
    default:
        StageObj_K1C_RunSeq(o);
        break;
    }
}

/* The owner's bone 0x25, through an inlined helper: its return value is a
 * temporary with a 16-byte stack slot, where m is spilled (sp+0xA0; a plain
 * `m = Em_Bone(em, 0x25)` gets a 4-byte slot at sp+0xAC). */
static inline f32 *StageObj_K1C_Bone(Em *em)
{
    return (f32 *)Em_Bone(em, 0x25);
}

void StageObj_K1C_RunChain(StageObj *o)
{
    f32 pos[3];
    Obj1CPartA *first;
    u8 moved = 0;
    Obj1CPartA *p = (Obj1CPartA *) o->buf;
    Em *em = o->owner;
    s16 i;
    s16 n = gStageObjK1C_Counts[o->type];
    s16 c1 = gStageObjK1C_ScaleCurveIdx[o->type];
    s16 c2 = gStageObjK1C_ColorCurveIdx[o->type];
    Obj1CPartA *prev;
    DrawSlot *node;
    f32 *m;

    switch (o->type) {
    case 0:
        first = p;
        switch (o->var0) {
        case 0:
            if (StageObj_K1C_OwnerActive(em) == 0 || (em->motionId[0] != 0x417 && em->motionId[0] != 0x41A) || o->timer >= 300) {
                *(s16 *) ((u8 *) p + 0x320) = 0;
                Vec_Copy(p->pos, p->base[0]);
                o->var0++;
            } else {
                m = StageObj_K1C_Bone(em);
            }
            break;
        case 1:
            if (++*(s16 *) ((u8 *) p + 0x320) >= 0x26) {
                o->state++;
                o->visible = 0;
                return;
            }
            break;
        }
        break;
    }
    for (i = 0; i < n; i++, p++) {
        p->unk10++;
        switch (o->type) {
        case 0:
            switch (o->var0) {
            case 0:
                StageObj_K1C_FollowBone(o, p, prev, m, first->dir[0], 1);
                prev = p;
                p->unk18 += p->unk1A;
                Vec_Copy((f32 *) &pos, p->pos);
                moved = 1;
                break;
            case 1:
            default:
                if (++p->unk12 == 0) {
                    StageObj_K1C_RandomSpin(p);
                }
                if (p->unk12 >= 30) {
                    continue;
                }
                if (p->unk12 >= 0) {
                    Key_LerpVec2(gStageObjK1C_ScaleCurves[c1], p->sc, p->unk12);
                    Color_EvalKeys((s32 *)gStageObjK1C_ColorKeys[c2], (f32) p->unk12, (u32 *)&p->color);
                    p->pos[1] += 2.0f;
                }
                break;
            case 2:
                if (++p->unk12 == 0) {
                    if (o->var2 != 0xFF) {
                        if (StageObj_K1C_OwnerActive(em) == 1) {
                            Em_BonePos(em, 0x25, p->pos);
                        } else {
                            o->var2 = 0xFF;
                            p->unk12 = 30;
                        }
                    } else {
                        p->unk12 = 30;
                    }
                }
                if (p->unk12 < 30 && p->unk12 > 0) {
                    Key_LerpVec2(gStageObjK1C_ScaleCurves[c1], p->sc, p->unk12);
                    Color_EvalKeys((s32 *)gStageObjK1C_ColorKeys[c2], (f32) p->unk12, (u32 *)&p->color);
                    p->pos[1] += 2.0f;
                } else {
                    continue;
                }
                break;
            }
            break;
        }
        if ((node = p->node) != NULL) {
            Vec_Copy(node->pos, p->pos);
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
        }
    }
    if (moved & 0xFF) {
        switch (o->type) {
        case 0:
            if (o->timer % 10 == 0) {
                Em_PlaySeChecked((struct Em *)em, 0x42, 0, (f32 *) &pos, 6, 0);
            }
            break;
        }
    }
}

extern u32 gStageObjK1C_SeqTbls[];
s32 EmKushala_GroundEffectStateInWind(Em *em);
s32 EmKushala_WindActive(Em *em);
s32 EmKushala_WindActive2(Em *em);
void EffEm07_CreateAt(Em *em, f32 *pos, s8 kind);
s32 StageObj_K1C_AllDone(StageObj *o);
void StageObj_K1C_RestartStepsA(StageObj *o);
void StageObj_K1C_RestartStepsB(StageObj *o);
s32 StageObj_K1C_StepsDone(StageObj *o);
static void StageObj_K1C_NextStepA(StageObj *o, Obj1CPart *w);
static void StageObj_K1C_NextStepB(StageObj *o, Obj1CPart *w);
void StageObj_K1C_InitStepA(StageObj *o, Obj1CPart *w);
void StageObj_K1C_InitStepB(StageObj *o, Obj1CPart *w);
void StageObj_K1C_InitStepC(StageObj *o, Obj1CPart *w);
void StageObj_K1C_InitStepD(StageObj *o, Obj1CPart *w);
void StageObj_K1C_CreateTwoPoint(f32 *pos, f32 *pos2, u8 k, f32 scale);

void StageObj_K1C_RunSeq(StageObj *o)
{
    f32 f;
    f32 t[3];
    f32 v[3];
    f32 v2[3];
    f32 bp[3];
    s16 i;
    s16 limit;
    u16 ang;
    u8 flag;
    s32 nn;
    Obj1CPart *p = (Obj1CPart *) o->buf;
    Em *em = o->owner;
    s16 n = gStageObjK1C_Counts[o->type];
    s16 c1 = gStageObjK1C_ScaleCurveIdx[o->type];
    s16 c2 = gStageObjK1C_ColorCurveIdx[o->type];
    s16 *tbl = (s16 *) gStageObjK1C_SeqTbls[o->type];
    f32 *m;
    f32 k;
    f32 *key;
    s32 w;
    u8 *ckey;

    switch (o->type) {
    case 3:
        flag = 0;
        switch (o->var0) {
        case 0:
            if (o->var2 != 0xFF) {
                if (StageObj_K1C_OwnerActive(em) == 0) {
                    o->var2 = 0xFF;
                } else {
                    m = (f32 *)Em_Bone(em, 0x25);
                    Mtx_GetTrans(o->pos, m);
                }
            }
            limit = 10;
            if (o->timer >= 10) {
                flag = 1;
                o->var0++;
                limit = 200;
            }
            break;
        case 1:
            if (o->var2 != 0xFF) {
                if (StageObj_K1C_OwnerActive(em) == 0) {
                    o->var2 = 0xFF;
                } else {
                    m = (f32 *)Em_Bone(em, 0x25);
                    Mtx_GetTrans(o->pos, m);
                }
            }
            limit = 200;
            if (o->timer >= 210) {
                flag = 1;
                o->var0++;
                o->var2 = 0xFF;
                limit = 30;
            }
            break;
        case 2:
        default:
            c1++;
            c2++;
            limit = 30;
            break;
        }
        if (!(o->timer & 3)) {
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 1.0f;
            Vec_RotateX(v, 6.2831855f * p->unk2A / 65536.0f);
            Vec_RotateY(v, 6.2831855f * p->unk2C / 65536.0f);
            t[0] = p->pos[0] + 1000.0f * v[0] * o->scale;
            t[1] = p->pos[1] + 1000.0f * v[1] * o->scale;
            t[2] = p->pos[2] + 1000.0f * v[2] * o->scale;
            StageObj_K1C_CreateTwoPoint(t, v, 4, o->scale);
        }
        break;
    case 4:
        limit = 0x1A;
        break;
    case 7:
    case 0xD:
        if (StageObj_K1C_OwnerActive(em) == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        break;
    case 8:
        if (o->timer == 5) {
            StageObj_K1C_CreateAtYaw(o->pos, o->angle, 9, o->scale);
        }
        break;
    case 0xC:
        if (StageObj_K1C_OwnerActive(em) == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        if (!(u8)(s32)Player_InCurrentArea(em)) {
            return;
        }
        Em_BonePos(em, 2, bp);
        ang = em->rotY;
        switch (o->var0) {
        case 0:
            if (++o->var1 == 7) {
                Vec_Copy(o->pos, bp);
                o->pos[1] = em->floorY;
                o->angle2 = em->rotY;
                if (o->pos[1] < em->posY - 1000.0f || EmKushala_GroundEffectStateInWind(em) == 0) {
                    o->ownerIndex = 1;
                } else {
                    EffEm07_CreateAt(em, o->pos, 3);
                    Em_PlaySeChecked((struct Em *)em, 0x40, 0, o->pos, 6, 0);
                }
            }
            if (StageObj_K1C_AllDone(o) == 1) {
                o->var0++;
                o->var1 = 0;
                o->ownerIndex = 0;
                o->angle = em->rage;
                return;
            }
            break;
        case 1:
            if (EmKushala_WindActive(em) == 0) {
                return;
            }
            switch (em->motionId[0]) {
            case 0x408:
            case 0x416:
            case 0x417:
            case 0x41A:
            case 0x429:
            case 0x42C:
            case 0x42D:
                return;
            }
            if (++o->var1 >= 0) {
                o->scale = em->scaleX;
                o->var0 = 0;
                o->var1 = 0;
                StageObj_K1C_RestartStepsA(o);
                if (Player_InCurrentArea(em)) {
                    Em_PlaySeChecked((struct Em *)em, 0x3F, 0, bp, 6, 0);
                }
            }
            return;
        }
        break;
    case 0xF:
    case 0x11:
    case 0x13:
        if (StageObj_K1C_AllDone(o) == 1) {
            o->state++;
            o->visible = 0;
            return;
        }
        break;
    case 0x18:
        if (StageObj_K1C_OwnerActive(em) == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        Em_BonePos(em, 4, o->pos);
        o->pos[1] -= 100.0f;
        break;
    case 0x19:
        if (StageObj_K1C_OwnerActive(em) == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        if (em->motionId[0] != 0x3F5 && em->motionId[0] != 0x413) {
            o->state++;
            o->visible = 0;
            return;
        }
        StageObj_K1C_PlaceBehindOwner(o);
        break;
    case 0x1D:
        if (StageObj_K1C_OwnerActive(em) == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        if (!(u8)(s32)Player_InCurrentArea(em)) {
            return;
        }
        Em_BonePos(em, 2, bp);
        ang = em->rotY;
        switch (o->var0) {
        case 0:
        again:
            if (EmKushala_WindActive2(em) == 0) {
                return;
            }
            switch (em->motionId[0]) {
            case 0x408:
            case 0x416:
            case 0x417:
            case 0x41A:
            case 0x429:
            case 0x42C:
            case 0x42D:
                return;
            }
            if (EmKushala_GroundEffectStateInWind(em) == 0) {
                break;
            }
            o->var0++;
            StageObj_K1C_RestartStepsB(o);
            Vec_Copy(o->pos, bp);
            o->angle2 = em->rotY;
        case 1:
            if (StageObj_K1C_StepsDone(o) == 1) {
                o->scale = em->scaleX;
                o->var0 = 0;
                o->var1 = 0;
                o->angle = 0;
                goto again;
            }
            break;
        }
        break;
    case 0x23:
        limit = 4;
        break;
    case 0x26:
        m = (f32 *)Em_Bone(em, 0x33);
        v[0] = 0.0f;
        v[1] = -91.6f;
        v[2] = 132.8f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(t, m);
        o->pos[0] = t[0] + v[0];
        o->pos[1] = t[1] + v[1];
        o->pos[2] = t[2] + v[2];
        break;
    case 0x28:
        if (StageObj_K1C_OwnerActive(em) == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        StageObj_K1C_GetOwnerBone(o, o->pos);
        break;
    }
    nn = n;
    for (i = 0; i < nn; i++, p++) {
        switch (o->type) {
        case 3:
            if (flag) {
                p->wait = 0;
            }
            if (o->var2 != 0xFF) {
                Vec_DirToAngles(m + 8, (s16 *) &p->unk2A, (s16 *) &p->unk2C);
            }
            Vec_Copy(p->pos, o->pos);
            p->wait++;
            switch (o->var0) {
            case 1:
                goto move;
            case 0:
            case 2:
                if (p->wait <= 0) {
                    continue;
                }
                if (p->wait > limit) {
                    goto move;
                }
                c1 = gStageObjK1C_ScaleCurveIdx[o->type];
                c2 = gStageObjK1C_ColorCurveIdx[o->type];
                if (o->var0 == 2) {
                    c1++;
                    c2++;
                }
                goto curve;
            }
            break;
        case 12:
        case 29:
            if (p->idx == 0xFF) {
                continue;
            }
            c1 = p->idx + gStageObjK1C_ScaleCurveIdx[o->type];
            c2 = p->idx + gStageObjK1C_ColorCurveIdx[o->type];
            break;
        case 15:
        case 17:
        case 19:
            if (p->idx == 0xFF) {
                continue;
            }
            c1 = p->idx + gStageObjK1C_ScaleCurveIdx[o->type];
            c2 = p->idx + gStageObjK1C_ColorCurveIdx[o->type];
            break;
        case 10: case 14: case 16: case 18: case 20: case 21: case 22: case 24: case 25: case 26:
        case 30: case 31: case 32: case 33: case 34: case 36: case 37: case 38: case 39: case 40:
            c1 = p->idx + gStageObjK1C_ScaleCurveIdx[o->type];
            c2 = p->idx + gStageObjK1C_ColorCurveIdx[o->type];
            break;
        case 23:
            c1 = p->idx + gStageObjK1C_ScaleCurveIdx[o->type];
            c2 = p->sub + gStageObjK1C_ColorCurveIdx[o->type];
            break;
        case 7:
            c1 = p->idx + gStageObjK1C_ScaleCurveIdx[o->type] + o->var1 * 3;
            c2 = p->idx + gStageObjK1C_ColorCurveIdx[o->type] + o->var1 * 3;
            break;
        case 13:
            c1 = p->idx + gStageObjK1C_ScaleCurveIdx[o->type] + o->var1 * 3;
            c2 = p->idx + gStageObjK1C_ColorCurveIdx[o->type] + o->var1 * 3;
            break;
        default:
            if (tbl == NULL) {
                c1 = gStageObjK1C_ScaleCurveIdx[o->type];
                c2 = gStageObjK1C_ColorCurveIdx[o->type];
            }
            break;
        }
        if (tbl != NULL) {
            limit = tbl[p->idx];
        }
        if (++p->wait > limit) {
            switch (o->type) {
            case 9:
                switch (p->idx) {
                case 2:
                    if (o->timer < 0x24) {
                        p->wait = -1;
                        Vec_Copy(p->pos, o->pos);
                        StageObj_K1C_Jitter(p, 300.0f);
                    }
                    break;
                }
                break;
            case 0xC:
                StageObj_K1C_NextStepA(o, p);
                if (p->idx == 0xFF) {
                    continue;
                }
                if (tbl != NULL) {
                    limit = tbl[p->idx];
                }
                break;
            case 0xF:
            case 0x11:
                StageObj_K1C_NextStepC(o, p);
                if (p->idx == 0xFF) {
                    continue;
                }
                if (tbl != NULL) {
                    limit = tbl[p->idx];
                }
                break;
            case 0x13:
                StageObj_K1C_NextStepD(o, p);
                if (p->idx == 0xFF) {
                    continue;
                }
                if (tbl != NULL) {
                    limit = tbl[p->idx];
                }
                break;
            case 0x19:
                p->wait = 1;
                break;
            case 0x1D:
                StageObj_K1C_NextStepB(o, p);
                if (p->idx == 0xFF) {
                    continue;
                }
                if (tbl != NULL) {
                    limit = tbl[p->idx];
                }
                break;
            case 0x21:
                switch (p->idx) {
                case 2:
                    if (o->timer < 0x34) {
                        p->wait = 1;
                        p->unk2E = Rand_Next(1);
                        p->pos[0] = o->pos[0] + 400.0f * Rand_Signed();
                        p->pos[2] = o->pos[2] + 400.0f * Rand_Signed();
                    }
                    break;
                }
                break;
            }
        }
        if (p->wait <= 0 || limit < p->wait) {
            if (p->unk36) {
                c2++;
                c1++;
            } else {
                c1 += 2;
            }
            continue;
        }
        if (p->wait == 1) {
            switch (o->type) {
            case 7:
                m = (f32 *)Em_Bone(em, 0x12);
                v[0] = 0.0f;
                v[1] = -70.0f;
                v[2] = 42.0f;
                Vec_RotateSelf(v, m);
                Mtx_GetTrans(t, m);
                p->pos[0] = t[0] + v[0];
                p->pos[1] = t[1] + v[1];
                p->pos[2] = t[2] + v[2];
                Vec_Copy(v2, m + 4);
                v2[0] = -v2[0];
                v2[1] = -v2[1];
                v2[2] = -v2[2];
                Vec_DirToAngles(v2, (s16 *) &p->unk2A, (s16 *) &p->unk2C);
                p->unk2A = p->unk2A + 0x7FFF + 0x7001;
                break;
            case 0xC:
                StageObj_K1C_InitStepA(o, p);
                break;
            case 0xD:
                switch (em->type) {
                case 0x3B:
                    m = (f32 *)Em_Bone(em, 0x29);
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = 30.0f;
                    break;
                case 0x44:
                    m = (f32 *)Em_Bone(em, 0xB);
                    v[0] = 0.0f;
                    v[1] = -10.0f;
                    v[2] = 20.0f;
                    break;
                default:
                    m = (f32 *)Em_Bone(em, 0x27);
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = 100.0f;
                    break;
                }
                Vec_RotateSelf(v, m);
                Mtx_GetTrans(t, m);
                p->pos[0] = t[0] + v[0];
                p->pos[1] = t[1] + v[1];
                p->pos[2] = t[2] + v[2];
                Vec_Copy(v2, m + 8);
                Vec_DirToAngles(v2, (s16 *) &p->unk2A, (s16 *) &p->unk2C);
                p->unk2A = p->unk2A + 0x7FFF + 0x7001;
                break;
            case 0xF:
            case 0x11:
                StageObj_K1C_InitStepC(o, p);
                break;
            case 0x13:
                StageObj_K1C_InitStepD(o, p);
                break;
            case 0x1D:
                StageObj_K1C_InitStepB(o, p);
                break;
            case 0x22:
                m = (f32 *)Em_Bone(em, 0xE);
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 0.0f;
                Vec_RotateSelf(v, m);
                Mtx_GetTrans(t, m);
                p->pos[0] = t[0] + v[0];
                p->pos[1] = t[1] + v[1];
                p->pos[2] = t[2] + v[2];
                break;
            }
        }
    curve:
        switch (o->type) {
        case 0x19:
            k = (o->timer & 3) + 1;
            break;
        default:
            k = p->wait;
            break;
        }
        if (p->unk37) {
            key = gStageObjK1C_ScaleCurves[c1++];
            Key_LerpVec2(key, p->sc, k);
        } else {
            key = gStageObjK1C_ScaleCurves[c1++];
            Key_LerpVec3(key, p->sc, k);
        }
        w = p->wait;
        if (p->unk36 == 0) {
            key = gStageObjK1C_ScaleCurves[c1++];
            Key_LerpFloat(key, &f, w);
            p->color = ((u8) (255.0f * f) << 24) | 0xFFFFFF;
        } else {
            ckey = gStageObjK1C_ColorKeys[c2++];
            Color_EvalKeys((s32 *)ckey, w, (u32 *)&p->color);
        }
    move:
        switch (o->type) {
        case 1:
            switch (p->idx) {
            case 4:
                    p->pos[1] += 6.0f;
                break;
            }
            break;
        case 4:
            p->pos[0] += 15.0f * o->pos[0];
            p->pos[1] += 15.0f * o->pos[1];
            p->pos[2] += 15.0f * o->pos[2];
            break;
        case 5:
        case 6:
            switch (p->idx) {
            case 0:
                break;
            case 1:
            default:
                p->unk38 += -2.5f;
                p->pos[1] += p->unk38;
                break;
            }
            break;
        case 7:
        case 13:
            switch (p->idx) {
            case 0:
                break;
            default:
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = p->unk38;
                    Vec_RotateX(v, 6.2831855f * p->unk2A / 65536.0f);
                    Vec_RotateY(v, 6.2831855f * p->unk2C / 65536.0f);
                    Vec_Add(p->pos, p->pos, v);
                break;
            }
            break;
        case 8:
            switch (p->idx) {
            case 2:
                    p->unk38 += -1.0f;
                    p->pos[1] += p->unk38;
                break;
            }
            break;
        case 9:
            switch (p->idx) {
            case 1:
                    p->unk38 += -1.0f;
                    p->pos[1] += p->unk38;
                break;
            }
            break;
        case 10:
        case 18:
            switch (p->idx) {
            case 2:
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = p->unk38;
                    Vec_RotateX(v, 6.2831855f * o->angle2 / 65536.0f);
                    Vec_RotateY(v, 6.2831855f * p->unk2C / 65536.0f);
                    p->pos[0] += v[0];
                    p->pos[1] += v[1];
                    p->pos[2] += v[2];
                break;
            }
            break;
        case 12:
            switch (p->idx) {
            case 0:
                Vec_Copy(p->pos, bp);
                p->unk2C = ang;
                break;
            case 1:
                if (o->ownerIndex != 0) {
                    continue;
                }
                break;
            case 2:
            default:
                if (o->ownerIndex != 0) {
                    continue;
                }
                p->unk38 *= 0.9f;
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = p->unk38;
                Vec_RotateY(v, 6.2831855f * p->unk2C / 65536.0f);
                p->pos[0] += v[0];
                p->pos[1] += 10.0f * o->scale;
                p->pos[2] += v[2];
                break;
            }
            break;
        case 14:
            switch (p->idx) {
            case 2:
            case 3:
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = p->unk38;
                Vec_RotateY(v, 6.2831855f * p->unk2C / 65536.0f);
                p->pos[0] += v[0];
                p->pos[2] += v[2];
                break;
            }
            break;
        case 15:
        case 17:
            switch (p->idx) {
            case 3:
                p->unk38 += -2.0f;
                p->pos[1] += p->unk38;
                break;
            case 4:
            case 5:
                p->unk3C += -8.0f;
                v[0] = 0.0f;
                v[1] = p->unk3C;
                v[2] = p->unk38;
                Vec_RotateY(v, 6.2831855f * (o->angle + (p->sub << 14)) / 65536.0f);
                p->pos[0] += v[0];
                p->pos[1] += v[1];
                p->pos[2] += v[2];
                break;
            case 6:
            case 7:
                p->unk38 *= 0.95f;
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = p->unk38;
                Vec_RotateY(v, 6.2831855f * p->unk2C / 65536.0f);
                p->pos[0] += v[0];
                p->pos[1] += 8.0f * o->scale;
                p->pos[2] += v[2];
                break;
            }
            break;
        case 16:
            switch (p->idx) {
            case 0:
                break;
            default:
                    p->unk38 *= 0.9f;
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = p->unk38;
                    Vec_RotateY(v, 6.2831855f * p->unk2C / 65536.0f);
                    p->pos[0] += v[0];
                    p->pos[1] += 10.0f * o->scale;
                    p->pos[2] += v[2];
                break;
            }
            break;
        case 19:
            switch (p->idx) {
            case 0:
                break;
            default:
                    p->unk38 *= 0.95f;
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = p->unk38;
                    Vec_RotateY(v, 6.2831855f * p->unk2C / 65536.0f);
                    p->pos[0] += v[0];
                    p->pos[1] += 3.0f * p->scale;
                    p->pos[2] += v[2];
                break;
            }
            break;
        case 20:
        case 21:
            p->pos[1] += 5.0f * p->scale;
            break;
        case 22:
            p->unk38 += -2.0f;
            p->pos[1] += p->unk38;
            break;
        case 24:
            Vec_Copy(p->pos, o->pos);
            break;
        case 25:
            Vec_Copy(p->pos, o->pos);
            p->unk2C = o->angle;
            p->scale += 0.01f * (1.0f - p->scale);
            if (p->scale > 1.0f) {
                p->scale = 1.0f;
            }
            break;
        case 27:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -7.5f;
            Vec_RotateX(v, 6.2831855f * o->angle2 / 65536.0f);
            Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
            p->pos[0] += v[0];
            p->pos[1] += v[1];
            p->pos[2] += v[2];
            break;
        case 28:
            if (p->wait < 0xB) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 25.0f * p->scale;
                Vec_RotateX(v, 6.2831855f * o->angle2 / 65536.0f);
                Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
                p->pos[0] += v[0];
                p->pos[1] += v[1];
                p->pos[2] += v[2];
            } else {
                p->unk34 = 0;
            }
            break;
        case 29:
            switch (p->idx) {
            case 0:
                Vec_Copy(p->pos, bp);
                break;
            case 1:
                Vec_Copy(p->pos, bp);
                p->unk2C = ang;
                break;
            case 2:
            case 3:
            default:
                p->pos[1] += p->unk38;
                break;
            }
            break;
        case 30:
            switch (p->idx) {
            case 0:
            case 1:
            case 2:
            case 3:
                if (p->idx < 2) {
                    p->unk3C = p->unk3C - 3.0f;
                } else {
                    p->unk3C = p->unk3C - 1.5f;
                }
                p->unk38 *= 0.95f;
                v2[0] = 0.0f;
                v2[1] = p->unk3C;
                v2[2] = p->unk38;
                Vec_RotateY(v2, 6.2831855f * (o->angle + p->unk32) / 65536.0f);
                p->pos[0] += v2[0];
                p->pos[1] += v2[1];
                p->pos[2] += v2[2];
                break;
            case 4:
            case 5:
            case 6:
                p->pos[1] = 100.0f + o->pos[1];
                break;
            }
            break;
        case 31:
            p->unk38 = p->unk38 - 2.0f;
            p->pos[1] += p->unk38;
            break;
        case 32:
            switch (p->idx) {
            case 2:
                    v[0] = 0.0f;
                    v[1] = 3.0f - 2.5f * p->wait;
                    v[2] = p->unk38;
                    Vec_RotateY(v, 6.2831855f * (p->sub << 14) / 65536.0f);
                    p->pos[0] += v[0];
                    p->pos[1] += v[1];
                    p->pos[2] += v[2];
                break;
            }
            break;
        case 33:
            switch (p->idx) {
            case 0:
                    v[0] = 0.0f;
                    v[1] = 12.0f - p->wait;
                    v[2] = p->unk38;
                    Vec_RotateY(v, 6.2831855f * (p->sub << 14) / 65536.0f);
                    p->pos[0] += v[0];
                    p->pos[1] += v[1];
                    p->pos[2] += v[2];
                break;
            }
            break;
        case 34:
            if (o->var2 == 0) {
                p->unk38 = p->unk38 - 4.0f;
            } else {
                p->unk38 = p->unk38 - 6.0f;
            }
            p->pos[1] += p->unk38;
            if (p->pos[1] < o->pos[1]) {
                p->wait = limit;
                Em_PlaySeChecked((struct Em *)em, 0x32, 0, p->pos, 6, 0);
                continue;
            }
            break;
        case 39:
            p->pos[1] += 15.0f * o->scale;
            break;
        case 38:
        case 40:
            Vec_Copy(p->pos, o->pos);
        case 0:
        case 2:
        case 3:
        case 11:
        case 23:
        case 26:
        case 35:
        case 36:
        case 37:
            break;
        }
        p->unk2A += p->unk30;
        p->unk2C += p->unk32;
        p->unk2E += p->unk34;
        if (p->node != NULL) {
            Vec_Copy(p->node->pos, p->pos);
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
        }
    }
}

void StageObj_K1C_End(StageObj *o)
{
    s16 n;

    o->state++;
    n = gStageObjK1C_Counts[o->type];
    switch (o->type) {
    case 0:
    case 0xB:
        StageObj_K1C_EndChain(o, n);
        break;
    default:
        StageObj_K1C_EndSeq(o, n);
        break;
    }
}

void StageObj_K1C_EndChain(StageObj *o, s16 n)
{
    u8 *p = o->buf;
    s16 i;

    for (i = 0; i < n; i++) {
        if (*(s32 *) (p + 0x20) != 0) {
            DrawSlot_Free(*(s16 *) p);
        }
        p += 0x64;
    }
}

void StageObj_K1C_EndSeq(StageObj *o, s16 n)
{
    u8 *p = o->buf;
    s16 i;

    for (i = 0; i < n; i++) {
        if (*(s32 *) (p + 0x24) != 0) {
            DrawSlot_Free(*(s16 *) p);
        }
        p += 0x40;
    }
}

void StageObj_K1C_Free(StageObj *o)
{
    StageObj_Free(o);
}

s32 StageObj_K1C_FindModelSet(u8 k)
{
    u8 *p = gStageObjK1C_ModelSets[k];
    s16 r;

    for (; *p != 0xFF; p++) {
        r = EmSlot_FindKind(*p);
        if (r != -1) {
            return r;
        }
    }
    return -1;
}

void StageObj_K1C_Draw(DrawSlot *node)
{
    switch (((StageObj *) node->owner)->type) {
    case 0:
    case 0xB:
        StageObj_K1C_DrawChain(node);
        break;
    default:
        StageObj_K1C_DrawSeq(node);
        break;
    }
}

void StageObj_K1C_DrawChain(DrawSlot *node)
{
    f32 scale[3];
    f32 rot[3];
    f32 v[3];
    f32 m[16];
    f32 tm[16];
    StageObj *o = node->owner;
    Obj1CPartA *p = (Obj1CPartA *) o->buf + node->part;
    Model *set;
    ModelMat *mats;
    ModelMatSet *model;
    u32 color;
    u16 mode = 0;
    s16 k;
    s32 t;
    u8 alpha;
    f32 f;

    k = EmSlot_FindKind(((Em *) o->owner)->type);
    if (k != -1) {
        set = gScene.partModels[k];
        if (set != NULL && set->active != 0) {
            mats = set->mats;
            f = StageObj_K1C_GrowFactor(o);
            scale[0] = f * (p->sc[0] * p->scale);
            scale[1] = f * (p->sc[1] * p->scale);
            scale[2] = f * (p->sc[2] * p->scale);
            switch (o->type) {
            case 0:
                rot[2] = (6.2831855f * (u32) p->unk18) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
                alpha = (((u32) p->color >> 24) & 0xFF) * (1.0f - 0.5f * (p->kind / 8.0f));
                color = ((u32) p->color & 0xFF) | (((((u32) p->color >> 8) & 0xFF) << 8) | ((alpha << 24) | ((((u32) p->color >> 16) & 0xFF) << 16)));
                if (p->unk12 < 0) {
                    mode |= 2;
                    switch (p->kind) {
                    case 0:
                        v[0] = v[1] = v[2] = 0.0f;
                        rot[0] = (6.2831855f * Vec_Elevation(p->dir[0], v)) / 65536.0f;
                        rot[1] = (6.2831855f * ((Vec_Heading(p->dir[0], v) & 0xFFFF) + 0x4000)) / 65536.0f;
                        Mtx_RotateX(m, rot[0]);
                        Mtx_RotateY(m, rot[1]);
                        model = &set->matSets[2];
                        Mtx_Translation(tm, 0.0f, (p->unk10 & 0xF) / 16.0f, 0.0f);
                        break;
                    default:
                        Mtx_Concat3x3(m, gCamMatrix);
                        model = set->matSets;
                        t = p->unk10 % 15;
                        Mtx_Translation(tm, 0.25f * (t & 3), 0.25f * (t >> 2), 0.0f);
                        break;
                    }
                    Gs_SetState(0x19, (s32) tm);
                } else {
                    Mtx_Concat3x3(m, gCamMatrix);
                    model = &set->matSets[1];
                }
                break;
            case 11:
                rot[2] = (6.2831855f * (u32) p->unk18) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
                color = p->color;
                v[0] = v[1] = v[2] = 0.0f;
                rot[0] = (6.2831855f * Vec_Elevation(p->dir[0], v)) / 65536.0f;
                rot[1] = (6.2831855f * ((Vec_Heading(p->dir[0], v) & 0xFFFF) + 0x4000)) / 65536.0f;
                switch (p->kind) {
                case 0:
                    Mtx_RotateX(m, rot[0]);
                    Mtx_RotateY(m, rot[1]);
                    model = &set->matSets[4];
                    Mtx_Translation(tm, 0.0f, (p->unk10 & 0xF) / 16.0f, 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    mode |= 2;
                    break;
                default:
                    Mtx_RotateX(m, rot[0] + (6.2831855f * (u32) p->unk14) / 65536.0f);
                    Mtx_RotateY(m, rot[1] + (6.2831855f * (u32) p->unk16) / 65536.0f);
                    model = &set->matSets[9];
                    break;
                }
                break;
            }
            Gs_SetState(0x6C, 0);
            Effect_DrawModel(model, m, color, mode, mats);
            Gs_SetState(0x6C, 1);
        }
    }
}

void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
extern f32 gCamYawMtx[];
extern u32 gStageObjK1C_DrawTbls[], gStageObjK1C_ModelSetIds[], StageObj_K1C_T2Frames[];

void StageObj_K1C_DrawSeq(DrawSlot *node)
{
    u8 r;
    u8 g;
    u8 b;
    u8 a;
    f32 scale[3];
    f32 rot[3];
    f32 m[16];
    f32 tm[16];
    u16 order;
    u16 mode;
    u8 cm;
    StageObj *o;
    Obj1CPart *p;
    u32 color;
    Model *set;
    u16 *tbl;
    ModelMat *mats;
    ModelMatSet *model;
    f32 f;
    s16 k;
    u8 alpha;

    mode = 0;
    o = node->owner;
    p = (Obj1CPart *) o->buf + node->part;
    set = gStageModelSet;
    tbl = ((u16 **) gStageObjK1C_DrawTbls)[o->type];
    cm = 1;
    if (tbl[p->idx] & 0x8000) {
        k = StageObj_K1C_FindModelSet(((u8 (*)[2]) gStageObjK1C_ModelSetIds)[o->type][0]);
        if (k == -1) {
            return;
        }
        set = gScene.partModels[k];
    }
    if (set != NULL && set->active != 0) {
    model = &set->matSets[tbl[p->idx] & ~0x8000];
    mats = set->mats;
    color = p->color;
    switch (o->type) {
    case 1:
        switch (p->idx) {
        case 0:
        default:
            mode |= 2;
            order = 3;
            break;
        case 1:
            mode |= 2;
            order = 0;
            break;
        case 2:
            mode |= 2;
            order = 6;
            Mtx_Translation(tm, 0.0f, (p->wait & 0x7F) / 128.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            cm = 0;
            break;
        case 3:
            order = 3;
            mode |= 2;
            Mtx_Translation(tm, 0.25f * ((p->wait >> 1) & 3), 0.25f * (p->wait >> 3), 0.0f);
            Gs_SetState(0x19, (s32) tm);
            break;
        case 4:
            order = 3;
            break;
        }
        break;
    case 2:
        mode |= 2;
        switch (p->idx) {
        default:
            order = 6;
            cm = 0;
            break;
        case 1:
        case 2:
            order = 3;
            break;
        }
        break;
    case 3:
        mode |= 2;
        order = 7;
        cm = 0;
        Mtx_Translation(tm, 0.0f, (o->timer & 0x1F) / 32.0f, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        break;
    case 4:
        mode |= 2;
        order = 3;
        break;
    case 5:
    case 6:
        switch (p->idx) {
        case 0:
            order = 2;
            cm = 0;
            break;
        default:
        case 1:
            order = 3;
            break;
        }
        break;
    case 7:
    case 13:
        Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        order = 3;
        if (o->var1 == 0) {
            mode |= 2;
        }
        break;
    case 8:
        Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        switch (p->idx) {
        case 0:
            cm = 0;
            order = 7;
            break;
        default:
            order = 3;
            break;
        }
        break;
    case 9:
        Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        order = 3;
        switch (p->idx) {
        case 1:
        case 2:
            mode |= 2;
            break;
        }
        break;
    case 10:
    case 18:
        Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        switch (p->idx) {
        case 0:
        case 2:
            cm = 0;
            order = 7;
            mode |= 2;
            break;
        }
        break;
    case 12:
        Effect_GetColor(gScene.area, &r, &g, &b, &a);
        f = a / 255.0f;
        switch (p->idx) {
        case 0:
            cm = 0;
            order = 7;
            mode |= 2;
            break;
        case 1:
            cm = 0;
            order = 7;
            alpha = f * ((p->color >> 24) & 0xFF);
            color = b | ((g << 8) | ((r << 16) | (alpha << 24)));
            break;
        default:
        case 2:
            order = 3;
            alpha = f * ((p->color >> 24) & 0xFF);
            color = b | ((g << 8) | ((r << 16) | (alpha << 24)));
            break;
        }
        break;
    case 14:
        Effect_GetColor(gScene.area, &r, &g, &b, &a);
        alpha = (a / 255.0f) * ((p->color >> 24) & 0xFF);
        color = b | ((g << 8) | ((r << 16) | (alpha << 24)));
        cm = 2;
        switch (p->idx) {
        case 0:
            order = 3;
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            break;
        case 1:
            order = 0;
            Mtx_Translation(tm, 0.125f * (p->sub & 3), 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            break;
        default:
        case 2:
        case 3:
            order = 3;
            break;
        }
        break;
    case 15:
    case 17:
        Effect_GetColor(gScene.area, &r, &g, &b, &a);
        alpha = (a / 255.0f) * ((p->color >> 24) & 0xFF);
        color = b | ((g << 8) | ((r << 16) | (alpha << 24)));
        Mtx_Identity(tm);
        switch (p->idx) {
        case 0:
            cm = 2;
            order = 3;
            break;
        case 1:
            cm = 0;
            order = 2;
            break;
        case 2:
            cm = 2;
            order = 0;
            Mtx_SetTrans(tm, 0.125f * (p->sub & 3), 0.0f, 0.0f);
            break;
        case 3:
            cm = 0;
            order = 6;
            Mtx_SetTrans(tm, 0.125f * (p->sub & 3), 0.0f, 0.0f);
            break;
        case 4:
        case 5:
            cm = 0;
            order = 6;
            break;
        case 6:
        case 7:
            order = 3;
            break;
        case 8:
        default:
            cm = 0;
            order = 7;
            break;
        }
        Gs_SetState(0x19, (s32) tm);
        break;
    case 16:
        Effect_GetColor(gScene.area, &r, &g, &b, &a);
        alpha = (a / 255.0f) * ((p->color >> 24) & 0xFF);
        color = b | ((g << 8) | ((r << 16) | (alpha << 24)));
        switch (p->idx) {
        case 0:
            cm = 0;
            order = 2;
            break;
        case 1:
            cm = 1;
            order = 3;
            break;
        }
        Mtx_Identity(tm);
        Gs_SetState(0x19, (s32) tm);
        break;
    case 19:
        switch (p->idx) {
        case 0:
            order = 2;
            cm = 0;
            Effect_GetColor(gScene.area, &r, &g, &b, &a);
            alpha = (a / 255.0f) * ((p->color >> 24) & 0xFF);
            color = b | ((g << 8) | ((r << 16) | (alpha << 24)));
            break;
        case 1:
        case 2:
            if (p->sub == 0) {
                Mtx_Identity(tm);
            } else {
                Mtx_Translation(tm, 0.5f, 0.75f, 0.0f);
            }
            cm = 1;
            order = 3;
            break;
        default:
            return;
        }
        Gs_SetState(0x19, (s32) tm);
        break;
    case 20:
    case 21:
        if (p->sub == 0) {
            Mtx_Identity(tm);
        } else {
            Mtx_Translation(tm, 0.5f, 0.75f, 0.0f);
        }
        cm = 1;
        order = 3;
        Gs_SetState(0x19, (s32) tm);
        break;
    case 22:
        cm = 1;
        order = 3;
        break;
    case 23:
        cm = 0;
        order = 2;
        Mtx_Identity(tm);
        Gs_SetState(0x19, (s32) tm);
        break;
    case 24:
        cm = 0;
        order = 2;
        mode |= 2;
        break;
    case 25:
        if (o->timer < 10) {
            f = o->timer / 10.0f;
        } else {
            f = 1.0f;
        }
        alpha = f * ((color >> 24) & 0xFF);
        color = (color & 0xFF) | ((((color >> 8) & 0xFF) << 8) | ((((color >> 16) & 0xFF) << 16) | (alpha << 24)));
        switch (p->idx) {
        case 0:
            cm = 0;
            order = 7;
            Mtx_Translation(tm, 1.0f - (o->timer & 0xFF) / 256.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            break;
        case 1:
            cm = 1;
            order = 3;
            break;
        default:
            return;
        }
        mode |= 2;
        break;
    case 26:
        switch (p->idx) {
        case 0:
            cm = 1;
            order = 3;
            break;
        case 1:
        case 2:
            cm = 0;
            order = 7;
            break;
        default:
            return;
        }
        mode |= 2;
        break;
    case 27:
        mode |= 2;
        cm = 1;
        order = 3;
        Mtx_Identity(tm);
        Gs_SetState(0x19, (s32) tm);
        break;
    case 28:
        cm = 1;
        order = 3;
        if (((color >> 24) & 0xFF) != 0xFF) {
            Mtx_Translation(tm, 0.75f, 0.0f, 0.0f);
        } else {
            Mtx_Identity(tm);
        }
        Gs_SetState(0x19, (s32) tm);
        break;
    case 29:
        switch (p->idx) {
        case 0:
        case 1:
            cm = 0;
            order = 7;
            mode |= 2;
            break;
        case 2:
            order = 3;
            cm = 1;
            break;
        default:
        case 3:
            cm = 0;
            order = 4;
            mode |= 2;
            Mtx_Translation(tm, 0.0f, p->idx / (f32) StageObj_K1C_StepBFrames[p->idx], 0.0f);
            Gs_SetState(0x19, (s32) tm);
            break;
        }
        break;
    case 30:
        order = 3;
        mode |= 4;
        if (p->idx >= 4) {
            Mtx_Translation(tm, 0.25f * (p->sub & 3), 0.25f * (p->sub / 4), 0.0f);
            Gs_SetState(0x19, (s32) tm);
        } else {
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
        }
        break;
    case 31:
        cm = 1;
        order = 3;
        break;
    case 32:
        switch (p->idx) {
        case 0:
            order = 2;
            cm = 0;
            if (p->wait <= 0) {
                Mtx_Identity(tm);
            } else {
                /* w caches p->wait for the translation (the test above reads the field).
                 * Declared s32, it keeps its declaration's number, which loads the 0.0f
                 * arguments before the y argument as in the original; read directly, or
                 * held in an s16, the load is numbered like a created value and the 0.0f
                 * loads come after the div.s (22 words off). */
                s32 w = p->wait;

                Mtx_Translation(tm, 0.0f, 0.125f * (w - 1) / (((u16 *) StageObj_K1C_T2Frames)[p->idx] - 1), 0.0f);
            }
            Gs_SetState(0x19, (s32) tm);
            break;
        case 1:
            order = 2;
            cm = 0;
            break;
        case 2:
            order = 3;
            cm = 1;
            break;
        case 3:
            cm = 2;
            break;
        default:
            return;
        }
        break;
    case 33:
        switch (p->idx) {
        case 0:
            cm = 1;
            order = 3;
            break;
        case 1:
            cm = 0;
            order = 7;
            Mtx_Identity(tm);
            Gs_SetState(0x19, (s32) tm);
            break;
        case 2:
            cm = 1;
            order = 3;
            mode |= 2;
            break;
        default:
            return;
        }
        break;
    case 34:
    case 35:
        cm = 2;
        break;
    case 36:
        cm = 0;
        order = 7;
        mode |= 2;
        break;
    case 37:
        mode |= 2;
        switch (p->idx) {
        case 0:
            cm = 1;
            break;
        case 1:
        case 2:
            cm = 0;
            order = 4;
            break;
        default:
            return;
        }
        break;
    case 38:
        mode |= 2;
        switch (p->idx) {
        case 0:
            cm = 1;
            break;
        case 1:
        case 2:
            cm = 0;
            order = 7;
            Mtx_Translation(tm, 0.0f, 0.1875f * (p->wait / 30.0f), 0.0f);
            Gs_SetState(0x19, (s32) tm);
            break;
        default:
            return;
        }
        break;
    case 39:
        mode |= 2;
        order = 3;
        cm = 1;
        break;
    case 40:
        mode |= 2;
        order = 6;
        cm = 0;
        Mtx_Translation(tm, 0.0f, 0.25f * (p->wait / 8.0f), 0.0f);
        Gs_SetState(0x19, (s32) tm);
        break;
    }
    scale[0] = p->sc[0] * p->scale;
    scale[1] = p->sc[1] * p->scale;
    scale[2] = p->sc[2] * p->scale;
    rot[0] = (6.2831855f * (u32) (u16) p->unk2A) / 65536.0f;
    rot[1] = (6.2831855f * (u32) (u16) p->unk2C) / 65536.0f;
    rot[2] = (6.2831855f * (u32) (u16) p->unk2E) / 65536.0f;
    /* the game reads a register it never set: order is s5, still the caller's, for types 34 and 35 */
    Mtx_FromScaleRotTrans(scale, rot, node->pos, order, m);
    switch (cm) {
    case 0:
        break;
    case 1:
        Mtx_Concat3x3(m, gCamMatrix);
        break;
    case 2:
        Mtx_Concat3x3(m, gCamYawMtx);
        break;
    }
    Gs_SetState(0x6C, 0);
    Effect_DrawModel(model, m, color, mode, mats);
    Gs_SetState(0x6C, 1);
    }
}

void StageObj_K1C_FollowBone(StageObj *o, Obj1CPartA *w, Obj1CPartA *prev, f32 *m, f32 *dir, s16 first)
{
    f32 f;
    s16 t;
    s16 i;
    f32 v[3];
    u16 yaw;
    u16 pitch;

    switch (w->kind) {
    case 0:
        Mtx_GetTrans(w->pos, m);
        Vec_Copy(v, m + 8);
        Vec_DirToAngles(v, (s16 *) &yaw, (s16 *) &pitch);
        dir[0] = 0.0f;
        dir[1] = 0.0f;
        dir[2] = 1.0f;
        Vec_RotateX(dir, (6.2831855f * o->angle2) / 65536.0f);
        Vec_RotateY(dir, (6.2831855f * pitch) / 65536.0f);
        w->base[0][0] = w->pos[0] + 150.0f * dir[0];
        w->base[0][1] = w->pos[1] + 150.0f * dir[1];
        w->base[0][2] = w->pos[2] + 150.0f * dir[2];
        return;
    case 1:
        f = 300.0f * StageObj_K1C_GrowFactor(o);
        break;
    default:
        t = o->timer;
        if (t < 0) {
            f = 0.0f;
        } else if (t < 15) {
            f = 100.0f * (t / 15.0f);
        } else {
            f = 100.0f;
        }
        break;
    }
    if (first == 0) {
        for (i = 0; i < 2; i++) {
            Vec_Copy(w->dir[i], dir);
            Vec_Copy(w->base[i], prev->pos);
        }
    } else {
        if (w->kind == 1) {
            Vec_Copy(w->dir[0], dir);
            Vec_Copy(w->base[0], prev->pos);
        } else {
            for (i = 0; i < 1; i++) {
                Vec_Copy(w->dir[i], w->dir[i + 1]);
                Vec_Copy(w->base[i], w->base[i + 1]);
            }
        }
        Vec_Copy(w->dir[1], dir);
        Vec_Copy(w->base[1], prev->pos);
    }
    w->pos[0] = w->base[0][0] + w->dir[0][0] * f;
    w->pos[1] = w->base[0][1] + w->dir[0][1] * f;
    w->pos[2] = w->base[0][2] + w->dir[0][2] * f;
}

void StageObj_K1C_RandomSpin(Obj1CPartA *w)
{
    w->unk18 = Rand_Next(1);
    w->unk1A = (s16) ((Rand_Next(1) & 0xFFFF & 0x1FF) - 0x100);
}

static void StageObj_K1C_NextStepA(StageObj *o, Obj1CPart *w)
{
    u8 k;

    k = D_008D6138[o->var2];
    if (k == 0xFF) {
        w->idx = 0xFF;
        return;
    }
    w->idx = k;
    if (w->idx == 2) {
        switch (o->var2) {
        case 8:
        default:
            w->sub = 0;
            break;
        case 10:
            w->sub = 1;
            break;
        case 11:
            w->sub = 2;
            break;
        case 12:
            w->sub = 3;
            break;
        }
    }
    w->wait = o->var1 - StageObj_K1C_StepAStart[o->var2];
    o->var2++;
}

void StageObj_K1C_InitStepA(StageObj *o, Obj1CPart *w)
{
    f32 f;

    switch (w->idx) {
    case 0:
        w->scale = o->scale;
        w->unk37 = 0;
        w->unk2A = (u16) (9102.0f * Rand_Unit()) + 0x31C7;
        f = 60.0f;
        break;
    case 1:
        w->scale = o->scale;
        w->unk37 = 0;
        w->unk2A = (u16) (5461.0f * Rand_Unit()) + 0x38E4;
        f = 600.0f;
        Vec_Copy(w->pos, o->pos);
        w->unk2C = o->angle2;
        break;
    case 2:
    default:
        w->scale = 0.75f * o->scale;
        w->unk37 = 1;
        f = 30.0f;
        w->unk2A = 0;
        Vec_Copy(w->pos, o->pos);
        w->unk2C = o->angle2 + (w->sub << 14);
        w->unk38 = 120.0f * w->scale;
        break;
    }
    w->unk2E = Rand_Next(1);
    w->unk34 = 0.5f + 65536.0f * (f / StageObj_K1C_StepAFrames[w->idx]) / 360.0f;
    if (Rand_Next(1) & 1) {
        w->unk34 = -w->unk34;
    }
}

s32 StageObj_K1C_AllDone(StageObj *o)
{
    s16 n = gStageObjK1C_Counts[o->type];
    u8 *p = o->buf;
    s16 i;

    for (i = 0; i < n; i++) {
        if (p[2] != 0xFF) {
            return 0;
        }
        p += 0x40;
    }
    return 1;
}

void StageObj_K1C_RestartStepsA(StageObj *o)
{
    Obj1CPart *w = (Obj1CPart *) o->buf;
    s16 i;

    o->var2 = 0;
    for (i = 0; i < 7; i++, w++) {
        StageObj_K1C_NextStepA(o, w);
    }
}

void StageObj_K1C_NextStepC(StageObj *o, Obj1CPart *w)
{
    u8 *ks;
    u16 *ws;
    u8 k;

    switch (o->type) {
    case 15:
        ks = D_008D6738;
        ws = StageObj_K1C_StepCWaits;
        break;
    case 17:
        ks = StageObj_K1C_StepCKinds17;
        ws = StageObj_K1C_StepCWaits17;
        break;
    default:
        return;
    }
    k = ks[o->var2];
    if (k == 0xFF) {
        w->idx = 0xFF;
        return;
    }
    w->idx = k;
    switch (w->idx) {
    case 4:
        if (Rand_Next(1) & 1) {
            w->idx++;
        }
        switch (o->var2) {
        case 4:
        default:
            w->sub = 0;
            break;
        case 5:
            w->sub = 1;
            break;
        case 6:
            w->sub = 2;
            break;
        case 7:
            w->sub = 3;
            break;
        }
        break;
    case 6:
        if (Rand_Next(1) & 1) {
            w->idx++;
        }
        switch (o->var2) {
        case 8:
        default:
            w->sub = 0;
            break;
        case 9:
            w->sub = 1;
            break;
        case 10:
            w->sub = 2;
            break;
        case 11:
            w->sub = 3;
            break;
        }
        break;
    }
    w->wait = o->timer - ws[o->var2];
    o->var2++;
}

void StageObj_K1C_InitStepC(StageObj *o, Obj1CPart *w)
{
    w->unk2A = 0;
    w->unk2C = 0;
    w->unk2E = 0;
    w->unk30 = 0;
    w->unk32 = 0;
    w->unk34 = 0;
    Vec_Copy(w->pos, o->pos);
    switch (w->idx) {
    case 0:
        w->unk37 = 1;
        w->unk2E = 0x4000;
        break;
    case 1:
        w->unk37 = 0;
        w->unk2C = Rand_Next(1);
        w->unk32 = 0.5f + 65536.0f * (48.0f / StageObj_K1C_StepCFrames[w->idx]) / 360.0f;
        break;
    case 2:
        w->unk37 = 1;
        w->unk2E = 0x4000;
        w->sub = Rand_Next(1) & 3;
        break;
    case 3:
        w->unk37 = 0;
        w->unk2C = Rand_Next(1);
        w->unk38 = 5.0f * w->scale;
        break;
    case 4:
    case 5:
        w->unk37 = 0;
        w->unk2A = Rand_Next(1);
        w->unk2C = Rand_Next(1);
        w->unk2E = Rand_Next(1);
        w->unk30 = (Rand_Next(1) & 0x1FF) - 0x100;
        w->unk32 = (Rand_Next(1) & 0x1FF) - 0x100;
        w->unk34 = (Rand_Next(1) & 0x1FF) - 0x100;
        w->unk38 = w->scale * ((400.0f + 200.0f * Rand_Unit()) / StageObj_K1C_StepCFrames[w->idx]);
        w->unk3C = w->scale * (70.0f + 25.0f * Rand_Unit());
        break;
    case 6:
    case 7:
        w->unk37 = 1;
        w->unk2E = Rand_Next(1);
        w->unk34 = 0.5f + 65536.0f * (50.0f / StageObj_K1C_StepCFrames[w->idx]) / 360.0f;
        if (Rand_Next(1) & 1) {
            w->unk34 = -w->unk34;
        }
        w->unk2C = o->angle + (w->sub << 14);
        w->unk38 = 30.0f * w->scale;
        break;
    case 8:
    default:
        w->unk37 = 0;
        w->unk2A = (u16) (5461.0f * Rand_Unit()) + 0x38E4;
        w->unk2E = Rand_Next(1);
        w->unk34 = 0.5f + 65536.0f * (600.0f / StageObj_K1C_StepCFrames[w->idx]) / 360.0f;
        if (Rand_Next(1) & 1) {
            w->unk34 = -w->unk34;
        }
        break;
    }
}

f32 StageObj_K1C_GrowFactor(StageObj *o)
{
    f32 f;
    s16 t;

    switch (o->type) {
    default:
        return 1.0f;
    case 0:
        t = o->timer;
        if (t < 0) {
            return 0.2f;
        }
        if (t < 15) {
            f = 0.2f + 0.8f * t / 15.0f;
        } else {
            f = 1.0f;
        }
        return f;
    }
}

/* Level 3: at level 4 the index and the table base swap registers (5 words); no index form
 * or declaration order fixed it. */
#pragma optimization_level 3
void StageObj_K1C_NextStepD(StageObj *o, Obj1CPart *w)
{
    u16 *ws = StageObj_K1C_StepDWaits;
    u8 *ks;
    u8 k;
    s32 a;

    ks = StageObj_K1C_StepDKinds;
    k = *(ks += o->var2);
    if (k == 0xFF) {
        w->idx = 0xFF;
        return;
    }
    w->idx = k;
    w->wait = o->timer - ws[o->var2];
    switch (w->idx) {
    case 1:
        a = o->angle + 0x7FFF + 1 + (o->var2 % 3 - 1) * 0x18E4;
        w->unk2C = a + ((Rand_Next(1) & 0x1FFF) - 0x1000);
        break;
    case 2:
        a = o->angle + 0x7FFF + 1 + ((o->var2 & 1) - 1) * 0x2000;
        w->unk2C = a + (Rand_Next(1) & 0x1FFF);
        break;
    default:
        w->unk2C = o->angle;
        break;
    }
    o->var2++;
}
#pragma optimization_level reset

void StageObj_K1C_InitStepD(StageObj *o, Obj1CPart *w)
{
    f32 y = w->pos[1];

    Vec_Copy(w->pos, o->pos);
    switch (w->idx) {
    case 0:
        w->unk37 = 0;
        w->scale = o->scale;
        w->pos[1] = y;
        break;
    case 1:
        w->unk37 = 1;
        w->unk2E = Rand_Next(1);
        w->unk34 = 0x5B;
        w->sub = Rand_Next(1) & 1;
        w->scale = o->scale;
        w->unk38 = 20.0f * w->scale;
        break;
    case 2:
        w->unk37 = 1;
        w->unk2E = Rand_Next(1);
        w->unk34 = 0x5B;
        w->sub = Rand_Next(1) & 1;
        w->scale = 0.5f * o->scale;
        w->unk38 = 15.0f * w->scale;
        break;
    }
}

void StageObj_K1C_PlaceBehindOwner(StageObj *o)
{
    f32 *m;
    Em *em;
    f32 v[3];

    em = o->owner;
    m = (f32 *)Em_Bone(em, 0xE);
    v[0] = 0.0f;
    v[1] = -50.0f;
    v[2] = -200.0f;
    Vec_RotateSelf(v, m);
    o->pos[0] = m[12] + v[0];
    o->pos[1] = m[13] + v[1];
    o->pos[2] = m[14] + v[2];
    o->angle = em->rotY + 0x7FFF + 1;
}

void StageObj_K1C_InitStepB(StageObj *o, Obj1CPart *w)
{
    Em *em = o->owner;
    f32 v[3];

    w->unk2A = 0;
    w->unk2C = 0;
    w->unk2E = 0;
    w->unk30 = 0;
    w->unk32 = 0;
    w->unk34 = 0;
    w->scale = o->scale;
    switch (w->idx) {
    case 0:
        w->unk2A = Rand_Next(1);
        w->unk2C = Rand_Next(1);
        w->unk2E = Rand_Next(1);
        w->unk34 = 0.5f + (65536.0f * (16.0f / StageObj_K1C_StepBFrames[w->idx])) / 360.0f;
        if (!(Rand_Next(1) & 1)) {
            w->unk34 = -w->unk34;
        }
        w->unk37 = 0;
        break;
    case 1:
        w->unk2A = (s16) (0.5f + (65536.0f * (70.0f + 50.0f * Rand_Unit())) / 360.0f);
        w->unk2C = o->angle2;
        w->unk2E = Rand_Next(1);
        w->unk37 = 0;
        w->unk34 = 0.5f + (65536.0f * (60.0f / StageObj_K1C_StepBFrames[w->idx])) / 360.0f;
        if (!(Rand_Next(1) & 1)) {
            w->unk34 = -w->unk34;
        }
        break;
    case 2:
        w->unk37 = 1;
        w->unk2E = Rand_Next(1);
        w->unk34 = 0.5f + (65536.0f * (10.0f / StageObj_K1C_StepBFrames[w->idx])) / 360.0f;
        if (!(Rand_Next(1) & 1)) {
            w->unk34 = -w->unk34;
        }
        Vec_Copy(w->pos, o->pos);
        v[0] = w->scale * (300.0f * Rand_Signed());
        v[1] = w->scale * (150.0f * Rand_Signed());
        v[2] = w->scale * (400.0f * Rand_Signed());
        Vec_RotateY(v, (6.2831855f * o->angle2) / 65536.0f);
        w->pos[0] += v[0];
        w->pos[1] += v[1];
        w->pos[2] += v[2];
        w->unk38 = (100.0f + 100.0f * Rand_Unit()) / StageObj_K1C_StepBFrames[w->idx];
        if (!(o->angle & 1)) {
            Em_PlaySeChecked((struct Em *)em, 0x18, 0, w->pos, 6, 0);
        }
        o->angle++;
        break;
    case 3:
    default:
        w->unk2A = (s16) (0.5f + (65536.0f * (70.0f + 50.0f * Rand_Unit())) / 360.0f);
        w->unk2C = Rand_Next(1);
        if (!(Rand_Next(1) & 1)) {
            w->unk32 = -w->unk32;
        }
        w->unk37 = 0;
        w->scale *= 1.3f;
        Vec_Copy(w->pos, o->pos);
        w->unk38 = (200.0f + 100.0f * Rand_Unit()) / StageObj_K1C_StepBFrames[w->idx];
        break;
    }
}

static void StageObj_K1C_NextStepB(StageObj *o, Obj1CPart *w)
{
    u8 k;

    if (w->idx == 0) {
        w->wait = 1;
        return;
    }
    k = StageObj_K1C_StepBKinds[o->var2];
    if (k == 0xFF) {
        w->idx = 0xFF;
        return;
    }
    w->idx = k;
    w->wait = o->var1 - StageObj_K1C_StepBStart[o->var2];
    o->var2++;
}

void StageObj_K1C_RestartStepsB(StageObj *o)
{
    Obj1CPart *w = (Obj1CPart *) o->buf + 2;
    s16 i;

    o->var2 = 2;
    for (i = 2; i < 8; i++, w++) {
        StageObj_K1C_NextStepB(o, w);
    }
}

s32 StageObj_K1C_StepsDone(StageObj *o)
{
    Obj1CPart *w = (Obj1CPart *) o->buf + 2;
    s16 n = gStageObjK1C_Counts[o->type];
    s16 i;

    for (i = 2; i < n; i++, w++) {
        if (w->idx != 0xFF) {
            return 0;
        }
    }
    return 1;
}

void StageObj_K1C_GetOwnerBone(StageObj *o, f32 *out)
{
    Em *em = o->owner;

    switch (o->var2) {
    default:
        Em_BonePos(em, 0x15, out);
        break;
    case 1:
        Em_BonePos(em, 0x1E, out);
        break;
    case 2:
        Em_BonePos(em, 0x19, out);
        break;
    }
}

s32 StageObj_K1C_OwnerActive(Em *em)
{
    if (em->active == 0 || em->stateVar1 == 3) {
        return 0;
    }
    return 1;
}

void StageObj_K1C_Jitter(Obj1CPart *w, f32 r)
{
    w->pos[0] += r * Rand_Signed();
    w->pos[1] += r * Rand_Signed();
    w->pos[2] += r * Rand_Signed();
}

void StageObj_RandomDistinct(s16 n, s16 count, s16 *out)
{
    s16 used[4];
    s16 j;
    s16 k;
    s16 i;

    if (n > 4) {
        n = 4;
    }
    for (i = 0; i < count; i++) {
        out[i] = Rand_Next(1) % (n - i);
        for (j = 0; j < i; j++) {
            used[j] = 0;
        }
        while (1) {
            k = 0;
            for (j = 0; j < i; j++) {
                if (used[j] == 0 && out[j] <= out[i]) {
                    used[j] = 1;
                    k++;
                }
            }
            if (k <= 0) {
                break;
            }
            out[i] += k;
        }
    }
}

StageObj *StageObj_K1C_Alloc(u8 k)
{
    s16 n;

    switch (k) {
    case 0:
    case 0xB:
        n = 2;
        break;
    case 1:
        n = 2;
        break;
    case 2:
    case 3:
    case 4:
    default:
        n = 1;
        break;
    }
    return StageObj_Alloc(n);
}

StageObj *StageObj_K1C_CreateOnEm(Em *em, u8 k, f32 scale)
{
    StageObj *o;

    if (k != 0xC && k != 0x1D && !((s32)Player_InCurrentArea(em) & 0xFF)) {
        return NULL;
    }
    o = StageObj_K1C_Alloc(k);
    if (o == NULL) {
        return NULL;
    }
    o->kind = 0x1C;
    o->update = StageObj_K1C_Update;
    o->owner = em;
    o->type = k;
    o->scale = scale;
    return o;
}

void StageObj_K1C_CreateOnEmC(Em *em, u8 k, u8 c, f32 scale)
{
    StageObj *o = StageObj_K1C_CreateOnEm(em, k, scale);

    if (o != NULL) {
        o->var2 = c;
    }
}

void StageObj_K1C_CreateAt(f32 *src, u8 k, f32 f)
{
    StageObj *o = StageObj_K1C_Alloc(k);

    if (o != NULL) {
        o->kind = 0x1C;
        o->update = StageObj_K1C_Update;
        o->type = k;
        o->scale = f;
        Vec_Copy(o->pos, src);
    }
}

void StageObj_K1C_CreateTwoPoint(f32 *pos, f32 *pos2, u8 k, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_K1C_Alloc(k)) != NULL) {
        o->kind = 0x1C;
        o->update = StageObj_K1C_Update;
        o->type = k;
        o->scale = scale;
        Vec_Copy(o->pos, pos);
        Vec_Copy((f32 *) (o->buf + 4), pos2);
    }
}

StageObj *StageObj_K1C_CreateAtYaw(f32 *pos, u16 v, u8 k, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_K1C_Alloc(k)) == NULL) {
        return NULL;
    }
    o->kind = 0x1C;
    o->update = StageObj_K1C_Update;
    o->type = k;
    o->scale = scale;
    o->angle = v;
    Vec_Copy(o->pos, pos);
    return o;
}

void StageObj_K1C_CreateAtAngled(f32 *pos, u16 w, u16 v, u8 k, f32 scale)
{
    StageObj *o = StageObj_K1C_CreateAtYaw(pos, v, k, scale);

    if (o != NULL) {
        o->angle2 = w;
    }
}

void StageObj_K1C_CreateOnEmB(Em *em, u8 k, u16 w, s16 v, f32 scale)
{
    StageObj *o;

    if ((s32)Player_InCurrentArea(em) & 0xFF) {
        o = StageObj_K1C_Alloc(k);
        if (o != NULL) {
            o->kind = 0x1C;
            o->update = StageObj_K1C_Update;
            o->owner = em;
            o->type = k;
            o->scale = scale;
            o->angle2 = w;
            o->angle = v;
        }
    }
}

void StageObj_AreaFlyer_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_AreaFlyer_Setup(o);
        break;
    case 1:
        StageObj_AreaFlyer_Run(o);
        break;
    case 2:
        StageObj_AreaFlyer_End(o);
        break;
    case 3:
        StageObj_AreaFlyer_Free(o);
        break;
    }
}

void StageObj_AreaFlyer_Setup(StageObj *o)
{
    AreaState *area = &gAreaState;

    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    o->timer = 0;
    o->angle = 0;
    o->ownerIndex = 0;
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    StageObj_AreaFlyer_GetCenter(gAreaState.flyerPos, o->type);
    o->scale = 0.7f;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->draw = StageObj_AreaFlyer_Draw;
    } else {
        area->flyer = 0;
        o->node = NULL;
    }
    StageObj_AreaFlyer_Run(o);
}

void StageObj_AreaFlyer_Run(StageObj *o)
{
    AreaState *area = &gAreaState;
    f32 *base;
    f32 a;

    base = gAreaFlyerCenters[o->type];
    o->timer += 0x12;
    o->angle += 5;
    o->ownerIndex++;
    if (o->ownerIndex > 5) {
        o->ownerIndex = 0;
    }
    a = (6.2831855f * o->angle) / 65536.0f;
    area->flyerPos[0] = base[0] + 1400.0f * Math_Sin(3.0f * a);
    area->flyerPos[1] = base[1] + 1400.0f * Math_Sin(4.0f * a);
    area->flyerPos[2] = base[2] + 1400.0f * Math_Sin(5.0f * a);
    if (o->var0 == 0) {
        if ((u16) gPlayers[gScene.localPlayer].actDetectTimer != 0) {
            o->var0 = 1;
        }
    } else {
        o->var0++;
        if (o->var0 > 0xD) {
            if (++o->var1 > 0xD) {
                o->var0 = 0;
                o->var1 = 0;
            }
        }
    }
    if (o->node != NULL) {
        o->node->pos[0] = area->flyerPos[0];
        o->node->pos[1] = area->flyerPos[1];
        o->node->pos[2] = area->flyerPos[2];
        OT_AddByDepth(gDrawOt32, o->node, 0x20, 0);
    }
}

void StageObj_AreaFlyer_End(StageObj *o)
{
    o->state++;
    if (o->node != NULL) {
        DrawSlot_Free(o->slot);
    }
}

void StageObj_AreaFlyer_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_AreaFlyer_Draw(DrawSlot *node)
{
    StageObj *o;
    ModelMat *mats;
    Model *mdl;
    ModelMatSet *mesh;
    ModelMat *e;
    s16 i;
    s32 k;
    f32 scale[3];
    f32 rot[3];
    f32 dir[3];
    f32 m[16];
    f32 m2[16];

    mdl = gCommonModels[0];
    o = node->owner;
    if (gScene.eventRunning == 0 && mdl != NULL && mdl->active != 0 && o->visible != 0) {
        mats = mdl->mats;
        k = -1;
        mesh = &mdl->matSets[150];
        scale[0] = o->scale;
        scale[1] = o->scale;
        scale[2] = o->scale;
        rot[0] = 0.0f;
        rot[1] = (6.2831855f * (u16) o->timer) / 65536.0f;
        rot[2] = 0.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 2, m);
        for (i = 1; i < mesh->count; i++) {
            e = &mats[mesh->mat[i]];
            if (i == o->ownerIndex / 2 + 1) {
                e->alpha = 1.0f;
            } else {
                e->alpha = 0.0f;
            }
        }
        Effect_DrawModel(mesh, m, k, 0, mats);
        if (o->var0 != 0 && o->var1 == 0) {
            Gs_SetState(0x6C, 0);
            Mtx_Identity(m2);
            Gs_SetState(0x19, (s32) m2);
            Key_LerpVec2(gAreaFlyerScaleKeys, scale, o->var0);
            Color_EvalKeys((s32 *)gAreaFlyerColorKeys, o->var0, (u32 *)&k);
            dir[0] = gCamMatrix[12] - node->pos[0];
            dir[1] = gCamMatrix[13] - node->pos[1];
            dir[2] = gCamMatrix[14] - node->pos[2];
            Vec_Normalize(dir);
            dir[0] *= 400.0f;
            dir[1] *= 400.0f;
            dir[2] *= 400.0f;
            dir[0] += node->pos[0];
            dir[1] += node->pos[1];
            dir[2] += node->pos[2];
            Mtx_FromScaleRotTrans(scale, rot, dir, 0, m);
            Mtx_Concat3x3(m, gCamMatrix);
            Effect_DrawModel(&mdl->matSets[91], m, k, 2, mats);
            Gs_SetState(0x6C, 1);
        }
    }
}

void StageObj_AreaFlyer_GetCenter(f32 *out, u8 idx)
{
    Vec_Copy(out, gAreaFlyerCenters[idx]);
}

void StageObj_AreaFlyer_Create(u8 k)
{
    AreaState *area = &gAreaState;
    StageObj *o;

    if (gQuest.flyer != 0) {
        o = StageObj_Alloc(0);
        if (o != NULL) {
            o->kind = 0x1D;
            o->update = StageObj_AreaFlyer_Update;
            o->type = k;
            area->flyer = 1;
        } else {
            area->flyer = 0;
        }
    }
}

void StageObj_Gun_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_Gun_Setup(o);
        break;
    case 1:
        StageObj_Gun_Run(o);
        break;
    case 2:
        StageObj_Gun_End(o);
        break;
    case 3:
        StageObj_Gun_Free(o);
        break;
    }
}

/* a part of a kind 0x1F object (0x20 bytes; the view of StageObj_Gun_Draw in unk_88E760.c) */
typedef struct Obj1FPart {
    s16 slot;                   /* 0x00 */
    s8 t;                       /* 0x02: its frame, negative: not shown yet */
    s8 frame;                   /* 0x03: a texture frame (4 x 4) */
    s32 color;                  /* 0x04 */
    f32 sc[3];                  /* 0x08: its scale */
    DrawSlot *node;                /* 0x14 */
    f32 unk18;                  /* 0x18: a size */
    s16 unk1C;                  /* 0x1C: its spin */
    s16 angle;                  /* 0x1E: its z angle */
} Obj1FPart;

/* the work block of a kind 0x1F object */
typedef struct Obj1FWork {
    f32 m[16];                  /* 0x000 */
    f32 scale;                  /* 0x040 */
    Obj1FPart part[11];         /* 0x044 */
    u8 count;                   /* 0x1A4: parts in use */
    s8 unk1A5;                  /* 0x1A5 */
    s8 unk1A6;                  /* 0x1A6 */
    u8 unk1A7;                  /* 0x1A7 */
    u8 unk1A8[3];               /* 0x1A8: type 3 */
    u8 unk1AB[3];               /* 0x1AB */
    s8 unk1AE[3];               /* 0x1AE */
    s8 unk1B1[3];               /* 0x1B1 */
    s8 unk1B4[3];               /* 0x1B4 */
    u8 unk1B7;                  /* 0x1B7 */
    s32 angle[3];               /* 0x1B8: types 2 and 3 */
    s16 slot[3];                /* 0x1C4: type 3 */
    u8 unk1CA[2];
    DrawSlot *node[3];             /* 0x1CC */
} Obj1FWork;

EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void Vec_Scale(f32 *out, f32 *v, f32 s);
void StageObj_Gun_Draw(DrawSlot *node);

extern u32 StageObj_Gun_Pos01[], StageObj_Gun_Pos23[], StageObj_Gun_Vars01[], StageObj_Gun_Vars23[], StageObj_Gun_Scale01[], StageObj_Gun_Scale23[], StageObj_Gun_Life4[], StageObj_Gun_Param4[], StageObj_Gun_ScaleZ1[], StageObj_Gun_Jitter1[], StageObj_Gun_Jitter2[], StageObj_Gun_ScaleZ3[], StageObj_Gun_Life1[], StageObj_Gun_Life6[], StageObj_Gun_Param6[];
void StageObj_Gun_Setup(StageObj *o)
{
    Obj1FWork *w;
    Obj1FPart *p;
    u8 i;
    u8 j;
    f32 (*pos)[3];
    s32 *var;
    f32 *sc;
    f32 *r;
    f32 f;
    f32 b[3];
    f32 a[3];
    f32 d[3];
    f32 rp[3][3];

    w = (Obj1FWork *) o->buf;
    o->var0 = 0;
    o->var1 = 0;
    o->timer = 0;
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    p = w->part;
    Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 0xE)));
    switch (o->type) {
    case 0:
        w->count = 3;
        Mtx_GetTrans(o->pos, w->m);
        w->scale = o->scale;
        for (i = 0; i < w->count; i++, p++) {
            p->slot = DrawSlot_Alloc();
            if (p->slot != -1) {
                p->node = DrawSlot_Get(p->slot);
                p->node->owner = o;
                p->node->part = i;
                p->node->draw = StageObj_Gun_Draw;
            } else {
                p->node = NULL;
            }
        }
        break;
    case 1:
        switch (o->var2) {
        case 0:
            w->count = 3;
            pos = (f32 (*)[3]) StageObj_Gun_Pos01[0];
            var = (s32 *) StageObj_Gun_Vars01[0];
            sc = (f32 *) StageObj_Gun_Scale01[0];
            break;
        case 1:
            w->count = 9;
            pos = (f32 (*)[3]) StageObj_Gun_Pos01[1];
            var = (s32 *) StageObj_Gun_Vars01[1];
            sc = (f32 *) StageObj_Gun_Scale01[1];
            break;
        case 2:
            w->count = 6;
            r = (f32 *) StageObj_Gun_Pos23[0];
            for (j = 0; j < 3; j++) {
                rp[j][0] = (f32) (u32) (1.0f + 2.0f * r[0]) * Rand_Unit() - 150.0f;
                rp[j][1] = (f32) (u32) (1.0f + 2.0f * r[1]) * Rand_Unit() - 150.0f;
                rp[j][2] = (f32) (u32) (1.0f + 2.0f * r[2]) * Rand_Unit() - 150.0f;
            }
            pos = rp;
            var = (s32 *) StageObj_Gun_Vars23[0];
            sc = (f32 *) StageObj_Gun_Scale23[0];
            break;
        case 3:
            w->count = 11;
            pos = (f32 (*)[3]) StageObj_Gun_Pos23[1];
            var = (s32 *) StageObj_Gun_Vars23[1];
            sc = (f32 *) StageObj_Gun_Scale23[1];
            break;
        }
        for (i = 0; i < w->count; i++, p++) {
            p->sc[2] = 0.0f;
            switch (i) {
            case 0:
            case 1:
            case 2:
                f = 25.0f * Rand_Signed();
                a[0] = f + pos[i][0];
                a[1] = f + pos[i][1];
                a[2] = f + pos[i][2];
                Vec_RotateY(a, 0.5462881f);
                Vec_RotateSelf(a, w->m);
                b[0] = 105.0f;
                b[1] = 15.0f;
                b[2] = 167.0f;
                Vec_RotateSelf(b, w->m);
                b[0] += a[0];
                b[1] += a[1];
                b[2] += a[2];
                Mtx_GetTrans(o->pos, w->m);
                p->t = var[i];
                p->angle = Rand_Next(1);
                p->unk1C = 0.5f + 65536.0f * ((s16) (s32) (30.0f * Rand_Signed()) / 30.0f) / 360.0f;
                p->unk18 = sc[i];
                break;
            case 3:
            case 4:
            case 5:
            case 9:
                a[0] = pos[i - 3][0];
                a[1] = pos[i - 3][1];
                a[2] = pos[i - 3][2];
                Vec_RotateY(a, 0.5462881f);
                Vec_RotateSelf(a, w->m);
                b[0] = 105.0f;
                b[1] = 15.0f;
                b[2] = 167.0f;
                Vec_RotateSelf(b, w->m);
                b[0] += a[0];
                b[1] += a[1];
                b[2] += a[2];
                Mtx_GetTrans(o->pos, w->m);
                p->t = var[i];
                p->unk18 = sc[i - 3];
                p->frame = -1;
                p->angle = 0;
                p->unk1C = 0;
                break;
            case 6:
            case 7:
            case 8:
            case 10:
                a[0] = pos[i - 6][0];
                a[1] = pos[i - 6][1];
                a[2] = pos[i - 6][2];
                Vec_RotateY(a, 0.5462881f);
                Vec_RotateSelf(a, w->m);
                b[0] = 105.0f;
                b[1] = 15.0f;
                b[2] = 167.0f;
                Vec_RotateSelf(b, w->m);
                b[0] += a[0];
                b[1] += a[1];
                b[2] += a[2];
                Mtx_GetTrans(o->pos, w->m);
                p->t = var[i];
                p->unk18 = sc[i - 6];
                p->angle = Rand_Next(1);
                p->unk1C = 0;
                p->frame = 0;
                break;
            }
            p->slot = DrawSlot_Alloc();
            if (p->slot != -1) {
                p->node = DrawSlot_Get(p->slot);
                p->node->owner = o;
                p->node->part = i;
                p->node->draw = StageObj_Gun_Draw;
                p->node->pos[0] = o->pos[0] + b[0];
                p->node->pos[1] = o->pos[1] + b[1];
                p->node->pos[2] = o->pos[2] + b[2];
            } else {
                p->node = NULL;
            }
        }
        break;
    case 2:
        Mtx_GetTrans(o->pos, w->m);
        w->unk1A5 = -5;
        w->unk1A6 = 10;
        o->slot = DrawSlot_Alloc();
        if (o->slot != -1) {
            o->node = DrawSlot_Get(o->slot);
            o->node->owner = o;
            o->node->draw = StageObj_Gun_Draw;
        } else {
            StageObj_Free(o);
        }
        break;
    case 3:
        w->unk1A7 = 0;
        for (i = 0; i < 3; i++) {
            w->unk1B1[i] = 2;
            w->unk1B4[i] = 0x14;
            w->unk1A8[i] = 0;
            if (i == 0) {
                w->unk1AE[i] = 0;
            } else {
                w->unk1AE[i] = 10.0f * Rand_Unit();
            }
            w->unk1AB[i] = 0;
            w->angle[i] = 0;
            w->slot[i] = DrawSlot_Alloc();
            if (w->slot[i] != -1) {
                w->node[i] = DrawSlot_Get(w->slot[i]);
                w->node[i]->owner = o;
                w->node[i]->part = i;
                w->node[i]->draw = StageObj_Gun_Draw;
                b[0] = 35.714287f;
                b[1] = 5.714286f;
                b[2] = 68.57143f;
                Vec_RotateSelf(b, w->m);
                Mtx_GetTrans(w->node[i]->pos, w->m);
                w->node[i]->pos[0] += b[0];
                w->node[i]->pos[1] += b[1];
                w->node[i]->pos[2] += b[2];
            } else {
                w->node[i] = NULL;
            }
        }
        break;
    case 4:
        w->count = 8;
        for (i = 0; i < w->count; i++, p++) {
            p->t = ((s8 *) StageObj_Gun_Life4)[i] - Rand_Next(1) % 2;
            p->unk18 = ((f32 *) StageObj_Gun_Param4)[0];
            p->slot = DrawSlot_Alloc();
            if (p->slot != -1) {
                p->node = DrawSlot_Get(p->slot);
                p->node->owner = o;
                p->node->part = i;
                p->node->draw = StageObj_Gun_Draw;
                switch (i) {
                case 0:
                case 1:
                    b[0] = 128.0f;
                    b[1] = 25.0f;
                    b[2] = 201.0f;
                    break;
                case 2:
                case 3:
                case 4:
                case 5:
                    b[0] = 105.0f + 10.0f * Rand_Signed();
                    b[1] = 15.0f + 10.0f * Rand_Signed();
                    b[2] = 167.0f - 80.0f * Rand_Unit();
                    p->angle = Rand_Next(1);
                    p->unk1C = 0.5f + 65536.0f * ((s16) (s32) (15.0f * Rand_Signed()) / 15.0f) / 360.0f;
                    break;
                case 6:
                case 7:
                    b[0] = 105.0f;
                    b[1] = 15.0f;
                    b[2] = 167.0f;
                    p->sc[2] = 80.0f + 20.0f * Rand_Unit();
                    p->angle = Rand_Next(1);
                    break;
                }
                Vec_RotateSelf(b, w->m);
                Mtx_GetTrans(o->pos, w->m);
                p->node->pos[0] = o->pos[0] + b[0];
                p->node->pos[1] = o->pos[1] + b[1];
                p->node->pos[2] = o->pos[2] + b[2];
            } else {
                p->node = NULL;
            }
        }
        break;
    case 5:
        w->unk1B7 = 0;
        w->count = 4;
        for (i = 0; i < w->count; i++, p++) {
            switch (i) {
            case 0:
            case 1:
                p->unk18 = 1.0f + Rand_Unit();
                p->angle = Rand_Next(1);
                p->sc[2] = ((f32 *) StageObj_Gun_ScaleZ1)[w->unk1B7] * Rand_Unit() + 2.0f * ((f32 *) StageObj_Gun_ScaleZ1)[w->unk1B7];
                b[0] = ((f32 *) StageObj_Gun_Jitter1)[w->unk1B7] * Rand_Signed();
                b[1] = -25.0f + ((f32 *) StageObj_Gun_Jitter1)[w->unk1B7] * Rand_Signed();
                b[2] = 80.0f + ((f32 *) StageObj_Gun_Jitter1)[w->unk1B7] * Rand_Signed();
                p->t = ((s8 *) StageObj_Gun_Life1)[i];
                break;
            case 2:
                p->unk18 = 0.01f * (100.0f + 20.0f * Rand_Unit());
                p->angle = Rand_Next(1);
                p->sc[2] = 100.0f + 20.0f * Rand_Unit();
                b[0] = ((f32 (*)[3]) StageObj_Gun_Jitter2)[w->unk1B7][0] * Rand_Signed();
                b[1] = -25.0f + ((f32 (*)[3]) StageObj_Gun_Jitter2)[w->unk1B7][1] * Rand_Signed();
                b[2] = 80.0f + ((f32 (*)[3]) StageObj_Gun_Jitter2)[w->unk1B7][2] * Rand_Signed();
                p->t = -1.0f * (10.0f + 10.0f * Rand_Unit());
                break;
            case 3:
                p->unk18 = 1.0f;
                p->angle = Rand_Next(1);
                p->sc[2] = ((f32 *) StageObj_Gun_ScaleZ3)[w->unk1B7] * Rand_Unit() + 4.0f * ((f32 *) StageObj_Gun_ScaleZ3)[w->unk1B7];
                p->unk1C = 0;
                b[0] = 0.0f;
                b[1] = -25.0f;
                b[2] = 80.0f;
                p->t = -1.0f * (5.0f + 5.0f * Rand_Signed());
                break;
            }
            p->frame = 0;
            p->slot = DrawSlot_Alloc();
            if (p->slot != -1) {
                p->node = DrawSlot_Get(p->slot);
                p->node->owner = o;
                p->node->part = i;
                p->node->draw = StageObj_Gun_Draw;
                Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 0x1F)));
                Vec_RotateSelf(b, w->m);
                Mtx_GetTrans(o->pos, w->m);
                p->node->pos[0] = o->pos[0] + b[0];
                p->node->pos[1] = o->pos[1] + b[1];
                p->node->pos[2] = o->pos[2] + b[2];
            } else {
                p->node = NULL;
            }
        }
        break;
    case 6:
        w->count = 4;
        for (i = 0; i < w->count; i++, p++) {
            p->t = ((s8 *) StageObj_Gun_Life6)[i] - Rand_Next(1) % 2;
            p->unk18 = ((f32 *) StageObj_Gun_Param6)[0];
            p->slot = DrawSlot_Alloc();
            if (p->slot != -1) {
                p->node = DrawSlot_Get(p->slot);
                p->node->owner = o;
                p->node->part = i;
                p->node->draw = StageObj_Gun_Draw;
                p->angle = Rand_Next(1);
                p->unk1C = 0.5f + 65536.0f * ((s16) (s32) (15.0f * Rand_Signed()) / 15.0f) / 360.0f;
                b[0] = 105.0f;
                b[1] = 15.0f;
                b[2] = 167.0f;
                Vec_Copy(d, b);
                Vec_Normalize(d);
                Vec_Scale(d, d, -10.0f - 70.0f * Rand_Unit());
                b[0] += d[0];
                b[1] += d[1];
                b[2] += d[2];
                b[0] += 10.0f * Rand_Signed();
                b[1] += 10.0f * Rand_Signed();
                b[2] += 10.0f * Rand_Signed();
                Vec_RotateSelf(b, w->m);
                Mtx_GetTrans(o->pos, w->m);
                p->node->pos[0] = o->pos[0] + b[0];
                p->node->pos[1] = o->pos[1] + b[1];
                p->node->pos[2] = o->pos[2] + b[2];
            } else {
                p->node = NULL;
            }
        }
        break;
    }
}

s32 func_0088C890(Em *em)
{
    u8 k = 0;

    switch (em->motionId[0]) {
    case 0x3E9:
    case 0x3EA:
    case 0x3EB:
    case 0x3ED:
    case 0x3EE:
    case 0x3EF:
    case 0x3F0:
    case 0x3F7:
    case 0x3F8:
    case 0x3FB:
    case 0x3FD:
    case 0x3FF:
    case 0x401:
    case 0x40E:
    case 0x415:
    case 0x41B:
    case 0x41C:
    case 0x41D:
    case 0x41E:
    case 0x420:
        k = 1;
        break;
    }
    if ((func_00761C70(em) == 1 && k == 1)
        || (em->rage == 0 && em->motionId[0] == 0x40F && !EmMotion_ReachedFrame(em, 0, 70.0f))
        || (em->rage == 1 && k == 1)) {
        return 1;
    }
    return 0;
}

void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Color_EvalKeys(s32 *a, s32 t, u32 *c);
void Sound_PlayPlayerSe(void *obj, s32 id, s32 a, void *pos, s32 b, s32 c);
void Player_PlaySe(Player *pl, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 func_0088C890(Em *em);
extern u32 StageObj_Gun_ScaleKeys2[], StageObj_Gun_ColorKeys2[], StageObj_Gun_ScaleKeys9[], StageObj_Gun_ColorKeys9[], StageObj_Gun_ScaleKeys10[], StageObj_Gun_ColorKeys10[];
extern u32 gStageObjGun_Lifetimes[], StageObj_Gun_ScaleKeys0[], StageObj_Gun_ColorKeys0[], StageObj_Gun_ScaleKeys1[], StageObj_Gun_ColorKeys1[], StageObj_Gun_ScaleKeys5[];
extern u32 StageObj_Gun_ColorKeys5[], StageObj_Gun_ScaleKeys7[], StageObj_Gun_ColorKeys7[], StageObj_Gun_End1[];
extern u32 StageObj_Gun_SizeScaleKeys1[], StageObj_Gun_SizeColorKeys1[], StageObj_Gun_SizeScaleKeys2[], StageObj_Gun_SizeColorKeys2[], StageObj_Gun_SizeScaleKeys3[], StageObj_Gun_SizeColorKeys3[];
extern u32 StageObj_Gun_ScaleKeys6[], StageObj_Gun_ColorKeys6[];

void StageObj_Gun_Run(StageObj *o)
{
    Obj1FPart *q;
    Obj1FWork *w;
    u8 flag;
    Player *pl;
    u8 i;
    u8 j;
    s8 n;
    void *sc;
    void *cc;
    f32 f;
    f32 b[3];
    f32 a[3];
    f32 d[3];

    w = (Obj1FWork *) o->buf;
    flag = 0;
    q = w->part;
    switch (o->type) {
    case 0:
        if (o->timer++ >= 6) {
            o->state++;
            o->timer = 0;
            return;
        }
        b[0] = 105.0f;
        b[1] = 15.0f;
        b[2] = 167.0f;
        Vec_RotateSelf(b, w->m);
        Mtx_GetTrans(o->pos, w->m);
        for (i = 0; i < w->count; i++, q++) {
            if (q->node != NULL) {
                q->node->pos[0] = o->pos[0] + b[0];
                q->node->pos[1] = o->pos[1] + b[1];
                q->node->pos[2] = o->pos[2] + b[2];
                OT_AddByDepth(gDrawOtSorted, q->node, 0x40, 0);
            }
        }
        break;
    case 1:
        switch (o->var2) {
        case 0:
            n = 0x26;
            break;
        case 1:
            n = 0x2E;
            break;
        case 2:
            n = 0x30;
            break;
        case 3:
            n = 0x32;
            break;
        }
        if (o->timer++ >= n) {
            o->state++;
            o->timer = 0;
            return;
        }
        for (i = 0; i < w->count; i++, q++) {
            if (q->node != NULL) {
                if (++q->t >= 0) {
                    switch (i) {
                    case 0:
                    case 1:
                    case 2:
                        sc = StageObj_Gun_ScaleKeys2;
                        cc = StageObj_Gun_ColorKeys2;
                        q->angle += q->unk1C;
                        q->node->pos[1] += 80.0f / n;
                        break;
                    case 3:
                    case 4:
                    case 5:
                    case 9:
                        sc = StageObj_Gun_ScaleKeys9;
                        cc = StageObj_Gun_ColorKeys9;
                        if (q->frame < 15) {
                            q->frame++;
                        }
                        a[0] = 0.0f;
                        a[1] = 0.0f;
                        a[2] = 10.0f / n;
                        Vec_RotateY(a, 0.5462881f);
                        Vec_RotateSelf(a, w->m);
                        q->node->pos[0] += a[0];
                        q->node->pos[1] += a[1];
                        q->node->pos[2] += a[2];
                        break;
                    case 6:
                    case 7:
                    case 8:
                    case 10:
                        sc = StageObj_Gun_ScaleKeys10;
                        cc = StageObj_Gun_ColorKeys10;
                        break;
                    }
                    f = q->sc[2];
                    Key_LerpVec3(sc, q->sc, q->t);
                    q->sc[0] *= q->unk18;
                    q->sc[1] *= q->unk18;
                    q->sc[2] = f;
                    Color_EvalKeys((s32 *)cc, q->t, (u32 *)&q->color);
                    OT_AddByDepth(gDrawOtSorted, q->node, 0x40, 0);
                    switch (o->var2) {
                    case 1:
                        if (i == 3 && q->sc[2] < 2.0f) {
                            q->sc[2] = 1.0f;
                        }
                        break;
                    case 2:
                        if ((i == 3 || i == 4 || i == 5) && q->sc[2] < 2.0f) {
                            q->sc[2] = 1.0f;
                        }
                        break;
                    case 3:
                        if ((i == 3 || i == 4 || i == 5 || i == 9) && q->sc[2] < 2.0f) {
                            q->sc[2] = 1.0f;
                        }
                        break;
                    }
                }
                if (q->sc[2] == 1.0f) {
                    switch (o->var2) {
                    case 1:
                        Sound_PlayPlayerSe(o->owner, 0x85, 0, q->node->pos, 1, 0);
                        q->sc[2] += 1.0f;
                        break;
                    case 2:
                        Sound_PlayPlayerSe(o->owner, 0x86, 0, q->node->pos, 1, 0);
                        q->sc[2] += 1.0f;
                        break;
                    case 3:
                        Sound_PlayPlayerSe(o->owner, 0x87, 0, q->node->pos, 1, 0);
                        q->sc[2] += 1.0f;
                        break;
                    }
                }
            }
        }
        break;
    case 2:
        w->unk1A6 += -2.5f;
        a[0] = w->unk1A5;
        a[1] = w->unk1A6;
        a[2] = 0.0f;
        Vec_RotateY(a, (6.2831855f * ((Player *) o->owner)->rot[1]) / 65536.0f);
        o->pos[0] += a[0];
        o->pos[1] += a[1];
        o->pos[2] += a[2];
        Vec_Copy(o->node->pos, o->pos);
        OT_AddByDepth(gDrawOt32, o->node, 0x40, 0);
        if (o->pos[1] <= ((Player *) o->owner)->floorY) {
            o->state++;
        }
        break;
    case 3:
        w->unk1A7 = 0;
        for (i = 0; i < 3; i++) {
            if (w->node[i] == NULL) {
                w->unk1A7++;
            } else {
                switch (w->unk1A8[i]) {
                case 0:
                    if (--w->unk1AE[i] <= 0) {
                        w->unk1A8[i]++;
                    }
                    break;
                case 1:
                    w->unk1B4[i] += -2.5f;
                    a[0] = w->unk1B1[i];
                    a[1] = w->unk1B4[i];
                    a[2] = 0.0f;
                    Vec_RotateY(a, (6.2831855f * ((Player *) o->owner)->rot[1]) / 65536.0f + (6.2831855f * (120.0f * i)) / 360.0f);
                    w->node[i]->pos[0] += a[0];
                    w->node[i]->pos[1] += a[1];
                    w->node[i]->pos[2] += a[2];
                    w->angle[i] += 0x2000;
                    if (w->node[i]->pos[1] <= ((Player *) o->owner)->floorY && w->unk1B4[i] <= 0) {
                        w->unk1AB[i]++;
                        w->unk1A8[i]++;
                        w->node[i]->pos[1] = ((Player *) o->owner)->floorY;
                    }
                    OT_AddByDepth(gDrawOt32, w->node[i], 0x40, 0);
                    break;
                case 2:
                    if (w->unk1AB[i] < 2) {
                        w->unk1B1[i] = 1.25f + 0.1f * (10.0f * Rand_Unit());
                        w->unk1B4[i] = 10.0f + 5.0f * Rand_Unit();
                        w->unk1A8[i] = 1;
                    } else {
                        w->unk1A8[i]++;
                    }
                    break;
                case 3:
                    w->unk1A7++;
                    break;
                }
            }
        }
        if (w->unk1A7 == 3) {
            o->state++;
        }
        break;
    case 4:
        if (o->timer++ >= 50 || ((Player *) o->owner)->stateGroup != 1) {
            Player_PlaySe(o->owner, 0x1E, 0, o->pos, 1, 0);
            o->state++;
            o->timer = 0;
            return;
        }
        for (i = 0; i < w->count; i++, q++) {
            if (q->node != NULL) {
                if (++q->t >= 0) {
                    switch (i) {
                    case 0:
                        q->unk18 = ((f32 *) StageObj_Gun_Param4)[q->t % 2];
                        sc = StageObj_Gun_ScaleKeys0;
                        cc = StageObj_Gun_ColorKeys0;
                        break;
                    case 1:
                        q->unk18 = ((f32 *) StageObj_Gun_Param4)[q->t % 2];
                        sc = StageObj_Gun_ScaleKeys1;
                        cc = StageObj_Gun_ColorKeys1;
                        break;
                    case 2:
                    case 3:
                    case 4:
                    case 5:
                        if (q->t > ((s8 *) gStageObjGun_Lifetimes)[i] && o->timer < 35) {
                            q->t = 0;
                            b[0] = 105.0f + 10.0f * Rand_Signed();
                            b[1] = 15.0f + 10.0f * Rand_Signed();
                            b[2] = 167.0f - 80.0f * Rand_Unit();
                            q->angle = Rand_Next(1);
                            q->unk1C = 0.5f + 65536.0f * ((s16) (s32) (15.0f * Rand_Signed()) / 15.0f) / 360.0f;
                            Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 0xE)));
                            Vec_RotateSelf(b, w->m);
                            Mtx_GetTrans(o->pos, w->m);
                            q->node->pos[0] = o->pos[0] + b[0];
                            q->node->pos[1] = o->pos[1] + b[1];
                            q->node->pos[2] = o->pos[2] + b[2];
                        }
                        q->unk18 = 1.0f;
                        sc = StageObj_Gun_ScaleKeys5;
                        cc = StageObj_Gun_ColorKeys5;
                        q->angle += q->unk1C;
                        q->node->pos[1] += 30.0f / ((s8 *) gStageObjGun_Lifetimes)[i];
                        break;
                    case 6:
                    case 7:
                        if (q->t > ((s8 *) gStageObjGun_Lifetimes)[i] && o->timer < 26) {
                            q->t = 2.0f * Rand_Unit() - 4.0f;
                            q->sc[2] = 80.0f + 20.0f * Rand_Unit();
                            q->angle = Rand_Next(1);
                            b[0] = 105.0f;
                            b[1] = 15.0f;
                            b[2] = 167.0f;
                            Vec_RotateSelf(b, w->m);
                            Mtx_GetTrans(o->pos, w->m);
                            q->node->pos[0] = o->pos[0] + b[0];
                            q->node->pos[1] = o->pos[1] + b[1];
                            q->node->pos[2] = o->pos[2] + b[2];
                        }
                        q->unk18 = 1.0f;
                        f = q->sc[2];
                        sc = StageObj_Gun_ScaleKeys7;
                        cc = StageObj_Gun_ColorKeys7;
                        a[0] = 0.0f;
                        a[1] = 0.0f;
                        a[2] = f / ((s8 *) gStageObjGun_Lifetimes)[i];
                        Vec_RotateY(a, 0.5462881f);
                        Vec_RotateSelf(a, w->m);
                        q->node->pos[0] += a[0];
                        q->node->pos[1] += a[1];
                        q->node->pos[2] += a[2];
                        break;
                    }
                    Key_LerpVec3(sc, q->sc, q->t);
                    q->sc[0] *= q->unk18;
                    q->sc[1] *= q->unk18;
                    q->sc[2] *= q->unk18;
                    Color_EvalKeys((s32 *)cc, q->t, (u32 *)&q->color);
                    if (i < 2) {
                        b[0] = 128.0f;
                        b[1] = 25.0f;
                        b[2] = 201.0f;
                        Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 0xE)));
                        Vec_RotateSelf(b, w->m);
                        Mtx_GetTrans(q->node->pos, w->m);
                        q->node->pos[0] += b[0];
                        q->node->pos[1] += b[1];
                        q->node->pos[2] += b[2];
                    }
                    if (i == 6 || i == 7) {
                        q->sc[2] = f;
                    }
                    OT_AddByDepth(gDrawOtSorted, q->node, 0x40, 0);
                }
            }
        }
        break;
    case 5:
        pl = o->owner;
        if (pl->active == 0) {
            o->state++;
            o->timer = 0;
            return;
        }
        if (!Player_InCurrentArea(pl)) {
            return;
        }
        j = 0;
        if (func_0088C890((Em *) pl) == 1) {
            if (func_00761C70((Em *) pl) == 1) {
                w->unk1B7 = 1;
            } else {
                w->unk1B7 = 0;
            }
            q = w->part;
            for (i = 0; i < w->count; i++, q++) {
                if (q->node != NULL) {
                    q->frame = 0;
                }
            }
        } else {
            flag = 1;
            for (i = 0; i < w->count; i++, q++) {
                if (q->node == NULL) {
                    if (j++ >= w->count) {
                        return;
                    }
                    continue;
                }
                if (q->t >= ((s8 *) StageObj_Gun_End1)[i] || q->t < 0) {
                    q->frame = 1;
                }
                if (q->frame == 1) {
                    switch (i) {
                    case 0:
                    case 1:
                        q->t = ((s8 *) StageObj_Gun_Life1)[i];
                        break;
                    case 2:
                        q->t = -1.0f * (10.0f + 10.0f * Rand_Unit());
                        break;
                    case 3:
                        q->t = -1.0f * (5.0f + 5.0f * Rand_Signed());
                        break;
                    }
                    b[0] = 0.0f;
                    b[1] = -25.0f;
                    b[2] = 80.0f;
                    Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 0x1F)));
                    Vec_RotateSelf(b, w->m);
                    Mtx_GetTrans(o->pos, w->m);
                    q->node->pos[0] = o->pos[0] + b[0];
                    q->node->pos[1] = o->pos[1] + b[1];
                    q->node->pos[2] = o->pos[2] + b[2];
                    if (j++ >= w->count) {
                        return;
                    }
                }
            }
        }
        if (++o->timer % 30 == 0) {
            if (w->unk1B7 == 1 && flag == 0) {
                Em_PlaySeChecked((struct Em *)pl, 0x2F, 0, o->pos, 1, 0);
            }
            o->timer = 0;
        }
        q = w->part;
        for (i = 0; i < w->count; i++, q++) {
            if (q->frame != 1 && q->node != NULL) {
                if (++q->t >= 0) {
                    switch (i) {
                    case 0:
                    case 1:
                        if (q->t > ((s8 *) StageObj_Gun_End1)[i]) {
                            q->unk18 = 1.0f + Rand_Unit();
                            q->angle = Rand_Next(1);
                            b[0] = ((f32 *) StageObj_Gun_Jitter1)[w->unk1B7] * Rand_Signed();
                            b[1] = -25.0f + ((f32 *) StageObj_Gun_Jitter1)[w->unk1B7] * Rand_Signed();
                            b[2] = 80.0f + ((f32 *) StageObj_Gun_Jitter1)[w->unk1B7] * Rand_Signed();
                            q->sc[2] = ((f32 *) StageObj_Gun_ScaleZ1)[w->unk1B7] * Rand_Unit() + 2.0f * ((f32 *) StageObj_Gun_ScaleZ1)[w->unk1B7];
                        }
                        f = q->sc[2];
                        sc = ((void **) StageObj_Gun_SizeScaleKeys1)[w->unk1B7];
                        cc = ((void **) StageObj_Gun_SizeColorKeys1)[w->unk1B7];
                        a[0] = 0.0f;
                        a[1] = 0.0f;
                        a[2] = f / ((s8 *) StageObj_Gun_End1)[i];
                        break;
                    case 2:
                        if (q->t > ((s8 *) StageObj_Gun_End1)[i]) {
                            q->unk18 = 0.01f * (100.0f + 20.0f * Rand_Unit());
                            b[0] = ((f32 (*)[3]) StageObj_Gun_Jitter2)[w->unk1B7][0] * Rand_Signed();
                            b[1] = -25.0f + ((f32 (*)[3]) StageObj_Gun_Jitter2)[w->unk1B7][1] * Rand_Signed();
                            b[2] = 80.0f + ((f32 (*)[3]) StageObj_Gun_Jitter2)[w->unk1B7][2] * Rand_Signed();
                            q->sc[2] = 100.0f + 20.0f * Rand_Unit();
                        }
                        f = q->sc[2];
                        sc = ((void **) StageObj_Gun_SizeScaleKeys2)[w->unk1B7];
                        cc = ((void **) StageObj_Gun_SizeColorKeys2)[w->unk1B7];
                        a[0] = 0.0f;
                        a[1] = (-1.0f * f) / ((s8 *) StageObj_Gun_End1)[i];
                        a[2] = 0.0f;
                        break;
                    case 3:
                        if (q->t > ((s8 *) StageObj_Gun_End1)[i]) {
                            b[0] = 0.0f;
                            b[1] = -25.0f;
                            b[2] = 80.0f;
                            q->sc[2] = ((f32 *) StageObj_Gun_ScaleZ3)[w->unk1B7] * Rand_Unit() + 4.0f * ((f32 *) StageObj_Gun_ScaleZ3)[w->unk1B7];
                            q->unk1C = 0;
                        }
                        f = q->sc[2];
                        sc = ((void **) StageObj_Gun_SizeScaleKeys3)[w->unk1B7];
                        cc = ((void **) StageObj_Gun_SizeColorKeys3)[w->unk1B7];
                        q->unk1C -= 5;
                        a[0] = 0.0f;
                        a[1] = q->unk1C;
                        a[2] = f / ((s8 *) StageObj_Gun_End1)[i];
                        break;
                    }
                    if (q->t > ((s8 *) StageObj_Gun_End1)[i]) {
                        Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 0x1F)));
                        Vec_RotateSelf(b, w->m);
                        Mtx_GetTrans(o->pos, w->m);
                        q->node->pos[0] = o->pos[0] + b[0];
                        q->node->pos[1] = o->pos[1] + b[1];
                        q->node->pos[2] = o->pos[2] + b[2];
                        switch (i) {
                        case 0:
                        case 1:
                            q->t = 0;
                            break;
                        case 2:
                            q->t = -1.0f * (10.0f + 10.0f * Rand_Unit());
                            break;
                        case 3:
                            q->t = -1.0f * (5.0f + 5.0f * Rand_Unit());
                            break;
                        }
                    }
                    Vec_RotateSelf(a, w->m);
                    q->node->pos[0] += a[0];
                    q->node->pos[1] += a[1];
                    q->node->pos[2] += a[2];
                    Key_LerpVec3(sc, q->sc, q->t);
                    q->sc[0] *= q->unk18;
                    q->sc[1] *= q->unk18;
                    q->sc[2] *= q->unk18;
                    Color_EvalKeys((s32 *)cc, q->t, (u32 *)&q->color);
                    q->sc[2] = f;
                    OT_AddByDepth(gDrawOtSorted, q->node, 0x40, 0);
                }
            }
        }
        break;
    case 6:
        if (o->timer++ >= 0x555) {
            o->state++;
            o->timer = 0;
            return;
        }
        if (((Player *) o->owner)->weaponOut != 0) {
            for (i = 0; i < w->count; i++, q++) {
                if (q->node != NULL) {
                    if (++q->t > 0) {
                        if (o->timer < 0x546 && q->t > 15) {
                            q->t = -5;
                        }
                        q->unk18 = 1.0f;
                        q->angle += q->unk1C;
                        q->node->pos[1] += 2.0f;
                        Key_LerpVec3((f32 *)StageObj_Gun_ScaleKeys6, q->sc, q->t);
                        q->sc[0] *= q->unk18;
                        q->sc[1] *= q->unk18;
                        q->sc[2] *= q->unk18;
                        Color_EvalKeys((s32 *)StageObj_Gun_ColorKeys6, q->t, (u32 *)&q->color);
                        OT_AddByDepth(gDrawOtSorted, q->node, 0x40, 0);
                    } else if (q->t <= 0) {
                        b[0] = 105.0f;
                        b[1] = 15.0f;
                        b[2] = 167.0f;
                        Vec_Copy(d, b);
                        Vec_Normalize(d);
                        Vec_Scale(d, d, -10.0f - 70.0f * Rand_Unit());
                        b[0] += d[0];
                        b[1] += d[1];
                        b[2] += d[2];
                        b[0] += 10.0f * Rand_Signed();
                        b[1] += 10.0f * Rand_Signed();
                        b[2] += 10.0f * Rand_Signed();
                        q->angle = Rand_Next(1);
                        q->unk1C = 0.5f + 65536.0f * ((s16) (s32) (15.0f * Rand_Signed()) / 15.0f) / 360.0f;
                        Mtx_Copy((f32 *) w->m, (f32 *) (Em_GetBone(o->owner, 0xE)));
                        Vec_RotateSelf(b, w->m);
                        Mtx_GetTrans(o->pos, w->m);
                        q->node->pos[0] = o->pos[0] + b[0];
                        q->node->pos[1] = o->pos[1] + b[1];
                        q->node->pos[2] = o->pos[2] + b[2];
                    }
                }
            }
        }
        break;
    }
}

/* unk_880D50's .data (0x8D53B0-0x8D8B90) */
f32 StageObj_K1B_SpinDeg[6] = {
#include "game/008D53B0.inc"
};
u8 StageObj_K1B_PartFalls[8] = {
#include "game/008D53C8.inc"
};
u32 StageObj_K1B_AlphaKeys_0[8] = {
#include "game/008D53D0.inc"
};
u32 StageObj_K1B_AlphaKeys_1[8] = {
#include "game/008D53F0.inc"
};
u32 StageObj_K1B_AlphaKeys_2[12] = {
#include "game/008D5410.inc"
};
u32 StageObj_K1B_AlphaKeys_3[8] = {
#include "game/008D5440.inc"
};
u32 StageObj_K1B_AlphaKeys_4[8] = {
#include "game/008D5460.inc"
};
void *StageObj_K1B_AlphaKeys[8] = {
    (void *)StageObj_K1B_AlphaKeys_0, (void *)StageObj_K1B_AlphaKeys_1, (void *)StageObj_K1B_AlphaKeys_2, (void *)StageObj_K1B_AlphaKeys_3,
    (void *)StageObj_K1B_AlphaKeys_4, NULL, NULL, NULL,
};
u32 StageObj_K1B_ScaleKeys_1[16] = {
#include "game/008D54A0.inc"
};
u32 StageObj_K1B_ScaleKeys_2[16] = {
#include "game/008D54E0.inc"
};
u32 StageObj_K1B_ScaleKeys_3[16] = {
#include "game/008D5520.inc"
};
void *StageObj_K1B_ScaleKeys[8] = {
    NULL, (void *)StageObj_K1B_ScaleKeys_1, (void *)StageObj_K1B_ScaleKeys_2, (void *)StageObj_K1B_ScaleKeys_3,
    NULL, NULL, NULL, NULL,
};
u8 StageObj_K1B_Type1ScaleKeys[48] = {
#include "game/008D5580.inc"
};
f32 gStageObjK1C_ScaleCurves_0[12] = {
#include "game/008D55B0.inc"
};
u8 gStageObjK1C_ColorKeys_0[48] = {
#include "game/008D55E0.inc"
};
f32 gStageObjK1C_ScaleCurves_1[8] = {
#include "game/008D5610.inc"
};
f32 gStageObjK1C_ScaleCurves_3[12] = {
#include "game/008D5630.inc"
};
f32 gStageObjK1C_ScaleCurves_5[16] = {
#include "game/008D5660.inc"
};
f32 gStageObjK1C_ScaleCurves_7[12] = {
#include "game/008D56A0.inc"
};
f32 gStageObjK1C_ScaleCurves_9[16] = {
#include "game/008D56D0.inc"
};
f32 gStageObjK1C_ScaleCurves_2[8] = {
#include "game/008D5710.inc"
};
f32 gStageObjK1C_ScaleCurves_4[8] = {
#include "game/008D5730.inc"
};
u8 gStageObjK1C_ColorKeys_1[32] = {
#include "game/008D5750.inc"
};
u8 gStageObjK1C_ColorKeys_3[32] = {
#include "game/008D5770.inc"
};
u8 gStageObjK1C_ColorKeys_5[40] = {
#include "game/008D5790.inc"
};
u32 gStageObjK1C_SeqTbls_1[3] = {
#include "game/008D57B8.inc"
};
u32 StageObj_K1C_T1Delay[8] = {
#include "game/008D57D0.inc"
};
u32 StageObj_K1C_T1Rise[12] = {
#include "game/008D57F0.inc"
};
u32 StageObj_K1C_T1Scale[10] = {
#include "game/008D5820.inc"
};
u32 gStageObjK1C_DrawTbls_1[3] = {
#include "game/008D5848.inc"
};
f32 gStageObjK1C_ScaleCurves_12[16] = {
#include "game/008D5860.inc"
};
f32 gStageObjK1C_ScaleCurves_15[12] = {
#include "game/008D58A0.inc"
};
f32 gStageObjK1C_ScaleCurves_16[12] = {
#include "game/008D58D0.inc"
};
u8 gStageObjK1C_ColorKeys_8[48] = {
#include "game/008D5900.inc"
};
u8 gStageObjK1C_ColorKeys_11[48] = {
#include "game/008D5930.inc"
};
u8 gStageObjK1C_ColorKeys_12[40] = {
#include "game/008D5960.inc"
};
u32 gStageObjK1C_SeqTbls_2[2] = {
#include "game/008D5988.inc"
};
u32 StageObj_K1C_T2Delay[4] = {
#include "game/008D5990.inc"
};
u32 gStageObjK1C_DrawTbls_2[4] = {
#include "game/008D59A0.inc"
};
f32 gStageObjK1C_ScaleCurves_17[16] = {
#include "game/008D59B0.inc"
};
f32 gStageObjK1C_ScaleCurves_18[12] = {
#include "game/008D59F0.inc"
};
u8 gStageObjK1C_ColorKeys_13[32] = {
#include "game/008D5A20.inc"
};
u8 gStageObjK1C_ColorKeys_14[24] = {
#include "game/008D5A40.inc"
};
u32 gStageObjK1C_DrawTbls_3[2] = {
#include "game/008D5A58.inc"
};
f32 gStageObjK1C_ScaleCurves_19[12] = {
#include "game/008D5A60.inc"
};
u8 gStageObjK1C_ColorKeys_15[40] = {
#include "game/008D5A90.inc"
};
u32 gStageObjK1C_DrawTbls_4[2] = {
#include "game/008D5AB8.inc"
};
f32 gStageObjK1C_ScaleCurves_20[20] = {
#include "game/008D5AC0.inc"
};
f32 gStageObjK1C_ScaleCurves_21[12] = {
#include "game/008D5B10.inc"
};
u8 gStageObjK1C_ColorKeys_16[32] = {
#include "game/008D5B40.inc"
};
u8 gStageObjK1C_ColorKeys_17[40] = {
#include "game/008D5B60.inc"
};
u32 StageObj_K1C_T6Index[2] = {
#include "game/008D5B88.inc"
};
u32 StageObj_K1C_T6Rise[4] = {
#include "game/008D5B90.inc"
};
u32 gStageObjK1C_SeqTbls_5[2] = {
#include "game/008D5BA0.inc"
};
u32 gStageObjK1C_DrawTbls_5[2] = {
#include "game/008D5BA8.inc"
};
u32 gStageObjK1C_DrawTbls_6[4] = {
#include "game/008D5BB0.inc"
};
f32 gStageObjK1C_ScaleCurves_24[12] = {
#include "game/008D5BC0.inc"
};
f32 gStageObjK1C_ScaleCurves_25[8] = {
#include "game/008D5BF0.inc"
};
u8 gStageObjK1C_ColorKeys_20[32] = {
#include "game/008D5C10.inc"
};
u8 gStageObjK1C_ColorKeys_21[32] = {
#include "game/008D5C30.inc"
};
f32 gStageObjK1C_ScaleCurves_27[12] = {
#include "game/008D5C50.inc"
};
f32 gStageObjK1C_ScaleCurves_28[8] = {
#include "game/008D5C80.inc"
};
u8 gStageObjK1C_ColorKeys_23[32] = {
#include "game/008D5CA0.inc"
};
u8 gStageObjK1C_ColorKeys_24[24] = {
#include "game/008D5CC0.inc"
};
u32 gStageObjK1C_DrawTbls_7[2] = {
#include "game/008D5CD8.inc"
};
u32 gStageObjK1C_SeqTbls_7[4] = {
#include "game/008D5CE0.inc"
};
f32 gStageObjK1C_ScaleCurves_30[16] = {
#include "game/008D5CF0.inc"
};
f32 gStageObjK1C_ScaleCurves_31[8] = {
#include "game/008D5D30.inc"
};
f32 gStageObjK1C_ScaleCurves_32[8] = {
#include "game/008D5D50.inc"
};
u8 gStageObjK1C_ColorKeys_26[32] = {
#include "game/008D5D70.inc"
};
u8 gStageObjK1C_ColorKeys_27[32] = {
#include "game/008D5D90.inc"
};
u8 gStageObjK1C_ColorKeys_28[40] = {
#include "game/008D5DB0.inc"
};
u32 gStageObjK1C_DrawTbls_8[2] = {
#include "game/008D5DD8.inc"
};
u32 gStageObjK1C_SeqTbls_8[2] = {
#include "game/008D5DE0.inc"
};
u32 StageObj_K1C_T8Depth[2] = {
#include "game/008D5DE8.inc"
};
f32 gStageObjK1C_ScaleCurves_34[8] = {
#include "game/008D5DF0.inc"
};
f32 gStageObjK1C_ScaleCurves_35[12] = {
#include "game/008D5E10.inc"
};
f32 gStageObjK1C_ScaleCurves_38[8] = {
#include "game/008D5E40.inc"
};
u8 gStageObjK1C_ColorKeys_30[48] = {
#include "game/008D5E60.inc"
};
u8 gStageObjK1C_ColorKeys_31[48] = {
#include "game/008D5E90.inc"
};
u8 gStageObjK1C_ColorKeys_34[24] = {
#include "game/008D5EC0.inc"
};
u32 gStageObjK1C_DrawTbls_9[2] = {
#include "game/008D5ED8.inc"
};
u32 gStageObjK1C_SeqTbls_9[4] = {
#include "game/008D5EE0.inc"
};
f32 gStageObjK1C_ScaleCurves_39[16] = {
#include "game/008D5EF0.inc"
};
f32 gStageObjK1C_ScaleCurves_40[8] = {
#include "game/008D5F30.inc"
};
f32 gStageObjK1C_ScaleCurves_41[12] = {
#include "game/008D5F50.inc"
};
u8 gStageObjK1C_ColorKeys_35[32] = {
#include "game/008D5F80.inc"
};
u8 gStageObjK1C_ColorKeys_36[32] = {
#include "game/008D5FA0.inc"
};
u8 gStageObjK1C_ColorKeys_37[32] = {
#include "game/008D5FC0.inc"
};
u32 gStageObjK1C_DrawTbls_10[2] = {
#include "game/008D5FE0.inc"
};
u32 gStageObjK1C_SeqTbls_10[2] = {
#include "game/008D5FE8.inc"
};
u32 StageObj_K1C_T10Delay[4] = {
#include "game/008D5FF0.inc"
};
f32 gStageObjK1C_ScaleCurves_43[12] = {
#include "game/008D6000.inc"
};
f32 gStageObjK1C_ScaleCurves_44[16] = {
#include "game/008D6030.inc"
};
f32 gStageObjK1C_ScaleCurves_45[12] = {
#include "game/008D6070.inc"
};
u8 gStageObjK1C_ColorKeys_39[32] = {
#include "game/008D60A0.inc"
};
u8 gStageObjK1C_ColorKeys_42[32] = {
#include "game/008D60C0.inc"
};
u8 gStageObjK1C_ColorKeys_40[32] = {
#include "game/008D60E0.inc"
};
u8 gStageObjK1C_ColorKeys_41[40] = {
#include "game/008D6100.inc"
};
u32 gStageObjK1C_DrawTbls_12[2] = {
#include "game/008D6128.inc"
};
u16 StageObj_K1C_StepAFrames[16] = {
#include "game/008D6130.inc"
};
u16 StageObj_K1C_StepAStart[16] = {
#include "game/008D6150.inc"
};
f32 gStageObjK1C_ScaleCurves_46[12] = {
#include "game/008D6170.inc"
};
f32 gStageObjK1C_ScaleCurves_47[8] = {
#include "game/008D61A0.inc"
};
u8 gStageObjK1C_ColorKeys_43[32] = {
#include "game/008D61C0.inc"
};
u8 gStageObjK1C_ColorKeys_44[32] = {
#include "game/008D61E0.inc"
};
f32 gStageObjK1C_ScaleCurves_49[12] = {
#include "game/008D6200.inc"
};
f32 gStageObjK1C_ScaleCurves_50[8] = {
#include "game/008D6230.inc"
};
u8 gStageObjK1C_ColorKeys_46[32] = {
#include "game/008D6250.inc"
};
u8 gStageObjK1C_ColorKeys_47[32] = {
#include "game/008D6270.inc"
};
f32 gStageObjK1C_ScaleCurves_52[12] = {
#include "game/008D6290.inc"
};
f32 gStageObjK1C_ScaleCurves_53[8] = {
#include "game/008D62C0.inc"
};
u8 gStageObjK1C_ColorKeys_49[32] = {
#include "game/008D62E0.inc"
};
u8 gStageObjK1C_ColorKeys_50[32] = {
#include "game/008D6300.inc"
};
u32 gStageObjK1C_DrawTbls_13[2] = {
#include "game/008D6320.inc"
};
u32 gStageObjK1C_SeqTbls_13[2] = {
#include "game/008D6328.inc"
};
f32 gStageObjK1C_ScaleCurves_55[12] = {
#include "game/008D6330.inc"
};
f32 gStageObjK1C_ScaleCurves_56[8] = {
#include "game/008D6360.inc"
};
u8 gStageObjK1C_ColorKeys_52[32] = {
#include "game/008D6380.inc"
};
u8 gStageObjK1C_ColorKeys_53[32] = {
#include "game/008D63A0.inc"
};
f32 gStageObjK1C_ScaleCurves_58[8] = {
#include "game/008D63C0.inc"
};
f32 gStageObjK1C_ScaleCurves_59[12] = {
#include "game/008D63E0.inc"
};
f32 gStageObjK1C_ScaleCurves_60[12] = {
#include "game/008D6410.inc"
};
u8 gStageObjK1C_ColorKeys_55[32] = {
#include "game/008D6440.inc"
};
u8 gStageObjK1C_ColorKeys_56[32] = {
#include "game/008D6460.inc"
};
u8 gStageObjK1C_ColorKeys_57[24] = {
#include "game/008D6480.inc"
};
u32 gStageObjK1C_DrawTbls_14[2] = {
#include "game/008D6498.inc"
};
u32 gStageObjK1C_SeqTbls_14[4] = {
#include "game/008D64A0.inc"
};
f32 gStageObjK1C_ScaleCurves_62[8] = {
#include "game/008D64B0.inc"
};
f32 gStageObjK1C_ScaleCurves_63[16] = {
#include "game/008D64D0.inc"
};
f32 gStageObjK1C_ScaleCurves_64[12] = {
#include "game/008D6510.inc"
};
f32 gStageObjK1C_ScaleCurves_65[12] = {
#include "game/008D6540.inc"
};
f32 gStageObjK1C_ScaleCurves_66[12] = {
#include "game/008D6570.inc"
};
f32 gStageObjK1C_ScaleCurves_68[12] = {
#include "game/008D65A0.inc"
};
f32 gStageObjK1C_ScaleCurves_70[16] = {
#include "game/008D65D0.inc"
};
u8 gStageObjK1C_ColorKeys_59[32] = {
#include "game/008D6610.inc"
};
u8 gStageObjK1C_ColorKeys_60[32] = {
#include "game/008D6630.inc"
};
u8 gStageObjK1C_ColorKeys_61[32] = {
#include "game/008D6650.inc"
};
u8 gStageObjK1C_ColorKeys_62[32] = {
#include "game/008D6670.inc"
};
u8 gStageObjK1C_ColorKeys_63[32] = {
#include "game/008D6690.inc"
};
u8 gStageObjK1C_ColorKeys_65[48] = {
#include "game/008D66B0.inc"
};
u8 gStageObjK1C_ColorKeys_67[32] = {
#include "game/008D66E0.inc"
};
u32 gStageObjK1C_DrawTbls_15[8] = {
#include "game/008D6700.inc"
};
s16 StageObj_K1C_StepCFrames[24] = {
#include "game/008D6720.inc"
};
u16 StageObj_K1C_StepCWaits[16] = {
#include "game/008D6750.inc"
};
f32 gStageObjK1C_ScaleCurves_71[16] = {
#include "game/008D6770.inc"
};
f32 gStageObjK1C_ScaleCurves_72[12] = {
#include "game/008D67B0.inc"
};
u8 gStageObjK1C_ColorKeys_68[32] = {
#include "game/008D67E0.inc"
};
u8 gStageObjK1C_ColorKeys_69[40] = {
#include "game/008D6800.inc"
};
u32 gStageObjK1C_DrawTbls_16[2] = {
#include "game/008D6828.inc"
};
u32 StageObj_K1C_T0Frames[2] = {
#include "game/008D6830.inc"
};
u8 StageObj_K1C_StepCKinds17[12] = {
#include "game/008D6838.inc"
};
u16 StageObj_K1C_StepCWaits17[16] = {
#include "game/008D6850.inc"
};
f32 gStageObjK1C_ScaleCurves_73[16] = {
#include "game/008D6870.inc"
};
f32 gStageObjK1C_ScaleCurves_74[12] = {
#include "game/008D68B0.inc"
};
u8 gStageObjK1C_ColorKeys_70[32] = {
#include "game/008D68E0.inc"
};
u8 gStageObjK1C_ColorKeys_71[40] = {
#include "game/008D6900.inc"
};
u32 gStageObjK1C_DrawTbls_19[2] = {
#include "game/008D6928.inc"
};
u32 gStageObjK1C_SeqTbls_19[2] = {
#include "game/008D6930.inc"
};
u8 StageObj_K1C_StepDKinds[12] = {
#include "game/008D6938.inc"
};
u16 StageObj_K1C_StepDWaits[16] = {
#include "game/008D6950.inc"
};
f32 gStageObjK1C_ScaleCurves_76[12] = {
#include "game/008D6970.inc"
};
u8 gStageObjK1C_ColorKeys_73[40] = {
#include "game/008D69A0.inc"
};
u32 gStageObjK1C_DrawTbls_20[2] = {
#include "game/008D69C8.inc"
};
u32 gStageObjK1C_SeqTbls_20[4] = {
#include "game/008D69D0.inc"
};
f32 gStageObjK1C_ScaleCurves_77[8] = {
#include "game/008D69E0.inc"
};
u8 gStageObjK1C_ColorKeys_74[32] = {
#include "game/008D6A00.inc"
};
u32 gStageObjK1C_DrawTbls_22[2] = {
#include "game/008D6A20.inc"
};
u32 gStageObjK1C_SeqTbls_22[2] = {
#include "game/008D6A28.inc"
};
f32 gStageObjK1C_ScaleCurves_78[12] = {
#include "game/008D6A30.inc"
};
u8 gStageObjK1C_ColorKeys_75[32] = {
#include "game/008D6A60.inc"
};
u8 gStageObjK1C_ColorKeys_76[32] = {
#include "game/008D6A80.inc"
};
u8 gStageObjK1C_ColorKeys_77[32] = {
#include "game/008D6AA0.inc"
};
u8 gStageObjK1C_ColorKeys_78[24] = {
#include "game/008D6AC0.inc"
};
u32 gStageObjK1C_DrawTbls_23[2] = {
#include "game/008D6AD8.inc"
};
u32 gStageObjK1C_SeqTbls_23[4] = {
#include "game/008D6AE0.inc"
};
f32 gStageObjK1C_ScaleCurves_79[16] = {
#include "game/008D6AF0.inc"
};
f32 gStageObjK1C_ScaleCurves_80[12] = {
#include "game/008D6B30.inc"
};
u8 gStageObjK1C_ColorKeys_79[32] = {
#include "game/008D6B60.inc"
};
u8 gStageObjK1C_ColorKeys_80[32] = {
#include "game/008D6B80.inc"
};
u32 StageObj_K1C_T24Index[2] = {
#include "game/008D6BA0.inc"
};
u32 gStageObjK1C_DrawTbls_24[2] = {
#include "game/008D6BA8.inc"
};
u32 gStageObjK1C_SeqTbls_24[4] = {
#include "game/008D6BB0.inc"
};
f32 gStageObjK1C_ScaleCurves_81[12] = {
#include "game/008D6BC0.inc"
};
f32 gStageObjK1C_ScaleCurves_82[12] = {
#include "game/008D6BF0.inc"
};
u8 gStageObjK1C_ColorKeys_81[32] = {
#include "game/008D6C20.inc"
};
u8 gStageObjK1C_ColorKeys_82[32] = {
#include "game/008D6C40.inc"
};
u32 gStageObjK1C_DrawTbls_25[2] = {
#include "game/008D6C60.inc"
};
u32 gStageObjK1C_SeqTbls_25[2] = {
#include "game/008D6C68.inc"
};
f32 gStageObjK1C_ScaleCurves_83[8] = {
#include "game/008D6C70.inc"
};
f32 gStageObjK1C_ScaleCurves_84[12] = {
#include "game/008D6C90.inc"
};
f32 gStageObjK1C_ScaleCurves_85[16] = {
#include "game/008D6CC0.inc"
};
u8 gStageObjK1C_ColorKeys_83[32] = {
#include "game/008D6D00.inc"
};
u8 gStageObjK1C_ColorKeys_84[32] = {
#include "game/008D6D20.inc"
};
u8 gStageObjK1C_ColorKeys_85[24] = {
#include "game/008D6D40.inc"
};
u32 gStageObjK1C_DrawTbls_26[2] = {
#include "game/008D6D58.inc"
};
u32 gStageObjK1C_SeqTbls_26[2] = {
#include "game/008D6D60.inc"
};
u32 StageObj_K1C_T26Delay[2] = {
#include "game/008D6D68.inc"
};
f32 gStageObjK1C_ScaleCurves_86[16] = {
#include "game/008D6D70.inc"
};
u8 gStageObjK1C_ColorKeys_86[24] = {
#include "game/008D6DB0.inc"
};
u32 gStageObjK1C_DrawTbls_27[2] = {
#include "game/008D6DC8.inc"
};
u32 gStageObjK1C_SeqTbls_27[4] = {
#include "game/008D6DD0.inc"
};
f32 gStageObjK1C_ScaleCurves_87[12] = {
#include "game/008D6DE0.inc"
};
u8 gStageObjK1C_ColorKeys_87[32] = {
#include "game/008D6E10.inc"
};
u32 gStageObjK1C_DrawTbls_28[2] = {
#include "game/008D6E30.inc"
};
u32 gStageObjK1C_SeqTbls_28[2] = {
#include "game/008D6E38.inc"
};
f32 gStageObjK1C_ScaleCurves_90[12] = {
#include "game/008D6E40.inc"
};
f32 gStageObjK1C_ScaleCurves_91[16] = {
#include "game/008D6E70.inc"
};
f32 gStageObjK1C_ScaleCurves_92[12] = {
#include "game/008D6EB0.inc"
};
f32 gStageObjK1C_ScaleCurves_93[12] = {
#include "game/008D6EE0.inc"
};
u8 gStageObjK1C_ColorKeys_90[32] = {
#include "game/008D6F10.inc"
};
u8 gStageObjK1C_ColorKeys_91[32] = {
#include "game/008D6F30.inc"
};
u8 gStageObjK1C_ColorKeys_92[48] = {
#include "game/008D6F50.inc"
};
u8 gStageObjK1C_ColorKeys_93[32] = {
#include "game/008D6F80.inc"
};
u32 gStageObjK1C_DrawTbls_29[2] = {
#include "game/008D6FA0.inc"
};
u16 StageObj_K1C_StepBFrames[4] = {
#include "game/008D6FA8.inc"
};
u8 StageObj_K1C_StepBKinds[16] = {
#include "game/008D6FB0.inc"
};
u16 StageObj_K1C_StepBStart[16] = {
#include "game/008D6FC0.inc"
};
u32 gStageObjK1C_DrawTbls_30[4] = {
#include "game/008D6FE0.inc"
};
u32 StageObj_K1C_T30Delay[4] = {
#include "game/008D6FF0.inc"
};
u32 StageObj_K1C_T6Frames[4] = {
#include "game/008D7000.inc"
};
f32 gStageObjK1C_ScaleCurves_94[12] = {
#include "game/008D7010.inc"
};
f32 gStageObjK1C_ScaleCurves_96[16] = {
#include "game/008D7040.inc"
};
f32 gStageObjK1C_ScaleCurves_98[16] = {
#include "game/008D7080.inc"
};
u8 gStageObjK1C_ColorKeys_94[32] = {
#include "game/008D70C0.inc"
};
u8 gStageObjK1C_ColorKeys_96[32] = {
#include "game/008D70E0.inc"
};
u8 gStageObjK1C_ColorKeys_98[48] = {
#include "game/008D7100.inc"
};
f32 gStageObjK1C_ScaleCurves_101[8] = {
#include "game/008D7130.inc"
};
u8 gStageObjK1C_ColorKeys_101[40] = {
#include "game/008D7150.inc"
};
u32 gStageObjK1C_DrawTbls_31[2] = {
#include "game/008D7178.inc"
};
u32 gStageObjK1C_SeqTbls_31[4] = {
#include "game/008D7180.inc"
};
f32 gStageObjK1C_ScaleCurves_102[16] = {
#include "game/008D7190.inc"
};
f32 gStageObjK1C_ScaleCurves_103[16] = {
#include "game/008D71D0.inc"
};
f32 gStageObjK1C_ScaleCurves_104[12] = {
#include "game/008D7210.inc"
};
f32 gStageObjK1C_ScaleCurves_105[16] = {
#include "game/008D7240.inc"
};
u8 gStageObjK1C_ColorKeys_102[32] = {
#include "game/008D7280.inc"
};
u8 gStageObjK1C_ColorKeys_103[32] = {
#include "game/008D72A0.inc"
};
u8 gStageObjK1C_ColorKeys_104[32] = {
#include "game/008D72C0.inc"
};
u8 gStageObjK1C_ColorKeys_105[32] = {
#include "game/008D72E0.inc"
};
u32 gStageObjK1C_DrawTbls_32[2] = {
#include "game/008D7300.inc"
};
u32 StageObj_K1C_T2Frames[2] = {
#include "game/008D7308.inc"
};
u32 StageObj_K1C_T32Index[8] = {
#include "game/008D7310.inc"
};
f32 gStageObjK1C_ScaleCurves_106[12] = {
#include "game/008D7330.inc"
};
f32 gStageObjK1C_ScaleCurves_107[12] = {
#include "game/008D7360.inc"
};
f32 gStageObjK1C_ScaleCurves_108[8] = {
#include "game/008D7390.inc"
};
u8 gStageObjK1C_ColorKeys_106[48] = {
#include "game/008D73B0.inc"
};
u8 gStageObjK1C_ColorKeys_107[48] = {
#include "game/008D73E0.inc"
};
u8 gStageObjK1C_ColorKeys_108[24] = {
#include "game/008D7410.inc"
};
u32 gStageObjK1C_DrawTbls_33[2] = {
#include "game/008D7428.inc"
};
u32 StageObj_K1C_T33Frames[2] = {
#include "game/008D7430.inc"
};
u32 StageObj_K1C_T33Index[2] = {
#include "game/008D7438.inc"
};
u32 StageObj_K1C_T33Delay[4] = {
#include "game/008D7440.inc"
};
f32 gStageObjK1C_ScaleCurves_109[12] = {
#include "game/008D7450.inc"
};
u8 gStageObjK1C_ColorKeys_109[32] = {
#include "game/008D7480.inc"
};
u32 gStageObjK1C_DrawTbls_34[2] = {
#include "game/008D74A0.inc"
};
u32 gStageObjK1C_SeqTbls_34[2] = {
#include "game/008D74A8.inc"
};
f32 gStageObjK1C_ScaleCurves_110[12] = {
#include "game/008D74B0.inc"
};
u8 gStageObjK1C_ColorKeys_110[24] = {
#include "game/008D74E0.inc"
};
u32 gStageObjK1C_DrawTbls_35[2] = {
#include "game/008D74F8.inc"
};
f32 gStageObjK1C_ScaleCurves_111[12] = {
#include "game/008D7500.inc"
};
u8 gStageObjK1C_ColorKeys_111[32] = {
#include "game/008D7530.inc"
};
u32 gStageObjK1C_DrawTbls_36[2] = {
#include "game/008D7550.inc"
};
u32 gStageObjK1C_SeqTbls_36[2] = {
#include "game/008D7558.inc"
};
f32 gStageObjK1C_ScaleCurves_112[8] = {
#include "game/008D7560.inc"
};
f32 gStageObjK1C_ScaleCurves_113[12] = {
#include "game/008D7580.inc"
};
f32 gStageObjK1C_ScaleCurves_114[12] = {
#include "game/008D75B0.inc"
};
u8 gStageObjK1C_ColorKeys_112[32] = {
#include "game/008D75E0.inc"
};
u8 gStageObjK1C_ColorKeys_113[32] = {
#include "game/008D7600.inc"
};
u8 gStageObjK1C_ColorKeys_114[24] = {
#include "game/008D7620.inc"
};
u32 gStageObjK1C_DrawTbls_37[2] = {
#include "game/008D7638.inc"
};
u32 gStageObjK1C_SeqTbls_37[4] = {
#include "game/008D7640.inc"
};
f32 gStageObjK1C_ScaleCurves_115[8] = {
#include "game/008D7650.inc"
};
f32 gStageObjK1C_ScaleCurves_116[12] = {
#include "game/008D7670.inc"
};
f32 gStageObjK1C_ScaleCurves_117[12] = {
#include "game/008D76A0.inc"
};
u8 gStageObjK1C_ColorKeys_115[32] = {
#include "game/008D76D0.inc"
};
u8 gStageObjK1C_ColorKeys_116[48] = {
#include "game/008D76F0.inc"
};
u8 gStageObjK1C_ColorKeys_117[40] = {
#include "game/008D7720.inc"
};
u32 gStageObjK1C_DrawTbls_38[2] = {
#include "game/008D7748.inc"
};
u32 gStageObjK1C_SeqTbls_38[2] = {
#include "game/008D7750.inc"
};
u32 StageObj_K1C_T38Delay[2] = {
#include "game/008D7758.inc"
};
f32 gStageObjK1C_ScaleCurves_118[8] = {
#include "game/008D7760.inc"
};
u8 gStageObjK1C_ColorKeys_118[40] = {
#include "game/008D7780.inc"
};
u32 gStageObjK1C_DrawTbls_39[2] = {
#include "game/008D77A8.inc"
};
u32 gStageObjK1C_SeqTbls_39[4] = {
#include "game/008D77B0.inc"
};
f32 gStageObjK1C_ScaleCurves_120[16] = {
#include "game/008D77C0.inc"
};
u8 gStageObjK1C_ColorKeys_120[32] = {
#include "game/008D7800.inc"
};
u32 gStageObjK1C_DrawTbls_40[2] = {
#include "game/008D7820.inc"
};
u32 gStageObjK1C_SeqTbls_40[2] = {
#include "game/008D7828.inc"
};
f32 *gStageObjK1C_ScaleCurves[124] = {
    gStageObjK1C_ScaleCurves_0, gStageObjK1C_ScaleCurves_1, gStageObjK1C_ScaleCurves_2, gStageObjK1C_ScaleCurves_3,
    gStageObjK1C_ScaleCurves_4, gStageObjK1C_ScaleCurves_5, gStageObjK1C_ScaleCurves_5, gStageObjK1C_ScaleCurves_7,
    gStageObjK1C_ScaleCurves_7, gStageObjK1C_ScaleCurves_9, gStageObjK1C_ScaleCurves_9, gStageObjK1C_ScaleCurves_9,
    gStageObjK1C_ScaleCurves_12, gStageObjK1C_ScaleCurves_12, gStageObjK1C_ScaleCurves_12, gStageObjK1C_ScaleCurves_15,
    gStageObjK1C_ScaleCurves_16, gStageObjK1C_ScaleCurves_17, gStageObjK1C_ScaleCurves_18, gStageObjK1C_ScaleCurves_19,
    gStageObjK1C_ScaleCurves_20, gStageObjK1C_ScaleCurves_21, gStageObjK1C_ScaleCurves_21, gStageObjK1C_ScaleCurves_21,
    gStageObjK1C_ScaleCurves_24, gStageObjK1C_ScaleCurves_25, gStageObjK1C_ScaleCurves_25, gStageObjK1C_ScaleCurves_27,
    gStageObjK1C_ScaleCurves_28, gStageObjK1C_ScaleCurves_28, gStageObjK1C_ScaleCurves_30, gStageObjK1C_ScaleCurves_31,
    gStageObjK1C_ScaleCurves_32, gStageObjK1C_ScaleCurves_32, gStageObjK1C_ScaleCurves_34, gStageObjK1C_ScaleCurves_35,
    gStageObjK1C_ScaleCurves_35, gStageObjK1C_ScaleCurves_35, gStageObjK1C_ScaleCurves_38, gStageObjK1C_ScaleCurves_39,
    gStageObjK1C_ScaleCurves_40, gStageObjK1C_ScaleCurves_41, gStageObjK1C_ScaleCurves_39, gStageObjK1C_ScaleCurves_43,
    gStageObjK1C_ScaleCurves_44, gStageObjK1C_ScaleCurves_45, gStageObjK1C_ScaleCurves_46, gStageObjK1C_ScaleCurves_47,
    gStageObjK1C_ScaleCurves_47, gStageObjK1C_ScaleCurves_49, gStageObjK1C_ScaleCurves_50, gStageObjK1C_ScaleCurves_50,
    gStageObjK1C_ScaleCurves_52, gStageObjK1C_ScaleCurves_53, gStageObjK1C_ScaleCurves_53, gStageObjK1C_ScaleCurves_55,
    gStageObjK1C_ScaleCurves_56, gStageObjK1C_ScaleCurves_56, gStageObjK1C_ScaleCurves_58, gStageObjK1C_ScaleCurves_59,
    gStageObjK1C_ScaleCurves_60, gStageObjK1C_ScaleCurves_60, gStageObjK1C_ScaleCurves_62, gStageObjK1C_ScaleCurves_63,
    gStageObjK1C_ScaleCurves_64, gStageObjK1C_ScaleCurves_65, gStageObjK1C_ScaleCurves_66, gStageObjK1C_ScaleCurves_66,
    gStageObjK1C_ScaleCurves_68, gStageObjK1C_ScaleCurves_68, gStageObjK1C_ScaleCurves_70, gStageObjK1C_ScaleCurves_71,
    gStageObjK1C_ScaleCurves_72, gStageObjK1C_ScaleCurves_73, gStageObjK1C_ScaleCurves_74, gStageObjK1C_ScaleCurves_74,
    gStageObjK1C_ScaleCurves_76, gStageObjK1C_ScaleCurves_77, gStageObjK1C_ScaleCurves_78, gStageObjK1C_ScaleCurves_79,
    gStageObjK1C_ScaleCurves_80, gStageObjK1C_ScaleCurves_81, gStageObjK1C_ScaleCurves_82, gStageObjK1C_ScaleCurves_83,
    gStageObjK1C_ScaleCurves_84, gStageObjK1C_ScaleCurves_85, gStageObjK1C_ScaleCurves_86, gStageObjK1C_ScaleCurves_87,
    gStageObjK1C_ScaleCurves_87, gStageObjK1C_ScaleCurves_87, gStageObjK1C_ScaleCurves_90, gStageObjK1C_ScaleCurves_91,
    gStageObjK1C_ScaleCurves_92, gStageObjK1C_ScaleCurves_93, gStageObjK1C_ScaleCurves_94, gStageObjK1C_ScaleCurves_94,
    gStageObjK1C_ScaleCurves_96, gStageObjK1C_ScaleCurves_96, gStageObjK1C_ScaleCurves_98, gStageObjK1C_ScaleCurves_98,
    gStageObjK1C_ScaleCurves_98, gStageObjK1C_ScaleCurves_101, gStageObjK1C_ScaleCurves_102, gStageObjK1C_ScaleCurves_103,
    gStageObjK1C_ScaleCurves_104, gStageObjK1C_ScaleCurves_105, gStageObjK1C_ScaleCurves_106, gStageObjK1C_ScaleCurves_107,
    gStageObjK1C_ScaleCurves_108, gStageObjK1C_ScaleCurves_109, gStageObjK1C_ScaleCurves_110, gStageObjK1C_ScaleCurves_111,
    gStageObjK1C_ScaleCurves_112, gStageObjK1C_ScaleCurves_113, gStageObjK1C_ScaleCurves_114, gStageObjK1C_ScaleCurves_115,
    gStageObjK1C_ScaleCurves_116, gStageObjK1C_ScaleCurves_117, gStageObjK1C_ScaleCurves_118, gStageObjK1C_ScaleCurves_118,
    gStageObjK1C_ScaleCurves_120, gStageObjK1C_ScaleCurves_120, NULL, NULL,
};
u8 *gStageObjK1C_ColorKeys[124] = {
    gStageObjK1C_ColorKeys_0, gStageObjK1C_ColorKeys_1, gStageObjK1C_ColorKeys_1, gStageObjK1C_ColorKeys_3,
    gStageObjK1C_ColorKeys_3, gStageObjK1C_ColorKeys_5, gStageObjK1C_ColorKeys_5, gStageObjK1C_ColorKeys_5,
    gStageObjK1C_ColorKeys_8, gStageObjK1C_ColorKeys_8, gStageObjK1C_ColorKeys_8, gStageObjK1C_ColorKeys_11,
    gStageObjK1C_ColorKeys_12, gStageObjK1C_ColorKeys_13, gStageObjK1C_ColorKeys_14, gStageObjK1C_ColorKeys_15,
    gStageObjK1C_ColorKeys_16, gStageObjK1C_ColorKeys_17, gStageObjK1C_ColorKeys_17, gStageObjK1C_ColorKeys_17,
    gStageObjK1C_ColorKeys_20, gStageObjK1C_ColorKeys_21, gStageObjK1C_ColorKeys_21, gStageObjK1C_ColorKeys_23,
    gStageObjK1C_ColorKeys_24, gStageObjK1C_ColorKeys_24, gStageObjK1C_ColorKeys_26, gStageObjK1C_ColorKeys_27,
    gStageObjK1C_ColorKeys_28, gStageObjK1C_ColorKeys_28, gStageObjK1C_ColorKeys_30, gStageObjK1C_ColorKeys_31,
    gStageObjK1C_ColorKeys_31, gStageObjK1C_ColorKeys_31, gStageObjK1C_ColorKeys_34, gStageObjK1C_ColorKeys_35,
    gStageObjK1C_ColorKeys_36, gStageObjK1C_ColorKeys_37, gStageObjK1C_ColorKeys_35, gStageObjK1C_ColorKeys_39,
    gStageObjK1C_ColorKeys_40, gStageObjK1C_ColorKeys_41, gStageObjK1C_ColorKeys_42, gStageObjK1C_ColorKeys_43,
    gStageObjK1C_ColorKeys_44, gStageObjK1C_ColorKeys_44, gStageObjK1C_ColorKeys_46, gStageObjK1C_ColorKeys_47,
    gStageObjK1C_ColorKeys_47, gStageObjK1C_ColorKeys_49, gStageObjK1C_ColorKeys_50, gStageObjK1C_ColorKeys_50,
    gStageObjK1C_ColorKeys_52, gStageObjK1C_ColorKeys_53, gStageObjK1C_ColorKeys_53, gStageObjK1C_ColorKeys_55,
    gStageObjK1C_ColorKeys_56, gStageObjK1C_ColorKeys_57, gStageObjK1C_ColorKeys_57, gStageObjK1C_ColorKeys_59,
    gStageObjK1C_ColorKeys_60, gStageObjK1C_ColorKeys_61, gStageObjK1C_ColorKeys_62, gStageObjK1C_ColorKeys_63,
    gStageObjK1C_ColorKeys_63, gStageObjK1C_ColorKeys_65, gStageObjK1C_ColorKeys_65, gStageObjK1C_ColorKeys_67,
    gStageObjK1C_ColorKeys_68, gStageObjK1C_ColorKeys_69, gStageObjK1C_ColorKeys_70, gStageObjK1C_ColorKeys_71,
    gStageObjK1C_ColorKeys_71, gStageObjK1C_ColorKeys_73, gStageObjK1C_ColorKeys_74, gStageObjK1C_ColorKeys_75,
    gStageObjK1C_ColorKeys_76, gStageObjK1C_ColorKeys_77, gStageObjK1C_ColorKeys_78, gStageObjK1C_ColorKeys_79,
    gStageObjK1C_ColorKeys_80, gStageObjK1C_ColorKeys_81, gStageObjK1C_ColorKeys_82, gStageObjK1C_ColorKeys_83,
    gStageObjK1C_ColorKeys_84, gStageObjK1C_ColorKeys_85, gStageObjK1C_ColorKeys_86, gStageObjK1C_ColorKeys_87,
    gStageObjK1C_ColorKeys_87, gStageObjK1C_ColorKeys_87, gStageObjK1C_ColorKeys_90, gStageObjK1C_ColorKeys_91,
    gStageObjK1C_ColorKeys_92, gStageObjK1C_ColorKeys_93, gStageObjK1C_ColorKeys_94, gStageObjK1C_ColorKeys_94,
    gStageObjK1C_ColorKeys_96, gStageObjK1C_ColorKeys_96, gStageObjK1C_ColorKeys_98, gStageObjK1C_ColorKeys_98,
    gStageObjK1C_ColorKeys_98, gStageObjK1C_ColorKeys_101, gStageObjK1C_ColorKeys_102, gStageObjK1C_ColorKeys_103,
    gStageObjK1C_ColorKeys_104, gStageObjK1C_ColorKeys_105, gStageObjK1C_ColorKeys_106, gStageObjK1C_ColorKeys_107,
    gStageObjK1C_ColorKeys_108, gStageObjK1C_ColorKeys_109, gStageObjK1C_ColorKeys_110, gStageObjK1C_ColorKeys_111,
    gStageObjK1C_ColorKeys_112, gStageObjK1C_ColorKeys_113, gStageObjK1C_ColorKeys_114, gStageObjK1C_ColorKeys_115,
    gStageObjK1C_ColorKeys_116, gStageObjK1C_ColorKeys_117, gStageObjK1C_ColorKeys_118, gStageObjK1C_ColorKeys_118,
    gStageObjK1C_ColorKeys_120, gStageObjK1C_ColorKeys_120, NULL, NULL,
};
s16 gStageObjK1C_Counts[48] = {
#include "game/008D7C10.inc"
};
s16 gStageObjK1C_Limits[48] = {
#include "game/008D7C70.inc"
};
s16 gStageObjK1C_ScaleCurveIdx[48] = {
#include "game/008D7CD0.inc"
};
s16 gStageObjK1C_ColorCurveIdx[48] = {
#include "game/008D7D30.inc"
};
u32 gStageObjK1C_DrawTbls[44] = {
    0, (u32)gStageObjK1C_DrawTbls_1, (u32)gStageObjK1C_DrawTbls_2, (u32)gStageObjK1C_DrawTbls_3,
    (u32)gStageObjK1C_DrawTbls_4, (u32)gStageObjK1C_DrawTbls_5, (u32)gStageObjK1C_DrawTbls_6, (u32)gStageObjK1C_DrawTbls_7,
    (u32)gStageObjK1C_DrawTbls_8, (u32)gStageObjK1C_DrawTbls_9, (u32)gStageObjK1C_DrawTbls_10, 0,
    (u32)gStageObjK1C_DrawTbls_12, (u32)gStageObjK1C_DrawTbls_13, (u32)gStageObjK1C_DrawTbls_14, (u32)gStageObjK1C_DrawTbls_15,
    (u32)gStageObjK1C_DrawTbls_16, (u32)gStageObjK1C_DrawTbls_15, (u32)gStageObjK1C_DrawTbls_10, (u32)gStageObjK1C_DrawTbls_19,
    (u32)gStageObjK1C_DrawTbls_20, (u32)gStageObjK1C_DrawTbls_20, (u32)gStageObjK1C_DrawTbls_22, (u32)gStageObjK1C_DrawTbls_23,
    (u32)gStageObjK1C_DrawTbls_24, (u32)gStageObjK1C_DrawTbls_25, (u32)gStageObjK1C_DrawTbls_26, (u32)gStageObjK1C_DrawTbls_27,
    (u32)gStageObjK1C_DrawTbls_28, (u32)gStageObjK1C_DrawTbls_29, (u32)gStageObjK1C_DrawTbls_30, (u32)gStageObjK1C_DrawTbls_31,
    (u32)gStageObjK1C_DrawTbls_32, (u32)gStageObjK1C_DrawTbls_33, (u32)gStageObjK1C_DrawTbls_34, (u32)gStageObjK1C_DrawTbls_35,
    (u32)gStageObjK1C_DrawTbls_36, (u32)gStageObjK1C_DrawTbls_37, (u32)gStageObjK1C_DrawTbls_38, (u32)gStageObjK1C_DrawTbls_39,
    (u32)gStageObjK1C_DrawTbls_40, 0, 0, 0,
};
u32 gStageObjK1C_ModelSetIds[24] = {
#include "game/008D7E40.inc"
};
u32 gStageObjK1C_SeqTbls[42] = {
    0, (u32)gStageObjK1C_SeqTbls_1, (u32)gStageObjK1C_SeqTbls_2, 0,
    0, (u32)gStageObjK1C_SeqTbls_5, (u32)gStageObjK1C_SeqTbls_5, (u32)gStageObjK1C_SeqTbls_7,
    (u32)gStageObjK1C_SeqTbls_8, (u32)gStageObjK1C_SeqTbls_9, (u32)gStageObjK1C_SeqTbls_10, 0,
    (u32)StageObj_K1C_StepAFrames, (u32)gStageObjK1C_SeqTbls_13, (u32)gStageObjK1C_SeqTbls_14, (u32)StageObj_K1C_StepCFrames,
    (u32)StageObj_K1C_T0Frames, (u32)StageObj_K1C_StepCFrames, (u32)gStageObjK1C_SeqTbls_10, (u32)gStageObjK1C_SeqTbls_19,
    (u32)gStageObjK1C_SeqTbls_20, (u32)gStageObjK1C_SeqTbls_20, (u32)gStageObjK1C_SeqTbls_22, (u32)gStageObjK1C_SeqTbls_23,
    (u32)gStageObjK1C_SeqTbls_24, (u32)gStageObjK1C_SeqTbls_25, (u32)gStageObjK1C_SeqTbls_26, (u32)gStageObjK1C_SeqTbls_27,
    (u32)gStageObjK1C_SeqTbls_28, (u32)StageObj_K1C_StepBFrames, (u32)StageObj_K1C_T6Frames, (u32)gStageObjK1C_SeqTbls_31,
    (u32)StageObj_K1C_T2Frames, (u32)StageObj_K1C_T33Frames, (u32)gStageObjK1C_SeqTbls_34, 0,
    (u32)gStageObjK1C_SeqTbls_36, (u32)gStageObjK1C_SeqTbls_37, (u32)gStageObjK1C_SeqTbls_38, (u32)gStageObjK1C_SeqTbls_39,
    (u32)gStageObjK1C_SeqTbls_40, 0,
};
u8 gStageObjK1C_ModelSets_0[8] = {
#include "game/008D7F48.inc"
};
u8 gStageObjK1C_ModelSets_1[8] = {
#include "game/008D7F50.inc"
};
u8 gStageObjK1C_ModelSets_2[8] = {
#include "game/008D7F58.inc"
};
u8 gStageObjK1C_ModelSets_3[8] = {
#include "game/008D7F60.inc"
};
u8 gStageObjK1C_ModelSets_4[8] = {
#include "game/008D7F68.inc"
};
u8 gStageObjK1C_ModelSets_5[8] = {
#include "game/008D7F70.inc"
};
u8 gStageObjK1C_ModelSets_6[8] = {
#include "game/008D7F78.inc"
};
u8 gStageObjK1C_ModelSets_7[8] = {
#include "game/008D7F80.inc"
};
u8 gStageObjK1C_ModelSets_8[8] = {
#include "game/008D7F88.inc"
};
u8 gStageObjK1C_ModelSets_9[16] = {
#include "game/008D7F90.inc"
};
u8 *gStageObjK1C_ModelSets[12] = {
    gStageObjK1C_ModelSets_0, gStageObjK1C_ModelSets_1, gStageObjK1C_ModelSets_2, gStageObjK1C_ModelSets_3,
    gStageObjK1C_ModelSets_4, gStageObjK1C_ModelSets_5, gStageObjK1C_ModelSets_6, gStageObjK1C_ModelSets_7,
    gStageObjK1C_ModelSets_8, gStageObjK1C_ModelSets_9, NULL, NULL,
};
f32 gAreaFlyerCenters[65][3] = {
#include "game/008D7FD0.inc"
};
f32 gAreaFlyerScaleKeys[8] = {
#include "game/008D82E0.inc"
};
u8 gAreaFlyerColorKeys[32] = {
#include "game/008D8300.inc"
};
u32 StageObj_Gun_ScaleKeyTbl_0[20] = {
#include "game/008D8320.inc"
};
u32 StageObj_Gun_ScaleKeyTbl_1[20] = {
#include "game/008D8370.inc"
};
u32 StageObj_Gun_ScaleKeyTbl_2[12] = {
#include "game/008D83C0.inc"
};
u32 StageObj_Gun_ColorKeyTbl_0[8] = {
#include "game/008D83F0.inc"
};
u32 StageObj_Gun_ColorKeyTbl_1[8] = {
#include "game/008D8410.inc"
};
u32 StageObj_Gun_ColorKeyTbl_2[6] = {
#include "game/008D8430.inc"
};
u32 StageObj_Gun_ModelIds[2] = {
#include "game/008D8448.inc"
};
u32 StageObj_Gun_ScaleKeyTbl[4] = {
    (u32)StageObj_Gun_ScaleKeyTbl_0, (u32)StageObj_Gun_ScaleKeyTbl_1, (u32)StageObj_Gun_ScaleKeyTbl_2, 0,
};
u32 StageObj_Gun_ColorKeyTbl[4] = {
    (u32)StageObj_Gun_ColorKeyTbl_0, (u32)StageObj_Gun_ColorKeyTbl_1, (u32)StageObj_Gun_ColorKeyTbl_2, 0,
};
u32 StageObj_Gun_ScaleKeys2[12] = {
#include "game/008D8470.inc"
};
u32 StageObj_Gun_ColorKeys2[12] = {
#include "game/008D84A0.inc"
};
u32 StageObj_Gun_ScaleKeys9[12] = {
#include "game/008D84D0.inc"
};
u32 StageObj_Gun_ColorKeys9[8] = {
#include "game/008D8500.inc"
};
u32 StageObj_Gun_ScaleKeys10[12] = {
#include "game/008D8520.inc"
};
u32 StageObj_Gun_ColorKeys10[6] = {
#include "game/008D8550.inc"
};
u32 StageObj_Gun_Vars01_0[3] = {
#include "game/008D8568.inc"
};
u32 StageObj_Gun_Vars01_1[12] = {
#include "game/008D8580.inc"
};
u32 StageObj_Gun_Vars23_0[8] = {
#include "game/008D85B0.inc"
};
u32 StageObj_Gun_Vars23_1[12] = {
#include "game/008D85D0.inc"
};
u32 StageObj_Gun_Vars01[2] = {
    (u32)StageObj_Gun_Vars01_0, (u32)StageObj_Gun_Vars01_1,
};
u32 StageObj_Gun_Vars23[2] = {
    (u32)StageObj_Gun_Vars23_0, (u32)StageObj_Gun_Vars23_1,
};
u32 StageObj_Gun_Pos01_0[12] = {
#include "game/008D8610.inc"
};
u32 StageObj_Gun_Pos01_1[10] = {
#include "game/008D8640.inc"
};
u32 StageObj_Gun_Pos23_0[3] = {
#include "game/008D8668.inc"
};
u32 StageObj_Gun_Pos23_1[24] = {
#include "game/008D8680.inc"
};
u32 StageObj_Gun_Pos01[2] = {
    (u32)StageObj_Gun_Pos01_0, (u32)StageObj_Gun_Pos01_1,
};
u32 StageObj_Gun_Pos23[2] = {
    (u32)StageObj_Gun_Pos23_0, (u32)StageObj_Gun_Pos23_1,
};
u32 StageObj_Gun_Scale01_0[4] = {
#include "game/008D86F0.inc"
};
u32 StageObj_Gun_Scale01_1[4] = {
#include "game/008D8700.inc"
};
u32 StageObj_Gun_Scale23_0[4] = {
#include "game/008D8710.inc"
};
u32 StageObj_Gun_Scale23_1[8] = {
#include "game/008D8720.inc"
};
u32 StageObj_Gun_Scale01[2] = {
    (u32)StageObj_Gun_Scale01_0, (u32)StageObj_Gun_Scale01_1,
};
u32 StageObj_Gun_Scale23[2] = {
    (u32)StageObj_Gun_Scale23_0, (u32)StageObj_Gun_Scale23_1,
};
u32 gStageObjGun_Lifetimes[2] = {
#include "game/008D8750.inc"
};
u32 StageObj_Gun_Life4[2] = {
#include "game/008D8758.inc"
};
u32 StageObj_Gun_Param4[4] = {
#include "game/008D8760.inc"
};
u32 StageObj_Gun_ScaleKeys0[16] = {
#include "game/008D8770.inc"
};
u32 StageObj_Gun_ColorKeys0[8] = {
#include "game/008D87B0.inc"
};
u32 StageObj_Gun_ScaleKeys1[12] = {
#include "game/008D87D0.inc"
};
u32 StageObj_Gun_ColorKeys1[8] = {
#include "game/008D8800.inc"
};
u32 StageObj_Gun_ScaleKeys5[12] = {
#include "game/008D8820.inc"
};
u32 StageObj_Gun_ColorKeys5[8] = {
#include "game/008D8850.inc"
};
u32 StageObj_Gun_ScaleKeys7[12] = {
#include "game/008D8870.inc"
};
u32 StageObj_Gun_ColorKeys7[6] = {
#include "game/008D88A0.inc"
};
u32 StageObj_Gun_ScaleZ1[2] = {
#include "game/008D88B8.inc"
};
u32 StageObj_Gun_Jitter1[4] = {
#include "game/008D88C0.inc"
};
u32 StageObj_Gun_Jitter2[6] = {
#include "game/008D88D0.inc"
};
u32 StageObj_Gun_ScaleZ3[2] = {
#include "game/008D88E8.inc"
};
u32 StageObj_Gun_Life1[2] = {
#include "game/008D88F0.inc"
};
u32 StageObj_Gun_End1[2] = {
#include "game/008D88F8.inc"
};
u32 StageObj_Gun_SizeScaleKeys1_0[12] = {
#include "game/008D8900.inc"
};
u32 StageObj_Gun_SizeColorKeys1_0[8] = {
#include "game/008D8930.inc"
};
u32 StageObj_Gun_SizeScaleKeys2_0[16] = {
#include "game/008D8950.inc"
};
u32 StageObj_Gun_SizeColorKeys2_0[8] = {
#include "game/008D8990.inc"
};
u32 StageObj_Gun_SizeScaleKeys3_0[12] = {
#include "game/008D89B0.inc"
};
u32 StageObj_Gun_SizeColorKeys3_0[8] = {
#include "game/008D89E0.inc"
};
u32 StageObj_Gun_SizeScaleKeys1_1[12] = {
#include "game/008D8A00.inc"
};
u32 StageObj_Gun_SizeColorKeys1_1[8] = {
#include "game/008D8A30.inc"
};
u32 StageObj_Gun_SizeScaleKeys2_1[16] = {
#include "game/008D8A50.inc"
};
u32 StageObj_Gun_SizeColorKeys2_1[8] = {
#include "game/008D8A90.inc"
};
u32 StageObj_Gun_SizeScaleKeys3_1[12] = {
#include "game/008D8AB0.inc"
};
u32 StageObj_Gun_SizeColorKeys3_1[6] = {
#include "game/008D8AE0.inc"
};
u32 D_008D8AF8[2] = {
#include "game/008D8AF8.inc"
};
u32 StageObj_Gun_SizeScaleKeys1[2] = {
    (u32)StageObj_Gun_SizeScaleKeys1_0, (u32)StageObj_Gun_SizeScaleKeys1_1,
};
u32 StageObj_Gun_SizeColorKeys1[2] = {
    (u32)StageObj_Gun_SizeColorKeys1_0, (u32)StageObj_Gun_SizeColorKeys1_1,
};
u32 StageObj_Gun_SizeScaleKeys2[2] = {
    (u32)StageObj_Gun_SizeScaleKeys2_0, (u32)StageObj_Gun_SizeScaleKeys2_1,
};
u32 StageObj_Gun_SizeColorKeys2[2] = {
    (u32)StageObj_Gun_SizeColorKeys2_0, (u32)StageObj_Gun_SizeColorKeys2_1,
};
u32 StageObj_Gun_SizeScaleKeys3[2] = {
    (u32)StageObj_Gun_SizeScaleKeys3_0, (u32)StageObj_Gun_SizeScaleKeys3_1,
};
u32 StageObj_Gun_SizeColorKeys3[2] = {
    (u32)StageObj_Gun_SizeColorKeys3_0, (u32)StageObj_Gun_SizeColorKeys3_1,
};
u32 StageObj_Gun_Life6[2] = {
#include "game/008D8B30.inc"
};
u32 StageObj_Gun_Param6[2] = {
#include "game/008D8B38.inc"
};
u32 StageObj_Gun_ScaleKeys6[12] = {
#include "game/008D8B40.inc"
};
u32 StageObj_Gun_ColorKeys6[8] = {
#include "game/008D8B70.inc"
};
