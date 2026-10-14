/*
 * Game overlay code at 0x868110-0x874B90, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "sprite.h"
#include "quest.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "objattach.h"
#include "capsule.h"
#include "scene.h"
/* prototypes of callees (round 13) */
void Vec_Copy(f32 *out, f32 *in);
void DrawSlot_Free(s32 i);
void DrawSlotB_Free(s32);
void Vec_Rotate(f32 *out, f32 *in, f32 *mtx);
u8 Player_InCurrentArea(void *obj);

typedef struct Obj0FPart {
    s16 kind;                   /* 0x00: its index; type 9: 1 */
    s16 slot;                   /* 0x02: its draw slot */
    f32 pos[3];                 /* 0x04 */
    f32 sc[3];                  /* 0x10: its scale, from a curve by its frame */
    f32 scale;                  /* 0x1C */
    DrawSlot *node;                /* 0x20 */
    s16 unk24;                  /* 0x24: its frame, from a delay (negative) */
    u16 unk26;                  /* 0x26: a random angle */
    s16 unk28;                  /* 0x28: its spin */
    u8 alpha;                   /* 0x2A: from a curve by its frame */
    u8 unk2B;
} Obj0FPart;

/* a part of a kind 0xE object (0x34 bytes, in its blocks) */
typedef struct Obj0EPart {
    s16 unk00;                  /* 0x00: a random 0..31 */
    s16 unk02;                  /* 0x02: counts up from -9 or -15 */
    s16 slot;                   /* 0x04: its draw slot */
    s16 unk06;                  /* 0x06 */
    u16 unk08;                  /* 0x08: a random angle */
    s16 unk0A;                  /* 0x0A: its speed, +-0x5B */
    f32 unk0C[3];               /* 0x0C */
    f32 unk18[3];               /* 0x18 */
    f32 unk24;                  /* 0x24: a scale */
    f32 unk28;                  /* 0x28: a scale */
    f32 unk2C;                  /* 0x2C */
    DrawSlot *node;                /* 0x30 */
} Obj0EPart;

/* a part of a kind 0x0C object of types 5 and 6 (0x34 bytes, in its blocks) */
typedef struct Obj0CPart {
    s16 slot;                   /* 0x00: its draw slot */
    s16 idx;                    /* 0x02 */
    f32 unk04[3];               /* 0x04: the shape (the scale is unk10 times it) */
    f32 unk10;                  /* 0x10: the size: 1.6..2.2 (type 6) */
    f32 alpha;                  /* 0x14 */
    f32 unk18;                  /* 0x18: 0.15 (type 6) */
    s16 delay;                  /* 0x1C: frames before it starts (-5 per part) */
    u16 unk1E;                  /* 0x1E: a random angle */
    DrawSlot *node;                /* 0x20 */
    f32 pos[3];                 /* 0x24 */
    s16 spin;                   /* 0x30 */
    u8 unk32[0x34 - 0x32];
} Obj0CPart;

/* a colour (r, g, b, unused) */
typedef struct Rgb4 {
    u8 r;
    u8 g;
    u8 b;
    u8 unk3;
} Rgb4;

/* a part of a kind 0x10 object (0x34 bytes, in its blocks) */
typedef struct Obj10Part {
    s16 slot;                   /* 0x00: its draw slot */
    s16 delay;                  /* 0x02: frames before it starts (negative) */
    f32 pos[3];                 /* 0x04 */
    f32 sc[3];                  /* 0x10: its scale */
    s16 idx;                    /* 0x1C: its index or frame */
    u16 angle;                  /* 0x1E */
    u16 alpha;                  /* 0x20: 0xFF */
    u8 unk22[0x24 - 0x22];
    f32 scale;                  /* 0x24 */
    f32 unk28;                  /* 0x28: -0.8 (type 0) */
    DrawSlot *node;                /* 0x2C */
    s16 unk30;                  /* 0x30 */
    u8 unk32[0x34 - 0x32];
} Obj10Part;

/* a step of the kind 0x10 type 0 burst: at frame t, a spawn of size v (tenths) */
typedef struct K10Step {
    s16 t;
    s16 v;
} K10Step;

/* prototypes */
extern u8 StageObj_K0F_ColorKeys1[];
extern u8 StageObj_K0F_ColorKeysB[];
extern u8 StageObj_K0F_ColorKeys[];
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
typedef struct Obj0EWork {
    Obj0EPart part[7];          /* 0x000 */
    EmObj flash;                /* 0x16C: its flash (FlashObj_Add): 5 frames */
} Obj0EWork;
s32 Em_PlaySeChecked(Em *em, s32 id, s32 a, void *pos, s32 b, s32 c);
s32 FlashObj_Add(EmObj *obj);
void StageObj_K0E_CreateOwned(f32 *pos, s16 k, f32 scale, void *owner);
extern void *StageObj_K0E_PartCurves[];
extern s16 StageObj_K0E_ListA[];
extern s16 StageObj_K0E_ListB[];
void SoundObj_Start(s32 bank, s32 se, s32 k, f32 *pos, s16 n, u8 quiet);
s32 SpecialMine_Rank(void);
extern s16 D_008CEEA8[];
void StageObj_K10_PlaySound(StageObj *o, f32 *pos);
void StageObj_K10_Draw(DrawSlot *node);
s32 Effect_KindClass_Unk(s32 a);
void Effect_ScaleVec(f32 *scale);
extern Rgb4 gEffectColors[];
void Mtx_RotateX(f32 *m, f32 a);
extern void *StageObj_K0C_PartCurves[];
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void StageObj_K0C_BonePosOffset(f32 *out, f32 *m, f32 *v);
void Mtx_RotateXYZ(f32 *m, f32 x, f32 y, f32 z);
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
extern s16 StageObj_K0F_Limits[];
extern s16 StageObj_K0F_Steps[];
extern s16 StageObj_K0F_CurveIdx[];
extern s16 StageObj_K0F_Durations[];
extern s16 *StageObj_K0F_DurLists[];
extern void *StageObj_K0F_Curves[];
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Em_BonePos(void *obj, s16 bone, f32 *out);
s32 StageObj_K0F_RestartPart(StageObj *o, Obj0FPart *w);
s32 StageObj_K0F_OwnerBone(StageObj *o);
s32 EmMotion_ReachedFrame(void *obj, s32 a, f32 frame);
extern s16 StageObj_K0F_DelaysA[];
extern s16 StageObj_K0F_DelaysC[];
extern s16 StageObj_K0F_DelaysB[];
void StageObj_K0F_PlaySound(StageObj *o);
void StageObj_K0F_Draw(DrawSlot *node);
extern s16 StageObj_K0C_PlayerVals[];
s32 Player_IsLocal(void *obj);
void Vec_Set(f32 *v, f32 x, f32 y, f32 z);
s32 ObjList_Add(EmObj *obj);
void Vec_RotateY(f32 *v, f32 a);
extern Rgb4 StageObj_K0C_Colors[];
extern f32 gCamMatrix[];
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Concat3x3(f32 *m, f32 *n);
s32 Gs_SetState(u8 reg, u32 v);
s32 Effect_DrawModel(void *mdl, f32 *m, s32 color, u16 flags, void *a4);
void StageObj_K0C_Draw(DrawSlot *node);
void StageObj_K0C_Run(StageObj *o);
void StageObj_K0C_PlaySound(StageObj *o, s16 i);
EmBone *Em_Bone(void *obj, s16 bone);
extern u32 StageObj_K0E_PartInit[];
extern f32 D_008CEE84[]; /* StageObj_K0E_PartInit + 4 */
f32 Lure_RandSigned(void);
extern f32 StageObj_K10_Scales[];
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void StageObj_K0E_Draw(DrawSlot *node);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
void StageObj_K0C_Create(f32 *pos, s16 v, s32 k, u8 c);
void Mtx_Copy(f32 *dst, f32 *src);
void Mtx_Inverse(f32 *out, f32 *m);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_RotateSelf(f32 *v, f32 *m);
void Vec_RotateY(f32 *v, f32 angle);
u16 Rand_Next(s32 stream);
void StageObj_K0E_Setup(StageObj *o);
void StageObj_K0E_SetupParts(StageObj *o);
void StageObj_K0E_SetupModel(StageObj *o);
void StageObj_K0E_Run(StageObj *o);
void StageObj_K0E_RunParts(StageObj *o);
void StageObj_K0E_RunModel(StageObj *o);
void StageObj_K0E_End(StageObj *o);
void StageObj_K0E_Free(StageObj *o);
void StageObj_K0F_Setup(StageObj *o);
void StageObj_K0F_Run(StageObj *o);
void StageObj_K0F_End(StageObj *o);
void StageObj_K0F_Free(StageObj *o);
StageObj *StageObj_K0F_CreateAt(f32 *, s32, s32, f32);
void StageObj_K10_Setup(StageObj *o);
void StageObj_K10_Run(StageObj *o);
void StageObj_K10_End(StageObj *o);
void StageObj_K10_Free(StageObj *o);
void StageObj_K11_Setup(StageObj *o);
void StageObj_K11_Run(StageObj *o);
void StageObj_K11_End(StageObj *o);
void StageObj_K11_Free(StageObj *o);
void StageObj_K11_RunRocks(StageObj *o);
void StageObj_K11_RunDefault(StageObj *o);
void StageObj_K11_EndRocks(StageObj *o);
void StageObj_K11_EndDefault(StageObj *o);
void StageObj_K0C_Update(StageObj *o);
extern s16 StageObj_K0F_PartCounts[];
extern s16 StageObj_K10_PartCounts[];
extern s16 StageObj_K0E_PartCounts[];
void FlashObj_Remove(EmObj *obj);
void StageObj_K11_SetupRocks(StageObj *o);
void StageObj_K11_SetupDefault(StageObj *o);
void StageObj_K11_DrawDefault(DrawSlot *node);
void StageObj_K11_DrawRock(DrawSlot *node);
StageObj *StageObj_K0C_Alloc(s32 kind);
f32 Math_Sin(f32 x);
void Gfx_SetBlend(s32 a, s32 b);
extern s16 StageObj_K0E_Ids[];
extern Rgb4 StageObj_K0E_Colors[];
s16 EmSlot_FindKind(s16 id);
void Draw_SetDefaultState(void);
void Mtx_TurnY(f32 *m, f32 a);
extern u8 StageObj_K11_Curve3A[];
extern u8 StageObj_K11_ColorKeys[];
extern u8 StageObj_K11_Curve3B[];
extern u8 StageObj_K11_Curve10A[];
extern u8 StageObj_K11_Curve10B[];
extern u8 StageObj_K11_CurveFA[];
extern u8 StageObj_K11_CurveFB[];
extern u8 StageObj_K11_CurveFC[];
extern u8 StageObj_K11_Curve12A[];
extern u8 StageObj_K11_Curve12B[];
extern s16 StageObj_K10_List[];
extern u32 StageObj_K10_Curves[];
extern K10Step StageObj_K10_TblA[];
extern K10Step StageObj_K10_TblB[];
extern K10Step StageObj_K10_TblC[];
extern s16 StageObj_K10_Lives[];
extern s16 StageObj_K10_CurveIdx[];
extern s16 StageObj_K10_Ends[];
extern s16 StageObj_K10_Strides[];
f32 Effect_Scale(void);
void Effect02_SpawnForOwner(void *owner, s32 ang, s32 type, s32 sub, f32 *pos, f32 scale);
s32 StageObj_K10_SpinSpeed(s16 x);
/* end of prototypes */

void StageObj_K0C_SetupParts(StageObj *o)
{
    Obj0CPart *p = (Obj0CPart *) o->buf;
    s16 n;
    s16 i;

    o->draw = NULL;
    o->timer = 0;
    o->angle = 0;
    switch (o->type) {
    case 5:
    default:
        StageObj_K0C_PlaySound(o, 0);
        n = 6;
        break;
    case 6:
        n = 1;
        break;
    }
    for (i = 0; i < n; i++, p++) {
        switch (o->type) {
        case 5:
        default:
            p->idx = i;
            p->delay = -i * 5;
            p->unk1E = Rand_Next(1);
            Vec_Copy(p->pos, o->pos);
            break;
        case 6:
            p->idx = i;
            p->delay = 0;
            p->unk1E = Rand_Next(1);
            p->spin = (Rand_Next(1) & 0x1FF) - 0x100;
            p->unk18 = 0.15f;
            p->unk10 = 1.6f + 0.6f * (0.001f * (s32) (Rand_Next(1) & 0x3FF));
            Vec_Copy(p->pos, o->pos);
            break;
        }
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K0C_Draw;
        } else {
            p->node = NULL;
        }
    }
    StageObj_K0C_Run(o);
}

void StageObj_K0C_RunParts(StageObj *o)
{
    Obj0CPart *p = (Obj0CPart *) o->buf;
    s16 i;
    s16 dur;
    s16 limit;
    s16 n;
    void *curve;
    s16 c;

    switch (o->type) {
    case 5:
    default:
        n = 6;
        limit = 0x7E;
        dur = 0x15;
        if (o->timer < 90 && o->timer % 20 == 0) {
            StageObj_K0C_PlaySound(o, 1);
        }
        break;
    case 6:
        limit = 60;
        n = 1;
        dur = 60;
        break;
    }
    if (++o->timer > limit) {
        o->state++;
        o->visible = 0;
        return;
    }
    for (i = 0; i < n; i++) {
        switch (o->type) {
        case 5:
        default:
            c = 0;
            break;
        case 6:
            c = 2;
            break;
        }
        if (++p->delay <= 0) {
            p++;
            continue;
        }
        if (p->delay == 1) {
            if (o->type == 5) {
                if (o->timer < 50) {
                    p->unk10 = 4.0f + 4.0f * (o->timer / 50.0f);
                } else {
                    p->unk10 = 8.0f;
                }
                if (o->timer < 90) {
                    p->unk18 = 1.0f;
                } else {
                    p->unk18 = 1.0f - (o->timer - 90) / 15.0f;
                    if (p->unk18 < 0.0f) {
                        p->unk18 = 0.0f;
                    }
                }
                p->unk1E = Rand_Next(1);
                p->spin = 0.5f + (65536.0f * (0.002f * ((Rand_Next(1) & 0x3FF) - 0x200))) / 360.0f;
                Vec_Copy(p->pos, o->pos);
            }
        } else if (p->delay > dur) {
            if (o->type == 5 && o->timer < 105) {
                p->delay = -9;
                p->idx += 6;
            }
            p++;
            continue;
        }
        curve = StageObj_K0C_PartCurves[c++];
        Key_LerpVec3(curve, p->unk04, p->delay);
        Key_LerpF32(StageObj_K0C_PartCurves[c], &p->alpha, p->delay);
        p->alpha *= p->unk18;
        switch (o->type) {
        case 5:
        default:
            p->pos[1] += (40.0f * p->unk10) / 21.0f;
            break;
        case 6:
            p->pos[1] += p->unk10 * (0.5f + 0.00025f * (Rand_Next(1) & 0x3FF));
            break;
        }
        p->unk1E += p->spin;
        if (p->node != NULL) {
            Vec_Copy(p->node->pos, p->pos);
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
        }
        p++;
    }
}

void StageObj_K0C_DrawParts(StageObj *o, DrawSlot *node)
{
    f32 scale[3];
    f32 rot[3];
    f32 m[16];
    Obj0CPart *p = (Obj0CPart *) o->buf + node->part;
    ModelMat *mats;
    s32 color;
    u8 a;
    ModelMatSet *model;
    Model *set = gStageModelSet;

    if (set != NULL && set->active != 0) {
        mats = set->mats;
        Gs_SetState(0x60, 0);
        Gs_SetState(0x6C, 0);
        scale[0] = p->unk10 * p->unk04[0];
        scale[1] = p->unk10 * p->unk04[1];
        scale[2] = p->unk10 * p->unk04[2];
        rot[2] = (6.2831855f * p->unk1E) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        a = 255.0f * p->alpha;
        switch (o->type) {
        case 5:
        default:
            model = &set->matSets[72];
            color = StageObj_K0C_Colors[o->var1].b
                    | ((StageObj_K0C_Colors[o->var1].g << 8) | ((a << 24) | (StageObj_K0C_Colors[o->var1].r << 16)));
            break;
        case 6:
            model = &set->matSets[74];
            color = (a << 24) | 0xFFFFFF;
            break;
        }
        Mtx_Concat3x3(m, gCamMatrix);
        Effect_DrawModel(model, m, color, 0, mats);
        Gs_SetState(0x6C, 1);
    }
}

void StageObj_K0C_End(StageObj *o)
{
    u8 *p = o->buf;
    s16 n;
    s16 i;

    o->state++;
    o->visible = 0;
    switch (o->type) {
    default:
        DrawSlot_Free(o->slot);
        return;
    case 1:
        n = 4;
        break;
    case 3:
        DrawSlotB_Free(o->slot);
        return;
    case 5:
        n = 6;
        break;
    case 6:
        n = 1;
        break;
    }
    for (i = 0; i < n; i++) {
        if (*(s32 *) (p + 0x20) != 0) {
            DrawSlot_Free(*(s16 *) p);
        }
        p += 0x34;
    }
}

void StageObj_K0C_Free(StageObj *o)
{
    if (o->type == 3 && o->ownerIndex == gScene.localPlayer) {
        (gScene.placed3)--;
    }
    StageObj_Free(o);
}

void StageObj_K0C_PlaySound(StageObj *o, s16 i)
{
    switch (o->type) {
    case 5:
        switch (i) {
        case 0:
            Sound_PlaySeAtPosChecked(1, 0x26, 0, o->pos, 1, 0);
            break;
        case 1:
            Sound_PlaySeAtPosChecked(1, 0x27, 0, o->pos, 1, 0);
            break;
        }
        break;
    }
}

StageObj *StageObj_K0C_Alloc(s32 kind)
{
    switch ((s16) kind) {
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 1:
        return StageObj_Alloc(1);
    case 0:
    default:
        return StageObj_Alloc(0);
    }
}

s32 Obj_InCurrentArea(void *obj);
void StageObj_K0C_CreateAtPlayer(Player *p, s32 k)
{
    StageObj *o;
    StageObjK0CCarry *w;
    f32 sx;
    f32 sy;
    f32 sz;
    f32 rx;
    f32 ry;
    f32 rz;
    f32 m[16];
    f32 v[3];

    if ((s16) k == 3 || (Obj_InCurrentArea(p) & 0xFF)) {
        o = StageObj_K0C_Alloc(k);
        if (o != NULL) {
            o->kind = 0xC;
            o->update = StageObj_K0C_Update;
            o->type = k;
            o->owner = p;
            o->angle2 = p->area;
            o->angle = p->rot[1];
            switch ((s16) k) {
            case 3:
                Vec_Copy(o->pos, p->pos);
                o->var1 = 0;
                v[0] = 0.0f;
                v[1] = 13.0f;
                v[2] = 50.0f;
                Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
                o->pos[0] += v[0];
                o->pos[1] += v[1];
                o->pos[2] += v[2];
                o->keep = 1;
                o->ownerIndex = ((Player *) o->owner)->index;
                if (Player_IsLocal(p)) {
                    (gScene.placed3)++;
                }
                break;
            case 4:
                w = (StageObjK0CCarry *) o->buf;
                w->kind = (p->carry & 0xF0) >> 4;
                switch (w->kind) {
                case 0:
                case 1:
                case 6:
                    sx = 1.0f;
                    sy = 1.3f;
                    sz = 1.0f;
                    rx = -0.9424778f;
                    ry = -3.2812192f;
                    rz = 2.0594885f;
                    v[0] = 5.5f;
                    v[1] = -28.0f;
                    v[2] = 12.0f;
                    break;
                case 2:
                    sx = 1.0f;
                    sy = 1.0f;
                    sz = 1.0f;
                    rx = -0.9424778f;
                    ry = -3.2812192f;
                    rz = 2.0594885f;
                    v[0] = 8.9f;
                    v[1] = -25.0f;
                    v[2] = 4.6f;
                    break;
                case 3:
                case 4:
                case 5:
                    sx = 2.5f;
                    sy = 3.0f;
                    sz = 2.0f;
                    rx = -0.24434611f;
                    ry = 2.86234f;
                    rz = 1.2566371f;
                    v[0] = 3.3f;
                    v[1] = -24.0f;
                    v[2] = 4.0f;
                    break;
                default:
                    return;
                }
                Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(p, 0xE)));
                StageObj_K0C_BonePosOffset(o->pos, m, v);
                Mtx_Scale(w->mtx, sx, sy, sz);
                Mtx_RotateXYZ(w->mtx, rx, ry, rz);
                Mtx_Concat3x3(w->mtx, m);
                break;
            case 6:
                Em_GetBonePos(p, 0xE, o->pos);
                Em_GetBonePos(p, 0x12, v);
                o->pos[0] += v[0];
                o->pos[1] += v[1];
                o->pos[2] += v[2];
                o->pos[0] *= 0.5f;
                o->pos[1] *= 0.5f;
                o->pos[2] *= 0.5f;
                o->pos[1] += 15.0f;
                break;
            default:
                Vec_Copy(o->pos, p->pos);
                break;
            }
        }
    }
}

void StageObj_K0C_CreateFromAttach(ObjAttach *oa, s32 k, u8 c)
{
    StageObj *o;

    if (oa->area == gScene.area) {
        o = StageObj_K0C_Alloc(k);
        if (o != NULL) {
            o->kind = 0xC;
            o->update = StageObj_K0C_Update;
            o->type = k;
            o->pos[0] = oa->pos[0];
            o->pos[1] = oa->pos[1];
            o->pos[2] = oa->pos[2];
            o->var1 = c;
            o->angle2 = oa->area;
            if (o->type == 1) {
                StageObj_K0C_Create(o->pos, 0, 5, o->var1);
            }
        }
    }
}

void StageObj_K0C_Create(f32 *pos, s16 v, s32 k, u8 c)
{
    StageObj *o = StageObj_K0C_Alloc(k);

    if (o != NULL) {
        o->kind = 0xC;
        o->update = StageObj_K0C_Update;
        o->type = k;
        o->var1 = c;
        o->angle = v;
        Vec_Copy(o->pos, pos);
    }
}

void StageObj_K0C_CreateOnPlayer(Player *p, s32 k, u8 c)
{
    StageObj *o;
    StageObjK0CBuf *w;
    f32 v[3];

    o = StageObj_K0C_Alloc(k);
    if (o != NULL) {
        o->kind = 0xC;
        o->update = StageObj_K0C_Update;
        o->type = k;
        o->owner = p;
        o->var1 = c;
        o->angle2 = p->area;
        o->angle = p->rot[1];
        Vec_Copy(o->pos, p->pos);
        v[0] = 0.0f;
        v[1] = 13.0f;
        v[2] = 50.0f;
        Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        o->keep = 1;
        if ((s16) k == 3) {
            o->ownerIndex = ((Player *) o->owner)->index;
            if (Player_IsLocal(p)) {
                (gScene.placed3)++;
            }
            w = (StageObjK0CBuf *) o->buf;
            w->obj.radius = 5000.0f;
            Vec_Set(w->obj.pos, o->pos[0], o->pos[1], o->pos[2]);
            w->obj.goalArg = ((Player *) o->owner)->index;
            w->obj.goalNo = gScene.itemUses[((Player *) o->owner)->index];
            w->obj.done = 0;
            w->obj.kind = o->var1;
            w->obj.unk_Param16 = StageObj_K0C_PlayerVals[o->var1];
            w->obj.area = *(u8 *) &((Player *) o->owner)->area;
            ObjList_Add(&w->obj);
        }
    }
}

void StageObj_K0E_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K0E_Setup(o);
        break;
    case 1:
        StageObj_K0E_Run(o);
        break;
    case 2:
        StageObj_K0E_End(o);
        break;
    case 3:
        StageObj_K0E_Free(o);
        break;
    }
}

void StageObj_K0E_Setup(StageObj *o)
{
    o->state++;
    o->var0 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    if (o->type != 2) {
        StageObj_K0E_SetupParts(o);
        return;
    }
    StageObj_K0E_SetupModel(o);
}

void StageObj_K0E_InitPart(Obj0EPart *w)
{
    s16 i;

    w->unk02 = -9;
    w->unk00 = Rand_Next(1) & 0x1F;
    i = Rand_Next(1) % 5;
    w->unk0C[0] = ((f32 (*)[2])StageObj_K0E_PartInit)[i][0];
    w->unk0C[1] = ((f32 (*)[2])D_008CEE84)[i][0];
    w->unk0C[2] = 1.0f;
    w->unk18[0] = 0.015000001f * (s32) ((Rand_Next(1) & 0x3FF) - 0x200);
    w->unk18[1] = 0.005f * (s32) (Rand_Next(1) & 0x3FF);
    w->unk18[2] = 0.015000001f * (s32) ((Rand_Next(1) & 0x3FF) - 0x200);
}

void StageObj_K0E_InitPartB(Obj0EPart *w)
{
    s16 i;

    w->unk00 = Rand_Next(1) & 0x1F;
    i = Rand_Next(1) % 5;
    w->unk0C[0] = ((f32 (*)[2])StageObj_K0E_PartInit)[i][0];
    w->unk0C[1] = ((f32 (*)[2])D_008CEE84)[i][0];
    w->unk0C[2] = 1.0f;
    w->unk18[0] = 0.010000001f * (s32) ((Rand_Next(1) & 0x3FF) - 0x200);
    w->unk18[1] = 0.0050000004f * (s32) (Rand_Next(1) & 0x3FF);
    w->unk18[2] = 0.010000001f * (s32) ((Rand_Next(1) & 0x3FF) - 0x200);
}

void StageObj_K0E_InitPartAt(StageObj *o, Obj0EPart *w)
{
    u16 r;

    Vec_Copy(w->unk18, o->pos);
    r = Rand_Next(1);
    w->unk02 = -0xF;
    w->unk18[0] += o->scale * (5.0f * Lure_RandSigned());
    w->unk18[2] += o->scale * (5.0f * Lure_RandSigned());
    w->unk06 = r & 1;
    w->unk08 = r;
    w->unk0A = 0x5B;
    if ((r >> 1) & 1) {
        w->unk0A = -w->unk0A;
    }
}

void StageObj_K0E_SetupParts(StageObj *o)
{
    Obj0EPart *w = (Obj0EPart *) o->buf;
    s16 n = StageObj_K0E_PartCounts[o->type];
    s16 i;
    f32 sc;

    switch (o->type) {
    case 3:
        SoundObj_Start(7, 0x10, 0x13, o->pos, 0x4B, 2);
        break;
    case 10:
        o->var0 = SpecialMine_Rank();
        o->angle = 0;
        break;
    }
    for (i = 0; i < n; w++, i++) {
        w->slot = DrawSlot_Alloc();
        if (w->slot != -1) {
            w->node = DrawSlot_Get(w->slot);
            w->node->owner = o;
            w->node->part = i;
            w->node->draw = StageObj_K0E_Draw;
        } else {
            w->node = NULL;
        }
        w->unk02 = 0;
        w->unk00 = 0;
        switch (o->type) {
        case 0:
        case 11:
            w->unk06 = i;
            if (i == 0) {
                w->unk18[0] = 0.0f;
                w->unk18[1] = 0.0f;
                w->unk18[2] = 0.0f;
            }
            if (i == 1 || i == 2) {
                if (Rand_Next(1) & 1) {
                    w->unk06 = 0xFF;
                } else {
                    w->unk02 = -((Rand_Next(1) & 3) + 3);
                    w->unk18[0] = 0.2f * o->scale * ((Rand_Next(1) & 0x3FF) - 0x200);
                    w->unk18[1] = 0.2f * o->scale * ((Rand_Next(1) & 0x3FF) - 0x200);
                    w->unk18[2] = 0.0f;
                }
            }
            w->unk08 = Rand_Next(1);
            break;
        case 1:
            w->unk06 = i;
            w->unk02 = -3 - i * 2;
            w->unk18[0] = o->scale * ((Rand_Next(1) & 0x1F) - 0x10);
            w->unk18[1] = o->scale * ((Rand_Next(1) & 0x1F) - 0x10);
            w->unk18[2] = 20.0f + i * 20;
            w->unk28 = o->scale * (1.0f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            w->unk08 = Rand_Next(1);
            break;
        case 3:
        case 12:
            if (i == 0) {
                w->unk06 = 0xFF;
                w->unk02 = 0;
                w->unk00 = Rand_Next(1) & 0xF;
                w->unk0C[0] = 2.0f;
                w->unk0C[1] = 2.0f;
                w->unk0C[2] = 1.0f;
                w->unk24 = 1.0f;
                w->unk18[0] = 0.0f;
                w->unk18[1] = 0.0f;
                w->unk18[2] = 0.0f;
            } else {
                w->unk06 = i - 1;
                StageObj_K0E_InitPart(w);
                w->unk02 = -w->unk06 * 5;
            }
            w->unk28 = 1.0f;
            break;
        case 4:
            w->unk02 = D_008CEEA8[i];
            if (i < 3) {
                w->unk08 = 0;
                w->unk06 = i;
            } else {
                w->unk08 = Rand_Next(1);
                w->unk06 = 3;
            }
            Vec_Copy(w->unk18, o->pos);
            break;
        case 5:
            w->unk06 = i;
            Vec_Copy(w->unk18, o->pos);
            break;
        case 6:
            w->unk06 = i - 1;
            StageObj_K0E_InitPartB(w);
            w->unk02 = -i * 5;
            w->unk28 = 1.0f;
            break;
        case 8:
            w->unk02 = -6;
            w->unk08 = Rand_Next(1);
            w->unk06 = Rand_Next(1) & 7;
            if (!(w->unk06 & 2)) {
                sc = 0.5f;
                w->unk0A = (s32) (0.5f + (65536.0f * (0.375f * (0.001f * (Rand_Next(1) & 0x3FF)))) / 360.0f);
            } else {
                sc = 0.5f;
                w->unk0A = (s32) (0.5f + (65536.0f * (0.5f * (0.001f * (Rand_Next(1) & 0x3FF)))) / 360.0f);
            }
            if (w->unk06 & 4) {
                w->unk0A = -w->unk0A;
            }
            w->unk28 = 0.7f + 0.0008f * (Rand_Next(1) & 0x3FF);
            w->unk2C = sc + 0.0005f * (Rand_Next(1) & 0x3FF);
            w->unk18[0] = 0.2f * o->scale * ((Rand_Next(1) & 0x3FF) - 0x200);
            w->unk18[1] = 0.1f * o->scale * ((Rand_Next(1) & 0x3FF) - 0x200);
            w->unk18[2] = -30.0f - 5.0f * i;
            break;
        case 9:
            w->unk06 = i;
            w->unk08 = Rand_Next(1);
            w->unk24 = 1.0f;
            break;
        case 10:
            StageObj_K0E_InitPartAt(o, w);
            w->unk02 = -15 * i;
            w->unk28 = 1.0f;
            break;
        case 2:
        case 7:
        default:
            w->unk06 = i;
            break;
        }
    }
}

void StageObj_K0E_SetupModel(StageObj *o)
{
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->draw = StageObj_K0E_Draw;
        Vec_Copy(o->node->pos, o->pos);
    } else {
        StageObj_Free(o);
    }
}

void StageObj_K0E_Run(StageObj *o)
{
    if (o->type != 2) {
        StageObj_K0E_RunParts(o);
        return;
    }
    StageObj_K0E_RunModel(o);
}

extern s16 gAreaFadeTimer;
extern s16 gAreaFade;            /* sub_main: the area's fade */
void StageObj_K0E_RunParts(StageObj *o)
{
    Obj0EPart *w = (Obj0EPart *) o->buf;
    s16 n = StageObj_K0E_PartCounts[o->type];
    u16 limit;
    s16 k;
    u16 dur;
    s16 i;
    s16 j;
    s16 c;
    s32 r;
    Em *em;
    f32 *m;
    f32 v[3];
    void *curve;

    switch (o->type) {
    case 0:
    case 11:
        limit = 0x23;
        break;
    case 1:
        limit = 0x19;
        dur = 0x12;
        break;
    case 10:
        if (gQuest.specialMinePoint == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        dur = limit = 0x1E;
        r = SpecialMine_Rank();
        if (o->var0 != r) {
            o->angle = 0;
            o->var0 = r;
        }
        if (o->angle <= 15) {
            o->angle++;
        }
        break;
    case 3:
        dur = limit = 0x1A;
        break;
    case 4:
        limit = 0x1E;
        if (o->timer == 12) {
            StageObj_K0E_CreateOwned(o->pos, 5, o->scale, o->owner);
        }
        break;
    case 5:
        limit = 0x14;
        switch (o->var0) {
        case 0:
            if (o->timer >= 9) {
                em = o->owner;
                if (em->isEm == 0) {
                    Vec_Copy(((Obj0EWork *) w)->flash.pos, o->pos);
                    ((Obj0EWork *) w)->flash.radius = 2500.0f;
                    ((Obj0EWork *) w)->flash.goalArg = ((Em *) o->owner)->index;
                    ((Obj0EWork *) w)->flash.timer = 5;
                    ((Obj0EWork *) w)->flash.area = gScene.area;
                    FlashObj_Add((EmObj *)((u8 *) w + 0x16C));
                }
                o->var0++;
                if (gAreaFade == 3) {
                    if (gAreaFadeTimer <= 50) {
                        gAreaFade = 1;
                        gAreaFadeTimer = 2;
                    }
                } else if (gAreaFade == 0) {
                    gAreaFade = 1;
                    gAreaFadeTimer = 2;
                }
            }
            break;
        case 1:
        default:
            ((Obj0EWork *) w)->flash.timer = 5;
            break;
        }
        break;
    case 6:
        limit = 0x24;
        dur = 0x1A;
        break;
    case 8:
        limit = 0x58;
        break;
    case 9:
        dur = limit = 2;
        break;
    case 12:
        em = o->owner;
        if (em == NULL || em->active == 0 || em->taskPhase >= 2) {
            o->state++;
            o->visible = 0;
            return;
        }
        dur = limit = 0x1A;
        m = (f32 *)Em_Bone(em, 0xB);
        v[0] = 0.0f;
        v[1] = 30.0f;
        v[2] = 8.5f;
        Vec_RotateSelf(v, m);
        Mtx_GetTrans(o->pos, m);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        if (o->timer % 25 == 0) {
            Em_PlaySeChecked(o->owner, 0x33, 0, o->pos, 1, 0);
        }
        break;
    }
    o->timer++;
    if (o->type != 3 && o->type != 10 && o->type != 12 && o->timer > limit) {
        o->state++;
        o->visible = 0;
        return;
    }
    for (i = 0; i < n; i++) {
        switch (o->type) {
        case 0:
        case 11:
            j = w->unk06;
            switch (j) {
            case 0:
            case 1:
            case 2:
                k = 0;
                dur = 0x1B;
                break;
            case 3:
                k = 2;
                dur = 8;
                break;
            case 4:
                k = 4;
                dur = 9;
                break;
            case 0xFF:
                w++;
                continue;
            }
            break;
        case 1:
            k = 0x12;
            break;
        case 3:
        case 6:
        case 12:
            k = 0x19;
            break;
        case 4:
            switch (w->unk06) {
            case 0:
                k = 6;
                dur = 8;
                break;
            case 1:
                k = 8;
                dur = 6;
                break;
            case 2:
                k = 0xA;
                dur = 0xB;
                break;
            case 3:
                k = 0xC;
                dur = 0x15;
                break;
            }
            break;
        case 5:
            switch (w->unk06) {
            case 0:
                k = 0xE;
                dur = 0x14;
                break;
            case 1:
            default:
                k = 0x10;
                dur = 0xE;
                break;
            }
            break;
        case 8:
            k = 0x14;
            if (!(w->unk06 & 2)) {
                dur = 0x50;
            } else {
                k = 0x16;
                dur = 0x3C;
            }
            break;
        case 9:
            k = 0x18;
            break;
        case 10:
            k = 0x1A;
            dur = 0x1E;
            break;
        }
        if (w->unk06 != 0xFF && ++w->unk02 <= 0) {
            w++;
            continue;
        }
        if (w->unk06 != 0xFF && w->unk02 > dur) {
            if (o->type == 3 || o->type == 12) {
                StageObj_K0E_InitPart(w);
            } else if (o->type == 10) {
                StageObj_K0E_InitPartAt(o, w);
            }
            w++;
            continue;
        }
        if (o->type != 3 && o->type != 6 && o->type != 12) {
            curve = StageObj_K0E_PartCurves[k++];
            Key_LerpVec3(curve, w->unk0C, w->unk02);
        }
        if (o->type != 9 && w->unk06 != 0xFF) {
            curve = StageObj_K0E_PartCurves[k++];
            Key_LerpF32(curve, &w->unk24, w->unk02);
        }
        if ((o->type == 0 || o->type == 11) && ((u32) w->unk06 <= 1 || w->unk06 == 2)) {
            for (j = 0; StageObj_K0E_ListA[j] != -1; j++) {
                if (w->unk02 == StageObj_K0E_ListA[j]) {
                    w->unk00++;
                    break;
                }
            }
        } else if (o->type == 1) {
            for (j = 0; StageObj_K0E_ListB[j] != -1; j++) {
                if (w->unk02 == StageObj_K0E_ListB[j]) {
                    w->unk00++;
                    break;
                }
            }
        } else if (o->type == 3 || o->type == 6 || o->type == 12) {
            c = w->unk00 & 0xF;
            w->unk00 &= ~0xF;
            c++;
            w->unk00 |= c & 0xF;
        }
        if (w->node != NULL) {
            switch (o->type) {
            case 0:
            case 11:
                Vec_Copy(w->node->pos, o->pos);
                if (w->unk06 <= 2) {
                    v[0] = w->unk18[0];
                    v[1] = w->unk18[1];
                    v[2] = 10.0f + w->unk18[2];
                } else if (w->unk06 == 3) {
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = 20.0f;
                } else {
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = 0.0f;
                }
                Vec_RotateSelf(v, gCamMatrix);
                w->node->pos[0] += v[0];
                w->node->pos[1] += v[1];
                w->node->pos[2] += v[2];
                break;
            case 1:
                Vec_Copy(w->node->pos, o->pos);
                Vec_Rotate(v, w->unk18, gCamMatrix);
                w->node->pos[0] += v[0];
                w->node->pos[1] += v[1];
                w->node->pos[2] += v[2];
                break;
            case 3:
            case 6:
                w->node->pos[0] = o->pos[0] + w->unk18[0];
                w->node->pos[1] = o->pos[1] + w->unk18[1];
                w->node->pos[2] = o->pos[2] + w->unk18[2];
                break;
            case 4:
                if (w->unk06 == 2) {
                    w->unk18[1] -= 5.0f;
                }
            case 5:
                Vec_Copy(w->node->pos, w->unk18);
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 10.0f + (15.0f - 5.0f * i);
                Vec_RotateSelf(v, gCamMatrix);
                w->node->pos[0] += v[0];
                w->node->pos[1] += v[1];
                w->node->pos[2] += v[2];
                break;
            case 8:
                w->unk18[1] += w->unk2C;
                w->unk08 += w->unk0A;
                Vec_Rotate(v, w->unk18, gCamMatrix);
                w->node->pos[0] = o->pos[0] + v[0];
                w->node->pos[1] = o->pos[1] + v[1];
                w->node->pos[2] = o->pos[2] + v[2];
                break;
            case 9:
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = i * 5 + 25;
                Vec_RotateSelf(v, gCamMatrix);
                w->node->pos[0] = o->pos[0] + v[0];
                w->node->pos[1] = o->pos[1] + v[1];
                w->node->pos[2] = o->pos[2] + v[2];
                break;
            case 10:
                w->unk18[1] += 2.0f * o->scale;
                w->unk08 += w->unk0A;
                w->node->pos[0] = w->unk18[0];
                w->node->pos[1] = w->unk18[1];
                w->node->pos[2] = w->unk18[2];
                break;
            case 12:
                w->unk24 *= ((Em *) o->owner)->alpha;
                w->node->pos[0] = o->pos[0] + w->unk18[0];
                w->node->pos[1] = o->pos[1] + w->unk18[1];
                w->node->pos[2] = o->pos[2] + w->unk18[2];
                break;
            }
            OT_AddByDepth(gDrawOtSorted, w->node, 0x40, 0);
        }
        w++;
    }
}

void StageObj_K0E_RunModel(StageObj *o)
{
    o->timer += (Rand_Next(1) & 1) + 1;
    if (o->timer > 0x10) {
        o->timer = 1;
    }
    OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
}

void StageObj_K0E_End(StageObj *o)
{
    u8 *p = o->buf;
    s16 n;
    s16 i;

    o->state++;
    o->visible = 0;
    if (o->type == 5) {
        FlashObj_Remove((EmObj *)(p + 0x16C));
    }
    n = StageObj_K0E_PartCounts[o->type];
    if (n == 0) {
        DrawSlot_Free(o->slot);
    } else {
        for (i = 0; i < n; i++) {
            if (*(s32 *) (p + 0x30) != 0) {
                DrawSlot_Free(*(s16 *) (p + 4));
            }
            p += 0x34;
        }
    }
}

void StageObj_K0E_Free(StageObj *o)
{
    StageObj_Free(o);
}

/* the area's fade (sub_main), named: its load in case 5 moves above the scale[0] stack store,
   which the absolute *(s16 *) 0x5BACE0 doesn't */
extern s16 gAreaFade;

inline f32 Lerp(u32 a, u32 b, f32 t)
{
    s16 d = b - a;

    return a + d * t;
}

void StageObj_K0E_Draw(DrawSlot *node)
{
    u8 a;
    u8 g;
    ModelMat *mats;
    s16 id;
    u16 mode = 0;
    u8 r;
    Model *set = gStageModelSet;
    StageObj *o = node->owner;
    Obj0EPart *p = (Obj0EPart *) o->buf + node->part;
    u16 order = 0;
    u8 b;
    s16 d;
    s16 x;
    Rgb4 *c;
    Rgb4 *c0;
    f32 t;
    f32 rot[3];
    f32 scale[3];
    f32 m[16];
    f32 tm[16];

    if (set != NULL && set->active) {
        mats = set->mats;
        r = 0xFF;
        g = 0xFF;
        b = 0xFF;
        a = 255.0f * p->unk24;
        switch (o->type) {
        case 0:
        case 11:
            Gs_SetState(0x6C, 0);
            mode |= 2;
            scale[0] = o->scale * p->unk0C[0];
            scale[1] = o->scale * p->unk0C[1];
            scale[2] = o->scale * p->unk0C[2];
            switch (p->unk06) {
            case 0:
            case 1:
            case 2:
                id = 0x4E;
                Mtx_Translation(tm, 0.25f * (p->unk00 & 3), 0.25f * (p->unk00 / 4), 0.0f);
                Gs_SetState(0x19, (s32) tm);
                break;
            case 3:
                id = 0x50;
                break;
            case 4:
                id = 0x58;
                break;
            }
            rot[2] = (6.2831855f * p->unk08) / 65536.0f;
            order = 3;
            break;
        case 1:
            scale[0] = p->unk28 * p->unk0C[0];
            id = 0x4E;
            mode |= 2;
            scale[1] = p->unk28 * p->unk0C[1];
            scale[2] = p->unk28 * p->unk0C[2];
            Mtx_Translation(tm, 0.25f * (p->unk00 & 3), 0.25f * (p->unk00 >> 2), 0.0f);
            Gs_SetState(0x19, (s32) tm);
            rot[2] = (6.2831855f * p->unk08) / 65536.0f;
            order = 3;
            break;
        case 2:
            id = 0x15;
            scale[0] = o->scale;
            scale[1] = o->scale;
            scale[2] = o->scale;
            Gs_SetState(0x6C, 0);
            mode |= 2;
            a = 255.0f * (0.05f + 0.025f * (1.0f + Math_Sin((6.2831855f * (360.0f * (o->timer / 16.0f) - 90.0f)) / 360.0f)));
            break;
        case 3:
        case 6:
        case 12:
            id = 0x60;
            scale[0] = p->unk0C[0];
            scale[1] = p->unk0C[1];
            scale[2] = p->unk0C[2];
            Gs_SetState(0x6C, 0);
            mode |= 2;
            Mtx_Translation(tm, 0.0625f * (p->unk00 & 7), 0.0625f * (p->unk00 >> 3), 0.0f);
            Gs_SetState(0x19, (s32) tm);
            r = 0xFF;
            g = 0x5F;
            b = 0;
            break;
        case 4:
            scale[0] = p->unk0C[0];
            scale[1] = p->unk0C[1];
            scale[2] = p->unk0C[2];
            switch (p->unk06) {
            case 0:
            case 1:
                mode |= 2;
                id = 0x15;
                break;
            case 2:
                id = 0x23;
                break;
            case 3:
                id = 0x48;
                scale[0] *= 2.0f;
                scale[1] *= 2.0f;
                scale[2] *= 2.0f;
                break;
            }
            rot[2] = (6.2831855f * p->unk08) / 65536.0f;
            order = 3;
            break;
        case 5:
            scale[0] = p->unk0C[0];
            x = gAreaFade;
            scale[1] = p->unk0C[1];
            scale[2] = p->unk0C[2];
            id = node->part + 0x51;
            if (x == 1 || x == 2) {
                mode |= 1;
            } else if (p->unk06 < 2) {
                mode |= 2;
            }
            break;
        case 8:
            Gs_SetState(0x6C, 0);
            scale[0] = p->unk0C[0] * (o->scale * p->unk28);
            scale[1] = p->unk0C[1] * (o->scale * p->unk28);
            scale[2] = p->unk0C[2] * (o->scale * p->unk28);
            if (!(p->unk06 & 1)) {
                id = 0x4D;
                Mtx_Translation(tm, 0.0f, 0.25f, 0.0f);
            } else {
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
                id = 0x4F;
            }
            Gs_SetState(0x19, (s32) tm);
            rot[2] = (6.2831855f * p->unk08) / 65536.0f;
            order = 3;
            break;
        case 9:
            scale[0] = 3.0f * o->scale * p->unk0C[0];
            scale[1] = 3.0f * o->scale * p->unk0C[1];
            scale[2] = 3.0f * o->scale * p->unk0C[2];
            Mtx_Translation(tm, 0.25f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            id = 0x22;
            mode |= 2;
            rot[2] = (6.2831855f * p->unk08) / 65536.0f;
            order = 3;
            break;
        case 10:
            scale[0] = p->unk0C[0];
            scale[1] = p->unk0C[1];
            scale[2] = p->unk0C[2];
            id = StageObj_K0E_Ids[p->unk06];
            Gs_SetState(0x6C, 0);
            c = &StageObj_K0E_Colors[o->var0];
            if (o->var0 != 0 && o->angle < 16) {
                c0 = &StageObj_K0E_Colors[o->var0 - 1];
                t = o->angle / 16.0f;
                d = c->r - c0->r;
                r = c0->r + d * t;
                d = c->g - c0->g;
                g = c0->g + d * t;
                d = c->b - c0->b;
                b = c0->b + d * t;
                a = p->unk24 * Lerp(c0->unk3, c->unk3, t);
            } else {
                r = c->r;
                g = c->g;
                b = c->b;
                a = p->unk24 * c->unk3;
            }
            break;
        }
        Gs_SetState(0x60, 0);
        Mtx_FromScaleRotTrans(scale, rot, node->pos, order, m);
        Mtx_Concat3x3(m, gCamMatrix);
        Effect_DrawModel(&set->matSets[id], m, b | ((g << 8) | ((a << 24) | (r << 16))), mode, mats);
        Gfx_SetBlend(4, 5);
        Gs_SetState(0x6C, 1);
    }
}

void StageObj_K0E_CreateScaled(f32 *pos, s16 k, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0xE;
        o->update = StageObj_K0E_Update;
        o->type = k;
        o->owner = NULL;
        o->scale = scale;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        if (k < 5) {
            o->angle = Rand_Next(1);
        }
        if (k == 0) {
            StageObj_K0E_CreateScaled(pos, 8, scale);
        }
        if (k == 0xB) {
            o->scale *= 0.7f;
        }
    }
}

void StageObj_K0E_CreateAt(f32 *pos, s16 k)
{
    StageObj *o;

    if (k == 2) {
        o = StageObj_Alloc(0);
    } else {
        o = StageObj_Alloc(1);
    }
    if (o != NULL) {
        o->kind = 0xE;
        o->update = StageObj_K0E_Update;
        o->type = k;
        o->owner = NULL;
        if (k == 2) {
            o->scale = 2.0f;
        } else {
            o->scale = 1.0f;
        }
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
    }
}

void StageObj_K0E_CreateOwned(f32 *pos, s16 k, f32 scale, void *owner)
{
    StageObj *o;

    if (Obj_InCurrentArea(owner) & 0xFF) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            o->kind = 0xE;
            o->update = StageObj_K0E_Update;
            o->type = k;
            o->owner = owner;
            o->scale = scale;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            if (k < 5) {
                o->angle = Rand_Next(1);
            }
        }
    }
}

void StageObj_K0E_CreateOnEm(Em *em, s16 k)
{
    StageObj *o;
    f32 v[3];

    if (Obj_InCurrentArea(em) & 0xFF) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            o->kind = 0xE;
            o->update = StageObj_K0E_Update;
            o->type = k;
            o->owner = em;
            o->scale = 1.0f;
            if (k == 6) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 128.0f;
                Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
                o->pos[0] = em->posX + v[0];
                o->pos[1] = em->floorY + v[1];
                o->pos[2] = em->posZ + v[2];
            }
        }
    }
}

void StageObj_K0E_CreateScaledB(f32 *pos, s16 k, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0xE;
        o->update = StageObj_K0E_Update;
        o->type = k;
        o->owner = NULL;
        o->scale = scale;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        if (k < 5) {
            o->angle = Rand_Next(1);
        }
        if (k == 0) {
            StageObj_K0E_CreateScaled(pos, 8, scale);
        }
    }
}

void StageObj_K0F_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K0F_Setup(o);
        break;
    case 1:
        StageObj_K0F_Run(o);
        break;
    case 2:
        StageObj_K0F_End(o);
        break;
    case 3:
        StageObj_K0F_Free(o);
        break;
    }
}

void StageObj_K0F_Setup(StageObj *o)
{
    s16 i;
    Obj0FPart *p = (Obj0FPart *) o->buf;
    s16 n;

    o->state++;
    o->var0 = 0;
    o->var1 = 0;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    n = StageObj_K0F_PartCounts[o->type];
    switch (o->type) {
    case 4:
        StageObj_K0F_PlaySound(o);
        break;
    }
    for (i = 0; i < n; p++, i++) {
        p->slot = DrawSlot_Alloc();
        p->kind = i;
        p->scale = o->scale;
        p->unk24 = 0;
        switch (o->type) {
        case 0:
            p->unk26 = Rand_Next(1);
            p->unk24 = StageObj_K0F_DelaysA[i] - 4;
            Vec_Copy(p->pos, o->pos);
            switch (p->kind) {
            case 0:
                p->unk28 = (Rand_Next(1) & 0x3FF) - 0x200;
                break;
            case 2:
                p->unk28 = (Rand_Next(1) & 0x7FF) - 0x400;
                break;
            default:
                p->unk28 = 0;
                break;
            }
            break;
        case 1:
        case 6:
            p->unk26 = Rand_Next(1);
            p->unk24 = i * -5 - 5;
            Vec_Copy(p->pos, o->pos);
            p->scale *= 1.0f - 0.2f * i;
            p->unk28 = (Rand_Next(1) & 0xFF) - 0x80;
            break;
        case 2:
            p->unk26 = Rand_Next(1);
            if (o->var2 != 1) {
                p->unk24 = StageObj_K0F_DelaysC[i] - 5;
            }
            Vec_Copy(p->pos, o->pos);
            switch (p->kind) {
            case 0:
            case 1:
                p->scale *= 0.75f;
                p->unk28 = 0;
                break;
            case 2:
            default:
                p->unk28 = 0;
                break;
            }
            break;
        case 3:
        case 7:
        case 8:
            if (o->var2 != 1) {
                p->unk24 -= 5;
            }
            p->unk26 = Rand_Next(1);
            Vec_Copy(p->pos, o->pos);
            if (o->type == 8 && p->kind == 1) {
                p->scale *= 1.3f;
            }
            break;
        case 4:
        case 5:
            p->unk24 = StageObj_K0F_DelaysB[i];
            switch (i) {
            case 5:
            case 6:
                p->unk26 = Rand_Next(1);
                break;
            default:
                p->unk26 = 0;
                break;
            }
            Vec_Copy(p->pos, o->pos);
            break;
        case 9:
            p->kind = 1;
            p->unk26 = Rand_Next(1);
            Vec_Copy(p->pos, o->pos);
            break;
        }
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K0F_Draw;
        } else {
            p->node = NULL;
        }
    }
}

void StageObj_K0F_Run(StageObj *o)
{
    Obj0FPart *p = (Obj0FPart *) o->buf;
    s16 i;
    Em *em = o->owner;
    s16 n;
    s16 limit;
    s16 step;
    s16 dur;
    s16 c;
    s16 *durs;
    s16 bone;
    s32 ci;
    void *curve;
    f32 a;
    f32 d[3];
    f32 v[3];

    n = StageObj_K0F_PartCounts[o->type];
    limit = StageObj_K0F_Limits[o->type];
    step = StageObj_K0F_Steps[o->type];
    c = StageObj_K0F_CurveIdx[o->type];
    dur = StageObj_K0F_Durations[o->type];
    durs = StageObj_K0F_DurLists[o->type];
    switch (o->type) {
    case 4:
        limit += (s16) (o->var2 * 10);
        break;
    case 5:
        if (em->active == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        bone = StageObj_K0F_OwnerBone(o);
        if (bone < 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        Em_BonePos(em, bone, o->pos);
        limit += (s16) (o->var2 * 10);
        break;
    default:
        if (o->timer == 5) {
            StageObj_K0F_PlaySound(o);
        }
        break;
    }
    if (++o->timer > limit) {
        o->state++;
        o->visible = 0;
        return;
    }
    for (i = 0; i < n; i++, p++) {
        if (durs == NULL) {
            c = StageObj_K0F_CurveIdx[o->type];
        } else {
            dur = durs[i];
        }
        d[0] = 0.0f;
        d[1] = 0.0f;
        d[2] = 0.0f;
        if (++p->unk24 <= 0) {
            c += step;
            continue;
        }
        if (p->unk24 > dur) {
            if (o->type == 4 || o->type == 5) {
                if ((s16) StageObj_K0F_RestartPart(o, p) == 0) {
                    c += step;
                    continue;
                }
            } else {
                c += step;
                continue;
            }
        }
        switch (o->type) {
        case 0:
            ci = c;
            curve = StageObj_K0F_Curves[ci];
            Key_LerpVec3(curve, p->sc, p->unk24);
            c += 2;
            Key_LerpF32(StageObj_K0F_Curves[(s16) (ci + 1)], &a, p->unk24);
            p->alpha = 255.0f * a;
            switch (p->kind) {
            case 0:
            case 2:
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 100.0f / dur;
                Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
                p->pos[0] += v[0];
                p->pos[1] += v[1];
                p->pos[2] += v[2];
                break;
            }
            p->unk26 += p->unk28;
            break;
        case 1:
        case 6:
            curve = StageObj_K0F_Curves[c++];
            Key_LerpVec3(curve, p->sc, p->unk24);
            p->unk26 += p->unk28;
            p->pos[1] += 60.0f / dur;
            d[2] = 10.0f * i;
            Vec_RotateSelf(d, gCamMatrix);
            break;
        case 2:
            curve = StageObj_K0F_Curves[c++];
            Key_LerpVec3(curve, p->sc, p->unk24);
            p->unk26 += p->unk28;
            break;
        case 3:
        case 7:
        case 8:
        case 9:
            ci = c;
            curve = StageObj_K0F_Curves[ci];
            Key_LerpVec3(curve, p->sc, p->unk24);
            c += 2;
            Key_LerpF32(StageObj_K0F_Curves[(s16) (ci + 1)], &a, p->unk24);
            p->alpha = 255.0f * a;
            break;
        case 5:
            Vec_Copy(p->pos, o->pos);
        case 4:
            ci = c;
            curve = StageObj_K0F_Curves[ci];
            Key_LerpVec3(curve, p->sc, p->unk24);
            c += 2;
            Key_LerpF32(StageObj_K0F_Curves[(s16) (ci + 1)], &a, p->unk24);
            p->alpha = 255.0f * a;
            d[0] = 0.0f;
            d[1] = 0.0f;
            d[2] = 20.0f - 5.0f * i;
            Vec_RotateSelf(d, gCamMatrix);
            break;
        }
        if (p->node != NULL) {
            p->node->pos[0] = d[0] + p->pos[0];
            p->node->pos[1] = d[1] + p->pos[1];
            p->node->pos[2] = d[2] + p->pos[2];
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
        }
    }
}

void StageObj_K0F_End(StageObj *o)
{
    s16 n;
    s16 i;
    Obj0FPart *p = (Obj0FPart *) o->buf;

    o->state++;
    n = StageObj_K0F_PartCounts[o->type];
    for (i = 0; i < n; i++) {
        if (p->node != NULL) {
            DrawSlot_Free(p->slot);
        }
        p++;
    }
}

void StageObj_K0F_Free(StageObj *o)
{
    StageObj_Free(o);
}

/* Draws one part of a kind 0x0F stage object. The kind is read into a local once for the
 * switch and its tests (type), while case 1/6 reads o->type again for the colour keys. */
void StageObj_K0F_Draw(DrawSlot *node)
{
    f32 m[16];
    u32 type;
    f32 tm[16];
    f32 scale[3];
    u32 color;
    StageObj *o = node->owner;
    Model *set = gStageModelSet;
    Obj0FPart *p = (Obj0FPart *) o->buf + node->part;
    f32 rot[3];
    ModelMat *mats;
    ModelMatSet *model;
    u16 mode = 0;
    s32 order;
    s32 t;

    if (set != NULL && set->active != 0) {
        mats = set->mats;
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        type = o->type;
        switch (type) {
        case 0:
            switch (p->kind) {
            case 0:
                model = &set->matSets[54];
                mode |= 2;
                t = p->unk24 - 1;
                Mtx_Translation(tm, 0.25f * (t & 3), 0.25f * (t >> 2), 0.0f);
                break;
            case 1:
                mode |= 2;
                model = &set->matSets[66];
                Mtx_Translation(tm, 0.0f, 0.75f, 0.0f);
                break;
            case 2:
                model = &set->matSets[94];
                Mtx_Translation(tm, 0.0f, 0.75f, 0.0f);
                break;
            }
            Gs_SetState(0x19, (s32) tm);
            rot[2] = (6.2831855f * p->unk26) / 65536.0f;
            order = 3;
            color = (p->alpha << 24) | 0xFFFFFF;
            break;
        case 1:
        case 6:
            model = &set->matSets[72];
            rot[2] = (6.2831855f * p->unk26) / 65536.0f;
            order = 3;
            Color_EvalKeys((s32 *) (o->type == 1 ? StageObj_K0F_ColorKeys1 : StageObj_K0F_ColorKeys), p->unk24, &color);
            break;
        case 2:
            switch (p->kind) {
            case 0:
                model = &set->matSets[p->unk24];
                Mtx_Translation(tm, 0.5f, 0.124f, 0.0f);
                color = -1;
                break;
            case 1:
                model = set->matSets + p->unk24 + 27;
                Mtx_Translation(tm, 0.5f, 0.004f, 0.0f);
                color = -1;
                break;
            case 2:
            default:
                model = &set->matSets[94];
                Mtx_Translation(tm, 0.75f, 0.75f, 0.0f);
                Color_EvalKeys((s32 *) StageObj_K0F_ColorKeysB, p->unk24, &color);
                break;
            }
            Gs_SetState(0x6C, 0);
            Gs_SetState(0x19, (s32) tm);
            rot[2] = (6.2831855f * p->unk26) / 65536.0f;
            order = 3;
            break;
        case 3:
        case 7:
        case 8:
        case 9:
            if (type == 8) {
                mode |= 8;
            } else {
                mode |= 2;
            }
            if (p->kind == 0) {
                model = &set->matSets[66];
                if (type == 3 || type == 8 || type == 9) {
                    Mtx_Translation(tm, 0.25f, 0.75f, 0.0f);
                } else {
                    Mtx_Translation(tm, 0.0f, 0.75f, 0.0f);
                }
            } else {
                model = &set->matSets[75];
                if (type == 3 || type == 8 || type == 9) {
                    Mtx_Translation(tm, 0.0f, 0.046875f * p->unk24, 0.0f);
                } else {
                    Mtx_Translation(tm, 0.0625f, 0.5f + 0.046875f * p->unk24, 0.0f);
                }
            }
            Gs_SetState(0x6C, 0);
            Gs_SetState(0x19, (s32) tm);
            rot[2] = (6.2831855f * p->unk26) / 65536.0f;
            order = 3;
            color = (p->alpha << 24) | 0xFFFFFF;
            break;
        case 4:
        case 5:
            switch (p->kind) {
            case 0:
                if (type == 5) {
                    return;
                }
                model = &set->matSets[0];
                mode |= 2;
                Mtx_Translation(tm, 0.125f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                break;
            case 1:
                model = &set->matSets[64];
                break;
            case 2:
            case 3:
            case 4:
                model = &set->matSets[88];
                break;
            case 5:
            case 6:
                model = &set->matSets[35];
                break;
            }
            rot[2] = (6.2831855f * p->unk26) / 65536.0f;
            order = 3;
            color = (p->alpha << 24) | 0xFFFFFF;
            break;
        }
        Mtx_FromScaleRotTrans(scale, rot, node->pos, order, m);
        Mtx_Concat3x3(m, gCamMatrix);
        Effect_DrawModel(model, m, color, mode, mats);
        Gs_SetState(0x6C, 1);
    }
}

s32 StageObj_K0F_RestartPart(StageObj *o, Obj0FPart *w)
{
    if (o->var1 >= o->var2) {
        return 0;
    }
    switch (w->kind) {
    case 3:
    case 4:
        if (o->var1 == 0) {
            w->unk24 = -0xE;
        } else {
            w->unk24 = -6;
        }
        break;
    case 6:
        w->unk26 = Rand_Next(1);
        w->unk24 = 1;
        o->var1++;
        return 1;
    }
    return 0;
}

s32 StageObj_K0F_OwnerBone(StageObj *o)
{
    Em *em = o->owner;

    switch (em->type) {
    case 0xB:
    case 0x29:
    case 0x31:
    case 0x1:
    case 0x2A:
    case 0x25:
        if (!((em->motionId[0] == 0x41E && EmMotion_ReachedFrame(em, 0, 110.0f)) ||
            (em->motionId[0] == 0x41B && EmMotion_ReachedFrame(em, 0, 160.0f)))) {
            return -1;
        }
        return 0x22;
    case 0x11:
    case 0x2F:
    case 0x16:
        if (!((em->motionId[0] == 0x41E && EmMotion_ReachedFrame(em, 0, 110.0f)) ||
            (em->motionId[0] == 0x452 && EmMotion_ReachedFrame(em, 0, 110.0f)))) {
            return -1;
        }
        return 0x22;
    case 0xE:
    case 0x2B:
    case 0x1A:
    case 0x2C:
        if (!((em->motionId[0] == 0x457 && EmMotion_ReachedFrame(em, 0, 112.0f)) ||
            (em->motionId[0] == 0x41E && EmMotion_ReachedFrame(em, 0, 110.0f)) ||
            (em->motionId[0] == 0x41B && EmMotion_ReachedFrame(em, 0, 160.0f)))) {
            return -1;
        }
        return 0x22;
    case 0x2:
    case 0x24:
    case 0x47:
        if (!(em->motionId[0] == 0x408 && EmMotion_ReachedFrame(em, 0, 66.0f))) {
            return -1;
        }
        return 0x33;
    case 0xF:
    case 0x2D:
        if (!((em->motionId[0] == 0x451 && EmMotion_ReachedFrame(em, 0, 66.0f)) ||
            (em->motionId[0] == 0x41E && EmMotion_ReachedFrame(em, 0, 78.0f)))) {
            return -1;
        }
        return 0x22;
    case 0x7:
    case 0x32:
        if (!(em->motionId[0] == 0x3EF && EmMotion_ReachedFrame(em, 0, 4.0f))) {
            return -1;
        }
        return 0x25;
    case 0x28:
        if (!(em->motionId[0] == 0x410 && EmMotion_ReachedFrame(em, 0, 60.0f))) {
            return -1;
        }
        return 0x23;
    case 0x33:
    case 0x35:
        if (!((em->motionId[0] == 0x41C && EmMotion_ReachedFrame(em, 0, 38.0f)) ||
            (em->motionId[0] == 0x423 && EmMotion_ReachedFrame(em, 0, 64.0f)))) {
            return -1;
        }
        return 0x13;
    case 0x34:
        if (!(em->motionId[0] == 0x43E && EmMotion_ReachedFrame(em, 0, 100.0f))) {
            return -1;
        }
        return 0x13;
    case 0x36:
    case 0x3C:
    case 0x40:
    case 0x41:
        if (!(em->motionId[0] == 0x408 && EmMotion_ReachedFrame(em, 0, 62.0f))) {
            return -1;
        }
        return 0x27;
    }
    return -1;
}

void StageObj_K0F_PlaySound(StageObj *o)
{
    switch (o->type) {
    case 0:
        Sound_PlaySeAtPosChecked(1, 0x5E, 0, o->pos, 1, 0);
        break;
    case 2:
        if (o->var2 != 1) {
            Sound_PlaySeAtPosChecked(1, 0x5F, 0, o->pos, 1, 0);
        }
        break;
    case 3:
    case 7:
        if (o->var2 != 1) {
            Sound_PlaySeAtPosChecked(1, 0x61, 0, o->pos, 1, 0);
        }
        break;
    case 8:
        if (o->var2 != 1) {
            Sound_PlaySeAtPosChecked(1, 0x62, 0, o->pos, 1, 0);
        }
        break;
    case 4:
        if (o->angle == 0) {
            Sound_PlaySeAtPosChecked(1, 0x75, 0, o->pos, 6, 0);
        }
        break;
    case 1:
    case 5:
    case 6:
        break;
    }
}

void StageObj_K0F_Create(f32 *pos, s32 k, u16 v, void *owner, f32 scale)
{
    StageObj *o;

    if (owner == NULL || (Obj_InCurrentArea(owner) & 0xFF)) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            o->kind = 0xF;
            o->update = StageObj_K0F_Update;
            o->type = k;
            Vec_Copy(o->pos, pos);
            o->angle = v;
            o->var2 = 0;
            o->owner = owner;
            o->scale = scale;
        }
    }
}

StageObj *StageObj_K0F_CreateAt(f32 *pos, s32 k, s32 c, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_Alloc(1)) == NULL) {
        return NULL;
    }
    o->kind = 0xF;
    o->update = StageObj_K0F_Update;
    o->type = k;
    Vec_Copy(o->pos, pos);
    o->var2 = c;
    o->owner = NULL;
    o->angle = 0;
    o->scale = scale;
    return o;
}

void StageObj_K0F_CreateAtMarked(f32 *pos, s32 k, s32 c, f32 scale)
{
    StageObj *o;

    o = StageObj_K0F_CreateAt(pos, k, c, scale);
    if (o != NULL) {
        o->angle = 1;
    }
}

void StageObj_K0F_CreateOnEm(Em *em, u8 k, u8 c, f32 scale)
{
    StageObj *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0xF;
            o->update = StageObj_K0F_Update;
            o->type = k;
            o->var2 = c;
            o->owner = em;
            o->scale = scale;
        }
    }
}

void StageObj_K0F_CreateNoOwner(f32 *pos, u8 k, u8 c, u16 v, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0xF;
        o->update = StageObj_K0F_Update;
        o->type = k;
        Vec_Copy(o->pos, pos);
        o->angle = v;
        o->var2 = c;
        o->owner = NULL;
        o->scale = scale;
    }
}

void StageObj_K10_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K10_Setup(o);
        break;
    case 1:
        StageObj_K10_Run(o);
        break;
    case 2:
        StageObj_K10_End(o);
        break;
    case 3:
        StageObj_K10_Free(o);
        break;
    }
}

void StageObj_K10_Setup(StageObj *o)
{
    Obj10Part *p = (Obj10Part *) o->buf;
    s16 n;
    s16 i;
    f32 pos[3];

    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    n = StageObj_K10_PartCounts[o->type];
    switch (o->type) {
    case 2:
    case 5:
    case 6:
        StageObj_K10_PlaySound(o, o->pos);
        break;
    case 8:
        if (o->var2 != 0xFF) {
            Em_GetBonePos(o->owner, o->var2, pos);
            StageObj_K10_PlaySound(o, pos);
        }
        break;
    case 3:
    case 4:
        return;
    }
    for (i = 0; i < n; p++, i++) {
        switch (o->type) {
        case 0:
            p->idx = i;
            if (!(o->var0 & 0x20)) {
                p->delay = -10 - (Rand_Next(1) & 7);
            } else {
                p->delay = 0;
            }
            p->scale = o->scale;
            p->unk28 = -0.8f;
            break;
        case 2:
            p->delay = -i;
            p->idx = 0;
            p->alpha = 0xFF;
            p->scale = o->scale;
            p->angle = Rand_Next(1);
            break;
        case 5:
            if (i < 3) {
                p->idx = 0;
                p->delay = -i;
                p->scale = o->scale;
                p->angle = o->angle + (i * 0x2AAA + Rand_Next(1) / 3);
            } else {
                p->idx = 1;
                p->delay = 0;
                p->scale = o->scale;
                p->angle = Rand_Next(1);
            }
            p->alpha = 0xFF;
            break;
        case 6:
            p->delay = 0;
            p->idx = i;
            if (i == 0) {
                p->alpha = 0xFF;
            }
            p->scale = o->scale;
            break;
        case 8:
            p->idx = i;
            p->delay = i * -4 - 10;
            p->scale = o->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            p->unk30 = 0;
            break;
        case 9:
            p->idx = Rand_Next(1) & 7;
            p->delay = i * -12 - 10;
            p->scale = o->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            break;
        case 12:
            p->idx = Rand_Next(1) & 7;
            p->delay = i * -10;
            p->scale = o->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            break;
        case 13:
            p->idx = Rand_Next(1) & 7;
            p->delay = -o->var1 * i;
            p->scale = o->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            break;
        case 14:
            p->idx = i;
            p->unk30 = Rand_Next(1) & 3;
            p->angle = Rand_Next(1);
            p->delay = -o->var1;
            p->scale = o->scale;
            p->alpha = 0xFF;
            break;
        case 15:
            p->idx = i;
            p->delay = 0;
            p->scale = o->scale * (0.5f + 0.5f * (0.001f * (Rand_Next(1) & 0x3FF)));
            p->unk30 = 0;
            break;
        }
        p->pos[0] = o->pos[0];
        p->pos[1] = o->pos[1];
        p->pos[2] = o->pos[2];
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K10_Draw;
        } else {
            p->node = NULL;
        }
    }
}

void StageObj_K10_Run(StageObj *o)
{
    s16 k;
    f32 off[3];
    f32 pos[3];
    f32 m[16];
    Obj10Part *p;
    K10Step *tbl;
    s16 n;
    s16 life;
    f32 a;
    s16 end;
    s16 stride;
    s16 i;
    s16 t;
    s16 j;
    s16 found;
    s32 ci;
    s32 cj;
    s32 si;
    void *curve;
    f32 sc;
    f32 x;
    f32 y;

    p = (Obj10Part *) o->buf;
    n = StageObj_K10_PartCounts[o->type];
    life = StageObj_K10_Lives[o->type];
    k = StageObj_K10_CurveIdx[o->type];
    end = StageObj_K10_Ends[o->type];
    stride = StageObj_K10_Strides[o->type];
    switch (o->type) {
    case 13:
        life = o->angle;
        break;
    case 14:
        life = o->angle;
        if (life < 0) {
            life = 1000;
            o->timer = 0;
            end = life;
        } else {
            end = life;
        }
        break;
    }
    if (++o->timer > life) {
        o->state++;
        o->visible = 0;
        return;
    }
    if (o->var2 != 0xFF
        && (((Em *) o->owner)->taskPhase == 3 || ((Em *) o->owner)->active == 0)) {
        switch (o->type) {
        case 0:
        case 8:
        case 9:
            for (i = 0; i < n; i++) {
                p[i].idx = 0xFF;
            }
            break;
        case 3:
        case 4:
            o->state++;
            o->visible = 0;
            return;
        }
    }
    if (o->type == 0 && p->idx != 0xFF) {
        if ((o->var0 & 0x20) == 0) {
            t = o->timer - 10;
        } else {
            t = o->timer;
        }
        cj = o->var0 & 0xF0;
        switch (cj) {
        default:
            tbl = StageObj_K10_TblA;
            break;
        case 0x10:
            tbl = StageObj_K10_TblB;
            break;
        case 0x20:
            tbl = StageObj_K10_TblC;
            break;
        }
        j = 0;
        found = 0;
        for (; tbl[j].t != -1; j++) {
            if (t == tbl[j].t) {
                found = 1;
                break;
            }
        }
        if (found == 1) {
            if (o->var2 != 0xFF) {
                Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, o->var2)));
                Vec_Rotate(off, o->pos, m);
                Mtx_GetTrans(pos, m);
                pos[0] += off[0];
                pos[1] += off[1];
                pos[2] += off[2];
                if (j == 0 && ((Em *) o->owner)->isEm == 1 && (o->var0 & 0xF0) != 0x20) {
                    StageObj_K10_PlaySound(o, pos);
                }
                sc = 0.1f * tbl[j].v;
                ci = o->var0 & 0xF;
                sc *= o->scale * StageObj_K10_Scales[ci];
                if (ci == 0) {
                    sc *= Effect_Scale();
                }
                switch (o->var0 & 0xF0) {
                case 0:
                    Effect02_SpawnForOwner(o->owner, (u16) o->angle, 3, 2, pos, sc);
                    break;
                case 0x20:
                    Effect02_SpawnForOwner(o->owner, (u16) o->angle, 3, 0, pos, sc);
                    break;
                case 0x10:
                    Effect02_SpawnForOwner(o->owner, (u16) o->angle, 3, 3, pos, sc);
                    break;
                }
            } else {
                Vec_Copy(pos, o->pos);
            }
        }
    }
    for (i = 0; i < n; i++) {
        switch (o->type) {
        case 0:
        case 8:
        case 9:
        case 12:
        case 13:
        case 15:
            k = StageObj_K10_CurveIdx[o->type];
            break;
        case 2:
            k = StageObj_K10_CurveIdx[o->type];
            if (i == 2) {
                end = 4;
                k += stride;
            }
            break;
        case 5:
            if (p->idx == 0) {
                end = 5;
                k = StageObj_K10_CurveIdx[o->type];
            } else {
                end = 5;
                k = stride + StageObj_K10_CurveIdx[o->type];
            }
            break;
        case 6:
            if (i != 0) {
                stride = 2;
                end = 5;
            }
            break;
        case 14:
            if (o->angle < 0) {
                p->delay = 0;
            }
            break;
        }
        if (++p->delay <= 0) {
            p++;
            k += stride;
            continue;
        }
        if (p->delay == 1 && o->type != 2 && o->type != 5 && o->type != 6 && o->type != 14
            && o->type != 15) {
            if (o->var2 != 0xFF) {
                if (p->idx == 0xFF) {
                    p++;
                    continue;
                }
                Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, o->var2)));
                Vec_Rotate(off, o->pos, m);
                Mtx_GetTrans(pos, m);
                if (o->type == 0) {
                    if (p->node != NULL) {
                        p->node->pos[0] = pos[0] + off[0];
                        p->node->pos[1] = pos[1] + off[1];
                        p->node->pos[2] = pos[2] + off[2];
                    }
                    off[0] = 0.02f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    off[1] = 0.02f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    off[2] = 30.0f + 0.005f * (Rand_Next(1) & 0x3FF);
                    x = off[0];
                    y = off[1];
                    Effect_ScaleVec(off);
                    off[0] = x;
                    off[1] = y;
                    p->unk30 = Rand_Next(1) & 3;
                    p->angle = Rand_Next(1);
                    Vec_Copy(p->pos, off);
                    Vec_RotateY(p->pos, (6.2831855f * o->angle) / 65536.0f);
                } else if (o->type == 8 || o->type == 9) {
                    p->pos[0] = pos[0] + off[0];
                    p->pos[1] = pos[1] + off[1];
                    p->pos[2] = pos[2] + off[2];
                    if (o->type == 8) {
                        off[0] = 0.04f * ((Rand_Next(1) & 0x3FF) - 0x200);
                        off[1] = 0.02f * ((Rand_Next(1) & 0x3FF) - 0x200);
                        off[2] = 3.0f;
                        p->pos[0] += off[0];
                        p->pos[1] += off[1];
                        p->pos[2] += off[2];
                    }
                } else {
                    p->pos[0] = pos[0] + off[0] + 0.004f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    p->pos[1] = 10.0f + (pos[1] + off[1] + 0.004f * ((Rand_Next(1) & 0x3FF) - 0x200));
                    p->pos[2] = pos[2] + off[2] + 0.004f * ((Rand_Next(1) & 0x3FF) - 0x200);
                }
            }
        } else if (p->delay > end) {
            if (o->type == 13) {
                p->delay = -o->var1 * 9 + 60;
                Vec_Copy(p->pos, o->pos);
            }
            k += stride;
            p++;
            continue;
        }
        switch (o->type) {
        case 0:
        case 9:
        case 12:
        case 13:
            cj = k;
            curve = (void *) StageObj_K10_Curves[cj];
            Key_LerpVec3(curve, p->sc, p->delay);
            k += 2;
            Key_LerpF32((void *) StageObj_K10_Curves[(s16) (cj + 1)], &a, p->delay);
            p->alpha = 255.0f * a;
            break;
        case 2:
            curve = (void *) StageObj_K10_Curves[k++];
            Key_LerpVec3(curve, p->sc, p->delay);
            if (i == 2) {
                p->angle = Rand_Next(1);
                p->idx = Rand_Next(1) % 3;
            }
            break;
        case 5:
            curve = (void *) StageObj_K10_Curves[k++];
            Key_LerpVec3(curve, p->sc, p->delay);
            if (i >= 3) {
                p->idx = Rand_Next(1) % 3 + 1;
                p->angle = Rand_Next(1);
            }
            break;
        case 6:
            curve = (void *) StageObj_K10_Curves[k++];
            Key_LerpVec3(curve, p->sc, p->delay);
            if (i == 0) {
                p->angle = Rand_Next(1);
                p->idx = Rand_Next(1) % 3;
            } else {
                curve = (void *) StageObj_K10_Curves[k++];
                Key_LerpF32(curve, &a, p->delay);
                p->alpha = 255.0f * a;
            }
            break;
        case 14:
            break;
        default:
            ci = k;
            curve = (void *) StageObj_K10_Curves[ci];
            Key_LerpVec3(curve, p->sc, p->delay);
            k += 2;
            Key_LerpF32((void *) StageObj_K10_Curves[(s16) (ci + 1)], &a, p->delay);
            p->alpha = 255.0f * a;
            for (si = 0; StageObj_K10_List[si] != -1; si++) {
                if (p->delay == StageObj_K10_List[si]) {
                    p->unk30++;
                    break;
                }
            }
            break;
        }
        if (p->node != NULL) {
            switch (o->type) {
            case 0:
                p->pos[1] += p->unk28;
                p->node->pos[0] += p->pos[0];
                p->node->pos[1] += p->pos[1];
                p->node->pos[2] += p->pos[2];
                break;
            case 2:
            case 5:
                off[0] = 0.0f;
                off[1] = 0.0f;
                off[2] = i * 5 + 15;
                Vec_RotateSelf(off, gCamMatrix);
                p->node->pos[0] = p->pos[0] + off[0];
                p->node->pos[1] = p->pos[1] + off[1];
                p->node->pos[2] = p->pos[2] + off[2];
                break;
            case 6:
                off[0] = 0.0f;
                off[1] = 0.0f;
                off[2] = i * 5 + 15;
                Vec_RotateSelf(off, gCamMatrix);
                p->node->pos[0] = p->pos[0] + off[0];
                p->node->pos[1] = p->pos[1] + off[1];
                p->node->pos[2] = p->pos[2] + off[2];
                break;
            case 8:
            case 15:
                switch (i) {
                case 6:
                    p->sc[0] *= 0.8f;
                    p->sc[1] *= 0.8f;
                    p->sc[2] *= 0.8f;
                    p->alpha = 0.8f * p->alpha;
                    break;
                case 7:
                    p->sc[0] *= 0.6f;
                    p->sc[1] *= 0.6f;
                    p->sc[2] *= 0.6f;
                    p->alpha = 0.6f * p->alpha;
                    break;
                case 8:
                    p->sc[0] *= 0.4f;
                    p->sc[1] *= 0.4f;
                    p->sc[2] *= 0.4f;
                    p->alpha = 0.4f * p->alpha;
                    break;
                }
                p->pos[1] += 3.5f + 0.003f * (Rand_Next(1) & 0x3FF);
                p->node->pos[0] = p->pos[0];
                p->node->pos[1] = p->pos[1];
                p->node->pos[2] = p->pos[2];
                break;
            case 9:
                switch (i) {
                case 4:
                    p->alpha = 0.8f * p->alpha;
                    break;
                case 5:
                    p->alpha = 0.6f * p->alpha;
                    break;
                case 6:
                    p->alpha = 0.4f * p->alpha;
                    break;
                }
                p->angle += (u16) StageObj_K10_SpinSpeed(p->idx);
                p->pos[1] += 2.0f + 0.001f * (Rand_Next(1) & 0x3FF);
                p->node->pos[0] = p->pos[0];
                p->node->pos[1] = p->pos[1];
                p->node->pos[2] = p->pos[2];
                break;
            case 12:
            case 13:
                p->angle += (u16) StageObj_K10_SpinSpeed(p->idx);
                if (o->type == 12) {
                    p->alpha = 0.1f * p->alpha;
                }
                off[0] = 0.0f;
                off[1] = 0.0f;
                off[2] = -(i * 5);
                Vec_RotateSelf(off, gCamMatrix);
                p->pos[1] += 3.0f + 0.001f * (Rand_Next(1) & 0x3FF);
                p->node->pos[0] = p->pos[0] + off[0];
                p->node->pos[1] = p->pos[1] + off[1];
                p->node->pos[2] = p->pos[2] + off[2];
                break;
            case 14:
                Vec_Copy(p->node->pos, o->pos);
                break;
            }
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
        }
        p++;
    }
}

void StageObj_K10_End(StageObj *o)
{
    s16 n;
    s16 i;
    Obj10Part *p = (Obj10Part *) o->buf;

    o->state++;
    n = StageObj_K10_PartCounts[o->type];
    for (i = 0; i < n; i++) {
        if (p->node != NULL) {
            DrawSlot_Free(p->slot);
        }
        p++;
    }
}

void StageObj_K10_Free(StageObj *o)
{
    StageObj_Free(o);
}

extern u8 D_0041CEC1;
extern u8 D_0041CEC2;
void StageObj_K10_Draw(DrawSlot *node)
{
    StageObj *o = node->owner;
    s16 i;
    Obj10Part *p = (Obj10Part *) o->buf + node->part;
    u16 mode = 0;
    ModelMatSet *model;
    ModelMat *mats;
    u8 r;
    u8 g;
    u8 b;
    Model *set = gStageModelSet;
    f32 t;
    f32 rot[3];
    f32 scale[3];
    f32 m[16];
    f32 tm[16];

    if (set != NULL && set->active) {
        mats = set->mats;
        r = 0xFF;
        g = 0xFF;
        b = 0xFF;
        switch (o->type) {
        case 0:
            Gs_SetState(0x6C, 0);
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            scale[0] = p->sc[0] * (p->scale * StageObj_K10_Scales[o->var0 & 0xF]);
            scale[1] = p->sc[1] * (p->scale * StageObj_K10_Scales[o->var0 & 0xF]);
            scale[2] = p->sc[2] * (p->scale * StageObj_K10_Scales[o->var0 & 0xF]);
            if ((o->var0 & 0xF) == 0) {
                Effect_ScaleVec(scale);
            }
            model = &set->matSets[97];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Translation(tm, 0.125f * (p->unk30 & 1), 0.125f * (p->unk30 >> 1), 0.0f);
            Gs_SetState(0x19, (s32) tm);
            i = Effect_KindClass_Unk(o->ownerIndex);
            r = gEffectColors[i].r;
            g = gEffectColors[i].g;
            b = gEffectColors[i].b;
            if (p->delay >= 8) {
                t = (p->delay - 8) / 8.0f;
                r = r - (u8) (r / 2 * t);
                g = g - (u8) (g / 2 * t);
                b = b - (u8) (b / 2 * t);
            }
            break;
        case 2:
            Gs_SetState(0x6C, 0);
            scale[0] = p->scale * p->sc[0];
            scale[1] = p->scale * p->sc[1];
            scale[2] = p->scale * p->sc[2];
            if (node->part == 0) {
                if ((o->var0 & 0xF0) == 0x20) {
                    return;
                }
                rot[2] = (6.2831855f * o->angle) / 65536.0f;
                model = &set->matSets[0];
            } else {
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                model = &set->matSets[p->idx] + 32;
            }
            if (o->var1 == 1) {
                r = 0xFF;
                mode |= 2;
                g = 0xFF;
                b = 0xFF;
                if (node->part == 0) {
                    Mtx_Translation(tm, 0.125f, 0.0f, 0.0f);
                } else {
                    Mtx_Translation(tm, 0.25f, 0.0f, 0.0f);
                }
            } else {
                Effect_ScaleVec(scale);
                i = Effect_KindClass_Unk(o->ownerIndex);
                r = gEffectColors[i].r;
                g = gEffectColors[i].g;
                b = gEffectColors[i].b;
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Gs_SetState(0x19, (s32) tm);
            break;
        case 5:
            Gs_SetState(0x6C, 0);
            scale[0] = p->scale * p->sc[0];
            scale[1] = p->scale * p->sc[1];
            scale[2] = p->scale * p->sc[2];
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            if (p->idx == 0) {
                model = &set->matSets[0];
            } else {
                model = &set->matSets[p->idx] + 31;
            }
            if (o->var1 == 1) {
                r = 0xFF;
                mode |= 2;
                g = 0xFF;
                b = 0xFF;
                if (p->idx == 0) {
                    Mtx_Translation(tm, 0.125f, 0.0f, 0.0f);
                } else {
                    Mtx_Translation(tm, 0.25f, 0.0f, 0.0f);
                }
            } else {
                Effect_ScaleVec(scale);
                i = Effect_KindClass_Unk(o->ownerIndex);
                r = gEffectColors[i].r;
                g = gEffectColors[i].g;
                b = gEffectColors[i].b;
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Gs_SetState(0x19, (s32) tm);
            break;
        case 6:
            mode |= 2;
            scale[0] = p->scale * p->sc[0];
            scale[1] = p->scale * p->sc[1];
            scale[2] = p->scale * p->sc[2];
            if (node->part == 0) {
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                model = &set->matSets[p->idx] + 32;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
                Mtx_Translation(tm, 0.25f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) tm);
            } else {
                model = &set->matSets[21];
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, m);
            }
            break;
        case 8:
        case 15:
            mode |= 2;
            scale[0] = 0.5f * p->scale * p->sc[0];
            scale[1] = 0.5f * p->scale * p->sc[1];
            scale[2] = 0.5f * p->scale * p->sc[2];
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            model = &set->matSets[78];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Translation(tm, 0.25f * (p->unk30 & 3), 0.25f * (p->unk30 >> 2), 0.0f);
            Gs_SetState(0x19, (s32) tm);
            break;
        case 9:
        case 12:
        case 13:
            Gs_SetState(0x6C, 0);
            if (o->type == 9) {
                scale[0] = 0.5f * p->scale * p->sc[0];
                scale[1] = 0.5f * p->scale * p->sc[1];
                scale[2] = 0.5f * p->scale * p->sc[2];
            } else if (o->type == 10 || o->type == 13) {
                scale[0] = 0.2f * p->scale * p->sc[0];
                scale[1] = 0.2f * p->scale * p->sc[1];
                scale[2] = 0.2f * p->scale * p->sc[2];
            } else if (o->type == 12) {
                scale[0] = 2.0f * p->scale * p->sc[0];
                scale[1] = 2.0f * p->scale * p->sc[1];
                scale[2] = 2.0f * p->scale * p->sc[2];
            }
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            if (o->type == 12) {
                model = &set->matSets[74];
            } else {
                if (!(p->idx & 1)) {
                    model = &set->matSets[77];
                    Mtx_Translation(tm, 0.0f, 0.25f, 0.0f);
                } else {
                    model = &set->matSets[79];
                    Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
                }
                Gs_SetState(0x19, (s32) tm);
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            break;
        case 14:
            scale[0] = p->scale;
            scale[1] = p->scale;
            scale[2] = p->scale;
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            model = &set->matSets[97];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_RotateX(m, 4.712389f);
            Mtx_Translation(tm, 0.125f * (p->unk30 & 1), 0.125f * (p->unk30 >> 1), 0.0f);
            Gs_SetState(0x19, (s32) tm);
            r = *(u8 *)gEffectColors;
            g = D_0041CEC1;
            b = D_0041CEC2;
            break;
        }
        if (o->type != 14) {
            Mtx_Concat3x3(m, gCamMatrix);
        }
        Effect_DrawModel(model, m, b | ((g << 8) | ((p->alpha << 24) | (r << 16))), mode, mats);
        Gfx_SetBlend(4, 5);
        Gs_SetState(0x6C, 1);
    }
}

s32 StageObj_K10_SpinSpeed(s16 x)
{
    switch (x & 6) {
    case 0:
        return 0x111;
    case 2:
        return -0x110;
    case 4:
        return 0x89;
    case 6:
        return -0x88;
    }
    return 0;
}

void StageObj_K10_PlaySound(StageObj *o, f32 *pos)
{
    switch (o->type) {
    case 0:
        if (StageObj_K10_Scales[o->var0 & 0xF] <= 0.5f) {
            Sound_PlaySeAtPosChecked(1, 0x6E, 0, pos, 1, 0);
        } else {
            switch (o->var0 & 0xF0) {
            case 0:
                Sound_PlaySeAtPosChecked(1, 0x6E, 0, pos, 1, 0);
                break;
            case 0x10:
                Sound_PlaySeAtPosChecked(1, 0x6F, 0, pos, 1, 0);
                break;
            }
        }
        break;
    case 2:
    case 5:
        switch (o->var1) {
        case 1:
            switch (o->var0 & 0xF0) {
            case 0x10:
                Sound_PlaySeAtPosChecked(1, 0x65, 0, pos, 1, 0);
                break;
            case 0:
                Sound_PlaySeAtPosChecked(1, 0x64, 0, pos, 1, 0);
                break;
            }
            break;
        }
        break;
    case 6:
        Sound_PlaySeAtPosChecked(1, 0x63, 0, pos, 1, 0);
        break;
    case 8:
        Sound_PlaySeAtPosChecked(1, 0x95, 0, pos, 1, 0);
        break;
    }
}

void StageObj_K10_CreateOnEm(Em *em, ObjAttach *hit, u8 k, s16 mode, f32 *pos, f32 scale)
{
    StageObj *o;
    f32 m[16];
    f32 inv[16];
    f32 v[3];

    if (em != NULL && (Obj_InCurrentArea(em) & 0xFF)) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x10;
            o->update = StageObj_K10_Update;
            o->type = k;
            o->owner = em;
            o->ownerIndex = 0;
            switch (mode) {
            case 2:
            default:
                o->var0 = 0;
                break;
            case 3:
                o->var0 = 0x10;
                break;
            case 0:
                o->var0 = 0x20;
                break;
            }
            o->scale = scale;
            if (o->type == 6) {
                o->pos[0] = pos[0];
                o->pos[1] = pos[1];
                o->pos[2] = pos[2];
                o->angle = Rand_Next(1);
                o->var2 = 0xFF;
                return;
            }
            o->angle = em->hitAngle + 0x7FFF + 1;
            o->var0 &= 0xF0;
            if (em->isEm != 0) {
                o->ownerIndex = em->type;
                switch (o->ownerIndex) {
                case 9:
                case 0x17:
                case 0x13:
                case 0x18:
                case 0x38:
                    o->var0 |= 2;
                    break;
                }
                if (hit != NULL && hit->targetPart != NULL) {
                    o->var2 = hit->targetPart->bone;
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, o->var2)));
                    Mtx_GetTrans(v, m);
                    o->pos[0] = pos[0] - v[0];
                    o->pos[1] = pos[1] - v[1];
                    o->pos[2] = pos[2] - v[2];
                    Mtx_Inverse(inv, m);
                    Vec_RotateSelf(o->pos, inv);
                    return;
                }
            } else {
                o->var0 |= 1;
                o->var2 = 0xA;
                if (o->type == 8 || o->type == 9) {
                    o->pos[0] = 0.0f;
                    o->pos[1] = 0.0f;
                    o->pos[2] = 0.0f;
                } else {
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, o->var2)));
                    Mtx_GetTrans(v, m);
                    o->pos[0] = pos[0] - v[0];
                    o->pos[1] = pos[1] - v[1];
                    o->pos[2] = pos[2] - v[2];
                    Mtx_Inverse(inv, m);
                    Vec_RotateSelf(o->pos, inv);
                }
                return;
            }
            o->var2 = 0xFF;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
        }
    }
}

void StageObj_K10_CreateAt(f32 *pos, u8 k, s16 v, u8 c, f32 scale)
{
    StageObj *o = StageObj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x10;
        o->update = StageObj_K10_Update;
        o->type = k;
        o->angle = v;
        o->scale = scale;
        o->var1 = c;
        o->var2 = 0xFF;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
    }
}

void StageObj_K10_CreateOnEmAt(Em *em, f32 *pos, u8 k, s16 mode, s16 part, f32 scale)
{
    StageObj *o;

    if (em != NULL && (Obj_InCurrentArea(em) & 0xFF)) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x10;
            o->update = StageObj_K10_Update;
            o->type = k;
            o->owner = em;
            o->ownerIndex = 0;
            switch (mode) {
            case 2:
            default:
                o->var0 = 0;
                break;
            case 3:
                o->var0 = 0x10;
                break;
            case 0:
                o->var0 = 0x20;
                break;
            }
            o->var0 &= 0x3F;
            switch (part) {
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
                o->var1 = 1;
                break;
            default:
                o->var1 = 0;
                if (em->isEm != 0) {
                    o->ownerIndex = em->type;
                }
                break;
            }
            o->scale = scale;
            o->pos[0] = pos[0];
            o->pos[1] = pos[1];
            o->pos[2] = pos[2];
            o->angle = Rand_Next(1);
            o->var2 = 0xFF;
        }
    }
}

void StageObj_K10_CreateAtScaled(f32 *pos, u8 k, f32 scale)
{
    StageObj *o = StageObj_Alloc(1);

    if (o != NULL) {
        o->kind = 0x10;
        o->update = StageObj_K10_Update;
        o->type = k;
        o->owner = NULL;
        o->ownerIndex = 0;
        o->scale = scale;
        o->pos[0] = pos[0];
        o->pos[1] = pos[1];
        o->pos[2] = pos[2];
        o->angle = Rand_Next(1);
        o->var2 = 0xFF;
    }
}

f32 StageObj_RandUnit(void)
{
    return 0.001f * (f32) (Rand_Next(1) & 0xFFFF & 0x3FF);
}

f32 StageObj_RandSigned(void)
{
    return 0.001f * (f32) ((Rand_Next(1) & 0xFFFF & 0x3FF) - 0x200);
}

void StageObj_K11_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K11_Setup(o);
        break;
    case 1:
        StageObj_K11_Run(o);
        break;
    case 2:
        StageObj_K11_End(o);
        break;
    case 3:
        StageObj_K11_Free(o);
        break;
    }
}

void StageObj_K11_Setup(StageObj *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    case 8:
        StageObj_K11_SetupRocks(o);
        break;
    default:
        StageObj_K11_SetupDefault(o);
        break;
    }
}

void StageObj_K11_Run(StageObj *o)
{
    switch (o->type) {
    case 8:
        StageObj_K11_RunRocks(o);
        break;
    default:
        StageObj_K11_RunDefault(o);
        break;
    }
}

void StageObj_K11_End(StageObj *o)
{
    switch (o->type) {
    case 8:
        StageObj_K11_EndRocks(o);
        break;
    default:
        StageObj_K11_EndDefault(o);
        break;
    }
}

void StageObj_K11_Draw(DrawSlot *node)
{
    switch (((StageObj *) node->owner)->type) {
    case 8:
        StageObj_K11_DrawRock(node);
        break;
    default:
        StageObj_K11_DrawDefault(node);
        break;
    }
}

/* a part of a kind 0x11 object of the default types (0x30 bytes, in its blocks) */
typedef struct Obj11Part {
    s16 slot;                   /* 0x00: its draw slot */
    s16 idx;                    /* 0x02 */
    f32 sc[3];                  /* 0x04: its scale, from a curve by its frame */
    f32 pos[3];                 /* 0x10 */
    f32 unk1C;                  /* 0x1C */
    s16 delay;                  /* 0x20: frames before it starts (negative) */
    s16 spin;                   /* 0x22 */
    u16 unk24;                  /* 0x24 */
    u16 angle;                  /* 0x26 */
    f32 scale;                  /* 0x28 */
    DrawSlot *node;                /* 0x2C */
} Obj11Part;
extern s16 StageObj_K11_DelaysA[];
extern s16 StageObj_K11_DelaysB[];
void StageObj_K11_PlaceAtBone(StageObj *o);
s32 StageObj_K11_OwnerInMotion(StageObj *o);
f32 StageObj_RandUnit(void);
f32 StageObj_RandSigned(void);
extern s16 StageObj_K11_PartCounts[];
void Mtx_TurnX(f32 *m, f32 a);
extern u16 StageObj_K11_Durations[];
extern s16 StageObj_K11_CurveIdx[];
extern s16 StageObj_K11_Steps[];
extern u16 *StageObj_K11_Tbls[];
extern void *StageObj_K11_Curves[];
void Vec_RotateX(f32 *v, f32 a);

void StageObj_K11_SetupDefault(StageObj *o)
{
    Obj11Part *p = (Obj11Part *) o->buf;
    s16 n;
    s16 i;
    u16 a;
    f32 m[16];
    f32 v[3];

    n = StageObj_K11_PartCounts[o->type];
    o->var1 = 0;
    switch (o->type) {
    case 0:
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 50.0f;
        Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        break;
    case 1:
        o->var2 = ((Em *) o->owner)->type;
        v[0] = 0.0f;
        if (o->var2 == 2 || o->var2 == 0x24 || o->var2 == 0x47) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 0x33)));
            v[1] = -41.8f;
            v[2] = 50.0f;
        } else if (o->var2 == 0x3F) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 0x13)));
            v[1] = -50.0f;
            v[2] = 50.0f;
        } else {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 0x22)));
            v[1] = -80.0f;
            v[2] = 80.0f;
        }
        Vec_RotateSelf(v, m);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        o->angle = ((Em *) o->owner)->rotY;
        break;
    case 2:
        o->var2 = ((Em *) o->owner)->type;
        v[0] = 0.0f;
        if (o->var2 == 2 || o->var2 == 0x24 || o->var2 == 0x47) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 0x33)));
            v[1] = -91.6f;
            v[2] = 132.8f;
        } else if (o->var2 == 0x3F) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 0x13)));
            v[1] = -50.0f;
            v[2] = 50.0f;
        } else {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 0x22)));
            v[1] = -80.0f;
            v[2] = 120.0f;
        }
        Vec_RotateSelf(v, m);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        o->angle = ((Em *) o->owner)->rotY;
        break;
    case 3:
        if (o->var0 == 0) {
            n = 1;
        } else {
            n = o->var0;
        }
        a = o->angle;
        o->angle = o->timer;
        o->timer = 0;
        break;
    case 6:
        o->angle = Rand_Next(1);
        break;
    case 7:
        if (StageObj_K11_OwnerInMotion(o) == 1) {
            StageObj_K11_PlaceAtBone(o);
        } else {
            o->var1 = 0xFF;
        }
        o->angle = Rand_Next(1);
        break;
    case 12:
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, o->angle)));
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 50.0f * ((Em *) o->owner)->scaleZ;
        Vec_RotateSelf(v, m);
        v[0] += o->pos[0];
        v[1] += o->pos[1];
        v[2] += o->pos[2];
        Mtx_TurnX(m, 6.2831855f * (o->var0 << 8) / 65536.0f);
        Vec_Copy(o->pos, &m[8]);
        o->pos[0] *= 10.0f;
        o->pos[1] *= 10.0f;
        o->pos[2] *= 10.0f;
        o->var2 = ((Em *) o->owner)->type;
        break;
    case 14:
        o->var2 = ((Em *) o->owner)->type;
        o->scale *= 2.0f;
        break;
    case 5:
    case 16:
    case 17:
        break;
    case 18:
        o->var1 = Rand_Next(1) & 0x1FF;
        break;
    }
    o->timer = 0;
    for (i = 0; i < n; p++, i++) {
        p->idx = i;
        switch (o->type) {
        case 0:
            p->delay = 0;
            p->angle = Rand_Next(1);
            p->scale = 1.0f;
            break;
        case 1:
            switch (i) {
            case 0:
            case 1:
                p->idx = 0;
                p->delay = 0;
                break;
            case 2:
            case 3:
                p->idx = 1;
                p->delay = -2;
                break;
            }
            p->scale = o->scale * (1.0f + 0.5f * StageObj_RandUnit());
            p->unk24 = Rand_Next(1);
            p->angle = Rand_Next(1);
            p->spin = (Rand_Next(1) & 0xFF) - 0x80;
            break;
        case 2:
            if (i == 0) {
                p->delay = 0;
            } else {
                p->delay = -1;
            }
            p->angle = Rand_Next(1);
            p->scale = 0.0f;
            Vec_Copy(p->pos, o->pos);
            break;
        case 3:
            p->delay = 0;
            p->angle = Rand_Next(1);
            p->unk24 = a;
            if (o->var2 == 2 || o->var2 == 0x24 || o->var2 == 0x47) {
                p->scale = 1.5f;
            } else {
                p->scale = 1.0f;
            }
            if (i == 0) {
                p->scale *= 1.0f + StageObj_RandUnit();
                p->spin = (Rand_Next(1) & 0xFF) - 0x80;
            } else {
                p->scale *= 1.0f + 1.5f * StageObj_RandUnit();
                p->spin = (Rand_Next(1) & 0x1FF) - 0x100;
            }
            break;
        case 4:
            p->delay = -5 - i * 2;
            p->angle = Rand_Next(1);
            p->spin = 0.5f + 65536.0f * StageObj_RandSigned() / 360.0f;
            p->scale = o->scale * (1.0f + 0.5f * StageObj_RandUnit());
            p->pos[0] = o->scale * ((Rand_Next(1) & 0x1F) - 0x10);
            p->pos[1] = o->scale * ((Rand_Next(1) & 0x1F) - 0x10);
            p->pos[2] = i * -10;
            break;
        case 5:
        case 16:
            p->delay = -4 - i * 5;
            p->idx = i;
            p->angle = Rand_Next(1);
            p->scale = o->scale * (1.0f + 0.5f * StageObj_RandUnit());
            p->spin = 0.5f + 65536.0f * (2.0f * StageObj_RandSigned()) / 360.0f;
            p->pos[0] = 3.3333333f * (2.0f * StageObj_RandSigned());
            p->pos[1] = 7.5f;
            p->pos[2] = 0.0f;
            break;
        case 6:
            p->delay = 0;
            p->angle = 0;
            p->scale = 1.0f;
            p->unk1C = 1.0f;
            break;
        case 7:
            p->delay = 0;
            p->angle = Rand_Next(1);
            p->scale = 1.0f;
            p->unk1C = 1.0f;
            break;
        case 9:
            p->delay = -i;
            p->angle = Rand_Next(1);
            break;
        case 10:
            p->delay = -2;
            p->pos[0] = o->pos[0];
            p->pos[1] = 2.0f + o->pos[1];
            p->pos[2] = o->pos[2];
            break;
        case 11:
            p->delay = StageObj_K11_DelaysA[i];
            p->scale = 1.2f * o->scale;
            break;
        case 12:
            p->delay = 0;
            p->angle = Rand_Next(1);
            p->scale = 0.0f;
            Vec_Copy(p->pos, v);
            break;
        case 13:
            p->delay = 0;
            p->idx = Rand_Next(1) & 3;
            p->angle = 0;
            p->scale = o->scale * (0.8f + 0.4f * StageObj_RandUnit());
            Vec_Copy(p->pos, o->pos);
            break;
        case 14:
            p->delay = 0;
            p->idx = Rand_Next(1) & 1;
            p->angle = Rand_Next(1);
            p->scale = o->scale * (0.8f + 0.4f * StageObj_RandUnit());
            p->pos[0] = o->pos[0] + 5.0f * StageObj_RandSigned();
            p->pos[1] = o->pos[1] + 5.0f * StageObj_RandSigned();
            p->pos[2] = o->pos[2] + 5.0f * StageObj_RandSigned();
            break;
        case 15:
            p->delay = StageObj_K11_DelaysB[i];
            if (i == 0) {
                p->idx = 0;
            } else {
                p->idx = Rand_Next(1) % 6 + 1;
            }
            p->angle = 0;
            Vec_Copy(p->pos, o->pos);
            p->scale = o->scale * (0.8f + 0.4f * StageObj_RandUnit());
            break;
        case 17:
            p->delay = 0;
            p->angle = Rand_Next(1);
            Vec_Copy(p->pos, o->pos);
            break;
        case 18:
            p->delay = 0;
            p->angle = Rand_Next(1);
            p->scale = 0.8f + 0.2f * StageObj_RandUnit();
            break;
        case 19:
            p->delay = i * -3;
            p->angle = Rand_Next(1);
            p->scale = 0.0f;
            Vec_Copy(p->pos, v);
            p->pos[0] += (Rand_Next(1) & 0x1F) - 0x10;
            p->pos[1] += (Rand_Next(1) & 0x1F) - 0x10;
            p->pos[2] += (Rand_Next(1) & 0x1F) - 0x10;
            break;
        case 20:
            p->delay = i * -3;
            p->idx = Rand_Next(1) & 3;
            p->angle = 0;
            p->scale = o->scale * (0.8f + 0.4f * StageObj_RandUnit());
            Vec_Copy(p->pos, o->pos);
            p->pos[0] += (Rand_Next(1) & 0x1F) - 0x10;
            p->pos[1] += (Rand_Next(1) & 0x1F) - 0x10;
            p->pos[2] += (Rand_Next(1) & 0x1F) - 0x10;
            break;
        }
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K11_Draw;
        } else {
            p->node = NULL;
        }
    }
    StageObj_K11_Run(o);
}

void StageObj_K11_RunDefault(StageObj *o)
{
    Obj11Part *p;
    s16 i;
    u16 dur;
    s16 k;
    s16 step;
    u16 *tbl;
    u16 life;
    s16 n;
    Em *em;
    void *curve;
    f32 m[16];
    f32 v[3];

    p = (Obj11Part *) o->buf;
    n = StageObj_K11_PartCounts[o->type];
    dur = StageObj_K11_Durations[o->type];
    k = StageObj_K11_CurveIdx[o->type];
    step = StageObj_K11_Steps[o->type];
    tbl = StageObj_K11_Tbls[o->type];

    switch (o->type) {
    case 1:
        life = 0x12;
        break;
    case 3:
        if (o->var0 == 0) {
            n = 1;
        } else {
            n = o->var0;
        }
        break;
    case 4:
        life = 0x26;
        break;
    case 5:
    case 16:
        life = 0x18;
        break;
    case 7:
        if (o->var1 != 0xFF) {
            if (StageObj_K11_OwnerInMotion(o) == 1) {
                StageObj_K11_PlaceAtBone(o);
            } else {
                o->var1 = 0xFF;
            }
        }
        life = dur;
        break;
    case 10:
        life = 6;
        break;
    case 0:
    case 6:
    case 12:
    case 13:
    case 17:
    case 18:
        life = dur;
        break;
    case 14:
        em = o->owner;
        life = dur;
        if (em->taskPhase == 3 || em->active == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, o->angle)));
        v[0] = 0.0f;
        v[1] = -50.0f;
        v[2] = 100.0f;
        Vec_RotateSelf(v, m);
        o->pos[0] = m[12] + v[0];
        o->pos[1] = m[13] + v[1];
        o->pos[2] = m[14] + v[2];
        break;
    case 19:
        life = 0xF;
        break;
    case 20:
        life = 0x12;
        break;
    }
    if (++o->timer >= dur) {
        o->state++;
        o->visible = 0;
        return;
    }
    for (i = 0; i < n; i++) {
        switch (o->type) {
        case 1:
        case 4:
        case 5:
        case 16:
        case 19:
        case 20:
            k = StageObj_K11_CurveIdx[o->type];
            break;
        case 2:
        case 11:
        case 15:
            life = tbl[i];
            break;
        case 3:
            if (i == 0) {
                life = 10;
            } else {
                life = 35;
            }
            break;
        case 9:
            if (i == 0) {
                life = 15;
            } else {
                life = 20;
            }
            break;
        }
        if (++p->delay <= 0) {
            p++;
            k += step;
            continue;
        }
        if (p->delay >= life) {
            p++;
            k += step;
            continue;
        }
        curve = StageObj_K11_Curves[k++];
        Key_LerpVec3(curve, p->sc, p->delay);
        switch (o->type) {
        case 0:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 80.0f * p->delay / life;
            Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
            p->pos[0] = o->pos[0] + v[0];
            p->pos[1] = o->pos[1] + v[1];
            p->pos[2] = o->pos[2] + v[2];
            break;
        case 5:
        case 16:
            p->angle = p->angle + (u16) p->spin;
            break;
        case 1:
            curve = StageObj_K11_Curves[k++];
            Key_LerpVec3(curve, p->pos, p->delay);
            switch (p->idx) {
            case 0:
                break;
            case 1:
                p->pos[0] -= 40.0f;
                break;
            }
            if (i == 1 || i == 3) {
                p->pos[0] = -p->pos[0];
            }
            p->pos[0] *= o->scale;
            p->pos[1] *= o->scale;
            p->pos[2] *= o->scale;
            Vec_RotateY(p->pos, 6.2831855f * o->angle / 65536.0f);
            p->pos[0] += o->pos[0];
            p->pos[1] += o->pos[1];
            p->pos[2] += o->pos[2];
            p->angle = p->angle + (u16) p->spin;
            break;
        case 2:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 5.0f * i;
            Vec_RotateSelf(v, gCamMatrix);
            p->pos[0] = o->pos[0] + v[0];
            p->pos[1] = o->pos[1] + v[1];
            p->pos[2] = o->pos[2] + v[2];
            break;
        case 3:
            p->pos[0] = 0.0f;
            p->pos[1] = 0.0f;
            if (i == 0) {
                p->pos[2] = 120.0f - 50.0f * p->delay / life;
            } else {
                p->pos[2] = -(30.0f * p->delay / life);
            }
            p->angle = p->angle + (u16) p->spin;
            Vec_RotateX(p->pos, 6.2831855f * o->angle / 65536.0f);
            Vec_RotateY(p->pos, 6.2831855f * p->unk24 / 65536.0f);
            p->pos[0] += o->pos[0];
            p->pos[1] += o->pos[1];
            p->pos[2] += o->pos[2];
            break;
        case 4:
            Vec_Rotate(v, p->pos, gCamMatrix);
            p->angle = p->angle + (u16) p->spin;
            break;
        case 6:
            p->scale = o->scale * (1.0f + 0.2f * StageObj_RandUnit());
            Vec_Copy(p->pos, o->pos);
            break;
        case 7:
            p->angle = Rand_Next(1);
            p->scale = o->scale * (1.0f + 0.2f * StageObj_RandUnit());
            Vec_Copy(p->pos, o->pos);
            if (p->delay > 23) {
                p->unk1C = (f32) (life - p->delay) / 13.0f;
            }
            break;
        case 9:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 20.0f + i * 3;
            Vec_RotateSelf(v, gCamMatrix);
            p->pos[0] = o->pos[0] + v[0];
            p->pos[1] = o->pos[1] + v[1];
            p->pos[2] = o->pos[2] + v[2];
            break;
        case 10:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -1000.0f;
            Vec_RotateSelf(v, gCamMatrix);
            p->pos[0] = o->pos[0] + v[0];
            p->pos[1] = o->pos[1] + v[1];
            p->pos[2] = o->pos[2] + v[2];
            break;
        case 11:
            curve = StageObj_K11_Curves[k++];
            Key_LerpVec3(curve, p->pos, p->delay);
            p->pos[0] *= o->scale;
            p->pos[1] *= o->scale;
            p->pos[2] *= o->scale;
            p->pos[0] += o->pos[0];
            p->pos[1] += o->pos[1];
            p->pos[2] += o->pos[2];
            break;
        case 12:
            p->pos[0] += o->pos[0];
            p->pos[1] += o->pos[1];
            p->pos[2] += o->pos[2];
            break;
        case 14:
            curve = StageObj_K11_Curves[k++];
            Key_LerpVec3(curve, p->pos, p->delay);
            Vec_RotateSelf(p->pos, m);
            p->pos[0] += o->pos[0];
            p->pos[1] += o->pos[1];
            p->pos[2] += o->pos[2];
            break;
        case 15:
            curve = StageObj_K11_Curves[k++];
            Key_LerpVec3(curve, v, p->delay);
            Vec_RotateY(v, 6.2831855f * o->angle / 65536.0f);
            p->pos[0] = 0.0f;
            p->pos[1] = 0.0f;
            p->pos[2] = 5.0f * i;
            Vec_RotateSelf(p->pos, gCamMatrix);
            p->pos[0] += o->pos[0] + v[0];
            p->pos[1] += o->pos[1] + v[0];
            p->pos[2] += o->pos[2] + v[0];
            break;
        case 18:
            p->angle += o->var1 - 0x100;
            p->pos[0] = o->pos[0];
            p->pos[1] = o->pos[1];
            p->pos[2] = o->pos[2];
            break;
        case 19:
        case 20:
            p->pos[0] = o->pos[0];
            p->pos[1] = o->pos[1];
            p->pos[2] = o->pos[2];
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 5.0f * i;
            Vec_RotateSelf(v, gCamMatrix);
            break;
        }
        if (step > 1 && o->type != 15) {
            curve = StageObj_K11_Curves[k++];
            Key_LerpF32(curve, &p->unk1C, p->delay);
        }
        if (p->node != NULL) {
            if (o->type == 5 || o->type == 16) {
                p->node->pos[0] = o->pos[0] + p->delay * p->pos[0];
                p->node->pos[1] = o->pos[1] + p->delay * p->pos[1];
                p->node->pos[2] = o->pos[2] + p->delay * p->pos[2];
            } else if (o->type == 4 || o->type == 19 || o->type == 20) {
                p->node->pos[0] = o->pos[0] + v[0];
                p->node->pos[1] = o->pos[1] + v[1];
                p->node->pos[2] = o->pos[2] + v[2];
            } else {
                p->node->pos[0] = p->pos[0];
                p->node->pos[1] = p->pos[1];
                p->node->pos[2] = p->pos[2];
            }
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
        }
        p++;
    }
}

void StageObj_K11_DrawDefault(DrawSlot *node)
{
    StageObj *o;
    Obj11Part *p;
    ModelMatSet *model;
    u16 mode;
    f32 m[16];
    ModelMat *mats;
    void *curve;
    s16 k;
    u32 color;
    f32 rot[3];
    f32 scale[3];
    f32 t[3];
    f32 tm[16];
    Model *set;

    mode = 0;
    o = node->owner;
    p = (Obj11Part *) o->buf + node->part;
    set = gStageModelSet;
    if (set == NULL || !set->active) {
        goto end;
    }
    mats = set->mats;
    switch (o->type) {
    case 0:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        mode |= 2;
        if (o->var2 == 0x35) {
            model = &set->matSets[3];
        } else {
            model = &set->matSets[4];
        }
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = o->scale * p->sc[0];
        scale[1] = o->scale * p->sc[1];
        scale[2] = o->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        break;
    case 1:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        mode |= 2;
        model = &set->matSets[2];
        Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
        Gs_SetState(0x19, (s32) tm);
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xF4902E;
        break;
    case 2:
        if (p->idx == 0) {
            model = &set->matSets[66];
        } else {
            model = &set->matSets[80];
        }
        mode |= 2;
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = o->scale * p->sc[0];
        scale[1] = o->scale * p->sc[1];
        scale[2] = o->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        break;
    case 3:
        if (p->idx == 0) {
            k = EmSlot_FindKind(o->var2);
            if (k == -1) {
                goto end;
            }
            set = gScene.partModels[k];
            if (set == NULL || !set->active) {
                goto end;
            }
            model = &set->matSets[2];
            mats = set->mats;
            mode |= 2;
            Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
            Gs_SetState(0x19, (s32) tm);
            if (o->var2 == 2 || o->var2 == 0x24 || o->var2 == 0x47) {
                curve = StageObj_K11_Curve3B;
            } else {
                curve = StageObj_K11_Curve3A;
            }
            Color_EvalKeys(curve, p->delay, &color);
        } else {
            model = &set->matSets[72];
            Color_EvalKeys((s32 *) StageObj_K11_ColorKeys, p->delay, &color);
        }
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = p->sc[0] * p->scale;
        scale[1] = p->sc[1] * p->scale;
        scale[2] = p->sc[2] * p->scale;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        break;
    case 4:
        scale[0] = p->sc[0] * p->scale;
        scale[1] = p->sc[1] * p->scale;
        scale[2] = p->sc[2] * p->scale;
        model = &set->matSets[74];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0x111111;
        break;
    case 5:
    case 16:
        model = &set->matSets[74];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        if (o->type == 0) {
            color = ((u8) (255.0f * p->unk1C) << 24) | 0x333333;
        } else {
            if (o->type == 5) {
                curve = StageObj_K11_Curve10A;
            } else {
                curve = StageObj_K11_Curve10B;
            }
            Color_EvalKeys(curve, p->delay, &color);
        }
        break;
    case 6:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        mode |= 2;
        if (o->var2 == 0x35) {
            model = &set->matSets[5];
        } else {
            model = &set->matSets[8];
        }
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        break;
    case 7:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        mode |= 2;
        model = &set->matSets[2];
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        break;
    case 9:
        mode |= 2;
        if (node->part == 0) {
            model = &set->matSets[59];
        } else {
            model = &set->matSets[81];
        }
        scale[0] = 5.0f * o->scale * p->sc[0];
        scale[1] = 5.0f * o->scale * p->sc[1];
        scale[2] = 5.0f * o->scale * p->sc[2];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        break;
    case 10:
        mode |= 2;
        scale[0] = o->scale * p->sc[0];
        scale[1] = o->scale * p->sc[1];
        scale[2] = o->scale * p->sc[2];
        model = &set->matSets[42];
        t[0] = o->pos[0];
        t[1] = 2.0f + o->pos[1];
        t[2] = o->pos[2];
        Gs_SetState(0x6C, 0);
        Mtx_FromScaleRotTrans(scale, rot, t, 0, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        break;
    case 11:
        Gs_SetState(0x6C, 0);
        model = &set->matSets[35];
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 0, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0x111111;
        break;
    case 12:
    case 19:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        if (o->var2 == 0x15 || o->var2 == 0x2E) {
            mode |= 2;
            color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        } else {
            color = ((u8) (255.0f * p->unk1C) << 24) | 0xD8B68A;
        }
        model = &set->matSets[4];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = o->scale * p->sc[0];
        scale[1] = o->scale * p->sc[1];
        scale[2] = o->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        break;
    case 13:
    case 20:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        Gs_SetState(0x6C, 0);
        if (o->var2 == 0x15 || o->var2 == 0x2E) {
            mode |= 2;
            color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        } else {
            color = ((u8) (255.0f * p->unk1C) << 24) | 0xD8B68A;
        }
        model = &set->matSets[p->idx];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        break;
    case 14:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        model = &set->matSets[0];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xF8D197;
        break;
    case 15:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        if (p->idx == 0) {
            curve = StageObj_K11_CurveFA;
            model = &set->matSets[3];
        } else {
            model = set->matSets + p->idx % 3 + 4;
            if (o->var2 == 0x14 || o->var2 == 0x27) {
                curve = StageObj_K11_CurveFB;
            } else {
                curve = StageObj_K11_CurveFC;
            }
            if (p->idx & 1) {
                Mtx_TurnY(m, 3.1415927f);
            }
        }
        Color_EvalKeys(curve, p->delay, &color);
        break;
    case 17:
        mode |= 2;
        model = &set->matSets[81];
        scale[0] = o->scale * p->sc[0];
        scale[1] = o->scale * p->sc[1];
        scale[2] = o->scale * p->sc[2];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        color = ((u8) (255.0f * p->unk1C) << 24) | 0xFFFFFF;
        break;
    case 18:
        k = EmSlot_FindKind(o->var2);
        if (k == -1) {
            goto end;
        }
        set = gScene.partModels[k];
        if (set == NULL || !set->active) {
            goto end;
        }
        mats = set->mats;
        model = &set->matSets[0];
        scale[0] = p->scale * p->sc[0];
        scale[1] = p->scale * p->sc[1];
        scale[2] = p->scale * p->sc[2];
        rot[2] = (6.2831855f * p->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        if (o->var2 == 0x14 || o->var2 == 0x27) {
            curve = StageObj_K11_Curve12A;
        } else {
            curve = StageObj_K11_Curve12B;
        }
        Color_EvalKeys(curve, p->delay, &color);
        break;
    }
    if (o->type != 10) {
        Mtx_Concat3x3(m, gCamMatrix);
    }
    Effect_DrawModel(model, m, color, mode, mats);
    Gs_SetState(0x6C, 1);
    Draw_SetDefaultState();
end:;
}

/* unk_868110's .data (0x8CE640-0x8D0820) */
u32 StageObj_K0C_PartCurves_0[24] = {
#include "game/008CE640.inc"
};
u32 StageObj_K0C_PartCurves_1[12] = {
#include "game/008CE6A0.inc"
};
u32 StageObj_K0C_PartCurves_2[24] = {
#include "game/008CE6D0.inc"
};
u32 StageObj_K0C_PartCurves_3[8] = {
#include "game/008CE730.inc"
};
void *StageObj_K0C_PartCurves[4] = {
    (void *)StageObj_K0C_PartCurves_0, (void *)StageObj_K0C_PartCurves_1, (void *)StageObj_K0C_PartCurves_2, (void *)StageObj_K0C_PartCurves_3,
};
Rgb4 StageObj_K0C_Colors[8] = {
#include "game/008CE760.inc"
};
u32 StageObj_K0C_Type3R0[4] = {
#include "game/008CE780.inc"
};
u32 StageObj_K0C_Type3R1[4] = {
#include "game/008CE790.inc"
};
u32 StageObj_K0E_PartCurves_0[16] = {
#include "game/008CE7A0.inc"
};
u32 StageObj_K0E_PartCurves_2[24] = {
#include "game/008CE7E0.inc"
};
u32 StageObj_K0E_PartCurves_4[20] = {
#include "game/008CE840.inc"
};
u32 StageObj_K0E_PartCurves_18[24] = {
#include "game/008CE890.inc"
};
u32 StageObj_K0E_PartCurves_6[20] = {
#include "game/008CE8F0.inc"
};
u32 StageObj_K0E_PartCurves_8[24] = {
#include "game/008CE940.inc"
};
u32 StageObj_K0E_PartCurves_10[12] = {
#include "game/008CE9A0.inc"
};
u32 StageObj_K0E_PartCurves_12[12] = {
#include "game/008CE9D0.inc"
};
u32 StageObj_K0E_PartCurves_14[28] = {
#include "game/008CEA00.inc"
};
u32 StageObj_K0E_PartCurves_16[12] = {
#include "game/008CEA70.inc"
};
u32 StageObj_K0E_PartCurves_20[24] = {
#include "game/008CEAA0.inc"
};
u32 StageObj_K0E_PartCurves_22[24] = {
#include "game/008CEB00.inc"
};
u32 StageObj_K0E_PartCurves_24[16] = {
#include "game/008CEB60.inc"
};
u32 StageObj_K0E_PartCurves_1[8] = {
#include "game/008CEBA0.inc"
};
u32 StageObj_K0E_PartCurves_3[8] = {
#include "game/008CEBC0.inc"
};
u32 StageObj_K0E_PartCurves_5[8] = {
#include "game/008CEBE0.inc"
};
u32 StageObj_K0E_PartCurves_19[8] = {
#include "game/008CEC00.inc"
};
u32 StageObj_K0E_PartCurves_7[8] = {
#include "game/008CEC20.inc"
};
u32 StageObj_K0E_PartCurves_9[8] = {
#include "game/008CEC40.inc"
};
u32 StageObj_K0E_PartCurves_11[8] = {
#include "game/008CEC60.inc"
};
u32 StageObj_K0E_PartCurves_13[8] = {
#include "game/008CEC80.inc"
};
u32 StageObj_K0E_PartCurves_15[8] = {
#include "game/008CECA0.inc"
};
u32 StageObj_K0E_PartCurves_17[8] = {
#include "game/008CECC0.inc"
};
u32 StageObj_K0E_PartCurves_21[8] = {
#include "game/008CECE0.inc"
};
u32 StageObj_K0E_PartCurves_23[8] = {
#include "game/008CED00.inc"
};
u32 StageObj_K0E_PartCurves_25[12] = {
#include "game/008CED20.inc"
};
u32 StageObj_K0E_PartCurves_26[12] = {
#include "game/008CED50.inc"
};
u32 StageObj_K0E_PartCurves_27[10] = {
#include "game/008CED80.inc"
};
s16 StageObj_K0E_Ids[4] = {
#include "game/008CEDA8.inc"
};
Rgb4 StageObj_K0E_Colors[8] = {
#include "game/008CEDB0.inc"
};
void *StageObj_K0E_PartCurves[28] = {
    (void *)StageObj_K0E_PartCurves_0, (void *)StageObj_K0E_PartCurves_1, (void *)StageObj_K0E_PartCurves_2, (void *)StageObj_K0E_PartCurves_3,
    (void *)StageObj_K0E_PartCurves_4, (void *)StageObj_K0E_PartCurves_5, (void *)StageObj_K0E_PartCurves_6, (void *)StageObj_K0E_PartCurves_7,
    (void *)StageObj_K0E_PartCurves_8, (void *)StageObj_K0E_PartCurves_9, (void *)StageObj_K0E_PartCurves_10, (void *)StageObj_K0E_PartCurves_11,
    (void *)StageObj_K0E_PartCurves_12, (void *)StageObj_K0E_PartCurves_13, (void *)StageObj_K0E_PartCurves_14, (void *)StageObj_K0E_PartCurves_15,
    (void *)StageObj_K0E_PartCurves_16, (void *)StageObj_K0E_PartCurves_17, (void *)StageObj_K0E_PartCurves_18, (void *)StageObj_K0E_PartCurves_19,
    (void *)StageObj_K0E_PartCurves_20, (void *)StageObj_K0E_PartCurves_21, (void *)StageObj_K0E_PartCurves_22, (void *)StageObj_K0E_PartCurves_23,
    (void *)StageObj_K0E_PartCurves_24, (void *)StageObj_K0E_PartCurves_25, (void *)StageObj_K0E_PartCurves_26, (void *)StageObj_K0E_PartCurves_27,
};
s16 StageObj_K0E_ListA[16] = {
#include "game/008CEE40.inc"
};
s16 StageObj_K0E_ListB[16] = {
#include "game/008CEE60.inc"
};
u32 StageObj_K0E_PartInit[16] = {
#include "game/008CEE80.inc"
};
s16 StageObj_K0E_PartCounts[16] = {
#include "game/008CEEC0.inc"
};
u32 StageObj_K0F_Curves_0[16] = {
#include "game/008CEEE0.inc"
};
u32 StageObj_K0F_Curves_2[20] = {
#include "game/008CEF20.inc"
};
u32 StageObj_K0F_Curves_4[12] = {
#include "game/008CEF70.inc"
};
u32 StageObj_K0F_Curves_1[8] = {
#include "game/008CEFA0.inc"
};
u32 StageObj_K0F_Curves_3[8] = {
#include "game/008CEFC0.inc"
};
u32 StageObj_K0F_Curves_5[8] = {
#include "game/008CEFE0.inc"
};
s16 StageObj_K0F_DurLists_0[4] = {
#include "game/008CF000.inc"
};
s16 StageObj_K0F_DelaysA[4] = {
#include "game/008CF008.inc"
};
u32 StageObj_K0F_Curves_6[20] = {
#include "game/008CF010.inc"
};
u8 StageObj_K0F_ColorKeys1[48] = {
#include "game/008CF060.inc"
};
u32 StageObj_K0F_Curves_7[12] = {
#include "game/008CF090.inc"
};
u32 StageObj_K0F_Curves_8[12] = {
#include "game/008CF0C0.inc"
};
u32 StageObj_K0F_Curves_9[16] = {
#include "game/008CF0F0.inc"
};
u8 StageObj_K0F_ColorKeysB[32] = {
#include "game/008CF130.inc"
};
s16 StageObj_K0F_DurLists_2[8] = {
#include "game/008CF150.inc"
};
u32 StageObj_K0F_Curves_10[28] = {
#include "game/008CF160.inc"
};
u32 StageObj_K0F_Curves_12[12] = {
#include "game/008CF1D0.inc"
};
u32 StageObj_K0F_Curves_11[8] = {
#include "game/008CF200.inc"
};
u32 StageObj_K0F_Curves_13[6] = {
#include "game/008CF220.inc"
};
s16 StageObj_K0F_DurLists_3[4] = {
#include "game/008CF238.inc"
};
u32 StageObj_K0F_Curves_14[20] = {
#include "game/008CF240.inc"
};
u32 StageObj_K0F_Curves_16[24] = {
#include "game/008CF290.inc"
};
u32 StageObj_K0F_Curves_18[12] = {
#include "game/008CF2F0.inc"
};
u32 StageObj_K0F_Curves_20[12] = {
#include "game/008CF320.inc"
};
u32 StageObj_K0F_Curves_24[12] = {
#include "game/008CF350.inc"
};
u32 StageObj_K0F_Curves_26[12] = {
#include "game/008CF380.inc"
};
u32 StageObj_K0F_Curves_15[8] = {
#include "game/008CF3B0.inc"
};
u32 StageObj_K0F_Curves_17[8] = {
#include "game/008CF3D0.inc"
};
u32 StageObj_K0F_Curves_19[8] = {
#include "game/008CF3F0.inc"
};
u32 StageObj_K0F_Curves_21[8] = {
#include "game/008CF410.inc"
};
u32 StageObj_K0F_Curves_25[8] = {
#include "game/008CF430.inc"
};
u32 StageObj_K0F_Curves_27[8] = {
#include "game/008CF450.inc"
};
s16 StageObj_K0F_DelaysB[8] = {
#include "game/008CF470.inc"
};
s16 StageObj_K0F_DurLists_4[8] = {
#include "game/008CF480.inc"
};
u8 StageObj_K0F_ColorKeys[40] = {
#include "game/008CF490.inc"
};
s16 StageObj_K0F_DurLists_9[4] = {
#include "game/008CF4B8.inc"
};
void *StageObj_K0F_Curves[28] = {
    (void *)StageObj_K0F_Curves_0, (void *)StageObj_K0F_Curves_1, (void *)StageObj_K0F_Curves_2, (void *)StageObj_K0F_Curves_3,
    (void *)StageObj_K0F_Curves_4, (void *)StageObj_K0F_Curves_5, (void *)StageObj_K0F_Curves_6, (void *)StageObj_K0F_Curves_7,
    (void *)StageObj_K0F_Curves_8, (void *)StageObj_K0F_Curves_9, (void *)StageObj_K0F_Curves_10, (void *)StageObj_K0F_Curves_11,
    (void *)StageObj_K0F_Curves_12, (void *)StageObj_K0F_Curves_13, (void *)StageObj_K0F_Curves_14, (void *)StageObj_K0F_Curves_15,
    (void *)StageObj_K0F_Curves_16, (void *)StageObj_K0F_Curves_17, (void *)StageObj_K0F_Curves_18, (void *)StageObj_K0F_Curves_19,
    (void *)StageObj_K0F_Curves_20, (void *)StageObj_K0F_Curves_21, (void *)StageObj_K0F_Curves_20, (void *)StageObj_K0F_Curves_21,
    (void *)StageObj_K0F_Curves_24, (void *)StageObj_K0F_Curves_25, (void *)StageObj_K0F_Curves_26, (void *)StageObj_K0F_Curves_27,
};
s16 StageObj_K0F_PartCounts[16] = {
#include "game/008CF530.inc"
};
s16 StageObj_K0F_Limits[16] = {
#include "game/008CF550.inc"
};
s16 StageObj_K0F_Steps[16] = {
#include "game/008CF570.inc"
};
s16 StageObj_K0F_CurveIdx[16] = {
#include "game/008CF590.inc"
};
s16 StageObj_K0F_Durations[16] = {
#include "game/008CF5B0.inc"
};
s16 *StageObj_K0F_DurLists[12] = {
    StageObj_K0F_DurLists_0, NULL, StageObj_K0F_DurLists_2, StageObj_K0F_DurLists_3,
    StageObj_K0F_DurLists_4, StageObj_K0F_DurLists_4, NULL, StageObj_K0F_DurLists_3,
    StageObj_K0F_DurLists_3, StageObj_K0F_DurLists_9, NULL, NULL,
};
u32 StageObj_K10_Curves_0[24] = {
#include "game/008CF600.inc"
};
u32 StageObj_K10_Curves_1[24] = {
#include "game/008CF660.inc"
};
u32 StageObj_K10_Curves_2[16] = {
#include "game/008CF6C0.inc"
};
u32 StageObj_K10_Curves_3[20] = {
#include "game/008CF700.inc"
};
u32 StageObj_K10_Curves_5[20] = {
#include "game/008CF750.inc"
};
u32 StageObj_K10_Curves_7[24] = {
#include "game/008CF7A0.inc"
};
u32 StageObj_K10_Curves_9[24] = {
#include "game/008CF800.inc"
};
u32 StageObj_K10_Curves_11[24] = {
#include "game/008CF860.inc"
};
u32 StageObj_K10_Curves_12[24] = {
#include "game/008CF8C0.inc"
};
u32 StageObj_K10_Curves_4[12] = {
#include "game/008CF920.inc"
};
u32 StageObj_K10_Curves_6[8] = {
#include "game/008CF950.inc"
};
u32 StageObj_K10_Curves_8[8] = {
#include "game/008CF970.inc"
};
u32 StageObj_K10_Curves_10[8] = {
#include "game/008CF990.inc"
};
s16 StageObj_K10_List[16] = {
#include "game/008CF9B0.inc"
};
u32 StageObj_K10_Curves[14] = {
    (u32)StageObj_K10_Curves_0, (u32)StageObj_K10_Curves_1, (u32)StageObj_K10_Curves_2, (u32)StageObj_K10_Curves_3,
    (u32)StageObj_K10_Curves_4, (u32)StageObj_K10_Curves_5, (u32)StageObj_K10_Curves_6, (u32)StageObj_K10_Curves_7,
    (u32)StageObj_K10_Curves_8, (u32)StageObj_K10_Curves_9, (u32)StageObj_K10_Curves_10, (u32)StageObj_K10_Curves_11,
    (u32)StageObj_K10_Curves_12, 0,
};
f32 StageObj_K10_Scales[3] = {
#include "game/008CFA08.inc"
};
K10Step StageObj_K10_TblA[8] = {
#include "game/008CFA20.inc"
};
K10Step StageObj_K10_TblB[6] = {
#include "game/008CFA40.inc"
};
K10Step StageObj_K10_TblC[2] = {
#include "game/008CFA58.inc"
};
s16 StageObj_K10_PartCounts[16] = {
#include "game/008CFA60.inc"
};
s16 StageObj_K10_Lives[16] = {
#include "game/008CFA80.inc"
};
s16 StageObj_K10_CurveIdx[16] = {
#include "game/008CFAA0.inc"
};
s16 StageObj_K10_Ends[16] = {
#include "game/008CFAC0.inc"
};
s16 StageObj_K10_Strides[16] = {
#include "game/008CFAE0.inc"
};
u32 StageObj_K11_Curves_0[12] = {
#include "game/008CFB00.inc"
};
u32 StageObj_K11_Curves_1[8] = {
#include "game/008CFB30.inc"
};
u32 StageObj_K11_Curves_2[20] = {
#include "game/008CFB50.inc"
};
u32 StageObj_K11_Curves_3[12] = {
#include "game/008CFBA0.inc"
};
u32 StageObj_K11_Curves_4[8] = {
#include "game/008CFBD0.inc"
};
u32 StageObj_K11_Curves_5[12] = {
#include "game/008CFBF0.inc"
};
u32 StageObj_K11_Curves_7[12] = {
#include "game/008CFC20.inc"
};
u32 StageObj_K11_Curves_6[8] = {
#include "game/008CFC50.inc"
};
u32 StageObj_K11_Curves_8[8] = {
#include "game/008CFC70.inc"
};
u16 StageObj_K11_Tbls_2[8] = {
#include "game/008CFC90.inc"
};
u32 StageObj_K11_Curves_9[20] = {
#include "game/008CFCA0.inc"
};
u32 StageObj_K11_Curves_10[16] = {
#include "game/008CFCF0.inc"
};
u8 StageObj_K11_Curve3A[32] = {
#include "game/008CFD30.inc"
};
u8 StageObj_K11_ColorKeys[48] = {
#include "game/008CFD50.inc"
};
u8 StageObj_K11_Curve3B[32] = {
#include "game/008CFD80.inc"
};
u32 StageObj_K11_Curves_11[24] = {
#include "game/008CFDA0.inc"
};
u32 StageObj_K11_Curves_12[8] = {
#include "game/008CFE00.inc"
};
u32 StageObj_K11_Curves_13[12] = {
#include "game/008CFE20.inc"
};
u8 StageObj_K11_Curve10A[32] = {
#include "game/008CFE50.inc"
};
u32 StageObj_K11_Curves_14[12] = {
#include "game/008CFE70.inc"
};
u32 StageObj_K11_Curves_15[24] = {
#include "game/008CFEA0.inc"
};
u32 StageObj_K11_Curves_16[24] = {
#include "game/008CFF00.inc"
};
u32 StageObj_K11_Curves_18[24] = {
#include "game/008CFF60.inc"
};
u32 StageObj_K11_Curves_17[8] = {
#include "game/008CFFC0.inc"
};
u32 StageObj_K11_Curves_19[12] = {
#include "game/008CFFE0.inc"
};
u32 StageObj_K11_Curves_20[16] = {
#include "game/008D0010.inc"
};
u32 StageObj_K11_Curves_21[8] = {
#include "game/008D0050.inc"
};
u32 StageObj_K11_Curves_22[16] = {
#include "game/008D0070.inc"
};
u32 StageObj_K11_Curves_25[16] = {
#include "game/008D00B0.inc"
};
u32 StageObj_K11_Curves_23[28] = {
#include "game/008D00F0.inc"
};
u32 StageObj_K11_Curves_26[24] = {
#include "game/008D0160.inc"
};
u32 StageObj_K11_Curves_24[8] = {
#include "game/008D01C0.inc"
};
u32 StageObj_K11_Curves_27[8] = {
#include "game/008D01E0.inc"
};
u16 StageObj_K11_Tbls_11[4] = {
#include "game/008D0200.inc"
};
s16 StageObj_K11_DelaysA[4] = {
#include "game/008D0208.inc"
};
u32 StageObj_K11_Curves_28[16] = {
#include "game/008D0210.inc"
};
u32 StageObj_K11_Curves_29[12] = {
#include "game/008D0250.inc"
};
u32 StageObj_K11_Curves_30[28] = {
#include "game/008D0280.inc"
};
u32 StageObj_K11_Curves_31[12] = {
#include "game/008D02F0.inc"
};
u32 StageObj_K11_Curves_32[12] = {
#include "game/008D0320.inc"
};
u32 StageObj_K11_Curves_33[12] = {
#include "game/008D0350.inc"
};
u32 StageObj_K11_Curves_34[8] = {
#include "game/008D0380.inc"
};
u32 StageObj_K11_Curves_35[20] = {
#include "game/008D03A0.inc"
};
u32 StageObj_K11_Curves_37[32] = {
#include "game/008D03F0.inc"
};
u32 StageObj_K11_Curves_36[12] = {
#include "game/008D0470.inc"
};
u32 StageObj_K11_Curves_38[12] = {
#include "game/008D04A0.inc"
};
u8 StageObj_K11_CurveFA[32] = {
#include "game/008D04D0.inc"
};
u8 StageObj_K11_CurveFB[48] = {
#include "game/008D04F0.inc"
};
u8 StageObj_K11_CurveFC[32] = {
#include "game/008D0520.inc"
};
u16 StageObj_K11_Tbls_15[4] = {
#include "game/008D0540.inc"
};
s16 StageObj_K11_DelaysB[4] = {
#include "game/008D0548.inc"
};
u8 StageObj_K11_Curve10B[32] = {
#include "game/008D0550.inc"
};
u32 StageObj_K11_Curves_43[16] = {
#include "game/008D0570.inc"
};
u32 StageObj_K11_Curves_44[8] = {
#include "game/008D05B0.inc"
};
u32 StageObj_K11_Curves_45[12] = {
#include "game/008D05D0.inc"
};
u8 StageObj_K11_Curve12A[32] = {
#include "game/008D0600.inc"
};
u8 StageObj_K11_Curve12B[32] = {
#include "game/008D0620.inc"
};
void *StageObj_K11_Curves[48] = {
    (void *)StageObj_K11_Curves_0, (void *)StageObj_K11_Curves_1, (void *)StageObj_K11_Curves_2, (void *)StageObj_K11_Curves_3,
    (void *)StageObj_K11_Curves_4, (void *)StageObj_K11_Curves_5, (void *)StageObj_K11_Curves_6, (void *)StageObj_K11_Curves_7,
    (void *)StageObj_K11_Curves_8, (void *)StageObj_K11_Curves_9, (void *)StageObj_K11_Curves_10, (void *)StageObj_K11_Curves_11,
    (void *)StageObj_K11_Curves_12, (void *)StageObj_K11_Curves_13, (void *)StageObj_K11_Curves_14, (void *)StageObj_K11_Curves_15,
    (void *)StageObj_K11_Curves_16, (void *)StageObj_K11_Curves_17, (void *)StageObj_K11_Curves_18, (void *)StageObj_K11_Curves_19,
    (void *)StageObj_K11_Curves_20, (void *)StageObj_K11_Curves_21, (void *)StageObj_K11_Curves_22, (void *)StageObj_K11_Curves_23,
    (void *)StageObj_K11_Curves_24, (void *)StageObj_K11_Curves_25, (void *)StageObj_K11_Curves_26, (void *)StageObj_K11_Curves_27,
    (void *)StageObj_K11_Curves_28, (void *)StageObj_K11_Curves_29, (void *)StageObj_K11_Curves_30, (void *)StageObj_K11_Curves_31,
    (void *)StageObj_K11_Curves_32, (void *)StageObj_K11_Curves_33, (void *)StageObj_K11_Curves_34, (void *)StageObj_K11_Curves_35,
    (void *)StageObj_K11_Curves_36, (void *)StageObj_K11_Curves_37, (void *)StageObj_K11_Curves_38, (void *)StageObj_K11_Curves_37,
    (void *)StageObj_K11_Curves_38, (void *)StageObj_K11_Curves_37, (void *)StageObj_K11_Curves_38, (void *)StageObj_K11_Curves_43,
    (void *)StageObj_K11_Curves_44, (void *)StageObj_K11_Curves_45, NULL, NULL,
};
s16 StageObj_K11_PartCounts[24] = {
#include "game/008D0700.inc"
};
u16 StageObj_K11_Durations[24] = {
#include "game/008D0730.inc"
};
s16 StageObj_K11_CurveIdx[24] = {
#include "game/008D0760.inc"
};
s16 StageObj_K11_Steps[24] = {
#include "game/008D0790.inc"
};
u16 *StageObj_K11_Tbls[24] = {
    NULL, NULL, StageObj_K11_Tbls_2, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, StageObj_K11_Tbls_11,
    NULL, NULL, NULL, StageObj_K11_Tbls_15,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
