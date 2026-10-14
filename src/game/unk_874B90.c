/*
 * Game overlay code at 0x874B90-0x880D50, not identified yet: a
 * range of the overlay split off for the functions m2c drafts that match.
 */
#include "common.h"
#include "sprite.h"
#include "em.h"
#include "player.h"
#include "stageobj.h"
#include "lobby_vobj.h"
#include "scene.h"

/* the work block of the stun stars (kind 0x18) */
typedef struct StunWork {
    s16 slot[5];                /* 0x00: a draw slot per star */
    u8 unk0A[2];
    DrawSlot *node[5];             /* 0x0C */
    f32 ofs[3];                 /* 0x20: the circle's offset from the bone; ofs[2] its radius */
    u16 group;                  /* 0x2C: the owner's state (group, state) the stars last for */
    u16 state;                  /* 0x2E */
    s16 bone;                   /* 0x30: the bone they circle */
    s16 limit;                  /* 0x32: frames (monsters) */
} StunWork;

/* a piece of a kind 0x12 type 4 object (0x28 bytes) */
typedef struct Obj12Piece {
    u8 alive;                   /* 0x00 */
    u8 unk01;                   /* 0x01 */
    s16 slot;                   /* 0x02: its draw slot */
    DrawSlot *node;                /* 0x04 */
    f32 vel[3];                 /* 0x08 */
    f32 pos[3];                 /* 0x14 */
    u16 rot[3];                 /* 0x20 */
    u8 unk26[0x28 - 0x26];
} Obj12Piece;

/* a drop of a kind 0x15 object (0x34 bytes, three in its blocks) */
typedef struct Obj15Drop {
    u8 alive;                   /* 0x00 */
    u8 visible;                 /* 0x01 */
    u8 phase;                   /* 0x02: 0 wait, 1 fall, 2 bounce */
    u8 unk03;
    s16 timer;                  /* 0x04 */
    s16 slot;                   /* 0x06: its draw slot, -1: none */
    f32 pos[3];                 /* 0x08 */
    f32 vel[3];                 /* 0x14 */
    s32 rot[3];                 /* 0x20 */
    DrawSlot *node;                /* 0x2C */
    f32 scale;                  /* 0x30 */
} Obj15Drop;

/* a rock of a kind 0x11 type 8 object (0x30 bytes, ten in its blocks) */
typedef struct Obj11Rock {
    s16 slot;                   /* 0x00: its draw slot */
    s16 bounces;                /* 0x02: bounces left (0..2), -1: done */
    f32 vel[3];                 /* 0x04 */
    f32 pos[3];                 /* 0x10 */
    f32 scale;                  /* 0x1C */
    s16 unk20;                  /* 0x20 */
    u16 rot[3];                 /* 0x22 */
    s16 spin;                   /* 0x28: added to rot[2] each frame */
    u8 unk2A[0x2C - 0x2A];
    DrawSlot *node;                /* 0x2C */
} Obj11Rock;

/* a part of a kind 0x12 object of the default types (0x2C bytes, after a matrix in its blocks) */
typedef struct Obj12Part {
    f32 sc[3];                  /* 0x00: its scale (types 1 and 11: StageObj_K12_Scales by pair) */
    f32 alpha;                  /* 0x0C: 1.0 at first, then from a curve */
    f32 pos[3];                 /* 0x10 */
    DrawSlot *node;                /* 0x1C */
    s16 slot;                   /* 0x20: its draw slot */
    s16 idx;                    /* 0x22 */
    s16 delay;                  /* 0x24: frames before it starts (negative) */
    s16 unk26;                  /* 0x26 */
    u16 angle;                  /* 0x28 */
    s16 spin;                   /* 0x2A */
} Obj12Part;

/* a part of a kind 0x14 object (0x30 bytes, in its blocks) */
typedef struct Obj14Part {
    s8 flag;                    /* 0x00: -1: gone (type 0x10, when the owner goes) */
    u8 unk01;                   /* 0x01: nonzero: no alpha curve */
    s16 slot;                   /* 0x02: its draw slot */
    f32 sc[3];                  /* 0x04: its scale, from a curve */
    f32 pos[3];                 /* 0x10: type 0x10: its own position */
    f32 alpha;                  /* 0x1C: from a curve */
    s16 time;                   /* 0x20: frames since it started (negative: a delay) */
    u16 rot;                    /* 0x22 */
    s16 rotv;                   /* 0x24: added to rot every frame */
    u8 unk26[0x28 - 0x26];
    f32 size;                   /* 0x28: the object scale, with a random factor in some types */
    DrawSlot *node;                /* 0x2C: DrawSlot_Get(slot) */
} Obj14Part;

/* prototypes */
extern u8 StageObj_K1B_PartCounts[];
extern u8 StageObj_K1B_Models[];
extern u16 StageObj_K1B_Lives[];
void StageObj_K1B_Draw(void *arg);
void *memset(void *p, s32 c, u32 n);
extern s16 StageObj_K12_CurveIdx[];
extern s16 StageObj_K12_DursB[];
extern s16 StageObj_K12_DursC[];
extern s16 StageObj_K12_DursA[];
extern void *StageObj_K12_Curves[];
extern f32 StageObj_K12_Spin[];
void StageObj_K12_PlaySound(StageObj *o, f32 *pos);
void Effect0D_SpawnAt(f32 *pos, s32 a, f32 f);
extern s16 StageObj_K12_DelaysA[];
extern f32 StageObj_K12_Scales[];
void Vec_RotateY(f32 *v, f32 angle);
extern f32 StageObj_K13_MoveKeys[];
extern f32 *StageObj_K13_EmOffsets[];
extern f32 *StageObj_K13_Offset;
void Em_GetBonePos(void *obj, s16 bone, f32 *out);
f32 StageObj_RandUnit(void);
f32 StageObj_RandSigned(void);
void StageObj_K11_Draw(DrawSlot *node);
void Mtx_GetTrans(f32 *out, f32 *m);
void Vec_Add(f32 *out, f32 *a, f32 *b);
void Mtx_TurnY(f32 *m, f32 a);
void StageObj_K15_Draw(DrawSlot *node);
void Mtx_Translation(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetRotXYZ(f32 *m, f32 x, f32 y, f32 z);
u16 Rand_Next(s32 stream);
extern f32 StageObj_K13_ScaleKeys[];
extern f32 StageObj_K13_ScaleKeysB[];
extern s16 StageObj_K13_Models[];
void Gs_SetLightList(ModelMat *mats, ModelMatSet *set);
void Draw_ApplyStates(u32 a);
s32 Model_Draw(u32 h, s32 flags);
void Draw_SetDefaultState(void);
f32 AreaGrid2_GetFloorY(f32 *pos);
void Vec_RotateX(f32 *v, f32 a);
void StageObj_K12_Draw(DrawSlot *node);
void Key_LerpVec3(f32 *k, f32 *out, f32 t);
void Key_LerpF32(f32 *k, f32 *out, f32 t);
void Gfx_SetBlend(s32 a, s32 b);
void Mtx_FromScaleRotTrans(f32 *scale, f32 *rot, f32 *trans, u16 order, f32 *m);
void Mtx_Identity(f32 *m);
void Mtx_RotateX(f32 *m, f32 a);
void Mtx_RotateY(f32 *m, f32 a);
void Mtx_TurnZ(f32 *m, f32 a);
s32 Effect_DrawModel(void *mdl, f32 *m, s32 color, u16 flags, void *a4);
extern void *D_008D0F34;
extern void *D_008D0F38;
/* unit's own declaration: with the definition's s16 (void *obj, u8 state, u8 sub) (sub_main/unk_3A1070.c), StageObj_Stun_Run: 2 of 198 words differ */
s16 Player_IsState(void *obj, s32 group, s32 state);
EmBone *Em_GetBone(void *obj, s16 bone);
void Mtx_Copy(f32 *dst, f32 *src);
void StageObj_Stun_Draw(DrawSlot *node);
void Vec_RotateY(f32 *v, f32 a);
void Mtx_Scale(f32 *m, f32 x, f32 y, f32 z);
void Mtx_SetTrans(f32 *m, f32 x, f32 y, f32 z);
void Mtx_Concat3x3(f32 *m, f32 *n);
s32 Gs_SetState(u8 a, u32 b);
s32 Effect_DrawModelAdd(void *list, f32 *v, void *base);
extern f32 gCamMatrix[];
extern s16 StageObj_K14_GroundTbl[];
extern s16 gAreaKinds[];
u8 AreaGrid2_GetFloorHeight(f32 *pos, f32 *y);
f32 Area_GroundOffset(u16 area);
void Effect08_Spawn(f32 *pos, s32 a, s32 b, f32 f);
s32 Sound_PlaySeAtPosChecked(s32 slot, s32 id, s32 a, void *pos, s32 b, s32 c);
s16 StageObj_K14_PlaceAtEm(f32 *pos, Em *em, s16 k, s16 c);
void Eff01_CreateAt(Em *em, f32 *pos, s32 kind); /* unit's own declaration: the definition takes s8 kind; s8 here changes one function */ /* unit's own declaration: with the definition's void (Em *em, f32 *pos, s8 kind) (game/unk_6BB9D0.c), StageObj_K14_PlaceAtEm 22 of 1210 words differ */
s8 StageObj_K14_SpawnEffects(Em *em, f32 *pos, s32 mode, f32 scale);
EmBone *Em_Bone(void *obj, s16 bone);
void Vec_RotateSelf(f32 *v, f32 *m);
void Mtx_GetTrans(f32 *out, f32 *m);
s16 DrawSlot_Alloc(void);
DrawSlot *DrawSlot_Get(s32 i);
void StageObj_K13_Draw(DrawSlot *node);
s32 OT_AddByDepth(u32 *table, DrawSlot *node, u32 n, s32 clamp);
extern s16 StageObj_K12_Limits[];
void StageObj_K15_Update(StageObj *o);
void StageObj_Stun_Update(StageObj *o);
void StageObj_K12_Setup(StageObj *o);
void StageObj_K12_Run(StageObj *o);
void StageObj_K12_End(StageObj *o);
void StageObj_K12_Free(StageObj *o);
StageObj *StageObj_K12_Alloc(s32 k);
void StageObj_K13_Setup(StageObj *o);
void StageObj_K13_Run(StageObj *o);
void StageObj_K13_End(StageObj *o);
void StageObj_K13_Free(StageObj *o);
void StageObj_K14_Setup(StageObj *o);
void StageObj_K14_Run(StageObj *o);
void StageObj_K14_End(StageObj *o);
void StageObj_K14_Free(StageObj *o);
void StageObj_K15_Setup(StageObj *o);
void StageObj_K15_Run(StageObj *o);
void StageObj_K15_End(StageObj *o);
void StageObj_K15_Free(StageObj *o);
void StageObj_Stun_Setup(StageObj *o);
void StageObj_Stun_Run(StageObj *o);
void StageObj_Stun_End(StageObj *o);
void StageObj_Stun_Free(StageObj *o);
void StageObj_K1B_Setup(StageObj *o);
void StageObj_K1B_Run(StageObj *o);
void StageObj_K1B_End(StageObj *o);
void StageObj_K1B_Free(StageObj *o);
s32 Obj_InCurrentArea(void *);
void StageObj_K12_SetupDefault(StageObj *o);
void StageObj_K12_SetupType3(StageObj *o);
void StageObj_K12_SetupPieces(StageObj *o);
void StageObj_K12_DrawDefault(DrawSlot *node);
void StageObj_K12_DrawType3(DrawSlot *node);
void StageObj_K12_DrawPiece(DrawSlot *node);
void StageObj_K12_RunDefault(StageObj *o);
void StageObj_K12_RunType3(StageObj *o);
void StageObj_K12_RunPieces(StageObj *o);
void Vec_Copy(f32 *dst, f32 *src);
void DrawSlot_Free(s32 i);
void StageObj_K11_Update(StageObj *o);
f32 Vec_Distance(f32 *a, f32 *b);
extern s16 StageObj_K14_PartCounts[];
extern s16 StageObj_K11_PartCounts[];
extern s16 StageObj_K12_PartCounts[];
extern u8 StageObj_K12_GunlanceShellArgs[];
u8 Player_InCurrentArea(void *obj);
void Em_BonePos(void *obj, s16 bone, f32 *out);
void Color_EvalKeys(s32 *k, s32 t, u32 *out);
void Mtx_Billboard(f32 *m, f32 *pos);
s32 Gs_SetState(u8 a, u32 b);
extern u8 **StageObj_K12_Tbls[];
extern u8 *StageObj_K12_ColorKeysA[];
extern u8 *StageObj_K12_ColorKeysB[];
extern u8 *StageObj_K12_ColorKeysC[];
extern u16 StageObj_K14_Totals[];
extern u16 *StageObj_K14_DurLists[];
extern s16 StageObj_K14_Steps[];
extern s16 StageObj_K14_CurveIdx[];
extern void *StageObj_K14_Curves[];
extern f32 StageObj_K14_Drift[];
extern s16 StageObj_K14_Sectors[];
u16 Mtx_Heading(f32 *m);
void Eff04_CreateOnObj(StageObj *s, s8 kind);
void Eff01_CreateOnObj(StageObj *s, s8 kind);
s32 Em_TypeUsesK14(u8 type);
extern s16 *StageObj_K14_DelayLists[];
extern f32 StageObj_K14_Sizes[];
void StageObj_K14_Draw(DrawSlot *node);
/* end of prototypes */

void StageObj_K11_SetupRocks(StageObj *o)
{
    Obj11Rock *r = (Obj11Rock *) o->buf;
    s16 i;
    f32 f;

    switch (o->type) {
    case 8:
        f = 70.0f;
        break;
    }
    o->timer = 0;
    for (i = 0; i < 10; i++, r++) {
        r->slot = DrawSlot_Alloc();
        if (r->slot != -1) {
            r->bounces = Rand_Next(1) % 3;
            r->unk20 = 0;
            Vec_Copy(r->pos, o->pos);
            r->scale = o->scale * (1.5f * (1.0f + StageObj_RandUnit()));
            if (i < 5) {
                r->vel[0] = 40.0f * o->scale * StageObj_RandSigned();
                r->vel[1] = (o->scale * (100.0f + f * StageObj_RandUnit())) / r->scale;
                r->vel[2] = 40.0f * o->scale * StageObj_RandSigned();
            } else {
                r->vel[0] = 80.0f * o->scale * StageObj_RandSigned();
                r->vel[1] = (o->scale * (100.0f + f * StageObj_RandUnit())) / r->scale;
                r->vel[2] = 80.0f * o->scale * StageObj_RandSigned();
            }
            r->rot[0] = Rand_Next(1);
            r->rot[1] = Rand_Next(1);
            r->rot[2] = Rand_Next(1);
            r->spin = (Rand_Next(1) & 0xFFF) - 0x7FF;
            r->node = DrawSlot_Get(r->slot);
            r->node->owner = o;
            r->node->part = i;
            r->node->draw = StageObj_K11_Draw;
        } else {
            r->node = NULL;
        }
    }
}

void StageObj_K11_RunRocks(StageObj *o)
{
    Obj11Rock *r = (Obj11Rock *) o->buf;
    s16 i;
    s16 done;
    f32 y;

    if (++o->timer > 150) {
        o->state++;
        o->visible = 0;
        return;
    }
    done = 0;
    for (i = 0; i < 10; i++) {
        if (r->bounces == -1) {
            r++;
            done++;
            continue;
        }
        r->pos[0] += r->vel[0];
        r->pos[1] += r->vel[1];
        r->pos[2] += r->vel[2];
        r->vel[1] += -9.8f;
        r->rot[2] += r->spin;
        y = AreaGrid2_GetFloorY(r->pos);
        if (r->pos[1] <= y) {
            r->bounces--;
            if (r->bounces < 0) {
                r++;
                continue;
            }
            r->pos[1] = y;
            r->vel[1] *= -0.5f;
        }
        if (r->node != NULL) {
            r->node->pos[0] = r->pos[0];
            r->node->pos[1] = r->pos[1];
            r->node->pos[2] = r->pos[2];
            OT_AddByDepth(gDrawOt32, r->node, 0x20, 0);
        }
        r++;
    }
    if (done >= 10) {
        o->state++;
        o->visible = 0;
    }
}

void StageObj_K11_DrawRock(DrawSlot *node)
{
    f32 rot[3];
    f32 scale[3];
    f32 m[16];
    ModelMat *tex;
    ModelMatSet *model;
    Model *set = gStageModelSet;
    Obj11Rock *r = (Obj11Rock *) ((StageObj *) node->owner)->buf + node->part;

    if (set != NULL && set->active != 0) {
        tex = set->mats;
        rot[0] = (6.2831855f * r->rot[0]) / 65536.0f;
        rot[1] = (6.2831855f * r->rot[1]) / 65536.0f;
        rot[2] = (6.2831855f * r->rot[2]) / 65536.0f;
        scale[0] = r->scale;
        scale[1] = r->scale;
        scale[2] = r->scale;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 6, m);
        Mtx_Concat3x3(m, gCamMatrix);
        Gs_SetState(0x67, -1);
        model = &set->matSets[73];
        if (model != NULL && model->id != -1) {
            Gs_SetState(0x60, 0x80);
            Gs_SetState(0x1A, (s32) m);
            Gs_SetLightList(tex, model);
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void StageObj_K11_EndDefault(StageObj *o)
{
    u8 *p = o->buf;
    s16 n;
    s16 i;

    o->state++;
    n = StageObj_K11_PartCounts[o->type];
    switch (o->type) {
    case 3:
        if (o->var0 == 0) {
            n = 1;
        } else {
            n = o->var0;
        }
        break;
    }
    for (i = 0; i < n; i++) {
        if (*(s32 *) (p + 0x2C) != 0) {
            DrawSlot_Free(*(s16 *) p);
        }
        p += 0x30;
    }
}

void StageObj_K11_EndRocks(StageObj *o)
{
    u8 *p = o->buf;
    s16 i;

    o->state++;
    for (i = 0; i < 10; i++) {
        if (*(s32 *) (p + 0x2C) != 0) {
            DrawSlot_Free(*(s16 *) p);
        }
        p += 0x30;
    }
}

void StageObj_K11_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_K11_PlaceAtBone(StageObj *o)
{
    Em *em = o->owner;
    void *m;
    f32 v[3];

    if (em->type == 0x35) {
        m = Em_Bone(em, 1);
        v[0] = 0.0f;
        v[1] = 180.0f;
        v[2] = 330.0f;
    } else {
        m = Em_Bone(em, 0x25);
        v[0] = 0.0f;
        v[1] = -60.0f;
        v[2] = 140.0f;
    }
    Vec_RotateSelf(v, m);
    Mtx_GetTrans(o->pos, m);
    o->pos[0] += v[0];
    o->pos[1] += v[1];
    o->pos[2] += v[2];
}

s32 StageObj_K11_OwnerInMotion(StageObj *o)
{
    Em *t = o->owner;

    if (t->active == 0 || (t->motionId[0] != o->angle2 && t->motionId[0] != 0x45A)) {
        return 0;
    }
    return 1;
}

StageObj *StageObj_K11_Alloc(s16 k, s16 v)
{
    StageObj *o = StageObj_Alloc(1);

    if (o == NULL) {
        return NULL;
    }
    o->kind = 0x11;
    o->update = StageObj_K11_Update;
    o->type = k;
    o->angle = v;
    return o;
}

void StageObj_K11_CreateAtBone(Em *em, s16 bone, s16 k, u16 c)
{
    StageObj *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = StageObj_K11_Alloc(k, bone)) != NULL) {
            o->var0 = c;
            Em_BonePos(em, bone, o->pos);
            o->owner = em;
            switch (k) {
            case 1:
            case 2:
                if (c == 0) {
                    o->scale = 1.0f;
                } else if (c == 1) {
                    o->scale = 1.5f;
                } else {
                    o->scale = 0.5f;
                }
                break;
            case 7:
                StageObj_K11_PlaceAtBone(o);
                o->angle2 = em->motionId[0];
                o->var2 = em->type;
                o->scale = 0.3f;
                break;
            default:
                o->scale = 1.0f;
                break;
            }
        }
    }
}

void StageObj_K11_CreateType3(f32 *pos, u16 v, u16 w, u8 a, u8 b)
{
    StageObj *o = StageObj_K11_Alloc(3, w);

    if (o != NULL) {
        o->var0 = a;
        o->timer = v;
        o->var2 = b;
        o->scale = 1.0f;
        o->owner = NULL;
        Vec_Copy(o->pos, pos);
    }
}

void StageObj_K11_CreateAtScaled(f32 *pos, s32 v, s16 k, f32 scale)
{
    StageObj *o = StageObj_K11_Alloc(k, v);

    if (o != NULL) {
        o->scale = scale;
        o->owner = NULL;
        Vec_Copy(o->pos, pos);
    }
}

void StageObj_K11_CreateAtBoneB(Em *em, s16 bone, s16 kind, u16 arg3)
{
    StageObj *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        if ((o = StageObj_K11_Alloc(kind, bone)) != NULL) {
            o->var0 = arg3 >> 8;
            Em_BonePos(em, bone, o->pos);
            o->owner = em;
            o->scale = 1.0f;
        }
    }
}

void StageObj_K11_CreateAt(f32 *pos, s16 k, u8 c, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_K11_Alloc(k, 0)) != NULL) {
        o->var2 = c;
        Vec_Copy(o->pos, pos);
        o->owner = NULL;
        o->scale = scale;
    }
}

void StageObj_K11_CreateAtYaw(f32 *pos, s16 k, u8 c, s32 v, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_K11_Alloc(k, v)) != NULL) {
        o->var2 = c;
        Vec_Copy(o->pos, pos);
        o->owner = NULL;
        o->scale = scale;
    }
}

void StageObj_K12_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K12_Setup(o);
        break;
    case 1:
        StageObj_K12_Run(o);
        break;
    case 2:
        StageObj_K12_End(o);
        break;
    case 3:
        StageObj_K12_Free(o);
        break;
    }
}

void StageObj_K12_Setup(StageObj *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    default:
        StageObj_K12_SetupDefault(o);
        break;
    case 3:
        StageObj_K12_SetupType3(o);
        break;
    case 4:
        StageObj_K12_SetupPieces(o);
        break;
    }
}

void StageObj_K12_Run(StageObj *o)
{
    switch (o->type) {
    default:
        StageObj_K12_RunDefault(o);
        break;
    case 3:
        StageObj_K12_RunType3(o);
        break;
    case 4:
        StageObj_K12_RunPieces(o);
        break;
    }
}

void StageObj_K12_Draw(DrawSlot *node)
{
    switch (((StageObj *) node->owner)->type) {
    default:
        StageObj_K12_DrawDefault(node);
        break;
    case 3:
        StageObj_K12_DrawType3(node);
        break;
    case 4:
        StageObj_K12_DrawPiece(node);
        break;
    }
}

void StageObj_K12_SetupDefault(StageObj *o)
{
    f32 v[3];
    f32 m2[16];
    f32 m[16];
    u8 *w = o->buf;
    Em *em = o->owner;
    s16 n = StageObj_K12_PartCounts[o->type];
    Obj12Part *p = (Obj12Part *) (w + 0x40);
    s16 i;
    s16 k;
    u16 ang;
    f32 *t;

    switch (o->type) {
    case 0:
    case 1:
    case 5:
    case 6:
        v[0] = -10.0f;
        v[1] = -15.0f;
        v[2] = 87.0f;
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, o->angle)));
        Mtx_GetTrans(o->pos, m);
        Mtx_Copy(m2, m);
        Mtx_TurnY(m2, 3.1415927f);
        Vec_RotateSelf(v, m2);
        o->pos[0] += v[0];
        o->pos[1] += v[1];
        o->pos[2] += v[2];
        Mtx_Copy((f32 *) w, m2);
        break;
    case 10:
    case 11:
        Mtx_Identity((f32 *) w);
        Mtx_RotateX((f32 *) w, (6.2831855f * o->timer) / 65536.0f);
        Mtx_RotateY((f32 *) w, (6.2831855f * o->angle) / 65536.0f);
        break;
    }
    o->timer = 0;
    for (i = 0; i < n; p++, i++) {
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K12_Draw;
        } else {
            p->node = NULL;
        }
        switch (o->type) {
        case 0:
        case 10:
            p->idx = i;
            p->delay = 0;
            break;
        case 1:
        case 11:
            k = i / 2;
            p->idx = i & 1;
            if (p->idx == 0) {
                p->angle = Rand_Next(1);
                ang = p->angle;
                t = &StageObj_K12_Scales[k];
                p->sc[0] = *t;
                p->sc[1] = *t;
                p->sc[2] = *t;
            } else {
                p->angle = ang;
            }
            p->delay = -4;
            p->alpha = 1.0f;
            if (i == 2) {
                p->unk26 = 1;
            } else if (i == 4) {
                p->unk26 = 3;
            } else {
                p->unk26 = -1;
            }
            break;
        case 2:
            p->idx = ((30.0f * Rand_Next(1)) / 65535.0f - 15.0f) / 20.0f;
            p->angle = Rand_Next(1);
            p->delay = -4 - i * 3;
            break;
        case 12:
            p->idx = ((30.0f * Rand_Next(1)) / 65535.0f - 15.0f) / 20.0f;
            p->angle = Rand_Next(1);
            p->delay = -(o->var2 + i * 3);
            break;
        case 5:
            p->idx = i;
            switch (i) {
            case 2:
                p->delay = 0;
                p->angle = Rand_Next(1);
                break;
            case 4:
                p->delay = -1;
                p->angle = Rand_Next(1);
                break;
            default:
                p->delay = 0;
                p->angle = 0;
                break;
            }
            break;
        case 6:
            p->idx = i;
            p->angle = Rand_Next(1);
            p->delay = -5 - i * 2;
            break;
        case 7:
            if (i == 0) {
                p->idx = 0;
                p->angle = 0;
                p->delay = 0;
                Vec_Copy(p->pos, o->pos);
            } else {
                p->idx = i;
                p->angle = Rand_Next(1);
                p->spin = (Rand_Next(1) & 0x1FF) - 0x100;
                p->delay = -6 - (i - 1) * 5;
                p->pos[0] = o->pos[0] + 10.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
                p->pos[1] = o->pos[1] + 5.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
                p->pos[2] = o->pos[2] + 10.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            }
            break;
        case 8:
            p->delay = StageObj_K12_DelaysA[i];
            Vec_Copy(p->pos, o->pos);
            switch (i) {
            case 0:
            case 1:
                p->idx = i;
                p->angle = 0;
                break;
            case 2:
            case 3:
            case 4:
            default:
                p->idx = 2;
                p->angle = Rand_Next(1);
                break;
            }
            break;
        case 9:
            p->idx = i + 1;
            p->angle = Rand_Next(1);
            p->spin = (Rand_Next(1) & 0x1FF) - 0x100;
            p->delay = -6 - (i - 1) * 5;
            p->pos[0] = o->pos[0] + 10.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            p->pos[1] = o->pos[1] + 5.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            p->pos[2] = o->pos[2] + 10.0f * 0.001f * ((Rand_Next(1) & 0x3FF) - 0x200);
            break;
        }
    }
    StageObj_K12_Run(o);
}

void StageObj_K12_RunDefault(StageObj *o)
{
    f32 v[3];
    f32 d[3];
    f32 m[16];
    u8 *w = o->buf;
    s16 count = 0;
    Obj12Part *p = (Obj12Part *) (w + 0x40);
    s16 n = StageObj_K12_PartCounts[o->type];
    s16 c = StageObj_K12_CurveIdx[o->type];
    s16 limit = StageObj_K12_Limits[o->type];
    s16 i;
    s16 dur;
    s16 step;
    s32 ci;
    void *curve;
    f32 f;

    if (++o->timer > limit) {
        o->state++;
        o->visible = 0;
        return;
    }
    switch (o->type) {
    case 0:
    case 10:
        dur = 7;
        step = 1;
        break;
    case 2:
    case 12:
        dur = 0x14;
        step = 3;
        break;
    case 5:
        step = 1;
        break;
    case 6:
        dur = 0x11;
        step = 1;
        break;
    case 7:
        if (o->timer == 6) {
            StageObj_K12_PlaySound(o, o->pos);
        }
        step = 1;
        break;
    case 8:
        step = 1;
        if (o->timer == limit - 1) {
            Effect0D_SpawnAt(o->pos, 0x1D, 1.0f);
        }
        break;
    case 9:
        dur = 0x1E;
        step = 1;
        break;
    }
    for (i = 0; i < n; i++, p++) {
        switch (o->type) {
        case 1:
        case 11:
            if (p->idx == 0) {
                step = 1;
            } else {
                step = 3;
            }
            dur = StageObj_K12_DursB[i];
            break;
        case 2:
        case 12:
            c = 0xE;
            break;
        case 5:
            dur = StageObj_K12_DursC[i];
            break;
        case 6:
            c = 0x18;
            break;
        case 7:
            if (p->idx == 0) {
                dur = 4;
            } else {
                dur = 0x1E;
            }
            break;
        case 8:
            dur = StageObj_K12_DursA[i];
            break;
        }
        if (++p->delay <= 0) {
            c += step;
            continue;
        }
        if (p->delay == 1) {
            if (o->type == 2 || o->type == 6) {
                if (p->node != NULL) {
                    v[0] = -10.0f;
                    v[1] = -15.0f;
                    v[2] = 87.0f;
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, o->angle)));
                    Mtx_GetTrans(p->node->pos, m);
                    Mtx_TurnY(m, 3.1415927f);
                    Vec_RotateSelf(v, m);
                    p->node->pos[0] += v[0];
                    p->node->pos[1] += v[1];
                    p->node->pos[2] += v[2];
                }
            } else if (o->type == 12) {
                if (p->node != NULL) {
                    v[0] = 110.0f;
                    v[1] = 16.0f;
                    v[2] = 170.0f;
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, o->angle)));
                    Mtx_GetTrans(p->node->pos, m);
                    Vec_RotateSelf(v, m);
                    p->node->pos[0] += v[0];
                    p->node->pos[1] += v[1];
                    p->node->pos[2] += v[2];
                }
            }
        } else if (p->delay > dur) {
            c += step;
            if (o->type == 12) {
                if (++count >= n) {
                    o->state++;
                    o->visible = 0;
                    return;
                }
            }
            continue;
        }
        switch (o->type) {
        case 0:
        case 5:
        case 10:
            curve = StageObj_K12_Curves[c++];
            Key_LerpVec3(curve, p->sc, p->delay);
            if (o->type == 5 && p->idx == 1) {
                p->sc[0] *= 0.75f;
                p->sc[1] *= 0.75f;
                p->sc[2] *= 0.75f;
            }
            if (p->node != NULL) {
                p->node->pos[0] = o->pos[0];
                p->node->pos[1] = o->pos[1];
                p->node->pos[2] = o->pos[2];
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 1:
        case 11:
            curve = StageObj_K12_Curves[c++];
            Key_LerpVec3(curve, v, p->delay);
            Vec_RotateSelf((f32 *) v, (f32 *) w);
            p->angle += (u16) (s32) (0.5f + (65536.0f * StageObj_K12_Spin[i / 2]) / 360.0f);
            if (p->idx == 0) {
                p->unk26++;
            } else {
                d[0] = 0.0f;
                d[1] = 0.0f;
                d[2] = i * -5;
                Vec_RotateSelf(d, gCamMatrix);
                v[0] += d[0];
                v[1] += d[1];
                v[2] += d[2];
                ci = c;
                curve = StageObj_K12_Curves[ci];
                Key_LerpVec3(curve, p->sc, p->delay);
                c += 2;
                Key_LerpF32(StageObj_K12_Curves[(s16) (ci + 1)], &p->alpha, p->delay);
            }
            if (p->node != NULL) {
                p->node->pos[0] = o->pos[0] + v[0];
                p->node->pos[1] = o->pos[1] + v[1];
                p->node->pos[2] = o->pos[2] + v[2];
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 2:
        case 12:
            ci = c;
            p->angle += p->idx;
            Key_LerpVec3(StageObj_K12_Curves[ci], v, p->delay);
            curve = StageObj_K12_Curves[(s16) (ci + 1)];
            Key_LerpVec3(curve, p->sc, p->delay);
            c += 3;
            Key_LerpF32(StageObj_K12_Curves[(s16) (ci + 2)], &p->alpha, p->delay);
            if (p->node != NULL) {
                p->node->pos[1] += 0.5f;
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 6:
            p->angle += 0xA1;
            curve = StageObj_K12_Curves[c++];
            Key_LerpVec3(curve, p->sc, p->delay);
            f = 1.4f - 0.2f * i;
            p->sc[0] *= f;
            p->sc[1] *= f;
            p->sc[2] *= f;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = (100.0f - 20.0f * i) / dur;
            if (p->node != NULL) {
                Vec_RotateSelf((f32 *) v, (f32 *) w);
                p->node->pos[0] += v[0];
                p->node->pos[1] += v[1];
                p->node->pos[2] += v[2];
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 7:
        case 9:
            curve = StageObj_K12_Curves[c++];
            Key_LerpVec3(curve, p->sc, p->delay);
            p->sc[0] *= o->scale;
            p->sc[1] *= o->scale;
            p->sc[2] *= o->scale;
            if (p->node != NULL) {
                if (p->idx == 0) {
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = 30.0f;
                    Vec_RotateSelf(v, gCamMatrix);
                    p->node->pos[0] = p->pos[0] + v[0];
                    p->node->pos[1] = p->pos[1] + v[1];
                    p->node->pos[2] = p->pos[2] + v[2];
                } else {
                    p->angle += p->spin;
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = 5.0f * i;
                    p->pos[1] += 2.0f;
                    p->node->pos[0] = p->pos[0] + v[0];
                    p->node->pos[1] = p->pos[1] + v[1];
                    p->node->pos[2] = p->pos[2] + v[2];
                }
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        case 8:
            curve = StageObj_K12_Curves[c++];
            Key_LerpVec3(curve, p->sc, p->delay);
            if (p->node != NULL) {
                p->node->pos[0] = p->pos[0];
                p->node->pos[1] = p->pos[1];
                p->node->pos[2] = p->pos[2];
                OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
            }
            break;
        }
    }
}

void StageObj_K12_DrawDefault(DrawSlot *node)
{
    StageObj *o = node->owner;
    ModelMat *mats;
    f32 *w = (f32 *) o->buf;
    Obj12Part *p = (Obj12Part *) ((u8 *) w + node->part * sizeof(Obj12Part) + 0x40);
    u8 **tbl;
    s16 id;
    u16 mode = 0;
    Model *set = gStageModelSet;
    u32 color;
    f32 *sy;
    f32 *sz;
    f32 f;
    f32 rot[3];
    f32 scale[3];
    f32 m[16];
    f32 tm[16];

    if (set != NULL && set->active) {
        mats = set->mats;
        switch (o->type) {
        case 0:
        case 10:
            mode |= 2;
            Gs_SetState(0x6C, 0);
            switch (p->idx) {
            case 0:
                rot[0] = 0.0f;
                id = 0x10;
                rot[1] = 0.0f;
                rot[2] = 0.0f;
                Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 0, m);
                Mtx_Concat3x3(m, gCamMatrix);
                break;
            case 1:
                if (p->delay == 4) {
                    id = 0x12;
                } else {
                    id = 0x11;
                }
                Mtx_Copy(m, w);
                Mtx_Billboard(m, o->pos);
                m[0] *= p->sc[0];
                m[1] *= p->sc[0];
                m[2] *= p->sc[0];
                m[4] *= p->sc[1];
                m[5] *= p->sc[1];
                m[6] *= p->sc[1];
                m[8] *= p->sc[2];
                m[9] *= p->sc[2];
                m[10] *= p->sc[2];
                Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
                break;
            }
            color = 0xFFFFFFFF;
            break;
        case 1:
        case 11:
            Gs_SetState(0x6C, 0);
            if (p->idx == 0) {
                mode |= 2;
                id = 0x4E;
                Mtx_Translation(tm, 0.25f * (p->unk26 & 3), 0.25f * (p->unk26 / 4), 0.0f);
                Gs_SetState(0x19, (s32) tm);
            } else {
                id = 0x14;
            }
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            color = ((u8) (255.0f * p->alpha) << 24) | 0xFFFFFF;
            break;
        case 2:
        case 12:
            id = 0x14;
            if (o->type == 12) {
                scale[0] = 2.0f * p->sc[0];
                scale[1] = 2.0f * p->sc[1];
                scale[2] = 2.0f * p->sc[2];
            } else {
                Vec_Copy(scale, p->sc);
            }
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            color = ((u8) (255.0f * p->alpha) << 24) | 0xFFFFFF;
            break;
        case 5:
            mode |= 2;
            Gs_SetState(0x6C, 0);
            sy = &scale[1];
            sz = &scale[2];
            scale[0] = 3.0f * p->sc[0];
            *sy = 3.0f * p->sc[1];
            *sz = 3.0f * p->sc[2];
            tbl = StageObj_K12_Tbls[p->idx];
            if (tbl == NULL) {
                color = 0xFFFFFFFF;
            } else {
                Color_EvalKeys((s32 *) tbl[o->var2], p->delay, &color);
            }
            switch (p->idx) {
            case 0:
            case 1:
                id = 0x79;
                Mtx_Copy(m, w);
                Mtx_Billboard(m, o->pos);
                m[0] *= scale[0];
                m[1] *= scale[0];
                m[2] *= scale[0];
                f = *sy;
                m[4] *= f;
                m[5] *= f;
                m[6] *= f;
                f = *sz;
                m[8] *= f;
                m[9] *= f;
                m[10] *= f;
                Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
                break;
            case 2:
                id = 0x42;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
                Mtx_Concat3x3(m, gCamMatrix);
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                break;
            case 3:
                id = 0x40;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
                Mtx_Concat3x3(m, gCamMatrix);
                break;
            case 4:
                id = 0x23;
                rot[2] = (6.2831855f * p->angle) / 65536.0f;
                Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
                Mtx_Concat3x3(m, gCamMatrix);
                break;
            }
            switch (o->var2) {
            case 0:
            case 5:
            case 6:
            case 7:
            case 9:
            case 10:
            case 11:
            case 12:
                mode |= 2;
                break;
            }
            break;
        case 6:
            scale[0] = 3.0f * p->sc[0];
            scale[1] = 3.0f * p->sc[1];
            scale[2] = 3.0f * p->sc[2];
            id = 0x48;
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            Color_EvalKeys((s32 *) StageObj_K12_ColorKeysA[o->var2], p->delay, &color);
            switch (o->var2) {
            case 0:
            case 5:
            case 6:
            case 7:
            case 9:
            case 10:
            case 11:
            case 12:
                mode |= 2;
                break;
            }
            break;
        case 7:
        case 9:
            if (p->idx == 0) {
                id = 0x40;
                mode |= 2;
                color = 0xFFFFFF7F;
                scale[0] = 4.0f * p->sc[0];
                scale[1] = 4.0f * p->sc[1];
                scale[2] = 4.0f * p->sc[2];
            } else {
                id = 0x48;
                Color_EvalKeys((s32 *) StageObj_K12_ColorKeysB[o->var2], p->delay, &color);
                scale[0] = 2.0f * (1.0f - 0.1f * (p->idx - 1)) * p->sc[0];
                scale[1] = 2.0f * (1.0f - 0.1f * (p->idx - 1)) * p->sc[1];
                scale[2] = 2.0f * (1.0f - 0.1f * (p->idx - 1)) * p->sc[2];
            }
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        case 8:
            mode |= 2;
            switch (p->idx) {
            case 0:
                id = 0x40;
                break;
            case 1:
                id = 0x58;
                break;
            case 2:
                id = 0x7A;
                break;
            }
            Color_EvalKeys((s32 *) StageObj_K12_ColorKeysC[p->idx], p->delay, &color);
            rot[2] = (6.2831855f * p->angle) / 65536.0f;
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            break;
        }
        Effect_DrawModel(&set->matSets[id], m, color, mode, mats);
        Gfx_SetBlend(4, 5);
        Gs_SetState(0x6C, 1);
    }
}

void StageObj_K12_SetupType3(StageObj *o)
{
    o->angle = Rand_Next(1);
    o->timer = 0;
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->draw = StageObj_K12_Draw;
        Vec_Copy(o->node->pos, o->pos);
    } else {
        StageObj_Free(o);
    }
}

void StageObj_K12_RunType3(StageObj *o)
{
    if (++o->timer > StageObj_K12_Limits[o->type]) {
        o->state++;
        o->visible = 0;
        return;
    }
    OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
}

void StageObj_K12_DrawType3(DrawSlot *node)
{
    f32 alpha;
    f32 scale[3];
    f32 rot[3];
    f32 m[16];
    StageObj *o = node->owner;
    ModelMat *mats;
    ModelMatSet *mesh;
    Model *mdl = gCommonModels[0];

    if (mdl != NULL && mdl->active != 0) {
        mats = mdl->mats;
        Key_LerpVec3(D_008D0F34, scale, o->timer);
        Key_LerpF32(D_008D0F38, &alpha, o->timer);
        rot[2] = (6.2831855f * o->angle) / 65536.0f;
        Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
        Mtx_Concat3x3(m, gCamMatrix);
        mesh = &mdl->matSets[88];
        Gfx_SetBlend(4, 1);
        Effect_DrawModel(mesh, m, ((u8) (255.0f * alpha) << 24) | 0xFFFFFF, 2, mats);
        Gfx_SetBlend(4, 5);
    }
}

void StageObj_K12_SetupPieces(StageObj *o)
{
    f32 d[3];
    f32 v[3];
    Obj12Piece *p = (Obj12Piece *) o->buf;
    s16 i;
    u16 rx;
    u16 ry;
    s16 n = StageObj_K12_PartCounts[o->type];

    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = o->scale;
    rx = o->var0 << 8;
    ry = o->var1 << 8;
    Vec_RotateX(v, (6.2831855f * rx) / 65536.0f);
    Vec_RotateY(v, (6.2831855f * ry) / 65536.0f);
    o->timer = 0;
    for (i = 0; i < n; i++, p++) {
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->alive = 1;
            p->unk01 = 1;
            p->vel[0] = v[0];
            p->vel[1] = v[1];
            p->vel[2] = v[2];
            p->rot[0] = rx;
            p->rot[1] = ry;
            if (i == 0) {
                p->rot[2] = 0;
            } else {
                p->rot[2] = 0x8000;
            }
            Vec_Copy(p->pos, o->pos);
            d[0] = 0.0f;
            d[1] = 3.0f;
            d[2] = 10.0f;
            Vec_RotateY(d, (6.2831855f * (p->rot[1] + (i ? -0x4000 : 0x4000))) / 65536.0f);
            p->vel[0] += d[0];
            p->vel[1] += d[1];
            p->vel[2] += d[2];
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = StageObj_K12_Draw;
        } else {
            p->node = NULL;
        }
    }
    StageObj_K12_RunPieces(o);
}

void StageObj_K12_RunPieces(StageObj *o)
{
    Obj12Piece *p = (Obj12Piece *) o->buf;
    s16 n = StageObj_K12_PartCounts[o->type];
    s16 i;
    u8 alive = 0;

    if (++o->timer > StageObj_K12_Limits[o->type]) {
        o->state++;
        o->visible = 0;
    }
    for (i = 0; i < n; i++) {
        if (p->alive == 0) {
            p++;
            continue;
        }
        alive = 1;
        p->pos[0] += p->vel[0];
        p->pos[1] += p->vel[1];
        p->pos[2] += p->vel[2];
        p->rot[0] -= 0x180;
        if (i == 0) {
            p->rot[1] += 0x200;
        } else {
            p->rot[1] -= 0x200;
        }
        p->vel[1] += -2.5f;
        p->vel[0] *= 0.9f;
        p->vel[1] *= 0.9f;
        p->vel[2] *= 0.9f;
        if (p->pos[1] <= AreaGrid2_GetFloorY(p->pos)) {
            p->alive = 0;
        }
        if (p->node != NULL) {
            Vec_Copy(p->node->pos, p->pos);
            OT_AddByDepth(gDrawOt32, p->node, 0x20, 0);
        }
        p++;
    }
    if (alive == 0) {
        o->state++;
        o->visible = 0;
    }
}

void StageObj_K12_DrawPiece(DrawSlot *node)
{
    Model *mdl = gCommonModels[0];
    Obj12Piece *p = (Obj12Piece *) ((StageObj *) node->owner)->buf + node->part;
    ModelMatSet *mesh;
    ModelMat *tex;
    f32 m[16];

    if (mdl != NULL && mdl->active != 0) {
        tex = mdl->mats;
        mesh = &mdl->matSets[89];
        Mtx_Identity(m);
        Mtx_RotateX(m, (6.2831855f * p->rot[0]) / 65536.0f);
        Mtx_RotateY(m, (6.2831855f * p->rot[1]) / 65536.0f);
        Mtx_TurnZ(m, (6.2831855f * p->rot[2]) / 65536.0f);
        Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
        Effect_DrawModelAdd(mesh, m, tex);
    }
}

void StageObj_K12_End(StageObj *o)
{
    Obj12Part *q;
    u8 *p = o->buf;
    s16 n;
    s16 i;

    q = (Obj12Part *) (p + 0x40);
    o->state++;
    n = StageObj_K12_PartCounts[o->type];
    if (n == 0) {
        DrawSlot_Free(o->slot);
    } else {
        for (i = 0; i < n; i++) {
            if (o->type == 4) {
                if (((Obj12Piece *) p)->node != NULL) {
                    DrawSlot_Free(((Obj12Piece *) p)->slot);
                }
                p += 0x28;
            } else {
                if (q->node != NULL) {
                    DrawSlot_Free(q->slot);
                }
                q++;
            }
        }
    }
}

void StageObj_K12_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_K12_PlaySound(StageObj *o, f32 *pos)
{
    switch (o->type) {
    case 7:
        switch (o->var2) {
        case 0:
            break;
        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
            Sound_PlaySeAtPosChecked(1, 0x26, 0, pos, 1, 0);
            break;
        }
        break;
    }
}

StageObj *StageObj_K12_Alloc(s32 k)
{
    StageObj *o;

    switch ((s16) k) {
    default:
        o = StageObj_Alloc(1);
        break;
    case 3:
        o = StageObj_Alloc(0);
        break;
    }
    if (o != NULL) {
        o->kind = 0x12;
        o->update = StageObj_K12_Update;
        o->type = k;
        o->scale = 1.0f;
    }
    return o;
}

void StageObj_K12_CreateOwned(s32 owner, s16 v, s32 k)
{
    StageObj *o;

    if (Obj_InCurrentArea((void *) owner) & 0xFF) {
        o = StageObj_K12_Alloc(k);
        if (o != NULL) {
            o->angle = v;
            o->owner = (void *) owner;
        }
    }
}

void StageObj_K12_CreateAt(f32 *pos, s32 k, u8 c)
{
    StageObj *o = StageObj_K12_Alloc(k);

    if (o != NULL) {
        o->var2 = c;
        Vec_Copy(o->pos, pos);
    }
}

void StageObj_K12_CreateAtScaled(f32 *pos, s32 k, u8 c, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_K12_Alloc(k)) != NULL) {
        o->var2 = c;
        Vec_Copy(o->pos, pos);
        o->scale = scale;
    }
}

void StageObj_K12_CreateBetween(u8 *src, s32 k, u8 c)
{
    StageObj *o;

    if ((o = StageObj_K12_Alloc(k)) != NULL) {
        o->var0 = *(s32 *) (src + 0x24) >> 8;
        o->var1 = *(s32 *) (src + 0x28) >> 8;
        o->var2 = c;
        Vec_Copy(o->pos, (f32 *) (src + 0x30));
        o->scale = 0.5f * Vec_Distance((f32 *) (src + 0x30), (f32 *) (src + 0x3C));
    }
}

void StageObj_K12_CreateOnEm(Player *p, s16 v, s32 k, u8 c)
{
    StageObj *o;

    if (Obj_InCurrentArea(p) & 0xFF) {
        o = StageObj_K12_Alloc(k);
        if (o != NULL) {
            o->var1 = (s8) ((s32) p->rot[1] >> 8);
            o->var2 = c;
            o->angle = v;
            o->owner = p;
        }
    }
}

void StageObj_K12_CreateAtTimed(f32 *pos, s32 k, u16 a, u16 b)
{
    StageObj *o;

    if ((o = StageObj_K12_Alloc(k)) != NULL) {
        o->timer = a;
        o->angle = b;
        Vec_Copy(o->pos, pos);
        o->owner = NULL;
    }
}

void StageObj_K12_CreateGunlanceShell(Player *p, s16 i)
{
    StageObj_K12_CreateOnEm(p, 0xE, 0xC, StageObj_K12_GunlanceShellArgs[i]);
}

void StageObj_K13_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K13_Setup(o);
        break;
    case 1:
        StageObj_K13_Run(o);
        break;
    case 2:
        StageObj_K13_End(o);
        break;
    case 3:
        StageObj_K13_Free(o);
        break;
    }
}

void StageObj_K13_Setup(StageObj *o)
{
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    if (o->type == 0) {
        o->var0 = 0;
        o->timer = 0xF;
    } else if (o->type == 1) {
        o->var0 = 3;
        o->timer = 0;
    }
    o->slot = DrawSlot_Alloc();
    if (o->slot != -1) {
        o->node = DrawSlot_Get(o->slot);
        o->node->owner = o;
        o->node->part = 0;
        o->node->draw = StageObj_K13_Draw;
    } else {
        o->node = NULL;
    }
}

void StageObj_K13_Run(StageObj *o)
{
    Em *pl;
    Em *em;
    u8 *owner;
    f32 *ofs;
    u8 k;
    s32 t;
    f32 pos[3];
    f32 v[3];
    f32 m[16];

    if (o->var2 == 0) {
        pl = (Em *) (owner = o->owner);
    } else {
        em = (Em *) (owner = o->owner);
    }
    if (owner == NULL || owner[0] == 0 || owner[4] == 2) {
        o->state++;
        return;
    }
    v[0] = 0.0f;
    v[1] = 0.0f;
    v[2] = 0.0f;
    if (o->var2 != 0) {
        if (*((s8 *) em + 0x7D8) == 0 && *((s8 *) em + 0x7D9) == 0) {
            k = 0;
        } else if (*((s8 *) em + 0x7D9) != 0) {
            k = 2;
        } else if (*((s8 *) em + 0x7D8) != 0) {
            k = 1;
        } else {
            k = 0;
        }
        if (k == 0) {
            if (++o->timer > 0x3C || o->var0 == 0) {
                o->timer = 0x3C;
                o->var0 = 0;
                return;
            }
            t = o->timer;
            t = (t > 0xF) ? 0xF : t;
            Key_LerpVec3(StageObj_K13_MoveKeys, v, t);
        } else {
            o->var0 = k;
            o->timer = 0;
        }
        Em_BonePos(em, o->angle, pos);
        Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, o->angle)));
        ofs = StageObj_K13_EmOffsets[em->type];
    } else {
        if (++o->timer > 0x3C) {
            o->state++;
            return;
        }
        Em_GetBonePos(pl, o->angle, pos);
        Mtx_Copy(m, gCamMatrix);
        ofs = StageObj_K13_Offset;
    }
    v[0] += *ofs++;
    v[1] += *ofs++;
    v[2] += *ofs;
    Vec_RotateSelf(v, m);
    if (o->node != NULL) {
        o->node->pos[0] = pos[0] + v[0];
        o->node->pos[1] = pos[1] + v[1];
        o->node->pos[2] = pos[2] + v[2];
        OT_AddByDepth(gDrawOtSorted, o->node, 0x40, 0);
    }
}

void StageObj_K13_End(StageObj *o)
{
    o->state++;
    if (o->node != NULL) {
        DrawSlot_Free(o->slot);
    }
}

void StageObj_K13_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_K13_Draw(DrawSlot *node)
{
    f32 m[16];
    f32 scale[3];
    s32 t;
    StageObj *o;
    ModelMat *tex;
    ModelMatSet *model;
    Model *mdl;

    mdl = gCommonModels[0];
    o = node->owner;

    if (mdl != NULL && mdl->active != 0 && (Obj_InCurrentArea(o->owner) & 0xFF)) {
        tex = mdl->mats;
        switch (o->type) {
        case 0:
            t = (o->timer > 0xF) ? 0xF : o->timer;
            Key_LerpVec3(StageObj_K13_ScaleKeys, scale, t);
            scale[0] *= 1.5f;
            scale[1] *= 1.5f;
            scale[2] *= 1.5f;
            break;
        default:
            Key_LerpVec3(StageObj_K13_ScaleKeysB, scale, o->timer);
            break;
        }
        Mtx_Scale(m, scale[0], scale[1], scale[2]);
        Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
        Mtx_Concat3x3(m, gCamMatrix);
        model = &mdl->matSets[StageObj_K13_Models[o->var0]];
        if (model != NULL && model->id != -1) {
            Gs_SetState(0x60, 0);
            Gs_SetState(0x1A, (s32) m);
            Gs_SetState(0x67, -1);
            Gs_SetLightList(tex, model);
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void StageObj_K13_Create(void *owner, s16 v, u8 k)
{
    StageObj *o = StageObj_Alloc(0);

    if (o != NULL) {
        o->kind = 0x13;
        o->update = StageObj_K13_Update;
        o->type = k;
        o->var2 = 1;
        o->angle = v;
        o->owner = owner;
    }
}

void StageObj_K14_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K14_Setup(o);
        break;
    case 1:
        StageObj_K14_Run(o);
        break;
    case 2:
        StageObj_K14_End(o);
        break;
    case 3:
        StageObj_K14_Free(o);
        break;
    }
}

void StageObj_K14_Setup(StageObj *o)
{
    s16 n;
    Em *em;
    s16 *delays;
    Obj14Part *p;
    s16 fx;
    s16 i;
    f32 m[16];

    p = (Obj14Part *) o->buf;
    em = o->owner;
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    o->timer = 0;
    n = StageObj_K14_PartCounts[o->type];
    delays = StageObj_K14_DelayLists[o->type];
    if (o->var2 & 0x80) {
        fx = 0;
        o->var2 ^= 0x80;
    } else {
        fx = 1;
    }
    if (em != NULL) {
        o->angle = em->rotY;
        if (o->var1 != 1) {
            switch (o->type) {
            case 0:
            case 1:
            case 9:
            case 11:
            case 14:
            case 18:
            case 19:
                switch (em->type) {
                case 2:
                case 0x24:
                case 0x47:
                    if (fx) {
                        switch (o->type) {
                        case 1:
                            Eff04_CreateOnObj(o, 0x15);
                            break;
                        }
                    }
                    break;
                case 0x1:
                case 0x2A:
                case 0x25:
                case 0x6:
                case 0x26:
                case 0x8:
                case 0xB:
                case 0x29:
                case 0x31:
                case 0xE:
                case 0x2B:
                case 0xF:
                case 0x2D:
                case 0x11:
                case 0x2F:
                case 0x14:
                case 0x27:
                case 0x15:
                case 0x2E:
                case 0x16:
                case 0x1A:
                case 0x2C:
                case 0x22:
                case 0x28:
                    if (fx) {
                        switch (o->type) {
                        case 1:
                            if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                                Eff01_CreateOnObj(o, 0x21);
                            } else {
                                Eff01_CreateOnObj(o, 0x2C);
                            }
                            break;
                        case 9:
                            if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                                Eff01_CreateOnObj(o, 0x27);
                            } else {
                                Eff01_CreateOnObj(o, 0x2E);
                            }
                            break;
                        case 11:
                            if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                                Eff01_CreateOnObj(o, 0x26);
                            } else {
                                Eff01_CreateOnObj(o, 0x2D);
                            }
                            break;
                        }
                    }
                    break;
                }
                break;
            case 2:
            case 3:
                switch (o->var2) {
                case 0x1B:
                case 0x1C:
                case 0x21:
                case 0x22:
                case 0x23:
                case 0x24:
                case 0x1E:
                case 0x2B:
                case 0x2F:
                    o->angle += (s16) 0x8000;
                    break;
                }
                break;
            case 7:
                o->scale *= 0.7f + 0.0002f * (Rand_Next(1) & 0x3FF);
                break;
            case 10:
                switch (em->type) {
                case 2:
                case 0x24:
                case 0x47:
                    if (fx) {
                        Eff04_CreateOnObj(o, 0x15);
                    }
                    switch (o->var2) {
                    case 0xB:
                    case 0x30:
                        o->angle += (s16) 0x8000;
                        break;
                    }
                    break;
                case 0x1:
                case 0x2A:
                case 0x25:
                case 0x6:
                case 0x26:
                case 0x8:
                case 0xB:
                case 0x29:
                case 0x31:
                case 0xE:
                case 0x2B:
                case 0xF:
                case 0x2D:
                case 0x11:
                case 0x2F:
                case 0x14:
                case 0x27:
                case 0x15:
                case 0x2E:
                case 0x16:
                case 0x1A:
                case 0x2C:
                case 0x22:
                case 0x28:
                    if (fx) {
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            Eff01_CreateOnObj(o, 0x20);
                        } else {
                            Eff01_CreateOnObj(o, 0x30);
                        }
                    }
                    break;
                }
                break;
            case 25:
                o->var0 = Rand_Next(1) & 1;
                o->var1 = em->type;
                if (o->var1 == 2 || o->var1 == 0x24 || o->var1 == 0x47) {
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, 0x33)));
                } else {
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, 0x24)));
                }
                o->angle = Mtx_Heading(m) + 0x4000;
                break;
            case 26:
                o->var1 = em->type;
                if (o->var1 == 2 || o->var1 == 0x24 || o->var1 == 0x47) {
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, 0x33)));
                } else {
                    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, 0x24)));
                    if (o->var1 == 0x16) {
                        if (em->stateGroup == 3 && em->state == 4 && em->motionId[0] == 0x456) {
                            o->var0 = 1;
                        } else {
                            o->var0 = 0;
                        }
                    }
                }
                o->angle = Mtx_Heading(m) + 0x4000;
                break;
            }
        }
    }
    for (i = 0; i < n; p++, i++) {
        if (delays != NULL) {
            p->time = delays[i];
        } else {
            p->time = 0;
        }
        p->size = o->scale;
        p->unk01 = 0;
        switch (o->type) {
        case 0:
            p->flag = i;
            p->rot = Rand_Next(1);
            p->rotv = (Rand_Next(1) & 0x1F) - 10;
            break;
        case 1:
            p->flag = i;
            p->rot = Rand_Next(1);
            p->rotv = 0.5f + (65536.0f * (0.15f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            p->size = 10.0f * o->scale;
            break;
        case 2:
        case 3:
            p->rot = Rand_Next(1);
            if (i < 2) {
                p->flag = 0;
                p->rotv = 0.5f + (65536.0f * (0.4f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            } else {
                p->flag = 1;
                if (o->var1 == 1) {
                    p->size *= 0.4f;
                }
                p->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            }
            break;
        case 4:
            p->rot = Rand_Next(1);
            if (i == 0) {
                p->flag = 0;
                p->rotv = 0.5f + (65536.0f * (0.3f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            } else {
                p->flag = 1;
                p->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            }
            break;
        case 5:
            p->rot = Rand_Next(1);
            if (i < 5) {
                p->flag = 0;
                p->rotv = 0.5f + (65536.0f * (0.4f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            } else {
                p->flag = 1;
                p->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            }
            break;
        case 6:
            p->flag = Rand_Next(1) & 1;
            p->size = 0.8f + 0.00020000001f * (Rand_Next(1) & 0x3FF);
            p->rot = Rand_Next(1);
            break;
        case 7:
        case 17:
            p->flag = 0;
            p->rot = Rand_Next(1);
            break;
        case 8:
            p->flag = i;
            p->rot = Rand_Next(1);
            break;
        case 9:
            p->flag = i;
            p->rot = Rand_Next(1);
            p->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            p->size = 0.8f + 0.00020000001f * (Rand_Next(1) & 0x3FF);
            break;
        case 10:
        case 18:
            if (o->type == 18) {
                p->time = i * -3;
                p->flag = 0;
            } else {
                p->time = 0;
                p->flag = i;
            }
            p->rot = Rand_Next(1);
            if (i == 0) {
                p->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            } else {
                p->rotv = 0.5f + (65536.0f * (0.03f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            }
            p->size = o->scale * (0.8f + 0.00040000002f * (Rand_Next(1) & 0x3FF));
            break;
        case 11:
            p->flag = i;
            p->rot = Rand_Next(1);
            p->rotv = 0.5f + (65536.0f * (0.15f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            break;
        case 12:
            p->rot = 0;
            p->rotv = 0;
            break;
        case 13:
            p->rot = Rand_Next(1);
            p->rotv = 0;
            break;
        case 19:
            p->flag = i;
            p->rot = Rand_Next(1);
            p->rotv = 0.5f + (65536.0f * (0.1f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            break;
        case 14:
        case 15:
            p->flag = i;
            p->rot = Rand_Next(1);
            break;
        case 16:
            p->time = -i * 2;
            p->flag = Rand_Next(1) & 1;
            p->rot = 0;
            p->size = 0.8f + 0.0003f * (Rand_Next(1) & 0x3FF);
            break;
        case 20:
            p->flag = i;
            p->rot = Rand_Next(1);
            if (i < 2) {
                p->rotv = (Rand_Next(1) & 0x1FF) - 0x100;
            } else {
                p->rotv = (Rand_Next(1) & 0xFF) - 0x80;
            }
            p->size = o->scale * (0.8f * StageObj_K14_Sizes[i]);
            break;
        case 21:
            p->time = -i;
            p->rot = Rand_Next(1);
            p->rotv = (Rand_Next(1) & 0xFF) - 0x80;
            p->flag = i;
            p->unk01 = 1;
            p->size = o->scale * (1.0f + 0.0005f * (Rand_Next(1) & 0x3FF));
            break;
        case 25:
            p->flag = i;
            p->time = -i;
            p->unk01 = 1;
            p->alpha = 1.0f;
            p->rot = Rand_Next(1);
            p->rotv = 0.5f + (65536.0f * (0.2f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200)))) / 360.0f;
            p->size = o->scale * (0.8f + 0.00070000003f * (Rand_Next(1) & 0x3FF));
            Vec_Copy(p->pos, &((Em *) o->owner)->posX);
            break;
        case 26:
            p->flag = Rand_Next(1) & 1;
            p->unk01 = 1;
            p->alpha = 1.0f;
            p->rot = Rand_Next(1);
            p->rotv = (Rand_Next(1) & 0x3FF) - 0x200;
            p->size = o->scale * (1.0f + 0.00080000004f * (Rand_Next(1) & 0x3FF));
            break;
        case 28:
            p->rot = Rand_Next(1);
            p->size = 1.2f;
            p->rotv = 0.5f + (65536.0f * (0.001f * ((Rand_Next(1) & 0x3FF) - 0x200))) / 360.0f;
            break;
        case 29:
            p->time = -i * 4;
            p->rot = Rand_Next(1);
            p->rotv = (Rand_Next(1) & 0x3FF) - 0x200;
            p->flag = i;
            p->unk01 = 1;
            p->size = o->scale * (1.5f + 0.0005f * (Rand_Next(1) & 0x3FF));
            break;
        }
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = (void *) o;
            p->node->part = i;
            p->node->draw = StageObj_K14_Draw;
        } else {
            p->node = NULL;
        }
    }
    if (o->type == 0x16 || o->type == 0x19) {
        StageObj_K14_Run(o);
    }
}

void StageObj_K14_Run(StageObj *o)
{
    Obj14Part *p;
    s16 n;
    u16 dur;
    u16 total;
    u16 *durs;
    s16 step;
    s16 ci;
    s16 c;
    s16 i;
    void *curve;
    f32 v[3];
    f32 w[3];
    f32 m[16];

    p = (Obj14Part *) o->buf;
    n = StageObj_K14_PartCounts[o->type];
    total = StageObj_K14_Totals[o->type];
    durs = StageObj_K14_DurLists[o->type];
    ci = StageObj_K14_CurveIdx[o->type];
    step = StageObj_K14_Steps[o->type];
    switch (o->type) {
    case 7:
    case 12:
    case 13:
    case 26:
    case 28:
        dur = total;
        break;
    case 0:
        dur = 0x41;
        break;
    case 1:
        dur = 0x24;
        break;
    case 11:
        dur = 0x42;
        break;
    case 16:
        dur = 0x18;
        break;
    case 18:
        dur = 0x26;
        break;
    case 29:
        dur = 0x13;
        break;
    case 21:
        dur = 0x2D;
        break;
    case 25:
        c = (u32) o->var2 >> 1;
        ci += c * 4;
        break;
    }
    if (++o->timer > total) {
        o->state++;
        o->visible = 0;
        return;
    }
    if (o->type == 0x10) {
        if (((Em *) o->owner)->active == 0 || ((Em *) o->owner)->taskPhase == 3) {
            for (i = 0; i < n; i++) {
                p[i].flag = -1;
            }
        }
    } else if (o->type == 0x19) {
        if (((Em *) o->owner)->active == 0 || ((Em *) o->owner)->taskPhase == 3) {
            o->state++;
            o->visible = 0;
            return;
        }
        if (c != 3) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(o->owner, 0x24)));
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 80.0f;
            Vec_RotateSelf(v, m);
            o->pos[0] = m[12] + v[0];
            o->pos[2] = m[14] + v[2];
            o->angle = Mtx_Heading(m) + 0x4000;
        }
    }
    for (i = 0; i < n; i++, p++) {
        switch (o->type) {
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
        case 8:
        case 9:
        case 10:
        case 14:
        case 15:
        case 19:
        case 20:
        case 25:
            dur = durs[i];
            break;
        case 0:
        case 1:
        case 7:
        case 11:
        case 13:
        case 16:
        case 18:
        case 21:
        case 28:
        case 29:
            ci = StageObj_K14_CurveIdx[o->type];
            break;
        }
        if (++p->time <= 0) {
            ci += step;
        } else if (p->time > (u16) dur) {
            ci += step;
        } else {
            if (o->type == 0x10 && p->time == 1) {
                if (p->flag != 0xFF) {
                    Em_BonePos(o->owner, 0x24, p->pos);
                    p->pos[0] += 0.05f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    p->pos[1] += 0.05f * ((Rand_Next(1) & 0x3FF) - 0x200);
                    p->pos[2] += 0.05f * ((Rand_Next(1) & 0x3FF) - 0x200);
                } else {
                    ci += step;
                    continue;
                }
            }
        curve = StageObj_K14_Curves[ci++];
        Key_LerpVec3(curve, p->sc, p->time);
        switch (o->type) {
        case 0:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            v[0] *= o->scale;
            v[1] *= o->scale;
            v[2] *= o->scale;
            Vec_RotateY(v, (6.2831855f * (o->angle + i * 0x3333)) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 1:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            v[0] *= o->scale;
            v[1] *= o->scale;
            v[2] *= o->scale;
            Vec_RotateY(v, (6.2831855f * (o->angle + i * 0x3333)) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 4:
        case 5:
        case 9:
        case 10:
        case 12:
        case 18:
        case 19:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            if (o->type == 1 || o->type == 19) {
                v[0] *= o->scale;
                v[1] *= o->scale;
                v[2] *= o->scale;
            } else if (o->type == 18) {
                v[0] += StageObj_K14_Drift[i];
                if (o->var2 == 1) {
                    v[0] = -v[0];
                }
                v[2] += 100.0f;
            }
            Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 2:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            v[0] += 50.0f;
            v[0] *= o->scale;
            v[1] *= o->scale;
            v[2] *= o->scale;
            Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 3:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            v[0] += 50.0f;
            v[0] = -v[0];
            v[0] *= o->scale;
            v[1] *= o->scale;
            v[2] *= o->scale;
            Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 6:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            if (o->var2 != 0) {
                v[0] = -v[0];
            }
            Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
            break;
        case 7:
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = -1000.0f;
            Vec_RotateSelf(v, gCamMatrix);
            break;
        case 8:
        case 13:
        case 14:
        case 15:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
            break;
        case 11:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            v[0] *= o->scale;
            v[1] *= o->scale;
            v[2] *= o->scale;
            Vec_RotateY(v, (6.2831855f * (o->angle + i * 0x3333)) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 16:
            p->pos[1] -= 10.25f;
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 0.0f;
            break;
        case 20:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            w[0] = 0.0f;
            w[1] = 0.0f;
            w[2] = i * 10;
            Vec_RotateSelf(w, gCamMatrix);
            v[0] += w[0];
            v[1] += w[1];
            v[2] += w[2];
            p->rot += (u16) p->rotv;
            break;
        case 21:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            v[0] *= o->scale;
            v[2] *= o->scale;
            Vec_RotateY(v, (6.2831855f * (o->angle + i * 0x2AAA)) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 25:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            if ((o->var2 & 1) == 1) {
                v[0] = -v[0];
            }
            v[1] += ((Em *) o->owner)->posY - p->pos[1];
            Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 26:
            v[0] = p->time * (50.0f / dur);
            if (o->var2 == 0) {
                v[0] = -v[0];
            }
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, (6.2831855f * o->angle) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 28:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            if (o->var2 == 1) {
                v[0] = -v[0];
            }
            Vec_RotateY(v, (6.2831855f * (o->angle + (i - 1) * 0x2AAB)) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        case 29:
            curve = StageObj_K14_Curves[ci++];
            Key_LerpVec3(curve, v, p->time);
            v[0] *= o->scale;
            v[1] *= o->scale;
            v[2] *= o->scale;
            Vec_RotateY(v, (6.2831855f * (o->angle + StageObj_K14_Sectors[i] * 0x2AAA)) / 65536.0f);
            p->rot += (u16) p->rotv;
            break;
        }
        if (p->unk01 == 0) {
            curve = StageObj_K14_Curves[ci++];
            Key_LerpF32(curve, &p->alpha, p->time);
        }
        if (p->node != NULL) {
            if (o->type == 0x10) {
                p->node->pos[0] = p->pos[0];
                p->node->pos[1] = p->pos[1];
                p->node->pos[2] = p->pos[2];
            } else {
                p->node->pos[0] = o->pos[0] + v[0];
                p->node->pos[1] = o->pos[1] + v[1];
                p->node->pos[2] = o->pos[2] + v[2];
            }
            OT_AddByDepth(gDrawOtSorted, p->node, 0x40, 0);
        }
        }
    }
}

void StageObj_K14_End(StageObj *o)
{
    u8 *p = o->buf;
    s16 n;
    s16 i;

    o->state++;
    n = StageObj_K14_PartCounts[o->type];
    for (i = 0; i < n; i++) {
        if (*(s32 *) (p + 0x2C) != 0) {
            DrawSlot_Free(*(s16 *) (p + 2));
        }
        p += 0x30;
    }
}

void StageObj_K14_Free(StageObj *o)
{
    StageObj_Free(o);
}

s16 Effect_MapTexId(s16 id);
void Effect_GetColor(s32 area, u8 *r, u8 *g, u8 *b, u8 *a);
s32 Model_DrawFade(void *mdl, f32 *m, u16 flags, void *a3, f32 alpha);
s16 EmSlot_FindKind(s16 k);
extern u8 StageObj_K14_ColorKeys[];
extern u8 StageObj_K14_CurveDef[];
extern u8 StageObj_K14_Curve1[];
extern u8 StageObj_K14_CurveBA[];
extern u8 StageObj_K14_CurveBB[];
extern u8 StageObj_K14_Curve14A[];
extern u8 StageObj_K14_Curve14B[];
extern u8 StageObj_K14_Curve6A[];
extern u8 StageObj_K14_Curve6B[];
extern u8 StageObj_K14_CurveFA[];
extern u8 StageObj_K14_CurveFB[];
extern u8 StageObj_K14_Curve11A[];
extern u8 StageObj_K14_Curve11B[];
extern u8 StageObj_K14_Curve16A[];
extern u8 StageObj_K14_Curve16B[];
extern u8 StageObj_K14_CurveDefA[];
extern u8 StageObj_K14_CurveDefB[];
extern u8 StageObj_K14_Curve31A[];
extern u8 StageObj_K14_Curve31B[];
extern u8 StageObj_K14_Curve27[];
extern u8 StageObj_K14_Curve26A[];
extern u8 StageObj_K14_Curve26B[];
extern u8 StageObj_K14_Curve2D[];
extern u8 StageObj_K14_Curve2E[];
extern u8 StageObj_K14_Curve22[];
extern u8 StageObj_K14_Curve2F[];
extern u8 StageObj_K14_Curve16[];
extern u8 StageObj_K14_CurveDef2[];
extern u8 StageObj_K14_Curve24A[];
extern u8 StageObj_K14_Curve24B[];
extern u8 *StageObj_K14_ColorKeysByVar[];

void StageObj_K14_Draw(DrawSlot *node)
{
    StageObj *o = node->owner;
    Obj14Part *p = (Obj14Part *) (o->buf + node->part * sizeof(Obj14Part));
    ModelMat *mats;
    u16 mode = 0;
    Model *set = gStageModelSet;
    Model *es;
    void *mesh;
    u8 *curve;
    s32 id;
    s16 tex;
    f32 f;
    u32 color;
    u8 r, g, b, a;
    f32 m[16];
    f32 tm[16];
    f32 rot[3];
    f32 scale[3];

    if (set != NULL && set->active) {
        mats = set->mats;
        Effect_GetColor(gScene.area, &r, &g, &b, &a);
        f = a / 255.0f;
        switch (o->type) {
        case 0:
        case 1:
        case 9:
        case 11:
        case 12:
        case 14:
        case 19:
        case 20:
        case 28:
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            if (o->type == 0xE) {
                if ((id = Effect_MapTexId(0x23)) != -1) {
                    Effect_DrawModel(&set->matSets[id], m, ((u8) (255.0f * f * p->alpha) << 24) | (r << 16) | (g << 8) | b, 0, mats);
                }
            } else {
                if ((id = Effect_MapTexId(0x4A)) != -1) {
                    mesh = &set->matSets[id];
                    if (o->type == 0xC) {
                        r = 0xFF;
                        g = 0xFF;
                        b = 0xFF;
                        f = 1.0f;
                    } else if (o->type == 0x14) {
                        r = 0x7F;
                        g = 0x6D;
                        b = 0x54;
                        f = 1.0f;
                    }
                    Effect_DrawModel(mesh, m, ((u8) (255.0f * f * p->alpha) << 24) | (r << 16) | (g << 8) | b, 0, mats);
                }
            }
            break;
        case 2:
        case 3:
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            if (o->type == 3) {
                Mtx_RotateY(m, 3.1415927f);
            }
            Mtx_Concat3x3(m, gCamMatrix);
            if (p->flag == 0) {
                if ((id = Effect_MapTexId(0x4A)) != -1) {
                    Effect_DrawModel(&set->matSets[id], m, ((u8) (255.0f * f * p->alpha) << 24) | (r << 16) | (g << 8) | b, 0, mats);
                }
            } else {
                if ((id = Effect_MapTexId(0x23)) != -1) {
                    Effect_DrawModel(&set->matSets[id], m, ((u8) (255.0f * p->alpha) << 24) | (r << 16) | (g << 8) | b, 0, mats);
                }
            }
            break;
        case 4:
        case 5:
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(p->sc, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            if (p->flag == 0) {
                if ((id = Effect_MapTexId(0x4A)) != -1) {
                    Effect_DrawModel(&set->matSets[id], m, ((u8) (255.0f * f * p->alpha) << 24) | (r << 16) | (g << 8) | b, 0, mats);
                }
            } else {
                if ((id = Effect_MapTexId(0x23)) != -1) {
                    Effect_DrawModel(&set->matSets[id], m, ((u8) (255.0f * p->alpha) << 24) | 0x555555, 0, mats);
                }
            }
            break;
        case 6:
        case 8:
        case 15:
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            if ((id = Effect_MapTexId(0x55)) != -1) {
                Model_DrawFade(&set->matSets[id], m, 0, mats, p->alpha);
            }
            break;
        case 7:
            Gs_SetState(0x6C, 0);
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            rot[1] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, o->pos, 2, m);
            if ((id = Effect_MapTexId(0x5D)) != -1) {
                mesh = &set->matSets[id];
                Mtx_Translation(tm, 0.0f, 0.0f, 0.0f);
                Gs_SetState(0x19, (s32) tm);
                Model_DrawFade(mesh, m, 0, mats, p->alpha);
            }
            break;
        case 10:
        case 18:
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            if (p->flag == 0) {
                if ((id = Effect_MapTexId(0x4A)) != -1) {
                    Effect_DrawModel(&set->matSets[id], m, ((u8) (255.0f * f * p->alpha) << 24) | (r << 16) | (g << 8) | b, 0, mats);
                }
            } else {
                if ((id = Effect_MapTexId(0x23)) != -1) {
                    Model_DrawFade(&set->matSets[id], m, 0, mats, p->alpha);
                }
            }
            break;
        case 13:
            Gs_SetState(0x6C, 0);
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            rot[1] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 2, m);
            if ((id = Effect_MapTexId(0x2A)) != -1) {
                mesh = &set->matSets[id];
                Color_EvalKeys((s32 *) StageObj_K14_ColorKeys, p->time, &color);
                Effect_DrawModel(mesh, m, color, 2, mats);
            }
            break;
        case 16:
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            if (p->flag != 0) {
                rot[1] = 0.0f;
            } else {
                rot[1] = 3.1415927f;
            }
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 2, m);
            Mtx_Concat3x3(m, gCamMatrix);
            if ((id = Effect_MapTexId(0x24)) != -1) {
                Model_DrawFade(&set->matSets[id], m, 0, mats, p->alpha);
            }
            break;
        case 21:
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            if ((id = Effect_MapTexId(0x4A)) != -1) {
                mesh = &set->matSets[id];
                switch (o->var2) {
                default:
                    curve = StageObj_K14_CurveDef;
                    break;
                case 1:
                    curve = StageObj_K14_Curve1;
                    break;
                }
                Color_EvalKeys((s32 *) curve, p->time, &color);
                Effect_DrawModel(mesh, m, color, 0, mats);
            }
            break;
        case 25:
            if ((id = EmSlot_FindKind(o->var1)) == -1) {
                return;
            }
            es = gScene.partModels[id];
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            switch (o->var1) {
            case 1:
            case 11:
                if (es == NULL) {
                    return;
                }
                if (!es->active) {
                    return;
                }
                if (p->flag == 0) {
                    curve = StageObj_K14_CurveBA;
                    mesh = &es->matSets[2];
                    mats = es->mats;
                    Mtx_Translation(tm, 0.25f * o->var0, 0.0f, 0.0f);
                    Gs_SetState(0x19, (s32) tm);
                    mode |= 2;
                } else {
                    if ((id = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[id];
                    mats = set->mats;
                    curve = StageObj_K14_CurveBB;
                }
                break;
            case 20:
                if ((id = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                mats = set->mats;
                if (p->flag == 0) {
                    curve = StageObj_K14_Curve14A;
                } else {
                    curve = StageObj_K14_Curve14B;
                }
                break;
            case 6:
                if (es == NULL) {
                    return;
                }
                if (!es->active) {
                    return;
                }
                if (p->flag == 0) {
                    mats = es->mats;
                    curve = StageObj_K14_Curve6A;
                    mesh = &es->matSets[2];
                } else {
                    if ((id = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[id];
                    mats = set->mats;
                    curve = StageObj_K14_Curve6B;
                }
                break;
            case 15:
                if ((id = Effect_MapTexId(0x84)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                mats = set->mats;
                mode |= 2;
                if (p->flag == 0) {
                    curve = StageObj_K14_CurveFA;
                } else {
                    curve = StageObj_K14_CurveFB;
                }
                break;
            case 17:
                if ((id = Effect_MapTexId(0x84)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                mats = set->mats;
                mode |= 2;
                if (p->flag == 0) {
                    curve = StageObj_K14_Curve11A;
                } else {
                    curve = StageObj_K14_Curve11B;
                }
                break;
            case 22:
                if ((id = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                mats = set->mats;
                if (p->flag == 0) {
                    curve = StageObj_K14_Curve16A;
                } else {
                    curve = StageObj_K14_Curve16B;
                }
                break;
            case 14:
            case 26:
            default:
                if ((id = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                mats = set->mats;
                if (p->flag == 0) {
                    curve = StageObj_K14_CurveDefA;
                } else {
                    curve = StageObj_K14_CurveDefB;
                }
                break;
            }
            Color_EvalKeys((s32 *) curve, p->time, &color);
            Effect_DrawModel(mesh, m, color, mode, mats);
            break;
        case 26:
            if ((id = EmSlot_FindKind(o->var1)) == -1) {
                return;
            }
            es = gScene.partModels[id];
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            switch (o->var1) {
            case 1:
            case 0x2A:
            case 0x25:
            case 0xB:
            case 0x29:
            case 0x31:
                if (es == NULL) {
                    return;
                }
                if (!es->active) {
                    return;
                }
                if (p->flag == 0) {
                    mode |= 2;
                    curve = StageObj_K14_Curve31A;
                    mats = es->mats;
                    mesh = &es->matSets[2];
                    Mtx_Translation(tm, 0.25f, 0.0f, 0.0f);
                } else {
                    if ((id = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[id];
                    curve = StageObj_K14_Curve31B;
                    mats = set->mats;
                }
                break;
            case 0x14:
            case 0x27:
                if ((id = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                curve = StageObj_K14_Curve27;
                mats = set->mats;
                break;
            case 6:
            case 0x26:
                if (es == NULL) {
                    return;
                }
                if (!es->active) {
                    return;
                }
                if (p->flag == 0) {
                    curve = StageObj_K14_Curve26A;
                    mats = es->mats;
                    mesh = &es->matSets[2];
                } else {
                    mats = set->mats;
                    if ((id = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[id];
                    curve = StageObj_K14_Curve26B;
                }
                break;
            case 0xF:
            case 0x2D:
                mode |= 2;
                if ((id = Effect_MapTexId(0x84)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                curve = StageObj_K14_Curve2D;
                mats = set->mats;
                break;
            case 0x15:
            case 0x2E:
                if ((id = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                curve = StageObj_K14_Curve2E;
                mats = set->mats;
                break;
            case 8:
            case 0x22:
                if ((id = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                curve = StageObj_K14_Curve22;
                mats = set->mats;
                break;
            case 0x11:
            case 0x2F:
                mode |= 2;
                if ((id = Effect_MapTexId(0x84)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                curve = StageObj_K14_Curve2F;
                mats = set->mats;
                break;
            case 0x16:
                mats = set->mats;
                if (o->var0 != 0) {
                    tex = 0x84;
                    mode |= 2;
                    curve = StageObj_K14_Curve2F;
                } else {
                    tex = 0x4A;
                    curve = StageObj_K14_Curve16;
                }
                if ((id = Effect_MapTexId(tex)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                break;
            case 0xE:
            case 0x2B:
            case 0x1A:
            case 0x2C:
            case 0x47:
            default:
                if ((id = Effect_MapTexId(0x4A)) == -1) {
                    goto end;
                }
                mesh = &set->matSets[id];
                curve = StageObj_K14_CurveDef2;
                mats = set->mats;
                break;
            case 2:
            case 0x24:
                if (es == NULL) {
                    return;
                }
                if (!es->active) {
                    return;
                }
                if (p->flag == 0) {
                    mode |= 2;
                    curve = StageObj_K14_Curve24A;
                    mats = es->mats;
                    mesh = &es->matSets[2];
                    Mtx_Translation(tm, 0.25f, 0.0f, 0.0f);
                } else {
                    mats = set->mats;
                    if ((id = Effect_MapTexId(0x4A)) == -1) {
                        goto end;
                    }
                    mesh = &set->matSets[id];
                    curve = StageObj_K14_Curve24B;
                }
                break;
            }
            Gs_SetState(0x19, (s32) tm);
            Color_EvalKeys((s32 *) curve, p->time, &color);
            Effect_DrawModel(mesh, m, color, mode, mats);
            break;
        case 29:
            scale[0] = p->size * p->sc[0];
            scale[1] = p->size * p->sc[1];
            scale[2] = p->size * p->sc[2];
            rot[2] = (6.2831855f * p->rot) / 65536.0f;
            Mtx_FromScaleRotTrans(scale, rot, node->pos, 3, m);
            Mtx_Concat3x3(m, gCamMatrix);
            if ((id = Effect_MapTexId(0x4A)) != -1) {
                mesh = &set->matSets[id];
                Color_EvalKeys((s32 *) StageObj_K14_ColorKeysByVar[o->var2], p->time, &color);
                Effect_DrawModel(mesh, m, color, 0, mats);
            }
            break;
        default:
            break;
        }
    end:
        Gs_SetState(0x6C, 1);
    }
}

s16 StageObj_K14_FindGround(Em *em, f32 *pos, s16 i)
{
    s16 *p = &StageObj_K14_GroundTbl[i];
    s16 t;
    f32 y;

    if (*p == 0) {
        return 0;
    }
    if (AreaGrid2_GetFloorHeight(pos, &y)) {
        if (y < pos[1] - 18.0f) {
            pos[1] = em->floorY;
        } else {
            pos[1] = 5.0f + y;
            return *p;
        }
    } else {
        pos[1] = em->floorY;
    }
    t = gAreaKinds[gScene.area];
    if (t == 10 || t == 14) {
        pos[1] = 5.0f + em->floorY;
        pos[1] += Area_GroundOffset(gScene.area);
        return *p;
    }
    return 0;
}

s16 StageObj_K14_PlaceAtEm(f32 *pos, Em *em, s16 k, s16 c)
{
    s16 g;
    u8 yset;
    s16 fx;
    s16 kind;
    f32 v[3];
    f32 m[16];

    yset = 0;
    kind = 0;
    if (c & 0x80) {
        fx = 0;
        c ^= 0x80;
    } else {
        fx = 1;
    }
    switch (k) {
    case 0:
    case 1:
    case 9:
    case 11:
    case 14:
    case 18:
    case 19:
        switch (em->type) {
        case 2:
        case 0x24:
        case 0x47:
            switch (c) {
            case 0:
                Em_BonePos(em, 0x1F, pos);
                break;
            default:
                Em_BonePos(em, 0x13, pos);
                break;
            }
            pos[1] = em->floorY;
            break;
        case 0x33:
        case 0x34:
        case 0x35:
        case 0x3D:
        case 0x3E:
            switch (c) {
            case 19:
                Em_BonePos(em, 0x10, pos);
                break;
            case 17:
                Em_BonePos(em, 0xE, pos);
                break;
            case 20:
                Em_BonePos(em, 4, pos);
                break;
            default:
                pos[0] = em->posX;
                pos[2] = em->posZ;
                break;
            }
            pos[1] = em->floorY;
            break;
        case 0x30:
        case 0x42:
        case 0x49:
        case 0x43:
            switch (c) {
            case 21:
                Em_BonePos(em, 0xC, pos);
                break;
            case 24:
                Em_BonePos(em, 0xB, pos);
                break;
            case 37:
                Em_BonePos(em, 0xD, pos);
                break;
            default:
                pos[0] = em->posX;
                pos[2] = em->posZ;
                break;
            }
            pos[1] = em->floorY;
            break;
        case 0x36:
        case 0x3C:
        case 0x40:
        case 0x41:
            switch (c) {
            case 25:
                Em_BonePos(em, 0x1F, pos);
                break;
            case 29:
                Em_BonePos(em, 0x1F, pos);
                break;
            default:
                pos[0] = em->posX;
                pos[2] = em->posZ;
                break;
            }
            pos[1] = em->floorY;
            break;
        case 0x37:
            switch (c) {
            case 38:
                Em_BonePos(em, 0xA, pos);
                break;
            case 39:
                Em_BonePos(em, 6, pos);
                break;
            case 40:
                Em_BonePos(em, 2, pos);
                break;
            default:
                pos[0] = em->posX;
                pos[2] = em->posZ;
                break;
            }
            pos[1] = em->floorY;
            break;
        default:
            switch (c) {
            default:
                pos[0] = em->posX;
                pos[1] = em->floorY;
                pos[2] = em->posZ;
                break;
            case 9:
                Em_BonePos(em, 4, pos);
                pos[1] = em->floorY;
                break;
            }
            if (fx) {
                switch (em->type) {
                case 2:
                case 0x24:
                case 0x47:
                    if (fx) { break; }
                    break;
                case 0x1:
                case 0x2A:
                case 0x25:
                case 0x6:
                case 0x26:
                case 0x8:
                case 0xB:
                case 0x29:
                case 0x31:
                case 0xE:
                case 0x2B:
                case 0xF:
                case 0x2D:
                case 0x11:
                case 0x2F:
                case 0x14:
                case 0x27:
                case 0x15:
                case 0x2E:
                case 0x16:
                case 0x1A:
                case 0x2C:
                case 0x22:
                case 0x28:
                    switch (k) {
                    case 1:
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            kind = 0x21;
                        } else {
                            kind = 0x2C;
                        }
                        break;
                    case 9:
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            kind = 0x27;
                        } else {
                            kind = 0x2E;
                        }
                        break;
                    case 11:
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            kind = 0x26;
                        } else {
                            kind = 0x2D;
                        }
                        break;
                    }
                    break;
                }
            }
            break;
        }
        break;
    case 2:
    case 3:
        switch (c) {
        case 4:
            Em_BonePos(em, 0xA, pos);
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 20.0f;
            Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
            pos[0] += v[0];
            pos[2] += v[2];
            break;
        case 5:
            Vec_Copy(pos, &em->posX);
            if (k == 2) {
                v[0] = -85.0f;
            } else {
                v[0] = 85.0f;
            }
            v[1] = 0.0f;
            v[2] = 0.0f;
            Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
            yset = 1;
            pos[0] += v[0];
            pos[1] += v[1];
            pos[2] += v[2];
            break;
        case 6:
            Em_BonePos(em, 2, pos);
            break;
        case 8:
            Em_BonePos(em, 0x1C, pos);
            break;
        case 11:
            Em_BonePos(em, 0x1E, pos);
            break;
        case 48:
            Em_BonePos(em, 0x19, pos);
            break;
        case 12:
            Em_BonePos(em, 2, pos);
            break;
        case 13:
            Em_BonePos(em, 1, pos);
            break;
        case 16:
            Em_BonePos(em, 0x17, pos);
            break;
        case 18:
        case 34:
            Em_BonePos(em, 9, pos);
            break;
        case 15:
            Em_BonePos(em, 0x1B, pos);
            break;
        case 17:
        case 33:
            Em_BonePos(em, 0xE, pos);
            break;
        case 23:
        case 36:
            Em_BonePos(em, 6, pos);
            break;
        case 22:
        case 35:
            Em_BonePos(em, 0xA, pos);
            break;
        case 25:
            Em_BonePos(em, 8, pos);
            break;
        case 26:
            Em_BonePos(em, 0xD, pos);
            break;
        case 28:
            Em_BonePos(em, 0x1A, pos);
            break;
        case 29:
        case 30:
            Em_BonePos(em, 0x1F, pos);
            break;
        case 31:
            Em_BonePos(em, 0x27, pos);
            break;
        case 32:
            Em_BonePos(em, 0x12, pos);
            break;
        case 38:
            Em_BonePos(em, 0xA, pos);
            break;
        case 39:
            Em_BonePos(em, 6, pos);
            break;
        case 41:
            Em_BonePos(em, 0xD, pos);
            break;
        case 42:
        case 47:
            Em_BonePos(em, 0x1F, pos);
            break;
        case 43:
            Em_BonePos(em, 0x29, pos);
            break;
        case 44:
            Em_BonePos(em, 0x17, pos);
            break;
        case 45:
            Em_BonePos(em, 6, pos);
            break;
        case 46:
            Em_BonePos(em, 9, pos);
            break;
        default:
            if (k == 2) {
                Em_BonePos(em, 0x1A, pos);
                if (c == 2) {
                    v[0] = 0.0f;
                    v[1] = 0.0f;
                    v[2] = -50.0f;
                    Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
                    pos[0] += v[0];
                    pos[2] += v[2];
                }
            } else {
                Em_BonePos(em, 0x14, pos);
            }
            break;
        }
        if (yset == 0) {
            pos[1] = em->floorY;
        }
        break;
    case 4:
        if (c == 0) {
            Em_BonePos(em, 0x15, pos);
        } else {
            Em_BonePos(em, 0x1B, pos);
        }
        pos[1] = em->floorY;
        break;
    case 5:
        Em_BonePos(em, 0x1B, pos);
        pos[1] = em->floorY;
        break;
    case 6:
    case 15:
        Em_BonePos(em, 0x23, pos);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 60.0f;
        Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
        pos[0] += v[0];
        pos[1] += v[1];
        pos[2] += v[2];
        pos[1] = AreaGrid2_GetFloorY(pos);
        break;
    case 8:
        Em_BonePos(em, 0x23, pos);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 30.0f;
        Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
        pos[0] += v[0];
        pos[1] += v[1];
        pos[2] += v[2];
        pos[1] = AreaGrid2_GetFloorY(pos);
        break;
    case 7:
        Em_BonePos(em, 0x23, pos);
        v[0] = 0.1f * ((Rand_Next(1) & 0x3FF) - 0x200);
        v[1] = 0.0f;
        v[2] = 80.0f + 0.1f * ((Rand_Next(1) & 0x3FF) - 0x200);
        Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
        pos[0] += v[0];
        pos[2] += v[2];
        pos[1] = AreaGrid2_GetFloorY(pos);
        pos[1] += -5.0f;
        break;
    case 10:
        if (c != 0x30) {
            if (c != 0xB) {
                switch (em->type) {
                case 2:
                case 0x24:
                case 0x47:
                    Em_BonePos(em, 1, pos);
                    pos[1] = AreaGrid2_GetFloorY(pos);
                    break;
                case 0x1:
                case 0x2A:
                case 0x25:
                case 0x6:
                case 0x26:
                case 0x8:
                case 0xB:
                case 0x29:
                case 0x31:
                case 0xE:
                case 0x2B:
                case 0xF:
                case 0x2D:
                case 0x11:
                case 0x2F:
                case 0x14:
                case 0x27:
                case 0x15:
                case 0x2E:
                case 0x16:
                case 0x1A:
                case 0x2C:
                case 0x22:
                case 0x28:
                    Em_BonePos(em, 0x29, pos);
                    pos[1] = AreaGrid2_GetFloorY(pos);
                    if (fx) {
                        if (!(Em_TypeUsesK14(em->type) & 0xFF)) {
                            kind = 0x20;
                        } else {
                            kind = 0x30;
                        }
                    }
                    break;
                }
            } else {
                Em_BonePos(em, 0x1E, pos);
                pos[1] = AreaGrid2_GetFloorY(pos);
            }
        } else {
            Em_BonePos(em, 0x19, pos);
            pos[1] = AreaGrid2_GetFloorY(pos);
        }
        break;
    case 13:
    case 21:
    case 29:
        pos[0] = em->posX;
        pos[1] = em->floorY;
        pos[2] = em->posZ;
        break;
    case 25:
        if (em->type == 2 || em->type == 0x24 || em->type == 0x47) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, 0x33)));
        } else {
            Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, 0x24)));
        }
        Mtx_GetTrans(pos, m);
        v[0] = 0.0f;
        v[1] = 0.0f;
        v[2] = 80.0f * em->scaleZ;
        Vec_RotateSelf(v, m);
        pos[0] += v[0];
        pos[1] += v[1];
        pos[2] += v[2];
        break;
    case 26:
        if (em->type == 2 || em->type == 0x24 || em->type == 0x47) {
            Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, 0x33)));
            v[0] = 0.0f;
            v[1] = -23.7f;
            v[2] = 151.9f;
        } else {
            Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, 0x24)));
            v[0] = 0.0f;
            v[1] = 0.0f;
            v[2] = 50.0f * em->scaleZ;
        }
        Mtx_GetTrans(pos, m);
        Vec_RotateSelf(v, m);
        pos[0] += v[0];
        pos[1] += v[1];
        pos[2] += v[2];
        break;
    case 28:
        Mtx_Copy((f32 *) m, (f32 *) (Em_Bone(em, 2)));
        if (c == 0) {
            v[0] = -65.0f;
        } else {
            v[0] = 65.0f;
        }
        v[1] = 0.0f;
        v[2] = 0.0f;
        Vec_RotateY(v, (6.2831855f * em->rotY) / 65536.0f);
        pos[0] = m[12] + v[0];
        pos[1] = em->floorY;
        pos[2] = m[14] + v[2];
        break;
    }
    if ((g = StageObj_K14_FindGround(em, pos, k)) != 0 && kind != 0) {
        Eff01_CreateAt(em, pos, kind);
    }
    return g;
}

void StageObj_K14_Create(Em *em, s16 k, s16 c, f32 scale)
{
    StageObj *o;
    f32 pos[3];

    if ((Player_InCurrentArea(em) & 0xFF) && StageObj_K14_SpawnEffects(em, pos, StageObj_K14_PlaceAtEm(pos, em, k, c), scale) == 0) {
        if ((o = StageObj_Alloc(1)) != NULL) {
            o->kind = 0x14;
            o->update = StageObj_K14_Update;
            o->type = k;
            o->owner = em;
            o->scale = scale;
            Vec_Copy(o->pos, pos);
            o->var2 = c;
            o->var1 = 0;
        }
    }
}

void StageObj_K14_CreateAtYaw(f32 *pos, s32 k, u16 v, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0x14;
        o->update = StageObj_K14_Update;
        o->type = k;
        o->owner = NULL;
        o->scale = scale;
        o->var2 = 0;
        o->angle = v;
        Vec_Copy(o->pos, pos);
        o->var1 = 2;
    }
}

void StageObj_K14_CreateOnEm(Player *p, s16 k, s16 c, f32 scale)
{
    StageObj *o;
    f32 pos[3];
    f32 v[3];

    if (Obj_InCurrentArea(p) & 0xFF) {
        switch (k) {
        case 2:
            Em_GetBonePos(p, 8, pos);
            pos[1] = p->floorY;
            if (c == 2) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 50.0f;
                Vec_RotateY(v, (6.2831855f * p->rot[1]) / 65536.0f);
                pos[0] += v[0];
                pos[1] += v[1];
                pos[2] += v[2];
            } else if (c == 3) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = -50.0f;
                Vec_RotateY(v, (6.2831855f * p->rot[1]) / 65536.0f);
                pos[0] += v[0];
                pos[1] += v[1];
                pos[2] += v[2];
            }
            break;
        case 3:
            Em_GetBonePos(p, 5, pos);
            pos[1] = p->floorY;
            if (c == 2) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 50.0f;
                Vec_RotateY(v, (6.2831855f * p->rot[1]) / 65536.0f);
                pos[0] += v[0];
                pos[1] += v[1];
                pos[2] += v[2];
            } else if (c == 3) {
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = -50.0f;
                Vec_RotateY(v, (6.2831855f * p->rot[1]) / 65536.0f);
                pos[0] += v[0];
                pos[1] += v[1];
                pos[2] += v[2];
            }
            break;
        }
        if (StageObj_K14_SpawnEffects((Em *) p, pos, StageObj_K14_FindGround((Em *) p, pos, k), scale) == 0) {
            if ((o = StageObj_Alloc(1)) != NULL) {
                o->kind = 0x14;
                o->update = StageObj_K14_Update;
                o->type = k;
                o->owner = p;
                Vec_Copy(o->pos, pos);
                o->scale = scale;
                o->var2 = c;
                o->var1 = 1;
            }
        }
    }
}

void StageObj_K14_CreateAt(f32 *pos, u8 k, u8 c, f32 scale)
{
    StageObj *o;

    if ((o = StageObj_Alloc(1)) != NULL) {
        o->kind = 0x14;
        o->update = StageObj_K14_Update;
        o->type = k;
        o->owner = NULL;
        o->scale = scale;
        Vec_Copy(o->pos, pos);
        o->var2 = c;
        o->var1 = 2;
    }
}

s8 StageObj_K14_SpawnEffects(Em *em, f32 *pos, s32 mode, f32 scale)
{
    f32 f;

    switch ((s16) mode) {
    case 0:
        return 0;
    default:
    case 1:
        f = 5.0f * scale;
        Effect08_Spawn(pos, 4, 0, f);
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 5, 0, f);
        break;
    case 2:
        Effect08_Spawn(pos, 4, 0, 5.0f * scale);
        pos[1] += 5.0f;
        f = 4.5f * scale;
        Effect08_Spawn(pos, 1, 0, f);
        break;
    case 3:
        f = 10.0f * scale;
        pos[1] += 5.0f;
        Effect08_Spawn(pos, 2, 0, f);
        break;
    }
    if (!(f <= 4.0f)) {
        Sound_PlaySeAtPosChecked(1, 0x76, 0, pos, 1, 0);
    } else {
        Sound_PlaySeAtPosChecked(1, 0x70, 0, pos, 1, 0);
    }
    return 1;
}

void StageObj_K15_Create(Em *em, u8 k)
{
    StageObj *o;

    if (Obj_InCurrentArea(em) & 0xFF) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x15;
            o->update = StageObj_K15_Update;
            o->draw = NULL;
            o->owner = em;
            o->ownerIndex = em->index;
            o->type = k;
        }
    }
}

void StageObj_K15_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K15_Setup(o);
        break;
    case 1:
        StageObj_K15_Run(o);
        break;
    case 2:
        StageObj_K15_End(o);
        break;
    case 3:
        StageObj_K15_Free(o);
        break;
    }
}

void StageObj_K15_Setup(StageObj *o)
{
    Em *em = o->owner;
    Obj15Drop *d = (Obj15Drop *) o->buf;
    s16 failed;
    s16 i;
    f32 f;
    f32 m[16];
    f32 v[3];

    o->state++;
    o->var0 = 0;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    default:
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, 0xA)));
        f = 1.0f;
        Mtx_GetTrans(o->pos, m);
        break;
    case 1:
        switch (em->type) {
        case 0x11:
        case 0x2F:
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, 0x24)));
            f = 1.5f;
            Mtx_GetTrans(o->pos, m);
            break;
        case 0x36:
        case 0x3C:
            Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, 0x25)));
            v[0] = 0.0f;
            f = 1.5f;
            v[1] = -45.0f;
            v[2] = 165.0f;
            Vec_RotateSelf(v, m);
            Mtx_GetTrans(o->pos, m);
            o->pos[0] += v[0];
            o->pos[1] += v[1];
            o->pos[2] += v[2];
            break;
        default:
            StageObj_Free(o);
            return;
        }
        break;
    case 2:
        Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, 0x27)));
        f = 1.5f;
        v[0] = 0.0f;
        v[1] = 30.0f;
        v[2] = 100.0f;
        Mtx_GetTrans(o->pos, m);
        Vec_RotateSelf(v, m);
        Vec_Add(o->pos, o->pos, v);
        Mtx_TurnY(m, 3.1415927f);
        break;
    }
    failed = 1;
    for (i = 0; i < 3; i++, d++) {
        d->phase = 0;
        d->visible = 0;
        d->scale = f * (0.3f + 0.01f * (s32) (Rand_Next(1) & 0x3F));
        d->timer = Rand_Next(1) & 0xF;
        v[0] = (s32) (Rand_Next(1) & 0x7F) - 64.0f;
        v[1] = (s32) (Rand_Next(1) & 0x3F) - 32.0f;
        v[2] = -1.0f * (s32) (Rand_Next(1) & 0x3F) - 32.0f;
        d->pos[0] = o->pos[0] + v[0];
        d->pos[1] = o->pos[1] + v[1];
        d->pos[2] = o->pos[2] + v[2];
        d->rot[0] = Rand_Next(1);
        d->rot[1] = Rand_Next(1);
        d->rot[2] = Rand_Next(1);
        v[0] = 0.1f * (s32) (Rand_Next(1) & 0x3F) - 3.2f;
        v[1] = 0.0f;
        v[2] = 2.0f + 0.1f * (s32) (Rand_Next(1) & 0xF);
        Vec_RotateSelf(v, m);
        d->vel[0] = v[0];
        d->vel[1] = 0.0f;
        d->vel[2] = v[2];
        d->slot = DrawSlot_Alloc();
        if (d->slot != -1) {
            d->node = DrawSlot_Get(d->slot);
            failed = 0;
            d->node->owner = o;
            d->node->part = i;
            d->node->draw = StageObj_K15_Draw;
            d->alive = 1;
        } else {
            d->node = NULL;
            d->alive = 0;
        }
    }
    o->pos[1] = em->floorY;
    if (failed != 0) {
        StageObj_Free(o);
    }
}

void StageObj_K15_MoveDrop(StageObj *o, Obj15Drop *d)
{
    d->pos[0] += d->vel[0];
    d->pos[1] += d->vel[1];
    d->pos[2] += d->vel[2];
    d->vel[1] += -1.4545455f;
}

void StageObj_K15_Run(StageObj *o)
{
    Obj15Drop *d;
    s16 i;
    s16 done;
    void *owner;

    i = 0;
    done = 1;
    d = (Obj15Drop *) o->buf;
    owner = o->owner;
    for (; i < 3; i++) {
        if (d->alive) {
            done = 0;
            switch (d->phase) {
            case 0:
                if (--d->timer <= 0) {
                    d->phase++;
                    d->visible = 1;
                }
                break;
            case 1:
                StageObj_K15_MoveDrop(o, d);
                if (d->pos[1] <= o->pos[1]) {
                    d->phase++;
                    d->timer = 100;
                    d->pos[1] = o->pos[1];
                    d->vel[0] *= 1.8f;
                    d->vel[2] *= 1.8f;
                    d->vel[1] = 3.0f + 0.15f * (s32) (Rand_Next(1) & 0x1F);
                }
                break;
            case 2:
                StageObj_K15_MoveDrop(o, d);
                if (--d->timer <= 0) {
                    d->alive = 0;
                }
                break;
            }
            if (d->visible) {
                d->node->pos[0] = d->pos[0];
                d->node->pos[1] = d->pos[1];
                d->node->pos[2] = d->pos[2];
                OT_AddByDepth(gDrawOtSorted, d->node, 0x40, 0);
            }
            d++;
        }
    }
    if (done != 0 || !(Obj_InCurrentArea(owner) & 0xFF)) {
        o->state++;
    }
}

void StageObj_K15_End(StageObj *o)
{
    u8 *p = o->buf;
    s16 i;

    o->state++;
    for (i = 0; i < 3; i++) {
        if (*(s16 *) (p + 6) != -1) {
            DrawSlot_Free(*(s16 *) (p + 6));
        }
        p += 0x34;
    }
}

void StageObj_K15_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_K15_Draw(DrawSlot *node)
{
    f32 m[16];
    f32 s[16];
    StageObj *o = node->owner;
    ModelMat *mats;
    ModelMatSet *model;
    ModelMat *mat;
    s32 i;
    Obj15Drop *d = (Obj15Drop *) o->buf + node->part;
    Model *set = gStageModelSet;

    if (set != NULL && set->active != 0) {
        switch (o->type) {
        case 0:
            model = &set->matSets[43];
            Gs_SetState(0x67, -1);
            break;
        case 1:
        default:
            model = &set->matSets[41];
            Gs_SetState(0x67, -1);
            break;
        case 2:
            model = &set->matSets[41];
            Gs_SetState(0x67, 0xFFD08E65);
            break;
        }
        mats = set->mats;
        Mtx_Scale(s, d->scale, d->scale, d->scale);
        Mtx_Translation(m, d->pos[0], d->pos[1], d->pos[2]);
        Mtx_SetRotXYZ(m, (6.2831855f * d->rot[0]) / 65536.0f, (6.2831855f * d->rot[1]) / 65536.0f,
                      (6.2831855f * d->rot[2]) / 65536.0f);
        Mtx_Concat3x3(m, s);
        Gs_SetState(0x1A, (s32) m);
        if (model != NULL && model->id != -1) {
            for (i = 0; i < model->count; i++) {
                mat = &mats[model->mat[i]];
                mat->rgb[0] = 1.0f;
                mat->rgb[1] = 1.0f;
                mat->rgb[2] = 1.0f;
                Gs_SetState((u8) (i + 0x3A), (s32) mat);
            }
            Draw_ApplyStates(model->flags);
            Model_Draw(model->id, 0);
        }
        Draw_SetDefaultState();
    }
}

void StageObj_Stun_CreatePlayer(Player *p, u8 k)
{
    StageObj *o;

    if (Obj_InCurrentArea(p) & 0xFF) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x18;
            o->update = StageObj_Stun_Update;
            o->draw = NULL;
            o->owner = p;
            o->ownerIndex = p->index;
            o->type = k;
            o->pos[0] = 0.0f;
            o->pos[1] = 30.0f;
            o->pos[2] = 25.0f;
            o->scale = 1.0f;
            o->angle = 0x14;
        }
    }
}

void StageObj_Stun_CreateEm(Em *em, u8 k, s16 v, s16 t, f32 y, f32 scale)
{
    StageObj *o;

    if (Player_InCurrentArea(em) & 0xFF) {
        o = StageObj_Alloc(1);
        if (o != NULL) {
            o->kind = 0x18;
            o->update = StageObj_Stun_Update;
            o->draw = NULL;
            o->owner = em;
            o->ownerIndex = em->index;
            o->type = k;
            o->timer = t;
            o->pos[1] = y;
            o->pos[0] = 0.0f;
            o->pos[2] = 25.0f;
            o->scale = scale;
            o->angle = v;
        }
    }
}

void StageObj_Stun_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_Stun_Setup(o);
        break;
    case 1:
        StageObj_Stun_Run(o);
        break;
    case 2:
        StageObj_Stun_End(o);
        break;
    case 3:
        StageObj_Stun_Free(o);
        break;
    }
}

void StageObj_Stun_Setup(StageObj *o)
{
    StunWork *w = (StunWork *) o->buf;
    s16 i;

    o->state++;
    o->var0 = 0;
    o->var1 = 0;
    o->visible = 1;
    o->draw = NULL;
    switch (o->type) {
    case 0:
        w->group = 2;
        w->state = 0x13;
        break;
    case 1:
        w->limit = o->timer;
        w->group = ((Em *) o->owner)->stateGroup;
        w->state = ((Em *) o->owner)->state;
        break;
    case 2:
        w->limit = o->timer;
        w->group = ((Em *) o->owner)->stateGroup;
        w->state = ((Em *) o->owner)->state;
        break;
    }
    w->bone = o->angle;
    Vec_Copy(w->ofs, o->pos);
    w->ofs[2] *= o->scale;
    o->timer = 0;
    o->angle = Rand_Next(1);
    for (i = 0; i < 5; i++) {
        w->slot[i] = DrawSlot_Alloc();
        if (w->slot[i] != -1) {
            w->node[i] = DrawSlot_Get(w->slot[i]);
            w->node[i]->owner = o;
            w->node[i]->part = i;
            w->node[i]->draw = StageObj_Stun_Draw;
        } else {
            w->node[i] = NULL;
        }
    }
}

void StageObj_Stun_Run(StageObj *o)
{
    StunWork *w = (StunWork *) o->buf;
    Em *em = o->owner;
    s16 n;
    s16 i;
    f32 m[16];

    if (o->type == 2) {
        if (em->active == 0) {
            o->state++;
            o->visible = 0;
            return;
        }
        if (em->stateGroup == 5 || em->blindTimer <= 0) {
            o->state++;
            o->visible = 0;
            return;
        }
    } else if (em->active == 0 || Player_IsState(em, w->group, w->state) == 0) {
        o->state++;
        o->visible = 0;
        return;
    }
    o->timer++;
    switch (o->type) {
    case 0:
        n = em->stateVar1;
        if (o->timer % 23 == 1) {
            Sound_PlaySeAtPosChecked(1, 0x38, 0, &em->posX, 1, 0);
        }
        break;
    case 1:
    case 2:
        if (o->timer > w->limit) {
            o->state++;
            o->visible = 0;
            return;
        }
        n = 5;
        break;
    }
    if (n == 0) {
        o->state++;
        o->visible = 0;
        return;
    }
    if (n > 5) {
        n = 5;
    }
    Mtx_Copy((f32 *) m, (f32 *) (Em_GetBone(em, w->bone)));
    Mtx_GetTrans(o->pos, m);
    o->angle += 0x800;
    for (i = 0; i < n; i++) {
        if (w->node[i] != NULL) {
            Vec_Copy(w->node[i]->pos, w->ofs);
            Vec_RotateY(w->node[i]->pos, (6.2831855f * (o->angle + i * (0x10000 / n))) / 65536.0f);
            Vec_RotateSelf(w->node[i]->pos, m);
            w->node[i]->pos[0] += o->pos[0];
            w->node[i]->pos[1] += o->pos[1];
            w->node[i]->pos[2] += o->pos[2];
            OT_AddByDepth(gDrawOt32, w->node[i], 0x20, 0);
        }
    }
}

typedef struct Handles5 { s16 id[5]; u8 unkA[0xC - 0xA]; s32 h[5]; } Handles5;

void StageObj_Stun_End(StageObj *o)
{
    Handles5 *x = (Handles5 *) o->buf;
    s16 i;

    o->state++;
    for (i = 0; i < 5; i++) {
        if (x->h[i] != 0) {
            DrawSlot_Free(x->id[i]);
        }
    }
}

void StageObj_Stun_Free(StageObj *o)
{
    StageObj_Free(o);
}

void StageObj_Stun_Draw(DrawSlot *node)
{
    Model *mdl = gCommonModels[0];
    StageObj *o = node->owner;
    ModelMat *tex;
    ModelMatSet *mesh;
    f32 m[16];

    if (mdl == NULL || mdl->active == 0) {
        return;
    }
    tex = mdl->mats;
    mesh = &mdl->matSets[118];
    Mtx_Scale(m, o->scale, o->scale, o->scale);
    Mtx_SetTrans(m, node->pos[0], node->pos[1], node->pos[2]);
    Mtx_Concat3x3(m, gCamMatrix);
    Gs_SetState(0x67, -1);
    Effect_DrawModelAdd(mesh, m, tex);
}

void StageObj_K1B_Update(StageObj *o)
{
    switch (o->state) {
    case 0:
        StageObj_K1B_Setup(o);
        break;
    case 1:
        StageObj_K1B_Run(o);
        break;
    case 2:
        StageObj_K1B_End(o);
        break;
    case 3:
        StageObj_K1B_Free(o);
        break;
    }
}

void StageObj_K1B_Setup(StageObj *o)
{
    Obj1BPart *p = (Obj1BPart *) o->buf;
    f32 bp[3];
    f32 v[3];
    u8 i;
    u16 yaw;
    u16 r;
    if (o->owner == NULL) {
        o->state = 2;
        o->visible = 0;
        return;
    }
    o->var0 = 0;
    o->var1 = 0;
    o->var2 = 0;
    Em_GetBonePos(o->owner, 2, bp);
    Rand_Next(1);
    switch (o->type) {
    case 0:
        Sound_PlaySeAtPosChecked(1, 0x8D, 0, bp, 1, 0);
        break;
    }
    for (i = 0; i < StageObj_K1B_PartCounts[o->type]; i++) {
        switch (o->type) {
        case 0:
            memset(p, 0, 0x40);
            p->alpha = 0xFF;
            p->model = StageObj_K1B_Models[i];
            p->life = StageObj_K1B_Lives[i];
            switch (i) {
            case 0:
            case 4:
                p->ground = ((Em *) o->owner)->floorY;
                p->pos[0] = -20.0f + Rand_Next(1) % 40;
                p->pos[1] = -40.0f + Rand_Next(1) % 80;
                p->pos[2] = -20.0f + Rand_Next(1) % 40;
                p->rot[0] = Rand_Next(1) % 0xE39;
                p->rot[1] = Rand_Next(1);
                if (i == 0) {
                    yaw = p->rot[1];
                }
                if (i == 4) {
                    p->rot[1] = yaw + 0x6000 + (Rand_Next(1) & 0x1FFF);
                }
                p->rot[2] = Rand_Next(1) % 0x1C72;
                p->scale[0] = p->scale[1] = p->scale[2] = 1.0f + 0.01f * (Rand_Next(1) % 20);
                p->fall = -7.0f;
                r = Rand_Next(1);
                o->var0 = r & 1;
                o->var1 = (r >> 1) & 1;
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 10.0f + (Rand_Next(1) & 3);
                Vec_RotateY(v, (6.2831855f * p->rot[1]) / 65536.0f);
                p->vel[0] = v[0];
                p->vel[1] = v[1];
                p->vel[2] = v[2];
                p->rot[1] -= 0x4000;
                break;
            case 1:
                p->scale[2] = 1.0f;
                break;
            case 2:
                p->rot[2] = Rand_Next(1);
                p->scale[0] = p->scale[1] = p->scale[2] = 1.0f;
                break;
            case 3:
                p->rot[2] = Rand_Next(1);
                p->scale[0] = 0.5f;
                p->scale[1] = 1.0f;
                o->var0 = Rand_Next(1) & 1;
                p->rot[1] = Rand_Next(1);
                v[0] = 0.0f;
                v[1] = 0.0f;
                v[2] = 10.0f + (Rand_Next(1) & 3);
                Vec_RotateY(v, (6.2831855f * p->rot[1]) / 65536.0f);
                p->vel[0] = v[0];
                p->vel[1] = v[1];
                p->vel[2] = v[2];
                p->fall = -7.0f - (Rand_Next(1) & 3);
                p->pos[0] = -20.0f + Rand_Next(1) % 40;
                p->pos[1] = -80.0f + Rand_Next(1) % 160;
                p->pos[2] = -20.0f + Rand_Next(1) % 40;
                break;
            }
            p->pos[0] += bp[0];
            p->pos[1] += bp[1];
            p->pos[2] += bp[2];
            break;
        case 1:
            memset(p, 0, 0x40);
            p->alpha = 0xFF;
            p->model = (Rand_Next(1) & 1) + 0x92;
            p->t = -(Rand_Next(1) % 5);
            p->pos[2] = 20.0f + (u32) (Rand_Next(1) % 10U);
            Vec_RotateY(p->pos, (6.2831855f * (s32) Rand_Next(1)) / 65536.0f);
            p->rot[2] = Rand_Next(1);
            p->fall = 1.0f;
            o->var0 = Rand_Next(1) & 1;
            p->life = 10;
            p->scale[0] = p->scale[1] = p->scale[2] = 1.0f;
            break;
        }
        p->slot = DrawSlot_Alloc();
        if (p->slot != -1) {
            p->node = DrawSlot_Get(p->slot);
            p->node->owner = o;
            p->node->part = i;
            p->node->draw = (DrawSlotFn)StageObj_K1B_Draw; /* defined with void *arg: a DrawSlot * parameter changes its registers */
        } else {
            p->node = NULL;
        }
        p++;
    }
    o->state++;
    o->visible = 1;
    o->draw = NULL;
    StageObj_K1B_Run(o);
}

/* unk_874B90's .data (0x8D0820-0x8D53B0) */
extern u32 D_008F5CD8[];
u32 StageObj_K12_Curves_0[36] = {
#include "game/008D0820.inc"
};
u32 StageObj_K12_Curves_1[36] = {
#include "game/008D08B0.inc"
};
u32 StageObj_K12_Curves_4[16] = {
#include "game/008D0940.inc"
};
u32 StageObj_K12_Curves_8[16] = {
#include "game/008D0980.inc"
};
u32 StageObj_K12_Curves_12[16] = {
#include "game/008D09C0.inc"
};
u32 StageObj_K12_Curves_2[12] = {
#include "game/008D0A00.inc"
};
u32 StageObj_K12_Curves_6[12] = {
#include "game/008D0A30.inc"
};
u32 StageObj_K12_Curves_10[12] = {
#include "game/008D0A60.inc"
};
u32 StageObj_K12_Curves_5[8] = {
#include "game/008D0A90.inc"
};
u32 StageObj_K12_Curves_9[8] = {
#include "game/008D0AB0.inc"
};
u32 StageObj_K12_Curves_13[8] = {
#include "game/008D0AD0.inc"
};
u32 StageObj_K12_Curves_15[12] = {
#include "game/008D0AF0.inc"
};
u32 StageObj_K12_Curves_14[12] = {
#include "game/008D0B20.inc"
};
u32 StageObj_K12_Curves_16[8] = {
#include "game/008D0B50.inc"
};
u32 StageObj_K12_Curves_17[20] = {
#include "game/008D0B70.inc"
};
u32 StageObj_K12_Curves_18[8] = {
#include "game/008D0BC0.inc"
};
u32 StageObj_K12_Curves_19[24] = {
#include "game/008D0BE0.inc"
};
u32 StageObj_K12_Curves_21[28] = {
#include "game/008D0C40.inc"
};
u32 StageObj_K12_Curves_22[20] = {
#include "game/008D0CB0.inc"
};
u32 StageObj_K12_Curves_23[16] = {
#include "game/008D0D00.inc"
};
u32 StageObj_K12_Curves_24[12] = {
#include "game/008D0D40.inc"
};
u32 StageObj_K12_Curves_25[20] = {
#include "game/008D0D70.inc"
};
u32 StageObj_K12_Curves_26[20] = {
#include "game/008D0DC0.inc"
};
u32 StageObj_K12_Curves_29[12] = {
#include "game/008D0E10.inc"
};
u32 StageObj_K12_Curves_30[16] = {
#include "game/008D0E40.inc"
};
u32 StageObj_K12_Curves_31[18] = {
#include "game/008D0E80.inc"
};
s16 StageObj_K12_DelaysA[6] = {
#include "game/008D0EC8.inc"
};
s16 StageObj_K12_DursA[6] = {
#include "game/008D0ED8.inc"
};
u8 StageObj_K12_GunlanceShellArgs[8] = {
#include "game/008D0EE8.inc"
};
void *StageObj_K12_Curves[34] = {
    (void *)StageObj_K12_Curves_0, (void *)StageObj_K12_Curves_1, (void *)StageObj_K12_Curves_2, (void *)StageObj_K12_Curves_2,
    (void *)StageObj_K12_Curves_4, (void *)StageObj_K12_Curves_5, (void *)StageObj_K12_Curves_6, (void *)StageObj_K12_Curves_6,
    (void *)StageObj_K12_Curves_8, (void *)StageObj_K12_Curves_9, (void *)StageObj_K12_Curves_10, (void *)StageObj_K12_Curves_10,
    (void *)StageObj_K12_Curves_12, (void *)StageObj_K12_Curves_13, (void *)StageObj_K12_Curves_14, (void *)StageObj_K12_Curves_15,
    (void *)StageObj_K12_Curves_16, (void *)StageObj_K12_Curves_17, (void *)StageObj_K12_Curves_18, (void *)StageObj_K12_Curves_19,
    (void *)StageObj_K12_Curves_19, (void *)StageObj_K12_Curves_21, (void *)StageObj_K12_Curves_22, (void *)StageObj_K12_Curves_23,
    (void *)StageObj_K12_Curves_24, (void *)StageObj_K12_Curves_25, (void *)StageObj_K12_Curves_26, (void *)StageObj_K12_Curves_26,
    (void *)StageObj_K12_Curves_26, (void *)StageObj_K12_Curves_29, (void *)StageObj_K12_Curves_30, (void *)StageObj_K12_Curves_31,
    (void *)StageObj_K12_Curves_31, (void *)StageObj_K12_Curves_31,
};
s16 StageObj_K12_DursB[6] = {
#include "game/008D0F78.inc"
};
f32 StageObj_K12_Scales[3] = {
#include "game/008D0F88.inc"
};
f32 StageObj_K12_Spin[3] = {
#include "game/008D0F98.inc"
};
u32 StageObj_K12_Tbls_0_0[8] = {
#include "game/008D0FB0.inc"
};
u32 StageObj_K12_Tbls_2_0[8] = {
#include "game/008D0FD0.inc"
};
u32 StageObj_K12_Tbls_3_0[12] = {
#include "game/008D0FF0.inc"
};
u32 StageObj_K12_Tbls_4_0[8] = {
#include "game/008D1020.inc"
};
u32 StageObj_K12_Tbls_0_1[8] = {
#include "game/008D1040.inc"
};
u32 StageObj_K12_Tbls_2_1[8] = {
#include "game/008D1060.inc"
};
u32 StageObj_K12_Tbls_3_1[12] = {
#include "game/008D1080.inc"
};
u32 StageObj_K12_Tbls_4_1[8] = {
#include "game/008D10B0.inc"
};
u32 StageObj_K12_Tbls_0_2[8] = {
#include "game/008D10D0.inc"
};
u32 StageObj_K12_Tbls_2_2[8] = {
#include "game/008D10F0.inc"
};
u32 StageObj_K12_Tbls_3_2[12] = {
#include "game/008D1110.inc"
};
u32 StageObj_K12_Tbls_4_2[8] = {
#include "game/008D1140.inc"
};
u32 StageObj_K12_Tbls_0_3[8] = {
#include "game/008D1160.inc"
};
u32 StageObj_K12_Tbls_2_3[8] = {
#include "game/008D1180.inc"
};
u32 StageObj_K12_Tbls_3_3[12] = {
#include "game/008D11A0.inc"
};
u32 StageObj_K12_Tbls_4_3[8] = {
#include "game/008D11D0.inc"
};
u32 StageObj_K12_Tbls_0_4[8] = {
#include "game/008D11F0.inc"
};
u32 StageObj_K12_Tbls_2_4[8] = {
#include "game/008D1210.inc"
};
u32 StageObj_K12_Tbls_3_4[12] = {
#include "game/008D1230.inc"
};
u32 StageObj_K12_Tbls_4_4[8] = {
#include "game/008D1260.inc"
};
u32 StageObj_K12_Tbls_0_5[8] = {
#include "game/008D1280.inc"
};
u32 StageObj_K12_Tbls_2_5[8] = {
#include "game/008D12A0.inc"
};
u32 StageObj_K12_Tbls_3_5[12] = {
#include "game/008D12C0.inc"
};
u32 StageObj_K12_Tbls_4_5[8] = {
#include "game/008D12F0.inc"
};
u32 StageObj_K12_Tbls_0_6[8] = {
#include "game/008D1310.inc"
};
u32 StageObj_K12_Tbls_2_6[8] = {
#include "game/008D1330.inc"
};
u32 StageObj_K12_Tbls_3_6[12] = {
#include "game/008D1350.inc"
};
u32 StageObj_K12_Tbls_4_6[8] = {
#include "game/008D1380.inc"
};
u32 StageObj_K12_Tbls_0_7[8] = {
#include "game/008D13A0.inc"
};
u32 StageObj_K12_Tbls_2_7[8] = {
#include "game/008D13C0.inc"
};
u32 StageObj_K12_Tbls_3_7[12] = {
#include "game/008D13E0.inc"
};
u32 StageObj_K12_Tbls_4_7[8] = {
#include "game/008D1410.inc"
};
u32 StageObj_K12_Tbls_0_8[8] = {
#include "game/008D1430.inc"
};
u32 StageObj_K12_Tbls_2_8[8] = {
#include "game/008D1450.inc"
};
u32 StageObj_K12_Tbls_3_8[12] = {
#include "game/008D1470.inc"
};
u32 StageObj_K12_Tbls_4_8[8] = {
#include "game/008D14A0.inc"
};
u32 StageObj_K12_Tbls_0_9[8] = {
#include "game/008D14C0.inc"
};
u32 StageObj_K12_Tbls_2_9[8] = {
#include "game/008D14E0.inc"
};
u32 StageObj_K12_Tbls_3_9[12] = {
#include "game/008D1500.inc"
};
u32 StageObj_K12_Tbls_4_9[8] = {
#include "game/008D1530.inc"
};
u32 StageObj_K12_Tbls_0_10[8] = {
#include "game/008D1550.inc"
};
u32 StageObj_K12_Tbls_2_10[8] = {
#include "game/008D1570.inc"
};
u32 StageObj_K12_Tbls_3_10[12] = {
#include "game/008D1590.inc"
};
u32 StageObj_K12_Tbls_4_10[8] = {
#include "game/008D15C0.inc"
};
u32 StageObj_K12_Tbls_0_11[8] = {
#include "game/008D15E0.inc"
};
u32 StageObj_K12_Tbls_2_11[8] = {
#include "game/008D1600.inc"
};
u32 StageObj_K12_Tbls_3_11[12] = {
#include "game/008D1620.inc"
};
u32 StageObj_K12_Tbls_4_11[8] = {
#include "game/008D1650.inc"
};
u32 StageObj_K12_Tbls_0_12[8] = {
#include "game/008D1670.inc"
};
u32 StageObj_K12_Tbls_2_12[8] = {
#include "game/008D1690.inc"
};
u32 StageObj_K12_Tbls_3_12[12] = {
#include "game/008D16B0.inc"
};
u32 StageObj_K12_Tbls_4_12[8] = {
#include "game/008D16E0.inc"
};
u32 StageObj_K12_Tbls_0_13[8] = {
#include "game/008D1700.inc"
};
u32 StageObj_K12_Tbls_2_13[8] = {
#include "game/008D1720.inc"
};
u32 StageObj_K12_Tbls_3_13[12] = {
#include "game/008D1740.inc"
};
u32 StageObj_K12_Tbls_4_13[8] = {
#include "game/008D1770.inc"
};
u8 *StageObj_K12_Tbls_0[16] = {
    (u8 *)StageObj_K12_Tbls_0_0, (u8 *)StageObj_K12_Tbls_0_1, (u8 *)StageObj_K12_Tbls_0_2, (u8 *)StageObj_K12_Tbls_0_3,
    (u8 *)StageObj_K12_Tbls_0_4, (u8 *)StageObj_K12_Tbls_0_5, (u8 *)StageObj_K12_Tbls_0_6, (u8 *)StageObj_K12_Tbls_0_7,
    (u8 *)StageObj_K12_Tbls_0_8, (u8 *)StageObj_K12_Tbls_0_9, (u8 *)StageObj_K12_Tbls_0_10, (u8 *)StageObj_K12_Tbls_0_11,
    (u8 *)StageObj_K12_Tbls_0_12, (u8 *)StageObj_K12_Tbls_0_13, NULL, NULL,
};
u8 *StageObj_K12_Tbls_2[16] = {
    (u8 *)StageObj_K12_Tbls_2_0, (u8 *)StageObj_K12_Tbls_2_1, (u8 *)StageObj_K12_Tbls_2_2, (u8 *)StageObj_K12_Tbls_2_3,
    (u8 *)StageObj_K12_Tbls_2_4, (u8 *)StageObj_K12_Tbls_2_5, (u8 *)StageObj_K12_Tbls_2_6, (u8 *)StageObj_K12_Tbls_2_7,
    (u8 *)StageObj_K12_Tbls_2_8, (u8 *)StageObj_K12_Tbls_2_9, (u8 *)StageObj_K12_Tbls_2_10, (u8 *)StageObj_K12_Tbls_2_11,
    (u8 *)StageObj_K12_Tbls_2_12, (u8 *)StageObj_K12_Tbls_2_13, NULL, NULL,
};
u8 *StageObj_K12_Tbls_3[16] = {
    (u8 *)StageObj_K12_Tbls_3_0, (u8 *)StageObj_K12_Tbls_3_1, (u8 *)StageObj_K12_Tbls_3_2, (u8 *)StageObj_K12_Tbls_3_3,
    (u8 *)StageObj_K12_Tbls_3_4, (u8 *)StageObj_K12_Tbls_3_5, (u8 *)StageObj_K12_Tbls_3_6, (u8 *)StageObj_K12_Tbls_3_7,
    (u8 *)StageObj_K12_Tbls_3_8, (u8 *)StageObj_K12_Tbls_3_9, (u8 *)StageObj_K12_Tbls_3_10, (u8 *)StageObj_K12_Tbls_3_11,
    (u8 *)StageObj_K12_Tbls_3_12, (u8 *)StageObj_K12_Tbls_3_13, NULL, NULL,
};
u8 *StageObj_K12_Tbls_4[16] = {
    (u8 *)StageObj_K12_Tbls_4_0, (u8 *)StageObj_K12_Tbls_4_1, (u8 *)StageObj_K12_Tbls_4_2, (u8 *)StageObj_K12_Tbls_4_3,
    (u8 *)StageObj_K12_Tbls_4_4, (u8 *)StageObj_K12_Tbls_4_5, (u8 *)StageObj_K12_Tbls_4_6, (u8 *)StageObj_K12_Tbls_4_7,
    (u8 *)StageObj_K12_Tbls_4_8, (u8 *)StageObj_K12_Tbls_4_9, (u8 *)StageObj_K12_Tbls_4_10, (u8 *)StageObj_K12_Tbls_4_11,
    (u8 *)StageObj_K12_Tbls_4_12, (u8 *)StageObj_K12_Tbls_4_13, NULL, NULL,
};
u8* *StageObj_K12_Tbls[6] = {
    StageObj_K12_Tbls_0, NULL, StageObj_K12_Tbls_2, StageObj_K12_Tbls_3,
    StageObj_K12_Tbls_4, NULL,
};
s16 StageObj_K12_DursC[6] = {
#include "game/008D18A8.inc"
};
u8 StageObj_K12_ColorKeysA_0[48] = {
#include "game/008D18C0.inc"
};
u8 StageObj_K12_ColorKeysA_1[48] = {
#include "game/008D18F0.inc"
};
u8 StageObj_K12_ColorKeysA_2[48] = {
#include "game/008D1920.inc"
};
u8 StageObj_K12_ColorKeysA_3[48] = {
#include "game/008D1950.inc"
};
u8 StageObj_K12_ColorKeysA_4[48] = {
#include "game/008D1980.inc"
};
u8 StageObj_K12_ColorKeysA_5[48] = {
#include "game/008D19B0.inc"
};
u8 StageObj_K12_ColorKeysA_6[48] = {
#include "game/008D19E0.inc"
};
u8 StageObj_K12_ColorKeysA_7[48] = {
#include "game/008D1A10.inc"
};
u8 StageObj_K12_ColorKeysA_8[48] = {
#include "game/008D1A40.inc"
};
u8 StageObj_K12_ColorKeysA_9[48] = {
#include "game/008D1A70.inc"
};
u8 StageObj_K12_ColorKeysA_12[48] = {
#include "game/008D1AA0.inc"
};
u8 *StageObj_K12_ColorKeysA[16] = {
    StageObj_K12_ColorKeysA_0, StageObj_K12_ColorKeysA_1, StageObj_K12_ColorKeysA_2, StageObj_K12_ColorKeysA_3,
    StageObj_K12_ColorKeysA_4, StageObj_K12_ColorKeysA_5, StageObj_K12_ColorKeysA_6, StageObj_K12_ColorKeysA_7,
    StageObj_K12_ColorKeysA_8, StageObj_K12_ColorKeysA_9, StageObj_K12_ColorKeysA_9, StageObj_K12_ColorKeysA_9,
    StageObj_K12_ColorKeysA_12, NULL, NULL, NULL,
};
u8 StageObj_K12_ColorKeysB_0[48] = {
#include "game/008D1B10.inc"
};
u8 StageObj_K12_ColorKeysB_1[48] = {
#include "game/008D1B40.inc"
};
u8 StageObj_K12_ColorKeysB_2[48] = {
#include "game/008D1B70.inc"
};
u8 StageObj_K12_ColorKeysB_3[48] = {
#include "game/008D1BA0.inc"
};
u8 StageObj_K12_ColorKeysB_4[48] = {
#include "game/008D1BD0.inc"
};
u8 StageObj_K12_ColorKeysB_5[48] = {
#include "game/008D1C00.inc"
};
u8 StageObj_K12_ColorKeysB_6[48] = {
#include "game/008D1C30.inc"
};
u8 StageObj_K12_ColorKeysB_7[48] = {
#include "game/008D1C60.inc"
};
u8 StageObj_K12_ColorKeysB_8[48] = {
#include "game/008D1C90.inc"
};
u8 *StageObj_K12_ColorKeysB[12] = {
    StageObj_K12_ColorKeysB_0, StageObj_K12_ColorKeysB_1, StageObj_K12_ColorKeysB_2, StageObj_K12_ColorKeysB_3,
    StageObj_K12_ColorKeysB_4, StageObj_K12_ColorKeysB_5, StageObj_K12_ColorKeysB_6, StageObj_K12_ColorKeysB_7,
    StageObj_K12_ColorKeysB_8, NULL, NULL, NULL,
};
u8 StageObj_K12_ColorKeysC_0[32] = {
#include "game/008D1CF0.inc"
};
u8 StageObj_K12_ColorKeysC_1[32] = {
#include "game/008D1D10.inc"
};
u8 StageObj_K12_ColorKeysC_2[48] = {
#include "game/008D1D30.inc"
};
u8 StageObj_K12_ColorKeysC_3[32] = {
#include "game/008D1D60.inc"
};
u8 *StageObj_K12_ColorKeysC[4] = {
    StageObj_K12_ColorKeysC_0, StageObj_K12_ColorKeysC_1, StageObj_K12_ColorKeysC_2, StageObj_K12_ColorKeysC_3,
};
s16 StageObj_K12_PartCounts[16] = {
#include "game/008D1D90.inc"
};
s16 StageObj_K12_Limits[16] = {
#include "game/008D1DB0.inc"
};
s16 StageObj_K12_CurveIdx[16] = {
#include "game/008D1DD0.inc"
};
f32 StageObj_K13_EmOffsets_1[4] = {
#include "game/008D1DF0.inc"
};
f32 StageObj_K13_EmOffsets_6[4] = {
#include "game/008D1E00.inc"
};
f32 StageObj_K13_EmOffsets_14[4] = {
#include "game/008D1E10.inc"
};
f32 *StageObj_K13_EmOffsets[74] = {
    (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_1, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_6, (f32 *)D_008F5CD8,
    StageObj_K13_EmOffsets_1, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_1,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_14, StageObj_K13_EmOffsets_1,
    (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_14, StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_1,
    StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_6, StageObj_K13_EmOffsets_14, StageObj_K13_EmOffsets_1,
    StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_14, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_1, (f32 *)D_008F5CD8,
    StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_6, StageObj_K13_EmOffsets_1,
    StageObj_K13_EmOffsets_6, StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_14,
    StageObj_K13_EmOffsets_14, StageObj_K13_EmOffsets_1, StageObj_K13_EmOffsets_6, StageObj_K13_EmOffsets_14,
    (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_1, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_14,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_1,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, (f32 *)D_008F5CD8, StageObj_K13_EmOffsets_1,
    (f32 *)D_008F5CD8, (f32 *)D_008F5CD8,
};
f32 D_008D1F48[3] = {
#include "game/008D1F48.inc"
};
f32 *StageObj_K13_Offset = D_008D1F48;
f32 StageObj_K13_ScaleKeys[32] = {
#include "game/008D1F60.inc"
};
f32 StageObj_K13_MoveKeys[36] = {
#include "game/008D1FE0.inc"
};
f32 StageObj_K13_ScaleKeysB[22] = {
#include "game/008D2070.inc"
};
s16 StageObj_K13_Models[4] = {
#include "game/008D20C8.inc"
};
u32 StageObj_K14_Curves_0[20] = {
#include "game/008D20D0.inc"
};
u32 StageObj_K14_Curves_1[12] = {
#include "game/008D2120.inc"
};
u32 StageObj_K14_Curves_2[8] = {
#include "game/008D2150.inc"
};
u32 StageObj_K14_Curves_3[20] = {
#include "game/008D2170.inc"
};
u32 StageObj_K14_Curves_4[20] = {
#include "game/008D21C0.inc"
};
u32 StageObj_K14_Curves_5[10] = {
#include "game/008D2210.inc"
};
s16 StageObj_K14_DelayLists_1[6] = {
#include "game/008D2238.inc"
};
u32 StageObj_K14_Curves_6[20] = {
#include "game/008D2250.inc"
};
u32 StageObj_K14_Curves_9[20] = {
#include "game/008D22A0.inc"
};
u32 StageObj_K14_Curves_12[24] = {
#include "game/008D22F0.inc"
};
u32 StageObj_K14_Curves_15[20] = {
#include "game/008D2350.inc"
};
u32 StageObj_K14_Curves_7[16] = {
#include "game/008D23A0.inc"
};
u32 StageObj_K14_Curves_10[16] = {
#include "game/008D23E0.inc"
};
u32 StageObj_K14_Curves_13[24] = {
#include "game/008D2420.inc"
};
u32 StageObj_K14_Curves_16[20] = {
#include "game/008D2480.inc"
};
u32 StageObj_K14_Curves_8[8] = {
#include "game/008D24D0.inc"
};
u32 StageObj_K14_Curves_11[8] = {
#include "game/008D24F0.inc"
};
u32 StageObj_K14_Curves_14[8] = {
#include "game/008D2510.inc"
};
u32 StageObj_K14_Curves_17[10] = {
#include "game/008D2530.inc"
};
s16 StageObj_K14_DelayLists_2[4] = {
#include "game/008D2558.inc"
};
u16 StageObj_K14_DurLists_2[8] = {
#include "game/008D2560.inc"
};
u32 StageObj_K14_Curves_18[24] = {
#include "game/008D2570.inc"
};
u32 StageObj_K14_Curves_21[24] = {
#include "game/008D25D0.inc"
};
u32 StageObj_K14_Curves_19[12] = {
#include "game/008D2630.inc"
};
u32 StageObj_K14_Curves_22[16] = {
#include "game/008D2660.inc"
};
u32 StageObj_K14_Curves_20[12] = {
#include "game/008D26A0.inc"
};
u32 StageObj_K14_Curves_23[10] = {
#include "game/008D26D0.inc"
};
s16 StageObj_K14_DelayLists_4[4] = {
#include "game/008D26F8.inc"
};
u16 StageObj_K14_DurLists_4[8] = {
#include "game/008D2700.inc"
};
u32 StageObj_K14_Curves_24[24] = {
#include "game/008D2710.inc"
};
u32 StageObj_K14_Curves_27[24] = {
#include "game/008D2770.inc"
};
u32 StageObj_K14_Curves_30[24] = {
#include "game/008D27D0.inc"
};
u32 StageObj_K14_Curves_33[24] = {
#include "game/008D2830.inc"
};
u32 StageObj_K14_Curves_36[24] = {
#include "game/008D2890.inc"
};
u32 StageObj_K14_Curves_39[24] = {
#include "game/008D28F0.inc"
};
u32 StageObj_K14_Curves_42[24] = {
#include "game/008D2950.inc"
};
u32 StageObj_K14_Curves_25[16] = {
#include "game/008D29B0.inc"
};
u32 StageObj_K14_Curves_28[16] = {
#include "game/008D29F0.inc"
};
u32 StageObj_K14_Curves_31[24] = {
#include "game/008D2A30.inc"
};
u32 StageObj_K14_Curves_34[24] = {
#include "game/008D2A90.inc"
};
u32 StageObj_K14_Curves_37[24] = {
#include "game/008D2AF0.inc"
};
u32 StageObj_K14_Curves_40[20] = {
#include "game/008D2B50.inc"
};
u32 StageObj_K14_Curves_43[20] = {
#include "game/008D2BA0.inc"
};
u32 StageObj_K14_Curves_26[12] = {
#include "game/008D2BF0.inc"
};
u32 StageObj_K14_Curves_29[12] = {
#include "game/008D2C20.inc"
};
u32 StageObj_K14_Curves_32[12] = {
#include "game/008D2C50.inc"
};
u32 StageObj_K14_Curves_35[12] = {
#include "game/008D2C80.inc"
};
u32 StageObj_K14_Curves_38[12] = {
#include "game/008D2CB0.inc"
};
u32 StageObj_K14_Curves_41[12] = {
#include "game/008D2CE0.inc"
};
u32 StageObj_K14_Curves_44[20] = {
#include "game/008D2D10.inc"
};
u32 StageObj_K14_Curves_45[24] = {
#include "game/008D2D60.inc"
};
u32 StageObj_K14_Curves_48[24] = {
#include "game/008D2DC0.inc"
};
u32 StageObj_K14_Curves_51[24] = {
#include "game/008D2E20.inc"
};
u32 StageObj_K14_Curves_46[20] = {
#include "game/008D2E80.inc"
};
u32 StageObj_K14_Curves_49[20] = {
#include "game/008D2ED0.inc"
};
u32 StageObj_K14_Curves_52[20] = {
#include "game/008D2F20.inc"
};
u32 StageObj_K14_Curves_47[8] = {
#include "game/008D2F70.inc"
};
u32 StageObj_K14_Curves_50[8] = {
#include "game/008D2F90.inc"
};
u32 StageObj_K14_Curves_53[8] = {
#include "game/008D2FB0.inc"
};
s16 StageObj_K14_DelayLists_6[4] = {
#include "game/008D2FD0.inc"
};
u16 StageObj_K14_DurLists_6[4] = {
#include "game/008D2FD8.inc"
};
u32 StageObj_K14_Curves_54[24] = {
#include "game/008D2FE0.inc"
};
u32 StageObj_K14_Curves_55[8] = {
#include "game/008D3040.inc"
};
u32 StageObj_K14_Curves_56[28] = {
#include "game/008D3060.inc"
};
u32 StageObj_K14_Curves_59[28] = {
#include "game/008D30D0.inc"
};
u32 StageObj_K14_Curves_62[28] = {
#include "game/008D3140.inc"
};
u32 StageObj_K14_Curves_65[28] = {
#include "game/008D31B0.inc"
};
u32 StageObj_K14_Curves_68[28] = {
#include "game/008D3220.inc"
};
u32 StageObj_K14_Curves_57[28] = {
#include "game/008D3290.inc"
};
u32 StageObj_K14_Curves_60[28] = {
#include "game/008D3300.inc"
};
u32 StageObj_K14_Curves_63[28] = {
#include "game/008D3370.inc"
};
u32 StageObj_K14_Curves_66[16] = {
#include "game/008D33E0.inc"
};
u32 StageObj_K14_Curves_69[16] = {
#include "game/008D3420.inc"
};
u32 StageObj_K14_Curves_58[8] = {
#include "game/008D3460.inc"
};
u32 StageObj_K14_Curves_61[8] = {
#include "game/008D3480.inc"
};
u32 StageObj_K14_Curves_64[8] = {
#include "game/008D34A0.inc"
};
u32 StageObj_K14_Curves_67[8] = {
#include "game/008D34C0.inc"
};
u32 StageObj_K14_Curves_70[8] = {
#include "game/008D34E0.inc"
};
s16 StageObj_K14_DelayLists_8[8] = {
#include "game/008D3500.inc"
};
u16 StageObj_K14_DurLists_8[8] = {
#include "game/008D3510.inc"
};
u32 StageObj_K14_Curves_71[16] = {
#include "game/008D3520.inc"
};
u32 StageObj_K14_Curves_74[16] = {
#include "game/008D3560.inc"
};
u32 StageObj_K14_Curves_80[16] = {
#include "game/008D35A0.inc"
};
u32 StageObj_K14_Curves_86[16] = {
#include "game/008D35E0.inc"
};
u32 StageObj_K14_Curves_72[12] = {
#include "game/008D3620.inc"
};
u32 StageObj_K14_Curves_75[12] = {
#include "game/008D3650.inc"
};
u32 StageObj_K14_Curves_78[12] = {
#include "game/008D3680.inc"
};
u32 StageObj_K14_Curves_81[12] = {
#include "game/008D36B0.inc"
};
u32 StageObj_K14_Curves_84[12] = {
#include "game/008D36E0.inc"
};
u32 StageObj_K14_Curves_87[12] = {
#include "game/008D3710.inc"
};
u32 StageObj_K14_Curves_90[12] = {
#include "game/008D3740.inc"
};
u32 StageObj_K14_Curves_93[12] = {
#include "game/008D3770.inc"
};
u32 StageObj_K14_Curves_73[8] = {
#include "game/008D37A0.inc"
};
u32 StageObj_K14_Curves_76[8] = {
#include "game/008D37C0.inc"
};
u32 StageObj_K14_Curves_82[8] = {
#include "game/008D37E0.inc"
};
u32 StageObj_K14_Curves_88[8] = {
#include "game/008D3800.inc"
};
s16 StageObj_K14_DelayLists_9[8] = {
#include "game/008D3820.inc"
};
u16 StageObj_K14_DurLists_9[8] = {
#include "game/008D3830.inc"
};
u32 StageObj_K14_Curves_95[24] = {
#include "game/008D3840.inc"
};
u32 StageObj_K14_Curves_98[20] = {
#include "game/008D38A0.inc"
};
u32 StageObj_K14_Curves_96[16] = {
#include "game/008D38F0.inc"
};
u32 StageObj_K14_Curves_99[20] = {
#include "game/008D3930.inc"
};
u32 StageObj_K14_Curves_97[12] = {
#include "game/008D3980.inc"
};
u32 StageObj_K14_Curves_100[8] = {
#include "game/008D39B0.inc"
};
u16 StageObj_K14_DurLists_10[8] = {
#include "game/008D39D0.inc"
};
u32 StageObj_K14_Curves_101[20] = {
#include "game/008D39E0.inc"
};
u32 StageObj_K14_Curves_102[20] = {
#include "game/008D3A30.inc"
};
u32 StageObj_K14_Curves_103[10] = {
#include "game/008D3A80.inc"
};
s16 StageObj_K14_DelayLists_11[6] = {
#include "game/008D3AA8.inc"
};
u32 StageObj_K14_Curves_104[16] = {
#include "game/008D3AC0.inc"
};
u32 StageObj_K14_Curves_105[16] = {
#include "game/008D3B00.inc"
};
u32 StageObj_K14_Curves_106[8] = {
#include "game/008D3B40.inc"
};
u32 StageObj_K14_Curves_107[12] = {
#include "game/008D3B60.inc"
};
u32 StageObj_K14_Curves_108[12] = {
#include "game/008D3B90.inc"
};
u8 StageObj_K14_ColorKeys[32] = {
#include "game/008D3BC0.inc"
};
u32 StageObj_K14_Curves_109[20] = {
#include "game/008D3BE0.inc"
};
u32 StageObj_K14_Curves_112[20] = {
#include "game/008D3C30.inc"
};
u32 StageObj_K14_Curves_110[16] = {
#include "game/008D3C80.inc"
};
u32 StageObj_K14_Curves_113[12] = {
#include "game/008D3CC0.inc"
};
u32 StageObj_K14_Curves_111[8] = {
#include "game/008D3CF0.inc"
};
u32 StageObj_K14_Curves_114[8] = {
#include "game/008D3D10.inc"
};
s16 StageObj_K14_DelayLists_14[4] = {
#include "game/008D3D30.inc"
};
u16 StageObj_K14_DurLists_14[4] = {
#include "game/008D3D38.inc"
};
u32 StageObj_K14_Curves_115[28] = {
#include "game/008D3D40.inc"
};
u32 StageObj_K14_Curves_118[28] = {
#include "game/008D3DB0.inc"
};
u32 StageObj_K14_Curves_121[28] = {
#include "game/008D3E20.inc"
};
u32 StageObj_K14_Curves_116[28] = {
#include "game/008D3E90.inc"
};
u32 StageObj_K14_Curves_119[28] = {
#include "game/008D3F00.inc"
};
u32 StageObj_K14_Curves_122[28] = {
#include "game/008D3F70.inc"
};
u32 StageObj_K14_Curves_117[8] = {
#include "game/008D3FE0.inc"
};
u32 StageObj_K14_Curves_120[8] = {
#include "game/008D4000.inc"
};
u32 StageObj_K14_Curves_123[6] = {
#include "game/008D4020.inc"
};
s16 StageObj_K14_DelayLists_15[4] = {
#include "game/008D4038.inc"
};
u16 StageObj_K14_DurLists_15[8] = {
#include "game/008D4040.inc"
};
u32 StageObj_K14_Curves_124[12] = {
#include "game/008D4050.inc"
};
u32 StageObj_K14_Curves_125[12] = {
#include "game/008D4080.inc"
};
u32 StageObj_K14_Curves_126[24] = {
#include "game/008D40B0.inc"
};
u32 StageObj_K14_Curves_127[16] = {
#include "game/008D4110.inc"
};
u32 StageObj_K14_Curves_128[12] = {
#include "game/008D4150.inc"
};
f32 StageObj_K14_Drift[4] = {
#include "game/008D4180.inc"
};
u32 StageObj_K14_Curves_129[28] = {
#include "game/008D4190.inc"
};
u32 StageObj_K14_Curves_132[20] = {
#include "game/008D4200.inc"
};
u32 StageObj_K14_Curves_135[28] = {
#include "game/008D4250.inc"
};
u32 StageObj_K14_Curves_130[20] = {
#include "game/008D42C0.inc"
};
u32 StageObj_K14_Curves_133[20] = {
#include "game/008D4310.inc"
};
u32 StageObj_K14_Curves_136[20] = {
#include "game/008D4360.inc"
};
u32 StageObj_K14_Curves_131[8] = {
#include "game/008D43B0.inc"
};
u32 StageObj_K14_Curves_134[8] = {
#include "game/008D43D0.inc"
};
u32 StageObj_K14_Curves_137[8] = {
#include "game/008D43F0.inc"
};
u16 StageObj_K14_DurLists_19[8] = {
#include "game/008D4410.inc"
};
u32 StageObj_K14_Curves_138[24] = {
#include "game/008D4420.inc"
};
u32 StageObj_K14_Curves_141[16] = {
#include "game/008D4480.inc"
};
u32 StageObj_K14_Curves_139[28] = {
#include "game/008D44C0.inc"
};
u32 StageObj_K14_Curves_142[12] = {
#include "game/008D4530.inc"
};
u32 StageObj_K14_Curves_140[12] = {
#include "game/008D4560.inc"
};
u32 StageObj_K14_Curves_143[8] = {
#include "game/008D4590.inc"
};
s16 StageObj_K14_DelayLists_20[4] = {
#include "game/008D45B0.inc"
};
u16 StageObj_K14_DurLists_20[4] = {
#include "game/008D45B8.inc"
};
f32 StageObj_K14_Sizes[4] = {
#include "game/008D45C0.inc"
};
u32 StageObj_K14_Curves_150[16] = {
#include "game/008D45D0.inc"
};
u32 StageObj_K14_Curves_151[16] = {
#include "game/008D4610.inc"
};
u8 StageObj_K14_CurveDef[48] = {
#include "game/008D4650.inc"
};
u8 StageObj_K14_Curve1[48] = {
#include "game/008D4680.inc"
};
u32 StageObj_K14_Curves_152[20] = {
#include "game/008D46B0.inc"
};
u32 StageObj_K14_Curves_154[20] = {
#include "game/008D4700.inc"
};
u32 StageObj_K14_Curves_153[12] = {
#include "game/008D4750.inc"
};
u32 StageObj_K14_Curves_155[12] = {
#include "game/008D4780.inc"
};
u32 StageObj_K14_Curves_157[12] = {
#include "game/008D47B0.inc"
};
u32 StageObj_K14_Curves_159[12] = {
#include "game/008D47E0.inc"
};
u32 StageObj_K14_Curves_161[12] = {
#include "game/008D4810.inc"
};
u32 StageObj_K14_Curves_163[12] = {
#include "game/008D4840.inc"
};
u32 StageObj_K14_Curves_165[12] = {
#include "game/008D4870.inc"
};
u32 StageObj_K14_Curves_167[12] = {
#include "game/008D48A0.inc"
};
u8 StageObj_K14_CurveBA[32] = {
#include "game/008D48D0.inc"
};
u8 StageObj_K14_CurveBB[32] = {
#include "game/008D48F0.inc"
};
u8 StageObj_K14_Curve14A[32] = {
#include "game/008D4910.inc"
};
u8 StageObj_K14_Curve14B[32] = {
#include "game/008D4930.inc"
};
u8 StageObj_K14_Curve6A[32] = {
#include "game/008D4950.inc"
};
u8 StageObj_K14_Curve6B[32] = {
#include "game/008D4970.inc"
};
u8 StageObj_K14_CurveFA[32] = {
#include "game/008D4990.inc"
};
u8 StageObj_K14_CurveFB[32] = {
#include "game/008D49B0.inc"
};
u8 StageObj_K14_Curve11A[32] = {
#include "game/008D49D0.inc"
};
u8 StageObj_K14_Curve11B[32] = {
#include "game/008D49F0.inc"
};
u8 StageObj_K14_Curve16A[32] = {
#include "game/008D4A10.inc"
};
u8 StageObj_K14_Curve16B[32] = {
#include "game/008D4A30.inc"
};
u8 StageObj_K14_CurveDefA[32] = {
#include "game/008D4A50.inc"
};
u8 StageObj_K14_CurveDefB[32] = {
#include "game/008D4A70.inc"
};
u16 StageObj_K14_DurLists_25[8] = {
#include "game/008D4A90.inc"
};
u32 StageObj_K14_Curves_168[16] = {
#include "game/008D4AA0.inc"
};
u8 StageObj_K14_Curve31A[32] = {
#include "game/008D4AE0.inc"
};
u8 StageObj_K14_Curve31B[32] = {
#include "game/008D4B00.inc"
};
u8 StageObj_K14_Curve27[32] = {
#include "game/008D4B20.inc"
};
u8 StageObj_K14_Curve26A[32] = {
#include "game/008D4B40.inc"
};
u8 StageObj_K14_Curve26B[32] = {
#include "game/008D4B60.inc"
};
u8 StageObj_K14_Curve2D[32] = {
#include "game/008D4B80.inc"
};
u8 StageObj_K14_Curve2E[32] = {
#include "game/008D4BA0.inc"
};
u8 StageObj_K14_Curve22[32] = {
#include "game/008D4BC0.inc"
};
u8 StageObj_K14_Curve2F[32] = {
#include "game/008D4BE0.inc"
};
u8 StageObj_K14_Curve16[32] = {
#include "game/008D4C00.inc"
};
u8 StageObj_K14_CurveDef2[32] = {
#include "game/008D4C20.inc"
};
u8 StageObj_K14_Curve24A[32] = {
#include "game/008D4C40.inc"
};
u8 StageObj_K14_Curve24B[32] = {
#include "game/008D4C60.inc"
};
u32 StageObj_K14_Curves_169[24] = {
#include "game/008D4C80.inc"
};
u32 StageObj_K14_Curves_170[16] = {
#include "game/008D4CE0.inc"
};
u32 StageObj_K14_Curves_171[8] = {
#include "game/008D4D20.inc"
};
u32 StageObj_K14_Curves_172[20] = {
#include "game/008D4D40.inc"
};
u32 StageObj_K14_Curves_173[20] = {
#include "game/008D4D90.inc"
};
u8 StageObj_K14_ColorKeysByVar_0[48] = {
#include "game/008D4DE0.inc"
};
u8 StageObj_K14_ColorKeysByVar_1[48] = {
#include "game/008D4E10.inc"
};
u8 StageObj_K14_ColorKeysByVar_2[40] = {
#include "game/008D4E40.inc"
};
u8 *StageObj_K14_ColorKeysByVar[3] = {
    StageObj_K14_ColorKeysByVar_0, StageObj_K14_ColorKeysByVar_1, StageObj_K14_ColorKeysByVar_2,
};
s16 StageObj_K14_Sectors[6] = {
#include "game/008D4E78.inc"
};
void *StageObj_K14_Curves[176] = {
    (void *)StageObj_K14_Curves_0, (void *)StageObj_K14_Curves_1, (void *)StageObj_K14_Curves_2, (void *)StageObj_K14_Curves_3,
    (void *)StageObj_K14_Curves_4, (void *)StageObj_K14_Curves_5, (void *)StageObj_K14_Curves_6, (void *)StageObj_K14_Curves_7,
    (void *)StageObj_K14_Curves_8, (void *)StageObj_K14_Curves_9, (void *)StageObj_K14_Curves_10, (void *)StageObj_K14_Curves_11,
    (void *)StageObj_K14_Curves_12, (void *)StageObj_K14_Curves_13, (void *)StageObj_K14_Curves_14, (void *)StageObj_K14_Curves_15,
    (void *)StageObj_K14_Curves_16, (void *)StageObj_K14_Curves_17, (void *)StageObj_K14_Curves_18, (void *)StageObj_K14_Curves_19,
    (void *)StageObj_K14_Curves_20, (void *)StageObj_K14_Curves_21, (void *)StageObj_K14_Curves_22, (void *)StageObj_K14_Curves_23,
    (void *)StageObj_K14_Curves_24, (void *)StageObj_K14_Curves_25, (void *)StageObj_K14_Curves_26, (void *)StageObj_K14_Curves_27,
    (void *)StageObj_K14_Curves_28, (void *)StageObj_K14_Curves_29, (void *)StageObj_K14_Curves_30, (void *)StageObj_K14_Curves_31,
    (void *)StageObj_K14_Curves_32, (void *)StageObj_K14_Curves_33, (void *)StageObj_K14_Curves_34, (void *)StageObj_K14_Curves_35,
    (void *)StageObj_K14_Curves_36, (void *)StageObj_K14_Curves_37, (void *)StageObj_K14_Curves_38, (void *)StageObj_K14_Curves_39,
    (void *)StageObj_K14_Curves_40, (void *)StageObj_K14_Curves_41, (void *)StageObj_K14_Curves_42, (void *)StageObj_K14_Curves_43,
    (void *)StageObj_K14_Curves_44, (void *)StageObj_K14_Curves_45, (void *)StageObj_K14_Curves_46, (void *)StageObj_K14_Curves_47,
    (void *)StageObj_K14_Curves_48, (void *)StageObj_K14_Curves_49, (void *)StageObj_K14_Curves_50, (void *)StageObj_K14_Curves_51,
    (void *)StageObj_K14_Curves_52, (void *)StageObj_K14_Curves_53, (void *)StageObj_K14_Curves_54, (void *)StageObj_K14_Curves_55,
    (void *)StageObj_K14_Curves_56, (void *)StageObj_K14_Curves_57, (void *)StageObj_K14_Curves_58, (void *)StageObj_K14_Curves_59,
    (void *)StageObj_K14_Curves_60, (void *)StageObj_K14_Curves_61, (void *)StageObj_K14_Curves_62, (void *)StageObj_K14_Curves_63,
    (void *)StageObj_K14_Curves_64, (void *)StageObj_K14_Curves_65, (void *)StageObj_K14_Curves_66, (void *)StageObj_K14_Curves_67,
    (void *)StageObj_K14_Curves_68, (void *)StageObj_K14_Curves_69, (void *)StageObj_K14_Curves_70, (void *)StageObj_K14_Curves_71,
    (void *)StageObj_K14_Curves_72, (void *)StageObj_K14_Curves_73, (void *)StageObj_K14_Curves_74, (void *)StageObj_K14_Curves_75,
    (void *)StageObj_K14_Curves_76, (void *)StageObj_K14_Curves_74, (void *)StageObj_K14_Curves_78, (void *)StageObj_K14_Curves_76,
    (void *)StageObj_K14_Curves_80, (void *)StageObj_K14_Curves_81, (void *)StageObj_K14_Curves_82, (void *)StageObj_K14_Curves_80,
    (void *)StageObj_K14_Curves_84, (void *)StageObj_K14_Curves_82, (void *)StageObj_K14_Curves_86, (void *)StageObj_K14_Curves_87,
    (void *)StageObj_K14_Curves_88, (void *)StageObj_K14_Curves_80, (void *)StageObj_K14_Curves_90, (void *)StageObj_K14_Curves_82,
    (void *)StageObj_K14_Curves_80, (void *)StageObj_K14_Curves_93, (void *)StageObj_K14_Curves_82, (void *)StageObj_K14_Curves_95,
    (void *)StageObj_K14_Curves_96, (void *)StageObj_K14_Curves_97, (void *)StageObj_K14_Curves_98, (void *)StageObj_K14_Curves_99,
    (void *)StageObj_K14_Curves_100, (void *)StageObj_K14_Curves_101, (void *)StageObj_K14_Curves_102, (void *)StageObj_K14_Curves_103,
    (void *)StageObj_K14_Curves_104, (void *)StageObj_K14_Curves_105, (void *)StageObj_K14_Curves_106, (void *)StageObj_K14_Curves_107,
    (void *)StageObj_K14_Curves_108, (void *)StageObj_K14_Curves_109, (void *)StageObj_K14_Curves_110, (void *)StageObj_K14_Curves_111,
    (void *)StageObj_K14_Curves_112, (void *)StageObj_K14_Curves_113, (void *)StageObj_K14_Curves_114, (void *)StageObj_K14_Curves_115,
    (void *)StageObj_K14_Curves_116, (void *)StageObj_K14_Curves_117, (void *)StageObj_K14_Curves_118, (void *)StageObj_K14_Curves_119,
    (void *)StageObj_K14_Curves_120, (void *)StageObj_K14_Curves_121, (void *)StageObj_K14_Curves_122, (void *)StageObj_K14_Curves_123,
    (void *)StageObj_K14_Curves_124, (void *)StageObj_K14_Curves_125, (void *)StageObj_K14_Curves_126, (void *)StageObj_K14_Curves_127,
    (void *)StageObj_K14_Curves_128, (void *)StageObj_K14_Curves_129, (void *)StageObj_K14_Curves_130, (void *)StageObj_K14_Curves_131,
    (void *)StageObj_K14_Curves_132, (void *)StageObj_K14_Curves_133, (void *)StageObj_K14_Curves_134, (void *)StageObj_K14_Curves_135,
    (void *)StageObj_K14_Curves_136, (void *)StageObj_K14_Curves_137, (void *)StageObj_K14_Curves_138, (void *)StageObj_K14_Curves_139,
    (void *)StageObj_K14_Curves_140, (void *)StageObj_K14_Curves_141, (void *)StageObj_K14_Curves_142, (void *)StageObj_K14_Curves_143,
    (void *)StageObj_K14_Curves_141, (void *)StageObj_K14_Curves_142, (void *)StageObj_K14_Curves_143, (void *)StageObj_K14_Curves_141,
    (void *)StageObj_K14_Curves_142, (void *)StageObj_K14_Curves_143, (void *)StageObj_K14_Curves_150, (void *)StageObj_K14_Curves_151,
    (void *)StageObj_K14_Curves_152, (void *)StageObj_K14_Curves_153, (void *)StageObj_K14_Curves_154, (void *)StageObj_K14_Curves_155,
    (void *)StageObj_K14_Curves_152, (void *)StageObj_K14_Curves_157, (void *)StageObj_K14_Curves_154, (void *)StageObj_K14_Curves_159,
    (void *)StageObj_K14_Curves_152, (void *)StageObj_K14_Curves_161, (void *)StageObj_K14_Curves_154, (void *)StageObj_K14_Curves_163,
    (void *)StageObj_K14_Curves_152, (void *)StageObj_K14_Curves_165, (void *)StageObj_K14_Curves_154, (void *)StageObj_K14_Curves_167,
    (void *)StageObj_K14_Curves_168, (void *)StageObj_K14_Curves_169, (void *)StageObj_K14_Curves_170, (void *)StageObj_K14_Curves_171,
    (void *)StageObj_K14_Curves_172, (void *)StageObj_K14_Curves_173, NULL, NULL,
};
s16 StageObj_K14_PartCounts[32] = {
#include "game/008D5150.inc"
};
u16 StageObj_K14_Totals[32] = {
#include "game/008D5190.inc"
};
s16 *StageObj_K14_DelayLists[32] = {
    NULL, StageObj_K14_DelayLists_1, StageObj_K14_DelayLists_2, StageObj_K14_DelayLists_2,
    StageObj_K14_DelayLists_4, (s16 *)&StageObj_K14_Curves_44[10], StageObj_K14_DelayLists_6, NULL,
    StageObj_K14_DelayLists_8, StageObj_K14_DelayLists_9, NULL, StageObj_K14_DelayLists_11,
    NULL, NULL, StageObj_K14_DelayLists_14, StageObj_K14_DelayLists_15,
    NULL, NULL, NULL, NULL,
    StageObj_K14_DelayLists_20, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
u16 *StageObj_K14_DurLists[32] = {
    NULL, NULL, StageObj_K14_DurLists_2, StageObj_K14_DurLists_2,
    StageObj_K14_DurLists_4, (u16 *)&StageObj_K14_Curves_44[14], StageObj_K14_DurLists_6, NULL,
    StageObj_K14_DurLists_8, StageObj_K14_DurLists_9, StageObj_K14_DurLists_10, NULL,
    NULL, NULL, StageObj_K14_DurLists_14, StageObj_K14_DurLists_15,
    NULL, NULL, NULL, StageObj_K14_DurLists_19,
    StageObj_K14_DurLists_20, NULL, NULL, NULL,
    NULL, StageObj_K14_DurLists_25, NULL, NULL,
    NULL, NULL, NULL, NULL,
};
s16 StageObj_K14_CurveIdx[32] = {
#include "game/008D52D0.inc"
};
s16 StageObj_K14_Steps[32] = {
#include "game/008D5310.inc"
};
s16 StageObj_K14_GroundTbl[32] = {
#include "game/008D5350.inc"
};
u8 StageObj_K1B_PartCounts[8] = {
#include "game/008D5390.inc"
};
u8 StageObj_K1B_Models[8] = {
#include "game/008D5398.inc"
};
u16 StageObj_K1B_Lives[8] = {
#include "game/008D53A0.inc"
};
